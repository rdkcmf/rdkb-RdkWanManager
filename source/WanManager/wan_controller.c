/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
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

#include "wan_controller.h"
#include "wan_manager.h"
#include "wan_mgr_internal.h"
#include "wan_interface_internal.h"
#include "wan_interface_dml_apis.h"
#include "dmsb_tr181_psm_definitions.h"
#include "wan_manager_private.h"
#include "wan_controller_utils.h"
#include "ansc_platform.h"
#include "plugin_main_apis.h"

#define  SYSEVENT_XDSL_LINE_TRAINING    "xdsl_line_training"
#define  STARTED                        "started"

extern char g_Subsystem[32];
extern ANSC_HANDLE bus_handle;

//Wan event handling routines
static ANSC_STATUS handle_wan_policy_change_event();
static ANSC_STATUS handle_wan_enable_event(DML_WAN_CONFIGURATION config);

//Global mutex for wan controller state machine
pthread_mutex_t gmWanInterfaceData = PTHREAD_MUTEX_INITIALIZER;

/* WanController_Init_StateMachine */
ANSC_STATUS WanController_Init_StateMachine(PANSC_HANDLE phContext)
{
    int iTotalInterfaces = 0;
    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;
    DML_WAN_POLICY wan_policy;

    CcspTraceInfo(("%s %d \n", __FUNCTION__, __LINE__ ));
    PDATAMODEL_WAN_IFACE pMyObject = (PDATAMODEL_WAN_IFACE) phContext;

    if(pMyObject == NULL)  {
        CcspTraceInfo(("%s %d  Error: pMyObject is null \n", __FUNCTION__, __LINE__ ));
        return ANSC_STATUS_FAILURE;
    }

    PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController = pMyObject->pWanController;

    if(pWanController == NULL) {
        CcspTraceInfo(("%s %d  Error: pWanController is null \n", __FUNCTION__, __LINE__ ));
        return ANSC_STATUS_FAILURE;
    }

    // Get the configured wan policy
    if(WanController_getWanPolicy(&wan_policy) != ANSC_STATUS_SUCCESS) {
        CcspTraceInfo(("%s %d  Error: WanController_getWanPolicy() failed \n", __FUNCTION__, __LINE__ ));
        return ANSC_STATUS_FAILURE;
    }

    if(DmlGetTotalNoOfWanInterfaces(&iTotalInterfaces) == ANSC_STATUS_FAILURE) {
        return ANSC_STATUS_FAILURE;
    }

    pWanController->uInterfaceCount = iTotalInterfaces;
    pWanController->WanEnable = TRUE;
    pWanController->activeInterface = -1;
    pWanController->activeSecondaryInterface = -1;

    if(WanController_Start_StateMachine(wan_policy, pWanController) != ANSC_STATUS_SUCCESS) {
        CcspTraceInfo(("%s %d Error: WanController_Start_StateMachine failed \n", __FUNCTION__, __LINE__ ));
        return ANSC_STATUS_FAILURE;
    }

    return returnStatus;
}

/* WanController_Start_StateMachine() */
ANSC_STATUS WanController_Start_StateMachine(DML_WAN_POLICY wan_policy, PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController)
{
    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;
    pthread_t StateMachineThread;
    int iErrorCode = 0;

    CcspTraceInfo(("%s %d \n", __FUNCTION__, __LINE__ ));

    //Starts wan controller threads
    switch (wan_policy) {
        case FIXED_MODE:
            iErrorCode = pthread_create( &StateMachineThread, NULL, &WanController_FixedModePolicy_SMThread, (void *) pWanController);
            break;

        case FIXED_MODE_ON_BOOTUP:
            iErrorCode = pthread_create( &StateMachineThread, NULL, &WanController_FixedModeOnBootupPolicy_SMThread, (void *) pWanController);
            break;

        case PRIMARY_PRIORITY:
            iErrorCode = pthread_create( &StateMachineThread, NULL, &WanController_PrimaryPriorityPolicy_SMThread, (void *) pWanController);
            break;

        case PRIMARY_PRIORITY_ON_BOOTUP:
            iErrorCode = pthread_create( &StateMachineThread, NULL, &WanController_PrimaryPriorityOnBootupPolicy_SMThread, (void *) pWanController);
            break;

        case MULTIWAN_MODE:
            break;
    }

    if( 0 != iErrorCode ) {
        CcspTraceInfo(("%s %d Error: Failed to start State Machine Thread error code: %d \n", __FUNCTION__, __LINE__, iErrorCode ));
    }

    return returnStatus;
}

static ANSC_STATUS handle_wan_policy_change_event() {
    /* Wan policy changed. Cpe needs a restart! */
    FILE *fp = NULL;
    char value[25] = {0};
    char cmd[128] = {0};
    char acOutput[64] = {0};
    int seconds = 30;
    int rebootCount = 1;

    memset(value, 0, sizeof(value));
    fp = popen("syscfg get X_RDKCENTRAL-COM_LastRebootCounter", "r");
    if (fp == NULL) {
        return ANSC_STATUS_FAILURE;
    }
    pclose(fp);

    rebootCount = atoi(value);

    CcspTraceInfo(("Updating the last reboot reason and last reboot counter\n"));
    sprintf(cmd, "syscfg set X_RDKCENTRAL-COM_LastRebootReason Wan_Policy_Change ");
    system(cmd);
    sprintf(cmd, "syscfg set X_RDKCENTRAL-COM_LastRebootCounter %d ",rebootCount);
    system(cmd);
    system("syscfg commit");

    while(seconds > 0)
    {
        printf("...(%d)...\n", seconds);
        seconds -= 10;
        sleep(10);
    }

    system("/rdklogger/backupLogs.sh true");

    return ANSC_STATUS_SUCCESS;
}

static ANSC_STATUS handle_wan_enable_event(DML_WAN_CONFIGURATION config) {
    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;
    PDATAMODEL_WAN_IFACE pMyObject = (PDATAMODEL_WAN_IFACE)g_pBEManager->hWanIface;
    PWAN_CONTROLLER_PRIVATE_SM_INFO p_WanController = pMyObject->pWanController;
    p_WanController->WanEnable = config.value;
    return returnStatus;
}

ANSC_STATUS WanController_Conf_Changed(DML_WAN_CONFIGURATION config)
{
    ANSC_STATUS returnStatus = ANSC_STATUS_SUCCESS;

    switch(config.event) {
        case WAN_POLICY_CHANGED:
            returnStatus = handle_wan_policy_change_event();
            if(returnStatus != ANSC_STATUS_SUCCESS) {
                AnscTraceWarning(("handle_wan_policy_change_event failed! \n"));
            }
            break;
        case WAN_ENABLE:
            returnStatus = handle_wan_enable_event(config);
            if(returnStatus != ANSC_STATUS_SUCCESS) {
                AnscTraceWarning(("handle_wan_enable_event failed! \n"));
            }
            break;
    }

    return returnStatus;
}
