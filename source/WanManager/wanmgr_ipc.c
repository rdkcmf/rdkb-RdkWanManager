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

/* ---- Include Files ---------------------------------------- */
#include <sys/un.h>
#include <errno.h>
#include "wanmgr_ipc.h"
#include "wanmgr_data.h"
#include "wanmgr_sysevents.h"
#include "wanmgr_net_utils.h"
#include "wanmgr_dhcpv4_apis.h"


#define WANMGR_MAX_IPC_PROCCESS_TRY             5
#define WANMGR_IPC_PROCCESS_TRY_WAIT_TIME       30000 //us


typedef struct _WanIpcCtrl_t_
{
    INT interfaceIdx;
} WanIpcCtrl_t;


static int   ipcListenFd;   /* Unix domain IPC listening socket fd */





///* ---- Private Functions ------------------------------------ */
//#ifdef FEATURE_IPOE_HEALTH_CHECK
//static ANSC_STATUS ProcessIpoeHealthCheckFailedIpv4Msg(void);
//static ANSC_STATUS ProcessIpoeHealthCheckFailedRenewIpv4Msg(void);
//static ANSC_STATUS ProcessIpoeHealthCheckFailedIpv6Msg(void);
//static ANSC_STATUS ProcessIpoeHealthCheckFailedRenewIpv6Msg(void);
//static ANSC_STATUS ProcessIpoeHealthCheckDhcp6cRestart(void);
//#endif /*FEATURE_IPOE_HEALTH_CHECK*/
//
/* ---- Private Variables ------------------------------------ */


//#ifdef FEATURE_IPOE_HEALTH_CHECK
//static ANSC_STATUS ProcessIpoeHealthCheckFailedIpv4Msg(void)
//{
//    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
//    /* Kill DHCPv4 client */
//    WanManager_StopDhcpv4Client(TRUE);
//
//    // update local wan interface data
//    WanManager_UpdateWANInterface(WAN_CONNECTION_DOWN, NULL);
//
//    return ret;
//}
//
//static ANSC_STATUS ProcessIpoeHealthCheckFailedRenewIpv4Msg(void)
//{
//    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
//
//    /*send force renew request to DHCPC*/
//    if (WanManager_IsApplicationRunning(DHCPV4_CLIENT_NAME) == TRUE)
//    {
//        int pid = util_getPidByName(DHCPV4_CLIENT_NAME);
//        CcspTraceInfo(("sending SIGUSR1 to %s[pid=%d], this will let the %s to send renew packet out \n", DHCPV4_CLIENT_NAME, pid, DHCPV4_CLIENT_NAME));
//        util_signalProcess(pid, SIGUSR1);
//    }
//
//    // update local wan interface data
//    WanManager_UpdateWANInterface(WAN_CONNECTION_DOWN, NULL);
//
//    return ret;
//}
//static ANSC_STATUS ProcessIpoeHealthCheckFailedIpv6Msg(void)
//{
//    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
//
//    /* Kill DHCPv6 client */
//    WanManager_StopDhcpv6Client(TRUE);
//
//    // update local wan interface data
//    WanManager_UpdateWANInterface(WAN_CONNECTION_IPV6_DOWN, NULL);
//
//    return ret;
//}
//
//static ANSC_STATUS ProcessIpoeHealthCheckFailedRenewIpv6Msg(void)
//{
//    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
//
//    if (WanManager_IsApplicationRunning(DHCPV6_CLIENT_NAME) == TRUE)
//    {
//        int pid = prctl_getPidByName(DHCPV6_CLIENT_NAME);
//        CcspTraceInfo(("sending SIGUSR2 to dhcp6c, this will let the dhcp6c to send renew packet out \n"));
//        util_signalProcess(pid, SIGUSR2);
//    }
//
//    // update local wan interface data
//    WanManager_UpdateWANInterface(WAN_CONNECTION_IPV6_DOWN, NULL);
//
//    return ret;
//}
//
//static ANSC_STATUS ProcessIpoeHealthCheckDhcp6cRestart(void)
//{
//    ANSC_STATUS ret = ANSC_STATUS_SUCCESS;
//
//    /* Kill DHCPv6 client */
//    CcspTraceInfo(("Stoping DHCPv6 Client \n"));
//    if ((ret = WanManager_StopDhcpv6Client(FALSE)) != ANSC_STATUS_SUCCESS)
//    {
//        CcspTraceError(("Could not stop Dhcpv6 Client!\n"));
//        return ret;
//    }
//    CcspTraceInfo(("Starting DHCPv6 Client \n"));
//    WAN_CONFIG_DATA* pWanConfigData = WanMgr_GetConfigData_locked();
//    if(pWanConfigData != NULL)
//    {
//        WanData_t* pWanData = &(pWanConfigData->data);
//
//        if ((ret = WanManager_StartDhcpv6Client(pWanData->ifName, FALSE)) != ANSC_STATUS_SUCCESS)
//        {
//            CcspTraceError(("Could not start Dhcpv6 Client!\n"));
//        }
//
//        WanMgrDml_GetConfigData_release(pWanConfigData);
//    }
//
//
//    return ret;
//}
//#endif /*FEATURE_IPOE_HEALTH_CHECK*/
//

//ANSC_STATUS WanManager_sendIpcMsgToClient_AndGetReplyWithTimeout(ipc_msg_payload_t * payload)
//{
//    return ANSC_STATUS_SUCCESS;
//}
//
//
//ANSC_STATUS WanManager_sendIpcMsgToClient(ipc_msg_payload_t * payload)
//{
//    return ANSC_STATUS_SUCCESS;
//}


