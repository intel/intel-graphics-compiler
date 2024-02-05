/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXTypeLegalization
/// ---------------------------
///
/// GenXTypeLegalization is a function pass which legalizes instructions in a
/// term of types, e.g.:
/// from
///   %src = trunc i64 %in to i11
/// to
///   %src = trunc i64 %in to i16
///
//===----------------------------------------------------------------------===//

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Support/GenXDiagnostic.h"

#include "Probe/Assertion.h"

#include "llvmWrapper/IR/IRBuilder.h"

#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>

#define DEBUG_TYPE "GENX_TYPE_LEGALIZATION"

using namespace llvm;

namespace {

class GenXTypeLegalization final
    : public FunctionPass,
      public InstVisitor<GenXTypeLegalization, Value *> {
  const DataLayout *DL = nullptr;
  // Map from illegal value to the legalized one.
  DenseMap<Value *, Value *> ValueMap;

public:
  static char ID;
  GenXTypeLegalization() : FunctionPass(ID) {}
  StringRef getPassName() const override { return "GenX type legalization"; }
  bool runOnFunction(Function &F) override;

  // Visitors return the new inst if type of the incoming
  // instruction is invalid and nullptr otherwise.
  Value *visitInstruction(Instruction &I);
  Value *visitSwitchInst(SwitchInst &I);
  Value *visitSelectInst(SelectInst &I);
  Value *visitBinaryOperator(BinaryOperator &I);
  Value *visitCastInst(CastInst &I);

private:
  bool isLegalType(Type *Ty);
  bool isLegalInst(Instruction *I);

  Type *getLegalizedType(Type *Ty);
  Value *getLegalizedValue(Value *V);
  Value *getLegalizedValueIfNeeded(Value *V);
};

} // end namespace

char GenXTypeLegalization::ID = 0;
namespace llvm {
void initializeGenXTypeLegalizationPass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXTypeLegalization, "GenXTypeLegalization",
                      "GenXTypeLegalization", false, false)
INITIALIZE_PASS_END(GenXTypeLegalization, "GenXTypeLegalization",
                    "GenXTypeLegalization", false, false)
FunctionPass *llvm::createGenXTypeLegalizationPass() {
  initializeGenXTypeLegalizationPass(*PassRegistry::getPassRegistry());
  return new GenXTypeLegalization;
}

static Twine getLegalizedName(StringRef OldN) { return OldN + ".l"; }

// Only legalization of integer types is currently supported.
bool GenXTypeLegalization::isLegalType(Type *Ty) {
  if (!Ty->isIntOrIntVectorTy())
    return true;
  auto *ITy = cast<IntegerType>(Ty->getScalarType());
  // 1bit integers are correctly handled by the compiler.
  return DL->isLegalInteger(ITy->getBitWidth()) || ITy->isIntegerTy(1);
}

bool GenXTypeLegalization::isLegalInst(Instruction *I) {
  if (I->isCast()) {
    auto *CI = cast<CastInst>(I);
    return isLegalType(CI->getSrcTy()) && isLegalType(CI->getDestTy());
  }
  return llvm::all_of(I->operands(), [this](Use &U) {
    return isLegalType(U.get()->getType());
  });
}

Type *GenXTypeLegalization::getLegalizedType(Type *OldTy) {
  IGC_ASSERT(!isLegalType(OldTy));
  auto *NewTy = DL->getSmallestLegalIntType(OldTy->getContext(),
                                            DL->getTypeSizeInBits(OldTy));
  IGC_ASSERT(NewTy);
  return NewTy;
}

Value *GenXTypeLegalization::getLegalizedValue(Value *OldV) {
  if (auto *C = dyn_cast<Constant>(OldV)) {
    auto *NewCType = getLegalizedType(C->getType());
    // TODO: consider signess here.
    return ConstantExpr::getZExt(C, NewCType);
  }
  auto *NewV = ValueMap[OldV];
  // Instructions are visited in topological order so a record should exist.
  IGC_ASSERT(NewV);
  return NewV;
}

Value *GenXTypeLegalization::getLegalizedValueIfNeeded(Value *OldV) {
  return isLegalType(OldV->getType()) ? OldV : getLegalizedValue(OldV);
}

