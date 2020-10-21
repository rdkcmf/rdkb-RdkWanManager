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
#include "wanmgr_dhcpv4_apis.h"
#include "wanmgr_interface_sm.h"
#include "wanmgr_sysevents.h"
#include "wanmgr_ipc.h"
#include "wanmgr_utils.h"
#include "wanmgr_rdkbus_apis.h"

#include <sysevent/sysevent.h>
extern int sysevent_fd;
extern token_t sysevent_token;

#define P2P_SUB_NET_MASK   "255.255.255.255"
#define DHCP_STATE_UP      "Up"
#define DHCP_STATE_DOWN    "Down"

static ANSC_STATUS wanmgr_dchpv4_get_ipc_msg_info(WANMGR_IPV4_DATA* pDhcpv4Data, ipc_dhcpv4_data_t* pIpcIpv4Data)
{
    if((pDhcpv4Data == NULL) || (pIpcIpv4Data == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }
    memcpy(pDhcpv4Data->ifname, pIpcIpv4Data->dhcpcInterface, BUFLEN_64);
    memcpy(pDhcpv4Data->ip, pIpcIpv4Data->ip, BUFLEN_32);
    memcpy(pDhcpv4Data->mask , pIpcIpv4Data->mask, BUFLEN_32);
    memcpy(pDhcpv4Data->gateway, pIpcIpv4Data->gateway, BUFLEN_32);
    memcpy(pDhcpv4Data->dnsServer, pIpcIpv4Data->dnsServer, BUFLEN_64);
    memcpy(pDhcpv4Data->dnsServer1, pIpcIpv4Data->dnsServer1, BUFLEN_64);

    return ANSC_STATUS_SUCCESS;
}


ANSC_STATUS wanmgr_handle_dchpv4_event_data(DML_WAN_IFACE* pIfaceData)
{
    if(NULL == pIfaceData)
    {
       return ANSC_STATUS_FAILURE;
    }

    ipc_dhcpv4_data_t* pDhcpcInfo = pIfaceData->IP.pIpcIpv4Data;
    if(NULL == pDhcpcInfo)
    {
       return ANSC_STATUS_BAD_PARAMETER;
    }

    CcspTraceInfo(("%s %d - Enter ProcessDhcpcStateChanged() \n", __FUNCTION__, __LINE__));
    bool IPv4ConfigChanged = FALSE;


    if (strcmp(pIfaceData->IP.Ipv4Data.ip, pDhcpcInfo->ip) ||
      strcmp(pIfaceData->IP.Ipv4Data.mask, pDhcpcInfo->mask) ||
      strcmp(pIfaceData->IP.Ipv4Data.gateway, pDhcpcInfo->gateway) ||
      strcmp(pIfaceData->IP.Ipv4Data.dnsServer, pDhcpcInfo->dnsServer) ||
      strcmp(pIfaceData->IP.Ipv4Data.dnsServer1, pDhcpcInfo->dnsServer1))
    {
        CcspTraceInfo(("%s %d - IPV4 configuration changed \n", __FUNCTION__, __LINE__));
        IPv4ConfigChanged = TRUE;
    }

    char name[64] = {0};
    char value[64] = {0};
    uint32_t up_time = 0;

    /* ipv4_start_time should be set in every v4 packets */
    snprintf(name,sizeof(name),SYSEVENT_IPV4_START_TIME,pDhcpcInfo->dhcpcInterface);
    up_time = WanManager_getUpTime();
    snprintf(value, sizeof(value), "%u", up_time);
    sysevent_set(sysevent_fd, sysevent_token, name, value, 0);

    if (pDhcpcInfo->addressAssigned)
    {
        CcspTraceInfo(("assigned ip=%s netmask=%s gateway=%s dns server=%s,%s leasetime = %d, rebindtime = %d, renewaltime = %d, dhcp state = %s\n",
                     pDhcpcInfo->ip,
                     pDhcpcInfo->mask,
                     pDhcpcInfo->gateway,
                     pDhcpcInfo->dnsServer,
                     pDhcpcInfo->dnsServer1,
                     pDhcpcInfo->leaseTime,
                     pDhcpcInfo->rebindingTime,
                     pDhcpcInfo->renewalTime,
                     pDhcpcInfo->dhcpState));

        if (IPv4ConfigChanged)
        {
            if (wanmgr_sysevents_ipv4Info_set(pDhcpcInfo, pDhcpcInfo->dhcpcInterface) != ANSC_STATUS_SUCCESS)
            {
                CcspTraceError(("%s %d - Could not store ipv4 data!", __FUNCTION__, __LINE__));
            }
            //Update isIPv4ConfigChanged flag.
            pIfaceData->IP.Ipv4Changed = TRUE;
        }
        else
        {
            CcspTraceInfo(("%s %d - IPV4 optional configuration received \n", __FUNCTION__, __LINE__));
            snprintf(name, sizeof(name), SYSEVENT_IPV4_DS_CURRENT_RATE, pDhcpcInfo->dhcpcInterface);
            snprintf(value, sizeof(value), "%d", pDhcpcInfo->downstreamCurrRate);
            sysevent_set(sysevent_fd, sysevent_token, name, value, 0);

            snprintf(name, sizeof(name), SYSEVENT_IPV4_US_CURRENT_RATE, pDhcpcInfo->dhcpcInterface);
            snprintf(value, sizeof(value), "%d", pDhcpcInfo->upstreamCurrRate);
            sysevent_set(sysevent_fd, sysevent_token, name, value, 0);

            snprintf(name, sizeof(name), SYSEVENT_IPV4_LEASE_TIME, pDhcpcInfo->dhcpcInterface);
            snprintf(value, sizeof(value), "%d", pDhcpcInfo->leaseTime);
            sysevent_set(sysevent_fd, sysevent_token, name, value, 0);

            if (pDhcpcInfo->isTimeOffsetAssigned)
            {
                snprintf(value, sizeof(value), "@%d", pDhcpcInfo->timeOffset);
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_TIME_OFFSET, value, 0);
            }
            else
            {
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_TIME_OFFSET, "", 0);
            }

            sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_TIME_ZONE, pDhcpcInfo->timeZone, 0);
        }

        // update current IPv4 data
        wanmgr_dchpv4_get_ipc_msg_info(&(pIfaceData->IP.Ipv4Data), pDhcpcInfo);
        WanManager_UpdateInterfaceStatus(pIfaceData, WANMGR_IFACE_CONNECTION_UP);

        return ANSC_STATUS_SUCCESS;
    }

    if (pDhcpcInfo->isExpired)
    {
        CcspTraceInfo(("DHCPC Lease expired!!!!!!!!!!\n"));
        // update current IPv4 data
        wanmgr_dchpv4_get_ipc_msg_info(&(pIfaceData->IP.Ipv4Data), pDhcpcInfo);
        WanManager_UpdateInterfaceStatus(pIfaceData, WANMGR_IFACE_CONNECTION_DOWN);
    }

    return ANSC_STATUS_SUCCESS;
}

