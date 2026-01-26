/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/GenericAddressResolution/GenericNullPtrPropagation.hpp"
#include "Compiler/CodeGenPublic.h"

#include "IGCPassSupport.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include <llvmWrapper/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>

using namespace llvm;
using namespace IGC;

namespace {
class GenericNullPtrPropagation : public FunctionPass, public InstVisitor<GenericNullPtrPropagation> {
public:
  static char ID;

  GenericNullPtrPropagation() : FunctionPass(ID) {}
  ~GenericNullPtrPropagation() = default;

  StringRef getPassName() const override { return "GenericNullPtrPropagation"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
    AU.setPreservesCFG();
  }
  bool runOnFunction(Function &F) override;
  void visitAddrSpaceCastInst(AddrSpaceCastInst &I);

  void releaseMemory() override { toVisit.clear(); }

private:
  DominatorTree *dt = nullptr;
  CodeGenContext *m_ctx = nullptr;
  SmallPtrSet<AddrSpaceCastInst *, 8> toVisit;
};
} // namespace

#if LLVM_VERSION_MAJOR < 16
static Instruction *findNearestCommonInstDominator(Instruction *I1, Instruction *I2, DominatorTree *DT) {
  BasicBlock *BB1 = I1->getParent();
  BasicBlock *BB2 = I2->getParent();
  if (BB1 == BB2)
    return I1->comesBefore(I2) ? I1 : I2;
  if (!DT->isReachableFromEntry(BB2))
    return I1;
  if (!DT->isReachableFromEntry(BB1))
    return I2;
  BasicBlock *DomBB = DT->findNearestCommonDominator(BB1, BB2);
  if (BB1 == DomBB)
    return I1;
  if (BB2 == DomBB)
    return I2;
  return DomBB->getTerminator();
}
#endif

// Update the location of the addrspacecast to be closer to its uses if we can.
// This enables PrivateMemoryResolution pass to sink allocas better and reduce RP.
static void updateAddrSpaceCastLocation(AddrSpaceCastInst &I, DominatorTree *DT) {
  if (I.use_empty()) {
    return;
  }

  SmallVector<Instruction *, 4> UInsts;

  for (auto *U : I.users()) {
    auto *UI = dyn_cast<Instruction>(U);
    if (!UI || isa<PHINode>(UI)) {
      return;
    }
    UInsts.push_back(UI);
  }

  auto *DomInst = UInsts[0];
  for (unsigned i = 1; i < UInsts.size(); ++i) {
#if LLVM_VERSION_MAJOR >= 16
    DomInst = DT->findNearestCommonDominator(DomInst, UInsts[i]);
#else
    DomInst = findNearestCommonInstDominator(DomInst, UInsts[i], DT);
#endif
    if (!DomInst) {
      return;
    }
  }

  auto *DomBB = DomInst->getParent();
  auto FirstInsertPt = DomBB->getFirstInsertionPt();
  if (FirstInsertPt == DomBB->end() || DomInst->comesBefore(&*FirstInsertPt)) {
    return;
  }
  I.moveBefore(&*DomInst);
}

static void addChecks(AddrSpaceCastInst &I, DominatorTree *DT) {
  updateAddrSpaceCastLocation(I, DT);

  Value *src = I.getPointerOperand();
  Value *srcNull = ConstantPointerNull::get(cast<PointerType>(src->getType()));
  Value *dstNull = ConstantPointerNull::get(cast<PointerType>(I.getType()));

  IGCLLVM::IRBuilder<> builder(&I);
  auto *AddrSpaceCastCpy = builder.CreateAddrSpaceCast(src, I.getType(), I.getName());
  auto *isNotNull = builder.CreateICmpNE(src, srcNull);
  auto *select = builder.CreateSelect(isNotNull, AddrSpaceCastCpy, dstNull);

  I.replaceAllUsesWith(select);
  I.eraseFromParent();
}

#define PASS_FLAG "igc-generic-null-ptr-propagation"
#define PASS_DESCRIPTION "Propagates null pointers through addrespace casts."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GenericNullPtrPropagation, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(GenericNullPtrPropagation, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char GenericNullPtrPropagation::ID = 0;

bool GenericNullPtrPropagation::runOnFunction(Function &F) {
  m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  dt = &getAnalysis<llvm::DominatorTreeWrapperPass>().getDomTree();
  visit(F);
  llvm::for_each(toVisit, [this](AddrSpaceCastInst *I) { addChecks(*I, dt); });
  return !toVisit.empty();
}

void GenericNullPtrPropagation::visitAddrSpaceCastInst(AddrSpaceCastInst &I) {
  if ((I.getSrcAddressSpace() == ADDRESS_SPACE_LOCAL ||
       (I.getSrcAddressSpace() == ADDRESS_SPACE_PRIVATE && m_ctx->mustDistinguishBetweenPrivateAndGlobalPtr())) &&
      I.getDestAddressSpace() == ADDRESS_SPACE_GENERIC) {

    toVisit.insert(&I);
    return;
  }
}

namespace IGC {
FunctionPass *createGenericNullPtrPropagationPass() { return new GenericNullPtrPropagation; }
} // namespace IGC
