/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/LoopPass.h"
#include "common/LLVMWarningsPop.hpp"

namespace llvm {
class DominatorTree;
class Instruction;
class Loop;
class PassRegistry;
class Value;
} // namespace llvm

namespace IGC {
////////////////////////////////////////////////////////////////////////
/// @brief Loop-aware speculative issue of sampler iterations.
///
/// Runs after LLVM's forced partial unroll of a data-dependent sampler loop.
/// It recognizes the K cloned sampler iterations and moves their independent
/// address-generation slices and sampler sends ahead of the retained,
/// result-dependent iteration exit checks so the finalizer can overlap the
/// sends. It only relocates existing instructions; it never clones the loop or
/// alters the CFG.
class SamplerLoopSpeculation : public llvm::LoopPass {
public:
  static char ID;

  SamplerLoopSpeculation();

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  bool runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM) override;
  llvm::StringRef getPassName() const override;

private:
  bool ProcessSamplerLoop(llvm::Loop *L);

  bool CollectSpeculativeAddressSlice(llvm::Value *V, llvm::Instruction *InsertBefore, llvm::Loop *L,
                                      llvm::SmallPtrSetImpl<llvm::Instruction *> &Visited,
                                      llvm::SmallVectorImpl<llvm::Instruction *> &AddressSlice);

  llvm::DominatorTree *m_pDT = nullptr;
};

llvm::LoopPass *createSamplerLoopSpeculation();
} // namespace IGC

void initializeSamplerLoopSpeculationPass(llvm::PassRegistry &);
