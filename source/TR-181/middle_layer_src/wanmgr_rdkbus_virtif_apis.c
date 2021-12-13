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

//!!!  This code assumes that all data structures are the SAME in middle-layer APIs and HAL layer APIs
//!!!  So it uses casting from one to the other
#include "wanmgr_rdkbus_virtif_apis.h"
#include "dmsb_tr181_psm_definitions.h"
#include "wanmgr_net_utils.h"
//
#define PSM_ENABLE_STRING_TRUE  "TRUE"
#define PSM_ENABLE_STRING_FALSE  "FALSE"

#define DATA_SKB_MARKING_LOCATION "/tmp/skb_marking.conf"
extern char g_Subsystem[32];
extern ANSC_HANDLE bus_handle;

static int WanManager_Iface_GetParamFromPSM(ULONG instancenum, DML_WAN_IFACE* p_Interface)
{
    int retPsmGet = CCSP_SUCCESS;
    char param_value[256];
    char param_name[512];

    p_Interface->uiInstanceNumber = instancenum;

    CcspTraceWarning(("%s-%d: instancenum=%d \n",__FUNCTION__, __LINE__, instancenum));
    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_NAME, instancenum);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        AnscCopyString(p_Interface->Name, param_value);
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_SEL_ENABLE, instancenum);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0)
        {
             p_Interface->Selection.Enable = TRUE;
        }
        else
        {
             p_Interface->Selection.Enable = FALSE;
        }
    }
    else
    {
        p_Interface->Selection.Enable = FALSE;
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_ACTIVELINK, instancenum);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0)
        {
             p_Interface->Selection.ActiveLink = TRUE;
        }
        else
        {
             p_Interface->Selection.ActiveLink = FALSE;
        }
    }
    else
    {
        p_Interface->Selection.ActiveLink = FALSE;
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_SEL_STATUS, instancenum);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        _ansc_sscanf(param_value, "%d", &(p_Interface->Selection.Status));
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_SEL_PRIORITY, instancenum);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        _ansc_sscanf(param_value, "%d", &(p_Interface->Selection.Priority));
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_SEL_TIMEOUT, instancenum);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        _ansc_sscanf(param_value, "%d", &(p_Interface->Selection.TimeOut));
    } 

    return ANSC_STATUS_SUCCESS;
}

static int WanManager_VirtIf_GetParamFromPSM(DML_VIRTIFACE_INFO *VirtIf)
{
    int retPsmGet = CCSP_SUCCESS;
    char param_value[256];
    char param_name[512];
    ULONG VirtIfInstanceNum = VirtIf->InstanceNumber;
    ULONG WanInstanceNum = VirtIf->ulWANIfInstanceNumber;

     CcspTraceWarning(("%s-%d: VirtIfInstanceNum=%d, WanInstanceNum=%d \n",__FUNCTION__, __LINE__, VirtIfInstanceNum, WanInstanceNum));

     _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_VIRTIF_ENABLE, WanInstanceNum, VirtIfInstanceNum);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0)
        {
             VirtIf->Enable = TRUE;
        }
        else
        {
             VirtIf->Enable = FALSE;
        }
    }
    else
    {
        VirtIf->Enable = FALSE;
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_VIRTIF_NAME, WanInstanceNum, VirtIfInstanceNum);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        AnscCopyString(VirtIf->Name, param_value);
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_VIRTIF_ALIAS, WanInstanceNum, VirtIfInstanceNum);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        AnscCopyString(VirtIf->Alias, param_value);
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_VIRTIF_TIMEOUT, WanInstanceNum, VirtIfInstanceNum);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        _ansc_sscanf(param_value, "%d", &(VirtIf->TimeOut));
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_VIRTIF_WANPROTOCOL, WanInstanceNum, VirtIfInstanceNum);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        _ansc_sscanf(param_value, "%d", &(VirtIf->WanProtocol));
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_XRDK_IPV4_ENABLE, WanInstanceNum, VirtIfInstanceNum);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0)
        {
             VirtIf->Xrdk_IP.IPv4.Enable = TRUE;
        }
        else
        {
             VirtIf->Xrdk_IP.IPv4.Enable = FALSE;
        }
    }
    else
    {
        VirtIf->Xrdk_IP.IPv4.Enable = FALSE;
    }
    
    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_XRDK_IPV4_ADDRTYPE, WanInstanceNum, VirtIfInstanceNum);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        _ansc_sscanf(param_value, "%d", &(VirtIf->Xrdk_IP.IPv4.AddressType));
    }

    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_XRDK_IPV6_ENABLE, WanInstanceNum, VirtIfInstanceNum);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0)
        {
             VirtIf->Xrdk_IP.IPv6.Enable = TRUE;
        }
        else
        {
             VirtIf->Xrdk_IP.IPv6.Enable = FALSE;
        }
    }
    else
    {
        VirtIf->Xrdk_IP.IPv6.Enable = FALSE;
    }
    
    _ansc_memset(param_name, 0, sizeof(param_name));
    _ansc_memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_XRDK_IPV6_ADDRTYPE, WanInstanceNum, VirtIfInstanceNum);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS)
    {
        _ansc_sscanf(param_value, "%d", &(VirtIf->Xrdk_IP.IPv6.AddressType));
    }

    return ANSC_STATUS_SUCCESS;
}

static int WanManager_Iface_SetParamFromPSM(ULONG instancenum, DML_WAN_IFACE* p_Interface)
{
    int retPsmSet = CCSP_SUCCESS;
    char param_name[256] = {0};
    char param_value[256] = {0};

    CcspTraceWarning(("%s-%d: instancenum=%d \n",__FUNCTION__, __LINE__, instancenum));
    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    _ansc_sprintf(param_value, "%d", p_Interface->Name );
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_NAME, instancenum);
    WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    if(p_Interface->Selection.Enable)
    {
        _ansc_sprintf(param_value, "TRUE");
    }
    else
    {
        _ansc_sprintf(param_value, "FALSE");
    }
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_SEL_ENABLE, instancenum);
    WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    _ansc_sprintf(param_value, "%d", p_Interface->Selection.Status );
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_SEL_STATUS, instancenum);
    WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    _ansc_sprintf(param_value, "%d", p_Interface->Selection.Priority );
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_SEL_PRIORITY, instancenum);
    WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    _ansc_sprintf(param_value, "%d", p_Interface->Selection.TimeOut );
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_SEL_TIMEOUT, instancenum);
    WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    return ANSC_STATUS_SUCCESS;
}

static int WanManager_VirtIf_SetParamFromPSM(DML_VIRTIFACE_INFO *VirtIf)
{
    int retPsmSet = CCSP_SUCCESS;
    char param_name[256] = {0};
    char param_value[256] = {0};
    ULONG VirtIfInstanceNum = VirtIf->InstanceNumber;
    ULONG WanInstanceNum = VirtIf->ulWANIfInstanceNumber;

    CcspTraceWarning(("%s-%d: VirtIfInstanceNum=%d, WanInstanceNum=%d \n",__FUNCTION__, __LINE__, VirtIfInstanceNum, WanInstanceNum));
    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    if(VirtIf->Enable)
    {
        _ansc_sprintf(param_value, "TRUE");
    }
    else
    {
        _ansc_sprintf(param_value, "FALSE");
    }
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_VIRTIF_ENABLE, WanInstanceNum, VirtIfInstanceNum);
    WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    _ansc_sprintf(param_value, "%d", VirtIf->Name );
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_VIRTIF_NAME, WanInstanceNum, VirtIfInstanceNum);
    WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    _ansc_sprintf(param_value, "%d", VirtIf->Alias );
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_VIRTIF_ALIAS, WanInstanceNum, VirtIfInstanceNum);
    WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    _ansc_sprintf(param_value, "%d", VirtIf->TimeOut );
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_VIRTIF_TIMEOUT, WanInstanceNum, VirtIfInstanceNum);
    WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    _ansc_sprintf(param_value, "%d", VirtIf->Xrdk_IP.IPv4.Enable );
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_XRDK_IPV4_ENABLE, WanInstanceNum, VirtIfInstanceNum);
    WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    _ansc_sprintf(param_value, "%d", VirtIf->Xrdk_IP.IPv4.AddressType );
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_XRDK_IPV4_ADDRTYPE, WanInstanceNum, VirtIfInstanceNum);
    WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    _ansc_sprintf(param_value, "%d", VirtIf->Xrdk_IP.IPv6.Enable );
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_XRDK_IPV6_ENABLE, WanInstanceNum, VirtIfInstanceNum);
    WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    _ansc_sprintf(param_value, "%d", VirtIf->Xrdk_IP.IPv6.AddressType );
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_XRDK_IPV6_ADDRTYPE, WanInstanceNum, VirtIfInstanceNum);
    WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    return ANSC_STATUS_SUCCESS;
}




/* DmlWanGetPSMRecordValue() */
static int
DmlWanGetPSMRecordValue
    (
         char *pPSMEntry,
         char *pOutputString
    )
{
    int   retPsmGet = CCSP_SUCCESS;
    char  strValue[256]  = {0};

    //Validate buffer
    if( ( NULL == pPSMEntry ) && ( NULL == pOutputString ) )
    {
        CcspTraceError(("%s %d Invalid buffer\n",__FUNCTION__,__LINE__));
        return retPsmGet;
    }

    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB( pPSMEntry, strValue, sizeof(strValue) );
    if ( retPsmGet == CCSP_SUCCESS )
    {
        //Copy till end of the string
        snprintf( pOutputString, strlen( strValue ) + 1, "%s", strValue );
    }

    return retPsmGet;
}

/* DmlWanSetPSMRecordValue() */
static int
DmlWanSetPSMRecordValue
    (
         char *pPSMEntry,
         char *pSetString
    )
{
    int   retPsmGet = CCSP_SUCCESS;

    //Validate buffer
    if( ( NULL == pPSMEntry ) && ( NULL == pSetString ) )
    {
        CcspTraceError(("%s %d Invalid buffer\n",__FUNCTION__,__LINE__));
        return retPsmGet;
    }

    retPsmGet = WanMgr_RdkBus_SetParamValuesToDB(pPSMEntry,pSetString);

    return retPsmGet;
}

/* DmlWanDeletePSMRecordValue() */
static int
DmlWanDeletePSMRecordValue
    (
         char *pPSMEntry
    )
{
    int   retPsmGet = CCSP_SUCCESS;

    //Validate buffer
    if( NULL == pPSMEntry )
    {
        CcspTraceError(("%s %d Invalid buffer\n",__FUNCTION__,__LINE__));
        return retPsmGet;
    }

    retPsmGet = PSM_Del_Record( bus_handle, g_Subsystem, pPSMEntry );

    return retPsmGet;
}


