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

#ifndef WAN_MANAGER_PRIVATE_H
#define WAN_MANAGER_PRIVATE_H

/* ---- Include Files ---------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "sysevent/sysevent.h"

#include "syscfg.h"
#include "wan_manager_ipc_msg.h"
#include "wan_manager_dhcpv4_apis.h"


/* ---- Global Constants -------------------------- */
#define DHCPV6_CLIENT_NAME "dibbler-client"

#ifdef FEATURE_IPOE_HEALTH_CHECK
#define BFD_CLIENT_NAME "skybfd"
#endif /* FEATURE_IPOE_HEALTH_CHECK */


#define MAKE_IP_ADDRESS(x) ((x&0xFF000000)>>24),((x&0x00FF0000)>>16), ((x&0x0000FF00)>>8),(x&0x000000FF)
#define IS_EMPTY_STRING(s)    ((s == NULL) || (*s == '\0'))

/* Defined used instead of MDMVS macros. */
#define WAN_CONN_UP "Up"
#define WAN_CONN_DOWN "Down"
#define WAN_CONNECTING "Connecting"
#define WAN_CONNECTED "Connected"
#define WAN_DISCONNECTED "Disconnected"

#ifndef CONFIG_VDSL
#define WAN_IF_NAME "pppoe0"
#define WAN_L2_IF_NAME "atm0/0.8.35"
#else //CONFIG_VDSL
#define WAN_L2_IF_NAME "ptm0/0.1.1"
#define WAN_IF_NAME "pppoe0"
/* VLAN ID Define  */
#define VDSL_VLAN_ID_DEFAULT    (-1)
#define VDSL_VLAN_ID_UK         101
#define VDSL_VLAN_ID_ROI        10
#define VDSL_VLAN_ID_ITALY      101
/* CoS bit for Priority  */
#define VDSL_PRI_DEFAULT (-1)
#define VDSL_PRI_UK     0
#define VDSL_PRI_ROI    0
#define VDSL_PRI_ITALY  0
#endif /* CONFIG_VDSL */
#ifdef FTTP_FEATURE_ENABLED
#define WANOE_DISCOVER_BRIDGENAME_FTTP "wanoe_br"
#endif // FTTP_FEATURE_ENABLED

#ifdef FEATURE_MAPT
#define IVICTL_COMMAND_ERROR 0xff
#endif
#define VDSL_LINE_TYPE_VLAN_PRIO 3 /*VLAN mux priority for VDSL*/
#define FTTP_LINE_TYPE_VLAN_PRIO 5 /*VLAN mux priority for FTTP*/

#define WANOE_DISCOVER_BRIDGENAME "brlan0"
#define WANOE_DISCOVER_INTERFACE "erouter0"

#define ETH_IFC_STR "eth"
#define DSL_IFC_STR "dsl"

#define SUCCESS "success"
#define FAILED "failed"

 /* sysevent/syscfg configurations used/managed by wanmanager.
 * The following defines are used for sysevent retrieval. */
#define SYSEVENT_IPV4_CONNECTION_STATE "ipv4_connection_state"
#define SYSEVENT_IPV6_CONNECTION_STATE "ipv6_connection_state"
#define SYSEVENT_IPV4_IP_ADDRESS "ipv4_%s_ipaddr"
#define SYSEVENT_IPV4_WAN_ADDRESS "ipv4_wan_ipaddr"
#define SYSEVENT_IPV4_SUBNET "ipv4_%s_subnet"
#define SYSEVENT_IPV4_WAN_SUBNET "ipv4_wan_subnet"
#define SYSEVENT_IPV4_DNS_NUMBER "ipv4_%s_dns_number"
#define SYSEVENT_IPV4_DNS_PRIMARY "ipv4_%s_dns_0"
#define SYSEVENT_IPV4_DNS_SECONDARY "ipv4_%s_dns_1"
#define SYSEVENT_IPV4_GW_ADDRESS "ipv4_%s_gw_0"
#define SYSEVENT_VENDOR_CLASS "vendor_class"
#define SYSEVENT_IPV4_DEFAULT_ROUTER "default_router"
#define SYSEVENT_IPV4_TIME_OFFSET "ipv4-timeoffset"
#define SYSEVENT_IPV4_TIME_ZONE "ipv4_timezone"
#define SYSEVENT_DHCPV4_TIME_OFFSET "dhcpv4_time_offset"
#define SYSEVENT_IPV4_US_CURRENT_RATE "ipv4_%s_us_current_rate_0"
#define SYSEVENT_IPV4_DS_CURRENT_RATE "ipv4_%s_ds_current_rate_0"

