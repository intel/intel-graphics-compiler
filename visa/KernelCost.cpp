/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file contains the implementation of frequency info pass.

#include "KernelCost.hpp"
#include "G4_IR.hpp"
#include "G4_Kernel.hpp"
#include "FlowGraph.h"

#include <list>
#include <unordered_map>
#include <iostream>
#include <algorithm>

using namespace vISA;

KernelCost::KernelCost(G4_Kernel* pK, std::vector<VISA_BB_INFO> &BBInfo)
    : m_kernel(pK), m_loops(pK->fg.getLoops()) {
  vISA_ASSERT(BBInfo.size() == pK->fg.getNumBB(),
    "KernelCost(): invliad the number of BBs");
  // Set up cost info for each BB
  for (auto BB : pK->fg.getBBList()) {
    uint32_t id = BB->getId();
    BBCostInfo BCI;
    BCI.m_cycles = BBInfo[id].staticCycle;
    BCI.m_prob = 0;
    m_BBCostInfo.insert(std::make_pair(BB, BCI));
  }
}

void KernelCost::run() {
  m_loops.setStale();
  m_loops.recomputeIfStale();

  init();

  calculateProb();

  collectPerfMetrics();

  // save data into G4_Kernel for IGC to access
  m_kernel->createKernelCostInfo(this);
}

// DFS_PO: Post-order traversal and record each BB in RPOT vector
void KernelCost::DFS_PO(G4_BB *BB) {
  if (visited[BB] == 1)
    return;
  visited[BB] = 1;
  if (BB->getBBType() & G4_BB_CALL_TYPE) {
    G4_BB *N = BB->getPhysicalSucc();
    DFS_PO(N);
  } else if (!(BB->getBBType() & G4_BB_EXIT_TYPE)) {
    for (G4_BB *S : BB->Succs)
      DFS_PO(S);
  }
  // BB in the increasing PO order, and thus its reverse is in RPOT order.
  RPOT.push_back(BB);
}

void KernelCost::doRPOT(G4_BB *EntryBB) {
  RPOT.clear();
  visited.clear();

  DFS_PO(EntryBB);
  std::reverse(RPOT.begin(), RPOT.end());
  assert(EntryBB == RPOT.front());
}

// Propagate prob into BB's succ.
//   If L is not nullptr, it means this BB is inside a loop and BB's succ
//   to L's header is ignored and to any block outside L is also ignored.
void KernelCost::propagateBBProb(G4_BB* BB, Loop *L) {
  visited[BB] = 1;

  // Special case
  if (BB->getBBType() & G4_BB_CALL_TYPE) {
    G4_BB* S = BB->getPhysicalSucc();
    updateBBProb(S, getBBProb(BB));
    return;
  } else if (BB->getBBType() & G4_BB_EXIT_TYPE) {
    return;
  }

  std::vector<ProbType> SuccProb;
  getSuccEdgeProb(BB, SuccProb);

  if (SuccProb.size() == 0) {
      // No succ that needs propagation
    return;
  }

  vISA_ASSERT(SuccProb.size() == BB->Succs.size(),
              "Succ's prob has wrong vector size");
  int i = -1;
  for (G4_BB *S : BB->Succs) {
    ++i;
    // Skip back-edge as it does not affect header's prob
    if (L) {
      if (S == L->getHeader())
        continue;
      // Skip BBs that are outside the loop
      if (!L->contains(S))
        continue;
    }
    updateBBProb(S, SuccProb[i]);
  }
}

