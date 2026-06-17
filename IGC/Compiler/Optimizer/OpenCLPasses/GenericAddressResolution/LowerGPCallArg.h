/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GASPropagator.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;

namespace IGC {

llvm::ModulePass *createLowerGPCallArg();

// Shared implementation. Holds the logic and is used by both the legacy and the new-pass-manager
// wrappers below; it is not itself an llvm::Pass. The CodeGenContext, MetaDataUtils and CallGraph
// are injected by the caller (run). LoopInfo is computed on demand (DominatorTree + LoopInfo) per
// transformed function, matching the legacy on-demand getAnalysis<LoopInfoWrapperPass>(F).
class LowerGPCallArg {
public:
  bool run(Module &M, CodeGenContext *Ctx, IGCMD::MetaDataUtils *MdUtils, llvm::CallGraph &CG);

private:
  //
  // Functions to be updated.
  // NewArgs keeps track of generic pointer arguments: arg number and address space
  //
  struct ArgDesc {
    unsigned int argNo;
    unsigned int addrSpace;
  };
  using GenericPointerArgs = std::vector<ArgDesc>;

  IGCMD::MetaDataUtils *m_mdUtils = nullptr;
  CodeGenContext *m_ctx = nullptr;
  Module *m_module = nullptr;

  std::optional<unsigned> getOriginAddressSpace(Function *func, unsigned argNo);
  void updateFunctionArgs(Function *oldFunc, Function *newFunc);
  void updateAllUsesWithNewFunction(Function *oldFunc, Function *newFunc);
  void updateMetadata(Function *oldFunc, Function *newFunc);
  Function *createFuncWithLoweredArgs(Function *F, GenericPointerArgs &argsInfo);
  std::vector<Function *> findCandidates(CallGraph &CG);
};

// Legacy Pass Manager wrapper.
class LowerGPCallArgLPM : public ModulePass {
public:
  static char ID;

  LowerGPCallArgLPM() : ModulePass(ID) { initializeLowerGPCallArgLPMPass(*PassRegistry::getPassRegistry()); }

  bool runOnModule(Module &M) override {
    return m_impl.run(M, getAnalysis<CodeGenContextWrapper>().getCodeGenContext(),
                      getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                      getAnalysis<CallGraphWrapperPass>().getCallGraph());
  }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CallGraphWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
  }

  virtual StringRef getPassName() const override { return "LowerGenericPointerCallArgs"; }

private:
  LowerGPCallArg m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class LowerGPCallArgNPM : public llvm::PassInfoMixin<LowerGPCallArgNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-lower-gp-arg"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // End namespace IGC
