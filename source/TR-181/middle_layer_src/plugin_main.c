/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2017 RDK Management
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
  
    module: plugin_main.c

        Implement COSA Data Model Library Init and Unload apis.
 
    ---------------------------------------------------------------

    author:

        COSA XML TOOL CODE GENERATOR 1.0

    ---------------------------------------------------------------

    revision:

        09/28/2011    initial revision.

**********************************************************************/

#include "ansc_platform.h"
#include "ansc_load_library.h"
#include "cosa_plugin_api.h"
#include "plugin_main.h"
#include "plugin_main_apis.h"
#include "wan_plugin_apis.h"
#include "ccsp_psm_helper.h"
#include "wan_mgr_internal.h"
#include "wan_interface_dml.h"
#include "dmsb_tr181_psm_definitions.h"

#include <sysevent/sysevent.h>
#include <pthread.h>
#include <syscfg/syscfg.h>

PBACKEND_MANAGER_OBJECT g_pBEManager;
void *                       g_pDslhDmlAgent;
extern ANSC_HANDLE     g_MessageBusHandle_Irep;
extern char            g_SubSysPrefix_Irep[32];
extern COSARepopulateTableProc            g_COSARepopulateTable;
extern ANSC_HANDLE bus_handle;

#define THIS_PLUGIN_VERSION                         1
#ifdef _HUB4_PRODUCT_REQ_
static int sysevent_fd;
static token_t sysevent_token;
static pthread_t sysevent_tid;

#define BUFLEN_42 42
#define SYSEVENT_ULA_ENABLE            "lan_ula_enable"
#define SYSEVENT_IPV6_ENABLE           "lan_ipv6_enable"
#define STARTED     "started"
#define UP "up"
#define WANMANAGER_SYSNAME     "wanmanager"
#define SYS_IP_ADDR     "127.0.0.1"
#define SYSEVENT_ULA_ADDRESS "lan_ula_address"

#define ENABLE          1
#define DISABLE         0

static int WanManagerSyseventInit()
{
    sysevent_fd =  sysevent_open(SYS_IP_ADDR, SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, WANMANAGER_SYSNAME, &sysevent_token);
    if (sysevent_fd < 0)
        return -1;
    return 0;
}

static void WanManagerSyseventClose()
{
    if (0 <= sysevent_fd)
    {
        sysevent_close(sysevent_fd, sysevent_token);
    }
}

static int SetDataModelParameter(parameterValStruct_t *value) {
    CCSP_MESSAGE_BUS_INFO *bus_info           = (CCSP_MESSAGE_BUS_INFO *)bus_handle;
    char                 pComponentName[ 64 ] = "eRT.com.cisco.spvtg.ccsp.pam";
    char                 pComponentPath[ 64 ] = "/com/cisco/spvtg/ccsp/pam";
    char                *faultParam           = NULL;
    int                  ret                  = 0;

    ret = CcspBaseIf_setParameterValues
            (
                bus_handle,
                pComponentName,
                pComponentPath,
                0,
                0x0,   /* session id and write id */
                value,
                1,
                TRUE,   /* Commit  */
                &faultParam
            );

    if ( ( ret != CCSP_SUCCESS ) && \
         ( faultParam )
        )
    {
        AnscTraceFlow(("%s CcspBaseIf_setParameterValues failed for param %s\n",__FUNCTION__,faultParam ) );
        bus_info->freefunc( faultParam );
        return ANSC_STATUS_FAILURE;
    }

    return ANSC_STATUS_SUCCESS;
}

