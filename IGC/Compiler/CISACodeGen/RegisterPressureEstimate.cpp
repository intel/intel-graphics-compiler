/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

// Estimates the register pressure at a program point.

#include "RegisterPressureEstimate.hpp"
#include "Compiler/IGCPassSupport.h"
#include <Compiler/CodeGenContextWrapper.hpp>
#include <Compiler/CodeGenPublic.h>
#include <set>
#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Function.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC::Debug;
using namespace IGC;

char RegisterPressureEstimate::ID = 0;
#define PASS_FLAG "igc-RegisterPressureEstimate"
#define PASS_DESCRIPTION "GenX Register Pressure Analysis"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RegisterPressureEstimate, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass);
IGC_INITIALIZE_PASS_END(RegisterPressureEstimate, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC {
void RegisterPressureEstimate::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<CodeGenContextWrapper>();
  AU.addRequired<LoopInfoWrapperPass>();
  if (m_requireWIA) {
    AU.addRequired<WIAnalysis>();
  }
}

bool RegisterPressureEstimate::runOnFunction(Function &F) {
  m_DL = &F.getParent()->getDataLayout();
  m_pFunc = &F;
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  IGC::CodeGenContext *pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  WI = getAnalysisIfAvailable<WIAnalysis>();

  if (pCtx->type == ShaderType::COMPUTE_SHADER || pCtx->type == ShaderType::OPENCL_SHADER || m_countTemps) {
    OVERALL_PRESSURE_UPBOUND = 4 * 1024;
  } else {
    OVERALL_PRESSURE_UPBOUND = 512;
  }

  m_available = buildLiveIntervals(true);

  if (m_available && m_countTemps && pCtx->m_tempCount == 0) {
    pCtx->m_tempCount = getMaxRegisterPressure() / 4;
  }
  return false;
}

/// \brief Assign a number to each instruction in a function.
///
/// - Arguments get number 0;
/// - Basic blocks get a number;
/// - Phi nodes in each basic block get the same number;
/// - All other instructions get a unique number, if assigned.
///
void RegisterPressureEstimate::assignNumbers() {
  unsigned Num = 0;

  // Arguments assigned with number 0.
  for (auto AI = m_pFunc->arg_begin(), AE = m_pFunc->arg_end(); AI != AE; ++AI) {
    Argument *Arg = &(*AI);
    m_pNumbers[Arg] = Num;
    if (!Arg->use_empty()) {
      getOrCreateLiveRange(Arg);
    }
  }

  // Assign a number to basic blocks and instructions.
  for (auto &BB : *m_pFunc) {
    unsigned BlockNum = m_pNumbers[&BB] = Num++;
    for (auto &II : BB) {
      Instruction *Inst = &II;
      if (isa<DbgInfoIntrinsic>(Inst)) {
        continue;
      }
      if (isa<PHINode>(Inst)) {
        m_pNumbers[Inst] = BlockNum;
      } else {
        m_pNumbers[Inst] = Num++;
      }
      if (!Inst->use_empty()) {
        getOrCreateLiveRange(Inst);
      }
    }
  }

  MaxAssignedNumber = Num;
}

/// Construct a string from a unsigned integer with the intended width.
static std::string alignedString(unsigned Val) {
  const unsigned Width = 3;
  std::string Str = Twine(Val).str();
  if (Str.size() < Width) {
    Str.insert(Str.begin(), Width - Str.size(), ' ');
  }
  return Str;
}

void RegisterPressureEstimate::printNumbering(raw_ostream &OS) {
  unsigned UnamedBBNum = 1;
  for (auto BI = m_pFunc->begin(), BE = m_pFunc->end(); BI != BE; ++BI) {
    if (m_pNumbers.count(&(*BI))) {
      OS << "[" << alignedString(m_pNumbers[&(*BI)]) << "] ";
    }
    if (BI->hasName()) {
      OS << BI->getName() << ":\n";
    } else {
      OS << ";<label>:" + Twine(UnamedBBNum++) << "\n";
    }

    for (auto II = BI->begin(), IE = BI->end(); II != IE; ++II) {
      Instruction *Inst = &(*II);
      if (m_pNumbers.count(Inst)) {
        OS << "[" << alignedString(m_pNumbers[Inst]) << "] ";
      }
      Inst->print(OS);
      OS << "\n";
    }
  }
}

void RegisterPressureEstimate::dumpNumbering() { printNumbering(ods()); }

unsigned RegisterPressureEstimate::getAssignedNumberForInst(Instruction *pInst) { return m_pNumbers[pInst]; }

unsigned RegisterPressureEstimate::getMaxAssignedNumberForBB(BasicBlock *pBB) { return m_pNumbers[&pBB->back()]; }

unsigned RegisterPressureEstimate::getMinAssignedNumberForBB(BasicBlock *pBB) { return m_pNumbers[&pBB->front()]; }

void RegisterPressureEstimate::LiveRange::setBegin(unsigned Begin) {
  for (auto &Seg : Segments) {
    if (Seg.Begin < Begin) {
      Seg.Begin = Begin;
    }
  }
}

void RegisterPressureEstimate::LiveRange::sortAndMerge() {
  std::sort(Segments.begin(), Segments.end());
  unsigned NewSize = 0;
  for (unsigned i = 0; i != Segments.size(); ++i) {
    if (NewSize && Segments[i].Begin <= Segments[NewSize - 1].End) {
      Segments[NewSize - 1].End = std::max(Segments[i].End, Segments[NewSize - 1].End);
    } else {
      Segments[NewSize++] = Segments[i];
    }
  }
  Segments.resize(NewSize);
}

void RegisterPressureEstimate::LiveRange::print(raw_ostream &OS) const {
  for (auto &Seg : Segments) {
    OS << "  [" << Seg.Begin << ", " << Seg.End << ")";
  }
}

void RegisterPressureEstimate::LiveRange::dump() const { print(ods()); }

void RegisterPressureEstimate::printLiveRanges(raw_ostream &OS) {
  OS << "\nLive ranges:";
  for (auto &Item : m_pLiveRanges) {
    OS << "\n";
    Item.first->printAsOperand(OS);
    OS << ":\n";
    Item.second->print(OS);
    OS << "\n";
  }
}

void RegisterPressureEstimate::dumpLiveRanges() { printLiveRanges(ods()); }

/// The algorithm is from "Linear Scan Register Allocation On SSA Form" by
/// Christian Wimmer and Michael Franz, CGO 2010.
///
/// For each block b in reverse order do
///    live = union of successor.livein for each successor of b
///
///    for each phi of successors of b do
///       live.add(phi.inputOf(b))
///
///    for each opnd in live do
///       intervals[opnd].addRange(b.from, b.to)
///
///    for each operation op of b in reverse order do
///       for each output operand opnd of op do
///           intervals[opnd].setFrom(op.id)
///           live.remove(opnd)
///       for each input operand opnd of op do
///           intervals[opnd].addRange(b.from, op.id)
///           live.add(opnd)
///
///    for each phi of b do
///       live.remove(phi.output)
///
///    if b is loop header then
///       loopEnd = last block of the loop starting at b
///       for each opnd in live do
///           intervals[opnd].addRange(b.from, loopEnd.to)
///
///    b.livein = live
///
/// Return true if live interval is calculated successfully; false otherwise.
bool RegisterPressureEstimate::buildLiveIntervals(bool RemoveLR) {
  // Clear existing data if any.
  m_pNumbers.clear();

  clear(RemoveLR);

  // Assign a number to arguments, basic blocks and instructions.
  // build the live-range pool.
  assignNumbers();

  unsigned OverallEstimate = 0;
  // quick estimate
  for (auto VI = m_pLiveRanges.begin(), VE = m_pLiveRanges.end(); VI != VE; ++VI) {
    auto V = VI->first;
    unsigned RangeStart = m_pNumbers[V];
    unsigned MaxRange = 0;
    // need to find the last use
    for (auto UI = V->user_begin(), UE = V->user_end(); UI != UE; ++UI) {
      Instruction *UseI = dyn_cast<Instruction>(*UI);
      if (!UseI || isInstructionTriviallyDead(UseI))
        continue;
      unsigned RangeEnd = RangeStart;
      if (PHINode *PN = dyn_cast<PHINode>(UseI)) {
        // PHI nodes use the operand in the predecessor block,
        // not the block with the PHI.
        Use &U = UI.getUse();
        unsigned num = PHINode::getIncomingValueNumForOperand(U.getOperandNo());
        auto UseBB = PN->getIncomingBlock(num);
        RangeEnd = m_pNumbers[&UseBB->back()] + 1;
      } else {
        RangeEnd = m_pNumbers[UseI];
      }
      if (RangeEnd > RangeStart && RangeEnd - RangeStart > MaxRange)
        MaxRange = RangeEnd - RangeStart;
      else if (RangeStart > RangeEnd && RangeStart - RangeEnd > MaxRange)
        MaxRange = RangeStart - RangeEnd;
    }
    OverallEstimate += MaxRange * getValueBytes(V);
  }
  OverallEstimate = iSTD::Round(OverallEstimate, SIMD_PRESSURE_MULTIPLIER) / SIMD_PRESSURE_MULTIPLIER;
  if (0 == MaxAssignedNumber) {
    return false;
  } else {
    OverallEstimate = OverallEstimate / getMaxAssignedNumberForFunction();
  }
  if (OverallEstimate > OVERALL_PRESSURE_UPBOUND)
    return false;

  DenseMap<BasicBlock *, std::set<Value *>> BlockLiveMap;
  // Top level loop to visit each block once in reverse order.
  for (auto BI = IGCLLVM::rbegin(m_pFunc), BE = IGCLLVM::rend(m_pFunc); BI != BE; ++BI) {
    BasicBlock *BB = &*BI;
    auto Result = BlockLiveMap.insert(std::make_pair(BB, std::set<Value *>()));
    IGC_ASSERT_MESSAGE(Result.second, "must not be processed yet");
    std::set<Value *> &Live = Result.first->second;

    // live = union of successor.livein for each successor of b
    //
    // for each phi of successors of b do
    //     live.add(phi.inputOf(b))
    //
    for (auto PI = succ_begin(BB), PE = succ_end(BB); PI != PE; ++PI) {
      BasicBlock *Succ = *PI;
      auto Iter = BlockLiveMap.find(Succ);
      if (Iter != BlockLiveMap.end()) {
        std::set<Value *> &SuccLive = Iter->second;
        Live.insert(SuccLive.begin(), SuccLive.end());
      }

      // For each phi node from successors, update liveness.
      for (auto II = Succ->begin(), IE = Succ->end(); II != IE; ++II) {
        Instruction *Inst = &*II;
        if (auto PN = dyn_cast<PHINode>(Inst)) {
          Live.insert(PN->getIncomingValueForBlock(BB));
        } else {
          // all phi's are in the first few instructions.
          break;
        }
      }
    }

    // The basic block number.
    unsigned BlockNum = m_pNumbers[BB];

    // for each opnd in live do
    //    intervals[opnd].addRange(b.from, b.to)
    for (auto I = Live.begin(), E = Live.end(); I != E; ++I) {
      unsigned End = m_pNumbers[&BB->back()] + 1;
      Value *V = *I;
      if (auto LR = getLiveRangeOrNull(V)) {
        LR->addSegment(BlockNum, End);
      }
    }
    // for each operation op of b in reverse order do
    //     for each output operand opnd of op do
    //         intervals[opnd].setFrom(op.id)
    //         live.remove(opnd)
    //     for each input operand opnd of op do
    //         intervals[opnd].addRange(b.from, op.id)
    //         live.add(opnd)
    for (auto II = BB->rbegin(), IE = BB->rend(); II != IE; ++II) {
      Instruction *Inst = &*II;

      // Skip debugging intrinsic calls.
      if (isa<DbgInfoIntrinsic>(Inst)) {
        continue;
      }

      // Skip phi nodes and its predecessors decide the live variables.
      if (isa<PHINode>(Inst)) {
        continue;
      }

      // The instruction number.
      unsigned InstNum = m_pNumbers[Inst];

      if (!Inst->use_empty()) {
        if (auto LR = getLiveRangeOrNull(Inst)) {
          LR->setBegin(InstNum);
        }
        Live.erase(Inst);
      }

      // Handle its input operands.
      for (auto OI = Inst->op_begin(), OE = Inst->op_end(); OI != OE; ++OI) {
        Value *Opnd = *OI;
        if (isa<Argument>(Opnd) || isa<Instruction>(Opnd)) {
          if (LiveRange *LR = getLiveRangeOrNull(Opnd)) {
            LR->addSegment(BlockNum, InstNum);
          }
          Live.insert(Opnd);
        }
      }
    }

    // for each phi of b do
    //     live.remove(phi.output)
    //
    for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II) {
      Instruction *Inst = &*II;
      if (isa<PHINode>(Inst)) {
        Live.erase(Inst);
      } else {
        // all phi's are in the first few instructions.
        break;
      }
    }

    // if b is loop header then
    //     loopEnd = last block of the loop starting at b
    //     for each opnd in live do
    //         intervals[opnd].addRange(b.from, loopEnd.to)
    //
    if (LI->isLoopHeader(BB)) {
      Loop *L = LI->getLoopFor(BB);
      if (L != nullptr) {
        if (BasicBlock *Latch = L->getLoopLatch()) {
          for (auto I = Live.begin(), E = Live.end(); I != E; ++I) {
            unsigned End = m_pNumbers[&Latch->back()] + 1;
            if (auto LR = getLiveRangeOrNull(*I)) {
              LR->addSegment(BlockNum, End);
            }
          }
        }
      } else {
        // Just set unavailable of live range info for now.
        clear(RemoveLR);
        return false;
        // IGC_ASSERT_EXIT_MESSAGE(0, "Support for unnatural loops, not implemented yet");
      }
    }
  }

  // Finally, combine multiple live ranges into a single one and sort them.
  mergeLiveRanges();
  return true;
}

