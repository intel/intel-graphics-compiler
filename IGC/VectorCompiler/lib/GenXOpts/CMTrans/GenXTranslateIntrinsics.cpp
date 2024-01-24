/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/InternalIntrinsics/InternalIntrinsics.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"

#include "visa_igc_common_header.h"

#include <llvm/GenXIntrinsics/GenXIntrinsics.h>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/Debug.h>

#define DEBUG_TYPE "genx-translate-intrinsics"

using namespace llvm;

namespace {
class GenXTranslateIntrinsics final
    : public FunctionPass,
      public InstVisitor<GenXTranslateIntrinsics> {
public:
  static char ID;
  GenXTranslateIntrinsics() : FunctionPass(ID) {}
  StringRef getPassName() const override {
    return "GenX intrinsics translator";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
  }

  bool runOnFunction(Function &F) override;

  void visitCallInst(CallInst &I) const;

private:
  Constant *translateCacheControls(Constant *L1, Constant *L3) const;
  Value *translateMath(CallInst &I, Intrinsic::ID IID,
                       bool HasApproxFunc = true) const;
  Value *translateBFloat16Convert(CallInst &I) const;
  Value *translateTFloat32Convert(CallInst &I) const;
  Value *translateStochasticRounding(CallInst &I) const;
  Value *translateLscAtomic(CallInst &I) const;
  Value *translateLscLoadStore(CallInst &I) const;
  Value *translateLscLoadStoreBlock2D(CallInst &I) const;
  Value *translateLscTyped(CallInst &I) const;
};
} // namespace

char GenXTranslateIntrinsics::ID = 0;

INITIALIZE_PASS_BEGIN(GenXTranslateIntrinsics, "GenXTranslateIntrinsics",
                      "GenXTranslateIntrinsics", false, false)
INITIALIZE_PASS_END(GenXTranslateIntrinsics, "GenXTranslateIntrinsics",
                    "GenXTranslateIntrinsics", false, false)

FunctionPass *llvm::createGenXTranslateIntrinsicsPass() {
  initializeGenXTranslateIntrinsicsPass(*PassRegistry::getPassRegistry());
  return new GenXTranslateIntrinsics;
}

bool GenXTranslateIntrinsics::runOnFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "GenXTranslateIntrinsics started\n");
  visit(F);
  LLVM_DEBUG(dbgs() << "GenXTranslateIntrinsics finished\n");
  return true;
}

void GenXTranslateIntrinsics::visitCallInst(CallInst &I) const {
  auto IID = GenXIntrinsic::getGenXIntrinsicID(&I);
  Value *NewI = nullptr;

  switch (IID) {
  default:
    return;
  case GenXIntrinsic::genx_fmax:
    NewI = translateMath(I, Intrinsic::maxnum, false);
    break;
  case GenXIntrinsic::genx_fmin:
    NewI = translateMath(I, Intrinsic::minnum, false);
    break;
  case GenXIntrinsic::genx_cos:
    NewI = translateMath(I, Intrinsic::cos);
    break;
  case GenXIntrinsic::genx_exp:
    NewI = translateMath(I, Intrinsic::exp2);
    break;
  case GenXIntrinsic::genx_log:
    NewI = translateMath(I, Intrinsic::log2);
    break;
  case GenXIntrinsic::genx_sin:
    NewI = translateMath(I, Intrinsic::sin);
    break;
  case GenXIntrinsic::genx_pow:
    NewI = translateMath(I, Intrinsic::pow);
    break;
  case GenXIntrinsic::genx_bf_cvt:
    NewI = translateBFloat16Convert(I);
    break;
  case GenXIntrinsic::genx_tf32_cvt:
    NewI = translateTFloat32Convert(I);
    break;
  case GenXIntrinsic::genx_srnd:
    NewI = translateStochasticRounding(I);
    break;
  case GenXIntrinsic::genx_lsc_xatomic_bti:
  case GenXIntrinsic::genx_lsc_xatomic_slm:
  case GenXIntrinsic::genx_lsc_xatomic_stateless:
    NewI = translateLscAtomic(I);
    break;
  case GenXIntrinsic::genx_lsc_load_bti:
  case GenXIntrinsic::genx_lsc_load_merge_bti:
  case GenXIntrinsic::genx_lsc_load_merge_quad_bti:
  case GenXIntrinsic::genx_lsc_load_merge_quad_slm:
  case GenXIntrinsic::genx_lsc_load_merge_quad_stateless:
  case GenXIntrinsic::genx_lsc_load_merge_slm:
  case GenXIntrinsic::genx_lsc_load_merge_stateless:
  case GenXIntrinsic::genx_lsc_load_quad_bti:
  case GenXIntrinsic::genx_lsc_load_quad_slm:
  case GenXIntrinsic::genx_lsc_load_quad_stateless:
  case GenXIntrinsic::genx_lsc_load_slm:
  case GenXIntrinsic::genx_lsc_load_stateless:
  case GenXIntrinsic::genx_lsc_prefetch_bti:
  case GenXIntrinsic::genx_lsc_prefetch_stateless:
  case GenXIntrinsic::genx_lsc_store_bti:
  case GenXIntrinsic::genx_lsc_store_quad_bti:
  case GenXIntrinsic::genx_lsc_store_quad_slm:
  case GenXIntrinsic::genx_lsc_store_quad_stateless:
  case GenXIntrinsic::genx_lsc_store_slm:
  case GenXIntrinsic::genx_lsc_store_stateless:
    NewI = translateLscLoadStore(I);
    break;
  case GenXIntrinsic::genx_lsc_load2d_stateless:
  case GenXIntrinsic::genx_lsc_prefetch2d_stateless:
  case GenXIntrinsic::genx_lsc_store2d_stateless:
    NewI = translateLscLoadStoreBlock2D(I);
    break;
  case GenXIntrinsic::genx_lsc_load_merge_quad_typed_bti:
  case GenXIntrinsic::genx_lsc_prefetch_quad_typed_bti:
  case GenXIntrinsic::genx_lsc_store_quad_typed_bti:
    NewI = translateLscTyped(I);
    break;
  }

  if (!NewI)
    return;

  if (!I.getType()->isVoidTy()) {
    NewI->takeName(&I);
    I.replaceAllUsesWith(NewI);
  }

  I.eraseFromParent();
  return;
}