#ifdef FEATURE_802_1P_COS_MARKING

#ifdef _HUB4_PRODUCT_REQ_
static void AddSkbMarkingToConfFile(UINT data_skb_mark)
{
   FILE * fp = fopen(DATA_SKB_MARKING_LOCATION, "w+");
   if (!fp)
   {
      AnscTraceError(("%s Error writing skb mark\n", __FUNCTION__));
   }
   else
   {
      fprintf(fp, "data_skb_marking %d\n",data_skb_mark);
      fclose(fp);
   }
}
#endif

/* * SListPushMarkingEntryByInsNum() */
ANSC_STATUS
SListPushMarkingEntryByInsNum
    (
        PSLIST_HEADER               pListHead,
        PCONTEXT_LINK_OBJECT   pLinkContext
    )
{
    ANSC_STATUS                 returnStatus      = ANSC_STATUS_SUCCESS;
    PCONTEXT_LINK_OBJECT   pLineContextEntry = (PCONTEXT_LINK_OBJECT)NULL;
    PSINGLE_LINK_ENTRY          pSLinkEntry       = (PSINGLE_LINK_ENTRY       )NULL;
    ULONG                       ulIndex           = 0;

    if ( pListHead->Depth == 0 )
    {
        AnscSListPushEntryAtBack(pListHead, &pLinkContext->Linkage);
    }
    else
    {
        pSLinkEntry = AnscSListGetFirstEntry(pListHead);

        for ( ulIndex = 0; ulIndex < pListHead->Depth; ulIndex++ )
        {
            pLineContextEntry = ACCESS_CONTEXT_LINK_OBJECT(pSLinkEntry);
            pSLinkEntry       = AnscSListGetNextEntry(pSLinkEntry);

            if ( pLinkContext->InstanceNumber < pLineContextEntry->InstanceNumber )
            {
                AnscSListPushEntryByIndex(pListHead, &pLinkContext->Linkage, ulIndex);

                return ANSC_STATUS_SUCCESS;
            }
        }

        AnscSListPushEntryAtBack(pListHead, &pLinkContext->Linkage);
    }

    return ANSC_STATUS_SUCCESS;
}


#ifdef FEATURE_802_1P_COS_MARKING

ANSC_HANDLE WanManager_AddIfaceMarking(DML_WAN_IFACE* pWanDmlIface, ULONG* pInsNumber)
{
    DATAMODEL_MARKING*              pDmlMarking     = (DATAMODEL_MARKING*) &(pWanDmlIface->Marking);
    DML_MARKING*                    p_Marking       = NULL;
    CONTEXT_MARKING_LINK_OBJECT*    pMarkingCxtLink = NULL;

    //Verify limit of the marking table
    if( WAN_IF_MARKING_MAX_LIMIT < pDmlMarking->ulNextInstanceNumber )
    {
        CcspTraceError(("%s %d - Failed to add Marking entry due to maximum limit(%d)\n",__FUNCTION__,__LINE__,WAN_IF_MARKING_MAX_LIMIT));
        return NULL;
    }

    p_Marking       = (DML_MARKING*)AnscAllocateMemory(sizeof(DML_MARKING));
    pMarkingCxtLink = (CONTEXT_MARKING_LINK_OBJECT*)AnscAllocateMemory(sizeof(CONTEXT_MARKING_LINK_OBJECT));
    if((p_Marking == NULL) || (pMarkingCxtLink == NULL))
    {
        if( NULL != pMarkingCxtLink )
        {
          AnscFreeMemory(pMarkingCxtLink);
          pMarkingCxtLink = NULL;
        }

        if( NULL != p_Marking )
        {
          AnscFreeMemory(p_Marking);
          p_Marking = NULL;
        }
        return NULL;
    }


    /* now we have this link content */
    pMarkingCxtLink->hContext = (ANSC_HANDLE)p_Marking;
    pMarkingCxtLink->bNew     = TRUE;

    pMarkingCxtLink->InstanceNumber  = pDmlMarking->ulNextInstanceNumber;
    *pInsNumber                      = pDmlMarking->ulNextInstanceNumber;

    //Assign actual instance number
    p_Marking->InstanceNumber = pDmlMarking->ulNextInstanceNumber;

    pDmlMarking->ulNextInstanceNumber++;

    //Assign WAN interface instance for reference
    p_Marking->ulWANIfInstanceNumber = pWanDmlIface->uiInstanceNumber;

    //Initialise all marking members
    memset( p_Marking->Alias, 0, sizeof( p_Marking->Alias ) );
    p_Marking->SKBPort = 0;
    p_Marking->SKBMark = 0;
    p_Marking->EthernetPriorityMark = -1;

    SListPushMarkingEntryByInsNum(&pDmlMarking->MarkingList, (PCONTEXT_LINK_OBJECT)pMarkingCxtLink);

    return (ANSC_HANDLE)pMarkingCxtLink;
}


#endif /* * FEATURE_802_1P_COS_MARKING */


