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
#include "ccsp_psm_helper.h"
#include "dmsb_tr181_psm_definitions.h"
#include "wanmgr_plugin_main.h"
#include "wanmgr_plugin_main_apis.h"
#include "wanmgr_dml_apis.h"
#include "wanmgr_dml_iface_apis.h"
#include "wanmgr_dml_dhcpv4.h"
#include "wanmgr_dml_dhcpv6.h"

void *                  g_pDslhDmlAgent;
extern ANSC_HANDLE      g_MessageBusHandle_Irep;
extern char             g_SubSysPrefix_Irep[32];
extern COSARepopulateTableProc            g_COSARepopulateTable;

#define THIS_PLUGIN_VERSION  1


int ANSC_EXPORT_API WanManagerDmlInit(ULONG uMaxVersionSupported, void* hCosaPlugInfo)
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


    pGetParamValueIntProc = (COSAGetParamValueIntProc)pPlugInfo->AcquireFunction("COSAGetParamValueInt");

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

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPPPCfg_GetParamUlongValue", WanIfPPPCfg_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPPPCfg_SetParamUlongValue", WanIfPPPCfg_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPPPCfg_GetParamStringValue", WanIfPPPCfg_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPPPCfg_SetParamStringValue", WanIfPPPCfg_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPPPCfg_GetParamBoolValue", WanIfPPPCfg_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPPPCfg_SetParamBoolValue", WanIfPPPCfg_SetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPPPCfg_Validate", WanIfPPPCfg_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPPPCfg_Rollback", WanIfPPPCfg_Rollback);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "WanIfPPPCfg_Commit", WanIfPPPCfg_Commit);

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

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "DHCPv6_GetParamBoolValue", DHCPv6_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "DHCPv6_GetParamIntValue", DHCPv6_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "DHCPv6_GetParamUlongValue", DHCPv6_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "DHCPv6_GetParamStringValue", DHCPv6_GetParamStringValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client3_GetEntryCount", Client3_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client3_GetEntry", Client3_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client3_AddEntry", Client3_AddEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client3_DelEntry", Client3_DelEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client3_GetParamBoolValue", Client3_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client3_GetParamIntValue", Client3_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client3_GetParamUlongValue", Client3_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client3_GetParamStringValue", Client3_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client3_SetParamBoolValue", Client3_SetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client3_SetParamIntValue", Client3_SetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client3_SetParamUlongValue", Client3_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client3_SetParamStringValue", Client3_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client3_Validate", Client3_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client3_Commit", Client3_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client3_Rollback", Client3_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Server2_GetEntryCount", Server2_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Server2_GetEntry", Server2_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Server2_IsUpdated", Server2_IsUpdated);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Server2_Synchronize", Server2_Synchronize);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Server2_GetParamBoolValue", Server2_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Server2_GetParamIntValue", Server2_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Server2_GetParamUlongValue", Server2_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Server2_GetParamStringValue", Server2_GetParamStringValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption1_GetEntryCount", SentOption1_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption1_GetEntry", SentOption1_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption1_AddEntry", SentOption1_AddEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption1_DelEntry", SentOption1_DelEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption1_GetParamBoolValue", SentOption1_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption1_GetParamIntValue", SentOption1_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption1_GetParamUlongValue", SentOption1_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption1_GetParamStringValue", SentOption1_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption1_SetParamBoolValue", SentOption1_SetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption1_SetParamIntValue", SentOption1_SetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption1_SetParamUlongValue", SentOption1_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption1_SetParamStringValue", SentOption1_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption1_Validate", SentOption1_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption1_Commit", SentOption1_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption1_Rollback", SentOption1_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "DHCPv4_GetParamBoolValue", DHCPv4_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "DHCPv4_GetParamIntValue", DHCPv4_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "DHCPv4_GetParamUlongValue", DHCPv4_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "DHCPv4_GetParamStringValue", DHCPv4_GetParamStringValue);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client_GetEntryCount", Client_GetEntryCount);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client_GetEntry", Client_GetEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client_AddEntry", Client_AddEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client_DelEntry", Client_DelEntry);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client_GetParamBoolValue", Client_GetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client_GetParamIntValue", Client_GetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client_GetParamUlongValue", Client_GetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client_GetParamStringValue", Client_GetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client_SetParamBoolValue", Client_SetParamBoolValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client_SetParamIntValue", Client_SetParamIntValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client_SetParamUlongValue", Client_SetParamUlongValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client_SetParamStringValue", Client_SetParamStringValue);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client_Validate", Client_Validate);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client_Commit", Client_Commit);
    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "Client_Rollback", Client_Rollback);

    pPlugInfo->RegisterFunction(pPlugInfo->hContext, "SentOption_GetEntryCount", SentOption_GetEntryCount);



    /* Create backend framework */
    g_pWanMgrBE = (WANMGR_BACKEND_OBJ*)BackEndManagerCreate();

    if ( g_pWanMgrBE && g_pWanMgrBE->Initialize )
    {
        g_pWanMgrBE->hCosaPluginInfo = pPlugInfo;

        g_pWanMgrBE->Initialize   ((ANSC_HANDLE)g_pWanMgrBE);
    }




    return  0;

EXIT:
    return -1;
}

BOOL ANSC_EXPORT_API COSA_IsObjectSupported(char* pObjName)
{

    return TRUE;
}

void ANSC_EXPORT_API COSA_Unload(void)
{
    ANSC_STATUS                     returnStatus            = ANSC_STATUS_SUCCESS;

    /* unload the memory here */

    returnStatus  =  BackEndManagerRemove(g_pWanMgrBE);

    if ( returnStatus == ANSC_STATUS_SUCCESS )
    {
        g_pWanMgrBE = NULL;
    }
    else
    {
        /* print error trace*/
        g_pWanMgrBE = NULL;
    }
}



void ANSC_EXPORT_API COSA_MemoryCheck(void)
{
    ANSC_STATUS                     returnStatus            = ANSC_STATUS_SUCCESS;
    PCOSA_PLUGIN_INFO               pPlugInfo               = (PCOSA_PLUGIN_INFO)g_pWanMgrBE->hCosaPluginInfo;

    /* unload the memory here */

    returnStatus  =  BackEndManagerRemove(g_pWanMgrBE);

    if ( returnStatus == ANSC_STATUS_SUCCESS )
    {
        g_pWanMgrBE = NULL;
    }
    else
    {
        g_pWanMgrBE = NULL;
    }


    g_pWanMgrBE = (WANMGR_BACKEND_OBJ*)BackEndManagerCreate();

    if ( g_pWanMgrBE && g_pWanMgrBE->Initialize )
    {
        g_pWanMgrBE->hCosaPluginInfo = pPlugInfo;

        g_pWanMgrBE->Initialize   ((ANSC_HANDLE)g_pWanMgrBE);
    }
}
