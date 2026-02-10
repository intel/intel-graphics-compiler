/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/RegisterEstimator.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/debug/Debug.hpp"
#include "common/igc_regkeys.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/SparseBitVector.h"
#include <llvm/IR/CFG.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/DerivedTypes.h>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

char RegisterEstimator::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-registerestimator"
#define PASS_DESCRIPTION "Use LivenessAnalysis to calculate register pressure"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(RegisterEstimator, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LivenessAnalysis)
IGC_INITIALIZE_PASS_END(RegisterEstimator, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

uint32_t RegisterEstimator::getNumLiveGRFAtInst(Instruction *I, uint16_t simdsize) {
  InstToRegUsageMap::iterator MI = m_LiveVirtRegs.find(I);
  if (MI != m_LiveVirtRegs.end()) {
    RegUse &ruse = MI->second.allUses[REGISTER_CLASS_GRF];
    return getNumRegs(ruse, simdsize);
  }

  BasicBlock *BB = I->getParent();
  RegUsage nCurrLiveIns = m_BBLiveInVirtRegs[BB];

  ValueToIntMap &ValueIds = m_LVA->ValueIds;
  ValueToValueVecMap &KillInfo = m_LVA->KillInsts;

  // Calculate the number of lives for each instruction of this BB
  for (BasicBlock::iterator II = BB->begin(), IE = BB->end(); II != IE; ++II) {
    Instruction *Inst = &*II;
    if (Inst == I) {
      RegUse &ruse = nCurrLiveIns.allUses[REGISTER_CLASS_GRF];
      return getNumRegs(ruse, simdsize);
    }

    // Kills
    ValueToValueVecMap::iterator II0 = KillInfo.find(Inst);
    if (II0 != KillInfo.end()) {
      ValueVec &VS = II0->second;
      for (int i = 0, e = (int)VS.size(); i < e; ++i) {
        Value *killVal = VS[i];
        if (const RegUse *pregs = getRegUse(killVal)) {
          RegUse &ruse_curr = nCurrLiveIns.allUses[pregs->rClass];
          ruse_curr -= (*pregs);
        }
      }
    }

    // Defs
    ValueToIntMap::iterator II1 = ValueIds.find(Inst);
    if (II1 != ValueIds.end()) {
      RegUse regs = estimateNumOfRegs(Inst);
      RegUse &ruse_curr = nCurrLiveIns.allUses[regs.rClass];
      ruse_curr += regs;
    }
  }
  IGC_ASSERT_MESSAGE(0, "Instruction is not in its BB, something wrong!");
  return 0;
}

// Add register usage of all values in BV into RUsage:
// RUsage:  both in and out.
void RegisterEstimator::addRegUsage(RegUsage &RUsage, SBitVector &BV) {
  for (SBitVector::iterator I = BV.begin(), E = BV.end(); I != E; ++I) {
    int id = *I;
    if (const RegUse *pregs = getRegUse(id)) {
      RUsage.allUses[pregs->rClass] += (*pregs);
    }
  }
}

RegUse RegisterEstimator::estimateNumOfRegs(Value *V) const {
  // Assume no sharing GRF among Values.
  RegUse regs;
  if (getValueRegClass(V) == REGISTER_CLASS_FLAG) {
    // Flag register
    regs.rClass = (RegClass)REGISTER_CLASS_FLAG;
    regs.nregs_simd16 = 1;
    return regs;
  }

  // GRF
  Type *Ty = V->getType();
  if (!Ty->isVoidTy()) {
    IGCLLVM::FixedVectorType *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
    Type *eltTy = VTy ? VTy->getElementType() : Ty;
    uint32_t nelts = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;
    uint32_t eltBits = (uint32_t)m_DL->getTypeSizeInBits(eltTy);
    uint32_t nBytes = nelts * ((eltBits + 7) / 8);

    regs.rClass = (RegClass)REGISTER_CLASS_GRF;
    if (m_LVA->isUniform(V)) {
      // round up to DW
      uint16_t sz = (nBytes + DWORD_SIZE_IN_BYTE - 1) / DWORD_SIZE_IN_BYTE;
      regs.uniformInBytes += sz;
    } else {
      uint16_t sz16 = (nBytes * 16 + GRF_SIZE_IN_BYTE - 1) / GRF_SIZE_IN_BYTE; // SIMD16
      regs.nregs_simd16 = sz16;
    }
  }
  return regs;
}

void RegisterEstimator::calculate(bool doRPEPerInst) {
  if (m_BBMaxLiveVirtRegs.size() > 0) {
    // Already computed, just return.
    return;
  }
  if (!doRPEPerInst && IGC_GET_FLAG_VALUE(ForceRPE) == FLAG_LEVEL_2) {
    doRPEPerInst = true;
  }

  m_LVA->calculate(m_F);

  // Pre-allocate maps
  // We will allocate a bit more (~10%) in case it needs expansion.
  // Also, as llvm::DenseMap will resize if the Map's capacity is 75% full,
  // allocate even more to avoid such automatic resizing.
  if (doRPEPerInst) {
    // Use IdValues's size as the number of values to be considered.
    size_t nVals = m_LVA->IdValues.size();

    m_LiveVirtRegs.clear();
    uint32_t mapCap1 = int_cast<uint32_t>((size_t)(nVals * 1.40f));
    m_LiveVirtRegs.reserve(mapCap1);
  }

  uint32_t mapCap2 = int_cast<uint32_t>((size_t)(m_F->size() * 1.40f));
  m_BBMaxLiveVirtRegs.clear();
  m_BBLiveInVirtRegs.clear();
  m_BBMaxLiveVirtRegs.reserve(mapCap2);
  m_BBLiveInVirtRegs.reserve(mapCap2);

  BBLiveInMap &BBLiveIns = m_LVA->BBLiveIns;
  ValueToIntMap &ValueIds = m_LVA->ValueIds;
  ValueToValueVecMap &KillInfo = m_LVA->KillInsts;
  for (Function::iterator BI = m_F->begin(), BE = m_F->end(); BI != BE; ++BI) {
    BasicBlock *BB = &*BI;
    SBitVector &BitVec = BBLiveIns[BB];
    RegUsage nCurrLiveIns;

    // Calculate the number of live-ins at entry to BB
    for (SBitVector::iterator I = BitVec.begin(), E = BitVec.end(); I != E; ++I) {
      int id = *I;
      if (const RegUse *pregs = getRegUse(id)) {
        nCurrLiveIns.allUses[pregs->rClass] += (*pregs);
      }
    }

    m_BBLiveInVirtRegs.insert(std::make_pair(BB, nCurrLiveIns));

    RegUsage bbMaxRegs = nCurrLiveIns;

    // Calculate the number of lives for each instruction of this BB.
    // For simplicity, the number of lives are the one that is at exit
    // of the instruction, not at entry (nor the largest of the both).
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
      Instruction *Inst = &*I;
      ValueToIntMap::iterator IDef = ValueIds.find(Inst);
      if (IDef != ValueIds.end()) {
        if (const RegUse *pregs = getRegUse(Inst)) {
          RegUse &ruse_curr = nCurrLiveIns.allUses[pregs->rClass];
          ruse_curr += (*pregs);
        }
      }

      // Kills
      ValueToValueVecMap::iterator IKill = KillInfo.find(Inst);
      if (IKill != KillInfo.end()) {
        ValueVec &VS = IKill->second;
        for (int i = 0, e = (int)VS.size(); i < e; ++i) {
          Value *killVal = VS[i];
          if (const RegUse *pregs = getRegUse(killVal)) {
            RegUse &ruse_curr = nCurrLiveIns.allUses[pregs->rClass];
            ruse_curr -= (*pregs);
          }
        }
      }

      for (int i = 0; i < REGISTER_CLASS_TOTAL; ++i) {
        RegClass RC = (RegClass)i;
        RegUse &ruse1 = bbMaxRegs.allUses[RC];
        RegUse &ruse2 = nCurrLiveIns.allUses[RC];

        if (ruse1 < ruse2) {
          ruse1 = ruse2;
        }
      }
      if (doRPEPerInst) {
        m_LiveVirtRegs.insert(std::make_pair(Inst, nCurrLiveIns));
      }
    }

    // save it in map
    m_BBMaxLiveVirtRegs.insert(std::make_pair(BB, bbMaxRegs));

    for (int i = 0; i < REGISTER_CLASS_TOTAL; ++i) {
      RegUse &ruse_max = m_MaxRegs.allUses[(RegClass)i];
      RegUse &ruse_curr = bbMaxRegs.allUses[(RegClass)i];
      if (ruse_max < ruse_curr) {
        ruse_max = ruse_curr;
      }
    }
  }

#if 0
    // Sort LiveVirtRegs in decreasing order. As Map cannot be sorted, a list
    // is used for sorting it.
    for (ValueToIntMap::iterator I = m_LiveVirtRegs.begin(),
        E = m_LiveVirtRegs.end();
        I != E; ++I)
    {
        Value* V = I->first;
        m_AllValues.push_back(V);
    }
    std::sort(m_AllValues.begin(), m_AllValues.end(), isNRegGreater(m_LiveVirtRegs));
#endif
  int dumpLevel = IGC_GET_FLAG_VALUE(RPEDumpLevel);
  if (dumpLevel > FLAG_LEVEL_0) {
    print(errs(), dumpLevel);
  }
}

