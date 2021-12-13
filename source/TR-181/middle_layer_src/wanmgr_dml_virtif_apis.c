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

#include "wanmgr_dml_virtif_apis.h"
#include "wanmgr_rdkbus_virtif_apis.h"
#include "wanmgr_net_utils.h"
#include "wanmgr_dhcpv4_apis.h"
#include "wanmgr_dhcpv6_apis.h"
#include "wanmgr_data.h"

/***********************************************************************

 APIs for Object:

    X_RDK_WanManager.CPEInterface.

    *  WanIf_GetEntryCount
    *  WanIf_GetEntry
    *  WanIf_GetParamUlongValue
    *  WanIf_SetParamUlongValue   
    *  WanIf_GetParamStringValue
    *  WanIf_SetParamStringValue
    *  WanIf_GetParamBoolValue
    *  WanIf_SetParamBoolValue
    *  WanIf_Validate
    *  WanIf_Commit
    *  WanIf_Rollback

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG WanIf_GetEntryCount(ANSC_HANDLE hInsContext);

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG WanIf_GetEntryCount(ANSC_HANDLE hInsContext)
{
    ULONG count = 0;
    count = (ULONG)WanMgr_IfaceData_GetTotalWanIface();

    return count;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_HANDLE WanIf_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber);

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
ANSC_HANDLE WanIf_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber)
{
    ANSC_HANDLE pDmlEntry = NULL;

    WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(nIndex);
    if(pWanDmlIfaceData != NULL)
    {
        *pInsNumber = nIndex + 1;
        pDmlEntry = (ANSC_HANDLE) pWanDmlIfaceData;

        WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
    }
    return pDmlEntry;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIf_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);

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
BOOL WanIf_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
{
    BOOL ret = FALSE;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and return the corresponding value */
            if( AnscEqualString(ParamName, "LinkStatus", TRUE))
            {
                *puLong = pWanDmlIface->LinkStatus;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "Status", TRUE))
            {
                *puLong = pWanDmlIface->Status;
                ret = TRUE;
            }
    
            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIf_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);

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
BOOL WanIf_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue)
{
    BOOL ret = FALSE;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            if( AnscEqualString(ParamName, "LinkStatus", TRUE))
            {
                pWanDmlIface->LinkStatus = uValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "Status", TRUE))
            {
                pWanDmlIface->Status = uValue;
                ret = TRUE;
            }
	    
            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG WanIf_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);

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
ULONG WanIf_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize)
{
    ULONG ret = -1;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            ///* check the parameter name and return the corresponding value */
            if( AnscEqualString(ParamName, "Name", TRUE) )
            {
               /* collect value */
               if ( ( sizeof( pWanDmlIface->Name ) - 1 ) < *pUlSize )
               {
                   AnscCopyString( pValue, pWanDmlIface->Name );
                   ret = 0;
               }
               else
               {
                   *pUlSize = sizeof( pWanDmlIface->Name );
                   ret = 1;
               }
            }
            else if( AnscEqualString(ParamName, "BaseInterface", TRUE) )
            {
               /* collect value */
               if ( ( sizeof( pWanDmlIface->BaseInterface ) - 1 ) < *pUlSize )
               {
                   AnscCopyString( pValue, pWanDmlIface->BaseInterface );
                   ret = 0;
               }
               else
               {
                   *pUlSize = sizeof( pWanDmlIface->BaseInterface );
                   ret = 1;
               }
            }

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIf_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);

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
BOOL WanIf_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
{
    BOOL ret = FALSE;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            if( AnscEqualString(ParamName, "Name", TRUE))
            {
		if (AnscSizeOfString(pWanDmlIface->Name) > 0)
		{
		    if (AnscEqualString(pWanDmlIface->Name, pString, TRUE))
		    {
                        return(FALSE);
		    }
                    memset(pWanDmlIface->Name, 0, sizeof(pWanDmlIface->Name));
		}

                if (AnscSizeOfString(pString) < sizeof(pWanDmlIface->Name))
                {
                    AnscCopyString(pWanDmlIface->Name, pString);
                    return(TRUE);
		}
		return(FALSE);
            }

            if( AnscEqualString(ParamName, "BaseInterface", TRUE))
            {
                if (AnscSizeOfString(pWanDmlIface->BaseInterface) > 0)
                {
                    if (AnscEqualString(pWanDmlIface->BaseInterface, pString, TRUE))
                    {
                        return(FALSE);
                    }
                    memset(pWanDmlIface->BaseInterface, 0, sizeof(pWanDmlIface->BaseInterface));
                }

                if (AnscSizeOfString(pString) < sizeof(pWanDmlIface->BaseInterface))
                {
                    AnscCopyString(pWanDmlIface->BaseInterface, pString);
                    return(TRUE);
		}
		return(FALSE);
            }

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIf_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool);

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
BOOL WanIf_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool)
{
    BOOL ret = FALSE;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            //* check the parameter name and return the corresponding value */

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIf_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue);

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
BOOL WanIf_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue)
{
    BOOL ret = FALSE;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return ret;
}



/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIf_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);

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