/* DmlWanIfMarkingInit() */
ANSC_STATUS WanMgr_WanIfaceMarkingInit (WanMgr_IfaceCtrl_Data_t* pWanIfaceCtrl)
{
    INT iLoopCount  = 0;

    //Validate received buffer
    if( NULL == pWanIfaceCtrl )
    {
        CcspTraceError(("%s %d - Invalid buffer\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    //Initialise Marking Params
    for( iLoopCount = 0; iLoopCount < pWanIfaceCtrl->ulTotalNumbWanInterfaces; iLoopCount++ )
    {
        WanMgr_Iface_Data_t* pWanIfaceData = WanMgr_GetIfaceData_locked(iLoopCount);
        if(pWanIfaceData != NULL)
        {
            DML_WAN_IFACE*      pWanIface           = &(pWanIfaceData->data);
            DATAMODEL_MARKING*  pDataModelMarking   = &(pWanIface->Marking);
            ULONG                    ulIfInstanceNumber = 0;
            char                     acPSMQuery[128]    = { 0 },
                                     acPSMValue[64]     = { 0 };

            /* Initiation all params */
            AnscSListInitializeHeader( &pDataModelMarking->MarkingList );
            pDataModelMarking->ulNextInstanceNumber     = 1;

            //Interface instance number
            ulIfInstanceNumber = pWanIface->uiInstanceNumber;

            //Query marking list for corresponding interface
            snprintf( acPSMQuery, sizeof( acPSMQuery ), PSM_MARKING_LIST, ulIfInstanceNumber );
            if ( ( CCSP_SUCCESS == DmlWanGetPSMRecordValue( acPSMQuery, acPSMValue ) ) && \
                 ( strlen( acPSMValue ) > 0 ) )
            {
                char acTmpString[64] = { 0 };
                char *token          = NULL;


                //Parse PSM output
                snprintf( acTmpString, sizeof( acTmpString ), acPSMValue );

                //split marking table value
                token = strtok( acTmpString, "-" );

                //check and add
                while ( token != NULL )
                {
                   CONTEXT_MARKING_LINK_OBJECT*         pMarkingCxtLink   = NULL;
                   ULONG                                ulInstanceNumber  = 0;

                   /* Insert into marking table */
                   if( ( NULL != ( pMarkingCxtLink = WanManager_AddIfaceMarking( pWanIface, &ulInstanceNumber ) ) )  &&
                       ( 0 < ulInstanceNumber ) )
                   {
                       DML_MARKING*    p_Marking = ( DML_MARKING* )pMarkingCxtLink->hContext;

                       //Reset this flag during init so set should happen in next time onwards
                       pMarkingCxtLink->bNew  = FALSE;

                       if( NULL != p_Marking )
                       {
                           char acTmpMarkingData[ 32 ] = { 0 };

                           //Stores into tmp buffer
                           snprintf( acTmpMarkingData, sizeof( acTmpMarkingData ), "%s", token );

                           //Get Alias from PSM
                           memset( acPSMQuery, 0, sizeof( acPSMQuery ) );
                           memset( acPSMValue, 0, sizeof( acPSMValue ) );

                           snprintf( acPSMQuery, sizeof( acPSMQuery ), PSM_MARKING_ALIAS, ulIfInstanceNumber, acTmpMarkingData );
                           if ( ( CCSP_SUCCESS == DmlWanGetPSMRecordValue( acPSMQuery, acPSMValue ) ) && \
                                ( strlen( acPSMValue ) > 0 ) )
                           {
                                snprintf( p_Marking->Alias, sizeof( p_Marking->Alias ), "%s", acPSMValue );
                           }

                           //Get SKB Port from PSM
                           memset( acPSMQuery, 0, sizeof( acPSMQuery ) );
                           memset( acPSMValue, 0, sizeof( acPSMValue ) );

                           snprintf( acPSMQuery, sizeof( acPSMQuery ), PSM_MARKING_SKBPORT, ulIfInstanceNumber, acTmpMarkingData );
                           if ( ( CCSP_SUCCESS == DmlWanGetPSMRecordValue( acPSMQuery, acPSMValue ) ) && \
                                ( strlen( acPSMValue ) > 0 ) )
                           {
                                p_Marking->SKBPort = atoi( acPSMValue );

                                //Re-adjust SKB Port if it is not matching with instance number
                                if ( p_Marking->InstanceNumber != p_Marking->SKBPort )
                                {
                                    p_Marking->SKBPort = p_Marking->InstanceNumber;

                                    //Set SKB Port into PSM
                                    memset( acPSMValue, 0, sizeof( acPSMValue ) );

                                    snprintf( acPSMValue, sizeof( acPSMValue ), "%u", p_Marking->SKBPort );
                                    DmlWanSetPSMRecordValue( acPSMQuery, acPSMValue );
                                }
                           }

                           //Get SKB Mark from PSM
                           memset( acPSMQuery, 0, sizeof( acPSMQuery ) );
                           memset( acPSMValue, 0, sizeof( acPSMValue ) );

                           snprintf( acPSMQuery, sizeof( acPSMQuery ), PSM_MARKING_SKBMARK, ulIfInstanceNumber, acTmpMarkingData );
                           if ( ( CCSP_SUCCESS == DmlWanGetPSMRecordValue( acPSMQuery, acPSMValue ) ) && \
                                ( strlen( acPSMValue ) > 0 ) )
                            {
                                /*
                                 * Re-adjust SKB Mark
                                 *
                                 * 0x100000 * InstanceNumber(1,2,3, etc)
                                 * 1048576 is decimal equalent to 0x100000 hexa decimal
                                 */
                                p_Marking->SKBMark = ( p_Marking->InstanceNumber ) * ( 1048576 );

                                //Set SKB Port into PSM
                                memset( acPSMValue, 0, sizeof( acPSMValue ) );

                                snprintf( acPSMValue, sizeof( acPSMValue ), "%u", p_Marking->SKBMark );
                                DmlWanSetPSMRecordValue( acPSMQuery, acPSMValue );
                            }

                            //Get Ethernet Priority Mark from PSM
                            memset( acPSMQuery, 0, sizeof( acPSMQuery ) );
                            memset( acPSMValue, 0, sizeof( acPSMValue ) );

                            snprintf( acPSMQuery, sizeof( acPSMQuery ), PSM_MARKING_ETH_PRIORITY_MASK, ulIfInstanceNumber, acTmpMarkingData );
                            if ( ( CCSP_SUCCESS == DmlWanGetPSMRecordValue( acPSMQuery, acPSMValue ) ) && \
                                    ( strlen( acPSMValue ) > 0 ) )
                            {
                                p_Marking->EthernetPriorityMark = atoi( acPSMValue );
                            }

                            CcspTraceInfo(("%s - Name[%s] Data[%s,%u,%u,%d]\n", __FUNCTION__, acTmpMarkingData, p_Marking->Alias, p_Marking->SKBPort, p_Marking->SKBMark, p_Marking->EthernetPriorityMark));


#ifdef _HUB4_PRODUCT_REQ_
                            /* Adding skb mark to config file if alis is 'DATA', so that udhcpc could use it to mark dhcp packets */
                            if(0 == strncmp(p_Marking->Alias, "DATA", 4))
                            {
                                AddSkbMarkingToConfFile(p_Marking->SKBMark);
                            }
#endif
                       }
                   }

                   token = strtok( NULL, "-" );
                }
            }

            WanMgrDml_GetIfaceData_release(pWanIfaceData);
        }
    }

    return ANSC_STATUS_SUCCESS;
}


ANSC_STATUS
DmlCheckAndProceedMarkingOperations
    (
        ANSC_HANDLE         hContext,
        DML_MARKING*   pMarking,
        DML_WAN_MARKING_DML_OPERATIONS enMarkingOp
    )
{
    char    acPSMQuery[128]    = { 0 },
            acPSMValue[64]     = { 0 };
    ULONG   ulIfInstanceNumber = 0;

    //Validate param
    if ( NULL == pMarking )
    {
        CcspTraceError(("%s %d Invalid Buffer\n", __FUNCTION__,__LINE__));
        return ANSC_STATUS_FAILURE;
    }

    //Find the Marking entry in PSM
    ulIfInstanceNumber = pMarking->ulWANIfInstanceNumber;

    //Query marking list for corresponding interface
    snprintf( acPSMQuery, sizeof( acPSMQuery ), PSM_MARKING_LIST, ulIfInstanceNumber );
    if ( CCSP_SUCCESS == DmlWanGetPSMRecordValue( acPSMQuery, acPSMValue ) )
    {
        char     acTmpString[64]          = { 0 },
                 acFoundMarkingRecord[64] = { 0 };
        char     *token                   = NULL;
        BOOL     IsMarkingRecordFound     = FALSE;

        //Parse PSM output
        snprintf( acTmpString, sizeof( acTmpString ), acPSMValue );

        //split marking table value
        token = strtok( acTmpString, "-" );

        //check and add
        while ( token != NULL )
        {
            if( 0 == strcmp( pMarking->Alias, token ) )
            {
                IsMarkingRecordFound = TRUE;
                snprintf( acFoundMarkingRecord, sizeof( acFoundMarkingRecord ), "%s", token );
                break;
            }

            token = strtok( NULL, "-" );
        }

        /*
         *
         * Note:
         * ----
         * If record found when add then reject that process
         * If record not found when add then needs to create new entry and update fields and LIST
         *
         * If record not found when delete then reject that process
         * If record found when delete then needs to delete all corresponding fields in DB and update LIST
         *
         * If record not found when update then reject that process
         * If record found when update then needs to update fields only not LIST
         *
         */
        switch( enMarkingOp )
        {
             case WAN_MARKING_ADD:
             {
                char    acPSMRecEntry[64],
                        acPSMRecValue[64];

                if( TRUE == IsMarkingRecordFound )
                {
                    CcspTraceError(("%s %d - Failed to add since record(%s) already exists!\n",__FUNCTION__,__LINE__,acFoundMarkingRecord));
                    return ANSC_STATUS_FAILURE;
                }

                //Set LIST into PSM
                memset( acPSMRecEntry, 0, sizeof( acPSMRecEntry ) );
                memset( acPSMRecValue, 0, sizeof( acPSMRecValue ) );

                snprintf( acPSMRecEntry, sizeof( acPSMRecEntry ), PSM_MARKING_LIST, ulIfInstanceNumber );

                //Check whether already LIST is having another MARKING or not.
                if( 0 < strlen( acPSMValue ) )
                {
                    snprintf( acPSMRecValue, sizeof( acPSMRecValue ), "%s-%s", acPSMValue, pMarking->Alias );
                }
                else
                {
                    snprintf( acPSMRecValue, sizeof( acPSMRecValue ), "%s", pMarking->Alias );
                }

                //Check set is proper or not
                if ( CCSP_SUCCESS != DmlWanSetPSMRecordValue( acPSMRecEntry, acPSMRecValue ) )
                {
                    CcspTraceError(("%s %d Failed to set PSM record %s\n", __FUNCTION__,__LINE__,acPSMRecEntry));
                    return ANSC_STATUS_FAILURE;
                }

                //Set Alias into PSM
                memset( acPSMRecEntry, 0, sizeof( acPSMRecEntry ) );
                memset( acPSMRecValue, 0, sizeof( acPSMRecValue ) );

                snprintf( acPSMRecEntry, sizeof( acPSMRecEntry ), PSM_MARKING_ALIAS, ulIfInstanceNumber, pMarking->Alias );
                snprintf( acPSMRecValue, sizeof( acPSMRecValue ), "%s", pMarking->Alias );
                DmlWanSetPSMRecordValue( acPSMRecEntry, acPSMRecValue );

                //Set SKBPort into PSM
                memset( acPSMRecEntry, 0, sizeof( acPSMRecEntry ) );
                memset( acPSMRecValue, 0, sizeof( acPSMRecValue ) );

                snprintf( acPSMRecEntry, sizeof( acPSMRecEntry ), PSM_MARKING_SKBPORT, ulIfInstanceNumber, pMarking->Alias );

                /*
                 * Generate SKB port
                 *
                 * Stores the SKB Port for the entry. This is auto-generated for each entry starting from "1".
                 * Its value matches the instance index.
                 */
                pMarking->SKBPort = pMarking->InstanceNumber;

                snprintf( acPSMRecValue, sizeof( acPSMRecValue ), "%u", pMarking->SKBPort );
                DmlWanSetPSMRecordValue( acPSMRecEntry, acPSMRecValue );

                //Set SKBMark into PSM
                memset( acPSMRecEntry, 0, sizeof( acPSMRecEntry ) );
                memset( acPSMRecValue, 0, sizeof( acPSMRecValue ) );

                snprintf( acPSMRecEntry, sizeof( acPSMRecEntry ), PSM_MARKING_SKBMARK, ulIfInstanceNumber, pMarking->Alias );

                /*
                 * Generate SKB Mark
                 *
                 * Stores the SKB Mark for the entry. This is auto-generated for each entry starting from "0x100000".
                 * Its value increments by "0x100000", so the next would be "0x200000", then "0x300000", etc...
                 *
                 * 0x100000 * InstanceNumber(1,2,3, etc)
                 * 1048576 is decimal equalent to 0x100000 hexa decimal
                 */
                pMarking->SKBMark = ( pMarking->InstanceNumber ) * ( 1048576 );
                snprintf( acPSMRecValue, sizeof( acPSMRecValue ), "%u", pMarking->SKBMark );
                DmlWanSetPSMRecordValue( acPSMRecEntry, acPSMRecValue );

                //Set Ethernet Priority Mark into PSM
                memset( acPSMRecEntry, 0, sizeof( acPSMRecEntry ) );
                memset( acPSMRecValue, 0, sizeof( acPSMRecValue ) );

                snprintf( acPSMRecEntry, sizeof( acPSMRecEntry ), PSM_MARKING_ETH_PRIORITY_MASK, ulIfInstanceNumber, pMarking->Alias );
                snprintf( acPSMRecValue, sizeof( acPSMRecValue ), "%d", pMarking->EthernetPriorityMark );
                DmlWanSetPSMRecordValue( acPSMRecEntry, acPSMRecValue );

                CcspTraceInfo(("%s Marking table(%s) and records added successfully\n",__FUNCTION__,pMarking->Alias));
             }
             break; /* * WAN_MARKING_ADD */

             case WAN_MARKING_UPDATE:
             {
                char    acPSMRecEntry[64],
                        acPSMRecValue[64];

                if( FALSE == IsMarkingRecordFound )
                {
                    CcspTraceError(("%s %d - Failed to update since record(%s) not exists!\n",__FUNCTION__,__LINE__,pMarking->Alias));
                    return ANSC_STATUS_FAILURE;
                }

                //Set Alias into PSM
                memset( acPSMRecEntry, 0, sizeof( acPSMRecEntry ) );
                memset( acPSMRecValue, 0, sizeof( acPSMRecValue ) );

                snprintf( acPSMRecEntry, sizeof( acPSMRecEntry ), PSM_MARKING_ALIAS, ulIfInstanceNumber, pMarking->Alias );
                snprintf( acPSMRecValue, sizeof( acPSMRecValue ), "%s", pMarking->Alias );
                DmlWanSetPSMRecordValue( acPSMRecEntry, acPSMRecValue );

                //Set SKBPort into PSM
                memset( acPSMRecEntry, 0, sizeof( acPSMRecEntry ) );
                memset( acPSMRecValue, 0, sizeof( acPSMRecValue ) );

                snprintf( acPSMRecEntry, sizeof( acPSMRecEntry ), PSM_MARKING_SKBPORT, ulIfInstanceNumber, pMarking->Alias );
                snprintf( acPSMRecValue, sizeof( acPSMRecValue ), "%u", pMarking->SKBPort );
                DmlWanSetPSMRecordValue( acPSMRecEntry, acPSMRecValue );

                //Set SKBMark into PSM
                memset( acPSMRecEntry, 0, sizeof( acPSMRecEntry ) );
                memset( acPSMRecValue, 0, sizeof( acPSMRecValue ) );

                snprintf( acPSMRecEntry, sizeof( acPSMRecEntry ), PSM_MARKING_SKBMARK, ulIfInstanceNumber, pMarking->Alias );
                snprintf( acPSMRecValue, sizeof( acPSMRecValue ), "%u", pMarking->SKBMark );
                DmlWanSetPSMRecordValue( acPSMRecEntry, acPSMRecValue );

                //Set Ethernet Priority Mark into PSM
                memset( acPSMRecEntry, 0, sizeof( acPSMRecEntry ) );
                memset( acPSMRecValue, 0, sizeof( acPSMRecValue ) );

                snprintf( acPSMRecEntry, sizeof( acPSMRecEntry ), PSM_MARKING_ETH_PRIORITY_MASK, ulIfInstanceNumber, pMarking->Alias );
                snprintf( acPSMRecValue, sizeof( acPSMRecValue ), "%d", pMarking->EthernetPriorityMark );
                DmlWanSetPSMRecordValue( acPSMRecEntry, acPSMRecValue );

                CcspTraceInfo(("%s Marking table(%s) and records updated successfully\n",__FUNCTION__,pMarking->Alias));
             }
             break; /* * WAN_MARKING_UPDATE */

             case WAN_MARKING_DELETE:
             {
                char    acPSMRecEntry[64],
                        acPSMRecValue[64],
                        acNewMarkingList[64] = { 0 },
                        acNewTmpString[64]   = { 0 },
                        *tmpToken            = NULL;
                INT     iTotalMarking        = 0;


                if( FALSE == IsMarkingRecordFound )
                {
                    CcspTraceError(("%s %d - Failed to delete since record(%s) not exists!\n",__FUNCTION__,__LINE__,pMarking->Alias));
                    return ANSC_STATUS_FAILURE;
                }

                //Set Alias into PSM
                memset( acPSMRecEntry, 0, sizeof( acPSMRecEntry ) );

                snprintf( acPSMRecEntry, sizeof( acPSMRecEntry ), PSM_MARKING_ALIAS, ulIfInstanceNumber, pMarking->Alias );
                DmlWanDeletePSMRecordValue( acPSMRecEntry );

                //Set SKBPort into PSM
                memset( acPSMRecEntry, 0, sizeof( acPSMRecEntry ) );

                snprintf( acPSMRecEntry, sizeof( acPSMRecEntry ), PSM_MARKING_SKBPORT, ulIfInstanceNumber, pMarking->Alias );
                DmlWanDeletePSMRecordValue( acPSMRecEntry );

                //Set SKBMark into PSM
                memset( acPSMRecEntry, 0, sizeof( acPSMRecEntry ) );

                snprintf( acPSMRecEntry, sizeof( acPSMRecEntry ), PSM_MARKING_SKBMARK, ulIfInstanceNumber, pMarking->Alias );
                DmlWanDeletePSMRecordValue( acPSMRecEntry );

                //Set Ethernet Priority Mark into PSM
                memset( acPSMRecEntry, 0, sizeof( acPSMRecEntry ) );

                snprintf( acPSMRecEntry, sizeof( acPSMRecEntry ), PSM_MARKING_ETH_PRIORITY_MASK, ulIfInstanceNumber, pMarking->Alias );
                DmlWanDeletePSMRecordValue( acPSMRecEntry );

                //Remove entry from LIST

                //Parse PSM output
                snprintf( acNewTmpString, sizeof( acNewTmpString ), acPSMValue );

                //split marking table value
                tmpToken = strtok( acNewTmpString, "-" );

                //check and add
                while ( tmpToken != NULL )
                {
                    //Copy all the values except delete alias
                    if( 0 != strcmp( pMarking->Alias, tmpToken ) )
                    {
                        if( 0 == iTotalMarking )
                        {
                            snprintf( acNewMarkingList, sizeof( acNewMarkingList ), "%s", tmpToken );
                        }
                        else
                        {
                            //Append remaining marking strings
                            strncat( acNewMarkingList, "-", strlen("-") + 1 );
                            strncat( acNewMarkingList, tmpToken, strlen( tmpToken ) + 1 );
                        }

                        iTotalMarking++;
                    }

                    tmpToken = strtok( NULL, "-" );
                }

                //Check whether any marking available or not
                if( iTotalMarking == 0 )
                {
                    snprintf( acPSMRecValue, sizeof( acPSMRecValue ), "%s", "" ); //Copy empty
                }
                else
                {
                    snprintf( acPSMRecValue, sizeof( acPSMRecValue ), "%s", acNewMarkingList ); //Copy new string
                }

                //Set Marking LIST into PSM
                memset( acPSMRecEntry, 0, sizeof( acPSMRecEntry ) );

                snprintf( acPSMRecEntry, sizeof( acPSMRecEntry ), PSM_MARKING_LIST, ulIfInstanceNumber );

                DmlWanSetPSMRecordValue( acPSMRecEntry, acPSMRecValue );

                CcspTraceInfo(("%s Marking table(%s) and records deleted successfully\n",__FUNCTION__,pMarking->Alias));
             }
             break; /* * WAN_MARKING_DELETE */

             default:
             {
                 CcspTraceError(("%s Invalid case\n",__FUNCTION__));
                 return ANSC_STATUS_FAILURE;
             }
        }
    }

    return ANSC_STATUS_SUCCESS;
}

/* * DmlAddMarking() */
ANSC_STATUS
DmlAddMarking
    (
        ANSC_HANDLE         hContext,
        DML_MARKING*   pMarking
    )
{
    ANSC_STATUS                 returnStatus      = ANSC_STATUS_SUCCESS;

    //Validate param
    if ( NULL == pMarking )
    {
        CcspTraceError(("%s %d Invalid Buffer\n", __FUNCTION__,__LINE__));
        return ANSC_STATUS_FAILURE;
    }

    returnStatus = DmlCheckAndProceedMarkingOperations( hContext, pMarking, WAN_MARKING_ADD );

    if( ANSC_STATUS_SUCCESS != returnStatus )
    {
        CcspTraceError(("%s %d - Failed to Add Marking Entry\n",__FUNCTION__,__LINE__));
    }

    return ANSC_STATUS_SUCCESS;
}

/* * DmlDeleteMarking() */
ANSC_STATUS
DmlDeleteMarking
    (
        ANSC_HANDLE         hContext,
        DML_MARKING*   pMarking
    )
{
    ANSC_STATUS                 returnStatus      = ANSC_STATUS_SUCCESS;

    //Validate param
    if ( NULL == pMarking )
    {
        CcspTraceError(("%s %d Invalid Buffer\n", __FUNCTION__,__LINE__));
        return ANSC_STATUS_FAILURE;
    }

    returnStatus = DmlCheckAndProceedMarkingOperations( hContext, pMarking, WAN_MARKING_DELETE );

    if( ANSC_STATUS_SUCCESS != returnStatus )
    {
        CcspTraceError(("%s %d - Failed to Delete Marking Entry\n",__FUNCTION__,__LINE__));
    }

    return returnStatus;
}

/* * DmlSetMarking() */
ANSC_STATUS
DmlSetMarking
    (
        ANSC_HANDLE         hContext,
        DML_MARKING*   pMarking
    )
{
    ANSC_STATUS                 returnStatus      = ANSC_STATUS_SUCCESS;

    //Validate param
    if ( NULL == pMarking )
    {
        CcspTraceError(("%s %d Invalid Buffer\n", __FUNCTION__,__LINE__));
        return ANSC_STATUS_FAILURE;
    }

    returnStatus = DmlCheckAndProceedMarkingOperations( hContext, pMarking, WAN_MARKING_UPDATE );

    if( ANSC_STATUS_SUCCESS != returnStatus )
    {
        CcspTraceError(("%s %d - Failed to Update Marking Entry\n",__FUNCTION__,__LINE__));
    }

    return returnStatus;
}
#endif /* * FEATURE_802_1P_COS_MARKING */

/* DmlGetTotalNoOfWanInterfaces() */
ANSC_STATUS DmlGetTotalNoOfWanInterfaces(int *wan_if_count)
{
    int ret_val = ANSC_STATUS_SUCCESS;
    int retPsmGet = CCSP_SUCCESS;
    char param_value[64] = {0};

    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(PSM_WANMANAGER_WANIFCOUNT,param_value,sizeof(param_value));
    if (retPsmGet != CCSP_SUCCESS) { \
        AnscTraceFlow(("%s Error %d reading %s\n", __FUNCTION__, retPsmGet, PSM_WANMANAGER_WANIFCOUNT));
        ret_val = ANSC_STATUS_FAILURE;
    }
    else if(param_value[0] != '\0') {
        _ansc_sscanf(param_value, "%d", wan_if_count);
    }

    return ret_val;
}

/* DmlGetWanIfCfg() */
ANSC_STATUS DmlGetWanIfCfg( INT LineIndex, DML_WAN_IFACE* pstLineInfo )
{
    return ANSC_STATUS_SUCCESS;
}


/* WanManager_SetWanIfCfg() */
ANSC_STATUS WanManager_SetWanIfCfg( INT LineIndex, DML_WAN_IFACE* pstLineInfo )
{
    int ret_val = ANSC_STATUS_SUCCESS;
    ret_val = WanManager_Iface_SetParamFromPSM(LineIndex, pstLineInfo);
    if(ret_val != ANSC_STATUS_SUCCESS) {
        AnscTraceFlow(("%s Failed!! Error code: %d", __FUNCTION__, ret_val));
    }

    return ret_val;
}

/* WanManager_SetVirtIfCfg() */
ANSC_STATUS WanManager_SetVirtIfCfg(DML_VIRTIFACE_INFO *VirtIf)
{
    int ret_val = ANSC_STATUS_SUCCESS;
    ret_val = WanManager_VirtIf_SetParamFromPSM(VirtIf);
    if(ret_val != ANSC_STATUS_SUCCESS) {
        AnscTraceFlow(("%s Failed!! Error code: %d", __FUNCTION__, ret_val));
    }

    return ret_val;
}

ANSC_STATUS WanMgr_WanIfaceConfInit(WanMgr_IfaceCtrl_Data_t* pWanIfaceCtrl)
{
    if(pWanIfaceCtrl != NULL)
    {
        ANSC_STATUS result;
        UINT        uiTotalIfaces;
        UINT        idx;
	ULONG pInsNumber;
	CONTEXT_VIRTIF_LINK_OBJECT*    pVirtIfCxtLink = NULL;
        DML_VIRTIFACE_INFO*           p_VirtIf       = NULL;

        result = DmlGetTotalNoOfWanInterfaces(&uiTotalIfaces);
        if(result == ANSC_STATUS_FAILURE) {
            return ANSC_STATUS_FAILURE;
        }

        pWanIfaceCtrl->pIface = (WanMgr_Iface_Data_t*) AnscAllocateMemory( sizeof(WanMgr_Iface_Data_t) * uiTotalIfaces);
        if( NULL == pWanIfaceCtrl->pIface )
        {
	    CcspTraceWarning(("%s-%d: Error in Mem Alloc for Iface\n",__FUNCTION__, __LINE__));
            return ANSC_STATUS_FAILURE;
        }

	CcspTraceWarning(("%s-%d: uiTotalIfaces=%d\n",__FUNCTION__, __LINE__, uiTotalIfaces));
        pWanIfaceCtrl->ulTotalNumbWanInterfaces = uiTotalIfaces;

        //Memset all memory
        memset( pWanIfaceCtrl->pIface, 0, ( sizeof(WanMgr_Iface_Data_t) * uiTotalIfaces ) );

        //Get static interface configuration from PSM data store
        for( idx = 0 ; idx < uiTotalIfaces ; idx++ )
        {
            WanMgr_Iface_Data_t*  pIfaceData  = &(pWanIfaceCtrl->pIface[idx]);

	    DML_WAN_IFACE* pWanDmlIface = &(pIfaceData->data);
	    WanMgr_IfaceData_Init(pIfaceData, idx);
	    pWanDmlIface->VirtIf.ulNextInstanceNumber = 1;
            WanManager_Iface_GetParamFromPSM((idx+1), pWanDmlIface);
	    for (int i = 0; i < 3; i++)
	    {
                pVirtIfCxtLink = WanManager_Create_VirtIf_Entry(pWanDmlIface, &pInsNumber);
                CcspTraceWarning(("%s-%d: pInsNumber=%d\n",__FUNCTION__, __LINE__, pInsNumber));

	        p_VirtIf = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;
	        WanManager_VirtIf_GetParamFromPSM(p_VirtIf);
	    }
        }
    }

    return ANSC_STATUS_SUCCESS;
}


static ANSC_STATUS WanMgr_WanConfInit (DML_WANMGR_CONFIG* pWanConfig)
{
    unsigned int wan_enable;
    unsigned int wan_policy;
    unsigned int wan_idle_timeout;
    int ret_val = ANSC_STATUS_SUCCESS;
    int retPsmGet = CCSP_SUCCESS;
    char param_name[256] = {0};
    char param_value[256] = {0};

    memset(param_name, 0, sizeof(param_name));
    memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_WANENABLE);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS && param_value[0] != '\0')
        wan_enable = atoi(param_value);
    else
        ret_val = ANSC_STATUS_FAILURE;

    pWanConfig->Enable = wan_enable;

    memset(param_name, 0, sizeof(param_name));
    memset(param_value, 0, sizeof(param_value));
    _ansc_sprintf(param_name, PSM_WANMANAGER_WANPOLICY);
    retPsmGet = WanMgr_RdkBus_GetParamValuesFromDB(param_name,param_value,sizeof(param_value));
    if (retPsmGet == CCSP_SUCCESS && param_value[0] != '\0')
        wan_policy = atoi(param_value);
    else
        ret_val = ANSC_STATUS_FAILURE;

    pWanConfig->Policy = wan_policy;

    return ret_val;
}


ANSC_STATUS WanMgr_WanConfigInit(void)
{
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;

    //Wan Configuration init
    WanMgr_Config_Data_t* pWanConfigData = WanMgr_GetConfigData_locked();
    if(pWanConfigData != NULL)
    {
        retStatus = WanMgr_WanConfInit(&(pWanConfigData->data));

        WanMgrDml_GetConfigData_release(pWanConfigData);
    }

    if(retStatus != ANSC_STATUS_SUCCESS)
    {
        return retStatus;
    }

    //Wan Interface Configuration init
    retStatus = WanMgr_WanDataInit();
    return retStatus;
}


ANSC_STATUS
SListPushEntryByInsNum
    (
        PSLIST_HEADER               pListHead,
        PCONTEXT_LINK_OBJECT   pContext
    )
{
    ANSC_STATUS                     returnStatus      = ANSC_STATUS_SUCCESS;
    PCONTEXT_LINK_OBJECT       pContextEntry = (PCONTEXT_LINK_OBJECT)NULL;
    PSINGLE_LINK_ENTRY              pSLinkEntry       = (PSINGLE_LINK_ENTRY       )NULL;
    ULONG                           ulIndex           = 0;

    if ( pListHead->Depth == 0 )
    {
        AnscSListPushEntryAtBack(pListHead, &pContext->Linkage);
    }
    else
    {
        pSLinkEntry = AnscSListGetFirstEntry(pListHead);

        for ( ulIndex = 0; ulIndex < pListHead->Depth; ulIndex++ )
        {
            pContextEntry = ACCESS_CONTEXT_LINK_OBJECT(pSLinkEntry);
            pSLinkEntry       = AnscSListGetNextEntry(pSLinkEntry);

            if ( pContext->InstanceNumber < pContextEntry->InstanceNumber )
            {
                AnscSListPushEntryByIndex(pListHead, &pContext->Linkage, ulIndex);

                return ANSC_STATUS_SUCCESS;
            }
        }

        AnscSListPushEntryAtBack(pListHead, &pContext->Linkage);
    }

    return ANSC_STATUS_SUCCESS;
}

PCONTEXT_LINK_OBJECT SListGetEntryByInsNum( PSLIST_HEADER pListHead, ULONG InstanceNumber)
{
    ANSC_STATUS                     returnStatus      = ANSC_STATUS_SUCCESS;
    PCONTEXT_LINK_OBJECT            pContextEntry = (PCONTEXT_LINK_OBJECT)NULL;
    PSINGLE_LINK_ENTRY              pSLinkEntry       = (PSINGLE_LINK_ENTRY       )NULL;
    ULONG                           ulIndex           = 0;

    if ( pListHead->Depth == 0 )
    {
        return NULL;
    }
    else
    {
        pSLinkEntry = AnscSListGetFirstEntry(pListHead);

        for ( ulIndex = 0; ulIndex < pListHead->Depth; ulIndex++ )
        {
            pContextEntry = ACCESS_CONTEXT_LINK_OBJECT(pSLinkEntry);
            pSLinkEntry       = AnscSListGetNextEntry(pSLinkEntry);

            if ( pContextEntry->InstanceNumber == InstanceNumber )
            {
                return pContextEntry;
            }
        }
    }

    return NULL;
}

ANSC_STATUS DmlSetWanActiveLinkInPSMDB( UINT uiInterfaceIdx , bool storeValue ) 
{ 
   int retPsmSet = CCSP_SUCCESS;
   char param_name[256] = {0};
   char param_value[256] = {0};
 
   memset(param_value, 0, sizeof(param_value));
   memset(param_name, 0, sizeof(param_name));
 
   if(storeValue == TRUE)
   {
       _ansc_sprintf(param_value, PSM_ENABLE_STRING_TRUE);
   }
   else
   {
       _ansc_sprintf(param_value, PSM_ENABLE_STRING_FALSE);
   }
   _ansc_sprintf(param_name, PSM_WANMANAGER_IF_ACTIVELINK, (uiInterfaceIdx + 1));
 
   CcspTraceInfo(("%s %d: setting %s = %s\n", __FUNCTION__, __LINE__, param_name, param_value)); 
   if (WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value) != CCSP_SUCCESS) 
   { 
       CcspTraceError(("%s %d: setting %s = %s in PSM failed\n", __FUNCTION__, __LINE__, param_name, param_value)); 
       return ANSC_STATUS_FAILURE; 
   } 

    return ANSC_STATUS_SUCCESS; 
}
 
