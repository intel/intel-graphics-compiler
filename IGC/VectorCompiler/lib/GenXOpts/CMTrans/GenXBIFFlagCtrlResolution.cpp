/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGC/BiFModule/Headers/bif_control_common.h"
#include "IGC/common/StringMacros.hpp"
#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Support/GenXDiagnostic.h"
#include "llvm/PassRegistry.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/Registry.h>
#include <llvmWrapper/IR/Instructions.h>
#include <map>

using namespace llvm;
#define DEBUG_TYPE "GenXBIFFlagCtrlResolution"

#define BIF_FLAG_CONTROL(BIF_FLAG_TYPE, BIF_FLAG_NAME)                            \
  BIF_FLAG_CTRL_N_S(BIF_FLAG_NAME),

#define BIF_FLAG_CTRL_SET(BIF_FLAG_NAME, BIF_FLAG_VALUE)                          \
  ListDelegates.emplace(BIF_FLAG_CTRL_N_S(BIF_FLAG_NAME), [this]() -> bool {      \
    return replace(BIF_FLAG_VALUE,                                                \
                   pModule->getGlobalVariable(BIF_FLAG_CTRL_N_S(BIF_FLAG_NAME))); \
  })


namespace {
class GenXBIFFlagCtrlResolution final : public llvm::ModulePass {
public:
  // Pass identification, replacement for typeid
  static char ID;
  GenXBIFFlagCtrlResolution() : ModulePass(ID) {}

  ~GenXBIFFlagCtrlResolution() {}

  virtual bool runOnModule(Module &M) override;

private:
  Module *pModule = nullptr;
  std::map<StringRef, std::function<bool()>> ListDelegates;

  template <typename T> bool replace(T Value, GlobalVariable *GV);

  void FillFlagCtrl();
};
} // namespace

void GenXBIFFlagCtrlResolution::FillFlagCtrl() {
  // Here we can add more flags
  // Provide in first argument BiF flag control defined in
  // IGC/BiFModule/Headers/bif_flag_controls.h
  // In second place provide the value which should be
  // inserted for this flag. If needed feed class GenXBIFFlagCtrlResolution
  // with new value from outside.

  // Need to feed this correctly
  BIF_FLAG_CTRL_SET(PlatformType, 0 /*platform.GetProductFamily()*/);
  BIF_FLAG_CTRL_SET(RenderFamily, 0 /*platform.eRenderCoreFamily*/);

  BIF_FLAG_CTRL_SET(FlushDenormals, true);
  BIF_FLAG_CTRL_SET(DashGSpecified, false);
  BIF_FLAG_CTRL_SET(FastRelaxedMath, false);
  BIF_FLAG_CTRL_SET(MadEnable, false);
  BIF_FLAG_CTRL_SET(UseNative64BitIntBuiltin, true);
  BIF_FLAG_CTRL_SET(UseNative64BitFloatBuiltin, true);
  BIF_FLAG_CTRL_SET(CRMacros, true);
  BIF_FLAG_CTRL_SET(IsSPIRV, false);
  BIF_FLAG_CTRL_SET(EnableSWSrgbWrites, false);
  BIF_FLAG_CTRL_SET(ProfilingTimerResolution, 0.0f);
  BIF_FLAG_CTRL_SET(UseMathWithLUT, false);
  BIF_FLAG_CTRL_SET(UseHighAccuracyMath, false);
  BIF_FLAG_CTRL_SET(UseAssumeInGetGlobalId, true);
  // FIXME: target specific, but subtarget cannot be reached in middle-end.
  BIF_FLAG_CTRL_SET(HasInt64SLMAtomicCAS, false);
  BIF_FLAG_CTRL_SET(JointMatrixLoadStoreOpt, 3);
  BIF_FLAG_CTRL_SET(OptDisable, false);
  BIF_FLAG_CTRL_SET(UseNativeFP32GlobalAtomicAdd, false);
  BIF_FLAG_CTRL_SET(UseNativeFP16AtomicMinMax, false);
  BIF_FLAG_CTRL_SET(UseNativeFP64GlobalAtomicAdd, false);
  BIF_FLAG_CTRL_SET(HasThreadPauseSupport, false);
  BIF_FLAG_CTRL_SET(hasHWLocalThreadID, false);
  BIF_FLAG_CTRL_SET(APIRS, false);
  BIF_FLAG_CTRL_SET(UseLSC, false);
  BIF_FLAG_CTRL_SET(UseBfn, false);
  BIF_FLAG_CTRL_SET(ForceL1Prefetch, false);
  BIF_FLAG_CTRL_SET(UseNativeFP64GlobalAtomicAdd, false);
  BIF_FLAG_CTRL_SET(MaxHWThreadIDPerSubDevice, 1);
  BIF_FLAG_CTRL_SET(UseOOBChecks, false);
  BIF_FLAG_CTRL_SET(UseBindlessImage, false);
}

