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

#ifndef _TD_LIBS_SYNC_MESSAGE_H
#define _TD_LIBS_SYNC_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "syncInt.h"

// ---------------------------------------------
typedef struct SyncPing {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  SRaftId  srcId;
  SRaftId  destId;
  // private data
  uint32_t dataLen;
  char     data[];
} SyncPing;


void      syncPingDestroy(SyncPing* pMsg);
void      syncPingSerialize(const SyncPing* pMsg, char* buf, uint32_t bufLen);
void      syncPingDeserialize(const char* buf, uint32_t len, SyncPing* pMsg);
SyncPing* syncPingDeserialize2(const char* buf, uint32_t len);
SyncPing* syncPingFromRpcMsg2(const SRpcMsg* pRpcMsg);

// ---------------------------------------------
typedef struct SyncPingReply {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  SRaftId  srcId;
  SRaftId  destId;
  // private data
  uint32_t dataLen;
  char     data[];
} SyncPingReply;

SyncPingReply* syncPingReplyBuild(uint32_t dataLen);
SyncPingReply* syncPingReplyBuild2(const SRaftId* srcId, const SRaftId* destId, int32_t vgId, const char* str);
SyncPingReply* syncPingReplyBuild3(const SRaftId* srcId, const SRaftId* destId, int32_t vgId);
void           syncPingReplyDestroy(SyncPingReply* pMsg);
void           syncPingReplySerialize(const SyncPingReply* pMsg, char* buf, uint32_t bufLen);
void           syncPingReplyDeserialize(const char* buf, uint32_t len, SyncPingReply* pMsg);
char*          syncPingReplySerialize2(const SyncPingReply* pMsg, uint32_t* len);
SyncPingReply* syncPingReplyDeserialize2(const char* buf, uint32_t len);
int32_t        syncPingReplySerialize3(const SyncPingReply* pMsg, char* buf, int32_t bufLen);
SyncPingReply* syncPingReplyDeserialize3(void* buf, int32_t bufLen);
void           syncPingReply2RpcMsg(const SyncPingReply* pMsg, SRpcMsg* pRpcMsg);
void           syncPingReplyFromRpcMsg(const SRpcMsg* pRpcMsg, SyncPingReply* pMsg);
SyncPingReply* syncPingReplyFromRpcMsg2(const SRpcMsg* pRpcMsg);
cJSON*         syncPingReply2Json(const SyncPingReply* pMsg);
char*          syncPingReply2Str(const SyncPingReply* pMsg);

// for debug ----------------------
void syncPingReplyPrint(const SyncPingReply* pMsg);
void syncPingReplyPrint2(char* s, const SyncPingReply* pMsg);
void syncPingReplyLog(const SyncPingReply* pMsg);
void syncPingReplyLog2(char* s, const SyncPingReply* pMsg);

// ---------------------------------------------
typedef enum ESyncTimeoutType {
  SYNC_TIMEOUT_PING = 100,
  SYNC_TIMEOUT_ELECTION,
  SYNC_TIMEOUT_HEARTBEAT,
} ESyncTimeoutType;

const char* syncTimerTypeStr(enum ESyncTimeoutType timerType);

typedef struct SyncTimeout {
  uint32_t         bytes;
  int32_t          vgId;
  uint32_t         msgType;
  ESyncTimeoutType timeoutType;
  uint64_t         logicClock;
  int32_t          timerMS;
  void*            data;  // need optimized
} SyncTimeout;

SyncTimeout* syncTimeoutBuild();
SyncTimeout* syncTimeoutBuild2(ESyncTimeoutType timeoutType, uint64_t logicClock, int32_t timerMS, int32_t vgId,
                               void* data);
void         syncTimeoutDestroy(SyncTimeout* pMsg);
void         syncTimeoutSerialize(const SyncTimeout* pMsg, char* buf, uint32_t bufLen);
void         syncTimeoutDeserialize(const char* buf, uint32_t len, SyncTimeout* pMsg);
char*        syncTimeoutSerialize2(const SyncTimeout* pMsg, uint32_t* len);
SyncTimeout* syncTimeoutDeserialize2(const char* buf, uint32_t len);
void         syncTimeout2RpcMsg(const SyncTimeout* pMsg, SRpcMsg* pRpcMsg);
void         syncTimeoutFromRpcMsg(const SRpcMsg* pRpcMsg, SyncTimeout* pMsg);
SyncTimeout* syncTimeoutFromRpcMsg2(const SRpcMsg* pRpcMsg);
cJSON*       syncTimeout2Json(const SyncTimeout* pMsg);
char*        syncTimeout2Str(const SyncTimeout* pMsg);

