/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/helper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/IR/Function.h>
#include "llvm/IR/Verifier.h"
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/BasicBlock.h>

#include "GenerateBlockMemOpsPass.hpp"

using namespace llvm;
using namespace IGC;

char GenerateBlockMemOpsPass::ID = 0;

#define PASS_FLAG "generate-block-mem-ops"
#define PASS_DESCRIPTION "Generation of block load / block stores instead of regular load / stores."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GenerateBlockMemOpsPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(GenerateBlockMemOpsPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

const size_t MaxSgSize = 32;

GenerateBlockMemOpsPass::GenerateBlockMemOpsPass() : FunctionPass(ID) {
  initializeGenerateBlockMemOpsPassPass(*PassRegistry::getPassRegistry());
}

bool GenerateBlockMemOpsPass::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  bool Changed = false;
  // Load / store instructions which are not in code divergence and can be optimized.
  SmallVector<Instruction *, 32> LoadStoreToProcess;
  // Load / store instructions which are inside the loop and can be optimized.
  DenseMap<Loop *, SmallVector<Instruction *, 32>> LoadStoreInLoop;

  MdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
  WI = &getAnalysis<WIAnalysis>();

  IGCMD::FunctionInfoMetaDataHandle Info = MdUtils->getFunctionsInfoItem(&F);
  if (Info->getType() != FunctionTypeMD::KernelFunction)
    return false;

  // If the subgroup size is not specified, then the maximum subgroup size is used.
  IGC::IGCMD::FunctionInfoMetaDataHandle FuncInfoMD = MdUtils->getFunctionsInfoItem(&F);
  IGC::IGCMD::SubGroupSizeMetaDataHandle SubGroupSize = FuncInfoMD->getSubGroupSize();
  if (SubGroupSize->hasValue()) {
    SimdSize = SubGroupSize->getSIMDSize();
  } else {
    SimdSize = MaxSgSize;
  }

  // Check that workgroups have been scalarized along the x-axis.
  if (!checkVectorizationAlongX(&F))
    return false;

  // Collect all load / store instructions which can be replaced.
  for (auto &B : F) {
    for (auto &I : B) {
      if (!isa<LoadInst>(&I) && !isa<StoreInst>(&I))
        continue;

      if (!canOptLoadStore(&I))
        continue;

      // Block read and write instructions must be called by all elements in the subgroup.
      if (!WI->insideDivergentCF(&I)) {
        LoadStoreToProcess.push_back(&I);
      } else if (Loop *L = LI->getLoopFor(I.getParent())) {
        // In some cases IGC can't proof that there is no code divergence in the loop.
        // Handle these cases here.
        // Check that the loop has been already analyzed.
        if (LoadStoreInLoop.find(L) == LoadStoreInLoop.end()) {
          if (!isLoopPattern(L))
            continue;

          SmallVector<Instruction *, 32> Vec;
          Vec.push_back(&I);
          LoadStoreInLoop.insert(std::make_pair(L, Vec));
        } else {
          LoadStoreInLoop[L].push_back(&I);
        }
      }
    }
  }

  // Optimize cases without loops.
  for (auto I : LoadStoreToProcess)
    Changed |= changeToBlockInst(I);

  // Optimize cases with loops. Split loop into a remainder calculation and a new uniform loop.
  // The remainder contains code divergence.
  // The new loop will contain the main part of the loop without code divergence.
  //
  // For example:
  //
  // for (int idx = global_id_x + offset; idx < N; idx += simdsize) {
  //    A[idx] = B[idx];
  // }
  //
  // will be split into:
  //
  // if (global_id_x + offset < N - (N - offset) / simdsize * simdsize) {
  //    A[idx] = B[idx];
  // }
  //
  // for (int idx = global_id_x + offset + N - (N - offset) / simdsize * simdsize - offset; idx < N; idx += simdsize) {
  //   auto x = sg.load(&B[idx]);
  //   sg.store(&A[idx], x);
  // }
  //
  for (const auto &Pair : LoadStoreInLoop) {
    Loop *L = Pair.first;
    BasicBlock *OldLatch = L->getLoopLatch();
    BasicBlock *OldPreheader = L->getLoopPreheader();
    PHINode *OldInductionPHI = L->getInductionVariable(*SE);
    ICmpInst *OldLatchCmp = cast<ICmpInst>(cast<BranchInst>(OldLatch->getTerminator())->getCondition());
    Value *OldLimit = OldLatchCmp->getOperand(1);
    Value *OldIncomingIndV = OldInductionPHI->getIncomingValueForBlock(OldPreheader);

    SmallVector<BasicBlock *, 1> ExitBlocks;
    L->getExitBlocks(ExitBlocks);
    BasicBlock *Exit = ExitBlocks[0];

    // Get BranchInst which defines the condition for entering the loop.
    BranchInst *PreConditionBranch = cast<BranchInst>(OldPreheader->getTerminator());
    if (!PreConditionBranch->isConditional())
      PreConditionBranch = cast<BranchInst>((*pred_begin(OldPreheader))->getTerminator());
    ICmpInst *PreCondition = cast<ICmpInst>(PreConditionBranch->getCondition());

    // Get offset for the initial value of the induction variable..
    SmallVector<Value *, 2> Offset;
    if (!getOffset(OldIncomingIndV, Offset))
      continue;

    // Create a new basic block which will separate the remainder and the new loop.
    LLVMContext &Context = OldLatch->getContext();
    BasicBlock *SeparatorBasicBlock = BasicBlock::Create(Context, ".separator", &F);
    SeparatorBasicBlock->moveAfter(OldLatch);

    // Clone the loop.
    ValueToValueMapTy VMap;
    BasicBlock *ClonedLatch = CloneBasicBlock(OldLatch, VMap, ".new.loop", &F);
    for (auto &I : *ClonedLatch)
      RemapInstruction(&I, VMap, RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);

    // Clone the pre-condition and pre-condition branch instructions in the separator block.
    ICmpInst *ClonedPreCondition = cast<ICmpInst>(PreCondition->clone());
    BranchInst *ClonedPreConditionBranch = cast<BranchInst>(PreConditionBranch->clone());
    IGCLLVM::pushBackInstruction(SeparatorBasicBlock, ClonedPreCondition);
    IGCLLVM::pushBackInstruction(SeparatorBasicBlock, ClonedPreConditionBranch);

    // Create empty exit for the new loop.
    BasicBlock *ExitForTheNewLoop = BasicBlock::Create(Context, ".new.exit", &F);
    ExitForTheNewLoop->moveAfter(ClonedLatch);
    IRBuilder<> Builder(ExitForTheNewLoop);
    Builder.CreateBr(Exit);
    Changed = true;

    // Create empty preheader for the new loop.
    BasicBlock *PreheaderForTheNewLoop = BasicBlock::Create(Context, ".new.preheader", &F);
    PreheaderForTheNewLoop->moveAfter(SeparatorBasicBlock);
    Builder.SetInsertPoint(PreheaderForTheNewLoop);
    Builder.CreateBr(ClonedLatch);

    // Update the cloned pre-condition branch successors.
    ClonedPreConditionBranch->setCondition(ClonedPreCondition);
    ClonedPreConditionBranch->setSuccessor(0, PreheaderForTheNewLoop);
    ClonedPreConditionBranch->setSuccessor(1, Exit);

    // Update the cloned latch branch successors.
    BranchInst *ClonedLatchBranch = cast<BranchInst>(ClonedLatch->getTerminator());
    ClonedLatchBranch->setSuccessor(0, ClonedLatch);
    ClonedLatchBranch->setSuccessor(1, ExitForTheNewLoop);

    // Insert the cloned latch block after the separator block.
    ClonedLatch->moveAfter(SeparatorBasicBlock);

    // Calculate the new limit for the remainder:
    // newlimit = limit - (limit - offset1 - offset2) / simdsize * simdsize
    //
    // In IR it looks like:
    //
    // %suboffset1 = sub i32 %limit, %offset1
    // %suboffset2 = sub i32 %suboffset1, %offset2
    // %neg_qot = ashr i32 %suboffset2, log2(SimdSize)
    // %qot = sub i32 0, %neg_qot
    // %qotshl = shl i32 %qot, log2(SimdSize)
    // %58 = add nsw i32 %limit, %qotshl
    //
    Type *LimitType = OldLimit->getType();

    auto processOffset = [&](Value *SubArg) {
      for (auto Val : Offset) {
        if (!Val)
          break;

        Value *OffsetVal = Val;
        Type *ValType = Val->getType();
        if (LimitType != ValType)
          OffsetVal = Builder.CreateZExt(Val, LimitType, "casted_offset");

        SubArg = Builder.CreateSub(SubArg, OffsetVal);
      }
      return SubArg;
    };

    // Calculate the new limit (NewLimitFisrtLoop) for the remainder.
    Builder.SetInsertPoint(PreCondition);
    Value *SubOffset = processOffset(OldLimit);

    int LogSimdSizeBase2 = std::log2(SimdSize);
    Value *AshrInst = Builder.CreateAShr(SubOffset, ConstantInt::get(LimitType, LogSimdSizeBase2), "ashr");
    Value *Neg = Builder.CreateSub(ConstantInt::get(LimitType, 0), AshrInst, "neg");
    Value *Shl = Builder.CreateShl(Neg, ConstantInt::get(LimitType, LogSimdSizeBase2));
    Value *NewLimitFisrtLoop = Builder.CreateAdd(OldLimit, Shl);

    // Update cmp instruction in the remainder and preheader with new limit.
    PreCondition->setOperand(1, NewLimitFisrtLoop);
    OldLatchCmp->setOperand(1, NewLimitFisrtLoop);

    // Calculate the induction variable initial value for the the new loop.
    Builder.SetInsertPoint(SeparatorBasicBlock, SeparatorBasicBlock->getFirstInsertionPt());
    Value *OffsetForNewLoop = processOffset(NewLimitFisrtLoop);

    Value *NewIncInductiveVar = Builder.CreateAdd(OldIncomingIndV, OffsetForNewLoop);

    // Set operands for the cloned pre-condition.
    ClonedPreCondition->setOperand(0, NewIncInductiveVar);
    ClonedPreCondition->setOperand(1, OldLimit);

    // Substitude load/store instructions with block ones.
    for (auto I : Pair.second) {
      Instruction *NewI = cast<Instruction>(VMap[cast<Value>(I)]);
      changeToBlockInst(NewI);
    }

    std::vector<PHINode *> PhiNodes;
    // Set operands for phi instructions in the new loop and prepare initial values for the new loop.
    for (auto &I : *OldLatch) {
      if (!isa<PHINode>(&I))
        break;

      Value *IVal = cast<Value>(&I);
      PHINode *Phi = cast<PHINode>(&I);
      PHINode *NewPhi = dyn_cast<PHINode>(VMap[IVal]);
      Value *OldIncomingV = Phi->getIncomingValueForBlock(OldPreheader);
      PhiNodes.push_back(Phi);

      for (unsigned i = 0; i < Phi->getNumIncomingValues(); ++i) {
        if (NewPhi->getIncomingBlock(i) == OldLatch) {
          NewPhi->setIncomingBlock(i, ClonedLatch);
        } else if (NewPhi->getIncomingBlock(i) == OldPreheader) {
          NewPhi->setIncomingBlock(i, PreheaderForTheNewLoop);
        }
      }

      Value *NewInc = nullptr;
      if (GetElementPtrInst *Gep = dyn_cast<GetElementPtrInst>(OldIncomingV)) {
        Type *GEPType = Gep->getResultElementType();
        NewInc = Builder.CreateGEP(GEPType, OldIncomingV, OffsetForNewLoop);
      } else if (Phi == OldInductionPHI) {
        NewInc = NewIncInductiveVar;
      }
      NewPhi->setIncomingValueForBlock(PreheaderForTheNewLoop, NewInc);
    }

    // Erase phi instructions from values (make it if-statement).
    for (auto Phi : PhiNodes) {
      Value *OldIncomingV = Phi->getIncomingValueForBlock(OldPreheader);
      Phi->replaceAllUsesWith(OldIncomingV);
      Phi->eraseFromParent();
    }

    // Erase conditional branch from the old latch and creat unconditional branch.
    BranchInst *OldLatchBranch = cast<BranchInst>(OldLatch->getTerminator());
    Builder.SetInsertPoint(OldLatchBranch);
    Builder.CreateBr(SeparatorBasicBlock);
    OldLatchBranch->eraseFromParent();
    PreConditionBranch->setSuccessor(1, SeparatorBasicBlock);
  }

  return Changed;
}

