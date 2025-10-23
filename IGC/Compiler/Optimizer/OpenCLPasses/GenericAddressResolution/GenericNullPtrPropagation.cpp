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
      }
      bool runOnFunction(Function &F) override;
      void visitAddrSpaceCastInst(AddrSpaceCastInst &I);
    private:
      bool m_modified = false;
      CodeGenContext *m_ctx = nullptr;
  };
}

#define PASS_FLAG "igc-generic-null-ptr-propagation"
#define PASS_DESCRIPTION "Propagates null pointers through addrespace casts."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GenericNullPtrPropagation, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(GenericNullPtrPropagation, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char GenericNullPtrPropagation::ID = 0;

bool GenericNullPtrPropagation::runOnFunction(Function &F) {
  m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  visit(F);
  return m_modified;
}

void GenericNullPtrPropagation::visitAddrSpaceCastInst(AddrSpaceCastInst &I) {
  if ((I.getSrcAddressSpace() == ADDRESS_SPACE_LOCAL
    || (I.getSrcAddressSpace() == ADDRESS_SPACE_PRIVATE && m_ctx->mustDistinguishBetweenPrivateAndGlobalPtr()))
    && I.getDestAddressSpace() == ADDRESS_SPACE_GENERIC) {
    Value* src = I.getPointerOperand();
    Value* srcNull = ConstantPointerNull::get(cast<PointerType>(src->getType()));
    Value* dstNull = ConstantPointerNull::get(cast<PointerType>(I.getType()));

    IGCLLVM::IRBuilder<> builder(&I);
    auto *AddrSpaceCastCpy = builder.CreateAddrSpaceCast(src, I.getType(), I.getName());
    auto *isNotNull = builder.CreateICmpNE(src, srcNull);
    auto *select = builder.CreateSelect(isNotNull, AddrSpaceCastCpy, dstNull);

    I.replaceAllUsesWith(select);
    I.eraseFromParent();
    m_modified = true;
  }
}

namespace IGC {
FunctionPass *createGenericNullPtrPropagationPass() { return new GenericNullPtrPropagation; }
} // namespace IGC

