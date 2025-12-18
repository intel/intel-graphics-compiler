/*========================== begin_copyright_notice ============================

Copyright (C) 2024-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXPropagateSurfaceState
/// -----------------------------
///
/// This pass transforms 32-bit surface pointer data flow into 64-bit one
/// making possible to enable efficient 64-bit addressing later on.
/// Mainly there are two cases:
/// 1. Array of surface pointers.
///    64-bit surface pointers are truncated to 32-bit ones,
///    array is formed using series of insertelement instructions
///    then rdregioni gets certain element
///    that is used by BTI memory instrinsics:
///
///    The above is transfomed to:
///    64-bit surface pointer array is formed using series of
///    insertelement instructions then rdregioni gets certain element
///    that is truncated to 32-bit
///    that is used by BTI memory instrinsics
///
/// 2. Function call with surface pointer argument(s).
///    64-bit surface pointers are truncated to 32-bit ones,
///    that are used as function arguments
///    then the functions pass 32-bit surface pointers to
///    BTI memory instrinsics:
///
///    The above is transfomed to:
///    Functions that have 32-bit surface pointer arguments are cloned
///    with 32-bit surface pointer arguments replaced with 64-bit one
///    that are truncated to 32-bit and passed to BTI memory instrinsics

#include "GenX.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"

#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/Intrinsics.h"

#include "Probe/Assertion.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueMap.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include "llvmWrapper/IR/Function.h"

#include <queue>

#define DEBUG_TYPE "genx-propagate-surface-state"

using namespace llvm;

namespace {
class GenXPropagateSurfaceState final
    : public ModulePass,
      public InstVisitor<GenXPropagateSurfaceState, Value *> {
public:
  static char ID;

  GenXPropagateSurfaceState() : ModulePass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<TargetPassConfig>();
  }

  StringRef getPassName() const override {
    return "GenX Propagate Surface State Pointer";
  }

  bool runOnModule(Module &M) override;

  Value *visitInstruction(Instruction &I) const;
  Value *visitTruncInst(TruncInst &I);
  Value *visitInsertElementInst(InsertElementInst &I);
  Value *visitCastInst(CastInst &I) const;
  Value *visitCallInst(CallInst &I);
  Value *visitSelectInst(SelectInst &I) const;

private:
  bool Modify = false;
  Function *Func = nullptr;

  // main queue to handle 32-bit surface pointer arguments
  std::queue<Value *> OpQueue;

  // instructions that will be replaced with new ones
  ValueMap<Value *, Value *> Map;

  // to track uniqueness of OpQueue
  SmallPtrSet<Value *, 16> ToHandle;

  // functions with their 32-bit surface pointer arguments
  ValueMap<Function *, SmallSet<unsigned, 6>> Fmap;

  // 64-bit functions that will replace 32-bit ones
  ValueMap<Function *, Function *> MapOldToNew;

  void transferArgumentUses(Function &NewF, Function &OldF);
  Function *cloneFunction(Function &F, const SmallSet<unsigned, 6> &Indices);
  void processKernelFunction(Function &F);
  void processFunction(Function &F);
};
} // namespace

char GenXPropagateSurfaceState::ID = 0;

INITIALIZE_PASS_BEGIN(GenXPropagateSurfaceState, "GenXPropagateSurfaceState",
                      "GenXPropagateSurfaceState", false, false);
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(GenXPropagateSurfaceState, "GenXPropagateSurfaceState",
                    "GenXPropagateSurfaceState", false, false);

namespace llvm {
ModulePass *createGenXPropagateSurfaceStatePass() {
  initializeGenXPropagateSurfaceStatePass(*PassRegistry::getPassRegistry());
  return new GenXPropagateSurfaceState();
}
} // namespace llvm

void GenXPropagateSurfaceState::transferArgumentUses(Function &NewF,
                                                     Function &OldF) {
  auto *InsPt = &*NewF.getEntryBlock().getFirstInsertionPt();
  IRBuilder<> Builder(InsPt);

  IGC_ASSERT(OldF.arg_size() == NewF.arg_size());
  for (auto &&[OldArg, NewArg] : zip(OldF.args(), NewF.args())) {
    Value *NewVal = &NewArg;

    if (OldArg.getType() != NewArg.getType()) {
      LLVM_DEBUG(dbgs() << "Updating argument <" << OldArg << "> to <" << NewArg
                        << ">\n");
      Modify = true;
      // inserting truncate as the first instruction
      NewVal = Builder.CreateTrunc(NewVal, OldArg.getType());

      OpQueue.push(NewVal);
      ToHandle.insert(NewVal);
    }

    OldArg.replaceAllUsesWith(NewVal);
    NewArg.takeName(&OldArg);
  }
}

Function *
GenXPropagateSurfaceState::cloneFunction(Function &F,
                                         const SmallSet<unsigned, 6> &Indices) {

  LLVM_DEBUG(dbgs() << "Cloning " << F.getName() << "\n");

  SmallVector<Type *> NewArgTypes;
  transform(
      F.args(), std::back_inserter(NewArgTypes), [&](auto &Arg) -> Type * {
        auto *Ty = Arg.getType();
        if (Indices.contains(Arg.getArgNo())) {
          auto *I64Ty = IntegerType::get(Ty->getContext(), 64);
          if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty))
            return IGCLLVM::FixedVectorType::get(I64Ty, VTy->getNumElements());
          return I64Ty;
        }
        return Ty;
      });

  auto *NewFTy = FunctionType::get(F.getReturnType(), NewArgTypes, false);
  auto *NewF = Function::Create(NewFTy, F.getLinkage());

  // Copy name, atributes and calling convention
  NewF->takeName(&F);
  NewF->setAttributes(F.getAttributes());
  NewF->setCallingConv(F.getCallingConv());

  F.getParent()->getFunctionList().insert(F.getIterator(), NewF);

  // Transfer debug info to the new function
  auto *DISubprog = F.getSubprogram();
  NewF->setSubprogram(DISubprog);
  F.setSubprogram(nullptr);

  // Splice the body of the old function into the new function.
  IGCLLVM::splice(NewF, NewF->begin(), &F);

  LLVM_DEBUG(dbgs() << "NewF " << NewF->getName() << "\n");

  transferArgumentUses(*NewF, F);

  return NewF;
}

bool GenXPropagateSurfaceState::runOnModule(Module &M) {
  const GenXSubtarget &ST = getAnalysis<TargetPassConfig>()
                                .getTM<GenXTargetMachine>()
                                .getGenXSubtarget();

  // Doing nothing if e64 is not enabled
  if (!ST.hasEfficient64b())
    return false;

  for (auto &F : M.functions()) {
    if (!F.isDeclaration() && vc::isKernel(F))
      processKernelFunction(F);
  }

  while (!Fmap.empty()) {
    auto It = Fmap.begin();
    auto *F = It->first;
    auto &Indices = It->second;
    auto *NewF = cloneFunction(*F, Indices);
    MapOldToNew.insert({F, NewF});
    processFunction(*NewF);
    Fmap.erase(It);
  }

  for (auto &&[Old, New] : MapOldToNew) {
    for (auto UseIt = Old->use_begin(); UseIt != Old->use_end();) {
      Use &U = *UseIt++;
      auto *CI = cast<CallInst>(U.getUser());
      IRBuilder<> Builder(CI);
      SmallVector<Value *, 6> Args;
      std::transform(CI->arg_begin(), CI->arg_end(), New->arg_begin(),
                     std::back_inserter(Args), [](auto &OldArg, auto &NewArg) {
                       if (OldArg->getType() == NewArg.getType())
                         return OldArg.get();
                       return cast<TruncInst>(OldArg)->getOperand(0);
                     });
      auto *NewCI = Builder.CreateCall(New, Args);
      Modify = true;
      NewCI->takeName(CI);
      CI->replaceAllUsesWith(NewCI);
      CI->eraseFromParent();
    }
    Old->eraseFromParent();
  }

  return Modify;
}

void GenXPropagateSurfaceState::processKernelFunction(Function &F) {
  IGC_ASSERT(!F.isDeclaration() && vc::isKernel(F));

  Map.clear();
  ToHandle.clear();

  vc::KernelMetadata KM{&F};
  auto ArgKinds = KM.getArgKinds();

  IGC_ASSERT_MESSAGE(ArgKinds.size() == F.arg_size(),
                     "Inconsistent arg kind metadata");

  for (auto &&[Arg, Kind] : zip(F.args(), ArgKinds)) {
    if (Kind == vc::KernelMetadata::AK_SAMPLER ||
        Kind == vc::KernelMetadata::AK_SURFACE) {
      OpQueue.push(&Arg);
      ToHandle.insert(&Arg);
    }
  }
  processFunction(F);
}

static bool eligibleForGettingUsers(Value *V) {
  // We have to stop when we reach a memory intrinsic
  if (vc::InternalIntrinsic::isInternalMemoryIntrinsic(V))
    return false;

  return !isa<CallInst>(V) || vc::isAnyNonTrivialIntrinsic(V);
}

void GenXPropagateSurfaceState::processFunction(Function &F) {
  auto HandleUsers = [&](Value *V) {
    for (auto *User : V->users())
      if (!ToHandle.contains(User)) {
        OpQueue.push(User);
        ToHandle.insert(User);
      }
  };

  Func = &F;

  while (!OpQueue.empty()) {
    auto *V = OpQueue.front();
    OpQueue.pop();
    LLVM_DEBUG(dbgs() << "Processing value: " << *V << "\n");

    bool NeedsHandleUsers = true;

    if (auto *I = dyn_cast<Instruction>(V)) {
      NeedsHandleUsers = false;
      if (auto *NewV = visit(I)) {
        NeedsHandleUsers = true;
        if (NewV != I) {
          LLVM_DEBUG(dbgs() << "Map.insert: " << *I << "->" << *NewV << "\n");
          Map.insert({I, NewV});
        }
      }
    }

    if (NeedsHandleUsers)
      HandleUsers(V);
  }
}

Value *GenXPropagateSurfaceState::visitCastInst(CastInst &I) const {
  return &I;
}

Value *GenXPropagateSurfaceState::visitSelectInst(SelectInst &I) const {
  return &I;
}

Value *GenXPropagateSurfaceState::visitInstruction(Instruction &I) const {
  IGC_ASSERT_EXIT_MESSAGE(0, "yet unsupported instruction");
  return nullptr;
}

Value *GenXPropagateSurfaceState::visitTruncInst(TruncInst &I) {
  auto *Ty = I.getType();
  // Only dealing with 32-bit surface pointers
  if (!Ty->isIntOrIntVectorTy(32))
    return nullptr;

  IGC_ASSERT(I.getSrcTy()->isIntOrIntVectorTy(64));
  return I.getOperand(0);
}

Value *GenXPropagateSurfaceState::visitInsertElementInst(InsertElementInst &I) {
  auto *Ty = I.getType();
  // Only dealing with 32-bit surface pointers
  if (!Ty->isIntOrIntVectorTy(32))
    return nullptr;

  auto *Into = I.getOperand(0);
  auto *V = I.getOperand(1);
  auto *Index = I.getOperand(2);

  auto It = Map.find(V);
  IGC_ASSERT(It != Map.end());
  V = It->second;

  if (isa<UndefValue>(Into)) {
    auto *VTy = cast<IGCLLVM::FixedVectorType>(Into->getType());
    auto *NewVTy =
        IGCLLVM::FixedVectorType::get(V->getType(), VTy->getNumElements());
    Into = UndefValue::get(NewVTy);
  } else {
    auto It = Map.find(Into);
    if (It == Map.end()) {
      // Try again later if Into operand is not ready
      OpQueue.push(&I);
      return nullptr;
    }
    Into = It->second;
  }

  Modify = true;
  IRBuilder<> Builder(&I);
  return Builder.CreateInsertElement(Into, V, Index);
}

Value *GenXPropagateSurfaceState::visitCallInst(CallInst &I) {
  IRBuilder<> Builder(&I);

  auto IID = vc::getAnyIntrinsicID(&I);
  if (IID == vc::InternalIntrinsic::optimization_fence)
    return I.getArgOperand(0);

  if (GenXIntrinsic::isRdRegion(&I)) {
    auto *Ty = I.getType();
    // Only dealing with 32-bit surface pointers
    if (!Ty->isIntOrIntVectorTy(32))
      return nullptr;

    auto *Src = I.getOperand(0);
    auto It = Map.find(Src);
    if (It == Map.end()) {
      // Try again later if Src operand is not ready
      OpQueue.push(&I);
      return nullptr;
    }

    auto *NewSrc = It->second;
    auto *NewSrcTy = cast<IGCLLVM::FixedVectorType>(NewSrc->getType());

    auto *ResTy = I.getType();
    auto *NewResTy = NewSrcTy->getElementType();
    if (auto *ResVTy = dyn_cast<IGCLLVM::FixedVectorType>(ResTy))
      NewResTy =
          IGCLLVM::FixedVectorType::get(NewResTy, ResVTy->getNumElements());

    Modify = true;

    // Multiplying by 2 as 32-bit element is replaced with 64-bit
    SmallVector<Value *, 6> Args = {
        NewSrc,
        I.getOperand(1),                                         // vstride
        I.getOperand(2),                                         // width
        I.getOperand(3),                                         // stride
        Builder.CreateMul(I.getOperand(4), Builder.getInt16(2)), // offset
        I.getOperand(5),
    };

    auto *Decl = vc::getAnyDeclarationForArgs(
        Func->getParent(), GenXIntrinsic::genx_rdregioni, NewResTy, Args);
    return Builder.CreateCall(Decl, Args);
  }

  if (GenXIntrinsic::isWrRegion(&I)) {
    auto *Ty = I.getType();
    // Only dealing with 32-bit surface pointers
    if (!Ty->isIntOrIntVectorTy(32))
      return nullptr;

    auto *Src = I.getOperand(1);
    auto It = Map.find(Src);
    if (It == Map.end()) {
      // Try again later if Src operand is not ready
      OpQueue.push(&I);
      return nullptr;
    }

    auto *NewSrc = It->second;
    auto *NewSrcTy = cast<IGCLLVM::FixedVectorType>(NewSrc->getType());

    auto *Into = I.getOperand(0);
    if (isa<UndefValue>(Into)) {
      auto *VTy = cast<IGCLLVM::FixedVectorType>(Into->getType());
      auto *NewVTy = IGCLLVM::FixedVectorType::get(NewSrcTy->getElementType(),
                                                   VTy->getNumElements());
      Into = UndefValue::get(NewVTy);
    } else {
      auto It = Map.find(Into);
      if (It == Map.end()) {
        // Try again later if Into operand is not ready
        OpQueue.push(&I);
        return nullptr;
      }
      Into = It->second;
    }

    auto *ResTy = I.getType();
    auto *NewResTy = NewSrcTy->getElementType();
    if (auto *ResVTy = dyn_cast<IGCLLVM::FixedVectorType>(ResTy))
      NewResTy =
          IGCLLVM::FixedVectorType::get(NewResTy, ResVTy->getNumElements());

    Modify = true;

    // Multiplying by 2 as 32-bit element is replaced with 64-bit
    SmallVector<Value *, 8> Args = {
        Into,
        NewSrc,
        I.getOperand(2),                                         // vstride
        I.getOperand(3),                                         // width
        I.getOperand(4),                                         // stride
        Builder.CreateMul(I.getOperand(5), Builder.getInt16(2)), // offset
        I.getOperand(6),
        I.getOperand(7),
    };

    auto *Decl = vc::getAnyDeclarationForArgs(
        Func->getParent(), GenXIntrinsic::genx_wrregioni, NewResTy, Args);
    return Builder.CreateCall(Decl, Args);
  }

  if (vc::InternalIntrinsic::isInternalMemoryIntrinsic(&I)) {
    const auto IID = vc::getAnyIntrinsicID(&I);
    const auto BTIIndex =
        vc::InternalIntrinsic::getMemorySurfaceOperandIndex(IID);
    IGC_ASSERT_EXIT_MESSAGE(BTIIndex >= 0, "Unknown BTI operand number");
    auto *BTI = I.getOperand(BTIIndex);
    if (isa<Argument>(BTI)) {
      // generate an error if BTI is a function argument to avoid a hang
      auto &Ctx = Func->getContext();
      vc::diagnose(Ctx, "GenXPropagateSurfaceState",
                   "BTI argument is not expected");
      return nullptr;
    }
    auto It = Map.find(BTI);
    if (It == Map.end()) {
      // Try again later if BIT operand is not ready
      OpQueue.push(&I);
      return nullptr;
    }

    // truncate can have been already inserted by clone function
    if (isa<TruncInst>(BTI))
      return nullptr;

    Modify = true;
    auto *Trunc = Builder.CreateTrunc(It->second, BTI->getType());
    I.setOperand(BTIIndex, Trunc);
    return nullptr;
  }

  if (auto *F = I.getCalledFunction(); F && !F->isDeclaration()) {
    SmallSet<unsigned, 6> BtiIndexSet;
    auto Fit = Fmap.find(F);
    if (Fit == Fmap.end()) {
      auto [Jt, Ok] = Fmap.insert({F, BtiIndexSet});
      IGC_ASSERT(Ok);
      Fit = Jt;
    }

    for (auto &Arg : I.args()) {
      auto It = Map.find(Arg);
      if (It != Map.end()) {
        Fit->second.insert(Arg.getOperandNo());
        Modify = true;
        auto *Trunc = Builder.CreateTrunc(It->second, Arg->getType());
        Arg.set(Trunc);
      }
    }
    return nullptr;
  }

  IGC_ASSERT_EXIT_MESSAGE(0, "Unimplemented");
  return nullptr;
}