using namespace llvm::PatternMatch;
bool GenerateBlockMemOpsPass::getOffset(Value *Init, SmallVector<Value *, 2> &Offset) {
  Value *NonUnifOp = Init;
  while (NonUnifOp) {

    if (ZExtInst *ZExt = dyn_cast<ZExtInst>(NonUnifOp)) {
      NonUnifOp = ZExt->getOperand(0);
    } else if (SExtInst *SExt = dyn_cast<SExtInst>(NonUnifOp)) {
      NonUnifOp = SExt->getOperand(0);
    } else if (Instruction *Inst = dyn_cast<Instruction>(NonUnifOp)) {
      if (Inst->getOpcode() != Instruction::Add)
        return false;

      IGC::IGCMD::FunctionInfoMetaDataHandle FuncInfoMD = MdUtils->getFunctionsInfoItem(Inst->getFunction());
      IGC::IGCMD::ThreadGroupSizeMetaDataHandle ThreadGroupSize = FuncInfoMD->getThreadGroupSize();

      // ThreadGroupSize should be specified. It is checked earlier in checkVectorizationAlongX function.
      IGC_ASSERT(ThreadGroupSize->hasValue());
      int LogBase2 = std::log2((int32_t)ThreadGroupSize->getXDim());

      // Check global_id_x pattern
      Value *LocalIdX = nullptr;
      Value *R0 = nullptr;
      auto GlobalIdXPattern =
          m_Add(m_Shl(m_ExtractElt(m_Value(R0), m_SpecificInt(1)), m_SpecificInt(LogBase2)), m_Value(LocalIdX));
      if (match(NonUnifOp, GlobalIdXPattern)) {
        if (ZExtInst *ZExt = dyn_cast<ZExtInst>(LocalIdX))
          LocalIdX = ZExt->getOperand(0);

        if (isLocalIdX(LocalIdX) && isR0(R0))
          return true;
      }

      Value *Op0 = Inst->getOperand(0);
      Value *Op1 = Inst->getOperand(1);
      if (!WI->isUniform(Op1) && !WI->isUniform(Op0))
        return false;

      if (Offset.size() == 2)
        return false;

      if (WI->isUniform(Op0)) {
        Offset.push_back(Op0);
        NonUnifOp = Op1;
      } else {
        Offset.push_back(Op1);
        NonUnifOp = Op0;
      }
    } else {
      return false;
    }
  }

  return false;
}

