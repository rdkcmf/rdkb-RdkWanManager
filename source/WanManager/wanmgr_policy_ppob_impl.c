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

typedef enum {
    SELECTING_WAN_INTERFACE = 0,
    SELECTED_INTERFACE_DOWN,
    SELECTED_INTERFACE_UP
} WcPpobPolicyState_t;


/* STATES */
static WcPpobPolicyState_t State_SelectingWanInterface(WanMgr_Policy_Controller_t* pWanController);
static WcPpobPolicyState_t State_SelectedInterfaceDown(WanMgr_Policy_Controller_t* pWanController);
static WcPpobPolicyState_t State_SelectedInterfaceUp(WanMgr_Policy_Controller_t* pWanController);

/* TRANSITIONS */
static WcPpobPolicyState_t Transition_Start(WanMgr_Policy_Controller_t* pWanController);
static WcPpobPolicyState_t Transition_WanInterfaceSelected(WanMgr_Policy_Controller_t* pWanController);
static WcPpobPolicyState_t Transition_SelectedInterfaceUp(WanMgr_Policy_Controller_t* pWanController);
static WcPpobPolicyState_t Transition_SelectedInterfaceDown(WanMgr_Policy_Controller_t* pWanController);

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


/*********************************************************************************/
/************************** TRANSITIONS ******************************************/
/*********************************************************************************/
static WcPpobPolicyState_t Transition_Start(WanMgr_Policy_Controller_t* pWanController)
{
    if(pWanController == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

    WanMgr_UpdatePlatformStatus(WANMGR_DISCONNECTED);

    return SELECTING_WAN_INTERFACE;
}

static WcPpobPolicyState_t Transition_WanInterfaceSelected(WanMgr_Policy_Controller_t* pWanController)
{
    DML_WAN_IFACE* pActiveInterface = NULL;
    bool bWanActive = false;
    WanMgr_IfaceSM_Controller_t wanIfCtrl;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }


    /* Select WAN as Active */
    WanMgr_Iface_Data_t*   pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pWanController->activeInterfaceIdx);
    if(pWanDmlIfaceData != NULL)
    {
        DML_WAN_IFACE* pWanIfaceData = &(pWanDmlIfaceData->data);

        //Set ActiveLink to TRUE
        pWanIfaceData->Wan.ActiveLink = TRUE;
        bWanActive = true;

        WanMgr_IfaceSM_Init(&wanIfCtrl, pWanIfaceData->uiIfaceIdx);

        WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
    }

    if(bWanActive == false)
    {
        return SELECTED_INTERFACE_DOWN;
    }

    WanMgr_UpdatePlatformStatus(WANMGR_CONNECTING);

    /* Starts an instance of the WAN Interface State Machine on
    the interface to begin configuring the WAN link */
    WanMgr_StartInterfaceStateMachine(&wanIfCtrl);

    return SELECTED_INTERFACE_UP;
}

static WcPpobPolicyState_t Transition_SelectedInterfaceUp(WanMgr_Policy_Controller_t* pWanController)
{
    DML_WAN_IFACE* pActiveInterface = NULL;
    bool bWanActive = false;
    WanMgr_IfaceSM_Controller_t wanIfCtrl;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* Cheack Active WAN is present */
    WanMgr_Iface_Data_t*   pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pWanController->activeInterfaceIdx);
    if(pWanDmlIfaceData != NULL)
    {
        DML_WAN_IFACE* pWanIfaceData = &(pWanDmlIfaceData->data);

        WanMgr_IfaceSM_Init(&wanIfCtrl, pWanIfaceData->uiIfaceIdx);

        bWanActive = true;

        WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
    }

    if(bWanActive == false)
    {
        return ANSC_STATUS_FAILURE;
    }


    WanMgr_UpdatePlatformStatus(WANMGR_CONNECTING);

    /* Starts an instance of the WAN Interface State Machine on
    the interface to begin configuring the WAN link */
    WanMgr_StartInterfaceStateMachine(&wanIfCtrl);

    return SELECTED_INTERFACE_UP;
}

static WcPpobPolicyState_t Transition_SelectedInterfaceDown(WanMgr_Policy_Controller_t* pWanController)
{
    WanMgr_UpdatePlatformStatus(WANMGR_DISCONNECTED);
    return SELECTED_INTERFACE_DOWN;
}

