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

#define _DEFAULT_SOURCE
#include "syncTest.h"

// ---- message process SyncPing----
SyncPing* syncPingBuild(uint32_t dataLen) {
  uint32_t  bytes = sizeof(SyncPing) + dataLen;
  SyncPing* pMsg = taosMemoryMalloc(bytes);
  memset(pMsg, 0, bytes);
  pMsg->bytes = bytes;
  pMsg->msgType = TDMT_SYNC_PING;
  pMsg->dataLen = dataLen;
  return pMsg;
}

SyncPing* syncPingBuild2(const SRaftId* srcId, const SRaftId* destId, int32_t vgId, const char* str) {
  uint32_t  dataLen = strlen(str) + 1;
  SyncPing* pMsg = syncPingBuild(dataLen);
  pMsg->vgId = vgId;
  pMsg->srcId = *srcId;
  pMsg->destId = *destId;
  snprintf(pMsg->data, pMsg->dataLen, "%s", str);
  return pMsg;
}

SyncPing* syncPingBuild3(const SRaftId* srcId, const SRaftId* destId, int32_t vgId) {
  SyncPing* pMsg = syncPingBuild2(srcId, destId, vgId, "ping");
  return pMsg;
}

char* syncPingSerialize2(const SyncPing* pMsg, uint32_t* len) {
  char* buf = taosMemoryMalloc(pMsg->bytes);
  ASSERT(buf != NULL);
  syncPingSerialize(pMsg, buf, pMsg->bytes);
  if (len != NULL) {
    *len = pMsg->bytes;
  }
  return buf;
}

void syncPing2RpcMsg(const SyncPing* pMsg, SRpcMsg* pRpcMsg) {
  memset(pRpcMsg, 0, sizeof(*pRpcMsg));
  pRpcMsg->msgType = pMsg->msgType;
  pRpcMsg->contLen = pMsg->bytes;
  pRpcMsg->pCont = rpcMallocCont(pRpcMsg->contLen);
  syncPingSerialize(pMsg, pRpcMsg->pCont, pRpcMsg->contLen);
}

void syncPingFromRpcMsg(const SRpcMsg* pRpcMsg, SyncPing* pMsg) {
  syncPingDeserialize(pRpcMsg->pCont, pRpcMsg->contLen, pMsg);
}

SyncPing* syncPingDeserialize3(void* buf, int32_t bufLen) {
  SDecoder decoder = {0};
  tDecoderInit(&decoder, buf, bufLen);
  if (tStartDecode(&decoder) < 0) {
    return NULL;
  }

  SyncPing* pMsg = NULL;
  uint32_t  bytes;
  if (tDecodeU32(&decoder, &bytes) < 0) {
    return NULL;
  }

  pMsg = taosMemoryMalloc(bytes);
  ASSERT(pMsg != NULL);
  pMsg->bytes = bytes;

  if (tDecodeI32(&decoder, &pMsg->vgId) < 0) {
    taosMemoryFree(pMsg);
    return NULL;
  }
  if (tDecodeU32(&decoder, &pMsg->msgType) < 0) {
    taosMemoryFree(pMsg);
    return NULL;
  }
  if (tDecodeU64(&decoder, &pMsg->srcId.addr) < 0) {
    taosMemoryFree(pMsg);
    return NULL;
  }
  if (tDecodeI32(&decoder, &pMsg->srcId.vgId) < 0) {
    taosMemoryFree(pMsg);
    return NULL;
  }
  if (tDecodeU64(&decoder, &pMsg->destId.addr) < 0) {
    taosMemoryFree(pMsg);
    return NULL;
  }
  if (tDecodeI32(&decoder, &pMsg->destId.vgId) < 0) {
    taosMemoryFree(pMsg);
    return NULL;
  }
  if (tDecodeU32(&decoder, &pMsg->dataLen) < 0) {
    taosMemoryFree(pMsg);
    return NULL;
  }
  uint32_t len;
  char*    data = NULL;
  if (tDecodeBinary(&decoder, (uint8_t**)(&data), &len) < 0) {
    taosMemoryFree(pMsg);
    return NULL;
  }
  ASSERT(len == pMsg->dataLen);
  memcpy(pMsg->data, data, len);

  tEndDecode(&decoder);
  tDecoderClear(&decoder);
  return pMsg;
}