BOOL WanIf_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIf_Commit(ANSC_HANDLE hInsContext);

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIf_Commit(ANSC_HANDLE hInsContext)
{
    return 0;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIf_Rollback(ANSC_HANDLE hInsContext);

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIf_Rollback(ANSC_HANDLE hInsContext)
{
    return 0;
}

/***********************************************************************

 APIs for Object:

    X_RDK_WanManager.CPEInterface.{i}.Selection.

    *  WanIfSelection_GetParamUlongValue
    *  WanIfSelection_SetParamUlongValue
    *  WanIfSelection_GetParamBoolValue
    *  WanIfSelection_SetParamBoolValue
    *  WanIfSelection_GetParamIntValue
    *  WanIfSelection_SetParamIntValue
    *  WanIfSelection_Validate
    *  WanIfSelection_Commit
    *  WanIfSelection_Rollback

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfSelection_GetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int* pInt);

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
BOOL WanIfSelection_GetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int* pInt)
{
    BOOL ret = FALSE;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            if( AnscEqualString(ParamName, "Priority", TRUE))
            {
                *pInt = pWanDmlIface->Selection.Priority;
                ret = TRUE;
            }

            if( AnscEqualString(ParamName, "TimeOut", TRUE))
            {
                *pInt = pWanDmlIface->Selection.TimeOut;
                ret = TRUE;
            }

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfSelection_SetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int iValue);

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
BOOL WanIfSelection_SetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int iValue)
{
    BOOL ret = FALSE;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);
            /* check the parameter name and set the corresponding value */
            if( AnscEqualString(ParamName, "Priority", TRUE))
            {
                pWanDmlIface->Selection.Priority = iValue;
		ret = TRUE;
            }

            if( AnscEqualString(ParamName, "TimeOut", TRUE))
            {
                pWanDmlIface->Selection.TimeOut = iValue;
		ret = TRUE;
            }

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfSelection_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);

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
BOOL WanIfSelection_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
{
    BOOL ret = FALSE;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and return the corresponding value */
            if( AnscEqualString(ParamName, "Status", TRUE))
            {
                *puLong = pWanDmlIface->Selection.Status;
                ret = TRUE;
            }

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfSelection_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);

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
BOOL WanIfSelection_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue)
{
    BOOL ret = FALSE;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            if( AnscEqualString(ParamName, "Status", TRUE))
            {
                pWanDmlIface->Selection.Status = uValue;
                ret = TRUE;
            }

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfSelection_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool);

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
BOOL WanIfSelection_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool)
{
    BOOL ret = FALSE;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            //* check the parameter name and return the corresponding value */
            if( AnscEqualString(ParamName, "Enable", TRUE))
            {
                *pBool = pWanDmlIface->Selection.Enable;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "RequireReboot", TRUE))
            {
                *pBool = pWanDmlIface->Selection.RequireReboot;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "ActiveLink", TRUE))
            {
                *pBool = pWanDmlIface->Selection.ActiveLink;
                ret = TRUE;
            }

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfSelection_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue);

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
BOOL WanIfSelection_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue)
{
    BOOL ret = FALSE;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            if( AnscEqualString(ParamName, "Enable", TRUE))
            {
                pWanDmlIface->Selection.Enable  = bValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "RequireReboot", TRUE))
            {
                pWanDmlIface->Selection.RequireReboot = bValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "ActiveLink", TRUE))
            {
                pWanDmlIface->Selection.ActiveLink = bValue;
                ret = TRUE;
            }

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfSelection_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);

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
BOOL WanIfSelection_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIfSelection_Commit(ANSC_HANDLE hInsContext);

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfSelection_Commit(ANSC_HANDLE hInsContext)
{
    ULONG ret = -1;
    ANSC_STATUS result;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            result = WanManager_SetWanIfCfg( pWanDmlIface->uiInstanceNumber, pWanDmlIface );
            if(result != ANSC_STATUS_SUCCESS)
            {
                AnscTraceError(("%s: Failed \n", __FUNCTION__));
            }
            else
            {
                ret = 0;
            }

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }
    return ret;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIfSelection_Rollback(ANSC_HANDLE hInsContext);

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfSelection_Rollback(ANSC_HANDLE hInsContext)
{
    return TRUE;
}



/*********************************************************************** 
  
 APIs for Object: 
 
   X_RDK_WanManager.CPEInterface.{i}.VirtualInterface.{i}.
 
    *  WanIfVirtIf_GetEntryCount 
    *  WanIfVirtIf_GetEntry 
    *  WanIfVirtIf_AddEntry 
    *  WanIfVirtIf_DelEntry 
    *  WanIfVirtIf_GetParamUlongValue 
    *  WanIfVirtIf_SetParamUlongValue 
    *  WanIfVirtIf_GetParamBoolValue 
    *  WanIfVirtIf_SetParamBoolValue 
    *  WanIfVirtIf_GetParamStringValue
    *  WanIfVirtIf_SetParamStringValue
    *  WanIfVirtIf_Validate 
    *  WanIfVirtIf_Commit 
    *  WanIfVirtIf_Rollback 
  
***********************************************************************/ 

/********************************************************************** 
 
    caller:     owner of this object 
 
    prototype: 
 
        ULONG WanIfVirtIf_GetEntryCount(ANSC_HANDLE hInsContext); 
  
     description: 
 
        This function is called to retrieve the count of the table. 
  
    argument:   ANSC_HANDLE                 hInsContext, 
                The instance handle; 
 
    return:     The count of the table 
 
**********************************************************************/ 
 
ULONG WanIfVirtIf_GetEntryCount(ANSC_HANDLE hInsContext) 
{ 
    ULONG count = 0;
    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            count = AnscSListQueryDepth( &(pWanDmlIface->VirtIf.VirtIfList) );

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return count;
} 


