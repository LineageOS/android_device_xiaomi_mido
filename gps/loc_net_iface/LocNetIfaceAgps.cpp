
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
#define LOG_TAG "LocSvc_LocNetIfaceHolder"

#include <LocNetIfaceAgps.h>
#include <platform_lib_log_util.h>

/* LocNetIfaceAgps members */
LocNetIface* LocNetIfaceAgps::sLocNetIfaceAgpsInternet = NULL;
LocNetIface* LocNetIfaceAgps::sLocNetIfaceAgpsSupl = NULL;
LocAgpsOpenResultCb LocNetIfaceAgps::sAgpsOpenResultCb = NULL;
LocAgpsCloseResultCb LocNetIfaceAgps::sAgpsCloseResultCb = NULL;
void* LocNetIfaceAgps::sUserDataPtr = NULL;

/* Method accessed from HAL */
agps_status_extended LocNetIfaceAgps_getStatusCb(
        LocAgpsOpenResultCb openResultCb,
        LocAgpsCloseResultCb closeResultCb, void* userDataPtr) {

    ENTRY_LOG();

    /* Save callbacks and userDataPtr */
    LocNetIfaceAgps::sAgpsOpenResultCb = openResultCb;
    LocNetIfaceAgps::sAgpsCloseResultCb = closeResultCb;
    LocNetIfaceAgps::sUserDataPtr = userDataPtr;

    /* Create LocNetIface instances */
    if (LocNetIfaceAgps::sLocNetIfaceAgpsInternet == NULL) {
        LocNetIfaceAgps::sLocNetIfaceAgpsInternet =
                new LocNetIface(LOC_NET_CONN_TYPE_WWAN_INTERNET);
        LocNetIfaceAgps::sLocNetIfaceAgpsInternet->
        registerWwanCallStatusCallback(
                LocNetIfaceAgps::wwanStatusCallback,
                LocNetIfaceAgps::sLocNetIfaceAgpsInternet);
    } else {
        LOC_LOGE("sLocNetIfaceAgpsInternet not NULL");
    }

    if (LocNetIfaceAgps::sLocNetIfaceAgpsSupl == NULL) {
        LocNetIfaceAgps::sLocNetIfaceAgpsSupl =
                new LocNetIface(LOC_NET_CONN_TYPE_WWAN_SUPL);
        LocNetIfaceAgps::sLocNetIfaceAgpsSupl->
        registerWwanCallStatusCallback(
                LocNetIfaceAgps::wwanStatusCallback,
                LocNetIfaceAgps::sLocNetIfaceAgpsSupl);
    } else {
        LOC_LOGE("sLocNetIfaceAgpsSupl not NULL");
    }

    /* Return our callback */
    return LocNetIfaceAgps::agpsStatusCb;
}

void LocNetIfaceAgps::agpsStatusCb(AGpsExtStatus* status){

    ENTRY_LOG();

    /* Validate */
    if (sLocNetIfaceAgpsInternet == NULL) {
        LOC_LOGE("Not init'd");
        return;
    }
    if (status == NULL) {
        LOC_LOGE("NULL status");
        return;
    }

    if (status->status == LOC_GPS_REQUEST_AGPS_DATA_CONN) {

        if (status->type == LOC_AGPS_TYPE_SUPL) {

            LOC_LOGV("REQUEST LOC_AGPS_TYPE_SUPL");
            if (!sLocNetIfaceAgpsSupl->setupWwanCall()) {
                LOC_LOGE("Setup wwan call failed !");
                wwanStatusCallback(
                        sLocNetIfaceAgpsSupl,
                        LOC_NET_WWAN_CALL_EVT_OPEN_FAILED,
                        NULL, LOC_NET_CONN_IP_TYPE_INVALID);
            }
        } else if (status->type == LOC_AGPS_TYPE_WWAN_ANY) {

            LOC_LOGV("REQUEST LOC_AGPS_TYPE_WWAN_ANY");
            if (!sLocNetIfaceAgpsInternet->connectBackhaul()) {
                LOC_LOGE("Connect Backhaul failed");
                wwanStatusCallback(
                        sLocNetIfaceAgpsInternet,
                        LOC_NET_WWAN_CALL_EVT_OPEN_FAILED,
                        NULL, LOC_NET_CONN_IP_TYPE_INVALID);
            }
        } else {

            LOC_LOGE("Unsupported AGPS type %d", status->type);
        }
    }
    else if (status->status == LOC_GPS_RELEASE_AGPS_DATA_CONN) {

        if (status->type == LOC_AGPS_TYPE_SUPL) {

            LOC_LOGV("RELEASE LOC_AGPS_TYPE_SUPL");
            if (!sLocNetIfaceAgpsSupl->stopWwanCall()) {
                LOC_LOGE("Stop wwan call failed !");
                wwanStatusCallback(
                        sLocNetIfaceAgpsSupl,
                        LOC_NET_WWAN_CALL_EVT_CLOSE_FAILED,
                        NULL, LOC_NET_CONN_IP_TYPE_INVALID);
            }
        } else if (status->type == LOC_AGPS_TYPE_WWAN_ANY) {

            LOC_LOGV("RELEASE LOC_AGPS_TYPE_WWAN_ANY");
            if (!sLocNetIfaceAgpsInternet->disconnectBackhaul()) {
                LOC_LOGE("Disconnect backhaul failed !");
                wwanStatusCallback(
                        sLocNetIfaceAgpsInternet,
                        LOC_NET_WWAN_CALL_EVT_CLOSE_FAILED,
                        NULL, LOC_NET_CONN_IP_TYPE_INVALID);
            }
        } else {

            LOC_LOGE("Unsupported AGPS type %d", status->type);
        }
    }
    else {
        LOC_LOGE("Unsupported AGPS action %d", status->status);
    }
}

