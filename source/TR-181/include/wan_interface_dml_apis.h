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

#ifndef  _WAN_INTERFACE_APIS_H
#define  _WAN_INTERFACE_APIS_H

#include "wan_mgr_apis.h"
#include "ccsp_psm_helper.h"

#define WAN_IF_MARKING_MAX_LIMIT       ( 15 )

typedef enum
_DML_WAN_IFACE_STATUS
{
    WAN_IFACE_STATUS_DISABLED = 1,
    WAN_IFACE_STATUS_INITIALISING,
    WAN_IFACE_STATUS_VALIDATING,
    WAN_IFACE_STATUS_UP  
} DML_WAN_IFACE_STATUS;

typedef enum
_DML_WAN_IFACE_LINKSTATUS
{
    WAN_IFACE_LINKSTATUS_DOWN = 1,
    WAN_IFACE_LINKSTATUS_CONFIGURING,
    WAN_IFACE_LINKSTATUS_UP
} DML_WAN_IFACE_LINKSTATUS;
typedef enum
_WAN_MANAGER_STATUS
{
    WAN_MANAGER_DOWN = 1,
    WAN_MANAGER_UP
} WAN_MANAGER_STATUS;

/** enum wan iface phy status */
typedef enum
_DML_WAN_IFACE_PHY_STATUS
{
    WAN_IFACE_PHY_STATUS_DOWN = 1,
    WAN_IFACE_PHY_STATUS_INITIALIZING,
    WAN_IFACE_PHY_STATUS_UP
} DML_WAN_IFACE_PHY_STATUS;

/** enum wan status */
typedef enum
_DML_WAN_IFACE_TYPE
{
    WAN_IFACE_TYPE_UNCONFIGURED = 1,
    WAN_IFACE_TYPE_PRIMARY,
    WAN_IFACE_TYPE_SECONDARY    
} DML_WAN_IFACE_TYPE;

/** enum wan status */
typedef enum
_DML_WAN_IFACE_IPV4_STATE
{
    WAN_IFACE_IPV4_STATE_UP = 1,
    WAN_IFACE_IPV4_STATE_DOWN
} DML_WAN_IFACE_IPV4_STATE;

/** enum wan status */
typedef enum
_DML_WAN_IFACE_IPV6_STATE
{
    WAN_IFACE_IPV6_STATE_UP = 1,
    WAN_IFACE_IPV6_STATE_DOWN
} DML_WAN_IFACE_IPV6_STATE;

/** enum wan status */
typedef enum
_DML_WAN_IFACE_MAPT_STATE
{
    WAN_IFACE_MAPT_STATE_UP = 1,
    WAN_IFACE_MAPT_STATE_DOWN
} DML_WAN_IFACE_MAPT_STATE;

/** enum dslite status */
typedef enum _DML_WAN_IFACE_DSLITE_STATE
{
    WAN_IFACE_DSLITE_STATE_UP = 1,
    WAN_IFACE_DSLITE_STATE_DOWN
} DML_WAN_IFACE_DSLITE_STATE;

/** enum wan status */
typedef enum
_WAN_NOTIFY_ENUM
{
    NOTIFY_TO_VLAN_AGENT        = 1
} WAN_NOTIFY_ENUM;

/** Enum IP (IPV4/IPV6/MAPT) state type. **/
typedef enum _DML_WAN_IFACE_IP_STATE_TYPE
{
    WAN_IFACE_IPV4_STATE = 0,
    WAN_IFACE_IPV6_STATE,
    WAN_IFACE_MAPT_STATE,
    WAN_IFACE_DSLITE_STATE
} DML_WAN_IFACE_IP_STATE_TYPE;

/** Enum IP state. UP/DOWN */
typedef enum _DML_WAN_IFACE_IP_STATE
{
    WAN_IFACE_IP_STATE_UP = 1,
    WAN_IFACE_IP_STATE_DOWN,
} DML_WAN_IFACE_IP_STATE;
/*
 *  Wan Marking object
 */
