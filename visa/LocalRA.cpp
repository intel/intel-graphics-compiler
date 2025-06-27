/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LocalRA.h"
#include "Assertions.h"
#include "DebugInfo.h"
#include "common.h"

#include <fstream>
#include <tuple>

using namespace vISA;

// #define _DEBUG
// #define cout cerr

#define NUM_PREGS_FOR_UNIQUE_ASSIGN 50

#define SPLIT_REF_CNT_THRESHOLD 3
#define SPLIT_USE_CNT_THRESHOLD 2
#define SPLIT_USE_DISTANCE_THRESHOLD 100

void getForbiddenGRFs(std::vector<unsigned int> &regNum, G4_Kernel &kernel,
                      unsigned stackCallRegSize, unsigned reserveSpillSize,
                      unsigned reservedRegNum);
void getCallerSaveGRF(std::vector<unsigned int> &regNum, G4_Kernel *kernel);

LocalRA::LocalRA(BankConflictPass &b, GlobalRA &g)
    : kernel(g.kernel), builder(g.builder), bc(b), gra(g) {}

BankAlign LocalRA::getBankAlignForUniqueAssign(G4_Declare *dcl) {
  // FIXME: this code is rather suspicious (we return even alignment
  // for first_half_odd???), should revisit
  switch (gra.getBankConflict(dcl)) {
  case BANK_CONFLICT_FIRST_HALF_EVEN:
  case BANK_CONFLICT_FIRST_HALF_ODD:
    return builder.oneGRFBankDivision() ? BankAlign::Even : BankAlign::Even2GRF;
  case BANK_CONFLICT_SECOND_HALF_EVEN:
  case BANK_CONFLICT_SECOND_HALF_ODD:
    return builder.oneGRFBankDivision() ? BankAlign::Odd : BankAlign::Odd2GRF;
  default:
    return BankAlign::Either;
  }
}

// returns true if kernel contains a back edge
// call/return edge may also be considered as a back edge depending on layout.
bool LocalRA::hasBackEdge() {
  for (auto curBB : kernel.fg) {

    vISA_ASSERT(curBB->size() > 0 || curBB->Succs.size() > 1,
                "Error in detecting back-edge: Basic block found with no inst "
                "list and multiple successors.");

    if (curBB->size() > 0) {
      for (auto succ : curBB->Succs) {
        vISA_ASSERT(succ->size() > 0,
                    "Error in detecting back-edge: Destination basic block of "
                    "a jmp has no instructions.");
        if (curBB->getId() >= succ->getId()) {
          return true;
        }
      }
    }
  }
  return false;
}

void LocalRA::getRowInfo(int size, int &nrows, int &lastRowSize,
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

void LocalRA::specialAlign() {
  if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D &&
      kernel.fg.size() > 2) {
    if (gra.use4GRFAlign) {
      gra.augAlign();
    } else if ((GlobalRA::useGenericAugAlign(kernel.getPlatformGeneration()) &&
         kernel.getSimdSize() >= kernel.numEltPerGRF<Type_UD>()) ||
        (!GlobalRA::useGenericAugAlign(kernel.getPlatformGeneration()) &&
         kernel.getSimdSize() > kernel.numEltPerGRF<Type_UD>())) {
      // Set alignment of all GRF candidates
      // to 2GRF except for NoMask variables
#ifdef DEBUG_VERBOSE_ON
      DEBUG_VERBOSE("Updating alignment to Even for GRF candidates"
                    << "\n");
#endif

      gra.augAlign();
    }
    gra.updateSubRegAlignment(builder.getGRFAlign());
    // Since we are piggy backing on mask field of G4_Declare,
    // we need to make sure we reset it before going further.
    resetMasks();
  }

  return;
}

/*
 *  Pre-localRA analsyis including:
 *  1) Mark reference to create live ranges, 2) alignment handling, 3) Avialable
 * register calcuating and marking
 */
