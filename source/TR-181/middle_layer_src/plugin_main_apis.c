/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
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

/*********************************************************************** 
  
    module: plugin_main_apis.c

        Implement COSA Data Model Library Init and Unload apis.
        This files will hold all data in it.
 
    ---------------------------------------------------------------

    description:

        This module implements the advanced state-access functions
        of the Dslh Var Record Object.

        *   BackEndManagerCreate
        *   BackEndManagerInitialize
        *   BackEndManagerRemove
    ---------------------------------------------------------------

    author:

        COSA XML TOOL CODE GENERATOR 1.0

    ---------------------------------------------------------------

    revision:

        01/11/2011    initial revision.

**********************************************************************/

//#include "dml_tr181_custom_cfg.h"
#include "plugin_main_apis.h"

/*PCOSA_DIAG_PLUGIN_INFO             g_pCosaDiagPluginInfo;*/
COSAGetParamValueByPathNameProc    g_GetParamValueByPathNameProc;
COSASetParamValueByPathNameProc    g_SetParamValueByPathNameProc;
COSAGetParamValueStringProc        g_GetParamValueString;
COSAGetParamValueUlongProc         g_GetParamValueUlong;
COSAGetParamValueIntProc           g_GetParamValueInt;
COSAGetParamValueBoolProc          g_GetParamValueBool;
COSASetParamValueStringProc        g_SetParamValueString;
COSASetParamValueUlongProc         g_SetParamValueUlong;
COSASetParamValueIntProc           g_SetParamValueInt;
COSASetParamValueBoolProc          g_SetParamValueBool;
COSAGetInstanceNumbersProc         g_GetInstanceNumbers;

COSAValidateHierarchyInterfaceProc g_ValidateInterface;
COSAGetHandleProc                  g_GetRegistryRootFolder;
COSAGetInstanceNumberByIndexProc   g_GetInstanceNumberByIndex;
COSAGetInterfaceByNameProc         g_GetInterfaceByName;
COSAGetHandleProc                  g_GetMessageBusHandle;
COSAGetSubsystemPrefixProc         g_GetSubsystemPrefix;
PCCSP_CCD_INTERFACE                g_pPnmCcdIf;
ANSC_HANDLE                        g_MessageBusHandle;
char*                              g_SubsystemPrefix;
COSARegisterCallBackAfterInitDmlProc  g_RegisterCallBackAfterInitDml;
COSARepopulateTableProc            g_COSARepopulateTable;

/**********************************************************************

    caller:     owner of the object

    prototype:

        ANSC_HANDLE
        BackEndManagerCreate
            (
            );

    description:

        This function constructs cosa datamodel object and return handle.

    argument:  

    return:     newly created qos object.

**********************************************************************/

ANSC_HANDLE
BackEndManagerCreate
    (
        VOID
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PBACKEND_MANAGER_OBJECT    pMyObject    = (PBACKEND_MANAGER_OBJECT)NULL;

    /*
        * We create object by first allocating memory for holding the variables and member functions.
        */
    pMyObject = (PBACKEND_MANAGER_OBJECT)AnscAllocateMemory(sizeof(BACKEND_MANAGER_OBJECT));

    if ( !pMyObject )
    {
        return  (ANSC_HANDLE)NULL;
    }

    /*
     * Initialize the common variables and functions for a container object.
     */
    pMyObject->Oid               = DATAMODEL_BASE_OID;
    pMyObject->Create            = BackEndManagerCreate;
    pMyObject->Remove            = BackEndManagerRemove;
    pMyObject->Initialize        = BackEndManagerInitialize;

    /*pMyObject->Initialize   ((ANSC_HANDLE)pMyObject);*/

    return  (ANSC_HANDLE)pMyObject;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        BackEndManagerInitialize
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate cosa manager object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
BackEndManagerInitialize
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PBACKEND_MANAGER_OBJECT  pMyObject    = (PBACKEND_MANAGER_OBJECT)hThisObject;

    if (pMyObject == NULL)
    {
        AnscTraceError(("%s:%d:: Pointer is null!!\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

#ifdef _COSA_SIM_
        pMyObject->has_moca_slap  = 0;
        pMyObject->has_wifi_slap  = 0;    
#endif

    AnscTraceWarning(("%s...\n", __FUNCTION__));

#ifdef _COSA_INTEL_XB3_ARM_
	//syscfg init
    if ( 0 != syscfg_init( ) ) 
	{
		AnscTraceWarning(("%s syscfg_init() fails\n", __FUNCTION__));
    }  
#endif /* _COSA_INTEL_XB3_ARM_ */	
		
    /*create a thread to handle the sysevent asynchronously after all of the dmls created
     *
     * when a specified event trigger, the related functions which are registered previously
     * will run step by step.
     */
    // need to revisit this handling thread. causing SEGV.  RTian 5/31/2014
    // EvtDispterHandleEventAsync();

    pMyObject->hWA  = (ANSC_HANDLE)WanManagerCreate();
    AnscTraceWarning(("WanManagerCreate done!\n"));
	
	pMyObject->hWanIface  = (ANSC_HANDLE)WanIfaceCreate();
    AnscTraceWarning(("WanIfaceCreate done!\n"));

    return returnStatus;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        BackEndManagerRemove
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function remove cosa manager object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
BackEndManagerRemove
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PBACKEND_MANAGER_OBJECT  pMyObject    = (PBACKEND_MANAGER_OBJECT)hThisObject;

    if (pMyObject == NULL)
    {
        AnscTraceError(("%s:%d:: Pointer is null!!\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* Remove self */
    AnscFreeMemory((ANSC_HANDLE)pMyObject);

    return returnStatus;
}
