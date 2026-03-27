/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===- CoroFrame.cpp - Builds and manipulates coroutine frame -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SplitAsyncUtils.h"
#include "RTBuilder.h"
#include "RTArgs.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Analysis/LoopInfo.h>
#include "common/LLVMWarningsPop.hpp"

#include <optional>

using namespace llvm;
using namespace IGC;

#if defined(_DEBUG) || defined(_INTERNAL)
#define REMAT_DIAG(X)                                                                                                  \
  if (m_pStream) {                                                                                                     \
    X;                                                                                                                 \
  }
#define REMAT_DIAG_SET_REASON(R)                                                                                       \
  if (m_pStream) {                                                                                                     \
    (*m_UniqueReasonMap)[m_RootInstruction] = R;                                                                       \
  }
#else
#define REMAT_DIAG(X)
#define REMAT_DIAG_SET_REASON(X)
#endif

static void rewritePHIs(BasicBlock &BB) {
  // For every incoming edge we will create a block holding all
  // incoming values in a single PHI nodes.
  //
  // loop:
  //    %n.val = phi i32[%n, %entry], [%inc, %loop]
  //
  // It will create:
  //
  // loop.from.entry:
  //    %n.loop.pre = phi i32 [%n, %entry]
  //    br %label loop
  // loop.from.loop:
  //    %inc.loop.pre = phi i32 [%inc, %loop]
  //    br %label loop
  //
  // After this rewrite, further analysis will ignore any phi nodes with more
  // than one incoming edge.

  // TODO: Simplify PHINodes in the basic block to remove duplicate
  // predecessors.

  SmallVector<BasicBlock *, 8> Preds(pred_begin(&BB), pred_end(&BB));
  for (BasicBlock *Pred : Preds) {
    auto *IncomingBB = SplitEdge(Pred, &BB);
    IncomingBB->setName(BB.getName() + Twine(".from.") + Pred->getName());
    auto *PN = cast<PHINode>(&BB.front());
    do {
      int Index = PN->getBasicBlockIndex(IncomingBB);
      Value *V = PN->getIncomingValue(Index);
      PHINode *InputV =
          PHINode::Create(V->getType(), 1, V->getName() + Twine(".") + BB.getName(), &IncomingBB->front());
      InputV->addIncoming(V, Pred);
      PN->setIncomingValue(Index, InputV);
      PN = dyn_cast<PHINode>(PN->getNextNode());
    } while (PN);
  }
}

