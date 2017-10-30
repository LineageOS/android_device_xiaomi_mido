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
#ifndef LOC_NET_IFACE_AGPS_H
#define LOC_NET_IFACE_AGPS_H

#include <LocNetIface.h>
#include <gps_extended_c.h>

/* these declarations will be moved inside api file */
/* Constructs for interaction with loc_net_iface library */
typedef void (*LocAgpsOpenResultCb)(
        bool isSuccess, AGpsExtType agpsType, const char* apn,
        AGpsBearerType bearerType, void* userDataPtr);

typedef void (*LocAgpsCloseResultCb)(
        bool isSuccess, AGpsExtType agpsType, void* userDataPtr);

/*--------------------------------------------------------------------
 * CLASS LocNetIfaceAgps
 *
 * Functionality:
 * This class holds reference to LocNetIface instances for AGPS
 *-------------------------------------------------------------------*/
class LocNetIfaceAgps {

public:
    /* status method registered as part of AGPS Extended callbacks */
    static void agpsStatusCb(AGpsExtStatus* status);

    /* Callbacks registered with Internet and SUPL LocNetIface instances */
    static void wwanStatusCallback(
            void* userDataPtr, LocNetWwanCallEvent event,
            const char* apn, LocNetConnIpType apnIpType);

    /* LocNetIface instances for different clients */
    static LocNetIface* sLocNetIfaceAgpsInternet;
    static LocNetIface* sLocNetIfaceAgpsSupl;

    /* AGPS interface methods to be invoked on call setup/failure */
    static LocAgpsOpenResultCb sAgpsOpenResultCb;
    static LocAgpsCloseResultCb sAgpsCloseResultCb;
    static void* sUserDataPtr;
};

/* Global method accessed from HAL to fetch AGPS status cb */
extern "C" agps_status_extended LocNetIfaceAgps_getStatusCb(
        LocAgpsOpenResultCb openResultCb,
        LocAgpsCloseResultCb closeResultCb, void* userDataPtr);

#endif /* #ifndef LOC_NET_IFACE_AGPS_H */
