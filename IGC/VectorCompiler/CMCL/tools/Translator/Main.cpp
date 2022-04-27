/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "cmcl/Support/BuiltinTranslator.h"

#include <llvm/Pass.h>
#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/FileSystem.h>

using namespace llvm;

static cl::opt<std::string> InputFilename(cl::Positional,
                                          cl::desc("<input_file_name.{ll,bc}>"),
                                          cl::init("-"));
static cl::opt<bool> TextOutput(
    "S", cl::desc("emitting text IR when the option is set, binary otherwise"));

static cl::opt<std::string> OutputFilename("o",
                                           cl::desc("Override output filename"),
                                           cl::value_desc("filename"),
                                           cl::init("-"));

int main(int argc, const char *argv[]) {
  InitLLVM X(argc, argv);
  LLVMContext Context;
  SMDiagnostic Err;
  cl::ParseCommandLineOptions(argc, argv);
  std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
  if (!M) {
    Err.print(argv[0], errs());
    return 1;
  }
  cmcl::translateBuiltins(*M);

  // output
  std::error_code EC;
  sys::fs::OpenFlags Flags = TextOutput ? sys::fs::OF_Text : sys::fs::OF_None;
  ToolOutputFile Output{OutputFilename, EC, Flags};
  legacy::PassManager PM;
  if (TextOutput)
    PM.add(createPrintModulePass(Output.os()));
  else
    PM.add(createBitcodeWriterPass(Output.os()));
  PM.run(*M);
  Output.keep();
  return 0;
}