void LocNetIfaceAgps::wwanStatusCallback(
            void* userDataPtr, LocNetWwanCallEvent event,
            const char* apn, LocNetConnIpType apnIpType){

    ENTRY_LOG();
    LOC_LOGV("event: %d, apnIpType: %d", event, apnIpType);

    /* Derive bearer type */
    AGpsBearerType bearerType = AGPS_APN_BEARER_INVALID;
    switch (apnIpType) {
        case LOC_NET_CONN_IP_TYPE_V4:
            bearerType = AGPS_APN_BEARER_IPV4;
            break;
        case LOC_NET_CONN_IP_TYPE_V6:
            bearerType = AGPS_APN_BEARER_IPV6;
            break;
        case LOC_NET_CONN_IP_TYPE_V4V6:
            bearerType = AGPS_APN_BEARER_IPV4V6;
            break;
        default:
            LOC_LOGE("Invalid APN IP type %d", apnIpType);
    }

    /* Derive AGPS type */
    AGpsExtType agpsType = LOC_AGPS_TYPE_INVALID;
    if (userDataPtr == (void*)sLocNetIfaceAgpsInternet) {
        agpsType = LOC_AGPS_TYPE_WWAN_ANY;
    }
    else if (userDataPtr == (void*)sLocNetIfaceAgpsSupl) {
        agpsType = LOC_AGPS_TYPE_SUPL;
    }
    else {
        LOC_LOGE("Invalid user data ptr %p", userDataPtr);
        return;
    }

    /* Complete AGPS call flow */
    if (event == LOC_NET_WWAN_CALL_EVT_OPEN_SUCCESS) {
        LOC_LOGV("LOC_NET_WWAN_CALL_EVT_OPEN_SUCCESS");
        sAgpsOpenResultCb(
                true, agpsType, apn, bearerType, sUserDataPtr);
    }
    else if (event == LOC_NET_WWAN_CALL_EVT_OPEN_FAILED) {
        LOC_LOGE("LOC_NET_WWAN_CALL_EVT_OPEN_FAILED");
        sAgpsOpenResultCb(
                false, agpsType, apn, bearerType, sUserDataPtr);
    }
    else if (event == LOC_NET_WWAN_CALL_EVT_CLOSE_SUCCESS) {
        LOC_LOGV("LOC_NET_WWAN_CALL_EVT_CLOSE_SUCCESS");
        sAgpsCloseResultCb(true, agpsType, sUserDataPtr);
    }
    else if (event == LOC_NET_WWAN_CALL_EVT_CLOSE_FAILED) {
        LOC_LOGE("LOC_NET_WWAN_CALL_EVT_CLOSE_FAILED");
        sAgpsCloseResultCb(false, agpsType, sUserDataPtr);
    }
    else {
        LOC_LOGE("Unsupported event %d", event);
    }
}
