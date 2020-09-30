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

#include <unistd.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <wan_manager.h>
#include <wan_manager_private.h>
#include "platform_hal.h"
#include "sysevent/sysevent.h"
#include "wan_interface_dml_apis.h"
#include "wan_interface_internal.h"

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
extern unsigned int resend_msg[2];

/*WAN Manager States*/
static eWanState_t wan_state_configuring_wan(WanInterfaceData_t *wanIf);
static eWanState_t wan_state_validating_wan(WanInterfaceData_t *wanIf);
static eWanState_t wan_state_obtaining_ip_addresses(WanInterfaceData_t *wanIf);
static eWanState_t wan_state_ipv4_leased(WanInterfaceData_t *wanIf);
static eWanState_t wan_state_ipv6_leased(WanInterfaceData_t *wanIf);
static eWanState_t wan_state_dual_stack_active(WanInterfaceData_t *wanIf);
static eWanState_t wan_state_ipv4_over_ipv6_active(WanInterfaceData_t *wanIf);
static eWanState_t wan_state_refreshing_wan(WanInterfaceData_t *wanIf);
static eWanState_t wan_state_deconfiguring_wan(WanInterfaceData_t *wanIf);

/*WAN Manager Transitions*/
static eWanState_t wan_transition_start(WanInterfaceData_t *wanIf);
static eWanState_t wan_transition_physical_interface_down(WanInterfaceData_t *wanIf);
static eWanState_t wan_transition_wan_up(WanInterfaceData_t *wanIf);
static eWanState_t wan_transition_wan_validated(WanInterfaceData_t *wanIf);
static eWanState_t wan_transition_refreshing_wan(WanInterfaceData_t *wanIf);
static eWanState_t wan_transition_wan_refreshed(WanInterfaceData_t *wanIf);
static eWanState_t wan_transition_ipv4_up(WanInterfaceData_t *wanIf);
static eWanState_t wan_transition_ipv4_down(WanInterfaceData_t *wanIf);
static eWanState_t wan_transition_ipv6_up(WanInterfaceData_t *wanIf);
static eWanState_t wan_transition_ipv6_down(WanInterfaceData_t *wanIf);
static eWanState_t wan_transition_dual_stack_down(WanInterfaceData_t *wanIf);
static eWanState_t wan_transition_ipv4_over_ipv6_up(WanInterfaceData_t *wanIf);
static eWanState_t wan_transition_ipv4_over_ipv6_down(WanInterfaceData_t *wanIf);
static eWanState_t wan_transition_exit(WanInterfaceData_t *wanIf);

/********************************************************************************
 * @brief Configure IPV4 configuration on the interface.
 * This API calls the HAL routine to configure ipv4.
 * @param ifname Wan interface name
 * @param wanData pointer to WanData_t holds the wan data
 * @return RETURN_OK upon success else returned error code.
 *********************************************************************************/
static int wan_setUpIPv4(const char *wanIfName, WanData_t* wanData);

/********************************************************************************
 * @brief Unconfig IPV4 configuration on the interface.
 * This API calls the HAL routine to unconfig ipv4.
 * @param ifname Wan interface name
 * @param wanData pointer to WanData_t holds the wan data
 * @return RETURN_OK upon success else returned error code.
 *********************************************************************************/
static int wan_tearDownIPv4(const char *wanIfName, WanData_t* wanData);

/*************************************************************************************
 * @brief Configure IPV6 configuration on the interface.
 * This API calls the HAL routine to config ipv6.
 * @param ifname Wan interface name
 * @param wanData pointer to WanData_t holds the wan data
 * @return  RETURN_OK upon success else returned error code.
 **************************************************************************************/
static int wan_setUpIPv6(const char *ifname, WanData_t* wanData);

/*************************************************************************************
 * @brief Unconfig IPV6 configuration on the interface.
 * This API calls the HAL routine to unconfig ipv6.
 * @param ifname Wan interface name
 * @param wanData pointer to WanData_t holds the wan data
 * @return RETURN_OK upon success else returned error code.
 **************************************************************************************/
static int wan_tearDownIPv6(const char *ifname, WanData_t* wanData);

/**************************************************************************************
 * @brief Update DNS configuration into /etc/resolv.conf
 * @param wanIfname wan interface name
 * @param addIPv4 boolean flag indicates whether IPv4 DNS data needs to be update
 * @param addIPv6 boolean flag indicates whether IPv6 DNS data needs to be update
 * @return RETURN_OK upon success else ERROR code returned
 **************************************************************************************/
static int wan_updateDNS(const char *wanIfname, BOOL addIPv4, BOOL addIPv6);

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
static int setUpLanPrefixIPv6(WanData_t* wanData);

/*Statemachine Thread*/
static void* WanManagerStateMachineThread( void *arg );
static pthread_key_t       sm_private_key;

int WanManager_Init()
{
    // Initialise syscfg
    if (syscfg_init() < 0)
    {
        CcspTraceError(("failed to initialise syscfg"));
        return -1;
    }

    // Initialize sysevent daemon
    if (WanManager_SyseventInit() < 0)
    {
        return -1;
    }
    // Initialize syscfg value of ipv6 address to release previous value
    syscfg_set_string(SYSCFG_FIELD_IPV6_PREFIX_ADDRESS, "");
}

int WanManager_DeInit()
{
    WanManager_SyseventClose();
    return 0;
}

