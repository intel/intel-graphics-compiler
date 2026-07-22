/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/ADT/StringRef.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-opencl-printf-analysis"
#define PASS_DESCRIPTION "Analyzes OpenCL printf calls"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(OpenCLPrintfAnalysisLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(OpenCLPrintfAnalysisLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char OpenCLPrintfAnalysisLPM::ID = 0;

OpenCLPrintfAnalysisLPM::OpenCLPrintfAnalysisLPM() : ModulePass(ID) {
  initializeOpenCLPrintfAnalysisLPMPass(*PassRegistry::getPassRegistry());
}

// TODO: move to a common place
const StringRef OpenCLPrintfAnalysis::OPENCL_PRINTF_FUNCTION_NAME = "printf";
const StringRef OpenCLPrintfAnalysis::BUILTIN_PRINTF_FUNCTION_NAME = "__builtin_IB_printf_to_buffer";

bool OpenCLPrintfAnalysis::isOpenCLPrintf(const llvm::Function *F) {
  return F->getName() == OPENCL_PRINTF_FUNCTION_NAME;
}

bool OpenCLPrintfAnalysis::isBuiltinPrintf(const llvm::Function *F) {
  return F->getName() == BUILTIN_PRINTF_FUNCTION_NAME;
}

bool OpenCLPrintfAnalysis::run(Module &M, IGCMD::MetaDataUtils *pMdUtils, ModuleMetaData *pModMD) {
  m_pMDUtils = pMdUtils;
  m_modMD = pModMD;

  visit(M);
  bool changed = false;
  if (m_hasPrintfs.size()) {
    for (Function &func : M.getFunctionList()) {
      if (!func.isDeclaration() && m_hasPrintfs.find(&func) != m_hasPrintfs.end()) {
        addPrintfBufferArgs(func);
        changed = true;

        m_modMD->FuncMD[&func].hasPrintfCalls = true;
      }
    }
  }

  // Update LLVM metadata based on IGC MetadataUtils
  if (changed)
    m_pMDUtils->save(M.getContext());

  return m_hasPrintfs.size();
}

void OpenCLPrintfAnalysis::visitCallInst(CallInst &callInst) {
  Function *pF = callInst.getParent()->getParent();
  if (!callInst.getCalledFunction() || m_hasPrintfs.find(pF) != m_hasPrintfs.end()) {

    if (callInst.isIndirectCall()) {
      m_modMD->FuncMD[pF].hasIndirectCalls = true;
    }
    return;
  }

  StringRef funcName = callInst.getCalledFunction()->getName();
  bool hasPrintf = (funcName == OpenCLPrintfAnalysis::OPENCL_PRINTF_FUNCTION_NAME);
  if (hasPrintf) {
    m_hasPrintfs.insert(pF);
  }
}

void OpenCLPrintfAnalysis::addPrintfBufferArgs(Function &F) {
  SmallVector<ImplicitArg::ArgType, 1> implicitArgs;
  implicitArgs.push_back(ImplicitArg::PRINTF_BUFFER);
  ImplicitArgs::addImplicitArgs(F, implicitArgs, m_pMDUtils, m_modMD);
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses OpenCLPrintfAnalysisNPM::run(Module &M, ModuleAnalysisManager &AM) {
  auto &MDU = AM.getResult<MetaDataUtilsAnalysis>(M);
  bool changed = OpenCLPrintfAnalysis().run(M, MDU.MdUtils, MDU.ModMD);
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
