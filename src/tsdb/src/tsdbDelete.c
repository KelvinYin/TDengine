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
#include "tsdbint.h"
#include "tsdbDelete.h"

enum {
  TSDB_NO_DELETE,
  TSDB_IN_DELETE,
  TSDB_WAITING_DELETE,
};

enum BlockSolve {
  BLOCK_READ = 0,
  BLOCK_MODIFY,
  BLOCK_DELETE
};

typedef struct {
  STable *    pTable;
  SBlockIdx * pBlkIdx;
  SBlockIdx   bIndex;
  SBlockInfo *pInfo;
  bool        update; // need update lastrow
} STableDeleteH;

typedef struct {
  STsdbRepo *pRepo;
  SRtn       rtn;
  SFSIter    fsIter;
  SArray *   tblArray;  // STableDeleteH, table array to cache table obj and block indexes
  SReadH     readh;
  SDFileSet  wSet;
  SArray *   aBlkIdx;
  SArray *   aSupBlk;
  SArray *   aSubBlk;
  SDataCols *pDCols;
  SControlDataInfo* pCtlInfo;
  SArray *   aUpdates;
  SArray *   aAffectTables;
} SDeleteH;


#define TSDB_DELETE_WSET(pdh) (&((pdh)->wSet))
#define TSDB_DELETE_REPO(pdh) TSDB_READ_REPO(&((pdh)->readh))
#define TSDB_DELETE_HEAD_FILE(pdh) TSDB_DFILE_IN_SET(TSDB_DELETE_WSET(pdh), TSDB_FILE_HEAD)
#define TSDB_DELETE_DATA_FILE(pdh) TSDB_DFILE_IN_SET(TSDB_DELETE_WSET(pdh), TSDB_FILE_DATA)
#define TSDB_DELETE_LAST_FILE(pdh) TSDB_DFILE_IN_SET(TSDB_DELETE_WSET(pdh), TSDB_FILE_LAST)
#define TSDB_DELETE_SMAD_FILE(pdh) TSDB_DFILE_IN_SET(TSDB_DELETE_WSET(pdh), TSDB_FILE_SMAD)
#define TSDB_DELETE_SMAL_FILE(pdh) TSDB_DFILE_IN_SET(TSDB_DELETE_WSET(pdh), TSDB_FILE_SMAL)
#define TSDB_DELETE_BUF(pdh) TSDB_READ_BUF(&((pdh)->readh))
#define TSDB_DELETE_COMP_BUF(pdh) TSDB_READ_COMP_BUF(&((pdh)->readh))
#define TSDB_DELETE_EXBUF(pdh) TSDB_READ_EXBUF(&((pdh)->readh))


static void  tsdbStartDeleteTrans(STsdbRepo *pRepo);
static void  tsdbEndDeleteTrans(STsdbRepo *pRepo, int eno);
static int   tsdbDeleteTSData(STsdbRepo *pRepo, SControlDataInfo* pCtlInfo, SArray* pArray, SArray* pAffectTables);
static int   tsdbFSetDelete(SDeleteH *pdh, SDFileSet *pSet);
static int   tsdbInitDeleteH(SDeleteH *pdh, STsdbRepo *pRepo);
static void  tsdbDestroyDeleteH(SDeleteH *pdh);
static int   tsdbInitDeleteTblArray(SDeleteH *pdh);
static void  tsdbDestroyDeleteTblArray(SDeleteH *pdh);
static int   tsdbCacheFSetIndex(SDeleteH *pdh);
static int   tsdbFSetInit(SDeleteH *pdh, SDFileSet *pSet);
static void  tsdbDeleteFSetEnd(SDeleteH *pdh);
static int   tsdbFSetDeleteImpl(SDeleteH *pdh);
static int   tsdbBlockSolve(SDeleteH *pdh, SBlock *pBlock);
static int   tsdbWriteBlockToFile(SDeleteH *pdh, STable *pTable, SDataCols *pDCols, void **ppBuf,
                                       void **ppCBuf, void **ppExBuf, SBlock * pBlock);
static int   tsdbDeleteImplCommon(STsdbRepo *pRepo, SControlDataInfo* pCtlInfo);


