/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
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

#include <unistd.h>
#include <pthread.h>
#include <ifaddrs.h>
#include "wanmgr_interface_sm.h"
#include "wanmgr_utils.h"
#include "platform_hal.h"
#include "wanmgr_sysevents.h"
#include "wanmgr_ipc.h"
#include "wanmgr_rdkbus_utils.h"
#include "wanmgr_data.h"
#include "wanmgr_interface_sm.h"
#include "wanmgr_platform_events.h"
#include "wanmgr_net_utils.h"
#include "wanmgr_dhcpv4_apis.h"
#include "wanmgr_dhcpv6_apis.h"

typedef enum
{
    WAN_STATE_EXIT = 0,
    WAN_STATE_CONFIGURING_WAN,
    WAN_STATE_VALIDATING_WAN,
    WAN_STATE_OBTAINING_IP_ADDRESSES,
    WAN_STATE_IPV4_LEASED,
    WAN_STATE_IPV6_LEASED,
    WAN_STATE_DUAL_STACK_ACTIVE,
    WAN_STATE_IPV4_OVER_IPV6_ACTIVE,
    WAN_STATE_REFRESHING_WAN,
    WAN_STATE_DECONFIGURING_WAN
} eWanState_t;

#define LOOP_TIMEOUT 50000 // timeout in milliseconds. This is the state machine loop interval