/********************************************************************** 
 
    caller:     owner of this object 
 
    prototype: 
 
        ANSC_HANDLE WanIfVirtIf_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber); 
 
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
ANSC_HANDLE WanIfVirtIf_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber) 
{ 
    PSINGLE_LINK_ENTRY  pSListEntry = NULL;
    *pInsNumber= 0;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            pSListEntry       = AnscSListGetEntryByIndex(&(pWanDmlIface->VirtIf.VirtIfList), nIndex);
            if ( pSListEntry )
            {
                CONTEXT_VIRTIF_LINK_OBJECT* pCxtLink      = ACCESS_CONTEXT_VIRTIF_LINK_OBJECT(pSListEntry);
                *pInsNumber   = pCxtLink->InstanceNumber;
            }


            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return (ANSC_HANDLE)pSListEntry;
} 
 
/********************************************************************** 
 
    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE WanIfVirtIf_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber); 

    description: 

       This function is called to add a new entry. 
 
    argument:   ANSC_HANDLE                 hInsContext, 
                The instance handle; 
 
                ULONG*                      pInsNumber 
                The output instance number; 
  
    return:     The handle of new added entry. 
 
**********************************************************************/ 
 
ANSC_HANDLE WanIfVirtIf_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber) 
{ 
    ANSC_HANDLE newMarking = NULL;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            newMarking = WanManager_Create_VirtIf_Entry(pWanDmlIface, pInsNumber);

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return newMarking;
} 
 
/********************************************************************** 
 
    caller:     owner of this object 
 
    prototype: 
 
       ULONG WanIfVirtIf_DelEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance); 
 
    description: 

        This function is called to delete an exist entry. 
 
    argument:   ANSC_HANDLE                 hInsContext, 
                The instance handle; 
 
                ANSC_HANDLE                 hInstance 
                The exist entry handle; 
 
    return:     The status of the operation. 
 
**********************************************************************/ 
ULONG WanIfVirtIf_DelEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance) 
{ 
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInstance;
    ULONG returnStatus = -1;

    if(pVirtIfCxtLink)
    {
        WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
        if(pIfaceDmlEntry != NULL)
        {
            WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
            if(pWanDmlIfaceData != NULL)
            {
                DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

                /* check the parameter name and set the corresponding value */
                returnStatus = WanManager_Remove_VirtIf_Entry(pWanDmlIface, pVirtIfCxtLink);

                WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
            }
        }
    }

    return returnStatus;
} 