typedef enum
_DML_WAN_MARKING_DML_OPERATIONS
{
    WAN_MARKING_ADD = 1,
    WAN_MARKING_DELETE,
    WAN_MARKING_UPDATE
} DML_WAN_MARKING_DML_OPERATIONS;

#define  CONTEXT_MARKING_LINK_CLASS_CONTENT            \
        CONTEXT_LINK_CLASS_CONTENT                     \
        BOOL                            bFound;             \

typedef  struct
_CONTEXT_MARKING_LINK_OBJECT
{
    CONTEXT_MARKING_LINK_CLASS_CONTENT
}
CONTEXT_MARKING_LINK_OBJECT,  *PCONTEXT_MARKING_LINK_OBJECT;

typedef  struct
_DML_MARKING
{
    ULONG      InstanceNumber;
    ULONG      ulWANIfInstanceNumber;
    CHAR       Alias[64];
    UINT       SKBPort;
    UINT       SKBMark;
    INT        EthernetPriorityMark;
}
DML_MARKING,  *PDML_MARKING;

typedef  struct
_DATAMODEL_MARKING
{
    SLIST_HEADER      MarkingList;                       
    ULONG             ulNextInstanceNumber;
}
DATAMODEL_MARKING,  *PDATAMODEL_MARKING;

#define  ACCESS_CONTEXT_MARKING_LINK_OBJECT(p)              \
         ACCESS_CONTAINER(p, CONTEXT_MARKING_LINK_OBJECT, Linkage)

/* * WAN Interface */
typedef  struct
_DML_WAN_INTERFACE
{
    ULONG                        ulInstanceNumber;
    CHAR                         Name[64];
    CHAR                         DisplayName[64];
    CHAR                         PhyPath[64];
    DML_WAN_IFACE_PHY_STATUS     PhyStatus;
    BOOL                         CfgEnable;
    CHAR                         CfgName[64];
    BOOL                         CfgRefresh;
    BOOL                         CfgActiveLink;
    UINT                         CfgSelectionTimeout;
    INT                          CfgPriority;
    DML_WAN_IFACE_TYPE           CfgType;
    DML_WAN_IFACE_STATUS         CfgStatus;
    DML_WAN_IFACE_LINKSTATUS     CfgLinkStatus;
    BOOL                         DynTriggerEnable;
    ULONG                        DynTriggerDelay;
    DML_WAN_IFACE_IPV4_STATE     Ipv4State;
    DML_WAN_IFACE_IPV6_STATE     Ipv6State;
    DML_WAN_IFACE_MAPT_STATE     MaptState;
    DML_WAN_IFACE_DSLITE_STATE   DSLiteState;
    CHAR                         IpPath[64];
    BOOL                         CfgEnablePPP;
    BOOL                         CfgEnableMAPT;
    BOOL                         CfgEnableDSLite;
    BOOL                         CfgEnableIPoE;
    BOOL                         CfgValidationDiscoverOffer;
    BOOL                         CfgValidationSolicitAdvertise;
    BOOL                         CfgValidationRsRa;
    BOOL                         CfgValidationPadiPado;
    CHAR                         MaptPath[64];
    DML_WAN_IFACE_DSLITE_STATE   DsliteState;
    CHAR                         DslitePath[64];
    DATAMODEL_MARKING            stDataModelMarking;
}
DML_WAN_IFACE, *PDML_WAN_IFACE;

/**
 * Structure to hold global data used for wanmanager state machine.
 **/
