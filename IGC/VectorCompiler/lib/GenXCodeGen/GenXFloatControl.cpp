/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenX.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"
#include "GenXVisa.h"

#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/InitializePasses.h"

#define DEBUG_TYPE "GENX_FLOAT_CONTROL"

namespace llvm {

class GenXFloatControl : public FunctionPass {
  uint32_t Mask = 0;

  bool getFloatControl(Function &F, uint32_t *Val);
  Value *buildCr0Update(uint32_t Value, Instruction *InsertBefore);
  void buildCr0Write(Value *V, Instruction *InsertBefore);

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
using namespace visa;

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
  Mask = CRBits::RoundingBitMask | CRBits::DoublePrecisionDenorm |
         CRBits::SinglePrecisionDenorm | CRBits::HalfPrecisionDenorm;
  // Default float control:
  //   rounding mode = nearest even
  //   denormals = flush
  uint32_t FloatControl = CRBits::RTNE;
  const auto *Subtarget = &getAnalysis<TargetPassConfig>()
                               .getTM<GenXTargetMachine>()
                               .getGenXSubtarget();
  if (!getFloatControl(F, &FloatControl) && !fg::isHead(F))
    return false;
  if (Subtarget->hasSystolicDenormControl()) {
    // Always retain denormals in systolic operations
    Mask |= CRBits::SystolicDenorm;
    FloatControl |= CRBits::SystolicDenorm;
  }
  // On kernel entry cr0 is set to zero, so in case of zero float control
  // we don't need to update it
  if (FloatControl == 0 && fg::isGroupHead(F))
    return false;
  // Kernels, stackcalls and subroutines with attribute set float control on
  // entry - provided by the attribute or the default one
  auto *OldV = buildCr0Update(FloatControl, F.getEntryBlock().getFirstNonPHI());
  if (fg::isGroupHead(F))
    return true;
  // Stackcalls and subroutines with attribute must save caller's float
  // control on entry and restore it before return
  for (auto &BB : F)
    if (auto *RI = dyn_cast<ReturnInst>(BB.getTerminator()))
      buildCr0Write(OldV, RI);
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

Value *GenXFloatControl::buildCr0Update(uint32_t Value, Instruction *InsertBefore) {
  IRBuilder<> B(InsertBefore);
  auto &DL = InsertBefore->getDebugLoc();
  auto *M = InsertBefore->getModule();
  auto *Ty = B.getInt32Ty();
  auto *VTy = IGCLLVM::FixedVectorType::get(Ty, 4);
  auto *Id = B.getInt32(PreDefined_Vars::PREDEFINED_CR0);
  Region R(Ty);
  auto *ReadPredefDecl = vc::getAnyDeclaration(M, GenXIntrinsic::genx_read_predef_reg, {VTy, VTy});
  auto *WritePredefDecl = vc::getAnyDeclaration(M, GenXIntrinsic::genx_write_predef_reg, {VTy, VTy});
  auto *AndReadPredef = B.CreateCall(ReadPredefDecl, {Id, UndefValue::get(VTy)});
  auto *AndRdRegion = R.createRdRegion(AndReadPredef, "", InsertBefore, DL, true);
  auto *And = B.CreateAnd(AndRdRegion, ~Mask);
  auto *AndWrRegion = R.createWrRegion(AndReadPredef, And, "", InsertBefore, DL);
  B.CreateCall(WritePredefDecl, {Id, AndWrRegion});
  auto *OrReadPredef = B.CreateCall(ReadPredefDecl, {Id, UndefValue::get(VTy)});
  auto *OrRdRegion = R.createRdRegion(OrReadPredef, "", InsertBefore, DL, true);
  auto *Or = B.CreateOr(OrRdRegion, Value & Mask);
  auto *OrWrRegion = R.createWrRegion(OrReadPredef, Or, "", InsertBefore, DL);
  B.CreateCall(WritePredefDecl, {Id, OrWrRegion});
  return AndRdRegion;
}

void GenXFloatControl::buildCr0Write(Value* V, Instruction *InsertBefore) {
  IRBuilder<> B(InsertBefore);
  auto &DL = InsertBefore->getDebugLoc();
  auto *M = InsertBefore->getModule();
  auto *Ty = B.getInt32Ty();
  auto *VTy = IGCLLVM::FixedVectorType::get(Ty, 4);
  auto *Id = B.getInt32(PreDefined_Vars::PREDEFINED_CR0);
  Region R(Ty);
  auto *ReadPredefDecl = vc::getAnyDeclaration(M, GenXIntrinsic::genx_read_predef_reg, {VTy, VTy});
  auto *WritePredefDecl = vc::getAnyDeclaration(M, GenXIntrinsic::genx_write_predef_reg, {VTy, VTy});
  auto *ReadPredef = B.CreateCall(ReadPredefDecl, {Id, UndefValue::get(VTy)});
  auto *WrRegion = R.createWrRegion(ReadPredef, V, "", InsertBefore, DL);
  B.CreateCall(WritePredefDecl, {Id, WrRegion});
}