/********************************************************************** 
 
    caller:     owner of this object 
 
    prototype: 
 
        BOOL WanIfVirtIf_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong); 
 
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
BOOL WanIfVirtIf_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong) 
{ 
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and return the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;

        if( AnscEqualString(ParamName, "TimeOut", TRUE))
        {
            *puLong = p_VirtIf->TimeOut;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "WanProtocol", TRUE))
        {
            *puLong = p_VirtIf->WanProtocol;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "Status", TRUE))
        {
            *puLong = p_VirtIf->Status;
            ret = TRUE;
        }
	
    }

    return ret;
} 
 
/********************************************************************** 

   caller:     owner of this object 

   prototype: 

       BOOL WanIfVirtIf_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue); 
 
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
BOOL WanIfVirtIf_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue) 
{ 
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and set the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;
    
        if( AnscEqualString(ParamName, "TimeOut", TRUE))
        {
            p_VirtIf->TimeOut = uValue;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "WanProtocol", TRUE))
        {
            p_VirtIf->WanProtocol = uValue;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "Status", TRUE))
        {
            p_VirtIf->Status = uValue;
            ret = TRUE;
        }

    }

    return ret;
} 
 
/********************************************************************** 
 
    caller:     owner of this object 
 
    prototype: 
 
        BOOL WanIfVirtIf_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool); 
 
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
BOOL WanIfVirtIf_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool) 
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and return the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;

        if( AnscEqualString(ParamName, "Enable", TRUE))
        {
            *pBool = p_VirtIf->Enable;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "Refresh", TRUE))
        {
            *pBool = p_VirtIf->Refresh;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "VLANInUse", TRUE))
        {
            *pBool = p_VirtIf->VLANInUse;
            ret = TRUE;
        }
    }

    return ret;
} 
 
/********************************************************************** 
 
    caller:     owner of this object 
 
    prototype: 
 
       BOOL WanIfVirtIf_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue); 
 
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
BOOL WanIfVirtIf_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue) 
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and set the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;

        if( AnscEqualString(ParamName, "Enable", TRUE))
        {
            p_VirtIf->Enable = bValue;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "Refresh", TRUE))
        {
            p_VirtIf->Refresh = bValue;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "VLANInUse", TRUE))
        {
            p_VirtIf->VLANInUse = bValue;
            ret = TRUE;
        }
    }
    
    return ret;
} 
 
/********************************************************************** 
 
    caller:     owner of this object 
 
    prototype: 
 
        ULONG WanIfVirtIf_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize); 
 
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
ULONG WanIfVirtIf_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize) 
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;


    /* check the parameter name and return the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf  = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;

        if( AnscEqualString(ParamName, "Name", TRUE))
        {
            if ( AnscSizeOfString(p_VirtIf->Name) < *pUlSize)
            {
                AnscCopyString(pValue, p_VirtIf->Name);
                return 0;
            }
            else
            {
                *pUlSize = AnscSizeOfString(p_VirtIf->Name)+1;
                return 1;
            }
        }

        if( AnscEqualString(ParamName, "Alias", TRUE))
        {
            if ( AnscSizeOfString(p_VirtIf->Alias) < *pUlSize)
            {
                AnscCopyString(pValue, p_VirtIf->Alias);
                return 0;
            }
            else
            {
                *pUlSize = AnscSizeOfString(p_VirtIf->Alias)+1;
                return 1;
            }
        }

        if( AnscEqualString(ParamName, "IPInterface", TRUE))
        {
            if ( AnscSizeOfString(p_VirtIf->IPInterface) < *pUlSize)
            {
                AnscCopyString(pValue, p_VirtIf->IPInterface);
                return 0;
            }
            else
            {
                *pUlSize = AnscSizeOfString(p_VirtIf->IPInterface)+1;
                return 1;
            }
        }

        if( AnscEqualString(ParamName, "PPPInterface", TRUE))
        {
            if ( AnscSizeOfString(p_VirtIf->PPPInterface) < *pUlSize)
            {
                AnscCopyString(pValue, p_VirtIf->PPPInterface);
                return 0;
            }
            else
            {
                *pUlSize = AnscSizeOfString(p_VirtIf->PPPInterface)+1;
                return 1;
            }
        }
    }
    
    return -1;
} 
 
/********************************************************************** 
 
    caller:     owner of this object 
 
    prototype: 
 
        BOOL WanIfVirtIf_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString); 

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
BOOL WanIfVirtIf_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString) 
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and set the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;

        if( AnscEqualString(ParamName, "Name", TRUE))
        {
            if (AnscSizeOfString(p_VirtIf->Name) > 0)
            {
                if (AnscEqualString(p_VirtIf->Name, pString, TRUE))
                {
                    return(FALSE);
                }
                memset(p_VirtIf->Name, 0, sizeof(p_VirtIf->Name));
            }

            if (AnscSizeOfString(pString) < sizeof(p_VirtIf->Name))
            {
                AnscCopyString(p_VirtIf->Name, pString);
                return(TRUE);
	    }
	    return(FALSE);
        }

        if( AnscEqualString(ParamName, "Alias", TRUE))
        {
            if (AnscSizeOfString(p_VirtIf->Alias) > 0)
            {
                if (AnscEqualString(p_VirtIf->Alias, pString, TRUE))
                {
                    return(FALSE);
                }
                memset(p_VirtIf->Alias, 0, sizeof(p_VirtIf->Alias));
            }

            if (AnscSizeOfString(pString) < sizeof(p_VirtIf->Alias))
            {
                AnscCopyString(p_VirtIf->Alias, pString);
                return(TRUE);
	    }
	    return(FALSE);
        }

        if( AnscEqualString(ParamName, "IPInterface", TRUE))
        {
            if (AnscSizeOfString(p_VirtIf->IPInterface) > 0)
            {
                if (AnscEqualString(p_VirtIf->IPInterface, pString, TRUE))
                {
                    return(FALSE);
                }
                memset(p_VirtIf->IPInterface, 0, sizeof(p_VirtIf->IPInterface));
            }

            if (AnscSizeOfString(pString) < sizeof(p_VirtIf->IPInterface))
            {
                AnscCopyString(p_VirtIf->IPInterface, pString);
                return(TRUE);
	    }
	    return(FALSE);
        }

        if( AnscEqualString(ParamName, "PPPInterface", TRUE))
        {
            if (AnscSizeOfString(p_VirtIf->PPPInterface) > 0)
            {
                if (AnscEqualString(p_VirtIf->PPPInterface, pString, TRUE))
                {
                    return(FALSE);
                }
                memset(p_VirtIf->PPPInterface, 0, sizeof(p_VirtIf->PPPInterface));
            }

            if (AnscSizeOfString(pString) < sizeof(p_VirtIf->PPPInterface))
            {
                AnscCopyString(p_VirtIf->PPPInterface, pString);
                return(TRUE);
	    }
	    return(FALSE);
        }
    }

    return ret;
} 
 
/********************************************************************** 

   caller:     owner of this object 
 
   prototype: 
 
       BOOL WanIfVirtIf_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength); 
 
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
BOOL WanIfVirtIf_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength) 
{ 
    return TRUE; 
} 

/********************************************************************** 
   caller:     owner of this object 
 
   prototype: 
        ULONG WanIfVirtIf_Commit(ANSC_HANDLE hInsContext); 
 
    description: 
        This function is called to finally commit all the update. 
 
    argument:   ANSC_HANDLE                 hInsContext, 
                The instance handle; 

    return:     The status of the operation. 
**********************************************************************/ 
ULONG WanIfVirtIf_Commit(ANSC_HANDLE hInsContext) 
{ 
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    ULONG returnStatus = 0;

    if (pVirtIfCxtLink)
    {
        returnStatus = WanManager_Update_VirtIf_Entry(pVirtIfCxtLink);
    }
    return returnStatus;
} 
 
/********************************************************************** 
    caller:     owner of this object 
 
    prototype: 
        ULONG WanIfVirtIf_Rollback(ANSC_HANDLE hInsContext); 
 
    description: 
        This function is called to roll back the update whenever there's a 
        validation found. 
 
    argument:   ANSC_HANDLE                 hInsContext, 
                 The instance handle; 
 
    return:     The status of the operation. 
**********************************************************************/ 
ULONG WanIfVirtIf_Rollback(ANSC_HANDLE hInsContext) 
{ 
     return TRUE; 
} 


/*********************************************************************** 
APIs for Object: 
 
    Device.X_RDK_WanManager.CPEInterface.{i}.VirtualInterface.VLAN.{i}. 
 
    *  WanIfVirtIfVLAN_GetEntryCount 
    *  WanIfVirtIfVLAN_GetEntry 
    *  WanIfVirtIfVLAN_AddEntry 
    *  WanIfVirtIfVLAN_DelEntry 
    *  WanIfVirtIfVLAN_GetParamStringValue 
    *  WanIfVirtIfVLAN_SetParamStringValue 
    *  WanIfVirtIfVLAN_Validate 
    *  WanIfVirtIfVLAN_Commit 
    *  WanIfVirtIfVLAN_Rollback 
  
***********************************************************************/ 

