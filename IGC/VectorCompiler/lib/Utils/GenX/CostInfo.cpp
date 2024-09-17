/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/GenX/CostInfo.h"

#include "Probe/Assertion.h"

#include <llvm/IR/Constants.h>

using namespace llvm;
using namespace vc;

static constexpr auto VCLoopExprMetaKind = "vc.lce";

static ConstantAsMetadata *getFloatAsMetadata(float Val, LLVMContext &Ctx) {
  return ConstantAsMetadata::get(ConstantFP::get(Ctx, APFloat(Val)));
}

static ConstantAsMetadata *getIntAsMetadata(int Val, unsigned NumBits,
                                            LLVMContext &Ctx) {
  return ConstantAsMetadata::get(
      ConstantInt::get(Ctx, APInt(NumBits, Val, true)));
}

static float extractConstantFloatMD(const MDOperand &Op) {
  ConstantFP *V = nullptr;
  if (auto *VM = dyn_cast<ValueAsMetadata>(Op))
    V = dyn_cast<ConstantFP>(VM->getValue());
  IGC_ASSERT_MESSAGE(V, "Unexpected null value in metadata");
  return V->getValue().convertToFloat();
}

static int extractConstantIntMD(const MDOperand &Op) {
  ConstantInt *V = nullptr;
  if (auto *VM = dyn_cast<ValueAsMetadata>(Op))
    V = dyn_cast<ConstantInt>(VM->getValue());
  IGC_ASSERT_MESSAGE(V, "Unexpected null value in metadata");
  return V->getSExtValue();
}

bool vc::saveLCEToMetadata(const Loop &L, Module &M, LoopCountExpr LCE) {
  if (LCE.IsUndef)
    return false;

  auto *Latch = L.getLoopLatch();
  IGC_ASSERT(Latch);
  auto *TI = Latch->getTerminator();
  IGC_ASSERT(TI);

  auto &Ctx = TI->getContext();
  if (LCE.Factor == 0.0f) {
    auto *MD = getFloatAsMetadata(LCE.Addend, Ctx);
    TI->setMetadata(VCLoopExprMetaKind, MDNode::get(Ctx, MD));
    return true;
  }

  auto *SymNumMD = getIntAsMetadata(LCE.Symbol.Num, 32, Ctx);
  auto *SymOffMD = getIntAsMetadata(LCE.Symbol.Offset, 32, Ctx);
  auto *SymSizeMD = getIntAsMetadata(LCE.Symbol.Size, 32, Ctx);
  auto *SymIsIndirectMD = getIntAsMetadata(LCE.Symbol.IsIndirect, 1, Ctx);

  auto *SymMD =
      MDNode::get(Ctx, {SymNumMD, SymOffMD, SymSizeMD, SymIsIndirectMD});
  auto *FactorMD = getFloatAsMetadata(LCE.Factor, Ctx);
  auto *AddendMD = getFloatAsMetadata(LCE.Addend, Ctx);
  TI->setMetadata(VCLoopExprMetaKind,
                  MDNode::get(Ctx, {FactorMD, SymMD, AddendMD}));
  return true;
}

LoopCountExpr vc::restoreLCEFromMetadata(const llvm::Loop &L) {
  enum MDNumOperands { Constant = 1, Expression = 3, Argument = 4 };

  auto *Latch = L.getLoopLatch();
  if (!Latch)
    return LoopCountExpr{};
  auto *TI = Latch->getTerminator();
  IGC_ASSERT(TI);
  auto *ExprNode = TI->getMetadata(VCLoopExprMetaKind);
  if (!ExprNode)
    return LoopCountExpr{};

  LoopCountExpr LCE;
  LCE.IsUndef = false;
  if (ExprNode->getNumOperands() == MDNumOperands::Constant) {
    LCE.Addend = extractConstantFloatMD(ExprNode->getOperand(0));
    IGC_ASSERT(LCE.Addend);
    return LCE;
  }

  IGC_ASSERT(ExprNode->getNumOperands() == MDNumOperands::Expression);
  LCE.Factor = extractConstantFloatMD(ExprNode->getOperand(0));
  LCE.Addend = extractConstantFloatMD(ExprNode->getOperand(2));

  auto *ArgNode = dyn_cast<MDNode>(ExprNode->getOperand(1));
  IGC_ASSERT(ArgNode && ArgNode->getNumOperands() == MDNumOperands::Argument);
  LCE.Symbol.Num = extractConstantIntMD(ArgNode->getOperand(0));
  LCE.Symbol.Size = extractConstantIntMD(ArgNode->getOperand(1));
  LCE.Symbol.Offset = extractConstantIntMD(ArgNode->getOperand(2));
  LCE.Symbol.IsIndirect = extractConstantIntMD(ArgNode->getOperand(3));
  return LCE;
}
