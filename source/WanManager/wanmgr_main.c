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

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

/*********************************************************************************

    description:

        This is the template file of ssp_main.c for XxxxSsp.
        Please replace "XXXX" with your own ssp name with the same up/lower cases.

  ------------------------------------------------------------------------------

    revision:

        09/08/2011    initial revision.

**********************************************************************************/


#ifdef __GNUC__
#ifndef _BUILD_ANDROID
#include <execinfo.h>
#endif
#endif

#include <syscfg/syscfg.h>
#include "wanmgr_ssp_global.h"
#include "wanmgr_core.h"
#include "wanmgr_data.h"
#include "stdlib.h"
#include "ccsp_dm_api.h"

#include "webconfig_framework.h"
#if defined (_HUB4_PRODUCT_REQ_) || defined(_PLATFORM_RASPBERRYPI_)
#include "wanmgr_rbus_handler_apis.h"
#endif
#define DEBUG_INI_NAME "/etc/debug.ini"

#ifdef ENABLE_SD_NOTIFY
#include <systemd/sd-daemon.h>
#endif

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif
#include "cap.h"

cap_user appcaps;

extern char*                                pComponentName;
char                                        g_Subsystem[32]         = {0};

#if defined (FEATURE_RDKB_WAN_MANAGER)
#if !defined(AUTOWAN_ENABLE) && !defined(INTEL_PUMA7)// This is not needed when auto wan is enabled for TCXBX platforms
extern ANSC_HANDLE bus_handle;

#if defined(_HUB4_PRODUCT_REQ_) || defined(_PLATFORM_RASPBERRYPI_)
#define  ARRAY_SZ(x) (sizeof(x) / sizeof((x)[0]))

typedef struct
{
    char binaryLocation[64];  // binary path.
    char rbusName[64];        // their rbus name.
}Rbus_Module;

static void waitUntilSystemReady()
{
    int wait_time = 0;
    char pModule[1024] = {0};
    /* list of dependency modules should get registered before wanmgr start */
    Rbus_Module pModuleNames[] = {{"/usr/bin/PsmSsp",    "rbusPsmSsp"},
#if !defined(_PLATFORM_RASPBERRYPI_)
                               {"/usr/bin/VlanManager", "eRT.com.cisco.spvtg.ccsp.vlanmanager"},
                               {"/usr/bin/xdslmanager",   "eRT.com.cisco.spvtg.ccsp.xdslmanager"},
                               {"/usr/bin/pppmanager",    "eRT.com.cisco.spvtg.ccsp.pppmanager"},
                               {"/usr/bin/CcspCMAgentSsp","eRT.com.cisco.spvtg.ccsp.cm"},
                               {"/usr/bin/rdkledmanager", "eRT.com.cisco.spvtg.ccsp.ledmanager"},
#endif
                               {"/usr/bin/CcspEthAgent",  "RbusEthAgent"},
                               {"/usr/bin/CcspPandMSsp",  "CcspPandMSsp"}};

    int elementCnt = ARRAY_SZ(pModuleNames);

    /* prepare dependency module name list, if binary exists     */
    /* Example: pModule = "rbusPsmSsp RbusEthAgent CcspPandMSsp" */
    for(int i=0; i<elementCnt;i++)
    {
        if (util_isFilePresent(pModuleNames[i].binaryLocation))
        {
            strcat(pModule,pModuleNames[i].rbusName);
            strcat(pModule," ");
        }
    }

    /* Check System (CR / Rbus) is ready.*/
    while(wait_time <= 90)
    {
        /* check  dependency modules get registered */
        if(WanMgr_Rbus_discover_components(pModule)){
            /* all dependency modules registered - exit loop */
            break;
        }
        wait_time++;
        sleep(2);
    }
}
#endif //_HUB4_PRODUCT_REQ_ || _PLATFORM_RASPBERRYPI_
#endif
#endif //#if defined (FEATURE_RDKB_WAN_MANAGER)