/*WAN Manager States*/
static eWanState_t wan_state_configuring_wan(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_state_validating_wan(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_state_obtaining_ip_addresses(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_state_ipv4_leased(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_state_ipv6_leased(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_state_dual_stack_active(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_state_ipv4_over_ipv6_active(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_state_refreshing_wan(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_state_deconfiguring_wan(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_state_exit(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);

/*WAN Manager Transitions*/
static eWanState_t wan_transition_start(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_transition_physical_interface_down(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_transition_wan_up(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_transition_wan_validated(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_transition_refreshing_wan(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_transition_wan_refreshed(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_transition_ipv4_up(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_transition_ipv4_down(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_transition_ipv6_up(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_transition_ipv6_down(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_transition_dual_stack_down(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_transition_ipv4_over_ipv6_up(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_transition_ipv4_over_ipv6_down(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);
static eWanState_t wan_transition_exit(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl);

/********************************************************************************
 * @brief Configure IPV4 configuration on the interface.
 * This API calls the HAL routine to configure ipv4.
 * @param ifname Wan interface name
 * @param wanData pointer to WanData_t holds the wan data
 * @return RETURN_OK upon success else returned error code.
 *********************************************************************************/
static int wan_setUpIPv4(DML_WAN_IFACE* pInterface);

/********************************************************************************
 * @brief Unconfig IPV4 configuration on the interface.
 * This API calls the HAL routine to unconfig ipv4.
 * @param ifname Wan interface name
 * @param wanData pointer to WanData_t holds the wan data
 * @return RETURN_OK upon success else returned error code.
 *********************************************************************************/
static int wan_tearDownIPv4(DML_WAN_IFACE* pInterface);

/*************************************************************************************
 * @brief Configure IPV6 configuration on the interface.
 * This API calls the HAL routine to config ipv6.
 * @param ifname Wan interface name
 * @param wanData pointer to WanData_t holds the wan data
 * @return  RETURN_OK upon success else returned error code.
 **************************************************************************************/
static int wan_setUpIPv6(DML_WAN_IFACE* pInterface);

/*************************************************************************************
 * @brief Unconfig IPV6 configuration on the interface.
 * This API calls the HAL routine to unconfig ipv6.
 * @param ifname Wan interface name
 * @param wanData pointer to WanData_t holds the wan data
 * @return RETURN_OK upon success else returned error code.
 **************************************************************************************/
static int wan_tearDownIPv6(DML_WAN_IFACE* pInterface);

/**************************************************************************************
 * @brief Update DNS configuration into /etc/resolv.conf
 * @param wanIfname wan interface name
 * @param addIPv4 boolean flag indicates whether IPv4 DNS data needs to be update
 * @param addIPv6 boolean flag indicates whether IPv6 DNS data needs to be update
 * @return RETURN_OK upon success else ERROR code returned
 **************************************************************************************/
static int wan_updateDNS(DML_WAN_IFACE* pInterface, BOOL addIPv4, BOOL addIPv6);

/**************************************************************************************
 * @brief Clear the DHCP client data stored.
 * It should be used to clear the old data before start a new DHCP client.
 * @param Interface data structure
 * @return ANSC_STATUS_SUCCESS upon success else ANSC_STATUS_FAILURE
 **************************************************************************************/
static ANSC_STATUS WanManager_ClearDHCPData(DML_WAN_IFACE* pInterface);

#ifdef FEATURE_MAPT
/*************************************************************************************
 * @brief Enable mapt configuration on the interface.
 * This API calls the HAL routine to Enable mapt.
 * @param ifname Wan interface name
 * @return RETURN_OK upon success else ERROR code returned
 **************************************************************************************/
static int wan_setUpMapt(const char *ifName);

/*************************************************************************************
 * @brief Disable mapt configuration on the interface.
 * This API calls the HAL routine to disable mapt.
 * @param ifname Wan interface name
 * @return RETURN_OK upon success else ERROR code returned
 **************************************************************************************/
static int wan_tearDownMapt(const char *ifName);
#endif //FEATURE_MAPT
/*************************************************************************************
 * @brief Check IPv6 address assigned to interface or not.
 * This API internally checks ipv6 prefix being set, received valid gateway and
 * lan ipv6 address ready to use.
 * @return RETURN_OK on success else RETURN_ERR
 *************************************************************************************/
static int checkIpv6AddressAssignedToBridge();

/*************************************************************************************
 * @brief Check IPv6 address is ready to use or not
 * @return RETURN_OK on success else RETURN_ERR
 *************************************************************************************/
static int checkIpv6LanAddressIsReadyToUse();

/*************************************************************************************
 * @brief Check CPE received a valid IPv6 gw address
 * @return RETURN_OK on success else RETURN_ERR
 ************************************************************************************/
static int validate_v6_gateway_address(void);

/************************************************************************************
 * @brief Set v6 prefixe required for lan configuration
 * @return RETURN_OK on success else RETURN_ERR
 ************************************************************************************/
static int setUpLanPrefixIPv6(DML_WAN_IFACE* pIfaceData);



#ifdef FEATURE_MAPT
static int wan_setUpMapt(const char *ifName)
{
    int ret = RETURN_OK;

    if (ifName == NULL)
    {
        CcspTraceError(("%s %d - Invalid memory \n", __FUNCTION__, __LINE__));
        return RETURN_ERR;
    }

    /* Disable flow cache. */
    if (WanManager_DoSystemActionWithStatus("wanmanager", "fcctl disable") != RETURN_OK)
    {
        CcspTraceError(("%s %d - wanmanager: Failed to disable packet accelaration \n ", __FUNCTION__, __LINE__));
        ret = RETURN_ERR;
    }

    if (WanManager_DoSystemActionWithStatus("wanmanager", "fcctl flush") != RETURN_OK)
    {
        CcspTraceError(("%s %d - wanmanager: Failed to flush the cache \n", __FUNCTION__, __LINE__));
        ret = RETURN_ERR;
    }

    /* Disable runner. */
    if (WanManager_DoSystemActionWithStatus("wanmanager", "runner disable") != RETURN_OK)
    {
        CcspTraceError(("%s %d - wanmanager: Failed to disable the runner \n", __FUNCTION__, __LINE__));
        ret = RETURN_ERR;
    }

    if (WanManager_DoSystemActionWithStatus("wanmanager", "insmod /lib/modules/`uname -r`/extra/ivi.ko") != RETURN_OK)
    {
        CcspTraceError(("%s %d -insmod: Failed to add ivi.ko \n", __FUNCTION__, __LINE__));
        ret = RETURN_ERR;
    }

    return ret;
}

static int wan_tearDownMapt(const char *ifName)
{
    int ret = RETURN_OK;
    FILE *file;
    char line[BUFLEN_64];

    if (ifName == NULL)
    {
        CcspTraceError(("%s %d - Invalid memory \n", __FUNCTION__, __LINE__));
        return RETURN_ERR;
    }

    file = popen("cat /proc/modules | grep ivi","r");
    if( file == NULL)
    {
        CcspTraceError(("[%s][%d]Failed to open  /proc/modules \n", __FUNCTION__, __LINE__));
        ret = RETURN_ERR;
    }
    else
    {
        if( fgets (line, BUFLEN_64, file) !=NULL ) {
            if( strstr(line, "ivi")) {
                if (WanManager_DoSystemActionWithStatus("wanmanager", "ivictl -q") != RETURN_OK)
                {
                    CcspTraceError(("%s %d ivictl: Failed to stop \n", __FUNCTION__, __LINE__));
                }
                else
                {
                    CcspTraceError(("%s %d ivictl stopped successfully\n", __FUNCTION__, __LINE__));
                }

                if (WanManager_DoSystemActionWithStatus("wanmanager", "rmmod -f /lib/modules/`uname -r`/extra/ivi.ko") != RETURN_OK)
                {
                    CcspTraceError(("%s %d rmmod: Failed to remove ivi.ko \n", __FUNCTION__, __LINE__));
                }
                else
                {
                    CcspTraceError(("%s %d ivi.ko removed\n", __FUNCTION__, __LINE__));
                }
            }
        }
        pclose(file);
    }

    /* Enable packet accelaration. */
    if (WanManager_DoSystemActionWithStatus("wanmanager", "fcctl enable") != RETURN_OK)
    {
        CcspTraceError(("%s %dwanmanager: Failed to enable packet accelaration \n ", __FUNCTION__, __LINE__));
        ret = RETURN_ERR;
    }

    /* Enable runner. */
    if (WanManager_DoSystemActionWithStatus("wanmanager", "runner enable") != RETURN_OK)
    {
        CcspTraceError(("%s %d wanmanager: Failed to enable the runner \n ", __FUNCTION__, __LINE__));
        ret = RETURN_ERR;
    }

    return ret;
}
#endif



/*********************************************************************************/
/**************************** ACTIONS ********************************************/
/*********************************************************************************/
void WanManager_UpdateInterfaceStatus(DML_WAN_IFACE* pIfaceData, wanmgr_iface_status_t iface_status)
{
    CcspTraceInfo(("ifName: %s, link: %s, ipv4: %s, ipv6: %s\n", ((pIfaceData != NULL) ? pIfaceData->Wan.Name : "NULL"),
                   ((iface_status == WANMGR_IFACE_LINK_UP) ? "UP" : (iface_status == WANMGR_IFACE_LINK_DOWN) ? "DOWN" : "N/A"),
                   ((iface_status == WANMGR_IFACE_CONNECTION_UP) ? "UP" : (iface_status == WANMGR_IFACE_CONNECTION_DOWN) ? "DOWN" : "N/A"),
                   ((iface_status == WANMGR_IFACE_CONNECTION_IPV6_UP) ? "UP" : (iface_status == WANMGR_IFACE_CONNECTION_IPV6_DOWN) ? "DOWN" : "N/A")
                ));

    if(pIfaceData == NULL)
    {
        return;
    }

#ifdef FEATURE_MAPT
    CcspTraceInfo(("mapt: %s \n",
                   ((iface_status == WANMGR_IFACE_MAPT_START) ? "UP" : (iface_status == WANMGR_IFACE_MAPT_STOP) ? "DOWN" : "N/A")));
#endif

    switch (iface_status)
    {
        case WANMGR_IFACE_LINK_UP:
        {
            break;
        }
        case WANMGR_IFACE_LINK_DOWN:
        {
            break;
        }
        case WANMGR_IFACE_CONNECTION_UP:
        {
            pIfaceData->IP.Ipv4Status = WAN_IFACE_IPV4_STATE_UP;
            break;
        }
        case WANMGR_IFACE_CONNECTION_DOWN:
        {
            pIfaceData->IP.Ipv4Status = WAN_IFACE_IPV4_STATE_DOWN;
            pIfaceData->IP.Ipv4Changed = FALSE;
            strncpy(pIfaceData->IP.Ipv4Data.ip, "", sizeof(pIfaceData->IP.Ipv4Data.ip));
            wanmgr_sysevents_ipv4Info_init(pIfaceData->Wan.Name); // reset the sysvent/syscfg fields
            break;
        }
        case WANMGR_IFACE_CONNECTION_IPV6_UP:
        {
            pIfaceData->IP.Ipv6Status = WAN_IFACE_IPV6_STATE_UP;
            break;
        }
        case WANMGR_IFACE_CONNECTION_IPV6_DOWN:
        {
            pIfaceData->IP.Ipv6Status = WAN_IFACE_IPV6_STATE_DOWN;
            pIfaceData->IP.Ipv6Changed = FALSE;
            pIfaceData->MAP.MaptStatus = WAN_IFACE_MAPT_STATE_DOWN;     // reset MAPT flag
            pIfaceData->MAP.MaptChanged = FALSE;                        // reset MAPT flag
            strncpy(pIfaceData->IP.Ipv6Data.address, "", sizeof(pIfaceData->IP.Ipv6Data.address));
            strncpy(pIfaceData->IP.Ipv6Data.pdIfAddress, "", sizeof(pIfaceData->IP.Ipv6Data.pdIfAddress));
	    strncpy(pIfaceData->IP.Ipv6Data.sitePrefix, "", sizeof(pIfaceData->IP.Ipv6Data.sitePrefix));
            strncpy(pIfaceData->IP.Ipv6Data.nameserver, "", sizeof(pIfaceData->IP.Ipv6Data.nameserver));
            strncpy(pIfaceData->IP.Ipv6Data.nameserver1, "", sizeof(pIfaceData->IP.Ipv6Data.nameserver1));
            wanmgr_sysevents_ipv6Info_init(); // reset the sysvent/syscfg fields
            break;
        }
#ifdef FEATURE_MAPT
        case WANMGR_IFACE_MAPT_START:
        {
            pIfaceData->MAP.MaptStatus = WAN_IFACE_MAPT_STATE_DUP;
            break;
        }
        case WANMGR_IFACE_MAPT_STOP:
        {
            pIfaceData->MAP.MaptStatus = WAN_IFACE_MAPT_STATE_DOWN;     // reset MAPT flag
            pIfaceData->MAP.MaptChanged = FALSE;                        // reset MAPT flag
            break;
        }
#endif
        default:
            /* do nothing */
            break;
    }

    return;
}

static ANSC_STATUS WanMgr_Send_InterfaceRefresh(DML_WAN_IFACE* pInterface)
{
    DML_WAN_IFACE*      pWanIface4Thread = NULL;
    pthread_t           refreshThreadId;
    INT                 iErrorCode = -1;

    if(pInterface == NULL)
    {
         return ANSC_STATUS_FAILURE;
    }

    if((pInterface->Wan.Refresh == TRUE) &&
       (pInterface->Wan.LinkStatus == WAN_IFACE_LINKSTATUS_UP))
    {
        //Allocate memory for interface struct
        pWanIface4Thread = (DML_WAN_IFACE*)malloc(sizeof(DML_WAN_IFACE));
        if( NULL == pWanIface4Thread )
        {
            CcspTraceError(("%s %d Failed to allocate memory\n", __FUNCTION__, __LINE__));
            return ANSC_STATUS_FAILURE;
        }

        //Copy WAN interface structure for thread
        memset( pWanIface4Thread, 0, sizeof(DML_WAN_IFACE));
        memcpy( pWanIface4Thread, pInterface, sizeof(DML_WAN_IFACE) );

        //WAN refresh thread
        iErrorCode = pthread_create( &refreshThreadId, NULL, &WanMgr_RdkBus_WanIfRefreshThread, (void*)pWanIface4Thread );
        if( 0 != iErrorCode )
        {
         CcspTraceInfo(("%s %d - Failed to start WAN refresh thread EC:%d\n", __FUNCTION__, __LINE__, iErrorCode ));
         return ANSC_STATUS_FAILURE;
        }
    }

    return ANSC_STATUS_SUCCESS;
}

static int wan_updateDNS(DML_WAN_IFACE* pInterface, BOOL addIPv4, BOOL addIPv6)
{
    int ret = RETURN_OK;
    DnsData_t dnsData;

    if (NULL == pInterface)
    {
        CcspTraceError(("%s %d - Invalid memory \n", __FUNCTION__, __LINE__));
        return RETURN_ERR;
    }

    memset(&dnsData, 0, sizeof(DnsData_t));

    if (addIPv4)
    {
        strncpy(dnsData.dns_ipv4_1, pInterface->IP.Ipv4Data.dnsServer, sizeof(dnsData.dns_ipv4_1));
        strncpy(dnsData.dns_ipv4_2, pInterface->IP.Ipv4Data.dnsServer1, sizeof(dnsData.dns_ipv4_2));
    }

    if (addIPv6)
    {
        strncpy(dnsData.dns_ipv6_1, pInterface->IP.Ipv6Data.nameserver, sizeof(dnsData.dns_ipv6_1));
        strncpy(dnsData.dns_ipv6_2, pInterface->IP.Ipv6Data.nameserver1, sizeof(dnsData.dns_ipv6_2));
    }

    if ((ret = WanManager_CreateResolvCfg(&dnsData)) != RETURN_OK)
    {
        CcspTraceError(("%s %d - Failed to set up DNS servers \n", __FUNCTION__, __LINE__));
    }
    else
    {
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_DHCP_SERVER_RESTART, NULL, 0);
    }

    return ret;
}

static int validate_v6_gateway_address(void)
{
    char command[BUFLEN_64] = {0};
    char line[BUFLEN_128] = {0};
    char defaultGateway[BUFLEN_64] = {0};
    int defaultGatewayLen = 64;
    FILE *fp;
    int ret = RETURN_OK;

    snprintf(command, sizeof(command), "ip -6 route show default | grep default | awk '{print $3}'");

    fp = popen(command, "r");

    if (fp)
    {
        if (fgets(line, sizeof(line), fp) != NULL)
        {
            char *token = strtok(line, "\n"); // get string up until newline character
            if (token)
            {
                strncpy(defaultGateway, token, defaultGatewayLen);
                CcspTraceInfo(("IPv6 Default Gateway Address  = %s \n", defaultGateway));
            }
            else
            {
                CcspTraceError(("Could not parse IPv6 Gateway Address \n"));
                ret = RETURN_ERR;
            }
        }
        else
        {
            ret = RETURN_ERR;
        }
        pclose(fp);
    }
    else
    {
        CcspTraceError(("Failed to get the default Gateway Address \n"));
        ret = RETURN_ERR;
    }

    return ret;
}

static int checkIpv6LanAddressIsReadyToUse()
{
    char buffer[BUFLEN_256] = {0};
    FILE *fp_dad   = NULL;
    FILE *fp_route = NULL;
    int address_flag   = 0;
    int dad_flag       = 0;
    int route_flag     = 0;
    struct ifaddrs *ifap = NULL;
    struct ifaddrs *ifa  = NULL;
    char addr[INET6_ADDRSTRLEN] = {0};
    int i;

    /* We need to check the interface has got an IPV6-prefix , beacuse P-and-M can send
    the same event when interface is down, so we ensure send the UP event only
    when interface has an IPV6-prefix.
    */
    if (!getifaddrs(&ifap)) {
        for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
            if(strncmp(ifa->ifa_name,ETH_BRIDGE_NAME, strlen(ETH_BRIDGE_NAME)))
                continue;
            if (ifa->ifa_addr->sa_family != AF_INET6)
                continue;
            getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6), addr,
                    sizeof(addr), NULL, 0, NI_NUMERICHOST);
            if((strncmp(addr + (strlen(addr) - 3), "::1", 3) == 0)){
                address_flag = 1;
                break;
            }
        }//for loop
        freeifaddrs(ifap);
    }//getifaddr close

    if(address_flag == 0) {
        return -1;
    }
    /* Check Duplicate Address Detection (DAD) status. The way it works is that
       after an address is added to an interface, the operating system uses the
       Neighbor Discovery Protocol to check if any other host on the network
       has the same address. The whole process will take around 3 to 4 seconds
       to complete. Also we need to check and ensure that the gateway has
       a valid default route entry.
    */
    for(i=0; i<15; i++) {
        buffer[0] = '\0';
        if(dad_flag == 0) {
            if ((fp_dad = popen("ip address show dev brlan0 tentative", "r"))) {
                if(fp_dad != NULL) {
                    fgets(buffer, BUFLEN_256, fp_dad);
                    if(strlen(buffer) == 0 ) {
                        dad_flag = 1;
                    }
                    pclose(fp_dad);
                }
            }
        }

        if(route_flag == 0) {
            buffer[0] = '\0';
            if ((fp_route = popen("ip -6 ro | grep default", "r"))) {
                if(fp_route != NULL) {
                    fgets(buffer, BUFLEN_256, fp_route);
                    if(strlen(buffer) > 0 ) {
                        route_flag = 1;
                    }
                    pclose(fp_route);
                }
            }
        }

        if(dad_flag == 0 || route_flag == 0) {
            sleep(1);
        }
        else {
            break;
       }
    }

    if(dad_flag == 0 || route_flag == 0) {
        return -1;
    }

    return 0;
}

static int checkIpv6AddressAssignedToBridge()
{
    char lanPrefix[BUFLEN_128] = {0};
    int ret = RETURN_ERR;

    sysevent_get(sysevent_fd, sysevent_token, SYSEVENT_GLOBAL_IPV6_PREFIX_SET, lanPrefix, sizeof(lanPrefix));

    if(strlen(lanPrefix) > 0)
    {
        if ((validate_v6_gateway_address() == RETURN_OK) && (checkIpv6LanAddressIsReadyToUse() == 0))
        {
            ret = RETURN_OK;
        }
    }

    return ret;
}

static int setUpLanPrefixIPv6(DML_WAN_IFACE* pIfaceData)
{
    if (pIfaceData == NULL)
    {
        CcspTraceError(("%s %d - Invalid memory \n", __FUNCTION__, __LINE__));
        return RETURN_ERR;
    }

    int index = strcspn(pIfaceData->IP.Ipv6Data.sitePrefix, "/");
    if (index < strlen(pIfaceData->IP.Ipv6Data.sitePrefix))
    {
        char lanPrefix[BUFLEN_48] = {0};
        strncpy(lanPrefix, pIfaceData->IP.Ipv6Data.sitePrefix, index);
        if ((sizeof(lanPrefix) - index) > 3)
        {
            char previousPrefix[BUFLEN_48] = {0};
            char previousPrefix_vldtime[BUFLEN_48] = {0};
            char previousPrefix_prdtime[BUFLEN_48] = {0};
            strncat(lanPrefix, "/64", 3);
            sysevent_get(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_PREFIX, previousPrefix, sizeof(previousPrefix));
            sysevent_get(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_PREFIXVLTIME, previousPrefix_vldtime, sizeof(previousPrefix_vldtime));
            sysevent_get(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_PREFIXPLTIME, previousPrefix_prdtime, sizeof(previousPrefix_prdtime));
            if (strncmp(previousPrefix, lanPrefix, BUFLEN_48) == 0)
            {
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_PREVIOUS_IPV6_PREFIX, "", 0);
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_PREVIOUS_IPV6_PREFIXVLTIME, "0", 0);
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_PREVIOUS_IPV6_PREFIXPLTIME, "0", 0);
            }
            else if (strncmp(previousPrefix, "", BUFLEN_48) != 0)
            {
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_PREVIOUS_IPV6_PREFIX, previousPrefix, 0);
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_PREVIOUS_IPV6_PREFIXVLTIME, previousPrefix_vldtime, 0);
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_PREVIOUS_IPV6_PREFIXPLTIME, previousPrefix_prdtime, 0);
            }
            sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_PREFIX, lanPrefix, 0);
            sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_TR_EROUTER_DHCPV6_CLIENT_PREFIX, lanPrefix, 0);
        }
    }

    return RETURN_OK;
}


static int wan_setUpIPv4(DML_WAN_IFACE* pInterface)
{
    int ret = RETURN_OK;
    char cmdStr[BUFLEN_128 + IP_ADDR_LENGTH] = {0};
    char bCastStr[IP_ADDR_LENGTH] = {0};
    char line[BUFLEN_64] = {0};
    char *cp = NULL;
    FILE *fp = NULL;

    if (pInterface == NULL)
    {
        CcspTraceError(("%s %d - Invalid memory \n", __FUNCTION__, __LINE__));
        return RETURN_ERR;
    }
    if (RETURN_OK == wan_updateDNS(pInterface, TRUE, (pInterface->IP.Ipv6Status == WAN_IFACE_IPV6_STATE_UP)))
    {
        CcspTraceInfo(("%s %d -  IPv4 DNS servers configures successfully \n", __FUNCTION__, __LINE__));
        /* DHCP restart triggered. */
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_DHCP_SERVER_RESTART, NULL, 0);
    }
    else
    {
        CcspTraceInfo(("%s %d - Failed to configure IPv4 DNS servers \n", __FUNCTION__, __LINE__));
    }

    /** Setup IPv4: such as
     * "ifconfig eth0 10.6.33.165 netmask 255.255.255.192 broadcast 10.6.33.191 up"
     */
    if (WanManager_GetBCastFromIpSubnetMask(pInterface->IP.Ipv4Data.ip, pInterface->IP.Ipv4Data.mask, bCastStr) != RETURN_OK)
    {
        CcspTraceError((" %s %d - bad address %s/%s \n",__FUNCTION__,__LINE__, pInterface->IP.Ipv4Data.ip, pInterface->IP.Ipv4Data.mask));
        return RETURN_ERR;
    }

    snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s %s netmask %s broadcast %s",
             pInterface->IP.Ipv4Data.ifname, pInterface->IP.Ipv4Data.ip, pInterface->IP.Ipv4Data.mask, bCastStr);
    CcspTraceInfo(("%s %d -  IP configuration = %s \n", __FUNCTION__, __LINE__, cmdStr));
    WanManager_DoSystemAction("setupIPv4:", cmdStr);

    snprintf(cmdStr, sizeof(cmdStr), "sendarp -s %s -d %s", ETH_BRIDGE_NAME, ETH_BRIDGE_NAME);
    WanManager_DoSystemAction("setupIPv4", cmdStr);

    /** Need to manually add route if the connection is PPP connection*/
    if (pInterface->PPP.Enable == TRUE)
    {
        if (WanManager_AddGatewayRoute(&pInterface->IP.Ipv4Data) != RETURN_OK)
        {
            CcspTraceError(("%s %d - Failed to set up system gateway", __FUNCTION__, __LINE__));
        }
    }

    /** Set default gatway. */
    if (WanManager_AddDefaultGatewayRoute(&pInterface->IP.Ipv4Data) != RETURN_OK)
    {
        CcspTraceError(("%s %d - Failed to set up default system gateway", __FUNCTION__, __LINE__));
    }

    /** Update required sysevents. */
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_CONNECTION_STATE, WAN_STATUS_UP, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_CURRENT_IPV4_LINK_STATE, WAN_STATUS_UP, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_CURRENT_WAN_IPADDR, pInterface->IP.Ipv4Data.ip, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_CURRENT_WAN_SUBNET, pInterface->IP.Ipv4Data.mask, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_CURRENT_WAN_STATE, WAN_STATUS_UP, 0);
    if ((fp = fopen("/proc/uptime", "rb")) == NULL)
    {
        return RETURN_ERR;
    }
    if (fgets(line, sizeof(line), fp) != NULL)
    {
        if ((cp = strchr(line, ',')) != NULL)
            *cp = '\0';
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_WAN_START_TIME, line, 0);
    }
    fclose(fp);

    if (strstr(pInterface->Phy.Path, "Ethernet"))
    {
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_ETHWAN_INITIALIZED, "1", 0);
    }
    if (pInterface->IP.Ipv6Status == WAN_IFACE_IPV6_STATE_DOWN)
    {
        int  uptime = 0;
        char buffer[64] = {0};

        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_WAN_STATUS, WAN_STATUS_STARTED, 0);
        CcspTraceInfo(("%s %d - wan-status event set to started \n", __FUNCTION__, __LINE__));

        //Get WAN uptime
        WanManager_GetDateAndUptime( buffer, &uptime );
        LOG_CONSOLE("%s Wan_init_complete:%d\n",buffer,uptime);

        system("print_uptime \"boot_to_wan_uptime\"");
    }

    /* Firewall restart. */
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIREWALL_RESTART, NULL, 0);
    return ret;
}


static int wan_tearDownIPv4(DML_WAN_IFACE* pInterface)
{
    int ret = RETURN_OK;
    char cmdStr[BUFLEN_64] = {0};

    if (pInterface == NULL)
    {
        CcspTraceError(("%s %d - Invalid memory \n", __FUNCTION__, __LINE__));
        return RETURN_ERR;
    }

    /** Reset IPv4 DNS configuration. */
    if (RETURN_OK == wan_updateDNS(pInterface, FALSE, (pInterface->IP.Ipv6Status == WAN_IFACE_IPV6_STATE_UP)))
    {
        CcspTraceInfo(("%s %d -  IPv4 DNS servers unconfig successfully \n", __FUNCTION__, __LINE__));
        /* DHCP restart trigger. */
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_DHCP_SERVER_RESTART, NULL, 0);
    }
    else
    {
        CcspTraceError(("%s %d - Failed to unconfig IPv4 DNS servers \n", __FUNCTION__, __LINE__));
        ret = RETURN_ERR;
    }

    /* Need to remove the network from the routing table by
    * doing "ifconfig L3IfName 0.0.0.0"
    * wanData->ipv4Data.ifname is Empty.
    */
    snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s 0.0.0.0", pInterface->Wan.Name);
    if (WanManager_DoSystemActionWithStatus("wan_tearDownIPv4: ifconfig L3IfName 0.0.0.0", (cmdStr)) != 0)
    {
        CcspTraceError(("%s %d - failed to run cmd: %s", __FUNCTION__, __LINE__, cmdStr));
        ret = RETURN_ERR;
    }

    /* ReSet the required sysevents. */
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_CONNECTION_STATE, WAN_STATUS_DOWN, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_CURRENT_IPV4_LINK_STATE, WAN_STATUS_DOWN, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_CURRENT_WAN_STATE, WAN_STATUS_DOWN, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_WAN_START_TIME, "0", 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_CURRENT_WAN_IPADDR, "0.0.0.0", 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_CURRENT_WAN_SUBNET, "255.255.255.0", 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIREWALL_RESTART, NULL, 0);
    if (strstr(pInterface->Phy.Path, "Ethernet"))
    {
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_ETHWAN_INITIALIZED, "0", 0);
    }

    if (pInterface->IP.Ipv6Status == WAN_IFACE_IPV6_STATE_DOWN)
    {
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_WAN_STATUS, WAN_STATUS_STOPPED, 0);
        CcspTraceInfo(("%s %d - wan-status event set to stopped \n", __FUNCTION__, __LINE__));
    }

    return ret;
}


