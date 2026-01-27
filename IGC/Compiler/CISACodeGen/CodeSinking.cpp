/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

#include <fstream>
#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
#include "common/Stats.hpp"
#include "common/LLVMUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Verifier.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvmWrapper/IR/Value.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvmWrapper/Analysis/TargetLibraryInfo.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/CodeSinking.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC::Debug;

namespace IGC {

/// ================================= ///
/// Common functions for code sinking ///
/// ================================= ///

// Move referenced DbgValueInst intrinsics calls after defining instructions
// it is required for correct work of LiveVariables analysis and other
static void ProcessDbgValueInst(BasicBlock &blk, DominatorTree *DT) {
  llvm::DenseMap<Instruction *, Instruction *> PositionMap;
  for (auto I = blk.rbegin(), E = blk.rend(); I != E; ++I) {
    Instruction *inst = cast<Instruction>(&*I);
    if (auto *DVI = dyn_cast<DbgValueInst>(inst)) {
      // As debug intrinsics are not specified as users of an llvm instructions,
      // it may happen during transformation/optimization the first argument is
      // malformed (actually is dead). Not to chase each possible optimzation
      // let's do a general check here.
      if (DVI->getValue() != nullptr) {
        if (auto *def = dyn_cast<Instruction>(DVI->getValue())) {
          if (!DT->dominates(def, inst)) {
            if (isa<PHINode>(def)) {
              // If the instruction is a PHI node, insert the new instruction at the beginning of the block.
              PositionMap[inst] = &*def->getParent()->getFirstInsertionPt();
            } else {
              // Otherwise, insert the new instruction after the defining instruction.
              PositionMap[inst] = def->getNextNonDebugInstruction();
              IGC_ASSERT(!isa<BranchInst>(def));
            }
          }
        }
      } else {
        // The intrinsic is actually unneeded and will be removed later. Thus the type of the
        // first argument is not important now.
        Value *undef = UndefValue::get(llvm::Type::getInt32Ty(inst->getContext()));
        MetadataAsValue *MAV = MetadataAsValue::get(inst->getContext(), ValueAsMetadata::get(undef));
        cast<CallInst>(inst)->setArgOperand(0, MAV);
      }
    }
  }
  for (auto &[I, Pos] : PositionMap) {
    I->moveBefore(Pos);
  }
}

// Check the instruction is a 2d block read
static bool is2dBlockRead(Instruction *I) {
  if (GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(I)) {
    switch (Intr->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_LSC2DBlockRead:
    case GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload:
      return true;
    default:
      break;
    }
  }
  return false;
}

// Check if the instruction is a load or an allowed intrinsic that reads memory
static bool isAllowedLoad(Instruction *I) {
  if (isa<LoadInst>(I))
    return true;

  if (is2dBlockRead(I))
    return true;

  return false;
}

// Find the BasicBlock to sink
// return nullptr if instruction cannot be moved to another block
static BasicBlock *findLowestSinkTarget(Instruction *inst, SmallPtrSetImpl<Instruction *> &usesInBlk, bool &outerLoop,
                                        bool doLoopSink, llvm::DominatorTree *DT, llvm::LoopInfo *LI) {
  usesInBlk.clear();
  BasicBlock *tgtBlk = nullptr;
  outerLoop = false;
  for (Value::user_iterator I = inst->user_begin(), E = inst->user_end(); I != E; ++I) {
    // Determine the block of the use.
    Instruction *useInst = cast<Instruction>(*I);
    BasicBlock *useBlock = useInst->getParent();
    if (PHINode *PN = dyn_cast<PHINode>(useInst)) {
      // PHI nodes use the operand in the predecessor block,
      // not the block with the PHI.
      Use &U = I.getUse();
      unsigned num = PHINode::getIncomingValueNumForOperand(U.getOperandNo());
      useBlock = PN->getIncomingBlock(num);
    } else {
      if (useBlock == inst->getParent()) {
        return nullptr;
      }
    }
    if (tgtBlk == nullptr) {
      tgtBlk = useBlock;
    } else {
      tgtBlk = DT->findNearestCommonDominator(tgtBlk, useBlock);
      if (tgtBlk == nullptr)
        break;
    }
  }

  BasicBlock *curBlk = inst->getParent();
  Loop *curLoop = LI->getLoopFor(inst->getParent());
  while (tgtBlk && tgtBlk != curBlk) {
    Loop *tgtLoop = LI->getLoopFor(tgtBlk);
    EOPCODE intrinsic_name = GetOpCode(inst);
    // sink the pln instructions in the loop to reduce pressure
    // Sink instruction outside of loop into the loop if doLoopSink is true.
    if (intrinsic_name == llvm_input || (!tgtLoop || tgtLoop->contains(curLoop)) ||
        (doLoopSink && tgtLoop && (!curLoop || curLoop->contains(tgtLoop)))) {
      for (Value::user_iterator I = inst->user_begin(), E = inst->user_end(); I != E; ++I) {
        // Determine the block of the use.
        Instruction *useInst = cast<Instruction>(*I);
        BasicBlock *useBlock = useInst->getParent();
        if (useBlock == tgtBlk && !isa<PHINode>(useInst)) {
          usesInBlk.insert(useInst);
        }
      }
      outerLoop = (tgtLoop != curLoop);
      return tgtBlk;
    } else {
      tgtBlk = DT->getNode(tgtBlk)->getIDom()->getBlock();
    }
  }
  return nullptr;
}

static bool isCastInstrReducingPressure(Instruction *Inst, bool FlagPressureAware) {
  if (auto CI = dyn_cast<CastInst>(Inst)) {
    unsigned SrcSize = (unsigned int)CI->getSrcTy()->getPrimitiveSizeInBits();
    unsigned DstSize = (unsigned int)CI->getDestTy()->getPrimitiveSizeInBits();
    if (SrcSize == 0 || DstSize == 0) {
      // Non-primitive types.
      return false;
    }
    if (FlagPressureAware) {
      if (SrcSize == 1) {
        // i1 -> i32, reduces GRF pressure but increases flag pressure.
        // Do not consider it as reduce.
        return false;
      } else if (DstSize == 1) {
        // i32 -> i1, reduces flag pressure but increases grf pressure.
        // Consider it as reduce.
        return true;
      }
      if (SrcSize < DstSize) {
        // sext i32 to i64.
        return true;
      }
    } else {
      return SrcSize < DstSize;
    }
  }

  return false;
}

// Number of instructions in the function
static unsigned numInsts(const Function &F) {
  return std::count_if(llvm::inst_begin(F), llvm::inst_end(F), [](const auto &I) { return !isDbgIntrinsic(&I); });
}

static bool isDPAS(Value *V) {
  GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(V);
  if (!Intr)
    return false;
  switch (Intr->getIntrinsicID()) {
  case GenISAIntrinsic::GenISA_dpas:
  case GenISAIntrinsic::GenISA_sub_group_dpas:
  case GenISAIntrinsic::GenISA_sub_group_bdpas:
    return true;
  default:
    break;
  }
  return false;
};

/// ===================== ///
/// Non-loop code sinking ///
/// ===================== ///

// Register pass to igc-opt
#define PASS_FLAG "igc-code-sinking"
#define PASS_DESCRIPTION "code sinking"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CodeSinking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(CodeSinking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char CodeSinking::ID = 0;
CodeSinking::CodeSinking() : FunctionPass(ID) { initializeCodeSinkingPass(*PassRegistry::getPassRegistry()); }

// Sink the code down the tree, but not in the loop
bool CodeSinking::treeSink(Function &F) {
  bool IterChanged, EverChanged = false;
  totalGradientMoved = 0;
  // even if we limit code-sinking to ps-input instructions, we still need to iterate through
  // all the blocks because llvm-InstCombine may have sinked some ps-input instructions out of entry-block
  do {
    IterChanged = false;
    // Process all basic blocks in dominator-tree post-order
    for (po_iterator<DomTreeNode *> domIter = po_begin(DT->getRootNode()), domEnd = po_end(DT->getRootNode());
         domIter != domEnd; ++domIter) {
      IterChanged |= processBlock(*(domIter->getBlock()));
    }
  } while (IterChanged);

  EverChanged = IterChanged;
  for (auto BI = LocalBlkSet.begin(), BE = LocalBlkSet.end(); BI != BE; BI++) {
    IterChanged = localSink(*BI);
    EverChanged |= IterChanged;
  }
  LocalBlkSet.clear();
  LocalInstSet.clear();
  CTX->m_numGradientSinked = totalGradientMoved;

  return EverChanged;
}

bool CodeSinking::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  CTX = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  // only limited code-sinking to several shader-type
  // vs input has the URB-reuse issue to be resolved.
  // Also need to understand the performance benefit better.
  if (CTX->type != ShaderType::PIXEL_SHADER && CTX->type != ShaderType::DOMAIN_SHADER &&
      CTX->type != ShaderType::OPENCL_SHADER && CTX->type != ShaderType::RAYTRACING_SHADER &&
      CTX->type != ShaderType::COMPUTE_SHADER) {
    return false;
  }

  if (IGC_IS_FLAG_ENABLED(DisableCodeSinking) || numInsts(F) < IGC_GET_FLAG_VALUE(CodeSinkingMinSize)) {
    return false;
  }

  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  DL = &F.getParent()->getDataLayout();

  bool Changed = treeSink(F);

  if (Changed) {
    // the verifier currently rejects allocas with non-default
    // address space (which is legal). Raytracing does this, so we skip
    // verification here.
    if (CTX->type != ShaderType::RAYTRACING_SHADER) {
      IGC_ASSERT(false == verifyFunction(F, &dbgs()));
    }
  }

  return Changed;
}

bool CodeSinking::processBlock(BasicBlock &blk) {
  if (blk.empty())
    return false;

  uint32_t registerPressureThreshold = CTX->getNumGRFPerThread();

  uint pressure0 = 0;
  if (registerPressureThreshold) {
    // estimate live-out register pressure for this blk
    pressure0 = estimateLiveOutPressure(&blk, DL);
  }

  bool madeChange = false;
  numGradientMovedOutBB = 0;

  // Walk the basic block bottom-up.  Remember if we saw a store.
  BasicBlock::iterator I = blk.end();
  --I;
  bool processedBegin = false;
  bool metDbgValueIntrinsic = false;
  SmallPtrSet<Instruction *, 16> stores;
  UndoLocas.clear();
  MovedInsts.clear();
  Instruction *prevLoca = 0x0;
  do {
    Instruction *inst = &(*I); // The instruction to sink.

    // Predecrement I (if it's not begin) so that it isn't invalidated by sinking.
    processedBegin = (I == blk.begin());
    if (!processedBegin)
      --I;

    if (inst->mayWriteToMemory()) {
      stores.insert(inst);
      prevLoca = inst;
    }
    // intrinsic like discard has no explict use, gets skipped here
    else if (isa<DbgInfoIntrinsic>(inst) || inst->isTerminator() || isa<PHINode>(inst) || inst->use_empty()) {
      if (isa<DbgValueInst>(inst)) {
        metDbgValueIntrinsic = true;
      }
      prevLoca = inst;
    } else {
      Instruction *undoLoca = prevLoca;
      prevLoca = inst;

      if (sinkInstruction(inst, stores)) {
        if (ComputesGradient(inst))
          numGradientMovedOutBB++;
        madeChange = true;
        MovedInsts.push_back(inst);
        UndoLocas.push_back(undoLoca);
      }
    }
    // If we just processed the first instruction in the block, we're done.
  } while (!processedBegin);

  if (registerPressureThreshold) {
    if (madeChange) {
      // measure the live-out register pressure again
      uint pressure1 = estimateLiveOutPressure(&blk, DL);
      if (pressure1 > pressure0 + registerPressureThreshold) {
        rollbackSinking(&blk);
        madeChange = false;
      } else {
        totalGradientMoved += numGradientMovedOutBB;
      }
    }
  }
  if ((madeChange || metDbgValueIntrinsic) && CTX->m_instrTypes.hasDebugInfo) {
    ProcessDbgValueInst(blk, DT);
  }

  return madeChange;
}

