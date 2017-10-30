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
#define LOG_TAG "LocSvc_LocNetIfaceLE"

#include "LocNetIface.h"
#include <QCMAP_Client.h>
#include "qualcomm_mobile_access_point_msgr_v01.h"
#include <platform_lib_log_util.h>
#include "DataItemConcreteTypes.h"
#include <loc_cfg.h>
#include <platform_lib_macros.h>
#include <unistd.h>


/* LocNetIface singleton instance
 * Used for QCMAP registration */
LocNetIface* LocNetIface::sLocNetIfaceInstance = NULL;


void LocNetIface::subscribe (
        const std::list<DataItemId>& itemListToSubscribe) {

    ENTRY_LOG();

    /* Add items to subscribed list */
    bool anyUpdatesToSubscriptionList =
            updateSubscribedItemList(itemListToSubscribe, true);

    /* If either of network info items is in subscription list,
     * subscribe with QCMAP */
    if (anyUpdatesToSubscriptionList) {
        if (isItemSubscribed(NETWORKINFO_DATA_ITEM_ID)) {
            subscribeWithQcmap();
            notifyCurrentNetworkInfo();
        }
        if (isItemSubscribed(WIFIHARDWARESTATE_DATA_ITEM_ID)) {
            subscribeWithQcmap();
            notifyCurrentWifiHardwareState();
        }
    }

    EXIT_LOG_WITH_ERROR("%d", 0);
}

void LocNetIface::unsubscribe (
        const std::list<DataItemId>& itemListToUnsubscribe) {

    ENTRY_LOG();

    /* Remove items from subscribed list */
    bool anyUpdatesToSubscriptionList =
            updateSubscribedItemList(itemListToUnsubscribe, false);

    /* If neither of below two items left in subscription, we can unsubscribe
     * from QCMAP */
    if (anyUpdatesToSubscriptionList &&
            !isItemSubscribed(NETWORKINFO_DATA_ITEM_ID) &&
            !isItemSubscribed(WIFIHARDWARESTATE_DATA_ITEM_ID)) {

        unsubscribeWithQcmap();
    }
}

void LocNetIface::unsubscribeAll () {

    ENTRY_LOG();

    /* Check about network items */
    if (isItemSubscribed(NETWORKINFO_DATA_ITEM_ID) ||
            isItemSubscribed(WIFIHARDWARESTATE_DATA_ITEM_ID)) {

        unsubscribeWithQcmap();
    }

    /* Clear subscription list */
    mSubscribedItemList.clear();
}

void LocNetIface::requestData (
        const std::list<DataItemId>& itemListToRequestData) {

    ENTRY_LOG();

    /* NO-OP for LE platform
     * We don't support any data item to fetch data for */
}

void LocNetIface::subscribeWithQcmap () {

    ENTRY_LOG();

    qmi_error_type_v01 qcmapErr = QMI_ERR_NONE_V01;

    /* We handle qcmap subscription from an exclusive instance */
    if (LocNetIface::sLocNetIfaceInstance != NULL) {

        LOC_LOGI("QCMAP registration already done !");
        return;
    }

    /* First time registration */
    if (LocNetIface::sLocNetIfaceInstance == NULL) {
        LocNetIface::sLocNetIfaceInstance = this;
    }

    /* Are we already subscribed */
    if (mQcmapClientPtr != NULL) {
        LOC_LOGW("Already subscribed !");
        return;
    }

    /* Create a QCMAP Client instance */
    mQcmapClientPtr = new QCMAP_Client(qcmapClientCallback);
    if (mQcmapClientPtr == NULL) {
        LOC_LOGE("Failed to allocate QCMAP instance !");
        return;
    }
    LOC_LOGD("Created QCMAP_Client instance %p", mQcmapClientPtr);

    /* Need to enable MobileAP to get station mode status indications */
    bool ret = mQcmapClientPtr->EnableMobileAP(&qcmapErr);
    if (ret == false || qcmapErr != 0) {
        LOC_LOGE("Failed to enable mobileap, qcmapErr %d", qcmapErr);
    }
    /* Invoke WLAN status registration
     * WWAN is by default registered */
    ret = mQcmapClientPtr->RegisterForWLANStatusIND(&qcmapErr, true);
    if (ret == false || qcmapErr != 0) {
        LOC_LOGE("RegisterForWLANStatusIND failed, qcmapErr %d", qcmapErr);
    }
}