Value *GenXTranslateIntrinsics::translateMath(CallInst &I, Intrinsic::ID IID,
                                              bool HasApproxFunc) const {
  LLVM_DEBUG(dbgs() << "Translate: " << I << "\n");
  IRBuilder<> Builder(&I);

  SmallVector<Value *, 4> Args(I.args());
  auto *NewI = Builder.CreateIntrinsic(IID, {I.getType()}, Args);
  NewI->setHasApproxFunc(HasApproxFunc);

  LLVM_DEBUG(dbgs() << "Created: " << *NewI << "\n");
  return NewI;
}

Value *GenXTranslateIntrinsics::translateBFloat16Convert(CallInst &I) const {
  IGC_ASSERT_EXIT(GenXIntrinsic::getGenXIntrinsicID(&I) ==
                  GenXIntrinsic::genx_bf_cvt);
  LLVM_DEBUG(dbgs() << "Translate: " << I << "\n");
  IRBuilder<> Builder(&I);
  Module *M = I.getModule();

  auto *Arg = I.getArgOperand(0);
  auto *ArgTy = Arg->getType();
  auto *ArgETy = ArgTy;
  auto *RetTy = I.getType();

  Type *I16Ty = Builder.getInt16Ty();

  if (auto *ArgVTy = dyn_cast<IGCLLVM::FixedVectorType>(ArgTy)) {
    ArgETy = ArgVTy->getElementType();
    I16Ty = IGCLLVM::FixedVectorType::get(I16Ty, ArgVTy->getNumElements());
  }

  Function *Func = nullptr;
  if (ArgETy->isHalfTy()) {
    Func = vc::InternalIntrinsic::getInternalDeclaration(
        M, vc::InternalIntrinsic::cast_from_bf16, {RetTy, I16Ty});
    Arg = Builder.CreateBitCast(Arg, I16Ty);
  } else {
    Func = vc::InternalIntrinsic::getInternalDeclaration(
        M, vc::InternalIntrinsic::cast_to_bf16, {I16Ty, ArgTy});
  }

  auto *NewI = Builder.CreateCall(Func, {Arg});
  LLVM_DEBUG(dbgs() << "Created: " << *NewI << "\n");

  return Builder.CreateBitCast(NewI, RetTy);
}

