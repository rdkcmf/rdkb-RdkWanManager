
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

#ifndef WAN_CONTROLLER_UTILS_H
#define WAN_CONTROLLER_UTILS_H

#include <stdbool.h>
#include "wan_mgr_apis.h"
#include "wan_controller.h"
#include "wan_mgr_internal.h"
#include "wan_interface_dml_apis.h"

ANSC_STATUS WanController_getWanPolicy(DML_WAN_POLICY *wan_policy);
ANSC_STATUS WanController_updateWanInterfaceUpstreamFlag(PWAN_CONTROLLER_PRIVATE_SM_INFO pController, bool flag);
ANSC_STATUS WanController_updateWanActiveLinkFlag(int instance, bool flag);

#endif /* WAN_CONTROLLER_UTILS_H */
