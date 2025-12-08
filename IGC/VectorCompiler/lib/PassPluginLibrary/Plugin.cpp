/*========================== begin_copyright_notice ============================

Copyright (C) 2024-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvmWrapper/TargetParser/Triple.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Target/TargetOptions.h"

#include "vc/GenXCodeGen/GenXLowerAggrCopies.h"
#include "vc/GenXCodeGen/GenXVerify.h"
#include "vc/GenXOpts/GenXOptsNewPM.h"
#include "vc/Support/BackendConfig.h"

#include "vc/GenXCodeGen/GenXRegionCollapsing.h"
#include "vc/GenXCodeGen/GenXTarget.h"
#include "vc/GenXCodeGen/TargetMachine.h"

#include "GenXTargetMachine.h"

using namespace llvm;

namespace {

static TargetMachine *GetTargetMachine(Triple TheTriple, StringRef CPUStr,
                                       StringRef FeaturesStr,
                                       const TargetOptions &Options) {
  std::string Error;
  const Target *TheTarget =
      TargetRegistry::lookupTarget(codegen::getMArch(), TheTriple, Error);
  // Some modules don't specify a triple, and this is okay.
  if (!TheTarget) {
    llvm::errs() << Error << "\n";
    return nullptr;
  }

  return TheTarget->createTargetMachine(
      TheTriple.getTriple(), codegen::getCPUStr(), codegen::getFeaturesStr(),
      Options, codegen::getExplicitRelocModel(),
      codegen::getExplicitCodeModel(), CodeGenOpt::Default);
}

void registerPluginPasses(PassBuilder &PB) {

  LLVMInitializeGenXTarget();
  LLVMInitializeGenXTargetInfo();

  PB.registerAnalysisRegistrationCallback([=](ModuleAnalysisManager &MAM) {
    MAM.registerPass([&] { return CMABIAnalysisPass(); });
    MAM.registerPass([&] { return GenXBackendConfigPass(); });
  });

  auto BC = std::make_shared<GenXBackendConfig>();

  const TargetOptions Options;
  auto TheTriple = Triple("genx64-unknown-unknown");
  std::string Error = "";
  std::string CPUStr = "";
  std::string FeaturesStr = "";

  llvm::TargetMachine *TM =
      GetTargetMachine(std::move(TheTriple), CPUStr, FeaturesStr, Options);

  auto *GTM = static_cast<GenXTargetMachine *>(TM);

#define ADD_PASS(NAME, CREATE_PASS)                                            \
  if (Name == NAME) {                                                          \
    PM.addPass(CREATE_PASS);                                                   \
    return true;                                                               \
  }

  PB.registerPipelineParsingCallback(
      [=](StringRef Name, CGSCCPassManager &PM,
          ArrayRef<PassBuilder::PipelineElement>) {
#define CGSCC_PASS(NAME, CREATE_PASS) ADD_PASS(NAME, CREATE_PASS)
#include "GenXPassRegistry.h"
#undef CGSCC_PASS
        return false;
      });

  PB.registerPipelineParsingCallback(
      [=](StringRef Name, ModulePassManager &PM,
          ArrayRef<PassBuilder::PipelineElement>) {
#define MODULE_PASS(NAME, CREATE_PASS) ADD_PASS(NAME, CREATE_PASS)
#include "GenXPassRegistry.h"
#undef MODULE_PASS
        return false;
      });

  PB.registerPipelineParsingCallback(
      [=](StringRef Name, FunctionPassManager &PM,
          ArrayRef<PassBuilder::PipelineElement>) {
#define FUNCTION_PASS(NAME, CREATE_PASS) ADD_PASS(NAME, CREATE_PASS)
#include "GenXPassRegistry.h"
#undef FUNCTION_PASS
        return false;
      });
}
} // namespace

#ifdef __GNUC__
#define DLL_PUBLIC __attribute__((visibility("default")))
#else
#define DLL_PUBLIC __declspec(dllexport)
#endif

extern "C" {
PassPluginLibraryInfo DLL_PUBLIC getNewPMPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "NewPMPlugin", "v0.1", registerPluginPasses};
}

#if !(defined(_WIN64) || defined(_WIN32))
LLVM_ATTRIBUTE_WEAK DLL_PUBLIC PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return getNewPMPluginInfo();
}
#endif
}