int RegisterEstimator::getNUsesInBB(Value *V, BasicBlock *BB) {
  int nUses = 0;
  for (auto UI = V->user_begin(), UE = V->user_end(); UI != UE; ++UI) {
    Instruction *I = cast<Instruction>(*UI);
    if (I->getParent() == BB) {
      ++nUses;
    }
  }
  return nUses;
}

bool RegisterEstimator::runOnFunction(Function &F) {
  m_F = &F;
  m_DL = &F.getParent()->getDataLayout();
  m_LVA = &getAnalysis<LivenessAnalysis>();

  m_WIA = getAnalysisIfAvailable<WIAnalysis>();

  uint32_t nVals = (uint32_t)m_LVA->IdValues.size();
  uint32_t Caps = (uint32_t)m_LVA->IdValues.capacity();
  m_ValueRegUses.reserve(Caps);
  m_ValueRegUses.resize(nVals);

  // 1. Pre-compute register needed for each values.
  // 2. The max possible registers needed, assume all values are live,
  RegUsage estNumRegs; // RPE assuming all values are live
  ValueToIntMap::iterator VI, VE;
  for (VI = m_LVA->ValueIds.begin(), VE = m_LVA->ValueIds.end(); VI != VE; ++VI) {
    Value *Val = VI->first;
    uint32_t valId = VI->second;
    RegUse regs = estimateNumOfRegs(Val);
    RegUse &ruse = estNumRegs.allUses[regs.rClass];
    ruse += regs;
    IGC_ASSERT_MESSAGE(valId < nVals, "ValueIds does not match IdValues!");
    m_ValueRegUses[valId] = regs;
  }

  m_noGRFPressure = isGRFPressureLow(16, estNumRegs);

  // Note that runOnFunction does not do RPE calculation unless ForceRPE
  // is enabled (for debugging).  The RPE is calcualted for users to call
  // calculate() explicitly.
  if (IGC_IS_FLAG_ENABLED(ForceRPE)) {
    // Calculate register pressure for each BB
    calculate();
  }
  return false;
}