bool CodeSinking::sinkInstruction(Instruction *InstToSink, SmallPtrSetImpl<Instruction *> &Stores) {
  // Check if it's safe to move the instruction.
  bool HasAliasConcern = false;
  bool ReducePressure = false;

  if (!isSafeToMove(InstToSink, ReducePressure, HasAliasConcern, Stores))
    return false;

  // SuccToSinkTo - This is the successor to sink this instruction to, once we
  // decide.
  BasicBlock *SuccToSinkTo = nullptr;
  SmallPtrSet<Instruction *, 16> UsesInBB;

  if (!HasAliasConcern) {
    // find the lowest common dominator of all uses
    bool IsOuterLoop = false;
    if (BasicBlock *TgtBB = findLowestSinkTarget(InstToSink, UsesInBB, IsOuterLoop, false, DT, LI)) {
      // heuristic, avoid code-motion that does not reduce execution frequency
      // but may increase register usage
      if (ReducePressure || (TgtBB && (IsOuterLoop || !PDT->dominates(TgtBB, InstToSink->getParent())))) {
        SuccToSinkTo = TgtBB;
      }
    } else {
      // local code motion for cases like cmp and pln
      if (ReducePressure) {
        LocalBlkSet.insert(InstToSink->getParent());
        LocalInstSet.insert(InstToSink);
      }
      return false;
    }
  } else {
    // when aliasing is a concern, only look at all the immed successors and
    // decide which one we should sink to, if any.
    BasicBlock *CurBB = InstToSink->getParent();
    for (succ_iterator I = succ_begin(InstToSink->getParent()), E = succ_end(InstToSink->getParent());
         I != E && SuccToSinkTo == 0; ++I) {
      // avoid sinking an instruction into its own block.  This can
      // happen with loops.
      if ((*I) == CurBB)
        continue;
      // punt on it because of alias concern
      if ((*I)->getUniquePredecessor() != CurBB)
        continue;
      // Don't move instruction across a loop.
      Loop *succLoop = LI->getLoopFor((*I));
      Loop *currLoop = LI->getLoopFor(CurBB);
      if (succLoop != currLoop)
        continue;
      if (allUsesDominatedByBlock(InstToSink, (*I), UsesInBB))
        SuccToSinkTo = *I;
    }
  }

  // If we couldn't find a block to sink to, ignore this instruction.
  if (!SuccToSinkTo) {
    return false;
  }

  if (!ReducePressure || HasAliasConcern) {
    InstToSink->moveBefore(&(*SuccToSinkTo->getFirstInsertionPt()));
  }
  // when alasing is not an issue and reg-pressure is not an issue
  // move it as close to the uses as possible
  else if (UsesInBB.empty()) {
    InstToSink->moveBefore(SuccToSinkTo->getTerminator());
  } else if (UsesInBB.size() == 1) {
    InstToSink->moveBefore(*(UsesInBB.begin()));
  } else {
    // first move to the beginning of the target block
    InstToSink->moveBefore(&(*SuccToSinkTo->getFirstInsertionPt()));
    // later on, move it close to the use
    LocalBlkSet.insert(SuccToSinkTo);
    LocalInstSet.insert(InstToSink);
  }
  return true;
}

// Sink to the use within basic block
bool CodeSinking::localSink(BasicBlock *BB) {
  bool Changed = false;
  for (auto &I : *BB) {
    Instruction *Use = &I;

    // "Use" can be a phi-node for a single-block loop,
    // which is not really a local-code-motion
    if (isa<PHINode>(Use))
      continue;

    for (unsigned i = 0; i < Use->getNumOperands(); ++i) {
      Instruction *Def = dyn_cast<Instruction>(Use->getOperand(i));
      if (!Def)
        continue;

      if (Def->getParent() == BB && LocalInstSet.count(Def)) {
        if (Def->getNextNode() != Use) {
          Instruction *InsertPoint = Use;
          Def->moveBefore(InsertPoint);
          Changed = true;
        }
        LocalInstSet.erase(Def);
      }
    }
  }
  if (Changed && CTX->m_instrTypes.hasDebugInfo) {
    ProcessDbgValueInst(*BB, DT);
  }
  return Changed;
}

bool CodeSinking::isSafeToMove(Instruction *inst, bool &reducePressure, bool &hasAliasConcern,
                               SmallPtrSetImpl<Instruction *> &Stores) {
  if (isa<AllocaInst>(inst) || isa<ExtractValueInst>(inst)) {
    return false;
  }
  if (isa<CallInst>(inst) && cast<CallInst>(inst)->isConvergent()) {
    return false;
  }
  hasAliasConcern = true;
  reducePressure = false;
  if (isa<GetElementPtrInst>(inst) || isa<ExtractElementInst>(inst) || isa<InsertElementInst>(inst) ||
      isa<InsertValueInst>(inst) || (isa<UnaryInstruction>(inst) && !isa<LoadInst>(inst)) ||
      isa<BinaryOperator>(inst)) {
    hasAliasConcern = false;
    // sink CmpInst to make the flag-register lifetime short
    reducePressure = (isCastInstrReducingPressure(inst, true) || isa<CmpInst>(inst));
    return true;
  }
  if (isa<CmpInst>(inst)) {
    hasAliasConcern = false;
    reducePressure = true;
    return true;
  }
  EOPCODE intrinsic_name = GetOpCode(inst);
  if (intrinsic_name == llvm_input || intrinsic_name == llvm_shaderinputvec) {
    if (IGC_IS_FLAG_ENABLED(DisableCodeSinkingInputVec)) {
      hasAliasConcern = true;
      reducePressure = false;
      return false;
    }
    hasAliasConcern = false;
    reducePressure = true;
    return true;
  }


  if (IsMathIntrinsic(intrinsic_name) || IsGradientIntrinsic(intrinsic_name)) {
    hasAliasConcern = false;
    reducePressure = false;
    return true;
  }
  if (isSampleInstruction(inst) || isGather4Instruction(inst) || isInfoInstruction(inst) || isLdInstruction(inst)) {
    if (!inst->mayReadFromMemory()) {
      hasAliasConcern = false;
      return true;
    }
  }
  if (isSubGroupIntrinsic(inst)) {
    return false;
  }

  if (LoadInst *load = dyn_cast<LoadInst>(inst)) {
    if (load->isVolatile())
      return false;

    BufferType bufType = GetBufferType(load->getPointerAddressSpace());
    if (bufType == CONSTANT_BUFFER || bufType == RESOURCE) {
      hasAliasConcern = false;
      return true;
    }
    if (!Stores.empty()) {
      return false;
    }
  } else if (SamplerLoadIntrinsic *intrin = dyn_cast<SamplerLoadIntrinsic>(inst)) {
    Value *texture = intrin->getTextureValue();
    if (texture->getType()->isPointerTy()) {
      unsigned as = texture->getType()->getPointerAddressSpace();
      BufferType bufType = GetBufferType(as);
      if (bufType == CONSTANT_BUFFER || bufType == RESOURCE) {
        hasAliasConcern = false;
        return true;
      } else {
        return (Stores.empty());
      }
    } else {
      hasAliasConcern = false;
      return true;
    }
  } else if (inst->mayReadFromMemory()) {
    return (Stores.empty());
  }

  return true;
}

/// AllUsesDominatedByBlock - Return true if all uses of the specified value
/// occur in blocks dominated by the specified block.
bool CodeSinking::allUsesDominatedByBlock(Instruction *inst, BasicBlock *blk,
                                          SmallPtrSetImpl<Instruction *> &usesInBlk) const {
  usesInBlk.clear();
  // Ignoring debug uses is necessary so debug info doesn't affect the code.
  // This may leave a referencing dbg_value in the original block, before
  // the definition of the vreg.  Dwarf generator handles this although the
  // user might not get the right info at runtime.
  for (Value::user_iterator I = inst->user_begin(), E = inst->user_end(); I != E; ++I) {
    // Determine the block of the use.
    Instruction *useInst = cast<Instruction>(*I);
    BasicBlock *useBlock = useInst->getParent();
    if (useBlock == blk) {
      usesInBlk.insert(useInst);
    }
    if (PHINode *PN = dyn_cast<PHINode>(useInst)) {
      // PHI nodes use the operand in the predecessor block,
      // not the block with the PHI.
      Use &U = I.getUse();
      unsigned num = PHINode::getIncomingValueNumForOperand(U.getOperandNo());
      useBlock = PN->getIncomingBlock(num);
    }
    // Check that it dominates.
    if (!DT->dominates(blk, useBlock))
      return false;
  }
  return true;
}

uint CodeSinking::estimateLiveOutPressure(BasicBlock *blk, const DataLayout *DL) {
  // Walk the basic block bottom-up.  Remember if we saw a store.
  uint pressure = 0;
  BasicBlock::iterator I = blk->end();
  --I;
  bool processedBegin = false;
  do {
    Instruction *inst = &(*I); // The instruction to sink.

    // Predecrement I (if it's not begin) so that it isn't invalidated by sinking.
    processedBegin = (I == blk->begin());
    if (!processedBegin)
      --I;

    if (isa<DbgInfoIntrinsic>(inst))
      continue;
    // intrinsic like discard has no explicit use, get skipped here
    if (inst->use_empty())
      continue;

    bool useOutside = false;
    for (Value::user_iterator useI = inst->user_begin(), useE = inst->user_end(); !useOutside && useI != useE; ++useI) {
      // Determine the block of the use.
      Instruction *useInst = cast<Instruction>(*useI);
      BasicBlock *useBlock = useInst->getParent();
      if (useBlock != blk) {
        if (PHINode *PN = dyn_cast<PHINode>(useInst)) {
          // PHI nodes use the operand in the predecessor block,
          // not the block with the PHI.
          Use &U = useI.getUse();
          unsigned num = PHINode::getIncomingValueNumForOperand(U.getOperandNo());
          if (PN->getIncomingBlock(num) != blk) {
            useOutside = true;
          }
        } else {
          useOutside = true;
        }
      }
    }

    // estimate register usage by value
    if (useOutside) {
      pressure += (uint)(DL->getTypeAllocSize(inst->getType()));
    }
    // If we just processed the first instruction in the block, we're done.
  } while (!processedBegin);
  return pressure;
}

void CodeSinking::rollbackSinking(BasicBlock *BB) {
  // undo code motion
  int NumChanges = MovedInsts.size();
  for (int i = 0; i < NumChanges; ++i) {
    Instruction *UndoLoca = UndoLocas[i];
    IGC_ASSERT(UndoLoca->getParent() == BB);
    MovedInsts[i]->moveBefore(UndoLoca);
  }
}

/// ==================///
/// Loop code sinking ///
/// ==================///

// Sink in the loop if loop preheader's potential to sink covers at least 20% of registers delta
// between grf number and max estimated pressure in the loop
#define LOOPSINK_PREHEADER_IMPACT_THRESHOLD 0.2
#define LOOPSINK_RESCHEDULE_ITERATIONS 5

// Helper functions for loop sink debug dumps
#define PrintDump(Level, Contents)                                                                                     \
  if (IGC_IS_FLAG_ENABLED(DumpLoopSink) && (Level <= IGC_GET_FLAG_VALUE(LoopSinkDumpLevel))) {                         \
    *LogStream << Contents;                                                                                            \
  }
#define PrintInstructionDump(Level, Inst)                                                                              \
  if (IGC_IS_FLAG_ENABLED(DumpLoopSink) && (Level <= IGC_GET_FLAG_VALUE(LoopSinkDumpLevel))) {                         \
    (Inst)->print(*LogStream, false);                                                                                  \
    *LogStream << "\n";                                                                                                \
  }
#define PrintOUGDump(Level, OUG)                                                                                       \
  if (IGC_IS_FLAG_ENABLED(DumpLoopSink) && (Level <= IGC_GET_FLAG_VALUE(LoopSinkDumpLevel))) {                         \
    OUG.print(*LogStream);                                                                                             \
    *LogStream << "\n";                                                                                                \
  }