int WanManager_StartStateMachine(WanInterfaceData_t *wanIf)
{
    WanInterfaceData_t *     wanIfLocal = NULL;
    pthread_t                wanSmThreadId;
    int                      iErrorCode     = 0;
    static int               siKeyCreated   = 0;

    //Need to create pthread key at once
    if( 0 == siKeyCreated )
    {
        if ( 0 != pthread_key_create( &sm_private_key, NULL ) )
        {
            CcspTraceError(("%s %d Unable to create pthread_key\n", __FUNCTION__, __LINE__));
            return -1;
        }
        siKeyCreated = 1;
    }

    //Allocate memory and pass it to thread
    wanIfLocal = ( WanInterfaceData_t * )malloc( sizeof( WanInterfaceData_t ) );
    if( NULL == wanIfLocal )
    {
        CcspTraceError(("%s %d Failed to allocate memory\n", __FUNCTION__, __LINE__));
        return -1;
    }

    //Copy buffer
    memcpy( wanIfLocal , wanIf, sizeof(WanInterfaceData_t) );

    CcspTraceInfo (("%s %d - wan interface name received in the state machine = %s \n", __FUNCTION__, __LINE__, wanIfLocal->ifName));

    //Wanmanager state machine thread
    iErrorCode = pthread_create( &wanSmThreadId, NULL, &WanManagerStateMachineThread, (void*)wanIfLocal );

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

static void* WanManagerStateMachineThread( void *arg )
{
    // event handler
    int n=0;
    struct timeval tv;
    eWanState_t currentSmState   = WAN_STATE_EXIT;

    //Validate buffer
    if ( NULL == arg )
    {
       CcspTraceError(("%s %d Invalid buffer\n", __FUNCTION__,__LINE__));
       //Cleanup current thread when exit
       pthread_exit(NULL);
    }

    WanInterfaceData_t * wanIf = ( WanInterfaceData_t *) arg;
    CcspTraceError(("%s %d Interface name = %s\n", __FUNCTION__,__LINE__, wanIf->ifName));

    //detach thread from caller stack
    pthread_detach(pthread_self());

    //Set thread specific private data
    if ( 0 != pthread_setspecific( sm_private_key, arg ) )
    {
       CcspTraceError(("%s %d Unable to set private key so exiting\n", __FUNCTION__, __LINE__));
       if( NULL != arg )
       {
           free( arg );
           arg = NULL;
       }
       //Cleanup current thread when exit
       pthread_exit(NULL);
    }

    // local variables
    bool bRunning = true;

    // initialise state machine
    currentSmState = wan_transition_start(wanIf); // do this first before anything else to init variables

    while (bRunning)
    {

        /* Wait up to 500 milliseconds */
        tv.tv_sec = 0;
        tv.tv_usec = LOOP_TIMEOUT;

        n = select(0, NULL, NULL, NULL, &tv);
        if (n < 0)
        {
            /* interrupted by signal or something, continue */
            continue;
        }

        //Get thread specific data
        wanIf = ( WanInterfaceData_t *) pthread_getspecific( sm_private_key );

        // process state
        switch (currentSmState)
        {

            case WAN_STATE_CONFIGURING_WAN:
                {
                    currentSmState = wan_state_configuring_wan(wanIf);
                    break;
                }
            case WAN_STATE_VALIDATING_WAN:
                {
                    currentSmState = wan_state_validating_wan(wanIf);
                    break;
                }
            case WAN_STATE_OBTAINING_IP_ADDRESSES:
                {
                    currentSmState = wan_state_obtaining_ip_addresses(wanIf);
                    break;
                }
            case WAN_STATE_IPV4_LEASED:
                {
                    currentSmState = wan_state_ipv4_leased(wanIf);
                    break;
                }
            case WAN_STATE_IPV6_LEASED:
                {
                    currentSmState = wan_state_ipv6_leased(wanIf);
                    break;
                }
            case WAN_STATE_DUAL_STACK_ACTIVE:
                {
                    currentSmState = wan_state_dual_stack_active(wanIf);
                    break;
                }
            case WAN_STATE_IPV4_OVER_IPV6_ACTIVE:
                {
                    currentSmState = wan_state_ipv4_over_ipv6_active(wanIf);
                    break;
                }
            case WAN_STATE_REFRESHING_WAN:
                {
                    currentSmState = wan_state_refreshing_wan(wanIf);
                    break;
                }
            case WAN_STATE_DECONFIGURING_WAN:
                {
                    currentSmState = wan_state_deconfiguring_wan(wanIf);
                    break;
                }
            case WAN_STATE_EXIT :
            default:
            {
                //Free current private resource before exit
                if( NULL != wanIf )
                {
                    free(wanIf);
                    wanIf = NULL;

                }
                bRunning = false;
                CcspTraceInfo(("%s %d - Exit from state machine\n", __FUNCTION__, __LINE__));
                break;
            }
        }
    }
    pthread_exit(NULL);
}

static eWanState_t wan_state_configuring_wan(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));
    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    if (wan_enable == FALSE ||
        stGlobalInfo.CfgStatus == WAN_IFACE_STATUS_DISABLED ||
        stGlobalInfo.CfgPhyStatus ==  WAN_IFACE_PHY_STATUS_DOWN ||
        stGlobalInfo.CfgLinkStatus ==  WAN_IFACE_LINKSTATUS_DOWN )
    {
        return wan_transition_physical_interface_down(wanIf);
    }

    if ( stGlobalInfo.CfgLinkStatus ==  WAN_IFACE_LINKSTATUS_UP )
    {
        return wan_transition_wan_up(wanIf);
    }

    return WAN_STATE_CONFIGURING_WAN;
}

static eWanState_t wan_state_validating_wan(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    if (wan_enable == FALSE ||
        stGlobalInfo.CfgStatus == WAN_IFACE_STATUS_DISABLED ||
        stGlobalInfo.CfgPhyStatus ==  WAN_IFACE_PHY_STATUS_DOWN ||
        stGlobalInfo.CfgLinkStatus ==  WAN_IFACE_LINKSTATUS_DOWN )
    {
        return wan_transition_physical_interface_down(wanIf);
    }

    if ( stGlobalInfo.CfgLinkStatus ==  WAN_IFACE_LINKSTATUS_CONFIGURING )
    {
        /* TODO: We'll need to call a transition that stops any running validation
        processes before returning to the CONFIGURING WAN state */
        return WAN_STATE_CONFIGURING_WAN;
    }

    /* TODO: Waits for every running validation process to complete, then checks the results */

    return wan_transition_wan_validated(wanIf);
}

