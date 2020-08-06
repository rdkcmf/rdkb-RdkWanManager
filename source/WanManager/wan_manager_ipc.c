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

/* ---- Include Files ---------------------------------------- */
#include "wan_manager.h"
#include <sys/un.h>
#include <errno.h>
#include "wan_interface_internal.h"
#include "wan_manager_prctl.h"
#include "wan_manager_private.h"
#include "wan_manager_db.h"

#define IFADDRCONF_ADD 0
#define IFADDRCONF_REMOVE 1
#define LAN_BRIDGE "brlan0"

/*
 * Global structure to hold the wan interface data.
 */
WanData_t gWanData;
extern PBACKEND_MANAGER_OBJECT  g_pBEManager;

/* ---- Private Functions ------------------------------------ */
static void* IpcServerThread( void *arg );
static ANSC_STATUS IpcServerInit();

static void ProcessDhcp6cStateChanged(dhcpv6_data_t * data);
#ifdef FEATURE_IPOE_HEALTH_CHECK
static ANSC_STATUS ProcessIpoeHealthCheckFailedIpv4Msg(void);
static ANSC_STATUS ProcessIpoeHealthCheckFailedRenewIpv4Msg(void);
static ANSC_STATUS ProcessIpoeHealthCheckFailedIpv6Msg(void);
static ANSC_STATUS ProcessIpoeHealthCheckFailedRenewIpv6Msg(void);
static ANSC_STATUS ProcessIpoeHealthCheckDhcp6cRestart(void);
#endif /*FEATURE_IPOE_HEALTH_CHECK*/

/* ---- Private Variables ------------------------------------ */
static int   ipcListenFd;   /* Unix domain IPC listening socket fd */
pthread_mutex_t gmWanDataMutex = PTHREAD_MUTEX_INITIALIZER;
unsigned int resend_msg[2] = {0};

/*****************************************************************************************
 * @brief Retrieve wanconnectiondata from global gWanData structure and
 * store it into the incoming structure pointer.
 * @param wanConnectionData Pointer to WanConnectionData_t to hold wanConnectionData info.
 * @return  ANSC_STATUS_SUCCESS in case of success else error code returned.
 ******************************************************************************************/
static ANSC_STATUS GetWanConnectionDataFromGlobalWanData(WanConnectionData_t *wanConnectionData);

int WanManager_StartIpcServer()
{
    pthread_t ipcThreadId;
    int ret = 0;

    if(IpcServerInit() != ANSC_STATUS_SUCCESS)
    {
        CcspTraceInfo(("Failed to initialise IPC messaging"));
        return -1;
    }

    ret = pthread_create( &ipcThreadId, NULL, &IpcServerThread, NULL );

    if( 0 != ret )
    {
        CcspTraceInfo(("%s %d - Failed to start IPC Thread Error:%d\n", __FUNCTION__, __LINE__, ret));
    }
    else
    {
        CcspTraceInfo(("%s %d - IPC Thread Started Successfully\n", __FUNCTION__, __LINE__));
    }
    return ret ;
}

ANSC_STATUS WanManager_sendIpcMsgToClient_AndGetReplyWithTimeout(msg_payload_t * payload)
{
    return ANSC_STATUS_SUCCESS;
}


ANSC_STATUS WanManager_sendIpcMsgToClient(msg_payload_t * payload)
{
    return ANSC_STATUS_SUCCESS;
}


static void* IpcServerThread( void *arg )
{

    //detach thread from caller stack
    pthread_detach(pthread_self());

    // local variables
    BOOL bRunning = TRUE;

    int bytes = 0;
    int msg_size = sizeof(msg_payload_t); 
    msg_payload_t msg; 
    memset (&msg, 0, sizeof(msg_payload_t));

    while (bRunning)
    {
        bytes = nn_recv(ipcListenFd, (msg_payload_t *)&msg, msg_size, 0);
        if ((bytes == msg_size)) 
        {
            switch(msg.msg_type)
            {
                case DHCPC_STATE_CHANGED:
                    if (WanManager_Dhcpv4_ProcessStateChangedMsg(&msg.data.dhcpv4) != ANSC_STATUS_SUCCESS)
                    {
                        CcspTraceError(("[%s-%d] Failed to proccess DHCPv4 state change message \n", __FUNCTION__, __LINE__));
                    }
                    break;
                case DHCP6C_STATE_CHANGED:
                    ProcessDhcp6cStateChanged(&msg.data.dhcpv6);
                    break;
            }
        }
        else
        {
            CcspTraceError(("[%s-%d] message size unexpected\n", __FUNCTION__, __LINE__));
        }
    }

    pthread_exit(NULL);
}