int32_t syncPingSerialize3(const SyncPing* pMsg, char* buf, int32_t bufLen) {
  SEncoder encoder = {0};
  tEncoderInit(&encoder, buf, bufLen);
  if (tStartEncode(&encoder) < 0) {
    return -1;
  }

  if (tEncodeU32(&encoder, pMsg->bytes) < 0) {
    return -1;
  }
  if (tEncodeI32(&encoder, pMsg->vgId) < 0) {
    return -1;
  }
  if (tEncodeU32(&encoder, pMsg->msgType) < 0) {
    return -1;
  }
  if (tEncodeU64(&encoder, pMsg->srcId.addr) < 0) {
    return -1;
  }
  if (tEncodeI32(&encoder, pMsg->srcId.vgId) < 0) {
    return -1;
  }
  if (tEncodeU64(&encoder, pMsg->destId.addr) < 0) {
    return -1;
  }
  if (tEncodeI32(&encoder, pMsg->destId.vgId) < 0) {
    return -1;
  }
  if (tEncodeU32(&encoder, pMsg->dataLen) < 0) {
    return -1;
  }
  if (tEncodeBinary(&encoder, pMsg->data, pMsg->dataLen)) {
    return -1;
  }

  tEndEncode(&encoder);
  int32_t tlen = encoder.pos;
  tEncoderClear(&encoder);
  return tlen;
}

cJSON* syncPing2Json(const SyncPing* pMsg) {
  char   u64buf[128] = {0};
  cJSON* pRoot = cJSON_CreateObject();

  if (pMsg != NULL) {
    cJSON_AddNumberToObject(pRoot, "bytes", pMsg->bytes);
    cJSON_AddNumberToObject(pRoot, "vgId", pMsg->vgId);
    cJSON_AddNumberToObject(pRoot, "msgType", pMsg->msgType);

    cJSON* pSrcId = cJSON_CreateObject();
    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pMsg->srcId.addr);
    cJSON_AddStringToObject(pSrcId, "addr", u64buf);
    {
      uint64_t u64 = pMsg->srcId.addr;
      cJSON*   pTmp = pSrcId;
      char     host[128] = {0};
      uint16_t port;
      syncUtilU642Addr(u64, host, sizeof(host), &port);
      cJSON_AddStringToObject(pTmp, "addr_host", host);
      cJSON_AddNumberToObject(pTmp, "addr_port", port);
    }
    cJSON_AddNumberToObject(pSrcId, "vgId", pMsg->srcId.vgId);
    cJSON_AddItemToObject(pRoot, "srcId", pSrcId);

    cJSON* pDestId = cJSON_CreateObject();
    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pMsg->destId.addr);
    cJSON_AddStringToObject(pDestId, "addr", u64buf);
    {
      uint64_t u64 = pMsg->destId.addr;
      cJSON*   pTmp = pDestId;
      char     host[128] = {0};
      uint16_t port;
      syncUtilU642Addr(u64, host, sizeof(host), &port);
      cJSON_AddStringToObject(pTmp, "addr_host", host);
      cJSON_AddNumberToObject(pTmp, "addr_port", port);
    }
    cJSON_AddNumberToObject(pDestId, "vgId", pMsg->destId.vgId);
    cJSON_AddItemToObject(pRoot, "destId", pDestId);

    cJSON_AddNumberToObject(pRoot, "dataLen", pMsg->dataLen);
    char* s;
    s = syncUtilPrintBin((char*)(pMsg->data), pMsg->dataLen);
    cJSON_AddStringToObject(pRoot, "data", s);
    taosMemoryFree(s);
    s = syncUtilPrintBin2((char*)(pMsg->data), pMsg->dataLen);
    cJSON_AddStringToObject(pRoot, "data2", s);
    taosMemoryFree(s);
  }

  cJSON* pJson = cJSON_CreateObject();
  cJSON_AddItemToObject(pJson, "SyncPing", pRoot);
  return pJson;
}

char* syncPing2Str(const SyncPing* pMsg) {
  cJSON* pJson = syncPing2Json(pMsg);
  char*  serialized = cJSON_Print(pJson);
  cJSON_Delete(pJson);
  return serialized;
}

// for debug ----------------------
void syncPingPrint(const SyncPing* pMsg) {
  char* serialized = syncPing2Str(pMsg);
  printf("syncPingPrint | len:%d | %s \n", (int32_t)strlen(serialized), serialized);
  fflush(NULL);
  taosMemoryFree(serialized);
}

