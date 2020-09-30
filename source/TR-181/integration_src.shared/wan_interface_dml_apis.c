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
#include "wan_interface_dml_apis.h"
#include "wan_interface_dml.h"
#include "wan_interface_internal.h"
#include "dmsb_tr181_psm_definitions.h"

#define PSM_ENABLE_STRING_TRUE  "TRUE"
#define PSM_ENABLE_STRING_FALSE  "FALSE"
#define ETH_IF_STR "eth"
#define DEFAULT_WAN_IF_NAME "erouter0"
#define DATA_SKB_MARKING_LOCATION "/tmp/skb_marking.conf"

//VLAN Agent
#define VLAN_DBUS_PATH                     "/com/cisco/spvtg/ccsp/vlanmanager"
#define VLAN_COMPONENT_NAME                "eRT.com.cisco.spvtg.ccsp.vlanmanager"
#define VLAN_ETHLINK_NOE_PARAM_NAME        "Device.X_RDK_Ethernet.LinkNumberOfEntries"
#define VLAN_ETHLINK_TABLE_NAME            "Device.X_RDK_Ethernet.Link."
#define VLAN_ETHLINK_REFRESH_PARAM_NAME    "Device.X_RDK_Ethernet.Link.%d.X_RDK_Refresh"
//XDSL Manager
#define DSL_COMPONENT_NAME "eRT.com.cisco.spvtg.ccsp.xdslmanager"
#define DSL_COMPONENT_PATH "/com/cisco/spvtg/ccsp/xdslmanager"
#define DSL_UPSTREAM_NAME ".Upstream"
//Eth Manager
#define ETH_COMPONENT_NAME "eRT.com.cisco.spvtg.ccsp.ethagent"
#define ETH_COMPONENT_PATH "/com/cisco/spvtg/ccsp/ethagent"
#define ETH_UPSTREAM_NAME ".Upstream"

extern char g_Subsystem[32];
extern ANSC_HANDLE bus_handle;
extern PBACKEND_MANAGER_OBJECT  g_pBEManager;

static int read_Wan_Interface_ParametersFromPSM(ULONG instancenum, PDML_WAN_IFACE p_Interface);
static int write_Wan_Interface_ParametersFromPSM(ULONG instancenum, PDML_WAN_IFACE p_Interface);
static int write_Wan_Interface_Validation_ParametersToPSM(ULONG instancenum, PDML_WAN_IFACE p_Interface);

#define _PSM_READ_PARAM(_PARAM_NAME) { \
    _ansc_memset(param_name, 0, sizeof(param_name)); \
    _ansc_sprintf(param_name, _PARAM_NAME, instancenum); \
    retPsmGet = PSM_Get_Record_Value2(bus_handle,g_Subsystem, param_name, NULL, &param_value); \
    if (retPsmGet != CCSP_SUCCESS) { \
        AnscTraceFlow(("%s Error %d reading %s %s\n", __FUNCTION__, retPsmGet, param_name, param_value));\
    } \
    else { \
        /*AnscTraceFlow(("%s: retPsmGet == CCSP_SUCCESS reading %s = \n%s\n", __FUNCTION__,param_name, param_value)); */\
    } \
}

#define _PSM_WRITE_PARAM(_PARAM_NAME) { \
    _ansc_sprintf(param_name, _PARAM_NAME, instancenum); \
    retPsmSet = PSM_Set_Record_Value2(bus_handle,g_Subsystem, param_name, ccsp_string, param_value); \
    if (retPsmSet != CCSP_SUCCESS) { \
        AnscTraceFlow(("%s Error %d writing %s %s\n", __FUNCTION__, retPsmSet, param_name, param_value));\
    } \
    else \
    { \
        /*AnscTraceFlow(("%s: retPsmSet == CCSP_SUCCESS writing %s = %s \n", __FUNCTION__,param_name,param_value)); */\
    } \
    _ansc_memset(param_name, 0, sizeof(param_name)); \
    _ansc_memset(param_value, 0, sizeof(param_value)); \
}

static int DmlWanGetPSMRecordValue ( char *pPSMCmd, char *pOutputString );
static int DmlWanSetPSMRecordValue ( char *pPSMEntry, char *pSetString );
static int DmlWanDeletePSMRecordValue ( char *pPSMEntry );

#ifdef _HUB4_PRODUCT_REQ_
static void AddSkbMarkingToConfFile(UINT data_skb_mark);
#endif

PDML_WAN_IFACE_GLOBAL_CONFIG gpstWanGInfo = NULL;
pthread_mutex_t gmWanGInfo_mutex = PTHREAD_MUTEX_INITIALIZER;
static ANSC_STATUS DmlWanIfPrepareGlobalInfo();

static void* DmlWanIfRefreshHandlingThread( void *arg );
static ANSC_STATUS DmlWanGetInterfaceInstanceInOtherAgent( WAN_NOTIFY_ENUM enNotifyAgent, char *pIfName, INT *piInstanceNumber );
static ANSC_STATUS DmlWanGetParamNames( char *pComponent, char *pBus, char *pParamName, char a2cReturnVal[][256], int *pReturnSize );
static ANSC_STATUS DmlWanGetParamValues( char *pComponent, char *pBus, char *pParamName, char *pReturnVal );
static ANSC_STATUS DmlWanSetParamValues( char *pComponent, char *pBus, char *pParamName, char *pParamVal, enum dataType_e type, BOOLEAN bCommit );

