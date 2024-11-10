/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "vc/GenXOpts/GenXOptsNewPM.h"
#include "vc/Support/PassManager.h"
#include "vc/Support/PassPrinters.h"

using namespace llvm;
// using namespace genx;

namespace {
void registerPluginPasses(PassBuilder &PB) {

#define ADD_PASS(NAME, CREATE_PASS)                                            \
  if (Name == NAME) {                                                          \
    PM.addPass(CREATE_PASS);                                                   \
    return true;                                                               \
  }

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

// #if defined(_MSC_VER)
// #pragma comment(linker, "/EXPORT:llvmGetPassPluginInfo")
// #endif // defined(_MSC_VER)

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return getNewPMPluginInfo();
}