/*========================== begin_copyright_notice ============================

Copyright (c) 2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "cmcl/Support/BuiltinTranslator.h"

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