unsigned RegisterPressureEstimate::getRegisterPressureForInstructionFromRPMap(unsigned number) const {
  auto Iter = m_pRegisterPressureByInstruction.find(number);
  if (Iter != m_pRegisterPressureByInstruction.end()) {
    return iSTD::Round(Iter->second, SIMD_PRESSURE_MULTIPLIER) / SIMD_PRESSURE_MULTIPLIER;
  } else {
    return 0;
  }
}

void RegisterPressureEstimate::buildRPMapPerInstruction() {
  unsigned maxNumberOfInstructions = getMaxAssignedNumberForFunction();
  for (unsigned number = 0; number < maxNumberOfInstructions; number++) {
    m_pRegisterPressureByInstruction[number] = 0;
  }

  // Segments are sorted.
  for (auto I = m_pLiveRanges.begin(), E = m_pLiveRanges.end(); I != E; ++I) {
    Value *V = I->first;
    unsigned int pressure = getValueBytes(V);
    for (auto &Seg : I->second->Segments) {
      for (unsigned number = Seg.Begin; number < Seg.End; number++) {
        m_pRegisterPressureByInstruction[number] += pressure;
      }
    }
  }
  return;
}

unsigned RegisterPressureEstimate::getRegisterWeightForInstruction(llvm::Instruction *Inst) const {
  return iSTD::Round(getValueBytes(Inst), SIMD_PRESSURE_MULTIPLIER) / SIMD_PRESSURE_MULTIPLIER;
}