void syncPingPrint2(char* s, const SyncPing* pMsg) {
  char* serialized = syncPing2Str(pMsg);
  printf("syncPingPrint2 | len:%d | %s | %s \n", (int32_t)strlen(serialized), s, serialized);
  fflush(NULL);
  taosMemoryFree(serialized);
}

void syncPingLog(const SyncPing* pMsg) {
  char* serialized = syncPing2Str(pMsg);
  sTrace("syncPingLog | len:%d | %s", (int32_t)strlen(serialized), serialized);
  taosMemoryFree(serialized);
}

void syncPingLog2(char* s, const SyncPing* pMsg) {
  if (gRaftDetailLog) {
    char* serialized = syncPing2Str(pMsg);
    sTrace("syncPingLog2 | len:%d | %s | %s", (int32_t)strlen(serialized), s, serialized);
    taosMemoryFree(serialized);
  }
}

// ---------------------------------------------
cJSON* syncRpcMsg2Json(SRpcMsg* pRpcMsg) {
  cJSON* pRoot;

  // in compiler optimization, switch case = if else constants
  if (pRpcMsg->msgType == TDMT_SYNC_TIMEOUT) {
    SyncTimeout* pSyncMsg = syncTimeoutDeserialize2(pRpcMsg->pCont, pRpcMsg->contLen);
    pRoot = syncTimeout2Json(pSyncMsg);
    syncTimeoutDestroy(pSyncMsg);

  } else if (pRpcMsg->msgType == TDMT_SYNC_PING) {
    SyncPing* pSyncMsg = syncPingDeserialize2(pRpcMsg->pCont, pRpcMsg->contLen);
    pRoot = syncPing2Json(pSyncMsg);
    syncPingDestroy(pSyncMsg);

  } else if (pRpcMsg->msgType == TDMT_SYNC_PING_REPLY) {
    SyncPingReply* pSyncMsg = syncPingReplyDeserialize2(pRpcMsg->pCont, pRpcMsg->contLen);
    pRoot = syncPingReply2Json(pSyncMsg);
    syncPingReplyDestroy(pSyncMsg);

  } else if (pRpcMsg->msgType == TDMT_SYNC_CLIENT_REQUEST) {
    SyncClientRequest* pSyncMsg = pRpcMsg->pCont;
    pRoot = syncClientRequest2Json(pSyncMsg);

  } else if (pRpcMsg->msgType == TDMT_SYNC_CLIENT_REQUEST_REPLY) {
    pRoot = syncRpcUnknownMsg2Json();

  } else if (pRpcMsg->msgType == TDMT_SYNC_REQUEST_VOTE) {
    SyncRequestVote* pSyncMsg = syncRequestVoteDeserialize2(pRpcMsg->pCont, pRpcMsg->contLen);
    pRoot = syncRequestVote2Json(pSyncMsg);
    syncRequestVoteDestroy(pSyncMsg);

  } else if (pRpcMsg->msgType == TDMT_SYNC_REQUEST_VOTE_REPLY) {
    SyncRequestVoteReply* pSyncMsg = syncRequestVoteReplyDeserialize2(pRpcMsg->pCont, pRpcMsg->contLen);
    pRoot = syncRequestVoteReply2Json(pSyncMsg);
    syncRequestVoteReplyDestroy(pSyncMsg);

  } else if (pRpcMsg->msgType == TDMT_SYNC_APPEND_ENTRIES) {
    SyncAppendEntries* pSyncMsg = syncAppendEntriesDeserialize2(pRpcMsg->pCont, pRpcMsg->contLen);
    pRoot = syncAppendEntries2Json(pSyncMsg);
    syncAppendEntriesDestroy(pSyncMsg);

  } else if (pRpcMsg->msgType == TDMT_SYNC_APPEND_ENTRIES_REPLY) {
    SyncAppendEntriesReply* pSyncMsg = syncAppendEntriesReplyDeserialize2(pRpcMsg->pCont, pRpcMsg->contLen);
    pRoot = syncAppendEntriesReply2Json(pSyncMsg);
    syncAppendEntriesReplyDestroy(pSyncMsg);

  } else if (pRpcMsg->msgType == TDMT_SYNC_SNAPSHOT_SEND) {
    SyncSnapshotSend* pSyncMsg = syncSnapshotSendDeserialize2(pRpcMsg->pCont, pRpcMsg->contLen);
    pRoot = syncSnapshotSend2Json(pSyncMsg);
    syncSnapshotSendDestroy(pSyncMsg);

  } else if (pRpcMsg->msgType == TDMT_SYNC_SNAPSHOT_RSP) {
    SyncSnapshotRsp* pSyncMsg = syncSnapshotRspDeserialize2(pRpcMsg->pCont, pRpcMsg->contLen);
    pRoot = syncSnapshotRsp2Json(pSyncMsg);
    syncSnapshotRspDestroy(pSyncMsg);

  } else if (pRpcMsg->msgType == TDMT_SYNC_LEADER_TRANSFER) {
    SyncLeaderTransfer* pSyncMsg = syncLeaderTransferDeserialize2(pRpcMsg->pCont, pRpcMsg->contLen);
    pRoot = syncLeaderTransfer2Json(pSyncMsg);
    syncLeaderTransferDestroy(pSyncMsg);

  } else if (pRpcMsg->msgType == TDMT_SYNC_COMMON_RESPONSE) {
    pRoot = cJSON_CreateObject();
    char* s;
    s = syncUtilPrintBin((char*)(pRpcMsg->pCont), pRpcMsg->contLen);
    cJSON_AddStringToObject(pRoot, "pCont", s);
    taosMemoryFree(s);
    s = syncUtilPrintBin2((char*)(pRpcMsg->pCont), pRpcMsg->contLen);
    cJSON_AddStringToObject(pRoot, "pCont2", s);
    taosMemoryFree(s);

  } else {
    pRoot = cJSON_CreateObject();
    char* s;
    s = syncUtilPrintBin((char*)(pRpcMsg->pCont), pRpcMsg->contLen);
    cJSON_AddStringToObject(pRoot, "pCont", s);
    taosMemoryFree(s);
    s = syncUtilPrintBin2((char*)(pRpcMsg->pCont), pRpcMsg->contLen);
    cJSON_AddStringToObject(pRoot, "pCont2", s);
    taosMemoryFree(s);
  }

  cJSON_AddNumberToObject(pRoot, "msgType", pRpcMsg->msgType);
  cJSON_AddNumberToObject(pRoot, "contLen", pRpcMsg->contLen);
  cJSON_AddNumberToObject(pRoot, "code", pRpcMsg->code);
  // cJSON_AddNumberToObject(pRoot, "persist", pRpcMsg->persist);

  cJSON* pJson = cJSON_CreateObject();
  cJSON_AddItemToObject(pJson, "RpcMsg", pRoot);
  return pJson;
}