// delete
int tsdbControlDelete(STsdbRepo* pRepo, SControlDataInfo* pCtlInfo) {
  int32_t ret = tsdbDeleteImplCommon(pRepo, pCtlInfo);
  if(pCtlInfo->pRsp) {
    pCtlInfo->pRsp->affectedRows = htonl(pCtlInfo->pRsp->affectedRows);
    pCtlInfo->pRsp->numOfTables  = htonl(pCtlInfo->pRsp->numOfTables);
    pCtlInfo->pRsp->code = ret;
  }

  return ret;
}

static void tsdbUpdateLastRow(STsdbRepo* pRepo, SArray * pArray) {
  size_t cnt = taosArrayGetSize(pArray);
  for (size_t i = 0; i < cnt; ++i) {
    STable* pTable = taosArrayGetP(pArray, i);
    tsdbLoadLastCache(pRepo, pTable, true);
  }
}

static void tsdbClearUpdates(SArray * pArray) {
  size_t cnt = taosArrayGetSize(pArray);
  for (size_t i = 0; i < cnt; ++i) {
    STable* pTable = taosArrayGetP(pArray, i);
    tsdbUnRefTable(pTable);
  }
  // destory
  taosArrayDestroy(&pArray);
}

static int tsdbDeleteMeta(STsdbRepo *pRepo) {
  STsdbFS *pfs = REPO_FS(pRepo);
  tsdbUpdateMFile(pfs, pfs->cstatus->pmf);
  return TSDB_CODE_SUCCESS;
}

static int tsdbDeleteImplCommon(STsdbRepo *pRepo, SControlDataInfo* pCtlInfo) {
  // check valid
  if ((REPO_FS(pRepo)->cstatus->pmf == NULL) || (taosArrayGetSize(REPO_FS(pRepo)->cstatus->df) <= 0)) {
    pRepo->deleteState = TSDB_NO_DELETE;
    tsem_post(&(pRepo->readyToCommit));
    tsdbInfo("vgId:%d delete over, no meta or data file", REPO_ID(pRepo));
    return -1;
  }

  SArray* aUpdates = taosArrayInit(10, sizeof(STable *));
  SArray* affectedTables = taosArrayInit(10, sizeof(int32_t)); // put tid

  // start transaction
  tsdbStartDeleteTrans(pRepo);

  if (tsdbDeleteMeta(pRepo) < 0) {
    tsdbError("vgId:%d failed to delete META data since %s", REPO_ID(pRepo), tstrerror(terrno));
    goto _err;
  }

  if (tsdbDeleteTSData(pRepo, pCtlInfo, aUpdates, affectedTables) < 0) {
    tsdbError("vgId:%d failed to delete TS data since %s", REPO_ID(pRepo), tstrerror(terrno));
    goto _err;
  }

  // end transaction
  tsdbEndDeleteTrans(pRepo, TSDB_CODE_SUCCESS);

  // set affected tables number
  if(pCtlInfo->pRsp) {
    pCtlInfo->pRsp->numOfTables = (int32_t)taosArrayGetSize(affectedTables);
  }

  // update last row
  tsdbUpdateLastRow(pRepo, aUpdates);
  tsdbClearUpdates(aUpdates);
  taosArrayDestroy(&affectedTables);
  return TSDB_CODE_SUCCESS;

_err:
  pRepo->code = terrno;
  tsdbEndDeleteTrans(pRepo, terrno);
  tsdbClearUpdates(aUpdates);
  taosArrayDestroy(&affectedTables);
  return -1;
}

static void tsdbStartDeleteTrans(STsdbRepo *pRepo) {
  assert(pRepo->deleteState != TSDB_IN_DELETE);
  tsdbInfo("vgId:%d start to delete!", REPO_ID(pRepo));
  tsdbStartFSTxn(pRepo, 0, 0);
  pRepo->code = TSDB_CODE_SUCCESS;
  pRepo->deleteState = TSDB_IN_DELETE;
}

static void tsdbEndDeleteTrans(STsdbRepo *pRepo, int eno) {
  if (eno != TSDB_CODE_SUCCESS) {
    tsdbEndFSTxnWithError(REPO_FS(pRepo));
  } else {
    tsdbEndFSTxn(pRepo);
  }
  pRepo->deleteState = TSDB_NO_DELETE;
  tsdbInfo("vgId:%d delete over, %s", REPO_ID(pRepo), (eno == TSDB_CODE_SUCCESS) ? "succeed" : "failed");
  tsem_post(&(pRepo->readyToCommit));
}