void LocNetIface::unsubscribeWithQcmap () {

    ENTRY_LOG();

    // Simply deleting the qcmap client instance is enough
    if (mQcmapClientPtr == NULL) {
        LOC_LOGE("No QCMAP instance to unsubscribe from");
        return;
    }

    delete mQcmapClientPtr;
    mQcmapClientPtr = NULL;
}

void LocNetIface::qcmapClientCallback (
        qmi_client_type user_handle, /**< QMI user handle. */
        unsigned int msg_id, /**< Indicator message ID. */
        void *ind_buf, /**< Raw indication data. */
        unsigned int ind_buf_len, /**< Raw data length. */
        void *ind_cb_data /**< User callback handle. */ ) {

    ENTRY_LOG();

    qmi_client_error_type qmi_error;

    // Check the message type
    // msg_id  = QMI_QCMAP_MSGR_WLAN_STATUS_IND_V01
    // ind_buf = qcmap_msgr_wlan_status_ind_msg_v01
    switch (msg_id) {

    case QMI_QCMAP_MSGR_WLAN_STATUS_IND_V01: {
        LOC_LOGD("Received QMI_QCMAP_MSGR_WLAN_STATUS_IND_V01");

        qcmap_msgr_wlan_status_ind_msg_v01 wlanStatusIndData;

        /* Parse the indication */
        qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION,
                msg_id, ind_buf, ind_buf_len, &wlanStatusIndData,
                sizeof(qcmap_msgr_wlan_status_ind_msg_v01));

        if (qmi_error != QMI_NO_ERR) {
            LOC_LOGE("qmi_client_message_decode error %d", qmi_error);
            return;
        }

        LocNetIface::sLocNetIfaceInstance->handleQcmapCallback(wlanStatusIndData);
        break;
    }

    case QMI_QCMAP_MSGR_STATION_MODE_STATUS_IND_V01: {
        LOC_LOGD("Received QMI_QCMAP_MSGR_STATION_MODE_STATUS_IND_V01");

        qcmap_msgr_station_mode_status_ind_msg_v01 stationModeIndData;

        /* Parse the indication */
        qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION,
                msg_id, ind_buf, ind_buf_len, &stationModeIndData,
                sizeof(qcmap_msgr_station_mode_status_ind_msg_v01));

        if (qmi_error != QMI_NO_ERR) {
            LOC_LOGE("qmi_client_message_decode error %d", qmi_error);
            return;
        }

        LocNetIface::sLocNetIfaceInstance->handleQcmapCallback(stationModeIndData);
        break;
    }

    case QMI_QCMAP_MSGR_WWAN_STATUS_IND_V01: {
        LOC_LOGD("Received QMI_QCMAP_MSGR_WWAN_STATUS_IND_V01");

        qcmap_msgr_wwan_status_ind_msg_v01 wwanStatusIndData;

        /* Parse the indication */
        qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION,
                msg_id, ind_buf, ind_buf_len, &wwanStatusIndData,
                sizeof(qcmap_msgr_wwan_status_ind_msg_v01));

        if (qmi_error != QMI_NO_ERR) {
            LOC_LOGE("qmi_client_message_decode error %d", qmi_error);
            return;
        }

        LocNetIface::sLocNetIfaceInstance->handleQcmapCallback(wwanStatusIndData);
        break;
    }

    case QMI_QCMAP_MSGR_BRING_UP_WWAN_IND_V01: {
        LOC_LOGD("Received QMI_QCMAP_MSGR_BRING_UP_WWAN_IND_V01");

        qcmap_msgr_bring_up_wwan_ind_msg_v01 bringUpWwanIndData;

        /* Parse the indication */
        qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION,
                msg_id, ind_buf, ind_buf_len, &bringUpWwanIndData,
                sizeof(qcmap_msgr_bring_up_wwan_ind_msg_v01));

        if (qmi_error != QMI_NO_ERR) {
            LOC_LOGE("qmi_client_message_decode error %d", qmi_error);
            return;
        }

        LocNetIface::sLocNetIfaceInstance->handleQcmapCallback(bringUpWwanIndData);
        break;
    }

    case QMI_QCMAP_MSGR_TEAR_DOWN_WWAN_IND_V01: {
        LOC_LOGD("Received QMI_QCMAP_MSGR_TEAR_DOWN_WWAN_IND_V01");

        qcmap_msgr_tear_down_wwan_ind_msg_v01 teardownWwanIndData;

        /* Parse the indication */
        qmi_error = qmi_client_message_decode(user_handle, QMI_IDL_INDICATION,
                msg_id, ind_buf, ind_buf_len, &teardownWwanIndData,
                sizeof(qcmap_msgr_tear_down_wwan_ind_msg_v01));

        if (qmi_error != QMI_NO_ERR) {
            LOC_LOGE("qmi_client_message_decode error %d", qmi_error);
            return;
        }

        LocNetIface::sLocNetIfaceInstance->handleQcmapCallback(teardownWwanIndData);
        break;
    }

    default:
        LOC_LOGE("Ignoring QCMAP indication: %d", msg_id);
    }
}