// for debug ----------------------
void syncTimeoutPrint(const SyncTimeout* pMsg);
void syncTimeoutPrint2(char* s, const SyncTimeout* pMsg);
void syncTimeoutLog(const SyncTimeout* pMsg);
void syncTimeoutLog2(char* s, const SyncTimeout* pMsg);

// ---------------------------------------------
typedef struct SyncClientRequest {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;          // TDMT_SYNC_CLIENT_REQUEST
  uint32_t originalRpcType;  // origin RpcMsg msgType
  uint64_t seqNum;
  bool     isWeak;
  uint32_t dataLen;  // origin RpcMsg.contLen
  char     data[];   // origin RpcMsg.pCont
} SyncClientRequest;

SyncClientRequest* syncClientRequestAlloc(uint32_t dataLen);
int32_t syncClientRequestBuildFromRpcMsg(SRpcMsg* pClientRequestRpcMsg, const SRpcMsg* pOriginalRpcMsg, uint64_t seqNum,
                                         bool isWeak, int32_t vgId);
int32_t syncClientRequestBuildFromNoopEntry(SRpcMsg* pClientRequestRpcMsg, const SSyncRaftEntry* pEntry, int32_t vgId);
void    syncClientRequest2RpcMsg(const SyncClientRequest* pMsg, SRpcMsg* pRpcMsg);  // step 2
void    syncClientRequestFromRpcMsg(const SRpcMsg* pRpcMsg, SyncClientRequest* pMsg);
cJSON*  syncClientRequest2Json(const SyncClientRequest* pMsg);
char*   syncClientRequest2Str(const SyncClientRequest* pMsg);

// for debug ----------------------
void syncClientRequestPrint(const SyncClientRequest* pMsg);
void syncClientRequestPrint2(char* s, const SyncClientRequest* pMsg);
void syncClientRequestLog(const SyncClientRequest* pMsg);
void syncClientRequestLog2(char* s, const SyncClientRequest* pMsg);

// ---------------------------------------------
typedef struct SRaftMeta {
  uint64_t seqNum;
  bool     isWeak;
} SRaftMeta;

// block1:
// block2: SRaftMeta array
// block3: rpc msg array (with pCont pointer)

typedef struct SyncClientRequestBatch {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;  // TDMT_SYNC_CLIENT_REQUEST_BATCH
  uint32_t dataCount;
  uint32_t dataLen;
  char     data[];  // block2, block3
} SyncClientRequestBatch;

SyncClientRequestBatch* syncClientRequestBatchBuild(SRpcMsg** rpcMsgPArr, SRaftMeta* raftArr, int32_t arrSize,
                                                    int32_t vgId);
void                    syncClientRequestBatch2RpcMsg(const SyncClientRequestBatch* pSyncMsg, SRpcMsg* pRpcMsg);
void                    syncClientRequestBatchDestroy(SyncClientRequestBatch* pMsg);
void                    syncClientRequestBatchDestroyDeep(SyncClientRequestBatch* pMsg);
SRaftMeta*              syncClientRequestBatchMetaArr(const SyncClientRequestBatch* pSyncMsg);
SRpcMsg*                syncClientRequestBatchRpcMsgArr(const SyncClientRequestBatch* pSyncMsg);
SyncClientRequestBatch* syncClientRequestBatchFromRpcMsg(const SRpcMsg* pRpcMsg);
cJSON*                  syncClientRequestBatch2Json(const SyncClientRequestBatch* pMsg);
char*                   syncClientRequestBatch2Str(const SyncClientRequestBatch* pMsg);

// for debug ----------------------
void syncClientRequestBatchPrint(const SyncClientRequestBatch* pMsg);
void syncClientRequestBatchPrint2(char* s, const SyncClientRequestBatch* pMsg);
void syncClientRequestBatchLog(const SyncClientRequestBatch* pMsg);
void syncClientRequestBatchLog2(char* s, const SyncClientRequestBatch* pMsg);

// ---------------------------------------------
typedef struct SyncClientRequestReply {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  int32_t  errCode;
  SRaftId  leaderHint;
} SyncClientRequestReply;

// ---------------------------------------------
typedef struct SyncRequestVote {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  SRaftId  srcId;
  SRaftId  destId;
  // private data
  SyncTerm  term;
  SyncIndex lastLogIndex;
  SyncTerm  lastLogTerm;
} SyncRequestVote;