// Redo exit bb's prob
//   1. loop's immediate exit is dependent upon the loop header's prob
//   2. where the exit target is does not matter
void KernelCost::propagateLoopProb(Loop *L, int RPOT_pos) {
  auto getLoopExitProb = [this](Loop *L, G4_BB *ExitBB) {
    ProbType retProb = 0;
    for (G4_BB *B : ExitBB->Preds) {
      Loop *iL = m_loops.getInnerMostLoop(B);
      if (iL && iL == L) {
        std::vector<ProbType> tProb;
        getSuccEdgeProb(B, tProb);
        if (tProb.size() == 0)
          continue;
        int i = 0;
        for (G4_BB *S : B->Succs) {
          if (S == ExitBB) {
            retProb += tProb[i];
            break;
          }
          ++i;
        }
      }
    }
    return retProb;
  };

  G4_BB *H = L->getHeader();
  for (int i = RPOT_pos; i < (int)RPOT.size(); ++i) {
    G4_BB *BB = RPOT[i];
    // visited BBs inside this loop
    if (visited[BB] || !L->contains(BB))
      continue;

    // Recusively handle immediate child loop
    Loop *CL = L->getOuterMostChildLoop(BB);
    if (CL && CL->getHeader() == BB)
      propagateLoopProb(CL, i);
    else if (!CL)
      propagateBBProb(BB, L);
  }

  // Calculate exits' probability. Only loop's exit BBs that are 1 level up
  // is considered. For any exit that is more than 1 level up, it will be
  // handled when the outer loop exits are handled.
  std::vector<G4_BB *> &lpExits = L->getLoopExits();
  std::vector<ProbType> immExitProbs;
  std::vector<G4_BB *> immExitBBs;
  uint32_t L_level = L->getNestingLevel();
  ProbType totalP = 0;
  for (G4_BB *B : lpExits) {
    Loop *tL = m_loops.getInnerMostLoop(B);
    if ((!tL && !L->parent) ||
        (tL && tL->getNestingLevel() == (L_level + 1))) {
      immExitBBs.push_back(B);
      ProbType tProb = getLoopExitProb(L, B);
      immExitProbs.push_back(tProb);
      totalP += tProb;
    }
  }

  // Normalize exit BB's probability so that sum of all prob(immediateExitBB)
  // is the same as prob(H).
  //    L0 :  P=1
  //       L1 :  P=0.5
  //          goto E0
  //          goto E1
  //       E1:
  //    E0
  // The following code will have Prob(E1) = 0.5 and Prob(E0) = 1 by handling
  // E1 for loop L1 and E0 for loop L0 (L1 does not handle E0!).
  //
  if (immExitProbs.size() > 0) {
    ProbType P = getBBProb(H);
    // totalP should not be larger than P. Here make sure it is the case.
    totalP = std::min(totalP, P);
    float factor = (float)P / totalP;
    ProbType P0 = P;
    for (int i = 1; i < (int)immExitProbs.size(); ++i) {
      G4_BB *B = immExitBBs[i];
      ProbType tP = (ProbType)(factor * immExitProbs[i]);
      updateBBProb(B, tP);
      // make sure no overflow
      P0 = (P0 >= tP ? P0 - tP : 0);
    }
    G4_BB *B0 = immExitBBs[0];
    updateBBProb(B0, P0);
  }
}

void KernelCost::calculateProb() {
  //   Each function's probability is calculated independently,
  //   assuming their entry prob is 1.

  // Leaf subroutine is calculated first, and kernel is cacluated last.
  for (int f = 0; f < (int)m_metrics.size(); ++f) {
    G4_BB *StartBB = m_metrics[f].m_funcInfo->getInitBB();

    doRPOT(StartBB);

    // Entry has prob 1 always.
    updateBBProb(StartBB, MAX_PROB_POINTS);

    // Start propagation in Rerverse Post-order traveral.
    visited.clear();
    for (int i = 0; i < (int)RPOT.size(); ++i) {
      G4_BB *BB = RPOT[i];
      if (visited[BB])
        continue;
      Loop *L = m_loops.getOuterMostLoop(BB);
      if (L && L->getHeader() == BB)
        propagateLoopProb(L, i);
      else if (!L)
        propagateBBProb(BB);
    }
  }
}

