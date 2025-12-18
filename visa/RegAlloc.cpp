/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "RegAlloc.h"
#include "Assertions.h"
#include "DebugInfo.h"
#include "FlowGraph.h"
#include "GraphColor.h"
#include "PointsToAnalysis.h"
#include "Timer.h"
#include "VarSplit.h"

#include <bitset>
#include <climits>
#include <cmath>
#include <fstream>
#include <optional>
#include <vector>

using namespace vISA;

#define GRAPH_COLOR

#if defined (_DEBUG) || defined(_INTERNAL)
bool vISA::RATraceFlag = false;
#endif

bool LivenessAnalysis::isLocalVar(G4_Declare *decl) const {
  if ((decl->isInput() == true &&
       !(fg.builder->getFCPatchInfo()->getFCComposableKernel() &&
         !decl->isLiveIn())) &&
      !(fg.builder->isPreDefArg(decl) &&
        (fg.builder->getIsKernel() ||
         (fg.getIsStackCallFunc() && fg.builder->getArgSize() == 0))))
    return false;

  // FIXME: Why special check? Why can't we compute this correctly?
  if (!fg.builder->canReadR0() && decl == fg.builder->getBuiltinR0())
    return false;

  if (decl->isOutput() == true &&
      !(fg.builder->isPreDefRet(decl) &&
        (fg.builder->getIsKernel() ||
         (fg.getIsStackCallFunc() && fg.builder->getRetVarSize() == 0))))
    return false;

  LocalLiveRange *dclLR = gra.getLocalLR(decl);
  if (dclLR && dclLR->isLiveRangeLocal() &&
      decl->getRegFile() != G4_INPUT) {
    return true;
  }

  // Since variable split is done only for local variables, there is no need to
  // analysis for global variables.
  if (decl->getIsSplittedDcl() || decl->getIsPartialDcl()) {
    return true;
  }

  return false;
}

bool LivenessAnalysis::setVarIDs(bool verifyRA, bool areAllPhyRegAssigned) {
  bool phyRegAssigned = areAllPhyRegAssigned;
  bool hasStackCall = fg.getHasStackCalls() || fg.getIsStackCallFunc();
  bool flag = (selectedRF & G4_GRF) &&
              !fg.builder->getOption(vISA_GlobalSendVarSplit) && !hasStackCall;

  auto handleSplitId = [&](G4_Declare *decl) {
    if (decl->getIsSplittedDcl()) {
      decl->setSplitVarStartID(0);
    }
    if (decl->getIsPartialDcl()) {
      auto declSplitDcl = gra.getSplittedDeclare(decl);
      if (declSplitDcl && declSplitDcl->getIsSplittedDcl()) {
        if (numSplitStartID == 0) {
          numSplitStartID = numVarId;
        }

        if (declSplitDcl->getSplitVarStartID() == 0) {
          declSplitDcl->setSplitVarStartID(numVarId);
        }
        numSplitVar++;
      } else {
        vISA_ASSERT_UNREACHABLE("Found child declare without parent");
      }
    }
  };

  bool incRAVerify = false;
  auto incRACheck = gra.incRA.isEnabled();
  if (incRACheck) {
    // New variables get id assignment from numVarId whereas old
    // variables reuse their ids.
    numVarId = gra.incRA.getNextVarId(selectedRF);
    incRAVerify = gra.incRA.isEnabledWithVerification();
    gra.incRA.unassignedVars.clear();
  }
  for (G4_Declare *decl : gra.kernel.Declares) {
    if (livenessCandidate(decl, verifyRA) && decl->getAliasDeclare() == NULL) {
      // Flag that indicates whether old id was assigned to current decl or
      // a fresh id is assigned in current iteration.
      bool useOldId = false;
      unsigned int varId = numVarId;
      if (incRACheck) {
        // If incremental RA is enabled then:
        // 1. First, check whether dcl had an id assigned from earlier iteration.
        //    If it did, then reuse the id.
        // 2. If dcl wasn't seen in earlier iteration then assign a fresh id to
        //    it beginning from numVarId.
        auto prevIterId = gra.incRA.getIdFromPrevIter(decl);
        if (prevIterId.first) {
          varId = prevIterId.second;
          useOldId = true;
        }
        else
          gra.incRA.recordVarId(decl, varId);
      }
      if (flag) {
        bool isLocal = isLocalVar(decl);
        if (isLocal)
          handleSplitId(decl);
        else
          globalVars.set(varId);
      } else {
        handleSplitId(decl);
        // All variables are treated as global in stack call
        globalVars.set(varId);
      }

      if(incRACheck && !gra.incRA.getLRs().empty()) {
        // Incremental RA requires that variables preserve their
        // RA id. So this sanity check is crucial for incremental
        // RA correctness.
        auto id = decl->getRegVar()->getId();
        vISA_ASSERT(id == varId || id == UNDEFINED_VAL,
                    "different id assigned to G4_RegVar");
        if (gra.incRA.getLRs().size() > id) {
          vISA_ASSERT(gra.incRA.getLRs()[id]->getDcl() == decl,
                      "some other decl found for same id");
        }
      }

      if (!useOldId) {
        // Actual id assignment to G4_Declare::G4_RegVar is done
        // only for new variables. It's assumed that older variables
        // preserve their ids.
        decl->getRegVar()->setId(varId);
      }

      // If new id was assigned then increment numVarId for next fresh variable
      if (!useOldId)
        numVarId++;
      if (!decl->getRegVar()->getPhyReg() && !decl->getIsPartialDcl()) {
        numUnassignedVarId++;

        if (incRAVerify) {
          // Store list of unassigned variables. This list is pruned later in
          // determineColorOrdering() to verify that all unallocated ranges
          // are coloring candidates. Note this is only to aid debugging and
          // verification of incremental RA.
          gra.incRA.unassignedVars.insert(decl);
        }

      }
      if (decl->getRegVar()->isPhyRegAssigned() == false) {
        phyRegAssigned = false;
      }
    } else {
      decl->getRegVar()->setId(UNDEFINED_VAL);
    }
  }

  return phyRegAssigned;
}

LivenessAnalysis::LivenessAnalysis(GlobalRA &g, unsigned char kind,
                                   bool verifyRA, bool forceRun)
    : selectedRF(kind), pointsToAnalysis(g.pointsToAnalysis), gra(g),
      fg(g.kernel.fg) {
  //
  // NOTE:
  // The maydef sets are simply aliases to the mayuse sets, since their uses are
  // mutually exclusive.
  //
  // Go over each reg var if it's a liveness candidate, assign id for bitset.
  //
  bool areAllPhyRegAssigned = !forceRun;

  areAllPhyRegAssigned = setVarIDs(verifyRA, areAllPhyRegAssigned);

  // For Alias Dcl
  for (auto decl : gra.kernel.Declares) {
    if (livenessCandidate(decl, verifyRA) && decl->getAliasDeclare() != NULL) {
      // It is an alias declaration. Set its id = base declaration id
      decl->getRegVar()->setId(decl->getAliasDeclare()->getRegVar()->getId());
    }
    VISA_DEBUG_VERBOSE({
      decl->getRegVar()->emit(std::cout);
      std::cout << " id = " << decl->getRegVar()->getId() << "\n";
    });
  }

  //
  // if no chosen candidate for reg allocation return
  //
  if (numVarId == 0 || (verifyRA == false && areAllPhyRegAssigned == true)) {
    // If all variables have physical register assignments
    // there are no candidates for allocation
    numVarId = 0;
    return;
  }

  //
  // put selected reg vars into vars[]
  //
  vars.resize(numVarId);
  for (auto dcl : gra.kernel.Declares) {
    if (livenessCandidate(dcl, verifyRA) && dcl->getAliasDeclare() == NULL) {
      G4_RegVar *var = dcl->getRegVar();
      vars[var->getId()] = var;
    }
  }

  addr_taken = BitSet(numVarId, false);

  numBBId = (unsigned)fg.size();

  def_in.resize(numBBId);
  def_out.resize(numBBId);
  use_in.resize(numBBId);
  use_out.resize(numBBId);
  use_gen.resize(numBBId);
  use_kill.resize(numBBId);
  indr_use.resize(numBBId);
}

LivenessAnalysis::~LivenessAnalysis() {
  //
  // if no chosen candidate for reg allocation return
  //
  if (numVarId == 0) {
    return;
  }

  // Remove liveness inserted pseudo kills
  for (auto bb : fg) {
    for (auto instIt = bb->begin(); instIt != bb->end();) {
      auto inst = (*instIt);
      if (inst->isPseudoKill()) {
        auto src0 = inst->getSrc(0);
        vISA_ASSERT(src0 && src0->isImm(),
                    "expecting src0 immediate for pseudo kill");
        if (src0->asImm()->getImm() ==
            static_cast<int64_t>(PseudoKillType::FromLiveness)) {
          instIt = bb->erase(instIt);
          continue;
        }
      }
      ++instIt;
    }
  }
}

bool LivenessAnalysis::livenessCandidate(const G4_Declare *decl,
                                         bool verifyRA) const {
  const LocalLiveRange *declLR = nullptr;
  if (verifyRA == false && (declLR = gra.getLocalLR(decl)) &&
      declLR->getAssigned() && !declLR->isEOT()) {
    return false;
  } else if ((selectedRF & decl->getRegFile())) {
    if (selectedRF & G4_GRF) {
      if ((decl->getRegFile() & G4_INPUT) &&
          decl->getRegVar()->isPhyRegAssigned() &&
          !decl->getRegVar()->isGreg()) {
        return false;
      }
      if (decl->getByteSize() == 0) {
        // regrettably, this can happen for arg/retval pre-defined variable
        return false;
      }
    }
    return true;
  } else {
    return false;
  }
}

void LivenessAnalysis::updateKillSetForDcl(
    G4_Declare *dcl, SparseBitVector *curBBGen, SparseBitVector *curBBKill,
    G4_BB *curBB, SparseBitVector *entryBBGen, SparseBitVector *entryBBKill,
    G4_BB *entryBB, unsigned scopeID) {
  if (scopeID != 0 && scopeID != UINT_MAX && dcl->getScopeID() == scopeID) {
    entryBBKill->set(dcl->getRegVar()->getId());
    entryBBGen->reset(dcl->getRegVar()->getId());
    VISA_DEBUG_VERBOSE(std::cout
                       << "Killed sub-routine scope " << dcl->getName()
                       << " at bb with id = " << entryBB->getId() << "\n");
  }
}

// Scoping info is stored per decl. A variable can be either global scope
// (default), sub-routine scope, or basic block scope. This function iterates
// over all instructions and their operands in curBB and if scoping for it is
// set in symbol table then it marks kills accordingly. A bb scope variable is
// killed in the bb it appears and a sub-routine local variable is killed in
// entry block of the sub-routine. No error check is performed currently so if
// variable scoping information is incorrect then generated code will be so too.
void LivenessAnalysis::performScoping(SparseBitVector *curBBGen,
                                      SparseBitVector *curBBKill, G4_BB *curBB,
                                      SparseBitVector *entryBBGen,
                                      SparseBitVector *entryBBKill,
                                      G4_BB *entryBB) {
  unsigned scopeID = curBB->getScopeID();
  for (G4_INST *inst : *curBB) {
    G4_DstRegRegion *dst = inst->getDst();

    if (dst && dst->getBase()->isRegAllocPartaker()) {
      G4_Declare *dcl = GetTopDclFromRegRegion(dst);
      updateKillSetForDcl(dcl, curBBGen, curBBKill, curBB, entryBBGen,
                          entryBBKill, entryBB, scopeID);
    }

    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
      G4_Operand *src = inst->getSrc(i);
      if (src) {
        if (src->isSrcRegRegion() &&
            src->asSrcRegRegion()->getBase()->isRegAllocPartaker()) {
          G4_Declare *dcl = GetTopDclFromRegRegion(src);
          updateKillSetForDcl(dcl, curBBGen, curBBKill, curBB, entryBBGen,
                              entryBBKill, entryBB, scopeID);
        } else if (src->isAddrExp() &&
                   src->asAddrExp()->getRegVar()->isRegAllocPartaker()) {
          G4_Declare *dcl =
              src->asAddrExp()->getRegVar()->getDeclare()->getRootDeclare();

          updateKillSetForDcl(dcl, curBBGen, curBBKill, curBB, entryBBGen,
                              entryBBKill, entryBB, scopeID);
        }
      }
    }
  }
}

void LivenessAnalysis::detectNeverDefinedVarRows() {
  // This function records variables and its rows that are never defined
  // in the kernel. This information helps detect kills for partial
  // writes when VISA optimizer optimizes away some rows of a variable.
  // In interest of compile time we only look for full rows that are
  // not defined rather than sub-regs.
  std::unordered_map<G4_Declare *, BitSet> largeDefs;

  // Populate largeDefs map with dcls > 1 GRF size
  for (auto dcl : gra.kernel.Declares) {
    if (dcl->getAliasDeclare() || dcl->getIsPartialDcl() || dcl->getAddressed())
      continue;

    if (dcl->getRegFile() != G4_GRF)
      continue;

    unsigned int dclNumRows = dcl->getNumRows();

    if (dclNumRows < 2)
      continue;

    BitSet bitset(dclNumRows, false);

    largeDefs.insert(std::make_pair(dcl, std::move(bitset)));
  }

  if (largeDefs.empty())
    return;

  const unsigned bytesPerGRF = fg.builder->numEltPerGRF<Type_UB>();

  // Update row usage of each dcl in largeDefs
  for (auto bb : gra.kernel.fg) {
    for (auto inst : *bb) {
      auto dst = inst->getDst();
      if (!dst)
        continue;

      auto dstTopDcl = dst->getTopDcl();

      if (dstTopDcl) {
        auto it = largeDefs.find(dstTopDcl);

        if (it == largeDefs.end()) {
          continue;
        }

        unsigned int rowStart = dst->getLeftBound() / bytesPerGRF;
        unsigned int rowEnd = dst->getRightBound() / bytesPerGRF;

        it->second.set(rowStart, rowEnd);
      }
    }
  }

  // Propagate largeDefs to neverDefinedRows bit vector to later bitwise OR it
  for (const auto &it : largeDefs) {
    unsigned int numRows = it.first->getNumRows();
    BitSet *undefinedRows = nullptr;
    for (unsigned int i = 0; i < numRows; i++) {
      if (!it.second.isSet(i)) {
        if (undefinedRows == nullptr) {
          undefinedRows =
              &neverDefinedRows
                   .emplace(it.first, BitSet(it.first->getByteSize(), false))
                   .first->second;
        }
        undefinedRows->set(i * bytesPerGRF, i * bytesPerGRF + bytesPerGRF - 1);
      }
    }
  }
}