void LocNetIface::handleQcmapCallback (
        qcmap_msgr_wlan_status_ind_msg_v01 &wlanStatusIndData) {

    ENTRY_LOG();

    LOC_LOGD("WLAN Status (enabled=1, disabled=2): %d",
            wlanStatusIndData.wlan_status);

    LOC_LOGD("WLAN Mode (AP=1, ... STA=6): %d",
            wlanStatusIndData.wlan_mode);

    /* Notify observers */
    if (wlanStatusIndData.wlan_status == QCMAP_MSGR_WLAN_ENABLED_V01) {
        notifyObserverForWlanStatus(true);
    } else if (wlanStatusIndData.wlan_status == QCMAP_MSGR_WLAN_DISABLED_V01) {
        notifyObserverForWlanStatus(false);
    } else {
        LOC_LOGE("Invalid wlan status %d", wlanStatusIndData.wlan_status);
    }
}
void LocNetIface::handleQcmapCallback (
        qcmap_msgr_station_mode_status_ind_msg_v01 &stationModeIndData){

    ENTRY_LOG();

    LOC_LOGI("station mode status: %d", stationModeIndData.station_mode_status);

    /* Notify observers */
    if (stationModeIndData.station_mode_status ==
            QCMAP_MSGR_STATION_MODE_CONNECTED_V01) {
        notifyObserverForNetworkInfo(true, LOC_NET_CONN_TYPE_WLAN);
    } else if (stationModeIndData.station_mode_status ==
                QCMAP_MSGR_STATION_MODE_DISCONNECTED_V01) {
        notifyObserverForNetworkInfo(false, LOC_NET_CONN_TYPE_WLAN);
    } else {
        LOC_LOGE("Unhandled station mode status %d",
                    stationModeIndData.station_mode_status);
    }
}
void LocNetIface::handleQcmapCallback (
        qcmap_msgr_wwan_status_ind_msg_v01 &wwanStatusIndData) {

    ENTRY_LOG();

    LOC_LOGD("WWAN Status (Connected_v4=3, Disconnected_v4=6): %d",
            wwanStatusIndData.wwan_status);

    /* Notify observers */
    if (wwanStatusIndData.wwan_status ==
            QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01) {
        notifyObserverForNetworkInfo(true, LOC_NET_CONN_TYPE_WWAN_INTERNET);
    } else if (wwanStatusIndData.wwan_status ==
            QCMAP_MSGR_WWAN_STATUS_DISCONNECTED_V01) {
        notifyObserverForNetworkInfo(false, LOC_NET_CONN_TYPE_WWAN_INTERNET);
    } else {
        LOC_LOGW("Unsupported wwan status %d",
                wwanStatusIndData.wwan_status);
    }
}
void LocNetIface::handleQcmapCallback (
        qcmap_msgr_bring_up_wwan_ind_msg_v01 &bringUpWwanIndData) {

    ENTRY_LOG();

    LOC_LOGD("WWAN Bring up status (Connected=3, Disconnected=6): %d",
            bringUpWwanIndData.conn_status);

    /* Notify observers */
    if (bringUpWwanIndData.conn_status ==
            QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01) {

        notifyObserverForNetworkInfo(true, LOC_NET_CONN_TYPE_WWAN_INTERNET);

        if (mIsConnectBackhaulPending &&
                mWwanCallStatusCb != NULL){
            LOC_LOGV("LOC_NET_WWAN_CALL_EVT_OPEN_SUCCESS");
            mWwanCallStatusCb(
                    mWwanCbUserDataPtr, LOC_NET_WWAN_CALL_EVT_OPEN_SUCCESS,
                    getApnNameFromConfig(), getIpTypeFromConfig());
        }
        mIsConnectBackhaulPending = false;

    } else if (bringUpWwanIndData.conn_status ==
            QCMAP_MSGR_WWAN_STATUS_CONNECTING_FAIL_V01) {

        if (mIsConnectBackhaulPending &&
                mWwanCallStatusCb != NULL){
            LOC_LOGV("LOC_NET_WWAN_CALL_EVT_OPEN_FAILED");
            mWwanCallStatusCb(
                    mWwanCbUserDataPtr, LOC_NET_WWAN_CALL_EVT_OPEN_FAILED, NULL,
                    LOC_NET_CONN_IP_TYPE_INVALID);
        }
        mIsConnectBackhaulPending = false;

    } else {
        LOC_LOGW("Unsupported wwan status %d",
                bringUpWwanIndData.conn_status);
    }
}

