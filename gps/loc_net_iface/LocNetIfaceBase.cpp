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
#define LOG_TAG "LocSvc_LocNetIfaceBase"

#include <LocNetIfaceBase.h>
#include <platform_lib_log_util.h>
#include <loc_cfg.h>

/* Data Item notify callback
 * Only one instance of LocNetIfaceBase can register this callback.
 * No support for multiple clients */
LocNetStatusChangeCb LocNetIfaceBase::sNotifyCb = NULL;
void* LocNetIfaceBase::sNotifyCbUserDataPtr = NULL;

void LocNetIfaceBase::registerWwanCallStatusCallback(
        LocWwanCallStatusCb wwanCallStatusCb, void* userDataPtr) {

    ENTRY_LOG();

    mWwanCallStatusCb = wwanCallStatusCb;
    mWwanCbUserDataPtr = userDataPtr;
}

void LocNetIfaceBase::registerDataItemNotifyCallback(
        LocNetStatusChangeCb callback, void* userDataPtr) {

    ENTRY_LOG();

    if (LocNetIfaceBase::sNotifyCb != NULL) {
        LOC_LOGE("Notify cb already registered !");
        return;
    }

    LocNetIfaceBase::sNotifyCb = callback;
    LocNetIfaceBase::sNotifyCbUserDataPtr = userDataPtr;
}

bool LocNetIfaceBase::updateSubscribedItemList(
        const std::list<DataItemId>& itemList, bool addOrDelete){

    ENTRY_LOG();
    bool anyUpdatesToList = false;

    /* Scroll through specified item list */
    std::list<DataItemId>::const_iterator it = itemList.begin();
    for (; it != itemList.end(); it++) {

        DataItemId itemId = *it;

        bool itemInSubscribedList = isItemSubscribed(itemId);

        /* Request to add */
        if (addOrDelete == true && !itemInSubscribedList) {

            mSubscribedItemList.push_back(itemId);
            anyUpdatesToList = true;

        } else if (addOrDelete == false && itemInSubscribedList) {
            /* Request to delete */
            mSubscribedItemList.erase(
                    std::remove(
                            mSubscribedItemList.begin(),
                            mSubscribedItemList.end(), itemId),
                            mSubscribedItemList.end());
            anyUpdatesToList = true;
        }
    }

    return anyUpdatesToList;
}

char* LocNetIfaceBase::getApnNameFromConfig(){

    return mApnName;
}

LocNetConnIpType LocNetIfaceBase::getIpTypeFromConfig(){

    /* Convert config value to LocNetConnIpType */
    if (mIpType == 4) {
        return LOC_NET_CONN_IP_TYPE_V4;
    } else if (mIpType == 6) {
        return LOC_NET_CONN_IP_TYPE_V6;
    } else if (mIpType == 10) {
        return LOC_NET_CONN_IP_TYPE_V4V6;
    }
    return LOC_NET_CONN_IP_TYPE_INVALID;
}

void LocNetIfaceBase::fetchConfigItems(){

    ENTRY_LOG();

    /* Fetch config items from gps.conf */
    if (mLocNetConnType == LOC_NET_CONN_TYPE_WWAN_INTERNET) {
        loc_param_s_type confItemsToFetchArray[] = {
                { "INTERNET_APN",     &mApnName, NULL, 's' },
                { "INTERNET_IP_TYPE", &mIpType,  NULL, 'n' } };
        UTIL_READ_CONF(LOC_PATH_GPS_CONF, confItemsToFetchArray);

    } else if (mLocNetConnType == LOC_NET_CONN_TYPE_WWAN_SUPL) {
        loc_param_s_type confItemsToFetchArray[] = {
                { "SUPL_APN",     &mApnName, NULL, 's' },
                { "SUPL_IP_TYPE", &mIpType,  NULL, 'n' } };
        UTIL_READ_CONF(LOC_PATH_GPS_CONF, confItemsToFetchArray);

    } else {
        LOC_LOGE("Invalid connType %d", mLocNetConnType);
    }
}