Value *GenXTranslateIntrinsics::translateTFloat32Convert(CallInst &I) const {
  IGC_ASSERT_EXIT(GenXIntrinsic::getGenXIntrinsicID(&I) ==
                  GenXIntrinsic::genx_tf32_cvt);
  LLVM_DEBUG(dbgs() << "Translate: " << I << "\n");
  IRBuilder<> Builder(&I);
  Module *M = I.getModule();

  auto *Arg = I.getArgOperand(0);
  auto *ArgTy = Arg->getType();
  auto *RetTy = I.getType();

  Function *Func = vc::InternalIntrinsic::getInternalDeclaration(
      M, vc::InternalIntrinsic::round_to_tf32, {RetTy, ArgTy});
  auto *NewI = Builder.CreateCall(Func, {Arg});
  LLVM_DEBUG(dbgs() << "Created: " << *NewI << "\n");

  return NewI;
}

Value *GenXTranslateIntrinsics::translateStochasticRounding(CallInst &I) const {
  IGC_ASSERT_EXIT(GenXIntrinsic::getGenXIntrinsicID(&I) ==
                  GenXIntrinsic::genx_srnd);
  LLVM_DEBUG(dbgs() << "Translate: " << I << "\n");
  IRBuilder<> Builder(&I);
  Module *M = I.getModule();

  auto *RetTy = I.getType();
  auto *SrcV = I.getArgOperand(0);
  auto *RndV = I.getArgOperand(1);

  auto RetElementSize = RetTy->getScalarSizeInBits();

  auto *RndOrigTy = RndV->getType();
  auto RndElementSize = RndOrigTy->getScalarSizeInBits();

  if (RndOrigTy->isFPOrFPVectorTy()) {
    Type *RndCastTy = Builder.getIntNTy(RndElementSize);
    if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(RndOrigTy))
      RndCastTy =
          IGCLLVM::FixedVectorType::get(RndCastTy, VTy->getNumElements());
    RndV = Builder.CreateBitCast(RndV, RndCastTy);
  }

  if (RndElementSize != RetElementSize) {
    Type *RndTruncTy = Builder.getIntNTy(RetElementSize);
    if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(RndOrigTy))
      RndTruncTy =
          IGCLLVM::FixedVectorType::get(RndTruncTy, VTy->getNumElements());
    RndV = Builder.CreateTrunc(RndV, RndTruncTy);
  }

  auto *SrcTy = SrcV->getType();
  auto *RndTy = RndV->getType();

  auto IID = vc::InternalIntrinsic::stochastic_round_to_f16;

  Function *Func = vc::InternalIntrinsic::getInternalDeclaration(
      M, IID, {RetTy, SrcTy, RndTy});
  auto *NewI = Builder.CreateCall(Func, {SrcV, RndV});
  LLVM_DEBUG(dbgs() << "Created: " << *NewI << "\n");

  return NewI;
}

Constant *GenXTranslateIntrinsics::translateCacheControls(Constant *L1,
                                                          Constant *L3) const {
  return ConstantVector::get({L1, L3});
}