static int tsdbDeleteTSData(STsdbRepo *pRepo, SControlDataInfo* pCtlInfo, SArray* pArray, SArray* pAffectTables) {
  STsdbCfg *       pCfg = REPO_CFG(pRepo);
  SDeleteH         deleteH = {0};
  SDFileSet *      pSet = NULL;

  tsdbDebug("vgId:%d start to delete TS data for %d", REPO_ID(pRepo), pCtlInfo->tids[0]);

  if (tsdbInitDeleteH(&deleteH, pRepo) < 0) {
    return -1;
  }

  deleteH.aUpdates = pArray;
  deleteH.pCtlInfo = pCtlInfo;
  STimeWindow win  = pCtlInfo->win;
  deleteH.aAffectTables = pAffectTables;

  int sFid = TSDB_KEY_FID(win.skey, pCfg->daysPerFile, pCfg->precision);
  int eFid = TSDB_KEY_FID(win.ekey, pCfg->daysPerFile, pCfg->precision);
  ASSERT(sFid <= eFid);

  while ((pSet = tsdbFSIterNext(&(deleteH.fsIter)))) {
    // remove expired files
    if (pSet->fid < deleteH.rtn.minFid) {
      tsdbInfo("vgId:%d FSET %d on level %d disk id %d expires, remove it", REPO_ID(pRepo), pSet->fid,
               TSDB_FSET_LEVEL(pSet), TSDB_FSET_ID(pSet));
      continue;
    }

    if ((pSet->fid < sFid) || (pSet->fid > eFid)) {
      tsdbDebug("vgId:%d no need to delete FSET %d, sFid %d, eFid %d", REPO_ID(pRepo), pSet->fid, sFid, eFid);
      if (tsdbApplyRtnOnFSet(pRepo, pSet, &(deleteH.rtn)) < 0) {
        return -1;
      }
      continue;
    }

#if 0  // TODO: How to make the decision? The test case should cover this scenario.
    if (TSDB_FSET_LEVEL(pSet) == TFS_MAX_LEVEL) {
      tsdbDebug("vgId:%d FSET %d on level %d, should not delete", REPO_ID(pRepo), pSet->fid, TFS_MAX_LEVEL);
      tsdbUpdateDFileSet(REPO_FS(pRepo), pSet);
      continue;
    }
#endif
    
    if (pCtlInfo->command & CMD_DELETE_DATA) {
      if (tsdbFSetDelete(&deleteH, pSet) < 0) {
        tsdbDestroyDeleteH(&deleteH);
        tsdbError("vgId:%d failed to delete data in FSET %d since %s", REPO_ID(pRepo), pSet->fid, tstrerror(terrno));
        return -1;
      }
    } else {
      ASSERT(false);
    }
    
  }

  tsdbDestroyDeleteH(&deleteH);
  tsdbDebug("vgId:%d delete TS data over", REPO_ID(pRepo));
  return 0;
}