SyncRequestVote* syncRequestVoteBuild(int32_t vgId);
void             syncRequestVoteDestroy(SyncRequestVote* pMsg);
void             syncRequestVoteSerialize(const SyncRequestVote* pMsg, char* buf, uint32_t bufLen);
void             syncRequestVoteDeserialize(const char* buf, uint32_t len, SyncRequestVote* pMsg);
char*            syncRequestVoteSerialize2(const SyncRequestVote* pMsg, uint32_t* len);
SyncRequestVote* syncRequestVoteDeserialize2(const char* buf, uint32_t len);
void             syncRequestVote2RpcMsg(const SyncRequestVote* pMsg, SRpcMsg* pRpcMsg);
void             syncRequestVoteFromRpcMsg(const SRpcMsg* pRpcMsg, SyncRequestVote* pMsg);
SyncRequestVote* syncRequestVoteFromRpcMsg2(const SRpcMsg* pRpcMsg);
cJSON*           syncRequestVote2Json(const SyncRequestVote* pMsg);
char*            syncRequestVote2Str(const SyncRequestVote* pMsg);

// for debug ----------------------
void syncRequestVotePrint(const SyncRequestVote* pMsg);
void syncRequestVotePrint2(char* s, const SyncRequestVote* pMsg);
void syncRequestVoteLog(const SyncRequestVote* pMsg);
void syncRequestVoteLog2(char* s, const SyncRequestVote* pMsg);

// ---------------------------------------------
typedef struct SyncRequestVoteReply {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  SRaftId  srcId;
  SRaftId  destId;
  // private data
  SyncTerm term;
  bool     voteGranted;
} SyncRequestVoteReply;

SyncRequestVoteReply* syncRequestVoteReplyBuild(int32_t vgId);
void                  syncRequestVoteReplyDestroy(SyncRequestVoteReply* pMsg);
void                  syncRequestVoteReplySerialize(const SyncRequestVoteReply* pMsg, char* buf, uint32_t bufLen);
void                  syncRequestVoteReplyDeserialize(const char* buf, uint32_t len, SyncRequestVoteReply* pMsg);
char*                 syncRequestVoteReplySerialize2(const SyncRequestVoteReply* pMsg, uint32_t* len);
SyncRequestVoteReply* syncRequestVoteReplyDeserialize2(const char* buf, uint32_t len);
void                  syncRequestVoteReply2RpcMsg(const SyncRequestVoteReply* pMsg, SRpcMsg* pRpcMsg);
void                  syncRequestVoteReplyFromRpcMsg(const SRpcMsg* pRpcMsg, SyncRequestVoteReply* pMsg);
SyncRequestVoteReply* syncRequestVoteReplyFromRpcMsg2(const SRpcMsg* pRpcMsg);
cJSON*                syncRequestVoteReply2Json(const SyncRequestVoteReply* pMsg);
char*                 syncRequestVoteReply2Str(const SyncRequestVoteReply* pMsg);

// for debug ----------------------
void syncRequestVoteReplyPrint(const SyncRequestVoteReply* pMsg);
void syncRequestVoteReplyPrint2(char* s, const SyncRequestVoteReply* pMsg);
void syncRequestVoteReplyLog(const SyncRequestVoteReply* pMsg);
void syncRequestVoteReplyLog2(char* s, const SyncRequestVoteReply* pMsg);

// ---------------------------------------------
// data: entry

typedef struct SyncAppendEntries {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  SRaftId  srcId;
  SRaftId  destId;

  // private data
  SyncTerm  term;
  SyncIndex prevLogIndex;
  SyncTerm  prevLogTerm;
  SyncIndex commitIndex;
  SyncTerm  privateTerm;
  uint32_t  dataLen;
  char      data[];
} SyncAppendEntries;

SyncAppendEntries* syncAppendEntriesBuild(uint32_t dataLen, int32_t vgId);
void               syncAppendEntriesDestroy(SyncAppendEntries* pMsg);
void               syncAppendEntriesSerialize(const SyncAppendEntries* pMsg, char* buf, uint32_t bufLen);
void               syncAppendEntriesDeserialize(const char* buf, uint32_t len, SyncAppendEntries* pMsg);
char*              syncAppendEntriesSerialize2(const SyncAppendEntries* pMsg, uint32_t* len);
SyncAppendEntries* syncAppendEntriesDeserialize2(const char* buf, uint32_t len);
void               syncAppendEntries2RpcMsg(const SyncAppendEntries* pMsg, SRpcMsg* pRpcMsg);
void               syncAppendEntriesFromRpcMsg(const SRpcMsg* pRpcMsg, SyncAppendEntries* pMsg);
SyncAppendEntries* syncAppendEntriesFromRpcMsg2(const SRpcMsg* pRpcMsg);
cJSON*             syncAppendEntries2Json(const SyncAppendEntries* pMsg);
char*              syncAppendEntries2Str(const SyncAppendEntries* pMsg);

// for debug ----------------------
void syncAppendEntriesPrint(const SyncAppendEntries* pMsg);
void syncAppendEntriesPrint2(char* s, const SyncAppendEntries* pMsg);
void syncAppendEntriesLog(const SyncAppendEntries* pMsg);
void syncAppendEntriesLog2(char* s, const SyncAppendEntries* pMsg);

// ---------------------------------------------

