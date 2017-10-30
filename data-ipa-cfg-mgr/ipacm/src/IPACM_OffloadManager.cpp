/*
Copyright (c) 2017, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials provided
  with the distribution.
* Neither the name of The Linux Foundation nor the names of its
  contributors may be used to endorse or promote products derived
  from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.Z
*/
/*!
  @file
  IPACM_OffloadManager.cpp

  @brief
  This file implements the basis Iface functionality.

  @Author
  Skylar Chang

*/
#include <IPACM_OffloadManager.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include "IPACM_ConntrackClient.h"
#include "IPACM_ConntrackListener.h"
#include "IPACM_Iface.h"
#include "IPACM_Config.h"

const char *IPACM_OffloadManager::DEVICE_NAME = "/dev/wwan_ioctl";

/* NatApp class Implementation */
IPACM_OffloadManager *IPACM_OffloadManager::pInstance = NULL;

IPACM_OffloadManager::IPACM_OffloadManager()
{
	default_gw_index = INVALID_IFACE;
	upstream_v4_up = false;
	upstream_v6_up = false;
	return ;
}

RET IPACM_OffloadManager::registerEventListener(IpaEventListener* eventlistener)
{
	RET result = SUCCESS;
	if (elrInstance == NULL)
		elrInstance = eventlistener;
	else {
		IPACMDBG_H("already register EventListener previously \n");
		result = FAIL_INPUT_CHECK;
	}
	return SUCCESS;
}

RET IPACM_OffloadManager::unregisterEventListener(IpaEventListener* )
{
	RET result = SUCCESS;
	if (elrInstance)
		elrInstance = NULL;
	else {
		IPACMDBG_H("already unregisterEventListener previously \n");
		result = SUCCESS_DUPLICATE_CONFIG;
	}
	return SUCCESS;
}

RET IPACM_OffloadManager::registerCtTimeoutUpdater(ConntrackTimeoutUpdater* timeoutupdater)
{
	RET result = SUCCESS;
	if (touInstance == NULL)
		touInstance = timeoutupdater;
	else {
		IPACMDBG_H("already register ConntrackTimeoutUpdater previously \n");
		result = FAIL_INPUT_CHECK;
	}
	return SUCCESS;
}

RET IPACM_OffloadManager::unregisterCtTimeoutUpdater(ConntrackTimeoutUpdater* )
{
	RET result = SUCCESS;
	if (touInstance)
		touInstance = NULL;
	else {
		IPACMDBG_H("already unregisterCtTimeoutUpdater previously \n");
		result = SUCCESS_DUPLICATE_CONFIG;
	}
	return SUCCESS;
}

RET IPACM_OffloadManager::provideFd(int fd, unsigned int groups)
{
	IPACM_ConntrackClient *cc;
	int on = 1, rel;

	cc = IPACM_ConntrackClient::GetInstance();

	if(!cc)
	{
		IPACMERR("Init failed: cc %p\n", cc);
		return FAIL_HARDWARE;
	}

	if (groups == cc->subscrips_tcp) {
		cc->fd_tcp = fd;
		IPACMDBG_H("Received fd %d with groups %d.\n", fd, groups);
		/* set netlink buf */
		rel = setsockopt(cc->fd_tcp, SOL_NETLINK, NETLINK_NO_ENOBUFS, &on, sizeof(int) );
		if (rel == -1)
		{
			IPACMERR( "setsockopt returned error code %d ( %s )", errno, strerror( errno ) );
		}
	} else if (groups == cc->subscrips_udp) {
		cc->fd_udp = fd;
		IPACMDBG_H("Received fd %d with groups %d.\n", fd, groups);
		/* set netlink buf */
		rel = setsockopt(cc->fd_tcp, SOL_NETLINK, NETLINK_NO_ENOBUFS, &on, sizeof(int) );
		if (rel == -1)
		{
			IPACMERR( "setsockopt returned error code %d ( %s )", errno, strerror( errno ) );
		}
	} else {
		IPACMERR("Received unexpected fd with groups %d.\n", groups);
	}
	if(cc->fd_tcp >0 && cc->fd_udp >0) {
		IPACMDBG_H(" Got both fds from framework, start conntrack listener thread.\n");
		CtList->CreateConnTrackThreads();
	}
	return SUCCESS;
}