bool GenerateBlockMemOpsPass::isLoopPattern(Loop *L) {
  // Check that Loop has good shape so it safe to use llvm methods to work with it.
  if (!L || !L->isSafeToClone() || (L->getNumBlocks() != 1) || !L->isLCSSAForm(*DT))
    return false;

  BasicBlock *Header = L->getHeader();
  BasicBlock *Latch = L->getLoopLatch();
  BasicBlock *Preheader = L->getLoopPreheader();
  PHINode *Phi = L->getInductionVariable(*SE);

  // Check that all parts of the loop can be found.
  if (!Phi || !Preheader || !Latch || !Header)
    return false;

  ICmpInst *LatchCmp = dyn_cast<ICmpInst>(cast<BranchInst>(Latch->getTerminator())->getCondition());
  if (!LatchCmp)
    return false;

  if (pred_size(Header) != 2)
    return false;

  // Check that the loop has only one exit block.
  SmallVector<BasicBlock *, 4> ExitBlocks;
  L->getExitBlocks(ExitBlocks);
  if (ExitBlocks.size() != 1)
    return false;

  BasicBlock *Exit = ExitBlocks[0];

  // Check that all values inside the loop have only internal users.
  if (doesLoopHaveExternUse(L))
    return false;

  // Check that the loop has phi instructions of specific type.
  if (!checkLoopPhiVals(L))
    return false;

  // Check that the induction variable is incremented by the simd size.
  Instruction *Inc = dyn_cast<Instruction>(Phi->getIncomingValueForBlock(Latch));
  if (Inc->getOpcode() != Instruction::Add || (Inc->getOperand(0) != Phi && Inc->getOperand(1) != Phi))
    return false;

  ConstantInt *CI = dyn_cast<ConstantInt>(Inc->getOperand(0));
  if (!CI)
    CI = dyn_cast<ConstantInt>(Inc->getOperand(1));
  if (!CI)
    return false;
  if (CI->getValue() != SimdSize)
    return false;

  // Check that the loop condition is ULT or SLT.
  CmpInst::Predicate Pred = LatchCmp->getPredicate();
  if (Pred != ICmpInst::ICMP_ULT && Pred != ICmpInst::ICMP_SLT)
    return false;

  // Loop limit should be uniform.
  Value *Limit = LatchCmp->getOperand(1);
  if (!WI->isUniform(Limit))
    return false;

  // Initial value for induction variable should be continuous.
  Value *InitValForIndVar = Phi->getIncomingValueForBlock(Preheader);
  if (!isIndexContinuous(InitValForIndVar))
    return false;

  // Find a conditional branch that defines if the loop should be executed.
  // It can be placed in the preheader or in its single predecessor.
  // This condition should match the condition in the loop latch.
  BranchInst *PreConditionBranch = cast<BranchInst>(Preheader->getTerminator());
  if (!PreConditionBranch->isConditional()) {
    if (Preheader->size() != 1)
      return false;

    PreConditionBranch = nullptr;

    if (Preheader->hasNPredecessors(1))
      PreConditionBranch = cast<BranchInst>((*pred_begin(Preheader))->getTerminator());
  }

  if (!PreConditionBranch || !PreConditionBranch->isConditional())
    return false;

  ICmpInst *PreCondition = dyn_cast<ICmpInst>(PreConditionBranch->getCondition());
  if (!PreCondition || PreCondition->getPredicate() != Pred || PreCondition->getOperand(1) != Limit)
    return false;

  if ((PreConditionBranch->getSuccessor(0) != Latch) && (PreConditionBranch->getSuccessor(0) != Preheader))
    return false;

  // That PreConditionBranch leads to the loop exit or to its single successor block.
  if (PreConditionBranch->getSuccessor(1) != Exit) {
    if (Exit->size() != 1)
      return false;

    BranchInst *ExitBranch = cast<BranchInst>(Exit->getTerminator());
    if (ExitBranch->isConditional())
      return false;

    if (ExitBranch->getSuccessor(0) != PreConditionBranch->getSuccessor(1))
      return false;
  }

  return true;
}

