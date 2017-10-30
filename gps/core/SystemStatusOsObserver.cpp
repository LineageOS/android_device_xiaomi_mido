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
#define LOG_TAG "LocSvc_SystemStatusOsObserver"

#include <string>
#include <cinttypes>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <iterator>
#include <algorithm>

#include <MsgTask.h>
#include <SystemStatusOsObserver.h>

#include <DataItemId.h>
#include <IDataItemCore.h>
#include <IClientIndex.h>
#include <IDataItemIndex.h>
#include <IndexFactory.h>

#include <DataItemsFactoryProxy.h>

#include <platform_lib_log_util.h>

namespace loc_core
{
#define BREAK_IF_ZERO(ERR,X) if(0==(X)) {result = (ERR); break;}
#define BREAK_IF_NON_ZERO(ERR,X) if(0!=(X)) {result = (ERR); break;}

SystemStatusOsObserver::SystemStatusOsObserver(const MsgTask* msgTask) :
    mAddress ("SystemStatusOsObserver"),
    mClientIndex(IndexFactory <IDataItemObserver *, DataItemId> :: createClientIndex ()),
    mDataItemIndex(IndexFactory <IDataItemObserver *, DataItemId> :: createDataItemIndex ())
{
    int result = -1;
    ENTRY_LOG ();
    do {
        BREAK_IF_ZERO (1, mClientIndex);
        BREAK_IF_ZERO (2, mDataItemIndex);
        mContext.mMsgTask = msgTask;
        result = 0;
    } while (0);
    EXIT_LOG_WITH_ERROR ("%d",result);
}

SystemStatusOsObserver :: ~SystemStatusOsObserver ()
{
    // Close data-item library handle
    DataItemsFactoryProxy::closeDataItemLibraryHandle();

    // Destroy cache
    map <DataItemId, IDataItemCore *> :: iterator citer = mDataItemCache.begin ();
    for (; citer != mDataItemCache.end (); ++citer) {
        if (citer->second != NULL) { delete citer->second; }
    }
    mDataItemCache.clear ();
    delete mClientIndex;
    delete mDataItemIndex;
    mClientIndex = NULL;
    mDataItemIndex = NULL;
}

/******************************************************************************
 Message proc
******************************************************************************/
void SystemStatusOsObserver :: HandleSubscribeReq :: proc () const {

    int result = 0;
    ENTRY_LOG ();
    do {
        if (mDataItemList.empty ()) {
            LOC_LOGV("mDataItemList is empty. Nothing to do. Exiting");
            result = 0;
            break;
        }
        //mDataItemList.sort ();
        // Handle First Response
        list <DataItemId> pendingFirstResponseList;
        this->mParent->mClientIndex->add (this->mClient, mDataItemList, pendingFirstResponseList);

        // Do not send first response for only pendingFirstResponseList,
        // instead send for all the data items  (present in the cache) that
        // have been subscribed for each time.
        this->mParent->sendFirstResponse (mDataItemList, this->mClient);

        list <DataItemId> yetToSubscribeDataItemsList;
        this->mParent->mDataItemIndex->add (this->mClient, mDataItemList, yetToSubscribeDataItemsList);
        // Send subscription list to framework
        if (!yetToSubscribeDataItemsList.empty ()) {
            this->mParent->mContext.mSubscriptionObj->subscribe
            (
                yetToSubscribeDataItemsList,
                this->mParent
            );
            LOC_LOGD ("Subscribe Request sent to framework for the following data items");
            this->mParent->logMe (yetToSubscribeDataItemsList);
        }

    } while (0);
    EXIT_LOG_WITH_ERROR ("%d", result);
    return;
}

void SystemStatusOsObserver :: HandleUpdateSubscriptionReq :: proc () const {
    int result = 0;
    ENTRY_LOG ();
    do {
        if (mDataItemList.empty ()) {
            LOC_LOGV("mDataItemList is empty. Nothing to do. Exiting");
            result = 0;
            break;
        }
        //mDataItemList.sort ();
        list <DataItemId> currentlySubscribedList;
        this->mParent->mClientIndex->getSubscribedList (this->mClient, currentlySubscribedList);
        list <DataItemId> removeDataItemList;
        set_difference (currentlySubscribedList.begin (), currentlySubscribedList.end (),
                       mDataItemList.begin (), mDataItemList.end (),
                       inserter (removeDataItemList,removeDataItemList.begin ()));
        // Handle First Response
        list <DataItemId> pendingFirstResponseList;
        this->mParent->mClientIndex->add (this->mClient, mDataItemList, pendingFirstResponseList);
        // Send First Response
        this->mParent->sendFirstResponse (pendingFirstResponseList, this->mClient);

        list <DataItemId> yetToSubscribeDataItemsList;
        this->mParent->mDataItemIndex->add (this->mClient, mDataItemList, yetToSubscribeDataItemsList);
        // Send subscription list to framework
        if (!yetToSubscribeDataItemsList.empty ()) {
            this->mParent->mContext.mSubscriptionObj->subscribe
            (
                yetToSubscribeDataItemsList,
                this->mParent
            );
            LOC_LOGD ("Subscribe Request sent to framework for the following data items");
            this->mParent->logMe (yetToSubscribeDataItemsList);
        }

        list <DataItemId> unsubscribeList;
        list <DataItemId> unused;
        this->mParent->mClientIndex->remove (this->mClient, removeDataItemList, unused);

        if (!this->mParent->mClientIndex->isSubscribedClient (this->mClient)) {
            this->mParent->mDataItemIndex->remove (list <IDataItemObserver *> (1,this->mClient), unsubscribeList);
        }
        if (!unsubscribeList.empty ()) {
            // Send unsubscribe to framework
            this->mParent->mContext.mSubscriptionObj->unsubscribe
            (
                unsubscribeList,
                this->mParent
            );
            LOC_LOGD ("Unsubscribe Request sent to framework for the following data items");
            this->mParent->logMe (unsubscribeList);
        }
    } while (0);
    EXIT_LOG_WITH_ERROR ("%d",result);
}

void SystemStatusOsObserver :: HandleRequestData :: proc () const {
    int result = 0;
    ENTRY_LOG ();

    do {
        if (mDataItemList.empty ()) {
            LOC_LOGV("mDataItemList is empty. Nothing to do. Exiting");
            result = 0;
            break;
        }
        //mDataItemList.sort ();
        list <DataItemId> yetToSubscribeDataItemsList;
        this->mParent->mClientIndex->add (this->mClient, mDataItemList, yetToSubscribeDataItemsList);
        this->mParent->mDataItemIndex->add (this->mClient, mDataItemList, yetToSubscribeDataItemsList);
        // Send subscription list to framework
        if (!mDataItemList.empty ()) {
            this->mParent->mContext.mSubscriptionObj->requestData
            (
                mDataItemList,
                this->mParent
            );
            LOC_LOGD ("Subscribe Request sent to framework for the following data items");
            this->mParent->logMe (yetToSubscribeDataItemsList);
        }

    } while (0);
    EXIT_LOG_WITH_ERROR ("%d",result);
}

void SystemStatusOsObserver :: HandleUnsubscribeReq :: proc () const {
    int result = 0;
    ENTRY_LOG ();
    do {
        if (mDataItemList.empty ()) {
            LOC_LOGV("mDataItemList is empty. Nothing to do. Exiting");
            result = 0;
            break;
        }
        //mDataItemList.sort ();
        list <DataItemId> unsubscribeList;
        list <DataItemId> unused;
        this->mParent->mClientIndex->remove (this->mClient, mDataItemList, unused);

        list <DataItemId> :: const_iterator it = mDataItemList.begin ();
        for (; it != mDataItemList.end (); ++it) {
            list <IDataItemObserver *> clientListSubs;
            list <IDataItemObserver *> clientListOut;
            this->mParent->mDataItemIndex->remove ((*it),
                                list <IDataItemObserver *> (1,this->mClient), clientListOut);
            // check if there are any other subscribed client for this data item id
            this->mParent->mDataItemIndex->getListOfSubscribedClients ( (*it), clientListSubs);
            if (clientListSubs.empty())
            {
                LOC_LOGD ("Client list subscribed is empty for dataitem - %d",(*it));
                unsubscribeList.push_back((*it));
            }
        }
        if (!unsubscribeList.empty ()) {
            // Send unsubscribe to framework
            this->mParent->mContext.mSubscriptionObj->unsubscribe
            (
                unsubscribeList,
                this->mParent
            );
            LOC_LOGD ("Unsubscribe Request sent to framework for the following data items");
            this->mParent->logMe (unsubscribeList);
        }
    } while (0);
    EXIT_LOG_WITH_ERROR ("%d",result);
}

void SystemStatusOsObserver :: HandleUnsubscribeAllReq :: proc () const {
    int result = 0;
    ENTRY_LOG ();
    do {
        list <IDataItemObserver *> clients (1, this->mClient);
        list <DataItemId> unsubscribeList;
        BREAK_IF_NON_ZERO (2, this->mParent->mClientIndex->remove (this->mClient));


        this->mParent->mDataItemIndex->remove (clients, unsubscribeList);
        if (!unsubscribeList.empty ()) {
            // Send unsubscribe to framework
            this->mParent->mContext.mSubscriptionObj->unsubscribe
            (
                unsubscribeList,
                this->mParent
            );
            LOC_LOGD ("Unsubscribe Request sent to framework for the following data items");
            this->mParent->logMe (unsubscribeList);
        }
    } while (0);
    EXIT_LOG_WITH_ERROR ("%d",result);
}

void SystemStatusOsObserver :: HandleNotify :: getListOfClients
 (const list <DataItemId> & dlist, list <IDataItemObserver *> & clients ) const {

     list <DataItemId> :: const_iterator it = dlist.begin ();
     for (; it != dlist.end (); ++it) {
         list <IDataItemObserver *> clientList;
         this->mParent->mDataItemIndex->getListOfSubscribedClients ( (*it), clientList);
         list <IDataItemObserver *> :: iterator citer = clientList.begin ();
         for (; citer != clientList.end (); ++citer) {
             clients.push_back (*citer);
         }
         clientList.clear ();
     }
     // remove duplicates
     clients.unique ();
}

void SystemStatusOsObserver :: HandleNotify :: proc () const {
    int result = 0;
    ENTRY_LOG ();
    do {
        // Update Cache with received data items and prepare
        // list of data items to be sent.
        list <IDataItemCore *> :: const_iterator it = mDList.begin ();
        list <DataItemId> dataItemIdsToBeSent;
        for (; it != mDList.end (); ++it) {
            bool dataItemUpdated = false;
            this->mParent->updateCache (*it, dataItemUpdated);
            if (dataItemUpdated) {
                dataItemIdsToBeSent.push_back ( (*it)->getId ());
            }
        }

        list <IDataItemObserver *> clientList;
        this->getListOfClients (dataItemIdsToBeSent, clientList);
        list <IDataItemObserver *> :: iterator citer = clientList.begin ();
        // Send data item to all subscribed clients
        LOC_LOGD ("LocTech-Label :: SystemStatusOsObserver :: Data Items Out");
        for (; citer != clientList.end (); ++citer) {
            do {
                list <DataItemId> dataItemIdsSubscribedByThisClient;
                list <DataItemId> dataItemIdsToBeSentForThisClient;
                this->mParent->mClientIndex->getSubscribedList (*citer, dataItemIdsSubscribedByThisClient);
                dataItemIdsSubscribedByThisClient.sort ();
                dataItemIdsToBeSent.sort ();
                set_intersection (dataItemIdsToBeSent.begin (),
                                 dataItemIdsToBeSent.end (),
                                 dataItemIdsSubscribedByThisClient.begin (),
                                 dataItemIdsSubscribedByThisClient.end (),
                                 inserter (dataItemIdsToBeSentForThisClient,
                                         dataItemIdsToBeSentForThisClient.begin ()));
                BREAK_IF_NON_ZERO (4,this->mParent->sendCachedDataItems (dataItemIdsToBeSentForThisClient, *citer));
                dataItemIdsSubscribedByThisClient.clear ();
                dataItemIdsToBeSentForThisClient.clear ();
            } while (0);
        }
    } while (0);
    EXIT_LOG_WITH_ERROR ("%d", result);
}

void SystemStatusOsObserver :: HandleTurnOn :: proc () const {
    int result = 0;
    ENTRY_LOG ();
    do {
        // Send action turn on to framework
        this->mParent->mContext.mFrameworkActionReqObj->turnOn(mDataItemId, mTimeOut);
    } while (0);
    EXIT_LOG_WITH_ERROR ("%d", result);
}

void SystemStatusOsObserver :: HandleTurnOff :: proc () const {
    int result = 0;
    ENTRY_LOG ();
    do {
        // Send action turn off to framework
        this->mParent->mContext.mFrameworkActionReqObj->turnOff(mDataItemId);
    } while (0);
    EXIT_LOG_WITH_ERROR ("%d", result);
}

/******************************************************************************
 IDataItemSubscription Overrides
******************************************************************************/
void SystemStatusOsObserver :: subscribe (const list <DataItemId> & l, IDataItemObserver * client) {
    int result = 0;
    ENTRY_LOG ();
    do {
        if (mContext.mSubscriptionObj != NULL) {
            HandleSubscribeReq * msg = new  (nothrow) HandleSubscribeReq (this, l, client);
            mContext.mMsgTask->sendMsg (msg);
        }
        else {
            LOC_LOGE("%s:%d]: Subscription object is NULL", __func__, __LINE__);
            result = 1;
        }
    } while (0);
    EXIT_LOG_WITH_ERROR ("%d",result);
}

void SystemStatusOsObserver :: updateSubscription (const list <DataItemId> & l, IDataItemObserver * client) {
    int result = 0;
    ENTRY_LOG ();
    do {
        if (mContext.mSubscriptionObj != NULL) {
            mContext.mMsgTask->sendMsg (new  (nothrow) HandleUpdateSubscriptionReq (this, l, client));
        }
        else {
            LOC_LOGE("%s:%d]: Subscription object is NULL", __func__, __LINE__);
            result = 1;
        }
    } while (0);
    EXIT_LOG_WITH_ERROR ("%d",result);
}

void SystemStatusOsObserver :: requestData (const list <DataItemId> & l, IDataItemObserver * client) {
    int result = 0;
    ENTRY_LOG ();
    do {
        if (mContext.mSubscriptionObj != NULL) {
            mContext.mMsgTask->sendMsg (new  (nothrow) HandleRequestData (this, l, client));
        }
        else {
            LOC_LOGE("%s:%d]: Subscription object is NULL", __func__, __LINE__);
            result = 1;
        }
    } while (0);

    EXIT_LOG_WITH_ERROR ("%d",result);
}

void SystemStatusOsObserver :: unsubscribe (const list <DataItemId> & l, IDataItemObserver * client) {
    int result = 0;
    ENTRY_LOG ();
    do {
        if (mContext.mSubscriptionObj != NULL) {
            mContext.mMsgTask->sendMsg (new  (nothrow) HandleUnsubscribeReq (this, l, client));
        }
        else {
            LOC_LOGE("%s:%d]: Subscription object is NULL", __func__, __LINE__);
            result = 1;
        }
    } while (0);
    EXIT_LOG_WITH_ERROR ("%d",result);
}

void SystemStatusOsObserver :: unsubscribeAll (IDataItemObserver * client) {
    int result = 0;
    ENTRY_LOG ();
    do {
        if (mContext.mSubscriptionObj != NULL) {
            mContext.mMsgTask->sendMsg (new  (nothrow) HandleUnsubscribeAllReq (this, client));
        }
        else {
            LOC_LOGE("%s:%d]: Subscription object is NULL", __func__, __LINE__);
            result = 1;
        }
    } while (0);
    EXIT_LOG_WITH_ERROR ("%d",result);
}

/******************************************************************************
 IDataItemObserver Overrides
******************************************************************************/
void SystemStatusOsObserver::getName(string & name) {
    name = mAddress;
}

void SystemStatusOsObserver::notify(const std::list <IDataItemCore *> & dlist) {
    int result = 0;
    ENTRY_LOG ();
    do {
        list <IDataItemCore *> :: const_iterator it = dlist.begin ();
        list <IDataItemCore *> dataItemList;
        list <DataItemId> ids;
        LOC_LOGD("LocTech-Label :: SystemStatusOsObserver :: Data Items In");
        for (; it != dlist.end (); ++it) {
            if (*it != NULL) {
                string dv;
                (*it)->stringify(dv);
                LOC_LOGD("LocTech-Value :: Data Item Value: %s", dv.c_str ());
                IDataItemCore * dataitem = DataItemsFactoryProxy::createNewDataItem((*it)->getId());
                BREAK_IF_ZERO (2, dataitem);
                // Copy contents into the newly created data item
                dataitem->copy(*it);
                dataItemList.push_back(dataitem);
                ids.push_back((*it)->getId());
            }
        }
        mContext.mMsgTask->sendMsg(new (nothrow) HandleNotify (this, dataItemList));
    } while  (0);
    EXIT_LOG_WITH_ERROR ("%d", result);
}

/******************************************************************************
 IFrameworkActionReq Overrides
******************************************************************************/
void SystemStatusOsObserver :: turnOn (DataItemId dit, int timeOut) {
    int result = 0;
    ENTRY_LOG ();
    do {
        if (mContext.mFrameworkActionReqObj != NULL) {
            // Check if data item exists in mActiveRequestCount
            map <DataItemId, int> :: iterator citer = mActiveRequestCount.find (dit);
            if (citer == mActiveRequestCount.end ()) {
                // Data item not found in map
                // Add reference count as 1 and add dataitem to map
                pair <DataItemId, int> cpair (dit, 1);
                mActiveRequestCount.insert (cpair);
                LOC_LOGD("Sending turnOn request");
                // Send action turn on to framework
                mContext.mMsgTask->sendMsg (new  (nothrow) HandleTurnOn (this, dit, timeOut));
            } else {
                // Found in map, update reference count
                citer->second++;
                LOC_LOGD("HandleTurnOn - Data item:%d Num_refs:%d",dit,citer->second);
            }
        }
        else {
            LOC_LOGE("%s:%d]: Framework action request object is NULL", __func__, __LINE__);
            result = 1;
        }
    } while (0);

    EXIT_LOG_WITH_ERROR ("%d", result);
}

void SystemStatusOsObserver :: turnOff (DataItemId dit) {
    int result = 0;
    ENTRY_LOG ();
    do {
        if (mContext.mFrameworkActionReqObj != NULL) {
            // Check if data item exists in mActiveRequestCount
            map <DataItemId, int> :: iterator citer = mActiveRequestCount.find (dit);
            if (citer != mActiveRequestCount.end ()) {
                citer->second--;
                LOC_LOGD("HandleTurnOff - Data item:%d Remaining Num_refs:%d",dit,citer->second);

                if(citer->second == 0) {
                    LOC_LOGD("Sending turnOff request");
                    // if this was last reference, remove item from map and turn off module
                    mActiveRequestCount.erase(citer);
                    // Send action turn off to framework
                    mContext.mMsgTask->sendMsg (new  (nothrow) HandleTurnOff (this, dit));
                }
            } else {
                // Not found in map
                LOC_LOGD ("Data item id %d not found in FrameworkModuleMap",dit);
            }
        }
        else {
            LOC_LOGE("%s:%d]: Framework action request object is NULL", __func__, __LINE__);
            result = 1;
        }
    } while (0);
    EXIT_LOG_WITH_ERROR ("%d", result);
}

/******************************************************************************
 Helpers
******************************************************************************/
void SystemStatusOsObserver :: logMe (const list <DataItemId> & l) {
    list <DataItemId> :: const_iterator it = l.begin ();
    for (;it != l.end (); ++it) {
        LOC_LOGD ("DataItem %d",*it);
    }
}

int SystemStatusOsObserver :: sendFirstResponse (const list <DataItemId> & l, IDataItemObserver * to) {
    int result = 0;
    ENTRY_LOG ();
    do {
        if (l.empty ()) {
            LOC_LOGV("list is empty. Nothing to do. Exiting");
            result = 0;
            break;
        }

        string clientName;
        to->getName (clientName);
        LOC_LOGD ("First response sent for the following data items To Client: %s", clientName.c_str());

        list <IDataItemCore *> dataItems;
        list <DataItemId> :: const_iterator diditer = l.begin ();
        for (; diditer != l.end (); ++diditer) {
            map <DataItemId, IDataItemCore*> :: const_iterator citer = mDataItemCache.find (*diditer);
            if (citer != mDataItemCache.end ()) {
                string dv;
                IDataItemCore * di = citer->second;
                di->stringify (dv);
                LOC_LOGD ("LocTech-Value :: Data Item: %s", dv.c_str ());
                dataItems.push_back (citer->second);
            }
        }
        if (dataItems.empty ()) {
            LOC_LOGV("No items to notify. Nothing to do. Exiting");
            result = 0;
            break;
        }

        // Notify Client
        to->notify (dataItems);

    } while (0);
    EXIT_LOG_WITH_ERROR ("%d", result);
    return result;
}

int SystemStatusOsObserver :: sendCachedDataItems (const list <DataItemId> & l, IDataItemObserver * to) {
    int result = 0;
    ENTRY_LOG ();
    do {
        list <IDataItemCore *> dataItems;
        list <DataItemId> :: const_iterator it = l.begin ();
        string clientName;
        to->getName (clientName);
        LOC_LOGD ("LocTech-Value :: To Client: %s", clientName.c_str ());
        for (; it != l.end (); ++it) {
            string dv;
            IDataItemCore * di = this->mDataItemCache [ (*it) ];
            di->stringify (dv);
            LOC_LOGI("LocTech-Value :: Data Item: %s >> %s", dv.c_str(), clientName.c_str());
            dataItems.push_back (di);
        }

        to->notify (dataItems);

    } while (0);
    EXIT_LOG_WITH_ERROR ("%d", result);
    return result;
}

int SystemStatusOsObserver :: updateCache (IDataItemCore * d, bool &dataItemUpdated) {
    int result = 0;
    ENTRY_LOG ();
    do {
        BREAK_IF_ZERO (1, d);
        // Check if data item exists in cache
        map <DataItemId, IDataItemCore*> :: iterator citer = mDataItemCache.find (d->getId ());
        if (citer == mDataItemCache.end ()) {
            // New data item; not found in cache
            IDataItemCore * dataitem = DataItemsFactoryProxy::createNewDataItem(d->getId());
            BREAK_IF_ZERO (2, dataitem);
            // Copy the contents of the data item
            dataitem->copy (d);
            pair <DataItemId, IDataItemCore*> cpair (d->getId (), dataitem);
            // Insert in mDataItemCache
            mDataItemCache.insert (cpair);
            dataItemUpdated = true;
        } else {
            // Found in cache; Update cache if necessary
            BREAK_IF_NON_ZERO(3, citer->second->copy (d, &dataItemUpdated));
        }

        if (dataItemUpdated) {
            LOC_LOGV("DataItem:%d updated:%d", d->getId (), dataItemUpdated);
        }
    } while (0);

    EXIT_LOG_WITH_ERROR ("%d", result);
    return result;
}

} // namespace loc_core

