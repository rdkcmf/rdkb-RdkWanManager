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

#include "wanmgr_dml_virtif_ip_apis.h"

#include "wanmgr_data.h"
#include "wanmgr_controller.h"
#include "wanmgr_rdkbus_virtif_apis.h"


/***********************************************************************
APIs for Object:

    Device.X_RDK_WanManager.CPEInterface.{i}.VirtualInterface.{i}.X_RDK_IPv4.

    *  Wan_Xrdk_IPv4_GetParamUlongValue
    *  Wan_Xrdk_IPv4_SetParamUlongValue
    *  Wan_Xrdk_IPv4_GetParamBoolValue
    *  Wan_Xrdk_IPv4_SetParamBoolValue
    *  Wan_Xrdk_IPv4_Validate
    *  Wan_Xrdk_IPv4_Commit
    *  Wan_Xrdk_IPv4_Rollback

***********************************************************************/

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL Wan_Xrdk_IPv4_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);

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
Wan_Xrdk_IPv4_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and return the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;

        if( AnscEqualString(ParamName, "Status", TRUE))
        {
            *puLong = p_VirtIf->Xrdk_IP.IPv4.Status;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "IPAddress", TRUE))
        {
            *puLong = p_VirtIf->Xrdk_IP.IPv4.IPAddress;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "SubnetMask", TRUE))
        {
            *puLong = p_VirtIf->Xrdk_IP.IPv4.SubnetMask;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "AddressType", TRUE))
        {
            *puLong = p_VirtIf->Xrdk_IP.IPv4.AddressType;
            ret = TRUE;
        }
    }

    return ret;
}

/**********************************************************************

   caller:     owner of this object

   prototype:

       BOOL Wan_Xrdk_IPv4_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);

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
Wan_Xrdk_IPv4_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue)
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and set the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;
        if( AnscEqualString(ParamName, "Status", TRUE))
        {
            p_VirtIf->Xrdk_IP.IPv4.Status = uValue;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "IPAddress", TRUE))
        {
            p_VirtIf->Xrdk_IP.IPv4.IPAddress = uValue;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "SubnetMask", TRUE))
        {
            p_VirtIf->Xrdk_IP.IPv4.SubnetMask = uValue;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "AddressType", TRUE))
        {
            p_VirtIf->Xrdk_IP.IPv4.AddressType = uValue;
            ret = TRUE;
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL Wan_Xrdk_IPv4_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool);

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
Wan_Xrdk_IPv4_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool)
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and return the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;
        if( AnscEqualString(ParamName, "Enable", TRUE))
        {
            *pBool = p_VirtIf->Xrdk_IP.IPv4.Enable;
            ret = TRUE;
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

       BOOL Wan_Xrdk_IPv4_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue);

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

BOOL Wan_Xrdk_IPv4_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue)
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and set the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;
        if( AnscEqualString(ParamName, "Enable", TRUE))
        {
            p_VirtIf->Xrdk_IP.IPv4.Enable = bValue;
            ret = TRUE;
        }
    }

    return ret;
}

/**********************************************************************

   caller:     owner of this object

   prototype:

       BOOL Wan_Xrdk_IPv4_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);

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

ULONG Wan_Xrdk_IPv4_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************
   caller:     owner of this object

   prototype:
        ULONG Wan_Xrdk_IPv4_Commit(ANSC_HANDLE hInsContext);

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/

ULONG Wan_Xrdk_IPv4_Commit(ANSC_HANDLE hInsContext)
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    ULONG returnStatus = 0;

    if (pVirtIfCxtLink)
    {
        returnStatus = WanManager_Update_VirtIf_Entry(pVirtIfCxtLink);
    }
    return returnStatus;
}


/***********************************************************************
APIs for Object:

    Device.X_RDK_WanManager.CPEInterface.{i}.VirtualInterface.{i}.X_RDK_IPv4.

    *  Wan_Xrdk_IPv6_GetParamUlongValue
    *  Wan_Xrdk_IPv6_SetParamUlongValue
    *  Wan_Xrdk_IPv6_GetParamBoolValue
    *  Wan_Xrdk_IPv6_SetParamBoolValue
    *  Wan_Xrdk_IPv6_GetParamStringValue
    *  Wan_Xrdk_IPv6_SetParamStringValue
    *  Wan_Xrdk_IPv6_Validate
    *  Wan_Xrdk_IPv6_Commit
    *  Wan_Xrdk_IPv6_Rollback

***********************************************************************/

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL Wan_Xrdk_IPv6_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);

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
Wan_Xrdk_IPv6_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong)
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and return the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;
        if( AnscEqualString(ParamName, "Status", TRUE))
        {
            *puLong = p_VirtIf->Xrdk_IP.IPv6.Status;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "AddressType", TRUE))
        {
            *puLong = p_VirtIf->Xrdk_IP.IPv6.AddressType;
            ret = TRUE;
        }
    }

    return ret;
}

