/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"

#include "vc/Support/BackendConfig.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/General/BiF.h"

#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

using namespace llvm;

static constexpr const char *RoundingRtzSuffix = "__rtz_";
static constexpr const char *RoundingRteSuffix = "__rte_";
static constexpr const char *RoundingRtpSuffix = "__rtp_";
static constexpr const char *RoundingRtnSuffix = "__rtn_";

static constexpr int VCRoundingRTE = 0;
static constexpr int VCRoundingRTP = 1 << 4;
static constexpr int VCRoundingRTN = 2 << 4;
static constexpr int VCRoundingRTZ = 3 << 4;

template <typename T> static void processToEraseList(T &EraseList) {
  std::for_each(EraseList.begin(), EraseList.end(),
                [](auto *Item) { Item->eraseFromParent(); });
  EraseList.clear();
}

// The purpose of GenXBiFPrepare is to process an input module that is expected
// to represent the emulation library in the following manner:
// 1. Identify primary functions (the ones that are directly used for emulation)
// and mark those with vc::FunctionMD::VCEmulationRoutine and appropriate
// rounding attributes (derived from the name of such functions)
// 2. Purge those functions that are not required for the current Subtarget.
// This pass is expected to be run only in a special "BiFCompilation"
// mode (that is during BiF precompilation process).
class GenXBiFPrepare : public ModulePass {
public:
  static char ID;

  GenXBiFPrepare() : ModulePass(ID) {}
  StringRef getPassName() const override {
    return "GenX Emulation Module Prepare";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<TargetPassConfig>();
    AU.addRequired<GenXBackendConfig>();
  }

  bool runOnModule(Module &M) override;

private:
  static bool isLibraryFunction(const Function &F);
  static bool isNeededForTarget(const Function &F, const GenXSubtarget &ST);

  static void DeriveRoundingAttributes(Function &F) {
    const auto &Name = F.getName();
    if (Name.contains(RoundingRtzSuffix)) {
      F.addFnAttr(genx::FunctionMD::CMFloatControl,
                  std::to_string(VCRoundingRTZ));
      return;
    }
    if (Name.contains(RoundingRteSuffix)) {
      F.addFnAttr(genx::FunctionMD::CMFloatControl,
                  std::to_string(VCRoundingRTE));
      return;
    }
    if (Name.contains(RoundingRtpSuffix)) {
      F.addFnAttr(genx::FunctionMD::CMFloatControl,
                  std::to_string(VCRoundingRTP));
      return;
    }
    if (Name.contains(RoundingRtnSuffix)) {
      F.addFnAttr(genx::FunctionMD::CMFloatControl,
                  std::to_string(VCRoundingRTN));
      return;
    }
  }
};

bool GenXBiFPrepare::runOnModule(Module &M) {
  const GenXSubtarget &ST = getAnalysis<TargetPassConfig>()
                                .getTM<GenXTargetMachine>()
                                .getGenXSubtarget();
  std::vector<Function *> ToErase;
  for (auto &F : M.functions()) {
    DeriveRoundingAttributes(F);
    if (!isLibraryFunction(F))
      continue;
    F.addFnAttr(vc::FunctionMD::VCBuiltinFunction);

    if (!isNeededForTarget(F, ST))
      ToErase.push_back(&F);
  }

  for (auto *F : ToErase)
    F->eraseFromParent();

  return true;
}

bool GenXBiFPrepare::isLibraryFunction(const Function &F) {
  const auto &Name = F.getName();
  return Name.startswith(vc::LibraryFunctionPrefix);
}

bool GenXBiFPrepare::isNeededForTarget(const Function &F,
                                       const GenXSubtarget &ST) {
  auto Name = F.getName();

  // Not an exported function, assume it's needed
  if (!Name.consume_front(vc::LibraryFunctionPrefix))
    return true;

  bool IsDouble = F.getReturnType()->getScalarType()->isDoubleTy() ||
                  llvm::any_of(F.args(), [](const auto &Arg) {
                    return Arg.getType()->getScalarType()->isDoubleTy();
                  });
  if (IsDouble) {
    // Get rid of all double precision functions if target doesn't support fp64
    if (!ST.hasFP64())
      return false;

    // Get rid of double precision fdiv and fsqrt emulation functions if target
    // hw has native support
    if (!ST.emulateFDivFSqrt64() &&
        (Name.startswith("fdiv") || Name.startswith("fsqrt")))
      return false;
  }

  static SmallVector<StringRef, 4> IDivRem = {"udiv", "sdiv", "urem", "srem"};
  auto IsDivRem = llvm::any_of(
      IDivRem, [&Name](const auto &Arg) { return Name.startswith(Arg); });
  if (IsDivRem && ST.hasIntDivRem32() &&
      !F.getReturnType()->isIntOrIntVectorTy(64))
    return false;

  static SmallVector<StringRef, 4> FpCvt = {"fptosi", "fptoui", "sitofp",
                                            "uitofp"};
  auto IsFpCvt = llvm::any_of(
      FpCvt, [&Name](const auto &Arg) { return Name.startswith(Arg); });
  if (IsFpCvt && !ST.emulateLongLong())
    return false;

  bool Is64bit = IsDouble || F.getReturnType()->getScalarType()->isIntegerTy(64);

  if (!ST.hasLocalIntegerCas64() && Is64bit && Name.startswith("atomic_slm"))
    return false;

  return true;
}

char GenXBiFPrepare::ID = 0;

namespace llvm {
void initializeGenXBiFPreparePass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXBiFPrepare, "GenXBiFPrepare", "GenXBiFPrepare", false,
                      false)
INITIALIZE_PASS_END(GenXBiFPrepare, "GenXBiFPrepare", "GenXBiFPrepare", false,
                    false)
ModulePass *llvm::createGenXBiFPreparePass() {
  initializeGenXBiFPreparePass(*PassRegistry::getPassRegistry());
  return new GenXBiFPrepare;
}
