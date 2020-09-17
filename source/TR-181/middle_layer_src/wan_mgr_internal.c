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

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

#include "plugin_main_apis.h"
#include "wan_apis.h"
#include "wan_mgr_internal.h"


/**********************************************************************

    caller:     owner of the object

    prototype:

        ANSC_HANDLE
        WanManagerCreate
            (
            );

    description:

        This function constructs cosa wan manager object and return handle.

    argument:  

    return:     newly created wan manager object.

**********************************************************************/

ANSC_HANDLE
WanManagerCreate
    (
        VOID
    )
{
    ANSC_STATUS                returnStatus = ANSC_STATUS_SUCCESS;
    PDATAMODEL_WANMANAGER   pMyObject    = (PDATAMODEL_WANMANAGER)NULL;

    /*
     * We create object by first allocating memory for holding the variables and member functions.
     */
    pMyObject = (PDATAMODEL_WANMANAGER)AnscAllocateMemory(sizeof(DATAMODEL_WANMANAGER));

    if ( !pMyObject )
    {
        return  (ANSC_HANDLE)NULL;
    }

    AnscZeroMemory(pMyObject, sizeof(DATAMODEL_WANMANAGER));

    /*
     * Initialize the common variables and functions for a container object.
     */

    pMyObject->Create            = WanManagerCreate;
    pMyObject->Remove            = WanManagerRemove;
    pMyObject->Initialize        = WanManagerInitialize;

    pMyObject->Initialize   ((ANSC_HANDLE)pMyObject);

    return  (ANSC_HANDLE)pMyObject;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        WanManagerInitialize
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate  cosa device info object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
WanManagerInitialize
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                returnStatus = ANSC_STATUS_SUCCESS;
    PDATAMODEL_WANMANAGER   pMyObject    = (PDATAMODEL_WANMANAGER)hThisObject;

    /* Initiation all functions */

    /* Initialize middle layer for Device.WanManager.  */
    DmlWanManagerInit(NULL, (PANSC_HANDLE)pMyObject);

    return returnStatus;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        WanManagerRemove
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate cosa wan manager object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
WanManagerRemove
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                   returnStatus = ANSC_STATUS_SUCCESS;
    PDATAMODEL_WANMANAGER      pMyObject    = (PDATAMODEL_WANMANAGER)hThisObject;

    /* Remove necessary resounce */

    /* Remove self */
    AnscFreeMemory((ANSC_HANDLE)pMyObject);

    return returnStatus;
}