cJSON* syncRpcUnknownMsg2Json() {
  cJSON* pRoot = cJSON_CreateObject();
  cJSON_AddNumberToObject(pRoot, "msgType", TDMT_SYNC_UNKNOWN);
  cJSON_AddStringToObject(pRoot, "data", "unknown message");

  cJSON* pJson = cJSON_CreateObject();
  cJSON_AddItemToObject(pJson, "SyncUnknown", pRoot);
  return pJson;
}

char* syncRpcMsg2Str(SRpcMsg* pRpcMsg) {
  cJSON* pJson = syncRpcMsg2Json(pRpcMsg);
  char*  serialized = cJSON_Print(pJson);
  cJSON_Delete(pJson);
  return serialized;
}

// for debug ----------------------
void syncRpcMsgPrint(SRpcMsg* pMsg) {
  char* serialized = syncRpcMsg2Str(pMsg);
  printf("syncRpcMsgPrint | len:%d | %s \n", (int32_t)strlen(serialized), serialized);
  fflush(NULL);
  taosMemoryFree(serialized);
}

void syncRpcMsgPrint2(char* s, SRpcMsg* pMsg) {
  char* serialized = syncRpcMsg2Str(pMsg);
  printf("syncRpcMsgPrint2 | len:%d | %s | %s \n", (int32_t)strlen(serialized), s, serialized);
  fflush(NULL);
  taosMemoryFree(serialized);
}

void syncRpcMsgLog(SRpcMsg* pMsg) {
  char* serialized = syncRpcMsg2Str(pMsg);
  sTrace("syncRpcMsgLog | len:%d | %s", (int32_t)strlen(serialized), serialized);
  taosMemoryFree(serialized);
}

void syncRpcMsgLog2(char* s, SRpcMsg* pMsg) {
  if (gRaftDetailLog) {
    char* serialized = syncRpcMsg2Str(pMsg);
    sTrace("syncRpcMsgLog2 | len:%d | %s | %s", (int32_t)strlen(serialized), s, serialized);
    taosMemoryFree(serialized);
  }
}