static eWanState_t wan_state_obtaining_ip_addresses(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    if (wan_enable == FALSE ||
        stGlobalInfo.CfgStatus == WAN_IFACE_STATUS_DISABLED ||
        stGlobalInfo.CfgPhyStatus == WAN_IFACE_PHY_STATUS_DOWN ||
        stGlobalInfo.CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN )
    {
        return wan_transition_physical_interface_down(wanIf);
    }

    if ( stGlobalInfo.CfgLinkStatus ==  WAN_IFACE_LINKSTATUS_CONFIGURING ||
         stGlobalInfo.CfgRefresh == TRUE)
    {
        return wan_state_refreshing_wan(wanIf);
    }

    if (wanData.isIPv4Up == TRUE)
    {
        return wan_transition_ipv4_up(wanIf);
    }

    if (wanData.isIPv6Up == TRUE)
    {
        if(wanData.isIPv6ConfigChanged == TRUE)
        {
            /* Set sysevents to trigger P&M */
            if (setUpLanPrefixIPv6(&wanData) != RETURN_OK)
            {
                CcspTraceError((" %s %d - Failed to configure IPv6 prefix \n", __FUNCTION__, __LINE__));
            }

            /* Reset isIPv6ConfigChanged  */
            WanManager_UpdateGlobalWanData(IPV6_CONFIG_CHANGED, FALSE);
            return WAN_STATE_IPV4_LEASED;
        }

        if (checkIpv6AddressAssignedToBridge() == RETURN_OK)
        {
            return wan_transition_ipv6_up(wanIf);
        }
    }

    return WAN_STATE_OBTAINING_IP_ADDRESSES;
}

static eWanState_t wan_state_ipv4_leased(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
    */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    /* TODO: Disabled CfgActiveLinkChanged event for now
    as this event is getting triggering when the link first
    comes up. This issue need to be fixed.
    if (stGlobalInfo.CfgActiveLinkChanged == TRUE)
    {
        DmlWanIfResetCfgActiveLinkChanged(wanIf->baseIfName);
        WanManager_UpdateGlobalWanData(IPV4_CONFIG_CHANGED, TRUE);
        return wan_transition_ipv4_down(wanIf);
    }*/

    if( wan_enable == FALSE ||
        wanData.isIPv4Up == FALSE ||
        wanData.isIPv4ConfigChanged == TRUE ||
        stGlobalInfo.CfgPhyStatus == WAN_IFACE_PHY_STATUS_DOWN ||
        stGlobalInfo.CfgStatus == WAN_IFACE_STATUS_DISABLED ||
        stGlobalInfo.CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN )
    {
        return wan_transition_ipv4_down(wanIf);
    }
    else if ((wanData.isIPv6Up == TRUE))
    {
        if (checkIpv6AddressAssignedToBridge() == RETURN_OK)
        {
            return wan_transition_ipv6_up(wanIf);
        }
    }

    return WAN_STATE_IPV4_LEASED;
}

static eWanState_t wan_state_ipv6_leased(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    /* TODO: Disabled CfgActiveLinkChanged event for now
    as this event is getting triggering when the link first
    comes up. This issue need to be fixed.
    if (stGlobalInfo.CfgActiveLinkChanged == TRUE)
    {
        DmlWanIfResetCfgActiveLinkChanged(wanIf->baseIfName);
        WanManager_UpdateGlobalWanData(IPV6_CONFIG_CHANGED, TRUE);
        return wan_transition_ipv6_down(wanIf);
    } */

    if( wanData.isIPv6Up == FALSE ||
        wanData.isIPv6ConfigChanged == TRUE ||
        wan_enable == FALSE ||
        stGlobalInfo.CfgPhyStatus == WAN_IFACE_PHY_STATUS_DOWN ||
        stGlobalInfo.CfgStatus == WAN_IFACE_STATUS_DISABLED ||
        stGlobalInfo.CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN )
    {
        return wan_transition_ipv6_down(wanIf);
    }
    else if (wanData.isIPv4Up == TRUE)
    {
        return wan_transition_ipv4_up(wanIf);
    }
    else if (stGlobalInfo.CfgEnableDSLite == TRUE && stGlobalInfo.CfgActiveLink == TRUE && wanData.isDSLiteUp == TRUE)
    {
        return wan_transition_ipv4_over_ipv6_up(wanIf);
    }
    else if (stGlobalInfo.CfgEnableMAPT == TRUE && stGlobalInfo.CfgActiveLink == TRUE && wanData.isMAPTUp == TRUE)
    {
        return wan_transition_ipv4_over_ipv6_up(wanIf);
    }

    return WAN_STATE_IPV6_LEASED;
}

static eWanState_t wan_state_dual_stack_active(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    /*if (stGlobalInfo.CfgActiveLinkChanged == TRUE)
    {
        DmlWanIfResetCfgActiveLinkChanged(wanIf->baseIfName);
        WanManager_UpdateGlobalWanData(IPV6_CONFIG_CHANGED, TRUE);
        return wan_transition_dual_stack_down(wanIf);
    }*/

    if( wan_enable == FALSE ||
        stGlobalInfo.CfgPhyStatus == WAN_IFACE_PHY_STATUS_DOWN ||
        stGlobalInfo.CfgStatus == WAN_IFACE_STATUS_DISABLED ||
        stGlobalInfo.CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN ||
        (wanData.isIPv4ConfigChanged == TRUE && wanData.isIPv6ConfigChanged == TRUE))
    {
        return wan_transition_dual_stack_down(wanIf);
    }
    else if (wanData.isIPv4Up == FALSE || wanData.isIPv4ConfigChanged == TRUE)
    {
        /* TODO: Add IPoE Health Check failed for IPv4 here */
        return wan_transition_ipv4_down(wanIf);
    }
    else if (wanData.isIPv6Up == FALSE || wanData.isIPv6ConfigChanged == TRUE)
    {
        /* TODO: Add IPoE Health Check failed for IPv6 here */
        return wan_transition_ipv6_down(wanIf);
    }
    else if (stGlobalInfo.CfgEnableDSLite == TRUE && stGlobalInfo.CfgActiveLink == TRUE && wanData.isDSLiteUp == TRUE)
    {
        return wan_transition_ipv4_over_ipv6_up(wanIf);
    }
    else if (stGlobalInfo.CfgEnableMAPT == TRUE && stGlobalInfo.CfgActiveLink == TRUE && wanData.isMAPTUp == TRUE)
    {
        return wan_transition_ipv4_over_ipv6_up(wanIf);
    }

    return WAN_STATE_DUAL_STACK_ACTIVE;
}
static eWanState_t wan_state_ipv4_over_ipv6_active(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    if( wanData.isIPv6Up == FALSE ||
        wanData.isIPv6ConfigChanged == TRUE ||
        wan_enable == FALSE ||
        stGlobalInfo.CfgActiveLink != TRUE ||
        stGlobalInfo.CfgPhyStatus == WAN_IFACE_PHY_STATUS_DOWN ||
        stGlobalInfo.CfgStatus == WAN_IFACE_STATUS_DISABLED ||
        stGlobalInfo.CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN )
    {
        return wan_transition_ipv4_over_ipv6_down(wanIf);
    }
    else if (stGlobalInfo.CfgEnableDSLite == FALSE ||
             wanData.isDSLiteUp == FALSE ||
             wanData.isDSLiteConfigChanged == TRUE )
    {
        return wan_transition_ipv4_over_ipv6_down(wanIf);
    }
    else if (stGlobalInfo.CfgEnableMAPT == FALSE ||
             wanData.isMAPTUp == FALSE ||
             wanData.isMAPTConfigChanged == TRUE)
    {
        return wan_transition_ipv4_over_ipv6_down(wanIf);
    }

    return WAN_STATE_IPV4_OVER_IPV6_ACTIVE;
}

