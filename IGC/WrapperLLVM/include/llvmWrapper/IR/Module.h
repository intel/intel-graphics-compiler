/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_MODULE_H
#define IGCLLVM_IR_MODULE_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/ADT/StringRef.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
// TODO: Clean up obsolete uses at call sites
class Module : public llvm::Module {
public:
  Module(llvm::StringRef ModuleID, llvm::LLVMContext &C) : llvm::Module(ModuleID, C) {}

  // TODO: Delete getOrInsertFunction wrappers
  inline llvm::Value *getOrInsertFunction(llvm::StringRef Name, llvm::FunctionType *Ty) {
    return llvm::Module::getOrInsertFunction(Name, Ty).getCallee();
  }
  inline llvm::Value *getOrInsertFunction(llvm::StringRef Name, llvm::FunctionType *Ty,
                                          llvm::AttributeList attributeList) {
    return llvm::Module::getOrInsertFunction(Name, Ty, attributeList).getCallee();
  }

  // TODO: Refactor to use the LLVM 12+ signature at call sites
  inline llvm::StructType *getTypeByName(llvm::StringRef Name) {
    return llvm::StructType::getTypeByName(llvm::Module::getContext(), Name);
  }
};

// TODO: Refactor to use the LLVM 12+ signature at call sites
inline llvm::StructType *getTypeByName(llvm::Module &M, llvm::StringRef Name) {
  return llvm::StructType::getTypeByName(M.getContext(), Name);
}
} // namespace IGCLLVM

#endif