cJSON* syncAppendEntriesBatch2Json(const SyncAppendEntriesBatch* pMsg) {
  char   u64buf[128] = {0};
  cJSON* pRoot = cJSON_CreateObject();

  if (pMsg != NULL) {
    cJSON_AddNumberToObject(pRoot, "bytes", pMsg->bytes);
    cJSON_AddNumberToObject(pRoot, "vgId", pMsg->vgId);
    cJSON_AddNumberToObject(pRoot, "msgType", pMsg->msgType);

    cJSON* pSrcId = cJSON_CreateObject();
    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pMsg->srcId.addr);
    cJSON_AddStringToObject(pSrcId, "addr", u64buf);
    {
      uint64_t u64 = pMsg->srcId.addr;
      cJSON*   pTmp = pSrcId;
      char     host[128] = {0};
      uint16_t port;
      syncUtilU642Addr(u64, host, sizeof(host), &port);
      cJSON_AddStringToObject(pTmp, "addr_host", host);
      cJSON_AddNumberToObject(pTmp, "addr_port", port);
    }
    cJSON_AddNumberToObject(pSrcId, "vgId", pMsg->srcId.vgId);
    cJSON_AddItemToObject(pRoot, "srcId", pSrcId);

    cJSON* pDestId = cJSON_CreateObject();
    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pMsg->destId.addr);
    cJSON_AddStringToObject(pDestId, "addr", u64buf);
    {
      uint64_t u64 = pMsg->destId.addr;
      cJSON*   pTmp = pDestId;
      char     host[128] = {0};
      uint16_t port;
      syncUtilU642Addr(u64, host, sizeof(host), &port);
      cJSON_AddStringToObject(pTmp, "addr_host", host);
      cJSON_AddNumberToObject(pTmp, "addr_port", port);
    }
    cJSON_AddNumberToObject(pDestId, "vgId", pMsg->destId.vgId);
    cJSON_AddItemToObject(pRoot, "destId", pDestId);

    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pMsg->term);
    cJSON_AddStringToObject(pRoot, "term", u64buf);

    snprintf(u64buf, sizeof(u64buf), "%" PRId64, pMsg->prevLogIndex);
    cJSON_AddStringToObject(pRoot, "prevLogIndex", u64buf);

    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pMsg->prevLogTerm);
    cJSON_AddStringToObject(pRoot, "prevLogTerm", u64buf);

    snprintf(u64buf, sizeof(u64buf), "%" PRId64, pMsg->commitIndex);
    cJSON_AddStringToObject(pRoot, "commitIndex", u64buf);

    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pMsg->privateTerm);
    cJSON_AddStringToObject(pRoot, "privateTerm", u64buf);

    cJSON_AddNumberToObject(pRoot, "dataCount", pMsg->dataCount);
    cJSON_AddNumberToObject(pRoot, "dataLen", pMsg->dataLen);

    int32_t metaArrayLen = sizeof(SOffsetAndContLen) * pMsg->dataCount;  // <offset, contLen>
    int32_t entryArrayLen = pMsg->dataLen - metaArrayLen;

    cJSON_AddNumberToObject(pRoot, "metaArrayLen", metaArrayLen);
    cJSON_AddNumberToObject(pRoot, "entryArrayLen", entryArrayLen);

    SOffsetAndContLen* metaArr = (SOffsetAndContLen*)(pMsg->data);

    cJSON* pMetaArr = cJSON_CreateArray();
    cJSON_AddItemToObject(pRoot, "metaArr", pMetaArr);
    for (int i = 0; i < pMsg->dataCount; ++i) {
      cJSON* pMeta = cJSON_CreateObject();
      cJSON_AddNumberToObject(pMeta, "offset", metaArr[i].offset);
      cJSON_AddNumberToObject(pMeta, "contLen", metaArr[i].contLen);
      cJSON_AddItemToArray(pMetaArr, pMeta);
    }

    cJSON* pEntryArr = cJSON_CreateArray();
    cJSON_AddItemToObject(pRoot, "entryArr", pEntryArr);
    for (int i = 0; i < pMsg->dataCount; ++i) {
      SSyncRaftEntry* pEntry = (SSyncRaftEntry*)(pMsg->data + metaArr[i].offset);
      cJSON*          pEntryJson = syncEntry2Json(pEntry);
      cJSON_AddItemToArray(pEntryArr, pEntryJson);
    }

    char* s;
    s = syncUtilPrintBin((char*)(pMsg->data), pMsg->dataLen);
    cJSON_AddStringToObject(pRoot, "data", s);
    taosMemoryFree(s);
    s = syncUtilPrintBin2((char*)(pMsg->data), pMsg->dataLen);
    cJSON_AddStringToObject(pRoot, "data2", s);
    taosMemoryFree(s);
  }

  cJSON* pJson = cJSON_CreateObject();
  cJSON_AddItemToObject(pJson, "SyncAppendEntriesBatch", pRoot);
  return pJson;
}