typedef struct
_DML_WAN_INTERFACE_GLOBAL_CONFIG
{
    BOOL                         CfgEnable;
    DML_WAN_IFACE_TYPE           CfgType;
    INT                          CfgPriority;
    DML_WAN_IFACE_PHY_STATUS     CfgPhyStatus;
    CHAR                         CfgPhyPath[64];
    CHAR                         CfgBaseifName[64];
    CHAR                         CfgWanName[64];
    DML_WAN_IFACE_STATUS         CfgStatus;
    DML_WAN_IFACE_LINKSTATUS     CfgLinkStatus;
    WAN_MANAGER_STATUS           CfgWanManagerStatus;
    BOOL                         CfgRefresh;
    BOOL                         CfgActiveLink;
    BOOL                         CfgActiveLinkChanged;
    DML_WAN_IFACE_IPV4_STATE     CfgIpv4Status;
    DML_WAN_IFACE_IPV6_STATE     CfgIpv6Status;
    DML_WAN_IFACE_MAPT_STATE     CfgMAPTStatus;
    DML_WAN_IFACE_DSLITE_STATE   CfgDSLiteStatus;
    BOOL                         CfgEnablePPP;
    BOOL                         CfgEnableDSLite;
    BOOL                         CfgEnableIPoE;
    BOOL                         CfgEnableMAPT;
} DML_WAN_IFACE_GLOBAL_CONFIG, *PDML_WAN_IFACE_GLOBAL_CONFIG;

/*
    Standard function declaration 
*/
ANSC_STATUS
DmlWanIfInit
    (
        ANSC_HANDLE                 hDml,
        PANSC_HANDLE                phContext
    );

ANSC_STATUS
DmlWanIfConfInit
    (
        PANSC_HANDLE                phContext
    );

ANSC_STATUS
DmlGetTotalNoOfWanInterfaces
    (
        INT *wan_if_count
    );

ANSC_STATUS
DmlGetWanIfCfg
    (
        INT WanIfIndex,
        PDML_WAN_IFACE pWanIfInfo
    );

ANSC_STATUS
DmlSetWanIfCfg
    (
        INT WanIfIndex,
        PDML_WAN_IFACE pWanIfInfo
    );

ANSC_STATUS
DmlSetWanIfValidationCfg
    (
        INT WanIfIndex,
        PDML_WAN_IFACE pWanIfInfo
    );

ANSC_STATUS
DmlWanIfSetCfgStatus
    (
        INT IfIndex,
        DML_WAN_IFACE_STATUS status
    );

ANSC_STATUS
DmlWanIfGetCfgStatus
    (
        INT IfIndex,
        DML_WAN_IFACE_STATUS *status
    );

ANSC_STATUS
DmlWanIfSetCfgRefreshStatus
    (
        INT IfIndex,
        BOOL refresh
    );

ANSC_STATUS
DmlWanIfSetCfgLinkStatus
    (
        INT IfIndex,
        DML_WAN_IFACE_LINKSTATUS status
    );

ANSC_STATUS
DmlWanIfGetCfgRefreshStatus
    (
        INT IfIndex,
        BOOL *refresh
    );

ANSC_STATUS
DmlWanIfSetCfgName
    (
        INT IfIndex,
        char *name
    );

ANSC_STATUS
DmlWanIfGetCfgName
    (
        INT IfIndex,
        char *name
    );

ANSC_STATUS
DmlWanIfGetIndexFromIfName
    (
        char *ifname,
        INT *IfIndex
    );

ANSC_STATUS
DmlWanIfSetCfgActiveLink
    (
        INT IfIndex,
        BOOL status
    );

ANSC_STATUS
DmlWanIfSetCfgActiveLinkChanged
    (
        INT IfIndex
    );

ANSC_STATUS
DmlWanIfResetCfgActiveLinkChanged
    (
        char *ifname
    );

ANSC_STATUS
DmlWanIfGetCfgActiveLink
    (
        INT IfIndex,
        BOOL *status
    );

ANSC_STATUS
DmlWanIfSetCfgEnable
    (
        INT IfIndex,
        BOOL enable
    );

ANSC_STATUS
DmlWanIfGetCfgEnable
    (
        INT IfIndex,
        BOOL *enable
    );

ANSC_STATUS
DmlWanIfSetCfgType
    (
        INT IfIndex,
        DML_WAN_IFACE_TYPE type
    );

ANSC_STATUS
DmlWanIfGetCfgType
    (
        INT IfIndex,
        DML_WAN_IFACE_TYPE *type
    );

ANSC_STATUS
DmlWanIfSetCfgPriority
    (
        INT IfIndex,
        INT priority
    );

ANSC_STATUS
DmlWanIfGetCfgPriority
    (
        INT IfIndex,
        INT *priority
    );