static eWanState_t wan_state_refreshing_wan(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    if( stGlobalInfo.CfgPhyStatus == WAN_IFACE_PHY_STATUS_DOWN ||
        stGlobalInfo.CfgStatus == WAN_IFACE_STATUS_DISABLED ||
        stGlobalInfo.CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN ||
        wan_enable == FALSE )
    {
        return wan_transition_physical_interface_down(wanIf);
    }
    else if (stGlobalInfo.CfgLinkStatus == WAN_IFACE_LINKSTATUS_UP && stGlobalInfo.CfgRefresh == TRUE)
    {
        return wan_transition_refreshing_wan(wanIf);
    }
    else if (stGlobalInfo.CfgLinkStatus == WAN_IFACE_LINKSTATUS_UP && stGlobalInfo.CfgRefresh == FALSE)
    {
        return wan_transition_wan_refreshed(wanIf);
    }

    return WAN_STATE_REFRESHING_WAN;
}

static eWanState_t wan_state_deconfiguring_wan(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);

    if ( stGlobalInfo.CfgLinkStatus == WAN_IFACE_LINKSTATUS_DOWN )
    {
        return wan_transition_exit(wanIf);
    }

    return WAN_STATE_DECONFIGURING_WAN;
}

static eWanState_t wan_transition_start(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    WanData_t wanData;
    BOOL wan_enable = false;

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    WanManagerInitInterfaceData(&gWanData, wanIf->ifName);

    DmlWanIfSetIPState(stGlobalInfo.CfgWanName, WAN_IFACE_IPV4_STATE, WAN_IFACE_IP_STATE_DOWN);
    DmlWanIfSetIPState(stGlobalInfo.CfgWanName, WAN_IFACE_IPV6_STATE, WAN_IFACE_IP_STATE_DOWN);
    DmlWanIfSetIPState(stGlobalInfo.CfgWanName, WAN_IFACE_MAPT_STATE, WAN_IFACE_IP_STATE_DOWN);
    DmlWanIfSetIPState(stGlobalInfo.CfgWanName, WAN_IFACE_DSLITE_STATE, WAN_IFACE_IP_STATE_DOWN);

    DmlWanIfUpdateLinkStatusForGivenIfName(stGlobalInfo.CfgBaseifName, WAN_IFACE_LINKSTATUS_CONFIGURING);
    DmlWanIfUpdateWanStatusForGivenIfName(stGlobalInfo.CfgBaseifName, WAN_IFACE_STATUS_INITIALISING);

    WanManager_updateWanInterfaceUpstreamFlag(stGlobalInfo.CfgPhyPath, TRUE);

#ifdef _HUB4_PRODUCT_REQ_
    if (stGlobalInfo.CfgActiveLink == TRUE) 
    {
        const char if_dsl_name[] = "dsl";
        if (strncmp(stGlobalInfo.CfgBaseifName, if_dsl_name, strlen(if_dsl_name)) == 0)
        {
            wanmgr_setLedState(FLASHING_AMBER);
        }
        else    
        {
            wanmgr_setLedState(SOLID_AMBER);
        }        
    }
#endif

    /* TODO: Need to handle crash recovery */
    return WAN_STATE_CONFIGURING_WAN;
}

static eWanState_t wan_transition_physical_interface_down(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    WanData_t wanData;
    BOOL wan_enable = FALSE;
    int ret = RETURN_OK;

    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    DmlWanIfUpdateWanStatusForGivenIfName(stGlobalInfo.CfgBaseifName, WAN_IFACE_STATUS_DISABLED);

    if(stGlobalInfo.CfgEnablePPP == FALSE)
    {
        /* Stops DHCPv4 client */
        WanManager_StopDhcpv4Client(TRUE); // release dhcp lease

        /* Stops DHCPv6 client */
        WanManager_StopDhcpv6Client(TRUE); // release dhcp lease
    }

    /* TODO: If Wan.EnablePPPoE is set to TRUE, the PPP Manager will be informed to terminate
    its IPCP and IPv6CP protocols */

    WanManager_updateWanInterfaceUpstreamFlag(stGlobalInfo.CfgPhyPath, FALSE);

    return WAN_STATE_DECONFIGURING_WAN;
}

static eWanState_t wan_transition_wan_up(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    DmlWanIfUpdateWanStatusForGivenIfName(stGlobalInfo.CfgBaseifName, WAN_IFACE_STATUS_VALIDATING);

    /* TODO: Runs WAN Validation processes based on the Wan.Validation flags,
    e.g. if Wan.Validation.Discovery-Offer is set to TRUE, a threaded
    process will be started to run the DHCPv4 Discovery-Offer validation.
    The results of each validation process will be stored internally
    to the state machine (i.e. not expressed in the data model */

#ifdef _HUB4_PRODUCT_REQ_
    if (stGlobalInfo.CfgActiveLink == TRUE) 
    {
        wanmgr_setLedState(SOLID_AMBER);
    }
#endif

    return WAN_STATE_VALIDATING_WAN;
}