char* syncAppendEntriesBatch2Str(const SyncAppendEntriesBatch* pMsg) {
  cJSON* pJson = syncAppendEntriesBatch2Json(pMsg);
  char*  serialized = cJSON_Print(pJson);
  cJSON_Delete(pJson);
  return serialized;
}

// for debug ----------------------
void syncAppendEntriesBatchPrint(const SyncAppendEntriesBatch* pMsg) {
  char* serialized = syncAppendEntriesBatch2Str(pMsg);
  printf("syncAppendEntriesBatchPrint | len:%d | %s \n", (int32_t)strlen(serialized), serialized);
  fflush(NULL);
  taosMemoryFree(serialized);
}

void syncAppendEntriesBatchPrint2(char* s, const SyncAppendEntriesBatch* pMsg) {
  char* serialized = syncAppendEntriesBatch2Str(pMsg);
  printf("syncAppendEntriesBatchPrint2 | len:%d | %s | %s \n", (int32_t)strlen(serialized), s, serialized);
  fflush(NULL);
  taosMemoryFree(serialized);
}

void syncAppendEntriesBatchLog(const SyncAppendEntriesBatch* pMsg) {
  char* serialized = syncAppendEntriesBatch2Str(pMsg);
  sTrace("syncAppendEntriesBatchLog | len:%d | %s", (int32_t)strlen(serialized), serialized);
  taosMemoryFree(serialized);
}

void syncAppendEntriesBatchLog2(char* s, const SyncAppendEntriesBatch* pMsg) {
  if (gRaftDetailLog) {
    char* serialized = syncAppendEntriesBatch2Str(pMsg);
    sLTrace("syncAppendEntriesBatchLog2 | len:%d | %s | %s", (int32_t)strlen(serialized), s, serialized);
    taosMemoryFree(serialized);
  }
}

cJSON* syncSnapshotSend2Json(const SyncSnapshotSend* pMsg) {
  char   u64buf[128];
  cJSON* pRoot = cJSON_CreateObject();

  if (pMsg != NULL) {
    cJSON_AddNumberToObject(pRoot, "bytes", pMsg->bytes);
    cJSON_AddNumberToObject(pRoot, "vgId", pMsg->vgId);
    cJSON_AddNumberToObject(pRoot, "msgType", pMsg->msgType);

    cJSON* pSrcId = cJSON_CreateObject();
    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pMsg->srcId.addr);
    cJSON_AddStringToObject(pSrcId, "addr", u64buf);
    {
      uint64_t u64 = pMsg->srcId.addr;
      cJSON*   pTmp = pSrcId;
      char     host[128];
      uint16_t port;
      syncUtilU642Addr(u64, host, sizeof(host), &port);
      cJSON_AddStringToObject(pTmp, "addr_host", host);
      cJSON_AddNumberToObject(pTmp, "addr_port", port);
    }
    cJSON_AddNumberToObject(pSrcId, "vgId", pMsg->srcId.vgId);
    cJSON_AddItemToObject(pRoot, "srcId", pSrcId);

    cJSON* pDestId = cJSON_CreateObject();
    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pMsg->destId.addr);
    cJSON_AddStringToObject(pDestId, "addr", u64buf);
    {
      uint64_t u64 = pMsg->destId.addr;
      cJSON*   pTmp = pDestId;
      char     host[128];
      uint16_t port;
      syncUtilU642Addr(u64, host, sizeof(host), &port);
      cJSON_AddStringToObject(pTmp, "addr_host", host);
      cJSON_AddNumberToObject(pTmp, "addr_port", port);
    }
    cJSON_AddNumberToObject(pDestId, "vgId", pMsg->destId.vgId);
    cJSON_AddItemToObject(pRoot, "destId", pDestId);

    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pMsg->term);
    cJSON_AddStringToObject(pRoot, "term", u64buf);

    snprintf(u64buf, sizeof(u64buf), "%" PRId64, pMsg->startTime);
    cJSON_AddStringToObject(pRoot, "startTime", u64buf);

    snprintf(u64buf, sizeof(u64buf), "%" PRId64, pMsg->beginIndex);
    cJSON_AddStringToObject(pRoot, "beginIndex", u64buf);

    snprintf(u64buf, sizeof(u64buf), "%" PRId64, pMsg->lastIndex);
    cJSON_AddStringToObject(pRoot, "lastIndex", u64buf);

    snprintf(u64buf, sizeof(u64buf), "%" PRId64, pMsg->lastConfigIndex);
    cJSON_AddStringToObject(pRoot, "lastConfigIndex", u64buf);
    cJSON_AddItemToObject(pRoot, "lastConfig", syncCfg2Json((SSyncCfg*)&(pMsg->lastConfig)));

    snprintf(u64buf, sizeof(u64buf), "%" PRIu64, pMsg->lastTerm);
    cJSON_AddStringToObject(pRoot, "lastTerm", u64buf);

    cJSON_AddNumberToObject(pRoot, "seq", pMsg->seq);

    cJSON_AddNumberToObject(pRoot, "dataLen", pMsg->dataLen);
    char* s;
    s = syncUtilPrintBin((char*)(pMsg->data), pMsg->dataLen);
    cJSON_AddStringToObject(pRoot, "data", s);
    taosMemoryFree(s);
    s = syncUtilPrintBin2((char*)(pMsg->data), pMsg->dataLen);
    cJSON_AddStringToObject(pRoot, "data2", s);
    taosMemoryFree(s);
  }

  cJSON* pJson = cJSON_CreateObject();
  cJSON_AddItemToObject(pJson, "SyncSnapshotSend", pRoot);
  return pJson;
}