void KernelCost::getSuccEdgeProb(G4_BB *BB,
                                        std::vector<ProbType>& SuccEdgeProb) {
  ProbType P = getBBProb(BB);
  uint32_t nsuccs = (uint32_t)BB->Succs.size();

  if (nsuccs == 1 || (BB->getBBType() & G4_BB_CALL_TYPE)) {
    SuccEdgeProb.push_back(P);
    return;
  }
  if (nsuccs == 0 || (BB->getBBType() & G4_BB_EXIT_TYPE))
    return;

  G4_InstCF *br = BB->getLastCFInst();
  if (br && nsuccs == 2) {
    // common case
    if (!br->isUniform()) {
      SuccEdgeProb.insert(SuccEdgeProb.end(), nsuccs, P);
      return;
    }

    float takenP = 0.5;
    bool defByDst;
    G4_INST *predDefI = getBranchFlagLocalDef(BB, defByDst);
    if (predDefI && !defByDst) {
      G4_Predicate *Pred = br->getPredicate();
      bool isNegPred = (Pred->getState() == G4_PredState::PredState_Minus);
      G4_CondMod *cMod = predDefI->getCondMod();
      vISA_ASSERT(cMod, "ICE: condMod should be preset\n");
      switch (cMod->getMod()) {
      case G4_CondModifier::Mod_z:
      case G4_CondModifier::Mod_e:
      case G4_CondModifier::Mod_o:
      case G4_CondModifier::Mod_u:
        // ==, overflow, unordered : unlikely
        takenP = isNegPred ? 0.90f : 0.1f;
        break;
      case G4_CondModifier::Mod_nz:
      case G4_CondModifier::Mod_ne:
        // != : likely
        takenP = isNegPred ? 0.1f : 0.9f;
        break;
      default:
        break;
      }
    }
    ProbType target_P = (ProbType)(takenP * P);
    ProbType fallthru_P = (P >= target_P ? P - target_P : 0);
    SuccEdgeProb.push_back(fallthru_P);
    SuccEdgeProb.push_back(target_P);
    return;
  }

  // Shouldn't reach this place, but do it in case.
  SuccEdgeProb.resize(nsuccs);
  // Evenly assign probability to each edge
  //   (Nice to keep the sum of all succs' prob the same as BB's)
  ProbType total = 0;
  ProbType eachP = (P / nsuccs);
  for (int i = 1; i < (int)nsuccs; ++i) {
    SuccEdgeProb[i] = eachP;
    total += eachP;
  }
  SuccEdgeProb[0] = (P >= total ? P - total : 0);
  return;
}

G4_INST* KernelCost::getBranchFlagLocalDef(G4_BB* BB, bool& DefByDst) {
  DefByDst = false;
  G4_InstCF *br = BB->getLastCFInst();
  if (!br)
    return nullptr;
  G4_Predicate *P = br->getPredicate();
  if (!P)
    return nullptr;

  // RS: reverse iterator to br
  auto RS = BB->rbegin();
  for (auto RE = BB->rend(), RI = ++RS; RI != RE; ++RI) {
    G4_INST *I = *RI;
    G4_Operand *Dst = I->getDst();
    G4_Operand *cMod = I->getCondMod();
    if (Dst && !I->hasNULLDst() &&
        P->compareOperand(Dst, I->getBuilder()) != Rel_disjoint) {
      DefByDst = true;
      return I;
    }
    if (cMod && P->compareOperand(cMod, I->getBuilder()) != Rel_disjoint)
      return I;
  }
  return nullptr;
}

