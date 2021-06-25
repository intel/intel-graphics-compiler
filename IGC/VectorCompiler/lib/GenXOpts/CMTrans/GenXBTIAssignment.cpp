/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
/// GenXAssignBTI
/// -----------------
///
/// This pass calculates BT indices for kernel memory object arguments
/// that include buffers and images.
///
/// Calculated BTI are then used instead of corresponging kernel arguments
/// throughout the code.
///
//===----------------------------------------------------------------------===//

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/GenXOpts/Utils/KernelInfo.h"
#include "vc/Support/BackendConfig.h"

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"

#include "Probe/Assertion.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

using namespace llvm;

namespace {
class BTIAssignment final {
  Module &M;

public:
  BTIAssignment(Module &InM) : M(InM) {}

  bool run();

private:
  bool processKernel(Function &F);
};

class GenXBTIAssignment final : public ModulePass {
public:
  static char ID;

  GenXBTIAssignment() : ModulePass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<GenXBackendConfig>();
  }

  StringRef getPassName() const override { return "GenX BTI Assignment"; }

  bool runOnModule(Module &M) override;
};
} // namespace

char GenXBTIAssignment::ID = 0;

INITIALIZE_PASS_BEGIN(GenXBTIAssignment, "GenXBTIAssignment",
                      "GenXBTIAssignment", false, false);
INITIALIZE_PASS_END(GenXBTIAssignment, "GenXBTIAssignment", "GenXBTIAssignment",
                    false, false);

namespace llvm {
ModulePass *createGenXBTIAssignmentPass() {
  initializeGenXBTIAssignmentPass(*PassRegistry::getPassRegistry());
  return new GenXBTIAssignment();
}
} // namespace llvm

bool GenXBTIAssignment::runOnModule(Module &M) {
  BTIAssignment BA(M);

  return BA.run();
}

bool BTIAssignment::processKernel(Function &F) {
  genx::KernelMetadata KM{&F};

  bool Changed = false;

  auto *I32Ty = Type::getInt32Ty(M.getContext());

  IRBuilder<> IRB{&F.front().front()};

  auto ArgKinds = KM.getArgKinds();
  IGC_ASSERT_MESSAGE(ArgKinds.size() == F.arg_size(),
                     "Inconsistent arg kinds metadata");
  for (auto &&[Arg, Kind] : llvm::zip(F.args(), ArgKinds)) {
    if (Kind != genx::KernelMetadata::AK_SAMPLER &&
        Kind != genx::KernelMetadata::AK_SURFACE)
      continue;

    int32_t BTI = KM.getBTI(Arg.getArgNo());
    IGC_ASSERT_MESSAGE(BTI >= 0, "unassigned BTI");

    Value *BTIConstant = ConstantInt::get(I32Ty, BTI);

    Type *ArgTy = Arg.getType();
    // This code is to handle DPC++ contexts with correct OCL types.
    // Without actually doing something with users of args, we just
    // cast constant to pointer and replace arg with new value.
    // Later passes will do their work and clean up the mess.
    // FIXME(aus): proper unification of incoming IR is
    // required. Current approach will constantly blow all passes
    // where some additional case should be handled.
    if (ArgTy->isPointerTy())
      BTIConstant = IRB.CreateIntToPtr(BTIConstant, ArgTy, ".bti.cast");

    IGC_ASSERT_MESSAGE(ArgTy == BTIConstant->getType(),
                       "Only explicit i32 indices or opaque types are allowed "
                       "as bti argument");

    Arg.replaceAllUsesWith(BTIConstant);
    Changed = true;
  }

  return Changed;
}

bool BTIAssignment::run() {
  NamedMDNode *KernelsMD = M.getNamedMetadata(genx::FunctionMD::GenXKernels);
  // There can be no kernels in module.
  if (!KernelsMD)
    return false;

  bool Changed = false;
  for (MDNode *Kernel : KernelsMD->operands()) {
    Metadata *FuncRef = Kernel->getOperand(genx::KernelMDOp::FunctionRef);
    Function *F = cast<Function>(cast<ValueAsMetadata>(FuncRef)->getValue());
    Changed |= processKernel(*F);
  }

  return Changed;
}