Value *GenXTranslateIntrinsics::translateLscLoadStore(CallInst &I) const {
  auto IID = GenXIntrinsic::getGenXIntrinsicID(&I);
  LLVM_DEBUG(dbgs() << "Translate: " << I << "\n");
  IRBuilder<> Builder(&I);
  Module *M = I.getModule();

  auto *Pred = I.getArgOperand(0);

  auto *L1Control = cast<Constant>(I.getArgOperand(2));
  auto *L3Control = cast<Constant>(I.getArgOperand(3));
  auto *CacheOpts = translateCacheControls(L1Control, L3Control);

  auto *Scale = I.getArgOperand(4);
  auto *Offset = I.getArgOperand(5);
  auto *ElementSize = I.getArgOperand(6);
  auto *VectorSize = I.getArgOperand(7);
  auto *ChannelMask = I.getArgOperand(9);
  auto *Addr = I.getArgOperand(10);

  Value *Base = nullptr;
  Value *Src = nullptr;

  auto *ResTy = I.getType();
  if (!ResTy->isVoidTy())
    Src = UndefValue::get(ResTy);

  auto AddrSize = LSC_ADDR_SIZE_32b;
  bool IsQuad = false;

  auto NewIID = vc::InternalIntrinsic::not_internal_intrinsic;
  switch (IID) {
  default:
    IGC_ASSERT_UNREACHABLE();
  case GenXIntrinsic::genx_lsc_load_merge_quad_bti:
  case GenXIntrinsic::genx_lsc_load_quad_bti:
    IsQuad = true;
    LLVM_FALLTHROUGH;
  case GenXIntrinsic::genx_lsc_load_merge_bti:
  case GenXIntrinsic::genx_lsc_load_bti:
    Base = I.getArgOperand(11);
    if (IGCLLVM::getNumArgOperands(&I) == 13)
      Src = I.getArgOperand(12);
    NewIID = IsQuad ? vc::InternalIntrinsic::lsc_load_quad_bti
                    : vc::InternalIntrinsic::lsc_load_bti;
    break;
  case GenXIntrinsic::genx_lsc_load_merge_quad_slm:
  case GenXIntrinsic::genx_lsc_load_quad_slm:
    IsQuad = true;
    LLVM_FALLTHROUGH;
  case GenXIntrinsic::genx_lsc_load_merge_slm:
  case GenXIntrinsic::genx_lsc_load_slm:
    Base = Builder.getInt32(0);
    if (IGCLLVM::getNumArgOperands(&I) == 13)
      Src = I.getArgOperand(12);
    NewIID = IsQuad ? vc::InternalIntrinsic::lsc_load_quad_slm
                    : vc::InternalIntrinsic::lsc_load_slm;
    break;
  case GenXIntrinsic::genx_lsc_load_merge_quad_stateless:
  case GenXIntrinsic::genx_lsc_load_quad_stateless:
    IsQuad = true;
    LLVM_FALLTHROUGH;
  case GenXIntrinsic::genx_lsc_load_merge_stateless:
  case GenXIntrinsic::genx_lsc_load_stateless:
    Base = Builder.getInt64(0);
    if (IGCLLVM::getNumArgOperands(&I) == 13)
      Src = I.getArgOperand(12);
    NewIID = IsQuad ? vc::InternalIntrinsic::lsc_load_quad_ugm
                    : vc::InternalIntrinsic::lsc_load_ugm;
    AddrSize = LSC_ADDR_SIZE_64b;
    break;
  case GenXIntrinsic::genx_lsc_prefetch_bti:
    Src = nullptr;
    Base = I.getArgOperand(11);
    NewIID = vc::InternalIntrinsic::lsc_prefetch_bti;
    // not supported Quad-version(vc::InternalIntrinsic::lsc_prefetch_quad_bti)
    break;
  case GenXIntrinsic::genx_lsc_prefetch_stateless:
    Src = nullptr;
    Base = Builder.getInt64(0);
    NewIID = vc::InternalIntrinsic::lsc_prefetch_ugm;
    // not supported Quad-version(vc::InternalIntrinsic::lsc_prefetch_quad_ugm)
    AddrSize = LSC_ADDR_SIZE_64b;
    break;
  case GenXIntrinsic::genx_lsc_store_quad_bti:
    IsQuad = true;
    LLVM_FALLTHROUGH;
  case GenXIntrinsic::genx_lsc_store_bti:
    Base = I.getArgOperand(12);
    Src = I.getArgOperand(11);
    NewIID = IsQuad ? vc::InternalIntrinsic::lsc_store_quad_bti
                    : vc::InternalIntrinsic::lsc_store_bti;
    break;
  case GenXIntrinsic::genx_lsc_store_quad_slm:
    IsQuad = true;
    LLVM_FALLTHROUGH;
  case GenXIntrinsic::genx_lsc_store_slm:
    Base = Builder.getInt32(0);
    Src = I.getArgOperand(11);
    NewIID = IsQuad ? vc::InternalIntrinsic::lsc_store_quad_slm
                    : vc::InternalIntrinsic::lsc_store_slm;
    break;
  case GenXIntrinsic::genx_lsc_store_quad_stateless:
    IsQuad = true;
    LLVM_FALLTHROUGH;
  case GenXIntrinsic::genx_lsc_store_stateless:
    Base = Builder.getInt64(0);
    Src = I.getArgOperand(11);
    NewIID = IsQuad ? vc::InternalIntrinsic::lsc_store_quad_ugm
                    : vc::InternalIntrinsic::lsc_store_ugm;
    AddrSize = LSC_ADDR_SIZE_64b;
    break;
  }

  SmallVector<Type *, 4> Types;
  if (!ResTy->isVoidTy())
    Types.push_back(ResTy);
  Types.push_back(Pred->getType());
  Types.push_back(CacheOpts->getType());
  Types.push_back(Addr->getType());
  if (Src && ResTy->isVoidTy())
    Types.push_back(Src->getType());

  SmallVector<Value *, 10> Args = {
      Pred, // translate genx to internal intrinsic args
      Builder.getInt8(AddrSize),
      ElementSize,
      IsQuad ? ChannelMask : VectorSize,
      CacheOpts,
      Base,
      Addr,
      Scale,
      Offset,
  };
  if (Src)
    Args.push_back(Src);

  auto *Func = vc::InternalIntrinsic::getInternalDeclaration(M, NewIID, Types);
  auto *NewI = Builder.CreateCall(Func, Args);
  LLVM_DEBUG(dbgs() << "New intrinsic generated: " << *NewI);

  return NewI;
}

