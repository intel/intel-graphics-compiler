/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// Moves a read of a read-only buffer that follows a barrier or fence to
/// before it, so that its latency overlaps the wait instead of following it.
///
/// Read-only storage cannot be written by the threads being synchronized, so
/// the wait orders nothing the read depends on. The read moves directly above
/// the nearest run of waits before it in its own block, so it runs on exactly
/// the paths and as often as before and nothing is speculated. The side-effect-free
/// instructions that compute its address move with it.
class HoistImmutableLoadsAboveBarriers : public llvm::FunctionPass {
public:
  static char ID;

  HoistImmutableLoadsAboveBarriers();

  llvm::StringRef getPassName() const override { return "HoistImmutableLoadsAboveBarriers"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  bool runOnFunction(llvm::Function &F) override;
};

llvm::FunctionPass *createHoistImmutableLoadsAboveBarriersPass();
} // namespace IGC