static int wan_setUpIPv6(DML_WAN_IFACE* pInterface)
{
    int ret = RETURN_OK;

    if (pInterface == NULL)
    {
        CcspTraceError(("%s %d - Invalid memory \n", __FUNCTION__, __LINE__));
        return RETURN_ERR;
    }

    /** Reset IPv6 DNS configuration. */
    if (RETURN_OK == wan_updateDNS(pInterface, (pInterface->IP.Ipv4Status == WAN_IFACE_IPV4_STATE_UP), TRUE))
    {
        CcspTraceInfo(("%s %d -  IPv6 DNS servers configured successfully \n", __FUNCTION__, __LINE__));
        /* DHCP restart trigger. */
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_DHCP_SERVER_RESTART, NULL, 0);
    }
    else
    {
        CcspTraceError(("%s %d - Failed to configure IPv6 DNS servers \n", __FUNCTION__, __LINE__));
        ret = RETURN_ERR;
    }

    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV6_CONNECTION_STATE, WAN_STATUS_UP, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_RADVD_RESTART, NULL, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_DHCP_SERVER_RESTART, NULL, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIREWALL_RESTART, NULL, 0);

    if (pInterface->IP.Ipv4Status == WAN_IFACE_IPV4_STATE_DOWN)
    {
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_WAN_STATUS, WAN_STATUS_STARTED, 0);
        CcspTraceInfo(("%s %d - wan-status event set to started \n", __FUNCTION__, __LINE__));
    }

    return ret;
}