#undef BIF_FLAG_CTRL_SET

bool GenXBIFFlagCtrlResolution::runOnModule(Module &M) {
  pModule = &M;
  FillFlagCtrl();

  std::vector<StringRef> listOfFlagNames{
#include "IGC/BiFModule/Headers/bif_flag_controls.h"
  };

  bool wasModuleUpdated = false;

  for (size_t i = 0; i < listOfFlagNames.size(); ++i) {
    StringRef &bif_flag = listOfFlagNames[i];

    auto iter = ListDelegates.find(bif_flag);

    if (iter != ListDelegates.end()) {
      // Here we are replacing the Bif flag control if present,
      // it's executing the delegate function
      wasModuleUpdated |= iter->second();
    } else {
      IGC_ASSERT_EXIT_MESSAGE(0, "[BIF_VC] Missing setup for flag %s in FillFlagCtrl function", bif_flag.str().c_str());
    }
  }

  return wasModuleUpdated;
}

#undef BIF_FLAG_CONTROL

template <typename T>
bool GenXBIFFlagCtrlResolution::replace(T Value, GlobalVariable *GV) {
  bool Changed = false;
  if (!GV)
    return Changed;

  if (std::is_enum<T>() || std::is_integral<T>()) {
    GV->setInitializer(ConstantInt::getIntegerValue(
        GV->getValueType(),
        APInt((unsigned)GV->getValueType()->getPrimitiveSizeInBits(),
              (unsigned int)Value, std::is_signed<T>())));
    Changed = true;
  } else if (std::is_floating_point<T>()) {
    GV->setInitializer(ConstantFP::get(GV->getValueType(), (double)Value));
    Changed = true;
  } else {
    IGC_ASSERT_EXIT_MESSAGE(0, "[BIF_VC] Not supported BiF flag control type");
  }

  return Changed;
}

char GenXBIFFlagCtrlResolution::ID = 0;

INITIALIZE_PASS_BEGIN(GenXBIFFlagCtrlResolution, "GenXBIFFlagCtrlResolution",
                      "GenXBIFFlagCtrlResolution", false, false)

INITIALIZE_PASS_END(GenXBIFFlagCtrlResolution, "GenXBIFFlagCtrlResolution",
                    "GenXBIFFlagCtrlResolution", false, false)

namespace llvm {
ModulePass *createGenXBIFFlagCtrlResolutionPass() {
  initializeGenXBIFFlagCtrlResolutionPass(*PassRegistry::getPassRegistry());
  return new GenXBIFFlagCtrlResolution;
}
} // namespace llvm

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses
GenXBIFFlagCtrlResolutionPass::run(llvm::Module &M,
                                   llvm::AnalysisManager<llvm::Module> &) {
  GenXBIFFlagCtrlResolution GenXBiF;
  if (GenXBiF.runOnModule(M))
    return PreservedAnalyses::none();
  return PreservedAnalyses::all();
}
// TODO: No lit-tests
#endif
