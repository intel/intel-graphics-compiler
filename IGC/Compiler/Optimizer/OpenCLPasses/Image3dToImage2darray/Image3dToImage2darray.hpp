/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief Mark 3d images as 2d image arrays if possible.
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class Image3dToImage2darray : public llvm::InstVisitor<Image3dToImage2darray> {
public:
  Image3dToImage2darray() {}

  ~Image3dToImage2darray() {}

  static llvm::StringRef getPassName() { return "Image3dToImage2darray"; }

  // Per-function entry. The caller supplies the MetaDataUtils / ModuleMetaData that the
  // legacy pass used to obtain via getAnalysis<>.
  bool runOnFunction(llvm::Function &F, IGCMD::MetaDataUtils *pMdUtils, IGC::ModuleMetaData *pModMD);

  void visitCallInst(llvm::CallInst &CI);

private:
  bool m_Changed = false;
  IGCMD::MetaDataUtils *m_MetadataUtils = nullptr;
  IGC::ModuleMetaData *m_modMD = nullptr;
  static bool createImageAnnotations(llvm::GenIntrinsicInst *pCall, unsigned imageIdx,
                                     const IGCMD::MetaDataUtils *pMdUtils, IGC::ModuleMetaData *m_modMD,
                                     const llvm::Value *pCoord);
};

// Legacy Pass Manager wrapper.
class Image3dToImage2darrayLPM : public llvm::FunctionPass {
public:
  /// @brief  Pass identification.
  static char ID;

  Image3dToImage2darrayLPM();

  ~Image3dToImage2darrayLPM() {}

  virtual llvm::StringRef getPassName() const override { return Image3dToImage2darray::getPassName(); }

  virtual bool runOnFunction(llvm::Function &F) override {
    return m_impl.runOnFunction(F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
  }

private:
  // Reused across functions, mirroring the single legacy instance whose m_Changed
  // accumulates over the module.
  Image3dToImage2darray m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that loops over the defined
// functions: the seeded MetaDataUtilsAnalysis is module-level, and IGC passes never
// use skipFunction, so the per-function result is identical. name() returns the
// legacy pass argument so PrintBefore/PrintAfter=<pass argument> matches.
class Image3dToImage2darrayNPM : public llvm::PassInfoMixin<Image3dToImage2darrayNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-3d-to-2darray"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