static int tsdbFSetDelete(SDeleteH *pdh, SDFileSet *pSet) {
  STsdbRepo *pRepo = TSDB_DELETE_REPO(pdh);
  SDiskID    did = {0};
  SDFileSet *pWSet = TSDB_DELETE_WSET(pdh);

  tsdbDebug("vgId:%d start to delete data in FSET %d on level %d id %d", REPO_ID(pRepo), pSet->fid,
            TSDB_FSET_LEVEL(pSet), TSDB_FSET_ID(pSet));

  if (tsdbFSetInit(pdh, pSet) < 0) {
    tsdbError("vgId:%d fset init failed. FSET %d on level %d id %d", REPO_ID(pRepo), pSet->fid,
            TSDB_FSET_LEVEL(pSet), TSDB_FSET_ID(pSet));
    return -1;
  }

  // Create new fset as deleted fset
  tfsAllocDisk(tsdbGetFidLevel(pSet->fid, &(pdh->rtn)), &(did.level), &(did.id));
  if (did.level == TFS_UNDECIDED_LEVEL) {
    terrno = TSDB_CODE_TDB_NO_AVAIL_DISK;
    tsdbError("vgId:%d failed to delete table in FSET %d since %s", REPO_ID(pRepo), pSet->fid, tstrerror(terrno));
    tsdbDeleteFSetEnd(pdh);
    return -1;
  }

  // Only .head is created, use original .data/.last/.smad/.smal
  tsdbInitDFileSetEx(pWSet, pSet);
  pWSet->state = 0;
  SDFile *pHeadFile = TSDB_DFILE_IN_SET(pWSet, TSDB_FILE_HEAD);
  tsdbInitDFile(pHeadFile, did, REPO_ID(pRepo), TSDB_FSET_FID(pSet), FS_TXN_VERSION(REPO_FS(pRepo)), TSDB_FILE_HEAD);

  if (tsdbCreateDFile(pHeadFile, true, TSDB_FILE_HEAD) < 0) {
    tsdbError("vgId:%d failed to delete table in FSET %d since %s", REPO_ID(pRepo), pSet->fid, tstrerror(terrno));
    tsdbCloseDFile(pHeadFile);
    tsdbRemoveDFile(pHeadFile);
    return -1;
  }

  tsdbCloseDFile(pHeadFile);

  if (tsdbOpenDFileSet(pWSet, O_RDWR) < 0) {
    tsdbError("vgId:%d failed to open file set %d since %s", REPO_ID(pRepo), TSDB_FSET_FID(pWSet), tstrerror(terrno));
    return -1;
  }

  if (tsdbFSetDeleteImpl(pdh) < 0) {
    tsdbCloseDFileSet(TSDB_DELETE_WSET(pdh));
    tsdbRemoveDFileSet(TSDB_DELETE_WSET(pdh));
    tsdbDeleteFSetEnd(pdh);
    return -1;
  }

  tsdbCloseDFileSet(TSDB_DELETE_WSET(pdh));
  tsdbUpdateDFileSet(REPO_FS(pRepo), TSDB_DELETE_WSET(pdh));
  tsdbDebug("vgId:%d FSET %d delete data over", REPO_ID(pRepo), pSet->fid);

  tsdbDeleteFSetEnd(pdh);
  return 0;
}

static int tsdbInitDeleteH(SDeleteH *pdh, STsdbRepo *pRepo) {
  STsdbCfg *pCfg = REPO_CFG(pRepo);

  memset(pdh, 0, sizeof(*pdh));

  TSDB_FSET_SET_CLOSED(TSDB_DELETE_WSET(pdh));
  pdh->pRepo = pRepo;

  tsdbGetRtnSnap(pRepo, &(pdh->rtn));
  tsdbFSIterInit(&(pdh->fsIter), REPO_FS(pRepo), TSDB_FS_ITER_FORWARD);

  if (tsdbInitReadH(&(pdh->readh), pRepo) < 0) {
    return -1;
  }

  if (tsdbInitDeleteTblArray(pdh) < 0) {
    tsdbDestroyDeleteH(pdh);
    return -1;
  }

  pdh->aBlkIdx = taosArrayInit(1024, sizeof(SBlockIdx));
  if (pdh->aBlkIdx == NULL) {
    terrno = TSDB_CODE_TDB_OUT_OF_MEMORY;
    tsdbDestroyDeleteH(pdh);
    return -1;
  }

  pdh->aSupBlk = taosArrayInit(1024, sizeof(SBlock));
  if (pdh->aSupBlk == NULL) {
    terrno = TSDB_CODE_TDB_OUT_OF_MEMORY;
    tsdbDestroyDeleteH(pdh);
    return -1;
  }

  pdh->aSubBlk = taosArrayInit(20, sizeof(SBlock));
  if (pdh->aSubBlk == NULL) {
    terrno = TSDB_CODE_TDB_OUT_OF_MEMORY;
    tsdbDestroyDeleteH(pdh);
    return -1;
  }

  pdh->pDCols = tdNewDataCols(0, pCfg->maxRowsPerFileBlock);
  if (pdh->pDCols == NULL) {
    terrno = TSDB_CODE_TDB_OUT_OF_MEMORY;
    tsdbDestroyDeleteH(pdh);
    return -1;
  }

  return 0;
}

static void tsdbDestroyDeleteH(SDeleteH *pdh) {
  pdh->pDCols = tdFreeDataCols(pdh->pDCols);
  pdh->aSupBlk = taosArrayDestroy(&pdh->aSupBlk);
  pdh->aSubBlk = taosArrayDestroy(&pdh->aSubBlk);
  pdh->aBlkIdx = taosArrayDestroy(&pdh->aBlkIdx);
  tsdbDestroyDeleteTblArray(pdh);
  tsdbDestroyReadH(&(pdh->readh));
  tsdbCloseDFileSet(TSDB_DELETE_WSET(pdh));
}