static int wan_tearDownIPv6(DML_WAN_IFACE* pInterface)
{
    int ret = RETURN_OK;

    if (pInterface == NULL)
    {
        CcspTraceError(("%s %d - Invalid memory \n", __FUNCTION__, __LINE__));
        return RETURN_ERR;
    }

    /** Reset IPv6 DNS configuration. */
    if (RETURN_OK == wan_updateDNS(pInterface, (pInterface->IP.Ipv4Status == WAN_IFACE_IPV4_STATE_UP), FALSE))
    {
        CcspTraceInfo(("%s %d -  IPv6 DNS servers unconfig successfully \n", __FUNCTION__, __LINE__));
        /* DHCP restart trigger. */
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_DHCP_SERVER_RESTART, NULL, 0);
    }
    else
    {
        CcspTraceError(("%s %d - Failed to unconfig IPv6 DNS servers \n", __FUNCTION__, __LINE__));
    }

    /** Unconfig IPv6. */
    if ( WanManager_Ipv6AddrUtil(ETH_BRIDGE_NAME,DEL_ADDR,0,0) < 0)
    {
        AnscTraceError(("%s %d -  Failed to remove inactive address \n", __FUNCTION__,__LINE__));
    }

    // Reset sysvevents.
    char previousPrefix[BUFLEN_48] = {0};
    char previousPrefix_vldtime[BUFLEN_48] = {0};
    char previousPrefix_prdtime[BUFLEN_48] = {0};
    /* set ipv6 down sysevent notification. */
    sysevent_get(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_PREFIX, previousPrefix, sizeof(previousPrefix));
    sysevent_get(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_PREFIXVLTIME, previousPrefix_vldtime, sizeof(previousPrefix_vldtime));
    sysevent_get(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_PREFIXPLTIME, previousPrefix_prdtime, sizeof(previousPrefix_prdtime));
    if (strncmp(previousPrefix, "", BUFLEN_48) != 0)
    {
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_PREVIOUS_IPV6_PREFIX, previousPrefix, 0);
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_PREVIOUS_IPV6_PREFIXVLTIME, previousPrefix_vldtime, 0);
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_PREVIOUS_IPV6_PREFIXPLTIME, previousPrefix_prdtime, 0);
    }
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_PREFIX, "", 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_TR_EROUTER_DHCPV6_CLIENT_PREFIX, "", 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV6_CONNECTION_STATE, WAN_STATUS_DOWN, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIREWALL_RESTART, NULL, 0);

    if (pInterface->IP.Ipv4Status == WAN_IFACE_IPV4_STATE_DOWN)
    {
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_WAN_STATUS, WAN_STATUS_STOPPED, 0);
        CcspTraceInfo(("%s %d - wan-status event set to stopped \n", __FUNCTION__, __LINE__));
    }

    return ret;
}