/**********************************************************************

   caller:     owner of this object

   prototype:

       BOOL Wan_Xrdk_IPv6_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);

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
Wan_Xrdk_IPv6_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue)
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and set the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;

        if( AnscEqualString(ParamName, "Status", TRUE))
        {
            p_VirtIf->Xrdk_IP.IPv6.Status = uValue;
            ret = TRUE;
        }

        if( AnscEqualString(ParamName, "AddressType", TRUE))
        {
            p_VirtIf->Xrdk_IP.IPv6.AddressType = uValue;
            ret = TRUE;
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL Wan_Xrdk_IPv6_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool);

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
Wan_Xrdk_IPv6_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool)
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and return the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;

        if( AnscEqualString(ParamName, "Enable", TRUE))
        {
            *pBool = p_VirtIf->Xrdk_IP.IPv6.Enable;
            ret = TRUE;
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

       BOOL Wan_Xrdk_IPv6_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue);

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
BOOL Wan_Xrdk_IPv6_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue)
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and set the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;

        if( AnscEqualString(ParamName, "Enable", TRUE))
        {
            p_VirtIf->Xrdk_IP.IPv6.Enable = bValue;
            ret = TRUE;
        }
    }

    return ret;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG Wan_Xrdk_IPv6_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);

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
ULONG Wan_Xrdk_IPv6_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize)
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;


    /* check the parameter name and return the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;

        if( AnscEqualString(ParamName, "IPAddress", TRUE))
        {
            if ( AnscSizeOfString(p_VirtIf->Xrdk_IP.IPv6.IPAddress) < *pUlSize)
            {
                AnscCopyString(pValue, p_VirtIf->Xrdk_IP.IPv6.IPAddress);
                return 0;
            }
            else
            {
                *pUlSize = AnscSizeOfString(p_VirtIf->Xrdk_IP.IPv6.IPAddress)+1;
                return 1;
            }
        }

        if( AnscEqualString(ParamName, "SubnetMask", TRUE))
        {
            if ( AnscSizeOfString(p_VirtIf->Xrdk_IP.IPv6.SubnetMask) < *pUlSize)
            {
                AnscCopyString(pValue, p_VirtIf->Xrdk_IP.IPv6.SubnetMask);
                return 0;
            }
            else
            {
                *pUlSize = AnscSizeOfString(p_VirtIf->Xrdk_IP.IPv6.SubnetMask)+1;
                return 1;
            }
        }

        if( AnscEqualString(ParamName, "IPPrefix", TRUE))
        {
            if ( AnscSizeOfString(p_VirtIf->Xrdk_IP.IPv6.IPPrefix) < *pUlSize)
            {
                AnscCopyString(pValue, p_VirtIf->Xrdk_IP.IPv6.IPPrefix);
                return 0;
            }
            else
            {
                *pUlSize = AnscSizeOfString(p_VirtIf->Xrdk_IP.IPv6.IPPrefix)+1;
                return 1;
            }
        }
    }
    
    return -1;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL Wan_Xrdk_IPv6_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);

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
BOOL Wan_Xrdk_IPv6_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink  = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    BOOL ret = FALSE;

    /* check the parameter name and set the corresponding value */
    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO* )pVirtIfCxtLink->hContext;

        if( AnscEqualString(ParamName, "IPAddress", TRUE))
        {
            if (AnscSizeOfString(p_VirtIf->Xrdk_IP.IPv6.IPAddress) > 0)
            {
                if (AnscEqualString(p_VirtIf->Xrdk_IP.IPv6.IPAddress, pString, TRUE))
                {
                    return(FALSE);
                }
                memset(p_VirtIf->Xrdk_IP.IPv6.IPAddress, 0, sizeof(p_VirtIf->Xrdk_IP.IPv6.IPAddress));
            }

            if (AnscSizeOfString(pString) < sizeof(p_VirtIf->Xrdk_IP.IPv6.IPAddress))
            {
                AnscCopyString(p_VirtIf->Xrdk_IP.IPv6.IPAddress, pString);
                return(TRUE);
	    }
	    return(FALSE);
        }

        if( AnscEqualString(ParamName, "SubnetMask", TRUE))
        {
            if (AnscSizeOfString(p_VirtIf->Xrdk_IP.IPv6.SubnetMask) > 0)
            {
                if (AnscEqualString(p_VirtIf->Xrdk_IP.IPv6.SubnetMask, pString, TRUE))
                {
                    return(FALSE);
                }
                memset(p_VirtIf->Xrdk_IP.IPv6.SubnetMask, 0, sizeof(p_VirtIf->Xrdk_IP.IPv6.SubnetMask));
            }

            if (AnscSizeOfString(pString) < sizeof(p_VirtIf->Xrdk_IP.IPv6.SubnetMask))
            {
                AnscCopyString(p_VirtIf->Xrdk_IP.IPv6.SubnetMask, pString);
                return(TRUE);
	    }
	    return(FALSE);
        }

        if( AnscEqualString(ParamName, "IPPrefix", TRUE))
        {
            if (AnscSizeOfString(p_VirtIf->Xrdk_IP.IPv6.IPPrefix) > 0)
            {
                if (AnscEqualString(p_VirtIf->Xrdk_IP.IPv6.IPPrefix, pString, TRUE))
                {
                    return(FALSE);
                }
                memset(p_VirtIf->Xrdk_IP.IPv6.IPPrefix, 0, sizeof(p_VirtIf->Xrdk_IP.IPv6.IPPrefix));
            }

            if (AnscSizeOfString(pString) < sizeof(p_VirtIf->Xrdk_IP.IPv6.IPPrefix))
            {
                AnscCopyString(p_VirtIf->Xrdk_IP.IPv6.IPPrefix, pString);
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

       BOOL Wan_Xrdk_IPv6_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);

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

ULONG Wan_Xrdk_IPv6_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength)
{
    return TRUE;
}

/**********************************************************************
   caller:     owner of this object

   prototype:
        ULONG Wan_Xrdk_IPv6_Commit(ANSC_HANDLE hInsContext);

    description:
        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.
**********************************************************************/

ULONG Wan_Xrdk_IPv6_Commit(ANSC_HANDLE hInsContext)
{
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)hInsContext;
    ULONG returnStatus = 0;

    if (pVirtIfCxtLink)
    {
        returnStatus = WanManager_Update_VirtIf_Entry(pVirtIfCxtLink);
    }
    return returnStatus;
}

