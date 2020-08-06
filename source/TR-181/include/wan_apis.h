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

#ifndef  _WA_APIS_H
#define  _WA_APIS_H

#include "wan_mgr_apis.h"
#include "plugin_main_apis.h"

/**********************************************************************
                STRUCTURE AND CONSTANT DEFINITIONS
**********************************************************************/


/**********************************************************************
                FUNCTION PROTOTYPES
**********************************************************************/
ANSC_STATUS DmlWanManagerInit(ANSC_HANDLE hDml, PANSC_HANDLE phContext);
ANSC_STATUS DmlWanManagerGetWanConnectionEnable(ANSC_HANDLE hContext, unsigned int *);
ANSC_STATUS DmlWanManagerSetWanConnectionEnable(ANSC_HANDLE hContext, unsigned int wan_old_status, unsigned int wan_new_status);
ANSC_STATUS DmlWanManagerSetWanPolicy(ANSC_HANDLE hContext, unsigned int wan_policy);
ANSC_STATUS DmlWanManagerCommit(ANSC_HANDLE hContext);
#endif
