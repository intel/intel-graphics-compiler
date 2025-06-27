/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LinearScanRA.h"
#include "DebugInfo.h"
#include "LocalRA.h"
#include "PointsToAnalysis.h"
#include "SCCAnalysis.h"
#include "SpillManagerGMRF.h"
#include "common.h"
#include <optional>

using namespace vISA;

extern void getForbiddenGRFs(std::vector<unsigned int> &regNum,
                             G4_Kernel &kernel, unsigned stackCallRegSize,
                             unsigned reserveSpillSize,
                             unsigned reservedRegNum);

LinearScanRA::LinearScanRA(BankConflictPass &b, GlobalRA &g,
                           LivenessAnalysis &liveAnalysis)
    : kernel(g.kernel), builder(g.builder), l(liveAnalysis), LSMem(4096), bc(b),
      gra(g) {
  stackCallArgLR = nullptr;
  stackCallRetLR = nullptr;
}

void LinearScanRA::allocForbiddenVector(LSLiveRange *lr) {
  unsigned size = kernel.getNumRegTotal();
  bool *forbidden = (bool *)LSMem.alloc(sizeof(bool) * size);
  memset(forbidden, false, size);
  lr->setForbidden(forbidden);

  if (!builder.canWriteR0())
    lr->addForbidden(0);
  if (builder.mustReserveR1())
    lr->addForbidden(1);
}

void globalLinearScan::allocRetRegsVector(LSLiveRange *lr) {
  auto size = builder.kernel.getNumRegTotal();
  if (lr->getRetGRFs() == nullptr) {
    bool *forbidden = (bool *)GLSMem->alloc(sizeof(bool) * size);
    memset(forbidden, false, size);
    lr->setRegGRFs(forbidden);
  } else {
    // If we are vector is preallocated, simply clear it
    lr->clearRetGRF(size);
  }
}

LSLiveRange *LinearScanRA::GetOrCreateLocalLiveRange(G4_Declare *topdcl) {
  LSLiveRange *lr = gra.getSafeLSLR(topdcl);

  // Check topdcl of operand and setup a new live range if required
  if (!lr) {
    lr = new (LSMem) LSLiveRange();
    gra.setLSLR(topdcl, lr);
    // Should we lazy-create forbidden vector instead? It seems they should be
    // rare and we don't want to waste space on every live range.
    allocForbiddenVector(lr);
  }
  return lr;
}

LSLiveRange *LinearScanRA::CreateLocalLiveRange(G4_Declare *topdcl) {
  LSLiveRange *lr = new (LSMem) LSLiveRange();
  gra.setLSLR(topdcl, lr);
  allocForbiddenVector(lr);
  return lr;
}

class isLifetimeOpCandidateForRemoval {
public:
  isLifetimeOpCandidateForRemoval(GlobalRA &g) : gra(g) {}

  GlobalRA &gra;

  bool operator()(G4_INST *inst) {
    if (inst->isPseudoKill() || inst->isLifeTimeEnd()) {
      G4_Declare *topdcl = nullptr;

      if (inst->isPseudoKill()) {
        topdcl = GetTopDclFromRegRegion(inst->getDst());
      } else {
        topdcl = GetTopDclFromRegRegion(inst->getSrc(0));
      }

      if (topdcl) {
        LSLiveRange *lr = gra.getSafeLSLR(topdcl);
        if (lr && lr->getNumRefs() == 0 && (topdcl->getRegFile() == G4_GRF ||
                                      topdcl->getRegFile() == G4_INPUT)) {
          // Remove this lifetime op
          return true;
        }
      }
    }

    return false;
  }
};

void LinearScanRA::removeUnrequiredLifetimeOps() {
  // Iterate over all instructions and inspect only
  // pseudo_kills/lifetime.end instructions. Remove
  // instructions that have no other useful instruction.

  for (BB_LIST_ITER bb_it = kernel.fg.begin(); bb_it != kernel.fg.end();
       bb_it++) {
    G4_BB *bb = (*bb_it);
    bb->erase(std::remove_if(bb->begin(), bb->end(),
                             isLifetimeOpCandidateForRemoval(this->gra)),
              bb->end());
  }
}

void LinearScanRA::setLexicalID() {
  unsigned int id = 1;
  for (auto bb : kernel.fg) {
    for (auto curInst : *bb) {
      if (curInst->isPseudoKill() || curInst->isLifeTimeEnd()) {
        curInst->setLexicalId(id);
      } else {
        curInst->setLexicalId(id++);
      }
    }
  }
  lastInstLexID = id;
}

bool LinearScanRA::hasDstSrcOverlapPotential(G4_DstRegRegion *dst,
                                             G4_SrcRegRegion *src) {
  int dstOpndNumRows = 0;

  if (dst->getBase()->isRegVar()) {
    G4_Declare *dstDcl = dst->getBase()->asRegVar()->getDeclare();
    if (dstDcl != nullptr) {
      int dstOffset = (dstDcl->getOffsetFromBase() + dst->getLeftBound()) /
                      builder.numEltPerGRF<Type_UB>();
      G4_DstRegRegion *dstRgn = dst;
      dstOpndNumRows = dstRgn->getSubRegOff() * dstRgn->getTypeSize() +
                           dstRgn->getLinearizedEnd() -
                           dstRgn->getLinearizedStart() + 1 >
                       builder.numEltPerGRF<Type_UB>();

      if (src != NULL && src->isSrcRegRegion() &&
          src->asSrcRegRegion()->getBase()->isRegVar()) {
        G4_SrcRegRegion *srcRgn = src->asSrcRegRegion();
        G4_Declare *srcDcl = src->getBase()->asRegVar()->getDeclare();
        int srcOffset = (srcDcl->getOffsetFromBase() + src->getLeftBound()) /
                        builder.numEltPerGRF<Type_UB>();
        bool srcOpndNumRows = srcRgn->getSubRegOff() * dstRgn->getTypeSize() +
                                  srcRgn->getLinearizedEnd() -
                                  srcRgn->getLinearizedStart() + 1 >
                              builder.numEltPerGRF<Type_UB>();

        if (dstOpndNumRows || srcOpndNumRows) {
          if (!(gra.isEvenAligned(dstDcl) && gra.isEvenAligned(srcDcl) &&
                srcOffset % 2 == dstOffset % 2 && dstOpndNumRows &&
                srcOpndNumRows)) {
            return true;
          }
        }
      }
    }
  }

  return false;
}

void LinearScanRA::getRowInfo(int size, int &nrows, int &lastRowSize,
                              const IR_Builder &builder) {
  if (size <= (int)builder.numEltPerGRF<Type_UW>()) {
    nrows = 1;
  } else {
    // nrows is total number of rows, including last row even if it is partial
    nrows = size / builder.numEltPerGRF<Type_UW>();
    // lastrowsize is number of words actually used in last row
    lastRowSize = size % builder.numEltPerGRF<Type_UW>();

    if (size % builder.numEltPerGRF<Type_UW>() != 0) {
      nrows++;
    }

    if (lastRowSize == 0) {
      lastRowSize = builder.numEltPerGRF<Type_UW>();
    }
  }

  return;
}

unsigned int LinearScanRA::convertSubRegOffFromWords(G4_Declare *dcl,
                                                     int subregnuminwords) {
  // Return subreg offset in units of dcl's element size.
  // Input is subregnum in word units.
  unsigned int subregnum;

  subregnum = (subregnuminwords * 2) / dcl->getElemSize();

  return subregnum;
}

void LSLiveRange::recordRef(G4_BB *bb, bool fromEntry) {
  if (numRefsInFG < 2) {
    if (fromEntry) {
      numRefsInFG += 2;
    } else if (bb != prevBBRef) {
      numRefsInFG++;
      prevBBRef = bb;
    }
  }
  if (!fromEntry) {
    numRefs++;
  }
}

bool LSLiveRange::isGRFRegAssigned() {
  vISA_ASSERT(topdcl != NULL, "Top dcl not set");
  G4_RegVar *rvar = topdcl->getRegVar();
  bool isPhyRegAssigned = false;

  if (rvar) {
    if (rvar->isPhyRegAssigned())
      isPhyRegAssigned = true;
  }

  return isPhyRegAssigned;
}

unsigned int LSLiveRange::getSizeInWords(const IR_Builder &builder) {
  int nrows = getTopDcl()->getNumRows();
  int elemsize = getTopDcl()->getElemSize();
  int nelems = getTopDcl()->getNumElems();  //elements per row
  int words = 0;

  if (nrows > 1) {
    // If sizeInWords is set, use it otherwise consider entire row reserved
    unsigned int sizeInWords = getTopDcl()->getWordSize();

    if (sizeInWords > 0)
      words = sizeInWords;
    else
      words = nrows * builder.numEltPerGRF<Type_UW>();
  } else if (nrows == 1) {
    int nbytesperword = 2;
    words = (nelems * elemsize + 1) / nbytesperword;
  }

  return words;
}

// Mark physical register allocated to range lr as not busy
void globalLinearScan::freeAllocedRegs(LSLiveRange *lr, bool setInstID) {
  int sregnum;

  G4_VarBase *preg = lr->getPhyReg(sregnum);

  vISA_ASSERT(
      preg != NULL,
      "Physical register not assigned to live range. Cannot free regs.");

  unsigned int idx = 0;
  if (setInstID) {
    lr->getLastRef(idx);
  }

  if (!lr->isUseUnAvailableReg()) {
    pregManager.freeRegs(preg->asGreg()->getRegNum(), sregnum,
                         lr->getSizeInWords(builder), idx);
  }
}

void globalLinearScan::printActives() {
  std::cout << "====================ACTIVATE START==================="
            << "\n";
  for (auto lr : active) {
    unsigned int start, end;

    lr->getFirstRef(start);
    lr->getLastRef(end);

    int startregnum, endregnum, startsregnum = 0, endsregnum;
    G4_VarBase *op;
    op = lr->getPhyReg(startsregnum);

    startregnum = endregnum = op->asGreg()->getRegNum();
    endsregnum =
        startsregnum +
                 (lr->getTopDcl()->getTotalElems() *
                  lr->getTopDcl()->getElemSize() / 2) -
        1;

    if (lr->getTopDcl()->getNumRows() > 1) {
      endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;

      if (lr->getTopDcl()->getWordSize() > 0) {
        endsregnum =
            lr->getTopDcl()->getWordSize() % builder.numEltPerGRF<Type_UW>() -
            1;
        if (endsregnum < 0)
          endsregnum = 15;
      } else
        endsregnum = 15; // last word in GRF
    }
    if (lr->hasIndirectAccess()) {
      std::cout << "INDIR: ";
    } else {
      std::cout << "DIR  : ";
    }
    if (lr->getPreAssigned()) {
      std::cout << "\tPRE: ";
    } else {
      std::cout << "\tNOT: ";
    }

    std::cout << lr->getTopDcl()->getName() << "(" << start << ", " << end
              << ", " << lr->getTopDcl()->getByteSize() << ")";
    std::cout << " (r" << startregnum << "." << startsregnum << ":w - "
              << "r" << endregnum << "." << endsregnum << ":w)";
    std::cout << "\n";
  }
  for (int i = 0; i < (int)(numRegLRA); i++) {
    std::cout << "\nR" << i << ":";

    if (activeGRF[i].activeInput.size()) {
      for (auto lr : activeGRF[i].activeLV) {
        int startregnum, endregnum, startsregnum = 0, endsregnum;
        G4_VarBase *op;
        op = lr->getPhyReg(startsregnum);

        startregnum = endregnum = op->asGreg()->getRegNum();
        endsregnum = startsregnum +
                     (lr->getTopDcl()->getTotalElems() *
                      lr->getTopDcl()->getElemSize() / 2) -
                     1;

        if (lr->getTopDcl()->getNumRows() > 1) {
          endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;

          if (lr->getTopDcl()->getWordSize() > 0) {
            endsregnum = lr->getTopDcl()->getWordSize() %
                             builder.numEltPerGRF<Type_UW>() -
                         1;
            if (endsregnum < 0)
              endsregnum = 15;
          } else
            endsregnum = 15; // last word in GRF
        }

        std::cout << "\tIN: " << lr->getTopDcl()->getName();
        std::cout << "(r" << startregnum << "." << startsregnum << ":w - "
                  << "r" << endregnum << "." << endsregnum << ":w)";
      }
    }

    if (activeGRF[i].activeLV.size()) {
      // There may be multiple variables take same register with different
      // offsets
      for (auto lr : activeGRF[i].activeLV) {
        int startsregnum = 0;
        G4_VarBase *op = lr->getPhyReg(startsregnum);

        int startregnum = op->asGreg()->getRegNum();
        int endregnum = startregnum;
        int endsregnum = startsregnum +
                         (lr->getTopDcl()->getTotalElems() *
                          lr->getTopDcl()->getElemSize() / 2) -
                         1;

        if (lr->getTopDcl()->getNumRows() > 1) {
          endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;

          if (lr->getTopDcl()->getWordSize() > 0) {
            endsregnum = lr->getTopDcl()->getWordSize() %
                             builder.numEltPerGRF<Type_UW>() -
                         1;
            if (endsregnum < 0)
              endsregnum = 15;
          } else
            endsregnum = 15; // last word in GRF
        }

        std::cout << "\t" << lr->getTopDcl()->getName();
        std::cout << "(r" << startregnum << "." << startsregnum << ":w - "
                  << "r" << endregnum << "." << endsregnum << ":w)";
      }
    }
  }
  std::cout << "====================ACTIVATE END==================="
            << "\n";
}