/*********************************************************************************/
/**************************** STATES *********************************************/
/*********************************************************************************/
static WcPpobPolicyState_t State_SelectingWanInterface(WanMgr_Policy_Controller_t* pWanController)
{
    int selectedPrimaryInterface = -1;
    int selectedSecondaryInterface = -1;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    WanMgr_Policy_FM_SelectWANActive(pWanController, &selectedPrimaryInterface, &selectedSecondaryInterface);


    if(selectedPrimaryInterface != -1 && selectedSecondaryInterface != -1)
    {
        /* multiple interfaces connected, so start selectiontimeout timer
        if selectiontimeout timer > maximum selection timeout of the connected interfaces,
        use the highest priority interface */
        pWanController->activeInterfaceIdx = selectedPrimaryInterface;
    }
    else if(selectedPrimaryInterface != -1)
    {
        pWanController->activeInterfaceIdx = selectedPrimaryInterface;
    }
    else if(selectedSecondaryInterface != -1)
    {
        pWanController->activeInterfaceIdx = selectedSecondaryInterface;
    }

    if(pWanController->activeInterfaceIdx != -1)
    {
        return Transition_WanInterfaceSelected(pWanController);
    }

    return SELECTING_WAN_INTERFACE;
}

static WcPpobPolicyState_t State_SelectedInterfaceUp(WanMgr_Policy_Controller_t* pWanController)
{
    DML_WAN_IFACE* pActiveInterface = NULL;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    if(pWanController->pWanActiveIfaceData == NULL)
    {
        return SELECTED_INTERFACE_DOWN;
    }

    pActiveInterface = &(pWanController->pWanActiveIfaceData->data);


    if( pWanController->WanEnable == FALSE ||
        pActiveInterface->Phy.Status == WAN_IFACE_PHY_STATUS_DOWN ||
        pActiveInterface->Wan.Enable == FALSE)
    {
        return Transition_SelectedInterfaceDown(pWanController);
    }

    /* TODO: Traffic to the WAN Interface has been idle for a time that exceeds
    the configured IdleTimeout value */
    //return Transition_SelectedInterfaceDown(pWanController);

    return SELECTED_INTERFACE_UP;
}

static WcPpobPolicyState_t State_SelectedInterfaceDown(WanMgr_Policy_Controller_t* pWanController)
{
    DML_WAN_IFACE* pActiveInterface = NULL;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    if(pWanController->pWanActiveIfaceData == NULL)
    {
        return SELECTED_INTERFACE_DOWN;
    }

    pActiveInterface = &(pWanController->pWanActiveIfaceData->data);

    if(pWanController->WanEnable == TRUE &&
       (pActiveInterface->Phy.Status == WAN_IFACE_PHY_STATUS_UP ||
       pActiveInterface->Phy.Status == WAN_IFACE_PHY_STATUS_INITIALIZING) &&
       pActiveInterface->Wan.LinkStatus == WAN_IFACE_LINKSTATUS_DOWN &&
       pActiveInterface->Wan.Status == WAN_IFACE_STATUS_DISABLED)
    {
        return Transition_SelectedInterfaceUp(pWanController);
    }

    return SELECTED_INTERFACE_DOWN;
}


/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/* WanMgr_Policy_PrimaryPriorityOnBootupPolicy */
ANSC_STATUS WanMgr_Policy_PrimaryPriorityOnBootupPolicy(void)
{
    CcspTraceInfo(("%s %d \n", __FUNCTION__, __LINE__));

    //detach thread from caller stack
    pthread_detach(pthread_self());

    //policy variables
    ANSC_STATUS retStatus = ANSC_STATUS_SUCCESS;
    WanMgr_Policy_Controller_t    WanPolicyCtrl;
    WcPpobPolicyState_t ppob_sm_state;
    bool bRunning = true;

    // event handler
    int n = 0;
    struct timeval tv;


    if(WanMgr_Controller_PolicyCtrlInit(&WanPolicyCtrl) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d Policy Controller Error \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    CcspTraceInfo(("%s %d  Primary Priority On Bootup Policy Thread Starting \n", __FUNCTION__, __LINE__));

    // initialise state machine
    ppob_sm_state = Transition_Start(&WanPolicyCtrl); // do this first before anything else to init variables

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
        switch (ppob_sm_state)
        {
            case SELECTING_WAN_INTERFACE:
                ppob_sm_state = State_SelectingWanInterface(&WanPolicyCtrl);
                break;
            case SELECTED_INTERFACE_DOWN:
                ppob_sm_state = State_SelectedInterfaceDown(&WanPolicyCtrl);
                break;
            case SELECTED_INTERFACE_UP:
                ppob_sm_state = State_SelectedInterfaceUp(&WanPolicyCtrl);
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