int  cmd_dispatch(int  command)
{
    switch ( command )
    {
        case    'e' :

            CcspTraceInfo(("Connect to bus daemon...\n"));

            {
                char                            CName[256];

                if ( g_Subsystem[0] != 0 )
                {
                    _ansc_sprintf(CName, "%s%s", g_Subsystem, COMPONENT_ID_WANMANAGER);
                }
                else
                {
                    _ansc_sprintf(CName, "%s", COMPONENT_ID_WANMANAGER);
                }

                ssp_Mbi_MessageBusEngage
                    (
                        CName,
                        CCSP_MSG_BUS_CFG,
                        COMPONENT_PATH_WANMANAGER
                    );
            }

            ssp_create();
            ssp_engage();

            break;

        case    'm':

                AnscPrintComponentMemoryTable(pComponentName);

                break;

        case    't':

                AnscTraceMemoryTable();

                break;

        case    'c':

                ssp_cancel();

                break;

        default:
            break;
    }

    return 0;
}

static void _print_stack_backtrace(void)
{
#ifdef __GNUC__
#ifndef _BUILD_ANDROID
    void* tracePtrs[100];
    char** funcNames = NULL;
    int i, count = 0;

    count = backtrace( tracePtrs, 100 );
    backtrace_symbols_fd( tracePtrs, count, 2 );

    funcNames = backtrace_symbols( tracePtrs, count );

    if ( funcNames ) {
            // Print the stack trace
        for( i = 0; i < count; i++ )
        printf("%s\n", funcNames[i] );

            // Free the string pointers
            free( funcNames );
    }
#endif
#endif
}

static void daemonize(void) {
    int fd;
    switch (fork()) {
    case 0:
        break;
    case -1:
        // Error
        CcspTraceInfo(("Error daemonizing (fork)! %d - %s\n", errno, strerror(
                errno)));
        exit(0);
        break;
    default:
        _exit(0);
    }

    if (setsid() <     0) {
        CcspTraceInfo(("Error demonizing (setsid)! %d - %s\n", errno, strerror(errno)));
        exit(0);
    }

//    chdir("/");


#ifndef  _DEBUG

    fd = open("/dev/null", O_RDONLY);
    if (fd != 0) {
        dup2(fd, 0);
        close(fd);
    }
    fd = open("/dev/null", O_WRONLY);
    if (fd != 1) {
        dup2(fd, 1);
        close(fd);
    }
    fd = open("/dev/null", O_WRONLY);
    if (fd != 2) {
        dup2(fd, 2);
        close(fd);
    }
#endif
}

void sig_handler(int sig)
{
    if ( sig == SIGINT ) {
        signal(SIGINT, sig_handler); /* reset it to this function */
        CcspTraceInfo(("SIGINT received!\n"));
    exit(0);
    }
    else if ( sig == SIGUSR1 ) {
        signal(SIGUSR1, sig_handler); /* reset it to this function */
        CcspTraceInfo(("SIGUSR1 received!\n"));
    }
    else if ( sig == SIGUSR2 ) {
        CcspTraceInfo(("SIGUSR2 received!\n"));
    }
    else if ( sig == SIGCHLD ) {
        signal(SIGCHLD, sig_handler); /* reset it to this function */
        CcspTraceInfo(("SIGCHLD received!\n"));
    }
    else if ( sig == SIGPIPE ) {
        signal(SIGPIPE, sig_handler); /* reset it to this function */
        CcspTraceInfo(("SIGPIPE received!\n"));
    }
    else {
        /* get stack trace first */
        _print_stack_backtrace();
        CcspTraceInfo(("Signal %d received, exiting!\n", sig));
        exit(0);
    }

}

