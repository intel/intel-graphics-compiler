/*========================== begin_copyright_notice ============================

Copyright (C) 2024-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"

#include "vc/Support/BackendConfig.h"
#include "vc/Utils/GenX/Intrinsics.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"

#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Support/MathExtras.h"

#include "llvm/Analysis/ValueTracking.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/KnownBits.h"

#define DEBUG_TYPE "genx-lsc-addr-calc-folding"

using namespace llvm;
using namespace genx;

namespace {
class GenXLscAddrCalcFolding : public FunctionPass,
                               public InstVisitor<GenXLscAddrCalcFolding> {
public:
  static char ID;
  GenXLscAddrCalcFolding() : FunctionPass(ID) {}

  StringRef getPassName() const override {
    return "GenX LSC Address Calculation Folding";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<GenXBackendConfig>();
    AU.addRequired<TargetPassConfig>();
    AU.setPreservesCFG();
  }

  bool runOnFunction(Function &F) override;
  void visitCallInst(CallInst &CI);

private:
  bool foldLscAddrCalculation(CallInst &CI);
  bool foldLscAddrBase(CallInst &CI);
  bool foldLscAddrExtend(CallInst &CI);

  Value *applyLscAddrConstFolding(Value *Offsets, APInt &Scale, APInt &Offset);

  Value *applyLscAddrFolding(Value *Offsets, APInt &Scale, APInt &Offset,
                             CallInst *Inst);

  static constexpr unsigned Block2DIndexX = 10;
  static constexpr unsigned Block2DIndexY = 11;
  static constexpr unsigned Block2DOffsetX = 12;
  static constexpr unsigned Block2DOffsetY = 13;

  bool foldLscBlock2DAddrCalculation(CallInst &CI, unsigned IndexArg,
                                     unsigned OffsetArg);

  const GenXBackendConfig *BC = nullptr;
  const GenXSubtarget *ST = nullptr;

  unsigned Supported2DOffsetBits = 0;
  bool Changed = false;

  unsigned OffsetAlignment = 0;

  unsigned AllowedScale = 1;
};

} // namespace

char GenXLscAddrCalcFolding::ID = 0;

namespace llvm {
void initializeGenXLscAddrCalcFoldingPass(PassRegistry &);
} // namespace llvm

INITIALIZE_PASS_BEGIN(GenXLscAddrCalcFolding, "GenXLscAddrCalcFolding",
                      "GenXLscAddrCalcFolding", false, false)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXLscAddrCalcFolding, "GenXLscAddrCalcFolding",
                    "GenXLscAddrCalcFolding", false, false)

FunctionPass *llvm::createGenXLscAddrCalcFoldingPass() {
  initializeGenXLscAddrCalcFoldingPass(*PassRegistry::getPassRegistry());
  return new GenXLscAddrCalcFolding();
}

bool GenXLscAddrCalcFolding::runOnFunction(Function &F) {
  BC = &getAnalysis<GenXBackendConfig>();
  ST = &getAnalysis<TargetPassConfig>()
            .getTM<GenXTargetMachine>()
            .getGenXSubtarget();

  if (!ST->hasLSCMessages() || !ST->hasLSCOffset())
    return false;

  Changed = false;
  visit(F);
  return Changed;
}

void GenXLscAddrCalcFolding::visitCallInst(CallInst &CI) {
  IGC_ASSERT(ST->hasLSCMessages() && ST->hasLSCOffset());

  const auto IID = vc::getAnyIntrinsicID(&CI);

  switch (IID) {
  default:
    break;
  case vc::InternalIntrinsic::lsc_atomic_ugm:
  case vc::InternalIntrinsic::lsc_load_ugm:
  case vc::InternalIntrinsic::lsc_load_quad_ugm:
  case vc::InternalIntrinsic::lsc_prefetch_ugm:
  case vc::InternalIntrinsic::lsc_prefetch_quad_ugm:
  case vc::InternalIntrinsic::lsc_store_ugm:
  case vc::InternalIntrinsic::lsc_store_quad_ugm:
  case vc::InternalIntrinsic::lsc_atomic_slm:
  case vc::InternalIntrinsic::lsc_load_slm:
  case vc::InternalIntrinsic::lsc_load_quad_slm:
  case vc::InternalIntrinsic::lsc_store_slm:
  case vc::InternalIntrinsic::lsc_store_quad_slm:
    if (ST->hasLSCBase()) {
      Changed |= foldLscAddrCalculation(CI);
      Changed |= foldLscAddrBase(CI);
      Changed |= foldLscAddrCalculation(CI);
      Changed |= foldLscAddrExtend(CI);
    }
    LLVM_FALLTHROUGH;
  case vc::InternalIntrinsic::lsc_atomic_bti:
  case vc::InternalIntrinsic::lsc_load_bti:
  case vc::InternalIntrinsic::lsc_load_quad_bti:
  case vc::InternalIntrinsic::lsc_prefetch_bti:
  case vc::InternalIntrinsic::lsc_prefetch_quad_bti:
  case vc::InternalIntrinsic::lsc_store_bti:
  case vc::InternalIntrinsic::lsc_store_quad_bti:
    Changed |= foldLscAddrCalculation(CI);
    break;
  case vc::InternalIntrinsic::lsc_load_block_2d_ugm:
  case vc::InternalIntrinsic::lsc_load_block_2d_ugm_transposed:
  case vc::InternalIntrinsic::lsc_load_block_2d_ugm_vnni:
  case vc::InternalIntrinsic::lsc_prefetch_block_2d_ugm:
  case vc::InternalIntrinsic::lsc_store_block_2d_ugm:
    Changed |= foldLscBlock2DAddrCalculation(CI, Block2DIndexX, Block2DOffsetX);
    Changed |= foldLscBlock2DAddrCalculation(CI, Block2DIndexY, Block2DOffsetY);
    break;
  }
}

bool GenXLscAddrCalcFolding::foldLscBlock2DAddrCalculation(CallInst &CI,
                                                           unsigned IndexArg,
                                                           unsigned OffsetArg) {
  IGC_ASSERT(ST->hasLSCMessages() && ST->hasLSCOffset());

  auto *Index = CI.getArgOperand(IndexArg);
  auto *OldIndex = Index;
  auto Offset = cast<ConstantInt>(CI.getArgOperand(OffsetArg))->getValue();

  while (auto *BO = dyn_cast<BinaryOperator>(Index)) {
    auto Opcode = BO->getOpcode();
    if (Opcode != Instruction::Add && Opcode != Instruction::Sub)
      break;

    auto *Const = dyn_cast<ConstantInt>(BO->getOperand(1));
    if (!Const)
      break;

    auto ConstValue = Const->getValue();

    APInt NewOffset;
    bool Overflow = false;

    switch (Opcode) {
    case Instruction::Add:
      NewOffset = Offset.sadd_ov(ConstValue, Overflow);
      break;
    case Instruction::Sub:
      NewOffset = Offset.ssub_ov(ConstValue, Overflow);
      break;
    default:
      llvm_unreachable("Unexpected opcode");
    }

    if (Overflow)
      break;

    Offset = std::move(NewOffset);
    Index = BO->getOperand(0);

    LLVM_DEBUG(dbgs() << "LSC address folding found, index: " << *Index
                      << ", offset: " << Offset.getSExtValue() << "\n");
  }

  if (Index == OldIndex)
    return false;

  const auto OffsetV = Offset.getSExtValue();
  const auto ElementSizeBits =
      vc::InternalIntrinsic::getMemoryRegisterElementSize(&CI);
  if (OffsetV * ElementSizeBits % genx::DWordBits != 0) {
    LLVM_DEBUG(dbgs() << "Offset is not dword-aligned\n");
    return false;
  }

  IRBuilder<> Builder(&CI);

  LLVM_DEBUG(dbgs() << "Folding LSC address calculation for instruction: " << CI
                    << "\n");
  CI.setArgOperand(IndexArg, Index);
  CI.setArgOperand(OffsetArg, Builder.getInt32(OffsetV));
  LLVM_DEBUG(dbgs() << "Updated instruction: " << CI << "\n");

  return true;
}

bool GenXLscAddrCalcFolding::foldLscAddrCalculation(CallInst &Inst) {
  constexpr unsigned AddrIndex = 6, ScaleIndex = 7, OffsetIndex = 8;

  IGC_ASSERT(ST->hasLSCMessages() && ST->hasLSCOffset());
  IGC_ASSERT_MESSAGE(isa<ConstantInt>(Inst.getOperand(ScaleIndex)) &&
                         isa<ConstantInt>(Inst.getOperand(OffsetIndex)),
                     "Scale and Offset must be constant");

  bool Changed = false;
  auto *Index = Inst.getOperand(AddrIndex);
  auto Scale = cast<ConstantInt>(Inst.getOperand(ScaleIndex))->getValue();
  auto Offset = cast<ConstantInt>(Inst.getOperand(OffsetIndex))->getValue();

  OffsetAlignment = vc::InternalIntrinsic::getMemoryRegisterElementSize(&Inst) /
                    genx::ByteBits;

  AllowedScale = vc::InternalIntrinsic::getMemoryRegisterElementSize(&Inst) *
                 vc::InternalIntrinsic::getMemoryVectorSizePerLane(&Inst) /
                 genx::ByteBits;

  AllowedScale = isPowerOf2_32(AllowedScale) ? AllowedScale : 1;
  AllowedScale = std::min(AllowedScale, ST->getLSCScaleMax());

  while (auto *NewIndex = applyLscAddrFolding(Index, Scale, Offset, &Inst)) {
    Index = NewIndex;
    Changed = true;
    LLVM_DEBUG(dbgs() << "LSC address folding found, index: " << *Index
                      << ", scale: " << Scale.getZExtValue()
                      << ", offset: " << Offset.getSExtValue() << "\n");
  }

  if (Changed) {
    IRBuilder<> Builder(&Inst);
    LLVM_DEBUG(dbgs() << "Folding LSC address calculation for instruction: "
                      << Inst << "\n");
    auto OffsetV = Offset.getSExtValue();
    if (vc::InternalIntrinsic::isSlmIntrinsic(&Inst) &&
        (OffsetV & SlmNullProtectionMask) == genx::SlmNullProtection)
      OffsetV &= ~SlmNullProtectionMask;

    Inst.setOperand(AddrIndex, Index);
    Inst.setOperand(ScaleIndex, Builder.getInt16(Scale.getZExtValue()));
    Inst.setOperand(OffsetIndex, Builder.getInt32(OffsetV));
    LLVM_DEBUG(dbgs() << "Updated instruction: " << Inst << "\n");
  }

  return Changed;
}

// applyLscAddrFolding : fold address calculation of LSC intrinsics
//
// Addr = Offsets * Scale + Offsets
//
// If Offsets is add-like operation (Offsets = Offsets0 + Imm0), it can be
// folded in new ImmOffset.
//
// If Offsets is mul-like operation (Offsets = Offsets0 * Imm0), it can be
// folded in new Scale
//
// This folding is done iteratively for chains of such operations.
//
Value *GenXLscAddrCalcFolding::applyLscAddrFolding(Value *Offsets, APInt &Scale,
                                                   APInt &Offset,
                                                   CallInst *Inst) {
  LLVM_DEBUG(dbgs() << "applyLscAddrFolding instruction: " << Inst << "\n");
  IGC_ASSERT(ST->hasLSCMessages() && ST->hasLSCOffset());

  if (isa<ConstantDataVector>(Offsets))
    return applyLscAddrConstFolding(Offsets, Scale, Offset);

  if (!isa<BinaryOperator>(Offsets))
    return nullptr;

  auto *BinOp = cast<BinaryOperator>(Offsets);

  unsigned ConstIdx;
  if (isa<Constant>(BinOp->getOperand(0)))
    ConstIdx = 0;
  else if (isa<Constant>(BinOp->getOperand(1)))
    ConstIdx = 1;
  else
    return nullptr;

  auto *ConstOp = cast<Constant>(BinOp->getOperand(ConstIdx));
  if (!isa<ConstantInt>(ConstOp) &&
      (!ConstOp->getType()->isVectorTy() || !ConstOp->getSplatValue()))
    return nullptr;

  Value *NewOffsets = BinOp->getOperand(1 - ConstIdx);

  auto Imm = ConstOp->getUniqueInteger();
  auto NewScale(Scale);
  auto NewOffset(Offset);
  bool Overflow = false;

  const auto Opcode = BinOp->getOpcode();

  switch (Opcode) {
  default:
    return nullptr;
  case Instruction::Add:
  case Instruction::Sub:
    if (Imm.getSignificantBits() > Offset.getBitWidth())
      return nullptr;
    Imm = Imm.sextOrTrunc(Offset.getBitWidth())
              .smul_ov(Scale.zext(Offset.getBitWidth()), Overflow);
    if (Overflow)
      return nullptr;
    if (Opcode == Instruction::Add)
      NewOffset = Offset.sadd_ov(Imm, Overflow);
    else if (Opcode == Instruction::Sub)
      NewOffset = Offset.ssub_ov(Imm, Overflow);
    break;
  case Instruction::Mul:
    if (ST->getLSCScaleMax() == 1)
      return nullptr;
    if (!Imm.isIntN(Scale.getBitWidth()))
      return nullptr;
    if (Imm.getBitWidth() > Scale.getBitWidth())
      Imm = Imm.trunc(Scale.getBitWidth());
    NewScale = Scale.umul_ov(Imm, Overflow);
    break;
  case Instruction::Shl:
    if (ST->getLSCScaleMax() == 1)
      return nullptr;
    NewScale = Scale.ushl_ov(Imm, Overflow);
    break;
  }

  if (Overflow)
    return nullptr;

  if (auto NewOffsetVal = NewOffset.getSExtValue();
      NewOffsetVal % OffsetAlignment != 0)
    return nullptr;

  if (auto NewScaleVal = NewScale.getZExtValue();
      NewScaleVal != AllowedScale && NewScaleVal != 1)
    return nullptr;

  Scale = std::move(NewScale);
  Offset = std::move(NewOffset);

  return NewOffsets;
}

Value *GenXLscAddrCalcFolding::applyLscAddrConstFolding(Value *Offsets,
                                                        APInt &Scale,
                                                        APInt &Offset) {
  IGC_ASSERT(ST->hasLSCMessages() && ST->hasLSCOffset());

  auto *Const = cast<ConstantDataVector>(Offsets);
  if (ST->getLSCScaleMax() == 1)
    return nullptr;

  auto *Ty = Const->getType()->getElementType();
  if (!Ty->isIntegerTy(32))
    return nullptr;
  if (Const->getSplatValue() != nullptr)
    return nullptr;

  // Try to fold 32-bit constant indices
  uint32_t Mask = 0;
  int32_t Min = std::numeric_limits<int32_t>::max();

  SmallVector<int32_t, 32> Values;
  for (unsigned I = 0; I < Const->getType()->getNumElements(); I++) {
    auto V = Const->getAggregateElement(I)->getUniqueInteger().getSExtValue();
    Values.push_back(V);
    Mask |= V;
    if (V < Min)
      Min = V;
  }

  auto Shift = IGCLLVM::countr_zero(Mask);
  IGC_ASSERT(Shift < 32);

  bool Overflow = false;

  APInt Imm(32, Min);
  Imm = Imm.smul_ov(Scale.sext(32), Overflow);
  if (Overflow)
    return nullptr;

  auto NewOffset = Offset.sadd_ov(Imm, Overflow);
  if (Overflow)
    return nullptr;

  Offset = std::move(NewOffset);

  auto NewScale = Scale.ushl_ov(APInt(16, Shift), Overflow);
  if (Overflow || NewScale.getZExtValue() > ST->getLSCScaleMax())
    Shift = 0;
  else
    Scale = std::move(NewScale);

  if (Min == 0 && Shift == 0)
    return nullptr;

  SmallVector<Constant *, 32> NewOffsets;
  transform(Values, std::back_inserter(NewOffsets), [&](uint32_t V) {
    return ConstantInt::getSigned(Ty, (V - Min) >> Shift);
  });

  return ConstantVector::get(NewOffsets);
}

bool GenXLscAddrCalcFolding::foldLscAddrExtend(CallInst &Inst) {
  IGC_ASSERT(vc::InternalIntrinsic::isInternalMemoryIntrinsic(&Inst));
  constexpr unsigned AddrIdx = 6;

  auto *Addr = Inst.getArgOperand(AddrIdx);
  auto *Ty = Addr->getType();
  if (!Ty->isIntOrIntVectorTy(64))
    return false;

  IRBuilder<> Builder(&Inst);

  auto NewAddrSize = LSC_ADDR_SIZE_INVALID;
  Value *NewAddr = nullptr;
  if (auto *Const = dyn_cast<ConstantDataVector>(Addr)) {
    // Try to fold 32-bit constant indices
    SmallVector<Constant *, 32> Values;
    for (unsigned I = 0; I < Const->getType()->getNumElements(); I++) {
      auto V = Const->getAggregateElement(I)->getUniqueInteger();
      if (V.getMinSignedBits() > 32)
        return false;
      Values.push_back(Builder.getInt32(V.getSExtValue()));
    }
    NewAddrSize = LSC_ADDR_SIZE_32bS;
    NewAddr = ConstantVector::get(Values);
  } else {
    if (isa<SExtInst>(Addr))
      NewAddrSize = LSC_ADDR_SIZE_32bS;
    else if (isa<ZExtInst>(Addr))
      NewAddrSize = LSC_ADDR_SIZE_32bU;
    else
      return false;

    auto *Cast = cast<CastInst>(Addr);
    NewAddr = Cast->getOperand(0);
  }

  auto *NewTy = NewAddr->getType();
  if (!NewTy->isIntOrIntVectorTy(32))
    return false;

  auto IID = vc::getAnyIntrinsicID(&Inst);
  unsigned AddrSizeIdx = IID == vc::InternalIntrinsic::lsc_atomic_ugm ? 2 : 1;

  SmallVector<Type *, 3> Types;
  if (vc::InternalIntrinsic::isOverloadedRet(IID))
    Types.push_back(Inst.getType());

  for (unsigned I = 0; I < IGCLLVM::getNumArgOperands(&Inst); I++)
    if (vc::InternalIntrinsic::isOverloadedArg(IID, I))
      Types.push_back(I == AddrIdx ? NewTy : Inst.getArgOperand(I)->getType());

  auto *NewFunc = vc::getAnyDeclaration(Inst.getModule(), IID, Types);

  LLVM_DEBUG(dbgs() << "Folding LSC address extend for instruction: " << Inst
                    << "\n");
  Inst.setArgOperand(AddrSizeIdx, Builder.getInt8(NewAddrSize));
  Inst.setArgOperand(AddrIdx, NewAddr);
  Inst.setCalledFunction(NewFunc);
  LLVM_DEBUG(dbgs() << "Updated instruction: " << Inst << "\n");

  return true;
}

static Value *getBaseCandidate(Value *V) {
  if (isa<PtrToIntInst>(V))
    return V;

  // Check for splat shuffle
  if (auto *Shuffle = dyn_cast<ShuffleVectorInst>(V);
      Shuffle && Shuffle->isZeroEltSplat()) {
    auto *Src = Shuffle->getOperand(0);
    if (auto *Ins = dyn_cast<InsertElementInst>(Src)) {
      auto *Index = dyn_cast<ConstantInt>(Ins->getOperand(2));
      if (Index && Index->isZeroValue())
        return Ins->getOperand(1);
    }

    IRBuilder<> Builder(Shuffle);
    return Builder.CreateExtractElement(Src, Builder.getInt32(0));
  }

  if (!GenXIntrinsic::isRdRegion(V))
    return nullptr;

  auto *RdRegion = cast<CallInst>(V);
  auto *Src = RdRegion->getArgOperand(0);
  auto *VStride = cast<ConstantInt>(RdRegion->getArgOperand(1));
  auto *Stride = cast<ConstantInt>(RdRegion->getArgOperand(3));
  auto *Offset = RdRegion->getArgOperand(4);
  auto *OrigWidth = RdRegion->getArgOperand(5);
  auto *OffsetTy = Offset->getType();

  // Not a splat
  if (VStride->getZExtValue() != 0 || Stride->getZExtValue() != 0 ||
      !OffsetTy->isIntegerTy(16))
    return nullptr;

  auto *VTy = cast<IGCLLVM::FixedVectorType>(Src->getType());
  auto *ETy = VTy->getElementType();
  if (!ETy->isIntegerTy(64) && !ETy->isIntegerTy(32))
    return nullptr;

  IRBuilder<> Builder(RdRegion);
  auto *Cast = dyn_cast<BitCastInst>(Src);
  if (Cast &&
      (Cast->getSrcTy()->isIntegerTy(64) || Cast->getSrcTy()->isIntegerTy(32)))
    return Cast->getOperand(0);

  if (VTy->getNumElements() != 1) {
    auto IID = vc::getAnyIntrinsicID(RdRegion);
    auto *Func = vc::getAnyDeclaration(
        RdRegion->getModule(), IID,
        {VTy, IGCLLVM::FixedVectorType::get(ETy, 1), OffsetTy});
    Src = Builder.CreateCall(
        Func, {Src, VStride, Builder.getInt32(1), Stride, Offset, OrigWidth});
  }

  return Builder.CreateBitCast(Src, ETy);
}

bool GenXLscAddrCalcFolding::foldLscAddrBase(CallInst &Inst) {
  IGC_ASSERT(vc::InternalIntrinsic::isInternalMemoryIntrinsic(&Inst));
  constexpr unsigned BaseIdx = 5, AddrIdx = 6, ScaleIdx = 7, OffsetIndex = 8;

  // Check that base is not folded yet
  auto *OldBase = dyn_cast<ConstantInt>(Inst.getArgOperand(BaseIdx));
  if (!OldBase || OldBase->getZExtValue() != 0)
    return false;

  // Check that scale is not folded yet
  auto *Scale = cast<ConstantInt>(Inst.getArgOperand(ScaleIdx));
  if (Scale->getZExtValue() != 1)
    return false;

  // Check that address operand is an add instruction
  auto *Addr = dyn_cast<BinaryOperator>(Inst.getArgOperand(AddrIdx));
  if (!Addr || Addr->getOpcode() != Instruction::Add)
    return false;

  // Try to find scalar base
  auto *Base = Addr->getOperand(0);
  auto *Index = Addr->getOperand(1);

  if (auto *V = getBaseCandidate(Base)) {
    Base = V;
  } else {
    std::swap(Base, Index);
    Base = getBaseCandidate(Base);
  }

  if (!Base)
    return false;

  auto Offset = cast<ConstantInt>(Inst.getOperand(OffsetIndex))->getValue();
  if (ST->hasLSCOffset()) {
    auto *NewBase = Base;
    auto NewScale = Scale->getValue();
    auto NewOffset = Offset;
    for (; NewBase && NewScale.getZExtValue() == 1;
         NewBase = applyLscAddrFolding(Base, NewScale, NewOffset, &Inst)) {
      Base = NewBase;
      Offset = NewOffset;
      LLVM_DEBUG(dbgs() << "LSC address base folding found, base: " << *Base
                        << ", offset: " << Offset.getSExtValue() << "\n");
    }
  }

  LLVM_DEBUG(dbgs() << "Folding LSC base address for instruction: " << Inst
                    << "\n");
  IRBuilder<> Builder(&Inst);
  Inst.setArgOperand(BaseIdx, Base);
  Inst.setArgOperand(AddrIdx, Index);
  Inst.setArgOperand(OffsetIndex, Builder.getInt32(Offset.getSExtValue()));
  LLVM_DEBUG(dbgs() << "Updated instruction: " << Inst << "\n");

  return true;
}