RET IPACM_OffloadManager::clearAllFds()
{
	IPACM_ConntrackClient *cc;

	cc = IPACM_ConntrackClient::GetInstance();
	if(!cc)
	{
		IPACMERR("Init clear: cc %p \n", cc);
		return FAIL_HARDWARE;
	}
	cc->UNRegisterWithConnTrack();

	return SUCCESS;
}

bool IPACM_OffloadManager::isStaApSupported()
{
	return true;
}


RET IPACM_OffloadManager::setLocalPrefixes(std::vector<Prefix> &/* prefixes */)
{
	return SUCCESS;
}

RET IPACM_OffloadManager::addDownstream(const char * downstream_name, const Prefix &prefix)
{
	int index;
	ipacm_cmd_q_data evt;
	ipacm_event_ipahal_stream *evt_data;

	IPACMDBG_H("addDownstream name(%s), ip-family(%d) \n", downstream_name, prefix.fam);
	if (prefix.fam == V4) {
		IPACMDBG_H("subnet info v4Addr (%x) v4Mask (%x)\n", prefix.v4Addr, prefix.v4Mask);
	} else {
		IPACMDBG_H("subnet info v6Addr: %08x:%08x:%08x:%08x \n",
							prefix.v6Addr[0], prefix.v6Addr[1], prefix.v6Addr[2], prefix.v6Addr[3]);
		IPACMDBG_H("subnet info v6Mask: %08x:%08x:%08x:%08x \n",
							prefix.v6Mask[0], prefix.v6Mask[1], prefix.v6Mask[2], prefix.v6Mask[3]);
	}

	if(ipa_get_if_index(downstream_name, &index))
	{
		IPACMERR("fail to get iface index.\n");
		return FAIL_HARDWARE;
	}

	evt_data = (ipacm_event_ipahal_stream*)malloc(sizeof(ipacm_event_ipahal_stream));
	if(evt_data == NULL)
	{
		IPACMERR("Failed to allocate memory.\n");
		return FAIL_HARDWARE;
	}
	memset(evt_data, 0, sizeof(*evt_data));

	evt_data->if_index = index;
	memcpy(&evt_data->prefix, &prefix, sizeof(evt_data->prefix));

	memset(&evt, 0, sizeof(ipacm_cmd_q_data));
	evt.evt_data = (void*)evt_data;
	evt.event = IPA_DOWNSTREAM_ADD;

	IPACMDBG_H("Posting event IPA_DOWNSTREAM_ADD\n");
	IPACM_EvtDispatcher::PostEvt(&evt);

	return SUCCESS;
}

RET IPACM_OffloadManager::removeDownstream(const char * downstream_name, const Prefix &prefix)
{
	int index;
	ipacm_cmd_q_data evt;
	ipacm_event_ipahal_stream *evt_data;

	IPACMDBG_H("removeDownstream name(%s), ip-family(%d) \n", downstream_name, prefix.fam);
	if(ipa_get_if_index(downstream_name, &index))
	{
		IPACMERR("fail to get iface index.\n");
		return FAIL_HARDWARE;
	}

	evt_data = (ipacm_event_ipahal_stream*)malloc(sizeof(ipacm_event_ipahal_stream));
	if(evt_data == NULL)
	{
		IPACMERR("Failed to allocate memory.\n");
		return FAIL_HARDWARE;
	}
	memset(evt_data, 0, sizeof(*evt_data));

	evt_data->if_index = index;
	memcpy(&evt_data->prefix, &prefix, sizeof(evt_data->prefix));

	memset(&evt, 0, sizeof(ipacm_cmd_q_data));
	evt.evt_data = (void*)evt_data;
	evt.event = IPA_DOWNSTREAM_DEL;

	IPACMDBG_H("Posting event IPA_DOWNSTREAM_DEL\n");
	IPACM_EvtDispatcher::PostEvt(&evt);

	return SUCCESS;
}