void globalLinearScan::expireAllActive() {
  if (active.size() > 0) {
    // Expire any remaining ranges
    LSLiveRange *lastActive = active.back();
    unsigned int endIdx;

    lastActive->getLastRef(endIdx);

    expireGlobalRanges(endIdx);
  }
}

void LinearScanRA::linearScanMarkReferencesInOpnd(G4_Operand *opnd, bool isEOT,
                                                  bool isCall) {
  G4_Declare *topdcl = NULL;

  if ((opnd->isSrcRegRegion() || opnd->isDstRegRegion())) {
    topdcl = GetTopDclFromRegRegion(opnd);

    if (topdcl &&
        (topdcl->getRegFile() == G4_GRF || topdcl->getRegFile() == G4_INPUT)) {
      // Handle GRF here
      vISA_ASSERT(topdcl->getAliasDeclare() == NULL, "Not topdcl");
      LSLiveRange *lr = GetOrCreateLocalLiveRange(topdcl);

      lr->recordRef(curBB_, false);
      if (isEOT && kernel.fg.builder->hasEOTGRFBinding()) {
        lr->markEOT();
      }
      if (topdcl->getRegVar() && topdcl->getRegVar()->isPhyRegAssigned() &&
          topdcl->getRegVar()->getPhyReg()->isGreg()) {
        lr->setPreAssigned(true);
      }
      if (isCall) {
        lr->setIsCall(true);
      }
      if (topdcl->getRegFile() == G4_INPUT) {
        BBVector[curBB_->getId()].setRefInput(true);
      }
    }
  } else if (opnd->isAddrExp()) {
    G4_AddrExp *addrExp = opnd->asAddrExp();

    topdcl = addrExp->getRegVar()->getDeclare();
    while (topdcl->getAliasDeclare() != NULL)
      topdcl = topdcl->getAliasDeclare();

    vISA_ASSERT(topdcl != NULL, "Top dcl was null for addr exp opnd");

    LSLiveRange *lr = GetOrCreateLocalLiveRange(topdcl);

    lr->recordRef(curBB_, false);
    lr->markIndirectRef(true);
    if (topdcl->getRegVar() && topdcl->getRegVar()->isPhyRegAssigned() &&
        topdcl->getRegVar()->getPhyReg()->isGreg()) {
      lr->setPreAssigned(true);
    }
    if (topdcl->getRegFile() == G4_INPUT) {
      BBVector[curBB_->getId()].setRefInput(true);
    }
  }
}

void LinearScanRA::linearScanMarkReferencesInInst(INST_LIST_ITER inst_it) {
  auto inst = (*inst_it);

  // Scan dst
  G4_Operand *dst = inst->getDst();
  if (dst) {
    linearScanMarkReferencesInOpnd(dst, false, inst->isCall());
  }

  // Scan srcs
  for (int i = 0, nSrcs = inst->getNumSrc(); i < nSrcs; i++) {
    G4_Operand *src = inst->getSrc(i);

    if (src) {
      linearScanMarkReferencesInOpnd(src, inst->isEOT(), inst->isCall());
    }
  }
}

void LinearScanRA::linearScanMarkReferences(unsigned int &numRowsEOT) {
  // Iterate over all BBs
  for (auto curBB : kernel.fg) {
    curBB_ = curBB;
    // Iterate over all insts
    for (INST_LIST_ITER inst_it = curBB->begin(), inst_end = curBB->end();
         inst_it != inst_end; ++inst_it) {
      G4_INST *curInst = (*inst_it);

      if (curInst->isPseudoKill() || curInst->isLifeTimeEnd()) {
        if (curInst->isLifeTimeEnd()) {
          linearScanMarkReferencesInInst(inst_it);
        }
        continue;
      }

      if (curInst->isEOT() && kernel.fg.builder->hasEOTGRFBinding()) {
        numRowsEOT += curInst->getSrc(0)->getTopDcl()->getNumRows();

        if (curInst->isSplitSend() && !curInst->getSrc(1)->isNullReg()) {
          // both src0 and src1 have to be >=r112
          numRowsEOT += curInst->getSrc(1)->getTopDcl()->getNumRows();
        }
      }

      linearScanMarkReferencesInInst(inst_it);
    }

    if (BBVector[curBB->getId()].hasBackEdgeIn() || curBB->getId() == 0) {
      for (unsigned i = 0; i < kernel.Declares.size(); i++) {
        G4_Declare *dcl = kernel.Declares[i];
        if (dcl->getAliasDeclare() != NULL) {
          continue;
        }

        if (dcl->getRegFile() != G4_GRF && dcl->getRegFile() != G4_INPUT) {
          continue;
        }

        LSLiveRange *lr = gra.getSafeLSLR(dcl);
        if (lr == nullptr) {
          continue;
        }

        if (!l.isEmptyLiveness() &&
            l.isLiveAtEntry(curBB, dcl->getRegVar()->getId())) {
          lr->recordRef(curBB, true);
        }
      }
    }
  }

  getGlobalDeclares();
}

bool LSLiveRange::isLiveRangeGlobal() const {
  if (numRefsInFG > 1 || topdcl->isOutput() == true ||
      (topdcl->getRegVar() && topdcl->getRegVar()->isPhyRegAssigned() &&
       topdcl->getRegVar()->getPhyReg()->isGreg())) {
    return true;
  }

  return false;
}

void LinearScanRA::getGlobalDeclares() {
  for (G4_Declare *dcl : kernel.Declares) {
    const LSLiveRange *lr = gra.getSafeLSLR(dcl);

    if (lr && lr->isLiveRangeGlobal()) {
      globalDeclares.push_back(dcl);
    }
  }

  return;
}

void LinearScanRA::markBackEdges() {
  unsigned numBBId = (unsigned)kernel.fg.size();
  BBVector.resize(numBBId);

  for (auto curBB : kernel.fg) {
    for (auto succBB : curBB->Succs) {
      if (curBB->getId() >= succBB->getId()) {
        BBVector[succBB->getId()].setBackEdgeIn(true);
        BBVector[curBB->getId()].setBackEdgeOut(true);
      }
    }
  }

  if (!kernel.fg.isReducible()) {
    // Find exit BB of SCC and map it to the head BB of SCC
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
      for (auto exitBB : SCCSucc) // map the exitBB to the head of the SCC
      {
        loopHeadExitMap[headBB].push_back(exitBB);
      }
    }
  } else {
    // Find exit BB of loop and map it to the head BB of loop
    for (auto &&iter : kernel.fg.getAllNaturalLoops()) {
      auto &&backEdge = iter.first;
      G4_BB *headBB = backEdge.second;
      const std::set<G4_BB *> &loopBody = iter.second;

      for (auto block : loopBody) {
        for (auto succBB : block->Succs) {
          if (loopBody.find(succBB) == loopBody.end()) {
            G4_BB *exitBB = succBB;

            unsigned latchBBId = (backEdge.first)->getId();
            unsigned exitBBId = succBB->getId();
            if (exitBBId < latchBBId && succBB->Succs.size() == 1) {
              exitBB = succBB->Succs.front();
            }

            loopHeadExitMap[headBB].push_back(
                exitBB); // map the exitBB to the head of the loop
          }
        }
      }
    }
  }
}

void LinearScanRA::createLiveIntervals() {
  for (auto dcl : gra.kernel.Declares) {
    // Mark those physical registers busy that are declared with Output
    // attribute The live interval will gurantee they are not reused.
    if (dcl->isOutput() && dcl->isInput()) {
      pregs->markPhyRegs(dcl);
    }

    if (dcl->getAliasDeclare() != NULL) {
      continue;
    }
    LSLiveRange *lr = new (LSMem) LSLiveRange();
    gra.setLSLR(dcl, lr);
    allocForbiddenVector(lr);
  }
}

void LinearScanRA::preRAAnalysis() {
  int numGRF = kernel.getNumRegTotal();

  // Clear LSLiveRange* computed preRA
  gra.clearLocalLiveRanges();

  createLiveIntervals();

  markBackEdges();
  // Mark references made to decls
  linearScanMarkReferences(numRowsEOT);

  // Check whether pseudo_kill/lifetime.end are only references
  // for their respective variables. Remove them if so. Doing this
  // helps reduce number of variables in symbol table increasing
  // changes of skipping global RA.
  removeUnrequiredLifetimeOps();

  numRegLRA = numGRF;

  unsigned int reservedGRFNum =
      builder.getOptions()->getuInt32Option(vISA_ReservedGRFNum);
  bool hasStackCall =
      kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc();
  if (hasStackCall || reservedGRFNum || builder.getOption(vISA_Debug)) {
    std::vector<unsigned int> forbiddenRegs;
    unsigned int stackCallRegSize =
        hasStackCall ? kernel.stackCall.numReservedABIGRF() : 0;
    getForbiddenGRFs(forbiddenRegs, kernel, stackCallRegSize, 0,
                     reservedGRFNum);
    for (unsigned int i = 0, size = forbiddenRegs.size(); i < size; i++) {
      unsigned int regNum = forbiddenRegs[i];
      pregs->setGRFUnavailable(
          regNum); // un-available will always be there, if it's conflict with
                   // input or pre-assigned, it's still un-available.
    }
  } else {
    pregs->setSimpleGRFAvailable(true);
    if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) != VISA_3D ||
        !builder.canWriteR0()) {
      pregs->setR0Forbidden();
    }

    if (builder.mustReserveR1()) {
      pregs->setR1Forbidden();
    }
  }
  return;
}

void LinearScanRA::getCalleeSaveRegisters() {
  unsigned int callerSaveNumGRF = kernel.stackCall.getCallerSaveLastGRF() + 1;
  unsigned int numCalleeSaveRegs = kernel.stackCall.getNumCalleeSaveRegs();

  gra.calleeSaveRegs.resize(numCalleeSaveRegs, false);
  gra.calleeSaveRegCount = 0;

  G4_Declare *dcl = builder.kernel.fg.pseudoVCEDcl;
  LSLiveRange *lr = gra.getLSLR(dcl);
  const bool *forbidden = lr->getForbidden();
  unsigned int startCalleeSave = kernel.stackCall.getCallerSaveLastGRF() + 1;
  unsigned int endCalleeSave =
      startCalleeSave + kernel.stackCall.getNumCalleeSaveRegs() - 1;
  for (unsigned i = 0; i < kernel.getNumRegTotal(); i++) {
    if (forbidden[i]) {
      if (i >= startCalleeSave && i < endCalleeSave) {
        gra.calleeSaveRegs[i - callerSaveNumGRF] = true;
        gra.calleeSaveRegCount++;
      }
    }
  }
}