// #1 - (P166) sel (M1, 8) V2(0,0)<1> 0x1:d 0x0:d
// #2 - (W) mov(M1, 8) V1(0,0)<1> 0x0:ud
// #3 - (W) mov (M1, 1) V3 0x1:ud
// #4 - add (M1, 8) V1(0,0)<1> V2(0,0)<1;1,0> V3(0,0)<0;1,0>
//
// In above 3d program, V1 is written once using WriteEnable and and once without
// WriteEnable. A var must be treated as KILLED only for widest def. In this case,
// widest def of V1 is one with WriteEnable set. Although def of V1 in inst# 4
// writes all enabled channels, it may not be as wide as WriteEnable in inst#1.
// Marking #4 as KILL allows RA to use same register for V1 and V3 which can be
// incorrect if all but lowest channel are enabled. Because in this case lowest
// channel data would be overwritten by V3 def.
//
// In this method, we gather all variables written with WriteEnable and refer to
// this list when computing pseudo_kill. Note that WriteEnable takes effect only
// in divergent context. In other words, for ESIMD target, all channels are enabled
// at thread dispatch. Whereas for 3d, even entry BB is considered to be divergent.
void LivenessAnalysis::collectNoMaskVars() {
  for (auto bb : gra.kernel.fg) {
    for (auto inst : *bb) {
      if (!inst->isWriteEnableInst())
        continue;

      auto inDivergentContext = [&]() {
        // 3d is always in divergent context
        // ESIMD is divergent only in BBs that are explicitly divergent
        return fg.getKernel()->getInt32KernelAttr(Attributes::ATTR_Target) ==
                   VISA_3D ||
               !bb->isAllLaneActive();
      };

      G4_Operand *opnd = nullptr;
      if (livenessClass(G4_GRF))
        opnd = inst->getDst();
      else if (livenessClass(G4_FLAG))
        opnd = inst->getCondMod();

      if (opnd && opnd->getBase() && opnd->getBase()->isRegAllocPartaker() &&
          inDivergentContext()) {
        defWriteEnable.insert(opnd->getTopDcl());
      }
    }
  }
}

bool LivenessAnalysis::doesVarNeedNoMaskForKill(const G4_Declare* dcl) const {
  return defWriteEnable.count(dcl) == 0 ? false : true;
}

//
// compute liveness of reg vars
// Each reg var indicates a region within the register file. As such, the case
// in which two consecutive defs of a reg region without any use in between does
// not mean the second def overwrites the first one because the two defs may
// write different parts of the region. Def vectors are used to track which
// definitions of reg vars reach the entry and the end of a basic block, which
// tell us the first definitions of reg vars. Use vectors track which uses of
// reg vars are anticipated, which tell use the uses of reg vars.Def and Use
// vectors encapsulate the liveness of reg vars.
//
void LivenessAnalysis::computeLiveness() {
  //
  // no reg var is selected, then no need to compute liveness
  //
  if (getNumSelectedVar() == 0) {
    return;
  }

  startTimer(TimerID::LIVENESS);

  //
  // mark input arguments live at the entry of kernel
  // mark output arguments live at the exit of kernel
  //
  SparseBitVector inputDefs;
  SparseBitVector outputUses;

  for (unsigned i = 0; i < numVarId; i++) {
    bool setLiveIn = false;
    if (!vars[i])
      continue;

    G4_Declare *decl = vars[i]->getDeclare();

    if ((decl->isInput() == true &&
         !(fg.builder->getFCPatchInfo()->getFCComposableKernel() &&
           !decl->isLiveIn())) &&
        !(fg.builder->isPreDefArg(decl) &&
          (fg.builder->getIsKernel() ||
           (fg.getIsStackCallFunc() && fg.builder->getArgSize() == 0))))
      setLiveIn = true;

    // FIXME: Why make this liveIn? we are copying RealR0 to BuiltInR0 at kernel
    // entry, so by definition it's not live-in.
    if (!fg.builder->canReadR0() && decl == fg.builder->getBuiltinR0())
      setLiveIn = true;

    if (setLiveIn) {
      inputDefs.set(i);
      VISA_DEBUG_VERBOSE(std::cout << "First def input = " << decl->getName()
                                   << "\n");
    }

    bool setLiveOut = false;
    if (decl->isOutput() == true &&
        !(fg.builder->isPreDefRet(decl) &&
          (fg.builder->getIsKernel() ||
           (fg.getIsStackCallFunc() && fg.builder->getRetVarSize() == 0))))
      setLiveOut = true;

    // FIXME: Why do we need to force builtinR0 to be liveOut?
    if (!fg.builder->canReadR0() && decl == fg.builder->getBuiltinR0())
      setLiveOut = true;

    if (setLiveOut) {
      outputUses.set(i);
      VISA_DEBUG_VERBOSE(
          std::cout << "First def output    = " << decl->getName() << "\n");
    }
  }

  //
  // clean up def_in & def_out that are used in markFirstDef
  //
  for (unsigned i = 0; i < numBBId; i++) {
    def_in[i].clear();
    def_out[i].clear();
  }

  collectNoMaskVars();
  if (livenessClass(G4_GRF))
    detectNeverDefinedVarRows();

  //
  // compute def_out and use_in vectors for each BB
  //
  for (G4_BB *bb : fg) {
    unsigned id = bb->getId();

    computeGenKillandPseudoKill(bb, def_out[id], use_in[id], use_gen[id],
                                use_kill[id]);

    //
    // exit block: mark output parameters live
    //
    if (bb->Succs.empty()) {
      use_out[id] = outputUses;
    }
  }

  G4_BB *subEntryBB = NULL;
  SparseBitVector *subEntryKill = NULL;
  SparseBitVector *subEntryGen = NULL;

  if (fg.getKernel()->getInt32KernelAttr(Attributes::ATTR_Target) == VISA_CM) {
    //
    // Top-down order of BB list iteration guarantees that
    // entry BB of each sub-routine will be seen before any other
    // BBs belonging to that sub-routine. This assumes that BBs of
    // a sub-routine are laid out back to back in bb list.
    //
    for (auto bb : fg) {
      unsigned id = bb->getId();

      if (bb->getScopeID() != 0 && bb->getScopeID() != UINT_MAX) {
        subEntryBB = fg.sortedFuncTable[bb->getScopeID() - 1]->getInitBB();
        unsigned entryBBID = subEntryBB->getId();
        subEntryKill = &use_kill[entryBBID];
        subEntryGen = &use_gen[entryBBID];
      }

      //
      // Mark explicitly scoped variables as kills
      //
      performScoping(&use_gen[id], &use_kill[id], bb, subEntryGen, subEntryKill,
                     subEntryBB);
    }
  }

  //
  // compute indr accesses
  //
  if (selectedRF & G4_GRF) {
    // only GRF variables can have their address taken
    for (auto bb : fg) {
      const REGVAR_VECTOR &grfVec =
          pointsToAnalysis.getIndrUseVectorForBB(bb->getId());
      for (const pointInfo addrTaken : grfVec) {
        indr_use[bb->getId()].set(addrTaken.var->getId());
        addr_taken.set(addrTaken.var->getId(), true);
      }
    }
  }
  //
  // Perform inter-procedural context-sensitive flow analysis.
  // This is required when the CFG involves function calls with multiple calling
  // contexts for the same function, as peforming just a context-insensitive
  // analysis results in uses being propgated along paths that are not feasible
  // in the actual program.
  //
  if (performIPA()) {
    hierarchicalIPA(inputDefs, outputUses);
    stopTimer(TimerID::LIVENESS);
    return;
  }

  if (fg.getKernel()->getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D &&
      (selectedRF & G4_GRF || selectedRF & G4_FLAG) && (numFnId > 0)) {
    // compute the maydef for each subroutine
    maydefAnalysis();
  }

  auto getPostOrder = [](G4_BB *S, std::vector<G4_BB *> &PO) {
    std::stack<std::pair<G4_BB *, BB_LIST_ITER>> Stack;
    std::set<G4_BB *> Visited;

    Stack.push({S, S->Succs.begin()});
    Visited.insert(S);
    while (!Stack.empty()) {
      G4_BB *Curr = Stack.top().first;
      BB_LIST_ITER It = Stack.top().second;

      if (It != Curr->Succs.end()) {
        G4_BB *Child = *Stack.top().second++;
        if (Visited.insert(Child).second) {
          Stack.push({Child, Child->Succs.begin()});
        }
        continue;
      }
      PO.push_back(Curr);
      Stack.pop();
    }
  };

  std::vector<G4_BB *> PO;
  getPostOrder(fg.getEntryBB(), PO);

  bool change;

  //
  // backward flow analysis to propagate uses (locate last uses)
  //
  do {
    change = false;
    for (auto I = PO.begin(), E = PO.end(); I != E; ++I)
      change |= contextFreeUseAnalyze(*I, change);
  } while (change);

  //
  // initialize entry block with payload input
  //
  def_in[fg.getEntryBB()->getId()] = std::move(inputDefs);

  //
  // forward flow analysis to propagate defs (locate first defs)
  //
  do {
    change = false;
    for (auto I = PO.rbegin(), E = PO.rend(); I != E; ++I)
      change |= contextFreeDefAnalyze(*I, change);
  } while (change);

  //
  // dump vectors for debugging
  //
#if 0
    {
        dump_bb_vector("DEF IN", def_in);
        dump_bb_vector("DEF OUT", def_out);
        dump_bb_vector("USE IN", use_in);
        dump_bb_vector("USE OUT", use_out);
    }
#endif

  stopTimer(TimerID::LIVENESS);
}

//
// compute the maydef set for every subroutine
// This includes recursively all the variables that are defined by the
// subroutine, but does not include defs in the caller
// This means this must be called before we do fix-point on def_in/def_out
// and destroy their original values
// This is used by augmentation later to model all variables that may be defined
// by a call
// FIXME: we should use a separate def set to represent declares defined in each
// BB
//
void LivenessAnalysis::maydefAnalysis() {
  for (auto func : fg.sortedFuncTable) {
    unsigned fid = func->getId();
    if (fid == UINT_MAX) {
      // entry kernel
      continue;
    }

    auto &BV = subroutineMaydef[func];
    for (auto &&bb : func->getBBList()) {
      BV |= def_out[bb->getId()];
    }
    for (auto &&callee : func->getCallees()) {
      BV |= subroutineMaydef[callee];
    }
  }
}

//
// Use analysis for this subroutine only
// use_out[call-BB] = use_in[ret-BB]
// use_out[exit-BB] should be initialized by the caller
//
void LivenessAnalysis::useAnalysis(FuncInfo *subroutine) {
  bool changed = false;
  do {
    changed = false;
    for (auto BI = subroutine->getBBList().rbegin(),
              BE = subroutine->getBBList().rend();
         BI != BE; ++BI) {
      //
      // use_out = use_in(s1) + use_in(s2) + ...
      // where s1 s2 ... are the successors of bb
      // use_in  = use_gen + (use_out - use_kill)
      //
      G4_BB *bb = *BI;
      unsigned bbid = bb->getId();
      if (bb->getBBType() & G4_BB_EXIT_TYPE) {
        // use_out is set by caller
      } else if (bb->getBBType() & G4_BB_CALL_TYPE) {
        use_out[bbid] |= use_in[bb->getPhysicalSucc()->getId()];
      } else {
        for (auto succ : bb->Succs) {
          use_out[bbid] |= use_in[succ->getId()];
        }
      }

      if (changed) {
        // no need to update changed, save a copy
        use_in[bbid] = use_out[bbid];
        use_in[bbid] = use_in[bbid] - use_kill[bbid];
        use_in[bbid] |= use_gen[bbid];
      } else {
        SparseBitVector oldUseIn = use_in[bbid];

        use_in[bbid] = use_out[bbid];
        use_in[bbid] = use_in[bbid] - use_kill[bbid];
        use_in[bbid] |= use_gen[bbid];

        if (!(bb->getBBType() & G4_BB_INIT_TYPE) && oldUseIn != use_in[bbid]) {
          changed = true;
        }
      }
    }
  } while (changed);
}

//
// Use analysis for this subroutine only, considering both arg/retval of its
// callees use_out[call-BB] = (use_in[ret-BB] | arg[callee]) - retval[callee]
//
void LivenessAnalysis::useAnalysisWithArgRetVal(
    FuncInfo *subroutine,
    const std::unordered_map<FuncInfo *, SparseBitVector> &args,
    const std::unordered_map<FuncInfo *, SparseBitVector> &retVal) {
  bool changed = false;
  do {
    changed = false;
    for (auto BI = subroutine->getBBList().rbegin(),
              BE = subroutine->getBBList().rend();
         BI != BE; ++BI) {
      //
      // use_out = use_in(s1) + use_in(s2) + ...
      // where s1 s2 ... are the successors of bb
      // use_in  = use_gen + (use_out - use_kill)
      //
      G4_BB *bb = *BI;
      unsigned bbid = bb->getId();
      if (bb->getBBType() & G4_BB_EXIT_TYPE) {
        // use_out is set by previous analysis
      } else if (bb->getBBType() & G4_BB_CALL_TYPE) {
        use_out[bbid] = use_in[bb->getPhysicalSucc()->getId()];
        auto callee = bb->getCalleeInfo();
        auto BVIt = args.find(callee);
        vISA_ASSERT(BVIt != args.end(), "Missing entry in map");
        use_out[bbid] |= (*BVIt).second;
        BVIt = retVal.find(callee);
        vISA_ASSERT(BVIt != retVal.end(), "Missing entry in map");
        use_out[bbid] = use_out[bbid] - (*BVIt).second;
      } else {
        for (auto succ : bb->Succs) {
          use_out[bbid] |= use_in[succ->getId()];
        }
      }

      if (changed) {
        // no need to update changed, save a copy
        use_in[bbid] = use_out[bbid];
        use_in[bbid] = use_in[bbid] - use_kill[bbid];
        use_in[bbid] |= use_gen[bbid];
      } else {
        SparseBitVector oldUseIn = use_in[bbid];

        use_in[bbid] = use_out[bbid];
        use_in[bbid] = use_in[bbid] - use_kill[bbid];
        use_in[bbid] |= use_gen[bbid];

        if (!(bb->getBBType() & G4_BB_INIT_TYPE) && oldUseIn != use_in[bbid]) {
          changed = true;
        }
      }
    }
  } while (changed);
}

