##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2020 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

bin_PROGRAMS = wanmanager

AM_CPPFLAGS = -I$(top_srcdir)/hal/include \
		-I$(top_srcdir)/source/LogComponent \
		-I$(top_srcdir)/source/TR-181/middle_layer_src \
		-I$(top_srcdir)/source/TR-181/integration_src.shared \
		-I$(top_srcdir)/source/TR-181/include \
		-I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/components/include


wanmanager_DEPENDENCIES = \
        $(EXTRA_DEPENDENCIES) \
        ${top_builddir}/source/TR-181/middle_layer_src/libCcspWanManager_middle_layer_src.la \
        ${top_builddir}/source/TR-181/integration_src.shared/libCcspWanManager_integration_src_shared.la

wanmanager_CFLAGS = -D_ANSC_LINUX -D_ANSC_USER -D_ANSC_LITTLE_ENDIAN_ -DFEATURE_SUPPORT_RDKLOG $(DBUS_CFLAGS) $(SYSTEMD_CFLAGS)
wanmanager_SOURCES = ssp_action.c ssp_messagebus_interface.c ssp_main.c wan_policy_fmob_impl.c wan_policy_fm_impl.c wan_policy_ppob_impl.c wan_policy_pp_impl.c wan_manager.c wan_controller.c wan_manager_db.c wan_manager_ipc.c wan_manager_utils.c wan_manager_prctl.c wan_manager_dhcpv4_apis.c
wanmanager_LDFLAGS = -lccsp_common -lrdkloggers $(DBUS_LIBS) $(SYSTEMD_LDFLAGS) -lhal_platform
wanmanager_LDADD =  $(wanmanager_DEPENDENCIES)