char* syncSnapshotSend2Str(const SyncSnapshotSend* pMsg) {
  cJSON* pJson = syncSnapshotSend2Json(pMsg);
  char*  serialized = cJSON_Print(pJson);
  cJSON_Delete(pJson);
  return serialized;
}

// for debug ----------------------
void syncSnapshotSendPrint(const SyncSnapshotSend* pMsg) {
  char* serialized = syncSnapshotSend2Str(pMsg);
  printf("syncSnapshotSendPrint | len:%d | %s \n", (int32_t)strlen(serialized), serialized);
  fflush(NULL);
  taosMemoryFree(serialized);
}

void syncSnapshotSendPrint2(char* s, const SyncSnapshotSend* pMsg) {
  char* serialized = syncSnapshotSend2Str(pMsg);
  printf("syncSnapshotSendPrint2 | len:%d | %s | %s \n", (int32_t)strlen(serialized), s, serialized);
  fflush(NULL);
  taosMemoryFree(serialized);
}

void syncSnapshotSendLog(const SyncSnapshotSend* pMsg) {
  char* serialized = syncSnapshotSend2Str(pMsg);
  sTrace("syncSnapshotSendLog | len:%d | %s", (int32_t)strlen(serialized), serialized);
  taosMemoryFree(serialized);
}

void syncSnapshotSendLog2(char* s, const SyncSnapshotSend* pMsg) {
  if (gRaftDetailLog) {
    char* serialized = syncSnapshotSend2Str(pMsg);
    sTrace("syncSnapshotSendLog2 | len:%d | %s | %s", (int32_t)strlen(serialized), s, serialized);
    taosMemoryFree(serialized);
  }
}

