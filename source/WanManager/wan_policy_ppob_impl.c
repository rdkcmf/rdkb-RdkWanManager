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

typedef enum {
    SELECTING_WAN_INTERFACE = 0,
    SELECTED_INTERFACE_DOWN,
    SELECTED_INTERFACE_UP
} WcPpobPolicyState_t;

WcPpobPolicyState_t ppob_sm_state;

/* STATES */
static WcPpobPolicyState_t State_SelectingWanInterface(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcPpobPolicyState_t State_SelectedInterfaceDown(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcPpobPolicyState_t State_SelectedInterfaceUp(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);

/* TRANSITIONS */
static WcPpobPolicyState_t Transition_Start(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcPpobPolicyState_t Transition_WanInterfaceSelected(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcPpobPolicyState_t Transition_SelectedInterfaceUp(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcPpobPolicyState_t Transition_SelectedInterfaceDown(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);

/* WanController_PrimaryPriorityOnBootupPolicy_SMThread */
void *WanController_PrimaryPriorityOnBootupPolicy_SMThread(void *arg)
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
    ppob_sm_state = Transition_Start(pWanController); // do this first before anything else to init variables

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

        DmlWanIfGetCopyOfGlobalData(pGlobalIfaceInfo);
        pWanController->pInterface = pGlobalIfaceInfo;

        // process state
        switch (ppob_sm_state) {
            case SELECTING_WAN_INTERFACE:
                ppob_sm_state = State_SelectingWanInterface(pWanController);
                break;
            case SELECTED_INTERFACE_DOWN:
                ppob_sm_state = State_SelectedInterfaceDown(pWanController);
                break;
            case SELECTED_INTERFACE_UP:
                ppob_sm_state = State_SelectedInterfaceUp(pWanController);
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

static WcPpobPolicyState_t State_SelectingWanInterface(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    int iLoopCount;
    int selectedPrimaryInterface = -1;
    int selectedSecondaryInterface = -1;

    PDML_WAN_IFACE_GLOBAL_CONFIG pInterface = NULL;
    PDML_WAN_IFACE_GLOBAL_CONFIG pCurrentInterface = NULL;

    if(pWanController == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

    pInterface = pWanController->pInterface;
    if(pInterface == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

    for( iLoopCount = 0; iLoopCount < pWanController->uInterfaceCount; iLoopCount++ )
    {
        if (pWanController->WanEnable == TRUE &&
            pInterface[iLoopCount].CfgEnable == TRUE &&
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

    if(selectedPrimaryInterface != -1 && selectedSecondaryInterface != -1)
    {
        /* multiple interfaces connected, so start selectiontimeout timer
        if selectiontimeout timer > maximum selection timeout of the connected interfaces,
        use the highest priority interface */
        pWanController->activeInterface = selectedPrimaryInterface;
    }
    else if(selectedPrimaryInterface != -1)
    {
        pWanController->activeInterface = selectedPrimaryInterface;
    }
    else if(selectedSecondaryInterface != -1)
    {
        pWanController->activeInterface = selectedSecondaryInterface;
    }

    if(pWanController->activeInterface != -1)
    {
        return Transition_WanInterfaceSelected(pWanController);
    }

    return SELECTING_WAN_INTERFACE;
}

static WcPpobPolicyState_t State_SelectedInterfaceUp(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    if(pWanController == NULL) {
        return ANSC_STATUS_FAILURE;
    }

    PDML_WAN_IFACE_GLOBAL_CONFIG pSelectedInterface =
        (pWanController->pInterface) + (pWanController->activeInterface);

    if(pWanController->WanEnable == FALSE || pSelectedInterface->CfgPhyStatus == WAN_IFACE_PHY_STATUS_DOWN)
    {
        return Transition_SelectedInterfaceDown(pWanController);
    }

    /* TODO: Traffic to the WAN Interface has been idle for a time that exceeds
    the configured IdleTimeout value */
    //return Transition_SelectedInterfaceDown(pWanController);

    return SELECTED_INTERFACE_UP;
}

static WcPpobPolicyState_t State_SelectedInterfaceDown(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    PDML_WAN_IFACE_GLOBAL_CONFIG pSelectedInterface = NULL;

    if(pWanController == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

    pSelectedInterface = (pWanController->pInterface) + (pWanController->activeInterface);

    if(pWanController->WanEnable == TRUE &&
       pSelectedInterface->CfgEnable == TRUE &&
       (pSelectedInterface->CfgPhyStatus == WAN_IFACE_PHY_STATUS_UP ||
       pSelectedInterface->CfgPhyStatus == WAN_IFACE_PHY_STATUS_INITIALIZING) &&
       pSelectedInterface->CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN &&
       pSelectedInterface->CfgStatus == WAN_IFACE_STATUS_DISABLED)
    {
        return Transition_SelectedInterfaceUp(pWanController);
    }

    return SELECTED_INTERFACE_DOWN;
}

static WcPpobPolicyState_t Transition_Start(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    int iLoopCount = 0;
    PDML_WAN_IFACE_GLOBAL_CONFIG pInterface = NULL;

    if(pWanController == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

#ifdef _HUB4_PRODUCT_REQ_
        util_setWanLedState(OFF);
#endif

    pInterface = (pWanController->pInterface);

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
        strncpy(pInterface[iLoopCount].CfgWanName, "", sizeof(pInterface[iLoopCount].CfgWanName));
    }

    return SELECTING_WAN_INTERFACE;
}

static WcPpobPolicyState_t Transition_WanInterfaceSelected(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    WanInterfaceData_t wanIf;
    PDML_WAN_IFACE_GLOBAL_CONFIG pSelectedInterface = NULL;

#ifdef _HUB4_PRODUCT_REQ_
    util_setWanLedState(FLASHING_AMBER);
#endif

    pSelectedInterface = (pWanController->pInterface) + (pWanController->activeInterface);

    if(WanController_updateWanActiveLinkFlag((pWanController->activeInterface), TRUE) != ANSC_STATUS_SUCCESS)
    {
        return ANSC_STATUS_FAILURE;
    }

    /* Starts an instance of the WAN Interface State Machine on
    the interface to begin configuring the WAN link */
    strncpy(wanIf.ifName, pSelectedInterface->CfgWanName, sizeof(wanIf.ifName));
    strncpy(wanIf.baseIfName, pSelectedInterface->CfgBaseifName, sizeof(wanIf.baseIfName));
    WanManager_StartStateMachine(&wanIf);

    return SELECTED_INTERFACE_UP;
}

static WcPpobPolicyState_t Transition_SelectedInterfaceUp(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    WanInterfaceData_t wanIf;
    PDML_WAN_IFACE_GLOBAL_CONFIG pSelectedInterface = NULL;

    pSelectedInterface = (pWanController->pInterface) + (pWanController->activeInterface);

    /* Starts an instance of the WAN Interface State Machine on
    the interface to begin configuring the WAN link */
    strncpy(wanIf.ifName, pSelectedInterface->CfgWanName, sizeof(wanIf.ifName));
    strncpy(wanIf.baseIfName, pSelectedInterface->CfgBaseifName, sizeof(wanIf.baseIfName));
#ifdef _HUB4_PRODUCT_REQ_
    util_setWanLedState(SOLID_AMBER);
#endif
    WanManager_StartStateMachine(&wanIf);

    return SELECTED_INTERFACE_UP;
}

static WcPpobPolicyState_t Transition_SelectedInterfaceDown(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
#ifdef _HUB4_PRODUCT_REQ_
    util_setWanLedState(OFF);
#endif
    return SELECTED_INTERFACE_DOWN;
}
