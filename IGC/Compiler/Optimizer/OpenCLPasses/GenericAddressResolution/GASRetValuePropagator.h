/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GASPropagator.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/CallGraph.h"
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;

namespace IGC {

llvm::ModulePass *createGASRetValuePropagatorPass();

// Shared implementation. Holds the logic and is used by both the legacy and the new-pass-manager
// wrappers below; it is not itself an llvm::Pass. The CodeGenContext, MetaDataUtils and CallGraph
// are injected by the caller (run). LoopInfo is computed on demand per candidate function.
class GASRetValuePropagator {
  Module *m_module = nullptr;
  IGCMD::MetaDataUtils *m_mdUtils = nullptr;
  CodeGenContext *m_ctx = nullptr;
  GASPropagator *m_Propagator = nullptr;

public:
  bool run(Module &M, CodeGenContext *Ctx, IGCMD::MetaDataUtils *MdUtils, llvm::CallGraph &CG);

  bool propagateReturnValue(Function *&);
  std::vector<Function *> findCandidates(CallGraph &);

private:
  std::vector<ReturnInst *> getAllRetInstructions(Function &);
  void updateFunctionRetInstruction(Function *);
  PointerType *getRetValueNonGASType(Function *);
  void transferFunctionBody(Function *, Function *);
  void updateAllUsesWithNewFunction(Function *, Function *);
  void updateMetadata(Function *, Function *);
  void updateDwarfAddressSpace(Function *);
  DIDerivedType *getDIDerivedTypeWithDwarfAddrspace(DIDerivedType *, unsigned);
  Function *createNewFunctionDecl(Function *, Type *);
  Function *cloneFunctionWithModifiedRetType(Function *, PointerType *);
};

// Legacy Pass Manager wrapper.
class GASRetValuePropagatorLPM : public ModulePass {
public:
  static char ID;

  GASRetValuePropagatorLPM() : ModulePass(ID) {
    initializeGASRetValuePropagatorLPMPass(*PassRegistry::getPassRegistry());
  }

  bool runOnModule(Module &M) override {
    return m_impl.run(M, getAnalysis<CodeGenContextWrapper>().getCodeGenContext(),
                      getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                      getAnalysis<CallGraphWrapperPass>().getCallGraph());
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<CallGraphWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
  }

private:
  GASRetValuePropagator m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class GASRetValuePropagatorNPM : public llvm::PassInfoMixin<GASRetValuePropagatorNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-gas-ret-value-propagator"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // End namespace IGC