RET IPACM_OffloadManager::setUpstream(const char *upstream_name, const Prefix& gw_addr_v4 , const Prefix& gw_addr_v6)
{
	int index;
	ipacm_cmd_q_data evt;
	ipacm_event_data_addr *evt_data_addr;
	RET result = SUCCESS;

	/* if interface name is NULL, default route is removed */
	IPACMDBG_H("setUpstream upstream_name(%s), ipv4-fam(%d) ipv6-fam(%d)\n", upstream_name, gw_addr_v4.fam, gw_addr_v6.fam);

	if(upstream_name == NULL)
	{
		if (default_gw_index == INVALID_IFACE) {
			IPACMERR("no previous upstream set before\n");
			return FAIL_INPUT_CHECK;
		}

		if (gw_addr_v4.fam == V4 && upstream_v4_up == true) {
			IPACMDBG_H("clean upstream(%s) for ipv4-fam(%d) upstream_v4_up(%d)\n", upstream_name, gw_addr_v4.fam, upstream_v4_up);
			post_route_evt(IPA_IP_v4, default_gw_index, IPA_WAN_UPSTREAM_ROUTE_DEL_EVENT, gw_addr_v4);
			upstream_v4_up = false;
		}
		if (gw_addr_v6.fam == V6 && upstream_v6_up == true) {
			IPACMDBG_H("clean upstream(%s) for ipv6-fam(%d) upstream_v6_up(%d)\n", upstream_name, gw_addr_v6.fam, upstream_v6_up);
			post_route_evt(IPA_IP_v6, default_gw_index, IPA_WAN_UPSTREAM_ROUTE_DEL_EVENT, gw_addr_v6);
			upstream_v6_up = false;
		}
		default_gw_index = INVALID_IFACE;
	}
	else
	{
		if(ipa_get_if_index(upstream_name, &index))
		{
			IPACMERR("fail to get iface index.\n");
			return FAIL_INPUT_CHECK;
		}

		/* reset the stats when switch from LTE->STA */
		if (index != default_gw_index) {
			IPACMDBG_H(" interface switched to %s\n", upstream_name);
			if(memcmp(upstream_name, "wlan0", sizeof("wlan0")) == 0)
			{
				IPACMDBG_H("switch to STA mode, need reset wlan-fw stats\n");
				resetTetherStats(upstream_name);
			}
		}

		if (gw_addr_v4.fam == V4 && gw_addr_v6.fam == V6) {

			if (upstream_v4_up == false) {
				IPACMDBG_H("IPV4 gateway: 0x%x \n", gw_addr_v4.v4Addr);
				/* posting route add event for both IPv4 and IPv6 */
				post_route_evt(IPA_IP_v4, index, IPA_WAN_UPSTREAM_ROUTE_ADD_EVENT, gw_addr_v4);
				upstream_v4_up = true;
			} else {
				IPACMDBG_H("already setupstream iface(%s) ipv4 previously\n", upstream_name);
			}

			if (upstream_v6_up == false) {
				IPACMDBG_H("IPV6 gateway: %08x:%08x:%08x:%08x \n",
						gw_addr_v6.v6Addr[0], gw_addr_v6.v6Addr[1], gw_addr_v6.v6Addr[2], gw_addr_v6.v6Addr[3]);
				post_route_evt(IPA_IP_v6, index, IPA_WAN_UPSTREAM_ROUTE_ADD_EVENT, gw_addr_v6);
				upstream_v6_up = true;
			} else {
				IPACMDBG_H("already setupstream iface(%s) ipv6 previously\n", upstream_name);
			}
		} else if (gw_addr_v4.fam == V4 ) {
			IPACMDBG_H("check upstream_v6_up (%d) v4_up (%d), default gw index (%d)\n", upstream_v6_up, upstream_v4_up, default_gw_index);
			if (upstream_v6_up == true && default_gw_index != INVALID_IFACE ) {
				/* clean up previous V6 upstream event */
				IPACMDBG_H(" Post clean-up v6 default gw on iface %d\n", default_gw_index);
				post_route_evt(IPA_IP_v6, default_gw_index, IPA_WAN_UPSTREAM_ROUTE_DEL_EVENT, gw_addr_v6);
				upstream_v6_up = false;
			}

			if (upstream_v4_up == false) {
				IPACMDBG_H("IPV4 gateway: %x \n", gw_addr_v4.v4Addr);
				/* posting route add event for both IPv4 and IPv6 */
				post_route_evt(IPA_IP_v4, index, IPA_WAN_UPSTREAM_ROUTE_ADD_EVENT, gw_addr_v4);
				upstream_v4_up = true;
			} else {
				IPACMDBG_H("already setupstream iface(%s) ipv4 previously\n", upstream_name);
				result = SUCCESS_DUPLICATE_CONFIG;
			}
		} else if (gw_addr_v6.fam == V6) {
			IPACMDBG_H("check upstream_v6_up (%d) v4_up (%d), default gw index (%d)\n", upstream_v6_up, upstream_v4_up, default_gw_index);
			if (upstream_v4_up == true && default_gw_index != INVALID_IFACE ) {
				/* clean up previous V4 upstream event */
				IPACMDBG_H(" Post clean-up v4 default gw on iface %d\n", default_gw_index);
				post_route_evt(IPA_IP_v4, default_gw_index, IPA_WAN_UPSTREAM_ROUTE_DEL_EVENT, gw_addr_v4);
				upstream_v4_up = false;
			}

			if (upstream_v6_up == false) {
				IPACMDBG_H("IPV6 gateway: %08x:%08x:%08x:%08x \n",
						gw_addr_v6.v6Addr[0], gw_addr_v6.v6Addr[1], gw_addr_v6.v6Addr[2], gw_addr_v6.v6Addr[3]);
				post_route_evt(IPA_IP_v6, index, IPA_WAN_UPSTREAM_ROUTE_ADD_EVENT, gw_addr_v6);
				upstream_v6_up = true;
			} else {
				IPACMDBG_H("already setupstream iface(%s) ipv6 previously\n", upstream_name);
				result = SUCCESS_DUPLICATE_CONFIG;
			}
		}
		default_gw_index = index;
		IPACMDBG_H("Change degault_gw netdev to (%s)\n", upstream_name);
	}
	return result;
}

