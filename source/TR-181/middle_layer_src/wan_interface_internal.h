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

#ifndef  _WAN_INTERFACE_INTERNAL_H
#define  _WAN_INTERFACE_INTERNAL_H

#include "wan_mgr_apis.h"
#include "plugin_main_apis.h"
#include "wan_interface_dml_apis.h"
#include "wan_controller.h"

/* Collection */

#define  DATAMODEL_WAN_IFACE_CLASS_CONTENT            \
    /* duplication of the base object class content */     \
    BASE_CONTENT                                      \
    /* start of Wan Interface object class content */      \
    UINT                            ulTotalNoofWanInterfaces; \
    PDML_WAN_IFACE             pWanIface;          \
    PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController      \

typedef  struct
_DATAMODEL_WAN_IFACE
{
	DATAMODEL_WAN_IFACE_CLASS_CONTENT
}
DATAMODEL_WAN_IFACE,  *PDATAMODEL_WAN_IFACE;

/*
    Standard function declaration 
*/
ANSC_HANDLE
WanIfaceCreate
    (
        VOID
    );

ANSC_STATUS
WanIfaceInitialize
    (
        ANSC_HANDLE                 hThisObject
    );

ANSC_STATUS
WanIfaceRemove
    (
        ANSC_HANDLE                 hThisObject
    );
#endif 
