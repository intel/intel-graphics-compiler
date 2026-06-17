/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OCLBIConverter.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Intrinsics.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-conv-ocl-to-common"
#define PASS_DESCRIPTION "Convert builtin functions from OpenCL to common GenISA"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(BuiltinsConverterLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(BuiltinsConverterLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char BuiltinsConverterLPM::ID = 0;

BuiltinsConverterLPM::BuiltinsConverterLPM(void) : FunctionPass(ID) {
  initializeBuiltinsConverterLPMPass(*PassRegistry::getPassRegistry());
}

bool BuiltinsConverter::fillIndexMap(Function &F) {
  ModuleMetaData *modMD = m_pCtx->getModuleMetaData();
  FunctionMetaData *funcMD = &modMD->FuncMD[&F];
  ResourceAllocMD *resAllocMD = &funcMD->resAllocMD;
  for (Function::arg_iterator arg = F.arg_begin(), e = F.arg_end(); arg != e; ++arg) {
    int argNo = (*arg).getArgNo();
    IGC_ASSERT_MESSAGE(resAllocMD->argAllocMDList.size() > 0, "ArgAllocMDList is empty.");
    ArgAllocMD *argAlloc = &resAllocMD->argAllocMDList[argNo];
    if (argAlloc->type == OtherResourceType) {
      // Other resource type has no valid index and is not needed in the map.
      continue;
    }
    m_argIndexMap[&(*arg)] = CImagesBI::ParamInfo(argAlloc->indexType, (ResourceTypeEnum)argAlloc->type,
                                                  (ResourceExtensionTypeEnum)argAlloc->extensionType);
  }

  // The sampler arguments have already been allocated indices by the ResourceAllocator.
  // So, the first sampler we can allocate here may not be 0, but is the number of
  // already allocated indices.
  m_nextSampler = resAllocMD->samplersNumType;

  return true;
}

void BuiltinsConverter::visitCallInst(llvm::CallInst &CI) {
  Function *callee = CI.getCalledFunction();
  if (!callee)
    return;

  bool resolved = m_pResolve->resolveBI(&CI);
  if (resolved) {
    CI.eraseFromParent();
  }
}

bool BuiltinsConverter::runOnFunction(Function &F, CodeGenContext *Ctx) {
  m_argIndexMap.clear();
  m_inlineIndexMap.clear();
  m_pCtx = Ctx;

  // Make sure we are running on a kernel.
  ModuleMetaData *modMD = m_pCtx->getModuleMetaData();

  if (m_pCtx->getMetaDataUtils()->findFunctionsInfoItem(&F) == m_pCtx->getMetaDataUtils()->end_FunctionsInfo() ||
      modMD->FuncMD.find(&F) == modMD->FuncMD.end()) {
    return false;
  }

  if (!fillIndexMap(F))
    return false;

  CBuiltinsResolver resolve(&m_argIndexMap, &m_inlineIndexMap, &m_nextSampler, m_pCtx);
  m_pResolve = &resolve;
  visit(F);
  return true;
}

bool BuiltinsConverterLPM::runOnFunction(Function &F) {
  return m_impl.runOnFunction(F, getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
}

#if LLVM_VERSION_MAJOR >= 16
llvm::PreservedAnalyses BuiltinsConverterNPM::run(llvm::Module &M, llvm::ModuleAnalysisManager &AM) {
  CodeGenContext *ctx = AM.getResult<CodeGenContextAnalysis>(M).Ctx;
  BuiltinsConverter impl;
  bool changed = false;
  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    changed |= impl.runOnFunction(F, ctx);
  }
  return changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16

extern "C" FunctionPass *createBuiltinsConverterPass(void) { return new BuiltinsConverterLPM(); }