RET IPACM_OffloadManager::stopAllOffload()
{
	return SUCCESS;
}

RET IPACM_OffloadManager::setQuota(const char * upstream_name /* upstream */, uint64_t mb/* limit */)
{
	wan_ioctl_set_data_quota quota;
	int fd = -1;

	if ((fd = open(DEVICE_NAME, O_RDWR)) < 0)
	{
		IPACMERR("Failed opening %s.\n", DEVICE_NAME);
		return FAIL_HARDWARE;
	}

	quota.quota_mbytes = mb;
	quota.set_quota = true;

    memset(quota.interface_name, 0, IFNAMSIZ);
    if (strlcpy(quota.interface_name, upstream_name, IFNAMSIZ) >= IFNAMSIZ) {
		IPACMERR("String truncation occurred on upstream");
		close(fd);
		return FAIL_INPUT_CHECK;
	}

	IPACMDBG_H("SET_DATA_QUOTA %s %lld", quota.interface_name, mb);

	if (ioctl(fd, WAN_IOC_SET_DATA_QUOTA, &quota) < 0) {
        IPACMERR("IOCTL WAN_IOCTL_SET_DATA_QUOTA call failed: %s", strerror(errno));
		close(fd);
		return FAIL_TRY_AGAIN;
	}

	close(fd);
	return SUCCESS;
}

RET IPACM_OffloadManager::getStats(const char * upstream_name /* upstream */,
		bool reset /* reset */, OffloadStatistics& offload_stats/* ret */)
{
	int fd = -1;
	wan_ioctl_query_tether_stats_all stats;

	if ((fd = open(DEVICE_NAME, O_RDWR)) < 0) {
        IPACMERR("Failed opening %s.\n", DEVICE_NAME);
        return FAIL_HARDWARE;
    }

    memset(&stats, 0, sizeof(stats));
    if (strlcpy(stats.upstreamIface, upstream_name, IFNAMSIZ) >= IFNAMSIZ) {
		IPACMERR("String truncation occurred on upstream\n");
		close(fd);
		return FAIL_INPUT_CHECK;
	}
	stats.reset_stats = reset;
	stats.ipa_client = IPACM_CLIENT_MAX;

	if (ioctl(fd, WAN_IOC_QUERY_TETHER_STATS_ALL, &stats) < 0) {
        IPACMERR("IOCTL WAN_IOC_QUERY_TETHER_STATS_ALL call failed: %s \n", strerror(errno));
		close(fd);
		return FAIL_TRY_AGAIN;
	}
	/* feedback to IPAHAL*/
	offload_stats.tx = stats.tx_bytes;
	offload_stats.rx = stats.rx_bytes;

	IPACMDBG_H("send getStats tx:%lld rx:%lld \n", offload_stats.tx, offload_stats.rx);
	return SUCCESS;
}

