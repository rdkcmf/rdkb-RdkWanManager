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

#include "ansc_platform.h"
#include "plugin_main_apis.h"
#include "wan_interface_dml_apis.h"
#include "wan_interface_dml.h"
#include "wan_interface_internal.h"
#include "wan_controller.h"

/***********************************************************************

 APIs for Object:

    X_RDK_WanManager.CPEInterface.

    *  WanIf_GetEntryCount
    *  WanIf_GetEntry
    *  WanIf_GetParamStringValue
    *  WanIf_SetParamStringValue
    *  WanIf_Validate
    *  WanIf_Commit
    *  WanIf_Rollback

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        WanIf_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
WanIf_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PDATAMODEL_WAN_IFACE   pMyObject   = (PDATAMODEL_WAN_IFACE)g_pBEManager->hWanIface;

    return pMyObject->ulTotalNoofWanInterfaces;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        WanIf_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
WanIf_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
    PDATAMODEL_WAN_IFACE    pMyObject  = (PDATAMODEL_WAN_IFACE)g_pBEManager->hWanIface;
	
    if ( ( pMyObject->pWanIface  ) && ( nIndex < pMyObject->ulTotalNoofWanInterfaces ) )
    {
    	PDML_WAN_IFACE      pWanIface = NULL;
		
        pWanIface = pMyObject->pWanIface + nIndex;

        pWanIface->ulInstanceNumber = nIndex + 1;

        *pInsNumber = pWanIface->ulInstanceNumber;

        //Sync with current information
        DmlGetWanIfCfg(nIndex, pWanIface);

        return pWanIface;
    }
	
    return NULL; /* return the invlalid handle */
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        WanIf_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
WanIf_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PDML_WAN_IFACE      pWanIface = (PDML_WAN_IFACE)hInsContext;
    
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Name", TRUE) )
    {
       /* collect value */
       if ( ( sizeof( pWanIface->Name ) - 1 ) < *pUlSize )
       {
           AnscCopyString( pValue, pWanIface->Name );
           return 0;
       }
       else
       {
           *pUlSize = sizeof( pWanIface->Name );
           return 1;
       }
    }

    if( AnscEqualString(ParamName, "DisplayName", TRUE) )
    {
       /* collect value */
       if ( ( sizeof( pWanIface->DisplayName ) - 1 ) < *pUlSize )
       {
           AnscCopyString( pValue, pWanIface->DisplayName );
           return 0;
       }
       else
       {
           *pUlSize = sizeof( pWanIface->DisplayName );
           return 1;
       }
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        WanIf_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIf_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    PDML_WAN_IFACE      pWanIface = (PDML_WAN_IFACE)hInsContext;
    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "Name", TRUE)) {
        AnscCopyString(pWanIface->Name, pString);
        return TRUE;
    }
	
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIf_Validate
    	(
        	ANSC_HANDLE                 hInsContext,
        	char*                       pReturnParamName,
        	ULONG*                      puLength
    	)
    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/

BOOL
WanIf_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
	return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIf_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIf_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return 0;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIf_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIf_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return 0;
}

