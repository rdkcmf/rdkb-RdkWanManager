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

#include "wanmgr_dml_iface_apis.h"
#include "wanmgr_rdkbus_apis.h"
#include "wanmgr_net_utils.h"
#include "wanmgr_dhcpv4_apis.h"
#include "wanmgr_dhcpv6_apis.h"
#include "wanmgr_data.h"

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

    WanMgr_IfaceCtrl_Data_t*    pWanDmlIfaceCtrl = WanMgr_GetIfaceCtrl_locked();
    if(pWanDmlIfaceCtrl != NULL)
    {
        count = pWanDmlIfaceCtrl->ulTotalNumbWanInterfaces;

        WanMgrDml_GetIfaceCtrl_release(pWanDmlIfaceCtrl);
    }

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
            else if( AnscEqualString(ParamName, "DisplayName", TRUE) )
            {
               /* collect value */
               if ( ( sizeof( pWanDmlIface->DisplayName ) - 1 ) < *pUlSize )
               {
                   AnscCopyString( pValue, pWanDmlIface->DisplayName );
                   ret = 0;
               }
               else
               {
                   *pUlSize = sizeof( pWanDmlIface->DisplayName );
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
                AnscCopyString(pWanDmlIface->Name, pString);
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

        BOOL WanIfCfg_GetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int* pInt);

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
BOOL WanIfCfg_GetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int* pInt)
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
                *pInt = pWanDmlIface->Wan.Priority;
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

        BOOL WanIfCfg_SetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int iValue);

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
BOOL WanIfCfg_SetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int iValue)
{
    BOOL ret = FALSE;
    UINT uiTotalIfaces = -1;
    INT  IfIndex = 0;
    BOOL Status = FALSE;
    DML_WAN_IFACE_TYPE priorityType = WAN_IFACE_TYPE_UNCONFIGURED;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);
            IfIndex =  pWanDmlIface->uiIfaceIdx;
            priorityType = pWanDmlIface->Wan.Type;
            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);

            /* check the parameter name and set the corresponding value */
            if( AnscEqualString(ParamName, "Priority", TRUE))
            {
                uiTotalIfaces = WanIf_GetEntryCount(NULL);

                if ( WanManager_CheckGivenPriorityExists(IfIndex, uiTotalIfaces, iValue, priorityType, &Status) == ANSC_STATUS_SUCCESS )
                {
                    if(Status)
                    {
                        CcspTraceError(("%s Another interface with same type is already exists with given priority \n", __FUNCTION__));
                        return FALSE;
                    }
                    WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
                    if(pWanDmlIfaceData != NULL)
                    {
                        pWanDmlIface->Wan.Priority = iValue;
                        WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
                        return TRUE;
                    }
                }
            }
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfCfg_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);

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
BOOL WanIfCfg_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
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
            if( AnscEqualString(ParamName, "SelectionTimeout", TRUE))
            {
                *puLong = pWanDmlIface->Wan.SelectionTimeout;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "Status", TRUE))
            {
                *puLong = pWanDmlIface->Wan.Status;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "Type", TRUE))
            {
                *puLong = pWanDmlIface->Wan.Type;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "Priority", TRUE))
            {
                *puLong = pWanDmlIface->Wan.Priority;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "LinkStatus", TRUE))
            {
                *puLong = pWanDmlIface->Wan.LinkStatus;
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

        BOOL WanIfCfg_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);

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
BOOL WanIfCfg_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue)
{
    BOOL ret = FALSE;
    UINT uiTotalIfaces = -1;
    INT  IfIndex = 0;
    INT  priority;
    BOOL Status = FALSE;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            if( AnscEqualString(ParamName, "SelectionTimeout", TRUE))
            {
                pWanDmlIface->Wan.SelectionTimeout = uValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "Type", TRUE))
            {
                IfIndex =  pWanDmlIface->uiIfaceIdx;
                priority =  pWanDmlIface->Wan.Priority;
                WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
                uiTotalIfaces = WanIf_GetEntryCount(NULL);
                if ( WanManager_CheckGivenTypeExists(IfIndex, uiTotalIfaces, uValue, priority, &Status) == ANSC_STATUS_SUCCESS )
                {
                    if(Status)
                    {
                        CcspTraceError(("%s Another interface with same type and priority already exists\n", __FUNCTION__));
                        return FALSE;
                    }
                    WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
                    if(pWanDmlIfaceData != NULL)
                    {
                        pWanDmlIface->Wan.Type = uValue;
                        WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
                        return TRUE;
                    }
                    return FALSE;
                }
            }
            if( AnscEqualString(ParamName, "LinkStatus", TRUE))
            {
                pWanDmlIface->Wan.LinkStatus = uValue;
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

        BOOL WanIfCfg_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool);

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
BOOL WanIfCfg_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool)
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
                *pBool = pWanDmlIface->Wan.Enable;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "Refresh", TRUE))
            {
                *pBool = pWanDmlIface->Wan.Refresh;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "ActiveLink", TRUE))
            {
                *pBool = pWanDmlIface->Wan.ActiveLink;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "EnableDSLite", TRUE))
            {
                *pBool = pWanDmlIface->Wan.EnableDSLite;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "EnableIPoEHealthCheck", TRUE))
            {
                *pBool = pWanDmlIface->Wan.EnableIPoE;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "EnableMAPT", TRUE))
            {
                *pBool = pWanDmlIface->Wan.EnableMAPT;
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

        BOOL WanIfCfg_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue);

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
BOOL WanIfCfg_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue)
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
                pWanDmlIface->Wan.Enable  = bValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "Refresh", TRUE))
            {
                pWanDmlIface->Wan.Refresh = bValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "ActiveLink", TRUE))
            {
                pWanDmlIface->Wan.ActiveLink = bValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "EnableDSLite", TRUE))
            {
                pWanDmlIface->Wan.EnableDSLite = bValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "EnableIPoEHealthCheck", TRUE))
            {
                pWanDmlIface->Wan.EnableIPoE = bValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "EnableMAPT", TRUE))
            {
                pWanDmlIface->Wan.EnableMAPT = bValue;
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

        ULONG WanIfCfg_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);

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
ULONG WanIfCfg_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize)
{
    ULONG ret = -1;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* collect value */
            if ( ( sizeof( pWanDmlIface->Wan.Name ) - 1 ) < *pUlSize )
            {
                AnscCopyString( pValue, pWanDmlIface->Wan.Name );
                ret = 0;
            }
            else
            {
                *pUlSize = sizeof( pWanDmlIface->Wan.Name );
                ret = 1;
            }

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfCfg_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);

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
BOOL WanIfCfg_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
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
                AnscCopyString(pWanDmlIface->Wan.Name, pString);
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

        BOOL WanIfCfg_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);

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
BOOL WanIfCfg_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIfCfg_Commit(ANSC_HANDLE hInsContext);

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfCfg_Commit(ANSC_HANDLE hInsContext)
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

            result = DmlSetWanIfCfg( pWanDmlIface->uiInstanceNumber, pWanDmlIface );
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
        ULONG WanIfCfg_Rollback(ANSC_HANDLE hInsContext);

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfCfg_Rollback(ANSC_HANDLE hInsContext)
{
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

        BOOL WanIfValidation_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool);

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
BOOL WanIfValidation_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool)
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
            if( AnscEqualString(ParamName, "Discovery-Offer", TRUE))
            {
                *pBool = pWanDmlIface->Wan.Validation.DiscoverOffer;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "Solicit-Advertise", TRUE))
            {
                *pBool = pWanDmlIface->Wan.Validation.SolicitAdvertise;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "RS-RA", TRUE))
            {
                *pBool = pWanDmlIface->Wan.Validation.RS_RA;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "PADI-PADO", TRUE))
            {
                *pBool = pWanDmlIface->Wan.Validation.PadiPado;
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

        BOOL WanIfValidation_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue);

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
BOOL WanIfValidation_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue)
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
            if( AnscEqualString(ParamName, "Discovery-Offer", TRUE))
            {
                pWanDmlIface->Wan.Validation.DiscoverOffer = bValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "Solicit-Advertise", TRUE))
            {
                pWanDmlIface->Wan.Validation.SolicitAdvertise = bValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "RS-RA", TRUE))
            {
                pWanDmlIface->Wan.Validation.RS_RA = bValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "PADI-PADO", TRUE))
            {
                pWanDmlIface->Wan.Validation.PadiPado = bValue;
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

        BOOL WanIfValidation_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);

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
BOOL WanIfValidation_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIfValidation_Commit(ANSC_HANDLE hInsContext);

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfValidation_Commit(ANSC_HANDLE hInsContext)
{
    ULONG ret = -1;


    ANSC_STATUS result = ANSC_STATUS_SUCCESS;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            result = DmlSetWanIfValidationCfg(pWanDmlIface->uiInstanceNumber, pWanDmlIface);
            if (result != ANSC_STATUS_SUCCESS)
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
        ULONG WanIfValidation_Rollback(ANSC_HANDLE hInsContext);

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfValidation_Rollback(ANSC_HANDLE hInsContext)
{
    return 0;
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


ULONG WanIfPhy_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize)
{
    ULONG ret = -1;


    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            if( AnscEqualString(ParamName, "Path", TRUE) )
            {
               /* collect value */
               if ( ( sizeof( pWanDmlIface->Phy.Path ) - 1 ) < *pUlSize )
               {
                   AnscCopyString( pValue, pWanDmlIface->Phy.Path );
                   ret = 0;
               }
               else
               {
                   *pUlSize = sizeof( pWanDmlIface->Phy.Path );
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

        BOOL WanIfPhy_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);

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

BOOL WanIfPhy_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
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
            if( AnscEqualString(ParamName, "Path", TRUE))
            {
                AnscCopyString(pWanDmlIface->Phy.Path, pString);
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

        BOOL WanIfPhy_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);

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

BOOL WanIfPhy_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
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
                *puLong = pWanDmlIface->Phy.Status;
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

        BOOL WanIfPhy_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);

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

BOOL WanIfPhy_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue)
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
                pWanDmlIface->Phy.Status = uValue;
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

        BOOL WanIfPhy_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);

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
BOOL WanIfPhy_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIfPhy_Commit(ANSC_HANDLE hInsContext);

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfPhy_Commit(ANSC_HANDLE hInsContext)
{
    return 0;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIfPhy_Rollback(ANSC_HANDLE hInsContext);

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfPhy_Rollback(ANSC_HANDLE hInsContext)
{
    return 0;
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

        BOOL WanIfDynTrigger_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);

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
BOOL WanIfDynTrigger_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
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
            if( AnscEqualString(ParamName, "Delay", TRUE))
            {
                *puLong = pWanDmlIface->DynamicTrigger.Delay;
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

        BOOL WanIfDynTrigger_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);

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
BOOL WanIfDynTrigger_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue)
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
            if( AnscEqualString(ParamName, "Delay", TRUE))
            {
                pWanDmlIface->DynamicTrigger.Delay = uValue;
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

        BOOL WanIfDynTrigger_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool);

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
BOOL WanIfDynTrigger_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool)
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
                *pBool = pWanDmlIface->DynamicTrigger.Enable;
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

        BOOL WanIfDynTrigger_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue);

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
BOOL WanIfDynTrigger_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue)
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
                /* save update to backup */
                pWanDmlIface->DynamicTrigger.Enable     = bValue;
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

        BOOL WanIfDynTrigger_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);

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
BOOL WanIfDynTrigger_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIfDynTrigger_Commit(ANSC_HANDLE hInsContext);

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfDynTrigger_Commit(ANSC_HANDLE hInsContext)
{
    return 0;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIfDynTrigger_Rollback(ANSC_HANDLE hInsContext);

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfDynTrigger_Rollback(ANSC_HANDLE hInsContext)
{
    return 0;
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

        BOOL WanIfIpCfg_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);

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
BOOL WanIfIpCfg_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
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
            if( AnscEqualString(ParamName, "IPv4Status", TRUE))
            {
                *puLong = pWanDmlIface->IP.Ipv4Status;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "IPv6Status", TRUE))
            {
                *puLong = pWanDmlIface->IP.Ipv6Status;
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

        BOOL WanIfIpCfg_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);

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
BOOL WanIfIpCfg_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue)
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
            if( AnscEqualString(ParamName, "IPv4Status", TRUE))
            {
                pWanDmlIface->IP.Ipv4Status = uValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "IPv6Status", TRUE))
            {
                pWanDmlIface->IP.Ipv6Status = uValue;
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

        ULONG WanIfIpCfg_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);

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
ULONG WanIfIpCfg_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize)
{
    ULONG ret = -1;


    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            if( AnscEqualString(ParamName, "Path", TRUE) )
            {
                /* collect value */
                if ( ( sizeof( pWanDmlIface->IP.Path ) - 1 ) < *pUlSize )
                {
                    AnscCopyString( pValue, pWanDmlIface->IP.Path );
                    ret = 0;
                }
                else
                {
                    *pUlSize = sizeof( pWanDmlIface->IP.Path );
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

        BOOL WanIfIpCfg_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);

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
BOOL WanIfIpCfg_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
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
            if( AnscEqualString(ParamName, "Path", TRUE))
            {
                AnscCopyString(pWanDmlIface->IP.Path, pString);
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

        BOOL WanIfIpCfg_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);

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
BOOL WanIfIpCfg_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIfIpCfg_Commit(ANSC_HANDLE hInsContext);

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfIpCfg_Commit(ANSC_HANDLE hInsContext)
{
    return 0;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIfIpCfg_Rollback(ANSC_HANDLE hInsContext);

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfIpCfg_Rollback(ANSC_HANDLE hInsContext)
{
    return 0;
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

        BOOL WanIfMapt_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);

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
BOOL WanIfMapt_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
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
            if( AnscEqualString(ParamName, "MAPTStatus", TRUE))
            {
                *puLong = pWanDmlIface->MAP.MaptStatus;
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

        BOOL WanIfMapt_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);

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
BOOL WanIfMapt_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue)
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
            if( AnscEqualString(ParamName, "MAPTStatus", TRUE))
            {
#ifdef FEATURE_MAPT
                pWanDmlIface->MAP.MaptStatus = uValue;
                ret = TRUE;
#endif /* * FEATURE_MAPT */
            }

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG WanIfMapt_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);

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
ULONG WanIfMapt_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize)
{
    ULONG ret = -1;



    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            if( AnscEqualString(ParamName, "Path", TRUE) )
            {
                /* collect value */
                if ( ( sizeof( pWanDmlIface->MAP.Path ) - 1 ) < *pUlSize )
                {
                    AnscCopyString( pValue, pWanDmlIface->MAP.Path );
                    ret = 0;
                }
                else
                {
                    *pUlSize = sizeof( pWanDmlIface->MAP.Path );
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

        BOOL WanIfMapt_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);

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
BOOL WanIfMapt_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
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
            if( AnscEqualString(ParamName, "Path", TRUE))
            {
#ifdef FEATURE_MAPT
                AnscCopyString(pWanDmlIface->MAP.Path, pString);
                ret = TRUE;
#endif /* * FEATURE_MAPT */
            }

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL WanIfMapt_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);

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
BOOL WanIfMapt_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIfMapt_Commit(ANSC_HANDLE hInsContext);

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfMapt_Commit(ANSC_HANDLE hInsContext)
{
    return 0;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIfMapt_Rollback(ANSC_HANDLE hInsContext);

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfMapt_Rollback(ANSC_HANDLE hInsContext)
{
    return 0;
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

        BOOL WanIfDSLite_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);

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
BOOL WanIfDSLite_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
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
                *puLong = pWanDmlIface->DSLite.Status;
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

        BOOL WanIfDSLite_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);

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
BOOL WanIfDSLite_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue)
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
                pWanDmlIface->DSLite.Status = uValue;
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

        ULONG WanIfDSLite_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);

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
ULONG WanIfDSLite_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize)
{
    ULONG ret = -1;



    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            if( AnscEqualString(ParamName, "Path", TRUE) )
            {
                /* collect value */
                if ( ( sizeof( pWanDmlIface->DSLite.Path ) - 1 ) < *pUlSize )
                {
                    AnscCopyString( pValue, pWanDmlIface->DSLite.Path );
                    ret = 0;
                }
                else
                {
                    *pUlSize = sizeof( pWanDmlIface->DSLite.Path );
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

        BOOL WanIfDSLite_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);

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
BOOL WanIfDSLite_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
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
            if( AnscEqualString(ParamName, "Path", TRUE))
            {
                AnscCopyString(pWanDmlIface->DSLite.Path, pString);
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

       BOOL WanIfDSLite_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);

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
BOOL WanIfDSLite_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIfDSLite_Commit(ANSC_HANDLE hInsContext);

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfDSLite_Commit(ANSC_HANDLE hInsContext)
{
    return 0;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG WanIfDSLite_Rollback(ANSC_HANDLE hInsContext);

    description:
        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/
ULONG WanIfDSLite_Rollback(ANSC_HANDLE hInsContext)
{
    return 0;
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

        ULONG Marking_GetEntryCount(ANSC_HANDLE hInsContext);

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/

ULONG Marking_GetEntryCount(ANSC_HANDLE hInsContext)
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

        ANSC_HANDLE Marking_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber);

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

ANSC_HANDLE Marking_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber)
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

        ANSC_HANDLE Marking_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber);

    description:

        This function is called to add a new entry.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle of new added entry.

**********************************************************************/

ANSC_HANDLE Marking_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber)
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

        ULONG Marking_DelEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance);

    description:

        This function is called to delete an exist entry.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ANSC_HANDLE                 hInstance
                The exist entry handle;

    return:     The status of the operation.

