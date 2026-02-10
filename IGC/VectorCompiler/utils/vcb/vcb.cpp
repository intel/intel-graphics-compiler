/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include <Probe/Assertion.h>
#include <igc/Options/Options.h>

#include <vc/Driver/Driver.h>
#include <vc/GenXCodeGen/GenXTarget.h>
#include <vc/Support/PassManager.h>
#include <vc/Support/Status.h>

#include <optional>

#include "llvmWrapper/TargetParser/Triple.h"
#include <llvm/ADT/None.h>
#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/CodeGen/TargetSubtargetInfo.h>
#include <llvm/IR/DiagnosticInfo.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/ToolOutputFile.h>

#include <llvm/Pass.h>
#include <llvmWrapper/Target/TargetMachine.h>
#include <llvmWrapper/Support/TargetRegistry.h>
using namespace llvm;

void vcbCompileModule(std::unique_ptr<Module> &M, std::string Platform);
void vcbCompileUnique(std::string InputFilename, std::string OutputFilename);

static cl::opt<bool> TextOutput(
    "S", cl::desc("emitting text IR when the option is set, binary otherwise"));

static cl::opt<bool>
    BiFUnique("BiFUnique",
              cl::desc("special compilation mode for BiF. Use it in order to "
                       "find unique BiF modules and create lib for use"));

static cl::opt<std::string> InputFilename(cl::Positional,
                                          cl::desc("<input_file_name.{ll,bc}>"),
                                          cl::init("-"));

static cl::opt<bool> Is32Bit("b32", cl::desc("enabling 32bit mode"));

static cl::opt<std::string> FeaturesStr("feature",
                                        cl::desc("additional features"),
                                        cl::value_desc("feature"),
                                        cl::init("+ocl_runtime"));

static cl::opt<std::string>
    PlatformString("cpu", cl::desc("platform for compilation (default: Gen9)"),
                   cl::value_desc("platform"), cl::init("Gen9"));

static cl::opt<std::string> OutputFilename("o",
                                           cl::desc("Override output filename"),
                                           cl::value_desc("filename"),
                                           cl::init("-"));

static int initializeAll() {
  llvm::initializeGenX();
  LLVMInitializeGenXPasses();
  return 0;
}

static Expected<std::unique_ptr<TargetMachine>>
createTargetMachine(Triple &TheTriple, std::string CPUStr) {
  std::string Error;
  const Target *TheTarget = TargetRegistry::lookupTarget(
      TheTriple.getArchName().str(), TheTriple, Error);
  IGC_ASSERT_MESSAGE(TheTarget, "vc target was not registered");

  const TargetOptions Options;
  CodeGenOpt::Level OptLevel = CodeGenOpt::Default;

  std::unique_ptr<TargetMachine> TM{TheTarget->createTargetMachine(
      TheTriple.getTriple(), CPUStr, FeaturesStr, Options, /*RelocModel=*/{},
      /*CodeModel=*/{}, OptLevel)};
  if (!TM)
    return make_error<vc::TargetMachineError>();
  return {std::move(TM)};
}

// Precompiles input \p M module for the specified \p Platform. One of the
// primary entry points to vcb tool.
// Precompilation is performed via VC CodeGen pipeline configured to use a
// special "BiFCompilation" mode.
void vcbCompileModule(std::unique_ptr<Module> &M, std::string Platform) {
  GenXBackendOptions Options;
  Options.BiFCompilation = true;
  // Target configuration.
  Triple TheTriple{Is32Bit ? "genx32-unknown-unknown"
                           : "genx64-unknown-unknown"};
  M->setTargetTriple(TheTriple.getTriple());
  auto ExpTargetMachine = createTargetMachine(TheTriple, std::move(Platform));
  if (!ExpTargetMachine) {
    errs() << ExpTargetMachine.takeError();
    report_fatal_error("Can't find Target Machine");
  }
  TargetMachine &TM = *ExpTargetMachine.get();
  M->setDataLayout(TM.createDataLayout());

  // Fill/initialize VC Codegen pipeline.
  legacy::PassManager PM;
  llvm::raw_null_ostream NOS;
  auto FileType = IGCLLVM::TargetMachine::CodeGenFileType::CGFT_AssemblyFile;
  bool DisableIrVerifier = true;
  PM.add(new GenXBackendConfig{std::move(Options), GenXBackendData()});
  [[maybe_unused]] bool AddPasses =
      TM.addPassesToEmitFile(PM, NOS, nullptr, FileType, DisableIrVerifier);
  IGC_ASSERT_MESSAGE(!AddPasses, "Bad filetype for vc-codegen");

  // Output configuration.
  std::error_code EC;
  sys::fs::OpenFlags Flags = TextOutput ? sys::fs::OF_Text : sys::fs::OF_None;
  ToolOutputFile Output{OutputFilename, EC, Flags};
  if (EC)
    report_fatal_error(llvm::StringRef("Can't open file : " + OutputFilename));
  if (TextOutput)
    PM.add(createPrintModulePass(Output.os()));
  else
    PM.add(createBitcodeWriterPass(Output.os()));

  // Run codegen.
  PM.run(*M);
  Output.keep();
}

std::unique_ptr<Module> safeParseIRFile(StringRef Filename, SMDiagnostic &Err,
                                        LLVMContext &Context) {
  std::unique_ptr<Module> M = parseIRFile(Filename, Err, Context);
  if (!M)
    report_fatal_error("Failed to Parse IR");
  return M;
}

// Primary modes of operations for vcb:
// vcbCompileModule - pre-compiles emulation BiF for the target platform.
// vcbCompileUnique - generates .cpp code that allows VC backend to obtain
//                    platform-specific emulation library during runtime.
int main(int Argc, char **Argv) {
  initializeAll();

  LLVMContext Context;
  SMDiagnostic Err;

  cl::ParseCommandLineOptions(Argc, Argv);
  if (BiFUnique) {
    vcbCompileUnique(InputFilename, OutputFilename);
    return 0;
  }
  auto M = safeParseIRFile(InputFilename, Err, Context);

  vcbCompileModule(M, PlatformString);
  return 0;
}