/********************************************************************** 
  
    caller:     owner of this object 
 
    prototype: 
 
        ULONG WanIfVirtIfVLAN_GetEntryCount(ANSC_HANDLE hInsContext); 
 
    description: 
  
         This function is called to retrieve the count of the table. 
 
    argument:   ANSC_HANDLE                 hInsContext, 
                The instance handle; 
 
   return:     The count of the table 
  
**********************************************************************/ 
  
ULONG WanIfVirtIfVLAN_GetEntryCount(ANSC_HANDLE hInsContext /*, ANSC_HANDLE hInstance*/) 
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    ULONG VirtIfVlanCount = 0;

    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;
        VirtIfVlanCount = AnscSListQueryDepth( &(p_VirtIf->Vlan.VirtIfVlanList) );
    }
    
    return VirtIfVlanCount;
} 
  
/********************************************************************** 
  
     caller:     owner of this object 
  
     prototype: 
 
        ANSC_HANDLE WanIfVirtIfVLAN_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber); 
  
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
ANSC_HANDLE WanIfVirtIfVLAN_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber) 
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    PSINGLE_LINK_ENTRY pSListEntry = NULL;
    *pInsNumber= 0;

    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;

        pSListEntry = AnscSListGetEntryByIndex(&(p_VirtIf->Vlan.VirtIfVlanList), nIndex);
        if ( pSListEntry )
        {
            CONTEXT_VIRTIF_VLAN_LINK_OBJECT* pVirtIfVlanCxtLink = ACCESS_CONTEXT_VIRTIF_VLAN_LINK_OBJECT(pSListEntry);
            *pInsNumber = pVirtIfVlanCxtLink->InstanceNumber;
        }
    }

    return (ANSC_HANDLE)pSListEntry;
} 
  
/********************************************************************** 
  
     caller:     owner of this object 
  
     prototype: 
 
        ANSC_HANDLE WanIfVirtIfVLAN_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber); 
 
     description: 
  
        This function is called to add a new entry. 
  
     argument:   ANSC_HANDLE                 hInsContext, 
                 The instance handle; 
 
                 ULONG*                      pInsNumber 
                The output instance number; 
  
return:     The handle of new added entry. 
  
**********************************************************************/ 
  
ANSC_HANDLE WanIfVirtIfVLAN_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber) 
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    ANSC_HANDLE pVirtIfVlanCxtLink = NULL;
    *pInsNumber = 0;

    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;
        pVirtIfVlanCxtLink = WanManager_Create_VirtIfVlan_Entry(pVirtIfCxtLink, pInsNumber);
    }
    
    return pVirtIfVlanCxtLink;    
} 
  
/********************************************************************** 
 
    caller:     owner of this object 
 
    prototype: 
 
         ULONG WanIfVirtIfVLAN_DelEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance); 
  
    description: 
 
        This function is called to delete an exist entry. 
 
    argument:   ANSC_HANDLE                 hInsContext, 
                The instance handle; 
 
                ANSC_HANDLE                 hInstance 
                The exist entry handle; 
 
    return:     The status of the operation. 
  
**********************************************************************/ 
ULONG WanIfVirtIfVLAN_DelEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance) 
{ 
    CONTEXT_VIRTIF_VLAN_LINK_OBJECT* pVirtIfVlanCxtLink = (CONTEXT_VIRTIF_VLAN_LINK_OBJECT*)hInstance;
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;

    ULONG returnStatus = -1;

    if(pVirtIfCxtLink && pVirtIfVlanCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;
        returnStatus = WanManager_Remove_VirtIfVlan_Entry(pVirtIfCxtLink, pVirtIfVlanCxtLink);
    }

    return returnStatus;
} 

/**********************************************************************
     caller:     owner of this object

     prototype:

         ULONG WanIfVirtIfVLAN_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);

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
ULONG WanIfVirtIfVLAN_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize)
{
    CONTEXT_VIRTIF_VLAN_LINK_OBJECT* pVirtIfVlanCxtLink = (CONTEXT_VIRTIF_VLAN_LINK_OBJECT*)hInsContext;

    /* check the parameter name and return the corresponding value */
    if (pVirtIfVlanCxtLink)
    {
        DML_VIRTIF_VLAN* p_VirtIfVlan = (DML_VIRTIF_VLAN* )pVirtIfVlanCxtLink->hContext;

        if( AnscEqualString(ParamName, "Interface", TRUE))
        {
            if ( AnscSizeOfString(p_VirtIfVlan->Interface) < *pUlSize)
            {
                AnscCopyString(pValue, p_VirtIfVlan->Interface);
                return 0;
            }
            else
            {
                *pUlSize = AnscSizeOfString(p_VirtIfVlan->Interface)+1;
                return 1;
            }
        }
    }

    return -1;
}

/**********************************************************************
     caller:     owner of this object

     prototype:

         BOOL WanIfVirtIfVLAN_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);

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
BOOL WanIfVirtIfVLAN_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
{
    CONTEXT_VIRTIF_VLAN_LINK_OBJECT* pVirtIfVlanCxtLink = (CONTEXT_VIRTIF_VLAN_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and set the corresponding value */
    if (pVirtIfVlanCxtLink)
    {
        DML_VIRTIF_VLAN* p_VirtIfVlan = (DML_VIRTIF_VLAN* )pVirtIfVlanCxtLink->hContext;

        if( AnscEqualString(ParamName, "Interface", TRUE))
        {
            if (AnscSizeOfString(p_VirtIfVlan->Interface) > 0)
            {
                if (AnscEqualString(p_VirtIfVlan->Interface, pString, TRUE))
                {
                    return(FALSE);
                }
                memset(p_VirtIfVlan->Interface, 0, sizeof(p_VirtIfVlan->Interface));
            }

            if (AnscSizeOfString(pString) < sizeof(p_VirtIfVlan->Interface))
            {
                AnscCopyString(p_VirtIfVlan->Interface, pString);
                return(TRUE);
	    }
	    return(FALSE);
        }
    }

    return ret;
}