// Check that incoming values for phi instructions are getelementptr instructions except induction variable.
bool GenerateBlockMemOpsPass::checkLoopPhiVals(Loop *L) {
  BasicBlock *Preheader = L->getLoopPreheader();
  BasicBlock *Latch = L->getLoopLatch();
  PHINode *IndPhi = L->getInductionVariable(*SE);

  for (auto &I : *Latch) {
    PHINode *Phi = dyn_cast<PHINode>(&I);
    if (!Phi)
      break;

    Value *IncomingVal = Phi->getIncomingValueForBlock(Preheader);
    Value *InternalVal = Phi->getIncomingValueForBlock(Latch);

    if (Phi != IndPhi) {
      if (!isa<GetElementPtrInst>(IncomingVal))
        return false;

      if (!isa<GetElementPtrInst>(InternalVal))
        return false;
    }
  }

  return true;
}

// Check that loop has only internal users.
bool GenerateBlockMemOpsPass::doesLoopHaveExternUse(Loop *L) {
  // Expect that loop has only one exit block. It is checked earlier in checkLoopPattern function.
  IGC_ASSERT(L->getNumBlocks() == 1);

  BasicBlock *Latch = L->getLoopLatch();
  for (auto &I : *Latch) {
    for (auto UI = I.use_begin(), UE = I.use_end(); UI != UE; ++UI) {
      Instruction *Inst = dyn_cast<Instruction>(*UI);
      if (!Inst)
        return true;

      if (Inst->getParent() != Latch)
        return true;
    }
  }

  return false;
}

