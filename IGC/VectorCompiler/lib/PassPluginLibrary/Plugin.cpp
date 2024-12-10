/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "vc/GenXCodeGen/GenXVerify.h"
#include "vc/GenXOpts/GenXOptsNewPM.h"
#include "vc/Support/BackendConfig.h"

using namespace llvm;

namespace {
void registerPluginPasses(PassBuilder &PB) {

  PB.registerAnalysisRegistrationCallback([=](ModuleAnalysisManager &MAM) {
    MAM.registerPass([&] { return CMABIAnalysisPass(); });
    MAM.registerPass([&] { return GenXBackendConfigPass(); });
  });

  auto *BC = new GenXBackendConfig;

#define ADD_PASS(NAME, CREATE_PASS)                                            \
  if (Name == NAME) {                                                          \
    PM.addPass(CREATE_PASS);                                                   \
    return true;                                                               \
  }

  PB.registerPipelineParsingCallback(
      [=](StringRef Name, CGSCCPassManager &PM,
          ArrayRef<PassBuilder::PipelineElement>) {
#define CGSCC_PASS(NAME, CREATE_PASS) ADD_PASS(NAME, CREATE_PASS)
#include "GenXPassRegistry.def"
#undef CGSCC_PASS
        return false;
      });

  PB.registerPipelineParsingCallback(
      [=](StringRef Name, ModulePassManager &PM,
          ArrayRef<PassBuilder::PipelineElement>) {
#define MODULE_PASS(NAME, CREATE_PASS) ADD_PASS(NAME, CREATE_PASS)
#include "GenXPassRegistry.def"
#undef MODULE_PASS
        return false;
      });

  PB.registerPipelineParsingCallback(
      [=](StringRef Name, FunctionPassManager &PM,
          ArrayRef<PassBuilder::PipelineElement>) {
#define FUNCTION_PASS(NAME, CREATE_PASS) ADD_PASS(NAME, CREATE_PASS)
#include "GenXPassRegistry.def"
#undef FUNCTION_PASS
        return false;
      });
}
} // namespace

PassPluginLibraryInfo getNewPMPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "NewPMPlugin", "v0.1", registerPluginPasses};
}

#ifdef __GNUC__
#define DLL_PUBLIC __attribute__((visibility("default")))
#else
#define DLL_PUBLIC __declspec(dllexport)
#endif

// TODO: Fix visibility
extern "C" {
LLVM_ATTRIBUTE_WEAK DLL_PUBLIC PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return getNewPMPluginInfo();
}
}
