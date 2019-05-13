/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "FingerprintHal"
#include <binder/IServiceManager.h>
#include <hardware/fingerprint.h>
#include <log/log.h>
#include <unistd.h>

#include "BiometricsFingerprint.h"
#include "FingerprintDaemonProxy.h"
#include "IFingerprintDaemon.h"

using namespace android;

static sp<IFingerprintDaemon> g_service = NULL;

class BinderDiednotify: public IBinder::DeathRecipient {
    public:
        void binderDied(const wp<IBinder> __unused &who) {
            ALOGE("binderDied");
            g_service = NULL;
        }
};

static sp<BinderDiednotify> gDeathRecipient = new BinderDiednotify();

fingerprint_device_t* getWrapperService(fingerprint_notify_t notify) {
    int64_t ret = 0;
    do {
        if (g_service == NULL) {
            sp<IServiceManager> sm = defaultServiceManager();
            sp<IBinder> binder = sm->getService(FingerprintDaemonProxy::descriptor);
            if (binder == NULL) {
                ALOGE("getService failed");
                sleep(1);
                continue;
            }
            g_service = interface_cast<IFingerprintDaemon>(binder);
            binder->linkToDeath(gDeathRecipient, NULL, 0);

            if (g_service != NULL) {
                ALOGE("getService succeed");

                g_service->init(notify);

                ret = g_service->openHal();
                if (ret == 0) {
                    ALOGE("getService openHal failed!");
                    g_service = NULL;
                }
            }
        }
    } while (0);

    return reinterpret_cast<fingerprint_device_t*>(ret);
}