**********************************************************************/

ULONG Marking_DelEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance)
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

        BOOL Marking_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);

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

BOOL Marking_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
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

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG Marking_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);

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

ULONG Marking_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize)
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

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL Marking_GetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int* pInt);

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
BOOL Marking_GetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int* pInt)
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

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL Marking_SetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int iValue);

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
BOOL Marking_SetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int iValue)
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

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL Marking_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);

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

BOOL Marking_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue)
{
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL Marking_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);

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

BOOL Marking_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
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

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL Marking_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);

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

BOOL Marking_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG Marking_Commit(ANSC_HANDLE hInsContext);

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/

ULONG Marking_Commit(ANSC_HANDLE hInsContext)
{

    ANSC_STATUS                              returnStatus  = ANSC_STATUS_SUCCESS;
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

    return returnStatus;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG Marking_Rollback(ANSC_HANDLE hInsContext);

    description:

        This function is called to roll back the update whenever there's a
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/

ULONG Marking_Rollback(ANSC_HANDLE hInsContext)
{
    return 0;
}

/***********************************************************************

 APIs for Object:

    X_RDK_WanManager.CPEInterface.{i}.PPP.

    *  WanIfPPPCfg_GetParamUlongValue
    *  WanIfPPPCfg_SetParamUlongValue
    *  WanIfPPPCfg_GetParamStringValue
    *  WanIfPPPCfg_SetParamStringValue
    *  WanIfPPPCfg_GetParamBoolValue
    *  WanIfPPPCfg_SetParamBoolValue
    *  WanIfPPPCfg_Validate
    *  WanIfPPPCfg_Commit
    *  WanPPPIpCfg_Rollback

***********************************************************************/

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfPPPCfg_GetParamUlongValue
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
WanIfPPPCfg_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
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
            if( AnscEqualString(ParamName, "IPCPStatus", TRUE))
            {
                *puLong = pWanDmlIface->PPP.IPCPStatus;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "IPv6CPStatus", TRUE))
            {
                *puLong = pWanDmlIface->PPP.IPV6CPStatus;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "LCPStatus", TRUE))
            {
                *puLong = pWanDmlIface->PPP.LCPStatus;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "LinkStatus", TRUE))
            {
                *puLong = pWanDmlIface->PPP.LinkStatus;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "LinkType", TRUE))
            {
                *puLong = pWanDmlIface->PPP.LinkType;
                ret = TRUE;
            }

            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return ret;

}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfPPPCfg_SetParamUlongValue
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
WanIfPPPCfg_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    BOOL ret = FALSE;
    int iErrorCode = 0;
    char *pInterface = NULL;
    pthread_t IPCPHandlerThread;
    pthread_t IPV6CPHandlerThread;

    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            if( AnscEqualString(ParamName, "IPCPStatus", TRUE))
            {
                pWanDmlIface->PPP.IPCPStatus = uValue;
                pInterface = (char *) malloc(64);
                if (pInterface != NULL)
                {
                    strncpy(pInterface, pWanDmlIface->Wan.Name, 64);
                    iErrorCode = pthread_create( &IPCPHandlerThread, NULL, &IPCPStateChangeHandler, (void*) pInterface );
                    if( 0 != iErrorCode )
                    {
                        CcspTraceInfo(("%s %d - Failed to handle IPCP event change   %d\n", __FUNCTION__, __LINE__, iErrorCode ));
                    }
                    ret = TRUE;
                }
            }
            if( AnscEqualString(ParamName, "IPv6CPStatus", TRUE))
            {
                pWanDmlIface->PPP.IPV6CPStatus = uValue;
                pInterface = (char *) malloc(64);
                if (pInterface != NULL)
                {
                    strncpy(pInterface, pWanDmlIface->Wan.Name, 64);
                    iErrorCode = pthread_create( &IPCPHandlerThread, NULL, &IPV6CPStateChangeHandler, (void*) pInterface );
                    if( 0 != iErrorCode )
                    {
                        CcspTraceInfo(("%s %d - Failed to handle IPV6CP event change   %d\n", __FUNCTION__, __LINE__, iErrorCode ));
                    }
                    ret = TRUE;
                }
            }
            if( AnscEqualString(ParamName, "LCPStatus", TRUE))
            {
                pWanDmlIface->PPP.LCPStatus = uValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "LinkStatus", TRUE))
            {
                pWanDmlIface->PPP.LinkStatus = uValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "LinkType", TRUE))
            {
                pWanDmlIface->PPP.LinkType = uValue;
                ret = TRUE;
            }
            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }
    }


    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return ret;
}