static eWanState_t wan_transition_wan_validated(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan interface data.
    */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    /**
     * Get global copy of wan configuration DML data from DML layer.
    */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);

    DmlWanIfUpdateWanStatusForGivenIfName(stGlobalInfo.CfgBaseifName, WAN_IFACE_STATUS_UP);

    if( stGlobalInfo.CfgEnablePPP == FALSE )
    {
        /* Start DHCPv4 client */
        /* Force reset ipv4 state global flag. */
        WanManager_UpdateGlobalWanData(IPV4_STATE_UP, FALSE);
        CcspTraceInfo(("%s %d - Staring udhcpc on interface %s \n", __FUNCTION__, __LINE__, wanIf->ifName));
        uint32_t pid = WanManager_StartDhcpv4Client(wanIf->ifName, FALSE);
        CcspTraceInfo(("%s %d - Started udhcpc on interface %s, pid %d \n", __FUNCTION__, __LINE__, wanIf->ifName, pid));

        /* Start DHCPv6 Client */
        /* Force reset ipv6 state global flag. */
        WanManager_UpdateGlobalWanData(IPV6_STATE_UP, FALSE);
        CcspTraceInfo(("%s %d - Staring dibbler-client on interface %s \n", __FUNCTION__, __LINE__, wanIf->ifName));
        if (RETURN_OK != WanManager_StartDhcpv6Client(wanIf->ifName, FALSE))
        {
            CcspTraceError(("%s %d - Failed to start DHCPv6 client on %s \n", __FUNCTION__, __LINE__, wanIf->ifName));
        }
        CcspTraceInfo(("%s %d - Started dibbler-client on interface %s \n", __FUNCTION__, __LINE__, wanIf->ifName));
    }

    /* TODO: If Wan.EnablePPP is set to TRUE, DHCP clients will not be started,
    and instead the PPP Manager will be informed to begin its IPCP and IPv6CP
    protocols to respectively obtain IPv4 and IPv6 addresses */

    return WAN_STATE_OBTAINING_IP_ADDRESSES;
}

static eWanState_t wan_transition_refreshing_wan(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    ANSC_STATUS retStatus;
    BOOL wan_enable = FALSE;
    int wanIndex = -1;
    WanData_t wanData;

    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    retStatus = DmlWanIfGetIndexFromIfName(wanIf->ifName, &wanIndex);
    if ( (ANSC_STATUS_FAILURE == retStatus ) || ( -1 == wanIndex ) )
    {
        CcspTraceInfo(("%s Failed to get index for %s\n", __FUNCTION__, wanIf->ifName));
        return ANSC_STATUS_FAILURE;
    }

    DmlWanIfUpdateRefreshFlagForGivenIfName(stGlobalInfo.CfgBaseifName, FALSE);
    DmlWanIfUpdateLinkStatusForGivenIfName(stGlobalInfo.CfgBaseifName, WAN_IFACE_LINKSTATUS_CONFIGURING);

    if( stGlobalInfo.CfgEnablePPP == FALSE )
    {
        /* Stops DHCPv4 client */
        WanManager_StopDhcpv4Client(TRUE); // release dhcp lease

        /* Stops DHCPv6 client */
        WanManager_StopDhcpv6Client(TRUE); // release dhcp lease
    }

    /*TODO: If Wan.EnablePPP is set to TRUE, the PPP Manager will be informed to
    terminate its IPCP and IPv6CP protocols */

    /* Sets Ethernet.Link.{i}.X_RDK_Refresh to TRUE in VLAN & Bridging Manager
    in order to refresh the WAN link */
    PDATAMODEL_WAN_IFACE pMyObject = (PDATAMODEL_WAN_IFACE)g_pBEManager->hWanIface;
    PDML_WAN_IFACE p_Interface = pMyObject->pWanIface;
    DmlWanIfSetWanRefresh( p_Interface + wanIndex);

    return WAN_STATE_REFRESHING_WAN;
}

static eWanState_t wan_transition_wan_refreshed(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    if( stGlobalInfo.CfgEnablePPP == FALSE )
    {
        WanManager_UpdateGlobalWanData(IPV4_STATE_UP, FALSE);
        /* Start dhcp clients */
        /* DHCPv4 client */
        CcspTraceInfo(("%s %d - Staring dhcpc on interface %s \n", __FUNCTION__, __LINE__, wanIf->ifName));
        uint32_t pid = WanManager_StartDhcpv4Client(wanIf->ifName, FALSE);
        CcspTraceInfo(("%s %d - Started dhcpc on interface %s, pid %d \n", __FUNCTION__, __LINE__, wanIf->ifName, pid));

        WanManager_UpdateGlobalWanData(IPV6_STATE_UP, FALSE);
        /* DHCPv6 Client */
        if (RETURN_OK != WanManager_StartDhcpv6Client(wanIf->ifName, FALSE))
        {
            CcspTraceError(("%s %d - Failed to start DHCPv6 client on %s \n", __FUNCTION__, __LINE__, wanIf->ifName ));
        }
    }

    /* TODO: If Wan.EnablePPPoE is set to TRUE, DHCP clients will not be started,
    and instead the PPP Manager will be informed to begin its IPCP and IPv6CP
    protocols to respectively obtain IPv4 and IPv6 addresses */


    return WAN_STATE_OBTAINING_IP_ADDRESSES;
}

static eWanState_t wan_transition_ipv4_up(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    int ret = RETURN_OK;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    if(stGlobalInfo.CfgActiveLink == TRUE )
    {
        /* Configure IPv4. */
        ret = wan_setUpIPv4(wanIf->ifName, &wanData);
        if (ret != RETURN_OK)
        {
            CcspTraceError(("%s %d - Failed to configure IPv4 successfully \n", __FUNCTION__, __LINE__));
        }
    }

    /* Force reset ipv4 state global flag. */
    WanManager_UpdateGlobalWanData(IPV4_CONFIG_CHANGED, FALSE);

    /* TODO: If Wan.EnableIPoEHealthCheck is set to TRUE, the IPoE Health Check
    application will be started for IPv4
    if(stGlobalInfo.CfgEnableIPoEHealthCheck == TRUE )
    {
        // Starts IPoE healthcheck for IPv4
    }
    */

#ifdef _HUB4_PRODUCT_REQ_
    if (stGlobalInfo.CfgActiveLink == TRUE) 
    {
        wanmgr_setLedState(SOLID_GREEN);
    }
#endif

    DmlWanIfSetIPState(stGlobalInfo.CfgWanName, WAN_IFACE_IPV4_STATE, WAN_IFACE_IP_STATE_UP);

    if( stGlobalInfo.CfgIpv6Status == WAN_IFACE_IPV6_STATE_UP )
        return WAN_STATE_DUAL_STACK_ACTIVE;

    return WAN_STATE_IPV4_LEASED;
}

