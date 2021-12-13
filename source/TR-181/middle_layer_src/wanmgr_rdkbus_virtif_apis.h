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


#ifndef  _WANMGR_RDKBUS_APIS_H_
#define  _WANMGR_RDKBUS_APIS_H_

#include "wanmgr_apis.h"
#include "ccsp_psm_helper.h"
#include "ansc_platform.h"
#include "ansc_string_util.h"
#include "wanmgr_dml.h"
#include "wanmgr_data.h"
#include <stdio.h>
#include <stdbool.h>
#include "wanmgr_rdkbus_common.h"

#define WAN_IF_VIRTIF_MAX_LIMIT       ( 5 )

/*Virtual Interface Entry Contaxt Data Storage*/
typedef  struct _CONTEXT_VIRTIF_LINK_OBJECT
{
    CONTEXT_LINK_CLASS_CONTENT
    BOOL    bFound;
} CONTEXT_VIRTIF_LINK_OBJECT;

#define  ACCESS_CONTEXT_VIRTIF_LINK_OBJECT(p)              \
         ACCESS_CONTAINER(p, CONTEXT_VIRTIF_LINK_OBJECT, Linkage)


/*Virtual Interface Vlan Entry Contaxt Data Storage*/
typedef  struct _CONTEXT_VIRTIF_VLAN_LINK_OBJECT
{
    CONTEXT_LINK_CLASS_CONTENT
    BOOL    bFound;
} CONTEXT_VIRTIF_VLAN_LINK_OBJECT;

#define  ACCESS_CONTEXT_VIRTIF_VLAN_LINK_OBJECT(p)              \
         ACCESS_CONTAINER(p, CONTEXT_VIRTIF_VLAN_LINK_OBJECT, Linkage)


/*Virtual Interface Marking Entry Contaxt Data Storage*/
typedef  struct _CONTEXT_VIRTIF_MARKING_LINK_OBJECT
{
    CONTEXT_LINK_CLASS_CONTENT
    BOOL    bFound;
} CONTEXT_VIRTIF_MARKING_LINK_OBJECT;

#define  ACCESS_CONTEXT_VIRTIF_MARKING_LINK_OBJECT(p)              \
         ACCESS_CONTAINER(p, CONTEXT_VIRTIF_MARKING_LINK_OBJECT, Linkage)


ANSC_STATUS WanMgr_WanConfigInit(void);
ANSC_STATUS WanManager_SetWanIfCfg( INT LineIndex, DML_WAN_IFACE* pstLineInfo );
ANSC_STATUS WanManager_SetVirtIfCfg(DML_VIRTIFACE_INFO *VirtIf);
ANSC_STATUS DmlAddMarking(ANSC_HANDLE hContext,DML_MARKING* pMarking);
ANSC_STATUS DmlDeleteMarking(ANSC_HANDLE hContext, DML_MARKING* pMarking);
ANSC_STATUS DmlSetMarking(ANSC_HANDLE hContext, DML_MARKING*   pMarking);
ANSC_STATUS WanMgr_WanIfaceConfInit(WanMgr_IfaceCtrl_Data_t* pWanIfaceCtrl);
ANSC_STATUS WanMgr_WanIfaceMarkingInit (WanMgr_IfaceCtrl_Data_t* pWanIfaceCtrl);

ANSC_HANDLE WanManager_AddIfaceMarking(DML_WAN_IFACE* pWanDmlIface, ULONG* pInsNumber);
ANSC_STATUS DmlSetWanSelectionStatusInPSMDB( ULONG instancenum, WANMGR_IFACE_SELECTION Status);
ANSC_STATUS DmlSetWanActiveLinkInPSMDB( UINT uiInterfaceIdx , bool storeValue );
ANSC_STATUS WanController_ClearWanConfigurationsInPSM();

/*Virtual Interface Entry APIs */
ANSC_HANDLE WanManager_Create_VirtIf_Entry(DML_WAN_IFACE* pWanDmlIface, ULONG* pInsNumber);
ULONG WanManager_Remove_VirtIf_Entry(DML_WAN_IFACE* pWanDmlIface, CONTEXT_VIRTIF_LINK_OBJECT *pCxtLink);
ULONG WanManager_Update_VirtIf_Entry(CONTEXT_VIRTIF_LINK_OBJECT *pCxtLink);

/*Virtual Interface VLAN Entry APIs */
ANSC_HANDLE WanManager_Create_VirtIfVlan_Entry(CONTEXT_VIRTIF_LINK_OBJECT *pVirtIfCxtLink, ULONG* pInsNumber);
ULONG WanManager_Remove_VirtIfVlan_Entry(CONTEXT_VIRTIF_LINK_OBJECT *pVirtIfCxtLink, CONTEXT_VIRTIF_VLAN_LINK_OBJECT *pVirtIfVlanCxtLink);
ULONG WanManager_Update_VirtIfVlan_Entry(CONTEXT_VIRTIF_VLAN_LINK_OBJECT *pCxtLink);

/*Virtual Interface Marking Entry APIs */
ANSC_HANDLE WanManager_Create_VirtIfMarking_Entry(CONTEXT_VIRTIF_LINK_OBJECT *pVirtIfCxtLink, ULONG* pInsNumber);
ULONG WanManager_Remove_VirtIfMarking_Entry(CONTEXT_VIRTIF_LINK_OBJECT *pVirtIfCxtLink, CONTEXT_VIRTIF_MARKING_LINK_OBJECT *pVirtIfMarkingCxtLink);
ULONG WanManager_Update_VirtIfMarking_Entry(CONTEXT_VIRTIF_MARKING_LINK_OBJECT *pCxtLink);

#endif /* _WANMGR_RDKBUS_APIS_H_ */
