/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataApi/MetaDataApi.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/MDFrameWork.h"

namespace IGC {
// This pass provides access to the metadata api.
//
// To use this from within another pass:
//  1. Add MetaDataUtilsWrapper to the analysis usage of the pass:
//      virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {AU.addRequired<MetaDataUtilsWrapper>();}
//  2. Add MetaDataUtilsWrapper as a dependency of the pass:
//      IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
//  3. Use getAnalysis on MetaDataUtilsWrapper:
//      MetaDataUtilsWrapper &mduw = getAnalysis<MetaDataUtilsWrapper>();
//  4. Get the MetaDataUtils:
//      MetaDataUtils *mdUtils = mduw.getMetaDataUtils();
class MetaDataUtilsWrapper : public llvm::ImmutablePass {
public:
  static char ID;

  // Constructs a wrapper to an uninitialized MetaDataUtils instance.
  // To complete initialization, a module must be passed to MetaDataUtils by
  // calling getMetaDataUtils()->setModule(), or by making sure that
  // MetaDataUtilsWrapperInitializer pass runs before using the MetaDataUtils.
  //
  // MetaDataUtilsWrapper will be the owner of the MetaDataUtils, and will be
  // responsible for freeing the allocated memory.
  MetaDataUtilsWrapper();

  // Constructs a wrapper to the given MetaDataUtils instance.
  //
  // MetaDataUtilsWrapper will NOT be the owner of the MetaDataUtils.
  MetaDataUtilsWrapper(IGCMD::MetaDataUtils *pMdUtils, ModuleMetaData *moduleMD = nullptr);

  ~MetaDataUtilsWrapper();
  MetaDataUtilsWrapper(const MetaDataUtilsWrapper &) = delete;
  MetaDataUtilsWrapper &operator=(const MetaDataUtilsWrapper &) = delete;

  IGCMD::MetaDataUtils *getMetaDataUtils();
  ModuleMetaData *getModuleMetaData();

  virtual llvm::StringRef getPassName() const override { return "MetaData Utils Wrapper"; }

private:
  IGCMD::MetaDataUtils *m_pMdUtils;
  IGC::ModuleMetaData *modMD;
  bool m_isUtilsOwner;
};

#if LLVM_VERSION_MAJOR >= 16
// Result of MetaDataUtilsAnalysis: the same pointers held by the legacy
// MetaDataUtilsWrapper ImmutablePass.
struct MetaDataUtilsResult {
  IGCMD::MetaDataUtils *MdUtils = nullptr;
  ModuleMetaData *ModMD = nullptr;
};

// New Pass Manager analysis exposing the MetaDataUtils/ModuleMetaData to ported
// passes. Seeded externally (does not compute anything from the module); register
// it with a ModuleAnalysisManager via:
//   MAM.registerPass([Md, ModMD] { return MetaDataUtilsAnalysis(Md, ModMD); });
// and retrieve it in an NPM pass via:
//   MetaDataUtilsResult md = AM.getResult<MetaDataUtilsAnalysis>(M);
class MetaDataUtilsAnalysis : public llvm::AnalysisInfoMixin<MetaDataUtilsAnalysis> {
  friend llvm::AnalysisInfoMixin<MetaDataUtilsAnalysis>;
  static llvm::AnalysisKey Key;

  MetaDataUtilsResult m_result;

public:
  using Result = MetaDataUtilsResult;

  MetaDataUtilsAnalysis() = default;
  MetaDataUtilsAnalysis(IGCMD::MetaDataUtils *Md, ModuleMetaData *ModMD) : m_result{Md, ModMD} {}

  Result run(llvm::Module &, llvm::ModuleAnalysisManager &) { return m_result; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