/***********************************************************************

 APIs for Object:

    X_RDK_WanManager.CPEInterface.{i}.Wan.

    *  WanIfCfg_GetParamUlongValue
    *  WanIfCfg_SetParamUlongValue
    *  WanIfCfg_GetParamBoolValue
    *  WanIfCfg_SetParamBoolValue
    *  WanIfCfg_GetParamIntValue
    *  WanIfCfg_SetParamIntValue
    *  WanIfCfg_Validate
    *  WanIfCfg_Commit
    *  WanIfCfg_Rollback

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfCfg_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfCfg_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    ) {

    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Priority", TRUE))
    {
        *pInt = pWanIface->CfgPriority;
        return TRUE;
    }
	
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfCfg_SetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int                         iValue
            );

    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int                         iValue
                The updated int value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfCfg_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "Priority", TRUE))
    {
        pWanIface->CfgPriority = iValue;
        DmlWanIfSetCfgPriority(pWanIface->ulInstanceNumber - 1, pWanIface->CfgPriority);
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfCfg_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfCfg_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    ) {

    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and return the corresponding value */

    if( AnscEqualString(ParamName, "SelectionTimeout", TRUE))
    {
        *puLong = pWanIface->CfgSelectionTimeout;
        return TRUE;
    }
	if( AnscEqualString(ParamName, "Status", TRUE))
    {
        *puLong = pWanIface->CfgStatus;
        return TRUE;
    }	
	if( AnscEqualString(ParamName, "Type", TRUE))
    {
        *puLong = pWanIface->CfgType;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "Priority", TRUE))
    {
        *puLong = pWanIface->CfgPriority;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "LinkStatus", TRUE))
    {
        *puLong = pWanIface->CfgLinkStatus;
        return TRUE;
    }
	
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfCfg_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfCfg_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "SelectionTimeout", TRUE))
    {
        pWanIface->CfgSelectionTimeout = uValue;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "Type", TRUE))
    {
        pWanIface->CfgType = uValue;
        DmlWanIfSetCfgType(pWanIface->ulInstanceNumber - 1, pWanIface->CfgType);
        return TRUE;
    }
    if( AnscEqualString(ParamName, "Priority", TRUE))
    {
        pWanIface->CfgPriority = uValue;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "LinkStatus", TRUE))
    {
        pWanIface->CfgLinkStatus = uValue;
        DmlWanIfSetCfgLinkStatus(pWanIface->ulInstanceNumber - 1, pWanIface->CfgLinkStatus);
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfCfg_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfCfg_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    ) {

    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Enable", TRUE))
    {
        *pBool = pWanIface->CfgEnable;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "Refresh", TRUE))
    {
        *pBool = pWanIface->CfgRefresh;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "ActiveLink", TRUE))
    {
        *pBool = pWanIface->CfgActiveLink;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "EnablePPP", TRUE))
    {
        *pBool = pWanIface->CfgEnablePPP;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "EnableDSLite", TRUE))
    {
        *pBool = pWanIface->CfgEnableDSLite;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "EnableIPoEHealthCheck", TRUE))
    {
        *pBool = pWanIface->CfgEnableIPoE;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "EnableMAPT", TRUE))
    {
        *pBool = pWanIface->CfgEnableMAPT;
        return TRUE;
    }
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfCfg_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfCfg_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;
    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "Enable", TRUE))
    {
        pWanIface->CfgEnable  = bValue;
        DmlWanIfSetCfgEnable(pWanIface->ulInstanceNumber - 1, pWanIface->CfgEnable);
        return TRUE;
    }
    if( AnscEqualString(ParamName, "Refresh", TRUE))
    {
        pWanIface->CfgRefresh = bValue;
        DmlWanIfSetCfgRefreshStatus(pWanIface->ulInstanceNumber - 1, pWanIface->CfgRefresh);
        return TRUE;
    }
    if( AnscEqualString(ParamName, "ActiveLink", TRUE))
    {
        pWanIface->CfgActiveLink = bValue;
        DmlWanIfSetCfgActiveLink(pWanIface->ulInstanceNumber - 1, pWanIface->CfgActiveLink);
        return TRUE;
    }
    if( AnscEqualString(ParamName, "EnablePPP", TRUE))
    {
        pWanIface->CfgEnablePPP = bValue;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "EnableDSLite", TRUE))
    {
        pWanIface->CfgEnableDSLite = bValue;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "EnableIPoEHealthCheck", TRUE))
    {
        pWanIface->CfgEnableIPoE = bValue;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "EnableMAPT", TRUE))
    {
        pWanIface->CfgEnableMAPT = bValue;
        return TRUE;
    }
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        WanIfCfg_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
BOOL
WanIfCfg_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    ) {
    PDML_WAN_IFACE  pWanIface = (PDML_WAN_IFACE)hInsContext;
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Name", TRUE) )
    {
       /* collect value */
       if ( ( sizeof( pWanIface->CfgName ) - 1 ) < *pUlSize ) {
           AnscCopyString( pValue, pWanIface->CfgName );
           return 0;
       }
       else {
           *pUlSize = sizeof( pWanIface->CfgName );
           return 1;
       }
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        WanIfCfg_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfCfg_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    ) {
    PDML_WAN_IFACE      pWanIface = (PDML_WAN_IFACE)hInsContext;
    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "Name", TRUE)) {
        AnscCopyString(pWanIface->CfgName, pString);
        DmlWanIfSetCfgName(pWanIface->ulInstanceNumber - 1, pString);
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfCfg_Validate
    	(
        	ANSC_HANDLE                 hInsContext,
        	char*                       pReturnParamName,
        	ULONG*                      puLength
    	)
    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfCfg_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    ) {
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIfCfg_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIfCfg_Commit
    (
        ANSC_HANDLE                 hInsContext
    ) {
    ANSC_STATUS result = ANSC_STATUS_SUCCESS;
    PDML_WAN_IFACE pWanIface = (PDML_WAN_IFACE)hInsContext;

    DmlSetWanIfCfg( pWanIface->ulInstanceNumber, pWanIface );
    if(result != ANSC_STATUS_SUCCESS) {
        AnscTraceError(("%s: Failed \n", __FUNCTION__));
    }

    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIfCfg_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIfCfg_Rollback
    (
        ANSC_HANDLE                 hInsContext
    ) {
    return TRUE;
}

/***********************************************************************

 APIs for Object:

    X_RDK_WanManager.CPEInterface.{i}.Wan.Validaton

    *  WanIfValidation_GetParamBoolValue
    *  WanIfValidation_SetParamBoolValue
    *  WanIfValidation_Validate
    *  WanIfValidation_Commit
    *  WanIfValidation_Rollback
***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfValidation_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfValidation_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    ) {

    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Discovery-Offer", TRUE))
    {
        *pBool = pWanIface->CfgValidationDiscoverOffer;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "Solicit-Advertise", TRUE))
    {
        *pBool = pWanIface->CfgValidationSolicitAdvertise;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "RS-RA", TRUE))
    {
        *pBool = pWanIface->CfgValidationRsRa;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "PADI-PADO", TRUE))
    {
        *pBool = pWanIface->CfgValidationPadiPado;
        return TRUE;
    }
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfValidation_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfValidation_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;
    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "Discovery-Offer", TRUE))
    {
        pWanIface->CfgValidationDiscoverOffer = bValue;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "Solicit-Advertise", TRUE))
    {
        pWanIface->CfgValidationSolicitAdvertise = bValue;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "RS-RA", TRUE))
    {
        pWanIface->CfgValidationRsRa = bValue;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "PADI-PADO", TRUE))
    {
        pWanIface->CfgValidationPadiPado = bValue;
        return TRUE;
    }
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfValidation_Validate
        (
            ANSC_HANDLE                 hInsContext,
            char*                       pReturnParamName,
            ULONG*                      puLength
        )
    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfValidation_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    ) {
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIfValidation_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIfValidation_Commit
    (
        ANSC_HANDLE                 hInsContext
    ) {

    ANSC_STATUS result = ANSC_STATUS_SUCCESS;
    PDML_WAN_IFACE pWanIface = (PDML_WAN_IFACE)hInsContext;

    result = DmlSetWanIfValidationCfg(pWanIface->ulInstanceNumber, pWanIface);
    if (result != ANSC_STATUS_SUCCESS)
    {
        AnscTraceError(("%s: Failed \n", __FUNCTION__));
    }
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIfValidation_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIfValidation_Rollback
    (
        ANSC_HANDLE                 hInsContext
    ) {
    return TRUE;
}

/***********************************************************************

 APIs for Object:

    X_RDK_WanManager.CPEInterface.{i}.Phy.

    *  WanIfPhy_GetParamStringValue
    *  WanIfPhy_SetParamStringValue
    *  WanIfPhy_GetParamUlongValue
    *  WanIfPhy_SetParamUlongValue
    *  WanIfPhy_Validate
    *  WanIfPhy_Commit
    *  WanIfPhy_Rollback

***********************************************************************/


ULONG
WanIfPhy_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    ) {

    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;
    
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Path", TRUE) )
    {
       /* collect value */
       if ( ( sizeof( pWanIface->PhyPath ) - 1 ) < *pUlSize )
       {
           AnscCopyString( pValue, pWanIface->PhyPath );
           return 0;
       }
       else
       {
           *pUlSize = sizeof( pWanIface->PhyPath );
           return 1;
       }
    }
    
    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;

}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        WanIfPhy_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/

BOOL
WanIfPhy_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and set the corresponding value */    
    if( AnscEqualString(ParamName, "Path", TRUE))
    {
        AnscCopyString(pWanIface->PhyPath, pString);
        DmlWanIfSetCfgPhyPath(pWanIface->ulInstanceNumber - 1, pWanIface->PhyPath);
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfPhy_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/

BOOL
WanIfPhy_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Status", TRUE))
    {
        *puLong = pWanIface->PhyStatus;
        return TRUE;
    }
	
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfPhy_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/

BOOL
WanIfPhy_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "Status", TRUE))
    {
        pWanIface->PhyStatus = uValue;
        DmlWanIfSetCfgPhyStatus(pWanIface->ulInstanceNumber - 1, pWanIface->PhyStatus);
        return TRUE;
    }
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfPhy_Validate
    	(
        	ANSC_HANDLE                 hInsContext,
        	char*                       pReturnParamName,
        	ULONG*                      puLength
    	)
    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfPhy_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    ) {
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIfPhy_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIfPhy_Commit
    (
        ANSC_HANDLE                 hInsContext
    ) {
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIfPhy_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIfPhy_Rollback
    (
        ANSC_HANDLE                 hInsContext
    ) {
    return TRUE;
}

/***********************************************************************

 APIs for Object:

    X_RDK_WanManager.CPEInterface.{i}.DynamicTrigger.

    *  WanIfDynTrigger_GetParamUlongValue
    *  WanIfDynTrigger_GetParamBoolValue
    *  WanIfDynTrigger_SetParamUlongValue
    *  WanIfDynTrigger_SetParamBoolValue
    *  WanIfDynTrigger_Validate
    *  WanIfDynTrigger_Commit
    *  WanIfDynTrigger_Rollback

***********************************************************************/

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfDynTrigger_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfDynTrigger_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and return the corresponding value */

    if( AnscEqualString(ParamName, "Delay", TRUE))
    {
        *puLong = pWanIface->DynTriggerDelay;
        return TRUE;
    }
	
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;

}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfDynTrigger_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfDynTrigger_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "Delay", TRUE))
    {
        pWanIface->DynTriggerDelay = uValue;
        return TRUE;
    }
	
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfDynTrigger_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfDynTrigger_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Enable", TRUE))
    {
        *pBool = pWanIface->DynTriggerEnable;
        return TRUE;
    }
	
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfDynTrigger_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfDynTrigger_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;
    BOOL  bridgeMode;
	
    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "Enable", TRUE))
    {
        /* save update to backup */
        pWanIface->DynTriggerEnable     = bValue;

        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfDynTrigger_Validate
    	(
        	ANSC_HANDLE                 hInsContext,
        	char*                       pReturnParamName,
        	ULONG*                      puLength
    	)
    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfDynTrigger_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    ) {
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIfDynTrigger_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIfDynTrigger_Commit
    (
        ANSC_HANDLE                 hInsContext
    ) {
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIfDynTrigger_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIfDynTrigger_Rollback
    (
        ANSC_HANDLE                 hInsContext
    ) {
    return TRUE;
}

/***********************************************************************

 APIs for Object:

    X_RDK_WanManager.CPEInterface.{i}.IP.

    *  WanIfIpCfg_GetParamUlongValue
    *  WanIfIpCfg_SetParamUlongValue
    *  WanIfIpCfg_GetParamStringValue
    *  WanIfIpCfg_SetParamStringValue
    *  WanIfIpCfg_Validate
    *  WanIfIpCfg_Commit
    *  WanIfIpCfg_Rollback

***********************************************************************/

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfIpCfg_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfIpCfg_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "IPv4Status", TRUE))
    {
        *puLong = pWanIface->Ipv4State;
        return TRUE;
    }	
    if( AnscEqualString(ParamName, "IPv6Status", TRUE))
    {
        *puLong = pWanIface->Ipv6State;
        return TRUE;
    }
	
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfIpCfg_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfIpCfg_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "IPv4Status", TRUE))
    {
        pWanIface->Ipv4State = uValue;
        return TRUE;
    }	
    if( AnscEqualString(ParamName, "IPv6Status", TRUE))
    {
        pWanIface->Ipv6State = uValue;
        return TRUE;
    }
	
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        WanIfIpCfg_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
WanIfIpCfg_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    ) {    
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;
    
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Path", TRUE) )
    {
       /* collect value */
       if ( ( sizeof( pWanIface->IpPath ) - 1 ) < *pUlSize )
       {
           AnscCopyString( pValue, pWanIface->IpPath );
           return 0;
       }
       else
       {
           *pUlSize = sizeof( pWanIface->IpPath );
           return 1;
       }
    }
    
    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;

}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        WanIfIpCfg_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfIpCfg_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;
    
    /* check the parameter name and set the corresponding value */    
    if( AnscEqualString(ParamName, "Path", TRUE))
    {
        AnscCopyString(pWanIface->IpPath, pString);
        return TRUE;
    }
	
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfIpCfg_Validate
    	(
        	ANSC_HANDLE                 hInsContext,
        	char*                       pReturnParamName,
        	ULONG*                      puLength
    	)
    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfIpCfg_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    ) {
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIfIpCfg_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIfIpCfg_Commit
    (
        ANSC_HANDLE                 hInsContext
    ) {
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIfIpCfg_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIfIpCfg_Rollback
    (
        ANSC_HANDLE                 hInsContext
    ) {
    return TRUE;
}

/***********************************************************************

 APIs for Object:

    X_RDK_WanManager.CPEInterface.{i}.MAPT.

    *  WanIfMapt_GetParamUlongValue
    *  WanIfMapt_SetParamUlongValue
    *  WanIfMapt_GetParamStringValue
    *  WanIfMapt_SetParamStringValue
    *  WanIfMapt_Validate
    *  WanIfMapt_Commit
    *  WanIfMapt_Rollback

***********************************************************************/

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfMapt_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfMapt_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and return the corresponding value */

    if( AnscEqualString(ParamName, "MAPTStatus", TRUE))
    {
        *puLong = pWanIface->MaptState;
        return TRUE;
    }
	
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfMapt_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfMapt_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "MAPTStatus", TRUE))
    {
#ifdef FEATURE_MAPT
        pWanIface->MaptState = uValue;
        return TRUE;
#endif /* * FEATURE_MAPT */
    }
	
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        WanIfMapt_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
WanIfMapt_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;
    
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Path", TRUE) )
    {
       /* collect value */
       if ( ( sizeof( pWanIface->MaptPath ) - 1 ) < *pUlSize )
       {
           AnscCopyString( pValue, pWanIface->MaptPath );
           return 0;
       }
       else
       {
           *pUlSize = sizeof( pWanIface->MaptPath );
           return 1;
       }
    }
    
    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;

}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        WanIfMapt_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfMapt_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;
    
    /* check the parameter name and set the corresponding value */    
    if( AnscEqualString(ParamName, "Path", TRUE))
    {
#ifdef FEATURE_MAPT
        AnscCopyString(pWanIface->MaptPath, pString);
        return TRUE;
#endif /* * FEATURE_MAPT */
    }
	
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfMapt_Validate
    	(
        	ANSC_HANDLE                 hInsContext,
        	char*                       pReturnParamName,
        	ULONG*                      puLength
    	)
    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfMapt_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    ) {
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIfMapt_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIfMapt_Commit
    (
        ANSC_HANDLE                 hInsContext
    ) {
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIfMapt_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIfMapt_Rollback
    (
        ANSC_HANDLE                 hInsContext
    ) {
    return TRUE;
}

/***********************************************************************

 APIs for Object:

    X_RDK_WanManager.CPEInterface.{i}.MAPT.

    *  WanIfDSLite_GetParamUlongValue
    *  WanIfDSLite_SetParamUlongValue
    *  WanIfDSLite_GetParamStringValue
    *  WanIfDSLite_SetParamStringValue
    *  WanIfDSLite_Validate
    *  WanIfDSLite_Commit
    *  WanIfDSLite_Rollback

***********************************************************************/

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfDSLite_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfDSLite_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and return the corresponding value */

    if( AnscEqualString(ParamName, "Status", TRUE))
    {
        *puLong = pWanIface->DsliteState;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfDSLite_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfDSLite_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "Status", TRUE))
    {
        pWanIface->DsliteState = uValue;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        WanIfDSLite_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
WanIfDSLite_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Path", TRUE) )
    {
       /* collect value */
       if ( ( sizeof( pWanIface->DslitePath ) - 1 ) < *pUlSize )
       {
           AnscCopyString( pValue, pWanIface->DslitePath );
           return 0;
       }
       else
       {
           *pUlSize = sizeof( pWanIface->DslitePath );
           return 1;
       }
    }

    /* AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;

}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfDSLite_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfDSLite_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    ) {
    PDML_WAN_IFACE  pWanIface  =  (PDML_WAN_IFACE)hInsContext;

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "Path", TRUE))
    {
        AnscCopyString(pWanIface->DslitePath, pString);
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfDSLite_Validate
        (
            ANSC_HANDLE                 hInsContext,
            char*                       pReturnParamName,
            ULONG*                      puLength
        )
    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
WanIfDSLite_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    ) {
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIfDSLite_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIfDSLite_Commit
    (
        ANSC_HANDLE                 hInsContext
    ) {
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIfDSLite_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG
WanIfDSLite_Rollback
    (
        ANSC_HANDLE                 hInsContext
    ) {
    return TRUE;
}

/***********************************************************************

 APIs for Object:

    Device.X_RDK_WanManager.CPEInterface.{I}.Marking.{i}.

    *  Marking_GetEntryCount
    *  Marking_GetEntry
    *  Marking_AddEntry
    *  Marking_DelEntry
    *  Marking_GetParamUlongValue
    *  Marking_GetParamStringValue
    *  Marking_GetParamIntValue
    *  Marking_SetParamUlongValue
    *  Marking_SetParamStringValue
    *  Marking_SetParamIntValue
    *  Marking_Validate
    *  Marking_Commit
    *  Marking_Rollback

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        Marking_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/

ULONG
Marking_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
#ifdef FEATURE_802_1P_COS_MARKING
    PDML_WAN_IFACE          pWanIface  =  (PDML_WAN_IFACE)hInsContext;
    PDATAMODEL_MARKING      pMarking   =  (PDATAMODEL_MARKING) &(pWanIface->stDataModelMarking);

    return AnscSListQueryDepth( &pMarking->MarkingList );
#else
    return 0;
#endif /* * FEATURE_802_1P_COS_MARKING */
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_HANDLE
        Marking_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/

ANSC_HANDLE
Marking_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
    PDML_WAN_IFACE                     pWanIface  =  (PDML_WAN_IFACE)hInsContext;
    PDATAMODEL_MARKING                 pMarking   =  (PDATAMODEL_MARKING) &(pWanIface->stDataModelMarking);
    PSINGLE_LINK_ENTRY                      pSListEntry       = NULL;
    PCONTEXT_MARKING_LINK_OBJECT       pCxtLink          = NULL;

    pSListEntry       = AnscSListGetEntryByIndex(&(pMarking->MarkingList), nIndex);

    if ( pSListEntry )
    {
        pCxtLink      = ACCESS_CONTEXT_MARKING_LINK_OBJECT(pSListEntry);
        *pInsNumber   = pCxtLink->InstanceNumber;
    }

    return (ANSC_HANDLE)pSListEntry;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_HANDLE
        Marking_AddEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to add a new entry.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle of new added entry.

**********************************************************************/

ANSC_HANDLE
Marking_AddEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG*                      pInsNumber
    )
{
#ifdef FEATURE_802_1P_COS_MARKING
    ANSC_STATUS                             returnStatus      = ANSC_STATUS_SUCCESS;
    PDML_WAN_IFACE                     pWanIface  =  (PDML_WAN_IFACE)hInsContext;
    PDATAMODEL_MARKING                 pMarking   =  (PDATAMODEL_MARKING) &(pWanIface->stDataModelMarking);
    PDML_MARKING                       p_Marking         = NULL;
    PCONTEXT_MARKING_LINK_OBJECT       pMarkingCxtLink   = NULL;

    //Verify limit of the marking table
    if( WAN_IF_MARKING_MAX_LIMIT < pMarking->ulNextInstanceNumber )
    {
        CcspTraceError(("%s %d - Failed to add Marking entry due to maximum limit(%d)\n",__FUNCTION__,__LINE__,WAN_IF_MARKING_MAX_LIMIT));
        return NULL;
    }

    p_Marking = (PDML_MARKING)AnscAllocateMemory(sizeof(DML_MARKING));

    if ( !p_Marking )
    {
        return NULL;
    }

    pMarkingCxtLink = (PCONTEXT_MARKING_LINK_OBJECT)AnscAllocateMemory(sizeof(CONTEXT_MARKING_LINK_OBJECT));
    if ( !pMarkingCxtLink )
    {
        goto EXIT;
    }

    /* now we have this link content */
    pMarkingCxtLink->hContext = (ANSC_HANDLE)p_Marking;
    pMarkingCxtLink->bNew     = TRUE;

    pMarkingCxtLink->InstanceNumber  = pMarking->ulNextInstanceNumber;
    *pInsNumber                      = pMarking->ulNextInstanceNumber;

    //Assign actual instance number
    p_Marking->InstanceNumber = pMarking->ulNextInstanceNumber;

    pMarking->ulNextInstanceNumber++;

    //Assign WAN interface instance for reference
    p_Marking->ulWANIfInstanceNumber = pWanIface->ulInstanceNumber;
    
    //Initialise all marking members
    memset( p_Marking->Alias, 0, sizeof( p_Marking->Alias ) );
    p_Marking->SKBPort = 0;
    p_Marking->SKBMark = 0;
    p_Marking->EthernetPriorityMark = -1;

    SListPushMarkingEntryByInsNum(&pMarking->MarkingList, (PCONTEXT_LINK_OBJECT)pMarkingCxtLink);
   
    return (ANSC_HANDLE)pMarkingCxtLink;

EXIT:
    if( NULL != pMarkingCxtLink )
    {
      AnscFreeMemory(pMarkingCxtLink);
      pMarkingCxtLink = NULL;
    }

    if( NULL != p_Marking )
    {
      AnscFreeMemory(p_Marking);
      p_Marking = NULL;
    }

    return NULL;

#else

    return NULL;

#endif /* * FEATURE_802_1P_COS_MARKING */
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        Marking_DelEntry
            (
                ANSC_HANDLE                 hInsContext,
                ANSC_HANDLE                 hInstance
            );

    description:

        This function is called to delete an exist entry.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ANSC_HANDLE                 hInstance
                The exist entry handle;

    return:     The status of the operation.

**********************************************************************/

ULONG
Marking_DelEntry
    (
        ANSC_HANDLE                 hInsContext,
        ANSC_HANDLE                 hInstance
    )
{
#ifdef FEATURE_802_1P_COS_MARKING
    ANSC_STATUS                             returnStatus   = ANSC_STATUS_SUCCESS;
    PDML_WAN_IFACE                     pWanIface  =  (PDML_WAN_IFACE)hInsContext;
    PDATAMODEL_MARKING                 pMarking   =  (PDATAMODEL_MARKING) &(pWanIface->stDataModelMarking);
    PCONTEXT_MARKING_LINK_OBJECT       pMarkingCxtLink   = (PCONTEXT_MARKING_LINK_OBJECT)hInstance;
    PDML_MARKING                       p_Marking         = (PDML_MARKING)pMarkingCxtLink->hContext;

    if ( pMarkingCxtLink->bNew )
    {
        /* Set bNew to FALSE to indicate this node is not going to save to SysRegistry */
        pMarkingCxtLink->bNew = FALSE;
    }

    returnStatus = DmlDeleteMarking( NULL, p_Marking );

    if ( ( ANSC_STATUS_SUCCESS == returnStatus ) && \
         ( AnscSListPopEntryByLink(&pMarking->MarkingList, &pMarkingCxtLink->Linkage) ) )
    {
        AnscFreeMemory(pMarkingCxtLink->hContext);
        AnscFreeMemory(pMarkingCxtLink);
    }

    return returnStatus;
#else
    return ANSC_STATUS_FAILURE;
#endif /* * FEATURE_802_1P_COS_MARKING */
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        Marking_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/

BOOL
Marking_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    PCONTEXT_MARKING_LINK_OBJECT        pCxtLink      = (PCONTEXT_MARKING_LINK_OBJECT)hInsContext;
    PDML_MARKING                        p_Marking     = (PDML_MARKING )pCxtLink->hContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "SKBPort", TRUE))
    {
        *puLong = p_Marking->SKBPort;
        return TRUE;
    }
    if( AnscEqualString(ParamName, "SKBMark", TRUE))
    {
        *puLong = p_Marking->SKBMark;
        return TRUE;
    }
    
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        Marking_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/

ULONG
Marking_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PCONTEXT_MARKING_LINK_OBJECT        pCxtLink      = (PCONTEXT_MARKING_LINK_OBJECT)hInsContext;
    PDML_MARKING                        p_Marking     = (PDML_MARKING )pCxtLink->hContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Alias", TRUE))
    {
        if ( AnscSizeOfString(p_Marking->Alias) < *pUlSize)
        {
            AnscCopyString(pValue, p_Marking->Alias);
            return 0;
        }
        else
        {
            *pUlSize = AnscSizeOfString(p_Marking->Alias)+1;
            return 1;
        }
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        Marking_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Marking_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    ) 
{
    PCONTEXT_MARKING_LINK_OBJECT        pCxtLink      = (PCONTEXT_MARKING_LINK_OBJECT)hInsContext;
    PDML_MARKING                        p_Marking     = (PDML_MARKING )pCxtLink->hContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "EthernetPriorityMark", TRUE))
    {
        *pInt = p_Marking->EthernetPriorityMark;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        Marking_SetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int                         iValue
            );

    description:

        This function is called to set integer parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int                         iValue
                The updated integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Marking_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    ) 
{
    PCONTEXT_MARKING_LINK_OBJECT        pCxtLink      = (PCONTEXT_MARKING_LINK_OBJECT)hInsContext;
    PDML_MARKING                        p_Marking     = (PDML_MARKING )pCxtLink->hContext;

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "EthernetPriorityMark", TRUE))
    {
        p_Marking->EthernetPriorityMark = iValue;
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        Marking_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/

BOOL
Marking_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    PCONTEXT_MARKING_LINK_OBJECT        pCxtLink      = (PCONTEXT_MARKING_LINK_OBJECT)hInsContext;
    PDML_MARKING                        p_Marking     = (PDML_MARKING )pCxtLink->hContext;

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        Marking_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/

BOOL
Marking_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    PCONTEXT_MARKING_LINK_OBJECT        pCxtLink      = (PCONTEXT_MARKING_LINK_OBJECT)hInsContext;
    PDML_MARKING                        p_Marking     = (PDML_MARKING )pCxtLink->hContext;

    /* check the parameter name and set the corresponding value */
   
    if( AnscEqualString(ParamName, "Alias", TRUE))
    {
        //Alias should not overwrite after set
        if( 0 < AnscSizeOfString(p_Marking->Alias) )
        {
            return FALSE;
        }

        AnscCopyString(p_Marking->Alias, pString);
        return TRUE;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        Vlan_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation.

                ULONG*                      puLength
                The output length of the param name.

    return:     TRUE if there's no validation.

**********************************************************************/

BOOL
Marking_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        Marking_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/

ULONG
Marking_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    ANSC_STATUS                              returnStatus  = ANSC_STATUS_SUCCESS;
    PCONTEXT_MARKING_LINK_OBJECT        pCxtLink      = (PCONTEXT_MARKING_LINK_OBJECT)hInsContext;
    PDML_MARKING                        p_Marking     = (PDML_MARKING )pCxtLink->hContext;

    if ( pCxtLink->bNew ) 
    {
        //Add new marking params
        returnStatus = DmlAddMarking( NULL, p_Marking );

        if ( returnStatus == ANSC_STATUS_SUCCESS )
        {
            pCxtLink->bNew = FALSE;
        }
        else
        {
            //Re-init all memory
            memset( p_Marking->Alias, 0, sizeof( p_Marking->Alias ) );
            p_Marking->SKBPort = 0;
            p_Marking->SKBMark = 0;
            p_Marking->EthernetPriorityMark = -1;
        }
    }
    else 
    {
        //Update marking param values
        returnStatus = DmlSetMarking( NULL, p_Marking );
    }

    return returnStatus;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        Marking_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/

ULONG
Marking_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    ANSC_STATUS                              returnStatus  = ANSC_STATUS_SUCCESS;

    return returnStatus;
}