void LocalRA::preLocalRAAnalysis() {
  unsigned int numRowsEOT = 0;
  bool lifetimeOpFound = false;

  int numGRF = kernel.getNumRegTotal();

  // Clear LocalLiveRange* computed preRA
  gra.clearStaleLiveRanges();

  // Mark references made to decls to sieve local from global ranges
  markReferences(numRowsEOT, lifetimeOpFound);

  // Mark those physical registers unavailable that are declared with Output
  // attribute
  blockOutputPhyRegs();

  if (lifetimeOpFound == true) {
    // Check whether pseudo_kill/lifetime.end are only references
    // for their respective variables. Remove them if so. Doing this
    // helps reduce number of variables in symbol table increasing
    // changes of skipping global RA.
    removeUnrequiredLifetimeOps();
  }

  unsigned int numRowsReserved = numRowsEOT;

  // Remove unreferenced dcls
  gra.removeUnreferencedDcls();

  if (gra.useFastRA || gra.useHybridRAwithSpill) {
    unsigned reserveSpillSize = 0;
    unsigned int spillRegSize = 0;
    unsigned int indrSpillRegSize = 0;
    gra.determineSpillRegSize(spillRegSize, indrSpillRegSize);

    reserveSpillSize = spillRegSize + indrSpillRegSize;
    if (reserveSpillSize >= kernel.stackCall.getNumCalleeSaveRegs()) {
      gra.useFastRA = false;
      gra.useHybridRAwithSpill = false;
      numRegLRA = numGRF - numRowsReserved;
    } else {
      numRegLRA = numGRF - numRowsReserved - reserveSpillSize -
                  4; // spill Header, scratch offset, a0.2, WA
    }
  } else {
    numRegLRA = numGRF - numRowsReserved;
  }
  bool isStackCall =
      (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc());
  unsigned int stackCallRegSize = isStackCall ? kernel.stackCall.numReservedABIGRF() : 0;
  unsigned int reservedGRFNum =
      builder.getOptions()->getuInt32Option(vISA_ReservedGRFNum);

  if (isStackCall || reservedGRFNum || builder.getOption(vISA_Debug)) {
    std::vector<unsigned int> forbiddenRegs;
    getForbiddenGRFs(forbiddenRegs, kernel, stackCallRegSize, 0,
                     reservedGRFNum);
    for (unsigned int i = 0, size = forbiddenRegs.size(); i < size; i++) {
      unsigned int regNum = forbiddenRegs[i];
      pregs->setGRFUnavailable(regNum);
    }

    // FIXME: this will break if # of GRF is not 128.
    if (isStackCall) {
      // Set r60 to r99 as unavailable for local RA since these registers are
      // callee saved Local RA will use only caller save registers for
      // allocation.
      std::vector<unsigned int> callerSaveRegs;
      getCallerSaveGRF(callerSaveRegs, &kernel);
      for (unsigned int i = 0, size = callerSaveRegs.size(); i < size; i++) {
        unsigned int regNum = callerSaveRegs[i];
        pregs->setGRFUnavailable(regNum);
      }

      numRegLRA = (kernel.getNumRegTotal() - 3) - numRowsReserved;
    }

    if (builder.getOption(vISA_Debug)) {
      // Since LocalRA is not undone when debug info generation is required,
      // for keeping compile time low, we allow fewer physical registers
      // as assignable candidates. Without this, we could run in to a
      // situation where very few physical registers are available for GRA
      // and it is unable to assign registers even with spilling.
#define USABLE_GRFS_WITH_DEBUG_INFO 80
      int maxSendReg = 0;
      for (auto bb : kernel.fg) {
        for (auto inst : *bb) {
          if (inst->isSend() || inst->isSplitSend()) {
            maxSendReg = ((int)inst->getMsgDesc()->getDstLenRegs() > maxSendReg)
                             ? ((int)inst->getMsgDesc()->getDstLenRegs())
                             : maxSendReg;
            maxSendReg =
                ((int)inst->getMsgDesc()->getSrc0LenRegs() > maxSendReg)
                    ? ((int)inst->getMsgDesc()->getSrc0LenRegs())
                    : maxSendReg;
            maxSendReg =
                ((int)inst->getMsgDesc()->getSrc1LenRegs() > maxSendReg)
                    ? ((int)inst->getMsgDesc()->getSrc1LenRegs())
                    : maxSendReg;
          }
        }
      }

      int maxRegsToUse = USABLE_GRFS_WITH_DEBUG_INFO;
      if (maxSendReg > (numGRF - USABLE_GRFS_WITH_DEBUG_INFO)) {
        maxRegsToUse = (numGRF - maxSendReg) - 10;
      }

      // Also check max size of addressed GRF
      unsigned int maxAddressedRows = 0;
      for (auto dcl : kernel.Declares) {
        if (dcl->getAddressed() && maxAddressedRows < dcl->getNumRows()) {
          maxAddressedRows = dcl->getNumRows();
        }
      }

      // Assume indirect operand of maxAddressedRows exists
      // on dst, src0, src1. This is overly conservative but
      // should work for general cases.
      if ((numGRF - maxRegsToUse) / 3 < (int)maxAddressedRows) {
        maxRegsToUse = (numGRF - (maxAddressedRows * 3));

        if (maxRegsToUse < 0)
          maxRegsToUse = 0;
      }

      for (int i = maxRegsToUse; i < numGRF; i++) {
        pregs->setGRFUnavailable(i);
      }
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

void LocalRA::trivialAssignRA(bool &needGlobalRA, bool threeSourceCandidate) {
  RA_TRACE(std::cout << "\t--trivial RA\n");

  needGlobalRA = assignUniqueRegisters(threeSourceCandidate,
                                       builder.lowHighBundle(), false);

  if (!needGlobalRA) {
    if (threeSourceCandidate) {
      kernel.setRAType(RA_Type::TRIVIAL_BC_RA);
    } else {
      kernel.setRAType(RA_Type::TRIVIAL_RA);
    }
  }

  return;
}

// Entry point to local RA
bool LocalRA::localRAPass(bool doRoundRobin, bool doSplitLLR) {
  bool needGlobalRA = true;
  PhyRegsLocalRA localPregs = *pregs;

  // Iterate over each BB and perform linear scan
  VISA_DEBUG_VERBOSE({
    localPregs.printBusyRegs();
    printAddressTakenDecls();
    printLocalRACandidates();
  });

  if (kernel.fg.funcInfoTable.empty() && !hasBackEdge()) {
    calculateInputIntervals();
  }

  VISA_DEBUG_VERBOSE(printInputLiveIntervals());
  for (auto curBB : kernel.fg) {
    PhyRegsManager pregManager(builder, localPregs, doBCR);
    std::vector<LocalLiveRange *> liveIntervals;

    PhyRegSummary *summary = gra.createPhyRegSummary();

    calculateLiveIntervals(curBB, liveIntervals);

    VISA_DEBUG_VERBOSE(printLocalLiveIntervals(curBB, liveIntervals));

    LinearScan ra(gra, liveIntervals, inputIntervals, pregManager, localPregs,
                  summary, numRegLRA, globalLRSize, doRoundRobin, doBCR,
                  highInternalConflict, doSplitLLR, kernel.getSimdSize());
    ra.run(curBB, builder, LLRUseMap);

    VISA_DEBUG_VERBOSE({
      std::cout << "BB" << curBB->getId() << "\n";
      summary->printBusyRegs();
    });

    // Tag summary of physical registers used by local RA
    gra.addBBLRASummary(curBB, summary);

    if (kernel.getOptions()->getOption(vISA_GenerateDebugInfo)) {
      updateDebugInfo(kernel, liveIntervals);
    }
  }

  if (unassignedRangeFound() == false) {
    needGlobalRA = false;
    VISA_DEBUG_VERBOSE(
        std::cout
        << "Skipping global RA because local RA allocated all live-ranges.\n");
  }

  VISA_DEBUG_VERBOSE(localRAOptReport());

  if (needGlobalRA == true &&
      !(kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc())) {
    // Check whether unique physical registers can be assigned
    // to global ranges.
    bool twoBanksAssign = false;

    if (builder.oneGRFBankDivision()) {
      twoBanksAssign =
          highInternalConflict || kernel.getSimdSize() == g4::SIMD16;
    } else {
      twoBanksAssign = highInternalConflict;
    }

    needGlobalRA = assignUniqueRegisters(
        doBCR, twoBanksAssign,
        gra.useHybridRAwithSpill && !doRoundRobin);
  }

  if (needGlobalRA && (doRoundRobin || gra.twoSrcBundleBCR)) {
    undoLocalRAAssignments(true);
  }

  if (needGlobalRA == false) {
    VISA_DEBUG(std::cout << "Allocated 100% GRF ranges without graph coloring."
                         << "\n");
  }

  return needGlobalRA;
}

bool LocalRA::localRA() {
  RA_TRACE(std::cout << "--local RA--\n");

  bool doRoundRobin = builder.getOption(vISA_LocalRARoundRobin);

  int numGRF = kernel.getNumRegTotal();
  PhyRegsLocalRA phyRegs(builder, numGRF);
  pregs = &phyRegs;

  globalLRSize = 0;
  bool reduceBCInTAandFF = false;
  bool needGlobalRA = true;

  doSplitLLR = (builder.getOption(vISA_SpiltLLR) && kernel.fg.size() == 1 &&
                kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D);

  preLocalRAAnalysis();

  bool reduceBCInRR = false;

  if (builder.getOption(vISA_LocalBankConflictReduction) &&
      (builder.hasBankCollision() || gra.twoSrcBundleBCR)) {
    reduceBCInRR = bc.setupBankConflictsForKernel(
        doRoundRobin, reduceBCInTAandFF, numRegLRA, highInternalConflict);
  }

  if (!kernel.fg.getHasStackCalls() && !kernel.fg.getIsStackCallFunc() &&
      !hasSplitInsts) {
    trivialAssignRA(needGlobalRA, reduceBCInTAandFF);
  }

  if (doRoundRobin) {
    doRoundRobin = countLiveIntervals();
  }

  if ((!doRoundRobin && reduceBCInTAandFF) ||
      (doRoundRobin && reduceBCInRR)) // To avoid the scheduling issue for send
  {
    doBCR = true;
  }
  // Local RA passes:
  // RR+BC --> FF+BC -->FF-->Hybrids
  // or
  // RR --> FF --> Hybrid
  if (doRoundRobin) {
    RA_TRACE(std::cout << "\t--round-robin " << (doBCR ? "BCR " : "")
                       << "RA\n");
    needGlobalRA = localRAPass(true, false);
    if (needGlobalRA == true) {
      doRoundRobin = false;
    }
  }

  if (!doRoundRobin) {
    if ((gra.forceBCR || gra.twoSrcBundleBCR) && doBCR) {
      RA_TRACE(std::cout << "\t--first-fit BCR RA\n");
      needGlobalRA = localRAPass(false, doSplitLLR);
    }

    if (needGlobalRA) {
      RA_TRACE(std::cout << "\t--first-fit RA\n");
      globalLRSize = 0;
      if (gra.useHybridRAwithSpill) {
        countLiveIntervals();
      } else {
        globalLRSize = 0;
      }
      specialAlign();
      needGlobalRA = localRAPass(false, doSplitLLR);
    }
  }

  if (needGlobalRA == false) {
    if (doRoundRobin) {
      kernel.setRAType(doBCR ? RA_Type::LOCAL_ROUND_ROBIN_BC_RA
                             : RA_Type::LOCAL_ROUND_ROBIN_RA);
    } else {
      kernel.setRAType(doBCR ? RA_Type::LOCAL_FIRST_FIT_BC_RA
                             : RA_Type::LOCAL_FIRST_FIT_RA);
    }
  }

  setLexicalID(true);

  return !needGlobalRA;
}

void LocalRA::resetMasks() {
  auto &dcls = kernel.Declares;

  for (auto dcl : dcls) {
    gra.setMask(dcl, {});
  }
}

unsigned int LocalRA::getLargestInputGRF() {
  unsigned int largestInput = 0;

  for (auto dcl : kernel.Declares) {
    // Find out the largest GRF regsiter occupied by input variable
    // In case overlap with reserved registers
    if ((dcl->getRegFile() == G4_INPUT || dcl->isLiveIn()) &&
        dcl->isOutput() == false && !(dcl->getRegVar()->isAreg()) &&
        dcl->getRegVar()->isPhyRegAssigned()) {
      G4_RegVar *var = dcl->getRegVar();
      unsigned int regNum = var->getPhyReg()->asGreg()->getRegNum();
      unsigned int regOff = var->getPhyRegOff();
      unsigned int largestGRF =
          (regNum * kernel.numEltPerGRF<Type_UW>() +
           (regOff * dcl->getElemSize()) / G4_WSIZE + dcl->getWordSize() - 1) /
          (kernel.numEltPerGRF<Type_UW>());
      largestInput = largestInput < largestGRF ? largestGRF : largestInput;
    }
  }

  return largestInput;
}

void LocalRA::blockOutputPhyRegs() {
  for (auto dcl : kernel.Declares) {
    if (dcl->isOutput() && dcl->isInput()) {
      // The live-range may never be referenced in the CFG so we need
      // this special pass to block physical registers corresponding
      // to those declares. This may be true for FC.
      pregs->markPhyRegs(dcl);
    }
  }
}

class isLifetimeCandidateOpCandidateForRemoval {
public:
  isLifetimeCandidateOpCandidateForRemoval(GlobalRA &g) : gra(g) {}

  GlobalRA &gra;

  bool operator()(G4_INST *inst) {
    if (inst->isPseudoKill() || inst->isLifeTimeEnd()) {
      G4_Declare *topdcl;

      if (inst->isPseudoKill()) {
        topdcl = GetTopDclFromRegRegion(inst->getDst());
      } else {
        topdcl = GetTopDclFromRegRegion(inst->getSrc(0));
      }

      if (gra.getNumRefs(topdcl) == 0 && (topdcl->getRegFile() == G4_GRF ||
                                          topdcl->getRegFile() == G4_INPUT)) {
        // Remove this lifetime op
        return true;
      }
    }

    return false;
  }
};

void LocalRA::removeUnrequiredLifetimeOps() {
  // Iterate over all instructions and inspect only
  // pseudo_kills/lifetime.end instructions. Remove
  // instructions that have no other useful instruction.

  for (BB_LIST_ITER bb_it = kernel.fg.begin(); bb_it != kernel.fg.end();
       bb_it++) {
    G4_BB *bb = (*bb_it);
    bb->erase(
        std::remove_if(bb->begin(), bb->end(),
                       isLifetimeCandidateOpCandidateForRemoval(this->gra)),
        bb->end());
  }
}

void PhyRegsLocalRA::findRegisterCandiateWithAlignForward(int &i,
                                                          BankAlign align,
                                                          bool evenAlign) {
  if ((align == BankAlign::Even) && (i % 2 != 0)) {
    i++;
  } else if ((align == BankAlign::Odd) && (i % 2 == 0)) {
    i++;
  } else if ((align == BankAlign::QuadGRF) && (i % 4 != 0)) {
    i += (4 - (i % 4));
  } else if (align == BankAlign::Even2GRF) {
    while ((i % 4 >= 2) || (evenAlign && (i % 2 != 0))) {
      i++;
    }
  } else if (align == BankAlign::Odd2GRF) {
    while ((i % 4 < 2) || (evenAlign && (i % 2 != 0))) {
      i++;
    }
  }
}

unsigned int PhyRegsLocalRA::get_bundle(unsigned int baseReg, int offset) {
  if (builder.has64bundleSize2GRFPerBank()) {
    return (((baseReg + offset) % 32) / 4);
  }
  if (builder.hasPartialInt64Support()) {
    return (((baseReg + offset) % 32) / 2);
  }
  if (builder.has64bundleSize()) {
    return (((baseReg + offset) % 16) / 2);
  }
  return (((baseReg + offset) % 64) / 4);
}

int PhyRegsLocalRA::findBundleConflictFreeRegister(
    int curReg, int endReg, unsigned short occupiedBundles, BankAlign align,
    bool evenAlign) {
  int i = curReg;
  while (occupiedBundles & (1 << get_bundle(i, 0)) ||
         occupiedBundles & (1 << get_bundle(i, 1))) {
    i++;
    findRegisterCandiateWithAlignForward(i, align, evenAlign);
    if (i > endReg) // Out of bound, drop the bundle conflict considration
    {
      i = curReg;
      break;
    }
  }

  return i;
}

void PhyRegsLocalRA::findRegisterCandiateWithAlignBackward(int &i,
                                                           BankAlign align,
                                                           bool evenAlign) {
  if ((align == BankAlign::Even) && (i % 2 != 0)) {
    i--;
  } else if ((align == BankAlign::Odd) && (i % 2 == 0)) {
    i--;
  } else if ((align == BankAlign::QuadGRF) && (i % 4 != 0)) {
    if (i > 0)
      i -= (i % 4);
    else
      i -= (4 - (-i % 4));
  } else if (align == BankAlign::Even2GRF) {
    while (i >= 0 && ((i % 4 >= 2) || (evenAlign && (i % 2 != 0)))) {
      i--;
    }
  } else if (align == BankAlign::Odd2GRF) {
    while (i >= 0 && ((i % 4 < 2) || (evenAlign && (i % 2 != 0)))) {
      i--;
    }
  }
}

inline static unsigned short getOccupiedBundle(IR_Builder &builder,
                                               GlobalRA &gra, G4_Declare *dcl,
                                               BankAlign &preferBank) {
  unsigned short occupiedBundles = 0;
  unsigned bundleNum = 0;
  unsigned int evenBankNum = 0;
  unsigned int oddBankNum = 0;

  if (!builder.hasDPAS() ||
      !builder.getOption(vISA_EnableDPASBundleConflictReduction)) {
    return 0;
  }

  for (const BundleConflict &conflict : gra.getBundleConflicts(dcl)) {
    LocalLiveRange *lr = gra.getLocalLR(conflict.dcl);
    if (lr == nullptr)
      continue;
    int subregnum;
    G4_VarBase *preg = lr->getPhyReg(subregnum);
    if (preg == NULL) {
      preg = lr->getTopDcl()->getRegVar()->getPhyReg();
    }

    if (preg != NULL) {
      int offset = conflict.offset;
      unsigned int reg = preg->asGreg()->getRegNum();
      unsigned int bank = gra.get_bank(reg, offset);
      if (bank) {
        oddBankNum++;
      } else {
        evenBankNum++;
      }
      unsigned int bundle = gra.get_bundle(reg, offset);
      unsigned int bundle1 = gra.get_bundle(reg, offset + 1);
      if (!(occupiedBundles & ((unsigned short)1 << bundle))) {
        bundleNum++;
      }
      occupiedBundles |= (unsigned short)1 << bundle;
      occupiedBundles |= (unsigned short)1 << bundle1;
    }
  }
  if (bundleNum > 12) {
    occupiedBundles = 0;
  }

  if (evenBankNum || oddBankNum) {
    preferBank = (evenBankNum < oddBankNum) ? BankAlign::Even : BankAlign::Odd;
  }

  return occupiedBundles;
}

bool LocalRA::assignUniqueRegisters(bool twoBanksRA, bool twoDirectionsAssign,
                                    bool hybridWithSpill) {
  // Iterate over all dcls and calculate number of rows
  // required if each unallocated dcl had its own physical
  // register. Check number of registers used up by local RA.
  // If unique assignments are possible, then assign and set
  // needGlobalRA to false.
  unsigned int numRows = 0;
  std::vector<G4_Declare *> unallocatedRanges;
  bool needGlobalRA = true;
  uint32_t nextEOTGRF = numRegLRA;
  std::unordered_set<unsigned int> emptyForbidden;

  unsigned int largestInput = getLargestInputGRF();
  if (((nextEOTGRF < (kernel.getNumRegTotal() - 16)) ||
       (nextEOTGRF < largestInput)) &&
      builder.hasEOTGRFBinding()) {
    // we can't guarantee unique assignments for all the EOT sources
    // punt to global RA
    return true;
  }

  // Identify global ranges not having an allocation
  for (auto dcl : kernel.Declares) {
    if (dcl->getAliasDeclare() == NULL && dcl->getRegFile() == G4_GRF &&
        dcl->getRegVar()->isPhyRegAssigned() == false) {
      auto dclLR = gra.getLocalLR(dcl);

      if (dclLR && dclLR->isEOT() && builder.hasEOTGRFBinding()) {
        // For EOT use r112 - r127
        // for simplicity we assign each EOT source unique GRFs
        G4_Greg *phyReg = builder.phyregpool.getGreg(nextEOTGRF);
        dcl->getRegVar()->setPhyReg(phyReg, 0);
        dclLR->setPhyReg(phyReg, 0);
        dclLR->setAssigned(true);
        // we have to make sure src0 and src1 of split send get different GRFs
        nextEOTGRF += dcl->getNumRows();
        if (kernel.getOption(vISA_GenerateDebugInfo)) {
          uint32_t start = 0, end = 0;
          for (auto rit = kernel.fg.rbegin(); rit != kernel.fg.rend(); rit++) {
            G4_BB *bb = (*rit);

            if (bb->size() > 0) {
              end = bb->back()->getVISAId();
              break;
            }
          }
          updateDebugInfo(kernel, dcl, start, end);
        }
      } else {
        numRows += dcl->getNumRows();
        if (hybridWithSpill) {
          if (dcl->isDoNotSpill()) {
            unallocatedRanges.push_back(dcl);
          }
        } else {
          unallocatedRanges.push_back(dcl);
        }
      }
    }
  }

  if (numRows < numRegLRA || hybridWithSpill) {
    // Get superset of registers used by local RA
    // in all basic blocks.
    PhyRegsManager phyRegMgr(builder, *pregs, twoBanksRA);

    if (!hybridWithSpill && kernel.getOption(vISA_Debug)) {
      // when doing trivial RA, if any kernel input registers
      // are unreferenced, they're overwritten by other
      // variables. when running in -O0 mode, which is usually
      // for debug info generation, mark those input
      // registers as busy so they're not overwritten.
      // this is a problem for trivial assignments because
      // live-range computation is not done when trivial RA
      // succeeds. all variables are expected to be available
      // throughout the program since they're trivial assignments.
      for (auto dcl : kernel.Declares) {
        if (!dcl->getRootDeclare())
          continue;
        if (!dcl->isInput())
          continue;

        if (gra.getNumRefs(dcl) == 0) {
          // this is an unreferenced input
          auto phyReg = dcl->getRegVar()->getPhyReg();
          if (!phyReg || !phyReg->isGreg())
            continue;

          phyRegMgr.getAvailableRegs()->markPhyRegs(dcl);
        }
      }
    }

    for (auto bb : kernel.fg) {
      PhyRegSummary *summary = gra.getBBLRASummary(bb);
      if (summary) {
        for (unsigned int i = 0; i < numRegLRA; i++) {
          if (summary->isGRFBusy(i)) {
            phyRegMgr.getAvailableRegs()->setGRFBusy(i, 1);
          }
        }
      }
    }

    needGlobalRA = hybridWithSpill;
    bool assignFromFront = true;
#ifdef DEBUG_VERBOSE_ON
    COUT_ERROR << "Trival RA: "
               << "\n";
#endif
    for (auto dcl : unallocatedRanges) {
      int regNum = 0;
      int subregNum = 0;
      int sizeInWords = dcl->getWordSize();
      int nrows = 0;
      BankAlign bankAlign = BankAlign::Either;
      if (twoBanksRA && gra.getBankConflict(dcl) != BANK_CONFLICT_NONE) {
        bankAlign = getBankAlignForUniqueAssign(dcl);

        if (bankAlign == BankAlign::Even || bankAlign == BankAlign::Even2GRF) {
          assignFromFront = true;
        }
        if (twoDirectionsAssign &&
            (bankAlign == BankAlign::Odd || bankAlign == BankAlign::Odd2GRF)) {
          assignFromFront = false;
        }
      }

      auto assignAlign = gra.isQuadAligned(dcl)   ? BankAlign::QuadGRF
                         : gra.isEvenAligned(dcl) ? BankAlign::Even
                                                  : bankAlign;
      G4_SubReg_Align subAlign =
          builder.GRFAlign() ? builder.getGRFAlign() : gra.getSubRegAlign(dcl);

      if (assignFromFront) {
        BankAlign preferBank = BankAlign::Either;
        unsigned short occupiedBundles =
            getOccupiedBundle(builder, gra, dcl, preferBank);
        nrows = phyRegMgr.findFreeRegs(
            sizeInWords, assignAlign, subAlign, regNum, subregNum, 0,
            numRegLRA - 1, occupiedBundles, 0, false, emptyForbidden);
      } else {
        nrows = phyRegMgr.findFreeRegs(sizeInWords, assignAlign, subAlign,
                                       regNum, subregNum, numRegLRA - 1, 0, 0,
                                       0, false, emptyForbidden);
      }

      if (nrows) {
        G4_Greg *phyReg = builder.phyregpool.getGreg(regNum);
        // adjust subreg num to type size
        if (dcl->getElemSize() == 1) {
          subregNum *= 2;
        } else {
          subregNum /= (dcl->getElemSize() / 2);
        }
        dcl->getRegVar()->setPhyReg(phyReg, subregNum);
      } else {
        needGlobalRA = true;
        break;
      }
      if (twoBanksRA && twoDirectionsAssign) {
        assignFromFront = !assignFromFront;
      }
    }

    if (needGlobalRA) {
      for (auto dcl : unallocatedRanges) {
        if (!hybridWithSpill) {
          dcl->getRegVar()->resetPhyReg();
        } else if (!dcl->isDoNotSpill()) {
          dcl->getRegVar()->resetPhyReg();
        }
      }
    } else {
      if (kernel.getOption(vISA_GenerateDebugInfo)) {
        uint32_t start = 0, end = UNMAPPABLE_VISA_INDEX;
        for (auto rit = kernel.fg.rbegin(); rit != kernel.fg.rend(); rit++) {
          G4_BB *bb = (*rit);

          if (bb->size() > 0) {
            for (auto inst : *bb) {
              if (inst->getVISAId() != UNMAPPABLE_VISA_INDEX) {
                end = bb->back()->getVISAId();
                break;
              }
            }
            if (end != UNMAPPABLE_VISA_INDEX)
              break;
          }
        }

        for (auto dcl : unallocatedRanges) {
          updateDebugInfo(kernel, dcl, start, end);
        }

        // unallocatedRanges doesnt contain input dcls
        for (auto dcl : kernel.Declares) {
          if (dcl->isInput()) {
            // if an input is never referenced in the program, its
            // GRF storage may be overwritten in -O2 mode. only in
            // presence of -debug switch does compiler preserve
            // kernel inputs registers even when unreferenced with
            // trivial RA.
            if (gra.getNumRefs(dcl) > 0 || kernel.getOption(vISA_Debug))
              updateDebugInfo(kernel, dcl, start, end);
          }
        }
      }
    }

  } else {
    needGlobalRA = true;
  }
#ifdef DEBUG_VERBOSE_ON
  COUT_ERROR << "\n";
#endif

  return needGlobalRA;
}

void GlobalRA::removeUnreferencedDcls() {
  // Iterate over all dcls and remove those with 0
  // ref count and not addressed. This is done only for
  // GRF dcls.

  // Propagate top dcl info to aliases
  for (auto dcl : kernel.Declares) {
    if (dcl->getAliasDeclare() != nullptr) {
      setNumRefs(dcl, getNumRefs(dcl->getAliasDeclare()));
    }
  }

  auto isUnrefDcl = [this](G4_Declare *dcl) {
    return (dcl->getRegFile() == G4_GRF || dcl->getRegFile() == G4_INPUT) &&
           getNumRefs(dcl) == 0 &&
           dcl->getRegVar()->isPhyRegAssigned() == false &&
           dcl != kernel.fg.builder->getBuiltinR0() &&
           dcl != kernel.fg.builder->getSpillSurfaceOffset() &&
           (!kernel.fg.builder->getOptions()->getuInt32Option(vISA_CodePatch) ||
            dcl->getAliasDeclare() != kernel.fg.builder->getInputR1());
  };

  kernel.Declares.erase(std::remove_if(kernel.Declares.begin(),
                                       kernel.Declares.end(), isUnrefDcl),
                        kernel.Declares.end());
}

bool LocalRA::unassignedRangeFound() {
  bool unassignedRangeFound = false;
  for (auto dcl : kernel.Declares) {
    if (dcl->getAliasDeclare() == NULL && dcl->getRegFile() == G4_GRF &&
        dcl->getRegVar()->isPhyRegAssigned() == false) {
      unassignedRangeFound = true;
      break;
    }
  }

  return unassignedRangeFound;
}

// Update numRegsUsed to max physical register used based on summary
void LocalRA::updateRegUsage(PhyRegSummary *summary,
                             unsigned int &numRegsUsed) {
  for (uint32_t i = 0, numGRF = summary->getNumGRF(); i < numGRF; i++) {
    if (numRegsUsed < i && summary->isGRFBusy(i) == true) {
      numRegsUsed = i;
    }
  }
}

// Emit out localRA opt report
void LocalRA::localRAOptReport() {
  unsigned int totalRanges = 0, localRanges = 0;

  for (auto dcl : kernel.Declares) {
    auto dcl_it_lr = gra.getLocalLR(dcl);
    if (dcl_it_lr && dcl_it_lr->isLiveRangeLocal() &&
        dcl_it_lr->isGRFRegAssigned()) {
      localRanges++;
    }

    if ((dcl->getRegFile() == G4_GRF || dcl->getRegFile() == G4_INPUT) &&
        dcl->getAliasDeclare() == NULL) {
      totalRanges++;
    }
  }

  VISA_DEBUG({
    std::cout << "\n";
    std::cout << "Total GRF ranges: " << totalRanges << "\n";
    std::cout << "GRF ranges allocated by local RA: " << localRanges << "\n";
    if (totalRanges) {
      std::cout << (int)(localRanges * 100 / totalRanges)
                << "% allocated by local RA\n\n";
    }
  });
}

// Given a src/dst reg region in opnd, traverse to its base and
// return the declaration. Resolve any aliases and return the
// topmost declaration.
// opnd->topdcl should give same result for most cases. But for
// spill code, if an address register spills then opnd->topdcl does
// not return expected dcl for the fill.
G4_Declare *GetTopDclFromRegRegion(G4_Operand *opnd) {
  G4_Declare *dcl = nullptr;

  vISA_ASSERT(opnd->isRegRegion(),
              "Operand is not a register region so cannot have a top dcl");
  G4_VarBase *base = opnd->getBase();

  if (base && base->isRegVar()) {
    dcl = base->asRegVar()->getDeclare()->getRootDeclare();
  }

  return dcl;
}

unsigned int LocalRA::convertSubRegOffFromWords(G4_Declare *dcl,
                                                int subregnuminwords) {
  // Return subreg offset in units of dcl's element size.
  // Input is subregnum in word units.
  unsigned int subregnum;

  subregnum = (subregnuminwords * 2) / dcl->getElemSize();

  return subregnum;
}

unsigned int LocalRA::convertSubRegOffToWords(G4_Declare *dcl, int subregnum) {
  // Return subreg offset in units of words from dcl's element size
  unsigned int subregnuminwords;

  subregnuminwords = subregnum * dcl->getElemSize() / 2;

  return subregnuminwords;
}

void LocalRA::undoLocalRAAssignments(bool clearInterval) {
  VISA_DEBUG(std::cout << "Undoing local RA assignments\n");

  // Undo all assignments made by local RA
  for (auto dcl : kernel.Declares) {
    LocalLiveRange *lr = gra.getLocalLR(dcl);
    if (lr != NULL) {
      if (lr->getAssigned() == true) {
        // Undo the assignment
        lr->setAssigned(false);
        lr->getTopDcl()->getRegVar()->resetPhyReg();

        if (kernel.getOption(vISA_GenerateDebugInfo)) {
          kernel.getKernelDebugInfo()
              ->getLiveIntervalInfo(lr->getTopDcl(), false)
              ->clearLiveIntervals();
        }
      }
      if (clearInterval) {
        if (lr->isLiveRangeLocal()) {
          lr->setFirstRef(NULL, 0);
        }
        if (lr->getTopDcl()->getRegFile() == G4_INPUT) {
          lr->setLastRef(NULL, 0);
        }
      }
    }
  }

  // Delete summary information stored with each basic block
  gra.clearBBLRASummaries();
}

LocalLiveRange *GlobalRA::GetOrCreateLocalLiveRange(G4_Declare *topdcl) {
  LocalLiveRange *lr = getLocalLR(topdcl);

  // Check topdcl of operand and setup a new live range if required
  if (!lr) {
    localLiveRanges.push_back(LocalLiveRange(builder));
    lr = &localLiveRanges.back();
    setLocalLR(topdcl, lr);
  }

  vISA_ASSERT(lr != NULL, "Local LR could not be created");
  return lr;
}

void LocalRA::markReferencesInOpnd(G4_Operand *opnd, bool isEOT,
                                   INST_LIST_ITER inst_it, unsigned int pos) {
  G4_Declare *topdcl = NULL;

  if ((opnd->isSrcRegRegion() || opnd->isDstRegRegion())) {
    topdcl = GetTopDclFromRegRegion(opnd);

    if (topdcl &&
        (topdcl->getRegFile() == G4_GRF || topdcl->getRegFile() == G4_INPUT)) {
      // Handle GRF here
      vISA_ASSERT(topdcl->getAliasDeclare() == NULL, "Not topdcl");
      LocalLiveRange *lr = gra.GetOrCreateLocalLiveRange(topdcl);
      lr->recordRef(curBB);

      if (doSplitLLR && opnd->isSrcRegRegion()) {
        LLR_USE_MAP_ITER useMapIter;
        useMapIter = LLRUseMap.find(lr);
        if (useMapIter == LLRUseMap.end()) {
          std::vector<std::pair<INST_LIST_ITER, unsigned int>> useList;
          useList.push_back(make_pair(inst_it, pos));
          LLRUseMap.insert(make_pair(lr, useList));
        } else {
          (*useMapIter).second.push_back(make_pair(inst_it, pos));
        }
      }

      if (topdcl->getRegVar() && topdcl->getRegVar()->isPhyRegAssigned() &&
          topdcl->getRegVar()->getPhyReg()->isGreg()) {
        pregs->markPhyRegs(topdcl);
      }

      if (isEOT) {
        lr->markEOT();
      }

      gra.recordRef(topdcl);

      if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D &&
          opnd->isDstRegRegion() && !topdcl->getAddressed()) {
        if (opnd->getInst()->isWriteEnableInst() == false) {
        } else {
          gra.setAugmentationMask(topdcl, AugmentationMasks::NonDefault);
        }
      }
    } else {
      // Handle register other than GRF
    }
  } else if (opnd->isAddrExp()) {
    G4_AddrExp *addrExp = opnd->asAddrExp();

    topdcl = addrExp->getRegVar()->getDeclare();
    while (topdcl->getAliasDeclare() != NULL)
      topdcl = topdcl->getAliasDeclare();

    vISA_ASSERT(topdcl != NULL, "Top dcl was null for addr exp opnd");

    LocalLiveRange *lr = gra.GetOrCreateLocalLiveRange(topdcl);
    lr->recordRef(curBB);
    lr->markIndirectRef();

    if (topdcl->getRegVar() && topdcl->getRegVar()->isPhyRegAssigned() &&
        topdcl->getRegVar()->getPhyReg()->isGreg()) {
      pregs->markPhyRegs(topdcl);
    }

    gra.recordRef(topdcl);
  }
}

void LocalRA::markReferencesInInst(INST_LIST_ITER inst_it) {
  auto inst = (*inst_it);

  if (inst->getNumDst() > 0) {
    // Scan dst
    G4_Operand *dst = inst->getDst();

    if (dst) {
      markReferencesInOpnd(dst, false, inst_it, 0);
    }
  }

  // Scan srcs
  for (int i = 0, nSrcs = inst->getNumSrc(); i < nSrcs; i++) {
    G4_Operand *src = inst->getSrc(i);

    if (src) {
      markReferencesInOpnd(src, inst->isEOT(), inst_it, i);
    }
  }
}

void LocalRA::setLexicalID(bool includePseudo) {
  unsigned int id = 0;
  for (auto bb : kernel.fg) {
    for (auto curInst : *bb) {
      if ((!includePseudo) &&
          (curInst->isPseudoKill() || curInst->isLifeTimeEnd())) {
        curInst->setLexicalId(id);
      } else {
        curInst->setLexicalId(id++);
      }
    }
  }
}

void LocalRA::addOutOfBoundForbidden(G4_Declare *dcl, G4_Operand *opnd) {
  LocalLiveRange *lr = gra.getLocalLR(dcl);
  if (!lr) {
    return;
  }

  int opndEndGRF = opnd->getLinearizedEnd() / builder.numEltPerGRF<Type_UB>();
  int dclEndGRF = (dcl->getByteSize() - 1) / builder.numEltPerGRF<Type_UB>();
  if (opndEndGRF > dclEndGRF) {
    for (int i = 0; i < (opndEndGRF - dclEndGRF); i++) {
      lr->addForbidden(kernel.getNumRegTotal() - i + 1);
    }
  }

  return;
}

void LocalRA::markSpecialForbidden(INST_LIST_ITER inst_it) {
  G4_INST *inst = (*inst_it);

  if (!inst->isSend()) {
    return;
  }

  if (!inst->asSendInst()->isSVMScatterRW() ||
      inst->getExecSize() >= g4::SIMD8) {
    return;
  }

  G4_DstRegRegion *dst = inst->getDst();
  if (dst && !dst->isNullReg()) {
    G4_Declare *dcl = inst->getDst()->getTopDcl();
    if (dcl) {
      dcl = dcl->getRootDeclare();
      addOutOfBoundForbidden(dcl, dst);
    }
  }

  for (unsigned i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
    G4_Operand *src = inst->getSrc(i);
    if (src) {
      G4_Declare *dcl = src->getTopDcl();
      if (dcl) {
        dcl = dcl->getRootDeclare();
        addOutOfBoundForbidden(dcl, src);
      }
    }
  }

  return;
}

void LocalRA::markReferences(unsigned int &numRowsEOT, bool &lifetimeOpFound) {
  unsigned int id = 0;
  // Iterate over all BBs
  for (auto _curBB : kernel.fg) {
    curBB = _curBB;
    // Iterate over all insts
    for (INST_LIST_ITER inst_it = curBB->begin(), inst_end = curBB->end();
         inst_it != inst_end; ++inst_it) {
      G4_INST *curInst = (*inst_it);

      if (curInst->isPseudoKill() || curInst->isLifeTimeEnd()) {
        curInst->setLexicalId(id);
        lifetimeOpFound = true;
        if (curInst->isLifeTimeEnd()) {
          markReferencesInInst(inst_it);
        }
        continue;
      }

      if (curInst->isSplitIntrinsic())
        hasSplitInsts = true;

      // set lexical ID
      curInst->setLexicalId(id++);

      if (curInst->isEOT() && kernel.fg.builder->hasEOTGRFBinding()) {
        numRowsEOT += curInst->getSrc(0)->getTopDcl()->getNumRows();

        if (curInst->isSplitSend() && !curInst->getSrc(1)->isNullReg()) {
          // both src0 and src1 have to be >=r112
          numRowsEOT += curInst->getSrc(1)->getTopDcl()->getNumRows();
        }
      }

      markReferencesInInst(inst_it);
      markSpecialForbidden(inst_it);
    }
  }
}

// Function to calculate input register live intervals,
// which are now tracked at word granularity because
// there may be overlap between two different input variables.
void LocalRA::calculateInputIntervals() {
  setLexicalID(false);

  int numGRF = kernel.getNumRegTotal();
  std::vector<uint32_t> inputRegLastRef(numGRF * kernel.numEltPerGRF<Type_UW>(),
                                        UINT_MAX);

  for (BB_LIST_RITER bb_it = kernel.fg.rbegin(), bb_rend = kernel.fg.rend();
       bb_it != bb_rend; bb_it++) {
    G4_BB *bb = (*bb_it);

    for (INST_LIST_RITER inst_it = bb->rbegin(), inst_rend = bb->rend();
         inst_it != inst_rend; inst_it++) {
      G4_INST *curInst = (*inst_it);

      G4_Declare *topdcl = NULL;
      LocalLiveRange *lr = NULL;

      // scan dst operand (may be unnecessary but added for safety)
      if (curInst->getDst() != NULL) {
        // Scan dst
        G4_DstRegRegion *dst = curInst->getDst();

        topdcl = GetTopDclFromRegRegion(dst);

        if (topdcl && (lr = gra.getLocalLR(topdcl))) {
          if (topdcl->getRegFile() == G4_INPUT && !(dst->isAreg()) &&
              topdcl->isOutput() == false && lr->hasIndirectAccess() == false &&
              !builder.isPreDefArg(topdcl)) {
            unsigned int lastRef = 0;

            vISA_ASSERT(lr->isGRFRegAssigned(),
                        "Input variable has no pre-assigned physical register");

            if (lr->getLastRef(lastRef) == NULL) {
              unsigned int curInstId = curInst->getLexicalId();
              lr->setLastRef(curInst, curInstId);

              G4_RegVar *var = topdcl->getRegVar();
              unsigned int regNum = var->getPhyReg()->asGreg()->getRegNum();
              unsigned int regOff = var->getPhyRegOff();
              unsigned int idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                                 (regOff * topdcl->getElemSize()) / G4_WSIZE +
                                 topdcl->getWordSize() - 1;

              unsigned int numWords = topdcl->getWordSize();
              for (int i = numWords - 1; i >= 0; --i, --idx) {
                if (inputRegLastRef[idx] == UINT_MAX &&
                    // Check for registers that are marked as forbidden,
                    // e.g., BuiltinR0 and those reserved for stack call
                    pregs->isGRFAvailable(idx /
                                          kernel.numEltPerGRF<Type_UW>())) {
                  inputRegLastRef[idx] = curInstId;
                  inputIntervals.push_front(InputLiveRange(idx, curInstId));
                  if (kernel.getOptions()->getOption(vISA_GenerateDebugInfo)) {
                    updateDebugInfo(kernel, topdcl, 0, curInst->getVISAId());
                  }
                }
              }
            }
          }
        }
      }

      // Scan src operands
      for (int i = 0, nSrcs = curInst->getNumSrc(); i < nSrcs; i++) {
        G4_Operand *src = curInst->getSrc(i);

        if (src && src->getTopDcl()) {
          topdcl = GetTopDclFromRegRegion(src);

          if (topdcl && (lr = gra.getLocalLR(topdcl))) {
            // Check whether it is input
            if ((topdcl->getRegFile() == G4_INPUT || topdcl->isLiveIn()) &&
                !(src->isAreg()) && topdcl->isOutput() == false &&
                lr->hasIndirectAccess() == false &&
                !builder.isPreDefArg(topdcl)) {
              unsigned int lastRef = 0;

              vISA_ASSERT(
                  lr->isGRFRegAssigned(),
                  "Input variable has no pre-assigned physical register");

              if (lr->getLastRef(lastRef) == NULL) {
                unsigned int curInstId = curInst->getLexicalId();
                lr->setLastRef(curInst, curInstId);

                G4_RegVar *var = topdcl->getRegVar();
                unsigned int regNum = var->getPhyReg()->asGreg()->getRegNum();
                unsigned int regOff = var->getPhyRegOff();
                unsigned int idx =
                    regNum * kernel.numEltPerGRF<Type_UW>() +
                    (regOff * TypeSize(topdcl->getElemType())) / G4_WSIZE +
                    topdcl->getWordSize() - 1;

                unsigned int numWords = topdcl->getWordSize();
                for (int i = numWords - 1; i >= 0; --i, --idx) {
                  if (inputRegLastRef[idx] == UINT_MAX &&
                      // Check for registers that are marked as forbidden,
                      // e.g., BuiltinR0 and those reserved for stack call
                      pregs->isGRFAvailable(idx /
                                            kernel.numEltPerGRF<Type_UW>())) {
                    inputRegLastRef[idx] = curInstId;
                    if (builder.avoidDstSrcOverlap() &&
                        curInst->getDst() != NULL &&
                        hasDstSrcOverlapPotential(curInst->getDst(),
                                                  src->asSrcRegRegion())) {
                      inputIntervals.push_front(
                          InputLiveRange(idx, curInstId + 1));
                    } else {
                      inputIntervals.push_front(InputLiveRange(idx, curInstId));
                    }
                    if (kernel.getOptions()->getOption(
                            vISA_GenerateDebugInfo)) {
                      updateDebugInfo(kernel, topdcl, 0, curInst->getVISAId());
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

#ifdef _DEBUG
  for (DECLARE_LIST_ITER dcl_it = kernel.Declares.begin();
       dcl_it != kernel.Declares.end(); dcl_it++) {
    G4_Declare *curDcl = (*dcl_it);

    if (curDcl->isOutput() && gra.getLocalLR(curDcl) &&
        gra.getLocalLR(curDcl)->isGRFRegAssigned()) {
      G4_RegVar *var = curDcl->getRegVar();
      unsigned int regNum = var->getPhyReg()->asGreg()->getRegNum();
      unsigned int regOff = var->getPhyRegOff();
      unsigned int idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                         (regOff * TypeSize(curDcl->getElemType())) / G4_WSIZE;

      unsigned int numWords = curDcl->getWordSize();
      for (unsigned int i = 0; i < numWords; ++i, ++idx) {
        if (inputRegLastRef[idx] != UINT_MAX) {
          vISA_ASSERT(
              false,
              "Invalid output variable with overlapping input variable!");
        }
      }
    }
  }
#endif
}

bool LocalRA::hasDstSrcOverlapPotential(G4_DstRegRegion *dst,
                                        G4_SrcRegRegion *src) {
  bool dstOpndNumRows = false;

  if (dst->getBase()->isRegVar()) {
    G4_Declare *dstDcl = dst->getBase()->asRegVar()->getDeclare();
    if (dstDcl != nullptr) {
      int dstOffset = (dstDcl->getOffsetFromBase() + dst->getLeftBound()) /
                      kernel.numEltPerGRF<Type_UB>();
      G4_DstRegRegion *dstRgn = dst;
      dstOpndNumRows = dstRgn->getSubRegOff() * dstRgn->getTypeSize() +
                           dstRgn->getLinearizedEnd() -
                           dstRgn->getLinearizedStart() + 1 >
                       kernel.numEltPerGRF<Type_UB>();

      if (src != NULL && src->isSrcRegRegion() &&
          src->asSrcRegRegion()->getBase()->isRegVar()) {
        G4_SrcRegRegion *srcRgn = src->asSrcRegRegion();
        G4_Declare *srcDcl = src->getBase()->asRegVar()->getDeclare();
        int srcOffset = (srcDcl->getOffsetFromBase() + src->getLeftBound()) /
                        kernel.numEltPerGRF<Type_UB>();
        bool srcOpndNumRows = srcRgn->getSubRegOff() * srcRgn->getTypeSize() +
                                  srcRgn->getLinearizedEnd() -
                                  srcRgn->getLinearizedStart() + 1 >
                              kernel.numEltPerGRF<Type_UB>();

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

// Function to calculate local live intervals in ascending order of starting
// point in basic block bb.
//
// FIXME: current implementation cannot handle partial use-before-define case as
// shown below:
//
// loop_start:
// V1 (1) =
// P1 =
// (P1) (16) = V1
// ...
// V1 (16) =
// ...
// (P2) Jmp  loop_start
//
// One way to fix this is to check the emask and simd size of local def/use and
// make sure they satisfy the followign rules: 1) The SIMD size of a use should
// be equal to or smaller than earlier def seen for that variable. 2) The Emask
// of use should be equal of narrower than def seen earlier. 3) Each use should
// be fully defined in current BB (e.g., if a use is predicated,
//    the def should be either non-predicated or have the same predicate).
//
void LocalRA::calculateLiveIntervals(
    G4_BB *bb, std::vector<LocalLiveRange *> &liveIntervals) {
  int idx = 0;
  bool brk = false;

  for (INST_LIST_ITER inst_it = bb->begin(), bbend = bb->end();
       inst_it != bbend && !brk; inst_it++, idx += 2) {
    G4_INST *curInst = (*inst_it);
    G4_Declare *topdcl = NULL;
    LocalLiveRange *lr = NULL;

    if (curInst->isPseudoKill() || curInst->isLifeTimeEnd()) {
      continue;
    }

    // Scan srcs
    for (int i = 0, nSrcs = curInst->getNumSrc(); i < nSrcs; i++) {
      G4_Operand *src = curInst->getSrc(i);

      if (src && src->getTopDcl()) {
        // Scan all srcs
        topdcl = GetTopDclFromRegRegion(src);
        LocalLiveRange *topdclLR = nullptr;

        if (topdcl && (topdclLR = gra.getLocalLR(topdcl))) {
          lr = topdclLR;

          // Check whether local LR is a candidate
          if (lr->isLiveRangeLocal() && lr->isGRFRegAssigned() == false) {
            unsigned int startIdx;

            if (lr->getFirstRef(startIdx) == NULL) {
              // Skip this lr and update its ref count,
              // So it will be treated as a global LR
              lr->recordRef(NULL);
              continue;
            }

            vISA_ASSERT(idx > 0,
                        "Candidate use found in first inst of basic block");

            if ((builder.WaDisableSendSrcDstOverlap() &&
                 ((curInst->isSend() && i == 0) ||
                  (curInst->isSplitSend() && i == 1))) ||
                (curInst->isDpas() &&
                 i == 1) // For DPAS, as part of same instruction, src1 should
                         // not have overlap with dst. Src0 and src2 are okay to
                         // have overlap
                || (builder.avoidDstSrcOverlap() && curInst->getDst() != NULL &&
                    hasDstSrcOverlapPotential(curInst->getDst(),
                                              src->asSrcRegRegion()))) {
#ifdef DEBUG_VERBOSE_ON
              if (curInst->getDst() != NULL &&
                  hasDstSrcOverlapPotential(curInst->getDst(),
                                            src->asSrcRegRegion())) {
                curInst->dump();
              }
#endif
              lr->setLastRef(curInst, idx + 1);
            } else {
              lr->setLastRef(curInst, idx);
            }
          }
        }
      }
    }

    if (G4_Inst_Table[curInst->opcode()].n_dst == 1 ||
        curInst->isSplitIntrinsic()) {
      // Scan dst
      G4_DstRegRegion *dst = curInst->getDst();

      if (dst) {
        topdcl = GetTopDclFromRegRegion(dst);

        if (topdcl && (lr = gra.getLocalLR(topdcl))) {
          // Check whether local LR is a candidate
          if (lr->isLiveRangeLocal() && lr->isGRFRegAssigned() == false) {
            unsigned int startIdx;

            if (lr->getFirstRef(startIdx) == NULL) {
              lr->setFirstRef(curInst, idx);
              liveIntervals.push_back(lr);
            }

            lr->setLastRef(curInst, idx);
          }
        }
      }
    }
  }
}

void LocalRA::printAddressTakenDecls() {
  DEBUG_VERBOSE("Address taken decls:"
                << "\n");

  for (DECLARE_LIST_ITER dcl_it = kernel.Declares.begin();
       dcl_it != kernel.Declares.end(); dcl_it++) {
    G4_Declare *curDcl = (*dcl_it);

    if (curDcl->getAliasDeclare() != NULL) {
      vISA_ASSERT(gra.getLocalLR(curDcl) == NULL,
                  "Local LR found for alias declare");
      continue;
    }

    if (gra.getLocalLR(curDcl) &&
        gra.getLocalLR(curDcl)->hasIndirectAccess() == true) {
      DEBUG_VERBOSE(curDcl->getName() << ", ");
    }
  }

  DEBUG_VERBOSE("\n");
}

void LocalRA::printLocalRACandidates() {
  DEBUG_VERBOSE("Local RA candidates:"
                << "\n");

  for (DECLARE_LIST_ITER dcl_it = kernel.Declares.begin();
       dcl_it != kernel.Declares.end(); dcl_it++) {
    G4_Declare *curDcl = (*dcl_it);

    if (curDcl->getAliasDeclare() != NULL) {
      vISA_ASSERT(gra.getLocalLR(curDcl) == NULL,
                  "Local LR found for alias declare");
      continue;
    }

    if (gra.getLocalLR(curDcl) && gra.getLocalLR(curDcl)->isLiveRangeLocal() &&
        gra.getLocalLR(curDcl)->isGRFRegAssigned() == false) {
      DEBUG_VERBOSE(curDcl->getName() << ", ");
    }
  }

  DEBUG_VERBOSE("\n");
}

void LocalRA::printLocalLiveIntervals(
    G4_BB *bb, std::vector<LocalLiveRange *> &liveIntervals) {
  DEBUG_VERBOSE("Live intervals for bb " << bb->getId() << "\n");

  for (auto lr : liveIntervals) {
    unsigned int start, end;

    lr->getFirstRef(start);
    lr->getLastRef(end);

    DEBUG_VERBOSE(lr->getTopDcl()->getName()
                  << "(" << start << ", " << end << ", "
                  << lr->getTopDcl()->getByteSize() << ")");
    DEBUG_VERBOSE("\n");
  }

  DEBUG_VERBOSE("\n");
}

void LocalRA::printInputLiveIntervals() {
  DEBUG_VERBOSE("\n"
                << "Input Live intervals "
                << "\n");

  for (auto it = inputIntervals.begin(); it != inputIntervals.end(); it++) {
    [[maybe_unused]] unsigned int regWordIdx, lrEndIdx, regNum, subRegInWord;

    InputLiveRange lr = *it;

    regWordIdx = lr.getRegWordIdx();
    regNum = regWordIdx / kernel.numEltPerGRF<Type_UW>();
    subRegInWord = regWordIdx % kernel.numEltPerGRF<Type_UW>();
    lrEndIdx = lr.getLrEndIdx();

    DEBUG_VERBOSE("r" << regNum << "." << subRegInWord << " " << lrEndIdx);
    DEBUG_VERBOSE("\n");
  }

  DEBUG_VERBOSE("\n");
}

void LocalRA::countLocalLiveIntervals(
    std::vector<LocalLiveRange *> &liveIntervals, unsigned grfSize) {
  unsigned int numScalars = 0, numHalfGRF = 0, numOneGRF = 0,
               numTwoOrMoreGRF = 0, numTotal = 0;

  for (auto lr : liveIntervals) {
    unsigned size, numrows;
    G4_Declare *dcl = lr->getTopDcl();

    numrows = dcl->getNumRows();
    size = dcl->getNumElems() * dcl->getElemSize();
    numTotal++;

    if (numrows > 1) {
      numTwoOrMoreGRF++;
    } else {
      if (dcl->getNumElems() > 1 && size > (grfSize / 2u) && size <= grfSize)
        numOneGRF++;
      else if (dcl->getNumElems() > 1 && size <= (grfSize / 2u))
        numHalfGRF++;
      else if (dcl->getNumElems() == 1)
        numScalars++;
      else
        vISA_ASSERT_UNREACHABLE("Unknown size");
    }
  }

  printLocalLiveIntervalDistribution(numScalars, numHalfGRF, numOneGRF,
                                     numTwoOrMoreGRF, numTotal);
}

void LocalRA::printLocalLiveIntervalDistribution(unsigned int numScalars,
                                                 unsigned int numHalfGRF,
                                                 unsigned int numOneGRF,
                                                 unsigned int numTwoOrMoreGRF,
                                                 unsigned int numTotal) {
  DEBUG_VERBOSE("In units of num of live ranges:"
                << "\n");
  DEBUG_VERBOSE("Total local live ranges: " << numTotal << "\n");
  DEBUG_VERBOSE("2 GRFs or more: " << numTwoOrMoreGRF << "\n");
  DEBUG_VERBOSE("1 GRF: " << numOneGRF << "\n");
  DEBUG_VERBOSE("Half GRF: " << numHalfGRF << "\n");
  DEBUG_VERBOSE("Scalar: " << numScalars << "\n");
  DEBUG_VERBOSE("\n");
}

// return false if roundrobin RA should be disabled
bool LocalRA::countLiveIntervals() {
  int globalRows = 0;
  uint32_t localRows = 0;
  for (auto curDcl : kernel.Declares) {
    LocalLiveRange *curDclLR = nullptr;

    if (curDcl->getAliasDeclare() == NULL && curDcl->getRegFile() == G4_GRF &&
        (curDclLR = gra.getLocalLR(curDcl)) &&
        curDclLR->isGRFRegAssigned() == false) {
      if (curDclLR->isLiveRangeGlobal()) {
        globalRows += curDcl->getNumRows();
      } else if (curDclLR->isLiveRangeLocal()) {
        localRows += curDcl->getNumRows();
      }
    }
  }

  int scaledUniqueAssign = kernel.getScaledGRFSize(NUM_PREGS_FOR_UNIQUE_ASSIGN);

  if (globalRows <= scaledUniqueAssign) {
    globalLRSize = globalRows;
  } else {
    if (localRows < (numRegLRA - scaledUniqueAssign)) {
      globalLRSize = scaledUniqueAssign;
    } else {
      return false;
    }
  }

  return true;
}

// ********* LocalLiveRange class implementation *********
void LocalLiveRange::recordRef(G4_BB *bb) {
  if (bb != prevBBRef)
    numRefsInFG++;

  prevBBRef = bb;
}

bool LocalLiveRange::isLiveRangeLocal() const {
  if (isIndirectAccess == false && numRefsInFG == 1 && eot == false &&
      !builder.isPreDefArg(topdcl) && !builder.isPreDefRet(topdcl) &&
      topdcl->isOutput() == false) {
    return true;
  }

  return false;
}

bool LocalLiveRange::isLiveRangeGlobal() {
  if (isIndirectAccess == true || (numRefsInFG > 1 && eot == false) ||
      topdcl->isOutput() == true) {
    return true;
  }

  return false;
}

bool LocalLiveRange::isGRFRegAssigned() {
  vISA_ASSERT(topdcl != NULL, "Top dcl not set");
  G4_RegVar *rvar = topdcl->getRegVar();
  bool isPhyRegAssigned = false;

  if (rvar) {
    if (rvar->isPhyRegAssigned())
      isPhyRegAssigned = true;
  }

  return isPhyRegAssigned;
}

unsigned int LocalLiveRange::getSizeInWords() {
  int nrows = getTopDcl()->getNumRows();
  int elemsize = getTopDcl()->getElemSize();
  int nelems = getTopDcl()->getNumElems();
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

// ********* PhyRegsLocalRA class implementation *********
void PhyRegsLocalRA::setGRFBusy(int which) {
  vISA_ASSERT(isGRFAvailable(which), "Invalid register");

  // all 1 word mask based on register size
  uint64_t wordMask = (1ULL << (builder.getGRFSize() / 2)) - 1;
  regBusyVector[which] = (uint32_t)wordMask;

  if (twoBanksRA) {
    if (which < SECOND_HALF_BANK_START_GRF) {
      bank1AvailableRegNum--;
    } else {
      bank2AvailableRegNum--;
    }
  }
}

// ********* PhyRegsLocalRA class implementation *********
void PhyRegsLocalRA::setGRFBusy(int which, int howmany) {
  for (int i = 0; i < howmany; i++) {
    setGRFBusy(which + i);
  }
}

void PhyRegsLocalRA::setWordBusy(int whichgrf, int word) {
  vISA_ASSERT(isGRFAvailable(whichgrf), "Invalid register");
  vISA_ASSERT(word < (int)builder.numEltPerGRF<Type_UW>(), "Invalid word");

  if (twoBanksRA) {
    if (regBusyVector[whichgrf] == 0) {
      if (whichgrf < SECOND_HALF_BANK_START_GRF) {
        bank1AvailableRegNum--;
      } else {
        bank2AvailableRegNum--;
      }
    }
  }

  regBusyVector[whichgrf] |= (WORD_BUSY << word);
}

void PhyRegsLocalRA::setWordBusy(int whichgrf, int word, int howmany) {
  for (int i = 0; i < howmany; i++) {
    setWordBusy(whichgrf, word + i);
  }
}

void PhyRegsLocalRA::setGRFNotBusy(int which, int instID) {
  vISA_ASSERT(isGRFAvailable(which), "Invalid register");

  regBusyVector[which] = 0;

  if (twoBanksRA) {
    if (which < SECOND_HALF_BANK_START_GRF) {
      lastUseSum1 -= regLastUse[which];
      lastUseSum1 += instID;
      bank1AvailableRegNum++;
    } else {
      lastUseSum2 -= regLastUse[which];
      lastUseSum2 += instID;
      bank2AvailableRegNum++;
    }
  }
  if (instID) {
    regLastUse[which] = instID;
  }
}

void PhyRegsLocalRA::setWordNotBusy(int whichgrf, int word, int instID) {
  vISA_ASSERT(isGRFAvailable(whichgrf), "Invalid register");
  vISA_ASSERT(word < (int)builder.numEltPerGRF<Type_UW>(), "Invalid word");

  if (twoBanksRA) {
    if (whichgrf < SECOND_HALF_BANK_START_GRF) {
      lastUseSum1 -= regLastUse[whichgrf];
      lastUseSum1 += instID;
      if (regBusyVector[whichgrf] == 0) {
        bank1AvailableRegNum++;
      }
    } else {
      lastUseSum2 -= regLastUse[whichgrf];
      lastUseSum2 += instID;
      if (regBusyVector[whichgrf] == 0) {
        bank2AvailableRegNum++;
      }
    }
  }
  uint32_t mask = ~(1 << word);
  regBusyVector[whichgrf] &= mask;
  if (instID) {
    regLastUse[whichgrf] = instID;
  }
}

inline bool PhyRegsLocalRA::isWordBusy(int whichgrf, int word) {
  vISA_ASSERT(isGRFAvailable(whichgrf), "Invalid register");

  vISA_ASSERT(word < (int)builder.numEltPerGRF<Type_UW>(), "Invalid word");
  bool isBusy = ((regBusyVector[whichgrf] & (WORD_BUSY << word)) != 0);
  return isBusy;
}

bool PhyRegsLocalRA::isWordBusy(int whichgrf, int word, int howmany) {
  bool retval = false;

  for (int i = 0; i < howmany && !retval; i++) {
    retval |= isWordBusy(whichgrf, word + i);
  }

  return retval;
}

bool PhyRegsLocalRA::findFreeMultipleRegsForward(
    int regIdx, BankAlign align, int &regnum, int nrows, int lastRowSize,
    int endReg, unsigned short occupiedBundles, int instID, bool isHybridAlloc,
    std::unordered_set<unsigned int> &forbidden) {
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
  i = findBundleConflictFreeRegister(i, endReg, occupiedBundles, align,
                                     multiSteps);

  startReg = i;
  while (i <= endReg + nrows - 1) {
    if (isGRFAvailable(i) && forbidden.find(i) == forbidden.end() &&
        regBusyVector[i] == 0 &&
        (!isHybridAlloc ||
         (((instID - regLastUse[i]) / 2 >= LraFFWindowSize) ||
          (regLastUse[i] == 0)))) {
      foundItem++;
    } else if (foundItem < grfRows) {
      foundItem = 0;
      i++;
      findRegisterCandiateWithAlignForward(i, align, multiSteps);
      i = findBundleConflictFreeRegister(i, endReg, occupiedBundles, align,
                                         multiSteps);
      startReg = i;
      continue;
    }

    if (foundItem == grfRows) {
      if (lastRowSize % builder.numEltPerGRF<Type_UW>() == 0) {
        regnum = startReg;
        return true;
      } else {
        if (i + 1 <= endReg + nrows - 1 && isGRFAvailable(i + 1) &&
            forbidden.find(i + 1) == forbidden.end() &&
            (isWordBusy(i + 1, 0, lastRowSize) == false) &&
            (!isHybridAlloc ||
             (((instID - regLastUse[i + 1]) / 2 >= LraFFWindowSize) ||
              (regLastUse[i + 1] == 0)))) {
          regnum = startReg;
          return true;
        } else {
          foundItem = 0;
          i++;
          findRegisterCandiateWithAlignForward(i, align, multiSteps);
          i = findBundleConflictFreeRegister(i, endReg, occupiedBundles, align,
                                             multiSteps);
          startReg = i;
          continue;
        }
      }
    }

    i++;
  }

  return false;
}

bool PhyRegsLocalRA::findFreeMultipleRegsBackward(
    int regIdx, BankAlign align, int &regnum, int nrows, int lastRowSize,
    int endReg, int instID, bool isHybridAlloc,
    std::unordered_set<unsigned int> &forbidden) {
  int foundItem = 0;
  int startReg = 0;
  int grfRows = 0;
  int i = regIdx;
  bool multiSteps = nrows > 1;

  if (lastRowSize % builder.numEltPerGRF<Type_UW>() == 0) {
    grfRows = nrows;
  } else {
    grfRows = nrows - 1;
  }

  findRegisterCandiateWithAlignBackward(i, align, multiSteps);

  startReg = i;

  while (i >= endReg && i >= 0) {
    if (isGRFAvailable(i) && forbidden.find(i) == forbidden.end() &&
        regBusyVector[i] == 0 &&
        (!isHybridAlloc || (((instID - regLastUse[i]) / 2 >= LraFFWindowSize) ||
                            (regLastUse[i] == 0)))) {
      foundItem++;
    } else if (foundItem < grfRows) {
      foundItem = 0;
      i -= nrows;
      findRegisterCandiateWithAlignBackward(i, align, multiSteps);

      startReg = i;
      continue;
    }

    if (foundItem == grfRows) {
      if (lastRowSize % builder.numEltPerGRF<Type_UW>() == 0) {
        regnum = startReg;
        return true;
      } else {
        if (i + 1 <= endReg && isGRFAvailable(i + 1) &&
            forbidden.find(i + 1) == forbidden.end() &&
            (isWordBusy(i + 1, 0, lastRowSize) == false) &&
            (!isHybridAlloc ||
             (((instID - regLastUse[i + 1]) / 2 >= LraFFWindowSize) ||
              (regLastUse[i + 1] == 0)))) {
          regnum = startReg;
          return true;
        } else {
          foundItem = 0;
          i -= nrows;
          findRegisterCandiateWithAlignBackward(i, align, multiSteps);

          startReg = i;
          continue;
        }
      }
    }

    i++;
  }

  return false;
}

bool PhyRegsLocalRA::findFreeSingleReg(
    int regIdx, int size, BankAlign align, G4_SubReg_Align subalign,
    int &regnum, int &subregnum, int endReg, int instID, bool isHybridAlloc,
    bool forward, std::unordered_set<unsigned int> &forbidden) {
  int i = regIdx;
  bool found = false;

  while (!found) {
    if (forward) {
      if (i > endReg) //<= works
        break;
    } else {
      if (i < endReg) //>= works
        break;
    }

    // Align GRF
    if ((align == BankAlign::Even) && (i % 2 != 0)) {
      i += forward ? 1 : -1;
      continue;
    } else if ((align == BankAlign::Odd) && (i % 2 == 0)) {
      i += forward ? 1 : -1;
      continue;
    } else if ((align == BankAlign::QuadGRF) && (i % 4 != 0)) {
      i += forward ? (4 - (i % 4)) : -(i % 4);
      continue;
    } else if ((align == BankAlign::Even2GRF) && ((i % 4 >= 2))) {
      i += forward ? 1 : -1;
      continue;
    } else if ((align == BankAlign::Odd2GRF) && ((i % 4 < 2))) {
      i += forward ? 1 : -1;
      continue;
    }

    if (isGRFAvailable(i, 1) && forbidden.find(i) == forbidden.end() &&
        (!isHybridAlloc || (((instID - regLastUse[i]) / 2 >= LraFFWindowSize) ||
                            (regLastUse[i] == 0)))) {
      found = findFreeSingleReg(i, subalign, regnum, subregnum, size);
      if (found) {
        return true;
      }
    }
    i += forward ? 1 : -1;
  }

  return false;
}

void PhyRegsLocalRA::printBusyRegs() {
  for (int i = 0; i < (int)numRegs; i++) {
    if (isGRFAvailable(i) == false)
      continue;

    for (int j = 0; j < (int)builder.numEltPerGRF<Type_UW>(); j++) {
      if (isWordBusy(i, j) == true) {
        DEBUG_VERBOSE("r" << i << "." << j << ":w, "
                          << "\n");
      }
    }
  }
}

// Based on RegVar, mark available registers as busy
void PhyRegsLocalRA::markPhyRegs(G4_Declare *topdcl) {
  G4_RegVar *rvar = topdcl->getRegVar();
  unsigned numrows = topdcl->getNumRows();
  unsigned numwords = 0;
  unsigned int regnum = 0;

  if (!rvar->getPhyReg() || !rvar->isGreg()) {
    return;
  }
  // Calculate number of physical registers required by this dcl
  if (numrows == 1) {
    unsigned numbytes = topdcl->getElemSize() * topdcl->getNumElems();
    numwords = (numbytes + 1) / 2;
    unsigned int subReg = rvar->getPhyRegOff();
    unsigned int subRegInWord = subReg * rvar->getDeclare()->getElemSize() / 2;

    regnum = rvar->getPhyReg()->asGreg()->getRegNum();
    if (isGRFAvailable(regnum) == true) {
      for (unsigned int i = 0; i < numwords; i++) {
        // In case across one GRF
        if (subRegInWord + i >= builder.numEltPerGRF<Type_UW>()) {
          regnum++;
          if (!isGRFAvailable(regnum)) {
            break;
          }
          numwords -= i;
          i = 0;
          subRegInWord = 0;
        }

        // Starting from first word, mark each consecutive word busy
        setWordBusy(regnum, (subRegInWord + i));
      }
    }
  } else {
    regnum = rvar->getPhyReg()->asGreg()->getRegNum();
    for (unsigned int i = 0; i < topdcl->getNumRows(); i++) {
      if (isGRFAvailable(i + regnum) == true) {
        // Set entire GRF busy
        setGRFBusy(regnum + i);
      }
    }
  }
}

// ********* PhyRegsManager class implementation *********
bool PhyRegsLocalRA::findFreeSingleReg(int regIdx, G4_SubReg_Align subalign,
                                       int &regnum, int &subregnum, int size) {
  bool found = false;
  if (subalign == builder.getGRFAlign()) {
    if (isWordBusy(regIdx, 0, size) == false) {
      subregnum = 0;
      found = true;
    }
  } else if (subalign == builder.getHalfGRFAlign()) {
    if (isWordBusy(regIdx, 0, size) == false) {
      subregnum = 0;
      found = true;
    } else if (size <= (int)builder.numEltPerGRF<Type_UW>() / 2 &&
               isWordBusy(regIdx, builder.numEltPerGRF<Type_UW>() / 2, size) ==
                   false) {
      subregnum = builder.numEltPerGRF<Type_UW>() / 2;
      found = true;
    }
  } else {
    // ToDo: check if dynamic step size has compile time impact
    int step = 1;
    int upBound = builder.numEltPerGRF<Type_UW>() - size + 1;
    switch (subalign) {
    case Eight_Word:
      step = 8;
      break;
    case Four_Word:
      step = 4;
      break;
    case Even_Word:
      step = 2;
      break;
    case Any:
      upBound = builder.numEltPerGRF<Type_UW>() -
                size; // FIXME, why not the last word
      step = 1;
      break;
    default:
      vISA_ASSERT_UNREACHABLE("unexpected alignment");
    }
    for (int j = 0; j < upBound; j += step) {
      if (!isWordBusy(regIdx, j, size)) {
        subregnum = j;
        found = true;
        break;
      }
    }
  }

  if (found) {
    regnum = regIdx;
  }

  return found;
}

int PhyRegsManager::findFreeRegs(int size, BankAlign align,
                                 G4_SubReg_Align subalign, int &regnum,
                                 int &subregnum, int startRegNum, int endRegNum,
                                 unsigned short occupiedBundles,
                                 unsigned int instID, bool isHybridAlloc,
                                 std::unordered_set<unsigned int> &forbidden) {
  int nrows = 0;
  int lastRowSize = 0;
  LocalRA::getRowInfo(size, nrows, lastRowSize, builder);

  bool forward = (startRegNum <= endRegNum ? true : false);
  int startReg = forward ? startRegNum : startRegNum - nrows + 1;
  int endReg = forward ? endRegNum - nrows + 1 : endRegNum;

  bool found = false;

  if (size >= (int)builder.numEltPerGRF<Type_UW>()) {
    if (forward) {
      found = availableRegs.findFreeMultipleRegsForward(
          startReg, align, regnum, nrows, lastRowSize, endReg, occupiedBundles,
          instID, isHybridAlloc, forbidden);
    } else {
      found = availableRegs.findFreeMultipleRegsBackward(
          startReg, align, regnum, nrows, lastRowSize, endReg, instID,
          isHybridAlloc, forbidden);
    }
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
    found = availableRegs.findFreeSingleReg(startReg, size, align, subalign,
                                            regnum, subregnum, endReg, instID,
                                            isHybridAlloc, forward, forbidden);
    if (found) {
      availableRegs.setWordBusy(regnum, subregnum, size);
    }
  }

  if (found) {
    return nrows;
  }

  return 0;
}

// subregnum parameter is expected to be in units of word
void PhyRegsManager::freeRegs(int regnum, int subregnum, int numwords,
                              int instID) {
  while (numwords >= (int)builder.numEltPerGRF<Type_UW>()) {
    availableRegs.setGRFNotBusy(regnum, instID);
    numwords -= builder.numEltPerGRF<Type_UW>();
    regnum++;
  }

  while (numwords >= 1) {
    availableRegs.setWordNotBusy(regnum, subregnum, instID);
    subregnum++;

    if (subregnum >= (int)builder.numEltPerGRF<Type_UW>()) {
      subregnum = 0;
      regnum++;
    }
    numwords--;
  }
}

// ********* LinearScan class implementation *********

LinearScan::LinearScan(GlobalRA &g,
                       std::vector<LocalLiveRange *> &localLiveIntervals,
                       std::list<InputLiveRange> &inputLivelIntervals,
                       PhyRegsManager &pregMgr, PhyRegsLocalRA &pregs,
                       PhyRegSummary *s, unsigned int numReg, unsigned int glrs,
                       bool roundRobin, bool bankConflict,
                       bool internalConflict, bool splitLLR, unsigned int simdS)
    : gra(g), builder(g.builder), pregManager(pregMgr), initPregs(pregs),
      liveIntervals(localLiveIntervals), inputIntervals(inputLivelIntervals),
      summary(s),
      pregs(g.kernel.getNumRegTotal() * g.kernel.numEltPerGRF<Type_UW>(),
            false),
      simdSize(simdS), globalLRSize(glrs), numRegLRA(numReg),
      useRoundRobin(roundRobin), doBankConflict(bankConflict),
      highInternalConflict(internalConflict), doSplitLLR(splitLLR) {

  // FIXME: this entire class is a mess, whole thing needs a rewrite
  if (g.kernel.getNumRegTotal() < 128) {
    // code below can segfault if user specifies #GRF < 128. We just hack it so
    // bank1 == bank2
    bank1StartGRFReg = bank1_start = 0;
    bank2StartGRFReg = bank2_start = 0;
    bank1_end = bank2_end = numRegLRA - 1;
    startGRFReg = &bank1StartGRFReg;
    return;
  }

  // register number boundaries
  bank1_start = 0;
  bank1_end = SECOND_HALF_BANK_START_GRF - globalLRSize / 2 - 1;
  if (useRoundRobin) { // From middle to back
    bank2_start = SECOND_HALF_BANK_START_GRF + (globalLRSize + 1) / 2;
    bank2_end = numRegLRA - 1;
  } else { // From back to middle
    bank2_start = numRegLRA - 1;
    bank2_end = SECOND_HALF_BANK_START_GRF + (globalLRSize + 1) / 2;
  }

  // register number pointers
  bank1StartGRFReg = bank1_start;
  bank2StartGRFReg = bank2_start;

  // register pointer
  startGRFReg = &bank1StartGRFReg;

  int bank1AvailableRegNum = 0;
  for (int i = 0; i < SECOND_HALF_BANK_START_GRF; i++) {
    if (pregManager.getAvailableRegs()->isGRFAvailable(i) &&
        !pregManager.getAvailableRegs()->isGRFBusy(i)) {
      bank1AvailableRegNum++;
    }
  }
  pregManager.getAvailableRegs()->setBank1AvailableRegNum(bank1AvailableRegNum);

  int bank2AvailableRegNum = 0;
  for (unsigned int i = SECOND_HALF_BANK_START_GRF; i < numRegLRA; i++) {
    if (pregManager.getAvailableRegs()->isGRFAvailable(i) &&
        !pregManager.getAvailableRegs()->isGRFBusy(i)) {
      bank2AvailableRegNum++;
    }
  }
  pregManager.getAvailableRegs()->setBank2AvailableRegNum(bank2AvailableRegNum);
}

// Linear scan implementation
void LinearScan::run(G4_BB *bb, IR_Builder &builder, LLR_USE_MAP &LLRUseMap) {
  unsigned int idx = 0;
  bool allocateRegResult = false;

  unsigned int firstLocalIdx = 0, firstGlobalIdx = 0;
  if (liveIntervals.size() > 0) {
    LocalLiveRange *firstLr = (*liveIntervals.begin());
    G4_INST *firstInst = firstLr->getFirstRef(firstLocalIdx);
    firstGlobalIdx = firstInst->getLexicalId();
  }

  for (auto lr : liveIntervals) {
    G4_INST *currInst = lr->getFirstRef(idx);

    expireRanges(idx);
    expireInputRanges(currInst->getLexicalId(), idx, firstGlobalIdx);

    if (doBankConflict && builder.lowHighBundle() &&
        (highInternalConflict ||
         (simdSize >= 16 && builder.oneGRFBankDivision()))) {
      allocateRegResult = allocateRegsFromBanks(lr);
    } else {
      allocateRegResult = allocateRegs(lr, bb, builder, LLRUseMap);
    }

    if (allocateRegResult) {
      updateActiveList(lr);
#ifdef _DEBUG
      int startregnum, endregnum, startsregnum, endsregnum;
      G4_VarBase *op;
      op = lr->getPhyReg(startsregnum);

      startregnum = endregnum = op->asGreg()->getRegNum();
      endsregnum = startsregnum +
                   (lr->getTopDcl()->getNumElems() *
                    lr->getTopDcl()->getElemSize() / 2) -
                   1;

      vISA_ASSERT(((unsigned int)startregnum + lr->getTopDcl()->getNumRows() -
                   1) < numRegLRA,
                  "Linear scan allocated unavailable physical GRF");

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
#ifdef DEBUG_VERBOSE_ON
      DEBUG_VERBOSE("Assigned physical register to "
                    << lr->getTopDcl()->getName() << " (r" << startregnum << "."
                    << startsregnum << ":w - "
                    << "r" << endregnum << "." << endsregnum << ":w)"
                    << "\n");
#endif
#endif
    }
  }

  expireAllActive();
}

void LinearScan::updateActiveList(LocalLiveRange *lr) {
  bool done = false;
  unsigned int newlr_end;

  lr->getLastRef(newlr_end);

  for (auto active_it = active.begin(); active_it != active.end();
       active_it++) {
    unsigned int end_idx;
    LocalLiveRange *active_lr = (*active_it);

    active_lr->getLastRef(end_idx);

    if (end_idx > newlr_end) {
      active.insert(active_it, lr);
      done = true;
      break;
    }
  }

  if (done == false)
    active.push_back(lr);
}

void LinearScan::expireAllActive() {
  if (active.size() > 0) {
    // Expire any remaining ranges
    LocalLiveRange *lastActive = active.back();
    unsigned int endIdx;

    lastActive->getLastRef(endIdx);

    expireRanges(endIdx);
  }
}

// idx is the current position being processed
// The function will expire active ranges that end at or before idx
void LinearScan::expireRanges(unsigned int idx) {
  // active list is sorted in ascending order of starting index

  while (active.size() > 0) {
    unsigned int endIdx;
    LocalLiveRange *lr = active.front();

    lr->getLastRef(endIdx);

    if (endIdx <= idx) {
      G4_VarBase *preg;
      int subregnumword, subregnum;

      preg = lr->getPhyReg(subregnumword);

      if (preg) {
        subregnum =
            LocalRA::convertSubRegOffFromWords(lr->getTopDcl(), subregnumword);

        // Mark the RegVar object of dcl as assigned to physical register
        lr->getTopDcl()->getRegVar()->setPhyReg(preg, subregnum);
        lr->setAssigned(true);

        if (summary) {
          // mark physical register as used atleast once
          summary->markPhyRegs(preg, lr->getSizeInWords());
        }
      }

      // Free physical regs marked for this range
      freeAllocedRegs(lr, true);

#ifdef DEBUG_VERBOSE_ON
      DEBUG_VERBOSE("Expiring range " << lr->getTopDcl()->getName() << "\n");
#endif

      // Remove range from active list
      active.pop_front();
    } else {
      // As soon as we find first range that ends after ids break loop
      break;
    }
  }
}

void LinearScan::expireInputRanges(unsigned int global_idx,
                                   unsigned int local_idx,
                                   unsigned int first_idx) {
  while (inputIntervals.size() > 0) {
    InputLiveRange lr = inputIntervals.front();
    unsigned int endIdx = lr.getLrEndIdx();

    if (endIdx <= global_idx) {
      unsigned int regnum =
          lr.getRegWordIdx() / builder.numEltPerGRF<Type_UW>();
      unsigned int subRegInWord =
          lr.getRegWordIdx() % builder.numEltPerGRF<Type_UW>();
      int inputIdx =
          (endIdx < first_idx) ? 0 : (local_idx - (global_idx - endIdx) * 2);

      // Free physical regs marked for this range
      pregManager.freeRegs(regnum, subRegInWord, 1, inputIdx);
      initPregs.setWordNotBusy(regnum, subRegInWord, 0);

#ifdef DEBUG_VERBOSE_ON
      DEBUG_VERBOSE("Expiring input r" << regnum << "." << subRegInWord
                                       << "\n");
#endif

      // Remove range from inputIntervals list
      inputIntervals.pop_front();
    } else {
      // As soon as we find first range that ends after ids break loop
      break;
    }
  }
}

// Allocate registers to live range. It makes a decision whether to spill
// a currently active range or the range passed as parameter. The range
// that has larger size and is longer is the spill candidate.
// Return true if free registers found, false if range is to be spilled
bool LinearScan::allocateRegs(LocalLiveRange *lr, G4_BB *bb,
                              IR_Builder &builder, LLR_USE_MAP &LLRUseMap) {
  int regnum, subregnum;
  unsigned int localRABound = 0;
  unsigned int instID;

  G4_INST *currInst = lr->getFirstRef(instID);

  // Let local RA allocate only those ranges that need < 10 GRFs
  // Larger ranges are not many and are best left to global RA
  // as it can make a better judgment by considering the
  // spill cost.
  int nrows = 0;
  int size = lr->getSizeInWords();
  G4_Declare *dcl = lr->getTopDcl();

  G4_SubReg_Align subalign = gra.getSubRegAlign(dcl);
  BankAlign preBank = BankAlign::Either;
  unsigned short occupiedBundles =
      getOccupiedBundle(builder, gra, dcl, preBank);
  localRABound = numRegLRA - globalLRSize -
                 1; //-1, localRABound will be counted in findFreeRegs()
  BankAlign bankAlign = gra.isQuadAligned(dcl)   ? BankAlign::QuadGRF
                        : gra.isEvenAligned(dcl) ? BankAlign::Even
                                                 : BankAlign::Either;
  if (bankAlign == BankAlign::Either && doBankConflict) {
    bankAlign = gra.getBankAlign(dcl);
  } else {
    if (bankAlign == BankAlign::Either) {
      bankAlign = preBank;
    }
  }

  if (useRoundRobin) {
    nrows = pregManager.findFreeRegs(
        size, bankAlign, subalign, regnum, subregnum, *startGRFReg,
        localRABound, occupiedBundles, instID, false, lr->getForbidden());
  } else {
    nrows = pregManager.findFreeRegs(
        size, bankAlign, subalign, regnum, subregnum, *startGRFReg,
        localRABound, occupiedBundles, instID, true, lr->getForbidden());

    if (!nrows) {
      nrows = pregManager.findFreeRegs(
          size, bankAlign, subalign, regnum, subregnum, *startGRFReg,
          localRABound, occupiedBundles, instID, false, lr->getForbidden());
    }
  }

  if (useRoundRobin) {
    if (nrows) {
      *startGRFReg = (regnum + nrows) % (localRABound);
    } else {
      unsigned int endGRFReg = *startGRFReg + nrows;
      endGRFReg = (endGRFReg > localRABound) ? localRABound : endGRFReg;

      nrows = pregManager.findFreeRegs(size, bankAlign, subalign, regnum,
                                       subregnum, 0, endGRFReg, occupiedBundles,
                                       instID, false, lr->getForbidden());

      if (nrows) {
        *startGRFReg = (regnum + nrows) % (localRABound);
      }
    }
  }

  // If allocations fails, return false
  if (!nrows) {
    // Check if any range in active is longer and wider than lr.
    // Spill it if it is and call self again.
    for (auto active_it = active.rbegin(); active_it != active.rend();
         active_it++) {
      LocalLiveRange *activeLR = (*active_it);

      if (activeLR->getSizeInWords() >= lr->getSizeInWords() &&
          (!doSplitLLR ||
           gra.getNumRefs(activeLR->getTopDcl()) > SPLIT_REF_CNT_THRESHOLD)) {
        // Another active range is larger than lr
        unsigned int active_lr_end, lr_end;

        activeLR->getLastRef(active_lr_end);
        lr->getLastRef(lr_end);

        if (active_lr_end > lr_end) {
          // Range in active is longer, so spill it
#ifdef DEBUG_VERBOSE_ON
          DEBUG_VERBOSE("Spilling range " << activeLR->getTopDcl()->getName()
                                          << "\n");
#endif

          freeAllocedRegs(activeLR, false);
          active.erase(--(active_it.base()));

          if (doSplitLLR) {
            G4_Declare *oldDcl = activeLR->getTopDcl();
            unsigned short totalElems = oldDcl->getTotalElems();

            std::vector<std::pair<INST_LIST_ITER, unsigned int>> *useList =
                nullptr;
            auto Iter = LLRUseMap.find(activeLR);
            if (Iter != LLRUseMap.end())
              useList = &(Iter->second);

            if (useList && useList->size() > 2 &&
                gra.getNumRefs(oldDcl) - useList->size() == 1 &&
                (totalElems == 8 || totalElems == 16 || totalElems == 32)) {
              bool split = false;
              unsigned int useCnt = 0;
              INST_LIST_ITER lastUseIt;
              G4_Declare *newDcl = NULL;
              for (const auto &usePoint : *useList) {
                INST_LIST_ITER useIt = usePoint.first;
                G4_INST *useInst = *useIt;
                useCnt++;

                unsigned int currId = currInst->getLexicalId();
                if (useInst->getLexicalId() < currId ||
                    useCnt < SPLIT_USE_CNT_THRESHOLD) {
                  lastUseIt = useIt;
                } else if (split == false &&
                           (useInst->getLexicalId() -
                            (*lastUseIt)->getLexicalId()) >
                               SPLIT_USE_DISTANCE_THRESHOLD &&
                           !(oldDcl->getElemSize() >= 8 &&
                             oldDcl->getTotalElems() >= 16)) {
                  G4_Declare *splitDcl = NULL;
                  const char *splitDclName =
                      builder.getNameString(16, "split_%s", oldDcl->getName());
                  splitDcl = builder.createDeclare(
                      splitDclName, G4_GRF, oldDcl->getNumElems(),
                      oldDcl->getNumRows(), oldDcl->getElemType());
                  splitDcl->copyAlign(oldDcl);

                  LocalLiveRange *splitLR =
                      gra.GetOrCreateLocalLiveRange(splitDcl);
                  splitLR->markSplit();

                  INST_LIST_ITER iter = lastUseIt;
                  iter++;

                  // FIXME: The following code does not handle QWord variable
                  // spilling completely. See the last condition of this
                  // if-stmt.
                  G4_DstRegRegion *dst =
                      builder.createDstRegRegion(splitDcl, 1);
                  G4_SrcRegRegion *src = builder.createSrcRegRegion(
                      oldDcl, builder.getRegionStride1());
                  G4_ExecSize splitInstExecSize(oldDcl->getTotalElems() > 16
                                                    ? 16
                                                    : oldDcl->getTotalElems());
                  G4_INST *splitInst = builder.createMov(
                      splitInstExecSize, dst, src, InstOpt_WriteEnable, false);
                  bb->insertBefore(iter, splitInst);

                  unsigned int idx = 0;
                  gra.getLocalLR(oldDcl)->setLastRef(splitInst, idx);
                  gra.setNumRefs(oldDcl, useCnt);
                  splitLR->setFirstRef(splitInst, idx);
                  gra.setNumRefs(splitDcl, 2);

                  if (totalElems == 32) {
                    G4_DstRegRegion *dst = builder.createDst(
                        splitDcl->getRegVar(), 2, 0, 1, oldDcl->getElemType());
                    auto src = builder.createSrc(oldDcl->getRegVar(), 2, 0,
                                                 builder.getRegionStride1(),
                                                 oldDcl->getElemType());
                    G4_INST *splitInst2 = builder.createMov(
                        g4::SIMD16, dst, src, InstOpt_WriteEnable, false);
                    bb->insertBefore(iter, splitInst2);
                  }

                  const char *newDclName =
                      builder.getNameString(16, "copy_%s", oldDcl->getName());
                  newDcl = builder.createDeclare(
                      newDclName, G4_GRF, oldDcl->getNumElems(),
                      oldDcl->getNumRows(), oldDcl->getElemType());
                  newDcl->copyAlign(oldDcl);

                  unsigned int oldRefs = gra.getNumRefs(oldDcl);
                  LocalLiveRange *newLR = gra.GetOrCreateLocalLiveRange(newDcl);

                  iter = useIt;

                  dst = builder.createDstRegRegion(newDcl, 1);
                  src = builder.createSrcRegRegion(splitDcl,
                                                   builder.getRegionStride1());
                  G4_INST *movInst = builder.createMov(
                      G4_ExecSize(splitDcl->getTotalElems() > 16
                                      ? 16
                                      : splitDcl->getTotalElems()),
                      dst, src, InstOpt_WriteEnable, false);
                  bb->insertBefore(iter, movInst);

                  splitLR->setLastRef(movInst, idx);
                  G4_INST *old_last_use = activeLR->getLastRef(idx);
                  newLR->setFirstRef(splitInst, idx);
                  newLR->setLastRef(old_last_use, idx);
                  gra.setNumRefs(newDcl, oldRefs - useCnt + 1);

                  if (totalElems == 32) {
                    G4_DstRegRegion *dst = builder.createDst(
                        newDcl->getRegVar(), 2, 0, 1, splitDcl->getElemType());
                    auto src = builder.createSrc(splitDcl->getRegVar(), 2, 0,
                                                 builder.getRegionStride1(),
                                                 splitDcl->getElemType());
                    G4_INST *movInst2 = builder.createMov(
                        g4::SIMD16, dst, src, InstOpt_WriteEnable, false);
                    bb->insertBefore(iter, movInst2);
                  }

                  unsigned int pos = usePoint.second;
                  G4_SrcRegRegion *oldSrc =
                      useInst->getSrc(pos)->asSrcRegRegion();
                  G4_Declare *oldSrcDcl =
                      oldSrc->getBase()->asRegVar()->getDeclare();
                  G4_Declare *newSrcDcl = newDcl;
                  G4_Declare *aliasOldSrcDcl = oldSrcDcl->getAliasDeclare();
                  if (aliasOldSrcDcl != NULL) {
                    const char *newSrcDclName = builder.getNameString(
                        16, "copy_%s", oldSrcDcl->getName());
                    newSrcDcl = builder.createDeclare(
                        newSrcDclName, G4_GRF, oldSrcDcl->getNumElems(),
                        oldSrcDcl->getNumRows(), oldSrcDcl->getElemType());
                    newSrcDcl->copyAlign(oldSrcDcl);
                    newSrcDcl->setAliasDeclare(aliasOldSrcDcl,
                                               oldSrcDcl->getAliasOffset());
                  }
                  auto newSrc = builder.createSrcRegRegion(
                      oldSrc->getModifier(), Direct, newSrcDcl->getRegVar(),
                      oldSrc->getRegOff(), oldSrc->getSubRegOff(),
                      oldSrc->getRegion(), oldSrc->getType());
                  useInst->setSrc(newSrc, pos);
                  while (aliasOldSrcDcl && aliasOldSrcDcl != oldDcl) {
                    oldSrcDcl = aliasOldSrcDcl;
                    const char *newSrcDclName = builder.getNameString(
                        16, "copy_%s", oldSrcDcl->getName());
                    newSrcDcl = builder.createDeclare(
                        newSrcDclName, G4_GRF, oldSrcDcl->getNumElems(),
                        oldSrcDcl->getNumRows(), oldSrcDcl->getElemType());
                    newSrcDcl->copyAlign(oldSrcDcl);
                    aliasOldSrcDcl = oldSrcDcl->getAliasDeclare();
                    vISA_ASSERT(aliasOldSrcDcl != NULL, "Invalid alias decl");
                    newSrcDcl->setAliasDeclare(aliasOldSrcDcl,
                                               oldSrcDcl->getAliasOffset());
                  }

                  split = true;
                } else if (split) {
                  unsigned int pos = usePoint.second;
                  G4_SrcRegRegion *oldSrc =
                      useInst->getSrc(pos)->asSrcRegRegion();
                  G4_Declare *oldSrcDcl =
                      oldSrc->getBase()->asRegVar()->getDeclare();
                  G4_Declare *newSrcDcl = newDcl;
                  G4_Declare *aliasOldSrcDcl = oldSrcDcl->getAliasDeclare();
                  if (aliasOldSrcDcl != NULL) {
                    const char *newSrcDclName = builder.getNameString(
                        16, "copy_%s", oldSrcDcl->getName());
                    newSrcDcl = builder.createDeclare(
                        newSrcDclName, G4_GRF, oldSrcDcl->getNumElems(),
                        oldSrcDcl->getNumRows(), oldSrcDcl->getElemType());
                    newSrcDcl->copyAlign(oldSrcDcl);
                    newSrcDcl->setAliasDeclare(aliasOldSrcDcl,
                                               oldSrcDcl->getAliasOffset());
                  }
                  auto newSrc = builder.createSrcRegRegion(
                      oldSrc->getModifier(), Direct, newSrcDcl->getRegVar(),
                      oldSrc->getRegOff(), oldSrc->getSubRegOff(),
                      oldSrc->getRegion(), oldSrc->getType());
                  ;
                  useInst->setSrc(newSrc, pos);
                  while (aliasOldSrcDcl && aliasOldSrcDcl != oldDcl) {
                    oldSrcDcl = aliasOldSrcDcl;
                    const char *newSrcDclName = builder.getNameString(
                        16, "copy_%s", oldSrcDcl->getName());
                    newSrcDcl = builder.createDeclare(
                        newSrcDclName, G4_GRF, oldSrcDcl->getNumElems(),
                        oldSrcDcl->getNumRows(), oldSrcDcl->getElemType());
                    newSrcDcl->copyAlign(oldSrcDcl);
                    aliasOldSrcDcl = oldSrcDcl->getAliasDeclare();
                    vISA_ASSERT(aliasOldSrcDcl != NULL, "Invalid alias decl");
                    newSrcDcl->setAliasDeclare(aliasOldSrcDcl,
                                               oldSrcDcl->getAliasOffset());
                  }
                } else {
                  lastUseIt = useIt;
                }
              }
            }
          }

          // Try again
          return allocateRegs(lr, bb, builder, LLRUseMap);
        }
      }
    }

#ifdef DEBUG_VERBOSE_ON
    DEBUG_VERBOSE("Spilling range " << lr->getTopDcl()->getName() << "\n");
#endif

    // Even after spilling everything could not find an allocation
    return false;
  }
#ifdef DEBUG_VERBOSE_ON
  COUT_ERROR << lr->getTopDcl()->getName() << ":r" << regnum
             << "  BANK: " << gra.getBankConflict(lr->getTopDcl()) << "\n";
#endif

  lr->setPhyReg(builder.phyregpool.getGreg(regnum), subregnum);

  return true;
}

bool LinearScan::allocateRegsFromBanks(LocalLiveRange *lr) {
  vISA_ASSERT(!gra.use4GRFAlign, "not expecting 4GRF align");

  int regnum, subregnum;
  unsigned int tmpLocalRABound = 0;
  int nrows = 0;
  int lastUseSum1 = pregManager.getAvailableRegs()->getLastUseSum1();
  int lastUseSum2 = pregManager.getAvailableRegs()->getLastUseSum2();
  int bank1AvailableRegNum =
      pregManager.getAvailableRegs()->getBank1AvailableRegNum();
  int bank2AvailableRegNum =
      pregManager.getAvailableRegs()->getBank2AvailableRegNum();
  int size = lr->getSizeInWords();
  BankAlign align =
      gra.isEvenAligned(lr->getTopDcl()) ? BankAlign::Even : BankAlign::Either;
  G4_SubReg_Align subalign = gra.getSubRegAlign(lr->getTopDcl());
  unsigned int instID;
  lr->getFirstRef(instID);
  auto lrBC = gra.getBankConflict(lr->getTopDcl());

  // Reverse the order for FF two banks RA.
  // For FF, second bank register allocated from back to front
  if (align == BankAlign::Either && lrBC != BANK_CONFLICT_NONE) {
    switch (lrBC) {
    case BANK_CONFLICT_FIRST_HALF_EVEN:
    case BANK_CONFLICT_FIRST_HALF_ODD:
      align = BankAlign::Even;
      startGRFReg = &bank1StartGRFReg;
      tmpLocalRABound = bank1_end;
      break;
    case BANK_CONFLICT_SECOND_HALF_EVEN:
    case BANK_CONFLICT_SECOND_HALF_ODD:
      if (useRoundRobin) {
        align = BankAlign::Odd;
      }
      startGRFReg = &bank2StartGRFReg;
      tmpLocalRABound = bank2_end;
      break;
    default:
      break;
    }
  } else {
    if (useRoundRobin) {
      if (bank1AvailableRegNum == 0 && bank2AvailableRegNum == 0) {
        return false;
      }

      float bank1Average = bank1AvailableRegNum == 0
                               ? (float)0x7fffffff
                               : (float)lastUseSum1 / bank1AvailableRegNum;
      float bank2Average = bank2AvailableRegNum == 0
                               ? (float)0x7fffffff
                               : (float)lastUseSum2 / bank2AvailableRegNum;
      if (bank1Average > bank2Average) {
        startGRFReg = &bank2StartGRFReg;
        tmpLocalRABound = bank2_end;
      } else {
        startGRFReg = &bank1StartGRFReg;
        tmpLocalRABound = bank1_end;
      }
    } else {
      if (bank1AvailableRegNum < bank2AvailableRegNum) {
        startGRFReg = &bank2StartGRFReg;
        tmpLocalRABound = bank2_end;
      } else {
        startGRFReg = &bank1StartGRFReg;
        tmpLocalRABound = bank1_end;
      }
    }
  }

  if (useRoundRobin) {
    nrows = pregManager.findFreeRegs(size, align, subalign, regnum, subregnum,
                                     *startGRFReg, tmpLocalRABound, 0, instID,
                                     false, lr->getForbidden());

    if (nrows) {
      // Wrapper handling
      if (tmpLocalRABound) {
        *startGRFReg = (regnum + nrows) % (tmpLocalRABound);
      }

      if (startGRFReg == &bank2StartGRFReg) {
        if (*startGRFReg < SECOND_HALF_BANK_START_GRF) {
          *startGRFReg += bank2_start;
          if (*startGRFReg >= numRegLRA) {
            *startGRFReg = bank2_start;
            return false;
          }
        }
      }
    } else {
      // record the original endReg in case failed to allocate in bank switch
      unsigned int endReg = *startGRFReg + nrows;

      // If failed, try another bank first.
      if (startGRFReg == &bank2StartGRFReg) {
        startGRFReg = &bank1StartGRFReg;
        tmpLocalRABound = bank1_end;
      } else {
        startGRFReg = &bank2StartGRFReg;
        tmpLocalRABound = bank2_end;
      }

      nrows = pregManager.findFreeRegs(size, align, subalign, regnum, subregnum,
                                       *startGRFReg, tmpLocalRABound, 0, instID,
                                       false, lr->getForbidden());

      if (!nrows) {
        // Still fail, wrapper round in original bank
        if (startGRFReg == &bank1StartGRFReg) // switch back to before
        {
          startGRFReg = &bank2StartGRFReg;
          *startGRFReg = bank2_start;
          tmpLocalRABound = ((endReg) > (bank2_end)) ? bank2_end : endReg;
        } else {
          startGRFReg = &bank1StartGRFReg;
          *startGRFReg = bank1_start;
          tmpLocalRABound = (endReg > bank1_end) ? bank1_end : endReg;
        }
        nrows = pregManager.findFreeRegs(
            size, align, subalign, regnum, subregnum, *startGRFReg,
            tmpLocalRABound, 0, instID, false, lr->getForbidden());
      }
    }
  } else {
    nrows = pregManager.findFreeRegs(size, align, subalign, regnum, subregnum,
                                     *startGRFReg, tmpLocalRABound, 0, instID,
                                     true, lr->getForbidden());

    if (!nrows) { // Try without window, no even/odd alignment for bank, but
                  // still low and high(keep in same bank)
      nrows = pregManager.findFreeRegs(
          size,
          gra.isEvenAligned(lr->getTopDcl()) ? BankAlign::Even
                                             : BankAlign::Either,
          subalign, regnum, subregnum, *startGRFReg, tmpLocalRABound, 0, instID,
          false, lr->getForbidden());
    }

    if (!nrows) {
      // Try another bank, no even/odd alignment for bank, also no low and high
      if (startGRFReg == &bank2StartGRFReg) {
        startGRFReg = &bank1StartGRFReg;
        tmpLocalRABound = bank1_end;
      } else {
        startGRFReg = &bank2StartGRFReg;
        tmpLocalRABound = bank2_end;
      }

      nrows = pregManager.findFreeRegs(
          size,
          gra.isEvenAligned(lr->getTopDcl()) ? BankAlign::Even
                                             : BankAlign::Either,
          subalign, regnum, subregnum, *startGRFReg, tmpLocalRABound, 0, instID,
          false, lr->getForbidden());
    }
  }

  // If allocations fails, return false
  if (!nrows) {
    return false;
  }

  lr->setPhyReg(builder.phyregpool.getGreg(regnum), subregnum);

#ifdef DEBUG_VERBOSE_ON
  COUT_ERROR << lr->getTopDcl()->getName() << ":r" << regnum
             << "  BANK: " << gra.getBankConflict(lr->getTopDcl()) << "\n";
#endif
  return true;
}

// Mark physical register allocated to range lr as available
void LinearScan::freeAllocedRegs(LocalLiveRange *lr, bool setInstID) {
  int sregnum;

  G4_VarBase *preg = lr->getPhyReg(sregnum);

  vISA_ASSERT(
      preg != NULL,
      "Physical register not assigned to live range. Cannot free regs.");

  unsigned int idx = 0;
  if (setInstID) {
    lr->getLastRef(idx);
  }

  pregManager.freeRegs(preg->asGreg()->getRegNum(), sregnum,
                       lr->getSizeInWords(), idx);
}

// ********* PhyRegSummary class implementation *********

// Mark GRFs as used
void PhyRegSummary::markPhyRegs(G4_VarBase *pr, unsigned int size) {
  // Assume that pr is aligned to GRF start if it cannot fit in a single GRF
  vISA_ASSERT(pr->isGreg(), "Expecting GRF as operand");
  vISA_ASSERT(builder != nullptr, "ir builder should be set");

  int numGRFs = size / builder->numEltPerGRF<Type_UW>();
  unsigned int regnum = pr->asGreg()->getRegNum();

  if (size % builder->numEltPerGRF<Type_UW>() != 0)
    numGRFs++;

  for (int i = 0; i < numGRFs; i++) {
    GRFUsage[regnum + i] = true;
  }
}

void PhyRegSummary::printBusyRegs() {
  VISA_DEBUG_VERBOSE({
    std::cout << "GRFs used: ";
    for (int i = 0, size = static_cast<int>(GRFUsage.size()); i < size; i++) {
      if (isGRFBusy(i))
        std::cout << "r" << i << ", ";
    }
    std::cout << "\n\n";
  });
}