//
// Def analysis for each subroutine only
// at a call site, we do
// def_in[ret-BB] = def_out[call-BB] U def_out[callee exit-BB]
// callee's def_in/def_out is not modified
//
void LivenessAnalysis::defAnalysis(FuncInfo *subroutine) {

  // def_in[bb] = null (inputs for entry BB)
  // def_out[bb] is initialized to all defs in the bb
  bool changed = false;
  do {
    changed = false;
    for (auto &&bb : subroutine->getBBList()) {
      uint32_t bbid = bb->getId();
      std::optional<SparseBitVector> defInOrNull = std::nullopt;
      if (!changed) {
        defInOrNull = def_in[bbid];
      }
      auto phyPredBB =
          (bb == fg.getEntryBB()) ? nullptr : bb->getPhysicalPred();
      if (phyPredBB && (phyPredBB->getBBType() & G4_BB_CALL_TYPE)) {
        // this is the return BB, we take the def_out of the callBB + the
        // predecessors
        G4_BB *callBB = bb->getPhysicalPred();
        def_in[bbid] |= def_out[callBB->getId()];
        for (auto &&pred : bb->Preds) {
          def_in[bbid] |= def_out[pred->getId()];
        }
      } else if (bb->getBBType() & G4_BB_INIT_TYPE) {
        // do nothing as we don't want to propagate caller defs yet
      } else {
        for (auto &&pred : bb->Preds) {
          def_in[bbid] |= def_out[pred->getId()];
        }
      }

      if (!changed) {
        if (def_in[bbid] != defInOrNull.value()) {
          changed = true;
        }
      }
      def_out[bbid] |= def_in[bbid];
    }
  } while (changed);
}

void LivenessAnalysis::hierarchicalIPA(const SparseBitVector &kernelInput,
                                       const SparseBitVector &kernelOutput) {

  vISA_ASSERT(fg.sortedFuncTable.size() > 0,
              "topological sort must already be performed");

#ifdef _DEBUG
  auto verifyFuncTable = [&]() {
    auto accountedBBs = 0;
    for (auto &sub : fg.sortedFuncTable) {
      accountedBBs += sub->getBBList().size();
    }
    vISA_ASSERT(fg.getBBList().size() == accountedBBs, "unaccounted bbs");
  };
  verifyFuncTable();
#endif

  auto initKernelLiveOut = [this, &kernelOutput]() {
    for (auto &&bb : fg.kernelInfo->getBBList()) {
      if (bb->Succs.empty()) {
        // EOT BB
        use_out[bb->getId()] = kernelOutput;
      }
    }
  };
  // reset all live-in/out sets except for the kernel live-out
  auto clearLiveSets = [this]() {
    for (auto subroutine : fg.sortedFuncTable) {
      for (auto bb : subroutine->getBBList()) {
        use_in[bb->getId()].clear();
        use_out[bb->getId()].clear();
      }
    }
  };

  // top-down traversal to compute retval for each subroutine
  // retval[s] = live_out[s] - live_in[s],
  // where live_out[s] is the union of the live-in of the ret BB at each call
  // site (hence top-down traversal). this is not entirely accurate since we may
  // have pass-through retVals (e.g., A call B call C, C's retVal is
  // pass-through in B and used in A, which
  //  means it won't be killed in B if we do top-down)
  // But for now let's trade some loss of accuracy to save one more round of
  // fix-point
  initKernelLiveOut();
  for (auto FI = fg.sortedFuncTable.rbegin(), FE = fg.sortedFuncTable.rend();
       FI != FE; ++FI) {
    auto subroutine = *FI;
    useAnalysis(subroutine);
    if (subroutine != fg.kernelInfo) {
      retVal[subroutine] = use_out[subroutine->getExitBB()->getId()];
      retVal[subroutine] =
          retVal[subroutine] - use_in[subroutine->getInitBB()->getId()];
    }
    for (auto &&bb : subroutine->getBBList()) {
      if (bb->getBBType() & G4_BB_CALL_TYPE) {
        G4_BB *retBB = bb->getPhysicalSucc();
        G4_BB *exitBB = bb->getCalleeInfo()->getExitBB();
        vISA_ASSERT((exitBB->getBBType() & G4_BB_EXIT_TYPE),
                    "should be a subroutine's exit BB");
        use_out[exitBB->getId()] |= use_in[retBB->getId()];
      }
    }
  }

  // bottom-up traversal to compute arg for each subroutine
  // arg[s] = live-in[s], except retval of its callees are excluded as by
  // definition they will not be live-in The live-out of each subroutine is
  // initialized to null so that args are limited to variables actually used in
  // this subroutine (and its callees)
  clearLiveSets();
  initKernelLiveOut();
  for (auto subroutine : fg.sortedFuncTable) {
    useAnalysisWithArgRetVal(subroutine, args, retVal);
    if (subroutine != fg.kernelInfo) {
      args[subroutine] = use_in[subroutine->getInitBB()->getId()];
      args[subroutine] =
          args[subroutine] - use_out[subroutine->getExitBB()->getId()];
    }
  }

  // the real deal -- top-down traversal taking arg/retval/live-through all into
  // consideration again top-down traversal is needed to compute the live-out of
  // each subroutine.
  clearLiveSets();
  initKernelLiveOut();
  for (auto FI = fg.sortedFuncTable.rbegin(), FE = fg.sortedFuncTable.rend();
       FI != FE; ++FI) {
    auto subroutine = *FI;
    useAnalysisWithArgRetVal(subroutine, args, retVal);
    for (auto &&bb : subroutine->getBBList()) {
      if (bb->getBBType() & G4_BB_CALL_TYPE) {
        G4_BB *retBB = bb->getPhysicalSucc();
        G4_BB *exitBB = bb->getCalleeInfo()->getExitBB();
        vISA_ASSERT((exitBB->getBBType() & G4_BB_EXIT_TYPE),
                    "should be a subroutine's exit BB");
        use_out[exitBB->getId()] |= use_in[retBB->getId()];
      }
    }
  }

  maydefAnalysis(); // must be done before defAnalysis!

  // algorithm sketch for def-in/def-out:
  // In reverse topological order :
  //  -- Run def analysis on subroutine
  //  -- at each call site:
  //       def_in[ret-BB] |= def_out[call-BB] U def_out[exit-BB]
  // In topological order :
  //  -- At each call site:
  //       add def_out[call-BB] to all of callee's BBs
  def_in[fg.getEntryBB()->getId()] = kernelInput;
  for (auto subroutine : fg.sortedFuncTable) {
    defAnalysis(subroutine);
  }

  // FIXME: I assume we consider all caller's defs to be callee's defs too?
  for (auto FI = fg.sortedFuncTable.rbegin(), FE = fg.sortedFuncTable.rend();
       FI != FE; ++FI) {
    auto subroutine = *FI;
    if (subroutine->getCallees().size() == 0) {
      continue;
    }
    for (auto &&bb : subroutine->getBBList()) {
      if (bb->getBBType() & G4_BB_CALL_TYPE) {
        auto callee = bb->getCalleeInfo();
        for (auto &&calleeBB : callee->getBBList()) {
          def_in[calleeBB->getId()] |= def_out[bb->getId()];
          def_out[calleeBB->getId()] |= def_out[bb->getId()];
        }
      }
    }
  }
}

//
// determine if the dst writes the whole region of target declare
//
bool LivenessAnalysis::writeWholeRegion(const G4_BB *bb, const G4_INST *inst,
                                        G4_DstRegRegion *dst) const {
  unsigned execSize = inst->getExecSize();
  vISA_ASSERT(dst->getBase()->isRegVar(), ERROR_REGALLOC);

  if (!bb->isAllLaneActive() && !inst->isWriteEnableInst() &&
      fg.getKernel()->getInt32KernelAttr(Attributes::ATTR_Target) != VISA_3D) {
    // conservatively assume non-nomask instructions in simd control flow
    // may not write the whole region
    return false;
  }

  if (inst->getPredicate()) {
    return false;
  }

  if (inst->isFCall())
    return true;

  // Flags may be partially written when used as the destination
  // e.g., setp (M5_NM, 16) P11 V97(8,0)<0;1,0>
  // It can be only considered as a complete kill
  // if the computed bound diff matches with the number of flag elements
  if (dst->isFlag() == true) {
    if ((dst->getRightBound() - dst->getLeftBound() + 1) ==
        dst->getBase()->asRegVar()->getDeclare()->getNumberFlagElements()) {
      return true;
    } else {
      return false;
    }
  }

  //
  // Find Primary Variable Declare
  //

  const G4_Declare *decl = ((const G4_RegVar *)dst->getBase())->getDeclare();
  const G4_Declare *primaryDcl = decl->getRootDeclare();

  //
  //  Cannot write whole register if
  //     * alias offset in non zero
  //     * reg or sub-reg offset is non zero
  //     * horiz stride is non zero
  //     * predicate is non null
  //
  if (decl->getAliasOffset() != 0 || dst->getRegAccess() != Direct ||
      dst->getRegOff() != 0 || dst->getSubRegOff() != 0 ||
      dst->getHorzStride() != 1 || inst->isPartialWrite()) {
    return false;
  }

  //
  // For CISA3, pseudo-callee-save and pseudo-caller-save insts
  // are kills
  //
  if (fg.isPseudoDcl(primaryDcl)) {
    return true;
  }

  //
  // If the region does not cover the whole declare then it does not write the
  // whole region.
  //
  if ((dst->getTypeSize() * execSize != primaryDcl->getElemSize() *
                                            primaryDcl->getNumElems() *
                                            primaryDcl->getNumRows())) {
    return false;
  }

  if (doesVarNeedNoMaskForKill(primaryDcl) && !inst->isWriteEnableInst())
    return false;

  return true;
}

//
// determine if the dst writes the whole region of target declare
//
bool LivenessAnalysis::writeWholeRegion(const G4_BB *bb, const G4_INST *inst,
                                        const G4_VarBase *flagReg) const {
  if (!bb->isAllLaneActive() && !inst->isWriteEnableInst() &&
      gra.kernel.getKernelType() != VISA_3D) {
    // conservatively assume non-nomask instructions in simd control flow
    // may not write the whole region
    return false;
  }

  const G4_Declare *decl = flagReg->asRegVar()->getDeclare();
  if (inst->getExecSize() != G4_ExecSize(decl->getNumberFlagElements())) {
    return false;
  }

  return true;
}

// Set bits in dst footprint based on dst region's left/right bound
void LivenessAnalysis::footprintDst(const G4_BB *bb, const G4_INST *i,
                                    G4_Operand *opnd,
                                    BitSet *dstfootprint) const {
  if (dstfootprint && !(i->isPartialWrite()) &&
      ((bb->isAllLaneActive() || i->isWriteEnableInst() == true) ||
       (gra.kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D &&
        !doesVarNeedNoMaskForKill(opnd->getTopDcl())))) {
    // Bitwise OR left-bound/right-bound with dst footprint to indicate
    // bytes that are written in to
    opnd->updateFootPrint(*dstfootprint, true, *fg.builder);
  }
}

// Reset bits in srcfootprint based on src region's left/right bound
void LivenessAnalysis::footprintSrc(const G4_INST *i, G4_Operand *opnd,
                                    BitSet *srcfootprint) {
  // Reset bits in kill map footprint
  opnd->updateFootPrint(*srcfootprint, false, i->getBuilder());
}

