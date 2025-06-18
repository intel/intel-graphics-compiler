/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvmWrapper/Support/Alignment.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/Optimizer/PromoteToPredicatedMemoryAccess.hpp"

#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/Legalizer/TypeLegalizer.h"
#include "Probe/Assertion.h"

#define DEBUG_TYPE "PromoteToPredicatedMemoryAccess"

using namespace llvm;
using namespace IGC;
using namespace IGC::Legalizer;

static cl::opt<int> PredicatedMemOpIfConvertMaxInstrs(
    "igc-predmem-ifconv-max-instrs", cl::init(5), cl::Hidden,
    cl::desc("Max number of instructions in a block to consider replacing memory access with predicated memory access and if-converting"));

#define PASS_FLAG "igc-promote-to-predicated-memory-access"
#define PASS_DESCRIPTION "Replace memory access with predicated memory access and if-convert"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PromoteToPredicatedMemoryAccess, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(PromoteToPredicatedMemoryAccess, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC {
    char PromoteToPredicatedMemoryAccess::ID = 0;

    PromoteToPredicatedMemoryAccess::PromoteToPredicatedMemoryAccess() : FunctionPass(ID)
    {
        initializePromoteToPredicatedMemoryAccessPass(*PassRegistry::getPassRegistry());
    }

bool PromoteToPredicatedMemoryAccess::runOnFunction(Function &F) {
  CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  bool isStatlessToBindlessEnabled = (pCtx->type == ShaderType::OPENCL_SHADER &&
                                      static_cast<OpenCLProgramContext*>(pCtx)->m_InternalOptions.PromoteStatelessToBindless);
  if (!pCtx->platform.hasLSC() ||
      !pCtx->platform.LSCEnabled() ||
      isStatlessToBindlessEnabled ||
      pCtx->useStatelessToStateful()) {
    LLVM_DEBUG(dbgs() << "Skip promotion to predicated memory operations because one of conditions is false:\n"
                      << " - Platform has LSC: " << pCtx->platform.hasLSC() << "\n"
                      << " - LSC is enabled: " << pCtx->platform.LSCEnabled() << "\n"
                      << " - PromoteStatelessToBindless is disabled: " << !isStatlessToBindlessEnabled << "\n"
                      << " - useStatelessToStateful is disabled: " << !pCtx->useStatelessToStateful() << "\n");
    return false;
  }

  SmallVector<std::pair<BranchInst *, bool>, 8> WorkList;

  for (auto &BB : F) {
    auto *BI = dyn_cast<BranchInst>(BB.getTerminator());
    if (!BI || !BI->isConditional())
      continue;

    auto *Cond = BI->getCondition();
    auto *TrueBB = BI->getSuccessor(0);
    auto *FalseBB = BI->getSuccessor(1);

    if (trySingleBlockIfConv(*Cond, BB, *TrueBB, *FalseBB))
      WorkList.emplace_back(BI, false);
    else if (trySingleBlockIfConv(*Cond, BB, *FalseBB, *TrueBB, true))
      WorkList.emplace_back(BI, true);
  }

  for (auto &[BI, Inverse] : WorkList) {
    auto *TrueBB = BI->getSuccessor(0);
    auto *FalseBB = BI->getSuccessor(1);

    auto *SplitBB = BI->getParent();
    auto *TargetBB = Inverse ? FalseBB : TrueBB;
    auto *JoinBB = Inverse ? TrueBB : FalseBB;

    IRBuilder<> Builder(BI);

    // Replace the branch with an unconditional one
    Builder.CreateBr(TargetBB);
    BI->eraseFromParent();

    // Remove the phi nodes int the target block
    for (auto &Phi : make_early_inc_range(JoinBB->phis()))
      fixPhiNode(Phi, *TargetBB, *SplitBB);
  }

  return !WorkList.empty();
}

void PromoteToPredicatedMemoryAccess::fixPhiNode(PHINode &Phi, BasicBlock &Predecessor,
                                      BasicBlock &CondBlock) {
  auto *IncomingV = Phi.getIncomingValueForBlock(&Predecessor);
  Phi.replaceAllUsesWith(IncomingV);
  Phi.eraseFromParent();
}

namespace {
  bool IsTypeLegal(Type *Ty) {
    if (!Ty->isIntOrIntVectorTy())
      return true;

    unsigned Width = Ty->getScalarSizeInBits();
    return TypeLegalizer::isLegalInteger(Width) || Width == 1;
  }
} // namespace