void LocNetIface::handleQcmapCallback(
        qcmap_msgr_tear_down_wwan_ind_msg_v01 &teardownWwanIndData) {

    ENTRY_LOG();

    LOC_LOGD("WWAN teardown status (Connected=3, Disconnected=6): %d",
            teardownWwanIndData.conn_status);

    /* Notify observers */
    if (teardownWwanIndData.conn_status ==
            QCMAP_MSGR_WWAN_STATUS_DISCONNECTED_V01) {

        notifyObserverForNetworkInfo(false, LOC_NET_CONN_TYPE_WWAN_INTERNET);

        if (mIsDisconnectBackhaulPending &&
                mWwanCallStatusCb != NULL) {
            LOC_LOGV("LOC_NET_WWAN_CALL_EVT_CLOSE_SUCCESS");
            mWwanCallStatusCb(
                    mWwanCbUserDataPtr, LOC_NET_WWAN_CALL_EVT_CLOSE_SUCCESS,
                    getApnNameFromConfig(), getIpTypeFromConfig());
        }
        mIsDisconnectBackhaulPending = false;

    } else if (teardownWwanIndData.conn_status ==
            QCMAP_MSGR_WWAN_STATUS_DISCONNECTING_FAIL_V01) {

        if (mIsDisconnectBackhaulPending &&
                mWwanCallStatusCb != NULL){
            LOC_LOGV("LOC_NET_WWAN_CALL_EVT_CLOSE_FAILED");
            mWwanCallStatusCb(
                    mWwanCbUserDataPtr, LOC_NET_WWAN_CALL_EVT_CLOSE_FAILED, NULL,
                    LOC_NET_CONN_IP_TYPE_INVALID);
        }
        mIsDisconnectBackhaulPending = false;

    } else {
        LOC_LOGW("Unsupported wwan status %d",
                teardownWwanIndData.conn_status);
    }
}

void LocNetIface::notifyCurrentNetworkInfo () {

    ENTRY_LOG();

    /* Validate QCMAP Client instance */
    if (mQcmapClientPtr == NULL) {
        LOC_LOGE("No QCMAP instance !");
        return;
    }

    /* Fetch station mode status and notify observers */
    if (isWlanConnected()) {
        notifyObserverForNetworkInfo(true, LOC_NET_CONN_TYPE_WLAN);
    } else {
        notifyObserverForNetworkInfo(false, LOC_NET_CONN_TYPE_WLAN);
    }

    /* Fetch WWAN status and notify observers */
    if (isWwanConnected()) {
        notifyObserverForNetworkInfo(true, LOC_NET_CONN_TYPE_WWAN_INTERNET);
    } else {
        notifyObserverForNetworkInfo(false, LOC_NET_CONN_TYPE_WWAN_INTERNET);
    }
}

void LocNetIface::notifyCurrentWifiHardwareState () {

    ENTRY_LOG();

    /* Validate QCMAP Client instance */
    if (mQcmapClientPtr == NULL) {
        LOC_LOGE("No QCMAP instance !");
        return;
    }

    /* Fetch WLAN status */
    qcmap_msgr_wlan_mode_enum_v01 wlan_mode =
            QCMAP_MSGR_WLAN_MODE_ENUM_MIN_ENUM_VAL_V01;
    qmi_error_type_v01 qmi_err_num = QMI_ERROR_TYPE_MIN_ENUM_VAL_V01;

    if (!mQcmapClientPtr->GetWLANStatus(&wlan_mode, &qmi_err_num)) {
        LOC_LOGE("Failed to fetch wlan status, err %d", qmi_err_num);
        return;
    }

    if (wlan_mode == QCMAP_MSGR_WLAN_MODE_ENUM_MIN_ENUM_VAL_V01) {
        notifyObserverForWlanStatus(false);
    } else if (wlan_mode == QCMAP_MSGR_WLAN_MODE_STA_ONLY_V01 ||
            wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_STA_V01 ||
            wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_STA_V01 ||
            wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_STA_BRIDGE_V01 ||
            wlan_mode == QCMAP_MSGR_WLAN_MODE_AP_AP_STA_BRIDGE_V01 ||
            wlan_mode == QCMAP_MSGR_WLAN_MODE_STA_ONLY_BRIDGE_V01) {
        LOC_LOGD("notifying abt WLAN mode: %d", wlan_mode);
        notifyObserverForWlanStatus(true);
    }
}