void LinearScanRA::getCallerSaveRegisters() {
  unsigned int callerSaveNumGRF = kernel.stackCall.getCallerSaveLastGRF() + 1;

  for (BB_LIST_ITER it = builder.kernel.fg.begin();
       it != builder.kernel.fg.end(); ++it) {
    if ((*it)->isEndWithFCall()) {
      gra.callerSaveRegsMap[(*it)].resize(callerSaveNumGRF, false);
      gra.retRegsMap[(*it)].resize(callerSaveNumGRF, false);
      unsigned callerSaveRegCount = 0;
      G4_INST *callInst = (*it)->back();
      vISA_ASSERT((*it)->Succs.size() == 1,
                  "fcall basic block cannot have more than 1 successor");
      G4_Declare *dcl =
          builder.kernel.fg.fcallToPseudoDclMap[callInst->asCFInst()]
              .VCA->getRegVar()
              ->getDeclare();
      LSLiveRange *lr = gra.getLSLR(dcl);

      const bool *forbidden = lr->getForbidden();
      unsigned int startCalleeSave = 1;
      unsigned int endCalleeSave =
          startCalleeSave + kernel.stackCall.getCallerSaveLastGRF();
      for (unsigned i = 0; i < builder.kernel.getNumRegTotal(); i++) {
        if (forbidden[i]) {
          if (i >= startCalleeSave && i < endCalleeSave) {
            gra.callerSaveRegsMap[(*it)][i] = true;
            callerSaveRegCount++;
          }
        }
      }

      // ret
      const bool *rRegs = lr->getRetGRFs();
      if (rRegs != nullptr) {
        for (unsigned i = 0; i < builder.kernel.getNumRegTotal(); i++) {
          if (rRegs[i]) {
            if (i >= startCalleeSave && i < endCalleeSave) {
              gra.retRegsMap[(*it)][i] = true;
            }
          }
        }
      }

      gra.callerSaveRegCountMap[(*it)] = callerSaveRegCount;
    }
  }
}

void LinearScanRA::getSaveRestoreRegister() {
  if (!builder.getIsKernel()) {
    getCalleeSaveRegisters();
  }
  getCallerSaveRegisters();
}

/*
 * Calculate the last lexcial ID of executed instruction if the function is
 * called
 */
void LinearScanRA::calculateFuncLastID() {
  funcLastLexID.resize(kernel.fg.sortedFuncTable.size());
  for (unsigned i = 0; i < kernel.fg.sortedFuncTable.size(); i++) {
    funcLastLexID[i] = 0;
  }

  for (auto func : kernel.fg.sortedFuncTable) {
    unsigned fid = func->getId();

    if (fid == UINT_MAX) {
      // entry kernel
      continue;
    }

    funcLastLexID[fid] = func->getExitBB()->back()->getLexicalId() * 2;
    for (auto &&callee : func->getCallees()) {
      if (funcLastLexID[fid] < funcLastLexID[callee->getId()]) {
        funcLastLexID[fid] = funcLastLexID[callee->getId()];
      }
    }
  }
}

int LinearScanRA::linearScanRA() {
  std::list<LSLiveRange *> spillLRs;
  int iterator = 0;
  uint32_t GRFSpillFillCount = 0;
  bool hasStackCall =
      kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc();
  int globalScratchOffset =
      kernel.getInt32KernelAttr(Attributes::ATTR_SpillMemOffset);
  // FXIME: This does not match GCRA's definition (it has an additional
  // builder.suppportsLSC())
  bool useScratchMsgForSpill =
      !hasStackCall && (globalScratchOffset < (int)(SCRATCH_MSG_LIMIT * 0.6));
  bool enableSpillSpaceCompression =
      builder.getOption(vISA_SpillSpaceCompression);
  do {
    spillLRs.clear();
    funcCnt = 0;
    std::vector<LSLiveRange *> eotLiveIntervals;
    inputIntervals.clear();
    setLexicalID();
    calculateFuncLastID();

#ifdef DEBUG_VERBOSE_ON
    COUT_ERROR << "=============  ITERATION: " << iterator << "============"
               << "\n";
#endif

    // Input
    PhyRegsLocalRA initPregs = (*pregs);
    calculateInputIntervalsGlobal(initPregs);
#ifdef DEBUG_VERBOSE_ON
    COUT_ERROR << "===== printInputLiveIntervalsGlobal============"
               << kernel.getName() << "\n";
    printInputLiveIntervalsGlobal();
#endif

    liveThroughIntervals.clear();
    globalLiveIntervals.clear();
    preAssignedLiveIntervals.clear();
    eotLiveIntervals.clear();
    unsigned latestLexID = 0;

    // Some live through intervals are implicit
    calculateLiveThroughIntervals();
    for (auto bb : kernel.fg) {
      calculateLiveIntervalsGlobal(bb, globalLiveIntervals, eotLiveIntervals);
      latestLexID = bb->back()->getLexicalId() * 2;
    }

    if (liveThroughIntervals.size()) {
      for (auto lr : liveThroughIntervals) {
        globalLiveIntervals.insert(globalLiveIntervals.begin(), lr);
      }
    }

#ifdef DEBUG_VERBOSE_ON
    COUT_ERROR << "===== globalLiveIntervals============"
               << "\n";
    printLiveIntervals(globalLiveIntervals);
#endif

    if (eotLiveIntervals.size()) {
      assignEOTLiveRanges(builder, eotLiveIntervals);
      for (auto lr : eotLiveIntervals) {
        preAssignedLiveIntervals.push_back(lr);
      }
    }
#ifdef DEBUG_VERBOSE_ON
    COUT_ERROR << "===== preAssignedLiveIntervals============"
               << "\n";
    printLiveIntervals(preAssignedLiveIntervals);
#endif
    // Copy over alignment for vars inserted by RA
    gra.copyMissingAlignment();

    PhyRegsManager pregManager(builder, initPregs, doBCR);
    globalLinearScan ra(gra, &l, globalLiveIntervals, &preAssignedLiveIntervals,
                        inputIntervals, pregManager, numRegLRA, numRowsEOT,
                        latestLexID, doBCR, highInternalConflict, &LSMem);

    // Run linear scan RA
    bool success = ra.runLinearScan(builder, globalLiveIntervals, spillLRs);

    auto underSpillThreshold = [this](int numSpill, int asmCount) {
      int threshold = std::min(
          builder.getOptions()->getuInt32Option(vISA_AbortOnSpillThreshold),
          200u);
      return (numSpill * 200) < (threshold * asmCount);
    };

    for (auto lr : spillLRs) {
      GRFSpillFillCount += lr->getNumRefs();
    }

    int instNum = 0;
    for (auto bb : kernel.fg) {
      instNum += (int)bb->size();
    }
    if (GRFSpillFillCount && builder.getOption(vISA_AbortOnSpill) &&
        !underSpillThreshold(GRFSpillFillCount, instNum)) {
      // update jit metadata information
      if (auto jitInfo = builder.getJitInfo()) {
        jitInfo->stats.spillMemUsed = 0;
        jitInfo->stats.numAsmCountUnweighted = instNum;
        jitInfo->stats.numGRFSpillFillWeighted = GRFSpillFillCount;
      }

      // Early exit when -abortonspill is passed, instead of
      // spending time inserting spill code and then aborting.
      return VISA_SPILL;
    }

    // Try other graphcoloring
    if (!success) {
      undoLinearScanRAAssignments();
      return VISA_FAILURE;
    }

    if (spillLRs.size()) {
      if (iterator == 0 && enableSpillSpaceCompression &&
          kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D &&
          !hasStackCall) {
        enableSpillSpaceCompression = gra.spillSpaceCompression(
            gra.computeSpillSize(spillLRs), gra.nextSpillOffset);
      }

      SpillManagerGRF spillGRF(gra, gra.nextSpillOffset, &l, &spillLRs,
                               enableSpillSpaceCompression,
                               useScratchMsgForSpill);

      spillGRF.spillLiveRanges(&kernel);
      gra.nextSpillOffset = spillGRF.getNextOffset();
      gra.scratchOffset = std::max(gra.scratchOffset, spillGRF.getNextScratchOffset());

#ifdef DEBUG_VERBOSE_ON
      COUT_ERROR << "===== printSpillLiveIntervals============"
                 << "\n";
      printSpillLiveIntervals(spillLRs);
#endif

      if (builder.hasScratchSurface() && !hasStackCall &&
          (gra.nextSpillOffset + globalScratchOffset) > SCRATCH_MSG_LIMIT) {
        // create temp variable to store old a0.2 - this is marked as live-in
        // and live-out. because the variable is emitted only post RA to
        // preserve old value of a0.2.
        G4_Declare *a0Dcl = kernel.fg.builder->getOldA0Dot2Temp();
        LSLiveRange *lr = gra.getSafeLSLR(a0Dcl);
        if (!lr) {
          lr = CreateLocalLiveRange(a0Dcl);
          globalDeclares.push_back(a0Dcl);
        }
      } else if (gra.useLscForNonStackCallSpillFill ||
                 gra.useLscForScatterSpill) {
        // Xe2+ LSC-based spill/fill needs the same as above
        G4_Declare *a0Dcl = kernel.fg.builder->getOldA0Dot2Temp();
        LSLiveRange *lr = gra.getSafeLSLR(a0Dcl);
        if (!lr) {
          lr = CreateLocalLiveRange(a0Dcl);
          globalDeclares.push_back(a0Dcl);
        }
      }

      // update jit metadata information for spill
      if (auto jitInfo = builder.getJitInfo()) {

        jitInfo->hasStackcalls = kernel.fg.getHasStackCalls();

        if (builder.kernel.fg.frameSizeInOWord != 0) {
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
          jitInfo->stats.spillMemUsed = builder.kernel.fg.frameSizeInOWord * 16;

          // reserve spillMemUsed #bytes before 8kb boundary
          kernel.getGTPinData()->setScratchNextFree(
              8 * 1024 - kernel.getGTPinData()->getNumBytesScratchUse());
        } else {
          jitInfo->stats.spillMemUsed = gra.nextSpillOffset;
          kernel.getGTPinData()->setScratchNextFree(gra.nextSpillOffset);
        }
        jitInfo->stats.numGRFSpillFillWeighted = GRFSpillFillCount;
      }
      undoLinearScanRAAssignments();
    }

    RA_TRACE({
      std::cout << "\titeration: " << iterator << "\n";
      std::cout << "\t\tnextSpillOffset: " << gra.nextSpillOffset << "\n";
      std::cout << "\t\tGRFSpillFillCount: " << GRFSpillFillCount << "\n";
    });

    if (spillLRs.size()) {
      kernel.dumpToFile("after.Spill_GRF." + std::to_string(iterator));
    }

    iterator++;
  } while (spillLRs.size() && iterator < MAXIMAL_ITERATIONS);

  if (spillLRs.size()) {
    std::stringstream spilledVars;
    for (auto dcl : kernel.Declares) {
      if (dcl->isSpilled() && dcl->getRegFile() == G4_GRF) {
        spilledVars << dcl->getName() << "\t";
      }
    }
    spillLRs.clear();
    return VISA_FAILURE;
  }

  if (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc()) {
    getSaveRestoreRegister();
    unsigned localSpillAreaOwordSize = ROUND(gra.scratchOffset, 64) / 16;
    gra.addSaveRestoreCode(localSpillAreaOwordSize);
  }
  return VISA_SUCCESS;
}

int LinearScanRA::doLinearScanRA() {
  if (l.getNumSelectedVar() == 0) {
    return VISA_SUCCESS;
  }
  RA_TRACE(std::cout << "--Global linear Scan RA--\n");
  // Initial pregs which will be used in the preRAAnalysis
  PhyRegsLocalRA phyRegs(builder, kernel.getNumRegTotal());
  pregs = &phyRegs;
  preRAAnalysis();

  int success = linearScanRA();

  if (success == VISA_SUCCESS) {
    kernel.setRAType(RA_Type::GLOBAL_LINEAR_SCAN_RA);
  }

  return success;
}