typedef struct SOffsetAndContLen {
  int32_t offset;
  int32_t contLen;
} SOffsetAndContLen;

// data:
// block1: SOffsetAndContLen Array
// block2: entry Array

typedef struct SyncAppendEntriesBatch {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  SRaftId  srcId;
  SRaftId  destId;

  // private data
  SyncTerm  term;
  SyncIndex prevLogIndex;
  SyncTerm  prevLogTerm;
  SyncIndex commitIndex;
  SyncTerm  privateTerm;
  int32_t   dataCount;
  uint32_t  dataLen;
  char      data[];  // block1, block2
} SyncAppendEntriesBatch;

SyncAppendEntriesBatch* syncAppendEntriesBatchBuild(SSyncRaftEntry** entryPArr, int32_t arrSize, int32_t vgId);
SOffsetAndContLen*      syncAppendEntriesBatchMetaTableArray(SyncAppendEntriesBatch* pMsg);
void                    syncAppendEntriesBatchDestroy(SyncAppendEntriesBatch* pMsg);
void                    syncAppendEntriesBatchSerialize(const SyncAppendEntriesBatch* pMsg, char* buf, uint32_t bufLen);
void                    syncAppendEntriesBatchDeserialize(const char* buf, uint32_t len, SyncAppendEntriesBatch* pMsg);
char*                   syncAppendEntriesBatchSerialize2(const SyncAppendEntriesBatch* pMsg, uint32_t* len);
SyncAppendEntriesBatch* syncAppendEntriesBatchDeserialize2(const char* buf, uint32_t len);
void                    syncAppendEntriesBatch2RpcMsg(const SyncAppendEntriesBatch* pMsg, SRpcMsg* pRpcMsg);
void                    syncAppendEntriesBatchFromRpcMsg(const SRpcMsg* pRpcMsg, SyncAppendEntriesBatch* pMsg);
SyncAppendEntriesBatch* syncAppendEntriesBatchFromRpcMsg2(const SRpcMsg* pRpcMsg);

// ---------------------------------------------
typedef struct SyncAppendEntriesReply {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  SRaftId  srcId;
  SRaftId  destId;
  // private data
  SyncTerm  term;
  SyncTerm  privateTerm;
  bool      success;
  SyncIndex matchIndex;
  SyncIndex lastSendIndex;
  int64_t   startTime;
} SyncAppendEntriesReply;

SyncAppendEntriesReply* syncAppendEntriesReplyBuild(int32_t vgId);
void                    syncAppendEntriesReplyDestroy(SyncAppendEntriesReply* pMsg);
void                    syncAppendEntriesReplySerialize(const SyncAppendEntriesReply* pMsg, char* buf, uint32_t bufLen);
void                    syncAppendEntriesReplyDeserialize(const char* buf, uint32_t len, SyncAppendEntriesReply* pMsg);
char*                   syncAppendEntriesReplySerialize2(const SyncAppendEntriesReply* pMsg, uint32_t* len);
SyncAppendEntriesReply* syncAppendEntriesReplyDeserialize2(const char* buf, uint32_t len);
void                    syncAppendEntriesReply2RpcMsg(const SyncAppendEntriesReply* pMsg, SRpcMsg* pRpcMsg);
void                    syncAppendEntriesReplyFromRpcMsg(const SRpcMsg* pRpcMsg, SyncAppendEntriesReply* pMsg);
SyncAppendEntriesReply* syncAppendEntriesReplyFromRpcMsg2(const SRpcMsg* pRpcMsg);
cJSON*                  syncAppendEntriesReply2Json(const SyncAppendEntriesReply* pMsg);
char*                   syncAppendEntriesReply2Str(const SyncAppendEntriesReply* pMsg);

// for debug ----------------------
void syncAppendEntriesReplyPrint(const SyncAppendEntriesReply* pMsg);
void syncAppendEntriesReplyPrint2(char* s, const SyncAppendEntriesReply* pMsg);
void syncAppendEntriesReplyLog(const SyncAppendEntriesReply* pMsg);
void syncAppendEntriesReplyLog2(char* s, const SyncAppendEntriesReply* pMsg);

// ---------------------------------------------
typedef struct SyncHeartbeat {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  SRaftId  srcId;
  SRaftId  destId;

  // private data
  SyncTerm  term;
  SyncIndex commitIndex;
  SyncTerm  privateTerm;
  SyncTerm  minMatchIndex;

} SyncHeartbeat;

