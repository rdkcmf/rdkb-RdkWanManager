/*
 * If not stated otherwise in this file or this component's LICENSE file the
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

#ifndef _WANMGR_DHCPV6_APIS_H_
#define _WANMGR_DHCPV6_APIS_H_
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "ansc_platform.h"
#include "ipc_msg.h"


/**
 * @brief API to process DHCP state change event message.
 * @param msg - Pointer to msg_payload_t structure contains Dhcpv6 configuration as part of ipc message
 * @return ANSC_STATUS_SUCCESS upon success else error code returned.
 */
ANSC_STATUS wanmgr_handle_dchpv6_event_data(DML_WAN_IFACE* pIfaceData);
void* IPV6CPStateChangeHandler (void *arg);

#endif //_WANMGR_DHCPV6_APIS_H_
