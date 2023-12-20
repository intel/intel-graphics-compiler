/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "IGC/BiFModule/Headers/bif_control_common.h"
#include "IGC/common/Types.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#define BIF_FLAG_CONTROL(BIF_FLAG_TYPE, BIF_FLAG_NAME)                            \
  BIF_FLAG_CTRL_N_S(BIF_FLAG_NAME),

#define BIF_FLAG_CTRL_SET(BIF_FLAG_NAME, BIF_FLAG_VALUE)                          \
  ListDelegates.emplace(BIF_FLAG_CTRL_N_S(BIF_FLAG_NAME), [this]() -> bool {      \
    return replace(BIF_FLAG_VALUE,                                                \
                   pModule->getGlobalVariable(BIF_FLAG_CTRL_N_S(BIF_FLAG_NAME))); \
  })


namespace IGC {
/// @brief  BIFFlagCtrlResolution pass used for resolving BIF flag controls in
/// kernel.
class BIFFlagCtrlResolution : public llvm::ModulePass {
public:
  // Pass identification, replacement for typeid
  static char ID;

  /// @brief  Constructor
  BIFFlagCtrlResolution();

  BIFFlagCtrlResolution(CodeGenContext *ptrCGC);

  /// @brief  Destructor
  ~BIFFlagCtrlResolution();

  /// @brief  Provides name of pass
  virtual llvm::StringRef getPassName() const override {
    return "BIFFlagCtrlResolution";
  }

  virtual bool runOnModule(llvm::Module &M) override;

private:
  CodeGenContext *PtrCGC;
  llvm::Module *pModule;

  std::map<llvm::StringRef, std::function<bool()>> ListDelegates;

  template <typename T> bool replace(T Value, llvm::GlobalVariable *GV);

  void FillFlagCtrl();
};

} // namespace IGC