/*dhcp server restart*/
#define SYSEVENT_DHCP_SERVER_RESTART "dhcp_server-restart"

#define SYSCFG_FIELD_IPV6_ADDRESS         "wan_ipv6addr"
#define SYSCFG_FIELD_IPV6_PREFIX          "ipv6_prefix"
#define SYSCFG_FIELD_IPV6_PREFIX_ADDRESS  "ipv6_prefix_address"
#define SYSCFG_FIELD_IPV6_GW_ADDRESS      "wan_ipv6_default_gateway"
#define SYSCFG_FIELD_PREVIOUS_IPV6_PREFIX "previousipv6_prefix"
#define SYSEVENT_FIELD_IPV6_PREFIX        "ipv6_prefix"
#define SYSEVENT_FIELD_PREVIOUS_IPV6_PREFIX  "previous_ipv6_prefix"
#define SYSEVENT_FIELD_LAN_IPV6_START     "lan_ipv6-start"
#define SYSEVENT_FIELD_IPV6_DNS_PRIMARY   "ipv6_dns_0"
#define SYSEVENT_FIELD_IPV6_DNS_SECONDARY "ipv6_dns_1"
#define SYSEVENT_FIELD_IPV6_DOMAIN        "ipv6_domain"
#define SYSEVENT_FIELD_IPV6_PREFIXVLTIME  "ipv6_prefix_vldtime"
#define SYSEVENT_FIELD_IPV6_PREFIXPLTIME  "ipv6_prefix_prdtime"
#define SYSEVENT_FIELD_PREVIOUS_IPV6_PREFIXVLTIME "previous_ipv6_prefix_vldtime"
#define SYSEVENT_FIELD_PREVIOUS_IPV6_PREFIXPLTIME "previous_ipv6_prefix_prdtime"
#define SYSEVENT_RADVD_RESTART  "radvd_restart"

#define SYSEVENT_FIELD_TR_BRLAN0_DHCPV6_SERVER_ADDRESS        "tr_brlan0_dhcpv6_server_v6addr"
#define SYSEVENT_FIELD_TR_EROUTER_DHCPV6_CLIENT_PREFIX        "tr_erouter0_dhcpv6_client_v6pref"

/*WAN specific sysevent fieldnames*/
#define SYSEVENT_CURRENT_WAN_IFNAME "current_wan_ifname"
#define SYSEVENT_WAN_STATUS "wan-status"
#define SYSEVENT_CURRENT_WAN_IPADDR "current_wan_ipaddr"
#define SYSEVENT_CURRENT_WAN_SUBNET "current_wan_subnet"
#define SYSEVENT_WAN_SERVICE_STATUS "wan_service-status"

/*firewall restart*/
#define SYSEVENT_FIREWALL_RESTART "firewall-restart"
#define SYSEVENT_FIREWALL_STATUS "firewall-status"

#define WAN_MANAGER_STARTED "started"
#define WAN_STATUS_STARTED "started"
#define WAN_STATUS_STOPPED "stopped"
#define FIREWALL_STATUS_STARTED "started"
#define PTM_IFC_STR           "ptm"
#define PHY_WAN_IF_NAME "erouter0"
#define ETH_BRIDGE_NAME "brlan0"

#define UP "up"
#define DOWN "down"
#define SET "set"
#define UNSET "unset"
#define RESET "reset"

#define AF_SELECT_IPVX       (AF_SELECT_IPV4|AF_SELECT_IPV6)
#define IS_AF_ALLOW_IPV4 (ipvx)  ((ipvx) & AF_SELECT_IPV4)
#define IS_AF_ALLOW_IPV6 (ipvx)  ((ipvx) & AF_SELECT_IPV6)

