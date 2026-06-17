/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include <llvm/IR/DataLayout.h>
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "IGC/common/Types.hpp"

namespace IGC {
/// @brief  NamedBarriersResolution pass used for resolving OpenCL named barriers functions.
// Shared implementation. Holds the logic and the pass configuration (m_GFX_CORE) and is
// used by both the legacy and the new-pass-manager wrappers below; it is not itself an
// llvm::Pass.
class NamedBarriersResolution : public llvm::InstVisitor<NamedBarriersResolution, void> {
public:
  /// @brief  Constructor
  NamedBarriersResolution() : m_GFX_CORE(IGFX_UNKNOWN_CORE) {}
  NamedBarriersResolution(GFXCORE_FAMILY GFX_CORE) : m_GFX_CORE(GFX_CORE) {}

  /// @brief  Destructor
  ~NamedBarriersResolution() {}

  /// @brief  Provides name of pass
  static llvm::StringRef getPassName() { return "NamedBarriersResolution"; }

  bool run(llvm::Module &M, ModuleMetaData *pModMD);

  /// @brief  Call instructions visitor.
  ///         Checks for OpenCL Named Barier init  functions and resolves them into appropriate sequence of code
  /// @param  CI The call instruction.
  void visitCallInst(llvm::CallInst &CI);

  void initGlobalVariables(llvm::Module *, llvm::Type *);

  static const char *NAMED_BARRIERS_INIT;
  static const char *NAMED_BARRIERS_BARRIER_ARG2;
  static const char *NAMED_BARRIERS_BARRIER_ARG3;
  const int GetMaxNamedBarriers();

  static int AlignNBCnt2BarrierNumber(uint NBCnt);
  static bool NamedBarrierHWSupport(GFXCORE_FAMILY GFX_CORE);

  enum NamedBarrierType { ProducerConsumer = 0, Producer = 1, Consumer = 2 };

  static void CallSignal(llvm::Value *barrierID, llvm::Value *ProducerCnt, llvm::Value *ConsumerCnt,
                         NamedBarrierType Type, llvm::Instruction *pInsertBefore);
  static void CallWait(llvm::Value *barrierID, llvm::Instruction *pInsertBefore);

private:
  GFXCORE_FAMILY m_GFX_CORE;
  llvm::Type *m_NamedBarrierType = nullptr;
  llvm::GlobalVariable *m_NamedBarrierID = nullptr;
  llvm::GlobalVariable *m_NamedBarrierArray = nullptr;
  /// @brief  Indicates if the pass changed the processed function
  inline bool IsNamedBarriersAdded() { return m_CountNamedBarriers > 0; }

  int m_CountNamedBarriers = 0;

  bool isNamedBarrierInit(llvm::StringRef &FuncionName);
  bool isNamedBarrierSync(llvm::StringRef &FuncionName);

  void HandleNamedBarrierInitSW(llvm::CallInst &NBarrierInitCall);
  void HandleNamedBarrierSyncSW(llvm::CallInst &NBarrierSyncCall);

  struct s_namedBarrierInfo {
    llvm::Value *threadGroupNBarrierID;
    llvm::Value *threadGroupNBarrierCount;
    llvm::CallInst *threadGroupNBarrierInit;
  };
  llvm::Value *FindAllocStructNBarrier(llvm::Value *Val, bool IsNBarrierInitCall);

  llvm::DenseMap<llvm::Value *, s_namedBarrierInfo> m_MapInitToID;

  void HandleNamedBarrierInitHW(llvm::CallInst &NBarrierInitCall);
  void HandleNamedBarrierSyncHW(llvm::CallInst &NBarrierSyncCall);
};

// Legacy Pass Manager wrapper.
class NamedBarriersResolutionLPM : public llvm::ModulePass {
public:
  static char ID;

  NamedBarriersResolutionLPM();
  NamedBarriersResolutionLPM(GFXCORE_FAMILY GFX_CORE);
  ~NamedBarriersResolutionLPM() {}

  virtual llvm::StringRef getPassName() const override { return NamedBarriersResolution::getPassName(); }

  virtual bool runOnModule(llvm::Module &M) override {
    return m_impl.run(M, getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<MetaDataUtilsWrapper>(); }

private:
  NamedBarriersResolution m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Carries the same configuration as the legacy pass via its
// constructors. name() returns the legacy pass argument so PrintBefore/PrintAfter
// matches under the new pass manager.
class NamedBarriersResolutionNPM : public llvm::PassInfoMixin<NamedBarriersResolutionNPM> {
public:
  NamedBarriersResolutionNPM() {}
  NamedBarriersResolutionNPM(GFXCORE_FAMILY GFX_CORE) : m_impl(GFX_CORE) {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-named-barriers-resolution"; }
  static bool isRequired() { return true; }

private:
  NamedBarriersResolution m_impl;
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