void tsdbAddUpdates(SArray* pArray, STable* pTable) {
  size_t cnt = taosArrayGetSize(pArray);
  for ( size_t i = 0; i < cnt; i++) {
   STable* pt = taosArrayGetP(pArray, i);
   if ( pt == pTable) {
     // found
     return ;
   }
  }
  // ref count ++
  tsdbRefTable(pTable);
  // append
  taosArrayAddBatch(pArray, &pTable, 1);
}

void tsdbAddAffectTables(SArray* pArray, int32_t tid) {
  size_t cnt = taosArrayGetSize(pArray);
  for ( size_t i = 0; i < cnt; i++) {
   int32_t tid1 = *(int32_t *)taosArrayGet(pArray, i);
   if ( tid1 == tid) {
     // exist return
     return ;
   }
  }
  // append
  taosArrayAddBatch(pArray, &tid, 1);
}
// init tbl array with pRepo->meta
static int tsdbInitDeleteTblArray(SDeleteH *pdh) {
  STsdbRepo *pRepo = TSDB_DELETE_REPO(pdh);
  STsdbMeta *pMeta = pRepo->tsdbMeta;

  if (tsdbRLockRepoMeta(pRepo) < 0) return -1;

  pdh->tblArray = taosArrayInit(pMeta->maxTables, sizeof(STableDeleteH));
  if (pdh->tblArray == NULL) {
    terrno = TSDB_CODE_TDB_OUT_OF_MEMORY;
    tsdbUnlockRepoMeta(pRepo);
    return -1;
  }

  // Note here must start from 0
  for (int i = 0; i < pMeta->maxTables; ++i) {
    STableDeleteH tbl = {0};
    if (pMeta->tables[i] != NULL) {
      tsdbRefTable(pMeta->tables[i]);
      tbl.pTable = pMeta->tables[i];
    }

    if (taosArrayPush(pdh->tblArray, &tbl) == NULL) {
      terrno = TSDB_CODE_TDB_OUT_OF_MEMORY;
      tsdbUnlockRepoMeta(pRepo);
      return -1;
    }
  }

  if (tsdbUnlockRepoMeta(pRepo) < 0) return -1;
  return 0;
}

static void tsdbDestroyDeleteTblArray(SDeleteH *pdh) {
  STableDeleteH *pItem = NULL;

  if (pdh->tblArray == NULL) return;

  for (size_t i = 0; i < taosArrayGetSize(pdh->tblArray); ++i) {
    pItem = (STableDeleteH *)taosArrayGet(pdh->tblArray, i);
    if (pItem->pTable) {
      tsdbUnRefTable(pItem->pTable);
    }

    tfree(pItem->pInfo);
  }

  pdh->tblArray = taosArrayDestroy(&pdh->tblArray);
}

static int tsdbCacheFSetIndex(SDeleteH *pdh) {
  SReadH *pReadH = &(pdh->readh);

  if (tsdbLoadBlockIdx(pReadH) < 0) {
    return -1;
  }

  size_t cnt = taosArrayGetSize(pdh->tblArray);
  for (size_t tid = 1; tid < cnt; ++tid) {
    STableDeleteH *pItem = (STableDeleteH *)taosArrayGet(pdh->tblArray, tid);
    pItem->pBlkIdx = NULL;

    if (pItem->pTable == NULL) 
      continue;
    if (tsdbSetReadTable(pReadH, pItem->pTable) < 0)
      return -1;
    if (pReadH->pBlkIdx == NULL) 
      continue;
    pItem->bIndex = *(pReadH->pBlkIdx);
    pItem->pBlkIdx = &(pItem->bIndex);

    uint32_t originLen = 0;
    if (tsdbLoadBlockInfo(pReadH, (void **)(&(pItem->pInfo)), &originLen) < 0) {
      return -1;
    }
  }

  return 0;
}