namespace IGC {

void rewritePHIs(Function &F) {
  SmallVector<BasicBlock *, 8> WorkList;

  for (BasicBlock &BB : F)
    if (auto *PN = dyn_cast<PHINode>(&BB.front()))
      if (PN->getNumIncomingValues() > 1)
        WorkList.push_back(&BB);

  for (BasicBlock *BB : WorkList)
    ::rewritePHIs(*BB);
}

void insertSpills(CodeGenContext *CGCtx, Function &F, const SmallVector<Spill, 8> &Spills) {
  if (Spills.empty())
    return;

  uint32_t Idx = 0;
  DenseMap<Value *, uint32_t> SpillSlots;

  RTBuilder RTB(F.getContext(), *CGCtx);

  Value *CurrentValue = nullptr;
  BasicBlock *CurrentBlock = nullptr;
  Value *CurrentReload = nullptr;

  for (auto const &E : Spills) {
    if (CurrentValue != E.def()) {
      CurrentValue = E.def();
      CurrentBlock = nullptr;
      CurrentReload = nullptr;
    }

    uint32_t CurIdx = 0;
    if (auto I = SpillSlots.find(CurrentValue); I != SpillSlots.end()) {
      CurIdx = I->second;
    } else {
      CurIdx = Idx++;
      SpillSlots[CurrentValue] = CurIdx;

      Instruction *InsertPt = nullptr;

      if (isa<Argument>(CurrentValue))
        InsertPt = &F.getEntryBlock().front();
      else if (auto *PN = dyn_cast<PHINode>(CurrentValue))
        InsertPt = &*PN->getParent()->getFirstInsertionPt();
      else
        InsertPt = cast<Instruction>(CurrentValue)->getNextNode();

      RTB.SetInsertPoint(InsertPt);
      RTB.getSpillValue(CurrentValue, CurIdx);
    }

    if (CurrentBlock != E.userBlock()) {
      CurrentBlock = E.userBlock();
      RTB.SetInsertPoint(&*CurrentBlock->getFirstInsertionPt());
      CurrentReload =
          RTB.getFillValue(CurrentValue->getType(), CurIdx, VALUE_NAME(CurrentValue->getName() + Twine(".fill")));
    }

    // If we have a single edge PHINode, remove it and replace it with a
    // reload. We already took care of multi edge PHINodes by rewriting them
    // in the rewritePHIs function.
    if (auto *PN = dyn_cast<PHINode>(E.user())) {
      IGC_ASSERT_MESSAGE(PN->getNumIncomingValues() == 1, "unexpected number of incoming values in the PHINode");
      PN->replaceAllUsesWith(CurrentReload);
      PN->eraseFromParent();
      SpillSlots.insert(std::make_pair(CurrentReload, CurIdx));
      continue;
    }

    // Replace all uses of CurrentValue in the current instruction with reload.
    E.user()->replaceUsesOfWith(CurrentValue, CurrentReload);
  }
}

// For every use of the value that is across suspend point, recreate that value
// after a suspend point.
void rewriteMaterializableInstructions(const SmallVector<Spill, 8> &Spills) {
  BasicBlock *CurrentBlock = nullptr;
  Instruction *CurrentMaterialization = nullptr;
  Instruction *CurrentDef = nullptr;

  for (auto const &E : Spills) {
    // If it is a new definition, update CurrentXXX variables.
    if (CurrentDef != E.def()) {
      CurrentDef = cast<Instruction>(E.def());
      CurrentBlock = nullptr;
      CurrentMaterialization = nullptr;
    }

    // If we have not seen this block, materialize the value.
    if (CurrentBlock != E.userBlock()) {
      CurrentBlock = E.userBlock();
      CurrentMaterialization = CurrentDef->clone();
      CurrentMaterialization->setName(CurrentDef->getName());
      CurrentMaterialization->insertBefore(&*CurrentBlock->getFirstInsertionPt());
    }

    if (auto *PN = dyn_cast<PHINode>(E.user())) {
      IGC_ASSERT_MESSAGE(PN->getNumIncomingValues() == 1, "unexpected number of incoming values in the PHINode");
      PN->replaceAllUsesWith(CurrentMaterialization);
      PN->eraseFromParent();
      continue;
    }

    // Replace all uses of CurrentDef in the current instruction with the
    // CurrentMaterialization for the block.
    E.user()->replaceUsesOfWith(CurrentDef, CurrentMaterialization);
  }
}

RematChecker::RematChecker(CodeGenContext &Ctx, RematStage Stage) : Ctx(Ctx), Stage(Stage) {
#if defined(_DEBUG) || defined(_INTERNAL)
  m_pStream = nullptr;
  m_RootInstruction = nullptr;
#endif
}

RematChecker::RematChecker(CodeGenContext &Ctx, RematStage Stage, llvm::DominatorTree *DT, llvm::LoopInfo *LI)
    : Ctx(Ctx), Stage(Stage), DT(DT), LI(LI) {
#if defined(_DEBUG) || defined(_INTERNAL)
  m_pStream = nullptr;
  m_RootInstruction = nullptr;
#endif
}

#if defined(_DEBUG) || defined(_INTERNAL)
RematChecker::RematChecker(CodeGenContext &Ctx, RematStage Stage, llvm::DominatorTree *DT, llvm::LoopInfo *LI,
                           llvm::raw_ostream *Stream)
    : Ctx(Ctx), Stage(Stage), m_UniqueReasonMap(std::make_unique<RejectionReasonMapType>()), DT(DT), LI(LI),
      m_pStream(Stream) {}
#endif

bool RematChecker::isReadOnly(const Value *Ptr) const {
  uint32_t Addrspace = Ptr->getType()->getPointerAddressSpace();
  BufferType BufTy = GetBufferType(Addrspace);
  BufferAccessType Access = getDefaultAccessType(BufTy);
  return (Access == BufferAccessType::ACCESS_READ);
}

bool RematChecker::materializable(const Instruction &I) const {
  REMAT_DIAG(*m_pStream << "Query materializable: ");
  REMAT_DIAG(I.print(*m_pStream));
  REMAT_DIAG(*m_pStream << "\n");

  if (isa<CastInst>(&I) || isa<GetElementPtrInst>(&I) || isa<BinaryOperator>(&I) || isa<CmpInst>(&I) ||
      isa<SelectInst>(&I) || isa<ExtractElementInst>(&I)) {
    REMAT_DIAG(*m_pStream << "true: [one of: castinst, gep, binaryoperator, cmpinst, selectinst, eei]\n");
    return true;
  }

  if (auto *PHI = dyn_cast<PHINode>(&I)) {

    if (!LI) {
      REMAT_DIAG(*m_pStream << "false: [PHI join node. no loop info, assuming conditional PHI, rejecting]\n");
      REMAT_DIAG_SET_REASON(RejectionReason::COND_PHI);
      return false;
    }

    if (PHI->getNumOperands() == 1)
      return true;

    // Now we know we have two or more operands.
    const BasicBlock *BB = PHI->getParent();
    Loop *LP = LI ? LI->getLoopFor(BB) : nullptr;

    /// Distinguish the case:  1) where PHI is enclosed in a loop, 2) PHI is not enclosed in a loop
    if (LP) {
      /// 1) PHI is enclosed in a loop
      if (!LP->getLoopLatch()) {
        REMAT_DIAG(*m_pStream
                   << "false: [PHI join node. in a loop, but not latch info, assuming conditional PHI, rejecting]\n");
        REMAT_DIAG_SET_REASON(RejectionReason::COND_PHI);
        return false; // But we can't get a latch, so cannot further analyze. Be conservative then.
      }

      /// PHI node is in a loop, but need to distinguish two important cases:
      ///
      /// 1.1) PHI has a loop dependency (at least one of the arguments is loop carried)
      ///
      /// BB.pre.header :
      /// v_init <- ...
      /// BB.loop.header :
      /// dest = PHI (BB.pre.header:v_init, BB.latch: v_loop_defined)
      /// ...
      /// v_loop_defined <- ...
      /// ...
      /// BB.latch:
      ///   goto BB.loop.header
      /// loop END
      ///
      ///
      /// need to check whether it is loop carried:
      if (PHI->getIncomingBlock(0) == LP->getLoopLatch() || PHI->getIncomingBlock(1) == LP->getLoopLatch()) {
        REMAT_DIAG(*m_pStream << "false: [loop header PHI, defining loop carried dependency]\n");
        REMAT_DIAG_SET_REASON(RejectionReason::LOOP_PHI);
        return false;
      } else {
        /// 1.2) PHI is enclosed in a loop, but it's source values are fully defined within a single loop
        /// iteration (i.e. no loop carried definitions)
        /// One potential scenario is the following (BB.if.header, BB.then and BB.else fully enclosed in a loop).
        /// BB.pre.header :
        /// ...
        /// BB.loop.header :
        /// ...
        /// BB.if.header:
        /// BB.then:
        /// v1 <- ...
        /// BB.else:
        /// v2 <- ...
        /// endif
        ///
        /// dest = PHI (BB.then:v1, BB.else: v2)
        /// ...
        /// ...
        /// BB.latch:
        ///   goto BB.loop.header
        /// loop END
        ///
        REMAT_DIAG(*m_pStream << "false: [PHI join node. acyclical control-flow within loop, rejecting]\n");
        REMAT_DIAG_SET_REASON(RejectionReason::COND_PHI);
        return false; // no dominator tree, so no analysis performed, assume false
      }

    } else {
      /// Case 2) : PHI is not enclosed in a loop. We are outside a loop. PHI sources form control flow DAG.
      REMAT_DIAG(*m_pStream << "false: [PHI join node. acyclical control-flow, rejecting]\n");
      REMAT_DIAG_SET_REASON(RejectionReason::COND_PHI);
      return false; // we are not in a loop
    }
  } // end processing PHI node

  if (auto *LI = dyn_cast<LoadInst>(&I)) {
    bool Satisfies = LI->getPointerAddressSpace() == ADDRESS_SPACE_CONSTANT;

    REMAT_DIAG(*m_pStream << (Satisfies ? "true: [LOAD with constant address space]\n"
                                        : "false: [LOAD address space not satisfying]\n"));

    REMAT_DIAG_SET_REASON(Satisfies ? RejectionReason::ACCEPTED : RejectionReason::LOAD);
    return Satisfies;
  }

  if (auto *GII = dyn_cast<GenIntrinsicInst>(&I)) {
    switch (GII->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_RuntimeValue:
    case GenISAIntrinsic::GenISA_GlobalRootSignatureValue:
    case GenISAIntrinsic::GenISA_GlobalBufferPointer:
    case GenISAIntrinsic::GenISA_DispatchRayIndex:
    case GenISAIntrinsic::GenISA_DispatchDimensions:
    case GenISAIntrinsic::GenISA_frc:
    case GenISAIntrinsic::GenISA_ROUNDNE:
      REMAT_DIAG(*m_pStream << "true: [one of: accepted GenISA_* intrinsics]\n");
      return true;
    case GenISAIntrinsic::GenISA_ldraw_indexed:
    case GenISAIntrinsic::GenISA_ldrawvector_indexed:
      REMAT_DIAG(*m_pStream << (isReadOnly(cast<LdRawIntrinsic>(GII)->getResourceValue())
                                    ? "true: [ldraw with read-only buffer]\n"
                                    : "false: [ldraw not read-only]\n"));
      REMAT_DIAG_SET_REASON(isReadOnly(cast<LdRawIntrinsic>(GII)->getResourceValue()) ? RejectionReason::ACCEPTED
                                                                                      : RejectionReason::LDRAW);
      return isReadOnly(cast<LdRawIntrinsic>(GII)->getResourceValue());
    case GenISAIntrinsic::GenISA_ldptr:
    case GenISAIntrinsic::GenISA_ldlptr:
      REMAT_DIAG(*m_pStream << "true: [ldptr or ldlptr]\n");
      return true;
    default:
      REMAT_DIAG(*m_pStream << "false: [non-supported GenISA intrinsic - which?]\n");
      REMAT_DIAG_SET_REASON(RejectionReason::GEN_INTRINSIC);
      return false;
    }
  }

  if (auto *II = dyn_cast<IntrinsicInst>(&I)) {
    switch (II->getIntrinsicID()) {
    case Intrinsic::floor:
    case Intrinsic::minnum:
    case Intrinsic::maxnum:
    case Intrinsic::fabs:
    case Intrinsic::sqrt:
    case Intrinsic::pow:
      return true;
    default:
      REMAT_DIAG_SET_REASON(RejectionReason::LLVM_INTRINSIC);
      REMAT_DIAG(*m_pStream << "false: [non-satisfying LLVM intrinsic]\n");
      return false;
    }
  }

  if (auto *II = dyn_cast<AllocaInst>(&I)) {
    REMAT_DIAG(*m_pStream << "false: [alloca]\n");
    REMAT_DIAG_SET_REASON(RejectionReason::ALLOCA);
    return false;
  }

  REMAT_DIAG(*m_pStream << "false: [non-supported case: missed opportunity?]\n");
  REMAT_DIAG_SET_REASON(RejectionReason::OTHER);
  return false;
}

bool RematChecker::isFreeOperand(const Value *Op) const {
  if (isa<Constant>(Op))
    return true;

  auto *Arg = dyn_cast<Argument>(Op);
  if (!Arg)
    return false;

  if (Stage != RematStage::MID)
    return false;

  auto &F = *Arg->getParent();
  return ArgQuery{F, Ctx}.getPayloadArg(&F) == Arg;
}

bool RematChecker::canFullyRemat(Instruction *I, std::vector<Instruction *> &Insts,
                                 std::unordered_set<Instruction *> &Visited, unsigned StartDepth, unsigned Depth,
                                 ValueToValueMapTy *VM) const {
  REMAT_DIAG(*m_pStream << "\n" << std::string(StartDepth - Depth, ' ') << "canFullyRemat: ");
  REMAT_DIAG(I->print(*m_pStream));
  REMAT_DIAG(*m_pStream << "\n"
                        << std::string(StartDepth - Depth, ' ') << "|sd: " << StartDepth << ", depth: " << Depth
                        << "|  ");

  if (!Visited.insert(I).second)
    return true;

  if (StartDepth != Depth && VM && VM->count(I) != 0)
    return true;

  if (Depth == 0 || !materializable(*I)) {
    REMAT_DIAG(*m_pStream << "\n"
                          << std::string(StartDepth - Depth, ' ')
                          << (Depth == 0 ? "Depth exhausted." : "materializable false"));
    if (Depth == 0) {
      REMAT_DIAG_SET_REASON(RejectionReason::EXHAUSTED);
    }
    return false;
  }

  for (auto &Op : I->operands()) {
    if (isFreeOperand(Op))
      continue;

    auto *OpI = dyn_cast<Instruction>(Op);
    if (!OpI)
      return false;

    if (!canFullyRemat(OpI, Insts, Visited, StartDepth, Depth - 1, VM))
      return false;
  }

  Insts.push_back(I);
  return true;
}

std::optional<std::vector<Instruction *>> RematChecker::canFullyRemat(Instruction *I, uint32_t Threshold,
                                                                      ValueToValueMapTy *VM) const {
  std::vector<Instruction *> Insts;
  std::unordered_set<Instruction *> Visited;
#if defined(_DEBUG) || defined(_INTERNAL)
  m_RootInstruction = I;
#endif
  if (!canFullyRemat(I, Insts, Visited, Threshold, Threshold, VM))
    return std::nullopt;

  return Insts;
}

} // namespace IGC