void LocNetIface::notifyObserverForWlanStatus (bool isWlanEnabled) {

    ENTRY_LOG();

    /* Validate subscription object */
    if (LocNetIfaceBase::sNotifyCb == NULL){
        LOC_LOGE("Notify callback NULL !");
        return;
    }

    /* Create a wifi hardware status item */
    WifiHardwareStateDataItem wifiStateDataItem;
    IDataItem *dataItem = NULL;

    wifiStateDataItem.mEnabled = isWlanEnabled;
    dataItem = &wifiStateDataItem;

    // Create a list and push data item, since that's what observer expects
    std::list<IDataItem *> dataItemList;
    dataItemList.push_back(dataItem);

    /* Notify back to client */
    LocNetIfaceBase::sNotifyCb(
            LocNetIfaceBase::sNotifyCbUserDataPtr, dataItemList);
}

void LocNetIface::notifyObserverForNetworkInfo (
        boolean isConnected, LocNetConnType connType){

    ENTRY_LOG();

    // Check if observer is registered
    if (LocNetIfaceBase::sNotifyCb == NULL) {
        LOC_LOGE("Notify callback NULL !");
        return;
    }

    // Create a network data item
    NetworkInfoDataItem networkInfoDataItem;
    IDataItem *dataItem = NULL;

    networkInfoDataItem.mType = (int32)connType;
    networkInfoDataItem.mConnected = isConnected;

    dataItem = &networkInfoDataItem;

    // Create a list and push data item, since that's what observer expects
    std::list<IDataItem *> dataItemList;
    dataItemList.push_back(dataItem);

    /* Notify back to client */
    LocNetIfaceBase::sNotifyCb(
            LocNetIfaceBase::sNotifyCbUserDataPtr, dataItemList);
}

bool LocNetIface::setupWwanCall () {

    ENTRY_LOG();

    /* Validate call type requested */
    if (mLocNetConnType != LOC_NET_CONN_TYPE_WWAN_SUPL) {
        LOC_LOGE("Unsupported call type configured: %d", mLocNetConnType);
        return false;
    }

    /* Check for ongoing start/stop attempts */
    if (mIsDsiStartCallPending) {
        LOC_LOGW("Already start pending, returning as no-op");
        return true;
    }
    if (mIsDsiStopCallPending) {
        LOC_LOGE("Stop attempt pending, can't start now !");
        /* When stop completes and DS callback is received, we will
         * notify the client. So no need to notify now. */
        return false;
    }
    if (mIsDsiCallUp) {
        LOC_LOGW("Already ongoing data call");
        if (mWwanCallStatusCb != NULL) {
            mWwanCallStatusCb(
                    mWwanCbUserDataPtr, LOC_NET_WWAN_CALL_EVT_OPEN_SUCCESS,
                    getApnNameFromConfig(), getIpTypeFromConfig());
        }
        return true;
    }

    /* Initialize DSI library */
    int ret = -1;
    if (!mIsDsiInitDone) {

        if ((ret = dsi_init(DSI_MODE_GENERAL)) == DSI_SUCCESS) {
            LOC_LOGI("dsi_init success !");
        } else if (ret == DSI_EINITED) {
            LOC_LOGI("dsi_init already done !");
        } else {
            LOC_LOGE("dsi_init failed, err %d", ret);
        }
        mIsDsiInitDone = true;

        /* Sleep 100 ms for dsi_init() to complete */
        LOC_LOGV("Sleeping for 100 ms");
        usleep(100 * 1000);
    }

    /* Get DSI service handle */
    if (mDsiHandle == NULL) {
        mDsiHandle = dsi_get_data_srvc_hndl(
                LocNetIface::dsiNetEventCallback, this);
        if (mDsiHandle == NULL) {
            LOC_LOGE("NULL DSI Handle");
            return false;
        }
    }
    LOC_LOGD("DSI Handle for call %p", mDsiHandle);

    /* Set call parameters */
    dsi_call_param_value_t callParams;

    /* No Radio tech preference */
    callParams.buf_val = NULL;
    callParams.num_val = DSI_RADIO_TECH_UNKNOWN;
    LOC_LOGD("DSI_CALL_INFO_TECH_PREF = DSI_RADIO_TECH_UNKNOWN");
    dsi_set_data_call_param(mDsiHandle, DSI_CALL_INFO_TECH_PREF, &callParams);

    /* APN from gps.conf */
    char* apnName = getApnNameFromConfig();
    int apnNameLen = strnlen(apnName, APN_NAME_MAX_LEN);
    if (apnName != NULL &&  apnNameLen > 0) {
        callParams.buf_val = apnName;
        callParams.num_val = apnNameLen;
        LOC_LOGD("DSI_CALL_INFO_APN_NAME = %s", apnName);
        dsi_set_data_call_param(mDsiHandle, DSI_CALL_INFO_APN_NAME, &callParams);
    } else{
        LOC_LOGE("Failed to fetch APN for data call setup");
        return false;
    }

    /* IP type from gps.conf */
    LocNetConnIpType ipType = getIpTypeFromConfig();
    callParams.buf_val = NULL;
    if (ipType == LOC_NET_CONN_IP_TYPE_V4) {
        callParams.num_val = DSI_IP_VERSION_4;
    } else if (ipType == LOC_NET_CONN_IP_TYPE_V6) {
        callParams.num_val = DSI_IP_VERSION_6;
    } else if (ipType == LOC_NET_CONN_IP_TYPE_V4V6) {
        callParams.num_val = DSI_IP_VERSION_4_6;
    } else {
        LOC_LOGE("No IP Type in gps.conf, using default v4");
        callParams.num_val = DSI_IP_VERSION_4;
    }
    dsi_set_data_call_param(
            mDsiHandle, DSI_CALL_INFO_IP_VERSION, &callParams);

    /* Send the call setup request */
    ret = dsi_start_data_call(mDsiHandle);
    if (ret != DSI_SUCCESS) {

        LOC_LOGE("DSI_START_DATA_CALL FAILED, err %d", ret);
        return false;
    }

    mIsDsiStartCallPending = true;
    LOC_LOGI("Data call START request sent successfully to DSI");
    return true;
}

