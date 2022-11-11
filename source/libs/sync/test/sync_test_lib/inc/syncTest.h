/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TD_LIBS_SYNC_TEST_H
#define _TD_LIBS_SYNC_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "syncInt.h"

#include "tref.h"
#include "wal.h"

#include "tref.h"
#include "syncEnv.h"
#include "syncIO.h"
#include "syncIndexMgr.h"
#include "syncInt.h"
#include "syncMessage.h"
#include "syncRaftCfg.h"
#include "syncRaftEntry.h"
#include "syncRaftLog.h"
#include "syncRaftStore.h"
#include "syncRespMgr.h"
#include "syncSnapshot.h"
#include "syncUtil.h"
#include "syncVoteMgr.h"

extern void addEpIntoEpSet(SEpSet* pEpSet, const char* fqdn, uint16_t port);

cJSON* syncEntry2Json(const SSyncRaftEntry* pEntry);
char*  syncEntry2Str(const SSyncRaftEntry* pEntry);
void   syncEntryPrint(const SSyncRaftEntry* pObj);
void   syncEntryPrint2(char* s, const SSyncRaftEntry* pObj);
void   syncEntryLog(const SSyncRaftEntry* pObj);
void   syncEntryLog2(char* s, const SSyncRaftEntry* pObj);

char*   syncCfg2Str(SSyncCfg* pSyncCfg);
int32_t syncCfgFromStr(const char* s, SSyncCfg* pSyncCfg);

cJSON* raftCache2Json(SRaftEntryHashCache* pObj);
char*  raftCache2Str(SRaftEntryHashCache* pObj);
void   raftCachePrint(SRaftEntryHashCache* pObj);
void   raftCachePrint2(char* s, SRaftEntryHashCache* pObj);
void   raftCacheLog(SRaftEntryHashCache* pObj);
void   raftCacheLog2(char* s, SRaftEntryHashCache* pObj);

cJSON* raftEntryCache2Json(SRaftEntryCache* pObj);
char*  raftEntryCache2Str(SRaftEntryCache* pObj);
void   raftEntryCachePrint(SRaftEntryCache* pObj);
void   raftEntryCachePrint2(char* s, SRaftEntryCache* pObj);
void   raftEntryCacheLog(SRaftEntryCache* pObj);
void   raftEntryCacheLog2(char* s, SRaftEntryCache* pObj);

int32_t raftStoreFromJson(SRaftStore* pRaftStore, cJSON* pJson);
cJSON*  raftStore2Json(SRaftStore* pRaftStore);
char*   raftStore2Str(SRaftStore* pRaftStore);
void    raftStorePrint(SRaftStore* pObj);
void    raftStorePrint2(char* s, SRaftStore* pObj);
void    raftStoreLog(SRaftStore* pObj);
void    raftStoreLog2(char* s, SRaftStore* pObj);

cJSON* syncAppendEntriesBatch2Json(const SyncAppendEntriesBatch* pMsg);
char*  syncAppendEntriesBatch2Str(const SyncAppendEntriesBatch* pMsg);
void   syncAppendEntriesBatchPrint(const SyncAppendEntriesBatch* pMsg);
void   syncAppendEntriesBatchPrint2(char* s, const SyncAppendEntriesBatch* pMsg);
void   syncAppendEntriesBatchLog(const SyncAppendEntriesBatch* pMsg);
void   syncAppendEntriesBatchLog2(char* s, const SyncAppendEntriesBatch* pMsg);

cJSON* logStore2Json(SSyncLogStore* pLogStore);
char*  logStore2Str(SSyncLogStore* pLogStore);
cJSON* logStoreSimple2Json(SSyncLogStore* pLogStore);
char*  logStoreSimple2Str(SSyncLogStore* pLogStore);
void   logStorePrint(SSyncLogStore* pLogStore);
void   logStorePrint2(char* s, SSyncLogStore* pLogStore);
void   logStoreLog(SSyncLogStore* pLogStore);
void   logStoreLog2(char* s, SSyncLogStore* pLogStore);
void   logStoreSimplePrint(SSyncLogStore* pLogStore);
void   logStoreSimplePrint2(char* s, SSyncLogStore* pLogStore);
void   logStoreSimpleLog(SSyncLogStore* pLogStore);
void   logStoreSimpleLog2(char* s, SSyncLogStore* pLogStore);

cJSON* syncNode2Json(const SSyncNode* pSyncNode);
char*  syncNode2Str(const SSyncNode* pSyncNode);

cJSON* voteGranted2Json(SVotesGranted* pVotesGranted);
char*  voteGranted2Str(SVotesGranted* pVotesGranted);
cJSON* votesRespond2Json(SVotesRespond* pVotesRespond);
char*  votesRespond2Str(SVotesRespond* pVotesRespond);

cJSON* syncUtilNodeInfo2Json(const SNodeInfo* p);
char*  syncUtilRaftId2Str(const SRaftId* p);

cJSON* snapshotSender2Json(SSyncSnapshotSender* pSender);
char*  snapshotSender2Str(SSyncSnapshotSender* pSender);
cJSON* snapshotReceiver2Json(SSyncSnapshotReceiver* pReceiver);
char*  snapshotReceiver2Str(SSyncSnapshotReceiver* pReceiver);

cJSON*   syncIndexMgr2Json(SSyncIndexMgr* pSyncIndexMgr);
char*    syncIndexMgr2Str(SSyncIndexMgr* pSyncIndexMgr);
void     syncIndexMgrPrint(SSyncIndexMgr* pObj);
void     syncIndexMgrPrint2(char* s, SSyncIndexMgr* pObj);
void     syncIndexMgrLog(SSyncIndexMgr* pObj);
void     syncIndexMgrLog2(char* s, SSyncIndexMgr* pObj);

cJSON* syncRpcMsg2Json(SRpcMsg* pRpcMsg);
cJSON* syncRpcUnknownMsg2Json();
char*  syncRpcMsg2Str(SRpcMsg* pRpcMsg);
void   syncRpcMsgPrint(SRpcMsg* pMsg);
void   syncRpcMsgPrint2(char* s, SRpcMsg* pMsg);
void   syncRpcMsgLog(SRpcMsg* pMsg);
void   syncRpcMsgLog2(char* s, SRpcMsg* pMsg);


// origin syncMessage
SyncPing* syncPingBuild(uint32_t dataLen);
SyncPing* syncPingBuild2(const SRaftId* srcId, const SRaftId* destId, int32_t vgId, const char* str);
SyncPing* syncPingBuild3(const SRaftId* srcId, const SRaftId* destId, int32_t vgId);
char*     syncPingSerialize2(const SyncPing* pMsg, uint32_t* len);
int32_t   syncPingSerialize3(const SyncPing* pMsg, char* buf, int32_t bufLen);
SyncPing* syncPingDeserialize3(void* buf, int32_t bufLen);
void      syncPing2RpcMsg(const SyncPing* pMsg, SRpcMsg* pRpcMsg);
void      syncPingFromRpcMsg(const SRpcMsg* pRpcMsg, SyncPing* pMsg);
cJSON*    syncPing2Json(const SyncPing* pMsg);
char*     syncPing2Str(const SyncPing* pMsg);
void      syncPingPrint(const SyncPing* pMsg);
void      syncPingPrint2(char* s, const SyncPing* pMsg);
void      syncPingLog(const SyncPing* pMsg);
void      syncPingLog2(char* s, const SyncPing* pMsg);

#ifdef __cplusplus
}
#endif

#endif /*_TD_LIBS_SYNC_RAFT_ENTRY_H*/