void RegisterEstimator::print(raw_ostream &OS, BasicBlock *BB, int dumpLevel) {
  uint32_t grf_livein_simd8 = getNumLiveInGRFAtBB(BB, 8);
  uint32_t grf_livein_simd16 = getNumLiveInGRFAtBB(BB, 16);
  uint32_t grf_livein_simd32 = getNumLiveInGRFAtBB(BB, 32);
  uint32_t grf_maxlive_simd8 = getMaxLiveGRFAtBB(BB, 8);
  uint32_t grf_maxlive_simd16 = getMaxLiveGRFAtBB(BB, 16);
  uint32_t grf_maxlive_simd32 = getMaxLiveGRFAtBB(BB, 32);

  if (BB->hasName())
    OS << "  " << BB->getName() << "  ";
  OS << "\n  ";
  OS << "livein simd8|16|32=<" << grf_livein_simd8 << ", " << grf_livein_simd16 << ", " << grf_livein_simd32 << ">  ";
  OS << "maxLive simd8|16|32=<" << grf_maxlive_simd8 << ", " << grf_maxlive_simd16 << ", " << grf_maxlive_simd32
     << ">\n";
  if (dumpLevel < 2) {
    return;
  }

  // dumpLevel > 1
  IntToValueVector &IdValues = m_LVA->IdValues;
  // ValueToValueSetMap& KillInfo = m_LVA.KillInsts;
  BBLiveInMap &BBLiveIns = m_LVA->BBLiveIns;

  SBitVector &BitVec = BBLiveIns[BB];
  int nVals = 0;
  for (SBitVector::iterator I = BitVec.begin(), E = BitVec.end(); I != E; ++I) {
    int id = *I;
    Value *V = IdValues[id];
    IGC_ASSERT_MESSAGE(nullptr != V, "Value should be in Value Map!");

    const RegUse *pregs = getRegUse(id);
    if (pregs == nullptr) {
      IGC_ASSERT_MESSAGE(0, "Missing RegUse!");
      continue;
    }

    if (nVals == 0) {
      OS << "    ";
    }
    if (V->hasName()) {
      OS << V->getName() << " ";
    } else {
      OS << LivenessAnalysis::getllvmValueName(V) << " ";
    }
    OS << "{" << getNumRegs(*pregs, 8) << ", " << getNumRegs(*pregs, 16) << ", " << getNumRegs(*pregs, 32) << "}, ";
    ++nVals;
    if (nVals == 6) {
      // 6 values per line
      OS << "\n";
      nVals = 0;
    }
  }
  OS << (nVals != 0 ? "\n\n" : "\n");

  for (BasicBlock::iterator it = BB->begin(), ie = BB->end(); it != ie; ++it) {
    Instruction *I = &*it;
    if (m_LiveVirtRegs.count(I) == 0) {
      OS << "  { not computed } ";
    } else {
      RegUsage &ruse = m_LiveVirtRegs[I];
      RegUse &grfuse = ruse.allUses[REGISTER_CLASS_GRF];
      OS << "  {" << getNumRegs(grfuse, 8) << ", " << getNumRegs(grfuse, 16) << ", " << getNumRegs(grfuse, 32) << "} ";
    }
    OS << *I;
    if (!m_pBB2ID.empty()) {
      if (BranchInst *BrI = dyn_cast<BranchInst>(I)) {
        OS << "  (";
        for (int i = 0, e = (int)BrI->getNumSuccessors(); i < e; ++i) {
          BasicBlock *b = BrI->getSuccessor(i);
          OS << " BB[" << m_pBB2ID[b] << "]";
        }
        OS << " )";
      }
    }
    OS << "\n";
  }
}

