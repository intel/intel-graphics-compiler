/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"         // for suppressing LLVM warnings
#include <llvm/ADT/StringRef.h>                // for llvm::StringRef
#include <llvm/IR/Function.h>                  // for llvm::Function
#include <llvm/Pass.h>                         // for llvm::FunctionPass
#include "common/LLVMWarningsPop.hpp"          // for suppressing LLVM warnings
#include "GenISAIntrinsics/GenIntrinsicInst.h" // for llvm::GenISAIntrinsic::ID

namespace IGC {


class LSCCacheHints : public llvm::FunctionPass {
public:
  LSCCacheHints();
  llvm::StringRef getPassName() const override { return "LSCCacheHints"; }
  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  static char ID;

private:
  bool VisitInstruction(const ModuleMetaData *ModMD, llvm::Instruction &I);
  bool SetInstructionCacheHint(llvm::Instruction &I, const unsigned CacheHint, llvm::StringRef Reason,
                               bool AllowOverwrite = false) const;
  bool SetupLscCacheCtrl(const ModuleMetaData *ModMD, llvm::Instruction &I);
  // Operand-channel resolution. Distinct from the metadata channel:
  // these intrinsics carry their cache control in an immediate i32 operand that
  // is not represented by lsc.cache.ctrl metadata. The pass resolves the
  // constant-buffer load override, prefetch validation, and atomic override by
  // rewriting that operand in place to its final IGC1 LSC_L1_L3_CC enum value.
  static bool IsOperandChannelIntrinsic(const llvm::Instruction &I);
  bool ResolveOperandCacheControl(llvm::Instruction &I);
  // Non-operand atomic resolution. Scalar/typed atomics
  // (GenISA_intatomicraw / *typed / cmpxchg / singlelane families) have NO cache
  // operand. When the atomic override is set, attach cache-control metadata via
  // SetInstructionCacheHint; when unset, attach nothing so the default cache
  // behavior is preserved.
  // These must NOT go through VisitInstruction/SetupLscCacheCtrl/global-override:
  // those apply load/store defaults and Lsc/Tgm overrides that do not belong to
  // atomics.
  static bool IsOverridableAtomic(const llvm::Instruction &I);
  bool ResolveAtomicCacheControl(llvm::Instruction &I);
  bool TryApplyRovOrForceList(const ModuleMetaData *ModMD, llvm::Instruction &I);
  bool UseRasterizerOrderedByteAddressBuffer(const ModuleMetaData *ModMD, llvm::Instruction &I) const;
  bool TryApplyGlobalOverride(const ModuleMetaData *ModMD, llvm::Instruction &I, bool &Changed);
  static bool TryOverrideCacheOpts(uint32_t &CacheCtrl, bool IsLoad, bool IsTGM,
                                   const CacheControlOverride &CacheControlOption);
  static bool IsCacheHintRelevantInstruction(const llvm::Instruction &I);
  static bool IsLoadShapedIO(const llvm::Instruction &I);
  static bool IsTGMIntrinsic(llvm::GenISAIntrinsic::ID Id);

};

void initializeLSCCacheHintsPass(llvm::PassRegistry &);
llvm::FunctionPass *createLSCCacheHintsPass();

} // namespace IGC