static void *WanManagerSyseventHandler(void *data)
{
    async_id_t wanamangr_status_asyncid;
    async_id_t lan_ula_address_event_asyncid;
    async_id_t lan_ula_enable_asyncid;
    async_id_t lan_ipv6_enable_asyncid;

    sysevent_set_options(sysevent_fd, sysevent_token, SYSEVENT_ULA_ADDRESS, TUPLE_FLAG_EVENT);
    sysevent_setnotification(sysevent_fd, sysevent_token, SYSEVENT_ULA_ADDRESS, &lan_ula_address_event_asyncid);

    sysevent_set_options(sysevent_fd, sysevent_token, SYSEVENT_ULA_ENABLE, TUPLE_FLAG_EVENT);
    sysevent_setnotification(sysevent_fd, sysevent_token, SYSEVENT_ULA_ENABLE, &lan_ula_enable_asyncid);

    sysevent_set_options(sysevent_fd, sysevent_token, SYSEVENT_IPV6_ENABLE, TUPLE_FLAG_EVENT);
    sysevent_setnotification(sysevent_fd, sysevent_token, SYSEVENT_IPV6_ENABLE, &lan_ipv6_enable_asyncid);

    for(;;)
    {
        char name[BUFLEN_42] = {0};
        char val[BUFLEN_42] = {0};
        int namelen = sizeof(name);
        int vallen  = sizeof(val);
        async_id_t getnotification_asyncid;
        int err = 0;
        int result = 0;
        char *datamodel_value = NULL;

        err = sysevent_getnotification(sysevent_fd, sysevent_token, name, &namelen,  val, &vallen, &getnotification_asyncid);

        if(err)
        {
            AnscTraceFlow(("sysevent_getnotification failed with error: %d\n", err));
            sleep(2);
        }
        else
        {
            if ( strcmp(name, SYSEVENT_ULA_ADDRESS) == 0 )
            {
                AnscTraceFlow(("received notification event %s:%s\n",name, val ));

                datamodel_value = (char *) malloc(sizeof(char) * 256);
                if(datamodel_value != NULL)
                {
                    memset(datamodel_value, 0, 256);
                    strncpy(datamodel_value, val, sizeof(val));

                    parameterValStruct_t dns_enable = { "Device.DHCPv6.Server.Pool.1.X_RDKCENTRAL-COM_DNSServersEnabled", "true", ccsp_boolean };
                    parameterValStruct_t dns_server = { "Device.DHCPv6.Server.Pool.1.X_RDKCENTRAL-COM_DNSServers", datamodel_value, ccsp_string };

                    result = SetDataModelParameter(&dns_enable);
                    if(result == ANSC_STATUS_SUCCESS) {
                        result = SetDataModelParameter(&dns_server);
                        if(result != ANSC_STATUS_SUCCESS) {
                            AnscTraceFlow(("SetDataModelParameter() failed for X_RDKCENTRAL-COM_DNSServers parameter\n"));
                        }
                    }
                    else {
                        AnscTraceFlow(("SetDataModelParameter() failed for X_RDKCENTRAL-COM_DNSServersEnabled parameter\n"));
                    }

                    free(datamodel_value);
                }
            }
            else if ( strcmp(name, SYSEVENT_ULA_ENABLE) == 0 ) {

                AnscTraceFlow(("received notification event %s:%s\n",name, val ));

                datamodel_value = (char *) malloc(sizeof(char) * 256);
                if(datamodel_value != NULL)
                {
                    memset(datamodel_value, 0, 256);
                    strncpy(datamodel_value, val, sizeof(val));
                    parameterValStruct_t dns_enable = { "Device.DHCPv6.Server.Pool.1.X_RDKCENTRAL-COM_DNSServersEnabled", datamodel_value, ccsp_boolean };
                    if(SetDataModelParameter(&dns_enable) != ANSC_STATUS_SUCCESS)
                    {
                        AnscTraceFlow(("SetDataModelParameter failed on dns_enable request"));
                    }
                }

                free(datamodel_value);
            }
            else if ( strcmp(name, SYSEVENT_IPV6_ENABLE) == 0 ) {

                AnscTraceFlow(("received notification for lan_ipv6_enable val = %s", val));

                datamodel_value = (char *) malloc(sizeof(char) * 256);
                if(datamodel_value != NULL)
                {
                    memset(datamodel_value, 0, 256);
                    strncpy(datamodel_value, val, sizeof(val));
                    parameterValStruct_t ipv6_enable = { "Device.DHCPv6.Server.Pool.1.Enable", datamodel_value, ccsp_boolean };
                    if(SetDataModelParameter(&ipv6_enable) != ANSC_STATUS_SUCCESS)
                    {
                        AnscTraceFlow(("SetDataModelParameter failed on ipv6_enable request"));
                    }

                    free(datamodel_value);
                    system("sysevent set zebra-restart");
                }
            }
            else
                AnscTraceFlow(("undefined event %s:%s\n",name, val));
        }
    }

    AnscTraceFlow(("WanManagerSyseventHandler Exit\n"));
    return 0;
}

