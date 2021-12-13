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
#ifndef  _WANMGR_DML_IFACE_APIS_H_
#define  _WANMGR_DML_IFACE_APIS_H_

#include "ansc_platform.h"

/***********************************************************************

 APIs for Object:

    X_RDK_WanManager.CPEInterface.

    *  WanIf_GetEntryCount
    *  WanIf_GetEntry
    *  WanIf_AddEntry
    *  WanIf_DelEntry
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
ULONG WanIf_GetEntryCount(ANSC_HANDLE);
ANSC_HANDLE WanIf_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber);
ANSC_HANDLE WanIf_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber);
ULONG WanIf_DelEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance);
BOOL WanIf_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);
BOOL WanIf_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);
ULONG WanIf_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);
BOOL WanIf_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);
BOOL WanIf_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool);
BOOL WanIf_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue);
BOOL WanIf_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);
ULONG WanIf_Commit(ANSC_HANDLE hInsContext);
ULONG WanIf_Rollback(ANSC_HANDLE hInsContext);


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
BOOL WanIfSelection_GetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int* pInt);
BOOL WanIfSelection_SetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int iValue);
BOOL WanIfSelection_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);
BOOL WanIfSelection_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);
BOOL WanIfSelection_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool);
BOOL WanIfSelection_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue);
BOOL WanIfSelection_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);
ULONG WanIfSelection_Commit(ANSC_HANDLE hInsContext);
ULONG WanIfSelection_Rollback(ANSC_HANDLE hInsContext);


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
ULONG WanIfVirtIf_GetEntryCount(ANSC_HANDLE hInsContext);
ANSC_HANDLE WanIfVirtIf_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber);
ANSC_HANDLE WanIfVirtIf_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber);
ULONG WanIfVirtIf_DelEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance);
BOOL WanIfVirtIf_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);
BOOL WanIfVirtIf_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);
BOOL WanIfVirtIf_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* pBool);
BOOL WanIfVirtIf_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue);
ULONG WanIfVirtIf_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);
BOOL WanIfVirtIf_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);
BOOL WanIfVirtIf_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);
ULONG WanIfVirtIf_Commit(ANSC_HANDLE hInsContext);
ULONG WanIfVirtIf_Rollback(ANSC_HANDLE hInsContext);


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
ULONG WanIfVirtIfVLAN_GetEntryCount(ANSC_HANDLE hInsContext /*, ANSC_HANDLE hInstance*/);
ANSC_HANDLE WanIfVirtIfVLAN_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber);
ANSC_HANDLE WanIfVirtIfVLAN_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber);
ULONG WanIfVirtIfVLAN_DelEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance);
ULONG WanIfVirtIfVLAN_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);
BOOL WanIfVirtIfVLAN_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);
BOOL WanIfVirtIfVLAN_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);
ULONG WanIfVirtIfVLAN_Commit(ANSC_HANDLE hInsContext);
ULONG WanIfVirtIfVLAN_Rollback(ANSC_HANDLE hInsContext);


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
ULONG WanIfVirtIfMarking_GetEntryCount(ANSC_HANDLE hInsContext);
ANSC_HANDLE WanIfVirtIfMarking_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber);
ANSC_HANDLE WanIfVirtIfMarking_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber);
ULONG WanIfVirtIfMarking_DelEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance);
ULONG WanIfVirtIfMarking_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);
BOOL WanIfVirtIfMarking_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);
BOOL WanIfVirtIfMarking_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);
ULONG WanIfVirtIfMarking_Commit(ANSC_HANDLE hInsContext);
ULONG WanIfVirtIfMarking_Rollback(ANSC_HANDLE hInsContext);

/***********************************************************************

 APIs for Object:

    Device.X_RDK_WanManager.CPEInterface.{i}.Marking.{i}.

    *  Marking_GetEntryCount
    *  Marking_GetEntry
    *  Marking_AddEntry
    *  Marking_DelEntry
    *  Marking_GetParamUlongValue
    *  Marking_GetParamStringValue
    *  Marking_GetParamIntValue
    *  Marking_SetParamIntValue
    *  Marking_SetParamUlongValue
    *  Marking_SetParamStringValue
    *  Marking_Validate
    *  Marking_Commit
    *  Marking_Rollback

***********************************************************************/

ULONG WanIfMarking_GetEntryCount(ANSC_HANDLE hInsContext);
ANSC_HANDLE WanIfMarking_GetEntry(ANSC_HANDLE hInsContext, ULONG nIndex, ULONG* pInsNumber);
ANSC_HANDLE WanIfMarking_AddEntry(ANSC_HANDLE hInsContext, ULONG* pInsNumber);
ULONG WanIfMarking_DelEntry(ANSC_HANDLE hInsContext, ANSC_HANDLE hInstance);
BOOL WanIfMarking_GetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG* puLong);
ULONG WanIfMarking_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pUlSize);
BOOL WanIfMarking_GetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int* pInt);
BOOL WanIfMarking_SetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int iValue);
BOOL WanIfMarking_SetParamUlongValue(ANSC_HANDLE hInsContext, char* ParamName, ULONG uValue);
BOOL WanIfMarking_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString);
BOOL WanIfMarking_Validate(ANSC_HANDLE hInsContext, char* pReturnParamName, ULONG* puLength);
ULONG WanIfMarking_Commit(ANSC_HANDLE hInsContext);
ULONG WanIfMarking_Rollback(ANSC_HANDLE hInsContext);

#endif /* _WANMGR_DML_IFACE_APIS_H_ */
