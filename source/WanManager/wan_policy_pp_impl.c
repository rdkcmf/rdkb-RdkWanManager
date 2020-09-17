/*
   If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include "wan_controller.h"
#include "wan_controller_utils.h"
#include "wan_interface_internal.h"

/* ---- Global Constants -------------------------- */
#define LOOP_TIMEOUT 500000 // timeout in milliseconds. This is the state machine loop interval

extern pthread_mutex_t gmWanInterfaceData;

/* primary priority policy */
typedef enum {
    STATE_INTERFACE_DOWN = 0,
    STATE_PRIMARY_WAN_ACTIVE,
    STATE_SECONDARY_WAN_ACTIVE,
    STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP
} WcPpPolicyState_t;

static WcPpPolicyState_t pp_sm_state;

/* STATES */
static WcPpPolicyState_t State_WanDown(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcPpPolicyState_t State_PrimaryWanActive(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcPpPolicyState_t State_SecondaryWanActive(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcPpPolicyState_t State_PrimaryWanActiveSecondaryWanUp(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);

/* TRANSITIONS */
static WcPpPolicyState_t Transition_Start(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcPpPolicyState_t Transition_PrimaryInterfaceSelected(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcPpPolicyState_t Transition_PrimaryInterfaceDeSelected(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcPpPolicyState_t Transition_PrimaryInterfaceChanged(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcPpPolicyState_t Transition_SecondaryInterfaceSelected(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcPpPolicyState_t Transition_SecondaryInterfaceDeSelected(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcPpPolicyState_t Transition_SecondaryInterfaceUp(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcPpPolicyState_t Transition_SecondaryInterfaceDown(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);

/* WanController_PrimaryPriorityPolicy_SMThread */
void *WanController_PrimaryPriorityPolicy_SMThread(void *arg)
{
    // event handler
    int n = 0;
    struct timeval tv;

    CcspTraceInfo(("%s %d \n", __FUNCTION__, __LINE__));

    PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController = (PWAN_CONTROLLER_PRIVATE_SM_INFO)arg;
    if (pWanController == NULL) {
        CcspTraceError(("%s %d Invalid Argument \n", __FUNCTION__, __LINE__));
        pthread_exit(NULL);
    }

    //detach thread from caller stack
    pthread_detach(pthread_self());

    PDML_WAN_IFACE_GLOBAL_CONFIG pGlobalIfaceInfo = NULL;
    pGlobalIfaceInfo = (PDML_WAN_IFACE_GLOBAL_CONFIG)AnscAllocateMemory(sizeof(DML_WAN_IFACE_GLOBAL_CONFIG) * (pWanController->uInterfaceCount));
    if (pGlobalIfaceInfo == NULL) {
        CcspTraceError(("%s %d Invalid Memory!!! \n", __FUNCTION__, __LINE__));
        pthread_exit(NULL);
    }

    /* Wait till interface data models are initialised */
    while(!DmlWanIfCheckDataModelInitialised())
    {
        sleep(1);
    }

    // local variables
    bool bRunning = true;

    // initialise state machine
    DmlWanIfGetCopyOfGlobalData(pGlobalIfaceInfo);
    pWanController->pInterface = pGlobalIfaceInfo;
    pp_sm_state = Transition_Start(pWanController); // do this first before anything else to init variables

    while (bRunning)
    {
        /* Wait up to 500 milliseconds */
        tv.tv_sec = 0;
        tv.tv_usec = LOOP_TIMEOUT;

        n = select(0, NULL, NULL, NULL, &tv);
        if (n < 0) {
            /* interrupted by signal or something, continue */
            continue;
        }

        DmlWanIfGetCopyOfGlobalData(pGlobalIfaceInfo);
        pWanController->pInterface = pGlobalIfaceInfo;

        // process state
        switch (pp_sm_state) {
            case STATE_INTERFACE_DOWN:
                pp_sm_state = State_WanDown(pWanController);
                break;
            case STATE_PRIMARY_WAN_ACTIVE:
                pp_sm_state = State_PrimaryWanActive(pWanController);
                break;
            case STATE_SECONDARY_WAN_ACTIVE:
                pp_sm_state = State_SecondaryWanActive(pWanController);
                break;
            case STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP:
                pp_sm_state = State_PrimaryWanActiveSecondaryWanUp(pWanController);
                break;
            default:
                CcspTraceInfo(("%s %d - Case: default \n", __FUNCTION__, __LINE__));
                bRunning = false;
                if (NULL != pWanController) {
                    free(pWanController);
                    pWanController = NULL;
                }
                if (NULL != pGlobalIfaceInfo) {
                    free(pGlobalIfaceInfo);
                    pGlobalIfaceInfo = NULL;
                }
                CcspTraceInfo(("%s %d - Exit from state machine\n", __FUNCTION__, __LINE__));
                pthread_exit(NULL);
        }
    }
    return NULL;
}

static WcPpPolicyState_t State_WanDown(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    int iLoopCount;
    int selectedPrimaryInterface = -1;
    int selectedSecondaryInterface = -1;

    PDML_WAN_IFACE_GLOBAL_CONFIG pInterface = NULL;
    PDML_WAN_IFACE_GLOBAL_CONFIG pCurrentInterface = NULL;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    pInterface = pWanController->pInterface;
    if(pInterface == NULL)
    {
        CcspTraceError(("%s %d pInterface object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    for( iLoopCount = 0; iLoopCount < pWanController->uInterfaceCount; iLoopCount++ )
    {
        if (pWanController->WanEnable == TRUE &&
            pInterface[iLoopCount].CfgEnable == TRUE &&
            pInterface[iLoopCount].CfgStatus == WAN_IFACE_STATUS_DISABLED &&
            pInterface[iLoopCount].CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN &&
           (pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_UP ||
            pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_INITIALIZING))
        {
            if(pInterface[iLoopCount].CfgType == WAN_IFACE_TYPE_PRIMARY)
            {
                if(selectedPrimaryInterface == -1 )
                {
                    selectedPrimaryInterface = iLoopCount;
                }
                else if(pInterface[iLoopCount].CfgPriority < pInterface[selectedPrimaryInterface].CfgPriority)
                {
                     selectedPrimaryInterface = iLoopCount;
                }
            }
            else
            {
                if(selectedSecondaryInterface == -1 )
                {
                    selectedSecondaryInterface = iLoopCount;
                }
                else if(pInterface[iLoopCount].CfgPriority < pInterface[selectedSecondaryInterface].CfgPriority)
                {
                    selectedSecondaryInterface = iLoopCount;
                }
            }
        }
    }

    if(selectedPrimaryInterface != -1)
    {
        /* TODO: Implement selection timeout logic here */
        pWanController->activeInterface = selectedPrimaryInterface;
        return Transition_PrimaryInterfaceSelected(pWanController);
    }
    else if(selectedSecondaryInterface != -1)
    {
        /* TODO: Implement selection timeout logic here */
        pWanController->activeInterface = selectedSecondaryInterface;
        return Transition_SecondaryInterfaceSelected(pWanController);
    }

    return STATE_INTERFACE_DOWN;
}

static WcPpPolicyState_t State_PrimaryWanActive(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    PDML_WAN_IFACE_GLOBAL_CONFIG pInterface = NULL;
    PDML_WAN_IFACE_GLOBAL_CONFIG pCurrentInterface = NULL;
    int newPrimaryInterface = -1;
    int newSecondaryInterface = -1;
    WanInterfaceData_t wanIf;
    int iLoopCount = 0;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }
    pInterface = pWanController->pInterface;

    if(pInterface == NULL)
    {
        CcspTraceError(("%s %d pInterface object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }
    pCurrentInterface = (pWanController->pInterface) + (pWanController->activeInterface);

    if(pCurrentInterface == NULL)
    {
        CcspTraceError(("%s %d pCurrentInterface object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    if( pWanController->WanEnable == FALSE ||
        pCurrentInterface->CfgPhyStatus == WAN_IFACE_PHY_STATUS_DOWN ||
        pCurrentInterface->CfgType != WAN_IFACE_TYPE_PRIMARY ||
        pCurrentInterface->CfgEnable == FALSE)
    {
        return Transition_PrimaryInterfaceDeSelected(pWanController);
    }

    for( iLoopCount = 0; iLoopCount < pWanController->uInterfaceCount; iLoopCount++ )
    {
        if (strcmp(pInterface[iLoopCount].CfgBaseifName, pCurrentInterface->CfgBaseifName) == 0 )
            continue;
        if (pInterface[iLoopCount].CfgEnable == TRUE)
        {
            if(pInterface[iLoopCount].CfgType == WAN_IFACE_TYPE_PRIMARY )
            {
                if ((pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_UP ||
                    pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_INITIALIZING ) &&
                    pInterface[iLoopCount].CfgPriority < pCurrentInterface->CfgPriority )
                {
                    if( newPrimaryInterface == -1)
                    {
                        newPrimaryInterface = iLoopCount;
                    }
                    else if (pInterface[iLoopCount].CfgPriority < pInterface[newPrimaryInterface].CfgPriority )
                    {
                        newPrimaryInterface = iLoopCount;
                    }
                }
            }
            else if(pInterface[iLoopCount].CfgType == WAN_IFACE_TYPE_SECONDARY )
            {
                if ((pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_UP ||
                    pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_INITIALIZING ) &&
                    pInterface[iLoopCount].CfgStatus == WAN_IFACE_STATUS_DISABLED &&
                    pInterface[iLoopCount].CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN )
                {
                    if( newSecondaryInterface == -1)
                    {
                        newSecondaryInterface = iLoopCount;
                    }
                    else if (pInterface[iLoopCount].CfgPriority < pInterface[newSecondaryInterface].CfgPriority )
                    {
                        newSecondaryInterface = iLoopCount;
                    }
                }
            }
        }
    }

    if(newPrimaryInterface != -1 )
    {
        return Transition_PrimaryInterfaceDeSelected(pWanController);
    }

    if(newSecondaryInterface != -1 )
    {
        pWanController->activeSecondaryInterface = newSecondaryInterface;
        return Transition_SecondaryInterfaceUp(pWanController);
    }

    return STATE_PRIMARY_WAN_ACTIVE;
}

static WcPpPolicyState_t State_SecondaryWanActive(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    PDML_WAN_IFACE_GLOBAL_CONFIG pInterface = NULL;
    PDML_WAN_IFACE_GLOBAL_CONFIG pCurrentInterface = NULL;
    WanInterfaceData_t wanIf;
    int newPrimaryInterface = -1;
    int newSecondaryInterface = -1;
    int iLoopCount = 0;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }
    pInterface = pWanController->pInterface;

    if(pInterface == NULL)
    {
        CcspTraceError(("%s %d pInterface object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }
    pCurrentInterface = (pWanController->pInterface) + (pWanController->activeInterface);

    if(pCurrentInterface == NULL)
    {
        CcspTraceError(("%s %d pCurrentInterface object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    pInterface = pWanController->pInterface;
    pCurrentInterface = (pWanController->pInterface) + (pWanController->activeInterface);

    if((pWanController->WanEnable == FALSE) ||
       (pCurrentInterface->CfgPhyStatus == WAN_IFACE_PHY_STATUS_DOWN) ||
       (pCurrentInterface->CfgType != WAN_IFACE_TYPE_SECONDARY) ||
       (pCurrentInterface->CfgEnable == FALSE))
    {
        return Transition_SecondaryInterfaceDeSelected(pWanController);
    }

    for( iLoopCount = 0; iLoopCount < pWanController->uInterfaceCount; iLoopCount++ )
    {
        if (strcmp(pInterface[iLoopCount].CfgBaseifName, pCurrentInterface->CfgBaseifName) == 0 )
            continue;
        if (pInterface[iLoopCount].CfgEnable == TRUE)
        {
            if(pInterface[iLoopCount].CfgType == WAN_IFACE_TYPE_PRIMARY)
            {
               if((pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_UP ||
                   pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_INITIALIZING ) &&
                   pInterface[iLoopCount].CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN &&
                   pInterface[iLoopCount].CfgStatus == WAN_IFACE_STATUS_DISABLED )
               {
                   if (newPrimaryInterface != -1 )
                   {
                       newPrimaryInterface = iLoopCount;
                   }
                   else if (pInterface[iLoopCount].CfgPriority < pInterface[newPrimaryInterface].CfgPriority )
                   {
                       newPrimaryInterface = iLoopCount;
                   }
               }
            }
            else if(pInterface[iLoopCount].CfgType == WAN_IFACE_TYPE_SECONDARY)
            {
                if((pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_UP ||
                   pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_INITIALIZING ) &&
                   pInterface[iLoopCount].CfgPriority < pCurrentInterface->CfgPriority )
                {
                    if(newSecondaryInterface == -1)
                    {
                        newSecondaryInterface = iLoopCount;
                    }
                    else if (pInterface[iLoopCount].CfgPriority < pInterface[newSecondaryInterface].CfgPriority )
                    {
                        newSecondaryInterface = iLoopCount;
                    }
                }
            }
        }
    }

    if( newSecondaryInterface != -1 )
    {
        return Transition_SecondaryInterfaceDeSelected(pWanController);
    }

    if( newPrimaryInterface != -1 )
    {
        pWanController->activeInterface = newPrimaryInterface;
        return Transition_PrimaryInterfaceSelected(pWanController);
    }

    return STATE_SECONDARY_WAN_ACTIVE;
}

static WcPpPolicyState_t State_PrimaryWanActiveSecondaryWanUp(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    PDML_WAN_IFACE_GLOBAL_CONFIG pInterface = NULL;
    PDML_WAN_IFACE_GLOBAL_CONFIG pCurrentInterface = NULL;
    bool bAllSecondaryInterfaceDownFlag = true;
    bool bNewPrimaryInterfaceUpFlag = false;
    bool bNewSecondaryInterfaceUpFlag = false;
    int iLoopCount = 0;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    pInterface = pWanController->pInterface;
    if(pInterface == NULL)
    {
        CcspTraceError(("%s %d pInterface object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    pCurrentInterface = (pWanController->pInterface) + (pWanController->activeInterface);
    if(pCurrentInterface == NULL)
    {
        CcspTraceError(("%s %d pCurrentInterface object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* Phy.Status of the Active Primary Interface is DOWN, or Wan.Enable of the Active Primary Interface
    is FALSE, or Wan.Type of the Active Primary Interface is not PRIMARY, or Global Enable is FALSE */
    if (pWanController->WanEnable != TRUE ||
        pCurrentInterface->CfgEnable != TRUE ||
        pCurrentInterface->CfgPhyStatus == WAN_IFACE_PHY_STATUS_DOWN ||
        pCurrentInterface->CfgType != WAN_IFACE_TYPE_PRIMARY)
    {
        return Transition_SecondaryInterfaceSelected(pWanController);
    }

    for( iLoopCount = 0; iLoopCount < pWanController->uInterfaceCount; iLoopCount++ )
    {
        if (strcmp(pInterface[iLoopCount].CfgBaseifName, pCurrentInterface->CfgBaseifName) == 0 )
            continue;

        if (pInterface[iLoopCount].CfgEnable != TRUE)
            continue;

        if ( pInterface[iLoopCount].CfgType == WAN_IFACE_TYPE_PRIMARY )
        {
            /* Wan.Priority of the Active Primary Interface is no longer the highest of
            all the connected Primary Interfaces */
            if((pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_UP ||
                pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_STATUS_INITIALISING ) &&
                pInterface[iLoopCount].CfgPriority < pCurrentInterface->CfgPriority)
            {
                bNewPrimaryInterfaceUpFlag = true;
            }
        }
        else
        {
            /* Set SecondaryInterfaceDownFlag flag when Wan.Status of any Secondary Interfaces are not
            set to DISABLED or Wan.LinkStatus are not set to DOWN */
            if (pInterface[iLoopCount].CfgStatus != WAN_IFACE_STATUS_DISABLED ||
                pInterface[iLoopCount].CfgLinkStatus != WAN_IFACE_LINKSTATUS_DOWN)
            {
                bAllSecondaryInterfaceDownFlag = false;
            }
            /* Phy.Status is UP or INITIALISING for a new Secondary Interface, whose Wan.Enable is TRUE,
            and Wan.Type is SECONDARY, and Wan.Status is DISABLED and Wan.LinkStatus is DOWN, and
            Global Enable is TRUE */
            if ((pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_UP ||
                 pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_STATUS_INITIALISING) &&
                pInterface[iLoopCount].CfgStatus == WAN_IFACE_STATUS_DISABLED &&
                pInterface[iLoopCount].CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN)
            {
                bNewSecondaryInterfaceUpFlag = true;
            }
        }
    }

    /* Check any new Primary Interfaces is up */
    if(bNewPrimaryInterfaceUpFlag == true)
    {
        return Transition_PrimaryInterfaceChanged(pWanController);
    }

    /* Check any new Secondary Interfaces is up */
    if(bNewSecondaryInterfaceUpFlag == true)
    {
        return Transition_SecondaryInterfaceUp(pWanController);
    }

    /* If Wan.Status of all Secondary Interfaces are set to DISABLED and Wan.LinkStatus are set to DOWN */
    if(bAllSecondaryInterfaceDownFlag == true)
    {
        return Transition_SecondaryInterfaceDown(pWanController);
    }

    return STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP;
}

static WcPpPolicyState_t Transition_Start(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    int iLoopCount = 0;
    PDML_WAN_IFACE_GLOBAL_CONFIG pInterface = NULL;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

#ifdef _HUB4_PRODUCT_REQ_
        util_setWanLedState(OFF);
#endif

    pInterface = pWanController->pInterface;
    if(pInterface == NULL)
    {
        CcspTraceError(("%s %d pInterface object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    for( iLoopCount = 0; iLoopCount < pWanController->uInterfaceCount; iLoopCount++ )
    {
        pInterface[iLoopCount].CfgStatus = WAN_IFACE_STATUS_DISABLED;
        pInterface[iLoopCount].CfgLinkStatus = WAN_IFACE_LINKSTATUS_DOWN;
        pInterface[iLoopCount].CfgIpv4Status = WAN_IFACE_IPV4_STATE_DOWN;
        pInterface[iLoopCount].CfgIpv6Status = WAN_IFACE_IPV6_STATE_DOWN;
        pInterface[iLoopCount].CfgMAPTStatus = WAN_IFACE_MAPT_STATE_DOWN;
        pInterface[iLoopCount].CfgDSLiteStatus = WAN_IFACE_DSLITE_STATE_DOWN;
        pInterface[iLoopCount].CfgActiveLink = FALSE;
        pInterface[iLoopCount].CfgRefresh = FALSE;
    }

    CcspTraceInfo(("%s %d - State changed to STATE_INTERFACE_DOWN \n", __FUNCTION__, __LINE__));
    return STATE_INTERFACE_DOWN;
}

static WcPpPolicyState_t Transition_PrimaryInterfaceSelected(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    PDML_WAN_IFACE_GLOBAL_CONFIG pInterface = NULL;
    PDML_WAN_IFACE_GLOBAL_CONFIG pCurrentInterface = NULL;
    int WanStatusFlag = 0;
    WanInterfaceData_t wanIf;
    int iLoopCount = 0;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    pInterface = pWanController->pInterface;
    if(pInterface == NULL)
    {
        CcspTraceError(("%s %d pInterface object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    pCurrentInterface = (pWanController->pInterface) + (pWanController->activeInterface);
    if(pCurrentInterface == NULL)
    {
        CcspTraceError(("%s %d pCurrentInterface object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* Sets Wan.ActiveLink to TRUE for the selected WAN interface */
    if(WanController_updateWanActiveLinkFlag((pWanController->activeInterface), TRUE) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d WanController_updateWanActiveLinkFlag() failed \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* If Wan.ActiveLink is set to TRUE for any other interfaces,
    set those to FALSE but leave their Wan.Status unchanged */
    for( iLoopCount = 0; iLoopCount < pWanController->uInterfaceCount; iLoopCount++ )
    {
        if(iLoopCount == pWanController->activeInterface)
            continue;
        if(pInterface[iLoopCount].CfgActiveLink == TRUE)
        {
            if(WanController_updateWanActiveLinkFlag(iLoopCount, FALSE) != ANSC_STATUS_SUCCESS)
            {
                CcspTraceError(("%s %d WanController_updateWanActiveLinkFlag() failed \n", __FUNCTION__, __LINE__));
                return ANSC_STATUS_FAILURE;
            }
        }
    }

    /* Starts an instance of the WAN Interface State Machine on
    the interface to begin configuring the WAN link */
    strncpy(wanIf.ifName, pCurrentInterface->CfgWanName, sizeof(wanIf.ifName));
    strncpy(wanIf.baseIfName, pCurrentInterface->CfgBaseifName, sizeof(wanIf.baseIfName));
#ifdef _HUB4_PRODUCT_REQ_
    util_setWanLedState(SOLID_AMBER);
#endif
    WanManager_StartStateMachine(&wanIf);

    /* If Wan.Status is DISABLED for all other Interfaces : Change state to PrimaryWANActive */
    for( iLoopCount = 0; iLoopCount < pWanController->uInterfaceCount; iLoopCount++ )
    {
        if(iLoopCount == pWanController->activeInterface)
            continue;
        if(pInterface[iLoopCount].CfgStatus != WAN_IFACE_STATUS_DISABLED)
        {
            WanStatusFlag = 1;
        }
    }
    if(WanStatusFlag == 0)
    {
        CcspTraceInfo(("%s %d - State changed to STATE_PRIMARY_WAN_ACTIVE \n", __FUNCTION__, __LINE__));
        return STATE_PRIMARY_WAN_ACTIVE;
    }

    CcspTraceInfo(("%s %d - State changed to STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP \n", __FUNCTION__, __LINE__));
    return STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP;
}

static WcPpPolicyState_t Transition_PrimaryInterfaceDeSelected(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* Sets Wan.Status to DISABLED for the current active interface */
    if(WanController_updateWanStatus((pWanController->activeInterface), WAN_IFACE_STATUS_DISABLED) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d WanController_updateWanStatus failed \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* Sets Wan.ActiveLink to FALSE for the current active interface */
    if(WanController_updateWanActiveLinkFlag((pWanController->activeInterface), FALSE) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d WanController_updateWanActiveLinkFlag failed \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }
#ifdef _HUB4_PRODUCT_REQ_
    util_setWanLedState(OFF);
#endif
    CcspTraceInfo(("%s %d - State changed to STATE_INTERFACE_DOWN \n", __FUNCTION__, __LINE__));
    return STATE_INTERFACE_DOWN;
}

static WcPpPolicyState_t Transition_PrimaryInterfaceChanged(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    PDML_WAN_IFACE_GLOBAL_CONFIG pInterface = NULL;
    PDML_WAN_IFACE_GLOBAL_CONFIG pCurrentInterface = NULL;
    int iLoopCount = 0;
    int newPrimaryInterface = -1;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    pInterface = pWanController->pInterface;
    if(pInterface == NULL)
    {
        CcspTraceError(("%s %d pInterface object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    pCurrentInterface = pInterface + pWanController->activeInterface;
    if(pCurrentInterface == NULL)
    {
        CcspTraceError(("%s %d pCurrentInterface object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    for( iLoopCount = 0; iLoopCount < pWanController->uInterfaceCount; iLoopCount++ )
    {
        if(pInterface[iLoopCount].CfgEnable == TRUE && pInterface[iLoopCount].CfgType == WAN_IFACE_TYPE_PRIMARY)
        {
            if((pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_UP ||
               pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_INITIALIZING ) &&
               pInterface[iLoopCount].CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN &&
               pInterface[iLoopCount].CfgStatus == WAN_IFACE_STATUS_DISABLED &&
               pInterface[iLoopCount].CfgPriority < pCurrentInterface->CfgPriority )
            {
                if (newPrimaryInterface != -1 )
                {
                    newPrimaryInterface = iLoopCount;
                }
                else if (pInterface[iLoopCount].CfgPriority < pInterface[newPrimaryInterface].CfgPriority )
                {
                    newPrimaryInterface = iLoopCount;
                }
            }
        }
    }

    Transition_PrimaryInterfaceDeSelected(pWanController);

    if (newPrimaryInterface != -1)
    {
        pWanController->activeInterface = newPrimaryInterface;
        Transition_PrimaryInterfaceSelected(pWanController);
    }

    CcspTraceInfo(("%s %d - State changed to STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP \n", __FUNCTION__, __LINE__));
    return STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP;
}

static WcPpPolicyState_t Transition_SecondaryInterfaceSelected(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    PDML_WAN_IFACE_GLOBAL_CONFIG pInterface = NULL;
    PDML_WAN_IFACE_GLOBAL_CONFIG pCurrentInterface = NULL;
    WanInterfaceData_t wanIf;
    int iLoopCount = 0;
    int WanStatusFlag = 0;
    int SelectedSecondaryInterface = -1;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    pInterface = pWanController->pInterface;
    if(pInterface == NULL)
    {
        CcspTraceError(("%s %d pInterface object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    pCurrentInterface = (pWanController->pInterface) + (pWanController->activeInterface);
    if(pCurrentInterface == NULL)
    {
        CcspTraceError(("%s %d pCurrentInterface object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* If activeInterface is't changed (i.e. it's still set to an old primary interface that we've just disabled),
    we need to find the next secondary interface to bring up */
    if ( pCurrentInterface->CfgType == WAN_IFACE_TYPE_PRIMARY )
    {
        for( iLoopCount = 0; iLoopCount < pWanController->uInterfaceCount; iLoopCount++ )
        {
            if (pInterface[iLoopCount].CfgEnable == TRUE &&
                pInterface[iLoopCount].CfgType == WAN_IFACE_TYPE_SECONDARY &&
                pInterface[iLoopCount].CfgStatus != WAN_IFACE_STATUS_DISABLED &&
                (pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_UP ||
                pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_INITIALIZING ))
            {
                if(SelectedSecondaryInterface == -1 )
                {
                    SelectedSecondaryInterface = iLoopCount;
                }
                else if(pInterface[iLoopCount].CfgPriority < pInterface[SelectedSecondaryInterface].CfgPriority)
                {
                    SelectedSecondaryInterface = iLoopCount;
                }
            }
        }
        if(SelectedSecondaryInterface != -1)
        {
            pWanController->activeInterface = SelectedSecondaryInterface;
            pCurrentInterface = (pWanController->pInterface) + (pWanController->activeInterface);
        }
        else
        {
            CcspTraceError(("%s %d Couldn't find a secondary interface that was already up \n", __FUNCTION__, __LINE__));
        }
    }

    /* Sets Wan.ActiveLink to TRUE for the selected WAN interface */
    if(WanController_updateWanActiveLinkFlag((pWanController->activeInterface), TRUE) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d WanController_updateWanActiveLinkFlag failed \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* If Wan.ActiveLink is set to TRUE for any other interfaces,
    set those to FALSE but leave their Wan.Status unchanged */
    for( iLoopCount = 0; iLoopCount < pWanController->uInterfaceCount; iLoopCount++ )
    {
        if(iLoopCount == pWanController->activeInterface)
            continue;
        if(pInterface[iLoopCount].CfgActiveLink == TRUE)
        {
            if(WanController_updateWanActiveLinkFlag(iLoopCount, FALSE) != ANSC_STATUS_SUCCESS)
            {
                CcspTraceError(("%s %d WanController_updateWanActiveLinkFlag failed \n", __FUNCTION__, __LINE__));
                return ANSC_STATUS_FAILURE;
            }
        }
    }
    /* Starts an instance of the WAN Interface State Machine on
    the interface to begin configuring the WAN link */
    if ( pCurrentInterface->CfgStatus == WAN_IFACE_STATUS_DISABLED &&
         pCurrentInterface->CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN )
    {
        strncpy(wanIf.ifName, pCurrentInterface->CfgWanName, sizeof(wanIf.ifName));
        strncpy(wanIf.baseIfName, pCurrentInterface->CfgBaseifName, sizeof(wanIf.baseIfName));
        WanManager_StartStateMachine(&wanIf);
    }

    CcspTraceInfo(("%s %d - State changed to STATE_SECONDARY_WAN_ACTIVE \n", __FUNCTION__, __LINE__));
    return STATE_SECONDARY_WAN_ACTIVE;
}

static WcPpPolicyState_t Transition_SecondaryInterfaceDeSelected(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }
    if(WanController_updateWanStatus((pWanController->activeInterface ), WAN_IFACE_STATUS_DISABLED) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d WanController_updateWanStatus failed \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }
    if(WanController_updateWanActiveLinkFlag((pWanController->activeInterface), FALSE) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d WanController_updateWanActiveLinkFlag failed \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    CcspTraceInfo(("%s %d - State changed to STATE_INTERFACE_DOWN \n", __FUNCTION__, __LINE__));
    return STATE_INTERFACE_DOWN;
}

static WcPpPolicyState_t Transition_SecondaryInterfaceUp(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    PDML_WAN_IFACE_GLOBAL_CONFIG pInterface = NULL;
    WanInterfaceData_t wanIf;
    int iLoopCount = 0;

    if(pWanController == NULL)
    {
        CcspTraceError(("%s %d pWanController object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    pInterface = pWanController->pInterface;
    if(pInterface == NULL)
    {
        CcspTraceError(("%s %d pInterface object is NULL \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* Starts wanmanager on the physically UP Secondary interface */
    for( iLoopCount = 0; iLoopCount < pWanController->uInterfaceCount; iLoopCount++ )
    {
        if (pInterface[iLoopCount].CfgEnable == TRUE &&
            pInterface[iLoopCount].CfgStatus == WAN_IFACE_STATUS_DISABLED &&
            pInterface[iLoopCount].CfgPhyStatus == WAN_IFACE_PHY_STATUS_UP &&
            pInterface[iLoopCount].CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN &&
            pInterface[iLoopCount].CfgType == WAN_IFACE_TYPE_SECONDARY)
        {
            strncpy(wanIf.ifName, pInterface[iLoopCount].CfgWanName, sizeof(wanIf.ifName));
            strncpy(wanIf.baseIfName, pInterface[iLoopCount].CfgBaseifName, sizeof(wanIf.baseIfName));
            WanManager_StartStateMachine(&wanIf);
            break;
        }
    }

    CcspTraceInfo(("%s %d - State changed to STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP \n", __FUNCTION__, __LINE__));
    return STATE_PRIMARY_WAN_ACTIVE_SECONDARY_WAN_UP;
}

static WcPpPolicyState_t Transition_SecondaryInterfaceDown(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    pWanController->activeSecondaryInterface = -1;
    CcspTraceInfo(("%s %d - State changed to STATE_PRIMARY_WAN_ACTIVE \n", __FUNCTION__, __LINE__));
    return STATE_PRIMARY_WAN_ACTIVE;
}
