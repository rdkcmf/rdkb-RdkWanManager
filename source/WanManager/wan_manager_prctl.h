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

#ifndef _WAN_MANAGER_PRCTL_H
#define _WAN_MANAGER_PRCTL_H

#include "wan_manager_db.h"

#ifndef RETURN_OK
#define RETURN_OK   0
#endif

#ifndef RETURN_ERROR
#define RETURN_ERROR   -1
#endif

#define USECS_IN_MSEC 1000
#define MSECS_IN_SEC  1000

int util_spawnProcess(const char *execName, const char *processInfo, int * processId);
int util_terminateProcessForcefully(int32_t pid);
int util_signalProcess(int32_t pid, int32_t sig);
int util_getPidByName(const char *name);
int util_getNameByPid(int pid, char *nameBuf, int nameBufLen);
int util_collectProcess(int pid, int timeout);
int util_runCommandInShellBlocking(char *command);

#endif /* _WAN_MANAGER_PRCTL_H */