ANSC_STATUS DmlSetWanSelectionStatusInPSMDB( ULONG instancenum, WANMGR_IFACE_SELECTION Status)
{
    int retPsmSet = CCSP_SUCCESS;
    char param_name[256] = {0};
    char param_value[256] = {0};

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    _ansc_sprintf(param_value, "%d", Status );
    _ansc_sprintf(param_name, PSM_WANMANAGER_IF_SEL_STATUS, (instancenum+1));
    WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS WanController_ClearWanConfigurationsInPSM()
{
    char param_name[256] = {0};
    char param_value[256] = {0};
    UINT        uiTotalIfaces;
    ANSC_STATUS result;

    result = DmlGetTotalNoOfWanInterfaces(&uiTotalIfaces);
    if(result != ANSC_STATUS_SUCCESS) 
    {
        return ANSC_STATUS_FAILURE;
    }

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    for(int instancenum = 1; instancenum <=uiTotalIfaces; instancenum++)
    {
        _ansc_sprintf(param_value, PSM_ENABLE_STRING_FALSE);
        _ansc_sprintf(param_name, PSM_WANMANAGER_IF_ACTIVELINK, (instancenum));
        WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);
    }

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    for(int instancenum = 1; instancenum <=uiTotalIfaces; instancenum++)
    {
	_ansc_sprintf(param_value, "%d", WAN_IFACE_NOT_SELECTED);
        _ansc_sprintf(param_name, PSM_WANMANAGER_IF_SEL_STATUS, (instancenum));
        WanMgr_RdkBus_SetParamValuesToDB(param_name,param_value);
    }
    return ANSC_STATUS_SUCCESS;
}