void RegisterEstimator::print(raw_ostream &OS, int dumpLevel) {
  if (!m_F)
    return;

  std::stringstream ss;
  ss << "RegisterEstimator: " << m_F->getName().str();
  Debug::Banner(OS, ss.str());

  int bid = 0;
  m_pBB2ID.clear();
  for (Function::iterator I = m_F->begin(), E = m_F->end(); I != E; ++I) {
    BasicBlock *BB = &*I;
    m_pBB2ID[BB] = bid;
    ++bid;
  }
  for (Function::iterator I = m_F->begin(), E = m_F->end(); I != E; ++I) {
    BasicBlock *BB = &*I;
    if (dumpLevel > FLAG_LEVEL_1) {
      OS << "\n";
    }
    OS << "BB[" << m_pBB2ID[BB] << "]:";
    print(OS, BB, dumpLevel);
    ++bid;
  }
  OS << "\n\n";
  m_pBB2ID.clear();
}

RegPressureTracker::RegPressureTracker(RegisterEstimator *RPE) : m_BB(nullptr), m_pRPE(RPE) {
  m_TrackRegPressure = !m_pRPE->hasNoGRFPressure();
  if (!m_TrackRegPressure) {
    return;
  }

  m_pRPE->calculate();

#if 0
    // TODO: need per-BB possible Max estimate
    if (m_pRPE->isGRFPressureLow(32))
    {
        m_TrackRegPressure = false;
        return;
    }
#endif

  // Pre-allocate DenseMap
  size_t nVals = m_pRPE->getNumValues();
  uint32_t mapCap = int_cast<uint32_t>((size_t)(nVals * 1.40f));
  m_DeadValueNumUses.grow(mapCap);
}

