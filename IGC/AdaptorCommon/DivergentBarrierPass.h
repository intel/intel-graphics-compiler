/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "IGC/common/StringMacros.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the new-pass-manager
// wrappers below; it is not itself an llvm::Pass. The CodeGenContext and MetaDataUtils are injected
// by the caller (run) so the engine does not depend on getAnalysis<>.
class DivergentBarrierPass {
public:
  DivergentBarrierPass(void *Ctx = nullptr) : Ctx(Ctx) {}
  bool run(llvm::Module &M, CodeGenContext *CGCtx, IGCMD::MetaDataUtils *MDUtils);

private:
  struct FenceArgs {
    bool CommitEnable = true;
    bool L3_Flush_RW_Data = false;
    bool L3_Flush_Constant_Data = false;
    bool L3_Flush_Texture_Data = false;
    bool L3_Flush_Instructions = false;
    bool Global = false;
    bool L1_Invalidate = false;
    bool L1_Evict = false;
    // init Scope with GROUP which should be used for SLM barriers
    // to be consistent with Global argument initialized to false
    uint Scope = LSC_SCOPE_GROUP;
  };

  typedef llvm::DenseMap<uint64_t, WIAnalysis::WIDependancy> SlotDepMap;

  CodeGenContext *m_CGCtx = nullptr;
  IGCMD::MetaDataUtils *m_MDUtils = nullptr;
  bool processShader(llvm::Function *F);
  bool hasDivergentBarrier(const std::vector<llvm::Instruction *> &Barriers, WIAnalysisRunner &WI) const;
  llvm::Function *createContinuation(llvm::BasicBlock *EntryBB);
  void updateFenceArgs(const llvm::GenIntrinsicInst *I, FenceArgs &Args) const;
  void generateBody(llvm::Function *Wrapper, llvm::Function *Entry, const std::vector<llvm::Function *> &Continuations,
                    const FenceArgs &FA);
  llvm::Value *getGroupSize(llvm::Function &F) const;
  llvm::Value *allocateSLM(llvm::IRBuilder<> &IRB);
  llvm::CallInst *insertFence(llvm::IRBuilder<> &IRB, const FenceArgs &FA) const;
  void handleSpillFill(llvm::Function *F, SlotDepMap &depMap);

  void *Ctx = nullptr;
};

// Legacy Pass Manager wrapper.
class DivergentBarrierPassLPM : public llvm::ModulePass {
public:
  DivergentBarrierPassLPM(void *Ctx = nullptr) : llvm::ModulePass(ID), m_impl(Ctx) {}
  bool runOnModule(llvm::Module &M) override;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  static char ID;

  llvm::StringRef getPassName() const override { return "DivergentBarrierPass"; }

private:
  DivergentBarrierPass m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class DivergentBarrierPassNPM : public llvm::PassInfoMixin<DivergentBarrierPassNPM> {
public:
  DivergentBarrierPassNPM(void *Ctx = nullptr) : Ctx(Ctx) {}
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "divergent-barrier-pass"; }
  static bool isRequired() { return true; }

private:
  void *Ctx = nullptr;
};
#endif // LLVM_VERSION_MAJOR >= 16

void initializeDivergentBarrierPassLPMPass(llvm::PassRegistry &);
llvm::ModulePass *createDivergentBarrierPass(void *Ctx);
} // namespace IGC
