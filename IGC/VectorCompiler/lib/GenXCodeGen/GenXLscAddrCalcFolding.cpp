/*========================== begin_copyright_notice ============================

Copyright (C) 2024-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"

#include "vc/Utils/GenX/Intrinsics.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"

#include "llvmWrapper/IR/Instructions.h"

#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"

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
    AU.addRequired<TargetPassConfig>();
    AU.setPreservesCFG();
  }

  bool runOnFunction(Function &F) override;
  void visitCallInst(CallInst &CI);

private:
  bool foldLscAddrCalculation(CallInst &CI);

  Value *applyLscAddrFolding(Value *Offsets, APInt &Scale, APInt &Offset,
                             CallInst *Inst);

  static constexpr unsigned Block2DIndexX = 10;
  static constexpr unsigned Block2DIndexY = 11;
  static constexpr unsigned Block2DOffsetX = 12;
  static constexpr unsigned Block2DOffsetY = 13;

  bool foldLscBlock2DAddrCalculation(CallInst &CI, unsigned IndexArg,
                                     unsigned OffsetArg);

  const GenXSubtarget *ST = nullptr;

  unsigned Supported2DOffsetBits = 0;
  bool Changed = false;

  unsigned OffsetAlignment = 0;

};

} // namespace

char GenXLscAddrCalcFolding::ID = 0;

namespace llvm {
void initializeGenXLscAddrCalcFoldingPass(PassRegistry &);
} // namespace llvm

INITIALIZE_PASS_BEGIN(GenXLscAddrCalcFolding, "GenXLscAddrCalcFolding",
                      "GenXLscAddrCalcFolding", false, false)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(GenXLscAddrCalcFolding, "GenXLscAddrCalcFolding",
                    "GenXLscAddrCalcFolding", false, false)

FunctionPass *llvm::createGenXLscAddrCalcFoldingPass() {
  initializeGenXLscAddrCalcFoldingPass(*PassRegistry::getPassRegistry());
  return new GenXLscAddrCalcFolding();
}

bool GenXLscAddrCalcFolding::runOnFunction(Function &F) {
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
//
// This folding is done iteratively for chains of such operations.
//
Value *GenXLscAddrCalcFolding::applyLscAddrFolding(Value *Offsets, APInt &Scale,
                                                   APInt &Offset,
                                                   CallInst *Inst) {
  LLVM_DEBUG(dbgs() << "applyLscAddrFolding instruction: " << Inst << "\n");
  IGC_ASSERT(ST->hasLSCMessages() && ST->hasLSCOffset());

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
    if (Imm.getMinSignedBits() > Offset.getBitWidth())
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
  }

  if (Overflow)
    return nullptr;

  if (auto NewOffsetVal = NewOffset.getSExtValue();
      NewOffsetVal % OffsetAlignment != 0)
    return nullptr;

  Scale = std::move(NewScale);
  Offset = std::move(NewOffset);

  return BinOp->getOperand(1 - ConstIdx);
}