bool PromoteToPredicatedMemoryAccess::trySingleBlockIfConv(Value &Cond, BasicBlock &BranchBB,
                                                BasicBlock &ConvBB, BasicBlock &SuccBB,
                                                bool Inverse) {
  if (!ConvBB.hasNPredecessors(1))
    return false;

  if (!SuccBB.hasNPredecessors(2))
    return false;

  if (ConvBB.getSingleSuccessor() != &SuccBB)
    return false;

  const auto NumInstrs = count_if(ConvBB, [](const Instruction &I) {
    return !I.isTerminator() && !isa<CastInst>(&I) && !isa<PHINode>(&I);
  });
  if (NumInstrs > PredicatedMemOpIfConvertMaxInstrs) {
    LLVM_DEBUG(dbgs() << "Skip block, number of instructions " << NumInstrs
                      << " is more than threshold " << PredicatedMemOpIfConvertMaxInstrs << "\n"
                      << "For ConvBB: " << ConvBB << "\n");
    return false;
  }

  SmallDenseMap<Instruction *, Value *> Insts;

  // Collect all the instructions that are used outside the candidate block
  for (auto &Phi : SuccBB.phis()) {
    int Idx = Phi.getBasicBlockIndex(&ConvBB);
    if (Idx == -1)
      continue;

    auto *Inst = dyn_cast<Instruction>(Phi.getIncomingValue(Idx));
    if (!Inst)
      return false;
    if (auto *LI = dyn_cast<LoadInst>(Inst); !LI || !LI->isSimple())
      return false;

    if (!IsTypeLegal(Inst->getType()))
      return false;

    Insts[Inst] = Phi.getIncomingValueForBlock(&BranchBB);
  }

  // Collect the rest of the instructions
  for (auto &I : ConvBB) {
    // Check if this load is handled in the previous loop
    if (isa<LoadInst>(&I) && Insts.count(&I))
        continue;

    // Store
    if(auto *SI = dyn_cast<StoreInst>(&I)) {
        if (!SI->isSimple() || !IsTypeLegal(SI->getValueOperand()->getType()))
            return false;
        Insts[&I] = nullptr;
        continue;
    }

    if (I.mayHaveSideEffects() || I.mayReadFromMemory())
      return false;

    if (CallInst *CI = dyn_cast<CallInst>(&I); CI && CI->isConvergent())
        return false;
  }

  LLVM_DEBUG(dbgs() << "Found if-convertible block:\n"
                    << "  Branch: " << *BranchBB.getTerminator() << "\n"
                    << "  ConvBB: " << ConvBB << "\n"
                    << "  SuccBB: " << SuccBB << "\n");

  for (auto& [Inst, MergeV] : Insts) {
    LLVM_DEBUG(dbgs() << "Converting instruction: " << *Inst << "\n");
    convertMemoryAccesses(Inst, MergeV, &Cond, Inverse);
  }

  return !Insts.empty();
}

void PromoteToPredicatedMemoryAccess::convertMemoryAccesses(Instruction *Mem, Value *MergeV,
                                                            Value *Cond, bool Inverse) {
  LLVM_DEBUG(dbgs() << "Converting memory access: " << *Mem << "\n");
  bool IsLoad = isa<LoadInst>(Mem);
  IGC_ASSERT(IsLoad || isa<StoreInst>(Mem));
  IGC_ASSERT(!IsLoad || MergeV);

  IRBuilder<> Builder(Mem);
  Module *Mod = Mem->getParent()->getModule();
  LLVMContext& Ctx = Mem->getContext();
  Type *Int64Ty = Type::getInt64Ty(Ctx);

  if (Inverse) {
    Cond = Builder.CreateNot(Cond);
    LLVM_DEBUG(dbgs() << "Inverting predicate: " << *Cond << "\n");
  }

  if (IsLoad) {
    LoadInst* Load = cast<LoadInst>(Mem);
    Type *MemType = Mem->getType();
    Value *Ptr = Load->getPointerOperand();
    GenISAIntrinsic::ID iid = GenISAIntrinsic::GenISA_PredicatedLoad;
    Type* ITys[3] = { MemType, Ptr->getType(), MemType };
    Function *predLoadFunc = GenISAIntrinsic::getDeclaration(Mod, iid, ITys);

    Value *AlignV = ConstantInt::get(Int64Ty, IGCLLVM::getAlignmentValue(Load));
    Value* Args[4] = { Ptr, AlignV, Cond, MergeV };
    Instruction *PredLoad = Builder.CreateCall(predLoadFunc, Args);
    PredLoad->takeName(Mem);
    PredLoad->setDebugLoc(Mem->getDebugLoc());

    if (MDNode* lscMetadata = Mem->getMetadata("lsc.cache.ctrl"))
        PredLoad->setMetadata("lsc.cache.ctrl", lscMetadata);

    Mem->replaceAllUsesWith(PredLoad);
    LLVM_DEBUG(dbgs() << "Converted memory access: " << *PredLoad << "\n");
  } else {
    StoreInst* Store = cast<StoreInst>(Mem);
    Value *ValToStore = Store->getValueOperand();
    Value *Ptr = Store->getPointerOperand();

    GenISAIntrinsic::ID iid = GenISAIntrinsic::GenISA_PredicatedStore;
    Type* ITys[2] = { Ptr->getType(), ValToStore->getType() };
    Function *predStoreFunc = GenISAIntrinsic::getDeclaration(Mod, iid, ITys);

    Value *AlignV = ConstantInt::get(Int64Ty, IGCLLVM::getAlignmentValue(Store));
    Value* Args[4] = { Ptr, ValToStore, AlignV, Cond };
    Instruction *PredStore = Builder.CreateCall(predStoreFunc, Args);
    PredStore->setDebugLoc(Mem->getDebugLoc());
    LLVM_DEBUG(dbgs() << "Converted memory access: " << *PredStore << "\n");
  }

  Mem->eraseFromParent();
}

} // namespace IGC
