/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"

#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/InitializePasses.h"

#define DEBUG_TYPE "GENX_FLOAT_CONTROL"

namespace llvm {

class GenXFloatControl : public FGPassImplInterface, IDMixin<GenXFloatControl> {

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

  uint32_t DefaultFloatControl = 0;
  uint32_t Mask = 0;

  bool getFloatControl(Function *F, uint32_t *Val);

  Value *buildCr0Update(Value *V, Instruction *InsertBefore);

public:
  void releaseMemory() {}
  void print(raw_ostream &OS, const FunctionGroup *FG) const {}
  bool runOnFunctionGroup(FunctionGroup &FG);
  void verifyAnalysis() const {}

  static void getAnalysisUsage(AnalysisUsage &AU) {
    AU.addRequired<TargetPassConfig>();
    AU.setPreservesAll();
  }
  static StringRef getPassName() { return "GenXFloatControl"; }
};

void initializeGenXFloatControlWrapperPass(PassRegistry &);

using GenXFloatControlWrapper = FunctionGroupWrapperPass<GenXFloatControl>;

} // namespace llvm

using namespace llvm;
using namespace genx;

INITIALIZE_PASS_BEGIN(GenXFloatControlWrapper, "GenXFloatControlWrapper",
                      "GenXFloatControlWrapper", false, false)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(GenXFloatControlWrapper, "GenXFloatControlWrapper",
                    "GenXFloatControlWrapper", false, false)

ModulePass *llvm::createGenXFloatControlWrapperPass() {
  initializeGenXFloatControlWrapperPass(*PassRegistry::getPassRegistry());
  return new GenXFloatControlWrapper();
}

bool GenXFloatControl::runOnFunctionGroup(FunctionGroup &FG) {
  // By default retain all denormals, rounding mode - RTNE (both FPU and float
  // to integer conversion)
  DefaultFloatControl = Bits::DoublePrecisionDenorm |
                        Bits::SinglePrecisionDenorm |
                        Bits::HalfPrecisionDenorm | Bits::FPToIntRoundingMode;

  // By default allow to specify with the attribute only rounding and denorm
  // modes
  Mask = Bits::RoundingMode | Bits::DoublePrecisionDenorm |
         Bits::SinglePrecisionDenorm | Bits::HalfPrecisionDenorm;
  const auto *Subtarget = &getAnalysis<TargetPassConfig>()
                               .getTM<GenXTargetMachine>()
                               .getGenXSubtarget();
  if (Subtarget->hasSystolicDenormControl()) {
    DefaultFloatControl |= Bits::SystolicDenorm;
    Mask |= Bits::SystolicDenorm;
  }

  for (auto It = FG.begin(); It != FG.end(); It++) {
    Function *F = *It;
    uint32_t FloatControl = DefaultFloatControl;
    if (!getFloatControl(F, &FloatControl) && !fg::isHead(*F))
      continue;
    // Kernels, stackcalls and subroutines with attribute set float control on
    // entry - provided by the attribute or the default one
    auto *V = ConstantInt::get(Type::getInt32Ty(F->getContext()), FloatControl);
    auto *OldV = buildCr0Update(V, F->getEntryBlock().getFirstNonPHI());
    if (fg::isGroupHead(*F))
      continue;
    // Stackcalls and subroutines with attribute must save caller's float
    // control on entry and restore it before return
    for (auto &BB : *F)
      if (auto *RI = dyn_cast<ReturnInst>(BB.getTerminator()))
        buildCr0Update(OldV, RI);
  }

  return true;
}

bool GenXFloatControl::getFloatControl(Function *F, uint32_t *Val) {
  if (!F->hasFnAttribute(FunctionMD::CMFloatControl))
    return false;
  F->getFnAttribute(FunctionMD::CMFloatControl)
      .getValueAsString()
      .getAsInteger(0, *Val);
  return true;
}

Value *GenXFloatControl::buildCr0Update(Value *V, Instruction *InsertBefore) {
  IRBuilder<> B(InsertBefore);
  auto &DL = InsertBefore->getDebugLoc();
  auto *M = InsertBefore->getModule();

  auto *Ty = B.getInt32Ty();
  auto *VTy = IGCLLVM::FixedVectorType::get(Ty, 4);
  auto *Id = B.getInt32(PreDefined_Vars::PREDEFINED_CR0);

  auto *Cr0 = B.CreateCall(
      vc::getAnyDeclaration(M, GenXIntrinsic::genx_read_predef_reg, {VTy, VTy}),
      {Id, UndefValue::get(VTy)});

  Region R(Ty);

  auto *RdR = R.createRdRegion(Cr0, "cr0.0", InsertBefore, DL, true);
  auto *AndOld = B.CreateAnd(RdR, ~Mask);
  auto *AndNew = B.CreateAnd(V, Mask);
  auto *Or = B.CreateOr(AndOld, AndNew);
  auto *WrR = R.createWrRegion(Cr0, Or, "cr0.0.updated", InsertBefore, DL);

  B.CreateCall(vc::getAnyDeclaration(M, GenXIntrinsic::genx_write_predef_reg,
                                     {VTy, VTy}),
               {Id, WrR});

  return RdR;
}
