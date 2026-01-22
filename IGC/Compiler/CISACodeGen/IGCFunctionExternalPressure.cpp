/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGCLivenessAnalysis.h"

char IGCFunctionExternalRegPressureAnalysis::ID = 0;
// Register pass to igc-opt
#define PASS_FLAG2 "igc-external-pressure-analysis"
#define PASS_DESCRIPTION2                                                                                              \
  "computes full dataflow liveness analysis & and register pressure "                                                  \
  "estimator"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 true
IGC_INITIALIZE_PASS_BEGIN(IGCFunctionExternalRegPressureAnalysis, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2,
                          PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
IGC_INITIALIZE_PASS_END(IGCFunctionExternalRegPressureAnalysis, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2,
                        PASS_ANALYSIS2)
#define PRINT(str) llvm::errs() << str

IGCFunctionExternalRegPressureAnalysis::IGCFunctionExternalRegPressureAnalysis() : ModulePass(ID) {
  initializeIGCFunctionExternalRegPressureAnalysisPass(*PassRegistry::getPassRegistry());
};

WIAnalysisRunner &IGCFunctionExternalRegPressureAnalysis::runWIAnalysis(Function &F) {

  TranslationTable TT;
  TT.run(F);

  auto *DT = &getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();
  auto *PDT = &getAnalysis<PostDominatorTreeWrapperPass>(F).getPostDomTree();
  auto *LI = &getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();

  WIAnalysisMap.try_emplace(&F, &F, LI, DT, PDT, MDUtils, CGCtx, ModMD, &TT);
  auto &Runner = WIAnalysisMap[&F];
  Runner.run();
  return Runner;
}

void IGCFunctionExternalRegPressureAnalysis::generateTableOfPressure(llvm::Module &M) {

  // basic preprocessing, scan all of the functions,
  // collect callsite pressure for eash callsite
  for (auto &F : M) {
    if (F.isDeclaration())
      continue;

    unsigned int SIMD = numLanes(bestGuessSIMDSize(&F, FGA));
    livenessAnalysis(F);
    auto WI = getWIAnalysis(&F);

    for (auto &BB : F) {

      std::unique_ptr<InsideBlockPressureMap> PressureMap;
      for (auto &I : BB) {

        auto *Call = llvm::dyn_cast<CallInst>(&I);

        if (!Call)
          continue;
        if (Call->getCallingConv() != CallingConv::SPIR_FUNC)
          continue;
        if (!SetOfDefinitions.count(Call->getCalledFunction()))
          continue;

        if (!PressureMap)
          PressureMap = getPressureMapForBB(BB, SIMD, WI);

        CallSitePressure[Call] = (*PressureMap)[&I];
      }
    }
  }

  llvm::SmallVector<llvm::Function *, 16> PostOrder;

  CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  auto ExtNode = CG.getExternalCallingNode();
  for (auto I = po_begin(ExtNode), E = po_end(ExtNode); I != E; ++I) {
    auto CGNode = *I;

    if (auto F = CGNode->getFunction()) {
      if (F->isDeclaration())
        continue;
      // Ignore externally linked functions and stackcall functions
      if (F->hasFnAttribute("referenced-indirectly") || F->hasFnAttribute("invoke_simd_target") ||
          F->hasFnAttribute("hasRecursion") || F->hasFnAttribute("visaStackCall"))
        continue;

      PostOrder.push_back(F);
    }
  }

  // now it's REVERSE POST ORDER
  // we will use it to process external pressure for a function
  std::reverse(PostOrder.begin(), PostOrder.end());

  for (auto El : PostOrder) {
    unsigned int MaxPressure = 0;
    // top level functions won't go inside this cycle
    // noone is calling them
    for (auto U : El->users()) {
      CallInst *Callsite = llvm::dyn_cast<CallInst>(U);
      if (!Callsite)
        continue;
      // at this point, because we process in a specific order, every function
      // that could potentially call our function, should be processed already
      // and we know its external pressure, for a top level function it will be 0
      // and we will process them first
      unsigned int ExternalPressure = CallSitePressure[Callsite] + ExternalFunctionPressure[Callsite->getFunction()];
      MaxPressure = std::max(MaxPressure, ExternalPressure);
    }
    ExternalFunctionPressure[El] = MaxPressure;
  }
}

bool IGCFunctionExternalRegPressureAnalysis::runOnModule(llvm::Module &M) {

  FGA = getAnalysisIfAvailable<GenXFunctionGroupAnalysis>();
  CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  MDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  ModMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

  // simple check, we take every definition that we have in our module
  // and remember it, later we will check, if we have no calls to those
  // definitions, we don't have to compute external pressure
  for (auto &F : M) {
    if (F.isDeclaration())
      continue;
    if (F.getCallingConv() != CallingConv::SPIR_FUNC)
      continue;
    SetOfDefinitions.insert(&F);
  }

  // Collect WIAnalysis results for all functions
  for (auto &F : M) {
    if (F.isDeclaration())
      continue;
    runWIAnalysis(F);
  }

  if (!SetOfDefinitions.size())
    return false;

  generateTableOfPressure(M);

  return false;
}