typedef enum
{
    WAN_LINK_UP = 0,
    WAN_LINK_DOWN,
    WAN_CONNECTION_UP,
    WAN_CONNECTION_DOWN,
    WAN_CONNECTION_IPV6_UP,
    WAN_CONNECTION_IPV6_DOWN
}wan_msg_type_t;

typedef struct
{
   char ifName[IFNAME_LENGTH];
   char ipv4Address[IP_ADDR_LENGTH];
   char ipv6Address[BUFLEN_48];
} WanConnectionData_t;

typedef struct
{
   char dns_ipv4_1[BUFLEN_64];
   char dns_ipv4_2[BUFLEN_64];
   char dns_ipv6_1[BUFLEN_128];
   char dns_ipv6_2[BUFLEN_128];
} DnsData_t;

typedef struct WanData_struct {
    char baseIfName[32];   // net VLAN interface name (eg: eth3.101,ppp0.101)
    char ifName[32];   // net VLAN interface name (eg: eth3.101,ppp0.101)
    bool isIPv4Up;                          // IPv4 connection up or down
    bool isIPv4ConfigChanged;               // IPv4 configuration changed
    bool isIPv6Up;                          // IPv6 connection up or down
    bool isIPv6ConfigChanged;               // IPv6 configuration changed
    bool isMAPTUp;                          // MAPT configuration in use
    bool isMAPTConfigChanged;               // MAPT configuration changed
    bool isDSLiteUp;                        // DSLite configuration in use
    bool isDSLiteConfigChanged;             // DSLite configuration changed
    dhcpv4_data_t ipv4Data;
    dhcpv6_data_t ipv6Data;
    WanConnectionData_t wanConnectionData;
#ifdef FEATURE_MAPT
    Dhcp6cMAPTParametersMsgBody dhcp6cMAPTparameters;
#endif
} WanData_t;

/* Enum to define router region. */
typedef enum
{
    eRegionUnknown = 0,
    eRegionUK,
    eRegionROI,
    eRegionItaly
} eRouterRegion_t;

typedef enum
{
    DEL_ADDR = 0,
    SET_LFT = 1
} Ipv6OperType;

/**
 * Enum used to indicates which flag needs to
 * be updated the global wan configuration data.
 */
typedef enum
{
    IPV4_CONFIG_CHANGED = 0,
    IPV6_CONFIG_CHANGED,
    MAPT_CONFIG_CHANGED,
    IPV4_STATE_UP,
    IPV6_STATE_UP,
    MAPT_STATE_UP
} WAN_DATA_UPDATE_REQ_TYPE;


/* ---- Global Variables -------------------------- */
extern int sysevent_fd;
extern token_t sysevent_token;
extern eRouterRegion_t region;
/* Global structure to hold wan interface data.
 * This structure object defined in ipc module and this
 * can be also may required in other area. */
extern WanData_t gWanData;
extern pthread_mutex_t gmWanDataMutex;

/* ---- Global Prototypes -------------------------- */
/***************************************************************************
 * @brief API used to initialise sysevent demon
 * @return Return 0 upon success else returned -1.
 ***************************************************************************/
int WanManager_SyseventInit();

/***************************************************************************
 * @brief API used to close sysevent demon
 * @return None
 ***************************************************************************/
void WanManager_SyseventClose();

/***************************************************************************
 * @brief API used to send messages to clients
 * @param msg_payload_t Type of the ipc message
 * @return ANSC_STATUS_SUCCESS upon success else returned error code.
 ***************************************************************************/
ANSC_STATUS WanManager_sendIpcMsgToClient(msg_payload_t * payload);

ANSC_STATUS WanManager_sendIpcMsgToClient_AndGetReplyWithTimeout(msg_payload_t * payload);
/***************************************************************************
 * @brief API used to start Dhcpv6 client application.
 * @param pcInterfaceName Interface name on which the dhcpv6 needs to start
 * @param isPPP indicates PPP enabled or nor
 * @return ANSC_STATUS_SUCCESS upon success else returned error code.
 ***************************************************************************/
ANSC_STATUS WanManager_StartDhcpv6Client(const char *pcInterfaceName, BOOL isPPP);