Value *GenXTranslateIntrinsics::translateLscAtomic(CallInst &I) const {
  auto IID = GenXIntrinsic::getGenXIntrinsicID(&I);
  LLVM_DEBUG(dbgs() << "Translate: " << I << "\n");
  IRBuilder<> Builder(&I);
  Module *M = I.getModule();

  auto *Pred = I.getArgOperand(0);
  auto *Opcode = I.getArgOperand(1);

  auto *L1Control = cast<Constant>(I.getArgOperand(2));
  auto *L3Control = cast<Constant>(I.getArgOperand(3));
  auto *CacheOpts = translateCacheControls(L1Control, L3Control);

  auto *Scale = I.getArgOperand(4);
  auto *Offset = I.getArgOperand(5);
  auto *ElementSize = I.getArgOperand(6);
  auto *Addr = I.getArgOperand(10);
  auto *Src0 = I.getArgOperand(11);
  auto *Src1 = I.getArgOperand(12);
  auto *Passthru = I.getArgOperand(14);

  Value *Base = nullptr;
  auto AddrSize = LSC_ADDR_SIZE_32b;
  auto NewIID = vc::InternalIntrinsic::not_internal_intrinsic;

  switch (IID) {
  default:
    IGC_ASSERT_UNREACHABLE();
  case GenXIntrinsic::genx_lsc_xatomic_bti:
    Base = I.getArgOperand(13);
    NewIID = vc::InternalIntrinsic::lsc_atomic_bti;
    break;
  case GenXIntrinsic::genx_lsc_xatomic_slm:
    Base = Builder.getInt32(0);
    NewIID = vc::InternalIntrinsic::lsc_atomic_slm;
    break;
  case GenXIntrinsic::genx_lsc_xatomic_stateless:
    Base = Builder.getInt64(0);
    NewIID = vc::InternalIntrinsic::lsc_atomic_ugm;
    AddrSize = LSC_ADDR_SIZE_64b;
    break;
  }

  auto *Func = vc::InternalIntrinsic::getInternalDeclaration(
      M, NewIID,
      {I.getType(), Pred->getType(), CacheOpts->getType(), Addr->getType()});
  auto *NewI = Builder.CreateCall(
      Func, {Pred, Opcode, Builder.getInt8(AddrSize), ElementSize, CacheOpts,
             Base, Addr, Scale, Offset, Src0, Src1, Passthru});
  LLVM_DEBUG(dbgs() << "New intrinsic generated: " << *NewI);

  return NewI;
}