void LinearScanRA::undoLinearScanRAAssignments() {
  // Undo all assignments made by local RA
  for (auto dcl : kernel.Declares) {
    LSLiveRange *lr = gra.getSafeLSLR(dcl);
    if (lr != NULL) {
      if (lr->getAssigned() == true) {
        // Undo the assignment
        lr->setAssigned(false);
        if (lr->getTopDcl()->getRegFile() != G4_INPUT &&
            !lr->getPreAssigned()) {
          lr->getTopDcl()->getRegVar()->resetPhyReg();
        }
        lr->resetPhyReg();
      }
      lr->setActiveLR(false);
      lr->setFirstRef(NULL, 0);
      lr->setLastRef(NULL, 0);
      lr->clearForbiddenGRF(kernel.getNumRegTotal());
      lr->setPushed(false);
      if (kernel.getOption(vISA_GenerateDebugInfo)) {
        auto lrInfo = kernel.getKernelDebugInfo()->getLiveIntervalInfo(
            lr->getTopDcl(), false);
        if (lrInfo)
          lrInfo->clearLiveIntervals();
      }
    }
  }
}

void LinearScanRA::setPreAssignedLR(
    LSLiveRange *lr, std::vector<LSLiveRange *> &preAssignedLiveIntervals) {
  int subreg = 0;
  G4_VarBase *reg = lr->getPhyReg(subreg);
  unsigned regnum =
      lr->getTopDcl()->getRegVar()->getPhyReg()->asGreg()->getRegNum();
  if (reg == nullptr) {
    unsigned int subReg = lr->getTopDcl()->getRegVar()->getPhyRegOff();
    unsigned int subRegInWord =
        subReg * lr->getTopDcl()->getRegVar()->getDeclare()->getElemSize() / 2;

    lr->setPhyReg(builder.phyregpool.getGreg(regnum), subRegInWord);
  }
  lr->setAssigned(true);

  // Pre assigned registers may overlap the unavailable registers
  lr->setUseUnAvailableReg(
      isUseUnAvailableRegister(regnum, lr->getTopDcl()->getNumRows()));

  // Insert into preAssgined live intervals
  // If the pre-assigned register is marked as unavailable, not join the live
  // range
  // FIXME: What about partial overlap?
  if (std::find(preAssignedLiveIntervals.begin(),
                preAssignedLiveIntervals.end(),
                lr) == preAssignedLiveIntervals.end()) {
    preAssignedLiveIntervals.push_back(lr);
  }

  return;
}

void LinearScanRA::setDstReferences(
    G4_BB *bb, INST_LIST_ITER inst_it, G4_Declare *dcl,
    std::vector<LSLiveRange *> &liveIntervals,
    std::vector<LSLiveRange *> &eotLiveIntervals) {
  G4_INST *curInst = (*inst_it);
  LSLiveRange *lr = gra.getSafeLSLR(dcl);

  if (!lr &&
      dcl->getRegFile() == G4_GRF) // The new variables generated by spill/fill,
                                   // mark reference should handle it
  {
    lr = CreateLocalLiveRange(dcl);
  }

  if (!lr)
    return;

  if (dcl->isOutput() && !lr->isGRFRegAssigned()) {
    lr->setLastRef(curInst, lastInstLexID * 2 + 1);
    if (!lr->isPushedToIntervalList()) {
      liveThroughIntervals.push_back(lr);
      lr->setPushed(true);
    }
    return;
  }

  if ((dcl->getRegFile() == G4_INPUT &&
       dcl != kernel.fg.builder->getStackCallArg() &&
       dcl != kernel.fg.builder->getStackCallRet()) ||
      (lr->isGRFRegAssigned() && (!dcl->getRegVar()->isGreg()))) // ARF
  {
    return;
  }

  if (dcl == kernel.fg.builder->getStackCallArg()) {
    if (stackCallArgLR == nullptr) {
      lr = new (LSMem) LSLiveRange();
      stackCallArgLR = lr;
      lr->setTopDcl(dcl);
      allocForbiddenVector(lr);
    } else {
      lr = stackCallArgLR;
    }
  } else if (dcl == kernel.fg.builder->getStackCallRet()) {
    if (stackCallRetLR == nullptr) {
      lr = new (LSMem) LSLiveRange();
      stackCallRetLR = lr;
      lr->setTopDcl(dcl);
      allocForbiddenVector(lr);
    } else {
      lr = stackCallRetLR;
    }
  }
  // Check whether local LR is a candidate
  if (lr->isGRFRegAssigned() == false) {
    if (!lr->isPushedToIntervalList()) {
      liveIntervals.push_back(lr);
      lr->setPushed(true);
    }

    unsigned int startIdx;
    if (lr->getFirstRef(startIdx) == NULL && startIdx == 0) {
      lr->setFirstRef(curInst, curInst->getLexicalId() * 2);
    }

    // For dst, we set the last ref = lexcial_ID * 2 + 1.
    // So that the dst will not be released immediately if it's defined only.
    // This is to handle some special workload. Such as for LIT test
    lr->setLastRef(curInst, curInst->getLexicalId() * 2 + 1);
  } else if (dcl->getRegVar()
                 ->getPhyReg()
                 ->isGreg()) // Assigned already and is GRF
  {                          // Such as stack call varaibles
    unsigned int startIdx;
    if (!lr->isPushedToIntervalList()) {
      if (!curInst->isFCall()) {
        liveIntervals.push_back(lr);
      }
      lr->setPushed(true);

      // Mark live range as assigned
      setPreAssignedLR(lr, preAssignedLiveIntervals);
    }

    if (lr->getFirstRef(startIdx) == NULL && startIdx == 0) {
      lr->setFirstRef(curInst, curInst->getLexicalId() * 2);
    }
    // For dst, we set the last ref = lexcial_ID * 2 + 1.
    // So that the dst will not be released immediately if it's defined only.
    lr->setLastRef(curInst, curInst->getLexicalId() * 2 + 1);
  }

  if (lr->isEOT() && std::find(eotLiveIntervals.begin(), eotLiveIntervals.end(),
                               lr) == eotLiveIntervals.end()) {
    eotLiveIntervals.push_back(lr);
  }

  return;
}

void LinearScanRA::setSrcReferences(
    G4_BB *bb, INST_LIST_ITER inst_it, int srcIdx, G4_Declare *dcl,
    std::vector<LSLiveRange *> &liveIntervals,
    std::vector<LSLiveRange *> &eotLiveIntervals) {
  G4_INST *curInst = (*inst_it);
  LSLiveRange *lr = gra.getSafeLSLR(dcl);

  if (!lr && dcl->getRegFile() == G4_GRF) {
    lr = CreateLocalLiveRange(dcl);
  }

  if (!lr)
    return;

  if (dcl->isOutput() && !lr->isGRFRegAssigned()) {
    lr->setLastRef(curInst, lastInstLexID * 2 + 1);
    if (!lr->isPushedToIntervalList()) {
      liveThroughIntervals.push_back(lr);
      lr->setPushed(true);
    }
    return;
  }

  if ((dcl->getRegFile() == G4_INPUT &&
       dcl != kernel.fg.builder->getStackCallRet() &&
       dcl != kernel.fg.builder->getStackCallArg()) ||
      (lr->isGRFRegAssigned() && (!dcl->getRegVar()->isGreg()))) // ARF
  {
    return;
  }

  if (!lr->isPushedToIntervalList()) {
    liveIntervals.push_back(lr);
    lr->setPushed(true);
    gra.addUndefinedDcl(dcl);

    unsigned int startIdx;
    if (lr->getFirstRef(startIdx) == NULL &&
        startIdx == 0) { // Since we scan from front to end, not referenced
                         // before means not defined.

      if (lr->isGRFRegAssigned() && dcl->getRegVar()->getPhyReg()->isGreg()) {
        lr->setFirstRef(nullptr, 1);
        lr->setValid(false);
        setPreAssignedLR(lr, preAssignedLiveIntervals);
      } else // Not pre-asssigned, temp
      {
        lr->setFirstRef(curInst, curInst->getLexicalId() * 2);
      }
    }
  }

  lr->setLastRef(curInst, curInst->getLexicalId() * 2);

  unsigned firstIdx = 0;
  if ((builder.WaDisableSendSrcDstOverlap() &&
       ((curInst->isSend() && srcIdx == 0) ||
        (curInst->isSplitSend() && srcIdx == 1))) ||
      (curInst->isDpas() &&
       srcIdx == 1) || // For DPAS, as part of same instruction, src1 should not
                       // have overlap with dst.
      (builder.avoidDstSrcOverlap() && curInst->getDst() != NULL &&
       hasDstSrcOverlapPotential(curInst->getDst(),
                                 curInst->getSrc(srcIdx)->asSrcRegRegion())) ||
      lr->getFirstRef(firstIdx) == curInst) {
    lr->setLastRef(curInst, curInst->getLexicalId() * 2 + 1);
  }

  if (lr->isEOT() && std::find(eotLiveIntervals.begin(), eotLiveIntervals.end(),
                               lr) == eotLiveIntervals.end()) {
    eotLiveIntervals.push_back(lr);
  }

  return;
}

void LinearScanRA::generateInputIntervals(
    G4_Declare *topdcl, G4_INST *inst, std::vector<uint32_t> &inputRegLastRef,
    PhyRegsLocalRA &initPregs, bool avoidSameInstOverlap) {
  unsigned int instID = inst->getLexicalId();
  G4_RegVar *var = topdcl->getRegVar();
  unsigned int regNum = var->getPhyReg()->asGreg()->getRegNum();
  unsigned int regOff = var->getPhyRegOff();
  unsigned int idx = regNum * builder.numEltPerGRF<Type_UW>() +
                     (regOff * topdcl->getElemSize()) / G4_WSIZE +
                     topdcl->getWordSize() - 1;
  //Variable size may be larger than occupied GRF
  if (idx > (kernel.getNumRegTotal() * builder.numEltPerGRF<Type_UW>() - 1)) {
    idx = kernel.getNumRegTotal() * builder.numEltPerGRF<Type_UW>() - 1;
  }
  unsigned int numWords = topdcl->getWordSize();
  for (int i = numWords - 1; i >= 0; --i, --idx) {
    if ((inputRegLastRef[idx] == UINT_MAX || inputRegLastRef[idx] < instID) &&
        initPregs.isGRFAvailable(idx / builder.numEltPerGRF<Type_UW>())) {
      inputRegLastRef[idx] = instID;
      if (avoidSameInstOverlap) {
        inputIntervals.push_front(new (LSMem)
                                      LSInputLiveRange(idx, instID * 2 + 1));
      } else {
        inputIntervals.push_front(new (LSMem)
                                      LSInputLiveRange(idx, instID * 2));
      }

      if (kernel.getOptions()->getOption(vISA_GenerateDebugInfo)) {
        updateDebugInfo(kernel, topdcl, 0, inst->getVISAId());
      }
    }
  }

  initPregs.markPhyRegs(topdcl);

  return;
}

