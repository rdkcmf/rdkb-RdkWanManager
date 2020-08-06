/*
 * If not stated otherwise in this file or this component's LICENSE file the
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

#ifndef _WAN_MANAGER_DHCPV4_APIS_H_
#define _WAN_MANAGER_DHCPV4_APIS_H_
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "ansc_platform.h"

#define DHCPV4_CLIENT_NAME "udhcpc"
#define DHCPV4_ACTION_HANDLER "service_udhcpc"
#define SYSEVENT_IPV4_LEASE_TIME  "ipv4_%s_lease_time"
#define SYSEVENT_IPV4_DHCP_SERVER "ipv4_%s_dhcp_server"
#define SYSEVENT_IPV4_DHCP_STATE  "ipv4_%s_dhcp_state"
#define SYSEVENT_IPV4_START_TIME  "ipv4_%s_start_time"

/**
 * @brief API to process DHCP state change event message.
 * @param msg - Pointer to msg_payload_t structure contains Dhcpv4 configuration as part of ipc message
 * @return ANSC_STATUS_SUCCESS upon success else error code returned.
 */
ANSC_STATUS WanManager_Dhcpv4_ProcessStateChangedMsg(dhcpv4_data_t * msg);
#endif