Value *
GenXTranslateIntrinsics::translateLscLoadStoreBlock2D(CallInst &I) const {
  auto IID = GenXIntrinsic::getGenXIntrinsicID(&I);
  LLVM_DEBUG(dbgs() << "Translate: " << I << "\n");
  IRBuilder<> Builder(&I);
  Module *M = I.getModule();

  auto *Pred = I.getArgOperand(0);
  auto *PredTy = Pred->getType();

  IGC_ASSERT_EXIT(PredTy->isIntOrIntVectorTy(1));
  if (auto *PredVTy = dyn_cast<IGCLLVM::FixedVectorType>(PredTy)) {
    IGC_ASSERT_EXIT(PredVTy->getNumElements() == 1);
    constexpr uint64_t Index = 0;
    Pred = Builder.CreateExtractElement(Pred, Index);
  }

  auto *L1Control = cast<Constant>(I.getArgOperand(1));
  auto *L3Control = cast<Constant>(I.getArgOperand(2));
  auto *CacheOpts = translateCacheControls(L1Control, L3Control);

  auto *DataSize = I.getArgOperand(3);
  auto *NumBlocks = I.getArgOperand(5);
  auto *BlockWidth = I.getArgOperand(6);
  auto *BlockHeight = I.getArgOperand(7);

  auto *Base = I.getArgOperand(9);
  auto *Width = I.getArgOperand(10);
  auto *Height = I.getArgOperand(11);
  auto *Pitch = I.getArgOperand(12);
  auto *X = I.getArgOperand(13);
  auto *Y = I.getArgOperand(14);

  Value *Src = nullptr;
  auto *Ty = I.getType();

  const bool IsTransposed =
      cast<ConstantInt>(I.getArgOperand(4))->getZExtValue() ==
      LSC_DATA_ORDER_TRANSPOSE;
  const bool IsVNNI =
      cast<ConstantInt>(I.getArgOperand(8))->getZExtValue() != 0;

  auto NewIID = vc::InternalIntrinsic::not_internal_intrinsic;
  switch (IID) {
  default:
    IGC_ASSERT_UNREACHABLE();
  case GenXIntrinsic::genx_lsc_load2d_stateless:
    Src = UndefValue::get(Ty);
    if (IsTransposed)
      NewIID = vc::InternalIntrinsic::lsc_load_block_2d_ugm_transposed;
    else if (IsVNNI)
      NewIID = vc::InternalIntrinsic::lsc_load_block_2d_ugm_vnni;
    else
      NewIID = vc::InternalIntrinsic::lsc_load_block_2d_ugm;
    break;
  case GenXIntrinsic::genx_lsc_prefetch2d_stateless:
    IGC_ASSERT(!IsTransposed && !IsVNNI);
    NewIID = vc::InternalIntrinsic::lsc_prefetch_block_2d_ugm;
    break;
  case GenXIntrinsic::genx_lsc_store2d_stateless:
    IGC_ASSERT(!IsTransposed && !IsVNNI);
    Src = I.getArgOperand(15);
    NewIID = vc::InternalIntrinsic::lsc_store_block_2d_ugm;
    break;
  }

  SmallVector<Type *, 2> Types;
  if (!Ty->isVoidTy())
    Types.push_back(Ty);
  Types.push_back(CacheOpts->getType());
  if (Src && Ty->isVoidTy())
    Types.push_back(Src->getType());

  auto *Func = vc::InternalIntrinsic::getInternalDeclaration(M, NewIID, Types);

  SmallVector<Value *, 15> Args = {
      Pred,
      DataSize,
      CacheOpts,
      NumBlocks,
      BlockWidth,
      BlockHeight,
      Base,
      Width,
      Height,
      Pitch,
      X,
      Y,
      Builder.getInt32(0), // X offset
      Builder.getInt32(0), // Y offset
  };
  if (Src)
    Args.push_back(Src);

  auto *NewI = Builder.CreateCall(Func, Args);
  LLVM_DEBUG(dbgs() << "New intrinsic generated: " << *NewI);

  return NewI;
}

Value *GenXTranslateIntrinsics::translateLscTyped(CallInst &I) const {
  auto IID = GenXIntrinsic::getGenXIntrinsicID(&I);
  LLVM_DEBUG(dbgs() << "Translate: " << I << "\n");
  IRBuilder<> Builder(&I);
  Module *M = I.getModule();

  auto NewIID = vc::InternalIntrinsic::not_internal_intrinsic;

  switch (IID) {
  default:
    IGC_ASSERT_UNREACHABLE();
  case GenXIntrinsic::genx_lsc_load_merge_quad_typed_bti:
    NewIID = vc::InternalIntrinsic::lsc_load_quad_tgm;
    break;
  case GenXIntrinsic::genx_lsc_prefetch_quad_typed_bti:
    NewIID = vc::InternalIntrinsic::lsc_prefetch_quad_tgm;
    break;
  case GenXIntrinsic::genx_lsc_store_quad_typed_bti:
    NewIID = vc::InternalIntrinsic::lsc_store_quad_tgm;
    break;
  }

  SmallVector<Type *, 3> Types;

  if (vc::InternalIntrinsic::isOverloadedRet(NewIID))
    Types.push_back(I.getType());

  for (unsigned Idx = 0; Idx < I.arg_size(); Idx++)
    if (vc::InternalIntrinsic::isOverloadedArg(NewIID, Idx))
      Types.push_back(I.getArgOperand(Idx)->getType());

  auto *Func = vc::InternalIntrinsic::getInternalDeclaration(M, NewIID, Types);

  SmallVector<Value *, 10> Args(I.args());
  auto *NewI = Builder.CreateCall(Func, Args);
  LLVM_DEBUG(dbgs() << "New intrinsic generated: " << *NewI);

  return NewI;
}
