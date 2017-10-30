/* Copyright (c) 2017, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef LOC_NET_IFACE_H
#define LOC_NET_IFACE_H

#include <LocNetIfaceBase.h>
#include <dsi_netctrl.h>
#include <QCMAP_Client.h>

/*--------------------------------------------------------------------
 *  LE platform specific implementation for LocNetIface
 *-------------------------------------------------------------------*/
class LocNetIface : public LocNetIfaceBase {

public:
    /* Constructor */
    LocNetIface(LocNetConnType connType) :
        LocNetIfaceBase(connType),
        mQcmapClientPtr(NULL), mConnectBackhaulSent(false),
        mIsConnectBackhaulPending(false), mIsDisconnectBackhaulPending(false),
        mIsDsiInitDone(false), mDsiHandle(NULL), mIsDsiCallUp(false),
        mIsDsiStartCallPending(false), mIsDsiStopCallPending(false) {}
    LocNetIface() : LocNetIface(LOC_NET_CONN_TYPE_WWAN_INTERNET) {}

    /* Override base class pure virtual methods */
    bool setupWwanCall();
    bool stopWwanCall();
    void subscribe(const std::list<DataItemId>& itemListToSubscribe);
    void unsubscribe(const std::list<DataItemId>& itemListToUnsubscribe);
    void unsubscribeAll();
    void requestData(const std::list<DataItemId>& itemListToRequestData);

    /* Setup WWAN backhaul via QCMAP
     * This sets up IP routes as well for any AP socket */
    bool connectBackhaul();
    /* Disconnects the WWANbackhaul, only if it was setup by us */
    bool disconnectBackhaul();

    /* APIs to fetch current WLAN/WWAN status */
    bool isWlanConnected();
    bool isWwanConnected();

private:
    /* QCMAP client handle
     * This will be set only for static sQcmapInstance. */
    QCMAP_Client* mQcmapClientPtr;

    /* Flag to track whether we've setup QCMAP backhaul */
    bool mConnectBackhaulSent;
    bool mIsConnectBackhaulPending;
    bool mIsDisconnectBackhaulPending;

    /* Maintain an exclusive instance for QCMAP interaction.
     * QCMAP does NOT support passing in/out a void user data pointer,
     * Hence we need to track the instance used internally. */
    static LocNetIface* sLocNetIfaceInstance;

    /* Private APIs to interact with QCMAP module */
    void subscribeWithQcmap();
    void unsubscribeWithQcmap();
    void handleQcmapCallback(
            qcmap_msgr_wlan_status_ind_msg_v01 &wlanStatusIndData);
    void handleQcmapCallback(
            qcmap_msgr_station_mode_status_ind_msg_v01 &stationModeIndData);
    void handleQcmapCallback(
            qcmap_msgr_wwan_status_ind_msg_v01 &wwanStatusIndData);
    void handleQcmapCallback(
            qcmap_msgr_bring_up_wwan_ind_msg_v01 &bringUpWwanIndData);
    void handleQcmapCallback(
            qcmap_msgr_tear_down_wwan_ind_msg_v01 &teardownWwanIndData);
    void notifyObserverForWlanStatus(bool isWlanEnabled);
    void notifyObserverForNetworkInfo(boolean isConnected, LocNetConnType connType);
    void notifyCurrentNetworkInfo();
    void notifyCurrentWifiHardwareState();

    /* Callback registered with QCMAP */
    static void qcmapClientCallback
    (
      qmi_client_type user_handle,   /* QMI user handle. */
      unsigned int msg_id,           /* Indicator message ID. */
      void *ind_buf,                 /* Raw indication data. */
      unsigned int ind_buf_len,      /* Raw data length. */
      void *ind_cb_data              /* User callback handle. */
    );

    /* Data call setup specific members */
    bool mIsDsiInitDone;
    dsi_hndl_t mDsiHandle;
    bool mIsDsiCallUp;
    bool mIsDsiStartCallPending;
    bool mIsDsiStopCallPending;

    /* Callback registered with DSI */
    static void dsiNetEventCallback(
            dsi_hndl_t dsiHandle, void* userDataPtr, dsi_net_evt_t event,
            dsi_evt_payload_t* eventPayloadPtr);
    void handleDSCallback(dsi_hndl_t dsiHandle, bool isNetConnected);
};

#endif /* #ifndef LOC_NET_IFACE_H */