static ANSC_STATUS IpcServerInit()
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
    uint32_t i;

    if ((ipcListenFd = nn_socket(AF_SP, NN_PULL)) < 0)
    {
        return ANSC_STATUS_FAILURE;
    }
    if ((i = nn_bind(ipcListenFd, WAN_MANAGER_ADDR)) < 0)
    {
        return ANSC_STATUS_FAILURE;
    }

    return ANSC_STATUS_SUCCESS;
}


#ifdef FEATURE_IPOE_HEALTH_CHECK
static ANSC_STATUS ProcessIpoeHealthCheckFailedIpv4Msg(void)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
    /* Kill DHCPv4 client */
    WanManager_StopDhcpv4Client(TRUE);

    // update local wan interface data
    WanManager_UpdateWANInterface(WAN_CONNECTION_DOWN, NULL);

    return ret;
}

static ANSC_STATUS ProcessIpoeHealthCheckFailedRenewIpv4Msg(void)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;

    /*send force renew request to DHCPC*/
    if (WanManager_IsApplicationRunning(DHCPV4_CLIENT_NAME) == TRUE)
    {
        int pid = util_getPidByName(DHCPV4_CLIENT_NAME);
        CcspTraceInfo(("sending SIGUSR1 to %s[pid=%d], this will let the %s to send renew packet out \n", DHCPV4_CLIENT_NAME, pid, DHCPV4_CLIENT_NAME));
        util_signalProcess(pid, SIGUSR1);
    }

    // update local wan interface data
    WanManager_UpdateWANInterface(WAN_CONNECTION_DOWN, NULL);

    return ret;
}
static ANSC_STATUS ProcessIpoeHealthCheckFailedIpv6Msg(void)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;

    /* Kill DHCPv6 client */
    WanManager_StopDhcpv6Client(TRUE);

    // update local wan interface data
    WanManager_UpdateWANInterface(WAN_CONNECTION_IPV6_DOWN, NULL);

    return ret;
}

static ANSC_STATUS ProcessIpoeHealthCheckFailedRenewIpv6Msg(void)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;

    if (WanManager_IsApplicationRunning(DHCPV6_CLIENT_NAME) == TRUE)
    {
        int pid = prctl_getPidByName(DHCPV6_CLIENT_NAME);
        CcspTraceInfo(("sending SIGUSR2 to dhcp6c, this will let the dhcp6c to send renew packet out \n"));
        util_signalProcess(pid, SIGUSR2);
    }

    // update local wan interface data
    WanManager_UpdateWANInterface(WAN_CONNECTION_IPV6_DOWN, NULL);

    return ret;
}

static ANSC_STATUS ProcessIpoeHealthCheckDhcp6cRestart(void)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;

    /* Kill DHCPv6 client */
    CcspTraceInfo(("Stoping DHCPv6 Client \n"));
    if ((ret = WanManager_StopDhcpv6Client(FALSE)) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("Could not stop Dhcpv6 Client!\n"));
        return ret;
    }
    CcspTraceInfo(("Starting DHCPv6 Client \n"));
    if ((ret = WanManager_StartDhcpv6Client(gWanData.ifName, FALSE)) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("Could not start Dhcpv6 Client!\n"));
    }

    return ret;
}
#endif /*FEATURE_IPOE_HEALTH_CHECK*/

