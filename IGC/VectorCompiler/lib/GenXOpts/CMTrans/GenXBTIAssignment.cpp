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
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>

using namespace llvm;

static cl::opt<bool> BTIIndexCommoning("make-bti-index-common", cl::init(false),
                                       cl::Hidden,
                                       cl::desc("Enable BTI index commoning"));

namespace {
class BTIAssignment final {
  Module &M;

public:
  BTIAssignment(Module &InM) : M(InM) {}

  bool run();
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

static bool processKernel(Function &F) {
  genx::KernelMetadata KM{&F};

  bool Changed = false;

  unsigned Idx = 0;
  auto ArgKinds = KM.getArgKinds();
  auto Kind = ArgKinds.begin();
  for (auto &Arg : F.args()) {
    if (*Kind == genx::KernelMetadata::AK_SAMPLER ||
        *Kind == genx::KernelMetadata::AK_SURFACE) {
      int32_t BTI = KM.getBTI(Idx);
      IGC_ASSERT_MESSAGE(BTI >= 0, "unassigned BTI");

      Type *ArgTy = Arg.getType();
      if (ArgTy->isPointerTy()) {
        SmallVector<Instruction *, 8> ToErase;
        for (Use &U : Arg.uses()) {
          auto ArgUse = U.getUser();
          IGC_ASSERT_MESSAGE(isa<PtrToIntInst>(ArgUse),
                             "invalid surface input usage");

          std::transform(ArgUse->user_begin(), ArgUse->user_end(),
                         std::back_inserter(ToErase), [BTI](User *U) {
                           U->replaceAllUsesWith(
                               ConstantInt::get(U->getType(), BTI));
                           return cast<Instruction>(U);
                         });
          ToErase.push_back(cast<Instruction>(ArgUse));
        }
        std::for_each(ToErase.begin(), ToErase.end(),
                      [](Instruction *I) { I->eraseFromParent(); });

      } else {
        auto BTIConstant = ConstantInt::get(ArgTy, BTI);
        // If the number of uses for this arg more than 1 it's better to
        // create a separate instruction for the constant. Otherwise, category
        // conversion will create a separate ".categoryconv" instruction for
        // each use of the arg. As a result, each conversion instruction will
        // be materialized as movs to a surface var. This will increase
        // register pressure and the number of instructions. But if there is
        // only one use, it will be okay to wait for the replacement until
        // category conv do it.
        if (BTIIndexCommoning && Arg.getNumUses() > 1) {
          auto ID = ArgTy->isFPOrFPVectorTy() ? GenXIntrinsic::genx_constantf
                                              : GenXIntrinsic::genx_constanti;
          Module *M = F.getParent();
          Function *Decl = GenXIntrinsic::getGenXDeclaration(M, ID, ArgTy);
          auto NewInst = CallInst::Create(
              Decl, BTIConstant, Arg.getName() + ".bti", &*F.begin()->begin());
          Arg.replaceAllUsesWith(NewInst);
        } else
          Arg.replaceAllUsesWith(BTIConstant);
      }
      Changed = true;
    }
    ++Kind, ++Idx;
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