bool GenerateBlockMemOpsPass::isDataTypeSupported(Value *Ptr, Type *DataType) {
  unsigned ScalarSize = DataType->getScalarSizeInBits();

  // The list of possible alignments should be expanded.
  if (CGCtx->platform.isProductChildOf(IGFX_PVC))
    if (ScalarSize == 32 || ScalarSize == 64)
      return true;

  return false;
}

// This function checks if Indx is equal to 1 * LocalIdX + UniformPart, assuming LocalIdY and LocalIdZ are uniform
// values.
bool GenerateBlockMemOpsPass::isIndexContinuous(Value *Indx) {
  SmallVector<Value *, 2> NonUniformInstVector;
  NonUniformInstVector.push_back(Indx);
  PHINode *VisitedPhi = nullptr;

  // Continuity requires that only add and zext operations can be performed on a non-uniform value.
  while (NonUniformInstVector.size()) {
    for (auto It = NonUniformInstVector.begin(); It != NonUniformInstVector.end();) {
      Value *NonUnifOp = *It;

      if (!NonUnifOp)
        return false;

      NonUniformInstVector.erase(It);

      if (ZExtInst *ZExt = dyn_cast<ZExtInst>(NonUnifOp)) {
        NonUniformInstVector.push_back(ZExt->getOperand(0));
      } else if (SExtInst *SExt = dyn_cast<SExtInst>(NonUnifOp)) {
        NonUniformInstVector.push_back(SExt->getOperand(0));
      } else if (PHINode *Phi = dyn_cast<PHINode>(NonUnifOp)) {
        // Check that PHINode has two incoming values and one of them
        // is calculated from local_id_x and another one from this PHINode.
        if (VisitedPhi && VisitedPhi != Phi)
          return false;

        if (VisitedPhi)
          continue;

        unsigned NumIncomingValues = Phi->getNumIncomingValues();

        if (NumIncomingValues != 2)
          return false;

        for (Use &U : Phi->incoming_values()) {
          Value *V = U.get();
          if (WI->isUniform(V))
            return false;

          NonUniformInstVector.push_back(V);
        }
        VisitedPhi = Phi;
      } else if (Instruction *Inst = dyn_cast<Instruction>(NonUnifOp)) {
        if (Inst->getOpcode() != Instruction::Add && Inst->getOpcode() != Instruction::Sub)
          return false;

        Value *Op0 = Inst->getOperand(0);
        Value *Op1 = Inst->getOperand(1);

        if (!WI->isUniform(Op1) && !WI->isUniform(Op0))
          return false;

        if (WI->isUniform(Op0)) {
          if (Inst->getOpcode() == Instruction::Sub)
            return false;

          NonUniformInstVector.push_back(Op1);
        } else {
          NonUniformInstVector.push_back(Op0);
        }
      } else if (!isLocalIdX(NonUnifOp)) {
        // If local_id_x was met then index is continuous.
        return false;
      }
    }
  }

  return true;
}

