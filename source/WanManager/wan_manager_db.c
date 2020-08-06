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

#include "wan_manager_db.h"

int sysevent_fd = -1;
token_t sysevent_token;

int WanManager_SyseventInit()
{
    char sysevent_ip[] = "127.0.0.1";
    char sysevent_name[] = "wanmgr";
    sysevent_fd =  sysevent_open(sysevent_ip, SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, sysevent_name, &sysevent_token);
    if (sysevent_fd < 0)
        return -1;
    return 0;
}

void WanManager_SyseventClose()
{
    if (0 <= sysevent_fd)
    {
        sysevent_close(sysevent_fd, sysevent_token);
    }
}

ANSC_STATUS syscfg_set_string(const char* name, const char* value)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
    if (syscfg_set(NULL, name, value) != 0)
    {
        CcspTraceError(("syscfg_set failed: %s %s\n", name, value));
        ret = ANSC_STATUS_FAILURE;
    }
    else
    {
        if (syscfg_commit() != 0)
        {
            CcspTraceError(("syscfg_commit failed: %s %s\n", name, value));
            ret = ANSC_STATUS_FAILURE;
        }
    }

    return ret;
}

ANSC_STATUS syscfg_set_bool(const char* name, int value)
{
    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
    char buf[10];
    memset(buf,0,sizeof(buf));

    sprintf(buf, "%d", value);
    if (syscfg_set(NULL, name, buf) != 0)
    {
        CcspTraceError(("syscfg_set failed: %s %d\n", name, value));
        ret = ANSC_STATUS_FAILURE;
    }
    else
    {
        if (syscfg_commit() != 0)
            CcspTraceError(("syscfg_commit failed: %s %d\n", name, value));
        ret = ANSC_STATUS_FAILURE;
    }

    return ret;
}

ANSC_STATUS ipv6Info_init(dhcpv6_data_t *ipv6Data)
{
    memset(ipv6Data, 0, sizeof(dhcpv6_data_t));
    sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_FIELD_IPV6_DNS_PRIMARY, "", 0);
    sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_FIELD_IPV6_DNS_SECONDARY, "", 0);
    sysevent_set(sysevent_fd, sysevent_token,SYSEVENT_FIELD_IPV6_DOMAIN, "", 0);
    syscfg_set_string(SYSCFG_FIELD_IPV6_PREFIX_ADDRESS, "");
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS ipv4Info_init(dhcpv4_data_t *ipv4Data, const char *wanIfName)
{
    char name[BUFLEN_64] = {0};
    char value[BUFLEN_64] = {0};
    memset(ipv4Data, 0, sizeof(dhcpv4_data_t));
    strncpy(ipv4Data->ip, "0.0.0.0", strlen("0.0.0.0"));
    strncpy(ipv4Data->mask, "0.0.0.0", strlen("0.0.0.0"));
    strncpy(ipv4Data->gateway, "0.0.0.0", strlen("0.0.0.0"));
    strncpy(ipv4Data->dnsServer, "0.0.0.0", strlen("0.0.0.0"));
    strncpy(ipv4Data->dnsServer1, "0.0.0.0", strlen("0.0.0.0"));
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_CURRENT_WAN_IPADDR, "0.0.0.0", 0);
    snprintf(name, sizeof(name), SYSEVENT_IPV4_START_TIME, wanIfName);
    sysevent_set(sysevent_fd, sysevent_token, name, "0", 0);
    return ipv4Info_set(ipv4Data,wanIfName);
}

ANSC_STATUS ipv4Info_set(const dhcpv4_data_t *dhcp4Info, const char *wanIfName)
{
    char name[BUFLEN_64] = {0};
    char value[BUFLEN_64] = {0};

    snprintf(name, sizeof(name), SYSEVENT_CURRENT_WAN_IFNAME);
    sysevent_set(sysevent_fd, sysevent_token, name, dhcp4Info->dhcpcInterface, 0);

    snprintf(name, sizeof(name), SYSEVENT_IPV4_IP_ADDRESS, wanIfName);
    sysevent_set(sysevent_fd, sysevent_token, name, dhcp4Info->ip, 0);

    //same as SYSEVENT_IPV4_IP_ADDRESS. But this is required in other components
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_WAN_ADDRESS, dhcp4Info->ip, 0);

    snprintf(name, sizeof(name), SYSEVENT_IPV4_SUBNET, wanIfName);
    sysevent_set(sysevent_fd, sysevent_token, name, dhcp4Info->mask, 0);

    //same as SYSEVENT_IPV4_SUBNET. But this is required in other components
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_WAN_SUBNET, dhcp4Info->mask, 0);

    snprintf(name, sizeof(name), SYSEVENT_IPV4_GW_ADDRESS, wanIfName);
    sysevent_set(sysevent_fd, sysevent_token, name, dhcp4Info->gateway, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_DEFAULT_ROUTER, dhcp4Info->gateway, 0);

    snprintf(name, sizeof(name), SYSEVENT_IPV4_DNS_NUMBER, wanIfName);
    sysevent_set(sysevent_fd, sysevent_token, name, "2", 0);

    snprintf(name, sizeof(name), SYSEVENT_IPV4_DNS_PRIMARY, wanIfName);
    sysevent_set(sysevent_fd, sysevent_token, name, dhcp4Info->dnsServer, 0);

    snprintf(name, sizeof(name), SYSEVENT_IPV4_DNS_SECONDARY, wanIfName);
    sysevent_set(sysevent_fd, sysevent_token, name, dhcp4Info->dnsServer1, 0);

    snprintf(name, sizeof(name), SYSEVENT_IPV4_DS_CURRENT_RATE, wanIfName);
    snprintf(value, sizeof(value), "%d", dhcp4Info->downstreamCurrRate);
    sysevent_set(sysevent_fd, sysevent_token, name, value, 0);

    snprintf(name, sizeof(name), SYSEVENT_IPV4_US_CURRENT_RATE, wanIfName);
    snprintf(value, sizeof(value), "%d", dhcp4Info->upstreamCurrRate);
    sysevent_set(sysevent_fd, sysevent_token, name, value, 0);

    if (dhcp4Info->isTimeOffsetAssigned)
    {
        snprintf(value, sizeof(value), "@%d", dhcp4Info->timeOffset);
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_TIME_OFFSET, value, 0);
    }
    else
    {
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_TIME_OFFSET, "", 0);
    }

    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_TIME_ZONE, dhcp4Info->timeZone, 0);

    snprintf(name,sizeof(name),SYSEVENT_IPV4_DHCP_SERVER,dhcp4Info->dhcpcInterface);
    sysevent_set(sysevent_fd, sysevent_token, name, dhcp4Info->dhcpServerId,0);

    snprintf(name,sizeof(name),SYSEVENT_IPV4_DHCP_STATE ,dhcp4Info->dhcpcInterface);
    sysevent_set(sysevent_fd, sysevent_token, name, dhcp4Info->dhcpState,0);

    snprintf(name,sizeof(name), SYSEVENT_IPV4_LEASE_TIME, dhcp4Info->dhcpcInterface);
    snprintf(value, sizeof(value), "%u",dhcp4Info->leaseTime);
    sysevent_set(sysevent_fd, sysevent_token, name, value, 0);

    return ANSC_STATUS_SUCCESS;
}