void LivenessAnalysis::computeGenKillandPseudoKill(
    G4_BB *bb, SparseBitVector &def_out, SparseBitVector &use_in,
    SparseBitVector &use_gen, SparseBitVector &use_kill) const {
  //
  // Mark each fcall as using all globals and arg pre-defined var
  //
  if (bb->isEndWithFCall() && (selectedRF & G4_GRF)) {
    const G4_Declare *arg = fg.builder->getStackCallArg();
    const G4_Declare *ret = fg.builder->getStackCallRet();

    auto fcall = fg.builder->getFcallInfo(bb->back());
    vISA_ASSERT(fcall != std::nullopt, "fcall info not found");

    if (arg->getByteSize() != 0) {
      // arg var is a use and a kill at each fcall
      if (fcall->getArgSize() != 0) {
        use_gen.set(arg->getRegVar()->getId());
      }
      use_kill.set(arg->getRegVar()->getId());
    }

    if (ret->getByteSize() != 0) {
      // ret var is a kill at each fcall
      use_kill.set(ret->getRegVar()->getId());
      if (fcall->getRetSize() != 0) {
        def_out.set(ret->getRegVar()->getId());
      }
    }
  }

  std::map<unsigned, BitSet> footprints;
  std::vector<std::pair<G4_Declare *, INST_LIST_RITER>> pseudoKills;
  std::map<G4_Declare *, INST_LIST_RITER> pseudoKillsForSpills;

  for (INST_LIST::reverse_iterator rit = bb->rbegin(), rend = bb->rend();
       rit != rend; ++rit) {
    G4_INST *i = (*rit);
    if (i->isLifeTimeEnd()) {
      continue;
    }

    G4_DstRegRegion *dst = i->getDst();
    if (dst) {
      G4_DstRegRegion *dstrgn = dst;

      if (dstrgn->getBase()->isRegAllocPartaker()) {
        G4_Declare *topdcl = GetTopDclFromRegRegion(dstrgn);
        unsigned id = topdcl->getRegVar()->getId();
        if (i->isPseudoKill()) {
          // Mark kill, reset gen
          use_kill.set(id);
          use_gen.reset(id);

          continue;
        }

        BitSet *dstfootprint = &footprints[id];

        if (dstfootprint->getSize() == 0) {
          // Write for dst was not seen before, so insert in to map
          // bitsetSize is in bytes
          unsigned int bitsetSize = dstrgn->isFlag()
                                        ? topdcl->getNumberFlagElements()
                                        : topdcl->getByteSize();

          BitSet newBitSet(bitsetSize, false);

          auto it = neverDefinedRows.find(topdcl);
          if (it != neverDefinedRows.end()) {
            // Bitwise OR new bitset with never defined rows
            newBitSet |= it->second;
          }

          footprints[id] = std::move(newBitSet);
          if (gra.isBlockLocal(topdcl) && topdcl->getAddressed() == false &&
              dstrgn->getRegAccess() == Direct) {
            // Local live ranges are never live-out of the only
            // basic block they are defined. So in top-down order
            // the first lexical definition is a kill irrespective
            // of the footprint. In cases when local live-range
            // def and use have h-stride != 1, the footprint at this
            // lexically first definition will not have all bits set.
            // This prevents that def to be seen as a kill. A simple
            // solution to this is to set all bits when initializing
            // the bitvector while iterating in bottom-up order. As
            // we traverse further up uses will reset bits and defs
            // will set bits. So when we encounter the lexically first
            // def, we will be guaranteed to find all bits set, thus
            // interpreting that def as a kill.
            dstfootprint->setAll();
          }
        }

        if (dstrgn->getRegAccess() == Direct) {
          def_out.set(id);
          //
          // if the inst writes the whole region the var declared, we set
          // use_kill so that use of var will not pass through (i.e., var's
          // interval starts at this instruction.
          //
          if (writeWholeRegion(bb, i, dstrgn)) {
            use_kill.set(id);
            use_gen.reset(id);

            dstfootprint->setAll();
          } else {
            footprintDst(bb, i, dstrgn, dstfootprint);

            use_gen.set(id);
          }
        } else {
          use_gen.set(id);
        }
      } else if ((selectedRF & G4_GRF) && dst->isIndirect()) {
        // conservatively add each variable potentially accessed by dst to gen
        const REGVAR_VECTOR &pointsToSet =
            pointsToAnalysis.getAllInPointsToOrIndrUse(dst, bb);
        for (auto &pt : pointsToSet) {
          if (pt.var->isRegAllocPartaker()) {
            use_gen.set(pt.var->getId());
          }
        }
      }
    }

    //
    // process each source operand
    //
    for (unsigned j = 0, numSrc = i->getNumSrc(); j < numSrc; j++) {
      G4_Operand *src = i->getSrc(j);
      if (!src)
        continue;

      if (src->isSrcRegRegion()) {
        G4_Declare *topdcl = GetTopDclFromRegRegion(src);
        const G4_VarBase *base =
            (topdcl != nullptr ? topdcl->getRegVar()
                               : src->asSrcRegRegion()->getBase());
        if (base->isRegAllocPartaker()) {
          vASSERT(topdcl);
          unsigned id = topdcl->getRegVar()->getId();
          BitSet *srcfootprint = &footprints[id];

          if (srcfootprint->getSize() != 0) {
            footprintSrc(i, src->asSrcRegRegion(), srcfootprint);
          } else {
            unsigned int bitsetSize = (src->asSrcRegRegion()->isFlag())
                                          ? topdcl->getNumberFlagElements()
                                          : topdcl->getByteSize();

            BitSet newBitSet(bitsetSize, false);

            auto it = neverDefinedRows.find(topdcl);
            if (it != neverDefinedRows.end()) {
              // Bitwise OR new bitset with never defined rows
              newBitSet |= it->second;
            }

            footprints[id] = std::move(newBitSet);
            if (gra.isBlockLocal(topdcl) && topdcl->getAddressed() == false &&
                (topdcl->getRegFile() == G4_ADDRESS ||
                 src->asSrcRegRegion()->getRegAccess() == Direct)) {
              srcfootprint->setAll();
            }
            footprintSrc(i, src->asSrcRegRegion(), srcfootprint);
          }

          use_gen.set(static_cast<const G4_RegVar *>(base)->getId());
        }

        if ((selectedRF & G4_GRF) && src->isIndirect()) {
          int idx = 0;
          G4_RegVar *grf;
          G4_Declare *topdcl = GetTopDclFromRegRegion(src);

          while ((grf = pointsToAnalysis.getPointsTo(topdcl->getRegVar(),
                                                     idx++)) != NULL) {
            // grf is a variable that src potentially points to
            // since we dont know exactly which part of grf is sourced
            // assume entire grf is sourced
            // Also add grf to the gen set as it may be potentially used
            unsigned int id = grf->getId();
            use_gen.set(id);
            BitSet *srcfootprint = &footprints[id];

            if (srcfootprint->getSize() != 0) {
              srcfootprint->clear();

              DEBUG_VERBOSE("Found potential indirect use of "
                            << grf->getDeclare()->getName()
                            << " so resetting its footprint"
                            << "\n");
            }
          }
        }
      }
      //
      // treat the addr expr as both a use and a partial def
      //
      else if (src->isAddrExp()) {
        G4_RegVar *reg = static_cast<G4_AddrExp *>(src)->getRegVar();
        if (reg->isRegAllocPartaker() && !reg->isRegVarTmp()) {
          unsigned srcId = reg->getId();
          use_gen.set(srcId);
          def_out.set(srcId);
        }
      }
    }

    //
    // Process condMod
    //
    G4_CondMod *mod = i->getCondMod();
    if (mod) {
      G4_VarBase *flagReg = mod->getBase();
      if (flagReg) {
        if (flagReg->asRegVar()->isRegAllocPartaker()) {
          G4_Declare *topdcl = flagReg->asRegVar()->getDeclare();
          vISA_ASSERT(topdcl->getAliasDeclare() == nullptr,
                      "Invalid alias flag decl.");
          unsigned id = topdcl->getRegVar()->getId();

          BitSet *dstfootprint = &footprints[id];

          if (dstfootprint->getSize() == 0) {
            // Write for dst was not seen before, so insert in to map
            // bitsetSize is in bits for flag
            unsigned int bitsetSize = topdcl->getNumberFlagElements();

            BitSet newBitSet(bitsetSize, false);
            footprints[id] = std::move(newBitSet);

            if (gra.isBlockLocal(topdcl)) {
              dstfootprint->setAll();
            }
          }

          def_out.set(id);

          if (writeWholeRegion(bb, i, flagReg)) {
            use_kill.set(id);
            use_gen.reset(id);

            dstfootprint->setAll();
          } else {
            footprintDst(bb, i, mod, dstfootprint);
            use_gen.set(id);
          }
        }
      } else {
        vISA_ASSERT((i->opcode() == G4_sel || i->opcode() == G4_csel) &&
                        i->getCondMod() != nullptr,
                    "Invalid CondMod");
      }
    }

    //
    // Process predicate
    //
    G4_Predicate *predicate = i->getPredicate();
    if (predicate) {
      G4_VarBase *flagReg = predicate->getBase();
      vISA_ASSERT(flagReg->asRegVar()->getDeclare()->getAliasDeclare() ==
                      nullptr,
                  "Invalid alias flag decl.");
      if (flagReg->asRegVar()->isRegAllocPartaker()) {
        const G4_Declare *topdcl = flagReg->asRegVar()->getDeclare();
        unsigned id = topdcl->getRegVar()->getId();
        auto srcfootprint = &footprints[id];

        if (srcfootprint->getSize() != 0) {
          footprintSrc(i, predicate, srcfootprint);
        } else {
          unsigned int bitsetSize = topdcl->getNumberFlagElements();

          BitSet newBitSet(bitsetSize, false);
          footprints[id] = std::move(newBitSet);
          if (gra.isBlockLocal(topdcl)) {
            srcfootprint->setAll();
          }
          footprintSrc(i, predicate, srcfootprint);
        }

        use_gen.set(static_cast<const G4_RegVar *>(flagReg)->getId());
      }
    }

    //
    // Check whether dst can be killed at this point
    // A block of code is said to kill a variable when union
    // of all partial writes causes all elements to be written
    // into and any reads in the block can be sourced from
    // writes within that block itself
    //
    if (dst && dst->getBase()->isRegAllocPartaker()) {
      G4_Declare *topdcl = GetTopDclFromRegRegion(dst);
      vASSERT(topdcl);
      unsigned id = topdcl->getRegVar()->getId();
      auto dstfootprint = &footprints[id];

      if (dstfootprint->getSize() != 0) {
        // Found dst in map
        // Check whether all bits set
        // pseudo_kill for this dst was not found in this BB yet
        unsigned int first;
        LocalLiveRange *topdclLR = nullptr;

        if ((dstfootprint->isAllset() ||
             // Check whether the var is a spill and the inst does not write the
             // whole region
             topdcl->getRegVar()->isRegVarTransient() ||
             // Check whether local RA marked this range
             ((topdclLR = gra.getLocalLR(topdcl)) &&
              topdclLR->isLiveRangeLocal() && (!topdcl->isInput()) &&
              topdclLR->getFirstRef(first) == i)) &&
            // If single inst writes whole region then dont insert pseudo_kill
            writeWholeRegion(bb, i, dst) == false) {
          bool foundKill = false;
          INST_LIST::reverse_iterator nextIt = rit;
          ++nextIt;
          if (nextIt != bb->rend()) {
            const G4_INST *nextInst = (*nextIt);
            if (nextInst->isPseudoKill()) {
              G4_DstRegRegion *nextDst = nextInst->getDst();

              if (nextDst != NULL && nextDst->isDstRegRegion() &&
                  nextDst->getBase()->isRegAllocPartaker() &&
                  topdcl == GetTopDclFromRegRegion(nextDst)) {
                foundKill = true;
              }
            }
          }
          if (!foundKill) {
            // If the original spill is coalesced, then its
            // corresponding pseudo kill would be erased in the
            // cleaning pass. On the other hand, if a kill for the
            // spill is found, the spill is not the candidate for
            // coalescing, and the existing kill will be good.
            if (topdcl->getRegVar()->isRegVarTransient() ||
                topdcl->getRegVar()->isRegVarCoalesced()) {
              // Here we keep tracking the most favorable
              // position, i.e., the topmost, to insert the
              // pseudo kill for spilled and coalesced vars.
              pseudoKillsForSpills.insert_or_assign(topdcl, rit);
            } else {
              // All bytes of dst written at this point, so this is a good place
              // to insert pseudo kill inst
              pseudoKills.emplace_back(topdcl, rit);
            }
          }

          // Reset gen
          use_gen.reset(dst->getBase()->asRegVar()->getId());

          // Set kill
          use_kill.set(dst->getBase()->asRegVar()->getId());
          VISA_DEBUG_VERBOSE({
            std::cout << "Found kill at inst ";
            INST_LIST_ITER fwdIter = rit.base();
            fwdIter--;
            (*fwdIter)->emit(std::cout);
            std::cout << "\n";
          });
        }
      }
    }

    if (mod && mod->getBase() &&
        mod->getBase()->asRegVar()->isRegAllocPartaker()) {
      G4_VarBase *flagReg = mod->getBase();
      G4_Declare *topdcl = flagReg->asRegVar()->getDeclare();
      vASSERT(topdcl);
      unsigned id = topdcl->getRegVar()->getId();
      auto dstfootprint = &footprints[id];

      if (dstfootprint->getSize() != 0) {
        unsigned int first;
        const LocalLiveRange *topdclLR = nullptr;
        if ((dstfootprint->isAllset() ||
             // Check whether local RA marked this range
             // This may not be necessary as currently local RA is not performed
             // for flags.
             ((topdclLR = gra.getLocalLR(topdcl)) &&
              topdclLR->isLiveRangeLocal() &&
              topdclLR->getFirstRef(first) == i)) &&
            // If single inst writes whole region then dont insert pseudo_kill
            writeWholeRegion(bb, i, flagReg) == false) {
          // All bytes of dst written at this point, so this is a good place to
          // insert pseudo kill inst
          pseudoKills.emplace_back(topdcl, rit);

          // Reset gen
          use_gen.reset(flagReg->asRegVar()->getId());

          // Set kill
          use_kill.set(flagReg->asRegVar()->getId());
          VISA_DEBUG_VERBOSE({
            std::cout << "Found kill at inst ";
            INST_LIST_ITER fwdIter = rit.base();
            fwdIter--;
            (*fwdIter)->emit(std::cout);
            std::cout << "\n";
          });
        }
      }
    }
  }

  //
  // Insert pseudo_kill nodes in BB
  //
  for (auto &&pseudoKill : pseudoKills) {
    INST_LIST_ITER iterToInsert = pseudoKill.second.base();
    do {
      --iterToInsert;
    } while ((*iterToInsert)->isPseudoKill());
    G4_INST *killInst = fg.builder->createPseudoKill(
        pseudoKill.first, PseudoKillType::FromLiveness, false);
    bb->insertBefore(iterToInsert, killInst);
  }
  for (auto &pseudoKill : pseudoKillsForSpills) {
    INST_LIST_ITER iterToInsert = pseudoKill.second.base();
    do {
      --iterToInsert;
    } while ((*iterToInsert)->isPseudoKill());
    G4_INST *killInst = fg.builder->createPseudoKill(
        pseudoKill.first, PseudoKillType::FromLiveness, false);
    bb->insertBefore(iterToInsert, killInst);
  }

  //
  // initialize use_in
  //
  use_in = use_gen;
}

//
// use_out = use_in(s1) + use_in(s2) + ... where s1 s2 ... are the successors of
// bb use_in  = use_gen + (use_out - use_kill)
//
bool LivenessAnalysis::contextFreeUseAnalyze(G4_BB *bb, bool isChanged) {
  bool changed;

  unsigned bbid = bb->getId();

  if (bb->Succs.empty()) // exit block
  {
    changed = false;
  } else if (isChanged) {
    // no need to update changed. This saves a memcpy
    for (auto succBB : bb->Succs) {
      use_out[bbid] |= use_in[succBB->getId()];
    }
    changed = true;
  } else {
    SparseBitVector old = use_out[bbid];
    for (auto succBB : bb->Succs) {
      use_out[bbid] |= use_in[succBB->getId()];
    }

    changed = (old != use_out[bbid]);
  }

  //
  // in = gen + (out - kill)
  //
  use_in[bbid] = use_out[bbid];
  use_in[bbid] = use_in[bbid] - use_kill[bbid];
  use_in[bbid] |= use_gen[bbid];

  return changed;
}