bool GenerateBlockMemOpsPass::checkVectorizationAlongX(Function *F) {
  if (CGCtx->type != ShaderType::OPENCL_SHADER)
    return false;

  IGC::IGCMD::FunctionInfoMetaDataHandle FuncInfoMD = MdUtils->getFunctionsInfoItem(F);
  ModuleMetaData *ModMD = CGCtx->getModuleMetaData();
  auto FuncMD = ModMD->FuncMD.find(F);

  if (FuncMD == ModMD->FuncMD.end())
    return false;

  WorkGroupWalkOrderMD WorkGroupWalkOrder = FuncMD->second.workGroupWalkOrder;
  if (WorkGroupWalkOrder.dim0 != 0 || WorkGroupWalkOrder.dim1 != 1 || WorkGroupWalkOrder.dim2 != 2)
    return false;

  int32_t X = -1;
  IGC::IGCMD::ThreadGroupSizeMetaDataHandle ThreadGroupSize = FuncInfoMD->getThreadGroupSize();
  if (!ThreadGroupSize->hasValue())
    return false;

  X = (int32_t)ThreadGroupSize->getXDim();
  if (!X)
    return false;

  if (X % SimdSize == 0)
    return true;

  return false;
}

bool GenerateBlockMemOpsPass::canOptLoadStore(Instruction *I) {
  Value *Ptr = nullptr;
  Value *ValOp = nullptr;
  Type *DataType = nullptr;

  if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
    Ptr = LI->getPointerOperand();
    DataType = cast<Value>(LI)->getType();
  } else {
    StoreInst *SI = cast<StoreInst>(I);
    Ptr = SI->getPointerOperand();
    ValOp = SI->getValueOperand();
    DataType = ValOp->getType();
  }

  if (DataType->isVectorTy())
    return false;

  // Need to check what alignment block load/store requires for the specific architecture.
  if (!isDataTypeSupported(Ptr, DataType))
    return false;

  // Get the last index from the getelementptr instruction if it is not uniform in the subgroup.
  Instruction *PtrInstr = dyn_cast<Instruction>(Ptr);
  Value *Idx = checkGep(PtrInstr, DataType);

  if (!Idx)
    return false;

  // Check that memory access is continuous in the subgroup.
  if (!isIndexContinuous(Idx))
    return false;

  return true;
}