/**********************************************************************

     caller:     owner of this object

    prototype:

         BOOL WanIfVirtIfVLAN_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);

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
BOOL WanIfVirtIfVLAN_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************

     caller:     owner of this object

     prototype:

         ULONG WanIfVirtIfVLAN_Commit(ANSC_HANDLE hInsContext);

     description:

        This function is called to finally commit all the update.

     argument:   ANSC_HANDLE                 hInsContext,
                 The instance handle;
    return:     The status of the operation.

**********************************************************************/

ULONG WanIfVirtIfVLAN_Commit(ANSC_HANDLE hInsContext)
{
    CONTEXT_VIRTIF_VLAN_LINK_OBJECT* pVirtIfVlanCxtLink = (CONTEXT_VIRTIF_VLAN_LINK_OBJECT*)hInsContext;
    ULONG returnStatus = 0;

    if (pVirtIfVlanCxtLink)
    {
        returnStatus = WanManager_Update_VirtIfVlan_Entry(pVirtIfVlanCxtLink);
    }
    return returnStatus;
}

/**********************************************************************
    caller:     owner of this object

    prototype:

         ULONG WanIfVirtIfVLAN_Rollback(ANSC_HANDLE hInsContext);

     description:

        This function is called to roll back the update whenever there's a
        validation found.

     argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

	 return:     The status of the operation.

**********************************************************************/
ULONG WanIfVirtIfVLAN_Rollback(ANSC_HANDLE hInsContext)
{
    return 0;
}


/***********************************************************************
APIs for Object:

    Device.X_RDK_WanManager.CPEInterface.{i}.VirtualInterface.Marking.{i}.

    *  WanIfVirtIfMarking_GetEntryCount
    *  WanIfVirtIfMarking_GetEntry
    *  WanIfVirtIfMarking_AddEntry
    *  WanIfVirtIfMarking_DelEntry
    *  WanIfVirtIfMarking_GetParamStringValue
    *  WanIfVirtIfMarking_SetParamStringValue
    *  WanIfVirtIfMarking_Validate
    *  WanIfVirtIfMarking_Commit
    *  WanIfVirtIfMarking_Rollback

***********************************************************************/

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG WanIfVirtIfVLAN_GetEntryCount(ANSC_HANDLE hInsContext);

    description:

         This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

   return:     The count of the table

**********************************************************************/

ULONG WanIfVirtIfMarking_GetEntryCount(ANSC_HANDLE hInsContext)
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    ULONG VirtIfMarkingCount = 0;

    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;
        VirtIfMarkingCount = AnscSListQueryDepth( &(p_VirtIf->Marking.VirtIfMarkingList) );
    }

    return VirtIfMarkingCount;
}

/**********************************************************************

     caller:     owner of this object

     prototype:

        ANSC_HANDLE WanIfVirtIfMarking_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber);

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
ANSC_HANDLE WanIfVirtIfMarking_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber)
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    PSINGLE_LINK_ENTRY pSListEntry = NULL;
    *pInsNumber= 0;

    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;
        pSListEntry = AnscSListGetEntryByIndex(&(p_VirtIf->Marking.VirtIfMarkingList), nIndex);

        if ( pSListEntry )
        {
            CONTEXT_VIRTIF_MARKING_LINK_OBJECT* pVirtIfMarkingCxtLink = ACCESS_CONTEXT_VIRTIF_MARKING_LINK_OBJECT(pSListEntry);
            *pInsNumber = pVirtIfMarkingCxtLink->InstanceNumber;
        }
    }

    return (ANSC_HANDLE)pSListEntry;
}



/**********************************************************************

     caller:     owner of this object

     prototype:

        ANSC_HANDLE WanIfVirtIfMarking_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber);

     description:

        This function is called to add a new entry.

     argument:   ANSC_HANDLE                 hInsContext,
                 The instance handle;

                 ULONG*                      pInsNumber
                The output instance number;

return:     The handle of new added entry.

**********************************************************************/

ANSC_HANDLE WanIfVirtIfMarking_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber)
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    ANSC_HANDLE pVirtIfMarkingCxtLink = NULL;
    *pInsNumber = 0;

    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;
        pVirtIfMarkingCxtLink = WanManager_Create_VirtIfMarking_Entry(pVirtIfCxtLink, pInsNumber);
    }

    return pVirtIfMarkingCxtLink;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

         ULONG WanIfVirtIfMarking_DelEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance);

    description:

        This function is called to delete an exist entry.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ANSC_HANDLE                 hInstance
                The exist entry handle;

    return:     The status of the operation.

**********************************************************************/
ULONG WanIfVirtIfMarking_DelEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance)
{
    CONTEXT_VIRTIF_MARKING_LINK_OBJECT* pVirtIfMarkingCxtLink = (CONTEXT_VIRTIF_MARKING_LINK_OBJECT*)hInstance;
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    ULONG returnStatus = -1;

    if(pVirtIfCxtLink && pVirtIfMarkingCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;
        returnStatus = WanManager_Remove_VirtIfMarking_Entry(pVirtIfCxtLink, pVirtIfMarkingCxtLink);
    }

    return returnStatus;
}