SyncHeartbeat* syncHeartbeatBuild(int32_t vgId);
void           syncHeartbeatDestroy(SyncHeartbeat* pMsg);
void           syncHeartbeatSerialize(const SyncHeartbeat* pMsg, char* buf, uint32_t bufLen);
void           syncHeartbeatDeserialize(const char* buf, uint32_t len, SyncHeartbeat* pMsg);
char*          syncHeartbeatSerialize2(const SyncHeartbeat* pMsg, uint32_t* len);
SyncHeartbeat* syncHeartbeatDeserialize2(const char* buf, uint32_t len);
void           syncHeartbeat2RpcMsg(const SyncHeartbeat* pMsg, SRpcMsg* pRpcMsg);
void           syncHeartbeatFromRpcMsg(const SRpcMsg* pRpcMsg, SyncHeartbeat* pMsg);
SyncHeartbeat* syncHeartbeatFromRpcMsg2(const SRpcMsg* pRpcMsg);
cJSON*         syncHeartbeat2Json(const SyncHeartbeat* pMsg);
char*          syncHeartbeat2Str(const SyncHeartbeat* pMsg);

// for debug ----------------------
void syncHeartbeatPrint(const SyncHeartbeat* pMsg);
void syncHeartbeatPrint2(char* s, const SyncHeartbeat* pMsg);
void syncHeartbeatLog(const SyncHeartbeat* pMsg);
void syncHeartbeatLog2(char* s, const SyncHeartbeat* pMsg);

// ---------------------------------------------
typedef struct SyncHeartbeatReply {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  SRaftId  srcId;
  SRaftId  destId;

  // private data
  SyncTerm term;
  SyncTerm privateTerm;
  int64_t  startTime;
} SyncHeartbeatReply;

SyncHeartbeatReply* syncHeartbeatReplyBuild(int32_t vgId);
void                syncHeartbeatReplyDestroy(SyncHeartbeatReply* pMsg);
void                syncHeartbeatReplySerialize(const SyncHeartbeatReply* pMsg, char* buf, uint32_t bufLen);
void                syncHeartbeatReplyDeserialize(const char* buf, uint32_t len, SyncHeartbeatReply* pMsg);
char*               syncHeartbeatReplySerialize2(const SyncHeartbeatReply* pMsg, uint32_t* len);
SyncHeartbeatReply* syncHeartbeatReplyDeserialize2(const char* buf, uint32_t len);
void                syncHeartbeatReply2RpcMsg(const SyncHeartbeatReply* pMsg, SRpcMsg* pRpcMsg);
void                syncHeartbeatReplyFromRpcMsg(const SRpcMsg* pRpcMsg, SyncHeartbeatReply* pMsg);
SyncHeartbeatReply* syncHeartbeatReplyFromRpcMsg2(const SRpcMsg* pRpcMsg);
cJSON*              syncHeartbeatReply2Json(const SyncHeartbeatReply* pMsg);
char*               syncHeartbeatReply2Str(const SyncHeartbeatReply* pMsg);

// for debug ----------------------
void syncHeartbeatReplyPrint(const SyncHeartbeatReply* pMsg);
void syncHeartbeatReplyPrint2(char* s, const SyncHeartbeatReply* pMsg);
void syncHeartbeatReplyLog(const SyncHeartbeatReply* pMsg);
void syncHeartbeatReplyLog2(char* s, const SyncHeartbeatReply* pMsg);

// ---------------------------------------------
typedef struct SyncPreSnapshot {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  SRaftId  srcId;
  SRaftId  destId;

  // private data
  SyncTerm term;

} SyncPreSnapshot;

SyncPreSnapshot* syncPreSnapshotBuild(int32_t vgId);
void             syncPreSnapshotDestroy(SyncPreSnapshot* pMsg);
void             syncPreSnapshotSerialize(const SyncPreSnapshot* pMsg, char* buf, uint32_t bufLen);
void             syncPreSnapshotDeserialize(const char* buf, uint32_t len, SyncPreSnapshot* pMsg);
char*            syncPreSnapshotSerialize2(const SyncPreSnapshot* pMsg, uint32_t* len);
SyncPreSnapshot* syncPreSnapshotDeserialize2(const char* buf, uint32_t len);
void             syncPreSnapshot2RpcMsg(const SyncPreSnapshot* pMsg, SRpcMsg* pRpcMsg);
void             syncPreSnapshotFromRpcMsg(const SRpcMsg* pRpcMsg, SyncPreSnapshot* pMsg);
SyncPreSnapshot* syncPreSnapshotFromRpcMsg2(const SRpcMsg* pRpcMsg);
cJSON*           syncPreSnapshot2Json(const SyncPreSnapshot* pMsg);
char*            syncPreSnapshot2Str(const SyncPreSnapshot* pMsg);

// for debug ----------------------
void syncPreSnapshotPrint(const SyncPreSnapshot* pMsg);
void syncPreSnapshotPrint2(char* s, const SyncPreSnapshot* pMsg);
void syncPreSnapshotLog(const SyncPreSnapshot* pMsg);
void syncPreSnapshotLog2(char* s, const SyncPreSnapshot* pMsg);

