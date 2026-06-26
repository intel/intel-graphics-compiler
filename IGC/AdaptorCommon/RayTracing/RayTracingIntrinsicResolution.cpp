/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// Replace specified raytracing intrinsics with implicit args.
///
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include "RTBuilder.h"
#include "common/LLVMUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "AdaptorCommon/ImplicitArgs.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstVisitor.h"
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"
#include "AdaptorCommon/RayTracing/RayTracingPasses.hpp"

using namespace IGC;
using namespace llvm;

//////////////////////////////////////////////////////////////////////////
//
// Shared implementation. Now that we have implicit args, replace the intrinsics. Used by both the
// legacy and the new-pass-manager wrappers; it is not itself an llvm::Pass. The MetaDataUtils is
// injected by the caller (runOnFunction).
class RayTracingIntrinsicResolution : public InstVisitor<RayTracingIntrinsicResolution> {
public:
  RayTracingIntrinsicResolution() {}
  bool runOnFunction(Function &F, IGCMD::MetaDataUtils *MdUtils, IGC::ModuleMetaData *ModMD);

  void visitCallInst(CallInst &CI);

private:
  Value *getImplicitArg(Function *F, ImplicitArg::ArgType argType);

  ImplicitArgs m_implicitArgs;
  bool Changed = false;
  IGCMD::MetaDataUtils *m_pMdUtils = nullptr;
  IGC::ModuleMetaData *m_modMD = nullptr;
};

// Legacy Pass Manager wrapper.
class RayTracingIntrinsicResolutionLPM : public FunctionPass {
public:
  RayTracingIntrinsicResolutionLPM();
  bool runOnFunction(Function &F) override {
    return m_impl.runOnFunction(F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }
  StringRef getPassName() const override { return "RayTracingIntrinsicResolution"; }
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  static char ID;

private:
  RayTracingIntrinsicResolution m_impl;
};

#define PASS_FLAG "raytracing-intrinsic-resolution"
#define PASS_DESCRIPTION "replace intrinsics with implicit args"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RayTracingIntrinsicResolutionLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(RayTracingIntrinsicResolutionLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

Value *RayTracingIntrinsicResolution::getImplicitArg(Function *F, ImplicitArg::ArgType argType) {
  return m_implicitArgs.getImplicitArgValue(*F, argType, m_modMD);
}

RayTracingIntrinsicResolutionLPM::RayTracingIntrinsicResolutionLPM() : FunctionPass(ID) {
  initializeRayTracingIntrinsicResolutionLPMPass(*PassRegistry::getPassRegistry());
}

char RayTracingIntrinsicResolutionLPM::ID = 0;

void RayTracingIntrinsicResolution::visitCallInst(CallInst &CI) {
  Value *Arg = nullptr;
  Function *F = CI.getFunction();
  if (auto *GII = dyn_cast<GenIntrinsicInst>(&CI)) {
    switch (GII->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_GlobalBufferPointer:
      Arg = getImplicitArg(F, ImplicitArg::RT_GLOBAL_BUFFER_POINTER);
      break;
    case GenISAIntrinsic::GenISA_LocalBufferPointer:
      Arg = getImplicitArg(F, ImplicitArg::RT_LOCAL_BUFFER_POINTER);
      break;
    case GenISAIntrinsic::GenISA_AsyncStackID:
      Arg = getImplicitArg(F, ImplicitArg::RT_STACK_ID);
      break;
    case GenISAIntrinsic::GenISA_InlinedData: {
      // the global and local pointer are both passed in so the argument
      // is a vector of two pointers.
      Arg = getImplicitArg(F, ImplicitArg::RT_INLINED_DATA);
      IRBuilder<> IRB(&CI);
      Arg = IRB.CreateExtractElement(Arg, CI.getOperand(0));
      break;
    }
    default:
      break;
    }
  }

  if (!Arg)
    return;

  IRBuilder<> IRB(&CI);
  Arg = IRB.CreateBitCast(Arg, CI.getType());

  CI.replaceAllUsesWith(Arg);
  CI.eraseFromParent();

  Changed = true;
}

bool RayTracingIntrinsicResolution::runOnFunction(Function &F, IGCMD::MetaDataUtils *MdUtils,
                                                  IGC::ModuleMetaData *ModMD) {
  m_pMdUtils = MdUtils;
  m_modMD = ModMD;

  if (m_pMdUtils->findFunctionsInfoItem(&F) == m_pMdUtils->end_FunctionsInfo())
    return false;

  Changed = false;
  m_implicitArgs = ImplicitArgs(F, m_pMdUtils, m_modMD);
  visit(F);

  return Changed;
}

namespace IGC {
Pass *createRayTracingIntrinsicResolutionPass() { return new RayTracingIntrinsicResolutionLPM(); }

#if LLVM_VERSION_MAJOR >= 16
llvm::PreservedAnalyses RayTracingIntrinsicResolutionNPM::run(llvm::Module &M, llvm::ModuleAnalysisManager &AM) {
  RayTracingIntrinsicResolution impl;
  IGCMD::MetaDataUtils *mdUtils = AM.getResult<MetaDataUtilsAnalysis>(M).MdUtils;
  IGC::ModuleMetaData *modMD = AM.getResult<MetaDataUtilsAnalysis>(M).ModMD;
  bool changed = false;
  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    changed |= impl.runOnFunction(F, mdUtils, modMD);
  }
  return changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