bool GenerateBlockMemOpsPass::isLocalIdX(const Value *InputVal) {
  const Argument *A = dyn_cast<Argument>(InputVal);
  if (!A)
    return false;
  Function *F = const_cast<Function *>(A->getParent());
  ImplicitArgs implicitArgs(*F, MdUtils);
  Value *localIdX = implicitArgs.getImplicitArgValue(*F, ImplicitArg::LOCAL_ID_X, MdUtils);

  return A == localIdX;
}

bool GenerateBlockMemOpsPass::isR0(const Value *InputVal) {
  const Argument *A = dyn_cast<Argument>(InputVal);
  if (!A)
    return false;
  Function *F = const_cast<Function *>(A->getParent());
  ImplicitArgs implicitArgs(*F, MdUtils);
  Value *R0 = implicitArgs.getImplicitArgValue(*F, ImplicitArg::R0, MdUtils);

  return A == R0;
}

bool GenerateBlockMemOpsPass::changeToBlockInst(Instruction *I) {
  IRBuilder<> Builder(I);
  Function *BlockOpDecl = nullptr;
  CallInst *BlockOpCall = nullptr;
  alignment_t AlignmentOnInstruction = 0;

  if (isa<LoadInst>(I)) {
    Value *Args[1] = {I->getOperand(0)};
    Type *Types[2] = {I->getType(), I->getOperand(0)->getType()};
    BlockOpDecl = GenISAIntrinsic::getDeclaration(I->getModule(), GenISAIntrinsic::GenISA_simdBlockRead, Types);
    BlockOpCall = Builder.CreateCall(BlockOpDecl, Args);
    AlignmentOnInstruction = IGCLLVM::getAlignmentValue(cast<LoadInst>(I));
  } else {
    Value *Args[2] = {I->getOperand(1), I->getOperand(0)};
    Type *Types[2] = {I->getOperand(1)->getType(), I->getOperand(0)->getType()};
    BlockOpDecl = GenISAIntrinsic::getDeclaration(I->getModule(), GenISAIntrinsic::GenISA_simdBlockWrite, Types);
    BlockOpCall = Builder.CreateCall(BlockOpDecl, Args);
    AlignmentOnInstruction = IGCLLVM::getAlignmentValue(cast<StoreInst>(I));
  }

  if (!BlockOpCall)
    return false;

  setAlignmentAttr(BlockOpCall, AlignmentOnInstruction);
  I->replaceAllUsesWith(BlockOpCall);
  I->eraseFromParent();

  return true;
}

void GenerateBlockMemOpsPass::setAlignmentAttr(CallInst *CI, const unsigned &Alignment) {
  auto CustomAttr = llvm::Attribute::get(CI->getContext(), "alignmentrequirements", std::to_string(Alignment));
  CI->addFnAttr(CustomAttr);
}

