/*
   If not stated otherwise in this file or this component's Licenses.txt file the
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

/* ---- Include Files ---------------------------------------- */
#include "dmsb_tr181_psm_definitions.h"
#include "wan_controller_utils.h"
#include "ansc_platform.h"
#include "ccsp_psm_helper.h"
#include "wan_interface_internal.h"

#define DSL_COMPONENT_NAME "eRT.com.cisco.spvtg.ccsp.xdslmanager"
#define DSL_COMPONENT_PATH "/com/cisco/spvtg/ccsp/xdslmanager"
#define ETH_COMPONENT_NAME "eRT.com.cisco.spvtg.ccsp.ethagent"
#define ETH_COMPONENT_PATH "/com/cisco/spvtg/ccsp/ethagent"
#define DSL_UPSTREAM_NAME ".Upstream"
#define ETH_UPSTREAM_NAME ".Upstream"

extern void* g_pDslhDmlAgent;
extern ANSC_HANDLE g_MessageBusHandle;
extern COSAGetSubsystemPrefixProc g_GetSubsystemPrefix;
extern char g_Subsystem[32];
extern ANSC_HANDLE bus_handle;

ANSC_STATUS WanController_getWanPolicy(DML_WAN_POLICY *wan_policy) {
    int result = ANSC_STATUS_SUCCESS;
    int retPsmGet = CCSP_SUCCESS;
    char *param_value= NULL;
    char param_name[256]= {0};

    memset(param_name, 0, sizeof(param_name));
    _ansc_sprintf(param_name, PSM_WANMANAGER_WANPOLICY);
    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, param_name, NULL, &param_value);
    if (retPsmGet == CCSP_SUCCESS && param_value != NULL) {
        *wan_policy = atoi(param_value);
    }
    else {
        result = ANSC_STATUS_FAILURE;
    }

    if (retPsmGet == CCSP_SUCCESS) {
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }

    return result;
}

ANSC_STATUS WanController_updateWanInterfaceUpstreamFlag(PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController, bool flag) {
    char *param_name = NULL;
    char *param_value = NULL;
    char  pComponentName[64] = {0};
    char  pComponentPath[64] = {0};
    char *faultParam = NULL;
    int ret = 0;

    CCSP_MESSAGE_BUS_INFO *bus_info = (CCSP_MESSAGE_BUS_INFO *)bus_handle;
    parameterValStruct_t upstream_param[1] = {0};

    if(pWanController == NULL) {
        CcspTraceInfo(("%s %d Error: pWanController is NULL \n", __FUNCTION__, __LINE__ ));
        return ANSC_STATUS_FAILURE;
    }

    PDML_WAN_IFACE_GLOBAL_CONFIG pCurrentInterface =
    (pWanController->pInterface) + (pWanController->activeInterface);

    param_name = (char *) malloc(sizeof(char) * 256);
    param_value = (char *) malloc(sizeof(char) * 256);

    if(param_name == NULL || param_value == NULL)
    {
        CcspTraceInfo(("%s %d Memory allocation failed \n", __FUNCTION__, __LINE__ ));
        return ANSC_STATUS_FAILURE;
    }

    memset(param_name, 0, 256);
    memset(param_value, 0, 256);

    strncpy(param_name, pCurrentInterface->CfgPhyPath, 256);

    if(strstr(param_name, "DSL") != NULL) { // dsl wan interface
        strncat(param_name, DSL_UPSTREAM_NAME, 256);
        strncpy(pComponentName, DSL_COMPONENT_NAME, sizeof(pComponentName));
        strncpy(pComponentPath, DSL_COMPONENT_PATH, sizeof(pComponentPath));
    }
    else if(strstr(param_name, "Ethernet") != NULL) { // ethernet wan interface
        strncat(param_name, ETH_UPSTREAM_NAME, 256);
        strncpy(pComponentName, ETH_COMPONENT_NAME, sizeof(pComponentName));
        strncpy(pComponentPath, ETH_COMPONENT_PATH, sizeof(pComponentPath));
    }
    if(flag)
        strncpy(param_value, "true", 256);
    else
        strncpy(param_value, "false", 256);

    upstream_param[0].parameterName = param_name;
    upstream_param[0].parameterValue = param_value;
    upstream_param[0].type = ccsp_boolean;

    ret = CcspBaseIf_setParameterValues(bus_handle, pComponentName, pComponentPath,
                                        0, 0x0,   /* session id and write id */
                                        upstream_param, 1, TRUE,   /* Commit  */
                                        &faultParam);

    if (param_name != NULL) {
        free(param_name);
    }

    if (param_value != NULL) {
        free(param_value);
    }

    if ( ( ret != CCSP_SUCCESS ) && ( faultParam )) {
        CcspTraceInfo(("%s CcspBaseIf_setParameterValues failed with error %d\n",__FUNCTION__, ret ));
        bus_info->freefunc( faultParam );
        return ANSC_STATUS_FAILURE;
    }

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS WanController_updateWanActiveLinkFlag(int instance, bool flag) {
    PDATAMODEL_WAN_IFACE    pMyObject  = (PDATAMODEL_WAN_IFACE)g_pBEManager->hWanIface;
    PDML_WAN_IFACE p_Interface = NULL;
    int wan_if_count = 0;
    int iLoopCount = 0;

    if(instance < 0 || pMyObject == NULL) {
        return ANSC_STATUS_FAILURE;
    }

    p_Interface = pMyObject->pWanIface;
    wan_if_count = pMyObject->ulTotalNoofWanInterfaces;

    for( iLoopCount = 0 ; iLoopCount < wan_if_count ; iLoopCount++ ) {
        if (p_Interface[iLoopCount].ulInstanceNumber == (instance + 1)) {
            p_Interface[iLoopCount].CfgActiveLink = flag;
            /**
             * Update global wan data object for the state machine. State machine
             * also needs to check ActiveLink state.
             */
            DmlWanIfSetCfgActiveLink(iLoopCount, flag);
            break;
        }
    }

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS WanController_updateWanStatus(int instance, DML_WAN_IFACE_STATUS status) {
    PDATAMODEL_WAN_IFACE    pMyObject  = (PDATAMODEL_WAN_IFACE)g_pBEManager->hWanIface;
    PDML_WAN_IFACE p_Interface = NULL;
    int wan_if_count = 0;
    int iLoopCount = 0;

    if(instance < 0 || pMyObject == NULL) {
        return ANSC_STATUS_FAILURE;
    }

    p_Interface = pMyObject->pWanIface;
    wan_if_count = pMyObject->ulTotalNoofWanInterfaces;

    for( iLoopCount = 0 ; iLoopCount < wan_if_count ; iLoopCount++ ) {
        if (p_Interface[iLoopCount].ulInstanceNumber == (instance + 1)) {
            p_Interface[iLoopCount].CfgStatus = status;
            /**
             * Update global wan data object for the state machine.
            */
            DmlWanIfSetCfgStatus(iLoopCount, status);
            break;
        }
    }

    return ANSC_STATUS_SUCCESS;
}