static void ProcessDhcp6cStateChanged(dhcpv6_data_t * dhcp6cInfoNew)
{

    dhcpv6_data_t dhcp6cInfoPrvs;
    BOOL connected = FALSE;
    char set_value[BUFLEN_64];

    memset(set_value, 0, sizeof(set_value));

    CcspTraceInfo(("prefixAssigned=%dprefixCmd=%dsitePrefix=%spdIfAddress=%sprefixPltime=%dprefixVltime=%d\n"
                   "addrAssigned=%daddrCmd=%daddress=%sifname=%s\n"
                   "maptAssigned=%d mapeAssigned=%d\n"
                   "dnsAssigned=%dnameserver=%s,%saftrAssigned=%daftr=%sisExpired=%d \n",
                   dhcp6cInfoNew->prefixAssigned, dhcp6cInfoNew->prefixCmd, dhcp6cInfoNew->sitePrefix,
                   dhcp6cInfoNew->pdIfAddress, dhcp6cInfoNew->prefixPltime, dhcp6cInfoNew->prefixVltime,
                   dhcp6cInfoNew->addrAssigned, dhcp6cInfoNew->addrCmd, dhcp6cInfoNew->address, dhcp6cInfoNew->ifname,
                   dhcp6cInfoNew->maptAssigned, dhcp6cInfoNew->mapeAssigned,
                   dhcp6cInfoNew->dnsAssigned, dhcp6cInfoNew->nameserver, dhcp6cInfoNew->nameserver1, dhcp6cInfoNew->aftrAssigned, dhcp6cInfoNew->aftr, dhcp6cInfoNew->isExpired));

    // get previous data
    pthread_mutex_lock(&gmWanDataMutex);
    memcpy(&dhcp6cInfoPrvs, &gWanData.ipv6Data, sizeof(dhcpv6_data_t));
    pthread_mutex_unlock(&gmWanDataMutex);

    /*Checkleaseexpiry*/
    if (dhcp6cInfoNew->isExpired)
    {
        CcspTraceInfo(("DHCP6LeaseExpired\n"));
        // update current IPv6 data
        pthread_mutex_lock(&gmWanDataMutex);
        memcpy(&gWanData.ipv6Data, dhcp6cInfoNew, sizeof(dhcpv6_data_t));
        pthread_mutex_unlock(&gmWanDataMutex);
        WanManager_UpdateWANInterface(WAN_CONNECTION_IPV6_DOWN, gWanData.ifName);
        return;
    }

#ifdef FEATURE_MAPT
    Dhcp6cMAPTParametersMsgBody dhcp6cMAPTMsgBodyPrvs;
    size_t expectedLength = sizeof(dhcpv6_data_t) + sizeof(Dhcp6cMAPTParametersMsgBody);
    CcspTraceNotice(("FEATURE_MAPT: MAP-T Enable %d\n", dhcp6cInfoNew->maptAssigned));
    if (dhcp6cInfoNew->maptAssigned && (msg->dataLength == expectedLength))
    {
        Dhcp6cMAPTParametersMsgBody *dhcp6cMAPTMsgBody = (Dhcp6cMAPTParametersMsgBody *)(dhcp6cInfoNew + 1);
#ifdef FEATURE_MAPT_DEBUG
        LOG_PRINT_MAPT("Got an event in Wanmanager for MAPT - CONFIG");
#endif
        // get MAP-T previous data
        pthread_mutex_lock(&gmWanDataMutex);
        memcpy(&dhcp6cMAPTMsgBodyPrvs, &gWanData.dhcp6cMAPTparameters, sizeof(Dhcp6cMAPTParametersMsgBody));
        pthread_mutex_unlock(&gmWanDataMutex);

        if (memcmp(dhcp6cMAPTMsgBody, &dhcp6cMAPTMsgBodyPrvs, sizeof(Dhcp6cMAPTParametersMsgBody)) != 0)
        {
            WanManager_UpdateGlobalWanData(MAPT_CONFIG_CHANGED, TRUE);
        }

        pthread_mutex_lock(&gmWanDataMutex);
        // store MAP-T parameters locally
        memcpy(&gWanData.dhcp6cMAPTparameters, dhcp6cMAPTMsgBody, sizeof(Dhcp6cMAPTParametersMsgBody));
        pthread_mutex_unlock(&gmWanDataMutex);

        // update MAP-T flags
        WanManager_UpdateWANInterface(MAPT_START, gWanData.ifName);
    }
    else
    {
#ifdef FEATURE_MAPT_DEBUG
        LOG_PRINT_MAPT("Got an event in Wanmanager for MAPT - STOP");
#endif
        // reset MAP-T parameters
        pthread_mutex_lock(&gmWanDataMutex);
        memset(&gWanData.dhcp6cMAPTparameters, 0, sizeof(Dhcp6cMAPTParametersMsgBody));
        pthread_mutex_unlock(&gmWanDataMutex);

        WanManager_UpdateWANInterface(MAPT_STOP, gWanData.ifName);
    }
#endif // FEATURE_MAPT

    /* dhcp6c receives an IPv6 address for WAN interface */
    if (dhcp6cInfoNew->addrAssigned)
    {
        if (dhcp6cInfoNew->addrCmd == IFADDRCONF_ADD)
        {
            CcspTraceInfo(("assigned IPv6 address \n"));
            connected = TRUE;
            if (strcmp(dhcp6cInfoPrvs.address, dhcp6cInfoNew->address))
            {
                syscfg_set_string(SYSCFG_FIELD_IPV6_ADDRESS, dhcp6cInfoNew->address);
            }
        }
        else /* IFADDRCONF_REMOVE */
        {
            CcspTraceInfo(("remove IPv6 address \n"));
            syscfg_set_string(SYSCFG_FIELD_IPV6_ADDRESS, "");
        }
    }

    /* dhcp6c receives prefix delegation for LAN */
    if (dhcp6cInfoNew->prefixAssigned && !IS_EMPTY_STRING(dhcp6cInfoNew->sitePrefix))
    {
        if (dhcp6cInfoNew->prefixCmd == IFADDRCONF_ADD &&
            dhcp6cInfoNew->prefixPltime != 0 && dhcp6cInfoNew->prefixVltime != 0)
        {
            CcspTraceInfo(("assigned prefix=%s \n", dhcp6cInfoNew->sitePrefix));
            connected = TRUE;

            /* Update the WAN prefix validity time in the persistent storage */
            if (dhcp6cInfoPrvs.prefixVltime != dhcp6cInfoNew->prefixVltime)
            {
                snprintf(set_value, sizeof(set_value), "%d", dhcp6cInfoNew->prefixVltime);
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_PREFIXVLTIME, set_value, 0);
            }

            if (dhcp6cInfoPrvs.prefixPltime != dhcp6cInfoNew->prefixPltime)
            {
                snprintf(set_value, sizeof(set_value), "%d", dhcp6cInfoNew->prefixPltime);
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_PREFIXPLTIME, set_value, 0);
            }

            if (strcmp(dhcp6cInfoPrvs.sitePrefixOld, dhcp6cInfoNew->sitePrefixOld))
            {
                syscfg_set_string(SYSCFG_FIELD_PREVIOUS_IPV6_PREFIX, dhcp6cInfoNew->sitePrefixOld);
            }

            if (strcmp(dhcp6cInfoPrvs.sitePrefix, dhcp6cInfoNew->sitePrefix))
            {
                syscfg_set_string(SYSCFG_FIELD_IPV6_PREFIX, dhcp6cInfoNew->sitePrefix);
            }

            // create global IPv6 address (<prefix>::1)
            char prefix[BUFLEN_64] = {0};
            memset(prefix, 0, sizeof(prefix));

            int index = strcspn(dhcp6cInfoNew->sitePrefix, "/");
            if (index < strlen(dhcp6cInfoNew->sitePrefix) && index < sizeof(prefix))
            {
                strncpy(prefix, dhcp6cInfoNew->sitePrefix, index);                                            // only copy prefix without the prefix length
                snprintf(set_value, sizeof(set_value), "%s1", prefix);                                        // concatenate "1" onto the prefix, which is in the form "xxxx:xxxx:xxxx:xxxx::"
                snprintf(dhcp6cInfoNew->pdIfAddress, sizeof(dhcp6cInfoNew->pdIfAddress), "%s/64", set_value); // concatenate prefix address with length "/64"

                if (strcmp(dhcp6cInfoPrvs.pdIfAddress, dhcp6cInfoNew->pdIfAddress))
                {
                    syscfg_set_string(SYSCFG_FIELD_IPV6_PREFIX_ADDRESS, dhcp6cInfoNew->pdIfAddress);
                    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_TR_BRLAN0_DHCPV6_SERVER_ADDRESS, set_value, 0);
                }
            }
        }
        else /* IFADDRCONF_REMOVE: prefix remove */
        {
            /* Validate if the prefix to be removed is the same as the stored prefix */
            if (strcmp(dhcp6cInfoPrvs.sitePrefix, dhcp6cInfoNew->sitePrefix) == 0)
            {
                CcspTraceInfo(("remove prefix \n"));
                syscfg_set_string(SYSCFG_FIELD_IPV6_PREFIX, "");
                syscfg_set_string(SYSCFG_FIELD_PREVIOUS_IPV6_PREFIX, "");
                syscfg_set_string(SYSCFG_FIELD_IPV6_PREFIX_ADDRESS, "");
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_PREFIXPLTIME, "", 0);
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_PREFIXVLTIME, "", 0);
            }
        }
    }

    /* dhcp6c receives dns information */
    if (dhcp6cInfoNew->dnsAssigned)
    {
        if (!IS_EMPTY_STRING(dhcp6cInfoNew->nameserver))
        {
            CcspTraceInfo(("assigned nameserver=%s", dhcp6cInfoNew->nameserver));

            if (strcmp(dhcp6cInfoPrvs.nameserver, dhcp6cInfoNew->nameserver))
            {
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_DNS_PRIMARY, dhcp6cInfoNew->nameserver, 0);
            }
        }

        if (!IS_EMPTY_STRING(dhcp6cInfoNew->nameserver1))
        {
            CcspTraceInfo(("assigned nameserver=%s", dhcp6cInfoNew->nameserver1));

            if (strcmp(dhcp6cInfoPrvs.nameserver1, dhcp6cInfoNew->nameserver1))
            {
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_DNS_SECONDARY, dhcp6cInfoNew->nameserver1, 0);
            }
        }
    }

    /* dhcp6c receives domain name information */
    if (dhcp6cInfoNew->domainNameAssigned && !IS_EMPTY_STRING(dhcp6cInfoNew->domainName))
    {
        CcspTraceInfo(("assigned domain name=%s \n", dhcp6cInfoNew->domainName));

        if (strcmp(dhcp6cInfoPrvs.domainName, dhcp6cInfoNew->domainName))
        {
            sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_DOMAIN, dhcp6cInfoNew->domainName, 0);
        }
    }

    /* Even when dhcp6c is not used to get the WAN interface IP address,
     *  * use this message as a trigger to check the WAN interface IP.
     *   * Maybe we've been assigned an address by SLAAC.*/

    if (!dhcp6cInfoNew->addrAssigned)
    {
        char guAddr[IP_ADDR_LENGTH] = {0};
        char guAddrPrefix[IP_ADDR_LENGTH] = {0};
        uint32_t prefixLen = 0;
        ANSC_STATUS r2;

        r2 = WanManager_getGloballyUniqueIfAddr6(gWanData.ifName, guAddr, &prefixLen);
        if (ANSC_STATUS_SUCCESS == r2)
        {
            sprintf(guAddrPrefix, "%s/%d", guAddr, prefixLen);
            CcspTraceInfo(("Detected GloballyUnique Addr6 %s, mark connection up! \n", guAddrPrefix));
            connected = TRUE;

            if (strcmp(dhcp6cInfoPrvs.address, guAddrPrefix))
            {
                syscfg_set_string(SYSCFG_FIELD_IPV6_ADDRESS, guAddrPrefix);
            }
        }
    }

    /*
     * dhcp6c receives AFTR information
     * TODO: should we update aftr even WAN is not connected?
     */
    if (connected && dhcp6cInfoNew->aftrAssigned && !IS_EMPTY_STRING(dhcp6cInfoNew->aftr))
    {
        CcspTraceInfo(("assigned aftr=%s \n", dhcp6cInfoNew->aftr));
    }

    if (connected)
    {
        if (memcmp(dhcp6cInfoNew, &dhcp6cInfoPrvs, sizeof(dhcpv6_data_t)) != 0)
        {
            WanManager_UpdateGlobalWanData(IPV6_CONFIG_CHANGED, TRUE);
            CcspTraceInfo(("IPv6 configuration has been changed \n"));
        }
        else
        {
            /*TODO: Revisit this*/
            //call function for changing the prlft and vallft
            if ((WanManager_Ipv6AddrUtil(LAN_BRIDGE, SET_LFT, dhcp6cInfoNew->prefixPltime, dhcp6cInfoNew->prefixVltime) < 0))
            {
                CcspTraceError(("Life Time Setting Failed"));
            }
        }
        // update current IPv6 Data
        pthread_mutex_lock(&gmWanDataMutex);
        memcpy(&gWanData.ipv6Data, dhcp6cInfoNew, sizeof(dhcpv6_data_t));
        pthread_mutex_unlock(&gmWanDataMutex);

        WanManager_UpdateWANInterface(WAN_CONNECTION_IPV6_UP, gWanData.ifName);
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_RADVD_RESTART, NULL, 0);
    }
} /* End of ProcessDhcp6cStateChanged() */