bool LocNetIface::stopWwanCall () {

    ENTRY_LOG();

    /* Check for ongoing start/stop attempts */
    if (mIsDsiStopCallPending) {
        LOC_LOGW("Already stop pending, no-op");
        return true;
    }
    if (mIsDsiStartCallPending) {
        LOC_LOGE("Start attempt pending, can't stop now !");
        /* When start completes and DS callback is received, we will
         * notify the client. So no need to notify now. */
        return false;
    }
    if (!mIsDsiCallUp) {
        LOC_LOGE("No ongoing data call to stop");
        if (mWwanCallStatusCb != NULL) {
            mWwanCallStatusCb(
                    mWwanCbUserDataPtr, LOC_NET_WWAN_CALL_EVT_CLOSE_SUCCESS,
                    getApnNameFromConfig(), getIpTypeFromConfig());
        }
        return true;
    }

    /* Stop the call */
    LOC_LOGD("Stopping data call with handle %p", mDsiHandle);

    int ret = dsi_stop_data_call(mDsiHandle);
    if (ret != DSI_SUCCESS) {

        LOC_LOGE("dsi_stop_data_call() returned err %d", ret);
        return false;
    }

    mIsDsiStopCallPending = true;
    LOC_LOGI("Data call STOP request sent to DS");
    return true;
}

/* Static callback method */
void LocNetIface::dsiNetEventCallback (
        dsi_hndl_t dsiHandle, void* userDataPtr, dsi_net_evt_t event,
        dsi_evt_payload_t* eventPayloadPtr){

    ENTRY_LOG();

    /* Analyze event payload */
    LocNetIface* locNetIface = static_cast<LocNetIface*>(userDataPtr);
    if (locNetIface == NULL){
        LOC_LOGE("Null user data !");
        return;
    }

    if (event == DSI_EVT_NET_IS_CONN){
        LOC_LOGI("DSI_EVT_NET_IS_CONN");
        locNetIface->handleDSCallback(dsiHandle, true);
    } else if (event == DSI_EVT_NET_NO_NET){
        LOC_LOGI("DSI_EVT_NET_NO_NET");
        locNetIface->handleDSCallback(dsiHandle, false);
    } else {
        LOC_LOGW("Unsupported event %d", event);
    }
}