void KernelCost::calculateBBMetrics(CostMetricsWrapper& CM, G4_BB* BB)
{
  BBCostInfo &BInfo = m_BBCostInfo[BB];
  float factor = BInfo.m_prob / (float)MAX_PROB_POINTS;

  // make sure no BBs have 0 cycles!
  uint32_t cycles = std::max(1u, (uint32_t)(BInfo.m_cycles * factor));
  CM.setCycles(cycles);

  uint32_t loadBytes = 0;
  uint32_t storeBytes = 0;
  for (auto Inst : BB->getInstList()) {
    if (Inst->isSend()) {
      uint32_t rBytes = 0, wBytes = 0;
      collectSendMetrics(Inst->asSendInst(), rBytes, wBytes);
      loadBytes += rBytes;
      storeBytes += wBytes;
    }
  }

  // If loads/stores are not 0, make sure the final result is at least 1.
  if (loadBytes > 0)
    loadBytes = std::max(1u, (uint32_t)(loadBytes * factor));
  if (storeBytes > 0)
    storeBytes = std::max(1u, (uint32_t)(storeBytes * factor));
  CM.setLoadBytes(loadBytes);
  CM.setStoreBytes(storeBytes);;

  if (BB->isEndWithCall()) {
    FuncInfo *pFI = BB->getCalleeInfo();
    int ix = m_funcIndex[pFI];
    CostMetricsWrapper &calleeCM = m_metrics[ix].m_estimateCost;
    CM.add(calleeCM, BInfo.m_prob);
  }
}

void KernelCost::init()
{
  // Set up info for all subroutines
  int numFuncs = (int)m_kernel->fg.sortedFuncTable.size();
  if (numFuncs > 0) {
    m_metrics.resize(numFuncs);
    for (int i = 0; i < numFuncs; ++i) {
      FuncInfo *pFInfo = m_kernel->fg.sortedFuncTable[i];
      m_metrics[i].m_funcInfo = pFInfo;
      m_funcIndex[pFInfo] = i;
    }
  } else {
    m_metrics.resize(1);
    FuncInfo *kernelInfo = m_kernel->fg.kernelInfo;
    m_metrics[0].m_funcInfo = kernelInfo;
    m_funcIndex[kernelInfo] = 0;
    numFuncs = 1;
  }

  // set up loops for kernel/subroutine in program order
  BB_LIST &BBs = m_kernel->fg.getBBList();
  for (int i = 0; i < numFuncs; ++i) {
    FuncCost &FC = m_metrics[i];
    FuncInfo *pFI = FC.m_funcInfo;
    G4_BB *StartBB = pFI->getInitBB();
    G4_BB *EndBB = pFI->getExitBB();
    auto IS = std::find(BBs.begin(), BBs.end(), StartBB);
    auto IE = std::find(BBs.begin(), BBs.end(), EndBB);
    IE = std::next(IE);
    for (auto II = IS; II != IE; ++II) {
      G4_BB *BB = *II;

      // all the loops with the same header : BB
      std::list<Loop *> lps;
      Loop *L = m_loops.getInnerMostLoop(BB);
      while (L && L->getHeader() == BB) {
        lps.push_front(L);
        L = L->parent;
      }
      if (!lps.empty()) {
        int nextIdx = (int)FC.m_allLoopsInProgramOrder.size();
        for (auto al : lps) {
          FC.m_allLoopsInProgramOrder.push_back(al);
          LoopCost &LC = m_loopCosts[al];

          G4_BB *backedgeSrc = al->backEdgeSrc();
          int visaid = -1;
          if (!al->backEdgeSrc()->empty()) {
            G4_INST *br = backedgeSrc->back();
            visaid = br->getVISAId();
          }
          LC.m_backedge_visaId = visaid;
          LC.m_loopId = nextIdx;

          ++nextIdx;
        }
      }
    }
  }
}

void KernelCost::collectLoopMetrics(Loop *L) {
  LoopCost &LC = m_loopCosts[L];
  for (G4_BB *bb : L->getBBs()) {
    Loop *lp = L->getOuterMostChildLoop(bb);
    if (lp && lp->getHeader() == bb) {
      collectLoopMetrics(lp);

      LoopCost &aLC = m_loopCosts[lp];
      LC.m_estimateCost.add(aLC.m_estimateCost);
    } else if (!lp) {
      CostMetricsWrapper BCM;
      calculateBBMetrics(BCM, bb);
      LC.m_loopBodyCost.C.add(BCM);
    }
  }
  LC.m_estimateCost.add(LC.m_loopBodyCost.C);

  // Estimate loop cost assuming loop count = 16
  LC.m_estimateCost.mul(16);
}