static eWanState_t wan_transition_ipv4_down(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    int ret = RETURN_OK;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    if ((stGlobalInfo.CfgLinkStatus != WAN_IFACE_LINKSTATUS_UP) ||
        (stGlobalInfo.CfgPhyStatus == WAN_IFACE_PHY_STATUS_DOWN))
    {
        WanManager_UpdateGlobalWanData(IPV4_STATE_UP, FALSE);
    }
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    if (wan_tearDownIPv4(wanIf->ifName, &wanData) != RETURN_OK)
    {
        CcspTraceError(("%s %d - Failed to tear down IPv4 for %s \n", __FUNCTION__, __LINE__, wanIf->ifName));
    }

    /* TODO: If Wan.EnableIPoEHealthCheck is set to TRUE, stop
    the IPoE Health Check application for IPv4 */
    /*if(stGlobalInfo.CfgEnableIPoEHealthCheck == TRUE)
    {
    }*/

    DmlWanIfSetIPState(stGlobalInfo.CfgWanName, WAN_IFACE_IPV4_STATE, WAN_IFACE_IP_STATE_DOWN);

#ifdef _HUB4_PRODUCT_REQ_
    if (stGlobalInfo.CfgActiveLink == TRUE) 
    {
        wanmgr_setLedState(SOLID_AMBER);
    }
#endif

    if( stGlobalInfo.CfgIpv6Status == WAN_IFACE_IPV6_STATE_UP )
    {
        return WAN_STATE_IPV6_LEASED;
    }

    return WAN_STATE_OBTAINING_IP_ADDRESSES;
}

static eWanState_t wan_transition_ipv6_up(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    int ret = RETURN_OK;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    if(stGlobalInfo.CfgActiveLink == TRUE )
    {
        /* Configure IPv4. */
        ret = wan_setUpIPv6(wanIf->ifName, &wanData);
        if (ret != RETURN_OK)
        {
            CcspTraceError(("%s %d - Failed to configure IPv6 successfully \n", __FUNCTION__, __LINE__));
        }
    }

    WanManager_UpdateGlobalWanData(IPV6_CONFIG_CHANGED, FALSE);

    /* TODO: If Wan.EnableIPoEHealthCheck is set to TRUE, the IPoE Health Check
    application will be started for IPv6
    if(stGlobalInfo.CfgEnableIPoEHealthCheck == TRUE )
    {
        // Starts IPoE healthcheck for IPv6
    }
    */

    DmlWanIfSetIPState(stGlobalInfo.CfgWanName, WAN_IFACE_IPV6_STATE, WAN_IFACE_IP_STATE_UP);

    if( stGlobalInfo.CfgIpv4Status == WAN_IFACE_IPV4_STATE_UP )
    {
#ifdef _HUB4_PRODUCT_REQ_
        if (stGlobalInfo.CfgActiveLink == TRUE) 
        {
            wanmgr_setLedState(SOLID_GREEN);
        }
#endif
        return WAN_STATE_DUAL_STACK_ACTIVE;
    }
#ifdef _HUB4_PRODUCT_REQ_
    if (stGlobalInfo.CfgActiveLink == TRUE) 
    {
        wanmgr_setLedState(SOLID_AMBER);
    }
#endif
    return WAN_STATE_IPV6_LEASED;
}

static eWanState_t wan_transition_ipv6_down(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    int ret = RETURN_OK;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    if ((stGlobalInfo.CfgLinkStatus != WAN_IFACE_LINKSTATUS_UP) ||
        (stGlobalInfo.CfgPhyStatus == WAN_IFACE_PHY_STATUS_DOWN))
    {
        WanManager_UpdateGlobalWanData(IPV6_STATE_UP, FALSE);
    }

    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    if (wan_tearDownIPv6(wanIf->ifName, &wanData) != RETURN_OK)
    {
        CcspTraceError(("%s %d - Failed to tear down IPv6 for %s \n", __FUNCTION__, __LINE__, wanIf->ifName));
    }

    /* TODO: If Wan.EnableIPoEHealthCheck is set to TRUE, stop
    the IPoE Health Check application for IPv6 */
    /*if(stGlobalInfo.CfgEnableIPoEHealthCheck == TRUE)
    {
    }*/

    DmlWanIfSetIPState(stGlobalInfo.CfgWanName, WAN_IFACE_IPV6_STATE, WAN_IFACE_IP_STATE_DOWN);

    if( stGlobalInfo.CfgIpv4Status == WAN_IFACE_IPV4_STATE_UP )
    {
#ifdef _HUB4_PRODUCT_REQ_
        if (stGlobalInfo.CfgActiveLink == TRUE) 
        {
            wanmgr_setLedState(SOLID_GREEN);
        }
#endif
        return WAN_STATE_IPV4_LEASED;
    }
#ifdef _HUB4_PRODUCT_REQ_
    if (stGlobalInfo.CfgActiveLink == TRUE) 
    {
        wanmgr_setLedState(SOLID_AMBER);
    }
#endif

    return WAN_STATE_OBTAINING_IP_ADDRESSES;

}

static eWanState_t wan_transition_dual_stack_down(WanInterfaceData_t *wanIf)
{
    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));
    wan_transition_ipv4_down(wanIf);
    wan_transition_ipv6_down(wanIf);
    return WAN_STATE_OBTAINING_IP_ADDRESSES;
}

