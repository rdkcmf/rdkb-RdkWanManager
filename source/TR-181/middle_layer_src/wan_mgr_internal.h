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


#ifndef  _WA_INTERNAL_H
#define  _WA_INTERNAL_H

#include "wan_mgr_apis.h"
#include "plugin_main_apis.h"
#define BUFFER_LEN_16 16

/** enum wan policy */
typedef enum
_DML_WAN_POLICY
{
   FIXED_MODE_ON_BOOTUP = 1,
   FIXED_MODE,
   PRIMARY_PRIORITY_ON_BOOTUP,
   PRIMARY_PRIORITY,
   MULTIWAN_MODE
} DML_WAN_POLICY;

#define  DATAMODEL_WANMANAGER_CLASS_CONTENT \
	 BASE_CONTENT \
     BOOLEAN Enable; \
     DML_WAN_POLICY Policy; \
     unsigned int IdleTimeout; \

typedef  struct
_DATAMODEL_WANMANAGER_CLASS_CONTENT
{
    DATAMODEL_WANMANAGER_CLASS_CONTENT
}
DATAMODEL_WANMANAGER, *PDATAMODEL_WANMANAGER;

/*
    Standard function declaration
*/
ANSC_HANDLE
WanManagerCreate
    (
        VOID
    );

ANSC_STATUS
WanManagerInitialize
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
WanManagerRemove
    (
        ANSC_HANDLE                 hThisObject
    );

#endif