int IPACM_OffloadManager::post_route_evt(enum ipa_ip_type iptype, int index, ipa_cm_event_id event, const Prefix &gw_addr)
{
	ipacm_cmd_q_data evt;
	ipacm_event_data_iptype *evt_data_route;

	evt_data_route = (ipacm_event_data_iptype*)malloc(sizeof(ipacm_event_data_iptype));
	if(evt_data_route == NULL)
	{
		IPACMERR("Failed to allocate memory.\n");
		return -EFAULT;
	}
	memset(evt_data_route, 0, sizeof(*evt_data_route));

	evt_data_route->if_index = index;
	evt_data_route->if_index_tether = 0;
	evt_data_route->iptype = iptype;

#ifdef IPA_WAN_MSG_IPv6_ADDR_GW_LEN
	evt_data_route->ipv4_addr_gw = gw_addr.v4Addr;
	evt_data_route->ipv6_addr_gw[0] = gw_addr.v6Addr[0];
	evt_data_route->ipv6_addr_gw[1] = gw_addr.v6Addr[1];
	evt_data_route->ipv6_addr_gw[2] = gw_addr.v6Addr[2];
	evt_data_route->ipv6_addr_gw[3] = gw_addr.v6Addr[3];
	IPACMDBG_H("default gw ipv4 (%x)\n", evt_data_route->ipv4_addr_gw);
	IPACMDBG_H("IPV6 gateway: %08x:%08x:%08x:%08x \n",
					evt_data_route->ipv6_addr_gw[0], evt_data_route->ipv6_addr_gw[1], evt_data_route->ipv6_addr_gw[2], evt_data_route->ipv6_addr_gw[3]);
#endif
	IPACMDBG_H("Received WAN_UPSTREAM_ROUTE_ADD: fid(%d) tether_fid(%d) ip-type(%d)\n", evt_data_route->if_index,
			evt_data_route->if_index_tether, evt_data_route->iptype);

	memset(&evt, 0, sizeof(evt));
	evt.evt_data = (void*)evt_data_route;
	evt.event = event;

	IPACM_EvtDispatcher::PostEvt(&evt);

	return 0;
}

int IPACM_OffloadManager::ipa_get_if_index(const char * if_name, int * if_index)
{
	int fd;
	struct ifreq ifr;

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		IPACMERR("get interface index socket create failed \n");
		return IPACM_FAILURE;
	}

	if(strnlen(if_name, sizeof(if_name)) >= sizeof(ifr.ifr_name)) {
		IPACMERR("interface name overflows: len %d\n", strnlen(if_name, sizeof(if_name)));
		close(fd);
		return IPACM_FAILURE;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	(void)strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name));
	IPACMDBG_H("interface name (%s)\n", if_name);

	if(ioctl(fd,SIOCGIFINDEX , &ifr) < 0)
	{
		IPACMERR("call_ioctl_on_dev: ioctl failed, interface name (%s):\n", ifr.ifr_name);
		close(fd);
		return IPACM_FAILURE;
	}

	*if_index = ifr.ifr_ifindex;
	IPACMDBG_H("Interface netdev index %d\n", *if_index);
	close(fd);
	return IPACM_SUCCESS;
}

int IPACM_OffloadManager::resetTetherStats(const char * upstream_name /* upstream */)
{
	int fd = -1;
	wan_ioctl_reset_tether_stats stats;

	if ((fd = open(DEVICE_NAME, O_RDWR)) < 0) {
        IPACMERR("Failed opening %s.\n", DEVICE_NAME);
        return FAIL_HARDWARE;
    }

    memset(stats.upstreamIface, 0, IFNAMSIZ);
    if (strlcpy(stats.upstreamIface, upstream_name, IFNAMSIZ) >= IFNAMSIZ) {
		IPACMERR("String truncation occurred on upstream\n");
		close(fd);
		return FAIL_INPUT_CHECK;
	}
	stats.reset_stats = true;

	if (ioctl(fd, WAN_IOC_RESET_TETHER_STATS, &stats) < 0) {
        IPACMERR("IOCTL WAN_IOC_RESET_TETHER_STATS call failed: %s", strerror(errno));
		close(fd);
		return FAIL_HARDWARE;
	}
	IPACMDBG_H("Reset Interface %s stats\n", upstream_name);
	return IPACM_SUCCESS;
}

IPACM_OffloadManager* IPACM_OffloadManager::GetInstance()
{
	if(pInstance == NULL)
		pInstance = new IPACM_OffloadManager();

	return pInstance;
}