// ---------------------------------------------
typedef struct SyncPreSnapshotReply {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  SRaftId  srcId;
  SRaftId  destId;

  // private data
  SyncTerm  term;
  SyncIndex snapStart;

} SyncPreSnapshotReply;

SyncPreSnapshotReply* syncPreSnapshotReplyBuild(int32_t vgId);
void                  syncPreSnapshotReplyDestroy(SyncPreSnapshotReply* pMsg);
void                  syncPreSnapshotReplySerialize(const SyncPreSnapshotReply* pMsg, char* buf, uint32_t bufLen);
void                  syncPreSnapshotReplyDeserialize(const char* buf, uint32_t len, SyncPreSnapshotReply* pMsg);
char*                 syncPreSnapshotReplySerialize2(const SyncPreSnapshotReply* pMsg, uint32_t* len);
SyncPreSnapshotReply* syncPreSnapshotReplyDeserialize2(const char* buf, uint32_t len);
void                  syncPreSnapshotReply2RpcMsg(const SyncPreSnapshotReply* pMsg, SRpcMsg* pRpcMsg);
void                  syncPreSnapshotReplyFromRpcMsg(const SRpcMsg* pRpcMsg, SyncPreSnapshotReply* pMsg);
SyncPreSnapshotReply* syncPreSnapshotReplyFromRpcMsg2(const SRpcMsg* pRpcMsg);
cJSON*                syncPreSnapshotReply2Json(const SyncPreSnapshotReply* pMsg);
char*                 syncPreSnapshotReply2Str(const SyncPreSnapshotReply* pMsg);

// for debug ----------------------
void syncPreSnapshotReplyPrint(const SyncPreSnapshotReply* pMsg);
void syncPreSnapshotReplyPrint2(char* s, const SyncPreSnapshotReply* pMsg);
void syncPreSnapshotReplyLog(const SyncPreSnapshotReply* pMsg);
void syncPreSnapshotReplyLog2(char* s, const SyncPreSnapshotReply* pMsg);

// ---------------------------------------------
typedef struct SyncApplyMsg {
  uint32_t   bytes;
  int32_t    vgId;
  uint32_t   msgType;          // user SyncApplyMsg msgType
  uint32_t   originalRpcType;  // user RpcMsg msgType
  SFsmCbMeta fsmMeta;
  uint32_t   dataLen;  // user RpcMsg.contLen
  char       data[];   // user RpcMsg.pCont
} SyncApplyMsg;

SyncApplyMsg* syncApplyMsgBuild(uint32_t dataLen);
SyncApplyMsg* syncApplyMsgBuild2(const SRpcMsg* pOriginalRpcMsg, int32_t vgId, SFsmCbMeta* pMeta);
void          syncApplyMsgDestroy(SyncApplyMsg* pMsg);
void          syncApplyMsgSerialize(const SyncApplyMsg* pMsg, char* buf, uint32_t bufLen);
void          syncApplyMsgDeserialize(const char* buf, uint32_t len, SyncApplyMsg* pMsg);
char*         syncApplyMsgSerialize2(const SyncApplyMsg* pMsg, uint32_t* len);
SyncApplyMsg* syncApplyMsgDeserialize2(const char* buf, uint32_t len);
void syncApplyMsg2RpcMsg(const SyncApplyMsg* pMsg, SRpcMsg* pRpcMsg);     // SyncApplyMsg to SRpcMsg, put it into ApplyQ
void syncApplyMsgFromRpcMsg(const SRpcMsg* pRpcMsg, SyncApplyMsg* pMsg);  // get SRpcMsg from ApplyQ, to SyncApplyMsg
SyncApplyMsg* syncApplyMsgFromRpcMsg2(const SRpcMsg* pRpcMsg);
void syncApplyMsg2OriginalRpcMsg(const SyncApplyMsg* pMsg, SRpcMsg* pOriginalRpcMsg);  // SyncApplyMsg to OriginalRpcMsg
cJSON* syncApplyMsg2Json(const SyncApplyMsg* pMsg);
char*  syncApplyMsg2Str(const SyncApplyMsg* pMsg);

// for debug ----------------------
void syncApplyMsgPrint(const SyncApplyMsg* pMsg);
void syncApplyMsgPrint2(char* s, const SyncApplyMsg* pMsg);
void syncApplyMsgLog(const SyncApplyMsg* pMsg);
void syncApplyMsgLog2(char* s, const SyncApplyMsg* pMsg);