cJSON* syncClientRequestBatch2Json(const SyncClientRequestBatch* pMsg) {
  char   u64buf[128] = {0};
  cJSON* pRoot = cJSON_CreateObject();

  if (pMsg != NULL) {
    cJSON_AddNumberToObject(pRoot, "bytes", pMsg->bytes);
    cJSON_AddNumberToObject(pRoot, "vgId", pMsg->vgId);
    cJSON_AddNumberToObject(pRoot, "msgType", pMsg->msgType);
    cJSON_AddNumberToObject(pRoot, "dataLen", pMsg->dataLen);
    cJSON_AddNumberToObject(pRoot, "dataCount", pMsg->dataCount);

    SRaftMeta* metaArr = syncClientRequestBatchMetaArr(pMsg);
    SRpcMsg*   msgArr = syncClientRequestBatchRpcMsgArr(pMsg);

    cJSON* pMetaArr = cJSON_CreateArray();
    cJSON_AddItemToObject(pRoot, "metaArr", pMetaArr);
    for (int i = 0; i < pMsg->dataCount; ++i) {
      cJSON* pMeta = cJSON_CreateObject();
      cJSON_AddNumberToObject(pMeta, "seqNum", metaArr[i].seqNum);
      cJSON_AddNumberToObject(pMeta, "isWeak", metaArr[i].isWeak);
      cJSON_AddItemToArray(pMetaArr, pMeta);
    }

    cJSON* pMsgArr = cJSON_CreateArray();
    cJSON_AddItemToObject(pRoot, "msgArr", pMsgArr);
    for (int i = 0; i < pMsg->dataCount; ++i) {
      cJSON* pRpcMsgJson = syncRpcMsg2Json(&msgArr[i]);
      cJSON_AddItemToArray(pMsgArr, pRpcMsgJson);
    }

    char* s;
    s = syncUtilPrintBin((char*)(pMsg->data), pMsg->dataLen);
    cJSON_AddStringToObject(pRoot, "data", s);
    taosMemoryFree(s);
    s = syncUtilPrintBin2((char*)(pMsg->data), pMsg->dataLen);
    cJSON_AddStringToObject(pRoot, "data2", s);
    taosMemoryFree(s);
  }

  cJSON* pJson = cJSON_CreateObject();
  cJSON_AddItemToObject(pJson, "SyncClientRequestBatch", pRoot);
  return pJson;
}

char* syncClientRequestBatch2Str(const SyncClientRequestBatch* pMsg) {
  cJSON* pJson = syncClientRequestBatch2Json(pMsg);
  char*  serialized = cJSON_Print(pJson);
  cJSON_Delete(pJson);
  return serialized;
}

// for debug ----------------------
void syncClientRequestBatchPrint(const SyncClientRequestBatch* pMsg) {
  char* serialized = syncClientRequestBatch2Str(pMsg);
  printf("syncClientRequestBatchPrint | len:%d | %s \n", (int32_t)strlen(serialized), serialized);
  fflush(NULL);
  taosMemoryFree(serialized);
}

void syncClientRequestBatchPrint2(char* s, const SyncClientRequestBatch* pMsg) {
  char* serialized = syncClientRequestBatch2Str(pMsg);
  printf("syncClientRequestBatchPrint2 | len:%d | %s | %s \n", (int32_t)strlen(serialized), s, serialized);
  fflush(NULL);
  taosMemoryFree(serialized);
}

void syncClientRequestBatchLog(const SyncClientRequestBatch* pMsg) {
  char* serialized = syncClientRequestBatch2Str(pMsg);
  sTrace("syncClientRequestBatchLog | len:%d | %s", (int32_t)strlen(serialized), serialized);
  taosMemoryFree(serialized);
}

void syncClientRequestBatchLog2(char* s, const SyncClientRequestBatch* pMsg) {
  if (gRaftDetailLog) {
    char* serialized = syncClientRequestBatch2Str(pMsg);
    sLTrace("syncClientRequestBatchLog2 | len:%d | %s | %s", (int32_t)strlen(serialized), s, serialized);
    taosMemoryFree(serialized);
  }
}

// for debug ----------------------
void syncClientRequestPrint(const SyncClientRequest* pMsg) {
  char* serialized = syncClientRequest2Str(pMsg);
  printf("syncClientRequestPrint | len:%d | %s \n", (int32_t)strlen(serialized), serialized);
  fflush(NULL);
  taosMemoryFree(serialized);
}

void syncClientRequestPrint2(char* s, const SyncClientRequest* pMsg) {
  char* serialized = syncClientRequest2Str(pMsg);
  printf("syncClientRequestPrint2 | len:%d | %s | %s \n", (int32_t)strlen(serialized), s, serialized);
  fflush(NULL);
  taosMemoryFree(serialized);
}

void syncClientRequestLog(const SyncClientRequest* pMsg) {
  char* serialized = syncClientRequest2Str(pMsg);
  sTrace("syncClientRequestLog | len:%d | %s", (int32_t)strlen(serialized), serialized);
  taosMemoryFree(serialized);
}

void syncClientRequestLog2(char* s, const SyncClientRequest* pMsg) {
  if (gRaftDetailLog) {
    char* serialized = syncClientRequest2Str(pMsg);
    sTrace("syncClientRequestLog2 | len:%d | %s | %s", (int32_t)strlen(serialized), s, serialized);
    taosMemoryFree(serialized);
  }
}