void* IPCPStateChangeHandler (void *arg)
{
    char acTmpReturnValue[256] = {0};
    char acTmpQueryParam[256] = {0};
    char *token = NULL;
    int index = -1;

    const char *dhcpcInterface = (char *) arg;
    if(NULL == dhcpcInterface)
    {
       return ANSC_STATUS_FAILURE;
    }
    pthread_detach(pthread_self());

    //get iface data
    WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceDataByName_locked(dhcpcInterface);
    if(pWanDmlIfaceData != NULL)
    {
        DML_WAN_IFACE* pIfaceData = &(pWanDmlIfaceData->data);

        // Get PPP instance number
        sscanf(pIfaceData->PPP.Path, "%*[^0-9]%d", &index);

        //check if previously message was already handled
        if(pIfaceData->IP.pIpcIpv4Data == NULL)
        {
            //allocate
            pIfaceData->IP.pIpcIpv4Data = (ipc_dhcpv4_data_t*) malloc(sizeof(ipc_dhcpv4_data_t));
            if(pIfaceData->IP.pIpcIpv4Data != NULL)
            {
                switch (pIfaceData->PPP.IPCPStatus)
                {
                     case WAN_IFACE_IPCP_STATUS_UP:
                     {
                         pIfaceData->IP.pIpcIpv4Data->isExpired = FALSE;
                         pIfaceData->IP.pIpcIpv4Data->addressAssigned = TRUE;
                         strncpy (pIfaceData->IP.pIpcIpv4Data->dhcpState, DHCP_STATE_UP, BUFLEN_64);

                         //Query for Local IP address
                         snprintf( acTmpQueryParam, sizeof(acTmpQueryParam ), PPP_IPCP_LOCAL_IPADDRESS, index );
                         memset( acTmpReturnValue, 0, sizeof( acTmpReturnValue ) );
                         if ( ANSC_STATUS_FAILURE == WanMgr_RdkBus_GetParamValues( PPPMGR_COMPONENT_NAME, PPPMGR_DBUS_PATH, acTmpQueryParam, acTmpReturnValue ) )
                         {
                             CcspTraceError(("%s %d Failed to get param value for paramname %s \n", __FUNCTION__, __LINE__, acTmpQueryParam));
                             goto EXIT;
                         }
                         if (acTmpReturnValue[0] != '\0')
                         {
                             strncpy (pIfaceData->IP.pIpcIpv4Data->ip, acTmpReturnValue, BUFLEN_32);
                         }
                         else
                         {
                             strncpy (pIfaceData->IP.pIpcIpv4Data->dhcpState, DHCP_STATE_DOWN, BUFLEN_64);
                             pIfaceData->IP.pIpcIpv4Data->addressAssigned = FALSE;
                         }

                         //Query for Remote IP address
                         snprintf( acTmpQueryParam, sizeof(acTmpQueryParam ), PPP_IPCP_REMOTEIPADDRESS, index );
                         memset( acTmpReturnValue, 0, sizeof( acTmpReturnValue ) );
                         if ( ANSC_STATUS_FAILURE == WanMgr_RdkBus_GetParamValues( PPPMGR_COMPONENT_NAME, PPPMGR_DBUS_PATH, acTmpQueryParam, acTmpReturnValue ) )
                         {
                             CcspTraceError(("%s %d Failed to get param value for paramname %s \n", __FUNCTION__, __LINE__, acTmpQueryParam));
                             goto EXIT;
                         }
                         if (acTmpReturnValue[0] != '\0')
                         {
                              strncpy (pIfaceData->IP.pIpcIpv4Data->gateway, acTmpReturnValue, BUFLEN_32);
                         }
                         else
                         {
                             strncpy (pIfaceData->IP.pIpcIpv4Data->dhcpState, DHCP_STATE_DOWN, BUFLEN_64);
                             pIfaceData->IP.pIpcIpv4Data->addressAssigned = FALSE;
                         }
                         //Query for DNS Servers
                         snprintf( acTmpQueryParam, sizeof(acTmpQueryParam ), PPP_IPCP_DNS_SERVERS, index );
                         memset( acTmpReturnValue, 0, sizeof( acTmpReturnValue ) );
                         if ( ANSC_STATUS_FAILURE == WanMgr_RdkBus_GetParamValues( PPPMGR_COMPONENT_NAME, PPPMGR_DBUS_PATH, acTmpQueryParam, acTmpReturnValue ) )
                         {
                             CcspTraceError(("%s %d Failed to get param value for paramname %s \n", __FUNCTION__, __LINE__, acTmpQueryParam));
                             goto EXIT;
                         }
                         if (acTmpReturnValue[0] != '\0')
                         {
                             //Return first DNS Server
                             token = strtok(acTmpReturnValue, ",");
                             if (token != NULL)
                             {
                                 strcpy (pIfaceData->IP.pIpcIpv4Data->dnsServer, token );
                                 //Return first DNS Server
                                 token = strtok(NULL, ",");
                                 if (token != NULL)
                                 {
                                     strcpy (pIfaceData->IP.pIpcIpv4Data->dnsServer1, token);
                                 }
                             }
                         }

                          strncpy (pIfaceData->IP.pIpcIpv4Data->dhcpcInterface, dhcpcInterface, BUFLEN_64);
                          strncpy (pIfaceData->IP.pIpcIpv4Data->mask, P2P_SUB_NET_MASK, BUFLEN_32);
                         wanmgr_handle_dchpv4_event_data(pIfaceData);
                         break;
                     }
                     case WAN_IFACE_IPCP_STATUS_DOWN:
                     {
                         strncpy (pIfaceData->IP.pIpcIpv4Data->dhcpcInterface, dhcpcInterface, BUFLEN_64);
                        strncpy (pIfaceData->IP.pIpcIpv4Data->dhcpState, DHCP_STATE_DOWN, BUFLEN_64);
                        pIfaceData->IP.pIpcIpv4Data->addressAssigned = FALSE;
                        wanmgr_handle_dchpv4_event_data(pIfaceData);
                         break;
                     }
                }

            }
        }
        
    }

EXIT:
    if(pWanDmlIfaceData != NULL)
    {
        WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
    }
    if (arg != NULL)
    {
        free(arg);
    }

    return NULL;
}
