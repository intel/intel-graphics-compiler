/*========================== begin_copyright_notice ============================

Copyright (C) 2024-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringSet.h>
#include "common/LLVMWarningsPop.hpp"

#include <memory>
#include <string>
#include <vector>

#include "BiFManagerCommon.hpp"

namespace IGC {
namespace BiFManager {
// Builds the BiF package. Each module is loaded in its own LLVMContext, so that the
// tool fits into the memory of a 32-bit process.
class BiFManagerTool : public BiFManagerCommon {
public:
  BiFManagerTool(llvm::LLVMContext &Context);
  ~BiFManagerTool();

  // Writes the functions and globals each module defines and declares, as JSON for depgraph.py.
  bool WriteSymbols(const std::vector<std::string> &MainModules, const std::vector<std::string> &Size32Modules,
                    const std::vector<std::string> &Size64Modules, const llvm::StringRef SymbolsPath);

  // Prepares the modules, in the order of depgraph.py, for the package.
  // SharedGlobals : global variables which a module uses from another one
  bool MakeBiFPackage(const std::vector<std::string> &Modules, const llvm::StringSet<> &SharedGlobals);

  // Writes the bif-stream package
  void WriteBitcode(const llvm::StringRef BitCodePath);

private:
  // Bitcode of the prepared modules, in package order
  std::vector<llvm::SmallVector<char, 0>> BiFModules;

  // Loads a module and removes what it does not use, so that unused
  // declarations do not become dependencies.
  static std::unique_ptr<llvm::Module> loadModule(const std::string &Path, llvm::LLVMContext &ModuleContext);

  static void markBuiltinFunc(llvm::Module *pM);
};
} // namespace BiFManager
} // namespace IGC