/* WanManager_ClearDHCPData */
/* This function must be used only with the mutex locked */
static ANSC_STATUS WanManager_ClearDHCPData(DML_WAN_IFACE* pInterface)
{
    if(pInterface == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

    memset(pInterface->IP.Path, 0, sizeof(pInterface->IP.Path));

    /* DHCPv4 client */
    pInterface->IP.Ipv4Status = WAN_IFACE_IPV4_STATE_DOWN;
    pInterface->IP.Ipv4Changed = FALSE;
    memset(&(pInterface->IP.Ipv4Data), 0, sizeof(WANMGR_IPV4_DATA));
    pInterface->IP.Dhcp4cPid = 0;
    if(pInterface->IP.pIpcIpv4Data != NULL)
    {
        free(pInterface->IP.pIpcIpv4Data);
        pInterface->IP.pIpcIpv4Data = NULL;
    }

    /* DHCPv6 client */
    pInterface->IP.Ipv6Status = WAN_IFACE_IPV6_STATE_DOWN;
    pInterface->IP.Ipv6Changed = FALSE;
    memset(&(pInterface->IP.Ipv6Data), 0, sizeof(WANMGR_IPV6_DATA));
    pInterface->IP.Dhcp6cPid = 0;
    if(pInterface->IP.pIpcIpv6Data != NULL)
    {
        free(pInterface->IP.pIpcIpv6Data);
        pInterface->IP.pIpcIpv6Data = NULL;
    }

    return ANSC_STATUS_SUCCESS;
}

/*********************************************************************************/
/************************** TRANSITIONS ******************************************/
/*********************************************************************************/
static eWanState_t wan_transition_start(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    pInterface->IP.Ipv4Status = WAN_IFACE_IPV4_STATE_DOWN;
    pInterface->IP.Ipv6Status = WAN_IFACE_IPV6_STATE_DOWN;
    pInterface->MAP.MaptStatus = WAN_IFACE_MAPT_STATE_DOWN;
    pInterface->DSLite.Status = WAN_IFACE_DSLITE_STATE_DOWN;

    pInterface->Wan.Status = WAN_IFACE_STATUS_INITIALISING;
    pInterface->Wan.LinkStatus = WAN_IFACE_LINKSTATUS_CONFIGURING;

    WanMgr_RdkBus_updateInterfaceUpstreamFlag(pInterface->Phy.Path, TRUE);

    if(pInterface->Wan.ActiveLink == TRUE)
    {
        const char if_dsl_name[] = "dsl";
        if (strncmp(pInterface->Name, if_dsl_name, strlen(if_dsl_name)) == 0)
        {
            WanMgr_UpdatePlatformStatus(WANMGR_LINK_UP);
        }
        else
        {
            WanMgr_UpdatePlatformStatus(WANMGR_CONNECTING);
        }
    }

    CcspTraceInfo(("%s %d - Interface '%s' - TRANSITION START\n", __FUNCTION__, __LINE__, pInterface->Name));

    /* TODO: Need to handle crash recovery */
    return WAN_STATE_CONFIGURING_WAN;
}

static eWanState_t wan_transition_physical_interface_down(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if(pInterface->PPP.Enable == FALSE)
    {
        /* Stops DHCPv4 client */
        WanManager_StopDhcpv4Client(TRUE); // release dhcp lease

        /* Stops DHCPv6 client */
        WanManager_StopDhcpv6Client(TRUE); // release dhcp lease

#ifdef FEATURE_IPOE_HEALTH_CHECK
        if (pWanIfaceCtrl->IhcPid > 0)
        {
            if (WanManager_StopIpoeHealthCheckService(pWanIfaceCtrl->IhcPid) == ANSC_STATUS_FAILURE)
            {
                CcspTraceError(("%s %d - Failed to kill IHC process interface %s \n", __FUNCTION__, __LINE__, pInterface->Wan.Name));
            }
            pWanIfaceCtrl->IhcPid = 0;
        }
#endif  // FEATURE_IPOE_HEALTH_CHECK
    }
    else
    {
        /* Stops DHCPv6 client */
        WanManager_StopDhcpv6Client(TRUE); // release dhcp lease

        /* Delete PPP session */
        WanManager_DeletePPPSession(pInterface);
    }

    WanMgr_RdkBus_updateInterfaceUpstreamFlag(pInterface->Phy.Path, FALSE);

    CcspTraceInfo(("%s %d - Interface '%s' - TRANSITION DECONFIGURING WAN\n", __FUNCTION__, __LINE__, pInterface->Name));

    return WAN_STATE_DECONFIGURING_WAN;
}

static eWanState_t wan_transition_wan_up(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;


    pInterface->Wan.Status = WAN_IFACE_STATUS_VALIDATING;


    /* TODO: Runs WAN Validation processes based on the Wan.Validation flags,
    e.g. if Wan.Validation.Discovery-Offer is set to TRUE, a threaded
    process will be started to run the DHCPv4 Discovery-Offer validation.
    The results of each validation process will be stored internally
    to the state machine (i.e. not expressed in the data model */
    if(pInterface->Wan.ActiveLink == TRUE)
    {
        WanMgr_UpdatePlatformStatus(WANMGR_CONNECTING);
    }

    CcspTraceInfo(("%s %d - Interface '%s' - TRANSITION VALIDATING WAN\n", __FUNCTION__, __LINE__, pInterface->Name));

    return WAN_STATE_VALIDATING_WAN;
}

static eWanState_t wan_transition_wan_validated(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;


    pInterface->Wan.Status = WAN_IFACE_STATUS_UP;

    /* Clear DHCP data */
    WanManager_ClearDHCPData(pInterface);

    if( pInterface->PPP.Enable == FALSE )
    {
#ifdef FEATURE_IPOE_HEALTH_CHECK
        if (pInterface->Wan.ActiveLink == TRUE)
        {
            UINT IhcPid = 0;
            IhcPid = WanManager_StartIpoeHealthCheckService(pInterface->Wan.Name);
            if (IhcPid > 0)
            {
                pWanIfaceCtrl->IhcPid = IhcPid;
                CcspTraceError(("%s %d - Starting IPoE Health Check pid - %u for interface %s \n", __FUNCTION__, __LINE__, pWanIfaceCtrl->IhcPid, pInterface->Wan.Name));
            }
            else
            {
                CcspTraceError(("%s %d - Failed to start IPoE Health Check for interface %s \n", __FUNCTION__, __LINE__, pInterface->Wan.Name));
            }
        }
#endif // FEATURE_IPOE_HEALTH_CHECK
        /* Start DHCPv4 client */
        CcspTraceInfo(("%s %d - Staring udhcpc on interface %s \n", __FUNCTION__, __LINE__, pInterface->Wan.Name));
        uint32_t pid = WanManager_StartDhcpv4Client(pInterface->Wan.Name, FALSE);

        /* Start DHCPv6 Client */
        CcspTraceInfo(("%s %d - Staring dibbler-client on interface %s \n", __FUNCTION__, __LINE__, pInterface->Wan.Name));
        if (RETURN_OK != WanManager_StartDhcpv6Client(pInterface->Wan.Name, FALSE))
        {
            CcspTraceError(("%s %d - Failed to start DHCPv6 client on %s \n", __FUNCTION__, __LINE__, pInterface->Wan.Name));
        }
        CcspTraceInfo(("%s %d - Started dibbler-client on interface %s \n", __FUNCTION__, __LINE__, pInterface->Wan.Name));
    }
    else
    {
        WanManager_CreatePPPSession(pInterface);
    }

    CcspTraceInfo(("%s %d - Interface '%s' - TRANSITION OBTAINING IP ADDRESSES\n", __FUNCTION__, __LINE__, pInterface->Name));

    return WAN_STATE_OBTAINING_IP_ADDRESSES;
}

static eWanState_t wan_transition_refreshing_wan(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if(pInterface->PPP.Enable == FALSE)
    {
        /* Stops DHCPv4 client */
        WanManager_StopDhcpv4Client(TRUE); // release dhcp lease

        /* Stops DHCPv6 client */
        WanManager_StopDhcpv6Client(TRUE); // release dhcp lease
    }
    else
    {
        /* Stops DHCPv6 client */
        WanManager_StopDhcpv6Client(TRUE); // release dhcp lease

        /* Delete PPP session */
        WanManager_DeletePPPSession(pInterface);
    }

    /* Sets Ethernet.Link.{i}.X_RDK_Refresh to TRUE in VLAN & Bridging Manager
    in order to refresh the WAN link */
    if(WanMgr_Send_InterfaceRefresh(pInterface) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d - Interface '%s' - Sending Refresh message failed\n", __FUNCTION__, __LINE__, pInterface->Name));
    }

    pInterface->Wan.LinkStatus = WAN_IFACE_LINKSTATUS_CONFIGURING;
    pInterface->Wan.Refresh = FALSE;

    CcspTraceInfo(("%s %d - Interface '%s' - TRANSITION REFRESHING WAN\n", __FUNCTION__, __LINE__, pInterface->Name));

    return WAN_STATE_REFRESHING_WAN;
}

static eWanState_t wan_transition_wan_refreshed(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    /* Clear DHCP data */
    WanManager_ClearDHCPData(pInterface);

    if( pInterface->PPP.Enable == FALSE )
    {
        /* Start dhcp clients */
        /* DHCPv4 client */
        CcspTraceInfo(("%s %d - Staring dhcpc on interface %s \n", __FUNCTION__, __LINE__, pInterface->Wan.Name));
        uint32_t pid = WanManager_StartDhcpv4Client(pInterface->Wan.Name, FALSE);
        CcspTraceInfo(("%s %d - Started dhcpc on interface %s, pid %d \n", __FUNCTION__, __LINE__, pInterface->Wan.Name, pid));

        /* DHCPv6 Client */
        if (RETURN_OK != WanManager_StartDhcpv6Client(pInterface->Wan.Name, FALSE))
        {
            CcspTraceError(("%s %d - Failed to start DHCPv6 client on %s \n", __FUNCTION__, __LINE__, pInterface->Wan.Name ));
        }
    }
    else
    {
        WanManager_CreatePPPSession(pInterface);
    }

    CcspTraceInfo(("%s %d - Interface '%s' - TRANSITION OBTAINING IP ADDRESSES\n", __FUNCTION__, __LINE__, pInterface->Name));

    return WAN_STATE_OBTAINING_IP_ADDRESSES;
}

static eWanState_t wan_transition_ipv4_up(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    ANSC_STATUS ret;
    char buf[BUFLEN_128] = {0};

    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if(pInterface->Wan.ActiveLink == TRUE )
    {
        /* Configure IPv4. */
        ret = wan_setUpIPv4(pInterface);
        if (ret != RETURN_OK)
        {
            CcspTraceError(("%s %d - Failed to configure IPv4 successfully \n", __FUNCTION__, __LINE__));
        }

#ifdef FEATURE_IPOE_HEALTH_CHECK
        if ((pInterface->PPP.Enable == FALSE) && (pWanIfaceCtrl->IhcPid > 0))
        {
            WanMgr_SendMsgToIHC(IPOE_MSG_WAN_CONNECTION_UP, pInterface->Wan.Name);
        }
#endif
    }

    /* Force reset ipv4 state global flag. */
    pInterface->IP.Ipv4Changed = FALSE;


    pInterface->IP.Ipv4Status = WAN_IFACE_IPV4_STATE_UP;

    sysevent_get(sysevent_fd, sysevent_token, SYSEVENT_WAN_SERVICE_STATUS, buf, sizeof(buf));
    if (strcmp(buf, WAN_STATUS_STARTED))
    {
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_WAN_SERVICE_STATUS, WAN_STATUS_STARTED, 0);
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIREWALL_RESTART, NULL, 0);
    }

    memset(buf, 0, BUFLEN_128);
    sysevent_get(sysevent_fd, sysevent_token, SYSEVENT_IPV6_CONNECTION_STATE, buf, sizeof(buf));

    if(pInterface->IP.Ipv6Status == WAN_IFACE_IPV6_STATE_UP && !strcmp(buf, WAN_STATUS_UP))
    {
        if(pInterface->Wan.ActiveLink == TRUE)
        {
            WanMgr_UpdatePlatformStatus(WANMGR_CONNECTED);
        }

        CcspTraceInfo(("%s %d - Interface '%s' - TRANSITION DUAL STACK ACTIVE\n", __FUNCTION__, __LINE__, pInterface->Name));
        return WAN_STATE_DUAL_STACK_ACTIVE;
    }

    if(pInterface->Wan.ActiveLink == TRUE)
    {
        WanMgr_UpdatePlatformStatus(WANMGR_LINK_V4UP_V6DOWN);
    }

    CcspTraceInfo(("%s %d - Interface '%s' - TRANSITION IPV4 LEASED\n", __FUNCTION__, __LINE__, pInterface->Name));

    return WAN_STATE_IPV4_LEASED;
}