void WanManager_UpdateWANInterface(wan_msg_type_t msgType, const char *ifName)
{
    CcspTraceInfo(("ifName: %s, link: %s, ipv4: %s, ipv6: %s\n", ((ifName != NULL) ? ifName : "NULL"),
                   ((msgType == WAN_LINK_UP) ? "UP" : (msgType == WAN_LINK_DOWN) ? "DOWN" : "N/A"),
                   ((msgType == WAN_CONNECTION_UP) ? "UP" : (msgType == WAN_CONNECTION_DOWN) ? "DOWN" : "N/A"),
                   ((msgType == WAN_CONNECTION_IPV6_UP) ? "UP" : (msgType == WAN_CONNECTION_IPV6_DOWN) ? "DOWN" : "N/A")
                ));

#ifdef FEATURE_MAPT
    CcspTraceInfo(("mapt: %s \n", 
                   ((msgType == MAPT_START) ? "UP" : (msgType == MAPT_STOP) ? "DOWN" : "N/A")));
#endif
    /**
     * Since we updating global wandata instance,
     * protect it.
     */
    pthread_mutex_lock(&gmWanDataMutex);
    switch (msgType)
    {
    case WAN_LINK_UP:
    {
        if (ifName != NULL)
        {
            strncpy(gWanData.ifName, ifName, sizeof(gWanData.ifName) - 1);
            strncpy(gWanData.wanConnectionData.ifName, ifName, sizeof(gWanData.wanConnectionData.ifName));
        }
        break;
    }
    case WAN_LINK_DOWN:
    {
        break;
    }
    case WAN_CONNECTION_UP:
    {
        gWanData.isIPv4Up = TRUE;
        strncpy(gWanData.wanConnectionData.ipv4Address, gWanData.ipv4Data.ip, sizeof(gWanData.wanConnectionData.ipv4Address));

        break;
    }
    case WAN_CONNECTION_DOWN:
    {
        gWanData.isIPv4Up = FALSE;
        gWanData.isIPv4ConfigChanged = FALSE;
        strncpy(gWanData.wanConnectionData.ipv4Address, "", sizeof(gWanData.wanConnectionData.ipv4Address));
        ipv4Info_init(&gWanData.ipv4Data,&gWanData.ifName); // reset the sysvent/syscfg fields
    }
    case WAN_CONNECTION_IPV6_UP:
    {
        gWanData.isIPv6Up = TRUE;
        strncpy(gWanData.wanConnectionData.ipv6Address, !IS_EMPTY_STRING(gWanData.ipv6Data.pdIfAddress) ? gWanData.ipv6Data.pdIfAddress : gWanData.ipv6Data.address, sizeof(gWanData.wanConnectionData.ipv6Address));
        break;
    }
    case WAN_CONNECTION_IPV6_DOWN:
    {
        gWanData.isIPv6Up = FALSE;
        gWanData.isIPv6ConfigChanged = FALSE;
        gWanData.isMAPTUp = FALSE;              // reset MAPT flag
        gWanData.isMAPTConfigChanged = FALSE; // reset MAPT flag
        strncpy(gWanData.wanConnectionData.ipv6Address, "", sizeof(gWanData.wanConnectionData.ipv6Address));
        ipv6Info_init(&gWanData.ipv6Data); // reset the sysvent/syscfg fields
        break;
    }
#ifdef FEATURE_MAPT
    case MAPT_START:
    {
        gWanData.isMAPTUp = TRUE;
        break;
    }
    case MAPT_STOP:
    {
        gWanData.isMAPTUp = FALSE;
        gWanData.isMAPTConfigChanged = FALSE;
        break;
    }
#endif
    default:
        /* do nothing */
        break;
    }
    pthread_mutex_unlock(&gmWanDataMutex);
    return;
}