void LocNetIface::handleDSCallback (
        dsi_hndl_t dsiHandle, bool isNetConnected){

    ENTRY_LOG();
    LOC_LOGV("dsiHandle %p, isCallUp %d, stopPending %d, startPending %d",
              dsiHandle, mIsDsiCallUp, mIsDsiStopCallPending,
              mIsDsiStartCallPending);

    /* Validate handle */
    if (mDsiHandle != dsiHandle){
        LOC_LOGE("DS Handle mismatch: %p vs %p", mDsiHandle, dsiHandle);
        return;
    }

    /* Process event */
    if (isNetConnected){

        /* Invoke client callback if registered*/
        if (mIsDsiStartCallPending &&
                mWwanCallStatusCb != NULL){
            LOC_LOGV("LOC_NET_WWAN_CALL_EVT_OPEN_SUCCESS");
            mWwanCallStatusCb(
                    mWwanCbUserDataPtr, LOC_NET_WWAN_CALL_EVT_OPEN_SUCCESS,
                    getApnNameFromConfig(), getIpTypeFromConfig());
        }

        /* Start call complete */
        mIsDsiCallUp = true;
        mIsDsiStartCallPending = false;

    } else {

        /* Invoke client callback if registered */
        if (mIsDsiStopCallPending &&
                mWwanCallStatusCb != NULL) {
            LOC_LOGV("LOC_NET_WWAN_CALL_EVT_CLOSE_SUCCESS");
            mWwanCallStatusCb(
                    mWwanCbUserDataPtr, LOC_NET_WWAN_CALL_EVT_CLOSE_SUCCESS,
                    getApnNameFromConfig(), getIpTypeFromConfig());
        } else if (mIsDsiStartCallPending &&
                mWwanCallStatusCb != NULL){
            LOC_LOGV("LOC_NET_WWAN_CALL_EVT_OPEN_FAILED");
            mWwanCallStatusCb(
                    mWwanCbUserDataPtr, LOC_NET_WWAN_CALL_EVT_OPEN_FAILED, NULL,
                    LOC_NET_CONN_IP_TYPE_INVALID);
        }

        /* Stop call complete */
        mIsDsiCallUp = false;
        mIsDsiStopCallPending = false;
    }
}

bool LocNetIface::isWlanConnected() {

    ENTRY_LOG();

    /* Access QCMAP instance only from the static instance */
    if (this != LocNetIface::sLocNetIfaceInstance &&
            LocNetIface::sLocNetIfaceInstance != NULL) {
        return LocNetIface::sLocNetIfaceInstance->isWlanConnected();
    }

    /* Validate QCMAP Client instance */
    if (mQcmapClientPtr == NULL) {
        LOC_LOGE("No QCMAP instance !");
        return false;
    }

    /* Fetch station mode status */
    qcmap_msgr_station_mode_status_enum_v01 status =
            QCMAP_MSGR_STATION_MODE_STATUS_ENUM_MIN_ENUM_VAL_V01;
    qmi_error_type_v01 qmi_err_num = QMI_ERROR_TYPE_MIN_ENUM_VAL_V01;

    if (!mQcmapClientPtr->GetStationModeStatus(&status, &qmi_err_num)) {
        LOC_LOGE("Failed to fetch station mode status, err %d", qmi_err_num);
        return false;
    }

    /* Notify observers */
    if (status == QCMAP_MSGR_STATION_MODE_CONNECTED_V01) {
        LOC_LOGV("WLAN is connected.");
        return true;
    } else if (status == QCMAP_MSGR_STATION_MODE_DISCONNECTED_V01) {
        LOC_LOGV("WLAN is disconnected.");
        return false;
    } else {
        LOC_LOGE("Unhandled station mode status %d", status);
    }

    return false;
}

bool LocNetIface::isWwanConnected() {

    ENTRY_LOG();

    /* Access QCMAP instance only from the static instance */
    if (this != LocNetIface::sLocNetIfaceInstance &&
            LocNetIface::sLocNetIfaceInstance != NULL) {
        return LocNetIface::sLocNetIfaceInstance->isWwanConnected();
    }

    /* Validate QCMAP Client instance */
    if (mQcmapClientPtr == NULL) {
        LOC_LOGE("No QCMAP instance !");
        return false;
    }

    /* Fetch backhaul status */
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;
    qcmap_msgr_wwan_status_enum_v01 v4_status, v6_status;
    if (mQcmapClientPtr->GetWWANStatus(
            &v4_status, &v6_status, &qmi_err_num) == false) {
        LOC_LOGE("Failed to get wwan status, err 0x%x", qmi_err_num);
        return false;
    }
    if (v4_status == QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01) {
        LOC_LOGV("WWAN is connected.");
        return true;
    } else if (v4_status == QCMAP_MSGR_WWAN_STATUS_DISCONNECTED_V01) {
        LOC_LOGV("WWAN is disconnected.");
        return false;
    } else {
        LOC_LOGE("Unhandled wwan status %d", v4_status);
    }

    return false;
}