static int tsdbFSetInit(SDeleteH *pdh, SDFileSet *pSet) {
  taosArrayClear(pdh->aBlkIdx);
  taosArrayClear(pdh->aSupBlk);

  // open
  if (tsdbSetAndOpenReadFSet(&(pdh->readh), pSet) < 0) {
    return -1;
  }

  // load index to cache
  if (tsdbCacheFSetIndex(pdh) < 0) {
    tsdbCloseAndUnsetFSet(&(pdh->readh));
    return -1;
  }

  return 0;
}

static void tsdbDeleteFSetEnd(SDeleteH *pdh) { 
  tsdbCloseAndUnsetFSet(&(pdh->readh)); 
}

static int32_t tsdbFilterDataCols(SDeleteH *pdh, SDataCols *pSrcDCols) {
  SDataCols * pDstDCols = pdh->pDCols;
  int32_t delRows = 0;

  tdResetDataCols(pDstDCols);
  pDstDCols->maxCols = pSrcDCols->maxCols;
  pDstDCols->maxPoints = pSrcDCols->maxPoints;
  pDstDCols->numOfCols = pSrcDCols->numOfCols;
  pDstDCols->sversion = pSrcDCols->sversion;

  for (int i = 0; i < pSrcDCols->numOfRows; ++i) {
    int64_t tsKey = *(int64_t *)tdGetColDataOfRow(pSrcDCols->cols, i);
    if ((tsKey >= pdh->pCtlInfo->win.skey) && (tsKey <= pdh->pCtlInfo->win.ekey)) {
      // delete row
      delRows ++;
      continue;
    }
    for (int j = 0; j < pSrcDCols->numOfCols; ++j) {
      if (pSrcDCols->cols[j].len > 0 || pDstDCols->cols[j].len > 0) {
        dataColAppendVal(pDstDCols->cols + j, tdGetColDataOfRow(pSrcDCols->cols + j, i), pDstDCols->numOfRows,
                         pDstDCols->maxPoints, 0);
      }
    }
    ++ pDstDCols->numOfRows;
  }

  return delRows;
}

// table in delete list
bool tableInDel(SDeleteH* pdh, int32_t tid) {
  for (int32_t i = 0; i < pdh->pCtlInfo->tnum; i++) {
    if (tid == pdh->pCtlInfo->tids[i])
      return true;
  }

  return false;
}

// if pBlock is border block return true else return false
static int tsdbBlockSolve(SDeleteH *pdh, SBlock *pBlock) {
  // delete window
  STimeWindow* pdel = &pdh->pCtlInfo->win;

  // do nothing for no delete
  if(pBlock->keyFirst > pdel->ekey || pBlock->keyLast < pdel->skey)
    return BLOCK_READ;

  // border block
  if(pBlock->keyFirst <= pdel->skey || pBlock->keyLast >= pdel->ekey)
    return BLOCK_MODIFY;

  // need del
  return BLOCK_DELETE;
}

// remove del block from pBlockInfo
int tsdbRemoveDelBlocks(SDeleteH *pdh, STableDeleteH * pItem) {
  // loop
  int numOfBlocks = pItem->pBlkIdx->numOfBlocks;
  int from = -1;
  int delRows = 0;
  
  for (int i = numOfBlocks - 1; i >= 0; --i) {
    SBlock *pBlock = pItem->pInfo->blocks + i;
    int32_t solve = tsdbBlockSolve(pdh, pBlock);
    if (solve == BLOCK_DELETE) {
      if (from == -1)
         from = i;
      delRows += pBlock->numOfRows;   
    } else {
      if(from != -1) {
        // do del
        int delCnt = from - i;
        memmove(pItem->pInfo->blocks + i + 1, pItem->pInfo->blocks + i + 1 + delCnt, (numOfBlocks - (i+1) - delCnt) * sizeof(SBlock));
        numOfBlocks -= delCnt;
        from = -1;
      }
    }
  }

  if(from != -1) {
    int delCnt = from + 1;
    memmove(pItem->pInfo->blocks, pItem->pInfo->blocks + delCnt, (numOfBlocks - delCnt) * sizeof(SBlock));
    numOfBlocks -= delCnt;
  }

  // set current blocks num
  pItem->pBlkIdx->numOfBlocks = numOfBlocks;

  if(delRows > 0) {
    // affected Rows
    if(pdh->pCtlInfo->pRsp) {
      pdh->pCtlInfo->pRsp->affectedRows += delRows;
    }
    // affected Tables
    tsdbAddAffectTables(pdh->aAffectTables, pItem->pTable->tableId.tid);
  }  

  return TSDB_CODE_SUCCESS;
}