// Register pass to igc-opt
#define PASS_FLAG1 "igc-code-loop-sinking"
#define PASS_DESCRIPTION1 "code loop sinking"
#define PASS_CFG_ONLY1 false
#define PASS_ANALYSIS1 false
IGC_INITIALIZE_PASS_BEGIN(CodeLoopSinking, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(VectorShuffleAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(IGCLivenessAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(IGCFunctionExternalRegPressureAnalysis)
IGC_INITIALIZE_PASS_END(CodeLoopSinking, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)

char CodeLoopSinking::ID = 0;
CodeLoopSinking::CodeLoopSinking() : FunctionPass(ID), LogStringStream(Log) {
  if (IGC_IS_FLAG_ENABLED(PrintToConsole))
    LogStream = &IGC::Debug::ods();
  else
    LogStream = &LogStringStream;
  initializeCodeLoopSinkingPass(*PassRegistry::getPassRegistry());
}

bool CodeLoopSinking::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  CTX = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  if (CTX->type != ShaderType::OPENCL_SHADER)
    return false;

  if (IGC_IS_FLAG_ENABLED(DisableCodeSinking) || numInsts(F) < IGC_GET_FLAG_VALUE(CodeLoopSinkingMinSize)) {
    return false;
  }

  if (IGC_IS_FLAG_ENABLED(DumpLoopSink)) {
    auto printGlobalSettings = [](llvm::raw_ostream &LogStream) {
      // print every value to the dump
      LogStream << "ForceLoopSink: " << IGC_GET_FLAG_VALUE(ForceLoopSink) << "\n";
      LogStream << "EnableLoadsLoopSink: " << IGC_GET_FLAG_VALUE(EnableLoadsLoopSink) << "\n";
      LogStream << "ForceLoadsLoopSink: " << IGC_GET_FLAG_VALUE(ForceLoadsLoopSink) << "\n";
      LogStream << "PrepopulateLoadChainLoopSink: " << IGC_GET_FLAG_VALUE(PrepopulateLoadChainLoopSink) << "\n";
      LogStream << "EnableLoadChainLoopSink: " << IGC_GET_FLAG_VALUE(EnableLoadChainLoopSink) << "\n";
      LogStream << "LoopSinkRegpressureMargin: " << IGC_GET_FLAG_VALUE(LoopSinkRegpressureMargin) << "\n";
      LogStream << "CodeLoopSinkingMinSize: " << IGC_GET_FLAG_VALUE(CodeLoopSinkingMinSize) << "\n";
      LogStream << "CodeSinkingLoadSchedulingInstr: " << IGC_GET_FLAG_VALUE(CodeSinkingLoadSchedulingInstr) << "\n";
      LogStream << "LoopSinkMinSaveUniform: " << IGC_GET_FLAG_VALUE(LoopSinkMinSaveUniform) << "\n";
      LogStream << "LoopSinkMinSave: " << IGC_GET_FLAG_VALUE(LoopSinkMinSave) << "\n";
      LogStream << "LoopSinkThresholdDelta: " << IGC_GET_FLAG_VALUE(LoopSinkThresholdDelta) << "\n";
      LogStream << "LoopSinkRollbackThreshold: " << IGC_GET_FLAG_VALUE(LoopSinkRollbackThreshold) << "\n";
      LogStream << "LoopSinkEnableLoadsRescheduling: " << IGC_GET_FLAG_VALUE(LoopSinkEnableLoadsRescheduling) << "\n";
      LogStream << "LoopSinkCoarserLoadsRescheduling: " << IGC_GET_FLAG_VALUE(LoopSinkCoarserLoadsRescheduling) << "\n";
      LogStream << "LoopSinkEnable2dBlockReads: " << IGC_GET_FLAG_VALUE(LoopSinkEnable2dBlockReads) << "\n";
      LogStream << "LoopSinkEnableVectorShuffle: " << IGC_GET_FLAG_VALUE(LoopSinkEnableVectorShuffle) << "\n";
      LogStream << "LoopSinkForceRollback: " << IGC_GET_FLAG_VALUE(LoopSinkForceRollback) << "\n";
      LogStream << "LoopSinkDisableRollback: " << IGC_GET_FLAG_VALUE(LoopSinkDisableRollback) << "\n";
      LogStream << "LoopSinkAvoidSplittingDPAS: " << IGC_GET_FLAG_VALUE(LoopSinkAvoidSplittingDPAS) << "\n";
    };

    Log.clear();

    printGlobalSettings(*LogStream);

    PrintDump(VerbosityLevel::Low, "=====================================\n");
    PrintDump(VerbosityLevel::Low, "Function " << F.getName() << "\n");
  }

  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();
  TLI = &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();

  // Note: FRPE is a Module analysis and currently it runs only once.
  // If function A calls function B then
  // it's possible that transformation of function A reduces the regpressure good enough
  // and we could not apply sinking in function B, but we don't recompute FPRE
  // to save compile time, so in this case LoopSinking might apply for loops in function B

  RPE = &getAnalysis<IGCLivenessAnalysis>().getLivenessRunner();
  FRPE = &getAnalysis<IGCFunctionExternalRegPressureAnalysis>();
  WI = &FRPE->getWIAnalysis(&F);

  // clear caching structures before handling the new function
  MemoizedStoresInLoops.clear();
  BlacklistedLoops.clear();
  BBPressures.clear();

  bool Changed = loopSink(F);

  if (IGC_IS_FLAG_ENABLED(DumpLoopSink) && IGC_IS_FLAG_DISABLED(PrintToConsole)) {
    dumpToFile(Log);
  }

  IGC_ASSERT(false == verifyFunction(F, &dbgs()));

  return Changed;
}

void CodeLoopSinking::dumpToFile(const std::string &Log) {
  auto Name = Debug::DumpName(IGC::Debug::GetShaderOutputName())
                  .Hash(CTX->hash)
                  .Type(CTX->type)
                  .Retry(CTX->m_retryManager->GetRetryId())
                  .Pass("loopsink")
                  .Extension("txt");
  IGC::Debug::DumpLock();
  std::ofstream OutputFile(Name.str(), std::ios_base::app);
  if (OutputFile.is_open()) {
    OutputFile << Log;
  }
  OutputFile.close();
  IGC::Debug::DumpUnlock();
}

// Implementation of RPE->getMaxRegCountForLoop(*L, SIMD);
// with per-BB pressure caching to improve compile-time
uint CodeLoopSinking::getMaxRegCountForLoop(Loop *L) {
  IGC_ASSERT(RPE);
  Function *F = L->getLoopPreheader()->getParent();
  uint SIMD = numLanes(RPE->bestGuessSIMDSize(F));
  unsigned int Max = 0;
  for (BasicBlock *BB : L->getBlocks()) {
    auto BBPressureEntry = BBPressures.try_emplace(BB);
    unsigned int &BBPressure = BBPressureEntry.first->second;
    if (BBPressureEntry.second) // BB was not in the set, need to recompute
    {
      BBPressure = RPE->getMaxRegCountForBB(*BB, SIMD, WI);
    }
    Max = std::max(BBPressure, Max);
  }
  return Max;
}

// this function returns the best known regpressure, not up-to-date repgressure
// it was implemented this way to cut compilation time costs
uint CodeLoopSinking::getMaxRegCountForFunction(Function *F) {
  unsigned int MaxPressure = 0;
  for (const auto &BB : BBPressures) {
    if (BB.getFirst()->getParent() != F)
      continue;
    MaxPressure = std::max(BB.getSecond(), MaxPressure);
  }
  return MaxPressure;
}

// Find the loops with too high regpressure and sink the instructions from
// preheaders into them
bool CodeLoopSinking::loopSink(Function &F) {
  bool Changed = false;
  for (auto &L : LI->getLoopsInPreorder()) {
    LoopSinkMode SinkMode = IGC_IS_FLAG_ENABLED(ForceLoopSink) ? LoopSinkMode::FullSink : LoopSinkMode::NoSink;

    if (SinkMode == LoopSinkMode::NoSink)
      SinkMode = needLoopSink(L);
    if (SinkMode != LoopSinkMode::NoSink)
      Changed |= loopSink(L, SinkMode);
  }

  unsigned int MaxPressure = getMaxRegCountForFunction(&F);
  RPE->publishRegPressureMetadata(F, MaxPressure + FRPE->getExternalPressureForFunction(&F));
  return Changed;
}

LoopSinkMode CodeLoopSinking::needLoopSink(Loop *L) {
  BasicBlock *Preheader = L->getLoopPreheader();
  if (!Preheader)
    return LoopSinkMode::NoSink;
  if (!RPE)
    return LoopSinkMode::NoSink;

  Function *F = Preheader->getParent();
  uint GRFThresholdDelta = IGC_GET_FLAG_VALUE(LoopSinkThresholdDelta);
  uint NGRF = CTX->getNumGRFPerThread();
  uint SIMD = numLanes(RPE->bestGuessSIMDSize(F));

  PrintDump(VerbosityLevel::Low, "\n");
  if (!Preheader->getName().empty()) {
    PrintDump(VerbosityLevel::Low, "Checking loop with preheader " << Preheader->getName() << ": \n");
  } else if (!Preheader->empty()) {
    PrintDump(VerbosityLevel::Low, "Checking loop with unnamed preheader. First preheader instruction:\n");
    Instruction *First = &Preheader->front();
    PrintInstructionDump(VerbosityLevel::Low, First);
  } else {
    PrintDump(VerbosityLevel::Low, "Checking loop with unnamed empty preheader.");
  }

  if (!CTX->platform.isCoreChildOf(IGFX_XE3_CORE) && IGC_IS_FLAG_ENABLED(LoopSinkEnableLoadsRescheduling)) {
    for (auto &BB : L->getBlocks()) {
      for (auto &I : *BB) {
        if (is2dBlockRead(&I)) {
          PrintDump(VerbosityLevel::Low, ">> Loop has 2D block reads. Enabling loads rescheduling and sinking.\n");
          return IGC_IS_FLAG_ENABLED(LoopSinkForce2dBlockReadsMaxSink) ? LoopSinkMode::FullSink
                                                                       : LoopSinkMode::SinkWhileRegpressureIsHigh;
        }
      }
    }
  }

  // Estimate preheader's potential to sink
  ValueSet PreheaderDefs = RPE->getDefs(*Preheader);
  // Filter out preheader defined values that are used not in the loop or not supported
  ValueSet PreheaderDefsCandidates;
  for (Value *V : PreheaderDefs) {
    Instruction *I = dyn_cast<Instruction>(V);
    if (I && mayBeLoopSinkCandidate(I, L)) {
      PreheaderDefsCandidates.insert(V);
    }
  }

  if (PreheaderDefsCandidates.empty()) {
    PrintDump(VerbosityLevel::Low, ">> No sinking candidates in the preheader.\n");
    return LoopSinkMode::NoSink;
  }

  uint PreheaderDefsSizeInBytes = RPE->estimateSizeInBytes(PreheaderDefsCandidates, *F, SIMD, WI);
  uint PreheaderDefsSizeInRegs = RPE->bytesToRegisters(PreheaderDefsSizeInBytes);

  // Estimate max pressure in the loop and the external pressure
  uint MaxLoopPressure = getMaxRegCountForLoop(L);
  uint FunctionExternalPressure = FRPE ? FRPE->getExternalPressureForFunction(F) : 0;

  auto isSinkCriteriaMet = [&](uint MaxLoopPressure) {
    // loop sinking is needed if the loop's pressure is higher than number of GRFs by threshold
    // and preheader's potential to reduce the delta is good enough
    return ((MaxLoopPressure > NGRF + GRFThresholdDelta) &&
            (PreheaderDefsSizeInRegs > (MaxLoopPressure - NGRF) * LOOPSINK_PREHEADER_IMPACT_THRESHOLD));
  };

  PrintDump(VerbosityLevel::Low, "Threshold to sink = " << NGRF + GRFThresholdDelta << "\n");
  PrintDump(VerbosityLevel::Low, "MaxLoopPressure = " << MaxLoopPressure << "\n");
  PrintDump(VerbosityLevel::Low,
            "MaxLoopPressure + FunctionExternalPressure = " << MaxLoopPressure + FunctionExternalPressure << "\n");
  PrintDump(VerbosityLevel::Low, "PreheaderDefsSizeInRegs = " << PreheaderDefsSizeInRegs << "\n");
  PrintDump(VerbosityLevel::Low, "PreheaderPotentialThreshold = "
                                     << uint((MaxLoopPressure - NGRF) * LOOPSINK_PREHEADER_IMPACT_THRESHOLD) << "\n");

  // Sink if the regpressure in the loop is high enough (including function external regpressure)
  if (isSinkCriteriaMet(MaxLoopPressure + FunctionExternalPressure))
    return LoopSinkMode::SinkWhileRegpressureIsHigh;

  PrintDump(VerbosityLevel::Low, ">> No sinking.\n");
  return LoopSinkMode::NoSink;
}

bool CodeLoopSinking::allUsesAreInLoop(Instruction *I, Loop *L) {
  for (const User *UserInst : I->users()) {
    if (!isa<Instruction>(UserInst))
      return false;
    if (!L->contains(cast<Instruction>(UserInst)))
      return false;
  }
  return true;
}

// Adapter for the common function findLowestSinkTarget
// Ignore the uses in the BB and IsOuterLoop side effects
BasicBlock *CodeLoopSinking::findLowestLoopSinkTarget(Instruction *I, Loop *L) {
  SmallPtrSet<Instruction *, 16> UsesInBB;
  bool IsOuterLoop = false;
  BasicBlock *TgtBB = findLowestSinkTarget(I, UsesInBB, IsOuterLoop, true, DT, LI);
  if (!TgtBB)
    return nullptr;
  if (!L->contains(TgtBB))
    return nullptr;
  return TgtBB;
}

bool CodeLoopSinking::loopSink(Loop *L, LoopSinkMode Mode) {
  // Sink loop invariants back into the loop body if register
  // pressure can be reduced.

  IGC_ASSERT(L);

  // No Preheader, stop!
  BasicBlock *Preheader = L->getLoopPreheader();
  if (!Preheader)
    return false;

  PrintDump(VerbosityLevel::Low, ">> Sinking in the loop with preheader " << Preheader->getName() << "\n");

  Function *F = Preheader->getParent();
  uint NGRF = CTX->getNumGRFPerThread();

  uint InitialLoopPressure = getMaxRegCountForLoop(L);
  uint MaxLoopPressure = InitialLoopPressure;

  uint FunctionExternalPressure = FRPE ? FRPE->getExternalPressureForFunction(F) : 0;
  uint NeededRegpressure = NGRF - IGC_GET_FLAG_VALUE(LoopSinkRegpressureMargin);
  if ((NeededRegpressure >= FunctionExternalPressure) && (Mode == LoopSinkMode::SinkWhileRegpressureIsHigh)) {
    NeededRegpressure -= FunctionExternalPressure;
    PrintDump(VerbosityLevel::Low, "Targeting new own regpressure in the loop = " << NeededRegpressure << "\n");
  } else {
    Mode = LoopSinkMode::FullSink;
    PrintDump(VerbosityLevel::Low, "Doing full sink.\n");
  }

  PrintDump(VerbosityLevel::Low, "Initial regpressure:\n" << InitialLoopPressure << "\n");

  // We can only affect Preheader and the loop.
  // Collect affected BBs to invalidate cached regpressure
  // and request recomputation of liveness analysis preserving not affected BBs
  BBSet AffectedBBs;
  AffectedBBs.insert(Preheader);
  for (BasicBlock *BB : L->blocks())
    AffectedBBs.insert(BB);

  // Save original positions for rollback
  DenseMap<BasicBlock *, InstrVec> OriginalPositions;
  for (BasicBlock *BB : AffectedBBs) {
    InstrVec BBInstructions;
    for (Instruction &I : *BB)
      BBInstructions.push_back(&I);
    OriginalPositions[BB] = std::move(BBInstructions);
  }

  auto rerunLiveness = [&]() {
    for (BasicBlock *BB : AffectedBBs)
      BBPressures.erase(BB);
    RPE->rerunLivenessAnalysis(*F, &AffectedBBs);
  };

  bool EverChanged = false;

  InstSet LoadChains;

  if (IGC_IS_FLAG_ENABLED(PrepopulateLoadChainLoopSink))
    prepopulateLoadChains(L, LoadChains);

  bool AllowLoadSinking = IGC_IS_FLAG_ENABLED(ForceLoadsLoopSink);
  bool AllowOnlySingleUseLoadChainSinking = false;
  bool IterChanged = false;

  bool AchievedNeededRegpressure = false;
  bool RecomputeMaxLoopPressure = false;

  auto isBeneficialToSinkBitcast = [&](Instruction *I, Loop *L, bool AllowLoadSinking = false) {
    BitCastInst *BC = dyn_cast<BitCastInst>(I);
    if (!BC)
      return false;

    Value *Op = BC->getOperand(0);

    // if Op has uses in the loop then it's beneficial
    for (const User *UserInst : Op->users()) {
      if (!isa<Instruction>(UserInst))
        return false;
      if (L->contains(cast<Instruction>(UserInst)))
        return true;
    }

    Instruction *LI = dyn_cast<Instruction>(Op);
    if (!LI || !isAllowedLoad(LI))
      return true;

    // Either load will be sinked before bitcast or the loaded value would anyway be alive
    // in the whole loop body. So it's safe to sink the bitcast
    if (BC->hasOneUse())
      return true;

    // Now it makes sense to sink bitcast only if it would enable load sinking
    // Otherwise it can lead to the increase of register pressure
    if (!AllowLoadSinking)
      return false;

    // Check the load would be a candidate if not for this bitcast
    for (const User *UserInst : LI->users()) {
      if (!isa<Instruction>(UserInst))
        return false;
      if (dyn_cast<BitCastInst>(UserInst) == BC)
        continue;
      if (!L->contains(cast<Instruction>(UserInst)))
        return false;
    }

    return isSafeToLoopSinkLoad(LI, L);
  };

  // "Leaf" candidate is the one that doesn't use any other candidate
  // This function returns the map Instruction *->Candidate * of leaf candidates
  // and updates the Candidates vector and InstToCandidateMap to contain only non-leaf candidates
  auto getLeafInstToCandidateMap = [&](BasicBlock *TgtBB, CandidatePtrVec &Candidates,
                                       InstToCandidateMap &InstToCandidate) {
    InstToCandidateMap LeafInstToCandidate;
    CandidatePtrSet NotLeafCandidates;

    for (const auto &C : Candidates) {
      PrintDump(VerbosityLevel::High, "Finding leaf candidates... Checking:\n");
      for (Instruction *I : *C) {
        PrintInstructionDump(VerbosityLevel::High, I);
      }

      for (Instruction *I : *C) {
        // if any operand is a candidate, then this candidate is not a leaf
        for (auto OI = I->op_begin(), E = I->op_end(); OI != E; OI++) {
          Instruction *Op = dyn_cast<Instruction>(OI);
          if (!Op)
            continue;

          if (InstToCandidate.count(Op)) {
            const auto &OpCandidate = InstToCandidate[Op];
            if (OpCandidate != C) {
              PrintDump(VerbosityLevel::High, "Operand uses the current candidate, so is not a leaf:\n");
              PrintInstructionDump(VerbosityLevel::High, Op);
              NotLeafCandidates.insert(OpCandidate);
            }
          }
        }
      }
    }
    for (const auto &C : Candidates) {
      if (NotLeafCandidates.count(C))
        continue;

      for (Instruction *I : *C) {
        LeafInstToCandidate[I] = C;
        InstToCandidate.erase(I);
      }
    }

    Candidates = CandidatePtrVec(NotLeafCandidates.begin(), NotLeafCandidates.end());
    return LeafInstToCandidate;
  };

  auto rescheduleCandidates = [&](BasicBlock *BB, CandidateVec &SinkedCandidates,
                                  InstToCandidateMap &CurrentInstToCandidate, const int MaxLocalSchedulingIterations,
                                  bool Aggressive = false) {
    bool Changed = false;

    CandidatePtrVec SinkedCandidatesPtrs;
    for (auto *CI = SinkedCandidates.begin(), *CE = SinkedCandidates.end(); CI != CE; CI++) {
      const auto &C = *CI;
      if (C->TgtBB == BB)
        SinkedCandidatesPtrs.push_back(C);
    }

    // Sinking the candidates that don't use other candidates iteratively
    // Should end with break, using max number of iterations (MaxLocalSchedulingIterations) just to avoid an infinite
    // loop
    for (int i = 0; i < MaxLocalSchedulingIterations; i++) {
      PrintDump(VerbosityLevel::Medium, "Local scheduling iteration " << i << "...\n");
      InstToCandidateMap LeafCurrentInstToCandidate =
          getLeafInstToCandidateMap(BB, SinkedCandidatesPtrs, CurrentInstToCandidate);
      if (LeafCurrentInstToCandidate.empty()) {
        PrintDump(VerbosityLevel::Medium, "No more candidates to schedule in this block.\n");
        break;
      }
      Changed |= localSink(BB, LeafCurrentInstToCandidate, Aggressive);
    }

    return Changed;
  };

  bool ReschedulingIteration =
      !CTX->platform.isCoreChildOf(IGFX_XE3_CORE) && IGC_IS_FLAG_ENABLED(LoopSinkEnableLoadsRescheduling);
  bool LateReschedulingIteration = false;

  auto createSimpleCandidates = [&](InstSet &SkipInstructions, CandidateVec &SinkCandidates) {
    bool Changed = false;
    for (auto II = Preheader->rbegin(), IE = Preheader->rend(); II != IE;) {
      Instruction *I = &*II++;

      if (SkipInstructions.count(I))
        continue;

      if (!allUsesAreInLoop(I, L))
        continue;

      LoopSinkWorthiness Worthiness = LoopSinkWorthiness::Unknown;

      if (AllowOnlySingleUseLoadChainSinking) {
        if (!isLoadChain(I, LoadChains, true))
          continue;

        Worthiness = LoopSinkWorthiness::Sink;
      } else if (isa<BinaryOperator>(I) || isa<CastInst>(I)) {
        Worthiness = LoopSinkWorthiness::MaybeSink;

        if (isCastInstrReducingPressure(I, false))
          Worthiness = LoopSinkWorthiness::Sink;

        if (isBeneficialToSinkBitcast(I, L, AllowLoadSinking))
          Worthiness = LoopSinkWorthiness::Sink;
      }

      if (isAlwaysSinkInstruction(I)) {
        Worthiness = LoopSinkWorthiness::Sink;
      }

      // if LoadInst or GenISA_LSC2DBlockRead (standalone, non-payload allowed 2d load)
      GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(I);
      if (isa<LoadInst>(I) || (GII && GII->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockRead)) {
        if (!AllowLoadSinking)
          continue;

        if (!isSafeToLoopSinkLoad(I, L))
          continue;

        Worthiness = LoopSinkWorthiness::MaybeSink;
      }

      if (Worthiness == LoopSinkWorthiness::Sink || Worthiness == LoopSinkWorthiness::MaybeSink) {
        BasicBlock *TgtBB = findLowestLoopSinkTarget(I, L);
        if (!TgtBB)
          continue;

        Changed = true;
        SinkCandidates.push_back(std::make_shared<Candidate>(I, TgtBB, Worthiness, I->getNextNode()));
        continue;
      }

      IGC_ASSERT(Worthiness == LoopSinkWorthiness::Unknown);

      // Handle payload 2d loads separately as they go together with auxilary intrinsics
      // 2d block loads are usually large, so the sinking is beneficial when it's safe
      if (AllowLoadSinking && IGC_IS_FLAG_ENABLED(LoopSinkEnable2dBlockReads))
        tryCreate2dBlockReadGroupSinkingCandidate(I, L, SkipInstructions, SinkCandidates);
    }

    return Changed;
  };

  CandidateVec SinkedCandidates;
  InstToCandidateMap InstToCandidate;

  CandidateVec CurrentSinkCandidates;
  InstToCandidateMap CurrentInstToCandidate;

  // Candidate ownership:
  // Shared pointers are created in CurrentSinkCandidates on every iteration.
  // Then they are put in ToSink collection to be sinked (done in refineLoopSinkCandidates).
  // Then they are put in SinkedCandidates within iteration if they are actually sinked.
  // The actually sinked Candidates have therefore lifetime until the end ot loopSink function.
  //
  // CurrentInstToCandidate and InstToCandidate are maps Instruction->std::shared_ptr<Candidate>
  //
  // It's assumed that using std::shared_ptr we will successfully ensure only needed Candidates
  // will remain.

  InstSet SkipInstructions;

  int SinkIterations = 0;

  do {
    CurrentSinkCandidates.clear();
    CurrentInstToCandidate.clear();
    SkipInstructions.clear();

    // Moving LI back to the loop
    // If we sinked something we could allow sinking of the previous instructions as well
    // on the next iteration of do-loop
    //
    // For example, here we sink 2 EE first and need one more iteration to sink load:
    // preheader:
    //   %l = load <2 x double>
    //   extractelement 1, %l
    //   extractelement 2, %l
    // loop:
    //   ...

    IterChanged = false;

    // Try rescheduling the loads that are already in the loop
    // by adding them as a candidates, so that they are moved to the first use by LocalSink
    // Do it only once before starting sinking
    if (ReschedulingIteration) {
      PrintDump(VerbosityLevel::Low, "Trying to find loads to reschedule...\n");
      if (IGC_IS_FLAG_ENABLED(LoopSinkEnable2dBlockReads)) {
        // traverse Loop in the reverse order
        for (auto BBI = L->block_begin(), BBE = L->block_end(); BBI != BBE; BBI++) {
          BasicBlock *BB = *BBI;
          for (auto BI = BB->rbegin(), BE = BB->rend(); BI != BE; BI++) {
            Instruction *I = &*BI;
            GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(I);

            bool Found2dBlockReads = false;

            // If it's a non-payload 2d block load we can create candidate if it's safe to move
            if (GII && GII->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockRead) {
              if (SkipInstructions.count(I))
                continue;
              SkipInstructions.insert(I);

              if (!isSafeToLoopSinkLoad(I, L))
                continue;

              PrintDump(VerbosityLevel::Medium, "Found 2D block read to reschedule:\n");
              PrintInstructionDump(VerbosityLevel::Medium, I);

              CurrentSinkCandidates.push_back(
                  std::make_shared<Candidate>(I, I->getParent(), LoopSinkWorthiness::IntraLoopSink, I->getNextNode()));
              Found2dBlockReads = true;
            }

            // Handle possible payload 2d block loads separately
            Found2dBlockReads |=
                tryCreate2dBlockReadGroupSinkingCandidate(I, L, SkipInstructions, CurrentSinkCandidates);

            if (Found2dBlockReads && IGC_IS_FLAG_ENABLED(LoopSinkEnableVectorShuffle)) {
              // If there are 2d block reads we try to find vector shuffle patterns for rescheduling as well
              tryCreateShufflePatternCandidates(BB, L, SkipInstructions, CurrentSinkCandidates);
            }
          }
        }
      }
    } else if (!LateReschedulingIteration) {
      PrintDump(VerbosityLevel::Low, "Starting sinking iteration...\n");
      SinkIterations++;

      for (auto &Pair : InstToCandidate)
        SkipInstructions.insert(Pair.first);

      // lowered vector shuffle patterns are beneficial to sink,
      // because they can enable further sinking of the large loads
      // Create such candidates first
      if (IGC_IS_FLAG_ENABLED(LoopSinkEnableVectorShuffle))
        tryCreateShufflePatternCandidates(L->getLoopPreheader(), L, SkipInstructions, CurrentSinkCandidates);

      // Create simple (1-instr) candidates for sinking by traversing the preheader once
      createSimpleCandidates(SkipInstructions, CurrentSinkCandidates);
    } else {
      PrintDump(VerbosityLevel::Low, "Late rescheduling iteration...\n");
    }

    // Sink the beneficial instructions
    bool IterChanged = false;

    IterChanged |= LateReschedulingIteration;

    if (!LateReschedulingIteration) {
      // Make decisions for "MaybeSink" candidates
      CandidateVec ToSink = refineLoopSinkCandidates(CurrentSinkCandidates, LoadChains, L);

      for (auto &C : ToSink) {
        if (C->Worthiness == LoopSinkWorthiness::Sink || C->Worthiness == LoopSinkWorthiness::IntraLoopSink) {
          IGC_ASSERT(C->size() > 0);

          SinkedCandidates.push_back(C);

          bool SinkFromPH = C->Worthiness == LoopSinkWorthiness::Sink;
          Instruction *InsertPoint = SinkFromPH ? &*(C->TgtBB->getFirstInsertionPt()) : C->first()->getNextNode();

          for (Instruction *I : *C) {
            PrintDump(VerbosityLevel::Medium,
                      (SinkFromPH ? "Sinking instruction:\n" : "Scheduling instruction for local sink:\n"));
            PrintInstructionDump(VerbosityLevel::Medium, I);

            CurrentInstToCandidate[I] = C;
            InstToCandidate[I] = C;

            I->moveBefore(InsertPoint);
            InsertPoint = I;

            if (SinkFromPH) {
              if (isAllowedLoad(I) || isLoadChain(I, LoadChains))
                LoadChains.insert(I);
            }
          }

          UndoBlkSet.insert(C->UndoPos->getParent());
          LocalBlkSet.insert(C->TgtBB);

          PrintDump(VerbosityLevel::Medium, "\n");
          IterChanged = true;
        }
      }
    }

    if (IterChanged) {
      EverChanged = true;

      // Getting the size of the sinked on this iteration candidates
      // Must be before local sinking
      auto SIMD = numLanes(RPE->bestGuessSIMDSize(F));
      ValueSet InstsSet;
      for (auto &Pair : CurrentInstToCandidate) {
        InstsSet.insert(Pair.first);
      }
      uint SinkedSizeInBytes = RPE->estimateSizeInBytes(InstsSet, *F, SIMD, WI);
      uint SinkedSizeInRegs = RPE->bytesToRegisters(SinkedSizeInBytes);

      if (LateReschedulingIteration) {
        for (auto &C : SinkedCandidates) {
          LocalBlkSet.insert(C->TgtBB);
        }
      }

      // Invoke localSink() to move def to its first use
      if (LocalBlkSet.size() > 0) {
        for (auto BI = LocalBlkSet.begin(), BE = LocalBlkSet.end(); BI != BE; BI++) {
          BasicBlock *BB = *BI;

          if (ReschedulingIteration) {
            rescheduleCandidates(BB, SinkedCandidates, CurrentInstToCandidate, LOOPSINK_RESCHEDULE_ITERATIONS);
          } else if (LateReschedulingIteration) {
            InstToCandidateMap InstToCandidateCopy = InstToCandidate;
            rescheduleCandidates(BB, SinkedCandidates, InstToCandidateCopy,
                                 LOOPSINK_RESCHEDULE_ITERATIONS + SinkIterations, true);
          } else // sinking iteration
          {
            localSink(BB, CurrentInstToCandidate);
          }
        }
        LocalBlkSet.clear();
      }

      if (!LateReschedulingIteration) // do one more sinking iteration only if it's a sinking iteration
        if (MaxLoopPressure - SinkedSizeInRegs > NeededRegpressure) {
          // Heuristic to save recalculation of liveness
          // The size of the candidates set is not enough to reach the needed regpressure
          PrintDump(VerbosityLevel::Low, "Running one more iteration without recalculating liveness...\n");
          RecomputeMaxLoopPressure = true;
          ReschedulingIteration = false;
          continue;
        }

      rerunLiveness();
      MaxLoopPressure = getMaxRegCountForLoop(L);
      RecomputeMaxLoopPressure = false;
      PrintDump(VerbosityLevel::Low, "New max loop pressure = " << MaxLoopPressure << "\n");

      if (LateReschedulingIteration)
        break;

      if ((MaxLoopPressure < NeededRegpressure) && (Mode == LoopSinkMode::SinkWhileRegpressureIsHigh)) {
        AchievedNeededRegpressure = true;
        if (IGC_IS_FLAG_ENABLED(EnableLoadChainLoopSink) && !LoadChains.empty()) {
          PrintDump(VerbosityLevel::Low, "Allowing only chain sinking...\n");
          AllowOnlySingleUseLoadChainSinking = true;
        } else {
          PrintDump(VerbosityLevel::Low, "Achieved needed regpressure, finished.\n");
          break;
        }
      }
    } else if (!ReschedulingIteration) // sinking iteration
    {
      if (!AllowLoadSinking && IGC_IS_FLAG_ENABLED(EnableLoadsLoopSink)) {
        PrintDump(VerbosityLevel::Low, "Allowing loads...\n");
        AllowLoadSinking = true;
      } else if (!AchievedNeededRegpressure && Mode == LoopSinkMode::SinkWhileRegpressureIsHigh &&
                 IGC_IS_FLAG_ENABLED(LoopSinkEnableLateRescheduling)) {
        LateReschedulingIteration = true;
      } else {
        PrintDump(VerbosityLevel::Low, "Nothing to sink, finished.\n");
        break;
      }
    }

    ReschedulingIteration = false;
  } while (true);

  if (!EverChanged) {
    PrintDump(VerbosityLevel::Low, "No changes were made in this loop.\n");
    return false;
  }

  if (RecomputeMaxLoopPressure) {
    rerunLiveness();
    MaxLoopPressure = getMaxRegCountForLoop(L);
  }

  PrintDump(VerbosityLevel::Low, "New max loop pressure = " << MaxLoopPressure << "\n");

  bool NeedToRollback = IGC_IS_FLAG_ENABLED(LoopSinkForceRollback);

  // We always estimate if the sinking of a candidate is beneficial.
  // So it's unlikely we increase the regpressure in the loop.
  //
  // But due to iterative approach we have some heuristics to sink
  // instruction that don't reduce the regpressure immediately in order to
  // enable the optimization for some potential candidates on the next iteration.
  // Rollback the transformation if the result regpressure becomes higher
  // as a result of such speculative sinking.
  if (MaxLoopPressure > InitialLoopPressure) {
    PrintDump(VerbosityLevel::Low, "Loop pressure increased after sinking.\n");
    NeedToRollback = true;
  }

  // If we haven't achieved the needed regpressure, it's possible that even if the sinking
  // would be beneficial for small GRF, there still will be spills.
  // In this case there is a chance that just choosing
  // more GRF will be enough to eliminate spills and we would degrade performance
  // if we sinked. So we rollback the changes if autoGRF is provided
  if (Mode == LoopSinkMode::SinkWhileRegpressureIsHigh && !AchievedNeededRegpressure &&
      (NGRF <= 128 && CTX->isAutoGRFSelectionEnabled()) &&
      MaxLoopPressure >= (NGRF + IGC_GET_FLAG_VALUE(LoopSinkRollbackThreshold))) {
    PrintDump(VerbosityLevel::Low, "AutoGRF is enabled and the needed regpressure is not achieved:\n");
    PrintDump(VerbosityLevel::Low, "New max loop pressure = " << MaxLoopPressure << "\n");
    PrintDump(VerbosityLevel::Low,
              "Threshold to rollback = " << NGRF + IGC_GET_FLAG_VALUE(LoopSinkRollbackThreshold) << "\n");

    NeedToRollback = true;
  }

  if (NeedToRollback && IGC_IS_FLAG_DISABLED(LoopSinkDisableRollback)) {
    PrintDump(VerbosityLevel::Low, ">> Reverting the changes.\n\n");

    for (auto &[BB, BBInstructions] : OriginalPositions) {
      Instruction *InsertPoint = nullptr;
      for (Instruction *I : BBInstructions) {
        if (InsertPoint)
          I->moveAfter(InsertPoint);
        else
          I->moveBefore(&*BB->getFirstInsertionPt());

        InsertPoint = I;
      }
    }

    rerunLiveness();
    return false;
  }

  if (CTX->m_instrTypes.hasDebugInfo) {
    for (BasicBlock *BB : UndoBlkSet) {
      ProcessDbgValueInst(*BB, DT);
    }
  }

  // We decided we don't rollback, change the names of the instructions in IR
  for (auto &Pair : InstToCandidate) {
    Instruction *I = Pair.first;
    const auto &C = Pair.second;
    if (I->getType()->isVoidTy())
      continue;
    std::string Prefix = C->Worthiness == LoopSinkWorthiness::IntraLoopSink ? "sched" : "sink";
    I->setName(Prefix + "_" + I->getName());
  }

  return true;
}

// Try to create a candidate for sinking 2d block read group.
//
// If every use of the AddrPayload is in the same BB and every use is a GenISA_LSC2DBlockReadAddrPayload or
// GenISA_LSC2DBlockSetAddrPayloadField and it's safe to sink the loads
//
// Example:
// %Block2D_AddrPayload = call i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64 %base_addr, i32 127, i32 1023,
// i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)
//
// The candidate will be the following group of instructions:
// call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 5, i1 false)
// call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 6, i1 false)
// %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32
// 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
bool CodeLoopSinking::tryCreate2dBlockReadGroupSinkingCandidate(Instruction *I, Loop *L, InstSet &SkipInstructions,
                                                                CandidateVec &SinkCandidates) {
  BasicBlock *PH = L->getLoopPreheader();

  GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(I);
  if (!Intr)
    return false;

  auto Id = Intr->getIntrinsicID();
  if (Id != GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload &&
      Id != GenISAIntrinsic::GenISA_LSC2DBlockSetAddrPayloadField) {
    return false;
  }

  GenIntrinsicInst *AddrPayload = dyn_cast<GenIntrinsicInst>(I->getOperand(0));
  if (!AddrPayload)
    return false;

  if (SkipInstructions.count(cast<Instruction>(AddrPayload)))
    return false;

  PrintDump(VerbosityLevel::Medium, "Found 2d block read instruction, trying to create candidate group:\n");
  PrintInstructionDump(VerbosityLevel::Medium, I);
  PrintDump(VerbosityLevel::Medium, "AddrPayload:\n");
  PrintInstructionDump(VerbosityLevel::Medium, AddrPayload);

  bool Start = false;

  InstrVec CandidateInsts;
  BasicBlock *TgtBB = nullptr;

  // Traversing the PH or the BB in the loop in reverse order
  // from the found instruction to the beginning
  // or the AddrPayload inst if it's in the block

  if (I->getParent() == PH) {
    PrintDump(VerbosityLevel::High, "Traversing the preheader...\n");
  } else {
    PrintDump(VerbosityLevel::High, "Traversing the BB with the instruction...\n");
  }

  for (auto IB = I->getParent()->rbegin(), IE = I->getParent()->rend(); IB != IE;) {
    Instruction *II = &*IB++;

    if (II == AddrPayload)
      break;

    if (II == I)
      Start = true;
    else {
      if (!Start) {
        continue;
      } else {
        Intr = dyn_cast<GenIntrinsicInst>(II);
        if (!Intr)
          continue;
        Id = Intr->getIntrinsicID();
      }
    }

    if (II->getOperand(0) != AddrPayload) {
      continue;
    }

    SkipInstructions.insert(II);

    // We expect to see only the following intrinsics for this AddrPayload
    if (Id == GenISAIntrinsic::GenISA_LSC2DBlockSetAddrPayloadField) {
      PrintDump(VerbosityLevel::High, "Found GenISA_LSC2DBlockSetAddrPayloadField:\n");
      PrintInstructionDump(VerbosityLevel::High, II);
      CandidateInsts.push_back(II);
    } else if (Id == GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload) {
      PrintDump(VerbosityLevel::High, "Found GenISA_LSC2DBlockReadAddrPayload:\n");
      PrintInstructionDump(VerbosityLevel::High, II);

      if (!isSafeToLoopSinkLoad(II, L)) {
        PrintDump(VerbosityLevel::High, "Not safe to sink the load, skipping.\n");
        return false;
      }
      PrintDump(VerbosityLevel::High, "Safe to sink the load.\n");

      BasicBlock *CurrentTgtBB = I->getParent() == PH ? findLowestLoopSinkTarget(I, L) : I->getParent();
      if (!CurrentTgtBB)
        return false;

      if (!TgtBB)
        TgtBB = CurrentTgtBB;
      else
        TgtBB = DT->findNearestCommonDominator(TgtBB, CurrentTgtBB);

      if (TgtBB != CurrentTgtBB) {
        if (I->getParent() == PH) {
          TgtBB = DT->findNearestCommonDominator(TgtBB, CurrentTgtBB);
          if (!TgtBB) {
            PrintDump(VerbosityLevel::High, "No common dominator found, skipping.\n");
            return false;
          }
        } else {
          PrintDump(VerbosityLevel::High, "Not all the uses are in the same BB, skipping.\n");
          return false;
        }
      }

      PrintDump(VerbosityLevel::High, "Adding the instruction to this candidate group.\n");
      CandidateInsts.push_back(II);
    } else {
      PrintDump(VerbosityLevel::High, "Unexpected intrinsic, skipping:\n");
      PrintInstructionDump(VerbosityLevel::High, II);

      return false;
    }
  }

  SkipInstructions.insert(AddrPayload);

  if (!TgtBB) {
    PrintDump(VerbosityLevel::High, "No target block to sink, skipping.\n");
    return false;
  }

  // The creation of address payload can be in a different BB, we don't sink it
  // All other uses should be in the same BB
  if (CandidateInsts.size() != AddrPayload->getNumUses()) {
    PrintDump(VerbosityLevel::High, "Not all the uses of the AddrPayload are in the same BB, skipping.\n");
    return false;
  }

  // Check that all the uses are dominated by the remaining uses

  // We have a number of current candidates, they will be placed before their uses.
  // The remaining instructions are initially placed earlier than the current candidates.
  // If the remaining instructions are dominated by the current candidates, we can split the current candidates
  // So that they are scheduled separately, because in this case the order will be not changed.
  auto allUsesAreDominatedByRemainingUses = [&](InstrVec &CurrentCandidateInsts, InstSet &RemainingCandidateInsts) {
    auto instUsesDominateAllCurrentCandidateUses = [&](Instruction *RI) {
      for (User *RU : RI->users()) {
        Instruction *RUI = dyn_cast<Instruction>(RU);
        if (!RUI)
          return false;
        for (Instruction *CI : CurrentCandidateInsts) {
          for (User *CU : CI->users()) {
            Instruction *CUI = dyn_cast<Instruction>(CU);
            if (!CUI)
              return false;
            if (!DT->dominates(RUI, CUI))
              return false;
          }
        }
      }
      return true;
    };

    return std::all_of(RemainingCandidateInsts.begin(), RemainingCandidateInsts.end(),
                       instUsesDominateAllCurrentCandidateUses);
  };

  // If the uses are not dominated by the UndoPoint
  // It's possible that we put some instructions after their uses on rollback
  // So it needs to be checked if we sink not from PH
  auto allUsesAreDominatedByUndoPoint = [&](InstrVec &CurrentCandidateInsts, Instruction *UndoPoint) {
    for (Instruction *CI : CurrentCandidateInsts) {
      for (User *CU : CI->users()) {
        Instruction *CUI = dyn_cast<Instruction>(CU);
        if (!CUI)
          return false;
        if (!DT->dominates(UndoPoint, CUI))
          return false;
      }
    }
    return true;
  };

  auto assertOneLoad = [&](InstrVec &CurrentCandidateInsts) {
    int NumLoads = 0;
    for (Instruction *CI : CurrentCandidateInsts) {
      GenIntrinsicInst *CurIntr = cast<GenIntrinsicInst>(CI);
      if (CurIntr->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload) {
        NumLoads++;
      }
    }
    IGC_ASSERT(NumLoads == 1);
  };

  typedef SmallSet<int, 16> FieldIndicesSet;

  auto getAllSetFieldIndices = [&](InstrVec &CurrentCandidateInsts) {
    FieldIndicesSet AllSetFieldIndices;
    for (Instruction *CI : CurrentCandidateInsts) {
      GenIntrinsicInst *CurIntr = cast<GenIntrinsicInst>(CI);
      if (CurIntr->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockSetAddrPayloadField) {
        int FieldIndex = cast<ConstantInt>(CurIntr->getOperand(1))->getZExtValue();
        AllSetFieldIndices.insert(FieldIndex);
      }
    }
    return AllSetFieldIndices;
  };

  // All the uses are a candidate
  // Try splitting them into separate candidates for better scheduling within a BB
  bool SinkFromPH = I->getParent() == PH;
  auto Worthiness = SinkFromPH ? LoopSinkWorthiness::Sink : LoopSinkWorthiness::IntraLoopSink;

  DenseMap<Instruction *, DenseMap<int, Value *>> AddrPayloadFieldValues;
  DenseMap<int, Value *> CurrentAddrPayloadFieldValues;

  // Collect information about what fields are set before the load
  // AddrPayloadFieldValues can then be used to create more SetField intrinsics enabling finer scheduling
  // in case different fields are set before the load

  // iterate SinkCandidates in reverse order - so the instruction appear in the direct order in the BB
  for (auto II = CandidateInsts.rbegin(), IE = CandidateInsts.rend(); II != IE; II++) {
    GenIntrinsicInst *CurIntr = cast<GenIntrinsicInst>(*II);

    if (CurIntr->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockSetAddrPayloadField) {
      int FieldIndex = cast<ConstantInt>(CurIntr->getOperand(1))->getZExtValue();
      CurrentAddrPayloadFieldValues[FieldIndex] = CurIntr->getOperand(2);
    } else if (CurIntr->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload) {
      AddrPayloadFieldValues[*II] = DenseMap<int, Value *>(CurrentAddrPayloadFieldValues);
    }
  }

  // keys of map CurrentAddrPayloadFieldValues contain all fields
  // that were set at least by one SetField instruction
  FieldIndicesSet AllFields;
  for (auto &Pair : CurrentAddrPayloadFieldValues) {
    AllFields.insert(Pair.first);
  }

  InstrVec CurrentCandidateInsts;
  InstSet RemainingCandidateInsts(CandidateInsts.begin(), CandidateInsts.end());

  uint NCandidates = 0;
  for (Instruction *I : CandidateInsts) {
    GenIntrinsicInst *CurIntr = dyn_cast<GenIntrinsicInst>(I);
    if (!CurIntr)
      return false;

    auto Id = CurIntr->getIntrinsicID();

    if (CurrentCandidateInsts.size() > 0 && Id == GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload) {
      if (!SinkFromPH && !allUsesAreDominatedByUndoPoint(CurrentCandidateInsts, CurrentCandidateInsts[0])) {
        PrintDump(VerbosityLevel::High, "Not all the uses are dominated by the UndoPoint, skipping.\n");
        return false;
      }

      if (IGC_IS_FLAG_ENABLED(LoopSinkCoarserLoadsRescheduling)) {
        if (allUsesAreDominatedByRemainingUses(CurrentCandidateInsts, RemainingCandidateInsts)) {
          NCandidates++;
          SinkCandidates.push_back(std::make_shared<Candidate>(CurrentCandidateInsts, TgtBB, Worthiness,
                                                               CurrentCandidateInsts[0]->getNextNode()));
          CurrentCandidateInsts.clear();
        }
      } else {
        // We are going to create a separate Candidate for every load

        if (getAllSetFieldIndices(CurrentCandidateInsts) != AllFields) {
          /*
          The SetField intrinsics are not in SSA form and the order of them is important,
          as when we schedule the load together with the previous SetFields changing the order may affect the result

          For example, if we change the order of the 2 loads in the following example then load2 will no more have the
          field #2 == 80

          call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 2, i32 80, i1
          false) call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32
          3, i1 false) call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32
          6, i32 3, i1 false) %load3 = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8*
          %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

          call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 2, i1
          false) call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32
          2, i1 false) %load2 = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8*
          %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

          It's possible to create more GenISA_LSC2DBlockSetAddrPayloadField intrinsic calls, in this example
          Create SetField to set field 2 to 80 before load2.

          For this AddrPayloadFieldValues[BlockRead] can be used. If the field is not set in the
          AddrPayloadFieldValues[BlockRead] then the field should be taken from AddrPayload operands.

          For now it's unsupported as we can only have fields 5 and 6, so we skip the candidate creations for this
          AddrPayload completely.
          */

          PrintDump(VerbosityLevel::High, "Not all the fields are set, skipping the payload.\n");
          PrintDump(VerbosityLevel::High, "2d Block read:") Instruction *BlockRead =
              cast<Instruction>(CurrentCandidateInsts[0]);
          PrintInstructionDump(VerbosityLevel::High, BlockRead);
          PrintDump(VerbosityLevel::High, "AddrPayload:");
          PrintInstructionDump(VerbosityLevel::High, AddrPayload);

          return false;
        }

        assertOneLoad(CurrentCandidateInsts);
        NCandidates++;
        SinkCandidates.push_back(std::make_shared<Candidate>(CurrentCandidateInsts, TgtBB, Worthiness,
                                                             CurrentCandidateInsts[0]->getNextNode()));
        CurrentCandidateInsts.clear();
      }
    }
    CurrentCandidateInsts.push_back(I);
    RemainingCandidateInsts.erase(I);
  }

  if (CurrentCandidateInsts.size() > 0) {
    if (!SinkFromPH && !allUsesAreDominatedByUndoPoint(CurrentCandidateInsts, CurrentCandidateInsts[0])) {
      PrintDump(VerbosityLevel::High, "Not all the uses are dominated by the UndoPoint, skipping.\n");
      return false;
    }
    NCandidates++;
    SinkCandidates.push_back(
        std::make_shared<Candidate>(CurrentCandidateInsts, TgtBB, Worthiness, CurrentCandidateInsts[0]->getNextNode()));
  }

  PrintDump(VerbosityLevel::Medium, "Successfully created " << NCandidates << " candidates.\n");
  return NCandidates > 0;
}

CodeLoopSinking::StoresVec CodeLoopSinking::getAllStoresInLoop(Loop *L) {
  IGC_ASSERT(!BlacklistedLoops.count(L));

  // if all the stores for this loop are not memoized yet, do it first
  if (!MemoizedStoresInLoops.count(L)) {
    StoresVec &StoresInLoop = MemoizedStoresInLoops[L];
    for (BasicBlock *BB : L->blocks()) {
      for (Instruction &I : *BB) {
        if (I.mayWriteToMemory()) {
          StoresInLoop.push_back(&I);
        }
      }
    }
  }
  return MemoizedStoresInLoops[L];
}

bool CodeLoopSinking::tryCreateShufflePatternCandidates(BasicBlock *BB, Loop *L, InstSet &SkipInstructions,
                                                        CandidateVec &SinkCandidates) {
  BasicBlock *Preheader = L->getLoopPreheader();
  bool SinkFromPH = BB == Preheader;

  // It's possible that a large vector is shuffled to different smaller vectors
  // but if all the vector components are used only in ExtractElement and
  // InsertElement instructions we can sink all EE and IE instructions

  // This function checks if all the uses of the source vector are used in the dest vectors
  // that are defined by the first IE instruction (that has "undef" as a dst operand)

  // As a side effect, it also populates the DestVecMap
  // that maps the last IE instruction to the set of all the IE and EE instructions of
  // a particular dest vector
  auto SourceVectorIsFullyShuffled = [&](Value *SourceVec, SmallVectorImpl<InsertElementInst *> &IEs,
                                         DenseMap<InsertElementInst *, InstSet> &DestVecMap) {
    auto SourceVectorType = dyn_cast<IGCLLVM::FixedVectorType>(SourceVec->getType());
    if (!SourceVectorType)
      return false;
    auto SourceElemType = SourceVectorType->getElementType();
    if (!SourceElemType->isSingleValueType())
      return false;

    SmallSet<uint64_t, 32> EEIndices;

    for (InsertElementInst *CurrentBaseIE : IEs) {
      InsertElementInst *CurrentIE = CurrentBaseIE;
      InsertElementInst *LastIE = CurrentBaseIE;
      SmallSet<uint64_t, 32> IEIndices;
      auto IEVectorType = cast<IGCLLVM::FixedVectorType>(CurrentIE->getType());
      auto IEElemType = IEVectorType->getElementType();

      // support only the same base types for now
      if (IEElemType != SourceElemType)
        return false;

      // the set of all instruction for this dest vector
      InstSet ShuffleInst;

      for (;;) {
        auto *Idx = cast<ConstantInt>(CurrentIE->getOperand(2));
        IEIndices.insert(Idx->getZExtValue());
        ShuffleInst.insert(CurrentIE);

        ExtractElementInst *CurrentEE = dyn_cast<ExtractElementInst>(CurrentIE->getOperand(1));
        if (!CurrentEE || CurrentEE->getParent() != BB || (CurrentEE->getOperand(0) != SourceVec) ||
            !CurrentEE->hasOneUse()) {
          return false;
        }

        ShuffleInst.insert(CurrentEE);
        auto *IdxEE = cast<ConstantInt>(CurrentEE->getOperand(1));
        EEIndices.insert(IdxEE->getZExtValue());

        LastIE = CurrentIE;
        User *U = IGCLLVM::getUniqueUndroppableUser(CurrentIE);
        if (!U)
          break;

        CurrentIE = dyn_cast<InsertElementInst>(U);
        if (!CurrentIE)
          break;

        if (CurrentIE->getParent() != BB)
          break;
      }

      // We need to check all the indices are used in IE instructons
      // to guarantee there are no more other uses
      // of the dest vector.
      // Only "LastIE" instruction can have other uses
      if (IEIndices.size() != IEVectorType->getNumElements())
        return false;

      DestVecMap[LastIE] = std::move(ShuffleInst);
    }

    // The same logic with EE of the source vector:
    // we need all the indices are used
    if (EEIndices.size() != SourceVectorType->getNumElements())
      return false;

    // Check that all the source vector uses are in the ShuffleInst set
    bool AllSourceVecUsesInShuffleInst = std::all_of(SourceVec->user_begin(), SourceVec->user_end(), [&](User *U) {
      return std::any_of(
          DestVecMap.begin(), DestVecMap.end(),
          [&](const std::pair<InsertElementInst *, InstSet> &Pair) { return Pair.second.count(cast<Instruction>(U)); });
    });

    return AllSourceVecUsesInShuffleInst;
  };

  bool Changed = false;

  PrintDump(VerbosityLevel::Low, "Trying to create shuffle pattern candidates...\n");

  // Map {Source vector Value : InsertElement instructions that create a new vector}
  SmallDenseMap<Value *, SmallVector<InsertElementInst *, 4>> SourceVectors;
  for (Instruction &I : *BB) {
    if (auto *IE = dyn_cast<InsertElementInst>(&I)) {
      if (!isa<UndefValue>(IE->getOperand(0)))
        continue;

      ExtractElementInst *EE = dyn_cast<ExtractElementInst>(IE->getOperand(1));
      if (!EE)
        continue;
      if (EE->getParent() != BB)
        continue;
      Instruction *Source = dyn_cast<Instruction>(EE->getVectorOperand());
      if (!Source)
        continue;
      if (!isAllowedLoad(Source))
        continue;

      SourceVectors[EE->getVectorOperand()].push_back(IE);
    }
  }

  DenseMap<InsertElementInst *, InstSet> DestVecToShuffleInst;
  InstToCandidateMap ShuffleInstToCandidate;

  for (auto &VecIEs : SourceVectors) {
    DestVecToShuffleInst.clear();

    auto *SourceVec = VecIEs.first;
    auto &IEs = VecIEs.second;

    if (!SourceVectorIsFullyShuffled(SourceVec, IEs, DestVecToShuffleInst))
      continue;

    // We proved it's full shuffle pattern, but we need to also prove the last IE instructions
    // are candidates to sink, and collect them in the right order

    // In the following code DestVec means the last IE instruction
    DenseMap<InsertElementInst *, BasicBlock *> DestVecToTgtBB;
    for (auto &Pair : DestVecToShuffleInst) {
      auto *DestVec = Pair.first;

      if (!allUsesAreInLoop(cast<Instruction>(DestVec), L)) {
        break;
      }

      BasicBlock *TgtBB = SinkFromPH ? findLowestLoopSinkTarget(cast<Instruction>(DestVec), L) : BB;

      if (!TgtBB)
        break;

      DestVecToTgtBB[DestVec] = TgtBB;
    }

    if (DestVecToTgtBB.size() == DestVecToShuffleInst.size()) {
      // Found the target BB for all the dest vectors, safe to sink for every dest vector
      // Create the candidates and populate the mapping between unordered shuffle instructions
      // to the corresponding candidate
      for (auto &Pair : DestVecToShuffleInst) {
        auto *DestVec = Pair.first;
        auto &ShuffleInst = Pair.second;
        auto *TgtBB = DestVecToTgtBB[DestVec];

        PrintDump(VerbosityLevel::Medium, "Instruction is a part of shuffle pattern, create a candidate:\n");
        PrintDump(VerbosityLevel::Medium, "DestVector used in the loop:\n");
        PrintInstructionDump(VerbosityLevel::Medium, DestVec);

        auto C = std::make_shared<Candidate>(
            InstrVec{}, TgtBB, SinkFromPH ? LoopSinkWorthiness::Sink : LoopSinkWorthiness::IntraLoopSink, nullptr);
        Changed = true;

        for (Instruction *I : ShuffleInst) {
          ShuffleInstToCandidate[I] = C;
        }
      }
    }
  }

  CandidatePtrVec ShuffleCandidatesOrdered;

  // Traverse BB in reverse order and populate Candidates instructions so that they are in the right order
  // Populate the ShuffleCandidatesOrdered with Candidates in the right order
  for (auto IB = BB->rbegin(), IE = BB->rend(); IB != IE; ++IB) {
    Instruction *I = &*IB;

    if (ShuffleInstToCandidate.count(I)) {
      auto &C = ShuffleInstToCandidate[I];
      if (C->size() == 0) {
        C->UndoPos = I->getNextNode();
        ShuffleCandidatesOrdered.push_back(C);
      }
      C->Instructions.push_back(I);
      SkipInstructions.insert(I);
    }
  }

  // Add the candidates to the main list
  for (const auto &C : ShuffleCandidatesOrdered) {
    SinkCandidates.push_back(C);
  }
  return Changed;
}

bool CodeLoopSinking::isAlwaysSinkInstruction(Instruction *I) {
  return (isa<IntToPtrInst>(I) || isa<PtrToIntInst>(I) || isa<ExtractElementInst>(I) || isa<InsertValueInst>(I));
}

// Check that this instruction is a part of address calc
// chain of an already sinked load
bool CodeLoopSinking::isLoadChain(Instruction *I, InstSet &LoadChains, bool EnsureSingleUser) {
  if (!isa<BinaryOperator>(I) && !isa<CastInst>(I))
    return false;
  User *InstrUser = IGCLLVM::getUniqueUndroppableUser(I);
  if (EnsureSingleUser && !InstrUser)
    return false;

  return std::all_of(I->user_begin(), I->user_end(), [&](User *U) {
    Instruction *UI = dyn_cast<Instruction>(U);
    return UI && LoadChains.count(UI);
  });
}

// Prepopulate load chain with the loads that are already in the loop
void CodeLoopSinking::prepopulateLoadChains(Loop *L, InstSet &LoadChains) {
  std::function<void(Value *)> addInstructionIfLoadChain = [&](Value *V) -> void {
    Instruction *I = dyn_cast<Instruction>(V);
    if (!I)
      return;

    if (!L->contains(I))
      return;

    if (!isLoadChain(I, LoadChains))
      return;

    LoadChains.insert(I);
    for (auto &U : I->operands()) {
      addInstructionIfLoadChain(U);
    }
  };

  for (BasicBlock *BB : L->blocks()) {
    for (Instruction &I : *BB) {
      // support only LoadInst for now
      if (LoadInst *LI = dyn_cast<LoadInst>(&I)) {
        LoadChains.insert(&I);
        addInstructionIfLoadChain(LI->getPointerOperand());
      }
    }
  }
}

/// isSafeToLoopSinkLoad - Determine whether it is safe to sink the load
/// instruction in the loop using alias information
bool CodeLoopSinking::isSafeToLoopSinkLoad(Instruction *InstToSink, Loop *L) {
  PrintDump(VerbosityLevel::High, "Checking if it is safe to sink the load:\n");
  PrintInstructionDump(VerbosityLevel::High, InstToSink);

  if (!L || !AA)
    return false;

  if (BlacklistedLoops.count(L))
    return false;

  if (!isAllowedLoad(InstToSink)) {
    PrintDump(VerbosityLevel::High, "Unsupported load\n");
    return false;
  }

  auto getRemainingStoresInBB = [](Instruction *I) {
    StoresVec Stores;
    BasicBlock *BB = I->getParent();
    Instruction *Last = BB->getTerminator();
    for (; I != Last; I = I->getNextNode()) {
      if (I->mayWriteToMemory()) {
        Stores.push_back(I);
      }
    }
    return Stores;
  };

  auto getMemLoc = [&](Instruction *I) {
    if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
      return MemoryLocation::get(SI);
    }
    if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
      return MemoryLocation::get(LI);
    }
    if (GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(I)) {
      switch (Intr->getIntrinsicID()) {
      case GenISAIntrinsic::GenISA_LSC2DBlockRead:
      case GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload:
      case GenISAIntrinsic::GenISA_LSC2DBlockWrite:
      case GenISAIntrinsic::GenISA_LSC2DBlockWriteAddrPayload:
        return MemoryLocation::getForArgument(Intr, 0, TLI);
      default:
        break;
      }
    }
    return MemoryLocation();
  };

  StoresVec RemainingStores;
  if (InstToSink->getParent() == L->getLoopPreheader()) {
    RemainingStores = getRemainingStoresInBB(InstToSink);
  } else {
    IGC_ASSERT(L->contains(InstToSink->getParent()));
  }

  StoresVec LoopStores = getAllStoresInLoop(L);
  MemoryLocation A = getMemLoc(InstToSink);
  for (auto Stores : {&RemainingStores, &LoopStores}) {
    for (Instruction *I : *Stores) {
      PrintDump(VerbosityLevel::High, "Store:\n");
      PrintInstructionDump(VerbosityLevel::High, I);

      bool UnsupportedStore = true;
      if (GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(I)) {
        switch (Intr->getIntrinsicID()) {
        // Reads
        case GenISAIntrinsic::GenISA_LSCPrefetch:
        case GenISAIntrinsic::GenISA_LSC2DBlockRead:
        case GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload:
        case GenISAIntrinsic::GenISA_LSC2DBlockPrefetchAddrPayload:
        case GenISAIntrinsic::GenISA_LSC2DBlockPrefetch:
        case GenISAIntrinsic::GenISA_LSC2DBlockSetAddrPayloadField:
          PrintDump(VerbosityLevel::High, "Load/prefetch instruction, may not alias\n");
          continue;

        // Change only registers
        case GenISAIntrinsic::GenISA_LSC2DBlockCreateAddrPayload:
        case GenISAIntrinsic::GenISA_dpas:
        case GenISAIntrinsic::GenISA_sub_group_dpas:
        case GenISAIntrinsic::GenISA_sub_group_bdpas:
          PrintDump(VerbosityLevel::High, "Not a real store instruction, may not alias\n");
          continue;

        // Wave intrinsics
        case GenISAIntrinsic::GenISA_WaveShuffleIndex:
        case GenISAIntrinsic::GenISA_WaveBroadcast:
        case GenISAIntrinsic::GenISA_WaveClusteredBroadcast:
        case GenISAIntrinsic::GenISA_WaveBallot:
        case GenISAIntrinsic::GenISA_WaveInverseBallot:
        case GenISAIntrinsic::GenISA_WaveClusteredBallot:
        case GenISAIntrinsic::GenISA_WaveAll:
        case GenISAIntrinsic::GenISA_WaveClustered:
        case GenISAIntrinsic::GenISA_WaveInterleave:
        case GenISAIntrinsic::GenISA_WavePrefix:
        case GenISAIntrinsic::GenISA_WaveClusteredPrefix:
          PrintDump(VerbosityLevel::High, "Not a real store instruction, may not alias\n");
          continue;

        // Supported writes
        case GenISAIntrinsic::GenISA_LSC2DBlockWrite:
        case GenISAIntrinsic::GenISA_LSC2DBlockWriteAddrPayload:
          UnsupportedStore = false;
          break;

        default:
          break;
        }
      } else if (isa<StoreInst>(I)) {
        UnsupportedStore = false;
      }

      if (UnsupportedStore) {
        PrintDump(VerbosityLevel::High, "Unsupported store\n");

        if (L->contains(I->getParent()))
          BlacklistedLoops.insert(L);
        return false;
      }

      MemoryLocation B = getMemLoc(I);
      if (!A.Ptr || !B.Ptr || AA->alias(A, B)) {
        PrintDump(VerbosityLevel::High, "May alias\n");
        return false;
      }
    }
  }

  PrintDump(VerbosityLevel::High, "Safe\n");
  return true;
}