static void WanManager_VirtIf_Update(DML_VIRTIFACE_INFO *VirtIfDest, DML_VIRTIFACE_INFO *VirtIfSrc)
{
    VirtIfDest->Enable = VirtIfSrc->Enable;
    memcpy(VirtIfDest->Name, VirtIfSrc->Name, BUFLEN_64);
    memcpy(VirtIfDest->Alias, VirtIfSrc->Alias, BUFLEN_64);
    VirtIfDest->TimeOut = VirtIfSrc->TimeOut;
    VirtIfDest->WanProtocol = VirtIfSrc->WanProtocol;
    memcpy(VirtIfDest->PPPInterface, VirtIfSrc->PPPInterface, BUFLEN_256);
    memcpy(VirtIfDest->IPInterface, VirtIfSrc->IPInterface, BUFLEN_256);
    VirtIfDest->VLANInUse = VirtIfSrc->VLANInUse;

    VirtIfDest->Xrdk_IP.IPv4.Enable = VirtIfSrc->Xrdk_IP.IPv4.Enable;
    VirtIfDest->Xrdk_IP.IPv4.Status = VirtIfSrc->Xrdk_IP.IPv4.Status;

    VirtIfDest->Xrdk_IP.IPv4.IPAddress = VirtIfSrc->Xrdk_IP.IPv4.IPAddress;
    VirtIfDest->Xrdk_IP.IPv4.SubnetMask = VirtIfSrc->Xrdk_IP.IPv4.SubnetMask;
    VirtIfDest->Xrdk_IP.IPv4.AddressType = VirtIfSrc->Xrdk_IP.IPv4.AddressType;

    VirtIfDest->Xrdk_IP.IPv6.Enable = VirtIfSrc->Xrdk_IP.IPv6.Enable;
    VirtIfDest->Xrdk_IP.IPv6.Status = VirtIfSrc->Xrdk_IP.IPv6.Status;
    memcpy(VirtIfDest->Xrdk_IP.IPv6.IPAddress, VirtIfSrc->Xrdk_IP.IPv6.IPAddress, BUFLEN_64);
    memcpy(VirtIfDest->Xrdk_IP.IPv6.SubnetMask, VirtIfSrc->Xrdk_IP.IPv6.SubnetMask, BUFLEN_64);
    memcpy(VirtIfDest->Xrdk_IP.IPv6.IPPrefix, VirtIfSrc->Xrdk_IP.IPv6.IPPrefix, BUFLEN_64);
    VirtIfDest->Xrdk_IP.IPv6.AddressType = VirtIfSrc->Xrdk_IP.IPv6.AddressType;
}

