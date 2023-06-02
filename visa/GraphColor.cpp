/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GraphColor.h"
#include "BuildIR.h"
#include "DebugInfo.h"
#include "FlowGraph.h"
#include "LinearScanRA.h"
#include "LocalRA.h"
#include "Optimizer.h"
#include "PointsToAnalysis.h"
#include "RPE.h"
#include "Rematerialization.h"
#include "SCCAnalysis.h"
#include "SpillCleanup.h"
#include "SpillCode.h"
#include "SplitAlignedScalars.h"
#include "Timer.h"

#include <algorithm>
#include <cmath> // sqrt
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>

#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Path.h>

using namespace vISA;

#define GRAPH_COLOR_MEM_SIZE 16 * 1024
#define SCRATCH_MSG_LIMIT (128 * 1024)

const RAVarInfo GlobalRA::defaultValues;
const char GlobalRA::StackCallStr[] = "StackCall";

static const unsigned IN_LOOP_REFERENCE_COUNT_FACTOR = 4;

#define BANK_CONFLICT_HEURISTIC_INST 0.04
#define BANK_CONFLICT_HEURISTIC_REF_COUNT 0.25
#define BANK_CONFLICT_HEURISTIC_LOOP_ITERATION 5
#define BANK_CONFLICT_SEND_INST_CYCLE                                          \
  60 // Some send 200, some 400 we choose the small one
#define BANK_CONFLICT_SIMD8_OVERHEAD_CYCLE 1
#define BANK_CONFLICT_SIMD16_OVERHEAD_CYCLE 2
#define INTERNAL_CONFLICT_RATIO_HEURISTIC 0.25

#define NOMASK_BYTE 0x80

Interference::Interference(const LivenessAnalysis *l,
                           const LiveRangeVec& lr,
                           unsigned n, unsigned ns, unsigned nm, GlobalRA &g)
    : gra(g), kernel(g.kernel), lrs(lr), builder(*g.kernel.fg.builder),
      maxId(n), splitStartId(ns), splitNum(nm), liveAnalysis(l),
      rowSize(maxId / BITS_DWORD + 1), aug(g.kernel, *this, *l, lr, g),
      incRA(g.incRA) {
  denseMatrixLimit = builder.getuint32Option(vISA_DenseMatrixLimit);
  incRA.registerNextIter((G4_RegFileKind)l->getSelectedRF(), l);
}

criticalCmpForEndInterval::criticalCmpForEndInterval(GlobalRA &g) : gra(g) {}
bool criticalCmpForEndInterval::operator()(G4_Declare *A, G4_Declare *B) const {
  return gra.getEndInterval(A)->getLexicalId() >
         gra.getEndInterval(B)->getLexicalId();
}
AugmentPriorityQueue::AugmentPriorityQueue(criticalCmpForEndInterval cmp)
    : std::priority_queue<G4_Declare *, std::vector<G4_Declare *>,
                          criticalCmpForEndInterval>(cmp) {}

inline bool Interference::varSplitCheckBeforeIntf(unsigned v1,
                                                  unsigned v2) const {
  const LiveRange *l1 = lrs[v1];
  const LiveRange *l2 = lrs[v2];

  if (!l1->getIsPartialDcl() && !l2->getIsPartialDcl()) {
    return false;
  }

  // Don't do interference for two split declares
  if (l1->getIsPartialDcl() && l2->getIsPartialDcl()) {
    return true;
  }

  unsigned p1 = v1;
  unsigned p2 = v2;
  // Don't do inteference for child and parent delcares
  if (l1->getIsPartialDcl()) {
    p1 = l1->getParentLRID();
  }

  if (l2->getIsPartialDcl()) {
    p2 = l2->getParentLRID();
  }

  if (p1 == p2) {
    return true;
  }

  return false;
}

BankConflict BankConflictPass::setupBankAccordingToSiblingOperand(
    BankConflict assignedBank, unsigned offset, bool oneGRFBank) {
  BankConflict tgtBank;

  vISA_ASSERT(assignedBank != BANK_CONFLICT_NONE,
              "sibling bank is not assigned");

  // Set according to sibling
  tgtBank = (assignedBank == BANK_CONFLICT_FIRST_HALF_EVEN ||
             assignedBank == BANK_CONFLICT_FIRST_HALF_ODD)
                ? (assignedBank == BANK_CONFLICT_FIRST_HALF_EVEN
                       ? BANK_CONFLICT_SECOND_HALF_ODD
                       : BANK_CONFLICT_SECOND_HALF_EVEN)
                : (assignedBank == BANK_CONFLICT_SECOND_HALF_EVEN
                       ? BANK_CONFLICT_FIRST_HALF_ODD
                       : BANK_CONFLICT_FIRST_HALF_EVEN);

  // Adjust according to the offset
  if (oneGRFBank) {
    if (offset % 2) {
      if (tgtBank == BANK_CONFLICT_SECOND_HALF_EVEN ||
          tgtBank == BANK_CONFLICT_FIRST_HALF_EVEN) {
        tgtBank = (tgtBank == BANK_CONFLICT_FIRST_HALF_EVEN)
                      ? BANK_CONFLICT_FIRST_HALF_ODD
                      : BANK_CONFLICT_SECOND_HALF_ODD;
      } else {
        tgtBank = (tgtBank == BANK_CONFLICT_FIRST_HALF_ODD)
                      ? BANK_CONFLICT_FIRST_HALF_EVEN
                      : BANK_CONFLICT_SECOND_HALF_EVEN;
      }
    }
  } else {
    if (offset % 4 >= 2) {
      if (tgtBank == BANK_CONFLICT_SECOND_HALF_EVEN ||
          tgtBank == BANK_CONFLICT_FIRST_HALF_EVEN) {
        tgtBank = (tgtBank == BANK_CONFLICT_FIRST_HALF_EVEN)
                      ? BANK_CONFLICT_FIRST_HALF_ODD
                      : BANK_CONFLICT_SECOND_HALF_ODD;
      } else {
        tgtBank = (tgtBank == BANK_CONFLICT_FIRST_HALF_ODD)
                      ? BANK_CONFLICT_FIRST_HALF_EVEN
                      : BANK_CONFLICT_SECOND_HALF_EVEN;
      }
    }
  }

  return tgtBank;
}

void refNumBasedSort(const unsigned *refNum, unsigned *index) {
  if (refNum[2] > refNum[1]) {
    index[0] = 2;
    index[1] = 1;
  } else {
    index[0] = 1;
    index[1] = 2;
  }

  index[2] = 0;

  return;
}

bool BankConflictPass::hasInternalConflict3Srcs(BankConflict *srcBC) {
  if (((srcBC[0] == BANK_CONFLICT_SECOND_HALF_EVEN ||
        srcBC[0] == BANK_CONFLICT_FIRST_HALF_EVEN) &&
       (srcBC[1] == BANK_CONFLICT_SECOND_HALF_EVEN ||
        srcBC[1] == BANK_CONFLICT_FIRST_HALF_EVEN) &&
       (srcBC[2] == BANK_CONFLICT_SECOND_HALF_EVEN ||
        srcBC[2] == BANK_CONFLICT_FIRST_HALF_EVEN)) ||
      ((srcBC[0] == BANK_CONFLICT_SECOND_HALF_ODD ||
        srcBC[0] == BANK_CONFLICT_FIRST_HALF_ODD) &&
       (srcBC[1] == BANK_CONFLICT_SECOND_HALF_ODD ||
        srcBC[1] == BANK_CONFLICT_FIRST_HALF_ODD) &&
       (srcBC[2] == BANK_CONFLICT_SECOND_HALF_ODD ||
        srcBC[2] == BANK_CONFLICT_FIRST_HALF_ODD))) {
    return true;
  }
  if ((srcBC[0] < BANK_CONFLICT_SECOND_HALF_EVEN &&
       srcBC[1] < BANK_CONFLICT_SECOND_HALF_EVEN &&
       srcBC[2] < BANK_CONFLICT_SECOND_HALF_EVEN) ||
      (srcBC[0] >= BANK_CONFLICT_SECOND_HALF_EVEN &&
       srcBC[1] >= BANK_CONFLICT_SECOND_HALF_EVEN &&
       srcBC[2] >= BANK_CONFLICT_SECOND_HALF_EVEN)) {
    return true;
  }

  return false;
}

void BankConflictPass::setupEvenOddBankConflictsForDecls(
    G4_Declare *dcl_1, G4_Declare *dcl_2, unsigned offset1, unsigned offset2,
    BankConflict &srcBC1, BankConflict &srcBC2) {
  vISA_ASSERT(srcBC1 == BANK_CONFLICT_NONE, "Wrong Bank initial value");
  vISA_ASSERT(srcBC2 == BANK_CONFLICT_NONE, "Wrong Bank initial value");

  unsigned refNum1 = gra.getNumRefs(dcl_1);
  unsigned refNum2 = gra.getNumRefs(dcl_2);

  BankConflict bank1 = BANK_CONFLICT_NONE;
  BankConflict bank2 = BANK_CONFLICT_NONE;

  bank1 = (refNum1 >= refNum2) ? BANK_CONFLICT_FIRST_HALF_EVEN
                               : BANK_CONFLICT_SECOND_HALF_ODD;
  bank2 = (bank1 == BANK_CONFLICT_FIRST_HALF_EVEN)
              ? BANK_CONFLICT_SECOND_HALF_ODD
              : BANK_CONFLICT_FIRST_HALF_EVEN;

  srcBC1 = bank1;
  srcBC2 = bank2;

  // Adjust only for the single bank allocation
  if ((offset1 + offset2) % 2) {
    if (refNum1 >= refNum2) {
      bank2 = (bank2 == BANK_CONFLICT_FIRST_HALF_EVEN)
                  ? BANK_CONFLICT_SECOND_HALF_ODD
                  : BANK_CONFLICT_FIRST_HALF_EVEN;
    } else {
      bank1 = (bank1 == BANK_CONFLICT_FIRST_HALF_EVEN)
                  ? BANK_CONFLICT_SECOND_HALF_ODD
                  : BANK_CONFLICT_FIRST_HALF_EVEN;
    }
  }

  gra.setBankConflict(dcl_1, bank1);
  gra.setBankConflict(dcl_2, bank2);

  return;
}

//
// inst opcode is G4_mad. This function sets up a simple state machine to
// prevent conflict between src 1 and 2 of mad inst. Following is how GRF file
// is divided in to banks: bank-block A = 0, 2, 4, 6, ..., 62 bank-block B = 1,
// 3, 5, 7, ..., 63 bank-block C = 64, 66, 68, ..., 126 bank-block D = 65, 67,
// 69, ..., 127
//
// For ternary ops, if src1 and src2 are to the same bank then there will be an
// access collision. But unary and binary ops will have no collision, no matter
// what registers they use. The reason is second and third src operands are read
// in the same clock cycle, which is different than when src0 operand is read.
// This is true upto pre-SKL.
//
// Bank Conflict Herustics:
// 1. Try to balance the used registers in two banks for the potential
// conflicted registers.
// 2. reference number is used to decide which to be assigned first
// 3. When conflict detected, bank can be updated according to the reference
// count.
//
void BankConflictPass::setupBankConflictsOneGRFOld(G4_INST *inst,
                                                   int &bank1RegNum,
                                                   int &bank2RegNum,
                                                   float GRFRatio,
                                                   unsigned &internalConflict) {
  BankConflict srcBC[3];
  unsigned regNum[3];
  unsigned refNum[3];
  unsigned offset[3];
  G4_Declare *dcls[3];
  G4_Declare *opndDcls[3];
  int bank_num = 0;

  for (int i = 0; i < 3; i++) {
    dcls[i] = nullptr;
    opndDcls[i] = nullptr;

    G4_Operand *src = inst->getSrc(i);
    if (!src || !src->isSrcRegRegion() || src->isAccReg()) {
      // bank conflict not possible
      return;
    }

    dcls[i] = GetTopDclFromRegRegion(src);
    opndDcls[i] = src->getBase()->asRegVar()->getDeclare();

    regNum[i] = dcls[i]->getNumRows();
    refNum[i] = gra.getNumRefs(dcls[i]);
    offset[i] = (opndDcls[i]->getOffsetFromBase() + src->getLeftBound()) /
                gra.kernel.numEltPerGRF<Type_UB>();
    srcBC[i] = gra.getBankConflict(dcls[i]);

    if (src->getBase()->asRegVar()->isPhyRegAssigned()) {
      unsigned reg =
          src->getBase()->asRegVar()->getPhyReg()->asGreg()->getRegNum();
      if ((reg + offset[i]) < SECOND_HALF_BANK_START_GRF) {
        srcBC[i] = ((reg + offset[i]) % 2) ? BANK_CONFLICT_FIRST_HALF_ODD
                                           : BANK_CONFLICT_FIRST_HALF_EVEN;
      } else {
        srcBC[i] = ((reg + offset[i]) % 2) ? BANK_CONFLICT_SECOND_HALF_ODD
                                           : BANK_CONFLICT_SECOND_HALF_EVEN;
      }
      if (reg < SECOND_HALF_BANK_START_GRF) {
        bank1RegNum += regNum[i];
      } else {
        bank2RegNum += regNum[i];
      }
      gra.setBankConflict(dcls[i], srcBC[i]);
    } else if (srcBC[i] != BANK_CONFLICT_NONE) {
      if (offset[i] % 2) {
        // Get operand's bank from declare's bank
        if (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN ||
            srcBC[i] == BANK_CONFLICT_FIRST_HALF_ODD) {
          srcBC[i] = (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN)
                         ? BANK_CONFLICT_FIRST_HALF_ODD
                         : BANK_CONFLICT_FIRST_HALF_EVEN;
        } else {
          srcBC[i] = (srcBC[i] == BANK_CONFLICT_SECOND_HALF_EVEN)
                         ? BANK_CONFLICT_SECOND_HALF_ODD
                         : BANK_CONFLICT_SECOND_HALF_EVEN;
        }
      }
    }

    if (i > 0) {
      bank_num += srcBC[i];
    }
  }

  // In case src1 and src2 share same declare, i.e. use same register
  if (bank_num == 0 && dcls[1] == dcls[2]) {
    BankConflict bank1 = ((bank1RegNum * GRFRatio) > bank2RegNum)
                             ? BANK_CONFLICT_SECOND_HALF_EVEN
                             : BANK_CONFLICT_FIRST_HALF_EVEN;

    gra.setBankConflict(dcls[1], bank1);
    srcBC[1] = bank1;
    srcBC[2] = bank1;
    bank_num += bank1 * 2;
    if (bank1 < BANK_CONFLICT_SECOND_HALF_EVEN) {
      bank1RegNum += regNum[1];
    } else {
      bank2RegNum += regNum[1];
    }
  }

  // No bank assigned to src 1, 2.
  // assign the two delcares into different bundles/banks.
  if (bank_num == 0) {
    BankConflict bank1 = BANK_CONFLICT_NONE;
    BankConflict bank2 = BANK_CONFLICT_NONE;
    bool bank1First = false;
    if (GRFRatio == 1.0) {
      // For global RA: Try to reduce the size of bank 2
      if ((float)refNum[1] / regNum[1] >= (float)refNum[2] / regNum[2]) {
        bank1 = BANK_CONFLICT_SECOND_HALF_EVEN;
        bank2 = BANK_CONFLICT_FIRST_HALF_ODD;
        bank1First = true;
      } else {
        bank2 = BANK_CONFLICT_SECOND_HALF_EVEN;
        bank1 = BANK_CONFLICT_FIRST_HALF_ODD;
      }
    } else {
      // For local RA: Try to balance two banks
      if (refNum[1] >= refNum[2]) {
        bank1 = ((bank1RegNum * GRFRatio) > bank2RegNum)
                    ? BANK_CONFLICT_SECOND_HALF_EVEN
                    : BANK_CONFLICT_FIRST_HALF_EVEN;
        bank2 = (bank1 == BANK_CONFLICT_SECOND_HALF_EVEN)
                    ? BANK_CONFLICT_FIRST_HALF_ODD
                    : BANK_CONFLICT_SECOND_HALF_ODD;
        bank1First = true;
      } else {
        bank2 = (bank1RegNum * GRFRatio) > bank2RegNum
                    ? BANK_CONFLICT_SECOND_HALF_EVEN
                    : BANK_CONFLICT_FIRST_HALF_EVEN;
        bank1 = (bank2 == BANK_CONFLICT_SECOND_HALF_EVEN)
                    ? BANK_CONFLICT_FIRST_HALF_ODD
                    : BANK_CONFLICT_SECOND_HALF_ODD;
      }
    }

    // Adjust only for the single bank allocation
    if ((offset[1] + offset[2]) % 2) {
      if (bank1First) {
        bank2 = (bank2 == BANK_CONFLICT_FIRST_HALF_ODD)
                    ? BANK_CONFLICT_FIRST_HALF_EVEN
                    : BANK_CONFLICT_SECOND_HALF_EVEN;
      } else {
        bank1 = (bank1 == BANK_CONFLICT_SECOND_HALF_ODD)
                    ? BANK_CONFLICT_SECOND_HALF_EVEN
                    : BANK_CONFLICT_FIRST_HALF_EVEN;
      }
    }

    if (bank1 >= BANK_CONFLICT_SECOND_HALF_EVEN) {
      bank2RegNum += regNum[1];
      bank1RegNum += regNum[2];
    } else {
      bank1RegNum += regNum[1];
      bank2RegNum += regNum[2];
    }

    gra.setBankConflict(dcls[1], bank1);
    gra.setBankConflict(dcls[2], bank2);
  } else {
    if (srcBC[1] == BANK_CONFLICT_NONE || srcBC[2] == BANK_CONFLICT_NONE) {
      // One source operand is assigned bank already
      if (srcBC[2] == BANK_CONFLICT_NONE) {
        srcBC[2] =
            setupBankAccordingToSiblingOperand(srcBC[1], offset[2], true);
        gra.setBankConflict(dcls[2], srcBC[2]);

        if (srcBC[2] < BANK_CONFLICT_SECOND_HALF_EVEN)
          bank1RegNum += regNum[2];
        else
          bank2RegNum += regNum[2];
      } else {
        srcBC[1] =
            setupBankAccordingToSiblingOperand(srcBC[2], offset[1], true);
        gra.setBankConflict(dcls[1], srcBC[1]);
        if (srcBC[1] < BANK_CONFLICT_SECOND_HALF_EVEN)
          bank1RegNum += regNum[1];
        else
          bank2RegNum += regNum[1];
      }
    } else if (dcls[1] != dcls[2]) {
      if (((srcBC[1] == BANK_CONFLICT_SECOND_HALF_EVEN ||
            srcBC[1] == BANK_CONFLICT_FIRST_HALF_EVEN) &&
           (srcBC[2] == BANK_CONFLICT_SECOND_HALF_EVEN ||
            srcBC[2] == BANK_CONFLICT_FIRST_HALF_EVEN)) ||
          ((srcBC[1] == BANK_CONFLICT_SECOND_HALF_ODD ||
            srcBC[1] == BANK_CONFLICT_FIRST_HALF_ODD) &&
           (srcBC[2] == BANK_CONFLICT_SECOND_HALF_ODD ||
            srcBC[2] == BANK_CONFLICT_FIRST_HALF_ODD))) {
        internalConflict++;
      }
      if ((srcBC[1] < BANK_CONFLICT_SECOND_HALF_EVEN &&
           srcBC[2] < BANK_CONFLICT_SECOND_HALF_EVEN) ||
          (srcBC[1] >= BANK_CONFLICT_SECOND_HALF_EVEN &&
           srcBC[2] >= BANK_CONFLICT_SECOND_HALF_EVEN)) {
        internalConflict++;
      }
    }
  }
}

void BankConflictPass::getBanks(G4_INST *inst, BankConflict *srcBC,
                                G4_Declare **dcls, G4_Declare **opndDcls,
                                unsigned *offset) {
  for (int i = 0; i < 3; i++) {
    dcls[i] = nullptr;
    opndDcls[i] = nullptr;
    srcBC[i] = BANK_CONFLICT_NONE;

    G4_Operand *src = inst->getSrc(i);
    if (!src || !src->isSrcRegRegion() || src->isAccReg()) {
      return;
    }

    dcls[i] = GetTopDclFromRegRegion(src);
    if (!dcls[i]) {
      continue;
    }
    opndDcls[i] = src->getBase()->asRegVar()->getDeclare();

    offset[i] = (opndDcls[i]->getOffsetFromBase() + src->getLeftBound()) /
                gra.kernel.numEltPerGRF<Type_UB>();
    srcBC[i] = gra.getBankConflict(dcls[i]);

    if (src->getBase()->asRegVar()->isPhyRegAssigned()) {
      unsigned reg =
          src->getBase()->asRegVar()->getPhyReg()->asGreg()->getRegNum();
      srcBC[i] = ((reg + offset[i]) % 2) ? BANK_CONFLICT_SECOND_HALF_ODD
                                         : BANK_CONFLICT_FIRST_HALF_EVEN;
    } else if (srcBC[i] != BANK_CONFLICT_NONE) {
      if (offset[i] % 2) {
        if (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN) {
          srcBC[i] = BANK_CONFLICT_SECOND_HALF_ODD;
        } else {
          srcBC[i] = BANK_CONFLICT_FIRST_HALF_EVEN;
        }
      }
    }
  }

  return;
}

void BankConflictPass::getPrevBanks(G4_INST *inst, BankConflict *srcBC,
                                    G4_Declare **dcls, G4_Declare **opndDcls,
                                    unsigned *offset) {
  // We only care about ALU instructions which have a max number of 3 sources.
  int execSize[3];

  for (int i = 1; i < 3; i++) {
    dcls[i] = nullptr;
    opndDcls[i] = nullptr;
    srcBC[i] = BANK_CONFLICT_NONE;

    G4_Operand *src = inst->getSrc(i);
    if (!src || !src->isSrcRegRegion()) {
      return;
    }
    dcls[i] = GetTopDclFromRegRegion(src);
    if (dcls[i]->getRegFile() != G4_GRF) {
      return;
    }
    execSize[i] = src->getLinearizedEnd() - src->getLinearizedStart() + 1;

    opndDcls[i] = src->getBase()->asRegVar()->getDeclare();

    offset[i] = (opndDcls[i]->getOffsetFromBase() + src->getLeftBound()) /
                gra.kernel.numEltPerGRF<Type_UB>();
    srcBC[i] = gra.getBankConflict(dcls[i]);

    if (src->getBase()->asRegVar()->isPhyRegAssigned()) {
      unsigned reg =
          src->getBase()->asRegVar()->getPhyReg()->asGreg()->getRegNum();
      srcBC[i] = ((reg + offset[i]) % 2) ? BANK_CONFLICT_SECOND_HALF_ODD
                                         : BANK_CONFLICT_FIRST_HALF_EVEN;
    } else if (srcBC[i] != BANK_CONFLICT_NONE) {
      if (offset[i] % 2) {
        if (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN) {
          srcBC[i] = BANK_CONFLICT_SECOND_HALF_ODD;
        } else {
          srcBC[i] = BANK_CONFLICT_FIRST_HALF_EVEN;
        }
      }
    }
    if (execSize[i] > 32) {
      srcBC[i] = (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN)
                     ? BANK_CONFLICT_SECOND_HALF_ODD
                     : BANK_CONFLICT_FIRST_HALF_EVEN;
    }
  }

  return;
}

void BankConflictPass::setupBankForSrc0(G4_INST *inst, G4_INST *prevInst) {
  BankConflict srcBC[3];
  G4_Declare *dcls[3];
  G4_Declare *opndDcls[3];
  unsigned offset[3];

  BankConflict prevSrcBC[3];
  G4_Declare *prevDcls[3];
  G4_Declare *prevOpndDcls[3];
  unsigned prevOffset[3];

  if (prevInst->isSend() || prevInst->isMath()) {
    return;
  }

  getBanks(inst, srcBC, dcls, opndDcls, offset);
  getPrevBanks(prevInst, prevSrcBC, prevDcls, prevOpndDcls, prevOffset);

  if (dcls[0] != nullptr && srcBC[0] == BANK_CONFLICT_NONE &&
      prevSrcBC[1] != BANK_CONFLICT_NONE &&
      prevSrcBC[2] != BANK_CONFLICT_NONE) {
    if (prevSrcBC[1] == prevSrcBC[2]) {
      if (prevSrcBC[1] == BANK_CONFLICT_FIRST_HALF_EVEN) {
        srcBC[0] = offset[0] % 2 ? BANK_CONFLICT_FIRST_HALF_EVEN
                                 : BANK_CONFLICT_SECOND_HALF_ODD;
      } else {
        srcBC[0] = offset[0] % 2 ? BANK_CONFLICT_SECOND_HALF_ODD
                                 : BANK_CONFLICT_FIRST_HALF_EVEN;
      }

      gra.setBankConflict(dcls[0], srcBC[0]);
    }
  }

  return;
}

void BankConflictPass::setupBankConflictsforTwoGRFs(G4_INST *inst) {
  BankConflict srcBC[3];
  unsigned refNum[3];
  unsigned offset[3];
  G4_Declare *dcls[3];
  G4_Declare *opndDcls[3];
  int bank_num = 0;
  int execSize[3];

  for (int i = 0; i < 3; i++) {
    dcls[i] = nullptr;
    opndDcls[i] = nullptr;
    execSize[i] = 0;

    G4_Operand *src = inst->getSrc(i);
    if (!src || !src->isSrcRegRegion() || src->isAccReg()) {
      // bank conflict not possible
      return;
    }
    execSize[i] = src->getLinearizedEnd() - src->getLinearizedStart() + 1;

    dcls[i] = GetTopDclFromRegRegion(src);
    opndDcls[i] = src->getBase()->asRegVar()->getDeclare();

    refNum[i] = gra.getNumRefs(dcls[i]);
    offset[i] = (opndDcls[i]->getOffsetFromBase() + src->getLeftBound()) /
                gra.kernel.numEltPerGRF<Type_UB>();
    srcBC[i] = gra.getBankConflict(dcls[i]);

    if (src->getBase()->asRegVar()->isPhyRegAssigned()) {
      unsigned reg =
          src->getBase()->asRegVar()->getPhyReg()->asGreg()->getRegNum();
      srcBC[i] = ((reg + offset[i]) % 2) ? BANK_CONFLICT_SECOND_HALF_ODD
                                         : BANK_CONFLICT_FIRST_HALF_EVEN;
      gra.setBankConflict(dcls[i], srcBC[i]);
    } else if (srcBC[i] != BANK_CONFLICT_NONE) {
      if (offset[i] % 2) {
        if (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN) {
          srcBC[i] = BANK_CONFLICT_SECOND_HALF_ODD;
        } else {
          srcBC[i] = BANK_CONFLICT_FIRST_HALF_EVEN;
        }
      }
    }
    if (i != 0) {
      bank_num += srcBC[i];
    }
  }

  int simd8SrcNum = 0;
  for (int i = 0; i < 3; i++) {
    if (execSize[i] <= 32) {
      simd8SrcNum++;
    }
  }

  // In case (src0) src1 and src2 use same declare, i.e. use same register
  if ((dcls[0] == dcls[1]) && (dcls[1] == dcls[2])) {
    return;
  }

  // No bank assigned to src operands,
  // assign the two delcares into different bundles/banks.
  if (simd8SrcNum <= 1) // All simd16, do even align
  {
    for (int i = 0; i < 3; i++) {
      if (execSize[i] > 32) {
        srcBC[i] = offset[i] % 2 ? BANK_CONFLICT_SECOND_HALF_ODD
                                 : BANK_CONFLICT_FIRST_HALF_EVEN;
        gra.setBankConflict(dcls[i], srcBC[i]);
      }
    }
  } else if (bank_num == 0) {
    unsigned index[3];

    refNumBasedSort(refNum, index);

    if (dcls[index[0]] != dcls[index[1]]) {
      setupEvenOddBankConflictsForDecls(dcls[index[0]], dcls[index[1]],
                                        offset[index[0]], offset[index[1]],
                                        srcBC[index[0]], srcBC[index[1]]);
    }
  } else {
    if (srcBC[1] != BANK_CONFLICT_NONE) {
      srcBC[2] = (srcBC[1] == BANK_CONFLICT_FIRST_HALF_EVEN)
                     ? BANK_CONFLICT_SECOND_HALF_ODD
                     : BANK_CONFLICT_FIRST_HALF_EVEN;
      if (offset[2] % 2) {
        srcBC[2] = (srcBC[2] == BANK_CONFLICT_FIRST_HALF_EVEN)
                       ? BANK_CONFLICT_SECOND_HALF_ODD
                       : BANK_CONFLICT_FIRST_HALF_EVEN;
      }
      gra.setBankConflict(dcls[2], srcBC[2]);
    } else {
      srcBC[1] = (srcBC[2] == BANK_CONFLICT_FIRST_HALF_EVEN)
                     ? BANK_CONFLICT_SECOND_HALF_ODD
                     : BANK_CONFLICT_FIRST_HALF_EVEN;
      if (offset[1] % 2) {
        srcBC[1] = (srcBC[1] == BANK_CONFLICT_FIRST_HALF_EVEN)
                       ? BANK_CONFLICT_SECOND_HALF_ODD
                       : BANK_CONFLICT_FIRST_HALF_EVEN;
      }
      gra.setBankConflict(dcls[1], srcBC[1]);
    }
  }
}

bool BankConflictPass::isOddOffset(unsigned offset) const {
  if (gra.kernel.fg.builder->oneGRFBankDivision()) {
    return (offset % 2);
  } else {
    return ((offset % 4) / 2);
  }
}

void BankConflictPass::setupBankConflictsforDPAS(G4_INST *inst) {
  BankConflict srcBC[3];
  unsigned refNum[3];
  unsigned offset[3];
  G4_Declare *dcls[3];
  G4_Declare *opndDcls[3];
  int bank_num = 0;

  if (!inst->isDpas()) {
    return;
  }

  for (int i = 0; i < 3; i += 1) {
    opndDcls[i] = nullptr;

    G4_Operand *src = inst->getSrc(i);

    dcls[i] = GetTopDclFromRegRegion(src);
    if (dcls[i]) {
      opndDcls[i] = src->getBase()->asRegVar()->getDeclare();

      refNum[i] = gra.getNumRefs(dcls[i]);
      offset[i] = (opndDcls[i]->getOffsetFromBase() + src->getLeftBound()) /
                  gra.kernel.numEltPerGRF<Type_UB>();
      srcBC[i] = gra.getBankConflict(dcls[i]);

      if (srcBC[i] != BANK_CONFLICT_NONE) {
        if (isOddOffset(offset[i])) {
          if (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN) {
            srcBC[i] = BANK_CONFLICT_SECOND_HALF_ODD;
          } else {
            srcBC[i] = BANK_CONFLICT_FIRST_HALF_EVEN;
          }
        }
        if (i != 1) {
          bank_num++;
        }
      }
    }
  }
  if (dcls[0] && dcls[1]) {
    gra.addBundleConflictDcl(dcls[0], dcls[1], offset[0] - offset[1]);
    gra.addBundleConflictDcl(dcls[1], dcls[0], offset[1] - offset[0]);
  }
  if (dcls[1] && dcls[2]) {
    gra.addBundleConflictDcl(dcls[2], dcls[1], offset[2] - offset[1]);
    gra.addBundleConflictDcl(dcls[1], dcls[2], offset[1] - offset[2]);
  }
#if 0
    if (gra.kernel.getOption(vISA_forceBCR) && dcls[0] && dcls[2])
    {
        gra.addBundleConflictDcl(dcls[2], dcls[0], offset[2] - offset[0]);
        gra.addBundleConflictDcl(dcls[0], dcls[2], offset[0] - offset[2]);
    }
#endif

  // In case (src0) src1 and src2 use same declare, i.e. use same register
  if (dcls[0] == dcls[2] || !dcls[0] || !dcls[2]) {
    return;
  }

  if (bank_num == 0) {
    srcBC[0] = refNum[0] > refNum[2] ? BANK_CONFLICT_FIRST_HALF_EVEN
                                     : BANK_CONFLICT_SECOND_HALF_ODD;
    srcBC[2] = refNum[0] > refNum[2] ? BANK_CONFLICT_SECOND_HALF_ODD
                                     : BANK_CONFLICT_FIRST_HALF_EVEN;
    if (isOddOffset(offset[0])) {
      srcBC[0] = (srcBC[0] == BANK_CONFLICT_FIRST_HALF_EVEN)
                     ? BANK_CONFLICT_SECOND_HALF_ODD
                     : BANK_CONFLICT_FIRST_HALF_EVEN;
    }
    if (isOddOffset(offset[2])) {
      srcBC[2] = (srcBC[2] == BANK_CONFLICT_FIRST_HALF_EVEN)
                     ? BANK_CONFLICT_SECOND_HALF_ODD
                     : BANK_CONFLICT_FIRST_HALF_EVEN;
    }
    gra.setBankConflict(dcls[0], srcBC[0]);
    gra.setBankConflict(dcls[2], srcBC[2]);

  } else if (bank_num == 1) {
    if (srcBC[0] != BANK_CONFLICT_NONE) {
      srcBC[2] = (srcBC[0] == BANK_CONFLICT_FIRST_HALF_EVEN)
                     ? BANK_CONFLICT_SECOND_HALF_ODD
                     : BANK_CONFLICT_FIRST_HALF_EVEN;
      if (isOddOffset(offset[2])) {
        srcBC[2] = (srcBC[2] == BANK_CONFLICT_FIRST_HALF_EVEN)
                       ? BANK_CONFLICT_SECOND_HALF_ODD
                       : BANK_CONFLICT_FIRST_HALF_EVEN;
      }
      gra.setBankConflict(dcls[2], srcBC[2]);
    } else {
      srcBC[0] = (srcBC[2] == BANK_CONFLICT_FIRST_HALF_EVEN)
                     ? BANK_CONFLICT_SECOND_HALF_ODD
                     : BANK_CONFLICT_FIRST_HALF_EVEN;
      if (offset[0] % 2) {
        srcBC[0] = (srcBC[0] == BANK_CONFLICT_FIRST_HALF_EVEN)
                       ? BANK_CONFLICT_SECOND_HALF_ODD
                       : BANK_CONFLICT_FIRST_HALF_EVEN;
      }
      gra.setBankConflict(dcls[0], srcBC[0]);
    }
  }

#ifdef DEBUG_VERBOSE_ON
  for (int i = 0; i < 3; i += 2) {
    if (opndDcls[i]) {
      printf("%s, ", opndDcls[i]->getName());

      if (gra.getBankConflict(dcls[i]) == BANK_CONFLICT_FIRST_HALF_EVEN) {
        printf("%s\n", "EVEN");
      } else if (gra.getBankConflict(dcls[i]) ==
                 BANK_CONFLICT_SECOND_HALF_ODD) {
        printf("%s\n", "ODD");
      } else {
        printf("%s\n", "NONE");
      }
    }
  }
#endif

  return;
}

void BankConflictPass::setupBankConflictsforMad(G4_INST *inst) {
  BankConflict srcBC[3];
  unsigned offset[3];
  G4_Declare *dcls[3];
  G4_Declare *opndDcls[3];
  BankConflict assignedBank = BANK_CONFLICT_NONE; // Flip for next

  for (int i = 0; i < 3; i += 1) {
    dcls[i] = nullptr;
    opndDcls[i] = nullptr;

    G4_Operand *src = inst->getSrc(i);
    if (!src || !src->isSrcRegRegion() || src->isAreg()) {
      // bank conflict not possible
      continue;
    }

    dcls[i] = GetTopDclFromRegRegion(src);
    opndDcls[i] = src->getBase()->asRegVar()->getDeclare();
    offset[i] = (opndDcls[i]->getOffsetFromBase() + src->getLeftBound()) /
                gra.kernel.numEltPerGRF<Type_UB>();
    srcBC[i] = gra.getBankConflict(dcls[i]);

    if (srcBC[i] != BANK_CONFLICT_NONE) {
      if (isOddOffset(offset[i])) {
        if (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN) {
          srcBC[i] = BANK_CONFLICT_SECOND_HALF_ODD;
        } else {
          srcBC[i] = BANK_CONFLICT_FIRST_HALF_EVEN;
        }
      }
      if (assignedBank != BANK_CONFLICT_SECOND_HALF_EVEN) {
        if (assignedBank == BANK_CONFLICT_NONE) {
          assignedBank = srcBC[i];
        } else if (assignedBank != srcBC[i]) {
          assignedBank =
              BANK_CONFLICT_SECOND_HALF_EVEN; // BANK_CONFLICT_SECOND_HALF_EVEN
                                              // is used to represent all banks
                                              // are assigned
        }
      }
    }
  }
  // Add potential bundle conflicts, so that RA can handle it when option
  // -enableBundleCR with value 2 or 3
  if (gra.kernel.getuInt32Option(vISA_enableBundleCR) & 2) {
    if (dcls[0] && dcls[1]) {
      gra.addBundleConflictDcl(dcls[0], dcls[1], offset[0] - offset[1]);
      gra.addBundleConflictDcl(dcls[1], dcls[0], offset[1] - offset[0]);
    }
    if (dcls[1] && dcls[2]) {
      gra.addBundleConflictDcl(dcls[2], dcls[1], offset[2] - offset[1]);
      gra.addBundleConflictDcl(dcls[1], dcls[2], offset[1] - offset[2]);
    }
  }

  for (int k = 0; k < 2; k++) {
    for (int i = 2; i != -1; i--) {
      if (!dcls[i]) {
        continue;
      }

      LocalLiveRange *lr = gra.getLocalLR(dcls[i]);
      if (!lr || (k == 0 && !lr->isLiveRangeLocal())) {
        continue;
      }

      if (k == 1 && lr->isLiveRangeLocal()) {
        continue;
      }

      if (assignedBank == BANK_CONFLICT_SECOND_HALF_EVEN) {
        continue;
      }

      srcBC[i] = gra.getBankConflict(dcls[i]);
      if (srcBC[i] != BANK_CONFLICT_NONE) {
        if (isOddOffset(offset[i])) {
          if (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN) {
            srcBC[i] = BANK_CONFLICT_SECOND_HALF_ODD;
          } else {
            srcBC[i] = BANK_CONFLICT_FIRST_HALF_EVEN;
          }
        }

        if (assignedBank == BANK_CONFLICT_NONE) {
          assignedBank = srcBC[i];
        } else if (srcBC[i] != assignedBank) {
          assignedBank = BANK_CONFLICT_SECOND_HALF_EVEN;
        }

        continue;
      }

      if (assignedBank == BANK_CONFLICT_NONE) {
        srcBC[i] = BANK_CONFLICT_FIRST_HALF_EVEN;
        assignedBank = srcBC[i];
        if (isOddOffset(offset[i])) {
          srcBC[i] = (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN)
                         ? BANK_CONFLICT_SECOND_HALF_ODD
                         : BANK_CONFLICT_FIRST_HALF_EVEN;
        }
        gra.setBankConflict(dcls[i], srcBC[i]);
      } else {
        srcBC[i] = (assignedBank == BANK_CONFLICT_FIRST_HALF_EVEN)
                       ? BANK_CONFLICT_SECOND_HALF_ODD
                       : BANK_CONFLICT_FIRST_HALF_EVEN;
        if (isOddOffset(offset[i])) {
          srcBC[i] = (srcBC[i] == BANK_CONFLICT_FIRST_HALF_EVEN)
                         ? BANK_CONFLICT_SECOND_HALF_ODD
                         : BANK_CONFLICT_FIRST_HALF_EVEN;
        }
        gra.setBankConflict(dcls[i], srcBC[i]);
        assignedBank = BANK_CONFLICT_SECOND_HALF_EVEN;
      }
    }
  }

#ifdef DEBUG_VERBOSE_ON
  printf("$%d:\n", inst->getVISAId());
  for (int i = 0; i < 3; i++) {
    if (dcls[i]) {
      printf("%s, ", dcls[i]->getName());

      if (gra.getBankConflict(dcls[i]) == BANK_CONFLICT_FIRST_HALF_EVEN) {
        printf("%s\n", "EVEN");
      } else if (gra.getBankConflict(dcls[i]) ==
                 BANK_CONFLICT_SECOND_HALF_ODD) {
        printf("%s\n", "ODD");
      } else {
        printf("%s\n", "NONE");
      }
    }
  }
  printf("\n");
#endif

  return;
}

void BankConflictPass::setupBankConflictsForBB(G4_BB *bb,
                                               unsigned &threeSourceInstNum,
                                               unsigned &sendInstNum,
                                               unsigned numRegLRA,
                                               unsigned &internalConflict) {
  int bank1RegNum = 0;
  int bank2RegNum = 0;
  float GRFRatio = 0;
  G4_INST *prevInst = nullptr;

  if (numRegLRA) {
    GRFRatio = ((float)(numRegLRA - SECOND_HALF_BANK_START_GRF)) /
               SECOND_HALF_BANK_START_GRF;
  }

  for (auto i = bb->rbegin(), rend = bb->rend(); i != rend; i++) {
    G4_INST *inst = (*i);
    if (inst->getNumSrc() == 3 && !inst->isSend()) {
      threeSourceInstNum++;
      setupBankConflictsOneGRFOld(inst, bank1RegNum, bank2RegNum, GRFRatio,
                                  internalConflict);
    }
    if (inst->isSend() && !inst->isEOT()) {
      // Why only data port read causes issue?
      if (inst->getMsgDesc()->isRead()) {
        sendInstNum++;
      }
    }
  }

  if ((float)threeSourceInstNum / bb->size() > 0.1) {
    if (!gra.kernel.fg.builder->lowHighBundle() &&
        gra.kernel.fg.builder->hasEarlyGRFRead()) {
      for (G4_INST *inst : *bb) {
        if (prevInst && inst->getNumSrc() == 3 && !inst->isSend()) {
          setupBankForSrc0(inst, prevInst);
        }
        prevInst = inst;
      }
    }
  }
}

void BankConflictPass::setupBankConflictsForBBTGL(G4_BB *bb,
                                                  unsigned &threeSourceInstNum,
                                                  unsigned &sendInstNum,
                                                  unsigned numRegLRA,
                                                  unsigned &internalConflict) {
  float GRFRatio = 0;
  G4_INST *prevInst = nullptr;

  if (numRegLRA) {
    GRFRatio = ((float)(numRegLRA - SECOND_HALF_BANK_START_GRF)) /
               SECOND_HALF_BANK_START_GRF;
  }

  for (auto i = bb->rbegin(), rend = bb->rend(); i != rend; i++) {
    G4_INST *inst = (*i);
    if (inst->isSend() || inst->isCFInst() || inst->isLabel() ||
        inst->isOptBarrier()) {
      if (inst->isSend() && !inst->isEOT()) {
        // Why only data port read causes issue?
        if (inst->getMsgDesc()->isRead()) {
          sendInstNum++;
        }
      }
      continue;
    }
    if (inst->getNumSrc() == 3) {
      threeSourceInstNum++;
      if (inst->isDpas()) {
        hasDpasInst = true;
        setupBankConflictsforDPAS(inst);
      } else {
        setupBankConflictsforMad(inst);
      }
    } else if (gra.kernel.getOption(vISA_forceBCR) && !forGlobal &&
               inst->getNumSrc() == 2) {
      threeSourceInstNum++;
      setupBankConflictsforMad(inst);
    }
  }

  if ((float)threeSourceInstNum / bb->size() > 0.1) {
    if (!gra.kernel.fg.builder->lowHighBundle() &&
        gra.kernel.fg.builder->hasEarlyGRFRead()) {
      for (G4_INST *inst : *bb) {
        if (prevInst && inst->getNumSrc() == 3 && !inst->isSend()) {
          setupBankForSrc0(inst, prevInst);
        }
        prevInst = inst;
      }
    }
  }
}

// Use for BB sorting according to the loop nest level and the BB size.
bool compareBBLoopLevel(G4_BB *bb1, G4_BB *bb2) {
  if (bb1->getNestLevel() > bb2->getNestLevel()) {
    return true;
  } else if (bb1->getNestLevel() == bb2->getNestLevel()) {
    return bb1->size() > bb2->size();
  }

  return false;
}

/*
 * output:
 *        threeSourceCandidate, if there are enough three source instructions
 *        return value, if do bank confliction reduction to RR RA.
 */
bool BankConflictPass::setupBankConflictsForKernel(bool doLocalRR,
                                                   bool &threeSourceCandidate,
                                                   unsigned numRegLRA,
                                                   bool &highInternalConflict) {
  unsigned threeSourceInstNumInKernel = 0;
  unsigned internalConflict = 0;
  unsigned instNumInKernel = 0;
  unsigned sendInstNumInKernel = 0;

  std::vector<G4_BB *> orderedBBs(gra.kernel.fg.cbegin(), gra.kernel.fg.cend());
  std::sort(orderedBBs.begin(), orderedBBs.end(), compareBBLoopLevel);

  for (auto bb : orderedBBs) {
    unsigned instNum = 0;
    unsigned sendInstNum = 0;
    unsigned threeSourceInstNum = 0;
    unsigned conflicts = 0;

    unsigned loopNestLevel = 0;

    if (gra.kernel.fg.builder->lowHighBundle()) {
      setupBankConflictsForBB(bb, threeSourceInstNum, sendInstNum, numRegLRA,
                              conflicts);
    } else {
      setupBankConflictsForBBTGL(bb, threeSourceInstNum, sendInstNum, numRegLRA,
                                 conflicts);
    }

    loopNestLevel = bb->getNestLevel() + 1;

    if (threeSourceInstNum) {
      instNum = (uint32_t)bb->size() * loopNestLevel *
                BANK_CONFLICT_HEURISTIC_LOOP_ITERATION;
      threeSourceInstNum = threeSourceInstNum * loopNestLevel *
                           BANK_CONFLICT_HEURISTIC_LOOP_ITERATION;
      sendInstNum =
          sendInstNum * loopNestLevel * BANK_CONFLICT_HEURISTIC_LOOP_ITERATION;
      conflicts =
          conflicts * loopNestLevel * BANK_CONFLICT_HEURISTIC_LOOP_ITERATION;
      internalConflict += conflicts;
      threeSourceInstNumInKernel += threeSourceInstNum;
      instNumInKernel += instNum;
      sendInstNumInKernel += sendInstNum;
    }
  }

  if (!threeSourceInstNumInKernel ||
      (float)threeSourceInstNumInKernel / instNumInKernel <
          BANK_CONFLICT_HEURISTIC_INST) {
    return false;
  }

  highInternalConflict =
      ((float)internalConflict / threeSourceInstNumInKernel) >
      INTERNAL_CONFLICT_RATIO_HEURISTIC;

  // Bank conflict reduction is done only when there is enough three source
  // instructions.
  threeSourceCandidate = true;

  if (doLocalRR && sendInstNumInKernel) {
    if (!hasDpasInst && (sendInstNumInKernel > threeSourceInstNumInKernel)) {
      return false;
    }
  }

  return true;
}

bool GlobalRA::areAllDefsNoMask(G4_Declare *dcl) {
  bool retval = true;
  auto &maskUsed = getMask(dcl);
  if (maskUsed.size() > 0 &&
      getAugmentationMask(dcl) != AugmentationMasks::NonDefault) {
    auto byteSize = dcl->getByteSize();
    for (unsigned i = 0; i < byteSize; i++) {
      if (maskUsed[i] != NOMASK_BYTE) {
        retval = false;
        break;
      }
    }
  } else {
    if (getAugmentationMask(dcl) == AugmentationMasks::NonDefault)
      retval = true;
    else
      retval = false;
  }
  return retval;
}

BankAlign GlobalRA::getBankAlign(const G4_Declare *dcl) const {
  const IR_Builder *builder = kernel.fg.builder;
  switch (getBankConflict(dcl)) {
  case BANK_CONFLICT_FIRST_HALF_EVEN:
  case BANK_CONFLICT_SECOND_HALF_EVEN:
    return builder->oneGRFBankDivision() ? BankAlign::Even
                                         : BankAlign::Even2GRF;
  case BANK_CONFLICT_FIRST_HALF_ODD:
  case BANK_CONFLICT_SECOND_HALF_ODD:
    return builder->oneGRFBankDivision() ? BankAlign::Odd : BankAlign::Odd2GRF;
  default:
    return BankAlign::Either;
  }
}

void GlobalRA::emitFGWithLiveness(const LivenessAnalysis &liveAnalysis) const {
  VISA_DEBUG_VERBOSE({
    for (G4_BB *bb : kernel.fg) {
      std::cout << "\n"
                << "-------------------------------------------------------"
                   "----------";
      std::cout << "\nBB" << bb->getId() << ":";
      std::cout << "\nPreds: ";
      for (const G4_BB *pred : bb->Preds)
        std::cout << "BB" << pred->getId() << ", ";
      std::cout << "\nSuccs: ";
      for (const G4_BB *succ : bb->Succs)
        std::cout << "BB" << succ->getId() << ", ";

      if (kernel.getOption(vISA_LocalRA)) {
        if (auto summary = getBBLRASummary(bb)) {
          std::cout << "\nLocal RA: ";
          for (unsigned i = 0; i < kernel.getNumRegTotal(); i++) {
            if (summary->isGRFBusy(i))
              std::cout << "r" << i << ", ";
          }
        }
      }

      std::cout << "\nGen: ";
      for (const G4_Declare *dcl : kernel.Declares) {
        if (dcl->getAliasDeclare())
          continue;

        if (dcl->getRegVar()->isRegAllocPartaker()) {
          if (liveAnalysis.use_gen[bb->getId()].test(
                  dcl->getRegVar()->getId())) {
            std::cout << dcl->getName() << ", ";
          }
        }
      }

      std::cout << "\nKill: ";
      for (const G4_Declare *dcl : kernel.Declares) {
        if (dcl->getAliasDeclare())
          continue;

        if (dcl->getRegVar()->isRegAllocPartaker()) {
          if (liveAnalysis.use_kill[bb->getId()].test(
                  dcl->getRegVar()->getId())) {
            std::cout << dcl->getName() << ", ";
          }
        }
      }

      std::cout << "\nLive-in: ";
      for (const G4_Declare *dcl : kernel.Declares) {
        if (dcl->getAliasDeclare())
          continue;

        if (dcl->getRegVar()->isRegAllocPartaker()) {
          if (liveAnalysis.isLiveAtEntry(bb, dcl->getRegVar()->getId())) {
            std::cout << dcl->getName() << ", ";
          }
        }
      }

      std::cout << "\nLive-out: ";
      for (const G4_Declare *dcl : kernel.Declares) {
        if (dcl->getAliasDeclare())
          continue;

        if (dcl->getRegVar()->isRegAllocPartaker()) {
          if (liveAnalysis.isLiveAtExit(bb, dcl->getRegVar()->getId())) {
            std::cout << dcl->getName() << ", ";
          }
        }
      }
      std::cout << "\n";
      bb->emit(COUT_ERROR);
    }
  });
}

void GlobalRA::reportSpillInfo(const LivenessAnalysis &liveness,
                               const GraphColor &coloring) const {
  // Emit out interference graph of each spill candidate
  // and if a spill candidate is a local range, emit its
  // start and end line number in file.
  const auto& lrs = coloring.getLiveRanges();

  for (const vISA::LiveRange *slr : coloring.getSpilledLiveRanges()) {
    if (slr->getRegKind() == G4_GRF) {
      const G4_RegVar *spillVar = slr->getVar();
      VISA_DEBUG_VERBOSE({
        std::cout << "Spill candidate " << spillVar->getName() << " intf:";
        std::cout << "\t(" << spillVar->getDeclare()->getTotalElems()
                  << "):" << TypeSymbol(spillVar->getDeclare()->getElemType())
                  << "\n";
      });

      if (getLocalLR(spillVar->getDeclare())) {
        if (getLocalLR(spillVar->getDeclare())->isLiveRangeLocal()) {
          int start, end;
          unsigned dummy;
          start = getLocalLR(spillVar->getDeclare())
                      ->getFirstRef(dummy)
                      ->getLineNo();
          end = getLocalLR(spillVar->getDeclare())
                    ->getLastRef(dummy)
                    ->getLineNo();
          VISA_DEBUG_VERBOSE(std::cout
                             << "(Liverange is local starting at line #"
                             << start << " and ending at line #" << end << ")"
                             << "\n");

        }
      }

      const Interference *intf = coloring.getIntf();
      unsigned spillVarId = slr->getVar()->getId();

      for (int i = 0; i < (int)liveness.getNumSelectedVar(); i++) {
        if (intf->interfereBetween(spillVarId, i)) {
          const G4_RegVar *intfRangeVar = lrs[i]->getVar();
          (void)intfRangeVar;
          VISA_DEBUG_VERBOSE(
              std::cout << "\t" << intfRangeVar->getName() << "("
                        << intfRangeVar->getDeclare()->getTotalElems() << "):"
                        << TypeSymbol(
                               intfRangeVar->getDeclare()->getElemType()));

          if (!lrs[i]->getPhyReg()) {
            VISA_DEBUG_VERBOSE(std::cout << " --- spilled");
          }
          VISA_DEBUG_VERBOSE(std::cout << ",\n");
        }
      }

      VISA_DEBUG_VERBOSE(std::cout << "\n\n");
    }
  }
}

LiveRange::LiveRange(G4_RegVar *v, GlobalRA &g)
    : var(v), dcl(v->getDeclare()), regKind(dcl->getRegFile()), gra(g) {
  isCandidate = true;

  if (getRegKind() == G4_ADDRESS)
    numRegNeeded = v->getDeclare()->getNumElems() *
                   v->getDeclare()->getElemSize() / G4_WSIZE;
  else if (getRegKind() == G4_FLAG) {
    // number of elements are in words
    numRegNeeded = v->getDeclare()->getNumElems();
  } else {
    // number of GRFs
    numRegNeeded = v->getDeclare()->getNumRows();
  }
}

void LiveRange::initializeForbidden() {
  auto rf = gra.incRA.getSelectedRF();
  if (LivenessAnalysis::livenessClass(rf, G4_ADDRESS)) {
    setForbidden(forbiddenKind::FBD_ADDR);
  } else if (LivenessAnalysis::livenessClass(rf, G4_FLAG)) {
    setForbidden(forbiddenKind::FBD_FLAG);
  } else {
    setForbidden(forbiddenKind::FBD_RESERVEDGRF);
  };

  bool hasStackCall =
      gra.kernel.fg.getHasStackCalls() || gra.kernel.fg.getIsStackCallFunc();
  setCallerSaveBias(hasStackCall);
  if (getRegKind() == G4_GRF) {
    if (gra.kernel.fg.isPseudoVCADcl(dcl)) {
      setForbidden(forbiddenKind::FBD_CALLERSAVE);
    } else if (gra.kernel.fg.isPseudoVCEDcl(dcl)) {
      setForbidden(forbiddenKind::FBD_CALLEESAVE);
    } else if (dcl == gra.getOldFPDcl()) {
      setForbidden(forbiddenKind::FBD_CALLERSAVE);
    }
  }
}

void LiveRange::initialize() {
  if (gra.kernel.fg.isPseudoDcl(dcl)) {
    setIsPseudoNode();
  }
  if (dcl->getIsPartialDcl()) {
    if (G4_Declare *parentDcl = gra.getSplittedDeclare(dcl)) {
      setParentLRID(parentDcl->getRegVar()->getId());
      setIsPartialDcl();
    }
  }
  if (dcl->getIsSplittedDcl()) {
    setIsSplittedDcl(true);
  }
  setBC(gra.getBankConflict(dcl));

  initializeForbidden();
}

LiveRange *LiveRange::createNewLiveRange(G4_Declare *dcl, GlobalRA &gra) {
  auto &IncRAMem = gra.incRA.mem;
  G4_RegVar *var = dcl->getRegVar();
  vISA_ASSERT(!dcl->getAliasDeclare(),
              "error: attempt to create LiveRange for non-root dcl");
  auto *lr = new (IncRAMem) LiveRange(var, gra);

  lr->initialize();

  return lr;
}

void LiveRange::checkForInfiniteSpillCost(
    G4_BB *bb, std::list<G4_INST *>::reverse_iterator &it) {
  // G4_INST at *it defines liverange object (this ptr)
  // If next instruction of iterator uses same liverange then
  // it may be a potential infinite spill cost candidate.
  // To confirm, following requirements should be fulfilled:
  // a. this liverange is not a global
  // b. this liverange is defined/used in these 2 instructions only
  //
  // The idea is for ranges marked with infinite spill cost,
  // coloring will attempt to put them on top of stack so they
  // have higher chance of getting a color. If a range that should
  // be infinite spill cost is not marked as being so, the only
  // downside is extra compile time spent in inserting spill code
  // and then punting out when later spilled code will cause
  // even more spills.
  //
  // The assumption is that current live-range is a current register
  // allocation candidate.
  //
  G4_INST *curInst = (*it);

  // Skip the check if curInst is a pseudoKill
  // Otherwise, it may invalidate a previously marked infinite
  // spill cost candidate, e.g.,
  // pseudo_kill (1) P1(0,0)[1]:uw [Align1]
  // mov (1) P1(0,0)[1]:uw TV1(8,0)[0;1,0]:uw [Align1, NoMask]
  // (+P1.0) sel (16) V65(0,0)[1]:f TV0(0,0)[0;1,0]:f 0:f [Align1, H1]
  if (curInst->isPseudoKill()) {
    return;
  }

  // Check whether dst variable is a global
  if (gra.isBlockLocal(this->getDcl()) == false) {
    isCandidate = false;
    isInfiniteCost = false;

    return;
  }

  G4_DstRegRegion *dst = curInst->getDst();
  // If cur instruction dst is indirect write then return
  if (dst && dst->getRegAccess() == IndirGRF &&
      dst->getBase()->asRegVar()->getId() == this->getVar()->getId()) {
    return;
  }

  // isCandidate is set to true only for first definition ever seen.
  // If more than 1 def if found this gets set to false.
  const std::list<G4_INST *>::reverse_iterator rbegin = bb->rbegin();
  if (this->isCandidate == true && it != rbegin) {
    G4_INST *nextInst = NULL;
    if (this->getRefCount() != 2 || (this->getRegKind() == G4_GRF &&
                                     this->getDcl()->getAddressed() == true)) {
      // If a liverange has > 2 refs then it
      // cannot be a candidate.
      // Also an address taken GRF is not a candidate.
      // This represents an early exit.
      isCandidate = false;
      isInfiniteCost = false;

      return;
    }

    // Skip all pseudo kills
    std::list<G4_INST *>::reverse_iterator next = it;
    while (true) {
      if (next == rbegin) {
        isCandidate = isInfiniteCost = false;
        return;
      }
      --next;

      // This is not a pseudo-kill instruction, then find
      // the desired next instruction. Otherwise, continue.
      nextInst = *next;
      if (!(nextInst->isPseudoKill()))
        break;
    }

    // Check whether this liverange is used in nextInst
    for (unsigned i = 0, numSrc = nextInst->getNumSrc(); i < numSrc; i++) {
      G4_Operand *src = nextInst->getSrc(i);

      if (src && src->isSrcRegRegion() &&
          src->getBase()->isRegAllocPartaker()) {
        // src can be Direct/Indirect
        G4_SrcRegRegion *srcRgn = src->asSrcRegRegion();

        if (srcRgn->getRegAccess() == Direct && srcRgn->getBase()->isRegVar() &&
            srcRgn->getBase()->asRegVar()->getId() == this->getVar()->getId()) {
          // Def-use found back-to-back
          isInfiniteCost = true;
          // Identify no more candidates
          isCandidate = false;
        } else if (this->getRegKind() == G4_ADDRESS &&
                   srcRgn->getRegAccess() == IndirGRF &&
                   srcRgn->getBase()->isRegVar() &&
                   srcRgn->getBase()->asRegVar()->getId() ==
                       this->getVar()->getId()) {
          // Def-use found back-to-back
          isInfiniteCost = true;
          // Identify no more candidates
          isCandidate = false;
        }
      }
    }

    G4_DstRegRegion *nextDst = nextInst->getDst();
    if (isCandidate == true && this->getRegKind() == G4_ADDRESS && nextDst &&
        nextDst->getRegAccess() == IndirGRF && nextDst->getBase()->isRegVar() &&
        nextDst->getBase()->asRegVar()->isRegAllocPartaker() &&
        nextDst->getBase()->asRegVar()->getId() == this->getVar()->getId()) {
      // Pattern found:
      // A0=
      // r[A0]=
      isInfiniteCost = true;
      // Identify no more candidates
      isCandidate = false;
    }

    if (isCandidate == true && this->getRegKind() == G4_FLAG &&
        nextInst->getPredicate() && nextInst->getPredicate()->getBase() &&
        nextInst->getPredicate()->getBase()->isRegVar() &&
        nextInst->getPredicate()->getBase()->asRegVar()->isRegAllocPartaker() &&
        nextInst->getPredicate()->getBase()->asRegVar()->getId() ==
            this->getVar()->getId()) {
      // Pattern found:
      // P0 = or cmp.P0 = <-- P0 defined
      // (P0) ... <-- P0 used as predicate
      isInfiniteCost = true;
      // Identify no more candidates
      isCandidate = false;
    }

    VISA_DEBUG_VERBOSE({
      if (isInfiniteCost == true) {
        std::cout
            << "Marking " << this->getDcl()->getName()
            << " as having infinite spill cost due to back-to-back def-use"
            << "\n";
      }
    });

    // Once a def is seen, stop looking for more defs
    isCandidate = false;
  } else {
    VISA_DEBUG_VERBOSE({
      if (isInfiniteCost == true) {
        std::cout << "Unmarking " << this->getDcl()->getName()
                  << " as having infinite spill cost"
                  << "\n";
      }
    });
    isCandidate = false;
    isInfiniteCost = false;
  }
}

//
// return true, if live ranges v1 and v2 interfere
//
bool Interference::interfereBetween(unsigned v1, unsigned v2) const {
  if (v1 > v2) {
    std::swap(v1, v2);
  }

  if (useDenseMatrix()) {
    unsigned col = v2 / BITS_DWORD;
    return matrix[v1 * rowSize + col] & (1 << (v2 % BITS_DWORD));
  } else {
    auto &set1 = sparseMatrix[v1];
    auto &set2 = sparseMatrix[v2];
    return set1.test(v2) || set2.test(v1);
  }
}

//
// init live vector with all live ranges that are live at the exit
// also set the next seq use of any live range that is live across to be INT_MAX
// to indicate that this live range does not have exclusive sequential uses and
// hence is not a candidate for being marked with an infinite spill cost.
//
void Interference::buildInterferenceAtBBExit(const G4_BB *bb,
                                             llvm_SBitVector &live) {

  // live must be empty at this point
  live = liveAnalysis->use_out[bb->getId()];
  live &= liveAnalysis->def_out[bb->getId()];
}

//
// Filter out partial or splitted declares in batch interference.
//
inline void Interference::filterSplitDclares(unsigned startIdx, unsigned endIdx,
                                             unsigned n, unsigned col,
                                             unsigned &elt, bool is_partial) {

  if (is_partial) // Don't interference with parent
  {
    unsigned rowSplited = n / BITS_DWORD;
    if (rowSplited == col) {
      elt &= ~(1 << (n % BITS_DWORD));
    }
  }

  // if current is splitted dcl, don't interference with any of its child nodes.
  // if current is partial dcl, don't interference with any other child nodes.
  if (col >= startIdx / BITS_DWORD && col < (endIdx / BITS_DWORD + 1)) {
    unsigned selt = 0;
    unsigned start_id = col * BITS_DWORD > startIdx ? 0 : startIdx % BITS_DWORD;
    unsigned end_id =
        (col + 1) * BITS_DWORD > endIdx ? endIdx % BITS_DWORD : BITS_DWORD;

    for (unsigned i = start_id; i < end_id; i++) {
      selt |= 1 << i;
    }
    elt &= ~selt;
  }

  return;
}

//
// set interference for all live ranges that are currently live
// for partial declares, following rules are applied
// a. current partial declare does not interference with any other partial
// declare b. current parent declare does not interference with its children
// declares, can children declare interference with parent declare? c. current
// partial declare does not interference with hybrid declares added by local RA,
// the reason is simple, these declares are assigned register already.
//
void Interference::buildInterferenceWithLive(const llvm_SBitVector &live,
                                             unsigned i) {
  // set interference between variable with index "i" and variable set in "live".
  // j is the valid bit index in the live.
  for (unsigned j : live) {
    if (!varSplitCheckBeforeIntf(i, j)) {
      if (j < i) {
        safeSetInterference(j, i);
      } else if (j > i) {
        safeSetInterference(i, j);
      }
    }
  }
  const LiveRange *lr = lrs[i];
  bool is_partial = lr->getIsPartialDcl();
  bool is_splitted = lr->getIsSplittedDcl();
  unsigned n = 0;
  unsigned start_idx = 0; // The variable index of the first child declare, the
                          // child variables' indexes are contigious.
  unsigned end_idx = 0;   // The variable index of the last child declare
  if (is_splitted) // if current is splitted dcl, don't interference with all
                   // its child nodes.
  {
    start_idx = lr->getDcl()->getSplitVarStartID();
    end_idx = start_idx + gra.getSplitVarNum(lr->getDcl());
  }
  if (is_partial) // if current is partial dcl, don't interference with all
                  // other partial dcls, and it's parent dcl.
  {
    // n is the variable ID of the splitted(parent) declare
    n = gra.getSplittedDeclare(lr->getDcl())->getRegVar()->getId();
    start_idx = splitStartId;
    end_idx = splitStartId + splitNum;
  }

  if (is_partial) { // Don't interference with parent
    if (i < n) {
      safeClearInterference(i, n);
    } else {
      safeClearInterference(n, i);
    }
  }
  for (unsigned j = start_idx; j < end_idx; j++) { //Don't inteference with the child
    if (j < i) {
      safeClearInterference(j, i);
    } else {
      safeClearInterference(i, j);
    }
  }
}

void Interference::buildInterferenceWithSubDcl(unsigned lr_id, G4_Operand *opnd,
                                               llvm_SBitVector &live, bool setLive,
                                               bool setIntf) {

  const G4_Declare *dcl = lrs[lr_id]->getDcl();
  for (const G4_Declare *subDcl : gra.getSubDclList(dcl)) {
    unsigned leftBound = gra.getSubOffset(subDcl);
    unsigned rightBound = leftBound + subDcl->getByteSize() - 1;
    if (!(opnd->getRightBound() < leftBound ||
          rightBound < opnd->getLeftBound())) {
      int subID = subDcl->getRegVar()->getId();

      if (setIntf) {
        buildInterferenceWithLive(live, subID);
      }
      if (setLive) {
        live.set(subID);
      }
    }
  }

  return;
}

void Interference::buildInterferenceWithAllSubDcl(unsigned v1, unsigned v2) {
  const G4_Declare *d1 = lrs[v1]->getDcl();
  const G4_Declare *d2 = lrs[v2]->getDcl();

  if (d1->getIsSplittedDcl() && !d2->getIsPartialDcl()) {
    for (const G4_Declare *subDcl : gra.getSubDclList(d1)) {
      int subID = subDcl->getRegVar()->getId();
      checkAndSetIntf(v2, subID);
    }
  }

  if (d2->getIsSplittedDcl() && !d1->getIsPartialDcl()) {
    for (const G4_Declare *subDcl : gra.getSubDclList(d2)) {
      int subID = subDcl->getRegVar()->getId();
      checkAndSetIntf(v1, subID);
    }
  }

  return;
}
//
// Bias the live ranges in "live" to be assigned the callee-save registers as
// they are live through a stack call. Exclude file scope variables as they are
// always save/restore before/after call and are better assigned to the
// caller-save space.
//
void Interference::addCalleeSaveBias(const llvm_SBitVector &live) {
  for (unsigned i = 0; i < maxId; i++) {
    if (live.test(i)) {
      lrs[i]->setCallerSaveBias(false);
      lrs[i]->setCalleeSaveBias(true);
    }
  }
}

void Interference::buildInterferenceAmongLiveOuts() {
  // Mark interference between dcls marked as Output.
  //
  // Interference computation marks interference for a
  // variable only when definition for that variable is
  // seen, not otherwise.
  //
  // This method is useful when definition of such
  // "Output" variables are emitted to program post RA.
  //
  // It is safe to mark interference between all "Output"
  // dcls even when their definition is present in the program.

  // First gather all Output dcls in a vector to avoid an O(N^2)
  // lookup. Number of OutputDcls should be small.
  std::vector<G4_Declare *> OutputDcls;
  for (auto dcl : kernel.Declares) {
    if (!dcl->getRegVar()->isRegAllocPartaker() || !dcl->isOutput())
      continue;

    OutputDcls.push_back(dcl);
  }

  for (auto dcl1 : OutputDcls) {
    // dcl1 is RA partaker iter and is marked as Output
    for (auto dcl2 : OutputDcls) {
      if (dcl1 == dcl2)
        continue;

      checkAndSetIntf(dcl1->getRegVar()->getId(), dcl2->getRegVar()->getId());
    }
  }
}

void Interference::buildInterferenceAmongLiveIns() {
  //
  // Build interference between all live-ins. If all live-ins are only
  // read then their interference will be skipped in earlier phase.
  // For eg, arg and globals are both live-in. And both may only have
  // uses in function and no def.
  //
  const G4_BB *entryBB = kernel.fg.getEntryBB();

  for (auto it = liveAnalysis->globalVars.begin();
       it != liveAnalysis->globalVars.end(); ++it) {
    auto i = (*it);
    if (liveAnalysis->isLiveAtEntry(entryBB, i)) {
      // Mark reference can not gaurantee all the varaibles are local, update
      // here
      if (lrs[i]->getDcl()->getIsSplittedDcl()) {
        lrs[i]->getDcl()->setIsSplittedDcl(false);
        lrs[i]->setIsSplittedDcl(false);
      }

      auto nextIt = it;
      for (auto nit = ++nextIt; nit != liveAnalysis->globalVars.end(); ++nit) {
        auto j = (*nit);
        if (liveAnalysis->isLiveAtEntry(entryBB, j)) {
          if (lrs[i]->getDcl()->getRegFile() == G4_INPUT &&
              lrs[i]->getVar()->getPhyReg() != NULL &&
              lrs[j]->getDcl()->getRegFile() == G4_INPUT &&
              lrs[j]->getVar()->getPhyReg() != NULL) {
            continue;
          } else {
            if (!varSplitCheckBeforeIntf(i, j)) {
              checkAndSetIntf(i, j);
            }
          }
        }
      }
    }
  }
}

void Interference::markInterferenceForSend(G4_BB *bb, G4_INST *inst,
                                           G4_DstRegRegion *dst) {
  bool isDstRegAllocPartaker = false;
  bool isDstLocallyAssigned = false;
  unsigned dstId = 0;
  int dstPreg = 0, dstNumRows = 0;

  if (dst->getBase()->isRegVar()) {
    if (dst->getBase()->isRegAllocPartaker()) {
      G4_DstRegRegion *dstRgn = dst;
      isDstRegAllocPartaker = true;
      dstId = ((G4_RegVar *)dstRgn->getBase())->getId();
    } else if (kernel.getOption(vISA_LocalRA)) {
      LocalLiveRange *localLR = NULL;
      G4_Declare *topdcl = GetTopDclFromRegRegion(dst);

      if (topdcl)
        localLR = gra.getLocalLR(topdcl);

      if (localLR && localLR->getAssigned()) {
        int sreg;
        G4_VarBase *preg = localLR->getPhyReg(sreg);

        vISA_ASSERT(preg->isGreg(), "Register in dst was not GRF");

        isDstLocallyAssigned = true;
        dstPreg = preg->asGreg()->getRegNum();
        dstNumRows = localLR->getTopDcl()->getNumRows();
      }
    }

    if (isDstRegAllocPartaker || isDstLocallyAssigned) {
      for (unsigned j = 0, numSrc = inst->getNumSrc(); j < numSrc; j++) {
        G4_Operand *src = inst->getSrc(j);
        if (src && src->isSrcRegRegion() &&
            src->asSrcRegRegion()->getBase()->isRegVar()) {
          if (src->asSrcRegRegion()->getBase()->isRegAllocPartaker()) {
            unsigned srcId =
                src->asSrcRegRegion()->getBase()->asRegVar()->getId();

            if (isDstRegAllocPartaker) {
              if (!varSplitCheckBeforeIntf(dstId, srcId)) {
                checkAndSetIntf(dstId, srcId);
                buildInterferenceWithAllSubDcl(dstId, srcId);
              }
            } else {
              for (int j = dstPreg, sum = dstPreg + dstNumRows; j < sum; j++) {
                int k = getGRFDclForHRA(j)->getRegVar()->getId();
                if (!varSplitCheckBeforeIntf(k, srcId)) {
                  checkAndSetIntf(k, srcId);
                  buildInterferenceWithAllSubDcl(k, srcId);
                }
              }
            }
          } else if (kernel.getOption(vISA_LocalRA) && isDstRegAllocPartaker) {
            LocalLiveRange *localLR = nullptr;
            const G4_Declare *topdcl = GetTopDclFromRegRegion(src);

            if (topdcl)
              localLR = gra.getLocalLR(topdcl);

            if (localLR && localLR->getAssigned()) {
              int sreg;
              G4_VarBase *preg = localLR->getPhyReg(sreg);
              int numrows = localLR->getTopDcl()->getNumRows();

              vISA_ASSERT(preg->isGreg(), "Register in src was not GRF");

              int reg = preg->asGreg()->getRegNum();

              for (int j = reg, sum = reg + numrows; j < sum; j++) {
                int k = getGRFDclForHRA(j)->getRegVar()->getId();
                if (!varSplitCheckBeforeIntf(dstId, k)) {
                  checkAndSetIntf(dstId, k);
                  buildInterferenceWithAllSubDcl(dstId, k);
                }
              }
            }
          }
        }
      }
    }
  }
}

void Interference::markInterferenceToAvoidDstSrcOverlap(G4_BB *bb,
                                                        G4_INST *inst) {
  bool isDstRegAllocPartaker = false;
  bool isDstLocallyAssigned = false;
  unsigned dstId = 0;
  int dstPreg = 0, dstNumRows = 0;
  bool dstOpndNumRows = false;

  G4_DstRegRegion *dst = inst->getDst();
  if (dst->getBase()->isRegVar() &&
      (dst->getTopDcl()->getRegFile() == G4_GRF)) {
    G4_Declare *dstDcl = dst->getTopDcl();
    int dstOffset = dst->getLeftBound() / kernel.numEltPerGRF<Type_UB>();
    bool isDstEvenAlign = gra.isEvenAligned(dstDcl);

    if (dst->getBase()->isRegAllocPartaker()) {
      isDstRegAllocPartaker = true;
      dstId = ((G4_RegVar *)dst->getBase())->getId();
      dstOpndNumRows = dst->getSubRegOff() * dst->getTypeSize() +
                           dst->getLinearizedEnd() - dst->getLinearizedStart() +
                           1 >
                       kernel.numEltPerGRF<Type_UB>();
    } else if (kernel.getOption(vISA_LocalRA)) {
      LocalLiveRange *localLR = NULL;
      G4_Declare *topdcl = GetTopDclFromRegRegion(dst);

      if (topdcl)
        localLR = gra.getLocalLR(topdcl);
      if (localLR && localLR->getAssigned()) {
        int sreg;
        G4_VarBase *preg = localLR->getPhyReg(sreg);

        vISA_ASSERT(preg->isGreg(), "Register in dst was not GRF");

        isDstLocallyAssigned = true;
        dstPreg = preg->asGreg()->getRegNum();
        dstNumRows = localLR->getTopDcl()->getNumRows();
        dstOpndNumRows = dst->getSubRegOff() * dst->getTypeSize() +
            dst->getLinearizedEnd() - dst->getLinearizedStart() + 1 >
            kernel.numEltPerGRF<Type_UB>();
        isDstEvenAlign = (dstPreg % 2 == 0);
      }
    }

    if (isDstRegAllocPartaker || isDstLocallyAssigned) {
      for (unsigned j = 0, numSrc = inst->getNumSrc(); j < numSrc; j++) {
        if (inst->isDpas() && j != 1)
          continue;
        G4_Operand *src = inst->getSrc(j);
        if (src != NULL && src->isSrcRegRegion() &&
            src->asSrcRegRegion()->getBase()->isRegVar()) {
          G4_SrcRegRegion *srcRgn = src->asSrcRegRegion();
          G4_Declare *srcDcl = src->getTopDcl();
          if (srcRgn->getRegAccess() == Direct &&
              (src->getTopDcl()->getRegFile() == G4_GRF ||
               src->getTopDcl()->getRegFile() == G4_INPUT)) {
            int srcOffset =
                src->getLeftBound() / kernel.numEltPerGRF<Type_UB>();
            bool srcOpndNumRows =
                srcRgn->getSubRegOff() * srcRgn->getTypeSize() +
                                      srcRgn->getLinearizedEnd() -
                                      srcRgn->getLinearizedStart() + 1 >
                kernel.numEltPerGRF<Type_UB>();

            int srcReg = 0;
            bool isSrcEvenAlign = gra.isEvenAligned(srcDcl);
            if (!src->asSrcRegRegion()->getBase()->isRegAllocPartaker() &&
                kernel.getOption(vISA_LocalRA)) {
              int sreg;
              LocalLiveRange *localLR = NULL;
              G4_Declare *topdcl = GetTopDclFromRegRegion(src);

              if (topdcl)
                localLR = gra.getLocalLR(topdcl);
              if (localLR && localLR->getAssigned()) {
                G4_VarBase *preg = localLR->getPhyReg(sreg);

                vISA_ASSERT(preg->isGreg(), "Register in src was not GRF");
                srcReg = preg->asGreg()->getRegNum();
                isSrcEvenAlign = (srcReg % 2 == 0);
              }
            }

            if (srcDcl->getRegFile() == G4_INPUT &&
                srcDcl->getRegVar()->getPhyReg() != NULL &&
                srcDcl->getRegVar()->getPhyReg()->isGreg()) {
              srcReg = srcDcl->getRegVar()->getPhyReg()->asGreg()->getRegNum();
              isSrcEvenAlign = (srcReg % 2 == 0);
            }

            if (dstOpndNumRows || srcOpndNumRows) {
              if (!(isDstEvenAlign && isSrcEvenAlign &&
                    srcOffset % 2 == dstOffset % 2 && dstOpndNumRows &&
                    srcOpndNumRows)) {
                if (src->asSrcRegRegion()->getBase()->isRegAllocPartaker()) {
                  unsigned srcId =
                      src->asSrcRegRegion()->getBase()->asRegVar()->getId();
#ifdef DEBUG_VERBOSE_ON
                  printf("Src%d  ", j);
                  inst->dump();
#endif
                  if (isDstRegAllocPartaker) {
                    if (!varSplitCheckBeforeIntf(dstId, srcId)) {
                      checkAndSetIntf(dstId, srcId);
                      buildInterferenceWithAllSubDcl(dstId, srcId);
                    }
                  } else {
                    for (int j = dstPreg, sum = dstPreg + dstNumRows; j < sum;
                         j++) {
                      int k = getGRFDclForHRA(j)->getRegVar()->getId();
                      if (!varSplitCheckBeforeIntf(k, srcId)) {
                        checkAndSetIntf(k, srcId);
                        buildInterferenceWithAllSubDcl(k, srcId);
                      }
                    }
                  }
                } else if (kernel.getOption(vISA_LocalRA) &&
                           isDstRegAllocPartaker) {
                  LocalLiveRange *localLR = NULL;
                  G4_Declare *topdcl = GetTopDclFromRegRegion(src);

                  if (topdcl)
                    localLR = gra.getLocalLR(topdcl);

                  if (localLR && localLR->getAssigned()) {
                    int reg, sreg, numrows;
                    G4_VarBase *preg = localLR->getPhyReg(sreg);
                    numrows = localLR->getTopDcl()->getNumRows();

                    vISA_ASSERT(preg->isGreg(), "Register in src was not GRF");

                    reg = preg->asGreg()->getRegNum();
#ifdef DEBUG_VERBOSE_ON
                    printf("Src%d  ", j);
                    inst->dump();
#endif
                    for (int j = reg, sum = reg + numrows; j < sum; j++) {
                      int k = getGRFDclForHRA(j)->getRegVar()->getId();
                      if (!varSplitCheckBeforeIntf(dstId, k)) {
                        checkAndSetIntf(dstId, k);
                        buildInterferenceWithAllSubDcl(dstId, k);
                      }
                    }
                  }
                }
              }
            }
          } else if (srcRgn->getRegAccess() == IndirGRF) {
            // make every var in points-to set live
            const REGVAR_VECTOR &pointsToSet =
                liveAnalysis->getPointsToAnalysis().getAllInPointsToOrIndrUse(
                    srcRgn, bb);
            for (auto pt : pointsToSet) {
              if (pt.var->isRegAllocPartaker()) {
                unsigned srcId = pt.var->getId();
                if (isDstRegAllocPartaker) {
                  if (!varSplitCheckBeforeIntf(dstId, srcId)) {
                    checkAndSetIntf(dstId, srcId);
                    buildInterferenceWithAllSubDcl(dstId, srcId);
                  }
                } else {
                  for (int j = dstPreg, sum = dstPreg + dstNumRows; j < sum;
                       j++) {
                    int k = getGRFDclForHRA(j)->getRegVar()->getId();
                    if (!varSplitCheckBeforeIntf(k, srcId)) {
                      checkAndSetIntf(k, srcId);
                      buildInterferenceWithAllSubDcl(k, srcId);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

uint32_t GlobalRA::getRefCount(int loopNestLevel) {
  if (loopNestLevel == 0) {
    return 1;
  }
  return (uint32_t)std::pow(IN_LOOP_REFERENCE_COUNT_FACTOR,
                            std::min(loopNestLevel, 8));
}

// handle return value interference for fcall
void Interference::buildInterferenceForFcall(
    G4_BB *bb, llvm_SBitVector &live, G4_INST *inst,
    std::list<G4_INST *>::reverse_iterator i, const G4_VarBase *regVar) {
  vISA_ASSERT(inst->opcode() == G4_pseudo_fcall, "expect fcall inst");
  if (regVar->isRegAllocPartaker()) {
    unsigned id = static_cast<const G4_RegVar *>(regVar)->getId();

    buildInterferenceWithLive(live, id);
    updateLiveness(live, id, false);
  }
}


void Interference::buildInterferenceForDst(
    G4_BB *bb, llvm_SBitVector &live, G4_INST *inst,
    std::list<G4_INST *>::reverse_iterator i, G4_DstRegRegion *dst) {

  if (dst->getBase()->isRegAllocPartaker()) {
    unsigned id = ((G4_RegVar *)dst->getBase())->getId();
    //
    // In following code,
    // pseudo_kill V10
    // mov (8) V10, V11
    //
    // V10 and V11 do not interfere and can be assigned
    // same register.
    //
    // Following condition skips marking interference for
    // pseudo_kill nodes.
    //
    if (!inst->isPseudoKill() && !inst->isLifeTimeEnd()) {
      buildInterferenceWithLive(live, id);
      if (lrs[id]->getIsSplittedDcl()) {
        buildInterferenceWithSubDcl(id, (G4_Operand *)dst, live, false, true);
      }
    }

    //
    // if the write does not cover the whole dst region, we should continue let
    // the liveness propagate upwards
    //
    if (liveAnalysis->writeWholeRegion(bb, inst, dst, builder.getOptions()) ||
        inst->isPseudoKill()) {
      updateLiveness(live, id, false);

      if (lrs[id]->getIsSplittedDcl()) {
        for (unsigned i = lrs[id]->getDcl()->getSplitVarStartID();
             i < lrs[id]->getDcl()->getSplitVarStartID() +
                     gra.getSplitVarNum(lrs[id]->getDcl());
             i++) {
          live.reset(i); // kill all childs, there may be not used childs
                              // generated due to splitting, killed also.
        }
      }
    }
  } else if (dst->isIndirect() && liveAnalysis->livenessClass(G4_GRF)) {
    //
    // add interferences to the list of potential indirect destination accesses.
    //
    const REGVAR_VECTOR &pointsToSet =
        liveAnalysis->getPointsToAnalysis().getAllInPointsToOrIndrUse(dst, bb);
    for (auto pt : pointsToSet) {
      if (pt.var->isRegAllocPartaker()) {
        buildInterferenceWithLive(live, pt.var->getId());
      }
    }
  }
}

void Interference::buildInterferenceWithinBB(G4_BB *bb, llvm_SBitVector &live) {
  DebugInfoState state;

  for (auto i = bb->rbegin(); i != bb->rend(); i++) {
    G4_INST *inst = (*i);

    G4_DstRegRegion *dst = inst->getDst();
    if (dst) {
      buildInterferenceForDst(bb, live, inst, i, dst);
    }

    if (inst->opcode() == G4_pseudo_fcall) {
      if (liveAnalysis->livenessClass(G4_GRF)) {
        auto fcall = kernel.fg.builder->getFcallInfo(bb->back());
        G4_Declare *arg = kernel.fg.builder->getStackCallArg();
        G4_Declare *ret = kernel.fg.builder->getStackCallRet();
        vISA_ASSERT(fcall != std::nullopt, "fcall info not found");
        uint16_t retSize = fcall->getRetSize();
        uint16_t argSize = fcall->getArgSize();
        if (ret && retSize > 0 && ret->getRegVar()) {
          buildInterferenceForFcall(bb, live, inst, i, ret->getRegVar());
        }
        if (arg && argSize > 0 && arg->getRegVar()) {
          auto id = arg->getRegVar()->getId();
          updateLiveness(live, id, true);
        }
      } else if (liveAnalysis->livenessClass(G4_ADDRESS)) {
        // assume callee will use A0
        auto A0Dcl = kernel.fg.fcallToPseudoDclMap[inst->asCFInst()].A0;
        buildInterferenceWithLive(live, A0Dcl->getRegVar()->getId());
      } else if (liveAnalysis->livenessClass(G4_FLAG)) {
        // assume callee will use both F0 and F1
        auto flagDcl = kernel.fg.fcallToPseudoDclMap[inst->asCFInst()].Flag;
        buildInterferenceWithLive(live, flagDcl->getRegVar()->getId());
      }
    }

    if ((inst->isSend() || inst->isFillIntrinsic()) && !dst->isNullReg() &&
        kernel.fg.builder->WaDisableSendSrcDstOverlap()) {
      markInterferenceForSend(bb, inst, dst);
    } else if (kernel.fg.builder->avoidDstSrcOverlap() && dst &&
               !dst->isNullReg()) {
      markInterferenceToAvoidDstSrcOverlap(bb, inst);
    }

    if (inst->isSplitSend() && !inst->getSrc(1)->isNullReg()) {
      G4_SrcRegRegion *src0 = inst->getSrc(0)->asSrcRegRegion();
      G4_SrcRegRegion *src1 = inst->getSrc(1)->asSrcRegRegion();

      if (src0->getBase()->isRegAllocPartaker() &&
          src1->getBase()->isRegAllocPartaker()) {
        // src0 and src1 of split send may not overlap. In normal cases this is
        // handled automatically as we add interference edge when we reach
        // src0/src1's def.  If one source is an undefined variable (this can
        // happen for URB write payload) and the other an input, however, we
        // could miss the interference edge between the two.  So we add it
        // explicitly here
        int src0Id = src0->getBase()->asRegVar()->getId();
        int src1Id = src1->getBase()->asRegVar()->getId();

        checkAndSetIntf(src0Id, src1Id);
        buildInterferenceWithAllSubDcl(src0Id, src1Id);
      }
    }

    // DPAS: As part of same instruction, src1 should not have overlap with dst.
    // Src0 and src2 are okay to have overlap
    if (inst->isDpas() && !inst->getSrc(1)->isNullReg()) {
      G4_SrcRegRegion *src1 = inst->getSrc(1)->asSrcRegRegion();
      if (dst->getBase()->isRegAllocPartaker() &&
          src1->getBase()->isRegAllocPartaker()) {
        int dstId = dst->getBase()->asRegVar()->getId();
        int src1Id = src1->getBase()->asRegVar()->getId();
        checkAndSetIntf(dstId, src1Id);
        buildInterferenceWithAllSubDcl(dstId, src1Id);
      }
    }

    //
    // process each source operand
    //
    for (unsigned j = 0, numSrc = inst->getNumSrc(); j < numSrc; j++) {
      G4_Operand *src = inst->getSrc(j);
      if (!src)
        continue;
      if (src->isSrcRegRegion()) {
        G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
        if (srcRegion->getBase()->isRegAllocPartaker()) {
          unsigned id = ((G4_RegVar *)(srcRegion)->getBase())->getId();

          if (!inst->isLifeTimeEnd()) {
            updateLiveness(live, id, true);
            if (lrs[id]->getIsSplittedDcl()) {
              buildInterferenceWithSubDcl(id, src, live, true, false);
            }
          }
        } else if (srcRegion->isIndirect() &&
                   liveAnalysis->livenessClass(G4_GRF)) {
          // make every var in points-to set live
          const REGVAR_VECTOR &pointsToSet =
              liveAnalysis->getPointsToAnalysis().getAllInPointsToOrIndrUse(
                  srcRegion, bb);
          for (auto pt : pointsToSet) {
            if (pt.var->isRegAllocPartaker()) {
              updateLiveness(live, pt.var->getId(), true);
            }
          }
        }
      }
    }

    //
    // Process register-indirect destination uses of ARF.
    //
    if (dst) {
      if (dst->getBase()->isRegAllocPartaker() &&
          dst->getRegAccess() != Direct) {
        live.set(dst->getBase()->asRegVar()->getId());
      }
    }

    //
    // Process condMod
    //
    G4_CondMod *mod = inst->getCondMod();
    if (mod != NULL) {
      G4_VarBase *flagReg = mod->getBase();
      if (flagReg != NULL) {
        unsigned id = flagReg->asRegVar()->getId();
        if (flagReg->asRegVar()->isRegAllocPartaker()) {
          buildInterferenceWithLive(live, id);

          if (liveAnalysis->writeWholeRegion(bb, inst, flagReg)) {
            updateLiveness(live, id, false);
          }
        }
      } else {
        vISA_ASSERT((inst->opcode() == G4_sel || inst->opcode() == G4_csel) &&
                        inst->getCondMod() != NULL,
                    "Invalid CondMod");
      }
    }

    //
    // Process predicate
    //
    G4_Predicate *predicate = inst->getPredicate();
    if (predicate != NULL) {
      G4_VarBase *flagReg = predicate->getBase();
      unsigned id = flagReg->asRegVar()->getId();
      if (flagReg->asRegVar()->isRegAllocPartaker()) {
        live.set(id);
      }
    }

    // Update debug info intervals based on live set
    if (builder.getOption(vISA_GenerateDebugInfo)) {
      updateDebugInfo(kernel, inst, *liveAnalysis, lrs, live, &state,
                      inst == bb->front());
    }
  }
}

void Interference::applyPartitionBias() {
  // Any variable that interferes with a VCA dcl is live through an fcall.
  // This function makes such variables callee save biased to avoid save/restore
  // code around fcall. Save/restore may still be needed in case this is a
  // stack call function (vs kernel), but a single save/restore sequence can
  // free the callee save register throughout the function.
  for (auto i : liveAnalysis->globalVars) {
    if (kernel.fg.isPseudoVCADcl(lrs[i]->getDcl())) {
      const auto &intfs = sparseIntf[i];
      for (const auto edge : intfs) {
        // no point adding bias to any variable already assigned
        if (lrs[edge]->getPhyReg())
          continue;

        lrs[edge]->setCalleeSaveBias(true);
        lrs[edge]->setCallerSaveBias(false);
      }
    }
  }
}

// Any setting of LiveRange property that is discovered during interference
// must be done here. Because with incremental RA, we may not run interference
// computation for all BBs.
void Interference::setupLRs(G4_BB *bb) {
  unsigned refCount = GlobalRA::getRefCount(
      kernel.getOption(vISA_ConsiderLoopInfoInRA) ? bb->getNestLevel() : 0);
  bool incSpillCostAddrTaken = kernel.getOption(vISA_IncSpillCostAllAddrTaken);

  for (auto i = bb->rbegin(); i != bb->rend(); i++) {
    G4_INST *inst = (*i);

    auto dst = inst->getDst();
    if (dst) {
      if (dst->getBase()->isRegAllocPartaker()) {
        unsigned id = ((G4_RegVar *)dst->getBase())->getId();
        if (!inst->isPseudoKill() && !inst->isLifeTimeEnd()) {
          lrs[id]->setRefCount(lrs[id]->getRefCount() +
                               refCount); // update reference count
        }
        lrs[id]->checkForInfiniteSpillCost(bb, i);
      } else if (dst->isIndirect() && liveAnalysis->livenessClass(G4_GRF)) {
        const REGVAR_VECTOR &pointsToSet =
            liveAnalysis->getPointsToAnalysis().getAllInPointsToOrIndrUse(dst,
                                                                          bb);
        for (auto pt : pointsToSet) {
          if (!pt.var->isRegAllocPartaker() || !incSpillCostAddrTaken)
            continue;

          lrs[pt.var->getId()]->setRefCount(
              lrs[pt.var->getId()]->getRefCount() + refCount);
        }
      }
    }

    if (inst->opcode() == G4_pseudo_fcall &&
        liveAnalysis->livenessClass(G4_GRF)) {
      auto fcall = kernel.fg.builder->getFcallInfo(bb->back());
      G4_Declare *ret = kernel.fg.builder->getStackCallRet();
      vISA_ASSERT(fcall != std::nullopt, "fcall info not found");
      uint16_t retSize = fcall->getRetSize();
      if (ret && retSize > 0 && ret->getRegVar() &&
          ret->getRegVar()->isRegAllocPartaker()) {
        unsigned id = static_cast<const G4_RegVar *>(ret->getRegVar())->getId();
        lrs[id]->setRefCount(lrs[id]->getRefCount() + refCount);
      }
    }

    if ((inst->isSend() || inst->isFillIntrinsic()) && !dst->isNullReg()) {
      // r127 must not be used for return address when there is a src and dest
      // overlap in send instruction. This applies to split-send as well
      if (kernel.fg.builder->needsToReserveR127() &&
          liveAnalysis->livenessClass(G4_GRF) &&
          dst->getBase()->isRegAllocPartaker() &&
          !dst->getBase()->asRegVar()->isPhyRegAssigned()) {
        int dstId = dst->getBase()->asRegVar()->getId();
        lrs[dstId]->setForbidden(forbiddenKind::FBD_LASTGRF);
      }
    }

    //
    // process each source operand
    //
    for (unsigned j = 0, numSrc = inst->getNumSrc(); j < numSrc; j++) {
      G4_Operand *src = inst->getSrc(j);
      if (!src || !src->isSrcRegRegion())
        continue;

      G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
      if (srcRegion->getBase()->isRegAllocPartaker()) {
        unsigned id = ((G4_RegVar *)(srcRegion)->getBase())->getId();
        lrs[id]->setRefCount(lrs[id]->getRefCount() +
                             refCount); // update reference count
        if (inst->isEOT() && liveAnalysis->livenessClass(G4_GRF)) {
          // mark the liveRange as the EOT source
          lrs[id]->setEOTSrc();
          if (builder.hasEOTGRFBinding()) {
            lrs[id]->setForbidden(forbiddenKind::FBD_EOT);
          }
        }
        if (inst->isReturn()) {
          lrs[id]->setRetIp();
        }
      } else if (srcRegion->isIndirect() &&
                 liveAnalysis->livenessClass(G4_GRF)) {
        // make every var in points-to set live
        const REGVAR_VECTOR &pointsToSet =
            liveAnalysis->getPointsToAnalysis().getAllInPointsToOrIndrUse(
                srcRegion, bb);
        for (auto pt : pointsToSet) {
          if (!pt.var->isRegAllocPartaker() || !incSpillCostAddrTaken)
            continue;

          lrs[pt.var->getId()]->setRefCount(
              lrs[pt.var->getId()]->getRefCount() + refCount);
        }
      }
    }

    //
    // Process condMod
    //
    if (auto mod = inst->getCondMod()) {
      G4_VarBase *flagReg = mod->getBase();
      if (flagReg) {
        unsigned id = flagReg->asRegVar()->getId();
        if (flagReg->asRegVar()->isRegAllocPartaker()) {
          lrs[id]->setRefCount(lrs[id]->getRefCount() +
                               refCount); // update reference count

          lrs[id]->checkForInfiniteSpillCost(bb, i);
        }
      } else {
        vISA_ASSERT((inst->opcode() == G4_sel || inst->opcode() == G4_csel) &&
                        inst->getCondMod() != NULL,
                    "Invalid CondMod");
      }
    }

    //
    // Process predicate
    //
    if (auto predicate = inst->getPredicate()) {
      G4_VarBase *flagReg = predicate->getBase();
      unsigned id = flagReg->asRegVar()->getId();
      if (flagReg->asRegVar()->isRegAllocPartaker()) {
        lrs[id]->setRefCount(lrs[id]->getRefCount() +
                             refCount); // update reference count
      }
    }
  }
}

void Interference::computeInterference() {
  startTimer(TimerID::INTERFERENCE);

  for (auto bb : kernel.fg) {
    // Initialize LR properties like ref count and forbidden here.
    // This method is invoked for all BBs even with incremental RA.
    setupLRs(bb);
  }

  //
  // create bool vector, live, to track live ranges that are currently live
  //
  llvm_SBitVector live;

  buildInterferenceAmongLiveOuts();

  for (G4_BB *bb : kernel.fg) {
    //
    // mark all live ranges dead
    //
    live.clear();
    //
    // start with all live ranges that are live at the exit of BB
    //
    buildInterferenceAtBBExit(bb, live);
    //
    // traverse inst in the reverse order
    //
    buildInterferenceWithinBB(bb, live);
  }

  buildInterferenceAmongLiveIns();

  //
  // Build interference with physical registers assigned by local RA
  //
  if (kernel.getOption(vISA_LocalRA)) {
    for (auto curBB : kernel.fg) {
      buildInterferenceWithLocalRA(curBB);
    }
  }

  RA_TRACE({
    RPE rpe(gra, liveAnalysis);
    rpe.run();
    std::cout << "\t--max RP: " << rpe.getMaxRP() << "\n";
  });

  if ((builder.getOption(vISA_RATrace) ||
       builder.getOption(vISA_DumpPerfStatsVerbose)) &&
      builder.getJitInfo()->statsVerbose.RAIterNum == 1) {
    getNormIntfNum();
  }
  // Augment interference graph to accomodate non-default masks
  aug.augmentIntfGraph();

  generateSparseIntfGraph();

  countNeighbors();

  if (IncrementalRA::isEnabled(kernel)) {
    // Incremental interference was computed for current iteration.
    // Now prepare for incremental temps in next iteration.
    gra.incRA.clearCandidates();
  }

  // apply callee save bias after augmentation as interference graph is
  // up-to-date.
  if (kernel.fg.getHasStackCalls()) {
    applyPartitionBias();
  }
  stopTimer(TimerID::INTERFERENCE);
}

void Interference::getNormIntfNum() {
  unsigned numVars = liveAnalysis->getNumSelectedVar();
  uint32_t numEdges = 0;

  if (useDenseMatrix()) {
    // Iterate over intf graph matrix
    for (unsigned row = 0; row < numVars; row++) {
      unsigned rowOffset = row * rowSize;
      unsigned colStart = (row + 1) / BITS_DWORD;
      for (unsigned j = colStart; j < rowSize; j++) {
        unsigned intfBlk = getInterferenceBlk(rowOffset + j);
        if (intfBlk == 0) {
          continue;
        }
        for (unsigned k = 0; k < BITS_DWORD; k++) {
          if (!(intfBlk & (1 << k))) {
            continue;
          }
          unsigned v2 = (j * BITS_DWORD) + k;
          if (v2 != row) {
            numEdges++;
          }
        }
      }
    }
  } else {
    for (uint32_t v1 = 0; v1 < maxId; ++v1) {
      auto &intfSet = sparseMatrix[v1];
      numEdges += intfSet.count();
    }
  }

  builder.getJitInfo()->statsVerbose.normIntfNum = numEdges;
  std::cout << "\t--normal edge #: " << numEdges << "\n";
}

#define SPARSE_INTF_VEC_SIZE 64

void Interference::generateSparseIntfGraph() {
  // Generate sparse intf graph from the dense one
  unsigned numVars = liveAnalysis->getNumSelectedVar();

  sparseIntf.resize(numVars);

  for (unsigned row = 0; row < numVars; row++) {
    sparseIntf[row].reserve(SPARSE_INTF_VEC_SIZE);
  }

  if (useDenseMatrix()) {
    // Iterate over intf graph matrix
    for (unsigned row = 0; row < numVars; row++) {
      unsigned rowOffset = row * rowSize;
      unsigned colStart = (row + 1) / BITS_DWORD;
      for (unsigned j = colStart; j < rowSize; j++) {
        unsigned intfBlk = getInterferenceBlk(rowOffset + j);
        if (intfBlk != 0) {
          for (unsigned k = 0; k < BITS_DWORD; k++) {
            if (intfBlk & (1 << k)) {
              unsigned v2 = (j * BITS_DWORD) + k;
              if (v2 != row) {
                sparseIntf[v2].emplace_back(row);
                sparseIntf[row].emplace_back(v2);
              }
            }
          }
        }
      }
    }
  } else {
    for (uint32_t v1 = 0; v1 < maxId; ++v1) {
      auto &intfSet = sparseMatrix[v1];
      for (uint32_t v2 : intfSet) {
        sparseIntf[v1].emplace_back(v2);
        sparseIntf[v2].emplace_back(v1);
      }
    }
  }
}

void Interference::countNeighbors() {
  if (!builder.getOption(vISA_RATrace) &&
      !builder.getOption(vISA_DumpPerfStatsVerbose))
    return;

  uint32_t numNeighbor = 0;
  uint32_t maxNeighbor = 0;
  uint32_t maxIndex = 0;
  uint32_t numEdges = 0;
  for (int i = 0, numVar = (int)sparseIntf.size(); i < numVar; ++i) {
    if (lrs[i]->getPhyReg() == nullptr) {
      auto &intf = sparseIntf[i];
      numNeighbor += (uint32_t)intf.size();
      maxNeighbor = std::max(maxNeighbor, numNeighbor);
      if (maxNeighbor == numNeighbor)
        maxIndex = i;
    }
    numEdges += (uint32_t)sparseIntf[i].size();
  }
  float avgNeighbor = ((float)numNeighbor) / sparseIntf.size();
  if (builder.getJitInfo()->statsVerbose.RAIterNum == 1) {
    builder.getJitInfo()->statsVerbose.avgNeighbors = avgNeighbor;
    builder.getJitInfo()->statsVerbose.maxNeighbors = maxNeighbor;
    builder.getJitInfo()->statsVerbose.augIntfNum =
        (numEdges / 2) - builder.getJitInfo()->statsVerbose.normIntfNum;
  }
  RA_TRACE({
    std::cout << "\t--avg # neighbors: " << std::setprecision(6) << avgNeighbor
              << "\n";
    std::cout << "\t--max # neighbors: " << maxNeighbor << " ("
              << lrs[maxIndex]->getDcl()->getName() << ")\n";
    if (builder.getJitInfo()->statsVerbose.RAIterNum == 1) {
      std::cout << "\t--aug edge #: "
                << builder.getJitInfo()->statsVerbose.augIntfNum << "\n";
    }
  });
}

// This function can be invoked before local RA or after augmentation.
// This function will update sub-reg data only for non-NoMask vars and
// leave others unchanged, ie their value will be as per HW conformity
// or earlier phase.
void GlobalRA::updateSubRegAlignment(G4_SubReg_Align subAlign) {
  // Update alignment of all GRF declares to sub-align
  for (auto dcl : kernel.Declares) {
    if (dcl->getRegFile() & G4_GRF && !dcl->getIsPartialDcl()) {
      G4_Declare *topdcl = dcl->getRootDeclare();

      if (!areAllDefsNoMask(topdcl) &&
          getAugmentationMask(topdcl) != AugmentationMasks::NonDefault) {
        dcl->setSubRegAlign(subAlign);
        setSubRegAlign(dcl, subAlign);
      }
    }
  }
}

bool GlobalRA::evenAlignNeeded(G4_Declare *dcl) {
  if (GlobalRA::useGenericAugAlign(builder.getPlatformGeneration())) {
    // Return true if even alignment is needed
    // Even align needed if for given SIMD size and elem type,
    // a complete def uses between 1-2 GRFs.
    auto kernelSimdSizeToUse = kernel.getSimdSizeWithSlicing();
    G4_Declare *topdcl = dcl->getRootDeclare();
    auto topdclAugMask = getAugmentationMask(topdcl);

    if (!areAllDefsNoMask(topdcl) && !topdcl->getIsPartialDcl() &&
        topdclAugMask != AugmentationMasks::NonDefault) {
      auto elemSizeToUse = topdcl->getElemSize();
      if (elemSizeToUse < 4 && topdclAugMask == AugmentationMasks::Default32Bit)
        // :uw with hstride 2 can also be Default32Bit and hence needs even
        // alignment
        elemSizeToUse = 4;
      else if (elemSizeToUse < 8 &&
               topdclAugMask == AugmentationMasks::Default64Bit)
        elemSizeToUse = 8;

      if ( // Even align if size is between 1-2 GRFs, for >2GRF sizes use weak
           // edges
          (elemSizeToUse * kernelSimdSizeToUse) >
              (unsigned)kernel.numEltPerGRF<Type_UB>() &&
          (elemSizeToUse * kernelSimdSizeToUse) <=
              (unsigned)(2 * kernel.numEltPerGRF<Type_UB>()) &&
          !(kernel.fg.builder->getOption(vISA_enablePreemption) &&
            dcl == kernel.fg.builder->getBuiltinR0())) {
        return true;
      }
    }
  } else {
    if (dcl->getRegFile() & G4_GRF) {
      G4_Declare *topdcl = dcl->getRootDeclare();
      auto topdclAugMask = getAugmentationMask(topdcl);

      if (!areAllDefsNoMask(topdcl) && !topdcl->getIsPartialDcl() &&
          topdclAugMask != AugmentationMasks::NonDefault &&
          topdclAugMask != AugmentationMasks::Default64Bit) {
        if ((topdcl->getElemSize() >= 4 ||
             topdclAugMask == AugmentationMasks::Default32Bit) &&
            topdcl->getByteSize() >= kernel.numEltPerGRF<Type_UB>() &&
            !(kernel.fg.builder->getOption(vISA_enablePreemption) &&
              dcl == kernel.fg.builder->getBuiltinR0())) {
          return true;
        }
      }
    }
  }

  return false;
}

// This function can be invoked before local RA or after augmentation.
void GlobalRA::evenAlign() {
  // Update alignment of all GRF declares to align
  for (auto dcl : kernel.Declares) {
    if (dcl->getRegFile() & G4_GRF) {
      if (evenAlignNeeded(dcl)) {
        setEvenAligned(dcl, true);
      }
    }
  }
}

void GlobalRA::getBankAlignment(LiveRange *lr, BankAlign &align) {
  G4_Declare *dcl = lr->getDcl();
  if (kernel.getSimdSize() < g4::SIMD16) {
    return;
  }

  if (dcl->getRegFile() & G4_GRF) {
    G4_Declare *topdcl = dcl->getRootDeclare();
    auto topdclBC = getBankConflict(topdcl);

    if (topdclBC != BANK_CONFLICT_NONE) {
      if (topdcl->getElemSize() >= 4 && topdcl->getNumRows() > 1 &&
          !(kernel.fg.builder->getOption(vISA_enablePreemption) &&
            dcl == kernel.fg.builder->getBuiltinR0())) {
        if (topdclBC == BANK_CONFLICT_SECOND_HALF_EVEN ||
            topdclBC == BANK_CONFLICT_SECOND_HALF_ODD) {
          align = BankAlign::Odd;
        }
      }
    }
  }
}

Augmentation::Augmentation(G4_Kernel &k, Interference &i,
                           const LivenessAnalysis &l,
                           const LiveRangeVec &ranges, GlobalRA &g)
    : kernel(k), intf(i), gra(g), liveAnalysis(l), lrs(ranges),
      fcallRetMap(g.fcallRetMap) {}

// For Scatter read, the channel is not handled as the block read.
// Update the emask according to the definition of VISA
bool Augmentation::updateDstMaskForGather(G4_INST *inst,
                                          std::vector<unsigned char> &mask) {
  G4_InstSend *sendInst = reinterpret_cast<G4_InstSend *>(inst);
  G4_SendDesc *msgDesc = sendInst->getMsgDesc();

  if (msgDesc->isRaw()) {
    return updateDstMaskForGatherRaw(
        inst, mask, reinterpret_cast<const G4_SendDescRaw *>(msgDesc));
  }
  vISA_ASSERT_UNREACHABLE("unexpected descriptor");
  return false;
}

static void updateMaskSIMT(unsigned char curEMBit, unsigned char execSize,
                           std::vector<unsigned char> &mask,
                           unsigned dataSizeBytes, unsigned vecElems) {
  unsigned blockSize = dataSizeBytes;
  unsigned blockNum = vecElems;
  for (unsigned i = 0; i < execSize; i++) {
    for (unsigned j = 0; j < blockNum; j++) {
      for (unsigned k = 0; k < blockSize; k++) {
        mask[(j * execSize + i) * blockSize + k] = curEMBit;
      }
    }
    if (curEMBit != NOMASK_BYTE) {
      curEMBit++;
      vISA_ASSERT(curEMBit <= 32, "Illegal mask channel");
    }
  }
}

bool Augmentation::updateDstMaskForGatherRaw(G4_INST *inst,
                                             std::vector<unsigned char> &mask,
                                             const G4_SendDescRaw *msgDesc) {
  unsigned char execSize = inst->getExecSize();
  const G4_DstRegRegion *dst = inst->getDst();
  unsigned char curEMBit = (unsigned char)inst->getMaskOffset();
  unsigned short elemSize = dst->getElemSize();

  if (inst->isWriteEnableInst() ||
      kernel.fg.builder->hasGatherReadSuppressionWARA()) {
    curEMBit = NOMASK_BYTE;
  }

  SFID funcID = msgDesc->getFuncId();

  switch (funcID) {
  case SFID::DP_DC1:
    switch (msgDesc->getHdcMessageType()) {
    case DC1_A64_SCATTERED_READ: // a64 scattered read: svm_gather
    {
      unsigned blockNum = msgDesc->getElemsPerAddr();
      unsigned blockSize = msgDesc->getElemSize();

      for (unsigned i = 0; i < execSize; i++) {
        for (unsigned j = 0; j < blockNum; j++) {
          for (unsigned k = 0; k < blockSize; k++) {
            mask[(j * execSize + i) * blockSize + k] = curEMBit;
          }
        }
        if (curEMBit != NOMASK_BYTE) {
          curEMBit++;
          vISA_ASSERT(curEMBit <= 32, "Illegal mask channel");
        }
      }
      return true;
    } break;

    case DC1_A64_UNTYPED_SURFACE_READ: // SVM gather 4
    case DC1_UNTYPED_SURFACE_READ:     // VISA gather 4
    case DC1_TYPED_SURFACE_READ:       // Gather 4 typed
    {
      unsigned channelNum = msgDesc->getEnabledChannelNum();
      if (channelNum == 0) {
        return false;
      }
      if (elemSize < 4) {
        elemSize = 4;
      }

      for (unsigned i = 0; i < channelNum; i++) {
        for (unsigned j = 0; j < execSize; j++) {
          for (unsigned k = 0; k < elemSize; k++) {
            mask[(i * execSize + j) * elemSize + k] = curEMBit;
          }
          if (curEMBit != NOMASK_BYTE) {
            curEMBit++;
            vISA_ASSERT(curEMBit <= 32, "Illegal mask channel");
          }
        }
        curEMBit = (unsigned char)inst->getMaskOffset();
      }
      return true;
    } break;

    default:
      return false;
    }
    break;
  case SFID::DP_DC2:
    switch (msgDesc->getHdcMessageType()) {
    case DC2_UNTYPED_SURFACE_READ:     // gather 4 scaled
    case DC2_A64_UNTYPED_SURFACE_READ: // SVM gather 4 scaled
    {
      unsigned channelNum = msgDesc->getEnabledChannelNum();
      if (channelNum == 0) {
        return false;
      }
      if (elemSize < 4) {
        elemSize = 4;
      }

      for (unsigned i = 0; i < channelNum; i++) {
        for (unsigned j = 0; j < execSize; j++) {
          for (unsigned k = 0; k < elemSize; k++) {
            mask[(i * execSize + j) * elemSize + k] = curEMBit;
          }
          if (curEMBit != NOMASK_BYTE) {
            curEMBit++;
            vISA_ASSERT(curEMBit <= 32, "Illegal mask channel");
          }
        }
        curEMBit = (unsigned char)inst->getMaskOffset();
      }
      return true;
    }

    case DC2_BYTE_SCATTERED_READ: // scaled byte scattered read: gather_scaled,
                                  // handled as block read write
    default:
      return false;
    }
    break;
  case SFID::DP_DC0:
    switch (msgDesc->getHdcMessageType()) {
    case DC_DWORD_SCATTERED_READ: // dword scattered read: gather(dword),
                                  // handled as block read write
    case DC_BYTE_SCATTERED_READ: // byte scattered read:   gather(byte), handled
                                 // as block read write
    default:
      return false;
    }
    break;

  case SFID::SAMPLER: {
    unsigned respLength = msgDesc->ResponseLength();
    if (respLength * kernel.numEltPerGRF<Type_UB>() !=
            dst->getTopDcl()->getByteSize() &&
        msgDesc->isFence()) {
      // since send dst size is not exactly equal to ResponseLength encoded in
      // the descriptor, conservatively treat the send as being non-default
      auto sz = dst->getTopDcl()->getByteSize();
      for (unsigned int i = 0; i != sz; ++i)
        mask[i] = NOMASK_BYTE;
      return true;
    }
    unsigned char curEMBit = (unsigned char)inst->getMaskOffset();
    elemSize = msgDesc->is16BitReturn() ? 2 : 4;
    unsigned warpNum =
        respLength * kernel.numEltPerGRF<Type_UB>() / (execSize * elemSize);
    if (inst->isWriteEnableInst()) {
      curEMBit = NOMASK_BYTE;
    }
    for (unsigned i = 0; i < warpNum; i++) {
      for (unsigned j = 0; j < execSize; j++) {
        for (unsigned k = 0; k < elemSize; k++) {
          mask[(i * execSize + j) * elemSize + k] = curEMBit;
        }
        if (curEMBit != NOMASK_BYTE) {
          curEMBit++;
          vISA_ASSERT(curEMBit <= 32, "Illegal mask channel");
        }
      }
      curEMBit = (unsigned char)inst->getMaskOffset();
    }
    return true;
  }

  break;

  case SFID::UGM:
  case SFID::UGML:
  case SFID::SLM: {
    uint32_t desc = msgDesc->getDesc();
    uint32_t op = (desc & 0x3F);                  // [5:0]
    uint32_t dszEncd = (desc >> 9) & 0x7;         // [11:9]
    bool isTranspose = ((desc >> 15) & 0x1) != 0; // [15]
    if (op == LSC_LOAD && !isTranspose) {         // transpose not supported yet
      int dataSzReg = 0;
      switch (dszEncd) { // dat size [11:9]
      case 0:
        dataSzReg = 1;
        break; // d8
      case 1:
        dataSzReg = 2;
        break; // d16
      default:
        dataSzReg = 4;
        break; // d32, d8u32, d16u32, d16u32h
      case 3:
        dataSzReg = 8;
        break; // d64
      }
      int vecSz = 0;
      int vecSzEncd = (desc >> 12) & 0x7; // [14:12]
      if (vecSzEncd <= 3) {
        vecSz = vecSzEncd + 1; // V1, V2, V3, V4
      } else {
        vecSz = 4 << (vecSzEncd - 3); // V8, V16, V32, V64
      }
      updateMaskSIMT(curEMBit, execSize, mask, (unsigned)dataSzReg,
                     (unsigned)vecSz);
      return true;
    }
  }
  default:
    return false;
  }

  return false;
}


// Value stored at each byte in mask determines which bits
// of EM enable that byte for writing. When checkCmodOnly
// is set dst is ignored and mask only for cmod is set. For
// flag declares, mask is at bit granularity rather than byte.
// Function updates mask field in declaration of correspoing
// variable - dst or cmod.
void Augmentation::updateDstMask(G4_INST *inst, bool checkCmodOnly) {
  G4_DstRegRegion *dst = inst->getDst();
  G4_CondMod *cmod = inst->getCondMod();

  if ((checkCmodOnly == false && dst && dst->getBase() &&
       dst->getBase()->isRegVar()) ||
      (checkCmodOnly == true && cmod != NULL && cmod->getBase() != NULL)) {
    int dclOffset = 0;
    G4_Declare *topdcl = NULL;

    if (checkCmodOnly == false) {
      topdcl = dst->getBase()->asRegVar()->getDeclare();
    } else {
      topdcl = cmod->asCondMod()->getTopDcl();
    }

    while (topdcl->getAliasDeclare() != nullptr) {
      dclOffset += topdcl->getAliasOffset();
      topdcl = topdcl->getAliasDeclare();
    }

    auto &mask = const_cast<std::vector<unsigned char> &>(gra.getMask(topdcl));

    unsigned size = topdcl->getByteSize();
    if (checkCmodOnly == true || dst->isFlag()) {
      size *= BITS_PER_BYTE;
    }

    if (mask.size() == 0) {
      mask.resize(size);
    }

    vISA_ASSERT(mask.size() > 0, "Valid mask not found for dcl %s",
                topdcl->getName());

    unsigned short hstride, elemSize;
    short row, subReg;
    unsigned startByte;

    if (checkCmodOnly == false) {
      hstride = dst->getHorzStride();

      row = dst->getRegOff();
      subReg = dst->getSubRegOff();
      elemSize = dst->getElemSize();

      if (inst->isSend() && !inst->isEOT()) {
        if (updateDstMaskForGather(inst, mask)) {
          return;
        }
      }

      if (dst->isFlag()) {
        elemSize = 1;
      }

      startByte = (row * kernel.getGRFSize()) + (subReg * elemSize);

      if (dst->isFlag()) {
        startByte = (row * 32) + (subReg * 8);
      }
    } else {
      hstride = 1;
      row = 0;
      elemSize = 1;
      startByte = cmod->asCondMod()->getLeftBound();
    }

    unsigned rb = 0xffffffff;

    if (checkCmodOnly == true) {
      rb = cmod->asCondMod()->getRightBound();
    } else {
      rb = dst->getRightBound();
    }

    unsigned char curEMBit = (unsigned char)inst->getMaskOffset();
    if (inst->isWriteEnableInst()) {
      curEMBit = NOMASK_BYTE;
    }

    for (unsigned i = dclOffset + startByte; i <= rb;
         i += (hstride * elemSize)) {
      for (int j = 0; j < elemSize; j++) {
        vISA_ASSERT(i + j < size,
                    "updateDstMask writing past end of mask array size: %d",
                    size);
        mask[i + j] |= curEMBit;
      }
      if (curEMBit != NOMASK_BYTE) {
        curEMBit++;
      }
    }
  }
}

unsigned Augmentation::getByteSizeFromMask(AugmentationMasks type) {
  if (type == AugmentationMasks::Default16Bit) {
    return 2;
  } else if (type == AugmentationMasks::Default32Bit) {
    return 4;
  } else if (type == AugmentationMasks::Default64Bit) {
    return 8;
  }

  vISA_ASSERT_UNREACHABLE("Unexpected type of mask");

  return 0;
}

bool Augmentation::isDefaultMaskDcl(G4_Declare *dcl, unsigned simdSize,
                                    AugmentationMasks type) {
  // default mask is one where dst's hstride is 1 and
  // elem size is 4
  bool isDefault = false;
  auto &mask = gra.getMask(dcl);

  unsigned byteSize = getByteSizeFromMask(type);

  // treat simd32 as simd16 as the instruction is always split to 2 simd16
  if (simdSize == 32) {
    simdSize = 16;
  }
  if (mask.size() > 0) {
    G4_Declare *topdcl = dcl->getRootDeclare();
    bool isFlagDcl = (topdcl->getRegFile() == G4_FLAG);

    unsigned size = topdcl->getByteSize();
    unsigned char curEMBit = 0;
    bool found = true;
    unsigned wrapAround = simdSize * byteSize;

    if (isFlagDcl == true) {
      size *= BITS_PER_BYTE;
      wrapAround = 16;
    }

    for (unsigned i = 0; i < size; i += 1) {
      if (isFlagDcl == true) {
        curEMBit++;
      } else {
        if (byteSize && i % byteSize == 0) {
          curEMBit++;
        }
      }

      if (i % wrapAround == 0) {
        // Wrap around based on simd size
        // For SIMD8 wrap around each row,
        // for SIMD16 wrap around every other row
        curEMBit = 0;
      }

      if (mask[i] != curEMBit &&
          // For flags, we set bytesize = 2 although
          // the kernel is SIMD8. This means higher 8
          // bits of mask will be set to 0 since those
          // bits are never defined. Such masks need
          // not be considered non-default.
          !(isFlagDcl == true && mask[i] == 0)) {
        found = false;
        break;
      }
    }

    if (found == true) {
      isDefault = true;
    }
  }

  return isDefault;
}

bool Augmentation::isDefaultMaskSubDeclare(unsigned char *mask, unsigned lb,
                                           unsigned rb, G4_Declare *dcl,
                                           unsigned simdSize) {
  bool isDefault = false;

  // treat simd32 as simd16 as the instruction is always split to 2 simd16
  if (simdSize == 32) {
    simdSize = 16;
  }

  if (mask != NULL) {
    unsigned size = dcl->getByteSize();
    unsigned char curEMBit = 0;
    bool found = true;
    unsigned wrapAround = simdSize * 4;
    unsigned leftBound = gra.getSubOffset(dcl);
    unsigned rightBound = leftBound + size - 1;

    vISA_ASSERT(rightBound <= rb, "Wrong sub declare right bound!");

    for (unsigned i = lb; i < rightBound + 1; i += 1) {
      if ((i - lb) % 4 == 0) {
        curEMBit++;
      }

      if ((i - lb) % wrapAround == 0) {
        curEMBit = 0;
      }

      if (i >= leftBound) {
        if (mask[i] != curEMBit) {
          found = false;
          break;
        }
      }
    }

    if (found == true) {
      isDefault = true;
    }
  }

  return isDefault;
}

bool Augmentation::verifyMaskIfInit(G4_Declare *dcl, AugmentationMasks mask) {
  // Return true if dcl mask is either undetermined or same as mask
  auto m = gra.getAugmentationMask(dcl);
  if (m == mask || m == AugmentationMasks::Undetermined) {
    return true;
  }

  return false;
}

bool Augmentation::checkGRFPattern2(G4_Declare *dcl, G4_DstRegRegion *dst,
                                    unsigned maskOff, unsigned lb, unsigned rb,
                                    unsigned execSize) {
  auto opndByteSize = dst->getTypeSize();
  unsigned modWith = opndByteSize * kernel.getSimdSize();
  if (lb % modWith - (maskOff * opndByteSize * dst->getHorzStride()) <=
      opndByteSize) {
    if ((lb +
         (execSize * opndByteSize * dst->getHorzStride() -
          dst->getHorzStride()) -
         rb) < opndByteSize) {
      if (opndByteSize == 2 &&
          verifyMaskIfInit(dcl, AugmentationMasks::Default32Bit)) {
        gra.setAugmentationMask(dcl, AugmentationMasks::Default32Bit);
        return true;
      } else if (opndByteSize == 4 &&
                 verifyMaskIfInit(dcl, AugmentationMasks::Default64Bit)) {
        gra.setAugmentationMask(dcl, AugmentationMasks::Default64Bit);
        return true;
      } else {
        gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
        return true;
      }
    }
  }

  return false;
}

// Returns true if dcl mask deemed to be non-default, false otherwise.
bool Augmentation::checkGRFPattern1(G4_Declare *dcl, G4_DstRegRegion *dst,
                                    unsigned maskOff, unsigned lb, unsigned rb,
                                    unsigned execSize) {
  auto opndByteSize = dst->getTypeSize();
  unsigned modWith = opndByteSize * kernel.getSimdSize();
  if (dst->getHorzStride() == 1) {
    if ((lb % modWith == (maskOff * opndByteSize) &&
         rb == (lb + (execSize * opndByteSize) - 1))) {
      // This will be taken only when hstride = 1
      if (opndByteSize == 2 &&
          verifyMaskIfInit(dcl, AugmentationMasks::Default16Bit)) {
        gra.setAugmentationMask(dcl, AugmentationMasks::Default16Bit);
        return true;
      } else if (opndByteSize == 4 &&
                 verifyMaskIfInit(dcl, AugmentationMasks::Default32Bit)) {
        gra.setAugmentationMask(dcl, AugmentationMasks::Default32Bit);
        return true;
      } else if (opndByteSize == 8 &&
                 verifyMaskIfInit(dcl, AugmentationMasks::Default64Bit)) {
        gra.setAugmentationMask(dcl, AugmentationMasks::Default64Bit);
        return true;
      } else {
        gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
        return true;
      }
    }
  }

  return false;
}

void Augmentation::markNonDefaultDstRgn(G4_INST *inst, G4_Operand *opnd) {
  if (inst->isPseudoKill()) {
    return;
  }

  G4_DstRegRegion *dst = nullptr;
  G4_CondMod *condMod = nullptr;
  if (opnd->isDstRegRegion()) {
    dst = opnd->asDstRegRegion();
  } else if (opnd->isCondMod()) {
    condMod = opnd->asCondMod();
  } else {
    vISA_ASSERT(false, "Don't know how to handle this type of operand");
  }

  // Handle condMod
  if (condMod && condMod->getBase()) {
    G4_Declare *dcl = condMod->getTopDcl();
    dcl = dcl->getRootDeclare();

    if (inst->isWriteEnableInst() ||
        opnd->getLeftBound() != inst->getMaskOffset()) {
      gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
      return;
    }

    if (verifyMaskIfInit(dcl, AugmentationMasks::DefaultPredicateMask)) {
      gra.setAugmentationMask(dcl, AugmentationMasks::DefaultPredicateMask);
    }
    return;
  }

  // Handle dst
  if (inst->isCall() || inst->isCallerSave()) {
    const G4_Declare *dcl = dst->getBase()->asRegVar()->getDeclare();
    if (dcl && liveAnalysis.livenessClass(dcl->getRegFile())) {
      gra.setAugmentationMask(dcl->getRootDeclare(),
                              AugmentationMasks::NonDefault);
    }
    return;
  }

  bool isFlagRA = liveAnalysis.livenessClass(G4_FLAG);
  if (dst && dst->getBase() && dst->getBase()->isRegVar()) {
    G4_Declare *dcl = dst->getBase()->asRegVar()->getDeclare();
    if (!liveAnalysis.livenessClass(dcl->getRegFile())) {
      return;
    }
    unsigned offTopDcl = 0;
    while (dcl->getAliasDeclare()) {
      offTopDcl += dcl->getAliasOffset();
      dcl = dcl->getAliasDeclare();
    }

    // NoMask instructions's dst is always non-default
    if (inst->isWriteEnableInst()) {
      gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
      return;
    }

    if (gra.getAugmentationMask(dcl) == AugmentationMasks::NonDefault)
      return;

    unsigned maskOff = inst->getMaskOffset();
    unsigned lb = dst->getLeftBound() + offTopDcl;
    unsigned rb = dst->getRightBound() + offTopDcl;
    unsigned execSize = inst->getExecSize();

    if (dcl->getAddressed()) {
      gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
      return;
    }

    if (!isFlagRA) {
      // Treat send as special case because update mask for scatter
      // has some special checks.
      if (inst->isSend()) {
        if (gra.getAugmentationMask(dcl) == AugmentationMasks::NonDefault) {
          return;
        }

        updateDstMask(inst, false);
        if (isDefaultMaskDcl(dcl, kernel.getSimdSize(),
                             AugmentationMasks::Default16Bit)) {
          gra.setAugmentationMask(dcl, AugmentationMasks::Default16Bit);
        } else if (isDefaultMaskDcl(dcl, kernel.getSimdSize(),
                                    AugmentationMasks::Default32Bit)) {
          gra.setAugmentationMask(dcl, AugmentationMasks::Default32Bit);
        } else if (isDefaultMaskDcl(dcl, kernel.getSimdSize(),
                                    AugmentationMasks::Default64Bit)) {
          bool useNonDefault = false;
          useNonDefault |=
              (kernel.getSimdSize() >= g4::SIMD16 && dcl->getTotalElems() > 8);
          useNonDefault |=
              (kernel.getSimdSize() == g4::SIMD8 && dcl->getTotalElems() > 4);

          if (useNonDefault) {
            gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
          } else {
            gra.setAugmentationMask(dcl, AugmentationMasks::Default64Bit);
          }
        } else {
          gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
          return;
        }
      } else {
        bool found = false;
        // default one
        found |= checkGRFPattern1(dcl, dst, maskOff, lb, rb, execSize);
        if (!found ||
            gra.getAugmentationMask(dcl) == AugmentationMasks::Undetermined) {
          // hstride = 2 case
          found |= checkGRFPattern2(dcl, dst, maskOff, lb, rb, execSize);
        }

        if (!found ||
            gra.getAugmentationMask(dcl) == AugmentationMasks::Undetermined) {
          gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
        }
      }
    } else {
      // Handle flag register as destination here
      if (!(lb == maskOff && rb == (lb + execSize - 1))) {
        gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
        return;
      }

      if (verifyMaskIfInit(dcl, AugmentationMasks::DefaultPredicateMask)) {
        gra.setAugmentationMask(dcl, AugmentationMasks::DefaultPredicateMask);
      }
    }
  }
}

// Returns true if any inst found using non-default mask.
// This function sets up lexical id of all instructions.
bool Augmentation::markNonDefaultMaskDef() {
  // Iterate dcls list and mark obvious ones as non-default.
  // Obvoius non-default is 1 element, ie uniform dcl.
  for (auto dcl : kernel.Declares) {
    auto dclRegFile = dcl->getRegFile();
    if (!liveAnalysis.livenessClass(dclRegFile))
      continue;

    if (dclRegFile == G4_GRF || dclRegFile == G4_INPUT ||
        dclRegFile == G4_ADDRESS) {
      if (dcl->getTotalElems() < 8 || dclRegFile == G4_INPUT) {
        gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
      }
    } else if (dclRegFile == G4_FLAG) {
      // Flags are processed when processing instructions
    }
  }

  unsigned id = 0;
  bool isFlagRA = liveAnalysis.livenessClass(G4_FLAG);

  for (auto bb : kernel.fg) {
    for (auto inst : *bb) {
      inst->setLexicalId(id++);

      G4_DstRegRegion *dst = inst->getDst();

      if (dst) {
        markNonDefaultDstRgn(inst, dst);
      }

      if (isFlagRA && inst->getCondMod()) {
        markNonDefaultDstRgn(inst, inst->getCondMod());
      }
    }
  }

  // Update whether each dcl is default/not
  AugmentationMasks prevAugMask = AugmentationMasks::Undetermined;
  bool nonDefaultMaskDefFound = false;

  for (auto dcl : kernel.Declares) {
    if (liveAnalysis.livenessClass(dcl->getRegFile())) {
      if (gra.getAugmentationMask(dcl) == AugmentationMasks::Undetermined) {
        gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
        nonDefaultMaskDefFound = true;
      }

      if (kernel.getOption(vISA_forceBCR) &&
          gra.getBankConflict(dcl) != BANK_CONFLICT_NONE) {
        gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
        nonDefaultMaskDefFound = true;
      }

      if (!nonDefaultMaskDefFound &&
          gra.getAugmentationMask(dcl) != prevAugMask &&
          prevAugMask != AugmentationMasks::Undetermined) {
        nonDefaultMaskDefFound = true;
      }

      prevAugMask = gra.getAugmentationMask(dcl);
    }

    bool checkLRAAlign = false;
    if (liveAnalysis.livenessClass(G4_GRF)) {
      if ((GlobalRA::useGenericAugAlign(kernel.getPlatformGeneration()) &&
           gra.evenAlignNeeded(dcl)))
        checkLRAAlign = true;
      else if (gra.getAugmentationMask(dcl) ==
                   AugmentationMasks::Default32Bit &&
               kernel.getSimdSize() > kernel.numEltPerGRF<Type_UD>())
        checkLRAAlign = true;
    }

    if (checkLRAAlign) {
      auto dclLR = gra.getLocalLR(dcl);
      if (dclLR) {
        int s;
        auto phyReg = dclLR->getPhyReg(s);
        if (phyReg && phyReg->asGreg()->getRegNum() % 2 != 0) {
          // If LRA assignment is not 2GRF aligned for then
          // mark it as non-default. GRA candidates cannot fully
          // overlap with such ranges. Partial overlap is illegal.
          gra.setAugmentationMask(dcl, AugmentationMasks::NonDefault);
          nonDefaultMaskDefFound = true;
        }
      }
    }
  }

  return nonDefaultMaskDefFound;
}

G4_BB *Augmentation::getTopmostBBDst(G4_BB *src, G4_BB *end, G4_BB *origSrc,
                                     unsigned traversal) {
  // Start from src BB and do a DFS. If any back-edges
  // are found then recursively invoke itself with dst
  // of back-edge. Any path that reaches BB "end"
  // will not be propagated forward.
  unsigned topLexId = src->front()->getLexicalId();
  G4_BB *topmostBB = src;

  if (src != end) {
    src->markTraversed(traversal);
    src->setNestLevel();

    for (G4_BB *succ : src->Succs) {
      if (succ == origSrc) {
        // Src of traversal traversed again without
        // ever traversing end node. So abort this path.
        return nullptr;
      }

      if (succ->isAlreadyTraversed(traversal) == true)
        continue;

      G4_BB *recursiveTopMostBB =
          getTopmostBBDst(succ, end, origSrc, traversal);

      if (recursiveTopMostBB != NULL) {
        unsigned recursiveTopMostBBLexId =
            recursiveTopMostBB->front()->getLexicalId();

        if (recursiveTopMostBBLexId < topLexId) {
          topmostBB = recursiveTopMostBB;
          topLexId = recursiveTopMostBBLexId;
        }
      } else {
        if (src != origSrc) {
          topmostBB = NULL;
          topLexId = 0;
        }
      }

      succ->markTraversed(traversal);
      succ->setNestLevel();
    }
  }

  return topmostBB;
}

void Augmentation::updateStartIntervalForSubDcl(G4_Declare *dcl,
                                                G4_INST *curInst,
                                                G4_Operand *opnd) {
  for (const G4_Declare *subDcl : gra.getSubDclList(dcl)) {
    unsigned leftBound = gra.getSubOffset(subDcl);
    unsigned rightBound = leftBound + subDcl->getByteSize() - 1;
    if (!(opnd->getRightBound() < leftBound ||
          rightBound < opnd->getLeftBound())) {
      auto subDclStartInterval = gra.getStartInterval(subDcl);
      if (subDclStartInterval == NULL ||
          (subDclStartInterval->getLexicalId() > curInst->getLexicalId())) {
        gra.setStartInterval(subDcl, curInst);
      }

      auto subDclEndIntrval = gra.getEndInterval(subDcl);
      if (subDclEndIntrval == NULL ||
          (subDclEndIntrval->getLexicalId() < curInst->getLexicalId())) {
        gra.setEndInterval(subDcl, curInst);
      }
    }
  }

  return;
}

void Augmentation::updateEndIntervalForSubDcl(G4_Declare *dcl, G4_INST *curInst,
                                              G4_Operand *opnd) {
  for (const G4_Declare *subDcl : gra.getSubDclList(dcl)) {
    unsigned leftBound = gra.getSubOffset(subDcl);
    unsigned rightBound = leftBound + subDcl->getByteSize() - 1;
    if (!(opnd->getRightBound() < leftBound ||
          rightBound < opnd->getLeftBound())) {
      auto subDclEndInterval = gra.getEndInterval(subDcl);
      if (subDclEndInterval == NULL ||
          (subDclEndInterval->getLexicalId() < curInst->getLexicalId())) {
        gra.setEndInterval(subDcl, curInst);
      }

      auto subDclStartInterval = gra.getStartInterval(subDcl);
      if (subDclStartInterval == NULL ||
          (subDclStartInterval->getLexicalId() > curInst->getLexicalId())) {
        gra.setStartInterval(subDcl, curInst);
      }
    }
  }

  return;
}

void Augmentation::updateStartInterval(const G4_Declare *dcl,
                                       G4_INST *curInst) {
  auto dclStartInterval = gra.getStartInterval(dcl);
  if (dclStartInterval == NULL ||
      (dclStartInterval->getLexicalId() > curInst->getLexicalId())) {
    gra.setStartInterval(dcl, curInst);
  }

  auto dclEndInterval = gra.getEndInterval(dcl);
  if (dclEndInterval == NULL ||
      (dclEndInterval->getLexicalId() < curInst->getLexicalId())) {
    gra.setEndInterval(dcl, curInst);
  }
}

void Augmentation::updateEndInterval(const G4_Declare *dcl, G4_INST *curInst) {
  auto dclEndInterval = gra.getEndInterval(dcl);
  if (dclEndInterval == NULL ||
      (dclEndInterval->getLexicalId() < curInst->getLexicalId())) {
    gra.setEndInterval(dcl, curInst);
  }

  auto dclStartInterval = gra.getStartInterval(dcl);
  if (dclStartInterval == NULL ||
      (dclStartInterval->getLexicalId() > curInst->getLexicalId())) {
    gra.setStartInterval(dcl, curInst);
  }
}

void Augmentation::updateStartIntervalForLocal(G4_Declare *dcl,
                                               G4_INST *curInst,
                                               G4_Operand *opnd) {
  updateStartInterval(dcl, curInst);
  if (dcl->getIsSplittedDcl()) {
    updateStartIntervalForSubDcl(dcl, curInst, opnd);
  }
}

void Augmentation::updateEndIntervalForLocal(G4_Declare *dcl, G4_INST *curInst,
                                             G4_Operand *opnd) {
  updateEndInterval(dcl, curInst);
  if (dcl->getIsSplittedDcl()) {
    updateEndIntervalForSubDcl(dcl, curInst, opnd);
  }
}

void GlobalRA::printLiveIntervals() {
  for (const G4_Declare *dcl : kernel.Declares) {
    if (getStartInterval(dcl) != nullptr || getEndInterval(dcl) != nullptr) {
      DEBUG_VERBOSE(dcl->getName() << " (");

      if (getStartInterval(dcl) != nullptr) {
        DEBUG_VERBOSE(getStartInterval(dcl)->getLexicalId());
      } else {
        DEBUG_VERBOSE("*");
      }

      DEBUG_VERBOSE(", ");

      if (getEndInterval(dcl) != nullptr) {
        DEBUG_VERBOSE(getEndInterval(dcl)->getLexicalId());
      } else {
        DEBUG_VERBOSE("*");
      }

      DEBUG_VERBOSE("] "
                    << "\n");
    }
  }
}

void Augmentation::buildLiveIntervals() {
  // Treat variables live-in to program first
  G4_BB *entryBB = kernel.fg.getEntryBB();

  // Live-in variables have their start interval start with
  // first instruction of entry BB
  for (auto i : liveAnalysis.globalVars) {
    if (liveAnalysis.isLiveAtEntry(entryBB, i)) {
      const G4_Declare *dcl = lrs[i]->getDcl()->getRootDeclare();

      updateStartInterval(dcl, entryBB->front());
    }
  }

  unsigned funcCnt = 0;

  for (G4_BB *curBB : kernel.fg) {
    for (G4_INST *inst : *curBB) {
      if (inst->isPseudoKill() == true) {
        continue;
      }

      G4_DstRegRegion *dst = inst->getDst();

      if (inst->isCall()) {
        const char *name =
            kernel.fg.builder->getNameString(32, "SCALL_%d", funcCnt++);
        G4_Declare *scallDcl =
            kernel.fg.builder->createDeclare(name, G4_GRF, 1, 1, Type_UD);
        gra.addVarToRA(scallDcl);

        updateStartInterval(scallDcl, inst);
        updateEndInterval(scallDcl, inst);

        std::pair<G4_INST *, G4_BB*> callInfo(inst, curBB);
        callDclMap.emplace(scallDcl, callInfo);

        continue;
      }

      if (dst && dst->getRegAccess() == Direct && dst->getBase()) {
        // Destination
        G4_Declare *defdcl = GetTopDclFromRegRegion(dst);

        if (dst->getBase()->isRegAllocPartaker()) {
          if (defdcl && gra.getLocalLR(defdcl)) {
            updateStartIntervalForLocal(defdcl, inst, dst);
          } else {
            updateStartInterval(defdcl, inst);
          }
        } else if (liveAnalysis.livenessClass(G4_GRF)) {
          LocalLiveRange *defdclLR;

          // Handle ranges allocated by local RA
          if (defdcl && (defdclLR = gra.getLocalLR(defdcl)) &&
              defdclLR->getAssigned() == true && !defdclLR->isEOT()) {
            updateStartInterval(defdcl, inst);
          }
        }
      } else if (liveAnalysis.livenessClass(G4_ADDRESS) && dst &&
                 dst->getRegAccess() == IndirGRF && dst->getBase() &&
                 dst->getBase()->isRegVar()) {
        // Destination is indirect
        G4_Declare *defdcl = dst->getBaseRegVarRootDeclare();

        updateEndInterval(defdcl, inst);
      } else if (liveAnalysis.livenessClass(G4_GRF) && dst &&
                 dst->isIndirect()) {
        const REGVAR_VECTOR &pointsToSet =
            liveAnalysis.getPointsToAnalysis().getAllInPointsToOrIndrUse(dst,
                                                                         curBB);
        for (auto pointsToVar : pointsToSet) {
          if (pointsToVar.var->isRegAllocPartaker()) {
            updateStartInterval(pointsToVar.var->getDeclare()->getRootDeclare(),
                                inst);
          }
        }
      }

      if (liveAnalysis.livenessClass(G4_FLAG)) {
        G4_CondMod *cmod = inst->getCondMod();

        if (cmod != nullptr && cmod->getBase() != nullptr) {
          // Conditional modifier
          G4_Declare *dcl = cmod->getBaseRegVarRootDeclare();

          updateStartInterval(dcl, inst);
        }
      }

      for (unsigned i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
        G4_Operand *src = inst->getSrc(i);
        if (!src || !src->isSrcRegRegion()) {
          continue;
        }
        G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();

        if (srcRegion->getRegAccess() == Direct && srcRegion->getBase()) {
          G4_Declare *usedcl = GetTopDclFromRegRegion(src);

          if (srcRegion->getBase()->isRegAllocPartaker()) {
            if (gra.getLocalLR(usedcl)) {
              updateEndIntervalForLocal(usedcl, inst, src);
            } else {
              updateEndInterval(usedcl, inst);
            }
          } else if (liveAnalysis.livenessClass(G4_GRF)) {
            LocalLiveRange *usedclLR = nullptr;
            if (usedcl && (usedclLR = gra.getLocalLR(usedcl)) &&
                usedclLR->getAssigned() == true && !usedclLR->isEOT()) {
              updateEndInterval(usedcl, inst);
            }
          }
        } else if (liveAnalysis.livenessClass(G4_GRF) &&
                   srcRegion->isIndirect()) {
          const REGVAR_VECTOR &pointsToSet =
              liveAnalysis.getPointsToAnalysis().getAllInPointsToOrIndrUse(
                  srcRegion, curBB);
          for (auto pointsToVar : pointsToSet) {
            if (pointsToVar.var->isRegAllocPartaker()) {
              updateEndInterval(pointsToVar.var->getDeclare()->getRootDeclare(),
                                inst);
            }
          }
        } else if (liveAnalysis.livenessClass(G4_ADDRESS) &&
                   srcRegion->getRegAccess() == IndirGRF &&
                   srcRegion->getBase() && srcRegion->getBase()->isRegVar()) {
          G4_Declare *usedcl = src->getBaseRegVarRootDeclare();

          updateEndInterval(usedcl, inst);
        }
      }

      if (liveAnalysis.livenessClass(G4_FLAG)) {
        G4_Predicate *pred = inst->getPredicate();

        if (pred != NULL) {
          // Predicate
          G4_Declare *dcl = pred->getBaseRegVarRootDeclare();

          updateEndInterval(dcl, inst);
        }
      }
    }
  }

  // extend all variables that are live at bb entry to the given inst
  // ToDo: this seems very slow when # variable is large, should look for sparse
  // implementation
  auto extendVarLiveness = [this](G4_BB *bb, G4_INST *inst) {
    for (auto i : liveAnalysis.globalVars) {
      if (liveAnalysis.isLiveAtEntry(bb, i) == true &&
          !kernel.fg.isPseudoDcl(lrs[i]->getDcl())) {
        // Extend ith live-interval
        G4_Declare *dcl = lrs[i]->getDcl()->getRootDeclare();
        updateStartInterval(dcl, inst);
        VISA_DEBUG_VERBOSE({
          unsigned oldStart = gra.getStartInterval(dcl)->getLexicalId();
          if (oldStart > gra.getStartInterval(dcl)->getLexicalId()) {
            std::cout << "Extending " << dcl->getName() << " from old start "
                      << oldStart << " to "
                      << gra.getStartInterval(dcl)->getLexicalId()
                      << " due to back-edge"
                      << "\n";
          }
        });
      }
    }
  };

  if (!kernel.fg.isReducible()) {
    // use SCC instead
    // FIXME: does augmentation work in the presence of subroutine? neither
    // SCCAnalysis nor findNaturalLoops considers the call graph
    SCCAnalysis SCCFinder(kernel.fg);
    SCCFinder.run();
    for (auto iter = SCCFinder.SCC_begin(), iterEnd = SCCFinder.SCC_end();
         iter != iterEnd; ++iter) {
      auto &&anSCC = *iter;
      std::unordered_set<G4_BB *> SCCSucc; // any successor BB of the SCC
      G4_BB *headBB = anSCC.getEarliestBB();
      for (auto BI = anSCC.body_begin(), BIEnd = anSCC.body_end(); BI != BIEnd;
           ++BI) {
        G4_BB *bb = *BI;
        for (auto succ : bb->Succs) {
          if (!anSCC.isMember(succ)) {
            SCCSucc.insert(succ);
          }
        }
      }
      for (auto exitBB : SCCSucc) {
        extendVarLiveness(exitBB, headBB->front());
      }
    }
  } else {
    // process each natural loop
    for (auto &&iter : kernel.fg.naturalLoops) {
      auto &&backEdge = iter.first;
      G4_INST *startInst = (backEdge.second)->front();
      const std::set<G4_BB *> &loopBody = iter.second;

      for (auto block : loopBody) {
        // FIXME: this may process a BB multiple times
        for (auto succBB : block->Succs) {
          // A subroutine call BB's successor is callee's INIT BB.
          // Loop data structure doesn't include callee BB. So
          // succBB not part of loop may still be INIT BB of callee.
          // Such an INIT BB shouldn't be treated as a loop exit
          // for live-range extension. If we don't check for INIT BB
          // we end up extending RET__loc range to loop header
          // which isn't correct.
          if (loopBody.find(succBB) == loopBody.end() &&
              (succBB->getBBType() & G4_BB_INIT_TYPE) == 0) {
            G4_BB *exitBB = succBB;

            unsigned latchBBId = (backEdge.first)->getId();
            unsigned exitBBId = succBB->getId();
            if (exitBBId < latchBBId && succBB->Succs.size() == 1) {
              exitBB = succBB->Succs.front();
            }
            VISA_DEBUG_VERBOSE({
              std::cout << "==> Extend live-in for BB" << exitBB->getId()
                        << "\n";
              exitBB->emit(std::cout);
            });
            extendVarLiveness(exitBB, startInst);
          }
        }
      }

      G4_BB *startBB = backEdge.second;
      G4_BB *EndBB = backEdge.first;

      for (auto i : liveAnalysis.globalVars) {
        if (liveAnalysis.isLiveAtEntry(startBB, i) == true &&
            liveAnalysis.isLiveAtExit(EndBB, i) == true) {
          const G4_Declare *dcl = lrs[i]->getDcl()->getRootDeclare();
          unsigned oldEnd = gra.getEndInterval(dcl)->getLexicalId();
          (void)oldEnd;
          updateEndInterval(dcl, EndBB->back());
          VISA_DEBUG_VERBOSE({
            if (oldEnd < gra.getEndInterval(dcl)->getLexicalId()) {
              std::cout << "Extending " << dcl->getName() << " from old end "
                        << oldEnd << " to "
                        << gra.getEndInterval(dcl)->getLexicalId()
                        << " due to back-edge"
                        << "\n";
            }
          });
        }
      }
    }
  }

#ifdef DEBUG_VERBOSE_ON
  // Print calculated live-ranges
  gra.printLiveIntervals();
#endif
}

Augmentation::~Augmentation() {
  // Clear out calculated information so that subsequent RA
  // iterations don't have stale information
  for (DECLARE_LIST_ITER dcl_it = kernel.Declares.begin(),
                         end = kernel.Declares.end();
       dcl_it != end; dcl_it++) {
    gra.setStartInterval(*dcl_it, nullptr);
    gra.setEndInterval(*dcl_it, nullptr);
    gra.setMask(*dcl_it, {});
    gra.setAugmentationMask(*dcl_it, AugmentationMasks::Undetermined);
  }
}

class compareInterval {
public:
  GlobalRA &gra;

  compareInterval(GlobalRA &g) : gra(g) {}

  bool operator()(G4_Declare *dcl1, G4_Declare *dcl2) {
    return gra.getStartInterval(dcl1)->getLexicalId() <
           gra.getStartInterval(dcl2)->getLexicalId();
  }
};

void Augmentation::sortLiveIntervals() {
  // Sort all intervals in kernel based on their starting point in
  // ascending order and return them in sortedIntervals vector
  // This is actually more efficient (at least according to vTune) than the O(N)
  // bucket sort algorithm below, since it avoids most of the malloc/free
  // overhead from the vector.resize()
  for (G4_Declare *dcl : kernel.Declares) {
    if (gra.getStartInterval(dcl) != NULL) {
      sortedIntervals.push_back(dcl);
    }
  }

  std::sort(sortedIntervals.begin(), sortedIntervals.end(),
            compareInterval(gra));

  VISA_DEBUG_VERBOSE({
    std::cout << "Live-intervals in sorted order:\n";
    for (const G4_Declare *dcl : sortedIntervals) {
      std::cout << dcl->getName() << " - "
                << "(" << gra.getStartInterval(dcl)->getLexicalId() << ", "
                << gra.getEndInterval(dcl)->getLexicalId() << "]"
                << "\n";
    }
  });
}

unsigned Augmentation::getEnd(const G4_Declare *dcl) const {
  return gra.getEndInterval(dcl)->getLexicalId();
}

// Mark interference between dcls. Either one of dcls may have
// register assigned by local RA so handle those cases too.
// Re-entrant function.
void Augmentation::handleSIMDIntf(G4_Declare *firstDcl, G4_Declare *secondDcl,
                                  bool isCall) {
  auto markIntfWithLRAAssignment = [](const G4_Declare *firstDcl,
                                      const G4_Declare *lraAssigned,
                                      Interference &intf) {
    unsigned numRows = lraAssigned->getNumRows();
    const G4_VarBase *preg = lraAssigned->getRegVar()->getPhyReg();
    vISA_ASSERT(preg->isGreg(),
                "Expecting a physical register during building interference "
                "among incompatible masks");
    unsigned start = preg->asGreg()->getRegNum();

    for (unsigned i = start; i < (start + numRows); i++) {
      auto GRFDcl = intf.getGRFDclForHRA(i);
      intf.checkAndSetIntf(firstDcl->getRegVar()->getId(),
                           GRFDcl->getRegVar()->getId());
      VISA_DEBUG_VERBOSE(std::cout << "Marking interference between "
                                   << firstDcl->getName() << " and "
                                   << GRFDcl->getName() << "\n");
    }
  };

  if (firstDcl->getRegFile() == G4_INPUT &&
      firstDcl->getRegVar()->getPhyReg() != NULL &&
      secondDcl->getRegFile() == G4_INPUT &&
      secondDcl->getRegVar()->getPhyReg() != NULL) {
    return;
  }

  auto contain = [](const auto &C, auto pred) {
    return std::find_if(C.cbegin(), C.cend(), pred) != C.cend();
  };

  bool isFirstDcl = true;

  auto pred = [firstDcl, secondDcl, &isFirstDcl](const auto &el) {
    if (el.second.VCA == firstDcl)
      return true;
    if (el.second.VCA == secondDcl) {
      isFirstDcl = false;
      return true;
    }
    return false;
  };

  if (contain(kernel.fg.fcallToPseudoDclMap, pred)) {
    // Mark intf for following pattern:
    // V33 =
    // ...
    // if
    //     = V33
    //     fcall
    // ...
    // else
    //     = V33
    // endif
    //
    // V33 will interfere with VCA_SAVE pseudo node.
    // It also needs to interfere with retval to
    // ensure V33 and retval don't get same allocation.
    // Note that if V33 is actually live after fcall
    // then graph coloring will do this for us. In this
    // case however we need to rely on augmentation.
    FCALL_RET_MAP_ITER retIter =
        isFirstDcl ? fcallRetMap.find(firstDcl) : fcallRetMap.find(secondDcl);
    if (retIter != fcallRetMap.end()) {
      G4_Declare *retVar = retIter->second;
      LocalLiveRange *otherDclLR;
      G4_Declare *otherDcl = isFirstDcl ? secondDcl : firstDcl;
      if (otherDcl->getRegVar()->isRegAllocPartaker())
        intf.checkAndSetIntf(otherDcl->getRegVar()->getId(),
                             retVar->getRegVar()->getId());
      else if ((otherDclLR = gra.getLocalLR(otherDcl)) &&
               otherDclLR->getAssigned() && !otherDclLR->isEOT()) {
        markIntfWithLRAAssignment(retVar, otherDcl, intf);
      }
    }
  }

  if (firstDcl->getRegVar()->isRegAllocPartaker() &&
      secondDcl->getRegVar()->isRegAllocPartaker()) {
    if (!intf.varSplitCheckBeforeIntf(firstDcl->getRegVar()->getId(),
                                      secondDcl->getRegVar()->getId())) {
      intf.checkAndSetIntf(firstDcl->getRegVar()->getId(),
                           secondDcl->getRegVar()->getId());
      if (isCall) {
        intf.buildInterferenceWithAllSubDcl(firstDcl->getRegVar()->getId(),
                                            secondDcl->getRegVar()->getId());
      }
      VISA_DEBUG_VERBOSE(std::cout << "Marking interference between "
                                   << firstDcl->getName() << " and "
                                   << secondDcl->getName() << "\n");
    }
  } else if (liveAnalysis.livenessClass(G4_GRF)) {
    LocalLiveRange *secondDclLR = nullptr, *firstDclLR = nullptr;

    if (firstDcl->getRegVar()->isRegAllocPartaker() &&
        (secondDclLR = gra.getLocalLR(secondDcl)) &&
        secondDclLR->getAssigned() && !secondDclLR->isEOT()) {
      // secondDcl was assigned by local RA and it uses
      markIntfWithLRAAssignment(firstDcl, secondDcl, intf);
    } else if (secondDcl->getRegVar()->isRegAllocPartaker() &&
               (firstDclLR = gra.getLocalLR(firstDcl)) &&
               firstDclLR->getAssigned() && !firstDclLR->isEOT()) {
      // Call self with reversed parameters instead of re-implementing
      // above code
      handleSIMDIntf(secondDcl, firstDcl, isCall);
    }
  }
}

bool Augmentation::isNoMask(const G4_Declare *dcl, unsigned size) const {
  auto &mask = gra.getMask(dcl);
  bool result = false;

  if (mask.size() > 0) {
    result = true;

    for (unsigned i = 0; i < size; i++) {
      if (mask[i] != NOMASK_BYTE) {
        result = false;
      }
    }
  }

  return result;
}

bool Augmentation::isConsecutiveBits(const G4_Declare *dcl,
                                     unsigned size) const {
  auto &mask = gra.getMask(dcl);
  bool result = false;

  if (mask.size() > 0) {
    result = true;

    for (unsigned i = 0; i < size; i++) {
      if (mask[i] != i) {
        result = false;
      }
    }
  }

  return result;
}

bool Augmentation::isCompatible(const G4_Declare *testDcl,
                                const G4_Declare *biggerDcl) const {
  bool compatible = false;

  unsigned testSize = testDcl->getRegVar()->isFlag()
                          ? testDcl->getNumberFlagElements()
                          : testDcl->getByteSize();
  unsigned biggerSize = biggerDcl->getRegVar()->isFlag()
                            ? biggerDcl->getNumberFlagElements()
                            : biggerDcl->getByteSize();
  unsigned size = (testSize < biggerSize ? testSize : biggerSize);

  // Masks are compatible when:
  // i. Both decls have exactly 1 EM bit defining each byte
  //  (This means a dcl with Q1 in one inst and Q2 in another
  //   instruction writing same subregisters is not a candidate
  //   for next step).
  // ii. Bytes at common indices are enabled by same EM bit
  //  (This means NoMask dcl is compatible with NoMask dcl and
  //   not with any other dcl).
  // UPDATE: (ii) above is now altered such that NoMask dcls
  // that overlap are considered to be incompatible. This is to
  // handle removal of JIP edges (then->else edge).

  auto &testMask = gra.getMask(testDcl);
  auto &biggerMask = gra.getMask(biggerDcl);

  if (testMask.size() > 0 && biggerMask.size() > 0) {
    // Lets pattern match
    if (testDcl->getRegFile() == G4_FLAG) {
      if (isConsecutiveBits(testDcl, size) &&
          isConsecutiveBits(biggerDcl, size)) {
        compatible = true;
      }
    } else {
      // Add another pattern to check here
    }
  }

  return compatible;
}

void Augmentation::expireIntervals(unsigned startIdx) {
  // Expire elements from both lists
  while (defaultMaskQueue.size() > 0) {
    if (gra.getEndInterval(defaultMaskQueue.top())->getLexicalId() <=
        startIdx) {
      VISA_DEBUG_VERBOSE(std::cout << "Expiring "
                                   << defaultMaskQueue.top()->getName()
                                   << "\n");
      defaultMaskQueue.pop();
    } else {
      break;
    }
  }

  while (nonDefaultMaskQueue.size() > 0) {
    if (gra.getEndInterval(nonDefaultMaskQueue.top())->getLexicalId() <=
        startIdx) {
      VISA_DEBUG_VERBOSE(std::cout << "Expiring "
                                   << nonDefaultMaskQueue.top()->getName()
                                   << "\n");
      nonDefaultMaskQueue.pop();
    } else {
      break;
    }
  }
}

// Return true if edge between dcl1 and dcl2 is strong.
bool Interference::isStrongEdgeBetween(const G4_Declare *dcl1,
                                       const G4_Declare *dcl2) const {
  auto dcl1RegVar = dcl1->getRegVar();
  auto dcl2RegVar = dcl2->getRegVar();
  auto dcl1RAPartaker = dcl1RegVar->isRegAllocPartaker();
  auto dcl2RAPartaker = dcl2RegVar->isRegAllocPartaker();

  if (dcl1RAPartaker && dcl2RAPartaker) {
    if (interfereBetween(dcl1RegVar->getId(), dcl2RegVar->getId())) {
      return true;
    } else {
      return false;
    }
  }

  if (dcl1RAPartaker) {
    auto dcl2NumRows = dcl2->getNumRows();
    auto startPhyReg = dcl2RegVar->getPhyReg()->asGreg()->getRegNum();
    auto dcl2LR = gra.getLocalLR(dcl2);

    if (dcl2LR && dcl2LR->getAssigned()) {
      bool allEdgesStrong = true;
      for (unsigned i = startPhyReg; i < (startPhyReg + dcl2NumRows); i++) {
        const G4_Declare *lraPreg = getGRFDclForHRA(i);
        allEdgesStrong &= interfereBetween(lraPreg->getRegVar()->getId(),
                                           dcl1RegVar->getId());
      }

      if (allEdgesStrong)
        return true;
    }
  } else {
    return isStrongEdgeBetween(dcl2, dcl1);
  }

  return false;
}

bool Augmentation::weakEdgeNeeded(AugmentationMasks defaultDclMask,
                                  AugmentationMasks newDclMask) {
  if (GlobalRA::useGenericAugAlign(kernel.getPlatformGeneration())) {
    // Weak edge needed in case #GRF exceeds 2
    if (newDclMask == AugmentationMasks::Default64Bit)
      return (TypeSize(Type_Q) * kernel.getSimdSizeWithSlicing()) >
             (unsigned)(2 * kernel.numEltPerGRF<Type_UB>());

    if (newDclMask == AugmentationMasks::Default32Bit) {
      // Even align up to 2 GRFs size variable, use weak edges beyond
      return (TypeSize(Type_D) * kernel.getSimdSizeWithSlicing()) >
             (unsigned)(2 * kernel.numEltPerGRF<Type_UB>());
    }
  } else {
    return (defaultDclMask == AugmentationMasks::Default64Bit &&
            newDclMask == AugmentationMasks::Default64Bit);
  }

  return false;
}

// This method is invoked when building SIMD intf and current variable
// is the artificial variable created to model call. Live-intervals in
// default set and non-default set are ones that overlap with call site
// at end of callBB. The idea here is to mark every such active interval
// with mask associated with func. Later, we'll mark interference with
// each live-interval bit set here and maydef of func.
void Augmentation::addSIMDIntfDclForCallSite(G4_BB* callBB) {
  FuncInfo *func = callBB->getCalleeInfo();
  auto isLiveThroughFunc = [&](unsigned int id) {
    if (liveAnalysis.isLiveAtExit(callBB, id)) {
      auto retBB = func->getExitBB();
      if (liveAnalysis.isLiveAtExit(retBB, id))
        return true;
    }
    return false;
  };

  auto& overlapDeclares = overlapDclsWithFunc[func];
  for (auto defaultDcl : defaultMaskQueue) {
    auto id = defaultDcl->getRegVar()->getId();
    if (!isLiveThroughFunc(id))
      overlapDeclares.first.set(id);
  }

  for (auto nonDefaultDcl : nonDefaultMaskQueue) {
    auto id = nonDefaultDcl->getRegVar()->getId();
    if (!isLiveThroughFunc(id))
      overlapDeclares.second.set(id);
  }
}

void Augmentation::addSIMDIntfForRetDclares(G4_Declare *newDcl) {
  auto dclIt = retDeclares.find(newDcl);
  MaskDeclares *mask = nullptr;
  if (dclIt == retDeclares.end()) {
    MaskDeclares newMask;
    retDeclares[newDcl] = std::move(newMask);
    mask = &retDeclares[newDcl];
  } else {
    mask = &dclIt->second;
  }

  for (auto defaultDcl : defaultMaskQueue) {
    mask->first.set(defaultDcl->getRegVar()->getId());
  }

  for (auto nonDefaultDcl : nonDefaultMaskQueue) {
    mask->second.set(nonDefaultDcl->getRegVar()->getId());
  }
}

//
// Mark interference between newDcl and other incompatible dcls in current
// active lists.
//
void Augmentation::buildSIMDIntfDcl(G4_Declare *newDcl, bool isCall) {
  auto newDclAugMask = gra.getAugmentationMask(newDcl);

  for (auto defaultDcl : defaultMaskQueue) {
    if (gra.getAugmentationMask(defaultDcl) != newDclAugMask) {
      handleSIMDIntf(defaultDcl, newDcl, isCall);
    } else {
      if (liveAnalysis.livenessClass(G4_GRF) &&
          // Populate compatible sparse intf data structure
          // only for weak edges.
          weakEdgeNeeded(gra.getAugmentationMask(defaultDcl), newDclAugMask)) {
        if (defaultDcl->getRegVar()->isPhyRegAssigned() &&
            newDcl->getRegVar()->isPhyRegAssigned()) {
          continue;
        }

        if (intf.isStrongEdgeBetween(defaultDcl, newDcl)) {
          // No need to add weak edge
          continue;
        }

        // defaultDcl and newDcl are compatible live-ranges and can have weak
        // edge in intf graph
        auto it = intf.compatibleSparseIntf.find(defaultDcl);
        if (it != intf.compatibleSparseIntf.end()) {
          it->second.push_back(newDcl);
        } else {
          std::vector<G4_Declare *> v(1, newDcl);
          intf.compatibleSparseIntf.insert(std::make_pair(defaultDcl, v));
        }

        it = intf.compatibleSparseIntf.find(newDcl);
        if (it != intf.compatibleSparseIntf.end()) {
          it->second.push_back(defaultDcl);
        } else {
          std::vector<G4_Declare *> v(1, defaultDcl);
          intf.compatibleSparseIntf.insert(std::make_pair(newDcl, v));
        }
      }
    }
  }

  // Mark interference among non-default mask variables
  for (auto nonDefaultDcl : nonDefaultMaskQueue) {

    auto isAugNeeded = [&]() {
      if (newDclAugMask != AugmentationMasks::NonDefault)
        return true;

      // Skip augmentation check if both dcls are infinite spill cost tmp dcls
      // generated by RA. Such dcls have their interference correctly computed
      // by conventional interference computation. In case of address taken
      // spill/fill dcls, applying augmentation on them causes unexpected
      // interference edges.
      //
      // Unexpected intf shows up because we reuse dcl for address taken
      // spill/fill across BBs. As per generated code, such address taken
      // spill/fill dcl ranges are live only around the indirect operand. Also,
      // these ranges are never live across BBs. As augmentation models
      // live-intervals without holes, it ends up with unnecessary
      // interferences. Here is such an example of unnecessary interference
      // edge:
      //
      // BB1:
      // A0 = &ADDR_SP_FL_1 + offset
      // (W) Fill ADDR_SP_FL_1
      // r[A0] = ...
      // (W) Spill ADD_SP_FL_1
      //
      // BB2:
      // (W) Fill FL_V10
      //      = FL_V10
      //
      // BB10:
      // (W) Fill ADDR_SP_FL_1
      // r[A0] = ...
      // (W) Spill ADD_SP_FL_1
      //
      // ADDR_SP_FL_1 and FL_V10 shouldnt interfere. Without logic below, they
      // would interfere making RA results worse.

      auto regVar1 = nonDefaultDcl->getRegVar();
      auto regVar2 = newDcl->getRegVar();
      if (!((regVar1->isRegVarTmp() || regVar1->isRegVarTransient() ||
             regVar1->isRegVarCoalesced()) &&
            (regVar2->isRegVarTmp() || regVar2->isRegVarTransient() ||
             regVar2->isRegVarCoalesced())))
        return true;

      // Both dcls are RA tmps. Ordinarily they're never live-out of any BB. If
      // any of them is live across BBs then it's possible they don't interfere
      // as per scalar liveness but they may interfere due to divergent CF.
      // For example:
      //
      // if(cond)
      //    (W) V1 = ...
      // else
      //    (W) V2 = ...
      //           = V2
      // endif
      //
      // = V1
      //
      // In above example, V1 doesnt interfere with V2 as per scalar liveness
      // but it should if the branch were divergent. For correctness we need to
      // mark V1 and V2 as interfering. Since they're never live together as per
      // scalar liveness, they need to be handled in augmentation. This case
      // shouldnt occur for RA tmps as RA generated spill/fill tmps are
      // transient and never live-out of any BB. Still adding check to be safe.

      auto id1 = regVar1->getId();
      auto id2 = regVar2->getId();

      for (auto bb : kernel.fg.getBBList()) {
        if (liveAnalysis.isLiveAtExit(bb, id1) ||
            liveAnalysis.isLiveAtExit(bb, id2))
          return true;
      }

      // Conventional intf construction correctly handles the scenario when V1
      // and V2 are referenced in single (same) BB.

      return false;
    };

    if (!isAugNeeded())
      continue;
    // Non-default masks are different so mark interference
    handleSIMDIntf(nonDefaultDcl, newDcl, isCall);
  }
}

//
// Mark interference between newDcl and other incompatible dcls in current
// active lists. If newDcl was created for a subroutine call, do this for all
// varaibles in function summary.
//
void Augmentation::buildSIMDIntfAll(G4_Declare *newDcl) {
  auto callDclMapIt = callDclMap.find(newDcl);
  if (callDclMapIt != callDclMap.end()) {

    G4_Declare *varDcl = NULL;

    if (liveAnalysis.livenessClass(G4_GRF)) // For return value
    {
      G4_INST *callInst = callDclMapIt->second.first;
      varDcl = callInst->getDst()->getBase()->asRegVar()->getDeclare();
      addSIMDIntfForRetDclares(varDcl);
    }

    auto *callBB = callDclMapIt->second.second;
    addSIMDIntfDclForCallSite(callBB);
    return;
  }

  buildSIMDIntfDcl(newDcl, false);
  return;
}

//
// Perform linear scan and mark interference between conflicting dcls with
// incompatible masks.
//
void Augmentation::buildInterferenceIncompatibleMask() {
  // Create 2 active lists - 1 for holding active live-intervals
  // with non-default mask and other for default mask

  for (G4_Declare *newDcl : sortedIntervals) {
    unsigned startIdx = gra.getStartInterval(newDcl)->getLexicalId();
    VISA_DEBUG_VERBOSE(std::cout << "New idx " << startIdx << "\n");
    expireIntervals(startIdx);
    buildSIMDIntfAll(newDcl);

    // Add newDcl to correct list
    if (gra.getHasNonDefaultMaskDef(newDcl) || newDcl->getAddressed() == true) {
      nonDefaultMaskQueue.push(newDcl);
      VISA_DEBUG_VERBOSE(std::cout << "Adding " << newDcl->getName()
                                   << " to non-default list\n");
    } else {
      defaultMaskQueue.push(newDcl);
      VISA_DEBUG_VERBOSE(std::cout << "Adding " << newDcl->getName()
                                   << " to default list\n");
    }
  }

  for (auto func : kernel.fg.funcInfoTable) {
    buildInteferenceForCallsite(func);
  }
  buildInteferenceForRetDeclares();
}

void Augmentation::buildInteferenceForCallSiteOrRetDeclare(G4_Declare *newDcl,
                                                           MaskDeclares *mask) {
  auto newDclAugMask = gra.getAugmentationMask(newDcl);

  for (auto LI = mask->first.begin(), LE = mask->first.end(); LI != LE; ++LI) {
    unsigned i = *LI;
    // Process only global vars
    if (!liveAnalysis.globalVars.test(i))
      continue;
    {
      G4_Declare *defaultDcl = lrs[i]->getDcl();
      if (gra.getAugmentationMask(defaultDcl) != newDclAugMask) {
        handleSIMDIntf(defaultDcl, newDcl, true);
      } else {
        if (liveAnalysis.livenessClass(G4_GRF) &&
            // Populate compatible sparse intf data structure
            // only for weak edges.
            weakEdgeNeeded(gra.getAugmentationMask(defaultDcl),
                           newDclAugMask)) {
          if (defaultDcl->getRegVar()->isPhyRegAssigned() &&
              newDcl->getRegVar()->isPhyRegAssigned()) {
            continue;
          }

          if (intf.isStrongEdgeBetween(defaultDcl, newDcl)) {
            // No need to add weak edge
            continue;
          }

          // defaultDcl and newDcl are compatible live-ranges and can have weak
          // edge in intf graph
          auto it = intf.compatibleSparseIntf.find(defaultDcl);
          if (it != intf.compatibleSparseIntf.end()) {
            it->second.push_back(newDcl);
          } else {
            std::vector<G4_Declare *> v(1, newDcl);
            intf.compatibleSparseIntf.insert(std::make_pair(defaultDcl, v));
          }

          it = intf.compatibleSparseIntf.find(newDcl);
          if (it != intf.compatibleSparseIntf.end()) {
            it->second.push_back(defaultDcl);
          } else {
            std::vector<G4_Declare *> v(1, defaultDcl);
            intf.compatibleSparseIntf.insert(std::make_pair(newDcl, v));
          }
        }
      }
    }
  }

  for (auto LI = mask->second.begin(), LE = mask->second.end(); LI != LE;
       ++LI) {
    unsigned i = *LI;
    // Process only global vars
    if (!liveAnalysis.globalVars.test(i))
      continue;
    // Mark interference among non-default mask variables
    {
      G4_Declare *nonDefaultDcl = lrs[i]->getDcl();
      // Non-default masks are different so mark interference
      handleSIMDIntf(nonDefaultDcl, newDcl, true);
    }
  }
}

void Augmentation::buildInteferenceForCallsite(FuncInfo *func) {
  auto maydefConst = liveAnalysis.subroutineMaydef.find(func);
  if (maydefConst != liveAnalysis.subroutineMaydef.end()) {
    auto *maydef = const_cast<llvm_SBitVector *>(&maydefConst->second);
    for (auto bit : *maydef) {
      G4_Declare *varDcl = lrs[bit]->getDcl();
      buildInteferenceForCallSiteOrRetDeclare(varDcl,
                                              &overlapDclsWithFunc[func]);
    }
  }
  if (kernel.getOption(vISA_LocalRA)) {
    for (uint32_t j = 0; j < kernel.getNumRegTotal(); j++) {
      if (localSummaryOfCallee[func].isGRFBusy(j)) {
        G4_Declare *varDcl = gra.getGRFDclForHRA(j);
        buildInteferenceForCallSiteOrRetDeclare(varDcl,
                                                &overlapDclsWithFunc[func]);
      }
    }
  }
}

void Augmentation::buildInteferenceForRetDeclares() {
  for (auto retDclIt : retDeclares) {
    buildInteferenceForCallSiteOrRetDeclare(retDclIt.first, &retDclIt.second);
  }
}

void Augmentation::buildSummaryForCallees() {
  int totalGRFNum = kernel.getNumRegTotal();

  for (auto func : kernel.fg.sortedFuncTable) {
    unsigned fid = func->getId();
    if (fid == UINT_MAX) {
      // entry kernel
      continue;
    }
    PhyRegSummary funcSummary(kernel.fg.builder, totalGRFNum);
    for (auto &&bb : func->getBBList()) {
      if (auto summary = gra.getBBLRASummary(bb)) {
        for (int i = 0; i < totalGRFNum; i++) {
          if (summary->isGRFBusy(i)) {
            funcSummary.setGRFBusy(i);
          }
        }
      }
    }

    for (auto &&callee : func->getCallees()) {
      PhyRegSummary *summary = &localSummaryOfCallee[callee];
      if (summary) {
        for (int i = 0; i < totalGRFNum; i++) {
          if (summary->isGRFBusy(i)) {
            funcSummary.setGRFBusy(i);
          }
        }
      }
    }
    localSummaryOfCallee[func] = funcSummary;
  }
}

void dumpLiveRanges(GlobalRA &gra, DECLARE_LIST &sortedIntervals) {
  // Emit Python file to draw chart.
  auto &kernel = gra.kernel;
  std::ofstream of;
  std::string fname;
  if (kernel.getOptions()->getOptionCstr(VISA_AsmFileName))
    fname =
        std::string(kernel.getOptions()->getOptionCstr(VISA_AsmFileName)) + "-";
  fname += "iter-" + std::to_string(gra.getIterNo()) + std::string(".py");
  of.open(fname, std::ofstream::out);
  of << "import matplotlib.pyplot as plt"
     << "\n";
  of << "import numpy as np"
     << "\n";
  of << "fig,ax = plt.subplots()"
     << "\n";
  unsigned int x = 10;

  // draw line from (x1, y2) -> (x2, y2)
  // lbl is either "label" or "marker". Latter is used to mark def/use
  // locations. name contains variable name to write out
  auto plot = [&of](int x1, int y1, int x2, int y2, std::string lbl,
                    std::string name) {
    {
      std::string var = "x1 = [";
      var += std::to_string(x1);
      var += std::string(", ");
      var += std::to_string(x2);
      var += std::string("]\n");

      of << var;
    }

    {
      std::string var = std::string("y1");
      var += std::string(" = [");
      var += std::to_string(y1);
      var += std::string(", ");
      var += std::to_string(y2);
      var += std::string("]\n");

      of << var;
    }

    if (lbl == "label")
      of << "th1 = ax.text(*[" << x1 << "," << y1 + 1 << "], \"" << name
         << "\", fontsize=8,rotation=90, rotation_mode='anchor')"
         << "\n";

    of << "plt.plot(x1,y1, " << lbl << " = \"" << name << "\"";
    of << ")\n\n";
  };

  VarReferences refs(kernel);
  for (auto interval : sortedIntervals) {
    auto start = gra.getStartInterval(interval);
    auto end = gra.getEndInterval(interval);
    int startLexId = 0, endLexId = 0;
    if (!start || !end)
      continue;

    // Live-intervals are dumped top-down. So plot
    // them in 4th quadrant.
    startLexId = -(int)start->getLexicalId();
    endLexId = -(int)end->getLexicalId();

    plot(x, startLexId, x, endLexId, "label", interval->getName());

    // Add def/use markers
    // Def indicated by '+' sign on chart
    // Use indicated by a circle on chart
    // Note: Indirect refs are not indicated on chart as they're
    // not computed by VarReferences class.
    auto defs = refs.getDefs(interval);
    auto uses = refs.getUses(interval);

    if (defs) {
      for (auto &def : *defs) {
        auto defInst = std::get<0>(def);
        auto lexId = defInst->getLexicalId();
        plot(x, -(int)lexId, x, -(int)lexId, "marker", "+");
      }
    }

    if (uses) {
      for (auto &use : *uses) {
        auto useInst = std::get<0>(use);
        auto lexId = useInst->getLexicalId();
        plot(x, -(int)lexId, x, -(int)lexId, "marker", ".");
      }
    }
    ++x;
  }

  // Demarcate loops by drawing a box around them
  std::vector<Loop *> allLoops;
  for (auto loop : kernel.fg.getLoops().getTopLoops()) {
    std::stack<Loop *> nested;
    nested.push(loop);
    while (nested.size() > 0) {
      auto top = nested.top();
      allLoops.push_back(loop);
      nested.pop();
      for (auto child : top->immNested) {
        nested.push(child);
      }
    }
  }

  for (auto loop : allLoops) {
    unsigned int minLexId = 0xffffffff, maxLexId = 0;
    for (auto bb : loop->getBBs()) {
      for (auto inst : bb->getInstList()) {
        if (inst->getLexicalId() != UNDEFINED_VAL &&
            inst->getLexicalId() != 0) {
          minLexId = std::min(minLexId, inst->getLexicalId());
          maxLexId = std::max(maxLexId, inst->getLexicalId());
        }
      }
    }

    plot(0, -(int)minLexId, 0, -(int)maxLexId, "label",
         std::string("L") + std::to_string(loop->id));
    plot(x + 10, -(int)minLexId, x + 10, -(int)maxLexId, "label",
         std::string("L") + std::to_string(loop->id));
    plot(0, -(int)minLexId, x + 10, -(int)minLexId, "label",
         std::string("L") + std::to_string(loop->id));
    plot(0, -(int)maxLexId, x + 10, -(int)maxLexId, "label",
         std::string("L") + std::to_string(loop->id));
  }

  of << "plt.show()"
     << "\n";
  of.close();
}

void Augmentation::augmentIntfGraph() {
  if (!(kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D &&
        !liveAnalysis.livenessClass(G4_ADDRESS) && kernel.fg.size() > 2)) {
    if (!kernel.getOption(vISA_DumpRegChart)) {
      return;
    }
  }

  if (kernel.getOption(vISA_LocalRA)) {
    buildSummaryForCallees();
  }

  // First check whether any definitions exist with incompatible mask
  bool nonDefaultMaskDef = markNonDefaultMaskDef();

  if (nonDefaultMaskDef == true) {
    // Atleast one definition with non-default mask was found so
    // perform steps to augment intf graph with such defs

    // Now build live-intervals globally. This function will
    // calculate live-intervals and assign start/end inst
    // for respective declares.
    buildLiveIntervals();

    // Sort live-intervals based on their start
    sortLiveIntervals();

    if (kernel.getOption(vISA_DumpLiveRanges)) {
      dumpLiveRanges(gra, sortedIntervals);
    }

    if (kernel.getOption(vISA_DumpRegChart)) {
      gra.regChart = std::make_unique<RegChartDump>(gra);
      gra.regChart->recordLiveIntervals(sortedIntervals);
    }

    if (gra.verifyAugmentation) {
      gra.verifyAugmentation->loadAugData(
          sortedIntervals, lrs, intf.liveAnalysis->getNumSelectedVar(), &intf,
          gra);
    }

    if (kernel.getOption(vISA_SpillAnalysis)) {
      if (gra.spillAnalysis.get())
        gra.spillAnalysis->LoadAugIntervals(sortedIntervals, gra);
    }

    if (kernel.fg.builder->getOption(vISA_GenerateDebugInfo)) {
      // Following is done to prevent passing GlobalRA to debug info function
      // for clear interface.
      std::vector<std::tuple<G4_Declare *, G4_INST *, G4_INST *>> dclIntervals;
      dclIntervals.reserve(sortedIntervals.size());
      for (auto &dcl : sortedIntervals) {
        dclIntervals.push_back(std::make_tuple(dcl, gra.getStartInterval(dcl),
                                               gra.getEndInterval(dcl)));
      }
      updateDebugInfo(kernel, dclIntervals);
    }

    // Perform linear scan to augment graph
    buildInterferenceIncompatibleMask();

    if (liveAnalysis.livenessClass(G4_GRF)) {
      if ((GlobalRA::useGenericAugAlign(kernel.getPlatformGeneration()) &&
           kernel.getSimdSize() >= kernel.numEltPerGRF<Type_UD>()) ||
          (!GlobalRA::useGenericAugAlign(kernel.getPlatformGeneration()) &&
           kernel.getSimdSize() > kernel.numEltPerGRF<Type_UD>())) {
        // Set alignment of all GRF candidates
        // to 2GRF except for NoMask variables
        VISA_DEBUG_VERBOSE(std::cout
                           << "Kernel size is SIMD" << kernel.getSimdSize()
                           << " so updating all GRFs to be 2GRF aligned"
                           << "\n");
        gra.evenAlign();
      }
      gra.updateSubRegAlignment(kernel.getGRFAlign());
    }
  }
}

void Interference::buildInterferenceWithLocalRA(G4_BB *bb) {
  auto LRASummary = gra.getBBLRASummary(bb);
  if (LRASummary == nullptr) {
    return;
  }

  BitSet cur(kernel.getNumRegTotal(), true);
  llvm_SBitVector live;
  std::vector<int> curUpdate;

  buildInterferenceAtBBExit(bb, live);
  VISA_DEBUG_VERBOSE(std::cout << "BB" << bb->getId() << "\n");

  for (INST_LIST_RITER rit = bb->rbegin(), rend = bb->rend(); rit != rend;
       rit++) {
    bool update = false;
    G4_INST *inst = (*rit);
    curUpdate.clear();
    VISA_DEBUG_VERBOSE({
      inst->emit(std::cout);
      std::cout << "\n";
    });

    // Any physical registers defined will be marked available if
    // current inst is first def or if complete region is written
    G4_DstRegRegion *dst = inst->getDst();

    if (dst && dst->getBase()->isRegVar()) {
      LocalLiveRange *localLR = NULL;
      G4_Declare *topdcl = GetTopDclFromRegRegion(dst);
      unsigned t;

      if (topdcl)
        localLR = gra.getLocalLR(topdcl);

      if (localLR && localLR->getAssigned() && !localLR->isEOT()) {
        int reg, sreg, numrows;
        G4_VarBase *preg = localLR->getPhyReg(sreg);
        numrows = localLR->getTopDcl()->getNumRows();

        vISA_ASSERT(preg->isGreg(), "Register in dst was not GRF");

        reg = preg->asGreg()->getRegNum();

        // Check whether the dst physical register is busy/available.
        // If it is available, and we still see a def that means there was no
        // corresponding use. In such cases mark the physical register as
        // busy, so interference building can take place correctly.
        for (int j = reg, sum = reg + numrows; j < sum; j++) {
          int k = getGRFDclForHRA(j)->getRegVar()->getId();

          if (cur.isSet(j) == true) {
            buildInterferenceWithLive(live, k);
            VISA_DEBUG_VERBOSE(
                std::cout << "Found no use for r" << j
                          << ".0 so marking it as interfering with live set"
                          << "\n");
          }
        }

        if ((localLR->getFirstRef(t) == inst) ||
            liveAnalysis->writeWholeRegion(bb, inst, dst,
                                           builder.getOptions())) {
          // Last row may be only partially used by the current dcl
          // so we still need to pessimistically mark last range as
          // busy. Because some other src opnd that is live may still
          // be using the remaining GRF.
          if (localLR->getSizeInWords() % kernel.numEltPerGRF<Type_UW>() != 0)
            numrows--;

          for (int j = reg, sum = reg + numrows; j < sum; j++) {
            cur.set(j, true);
            VISA_DEBUG_VERBOSE(std::cout << "Setting r" << j << ".0 available"
                                         << "\n");
          }

          // Build interference only for point ranges, ideally which shouldnt
          // exist These are ranges that have a def, but no use
          if (localLR->getFirstRef(t) == localLR->getLastRef(t)) {
            for (int j = reg; j < reg + localLR->getTopDcl()->getNumRows();
                 j++) {
              int k = getGRFDclForHRA(j)->getRegVar()->getId();
              buildInterferenceWithLive(live, k);
            }
          }
        }
      } else if (dst->getBase()->isRegAllocPartaker()) {
        // Global range

        // In bottom-up order if the live-range has not started then
        // a use was not seen for this def. But we need to ensure this
        // variable interferes with all other live vars.
        bool isPointRange = !live.test(dst->getBase()->asRegVar()->getId());

        if (isPointRange) {
          // Mark interference with all busy physical registers
          for (unsigned i = 0; i < kernel.getNumRegTotal(); i++) {
            if (cur.isSet(i) == false) {
              int k = getGRFDclForHRA(i)->getRegVar()->getId();
              checkAndSetIntf(dst->getBase()->asRegVar()->getId(), k);
            }
          }
        }

        if (liveAnalysis->writeWholeRegion(bb, inst, dst,
                                           builder.getOptions()) ||
            inst->isPseudoKill()) {
          // Whole write or first def found so mark this operand as not live for
          // earlier instructions
          auto id = dst->getBase()->asRegVar()->getId();
          updateLiveness(live, id, false);
        }
      } else if (dst->isIndirect() && liveAnalysis->livenessClass(G4_GRF)) {
        // make every var in points-to set live
        const REGVAR_VECTOR &pointsToSet =
            liveAnalysis->getPointsToAnalysis().getAllInPointsToOrIndrUse(dst,
                                                                          bb);
        for (auto pt : pointsToSet) {
          if (pt.var->isRegAllocPartaker()) {
            updateLiveness(live, pt.var->getId(), true);
          }
        }
      }
    }

    // Any physical registers used by src opnds will be busy before the current
    // inst
    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
      G4_Operand *src = inst->getSrc(i);

      if (src && src->isSrcRegRegion() &&
          src->asSrcRegRegion()->getBase()->isRegVar()) {
        LocalLiveRange *localLR = NULL;
        G4_Declare *topdcl = GetTopDclFromRegRegion(src);

        if (topdcl)
          localLR = gra.getLocalLR(topdcl);

        if (localLR && localLR->getAssigned() && !localLR->isEOT()) {
          int sreg;
          G4_VarBase *preg = localLR->getPhyReg(sreg);
          int numrows = localLR->getTopDcl()->getNumRows();

          vISA_ASSERT(preg->isGreg(), "Register in src was not GRF");

          int reg = preg->asGreg()->getRegNum();

          for (int j = reg, sum = reg + numrows; j < sum; j++) {
            int k = getGRFDclForHRA(j)->getRegVar()->getId();

            if (cur.isSet(j) == true) {
              // G4_RegVar with id k was marked free, but becomes
              // busy at this instruction. For incremental updates
              // push this to a vector and use it while updating
              // interference graph incrementally.
              curUpdate.push_back(k);
            }

            cur.set(j, false);
            VISA_DEBUG_VERBOSE(std::cout << "Setting r" << j << ".0 busy\n");
          }
        } else if (src->asSrcRegRegion()->getBase()->isRegAllocPartaker()) {
          if (live.test(
                  src->asSrcRegRegion()->getBase()->asRegVar()->getId()) ==
              false)
            update = true;

          // Mark operand as live from this inst upwards
          auto id = src->asSrcRegRegion()->getBase()->asRegVar()->getId();
          updateLiveness(live, id, true);
        } else if (src->asSrcRegRegion()->isIndirect() &&
                   liveAnalysis->livenessClass(G4_GRF)) {
          // make every var in points-to set live
          const REGVAR_VECTOR &pointsToSet =
              liveAnalysis->getPointsToAnalysis().getAllInPointsToOrIndrUse(
                  src->asSrcRegRegion(), bb);
          for (auto pt : pointsToSet) {
            if (pt.var->isRegAllocPartaker()) {
              if (live.test(pt.var->getId()) == false)
                update = true;

              updateLiveness(live, pt.var->getId(), true);
            }
          }
        }
      }
    }

    if (update == true) {
      // Mark interference with all live
      for (unsigned i = 0; i < kernel.getNumRegTotal(); i++) {
        if (cur.isSet(i) == false) {
          int k = getGRFDclForHRA(i)->getRegVar()->getId();
          buildInterferenceWithLive(live, k);
        }
      }
    } else {
      if (curUpdate.size() > 0) {
        // Perform incremental update. This code is executed when:
        // 1) live set is unchanged, ie no new global range was started in inst
        // 2) cur set has changed, ie an earlier free GRF has become busy
        // Any new busy GRFs will have to be marked as interfering with
        // currently live-ranges. There is no need to iterate over all
        // busy GRFs. Instead only those GRFs that have got busy in this
        // iteration can be considered for incremental updates.
        for (int k : curUpdate) {
          buildInterferenceWithLive(live, k);
        }
      }
    }
  }

  for (unsigned i = 0; i < maxId; i++) {
    bool isAddrSensitive = liveAnalysis->isAddressSensitive(i);

    // If a range is Address taken AND (live-in or live-out or killed)
    // mark it to interfere with all physical registers used by local RA
    // FIXME: need to check if this is actually needed
    if (isAddrSensitive) {
      bool assigned = (lrs[i]->getVar()->getPhyReg() != NULL);
      if (!assigned) {
        bool isLiveIn = liveAnalysis->isLiveAtEntry(bb, i);
        bool isLiveOut = liveAnalysis->isLiveAtExit(bb, i);
        bool isKilled = liveAnalysis->use_kill[bb->getId()].test(i);
        if (isLiveIn || isLiveOut || isKilled) {
          // Make it to interfere with all physical registers used in the BB
          for (uint32_t j = 0, numReg = kernel.getNumRegTotal(); j < numReg;
               j++) {
            if (LRASummary->isGRFBusy(j)) {
              int k = getGRFDclForHRA(j)->getRegVar()->getId();
              checkAndSetIntf(i, k);
            }
          }
        }
      }
    }
  }
}

void Interference::interferenceVerificationForSplit() const {

  std::cout << "\n\n **** Interference Verification Table ****\n";
  for (unsigned i = 0; i < maxId; i++) {
    std::cout << "(" << i << ") ";
    // lrs[i]->dump();
    for (unsigned j = 0; j < maxId; j++) {
      if (interfereBetween(i, j)) {
        if (!interfereBetween(
                gra.getSplittedDeclare(lrs[i]->getDcl())->getRegVar()->getId(),
                j) &&
            (gra.getSplittedDeclare(lrs[i]->getDcl()) != lrs[j]->getDcl())) {
          std::cout << "\t";
          lrs[j]->getVar()->emit(std::cout);
        }
      }
    }
    std::cout << "\n";
  }
}

bool Interference::linearScanVerify() const {
  std::cout << "--------------- " << kernel.getName() << " ----------------"
            << "\n";

  for (unsigned i = 0; i < maxId; i++) {
    G4_VarBase *phyReg_i = lrs[i]->getVar()->getPhyReg();
    if (!phyReg_i || !phyReg_i->isGreg() ||
        gra.isUndefinedDcl(lrs[i]->getDcl()) ||
        lrs[i]->getDcl()->getRegVar()->isNullReg()) {
      continue;
    }
    unsigned regOff_i = lrs[i]->getVar()->getPhyRegOff() *
                        lrs[i]->getVar()->getDeclare()->getElemSize();
    unsigned GRFStart_i =
        phyReg_i->asGreg()->getRegNum() * kernel.numEltPerGRF<Type_UB>() +
        regOff_i;
    unsigned elemsSize_i = lrs[i]->getVar()->getDeclare()->getTotalElems() *
                           lrs[i]->getVar()->getDeclare()->getElemSize();
    unsigned GRFEnd_i = GRFStart_i + elemsSize_i - 1;
    for (unsigned j = 0; j < maxId; j++) {
      if (interfereBetween(i, j)) {
        if (gra.isUndefinedDcl(lrs[j]->getDcl()) ||
            builder.kernel.fg.isPseudoDcl(lrs[j]->getDcl()) ||
            lrs[j]->getDcl()->getRegVar()->isNullReg()) {
          continue;
        }

        G4_VarBase *phyReg_j = lrs[j]->getVar()->getPhyReg();
        unsigned regOff_j = lrs[j]->getVar()->getPhyRegOff() *
                            lrs[j]->getVar()->getDeclare()->getElemSize();
        unsigned GRFStart_j =
            phyReg_j->asGreg()->getRegNum() * kernel.numEltPerGRF<Type_UB>() +
            regOff_j;
        unsigned elemsSize_j = lrs[j]->getVar()->getDeclare()->getTotalElems() *
                               lrs[j]->getVar()->getDeclare()->getElemSize();
        unsigned GRFEnd_j = GRFStart_j + elemsSize_j - 1;
        if (!(GRFEnd_i < GRFStart_j || GRFEnd_j < GRFStart_i)) {
          LSLiveRange *i_LSLR = gra.getLSLR(lrs[i]->getDcl());
          LSLiveRange *j_LSLR = gra.getLSLR(lrs[j]->getDcl());
          unsigned i_start = 0;
          unsigned i_end = 0;
          if (i_LSLR) // For the stack call or some other function which will
                      // add extra declares after allocation
          {
            i_LSLR->getFirstRef(i_start);
            i_LSLR->getLastRef(i_end);
          }

          unsigned j_start = 0;
          unsigned j_end = 0;
          if (j_LSLR) {
            j_LSLR->getFirstRef(j_start);
            j_LSLR->getLastRef(j_end);
          }

          std::cout << "(" << i << "," << j << ")"
                    << lrs[i]->getDcl()->getName() << "(" << GRFStart_i << ":"
                    << GRFEnd_i << ")[" << i_start << "," << i_end << "] vs "
                    << lrs[j]->getDcl()->getName() << "(" << GRFStart_j << ":"
                    << GRFEnd_j << ")[" << j_start << "," << j_end << "]"
                    << "\n";
        }
      }
    }
  }

  return true;
}

/*
 * Find declare according to the name from the declare list
 */
G4_Declare *GraphColor::findDeclare(char *name) {
  for (G4_Declare *dcl : gra.kernel.Declares) {
    if (!strcmp(dcl->getName(), name)) {
      return dcl;
    }
  }

  return NULL;
}

/*
 * For RA debugging
 * Add extra interference info into intf graph
 */
void GraphColor::getExtraInterferenceInfo() {
  // Get the file name
  llvm::SmallString<32> intfName;
  const char *fileName =
      builder.getOptions()->getOptionCstr(vISA_ExtraIntfFile);
  if (!fileName) {
    // Without specific file name, assume the file name is the name of assembly
    // file + ".extraintf" extension and the file is put under the default
    // ShaderOverride path, i.e., c:\Intel\IGC\ShaderOverride on Windows or
    // /tmp/IntelIGC/ShaderOverride on Linux.
#if defined(_WIN32)
    llvm::sys::path::append(intfName, "C:", "Intel", "IGC", "ShaderOverride");
#else
    llvm::sys::path::append(intfName, "/", "tmp", "IntelIGC", "ShaderOVerride");
#endif
    vASSERT(m_options->getOptionCstr(VISA_AsmFileName));
    llvm::StringRef asmName = m_options->getOptionCstr(VISA_AsmFileName);
    llvm::sys::path::append(intfName,
        sanitizePathString(llvm::sys::path::stem(asmName).str()));
    intfName.append(".extraintf");
    fileName = intfName.c_str();
  }

  // open extra inteference info file
  FILE *intfInput = nullptr;
  if ((intfInput = fopen(fileName, "rb")) == nullptr) {
    // No file, just return;
    return;
  }

  // Read the declare variable inteference info from the file
  // The file is composed with the lines with following format in each line:
  // Keydeclare:vardeclare0,vardeclare1,vardeclare2,...
  // Which means the Keydeclare interferences with following vardeclares
  int c;
  char stringBuf[1024];
  char *buf_ptr = stringBuf;
  G4_Declare *keyDeclare = nullptr;
  G4_Declare *varDeclare = nullptr;
  while ((c = fgetc(intfInput)) != EOF) {
    if (c == '\n' || c == ',') {
      *buf_ptr = '\0'; // End of string

      // Find the declare according to the string name
      varDeclare = findDeclare(stringBuf);

      // Must have a KeyDeclare
      if (!keyDeclare) {
        vASSERT(false);
        return;
      }

      if (varDeclare) {
        int src0Id = keyDeclare->getRegVar()->getId();
        int src1Id = varDeclare->getRegVar()->getId();

        // Set inteference
        intf.checkAndSetIntf(src0Id, src1Id);
        VISA_DEBUG_VERBOSE(std::cout << keyDeclare->getName() << ":"
                                     << varDeclare->getName() << "\n");
      }

      // End of line, the key declare set to null
      if (c == '\n') {
        keyDeclare = nullptr;
      }
      buf_ptr = stringBuf;
      continue;
    }

    // Jump over space
    if (c == '\t' || c == ' ' || c == '\r') {
      continue;
    }

    //':' is the end of key declare name
    if (c == ':') {
      *buf_ptr = '\0';
      keyDeclare = findDeclare(stringBuf);
      if (!keyDeclare) {
        vASSERT(false);
        return;
      }
      buf_ptr = stringBuf;
      continue;
    }

    *buf_ptr = c;
    buf_ptr++;
  }

  fclose(intfInput);
}

void Interference::dumpInterference() const {

  std::cout << "\n\n **** Interference Table ****\n";
  for (unsigned i = 0; i < maxId; i++) {
    std::cout << "(" << i << ") ";
    lrs[i]->dump();
    std::cout << "\n";
    for (unsigned j = 0; j < maxId; j++) {
      if (interfereBetween(i, j)) {
        std::cout << "\t";
        lrs[j]->getVar()->emit(std::cout);
      }
    }
    std::cout << "\n\n";
  }
}

void Interference::dumpVarInterference() const {

  std::cout << "\n\n **** Var Interference Table ****\n";
  for (G4_Declare *decl : gra.kernel.Declares) {
    if (decl->getRegVar()->isRegAllocPartaker()) {
      unsigned i = decl->getRegVar()->getId();
      // std::cout << "(" << i << ") ";
      lrs[i]->dump();
      std::cout << "\n";
      for (G4_Declare *decl : gra.kernel.Declares) {
        if (decl->getRegVar()->isRegAllocPartaker()) {
          unsigned j = decl->getRegVar()->getId();
          if (interfereBetween(i, j)) {
            std::cout << "\t";
            lrs[j]->getVar()->emit(std::cout);
          }
        }
      }
      std::cout << "\n\n";
    }
  }
}

GraphColor::GraphColor(LivenessAnalysis &live, unsigned totalGRF, bool hybrid,
                       bool forceSpill_)
    : gra(live.gra), totalGRFRegCount(totalGRF),
      numVar(live.getNumSelectedVar()),
      numSplitStartID(live.getNumSplitStartID()),
      numSplitVar(live.getNumSplitVar()),
      intf(&live, live.gra.incRA.getLRs(), live.getNumSelectedVar(),
           live.getNumSplitStartID(), live.getNumSplitVar(), gra),
      regPool(gra.regPool), builder(gra.builder), isHybrid(hybrid),
      forceSpill(forceSpill_), GCMem(GRAPH_COLOR_MEM_SIZE), kernel(gra.kernel),
      liveAnalysis(live), lrs(live.gra.incRA.getLRs()) {
  spAddrRegSig.resize(getNumAddrRegisters(), 0);
  m_options = builder.getOptions();
}

//
// lrs[i] gives the live range whose id is i
//
void GraphColor::createLiveRanges() {
  lrs.resize(numVar);
  for (auto dcl : gra.kernel.Declares) {
    G4_RegVar *var = dcl->getRegVar();
    // Do not include alias var in liverange creation
    if (!var->isRegAllocPartaker() || dcl->getAliasDeclare() != NULL) {
      continue;
    }
    lrs[var->getId()] = LiveRange::createNewLiveRange(dcl, gra);
  }
}

void GraphColor::computeDegreeForGRF() {
  for (unsigned i = 0; i < numVar; i++) {
    unsigned degree = 0;

    if (!(lrs[i]->getIsPseudoNode()) && !(lrs[i]->getIsPartialDcl())) {
      const std::vector<unsigned> &intfs = intf.getSparseIntfForVar(i);
      unsigned bankDegree = 0;
      auto lraBC = lrs[i]->getBC();
      bool isOdd = (lraBC == BANK_CONFLICT_SECOND_HALF_EVEN ||
                    lraBC == BANK_CONFLICT_SECOND_HALF_ODD);


      auto computeDegree = [&](LiveRange *lr1) {
        if (!lr1->getIsPartialDcl()) {
          unsigned edgeDegree = edgeWeightGRF(lrs[i], lr1);

          degree += edgeDegree;

          auto lrsitBC = lr1->getBC();
          bool isOddBC = (lrsitBC == BANK_CONFLICT_SECOND_HALF_EVEN ||
                          lrsitBC == BANK_CONFLICT_SECOND_HALF_ODD);

          if ((isOdd && isOddBC) || (!isOdd && !isOddBC)) {
            bankDegree += edgeDegree;
          }
        }
      };

      for (auto it : intfs) {
        computeDegree(lrs[it]);
      }

      // consider weak edges in degree computation
      auto *weakEdges = intf.getCompatibleSparseIntf(lrs[i]->getDcl());
      if (weakEdges) {
        for (auto weakNeighbor : *weakEdges) {
          if (!weakNeighbor->getRegVar()->isRegAllocPartaker())
            continue;

          computeDegree(lrs[weakNeighbor->getRegVar()->getId()]);
        }
      }

      if (isOdd) {
        oddTotalDegree += bankDegree; // std::max(bankDegree, oddMaxDegree);
        oddTotalRegNum += lrs[i]->getNumRegNeeded();
        oddMaxRegNum = std::max(oddMaxRegNum, lrs[i]->getNumRegNeeded());
      } else {
        evenTotalDegree += bankDegree; // std::max(bankDegree, evenMaxDegree);
        evenTotalRegNum += lrs[i]->getNumRegNeeded();
        evenMaxRegNum = std::max(evenMaxRegNum, lrs[i]->getNumRegNeeded());
      }
    }

    lrs[i]->setDegree(degree);
  }

  if (kernel.getOption(vISA_SpillAnalysis)) {
    for (unsigned int i = 0; i != numVar; ++i) {
      auto dcl = lrs[i]->getDcl();
      auto degree = lrs[i]->getDegree();
      gra.spillAnalysis->LoadDegree(dcl, degree);
    }
  }
}

void GraphColor::computeDegreeForARF() {
  for (unsigned i = 0; i < numVar; i++) {
    unsigned degree = 0;

    if (!(lrs[i]->getIsPseudoNode())) {
      const std::vector<unsigned> &intfs = intf.getSparseIntfForVar(i);
      for (auto it : intfs) {
        degree += edgeWeightARF(lrs[i], lrs[it]);
      }
    }

    lrs[i]->setDegree(degree);
  }
}

void GraphColor::computeSpillCosts(bool useSplitLLRHeuristic, const RPE *rpe) {
  LiveRangeVec addressSensitiveVars;
  float maxNormalCost = 0.0f;
  VarReferences directRefs(kernel, true, false);
  std::unordered_map<G4_Declare *, std::list<std::pair<G4_INST *, G4_BB *>>>
      indirectRefs;
  // when reg pressure is not very high in iter0, use spill cost function
  // that favors allocating large variables
  bool useNewSpillCost =
      (builder.getOption(vISA_NewSpillCostFunctionISPC) ||
       builder.getOption(vISA_NewSpillCostFunction)) &&
      rpe &&
      !(gra.getIterNo() == 0 &&
        (float)rpe->getMaxRP() < (float)kernel.getNumRegTotal() * 0.80f);

  RA_TRACE({
    if (useNewSpillCost)
      std::cout << "\t--using new spill cost function\n";
  });

  if (useNewSpillCost && liveAnalysis.livenessClass(G4_GRF)) {
    // gather all instructions with indirect operands
    // for ref count computation once.
    for (auto bb : kernel.fg.getBBList()) {
      for (auto inst : bb->getInstList()) {
        auto dst = inst->getDst();
        if (dst && dst->isIndirect()) {
          auto pointsTo = liveAnalysis.getPointsToAnalysis().getAllInPointsTo(
              dst->getBase()
                  ->asRegVar()
                  ->getDeclare()
                  ->getRootDeclare()
                  ->getRegVar());
          if (pointsTo) {
            for (auto &pointee : *pointsTo)
              indirectRefs[pointee.var->getDeclare()->getRootDeclare()]
                  .push_back(std::make_pair(inst, bb));
          }
          continue;
        }

        for (unsigned int i = 0; i != inst->getNumSrc(); ++i) {
          auto src = inst->getSrc(i);
          if (!src || !src->isSrcRegRegion() ||
              !src->asSrcRegRegion()->isIndirect()) {
            continue;
          }
          auto pointsTo = liveAnalysis.getPointsToAnalysis().getAllInPointsTo(
              src->asSrcRegRegion()
                  ->getBase()
                  ->asRegVar()
                  ->getDeclare()
                  ->getRootDeclare()
                  ->getRegVar());
          if (pointsTo) {
            for (auto &pointee : *pointsTo)
              indirectRefs[pointee.var->getDeclare()->getRootDeclare()]
                  .push_back(std::make_pair(inst, bb));
          }
          continue;
        }
      }
    }
  }

  auto getWeightedRefCount = [&](G4_Declare *dcl, unsigned int useWt = 1,
                                 unsigned int defWt = 1) {
    auto defs = directRefs.getDefs(dcl);
    auto uses = directRefs.getUses(dcl);
    auto &loops = kernel.fg.getLoops();
    unsigned int refCount = 0;
    const unsigned int assumeLoopIter = 10;

    if (defs) {
      for (auto &def : *defs) {
        auto *bb = std::get<1>(def);
        auto *innerMostLoop = loops.getInnerMostLoop(bb);
        if (innerMostLoop) {
          auto nestingLevel = innerMostLoop->getNestingLevel();
          refCount += (unsigned int)std::pow(assumeLoopIter, nestingLevel);
        } else
          refCount += defWt;
      }
    }

    if (uses) {
      for (auto &use : *uses) {
        auto *bb = std::get<1>(use);
        auto *innerMostLoop = loops.getInnerMostLoop(bb);
        if (innerMostLoop) {
          auto nestingLevel = innerMostLoop->getNestingLevel();
          refCount += (unsigned int)std::pow(assumeLoopIter, nestingLevel);
        } else
          refCount += useWt;
      }
    }

    if (dcl->getAddressed()) {
      auto indirectRefsIt = indirectRefs.find(dcl);
      if (indirectRefsIt != indirectRefs.end()) {
        auto &dclIndirRefs = (*indirectRefsIt).second;
        for (auto &item : dclIndirRefs) {
          auto bb = item.second;

          auto *innerMostLoop = loops.getInnerMostLoop(bb);
          if (innerMostLoop) {
            auto nestingLevel = innerMostLoop->getNestingLevel();
            refCount += (unsigned int)std::pow(assumeLoopIter, nestingLevel);
          } else
            refCount += useWt;
        }
      }
    }

    return refCount == 0 ? 1 : refCount;
  };

  std::unordered_map<G4_Declare *, std::vector<G4_Declare *>> addrTakenMap,
      revAddrTakenMap;
  bool addrMapsComputed = false;
  auto incSpillCostCandidate = [&](LiveRange *lr) {
    if (kernel.getOption(vISA_IncSpillCostAllAddrTaken))
      return true;
    if (!addrMapsComputed) {
      const_cast<PointsToAnalysis &>(liveAnalysis.getPointsToAnalysis())
          .getPointsToMap(addrTakenMap);
      const_cast<PointsToAnalysis &>(liveAnalysis.getPointsToAnalysis())
          .getRevPointsToMap(revAddrTakenMap);
      addrMapsComputed = true;
    }

    // this condition is a safety measure and isnt expected to be true.
    auto it = revAddrTakenMap.find(lr->getDcl());
    if (it == revAddrTakenMap.end())
      return true;

    for (auto &addrVar : (*it).second) {
      if (addrTakenMap.count(addrVar) > 1)
        return true;
    }
    return false;
  };

  for (unsigned i = 0; i < numVar; i++) {
    G4_Declare *dcl = lrs[i]->getDcl();

    if (dcl->getIsPartialDcl()) {
      continue;
    }
    //
    // The spill cost of pseudo nodes inserted to aid generation of save/restore
    // code must be the minimum so that such nodes go to the bootom of the color
    // stack.
    //
    if (builder.kernel.fg.isPseudoDcl(dcl)) {
      if (builder.kernel.fg.isPseudoVCADcl(dcl)) {
        lrs[i]->setSpillCost(MINSPILLCOST + 1);
      } else {
        lrs[i]->setSpillCost(MINSPILLCOST);
      }
    }

    auto dclLR = gra.getLocalLR(dcl);
    if (dclLR != NULL && dclLR->getSplit()) {
      lrs[i]->setSpillCost(MINSPILLCOST + 2);
    }
    //
    // Give the tiny spill/fill ranges an infinite spill cost, so that they are
    // picked first for coloring.
    // Also ARF live ranges with exclusively sequential references within the
    // code are assigned an infinite spill cost as spilling them will not lower
    // the register pressure in the region they are referenced. This does not
    // necessarily hold for GRF live ranges are these are potentially large in
    // size but the portions accessed by each sequential use are limited to 2
    // registers for general instructions and 8 registers for SEND instructions.
    //
    else if (gra.isAddrFlagSpillDcl(dcl) || lrs[i]->isRetIp() ||
             lrs[i]->getIsInfiniteSpillCost() == true ||
             ((lrs[i]->getVar()->isRegVarTransient() == true ||
               lrs[i]->getVar()->isRegVarTmp() == true) &&
              lrs[i]->getVar()->isSpilled() == false) ||
             dcl == gra.getOldFPDcl() ||
             (m_options->getOption(vISA_enablePreemption) &&
              dcl == builder.getBuiltinR0())) {
      lrs[i]->setSpillCost(MAXSPILLCOST);
    } else if (dcl->isDoNotSpill()) {
      lrs[i]->setSpillCost(MAXSPILLCOST);
    }
    //
    // Calculate spill costs of regular nodes.
    //
    else {
      float spillCost = 0.0f;
      // NOTE: Add 1 to degree to avoid divide-by-0, as a live range may have no
      // neighbors
      if (builder.kernel.getInt32KernelAttr(Attributes::ATTR_Target) ==
          VISA_3D) {
        if (useSplitLLRHeuristic) {
          spillCost = 1.0f * lrs[i]->getRefCount() / (lrs[i]->getDegree() + 1);
        } else {
          vASSERT(lrs[i]->getDcl()->getTotalElems() > 0);
          if (!liveAnalysis.livenessClass(G4_GRF) || !useNewSpillCost) {
            // address or flag variables
            unsigned short numRows = lrs[i]->getDcl()->getNumRows();
            spillCost = 1.0f * lrs[i]->getRefCount() * lrs[i]->getRefCount() *
                        lrs[i]->getDcl()->getByteSize() *
                        (float)sqrt(lrs[i]->getDcl()->getByteSize()) /
                        ((float)sqrt(lrs[i]->getDegree() + 1) *
                         (float)(sqrt(sqrt(numRows))));
          } else {
            // GRF variables

            auto refCount = getWeightedRefCount(lrs[i]->getDcl());
            spillCost = 1.0f * refCount * refCount * refCount /
                        ((float)(lrs[i]->getDegree() + 1) *
                         (float)(lrs[i]->getDegree() + 1));
          }
        }
      } else {
        if (!useNewSpillCost) {
          spillCost = liveAnalysis.livenessClass(G4_GRF)
                          ? lrs[i]->getDegree()
                          : 1.0f * lrs[i]->getRefCount() *
                                lrs[i]->getRefCount() /
                                (lrs[i]->getDegree() + 1);
        } else {
          auto refCount = getWeightedRefCount(lrs[i]->getDcl());
          spillCost = 1.0f * refCount * refCount * refCount /
                      ((float)(lrs[i]->getDegree() + 1) *
                       (float)(lrs[i]->getDegree() + 1));
        }
      }

      lrs[i]->setSpillCost(spillCost);

      // Track address sensitive live range.
      if (liveAnalysis.isAddressSensitive(i) && incSpillCostCandidate(lrs[i])) {
        addressSensitiveVars.push_back(lrs[i]);
      } else {
        // Set the spill cost of all other normal live ranges, and
        // track the max normal cost.
        if (maxNormalCost < spillCost) {
          maxNormalCost = spillCost;
        }
      }
    }
  }

  //
  // Set the spill cost of address sensitive live ranges above all the
  // normal live ranges, so that they get colored before all the normal
  // live ranges.
  //
  for (LiveRange *lr : addressSensitiveVars) {
    if (lr->getSpillCost() != MAXSPILLCOST) {
      lr->setSpillCost(maxNormalCost + lr->getSpillCost());
    }
  }
}

//
// subtract lr's neighbors that are still in work list
//
void GraphColor::relaxNeighborDegreeGRF(LiveRange *lr) {
  if (!(lr->getIsPseudoNode()) && !(lr->getIsPartialDcl())) {
    unsigned lr_id = lr->getVar()->getId();

    // relax degree between 2 nodes
    auto relaxDegree = [&](LiveRange *lr1) {
      if (lr1->getActive() && !lr1->getIsPseudoNode() &&
          !(lr1->getIsPartialDcl())) {
        unsigned w = edgeWeightGRF(lr1, lr);
        VISA_DEBUG_VERBOSE({
          std::cout << "\t relax ";
          lr1->dump();
          std::cout << " degree(" << lr1->getDegree() << ") - " << w << "\n";
        });
        lr1->subtractDegree(w);

        unsigned availColor = numColor;
        availColor = numColor - lr1->getNumForbidden();

        if (lr1->getDegree() + lr1->getNumRegNeeded() <= availColor) {
          unconstrainedWorklist.push_back(lr1);
          lr1->setActive(false);
        }
      }
    };

    const std::vector<unsigned> &intfs = intf.getSparseIntfForVar(lr_id);
    for (auto it : intfs) {
      LiveRange *lrs_it = lrs[it];

      relaxDegree(lrs_it);
    }

    auto *weakEdges = intf.getCompatibleSparseIntf(lr->getDcl());
    if (weakEdges) {
      for (auto weakNeighbor : *weakEdges) {
        if (!weakNeighbor->getRegVar()->isRegAllocPartaker())
          continue;
        auto lr1 = lrs[weakNeighbor->getRegVar()->getId()];
        relaxDegree(lr1);
      }
    }
  }
}
void GraphColor::relaxNeighborDegreeARF(LiveRange *lr) {
  if (!(lr->getIsPseudoNode())) {
    unsigned lr_id = lr->getVar()->getId();
    const std::vector<unsigned> &intfs = intf.getSparseIntfForVar(lr_id);
    for (auto it : intfs) {
      LiveRange *lrs_it = lrs[it];

      if (lrs_it->getActive() && !lrs_it->getIsPseudoNode()) {
        unsigned w = edgeWeightARF(lrs_it, lr);
        VISA_DEBUG_VERBOSE({
          std::cout << "\t relax ";
          lrs_it->dump();
          std::cout << " degree(" << lrs_it->getDegree() << ") - " << w << "\n";
        });
        lrs_it->subtractDegree(w);

        unsigned availColor = numColor;

        if (lrs_it->getDegree() + lrs_it->getNumRegNeeded() <= availColor) {
          unconstrainedWorklist.push_back(lrs_it);
          lrs_it->setActive(false);
        }
      }
    }
  }
}

static bool compareSpillCost(LiveRange *lr1, LiveRange *lr2) {
  return lr1->getSpillCost() < lr2->getSpillCost() ||
         (lr1->getSpillCost() == lr2->getSpillCost() &&
          lr1->getVar()->getId() < lr2->getVar()->getId());
}

//
// All nodes in work list are all contrained (whose degree > max color)
// find one contrained node and move it to order list
//
void GraphColor::removeConstrained() {
  if (!constrainedWorklist.empty()) {
    LiveRange *lr = constrainedWorklist.front();
    constrainedWorklist.pop_front();

    if (lr->getActive()) {
      VISA_DEBUG_VERBOSE({
        std::cout << ".... Remove Constrained ";
        lr->dump();
        std::cout << "\n";
      });

      if (liveAnalysis.livenessClass(G4_GRF)) {
        relaxNeighborDegreeGRF(lr);
      } else {
        relaxNeighborDegreeARF(lr);
      }
      colorOrder.push_back(lr);
      lr->setActive(false);
    }
  }
}

void GraphColor::determineColorOrdering() {
  numColor = 0;
  if (liveAnalysis.livenessClass(G4_GRF))
    numColor = totalGRFRegCount;
  else if (liveAnalysis.livenessClass(G4_ADDRESS))
    numColor = getNumAddrRegisters();
  else if (liveAnalysis.livenessClass(G4_FLAG))
    numColor = builder.getNumFlagRegisters();

  unsigned numUnassignedVar = liveAnalysis.getNumUnassignedVar();

  //
  // create an array for sorting live ranges
  //
  LiveRangeVec sorted;
  sorted.reserve(numUnassignedVar);
  unsigned j = 0;
  for (unsigned i = 0; i < numVar; i++) {
    if (lrs[i]->getPhyReg() == nullptr && !lrs[i]->getIsPartialDcl()) {
      sorted.push_back(lrs[i]);
      j++;
    }
  }

  if (gra.incRA.isEnabledWithVerification(kernel)) {
    gra.incRA.computeLeftOverUnassigned(sorted, liveAnalysis);
  }

  vISA_ASSERT(j == numUnassignedVar, ERROR_GRAPHCOLOR);

  //
  // sort the live range array
  //
  std::sort(sorted.begin(), sorted.end(), compareSpillCost);

  for (unsigned i = 0; i < numUnassignedVar; i++) {
    LiveRange *lr = sorted[i];
    unsigned availColor = numColor;
    availColor = numColor - lr->getNumForbidden();

    if (lr->getDegree() + lr->getNumRegNeeded() <= availColor) {
      unconstrainedWorklist.push_back(lr);
      lr->setActive(false);
      if (lr->getRegKind() == G4_GRF) {
        // Mark current lr as unconstrained, which means RR algorithm can always
        // be applied to the variable.
        lr->setUnconstrained(true);
      }
    } else {
      constrainedWorklist.push_back(lr);
      lr->setActive(true);
    }
  }

  VISA_DEBUG_VERBOSE({
    std::cout << "\nSPILL COST\n";
    for (unsigned i = 0; i < numUnassignedVar; i++) {
      sorted[i]->dump();
      std::cout << "\t spillCost=" << sorted[i]->getSpillCost();
      std::cout << "\t degree=" << sorted[i]->getDegree();
      std::cout << "\t refCnt=" << sorted[i]->getRefCount();
      std::cout << "\t size=" << sorted[i]->getDcl()->getByteSize();
      std::cout << "\t active=" << sorted[i]->getActive();
      std::cout << "\n";
    }
    std::cout << "\n";
  });

  while (!constrainedWorklist.empty() || !unconstrainedWorklist.empty()) {
    while (!unconstrainedWorklist.empty()) {
      LiveRange *lr = unconstrainedWorklist.front();
      unconstrainedWorklist.pop_front();

      VISA_DEBUG_VERBOSE({
        std::cout << ".... Remove Unconstrained ";
        lr->dump();
        std::cout << "\n";
      });

      if (liveAnalysis.livenessClass(G4_GRF)) {
        relaxNeighborDegreeGRF(lr);
      } else {
        relaxNeighborDegreeARF(lr);
      }
      colorOrder.push_back(lr);
    }

    removeConstrained();
  }
}

void PhyRegUsage::updateRegUsage(LiveRange *lr) {
  G4_Declare *dcl = lr->getDcl();
  G4_VarBase *pr;
  if (lr->getIsPartialDcl()) {
    pr = lrs[lr->getParentLRID()]->getPhyReg();
  } else {
    pr = lr->getPhyReg();
  }

  if (!pr) {
    return;
  }
  if (pr->isGreg()) {
    if (dcl->getIsPartialDcl()) {
      // Assumptions:
      //  1. the offset of the sub declare must be G4_WSIZE aligned
      //  2. the size of the subdeclare must be G4_WSIZE aligned
      markBusyForDclSplit(G4_GRF, ((G4_Greg *)pr)->getRegNum(),
                          (lrs[lr->getParentLRID()]->getPhyRegOff() *
                               TypeSize(dcl->getElemType()) +
                           gra.getSubOffset(dcl)) /
                              G4_WSIZE,
                          dcl->getByteSize() / G4_WSIZE, dcl->getNumRows());
    } else {
      markBusyGRF(
          ((G4_Greg *)pr)->getRegNum(),
          PhyRegUsage::offsetAllocUnit(lr->getPhyRegOff(), dcl->getElemType()),
          dcl->getWordSize(), lr->getNumRegNeeded(), dcl->isPreDefinedVar());
    }
  } else if (pr->isFlag()) {
    auto flagWordOffset = lr->getPhyReg()->asAreg()->getFlagNum() * 2;
    markBusyFlag(
        0,
        PhyRegUsage::offsetAllocUnit(flagWordOffset + lr->getPhyRegOff(),
                                     dcl->getElemType()),
        PhyRegUsage::numAllocUnit(dcl->getNumElems(), dcl->getElemType()),
        dcl->getNumRows());
  } else if (pr->isA0()) {
    markBusyAddress(
        0, PhyRegUsage::offsetAllocUnit(lr->getPhyRegOff(), dcl->getElemType()),
        PhyRegUsage::numAllocUnit(dcl->getNumElems(), dcl->getElemType()),
        dcl->getNumRows());
  }
  else {
    vISA_ASSERT(false, ERROR_GRAPHCOLOR); // un-handled reg type
  }
}

bool GraphColor::assignColors(ColorHeuristic colorHeuristicGRF,
                              bool doBankConflict, bool highInternalConflict,
                              bool doBundleConflict, bool honorHints) {
  RA_TRACE(std::cout << "\t--"
                     << (colorHeuristicGRF == ROUND_ROBIN ? "round-robin"
                                                          : "first-fit")
                     << (doBankConflict ? " BCR" : "") << " graph coloring\n");

  unsigned bank1_end = 0;
  unsigned bank2_end = totalGRFRegCount - 1;
  unsigned bank1_start = 0;
  unsigned bank2_start = totalGRFRegCount - 1;
  unsigned totalGRFNum = kernel.getNumRegTotal();
  bool oneGRFBankDivision = gra.kernel.fg.builder->oneGRFBankDivision();
  bool allocFromBanks =
      liveAnalysis.livenessClass(G4_GRF) && builder.lowHighBundle() &&
      !builder.getOptions()->getuInt32Option(vISA_ReservedGRFNum) &&
      doBankConflict &&
      ((oneGRFBankDivision && gra.kernel.getSimdSize() >= g4::SIMD16) ||
       (!oneGRFBankDivision && highInternalConflict));

  if (allocFromBanks && (colorHeuristicGRF == ROUND_ROBIN)) {
    bank1_end = (unsigned)((totalGRFRegCount - 1) *
                           (((float)evenTotalDegree / evenTotalRegNum) /
                            (((float)evenTotalDegree / evenTotalRegNum) +
                             ((float)oddTotalDegree / oddTotalRegNum))));
    if (bank1_end < evenMaxRegNum ||
        totalGRFRegCount - bank1_end < oddMaxRegNum ||
        bank1_end == totalGRFRegCount - 1 || bank1_end == 0) {
      // FIXME: How can we early return without assigning???
      return false;
    }

    bank2_end = bank1_end + 1;
  }

  G4_RegFileKind rFile = G4_GRF;
  if (liveAnalysis.livenessClass(G4_FLAG))
    rFile = G4_FLAG;
  else if (liveAnalysis.livenessClass(G4_ADDRESS))
    rFile = G4_ADDRESS;

  FreePhyRegs FPR(kernel);

  unsigned maxGRFCanBeUsed = totalGRFRegCount;
  // FIXME: the bank configs should be computed in PhyRegAllocationState instead
  // of pased in, but the strange early return from above prevents this..
  PhyRegAllocationState parms(gra, lrs, rFile, maxGRFCanBeUsed, bank1_start,
                              bank1_end, bank2_start, bank2_end, doBankConflict,
                              doBundleConflict);
  bool noIndirForceSpills = builder.getOption(vISA_NoIndirectForceSpills);

  auto &varSplitPass = *gra.getVarSplitPass();

  // Returns true when valid assignment is found or when lr is added to spilled
  // set. Adding to spill set happens only if heuristic is not round_robin (FF
  // may not spill). Parameter returnFalseOnFail is set when the function is
  // required to return false on assignment failure. When parameter spillAllowed
  // is set to true, this function adds lr to spilled set. If spillAllowed is
  // false, the lr is not added to spill set. This logic is useful to try
  // re-allocation of a child/parent dcl when split is enabled.
  // ignoreChildrenIntf is set to true when all children are assigned to
  // consecutive ranges and we want to get fully coalesceable assignment for
  // parent. In such circumstance, we don't want to account for interference
  // between parent/child since doing so cannot result in a coalesceable
  // assignment.
  auto assignColor = [&](LiveRange *lr, bool ignoreChildrenIntf = false,
                         bool spillAllowed = true,
                         bool returnFalseOnFail = false) {
    auto lrVar = lr->getVar();

    //
    // assign register to live ranges
    //
    if (lr->getPhyReg() == NULL && !lrVar->isSpilled() &&
        !lr->getIsPartialDcl()) // no assigned register yet and not spilled
    {
      G4_Declare *parentDcl = nullptr;
      bool skipParentIntf = false;
      if (lr->hasAllocHint()) {
        parms.setStartGRF(
            (lr->getAllocHint() >= maxGRFCanBeUsed ? 0 : lr->getAllocHint()));
        if (varSplitPass.isPartialDcl(lr->getDcl())) {
          parentDcl = varSplitPass.getParentDcl(lr->getDcl());
          if (parentDcl) {
            auto parentGRF = parentDcl->getRegVar()->getPhyReg();
            if (!parentGRF && parentDcl->getRegVar()->isRegAllocPartaker()) {
              parentGRF = lrs[parentDcl->getRegVar()->getId()]->getPhyReg();
            }
            if (parentGRF) {
              // mark interference between partial lr and all
              // other GRFs allocated to parent dcl. this logic
              // allows either coalesceable allocation or a
              // fully non-overlapping assignment.
              auto siblingNum = varSplitPass.getSiblingNum(lr->getDcl());
              auto parentGRFNum = parentGRF->asGreg()->getRegNum();
              auto parentNumRows = parentDcl->getNumRows();
              auto numRows = lr->getDcl()->getNumRows();
              for (unsigned i = parentGRFNum;
                   i != (parentGRFNum + parentNumRows); i += numRows) {
                if ((i - parentGRFNum) == siblingNum * numRows)
                  continue;
                lr->markForbidden(GCMem, i, numRows);
              }
              skipParentIntf = true;
            }
          }
        }
      }

      unsigned lr_id = lrVar->getId();
      //
      // compute what registers are already assigned
      //
      PhyRegUsage regUsage(parms, FPR);

      const std::vector<unsigned> &intfs = intf.getSparseIntfForVar(lr_id);
      auto weakEdgeSet =
          intf.getCompatibleSparseIntf(lrVar->getDeclare()->getRootDeclare());
      for (auto it : intfs) {
        LiveRange *lrTemp = lrs[it];
        if (lrTemp->getPhyReg() != nullptr || lrTemp->getIsPartialDcl()) {
          if (lrTemp
                  ->getIsSplittedDcl()) // Only interfere with children declares
          {
            continue;
          }

          if (skipParentIntf && lrTemp->getDcl() == parentDcl)
            continue;

          if (ignoreChildrenIntf && varSplitPass.isParentChildRelation(
                                        lr->getDcl(), lrTemp->getDcl()))
            continue;

          regUsage.updateRegUsage(lrTemp);
        }
      }

      if (weakEdgeSet) {
        regUsage.runOverlapTest(true);
        for (auto weakDcl : *weakEdgeSet) {
          auto regVar = weakDcl->getRootDeclare()->getRegVar();
          unsigned pvar = 0, numRegs = 0;
          if (regVar->isPhyRegAssigned()) {
            // This branch will be taken for dcls assigned
            // regs by LRA.
            pvar = regVar->getPhyReg()->asGreg()->getRegNum();
            numRegs = weakDcl->getNumRows();
          } else {
            // For dcls not assigned regs by LRA, lookup temp
            // registers assigned to LiveRange instances.
            auto id = regVar->getId();
            auto lr = lrs[id];
            auto phyReg = lr->getPhyReg();
            if (phyReg) {
              pvar = phyReg->asGreg()->getRegNum();
              numRegs = weakDcl->getNumRows();
            }
          }

          // For now it is assumed only 8-byte types will appear
          // here. If other sized types will also appear then
          // augmentation mask also needs to be sent in
          // weak edge data structure below.
          for (unsigned r = pvar; r < (pvar + numRegs); r++) {
            auto use = regUsage.getWeakEdgeUse(r);
            if (use == 0 || use == (r - pvar + 1)) {
              regUsage.setWeakEdgeUse(r, r - pvar + 1);
            } else {
              // Indiates two neighbors use a physical
              // register with different overlap.
              regUsage.setWeakEdgeUse(r, 0xff);
            }
          }
        }
      }

      ColorHeuristic heuristic = colorHeuristicGRF;

      bool failed_alloc = false;
      G4_Declare *dcl = lrVar->getDeclare();

      if (!(noIndirForceSpills && liveAnalysis.isAddressSensitive(lr_id)) &&
          forceSpill &&
          (dcl->getRegFile() == G4_GRF || dcl->getRegFile() == G4_FLAG) &&
          lr->getRefCount() != 0 && lr->getSpillCost() != MAXSPILLCOST) {
        failed_alloc = true;
      }

      if ((dcl->getNumRows() > totalGRFNum) ||
          (dcl->isForceSpilled() && (lr->getSpillCost() != MAXSPILLCOST))) {
        // we sure as hell won't get an assignment
        failed_alloc = true;
      }

      if (kernel.getOption(vISA_GCRRInFF)) {
        if (lr->getRegKind() != G4_GRF) {
          // None GRF assignment, keep single FF or RR algorithm
          if (heuristic == FIRST_FIT) {
            parms.setStartGRF(0);
          }
        } else if (heuristic == FIRST_FIT && !lr->getIsUnconstrained()) {
          // GRF assignment, start GRF is always 0 if first fit algorithm is
          // used and the variable is constrainted
          parms.setStartGRF(0);
        }
      }

      if (!failed_alloc) {
        // When evenAlignNeeded is true, it is binding for correctness
        bool evenAlignNeeded = gra.isEvenAligned(lrVar->getDeclare());
        BankAlign align = evenAlignNeeded ? BankAlign::Even : BankAlign::Either;
        if (allocFromBanks && !lr->hasAllocHint()) {

          if (!isHybrid && oneGRFBankDivision &&
              (!evenAlignNeeded ||
               builder.getPlatformGeneration() == PlatformGen::GEN9)) {
            gra.getBankAlignment(lr, align);
          }
          failed_alloc |= !regUsage.assignGRFRegsFromBanks(
              lr, align, lr->getForbidden(), heuristic, oneGRFBankDivision);
        } else {
          failed_alloc |= !regUsage.assignRegs(
              highInternalConflict, lr, lr->getForbidden(), align,
              gra.getSubRegAlign(lrVar->getDeclare()), heuristic,
              lr->getSpillCost(), lr->hasAllocHint());
        }
      }

      //
      // assign unused color
      //
      if (failed_alloc) {
        //
        // for GRF register assignment, if we are performing round-robin (1st
        // pass) then abort on spill
        //
        if ((heuristic == ROUND_ROBIN ||
             (doBankConflict && !kernel.getOption(vISA_forceBCR))) &&
            (lr->getRegKind() == G4_GRF || lr->getRegKind() == G4_FLAG)) {
          return false;
        } else if (kernel.fg.isPseudoDcl(dcl)) {
          // these pseudo dcls are not (and cannot be) spilled, but instead
          // save/restore code will be inserted in stack call prolog/epilog
        } else {
          // for first-fit register assignment track spilled live ranges
          if (spillAllowed) {
            // When retrying a coalesceable assignment, don't spill
            // if there is no GRF available.
            spilledLRs.push_back(lr);
            lr->setSpilled(true);
          }
        }

        if (returnFalseOnFail) {
          return false;
        }
      } else {
        // Allocation succeeded, set hint if this is a split/child dcl
        if (!ignoreChildrenIntf &&
            (varSplitPass.isSplitDcl(dcl) || varSplitPass.isPartialDcl(dcl))) {
          varSplitPass.writeHints(dcl, lrs);
        }
      }
    }
    VISA_DEBUG_VERBOSE({
      lr->dump();
      std::cout << "\n";
    });
    return true;
  };

  // colorOrder is in reverse order (unconstrained at front)
  for (auto iter = colorOrder.rbegin(), iterEnd = colorOrder.rend();
       iter != iterEnd; ++iter) {
    auto lr = (*iter);

    // in case child/parent was already spilled earlier, don't recolor
    if (lr->isSpilled())
      continue;

    bool ret = assignColor(lr);

    // early exit
    if (!ret)
      return false;

    if (lr->getSpillCost() == MAXSPILLCOST && !lr->getPhyReg() && honorHints) {
      // infinite spill cost range spilled
      // undo all allocations done to split vars
      // and skip adhering to hints for preserving
      // correctness.
      resetTemporaryRegisterAssignments();
      return assignColors(colorHeuristicGRF, doBankConflict,
                          highInternalConflict, false, false);
    }

    if (honorHints && gra.getIterNo() == 0) {
      // attempt coalescing in non-spill iteration only
      if (varSplitPass.isSplitDcl(lr->getDcl())) {
        // Try allocating children, out of order in hopes
        // of getting a coalesceable assignment
        auto children = varSplitPass.getChildren(lr->getDcl());
        for (auto child : *children) {
          if (child->getRegVar()->isRegAllocPartaker()) {
            auto childLR = lrs[child->getRegVar()->getId()];
            if (!childLR->getPhyReg()) {
              auto isChildSpilled = childLR->isSpilled();
              assignColor(childLR, false, !isChildSpilled);
              // if allocated GRF is different than hint, then
              // undo allocation and let coloring take its course.
              // this can be done only if the childLR wasnt
              // already processed in colorOrder.
              if (!isChildSpilled && childLR->getPhyReg()) {
                auto hint = childLR->getAllocHint();
                if (childLR->getPhyReg()->asGreg()->getRegNum() != hint) {
                  // this is executed only if childLR is guaranteed to be
                  // processed later on in colorOrder.
                  childLR->resetPhyReg();
                }
              } else if (isChildSpilled && childLR->getPhyReg()) {
                // was spilled earlier, got allocation now
                spilledLRs.remove(childLR);
              }
            } else {
              // retry allocating as per hint
              auto oldPhyReg = childLR->getPhyReg();
              auto oldPhySubReg = childLR->getPhyRegOff();
              auto hint = childLR->getAllocHint();
              if (oldPhyReg->asGreg()->getRegNum() == hint)
                continue;
              childLR->resetPhyReg();
              bool success = assignColor(childLR, false, false, true);
              if (!success ||
                  childLR->getPhyReg()->asGreg()->getRegNum() != hint)
                childLR->setPhyReg(oldPhyReg, oldPhySubReg);
            }
          }
        }
      }

      // if all children are assigned consecutive GRFs but parent isnt
      // then try re-assigning parent
      if (varSplitPass.isPartialDcl(lr->getDcl()) &&
          varSplitPass.reallocParent(lr->getDcl(), getLiveRanges())) {
        auto parentDcl = varSplitPass.getParentDcl(lr->getDcl());
        auto parentLR = getLiveRanges()[parentDcl->getRegVar()->getId()];
        auto oldPhyReg = parentLR->getPhyReg();
        auto oldPhySubReg = parentLR->getPhyRegOff();
        bool isParentSpilled = parentLR->isSpilled();
        parentLR->resetPhyReg();
        varSplitPass.writeHints(lr->getDcl(), getLiveRanges());
        assignColor(parentLR, true, !isParentSpilled);
        // If parent's assigned GRF is non-coalesceable assignment then
        // undo it as it is risky to keep this because parent's intf
        // doesnt include children.
        auto newParentAssignment = parentLR->getPhyReg();
        if ((newParentAssignment &&
             newParentAssignment->asGreg()->getRegNum() !=
                 parentLR->getAllocHint()) ||
            !newParentAssignment)
          parentLR->setPhyReg(oldPhyReg, oldPhySubReg);

        if (isParentSpilled && parentLR->getPhyReg()) {
          // remove parent from spill list since it got an allocation this time
          spilledLRs.remove(parentLR);
          parentLR->setSpilled(false);
        }
      }
    }
  }

  if (failSafeIter) {
    // As per spec, EOT has to be allocated to r112+.
    // When fail safe iteration is run, upper GRFs are
    // reserved. It's possible that # of reserved
    // GRFs are too many and r112+ allocation restriction
    // on EOT cannot be fulfilled (eg, r116-r127 are reserved
    // EOT src operand size is 8 GRFs). This causes EOT var
    // to spill and then the spill range faces the same
    // restriction. The fix here is to check whether
    // reserved GRF restriction can be eased for EOT.
    auto hasSpilledNeighbor = [&](unsigned int id) {
      for (const auto *spillLR : spilledLRs) {
        if (id != spillLR->getVar()->getId() &&
            getIntf()->interfereBetween(id, spillLR->getVar()->getId()))
          return true;
      }
      return false;
    };

    if (builder.getOption(vISA_HybridRAWithSpill)) {
      // This local analysis is skipped in favor of
      // compile time in global RA loop, so run it here
      // when needed.
      gra.markGraphBlockLocalVars();
    }

    for (auto lrIt = spilledLRs.begin(); lrIt != spilledLRs.end(); ++lrIt) {
      auto lr = (*lrIt);
      bool needsEOTGRF = lr->getEOTSrc() && builder.hasEOTGRFBinding();
      if (needsEOTGRF && gra.isBlockLocal(lr->getDcl()) &&
          (totalGRFRegCount + lr->getNumRegNeeded()) <=
              kernel.getNumRegTotal() &&
          !hasSpilledNeighbor(lr->getVar()->getId())) {
        // Following conditions true:
        // 1. EOT range spilled that needs r112-r127 assignment,
        // 2. Variable is local to a BB,
        // 3. Reserved GRF start + # EOT GRFs fits within total GRFs,
        // 4. Has no spilled neighbor
        //
        // This makes it safe to directly assign a reserved GRF to this
        // variable than spill it.
        lr->setPhyReg(builder.phyregpool.getGreg(kernel.getNumRegTotal() -
                                                 lr->getNumRegNeeded()),
                      0);
        spilledLRs.erase(lrIt);
        break;
      }
    }
  }

  // record RA type
  if (liveAnalysis.livenessClass(G4_GRF)) {
    if (colorHeuristicGRF == ROUND_ROBIN) {
      kernel.setRAType(doBankConflict ? RA_Type::GRAPH_COLORING_RR_BC_RA
                                      : RA_Type::GRAPH_COLORING_RR_RA);
    } else {
      kernel.setRAType(doBankConflict ? RA_Type::GRAPH_COLORING_FF_BC_RA
                                      : RA_Type::GRAPH_COLORING_FF_RA);
    }
  }

#ifdef _DEBUG
  // Verify that spilledLRs has no duplicate
  for (auto item : spilledLRs) {
    unsigned count = 0;
    for (auto checkItem : spilledLRs) {
      if (checkItem == item) {
        vISA_ASSERT(count == 0, "Duplicate entry found in spilledLRs");
        count++;
      }
    }
  }

  // Verify that none of spilledLRs have an allocation
  for (auto lr : spilledLRs) {
    vISA_ASSERT(lr->getPhyReg() == nullptr,
                "Spilled LR contains valid allocation");
  }

  // Verify that all spilled LRs are synced
  for (auto lr : spilledLRs) {
    vISA_ASSERT(lr->isSpilled(),
                "LR not marked as spilled, but inserted in spilledLRs list");
  }

  // Verify if all LRs have either an allocation or are spilled
  for (auto lr : colorOrder) {
    if (!kernel.fg.isPseudoDcl(lr->getDcl())) {
      vISA_ASSERT(lr->isSpilled() || lr->getPhyReg() ||
                      lr->getDcl()->isSpilled(),
                  "Range without allocation and not spilled");
    }
  }
#endif

  return true;
}

template <class REGION_TYPE>
unsigned GlobalRA::getRegionDisp(REGION_TYPE *region, const IR_Builder &irb) {
  unsigned rowOffset = irb.numEltPerGRF<Type_UB>() * region->getRegOff();
  unsigned columnOffset = region->getSubRegOff() * region->getElemSize();
  return rowOffset + columnOffset;
}

void GlobalRA::addEUFusionCallWAInst(G4_INST *inst) {
  if (EUFusionCallWANeeded())
    EUFusionCallWAInsts.insert(inst);
}

void GlobalRA::addEUFusionNoMaskWAInst(G4_BB *BB, G4_INST *Inst) {
  if (EUFusionNoMaskWANeeded() && (BB->getBBType() & G4_BB_NM_WA_TYPE) != 0) {
    EUFusionNoMaskWAInsts.insert(Inst);
    Inst->setNeedPostRA(true);
  }
}

void GlobalRA::removeEUFusionNoMaskWAInst(G4_INST *Inst) {
  if (EUFusionNoMaskWANeeded()) {
    if (EUFusionNoMaskWAInsts.erase(Inst) > 0) {
      Inst->setNeedPostRA(false);
    }
  }
}

unsigned GlobalRA::getRegionByteSize(G4_DstRegRegion *region,
                                     unsigned execSize) {
  unsigned size =
      region->getHorzStride() * region->getElemSize() * (execSize - 1) +
      region->getElemSize();

  return size;
}

#define OWORD_BYTE_SIZE 16

template <class REGION_TYPE>
bool GlobalRA::isUnalignedRegion(REGION_TYPE *region, unsigned execSize) {
  unsigned regionDisp = getRegionDisp(region, builder);
  unsigned regionByteSize = getRegionByteSize(region, execSize);

  if (regionDisp % kernel.numEltPerGRF<Type_UB>() == 0 &&
      regionByteSize % kernel.numEltPerGRF<Type_UB>() == 0) {
    return regionByteSize / kernel.numEltPerGRF<Type_UB>() != 1 &&
           regionByteSize / kernel.numEltPerGRF<Type_UB>() != 2 &&
           regionByteSize / kernel.numEltPerGRF<Type_UB>() != 4;
  }
  return true;
}

bool GlobalRA::shouldPreloadDst(G4_INST *instContext, G4_BB *curBB) {
  // Check for partial and unaligned regions and add pre-load code, if
  // necessary.
  auto spilledRangeRegion = instContext->getDst();
  uint8_t execSize = instContext->getExecSize();

  if (isPartialRegion(spilledRangeRegion, execSize) ||
      isUnalignedRegion(spilledRangeRegion, execSize) ||
      instContext->isPartialWriteForSpill(!curBB->isAllLaneActive(),
                                          useLscForNonStackCallSpillFill)) {
    return true;
  }
  // No pre-load for whole and aligned region writes
  else {
    return false;
  }
}

bool GlobalRA::livenessCandidate(const G4_Declare *decl) const {
  if (decl->getAliasDeclare()) {
    return false;
  }

  if ((G4_GRF & decl->getRegFile())) {
    if ((decl->getRegFile() & G4_INPUT) &&
        decl->getRegVar()->isPhyRegAssigned() && !decl->getRegVar()->isGreg()) {
      return false;
    }
    if (decl->getByteSize() == 0) {
      // regrettably, this can happen for arg/retval pre-defined variable
      return false;
    }
    return true;
  } else {
    return false;
  }
}

void GlobalRA::determineSpillRegSize(unsigned &spillRegSize,
                                     unsigned &indrSpillRegSize) {
  // Iterate over all BBs
  for (auto curBB : kernel.fg) {
    // Iterate over all insts
    for (INST_LIST_ITER inst_it = curBB->begin(), iend = curBB->end();
         inst_it != iend; ++inst_it) {
      unsigned currentSpillRegSize = 0;
      unsigned currentIndrSpillRegSize = 0;

      G4_INST *curInst = (*inst_it);

      if (curInst->isPseudoKill() || curInst->isLifeTimeEnd() ||
          curInst->opcode() == G4_pseudo_fcall ||
          curInst->opcode() == G4_pseudo_fret) {
        continue;
      }

      if (curInst->isSend()) {
        G4_SendDesc *msgDesc = curInst->getMsgDesc();

        unsigned dstSpillRegSize = 0;
        dstSpillRegSize = msgDesc->getDstLenRegs();

        unsigned src0FillRegSize = 0;
        src0FillRegSize = msgDesc->getSrc0LenRegs();

        unsigned src1FillRegSize = 0;
        if (curInst->isSplitSend()) {
          src1FillRegSize = msgDesc->getSrc1LenRegs();
        }

        if (!kernel.fg.builder->useSends()) {
          dstSpillRegSize++;
        }

        currentSpillRegSize =
            dstSpillRegSize + src0FillRegSize + src1FillRegSize;
      } else if (curInst->isDpas()) {
        unsigned dstSpillRegSize = 0;
        G4_DstRegRegion *dst = curInst->getDst();
        if (dst && dst->getBase()->isRegVar()) {
          dstSpillRegSize =
              dst->getBase()->asRegVar()->getDeclare()->getNumRows();
        }

        unsigned srcFillRegSize = 0;
        for (int i = 0, srcNum = curInst->getNumSrc(); i < srcNum; i++) {
          G4_Operand *src = curInst->getSrc(i);

          if (src && src->isSrcRegRegion() &&
              src->asSrcRegRegion()->getBase()->isRegVar()) {
            if (src->asSrcRegRegion()
                    ->getBase()
                    ->asRegVar()
                    ->getDeclare()
                    ->getRegFile() == G4_GRF) {
              unsigned srcSize =
                  src->getBase()->asRegVar()->getDeclare()->getNumRows();
              // FIXME, currently we only use the max src size.
              // To save the spill registers, it's better the space can be
              // determined by checking if the variable is really spilled or
              // not.
              srcFillRegSize += srcSize;
            }
          }
        }
        currentSpillRegSize = srcFillRegSize + dstSpillRegSize;
      } else {
        ORG_REGVAR_VECTOR indrVars;

        unsigned dstSpillRegSize = 0;
        unsigned indrDstSpillRegSize = 0;
        if (G4_Inst_Table[curInst->opcode()].n_dst == 1) {
          G4_DstRegRegion *dst = curInst->getDst();

          if (dst && dst->getBase()->isRegVar()) {
            if (dst->getBase()->asRegVar()->getDeclare()->getRegFile() ==
                G4_GRF) {
              if (dst->isCrossGRFDst(builder)) {
                dstSpillRegSize = 2;
              } else {
                dstSpillRegSize = 1;
              }

              if (shouldPreloadDst(curInst, curBB)) {
                dstSpillRegSize *= 3;
              } else {
                dstSpillRegSize *= 2;
              }

              if (!kernel.fg.builder->useSends()) {
                dstSpillRegSize++;
              }
            } else if (dst->getRegAccess() == IndirGRF) {
              auto pointsToSet =
                  pointsToAnalysis.getAllInPointsTo(dst->getBase()->asRegVar());
              if (pointsToSet != nullptr) {
                for (auto pt : *pointsToSet) {
                  if (pt.var->isRegAllocPartaker() ||
                      ((builder.getOption(vISA_HybridRAWithSpill) ||
                        builder.getOption(vISA_FastCompileRA)) &&
                       livenessCandidate(pt.var->getDeclare()))) {
                    indrVars.push_back(pt.var);
                    indrDstSpillRegSize += pt.var->getDeclare()->getNumRows();
                  }
                }
              }
            }
          }
        }

        unsigned srcFillRegSize = 0;
        unsigned indirSrcFillRegSize = 0;
        // Scan srcs
        for (int i = 0, srcNum = curInst->getNumSrc(); i < srcNum; i++) {
          G4_Operand *src = curInst->getSrc(i);

          if (src && src->isSrcRegRegion() &&
              src->asSrcRegRegion()->getBase()->isRegVar()) {
            if (src->asSrcRegRegion()
                    ->getBase()
                    ->asRegVar()
                    ->getDeclare()
                    ->getRegFile() == G4_GRF) {
              if (src->asSrcRegRegion()->crossGRF(builder)) {
                srcFillRegSize += 2;
              } else {
                srcFillRegSize += 1;
              }
            } else if (src->asSrcRegRegion()->getRegAccess() == IndirGRF) {
              auto pointsToSet = pointsToAnalysis.getAllInPointsTo(
                  src->asSrcRegRegion()->getBase()->asRegVar());
              if (pointsToSet != nullptr) {
                for (auto pt : *pointsToSet) {
                  if (pt.var->isRegAllocPartaker() ||
                      ((builder.getOption(vISA_HybridRAWithSpill) ||
                        builder.getOption(vISA_FastCompileRA)) &&
                       livenessCandidate(pt.var->getDeclare()))) {
                    if (std::find(indrVars.begin(), indrVars.end(), pt.var) ==
                        indrVars.end()) {
                      indrVars.push_back(pt.var);
                      indirSrcFillRegSize += pt.var->getDeclare()->getNumRows();
                    }
                  }
                }
              }
            }
          }
        }

        if (builder.avoidDstSrcOverlap()) {
          currentSpillRegSize = srcFillRegSize + dstSpillRegSize;
        } else {
          currentSpillRegSize = srcFillRegSize > dstSpillRegSize
                                    ? srcFillRegSize
                                    : dstSpillRegSize;
        }
        currentIndrSpillRegSize = indrDstSpillRegSize + indirSrcFillRegSize;
      }

      spillRegSize = std::max(spillRegSize, currentSpillRegSize);
      indrSpillRegSize = std::max(indrSpillRegSize, currentIndrSpillRegSize);
    }
  }
}

void GraphColor::gatherScatterForbiddenWA() {
  if (!liveAnalysis.livenessClass(G4_GRF))
    return;

  // VISA spec supports gather.1 and scatter.1 instructions.
  // But they're not natively supported across platforms. When
  // lowering gather.1 (scatter.1) on unsupported platforms, we
  // use rsp len (msg len) = 2 while actual dst (payload) may be
  // smaller in size. This could cause a problem if dst (payload)
  // gets assigned to r127 as rsp len (msg len) = 2 could make
  // it access beyond last GRF. For eg,
  //
  // VISA:
  //.decl Rsp v_type=G type=q num_elts=1
  //.decl Addr v_type=G type=q num_elts=1
  // svm_gather.8.1 (M1, 1)   Addr   Rsp
  //
  // asm:
  // send.dc1 (1|M0)   r127   r4   null:0   exMSD   MSD   // wr:2+0, rd:2; a64
  // qword gathering read
  //
  // This asm instruction is illegal as Rsp (size = 8 bytes) was assigned r127
  // but send response length = 2.
  //
  // We fix such cases by looking them up and marking upper GRFs
  // as forbidden for allocation.
  for (auto bb : kernel.fg.getBBList()) {
    for (auto inst : bb->getInstList()) {
      if (!inst->isSend() || inst->getExecSize().value >= 8)
        continue;

      // dstLen is actual # of GRFs written based on rb, lb
      // src0Len is actual # of GRFs read based on rb, lb
      // src1Len is actual # of GRFs read based on rb, lb
      unsigned int dstLen = 0, src0Len = 0, src1Len = 0;
      auto sendDst = inst->getDst();
      auto sendHdr = inst->getSrc(0);
      auto sendPayload = inst->getSrc(1);

      auto getLenInGRF = [&](G4_Operand *opnd) {
        unsigned int sz = 0;
        if (opnd && !opnd->isNullReg() && opnd->getTopDcl())
          sz = (opnd->getRightBound() - opnd->getLeftBound() +
                kernel.getGRFSize() - 1) /
               kernel.getGRFSize();
        return sz;
      };

      dstLen = getLenInGRF(sendDst);
      src0Len = getLenInGRF(sendHdr);
      src1Len = getLenInGRF(sendPayload);

      auto sendRspLen = inst->asSendInst()->getMsgDesc()->getDstLenRegs();
      auto headerLen = inst->asSendInst()->getMsgDesc()->getSrc0LenRegs();
      auto payloadLen = inst->asSendInst()->getMsgDesc()->getSrc1LenRegs();

      // For gather.[1|2|4] (scatter.[1|2|4]) difference in actual dst
      // (src0/src1) size and rspLen (msg len/ext msg len) should not exceed 1
      // GRF.
      auto markForbiddenForDcl = [&](unsigned int opndLen, G4_Declare *dcl,
                                     unsigned int lenInSend) {
        if (opndLen > 0 && dcl && dcl->getRegVar() &&
            dcl->getRegVar()->isRegAllocPartaker()) {
          if (lenInSend == (opndLen + 1)) {
            lrs[dcl->getRegVar()->getId()]->setForbidden(
                forbiddenKind::FBD_LASTGRF);
          } else if (lenInSend > opndLen) {
            vISA_ASSERT(false,
                        "mismatch between len in send and that of operand");
          }
        }
      };

      markForbiddenForDcl(dstLen, sendDst->getTopDcl(), sendRspLen);
      markForbiddenForDcl(src0Len, sendHdr->getTopDcl(), headerLen);
      markForbiddenForDcl(src1Len, sendPayload->getTopDcl(), payloadLen);
    }
  }
}

bool GraphColor::regAlloc(bool doBankConflictReduction,
                          bool highInternalConflict, const RPE *rpe) {
  bool useSplitLLRHeuristic = false;
  unsigned doBundleConflictReduction = kernel.getuInt32Option(vISA_enableBundleCR);

  RA_TRACE(std::cout << "\t--# variables: " << liveAnalysis.getNumSelectedVar()
                     << "\n");

  // Copy over alignment for vars inserted by RA
  gra.copyMissingAlignment();

  //
  // create an array of live ranges.
  //
  if (!IncrementalRA::isEnabled(kernel) || lrs.size() == 0) {
    // Create vector of live ranges if we're not using
    // incremental RA or if this is 1st iteration.
    // With incremental RA, live-ranges are created right when
    // new temp var is created in RA.
    createLiveRanges();
  }

  //
  // set the pre-assigned registers
  //
  for (unsigned i = 0; i < numVar; i++) {
    if (lrs[i]->getVar()->getPhyReg()) {
      lrs[i]->setPhyReg(lrs[i]->getVar()->getPhyReg(),
                        lrs[i]->getVar()->getPhyRegOff());
    }

    G4_Declare *dcl = lrs[i]->getDcl();
    if (!useSplitLLRHeuristic) {
      auto dclLR = gra.getLocalLR(dcl);

      if (dclLR != nullptr && dclLR->getSplit()) {
        useSplitLLRHeuristic = true;
      }
    }
  }

  //
  // compute interference matrix
  //
  intf.init();
  intf.computeInterference();

  // If option is true, try to get extra interference info from file
  if (liveAnalysis.livenessClass(G4_GRF) &&
      kernel.getOption(vISA_AddExtraIntfInfo)) {
    getExtraInterferenceInfo();
  }

  TIME_SCOPE(COLORING);
  //
  // compute degree and spill costs for each live range
  //
  if (liveAnalysis.livenessClass(G4_GRF)) {
    computeDegreeForGRF();
  } else {
    computeDegreeForARF();
  }
  computeSpillCosts(useSplitLLRHeuristic, rpe);

  if (kernel.getOption(vISA_DumpRAIntfGraph))
    intf.dumpInterference();
  //
  // determine coloring order
  //
  determineColorOrdering();

  //
  // Set up the sub-reg alignment from declare information
  //
  for (unsigned i = 0; i < numVar; i++) {
    G4_Declare *dcl = lrs[i]->getDcl();

    if (gra.getSubRegAlign(dcl) == Any && !dcl->getIsPartialDcl()) {
      //
      // multi-row, subreg alignment = 16 words
      //
      if (dcl->getNumRows() > 1) {
        gra.setSubRegAlign(lrs[i]->getVar()->getDeclare(),
                           kernel.getGRFAlign());
      }
      //
      // single-row
      //
      else if (gra.getSubRegAlign(lrs[i]->getVar()->getDeclare()) == Any) {
        //
        // set up Odd word or Even word sub reg alignment
        //
        unsigned nbytes = dcl->getNumElems() * TypeSize(dcl->getElemType());
        unsigned nwords = nbytes / G4_WSIZE + nbytes % G4_WSIZE;
        if (nwords >= 2 && lrs[i]->getRegKind() == G4_GRF) {
          gra.setSubRegAlign(lrs[i]->getVar()->getDeclare(), Even_Word);
        }
      }
    }
  }

  gatherScatterForbiddenWA();

  //
  // assign registers for GRFs, GRFs are first attempted to be assigned using
  // round-robin and if it fails then we retry using a first-fit heuristic.
  //
  if (liveAnalysis.livenessClass(G4_GRF)) {
    bool hasStackCall =
        kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc();

    bool willSpill =
        ((builder.getOption(vISA_FastCompileRA) ||
          builder.getOption(vISA_HybridRAWithSpill)) &&
         (!hasStackCall ||
          builder.getOption(vISA_PartitionWithFastHybridRA))) ||
        (kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D &&
         rpe->getMaxRP() >= kernel.getNumRegTotal() + 24);
    if (willSpill) {
      // go straight to first_fit to save compile time since we are definitely
      // spilling we do this for 3D only since with indirect/subroutine the RP
      // pressure can be very unreliable
      // FIXME: due to factors like local split and scalar variables that are
      // not accurately modeled in RP estimate, RA may succeed even when RP is >
      // total #GRF. We should investigate these cases and fix RPE
      assignColors(FIRST_FIT, false, false);
      // assert(requireSpillCode() && "inaccurate GRF pressure estimate");
      return !requireSpillCode();
    }

    if (kernel.getOption(vISA_RoundRobin) && !hasStackCall) {
      if (assignColors(ROUND_ROBIN, doBankConflictReduction,
                       highInternalConflict,
                       doBundleConflictReduction) == false) {
        resetTemporaryRegisterAssignments();
        bool success = assignColors(FIRST_FIT, doBankConflictReduction,
                         highInternalConflict, doBundleConflictReduction);

        if (!success && doBankConflictReduction && isHybrid) {
          return false;
        }

        if (!kernel.getOption(vISA_forceBCR)) {
          if (!success && doBankConflictReduction) {
            resetTemporaryRegisterAssignments();
            assignColors(FIRST_FIT, false, false);
          }
        }
      }
    } else {
      bool success = assignColors(FIRST_FIT, true, highInternalConflict,
                                  doBundleConflictReduction);
      if (!success) {
        resetTemporaryRegisterAssignments();
        assignColors(FIRST_FIT, false, false);
      }
    }
  } else if (liveAnalysis.livenessClass(G4_FLAG)) {
    if (kernel.getOption(vISA_RoundRobin)) {
      if (assignColors(ROUND_ROBIN, false, false) == false) {
        resetTemporaryRegisterAssignments();
        assignColors(FIRST_FIT, false, false);
      }
    } else {
      assignColors(FIRST_FIT, false, false);
    }
  } else {
    // assign registers for ARFs using a first-fit heuristic
    assignColors(FIRST_FIT, false, false);
  }

  return (requireSpillCode() == false);
}

void GraphColor::confirmRegisterAssignments() {
  for (unsigned i = 0; i < numVar; i++) {
    if (lrs[i]->getPhyReg()) {
      if (lrs[i]->getVar()->getPhyReg()) {
        vISA_ASSERT((lrs[i]->getVar()->getPhyReg() == lrs[i]->getPhyReg()),
                    ERROR_GRAPHCOLOR);
      } else {
        lrs[i]->getVar()->setPhyReg(lrs[i]->getPhyReg(),
                                    lrs[i]->getPhyRegOff());
      }
    }
  }
}

void GraphColor::resetTemporaryRegisterAssignments() {
  for (unsigned i = 0; i < numVar; i++) {
    if (lrs[i]->getVar()->getPhyReg() == NULL) {
      lrs[i]->resetPhyReg();
      lrs[i]->resetAllocHint();
      lrs[i]->setSpilled(false);
    }
  }
  spilledLRs.clear();
}

void GraphColor::cleanupRedundantARFFillCode() {
  for (G4_BB *bb : builder.kernel.fg) {
    clearSpillAddrLocSignature();

    for (std::list<G4_INST *>::iterator i = bb->begin(); i != bb->end();) {
      G4_INST *inst = (*i);

      //
      // process writes to spill storage (GRF) of addr regs
      //
      G4_DstRegRegion *dst = inst->getDst();

      if (dst && dst->getBase() && dst->getBase()->isRegVar() &&
          (kernel.fg.isPseudoA0Dcl(dst->getBase()->asRegVar()->getDeclare()) ||
           inst->isPseudoKill())) {
        i++;
        continue;
      }

      if (dst != NULL && dst->getRegAccess() == Direct) {

        if (dst->getBase()->isRegVar() &&
            dst->getBase()->asRegVar()->isRegVarAddrSpillLoc()) {
          pruneActiveSpillAddrLocs(dst, inst->getExecSize(),
                                   inst->getExecType());
        }
        //
        // process writes to (allocated) addr regs
        //
        else if (dst->getBase()->isRegAllocPartaker()) {
          G4_RegVar *addrReg = dst->getBase()->asRegVar();

          if (gra.isAddrFlagSpillDcl(addrReg->getDeclare())) {
            G4_SrcRegRegion *srcRgn = inst->getSrc(0)->asSrcRegRegion();

            if (redundantAddrFill(dst, srcRgn, inst->getExecSize())) {
              std::list<G4_INST *>::iterator j = i++;
              bb->erase(j);
              continue;
            } else {
              updateActiveSpillAddrLocs(dst, srcRgn, inst->getExecSize());
            }
          } else {
            pruneActiveSpillAddrLocs(dst, inst->getExecSize(),
                                     inst->getExecType());
          }
        }
      }

      i++;
    }
  }
}

void GraphColor::pruneActiveSpillAddrLocs(G4_DstRegRegion *dstRegion,
                                          unsigned exec_size,
                                          G4_Type exec_type) {
  if (dstRegion->getBase()->asRegVar()->isRegVarAddrSpillLoc()) {
    vISA_ASSERT(((exec_type == Type_UW || exec_type == Type_W) &&
                 exec_size <= getNumAddrRegisters()) ||
                    (exec_size == 1),
                "Unexpected ADDR spill loc update format!");
    vISA_ASSERT(dstRegion->getRegAccess() == Direct,
                "Unexpected ADDR spill loc");

    G4_RegVarAddrSpillLoc *spillLocReg =
        static_cast<G4_RegVarAddrSpillLoc *>(dstRegion->getBase());
    unsigned startId = spillLocReg->getLocId() + dstRegion->getSubRegOff();
    unsigned endId = startId + exec_size * dstRegion->getHorzStride();

    for (unsigned i = 0, horzStride = dstRegion->getHorzStride();
         i < getNumAddrRegisters(); i += horzStride) {
      if (spAddrRegSig[i] >= startId && spAddrRegSig[i] < endId) {
        spAddrRegSig[i] = 0;
      }
    }
  } else if (dstRegion->getBase()->asRegVar()->isPhyRegAssigned()) {
    G4_RegVar *addrReg = dstRegion->getBase()->asRegVar();
    vISA_ASSERT(addrReg->getPhyReg()->isA0(),
                "Unknown error in ADDR reg spill code cleanup!");
    unsigned startId = addrReg->getPhyRegOff();
    unsigned endId = startId + exec_size * dstRegion->getHorzStride();
    vISA_ASSERT(endId <= getNumAddrRegisters(),
                "Unknown error in ADDR reg spill code cleanup!");

    for (unsigned i = startId; i < endId; i += dstRegion->getHorzStride()) {
      spAddrRegSig[i] = 0;
    }
  } else {
    vISA_ASSERT(false, "Unknown error in ADDR reg spill code cleanup!");
  }
}

void GraphColor::updateActiveSpillAddrLocs(G4_DstRegRegion *tmpDstRegion,
                                           G4_SrcRegRegion *srcRegion,
                                           unsigned exec_size) {
  vISA_ASSERT(
      gra.isAddrFlagSpillDcl(tmpDstRegion->getBase()->asRegVar()->getDeclare()),
      "Unknown error in ADDR reg spill code cleanup!");
  G4_RegVar *addrReg = tmpDstRegion->getBase()->asRegVar();
  vISA_ASSERT(addrReg->getPhyReg()->isA0(),
              "Unknown error in ADDR reg spill code cleanup!");
  unsigned startAddrId = addrReg->getPhyRegOff();
  unsigned endAddrId = startAddrId + exec_size * tmpDstRegion->getHorzStride();
  vISA_ASSERT(endAddrId <= getNumAddrRegisters(),
              "Unknown error in ADDR reg spill code cleanup!");

  vISA_ASSERT(srcRegion->getBase()->asRegVar()->isRegVarAddrSpillLoc(),
              "Unknown error in ADDR reg spill code cleanup!");
  G4_RegVarAddrSpillLoc *spillLocReg =
      static_cast<G4_RegVarAddrSpillLoc *>(srcRegion->getBase());
  unsigned startLocId = spillLocReg->getLocId() + srcRegion->getSubRegOff();

  for (unsigned i = startAddrId, j = startLocId; i < endAddrId;
       i += tmpDstRegion->getHorzStride(),
                j += srcRegion->getRegion()->horzStride) {
    spAddrRegSig[i] = j;
  }
}

bool GraphColor::redundantAddrFill(G4_DstRegRegion *tmpDstRegion,
                                   G4_SrcRegRegion *srcRegion,
                                   unsigned exec_size) {
  bool match = true;

  vISA_ASSERT(
      gra.isAddrFlagSpillDcl(tmpDstRegion->getBase()->asRegVar()->getDeclare()),
      "Unknown error in ADDR reg spill code cleanup!");
  G4_RegVar *addrReg = tmpDstRegion->getBase()->asRegVar();
  vISA_ASSERT(addrReg->getPhyReg()->isA0(),
              "Unknown error in ADDR reg spill code cleanup!");
  unsigned startAddrId = addrReg->getPhyRegOff();
  unsigned endAddrId = startAddrId + exec_size * tmpDstRegion->getHorzStride();
  vISA_ASSERT(endAddrId <= getNumAddrRegisters(),
              "Unknown error in ADDR reg spill code cleanup!");

  vISA_ASSERT(srcRegion->getBase()->asRegVar()->isRegVarAddrSpillLoc(),
              "Unknown error in ADDR reg spill code cleanup!");
  G4_RegVarAddrSpillLoc *spillLocReg =
      static_cast<G4_RegVarAddrSpillLoc *>(srcRegion->getBase());
  unsigned startLocId = spillLocReg->getLocId() + srcRegion->getSubRegOff();

  for (unsigned i = startAddrId, j = startLocId; i < endAddrId;
       i += tmpDstRegion->getHorzStride(),
                j += srcRegion->getRegion()->horzStride) {
    if (spAddrRegSig[i] != j) {
      match = false;
      break;
    }
  }

  return match;
}

unsigned GlobalRA::sendBlockSizeCode(unsigned owordSize) {
  unsigned code;

  switch (owordSize) {
  case 1:
    code = 0;
    break;
  case 2:
    code = 2;
    break;
  case 4:
    code = 3;
    break;
  case 8:
    code = 4;
    break;
  case 16:
    code = 5;
    break;
  default:
    vISA_ASSERT_UNREACHABLE(ERROR_REGALLOC);
    code = 0;
  }

  return code;
}

#define STATELESS_SURFACE_INDEX 0xFF
#define HEADER_PRESENT 0x80000
#define SEND_OWORD_READ_TYPE 0
#define SEND_OWORD_WRITE_TYPE 8
#define SEND_MSG_TYPE_BIT_OFFSET 14
#define SEND_RSP_LENGTH_BIT_OFFSET 20
#define SEND_MSG_LENGTH_BIT_OFFSET 25
#define SEND_DESC_DATA_SIZE_BIT_OFFSET 8

G4_Imm *GlobalRA::createMsgDesc(unsigned owordSize, bool writeType,
                                bool isSplitSend) {
  // If isSplitSend = true then messageLength = 1 and extMesLength =
  // (owordSize/2) GRFs
  unsigned message = STATELESS_SURFACE_INDEX;
  message |= HEADER_PRESENT;
  if (writeType) {
    unsigned messageType = SEND_OWORD_WRITE_TYPE;
    message |= messageType << SEND_MSG_TYPE_BIT_OFFSET;
    unsigned messageLength = 1;
    if (!isSplitSend) {
      messageLength += owordToGRFSize(
          ROUND(owordSize, kernel.numEltPerGRF<Type_UB>() / OWORD_BYTE_SIZE),
          builder);
    }
    message |= messageLength << SEND_MSG_LENGTH_BIT_OFFSET;
  } else {
    unsigned messageType = SEND_OWORD_READ_TYPE;
    message |= messageType << SEND_MSG_TYPE_BIT_OFFSET;
    unsigned responseLength = owordToGRFSize(
        ROUND(owordSize, kernel.numEltPerGRF<Type_UB>() / OWORD_BYTE_SIZE),
        builder);
    message |= responseLength << SEND_RSP_LENGTH_BIT_OFFSET;
    unsigned messageLength = 1;
    message |= messageLength << SEND_MSG_LENGTH_BIT_OFFSET;
  }
  unsigned writeOwordSize = sendBlockSizeCode(owordSize);
  message |= writeOwordSize << SEND_DESC_DATA_SIZE_BIT_OFFSET;
  return builder.createImm(message, Type_UD);
}

void GlobalRA::stackCallProlog() {
  // mov (8) r126.0<1>:ud    r0.0<8;8,1>:ud
  // This sets up the header for oword block r/w used for caller/callee-save

  // Kernel should've already setup r0 in r126.
  // Useful data in r126 is expected to be preserved by all functions.
  if (kernel.fg.getIsStackCallFunc()) {
    if (kernel.getOption(vISA_skipFDE))
      return;

    // emit frame descriptor
    auto payload =
        builder.createHardwiredDeclare(8, Type_UD, kernel.getFPSPGRF(), 0);
    payload->setName(builder.getNameString(24, "FrameDescriptorGRF"));
    auto payloadSrc =
        builder.createSrcRegRegion(payload, builder.getRegionStride1());
    const unsigned execSize = 8;
    G4_DstRegRegion *postDst = builder.createNullDst(Type_UD);
    G4_INST *store = nullptr;
    if (builder.supportsLSC()) {
      auto headerOpnd = getSpillFillHeader(*kernel.fg.builder, nullptr);
      store = builder.createSpill(
          postDst, headerOpnd, payloadSrc, G4_ExecSize(execSize), 1, 0,
          builder.getBESP(), InstOpt_WriteEnable, false);
    } else {
      store =
          builder.createSpill(postDst, payloadSrc, G4_ExecSize(execSize), 1, 0,
                              builder.getBESP(), InstOpt_WriteEnable, false);
    }
    builder.setFDSpillInst(store);
    G4_BB *entryBB = builder.kernel.fg.getEntryBB();
    auto iter = std::find_if(entryBB->begin(), entryBB->end(),
                             [](G4_INST *inst) { return !inst->isLabel(); });
    entryBB->insertBefore(iter, store);

    if (EUFusionCallWANeeded()) {
      auto oldSaveInst = builder.getPartFDSaveInst();
      builder.setPartFDSaveInst(store);
      entryBB->remove(oldSaveInst);
    }
    addEUFusionCallWAInst(store);

    return;
  }

  auto dstRgn = builder.createDstRegRegion(builder.kernel.fg.scratchRegDcl, 1);
  auto srcRgn = builder.createSrcRegRegion(builder.getBuiltinR0(),
                                           builder.getRegionStride1());

  G4_INST *mov = builder.createMov(G4_ExecSize(kernel.numEltPerGRF<Type_UD>()),
                                   dstRgn, srcRgn, InstOpt_WriteEnable, false);

  G4_BB *entryBB = builder.kernel.fg.getEntryBB();
  auto iter = std::find_if(entryBB->begin(), entryBB->end(),
                           [](G4_INST *inst) { return !inst->isLabel(); });
  entryBB->insertBefore(iter, mov);
}

//
// Generate the save code for startReg to startReg+owordSize/2.
//
void GlobalRA::saveRegs(unsigned startReg, unsigned owordSize,
                        G4_Declare *scratchRegDcl, G4_Declare *framePtr,
                        unsigned frameOwordOffset, G4_BB *bb,
                        INST_LIST_ITER insertIt,
                        std::unordered_set<G4_INST *> &group) {
  vISA_ASSERT(builder.getPlatform() >= GENX_SKL,
              "stack call only supported on SKL+");

  if ((useLscForSpillFill && owordSize == 16) || owordSize == 8 ||
      owordSize == 4 || owordSize == 2) {
    // add (1) r126.2<1>:ud    r125.3<0;1,0>:ud    0x2:ud
    // sends (8) null<1>:ud    r126.0    r1.0 ...
    G4_ExecSize execSize = (owordSize > 2) ? g4::SIMD16 : g4::SIMD8;
    unsigned messageLength = GlobalRA::owordToGRFSize(owordSize, builder);
    G4_Declare *msgDcl =
        builder.createTempVar(messageLength * builder.getGenxDataportIOSize(),
                              Type_UD, builder.getGRFAlign(), StackCallStr);
    msgDcl->getRegVar()->setPhyReg(regPool.getGreg(startReg), 0);
    auto sendSrc2 = builder.createSrc(msgDcl->getRegVar(), 0, 0,
                                      builder.getRegionStride1(), Type_UD);
    G4_DstRegRegion *dst =
        builder.createNullDst((execSize > 8) ? Type_UW : Type_UD);
    G4_INST *spillIntrinsic = nullptr;
    if (builder.supportsLSC()) {
      auto headerOpnd = getSpillFillHeader(*kernel.fg.builder, nullptr);
      spillIntrinsic = builder.createSpill(
          dst, headerOpnd, sendSrc2, execSize, messageLength,
          frameOwordOffset / 2, framePtr, InstOpt_WriteEnable, false);
    } else
      spillIntrinsic = builder.createSpill(
          dst, sendSrc2, execSize, messageLength, frameOwordOffset / 2,
          framePtr, InstOpt_WriteEnable, false);
    spillIntrinsic->inheritDIFrom(*insertIt);
    bb->insertBefore(insertIt, spillIntrinsic);
    group.insert(spillIntrinsic);
  } else if ((useLscForSpillFill && owordSize > 16)) {
    saveRegs(startReg, 16, scratchRegDcl, framePtr, frameOwordOffset, bb,
             insertIt, group);
    saveRegs(startReg + GlobalRA::owordToGRFSize(16, builder), owordSize - 16,
             scratchRegDcl, framePtr, frameOwordOffset + 16, bb, insertIt,
             group);
  } else if (owordSize > 8) {
    saveRegs(startReg, 8, scratchRegDcl, framePtr, frameOwordOffset, bb,
             insertIt, group);
    saveRegs(startReg + GlobalRA::owordToGRFSize(8, builder), owordSize - 8,
             scratchRegDcl, framePtr, frameOwordOffset + 8, bb, insertIt,
             group);
  }
  //
  // Split into chunks of sizes 4 and remaining owords.
  //
  else if (owordSize > 4) {
    saveRegs(startReg, 4, scratchRegDcl, framePtr, frameOwordOffset, bb,
             insertIt, group);
    saveRegs(startReg + GlobalRA::owordToGRFSize(4, builder), owordSize - 4,
             scratchRegDcl, framePtr, frameOwordOffset + 4, bb, insertIt,
             group);
  }
  //
  // Split into chunks of sizes 2 and remaining owords.
  //
  else if (owordSize > 2) {
    saveRegs(startReg, 2, scratchRegDcl, framePtr, frameOwordOffset, bb,
             insertIt, group);
    saveRegs(startReg + GlobalRA::owordToGRFSize(2, builder), owordSize - 2,
             scratchRegDcl, framePtr, frameOwordOffset + 2, bb, insertIt,
             group);
  } else {
    vISA_ASSERT(false, ERROR_REGALLOC);
  }
}

//
// Generate the save code for the i/p saveRegs.
//
void GlobalRA::saveActiveRegs(std::vector<bool> &saveRegs, unsigned startReg,
                              unsigned frameOffset, G4_BB *bb,
                              INST_LIST_ITER insertIt,
                              std::unordered_set<G4_INST *> &group) {
  G4_Declare *scratchRegDcl = builder.kernel.fg.scratchRegDcl;
  G4_Declare *framePtr = builder.kernel.fg.framePtrDcl;

  unsigned frameOwordPos = frameOffset;
  unsigned startPos = 0;

  while (startPos < saveRegs.size()) {
    for (; startPos < saveRegs.size() && saveRegs[startPos] == false;
         startPos++)
      ;
    if (startPos < saveRegs.size() && saveRegs[startPos]) {
      unsigned endPos = startPos + 1;
      for (; endPos < saveRegs.size() && saveRegs[endPos] == true; endPos++)
        ;
      unsigned owordSize =
          (endPos - startPos) * GlobalRA::GRFSizeToOwords(1, builder);
      owordSize = std::max(owordSize, GlobalRA::GRFSizeToOwords(1, builder));
      this->saveRegs(startPos + startReg, owordSize, scratchRegDcl, framePtr,
                     frameOwordPos, bb, insertIt, group);
      frameOwordPos += owordSize;
      startPos = endPos;
    }
  }
}

G4_SrcRegRegion *GraphColor::getScratchSurface() const {
  if (builder.hasScratchSurface()) {
    return builder.createSrcRegRegion(builder.getBuiltinScratchSurface(),
                                      builder.getRegionScalar());
  }
  return nullptr; // use stateless access
}

//
// Generate the restore code for startReg to startReg+owordSize/2.
//
void GlobalRA::restoreRegs(unsigned startReg, unsigned owordSize,
                           G4_Declare *scratchRegDcl, G4_Declare *framePtr,
                           unsigned frameOwordOffset, G4_BB *bb,
                           INST_LIST_ITER insertIt,
                           std::unordered_set<G4_INST *> &group, bool caller) {
  //
  // Process chunks of size 8, 4, 2 and 1.
  //
  if ((useLscForSpillFill && owordSize == 16) || owordSize == 8 ||
      owordSize == 4 || owordSize == 2) {
    G4_ExecSize execSize = (owordSize > 2) ? g4::SIMD16 : g4::SIMD8;
    unsigned responseLength = GlobalRA::owordToGRFSize(owordSize, builder);
    G4_Declare *dstDcl =
        builder.createTempVar(responseLength * builder.getGenxDataportIOSize(),
                              Type_UD, builder.getGRFAlign(), StackCallStr);
    if (caller) {
      kernel.callerRestoreDecls.push_back(dstDcl);
    }
    dstDcl->getRegVar()->setPhyReg(regPool.getGreg(startReg), 0);
    G4_DstRegRegion *dstRgn = builder.createDst(
        dstDcl->getRegVar(), 0, 0, 1, (execSize > 8) ? Type_UW : Type_UD);
    G4_INST *fillIntrinsic = nullptr;
    if (builder.supportsLSC()) {
      auto headerOpnd = getSpillFillHeader(*kernel.fg.builder, nullptr);
      fillIntrinsic = builder.createFill(headerOpnd, dstRgn, execSize,
                                         responseLength, frameOwordOffset / 2,
                                         framePtr, InstOpt_WriteEnable, false);
    } else
      fillIntrinsic = builder.createFill(dstRgn, execSize, responseLength,
                                         frameOwordOffset / 2, framePtr,
                                         InstOpt_WriteEnable, false);
    fillIntrinsic->inheritDIFrom(*insertIt);
    bb->insertBefore(insertIt, fillIntrinsic);
    group.insert(fillIntrinsic);
  }
  //
  // Split into chunks of sizes 8 and remaining owords.
  //
  else if ((useLscForSpillFill && owordSize > 16)) {
    restoreRegs(startReg, 16, scratchRegDcl, framePtr, frameOwordOffset, bb,
                insertIt, group, caller);
    restoreRegs(startReg + GlobalRA::owordToGRFSize(16, builder),
                owordSize - 16, scratchRegDcl, framePtr, frameOwordOffset + 16,
                bb, insertIt, group, caller);
  } else if (owordSize > 8) {
    restoreRegs(startReg, 8, scratchRegDcl, framePtr, frameOwordOffset, bb,
                insertIt, group, caller);
    restoreRegs(startReg + GlobalRA::owordToGRFSize(8, builder), owordSize - 8,
                scratchRegDcl, framePtr, frameOwordOffset + 8, bb, insertIt,
                group, caller);
  }
  //
  // Split into chunks of sizes 4 and remaining owords.
  //
  else if (owordSize > 4) {
    restoreRegs(startReg, 4, scratchRegDcl, framePtr, frameOwordOffset, bb,
                insertIt, group, caller);
    restoreRegs(startReg + GlobalRA::owordToGRFSize(4, builder), owordSize - 4,
                scratchRegDcl, framePtr, frameOwordOffset + 4, bb, insertIt,
                group, caller);
  }
  //
  // Split into chunks of sizes 2 and remaining owords.
  //
  else if (owordSize > 2) {
    restoreRegs(startReg, 2, scratchRegDcl, framePtr, frameOwordOffset, bb,
                insertIt, group, caller);
    restoreRegs(startReg + GlobalRA::owordToGRFSize(2, builder), owordSize - 2,
                scratchRegDcl, framePtr, frameOwordOffset + 2, bb, insertIt,
                group, caller);
  } else {
    vISA_ASSERT(false, ERROR_REGALLOC);
  }
}

//
// Generate the restore code for the i/p restoreRegs.
//
void GlobalRA::restoreActiveRegs(std::vector<bool> &restoreRegs,
                                 unsigned startReg, unsigned frameOffset,
                                 G4_BB *bb, INST_LIST_ITER insertIt,
                                 std::unordered_set<G4_INST *> &group,
                                 bool caller) {
  G4_Declare *scratchRegDcl = builder.kernel.fg.scratchRegDcl;
  G4_Declare *framePtr = builder.kernel.fg.framePtrDcl;

  unsigned frameOwordPos = frameOffset;
  unsigned startPos = 0;

  while (startPos < restoreRegs.size()) {
    for (; startPos < restoreRegs.size() && restoreRegs[startPos] == false;
         startPos++)
      ;
    if (startPos < restoreRegs.size() && restoreRegs[startPos]) {
      unsigned endPos = startPos + 1;
      for (; endPos < restoreRegs.size() && restoreRegs[endPos] == true;
           endPos++)
        ;
      unsigned owordSize =
          (endPos - startPos) * GlobalRA::GRFSizeToOwords(1, builder);
      owordSize = std::max(owordSize, GlobalRA::GRFSizeToOwords(1, builder));
      this->restoreRegs(startPos + startReg, owordSize, scratchRegDcl, framePtr,
                        frameOwordPos, bb, insertIt, group, caller);
      frameOwordPos += owordSize;
      startPos = endPos;
    }
  }
}

//
// Optimize the reg footprint so as to reduce the number of "send" instructions
// required for save/restore, at the cost of a little additional save/restore
// memory (if any). Since we are using oword read/write for save/restore, we can
// only read/write only in units of 1, 2 or 4 regs per "send" instruction.
//
void GlobalRA::OptimizeActiveRegsFootprint(std::vector<bool> &saveRegs) {
  unsigned startPos = 0;
  while (startPos < saveRegs.size()) {
    for (; startPos < saveRegs.size() && !saveRegs[startPos]; ++startPos)
      ;
    if (startPos == saveRegs.size()) {
      break;
    }
    if (startPos + 4 <= saveRegs.size()) {
      if (saveRegs[startPos] & saveRegs[startPos + 2] &
          !saveRegs[startPos + 3]) {
        saveRegs[startPos + 1] = saveRegs[startPos + 3] = true;
      } else if (saveRegs[startPos] & saveRegs[startPos + 3]) {
        if (startPos + 4 < saveRegs.size()) {
          if (!saveRegs[startPos + 4]) {
            saveRegs[startPos + 1] = saveRegs[startPos + 2] = true;
          }
        } else {
          saveRegs[startPos + 1] = saveRegs[startPos + 2] = true;
        }
      }
    }
    unsigned winBound =
        std::min(static_cast<unsigned>(saveRegs.size()), startPos + 4);
    for (; startPos < winBound && saveRegs[startPos]; ++startPos)
      ;
  }
}

void GlobalRA::OptimizeActiveRegsFootprint(std::vector<bool> &saveRegs,
                                           std::vector<bool> &retRegs) {
  unsigned startPos = 0;
  while (startPos < saveRegs.size()) {
    for (; startPos < saveRegs.size() && !saveRegs[startPos]; ++startPos)
      ;
    if (startPos == saveRegs.size()) {
      break;
    }
    if (startPos + 4 <= saveRegs.size()) {
      if (saveRegs[startPos] & saveRegs[startPos + 2]) {
        if (!saveRegs[startPos + 1] & !retRegs[startPos + 1]) {
          saveRegs[startPos + 1] = true;
        }
        if (!saveRegs[startPos + 3] & !retRegs[startPos + 3]) {
          saveRegs[startPos + 3] = true;
        }
      } else if (saveRegs[startPos] & saveRegs[startPos + 3]) {
        if (startPos + 4 < saveRegs.size()) {
          if (!saveRegs[startPos + 4]) {
            if (!saveRegs[startPos + 1] & !retRegs[startPos + 1]) {
              saveRegs[startPos + 1] = true;
            }
            if (!saveRegs[startPos + 2] & !retRegs[startPos + 2]) {
              saveRegs[startPos + 2] = true;
            }
          }
        } else {
          if (!saveRegs[startPos + 1] & !retRegs[startPos + 1]) {
            saveRegs[startPos + 1] = true;
          }
          if (!saveRegs[startPos + 2] & !retRegs[startPos + 2]) {
            saveRegs[startPos + 2] = true;
          }
        }
      }
    }
    unsigned winBound =
        std::min(static_cast<unsigned>(saveRegs.size()), startPos + 4);
    for (; startPos < winBound && saveRegs[startPos]; ++startPos)
      ;
  }
}

void GraphColor::getCallerSaveRegisters() {
  unsigned callerSaveNumGRF = builder.kernel.getCallerSaveLastGRF() + 1;

  for (BB_LIST_ITER it = builder.kernel.fg.begin();
       it != builder.kernel.fg.end(); ++it) {
    if ((*it)->isEndWithFCall()) {
      //
      // Determine the caller-save registers per call site.
      //
      gra.callerSaveRegsMap[(*it)].resize(callerSaveNumGRF, false);
      gra.retRegsMap[(*it)].resize(callerSaveNumGRF, false);
      unsigned callerSaveRegCount = 0;
      G4_INST *callInst = (*it)->back();
      unsigned pseudoVCAId =
          builder.kernel.fg.fcallToPseudoDclMap[callInst->asCFInst()]
              .VCA->getRegVar()
              ->getId();
      vISA_ASSERT((*it)->Succs.size() == 1,
                  "fcall basic block cannot have more than 1 successor");

      for (unsigned i = 0; i < numVar; i++) {
        if (i != pseudoVCAId &&
            kernel.fg.isPseudoVCEDcl(lrs[i]->getDcl()) != true &&
            intf.interfereBetween(pseudoVCAId, i) == true) {
          if (!builder.isPreDefArg(lrs[i]->getDcl())) {
            // It is possible that we end up with unallocated spill variable
            // when using new fail safe RA.
            if (lrs[i]->getDcl()->isSpilled() &&
                kernel.getOption(vISA_NewFailSafeRA))
              continue;
            // NOTE: Spilled live ranges should not be caller-save.
            vISA_ASSERT(lrs[i]->getPhyReg()->isGreg(), ERROR_REGALLOC);
            unsigned startReg = lrs[i]->getPhyReg()->asGreg()->getRegNum();
            unsigned endReg = startReg + lrs[i]->getDcl()->getNumRows();
            startReg =
                (startReg < callerSaveNumGRF) ? startReg : callerSaveNumGRF;
            startReg = (startReg > 0) ? startReg : 1;
            endReg = (endReg < callerSaveNumGRF) ? endReg : callerSaveNumGRF;
            endReg = (endReg > 0) ? endReg : 1;
            for (unsigned j = startReg; j < endReg; j++) {
              if (builder.isPreDefRet(lrs[i]->getDcl())) {
                if (gra.retRegsMap[(*it)][j] == false) {
                  gra.retRegsMap[(*it)][j] = true;
                }
              } else {
                if (gra.callerSaveRegsMap[(*it)][j] == false) {
                  gra.callerSaveRegsMap[(*it)][j] = true;
                  callerSaveRegCount++;
                }
              }
            }
          }
        }
      }

      gra.callerSaveRegCountMap[(*it)] = callerSaveRegCount;
      VISA_DEBUG_VERBOSE({
        std::cout << "Caller save size: "
                  << callerSaveRegCount * builder.getGRFSize()
                  << " bytes for fcall at cisa id "
                  << (*it)->back()->getVISAId() << "\n";
      });
    }
  }
}

//
// Add caller save/restore code before/after each stack call.
//
void GlobalRA::addCallerSaveRestoreCode() {
  uint32_t maxCallerSaveSize = 0;

  for (G4_BB *bb : builder.kernel.fg) {
    if (bb->isEndWithFCall()) {
      //
      // Determine the caller-save registers per call site.
      //
      G4_INST *callInst = bb->back();
      G4_BB *afterFCallBB = bb->Succs.front();

      OptimizeActiveRegsFootprint(callerSaveRegsMap[bb], retRegsMap[bb]);

      unsigned callerSaveRegsWritten = 0;
      for (bool csr : callerSaveRegsMap[bb])
        callerSaveRegsWritten += (csr ? 1 : 0);

      INST_LIST_ITER insertSaveIt = bb->end();
      --insertSaveIt, --insertSaveIt;
      while ((*insertSaveIt)->isPseudoKill()) {
        --insertSaveIt;
      }
      vISA_ASSERT((*insertSaveIt)->isCallerSave(), ERROR_REGALLOC);
      INST_LIST_ITER rmIt = insertSaveIt;
      if (insertSaveIt == bb->begin()) {
        insertSaveIt = bb->end();
      }

      if (insertSaveIt != bb->end()) {
        ++insertSaveIt;
      } else {
        insertSaveIt = bb->begin();
      }
      if (callerSaveRegCountMap[bb] > 0) {
        if (builder.kernel.getOption(vISA_GenerateDebugInfo)) {
          builder.kernel.getKernelDebugInfo()->clearOldInstList();
          builder.kernel.getKernelDebugInfo()->setOldInstList(bb);
        }

        saveActiveRegs(callerSaveRegsMap[bb], 0,
                       builder.kernel.fg.callerSaveAreaOffset, bb, insertSaveIt,
                       callerSaveInsts[callInst]);

        // mark instructions for EU Fusion WA
        for (auto save : callerSaveInsts[callInst])
          addEUFusionCallWAInst(save);

        if (builder.kernel.getOption(vISA_GenerateDebugInfo)) {
          auto deltaInstList =
              builder.kernel.getKernelDebugInfo()->getDeltaInstructions(bb);
          for (auto jt : deltaInstList) {
            builder.kernel.getKernelDebugInfo()->addCallerSaveInst(bb, jt);
          }
        }
      }
      bb->erase(rmIt);
      INST_LIST_ITER insertRestIt = afterFCallBB->begin();
      for (; !(*insertRestIt)->isCallerRestore(); ++insertRestIt)
        ;
      if (callerSaveRegCountMap[bb] > 0) {
        if (builder.kernel.getOption(vISA_GenerateDebugInfo)) {
          builder.kernel.getKernelDebugInfo()->clearOldInstList();
          builder.kernel.getKernelDebugInfo()->setOldInstList(afterFCallBB);
        }

        restoreActiveRegs(callerSaveRegsMap[bb], 0,
                          builder.kernel.fg.callerSaveAreaOffset, afterFCallBB,
                          insertRestIt, callerRestoreInsts[callInst], true);

        // mark instructions for EU Fusion WA
        for (auto restore : callerRestoreInsts[callInst])
          addEUFusionCallWAInst(restore);

        if (builder.kernel.getOption(vISA_GenerateDebugInfo)) {
          auto deltaInsts =
              builder.kernel.getKernelDebugInfo()->getDeltaInstructions(
                  afterFCallBB);
          for (auto jt : deltaInsts) {
            builder.kernel.getKernelDebugInfo()->addCallerRestoreInst(bb, jt);
          }
        }
      }
      afterFCallBB->erase(insertRestIt);

      maxCallerSaveSize = std::max(maxCallerSaveSize, callerSaveRegsWritten *
                                                          builder.getGRFSize());
    }
  }

  auto byteOffset =
      builder.kernel.fg.callerSaveAreaOffset * 16 + maxCallerSaveSize;
  builder.kernel.fg.frameSizeInOWord = ROUND(byteOffset, 64) / 16;

  builder.instList.clear();
}

void GraphColor::getCalleeSaveRegisters() {
  unsigned callerSaveNumGRF = builder.kernel.getCallerSaveLastGRF() + 1;
  unsigned numCalleeSaveRegs = builder.kernel.getNumCalleeSaveRegs();

  // Determine the callee-save registers.

  gra.calleeSaveRegs.resize(numCalleeSaveRegs, false);
  gra.calleeSaveRegCount = 0;

  unsigned pseudoVCEId = builder.kernel.fg.pseudoVCEDcl->getRegVar()->getId();
  unsigned stackCallStartReg = builder.kernel.getStackCallStartReg();
  for (unsigned i = 0; i < numVar; i++) {
    if (pseudoVCEId != i && intf.interfereBetween(pseudoVCEId, i)) {
      if (lrs[i]->getPhyReg()) {
        vISA_ASSERT(lrs[i]->getPhyReg()->isGreg(), ERROR_REGALLOC);
        unsigned startReg = lrs[i]->getPhyReg()->asGreg()->getRegNum();
        unsigned endReg = startReg + lrs[i]->getDcl()->getNumRows();
        startReg = (startReg >= callerSaveNumGRF) ? startReg : callerSaveNumGRF;
        startReg =
            (startReg < stackCallStartReg) ? startReg : stackCallStartReg;
        endReg = (endReg >= callerSaveNumGRF) ? endReg : callerSaveNumGRF;
        endReg = (endReg < stackCallStartReg) ? endReg : stackCallStartReg;
        for (unsigned j = startReg; j < endReg; j++) {
          if (gra.calleeSaveRegs[j - callerSaveNumGRF] == false) {
            gra.calleeSaveRegs[j - callerSaveNumGRF] = true;
            gra.calleeSaveRegCount++;
          }
        }
      }
    }
  }
}

//
// Add callee save/restore code at stack call function entry/exit.
//
void GlobalRA::addCalleeSaveRestoreCode() {
  unsigned callerSaveNumGRF = builder.kernel.getCallerSaveLastGRF() + 1;

  OptimizeActiveRegsFootprint(calleeSaveRegs);
  unsigned calleeSaveRegsWritten = 0;
  for (bool b : calleeSaveRegs)
    calleeSaveRegsWritten += (b ? 1 : 0);

  INST_LIST_ITER insertSaveIt = builder.kernel.fg.getEntryBB()->end();
  for (--insertSaveIt; !(*insertSaveIt)->isCalleeSave(); --insertSaveIt)
    ;
  if (calleeSaveRegCount > 0) {
    if (builder.kernel.getOption(vISA_GenerateDebugInfo)) {
      // Store old inst list so we can separate callee save
      // instructions that get inserted.
      builder.kernel.getKernelDebugInfo()->clearOldInstList();
      builder.kernel.getKernelDebugInfo()->setOldInstList(
          builder.kernel.fg.getEntryBB());
    }
    vISA_ASSERT(calleeSaveInsts.size() == 0,
                "Unexpected size of callee save set");
    saveActiveRegs(calleeSaveRegs, callerSaveNumGRF,
                   builder.kernel.fg.calleeSaveAreaOffset,
                   builder.kernel.fg.getEntryBB(), insertSaveIt,
                   calleeSaveInsts);

    if (builder.kernel.getOption(vISA_GenerateDebugInfo)) {
      // Delta of oldInstList and current instList are all
      // callee save instructions.
      auto instList = builder.kernel.getKernelDebugInfo()->getDeltaInstructions(
          builder.kernel.fg.getEntryBB());
      for (auto inst : instList) {
        builder.kernel.getKernelDebugInfo()->addCalleeSaveInst(inst);
      }
    }
  }
  builder.kernel.fg.getEntryBB()->erase(insertSaveIt);
  INST_LIST_ITER insertRestIt = builder.kernel.fg.getUniqueReturnBlock()->end();
  for (--insertRestIt; !(*insertRestIt)->isCalleeRestore(); --insertRestIt)
    ;
  INST_LIST_ITER eraseIt = insertRestIt++;
  if (calleeSaveRegCount > 0) {
    if (builder.kernel.getOption(vISA_GenerateDebugInfo)) {
      // Store old inst list so we can separate callee save
      // instructions that get inserted.
      builder.kernel.getKernelDebugInfo()->clearOldInstList();
      builder.kernel.getKernelDebugInfo()->setOldInstList(
          builder.kernel.fg.getUniqueReturnBlock());
    }
    vISA_ASSERT(calleeRestoreInsts.size() == 0,
                "Unexpected size of callee restore set");
    restoreActiveRegs(calleeSaveRegs, callerSaveNumGRF,
                      builder.kernel.fg.calleeSaveAreaOffset,
                      builder.kernel.fg.getUniqueReturnBlock(), insertRestIt,
                      calleeRestoreInsts, false);

    if (builder.kernel.getOption(vISA_GenerateDebugInfo)) {
      auto instList = builder.kernel.getKernelDebugInfo()->getDeltaInstructions(
          builder.kernel.fg.getUniqueReturnBlock());
      for (auto inst : instList) {
        builder.kernel.getKernelDebugInfo()->addCalleeRestoreInst(inst);
      }
    }
  }
  builder.kernel.fg.getUniqueReturnBlock()->erase(eraseIt);

  builder.instList.clear();

  // mark instructions for EU Fusion WA
  for (auto save : calleeSaveInsts)
    addEUFusionCallWAInst(save);
  for (auto restore : calleeRestoreInsts)
    addEUFusionCallWAInst(restore);

  // caller-save starts after callee-save and is 64-byte aligned
  auto byteOffset = builder.kernel.fg.calleeSaveAreaOffset * 16 +
                    calleeSaveRegsWritten * builder.getGRFSize();
  builder.kernel.fg.callerSaveAreaOffset = ROUND(byteOffset, 64) / 16;
  VISA_DEBUG({
    std::cout << "Callee save size: "
              << calleeSaveRegCount * builder.getGRFSize() << " bytes"
              << "\n";
  });
}

//
// Add code to setup the stack frame in callee.
//
void GlobalRA::addGenxMainStackSetupCode() {
  uint32_t fpInitVal =
      (uint32_t)kernel.getInt32KernelAttr(Attributes::ATTR_SpillMemOffset);
  // FIXME: a potential failure here is that frameSizeInOword is already the
  // offset based on GlobalSratchOffset, which is the value of fpInitVal. So
  // below we generate code to do SP = fpInitVal + frameSize, which does not
  // make sense. It is correct now since when there's stack call, IGC will not
  // use scratch, so fpInitVal will be 0.
  unsigned frameSize = builder.kernel.fg.frameSizeInOWord;
  uint16_t factor = 1;
  if (useLscForSpillFill)
    factor = 16;
  G4_Declare *framePtr = builder.kernel.fg.framePtrDcl;
  G4_Declare *stackPtr = builder.kernel.fg.stackPtrDcl;

  auto entryBB = builder.kernel.fg.getEntryBB();
  auto insertIt = std::find_if(entryBB->begin(), entryBB->end(),
                               [](G4_INST *inst) { return !inst->isLabel(); });
  //
  // FP = spillMemOffset
  //
  {
    G4_DstRegRegion *dst =
        builder.createDst(framePtr->getRegVar(), 0, 0, 1, Type_UD);
    G4_Imm *src = builder.createImm(fpInitVal, Type_UD);
    G4_INST *fpInst =
        builder.createMov(g4::SIMD1, dst, src, InstOpt_WriteEnable, false);
    insertIt = entryBB->insertBefore(insertIt, fpInst);

    setBEFPSetupInst(fpInst);

    if (builder.kernel.getOption(vISA_GenerateDebugInfo)) {
      builder.kernel.getKernelDebugInfo()->setBEFPSetupInst(fpInst);
      builder.kernel.getKernelDebugInfo()->setFrameSize(frameSize * 16);
    }
  }
  //
  // SP = FP + FrameSize (overflow-area offset + overflow-area size)
  //
  {
    G4_DstRegRegion *dst =
        builder.createDst(stackPtr->getRegVar(), 0, 0, 1, Type_UD);
    G4_Imm *src = builder.createImm(fpInitVal + frameSize * factor, Type_UD);
    G4_INST *spIncInst =
        builder.createMov(g4::SIMD1, dst, src, InstOpt_WriteEnable, false);
    entryBB->insertBefore(++insertIt, spIncInst);
  }

  VISA_DEBUG(std::cout << "Total frame size: " << frameSize * 16 << " bytes"
                       << "\n");
}

//
// Add code to setup the stack frame in callee.
//
void GlobalRA::addCalleeStackSetupCode() {
  int frameSize = (int)builder.kernel.fg.frameSizeInOWord;
  uint16_t factor = 1;
  // convert framesize to bytes from oword for LSC
  if (useLscForSpillFill)
    factor = 16;
  G4_Declare *framePtr = builder.kernel.fg.framePtrDcl;
  G4_Declare *stackPtr = builder.kernel.fg.stackPtrDcl;

  vISA_ASSERT(frameSize > 0, "frame size cannot be 0");

  //
  // BE_FP = BE_SP
  // BE_SP += FrameSize
  //
  {
    G4_DstRegRegion *dst =
        builder.createDst(stackPtr->getRegVar(), 0, 0, 1, Type_UD);
    G4_DstRegRegion *fp_dst =
        builder.createDst(framePtr->getRegVar(), 0, 0, 1, Type_UD);
    const RegionDesc *rDesc = builder.getRegionScalar();
    G4_Operand *src0 =
        builder.createSrc(stackPtr->getRegVar(), 0, 0, rDesc, Type_UD);
    G4_Operand *sp_src =
        builder.createSrc(stackPtr->getRegVar(), 0, 0, rDesc, Type_UD);
    G4_Imm *src1 = builder.createImm(frameSize * factor, Type_UD);
    auto createBEFP = builder.createMov(g4::SIMD1, fp_dst, sp_src,
                                        InstOpt_WriteEnable, false);
    createBEFP->addComment("vISA_FP = vISA_SP");
    auto addInst = builder.createBinOp(G4_add, g4::SIMD1, dst, src0, src1,
                                       InstOpt_WriteEnable, false);
    addInst->addComment("vISA_SP += vISA_frameSize");
    G4_BB *entryBB = builder.kernel.fg.getEntryBB();
    auto insertIt =
        std::find(entryBB->begin(), entryBB->end(), getSaveBE_FPInst());
    vISA_ASSERT(insertIt != entryBB->end(), "Can't find BE_FP store inst");

    setBEFPSetupInst(createBEFP);

    if (builder.kernel.getOption(vISA_GenerateDebugInfo)) {
      builder.kernel.getKernelDebugInfo()->setBEFPSetupInst(createBEFP);
      builder.kernel.getKernelDebugInfo()->setFrameSize(frameSize * 16);
    }

    addEUFusionCallWAInst(createBEFP);
    addEUFusionCallWAInst(addInst);

    if (EUFusionCallWANeeded()) {
      builder.kernel.getKernelDebugInfo()->setCallerBEFPSaveInst(createBEFP);
    }

    insertIt++;
    entryBB->insertBefore(insertIt, createBEFP);
    entryBB->insertBefore(insertIt, addInst);
  }

  // Stack is destroyed in function addStoreRestoreToReturn() where part FDE is
  // restored before fret. This is an optimization as 1 SIMD4 instruction
  // restores ret %ip, ret EM, caller's BE_FP, BE_SP.

  builder.instList.clear();

  VISA_DEBUG(std::cout << "\nTotal frame size: " << frameSize * 16
                       << " bytes\n");
}

//
// Add A0 save/restore code for stack calls.
//
void GraphColor::addA0SaveRestoreCode() {
  uint8_t numA0Elements = (uint8_t)getNumAddrRegisters();

  int count = 0;
  for (auto bb : builder.kernel.fg) {
    if (bb->isEndWithFCall()) {
      G4_BB *succ = bb->Succs.front();
      auto fcallInst = bb->back()->asCFInst();
      G4_RegVar *assocPseudoA0 =
          bb->getParent().fcallToPseudoDclMap[fcallInst].A0->getRegVar();

      if (!assocPseudoA0->getPhyReg()) {
        // Insert save/restore code because the pseudo node did not get an
        // allocation
        const char *name = builder.getNameString(20, "SA0_%d", count++);
        G4_Declare *savedDcl =
            builder.createDeclare(name, G4_GRF, numA0Elements, 1, Type_UW);

        {
          //
          // (W) mov (16) TMP_GRF<1>:uw a0.0<16;16,1>:uw
          //
          G4_DstRegRegion *dst =
              builder.createDst(savedDcl->getRegVar(), 0, 0, 1, Type_UW);
          const RegionDesc *rDesc = builder.getRegionStride1();
          G4_Operand *src =
              builder.createSrc(regPool.getAddrReg(), 0, 0, rDesc, Type_UW);
          G4_INST *saveInst = builder.createMov(
              G4_ExecSize(numA0Elements), dst, src, InstOpt_WriteEnable, false);
          INST_LIST_ITER insertIt = std::prev(bb->end());
          bb->insertBefore(insertIt, saveInst);

          gra.addEUFusionCallWAInst(saveInst);
        }

        {
          //
          // (W) mov (16) a0.0<1>:uw TMP_GRF<16;16,1>:uw
          //
          G4_DstRegRegion *dst =
              builder.createDst(regPool.getAddrReg(), 0, 0, 1, Type_UW);
          const RegionDesc *rDesc = builder.getRegionStride1();
          G4_Operand *src =
              builder.createSrc(savedDcl->getRegVar(), 0, 0, rDesc, Type_UW);
          G4_INST *restoreInst = builder.createMov(
              G4_ExecSize(numA0Elements), dst, src, InstOpt_WriteEnable, false);
          auto insertIt =
              std::find_if(succ->begin(), succ->end(),
                           [](G4_INST *inst) { return !inst->isLabel(); });
          succ->insertBefore(insertIt, restoreInst);

          gra.addEUFusionCallWAInst(restoreInst);
        }
      }
    }
  }

  builder.instList.clear();
}

//
// Add Flag save/restore code for stack calls.
//
void GraphColor::addFlagSaveRestoreCode() {
  int count = 0;
  int num32BitFlags = builder.getNumFlagRegisters() / 2;

  // each 32-bit flag gets a declare
  // ToDo: should we use flag ARF directly here?
  std::vector<G4_Declare *> tmpFlags;
  for (int i = 0; i < num32BitFlags; ++i) {
    G4_Declare *tmpFlag = builder.createTempFlag(2);
    tmpFlag->getRegVar()->setPhyReg(regPool.getFlagAreg(i), 0);
    tmpFlags.push_back(tmpFlag);
  }

  for (auto bb : builder.kernel.fg) {
    if (bb->isEndWithFCall()) {
      G4_BB *succ = bb->Succs.front();
      auto fcallInst = bb->back()->asCFInst();
      G4_RegVar *assocPseudoFlag =
          bb->getParent().fcallToPseudoDclMap[fcallInst].Flag->getRegVar();

      if (!assocPseudoFlag->getPhyReg()) {
        // Insert save/restore code because the pseudo node did not get an
        // allocation
        const char *name = builder.getNameString(32, "SFLAG_%d", count++);
        G4_Declare *savedDcl1 =
            builder.createDeclare(name, G4_GRF, num32BitFlags, 1, Type_UD);
        {
          //
          // (W) mov (1) TMP_GRF.0<1>:ud f0.0:ud
          // (W) mov (1) TMP_GRF.1<1>:ud f1.0:ud
          //
          auto createFlagSaveInst = [&](int index) {
            auto flagDcl = tmpFlags[index];
            G4_DstRegRegion *dst =
                builder.createDst(savedDcl1->getRegVar(), 0, index, 1, Type_UD);
            G4_Operand *src = builder.createSrc(
                flagDcl->getRegVar(), 0, 0, builder.getRegionScalar(), Type_UD);
            return builder.createMov(g4::SIMD1, dst, src, InstOpt_WriteEnable,
                                     false);
          };

          auto iter = std::prev(bb->end());
          for (int i = 0; i < num32BitFlags; ++i) {
            auto saveInst = createFlagSaveInst(i);
            bb->insertBefore(iter, saveInst);

            gra.addEUFusionCallWAInst(saveInst);
          }
        }

        {
          //
          // mov (1) f0.0:ud TMP_GRF.0<0;1,0>:ud
          // mov (1) f1.0:ud TMP_GRF.1<0;1,0>:ud
          //
          auto createRestoreFlagInst = [&](int index) {
            auto flagDcl = tmpFlags[index];
            G4_DstRegRegion *dst =
                builder.createDst(flagDcl->getRegVar(), 0, 0, 1, Type_UD);
            const RegionDesc *rDesc = builder.getRegionScalar();
            G4_Operand *src = builder.createSrc(savedDcl1->getRegVar(), 0,
                                                index, rDesc, Type_UD);
            return builder.createMov(g4::SIMD1, dst, src, InstOpt_WriteEnable,
                                     false);
          };
          auto insertIt =
              std::find_if(succ->begin(), succ->end(),
                           [](G4_INST *inst) { return !inst->isLabel(); });
          for (int i = 0; i < num32BitFlags; ++i) {
            auto restoreInst = createRestoreFlagInst(i);
            succ->insertBefore(insertIt, restoreInst);

            gra.addEUFusionCallWAInst(restoreInst);
          }
        }
      }
    }
  }

  builder.instList.clear();
}

void GraphColor::getSaveRestoreRegister() {
  if (!builder.getIsKernel()) {
    getCalleeSaveRegisters();
  }
  getCallerSaveRegisters();
}

//
// Get the forbidden vector size
//
unsigned ForbiddenRegs::getForbiddenVectorSize(G4_RegFileKind regKind) const {
  switch (regKind) {
  case G4_GRF:
  case G4_INPUT:
    return builder.kernel.getNumRegTotal();
  case G4_ADDRESS:
    return getNumAddrRegisters();
  case G4_FLAG:
    return builder.getNumFlagRegisters();
  default:
    vISA_ASSERT_UNREACHABLE("illegal reg file");
    return 0;
  }
}

//
// Get the forbidden vectors of reserved GRFs
// May be reserved for user, stack call, and spill
//
void ForbiddenRegs::generateReservedGRFForbidden(
    unsigned reserveSpillSize) {
  bool hasStackCall = builder.kernel.fg.getHasStackCalls() ||
                      builder.kernel.fg.getIsStackCallFunc();
  uint32_t reservedGRFNum = builder.getuint32Option(vISA_ReservedGRFNum);
  unsigned int stackCallRegSize =
      hasStackCall ? builder.kernel.numReservedABIGRF() : 0;

  // r0 - Forbidden when platform is not 3d
  // rMax, rMax-1, rMax-2 - Forbidden in presence of stack call sites
  forbiddenKind k = forbiddenKind::FBD_RESERVEDGRF;
  unsigned totalGRFNum = builder.kernel.getNumRegTotal();
  forbiddenVec[(size_t)k].resize(getForbiddenVectorSize(G4_GRF));
  forbiddenVec[(size_t)k].clear();

  if (builder.kernel.getKernelType() != VISA_3D ||
      builder.kernel.getOption(vISA_enablePreemption) ||
      reserveSpillSize > 0 || builder.kernel.getOption(vISA_ReserveR0) ||
      builder.kernel.getOption(vISA_PreserveR0InR0)) {
    forbiddenVec[(size_t)k].set(0, true);
  }

  if (builder.kernel.getOption(vISA_enablePreemption)) {
    // r1 is reserved for SIP kernel
    forbiddenVec[(size_t)k].set(1, true);
  }

  unsigned reservedRegSize = stackCallRegSize + reserveSpillSize;
  for (unsigned int i = 0; i < reservedRegSize; i++) {
    forbiddenVec[(size_t)k].set(totalGRFNum - 1 - i, true);
  }

  unsigned largestNoneReservedReg = totalGRFNum - reservedRegSize - 1;
  if (totalGRFNum - reservedRegSize >= totalGRFNum - 16) {
    largestNoneReservedReg = totalGRFNum - 16 - 1;
  }

  if (totalGRFNum - reservedRegSize < reservedGRFNum) {
    vISA_ASSERT(false, "After reservation, there is not enough regiser!");
  }

  for (unsigned int i = 0; i < reservedGRFNum; i++) {
    forbiddenVec[(size_t)k].set(largestNoneReservedReg - i, true);
  }
}

// ETO use only last 16 registers
void ForbiddenRegs::generateEOTGRFForbidden() {
  forbiddenVec[(size_t)forbiddenKind::FBD_EOT].resize(
      getForbiddenVectorSize(G4_GRF));
  forbiddenVec[(size_t)forbiddenKind::FBD_EOT].clear();
  for (unsigned i = 0; i < builder.kernel.getNumRegTotal() - 16; i++) {
    forbiddenVec[(size_t)forbiddenKind::FBD_EOT].set(i, true);
  }
  forbiddenVec[(size_t)forbiddenKind::FBD_EOT] |=
      forbiddenVec[(size_t)forbiddenKind::FBD_RESERVEDGRF];
}

void ForbiddenRegs::generateLastGRFForbidden() {
  forbiddenVec[(size_t)forbiddenKind::FBD_LASTGRF].resize(
      getForbiddenVectorSize(G4_GRF));
  forbiddenVec[(size_t)forbiddenKind::FBD_LASTGRF].clear();
  forbiddenVec[(size_t)forbiddenKind::FBD_LASTGRF].set(
      builder.kernel.getNumRegTotal() - 1, true);
  forbiddenVec[(size_t)forbiddenKind::FBD_LASTGRF] |=
      forbiddenVec[(size_t)forbiddenKind::FBD_RESERVEDGRF];
}

void ForbiddenRegs::generateEOTLastGRFForbidden() {
  forbiddenVec[(size_t)forbiddenKind::FBD_EOTLASTGRF].resize(
      getForbiddenVectorSize(G4_GRF));
  forbiddenVec[(size_t)forbiddenKind::FBD_EOTLASTGRF].clear();
  forbiddenVec[(size_t)forbiddenKind::FBD_EOTLASTGRF] |=
      forbiddenVec[(size_t)forbiddenKind::FBD_EOT];
  forbiddenVec[(size_t)forbiddenKind::FBD_EOTLASTGRF] |=
      forbiddenVec[(size_t)forbiddenKind::FBD_LASTGRF];
}


//
// mark forbidden registers for caller-save pseudo var
//
void ForbiddenRegs::generateCallerSaveGRFForbidden() {
  unsigned int startCalleeSave = builder.kernel.calleeSaveStart();
  unsigned int endCalleeSave =
      startCalleeSave + builder.kernel.getNumCalleeSaveRegs();
  // r60-r124 are caller save regs for SKL
  forbiddenVec[(size_t)forbiddenKind::FBD_CALLERSAVE].resize(
      getForbiddenVectorSize(G4_GRF));
  forbiddenVec[(size_t)forbiddenKind::FBD_CALLERSAVE].clear();
  for (unsigned int i = startCalleeSave; i < endCalleeSave; i++) {
    forbiddenVec[(size_t)forbiddenKind::FBD_CALLERSAVE].set(i, true);
  }
  forbiddenVec[(size_t)forbiddenKind::FBD_CALLERSAVE] |=
      forbiddenVec[(size_t)forbiddenKind::FBD_RESERVEDGRF];
}

//
// mark forbidden registers for callee-save pseudo var
//
void ForbiddenRegs::generateCalleeSaveGRFForbidden() {
  unsigned int numCallerSaveGRFs = builder.kernel.getCallerSaveLastGRF() + 1;
  forbiddenVec[(size_t)forbiddenKind::FBD_CALLEESAVE].resize(
      getForbiddenVectorSize(G4_GRF));
  forbiddenVec[(size_t)forbiddenKind::FBD_CALLEESAVE].clear();
  for (unsigned int i = 1; i < numCallerSaveGRFs; i++) {
    forbiddenVec[(size_t)forbiddenKind::FBD_CALLEESAVE].set(i, true);
  }
  forbiddenVec[(size_t)forbiddenKind::FBD_CALLEESAVE] |=
      forbiddenVec[(size_t)forbiddenKind::FBD_RESERVEDGRF];
}

//
// Add GRF caller/callee save/restore code for stack calls.
// localSpillAreaOwordsize specifices the starting offset of the
// caller/callee-save area in this frame. It is 64-byte aligned.
//
void GlobalRA::addSaveRestoreCode(unsigned localSpillAreaOwordSize) {
  if (builder.getIsKernel()) {
    builder.kernel.fg.callerSaveAreaOffset = localSpillAreaOwordSize;
  } else {
    builder.kernel.fg.calleeSaveAreaOffset = localSpillAreaOwordSize;
    addCalleeSaveRestoreCode();
  }
  addCallerSaveRestoreCode();
  if (builder.getIsKernel()) {
    addGenxMainStackSetupCode();
  } else {
    addCalleeStackSetupCode();
  }
  stackCallProlog();
  builder.instList.clear();
}

//
// If the graph has stack calls, then add the caller-save pseudo code
// immediately before and after the stack call. The pseudo code is either
// converted to actual save/restore code or is eliminated at the end of
// coloringRegAlloc().
//
void GlobalRA::addCallerSavePseudoCode() {
  unsigned retID = 0;

  for (G4_BB *bb : builder.kernel.fg) {
    if (bb->isEndWithFCall()) {
      // GRF caller save/restore
      auto fcallInst = bb->back()->asCFInst();
      G4_Declare *pseudoVCADcl =
          bb->getParent().fcallToPseudoDclMap[fcallInst].VCA;
      G4_DstRegRegion *dst =
          builder.createDst(pseudoVCADcl->getRegVar(), 0, 0, 1, Type_UD);
      G4_INST *saveInst = builder.createInternalIntrinsicInst(
          nullptr, Intrinsic::CallerSave, g4::SIMD1, dst, nullptr, nullptr,
          nullptr, InstOpt_WriteEnable);
      saveInst->inheritDIFrom(fcallInst);
      INST_LIST_ITER callBBIt = bb->end();
      bb->insertBefore(--callBBIt, saveInst);

      auto fcall = builder.getFcallInfo(bb->back());
      vISA_ASSERT(fcall != std::nullopt, "fcall info not found");
      uint16_t retSize = fcall->getRetSize();
      if (retSize > 0) {
        const char *name =
            builder.getNameString(32, "FCALL_RETVAL_%d", retID++);
        auto retDcl = builder.createHardwiredDeclare(
            kernel.numEltPerGRF<Type_UD>() * retSize, Type_UD,
            IR_Builder::ArgRet_Stackcall::Ret, 0);
        retDcl->setName(name);
        addVarToRA(retDcl);
        fcallRetMap.emplace(pseudoVCADcl, retDcl);
      }

      vISA_ASSERT(bb->Succs.size() == 1,
                  "fcall basic block cannot have more than 1 successor node");

      G4_BB *retBB = bb->Succs.front();
      const RegionDesc *rd = builder.getRegionScalar();
      G4_Operand *src =
          builder.createSrc(pseudoVCADcl->getRegVar(), 0, 0, rd, Type_UD);
      INST_LIST_ITER retBBIt = retBB->begin();
      for (; retBBIt != retBB->end() && (*retBBIt)->isLabel(); ++retBBIt)
        ;
      G4_INST *restoreInst = builder.createInternalIntrinsicInst(
          nullptr, Intrinsic::CallerRestore, g4::SIMD1, nullptr, src, nullptr,
          nullptr, InstOpt_WriteEnable);
      restoreInst->inheritDIFrom(fcallInst);
      retBB->insertBefore(retBBIt, restoreInst);
    }
  }
  builder.instList.clear();
}

//
// If the graph has stack calls, then add the callee-save pseudo code at the
// entry/exit blocks of the function. The pseudo code is either converted to
// actual save/restore code or is eliminated at the end of coloringRegAlloc().
//
void GlobalRA::addCalleeSavePseudoCode() {
  G4_Declare *pseudoVCEDcl = builder.kernel.fg.pseudoVCEDcl;

  G4_DstRegRegion *dst =
      builder.createDst(pseudoVCEDcl->getRegVar(), 0, 0, 1, Type_UD);
  auto saveInst = builder.createInternalIntrinsicInst(
      nullptr, Intrinsic::CalleeSave, g4::SIMD1, dst, nullptr, nullptr, nullptr,
      InstOpt_WriteEnable);
  INST_LIST_ITER insertIt = builder.kernel.fg.getEntryBB()->begin();
  for (; insertIt != builder.kernel.fg.getEntryBB()->end() &&
         (*insertIt)->isLabel();
       ++insertIt) { /*  void */
  };
  builder.kernel.fg.getEntryBB()->insertBefore(insertIt, saveInst);

  G4_BB *exitBB = builder.kernel.fg.getUniqueReturnBlock();
  const RegionDesc *rDesc = builder.getRegionScalar();
  G4_Operand *src =
      builder.createSrc(pseudoVCEDcl->getRegVar(), 0, 0, rDesc, Type_UD);
  G4_INST *restoreInst = builder.createInternalIntrinsicInst(
      nullptr, Intrinsic::CalleeRestore, g4::SIMD1, nullptr, src, nullptr,
      nullptr, InstOpt_WriteEnable);
  INST_LIST_ITER exitBBIt = exitBB->end();
  --exitBBIt;
  vISA_ASSERT((*exitBBIt)->isFReturn(), ERROR_REGALLOC);
  exitBB->insertBefore(exitBBIt, restoreInst);
  builder.instList.clear();
}

//
// Insert store r125.[0-4] at entry and restore before return.
// Dst of store will be a hardwired temp at upper end of caller save area.
// This method emits:
// (W) mov (4) SR_BEStack<1>:ud    r125.0<4;4,1>:ud <-- in prolog
// (W) mov (4) r125.0<1>:ud        SR_BEStack<4;4,1>:ud <-- in epilog
void GlobalRA::addStoreRestoreToReturn() {

  unsigned regNum = builder.kernel.getCallerSaveLastGRF();
  unsigned subRegNum = kernel.numEltPerGRF<Type_UD>() - 4;
  oldFPDcl = builder.createHardwiredDeclare(4, Type_UD, regNum, subRegNum);
  oldFPDcl->setName(builder.getNameString(24, "CallerSaveRetIp_BE_FP"));

  G4_DstRegRegion *oldFPDst =
      builder.createDst(oldFPDcl->getRegVar(), 0, 0, 1, Type_UD);
  const RegionDesc *rd = builder.getRegionStride1();
  G4_Operand *oldFPSrc =
      builder.createSrc(oldFPDcl->getRegVar(), 0, 0, rd, Type_UD);

  auto SRDecl =
      builder.createHardwiredDeclare(4, Type_UD, builder.kernel.getFPSPGRF(),
                                     IR_Builder::SubRegs_Stackcall::Ret_IP);
  addVarToRA(SRDecl);
  SRDecl->setName(builder.getNameString(24, "SR_BEStack"));
  G4_DstRegRegion *FPdst =
      builder.createDst(SRDecl->getRegVar(), 0, 0, 1, Type_UD);
  rd = builder.getRegionStride1();
  G4_Operand *FPsrc = builder.createSrc(SRDecl->getRegVar(), 0, 0, rd, Type_UD);

  saveBE_FPInst =
      builder.createMov(g4::SIMD4, oldFPDst, FPsrc, InstOpt_WriteEnable, false);
  saveBE_FPInst->addComment("save vISA SP/FP to temp");
  builder.setPartFDSaveInst(saveBE_FPInst);

  auto entryBB = builder.kernel.fg.getEntryBB();
  auto insertIt = std::find_if(entryBB->begin(), entryBB->end(),
                               [](G4_INST *inst) { return !inst->isLabel(); });
  entryBB->insertBefore(insertIt, saveBE_FPInst);

  auto fretBB = builder.kernel.fg.getUniqueReturnBlock();
  auto iter = std::prev(fretBB->end());
  vISA_ASSERT((*iter)->isFReturn(), "fret BB must end with fret");

  if (!EUFusionCallWANeeded()) {
    restoreBE_FPInst = builder.createMov(g4::SIMD4, FPdst, oldFPSrc,
                                         InstOpt_WriteEnable, false);
    fretBB->insertBefore(iter, restoreBE_FPInst);
  } else {
    // emit frame descriptor
    auto dstDcl =
        builder.createHardwiredDeclare(8, Type_UD, kernel.getFPSPGRF(), 0);
    dstDcl->setName(builder.getNameString(24, "FrameDescriptorGRF"));
    auto dstData = builder.createDstRegRegion(dstDcl, 1);
    const unsigned execSize = 8;
    G4_INST *load = nullptr;
    if (builder.supportsLSC()) {
      auto headerOpnd = getSpillFillHeader(*kernel.fg.builder, nullptr);
      load =
          builder.createFill(headerOpnd, dstData, G4_ExecSize(execSize), 1, 0,
                             builder.getBEFP(), InstOpt_WriteEnable, false);
    } else {
      load = builder.createFill(dstData, G4_ExecSize(execSize), 1, 0,
                                builder.getBEFP(), InstOpt_WriteEnable, false);
    }
    fretBB->insertBefore(iter, load);
    addEUFusionCallWAInst(load);
    restoreBE_FPInst = load;
  }

  restoreBE_FPInst->addComment("restore vISA SP/FP from temp");

  if (builder.kernel.getOption(vISA_GenerateDebugInfo)) {
    builder.kernel.getKernelDebugInfo()->setCallerBEFPRestoreInst(
        restoreBE_FPInst);
    builder.kernel.getKernelDebugInfo()->setCallerSPRestoreInst(
        restoreBE_FPInst);
    if (!EUFusionCallWANeeded())
      builder.kernel.getKernelDebugInfo()->setCallerBEFPSaveInst(saveBE_FPInst);
  }
}

void GlobalRA::reportUndefinedUses(LivenessAnalysis &liveAnalysis, G4_BB *bb,
                                   G4_INST *inst, G4_Declare *referencedDcl,
                                   std::set<G4_Declare *> &defs,
                                   Gen4_Operand_Number opndNum) {
  // Get topmost dcl
  while (referencedDcl->getAliasDeclare()) {
    referencedDcl = referencedDcl->getAliasDeclare();
  }

  if (referencedDcl->getAddressed() == true) {
    // Don't run analysis for addressed opnds.
    // Specifically, we don't analyze following,
    //
    // A0 = &V1
    // r[A0] = 0 <-- V1 indirectly defined
    // ... = V1 <-- Use-before-def warning for V1 skipped due to indirect def
    //
    return;
  }

  if (referencedDcl->getRegVar()->isRegAllocPartaker()) {
    const char *opndName = "";

    if (opndNum == Opnd_pred) {
      opndName = "predicate";
    } else if (opndNum == Opnd_src0) {
      opndName = "src0";
    } else if (opndNum == Opnd_src1) {
      opndName = "src1";
    } else if (opndNum == Opnd_src2) {
      opndName = "src2";
    }

    unsigned id = referencedDcl->getRegVar()->getId();
    if (liveAnalysis.def_in[bb->getId()].test(id) == false &&
        defs.find(referencedDcl) == defs.end()) {
      // Def not found for use so report it
      VISA_DEBUG_VERBOSE({
        std::cout << "Def not found for use " << referencedDcl->getName()
                  << " (" << opndName << ") at CISA offset "
                  << inst->getVISAId() << ", src line " << inst->getLineNo()
                  << ":\n";
        inst->emit(std::cout);
        std::cout << "\n\n";
      });
    }
  }
}

void GlobalRA::updateDefSet(std::set<G4_Declare *> &defs,
                            G4_Declare *referencedDcl) {
  // Get topmost dcl
  while (referencedDcl->getAliasDeclare() != NULL) {
    referencedDcl = referencedDcl->getAliasDeclare();
  }

  defs.insert(referencedDcl);
}

void GlobalRA::detectUndefinedUses(LivenessAnalysis &liveAnalysis,
                                   G4_Kernel &kernel) {
  // This function iterates over each inst and checks whether there is
  // a reaching def for each src operand.
  VISA_DEBUG_VERBOSE({
    std::cout << "\n";
    if (liveAnalysis.livenessClass(G4_FLAG)) {
      std::cout << "=== Uses with reaching def - Flags ===\n";
    } else if (liveAnalysis.livenessClass(G4_ADDRESS)) {
      std::cout << "=== Uses with reaching def - Address ===\n";
    } else {
      std::cout << "=== Uses with reaching def - GRF ===\n";
    }
    if (kernel.getOption(vISA_LocalRA)) {
      std::cout
          << "(Use -nolocalra switch for accurate results of uses without "
             "reaching defs)\n";
    }
  });

  for (G4_BB *bb : kernel.fg) {
    std::set<G4_Declare *> defs;
    std::set<G4_Declare *>::iterator defs_it;
    G4_Declare *referencedDcl = nullptr;

    for (G4_INST *inst : *bb) {
      // Src/predicate opnds are uses
      if (inst->getPredicate() && inst->getPredicate()->getBase() &&
          inst->getPredicate()->getBase()->isRegVar() &&
          inst->getPredicate()->getBase()->isRegAllocPartaker()) {
        referencedDcl = inst->getPredicate()
                            ->asPredicate()
                            ->getBase()
                            ->asRegVar()
                            ->getDeclare();
        reportUndefinedUses(liveAnalysis, bb, inst, referencedDcl, defs,
                            Opnd_pred);
      }

      for (unsigned i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
        G4_Operand *opnd = inst->getSrc(i);

        if (opnd && opnd->isAddrExp() == false && opnd->getBase() &&
            opnd->getBase()->isRegVar() &&
            opnd->getBase()->isRegAllocPartaker()) {
          referencedDcl = opnd->getBase()->asRegVar()->getDeclare();
          reportUndefinedUses(liveAnalysis, bb, inst, referencedDcl, defs,
                              (Gen4_Operand_Number)(i + Opnd_src0));
        }
      }

      // Dst/cond modifier opnds are defs
      if (inst->getCondModBase() && inst->getCondMod()->getBase()->isRegVar() &&
          inst->getCondMod()->getBase()->isRegAllocPartaker()) {
        referencedDcl = inst->getCondMod()
                            ->asCondMod()
                            ->getBase()
                            ->asRegVar()
                            ->getDeclare();
        updateDefSet(defs, referencedDcl);
      }

      if (inst->getDst() && inst->getDst()->getBase() &&
          inst->getDst()->getBase()->isRegVar() &&
          inst->getDst()->getBase()->isRegAllocPartaker()) {
        referencedDcl = inst->getDst()->getBase()->asRegVar()->getDeclare();
        updateDefSet(defs, referencedDcl);
      }
    }
  }

  VISA_DEBUG_VERBOSE(std::cout << "\n\n");
}

void GlobalRA::detectNeverDefinedUses() {
  // Detect variables that are used but never defined in entire CFG.
  // This does not use liveness information.
  // Hold all decls from symbol table as key.
  // Boolean mapped value determines whether the dcl is
  // defined in kernel or not.
  std::map<G4_Declare *, bool> vars;
  std::map<G4_Declare *, bool>::iterator map_it;

  for (auto bb : kernel.fg) {
    for (G4_INST *inst : *bb) {
      G4_Declare *referencedDcl = nullptr;

      if (inst->getDst() && inst->getDst()->getBase() &&
          inst->getDst()->getBase()->isRegVar()) {
        referencedDcl = inst->getDst()->getBaseRegVarRootDeclare();

        // Always insert top-most dcl
        map_it = vars.find(referencedDcl);
        if (map_it == vars.end()) {
          vars.emplace(referencedDcl, true);
        } else {
          map_it->second = true;
        }
      }

      if (inst->getCondModBase() && inst->getCondMod()->getBase()->isRegVar()) {
        referencedDcl = inst->getCondMod()->getBaseRegVarRootDeclare();

        map_it = vars.find(referencedDcl);
        if (map_it == vars.end()) {
          vars.emplace(referencedDcl, true);
        } else {
          map_it->second = true;
        }
      }

      if (inst->getPredicate() && inst->getPredicate()->getBase() &&
          inst->getPredicate()->getBase()->isRegVar()) {
        referencedDcl = inst->getPredicate()->getBaseRegVarRootDeclare();

        // Check whether dcl was already added to list.
        // If not, add it with flag set to false to indicate
        // that a use was found but a def hasnt been seen yet.
        map_it = vars.find(referencedDcl);
        if (map_it == vars.end()) {
          vars.emplace(referencedDcl, false);
        }
      }

      for (unsigned i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
        G4_Operand *opnd = inst->getSrc(i);

        if (opnd && opnd->getBase() && opnd->getBase()->isRegVar()) {
          referencedDcl = opnd->getBaseRegVarRootDeclare();

          map_it = vars.find(referencedDcl);
          if (map_it == vars.end()) {
            vars.emplace(referencedDcl, false);
          }
        }
      }
    }
  }
  VISA_DEBUG_VERBOSE(std::cout
                     << "\n=== Variables used but never defined ===\n\n");

  for (auto dcl : kernel.Declares) {
    while (dcl->getAliasDeclare())
      dcl = dcl->getAliasDeclare();

    map_it = vars.find(dcl);
    if (map_it != vars.end()) {
      if (map_it->second == false && dcl->getRegFile() != G4_INPUT &&
          dcl->getAddressed() == false) {
        // No def found for this non-input variable in
        // entire CFG so report it.
        VISA_DEBUG_VERBOSE({
          std::cout << dcl->getName();
          if (dcl->getRegFile() == G4_GRF) {
            std::cout << " (General)";
          } else if (dcl->getRegFile() == G4_ADDRESS) {
            std::cout << " (Address)";
          } else if (dcl->getRegFile() == G4_FLAG) {
            std::cout << " (Flag)";
          }
          std::cout << "\n";
        });
      }
    }
  }

  VISA_DEBUG_VERBOSE(std::cout << "\n\n");
}

//
//  Check the overlap of two sources' ranges and do range splitting
//  Such as, range1: 0~63, range2: 32~95  --> 0~31,32~63,64~95
//       or, range1: 0~63, range2: 32~63  --> 0~31,32~63
//
VarRange *VarSplit::splitVarRange(VarRange *src1, VarRange *src2,
                                  std::stack<VarRange *> *toDelete) {
  VarRange *new_var_range = nullptr;

  vISA_ASSERT(!(src1->leftBound == src2->leftBound &&
                src1->rightBound == src2->rightBound),
              "Same ranges can not be spiltted");

  if (src1->leftBound > src2->rightBound ||
      src1->rightBound < src2->leftBound) // No overlap
  {
    return NULL;
  }

  unsigned left1 = std::min(src1->leftBound, src2->leftBound); // left
  unsigned right1 = std::max(src1->leftBound, src2->leftBound);

  unsigned left2 = std::min(src1->rightBound, src2->rightBound); // right
  unsigned right2 = std::max(src1->rightBound, src2->rightBound);

  if (left1 == right1) // Same left
  {
    src1->leftBound = left1;
    src1->rightBound = left2;

    src2->leftBound = left2 + 1;
    src2->rightBound = right2;
  } else if (left2 == right2) // Same right
  {
    src1->leftBound = left1;
    src1->rightBound = right1 - 1;
    src2->leftBound = right1;
    src2->rightBound = right2;
  } else // No same boundary
  {
    src1->leftBound = left1; // Left one: in list already
    src1->rightBound = right1 - 1;

    src2->leftBound = left2 + 1; // Right one: keep in list
    src2->rightBound = right2;

    new_var_range = new VarRange;
    new_var_range->leftBound = right1; // Middle one: need add one range object
    new_var_range->rightBound = left2;
    toDelete->push(new_var_range);
  }

  return new_var_range;
}

//
// Scan the range list, Insert the new range into the range list.
// Range splitting is applied if required.
//
void VarSplit::rangeListSpliting(VAR_RANGE_LIST *rangeList, G4_Operand *opnd,
                                 std::stack<VarRange *> *toDelete) {
  VarRange *range = new VarRange;
  range->leftBound = opnd->getLeftBound();
  range->rightBound = opnd->getRightBound();
  toDelete->push(range);

  VAR_RANGE_LIST_ITER it = rangeList->begin();

  // The ranges in the list are ordered from low to high
  while (it != rangeList->end()) {
    if ((*it)->leftBound == range->leftBound &&
        ((*it)->rightBound == range->rightBound)) {
      // Same range exists in the list already
      return;
    }

    if ((*it)->leftBound > range->rightBound) {
      // The range item in the list is on the right of current range, insert it
      // before the postion. Since the whole range is inserted first, all the
      // ranges should be continuous.
      vISA_ASSERT((*it)->leftBound - range->rightBound == 1,
                  "none continuous spliting happened\n");
      rangeList->insert(it, range);
      return;
    }

    // Overlap happened, do splitting.
    //(*lt) is updated to the left range
    //"range" is updated to the right range
    // If "newRange" is not NULL, it's the middle range.
    VarRange *newRange = splitVarRange((*it), range, toDelete);

    // Insert the middle one
    it++;
    if (newRange) {
      it = rangeList->insert(it, newRange);
    }
  }

  rangeList->push_back(range); // Insert the right one

  return;
}

void VarSplit::getHeightWidth(G4_Type type, unsigned numberElements,
                              unsigned short &dclWidth,
                              unsigned short &dclHeight,
                              int &totalByteSize) const {
  dclWidth = 1, dclHeight = 1;
  totalByteSize = numberElements * TypeSize(type);
  if (totalByteSize <= (int)kernel.numEltPerGRF<Type_UB>()) {
    dclWidth = (uint16_t)numberElements;
  } else {
    // here we assume that the start point of the var is the beginning of a GRF?
    // so subregister must be 0?
    dclWidth = kernel.numEltPerGRF<Type_UB>() / TypeSize(type);
    dclHeight = totalByteSize / kernel.numEltPerGRF<Type_UB>();
    if (totalByteSize % kernel.numEltPerGRF<Type_UB>() != 0) {
      dclHeight++;
    }
  }
}

void VarSplit::createSubDcls(G4_Kernel &kernel, G4_Declare *oldDcl,
                             std::vector<G4_Declare *> &splitDclList) {
  if (oldDcl->getByteSize() <= kernel.numEltPerGRF<Type_UB>() ||
      oldDcl->getByteSize() % kernel.numEltPerGRF<Type_UB>()) {
    return;
  }

  int splitVarSize = kernel.getSimdSize() == g4::SIMD8 ? 1 : 2;
  for (unsigned i = 0, bSizePerGRFSize = (oldDcl->getByteSize() /
                                          kernel.numEltPerGRF<Type_UB>());
       i < bSizePerGRFSize; i += splitVarSize) {
    G4_Declare *splitDcl = NULL;
    unsigned leftBound = i * kernel.numEltPerGRF<Type_UB>();
    unsigned rightBound =
        (i + splitVarSize) * kernel.numEltPerGRF<Type_UB>() - 1;
    unsigned short dclWidth = 0;
    unsigned short dclHeight = 0;
    int dclTotalSize = 0;

    getHeightWidth(oldDcl->getElemType(),
                   (rightBound - leftBound + 1) / oldDcl->getElemSize(),
                   dclWidth, dclHeight, dclTotalSize);
    const char *splitDclName = kernel.fg.builder->getNameString(
        16, "split_%d_%s", i, oldDcl->getName());
    splitDcl = kernel.fg.builder->createDeclare(
        splitDclName, G4_GRF, dclWidth, dclHeight, oldDcl->getElemType());
    gra.setSubOffset(splitDcl, leftBound);
    splitDcl->copyAlign(oldDcl);
    gra.copyAlignment(splitDcl, oldDcl);
    unsigned nElementSize =
        (rightBound - leftBound + 1) / oldDcl->getElemSize();
    if ((rightBound - leftBound + 1) % oldDcl->getElemSize()) {
      nElementSize++;
    }
    splitDcl->setTotalElems(nElementSize);
    splitDclList.push_back(splitDcl);
  }

  return;
}

void VarSplit::insertMovesToTemp(IR_Builder &builder, G4_Declare *oldDcl,
                                 G4_Operand *dstOpnd, G4_BB *bb,
                                 INST_LIST_ITER instIter,
                                 std::vector<G4_Declare *> &splitDclList) {
  G4_INST *inst = (*instIter);
  INST_LIST_ITER iter = instIter;
  iter++;

  for (size_t i = 0, size = splitDclList.size(); i < size; i++) {
    G4_Declare *subDcl = splitDclList[i];
    unsigned leftBound = gra.getSubOffset(subDcl);
    unsigned rightBound = leftBound + subDcl->getByteSize() - 1;

    if (!(dstOpnd->getRightBound() < leftBound ||
          rightBound < dstOpnd->getLeftBound())) {
      unsigned maskFlag = (inst->getOption() & 0xFFF010C);
      G4_DstRegRegion *dst = builder.createDstRegRegion(subDcl, 1);
      auto src = builder.createSrc(
          oldDcl->getRegVar(),
          (gra.getSubOffset(subDcl)) / kernel.numEltPerGRF<Type_UB>(), 0,
          builder.getRegionStride1(), oldDcl->getElemType());
      G4_INST *splitInst = builder.createMov(
          G4_ExecSize(subDcl->getTotalElems()), dst, src, maskFlag, false);
      bb->insertBefore(iter, splitInst);
      if (splitInst->isWriteEnableInst() && gra.EUFusionNoMaskWANeeded()) {
        gra.addEUFusionNoMaskWAInst(bb, splitInst);
      }
    }
  }

  return;
}

void VarSplit::insertMovesFromTemp(G4_Kernel &kernel, G4_Declare *oldDcl,
                                   int index, G4_Operand *srcOpnd, int pos,
                                   G4_BB *bb, INST_LIST_ITER instIter,
                                   std::vector<G4_Declare *> &splitDclList) {
  G4_INST *inst = (*instIter);

  int sizeInGRF = (srcOpnd->getRightBound() - srcOpnd->getLeftBound() +
                   kernel.numEltPerGRF<Type_UB>() - 1) /
                  kernel.numEltPerGRF<Type_UB>();
  int splitSize = kernel.getSimdSize() == g4::SIMD8 ? 1 : 2;
  if (sizeInGRF != splitSize) {
    unsigned short dclWidth = 0;
    unsigned short dclHeight = 0;
    int dclTotalSize = 0;
    G4_SrcRegRegion *oldSrc = srcOpnd->asSrcRegRegion();
    getHeightWidth(oldSrc->getType(),
                   (srcOpnd->getRightBound() - srcOpnd->getLeftBound() + 1) /
                       oldSrc->getElemSize(),
                   dclWidth, dclHeight, dclTotalSize);
    const char *newDclName = kernel.fg.builder->getNameString(
        16, "copy_%d_%s", index, oldDcl->getName());
    G4_Declare *newDcl = kernel.fg.builder->createDeclare(
        newDclName, G4_GRF, dclWidth, dclHeight, oldSrc->getType());
    newDcl->copyAlign(oldDcl);
    gra.copyAlignment(newDcl, oldDcl);

    unsigned newLeftBound = 0;

    for (size_t i = 0, size = splitDclList.size(); i < size; i++) {
      G4_Declare *subDcl = splitDclList[i];
      unsigned leftBound = gra.getSubOffset(subDcl);
      unsigned rightBound = leftBound + subDcl->getByteSize() - 1;

      if (!(srcOpnd->getRightBound() < leftBound ||
            rightBound < srcOpnd->getLeftBound())) {

        G4_DstRegRegion *dst = kernel.fg.builder->createDst(
            newDcl->getRegVar(), newLeftBound / kernel.numEltPerGRF<Type_UB>(),
            0, 1, oldSrc->getType());
        newLeftBound += subDcl->getByteSize();
        G4_SrcRegRegion *src = kernel.fg.builder->createSrc(
            subDcl->getRegVar(), 0, 0, kernel.fg.builder->getRegionStride1(),
            oldSrc->getType());
        G4_INST *movInst =
            kernel.fg.builder->createMov(G4_ExecSize(subDcl->getTotalElems()),
                                         dst, src, InstOpt_WriteEnable, false);
        bb->insertBefore(instIter, movInst);
        if (gra.EUFusionNoMaskWANeeded()) {
          gra.addEUFusionNoMaskWAInst(bb, movInst);
        }
      }
    }
    auto newSrc = kernel.fg.builder->createSrcRegRegion(
        oldSrc->getModifier(), Direct, newDcl->getRegVar(), 0,
        oldSrc->getSubRegOff(), oldSrc->getRegion(), newDcl->getElemType());
    inst->setSrc(newSrc, pos);
  } else {
    for (size_t i = 0, size = splitDclList.size(); i < size; i++) {
      G4_Declare *subDcl = splitDclList[i];
      unsigned leftBound = gra.getSubOffset(subDcl);
      unsigned rightBound = leftBound + subDcl->getByteSize() - 1;

      if (!(srcOpnd->getRightBound() < leftBound ||
            rightBound < srcOpnd->getLeftBound())) {
        G4_SrcRegRegion *oldSrc = srcOpnd->asSrcRegRegion();
        G4_SrcRegRegion *newSrc = kernel.fg.builder->createSrcRegRegion(
            oldSrc->getModifier(), Direct, subDcl->getRegVar(), 0,
            oldSrc->getSubRegOff(), oldSrc->getRegion(), oldSrc->getType());
        inst->setSrc(newSrc, pos);
        break;
      }
    }
  }

  return;
}

bool VarSplit::canDoGlobalSplit(IR_Builder &builder, G4_Kernel &kernel,
                                uint32_t sendSpillRefCount) {
  if (!builder.getOption(vISA_GlobalSendVarSplit)) {
    return false;
  }

  if (!builder.getOption(vISA_Debug) && // Not work in debug mode
      kernel.getInt32KernelAttr(Attributes::ATTR_Target) ==
          VISA_3D && // Only works for 3D/OCL/OGL
      sendSpillRefCount) {
    return true;
  }

  return false;
}

void VarSplit::globalSplit(IR_Builder &builder, G4_Kernel &kernel) {
  typedef std::list<
      std::tuple<G4_BB *, G4_Operand *, int, unsigned, INST_LIST_ITER>>
      SPLIT_OPERANDS;
  typedef std::list<std::tuple<G4_BB *, G4_Operand *, int, unsigned,
                               INST_LIST_ITER>>::iterator SPLIT_OPERANDS_ITER;
  typedef std::map<G4_RegVar *, SPLIT_OPERANDS> SPLIT_DECL_OPERANDS;
  typedef std::map<G4_RegVar *, SPLIT_OPERANDS>::iterator
      SPLIT_DECL_OPERANDS_ITER;

  SPLIT_DECL_OPERANDS splitDcls;
  unsigned instIndex = 0;
  int splitSize = kernel.getSimdSize() == g4::SIMD8 ? 1 : 2;
  for (auto bb : kernel.fg) {
    for (INST_LIST_ITER it = bb->begin(), iend = bb->end(); it != iend;
         ++it, ++instIndex) {
      G4_INST *inst = (*it);
      G4_DstRegRegion *dst = inst->getDst();

      if (inst->isLifeTimeEnd() || inst->isPseudoKill()) {
        continue;
      }

      //
      // process send destination operand
      //
      if (inst->isSend() &&
          inst->getMsgDesc()->getDstLenRegs() > (size_t)splitSize &&
          inst->asSendInst()->isDirectSplittableSend()) {
        G4_DstRegRegion *dstrgn = dst;
        G4_Declare *topdcl = GetTopDclFromRegRegion(dstrgn);

        if (topdcl && dstrgn->getRegAccess() == Direct &&
            !topdcl->getAddressed() && topdcl->getRegFile() != G4_INPUT &&
            (dstrgn->getRightBound() - dstrgn->getLeftBound() + 1) ==
                topdcl->getByteSize() &&
            (dstrgn->getRightBound() - dstrgn->getLeftBound()) >
                kernel.numEltPerGRF<Type_UB>()) {
          // The tuple<G4_BB*, G4_Operand*, int pos, unsigned instIndex,
          // INST_LIST_ITER>, these info are tuning and split
          // operand/instruction generation
          splitDcls[topdcl->getRegVar()].push_front(
              make_tuple(bb, dst, 0, instIndex, it));
        }
      }
    }
  }

  instIndex = 0;
  for (auto bb : kernel.fg) {
    for (INST_LIST_ITER it = bb->begin(), end = bb->end(); it != end;
         ++it, ++instIndex) {

      G4_INST *inst = (*it);

      if (inst->isLifeTimeEnd() || inst->isPseudoKill()) {
        continue;
      }

      //
      // process each source operand
      //
      for (unsigned j = 0, numSrc = inst->getNumSrc(); j < numSrc; j++) {
        G4_Operand *src = inst->getSrc(j);

        if (src == NULL) {
          continue;
        }

        if (src->isSrcRegRegion()) {
          G4_Declare *topdcl = GetTopDclFromRegRegion(src);

          if (topdcl && topdcl->getRegFile() != G4_INPUT &&
              !topdcl->getAddressed() &&
              splitDcls.find(topdcl->getRegVar()) != splitDcls.end() &&
              ((src->asSrcRegRegion()->getRightBound() -
                src->asSrcRegRegion()->getLeftBound() + 1) <
               topdcl->getByteSize()) &&
              src->asSrcRegRegion()->getRegAccess() ==
                  Direct) // We don't split the indirect access
          {
            splitDcls[topdcl->getRegVar()].push_back(
                make_tuple(bb, src, j, instIndex, it));
          }
        }
      }
    }
  }

  for (SPLIT_DECL_OPERANDS_ITER it = splitDcls.begin();
       it != splitDcls.end();) {
    unsigned srcIndex = 0xFFFFFFFF;
    unsigned dstIndex = 0;
    SPLIT_DECL_OPERANDS_ITER succIt = it;
    succIt++;
    G4_Declare *topDcl = it->first->getDeclare();
    if (topDcl->getByteSize() <= kernel.numEltPerGRF<Type_UB>() * 2u) {
      splitDcls.erase(it);
      it = succIt;
      continue;
    }

    bool hasSrcOpearnd = false;
    for (SPLIT_OPERANDS_ITER vt = it->second.begin(); vt != it->second.end();
         vt++) {
      G4_BB *bb = nullptr;
      G4_Operand *opnd = nullptr;
      INST_LIST_ITER instIter;
      int pos = 0;
      unsigned iIndex = 0;

      std::tie(bb, opnd, pos, iIndex, instIter) = (*vt);

      if (opnd == nullptr) {
        continue;
      }

      if (opnd->isDstRegRegion()) {
        dstIndex = std::max(dstIndex, iIndex);
      }

      if (opnd->isSrcRegRegion()) {
        srcIndex = std::min(srcIndex, iIndex);
        hasSrcOpearnd = true;
      }
    }

    if (!hasSrcOpearnd ||
        (dstIndex > srcIndex && dstIndex - srcIndex < it->second.size() + 1)) {
      splitDcls.erase(it);
      it = succIt;
      continue;
    }

    it++;
  }

  for (SPLIT_DECL_OPERANDS_ITER it = splitDcls.begin(); it != splitDcls.end();
       it++) {
    G4_Declare *topDcl = it->first->getDeclare();
    std::vector<G4_Declare *> splitDclList;
    splitDclList.clear();

    createSubDcls(kernel, topDcl, splitDclList);
    int srcIndex = 0;
    for (SPLIT_OPERANDS_ITER vt = it->second.begin(); vt != it->second.end();
         vt++) {
      G4_BB *bb = nullptr;
      G4_Operand *opnd = nullptr;
      INST_LIST_ITER instIter;
      int pos = 0;
      unsigned instIndex = 0;
      std::tie(bb, opnd, pos, instIndex, instIter) = (*vt);

      if (opnd == nullptr) {
        continue;
      }

      if (opnd->isDstRegRegion()) {
        insertMovesToTemp(builder, topDcl, opnd, bb, instIter, splitDclList);
      }

      if (opnd->isSrcRegRegion()) {
        insertMovesFromTemp(kernel, topDcl, srcIndex, opnd, pos, bb, instIter,
                            splitDclList);
      }

      srcIndex++;
    }
  }

  return;
}

void VarSplit::localSplit(IR_Builder &builder, G4_BB *bb) {
  class CmpRegVarId {
  public:
    bool operator()(G4_RegVar *first, G4_RegVar *second) const {
      return first->getDeclare()->getDeclId() <
             second->getDeclare()->getDeclId();
    }
  };
  std::map<G4_RegVar *, std::vector<std::pair<G4_Operand *, INST_LIST_ITER>>,
           CmpRegVarId>
      localRanges;
  std::map<G4_RegVar *, std::vector<std::pair<G4_Operand *, INST_LIST_ITER>>,
           CmpRegVarId>::iterator localRangesIt;
  std::map<G4_RegVar *, VarRangeListPackage, CmpRegVarId> varRanges;
  std::map<G4_RegVar *, VarRangeListPackage, CmpRegVarId>::iterator varRangesIt;
  std::stack<VarRange *> toDelete;

  //
  // Iterate instruction in BB from back to front
  //
  for (INST_LIST::reverse_iterator rit = bb->rbegin(), rend = bb->rend();
       rit != rend; ++rit) {
    G4_INST *i = (*rit);
    G4_DstRegRegion *dst = i->getDst();

    if (i->isLifeTimeEnd() || i->isPseudoKill()) {
      continue;
    }

    //
    // process destination operand
    //
    if (dst) {
      // It's RA candidate
      G4_Declare *topdcl = GetTopDclFromRegRegion(dst);

      LocalLiveRange *topdclLR = nullptr;
      // Local only
      if ((topdcl && (topdclLR = gra.getLocalLR(topdcl)) &&
           topdcl->getIsRefInSendDcl() && topdclLR->isLiveRangeLocal()) &&
          topdcl->getRegFile() == G4_GRF) {
        varRangesIt = varRanges.find(topdcl->getRegVar());
        INST_LIST_ITER iterToInsert = rit.base();
        iterToInsert--; // Point to the iterator of current instruction
        if (varRangesIt == varRanges.end()) {
          VarRange *new_range = new VarRange;
          new_range->leftBound = 0;
          new_range->rightBound = topdcl->getByteSize() - 1;
          toDelete.push(new_range);
          varRanges[topdcl->getRegVar()].list.push_back(new_range);
        } else {
          rangeListSpliting(&(varRanges[topdcl->getRegVar()].list), dst,
                            &toDelete);
        }

        localRanges[topdcl->getRegVar()].emplace_back(
            dst, iterToInsert); // Ordered from back to front.
      }
    }

    //
    // process each source operand
    //
    for (unsigned j = 0, numSrc = i->getNumSrc(); j < numSrc; j++) {
      G4_Operand *src = i->getSrc(j);

      if (src == NULL) {
        continue;
      }

      // Local only
      if (src->isSrcRegRegion()) {
        G4_Declare *topdcl = GetTopDclFromRegRegion(src);
        LocalLiveRange *topdclLR = nullptr;

        if (topdcl && (topdclLR = gra.getLocalLR(topdcl)) &&
            topdcl->getIsRefInSendDcl() && topdclLR->isLiveRangeLocal() &&
            topdcl->getRegFile() == G4_GRF) {
          G4_VarBase *base =
              (topdcl != NULL ? topdcl->getRegVar()
                              : src->asSrcRegRegion()->getBase());

          INST_LIST_ITER iterToInsert = rit.base();
          iterToInsert--;

          varRangesIt = varRanges.find(base->asRegVar());
          if (varRangesIt == varRanges.end()) {
            VarRange *new_range = new VarRange;
            new_range->leftBound = 0;
            new_range->rightBound = topdcl->getByteSize() - 1;
            toDelete.push(new_range);
            varRanges[topdcl->getRegVar()].list.push_back(new_range);
          }

          rangeListSpliting(&(varRanges[topdcl->getRegVar()].list), src,
                            &toDelete);

          localRanges[topdcl->getRegVar()].emplace_back(
              src, iterToInsert); // Ordered from back to front.
        }
      }
    }
  }

  // Clean the varaibles without no partial usage, or whose partial live range
  // is too short
  std::map<G4_RegVar *, VarRangeListPackage>::iterator it = varRanges.begin();
  while (it != varRanges.end()) {
    std::map<G4_RegVar *, VarRangeListPackage>::iterator succ_it = it;
    succ_it++;

    // No partial
    if (it->second.list.size() <= 1) {
      varRanges.erase(it);
      it = succ_it;
      continue;
    }

    // If total GRF size divides partial number is less than 16 bytes (half
    // GRF), remove it
    if (((*it->second.list.rbegin())->rightBound -
         (*it->second.list.begin())->leftBound) /
            it->second.list.size() <
        kernel.numEltPerGRF<Type_UW>() * 2 / 2) {
      varRanges.erase(it);
      it = succ_it;
      continue;
    }

    G4_Declare *topDcl = it->first->getDeclare();
    bool aligned = true;
    for (const VarRange *vr : it->second.list) {
      unsigned leftBound = vr->leftBound;
      unsigned rightBound = vr->rightBound;
      int elementSize =
          topDcl->getElemSize() > G4_WSIZE ? topDcl->getElemSize() : G4_WSIZE;
      unsigned short elemsNum = (rightBound - leftBound + 1) / elementSize;

      if (!elemsNum) {
        aligned = false;
        break;
      }

      // TODO: we can merge serveral unaligned sub declares into one aligned.
      // Such as [0-1], [2-63]  --> [0-63]
      if (leftBound % kernel.numEltPerGRF<Type_UW>() ||
          (rightBound + 1) % kernel.numEltPerGRF<Type_UW>()) {
        aligned = false;
        break;
      }
    }

    if (!aligned) {
      varRanges.erase(it);
      it = succ_it;
      continue;
    }

    it = succ_it;
  }

  int splitid = 0;
  for (std::map<G4_RegVar *, VarRangeListPackage>::iterator it =
           varRanges.begin();
       it != varRanges.end(); it++) {
    G4_Declare *topDcl = it->first->getDeclare();
    const char *dclName = topDcl->getName();

    topDcl->setIsSplittedDcl(true);

    // Vertical split: varaible split
    unsigned splitVarNum = 0;
    unsigned pre_rightBound = 0;
    for (VAR_RANGE_LIST_ITER vt = it->second.list.begin();
         vt != it->second.list.end(); vt++) {
      unsigned leftBound = (*vt)->leftBound;
      unsigned rightBound = (*vt)->rightBound;
      int elementSize =
          topDcl->getElemSize() > G4_WSIZE ? topDcl->getElemSize() : G4_WSIZE;
      unsigned short elemsNum = (rightBound - leftBound + 1) / elementSize;

      if (!elemsNum) {
        vASSERT(false);
        pre_rightBound = rightBound;
        continue;
      }

      if (leftBound && pre_rightBound + 1 != leftBound) {
        vASSERT(false);
      }
      pre_rightBound = rightBound;

      std::stringstream nameStrm;
      nameStrm << dclName << "_" << splitid << "_" << leftBound << "_"
               << rightBound << std::ends;
      int nameLen = unsigned(nameStrm.str().length()) + 1;
      const char *name = builder.getNameString(nameLen, "%s_%d_%d_%d", dclName,
                                               splitid, leftBound, rightBound);

      unsigned short dclWidth = 0;
      unsigned short dclHeight = 0;
      int dclTotalSize = 0;

      getHeightWidth(topDcl->getElemType(),
                     (rightBound - leftBound + 1) / topDcl->getElemSize(),
                     dclWidth, dclHeight, dclTotalSize);
      G4_Declare *partialDcl = builder.createDeclare(
          name, G4_GRF, dclWidth, dclHeight, topDcl->getElemType());
      gra.setSubOffset(partialDcl, leftBound);
      partialDcl->setIsPartialDcl(true);
      gra.setSplittedDeclare(partialDcl, topDcl);
      unsigned nElementSize =
          (rightBound - leftBound + 1) / topDcl->getElemSize();
      if ((rightBound - leftBound + 1) % topDcl->getElemSize()) {
        nElementSize++;
      }
      partialDcl->setTotalElems(nElementSize);
      gra.addSubDcl(topDcl, partialDcl);
      splitVarNum++;
      VISA_DEBUG_VERBOSE(std::cout << "==> Sub Declare: " << splitid
                                   << "::" << name << "\n");
      splitid++;
    }
    if (splitVarNum) {
      gra.setSplitVarNum(topDcl, splitVarNum);
    }
  }

  while (toDelete.size() > 0) {
    delete toDelete.top();
    toDelete.pop();
  }

  return;
}

void GlobalRA::addrRegAlloc() {
  uint32_t addrSpillId = 0;
  unsigned maxRAIterations = 10;
  unsigned iterationNo = 0;

  while (iterationNo < maxRAIterations) {
    RA_TRACE(std::cout << "--address RA iteration " << iterationNo << "\n");
    //
    // choose reg vars whose reg file kind is ARF
    //
    LivenessAnalysis liveAnalysis(*this, G4_ADDRESS);
    liveAnalysis.computeLiveness();

    //
    // if no reg var needs to reg allocated, then skip reg allocation
    //
    if (liveAnalysis.getNumSelectedVar() > 0) {
      GraphColor coloring(liveAnalysis, kernel.getNumRegTotal(), false, false);
      if (!coloring.regAlloc(false, false, nullptr)) {
        SpillManager spillARF(*this, coloring.getSpilledLiveRanges(),
                              addrSpillId);
        spillARF.insertSpillCode();
        addrSpillId = spillARF.getNextTempDclId();

        //
        // if new addr temps are created, we need to do RA again so that newly
        // created temps can get registers. If there are no more newly created
        // temps, we then commit reg assignments
        //
        if (spillARF.isAnyNewTempCreated() == false) {
          coloring.confirmRegisterAssignments();
          coloring.cleanupRedundantARFFillCode();
          if ((builder.kernel.fg.getHasStackCalls() ||
               builder.kernel.fg.getIsStackCallFunc())) {
            coloring.addA0SaveRestoreCode();
          }
          break; // no more new addr temps; done with ARF allocation
        }
      } else // successfully allocate register without spilling
      {
        coloring.confirmRegisterAssignments();
        coloring.cleanupRedundantARFFillCode();
        if ((builder.kernel.fg.getHasStackCalls() ||
             builder.kernel.fg.getIsStackCallFunc())) {
          coloring.addA0SaveRestoreCode();
        }
        VISA_DEBUG_VERBOSE(detectUndefinedUses(liveAnalysis, kernel));

        break; // done with ARF allocation
      }
    } else {
      break; // no ARF allocation needed
    }
    kernel.dumpToFile("after.Address_RA." + std::to_string(iterationNo));
    iterationNo++;
  }

  vISA_ASSERT(iterationNo < maxRAIterations, "Address RA has failed.");
}

void GlobalRA::flagRegAlloc() {
  uint32_t flagSpillId = 0;
  unsigned maxRAIterations = 10;
  uint32_t iterationNo = 0;
  bool spillingFlag = false;

  while (iterationNo < maxRAIterations) {
    RA_TRACE(std::cout << "--flag RA iteration " << iterationNo << "\n");

    //
    // choose reg vars whose reg file kind is FLAG
    //
    LivenessAnalysis liveAnalysis(*this, G4_FLAG);
    liveAnalysis.computeLiveness();

    //
    // if no reg var needs to reg allocated, then skip reg allocation
    //
    if (liveAnalysis.getNumSelectedVar() > 0) {
      GraphColor coloring(liveAnalysis, kernel.getNumRegTotal(), false, false);
      if (!coloring.regAlloc(false, false, nullptr)) {
        SpillManager spillFlag(*this, coloring.getSpilledLiveRanges(),
                               flagSpillId);
        spillFlag.insertSpillCode();
        VISA_DEBUG_VERBOSE({
          printf("FLAG Spill inst count: %d\n",
                 spillFlag.getNumFlagSpillStore());
          printf("FLAG Fill inst count: %d\n", spillFlag.getNumFlagSpillLoad());
          printf("*************************\n");
        });
        flagSpillId = spillFlag.getNextTempDclId();

        spillingFlag = true;
        if (spillFlag.isAnyNewTempCreated() == false) {
          coloring.confirmRegisterAssignments();

          if ((builder.kernel.fg.getHasStackCalls() ||
               builder.kernel.fg.getIsStackCallFunc())) {
            coloring.addFlagSaveRestoreCode();
          }
          break;
        }
        builder.getJitInfo()->stats.numFlagSpillStore =
            spillFlag.getNumFlagSpillStore();
        builder.getJitInfo()->stats.numFlagSpillLoad =
            spillFlag.getNumFlagSpillLoad();
      } else // successfully allocate register without spilling
      {
        coloring.confirmRegisterAssignments();
        if ((builder.kernel.fg.getHasStackCalls() ||
             builder.kernel.fg.getIsStackCallFunc())) {
          coloring.addFlagSaveRestoreCode();
        }

        if (spillingFlag && builder.getOption(vISA_FlagSpillCodeCleanup)) {
          CLEAN_NUM_PROFILE clean_num_profile;

          FlagSpillCleanup f(*this);
          f.spillFillCodeCleanFlag(builder, kernel, &clean_num_profile);

#ifdef DEBUG_VERBOSE_ON1
          for (int i = 0; i < 3; i++) {
            printf("Profiler %d Spill clean: %d\n", i,
                   clean_num_profile.spill_clean_num[i]);
            printf("Profiler %d Fill clean: %d\n", i,
                   clean_num_profile.fill_clean_num[i]);
            clean_num += clean_num_profile.spill_clean_num[i];
            clean_num += clean_num_profile.fill_clean_num[i];
          }
          printf("**Flag clean num: %d\n", clean_num);
#endif
        }

        VISA_DEBUG_VERBOSE(detectUndefinedUses(liveAnalysis, kernel));

        break; // done with FLAG allocation
      }
    } else {
      break; // no FLAG allocation needed
    }
    kernel.dumpToFile("after.Flag_RA." + std::to_string(iterationNo));
    iterationNo++;
  }

  vISA_ASSERT(iterationNo < maxRAIterations, "Flag RA has failed.");
}

void GlobalRA::assignRegForAliasDcl() {
  //
  // assign Reg for Alias DCL
  //
  for (G4_Declare *dcl : kernel.Declares) {
    G4_RegVar *AliasRegVar;
    G4_RegVar *CurrentRegVar;
    unsigned tempoffset;

    if (dcl->getAliasDeclare() != NULL) {
      AliasRegVar = dcl->getAliasDeclare()->getRegVar();
      CurrentRegVar = dcl->getRegVar();
      tempoffset = AliasRegVar->getPhyRegOff() *
                       AliasRegVar->getDeclare()->getElemSize() +
                   dcl->getAliasOffset();
      if (AliasRegVar->getPhyReg() != NULL) {
        //
        // alias register assignment for A0
        //
        if (CurrentRegVar->getDeclare()->useGRF()) {
          // if the tempoffset is one grf
          if (tempoffset < kernel.numEltPerGRF<Type_UW>() * 2u) {
            CurrentRegVar->setPhyReg(
                AliasRegVar->getPhyReg(),
                tempoffset / CurrentRegVar->getDeclare()->getElemSize());
          }
          // tempoffset covers several GRFs
          else {
            unsigned addtionalrow =
                tempoffset / (kernel.numEltPerGRF<Type_UW>() * 2);
            unsigned actualoffset =
                tempoffset % (kernel.numEltPerGRF<Type_UW>() * 2);
            bool valid = false;
            unsigned orignalrow = AliasRegVar->ExRegNum(valid);
            vISA_ASSERT(valid == true, ERROR_REGALLOC);
            CurrentRegVar->setPhyReg(
                regPool.getGreg(orignalrow + addtionalrow),
                actualoffset / CurrentRegVar->getDeclare()->getElemSize());
          }
        } else if (CurrentRegVar->getDeclare()->getRegFile() == G4_ADDRESS) {
          vISA_ASSERT(tempoffset < getNumAddrRegisters() * 2,
                      ERROR_REGALLOC); // Must hold tempoffset in one A0 reg
          CurrentRegVar->setPhyReg(
              AliasRegVar->getPhyReg(),
              tempoffset / CurrentRegVar->getDeclare()->getElemSize());
        } else {
          vISA_ASSERT(false, ERROR_REGALLOC);
        }
      } else {
        if (dcl->isSpilled() == false)
          dcl->setSpillFlag();
      }
    }
  }

  return;
}

void GlobalRA::removeSplitDecl() {
  for (auto dcl : kernel.Declares) {
    if (!getSubDclList(dcl).empty()) {
      clearSubDcl(dcl);
      dcl->setIsSplittedDcl(false);
    }
  }

  kernel.Declares.erase(
      std::remove_if(kernel.Declares.begin(), kernel.Declares.end(),
                     [](G4_Declare *dcl) { return dcl->getIsPartialDcl(); }),
      kernel.Declares.end());
}

// FIXME: doBankConflictReduction and highInternalConflict are computed by local
// RA
//        they should be moved to some common code
bool GlobalRA::hybridRA(bool doBankConflictReduction, bool highInternalConflict,
                        LocalRA &lra) {
  RA_TRACE(std::cout << "--hybrid RA--\n");
  uint32_t numOrigDcl = (uint32_t)kernel.Declares.size();
  insertPhyRegDecls();

  LivenessAnalysis liveAnalysis(*this, G4_GRF | G4_INPUT);
  liveAnalysis.computeLiveness();

  if (liveAnalysis.getNumSelectedVar() > 0) {
    RPE rpe(*this, &liveAnalysis);
    rpe.run();

    bool spillLikely =
        kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D &&
        rpe.getMaxRP() >= kernel.getNumRegTotal() - 16;
    if (spillLikely) {
      RA_TRACE(std::cout << "\t--skip hybrid RA due to high pressure: "
                         << rpe.getMaxRP() << "\n");
      kernel.Declares.resize(numOrigDcl);
      lra.undoLocalRAAssignments(false);
      return false;
    }

    GraphColor coloring(liveAnalysis, kernel.getNumRegTotal(), true, false);
    generateForbiddenTemplates(0);
    bool isColoringGood =
        coloring.regAlloc(doBankConflictReduction, highInternalConflict, &rpe);
    if (!isColoringGood) {
      if (!kernel.getOption(vISA_Debug)) {
        // Why?? Keep LRA results when -debug is passed
        kernel.Declares.resize(numOrigDcl);
        lra.undoLocalRAAssignments(false);
      }
      // Restore alignment in case LRA modified it
      copyAlignment();
      return false;
    }
    coloring.confirmRegisterAssignments();

    if (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc()) {
      coloring.getSaveRestoreRegister();
      addSaveRestoreCode(0);
    }

    if (verifyAugmentation) {
      assignRegForAliasDcl();
      computePhyReg();
      verifyAugmentation->verify();
    }
  }

  kernel.setRAType(doBankConflictReduction ? RA_Type::HYBRID_BC_RA
                                           : RA_Type::HYBRID_RA);
  return true;
}

bool canDoHRA(G4_Kernel &kernel) {
  bool ret = true;

  if (kernel.getVarSplitPass()->splitOccured()) {
    ret = false;
  }

  return ret;
}


std::pair<unsigned, unsigned> GlobalRA::reserveGRFSpillReg(GraphColor &coloring) {
  coloring.markFailSafeIter(true);
  unsigned spillRegSize = 0;
  unsigned indrSpillRegSize = 0;

  if (kernel.getOption(vISA_NewFailSafeRA)) {
    spillRegSize = getNumReservedGRFs();
  } else {
    determineSpillRegSize(spillRegSize, indrSpillRegSize);
  }

  vISA_ASSERT(spillRegSize + indrSpillRegSize < kernel.getNumCalleeSaveRegs(),
              "Invalid reserveSpillSize in fail-safe RA!");
  coloring.setTotalGRFRegCount(coloring.getTotalGRFRegCount() -
                               (spillRegSize + indrSpillRegSize));
  return std::make_pair(spillRegSize, indrSpillRegSize);
}

// pre-allocate the bits for forbidden registers which will not be used in
// register assignment.
// Genernal GRF forbidden including:
//   reserved for spill,
//   user reserved,
//   reserved for stack call.
void GlobalRA::generateForbiddenTemplates(unsigned reserveSpillSize) {
  fbdRegs.generateReservedGRFForbidden(reserveSpillSize);
  fbdRegs.generateCallerSaveGRFForbidden();
  fbdRegs.generateCalleeSaveGRFForbidden();
  fbdRegs.generateEOTGRFForbidden();
  fbdRegs.generateLastGRFForbidden();
  fbdRegs.generateEOTLastGRFForbidden();
}

//
// graph coloring entry point.  returns nonzero if RA fails
//
int GlobalRA::coloringRegAlloc() {
  VISA_DEBUG_VERBOSE({
    std::cout << "\n=== Register Allocation ===\n";
    if (builder.getIsKernel() == false) {
      std::cout << "Function: " << kernel.getName() << "\n";
    } else {
      std::cout << "Kernel: " << kernel.getName() << "\n";
    }

    detectNeverDefinedUses();
  });

#ifndef DLL_MODE
  // Points-to analysis is done in RegAlloc.cpp just before constructing
  // GlobalRA instance.
  if (stopAfter("p2a")) {
    pointsToAnalysis.dump(std::cout);
    return VISA_EARLY_EXIT;
  }
#endif // DLL_MODE

  bool hasStackCall =
      kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc();

  // this needs to be called before addr/flag RA since it changes their
  // alignment as well
  fixAlignment();

  {
    TIME_SCOPE(ADDR_FLAG_RA);

    addrRegAlloc();

    flagRegAlloc();
  }
       // LSC messages are used when:
       // a. Stack call is used on PVC+,
       // b. Spill size exceeds what can be represented using hword msg on PVC+

  if (builder.supportsLSC()) {
    useLscForSpillFill = true;
    useLscForNonStackCallSpillFill =
        builder.getOption(vISA_lscNonStackSpill) != 0;
    useLscForScatterSpill = builder.getOption(vISA_scatterSpill);
  }
  //
  // If the graph has stack calls, then add the caller-save/callee-save pseudo
  // declares and code. This currently must be done after flag/addr RA due to
  // the assumption about the location of the pseudo save/restore instructions
  //
  if (hasStackCall) {
    addCallerSavePseudoCode();

    // Only GENX sub-graphs require callee-save code.

    if (builder.getIsKernel() == false) {
      addCalleeSavePseudoCode();
      addStoreRestoreToReturn();
    }

    if (!kernel.getOption(vISA_PreserveR0InR0)) {
      // bind builtinR0 to the reserved stack call ABI GRF so that caller and
      // callee can agree on which GRF to use for r0
      builder.getBuiltinR0()->getRegVar()->setPhyReg(
          builder.phyregpool.getGreg(kernel.getThreadHeaderGRF()), 0);
    }
  }

  if (kernel.getOption(vISA_SpillAnalysis)) {
    spillAnalysis = std::make_unique<SpillAnalysis>();
  }

  // Global linear scan RA
  if (builder.getOption(vISA_LinearScan) &&
      builder.kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D) {
    copyMissingAlignment();
    BankConflictPass bc(*this, false);
    LivenessAnalysis liveAnalysis(*this, G4_GRF | G4_INPUT);
    liveAnalysis.computeLiveness();

    TIME_SCOPE(LINEARSCAN_RA);
    LinearScanRA lra(bc, *this, liveAnalysis);
    int success = lra.doLinearScanRA();
    if (success == VISA_SUCCESS) {
      // TODO: Get correct spillSize from LinearScanRA
      unsigned spillSize = 0;
      expandSpillFillIntrinsics(spillSize);
      assignRegForAliasDcl();
      computePhyReg();
      if (builder.getOption(vISA_verifyLinearScan)) {
        resetGlobalRAStates();
        markGraphBlockLocalVars();
        LivenessAnalysis live(*this, G4_GRF | G4_INPUT, false, true);
        live.computeLiveness();
        GraphColor coloring(live, kernel.getNumRegTotal(), false, false);
        coloring.createLiveRanges();
        const auto &lrs = coloring.getLiveRanges();
        Interference intf(&live, lrs, live.getNumSelectedVar(),
                          live.getNumSplitStartID(), live.getNumSplitVar(),
                          *this);
        intf.init();
        intf.computeInterference();

        if (kernel.getOption(vISA_DumpRAIntfGraph))
          intf.dumpInterference();
        intf.linearScanVerify();
      }
      return VISA_SUCCESS;
    }

    if (success == VISA_SPILL) {
      return VISA_SPILL;
    }
  } else if (builder.getOption(vISA_LocalRA) && (!hasStackCall)) {
    copyMissingAlignment();
    BankConflictPass bc(*this, false);
    LocalRA lra(bc, *this);
    bool success = lra.localRA();
    if (!success && !builder.getOption(vISA_HybridRAWithSpill)) {
      if (canDoHRA(kernel)) {
        success = hybridRA(lra.doHybridBCR(), lra.hasHighInternalBC(), lra);
      } else {
        RA_TRACE(
            std::cout << "\t--skip HRA due to var split. undo LRA results\n");
        lra.undoLocalRAAssignments(false);
      }
    }
    if (success) {
      // either local or hybrid RA succeeds
      assignRegForAliasDcl();
      computePhyReg();
      return VISA_SUCCESS;
    }
    if (builder.getOption(vISA_HybridRAWithSpill)) {
      insertPhyRegDecls();
    }
  }

  startTimer(TimerID::GRF_GLOBAL_RA);
  unsigned maxRAIterations = 10;
  unsigned iterationNo = 0;

  int globalScratchOffset =
      kernel.getInt32KernelAttr(Attributes::ATTR_SpillMemOffset);
  bool useScratchMsgForSpill =
      !hasStackCall &&
      (globalScratchOffset < (int)(SCRATCH_MSG_LIMIT * 0.6)
       // useScratchMsgForSpill is true for
       // * scratch msg
       // * LSC msg
       // Spill insertion module decides whether to expand a fill/spill to
       // scratch or LSC depending on spill offset. oword is supported for PVC
       // but it is not emitted in favor of LSC.
       || builder.supportsLSC());
  bool enableSpillSpaceCompression =
      builder.getOption(vISA_SpillSpaceCompression);

  uint32_t nextSpillOffset = 0;
  uint32_t scratchOffset = 0;

  if (kernel.fg.getIsStackCallFunc()) {
    // Allocate space to store Frame Descriptor
    nextSpillOffset += 32;
    scratchOffset += 32;
  }

  uint32_t GRFSpillFillCount = 0;
  uint32_t sendAssociatedGRFSpillFillCount = 0;
  unsigned fastCompileIter = 1;
  bool fastCompile =
      (builder.getOption(vISA_FastCompileRA) ||
       builder.getOption(vISA_HybridRAWithSpill)) &&
      (!hasStackCall || builder.getOption(vISA_PartitionWithFastHybridRA));

  if (fastCompile) {
    fastCompileIter = 0;
  }

  if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_CM) {
    maxRAIterations = 12;
  }

  unsigned failSafeRAIteration =
      (builder.getOption(vISA_FastSpill) || fastCompile)
          ? fastCompileIter
          : builder.getuint32Option(vISA_FailSafeRALimit);
  if (failSafeRAIteration == 0) {
    builder.getSpillFillHeader();
    builder.getOldA0Dot2Temp();
    if (builder.hasScratchSurface()) {
      vISA_ASSERT(builder.instList.empty(),
                  "Inst list should be empty at this point before creating "
                  "instruction that initializes SSO");
      builder.initScratchSurfaceOffset();
      if (!builder.instList.empty()) {
        // If SSO is not yet initialized, insert the created
        // instruction into the entry BB.
        auto entryBB = builder.kernel.fg.getEntryBB();
        auto iter =
            std::find_if(entryBB->begin(), entryBB->end(),
                         [](G4_INST *inst) { return !inst->isLabel(); });
        entryBB->splice(iter, builder.instList);
      }
    }
    // BuiltinR0 may be spilled which is not allowed.
    // FIXME: BuiltinR0 spill cost has been set to MAX already,
    // keep spilling means there is some issue in cost model
    builder.getBuiltinR0()->setLiveOut();
    builder.getBuiltinR0()->getRegVar()->setPhyReg(
        builder.phyregpool.getGreg(0), 0);
  }
  bool rematDone = false, alignedScalarSplitDone = false;
  bool reserveSpillReg = false;
  VarSplit splitPass(*this);
  DynPerfModel perfModel(kernel);
  FINALIZER_INFO *jitInfo = builder.getJitInfo();

  // Reset state of incremental RA here as we move from hybrid RA
  // to global RA. Note that when moving from flag->address or from
  // address->GRF RA, we don't need to explicitly reset state because
  // incremental RA can deduce we're moving to RA for different
  // variable class. But it cannot deduce so when moving from hybrid
  // to global RA.
  incRA.moveFromHybridToGlobalGRF();

  while (iterationNo < maxRAIterations) {
    jitInfo->statsVerbose.RAIterNum++;
    if (builder.getOption(vISA_DynPerfModel)) {
      perfModel.NumRAIters++;
    }
    RA_TRACE(std::cout << "--GRF RA iteration " << iterationNo << "--"
                       << kernel.getName() << "\n");
    setIterNo(iterationNo);

    if (!builder.getOption(vISA_HybridRAWithSpill)) {
      resetGlobalRAStates();
    }

    if (builder.getOption(vISA_clearScratchWritesBeforeEOT) &&
        (globalScratchOffset + nextSpillOffset) > 0) {
      // we need to set r0 be live out for this WA
      builder.getBuiltinR0()->setLiveOut();
    }

    // Identify the local variables to speedup following analysis
    if (!builder.getOption(vISA_HybridRAWithSpill)) {
      markGraphBlockLocalVars();
    }

    if (kernel.getOption(vISA_SpillAnalysis)) {
      spillAnalysis->Clear();
    }

    // Do variable splitting in each iteration
    if (builder.getOption(vISA_LocalDeclareSplitInGlobalRA)) {
      RA_TRACE(std::cout << "\t--split local send--\n");
      for (auto bb : kernel.fg) {
        if (bb->isSendInBB()) {
          splitPass.localSplit(builder, bb);
        }
      }
    }

    bool doBankConflictReduction = false;
    bool highInternalConflict =
        false; // this is set by setupBankConflictsForKernel

    if (builder.getOption(vISA_LocalBankConflictReduction) &&
        builder.hasBankCollision()) {
      bool reduceBCInRR = false;
      bool reduceBCInTAandFF = false;
      BankConflictPass bc(*this, true);

      reduceBCInRR = bc.setupBankConflictsForKernel(
          true, reduceBCInTAandFF, SECOND_HALF_BANK_START_GRF * 2,
          highInternalConflict);
      doBankConflictReduction = reduceBCInRR && reduceBCInTAandFF;
    }

    bool allowAddrTaken = builder.getOption(vISA_FastSpill) || fastCompile ||
                          !kernel.getHasAddrTaken();
    if (builder.getOption(vISA_FailSafeRA) &&
        kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D &&
        !hasStackCall &&
        ((iterationNo == maxRAIterations - 1) ||
         (allowAddrTaken && iterationNo == failSafeRAIteration))) {
      RA_TRACE(std::cout << "\t--enable failSafe RA\n");
      reserveSpillReg = true;

      if (builder.hasScratchSurface() && !hasStackCall) {
        // Since this is fail safe RA iteration, we ensure the 2 special
        // variables are created before coloring so spill code can use
        // them, if needed.
        auto a0Dot2Temp = kernel.fg.builder->getOldA0Dot2Temp();
        addVarToRA(a0Dot2Temp);
        if (builder.supportsLSC()) {
          auto spillFillHdr = kernel.fg.builder->getSpillFillHeader();
          addVarToRA(spillFillHdr);
        }
      }
    }

    LivenessAnalysis liveAnalysis(*this, G4_GRF | G4_INPUT);
    liveAnalysis.computeLiveness();
#ifndef DLL_MODE
    if (stopAfter("Global_RA_liveness")) {
      return VISA_EARLY_EXIT;
    }
#endif // DLL_MODE
    if (builder.getOption(vISA_dumpLiveness)) {
      liveAnalysis.dump();
    }
    if (jitInfo->statsVerbose.RAIterNum == 1) {
      if (builder.getOption(vISA_DumpPerfStatsVerbose)) {
        jitInfo->statsVerbose.varNum = liveAnalysis.getNumSelectedVar();
        jitInfo->statsVerbose.globalVarNum =
            liveAnalysis.getNumSelectedGlobalVar();
      }
      RA_TRACE(std::cout << "\t--# global variable: "
                         << jitInfo->statsVerbose.globalVarNum << "\n");
    }
#ifdef DEBUG_VERBOSE_ON
    emitFGWithLiveness(liveAnalysis);
#endif
    //
    // if no reg var needs to reg allocated, then skip reg allocation
    //
    if (liveAnalysis.getNumSelectedVar() > 0) {
      if (builder.getOption(vISA_DumpUndefUsesFromLiveness) &&
          iterationNo == 0 && !rematDone) {
        liveAnalysis.reportUndefinedUses();
      }
      // force spill should be done only for the 1st iteration
      bool forceSpill =
          iterationNo > 0 ? false : builder.getOption(vISA_ForceSpills);
      RPE rpe(*this, &liveAnalysis);
      if (!fastCompile) {
        rpe.run();
        if (builder.getOption(vISA_DumpPerfStatsVerbose) &&
            builder.getJitInfo()->statsVerbose.RAIterNum == 1) {
          builder.getJitInfo()->statsVerbose.maxRP = rpe.getMaxRP();
        }
        if (builder.getOption(vISA_DumpPerfStats)) {
          builder.getJitInfo()->stats.maxGRFPressure = rpe.getMaxRP();
        }
      }
      GraphColor coloring(liveAnalysis, kernel.getNumRegTotal(), false,
                          forceSpill);

      if (builder.getOption(vISA_dumpRPE) && iterationNo == 0 && !rematDone) {
        // dump pressure the first time we enter global RA
        coloring.dumpRegisterPressure();
      }

      // Get the size of register which are reserved for spill
      unsigned spillRegSize = 0;
      unsigned indrSpillRegSize = 0;

      if (reserveSpillReg) {
        std::pair <unsigned, unsigned> p = reserveGRFSpillReg(coloring);
        spillRegSize = p.first;
        indrSpillRegSize = p.second;
      }
      generateForbiddenTemplates(spillRegSize + indrSpillRegSize);
      bool isColoringGood = coloring.regAlloc(
          doBankConflictReduction, highInternalConflict, &rpe);
      if (!isColoringGood) {

        bool runRemat =
            kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_CM
                ? true
                : kernel.getSimdSize() < kernel.numEltPerGRF<Type_UB>();
        // -noremat takes precedence over -forceremat
        bool rematOn = !kernel.getOption(vISA_Debug) &&
                       !kernel.getOption(vISA_NoRemat) &&
                       !kernel.getOption(vISA_FastSpill) && !fastCompile &&
                       (kernel.getOption(vISA_ForceRemat) || runRemat);
        bool rerunGRA = false;
        bool globalSplitChange = false;

        if (!rematDone && rematOn) {
          RA_TRACE(std::cout << "\t--rematerialize\n");
          Rematerialization remat(kernel, liveAnalysis, coloring, rpe, *this);
          remat.run();
          rematDone = true;

          // Re-run GRA loop only if remat caused changes to IR
          rerunGRA |= remat.getChangesMade();
        }

        if (kernel.getOption(vISA_SplitGRFAlignedScalar) && !fastCompile &&
            !kernel.getOption(vISA_FastSpill) && !alignedScalarSplitDone) {
          SplitAlignedScalars split(*this, coloring);
          split.run();
          alignedScalarSplitDone = true;

          // Re-run GRA loop if changes were made to IR
          rerunGRA |= split.getChangesMade();
        }

        // Calculate the spill caused by send to decide if global splitting is
        // required or not
        for (auto spilled : coloring.getSpilledLiveRanges()) {
          auto spillDcl = spilled->getDcl();
          if (spillDcl->getIsRefInSendDcl() && spillDcl->getNumRows() > 1) {
            sendAssociatedGRFSpillFillCount += spilled->getRefCount();
          }
        }

        int instNum = 0;
        for (auto bb : kernel.fg) {
          instNum += (int)bb->size();
        }

        if (iterationNo ==
                0 && // Only works when first iteration of Global RA failed.
            !splitPass.didGlobalSplit && // Do only one time.
            splitPass.canDoGlobalSplit(builder, kernel,
                                       sendAssociatedGRFSpillFillCount)) {
          RA_TRACE(std::cout << "\t--global send split\n");
          splitPass.globalSplit(builder, kernel);
          splitPass.didGlobalSplit = true;
          globalSplitChange = true;
          // TODO: Since global split is rarely enabled, for now we skip
          // incremental RA whenever it is enabled.
          incRA.skipIncrementalRANextIter();
        }

        if (iterationNo == 0 && (rerunGRA || globalSplitChange ||
                                 kernel.getOption(vISA_forceBCR))) {
          if (kernel.getOption(vISA_forceBCR)) {
            kernel.getOptions()->setOption(vISA_forceBCR, false);
          }

          continue;
        }

        if (!kernel.getOption(vISA_Debug) && iterationNo == 0 && !fastCompile &&
            kernel.getOption(vISA_DoSplitOnSpill)) {
          RA_TRACE(std::cout << "\t--var split around loop\n");
          LoopVarSplit loopSplit(kernel, &coloring, &liveAnalysis);
          kernel.fg.getLoops().computePreheaders();
          loopSplit.run();
        }

        // Very few spills in this iter. Check if we can convert this to fail
        // safe iter. By converting this iter to fail safe we can save (at
        // least) 1 additional iter to allocate spilled temps. But converting to
        // fail safe needs extra checks because no reserved GRF may exist at
        // this point. So push/pop needs to succeed without additional GRF
        // potentially.
        if (!kernel.getOption(vISA_Debug) && iterationNo >= 1 &&
            kernel.getOption(vISA_NewFailSafeRA) && !reserveSpillReg &&
            coloring.getSpilledLiveRanges().size() <=
                BoundedRA::MaxSpillNumVars &&
            liveAnalysis.getNumSelectedVar() > BoundedRA::LargeProgramSize) {
          // Stack call always has free GRF so it is safe to convert this iter
          // to fail safe
          if (builder.usesStack() ||
              // If LSC has to be used for spill/fill then we need to ensure
              // spillHeader is created
              (useLscForNonStackCallSpillFill &&
               builder.hasValidSpillFillHeader()) ||
       // If scratch is to be used then max spill offset must be within
       // addressable range and r0 must be available as reserved. If r0
       // is not reserved, we cannot conver current iteration to fail
       // safe because r0 may get assigned to other virtual variables.
              ((kernel.getOption(vISA_PreserveR0InR0) ||
                builder.getBuiltinR0()->isOutput()) &&
               (nextSpillOffset + BoundedRA::getNumPhyVarSlots(kernel)) <
                   SCRATCH_MSG_LIMIT)) {
            // Few ranges are spilled but this was not executed as fail
            // safe iteration. However, we've the capability of doing
            // push/pop with new fail safe RA implementation. So for very
            // few spills, we insert push/pop to free up some GRFs rather
            // than executing a new RA iteration. When doing so, we mark
            // this RA iteration as fail safe.
            reserveSpillReg = true;
            coloring.markFailSafeIter(true);
            // No reserved GRFs
            setNumReservedGRFsFailSafe(0);
            RA_TRACE(std::cout << "\t--enabling new fail safe RA\n");
          }
        }

        // Calculate the spill caused by send to decide if global splitting is
        // required or not
        for (auto spilled : coloring.getSpilledLiveRanges()) {
          GRFSpillFillCount += spilled->getRefCount();
        }

        if (iterationNo == 0) {
          // Dump out interference graph information of spill candidates
          VISA_DEBUG_VERBOSE(reportSpillInfo(liveAnalysis, coloring));
        }

        // vISA_AbortOnSpillThreshold is defined as [0..200]
        // where 0 means abort on any spill and 200 means never abort
        auto underSpillThreshold = [this](int numSpill, int asmCount) {
          int threshold = std::min(
              builder.getOptions()->getuInt32Option(vISA_AbortOnSpillThreshold),
              200u);
          return (numSpill * 200) < (threshold * asmCount);
        };

        bool isUnderThreshold = underSpillThreshold(GRFSpillFillCount, instNum);
        if (isUnderThreshold) {
          if (auto jitInfo = builder.getJitInfo()) {
            jitInfo->avoidRetry = true;
          }
        }

        if (builder.getOption(vISA_AbortOnSpill) && !isUnderThreshold) {
          // update jit metadata information
          if (auto jitInfo = builder.getJitInfo()) {
            jitInfo->stats.spillMemUsed = 0;
            jitInfo->stats.numAsmCountUnweighted = instNum;
            jitInfo->stats.numGRFSpillFillWeighted = GRFSpillFillCount;
          }

          // Early exit when -abortonspill is passed, instead of
          // spending time inserting spill code and then aborting.
          stopTimer(TimerID::GRF_GLOBAL_RA);
          return VISA_SPILL;
        }

        if (iterationNo == 0 && enableSpillSpaceCompression &&
            kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D &&
            !hasStackCall) {
          unsigned spillSize = 0;
          const LIVERANGE_LIST &spilledLRs = coloring.getSpilledLiveRanges();
          for (auto lr : spilledLRs) {
            spillSize += lr->getDcl()->getByteSize();
          }
          if ((int)(spillSize * 1.5) <
              (SCRATCH_MSG_LIMIT - globalScratchOffset)) {
            enableSpillSpaceCompression = false;
          }
        }

        startTimer(TimerID::SPILL);
        SpillManagerGRF spillGRF(
            *this, nextSpillOffset, liveAnalysis.getNumSelectedVar(),
            &liveAnalysis, coloring.getLiveRanges(), coloring.getIntf(),
            &coloring.getSpilledLiveRanges(), iterationNo++, reserveSpillReg,
            spillRegSize, indrSpillRegSize, enableSpillSpaceCompression,
            useScratchMsgForSpill, builder.avoidDstSrcOverlap());

        if (kernel.getOption(vISA_SpillAnalysis)) {
          spillAnalysis->Do(&liveAnalysis, &coloring, &spillGRF);
        }

        vISA_ASSERT(
            std::all_of(coloring.getSpilledLiveRanges().begin(),
                        coloring.getSpilledLiveRanges().end(),
                        [&](const LiveRange *spilledLR) {
                          // EOT spills even of infinite cost are specially
                          // handled in spill insertion when using old fail safe
                          // RA. So don't assert for such spills.
                          if (isEOTSpillWithFailSafeRA(builder, spilledLR,
                                                       reserveSpillReg) &&
                              !builder.getOption(vISA_NewFailSafeRA))
                            return true;
                          return spilledLR->getSpillCost() != MAXSPILLCOST;
                        }),
            "Spilled inf spill cost range");

        bool success = spillGRF.insertSpillFillCode(&kernel, pointsToAnalysis);
        nextSpillOffset = spillGRF.getNextOffset();

        if (kernel.getOption(vISA_VerifyRA)) {
          // For least false positives, turn off RMW opt and spill cleanup
          verifySpillFill();
        }

        if (builder.hasScratchSurface() && !hasStackCall &&
            (nextSpillOffset + globalScratchOffset) > SCRATCH_MSG_LIMIT) {
          // create temp variable to store old a0.2 - this is marked as live-in
          // and live-out. because the variable is emitted only post RA to
          // preserve old value of a0.2.
          kernel.fg.builder->getOldA0Dot2Temp();
        } else if (useLscForNonStackCallSpillFill || useLscForScatterSpill) {
          kernel.fg.builder->getOldA0Dot2Temp();
        }

        RA_TRACE({
          auto &&spills = coloring.getSpilledLiveRanges();
          std::cout << "\t--# variables spilled: " << spills.size() << "\n";
          if (spills.size() < 100) {
            std::cout << "\t--spilled variables: ";
            for (auto &&lr : spills) {
              std::cout << lr->getDcl()->getName() << "  ";
            }
            std::cout << "\n";
          }
          std::cout << "\t--current spill size: " << nextSpillOffset << "\n";
        });

        if (!success) {
          iterationNo = maxRAIterations;
          break;
        }

        kernel.dumpToFile("after.Spill_GRF." + std::to_string(iterationNo));
#ifndef DLL_MODE
        if (stopAfter("Spill_GRF")) {
          return VISA_EARLY_EXIT;
        }
#endif // DLL_MODE

        scratchOffset =
            std::max(scratchOffset, spillGRF.getNextScratchOffset());

        bool disableSpillCoalecse =
            builder.getOption(vISA_DisableSpillCoalescing) ||
            builder.getOption(vISA_FastSpill) || fastCompile ||
            builder.getOption(vISA_Debug) ||
            // spill cleanup is not support when we use oword msg for spill/fill
            // for non-stack calls.
            (!useScratchMsgForSpill && !hasStackCall);

        if (!reserveSpillReg && !disableSpillCoalecse && builder.useSends()) {
          RA_TRACE(std::cout << "\t--spill/fill cleanup\n");
          CoalesceSpillFills c(kernel, liveAnalysis, coloring, spillGRF,
                               iterationNo, rpe, *this);
          c.run();
#ifndef DLL_MODE
          if (stopAfter("spillCleanup")) {
            return VISA_EARLY_EXIT;
          }
#endif // DLL_MODE
        }

        if (iterationNo == builder.getuint32Option(vISA_FailSafeRALimit)) {
          if (coloring.getSpilledLiveRanges().size() < 2) {
            // give regular RA one more try as we are close to success
            failSafeRAIteration++;
          }
        }
        stopTimer(TimerID::SPILL);
      }
      // RA successfully allocates regs
      if (isColoringGood == true || reserveSpillReg) {
        coloring.confirmRegisterAssignments();

        if (hasStackCall) {
          // spill/fill intrinsics expect offset in HWord, so round up to 64
          // byte but maintain it in OWord unit ToDo: we really need to change
          // everything to byte for everyone's sanity..
          unsigned localSpillAreaOwordSize = ROUND(scratchOffset, 64) / 16;
          coloring.getSaveRestoreRegister();
          addSaveRestoreCode(localSpillAreaOwordSize);
        }

        if (kernel.getOption(vISA_DumpRegChart)) {
          assignRegForAliasDcl();
          computePhyReg();
          // invoke before expanding spill/fill since
          // it modifies IR
          regChart->dumpRegChart(std::cerr, {}, 0);
        }

        if (builder.getOption(vISA_DynPerfModel)) {
          perfModel.run();
        }

        expandSpillFillIntrinsics(nextSpillOffset);

        VISA_DEBUG_VERBOSE(detectUndefinedUses(liveAnalysis, kernel));

        if (nextSpillOffset) {
          switch (kernel.getRAType()) {
          case RA_Type::GRAPH_COLORING_RR_BC_RA:
            kernel.setRAType(RA_Type::GRAPH_COLORING_SPILL_RR_BC_RA);
            break;
          case RA_Type::GRAPH_COLORING_FF_BC_RA:
            kernel.setRAType(RA_Type::GRAPH_COLORING_SPILL_FF_BC_RA);
            break;
          case RA_Type::GRAPH_COLORING_RR_RA:
            kernel.setRAType(RA_Type::GRAPH_COLORING_SPILL_RR_RA);
            break;
          case RA_Type::GRAPH_COLORING_FF_RA:
            kernel.setRAType(RA_Type::GRAPH_COLORING_SPILL_FF_RA);
            break;
          default:
            vISA_ASSERT_UNREACHABLE("invalid ra type");
            break;
          }
        }

        if (verifyAugmentation) {
          assignRegForAliasDcl();
          computePhyReg();
          verifyAugmentation->verify();
        }
        break; // done
      }
    } else {
      break;
    }
  }
  assignRegForAliasDcl();
  computePhyReg();

  stopTimer(TimerID::GRF_GLOBAL_RA);
  //
  // Report failure to allocate due to excessive register pressure.
  //
  if (!reserveSpillReg && (iterationNo == maxRAIterations)) {
    std::stringstream spilledVars;
    for (auto dcl : kernel.Declares) {
      if (dcl->isSpilled() && dcl->getRegFile() == G4_GRF) {
        spilledVars << dcl->getName() << "\t";
      }
    }

    vISA_ASSERT(false, "%d GRF registers are NOT enough to compile kernel %s \
        The maximum register pressure in the kernel is higher than the available \
        physical registers in hardware (even with spill code). Please consider \
        rewriting the kernel. Compiling with the symbolic register option and \
        inspecting the spilled registers may help in determinig the region of high \
        pressure. The spilling virtual registers are as follows %s.",
                (kernel.getNumRegTotal() -
                 builder.getOptions()->getuInt32Option(vISA_ReservedGRFNum)),
                kernel.getName(), spilledVars.str().c_str());

    return VISA_SPILL;
  }

  // this includes vISA's scratch space use only and does not include whatever
  // IGC may use for private memory
  uint32_t spillMemUsed = builder.kernel.fg.frameSizeInOWord ?
      (builder.kernel.fg.frameSizeInOWord * 16) : nextSpillOffset;

  spillMemUsed = ROUND(spillMemUsed, kernel.numEltPerGRF<Type_UB>());

  if (spillMemUsed &&
      !(kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc())) {
    builder.criticalMsgStream()
        << "Spill memory used = " << spillMemUsed << " bytes for kernel "
        << kernel.getName()
        << "\n Compiling kernel with spill code may degrade performance."
        << " Please consider rewriting the kernel to use less registers.\n";
  }

  // update jit metadata information for spill
  if (auto jitInfo = builder.getJitInfo()) {
    // jitInfo->spillMemUsed is the entire visa stack size. Consider the
    // caller/callee save size if having caller/callee save
    // globalScratchOffset in unit of byte, others in Oword
    //
    // FIXME: globalScratchOffset must be 0 when having stack call, or
    // there is a problem at stack setup
    // (see GlobalRA::addGenxMainStackSetupCode)
    //
    //                               vISA stack
    //  globalScratchOffset     -> ---------------------
    //                             |  spill            |
    //  calleeSaveAreaOffset    -> ---------------------
    //                             |  callee save      |
    //  callerSaveAreaOffset    -> ---------------------
    //                             |  caller save      |
    //  frameSizeInOWord        -> ---------------------

    jitInfo->hasStackcalls = kernel.fg.getHasStackCalls();

    // Each function reports its required stack size.
    // We will summarize the final stack size of entire vISA module into
    // the main functions (ref: CISA_IR_Builder::summarizeFunctionInfo)
    jitInfo->stats.spillMemUsed = spillMemUsed;
    kernel.getGTPinData()->setScratchNextFree(spillMemUsed +
                                              globalScratchOffset);
    jitInfo->stats.numGRFSpillFillWeighted = GRFSpillFillCount;
  }

  if (builder.getOption(vISA_LocalDeclareSplitInGlobalRA)) {
    removeSplitDecl();
  }

  if (builder.getOption(vISA_DynPerfModel)) {
    perfModel.dump();
  }

  return VISA_SUCCESS;
}

/********************************************************************************************************************************************/
/********************************************************Spill Code Clean up
 * ****************************************************************/
/********************************************************************************************************************************************/

#define SPILL_MEMORY_OVERLAP(x, y)                                             \
  (!(x->leftOff > y->rightOff || y->leftOff > x->rightOff))

#define SPILL_MEMORY_OVERWRITE(target_memory, overwrite_memory)                \
  (target_memory->leftOff >= overwrite_memory->leftOff &&                      \
   overwrite_memory->rightOff >= target_memory->rightOff)

#define IS_FLAG_MOVE(inst)                                                     \
  (inst->opcode() == G4_mov && (inst->getDst() && inst->getSrc(0)) &&          \
   (inst->getDst()->getTopDcl() && inst->getSrc(0)->getTopDcl()) &&            \
   ((inst->getDst()->getTopDcl()->getRegFile() == G4_FLAG &&                   \
     inst->getSrc(0)->getTopDcl()->getRegFile() == G4_GRF) ||                  \
    (inst->getDst()->getTopDcl()->getRegFile() == G4_GRF &&                    \
     inst->getSrc(0)->getTopDcl()->getRegFile() == G4_FLAG)))

#define IS_SPILL_KILL_CANDIDATE(preScratchAccess)                              \
  (preScratchAccess->isSpill && !preScratchAccess->fillInUse)

#define IS_USE_KILL_CANDIDATE(preScratchAccess)                                \
  (!(preScratchAccess->regKilled || preScratchAccess->regPartialKilled ||      \
     preScratchAccess->scratchDefined))

#define IS_GRF_RANGE_OVERLAP(s1, e1, sa)                                       \
  (e1 >= sa->linearizedStart && sa->linearizedEnd >= s1)

#define IS_SCRATCH_RANGE_OVERLAP(s1, e1, sa)                                   \
  (!(e1 < sa->leftOff || sa->rightOff < s1))

#define IS_MERGEABLE_SCRATCH_RANGES(r1, r2)                                    \
  (!(((int)r1.leftOff - (int)r2.rightOff) > 1 ||                               \
     ((int)r2.leftOff - (int)r1.rightOff) > 1))

#define IS_MERGEABLE_GRF_RANGES(r1, r2)                                        \
  (!(((int)r1.linearizedStart - (int)r2.linearizedEnd) > 1 ||                  \
     ((int)r2.linearizedStart - (int)r1.linearizedEnd) > 1))

#define IS_GRF_RANGE_OVERWRITE(sa, s1, e1)                                     \
  (s1 <= sa->linearizedStart && sa->linearizedEnd <= e1)

#define IS_SCRATCH_RANGE_OVERWRITE(sa, s1, e1)                                 \
  (s1 <= sa->leftOff && sa->rightOff <= e1)

#define IS_FLAG_RANGE_OVERLAP(s1, e1, sa)                                      \
  (!(e1 < sa->linearizedStart || sa->linearizedEnd < s1))

#define IS_FLAG_RANGE_OVERWRITE(t, s, e)                                       \
  ((s <= t->linearizedStart && t->linearizedEnd <= e))

void FlagSpillCleanup::FlagLineraizedStartAndEnd(G4_Declare *topdcl,
                                                 unsigned &linearizedStart,
                                                 unsigned &linearizedEnd) {
  const G4_Areg *areg = topdcl->getRegVar()->getPhyReg()->asAreg();
  linearizedStart = areg->getFlagNum() * 4;
  linearizedStart +=
      topdcl->getRegVar()->getPhyRegOff() * topdcl->getElemSize();
  linearizedEnd = linearizedStart + topdcl->getByteSize();
  return;
}

/*
 * Reuse previous register
 */
bool FlagSpillCleanup::replaceWithPreDcl(IR_Builder &builder,
                                         SCRATCH_ACCESS *scratchAccess,
                                         SCRATCH_ACCESS *preScratchAccess) {
  int preRegOff = 0;
  int payloadHeaderSize = 0;
  G4_Operand *reuseOpnd = NULL;
  G4_INST *preInst = *preScratchAccess->inst_it;

  // Get reuse operand
  if (preScratchAccess->isSpill) {
    reuseOpnd = preInst->getSrc(0);
    preRegOff = reuseOpnd->asSrcRegRegion()->getSubRegOff();
    reuseOpnd = preInst->getSrc(0);
  } else {
    reuseOpnd = preInst->getDst();
    preRegOff = reuseOpnd->asDstRegRegion()
                    ->getSubRegOff(); // For flag register, only subRegOff
  }
  G4_Declare *dcl = reuseOpnd->getBase()->asRegVar()->getDeclare();

  if (builder.WaDisableSendSrcDstOverlap()) {
    for (auto &renameOpnd : scratchAccess->renameOperandVec) {
      if (renameOpnd.second < -1) // Flag
      {
        break;
      }

      G4_INST *inst = renameOpnd.first;

      if (renameOpnd.second >= 0) {
        if (inst->isSend() && !inst->getDst()->isNullReg()) {
          G4_DstRegRegion *dst = inst->getDst();
          bool noOverlap =
              dst->getLinearizedEnd() < preScratchAccess->linearizedStart ||
              preScratchAccess->linearizedEnd < dst->getLinearizedStart();
          if (!noOverlap) {
            return false;
          }
        }
      }
    }
  }

  // Replace the declare for all operands assciated with this scratch fill.
  for (auto &renameOpnd : scratchAccess->renameOperandVec) {
    G4_INST *inst = renameOpnd.first;

    if (renameOpnd.second == -3) // Flag modifier
    {
      G4_CondMod *mod = inst->getCondMod();
      int regOff = preRegOff;
      G4_CondMod *mod_Opnd = builder.createCondMod(
          mod->getMod(), dcl->getRegVar(), (unsigned short)regOff);

      inst->setCondMod(mod_Opnd);

    } else if (renameOpnd.second == -2) // Flag predicate
    {
      G4_Predicate *predicate = inst->getPredicate();
      int regOff = preRegOff;
      G4_Predicate *pred_Opnd = builder.createPredicate(
          predicate->getState(), dcl->getRegVar(), (unsigned short)regOff,
          predicate->getControl());

      inst->setPredicate(pred_Opnd);
    } else if (renameOpnd.second == -1) // GRF dst
    {
      G4_DstRegRegion *orgDstRegion = inst->getDst();
      int regOff = preRegOff +
                   (scratchAccess->leftOff - preScratchAccess->leftOff) /
                       gra.kernel.numEltPerGRF<Type_UB>() +
                   payloadHeaderSize / gra.kernel.numEltPerGRF<Type_UB>();
      G4_DstRegRegion *dstOpnd = builder.createDst(
          dcl->getRegVar(), (short)regOff, orgDstRegion->getSubRegOff(),
          orgDstRegion->getHorzStride(), orgDstRegion->getType());
      inst->setDest(dstOpnd);
    } else // GRF src
    {
      G4_Operand *opnd = inst->getSrc(renameOpnd.second);
      G4_SrcRegRegion *orgSrcRegion = opnd->asSrcRegRegion();

      int regOff = preRegOff +
                   (scratchAccess->leftOff - preScratchAccess->leftOff) /
                       gra.kernel.numEltPerGRF<Type_UB>() +
                   payloadHeaderSize / gra.kernel.numEltPerGRF<Type_UB>();
      G4_Operand *srcOpnd = builder.createSrcRegRegion(
          orgSrcRegion->getModifier(), orgSrcRegion->getRegAccess(),
          dcl->getRegVar(), (short)regOff, orgSrcRegion->getSubRegOff(),
          orgSrcRegion->getRegion(), orgSrcRegion->getType());

      inst->setSrc(srcOpnd, renameOpnd.second);
    }
  }

  return true;
}

/*
 *  1) The reuse target register in pre scratch access may be partial killed,
 *  2) and the corresponding scracth memory range is overlap with the memory of
 * current scratch access. In both cases, the current fill can not be removed
 */
bool FlagSpillCleanup::scratchKilledByPartial(
    SCRATCH_ACCESS *scratchAccess, SCRATCH_ACCESS *preScratchAccess) {
  bool killed = false;

  for (auto &range : preScratchAccess->killedScratchRange) {
    if (!(scratchAccess->leftOff > range.rightOff ||
          range.leftOff > scratchAccess->rightOff)) {
      killed = true;
    }
  }

  for (auto &range : preScratchAccess->killedRegRange) {
    // Map the register kill to scratch kill
    unsigned leftOff =
        preScratchAccess->leftOff +
        (range.linearizedStart - preScratchAccess->linearizedStart);
    unsigned rightOff =
        preScratchAccess->leftOff +
        (range.linearizedEnd - preScratchAccess->linearizedStart);

    if (!(scratchAccess->leftOff > rightOff ||
          leftOff > scratchAccess->rightOff)) {
      killed = true;
    }
  }

  return killed;
}

/*
 *  Record all killed GRF ranges.
 *  do merging of ranges when possible.
 */
bool FlagSpillCleanup::addKilledGRFRanges(unsigned linearizedStart,
                                          unsigned linearizedEnd,
                                          SCRATCH_ACCESS *scratchAccess,
                                          G4_Predicate *predicate) {
  REG_RANGE range;
  range.linearizedStart =
      std::max(scratchAccess->linearizedStart, linearizedStart);
  range.linearizedEnd = std::min(scratchAccess->linearizedEnd, linearizedEnd);
  range.predicate = predicate ? true : false;

  if (scratchAccess->killedRegRange.size() == 0) {
    scratchAccess->killedRegRange.push_back(range);
  } else {
    bool merged = false;
    REG_RANGE_VEC_ITER range_iter = scratchAccess->killedRegRange.begin();
    REG_RANGE_VEC_ITER range_iter_next;
    REG_RANGE *merged_range = NULL;
    while (range_iter != scratchAccess->killedRegRange.end()) {
      REG_RANGE &killedRange = *(range_iter);
      range_iter_next = range_iter;
      range_iter_next++;

      if (killedRange.predicate) // With predicate, the range can not be merged
                                 // with others
      {
        range_iter = range_iter_next;
        continue;
      }

      if (!merged && IS_MERGEABLE_GRF_RANGES(range, killedRange)) {
        killedRange.linearizedStart =
            std::min(killedRange.linearizedStart, range.linearizedStart);
        killedRange.linearizedEnd =
            std::max(killedRange.linearizedEnd, range.linearizedEnd);
        merged = true;
        merged_range = &killedRange;
      } else if (merged) {
        if (IS_MERGEABLE_GRF_RANGES((*merged_range), killedRange)) {
          merged_range->linearizedStart = std::min(
              killedRange.linearizedStart, merged_range->linearizedStart);
          merged_range->linearizedEnd =
              std::max(killedRange.linearizedEnd, merged_range->linearizedEnd);
        }
      }
      if (IS_GRF_RANGE_OVERWRITE(scratchAccess, killedRange.linearizedStart,
                                 killedRange.linearizedEnd)) {
        scratchAccess->regKilled = true;
        return true;
      }
      range_iter = range_iter_next;
    }
    if (!merged) {
      scratchAccess->killedRegRange.push_back(range);
    }
  }

  return false;
}

/*
 * Check if the register in previous scratch access is fully killed by current
 * register define
 */
bool FlagSpillCleanup::regFullyKilled(SCRATCH_ACCESS *scratchAccess,
                                      unsigned linearizedStart,
                                      unsigned linearizedEnd,
                                      unsigned short maskFlag) {

  if (IS_FLAG_RANGE_OVERWRITE(scratchAccess, linearizedStart, linearizedEnd)) {
    if (maskFlag & InstOpt_WriteEnable) // No mask == all range killed
    {
      return true;
    }

    if (linearizedStart == scratchAccess->linearizedStart &&
        linearizedEnd == scratchAccess->linearizedEnd &&
        scratchAccess->maskFlag == maskFlag) {
      return true;
    }
  }

  return false;
}

/*
 *  Check only part of scratch register is killed, at the same time no overlap.
 *  This is to make sure if the associated fill is removed, the define register
 * can be replaced with reuse register or not.
 */
bool FlagSpillCleanup::inRangePartialKilled(SCRATCH_ACCESS *scratchAccess,
                                            unsigned linearizedStart,
                                            unsigned linearizedEnd,
                                            unsigned short maskFlag) {
  if ((scratchAccess->linearizedStart <= linearizedStart &&
       scratchAccess->linearizedEnd >= linearizedEnd)) {
    if (maskFlag & InstOpt_WriteEnable) {
      return true;
    }

    if (scratchAccess->linearizedStart == linearizedStart &&
        scratchAccess->linearizedEnd == linearizedEnd &&
        scratchAccess->maskFlag == maskFlag) {
      return true;
    }
  }

  return false;
}

/*
 * Register kill analysis
 */
bool FlagSpillCleanup::regDefineAnalysis(SCRATCH_ACCESS *scratchAccess,
                                         unsigned linearizedStart,
                                         unsigned linearizedEnd,
                                         unsigned short maskFlag,
                                         G4_Predicate *predicate) {
  if (regFullyKilled(scratchAccess, linearizedStart, linearizedEnd, maskFlag)) {
    return true;
  } else if (!scratchAccess->regKilled) {
    // Handle partial overlap
    // What about the mask?
    if (addKilledGRFRanges(linearizedStart, linearizedEnd, scratchAccess,
                           predicate)) {
      // The register range is killed by accumulated partial range kills
      return true;
    }
    scratchAccess->regPartialKilled = true;
  }

  return false;
}

void FlagSpillCleanup::regDefineFlag(SCRATCH_PTR_LIST *scratchTraceList,
                                     G4_INST *inst, G4_Operand *opnd) {
  // Get the linearized address in GRF register file
  unsigned linearizedStart = 0;
  unsigned linearizedEnd = 0;
  G4_Predicate *predicate = inst->getPredicate();
  G4_Declare *topdcl = opnd->getTopDcl();

  FlagLineraizedStartAndEnd(opnd->getTopDcl(), linearizedStart, linearizedEnd);

  // Impact on previous scratch access
  SCRATCH_PTR_LIST_ITER it = scratchTraceList->begin();
  SCRATCH_PTR_LIST_ITER itEnd = scratchTraceList->end();

  if (it != itEnd && inst == *(scratchTraceList->back()->inst_it)) {
    itEnd--;
  }

  while (it != itEnd) {
    SCRATCH_PTR_LIST_ITER kt = it;
    kt++;

    SCRATCH_ACCESS *scratchAccess = *it;

    // Not instruction itself, def->use can not happen in single instruction.
    if (scratchAccess->regKilled) {
      it = kt;
      continue;
    }

    // Checked if the registers used in the previous scratch accesses (both
    // spill and fill) are killed (redefined).
    if (linearizedEnd &&
        IS_FLAG_RANGE_OVERLAP(linearizedStart, linearizedEnd, scratchAccess)) {
      // E mask
      unsigned maskFlag = (inst->getOption() & 0xFFF010C);

      if (regDefineAnalysis(scratchAccess, linearizedStart, linearizedEnd,
                            (unsigned short)maskFlag, predicate)) {
        // Fully killed
        scratchAccess->regKilled = true;
        if (scratchAccess->evicted) // Not in use
        {
          scratchTraceList->erase(
              it); // The previous one is not candidate for future use
        }
      }

      // For prefill and associated define and spill instructions
      // 1. Same dcl is used
      // 2. If the prefill register is fulled killed,
      //     a. The prefill instruction can be removed.
      //     b. But the define and instruction's registers are kept and will not
      //     reuse previous one.
      // 3. If the prefill register is partial killed, and the killed register
      // region is part of prefill region.
      //     a. The prefill instruction can be removed.
      //     b. and the register in define and spill instruction can reuse
      //     previous one.
      // 4. Otherwise, the (pre)fill instruction can not be removed, and no
      // reuse will happen.
      // 5. For pure fill, it's no killed by same declare
      G4_Declare *preDcl = scratchAccess->flagOpnd->getTopDcl();

      if (topdcl == preDcl) {
        if (inRangePartialKilled(scratchAccess, linearizedStart, linearizedEnd,
                                 (unsigned short)maskFlag)) {
          scratchAccess->renameOperandVec.emplace_back(inst, -1);
          scratchAccess->inRangePartialKilled = true;
        } else {
          scratchAccess->removeable = false;
        }
      }
    }

    it = kt;
  }
}

/*
 *  Analysis the use of register to determine if the scratchAccess can be
 * removed or not
 *
 */
bool FlagSpillCleanup::regUseAnalysis(SCRATCH_ACCESS *scratchAccess,
                                      unsigned linearizedStart,
                                      unsigned linearizedEnd) {
  // GRF in previous fill is used as part of current reg,
  // In this case, the fill can not be removed since the reuse can not happen.
  // Caller gauranteed the overlap of the registers
  if (linearizedEnd > scratchAccess->linearizedEnd ||
      linearizedStart < scratchAccess->linearizedStart) {
    return true;
  }

  // Can not be removed when the previous scratch access is killed or partial
  // killed before the use of current scratch access register
  // b
  SCRATCH_ACCESS *preScratchAccess = scratchAccess->preScratchAccess;
  if (preScratchAccess &&
      (preScratchAccess->regKilled ||
       scratchKilledByPartial(scratchAccess, preScratchAccess))) {
    return true;
  }

  // Back trace to update the reachable scratch accesses
  if (preScratchAccess && scratchAccess->prePreScratchAccess) {
    SCRATCH_ACCESS *prePreScratchAccess = preScratchAccess;
    preScratchAccess = scratchAccess;

    do {
      if ((prePreScratchAccess->regKilled ||
           scratchKilledByPartial(scratchAccess, prePreScratchAccess))) {
        scratchAccess->prePreScratchAccess = preScratchAccess;
        break;
      }
      preScratchAccess = prePreScratchAccess;
      prePreScratchAccess = preScratchAccess->preScratchAccess;
    } while (prePreScratchAccess &&
             preScratchAccess != scratchAccess->prePreScratchAccess);
  }

  return false;
}

void FlagSpillCleanup::regUseFlag(SCRATCH_PTR_LIST *scratchTraceList,
                                  G4_INST *inst, G4_Operand *opnd,
                                  int opndIndex) {
  // Get the linearized address in GRF register file
  unsigned linearizedStart = 0;
  unsigned linearizedEnd = 0;
  G4_Declare *topdcl = NULL;

  topdcl = opnd->getTopDcl();
  FlagLineraizedStartAndEnd(opnd->getTopDcl(), linearizedStart, linearizedEnd);

  // Impact on previous scratch access
  for (SCRATCH_ACCESS *scratchAccess : *scratchTraceList) {
    if (linearizedEnd &&
        IS_FLAG_RANGE_OVERLAP(linearizedStart, linearizedEnd, scratchAccess)) {
      // Not handle indirect GRF
      if (inst->isEOT() || inst->isPseudoUse()) {
        scratchAccess->removeable = false;
        continue;
      }

      if (scratchAccess->flagOpnd->getTopDcl() == topdcl) // Same declare
      {
        if (regUseAnalysis(scratchAccess, linearizedStart, linearizedEnd)) {
          // The filled register is in use
          scratchAccess->removeable = false;
        } else if (scratchAccess->inRangePartialKilled ||
                   !scratchAccess->regKilled) {
          // can reuse previous register
          scratchAccess->renameOperandVec.emplace_back(inst, opndIndex);
        }
      }
    }
  }
}

void FlagSpillCleanup::regUseScratch(SCRATCH_PTR_LIST *scratchTraceList,
                                     G4_INST *inst, G4_Operand *opnd,
                                     Gen4_Operand_Number opndNum) {
  const G4_Declare *topdcl = opnd->getTopDcl();

  // Impact on previous scratch access
  for (SCRATCH_ACCESS *scratchAccess : *scratchTraceList) {
    if (topdcl == scratchAccess->scratchDcl) {
      if (opndNum == Opnd_dst) {
        scratchAccess->scratchDefined = true;
      } else {
        scratchAccess->removeable = false;
      }
    }
  }
}

void FlagSpillCleanup::initializeScratchAccess(SCRATCH_ACCESS *scratchAccess,
                                               INST_LIST_ITER inst_it) {
#ifdef _DEBUG
  scratchAccess->regNum = -1;
#endif
  scratchAccess->scratchDcl = NULL;
  scratchAccess->flagOpnd = NULL;

  scratchAccess->linearizedStart = 0;
  scratchAccess->linearizedEnd = 0;
  scratchAccess->leftOff = 0;
  scratchAccess->rightOff = 0;
  scratchAccess->useCount = 0;

  scratchAccess->isSpill = false;
  scratchAccess->isBlockLocal = false;
  scratchAccess->directKill = false;

  scratchAccess->regKilled = false;
  scratchAccess->regPartialKilled = false;
  scratchAccess->regOverKilled = false;
  scratchAccess->inRangePartialKilled = false;
  scratchAccess->regInUse = false;

  scratchAccess->fillInUse = false;
  scratchAccess->removeable = true;
  scratchAccess->instKilled = false;
  scratchAccess->evicted = false;
  scratchAccess->scratchDefined = false;

  scratchAccess->preScratchAccess = NULL;
  scratchAccess->prePreScratchAccess = NULL;
  scratchAccess->preFillAccess = NULL;

  scratchAccess->inst_it = inst_it;
  G4_INST *inst = *inst_it;
  scratchAccess->maskFlag = (inst->getOption() & 0xFFF010C);

  return;
}

bool FlagSpillCleanup::initializeFlagScratchAccess(
    SCRATCH_PTR_VEC *scratchAccessList, SCRATCH_ACCESS *&scratchAccess,
    INST_LIST_ITER inst_it) {
  G4_INST *inst = (*inst_it);

  G4_DstRegRegion *dst = inst->getDst();
  G4_Operand *src = inst->getSrc(0);
  G4_Declare *topDcl_1 = dst->getTopDcl();
  G4_Declare *topDcl_2 = src->getTopDcl();

  // Create the spill/fill description
  if (topDcl_1->getRegFile() == G4_FLAG && topDcl_2->getRegFile() == G4_GRF) {
    if (src->asSrcRegRegion()->getBase()->isRegVar() &&
        src->asSrcRegRegion()->getBase()->asRegVar()->isRegVarAddrSpillLoc()) {
      scratchAccess = new SCRATCH_ACCESS;
      scratchAccessList->push_back(scratchAccess);
      initializeScratchAccess(scratchAccess, inst_it);
      // Fill
#ifdef _DEBUG
      scratchAccess->regNum =
          topDcl_1->getRegVar()->getPhyReg()->asAreg()->getArchRegType();
#endif
      scratchAccess->scratchDcl = topDcl_2; // Spill location

      if (gra.isBlockLocal(topDcl_2)) {
        scratchAccess->isBlockLocal = true;
      }
      FlagLineraizedStartAndEnd(topDcl_1, scratchAccess->linearizedStart,
                                scratchAccess->linearizedEnd);
      scratchAccess->flagOpnd = dst;
      if (inst->getPredicate()) {
        scratchAccess->removeable = false; // Partil spill/fill cannot be
                                           // removed
        scratchAccess->instKilled =
            true; // Not really killed, mark so that the instruction depends on
                  // current one will not be removed.
      }

      return true;
    }
  } else { // Spill
    if (dst->getBase()->isRegVar() &&
        dst->getBase()->asRegVar()->isRegVarAddrSpillLoc()) {
      scratchAccess = new SCRATCH_ACCESS;
      scratchAccessList->push_back(scratchAccess);
      initializeScratchAccess(scratchAccess, inst_it);
#ifdef _DEBUG
      scratchAccess->regNum =
          topDcl_2->getRegVar()->getPhyReg()->asAreg()->getArchRegType();
#endif
      scratchAccess->scratchDcl = topDcl_1;

      if (gra.isBlockLocal(topDcl_1)) {
        scratchAccess->isBlockLocal = true;
      }

      scratchAccess->isSpill = true;
      FlagLineraizedStartAndEnd(topDcl_2, scratchAccess->linearizedStart,
                                scratchAccess->linearizedEnd);
      scratchAccess->flagOpnd = src;
      if (inst->getPredicate()) {
        scratchAccess->removeable = false; // Partil spill/fill cannot be
                                           // removed
        scratchAccess->instKilled =
            true; // Not really killed, mark so that the instruction depends on
                  // current one will not be removed.
      }

      return true;
    }
  }

  return false;
}

void FlagSpillCleanup::freeScratchAccess(SCRATCH_PTR_VEC *scratchAccessList) {
  for (SCRATCH_ACCESS *scratchAccess : *scratchAccessList) {
    delete scratchAccess;
  }

  scratchAccessList->clear();

  return;
}

// Check the flag define instruction.
void FlagSpillCleanup::flagDefine(SCRATCH_PTR_LIST &scratchTraceList,
                                  G4_INST *inst) {
  G4_DstRegRegion *dst = inst->getDst();

  if (dst) {
    G4_Declare *topdcl = NULL;
    topdcl = GetTopDclFromRegRegion(dst);

    if (topdcl && topdcl->getRegFile() == G4_FLAG) {
      // Flag register define
      regDefineFlag(&scratchTraceList, inst, dst);
    }
  }

  G4_CondMod *mod = inst->getCondMod();
  if (!mod) {
    return;
  }

  // ConMod, handled as register define
  unsigned maskFlag = (inst->getOption() & 0xFFF010C);

  unsigned linearizedStart = 0;
  unsigned linearizedEnd = 0;

  G4_VarBase *flagReg = mod->getBase();
  if (!flagReg) {
    return;
  }

  G4_Declare *topdcl = flagReg->asRegVar()->getDeclare();
  FlagLineraizedStartAndEnd(topdcl, linearizedStart, linearizedEnd);

  SCRATCH_PTR_LIST_ITER it = scratchTraceList.begin();
  SCRATCH_PTR_LIST_ITER itEnd = scratchTraceList.end();
  while (it != itEnd) {
    SCRATCH_PTR_LIST_ITER kt = it;
    kt++;

    SCRATCH_ACCESS *preScratchAccess = *it;
    if (IS_FLAG_RANGE_OVERLAP(linearizedStart, linearizedEnd,
                              preScratchAccess)) {
      G4_Declare *preDcl = preScratchAccess->flagOpnd->getTopDcl();

      if (regDefineAnalysis(preScratchAccess, linearizedStart, linearizedEnd,
                            (unsigned short)maskFlag, NULL)) {
        preScratchAccess->regKilled = true;
        if (preScratchAccess->evicted) // Not in use
        {
          scratchTraceList.erase(
              it); // The previous one is not candidate for reuse
        }
      }
      if (topdcl == preDcl) {
        if (preScratchAccess->inRangePartialKilled) {
          preScratchAccess->renameOperandVec.emplace_back(inst, -3);
        } else {
          preScratchAccess->removeable = false;
        }
      }
    }
    it = kt;
  }

  return;
}

void FlagSpillCleanup::scratchUse(SCRATCH_PTR_LIST &scratchTraceList,
                                  G4_INST *inst) {
  G4_DstRegRegion *dst = inst->getDst();

  if (dst) {
    G4_Declare *topdcl = NULL;
    topdcl = GetTopDclFromRegRegion(dst);

    if (topdcl && topdcl->getRegFile() == G4_GRF) {
      // Flag scratch variable is redefined
      regUseScratch(&scratchTraceList, inst, dst, Opnd_dst);
    }
  }

  for (unsigned i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
    G4_Operand *src = inst->getSrc(i);

    if (src && src->isSrcRegRegion()) {
      G4_Declare *topdcl = NULL;

      if (inst->getSrc(i)->asSrcRegRegion()->getBase()->isRegVar()) {
        topdcl = GetTopDclFromRegRegion(src);
      }

      if (!topdcl || (topdcl->getRegFile() == G4_FLAG)) {
        continue;
      }

      regUseScratch(&scratchTraceList, inst, src, Opnd_src0);
    }
  }
}

void FlagSpillCleanup::flagUse(SCRATCH_PTR_LIST &scratchTraceList,
                               G4_INST *inst) {
  for (unsigned i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
    G4_Operand *src = inst->getSrc(i);

    if (src && src->isSrcRegRegion()) {
      G4_Declare *topdcl = NULL;

      if (inst->getSrc(i)->asSrcRegRegion()->getBase()->isRegVar()) {
        topdcl = GetTopDclFromRegRegion(src);
      }

      if (!topdcl || (topdcl->getRegFile() != G4_FLAG)) {
        continue;
      }

      regUseFlag(&scratchTraceList, inst, src, i);
    }
  }

  // Flag register is used as predicate
  G4_Predicate *predicate = inst->getPredicate();
  if (!predicate) {
    return;
  }

  G4_VarBase *flagReg = predicate->getBase();
  if (!flagReg) {
    return;
  }

  G4_Declare *topdcl = flagReg->asRegVar()->getDeclare();
  unsigned linearizedStart = 0;
  unsigned linearizedEnd = 0;
  FlagLineraizedStartAndEnd(topdcl, linearizedStart, linearizedEnd);

  for (SCRATCH_ACCESS *preScratchAccess : scratchTraceList) {
    if (IS_FLAG_RANGE_OVERLAP(linearizedStart, linearizedEnd,
                              preScratchAccess)) {
      G4_Declare *preDcl = preScratchAccess->flagOpnd->getTopDcl();
      // Use should have same top declare
      if (preDcl == topdcl) {
        if (regUseAnalysis(preScratchAccess, linearizedStart, linearizedEnd)) {
          preScratchAccess->removeable = false;
        } else if (preScratchAccess->inRangePartialKilled ||
                   !preScratchAccess->regKilled) {
          // can reuse previous register
          preScratchAccess->renameOperandVec.emplace_back(inst, -2);
        }
      }
    }
  }

  return;
}

bool FlagSpillCleanup::flagScratchDefineUse(
    G4_BB *bb, SCRATCH_PTR_LIST *scratchTraceList,
    SCRATCH_PTR_VEC *candidateList, SCRATCH_ACCESS *scratchAccess,
    CLEAN_NUM_PROFILE *clean_num_profile) {
  SCRATCH_PTR_LIST_ITER it = scratchTraceList->begin();
  SCRATCH_PTR_LIST_ITER itEnd = scratchTraceList->end();

  while (it != itEnd) {
    SCRATCH_PTR_LIST_ITER kt = it;
    kt++;

    SCRATCH_ACCESS *preScratchAccess = *it;

    // Evicted
    if (preScratchAccess->evicted) {
      it = kt;
      continue;
    }

    // Same scratch declare
    if (preScratchAccess->scratchDcl ==
        scratchAccess->scratchDcl) // Same scratch location
    {
      if (scratchAccess->isSpill) // Current is spill
      {
        if (IS_SPILL_KILL_CANDIDATE(
                preScratchAccess)) // previoius is spill as well and previous
                                   // spill is not used
        {
          // kill the previous spill
          bb->erase(preScratchAccess->inst_it);
          preScratchAccess->instKilled = true;
          clean_num_profile->spill_clean_num[0]++;
          scratchTraceList->erase(
              it); // The previous one is not candidate for reuse
          it = kt;

          continue;
        }

        preScratchAccess->evicted = true;
        scratchTraceList->erase(
            it); // The previous one is not a good candidate for reuse any more
      } else     // Current is fill
      {
        preScratchAccess->fillInUse = true;
        preScratchAccess->useCount++;

        if (IS_USE_KILL_CANDIDATE(preScratchAccess)) // Is not used before
        {
          scratchAccess->preScratchAccess =
              preScratchAccess; // set previous scrach location define
          candidateList->push_back(scratchAccess); // Add to candidate list
          if (IS_FLAG_RANGE_OVERWRITE(scratchAccess,
                                      preScratchAccess->linearizedStart,
                                      preScratchAccess->linearizedEnd)) {
            // Exactly same GRF, it's useless fill, since prevous fill or spill
            // not been killed
            scratchAccess->directKill = true;
            scratchTraceList->push_back(scratchAccess);
            return true;
          }
        }
      }
    }
    it = kt;
  }

  scratchTraceList->push_back(scratchAccess);

  return false;
}

void FlagSpillCleanup::flagSpillFillClean(
    G4_BB *bb, INST_LIST_ITER inst_it, SCRATCH_PTR_VEC &scratchAccessList,
    SCRATCH_PTR_LIST &scratchTraceList, SCRATCH_PTR_VEC &candidateList,
    CLEAN_NUM_PROFILE *clean_num_profile) {
  G4_INST *inst = (*inst_it);
  if (inst->isPseudoKill()) {
    return;
  }

  bool noDefineAnalysis = false;

  // Check if there is flag use
  flagUse(scratchTraceList, inst);

  // Check if it's spill/fill of the flag
  if (IS_FLAG_MOVE(inst)) {
    SCRATCH_ACCESS *scratchAccess = NULL;

    if (initializeFlagScratchAccess(&scratchAccessList, scratchAccess,
                                    inst_it)) {
      // Build the trace list and the candidate list
      // Trace list includes all spill/fill
      // Candidate includues ??
      // Checking if the spill/fill can be removed at the same time by comparing
      // previous one.
      noDefineAnalysis =
          flagScratchDefineUse(bb, &scratchTraceList, &candidateList,
                               scratchAccess, clean_num_profile);
    }
  } else {
    scratchUse(scratchTraceList, inst);
  }

  // Check if there is flag define
  if (!noDefineAnalysis) {
    flagDefine(scratchTraceList, inst);
  }

  return;
}

#ifdef _DEBUG
#define FILL_DEBUG_THRESHOLD 0xffffffff
#define SPILL_DEBUG_THRESHOLD 0xffffffff // 25
#endif

void FlagSpillCleanup::regFillClean(IR_Builder &builder, G4_BB *bb,
                                    SCRATCH_PTR_VEC &candidateList,
                                    CLEAN_NUM_PROFILE *clean_num_profile) {
  for (SCRATCH_ACCESS *scratchAccess : candidateList) {
    SCRATCH_ACCESS *preScratchAccess = scratchAccess->preScratchAccess;

    // Since the reuse happens from front to end.
    // If the pre scratchAccess is killed, current candidate can not reuse
    // previous register any more
    if (!scratchAccess->instKilled &&
        (scratchAccess->removeable && scratchAccess->directKill)) {
      if (scratchAccess->prePreScratchAccess) {
        while (preScratchAccess && preScratchAccess->preScratchAccess &&
               preScratchAccess != scratchAccess->prePreScratchAccess) {
          // If possible, propagate to previous scratchAccess
          if (preScratchAccess->preFillAccess) {
            // to jump over prefill.
            if (preScratchAccess->isSpill && preScratchAccess->preFillAccess &&
                preScratchAccess->preFillAccess->instKilled &&
                preScratchAccess->preScratchAccess) {
              preScratchAccess = preScratchAccess->preScratchAccess;
            } else {
              break;
            }
          } else {
            if (!preScratchAccess->instKilled) {
              break;
            }
            preScratchAccess = preScratchAccess->preScratchAccess;
          }
        }

        if (preScratchAccess) {
          if (preScratchAccess->isSpill && preScratchAccess->preFillAccess &&
              preScratchAccess->preFillAccess->instKilled) {
          } else if (!preScratchAccess->instKilled) {
            if (replaceWithPreDcl(builder, scratchAccess, preScratchAccess)) {
              gra.removeEUFusionNoMaskWAInst(*scratchAccess->inst_it);
              bb->erase(scratchAccess->inst_it);
              scratchAccess->instKilled = true;
              scratchAccess->preScratchAccess->useCount--;
              clean_num_profile->fill_clean_num[0]++;
            }
          }
        }
      } else {
        if (preScratchAccess && !preScratchAccess->instKilled) {
          if (replaceWithPreDcl(builder, scratchAccess, preScratchAccess)) {
            gra.removeEUFusionNoMaskWAInst(*scratchAccess->inst_it);
            bb->erase(scratchAccess->inst_it);
            scratchAccess->instKilled = true;
            scratchAccess->preScratchAccess->useCount--;
            clean_num_profile->fill_clean_num[0]++;
          }
        }
      }
    }
#ifdef _DEBUG
    if (clean_num_profile->fill_clean_num[0] > FILL_DEBUG_THRESHOLD)
      return;
#endif
  }

  return;
}

void FlagSpillCleanup::regSpillClean(IR_Builder &builder, G4_BB *bb,
                                     SCRATCH_PTR_VEC &candidateList,
                                     CLEAN_NUM_PROFILE *clean_num_profile) {
  for (SCRATCH_ACCESS *scratchAccess : candidateList) {
    if (scratchAccess->instKilled) {
      continue;
    }
    if (!scratchAccess->instKilled && scratchAccess->isSpill &&
        scratchAccess->removeable && scratchAccess->evicted &&
        scratchAccess->useCount == 0) {
      gra.removeEUFusionNoMaskWAInst(*scratchAccess->inst_it);
      bb->erase(scratchAccess->inst_it);
      scratchAccess->instKilled = true;
      clean_num_profile->spill_clean_num[0]++;
#ifdef _DEBUG
      if (clean_num_profile->spill_clean_num[0] > SPILL_DEBUG_THRESHOLD) {
        return;
      }
#endif
    }
  }

  return;
}

// Replace Scratch Block Read/Write message with OWord Block Read/Write message
// For spill code clean up, clean target may exist in all WAW, RAR, RAW, WAR.
void FlagSpillCleanup::spillFillCodeCleanFlag(
    IR_Builder &builder, G4_Kernel &kernel,
    CLEAN_NUM_PROFILE *clean_num_profile) {
  SCRATCH_PTR_VEC scratchAccessList;
  SCRATCH_PTR_LIST scratchTraceList;
  SCRATCH_PTR_VEC candidateList;
  FlowGraph &fg = kernel.fg;

  int candidate_size = 0;
  for (auto bb : fg) {
    INST_LIST_ITER inst_it = bb->begin();

    scratchTraceList.clear();
    candidateList.clear();
    freeScratchAccess(&scratchAccessList);

    // Top down scan within BB
    while (inst_it != bb->end()) {
      INST_LIST_ITER inst_it_next = inst_it;
      inst_it_next++;

      flagSpillFillClean(bb, inst_it, scratchAccessList, scratchTraceList,
                         candidateList, clean_num_profile);

      inst_it = inst_it_next;
    }

#ifdef _DEBUG
    candidate_size += (int)candidateList.size();
#endif
    // Clean the fills.
    regFillClean(builder, bb, candidateList, clean_num_profile);

#ifdef _DEBUG
    if (clean_num_profile->fill_clean_num[0] > FILL_DEBUG_THRESHOLD)
      return;
#endif
    // Clean the spills
    regSpillClean(builder, bb, scratchAccessList, clean_num_profile);

#ifdef _DEBUG
    if (clean_num_profile->spill_clean_num[0] > SPILL_DEBUG_THRESHOLD) {
      return;
    }
#endif
  }

  freeScratchAccess(&scratchAccessList);
  scratchTraceList.clear();
  candidateList.clear();

  VISA_DEBUG_VERBOSE(printf("Candidate size: %d\n", candidate_size));

  return;
}

// Insert declarations with pre-assigned registers in kernel
// this is needed for HRA, and the fake declares will be removed at the end of
// HRA
void GlobalRA::insertPhyRegDecls() {
  int numGRF = kernel.getNumRegTotal();
  std::vector<bool> grfUsed(numGRF, false);
  GRFDclsForHRA.resize(numGRF);

  for (auto curBB : kernel.fg) {
    if (auto summary = getBBLRASummary(curBB)) {
      for (int i = 0; i < numGRF; i++) {
        if (summary->isGRFBusy(i)) {
          grfUsed[i] = true;
        }
      }
    }
  }

  // Insert declarations for each GRF that is used
  unsigned numGRFsUsed = 0;
  for (int i = 0; i < numGRF; i++) {
    if (grfUsed[i] == true) {
      const char *dclName = builder.getNameString(10, "r%d", i);
      G4_Declare *phyRegDcl =
          builder.createDeclare(dclName, G4_GRF, kernel.numEltPerGRF<Type_UD>(),
                                1, Type_D, Regular, NULL, NULL);
      G4_Greg *phyReg = builder.phyregpool.getGreg(i);
      phyRegDcl->getRegVar()->setPhyReg(phyReg, 0);
      GRFDclsForHRA[i] = phyRegDcl;
      addVarToRA(phyRegDcl);
      numGRFsUsed++;
    }
  }

  VISA_DEBUG(std::cout << "Local RA used " << numGRFsUsed << " GRFs\n");
}

// compute physical register info and adjust foot print
// find indexed GRFs and construct a foot print for them
// set live operand in each instruction
void GlobalRA::computePhyReg() {
  auto &fg = kernel.fg;
  for (auto bb : fg) {
    for (auto inst : *bb) {
      if (inst->isPseudoKill() || inst->isLifeTimeEnd() ||
          inst->isPseudoUse()) {
        continue;
      }

      if (inst->getDst() && !(inst->hasNULLDst())) {
        G4_DstRegRegion *currDstRegion = inst->getDst();
        if (currDstRegion->getBase()->isRegVar() &&
            currDstRegion->getBase()
                    ->asRegVar()
                    ->getDeclare()
                    ->getGRFOffsetFromR0() == 0) {
          // Need to compute linearized offset only once per dcl
          currDstRegion->computePReg(builder);
        }
      }

      for (unsigned j = 0, size = inst->getNumSrc(); j < size; j++) {
        G4_Operand *curr_src = inst->getSrc(j);
        if (!curr_src || curr_src->isImm() ||
            (inst->opcode() == G4_math && j == 1 && curr_src->isNullReg()) ||
            curr_src->isLabel()) {
          continue;
        }

        if (curr_src->isSrcRegRegion() &&
            curr_src->asSrcRegRegion()->getBase() &&
            curr_src->asSrcRegRegion()->getBase()->isRegVar() &&
            curr_src->asSrcRegRegion()
                    ->getBase()
                    ->asRegVar()
                    ->getDeclare()
                    ->getGRFOffsetFromR0() == 0) {
          curr_src->asSrcRegRegion()->computePReg(builder);
        }
      }
    }
  }
}

void GraphColor::dumpRegisterPressure() {
  RPE rpe(gra, &liveAnalysis);
  uint32_t max = 0;
  std::vector<G4_INST *> maxInst;
  rpe.run();

  for (auto bb : builder.kernel.fg) {
    std::cerr << "BB " << bb->getId() << ": (Pred: ";
    for (auto pred : bb->Preds) {
      std::cerr << pred->getId() << ",";
    }
    std::cerr << " Succ: ";
    for (auto succ : bb->Succs) {
      std::cerr << succ->getId() << ",";
    }
    std::cerr << ")\n";
    for (auto inst : *bb) {
      uint32_t pressure = rpe.getRegisterPressure(inst);
      if (pressure > max) {
        max = pressure;
        maxInst.clear();
        maxInst.push_back(inst);
      } else if (pressure == max) {
        maxInst.push_back(inst);
      }

      std::cerr << "[" << pressure << "] ";
      inst->dump();
    }
  }
  std::cerr << "max pressure: " << max << ", " << maxInst.size()
            << " inst(s)\n";
  for (auto inst : maxInst) {
    inst->dump();
  }
}

void GlobalRA::fixAlignment() {
  // Copy over alignment from G4_RegVar to GlobalRA instance
  // Rest of RA shouldnt have to read/modify alignment of G4_RegVar
  copyAlignment();

  for (auto dcl : kernel.Declares) {
    if (dcl->getRegFile() & G4_FLAG) {
      if (dcl->getByteSize() > 2 ||
          (kernel.getSimdSize() == g4::SIMD32 &&
           kernel.getInt32KernelAttr(Attributes::ATTR_Target) != VISA_CM))
        setSubRegAlign(dcl, G4_SubReg_Align::Even_Word);
    }
  }

  if (builder.getPlatform() == GENX_BDW) {
    // BDW requires even_word alignment for scalar HF variables
    for (auto dcl : kernel.Declares) {
      if (dcl->getElemType() == Type_HF && dcl->getSubRegAlign() == Any) {
        setSubRegAlign(dcl, Even_Word);
      }
    }
  }

  // ToDo: remove these as it should be done by HWConformity
  for (auto BB : kernel.fg) {
    for (auto inst : *BB) {
      G4_DstRegRegion *dst = inst->getDst();
      if (dst && dst->getTopDcl()) {
        G4_RegVar *var = dst->getBase()->asRegVar();
        if (inst->isSend() && dst->getRegAccess() == Direct) {
          if (!var->isPhyRegAssigned()) {
            setSubRegAlign(dst->getTopDcl(), builder.getGRFAlign());
          }
        }

        if (!var->isPhyRegAssigned() && var->getDeclare()->getNumRows() <= 1 &&
            dst->getRegAccess() == Direct &&
            var->getDeclare()->getSubRegAlign() == Any) {
          if (inst->isAccSrcInst()) {
            setSubRegAlign(dst->getTopDcl(),
                           var->getDeclare()->getRegFile() != G4_ADDRESS
                               ? builder.getGRFAlign()
                               : Eight_Word);
          }
        }
      }
    }
  }
}

void VerifyAugmentation::verifyAlign(G4_Declare *dcl) {
  // Verify that dcl with Default32Bit align mask are 2GRF aligned
  auto it = masks.find(dcl);
  if (it == masks.end())
    return;

  if (dcl->getByteSize() >=
          kernel->numEltPerGRF<Type_UD>() * TypeSize(Type_UD) &&
      dcl->getByteSize() <=
          2 * kernel->numEltPerGRF<Type_UD>() * TypeSize(Type_UD) &&
      kernel->getSimdSize() > kernel->numEltPerGRF<Type_UD>()) {
    auto assignment = dcl->getRegVar()->getPhyReg();
    if (assignment && assignment->isGreg()) {
      auto phyRegNum = assignment->asGreg()->getRegNum();
      auto augMask = std::get<1>((*it).second);
      if (phyRegNum % 2 != 0 && augMask == AugmentationMasks::Default32Bit) {
        printf("Dcl %s is Default32Bit but assignment is not Even aligned\n",
               dcl->getName());
      }
    }
  }
}

void VerifyAugmentation::dump(const char *dclName) {
  std::string dclStr = dclName;
  for (auto &m : masks) {
    std::string first = m.first->getName();
    if (first == dclStr) {
      printf("%s, %d, %s\n", dclName, m.first->getRegVar()->getId(),
             getStr(std::get<1>(m.second)));
    }
  }
}

void VerifyAugmentation::labelBBs() {
  std::string prev = "X:";
  unsigned id = 0;
  for (auto bb : kernel->fg) {
    if (bbLabels.find(bb) == bbLabels.end())
      bbLabels[bb] = prev;
    else
      prev = bbLabels[bb];

    if (bb->back()->opcode() == G4_opcode::G4_if) {
      auto TBB = bb->Succs.front();
      auto FBB = bb->Succs.back();

      bool hasEndif = false;
      for (auto inst : *FBB) {
        if (inst->opcode() == G4_opcode::G4_endif) {
          hasEndif = true;
          break;
        }
      }

      bbLabels[TBB] = prev + "T" + std::to_string(id) + ":";

      if (!hasEndif) {
        // else
        bbLabels[FBB] = prev + "F" + std::to_string(id) + ":";
      } else {
        // endif block
        bbLabels[FBB] = prev;
      }

      prev = prev + "T" + std::to_string(id) + ":";

      id++;
    } else if (bb->back()->opcode() == G4_opcode::G4_else) {
      auto succBB = bb->Succs.front();
      auto lbl = prev;
      lbl.pop_back();
      while (lbl.back() != ':') {
        lbl.pop_back();
      }

      bbLabels[succBB] = lbl;
    } else if (bb->back()->opcode() == G4_opcode::G4_endif) {
    }
  }

#if 1
  for (auto bb : kernel->fg) {
    printf("BB%d -> %s\n", bb->getId(), bbLabels[bb].data());
  }
#endif
}

unsigned VerifyAugmentation::getGRFBaseOffset(const G4_Declare *dcl) const {
  unsigned regNum = dcl->getRegVar()->getPhyReg()->asGreg()->getRegNum();
  unsigned regOff = dcl->getRegVar()->getPhyRegOff();
  auto type = dcl->getElemType();
  return (regNum * kernel->numEltPerGRF<Type_UB>()) + (regOff * TypeSize(type));
}

bool VerifyAugmentation::interfereBetween(G4_Declare *dcl1, G4_Declare *dcl2) {
  bool interferes = true;
  unsigned v1 = dcl1->getRegVar()->getId();
  unsigned v2 = dcl2->getRegVar()->getId();
  bool v1Partaker = dcl1->getRegVar()->isRegAllocPartaker();
  bool v2Partaker = dcl2->getRegVar()->isRegAllocPartaker();

  if (v1Partaker && v2Partaker) {
    auto interferes = intf->interfereBetween(v1, v2);
    if (!interferes) {
      if (dcl1->getIsPartialDcl()) {
        interferes |= intf->interfereBetween(
            gra->getSplittedDeclare(dcl1)->getRegVar()->getId(), v2);
        if (dcl2->getIsPartialDcl()) {
          interferes |= intf->interfereBetween(
              v1, gra->getSplittedDeclare(dcl2)->getRegVar()->getId());
          interferes |= intf->interfereBetween(
              gra->getSplittedDeclare(dcl1)->getRegVar()->getId(),
              gra->getSplittedDeclare(dcl2)->getRegVar()->getId());
        }
      } else if (dcl2->getIsPartialDcl()) {
        interferes |= intf->interfereBetween(
            v1, gra->getSplittedDeclare(dcl2)->getRegVar()->getId());
      }
    }
    return interferes;
  } else if (!v1Partaker && v2Partaker) {
    // v1 is assigned by LRA
    unsigned startGRF = dcl1->getRegVar()->getPhyReg()->asGreg()->getRegNum();
    unsigned numGRFs = dcl1->getNumRows();

    for (unsigned grf = startGRF; grf != (startGRF + numGRFs); grf++) {
      for (unsigned var = 0; var != numVars; var++) {
        if (lrs[var] &&
            lrs[var]->getPhyReg() ==
                kernel->fg.builder->phyregpool.getGreg(grf) &&
            std::string(lrs[var]->getVar()->getName()) ==
                "r" + std::to_string(grf)) {
          if (!intf->interfereBetween(var, v2)) {
            interferes = false;
          }
        }
      }
    }
  } else if (v1Partaker && !v2Partaker) {
    return interfereBetween(dcl2, dcl1);
  } else if (!v1Partaker && !v2Partaker) {
    // both assigned by LRA
    if (dcl1->getRegFile() == G4_RegFileKind::G4_GRF &&
        dcl2->getRegFile() == G4_RegFileKind::G4_GRF) {
      auto lr1 = gra->getLocalLR(dcl1);
      auto lr2 = gra->getLocalLR(dcl2);

      if (lr1->getAssigned() && lr2->getAssigned()) {
        auto preg1Start = getGRFBaseOffset(dcl1);
        auto preg2Start = getGRFBaseOffset(dcl2);
        auto preg1End = preg1Start + dcl1->getByteSize();
        auto preg2End = preg2Start + dcl2->getByteSize();

        if (preg2Start >= preg1Start && preg2Start < preg1End) {
          return false;
        } else if (preg1Start >= preg2Start && preg1Start < preg2End) {
          return false;
        }
      }
    }

    interferes = true;
  }

  return interferes;
}

void VerifyAugmentation::verify() {
  std::cerr << "Start verification for kernel: "
            << kernel->getOptions()->getOptionCstr(VISA_AsmFileName) << "\n";

  for (auto dcl : kernel->Declares) {
    if (dcl->getIsSplittedDcl()) {
      auto &tup = masks[dcl];
      std::cerr << dcl->getName() << "(" << getStr(std::get<1>(tup))
                << ") is split"
                << "\n";
      for (const G4_Declare *subDcl : gra->getSubDclList(dcl)) {
        auto &tupSub = masks[subDcl];
        std::cerr << "\t" << subDcl->getName() << " ("
                  << getStr(std::get<1>(tupSub)) << ")"
                  << "\n";
      }
    }
  }

  std::cerr << "\n"
            << "\n"
            << "\n";

  auto overlapDcl = [this](G4_Declare *dcl1, G4_Declare *dcl2) {
    if (dcl1->getRegFile() == G4_RegFileKind::G4_GRF &&
        dcl2->getRegFile() == G4_RegFileKind::G4_GRF) {
      auto preg1Start = getGRFBaseOffset(dcl1);
      auto preg2Start = getGRFBaseOffset(dcl2);
      auto preg1End = preg1Start + dcl1->getByteSize();
      auto preg2End = preg2Start + dcl2->getByteSize();

      if (preg2Start >= preg1Start && preg2Start < preg1End) {
        return true;
      } else if (preg1Start >= preg2Start && preg1Start < preg2End) {
        return true;
      }
    }
    return false;
  };

  std::list<G4_Declare *> active;
  for (auto dcl : sortedLiveRanges) {
    auto &tup = masks[dcl];
    unsigned startIdx = std::get<2>(tup)->getLexicalId();
    auto dclMask = std::get<1>(tup);

    auto getMaskStr = [](AugmentationMasks m) {
      std::string str = "Undetermined";
      if (m == AugmentationMasks::Default16Bit)
        str = "Default16Bit";
      else if (m == AugmentationMasks::Default32Bit)
        str = "Default32Bit";
      else if (m == AugmentationMasks::Default64Bit)
        str = "Default64Bit";
      else if (m == AugmentationMasks::NonDefault)
        str = "NonDefault";
      else if (m == AugmentationMasks::DefaultPredicateMask)
        str = "DefaultPredicateMask";
      str.append("\n");

      return str;
    };

    std::cerr << dcl->getName() << " - " << getMaskStr(dclMask);

    verifyAlign(dcl);

    for (auto it = active.begin(); it != active.end();) {
      auto activeDcl = (*it);
      auto &tupActive = masks[activeDcl];
      if (startIdx >= std::get<3>(tupActive)->getLexicalId()) {
        it = active.erase(it);
        continue;
      }
      it++;
    }

    for (auto activeDcl : active) {
      auto &tupActive = masks[activeDcl];
      auto aDclMask = std::get<1>(tupActive);

      if (dclMask != aDclMask) {
        bool interfere = interfereBetween(activeDcl, dcl);

        if (activeDcl->getIsPartialDcl() || dcl->getIsPartialDcl())
          continue;

        if (!interfere) {
          std::cerr << dcl->getRegVar()->getName() << "(" << getStr(dclMask)
                    << ") and " << activeDcl->getRegVar()->getName() << "("
                    << getStr(aDclMask)
                    << ") are overlapping with incompatible emask but not "
                       "masked as interfering"
                    << "\n";
        }

        if (overlapDcl(activeDcl, dcl)) {
          if (!interfere) {
            std::cerr << dcl->getRegVar()->getName() << "(" << getStr(dclMask)
                      << ") and " << activeDcl->getName() << "("
                      << getStr(aDclMask)
                      << ") use overlapping physical assignments but not "
                         "marked as interfering"
                      << "\n";
          }
        }
      }
    }

    active.push_back(dcl);
  }

  std::cerr << "End verification for kenel: "
            << kernel->getOptions()->getOptionCstr(VISA_AsmFileName) << "\n"
            << "\n"
            << "\n";

  return;

#if 0
    // Following is useful for debugging when test has only if-else-endif constructs
    labelBBs();
    populateBBLexId();
    std::string msg;
    for (auto dcl : sortedLiveRanges)
    {
        auto lr = DclLRMap[dcl];
        if (lr->getPhyReg() && isClobbered(lr, msg))
        {
            printf("%s clobbered:\n\t%s\n\n", dcl->getName(), msg.data());
        }
    }
#endif
}

void VerifyAugmentation::populateBBLexId() {
  for (auto bb : kernel->fg) {
    if (bb->size() > 0)
      BBLexId.push_back(std::make_tuple(bb, bb->front()->getLexicalId(),
                                        bb->back()->getLexicalId()));
  }
}

bool VerifyAugmentation::isClobbered(LiveRange *lr, std::string &msg) {
  msg.clear();

  auto &tup = masks[lr->getDcl()];

  auto startLexId = std::get<2>(tup)->getLexicalId();
  auto endLexId = std::get<3>(tup)->getLexicalId();

  std::vector<std::pair<G4_INST *, G4_BB *>> insts;
  std::vector<std::tuple<INST_LIST_ITER, G4_BB *>> defs;
  std::vector<std::tuple<INST_LIST_ITER, G4_BB *>> uses;

  for (auto bb : kernel->fg) {
    if (bb->size() == 0)
      continue;

    if (bb->back()->getLexicalId() > endLexId &&
        bb->front()->getLexicalId() > endLexId)
      continue;

    if (bb->back()->getLexicalId() < startLexId &&
        bb->front()->getLexicalId() < startLexId)
      continue;

    // lr is active in current bb
    for (auto instIt = bb->begin(), end = bb->end(); instIt != end; instIt++) {
      auto inst = (*instIt);
      if (inst->isPseudoKill())
        continue;

      if (inst->getLexicalId() > startLexId &&
          inst->getLexicalId() <= endLexId) {
        insts.push_back(std::make_pair(inst, bb));
        auto dst = inst->getDst();
        if (dst && dst->isDstRegRegion()) {
          auto topdcl = dst->asDstRegRegion()->getTopDcl();
          if (topdcl == lr->getDcl())
            defs.push_back(std::make_tuple(instIt, bb));
        }

        for (unsigned i = 0, numSrc = inst->getNumSrc(); i != numSrc; i++) {
          auto src = inst->getSrc(i);
          if (src && src->isSrcRegRegion()) {
            auto topdcl = src->asSrcRegRegion()->getTopDcl();
            if (topdcl == lr->getDcl())
              uses.push_back(std::make_tuple(instIt, bb));
          }
        }
      }
    }
  }

  for (auto &use : uses) {
    auto &useStr = bbLabels[std::get<1>(use)];
    auto inst = *std::get<0>(use);
    vISA_ASSERT(useStr.size() > 0, "empty string found");
    std::list<std::tuple<G4_INST *, G4_BB *>> rd;

    for (unsigned i = 0, numSrc = inst->getNumSrc(); i != numSrc; i++) {
      auto src = inst->getSrc(i);
      if (src && src->isSrcRegRegion() &&
          src->asSrcRegRegion()->getTopDcl() == lr->getDcl()) {
        unsigned lb = 0, rb = 0;
        lb = lr->getPhyReg()->asGreg()->getRegNum() *
                 kernel->numEltPerGRF<Type_UB>() +
             (lr->getPhyRegOff() * lr->getDcl()->getElemSize());
        lb += src->getLeftBound();
        rb = lb + src->getRightBound() - src->getLeftBound();

        for (auto &otherInsts : insts) {
          if (otherInsts.first->getLexicalId() > inst->getLexicalId())
            break;

          auto oiDst = otherInsts.first->getDst();
          auto oiBB = otherInsts.second;
          if (oiDst && oiDst->isDstRegRegion() && oiDst->getTopDcl()) {
            unsigned oilb = 0, oirb = 0;
            auto oiLR = DclLRMap[oiDst->getTopDcl()];
            if (oiLR && !oiLR->getPhyReg())
              continue;

            oilb = oiLR->getPhyReg()->asGreg()->getRegNum() *
                       kernel->numEltPerGRF<Type_UB>() +
                   (oiLR->getPhyRegOff() * oiLR->getDcl()->getElemSize());
            oilb += oiDst->getLeftBound();
            oirb = oilb + oiDst->getRightBound() - oiDst->getLeftBound();

            if (oilb <= (unsigned)rb && oirb >= (unsigned)lb) {
              rd.push_back(std::make_tuple(otherInsts.first, oiBB));
            }
          }
        }
      }
    }

    auto isComplementary = [](std::string &cur, std::string &other) {
      if (cur.size() < other.size())
        return false;

      if (cur.substr(0, other.size() - 1) ==
          other.substr(0, other.size() - 1)) {
        char lastAlphabet = cur.at(other.size() - 1);
        if (lastAlphabet == 'T' && other.back() == 'F')
          return true;
        if (lastAlphabet == 'F' && other.back() == 'T')
          return true;
      }

      return false;
    };

    auto isSameEM = [](G4_INST *inst1, G4_INST *inst2) {
      if (inst1->getMaskOption() == inst2->getMaskOption() &&
          inst1->getMaskOffset() == inst2->getMaskOffset())
        return true;
      return false;
    };

    if (rd.size() > 0) {
      printf("Current use str = %s for inst:\t", useStr.data());
      inst->emit(std::cerr);
      printf("\t$%d\n", inst->getVISAId());
    }
    // process all reaching defs
    for (auto rid = rd.begin(); rid != rd.end();) {
      auto &reachingDef = (*rid);

      auto &str = bbLabels[std::get<1>(reachingDef)];

      // skip rd if it is from complementary branch
      if (isComplementary(str, useStr) &&
          isSameEM(inst, std::get<0>(reachingDef))) {
        rid = rd.erase(rid);
        continue;
      }
      rid++;
    }

    // keep rd that appears last in its BB
    for (auto rid = rd.begin(); rid != rd.end();) {
      auto ridBB = std::get<1>(*rid);
      for (auto rid1 = rd.begin(); rid1 != rd.end();) {
        if (*rid == *rid1) {
          rid1++;
          continue;
        }

        auto rid1BB = std::get<1>(*rid1);
        if (ridBB == rid1BB && std::get<0>(*rid)->getLexicalId() >
                                   std::get<0>(*rid1)->getLexicalId()) {
#if 0
                    printf("\tErasing inst at $%d due to later def at $%d\n", std::get<0>(*rid1)->getLexicalId(),
                        std::get<0>(*rid)->getLexicalId());
#endif
          rid1 = rd.erase(rid1);
          continue;
        }
        rid1++;
      }

      if (rid != rd.end())
        rid++;
    }

    if (rd.size() > 0) {
      bool printed = false;
      // display left overs in rd from different dcl
      for (auto &reachingDef : rd) {
        if (std::get<0>(reachingDef)->getDst()->getTopDcl() ==
            lr->getDcl()->getRootDeclare())
          continue;

        if (inst->getVISAId() == std::get<0>(reachingDef)->getVISAId())
          continue;

        if (!printed) {
          printf("\tLeft-over rd:\n");
          printed = true;
        }
        printf("\t");
        std::get<0>(reachingDef)->emit(std::cerr);
        printf("\t$%d\n", std::get<0>(reachingDef)->getVISAId());
      }
    }
  }

  return false;
}

void VerifyAugmentation::loadAugData(std::vector<G4_Declare *> &s,
                                     const LiveRangeVec &l,
                                     unsigned n, const Interference *i,
                                     GlobalRA &g) {
  reset();
  sortedLiveRanges = s;
  gra = &g;
  kernel = &gra->kernel;
  lrs = l;
  numVars = n;
  intf = i;

  for (unsigned i = 0; i != numVars; i++) {
    DclLRMap[lrs[i]->getDcl()] = lrs[i];
  }
  for (auto dcl : kernel->Declares) {
    if (dcl->getRegFile() == G4_RegFileKind::G4_GRF ||
        dcl->getRegFile() == G4_RegFileKind::G4_INPUT) {
      LiveRange *lr = nullptr;
      auto it = DclLRMap.find(dcl);
      if (it != DclLRMap.end()) {
        lr = it->second;
      }
      auto start = gra->getStartInterval(dcl);
      auto end = gra->getEndInterval(dcl);
      masks[dcl] =
          std::make_tuple(lr, gra->getAugmentationMask(dcl), start, end);
    }
  }
}

//
// DFS to check if there is any conflict in subroutine return location
//
bool GlobalRA::isSubRetLocConflict(G4_BB *bb, std::vector<unsigned> &usedLoc,
                                   unsigned stackTop) {
  auto &fg = kernel.fg;
  if (bb->isAlreadyTraversed(fg.getTraversalNum()))
    return false;
  bb->markTraversed(fg.getTraversalNum());

  G4_INST *lastInst = bb->size() == 0 ? NULL : bb->back();
  if (lastInst && lastInst->isReturn()) {
    if (lastInst->getPredicate() == NULL)
      return false;
    else {
      return isSubRetLocConflict(bb->fallThroughBB(), usedLoc, stackTop);
    }
  } else if (lastInst && lastInst->isCall()) // need to traverse to next level
  {
    unsigned curSubRetLoc = getSubRetLoc(bb);
    //
    // check conflict firstly
    //
    for (unsigned i = 0; i < stackTop; i++)
      if (usedLoc[i] == curSubRetLoc)
        return true;
    //
    // then traverse all the subroutines and return BB
    //
    usedLoc[stackTop] = curSubRetLoc;
    unsigned afterCallId = bb->BBAfterCall()->getId();

    // call can have 1 or 2 successors
    // If it has 1 then it is sub-entry block, if it has 2
    // then call has to be predicated. In case of predication,
    // 1st successor is physically following BB, 2nd is
    // sub-entry.
    if (lastInst->getPredicate()) {
      vISA_ASSERT(bb->Succs.size() == 2,
                  "Expecting 2 successor BBs for predicated call");
      if (isSubRetLocConflict(bb->Succs.back(), usedLoc, stackTop))
        return true;
    }

    if (bb->BBAfterCall()->getId() == afterCallId) {
      if (isSubRetLocConflict(bb->BBAfterCall(), usedLoc, stackTop))
        return true;
    }
  } else {
    for (G4_BB *succ : bb->Succs)
      if (isSubRetLocConflict(succ, usedLoc, stackTop))
        return true;
  }

  return false;
}

//
// The routine traverses all BBs that can be reached from the entry of a
// subroutine (not traversing into nested subroutine calls). Mark retLoc[bb] =
// entryId (to associate bb with the subroutine entry. When two subroutines
// share code, we return the location of the subroutine that was previously
// traversed so that the two routines can then use the same location to save
// their return addresses.
//
unsigned GlobalRA::determineReturnAddrLoc(unsigned entryId,
                                          std::vector<unsigned> &retLoc,
                                          G4_BB *bb) {
  auto &fg = kernel.fg;
  if (bb->isAlreadyTraversed(fg.getTraversalNum()))
    return retLoc[bb->getId()];
  bb->markTraversed(fg.getTraversalNum());

  if (retLoc[bb->getId()] != UNDEFINED_VAL)
    return retLoc[bb->getId()];

  retLoc[bb->getId()] = entryId;
  G4_INST *lastInst = bb->size() == 0 ? NULL : bb->back();

  if (lastInst && lastInst->isReturn()) {
    if (!lastInst->getPredicate())
      return entryId;
    return determineReturnAddrLoc(entryId, retLoc, bb->fallThroughBB());
  } else if (lastInst && lastInst->isCall()) {
    // skip nested subroutine calls
    return determineReturnAddrLoc(entryId, retLoc, bb->BBAfterCall());
  }
  unsigned sharedId = entryId;
  for (G4_BB *succ : bb->Succs) {
    unsigned loc = determineReturnAddrLoc(entryId, retLoc, succ);
    if (loc != entryId) {
      while (retLoc[loc] != loc) // find the root of subroutine loc
        loc = retLoc[loc];       // follow the link to reach the root
      if (sharedId == entryId) {
        sharedId = loc;
      } else if (sharedId != loc) {
        //
        // The current subroutine share code with two other subroutines, we
        // force all three of them to use the same location by linking them
        // togethers.
        //
        retLoc[loc] = sharedId;
      }
    }
  }
  return sharedId;
}

void GlobalRA::assignLocForReturnAddr() {
  auto &fg = kernel.fg;
  std::vector<unsigned> retLoc(fg.getNumBB(), UNDEFINED_VAL);
  // a data structure for doing a quick map[id] ---> block
  // FIXME: I have no idea why we need this vector, do we have to iterate the
  // blocks by their id for some reason?
  std::vector<G4_BB *> BBs(fg.getNumBB());
  for (G4_BB *bb : fg) {
    unsigned i = bb->getId();
    BBs[i] = bb; // BBs are sorted by ID
  }

  //
  // Firstly, keep the original algorithm unchanged to mark the retLoc
  //
  std::vector<G4_BB *> caller; // just to accelerate the algorithm later

  for (unsigned i = 0, bbNum = fg.getNumBB(); i < bbNum; i++) {
    G4_BB *bb = BBs[i];
    if (bb->isEndWithCall() == false) {
      continue;
    }

#ifdef _DEBUG
    G4_INST *last = bb->empty() ? NULL : bb->back();
    vISA_ASSERT(last, ERROR_FLOWGRAPH);
#endif

    caller.push_back(
        bb); // record the callers, just to accelerate the algorithm

    G4_BB *subEntry = bb->getCalleeInfo()->getInitBB();
    if (retLoc[subEntry->getId()] !=
        UNDEFINED_VAL) // a loc has been assigned to the subroutine
    {
      // Need to setSubRetLoc if subEntry is part of another subRoutine because,
      // in the final phase, we use SubRetLoc != UNDEFINED_VAL to indicate
      // a block is an entry of a subroutine.
      setSubRetLoc(subEntry, retLoc[subEntry->getId()]);
    } else {
      fg.prepareTraversal();
      unsigned loc =
          determineReturnAddrLoc(subEntry->getId(), retLoc, subEntry);
      if (loc != subEntry->getId()) {
        retLoc[subEntry->getId()] = loc;
      }
      setSubRetLoc(subEntry, loc);
      //
      // We do not merge indirect call here, because it will createt additional
      // (bb->getSubRetLoc() != bb->getId())  cases that kill the share code
      // detection
      //
    }

    // retBB is the exit basic block of callee, ie the block with return
    // statement at end
    G4_BB *retBB = bb->getCalleeInfo()->getExitBB();

    if (retLoc[retBB->getId()] == UNDEFINED_VAL) {
      // retBB block was unreachable so retLoc element corresponding to that
      // block was left undefined
      retLoc[retBB->getId()] = getSubRetLoc(subEntry);
    }
  }
  VISA_DEBUG_VERBOSE({
    std::cout << "\nBefore merge indirect call:\n";
    for (unsigned i = 0; i < fg.getNumBB(); i++)
      if (retLoc[i] == UNDEFINED_VAL) {
        std::cout << "BB" << i << ": X   ";
      } else {
        std::cout << "BB" << i << ": " << retLoc[i] << "   ";
      }
    std::cout << "\n";
  });

  //
  // this final phase is needed. Consider the following scenario.  Sub2 shared
  // code with both Sub1 and Sub3. All three must use the same location to save
  // return addresses. If we traverse Sub1 then Sub3, retLoc[Sub1] and
  // retLoc[Sub3] all point to their own roots.  As we traverse Sub2, code
  // sharing is detected, we need to this phase to make sure that Sub1 and Sub3
  // use the same location.
  //
  for (unsigned i = 0, bbNum = fg.getNumBB(); i < bbNum; i++) {
    G4_BB *bb = BBs[i];
    if (getSubRetLoc(bb) != UNDEFINED_VAL) {
      if (getSubRetLoc(bb) != bb->getId()) {
        unsigned loc = bb->getId();
        while (retLoc[loc] != loc) // not root
          loc = retLoc[loc];       // follow the link to reach the root
      }
    }
  }

  //
  // Merge the retLoc in indirect call cases
  //
  for (G4_BB *bb : caller) {
    G4_INST *last = bb->empty() ? NULL : bb->back();
    vISA_ASSERT(last, ERROR_FLOWGRAPH);

    unsigned fallThroughId = bb->fallThroughBB() == NULL
                                 ? UNDEFINED_VAL
                                 : bb->fallThroughBB()->getId();
    if ((last && last->getPredicate() == NULL && bb->Succs.size() > 1) ||
        (last && last->getPredicate() != NULL && bb->Succs.size() > 2)) {
      //
      // merge all subroutines to the last one, it is a trick to conduct the
      // conditional call by using last one instead of first one
      //
      unsigned masterEntryId = bb->Succs.back()->getId();
      //
      // find the root of the master subroutine
      //
      unsigned masterRetLoc = masterEntryId;
      while (retLoc[masterRetLoc] != masterRetLoc)
        masterRetLoc = retLoc[masterRetLoc];
      //
      // check other subroutines in one vertex
      //
      for (G4_BB *subBB : bb->Succs) {
        if (subBB->getId() != masterEntryId &&
            subBB->getId() != fallThroughId) {
          //
          // find the root of the current subroutine
          //
          unsigned loc = subBB->getId();
          while (retLoc[loc] != loc)
            loc = retLoc[loc];
          //
          // Merge: let all the items in retLoc with value loc pointing to
          // masterRetLoc Suppose indirect call X calls subroutine A and B,
          // indirect call Y calls B and C, and indirect call Z calls C and D.
          // Before merge, the A~D will be assigned different return location.
          // Suppose we process the callers in order X-->Z-->Y in the merge, if
          // we just modified the return locations of one indirect call, we will
          // fail to merge the return locations of A~D.
          //
          if (loc != masterRetLoc) {
            for (unsigned i = 0; i < fg.getNumBB(); i++)
              if (retLoc[i] == loc)
                retLoc[i] = masterRetLoc;
          }
        }
      }
    }
  }

  VISA_DEBUG_VERBOSE({
    std::cout << "\nAfter merge indirect call:\n";
    for (unsigned i = 0; i < fg.getNumBB(); i++)
      if (retLoc[i] == UNDEFINED_VAL) {
        std::cout << "BB" << i << ": X   ";
      } else {
        std::cout << "BB" << i << ": " << retLoc[i] << "   ";
      }
    std::cout << "\n";
  });

  //
  //  Assign ret loc for subroutines firstly, and then check if it is wrong (due
  //  to circle in call graph).
  //
  for (unsigned i = 0, bbNum = fg.getNumBB(); i < bbNum; i++) {
    //
    // reset the return BB's retLoc
    //
    unsigned loc = i;
    if (retLoc[i] != UNDEFINED_VAL) {
      while (retLoc[loc] != loc)
        loc = retLoc[loc];
      retLoc[i] = loc;
      setSubRetLoc(BBs[i], retLoc[loc]);
    }
  }

  for (G4_BB *bb : caller) {
    //
    // set caller BB's retLoc
    //
#ifdef _DEBUG
    G4_INST *last = bb->empty() ? NULL : bb->back();
    vISA_ASSERT(last, ERROR_FLOWGRAPH);
#endif
    G4_BB *subBB = bb->getCalleeInfo()->getInitBB();
    //
    // 1: Must use retLoc here, because some subBB is also the caller of another
    // subroutine, so the entry loc in BB may be changed in this step 2: In some
    // cases, the caller BB is also the entry BB. At this time, the associated
    // entry BB ID will be overwritten. However, it will not impact the conflict
    // detection and return location assignment, since we only check the return
    // BB and/or caller BB in these two moudles.
    //
    setSubRetLoc(bb, retLoc[subBB->getId()]);
  }

  VISA_DEBUG_VERBOSE({
    for (unsigned i = 0; i < fg.getNumBB(); i++) {
      G4_BB *bb = BBs[i];
      if (getSubRetLoc(bb) != UNDEFINED_VAL) {
        if (!bb->empty() && bb->front()->isLabel()) {
          std::cout << ((G4_Label *)bb->front()->getSrc(0))->getLabel()
                    << " assigned location " << getSubRetLoc(bb) << "\n";
        }
      }
    }
  });

  //
  // detect the conflict (circle) at last
  //
  std::vector<unsigned> usedLoc(fg.getNumBB());
  unsigned stackTop = 0;
  for (G4_BB *bb : caller) {
    //
    // Must re-start the traversal from each caller, otherwise will lose some
    // circle cases like TestRA_Call_1_1_3B, D, F, G, H
    //
    fg.prepareTraversal();

    usedLoc[stackTop] = getSubRetLoc(bb);

    G4_BB *subEntry = bb->Succs.back();

    if (isSubRetLocConflict(subEntry, usedLoc, stackTop + 1)) {
      vISA_ASSERT(false, "ERROR: Fail to assign call-return variables due to "
                         "cycle in call graph!");
    }
  }

  insertCallReturnVar();
}

void GlobalRA::insertCallReturnVar() {
  for (auto bb : kernel.fg) {
    G4_INST *last = bb->empty() ? NULL : bb->back();
    if (last) {
      if (last->isCall()) {
        insertSaveAddr(bb);
      } else {
        if (last->isReturn()) {
          // G4_BB_EXIT_TYPE is just a dummy BB, and the return will be the last
          // inst in each of its predecessors
          insertRestoreAddr(bb);
        }
      }
    }
  }
}

void GlobalRA::insertSaveAddr(G4_BB *bb) {
  vISA_ASSERT(bb != NULL, ERROR_INTERNAL_ARGUMENT);
  vISA_ASSERT(getSubRetLoc(bb) != UNDEFINED_VAL,
              ERROR_FLOWGRAPH); // must have a assigned loc

  G4_INST *last = bb->back();
  vASSERT(last->isCall());
  if (last->getDst() == NULL) {
    unsigned loc = getSubRetLoc(bb);
    G4_Declare *dcl = getRetDecl(loc);

    last->setDest(builder.createDst(dcl->getRegVar(), 0, 0, 1,
                                    Type_UD)); // RET__loc12<1>:ud

    last->setExecSize(g4::SIMD2);
  }
}

void GlobalRA::insertRestoreAddr(G4_BB *bb) {
  vISA_ASSERT(bb != NULL, ERROR_INTERNAL_ARGUMENT);

  G4_INST *last = bb->back();
  vASSERT(last->isReturn());
  if (last->getSrc(0) == NULL) {
    unsigned loc = getSubRetLoc(bb);
    G4_Declare *dcl = getRetDecl(loc);

    G4_SrcRegRegion *new_src = builder.createSrc(
        dcl->getRegVar(), 0, 0, builder.createRegionDesc(0, 2, 1), Type_UD);

    last->setSrc(new_src, 0);
    last->setDest(builder.createNullDst(Type_UD));

    last->setExecSize(g4::SIMD2);
  }
}

// This function returns the weight of interference edge lr1--lr2,
// which is used for computing the degree of lr1.
//
// When there is no alignment restriction, we should use the normal weight,
// which is lr1_nreg + lr2_nreg - 1.
//
// Otherewise, we need to take into account additional space that may be
// required because of the alignment restriction. For example,
// if lr1 has even alignment and lr2 has no alignment restriction,
// we need to consider the following cases that would require the
// maximal available GRF space for successful allocation:
// 1) lr1's size is odd, lr2's size is odd and lr2's start position is even,
//    the total space required would be (lr1_nreg + lr2_nreg + 1)
// 2) lr1's size is odd, lr2's size is even and lr2's start position is even,
//    the total space required would be (lr1_nreg + lr2_nreg)
// 3) lr1's size is even, lr2's size is odd and lr2's start position is odd,
//    the total space required would be (lr1_nreg + lr2_nreg)
// 4) lr1's size is even, lr2's size is even and lr2's start position is odd,
//    the total space required would be (lr1_nreg + lr2_nreg + 1)
// The above logic can be simplified to the following formula:
//    lr1_nreg + lr2_nreg + 1 - ((lr1_nreg + lr2_nreg) % 2)
//
// If both lr1 and lr2 have even alignment restriction,
// we need to consider the following cases that would require the
// maximal available GRF space for successful allocation:
// 1) lr1's size is odd, lr2's size is odd and lr2's start position is even,
//    the total space required would be (lr1_nreg + lr2_nreg + 1)
// 2) lr1's size is odd, lr2's size is even and lr2's start position is even,
//    the total space required would be (lr1_nreg + lr2_nreg)
// 3) lr1's size is even, lr2's size is odd and lr2's start position is even,
//    the total space required would be (lr1_nreg + lr2_nreg)
// 4) lr1's size is even, lr2's size is even and lr2's start position is even,
//    the total space required would be (lr1_nreg + lr2_nreg - 1)
// The above logic can be simplified to the following formula:
//    lr1_nreg + lr2_nreg - 1 + (lr1_nreg % 2) + (lr2_nreg % 2)
//
// Note: Edge weight between 2 nodes is asymmetric and depends on ordering
// of nodes. Swapping lr1, lr2 and invoking edgeWeightGRF() may return
// different result. So using the correct order of lr1, lr2 during edge
// weight computation and later during simplification is necessary for
// correctness.
//
unsigned GraphColor::edgeWeightGRF(const LiveRange *lr1, const LiveRange *lr2) {
  bool lr1EvenAlign = gra.isEvenAligned(lr1->getDcl());
  unsigned lr1_nreg = lr1->getNumRegNeeded();
  unsigned lr2_nreg = lr2->getNumRegNeeded();

  if (!lr1EvenAlign) {
    return lr1_nreg + lr2_nreg - 1;
  }

  bool lr2EvenAlign = gra.isEvenAligned(lr2->getDcl());
  if (!lr2EvenAlign) {
    unsigned sum = lr1_nreg + lr2_nreg;
    return sum + 1 - ((sum) % 2);
  } else if (lr2EvenAlign) {
    return lr1_nreg + lr2_nreg - 1 + (lr1_nreg % 2) + (lr2_nreg % 2);
  } else {
    vISA_ASSERT_UNREACHABLE("should be unreachable");
    return 0;
  }
}

unsigned GraphColor::edgeWeightARF(const LiveRange *lr1, const LiveRange *lr2) {
  if (lr1->getRegKind() == G4_FLAG) {
    G4_SubReg_Align lr1_align = gra.getSubRegAlign(lr1->getVar()->getDeclare());
    G4_SubReg_Align lr2_align = gra.getSubRegAlign(lr2->getVar()->getDeclare());
    unsigned lr1_nreg = lr1->getNumRegNeeded();
    unsigned lr2_nreg = lr2->getNumRegNeeded();

    if (lr1_align == Any) {
      return lr1_nreg + lr2_nreg - 1;
    } else if (lr1_align == Even_Word && lr2_align == Any) {
      return lr1_nreg + lr2_nreg + 1 - ((lr1_nreg + lr2_nreg) % 2);
    } else if (lr1_align == Even_Word && lr2_align == Even_Word) {
      if (lr1_nreg % 2 == 0 && lr2_nreg % 2 == 0) {
        return lr1_nreg + lr2_nreg - 2;
      } else {
        return lr1_nreg + lr2_nreg - 1 + (lr1_nreg % 2) + (lr2_nreg % 2);
      }
    } else {
      vISA_ASSERT_UNREACHABLE(
          "Found unsupported subRegAlignment in flag register allocation!");
      return 0;
    }
  } else if (lr1->getRegKind() == G4_ADDRESS) {
    G4_SubReg_Align lr1_align = gra.getSubRegAlign(lr1->getVar()->getDeclare());
    G4_SubReg_Align lr2_align = gra.getSubRegAlign(lr2->getVar()->getDeclare());
    unsigned lr1_nreg = lr1->getNumRegNeeded();
    unsigned lr2_nreg = lr2->getNumRegNeeded();

    if (lr1_align == Any) {
      return lr1_nreg + lr2_nreg - 1;
    } else if (lr1_align == Four_Word && lr2_align == Any) {
      return lr1_nreg + lr2_nreg + 3 - (lr1_nreg + lr2_nreg) % 4;
    } else if (lr1_align == Four_Word && lr2_align == Four_Word) {
      return lr1_nreg + lr2_nreg - 1 + (4 - lr1_nreg % 4) % 4 +
             (4 - lr2_nreg % 4) % 4;
    } else if (lr1_align == Eight_Word && lr2_align == Any) {
      return lr1_nreg + lr2_nreg + 7 - (lr1_nreg + lr2_nreg) % 8;
    } else if (lr1_align == Eight_Word && lr2_align == Four_Word) {
      if (((8 - lr1_nreg % 8) % 8) >= 4)
        return lr1_nreg + lr2_nreg - 1 + (8 - lr1_nreg % 8) % 8 - 4;
      return lr1_nreg + lr2_nreg - 1 + (8 - lr1_nreg % 8) % 8 +
             (4 - lr2_nreg % 4) % 4;
    } else if (lr1_align == Eight_Word && lr2_align == Eight_Word) {
      return lr1_nreg + lr2_nreg - 1 + (8 - lr1_nreg % 8) % 8 +
             (8 - lr2_nreg % 8) % 8;
    } else {
      vISA_ASSERT_UNREACHABLE(
          "Found unsupported subRegAlignment in address register allocation!");
      return 0;
    }
  }
  vISA_ASSERT_UNREACHABLE(
      "Found unsupported ARF reg type in register allocation!");
  return 0;
}

void GlobalRA::fixSrc0IndirFcall() {
  // Indirect calls look like:
  // mov (1|NM) V10    0x123456:ud
  // fcall (1) dst     V10 <-- V10 which is src0 contains %ip to jump to
  //
  // In this function, we want to set V10 to r125.0 which is same as dst of
  // fcall as per ABI. This way, when inserting save/restore code around fcall,
  // no special checks are needed to handle V10.
  //
  // But this works only if V10 is a local. If it not a local we create a mov
  // that copies V10 in to a new temp variable. And then we map this temp
  // variable to r125.0. Hopefully V10 being global would be a rare occurence.
  for (auto bb : kernel.fg) {
    if (bb->isEndWithFCall()) {
      auto fcall = bb->back()->asCFInst();
      if (!fcall->getSrc(0) || !fcall->getSrc(0)->isSrcRegRegion())
        continue;

      auto src0Rgn = fcall->getSrc(0)->asSrcRegRegion();
      auto src0Dcl = src0Rgn->getBase()->asRegVar()->getDeclare();
      auto src0TopDcl = src0Rgn->getTopDcl();

      if (src0Dcl != src0TopDcl || !isBlockLocal(src0TopDcl) ||
          src0TopDcl->getNumElems() > 1) {
        // create a copy
        auto tmpDcl = kernel.fg.builder->createHardwiredDeclare(
            1, src0Rgn->getType(), kernel.getFPSPGRF(),
            IR_Builder::SubRegs_Stackcall::Ret_IP);
        auto dst = kernel.fg.builder->createDst(tmpDcl->getRegVar(),
                                                src0Rgn->getType());
        auto src = kernel.fg.builder->duplicateOperand(src0Rgn);
        auto copy = kernel.fg.builder->createMov(g4::SIMD1, dst, src,
                                                 InstOpt_WriteEnable, false);
        auto iter = std::find_if(bb->begin(), bb->end(),
                                 [](G4_INST *inst) { return inst->isFCall(); });
        bb->insertBefore(iter, copy);
        auto newSrc = kernel.fg.builder->createSrc(
            tmpDcl->getRegVar(), 0, 0, kernel.fg.builder->getRegionScalar(),
            src0Rgn->getType());
        fcall->setSrc(newSrc, 0);
      } else {
        src0TopDcl->getRegVar()->setPhyReg(
            fcall->getDst()->getBase()->asRegVar()->getPhyReg(),
            fcall->getDst()->getBase()->asRegVar()->getPhyRegOff());
      }
    }
  }
}

bool dump(const char *s, const LiveRangeVec &lrs, unsigned size) {
  // Utility function to dump lr from name.
  // Returns true if lr name found.
  std::string name = s;
  for (unsigned i = 0; i != size; i++) {
    auto lr = lrs[i];
    if (lr && name.compare(lr->getVar()->getName()) == 0) {
      lr->dump();
      return true;
    }
  }
  return false;
}

bool dump(const char *s, const G4_Kernel *kernel) {
  // Utility function to dump dcl for given variable name.
  // Returns true if variable found.
  std::string name = s;
  for (auto dcl : kernel->Declares) {
    if (name.compare(dcl->getName()) == 0) {
      dcl->dump();
      return true;
    }
  }
  return false;
}

bool Interference::dumpIntf(const char *s) const {
  // Utility function to dump intf for given variable based on name.
  // Returns true if variable found.
  std::cout << "\n\n **** Interference Table ****\n";
  for (unsigned i = 0; i < maxId; i++) {
    std::string name = lrs[i]->getVar()->getName();
    if (name.compare(s) == 0) {
      std::cout << "(" << i << ") ";
      lrs[i]->dump();
      std::cout << "\n";
      for (unsigned j = 0; j < maxId; j++) {
        if (interfereBetween(i, j)) {
          std::cout << "\t";
          lrs[j]->getVar()->emit(std::cout);
        }
      }
      std::cout << "\n";
      return true;
    }
  }
  return false;
}

void LiveRange::setAllocHint(unsigned h) {
  if ((h + dcl->getNumRows()) <= gra.kernel.getNumRegTotal())
    allocHint = h;
}

// sortedIntervals comes from augmentation.
// This can be invoked either post RA where phy regs are assigned to dcls,
// or after assignColors with lrs and numLRs passed which makes this function
// use temp allocations from lrs. Doesnt handle sub-routines yet.
void RegChartDump::dumpRegChart(std::ostream &os, const LiveRangeVec& lrs,
                                unsigned numLRs) {
  constexpr unsigned N = 128;
  std::unordered_map<G4_INST *, std::bitset<N>> busyGRFPerInst;
  bool dumpHex = false;

  auto getPhyReg = [&](const G4_Declare *dcl) {
    auto preg = dcl->getRegVar()->getPhyReg();
    if (preg)
      return preg;

    for (unsigned i = 0; i != numLRs; i++) {
      const LiveRange *lr = lrs[i];
      if (lr->getDcl() == dcl) {
        preg = lr->getPhyReg();
        break;
      }
    }

    return preg;
  };

  for (auto dcl : sortedLiveIntervals) {
    if (dcl->getRegFile() != G4_RegFileKind::G4_GRF &&
        dcl->getRegFile() != G4_RegFileKind::G4_INPUT)
      continue;

    auto phyReg = getPhyReg(dcl);
    if (!phyReg)
      continue;

    if (!phyReg->isGreg())
      continue;

    auto GRFStart = phyReg->asGreg()->getRegNum();
    auto numRows = dcl->getNumRows();

    auto startInst = startEnd[dcl].first;
    auto endInst = startEnd[dcl].second;

    bool start = (dcl->getRegFile() == G4_RegFileKind::G4_INPUT);
    bool done = false;
    for (auto bb : gra.kernel.fg) {
      for (auto inst : *bb) {
        if (inst == startInst) {
          start = true;
          continue;
        }

        if (!start)
          continue;

        for (unsigned i = GRFStart; i != (GRFStart + numRows); i++) {
          busyGRFPerInst[inst].set(i, true);
        }

        if (inst == endInst || endInst == startInst) {
          done = true;
          break;
        }
      }

      if (done)
        break;
    }
  }

  // Now emit instructions with GRFs
  for (auto bb : gra.kernel.fg) {
    for (auto inst : *bb) {
      constexpr unsigned maxInstLen = 80;
      auto item = busyGRFPerInst[inst];
      std::stringstream ss;
      inst->emit(ss);
      auto len = ss.str().length();

      if (len <= maxInstLen) {
        os << ss.str();
        for (unsigned i = 0; i != maxInstLen - ss.str().length(); i++)
          os << " ";
      } else {
        auto tmpStr = ss.str();
        auto limitedStr = tmpStr.substr(0, maxInstLen);
        os << std::string(limitedStr);
      }

      os << "        ";

      if (!dumpHex) {
        // dump GRFs | - busy, * - free
        for (unsigned i = 0; i != N; i++) {
          // emit in groups of 10 GRFs
          if (i > 0 && (i % 10) == 0)
            os << "  ";

          if (item[i] == true)
            os << "|"; // busy
          else
            os << "*"; // free
        }
      } else {
        for (unsigned i = 0; i != N; i += sizeof(unsigned short) * 8) {
          unsigned short busyGRFs = 0;
          for (unsigned j = 0; j != sizeof(unsigned short) * 8; j++) {
            auto offset = i + j;
            if (offset < N) {
              if (item[offset])
                busyGRFs |= (1 << j);
            }
          }
          printf("r%d:%4x      ", i, busyGRFs);
        }
      }
      os << "\n";
    }
    os << "\n";
  }
}

void RegChartDump::recordLiveIntervals(const std::vector<G4_Declare *> &dcls) {
  sortedLiveIntervals = dcls;
  for (auto dcl : dcls) {
    auto start = gra.getStartInterval(dcl);
    auto end = gra.getEndInterval(dcl);
    startEnd.insert(std::make_pair(dcl, std::make_pair(start, end)));
  }
}

void DynPerfModel::run() {
  char LocalBuffer[1024];
  for (auto BB : Kernel.fg.getBBList()) {
    for (auto Inst : BB->getInstList()) {
      if (Inst->isLabel() || Inst->isPseudoKill())
        continue;

      if (Inst->isSpillIntrinsic())
        NumSpills++;
      if (Inst->isFillIntrinsic())
        NumFills++;

      auto InnerMostLoop = Kernel.fg.getLoops().getInnerMostLoop(BB);
      auto NestingLevel = InnerMostLoop ? InnerMostLoop->getNestingLevel() : 0;
      if (Inst->isFillIntrinsic()) {
        FillDynInst +=
            (unsigned int)std::pow<unsigned int>(10, NestingLevel) * 1;
      } else if (Inst->isSpillIntrinsic()) {
        SpillDynInst +=
            (unsigned int)std::pow<unsigned int>(10, NestingLevel) * 1;
      }
      TotalDynInst +=
          (unsigned int)std::pow<unsigned int>(10, NestingLevel) * 1;
    }
  }

  std::stack<Loop *> Loops;
  for (auto Loop : Kernel.fg.getLoops().getTopLoops()) {
    Loops.push(Loop);
  }
  while (Loops.size() > 0) {
    auto CurLoop = Loops.top();
    Loops.pop();
    unsigned int FillCount = 0;
    unsigned int SpillCount = 0;
    unsigned int TotalCount = 0;
    unsigned int LoopLevel = CurLoop->getNestingLevel();
    if (SpillFillPerNestingLevel.size() <= LoopLevel)
      SpillFillPerNestingLevel.resize(LoopLevel + 1);
    std::get<0>(SpillFillPerNestingLevel[LoopLevel]) += 1;
    for (auto BB : CurLoop->getBBs()) {
      for (auto Inst : BB->getInstList()) {
        if (Inst->isLabel() || Inst->isPseudoKill())
          continue;
        TotalCount++;
        if (Inst->isFillIntrinsic()) {
          FillCount++;
          std::get<2>(SpillFillPerNestingLevel[LoopLevel]) += 1;
        } else if (Inst->isSpillIntrinsic()) {
          SpillCount++;
          std::get<1>(SpillFillPerNestingLevel[LoopLevel]) += 1;
        }
      }
    }

    sprintf_s(LocalBuffer, sizeof(LocalBuffer),
              "Loop %d @ level %d: %d total, %d fill, %d spill, %d subroutine "
              "calls\n",
              CurLoop->id, LoopLevel, TotalCount, FillCount, SpillCount,
              CurLoop->subCalls);
    Buffer += std::string(LocalBuffer);

    for (auto Child : CurLoop->immNested)
      Loops.push(Child);
  };
}

void DynPerfModel::dump() {
  char LocalBuffer[1024];

  unsigned int InstCount = 0;
  for (auto BB : Kernel.fg.getBBList()) {
    for (auto Inst : BB->getInstList()) {
      if (!Inst->isLabel() && !Inst->isPseudoKill())
        InstCount++;
    }
  }
  auto AsmName = Kernel.getOptions()->getOptionCstr(VISA_AsmFileName);
  auto SpillSize = Kernel.fg.builder->getJitInfo()->stats.spillMemUsed;
  sprintf_s(LocalBuffer, sizeof(LocalBuffer),
            "Kernel name: %s\n # BBs : %d\n # Asm Insts: %d\n # RA Iters = "
            "%d\n Spill Size = %d\n # Spills: %d\n # Fills : %d\n",
            AsmName, (int)Kernel.fg.getBBList().size(), InstCount, NumRAIters,
            SpillSize, NumSpills, NumFills);
  Buffer += std::string(LocalBuffer);

  sprintf_s(LocalBuffer, sizeof(LocalBuffer),
            "Total dyn inst: %llu\nFill dyn inst: %llu\nSpill dyn inst: %llu\n",
            TotalDynInst, FillDynInst, SpillDynInst);
  Buffer += std::string(LocalBuffer);

  sprintf_s(LocalBuffer, sizeof(LocalBuffer),
            "Percent Fill/Total dyn inst: %f\n",
            (double)FillDynInst /
                ((double)TotalDynInst > 0 ? (double)TotalDynInst : 1) * 100.0f);
  Buffer += std::string(LocalBuffer);

  for (unsigned int I = 1; I != SpillFillPerNestingLevel.size() &&
                           SpillFillPerNestingLevel.size() > 0;
       ++I) {
    sprintf_s(LocalBuffer, sizeof(LocalBuffer), "LL%d(#%d): {S-%d, F-%d}, ", I,
              std::get<0>(SpillFillPerNestingLevel[I]),
              std::get<1>(SpillFillPerNestingLevel[I]),
              std::get<2>(SpillFillPerNestingLevel[I]));
    Buffer += std::string(LocalBuffer);
  }

  std::cerr << Buffer << "\n";
}