#ifdef FEATURE_MAPT
ANSC_STATUS maptInfo_set(const MaptData_t *maptInfo)
{
    if (NULL == maptInfo)
    {
        CcspTraceError(("Invalid arguments \n"));
        return ANSC_STATUS_FAILURE;
    }

    char name[BUFLEN_64] = {0};
    char value[BUFLEN_64] = {0};

    snprintf(name, sizeof(name), SYSEVENT_MAPT_CONFIG_FLAG);
    sysevent_set(sysevent_fd, sysevent_token, name, maptInfo->maptConfigFlag, 0);

    snprintf(name, sizeof(name),SYSEVENT_MAPT_RATIO);
    snprintf(value, sizeof(value), "%d", maptInfo->ratio);
    sysevent_set(sysevent_fd, sysevent_token, name, value, 0);

    snprintf(name, sizeof(name), SYSEVENT_MAPT_IPADDRESS);
    sysevent_set(sysevent_fd, sysevent_token, name, maptInfo->ipAddressString, 0);

    snprintf(name, sizeof(name), SYSEVENT_MAP_BR_IPV6_PREFIX);
    sysevent_set(sysevent_fd, sysevent_token, name, maptInfo->brIpv6PrefixString, 0);

    snprintf(name, sizeof(name), SYSEVENT_MAP_RULE_IPADDRESS);
    sysevent_set(sysevent_fd, sysevent_token, name, maptInfo->ruleIpAddressString, 0);

    snprintf(name, sizeof(name), SYSEVENT_MAPT_IPV6_ADDRESS);
    sysevent_set(sysevent_fd, sysevent_token, name, maptInfo->ipv6AddressString, 0);

    snprintf(name, sizeof(name), SYSEVENT_MAP_RULE_IPV6_ADDRESS);
    sysevent_set(sysevent_fd, sysevent_token, name, maptInfo->ruleIpv6AddressString, 0);

    snprintf(name, sizeof(name), SYSEVENT_MAPT_PSID_OFFSET);
    snprintf(value, sizeof(value), "%d", maptInfo->psidOffset);
    sysevent_set(sysevent_fd, sysevent_token, name, value, 0);

    snprintf(name, sizeof(name), SYSEVENT_MAPT_PSID_VALUE);
    snprintf(value, sizeof(value), "%d", maptInfo->psidValue);
    sysevent_set(sysevent_fd, sysevent_token, name, value, 0);

    snprintf(name, sizeof(name), SYSEVENT_MAPT_PSID_LENGTH);
    snprintf(value, sizeof(value), "%d", maptInfo->psidLen);
    sysevent_set(sysevent_fd, sysevent_token, name, value, 0);

    if(maptInfo->maptAssigned)
    {
        strncpy(value, "MAPT", 4);
    }
    else if(maptInfo->mapeAssigned)
    {
        strncpy(value, "MAPE", 4);
    }
    else
    {
        strncpy(value, "NON-MAP", 7);
    }
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_MAP_TRANSPORT_MODE, value, 0);

    if(maptInfo->isFMR ==1)
    {
        strncpy(value, "TRUE", 4);
    }
    else
    {
        strncpy(value, "FALSE", 5);
    }
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_MAP_IS_FMR, value, 0);

    snprintf(value, sizeof(value), "%d", maptInfo->eaLen);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_MAP_EA_LENGTH, value, 0);

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS maptInfo_reset()
{
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_MAPT_CONFIG_FLAG, RESET, 0);

    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_MAPT_RATIO, "", 0);

    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_MAPT_IPADDRESS, "", 0);

    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_MAPT_IPV6_ADDRESS, "", 0);

    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_MAPT_PSID_OFFSET, "", 0);

    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_MAPT_PSID_VALUE, "", 0);

    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_MAPT_PSID_LENGTH, "", 0);

    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_MAP_TRANSPORT_MODE, "", 0);

    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_MAP_IS_FMR, "", 0);

    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_MAP_EA_LENGTH, "", 0);

    return ANSC_STATUS_SUCCESS;
}
#endif // FEATURE_MAPT
