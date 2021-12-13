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
#ifndef  _WANMGR_DML_IP_APIS_H_
#define  _WANMGR_DML_IP_APIS_H_

#include "ansc_platform.h"

/***********************************************************************

 APIs for Object:

    Device.X_RDK_WanManager.CPEInterface.{i}.VirtualInterface.{i}.X_RDK_IPv4.

    *  Wan_Xrdk_IPv4_GetParamUlongValue
    *  Wan_Xrdk_IPv4_SetParamUlongValue
    *  Wan_Xrdk_IPv4_GetParamBoolValue
    *  Wan_Xrdk_IPv4_SetParamBoolValue
    *  Wan_Xrdk_IPv4_Validate
    *  Wan_Xrdk_IPv4_Commit

***********************************************************************/
BOOL Wan_Xrdk_IPv4_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);
BOOL Wan_Xrdk_IPv4_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);
BOOL Wan_Xrdk_IPv4_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool);
BOOL Wan_Xrdk_IPv4_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue);
ULONG Wan_Xrdk_IPv4_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);
ULONG Wan_Xrdk_IPv4_Commit(ANSC_HANDLE hInsContext);



/***********************************************************************

 APIs for Object:

    Device.X_RDK_WanManager.CPEInterface.{i}.VirtualInterface.{i}.X_RDK_IPv6.

    *  Wan_Xrdk_IPv6_GetParamUlongValue
    *  Wan_Xrdk_IPv6_SetParamUlongValue
    *  Wan_Xrdk_IPv6_SetParamStringValue
    *  Wan_Xrdk_IPv6_GetParamStringValue
    *  Wan_Xrdk_IPv6_GetParamBoolValue
    *  Wan_Xrdk_IPv6_SetParamBoolValue
    *  Wan_Xrdk_IPv6_Validate
    *  Wan_Xrdk_IPv6_Commit

***********************************************************************/
BOOL Wan_Xrdk_IPv6_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);
BOOL Wan_Xrdk_IPv6_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);
BOOL Wan_Xrdk_IPv6_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);
ULONG Wan_Xrdk_IPv6_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);
BOOL Wan_Xrdk_IPv6_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool);
BOOL Wan_Xrdk_IPv6_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue);
ULONG Wan_Xrdk_IPv6_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);
ULONG Wan_Xrdk_IPv6_Commit(ANSC_HANDLE hInsContext);

#endif //_WANMGR_DML_IP_APIS_H_