unsigned RegisterPressureEstimate::getRegisterPressure(Instruction *Inst) const {
  auto Iter = m_pNumbers.find(Inst);
  if (Iter != m_pNumbers.end()) {
    // Find the instruction location.
    unsigned N = Iter->second;

    // Now sum all intervals that contain this location.
    unsigned Pressure = 0;

    // Segments are sorted.
    for (auto I = m_pLiveRanges.begin(), E = m_pLiveRanges.end(); I != E; ++I) {
      if (I->second->contains(N)) {
        Value *V = I->first;
        Pressure += getValueBytes(V);
      }
    }

    return iSTD::Round(Pressure, SIMD_PRESSURE_MULTIPLIER) / SIMD_PRESSURE_MULTIPLIER;
  }

  // ignore this instruction.
  return 0;
}

unsigned RegisterPressureEstimate::getMaxRegisterPressure() {
  unsigned maxNumberOfInstructions = getMaxAssignedNumberForFunction();
  buildRPMapPerInstruction();
  unsigned MaxPressure = 0;
  for (unsigned number = 0; number < maxNumberOfInstructions; number++) {
    MaxPressure = std::max(m_pRegisterPressureByInstruction[number], MaxPressure);
  }
  return iSTD::Round(MaxPressure, SIMD_PRESSURE_MULTIPLIER) / SIMD_PRESSURE_MULTIPLIER;
}

unsigned RegisterPressureEstimate::getMaxRegisterPressure(BasicBlock *BB) const {
  // FIXME: if the BB is huge, it can be more efficient to call
  // buildRPMapPerInstruction, then query the map
  unsigned RP = 0;
  for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II) {
    Instruction *Inst = &(*II);
    RP = std::max(RP, getRegisterPressure(Inst));
  }
  return RP;
}

void RegisterPressureEstimate::printRegisterPressureInfo(bool Detailed, const char *msg) {
  unsigned MaxRP = getMaxRegisterPressure();
  if (Detailed) {
    for (inst_iterator I = inst_begin(m_pFunc), E = inst_end(m_pFunc); I != E; ++I) {
      Instruction *Inst = &*I;
      auto Number = getAssignedNumberForInst(Inst);
      ods() << "[RP = " << getRegisterPressureForInstructionFromRPMap(Number) << "]";
      Inst->print(ods());
      ods() << "\n";
    }
  }

  ods() << "; " << msg << "\n";
  ods() << "; Kernel " << m_pFunc->getName() << "\n";
  ods() << "; Max RP = " << MaxRP << " bytes, (" << ((MaxRP + 31) / 32) << " GRFs)\n\n";
}
} // namespace IGC