static ANSC_STATUS WanMgr_IpcNewIpv4Msg(ipc_dhcpv4_data_t* pNewIpv4Msg)
{
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;
    INT try = 0;

    while((retStatus != ANSC_STATUS_SUCCESS) && (try < WANMGR_MAX_IPC_PROCCESS_TRY))
    {
        //get iface data
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceDataByName_locked(pNewIpv4Msg->dhcpcInterface);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pIfaceData = &(pWanDmlIfaceData->data);

            //check if previously message was already handled
            if(pIfaceData->IP.pIpcIpv4Data == NULL)
            {
                //allocate
                pIfaceData->IP.pIpcIpv4Data = (ipc_dhcpv4_data_t*) malloc(sizeof(ipc_dhcpv4_data_t));
                if(pIfaceData->IP.pIpcIpv4Data != NULL)
                {
                    // copy data
                    memcpy(pIfaceData->IP.pIpcIpv4Data, pNewIpv4Msg, sizeof(ipc_dhcpv4_data_t));
                    retStatus = ANSC_STATUS_SUCCESS;
                }
            }

            //release lock
            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }

        if(retStatus != ANSC_STATUS_SUCCESS)
        {
            try++;
            usleep(WANMGR_IPC_PROCCESS_TRY_WAIT_TIME);
        }
    }

    return retStatus;
}


static ANSC_STATUS WanMgr_IpcNewIpv6Msg(ipc_dhcpv6_data_t* pNewIpv6Msg)
{
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;
    INT try = 0;

    while((retStatus != ANSC_STATUS_SUCCESS) && (try < WANMGR_MAX_IPC_PROCCESS_TRY))
    {
        //get iface data
        WanMgr_Iface_Data_t* pWanDmlIfaceData = WanMgr_GetIfaceDataByName_locked(pNewIpv6Msg->ifname);
        if(pWanDmlIfaceData != NULL)
        {
            DML_WAN_IFACE* pIfaceData = &(pWanDmlIfaceData->data);

            //check if previously message was already handled
            if(pIfaceData->IP.pIpcIpv6Data == NULL)
            {
                //allocate
                pIfaceData->IP.pIpcIpv6Data = (ipc_dhcpv6_data_t*) malloc(sizeof(ipc_dhcpv6_data_t));
                if(pIfaceData->IP.pIpcIpv6Data != NULL)
                {
                    // copy data
                    memcpy(pIfaceData->IP.pIpcIpv6Data, pNewIpv6Msg, sizeof(ipc_dhcpv6_data_t));
                    retStatus = ANSC_STATUS_SUCCESS;
                }
            }

            //release lock
            WanMgrDml_GetIfaceData_release(pWanDmlIfaceData);
        }

        if(retStatus != ANSC_STATUS_SUCCESS)
        {
            try++;
            usleep(WANMGR_IPC_PROCCESS_TRY_WAIT_TIME);
        }
    }

    return retStatus;
}



static void* IpcServerThread( void *arg )
{

    //detach thread from caller stack
    pthread_detach(pthread_self());

    // local variables
    BOOL bRunning = TRUE;

    int bytes = 0;
    int msg_size = sizeof(ipc_msg_payload_t);
    ipc_msg_payload_t ipc_msg;
    memset (&ipc_msg, 0, sizeof(ipc_msg_payload_t));

    while (bRunning)
    {
        bytes = nn_recv(ipcListenFd, (ipc_msg_payload_t *)&ipc_msg, msg_size, 0);
        if ((bytes == msg_size))
        {
            switch(ipc_msg.msg_type)
            {
                case DHCPC_STATE_CHANGED:
                    if (WanMgr_IpcNewIpv4Msg(&(ipc_msg.data.dhcpv4)) != ANSC_STATUS_SUCCESS)
                    {
                        CcspTraceError(("[%s-%d] Failed to proccess DHCPv4 state change message \n", __FUNCTION__, __LINE__));
                    }
                    break;
                case DHCP6C_STATE_CHANGED:
                    if (WanMgr_IpcNewIpv6Msg(&(ipc_msg.data.dhcpv6)) != ANSC_STATUS_SUCCESS)
                    {
                        CcspTraceError(("[%s-%d] Failed to proccess DHCPv6 state change message \n", __FUNCTION__, __LINE__));
                    }
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


ANSC_STATUS WanMgr_StartIpcServer()
{
    pthread_t ipcThreadId;
    ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;
    int ret = -1;

    if(IpcServerInit() != ANSC_STATUS_SUCCESS)
    {
        CcspTraceInfo(("Failed to initialise IPC messaging"));
        return -1;
    }

    //create thread
    ret = pthread_create( &ipcThreadId, NULL, &IpcServerThread, NULL );

    if( 0 != ret )
    {
        CcspTraceInfo(("%s %d - Failed to start IPC Thread Error:%d\n", __FUNCTION__, __LINE__, ret));
    }
    else
    {
        CcspTraceInfo(("%s %d - IPC Thread Started Successfully\n", __FUNCTION__, __LINE__));
        retStatus = ANSC_STATUS_SUCCESS;
    }
    return retStatus ;
}

ANSC_STATUS WanMgr_CloseIpcServer(void)
{
    //nn_close(ipcListenFd);

    return ANSC_STATUS_SUCCESS ;
}
