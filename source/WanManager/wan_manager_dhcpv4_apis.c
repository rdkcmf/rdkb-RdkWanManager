/*
 * If not stated otherwise in this file or this component's LICENSE file the
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
#include "wan_manager_private.h"
#include "platform_hal.h"

/**
 * Global structure to hold wan interface data.
 * This structure object defined in ipc module and this
 * can be also required here.
 **/
extern WanData_t gWanData;
extern pthread_mutex_t gmWanDataMutex;

ANSC_STATUS WanManager_Dhcpv4_ProcessStateChangedMsg(dhcpv4_data_t * dhcpcInfo)
{
    if (NULL == dhcpcInfo)
    {
        return ANSC_STATUS_BAD_PARAMETER;
    }

    CcspTraceInfo(("%s %d - Enter ProcessDhcpcStateChanged() \n", __FUNCTION__, __LINE__));
    bool IPv4ConfigChanged = FALSE;
    dhcpv4_data_t dhcpcInfoPrvs;

    // Get previous data
    // Protect the data before copying as the data being shared by multiple threads.
    pthread_mutex_lock(&gmWanDataMutex);
    memcpy(&dhcpcInfoPrvs, &gWanData.ipv4Data, sizeof(dhcpv4_data_t));
    pthread_mutex_unlock(&gmWanDataMutex);

    if (strcmp(dhcpcInfoPrvs.ip, dhcpcInfo->ip) ||
        strcmp(dhcpcInfoPrvs.mask, dhcpcInfo->mask) ||
        strcmp(dhcpcInfoPrvs.gateway, dhcpcInfo->gateway) ||
        strcmp(dhcpcInfoPrvs.dnsServer, dhcpcInfo->dnsServer) ||
        strcmp(dhcpcInfoPrvs.dnsServer1, dhcpcInfo->dnsServer1))
    {
        CcspTraceInfo(("%s %d - IPV4 configuration changed \n", __FUNCTION__, __LINE__));
        IPv4ConfigChanged = TRUE;
    }

    char name[64] = {0};
    char value[64] = {0};
    uint32_t up_time = 0;

    /* ipv4_start_time should be set in every v4 packets */
    snprintf(name,sizeof(name),SYSEVENT_IPV4_START_TIME,dhcpcInfo->dhcpcInterface);
    up_time = WanManager_getUpTime();
    snprintf(value, sizeof(value), "%u", up_time);
    sysevent_set(sysevent_fd, sysevent_token, name, value, 0);

    if (dhcpcInfo->addressAssigned)
    {
        CcspTraceInfo(("assigned ip=%s netmask=%s gateway=%s dns server=%s,%s leasetime = %d, rebindtime = %d, renewaltime = %d, dhcp state = %s\n",
                       dhcpcInfo->ip,
                       dhcpcInfo->mask,
                       dhcpcInfo->gateway,
                       dhcpcInfo->dnsServer,
                       dhcpcInfo->dnsServer1,
                       dhcpcInfo->leaseTime,
                       dhcpcInfo->rebindingTime,
                       dhcpcInfo->renewalTime,
                       dhcpcInfo->dhcpState));
        if (IPv4ConfigChanged)
        {
            if (ipv4Info_set(dhcpcInfo, dhcpcInfo->dhcpcInterface) != ANSC_STATUS_SUCCESS)
            {
                CcspTraceError(("%s %d - Could not store ipv4 data!", __FUNCTION__, __LINE__));
            }
            //Update isIPv4ConfigChanged flag.
            WanManager_UpdateGlobalWanData(IPV4_CONFIG_CHANGED, TRUE);
        }
        else
        {
            CcspTraceInfo(("%s %d - IPV4 optional configuration received \n", __FUNCTION__, __LINE__));
            snprintf(name, sizeof(name), SYSEVENT_IPV4_DS_CURRENT_RATE, dhcpcInfo->dhcpcInterface);
            snprintf(value, sizeof(value), "%d", dhcpcInfo->downstreamCurrRate);
            sysevent_set(sysevent_fd, sysevent_token, name, value, 0);

            snprintf(name, sizeof(name), SYSEVENT_IPV4_US_CURRENT_RATE, dhcpcInfo->dhcpcInterface);
            snprintf(value, sizeof(value), "%d", dhcpcInfo->upstreamCurrRate);
            sysevent_set(sysevent_fd, sysevent_token, name, value, 0);

            snprintf(name, sizeof(name), SYSEVENT_IPV4_LEASE_TIME, dhcpcInfo->dhcpcInterface);
            snprintf(value, sizeof(value), "%d", dhcpcInfo->leaseTime);
            sysevent_set(sysevent_fd, sysevent_token, name, value, 0);

            if (dhcpcInfo->isTimeOffsetAssigned)
            {
                snprintf(value, sizeof(value), "@%d", dhcpcInfo->timeOffset);
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_TIME_OFFSET, value, 0);
            }
            else
            {
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_TIME_OFFSET, "", 0);
            }

            sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_TIME_ZONE, dhcpcInfo->timeZone, 0);
        }

        // update current IPv4 data
        pthread_mutex_lock(&gmWanDataMutex);
        memcpy(&gWanData.ipv4Data, dhcpcInfo, sizeof(dhcpv4_data_t));
        pthread_mutex_unlock(&gmWanDataMutex);

        WanManager_UpdateWANInterface(WAN_CONNECTION_UP, dhcpcInfo->dhcpcInterface);
        return ANSC_STATUS_SUCCESS;
    }

    if (dhcpcInfo->isExpired)
    {
        CcspTraceInfo(("DHCPC Lease expired!!!!!!!!!!\n"));
        // update current IPv4 data
        pthread_mutex_lock(&gmWanDataMutex);
        memcpy(&gWanData.ipv4Data, dhcpcInfo, sizeof(dhcpv4_data_t));
        pthread_mutex_unlock(&gmWanDataMutex);

        WanManager_UpdateWANInterface(WAN_CONNECTION_DOWN, dhcpcInfo->dhcpcInterface);
    }
    return ANSC_STATUS_SUCCESS;
}