ANSC_HANDLE WanManager_Create_VirtIf_Entry(DML_WAN_IFACE* pWanDmlIface, ULONG* pInsNumber)
{
    DATAMODEL_VIRTIF*              pDmlVirtIf     = (DATAMODEL_VIRTIF*) &(pWanDmlIface->VirtIf);
    DML_VIRTIFACE_INFO*            p_VirtIf       = NULL;
    DML_VIRTIFACE_INFO            VirtIfConst;
    CONTEXT_VIRTIF_LINK_OBJECT*    pVirtIfCxtLink = NULL;

    //Verify limit of the marking table
    if( WAN_IF_VIRTIF_MAX_LIMIT < pDmlVirtIf->ulNextInstanceNumber )
    {
        CcspTraceError(("%s %d - Failed to Create Virtual Interface entry due to maximum limit(%d)\n"
			 ,__FUNCTION__,__LINE__,WAN_IF_VIRTIF_MAX_LIMIT));
        return NULL;
    }

    p_VirtIf       = (DML_VIRTIFACE_INFO*)AnscAllocateMemory(sizeof(DML_VIRTIFACE_INFO));
    pVirtIfCxtLink = (CONTEXT_VIRTIF_LINK_OBJECT*)AnscAllocateMemory(sizeof(CONTEXT_VIRTIF_LINK_OBJECT));
    if((p_VirtIf == NULL) || (pVirtIfCxtLink == NULL))
    {
        if( NULL != pVirtIfCxtLink )
        {
          AnscFreeMemory(pVirtIfCxtLink);
          pVirtIfCxtLink = NULL;
        }

        if( NULL != p_VirtIf )
        {
          AnscFreeMemory(p_VirtIf);
          p_VirtIf = NULL;
        }
        return NULL;
    }

    /* now we have this link content */
    pVirtIfCxtLink->hContext = (ANSC_HANDLE)p_VirtIf;
    pVirtIfCxtLink->bNew     = TRUE;

    pVirtIfCxtLink->InstanceNumber  = pDmlVirtIf->ulNextInstanceNumber;
    *pInsNumber                      = pDmlVirtIf->ulNextInstanceNumber;

    //Assign actual instance number
    p_VirtIf->InstanceNumber = pDmlVirtIf->ulNextInstanceNumber;

    //Assign WAN interface instance for reference
    p_VirtIf->ulWANIfInstanceNumber = pWanDmlIface->uiInstanceNumber;

    pDmlVirtIf->ulNextInstanceNumber++;

    /*Set Default Values to New Created Virtual Interface*/
    memset(&VirtIfConst, 0, sizeof(DML_VIRTIFACE_INFO));
    WanManager_VirtIf_Update(p_VirtIf, &VirtIfConst);

    p_VirtIf->Status = WAN_IFACE_STATUS_DISABLED;

    /*Set Vlan and Marking NextInstanceNumber Default Value */
    p_VirtIf->VLANNumberOfEntries = 0;
    p_VirtIf->Vlan.ulNextInstanceNumber = 1;
    p_VirtIf->MarkingNumberOfEntries = 0;
    p_VirtIf->Marking.ulNextInstanceNumber = 1;

    SListPushEntryByInsNum(&pDmlVirtIf->VirtIfList, (PCONTEXT_LINK_OBJECT)pVirtIfCxtLink);

    CcspTraceInfo(("%s-%d - Created Virtual Interface entry Succesfully, Instance=%d, NextInstance=%d\n"
                    ,__FUNCTION__,__LINE__,p_VirtIf->InstanceNumber, pDmlVirtIf->ulNextInstanceNumber));
    
    //Create default one entry for Vlan in First Virtual Interface Entry
    for (int i = 0; i < 3; i++)
    {
    ULONG VlanInstanceNumber = 0;
    CONTEXT_VIRTIF_VLAN_LINK_OBJECT*    pVirtIfVlanCxtLink = NULL;

    pVirtIfVlanCxtLink = WanManager_Create_VirtIfVlan_Entry(pVirtIfCxtLink, &VlanInstanceNumber);
    if (pVirtIfVlanCxtLink)
    {
        CcspTraceInfo(("%s-%d - Created Virtual Interface Vlan entry Succesfully, VirtIfInstance=%d, VlanInstance=%d\n"
                       ,__FUNCTION__,__LINE__,p_VirtIf->InstanceNumber, VlanInstanceNumber));

    }

    //Create default one entry for Marking in First Virtual Interface Entry
    ULONG MarkingInstanceNumber = 0;
    CONTEXT_VIRTIF_MARKING_LINK_OBJECT*    pVirtIfMarkingCxtLink = NULL;

    pVirtIfMarkingCxtLink = WanManager_Create_VirtIfMarking_Entry(pVirtIfCxtLink, &MarkingInstanceNumber);
    if (pVirtIfMarkingCxtLink)
    {
        CcspTraceInfo(("%s-%d - Created Virtual Interface Marking entry Succesfully, VirtIfInstance=%d, VlanInstance=%d\n"
                       ,__FUNCTION__,__LINE__,p_VirtIf->InstanceNumber, MarkingInstanceNumber));

    }
    }
    
    return (ANSC_HANDLE)pVirtIfCxtLink;
}

ULONG WanManager_Remove_VirtIf_Entry(DML_WAN_IFACE* pWanDmlIface, CONTEXT_VIRTIF_LINK_OBJECT *pCxtLink)
{
    DATAMODEL_VIRTIF*              pDmlVirtIf     = (DATAMODEL_VIRTIF*) &(pWanDmlIface->VirtIf);
    CONTEXT_VIRTIF_LINK_OBJECT*    pVirtIfCxtLink = NULL;
    ULONG returnStatus = -1;

    if(pDmlVirtIf->ulNextInstanceNumber <= 1)
    {
        CcspTraceError(("%s %d - Failed to Remove Virtual Interface entry due to minimum limit(%d)\n"
                         ,__FUNCTION__ ,__LINE__ ,pDmlVirtIf->ulNextInstanceNumber));
        return NULL;
    }

    if ( pCxtLink->bNew )
    {
        /* Set bNew to FALSE to indicate this node is not going to save to SysRegistry */
        pCxtLink->bNew = FALSE;
    }

    DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO*)pCxtLink->hContext;
    pVirtIfCxtLink = SListGetEntryByInsNum(&pDmlVirtIf->VirtIfList, p_VirtIf->InstanceNumber);
    
    if (pVirtIfCxtLink)
    {
	//Remove Virtual Interface All Vlan Entry
	DML_VIRTIFACE_INFO* c_VirtIf = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;
	DATAMODEL_VIRTIF_VLAN*              pDmlVirtIfVlan     = (DATAMODEL_VIRTIF_VLAN*) &(c_VirtIf->Vlan);
        CONTEXT_VIRTIF_VLAN_LINK_OBJECT*    pVirtIfVlanCxtLink = NULL;

	if (pDmlVirtIfVlan->ulNextInstanceNumber > 1)
	{
            for (int i = 1; i < pDmlVirtIfVlan->ulNextInstanceNumber; i++)
	    {
                pVirtIfVlanCxtLink = SListGetEntryByInsNum(&pDmlVirtIfVlan->VirtIfVlanList, i);
		AnscFreeMemory(pVirtIfVlanCxtLink->hContext);
                AnscFreeMemory(pVirtIfVlanCxtLink);
	    }
            CcspTraceInfo(("%s-%d - Remove Virtual Interface All Vlan entry Succesfully, TotalVlanInstanceNumber=%d\n"
                            , __FUNCTION__, __LINE__, pDmlVirtIfVlan->ulNextInstanceNumber));	    
	    pDmlVirtIfVlan->ulNextInstanceNumber = 1;
	}

	//Remove Virtual Interface All Marking Entry
        DATAMODEL_VIRTIF_MARKING*              pDmlVirtIfMarking     = (DATAMODEL_VIRTIF_MARKING*) &(c_VirtIf->Marking);
        CONTEXT_VIRTIF_MARKING_LINK_OBJECT*    pVirtIfMarkingCxtLink = NULL;

        if (pDmlVirtIfMarking->ulNextInstanceNumber > 1)
        {
            for (int i = 1; i < pDmlVirtIfMarking->ulNextInstanceNumber; i++)
            {
                pVirtIfMarkingCxtLink = SListGetEntryByInsNum(&pDmlVirtIfMarking->VirtIfMarkingList, i);
                AnscFreeMemory(pVirtIfMarkingCxtLink->hContext);
                AnscFreeMemory(pVirtIfMarkingCxtLink);
            }
            CcspTraceInfo(("%s-%d - Remove Virtual Interface All Marking entry Succesfully, TotalMarkingInstanceNumber=%d\n"
                            , __FUNCTION__, __LINE__, pDmlVirtIfMarking->ulNextInstanceNumber));
            pDmlVirtIfMarking->ulNextInstanceNumber = 1;
        }

        AnscFreeMemory(pVirtIfCxtLink->hContext);
        AnscFreeMemory(pVirtIfCxtLink);
	pWanDmlIface->VirtIf.ulNextInstanceNumber--;
	returnStatus = 0;
	CcspTraceInfo(("%s-%d - Remove Virtual Interface entry Succesfully, Instance=%d\n"
                         ,__FUNCTION__,__LINE__,p_VirtIf->InstanceNumber));
    }
    else
    {
        CcspTraceError(("%s-%d - Failed to Remove Virtual Interface entry, Instance=%d\n"
                         ,__FUNCTION__,__LINE__,p_VirtIf->InstanceNumber));
    }

    return returnStatus;
}

