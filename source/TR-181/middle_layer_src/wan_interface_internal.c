/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
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

/**************************************************************************

    module: wan_interface_internal.c

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    description:

        This file implementes back-end apis for the COSA Data Model Library

        *  WanIfaceCreate
        *  WanIfaceInitialize
        *  WanIfaceRemove
    -------------------------------------------------------------------

    environment:

        platform independent

    -------------------------------------------------------------------

    author:

        COSA XML TOOL CODE GENERATOR 1.0

    -------------------------------------------------------------------

    revision:

        20/12/2019    initial revision.

**************************************************************************/

#include "plugin_main_apis.h"
#include "wan_interface_dml_apis.h"
#include "wan_interface_dml.h"
#include "wan_interface_internal.h"

/**********************************************************************

    caller:     owner of the object

    prototype:

        ANSC_HANDLE
        WanIfaceCreate
            (
            );

    description:

        This function constructs cosa device info object and return handle.

    argument:  

    return:     newly created device info object.

**********************************************************************/

ANSC_HANDLE
WanIfaceCreate
    (
        VOID
    )
{
    ANSC_STATUS                 returnStatus = ANSC_STATUS_SUCCESS;
    PDATAMODEL_WAN_IFACE   pMyObject    = (PDATAMODEL_WAN_IFACE)NULL;

    /*
     * We create object by first allocating memory for holding the variables and member functions.
     */
    pMyObject = (PDATAMODEL_WAN_IFACE)AnscAllocateMemory(sizeof(DATAMODEL_WAN_IFACE));

    if ( !pMyObject )
    {
        return  (ANSC_HANDLE)NULL;
    }

    AnscZeroMemory(pMyObject, sizeof(DATAMODEL_WAN_IFACE));

    /*
     * Initialize the common variables and functions for a container object.
     */

    pMyObject->Create            = WanIfaceCreate;
    pMyObject->Remove            = WanIfaceRemove;
    pMyObject->Initialize        = WanIfaceInitialize;

    pMyObject->Initialize   ((ANSC_HANDLE)pMyObject);

    return  (ANSC_HANDLE)pMyObject;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        WanIfaceInitialize
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
WanIfaceInitialize
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS   returnStatus                       = ANSC_STATUS_SUCCESS;
    PDATAMODEL_WAN_IFACE   pMyObject            = (PDATAMODEL_WAN_IFACE)hThisObject;
    
    /* Initiation all functions */
    
    /* Initialize middle layer for Device.X_RDK_WanManager.CPEInterface.  */
    DmlWanIfInit(NULL, (PANSC_HANDLE)pMyObject);

    return returnStatus;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        WanIfaceRemove
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
WanIfaceRemove
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS  returnStatus             = ANSC_STATUS_SUCCESS;
    PDATAMODEL_WAN_IFACE  pMyObject  = (PDATAMODEL_WAN_IFACE)hThisObject;
    PDML_WAN_IFACE  pWanIfaceTmp     = pMyObject->pWanIface;

    /* Remove necessary resource */
    AnscFreeMemory( pWanIfaceTmp );

    /* Remove self */
    AnscFreeMemory((ANSC_HANDLE)pMyObject);

    return returnStatus;
}