// Very quick estimation to decide if we a going to sink in the loop
// The real Candidate selection will be done in CodeLoopSinking::loopSink()
bool CodeLoopSinking::mayBeLoopSinkCandidate(Instruction *I, Loop *L) {
  BasicBlock *PH = L->getLoopPreheader();

  // Limit sinking for the following case for now.
  for (User *UserInst : I->users()) {
    Instruction *II = dyn_cast<Instruction>(UserInst);

    if (!II)
      return false;

    if (!L->contains(II) && II->getParent() != PH)
      return false;
  }

  if (isAlwaysSinkInstruction(I) || isa<BinaryOperator>(I) || isa<CastInst>(I))
    return true;

  bool AllowLoadSinking = IGC_IS_FLAG_ENABLED(EnableLoadsLoopSink) || IGC_IS_FLAG_ENABLED(ForceLoadsLoopSink);
  if (AllowLoadSinking && isAllowedLoad(I)) {
    return isSafeToLoopSinkLoad(I, L);
  }

  return false;
}

CodeLoopSinking::CandidateVec CodeLoopSinking::refineLoopSinkCandidates(CandidateVec &SinkCandidates,
                                                                        InstSet &LoadChains, Loop *L) {
  struct OperandUseGroup {
    SmallPtrSet<Value *, 4> Operands;
    SmallVector<std::shared_ptr<Candidate>, 16> Users;

    void print(raw_ostream &OS) {
      OS << "OUG " << Operands.size() << " -> " << Users.size() << "\n";
      OS << "    Operands:\n";
      for (Value *V : Operands) {
        OS << "  ";
        V->print(OS);
        OS << "\n";
      }
      OS << "    Users:\n";
      for (auto &C : Users) {
        OS << "  ";
        C->print(OS);
        OS << "\n";
      }
    }
  };

  auto isUsedInLoop = [](Value *V, Loop *L) {
    if (isa<Constant>(V)) {
      // Ignore constant
      return false;
    }
    for (auto UI : V->users()) {
      if (Instruction *User = dyn_cast<Instruction>(UI)) {
        if (L->contains(User))
          return true;
      }
    }
    return false;
  };

  auto isUsedOnlyInLoop = [](Value *V, Loop *L) {
    return std::all_of(V->user_begin(), V->user_end(), [&](User *U) {
      Instruction *UI = dyn_cast<Instruction>(U);
      return UI && L->contains(UI);
    });
  };

  auto isSameSet = [](const SmallPtrSet<Value *, 4> &S0, const SmallPtrSet<Value *, 4> &S1) {
    if (S0.size() == S1.size()) {
      for (auto I : S1) {
        Value *V = I;
        if (!S0.count(V)) {
          return false;
        }
      }
      return true;
    }
    return false;
  };

  auto getNonConstCandidateOperandsOutsideLoop = [&](Candidate *C, Loop *L) {
    SmallPtrSet<Value *, 4> Operands;
    for (Instruction *I : *C) {
      for (Use &U : I->operands()) {
        Value *V = U;
        if (isa<Constant>(V) || isUsedInLoop(V, L))
          continue;
        Operands.insert(V);
      }
    }
    return Operands;
  };

  // Check if it's beneficial to sink it in the loop
  auto isBeneficialToSink = [&](OperandUseGroup &OUG) -> bool {
    auto getDstSize = [this](Value *V) {
      int DstSize = 0;
      Type *Ty = V->getType();
      if (Ty->isPointerTy()) {
        uint32_t addrSpace = cast<PointerType>(Ty)->getAddressSpace();
        int PtrSize = (int)CTX->getRegisterPointerSizeInBits(addrSpace);
        DstSize = PtrSize;
      } else {
        DstSize = (int)Ty->getPrimitiveSizeInBits();
      }
      return DstSize;
    };

    auto allUsersAreLoadChains = [&](OperandUseGroup &OUG) {
      return std::all_of(OUG.Users.begin(), OUG.Users.end(), [&](std::shared_ptr<Candidate> C) {
        return std::all_of(C->begin(), C->end(), [&](Instruction *I) { return isLoadChain(I, LoadChains); });
      });
    };

    // Estimate how much regpressure we save (in bytes).
    // Don't count uniform values. This way if every operand that is used only in the loop
    // is uniform, but the User (instruction to sink) is uniform, we'll decide it's beneficial to sink
    int AccSave = 0;

    for (Value *V : OUG.Operands) {
      int DstSize = getDstSize(V);
      if (!DstSize)
        return false;
      if (WI->isUniform(V))
        continue;
      AccSave -= DstSize / 8;
    }

    bool AllUsersAreUniform = true;
    for (const auto &C : OUG.Users) {
      for (Value *V : *C) {
        if (!V->hasNUsesOrMore(1))
          continue;
        if (!isUsedOnlyInLoop(V, L))
          continue;
        int DstSize = getDstSize(V);
        if (!DstSize)
          return false;
        if (WI->isUniform(V))
          continue;
        AllUsersAreUniform = false;
        AccSave += DstSize / 8;
      }
    }

    // If all uses are uniform, and we save enough SSA-values it's still beneficial
    if (AccSave >= 0 && AllUsersAreUniform &&
        ((int)OUG.Users.size() - (int)OUG.Operands.size() >= (int)(IGC_GET_FLAG_VALUE(LoopSinkMinSaveUniform)))) {
      return true;
    }

    // All instructions are part of a chain to already sinked load and don't
    // increase pressure too much. It simplifies the code a little and without
    // adding remat pass for simple cases
    if (AccSave >= 0 && allUsersAreLoadChains(OUG)) {
      return true;
    }

    // Compare estimated saved regpressure with the specified threshold
    // Number 4 here is just a constant multiplicator of the option to make the numbers more human-friendly,
    // as the typical minimum data size is usually 32-bit. 1 (=4b) means roughly 1 register of saved regpressure
    return AccSave >= (int)(IGC_GET_FLAG_VALUE(LoopSinkMinSave) * 4);
  };

  // For each candidate like the following:
  //   preheader:
  //            x = add y, z
  //   loop:
  //         ...
  //      BB:
  //           = x
  //
  // Afer sinking, x changes from global to local, and thus reduce pressure.
  // But y and z could change to global to local (if y and z are local).
  // Thus, we reduce pressure by 1 (x), but increase by the number of its
  // operands (y and z). If there are more candidates share the same operands,
  // we will reduce the pressure.  For example:
  //   preheader:
  //        x0 = add y, 10
  //        x1 = add y, 20
  //        x2 = add y, 100
  //        x3 = add y, 150
  //   loop:
  //         = x0
  //         = x1
  //         = x2
  //         = x3
  //
  // After sinking x0-x3 into loop, we make x0-x3 be local and make y be global,
  // which results in 3 (4 - 1) pressure reduction.
  //
  // Here we group all candidates based on its operands and select ones that definitely
  // reduce the pressure.
  //

  SmallVector<OperandUseGroup, 16> InstUseInfo;
  InstUseInfo.reserve(SinkCandidates.size());

  CandidateVec ToSink;

  for (const auto &C : SinkCandidates) {
    if (C->Worthiness == LoopSinkWorthiness::Sink || C->Worthiness == LoopSinkWorthiness::IntraLoopSink) {
      ToSink.push_back(C);
      continue;
    }

    const SmallPtrSet<Value *, 4> &CandidateOperands = getNonConstCandidateOperandsOutsideLoop(C.get(), L);

    // If this set of uses have been referenced by other instructions,
    // put this inst in the same group. Note that we don't union sets
    // that intersect each other.
    auto it = std::find_if(InstUseInfo.begin(), InstUseInfo.end(), [&](OperandUseGroup &OUG) {
      return CandidateOperands.size() > 0 && isSameSet(OUG.Operands, CandidateOperands);
    });

    if (it != InstUseInfo.end())
      it->Users.push_back(C);
    else
      InstUseInfo.push_back(OperandUseGroup{CandidateOperands, {C}});
  }

  // Check if it's beneficial to sink every OUG
  for (OperandUseGroup &OUG : InstUseInfo) {

    PrintDump(VerbosityLevel::Medium, "Checking if sinking the group is beneficial:\n");
    PrintOUGDump(VerbosityLevel::Medium, OUG);

    if (!isBeneficialToSink(OUG))
      continue;
    PrintDump(VerbosityLevel::Medium, ">> Beneficial to sink.\n\n");
    for (auto &C : OUG.Users) {
      C->Worthiness = LoopSinkWorthiness::Sink;
      ToSink.push_back(C);
    }
  }

  return ToSink;
}