static ANSC_STATUS DmlWanMsgHandler()
{
    WanManagerSyseventInit();

    if(pthread_create(&sysevent_tid, NULL, WanManagerSyseventHandler, NULL) == 0) {
        AnscTraceFlow(("DmlWanMsgHandler -- pthread_create successfully.\n"));
    }
    else {
        AnscTraceFlow(("DmlWanMsgHandler -- pthread_create FAILED.\n"));
    }

    return 0;
}
#endif //_HUB4_PRODUCT_REQ_

int ANSC_EXPORT_API
WanManagerDmlInit
    (
        ULONG                       uMaxVersionSupported, 
        void*                       hCosaPlugInfo         /* PCOSA_PLUGIN_INFO passed in by the caller */
    )
{
    PCOSA_PLUGIN_INFO               pPlugInfo                   = (PCOSA_PLUGIN_INFO                 )hCosaPlugInfo;
    COSAGetParamValueByPathNameProc pGetParamValueByPathNameProc = (COSAGetParamValueByPathNameProc)NULL;
    COSASetParamValueByPathNameProc pSetParamValueByPathNameProc = (COSASetParamValueByPathNameProc)NULL;
    COSAGetParamValueStringProc     pGetStringProc              = (COSAGetParamValueStringProc       )NULL;
    COSAGetParamValueUlongProc      pGetParamValueUlongProc     = (COSAGetParamValueUlongProc        )NULL;
    COSAGetParamValueIntProc        pGetParamValueIntProc       = (COSAGetParamValueIntProc          )NULL;
    COSAGetParamValueBoolProc       pGetParamValueBoolProc      = (COSAGetParamValueBoolProc         )NULL;
    COSASetParamValueStringProc     pSetStringProc              = (COSASetParamValueStringProc       )NULL;
    COSASetParamValueUlongProc      pSetParamValueUlongProc     = (COSASetParamValueUlongProc        )NULL;
    COSASetParamValueIntProc        pSetParamValueIntProc       = (COSASetParamValueIntProc          )NULL;
    COSASetParamValueBoolProc       pSetParamValueBoolProc      = (COSASetParamValueBoolProc         )NULL;
    COSAGetInstanceNumbersProc      pGetInstanceNumbersProc     = (COSAGetInstanceNumbersProc        )NULL;

    COSAGetCommonHandleProc         pGetCHProc                  = (COSAGetCommonHandleProc           )NULL;
    COSAValidateHierarchyInterfaceProc
                                    pValInterfaceProc           = (COSAValidateHierarchyInterfaceProc)NULL;
    COSAGetHandleProc               pGetRegistryRootFolder      = (COSAGetHandleProc                 )NULL;
    COSAGetInstanceNumberByIndexProc
                                    pGetInsNumberByIndexProc    = (COSAGetInstanceNumberByIndexProc  )NULL;
    COSAGetHandleProc               pGetMessageBusHandleProc    = (COSAGetHandleProc                 )NULL;
    COSAGetInterfaceByNameProc      pGetInterfaceByNameProc     = (COSAGetInterfaceByNameProc        )NULL;
    ULONG                           ret                         = 0;

    if ( uMaxVersionSupported < THIS_PLUGIN_VERSION )
    {
      /* this version is not supported */
        return -1;
    }

    pPlugInfo->uPluginVersion       = THIS_PLUGIN_VERSION;
    g_pDslhDmlAgent                 = pPlugInfo->hDmlAgent;

/*
    pGetCHProc = (COSAGetCommonHandleProc)pPlugInfo->AcquireFunction("COSAGetDiagPluginInfo");

    if( pGetCHProc != NULL)
    {
        g_pCosaDiagPluginInfo = pGetCHProc(NULL);
    }
    else
    {
        goto EXIT;
    }
*/
    pGetParamValueByPathNameProc = (COSAGetParamValueByPathNameProc)pPlugInfo->AcquireFunction("COSAGetParamValueByPathName");

    if( pGetParamValueByPathNameProc != NULL)
    {
        g_GetParamValueByPathNameProc = pGetParamValueByPathNameProc;
    }
    else
    {
        goto EXIT;
    }

    pSetParamValueByPathNameProc = (COSASetParamValueByPathNameProc)pPlugInfo->AcquireFunction("COSASetParamValueByPathName");

    if( pSetParamValueByPathNameProc != NULL)
    {
        g_SetParamValueByPathNameProc = pSetParamValueByPathNameProc;
    }
    else
    {
        goto EXIT;
    }

    pGetStringProc = (COSAGetParamValueStringProc)pPlugInfo->AcquireFunction("COSAGetParamValueString");

    if( pGetStringProc != NULL)
    {
        g_GetParamValueString = pGetStringProc;
    }
    else
    {
        goto EXIT;
    }

    pGetParamValueUlongProc = (COSAGetParamValueUlongProc)pPlugInfo->AcquireFunction("COSAGetParamValueUlong");

    if( pGetParamValueUlongProc != NULL)
    {
        g_GetParamValueUlong = pGetParamValueUlongProc;
    }
    else
    {
        goto EXIT;
    }


    pGetParamValueIntProc = (COSAGetParamValueUlongProc)pPlugInfo->AcquireFunction("COSAGetParamValueInt");

    if( pGetParamValueIntProc != NULL)
    {
        g_GetParamValueInt = pGetParamValueIntProc;
    }
    else
    {
        goto EXIT;
    }

    pGetParamValueBoolProc = (COSAGetParamValueBoolProc)pPlugInfo->AcquireFunction("COSAGetParamValueBool");

    if( pGetParamValueBoolProc != NULL)
    {
        g_GetParamValueBool = pGetParamValueBoolProc;
    }
    else
    {
        goto EXIT;
    }

    pSetStringProc = (COSASetParamValueStringProc)pPlugInfo->AcquireFunction("COSASetParamValueString");

    if( pSetStringProc != NULL)
    {
        g_SetParamValueString = pSetStringProc;
    }
    else
    {
        goto EXIT;
    }

    pSetParamValueUlongProc = (COSASetParamValueUlongProc)pPlugInfo->AcquireFunction("COSASetParamValueUlong");

    if( pSetParamValueUlongProc != NULL)
    {
        g_SetParamValueUlong = pSetParamValueUlongProc;
    }
    else
    {
        goto EXIT;
    }


    pSetParamValueIntProc = (COSASetParamValueIntProc)pPlugInfo->AcquireFunction("COSASetParamValueInt");

    if( pSetParamValueIntProc != NULL)
    {
        g_SetParamValueInt = pSetParamValueIntProc;
    }
    else
    {
        goto EXIT;
    }

    pSetParamValueBoolProc = (COSASetParamValueBoolProc)pPlugInfo->AcquireFunction("COSASetParamValueBool");

    if( pSetParamValueBoolProc != NULL)
    {
        g_SetParamValueBool = pSetParamValueBoolProc;
    }
    else
    {
        goto EXIT;
    }

    pGetInstanceNumbersProc = (COSAGetInstanceNumbersProc)pPlugInfo->AcquireFunction("COSAGetInstanceNumbers");

    if( pGetInstanceNumbersProc != NULL)
    {
        g_GetInstanceNumbers = pGetInstanceNumbersProc;
    }
    else
    {
        goto EXIT;
    }

    pValInterfaceProc = (COSAValidateHierarchyInterfaceProc)pPlugInfo->AcquireFunction("COSAValidateHierarchyInterface");

    if ( pValInterfaceProc )
    {
        g_ValidateInterface = pValInterfaceProc;
    }
    else
    {
        goto EXIT;
    }
/*
#ifndef _ANSC_WINDOWSNT
#ifdef _SOFTWAREMODULES_SUPPORT_NAF
    CosaSoftwareModulesInit(hCosaPlugInfo);
#endif
#endif
*/
    pGetRegistryRootFolder = (COSAGetHandleProc)pPlugInfo->AcquireFunction("COSAGetRegistryRootFolder");

    if ( pGetRegistryRootFolder != NULL )
    {
        g_GetRegistryRootFolder = pGetRegistryRootFolder;
    }
    else
    {
        printf("!!! haha, catcha !!!\n");
        goto EXIT;
    }

    pGetInsNumberByIndexProc = (COSAGetInstanceNumberByIndexProc)pPlugInfo->AcquireFunction("COSAGetInstanceNumberByIndex");

    if ( pGetInsNumberByIndexProc != NULL )
    {
        g_GetInstanceNumberByIndex = pGetInsNumberByIndexProc;
    }
    else
    {
        goto EXIT;
    }

    pGetInterfaceByNameProc = (COSAGetInterfaceByNameProc)pPlugInfo->AcquireFunction("COSAGetInterfaceByName");

    if ( pGetInterfaceByNameProc != NULL )
    {
        g_GetInterfaceByName = pGetInterfaceByNameProc;
    }
    else
    {
        goto EXIT;
    }

    g_pPnmCcdIf = g_GetInterfaceByName(g_pDslhDmlAgent, CCSP_CCD_INTERFACE_NAME);

    if ( !g_pPnmCcdIf )
    {
        CcspTraceError(("g_pPnmCcdIf is NULL !\n"));

        goto EXIT;
    }

    g_RegisterCallBackAfterInitDml = (COSARegisterCallBackAfterInitDmlProc)pPlugInfo->AcquireFunction("COSARegisterCallBackAfterInitDml");

    if ( !g_RegisterCallBackAfterInitDml )
    {
        goto EXIT;
    }

    g_COSARepopulateTable = (COSARepopulateTableProc)pPlugInfo->AcquireFunction("COSARepopulateTable");

    if ( !g_COSARepopulateTable )
    {
        goto EXIT;
    }

    /* Get Message Bus Handle */
    g_GetMessageBusHandle = (PFN_CCSPCCDM_APPLY_CHANGES)pPlugInfo->AcquireFunction("COSAGetMessageBusHandle");
    if ( g_GetMessageBusHandle == NULL )
    {
        goto EXIT;
    }

    g_MessageBusHandle = (ANSC_HANDLE)g_GetMessageBusHandle(g_pDslhDmlAgent);
    if ( g_MessageBusHandle == NULL )
    {
        goto EXIT;
    }
    g_MessageBusHandle_Irep = g_MessageBusHandle;

    /* Get Subsystem prefix */
    g_GetSubsystemPrefix = (COSAGetSubsystemPrefixProc)pPlugInfo->AcquireFunction("COSAGetSubsystemPrefix");
    if ( g_GetSubsystemPrefix != NULL )
    {
        char*   tmpSubsystemPrefix;

        if ( tmpSubsystemPrefix = g_GetSubsystemPrefix(g_pDslhDmlAgent) )
        {
            AnscCopyString(g_SubSysPrefix_Irep, tmpSubsystemPrefix);
        }

        /* retrieve the subsystem prefix */
        g_SubsystemPrefix = g_GetSubsystemPrefix(g_pDslhDmlAgent);
    }

    /* register the back-end apis for the data model */
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanManager_GetParamUlongValue",  WanManager_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanManager_SetParamUlongValue",  WanManager_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanManager_GetParamBoolValue",  WanManager_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanManager_SetParamBoolValue",  WanManager_SetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanManager_GetParamStringValue", WanManager_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanManager_Commit",  WanManager_Commit);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIf_GetEntryCount", WanIf_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIf_GetEntry", WanIf_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIf_GetParamStringValue", WanIf_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIf_SetParamStringValue", WanIf_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIf_Validate", WanIf_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIf_Commit", WanIf_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIf_Rollback", WanIf_Rollback);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPhy_GetParamStringValue", WanIfPhy_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPhy_SetParamStringValue", WanIfPhy_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPhy_GetParamUlongValue", WanIfPhy_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPhy_SetParamUlongValue", WanIfPhy_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPhy_Validate", WanIfPhy_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPhy_Commit", WanIfPhy_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPhy_Rollback", WanIfPhy_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfCfg_GetParamUlongValue", WanIfCfg_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfCfg_SetParamUlongValue", WanIfCfg_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfCfg_GetParamIntValue", WanIfCfg_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfCfg_SetParamIntValue", WanIfCfg_SetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfCfg_GetParamBoolValue", WanIfCfg_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfCfg_SetParamBoolValue", WanIfCfg_SetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfCfg_GetParamStringValue", WanIfCfg_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfCfg_SetParamStringValue", WanIfCfg_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfCfg_Validate", WanIfCfg_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfCfg_Commit", WanIfCfg_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfCfg_Rollback", WanIfCfg_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfDynTrigger_GetParamUlongValue", WanIfDynTrigger_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfDynTrigger_SetParamUlongValue", WanIfDynTrigger_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfDynTrigger_GetParamBoolValue", WanIfDynTrigger_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfDynTrigger_SetParamBoolValue", WanIfDynTrigger_SetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfDynTrigger_Validate", WanIfDynTrigger_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfDynTrigger_Commit", WanIfDynTrigger_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfDynTrigger_Rollback", WanIfDynTrigger_Rollback);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfIpCfg_GetParamUlongValue", WanIfIpCfg_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfIpCfg_SetParamUlongValue", WanIfIpCfg_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfIpCfg_GetParamStringValue", WanIfIpCfg_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfIpCfg_SetParamStringValue", WanIfIpCfg_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfIpCfg_Validate", WanIfIpCfg_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfIpCfg_Commit", WanIfIpCfg_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfIpCfg_Rollback", WanIfIpCfg_Rollback);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfMapt_GetParamUlongValue", WanIfMapt_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfMapt_SetParamUlongValue", WanIfMapt_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfMapt_GetParamStringValue", WanIfMapt_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfMapt_SetParamStringValue", WanIfMapt_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfMapt_Validate", WanIfMapt_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfMapt_Commit", WanIfMapt_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfMapt_Rollback", WanIfMapt_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfDSLite_GetParamUlongValue", WanIfDSLite_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfDSLite_SetParamUlongValue", WanIfDSLite_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfDSLite_GetParamStringValue", WanIfDSLite_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfDSLite_SetParamStringValue", WanIfDSLite_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfDSLite_Commit", WanIfDSLite_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfDSLite_Rollback", WanIfDSLite_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Marking_GetEntryCount", Marking_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Marking_GetEntry", Marking_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Marking_AddEntry", Marking_AddEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Marking_DelEntry", Marking_DelEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Marking_GetParamUlongValue", Marking_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Marking_GetParamStringValue", Marking_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Marking_GetParamIntValue", Marking_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Marking_SetParamIntValue", Marking_SetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Marking_SetParamUlongValue", Marking_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Marking_SetParamStringValue", Marking_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Marking_Validate", Marking_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Marking_Commit", Marking_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Marking_Rollback", Marking_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfValidation_GetParamBoolValue", WanIfValidation_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfValidation_SetParamBoolValue", WanIfValidation_SetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfValidation_Validate", WanIfValidation_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfValidation_Commit", WanIfValidation_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfValidation_Rollback", WanIfValidation_Rollback);

    /* Create backend framework */
    g_pBEManager = (PBACKEND_MANAGER_OBJECT)BackEndManagerCreate();

    if ( g_pBEManager && g_pBEManager->Initialize )
    {
        g_pBEManager->hCosaPluginInfo = pPlugInfo;

        g_pBEManager->Initialize   ((ANSC_HANDLE)g_pBEManager);
    }

