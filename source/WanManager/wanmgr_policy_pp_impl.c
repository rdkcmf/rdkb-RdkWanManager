/*
   If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2019 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/* ---- Include Files ---------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "wanmgr_controller.h"
#include "wanmgr_data.h"
#include "wanmgr_rdkbus_utils.h"
#include "wanmgr_interface_sm.h"
#include "wanmgr_platform_events.h"

/* ---- Global Constants -------------------------- */
#define LOOP_TIMEOUT 500000 // timeout in milliseconds. This is the state machine loop interval


/* primary priority policy */
typedef enum {
    STATE_INTERFACE_DOWN = 0,
    STATE_PRIMARY_WAN_ACTIVE,
    STATE_SECONDARY_WAN_ACTIVE,
    STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP
} WcPpPolicyState_t;


/* STATES */
static WcPpPolicyState_t State_WanDown(WanMgr_Policy_Controller_t* pWanController);
static WcPpPolicyState_t State_PrimaryWanActive(WanMgr_Policy_Controller_t* pWanController);
static WcPpPolicyState_t State_SecondaryWanActive(WanMgr_Policy_Controller_t* pWanController);
static WcPpPolicyState_t State_PrimaryWanActiveSecondaryWanUp(WanMgr_Policy_Controller_t* pWanController);

/* TRANSITIONS */
static WcPpPolicyState_t Transition_Start(WanMgr_Policy_Controller_t* pWanController);
static WcPpPolicyState_t Transition_PrimaryInterfaceSelected(WanMgr_Policy_Controller_t* pWanController);
static WcPpPolicyState_t Transition_PrimaryInterfaceDeSelected(WanMgr_Policy_Controller_t* pWanController);
static WcPpPolicyState_t Transition_PrimaryInterfaceChanged(WanMgr_Policy_Controller_t* pWanController);
static WcPpPolicyState_t Transition_SecondaryInterfaceSelected(WanMgr_Policy_Controller_t* pWanController);
static WcPpPolicyState_t Transition_SecondaryInterfaceDeSelected(WanMgr_Policy_Controller_t* pWanController);
static WcPpPolicyState_t Transition_SecondaryInterfaceUp(WanMgr_Policy_Controller_t* pWanController);
static WcPpPolicyState_t Transition_SecondaryInterfaceDown(WanMgr_Policy_Controller_t* pWanController);


/*********************************************************************************/
/**************************** ACTIONS ********************************************/
/*********************************************************************************/
static void WanMgr_Policy_FM_SelectWANActive(WanMgr_Policy_Controller_t* pWanController, INT* pPrimaryInterface, INT* pSecondaryInterface)
{
    UINT uiLoopCount;
    UINT uiTotalIfaces = -1;
    INT iSelPrimaryInterface = -1;
    INT iSelSecondaryInterface = -1;
    INT iSelPrimaryPriority = DML_WAN_IFACE_PRIORITY_MAX;
    INT iSelSecondaryPriority = DML_WAN_IFACE_PRIORITY_MAX;

    //Get uiTotalIfaces
    WanMgr_IfaceCtrl_Data_t*   pWanIfaceCtrl = WanMgr_GetIfaceCtrl_locked();
    if(pWanIfaceCtrl != NULL)
    {
        uiTotalIfaces = pWanIfaceCtrl->ulTotalNumbWanInterfaces;

        WanMgrDml_GetIfaceCtrl_release(pWanIfaceCtrl);
    }

    if(uiTotalIfaces > 0)
    {
        // Check the policy to determine if any primary interface should be used for WAN
        if(pWanController->WanEnable == TRUE)
        {
            for( uiLoopCount = 0; uiLoopCount < uiTotalIfaces; uiLoopCount++ )
            {
                WanMgr_Iface_Data_t*   pWanDmlIfaceData = WanMgr_GetIfaceData_locked(uiLoopCount);
                if(pWanDmlIfaceData != NULL)
                {
                    DML_WAN_IFACE* pWanIfaceData = &(pWanDmlIfaceData->data);

                    if (pWanIfaceData->Wan.Enable == TRUE &&
                       (pWanIfaceData->Phy.Status == WAN_IFACE_PHY_STATUS_UP ||
                        pWanIfaceData->Phy.Status == WAN_IFACE_PHY_STATUS_INITIALIZING))
                    {
                        if(pWanIfaceData->Wan.Type == WAN_IFACE_TYPE_PRIMARY)
                        {
                            if(pWanIfaceData->Wan.Priority < iSelPrimaryPriority)
                            {
                                if(pWanIfaceData->Wan.Priority >= 0)
                                {
                                    iSelPrimaryInterface = uiLoopCount;
                                    iSelPrimaryPriority = pWanIfaceData->Wan.Priority;
                                }
                            }
                        }
                        else
                        {
                            if(pWanIfaceData->Wan.Priority < iSelSecondaryPriority)
                            {
                                if(pWanIfaceData->Wan.Priority >= 0)
                                {
                                    iSelSecondaryInterface = uiLoopCount;
                                    iSelSecondaryPriority = pWanIfaceData->Wan.Priority;
                                }
                            }
                        }
                    }

                    WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
                }
            }
        }
    }

    *pPrimaryInterface = iSelPrimaryInterface;
    *pSecondaryInterface = iSelSecondaryInterface;

    return ;
}

