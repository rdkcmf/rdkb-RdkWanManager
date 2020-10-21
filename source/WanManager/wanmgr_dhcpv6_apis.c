/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2019 RDK Management
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
#include "wanmgr_data.h"
#include "wanmgr_dhcpv6_apis.h"
#include "wanmgr_interface_sm.h"
#include "wanmgr_sysevents.h"
#include "wanmgr_ipc.h"
#include "wanmgr_utils.h"
#include "wanmgr_net_utils.h"


#include <sysevent/sysevent.h>
extern int sysevent_fd;
extern token_t sysevent_token;

#define IFADDRCONF_ADD 0
#define IFADDRCONF_REMOVE 1


static ANSC_STATUS wanmgr_dchpv6_get_ipc_msg_info(WANMGR_IPV6_DATA* pDhcpv6Data, ipc_dhcpv6_data_t* pIpcIpv6Data)
{
    if((pDhcpv6Data == NULL) || (pIpcIpv6Data == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }
    memcpy(pDhcpv6Data->ifname, pIpcIpv6Data->ifname, BUFLEN_32);
    memcpy(pDhcpv6Data->address, pIpcIpv6Data->address, BUFLEN_48);
    memcpy(pDhcpv6Data->pdIfAddress, pIpcIpv6Data->pdIfAddress, BUFLEN_48);
    memcpy(pDhcpv6Data->nameserver, pIpcIpv6Data->nameserver, BUFLEN_128);
    memcpy(pDhcpv6Data->nameserver1, pIpcIpv6Data->nameserver1, BUFLEN_128);
    memcpy(pDhcpv6Data->domainName, pIpcIpv6Data->domainName, BUFLEN_64);
    memcpy(pDhcpv6Data->sitePrefix, pIpcIpv6Data->sitePrefix, BUFLEN_48);
    pDhcpv6Data->prefixPltime = pIpcIpv6Data->prefixPltime;
    pDhcpv6Data->prefixVltime = pIpcIpv6Data->prefixVltime;
    memcpy(pDhcpv6Data->sitePrefixOld, pIpcIpv6Data->sitePrefixOld, BUFLEN_48);

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS wanmgr_handle_dchpv6_event_data(DML_WAN_IFACE* pIfaceData)
{
    if(NULL == pIfaceData)
    {
       return ANSC_STATUS_FAILURE;
    }

    ipc_dhcpv6_data_t* pNewIpcMsg = pIfaceData->IP.pIpcIpv6Data;
    WANMGR_IPV6_DATA* pDhcp6cInfoCur = &(pIfaceData->IP.Ipv6Data);
    if((NULL == pDhcp6cInfoCur) || (NULL == pNewIpcMsg))
    {
       return ANSC_STATUS_BAD_PARAMETER;
    }

    BOOL connected = FALSE;
    char set_value[BUFLEN_64];

    memset(set_value, 0, sizeof(set_value));

    CcspTraceInfo(("prefixAssigned=%dprefixCmd=%dsitePrefix=%spdIfAddress=%sprefixPltime=%dprefixVltime=%d\n"
                   "addrAssigned=%daddrCmd=%daddress=%sifname=%s\n"
                   "maptAssigned=%d mapeAssigned=%d\n"
                   "dnsAssigned=%dnameserver=%s,%saftrAssigned=%daftr=%sisExpired=%d \n",
                   pNewIpcMsg->prefixAssigned, pNewIpcMsg->prefixCmd, pNewIpcMsg->sitePrefix,
                   pNewIpcMsg->pdIfAddress, pNewIpcMsg->prefixPltime, pNewIpcMsg->prefixVltime,
                   pNewIpcMsg->addrAssigned, pNewIpcMsg->addrCmd, pNewIpcMsg->address, pNewIpcMsg->ifname,
                   pNewIpcMsg->maptAssigned, pNewIpcMsg->mapeAssigned,
                   pNewIpcMsg->dnsAssigned, pNewIpcMsg->nameserver, pNewIpcMsg->nameserver1, pNewIpcMsg->aftrAssigned, pNewIpcMsg->aftr, pNewIpcMsg->isExpired));



    /*Check lease expiry*/
    if (pNewIpcMsg->isExpired)
    {
        CcspTraceInfo(("DHCP6LeaseExpired\n"));
        // update current IPv6 data
        wanmgr_dchpv6_get_ipc_msg_info(&(pIfaceData->IP.Ipv6Data), pNewIpcMsg);
        WanManager_UpdateInterfaceStatus(pIfaceData, WANMGR_IFACE_CONNECTION_IPV6_DOWN);
        return ANSC_STATUS_SUCCESS;
    }


#ifdef FEATURE_MAPT
    Dhcp6cMAPTParametersMsgBody dhcp6cMAPTMsgBodyPrvs;
    size_t expectedLength = sizeof(ipc_dhcpv6_data_t) + sizeof(Dhcp6cMAPTParametersMsgBody);
    CcspTraceNotice(("FEATURE_MAPT: MAP-T Enable %d\n", pNewIpcMsg->maptAssigned));
    if (pNewIpcMsg->maptAssigned && (msg->dataLength == expectedLength))
    {
        Dhcp6cMAPTParametersMsgBody *dhcp6cMAPTMsgBody = (Dhcp6cMAPTParametersMsgBody *)(pNewIpcMsg + 1);
#ifdef FEATURE_MAPT_DEBUG
        LOG_PRINT_MAPT("Got an event in Wanmanager for MAPT - CONFIG");
#endif
        //get MAP-T previous data
        memcpy(&dhcp6cMAPTMsgBodyPrvs, &(pIfaceData->MAP.dhcp6cMAPTparameters), sizeof(Dhcp6cMAPTParametersMsgBody));

        if (memcmp(dhcp6cMAPTMsgBody, &dhcp6cMAPTMsgBodyPrvs, sizeof(Dhcp6cMAPTParametersMsgBody)) != 0)
        {
            //WanManager_UpdateGlobalWanData(MAPT_CONFIG_CHANGED, TRUE);
            pIfaceData->MAP.MaptChanged = TRUE;
        }

        // store MAP-T parameters locally
        memcpy(&(pIfaceData->MAP.dhcp6cMAPTparameters), dhcp6cMAPTMsgBody, sizeof(Dhcp6cMAPTParametersMsgBody));

        // update MAP-T flags
        WanManager_UpdateInterfaceStatus(pIfaceData, WANMGR_IFACE_MAPT_START);
    }
    else
    {
#ifdef FEATURE_MAPT_DEBUG
        LOG_PRINT_MAPT("Got an event in Wanmanager for MAPT - STOP");
#endif
        // reset MAP-T parameters
        memset(&(pIfaceData->MAP.dhcp6cMAPTparameters), 0, sizeof(Dhcp6cMAPTParametersMsgBody));
        WanManager_UpdateInterfaceStatus(pIfaceData, WANMGR_IFACE_MAPT_STOP);
    }
#endif // FEATURE_MAPT


    /* dhcp6c receives an IPv6 address for WAN interface */
    if (pNewIpcMsg->addrAssigned)
    {
        if (pNewIpcMsg->addrCmd == IFADDRCONF_ADD)
        {
            CcspTraceInfo(("assigned IPv6 address \n"));
            connected = TRUE;
            if (strcmp(pDhcp6cInfoCur->address, pNewIpcMsg->address))
            {
                syscfg_set_string(SYSCFG_FIELD_IPV6_ADDRESS, pNewIpcMsg->address);
            }
        }
        else /* IFADDRCONF_REMOVE */
        {
            CcspTraceInfo(("remove IPv6 address \n"));
            syscfg_set_string(SYSCFG_FIELD_IPV6_ADDRESS, "");
        }
    }

    /* dhcp6c receives prefix delegation for LAN */
    if (pNewIpcMsg->prefixAssigned && !IS_EMPTY_STRING(pNewIpcMsg->sitePrefix))
    {
        if (pNewIpcMsg->prefixCmd == IFADDRCONF_ADD &&
            pNewIpcMsg->prefixPltime != 0 && pNewIpcMsg->prefixVltime != 0)
        {
            CcspTraceInfo(("assigned prefix=%s \n", pNewIpcMsg->sitePrefix));
            connected = TRUE;

            /* Update the WAN prefix validity time in the persistent storage */
            if (pDhcp6cInfoCur->prefixVltime != pNewIpcMsg->prefixVltime)
            {
                snprintf(set_value, sizeof(set_value), "%d", pNewIpcMsg->prefixVltime);
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_PREFIXVLTIME, set_value, 0);
            }

            if (pDhcp6cInfoCur->prefixPltime != pNewIpcMsg->prefixPltime)
            {
                snprintf(set_value, sizeof(set_value), "%d", pNewIpcMsg->prefixPltime);
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_PREFIXPLTIME, set_value, 0);
            }

            if (strcmp(pDhcp6cInfoCur->sitePrefixOld, pNewIpcMsg->sitePrefixOld))
            {
                syscfg_set_string(SYSCFG_FIELD_PREVIOUS_IPV6_PREFIX, pNewIpcMsg->sitePrefixOld);
            }

            if (strcmp(pDhcp6cInfoCur->sitePrefix, pNewIpcMsg->sitePrefix))
            {
                syscfg_set_string(SYSCFG_FIELD_IPV6_PREFIX, pNewIpcMsg->sitePrefix);
            }

            // create global IPv6 address (<prefix>::1)
            char prefix[BUFLEN_64] = {0};
            memset(prefix, 0, sizeof(prefix));

            int index = strcspn(pNewIpcMsg->sitePrefix, "/");
            if (index < strlen(pNewIpcMsg->sitePrefix) && index < sizeof(prefix))
            {
                strncpy(prefix, pNewIpcMsg->sitePrefix, index);                                            // only copy prefix without the prefix length
                snprintf(set_value, sizeof(set_value), "%s1", prefix);                                        // concatenate "1" onto the prefix, which is in the form "xxxx:xxxx:xxxx:xxxx::"
                snprintf(pNewIpcMsg->pdIfAddress, sizeof(pNewIpcMsg->pdIfAddress), "%s/64", set_value); // concatenate prefix address with length "/64"

                if (strcmp(pDhcp6cInfoCur->pdIfAddress, pNewIpcMsg->pdIfAddress))
                {
                    syscfg_set_string(SYSCFG_FIELD_IPV6_PREFIX_ADDRESS, pNewIpcMsg->pdIfAddress);
                    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_TR_BRLAN0_DHCPV6_SERVER_ADDRESS, set_value, 0);
                }

                if (strcmp(pDhcp6cInfoCur->sitePrefix, pNewIpcMsg->sitePrefix) != 0)
                {
                    CcspTraceInfo(("%s %d new prefix = %s, current prefix = %s \n", __FUNCTION__, __LINE__, pNewIpcMsg->sitePrefix, pDhcp6cInfoCur->sitePrefix));
                    strncat(prefix, "/64", 3);
                    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_PREFIX, prefix, 0);
                }
            }
        }
        else /* IFADDRCONF_REMOVE: prefix remove */
        {
            /* Validate if the prefix to be removed is the same as the stored prefix */
            if (strcmp(pDhcp6cInfoCur->sitePrefix, pNewIpcMsg->sitePrefix) == 0)
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
    if (pNewIpcMsg->dnsAssigned)
    {
        if (!IS_EMPTY_STRING(pNewIpcMsg->nameserver))
        {
            CcspTraceInfo(("assigned nameserver=%s", pNewIpcMsg->nameserver));

            if (strcmp(pDhcp6cInfoCur->nameserver, pNewIpcMsg->nameserver))
            {
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_DNS_PRIMARY, pNewIpcMsg->nameserver, 0);
            }
        }

        if (!IS_EMPTY_STRING(pNewIpcMsg->nameserver1))
        {
            CcspTraceInfo(("assigned nameserver=%s", pNewIpcMsg->nameserver1));

            if (strcmp(pDhcp6cInfoCur->nameserver1, pNewIpcMsg->nameserver1))
            {
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_DNS_SECONDARY, pNewIpcMsg->nameserver1, 0);
            }
        }
    }

    /* dhcp6c receives domain name information */
    if (pNewIpcMsg->domainNameAssigned && !IS_EMPTY_STRING(pNewIpcMsg->domainName))
    {
        CcspTraceInfo(("assigned domain name=%s \n", pNewIpcMsg->domainName));

        if (strcmp(pDhcp6cInfoCur->domainName, pNewIpcMsg->domainName))
        {
            sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_IPV6_DOMAIN, pNewIpcMsg->domainName, 0);
        }
    }

    /* Even when dhcp6c is not used to get the WAN interface IP address,
     *  * use this message as a trigger to check the WAN interface IP.
     *   * Maybe we've been assigned an address by SLAAC.*/
    if (!pNewIpcMsg->addrAssigned)
    {
        char guAddr[IP_ADDR_LENGTH] = {0};
        char guAddrPrefix[IP_ADDR_LENGTH] = {0};
        uint32_t prefixLen = 0;
        ANSC_STATUS r2;

        r2 = WanManager_getGloballyUniqueIfAddr6(pIfaceData->Wan.Name, guAddr, &prefixLen);

        if (ANSC_STATUS_SUCCESS == r2)
        {
            sprintf(guAddrPrefix, "%s/%d", guAddr, prefixLen);
            CcspTraceInfo(("Detected GloballyUnique Addr6 %s, mark connection up! \n", guAddrPrefix));
            connected = TRUE;

            if (strcmp(pDhcp6cInfoCur->address, guAddrPrefix))
            {
                syscfg_set_string(SYSCFG_FIELD_IPV6_ADDRESS, guAddrPrefix);
            }
        }
    }

    /*
     * dhcp6c receives AFTR information
     * TODO: should we update aftr even WAN is not connected?
     */
    if (connected && pNewIpcMsg->aftrAssigned && !IS_EMPTY_STRING(pNewIpcMsg->aftr))
    {
        CcspTraceInfo(("assigned aftr=%s \n", pNewIpcMsg->aftr));
    }

    if (connected)
    {
        WANMGR_IPV6_DATA Ipv6DataTemp;

        wanmgr_dchpv6_get_ipc_msg_info(&(Ipv6DataTemp), pNewIpcMsg);

        if (memcmp(&(Ipv6DataTemp), pDhcp6cInfoCur, sizeof(WANMGR_IPV6_DATA)) != 0)
        {
            pIfaceData->IP.Ipv6Changed = TRUE;
            CcspTraceInfo(("IPv6 configuration has been changed \n"));
        }
        else
        {
            /*TODO: Revisit this*/
            //call function for changing the prlft and vallft
            if ((WanManager_Ipv6AddrUtil(LAN_BRIDGE_NAME, SET_LFT, pNewIpcMsg->prefixPltime, pNewIpcMsg->prefixVltime) < 0))
            {
                CcspTraceError(("Life Time Setting Failed"));
            }
            sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_RADVD_RESTART, NULL, 0);
        }
        // update current IPv6 Data
        memcpy(&(pIfaceData->IP.Ipv6Data), &(Ipv6DataTemp), sizeof(WANMGR_IPV6_DATA));
        WanManager_UpdateInterfaceStatus(pIfaceData, WANMGR_IFACE_CONNECTION_IPV6_UP);
    }

    return ANSC_STATUS_SUCCESS;
} /* End of ProcessDhcp6cStateChanged() */

void* IPV6CPStateChangeHandler (void *arg)
{
    const char *dhcpcInterface = (char *) arg;
    if(NULL == dhcpcInterface)
    {
        return ANSC_STATUS_FAILURE;
    }

    pthread_detach(pthread_self());
    WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceDataByName_locked(dhcpcInterface);
    if(pWanDmlIfaceData != NULL)
    {
        DML_WAN_IFACE* pIfaceData = &(pWanDmlIfaceData->data);
        switch (pIfaceData->PPP.IPV6CPStatus)
        {
            case WAN_IFACE_IPV6CP_STATUS_UP:
                WanManager_StartDhcpv6Client(dhcpcInterface , TRUE);
                break;
            case WAN_IFACE_IPV6CP_STATUS_DOWN:
                WanManager_StopDhcpv6Client(TRUE);
                break;
        }

        WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
    }

    free (arg);

    return ANSC_STATUS_SUCCESS;
}