Value *GenerateBlockMemOpsPass::checkGep(Instruction *PtrInstr, Type *DataType) {
  if (!PtrInstr)
    return nullptr;

  GetElementPtrInst *Gep = nullptr;
  if (PHINode *Phi = dyn_cast<PHINode>(PtrInstr)) {
    unsigned NumIncomingValues = Phi->getNumIncomingValues();
    if (NumIncomingValues != 2) {
      return nullptr;
    }

    BasicBlock *BB = PtrInstr->getParent();
    // If this is not a loop, we can't be sure of the flow. Better do nothing.
    if (Loop *L = LI->getLoopFor(BB)) {
      BasicBlock *Preheader = L->getLoopPreheader();
      // Ensure the loop preheader is an incoming block to the PHI node before querying it.
      // The PHI provides the index used for a buffer load/store inside the loop, and the compiler
      // needs to analyze this index pattern to determine if it can apply a block load/store optimization.
      // If the preheader is not an incoming block, we cannot extract the initial value of the index,
      // which prevents the compiler from recognizing the access pattern and applying the optimization.
      // Additionally, calling getIncomingValueForBlock on a non-incoming block would crash or assert.
      if (Preheader && Phi->getBasicBlockIndex(Preheader) >= 0) {
        Value *IncomingVal1 = Phi->getIncomingValueForBlock(Preheader);
        Gep = dyn_cast<GetElementPtrInst>(IncomingVal1);
      }
    }
  } else {
    Gep = dyn_cast<GetElementPtrInst>(PtrInstr);
  }

  if (!Gep)
    return nullptr;

  bool IsPtrUniform = false, IsLastIndUniform = false;
  Value *Ptr = Gep->getOperand(0);

  if (WI->isUniform(Ptr))
    IsPtrUniform = true;

  bool TypesMatch = DataType == Gep->getResultElementType();
  Type *Int32Ty = Type::getInt32Ty(*CGCtx->getLLVMContext());
  Value *Zero = Constant::getNullValue(Int32Ty);

  // If `DataType` doesn't match the GEP result type -- then logically there are implicit zero indices at the end.
  // Here it doesn't matter how many zero indices there are.
  // If there's at least one implicit zero -- then we have to check all the indexes and the last index will be zero.
  auto E = TypesMatch ? Gep->idx_end() - 1 : Gep->idx_end();
  Value *LInst = TypesMatch ? *E : Zero;
  // Make sure that all indexes, not including the last one, are uniform.
  // This is important because the address must be continuous in the subgroup.
  for (auto Idx = Gep->idx_begin(); Idx != E; Idx++)
    if (!WI->isUniform(*Idx))
      return nullptr;

  if (WI->isUniform(LInst))
    IsLastIndUniform = true;

  if (!IsLastIndUniform && IsPtrUniform) {
    return LInst;
  } else if (IsLastIndUniform && !IsPtrUniform) {
    if (!isa<PHINode>(Ptr) && !isa<GetElementPtrInst>(Ptr))
      return nullptr;

    if (PHINode *Phi = dyn_cast<PHINode>(Ptr)) {
      if (Phi->getNumIncomingValues() != 2)
        return nullptr;

      for (Use &U : Phi->incoming_values()) {
        Value *V = U.get();

        if (!isa<GetElementPtrInst>(V))
          return nullptr;

        GetElementPtrInst *G = cast<GetElementPtrInst>(V);

        bool IsGepHasPhiArg = false;
        if (G->getOperand(0) == Phi) {
          // Check that the address was incremented using gep instruction and the value incrementation is uniform.
          IsGepHasPhiArg = true;
          for (auto Idx = G->idx_begin(), E = G->idx_end(); Idx != E; Idx++) {
            if (!WI->isUniform(*Idx)) {
              return nullptr;
            }
          }
        } else {
          // Get the incoming address value.
          Ptr = V;
        }

        if (!IsGepHasPhiArg)
          return nullptr;
      }
    }

    return checkGep(dyn_cast<GetElementPtrInst>(Ptr), DataType);
  }

  return nullptr;
}