/**********************************************************************
     caller:     owner of this object

     prototype:

         ULONG WanIfVirtIfMarking_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);

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
ULONG WanIfVirtIfMarking_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize)
{
    CONTEXT_VIRTIF_MARKING_LINK_OBJECT* pVirtIfMarkingCxtLink = (CONTEXT_VIRTIF_MARKING_LINK_OBJECT*)hInsContext;

    /* check the parameter name and return the corresponding value */
    if (pVirtIfMarkingCxtLink)
    {
        DML_VIRTIF_MARKING* p_VirtIfMarking = (DML_VIRTIF_MARKING* )pVirtIfMarkingCxtLink->hContext;

        if( AnscEqualString(ParamName, "Entry", TRUE))
        {
            if ( AnscSizeOfString(p_VirtIfMarking->Entry) < *pUlSize)
            {
                AnscCopyString(pValue, p_VirtIfMarking->Entry);
                return 0;
            }
            else
            {
                *pUlSize = AnscSizeOfString(p_VirtIfMarking->Entry)+1;
                return 1;
            }
        }
    }

    return -1;
}

/**********************************************************************
     caller:     owner of this object

     prototype:

         BOOL WanIfVirtIfMarking_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);

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
BOOL WanIfVirtIfMarking_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
{
    CONTEXT_VIRTIF_MARKING_LINK_OBJECT* pVirtIfMarkingCxtLink = (CONTEXT_VIRTIF_MARKING_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and set the corresponding value */
    if (pVirtIfMarkingCxtLink)
    {
        DML_VIRTIF_MARKING* p_VirtIfMarking = (DML_VIRTIF_MARKING* )pVirtIfMarkingCxtLink->hContext;

        if( AnscEqualString(ParamName, "Entry", TRUE))
        {
            if (AnscSizeOfString(p_VirtIfMarking->Entry) > 0)
            {
                if (AnscEqualString(p_VirtIfMarking->Entry, pString, TRUE))
                {
                    return(FALSE);
                }
                memset(p_VirtIfMarking->Entry, 0, sizeof(p_VirtIfMarking->Entry));
            }

	    if (AnscSizeOfString(pString) < sizeof(p_VirtIfMarking->Entry))
	    {
                AnscCopyString(p_VirtIfMarking->Entry, pString);
                return(TRUE);
	    }
	    return(FALSE);
        }
    }

    return ret;
}

/**********************************************************************

     caller:     owner of this object

    prototype:

         BOOL WanIfVirtIfMarking_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);

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
BOOL WanIfVirtIfMarking_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************

     caller:     owner of this object

     prototype:

         ULONG WanIfVirtIfMarking_Commit(ANSC_HANDLE hInsContext);

     description:

        This function is called to finally commit all the update.

     argument:   ANSC_HANDLE                 hInsContext,
                 The instance handle;
    return:     The status of the operation.

**********************************************************************/

ULONG WanIfVirtIfMarking_Commit(ANSC_HANDLE hInsContext)
{
    CONTEXT_VIRTIF_MARKING_LINK_OBJECT* pVirtIfMarkingCxtLink = (CONTEXT_VIRTIF_MARKING_LINK_OBJECT*)hInsContext;
    ULONG returnStatus = 0;

    if (pVirtIfMarkingCxtLink)
    {
        returnStatus = WanManager_Update_VirtIfMarking_Entry(pVirtIfMarkingCxtLink);
    }
    return returnStatus;
}

/**********************************************************************
    caller:     owner of this object

    prototype:

         ULONG WanIfVirtIfMarking_Rollback(ANSC_HANDLE hInsContext);

     description:

        This function is called to roll back the update whenever there's a
        validation found.

     argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

         return:     The status of the operation.

**********************************************************************/
ULONG WanIfVirtIfMarking_Rollback(ANSC_HANDLE hInsContext)
{
    return 0;
}

/***********************************************************************

 APIs for Object:

    Device.X_RDK_WanManager.CPEInterface.{I}.Marking.{i}.

    *  WanIfMarking_GetEntryCount
    *  WanIfMarking_GetEntry
    *  WanIfMarking_AddEntry
    *  WanIfMarking_DelEntry
    *  WanIfMarking_GetParamUlongValue
    *  WanIfMarking_GetParamStringValue
    *  WanIfMarking_GetParamIntValue
    *  WanIfMarking_SetParamUlongValue
    *  WanIfMarking_SetParamStringValue
    *  WanIfMarking_SetParamIntValue
    *  WanIfMarking_Validate
    *  WanIfMarking_Commit
    *  WanIfMarking_Rollback

***********************************************************************/
/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG WanIfMarking_GetEntryCount(ANSC_HANDLE hInsContext);

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/

ULONG WanIfMarking_GetEntryCount(ANSC_HANDLE hInsContext)
{

    ULONG count = 0;



#ifdef FEATURE_802_1P_COS_MARKING
    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            count = AnscSListQueryDepth( &(pWanDmlIface->Marking.MarkingList) );

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }
#endif /* * FEATURE_802_1P_COS_MARKING */

    return count;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_HANDLE WanIfMarking_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber);

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

ANSC_HANDLE WanIfMarking_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber)
{
    PSINGLE_LINK_ENTRY  pSListEntry = NULL;
    *pInsNumber= 0;

#ifdef FEATURE_802_1P_COS_MARKING
    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            pSListEntry       = AnscSListGetEntryByIndex(&(pWanDmlIface->Marking.MarkingList), nIndex);
            if ( pSListEntry )
            {
                CONTEXT_MARKING_LINK_OBJECT* pCxtLink      = ACCESS_CONTEXT_MARKING_LINK_OBJECT(pSListEntry);
                *pInsNumber   = pCxtLink->InstanceNumber;
            }


            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }
#endif /* * FEATURE_802_1P_COS_MARKING */

    return (ANSC_HANDLE)pSListEntry;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ANSC_HANDLE WanIfMarking_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber);

    description:

        This function is called to add a new entry.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle of new added entry.

