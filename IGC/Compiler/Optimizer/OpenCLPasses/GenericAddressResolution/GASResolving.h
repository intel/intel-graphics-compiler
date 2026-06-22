/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GASPropagator.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/LoopInfo.h>
#include "llvm/ADT/PostOrderIterator.h"
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

#define DEBUG_TYPE "gas-resolver"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace llvm::PatternMatch;

namespace IGC {

llvm::FunctionPass *createResolveGASPass();

// Shared implementation. Holds the logic and is used by both the legacy and the new-pass-manager
// wrappers below; it is not itself an llvm::Pass. The LoopInfo, MetaDataUtils and AliasAnalysis are
// injected by the caller (runOnFunction) so the engine does not depend on getAnalysis<>.
class GASResolving {
  const unsigned GAS = ADDRESS_SPACE_GENERIC;

  BuilderType *IRB = nullptr;
  GASPropagator *Propagator = nullptr;
  IGCMD::MetaDataUtils *m_pMdUtils = nullptr;
  IGC::ModuleMetaData *m_modMD = nullptr;
  llvm::AAResults *m_AA = nullptr;

public:
  GASResolving() {}

  bool runOnFunction(Function &F, llvm::LoopInfo &LI, IGCMD::MetaDataUtils *MdUtils, llvm::AAResults *AA,
                     IGC::ModuleMetaData *ModMD);

private:
  bool resolveOnFunction(Function *) const;
  bool resolveOnBasicBlock(BasicBlock *) const;

  bool resolveMemoryFromHost(Function &) const;

  bool checkGenericArguments(Function &F) const;
  void convertLoadToGlobal(LoadInst *LI) const;
  bool isLoadGlobalCandidate(LoadInst *LI) const;

  bool canonicalizeAddrSpaceCasts(Function &F) const;
};

// Legacy Pass Manager wrapper.
class GASResolvingLPM : public FunctionPass {
public:
  static char ID;

  GASResolvingLPM() : FunctionPass(ID) { initializeGASResolvingLPMPass(*PassRegistry::getPassRegistry()); }

  bool runOnFunction(Function &F) override {
    return m_impl.runOnFunction(
        F, getAnalysis<LoopInfoWrapperPass>().getLoopInfo(), getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
        &getAnalysis<AAResultsWrapperPass>().getAAResults(), getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<AAResultsWrapperPass>();
    AU.addRequired<MetaDataUtilsWrapper>();
  }

private:
  GASResolving m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that pulls the seeded MetaDataUtilsAnalysis
// and, per defined function, the LoopInfo + AAManager results from the function analysis manager
// (the IGCNewPassManager registers the default AA pipeline). name() returns the legacy pass
// argument so PrintBefore/PrintAfter matches under the new pass manager.
class GASResolvingNPM : public llvm::PassInfoMixin<GASResolvingNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-gas-resolve"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // End namespace IGC