// Generate the input intervals for current BB.
// The input live ranges either live through current BB or killed by current BB.
// So, it's enough we check the live out of the BB and the BB it's self
void LinearScanRA::calculateInputIntervalsGlobal(PhyRegsLocalRA &initPregs) {
  int numGRF = kernel.getNumRegTotal();
  std::vector<uint32_t> inputRegLastRef(
      numGRF * builder.numEltPerGRF<Type_UW>(), UINT_MAX);
  G4_INST *lastInst = nullptr;

  for (BB_LIST_RITER bb_it = kernel.fg.rbegin(), bb_rend = kernel.fg.rend();
       bb_it != bb_rend; bb_it++) {
    G4_BB *bb = (*bb_it);

    //@ the end of BB
    if (BBVector[bb->getId()].hasBackEdgeOut()) {
      for (auto dcl : globalDeclares) {
        if (dcl->getAliasDeclare() != NULL || dcl->isSpilled())
          continue;

        if (dcl->getRegFile() == G4_INPUT &&
            dcl->getRegVar()
                ->isGreg() && // Filter out the architecture registers
            dcl->isOutput() ==
                false && // Input and out should be marked as unavailable
            !builder.isPreDefArg(dcl) && // Not stack call associated variables
            l.isLiveAtExit(bb, dcl->getRegVar()->getId())) {
          vISA_ASSERT(dcl->getRegVar()->isPhyRegAssigned(),
                      "Input variable has no pre-assigned physical register");
          generateInputIntervals(dcl, bb->getInstList().back(), inputRegLastRef,
                                 initPregs, false);
        }
      }
    }

    //@BB
    for (INST_LIST_RITER inst_it = bb->rbegin(), inst_rend = bb->rend();
         inst_it != inst_rend; inst_it++) {
      G4_INST *curInst = (*inst_it);
      G4_Declare *topdcl = NULL;

      if (lastInst == nullptr) {
        lastInst = curInst;
      }
      // scan dst operand (may be unnecessary but added for safety)
      if (curInst->getDst() != NULL) {
        // Scan dst
        G4_DstRegRegion *dst = curInst->getDst();
        topdcl = GetTopDclFromRegRegion(dst);

        if (topdcl && topdcl->getRegFile() == G4_INPUT &&
            topdcl->getRegVar()->isGreg() && topdcl->isOutput() == false &&
            !builder.isPreDefArg(topdcl)) {
          generateInputIntervals(topdcl, curInst, inputRegLastRef, initPregs,
                                 false);
        }
      }

      // Scan src operands
      for (int i = 0, nSrcs = curInst->getNumSrc(); i < nSrcs; i++) {
        G4_Operand *src = curInst->getSrc(i);

        if (src == nullptr || src->isNullReg()) {
          continue;
        }

        if (src->getTopDcl()) {
          topdcl = GetTopDclFromRegRegion(src);

          if (topdcl && topdcl->getRegFile() == G4_INPUT &&
              (topdcl->getRegVar()->isGreg()) && topdcl->isOutput() == false &&
              !builder.isPreDefArg(topdcl)) {
            // Check whether it is input
            if (builder.avoidDstSrcOverlap() && curInst->getDst() != NULL &&
                hasDstSrcOverlapPotential(curInst->getDst(),
                                          src->asSrcRegRegion())) {
              generateInputIntervals(topdcl, curInst, inputRegLastRef,
                                     initPregs, true);
            } else {
              if (l.isLiveAtEntry(bb, topdcl->getRegVar()->getId())) {
                generateInputIntervals(topdcl, curInst, inputRegLastRef,
                                       initPregs, false);
              } else // Not capture by liveness analysis
              {
                generateInputIntervals(topdcl, lastInst, inputRegLastRef,
                                       initPregs, false);
              }
            }
          }
        } else if (src->isAddrExp()) {
          G4_AddrExp *addrExp = src->asAddrExp();

          topdcl = addrExp->getRegVar()->getDeclare();
          while (topdcl->getAliasDeclare() != NULL)
            topdcl = topdcl->getAliasDeclare();

          vISA_ASSERT(topdcl != NULL, "Top dcl was null for addr exp opnd");

          if (topdcl->getRegFile() == G4_INPUT &&
              topdcl->getRegVar()->isGreg() && topdcl->isOutput() == false &&
              !builder.isPreDefArg(topdcl)) {
            generateInputIntervals(topdcl, curInst, inputRegLastRef, initPregs,
                                   false);
          }
        }
      }
    }
  }

  return;
}

//
//@ the entry of BB
//
void LinearScanRA::calculateLiveInIntervals(
    G4_BB *bb, std::vector<LSLiveRange *> &liveIntervals) {
  // FIXME: The complexity is "block_num * declare_num"
  std::vector<LSLiveRange *> preAssignedLiveIntervals;

  for (auto dcl : globalDeclares) {
    if (dcl->getAliasDeclare() != NULL || dcl->getRegFile() == G4_INPUT ||
        dcl->isSpilled()) {
      continue;
    }
    LSLiveRange *lr = gra.getSafeLSLR(dcl);
    if (lr && !l.isEmptyLiveness() &&
        l.isLiveAtEntry(bb, dcl->getRegVar()->getId())) // Live in current BB
    {
      if (!lr->isPushedToIntervalList()) {
        if (lr->isGRFRegAssigned() && dcl->getRegVar()->isGreg()) {
          setPreAssignedLR(lr, preAssignedLiveIntervals);
        } else {
          liveIntervals.push_back(lr);
        }
        lr->setPushed(true);
      }

      unsigned curIdx = 0;
      if (lr->getFirstRef(curIdx) == NULL &&
          curIdx == 0) // not referenced before, assigned or not assigned?
      {
        lr->setFirstRef((*bb->begin()), (*bb->begin())->getLexicalId() * 2);
      }
    } else // Live in the exist BB of a loop
    {
      // Extend the live ranges of live in variable of exit BB of the loop to
      // the head of loop.
      std::map<G4_BB *, std::vector<G4_BB *>>::iterator loopHEIt =
          loopHeadExitMap.find(bb);
      if (loopHEIt !=
          loopHeadExitMap.end()) // If current BB is the head of a loop
      {
        for (auto exitBB : loopHeadExitMap[bb]) // for all exit BBs.
        {
          if (lr && !l.isEmptyLiveness() &&
              l.isLiveAtEntry(exitBB, dcl->getRegVar()->getId())) {
            if (!lr->isPushedToIntervalList()) // Is the interval pushed or not
            {
              if (lr->isGRFRegAssigned() && dcl->getRegVar()->isGreg()) {
                setPreAssignedLR(lr, preAssignedLiveIntervals);
              } else {
                liveIntervals.push_back(lr);
              }
              lr->setPushed(true);
            }

            unsigned curIdx = 0;
            if (lr->getFirstRef(curIdx) == NULL &&
                curIdx == 0) // not referenced before
            {
              lr->setFirstRef((*bb->begin()),
                              (*bb->begin())->getLexicalId() * 2);
            }
          }
        }
      }
    }
  }

  if (preAssignedLiveIntervals.size() &&
      bb->getId() == 0) // Should happen in the entry BB
  {
    liveIntervals.insert(liveIntervals.begin(),
                         preAssignedLiveIntervals.begin(),
                         preAssignedLiveIntervals.end());
  }

  return;
}

void LinearScanRA::calculateCurrentBBLiveIntervals(
    G4_BB *bb, std::vector<LSLiveRange *> &liveIntervals,
    std::vector<LSLiveRange *> &eotLiveIntervals) {
  for (INST_LIST_ITER inst_it = bb->begin(), bbend = bb->end();
       inst_it != bbend; inst_it++) {
    G4_INST *curInst = (*inst_it);
    G4_Declare *topdcl = NULL;

    if (curInst->isPseudoKill() || curInst->isLifeTimeEnd() ||
        curInst->isLabel()) {
      continue;
    }

    if (curInst->isCall() == true) {
      const char *name =
          kernel.fg.builder->getNameString(32, "SCALL_%d", funcCnt++);
      G4_Declare *scallDcl =
          kernel.fg.builder->createDeclare(name, G4_GRF, 1, 1, Type_UD);
      LSLiveRange *lr = CreateLocalLiveRange(scallDcl);

      liveIntervals.push_back(lr);
      lr->setPushed(true);
      lr->setFirstRef(curInst, curInst->getLexicalId() * 2);

      FuncInfo *callee = bb->getCalleeInfo();
      unsigned int funcId = callee->getId();
      lr->setLastRef(curInst, funcLastLexID[funcId]);
      lr->setIsCallSite(true);
    }

    if (curInst->isFCall()) {
      auto fcall = kernel.fg.builder->getFcallInfo(curInst);
      G4_Declare *arg = kernel.fg.builder->getStackCallArg();
      G4_Declare *ret = kernel.fg.builder->getStackCallRet();
      vISA_ASSERT(fcall != std::nullopt, "fcall info not found");

      uint16_t retSize = fcall->getRetSize();
      uint16_t argSize = fcall->getArgSize();
      if (ret && retSize > 0 && ret->getRegVar()) {
        LSLiveRange *stackCallRetLR = new (LSMem) LSLiveRange();
        stackCallRetLR->setTopDcl(ret);
        allocForbiddenVector(stackCallRetLR);
        stackCallRetLR->setFirstRef(curInst, curInst->getLexicalId() * 2);
        liveIntervals.push_back(stackCallRetLR);
        stackCallRetLR->setPushed(true);
      }
      if (stackCallArgLR && arg && argSize > 0 && arg->getRegVar()) {
        stackCallArgLR->setLastRef(
            curInst, curInst->getLexicalId() * 2 -
                         1); // Minus one so that arguments will not be spilled
      }
    }

    if (curInst->isFReturn()) {
      uint16_t retSize = kernel.fg.builder->getRetVarSize();
      if (retSize && stackCallRetLR) {
        stackCallRetLR->setLastRef(curInst, curInst->getLexicalId() * 2);
        stackCallRetLR = nullptr;
      }
    }

    // Scan srcs
    for (int i = 0, nSrcs = curInst->getNumSrc(); i < nSrcs; i++) {
      G4_Operand *src = curInst->getSrc(i);

      if (src == nullptr || src->isNullReg()) {
        continue;
      }

      if (src && src->isSrcRegRegion()) {
        if (src->asSrcRegRegion()->isIndirect()) {
          auto pointsToSet = l.getPointsToAnalysis().getAllInPointsTo(
              src->getBase()->asRegVar());
          for (const auto& pt : *pointsToSet) {
            G4_Declare *dcl = pt.var->getDeclare()->getRootDeclare();

            setSrcReferences(bb, inst_it, i, dcl, liveIntervals,
                             eotLiveIntervals);
          }
        } else {
          // Scan all srcs
          topdcl = GetTopDclFromRegRegion(src);
          if (topdcl) {
            setSrcReferences(bb, inst_it, i, topdcl, liveIntervals,
                             eotLiveIntervals);
          }
        }
      }
    }

    // Scan dst
    G4_DstRegRegion *dst = curInst->getDst();
    if (dst) {
      if (dst->isIndirect()) {
        auto pointsToSet = l.getPointsToAnalysis().getAllInPointsTo(
            dst->getBase()->asRegVar());
        for (const auto& pt : *pointsToSet) {
          G4_Declare *dcl = pt.var->getDeclare()->getRootDeclare();

          setDstReferences(bb, inst_it, dcl, liveIntervals, eotLiveIntervals);
        }
      } else {
        topdcl = GetTopDclFromRegRegion(dst);
        if (topdcl) {
          setDstReferences(bb, inst_it, topdcl, liveIntervals,
                           eotLiveIntervals);
        }
      }
    }
  }

  return;
}

void LinearScanRA::calculateLiveOutIntervals(
    G4_BB *bb, std::vector<LSLiveRange *> &liveIntervals) {
  for (auto dcl : globalDeclares) {
    if (dcl->getAliasDeclare() != NULL || dcl->getRegFile() == G4_INPUT ||
        dcl->isSpilled())
      continue;

    LSLiveRange *lr = gra.getSafeLSLR(dcl);
    if (lr && l.isLiveAtExit(bb, dcl->getRegVar()->getId())) {
      lr->setLastRef(bb->getInstList().back(),
                     bb->getInstList().back()->getLexicalId() * 2 + 1);
    }
  }

  return;
}

void LinearScanRA::calculateLiveThroughIntervals() {
  for (auto dcl : globalDeclares) {
    if (dcl->getAliasDeclare() != NULL || dcl->getRegFile() == G4_INPUT ||
        dcl->isSpilled())
      continue;

    LSLiveRange *lr = gra.getSafeLSLR(dcl);
    if (lr && dcl->isInput() && dcl->isOutput() && !lr->isGRFRegAssigned()) {
      lr->setFirstRef(nullptr, 0);
      lr->setLastRef(nullptr, lastInstLexID * 2 + 1);
      if (!lr->isPushedToIntervalList()) {
        liveThroughIntervals.push_back(lr);
        lr->setPushed(true);
      }
    }
  }

  return;
}