void KernelCost::collectPerfMetrics()
{
  BB_LIST& BBs = m_kernel->fg.getBBList();
  const int numFuncs = (int)m_metrics.size();
  for (int i = 0; i < numFuncs; ++i) {
    FuncCost &FC = m_metrics[i];
    FuncInfo *pFI = FC.m_funcInfo;
    G4_BB *StartBB = pFI->getInitBB();
    G4_BB *EndBB = pFI->getExitBB();
    auto IS = std::find(BBs.begin(), BBs.end(), StartBB);
    auto IE = std::find(BBs.begin(), BBs.end(), EndBB);
    IE = std::next(IE);
    for (auto II = IS; II != IE; ++II) {
      G4_BB *BB = *II;
      Loop *L = m_loops.getOuterMostLoop(BB);
      if (L && L->getHeader() == BB) {
        collectLoopMetrics(L);

        LoopCost &LC = m_loopCosts[L];
        FC.m_estimateCost.add(LC.m_estimateCost);
      } else if (!L) {
        CostMetricsWrapper BCM;
        calculateBBMetrics(BCM, BB);
        FC.m_funcCost.C.add(BCM);
      }
    }
    FC.m_estimateCost.add(FC.m_funcCost.C);

    // Loop part of cost expr, set up in program order.
    for (auto L : FC.m_allLoopsInProgramOrder) {
      if (L->getNumImmChildLoops() == 0)
        continue;

      LoopCost &LC = m_loopCosts[L];
      CostExprInternal &lce = LC.m_loopBodyCost;

      // make sure to iterate loops in program order
      std::list<Loop *> immLoops(L->begin(), L->end());
      immLoops.sort([this](Loop *a, Loop *b) {
        LoopCost *aLC = &m_loopCosts[a];
        LoopCost *bLC = &m_loopCosts[b];
        return aLC->m_loopId < bLC->m_loopId;
      });

      for (auto LI : immLoops) {
        Loop *nested = LI;
        LoopCost &nestedLC = m_loopCosts[nested];
        lce.LoopCosts.push_back(&nestedLC);
      }
    }
  }

  if (m_kernel->getOption(vISA_dumpKCI) ||
      m_kernel->getOption(vISA_dumpDetailKCI))
    print(std::cout);

  if (m_kernel->getOption(vISA_dumpKCIForLit)) {
    printForLit(std::cout);
  }
}

void KernelCost::collectSendMetrics(G4_InstSend* SendI, uint32_t& ldBytes, uint32_t& stBytes)
{
  // Todo: make sure the bytes are calculated correctly!
  ldBytes = 0;
  stBytes = 0;
  G4_SendDesc *mDesc = SendI->getMsgDesc();
  if (mDesc->isRead() || mDesc->isReadWrite()) {
    ldBytes = mDesc->getDstLenBytes();
  }
  if (mDesc->isWrite() || mDesc->isReadWrite()) {
    // Make sure this is correct.
    stBytes = mDesc->getSrc1LenBytes();
  }
}