static bool WanMgr_CheckAllIfacesDown(void)
{
    bool bAllDown = TRUE;
    UINT uiLoopCount;
    UINT uiTotalIfaces = 0;

    //Get uiTotalIfaces
    WanMgr_IfaceCtrl_Data_t*   pWanIfaceCtrl = WanMgr_GetIfaceCtrl_locked();
    if(pWanIfaceCtrl != NULL)
    {
        uiTotalIfaces = pWanIfaceCtrl->ulTotalNumbWanInterfaces;

        WanMgrDml_GetIfaceCtrl_release(pWanIfaceCtrl);
    }
    else
    {
        bAllDown = FALSE;
    }

    if(uiTotalIfaces > 0)
    {
        for( uiLoopCount = 0; uiLoopCount < uiTotalIfaces; uiLoopCount++ )
        {
            WanMgr_Iface_Data_t*   pWanDmlIfaceData = WanMgr_GetIfaceData_locked(uiLoopCount);
            if(pWanDmlIfaceData != NULL)
            {
                DML_WAN_IFACE* pWanIfaceData = &(pWanDmlIfaceData->data);

                if (pWanIfaceData->Wan.Status != WAN_IFACE_STATUS_DISABLED)
                {
                    bAllDown = FALSE;
                }

                WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
            }
        }
    }

    return bAllDown;
}

/*********************************************************************************/
/************************** TRANSITIONS ******************************************/
/*********************************************************************************/
static WcPpPolicyState_t Transition_Start(WanMgr_Policy_Controller_t* pWanController)
{
    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    WanMgr_UpdatePlatformStatus(WANMGR_DISCONNECTED);

    CcspTraceInfo(("%s %d - State changed to STATE_INTERFACE_DOWN \n", __FUNCTION__, __LINE__));
    return STATE_INTERFACE_DOWN;
}

static WcPpPolicyState_t Transition_PrimaryInterfaceSelected(WanMgr_Policy_Controller_t* pWanController)
{
    DML_WAN_IFACE* pActiveInterface = NULL;
    WanMgr_IfaceSM_Controller_t wanIfCtrl;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* If Secondary WAN is Active */
    if(pWanController->pWanActiveIfaceData != NULL)
    {
        /* Secondary WAN is Active */
        pActiveInterface = &(pWanController->pWanActiveIfaceData->data);

        //Set ActiveLink to FALSE
        pActiveInterface->Wan.ActiveLink = FALSE;
    }


    /* Select Primary WAN as Active */
    WanMgr_Iface_Data_t*   pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pWanController->activeInterfaceIdx);
    if(pWanDmlIfaceData != NULL)
    {
        DML_WAN_IFACE* pWanIfaceData = &(pWanDmlIfaceData->data);

        //Set ActiveLink to TRUE
        pWanIfaceData->Wan.ActiveLink = TRUE;

        WanMgr_IfaceSM_Init(&wanIfCtrl, pWanIfaceData->uiIfaceIdx);

        WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
    }


    WanMgr_UpdatePlatformStatus(WANMGR_CONNECTING);

    /* Starts an instance of the WAN Interface State Machine on
    the interface to begin configuring the WAN link */
    WanMgr_StartInterfaceStateMachine(&wanIfCtrl);

/* TODO: Disabling the below code to avoid conflict issues in interface state machines as we are using
erouter0 name for both primary and secondary connections. Once this issue is fixed the below code can
be enabled */
#ifdef WAN_ENABLE_STANDBY
    /* If Secondary WAN is DOWN : Change state to PrimaryWANActive */
    if(pWanController->selSecondaryInterfaceIdx < 0)
    {
        CcspTraceInfo(("%s %d - State changed to STATE_PRIMARY_WAN_ACTIVE \n", __FUNCTION__, __LINE__));
        return STATE_PRIMARY_WAN_ACTIVE;
    }

    CcspTraceInfo(("%s %d - State changed to STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP \n", __FUNCTION__, __LINE__));
    return STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP;

