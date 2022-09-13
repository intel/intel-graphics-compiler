/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/LoopInfo.h>
#include "common/LLVMWarningsPop.hpp"
#include <Compiler/CISACodeGen/RegisterPressureEstimate.hpp>
#include <Compiler/CodeGenContextWrapper.hpp>
#include <Compiler/CodeGenPublic.h>

namespace IGC {
class RegPressureLoopControl : public llvm::FunctionPass {
public:
  static char ID;
  const static int MaxRegPressureLoopHdr;

  RegPressureLoopControl();

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<RegisterPressureEstimate>();
    AU.addRequired<llvm::LoopInfoWrapperPass>();
  }

  bool runOnFunction(llvm::Function &F) override;

  llvm::StringRef getPassName() const override {
    return "IGC Register pressure loop control";
  }

private:
  RegisterPressureEstimate *m_pRegisterPressureEstimate = nullptr;
};

} // namespace IGC