void RegPressureTracker::init(BasicBlock *BB, bool doMaxRegInBB) {
  m_BB = BB;
  if (!m_TrackRegPressure) {
    return;
  }

  m_LiveOutSet.clear();
  m_DeadValueNumUses.clear();

  LivenessAnalysis *LVA = m_pRPE->getLivenessAnalysis();

  BBLiveInMap &BBLiveIns = LVA->BBLiveIns;
  SBitVector &BitVec = BBLiveIns[BB];
  for (succ_iterator SI = succ_begin(BB), E = succ_end(BB); SI != E; ++SI) {
    BasicBlock *SuccBB = *SI;
    SBitVector &succBitVec = BBLiveIns[SuccBB];
    m_LiveOutSet |= succBitVec;
  }

  for (SBitVector::iterator I = BitVec.begin(), E = BitVec.end(); I != E; ++I) {
    int id = *I;
    if (m_LiveOutSet.test(id)) {
      continue;
    }
    // This Value has the last use in this BB, add it into the map.
    Value *V = LVA->IdValues[id];
    int nUses = m_pRPE->getNUsesInBB(V, BB);
    m_DeadValueNumUses[V] = nUses;
  }

  for (BasicBlock::iterator BI = m_BB->begin(), BE = m_BB->end(); BI != BE; ++BI) {
    Instruction *Inst = &*BI;
    Value *V = Inst;
    ValueToIntMap::iterator VI = LVA->ValueIds.find(V);
    if (VI != LVA->ValueIds.end()) {
      int id = VI->second;
      if (m_LiveOutSet.test(id)) {
        continue;
      }
      int nUses = m_pRPE->getNUsesInBB(V, BB);
      m_DeadValueNumUses[V] = nUses;
    }
  }

  m_pRPE->getMaxLiveinRegsAtBB(m_RUsage, BB);
}

void RegPressureTracker::advance(llvm::Instruction *I) {
  if (!m_TrackRegPressure) {
    return;
  }

  Value *V = I;
  LivenessAnalysis *LVA = m_pRPE->getLivenessAnalysis();
  auto VI = LVA->ValueIds.find(V);
  if (VI != LVA->ValueIds.end()) {
    int id = VI->second;
    if (const RegUse *pRegs = m_pRPE->getRegUse(id)) {
      m_RUsage.allUses[pRegs->rClass] += *pRegs;
    }
  }

  // Update current register pressure
  for (auto OI = I->op_begin(), OE = I->op_end(); OI != OE; ++OI) {
    Value *Opr = *OI;
    if (isa<Constant>(Opr)) {
      continue;
    }
    auto DVI = m_DeadValueNumUses.find(Opr);
    if (DVI != m_DeadValueNumUses.end()) {
      int &numUses = DVI->second;
      --numUses;
      if (numUses == 0) {
        if (const RegUse *pRegs = m_pRPE->getRegUse(Opr)) {
          m_RUsage.allUses[pRegs->rClass] -= *pRegs;
        }
      }
    }
  }
}

#if defined(_DEBUG)

void RegisterEstimator::dump() { dump(1); }

void RegisterEstimator::dump(int dumpLevel) { print(dbgs(), dumpLevel); }

void RegisterEstimator::dump(BasicBlock *BB) { dump(BB, 1); }

void RegisterEstimator::dump(BasicBlock *BB, int dumpLevel) { print(dbgs(), BB, dumpLevel); }

#endif