ANSC_STATUS
DmlWanIfSetCfgPhyStatus
    (
        INT IfIndex,
        DML_WAN_IFACE_PHY_STATUS status
    );

ANSC_STATUS
DmlWanIfGetCfgPhyStatus
    (
        INT IfIndex,
        DML_WAN_IFACE_PHY_STATUS *status
    );

ANSC_STATUS
DmlWanIfSetCfgPhyPath
    (
        INT IfIndex,
        CHAR *path
    );

ANSC_STATUS
DmlWanIfGetCfgPhyPath
    (
        INT IfIndex,
        CHAR *path
    );

INT
DmlWanIfCheckDataModelInitialised();

ANSC_STATUS 
DmlWanIfSetCfgIpv4Status
    (
        char *ifname, 
        DML_WAN_IFACE_IPV4_STATE status
    );


ANSC_STATUS 
DmlWanIfSetCfgIpv6Status
    (
        char *ifname, 
        DML_WAN_IFACE_IPV6_STATE status
    );


ANSC_STATUS 
DmlWanIfSetCfgMAPTStatus
    (
        char *ifname, 
        DML_WAN_IFACE_MAPT_STATE status
    );


ANSC_STATUS 
DmlWanIfSetCfgDSLiteStatus
    (
        char *ifname, 
        DML_WAN_IFACE_DSLITE_STATE status
    );


ANSC_STATUS
DmlWanIfGetCopyOfGlobalData
    (
        PDML_WAN_IFACE_GLOBAL_CONFIG pGlobalInfo
    );

ANSC_STATUS
DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName
    (
        char *ifname,
        PDML_WAN_IFACE_GLOBAL_CONFIG pGlobalInfo
    );

ANSC_STATUS
DmlWanIfUpdateWanStatusForGivenIfName
    (
        char *ifname, 
        DML_WAN_IFACE_STATUS status
    );

ANSC_STATUS
DmlWanIfUpdateLinkStatusForGivenIfName
    (
        char *ifname, 
        DML_WAN_IFACE_LINKSTATUS status
    );

ANSC_STATUS
DmlWanIfUpdateRefreshFlagForGivenIfName
    (
        char *ifname, 
        BOOL  status
    );

ANSC_STATUS
DmlWanIfUpdateActiveLinkFlagForGivenIfName
    (
        char *ifname,
        BOOL  status
    );

ANSC_STATUS
WanManager_updateWanInterfaceUpstreamFlag
    (
        char *phyPath,
        BOOL flag
    );

ANSC_STATUS
DmlWanIfSetIPState
    (
        const char *wanIfName,
        DML_WAN_IFACE_IP_STATE_TYPE ipStateType,
        DML_WAN_IFACE_IP_STATE ipState
    );

#ifdef FEATURE_802_1P_COS_MARKING
/*
 *  WAN Marking
 */

ANSC_STATUS
DmlWanIfMarkingInit
    (
        ANSC_HANDLE                hContext
    );

ANSC_STATUS
DmlAddMarking
    (
        ANSC_HANDLE                 hContext,
        PDML_MARKING           pMarking
    );

ANSC_STATUS
DmlDeleteMarking
    (
        ANSC_HANDLE         hContext,
        PDML_MARKING   pMarking
    );

ANSC_STATUS
DmlSetMarking
    (
        ANSC_HANDLE                 hContext,
        PDML_MARKING           pMarking
    );

ANSC_STATUS
DmlCheckAndProceedMarkingOperations
    (   
        ANSC_HANDLE         hContext,
        PDML_MARKING   pMarking,
        DML_WAN_MARKING_DML_OPERATIONS enMarkingOp
    );

ANSC_STATUS
SListPushMarkingEntryByInsNum
    (
        PSLIST_HEADER               pListHead,
        PCONTEXT_LINK_OBJECT        pLinkContext
    );
#endif /* * FEATURE_802_1P_COS_MARKING */

ANSC_STATUS DmlWanIfSetWanRefresh( PDML_WAN_IFACE pstWanIface );

#endif /* _WAN_INTERFACE_APIS_H */
