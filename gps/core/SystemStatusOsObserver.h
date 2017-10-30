/* Copyright (c) 2015-2017, The Linux Foundation. All rights reserved.
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
 *
 */
#ifndef __SYSTEM_STATUS_OSOBSERVER__
#define __SYSTEM_STATUS_OSOBSERVER__

#include <stdint.h>
#include <string>
#include <list>
#include <map>
#include <new>
#include <vector>
#include <platform_lib_log_util.h>
#include <DataItemId.h>
#include <MsgTask.h>
#include <IOsObserver.h>

namespace loc_core
{

/******************************************************************************
 SystemStatusOsObserver
******************************************************************************/
// Forward Declarations
class IDataItemCore;

template <typename CT, typename DIT>
class IClientIndex;

template <typename CT, typename DIT>
class IDataItemIndex;

struct SystemContext {
    IDataItemSubscription *mSubscriptionObj;
    IFrameworkActionReq *mFrameworkActionReqObj;
    const MsgTask *mMsgTask;

    inline SystemContext() :
        mSubscriptionObj(NULL),
        mFrameworkActionReqObj(NULL),
        mMsgTask(NULL) {}
};

// Clients wanting to get data from OS/Framework would need to
// subscribe with OSObserver using IDataItemSubscription interface.
// Such clients would need to implement IDataItemObserver interface
// to receive data when it becomes available.
class SystemStatusOsObserver : public IOsObserver {

public:
    // ctor
    SystemStatusOsObserver(const MsgTask* msgTask);

    // dtor
    ~SystemStatusOsObserver();

    // To set the subscription object
    inline void setSubscriptionObj(IDataItemSubscription *subscriptionObj) {
        mContext.mSubscriptionObj = subscriptionObj;
    };

    // To set the framework action request object
    inline void setFrameworkActionReqObj(IFrameworkActionReq *frameworkActionReqObj) {
        mContext.mFrameworkActionReqObj = frameworkActionReqObj;
    }

    // IDataItemObserver Overrides
    virtual void getName (string & name);
    virtual void notify (const std::list <IDataItemCore *> & dlist);

    // IDataItemSubscription Overrides
    virtual void subscribe (const std :: list <DataItemId> & l, IDataItemObserver * client);
    virtual void updateSubscription
    (
        const std :: list <DataItemId> & l,
        IDataItemObserver * client
    );
    virtual void requestData
    (
        const std :: list <DataItemId> & l,
        IDataItemObserver * client
    );
    virtual void unsubscribe (const std :: list <DataItemId> & l, IDataItemObserver * client);
    virtual void unsubscribeAll (IDataItemObserver * client);

    // IFrameworkActionReq Overrides
    virtual void turnOn (DataItemId dit, int timeOut = 0);
    virtual void turnOff (DataItemId dit);

private:

    SystemContext                                        mContext;
    const string                                         mAddress;
    IClientIndex <IDataItemObserver *, DataItemId>      *mClientIndex;
    IDataItemIndex <IDataItemObserver *, DataItemId>    *mDataItemIndex;
    map < DataItemId, IDataItemCore * >                  mDataItemCache;
    map < DataItemId, int >                              mActiveRequestCount;

    // Nested types
    // Messages
    struct HandleMsgBase : public LocMsg {
        HandleMsgBase (SystemStatusOsObserver * parent);
        virtual ~HandleMsgBase ();
        // Data members
        SystemStatusOsObserver * mParent;
    };

    // Helpers
    int sendFirstResponse
    (
        const list <DataItemId> & l,
        IDataItemObserver * to
    );

    int sendCachedDataItems
    (
        const list <DataItemId> & l,
        IDataItemObserver * to
    );

    int updateCache (IDataItemCore * d, bool &dataItemUpdated);
    void logMe (const list <DataItemId> & l);

    // Messages
    struct HandleClientMsg : public LocMsg {
        HandleClientMsg (SystemStatusOsObserver * parent, IDataItemObserver * client);
        virtual ~HandleClientMsg ();
        // Data Members
        SystemStatusOsObserver * mParent;
        IDataItemObserver * mClient;
    };

    struct HandleSubscribeReq : public HandleClientMsg  {
        HandleSubscribeReq (SystemStatusOsObserver * parent,
                           const list <DataItemId> & l,
                           IDataItemObserver * client);
        virtual ~HandleSubscribeReq ();
        void proc () const;
        // Data members
        const list <DataItemId> mDataItemList;
    };

    struct HandleUpdateSubscriptionReq : public HandleClientMsg  {
        HandleUpdateSubscriptionReq (SystemStatusOsObserver * parent,
                                    const list <DataItemId> & l,
                                    IDataItemObserver * client);
        virtual ~HandleUpdateSubscriptionReq ();
        void proc () const;
        // Data members
        const list <DataItemId> mDataItemList;
    };

    struct HandleRequestData : public HandleClientMsg {
       HandleRequestData (SystemStatusOsObserver * parent,
                          const list <DataItemId> & l,
                          IDataItemObserver * client);
       virtual ~HandleRequestData ();
       void proc () const;
       const list <DataItemId> mDataItemList;
    };