int main(int argc, char* argv[])
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    BOOL                            bRunAsDaemon       = TRUE;
    int                             cmdChar            = 0;
    int                             idx = 0;
    char                            cmd[1024]          = {0};
    FILE                           *fd                 = NULL;

    appcaps.caps = NULL;
    appcaps.user_name = NULL;
    char buf[8] = {'\0'};
    extern ANSC_HANDLE bus_handle;
    char *subSys            = NULL;
    DmErr_t    err;

    bool blocklist_ret = false;
    blocklist_ret = isBlocklisted();
    if(blocklist_ret)
    {
        CcspTraceInfo(("NonRoot feature is disabled\n"));
    }
    else
    {
        CcspTraceInfo(("NonRoot feature is enabled, dropping root privileges for RdkWanManager Process\n"));
        init_capability();
        drop_root_caps(&appcaps);
        update_process_caps(&appcaps);
        read_capability(&appcaps);
    }

    for (idx = 1; idx < argc; idx++)
    {
        if ( (strcmp(argv[idx], "-subsys") == 0) )
        {
            AnscCopyString(g_Subsystem, argv[idx+1]);
        }
        else if ( strcmp(argv[idx], "-c") == 0 )
        {
            bRunAsDaemon = FALSE;
        }
    }

    pComponentName          = COMPONENT_NAME_WANMANAGER;

    rdk_logger_init(DEBUG_INI_NAME);

    //DATA INIT
    WanMgr_Data_Init();

    if ( bRunAsDaemon )
        daemonize();

    fd = fopen("/var/tmp/wanmanager.pid", "w+");
    if ( !fd )
    {
        CcspTraceWarning(("Create /var/tmp/wanmanager.pid error. \n"));
        return 1;
    }
    else
    {
        sprintf(cmd, "%d", getpid());
        fputs(cmd, fd);
        fclose(fd);
    }

#ifdef INCLUDE_BREAKPAD
    breakpad_ExceptionHandler();
#else
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    /*signal(SIGCHLD, sig_handler);*/
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);

    signal(SIGSEGV, sig_handler);
    signal(SIGBUS, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGFPE, sig_handler);
    signal(SIGILL, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGHUP, sig_handler);
#endif //INCLUDE_BREAKPAD

    cmd_dispatch('e');
#ifdef _COSA_SIM_
    subSys = "";        /* PC simu use empty string as subsystem */
#else
    subSys = NULL;      /* use default sub-system */
#endif //_COSA_SIM_
    err = Cdm_Init(bus_handle, subSys, NULL, NULL, pComponentName);
    if (err != CCSP_SUCCESS)
    {
        fprintf(stderr, "Cdm_Init: %s\n", Cdm_StrError(err));
        exit(1);
    }

#ifdef ENABLE_SD_NOTIFY
    sd_notifyf(0, "READY=1\n"
              "STATUS=wanmanager is Successfully Initialized\n"
              "MAINPID=%lu", (unsigned long) getpid());

    CcspTraceInfo(("RDKB_SYSTEM_BOOT_UP_LOG : wanmanager sd_notify Called\n"));
#endif //ENABLE_SD_NOTIFY

    /* Inform Webconfig framework if component is coming after crash */
    check_component_crash("/tmp/wanmanager_initialized");

    system("touch /tmp/wanmanager_initialized");


    //CORE INT
    WanMgr_Core_Init();

#if defined (FEATURE_RDKB_WAN_MANAGER)
#if !defined(AUTOWAN_ENABLE) && !defined(INTEL_PUMA7)// This is not needed when auto wan is enabled for TCXBX platforms
#if defined (_HUB4_PRODUCT_REQ_) || defined(_PLATFORM_RASPBERRYPI_)
    waitUntilSystemReady();
#endif //_HUB4_PRODUCT_REQ_ || _PLATFORM_RASPBERRYPI_
#endif
#endif //#if defined (FEATURE_RDKB_WAN_MANAGER)

    WanMgrDmlWanWebConfigInit();

    if ( bRunAsDaemon )
    {
        //MAIN THREAD
        WanMgr_Core_Start();
    }
    else
    {
        while ( cmdChar != 'q' )
        {
            cmdChar = getchar();

            cmd_dispatch(cmdChar);
        }
    }


    //CORE FINALISE
    WanMgr_Core_Finalise();

    err = Cdm_Term();
    if (err != CCSP_SUCCESS)
    {
    fprintf(stderr, "Cdm_Term: %s\n", Cdm_StrError(err));
    exit(1);
    }

    ssp_cancel();

    //DATA DELETE
    WanMgr_Data_Delete();

    return 0;
}