static int read_Wan_Interface_ParametersFromPSM(ULONG instancenum, PDML_WAN_IFACE p_Interface)
{
    int retPsmGet = CCSP_SUCCESS;
    char *param_value= NULL;
    char param_name[256]= {0};
    
    p_Interface[instancenum-1].ulInstanceNumber = instancenum;
    
    _PSM_READ_PARAM(PSM_WANMANAGER_IF_ENABLE);
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0)
        {
             p_Interface[instancenum-1].CfgEnable = TRUE;
        }
        else
        {
             p_Interface[instancenum-1].CfgEnable = FALSE;
        }
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }
    else
    {
        p_Interface[instancenum-1].CfgEnable = FALSE;
    }
    
    _PSM_READ_PARAM(PSM_WANMANAGER_IF_NAME);
    if (retPsmGet == CCSP_SUCCESS)
    {
        AnscCopyString(p_Interface[instancenum-1].Name, param_value);
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }
    
    _PSM_READ_PARAM(PSM_WANMANAGER_IF_DISPLAY_NAME);
    if (retPsmGet == CCSP_SUCCESS)
    {
        AnscCopyString(p_Interface[instancenum-1].DisplayName, param_value);
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }

    _PSM_READ_PARAM(PSM_WANMANAGER_IF_TYPE);
    if (retPsmGet == CCSP_SUCCESS)
    {
        _ansc_sscanf(param_value, "%d", &(p_Interface[instancenum-1].CfgType));
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }
    
    _PSM_READ_PARAM(PSM_WANMANAGER_IF_PRIORITY);
    if (retPsmGet == CCSP_SUCCESS)
    {
        _ansc_sscanf(param_value, "%d", &(p_Interface[instancenum-1].CfgPriority));
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }
    
    _PSM_READ_PARAM(PSM_WANMANAGER_IF_SELECTIONTIMEOUT);
    if (retPsmGet == CCSP_SUCCESS)
    {
        _ansc_sscanf(param_value, "%d", &(p_Interface[instancenum-1].CfgSelectionTimeout));
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }

    _PSM_READ_PARAM(PSM_WANMANAGER_IF_WAN_ENABLE_PPP);
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0)
        {
             p_Interface[instancenum-1].CfgEnablePPP = TRUE;
        }
        else
        {
             p_Interface[instancenum-1].CfgEnablePPP = FALSE;
        }
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }
    else
    {
        p_Interface[instancenum-1].CfgEnablePPP = FALSE;
    }

    _PSM_READ_PARAM(PSM_WANMANAGER_IF_WAN_ENABLE_MAPT);
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0)
        {
             p_Interface[instancenum-1].CfgEnableMAPT = TRUE;
        }
        else
        {
             p_Interface[instancenum-1].CfgEnableMAPT = FALSE;
        }
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }
    else
    {
        p_Interface[instancenum-1].CfgEnableMAPT = FALSE;
    }

    _PSM_READ_PARAM(PSM_WANMANAGER_IF_WAN_ENABLE_DSLITE);
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0)
        {
             p_Interface[instancenum-1].CfgEnableDSLite = TRUE;
        }
        else
        {
             p_Interface[instancenum-1].CfgEnableDSLite = FALSE;
        }
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }
    else
    {
        p_Interface[instancenum-1].CfgEnableDSLite = FALSE;
    }

    _PSM_READ_PARAM(PSM_WANMANAGER_IF_WAN_ENABLE_IPOE);
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0)
        {
             p_Interface[instancenum-1].CfgEnableIPoE = TRUE;
        }
        else
        {
             p_Interface[instancenum-1].CfgEnableIPoE = FALSE;
        }
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }
    else
    {
        p_Interface[instancenum-1].CfgEnableIPoE = FALSE;
    }

    _PSM_READ_PARAM(PSM_WANMANAGER_IF_WAN_VALIDATION_DISCOVERY_OFFER);
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0)
        {
             p_Interface[instancenum-1].CfgValidationDiscoverOffer = TRUE;
        }
        else
        {
             p_Interface[instancenum-1].CfgValidationDiscoverOffer = FALSE;
        }
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }
    else
    {
        p_Interface[instancenum-1].CfgValidationDiscoverOffer = FALSE;
    }

    _PSM_READ_PARAM(PSM_WANMANAGER_IF_WAN_VALIDATION_SOLICIT_ADVERTISE);
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0)
        {
             p_Interface[instancenum-1].CfgValidationSolicitAdvertise = TRUE;
        }
        else
        {
             p_Interface[instancenum-1].CfgValidationSolicitAdvertise = FALSE;
        }
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }
    else
    {
        p_Interface[instancenum-1].CfgValidationSolicitAdvertise = FALSE;
    }

    _PSM_READ_PARAM(PSM_WANMANAGER_IF_WAN_VALIDATION_RS_RA);
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0)
        {
             p_Interface[instancenum-1].CfgValidationRsRa = TRUE;
        }
        else
        {
             p_Interface[instancenum-1].CfgValidationRsRa = FALSE;
        }
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }
    else
    {
        p_Interface[instancenum-1].CfgValidationRsRa = FALSE;
    }

    _PSM_READ_PARAM(PSM_WANMANAGER_IF_WAN_VALIDATION_PADI_PADO);
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0)
        {
             p_Interface[instancenum-1].CfgValidationPadiPado = TRUE;
        }
        else
        {
             p_Interface[instancenum-1].CfgValidationPadiPado = FALSE;
        }
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }
    else
    {
        p_Interface[instancenum-1].CfgValidationPadiPado = FALSE;
    }
    
    _PSM_READ_PARAM(PSM_WANMANAGER_IF_DYNTRIGGERENABLE);
    if (retPsmGet == CCSP_SUCCESS)
    {
        if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0)
        {
             p_Interface[instancenum-1].DynTriggerEnable = TRUE;
        }
        else
        {
             p_Interface[instancenum-1].DynTriggerEnable = FALSE;
        }
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }
    else
    {
        p_Interface[instancenum-1].DynTriggerEnable = FALSE;
    }

    _PSM_READ_PARAM(PSM_WANMANAGER_IF_DYNTRIGGERDELAY);
    if (retPsmGet == CCSP_SUCCESS)
    {
        _ansc_sscanf(param_value, "%d", &(p_Interface[instancenum-1].DynTriggerDelay));
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }

    return ANSC_STATUS_SUCCESS;
}

static int write_Wan_Interface_ParametersFromPSM(ULONG instancenum, PDML_WAN_IFACE p_Interface)
{
    int retPsmSet = CCSP_SUCCESS;
    char param_name[256] = {0};
    char param_value[256] = {0};

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    if(p_Interface->CfgEnable)
    {
        _ansc_sprintf(param_value, "TRUE");
    }
    else
    {
        _ansc_sprintf(param_value, "FALSE");
    }
    _PSM_WRITE_PARAM(PSM_WANMANAGER_IF_ENABLE);

    _ansc_sprintf(param_value, "%d", p_Interface->CfgType );
    _PSM_WRITE_PARAM(PSM_WANMANAGER_IF_TYPE);

    _ansc_sprintf(param_value, "%d", p_Interface->CfgPriority );
    _PSM_WRITE_PARAM(PSM_WANMANAGER_IF_PRIORITY);

    _PSM_WRITE_PARAM(PSM_WANMANAGER_IF_SELECTIONTIMEOUT);
    _ansc_sprintf(param_value, "%d", p_Interface->CfgSelectionTimeout );
	
    if(p_Interface->DynTriggerEnable) {
        _ansc_sprintf(param_value, "TRUE");
    }
    else {
        _ansc_sprintf(param_value, "FALSE");
    }
    _PSM_WRITE_PARAM(PSM_WANMANAGER_IF_DYNTRIGGERENABLE);

        if(p_Interface->CfgEnablePPP)
    {
        _ansc_sprintf(param_value, "TRUE");
    }
    else
    {
        _ansc_sprintf(param_value, "FALSE");
    }
    _PSM_WRITE_PARAM(PSM_WANMANAGER_IF_WAN_ENABLE_PPP);

    if(p_Interface->CfgEnableMAPT)
    {
        _ansc_sprintf(param_value, "TRUE");
    }
    else
    {
        _ansc_sprintf(param_value, "FALSE");
    }
    _PSM_WRITE_PARAM(PSM_WANMANAGER_IF_WAN_ENABLE_MAPT);

    if(p_Interface->CfgEnableDSLite)
    {
        _ansc_sprintf(param_value, "TRUE");
    }
    else
    {
        _ansc_sprintf(param_value, "FALSE");
    }
    _PSM_WRITE_PARAM(PSM_WANMANAGER_IF_WAN_ENABLE_DSLITE);

    if(p_Interface->CfgEnableIPoE)
    {
        _ansc_sprintf(param_value, "TRUE");
    }
    else
    {
        _ansc_sprintf(param_value, "FALSE");
    }
    _PSM_WRITE_PARAM(PSM_WANMANAGER_IF_WAN_ENABLE_IPOE);

    if(p_Interface->CfgValidationDiscoverOffer)
    {
        _ansc_sprintf(param_value, "TRUE");
    }
    else
    {
        _ansc_sprintf(param_value, "FALSE");
    }

    _ansc_sprintf(param_value, "%d", p_Interface->CfgPriority );
    _PSM_WRITE_PARAM(PSM_WANMANAGER_IF_PRIORITY);

    _PSM_WRITE_PARAM(PSM_WANMANAGER_IF_DYNTRIGGERDELAY);
    _ansc_sprintf(param_value, "%d", p_Interface->DynTriggerDelay );

    return ANSC_STATUS_SUCCESS;
}

