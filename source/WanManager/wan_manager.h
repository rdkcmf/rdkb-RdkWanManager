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

#ifndef WAN_MANAGER_H
#define WAN_MANAGER_H
/* ---- Global Types -------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h> 
#include "wan_mgr_apis.h"
#ifdef _HUB4_PRODUCT_REQ_
#include "sysevent/sysevent.h"

extern int sysevent_fd;
extern token_t sysevent_token;

#define CONSOLE_LOG_FILE "/rdklogs/logs/Consolelog.txt.0"
#define LOG_CONSOLE(fmt ...)     {\
                                        FILE     *fp        = NULL;\
                                        fp = fopen ( CONSOLE_LOG_FILE, "a+");\
                                        if (fp)\
                                        {\
                                            fprintf(fp,fmt);\
                                            fclose(fp);\
                                        }\
                               }\

#define SYSEVENT_WAN_LED_STATE "wan_led_state"

#define OFF "Off"
#define FLASHING_AMBER "Flashing Amber"
#define SOLID_AMBER "Solid Amber"
#define SOLID_GREEN "Solid Green"
#endif

#ifdef FEATURE_MAPT_DEBUG
void logPrintMapt(char *fmt,...);
#define LOG_PRINT_MAPT(...) logPrintMapt(__VA_ARGS__ )
#endif /*FEATURE_MAPT_DEBUG*/

typedef struct WanInterfaceData_struct {
    char ifName[32];   // net VLAN interface name (eg: eth3.101,ppp0.101)
    char baseIfName[32]; // Name of the base interface
} WanInterfaceData_t;

int WanManager_Init();
int WanManager_DeInit();
int WanManager_StartIpcServer(); /*IPC server to handle WAN Manager clients*/
int WanManager_StartStateMachine(WanInterfaceData_t *wanif);

#endif /*WAN_MANAGER_H*/
