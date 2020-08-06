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

#include "ansc_platform.h"
#include "wan_mgr_internal.h"
#include "wan_controller.h"
#include "ccsp_trace.h"
#include "ccsp_syslog.h"


BOOL
WanManager_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    int wan_mode;
    int wan_policy;
    PDATAMODEL_WANMANAGER pMyObject = (PDATAMODEL_WANMANAGER) g_pBEManager->hWA;

    if(AnscEqualString(ParamName, "Policy", TRUE)) {
        *puLong= pMyObject->Policy;
    }
    if(AnscEqualString(ParamName, "IdleTimeout", TRUE)) {
        *puLong= pMyObject->IdleTimeout;
    }

    return TRUE;
}

BOOL
WanManager_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                      uValue
    )
{
    ULONG i = 0;
    PDATAMODEL_WANMANAGER pMyObject = (PDATAMODEL_WANMANAGER) g_pBEManager->hWA;
    DML_WAN_CONFIGURATION config;

    if(AnscEqualString(ParamName, "Policy", TRUE)) {
        config.event = WAN_POLICY_CHANGED;
        config.value = pMyObject->Policy;
        pMyObject->Policy = uValue;
        DmlWanManagerSetWanPolicy((ANSC_HANDLE) pMyObject, uValue);
        WanController_Conf_Changed(config);
    }
    if(AnscEqualString(ParamName, "IdleTimeout", TRUE)) {
        pMyObject->IdleTimeout = uValue;
    }

    return TRUE;
}

BOOL
WanManager_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    unsigned int wan_conn_enable;
    PDATAMODEL_WANMANAGER pMyObject = (PDATAMODEL_WANMANAGER) g_pBEManager->hWA;

    if(AnscEqualString(ParamName, "Enable", TRUE)) {
        *pBool= pMyObject->Enable;
    }

    return TRUE;
}

ULONG
WanManager_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pulSize
    )
{
    return -1; //Not supported parameter.
}

BOOL
WanManager_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    PDATAMODEL_WANMANAGER pMyObject = (PDATAMODEL_WANMANAGER) g_pBEManager->hWA;
    DML_WAN_CONFIGURATION config;

    if(AnscEqualString(ParamName, "Enable", TRUE)) {
        pMyObject->Enable = bValue;
        config.event = WAN_ENABLE;
        config.value = bValue;
        WanController_Conf_Changed(config);
    }

    return TRUE;
}
ULONG
WanManager_Validate
    (
        ANSC_HANDLE                 hInsContext
    )
{
    return TRUE;
}

ULONG
WanManager_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PDATAMODEL_WANMANAGER pMyObject = (PDATAMODEL_WANMANAGER) g_pBEManager->hWA;
    DmlWanManagerCommit((ANSC_HANDLE) pMyObject);
    return TRUE;
}
