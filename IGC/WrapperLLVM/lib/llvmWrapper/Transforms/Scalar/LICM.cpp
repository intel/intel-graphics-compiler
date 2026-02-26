/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LazyBlockFrequencyInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/PassInstrumentation.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/LICM.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/LICM.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

#include "Compiler/IGCPassSupport.h"

using namespace llvm;
#if LLVM_VERSION_MAJOR >= 16
namespace {

// NPM AA analysis that delegates to the LPM's full AAResults chain
class LPMAABridge : public AnalysisInfoMixin<LPMAABridge>, public AAResultBase {
  friend AnalysisInfoMixin<LPMAABridge>;
  static AnalysisKey Key;
  AAResults *LPMAResults;

public:
  using Result = LPMAABridge;
  explicit LPMAABridge(AAResults *AA) : LPMAResults(AA) {}
  LPMAABridge(LPMAABridge &&) = default;

  LPMAABridge run(Function &, FunctionAnalysisManager &) { return LPMAABridge(LPMAResults); }

  AliasResult alias(const MemoryLocation &LocA, const MemoryLocation &LocB, AAQueryInfo &, const Instruction *) {
    return LPMAResults->alias(LocA, LocB);
  }
};

AnalysisKey LPMAABridge::Key;
} // anonymous namespace
#endif // LLVM_VERSION_MAJOR >= 16

namespace IGCLLVM {

LICMLegacyPassWrapper::LICMLegacyPassWrapper(unsigned LicmMssaOptCap, unsigned LicmMssaNoAccForPromotionCap,
                                             bool LicmAllowSpeculation)
    : FunctionPass(ID), LicmMssaOptCap(LicmMssaOptCap), LicmMssaNoAccForPromotionCap(LicmMssaNoAccForPromotionCap),
      LicmAllowSpeculation(LicmAllowSpeculation) {
  initializeLICMLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
}
#if LLVM_VERSION_MAJOR >= 16
bool LICMLegacyPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  // Inject IGC's AddressSpaceAAResult into the NPM FAM.
  // PassBuilder only registers standard LLVM AAs. Pre-register
  // a custom AAManager with the LPM's full chain before registerFunctionAnalyses()
  // so version wins the registration race.
  AAResults &LPMAResults = getAnalysis<AAResultsWrapperPass>().getAAResults();

  // Skip non-simplified loops (indirectbr, irreducible CFG).
  // FunctionToLoopPassAdaptor runs verifyLoop() before each pass, but these
  // loops would fail the assertion yet be skipped by LICMPass::run() anyway.
  // Returning false marks them as skipped, suppressing the assertion.
  PassInstrumentationCallbacks PIC;
  PIC.registerShouldRunOptionalPassCallback([](StringRef, Any IR) -> bool {
    const Loop *const *LPtr = any_cast<const Loop *>(&IR);
    return !LPtr || !*LPtr || (*LPtr)->isLoopSimplifyForm();
  });

  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  PassBuilder PB(nullptr, PipelineTuningOptions(), std::nullopt, &PIC);

  // Pre-register bridge analysis and custom AAManager before
  // registerFunctionAnalyses().
  FAM.registerPass([&LPMAResults] { return LPMAABridge(&LPMAResults); });
  {
    AAManager CustomAA = PB.buildDefaultAAPipeline();
    CustomAA.registerFunctionAnalysis<LPMAABridge>();
    FAM.registerPass([AA = std::move(CustomAA)]() mutable { return std::move(AA); });
  }

  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  auto Adaptor = createFunctionToLoopPassAdaptor(
      LICMPass(LicmMssaOptCap, LicmMssaNoAccForPromotionCap, LicmAllowSpeculation), /*UseMemorySSA=*/true);

  PreservedAnalyses PA = Adaptor.run(F, FAM);
  return !PA.areAllPreserved();
}
#else
bool LICMLegacyPassWrapper::runOnFunction(Function &F) {
  // LLVM < 16: Not supported.
  return false;
}
#endif // LLVM_VERSION_MAJOR >= 16

void LICMLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addPreserved<DominatorTreeWrapperPass>();
  AU.addPreserved<LoopInfoWrapperPass>();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addRequired<MemorySSAWrapperPass>();
  AU.addPreserved<MemorySSAWrapperPass>();
  AU.addRequired<TargetTransformInfoWrapperPass>();
  AU.addRequired<AssumptionCacheTracker>();
  getLoopAnalysisUsage(AU);
  LazyBlockFrequencyInfoPass::getLazyBFIAnalysisUsage(AU);
  AU.addPreserved<LazyBlockFrequencyInfoPass>();
  AU.addPreserved<LazyBranchProbabilityInfoPass>();
}

char LICMLegacyPassWrapper::ID = 0;
#if LLVM_VERSION_MAJOR >= 16
llvm::Pass *createLegacyWrappedLICMPass() { return new LICMLegacyPassWrapper(); }
llvm::Pass *createLegacyWrappedLICMPass(unsigned LicmMssaOptCap, unsigned LicmMssaNoAccForPromotionCap,
                                        bool LicmAllowSpeculation) {
  return new LICMLegacyPassWrapper(LicmMssaOptCap, LicmMssaNoAccForPromotionCap, LicmAllowSpeculation);
}
#else
llvm::Pass *createLegacyWrappedLICMPass() { return createLICMPass(); }
llvm::Pass *createLegacyWrappedLICMPass(unsigned LicmMssaOptCap, unsigned LicmMssaNoAccForPromotionCap,
                                        bool LicmAllowSpeculation) {
  return createLICMPass(LicmMssaOptCap, LicmMssaNoAccForPromotionCap, LicmAllowSpeculation);
}
#endif
} // namespace IGCLLVM

using namespace IGCLLVM;

#define PASS_FLAG "licm-legacy-wrapped"
#define PASS_DESCRIPTION "Loop Invariant Code Motion LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

IGC_INITIALIZE_PASS_BEGIN(LICMLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LazyBFIPass)
IGC_INITIALIZE_PASS_END(LICMLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