// Sink to the use within basic block
bool CodeLoopSinking::localSink(BasicBlock *BB, InstToCandidateMap &InstToCandidate, bool Aggressive) {
  // A dpas macro sequence is a sequence of dpas without other
  // instructions in the middle. If a macro sequence is used in
  // this BB, skip sinking as code is likely manually-tuned code.
  //
  // The macro sequence normally has 8 dpas at least. Here, if there
  // are 8 dpas in the BB, assume the BB has a macro sequence.
  bool hasDPASMacro = false;
  if (IGC_IS_FLAG_ENABLED(LoopSinkSkipDPASMacro)) {
    int numDpas = 0;
    for (auto &II : *BB) {
      Instruction *I = &II;
      if (isDPAS(I)) {
        ++numDpas;
      }
    }
    hasDPASMacro = (numDpas >= 8);
  }

  auto isPartOfUnsplittableGroup = [&](Instruction *Inst) {
    auto haveCommonParameter = [](Instruction *Inst, Instruction *PrevInst) {
      for (unsigned i = 0; i < Inst->getNumOperands(); ++i) {
        for (unsigned j = 0; j < PrevInst->getNumOperands(); ++j) {
          Instruction *OpI = dyn_cast<Instruction>(Inst->getOperand(i));
          Instruction *OpPI = dyn_cast<Instruction>(PrevInst->getOperand(j));
          if (OpI && OpPI && OpI == OpPI)
            return true;
        }
      }
      return false;
    };

    if (IGC_IS_FLAG_ENABLED(LoopSinkAvoidSplittingDPAS) && isDPAS(Inst)) {
      if (!Aggressive)
        return true;

      // Aggressive local scheduling allows to sink in between DPASes
      // But we place only between DPAS instructions that don't have common parameters
      // (heuristic)
      PrintDump(VerbosityLevel::High, "Checking DPAS:\n");
      PrintInstructionDump(VerbosityLevel::High, Inst);

      Instruction *PrevInst = Inst->getPrevNode();
      if (!PrevInst || !isDPAS(PrevInst)) {
        if (PrevInst) {
          PrintDump(VerbosityLevel::High, "Previous instruction is not DPAS:\n");
          PrintInstructionDump(VerbosityLevel::High, PrevInst);
        }
        return false;
      }

      PrintDump(VerbosityLevel::High, "Checking previous DPAS:\n");
      PrintInstructionDump(VerbosityLevel::High, PrevInst);

      bool HCP = haveCommonParameter(Inst, PrevInst);
      PrintDump(VerbosityLevel::High, "Have common parameter: " << HCP << "\n");
      return HCP;
    }

    return false;
  };

  auto getInsertPointBeforeUse = [&](Instruction *InstToMove, Instruction *StartInsertPoint) {
    // Try scheduling the instruction earlier than the use.
    // Useful for loads to cover some latency.

    bool BreakAfterGroup = isPartOfUnsplittableGroup(StartInsertPoint);
    if (!BreakAfterGroup && !isAllowedLoad(InstToMove)) {
      PrintDump(VerbosityLevel::High, "Not part of unsplittable group and not a load. Place immediately.\n");
      return StartInsertPoint;
    }

    int Cnt = is2dBlockRead(InstToMove) ? IGC_GET_FLAG_VALUE(CodeSinking2dLoadSchedulingInstr)
                                        : IGC_GET_FLAG_VALUE(CodeSinkingLoadSchedulingInstr);

    Instruction *InsertPoint = StartInsertPoint;
    Instruction *I = StartInsertPoint->getPrevNode();
    for (;;) {
      if (I == nullptr)
        break;
      if (isa<PHINode>(I))
        break;
      if (std::any_of(I->use_begin(), I->use_end(),
                      [InstToMove](auto &U) { return llvm::cast<Instruction>(&U) == InstToMove; }))
        break;

      if (I->mayWriteToMemory()) {
        // At this point of the program we might have lost some information
        // About aliasing so don't schedule anything before possible stores
        // But it's OK to alias with prefetch
        GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(I);
        if (!(Intr && Intr->getIntrinsicID() == GenISAIntrinsic::GenISA_LSCPrefetch)) {
          break;
        }
      }

      InsertPoint = I;
      I = I->getPrevNode();

      if (isPartOfUnsplittableGroup(InsertPoint)) {
        BreakAfterGroup = true;
        continue;
      } else {
        if (BreakAfterGroup)
          break;
        else if (--Cnt <= 0)
          break;
      }
    }
    return InsertPoint;
  };

  bool Changed = false;
  for (auto &I : *BB) {
    Instruction *Use = &I;
    if (isa<PHINode>(Use))
      continue;

    PrintDump(VerbosityLevel::High, "Local sink: Checking use: ");
    PrintInstructionDump(VerbosityLevel::High, Use);

    auto UseCit = InstToCandidate.find(Use);
    if (UseCit != InstToCandidate.end()) {
      PrintDump(VerbosityLevel::High, "The instruction was sinked, skipping.\n");
      continue;
    }

    for (unsigned i = 0; i < Use->getNumOperands(); ++i) {
      Instruction *Def = dyn_cast<Instruction>(Use->getOperand(i));
      if (!Def)
        continue;

      if (Def->getParent() != BB)
        continue;

      // Skip load if there is DPAS macro
      if (hasDPASMacro && isAllowedLoad(Def))
        continue;

      auto Cit = InstToCandidate.find(Def);
      if (Cit == InstToCandidate.end())
        continue;

      PrintDump(VerbosityLevel::Medium, "Found candidate to local sink:\n");
      PrintInstructionDump(VerbosityLevel::Medium, Def);

      const auto &C = Cit->second;

      IGC_ASSERT(C->size() > 0);
      Instruction *MainInst = C->first();

      Instruction *InsertPoint = getInsertPointBeforeUse(MainInst, Use);

      PrintDump(VerbosityLevel::Medium, "Inserting before:\n");
      PrintInstructionDump(VerbosityLevel::Medium, InsertPoint);

      // Candidate can be a group of several instructions, so sinking the whole Candidate
      for (Instruction *CI : *C) {
        CI->moveBefore(InsertPoint);
        InstToCandidate.erase(CI);
        InsertPoint = CI;
      }

      Changed = true;
    }
  }
  if (Changed && CTX->m_instrTypes.hasDebugInfo) {
    ProcessDbgValueInst(*BB, DT);
  }
  return Changed;
}

} // namespace IGC