//
// Live intervals:
// 1. not input variables
// 2. variables without assigned value: normal Intervals.
// 3. variables without assigned value, without define: wired, added by front
// end. Such as cmp f1.0,  v11, v11. @BB only
// 4. variables which are pre-defined with registers: such as stack call
// pre-defined varaibles. @BB only
// 5. variables which are pre-defined but will not be assigned with registers:
// such as %null.  exclusive
// 6. variables which are assigned in previuos region (BB, BBs or function, ..).
// //@entry BB
// 7. live in of region: pre-assigned, or not.
// 8. live out of region: set the last reference.
//
void LinearScanRA::calculateLiveIntervalsGlobal(
    G4_BB *bb, std::vector<LSLiveRange *> &liveIntervals,
    std::vector<LSLiveRange *> &eotLiveIntervals) {
  //@ the entry of BB
  if (bb->getId() == 0 || BBVector[bb->getId()].hasBackEdgeIn()) {
    calculateLiveInIntervals(bb, liveIntervals);
  }

  //@ BB
  calculateCurrentBBLiveIntervals(bb, liveIntervals, eotLiveIntervals);

  //@ the exit of BB
  if (BBVector[bb->getId()].hasBackEdgeOut()) {
    calculateLiveOutIntervals(bb, liveIntervals);
  }

  return;
}

void LinearScanRA::printLiveIntervals(
    std::vector<LSLiveRange *> &liveIntervals) {
  for (auto lr : liveIntervals) {
    unsigned int start, end;

    lr->getFirstRef(start);
    lr->getLastRef(end);

    std::cout << lr->getTopDcl()->getName() << "(" << start << ", " << end
              << ", " << lr->getTopDcl()->getByteSize() << ")";
    std::cout << "\n";
  }

  std::cout << "\n";
}

void LinearScanRA::printSpillLiveIntervals(
    std::list<LSLiveRange *> &liveIntervals) {
  for (auto lr : liveIntervals) {
    unsigned int start, end;

    lr->getFirstRef(start);
    lr->getLastRef(end);

    std::cout << lr->getTopDcl()->getName() << "(" << start << ", " << end
              << ", " << lr->getTopDcl()->getByteSize() << ")";
    std::cout << "\n";
  }

  std::cout << "\n";
}

void LinearScanRA::printInputLiveIntervalsGlobal() {
  COUT_ERROR << "\n"
             << "Input Live intervals "
             << "\n";

  for (std::list<LSInputLiveRange *>::iterator it = inputIntervals.begin();
       it != inputIntervals.end(); it++) {
    unsigned int regWordIdx, lrEndIdx, regNum, subRegInWord;

    LSInputLiveRange *lr = (*it);

    regWordIdx = lr->getRegWordIdx();
    regNum = regWordIdx / builder.numEltPerGRF<Type_UW>();
    subRegInWord = regWordIdx % builder.numEltPerGRF<Type_UW>();
    lrEndIdx = lr->getLrEndIdx();

    COUT_ERROR << "r" << regNum << "." << subRegInWord << " " << lrEndIdx;
    COUT_ERROR << "\n";
  }

  COUT_ERROR << "\n";
}

static inline void printLiveInterval(LSLiveRange *lr, bool assign,
                                     const IR_Builder &builder) {
  int startregnum, endregnum, startsregnum = 0, endsregnum;
  G4_VarBase *op;
  op = lr->getPhyReg(startsregnum);

  startregnum = endregnum = op->asGreg()->getRegNum();
  endsregnum =
      startsregnum +
      (lr->getTopDcl()->getTotalElems() * lr->getTopDcl()->getElemSize() / 2) -
      1;

  if (lr->getTopDcl()->getNumRows() > 1) {
    endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;

    if (lr->getTopDcl()->getWordSize() > 0) {
      endsregnum =
          lr->getTopDcl()->getWordSize() % builder.numEltPerGRF<Type_UW>() - 1;
      if (endsregnum < 0)
        endsregnum = 15;
    } else
      endsregnum = 15; // last word in GRF
  }
  if (assign) {
    COUT_ERROR << "Assigned physical register to ";
  } else {
    COUT_ERROR << "Free physical register of ";
  }
  COUT_ERROR << lr->getTopDcl()->getName() << " (r" << startregnum << "."
             << startsregnum << ":w - "
             << "r" << endregnum << "." << endsregnum << ":w)"
             << "\n";

  return;
}

globalLinearScan::globalLinearScan(
    GlobalRA &g, LivenessAnalysis *l, std::vector<LSLiveRange *> &lv,
    std::vector<LSLiveRange *> *assignedLiveIntervals,
    std::list<LSInputLiveRange *, std_arena_based_allocator<LSInputLiveRange *>>
        &inputLivelIntervals,
    PhyRegsManager &pregMgr, unsigned int numReg, unsigned int numEOT,
    unsigned int lastLexID, bool bankConflict, bool internalConflict, Mem_Manager* GLSMem)
    : gra(g), builder(g.builder), GLSMem(GLSMem), pregManager(pregMgr),
      liveIntervals(lv), preAssignedIntervals(assignedLiveIntervals),
      inputIntervals(inputLivelIntervals), numRegLRA(numReg), numRowsEOT(numEOT),
      lastLexicalID(lastLexID),  doBankConflict(bankConflict),
      highInternalConflict(internalConflict) {
  startGRFReg = 0;
  activeGRF.resize(g.kernel.getNumRegTotal());
  for (auto lr : inputLivelIntervals) {
    unsigned int regnum = lr->getRegWordIdx() / builder.numEltPerGRF<Type_UW>();
    activeGRF[regnum].activeInput.push_back(lr);
  }
}

void globalLinearScan::getCalleeSaveGRF(std::vector<unsigned int> &regNum,
                                        G4_Kernel *kernel) {
  unsigned int startCallerSave = kernel->stackCall.calleeSaveStart();
  unsigned int endCallerSave = startCallerSave + kernel->stackCall.getNumCalleeSaveRegs();

  for (auto active_it = active.begin(); active_it != active.end();
       active_it++) {
    LSLiveRange *lr = (*active_it);

    G4_VarBase *op;
    int startsregnum = 0;
    op = lr->getPhyReg(startsregnum);
    unsigned startregnum = op->asGreg()->getRegNum();
    unsigned endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;

    for (unsigned i = startregnum; i <= endregnum; i++) {
      if (i >= startCallerSave && i <= endCallerSave) {
        regNum.push_back(i);
      }
    }
  }

  return;
}

void globalLinearScan::getCallerSaveGRF(std::vector<unsigned int> &regNum,
                                        std::vector<unsigned int> &retRegNum,
                                        G4_Kernel *kernel) {
  unsigned int startCalleeSave = 1;
  unsigned int endCalleeSave = startCalleeSave + kernel->stackCall.getCallerSaveLastGRF();

  for (auto active_it = active.begin(); active_it != active.end();
       active_it++) {
    LSLiveRange *lr = (*active_it);
    G4_Declare *dcl = lr->getTopDcl();

    if (!builder.kernel.fg.isPseudoVCEDcl(dcl) && !builder.isPreDefArg(dcl)) {
      G4_VarBase *op;
      int startsregnum = 0;
      op = lr->getPhyReg(startsregnum);
      unsigned startregnum = op->asGreg()->getRegNum();
      unsigned endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;

      for (unsigned i = startregnum; i <= endregnum; i++) {
        if (i >= startCalleeSave && i < endCalleeSave) {
          if (builder.isPreDefRet(dcl)) {
            retRegNum.push_back(i);
          } else {
            regNum.push_back(i);
          }
        }
      }
    }
  }

  for (auto inputlr : inputIntervals) {
    unsigned int regnum =
        inputlr->getRegWordIdx() / builder.numEltPerGRF<Type_UW>();
    std::vector<unsigned int>::iterator it =
        std::find(regNum.begin(), regNum.end(), regnum);
    if (it == regNum.end()) {
      if (regnum >= startCalleeSave && regnum < endCalleeSave) {
        regNum.push_back(regnum);
      }
    }
  }
}

bool LinearScanRA::isUseUnAvailableRegister(uint32_t startReg,
                                            uint32_t regNum) {
  for (uint32_t i = startReg; i < startReg + regNum; ++i) {
    if (!pregs->isGRFAvailable(i)) {
      return true;
    }
  }

  return false;
}

bool LinearScanRA::assignEOTLiveRanges(
    IR_Builder &builder, std::vector<LSLiveRange *> &liveIntervals) {
#ifdef DEBUG_VERBOSE_ON
  COUT_ERROR << "--------------------------------- "
             << "\n";
#endif
  uint32_t nextEOTGRF = numRegLRA - numRowsEOT;
  for (auto lr : liveIntervals) {
    vASSERT(lr->isEOT());
    G4_Declare *dcl = lr->getTopDcl();
    G4_Greg *phyReg = builder.phyregpool.getGreg(nextEOTGRF);
    dcl->getRegVar()->setPhyReg(phyReg, 0);
    lr->setPhyReg(phyReg, 0);
    lr->setAssigned(true);
    lr->setUseUnAvailableReg(
        isUseUnAvailableRegister(nextEOTGRF, dcl->getNumRows()));
    nextEOTGRF += dcl->getNumRows();
    if (nextEOTGRF > numRegLRA) {
      vASSERT(false);
    }
#ifdef DEBUG_VERBOSE_ON
    printLiveInterval(lr, true, builder);
#endif
  }

  return true;
}

void globalLinearScan::updateCallSiteLiveIntervals(LSLiveRange *callSiteLR) {
  unsigned lastIdx = 0;
  G4_INST *inst = callSiteLR->getLastRef(lastIdx);

  for (auto lr : active) {
    unsigned curLastIdx;
    lr->getLastRef(curLastIdx);
    if (curLastIdx < lastIdx) {
      lr->setLastRef(inst, lastIdx);
    }
  }

  for (auto inputlr : inputIntervals) {
    unsigned curLastIdx = inputlr->getLrEndIdx();
    if (curLastIdx < lastIdx) {
      inputlr->setLrEndIdx(lastIdx);
    }
  }

  return;
}

