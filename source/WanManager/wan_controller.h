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

#ifndef WAN_CONTROLLER_H
#define WAN_CONTROLLER_H

#include "wan_mgr_internal.h"
#include "wan_interface_dml_apis.h"
#include "wan_manager.h"

typedef enum
_WAN_CONTROLLER_EVENTS
{
    WAN_ENABLE = 1,
    WAN_POLICY_CHANGED
} WAN_CONTROLLER_EVENTS;

typedef  struct
_DML_WAN_CONFIGURATION
{
    PDML_WAN_IFACE pInterface;
    WAN_CONTROLLER_EVENTS event;
    UINT value;
}DML_WAN_CONFIGURATION;

typedef  struct
_WAN_CONTROLLER_PRIVATE_SM_INFO
{
    BOOL WanEnable;
    UINT uInterfaceCount;
    PDML_WAN_IFACE_GLOBAL_CONFIG pInterface;
    INT activeInterface;
    INT activeSecondaryInterface;
}WAN_CONTROLLER_PRIVATE_SM_INFO, *PWAN_CONTROLLER_PRIVATE_SM_INFO;

ANSC_STATUS WanController_Init_StateMachine(PANSC_HANDLE phContext);
ANSC_STATUS WanController_Start_StateMachine(DML_WAN_POLICY wan_policy, PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController);
ANSC_STATUS WanController_Conf_Changed(DML_WAN_CONFIGURATION config);

/* Thread routines */
void* WanController_FixedModePolicy_SMThread(void *arg);
void* WanController_FixedModeOnBootupPolicy_SMThread(void *arg);
void* WanController_PrimaryPriorityPolicy_SMThread(void *arg);
void* WanController_PrimaryPriorityOnBootupPolicy_SMThread(void *arg);
#endif /*WAN_CONTROLLER_H*/
