/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This pass and its Resolution equivalent follow the familar AdaptorOCL
/// pattern of an analysis followed by a resolution pass.  The idea early
/// on was that continuations would be invoked often as function calls so we
/// need to promote the below intrinsics to implicit args so we could later call
/// BuiltinCallGraphAnalysis to propagate the arguments through all calls.  It
/// is not currently clear how much we'll use this approach: it may be that
/// BTD + inlining of small continuations may is the way to go.
///
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include "common/LLVMUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "AdaptorCommon/ImplicitArgs.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstVisitor.h"
#include "common/LLVMWarningsPop.hpp"
#include "AdaptorCommon/RayTracing/RayTracingPasses.hpp"

using namespace IGC;
using namespace llvm;

//////////////////////////////////////////////////////////////////////////
//
// Shared implementation. Detects the use of ray tracing intrinsics and prepares them for
// addition as implicit arguments. Used by both the legacy and the new-pass-manager wrappers;
// it is not itself an llvm::Pass. The MetaDataUtils is injected by the caller (run).
class RayTracingIntrinsicAnalysis : public InstVisitor<RayTracingIntrinsicAnalysis> {
public:
  RayTracingIntrinsicAnalysis() {}
  bool run(Module &M, IGCMD::MetaDataUtils *MdUtils, IGC::ModuleMetaData *ModMD);
  bool runOnFunction(Function &F);

  void visitCallInst(CallInst &CI);

private:
  bool hasGlobalPointer = false;
  bool hasLocalPointer = false;
  bool hasStackID = false;
  bool hasInlinedData = false;
  IGCMD::MetaDataUtils *pMdUtils = nullptr;
  IGC::ModuleMetaData *m_modMD = nullptr;
};

// Legacy Pass Manager wrapper.
class RayTracingIntrinsicAnalysisLPM : public ModulePass {
public:
  RayTracingIntrinsicAnalysisLPM();
  bool runOnModule(Module &M) override {
    return m_impl.run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                      getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }
  StringRef getPassName() const override { return "RayTracingIntrinsicAnalysis"; }
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<MetaDataUtilsWrapper>(); }

  static char ID;

private:
  RayTracingIntrinsicAnalysis m_impl;
};

#define PASS_FLAG "raytracing-intrinsic-analysis"
#define PASS_DESCRIPTION "Mark raytracing intrinsics as implicit args"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RayTracingIntrinsicAnalysisLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(RayTracingIntrinsicAnalysisLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

RayTracingIntrinsicAnalysisLPM::RayTracingIntrinsicAnalysisLPM() : ModulePass(ID) {
  initializeRayTracingIntrinsicAnalysisLPMPass(*PassRegistry::getPassRegistry());
}

char RayTracingIntrinsicAnalysisLPM::ID = 0;

void RayTracingIntrinsicAnalysis::visitCallInst(CallInst &CI) {
  if (auto *GII = dyn_cast<GenIntrinsicInst>(&CI)) {
    switch (GII->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_GlobalBufferPointer:
      hasGlobalPointer = true;
      return;
    case GenISAIntrinsic::GenISA_LocalBufferPointer:
      hasLocalPointer = true;
      return;
    case GenISAIntrinsic::GenISA_AsyncStackID:
      hasStackID = true;
      return;
    case GenISAIntrinsic::GenISA_InlinedData:
      hasInlinedData = true;
      return;
    default:
      break;
    }
  }
}

bool RayTracingIntrinsicAnalysis::run(Module &M, IGCMD::MetaDataUtils *MdUtils, IGC::ModuleMetaData *ModMD) {
  bool changed = false;
  pMdUtils = MdUtils;
  m_modMD = ModMD;
  // Run on all functions defined in this module
  for (Function &F : M) {
    if (F.isDeclaration()) {
      continue;
    }
    if (runOnFunction(F)) {
      changed = true;
    }
  }

  // Update LLVM metadata based on IGC MetadataUtils
  if (changed)
    pMdUtils->save(M.getContext());

  return changed;
}

bool RayTracingIntrinsicAnalysis::runOnFunction(Function &F) {
  if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
    return false;

  hasGlobalPointer = false;
  hasLocalPointer = false;
  hasStackID = false;
  hasInlinedData = false;

  visit(F);

  bool changed = false;
  SmallVector<ImplicitArg::ArgType, ImplicitArg::NUM_IMPLICIT_ARGS> implicitArgs;

  if (hasGlobalPointer)
    implicitArgs.push_back(ImplicitArg::RT_GLOBAL_BUFFER_POINTER);

  if (hasLocalPointer)
    implicitArgs.push_back(ImplicitArg::RT_LOCAL_BUFFER_POINTER);

  if (hasStackID)
    implicitArgs.push_back(ImplicitArg::RT_STACK_ID);

  if (hasInlinedData)
    implicitArgs.push_back(ImplicitArg::RT_INLINED_DATA);

  if (!implicitArgs.empty()) {
    // Create IGC metadata representing the implicit args needed by this function
    ImplicitArgs::addImplicitArgsTotally(F, implicitArgs, pMdUtils, m_modMD);
    changed = true;
  }
  return changed;
}
namespace IGC {
Pass *createRayTracingIntrinsicAnalysisPass() { return new RayTracingIntrinsicAnalysisLPM(); }

#if LLVM_VERSION_MAJOR >= 16
llvm::PreservedAnalyses RayTracingIntrinsicAnalysisNPM::run(llvm::Module &M, llvm::ModuleAnalysisManager &AM) {
  RayTracingIntrinsicAnalysis impl;
  bool changed =
      impl.run(M, AM.getResult<MetaDataUtilsAnalysis>(M).MdUtils, AM.getResult<MetaDataUtilsAnalysis>(M).ModMD);
  return changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
