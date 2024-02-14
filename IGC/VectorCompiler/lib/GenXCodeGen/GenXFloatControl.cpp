/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenX.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"

#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/InitializePasses.h"

#define DEBUG_TYPE "GENX_FLOAT_CONTROL"

namespace llvm {

class GenXFloatControl : public FunctionPass {
  // Must be up to date with CR0 layout
  enum Bits {
    SinglePrecisionMode = 1,
    RoundingMode = 3 << 4,
    DoublePrecisionDenorm = 1 << 6,
    SinglePrecisionDenorm = 1 << 7,
    HalfPrecisionDenorm = 1 << 10,
    FPToIntRoundingMode = 1 << 12,
    SystolicDenorm = 1 << 30,
  };

  uint32_t Mask = 0;

  bool getFloatControl(Function &F, uint32_t *Val);
  Value *buildCr0Update(Value *V, Instruction *InsertBefore, bool ApplyMask);

public:
  static char ID;

  explicit GenXFloatControl() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<TargetPassConfig>();
    AU.setPreservesAll();
  }
  StringRef getPassName() const override { return "GenXFloatControl"; }
};

void initializeGenXFloatControlPass(PassRegistry &);

} // namespace llvm

using namespace llvm;
using namespace genx;

char GenXFloatControl::ID = 0;

INITIALIZE_PASS_BEGIN(GenXFloatControl, "GenXFloatControl", "GenXFloatControl",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(GenXFloatControl, "GenXFloatControl", "GenXFloatControl",
                    false, false)

FunctionPass *llvm::createGenXFloatControlPass() {
  initializeGenXFloatControlPass(*PassRegistry::getPassRegistry());
  return new GenXFloatControl();
}

bool GenXFloatControl::runOnFunction(Function &F) {
  // By default allow to specify with the attribute only
  // rounding and denorm modes
  Mask = Bits::RoundingMode | Bits::DoublePrecisionDenorm |
         Bits::SinglePrecisionDenorm | Bits::HalfPrecisionDenorm;

  const auto *Subtarget = &getAnalysis<TargetPassConfig>()
                               .getTM<GenXTargetMachine>()
                               .getGenXSubtarget();
  if (Subtarget->hasSystolicDenormControl()) {
    Mask |= Bits::SystolicDenorm;
  }
  uint32_t FloatControl = 0;
  if (!getFloatControl(F, &FloatControl) && !fg::isHead(F))
    return false;
  // Kernels, stackcalls and subroutines with attribute set float control on
  // entry - provided by the attribute or the default one
  auto *V = ConstantInt::get(Type::getInt32Ty(F.getContext()), FloatControl);
  auto *OldV = buildCr0Update(V, F.getEntryBlock().getFirstNonPHI(), true);
  if (fg::isGroupHead(F))
    return true;
  // Stackcalls and subroutines with attribute must save caller's float
  // control on entry and restore it before return
  for (auto &BB : F)
    if (auto *RI = dyn_cast<ReturnInst>(BB.getTerminator()))
      buildCr0Update(OldV, RI, false);
  return true;
}

bool GenXFloatControl::getFloatControl(Function &F, uint32_t *Val) {
  if (!F.hasFnAttribute(FunctionMD::CMFloatControl))
    return false;
  F.getFnAttribute(FunctionMD::CMFloatControl)
      .getValueAsString()
      .getAsInteger(0, *Val);
  return true;
}

Value *GenXFloatControl::buildCr0Update(Value *V, Instruction *InsertBefore,
                                        bool ApplyMask) {
  IRBuilder<> B(InsertBefore);
  auto &DL = InsertBefore->getDebugLoc();
  auto *M = InsertBefore->getModule();
  auto *Ty = B.getInt32Ty();
  auto *VTy = IGCLLVM::FixedVectorType::get(Ty, 4);
  auto *Id = B.getInt32(PreDefined_Vars::PREDEFINED_CR0);
  auto *OldCr0 = B.CreateCall(
      vc::getAnyDeclaration(M, GenXIntrinsic::genx_read_predef_reg, {VTy, VTy}),
      {Id, UndefValue::get(VTy)});
  Region R(Ty);
  Value *OldV = nullptr;
  auto *NewV = V;
  if (ApplyMask) {
    OldV = R.createRdRegion(OldCr0, "cr0.0", InsertBefore, DL, true);
    NewV = B.CreateOr(B.CreateAnd(OldV, ~Mask), B.CreateAnd(V, Mask));
  }
  auto *NewCr0 =
      R.createWrRegion(OldCr0, NewV, "cr0.updated", InsertBefore, DL);
  B.CreateCall(vc::getAnyDeclaration(M, GenXIntrinsic::genx_write_predef_reg,
                                     {VTy, VTy}),
               {Id, NewCr0});
  return OldV;
}