static void tsdbAddBlock(SDeleteH *pdh, STableDeleteH *pItem, SBlock *pBlock) {
  // append sub if have
  if (pBlock->numOfSubBlocks > 1) {
    int64_t offset = taosArrayGetSize(pdh->aSubBlk) * sizeof(SBlock);
    SBlock *jBlock = POINTER_SHIFT(pItem->pInfo, pBlock->offset);;
    for (int j = 0; j < pBlock->numOfSubBlocks; j++) {
      taosArrayPush(pdh->aSubBlk, (const void *)jBlock++);
    }
    // set new offset if have sub
    pBlock->offset = offset;
  }

  // append super
  taosArrayPush(pdh->aSupBlk, (const void *)pBlock);
}

// need modify blocks
static int tsdbModifyBlocks(SDeleteH *pdh, STableDeleteH *pItem) {
  SReadH *   pReadh  = &(pdh->readh);
  void **    ppBuf   = &(TSDB_DELETE_BUF(pdh));
  void **    ppCBuf  = &(TSDB_DELETE_COMP_BUF(pdh));
  void **    ppExBuf = &(TSDB_DELETE_EXBUF(pdh));
  STSchema  *pSchema = NULL;
  SBlockIdx  blkIdx  = {0};

  // update last row if need
  TSKEY lastKey = pItem->pTable->lastKey;
  if(lastKey >= pdh->pCtlInfo->win.skey && lastKey <= pdh->pCtlInfo->win.ekey) {
    // update lastkey and lastrow
    tsdbAddUpdates(pdh->aUpdates, pItem->pTable);
  }

  // get pSchema for del table
  if ((pSchema = tsdbGetTableSchemaImpl(pItem->pTable, true, true, -1, -1)) == NULL) {
    return -1;
  }
  
  if ((tdInitDataCols(pdh->pDCols, pSchema) < 0) || (tdInitDataCols(pReadh->pDCols[0], pSchema) < 0) ||
      (tdInitDataCols(pReadh->pDCols[1], pSchema) < 0)) {
    terrno = TSDB_CODE_TDB_OUT_OF_MEMORY;
    tdFreeSchema(pSchema);
    return -1;
  }
  tdFreeSchema(pSchema);

  // delete block
  tsdbRemoveDelBlocks(pdh, pItem);
  if(pItem->pBlkIdx->numOfBlocks == 0) {
    // all blocks were deleted
    return TSDB_CODE_SUCCESS;
  }

  taosArrayClear(pdh->aSupBlk);
  taosArrayClear(pdh->aSubBlk);

  int32_t affectedRows = 0;

  // Loop to delete each block data
  for (int i = 0; i < pItem->pBlkIdx->numOfBlocks; ++i) {
    SBlock *pBlock = pItem->pInfo->blocks + i;
    int32_t solve = tsdbBlockSolve(pdh, pBlock);
    if (solve == BLOCK_READ) {
      tsdbAddBlock(pdh, pItem, pBlock);
      continue;
    }

    // border block need load to delete no-use data
    if (tsdbLoadBlockData(pReadh, pBlock, pItem->pInfo) < 0) {
      return -1;
    }

    affectedRows += tsdbFilterDataCols(pdh, pReadh->pDCols[0]);
    if (pdh->pDCols->numOfRows <= 0) {
      continue;
    }

    SBlock newBlock = {0};
    if (tsdbWriteBlockToFile(pdh, pItem->pTable, pdh->pDCols, ppBuf, ppCBuf, ppExBuf, &newBlock) < 0) {
      return -1;
    }

    // add new block to info
    tsdbAddBlock(pdh, pItem, &newBlock);
  }

  // write block info for each table
  if (tsdbWriteBlockInfoImpl(TSDB_DELETE_HEAD_FILE(pdh), pItem->pTable, pdh->aSupBlk, pdh->aSubBlk,
                              ppBuf, &blkIdx) < 0) {
    return -1;
  }

  // each table's blkIdx 
  if (blkIdx.numOfBlocks > 0 && taosArrayPush(pdh->aBlkIdx, (const void *)(&blkIdx)) == NULL) {
    terrno = TSDB_CODE_TDB_OUT_OF_MEMORY;
    return -1;
  }

  // update new last row in last row was deleted
  if (affectedRows > 0) {
    // affectedRows
    if (pdh->pCtlInfo->pRsp) {
      pdh->pCtlInfo->pRsp->affectedRows += affectedRows;
    }
    // affectTables
    tsdbAddAffectTables(pdh->aAffectTables, pItem->pTable->tableId.tid);
  }

  return TSDB_CODE_SUCCESS;
}

