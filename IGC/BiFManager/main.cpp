/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/CommandLine.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/Instructions.h>
#include <llvmWrapper/IR/Module.h>
#include <llvmWrapper/ADT/STLExtras.h>
#include "BiFManagerTool.hpp"

#include <string>
#include <list>
#include <fstream>
#include <sstream>

using namespace std;
using namespace llvm;

static cl::opt<std::string> InputBCFilename(cl::Positional, cl::desc("<input .bc file>"), cl::init("-"));
static cl::opt<std::string> InputBC32Filename(cl::Positional, cl::desc("<input .bc file>"), cl::init("-"));
static cl::opt<std::string> InputBC64Filename(cl::Positional, cl::desc("<input .bc file>"), cl::init("-"));
static cl::opt<std::string> OutputPath(cl::Positional, cl::desc("<output .bifbc file>"), cl::init("-"));
static cl::opt<std::string> OutputPathH(cl::Positional, cl::desc("<output .h file>"), cl::init("-"));

int main(int argc, char *argv[]) {
  LLVMContext Context;
#if LLVM_VERSION_MAJOR >= 16
  bool enableOpaquePointers = __IGC_OPAQUE_POINTERS_API_ENABLED;

  if (enableOpaquePointers) {
    printf("[BiFManager] - Enabling Opaque Pointers\n");
  } else {
    printf("[BiFManager] - Disabling Opaque Pointers\n");
  }

#if LLVM_VERSION_MAJOR < 17 || defined(IGC_LLVM_TRUNK_REVISION)
  Context.setOpaquePointers(enableOpaquePointers);
#endif
#endif

  cl::ParseCommandLineOptions(argc, argv);

  auto LoadModule = [&](std::string *PathToModule) {
    SMDiagnostic Err;
    ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr = MemoryBuffer::getFileOrSTDIN(*PathToModule);
    std::unique_ptr<llvm::MemoryBuffer> genericBufferPtr(FileOrErr.get().release());
    Expected<std::unique_ptr<Module>> M = llvm::parseBitcodeFile(genericBufferPtr->getMemBufferRef(), Context);
    if (llvm::Error EC = M.takeError()) {
      std::stringstream stringBuilder("");
      stringBuilder << "[BiFManager] - Unable to Parse bitcode:";
      llvm::handleAllErrors(std::move(EC), [&](llvm::ErrorInfoBase &EIB) { stringBuilder << EIB.message(); });
      Err.print(stringBuilder.str().c_str(), errs());
    }
    return M;
  };
  auto ModuleMain = LoadModule(&InputBCFilename);
  if (ModuleMain)
    printf("[BiFManager] - Loaded Main module\n");
  else {
    printf("[BiFManager] - Problem with loading Main module\n");
    return -13;
  }

  auto Module32 = LoadModule(&InputBC32Filename);
  if (Module32)
    printf("[BiFManager] - Loaded Size32 module\n");
  else {
    printf("[BiFManager] - Problem with loading Size32 module\n");
    return -13;
  }
  auto Module64 = LoadModule(&InputBC64Filename);
  if (Module64)
    printf("[BiFManager] - Loaded Size64 module\n");
  else {
    printf("[BiFManager] - Problem with loading Size64 module\n");
    return -13;
  }

  IGC::BiFManager::BiFManagerTool bif(Context);
  bif.MakeBiFPackage(&(*ModuleMain.get()), &(*Module32.get()), &(*Module64.get()));

  bif.WriteHeader(OutputPathH.c_str());
  bif.WriteBitcode(OutputPath.c_str());

  return 0;
}
