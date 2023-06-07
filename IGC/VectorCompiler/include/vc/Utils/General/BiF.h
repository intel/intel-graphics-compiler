/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENERAL_BIF_H
#define VC_UTILS_GENERAL_BIF_H

#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/MemoryBuffer.h>

#include <string>
#include <vector>

namespace vc {

// Decodes binary module provided via \p BiFModuleBuffer. Returns obtained
// llvm::Module. If some errors occured reports fatal error.
// Note: wraps parseBitcodeFile.
std::unique_ptr<llvm::Module>
getBiFModuleOrReportError(llvm::MemoryBufferRef BiFModuleBuffer,
                          llvm::LLVMContext &Ctx);

// Same as getBiFModuleOrReportError but the decoding is lazy.
// Note: wraps getLazyBitcodeModule.
std::unique_ptr<llvm::Module>
getLazyBiFModuleOrReportError(llvm::MemoryBufferRef BiFModuleBuffer,
                              llvm::LLVMContext &Ctx);

using FunctionNameSeq = std::vector<std::string>;

// Collect all functions for which predicate \p Pred returns true.
// PredT is a functor that takes const Function &F as an argument and returns
// bool.
template <typename PredT>
FunctionNameSeq collectFunctionNamesIf(const llvm::Module &M, PredT Pred) {
  using namespace llvm;
  auto Functions = make_filter_range(M.getFunctionList(), Pred);
  FunctionNameSeq Names;
  llvm::transform(Functions, std::back_inserter(Names),
                  [](const Function &F) { return F.getName().str(); });
  return Names;
}

// Set internal linkage for functions whose name is in \p FuncNames.
// \p FuncNames may contain declarations, they won't be changed. AlwaysInline
// attribute is optionally set.
void internalizeImportedFunctions(const llvm::Module &M,
                                  const FunctionNameSeq &FuncNames,
                                  bool SetAlwaysInline);

constexpr inline const char LibraryFunctionPrefix[] = "__vc_builtin_";

} // namespace vc

#endif // VC_UTILS_GENERAL_BIF_H