// keep intact blocks info and write to head file then save offset to blkIdx
static int tsdbKeepIntactBlocks(SDeleteH *pdh, STableDeleteH * pItem) {
  // init
  SBlockIdx  blkIdx  = {0};
  taosArrayClear(pdh->aSupBlk);
  taosArrayClear(pdh->aSubBlk);

  for (int32_t i = 0; i < pItem->pBlkIdx->numOfBlocks; i++) {
    SBlock *pBlock = pItem->pInfo->blocks + i;
    tsdbAddBlock(pdh, pItem, pBlock);
  }

  // write block info for one table
  void **ppBuf = &(TSDB_DELETE_BUF(pdh));
  int32_t ret  = tsdbWriteBlockInfoImpl(TSDB_DELETE_HEAD_FILE(pdh), pItem->pTable, pdh->aSupBlk, 
                                       pdh->aSubBlk, ppBuf, &blkIdx);
  if (ret != TSDB_CODE_SUCCESS) {
    return ret;
  }

  // each table's blkIdx 
  if (blkIdx.numOfBlocks > 0 && taosArrayPush(pdh->aBlkIdx, (const void *)&blkIdx) == NULL) {
    terrno = TSDB_CODE_TDB_OUT_OF_MEMORY;
    return -1;
  }

  return ret;
}

static int tsdbFSetDeleteImpl(SDeleteH *pdh) {
  void **   ppBuf  = &(TSDB_DELETE_BUF(pdh));
  int32_t   ret    = TSDB_CODE_SUCCESS;

  // 1.INIT
  taosArrayClear(pdh->aBlkIdx);

  for (size_t tid = 1; tid < taosArrayGetSize(pdh->tblArray); ++tid) {
    STableDeleteH *pItem = (STableDeleteH *)taosArrayGet(pdh->tblArray, tid);

    // no table in this tid position
    if (pItem->pTable == NULL || pItem->pBlkIdx == NULL)
      continue;

    // 2.WRITE INFO OF EACH TABLE BLOCK INFO TO HEAD FILE
    if (tableInDel(pdh, tid)) {
      // modify blocks info and write to head file then save offset to blkIdx
      ret = tsdbModifyBlocks(pdh, pItem);
    } else {
      // keep intact blocks info and write to head file then save offset to blkIdx
      ret = tsdbKeepIntactBlocks(pdh, pItem);
    }
    if (ret != TSDB_CODE_SUCCESS)
      return ret;
  } // tid for

  // 3.WRITE INDEX OF ALL TABLE'S BLOCK TO HEAD FILE
  if (tsdbWriteBlockIdx(TSDB_DELETE_HEAD_FILE(pdh), pdh->aBlkIdx, ppBuf) < 0) {
    return -1;
  }

  return ret;
}

static int tsdbWriteBlockToFile(SDeleteH *pdh, STable *pTable, SDataCols *pDCols, void **ppBuf,
                                     void **ppCBuf, void **ppExBuf, SBlock *pBlock) {
  STsdbRepo *pRepo = TSDB_DELETE_REPO(pdh);
  STsdbCfg * pCfg = REPO_CFG(pRepo);
  SDFile *   pDFile = NULL;
  bool       isLast = false;

  ASSERT(pDCols->numOfRows > 0);

  if (pDCols->numOfRows < pCfg->minRowsPerFileBlock) {
    pDFile = TSDB_DELETE_LAST_FILE(pdh);
    isLast = true;
  } else {
    pDFile = TSDB_DELETE_DATA_FILE(pdh);
    isLast = false;
  }

  if (tsdbWriteBlockImpl(pRepo, pTable, pDFile,
                         isLast ? TSDB_DELETE_SMAL_FILE(pdh) : TSDB_DELETE_SMAD_FILE(pdh), pDCols,
                         pBlock, isLast, true, ppBuf, ppCBuf, ppExBuf) < 0) {
    return -1;
  }

  return 0;
}