#else
    CcspTraceInfo(("%s %d - State changed to STATE_PRIMARY_WAN_ACTIVE \n", __FUNCTION__, __LINE__));
    return STATE_PRIMARY_WAN_ACTIVE;
#endif //WAN_ENABLE_STANDBY
}

static WcPpPolicyState_t Transition_PrimaryInterfaceDeSelected(WanMgr_Policy_Controller_t* pWanController)
{
    DML_WAN_IFACE* pActiveInterface = NULL;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    if(pWanController->pWanActiveIfaceData == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }


    pActiveInterface = &(pWanController->pWanActiveIfaceData->data);

    //Set ActiveLink to FALSE
    pActiveInterface->Wan.ActiveLink = FALSE;


    pWanController->activeInterfaceIdx = -1;

    WanMgr_UpdatePlatformStatus(WANMGR_DISCONNECTED);

    CcspTraceInfo(("%s %d - State changed to STATE_INTERFACE_DOWN \n", __FUNCTION__, __LINE__));
    return STATE_INTERFACE_DOWN;
}

static WcPpPolicyState_t Transition_PrimaryInterfaceChanged(WanMgr_Policy_Controller_t* pWanController)
{
    DML_WAN_IFACE* pActiveInterface = NULL;
    int selectedPrimaryInterface = -1;
    int selectedSecondaryInterface = -1;
    WanMgr_IfaceSM_Controller_t wanIfCtrl;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    if(pWanController->pWanActiveIfaceData == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

    pActiveInterface = &(pWanController->pWanActiveIfaceData->data);

    //Set ActiveLink to FALSE
    pActiveInterface->Wan.ActiveLink = FALSE;

    pWanController->activeInterfaceIdx = -1;

    WanMgr_UpdatePlatformStatus(WANMGR_DISCONNECTED);

    WanMgr_Policy_FM_SelectWANActive(pWanController,&selectedPrimaryInterface, &selectedSecondaryInterface);
    if (selectedPrimaryInterface != -1)
    {
        pWanController->activeInterfaceIdx = selectedPrimaryInterface;

        /* Select New Primary WAN as Active */
        WanMgr_Iface_Data_t*   pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pWanController->activeInterfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanIfaceData = &(pWanDmlIfaceData->data);

            //Set ActiveLink to TRUE
            pWanIfaceData->Wan.ActiveLink = TRUE;
            WanMgr_IfaceSM_Init(&wanIfCtrl, pWanIfaceData->uiIfaceIdx);

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }


        WanMgr_UpdatePlatformStatus(WANMGR_CONNECTING);

        /* Starts an instance of the WAN Interface State Machine on
        the interface to begin configuring the WAN link */
        WanMgr_StartInterfaceStateMachine(&wanIfCtrl);
    }

    CcspTraceInfo(("%s %d - State changed to STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP \n", __FUNCTION__, __LINE__));
    return STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP;
}

static WcPpPolicyState_t Transition_SecondaryInterfaceSelected(WanMgr_Policy_Controller_t* pWanController)
{
    DML_WAN_IFACE* pActiveInterface = NULL;
    WanMgr_IfaceSM_Controller_t wanIfCtrl;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* If Primary WAN is Active */
    if(pWanController->pWanActiveIfaceData != NULL)
    {
        pActiveInterface = &(pWanController->pWanActiveIfaceData->data);

        //Set ActiveLink to FALSE
        pActiveInterface->Wan.ActiveLink = FALSE;
    }


    /* Select Secondary WAN as Active */
    pWanController->activeInterfaceIdx = pWanController->selSecondaryInterfaceIdx;
    WanMgr_Iface_Data_t*   pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pWanController->activeInterfaceIdx);
    if(pWanDmlIfaceData == NULL)
    {
        CcspTraceInfo(("%s %d - (Error) State changed to STATE_INTERFACE_DOWN \n", __FUNCTION__, __LINE__));
        return STATE_INTERFACE_DOWN;
    }

    DML_WAN_IFACE* pWanIfaceData = &(pWanDmlIfaceData->data);

    //Set ActiveLink to TRUE
    pWanIfaceData->Wan.ActiveLink = TRUE;
    WanMgr_IfaceSM_Init(&wanIfCtrl, pWanIfaceData->uiIfaceIdx);

    WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);


    WanMgr_UpdatePlatformStatus(WANMGR_CONNECTING);

    /* Starts an instance of the WAN Interface State Machine on
    the interface to begin configuring the WAN link */
    WanMgr_StartInterfaceStateMachine(&wanIfCtrl);

    CcspTraceInfo(("%s %d - State changed to STATE_SECONDARY_WAN_ACTIVE \n", __FUNCTION__, __LINE__));
    return STATE_SECONDARY_WAN_ACTIVE;
}