ULONG WanManager_Update_VirtIf_Entry(CONTEXT_VIRTIF_LINK_OBJECT *pCxtLink)
{

    DML_VIRTIFACE_INFO* p_VirtIf = (DML_VIRTIFACE_INFO*)pCxtLink->hContext;
    CONTEXT_VIRTIF_LINK_OBJECT*    pVirtIfCxtLink = NULL;
    ULONG returnStatus = -1;

    WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked((p_VirtIf->ulWANIfInstanceNumber-1));
    if(pWanDmlIfaceData != NULL)
    {
        DML_WAN_IFACE* pWanDmlIface    = &(pWanDmlIfaceData->data);
        DATAMODEL_VIRTIF* pDmlVirtIf     = (DATAMODEL_VIRTIF*) &(pWanDmlIface->VirtIf);
    
        if(p_VirtIf->InstanceNumber >= pDmlVirtIf->ulNextInstanceNumber)
        {
            CcspTraceError(("%s %d - Failed to Update Virtual Interface entry, InstanceNumber=%d, NextInstanceNumber=%d\n"
                             ,__FUNCTION__ ,__LINE__ ,p_VirtIf->InstanceNumber, pDmlVirtIf->ulNextInstanceNumber));
            return returnStatus;
        }

        pVirtIfCxtLink = SListGetEntryByInsNum(&pDmlVirtIf->VirtIfList, p_VirtIf->InstanceNumber);

        if (pVirtIfCxtLink)
        {
	    DML_VIRTIFACE_INFO* c_VirtIf = (DML_VIRTIFACE_INFO*)pCxtLink->hContext;
	    WanManager_VirtIf_Update(c_VirtIf, p_VirtIf);
	    returnStatus = WanManager_SetVirtIfCfg(p_VirtIf);
            CcspTraceInfo(("%s-%d - Updated Virtual Interface entry Succesfully, Instance=%d\n"
                             ,__FUNCTION__,__LINE__,p_VirtIf->InstanceNumber));
        }
        else
        {
            CcspTraceError(("%s-%d - Failed to Update Virtual Interface entry, Instance=%d\n"
                             ,__FUNCTION__,__LINE__,p_VirtIf->InstanceNumber));
        }

	WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
    }
    return 0;
}

ANSC_HANDLE WanManager_Create_VirtIfVlan_Entry(CONTEXT_VIRTIF_LINK_OBJECT *pVirtIfCxtLink, ULONG* pInsNumber)
{
    ANSC_HANDLE                  returnStatus  = NULL;
    DML_VIRTIFACE_INFO*           p_VirtIf       = NULL;

    if (pVirtIfCxtLink)
    {
	DML_VIRTIFACE_INFO*                 p_VirtIf           = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;
        DATAMODEL_VIRTIF_VLAN*              pDmlVirtIfVlan     = (DATAMODEL_VIRTIF_VLAN*) &(p_VirtIf->Vlan);
        DML_VIRTIF_VLAN*                    p_VirtIfVlan       = NULL;
        CONTEXT_VIRTIF_VLAN_LINK_OBJECT*    pVirtIfVlanCxtLink = NULL;

        p_VirtIfVlan       = (DML_VIRTIF_VLAN*)AnscAllocateMemory(sizeof(DML_VIRTIF_VLAN));
        pVirtIfVlanCxtLink = (CONTEXT_VIRTIF_VLAN_LINK_OBJECT*)AnscAllocateMemory(sizeof(CONTEXT_VIRTIF_VLAN_LINK_OBJECT));
        if((p_VirtIfVlan == NULL) || (pVirtIfVlanCxtLink == NULL))
        {
            if( NULL != pVirtIfVlanCxtLink )
            {
                AnscFreeMemory(pVirtIfVlanCxtLink);
                pVirtIfVlanCxtLink = NULL;
            }

            if( NULL != p_VirtIfVlan )
            {
                AnscFreeMemory(p_VirtIfVlan);
                p_VirtIfVlan = NULL;
            }
            return NULL;
        }
        /* Need to add here Limit on Number of Vlan create For Each VirtIf */
        /* now we have this link content */
        pVirtIfVlanCxtLink->hContext = (ANSC_HANDLE)p_VirtIfVlan;
        pVirtIfVlanCxtLink->bNew     = TRUE;

        pVirtIfVlanCxtLink->InstanceNumber  = pDmlVirtIfVlan->ulNextInstanceNumber;
        *pInsNumber                      = pDmlVirtIfVlan->ulNextInstanceNumber;

        //Assign actual instance number
        p_VirtIfVlan->VlanInstanceNumber = pDmlVirtIfVlan->ulNextInstanceNumber;

        //Assign WAN interface instance for reference
        p_VirtIfVlan->VirtIfInstanceNumber = p_VirtIf->InstanceNumber;
	p_VirtIfVlan->ulWANIfInstanceNumber = p_VirtIf->ulWANIfInstanceNumber;

        pDmlVirtIfVlan->ulNextInstanceNumber++;

        memset(p_VirtIfVlan->Interface, 0, sizeof(p_VirtIfVlan->Interface));

        SListPushEntryByInsNum(&pDmlVirtIfVlan->VirtIfVlanList, (PCONTEXT_LINK_OBJECT)pVirtIfVlanCxtLink);
    
        CcspTraceInfo(("%s-%d - Created Virtual Interface Vlan entry Succesfully, VlanInstance=%d, VlanNextInstance=%d\n"
                        ,__FUNCTION__,__LINE__,p_VirtIfVlan->VlanInstanceNumber, pDmlVirtIfVlan->ulNextInstanceNumber));

        returnStatus = (ANSC_HANDLE)pVirtIfVlanCxtLink;
    }
    else
    {
        CcspTraceError(("%s-%d - Virtual Interface entry Null, VirtIfInstace=%d\n"
                         ,__FUNCTION__,__LINE__, pVirtIfCxtLink->InstanceNumber));
    }
    
    return returnStatus;
}

ULONG WanManager_Remove_VirtIfVlan_Entry(CONTEXT_VIRTIF_LINK_OBJECT *pVirtIfCxtLink, CONTEXT_VIRTIF_VLAN_LINK_OBJECT *pCxtLink)
{
    ULONG returnStatus = -1;

    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO*                 p_VirtIf           = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;
        DATAMODEL_VIRTIF_VLAN*              pDmlVirtIfVlan     = (DATAMODEL_VIRTIF_VLAN*) &(p_VirtIf->Vlan);

        if(pDmlVirtIfVlan->ulNextInstanceNumber <= 1)
        {
            CcspTraceError(("%s %d - Failed to Remove Virtual Interface Vlan Entry due to minimum limit(%d)\n"
                            ,__FUNCTION__ ,__LINE__ ,pDmlVirtIfVlan->ulNextInstanceNumber));
            return NULL;
        }

        DML_VIRTIF_VLAN*                    p_VirtIfVlan       = NULL;
        CONTEXT_VIRTIF_VLAN_LINK_OBJECT*    pVirtIfVlanCxtLink = NULL;
	    

        if ( pCxtLink->bNew )
        {
            /* Set bNew to FALSE to indicate this node is not going to save to SysRegistry */
            pCxtLink->bNew = FALSE;
        }

        p_VirtIfVlan = (DML_VIRTIF_VLAN*)pCxtLink->hContext;
        pVirtIfVlanCxtLink = SListGetEntryByInsNum(&pDmlVirtIfVlan->VirtIfVlanList, p_VirtIfVlan->VlanInstanceNumber);

        if (pVirtIfVlanCxtLink)
        {
            AnscFreeMemory(pVirtIfVlanCxtLink->hContext);
            AnscFreeMemory(pVirtIfVlanCxtLink);
            pDmlVirtIfVlan->ulNextInstanceNumber--;
            returnStatus = 0;
            CcspTraceInfo(("%s-%d - Remove Virtual Interface Vlan entry Succesfully, Instance=%d\n"
                             ,__FUNCTION__,__LINE__,p_VirtIfVlan->VlanInstanceNumber));
        }
        else
        {
            CcspTraceError(("%s-%d - Failed to Remove Virtual Interface Vlan entry, Instance=%d\n"
                             ,__FUNCTION__,__LINE__,p_VirtIfVlan->VlanInstanceNumber));
        }

    }
    else
    {
            CcspTraceError(("%s-%d - Virtual Interface entry Null, VirtIf Instance=%d\n"
                             ,__FUNCTION__,__LINE__, pVirtIfCxtLink->InstanceNumber));
    }
    return returnStatus;
}

ULONG WanManager_Update_VirtIfVlan_Entry(CONTEXT_VIRTIF_VLAN_LINK_OBJECT *pCxtLink)
{
    DML_VIRTIF_VLAN* p_VirtIfVlan = (DML_VIRTIF_VLAN*)pCxtLink->hContext;
    CONTEXT_VIRTIF_VLAN_LINK_OBJECT* pVirtIfVlanCxtLink = NULL;
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = NULL;
    ULONG returnStatus = -1;
    
    WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked((p_VirtIfVlan->ulWANIfInstanceNumber-1));
    if(pWanDmlIfaceData != NULL)
    {
        DML_WAN_IFACE* pWanDmlIface    = &(pWanDmlIfaceData->data);
        DATAMODEL_VIRTIF* pDmlVirtIf     = (DATAMODEL_VIRTIF*) &(pWanDmlIface->VirtIf);

	pVirtIfCxtLink = SListGetEntryByInsNum(&pDmlVirtIf->VirtIfList, p_VirtIfVlan->VirtIfInstanceNumber);
        if(pVirtIfCxtLink == NULL)
        {
            CcspTraceError(("%s %d - Failed to Get Virtual Interface entry, WanInstanceNumber=%d, VirtIfInstanceNumber=%d\n"
                             , __FUNCTION__ , __LINE__ , p_VirtIfVlan->ulWANIfInstanceNumber, p_VirtIfVlan->VirtIfInstanceNumber));
            return returnStatus;
        }

	DML_VIRTIFACE_INFO*                 p_VirtIf           = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;
        DATAMODEL_VIRTIF_VLAN*              pDmlVirtIfVlan     = (DATAMODEL_VIRTIF_VLAN*) &(p_VirtIf->Vlan);

        pVirtIfVlanCxtLink = SListGetEntryByInsNum(&pDmlVirtIfVlan->VirtIfVlanList, p_VirtIfVlan->VlanInstanceNumber);

        if (pVirtIfVlanCxtLink)
        {
	    DML_VIRTIF_VLAN* c_VirtIfVlan = (DML_VIRTIF_VLAN*)pVirtIfVlanCxtLink->hContext;
	    memcpy(c_VirtIfVlan->Interface, p_VirtIfVlan->Interface, sizeof(p_VirtIfVlan->Interface));
            CcspTraceInfo(("%s-%d - Updated Virtual Interface Vlan entry Succesfully, Updated VlanInstance=%d, VlanInstance=%d\n"
                             ,__FUNCTION__,__LINE__,c_VirtIfVlan->VlanInstanceNumber, p_VirtIfVlan->VlanInstanceNumber));
	    returnStatus = 0;
        }
        else
        {
            CcspTraceError(("%s-%d - Failed to Update Virtual Interface Vlan entry, Instance=%d\n"
                             ,__FUNCTION__,__LINE__,p_VirtIfVlan->VlanInstanceNumber));
        }

        WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
    }

    return returnStatus;
}