/***************************************************************************
 * @brief API used to stop Dhcpv6 client application.
 * @param boolDisconnect This indicates whether this function called from
 * disconnect context or not
 * @return ANSC_STATUS_SUCCESS upon success else returned error code.
 ***************************************************************************/
ANSC_STATUS WanManager_StopDhcpv6Client(BOOL boolDisconnect);

/***************************************************************************
 * @brief API used to start Dhcpv4 client application.
 * @param intf Interface name on which the dhcpv4 needs to start
 * @param discover flag indicates discover on interface forced.
 * @return ANSC_STATUS_SUCCESS upon success else returned error code.
 ***************************************************************************/
uint32_t WanManager_StartDhcpv4Client(const char* intf, BOOL discover);

/***************************************************************************
 * @brief API used to stop Dhcpv4 client application.
 * @param sendReleaseAndExit flag indicates needs to send release packet before
 *                           exit.
 * @return ANSC_STATUS_SUCCESS upon success else returned error code.
 ***************************************************************************/
ANSC_STATUS WanManager_StopDhcpv4Client(BOOL sendReleaseAndExit);

/***************************************************************************
 * @brief API used to restart Dhcpv6 client application.
 * @param ifName_info interface name
 * @param dynamicIpEnabled indicates dynamicip needs to be enable
 * @param pdEnabled indicates packket delegation enabled
 * @return ANSC_STATUS_SUCCESS upon success else returned error code.
 ***************************************************************************/
uint32_t WanManager_RestartDhcp6c(const char *ifName_info, BOOL dynamicIpEnabled,
                            BOOL pdEnabled, BOOL aftrName);

/***************************************************************************
 * @brief API used to start IPoE health check application.
 * @return ANSC_STATUS_SUCCESS upon success else returned error code.
 ***************************************************************************/
ANSC_STATUS WanManager_StartIpoeHealthCheckService();

/***************************************************************************
 * @brief API used to atop IPoE health check application.
 * @return ANSC_STATUS_SUCCESS upon success else returned error code.
 ***************************************************************************/
ANSC_STATUS WanManager_StopIpoeHealthCheckService();

/***************************************************************************
 * @brief Utility function used to start application
 * @param appName string contains the application name
 * @param cmdLineArgs string contains arguments passed to application
 * @return Return PID of the started application.
 ***************************************************************************/
int WanManager_DoStartApp (const char *appName, const char *cmdLineArgs);

/***************************************************************************
 * @brief Utility function used to restart Application
 * @param appName string contains the application name
 * @param cmdLineArgs string contains arguments passed to application
 * @return Return PID of the started application.
 ***************************************************************************/
int WanManager_DoRestartApp(const char *appName, const char *cmdLineArgs);

/***************************************************************************
 * @brief Utility function used to stop Application
 * @param appName string contains the application name
 * @return None
 ***************************************************************************/
void WanManager_DoStopApp(const char *appName);

/***************************************************************************
 * @brief Utility function used to clean zombie Application.
 * @param appName string contains the application name
 * @return None
 ***************************************************************************/
void WanManager_DoCollectApp(const char *appName);

/***************************************************************************
 * @brief Utility function used to execute system command
 * @param from string indicates caller
 * @param cmd string indicates the commandline arguments
 * @return None
 ***************************************************************************/
void WanManager_DoSystemAction(const char* from, char *cmd);

/***************************************************************************
 * @brief Utility function used to execute system command
 * @param from string indicates caller
 * @param cmd string indicates the commandline arguments
 * @return status of system() call.
 ***************************************************************************/
int WanManager_DoSystemActionWithStatus(const char* from, char *cmd);

/***************************************************************************
 * @brief Utility function used to check Application is running.
 * @param appName string indicates application name
 * @return status of system() call.
 ***************************************************************************/
BOOL WanManager_IsApplicationRunning(const char* appName);

/***************************************************************************
 * @brief Utility function used to perform operation on IPV6 addresses
 * for a particular interface
 * @param ifname string indicates interface name
 * @param opr indicates operation type (Delete/Set)
 * @param preflft indicates preferred lifetime
 * @param vallft indicates valid lifetime
 * @return 0 upon success else -1 returned
 ***************************************************************************/
