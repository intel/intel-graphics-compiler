/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/PassManager.h>
#include <llvm/ADT/MapVector.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  This pass creates annotations for OpenCL program-scope structures.
//          Currently this is program-scope constants, but for OpenCL 2.0, it should
//          also support program-scope globals.
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class ProgramScopeConstantAnalysis {
public:
  /// @brief  Constructor
  ProgramScopeConstantAnalysis() {}

  /// @brief  Destructor
  ~ProgramScopeConstantAnalysis() {}

  /// @brief  Provides name of pass
  static llvm::StringRef getPassName() { return "ProgramScopeConstantAnalysisPass"; }

  /// @brief  Main entry point.
  ///         Runs on all GlobalVariables in this module, finds the constants, and
  ///         generates annotations for them.
  /// @param  M The destination module.
  bool run(llvm::Module &M, IGC::IGCMD::MetaDataUtils *pMdUtils, IGC::CodeGenContext *pCtx,
           IGC::ModuleMetaData *pModMD);

protected:
  typedef std::vector<unsigned char> DataVector;
  typedef llvm::MapVector<llvm::GlobalVariable *, int64_t> BufferOffsetMap;

  /// @brief  Add data from the inline constant into the buffer.
  /// @param  initializer                   The initializer of the constant being added.
  /// @param  inlineProgramScopeBufferType  Selects the target buffer (Globals, Constants, or ConstantStrings).
  /// @param  inlineProgramScopeOffsets     Map tracking each global's offset within its buffer.
  /// @param  addressSpace                  Address space where the global resides.
  /// @param  forceAlignmentOne             If true, uses alignment of 1; used for packed structs.
  void addData(llvm::Constant *initializer, InlineProgramScopeBufferType inlineProgramScopeBufferType,
               BufferOffsetMap &inlineProgramScopeOffsets, unsigned addressSpace, bool forceAlignmentOne = false);

  /// @brief  Align the buffer according to the required alignment
  /// @param  buffer     The buffer to align.
  /// @param  alignment  Required alignment in bytes.
  void alignBuffer(DataVector &buffer, alignment_t alignment);

  const llvm::DataLayout *m_DL = nullptr;
  ModuleMetaData *m_pModuleMd = nullptr;
};

// Legacy Pass Manager wrapper.
class ProgramScopeConstantAnalysisLPM : public llvm::ModulePass {
public:
  static char ID;

  ProgramScopeConstantAnalysisLPM();
  ~ProgramScopeConstantAnalysisLPM() {}

  virtual llvm::StringRef getPassName() const override { return ProgramScopeConstantAnalysis::getPassName(); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  virtual bool runOnModule(llvm::Module &M) override {
    return ProgramScopeConstantAnalysis().run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                              getAnalysis<CodeGenContextWrapper>().getCodeGenContext(),
                                              getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class ProgramScopeConstantAnalysisNPM : public llvm::PassInfoMixin<ProgramScopeConstantAnalysisNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-programscope-constant-analysis"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