bool GenXTypeLegalization::runOnFunction(Function &F) {
  DL = &F.getParent()->getDataLayout();
  bool Changed = false;
  ValueMap.clear();

  // Create new legal PHIs and fill ValueMap to replace
  // the uses of illegal PHIs later.
  for (auto &BB : F) {
    for (auto &I : BB) {
      auto *PN = dyn_cast<PHINode>(&I);
      if (!PN)
        break;
      if (isLegalType(PN->getType()))
        continue;
      auto *NewTy = getLegalizedType(PN->getType());
      auto *NewPN = PHINode::Create(NewTy, PN->getNumIncomingValues(),
                                    getLegalizedName(PN->getName()), &I);
      ValueMap[&I] = NewPN;
      Changed = true;
    }
  }

  // Legalize instructions in a topological order so that the 'def'
  // is always legalized before the legalization of the 'use'.
  for (auto &BB : depth_first(&F)) {
    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
      auto *I = &(*BI);
      if (isa<PHINode>(I) || isLegalInst(I))
        continue;
      // CallInst is currently not supported.
      IGC_ASSERT(!isa<CallInst>(I));
      IGC_ASSERT(!I->getType()->isVectorTy());
      if (auto *NewVal = visit(*I))
        ValueMap[I] = NewVal;
      Changed = true;
    }
  }

  // Populate new PHIs.
  for (auto const &[Illegal, Legal] : ValueMap) {
    auto *const PN = dyn_cast<PHINode>(Illegal);
    if (!PN)
      continue;
    auto *const NewPN = cast<PHINode>(Legal);
    for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
      auto *NewVal = getLegalizedValue(PN->getIncomingValue(i));
      NewPN->addIncoming(NewVal, PN->getIncomingBlock(i));
    }
  }

  for (auto const &[Illegal, Legal] : ValueMap) {
    auto *const I = cast<Instruction>(Illegal);
    if (!Illegal->use_empty())
      I->replaceAllUsesWith(UndefValue::get(I->getType()));
    I->eraseFromParent();
  }
  return Changed;
}

Value *GenXTypeLegalization::visitInstruction(Instruction &I) {
  vc::fatal(I.getContext(), "GenXTypeLegalization",
            "Unimplemented legalization.", &I);
  return nullptr;
}

Value *GenXTypeLegalization::visitSwitchInst(SwitchInst &I) {
  llvm::for_each(I.cases(), [this](SwitchInst::CaseHandle Case) {
    auto *NewVal = cast<ConstantInt>(getLegalizedValue(Case.getCaseValue()));
    Case.setValue(NewVal);
  });
  auto *OldV = I.getCondition();
  auto *NewV = getLegalizedValue(OldV);

  IGCLLVM::IRBuilder<> Builder{&I};
  uint64_t Mask =
      (1ULL << OldV->getType()->getScalarType()->getIntegerBitWidth()) - 1;
  auto *NewCond = Builder.CreateAnd(NewV, Mask);
  I.setCondition(NewCond);
  return nullptr;
}

Value *GenXTypeLegalization::visitSelectInst(SelectInst &I) {
  auto *Src1 = getLegalizedValue(I.getOperand(1));
  auto *Src2 = getLegalizedValue(I.getOperand(2));

  IGCLLVM::IRBuilder<> Builder{&I};
  return Builder.CreateSelect(I.getOperand(0), Src1, Src2,
                              getLegalizedName(I.getName()));
}

Value *GenXTypeLegalization::visitBinaryOperator(BinaryOperator &I) {
  auto *Src0 = getLegalizedValue(I.getOperand(0));
  auto *Src1 = getLegalizedValue(I.getOperand(1));

  IGCLLVM::IRBuilder<> Builder{&I};
  return Builder.CreateBinOp(I.getOpcode(), Src0, Src1,
                             getLegalizedName(I.getName()));
}

Value *GenXTypeLegalization::visitCastInst(CastInst &I) {
  auto *Val = getLegalizedValueIfNeeded(I.getOperand(0));
  auto *DstTy = I.getType();
  if (isLegalType(DstTy)) {
    if (Val->getType() == I.getType())
      I.replaceAllUsesWith(Val);
    else
      I.setOperand(0, Val);
    return nullptr;
  }
  auto *NewDstTy = getLegalizedType(DstTy);

  auto DstBitWidth =
      cast<IntegerType>(NewDstTy->getScalarType())->getBitWidth();
  auto SrcBitWidth =
      cast<IntegerType>(Val->getType()->getScalarType())->getBitWidth();

  IGCLLVM::IRBuilder<> Builder{&I};
  switch (I.getOpcode()) {
  default:
    vc::fatal(I.getContext(), "GenXTypeLegalization", "Unhandled cast opcode.",
              &I);
    return nullptr;
  case Instruction::ZExt:
    IGC_ASSERT(SrcBitWidth <= DstBitWidth);
    return Builder.CreateZExt(Val, NewDstTy, getLegalizedName(I.getName()));
  case Instruction::Trunc: {
    IGC_ASSERT(DstBitWidth <= SrcBitWidth);
    auto *NewTrunc = Builder.CreateTrunc(Val, NewDstTy);
    uint64_t Mask =
        (1ULL << I.getType()->getScalarType()->getIntegerBitWidth()) - 1;
    return Builder.CreateAnd(NewTrunc, Mask, getLegalizedName(I.getName()));
  }
  }
}
