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

static cl::opt<int>
    PredicatedMemOpIfConvertMaxInstrs("igc-predmem-ifconv-max-instrs", cl::init(5), cl::Hidden,
                                      cl::desc("Max number of instructions in a block to consider replacing memory "
                                               "access with predicated memory access and if-converting"));

#define PASS_FLAG "igc-promote-to-predicated-memory-access"
#define PASS_DESCRIPTION "Replace memory access with predicated memory access and if-convert"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PromoteToPredicatedMemoryAccess, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(PromoteToPredicatedMemoryAccess, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC {
char PromoteToPredicatedMemoryAccess::ID = 0;

PromoteToPredicatedMemoryAccess::PromoteToPredicatedMemoryAccess() : FunctionPass(ID) {
  initializePromoteToPredicatedMemoryAccessPass(*PassRegistry::getPassRegistry());
}

bool PromoteToPredicatedMemoryAccess::runOnFunction(Function &F) {
  pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  bool isStatlessToBindlessEnabled =
      (pCtx->type == ShaderType::OPENCL_SHADER &&
       static_cast<OpenCLProgramContext *>(pCtx)->m_InternalOptions.PromoteStatelessToBindless);
  if (!pCtx->platform.hasLSC() || !pCtx->platform.LSCEnabled() || isStatlessToBindlessEnabled ||
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
    auto *TargetBB = Inverse ? FalseBB : TrueBB;
    auto *JoinBB = Inverse ? TrueBB : FalseBB;

    IRBuilder<> Builder(BI);

    // Replace the branch with an unconditional one
    Builder.CreateBr(TargetBB);
    BI->eraseFromParent();

    // Remove the phi nodes in the target block
    for (auto &Phi : make_early_inc_range(JoinBB->phis()))
      fixPhiNode(Phi, *TargetBB);
  }

  return !WorkList.empty();
}

void PromoteToPredicatedMemoryAccess::fixPhiNode(PHINode &Phi, BasicBlock &Predecessor) {
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

bool IsLoadOkToConv(LoadInst *LI) { return LI->isSimple() && IsTypeLegal(LI->getType()); }
} // namespace