    struct HandleUnsubscribeReq : public HandleClientMsg  {
        HandleUnsubscribeReq (SystemStatusOsObserver * parent,
                             const list <DataItemId> & l,
                             IDataItemObserver * client);
        virtual ~HandleUnsubscribeReq ();
        void proc () const;
        // Data members
        const list <DataItemId> mDataItemList;
    };

    struct HandleUnsubscribeAllReq : public HandleClientMsg  {
        HandleUnsubscribeAllReq
        (
            SystemStatusOsObserver * parent,
            IDataItemObserver * client
        );
        virtual ~HandleUnsubscribeAllReq ();
        void proc () const;
    };

    struct HandleNotify : public HandleMsgBase {
        HandleNotify (SystemStatusOsObserver * parent, list <IDataItemCore *> dlist);
        virtual ~HandleNotify ();
        void getListOfClients
        (
            const list <DataItemId> & dlist,
            list <IDataItemObserver *> & clients
        ) const;
        void proc () const;
        // Data members
        list <IDataItemCore *> mDList;
    };

    struct HandleTurnOn : public HandleMsgBase  {
        HandleTurnOn (SystemStatusOsObserver * parent,
                          const DataItemId dit,
                          const int timeOut);
        virtual ~HandleTurnOn ();
        void proc () const;
        // Data members
        DataItemId mDataItemId;
        int mTimeOut;
    };

    struct HandleTurnOff : public HandleMsgBase  {
        HandleTurnOff (SystemStatusOsObserver * parent,const DataItemId dit);
        virtual ~HandleTurnOff ();
        void proc () const;
        // Data members
        DataItemId mDataItemId;
    };

};


/******************************************************************************
 Messages
******************************************************************************/
// Ctors
inline SystemStatusOsObserver :: HandleMsgBase :: HandleMsgBase (SystemStatusOsObserver * parent)
:
mParent (parent)
{}

inline SystemStatusOsObserver :: HandleClientMsg :: HandleClientMsg
(
    SystemStatusOsObserver * parent,
    IDataItemObserver * client
)
:
mParent (parent),
mClient (client)
{}

inline SystemStatusOsObserver :: HandleSubscribeReq :: HandleSubscribeReq
 (SystemStatusOsObserver * parent, const list <DataItemId> & l, IDataItemObserver * client)
:
HandleClientMsg (parent, client), mDataItemList (l)
{}

inline SystemStatusOsObserver :: HandleUpdateSubscriptionReq :: HandleUpdateSubscriptionReq
 (SystemStatusOsObserver * parent, const list <DataItemId> & l, IDataItemObserver * client)
:
HandleClientMsg (parent, client), mDataItemList (l)
{}

inline SystemStatusOsObserver :: HandleRequestData :: HandleRequestData
 (SystemStatusOsObserver * parent, const list <DataItemId> & l, IDataItemObserver * client)
:
HandleClientMsg (parent, client), mDataItemList (l)
{}

inline SystemStatusOsObserver :: HandleUnsubscribeReq :: HandleUnsubscribeReq
 (SystemStatusOsObserver * parent, const list <DataItemId> & l, IDataItemObserver * client)
:
HandleClientMsg (parent, client), mDataItemList (l)
{}

inline SystemStatusOsObserver :: HandleUnsubscribeAllReq :: HandleUnsubscribeAllReq
 (SystemStatusOsObserver * parent, IDataItemObserver * client)
:
HandleClientMsg (parent, client)
{}

inline SystemStatusOsObserver :: HandleNotify :: HandleNotify
 (SystemStatusOsObserver * parent, list <IDataItemCore *> dlist)
:
HandleMsgBase (parent), mDList (dlist)
{}

inline SystemStatusOsObserver :: HandleTurnOn :: HandleTurnOn
 (SystemStatusOsObserver * parent, const DataItemId dit,const int timeOut)
:
HandleMsgBase (parent), mDataItemId (dit), mTimeOut (timeOut)
{}

inline SystemStatusOsObserver :: HandleTurnOff :: HandleTurnOff
 (SystemStatusOsObserver * parent, const DataItemId dit)
:
HandleMsgBase (parent), mDataItemId (dit)
{}

// Dtors
inline SystemStatusOsObserver :: HandleMsgBase :: ~HandleMsgBase () {}
inline SystemStatusOsObserver :: HandleClientMsg :: ~HandleClientMsg () {}
inline SystemStatusOsObserver :: HandleSubscribeReq :: ~HandleSubscribeReq () {}
inline SystemStatusOsObserver :: HandleUpdateSubscriptionReq :: ~HandleUpdateSubscriptionReq() {}
inline SystemStatusOsObserver :: HandleRequestData :: ~HandleRequestData() {}
inline SystemStatusOsObserver :: HandleUnsubscribeReq :: ~HandleUnsubscribeReq () {}
inline SystemStatusOsObserver :: HandleUnsubscribeAllReq :: ~HandleUnsubscribeAllReq () {}

inline SystemStatusOsObserver :: HandleNotify :: ~HandleNotify () {
    list <IDataItemCore *> :: iterator it = mDList.begin ();
    for (; it != mDList.end (); ++it) {
        delete *it;
        *it = NULL;
    }
}

inline SystemStatusOsObserver :: HandleTurnOn :: ~HandleTurnOn () {}
inline SystemStatusOsObserver :: HandleTurnOff :: ~HandleTurnOff () {}


} // namespace loc_core

#endif //__SYSTEM_STATUS__