static WcPpPolicyState_t Transition_SecondaryInterfaceDeSelected(WanMgr_Policy_Controller_t* pWanController)
{
    DML_WAN_IFACE* pActiveInterface = NULL;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* If Secondary WAN is Active */
    if(pWanController->pWanActiveIfaceData != NULL)
    {
        pActiveInterface = &(pWanController->pWanActiveIfaceData->data);

        //Set ActiveLink to FALSE
        pActiveInterface->Wan.ActiveLink = FALSE;
    }

    pWanController->activeInterfaceIdx = -1;
    pWanController->selSecondaryInterfaceIdx = -1;


    WanMgr_UpdatePlatformStatus(WANMGR_DISCONNECTED);

    CcspTraceInfo(("%s %d - State changed to STATE_INTERFACE_DOWN \n", __FUNCTION__, __LINE__));
    return STATE_INTERFACE_DOWN;
}

static WcPpPolicyState_t Transition_SecondaryInterfaceUp(WanMgr_Policy_Controller_t* pWanController)
{
    DML_WAN_IFACE* pActiveInterface = NULL;
    bool bSecondaryUp = false;
    WanMgr_IfaceSM_Controller_t wanIfCtrl;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* Get Secondary WAN info */
    WanMgr_Iface_Data_t*   pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pWanController->selSecondaryInterfaceIdx);
    if(pWanDmlIfaceData != NULL)
    {
        DML_WAN_IFACE* pWanIfaceData = &(pWanDmlIfaceData->data);

        if (pWanIfaceData->Wan.Enable == TRUE &&
            pWanIfaceData->Wan.Status == WAN_IFACE_STATUS_DISABLED &&
            pWanIfaceData->Phy.Status == WAN_IFACE_PHY_STATUS_UP &&
            pWanIfaceData->Wan.LinkStatus == WAN_IFACE_LINKSTATUS_DOWN &&
            pWanIfaceData->Wan.Type == WAN_IFACE_TYPE_SECONDARY)
        {
            WanMgr_IfaceSM_Init(&wanIfCtrl, pWanIfaceData->uiIfaceIdx);
            bSecondaryUp = true;
        }

        WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
    }

    if(bSecondaryUp == false)
    {
        pWanController->selSecondaryInterfaceIdx = -1;
        CcspTraceInfo(("%s %d - State changed to STATE_PRIMARY_WAN_ACTIVE \n", __FUNCTION__, __LINE__));
        return STATE_PRIMARY_WAN_ACTIVE;
    }


    WanMgr_UpdatePlatformStatus(WANMGR_CONNECTING);

    /* Starts an instance of the WAN Interface State Machine on
    the interface to begin configuring the WAN link */
    WanMgr_StartInterfaceStateMachine(&wanIfCtrl);


    CcspTraceInfo(("%s %d - State changed to STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP \n", __FUNCTION__, __LINE__));
    return STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP;
}

static WcPpPolicyState_t Transition_SecondaryInterfaceDown(WanMgr_Policy_Controller_t* pWanController)
{
    pWanController->selSecondaryInterfaceIdx = -1;
    CcspTraceInfo(("%s %d - State changed to STATE_PRIMARY_WAN_ACTIVE \n", __FUNCTION__, __LINE__));
    return STATE_PRIMARY_WAN_ACTIVE;
}
/*********************************************************************************/
/**************************** STATES *********************************************/
/*********************************************************************************/
static WcPpPolicyState_t State_WanDown(WanMgr_Policy_Controller_t* pWanController)
{
    int selectedPrimaryInterface = -1;
    int selectedSecondaryInterface = -1;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

#ifndef WAN_ENABLE_STANDBY
    /* Waiting to tear down all in active wan connection */
    if(WanMgr_CheckAllIfacesDown() == FALSE)
    {
        return STATE_INTERFACE_DOWN;
    }
#endif //WAN_ENABLE_STANDBY

    WanMgr_Policy_FM_SelectWANActive(pWanController, &selectedPrimaryInterface, &selectedSecondaryInterface);

    if(selectedPrimaryInterface != -1)
    {
        /* TODO: Implement selection timeout logic here */
        pWanController->activeInterfaceIdx = selectedPrimaryInterface;
        return Transition_PrimaryInterfaceSelected(pWanController);
    }
    else if(selectedSecondaryInterface != -1)
    {
        /* TODO: Implement selection timeout logic here */
        pWanController->activeInterfaceIdx = selectedSecondaryInterface;
        pWanController->selSecondaryInterfaceIdx = selectedSecondaryInterface;
        return Transition_SecondaryInterfaceSelected(pWanController);
    }

    return STATE_INTERFACE_DOWN;
}