static eWanState_t wan_transition_ipv4_over_ipv6_up(WanInterfaceData_t *wanIf)
{
    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));
    /* TODO: Handle DSLite and MAP-T UP cases */
    return WAN_STATE_IPV4_LEASED;
}

static eWanState_t wan_transition_ipv4_over_ipv6_down(WanInterfaceData_t *wanIf)
{
    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));
    /* TODO: Handle DSLite and MAP-T DOWN cases */
    return WAN_STATE_IPV4_LEASED;
}

static eWanState_t wan_transition_exit(WanInterfaceData_t *wanIf)
{
    DML_WAN_IFACE_GLOBAL_CONFIG stGlobalInfo;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    CcspTraceInfo(("%s %d - Enter \n", __FUNCTION__, __LINE__));

    memset(&stGlobalInfo, 0, sizeof(stGlobalInfo));
    memset(&wanData, 0, sizeof(wanData));

    /**
     * Get global copy of wan configuration DML data from DML layer.
     */
    DmlWanIfGetCopyOfGlobalInfoForGivenBaseIfName(wanIf->baseIfName, &stGlobalInfo);
    /**
     * Get global copy of wan interface data.
     */
    WanManager_GetCopyofGlobalWanData(wanIf->ifName, &wan_enable, &wanData);

    DmlWanIfUpdateWanStatusForGivenIfName(stGlobalInfo.CfgBaseifName, WAN_IFACE_STATUS_DISABLED);
    DmlWanIfUpdateRefreshFlagForGivenIfName(stGlobalInfo.CfgBaseifName, FALSE);

#ifdef _HUB4_PRODUCT_REQ_
    if (stGlobalInfo.CfgActiveLink == TRUE) 
    {
        wanmgr_setLedState(OFF);
    }
#endif
    DmlWanIfUpdateActiveLinkFlagForGivenIfName(stGlobalInfo.CfgBaseifName, FALSE);

    CcspTraceInfo(("%s %d - WAN state machine stopped\n", __FUNCTION__, __LINE__));
    return WAN_STATE_EXIT;
}

static int wan_setUpIPv4(const char *wanIfName, WanData_t* wanData)
{
    int ret = RETURN_OK;
    char cmdStr[BUFLEN_128 + IP_ADDR_LENGTH] = {0};
    char bCastStr[IP_ADDR_LENGTH] = {0};

    if (wanIfName == NULL || wanData == NULL)
    {
        CcspTraceError(("%s %d - Invalid memory \n", __FUNCTION__, __LINE__));
        return RETURN_ERR;
    }
    if (RETURN_OK == wan_updateDNS(wanIfName, TRUE, wanData->isIPv6Up))
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
    if (WanManager_GetBCastFromIpSubnetMask(wanData->ipv4Data.ip, wanData->ipv4Data.mask, bCastStr) != RETURN_OK)
    {
        CcspTraceError((" %s %d - bad address %s/%s \n",__FUNCTION__,__LINE__,wanData->ipv4Data.ip, wanData->ipv4Data.mask));
        return RETURN_ERR;
    }

    snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s %s netmask %s broadcast %s",
             wanData->ipv4Data.dhcpcInterface, wanData->ipv4Data.ip, wanData->ipv4Data.mask, bCastStr);
    CcspTraceInfo(("%s %d -  IP configuration = %s \n", __FUNCTION__, __LINE__, cmdStr));
    WanManager_DoSystemAction("setupIPv4:", cmdStr);

    snprintf(cmdStr, sizeof(cmdStr), "sendarp -s %s -d %s", ETH_BRIDGE_NAME, ETH_BRIDGE_NAME);
    WanManager_DoSystemAction("setupIPv4", cmdStr);

    /** Set default gatway. */
    if (WanManager_AddDefaultGatewayRoute(&wanData->ipv4Data) != RETURN_OK)
    {
        CcspTraceError(("%s %d - Failed to set up default system gateway", __FUNCTION__, __LINE__));
    }

    /** Update required sysevents. */
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_CONNECTION_STATE, UP, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_CURRENT_WAN_IPADDR, wanData->ipv4Data.ip, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_CURRENT_WAN_SUBNET, wanData->ipv4Data.mask, 0);

    if (wanData->isIPv6Up == FALSE)
    {
        int  uptime = 0;
        char buffer[64] = {0};

        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_WAN_STATUS, WAN_STATUS_STARTED, 0);
        CcspTraceInfo(("%s %d - wan_transition_ipv4Up: wan-status event set to started \n", __FUNCTION__, __LINE__));

        //Get WAN uptime
        WanManager_GetDateAndUptime( buffer, &uptime );
        LOG_CONSOLE("%s Wan_init_complete:%d\n",buffer,uptime);

        system("print_uptime \"boot_to_wan_uptime\"");
    }

    /* Firewall restart. */
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIREWALL_RESTART, NULL, 0);
    return ret;
}

static int wan_tearDownIPv4(const char *wanIfName, WanData_t* wanData)
{
    int ret = RETURN_OK;
    char cmdStr[BUFLEN_64] = {0};

    if (wanIfName == NULL || wanData == NULL)
    {
        CcspTraceError(("%s %d - Invalid memory \n", __FUNCTION__, __LINE__));
        return RETURN_ERR;
    }

    /** Reset IPv4 DNS configuration. */
    if (RETURN_OK == wan_updateDNS(wanIfName, FALSE, wanData->isIPv6Up))
    {
        CcspTraceInfo(("%s %d -  IPv4 DNS servers unconfig successfully \n", __FUNCTION__, __LINE__));
        /* DHCP restart trigger. */
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_DHCP_SERVER_RESTART, NULL, 0);
    }
    else
    {
        CcspTraceError(("%s %d - Failed to unconfig IPv4 DNS servers \n", __FUNCTION__, __LINE__));
    }

    /* Need to remove the network from the routing table by
    * doing "ifconfig L3IfName 0.0.0.0"
    * wanData->ipv4Data.dhcpcInterface is Empty.
    */
    snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s 0.0.0.0", wanIfName);
    if (WanManager_DoSystemActionWithStatus("wan_tearDownIPv4: ifconfig L3IfName 0.0.0.0", (cmdStr)) != 0)
    {
        CcspTraceError(("%s %d - failed to run cmd: %s", __FUNCTION__, __LINE__, cmdStr));
        ret = RETURN_ERR;
    }

    /* ReSet the required sysevents. */
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV4_CONNECTION_STATE, DOWN, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_CURRENT_WAN_IPADDR, "0.0.0.0", 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_CURRENT_WAN_SUBNET, "255.255.255.0", 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIREWALL_RESTART, NULL, 0);

    return ret;
}