//
// def_in = def_out(p1) + def_out(p2) + ... where p1 p2 ... are the predecessors
// of bb def_out |= def_in
//
bool LivenessAnalysis::contextFreeDefAnalyze(G4_BB *bb, bool isChanged) {
  bool changed = false;
  unsigned bbid = bb->getId();

  if (bb->Preds.empty()) {
    changed = false;
  } else if (isChanged) {
    // no need to update changed. This saves a memcpy
    for (auto predBB : bb->Preds) {
      def_in[bbid] |= def_out[predBB->getId()];
    }
    changed = true;
  } else {
    SparseBitVector old = def_in[bbid];
    for (auto predBB : bb->Preds) {
      def_in[bbid] |= def_out[predBB->getId()];
    }
    changed = (old != def_in[bbid]);
  }

  def_out[bb->getId()] |= def_in[bb->getId()];

  return changed;
}

void LivenessAnalysis::dump_bb_vector(char *vname, std::vector<BitSet> &vec) {
  std::cerr << vname << "\n";
  for (BB_LIST_ITER it = fg.begin(); it != fg.end(); it++) {
    G4_BB *bb = (*it);
    std::cerr << "    BB" << bb->getId() << "\n";
    const BitSet &in = vec[bb->getId()];
    std::cerr << "        ";
    for (unsigned i = 0; i < in.getSize(); i += 10) {
      //
      // dump 10 bits a group
      //
      for (unsigned j = i; j < in.getSize() && j < i + 10; j++) {
        std::cerr << (in.isSet(j) ? "1" : "0");
      }
      std::cerr << " ";
    }
    std::cerr << "\n";
  }
}

void LivenessAnalysis::dump_fn_vector(char *vname, std::vector<FuncInfo *> &fns,
                                      std::vector<BitSet> &vec) {
  DEBUG_VERBOSE(vname << "\n");
  for (std::vector<FuncInfo *>::iterator it = fns.begin(); it != fns.end();
       it++) {
    FuncInfo *funcInfo = (*it);

    DEBUG_VERBOSE("    FN" << funcInfo->getId() << "\n");
    const BitSet &in = vec[funcInfo->getId()];
    DEBUG_VERBOSE("        ");
    for (unsigned i = 0; i < in.getSize(); i += 10) {
      //
      // dump 10 bits a group
      //
      for (unsigned j = i; j < in.getSize() && j < i + 10; j++) {
        DEBUG_VERBOSE(in.isSet(j) ? "1" : "0");
      }
      DEBUG_VERBOSE(" ");
    }
    DEBUG_VERBOSE("\n");
  }
}

//
// dump which vars are live at the entry of BB
//
void LivenessAnalysis::dump() const {
  for (auto bb : fg) {
    std::cerr << "BB" << bb->getId() << "'s live in: ";
    unsigned total_size = 0;
    auto dumpVar = [&total_size](G4_RegVar *var) {
      int size =
          var->getDeclare()->getTotalElems() * var->getDeclare()->getElemSize();
      std::cerr << var->getName() << "(" << size << "), ";
      total_size += size;
    };

    unsigned count = 0;
    for (auto var : vars) {
      if (var->isRegAllocPartaker() && isLiveAtEntry(bb, var->getId())) {
        if (count++ % 10 == 0)
          std::cerr << "\n";
        dumpVar(var);
      }
    }
    std::cerr << "\nBB" << bb->getId() << "'s live in size: "
              << total_size / fg.builder->numEltPerGRF<Type_UB>() << "\n\n";
    std::cerr << "BB" << bb->getId() << "'s live out: ";
    total_size = 0;
    count = 0;
    for (auto var : vars) {
      if (var->isRegAllocPartaker() && isLiveAtExit(bb, var->getId())) {
        if (count++ % 10 == 0)
          std::cerr << "\n";
        dumpVar(var);
      }
    }
    std::cerr << "\nBB" << bb->getId() << "'s live out size: "
              << total_size / fg.builder->numEltPerGRF<Type_UB>() << "\n\n";
  }
}

void LivenessAnalysis::dumpBB(G4_BB *bb) const {
  std::cerr << "\n\nBB" << bb->getId() << "'s live in: ";
  unsigned total_size = 0;
  auto dumpVar = [&total_size](G4_RegVar *var) {
    int size =
        var->getDeclare()->getTotalElems() * var->getDeclare()->getElemSize();
    std::cerr << var->getName() << "(" << size << ")"
              << "[" << var->getRegAllocPartaker() << "], ";
    total_size += size;
  };

  unsigned count = 0;
  for (auto var : vars) {
    if (var->isRegAllocPartaker() && isLiveAtEntry(bb, var->getId())) {
      if (count++ % 10 == 0)
        std::cerr << "\n";
      dumpVar(var);
    }
  }
  std::cerr << "\n\nBB" << bb->getId() << "'s live out: ";
  total_size = 0;
  count = 0;
  for (auto var : vars) {
    if (var->isRegAllocPartaker() && isLiveAtExit(bb, var->getId())) {
      if (count++ % 10 == 0)
        std::cerr << "\n";
      dumpVar(var);
    }
  }
  std::cerr << "\n\nBB" << bb->getId() << "'s use through: ";
  total_size = 0;
  count = 0;
  for (auto var : vars) {
    if (var->isRegAllocPartaker() && isUseThrough(bb, var->getId())) {
      if (count++ % 10 == 0)
        std::cerr << "\n";
      dumpVar(var);
    }
  }
  std::cerr << "\n\nBB" << bb->getId() << "'s def through: ";
  total_size = 0;
  count = 0;
  for (auto var : vars) {
    if (var->isRegAllocPartaker() && isDefThrough(bb, var->getId())) {
      if (count++ % 10 == 0)
        std::cerr << "\n";
      dumpVar(var);
    }
  }
}

void LivenessAnalysis::dumpLive(BitSet &live) const {
  auto dumpVar = [](G4_RegVar *var) {
    int size =
        var->getDeclare()->getTotalElems() * var->getDeclare()->getElemSize();
    std::cerr << var->getName() << "(" << size << ")"
              << "[" << var->getRegAllocPartaker() << "], ";
  };

  unsigned count = 0;
  for (auto var : vars) {
    if (live.isSet(var->getId())) {
      if (count++ % 10 == 0)
        std::cerr << "\n";
      dumpVar(var);
    }
  }
}

//
// dump which vars are live at the entry of BB
//
void LivenessAnalysis::dumpGlobalVarNum() const {
  SparseBitVector global_def_out;
  SparseBitVector global_use_in;

  for (auto bb : fg) {
    SparseBitVector global_in = use_in[bb->getId()];
    SparseBitVector global_out = def_out[bb->getId()];
    global_in &= def_in[bb->getId()];
    global_use_in |= global_in;
    global_out &= use_out[bb->getId()];
    global_def_out |= global_out;
  }

  int global_var_num = 0;
  for (auto var : vars) {
    if (var->isRegAllocPartaker()) {
      if (global_use_in.test(var->getId()) ||
          global_def_out.test(var->getId())) {
        global_var_num++;
      }
    }
  }
  std::cerr << "total var num: " << numVarId
            << " global var num: " << global_var_num << "\n";
}

void LivenessAnalysis::reportUndefinedUses() const {
  auto dumpVar = [](G4_RegVar *var) {
    int size =
        var->getDeclare()->getTotalElems() * var->getDeclare()->getElemSize();
    std::cerr << var->getName() << "(" << size << "), ";
  };

  std::cerr << "\nPossible undefined uses in kernel "
            << fg.getKernel()->getName() << ":\n";
  unsigned count = 0;
  for (auto var : vars) {
    // Skip if the var is not involved in RA.
    if (!var->isRegAllocPartaker())
      continue;
    // Skip if the var is a AddrSpillLoc.
    if (var->isRegVarAddrSpillLoc())
      continue;
    // Skip if the var is not in use_in of BB0
    if (!isUseIn(fg.getEntryBB(), var->getId()))
      continue;
    // Skip if the var is in def_in of BB0
    if (def_in[fg.getEntryBB()->getId()].test(var->getId()))
      continue;

    if (count++ % 10 == 0)
      std::cerr << "\n";
    dumpVar(var);
  }
  std::cerr << "\n";
}

bool LivenessAnalysis::isEmptyLiveness() const { return numBBId == 0; }

SparseBitVector LivenessAnalysis::getLiveAtEntry(const G4_BB* bb) const {
  return use_in[bb->getId()] & def_in[bb->getId()];
}

SparseBitVector LivenessAnalysis::getLiveAtExit(const G4_BB *bb) const {
  return use_out[bb->getId()] & def_out[bb->getId()];
}
//
// return true if var is live at the entry of bb
// check both use_in and def_in, if one condition fails then var is not in the
// live range
//
bool LivenessAnalysis::isLiveAtEntry(const G4_BB *bb, unsigned var_id) const {
  return use_in[bb->getId()].test(var_id) && def_in[bb->getId()].test(var_id);
}
//
// return true if var is live at the exit of bb
//
bool LivenessAnalysis::isLiveAtExit(const G4_BB *bb, unsigned var_id) const {
  return use_out[bb->getId()].test(var_id) &&
         def_out[bb->getId()].test(var_id);
}

//
// return true if var is user through the bb
//
bool LivenessAnalysis::isUseOut(const G4_BB *bb, unsigned var_id) const {
  return use_out[bb->getId()].test(var_id);
}

//
// return true if var is user through the bb
//
bool LivenessAnalysis::isUseIn(const G4_BB *bb, unsigned var_id) const {
  return use_in[bb->getId()].test(var_id);
}

//
// return true if var is user through the bb
//
bool LivenessAnalysis::isUseThrough(const G4_BB *bb, unsigned var_id) const {
  return use_in[bb->getId()].test(var_id) &&
         use_out[bb->getId()].test(var_id);
}
//
// return true if var is live at the exit of bb
//
bool LivenessAnalysis::isDefThrough(const G4_BB *bb, unsigned var_id) const {
  return def_in[bb->getId()].test(var_id) &&
         def_out[bb->getId()].test(var_id);
}

void GlobalRA::markBlockLocalVar(G4_RegVar *var, unsigned bbId) {
  G4_Declare *dcl = var->getDeclare()->getRootDeclare();

  if (dcl->isInput() || dcl->isOutput()) {
    setBBId(dcl, UINT_MAX - 1);
  } else {
    if (getBBId(dcl) == bbId) {
      // Do nothing.
    } else if (getBBId(dcl) == UINT_MAX) {
      setBBId(dcl, bbId);
    } else {
      setBBId(dcl, UINT_MAX - 1);
    }
  }
}

void GlobalRA::markBlockLocalVars() {
  for (auto bb : kernel.fg) {
    for (std::list<G4_INST *>::iterator it = bb->begin(); it != bb->end();
         it++) {
      G4_INST *inst = *it;

      // Chjeck if there is undefine variable used in CMP instruction, which is
      // used to detect the execution mask.
      //     cmp.eq (M1, 16) P12 V0147(0,0)<0;1,0> V0147(0,0)<0;1,0>
      if (inst->opcode() == G4_cmp) {
        const bool isModEq =
            inst->getCondMod() && inst->getCondMod()->getMod() == Mod_e;
        const bool isNullDst = !inst->getDst() || inst->hasNULLDst();
        const bool isSrc0SameAsSrc1 = inst->getSrc(0)->asSrcRegRegion() &&
                                      inst->getSrc(1)->asSrcRegRegion() &&
                                      *inst->getSrc(0)->asSrcRegRegion() ==
                                          *inst->getSrc(1)->asSrcRegRegion();
        if (isModEq && isNullDst && isSrc0SameAsSrc1) {
          G4_Declare *topdcl = GetTopDclFromRegRegion(inst->getSrc(0));
          if (topdcl && topdcl->getRegFile() == G4_GRF) {
            addUndefinedCmpDcl(topdcl);
          }
        }
      }

      // Track direct dst references.

      G4_DstRegRegion *dst = inst->getDst();

      if (dst != NULL) {
        G4_DstRegRegion *dstRgn = dst->asDstRegRegion();

        if (dstRgn->getBase()->isRegVar()) {
          markBlockLocalVar(dstRgn->getBase()->asRegVar(), bb->getId());

          G4_Declare *topdcl = GetTopDclFromRegRegion(dst);
          if (topdcl) {
            if (inst->isSend()) {
              topdcl->setIsRefInSendDcl(true);
            }

            LocalLiveRange *lr = GetOrCreateLocalLiveRange(topdcl);
            unsigned int startIdx;
            if (lr->getFirstRef(startIdx) == NULL) {
              lr->setFirstRef(inst, 0);
            }
            lr->recordRef(bb);
            lr->markDefined();
            recordRef(topdcl);
          }
        }
      }

      G4_CondMod *condMod = inst->getCondMod();

      if (condMod != NULL && condMod->getBase() != NULL) {
        if (condMod->getBase() && condMod->getBase()->isRegVar()) {
          markBlockLocalVar(condMod->getBase()->asRegVar(), bb->getId());

          G4_Declare *topdcl = condMod->getBase()->asRegVar()->getDeclare();
          if (topdcl) {
            LocalLiveRange *lr = GetOrCreateLocalLiveRange(topdcl);
            unsigned int startIdx;
            if (lr->getFirstRef(startIdx) == NULL) {
              lr->setFirstRef(inst, 0);
            }
            lr->recordRef(bb);
            recordRef(topdcl);
          }
        }
      }

      // Track direct src references.
      for (unsigned j = 0, numSrc = inst->getNumSrc(); j < numSrc; j++) {
        G4_Operand *src = inst->getSrc(j);

        if (src == NULL) {
          // Do nothing.
        } else if (src->isSrcRegRegion() &&
                   src->asSrcRegRegion()->getBase()->isRegVar()) {
          G4_SrcRegRegion *srcRgn = src->asSrcRegRegion();

          if (srcRgn->getBase()->isRegVar()) {
            markBlockLocalVar(src->asSrcRegRegion()->getBase()->asRegVar(),
                              bb->getId());

            G4_Declare *topdcl = GetTopDclFromRegRegion(src);
            if (topdcl) {
              if (inst->isSend()) {
                topdcl->setIsRefInSendDcl(true);
              }

              LocalLiveRange *lr = GetOrCreateLocalLiveRange(topdcl);

              lr->recordRef(bb);
              recordRef(topdcl);
              if (inst->isEOT()) {
                lr->markEOT();
              }
            }
          }
        } else if (src->isAddrExp()) {
          G4_RegVar *addExpVar = src->asAddrExp()->getRegVar();
          markBlockLocalVar(addExpVar, bb->getId());

          G4_Declare *topdcl = addExpVar->getDeclare()->getRootDeclare();
          vISA_ASSERT(topdcl != NULL, "Top dcl was null for addr exp opnd");

          LocalLiveRange *lr = GetOrCreateLocalLiveRange(topdcl);
          lr->recordRef(bb);
          lr->markIndirectRef();
          recordRef(topdcl);
        }
      }

      G4_Operand *pred = inst->getPredicate();

      if (pred != NULL) {
        if (pred->getBase() && pred->getBase()->isRegVar()) {
          markBlockLocalVar(pred->getBase()->asRegVar(), bb->getId());
          G4_Declare *topdcl = pred->getBase()->asRegVar()->getDeclare();
          if (topdcl) {
            LocalLiveRange *lr = GetOrCreateLocalLiveRange(topdcl);
            lr->recordRef(bb);
            recordRef(topdcl);
          }
        }
      }

      // Track all indirect references.
      const REGVAR_VECTOR &grfVec =
          pointsToAnalysis.getIndrUseVectorForBB(bb->getId());
      for (pointInfo grf : grfVec) {
        markBlockLocalVar(grf.var, bb->getId());
      }
    }
  }
}