static eWanState_t wan_transition_ipv4_down(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    char buf[BUFLEN_128] = {0};

    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if (wan_tearDownIPv4(pInterface) != RETURN_OK)
    {
        CcspTraceError(("%s %d - Failed to tear down IPv4 for %s \n", __FUNCTION__, __LINE__, pInterface->Wan.Name));
    }

    WanManager_UpdateInterfaceStatus(pInterface, WANMGR_IFACE_CONNECTION_DOWN);
#ifdef FEATURE_IPOE_HEALTH_CHECK
    if((pInterface->Wan.ActiveLink == TRUE) && (pInterface->PPP.Enable == FALSE) && (pWanIfaceCtrl->IhcPid > 0))
    {
        WanMgr_SendMsgToIHC(IPOE_MSG_WAN_CONNECTION_DOWN, pInterface->Wan.Name);
    }
#endif

    sysevent_get(sysevent_fd, sysevent_token, SYSEVENT_IPV6_CONNECTION_STATE, buf, sizeof(buf));

    if(pInterface->IP.Ipv6Status == WAN_IFACE_IPV6_STATE_UP && !strcmp(buf, WAN_STATUS_UP))
    {
        if(pInterface->Wan.ActiveLink == TRUE)
        {
            WanMgr_UpdatePlatformStatus(WANMGR_LINK_V6UP_V4DOWN);
        }

        CcspTraceInfo(("%s %d - Interface '%s' - TRANSITION IPV6 LEASED\n", __FUNCTION__, __LINE__, pInterface->Name));
        return WAN_STATE_IPV6_LEASED;
    }

    if(pInterface->Wan.ActiveLink == TRUE)
    {
        WanMgr_UpdatePlatformStatus(WANMGR_CONNECTING);
    }

    CcspTraceInfo(("%s %d - Interface '%s' - TRANSITION OBTAINING IP ADDRESSES\n", __FUNCTION__, __LINE__, pInterface->Name));
    return WAN_STATE_OBTAINING_IP_ADDRESSES;
}

static eWanState_t wan_transition_ipv6_up(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    ANSC_STATUS ret;
    char buf[BUFLEN_128] = {0};

    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if(pInterface->Wan.ActiveLink == TRUE )
    {
        /* Configure IPv6. */
        ret = wan_setUpIPv6(pInterface);
        if (ret != RETURN_OK)
        {
            CcspTraceError(("%s %d - Failed to configure IPv6 successfully \n", __FUNCTION__, __LINE__));
        }
#ifdef FEATURE_IPOE_HEALTH_CHECK
        if ((pInterface->PPP.Enable == FALSE) && (pWanIfaceCtrl->IhcPid > 0))
        {
            WanMgr_SendMsgToIHC(IPOE_MSG_WAN_CONNECTION_IPV6_UP, pInterface->Wan.Name);
        }
#endif
    }

    pInterface->IP.Ipv6Changed = FALSE;
    pInterface->IP.Ipv6Status = WAN_IFACE_IPV6_STATE_UP;

    sysevent_get(sysevent_fd, sysevent_token, SYSEVENT_WAN_SERVICE_STATUS, buf, sizeof(buf));
    if (strcmp(buf, WAN_STATUS_STARTED))
    {
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_WAN_SERVICE_STATUS, WAN_STATUS_STARTED, 0);
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIREWALL_RESTART, NULL, 0);
    }

    memset(buf, 0, BUFLEN_128);
    sysevent_get(sysevent_fd, sysevent_token, SYSEVENT_IPV4_CONNECTION_STATE, buf, sizeof(buf));

    if( pInterface->IP.Ipv4Status == WAN_IFACE_IPV4_STATE_UP && !strcmp(buf, WAN_STATUS_UP))
    {
        if(pInterface->Wan.ActiveLink == TRUE)
        {
            WanMgr_UpdatePlatformStatus(WANMGR_CONNECTED);
        }

        CcspTraceInfo(("%s %d - Interface '%s' - TRANSITION DUAL STACK ACTIVE\n", __FUNCTION__, __LINE__, pInterface->Name));
        return WAN_STATE_DUAL_STACK_ACTIVE;
    }

    if(pInterface->Wan.ActiveLink == TRUE)
    {
        WanMgr_UpdatePlatformStatus(WANMGR_LINK_V6UP_V4DOWN);
    }

    CcspTraceInfo(("%s %d - Interface '%s' - TRANSITION IPV6 LEASED\n", __FUNCTION__, __LINE__, pInterface->Name));
    return WAN_STATE_IPV6_LEASED;
}

static eWanState_t wan_transition_ipv6_down(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    char buf[BUFLEN_128] = {0};

    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if (wan_tearDownIPv6(pInterface) != RETURN_OK)
    {
        CcspTraceError(("%s %d - Failed to tear down IPv6 for %s \n", __FUNCTION__, __LINE__, pInterface->Wan.Name));
    }

    WanManager_UpdateInterfaceStatus(pInterface, WANMGR_IFACE_CONNECTION_IPV6_DOWN);

#ifdef FEATURE_IPOE_HEALTH_CHECK
    if ((pInterface->Wan.ActiveLink == TRUE) && (pInterface->PPP.Enable == FALSE) && (pWanIfaceCtrl->IhcPid > 0))
    {
        WanMgr_SendMsgToIHC(IPOE_MSG_WAN_CONNECTION_IPV6_DOWN, pInterface->Wan.Name);
    }
#endif

    sysevent_get(sysevent_fd, sysevent_token, SYSEVENT_IPV4_CONNECTION_STATE, buf, sizeof(buf));

    if(pInterface->IP.Ipv4Status == WAN_IFACE_IPV4_STATE_UP && !strcmp(buf, WAN_STATUS_UP))
    {
        if(pInterface->Wan.ActiveLink == TRUE)
        {
            WanMgr_UpdatePlatformStatus(WANMGR_LINK_V4UP_V6DOWN);
        }

        CcspTraceInfo(("%s %d - Interface '%s' - TRANSITION IPV4 LEASED\n", __FUNCTION__, __LINE__, pInterface->Name));
        return WAN_STATE_IPV4_LEASED;
    }

    if(pInterface->Wan.ActiveLink == TRUE)
    {
        WanMgr_UpdatePlatformStatus(WANMGR_CONNECTING);
    }

    CcspTraceInfo(("%s %d - Interface '%s' - TRANSITION OBTAINING IP ADDRESSES\n", __FUNCTION__, __LINE__, pInterface->Name));
    return WAN_STATE_OBTAINING_IP_ADDRESSES;

}

static eWanState_t wan_transition_dual_stack_down(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));
    wan_transition_ipv4_down(pWanIfaceCtrl);
    wan_transition_ipv6_down(pWanIfaceCtrl);

    CcspTraceInfo(("%s %d - TRANSITION OBTAINING IP ADDRESSES\n", __FUNCTION__, __LINE__));
    return WAN_STATE_OBTAINING_IP_ADDRESSES;
}

static eWanState_t wan_transition_ipv4_over_ipv6_up(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));
    /* TODO: Handle DSLite and MAP-T UP cases */

    CcspTraceInfo(("%s %d - TRANSITION IPV4 LEASED\n", __FUNCTION__, __LINE__));
    return WAN_STATE_IPV4_LEASED;
}

static eWanState_t wan_transition_ipv4_over_ipv6_down(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));
    /* TODO: Handle DSLite and MAP-T DOWN cases */

    CcspTraceInfo(("%s %d - TRANSITION IPV4 LEASED\n", __FUNCTION__, __LINE__));
    return WAN_STATE_IPV4_LEASED;
}

static eWanState_t wan_transition_exit(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    pInterface->Wan.Status = WAN_IFACE_STATUS_DISABLED;
    pInterface->Wan.Refresh = FALSE;
    pInterface->Wan.ActiveLink = FALSE;

    WanMgr_UpdatePlatformStatus(WANMGR_DISCONNECTED);

    CcspTraceInfo(("%s %d - Interface '%s' - EXITING STATE MACHINE\n", __FUNCTION__, __LINE__, pInterface->Name));
    return WAN_STATE_EXIT;
}


/*********************************************************************************/
/**************************** STATES *********************************************/
/*********************************************************************************/
static eWanState_t wan_state_configuring_wan(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if (pWanIfaceCtrl->WanEnable == FALSE ||
        pInterface->Wan.ActiveLink == FALSE ||
        pInterface->Phy.Status ==  WAN_IFACE_PHY_STATUS_DOWN ||
        pInterface->Wan.LinkStatus ==  WAN_IFACE_LINKSTATUS_DOWN )
    {
        return wan_transition_physical_interface_down(pWanIfaceCtrl);
    }

    if (pInterface->Wan.LinkStatus ==  WAN_IFACE_LINKSTATUS_UP )
    {
        return wan_transition_wan_up(pWanIfaceCtrl);
    }

    return WAN_STATE_CONFIGURING_WAN;
}