bool globalLinearScan::runLinearScan(IR_Builder &builder,
                                     std::vector<LSLiveRange *> &liveIntervals,
                                     std::list<LSLiveRange *> &spillLRs) {
  unsigned int idx = 0;
  bool allocateRegResult = false;

#ifdef DEBUG_VERBOSE_ON
  COUT_ERROR << "--------------------------------- "
             << "\n";
#endif

  for (auto lr : liveIntervals) {
    G4_Declare *dcl = lr->getTopDcl();

    lr->getFirstRef(idx);
    if (!lr->isEOT() && !lr->getAssigned()) {
      // Add forbidden for preAssigned registers
      auto isTopDclPseudoVCA = builder.kernel.fg.isPseudoVCADcl(lr->getTopDcl());
      for (auto preAssginedLI : *preAssignedIntervals) {
        if (isTopDclPseudoVCA &&
            (builder.isPreDefRet(preAssginedLI->getTopDcl()) ||
             builder.isPreDefArg(preAssginedLI->getTopDcl()))) {
          continue;
        }

        unsigned preFirstIdx, preLastIdx;
        preAssginedLI->getFirstRef(preFirstIdx);
        preAssginedLI->getLastRef(preLastIdx);

        unsigned lastIdx = 0;
        lr->getLastRef(lastIdx);

        if (!(lastIdx < preFirstIdx || preLastIdx < idx)) {
          G4_VarBase *preg;
          int subregnumword;

          preg = preAssginedLI->getPhyReg(subregnumword);
          unsigned reg = preg->asGreg()->getRegNum();
          unsigned rowNum = preAssginedLI->getTopDcl()->getNumRows();

          for (unsigned k = 0; k < rowNum; k++) {
            lr->addForbidden(reg + k);
          }
        }
      }
    }

#ifdef DEBUG_VERBOSE_ON
    COUT_ERROR << "-------- IDX: " << idx << "---------"
               << "\n";
#endif

    // Expire the live ranges ended befoe idx
    expireGlobalRanges(idx);
    expireInputRanges(idx);

    if (lr->isCallSite()) {
      updateCallSiteLiveIntervals(lr);
      continue;
    }

    if (builder.kernel.fg.isPseudoVCADcl(dcl)) {
      std::vector<unsigned int> callerSaveRegs;
      std::vector<unsigned int> regRegs;
      getCallerSaveGRF(callerSaveRegs, regRegs, &gra.kernel);
      for (unsigned int i = 0; i < callerSaveRegs.size(); i++) {
        unsigned int callerSaveReg = callerSaveRegs[i];
        lr->addForbidden(callerSaveReg);
      }
      for (unsigned int i = 0; i < regRegs.size(); i++) {
        unsigned int callerSaveReg = regRegs[i];
        if (lr->getRetGRFs() == nullptr) {
          allocRetRegsVector(lr);
        }
        lr->addRetRegs(callerSaveReg);
      }
      continue;
    } else if (builder.kernel.fg.isPseudoVCEDcl(dcl)) {
      calleeSaveLR = lr;
      continue;
    } else if (!lr->getAssigned()) {
      if (dcl == gra.getOldFPDcl()) {
        std::vector<unsigned int> callerSaveRegs;
        std::vector<unsigned int> regRegs;
        getCallerSaveGRF(callerSaveRegs, regRegs, &gra.kernel);
        for (unsigned int i = 0; i < callerSaveRegs.size(); i++) {
          unsigned int callerSaveReg = callerSaveRegs[i];
          lr->addForbidden(callerSaveReg);
        }
        for (unsigned int i = 0; i < regRegs.size(); i++) {
          unsigned int callerSaveReg = regRegs[i];
          if (lr->getRetGRFs() == nullptr) {
            allocRetRegsVector(lr);
          }
          lr->addRetRegs(callerSaveReg);
        }
      }

      allocateRegResult = allocateRegsLinearScan(lr, builder);
#ifdef DEBUG_VERBOSE_ON
      if (allocateRegResult) {
        printLiveInterval(lr, true, builder);
      }
#endif
    } else {
      allocateRegResult = true;
      int startregnum, subregnum;
      G4_VarBase *op;
      op = lr->getPhyReg(subregnum);

      startregnum = op->asGreg()->getRegNum();
      int nrows = 0;
      int lastRowSize = 0;
      int size = lr->getSizeInWords(builder);
      LinearScanRA::getRowInfo(size, nrows, lastRowSize, builder);

      if (!lr->isUseUnAvailableReg()) {
        if ((unsigned)size >= builder.numEltPerGRF<Type_UW>()) {
          if (size % builder.numEltPerGRF<Type_UW>() == 0) {
            pregManager.getAvailableRegs()->setGRFBusy(
                startregnum, lr->getTopDcl()->getNumRows());
          } else {
            pregManager.getAvailableRegs()->setGRFBusy(
                startregnum, lr->getTopDcl()->getNumRows() - 1);
            pregManager.getAvailableRegs()->setWordBusy(
                startregnum + lr->getTopDcl()->getNumRows() - 1, 0,
                lastRowSize);
          }
        } else {
          pregManager.getAvailableRegs()->setWordBusy(startregnum, subregnum,
                                                      size);
        }
      }
    }

    if (allocateRegResult) {
      updateGlobalActiveList(lr);
    } else // Spill
    {
      if (spillFromActiveList(lr, spillLRs)) {
        // Fixme: get the start GRF already, can allocate immediately
        allocateRegResult = allocateRegsLinearScan(lr, builder);
        if (!allocateRegResult) {
#ifdef DEBUG_VERBOSE_ON
          COUT_ERROR << "Failed assigned physical register to "
                     << lr->getTopDcl()->getName()
                     << ", rows :" << lr->getTopDcl()->getNumRows() << "\n";
          printActives();
#endif
          return false;
        } else {
          updateGlobalActiveList(lr);
#ifdef DEBUG_VERBOSE_ON
          printLiveInterval(lr, true, builder);
#endif
        }
      } else {
#ifdef DEBUG_VERBOSE_ON
        COUT_ERROR << "Failed to spill registers for "
                   << lr->getTopDcl()->getName()
                   << ", rows :" << lr->getTopDcl()->getNumRows() << "\n";
        printActives();
#endif
        return false;
      }
    }
  }

  int totalGRFNum = builder.kernel.getNumRegTotal();
  for (int i = 0; i < totalGRFNum; i++) {
    activeGRF[i].activeLV.clear();
    activeGRF[i].activeInput.clear();
  }

  // Assign the registers for the live out ones
  expireAllActive();

  if (builder.kernel.getOptions()->getOption(vISA_GenerateDebugInfo)) {
    updateDebugInfo(builder.kernel, liveIntervals);
  }

  return true;
}

void globalLinearScan::updateGlobalActiveList(LSLiveRange *lr) {
  bool done = false;
  unsigned int newlr_end;

  lr->getLastRef(newlr_end);

  for (auto active_it = active.begin(); active_it != active.end();
       active_it++) {
    unsigned int end_idx;
    LSLiveRange *active_lr = (*active_it);

    active_lr->getLastRef(end_idx);

    if (end_idx > newlr_end) {
      active.insert(active_it, lr);
      done = true;
      break;
    }
  }

  if (done == false)
    active.push_back(lr);

#ifdef DEBUG_VERBOSE_ON
  COUT_ERROR << "Add active " << lr->getTopDcl()->getName() << "\n";
#endif

  G4_VarBase *op;
  int startsregnum = 0;
  op = lr->getPhyReg(startsregnum);
  unsigned startregnum = op->asGreg()->getRegNum();
  unsigned endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;
  for (unsigned i = startregnum; i <= endregnum; i++) {
    activeGRF[i].activeLV.push_back(lr);
#ifdef DEBUG_VERBOSE_ON
    COUT_ERROR << "Add activeGRF " << lr->getTopDcl()->getName()
               << " Reg: " << i << "\n";
#endif
  }
}

bool globalLinearScan::insertLiveRange(std::list<LSLiveRange *> *liveIntervals,
                                       LSLiveRange *lr) {
  unsigned int idx = 0;
  lr->getFirstRef(idx);
  std::list<LSLiveRange *>::iterator it = liveIntervals->begin();
  while (it != liveIntervals->end()) {
    LSLiveRange *curLR = (*it);
    unsigned curIdx = 0;
    curLR->getFirstRef(curIdx);
    if (curIdx > idx) {
      liveIntervals->insert(it, lr);
      return true;
    }

    it++;
  }

  return false;
}

bool globalLinearScan::canBeSpilledLR(LSLiveRange *lr) {
  if (lr->isUseUnAvailableReg()) {
    return false;
  }

  if (lr->isEOT()) {
    return false;
  }

  if (lr->getTopDcl() == builder.getBuiltinR0()) {
    return false;
  }

  if (lr->getTopDcl()->isDoNotSpill()) {
    return false;
  }

  if (lr->isCall()) {
    return false;
  }

  if (lr->isGRFRegAssigned()) {
    return false;
  }

  if (lr->getTopDcl()->isSpilled()) {
    return false;
  }

  if (lr->getTopDcl()->getRegFile() == G4_INPUT) {
    return false;
  }

  if (lr->getTopDcl()->getRegVar()->getId() == UNDEFINED_VAL) {
    return false;
  }

  if (lr->getTopDcl()->getRegVar()->isRegVarTransient() ||
      lr->getTopDcl()->getRegVar()->isRegVarTmp()) {
    return false;
  }

  // Stack call variables
  if (lr->getTopDcl() == gra.getOldFPDcl()) {
    return false;
  }

  if (builder.kernel.fg.isPseudoVCADcl(lr->getTopDcl()) ||
      builder.kernel.fg.isPseudoVCEDcl(lr->getTopDcl())) {
    return false;
  }

  return true;
}

int globalLinearScan::findSpillCandidate(LSLiveRange *tlr) {
  unsigned short requiredRows = tlr->getTopDcl()->getNumRows();
  int referenceCount = 0;
  int startGRF = -1;
  float spillCost = (float)(int)0x7FFFFFFF;
  unsigned lastIdxs = 1;
  unsigned tStartIdx = 0;

  tlr->getFirstRef(tStartIdx);
  BankAlign bankAlign = getBankAlign(tlr);
  for (int i = 0; i < (int)(numRegLRA - requiredRows); i++) {
    unsigned endIdx = 0;
    bool canBeFree = true;
    LSLiveRange *analyzedLV = nullptr;

    pregManager.getAvailableRegs()->findRegisterCandiateWithAlignForward(
        i, bankAlign, false);

    // Check the following adjacent registers
    for (int k = i; k < i + requiredRows; k++) {
      if (activeGRF[k].activeInput.size() || tlr->getForbidden()[k]) {
        i = k;
        canBeFree = false;
        break;
      }

      if (activeGRF[k].activeLV.size()) {
        // There may be multiple variables take same register with different
        // offsets
        for (auto lr : activeGRF[k].activeLV) {
          if (lr == analyzedLV) // one LV may occupy multiple registers
          {
            continue;
          }

          analyzedLV = lr;

          if (!canBeSpilledLR(lr) || lr->getForbidden()[k]) {
            int startsregnum = 0;
            G4_VarBase *op = lr->getPhyReg(startsregnum);
            unsigned startregnum = op->asGreg()->getRegNum();

            canBeFree = false;
            i = startregnum + lr->getTopDcl()->getNumRows() -
                1; // Jump to k + rows - 1 to avoid unnecessory analysis.

            break;
          }

          int startsregnum = 0;
          G4_VarBase *op = lr->getPhyReg(startsregnum);
          int startregnum = op->asGreg()->getRegNum();
          unsigned effectGRFNum =
              startregnum > i
                  ? lr->getTopDcl()->getNumRows()
                  : lr->getTopDcl()->getNumRows() - (i - startregnum);
          lr->getLastRef(endIdx);
          lastIdxs += (endIdx - tStartIdx) * effectGRFNum;
          referenceCount += lr->getNumRefs();
        }

        if (!canBeFree) {
          break;
        }
      } else if (pregManager.getAvailableRegs()->isGRFAvailable(k) &&
                 !pregManager.getAvailableRegs()->isGRFBusy(k)) {
        lastIdxs += lastLexicalID - tStartIdx;
      } else // Reserved regsiters
      {
        i = k;
        canBeFree = false;
        break;
      }
    }

    if (canBeFree) {
      // Spill cost
      float currentSpillCost = (float)referenceCount / lastIdxs;

      if (currentSpillCost < spillCost) {
        startGRF = i;
        spillCost = currentSpillCost;
      }
    }

    lastIdxs = 1;
    referenceCount = 0;
  }

  return startGRF;
}