bool LocNetIface::connectBackhaul() {

    ENTRY_LOG();

    /* Access QCMAP instance only from the static instance */
    if (this != LocNetIface::sLocNetIfaceInstance &&
            LocNetIface::sLocNetIfaceInstance != NULL) {
        LOC_LOGV("Invoke from static LocNetIface instance..");
        if (mWwanCallStatusCb != NULL) {
            LocNetIface::sLocNetIfaceInstance->
            registerWwanCallStatusCallback(
                    mWwanCallStatusCb, mWwanCbUserDataPtr);
        }
        return LocNetIface::sLocNetIfaceInstance->connectBackhaul();
    }

    /* QCMAP client instance must have been created.
     * Happens when some client subscribes. */
    if (mQcmapClientPtr == NULL) {
        LOC_LOGE("No QCMAP instance");
        return false;
    }

    /* Check if backhaul is already connected */
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;
    qcmap_msgr_wwan_status_enum_v01 v4_status, v6_status;
    if (mQcmapClientPtr->GetWWANStatus(
            &v4_status, &v6_status, &qmi_err_num) == false) {
        LOC_LOGE("Failed to get wwan status, err 0x%x", qmi_err_num);
        return false;
    }
    if (v4_status == QCMAP_MSGR_WWAN_STATUS_CONNECTING_V01) {
        LOC_LOGI("Connection attempt already ongoing.");
        return true;
    }
    if (v4_status == QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01) {
        LOC_LOGV("Backhaul already connected !");
        if (mWwanCallStatusCb != NULL) {
            mWwanCallStatusCb(
                    mWwanCbUserDataPtr, LOC_NET_WWAN_CALL_EVT_OPEN_SUCCESS,
                    getApnNameFromConfig(), getIpTypeFromConfig());
        }
        return true;
    }

    /* Check if we've already sent the request */
    if (mConnectBackhaulSent) {
        LOC_LOGE("Connect request already sent !");
        return true;
    }

    /* Send connect request to QCMAP */
    qmi_err_num = QMI_ERR_NONE_V01;
    LOC_LOGV("Sending ConnectBackhaul request..");
    if (mQcmapClientPtr->ConnectBackHaul(
            QCMAP_MSGR_WWAN_CALL_TYPE_V4_V01, &qmi_err_num) == false) {
        LOC_LOGE("Connect backhaul failed, err 0x%x", qmi_err_num);
        return false;
    }

    /* Set the flag to track */
    mConnectBackhaulSent = true;
    mIsConnectBackhaulPending = true;
    return true;
}

bool LocNetIface::disconnectBackhaul() {

    ENTRY_LOG();

    /* Access QCMAP instance only from the static instance */
    if (this != LocNetIface::sLocNetIfaceInstance &&
            LocNetIface::sLocNetIfaceInstance != NULL) {
        return LocNetIface::sLocNetIfaceInstance->disconnectBackhaul();
    }

    /* QCMAP client instance must have been created.
     * Happens when some client subscribes. */
    if (mQcmapClientPtr == NULL) {
        LOC_LOGE("No QCMAP instance");
        return false;
    }

    /* Check if we've sent the request.
     * If we didn't send the connect request, no need to disconnect */
    if (!mConnectBackhaulSent) {
        LOC_LOGE("No connect req from us, ignore disconnect req");
        if (mWwanCallStatusCb != NULL) {
            mWwanCallStatusCb(
                    mWwanCbUserDataPtr, LOC_NET_WWAN_CALL_EVT_CLOSE_SUCCESS,
                    getApnNameFromConfig(), getIpTypeFromConfig());
        }
        return true;
    }

    /* Send disconnect request to QCMAP */
    qmi_error_type_v01 qmi_err_num = QMI_ERR_NONE_V01;
    LOC_LOGV("Sending DisconnectBackhaul..");
    if (mQcmapClientPtr->DisconnectBackHaul(
            QCMAP_MSGR_WWAN_CALL_TYPE_V4_V01, &qmi_err_num) == false) {
        LOC_LOGE("Disconnect backhaul failed, err 0x%x", qmi_err_num);
        return false;
    }

    /* Set the flag to track */
    mConnectBackhaulSent = false;
    mIsDisconnectBackhaulPending = true;
    return true;
}