static ANSC_STATUS GetWanConnectionDataFromGlobalWanData(WanConnectionData_t *wanConnectionData)
{

    if (wanConnectionData == NULL)
    {
        CcspTraceError(("Invalid memory \n"));
        return ANSC_STATUS_BAD_PARAMETER;
    }

    pthread_mutex_lock(&gmWanDataMutex);
    memcpy(wanConnectionData, &gWanData.wanConnectionData, sizeof(WanConnectionData_t));
    pthread_mutex_unlock(&gmWanDataMutex);

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS WanManager_GetCopyofGlobalWanData(const char *ifname, BOOL *wan_enable, WanData_t *wanData)
{
    /**
     * As of now ifname is not used as in the inital stage,
     * we are bring up wanmanager for the single wan mode. In
     * case of multiwan we have global array of structure to hold
     * the wan interface data, so in that case to copy which particular
     * instance of wanData we need to require can identify using ifname
     * parameter.
     */
    if (ifname == NULL || wanData == NULL)
    {
        CcspTraceError(("Invalid memory \n"));
        return ANSC_STATUS_BAD_PARAMETER;
    }

    PDATAMODEL_WAN_IFACE pMyObject = (PDATAMODEL_WAN_IFACE)g_pBEManager->hWanIface;
    if ( NULL == pMyObject )
    {
        CcspTraceError(("Invalid Memory \n"));
        return ANSC_STATUS_FAILURE;
    }

    PWAN_CONTROLLER_PRIVATE_SM_INFO pWanController = pMyObject->pWanController;

    if ( NULL == pWanController )
    {
        CcspTraceError(("Invalid Memory \n"));
        return ANSC_STATUS_FAILURE;
    }

    *wan_enable = pWanController->WanEnable;

    pthread_mutex_lock(&gmWanDataMutex);
    memcpy(wanData, &gWanData, sizeof(gWanData));
    pthread_mutex_unlock(&gmWanDataMutex);

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS WanManager_UpdateGlobalWanData(WAN_DATA_UPDATE_REQ_TYPE reqType, BOOL flag)
{

    /**
     * Since we updating global wandata instance,
     * protect it.
     */
    pthread_mutex_lock(&gmWanDataMutex);
    switch (reqType)
    {
    case IPV4_CONFIG_CHANGED:
    {
        gWanData.isIPv4ConfigChanged = flag;
        break;
    }

    case IPV6_CONFIG_CHANGED:
    {
        gWanData.isIPv6ConfigChanged = flag;
        break;
    }

    case MAPT_CONFIG_CHANGED:
    {
        gWanData.isMAPTConfigChanged = flag;
        break;
    }
    case IPV4_STATE_UP:
    {
        gWanData.isIPv4Up = flag;
        if(flag == FALSE)
        {
            gWanData.isIPv4ConfigChanged = FALSE;
            ipv4Info_init(&gWanData.ipv4Data,&gWanData.ifName);
        }
        break;
    }
    case IPV6_STATE_UP:
    {
        gWanData.isIPv6Up = flag;
        if (FALSE == flag)
        {
            gWanData.isIPv6ConfigChanged = flag;
            gWanData.isMAPTUp = flag;              // reset MAPT flag
            gWanData.isMAPTConfigChanged = flag; // reset MAPT flag
            ipv6Info_init(&gWanData.ipv6Data);
        }
        break;
    }
    case MAPT_STATE_UP:
    {
        gWanData.isMAPTUp = flag;
        break;
    }
    default:
        /* Do nothing. */
        break;
    }
    pthread_mutex_unlock(&gmWanDataMutex);

    return ANSC_STATUS_SUCCESS;
}
