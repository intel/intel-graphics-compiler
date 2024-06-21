/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"

#include "vc/Utils/GenX/Intrinsics.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"

#include "llvmWrapper/IR/Instructions.h"

#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/InstVisitor.h"
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

  Value *applyLscAddrFolding(Value *Offsets, APInt &Scale, APInt &Offset);

  const GenXSubtarget *ST = nullptr;

  bool Changed = false;
};

} // namespace

char GenXLscAddrCalcFolding::ID = 0;

namespace llvm {
void initializeGenXLscAddrCalcFoldingPass(PassRegistry &);
} // namespace llvm

INITIALIZE_PASS_BEGIN(GenXLscAddrCalcFolding, "GenXLscAddrCalcFolding",
                      "GenXLscAddrCalcFolding", false, false)
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
  }
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

  while (auto *NewIndex = applyLscAddrFolding(Index, Scale, Offset)) {
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
    Inst.setOperand(AddrIndex, Index);
    Inst.setOperand(ScaleIndex, Builder.getInt16(Scale.getZExtValue()));
    Inst.setOperand(OffsetIndex, Builder.getInt32(Offset.getZExtValue()));
    LLVM_DEBUG(dbgs() << "Updated instruction: " << Inst << "\n");
  }

  return Changed;
}

// applyLscAddrFolding : fold address calculation of LSC intriniscs
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
                                                   APInt &Offset) {
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

  Scale = std::move(NewScale);
  Offset = std::move(NewOffset);

  return BinOp->getOperand(1 - ConstIdx);
}