#ifdef _HUB4_PRODUCT_REQ_
    /* handler for 'wan_manager_status' sysevent */
    DmlWanMsgHandler();
#endif //_HUB4_PRODUCT_REQ_
    if(WanManager_Init() != -1) {
        if(WanManager_StartIpcServer() == -1) {
            CcspTraceInfo(("%s %d - IPC Thread failed to start!\n", __FUNCTION__, __LINE__ ));
        }
    }
    else {
        CcspTraceInfo(("%s %d - WanManager failed to initialise!\n", __FUNCTION__, __LINE__ ));
    }
    return  0;

EXIT:
    WanManagerSyseventClose();
    return -1;
}

BOOL ANSC_EXPORT_API
COSA_IsObjectSupported
    (
        char*                        pObjName
    )
{
    
    return TRUE;
}

void ANSC_EXPORT_API
COSA_Unload
    (
        void
    )
{
    ANSC_STATUS                     returnStatus            = ANSC_STATUS_SUCCESS;

    /* unload the memory here */

    returnStatus  =  BackEndManagerRemove(g_pBEManager);
        
    if ( returnStatus == ANSC_STATUS_SUCCESS )
    {
        g_pBEManager = NULL;
    }
    else
    {
        /* print error trace*/
        g_pBEManager = NULL;
    }
}



void ANSC_EXPORT_API
COSA_MemoryCheck
    (
        void
    )
{
    ANSC_STATUS                     returnStatus            = ANSC_STATUS_SUCCESS;
    PCOSA_PLUGIN_INFO               pPlugInfo               = (PCOSA_PLUGIN_INFO)g_pBEManager->hCosaPluginInfo;

    /* unload the memory here */

    returnStatus  =  BackEndManagerRemove(g_pBEManager);

    if ( returnStatus == ANSC_STATUS_SUCCESS )
    {
        g_pBEManager = NULL;
    }
    else
    {
        g_pBEManager = NULL;
    }


    g_pBEManager = (PBACKEND_MANAGER_OBJECT)BackEndManagerCreate();

    if ( g_pBEManager && g_pBEManager->Initialize )
    {
        g_pBEManager->hCosaPluginInfo = pPlugInfo;

        g_pBEManager->Initialize   ((ANSC_HANDLE)g_pBEManager);
    }
}