int WanManager_Ipv6AddrUtil(char *ifname,Ipv6OperType opr,int preflft,int vallft);

/***************************************************************************
 * @brief Utility function used to initaialize wanData object
 * @param wanData pointer to WanData_t structure
 * @param ifname indicates interface name
 * @return None
 ***************************************************************************/
void WanManagerInitInterfaceData(WanData_t *wanData, const char* ifname);

/***************************************************************************
 * @brief This API used to get the copy of global WanData_t object
 * to be used in the wanmanager state machine.
 * @param ifname string indicates the interface name
 * @param wanData Holds the copy of global wanData.
 * @return ANSC_STATUS_SUCCESS in case of success else error code returned.
 ***************************************************************************/
ANSC_STATUS WanManager_GetCopyofGlobalWanData(const char *ifname, BOOL *wan_enable, WanData_t *wanData);

/***********************************************************************************
 * @brief This API used to reset the config flag of global WanData object
 * based on the request.
 * @param reqType enum object indicates which configchanged flag needs to be update.
 * @param flag boolean value indicates whether set or reset.
 * @return RETURN_OK in case of success else error code returned.
 ************************************************************************************/
ANSC_STATUS WanManager_UpdateGlobalWanData(WAN_DATA_UPDATE_REQ_TYPE reqType, BOOL flag);

#ifdef FEATURE_MAPT
/***********************************************************************************
 * @brief This API used to process mapt configuration data.
 * @param dhcp6cMAPTMsgBody Hold the mapt data received from dhcp6 server.
 * @param baseIf Base interface name
 * @param vlanIf Vlan interface name
 * @return RETURN_OK in case of success else error code returned.
 ************************************************************************************/
int WanManager_ProcessMAPTConfiguration(Dhcp6cMAPTParametersMsgBody *dhcp6cMAPTMsgBody, const char *baseIf, const char *vlanIf);
#endif

/***********************************************************************************
 * @brief This API used to reset mapt configuration data.
 * @param baseIf Base interface name
 * @param vlanIf Vlan interface name
 * @return RETURN_OK in case of success else error code returned.
 ************************************************************************************/
int WanManager_ResetMAPTConfiguration(const char *baseIf, const char *vlanIf);

/***************************************************************************
 * @brief API used to update /etc/resolv.conf file with dns configuration
 * @param dnsInfo pointer to DnsData_t contains the dns info
 * @return RETURN_OK upon success else returned error code.
 ****************************************************************************/
int WanManager_CreateResolvCfg(const DnsData_t *dnsInfo);

/***************************************************************************
 * @brief API used to update default ipv4 gateway
 * @param ipv4Info pointer to dhcpv4_data_t holds the IPv4 configuration
 * @return RETURN_OK upon success else returned error code.
 ****************************************************************************/
int WanManager_AddDefaultGatewayRoute(const dhcpv4_data_t *ipv4Info);

/***************************************************************************
 * @brief API used to get broadcast IP from IP and subnet mask
 * @param inIpStr IP address
 * @param inSubnetMaskStr Subnet mask
 * @param outBcastStr Stores the broadcast address
 * @return RETURN_OK upon success else returned error code.
 ****************************************************************************/
int WanManager_GetBCastFromIpSubnetMask(const char *inIpStr, const char *inSubnetMaskStr, char *outBcastStr);

/*****************************************************************************************
 * @brief Update wanData global instance based on the message received from clients
 * @param msgType wan_msg_type_t message type
 * @param ifName interface name
 * @return  ANSC_STATUS_SUCCESS in case of success else error code returned.
 ******************************************************************************************/
void WanManager_UpdateWANInterface(wan_msg_type_t msgType, const char *ifName);

/***************************************************************************
 * @brief API used to get data and uptime from system
 * @param buffer to store date
 * @param uptime to store uptime value
 ****************************************************************************/
void WanManager_GetDateAndUptime(char *buffer, int *uptime);

/*****************************************************************************************
 * @brief  Utility API to get up time in seconds
 * @return return uptime in seconds
 ******************************************************************************************/
uint32_t WanManager_getUpTime();
#endif // SKY_WAN_MANAGER_H
