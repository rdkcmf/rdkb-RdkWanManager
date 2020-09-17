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

/* fixed mode policy states */
typedef enum {
    STATE_FIXING_WAN_INTERFACE = 0,
    STATE_FIXED_WAN_INTERFACE_DOWN,
    STATE_FIXED_WAN_INTERFACE_UP
} WcFmobPolicyState_t;

WcFmobPolicyState_t fmob_sm_state;

/* STATES */
static WcFmobPolicyState_t State_FixingWanInterface(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcFmobPolicyState_t State_FixedWanInterfaceDown(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcFmobPolicyState_t State_FixedWanInterfaceUp(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);

/* TRANSITIONS */
static WcFmobPolicyState_t Transition_Start(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcFmobPolicyState_t Transition_WanInterfaceFixed(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcFmobPolicyState_t Transition_FixedInterfaceUp(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
static WcFmobPolicyState_t Transition_FixedInterfaceDown(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);

/* WanController_FixedModeOnBootupPolicy_SMThread */
void *WanController_FixedModeOnBootupPolicy_SMThread(void *arg)
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

    PDML_WAN_IFACE_GLOBAL_CONFIG pGlobalIfaceInfo = NULL;
    pGlobalIfaceInfo = (PDML_WAN_IFACE_GLOBAL_CONFIG)AnscAllocateMemory(sizeof(DML_WAN_IFACE_GLOBAL_CONFIG) * (pWanController->uInterfaceCount));
    if (pGlobalIfaceInfo == NULL) {
        CcspTraceError(("%s %d Invalid Memory \n", __FUNCTION__, __LINE__));
        pthread_exit(NULL);
    }

    //detach thread from caller stack
    pthread_detach(pthread_self());

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
    fmob_sm_state = Transition_Start(pWanController); // do this first before anything else to init variables

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
        switch (fmob_sm_state) {
            case STATE_FIXING_WAN_INTERFACE:
                fmob_sm_state = State_FixingWanInterface(pWanController);
                break;
            case STATE_FIXED_WAN_INTERFACE_DOWN:
                fmob_sm_state = State_FixedWanInterfaceDown(pWanController);
                break;
            case STATE_FIXED_WAN_INTERFACE_UP:
                fmob_sm_state = State_FixedWanInterfaceUp(pWanController);
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

static WcFmobPolicyState_t State_FixingWanInterface(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    int iLoopCount;
    PDML_WAN_IFACE_GLOBAL_CONFIG pInterfaces = NULL;

    if(pWanController == NULL) {
        return ANSC_STATUS_FAILURE;
    }

    pInterfaces = pWanController->pInterface;
    if(pInterfaces == NULL) {
        return ANSC_STATUS_FAILURE;
    }

    pWanController->activeInterface = -1;
    // Check the policy to determine if any primary interface should be used for WAN
    for( iLoopCount = 0; iLoopCount < pWanController->uInterfaceCount; iLoopCount++ )
    {
        if ((pInterfaces[iLoopCount].CfgEnable == TRUE) && pInterfaces[iLoopCount].CfgType == WAN_IFACE_TYPE_PRIMARY)
        {
            if(pWanController->activeInterface == -1)
            {
                pWanController->activeInterface = iLoopCount;
            }
            else if(pInterfaces[iLoopCount].CfgPriority < pInterfaces[pWanController->activeInterface].CfgPriority)
            {
                pWanController->activeInterface = iLoopCount;
            }
        }
    }

    if(pWanController->activeInterface != -1)
    {
        return Transition_WanInterfaceFixed(pWanController);
    }

    return STATE_FIXING_WAN_INTERFACE;
}

static WcFmobPolicyState_t State_FixedWanInterfaceDown(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    PDML_WAN_IFACE_GLOBAL_CONFIG pFixedInterface = NULL;

    if(pWanController == NULL) {
        return ANSC_STATUS_FAILURE;
    }

    pFixedInterface = (pWanController->pInterface) + (pWanController->activeInterface);

    if( pWanController->WanEnable == TRUE &&
        (pFixedInterface->CfgPhyStatus == WAN_IFACE_PHY_STATUS_UP ||
         pFixedInterface->CfgPhyStatus == WAN_IFACE_PHY_STATUS_INITIALIZING) &&
        pFixedInterface->CfgStatus == WAN_IFACE_STATUS_DISABLED &&
        pFixedInterface->CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN )
    {
        return Transition_FixedInterfaceUp(pWanController);
    }

    return STATE_FIXED_WAN_INTERFACE_DOWN;
}

static WcFmobPolicyState_t State_FixedWanInterfaceUp(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    PDML_WAN_IFACE_GLOBAL_CONFIG pFixedInterface = NULL;
    WanInterfaceData_t wanIf;

    if(pWanController == NULL) {
        return ANSC_STATUS_FAILURE;
    }

    pFixedInterface = (pWanController->pInterface) + (pWanController->activeInterface);

    if( pWanController->WanEnable == FALSE ||
        pFixedInterface->CfgPhyStatus == WAN_IFACE_PHY_STATUS_DOWN)
    {
        return Transition_FixedInterfaceDown(pWanController);
    }

    return STATE_FIXED_WAN_INTERFACE_UP;
}

static WcFmobPolicyState_t Transition_Start(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
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
    for( iLoopCount = 0; iLoopCount < pWanController->uInterfaceCount; iLoopCount++)
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

    CcspTraceInfo(("%s %d - State changed to STATE_FIXING_WAN_INTERFACE \n", __FUNCTION__, __LINE__));
    return STATE_FIXING_WAN_INTERFACE;
}

static WcFmobPolicyState_t Transition_WanInterfaceFixed(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    WanInterfaceData_t wanIf;

    if(pWanController == NULL) {
        return ANSC_STATUS_FAILURE;
    }

    /* Sets Wan.ActiveLink to TRUE for the current fixed interface */
    if(WanController_updateWanActiveLinkFlag((pWanController->activeInterface), TRUE) != ANSC_STATUS_SUCCESS)
    {
        return ANSC_STATUS_FAILURE;
    }

#ifdef _HUB4_PRODUCT_REQ_
    util_setWanLedState(FLASHING_AMBER);
#endif

    CcspTraceInfo(("%s %d - State changed to STATE_FIXED_WAN_INTERFACE_DOWN \n", __FUNCTION__, __LINE__));
    return STATE_FIXED_WAN_INTERFACE_DOWN;
}

static WcFmobPolicyState_t Transition_FixedInterfaceDown(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    CcspTraceInfo(("%s %d - State changed to STATE_FIXED_WAN_INTERFACE_DOWN \n", __FUNCTION__, __LINE__));
#ifdef _HUB4_PRODUCT_REQ_
    util_setWanLedState(OFF);
#endif
    return STATE_FIXED_WAN_INTERFACE_DOWN;
}

static WcFmobPolicyState_t Transition_FixedInterfaceUp(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    PDML_WAN_IFACE_GLOBAL_CONFIG pFixedInterface = NULL;
    WanInterfaceData_t wanIf;

    if(pWanController == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

    pFixedInterface = (pWanController->pInterface) + (pWanController->activeInterface);
    if(pFixedInterface == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

    /* Starts an instance of the WAN Interface State Machine on the interface to begin configuring the WAN link */
    strncpy(wanIf.ifName, pFixedInterface->CfgWanName, sizeof(wanIf.ifName));
    strncpy(wanIf.baseIfName, pFixedInterface->CfgBaseifName, sizeof(wanIf.baseIfName));
#ifdef _HUB4_PRODUCT_REQ_
    util_setWanLedState(SOLID_AMBER);
#endif
    WanManager_StartStateMachine(&wanIf);

    CcspTraceInfo(("%s %d - State changed to STATE_FIXED_WAN_INTERFACE_UP \n", __FUNCTION__, __LINE__));
    return STATE_FIXED_WAN_INTERFACE_UP;
}
