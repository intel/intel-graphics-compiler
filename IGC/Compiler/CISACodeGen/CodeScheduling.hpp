/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/IGCLivenessAnalysis.h"
#include "Compiler/CISACodeGen/VectorShuffleAnalysis.hpp"
#include "Compiler/CISACodeGen/RematChainsAnalysis.hpp"
#include "Compiler/CISACodeGen/TranslationTable.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/LoopInfo.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
class CodeScheduling : public llvm::FunctionPass {
  // llvm::DominatorTree* DT = nullptr;
  // llvm::PostDominatorTree* PDT = nullptr;
  // llvm::LoopInfo* LI = nullptr;
  llvm::AliasAnalysis *AA = nullptr;
  VectorShuffleAnalysis *VSA = nullptr;
  RematChainsAnalysis *RCA = nullptr;
  WIAnalysisRunner *WI = nullptr;
  // IGCMD::MetaDataUtils* MDUtils = nullptr;
  IGCLivenessAnalysis *RPE = nullptr;
  IGCFunctionExternalRegPressureAnalysis *FRPE = nullptr;
  CodeGenContext *CTX = nullptr;

public:
  static char ID; // Pass identification

  CodeScheduling();

  virtual bool runOnFunction(llvm::Function &F) override;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();

    // AU.addRequired<llvm::DominatorTreeWrapperPass>();
    // AU.addRequired<llvm::LoopInfoWrapperPass>();
    // AU.addRequired<llvm::AAResultsWrapperPass>();
    AU.addRequired<IGCLivenessAnalysis>();
    AU.addRequired<IGCFunctionExternalRegPressureAnalysis>();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<VectorShuffleAnalysis>();
    AU.addRequired<RematChainsAnalysis>();

    // AU.addPreserved<llvm::DominatorTreeWrapperPass>();
    // AU.addPreserved<llvm::LoopInfoWrapperPass>();
    // AU.addPreserved<llvm::AAResultsWrapperPass>();
    AU.addPreserved<IGCLivenessAnalysis>();
    AU.addPreserved<IGCFunctionExternalRegPressureAnalysis>();
    AU.addPreserved<VectorShuffleAnalysis>();
    AU.addPreserved<RematChainsAnalysis>();
  }

private:
  /// dumping
  std::string Log;
  llvm::raw_string_ostream LogStringStream;
  llvm::raw_ostream *LogStream = nullptr;

  void dumpToFile(const std::string &Log);
};

void initializeCodeSchedulingPass(llvm::PassRegistry &);
} // namespace IGC