**********************************************************************/

ANSC_HANDLE WanIfMarking_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber)
{
    ANSC_HANDLE newMarking = NULL;

#ifdef FEATURE_802_1P_COS_MARKING
    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            newMarking = WanManager_AddIfaceMarking(pWanDmlIface, pInsNumber);

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }
#endif /* * FEATURE_802_1P_COS_MARKING */

    return newMarking;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG WanIfMarking_DelEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance);

    description:

        This function is called to delete an exist entry.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ANSC_HANDLE                 hInstance
                The exist entry handle;

    return:     The status of the operation.

**********************************************************************/

ULONG WanIfMarking_DelEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance)
{

    ULONG returnStatus = -1;

#ifdef FEATURE_802_1P_COS_MARKING
    CONTEXT_MARKING_LINK_OBJECT* pMarkingCxtLink   = (CONTEXT_MARKING_LINK_OBJECT*)hInstance;
    if(pMarkingCxtLink == NULL)
    {
        return -1;
    }


    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);


            if ( pMarkingCxtLink->bNew )
            {
                /* Set bNew to FALSE to indicate this node is not going to save to SysRegistry */
                pMarkingCxtLink->bNew = FALSE;
            }

            DML_MARKING* p_Marking = (DML_MARKING*)pMarkingCxtLink->hContext;

            returnStatus = DmlDeleteMarking( NULL, p_Marking );

            if ( ( ANSC_STATUS_SUCCESS == returnStatus ) && \
                ( AnscSListPopEntryByLink(&(pWanDmlIface->Marking.MarkingList), &pMarkingCxtLink->Linkage) ) )
            {
                AnscFreeMemory(pMarkingCxtLink->hContext);
                AnscFreeMemory(pMarkingCxtLink);
            }


            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }
#endif /* * FEATURE_802_1P_COS_MARKING */

    return returnStatus;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfMarking_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);

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

BOOL WanIfMarking_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
{
    BOOL                                ret = FALSE;
    CONTEXT_MARKING_LINK_OBJECT*        pCxtLink      = (CONTEXT_MARKING_LINK_OBJECT*)hInsContext;
    DML_MARKING*                        p_Marking     = (DML_MARKING* )pCxtLink->hContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "SKBPort", TRUE))
    {
        *puLong = p_Marking->SKBPort;
        ret = TRUE;
    }
    if( AnscEqualString(ParamName, "SKBMark", TRUE))
    {
        *puLong = p_Marking->SKBMark;
        ret = TRUE;
    }

    return -1;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG WanIfMarking_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);

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
ULONG WanIfMarking_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize)
{
    CONTEXT_MARKING_LINK_OBJECT*        pCxtLink      = (CONTEXT_MARKING_LINK_OBJECT*)hInsContext;
    DML_MARKING*                        p_Marking     = (DML_MARKING* )pCxtLink->hContext;

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

    return -1;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfMarking_GetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int* pInt);

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
BOOL WanIfMarking_GetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int* pInt)
{
    BOOL ret = FALSE;
    CONTEXT_MARKING_LINK_OBJECT*        pCxtLink      = (CONTEXT_MARKING_LINK_OBJECT*)hInsContext;
    DML_MARKING*                        p_Marking     = (DML_MARKING* )pCxtLink->hContext;

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "EthernetPriorityMark", TRUE))
    {
        *pInt = p_Marking->EthernetPriorityMark;
        ret = TRUE;
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfMarking_SetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int iValue);

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
BOOL WanIfMarking_SetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int iValue)
{
    BOOL ret = FALSE;
    CONTEXT_MARKING_LINK_OBJECT*        pCxtLink      = (CONTEXT_MARKING_LINK_OBJECT*)hInsContext;
    DML_MARKING*                        p_Marking     = (DML_MARKING* )pCxtLink->hContext;

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "EthernetPriorityMark", TRUE))
    {
        p_Marking->EthernetPriorityMark = iValue;
        ret = TRUE;
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfMarking_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);

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

BOOL WanIfMarking_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue)
{
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfMarking_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);

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

BOOL WanIfMarking_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
{
    BOOL ret = FALSE;
    CONTEXT_MARKING_LINK_OBJECT*        pCxtLink      = (CONTEXT_MARKING_LINK_OBJECT*)hInsContext;
    DML_MARKING*                        p_Marking     = (DML_MARKING* )pCxtLink->hContext;

    /* check the parameter name and set the corresponding value */

    if( AnscEqualString(ParamName, "Alias", TRUE))
    {
        //Alias should not overwrite after set
        if( 0 < AnscSizeOfString(p_Marking->Alias) )
        {
            return FALSE;
        }

        AnscCopyString(p_Marking->Alias, pString);
        ret = TRUE;
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfMarking_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);

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

BOOL WanIfMarking_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG WanIfMarking_Commit(ANSC_HANDLE hInsContext);

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/

ULONG WanIfMarking_Commit(ANSC_HANDLE hInsContext)
{

    ANSC_STATUS                              returnStatus  = ANSC_STATUS_SUCCESS;

#ifdef FEATURE_802_1P_COS_MARKING
    CONTEXT_MARKING_LINK_OBJECT*        pCxtLink      = (CONTEXT_MARKING_LINK_OBJECT*)hInsContext;
    DML_MARKING*                        p_Marking     = (DML_MARKING* )pCxtLink->hContext;

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
#endif /* * FEATURE_802_1P_COS_MARKING */

    return returnStatus;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG WanIfMarking_Rollback(ANSC_HANDLE hInsContext);

    description:

        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/

ULONG WanIfMarking_Rollback(ANSC_HANDLE hInsContext)
{
    return 0;
}

