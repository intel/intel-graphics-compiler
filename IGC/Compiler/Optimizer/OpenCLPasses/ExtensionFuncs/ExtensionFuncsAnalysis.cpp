/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ExtensionFuncs/ExtensionFuncsAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-extension-funcs-analysis"
#define PASS_DESCRIPTION "Analyzes extension functions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ExtensionFuncsAnalysisLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(ExtensionFuncsAnalysisLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ExtensionFuncsAnalysisLPM::ID = 0;

ExtensionFuncsAnalysisLPM::ExtensionFuncsAnalysisLPM() : ModulePass(ID) {
  initializeExtensionFuncsAnalysisLPMPass(*PassRegistry::getPassRegistry());
}

const StringRef ExtensionFuncsAnalysis::VME_MB_BLOCK_TYPE = "__builtin_IB_vme_mb_block_type";
const StringRef ExtensionFuncsAnalysis::VME_SUBPIXEL_MODE = "__builtin_IB_vme_subpixel_mode";
const StringRef ExtensionFuncsAnalysis::VME_SAD_ADJUST_MODE = "__builtin_IB_vme_sad_adjust_mode";
const StringRef ExtensionFuncsAnalysis::VME_SEARCH_PATH_TYPE = "__builtin_IB_vme_search_path_type";
const StringRef ExtensionFuncsAnalysis::VME_HELPER_GET_HANDLE = "__builtin_IB_vme_helper_get_handle";
const StringRef ExtensionFuncsAnalysis::VME_HELPER_GET_AS = "__builtin_IB_vme_helper_get_as";

bool ExtensionFuncsAnalysis::run(Module &M, IGCMD::MetaDataUtils *pMdUtils, IGC::ModuleMetaData *pModMD) {
  bool changed = false;
  m_pMDUtils = pMdUtils;
  m_modMD = pModMD;
  // Run on all functions defined in this module
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    Function *pFunc = &(*I);
    if (pFunc->isDeclaration())
      continue;
    if (runOnFunction(*pFunc)) {
      changed = true;
    }
  }

  // Update LLVM metadata based on IGC MetadataUtils
  if (changed)
    m_pMDUtils->save(M.getContext());

  return changed;
}

bool ExtensionFuncsAnalysis::runOnFunction(Function &F) {
  // Processing new function
  m_hasVME = false;

  // Visit the function
  visit(F);

  // Check if VME implicit information is needed based on the function analysis
  if (!m_hasVME)
    return false;

  // Add the implicit arguments needed by this function
  SmallVector<ImplicitArg::ArgType, ImplicitArg::NUM_IMPLICIT_ARGS> implicitArgs;

  implicitArgs.push_back(ImplicitArg::VME_MB_BLOCK_TYPE);
  implicitArgs.push_back(ImplicitArg::VME_SUBPIXEL_MODE);
  implicitArgs.push_back(ImplicitArg::VME_SAD_ADJUST_MODE);
  implicitArgs.push_back(ImplicitArg::VME_SEARCH_PATH_TYPE);

  // Create the metadata representing the VME implicit args needed by this function
  ImplicitArgs::addImplicitArgs(F, implicitArgs, m_pMDUtils, m_modMD);

  return true;
}

void ExtensionFuncsAnalysis::visitCallInst(CallInst &CI) {
  // Check for VME function calls
  if (Function *F = CI.getCalledFunction()) {
    StringRef funcName = F->getName();
    if (funcName == (VME_MB_BLOCK_TYPE) || funcName == (VME_SUBPIXEL_MODE) || funcName == (VME_SAD_ADJUST_MODE) ||
        funcName == (VME_SEARCH_PATH_TYPE) || IGCLLVM::starts_with(funcName, VME_HELPER_GET_HANDLE) ||
        IGCLLVM::starts_with(funcName, VME_HELPER_GET_AS)) {
      m_hasVME = true;
    }
  }
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses ExtensionFuncsAnalysisNPM::run(Module &M, ModuleAnalysisManager &AM) {
  bool changed = ExtensionFuncsAnalysis().run(M, AM.getResult<MetaDataUtilsAnalysis>(M).MdUtils,
                                              AM.getResult<MetaDataUtilsAnalysis>(M).ModMD);
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
