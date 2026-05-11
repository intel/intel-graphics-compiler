/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Analysis/AssumptionCache.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

//
// StatelessOffsetNarrowing - Narrows 64-bit stateless pointer arithmetic
// to 32-bit offset computation when buffer offsets from kernel arguments
// fit in 32 bits.
//
// On platforms without efficient native 64-bit addressing, all 64-bit pointer
// arithmetic (GEP chains) is emulated with multiple 32-bit operations. This
// pass replaces the 64-bit GEP arithmetic with 32-bit offset computation and
// reconstructs the full 64-bit address only at the point of the load/store.
//
// This pass is intended to run after StatelessToStateful, operating on the
// remaining stateless accesses that were not promoted to stateful.
//
class StatelessOffsetNarrowing : public llvm::FunctionPass {
public:
  static char ID;

  StatelessOffsetNarrowing();

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<llvm::AssumptionCacheTracker>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  virtual llvm::StringRef getPassName() const override { return "StatelessOffsetNarrowing"; }

  virtual bool runOnFunction(llvm::Function &F) override;

private:
  // Check whether the pointer operand of a load/store traces back to a
  // kernel argument through a chain of GEPs with non-negative indices.
  llvm::Value *isNarrowableStatelessAccess(llvm::Value *Pointer,
                                           llvm::SmallVectorImpl<llvm::GetElementPtrInst *> &GEPs);

  // Compute the 32-bit byte offset from a chain of GEPs.
  llvm::Value *getRawOffsetFromGEPs(const llvm::SmallVectorImpl<llvm::GetElementPtrInst *> &GEPs);

  // Check if the maximum possible byte offset produced by a GEP chain is
  // guaranteed to fit in an unsigned 32-bit integer. Uses computeKnownBits
  // to determine the number of active bits in each variable index.
  bool offsetFitsIn32Bits(const llvm::SmallVectorImpl<llvm::GetElementPtrInst *> &GEPs);

  // Check if the given pointer value can be traced back to any kernel argument.
  const KernelArg *getKernelArgFromPtr(const llvm::PointerType &PointerTy, llvm::Value *Base);

  bool HasPositivePointerOffset = false;
  llvm::Function *CurrentF = nullptr;
  llvm::AssumptionCache *CurrentAC = nullptr;
  KernelArgs *CurrentKernelArgs = nullptr;
};

} // namespace IGC