static int wan_setUpIPv6(const char *ifname, WanData_t* wanData)
{

    int ret = RETURN_OK;

    if (ifname == NULL || wanData == NULL)
    {
        CcspTraceError(("%s %d - Invalid memory \n", __FUNCTION__, __LINE__));
        return RETURN_ERR;
    }

    /** Reset IPv4 DNS configuration. */
    if (RETURN_OK == wan_updateDNS(ifname, wanData->isIPv4Up, TRUE))
    {
        CcspTraceInfo(("%s %d -  IPv6 DNS servers configured successfully \n", __FUNCTION__, __LINE__));
        /* DHCP restart trigger. */
        sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_DHCP_SERVER_RESTART, NULL, 0);
    }
    else
    {
        CcspTraceError(("%s %d - Failed to configure IPv6 DNS servers \n", __FUNCTION__, __LINE__));
    }


    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV6_CONNECTION_STATE, UP, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_RADVD_RESTART, NULL, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_DHCP_SERVER_RESTART, NULL, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIREWALL_RESTART, NULL, 0);

    return ret;
}

static int wan_tearDownIPv6(const char *ifname, WanData_t* wanData)
{

    int ret = RETURN_OK;

    if (ifname == NULL || wanData == NULL)
    {
        CcspTraceError(("%s %d - Invalid memory \n", __FUNCTION__, __LINE__));
        return RETURN_ERR;
    }

    /** Reset IPv4 DNS configuration. */
    if (RETURN_OK == wan_updateDNS(ifname, wanData->isIPv4Up, FALSE))
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
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_IPV6_CONNECTION_STATE, DOWN, 0);
    sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIREWALL_RESTART, NULL, 0);

    return ret;
}
#ifdef FEATURE_MAPT
static int wan_setUpMapt(const char *ifName)
{
    int ret = RETURN_OK;

    if (ifName == NULL)
    {
        CcspTraceError(("%s %d - Invalid memory \n", __FUNCTION__, __LINE__));
        return ret;
    }

    /* Disable flow cache. */
    if (WanManager_DoSystemActionWithStatus("wanmanager", "fcctl disable") != RETURN_OK)
    {
        CcspTraceError(("%s %d - wanmanager: Failed to disable packet accelaration \n ", __FUNCTION__, __LINE__));
    }

    if (WanManager_DoSystemActionWithStatus("wanmanager", "fcctl flush") != RETURN_OK)
    {
        CcspTraceError(("%s %d - wanmanager: Failed to flush the cache \n", __FUNCTION__, __LINE__));
    }

    /* Disable runner. */
    if (WanManager_DoSystemActionWithStatus("wanmanager", "runner disable") != RETURN_OK)
    {
        CcspTraceError(("%s %d - wanmanager: Failed to disable the runner \n", __FUNCTION__, __LINE__));
    }

    if (WanManager_DoSystemActionWithStatus("wanmanager", "insmod /lib/modules/`uname -r`/extra/ivi.ko") != RETURN_OK)
    {
        CcspTraceError(("%s %d -insmod: Failed to add ivi.ko \n", __FUNCTION__, __LINE__));
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
        return ret;
    }

    file = popen("cat /proc/modules | grep ivi","r");

    if( file == NULL) {
        CcspTraceError(("[%s][%d]Failed to open  /proc/modules \n", __FUNCTION__, __LINE__));
    }
    else {
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
    }

    /* Enable runner. */
    if (WanManager_DoSystemActionWithStatus("wanmanager", "runner enable") != RETURN_OK)
    {
        CcspTraceError(("%s %d wanmanager: Failed to enable the runner \n ", __FUNCTION__, __LINE__));
    }

    return ret;
}
#endif

static int wan_updateDNS(const char *wanIfName, BOOL addIPv4, BOOL addIPv6)
{
    int ret = RETURN_OK;
    DnsData_t dnsData;
    WanData_t wanData;
    BOOL wan_enable = FALSE;

    if (NULL == wanIfName)
    {
        CcspTraceError(("%s %d - Invalid memory \n", __FUNCTION__, __LINE__));
        return RETURN_ERR;
    }

    memset(&dnsData, 0, sizeof(DnsData_t));
    memset(&wanData, 0, sizeof(WanData_t));
    WanManager_GetCopyofGlobalWanData(wanIfName, &wan_enable, &wanData);

    if (addIPv4)
    {
        strncpy(dnsData.dns_ipv4_1, wanData.ipv4Data.dnsServer, sizeof(dnsData.dns_ipv4_1));
        strncpy(dnsData.dns_ipv4_2, wanData.ipv4Data.dnsServer1, sizeof(dnsData.dns_ipv4_2));
    }

    if (addIPv6)
    {
        strncpy(dnsData.dns_ipv6_1, wanData.ipv6Data.nameserver, sizeof(dnsData.dns_ipv6_1));
        strncpy(dnsData.dns_ipv6_2, wanData.ipv6Data.nameserver1, sizeof(dnsData.dns_ipv6_2));
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

static int setUpLanPrefixIPv6(WanData_t* wanData)
{
    if (wanData == NULL)
    {
        CcspTraceError(("%s %d - Invalid memory \n", __FUNCTION__, __LINE__));
        return RETURN_ERR;
    }

    int index = strcspn(wanData->ipv6Data.sitePrefix, "/");
    if (index < strlen(wanData->ipv6Data.sitePrefix))
    {
        char lanPrefix[BUFLEN_48] = {0};
        strncpy(lanPrefix, wanData->ipv6Data.sitePrefix, index);
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
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_PREVIOUS_IPV6_PREFIXVLTIME, "", 0);
                sysevent_set(sysevent_fd, sysevent_token, SYSEVENT_FIELD_PREVIOUS_IPV6_PREFIXPLTIME, "", 0);
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