void GlobalRA::resetGlobalRAStates() {
  auto origDclSize = kernel.Declares.size();
  if (builder.getOption(vISA_LocalDeclareSplitInGlobalRA)) {
    incRA.resetPartialDcls();
    // remove partial decls
    auto isPartialDcl = [](G4_Declare *dcl) { return dcl->getIsPartialDcl(); };

    kernel.Declares.erase(std::remove_if(kernel.Declares.begin(),
                                         kernel.Declares.end(), isPartialDcl),
                          kernel.Declares.end());
  }

  incRA.reduceMaxDclId(origDclSize - kernel.Declares.size());

  for (auto dcl : kernel.Declares) {
    // Reset all the local live ranges
    resetLocalLR(dcl);

    if (builder.getOption(vISA_LocalDeclareSplitInGlobalRA)) {
      // Remove the split declares
      if (dcl->getIsSplittedDcl()) {
        dcl->setIsSplittedDcl(false);
        clearSubDcl(dcl);
        incRA.markForIntfUpdate(dcl);
      }
    }

    // Remove the bank assignment
    if (builder.getOption(vISA_LocalBankConflictReduction) &&
        builder.hasBankCollision()) {
      setBankConflict(dcl, BANK_CONFLICT_NONE);
    }
    clearBundleConflictDcl(dcl);
  }

  return;
}

// We emit FDE so that:
// 1. Debug information can easily point to previous frame state,
// 2. We can restore caller frame state before returning.
//
// -skipFDE option skips emission of store in stack call function prolog.
// As an optimization, it's okay to skip this store when the function is a
// leaf function. If it's not a leaf function then we emit the store,
// irrespective of -skipFDE. When the switch is not specified (default)
// we emit the store even for leaf functions (in case debugger connects).
bool GlobalRA::canSkipFDE() const {
  return !kernel.fg.getHasStackCalls() && kernel.getOption(vISA_skipFDE);
}

void GlobalRA::setUndefinedVarCmp() {
  // Iterate over all dcls and remove those with 0
  // ref count and not addressed. This is done only for
  // GRF dcls.

  // Propagate top dcl info to aliases
  for (auto dcl : UndefinedCmpVars) {
    LocalLiveRange *lr = getLocalLR(dcl);
    if (!lr->isDefined()) {
      dcl->setIsCmpUseOnly(true);
    }
  }
}

//
// Mark block local (temporary) variables.
//
void GlobalRA::markGraphBlockLocalVars() {
  // Clear stale LocalLiveRange* first to avoid double ref counting
  clearStaleLiveRanges();

  // Create live ranges and record the reference info
  markBlockLocalVars();

  // Set undefined variable used in cmp
  setUndefinedVarCmp();

  VISA_DEBUG_VERBOSE({
    std::cout << "\t--LOCAL VARIABLES--\n";
    for (auto dcl : kernel.Declares) {
      LocalLiveRange *topdclLR = getLocalLR(dcl);

      if (topdclLR && topdclLR->isLiveRangeLocal()) {
        std::cout << dcl->getName() << ",\t";
      }
    }
    std::cout << "\n";
  });
}

unsigned int IR_Builder::getCallRetOpndSize() const {
  unsigned int numElems = 2;
  return numElems;
}

//
// Pre-assign phy regs to stack call function return variable as per ABI.
//
void FlowGraph::setABIForStackCallFunctionCalls() {
  // For each G4_pseudo_fcall inst, create dst of GRF type
  // with physical register 1.0 pre-assigned to it.
  // Similarly, for G4_pseudo_fret create src of GRF type
  // with physical register 1.0 pre-assigned to it.
  // Each will use 2 dwords of r1.0.
  int call_id = 0, ret_id = 0;

  for (auto bb : *this) {
    if (bb->isEndWithFCall()) {
      const char *n = builder->getNameString(25, "FCALL_RET_LOC_%d", call_id++);

      G4_INST *fcall = bb->back();
      // Set call dst to fpspGRF
      G4_Declare *r1_dst = builder->createDeclare(
          n, G4_GRF, builder->getCallRetOpndSize(), 1, Type_UD);
      r1_dst->getRegVar()->setPhyReg(
          builder->phyregpool.getGreg(builder->kernel.stackCall.getFPSPGRF()),
          builder->kernel.stackCall.subRegs.Ret_IP);
      G4_DstRegRegion *dstRgn =
          builder->createDst(r1_dst->getRegVar(), 0, 0, 1, Type_UD);
      fcall->setDest(dstRgn);
    }

    if (bb->isEndWithFRet()) {
      const char *n = builder->getNameString(25, "FRET_RET_LOC_%d", ret_id++);
      G4_INST *fret = bb->back();
      const RegionDesc *rd = builder->createRegionDesc(2, 2, 1);
      G4_Declare *r1_src = builder->createDeclare(
          n, G4_INPUT, builder->getCallRetOpndSize(), 1, Type_UD);
      r1_src->getRegVar()->setPhyReg(
          builder->phyregpool.getGreg(builder->kernel.stackCall.getFPSPGRF()),
          builder->kernel.stackCall.subRegs.Ret_IP);
      G4_Operand *srcRgn =
          builder->createSrc(r1_src->getRegVar(), 0, 0, rd, Type_UD);
      fret->setSrc(srcRgn, 0);
      if (fret->getExecSize() == g4::SIMD1) {
        // due to <2;2,1> regioning we must update exec size as well
        fret->setExecSize(g4::SIMD2);
      }
      if (builder->getOption(vISA_GenerateDebugInfo)) {
        pKernel->getKernelDebugInfo()->setFretVar(
            GetTopDclFromRegRegion(fret->getSrc(0)));
      }
    }
  }
}