void globalLinearScan::freeSelectedRegistsers(
    int startGRF, LSLiveRange *tlr, std::list<LSLiveRange *> &spillLRs) {
  unsigned short requiredRows = tlr->getTopDcl()->getNumRows();
#ifdef DEBUG_VERBOSE_ON
  COUT_ERROR << "Required GRF size for spill: " << requiredRows << "\n";
#endif

  // Free registers.
  for (int k = startGRF; k < startGRF + requiredRows; k++) {
#ifdef DEBUG_VERBOSE_ON
    if (!activeGRF[k].activeLV.size()) {
      COUT_ERROR << "Pick free GRF for spill: "
                 << " GRF:" << k << "\n";
    }
#endif

    while (activeGRF[k].activeLV.size()) {
      LSLiveRange *lr = activeGRF[k].activeLV.front();

      G4_VarBase *op;
      int startsregnum = 0;
      op = lr->getPhyReg(startsregnum);
      unsigned startregnum = op->asGreg()->getRegNum();
      unsigned endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;

      vASSERT(startregnum <= (unsigned)k);
      vASSERT(lr->getTopDcl()->getRegFile() != G4_INPUT);

      // Free from the register buckect array
      for (unsigned s = startregnum; s <= endregnum; s++) {
        std::vector<LSLiveRange *>::iterator it = std::find(
            activeGRF[s].activeLV.begin(), activeGRF[s].activeLV.end(), lr);
        if (it != activeGRF[s].activeLV.end()) {
#ifdef DEBUG_VERBOSE_ON
          COUT_ERROR << "SPILL: Free activeGRF from : "
                     << (lr)->getTopDcl()->getName() << " GRF:" << s << "\n";
#endif
          activeGRF[s].activeLV.erase(it);
        }
      }

#ifdef DEBUG_VERBOSE_ON
      printLiveInterval(lr, false, builder);
#endif

      // Free the allocated register
      freeAllocedRegs(lr, true);

      // Record spilled live range
      if (std::find(spillLRs.begin(), spillLRs.end(), lr) == spillLRs.end()) {
        spillLRs.push_back(lr);
      }

      // Remove spilled live range from active list
      std::list<LSLiveRange *>::iterator activeListIter = active.begin();
      while (activeListIter != active.end()) {
        std::list<LSLiveRange *>::iterator nextIt = activeListIter;
        nextIt++;

        if ((*activeListIter) == lr) {
#ifdef DEBUG_VERBOSE_ON
          COUT_ERROR << "SPILL: Free active lr: "
                     << (*activeListIter)->getTopDcl()->getName() << "\n";
#endif
          active.erase(activeListIter);
          break;
        }
        activeListIter = nextIt;
      }
    }
  }
}

bool globalLinearScan::spillFromActiveList(LSLiveRange *tlr,
                                           std::list<LSLiveRange *> &spillLRs) {
  int startGRF = findSpillCandidate(tlr);

  if (startGRF == -1) {
#ifdef DEBUG_VERBOSE_ON
    printActives();
#endif
    return false;
  }

  freeSelectedRegistsers(startGRF, tlr, spillLRs);

  return true;
}

void globalLinearScan::expireGlobalRanges(unsigned int idx) {
  // active list is sorted in ascending order of starting index

  while (active.size() > 0) {
    unsigned int endIdx;
    LSLiveRange *lr = active.front();

    lr->getLastRef(endIdx);

    if (endIdx <= idx) {
      G4_VarBase *preg;
      int subregnumword, subregnum;

      preg = lr->getPhyReg(subregnumword);

      if (preg) {
        subregnum = LinearScanRA::convertSubRegOffFromWords(lr->getTopDcl(),
                                                            subregnumword);

        // Mark the RegVar object of dcl as assigned to physical register
        lr->getTopDcl()->getRegVar()->setPhyReg(preg, subregnum);
        lr->setAssigned(true);
      }

#ifdef DEBUG_VERBOSE_ON
      printLiveInterval(lr, false, builder);
#endif
      if (preg) {
        unsigned startregnum = preg->asGreg()->getRegNum();
        unsigned endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;
        for (unsigned i = startregnum; i <= endregnum; i++) {
          std::vector<LSLiveRange *>::iterator activeListIter =
              activeGRF[i].activeLV.begin();
          while (activeListIter != activeGRF[i].activeLV.end()) {
            std::vector<LSLiveRange *>::iterator nextIt = activeListIter;
            nextIt++;
            if ((*activeListIter) == lr) {
              activeGRF[i].activeLV.erase(activeListIter);
#ifdef DEBUG_VERBOSE_ON
              COUT_ERROR << "Remove range " << lr->getTopDcl()->getName()
                         << " from activeGRF: " << i << "\n";
#endif
              break;
            }
            activeListIter = nextIt;
          }
        }

        if (calleeSaveLR) {
          unsigned int startCallerSave =
              builder.kernel.stackCall.calleeSaveStart();
          unsigned int endCallerSave =
              startCallerSave + builder.kernel.stackCall.getNumCalleeSaveRegs();

          for (unsigned i = startregnum; i <= endregnum; i++) {
            if (i >= startCallerSave && i <= endCallerSave) {
              calleeSaveLR->addForbidden(i);
            }
          }
        }
      }

      // Free physical regs marked for this range
      freeAllocedRegs(lr, true);

      // Remove range from active list
      active.pop_front();
    } else {
      // As soon as we find first range that ends after ids break loop
      break;
    }
  }
}

void globalLinearScan::expireInputRanges(unsigned int global_idx) {
  while (inputIntervals.size() > 0) {
    LSInputLiveRange *lr = inputIntervals.front();
    unsigned int endIdx = lr->getLrEndIdx();

    if (endIdx <= global_idx) {
      unsigned int regnum =
          lr->getRegWordIdx() / builder.numEltPerGRF<Type_UW>();
      unsigned int subRegInWord =
          lr->getRegWordIdx() % builder.numEltPerGRF<Type_UW>();

      // Free physical regs marked for this range
      pregManager.freeRegs(regnum, subRegInWord, 1, endIdx);

#ifdef DEBUG_VERBOSE_ON
      COUT_ERROR << "Expiring input r" << regnum << "." << subRegInWord << "\n";
#endif

      // Remove range from inputIntervals list
      inputIntervals.pop_front();
      vASSERT(lr == activeGRF[regnum].activeInput.front());
      activeGRF[regnum].activeInput.erase(
          activeGRF[regnum].activeInput.begin());
    } else {
      // As soon as we find first range that ends after ids break loop
      break;
    }
  }
}

BankAlign globalLinearScan::getBankAlign(LSLiveRange *lr) {
  G4_Declare *dcl = lr->getTopDcl();
  BankAlign bankAlign =
      gra.isEvenAligned(dcl) ? BankAlign::Even : BankAlign::Either;

  return bankAlign;
}

bool globalLinearScan::allocateRegsLinearScan(LSLiveRange *lr,
                                              IR_Builder &builder) {
  int regnum, subregnum;
  unsigned int localRABound = 0;
  unsigned int instID;

  lr->getFirstRef(instID);
  // Let local RA allocate only those ranges that need < 10 GRFs
  // Larger ranges are not many and are best left to global RA
  // as it can make a better judgement by considering the
  // spill cost.
  int nrows = 0;
  int size = lr->getSizeInWords(builder);
  G4_Declare *dcl = lr->getTopDcl();
  G4_SubReg_Align subalign = gra.getSubRegAlign(dcl);
  localRABound = numRegLRA - 1;

  BankAlign bankAlign = getBankAlign(lr);
  nrows = pregManager.findFreeRegs(size, bankAlign, subalign, regnum, subregnum,
                                   startGRFReg, localRABound, instID,
                                   lr->getForbidden());

  if (nrows) {
#ifdef DEBUG_VERBOSE_ON
    COUT_ERROR << lr->getTopDcl()->getName() << ":r" << regnum
               << "  BANK: " << (int)bankAlign << "\n";
#endif
    lr->setPhyReg(builder.phyregpool.getGreg(regnum), subregnum);
    if (!builder.getOptions()->getOption(vISA_LSFristFit)) {
      startGRFReg = (startGRFReg + nrows) % localRABound;
    } else {
      vASSERT(startGRFReg == 0);
    }

    return true;
  } else if (!builder.getOptions()->getOption(vISA_LSFristFit)) {
    startGRFReg = 0;
    nrows = pregManager.findFreeRegs(size, bankAlign, subalign, regnum,
                                     subregnum, startGRFReg, localRABound,
                                     instID, lr->getForbidden());
    if (nrows) {
#ifdef DEBUG_VERBOSE_ON
      COUT_ERROR << lr->getTopDcl()->getName() << ":r" << regnum
                 << "  BANK: " << (int)bankAlign << "\n";
#endif
      lr->setPhyReg(builder.phyregpool.getGreg(regnum), subregnum);
      startGRFReg = (startGRFReg + nrows) % localRABound;
      return true;
    }
  }
#ifdef DEBUG_VERBOSE_ON
  COUT_ERROR << lr->getTopDcl()->getName() << ": failed to allocate"
             << "\n";
#endif

  return false;
}

bool PhyRegsLocalRA::findFreeMultipleRegsForward(int regIdx, BankAlign align,
                                                 int &regnum, int nrows,
                                                 int lastRowSize, int endReg,
                                                 int instID,
                                                 const bool *forbidden) {
  int foundItem = 0;
  int startReg = 0;
  int i = regIdx;
  int grfRows = 0;
  bool multiSteps = nrows > 1;

  if (lastRowSize % builder.numEltPerGRF<Type_UW>() == 0) {
    grfRows = nrows;
  } else {
    grfRows = nrows - 1;
  }

  findRegisterCandiateWithAlignForward(i, align, multiSteps);

  startReg = i;
  while (i <= endReg + nrows - 1) {
    if (isGRFAvailable(i) && !forbidden[i] && regBusyVector[i] == 0) {
      foundItem++;
    } else if (foundItem < grfRows) {
      foundItem = 0;
      i++;
      findRegisterCandiateWithAlignForward(i, align, multiSteps);
      startReg = i;
      continue;
    }

    if (foundItem == grfRows) {
      if (lastRowSize % builder.numEltPerGRF<Type_UW>() == 0) {
        regnum = startReg;
        return true;
      } else {
        if (i + 1 <= endReg + nrows - 1 && isGRFAvailable(i + 1) &&
            !forbidden[i + 1] && (isWordBusy(i + 1, 0, lastRowSize) == false)) {
          regnum = startReg;
          return true;
        } else {
          foundItem = 0;
          i++;
          findRegisterCandiateWithAlignForward(i, align, multiSteps);
          startReg = i;
          continue;
        }
      }
    }

    i++;
  }

  return false;
}

bool PhyRegsLocalRA::findFreeSingleReg(int regIdx, int size, BankAlign align,
                                       G4_SubReg_Align subalign, int &regnum,
                                       int &subregnum, int endReg,
                                       const bool *forbidden) {
  int i = regIdx;
  bool found = false;

  while (!found) {
    if (i > endReg) //<= works
      break;

    // Align GRF
    if ((align == BankAlign::Even) && (i % 2 != 0)) {
      i++;
      continue;
    } else if ((align == BankAlign::Odd) && (i % 2 == 0)) {
      i++;
      continue;
    } else if ((align == BankAlign::Even2GRF) && ((i % 4 >= 2))) {
      i++;
      continue;
    } else if ((align == BankAlign::Odd2GRF) && ((i % 4 < 2))) {
      i++;
      continue;
    }

    if (isGRFAvailable(i, 1) && !forbidden[i]) {
      found = findFreeSingleReg(i, subalign, regnum, subregnum, size);
      if (found) {
        return true;
      }
    }
    i++;
  }

  return false;
}

int PhyRegsManager::findFreeRegs(int size, BankAlign align,
                                 G4_SubReg_Align subalign, int &regnum,
                                 int &subregnum, int startRegNum, int endRegNum,
                                 unsigned int instID, const bool *forbidden) {
  int nrows = 0;
  int lastRowSize = 0;
  LocalRA::getRowInfo(size, nrows, lastRowSize, builder);

  int startReg = startRegNum;
  int endReg = endRegNum - nrows + 1;

  bool found = false;

  if (size >= (int)builder.numEltPerGRF<Type_UW>()) {
    found = availableRegs.findFreeMultipleRegsForward(
        startReg, align, regnum, nrows, lastRowSize, endReg, instID, forbidden);
    if (found) {
      subregnum = 0;
      if (size % builder.numEltPerGRF<Type_UW>() == 0) {
        availableRegs.setGRFBusy(regnum, nrows);
      } else {
        availableRegs.setGRFBusy(regnum, nrows - 1);
        availableRegs.setWordBusy(regnum + nrows - 1, 0, lastRowSize);
      }
    }
  } else {
    found = availableRegs.findFreeSingleReg(
        startReg, size, align, subalign, regnum, subregnum, endReg, forbidden);
    if (found) {
      availableRegs.setWordBusy(regnum, subregnum, size);
    }
  }

  if (found) {
    return nrows;
  }

  return 0;
}