bool PromoteToPredicatedMemoryAccess::trySingleBlockIfConv(Value &Cond, BasicBlock &BranchBB, BasicBlock &ConvBB,
                                                           BasicBlock &SuccBB, bool Inverse) {
  if (!ConvBB.hasNPredecessors(1))
    return false;

  if (!SuccBB.hasNPredecessors(2))
    return false;

  if (ConvBB.getSingleSuccessor() != &SuccBB)
    return false;

  const auto NumInstrs = count_if(ConvBB, [](const Instruction &I) {
    return !I.isTerminator() && !isa<CastInst>(&I) && !isa<PHINode>(&I) && !isa<ExtractElementInst>(&I) &&
           !isa<DbgInfoIntrinsic>(&I);
  });

  if (NumInstrs > PredicatedMemOpIfConvertMaxInstrs) {
    LLVM_DEBUG(dbgs() << "Skip block, number of instructions " << NumInstrs << " is more than threshold "
                      << PredicatedMemOpIfConvertMaxInstrs << "\n"
                      << "For ConvBB: " << ConvBB << "\n");
    return false;
  }

  // Map with a `key` that is a memory instruction that should be converted to
  // predicated and for loads `value` is a merge value that should be used for
  // stores `value` is always nullptr
  SmallDenseMap<Instruction *, Value *> SimpleInsts;

  // Map with a `key` that is a load instruction that should be converted to
  // predicated. The difference from SimpleInsts is loaded value is a vector
  // that is scalarized and scalar values are used in PHI in the next basic
  // block, not direct result of load, so it should be handled differently Map
  // `value` is a vector of Values, that should be used to compose merge value
  // for predicated load
  SmallDenseMap<Instruction *, SmallVector<Value *, 4>> ScalarizedInsts;

  // Collect all the instructions that are used outside the candidate block
  for (auto &Phi : SuccBB.phis()) {
    int Idx = Phi.getBasicBlockIndex(&ConvBB);
    if (Idx == -1)
      continue;

    auto *Inst = dyn_cast<Instruction>(Phi.getIncomingValue(Idx));
    if (!Inst)
      return false;

    if (auto *LI = dyn_cast<LoadInst>(Inst)) {
      if (!IsLoadOkToConv(LI))
        return false;

      SimpleInsts[Inst] = Phi.getIncomingValueForBlock(&BranchBB);
      continue;
    }

    // Support scalarized case:
    // ConvBB:
    //   %17 = load <4 x float>, ptr addrspace(1) %16
    //   %.scalar = extractelement <4 x float> %17, i64 0
    //   %.scalar15 = extractelement <4 x float> %17, i64 1
    //   %.scalar16 = extractelement <4 x float> %17, i64 2
    //   %.scalar17 = extractelement <4 x float> %17, i64 3
    //   br label %SuccBB
    // SuccBB:
    //   %bc311 = phi float [ %.scalar, %ConvBB ], [ 0.000000e+00, %BranchBB ]
    //   %bc312 = phi float [ %.scalar15, %ConvBB ], [ 0.000000e+00, %BranchBB ]
    //   %bc313 = phi float [ %.scalar16, %ConvBB ], [ 0.000000e+00, %BranchBB ]
    //   %bc314 = phi float [ %.scalar17, %ConvBB ], [ 0.000000e+00, %BranchBB ]
    if (auto *EE = dyn_cast<ExtractElementInst>(Inst)) {
      if (!isa<ConstantInt>(EE->getIndexOperand()))
        return false;

      auto *Load = dyn_cast<LoadInst>(EE->getVectorOperand());
      if (!Load || !IsLoadOkToConv(Load))
        return false;
      if (Load->getParent() != &ConvBB)
        return false;

      // Compose merge value for load
      auto &MergeVector = ScalarizedInsts[Load];
      if (MergeVector.empty()) {
        FixedVectorType *VecTy = cast<FixedVectorType>(Load->getType());
        MergeVector.resize(VecTy->getNumElements(), Constant::getNullValue(EE->getType()));
      }
      unsigned Idx = cast<ConstantInt>(EE->getIndexOperand())->getZExtValue();
      if (Idx >= MergeVector.size()) {
        std::string msg =
            "Index " + std::to_string(Idx) + " is >= vector size " + std::to_string(MergeVector.size());
        pCtx->EmitWarning(msg.c_str(), EE);
        LLVM_DEBUG(dbgs() << "Skip block. " << msg << "\n"
                          << "For ConvBB: " << ConvBB << "\n");
        return false;
      }
      MergeVector[Idx] = Phi.getIncomingValueForBlock(&BranchBB);
      continue;
    }

    // Unsupported instruction coming from Phi
    return false;
  }

  // Collect the rest of the instructions
  for (auto &I : ConvBB) {
    // Check if this load is handled in the previous loop
    if (isa<LoadInst>(&I) && (SimpleInsts.count(&I) || ScalarizedInsts.count(&I)))
      continue;

    // Store
    if (auto *SI = dyn_cast<StoreInst>(&I)) {
      if (!SI->isSimple() || !IsTypeLegal(SI->getValueOperand()->getType()))
        return false;
      SimpleInsts[&I] = nullptr;
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

  for (auto &[Inst, MergeV] : SimpleInsts) {
    LLVM_DEBUG(dbgs() << "Converting instruction: " << *Inst << "\n");
    convertMemoryAccesses(Inst, MergeV, &Cond, Inverse);
  }

  for (auto &[Inst, MergeV] : ScalarizedInsts) {
    LLVM_DEBUG(dbgs() << "Converting scalarized instruction: " << *Inst << "\n");
    Value *MergeVVec = PoisonValue::get(Inst->getType());
    IRBuilder<> Builder(Inst);
    for (unsigned i = 0; i < MergeV.size(); ++i)
      MergeVVec = Builder.CreateInsertElement(MergeVVec, MergeV[i], i);
    convertMemoryAccesses(Inst, MergeVVec, &Cond, Inverse);
  }

  return !SimpleInsts.empty() || !ScalarizedInsts.empty();
}

void PromoteToPredicatedMemoryAccess::convertMemoryAccesses(Instruction *Mem, Value *MergeV, Value *Cond,
                                                            bool Inverse) {
  LLVM_DEBUG(dbgs() << "Converting memory access: " << *Mem << "\n");
  bool IsLoad = isa<LoadInst>(Mem);
  IGC_ASSERT(IsLoad || isa<StoreInst>(Mem));
  IGC_ASSERT(!IsLoad || MergeV);

  IRBuilder<> Builder(Mem);
  Module *Mod = Mem->getParent()->getModule();
  LLVMContext &Ctx = Mem->getContext();
  Type *Int64Ty = Type::getInt64Ty(Ctx);

  if (Inverse) {
    Cond = Builder.CreateNot(Cond);
    LLVM_DEBUG(dbgs() << "Inverting predicate: " << *Cond << "\n");
  }

  if (IsLoad) {
    LoadInst *Load = cast<LoadInst>(Mem);
    Type *MemType = Mem->getType();
    Value *Ptr = Load->getPointerOperand();
    GenISAIntrinsic::ID iid = GenISAIntrinsic::GenISA_PredicatedLoad;
    Type *ITys[3] = {MemType, Ptr->getType(), MemType};
    Function *predLoadFunc = GenISAIntrinsic::getDeclaration(Mod, iid, ITys);

    Value *AlignV = ConstantInt::get(Int64Ty, IGCLLVM::getAlignmentValue(Load));
    Value *Args[4] = {Ptr, AlignV, Cond, MergeV};
    Instruction *PredLoad = Builder.CreateCall(predLoadFunc, Args);
    PredLoad->takeName(Mem);
    PredLoad->setDebugLoc(Mem->getDebugLoc());

    if (MDNode *lscMetadata = Mem->getMetadata("lsc.cache.ctrl"))
      PredLoad->setMetadata("lsc.cache.ctrl", lscMetadata);

    Mem->replaceAllUsesWith(PredLoad);
    LLVM_DEBUG(dbgs() << "Converted memory access: " << *PredLoad << "\n");
  } else {
    StoreInst *Store = cast<StoreInst>(Mem);
    Value *ValToStore = Store->getValueOperand();
    Value *Ptr = Store->getPointerOperand();

    GenISAIntrinsic::ID iid = GenISAIntrinsic::GenISA_PredicatedStore;
    Type *ITys[2] = {Ptr->getType(), ValToStore->getType()};
    Function *predStoreFunc = GenISAIntrinsic::getDeclaration(Mod, iid, ITys);

    Value *AlignV = ConstantInt::get(Int64Ty, IGCLLVM::getAlignmentValue(Store));
    Value *Args[4] = {Ptr, ValToStore, AlignV, Cond};
    Instruction *PredStore = Builder.CreateCall(predStoreFunc, Args);
    PredStore->setDebugLoc(Mem->getDebugLoc());
    LLVM_DEBUG(dbgs() << "Converted memory access: " << *PredStore << "\n");
  }

  Mem->eraseFromParent();
}

} // namespace IGC