static eWanState_t wan_state_validating_wan(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if (pWanIfaceCtrl->WanEnable == FALSE ||
        pInterface->Wan.ActiveLink == FALSE ||
        pInterface->Phy.Status ==  WAN_IFACE_PHY_STATUS_DOWN ||
        pInterface->Wan.LinkStatus ==  WAN_IFACE_LINKSTATUS_DOWN )
    {
        return wan_transition_physical_interface_down(pWanIfaceCtrl);
    }

    if (pInterface->Wan.LinkStatus ==  WAN_IFACE_LINKSTATUS_CONFIGURING )
    {
        /* TODO: We'll need to call a transition that stops any running validation
        processes before returning to the CONFIGURING WAN state */
        return WAN_STATE_CONFIGURING_WAN;
    }

    /* TODO: Waits for every running validation process to complete, then checks the results */

    return wan_transition_wan_validated(pWanIfaceCtrl);
}

static eWanState_t wan_state_obtaining_ip_addresses(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if (pWanIfaceCtrl->WanEnable == FALSE ||
        pInterface->Wan.ActiveLink == FALSE ||
        pInterface->Phy.Status ==  WAN_IFACE_PHY_STATUS_DOWN ||
        pInterface->Wan.LinkStatus ==  WAN_IFACE_LINKSTATUS_DOWN )
    {
        return wan_transition_physical_interface_down(pWanIfaceCtrl);
    }

    if ( pInterface->Wan.LinkStatus ==  WAN_IFACE_LINKSTATUS_CONFIGURING ||
         pInterface->Wan.Refresh == TRUE)
    {
        return wan_state_refreshing_wan(pWanIfaceCtrl);
    }

    if (pInterface->IP.Ipv4Status == WAN_IFACE_IPV4_STATE_UP)
    {
        return wan_transition_ipv4_up(pWanIfaceCtrl);
    }
    else if (pInterface->IP.Ipv6Status == WAN_IFACE_IPV6_STATE_UP)
    {
        if(pInterface->IP.Ipv6Changed == TRUE)
        {
            /* Set sysevents to trigger P&M */
            if (setUpLanPrefixIPv6(pInterface) != RETURN_OK)
            {
                CcspTraceError((" %s %d - Failed to configure IPv6 prefix \n", __FUNCTION__, __LINE__));
            }
            /* Reset isIPv6ConfigChanged  */
            pInterface->IP.Ipv6Changed = FALSE;
            return WAN_STATE_OBTAINING_IP_ADDRESSES;
        }
        if (checkIpv6AddressAssignedToBridge() == RETURN_OK)
        {
            return wan_transition_ipv6_up(pWanIfaceCtrl);
        }
    }

    return WAN_STATE_OBTAINING_IP_ADDRESSES;
}

static eWanState_t wan_state_ipv4_leased(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if (pWanIfaceCtrl->WanEnable == FALSE ||
        pInterface->Wan.ActiveLink == FALSE ||
        pInterface->Phy.Status ==  WAN_IFACE_PHY_STATUS_DOWN ||
        pInterface->Wan.LinkStatus ==  WAN_IFACE_LINKSTATUS_DOWN ||
        pInterface->IP.Ipv4Status == WAN_IFACE_IPV4_STATE_DOWN ||
        pInterface->IP.Ipv4Changed == TRUE)
    {
        return wan_transition_ipv4_down(pWanIfaceCtrl);
    }
    else if (pInterface->IP.Ipv6Status == WAN_IFACE_IPV6_STATE_UP)
    {
        if(pInterface->IP.Ipv6Changed == TRUE)
        {
            /* Set sysevents to trigger P&M */
            if (setUpLanPrefixIPv6(pInterface) != RETURN_OK)
            {
                CcspTraceError((" %s %d - Failed to configure IPv6 prefix \n", __FUNCTION__, __LINE__));
            }
            /* Reset isIPv6ConfigChanged  */
            pInterface->IP.Ipv6Changed = FALSE;
            return WAN_STATE_IPV4_LEASED;
        }
        if (checkIpv6AddressAssignedToBridge() == RETURN_OK)
        {
            return wan_transition_ipv6_up(pWanIfaceCtrl);
        }
    }

    return WAN_STATE_IPV4_LEASED;
}

static eWanState_t wan_state_ipv6_leased(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if (pWanIfaceCtrl->WanEnable == FALSE ||
        pInterface->Wan.ActiveLink == FALSE ||
        pInterface->Phy.Status ==  WAN_IFACE_PHY_STATUS_DOWN ||
        pInterface->Wan.LinkStatus ==  WAN_IFACE_LINKSTATUS_DOWN ||
        pInterface->IP.Ipv6Status == WAN_IFACE_IPV6_STATE_DOWN ||
        pInterface->IP.Ipv6Changed == TRUE)
    {
        return wan_transition_ipv6_down(pWanIfaceCtrl);
    }
    else if (pInterface->IP.Ipv4Status == WAN_IFACE_IPV4_STATE_UP)
    {
        return wan_transition_ipv4_up(pWanIfaceCtrl);
    }
    else if (pInterface->Wan.EnableDSLite == TRUE &&
             pInterface->Wan.ActiveLink == TRUE &&
             pInterface->DSLite.Status == WAN_IFACE_DSLITE_STATE_UP)
    {
        return wan_transition_ipv4_over_ipv6_up(pWanIfaceCtrl);
    }
    else if (pInterface->Wan.EnableMAPT == TRUE &&
             pInterface->Wan.ActiveLink == TRUE &&
             pInterface->MAP.MaptStatus == WAN_IFACE_MAPT_STATE_UP)
    {
        return wan_transition_ipv4_over_ipv6_up(pWanIfaceCtrl);
    }

    return WAN_STATE_IPV6_LEASED;
}

static eWanState_t wan_state_dual_stack_active(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if (pWanIfaceCtrl->WanEnable == FALSE ||
        pInterface->Wan.ActiveLink == FALSE ||
        pInterface->Phy.Status ==  WAN_IFACE_PHY_STATUS_DOWN ||
        pInterface->Wan.LinkStatus ==  WAN_IFACE_LINKSTATUS_DOWN)
    {
        return wan_transition_dual_stack_down(pWanIfaceCtrl);
    }
    else if (pInterface->IP.Ipv4Status == WAN_IFACE_IPV4_STATE_DOWN )
    {
        /* TODO: Add IPoE Health Check failed for IPv4 here */
        return wan_transition_ipv4_down(pWanIfaceCtrl);
    }
    else if (pInterface->IP.Ipv6Status == WAN_IFACE_IPV6_STATE_DOWN )
    {
        /* TODO: Add IPoE Health Check failed for IPv6 here */
        return wan_transition_ipv6_down(pWanIfaceCtrl);
    }
    else if (pInterface->Wan.EnableDSLite == TRUE &&
             pInterface->Wan.ActiveLink == TRUE &&
             pInterface->DSLite.Status == WAN_IFACE_DSLITE_STATE_UP)
    {
        return wan_transition_ipv4_over_ipv6_up(pWanIfaceCtrl);
    }
    else if (pInterface->Wan.EnableMAPT == TRUE &&
             pInterface->Wan.ActiveLink == TRUE &&
             pInterface->MAP.MaptStatus == WAN_IFACE_MAPT_STATE_UP)
    {
        return wan_transition_ipv4_over_ipv6_up(pWanIfaceCtrl);
    }

    return WAN_STATE_DUAL_STACK_ACTIVE;
}
static eWanState_t wan_state_ipv4_over_ipv6_active(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if (pWanIfaceCtrl->WanEnable == FALSE ||
        pInterface->Wan.ActiveLink == FALSE ||
        pInterface->Phy.Status ==  WAN_IFACE_PHY_STATUS_DOWN ||
        pInterface->Wan.LinkStatus ==  WAN_IFACE_LINKSTATUS_DOWN ||
        pInterface->IP.Ipv6Status == WAN_IFACE_IPV6_STATE_DOWN ||
        pInterface->IP.Ipv6Changed == TRUE)
    {
        return wan_transition_ipv4_over_ipv6_down(pWanIfaceCtrl);
    }
    else if (pInterface->Wan.EnableDSLite == FALSE ||
             pInterface->DSLite.Status == WAN_IFACE_DSLITE_STATE_DOWN ||
             pInterface->DSLite.Changed == TRUE )
    {
        return wan_transition_ipv4_over_ipv6_down(pWanIfaceCtrl);
    }
    else if (pInterface->Wan.EnableMAPT == FALSE ||
             pInterface->MAP.MaptStatus == WAN_IFACE_MAPT_STATE_DOWN ||
             pInterface->MAP.MaptChanged == TRUE )
    {
        return wan_transition_ipv4_over_ipv6_down(pWanIfaceCtrl);
    }

    return WAN_STATE_IPV4_OVER_IPV6_ACTIVE;
}

static eWanState_t wan_state_refreshing_wan(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if (pWanIfaceCtrl->WanEnable == FALSE ||
        pInterface->Wan.ActiveLink == FALSE ||
        pInterface->Phy.Status ==  WAN_IFACE_PHY_STATUS_DOWN ||
        pInterface->Wan.LinkStatus ==  WAN_IFACE_LINKSTATUS_DOWN)
    {
        return wan_transition_physical_interface_down(pWanIfaceCtrl);
    }

    else if (pInterface->Wan.LinkStatus == WAN_IFACE_LINKSTATUS_UP &&
             pInterface->Wan.Refresh == TRUE)
    {
        return wan_transition_refreshing_wan(pWanIfaceCtrl);
    }
    else if (pInterface->Wan.LinkStatus == WAN_IFACE_LINKSTATUS_UP &&
             pInterface->Wan.Refresh == FALSE)
    {
        return wan_transition_wan_refreshed(pWanIfaceCtrl);
    }

    return WAN_STATE_REFRESHING_WAN;
}