// ---------------------------------------------
typedef struct SyncSnapshotSend {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  SRaftId  srcId;
  SRaftId  destId;

  SyncTerm  term;
  SyncIndex beginIndex;       // snapshot.beginIndex
  SyncIndex lastIndex;        // snapshot.lastIndex
  SyncTerm  lastTerm;         // snapshot.lastTerm
  SyncIndex lastConfigIndex;  // snapshot.lastConfigIndex
  SSyncCfg  lastConfig;
  int64_t   startTime;
  int32_t   seq;
  uint32_t  dataLen;
  char      data[];
} SyncSnapshotSend;

SyncSnapshotSend* syncSnapshotSendBuild(uint32_t dataLen, int32_t vgId);
void              syncSnapshotSendDestroy(SyncSnapshotSend* pMsg);
void              syncSnapshotSendSerialize(const SyncSnapshotSend* pMsg, char* buf, uint32_t bufLen);
void              syncSnapshotSendDeserialize(const char* buf, uint32_t len, SyncSnapshotSend* pMsg);
char*             syncSnapshotSendSerialize2(const SyncSnapshotSend* pMsg, uint32_t* len);
SyncSnapshotSend* syncSnapshotSendDeserialize2(const char* buf, uint32_t len);
void              syncSnapshotSend2RpcMsg(const SyncSnapshotSend* pMsg, SRpcMsg* pRpcMsg);
void              syncSnapshotSendFromRpcMsg(const SRpcMsg* pRpcMsg, SyncSnapshotSend* pMsg);
SyncSnapshotSend* syncSnapshotSendFromRpcMsg2(const SRpcMsg* pRpcMsg);
cJSON*            syncSnapshotSend2Json(const SyncSnapshotSend* pMsg);
char*             syncSnapshotSend2Str(const SyncSnapshotSend* pMsg);

// for debug ----------------------
void syncSnapshotSendPrint(const SyncSnapshotSend* pMsg);
void syncSnapshotSendPrint2(char* s, const SyncSnapshotSend* pMsg);
void syncSnapshotSendLog(const SyncSnapshotSend* pMsg);
void syncSnapshotSendLog2(char* s, const SyncSnapshotSend* pMsg);

// ---------------------------------------------
typedef struct SyncSnapshotRsp {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  SRaftId  srcId;
  SRaftId  destId;

  SyncTerm  term;
  SyncIndex lastIndex;
  SyncTerm  lastTerm;
  int64_t   startTime;
  int32_t   ack;
  int32_t   code;
  SyncIndex snapBeginIndex;  // when ack = SYNC_SNAPSHOT_SEQ_BEGIN, it's valid
} SyncSnapshotRsp;

SyncSnapshotRsp* syncSnapshotRspBuild(int32_t vgId);
void             syncSnapshotRspDestroy(SyncSnapshotRsp* pMsg);
void             syncSnapshotRspSerialize(const SyncSnapshotRsp* pMsg, char* buf, uint32_t bufLen);
void             syncSnapshotRspDeserialize(const char* buf, uint32_t len, SyncSnapshotRsp* pMsg);
char*            syncSnapshotRspSerialize2(const SyncSnapshotRsp* pMsg, uint32_t* len);
SyncSnapshotRsp* syncSnapshotRspDeserialize2(const char* buf, uint32_t len);
void             syncSnapshotRsp2RpcMsg(const SyncSnapshotRsp* pMsg, SRpcMsg* pRpcMsg);
void             syncSnapshotRspFromRpcMsg(const SRpcMsg* pRpcMsg, SyncSnapshotRsp* pMsg);
SyncSnapshotRsp* syncSnapshotRspFromRpcMsg2(const SRpcMsg* pRpcMsg);
cJSON*           syncSnapshotRsp2Json(const SyncSnapshotRsp* pMsg);
char*            syncSnapshotRsp2Str(const SyncSnapshotRsp* pMsg);

// for debug ----------------------
void syncSnapshotRspPrint(const SyncSnapshotRsp* pMsg);
void syncSnapshotRspPrint2(char* s, const SyncSnapshotRsp* pMsg);
void syncSnapshotRspLog(const SyncSnapshotRsp* pMsg);
void syncSnapshotRspLog2(char* s, const SyncSnapshotRsp* pMsg);

// ---------------------------------------------
typedef struct SyncLeaderTransfer {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  /*
   SRaftId  srcId;
   SRaftId  destId;
   */
  SNodeInfo newNodeInfo;
  SRaftId   newLeaderId;
} SyncLeaderTransfer;