static WcPpPolicyState_t State_PrimaryWanActive(WanMgr_Policy_Controller_t* pWanController)
{
    DML_WAN_IFACE* pActiveInterface = NULL;
    int newPrimaryInterface = -1;
    int newSecondaryInterface = -1;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    if(pWanController->pWanActiveIfaceData == NULL)
    {
        return STATE_INTERFACE_DOWN;
    }

    pActiveInterface = &(pWanController->pWanActiveIfaceData->data);


    if( pWanController->WanEnable == FALSE ||
        pActiveInterface->Phy.Status == WAN_IFACE_PHY_STATUS_DOWN ||
        pActiveInterface->Wan.Type != WAN_IFACE_TYPE_PRIMARY ||
        pActiveInterface->Wan.Enable == FALSE)
    {
        return Transition_PrimaryInterfaceDeSelected(pWanController);
    }


    //Check interface config/status changes
    WanMgr_Policy_FM_SelectWANActive(pWanController, &newPrimaryInterface, &newSecondaryInterface);

    if(newPrimaryInterface != pWanController->activeInterfaceIdx)
    {
        return Transition_PrimaryInterfaceDeSelected(pWanController);
    }

#ifdef WAN_ENABLE_STANDBY
    if(newSecondaryInterface != pWanController->selSecondaryInterfaceIdx)
    {
        pWanController->selSecondaryInterfaceIdx = newSecondaryInterface;
        return Transition_SecondaryInterfaceUp(pWanController);
    }
#endif //WAN_ENABLE_STANDBY

    return STATE_PRIMARY_WAN_ACTIVE;
}

static WcPpPolicyState_t State_SecondaryWanActive(WanMgr_Policy_Controller_t* pWanController)
{
    DML_WAN_IFACE* pActiveInterface = NULL;
    int newPrimaryInterface = -1;
    int newSecondaryInterface = -1;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    if(pWanController->pWanActiveIfaceData == NULL)
    {
        return STATE_INTERFACE_DOWN;
    }

    pActiveInterface = &(pWanController->pWanActiveIfaceData->data);


    if( pWanController->WanEnable == FALSE ||
        pActiveInterface->Phy.Status == WAN_IFACE_PHY_STATUS_DOWN ||
        pActiveInterface->Wan.Type != WAN_IFACE_TYPE_SECONDARY ||
        pActiveInterface->Wan.Enable == FALSE)
    {
        return Transition_SecondaryInterfaceDeSelected(pWanController);
    }


    //Check interface config/status changes
    WanMgr_Policy_FM_SelectWANActive(pWanController, &newPrimaryInterface, &newSecondaryInterface);

    if( newSecondaryInterface != pWanController->activeInterfaceIdx )
    {
        return Transition_SecondaryInterfaceDeSelected(pWanController);
    }

    if( newPrimaryInterface != -1 )
    {
#ifdef WAN_ENABLE_STANDBY
        pWanController->activeInterfaceIdx = newPrimaryInterface;
        return Transition_PrimaryInterfaceSelected(pWanController);
#else
        return Transition_SecondaryInterfaceDeSelected(pWanController);
#endif
    }

    return STATE_SECONDARY_WAN_ACTIVE;
}