static eWanState_t wan_state_deconfiguring_wan(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if (pInterface->Wan.LinkStatus == WAN_IFACE_LINKSTATUS_DOWN )
    {
        return wan_transition_exit(pWanIfaceCtrl);
    }

    return WAN_STATE_DECONFIGURING_WAN;
}

static eWanState_t wan_state_exit(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    //Clear WAN Name
    memset(pWanIfaceCtrl->pIfaceData->Wan.Name, 0, sizeof(pWanIfaceCtrl->pIfaceData->Wan.Name));

    /* Clear DHCP data */
    WanManager_ClearDHCPData(pWanIfaceCtrl->pIfaceData);


    return WAN_STATE_EXIT;
}

/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
ANSC_STATUS WanMgr_InterfaceSMThread_Init(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    int retStatus = ANSC_STATUS_SUCCESS;
    return retStatus;
}


ANSC_STATUS WanMgr_InterfaceSMThread_Finalise(void)
{
    int retStatus = ANSC_STATUS_SUCCESS;

    retStatus = WanMgr_CloseIpcServer();
    if(retStatus != ANSC_STATUS_SUCCESS)
    {
        CcspTraceInfo(("%s %d - IPC Thread failed to start!\n", __FUNCTION__, __LINE__ ));
    }

    return retStatus;
}


static ANSC_STATUS WanMgr_IfaceIpcMsg_handle(WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl)
{
    if((pWanIfaceCtrl == NULL) || (pWanIfaceCtrl->pIfaceData == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    DML_WAN_IFACE* pInterface = pWanIfaceCtrl->pIfaceData;

    if (pInterface->IP.pIpcIpv4Data != NULL )
    {
        wanmgr_handle_dchpv4_event_data(pInterface);
    }

    if (pInterface->IP.pIpcIpv6Data != NULL )
    {
        wanmgr_handle_dchpv6_event_data(pInterface);
    }

    return ANSC_STATUS_SUCCESS;
}


static void* WanMgr_InterfaceSMThread( void *arg )
{
    CcspTraceInfo(("%s %d \n", __FUNCTION__, __LINE__));

    //Validate buffer
    if ( NULL == arg )
    {
       CcspTraceError(("%s %d Invalid buffer\n", __FUNCTION__,__LINE__));
       //Cleanup current thread when exit
       pthread_exit(NULL);
    }

    //SM variables
    WanMgr_IfaceSM_Controller_t* pWanIfaceCtrl = ( WanMgr_IfaceSM_Controller_t *) arg;
    WanMgr_Iface_Data_t* pWanDmlIfaceData = NULL;
    eWanState_t iface_sm_state = WAN_STATE_EXIT;
    bool bRunning = true;

    // event handler
    int n = 0;
    struct timeval tv;


    //detach thread from caller stack
    pthread_detach(pthread_self());


    CcspTraceInfo(("%s %d - Interface state machine (TID %lu) initialising for iface idx %d\n", __FUNCTION__, __LINE__, pthread_self(), pWanIfaceCtrl->interfaceIdx));


    // initialise state machine
    if(WanMgr_InterfaceSMThread_Init(pWanIfaceCtrl) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d Policy Controller Error \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    //Transition Start
    pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pWanIfaceCtrl->interfaceIdx);
    if(pWanDmlIfaceData != NULL)
    {
        pWanIfaceCtrl->pIfaceData = &(pWanDmlIfaceData->data);
        iface_sm_state = wan_transition_start(pWanIfaceCtrl); // do this first before anything else to init variables
        WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
    }
    else
    {
        bRunning = false;
    }

    while (bRunning)
    {
        pWanIfaceCtrl->pIfaceData = NULL;

        /* Wait up to 500 milliseconds */
        tv.tv_sec = 0;
        tv.tv_usec = LOOP_TIMEOUT;

        n = select(0, NULL, NULL, NULL, &tv);
        if (n < 0)
        {
            /* interrupted by signal or something, continue */
            continue;
        }


        //Update Wan config
        WanMgr_Config_Data_t*   pWanConfigData = WanMgr_GetConfigData_locked();
        if(pWanConfigData != NULL)
        {
            pWanIfaceCtrl->WanEnable = pWanConfigData->data.Enable;

            WanMgrDml_GetConfigData_release(pWanConfigData);
        }

        //Get Interface data
        pWanDmlIfaceData = WanMgr_GetIfaceData_locked(pWanIfaceCtrl->interfaceIdx);
        if(pWanDmlIfaceData != NULL)
        {
            pWanIfaceCtrl->pIfaceData = &(pWanDmlIfaceData->data);
        }

        //Handle IPC messages
        WanMgr_IfaceIpcMsg_handle(pWanIfaceCtrl);

        // process state
        switch (iface_sm_state)
        {
            case WAN_STATE_CONFIGURING_WAN:
                {
                    iface_sm_state = wan_state_configuring_wan(pWanIfaceCtrl);
                    break;
                }
            case WAN_STATE_VALIDATING_WAN:
                {
                    iface_sm_state = wan_state_validating_wan(pWanIfaceCtrl);
                    break;
                }
            case WAN_STATE_OBTAINING_IP_ADDRESSES:
                {
                    iface_sm_state = wan_state_obtaining_ip_addresses(pWanIfaceCtrl);
                    break;
                }
            case WAN_STATE_IPV4_LEASED:
                {
                    iface_sm_state = wan_state_ipv4_leased(pWanIfaceCtrl);
                    break;
                }
            case WAN_STATE_IPV6_LEASED:
                {
                    iface_sm_state = wan_state_ipv6_leased(pWanIfaceCtrl);
                    break;
                }
            case WAN_STATE_DUAL_STACK_ACTIVE:
                {
                    iface_sm_state = wan_state_dual_stack_active(pWanIfaceCtrl);
                    break;
                }
            case WAN_STATE_IPV4_OVER_IPV6_ACTIVE:
                {
                    iface_sm_state = wan_state_ipv4_over_ipv6_active(pWanIfaceCtrl);
                    break;
                }
            case WAN_STATE_REFRESHING_WAN:
                {
                    iface_sm_state = wan_state_refreshing_wan(pWanIfaceCtrl);
                    break;
                }
            case WAN_STATE_DECONFIGURING_WAN:
                {
                    iface_sm_state = wan_state_deconfiguring_wan(pWanIfaceCtrl);
                    break;
                }
            case WAN_STATE_EXIT :
            default:
                {
                    iface_sm_state = wan_state_exit(pWanIfaceCtrl);
                    bRunning = false;
                    CcspTraceInfo(("%s %d - Exit from state machine\n", __FUNCTION__, __LINE__));
                    break;
                }
        }

        if(pWanDmlIfaceData != NULL)
        {
            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }

    }

    WanMgr_InterfaceSMThread_Finalise();


    CcspTraceInfo(("%s %d - Interface state machine (TID %lu) exiting for iface idx %d\n", __FUNCTION__, __LINE__, pthread_self(), pWanIfaceCtrl->interfaceIdx));
    
    //Free current private resource before exit
    if(NULL != pWanIfaceCtrl)
    {
        free(pWanIfaceCtrl);
        pWanIfaceCtrl = NULL;
    }

    pthread_exit(NULL);
}


int WanMgr_StartInterfaceStateMachine(WanMgr_IfaceSM_Controller_t *wanIf)
{
    WanMgr_IfaceSM_Controller_t *     wanIfLocal = NULL;
    pthread_t                wanSmThreadId;
    int                      iErrorCode     = 0;
    static int               siKeyCreated   = 0;

    //Allocate memory and pass it to thread
    wanIfLocal = ( WanMgr_IfaceSM_Controller_t * )malloc( sizeof( WanMgr_IfaceSM_Controller_t ) );
    if( NULL == wanIfLocal )
    {
        CcspTraceError(("%s %d Failed to allocate memory\n", __FUNCTION__, __LINE__));
        return -1;
    }

    //Copy buffer
    memcpy( wanIfLocal , wanIf, sizeof(WanMgr_IfaceSM_Controller_t) );

    CcspTraceInfo (("%s %d - WAN interface data received in the state machine (iface idx %d) \n", __FUNCTION__, __LINE__, wanIfLocal->interfaceIdx));

    //Wanmanager state machine thread
    iErrorCode = pthread_create( &wanSmThreadId, NULL, &WanMgr_InterfaceSMThread, (void*)wanIfLocal );

    if( 0 != iErrorCode )
    {
        CcspTraceInfo(("%s %d - Failed to start WanManager State Machine Thread EC:%d\n", __FUNCTION__, __LINE__, iErrorCode ));
    }
    else
    {
        CcspTraceInfo(("%s %d - WanManager State Machine Thread Started Successfully\n", __FUNCTION__, __LINE__ ));
    }
    return iErrorCode ;
}


void WanMgr_IfaceSM_Init(WanMgr_IfaceSM_Controller_t* pWanIfaceSMCtrl, INT iface_idx)
{
    if(pWanIfaceSMCtrl != NULL)
    {
       pWanIfaceSMCtrl->WanEnable = FALSE;
       pWanIfaceSMCtrl->interfaceIdx = iface_idx;
#ifdef FEATURE_IPOE_HEALTH_CHECK
       pWanIfaceSMCtrl->IhcPid = 0;
#endif
       pWanIfaceSMCtrl->pIfaceData = NULL;
    }
}