// Function to verify RA results
void GlobalRA::verifyRA(LivenessAnalysis &liveAnalysis) {
  for (auto bb : kernel.fg) {
    unsigned int numGRF = kernel.getNumRegTotal();

    // Verify PREG assignment
    for (auto inst : *bb) {
      G4_DstRegRegion *dst = inst->getDst();
      if (dst != nullptr && dst->getBase()->isRegAllocPartaker()) {
        vISA_ASSERT(dst->getBase()->asRegVar()->getPhyReg(),
                    "RA verification error: No PREG assigned for variable %s",
                    GetTopDclFromRegRegion(dst)->getName());
      }

      for (unsigned j = 0, numSrc = inst->getNumSrc(); j < numSrc; j++) {
        G4_Operand *src = inst->getSrc(j);
        if (src && src->isSrcRegRegion() &&
            src->asSrcRegRegion()->getBase()->isRegAllocPartaker()) {
          vISA_ASSERT(src->asSrcRegRegion()->getBase()->asRegVar()->getPhyReg(),
                      "RA verification error: No PREG assigned for variable %s",
                      GetTopDclFromRegRegion(src->asSrcRegRegion())->getName());
        }
      }

      if (inst->isSend()) {
        if (dst && dst->getBase()->isRegAllocPartaker()) {
          auto preg = dst->getBase()->asRegVar()->getPhyReg();
          if (preg && preg->isGreg()) {
            vISA_ASSERT(numGRF >=
                            (preg->asGreg()->getRegNum() +
                             inst->asSendInst()->getMsgDesc()->getDstLenRegs()),
                        "dst response length goes beyond last GRF");
          }
        }

        for (unsigned j = 0, numSrc = inst->getNumSrc(); j < numSrc; j++) {
          G4_Operand *src = inst->getSrc(j);
          if (src && src->isSrcRegRegion() &&
              src->getBase()->isRegAllocPartaker()) {
            auto preg = src->getBase()->asRegVar()->getPhyReg();
            if (preg && preg->isGreg()) {
              [[maybe_unused]] unsigned int srcLen = 0;
              if (j == 0)
                srcLen = inst->asSendInst()->getMsgDesc()->getSrc0LenRegs();
              else if (j == 1)
                srcLen = inst->asSendInst()->getMsgDesc()->getSrc1LenRegs();

              vISA_ASSERT(numGRF >= (preg->asGreg()->getRegNum() + srcLen),
                          "src payload length goes beyond last GRF");
            }
          }
        }
      }
    }

    // Verify Live-in
    std::map<uint32_t, G4_Declare *> LiveInRegMap;
    std::map<uint32_t, G4_Declare *>::iterator LiveInRegMapIt;
    std::vector<uint32_t> liveInRegVec(numGRF * kernel.numEltPerGRF<Type_UW>(),
                                       UINT_MAX);

    for (G4_Declare *dcl : kernel.Declares) {
      if (dcl->getAliasDeclare() != nullptr)
        continue;

      if (dcl->getRegVar()->isRegAllocPartaker()) {
        G4_RegVar *var = dcl->getRegVar();
        uint32_t varID = var->getId();
        if (liveAnalysis.isLiveAtEntry(bb, dcl->getRegVar()->getId())) {
          // Skip live-in that has no physical reg assigned.
          // Currently there might be some cases of a variable not
          // actually used but marked as live-in and live-out. For
          // example, spillHeader var is created for save/restore
          // spill/fill code in stack call case, and is not used when
          // expanding spill/fill using LSC.
          if (!var->isPhyRegAssigned())
            continue;

          vISA_ASSERT(
              var->getPhyReg()->isGreg(),
              "RA verification error: Invalid preg assignment for variable %s",
              dcl->getName());

          uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
          uint32_t regOff = var->getPhyRegOff();

          uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                         (regOff * dcl->getElemSize()) / G4_WSIZE;
          for (uint32_t i = 0; i < dcl->getWordSize(); ++i, ++idx) {
            LiveInRegMapIt = LiveInRegMap.find(idx);
            if (liveInRegVec[idx] != UINT_MAX) {
              vISA_ASSERT(
                  LiveInRegMapIt != LiveInRegMap.end(),
                  "RA verification error: Invalid entry in LiveInRegMap!");
              if (dcl->isInput()) {
                vISA_ASSERT(false,
                            "RA verification warning: Found  conflicting input "
                            "variables: %s and %s assigned to r%d.%d",
                            dcl->getName(), (*LiveInRegMapIt).second->getName(),
                            regNum, regOff);
                liveInRegVec[idx] = varID;
                LiveInRegMapIt->second = dcl;
              } else {
                // There's case that the 2 vars in outer scope are just live
                // through the subroutine, but IPA would mark 2 vars are live in
                // the subroutine. So, here we report error only if either var
                // has the same scope id as the current BB. If any var is used
                // in the subroutine, we can still catch the conflict when
                // verifying instructions later.
                //
                //     foo() {
                //         a = ...;
                //         bar();
                //         ... = a;
                //
                //         b = ...;
                //         bar();
                //         ... = b;
                //     }
                //     bar() {
                //         ...
                //     }
                vISA_ASSERT(bb->getScopeID() != dcl->getScopeID() &&
                                bb->getScopeID() !=
                                    LiveInRegMapIt->second->getScopeID(),
                            "RA verification error: Found conflicting live-in "
                            "variables: %s and %s assigned to r%d.%d",
                            dcl->getName(), LiveInRegMapIt->second->getName(),
                            regNum, regOff);
              }

            } else {
              liveInRegVec[idx] = varID;
              vISA_ASSERT(
                  LiveInRegMapIt == LiveInRegMap.end(),
                  "RA verification error: Invalid entry in LiveInRegMap!");
              LiveInRegMap.emplace(idx, dcl);
            }
          }
        }
      }
    }

    // Verify Live-out
    G4_Declare *ret = kernel.fg.builder->getStackCallRet();
    std::map<uint32_t, G4_Declare *> liveOutRegMap;
    std::map<uint32_t, G4_Declare *>::iterator liveOutRegMapIt;
    std::vector<uint32_t> liveOutRegVec(numGRF * kernel.numEltPerGRF<Type_UW>(),
                                        UINT_MAX);

    for (G4_Declare *dcl : kernel.Declares) {
      if (dcl->getAliasDeclare() != nullptr)
        continue;
      if (dcl->getRegVar()->isRegAllocPartaker()) {
        G4_RegVar *var = dcl->getRegVar();
        uint32_t varID = var->getId();
        if (liveAnalysis.isLiveAtExit(bb, varID)) {
          // Skip live-out w/o physical reg assigned as well.
          if (!var->isPhyRegAssigned())
            continue;

          vISA_ASSERT(
              var->getPhyReg()->isGreg(),
              "RA verification error: Invalid preg assignment for variable %s",
              dcl->getName());

          uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
          uint32_t regOff = var->getPhyRegOff();

          uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                         (regOff * dcl->getElemSize()) / G4_WSIZE;
          for (uint32_t i = 0; i < dcl->getWordSize(); ++i, ++idx) {
            liveOutRegMapIt = liveOutRegMap.find(idx);
            if (liveOutRegVec[idx] != UINT_MAX) {
              vISA_ASSERT(
                  liveOutRegMapIt != liveOutRegMap.end(),
                  "RA verification error: Invalid entry in liveOutRegMap!");
              if (dcl->isInput()) {
                vISA_ASSERT(false,
                            "RA verification error: Found conflicting input "
                            "variables %s and %s assigned to r%d.%d",
                            dcl->getName(), liveOutRegMapIt->second->getName(),
                            regNum, regOff);
                liveOutRegVec[idx] = varID;
                liveOutRegMapIt->second = dcl;
              } else {
                // Same as the case of live-in verification. Also report error
                // only if either var has the same scope id as the current BB.

                vISA_ASSERT(bb->getScopeID() != dcl->getScopeID() &&
                                bb->getScopeID() !=
                                    liveOutRegMapIt->second->getScopeID(),
                            "RA verification error: Found conflicting live out "
                            "variables: %s and %s assigned to r %d.%d",
                            dcl->getName(), liveOutRegMapIt->second->getName(),
                            regNum, regOff);
              }

            } else {
              liveOutRegVec[idx] = varID;
              vISA_ASSERT(
                  liveOutRegMapIt == liveOutRegMap.end(),
                  "RA verification error: Invalid entry in liveOutRegMap!");
              liveOutRegMap.emplace(idx, dcl);
            }
          }
        }
      }
    }

    std::set<G4_Declare *>
        spillFillVariableOverwriteSet; // the collection is used to check if
                                       // variables generated by spill/fill have
                                       // been overwrite
    for (INST_LIST::reverse_iterator rit = bb->rbegin(); rit != bb->rend();
         ++rit) {
      G4_INST *inst = (*rit);
      // Skip the special case that is used to get the execution mask.
      //
      //     cmp.eq (M1, 16) P12 V0147(0,0)<0;1,0> V0147(0,0)<0;1,0>
      //     cmp.eq (M5, 16) P12 V0147(0,0)<0;1,0> V0147(0,0)<0;1,0>
      //
      // FIXME: In the case, src0 and src1 are same and can be any
      // register. In addition, the var might not be defined by vISA user
      // resulting in imprecise liveness result. Some possible fix could
      // be using the pre-defined var like R0, but it's not clear if the
      // R0's live range would be affected a lot.
      if (inst->opcode() == G4_cmp) {
        const bool isModEq =
            inst->getCondMod() && inst->getCondMod()->getMod() == Mod_e;
        const bool isNullDst = !inst->getDst() || inst->hasNULLDst();
        const bool isSrc0SameAsSrc1 = inst->getSrc(0)->asSrcRegRegion() &&
                                      inst->getSrc(1)->asSrcRegRegion() &&
                                      *inst->getSrc(0)->asSrcRegRegion() ==
                                          *inst->getSrc(1)->asSrcRegRegion();
        if (isModEq && isNullDst && isSrc0SameAsSrc1)
          continue;
      }
      INST_LIST_RITER ritNext = rit;
      ritNext++;
      G4_INST *rNInst = nullptr;
      if (ritNext != bb->rend()) {
        rNInst = (*ritNext);
      }

      G4_DstRegRegion *dst = inst->getDst();
      G4_DstRegRegion *rNDst = nullptr;
      G4_Declare *rNDcl = nullptr;
      if (rNInst && rNInst->isPseudoKill()) {
        rNDst = rNInst->getDst();
        if (rNDst != nullptr) {
          rNDcl = GetTopDclFromRegRegion(rNDst);
        }
      }

      // verify dst operand
      if (dst != nullptr) {
        uint32_t varID = UINT_MAX;
        G4_Declare *dcl = nullptr;

        auto verifyDstRA = [&](uint32_t idx, uint32_t regNum, uint32_t regOff,
                               bool &suppressWarning) {
          vISA_ASSERT(dcl && dcl->getRegVar() &&
                          dcl->getRegVar()->isPhyRegAssigned(),
                      "RA verification error: Found dst variable without "
                      "physical register.");
          liveOutRegMapIt = liveOutRegMap.find(idx);
          if (liveOutRegVec[idx] == UINT_MAX) {
            vISA_ASSERT(
                liveOutRegMapIt == liveOutRegMap.end(),
                "RA verification error: Invalid entry in liveOutRegMap!");

            // Since we treat variables generated by spill/fill as whole region,
            // here need special handling for fill->modify->spill case that is
            // to skip dst check if it has been written. Take below case as
            // example, we should skip the dst(r13-r14) check of the fill
            // instruction since it has been written in mov instruction and has
            // been removed from liveOutRegMap.
            //
            // clang-format off
            // (W) send(16)    r13.0<1>:ud r25 0xA 0x22c1019:ud // $236:; scratch read, resLen=2, msgLen=1
            //     mov(8)      r13.1 < 2 > :d  r10.1 < 2; 1, 0 > : d // $236:&400:
            // (W) sends(16)   null : ud r25 r13 0x8a : ud 0x20f1019 : ud // $236:; scratch write, resLen=2, msgLen=1, extMsgLen=2
            // clang-format on
            if (!suppressWarning && !inst->isPseudoKill() &&
                !(dcl == rNDcl && rNInst->isPseudoKill()) &&
                spillFillVariableOverwriteSet.find(dcl) ==
                    spillFillVariableOverwriteSet.end()) {
              // This warning is possible as some variables are defined without
              // usage. TV5 is the example:
              // clang-format off
              // before RA:
              //     (W) send(8)  TV5(0,0)<1>:ud R0_Copy0(0,0) 0xA 0x219e0fe:ud // $130:&217:; memory fence, resLen=1, msgLen=1
              //     (W) and(8)   M7(0,0)<1>:ud  R0_Copy0(0,2)<0;1,0>:ud  0x7f000000:ud // $131:&218:
              // after RA:
              //     (W) send(8)  r2.0<1>:ud r10 0xA 0x219e0fe:ud // $130:&217:; memory fence, resLen=1, msgLen=1
              //     (W) and(8)   r2.0<1>:ud r10.2<0;1,0>:ud  0x7f000000 : ud // $131:&218:
              // clang-format on
              VISA_DEBUG(std::cout
                         << "RA verification warning: Found unused variable "
                         << dcl->getName() << ". Please double check!\n");
              suppressWarning = true;
            }
          } else {
            vISA_ASSERT(
                liveOutRegMapIt != liveOutRegMap.end(),
                "RA verification error: Invalid entry in liveOutRegMap!");
            if (liveOutRegVec[idx] != varID) {
              const SparseBitVector &indr_use = liveAnalysis.indr_use[bb->getId()];

              if (strstr(dcl->getName(), GlobalRA::StackCallStr) != nullptr) {
                vISA_ASSERT(false,
                            "RA verification error: Found conflicting "
                            "stackCall variable: %s and %s assigned to r%d.%d",
                            dcl->getName(), liveOutRegMapIt->second->getName(),
                            regNum, regOff);
              } else if (indr_use.test(liveOutRegVec[idx]) == true) {
                vISA_ASSERT(false,
                            "RA verification error: Found conflicting indirect "
                            "variables %s and %s assigned to r%d.%d",
                            dcl->getName(), liveOutRegMapIt->second->getName(),
                            regNum, regOff);
              } else {
                if (!inst->isPseudoKill()) {
                  vISA_ASSERT(false,
                              "RA verification error: Found conflicting "
                              "variables: %s and %s assigned to r%d.%d",
                              dcl->getName(),
                              liveOutRegMapIt->second->getName(), regNum,
                              regOff);
                }
              }
            } else {
              liveOutRegVec[idx] = UINT_MAX;
              liveOutRegMap.erase(liveOutRegMapIt);
            }
          }
        };

        if (dst->getBase()->isRegAllocPartaker()) {
          G4_DstRegRegion *dstrgn = dst;
          G4_RegVar *var = dstrgn->getBase()->asRegVar();
          varID = var->getId();
          dcl = GetTopDclFromRegRegion(dstrgn);
          vISA_ASSERT(dcl != nullptr, "Null declare found");
          var = dcl->getRegVar();

          vISA_ASSERT(var->getId() == varID,
                      "RA verification error: Invalid regVar ID!");
          vISA_ASSERT(var->getPhyReg()->isGreg(),
                      "RA verification error: Invalid dst reg!");

          uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
          uint32_t regOff = var->getPhyRegOff();
          bool suppressWarning =
              false; // used to suppress warning per word since each operand
                     // crosses multiple words

          // For spill/fill variable, should check whole region size
          if (var->isRegVarTransient()) {
            uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                           (regOff * dcl->getElemSize()) / G4_WSIZE;

            for (uint32_t i = 0; i < dcl->getWordSize(); ++i, ++idx) {
              verifyDstRA(idx, regNum, regOff, suppressWarning);
            }

            if (spillFillVariableOverwriteSet.find(dcl) ==
                spillFillVariableOverwriteSet.end()) {
              spillFillVariableOverwriteSet.insert(dcl);
            }
          }
          // For non spill/fill variables, should check operand size
          else {
            auto dstLB = dst->getLinearizedStart();
            auto dstRB = dst->getLinearizedEnd();

            for (unsigned int dstOffset = dstLB; dstOffset <= dstRB;
                 dstOffset += G4_WSIZE) {
              uint32_t idx = dstOffset / G4_WSIZE;
              verifyDstRA(idx, regNum, regOff, suppressWarning);
            }
          }
        } else if (dst->getRegAccess() == IndirGRF) {
          G4_DstRegRegion *dstrgn = dst;
          G4_Declare *addrdcl = GetTopDclFromRegRegion(dstrgn);
          G4_RegVar *ptvar = nullptr;
          int vid = 0;

          while ((ptvar = pointsToAnalysis.getPointsTo(addrdcl->getRegVar(),
                                                       vid++)) != nullptr) {
            varID = ptvar->getId();
            dcl = ptvar->getDeclare();
            vISA_ASSERT(dcl != nullptr, "Null declare found");
            while (dcl->getAliasDeclare()) {
              dcl = dcl->getAliasDeclare();
            }
            G4_RegVar *var = dcl->getRegVar();

            vISA_ASSERT(var->getId() == varID,
                        "RA verification error: Invalid regVar ID!");
            vISA_ASSERT(var->getPhyReg()->isGreg(),
                        "RA verification error: Invalid dst reg!");

            uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
            uint32_t regOff = var->getPhyRegOff();
            auto dstLB = regNum * kernel.numEltPerGRF<Type_UB>() +
                         regOff * TypeSize(dcl->getElemType());
            auto dstRB = dstLB + dcl->getByteSize() - 1;
            uint32_t idx = dstLB / G4_WSIZE;

            bool suppressWarning = false;
            for (unsigned int dstOffset = dstLB; dstOffset <= dstRB;
                 dstOffset += std::max((unsigned int)dst->getExecTypeSize(),
                                       (unsigned int)G4_WSIZE)) {
              for (unsigned int elementOffset = 0;
                   elementOffset < dst->getElemSize();
                   elementOffset += G4_WSIZE) {
                idx = (dstOffset + elementOffset) / G4_WSIZE;

                verifyDstRA(idx, regNum, regOff, suppressWarning);
              }
            }
          }
        }
      }

      if (inst->opcode() == G4_pseudo_fcall) {
        if (ret != nullptr && ret->getRegVar() != nullptr) {
          G4_RegVar *var = ret->getRegVar();
          [[maybe_unused]] uint32_t varID = var->getId();
          vISA_ASSERT(var->getId() == varID,
                      "RA verification error: Invalid regVar ID!");
          vISA_ASSERT(var->getPhyReg()->isGreg(),
                      "RA verification error: Invalid dst reg!");

          uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
          uint32_t regOff = var->getPhyRegOff();

          uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                         regOff * ret->getElemSize() / G4_WSIZE;
          for (uint32_t i = 0; i < ret->getWordSize(); ++i, ++idx) {
            liveOutRegMapIt = liveOutRegMap.find(idx);
            liveOutRegVec[idx] = UINT_MAX;
            vISA_ASSERT(
                liveOutRegMapIt != liveOutRegMap.end(),
                "RA verification error: Invalid entry in liveOutRegMap!");
            liveOutRegMap.erase(liveOutRegMapIt);
          }
        }
      }

      // verify each source operand
      for (unsigned j = 0, numSrc = inst->getNumSrc(); j < numSrc; j++) {
        G4_Operand *src = inst->getSrc(j);
        if (!src)
          continue;

        uint32_t varID = UINT_MAX;
        G4_Declare *dcl = nullptr;
        auto verifySrcRA = [&](uint32_t idx, uint32_t regNum, uint32_t regOff) {
          vISA_ASSERT(dcl && dcl->getRegVar() &&
                          dcl->getRegVar()->isPhyRegAssigned(),
                      "RA verification error: Found src variable without "
                      "physical register.");
          liveOutRegMapIt = liveOutRegMap.find(idx);
          if (liveOutRegVec[idx] == UINT_MAX) {
            liveOutRegVec[idx] = varID;
            vISA_ASSERT(
                liveOutRegMapIt == liveOutRegMap.end(),
                "RA verification error: Invalid entry in liveOutRegMap!");
            liveOutRegMap.emplace(idx, dcl);
          } else {
            if (liveOutRegVec[idx] != varID) {
              const SparseBitVector &indr_use = liveAnalysis.indr_use[bb->getId()];

              if (dcl->isInput()) {
                vISA_ASSERT(false,
                            "RA verification error: Found conflicting input "
                            "variables: %s and %s assigned to r%d.%d",
                            dcl->getName(), liveOutRegMapIt->second->getName(),
                            regNum, regOff);
                liveOutRegVec[idx] = varID;
                liveOutRegMapIt->second = dcl;
              } else if (strstr(dcl->getName(), GlobalRA::StackCallStr) !=
                         nullptr) {
                vISA_ASSERT(false,
                            "RA verification error: Found conflicting "
                            "stackCall variables: %s and %s assigned to r%d.%d",
                            dcl->getName(), liveOutRegMapIt->second->getName(),
                            regNum, regOff);
              } else if (indr_use.test(liveOutRegVec[idx]) == true) {
                vISA_ASSERT(false,
                            "RA verification error: Found conflicting indirect "
                            "variables: %s and %s assigned to r %d.%d",
                            dcl->getName(), liveOutRegMapIt->second->getName(),
                            regNum, regOff);
              } else {
                INST_LIST::reverse_iterator succ = rit;
                ++succ;
                bool idMismatch = false;
                G4_Declare *topdcl = GetTopDclFromRegRegion((*succ)->getDst());
                if (topdcl != nullptr &&
                    liveOutRegVec[idx] != topdcl->getRegVar()->getId()) {
                  idMismatch = true;
                }
                if (succ == bb->rbegin() || !(*succ)->isPseudoKill() ||
                    (*succ)->getDst() == nullptr || idMismatch) {
                  vISA_ASSERT(
                      false, "RA verification error: Found conflicting \
                      variables: %s and %s assigned to r%d.%d",
                      dcl->getName(), liveOutRegMapIt->second->getName(),
                      regNum, regOff);
                }
              }
            }
          }
        };

        if (src->isSrcRegRegion() &&
            src->asSrcRegRegion()->getBase()->isRegAllocPartaker()) {
          G4_SrcRegRegion *srcrgn = src->asSrcRegRegion();
          G4_RegVar *var = srcrgn->getBase()->asRegVar();
          varID = var->getId();
          dcl = GetTopDclFromRegRegion(srcrgn);
          var = dcl->getRegVar();
          vISA_ASSERT(var->getId() == varID,
                      "RA verification error: Invalid regVar ID!");
          vISA_ASSERT(var->getPhyReg()->isGreg(),
                      "RA verification error: Invalid dst reg!");

          if (!inst->isLifeTimeEnd()) {
            uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
            uint32_t regOff = var->getPhyRegOff();

            // For spill/fill variable, should check whole region size
            if (var->isRegVarTransient()) {
              uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                             (regOff * dcl->getElemSize()) / G4_WSIZE;
              for (uint32_t i = 0; i < dcl->getWordSize(); ++i, ++idx) {
                verifySrcRA(idx, regNum, regOff);
              }
            }
            // For non spill/fill variables, should check operand size
            else {
              auto srcLB = src->getLinearizedStart();
              auto srcRB = src->getLinearizedEnd();
              for (unsigned int srcOffset = srcLB; srcOffset <= srcRB;
                   srcOffset += G4_WSIZE) {
                unsigned idx = srcOffset / G4_WSIZE;
                verifySrcRA(idx, regNum, regOff);
              }
            }
          } else {
            uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
            uint32_t regOff = var->getPhyRegOff();

            uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                           (regOff * dcl->getElemSize()) / G4_WSIZE;
            for (uint32_t i = 0; i < dcl->getWordSize(); ++i, ++idx) {
              if (liveOutRegVec[idx] != UINT_MAX) {
                liveOutRegMapIt = liveOutRegMap.find(idx);
                vISA_ASSERT(
                    liveOutRegMapIt != liveOutRegMap.end(),
                    "RA verification error: Invalid entry in liveOutRegMap!");
                vISA_ASSERT(false,
                            "RA verification error: Found live variable: %s "
                            "after lifetime_end assigned to r %d.%d",
                            dcl->getName(), regNum, regOff);
              }
            }
          }

          // verify EOT source
          if (inst->isEOT() && kernel.fg.builder->hasEOTGRFBinding()) {
            [[maybe_unused]] uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
            vISA_ASSERT(
                regNum >= 112,
                "RA verification error: EOT source: %s is assigned to r.%d",
                dcl->getName(), regNum);
          }
        } else if (src->isSrcRegRegion() && src->isIndirect()) {
          G4_SrcRegRegion *srcrgn = src->asSrcRegRegion();
          G4_Declare *addrdcl = GetTopDclFromRegRegion(srcrgn);
          G4_RegVar *ptvar = nullptr;
          int vid = 0;

          while ((ptvar = pointsToAnalysis.getPointsTo(addrdcl->getRegVar(),
                                                       vid++)) != nullptr) {
            varID = ptvar->getId();
            dcl = ptvar->getDeclare()->getRootDeclare();
            G4_RegVar *var = dcl->getRegVar();

            uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
            uint32_t regOff = var->getPhyRegOff();

            uint32_t idx = regNum * kernel.numEltPerGRF<Type_UW>() +
                           (regOff * dcl->getElemSize()) / G4_WSIZE;
            for (uint32_t i = 0; i < dcl->getWordSize(); ++i, ++idx) {
              liveOutRegMapIt = liveOutRegMap.find(idx);
              if (liveOutRegVec[idx] == UINT_MAX) {
                liveOutRegVec[idx] = varID;
                vISA_ASSERT(
                    liveOutRegMapIt == liveOutRegMap.end(),
                    "RA verification error: Invalid entry in liveOutRegMap!");
                liveOutRegMap.emplace(idx, dcl);
              } else {
                if (liveOutRegVec[idx] != varID) {
                  const SparseBitVector &indr_use =
                      liveAnalysis.indr_use[bb->getId()];

                  if (dcl->isInput()) {
                    vISA_ASSERT(false,
                                "RA verification error: Found conflicting "
                                "input variables: %s and %s assigned to r%d.%d",
                                dcl->getName(),
                                liveOutRegMapIt->second->getName(), regNum,
                                regOff);
                    liveOutRegVec[idx] = varID;
                    liveOutRegMapIt->second = dcl;
                  } else if (indr_use.test(liveOutRegVec[idx]) == true) {
                    vISA_ASSERT(
                        false,
                        "RA verification error: Found conflicting indirect "
                        "variables: %s and %s assigned to r%d.%d",
                        dcl->getName(), liveOutRegMapIt->second->getName(),
                        regNum, regOff);
                  } else {
                    vISA_ASSERT(
                        false,
                        "RA verification error: Found conflicting variables: "
                        "%s and %s assigned to r %d.%s and %s assigned to "
                        "r%d.%d",
                        dcl->getName(), liveOutRegMapIt->second->getName(),
                        regNum, dcl->getName(),
                        liveOutRegMapIt->second->getName(), regNum, regOff);
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

void GlobalRA::verifySpillFill() {
  // Verify that whenever LSC or OW is used for spill in divergent CF, it is
  // preceeded by corresponding fill.
  for (auto *BB : kernel.fg.getBBList()) {
    auto InstIter = BB->begin();
    while (InstIter != BB->end()) {
      auto *Inst = (*InstIter);
      if (!Inst->isSpillIntrinsic()) {
        ++InstIter;
        continue;
      }

      bool UsesLSC =
          useLscForNonStackCallSpillFill || spillFillIntrinUsesLSC(Inst);
      bool UsesOW = (Inst->asSpillIntrinsic()->getOffset() ==
                     G4_SpillIntrinsic::InvalidOffset);
      bool UsesHW = !UsesLSC && !UsesOW;

      auto Offset = Inst->asSpillIntrinsic()->getOffset();
      auto Rows = Inst->asSpillIntrinsic()->getNumRows();

      auto CheckRMWFill = [&]() {
        // InstIter points to spill. Check for preceeding fill.
        bool FillFound = false;
        auto PrevInstIter = InstIter;
        --PrevInstIter;
        while (PrevInstIter != BB->begin()) {
          auto PrevInst = (*PrevInstIter);

          if (PrevInst->isFillIntrinsic()) {
            // If we see same Offset/Rows combination, we're safe
            auto OffsetPrev = PrevInst->asFillIntrinsic()->getOffset();
            auto RowsPrev = PrevInst->asFillIntrinsic()->getNumRows();

            if (OffsetPrev == Offset && RowsPrev == Rows) {
              FillFound = true;
              break;
            }
          }
          --PrevInstIter;
        }

        if (!FillFound) {
          std::cerr << "Didn't find RMW fill corresponding to spill at $"
                    << Inst->getVISAId() << std::endl;
          Inst->dump();
          std::cerr << std::endl;
        }
      };

      auto CheckWriteEnable = [&]() {
        if (!Inst->isWriteEnableInst()) {
          Inst->dump();
          std::cerr << "Expecting WriteEnable to be set on spill at $"
                    << Inst->getVISAId() << std::endl;
          std::cerr << std::endl;
        }
      };

      if (!UsesHW) {
        // For OW and LSC, RMW fill should exist, unless RMW opt in
        // spill insertion (correctly) decided otherwise. We cannot
        // lookup decision of RMW opt here, so we may see some false
        // positives.
        CheckRMWFill();
        // OW and LSC must use WriteEnable
        CheckWriteEnable();
      }
      ++InstIter;
    }
  }
}

static void replaceSSO(G4_Kernel &kernel) {
  // Invoke function only for XeHP_SDV and later
  // Replace SSO with r126.7:ud (scratch reg) up to VISA ABI v2
  if (!kernel.fg.builder->getSpillSurfaceOffset())
    return;

  auto dst = kernel.fg.builder->createDst(
      kernel.fg.getScratchRegDcl()->getRegVar(), 0, 7, 1, Type_UD);
  for (auto bb : kernel.fg) {
    for (auto instIt = bb->begin(); instIt != bb->end(); instIt++) {
      auto inst = (*instIt);
      if (inst->getDst() && inst->getDst()->getTopDcl() ==
                                kernel.fg.builder->getSpillSurfaceOffset()) {
        if (kernel.fg.getIsStackCallFunc()) {
          instIt = bb->erase(instIt);
          --instIt;
        } else
          inst->setDest(dst);

        // if an earlier pass inserted pseudokill for SSO dcl, remove it
        // but our final target is the instruction actually defining SSO.
        if (inst->isPseudoKill())
          continue;

        // Also update scratch msg dcl to be an alias
        kernel.fg.builder->getSpillSurfaceOffset()->setAliasDeclare(
            kernel.fg.getScratchRegDcl(), 7 * TypeSize(Type_UD));

        return;
      }
    }
  }
}

// Entry point for all RA flavors.
int regAlloc(IR_Builder &builder, PhyRegPool &regPool, G4_Kernel &kernel) {
  kernel.fg.callerSaveAreaOffset = kernel.fg.calleeSaveAreaOffset =
      kernel.fg.frameSizeInOWord = 0;

#if defined(_DEBUG) || defined(_INTERNAL)
  vISA::RATraceFlag = builder.getOption(vISA_RATrace);
#endif

  // This must be done before Points-to analysis as it may modify CFG and add
  // new BB!
  if (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc()) {
    kernel.fg.setABIForStackCallFunctionCalls();
    kernel.fg.addFrameSetupDeclares(builder, regPool);
    kernel.fg.normalizeFlowGraph();
    if (builder.getPlatform() >= Xe_XeHPSDV)
      replaceSSO(kernel);
  }

  if (kernel.getOption(vISA_DoSplitOnSpill)) {
    // loop computation is done here because we may need to add
    // new preheader BBs. later parts of RA assume no change
    // to CFG structure.
    kernel.fg.getLoops().computePreheaders();
  }

  kernel.fg.reassignBlockIDs();
  // do it once for whole RA pass. Assumption is RA should not modify CFG at all
  kernel.fg.setPhysicalPredSucc();

  if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_3D) {
    kernel.fg.findNaturalLoops();
  }

  //
  // Perform flow-insensitive points-to-analysis.
  //
  PointsToAnalysis pointsToAnalysis(kernel.Declares, kernel.fg.getNumBB());
  pointsToAnalysis.doPointsToAnalysis(kernel.fg);
  GlobalRA gra(kernel, regPool, pointsToAnalysis);

  //
  // insert pseudo save/restore return address so that reg alloc
  // can assign registers to hold the return addresses
  //
  gra.assignLocForReturnAddr();


  //
  // Mark block local variables for the whole graph prior to performing liveness
  // analysis.
  // 1. Is required for flag/address register allocation
  // 2. We must make sure the reference number, reference BB(which will be
  // identified in local RA as well) happens only one time. Otherwise, there
  // will be correctness issue
  gra.markGraphBlockLocalVars();

  // Remove the un-referenced declares
  gra.removeUnreferencedDcls();

  if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_CM) {
    kernel.fg.markScope();
  }

  //
  // perform graph coloring for whole program
  //

  if (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc()) {
    kernel.fg.addSaveRestorePseudoDeclares(builder);
    gra.fixSrc0IndirFcall();
  }

  int status = gra.coloringRegAlloc();

  if (auto jitInfo = builder.getJitInfo()) {
    jitInfo->numBytesScratchGtpin =
        kernel.getGTPinData()->getNumBytesScratchUse();
    // verify that spill memory used is within platform's acceptable limit
    unsigned int totalScratchUsed =
        jitInfo->stats.spillMemUsed +
        kernel.getInt32KernelAttr(Attributes::ATTR_SpillMemOffset);
    if (totalScratchUsed > builder.getMaxPTSS()) {
      builder.criticalMsgStream()
          << "Total scratch size used by shader exceeds platform capability: "
          << totalScratchUsed << "\n";
      return VISA_SPILL;
    }
  }

  // propagate address takens to gtpin info
  std::unordered_map<const G4_Declare *, std::vector<G4_Declare *>> addrTakenMap;
  pointsToAnalysis.getPointsToMap(addrTakenMap);
  auto gtpinData = kernel.getGTPinData();
  for (auto &indirRef : addrTakenMap) {
    for (auto target : indirRef.second)
      gtpinData->addIndirRef(indirRef.first, target);
  }

  if (status != VISA_SUCCESS) {
    return status;
  }

  if (builder.getOption(vISA_VerifyRA)) {
    // Mark local variables to improve the liveness analysis on partially
    // written local variables.
    // For example, for some payload decls, the region is not completely
    // written in the setup. That may cause gen is not blocked by the setup
    // inst, and the verifier might wrongly report a conflict assignment.
    //
    // clang-format off
    //   .declare M9 (2547)  rf=r size=32 type=ud align=16 words
    //
    //   (W) mov (1)              M9(0,2)<1>:ud  0xe0:uw // $211:
    //   (W) send (8)             V0173(0,0)<1>:d M9(0,0) 0x2184200:ud unaligned oword block read, dstLen=1, src0Len=1
    // clang-format on
    gra.markGraphBlockLocalVars();
    LivenessAnalysis liveAnalysis(gra, G4_GRF | G4_INPUT, true);
    liveAnalysis.computeLiveness();
    // Mark scope so that verifier can leverage the scope information to
    // verify live-in and live-out.
    kernel.fg.markScope();
    gra.verifyRA(liveAnalysis);
  }

  // printf("EU Fusion WA insts for func: %s\n", kernel.getName());
  for (auto inst : gra.getEUFusionCallWAInsts()) {
    kernel.setMaskOffset(inst, InstOpt_M16);
  }

#if defined(_DEBUG)
  for (auto inst : gra.getEUFusionNoMaskWAInsts()) {
    if (inst->getPredicate() != nullptr || inst->getCondMod() != nullptr) {
      vISA_ASSERT(false,
                  "Don't expect either predicate nor condmod for WA insts!");
    }
  }
#endif

  return status;
}