static int write_Wan_Interface_Validation_ParametersToPSM(ULONG instancenum, PDML_WAN_IFACE p_Interface)
{
    if (NULL == p_Interface)
    {
        AnscTraceFlow(("%s Invalid memory!!!\n", __FUNCTION__));
        return ANSC_STATUS_INTERNAL_ERROR;
    }

    int retPsmSet = CCSP_SUCCESS;
    char param_name[256] = {0};
    char param_value[256] = {0};

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    if(p_Interface->CfgValidationDiscoverOffer)
    {
        _ansc_sprintf(param_value, "TRUE");
    }
    else
    {
        _ansc_sprintf(param_value, "FALSE");
    }
    _PSM_WRITE_PARAM(PSM_WANMANAGER_IF_WAN_VALIDATION_DISCOVERY_OFFER);

    if(p_Interface->CfgValidationSolicitAdvertise)
    {
        _ansc_sprintf(param_value, "TRUE");
    }
    else
    {
        _ansc_sprintf(param_value, "FALSE");
    }
    _PSM_WRITE_PARAM(PSM_WANMANAGER_IF_WAN_VALIDATION_SOLICIT_ADVERTISE);

    if(p_Interface->CfgValidationRsRa)
    {
        _ansc_sprintf(param_value, "TRUE");
    }
    else
    {
        _ansc_sprintf(param_value, "FALSE");
    }
    _PSM_WRITE_PARAM(PSM_WANMANAGER_IF_WAN_VALIDATION_RS_RA);

    if(p_Interface->CfgValidationPadiPado)
    {
        _ansc_sprintf(param_value, "TRUE");
    }
    else
    {
        _ansc_sprintf(param_value, "FALSE");
    }
    _PSM_WRITE_PARAM(PSM_WANMANAGER_IF_WAN_VALIDATION_PADI_PADO);

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfInit
    (
        ANSC_HANDLE                 hDml,
        PANSC_HANDLE                phContext
    )
{
    PDML_WAN_IFACE      pMyObject    = (PDML_WAN_IFACE)phContext;

    //Wan Interface Configuration init
	DmlWanIfConfInit( pMyObject );

#ifdef FEATURE_802_1P_COS_MARKING
    /* Initialize middle layer for Device.X_RDK_WanManager.CPEInterface.{i}.Marking.  */
    DmlWanIfMarkingInit( pMyObject );
#endif /* * FEATURE_802_1P_COS_MARKING */

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfConfInit
    (
        PANSC_HANDLE                phContext
    )
{
    PDATAMODEL_WAN_IFACE  pMyObject    = (PDML_WAN_IFACE)phContext;
    PDML_WAN_IFACE        pWanIfTable  = NULL;
    PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController = NULL;
    INT                        iTotalLines  = 0, iLoopCount   = 0;
    INT                        result       = ANSC_STATUS_FAILURE;

    result = DmlGetTotalNoOfWanInterfaces(&iTotalLines);
    if(result == ANSC_STATUS_FAILURE) {
        return ANSC_STATUS_FAILURE;
    }

    pWanIfTable = (PDML_WAN_IFACE) AnscAllocateMemory( sizeof(DML_WAN_IFACE) * iTotalLines );
    pWanController = (PWAN_CONTROLLER_PRIVATE_SM_INFO) AnscAllocateMemory( sizeof(WAN_CONTROLLER_PRIVATE_SM_INFO));

    //Return failure if allocation failiure
    if( NULL == pWanIfTable )
    {
        return ANSC_STATUS_FAILURE;
    }

    pMyObject->ulTotalNoofWanInterfaces = iTotalLines;

    //Memset all memory
    memset( pWanIfTable, 0, ( sizeof(DML_WAN_IFACE) * iTotalLines ) );
    memset( pWanController, 0, ( sizeof(WAN_CONTROLLER_PRIVATE_SM_INFO) ) );

    //Get static interface configuration from PSM data store
    for( iLoopCount = 0 ; iLoopCount < iTotalLines ; iLoopCount++ )
    {
        pWanIfTable[ iLoopCount ].ulInstanceNumber = iLoopCount + 1;
        pWanIfTable[ iLoopCount ].PhyStatus = WAN_IFACE_PHY_STATUS_DOWN;
        pWanIfTable[ iLoopCount ].CfgActiveLink = FALSE;
        pWanIfTable[ iLoopCount ].CfgStatus = WAN_IFACE_STATUS_DISABLED;
        pWanIfTable[ iLoopCount ].CfgLinkStatus = WAN_IFACE_LINKSTATUS_DOWN;
        pWanIfTable[ iLoopCount ].Ipv4State = WAN_IFACE_IPV4_STATE_DOWN;
        pWanIfTable[ iLoopCount ].Ipv6State = WAN_IFACE_IPV6_STATE_DOWN;
        pWanIfTable[ iLoopCount ].MaptState = WAN_IFACE_MAPT_STATE_DOWN;
        pWanIfTable[ iLoopCount ].DsliteState = WAN_IFACE_DSLITE_STATE_DOWN;
        read_Wan_Interface_ParametersFromPSM((iLoopCount+1), pWanIfTable);
    }

    //Assign the memory address to oringinal structure
    pMyObject->pWanIface = pWanIfTable;
    pMyObject->pWanController = pWanController;

    /**
     * Initialize global structure to be used in the state machine.
     */
    DmlWanIfPrepareGlobalInfo();

    if(WanController_Init_StateMachine(pMyObject) != ANSC_STATUS_SUCCESS) {
        CcspTraceInfo(("%s %d Error: WanController_Init_StateMachine failed \n", __FUNCTION__, __LINE__ ));    
    }

    return ANSC_STATUS_SUCCESS;
}

/* DmlGetTotalNoOfWanInterfaces() */
ANSC_STATUS DmlGetTotalNoOfWanInterfaces(int *wan_if_count)
{
    int ret_val = ANSC_STATUS_SUCCESS;
    int retPsmGet = CCSP_SUCCESS;
    char* param_value = NULL;

    retPsmGet = PSM_Get_Record_Value2(bus_handle,g_Subsystem, PSM_WANMANAGER_WANIFCOUNT, NULL, &param_value);
    if (retPsmGet != CCSP_SUCCESS) { \
        AnscTraceFlow(("%s Error %d reading %s %s\n", __FUNCTION__, retPsmGet, PSM_WANMANAGER_WANIFCOUNT, param_value));
        ret_val = ANSC_STATUS_FAILURE;
    }
    else if(param_value != NULL) {
        _ansc_sscanf(param_value, "%d", wan_if_count); 
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }

	return ret_val;
}

/* DmlGetWanIfCfg() */
ANSC_STATUS DmlGetWanIfCfg( INT LineIndex, PDML_WAN_IFACE pstLineInfo )
{
    return ANSC_STATUS_SUCCESS;
}

/* DmlSetWanIfCfg() */
ANSC_STATUS DmlSetWanIfCfg( INT LineIndex, PDML_WAN_IFACE pstLineInfo )
{
    int ret_val = ANSC_STATUS_SUCCESS;
    ret_val = write_Wan_Interface_ParametersFromPSM(LineIndex, pstLineInfo);
    if(ret_val != ANSC_STATUS_SUCCESS) {
        AnscTraceFlow(("%s Failed!! Error code: %d", __FUNCTION__, ret_val));
    }

    return ret_val;
}

/* DmlSetWanIfValidationCfg() */
ANSC_STATUS DmlSetWanIfValidationCfg( INT WanIfIndex, PDML_WAN_IFACE pWanIfInfo)
{
    int ret_val = ANSC_STATUS_SUCCESS;

    if (NULL == pWanIfInfo)
    {
        AnscTraceFlow(("%s Failed!! Invalid memory \n", __FUNCTION__));
        return ANSC_STATUS_INTERNAL_ERROR;
    }
    ret_val = write_Wan_Interface_Validation_ParametersToPSM(WanIfIndex, pWanIfInfo);
    if(ret_val != ANSC_STATUS_SUCCESS) {
        AnscTraceFlow(("%s Failed!! Error code: %d", __FUNCTION__, ret_val));
    }

    return ret_val;
}

static ANSC_STATUS
DmlWanIfPrepareGlobalInfo()
{

    INT iLoopCount = 0;
    INT TotalWanInterfaces = 0;
    INT retPsmget = CCSP_SUCCESS;
    char *param_value= NULL;
    char param_name[256]= {0};

    DmlGetTotalNoOfWanInterfaces(&TotalWanInterfaces);

    //Allocate memory for Eth Global Status Information
    gpstWanGInfo = (PDML_WAN_IFACE_GLOBAL_CONFIG)AnscAllocateMemory(sizeof(DML_WAN_IFACE_GLOBAL_CONFIG) * TotalWanInterfaces);
    //Return failure if allocation failiure
    if (NULL == gpstWanGInfo)
    {
        return ANSC_STATUS_FAILURE;
    }

    memset(gpstWanGInfo, 0, sizeof(gpstWanGInfo) * TotalWanInterfaces);
    //Assign default value
    for (iLoopCount = 0; iLoopCount < TotalWanInterfaces; ++iLoopCount)
    {
        gpstWanGInfo[iLoopCount].CfgPhyStatus  = WAN_IFACE_PHY_STATUS_DOWN;
        gpstWanGInfo[iLoopCount].CfgStatus  = WAN_IFACE_STATUS_DISABLED;
        gpstWanGInfo[iLoopCount].CfgRefresh = FALSE;
        gpstWanGInfo[iLoopCount].CfgActiveLink = FALSE;
        gpstWanGInfo[iLoopCount].CfgLinkStatus = WAN_IFACE_LINKSTATUS_DOWN;
        gpstWanGInfo[iLoopCount].CfgWanManagerStatus = WAN_MANAGER_DOWN;
        gpstWanGInfo[iLoopCount].CfgIpv4Status = WAN_IFACE_IPV4_STATE_DOWN;
        gpstWanGInfo[iLoopCount].CfgIpv6Status = WAN_IFACE_IPV6_STATE_DOWN;
        gpstWanGInfo[iLoopCount].CfgMAPTStatus = WAN_IFACE_MAPT_STATE_DOWN;
        gpstWanGInfo[iLoopCount].CfgDSLiteStatus = WAN_IFACE_DSLITE_STATE_DOWN;

        //Get wan interface name from psm data store
        _ansc_memset(param_name, 0, sizeof(param_name));
        _ansc_sprintf(param_name, PSM_WANMANAGER_IF_NAME, (iLoopCount+1));
        retPsmget = PSM_Get_Record_Value2(bus_handle,g_Subsystem, param_name, NULL, &param_value);
        if (retPsmget != CCSP_SUCCESS)
        {
            snprintf(gpstWanGInfo[iLoopCount].CfgBaseifName, sizeof(gpstWanGInfo[iLoopCount].CfgBaseifName), " ");
        }
        else
        {
            snprintf(gpstWanGInfo[iLoopCount].CfgBaseifName, sizeof(gpstWanGInfo[iLoopCount].CfgBaseifName), "%s", param_value);
        }
        snprintf(gpstWanGInfo[iLoopCount].CfgWanName, sizeof(gpstWanGInfo[iLoopCount].CfgWanName), "%s", DEFAULT_WAN_IF_NAME);
        //Get wan interface type from psm data store
        _ansc_memset(param_name, 0, sizeof(param_name));
        _ansc_sprintf(param_name, PSM_WANMANAGER_IF_TYPE, (iLoopCount+1));
        retPsmget = PSM_Get_Record_Value2(bus_handle,g_Subsystem, param_name, NULL, &param_value);
        if (retPsmget == CCSP_SUCCESS)
        {
            gpstWanGInfo[iLoopCount].CfgType = atoi(param_value);
        }
        //Get wan interface priority from psm data store
        _ansc_memset(param_name, 0, sizeof(param_name));
        _ansc_sprintf(param_name, PSM_WANMANAGER_IF_PRIORITY, (iLoopCount+1));
        retPsmget = PSM_Get_Record_Value2(bus_handle,g_Subsystem, param_name, NULL, &param_value);
        if (retPsmget == CCSP_SUCCESS)
        {
            gpstWanGInfo[iLoopCount].CfgPriority = atoi(param_value);
        }
        //Get WAN Enable value from PSM store
        _ansc_memset(param_name, 0, sizeof(param_name));
        _ansc_sprintf(param_name, PSM_WANMANAGER_IF_ENABLE, (iLoopCount+1));
        retPsmget = PSM_Get_Record_Value2(bus_handle,g_Subsystem, param_name, NULL, &param_value);
        if (retPsmget == CCSP_SUCCESS)
        {
            if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0) {
                gpstWanGInfo[iLoopCount].CfgEnable = TRUE;
            }
            else {
                gpstWanGInfo[iLoopCount].CfgEnable = FALSE;
            }
        }
        //Get MAP-T Enable value from PSM store
        _ansc_memset(param_name, 0, sizeof(param_name));
        _ansc_sprintf(param_name, PSM_WANMANAGER_IF_WAN_ENABLE_MAPT, (iLoopCount+1));
        retPsmget = PSM_Get_Record_Value2(bus_handle,g_Subsystem, param_name, NULL, &param_value);
        if (retPsmget == CCSP_SUCCESS)
        {
            if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0) {
                gpstWanGInfo[iLoopCount].CfgEnableMAPT = TRUE;
            }
            else {
                gpstWanGInfo[iLoopCount].CfgEnableMAPT = FALSE;
            }
        }
        //Get DSLite Enable value from PSM store
        _ansc_memset(param_name, 0, sizeof(param_name));
        _ansc_sprintf(param_name, PSM_WANMANAGER_IF_WAN_ENABLE_DSLITE, (iLoopCount+1));
        retPsmget = PSM_Get_Record_Value2(bus_handle,g_Subsystem, param_name, NULL, &param_value);
        if (retPsmget == CCSP_SUCCESS)
        {
            if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0) {
                gpstWanGInfo[iLoopCount].CfgEnableDSLite = TRUE;
            }
            else {
                gpstWanGInfo[iLoopCount].CfgEnableDSLite = FALSE;
            }
        }
        //Get IPoE Enable value from PSM store
        _ansc_memset(param_name, 0, sizeof(param_name));
        _ansc_sprintf(param_name, PSM_WANMANAGER_IF_WAN_ENABLE_IPOE, (iLoopCount+1));
        retPsmget = PSM_Get_Record_Value2(bus_handle,g_Subsystem, param_name, NULL, &param_value);
        if (retPsmget == CCSP_SUCCESS)
        {
            if(strcmp(param_value, PSM_ENABLE_STRING_TRUE) == 0) {
                gpstWanGInfo[iLoopCount].CfgEnableIPoE = TRUE;
            }
            else {
                gpstWanGInfo[iLoopCount].CfgEnableIPoE = FALSE;
            }
        }
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfSetCfgStatus(INT IfIndex, DML_WAN_IFACE_STATUS status)
{

    if (IfIndex < 0)
    {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgStatus. */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    gpstWanGInfo[IfIndex].CfgStatus = status;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfSetCfgLinkStatus(INT IfIndex, DML_WAN_IFACE_LINKSTATUS status)
{
    if (IfIndex < 0)
    {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }
    /* Set Wan CfgLinkStatus. */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    gpstWanGInfo[IfIndex].CfgLinkStatus = status;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfGetCfgStatus(INT IfIndex, DML_WAN_IFACE_STATUS *status)
{

    if (IfIndex < 0)
    {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Get Wan CfgStatus. */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    *status = gpstWanGInfo[IfIndex].CfgStatus;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfGetIndexFromIfName(char *ifname, INT *IfIndex)
{

    INT                         iLoopCount;
    PDATAMODEL_WAN_IFACE   pMyObject         = (PDATAMODEL_WAN_IFACE)g_pBEManager->hWanIface;
    INT                         iTotalInterfaces  = pMyObject->ulTotalNoofWanInterfaces;

    if (NULL == ifname || IfIndex == NULL || gpstWanGInfo == NULL)
    {
        CcspTraceError(("Invalid Memory \n"));
        return ANSC_STATUS_FAILURE;
    }

    *IfIndex = -1;
    pthread_mutex_lock(&gmWanGInfo_mutex);
    for (iLoopCount = 0; iLoopCount < iTotalInterfaces; iLoopCount++)
    {
        if (0 == strncmp(gpstWanGInfo[iLoopCount].CfgBaseifName, ifname, strlen(ifname)))
        {
            *IfIndex = iLoopCount;
            pthread_mutex_unlock(&gmWanGInfo_mutex);
            return ANSC_STATUS_SUCCESS;
        }
    }
    pthread_mutex_unlock(&gmWanGInfo_mutex);
    return ANSC_STATUS_FAILURE;
}

INT DmlWanIfCheckDataModelInitialised()
{
    INT status = 0;
    PDATAMODEL_WAN_IFACE pMyObject = (PDATAMODEL_WAN_IFACE)g_pBEManager->hWanIface;
    if(pMyObject == NULL)
    {
        status = 0;
    }
    else
    {
        status = 1;
    }
    return status;
}

ANSC_STATUS
DmlWanIfGetCopyOfGlobalData(PDML_WAN_IFACE_GLOBAL_CONFIG pGlobalInfo)
{
    ANSC_STATUS                 retStatus         = ANSC_STATUS_SUCCESS;
    PDATAMODEL_WAN_IFACE   pMyObject         = (PDATAMODEL_WAN_IFACE)g_pBEManager->hWanIface;
    INT                         iTotalInterfaces  = pMyObject->ulTotalNoofWanInterfaces;

    //Copy of the data
    pthread_mutex_lock(&gmWanGInfo_mutex);
    memcpy(pGlobalInfo, gpstWanGInfo, sizeof(DML_WAN_IFACE_GLOBAL_CONFIG)*iTotalInterfaces);
    pthread_mutex_unlock(&gmWanGInfo_mutex);
    return retStatus;
}

ANSC_STATUS
DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(char *ifname, PDML_WAN_IFACE_GLOBAL_CONFIG pGlobalInfo)
{
    ANSC_STATUS   retStatus;
    INT           wanIndex = -1;
    if ((NULL == pGlobalInfo) || (ifname == NULL))
    {
        CcspTraceError(("%s Invalid data \n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }
    retStatus = DmlWanIfGetIndexFromIfName(ifname, &wanIndex);

    if ( (ANSC_STATUS_FAILURE == retStatus ) || ( -1 == wanIndex ) )
    {
        CcspTraceError(("%s Failed to get index for %s\n", __FUNCTION__,ifname));
        return ANSC_STATUS_FAILURE;
    }
    //Copy of the data
    pthread_mutex_lock(&gmWanGInfo_mutex);
    memcpy(pGlobalInfo, &gpstWanGInfo[wanIndex], sizeof(DML_WAN_IFACE_GLOBAL_CONFIG));
    pthread_mutex_unlock(&gmWanGInfo_mutex);
    return (ANSC_STATUS_SUCCESS);
}

ANSC_STATUS
DmlWanIfUpdateLinkStatusForGivenIfName(char *ifname, DML_WAN_IFACE_LINKSTATUS status)
{
    ANSC_STATUS   retStatus;
    INT           wanIndex = -1;
    if (ifname == NULL)
    {
        CcspTraceError(("%s Invalid data \n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }
    retStatus = DmlWanIfGetIndexFromIfName(ifname, &wanIndex);

    if ( (ANSC_STATUS_FAILURE == retStatus ) || ( -1 == wanIndex ) )
    {
        CcspTraceError(("%s Failed to get index for %s\n", __FUNCTION__, ifname));
        return ANSC_STATUS_FAILURE;
    }
    //Copy of the data
    pthread_mutex_lock(&gmWanGInfo_mutex);
    gpstWanGInfo[wanIndex].CfgLinkStatus = status;
    pthread_mutex_unlock(&gmWanGInfo_mutex);
    return (ANSC_STATUS_SUCCESS);
}
ANSC_STATUS
DmlWanIfUpdateWanStatusForGivenIfName(char *ifname, DML_WAN_IFACE_STATUS status)
{
    ANSC_STATUS             retStatus;
    INT                     wanIndex     = -1;
    PDATAMODEL_WAN_IFACE    pMyObject    = (PDATAMODEL_WAN_IFACE)g_pBEManager->hWanIface;
    PDML_WAN_IFACE          p_Interface  = NULL;
    INT                     wan_if_count = 0;
    INT                     iLoopCount   = 0;

    if (ifname == NULL)
    {
        CcspTraceError(("%s Invalid data \n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }

    p_Interface = pMyObject->pWanIface;
    if (p_Interface == NULL)
    {
        CcspTraceError(("%s p_Interface is NULL \n", __FUNCTION__ ));
        return ANSC_STATUS_FAILURE;
    }

    wan_if_count = pMyObject->ulTotalNoofWanInterfaces;

    retStatus = DmlWanIfGetIndexFromIfName(ifname, &wanIndex);
    if ( (ANSC_STATUS_FAILURE == retStatus ) || ( -1 == wanIndex ) )
    {
        CcspTraceError(("%s Failed to get index for %s\n", __FUNCTION__, ifname));
        return ANSC_STATUS_FAILURE;
    }

    for( iLoopCount = 0 ; iLoopCount < wan_if_count ; iLoopCount++ )
    {
        if (p_Interface[iLoopCount].ulInstanceNumber == (wanIndex + 1))
        {
            p_Interface[iLoopCount].CfgStatus = status;
            /**
            * Update global wan data object for the state machine.
            */
            DmlWanIfSetCfgStatus(iLoopCount, status);
            break;
        }
    }

    return (ANSC_STATUS_SUCCESS);
}

ANSC_STATUS
DmlWanIfUpdateRefreshFlagForGivenIfName(char *ifname, BOOL  status)
{
    ANSC_STATUS retStatus;
    INT wanIndex = -1;

    if (ifname == NULL)
    {
        CcspTraceError(("%s Invalid data \n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }
    retStatus = DmlWanIfGetIndexFromIfName(ifname, &wanIndex);

    if ( (ANSC_STATUS_FAILURE == retStatus ) || ( -1 == wanIndex ) )
    {
        CcspTraceError(("%s Failed to get index for %s\n", __FUNCTION__, ifname));
        return ANSC_STATUS_FAILURE;
    }
    //Copy of the data
    pthread_mutex_lock(&gmWanGInfo_mutex);
    gpstWanGInfo[wanIndex].CfgRefresh = status;
    pthread_mutex_unlock(&gmWanGInfo_mutex);

    return (ANSC_STATUS_SUCCESS);
}

ANSC_STATUS
DmlWanIfUpdateActiveLinkFlagForGivenIfName(char *ifname, BOOL status)
{
    ANSC_STATUS retStatus;
    INT wanIndex = -1;

    if (ifname == NULL)
    {
        CcspTraceError(("%s Invalid data \n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }
    retStatus = DmlWanIfGetIndexFromIfName(ifname, &wanIndex);

    if ( (ANSC_STATUS_FAILURE == retStatus ) || ( -1 == wanIndex ) )
    {
        CcspTraceError(("%s Failed to get index for %s\n", __FUNCTION__, ifname));
        return ANSC_STATUS_FAILURE;
    }
    //Copy of the data
    pthread_mutex_lock(&gmWanGInfo_mutex);
    gpstWanGInfo[wanIndex].CfgActiveLink = status;
    pthread_mutex_unlock(&gmWanGInfo_mutex);

    return (ANSC_STATUS_SUCCESS);
}




ANSC_STATUS
DmlWanIfSetCfgRefreshStatus(INT IfIndex, BOOL refresh)
{

    if (IfIndex < 0)
    {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgRefresh Status. */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    gpstWanGInfo[IfIndex].CfgRefresh = refresh;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfGetCfgRefreshStatus(INT IfIndex, BOOL *refresh)
{

    if (IfIndex < 0)
    {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgRefresh Status. */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    *refresh = gpstWanGInfo[IfIndex].CfgRefresh;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfSetCfgName(INT IfIndex, char* name)
{
    if (IfIndex < 0)
    {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan Cfg Name. */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    AnscCopyString(gpstWanGInfo[IfIndex].CfgWanName, name);
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfGetCfgName(INT IfIndex, char* name)
{
    if (IfIndex < 0)
    {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan Cfg Name. */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    AnscCopyString(name, gpstWanGInfo[IfIndex].CfgWanName);
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfSetCfgActiveLink(INT IfIndex, BOOL status)
{
    if (IfIndex < 0)
    {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgActiveLink Status */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    if (gpstWanGInfo[IfIndex].CfgActiveLink != status )
    {
        gpstWanGInfo[IfIndex].CfgActiveLinkChanged = true;
    }
    gpstWanGInfo[IfIndex].CfgActiveLink = status;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfSetCfgActiveLinkChanged(INT IfIndex)
{
    if (IfIndex < 0)
    {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgActiveLinkChanged Status */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    gpstWanGInfo[IfIndex].CfgActiveLinkChanged = true;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfResetCfgActiveLinkChanged(char *ifname)
{
    ANSC_STATUS   retStatus;
    INT           wanIndex = -1;

    if (ifname == NULL)
    {
        CcspTraceError(("%s Invalid data \n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }
    retStatus = DmlWanIfGetIndexFromIfName(ifname, &wanIndex);

    if ( (ANSC_STATUS_FAILURE == retStatus ) || ( -1 == wanIndex ) )
    {
        CcspTraceError(("%s Failed to get index for %s\n", __FUNCTION__, ifname));
        return ANSC_STATUS_FAILURE;
    }
    //Copy of the data
    pthread_mutex_lock(&gmWanGInfo_mutex);
    gpstWanGInfo[wanIndex].CfgActiveLinkChanged = false;
    pthread_mutex_unlock(&gmWanGInfo_mutex);

    return (ANSC_STATUS_SUCCESS);
}

ANSC_STATUS
DmlWanIfGetCfgActiveLink(INT IfIndex, BOOL *status)
{
    if (IfIndex < 0)
    {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgRefresh Status. */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    *status = gpstWanGInfo[IfIndex].CfgActiveLink;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfGetCfgEnable(INT IfIndex, BOOL *enable) {
    if (IfIndex < 0) {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgEnable parameter */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    *enable = gpstWanGInfo[IfIndex].CfgEnable;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfSetCfgEnable(INT IfIndex, BOOL status) {
    if (IfIndex < 0) {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgEnable parameter */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    gpstWanGInfo[IfIndex].CfgEnable = status;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfGetCfgType(INT IfIndex, DML_WAN_IFACE_TYPE *type) {
    if (IfIndex < 0) {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgType parameter */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    *type = gpstWanGInfo[IfIndex].CfgType;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfSetCfgType(INT IfIndex, DML_WAN_IFACE_TYPE type) {
    if (IfIndex < 0) {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgType parameter */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    gpstWanGInfo[IfIndex].CfgType = type;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfGetCfgPriority(INT IfIndex, INT *priority) {
    if (IfIndex < 0) {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgPriority parameter */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    *priority = gpstWanGInfo[IfIndex].CfgPriority;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfSetCfgPriority(INT IfIndex, INT priority) {
    if (IfIndex < 0) {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgPriority parameter */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    gpstWanGInfo[IfIndex].CfgPriority = priority;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfGetCfgPhyStatus(INT IfIndex, DML_WAN_IFACE_PHY_STATUS *status) {
    if (IfIndex < 0) {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgPhyStatus parameter */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    *status = gpstWanGInfo[IfIndex].CfgPhyStatus;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfSetCfgPhyStatus(INT IfIndex, DML_WAN_IFACE_PHY_STATUS status) {
    if (IfIndex < 0) {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgPhyStatus parameter */
    pthread_mutex_trylock (&gmWanGInfo_mutex);
    gpstWanGInfo[IfIndex].CfgPhyStatus = status;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfGetCfgPhyPath(INT IfIndex, char *path) {
    if (IfIndex < 0) {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgPhyPath parameter */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    strncpy(path, gpstWanGInfo[IfIndex].CfgPhyPath, sizeof(gpstWanGInfo[IfIndex].CfgPhyPath));
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfSetCfgPhyPath(INT IfIndex, char *path) {
    if (IfIndex < 0) {
        CcspTraceError(("%s Invalid index[%d]\n", __FUNCTION__, IfIndex));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgPhyPath parameter */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    strncpy(gpstWanGInfo[IfIndex].CfgPhyPath, path, sizeof(gpstWanGInfo[IfIndex].CfgPhyPath));
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfSetCfgIpv4Status(char *ifname, DML_WAN_IFACE_IPV4_STATE status)
{
    ANSC_STATUS   retStatus;
    INT           wanIndex = -1;
    if (ifname == NULL)
    {
        CcspTraceError(("%s Invalid data \n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }
    retStatus = DmlWanIfGetIndexFromIfName(ifname, &wanIndex);

    if ( (ANSC_STATUS_FAILURE == retStatus ) || ( -1 == wanIndex ) )
    {
        CcspTraceError(("%s Failed to get index for %s\n", __FUNCTION__,ifname));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan CfgIpv4Status Status. */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    gpstWanGInfo[wanIndex].CfgIpv4Status = status;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfSetCfgIpv6Status(char *ifname, DML_WAN_IFACE_IPV6_STATE status)
{
    ANSC_STATUS   retStatus;
    INT           wanIndex = -1;
    if (ifname == NULL)
    {
        CcspTraceError(("%s Invalid data \n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }
    retStatus = DmlWanIfGetIndexFromIfName(ifname, &wanIndex);

    if ( (ANSC_STATUS_FAILURE == retStatus ) || ( -1 == wanIndex ) )
    {
        CcspTraceError(("%s Failed to get index for %s\n", __FUNCTION__,ifname));
        return ANSC_STATUS_FAILURE;
    }

    /* Set Wan Ipv6 Status */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    gpstWanGInfo[wanIndex].CfgIpv6Status = status;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfSetCfgMAPTStatus(char *ifname, DML_WAN_IFACE_MAPT_STATE status)
{
    ANSC_STATUS   retStatus;
    INT wanIndex = -1;
    if (ifname == NULL)
    {
        CcspTraceError(("%s Invalid data \n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }
    retStatus = DmlWanIfGetIndexFromIfName(ifname, &wanIndex);

    if ( (ANSC_STATUS_FAILURE == retStatus ) || ( -1 == wanIndex ) )
    {
        CcspTraceError(("%s Failed to get index for %s\n", __FUNCTION__,ifname));
        return ANSC_STATUS_FAILURE;
    }

    /* Set MAPT Status */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    gpstWanGInfo[wanIndex].CfgMAPTStatus = status;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
DmlWanIfSetCfgDSLiteStatus(char *ifname, DML_WAN_IFACE_DSLITE_STATE status)
{
    ANSC_STATUS   retStatus;
    INT wanIndex = -1;
    if (ifname == NULL)
    {
        CcspTraceError(("%s Invalid data \n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }

    retStatus = DmlWanIfGetIndexFromIfName(ifname, &wanIndex);

    if ( (ANSC_STATUS_FAILURE == retStatus ) || ( -1 == wanIndex ) )
    {
        CcspTraceError(("%s Failed to get index for %s\n", __FUNCTION__,ifname));
        return ANSC_STATUS_FAILURE;
    }

    /* Set DSLite Status */
    pthread_mutex_lock (&gmWanGInfo_mutex);
    gpstWanGInfo[wanIndex].CfgDSLiteStatus = status;
    pthread_mutex_unlock (&gmWanGInfo_mutex);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS WanManager_updateWanInterfaceUpstreamFlag(char *phyPath, BOOL flag)
{
    char param_name[256] = {0};
    char param_value[256] = {0};
    char  pComponentName[64] = {0};
    char  pComponentPath[64] = {0};
    char *faultParam = NULL;
    int ret = 0;

    CCSP_MESSAGE_BUS_INFO *bus_info = (CCSP_MESSAGE_BUS_INFO *)bus_handle;
    parameterValStruct_t upstream_param[1] = {0};

    if(phyPath == NULL) {
        CcspTraceInfo(("%s %d Error: phyPath is NULL \n", __FUNCTION__, __LINE__ ));
        return ANSC_STATUS_FAILURE;
    }

    strncpy(param_name, phyPath, sizeof(param_name));

    if(strstr(param_name, "DSL") != NULL) { // dsl wan interface
        strncat(param_name, DSL_UPSTREAM_NAME, sizeof(param_name));
        strncpy(pComponentName, DSL_COMPONENT_NAME, sizeof(pComponentName));
        strncpy(pComponentPath, DSL_COMPONENT_PATH, sizeof(pComponentPath));
    }
    else if(strstr(param_name, "Ethernet") != NULL) { // ethernet wan interface
        strncat(param_name, ETH_UPSTREAM_NAME, sizeof(param_name));
        strncpy(pComponentName, ETH_COMPONENT_NAME, sizeof(pComponentName));
        strncpy(pComponentPath, ETH_COMPONENT_PATH, sizeof(pComponentPath));
    }
    if(flag)
        strncpy(param_value, "true", sizeof(param_value));
    else
        strncpy(param_value, "false", sizeof(param_value));

    upstream_param[0].parameterName = param_name;
    upstream_param[0].parameterValue = param_value;
    upstream_param[0].type = ccsp_boolean;

    ret = CcspBaseIf_setParameterValues(bus_handle, pComponentName, pComponentPath,
                                        0, 0x0,   /* session id and write id */
                                        upstream_param, 1, TRUE,   /* Commit  */
                                        &faultParam);

    if ( ( ret != CCSP_SUCCESS ) && ( faultParam )) {
        CcspTraceInfo(("%s CcspBaseIf_setParameterValues failed with error %d\n",__FUNCTION__, ret ));
        bus_info->freefunc( faultParam );
        return ANSC_STATUS_FAILURE;
    }

    return ANSC_STATUS_SUCCESS;
}

/** DmlWanIfSetIPState() */
ANSC_STATUS
DmlWanIfSetIPState
    (
        const char *wanIfName,
        DML_WAN_IFACE_IP_STATE_TYPE ipStateType,
        DML_WAN_IFACE_IP_STATE ipState
    )
{
    PDATAMODEL_WAN_IFACE pMyObject = (PDATAMODEL_WAN_IFACE)g_pBEManager->hWanIface;
    PDML_WAN_IFACE p_Interface = NULL;
    int wan_if_count = 0;
    int iLoopCount = 0;

    if (wanIfName == NULL || pMyObject == NULL)
    {
        CcspTraceError(("%s - Invalid memory \n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }

    p_Interface = pMyObject->pWanIface;
    wan_if_count = pMyObject->ulTotalNoofWanInterfaces;

    for (iLoopCount = 0; iLoopCount < wan_if_count; iLoopCount++)
    {
        if (strcmp(p_Interface[iLoopCount].CfgName, wanIfName) == 0)
        {
            switch (ipStateType)
            {
                case WAN_IFACE_IPV4_STATE:
                {
                    p_Interface[iLoopCount].Ipv4State = ipState;
                    DmlWanIfSetCfgIpv4Status(p_Interface[iLoopCount].Name, ipState);
                    break;
                }
                case WAN_IFACE_IPV6_STATE:
                {
                    p_Interface[iLoopCount].Ipv6State = ipState;
                    DmlWanIfSetCfgIpv6Status(p_Interface[iLoopCount].Name, ipState);
                    break;
                }
                case WAN_IFACE_MAPT_STATE:
                {
                    p_Interface[iLoopCount].MaptState = ipState;
                    DmlWanIfSetCfgMAPTStatus(p_Interface[iLoopCount].Name, ipState);
                    break;
                }
                case WAN_IFACE_DSLITE_STATE:
                {
                    p_Interface[iLoopCount].DSLiteState = ipState;
                    DmlWanIfSetCfgDSLiteStatus(p_Interface[iLoopCount].Name, ipState);
                }
                default:
                    break;
            }
        }
    }

    return ANSC_STATUS_SUCCESS;
}

static ANSC_STATUS DmlWanGetParamNames( char *pComponent, char *pBus, char *pParamName, char a2cReturnVal[][256], int *pReturnSize )
{
    CCSP_MESSAGE_BUS_INFO  *bus_info         = (CCSP_MESSAGE_BUS_INFO *)bus_handle;
    parameterInfoStruct_t  **retInfo;
    char                    *ParamName[ 1 ];
    int                    ret               = 0,
                           nval;

    ret = CcspBaseIf_getParameterNames(
                                    bus_handle,
                                    pComponent,
                                    pBus,
                                    pParamName,
                                    1,
                                    &nval,
                                    &retInfo);

    //Copy the value
    if( CCSP_SUCCESS == ret )
    {
        int iLoopCount;

        *pReturnSize = nval;

        for( iLoopCount = 0; iLoopCount < nval; iLoopCount++ )
        {
           if( NULL != retInfo[iLoopCount]->parameterName )
           {
              //CcspTraceInfo(("%s parameterName[%d,%s]\n",__FUNCTION__,iLoopCount,retInfo[iLoopCount]->parameterName));
              snprintf( a2cReturnVal[iLoopCount], strlen( retInfo[iLoopCount]->parameterName ) + 1, "%s", retInfo[iLoopCount]->parameterName );
           }
        }

        if( retInfo )
        {
          free_parameterInfoStruct_t(bus_handle, nval, retInfo);
        }

        return ANSC_STATUS_SUCCESS;
    }

    if( retInfo )
    {
      free_parameterInfoStruct_t(bus_handle, nval, retInfo);
    }

    return ANSC_STATUS_FAILURE;
}

static ANSC_STATUS DmlWanGetParamValues( char *pComponent, char *pBus, char *pParamName, char *pReturnVal )
{
    CCSP_MESSAGE_BUS_INFO  *bus_info         = (CCSP_MESSAGE_BUS_INFO *)bus_handle;
    parameterValStruct_t   **retVal;
    char                    *ParamName[ 1 ];
    int                    ret               = 0,
                           nval;

    //Assign address for get parameter name
    ParamName[0] = pParamName;

    ret = CcspBaseIf_getParameterValues(
                                    bus_handle,
                                    pComponent,
                                    pBus,
                                    ParamName,
                                    1,
                                    &nval,
                                    &retVal);

    //Copy the value
    if( CCSP_SUCCESS == ret )
    {
        //CcspTraceInfo(("%s parameterValue[%s]\n",__FUNCTION__,retVal[0]->parameterValue));

        if( NULL != retVal[0]->parameterValue )
        {
            memcpy( pReturnVal, retVal[0]->parameterValue, strlen( retVal[0]->parameterValue ) + 1 );
        }

        if( retVal )
        {
            free_parameterValStruct_t (bus_handle, nval, retVal);
        }

        return ANSC_STATUS_SUCCESS;
    }

    if( retVal )
    {
       free_parameterValStruct_t (bus_handle, nval, retVal);
    }

    return ANSC_STATUS_FAILURE;
}

static ANSC_STATUS DmlWanSetParamValues( char *pComponent, char *pBus, char *pParamName, char *pParamVal, enum dataType_e type, BOOLEAN bCommit )
{
    CCSP_MESSAGE_BUS_INFO *bus_info              = (CCSP_MESSAGE_BUS_INFO *)bus_handle;
    parameterValStruct_t   param_val[1]          = { 0 };
    char                  *faultParam            = NULL;
    char                   acParameterName[256]  = { 0 },
                           acParameterValue[128] = { 0 };
    int                    ret                   = 0;

    //Copy Name
    sprintf( acParameterName, "%s", pParamName );
    param_val[0].parameterName  = acParameterName;

    //Copy Value
    sprintf( acParameterValue, "%s", pParamVal );
    param_val[0].parameterValue = acParameterValue;

    //Copy Type
    param_val[0].type           = type;

    ret = CcspBaseIf_setParameterValues(
                                        bus_handle,
                                        pComponent,
                                        pBus,
                                        0,
                                        0,
                                        param_val,
                                        1,
                                        bCommit,
                                        &faultParam
                                       );

    if( ( ret != CCSP_SUCCESS ) && ( faultParam != NULL ) )
    {
        CcspTraceError(("%s-%d Failed to set %s\n",__FUNCTION__,__LINE__,pParamName));
        bus_info->freefunc( faultParam );
        return ANSC_STATUS_FAILURE;
    }

    return ANSC_STATUS_SUCCESS;
}

/* * DmlWanGetInterfaceInstanceInOtherAgent() */
static ANSC_STATUS DmlWanGetInterfaceInstanceInOtherAgent( WAN_NOTIFY_ENUM enNotifyAgent, char *pIfName, INT *piInstanceNumber )
{
    //Validate buffer
    if( ( NULL == pIfName ) || ( NULL == piInstanceNumber ) )
    {
        CcspTraceError(("%s Invalid Buffer\n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }

    //Initialise default value
    *piInstanceNumber   = -1;

    switch( enNotifyAgent )
    {
        case NOTIFY_TO_VLAN_AGENT:
        {
            char acTmpReturnValue[ 256 ]    = { 0 },
                 a2cTmpTableParams[10][256] = { 0 };
            INT  iLoopCount,
                 iTotalNoofEntries;

            if ( ANSC_STATUS_FAILURE == DmlWanGetParamValues( VLAN_COMPONENT_NAME, VLAN_DBUS_PATH, VLAN_ETHLINK_NOE_PARAM_NAME, acTmpReturnValue ) )
            {
                CcspTraceError(("%s %d Failed to get param value\n", __FUNCTION__, __LINE__));
                return ANSC_STATUS_FAILURE;
            }

            //Total count
            iTotalNoofEntries = atoi( acTmpReturnValue );
            CcspTraceInfo(("%s %d - TotalNoofEntries:%d\n", __FUNCTION__, __LINE__, iTotalNoofEntries));

            if( 0 >= iTotalNoofEntries )
            {
               return ANSC_STATUS_SUCCESS;
            }

            //Get table names
            iTotalNoofEntries = 0;
            if ( ANSC_STATUS_FAILURE == DmlWanGetParamNames( VLAN_COMPONENT_NAME, VLAN_DBUS_PATH, VLAN_ETHLINK_TABLE_NAME, a2cTmpTableParams , &iTotalNoofEntries ))
            {
                CcspTraceError(("%s %d Failed to get param value\n", __FUNCTION__, __LINE__));
                return ANSC_STATUS_FAILURE;
            }

            //Traverse from loop
            for ( iLoopCount = 0; iLoopCount < iTotalNoofEntries; iLoopCount++ )
            {
                char acTmpQueryParam[256] = { 0 };

                //Query
                snprintf( acTmpQueryParam, sizeof(acTmpQueryParam ), "%sAlias", a2cTmpTableParams[ iLoopCount ] );

                memset( acTmpReturnValue, 0, sizeof( acTmpReturnValue ) );
                if ( ANSC_STATUS_FAILURE == DmlWanGetParamValues( VLAN_COMPONENT_NAME, VLAN_DBUS_PATH, acTmpQueryParam, acTmpReturnValue ) )
                {
                    CcspTraceError(("%s %d Failed to get param value\n", __FUNCTION__, __LINE__));
                    continue;
                }

                //Compare ifname
                if( 0 == strcmp( acTmpReturnValue, pIfName ) )
                {
                    char  tmpTableParam[ 256 ] = { 0 };
                    const char *last_two;

                    //Copy table param
                    snprintf( tmpTableParam, sizeof(tmpTableParam), "%s", a2cTmpTableParams[ iLoopCount ] );

                    //Get last two chareters from return value and cut the instance
                    last_two = &tmpTableParam[strlen(tmpTableParam) - 2];

                    *piInstanceNumber   = atoi(last_two);
                    break;
                }
            }
        }
        break; /* * NOTIFY_TO_VLAN_AGENT */

        default:
        {
            CcspTraceError(("%s Invalid Case\n", __FUNCTION__));
        }
        break; /* * default */
    }

    return ANSC_STATUS_SUCCESS;
}

/* * DmlWanIfRefreshHandlingThread() */
static void* DmlWanIfRefreshHandlingThread( void *arg )
{
    PDML_WAN_IFACE    pstWanIface = (PDML_WAN_IFACE)arg; 
    char    acSetParamName[ 256 ];
    INT     iVLANInstance   = -1;

    //Validate buffer
    if( NULL == pstWanIface )
    {
        CcspTraceError(("%s Invalid Memory\n", __FUNCTION__));
        pthread_exit(NULL);
    }

    //detach thread from caller stack
    pthread_detach(pthread_self());

    //Need to sync with the state machine thread.
    sleep(5);

    //Get Instance for corresponding name
    DmlWanGetInterfaceInstanceInOtherAgent( NOTIFY_TO_VLAN_AGENT, pstWanIface->Name, &iVLANInstance );

    //Index is not present. so no need to do anything any VLAN instance
    if( -1 == iVLANInstance )
    {
        CcspTraceError(("%s %d VLAN instance not present\n", __FUNCTION__, __LINE__));
        goto EXIT;
    }

    CcspTraceInfo(("%s %d VLAN Instance:%d\n",__FUNCTION__, __LINE__,iVLANInstance));

    //Set VLAN EthLink Refresh
    memset( acSetParamName, 0, sizeof(acSetParamName) );
    snprintf( acSetParamName, sizeof(acSetParamName), VLAN_ETHLINK_REFRESH_PARAM_NAME, iVLANInstance );
    DmlWanSetParamValues( VLAN_COMPONENT_NAME, VLAN_DBUS_PATH, acSetParamName, "true", ccsp_boolean, TRUE );

    CcspTraceInfo(("%s %d Successfully notified refresh event to VLAN Agent for %s interface[%s]\n", __FUNCTION__, __LINE__, pstWanIface->Name,acSetParamName));

EXIT:

    //Free allocated resource
    if( NULL != pstWanIface )
    {
        free(pstWanIface);
        pstWanIface = NULL;
    }

    pthread_exit(NULL);

    return NULL;
}

/* DmlWanIfSetWanRefresh() */
ANSC_STATUS DmlWanIfSetWanRefresh( PDML_WAN_IFACE pstWanIface )
{
    ANSC_STATUS           returnStatus        = ANSC_STATUS_SUCCESS;
    PDML_WAN_IFACE   pstWanIface4Thread  = NULL;
    pthread_t             refreshThreadId;
    INT                   iErrorCode          = -1;

    /*
     *
     *  WAN.Refresh Impl
     *  ----------------
     *  When Refresh is set it as "0"
     *  Do nothing
     *
     *  When Refresh is set it as "1"
     *  1. Do nothing. If Wan.Status is "Down" but Save it in DML as "1"
     *  2. Do nothing. If Wan.Status is "Refreshing" but save it in DML as "1"
     *  3. Do refresh. If Wan.Status is "Up" then Change Wan.Status as "Refreshing" and
     *                 Immediately reset it as "0" to toggle again for other refresh.
     *                 Once finished then needs to set back Wan.Status as "Up" from VLAN agent after refresh.  
     *
     */

     if( FALSE == pstWanIface->CfgRefresh )
     {
         CcspTraceInfo(("%s %d - Wan interface(%s) Refresh flag is false . so no need to continue refresh process\n",__FUNCTION__,__LINE__,pstWanIface->Name));
         return returnStatus;
     }

     if( ( WAN_IFACE_LINKSTATUS_DOWN == pstWanIface->CfgLinkStatus ) || 
         ( WAN_IFACE_LINKSTATUS_CONFIGURING == pstWanIface->CfgLinkStatus ) )
     {
         CcspTraceInfo(("%s %d - Wan interface(%s) Status is Down . so no need to continue refresh process\n",__FUNCTION__,__LINE__,pstWanIface->Name));
         return returnStatus;
     }

     if( WAN_IFACE_LINKSTATUS_UP == pstWanIface->CfgLinkStatus )
     {
         //Change config status as refreshing
         pstWanIface->CfgLinkStatus = WAN_IFACE_LINKSTATUS_CONFIGURING;

         //Change refresh flag as false
         pstWanIface->CfgRefresh = FALSE;

         //After that needs to configure current WAN refresh and WAN Link status to global structure
         DmlWanIfSetCfgLinkStatus(pstWanIface->ulInstanceNumber - 1, pstWanIface->CfgLinkStatus);
         DmlWanIfSetCfgRefreshStatus(pstWanIface->ulInstanceNumber - 1, pstWanIface->CfgRefresh);

         //Allocate memory for interface struct
         pstWanIface4Thread = (PDML_WAN_IFACE)malloc(sizeof(DML_WAN_IFACE));
         if( NULL == pstWanIface4Thread )
         {
             CcspTraceError(("%s %d Failed to allocate memory\n", __FUNCTION__, __LINE__));
             return ANSC_STATUS_FAILURE;
         }

         //Copy WAN interface structure for thread
         memset( pstWanIface4Thread, 0, sizeof(DML_WAN_IFACE));
         memcpy( pstWanIface4Thread, pstWanIface, sizeof(DML_WAN_IFACE) );

         //WAN refresh thread
         iErrorCode = pthread_create( &refreshThreadId, NULL, &DmlWanIfRefreshHandlingThread, (void*)pstWanIface4Thread );
         if( 0 != iErrorCode )
         {
             CcspTraceInfo(("%s %d - Failed to start WAN refresh thread EC:%d\n", __FUNCTION__, __LINE__, iErrorCode ));
             return ANSC_STATUS_FAILURE;
         }
     }

    return returnStatus;
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
    char *strValue  = NULL;

    //Validate buffer
    if( ( NULL == pPSMEntry ) && ( NULL == pOutputString ) )
    {
        CcspTraceError(("%s %d Invalid buffer\n",__FUNCTION__,__LINE__));
        return retPsmGet;
    }

    retPsmGet = PSM_Get_Record_Value2( bus_handle, g_Subsystem, pPSMEntry, NULL, &strValue );
    if ( retPsmGet == CCSP_SUCCESS ) 
    {
        //Copy till end of the string
        snprintf( pOutputString, strlen( strValue ) + 1, "%s", strValue );

        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(strValue);
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

    retPsmGet = PSM_Set_Record_Value2( bus_handle, g_Subsystem, pPSMEntry, ccsp_string, pSetString );

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
/* DmlWanIfMarkingInit() */
ANSC_STATUS
DmlWanIfMarkingInit
    (
         ANSC_HANDLE                hContext
    )
{
    PDATAMODEL_WAN_IFACE      pMyObject    = (PDATAMODEL_WAN_IFACE)hContext;
    INT                            iLoopCount   = 0;

    //Validate received buffer
    if( NULL == pMyObject )
    {
        CcspTraceError(("%s %d - Invalid buffer\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    //Initialise Marking Params
    for( iLoopCount = 0; iLoopCount < pMyObject->ulTotalNoofWanInterfaces; iLoopCount++ )
    {
        PDML_WAN_IFACE      pWanIfTable        = &(pMyObject->pWanIface[iLoopCount]);
        PDATAMODEL_MARKING  pDataModelMarking  = &(pWanIfTable->stDataModelMarking);
        ULONG                    ulIfInstanceNumber = 0; 
        char                     acPSMQuery[128]    = { 0 },
                                 acPSMValue[64]     = { 0 };

        /* Initiation all params */
        AnscSListInitializeHeader( &pDataModelMarking->MarkingList );
        pDataModelMarking->ulNextInstanceNumber     = 1;

        //Interface instance number
        ulIfInstanceNumber = pWanIfTable->ulInstanceNumber;

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
               PCONTEXT_MARKING_LINK_OBJECT    pMarkingCxtLink   = NULL; 
               ULONG                                ulInstanceNumber  = 0;

               /* Insert into marking table */
               if( ( NULL != ( pMarkingCxtLink = Marking_AddEntry( pWanIfTable, &ulInstanceNumber ) ) )  &&  
                   ( 0 < ulInstanceNumber ) )
               {
                   PDML_MARKING    p_Marking = ( PDML_MARKING )pMarkingCxtLink->hContext;

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
    }

    return ANSC_STATUS_SUCCESS;
}

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

/* * DmlAddMarking() */
ANSC_STATUS
DmlCheckAndProceedMarkingOperations
    (
        ANSC_HANDLE         hContext,
        PDML_MARKING   pMarking,
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
        PDML_MARKING   pMarking
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
        PDML_MARKING   pMarking
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
        PDML_MARKING   pMarking
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
