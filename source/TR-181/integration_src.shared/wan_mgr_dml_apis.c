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

#include "wan_apis.h"
#include "wan_mgr_internal.h"
#include "ccsp_psm_helper.h"
#include "dmsb_tr181_psm_definitions.h"
#include "wan_controller.h"

#define COMMAND_SIZE 50
#define BUFFER_SIZE 8
#define PROG_NAME "WANMANAGER-SHARED"

extern char g_Subsystem[32];
extern ANSC_HANDLE bus_handle;

ANSC_STATUS
DmlWanManagerInit
    (
        ANSC_HANDLE     hDml,
        PANSC_HANDLE    phContext
    )
{
    PDATAMODEL_WANMANAGER pMyObject = (PDATAMODEL_WANMANAGER)phContext;
    unsigned int wan_enable;
    unsigned int wan_policy;
    unsigned int wan_idle_timeout;
    int ret_val = ANSC_STATUS_SUCCESS;
    int retPsmGet = CCSP_SUCCESS;
    char param_name[256] = {0};
    char* param_value = NULL;

    memset(param_name, 0, sizeof(param_name));
    _ansc_sprintf(param_name, PSM_WANMANAGER_WANENABLE);
    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, param_name, NULL, &param_value);
    if (retPsmGet == CCSP_SUCCESS && param_value != NULL) 
        wan_enable = atoi(param_value);
    else
        ret_val = ANSC_STATUS_FAILURE;

    pMyObject->Enable = wan_enable;

    memset(param_name, 0, sizeof(param_name));
    _ansc_sprintf(param_name, PSM_WANMANAGER_WANPOLICY);
    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, param_name, NULL, &param_value);
    if (retPsmGet == CCSP_SUCCESS && param_value != NULL) 
        wan_policy = atoi(param_value);
    else
        ret_val = ANSC_STATUS_FAILURE;

    pMyObject->Policy = wan_policy;

    memset(param_name, 0, sizeof(param_name));
    _ansc_sprintf(param_name, PSM_WANMANAGER_WANIDLETIMEOUT);
    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, param_name, NULL, &param_value);
    if (retPsmGet == CCSP_SUCCESS && param_value != NULL) 
        wan_idle_timeout = atoi(param_value);
    else
        ret_val = ANSC_STATUS_FAILURE;

    pMyObject->IdleTimeout = wan_idle_timeout;   

    if(param_value != NULL)
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);

    return ret_val;
}

ANSC_STATUS DmlWanManagerGetWanConnectionEnable(ANSC_HANDLE hContext, unsigned int *wan_conn_enable)
{
    PDATAMODEL_WANMANAGER pMyObject = (PDATAMODEL_WANMANAGER)hContext;
    *wan_conn_enable = pMyObject->Enable;
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS DmlWanManagerSetWanConnectionEnable(ANSC_HANDLE hContext, unsigned int wan_old_status, unsigned int wan_new_status)
{
    int result = ANSC_STATUS_SUCCESS;
    PDATAMODEL_WANMANAGER pMyObject = (PDATAMODEL_WANMANAGER)hContext;
    pMyObject->Enable = wan_new_status;
    return result;
}

ANSC_STATUS DmlWanManagerSetWanPolicy(ANSC_HANDLE hContext, unsigned int wan_policy)
{
    int result = ANSC_STATUS_SUCCESS;
    int retPsmSet = CCSP_SUCCESS;
    char param_name[256] = {0};
    char param_value[256] = {0};
    PDATAMODEL_WANMANAGER pMyObject = (PDATAMODEL_WANMANAGER)hContext;
    pMyObject->Policy = wan_policy;

    /* Update the wan policy information in PSM */
    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    snprintf(param_value, sizeof(param_value), "%d", wan_policy);
    _ansc_sprintf(param_name, PSM_WANMANAGER_WANPOLICY);

    retPsmSet = PSM_Set_Record_Value2(bus_handle,g_Subsystem, param_name, ccsp_string, param_value);
    if (retPsmSet != CCSP_SUCCESS) {
        AnscTraceError(("%s Error %d writing %s %s\n", __FUNCTION__, retPsmSet, param_name, param_value));
        result = ANSC_STATUS_FAILURE;
    }

    return result;
}

ANSC_STATUS DmlWanManagerCommit(ANSC_HANDLE hContext)
{
    return ANSC_STATUS_SUCCESS;
}