static WcPpPolicyState_t State_PrimaryWanActiveSecondaryWanUp(WanMgr_Policy_Controller_t* pWanController)
{
    DML_WAN_IFACE* pActiveInterface = NULL;
    int newPrimaryInterface = -1;
    int newSecondaryInterface = -1;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    if(pWanController->pWanActiveIfaceData == NULL)
    {
        return STATE_INTERFACE_DOWN;
    }

    pActiveInterface = &(pWanController->pWanActiveIfaceData->data);

    /* Phy.Status of the Active Primary Interface is DOWN, or Wan.Enable of the Active Primary Interface
    is FALSE, or Wan.Type of the Active Primary Interface is not PRIMARY, or Global Enable is FALSE */
    if (pWanController->WanEnable != TRUE ||
        pActiveInterface->Wan.Enable != TRUE ||
        pActiveInterface->Wan.Type != WAN_IFACE_TYPE_PRIMARY ||
        pActiveInterface->Phy.Status == WAN_IFACE_PHY_STATUS_DOWN)
    {
        return Transition_SecondaryInterfaceSelected(pWanController);
    }

    //Check interface config/status changes
    WanMgr_Policy_FM_SelectWANActive(pWanController, &newPrimaryInterface, &newSecondaryInterface);

    /* Check any new Primary Interfaces is up */
    if(newPrimaryInterface != pWanController->activeInterfaceIdx)
    {
        return Transition_PrimaryInterfaceChanged(pWanController);
    }

    /* If all Secondary Interfaces are set to DISABLED */
    if(newSecondaryInterface == -1)
    {
        return Transition_SecondaryInterfaceDown(pWanController);
    }

    /* Check any new Secondary Interfaces is up */
    if(newSecondaryInterface != pWanController->selSecondaryInterfaceIdx)
    {
        pWanController->selSecondaryInterfaceIdx = newSecondaryInterface;
        return Transition_SecondaryInterfaceUp(pWanController);
    }

    return STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP;
}


/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/* WanMgr_Policy_PrimaryPriorityPolicy */
ANSC_STATUS WanMgr_Policy_PrimaryPriorityPolicy(void)
{
    CcspTraceInfo(("%s %d \n", __FUNCTION__, __LINE__));

    //detach thread from caller stack
    pthread_detach(pthread_self());

    //policy variables
    ANSC_STATUS retStatus = ANSC_STATUS_SUCCESS;
    WanMgr_Policy_Controller_t    WanPolicyCtrl;
    WcPpPolicyState_t pp_sm_state;
    bool bRunning = true;

    // event handler
    int n = 0;
    struct timeval tv;

    if(WanMgr_Controller_PolicyCtrlInit(&WanPolicyCtrl) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d Policy Controller Error \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    CcspTraceInfo(("%s %d  Primary Priority Policy Thread Starting \n", __FUNCTION__, __LINE__));

    // initialise state machine
    pp_sm_state = Transition_Start(&WanPolicyCtrl); // do this first before anything else to init variables

    while (bRunning)
    {
        /* Wait up to 500 milliseconds */
        tv.tv_sec = 0;
        tv.tv_usec = LOOP_TIMEOUT;

        n = select(0, NULL, NULL, NULL, &tv);
        if (n < 0)
        {
            /* interrupted by signal or something, continue */
            continue;
        }

        //Update Wan config
        WanMgr_Config_Data_t*   pWanConfigData = WanMgr_GetConfigData_locked();
        if(pWanConfigData != NULL)
        {
            WanPolicyCtrl.WanEnable = pWanConfigData->data.Enable;

            WanMgrDml_GetConfigData_release(pWanConfigData);
        }

        //Lock Iface Data
        WanPolicyCtrl.pWanActiveIfaceData = WanMgr_GetIfaceData_locked(WanPolicyCtrl.activeInterfaceIdx);

        // process state
        switch (pp_sm_state)
        {
            case STATE_INTERFACE_DOWN:
                pp_sm_state = State_WanDown(&WanPolicyCtrl);
                break;
            case STATE_PRIMARY_WAN_ACTIVE:
                pp_sm_state = State_PrimaryWanActive(&WanPolicyCtrl);
                break;
            case STATE_SECONDARY_WAN_ACTIVE:
                pp_sm_state = State_SecondaryWanActive(&WanPolicyCtrl);
                break;
            case STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP:
                pp_sm_state = State_PrimaryWanActiveSecondaryWanUp(&WanPolicyCtrl);
                break;
            default:
                CcspTraceInfo(("%s %d - Case: default \n", __FUNCTION__, __LINE__));
                bRunning = false;
                retStatus = ANSC_STATUS_FAILURE;
                break;
        }

        //Release Lock Iface Data
        if(WanPolicyCtrl.pWanActiveIfaceData != NULL)
        {
            WanMgrDml_GetIfaceData_release(WanPolicyCtrl.pWanActiveIfaceData);
        }
    }

    CcspTraceInfo(("%s %d - Exit from state machine\n", __FUNCTION__, __LINE__));
    return retStatus;
}