SyncLeaderTransfer* syncLeaderTransferBuild(int32_t vgId);
void                syncLeaderTransferDestroy(SyncLeaderTransfer* pMsg);
void                syncLeaderTransferSerialize(const SyncLeaderTransfer* pMsg, char* buf, uint32_t bufLen);
void                syncLeaderTransferDeserialize(const char* buf, uint32_t len, SyncLeaderTransfer* pMsg);
char*               syncLeaderTransferSerialize2(const SyncLeaderTransfer* pMsg, uint32_t* len);
SyncLeaderTransfer* syncLeaderTransferDeserialize2(const char* buf, uint32_t len);
void                syncLeaderTransfer2RpcMsg(const SyncLeaderTransfer* pMsg, SRpcMsg* pRpcMsg);
void                syncLeaderTransferFromRpcMsg(const SRpcMsg* pRpcMsg, SyncLeaderTransfer* pMsg);
SyncLeaderTransfer* syncLeaderTransferFromRpcMsg2(const SRpcMsg* pRpcMsg);
cJSON*              syncLeaderTransfer2Json(const SyncLeaderTransfer* pMsg);
char*               syncLeaderTransfer2Str(const SyncLeaderTransfer* pMsg);

typedef enum {
  SYNC_LOCAL_CMD_STEP_DOWN = 100,
  SYNC_LOCAL_CMD_FOLLOWER_CMT,
} ESyncLocalCmd;

const char* syncLocalCmdGetStr(int32_t cmd);

typedef struct SyncLocalCmd {
  uint32_t bytes;
  int32_t  vgId;
  uint32_t msgType;
  SRaftId  srcId;
  SRaftId  destId;

  int32_t  cmd;
  SyncTerm sdNewTerm;  // step down new term
  SyncIndex fcIndex;// follower commit index

} SyncLocalCmd;

SyncLocalCmd* syncLocalCmdBuild(int32_t vgId);
void          syncLocalCmdDestroy(SyncLocalCmd* pMsg);
void          syncLocalCmdSerialize(const SyncLocalCmd* pMsg, char* buf, uint32_t bufLen);
void          syncLocalCmdDeserialize(const char* buf, uint32_t len, SyncLocalCmd* pMsg);
char*         syncLocalCmdSerialize2(const SyncLocalCmd* pMsg, uint32_t* len);
SyncLocalCmd* syncLocalCmdDeserialize2(const char* buf, uint32_t len);
void          syncLocalCmd2RpcMsg(const SyncLocalCmd* pMsg, SRpcMsg* pRpcMsg);
void          syncLocalCmdFromRpcMsg(const SRpcMsg* pRpcMsg, SyncLocalCmd* pMsg);
SyncLocalCmd* syncLocalCmdFromRpcMsg2(const SRpcMsg* pRpcMsg);
cJSON*        syncLocalCmd2Json(const SyncLocalCmd* pMsg);
char*         syncLocalCmd2Str(const SyncLocalCmd* pMsg);

// for debug ----------------------
void syncLocalCmdPrint(const SyncLocalCmd* pMsg);
void syncLocalCmdPrint2(char* s, const SyncLocalCmd* pMsg);
void syncLocalCmdLog(const SyncLocalCmd* pMsg);
void syncLocalCmdLog2(char* s, const SyncLocalCmd* pMsg);

// on message ----------------------
int32_t syncNodeOnPing(SSyncNode* ths, SyncPing* pMsg);
int32_t syncNodeOnPingReply(SSyncNode* ths, SyncPingReply* pMsg);

int32_t syncNodeOnRequestVote(SSyncNode* ths, SyncRequestVote* pMsg);
int32_t syncNodeOnRequestVoteReply(SSyncNode* ths, SyncRequestVoteReply* pMsg);

int32_t syncNodeOnAppendEntries(SSyncNode* ths, SyncAppendEntries* pMsg);
int32_t syncNodeOnAppendEntriesReply(SSyncNode* ths, SyncAppendEntriesReply* pMsg);

int32_t syncNodeOnPreSnapshot(SSyncNode* ths, SyncPreSnapshot* pMsg);
int32_t syncNodeOnPreSnapshotReply(SSyncNode* ths, SyncPreSnapshotReply* pMsg);

int32_t syncNodeOnSnapshot(SSyncNode* ths, SyncSnapshotSend* pMsg);
int32_t syncNodeOnSnapshotReply(SSyncNode* ths, SyncSnapshotRsp* pMsg);

int32_t syncNodeOnHeartbeat(SSyncNode* ths, SyncHeartbeat* pMsg);
int32_t syncNodeOnHeartbeatReply(SSyncNode* ths, SyncHeartbeatReply* pMsg);

int32_t syncNodeOnClientRequest(SSyncNode* ths, SRpcMsg* pMsg, SyncIndex* pRetIndex);
int32_t syncNodeOnLocalCmd(SSyncNode* ths, SyncLocalCmd* pMsg);

// -----------------------------------------

// option ----------------------------------
bool          syncNodeSnapshotEnable(SSyncNode* pSyncNode);
ESyncStrategy syncNodeStrategy(SSyncNode* pSyncNode);

// ---------------------------------------------

#ifdef __cplusplus
}
#endif

#endif /*_TD_LIBS_SYNC_MESSAGE_H*/