void KernelCost::print(std::ostream &OS)
{
  if (m_kernel->getOption(vISA_dumpDetailKCI)) {
    // Dump G4
    m_kernel->dumpToFile("KernelCost", true);

    // dump detail info for each BB
    OS << "\n\n---- BB Info ----\n\n";
    BB_LIST &BBs = m_kernel->fg.getBBList();
    for (auto II = BBs.begin(), IE = BBs.end(); II != IE; ++II) {
      G4_BB *bb = *II;
      BBCostInfo &BInfo = m_BBCostInfo[bb];
      OS << "BB" << bb->getId()
         << ":  Prob = " << (float)BInfo.m_prob / MAX_PROB_POINTS << " (0x"
         << std::hex << BInfo.m_prob << "/0x" << MAX_PROB_POINTS << std::dec
         << ");  cycles = " << BInfo.m_cycles << "\n";
    }
  }

  // Dump summary of cost metrics for each subroutine and kernel
  const int numFuncs = (int)m_metrics.size();
  for (int i = 0; i < numFuncs; ++i) {
    const FuncCost &FC = m_metrics[i];
    const CostMetricsWrapper &FC_ec = FC.m_estimateCost;
    const char *name;
    const char *nameKind = (((i + 1) == numFuncs) ? "kernel " : "subroutine ");
    if ((i + 1) == numFuncs) {
      name = m_kernel->getName();
    } else {
      // subroutine, get its entry label name
      FuncInfo *pFI = FC.m_funcInfo;
      G4_BB *StartBB = pFI->getInitBB();
      name = StartBB->getLabel()->getLabelName();
    }
    OS << "\nEstimated Kernel Cost Metrics for reference only: "
       << nameKind << name << "\n"
       << "    Total Cycles = " << FC_ec.getCycles() << "\n"
       << "    Total Bytes Loaded = " << FC_ec.getLoadBytes() << "\n"
       << "    Total Bytes Stored = " << FC_ec.getStoreBytes() << "\n\n\n";

    // Cost expression (output to be written to zeinfo)
    const CostMetricsWrapper &FCM = FC.m_funcCost.C;
    OS << "\nKernel Cost Metrics : " << nameKind << name << "\n"
       << "    Cycles (excluding loops) = " << FCM.getCycles() << "\n"
       << "    Bytes Loaded (excluding loops) = " << FCM.getLoadBytes() << "\n"
       << "    Bytes Stored (excluding loops) = " << FCM.getStoreBytes()
       << "\n\n";

    for (const Loop *aL : FC.m_allLoopsInProgramOrder) {
      LoopCost &LC = m_loopCosts[aL];
      CostExprInternal &lce = LC.m_loopBodyCost;

      int level = aL->getNestingLevel();
      std::string indent(4 * level, ' ');

      OS << indent << "L[" << LC.m_loopId << "] "
         << "[Header:BB" << aL->getHeader()->getId() << ", Tail:BB"
         << aL->backEdgeSrc()->getId() << ", visaid: " << LC.m_backedge_visaId
         << "]\n";
      CostMetricsWrapper &LCM = lce.C;
      OS << indent << "  Loop body only, excluding nested loops\n"
         << indent << "    Cycles = " << LCM.getCycles() << "\n"
         << indent << "    Bytes Loaded = " << LCM.getLoadBytes() << "\n"
         << indent << "    Bytes Stored = " << LCM.getStoreBytes() << "\n";
      if (aL->getNumImmChildLoops() > 0)
        OS << "\n";
    }
  }
}

void KernelCost::printForLit(std::ostream &OS) {
  const FuncCost &FC = m_metrics.back();
  int i = 0;
  for (const Loop *aL : FC.m_allLoopsInProgramOrder) {
    LoopCost &LC = m_loopCosts[aL];
    OS << "Loop " << i << ": id " << LC.m_loopId
       << ", level " << aL->getNestingLevel() << "\n";
    ++i;
  }

  OS << "#subroutines = " << m_metrics.size() - 1 << "\n";
}

void KernelCost::dump() {
  print(std::cout);
}

void KernelCost::dump() const {
  const_cast<KernelCost*>(this)->print(std::cout);
}

namespace vISA {

void collectKernelCostInfo(G4_Kernel* pK, std::vector<VISA_BB_INFO> &BBInfo) {
  KernelCost KCA(pK, BBInfo);
  KCA.run();
}

}