ANSC_HANDLE WanManager_Create_VirtIfMarking_Entry(CONTEXT_VIRTIF_LINK_OBJECT *pVirtIfCxtLink, ULONG* pInsNumber)
{
    ANSC_HANDLE                  returnStatus  = NULL;
    DML_VIRTIFACE_INFO*           p_VirtIf       = NULL;

    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO*                    p_VirtIf           = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;
        DATAMODEL_VIRTIF_MARKING*              pDmlVirtIfMarking  = (DATAMODEL_VIRTIF_MARKING*) &(p_VirtIf->Marking);
        DML_VIRTIF_MARKING*                    p_VirtIfMarking       = NULL;
        CONTEXT_VIRTIF_MARKING_LINK_OBJECT*    pVirtIfMarkingCxtLink = NULL;

        p_VirtIfMarking       = (DML_VIRTIF_MARKING*)AnscAllocateMemory(sizeof(DML_VIRTIF_MARKING));
        pVirtIfMarkingCxtLink = (CONTEXT_VIRTIF_MARKING_LINK_OBJECT*)AnscAllocateMemory(sizeof(CONTEXT_VIRTIF_MARKING_LINK_OBJECT));
        if((p_VirtIfMarking == NULL) || (pVirtIfMarkingCxtLink == NULL))
        {
            if( NULL != pVirtIfMarkingCxtLink )
            {
                AnscFreeMemory(pVirtIfMarkingCxtLink);
                pVirtIfMarkingCxtLink = NULL;
            }

            if( NULL != p_VirtIfMarking )
            {
                AnscFreeMemory(p_VirtIfMarking);
                p_VirtIfMarking = NULL;
            }
            return NULL;
        }
        /* Need to add here Limit on Number of Marking create For Each VirtIf */
        /* now we have this link content */
        pVirtIfMarkingCxtLink->hContext = (ANSC_HANDLE)p_VirtIfMarking;
        pVirtIfMarkingCxtLink->bNew     = TRUE;

        pVirtIfMarkingCxtLink->InstanceNumber  = pDmlVirtIfMarking->ulNextInstanceNumber;
        *pInsNumber                      = pDmlVirtIfMarking->ulNextInstanceNumber;

        //Assign actual instance number
        p_VirtIfMarking->MarkingInstanceNumber = pDmlVirtIfMarking->ulNextInstanceNumber;

        //Assign WAN interface instance for reference
        p_VirtIfMarking->VirtIfInstanceNumber = p_VirtIf->InstanceNumber;
	p_VirtIfMarking->ulWANIfInstanceNumber = p_VirtIf->ulWANIfInstanceNumber;

        pDmlVirtIfMarking->ulNextInstanceNumber++;

        memset(p_VirtIfMarking->Entry, 0, sizeof(p_VirtIfMarking->Entry));

        SListPushEntryByInsNum(&pDmlVirtIfMarking->VirtIfMarkingList, (PCONTEXT_LINK_OBJECT)pVirtIfMarkingCxtLink);

        CcspTraceInfo(("%s-%d - Created Virtual Interface Marking entry Succesfully, MarkingInstance=%d, MarkingNextInstance=%d\n"
                        ,__FUNCTION__,__LINE__,p_VirtIfMarking->MarkingInstanceNumber, pDmlVirtIfMarking->ulNextInstanceNumber));

        returnStatus = (ANSC_HANDLE)pVirtIfMarkingCxtLink;
    }
    else
    {
        CcspTraceError(("%s-%d - Virtual Interface entry Null, VirtIfInstance=%d\n"
                         ,__FUNCTION__,__LINE__, pVirtIfCxtLink->InstanceNumber));
    }

    return returnStatus;
}

ULONG WanManager_Remove_VirtIfMarking_Entry(CONTEXT_VIRTIF_LINK_OBJECT *pVirtIfCxtLink, CONTEXT_VIRTIF_MARKING_LINK_OBJECT *pCxtLink)
{
    ULONG returnStatus = -1;

    if (pVirtIfCxtLink)
    {
        DML_VIRTIFACE_INFO*                    p_VirtIf              = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;
        DATAMODEL_VIRTIF_MARKING*              pDmlVirtIfMarking     = (DATAMODEL_VIRTIF_MARKING*) &(p_VirtIf->Marking);

        if(pDmlVirtIfMarking->ulNextInstanceNumber <= 1)
        {
            CcspTraceError(("%s %d - Failed to Remove Virtual Interface Marking Entry due to minimum limit(%d)\n"
                            ,__FUNCTION__ ,__LINE__ ,pDmlVirtIfMarking->ulNextInstanceNumber));
            return NULL;
        }

        DML_VIRTIF_MARKING*                    p_VirtIfMarking       = NULL;
        CONTEXT_VIRTIF_MARKING_LINK_OBJECT*    pVirtIfMarkingCxtLink = NULL;


        if ( pCxtLink->bNew )
        {
            /* Set bNew to FALSE to indicate this node is not going to save to SysRegistry */
            pCxtLink->bNew = FALSE;
        }

        p_VirtIfMarking = (DML_VIRTIF_MARKING*)pCxtLink->hContext;
        pVirtIfMarkingCxtLink = SListGetEntryByInsNum(&pDmlVirtIfMarking->VirtIfMarkingList, p_VirtIfMarking->MarkingInstanceNumber);

        if (pVirtIfMarkingCxtLink)
        {
            AnscFreeMemory(pVirtIfMarkingCxtLink->hContext);
            AnscFreeMemory(pVirtIfMarkingCxtLink);
            pDmlVirtIfMarking->ulNextInstanceNumber--;
            returnStatus = 0;
            CcspTraceInfo(("%s-%d - Remove Virtual Interface Marking entry Succesfully, MarkingInstance=%d\n"
                             ,__FUNCTION__,__LINE__,p_VirtIfMarking->MarkingInstanceNumber));
        }
        else
        {
            CcspTraceError(("%s-%d - Failed to Remove Virtual Interface Marking entry, Instance=%d\n"
                             ,__FUNCTION__,__LINE__,p_VirtIfMarking->MarkingInstanceNumber));
        }

    }
    else
    {
            CcspTraceError(("%s-%d - Virtual Interface entry Null, VirtIf Instance=%d\n"
                             ,__FUNCTION__,__LINE__, pVirtIfCxtLink->InstanceNumber));
    }
    return returnStatus;
}

ULONG WanManager_Update_VirtIfMarking_Entry(CONTEXT_VIRTIF_MARKING_LINK_OBJECT *pCxtLink)
{
    DML_VIRTIF_MARKING* p_VirtIfMarking = (DML_VIRTIF_MARKING*)pCxtLink->hContext;
    CONTEXT_VIRTIF_MARKING_LINK_OBJECT* pVirtIfMarkingCxtLink = NULL;
    CONTEXT_VIRTIF_LINK_OBJECT* pVirtIfCxtLink = NULL;
    ULONG returnStatus = -1;

    WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceData_locked((p_VirtIfMarking->ulWANIfInstanceNumber-1));
    if(pWanDmlIfaceData != NULL)
    {
        DML_WAN_IFACE* pWanDmlIface    = &(pWanDmlIfaceData->data);
        DATAMODEL_VIRTIF* pDmlVirtIf     = (DATAMODEL_VIRTIF*) &(pWanDmlIface->VirtIf);

        pVirtIfCxtLink = SListGetEntryByInsNum(&pDmlVirtIf->VirtIfList, p_VirtIfMarking->VirtIfInstanceNumber);
        if(pVirtIfCxtLink == NULL)
        {
            CcspTraceError(("%s %d - Failed to Get Virtual Interface entry, WanInstanceNumber=%d, VirtIfInstanceNumber=%d\n"
                             , __FUNCTION__ , __LINE__ , p_VirtIfMarking->ulWANIfInstanceNumber, p_VirtIfMarking->VirtIfInstanceNumber));
            return returnStatus;
        }

        DML_VIRTIFACE_INFO*                 p_VirtIf           = (DML_VIRTIFACE_INFO*)pVirtIfCxtLink->hContext;
        DATAMODEL_VIRTIF_MARKING*           pDmlVirtIfMarking  = (DATAMODEL_VIRTIF_MARKING*) &(p_VirtIf->Marking);

        pVirtIfMarkingCxtLink = SListGetEntryByInsNum(&pDmlVirtIfMarking->VirtIfMarkingList, p_VirtIfMarking->MarkingInstanceNumber);

        if (pVirtIfMarkingCxtLink)
        {
            DML_VIRTIF_MARKING* c_VirtIfMarking = (DML_VIRTIF_MARKING*)pVirtIfMarkingCxtLink->hContext;
            memcpy(c_VirtIfMarking->Entry, p_VirtIfMarking->Entry, sizeof(p_VirtIfMarking->Entry));
            CcspTraceInfo(("%s-%d - Updated Virtual Interface Marking entry Succesfully, Updated MarkingInstance=%d, MarkingInstance=%d\n"
                             ,__FUNCTION__,__LINE__,c_VirtIfMarking->MarkingInstanceNumber, p_VirtIfMarking->MarkingInstanceNumber));
            returnStatus = 0;
        }
        else
        {
            CcspTraceError(("%s-%d - Failed to Update Virtual Interface Marking entry, Instance=%d\n"
                             ,__FUNCTION__,__LINE__,p_VirtIfMarking->MarkingInstanceNumber));
        }

        WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
    }

    return returnStatus;
}

