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


#ifndef WAN_MANAGER_DB_H
#define WAN_MANAGER_DB_H

/* ---- Include Files ---------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wan_manager.h"
#include "wan_manager_private.h"
#include "wan_manager_dhcpv4_apis.h"

int wanmgr_sysevent_init();
void wanmgr_sysevent_close();

/*
 * @brief Utility function used to set string values to syscfg.
 * @param[in] const char* name - Indicates string name represent the value
 * @param[in] const char* value- Indicates the value to pass to the curresponding mem
 * @return Returns ANSC_STATUS.
*/
ANSC_STATUS syscfg_set_string(const char* name, const char* value);

/*
 * @brief Utility function used to set bool values to syscfg.
 * @param[in] const char* name - Indicates string name represent the value
 * @param[in] int value- Indicates the value to pass to the curresponding mem
 * @return Returns ANSC_STATUS.
*/
ANSC_STATUS syscfg_set_bool(const char* name, int value);

/*
 * @brief Utility function used to init all IPv6 values in sysevent
 * @param[in] None
 * @return Returns ANSC_STATUS.
*/
ANSC_STATUS ipv6Info_init(dhcpv6_data_t *ipv6Data);

/*
 * @brief Utility function used to init all IPv4 values in sysevent
 * @param[in]
 * @return Returns ANSC_STATUS.
*/
ANSC_STATUS ipv4Info_init(dhcpv4_data_t *ipv4Data, const char *wanIfName);

/*
 * @brief Utility function used to store all dhcpv4_data_t values in sysevent
 * @param[in] dhcpv4_data_t *dhcp4Info
 * @return Returns ANSC_STATUS.
*/
ANSC_STATUS ipv4Info_set(const dhcpv4_data_t *dhcp4Info, const char *wanIfName);

#ifdef FEATURE_MAPT
/*
 * @brief Utility function used to store MAPT specific values in sysevent/
 * @param[in] MaptData_t *maptInfo
 * @return Returns ANSC_STATUS_SUCCESS if data properly update in sysevent.
*/
ANSC_STATUS maptInfo_set(const MaptData_t *maptInfo);
ANSC_STATUS maptInfo_reset();
#endif // FEATURE_MAPT


#endif /* WAN_MANAGER_DB_H */