/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        WanIfPPPCfg_GetParamStringValue
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
WanIfPPPCfg_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    ULONG ret = -1;


    WanMgr_Iface_Data_t* pIfaceDmlEntry = (WanMgr_Iface_Data_t*) hInsContext;
    if(pIfaceDmlEntry != NULL)
    {
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pIfaceDmlEntry->data.uiIfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pWanDmlIface = &(pWanDmlIfaceData->data);

            /* check the parameter name and set the corresponding value */
            if( AnscEqualString(ParamName, "Path", TRUE) )
            {
               /* collect value */
               if ( ( sizeof( pWanDmlIface->Phy.Path ) - 1 ) < *pUlSize )
               {
                   AnscCopyString( pValue, pWanDmlIface->PPP.Path);
                   ret = 0;
               }
               else
               {
                   *pUlSize = sizeof( pWanDmlIface->PPP.Path );
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

        BOOL
        WanIfPPPCfg_SetParamStringValue
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
WanIfPPPCfg_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
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
            if( AnscEqualString(ParamName, "Path", TRUE))
            {
                AnscCopyString(pWanDmlIface->PPP.Path, pString);
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

        BOOL
        WanIfPPPCfg_GetParamBoolValue
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

ULONG
WanIfPPPCfg_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
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
            if( AnscEqualString(ParamName, "Enable", TRUE))
            {
                *pBool = pWanDmlIface->PPP.Enable;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "IPCPEnable", TRUE))
            {
                *pBool = pWanDmlIface->PPP.IPCPEnable;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "IPv6CPEnable", TRUE))
            {
                *pBool = pWanDmlIface->PPP.IPV6CPEnable;
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

        BOOL
        WanIfPPPCfg_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
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
WanIfPPPCfg_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
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
                pWanDmlIface->PPP.Enable = bValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "IPCPEnable", TRUE))
            {
                pWanDmlIface->PPP.IPCPEnable = bValue;
                ret = TRUE;
            }
            if( AnscEqualString(ParamName, "IPv6CPEnable", TRUE))
            {
                pWanDmlIface->PPP.IPV6CPEnable = bValue;
                ret = TRUE;
            }
        }
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
        WanIfPPPCfg_Validate
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
WanIfPPPCfg_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    return TRUE;
}

/**********************************************************************
    caller:     owner of this object

    prototype:
        ULONG
        WanIfPPPCfg_Commit
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
WanIfPPPCfg_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
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

            result = DmlSetWanIfCfg( pWanDmlIface->uiInstanceNumber, pWanDmlIface );
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
        ULONG
        WanIfPPPCfg_Rollback
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
WanIfPPPCfg_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return TRUE;
}
