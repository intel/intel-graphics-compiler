/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncsAnalysis.hpp"
#include "FoldKnownWorkGroupSizes.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"

#include "LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include <llvm/IR/InstVisitor.h>
#include "LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGCMD;

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class FoldKnownWorkGroupSizes : public llvm::InstVisitor<FoldKnownWorkGroupSizes> {
private:
  CodeGenContext *ctx = nullptr;
  bool RequirePayloadHeader = true;

public:
  FoldKnownWorkGroupSizes() {}
  ~FoldKnownWorkGroupSizes() {}

  bool runOnFunction(llvm::Function &F, CodeGenContext *pCtx);
  void visitCallInst(llvm::CallInst &I);

  static llvm::StringRef getPassName() { return "FoldKnownWorkGroupSizes"; }
};

// Legacy Pass Manager wrapper.
class FoldKnownWorkGroupSizesLPM : public llvm::FunctionPass {
public:
  static char ID;
  FoldKnownWorkGroupSizesLPM() : FunctionPass(ID) {
    initializeFoldKnownWorkGroupSizesLPMPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(llvm::Function &F) override {
    return m_impl.runOnFunction(F, getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }

  llvm::StringRef getPassName() const override { return FoldKnownWorkGroupSizes::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<IGC::CodeGenContextWrapper>(); }

private:
  FoldKnownWorkGroupSizes m_impl;
};

bool m_changed = false;
char FoldKnownWorkGroupSizesLPM::ID = 0;

#define PASS_FLAG "igc-fold-workgroup-sizes"
#define PASS_DESCRIPTION "Fold global offset and enqueued local sizes"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(FoldKnownWorkGroupSizesLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(FoldKnownWorkGroupSizesLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

#if LLVM_VERSION_MAJOR >= 16
llvm::PreservedAnalyses FoldKnownWorkGroupSizesNPM::run(llvm::Module &M, llvm::ModuleAnalysisManager &AM) {
  CodeGenContext *pCtx = AM.getResult<CodeGenContextAnalysis>(M).Ctx;
  FoldKnownWorkGroupSizes impl;
  bool changed = false;
  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    changed |= impl.runOnFunction(F, pCtx);
  }
  return changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC

bool FoldKnownWorkGroupSizes::runOnFunction(Function &F, CodeGenContext *pCtx) {
  ctx = pCtx;
  RequirePayloadHeader = ctx->m_DriverInfo.RequirePayloadHeader();
  visit(F);
  return m_changed;
}

void FoldKnownWorkGroupSizes::visitCallInst(llvm::CallInst &I) {
  Function *function = I.getParent()->getParent();
  Module *module = function->getParent();
  Function *calledFunction = I.getCalledFunction();
  if (calledFunction == nullptr) {
    return;
  }
  StringRef funcName = calledFunction->getName();

  if (funcName == WIFuncsAnalysis::GET_GLOBAL_OFFSET && ctx->getModuleMetaData()->compOpt.replaceGlobalOffsetsByZero) {
    if (calledFunction->getReturnType() == Type::getInt32Ty(module->getContext())) {
      ConstantInt *IntZero = ConstantInt::get(Type::getInt32Ty(module->getContext()), 0);
      I.replaceAllUsesWith(IntZero);
      if (!RequirePayloadHeader)
        I.eraseFromParent();
      m_changed = true;
    }
  } else if (funcName == WIFuncsAnalysis::GET_ENQUEUED_LOCAL_SIZE) {
    auto Dims = IGCMetaDataHelper::getThreadGroupDims(ctx->getModuleMetaData(), I.getFunction());

    if (!Dims)
      return;

    IRBuilder<> IRB(&I);

    auto *CV = ConstantDataVector::get(I.getContext(), *Dims);

    auto *Dim = I.getArgOperand(0);
    auto *EE = IRB.CreateExtractElement(CV, Dim, "enqueuedLocalSize");

    I.replaceAllUsesWith(EE);
    // TODO: erase when patch token is not required
    // I.eraseFromParent();
    m_changed = true;
  }
}

namespace IGC {
llvm::FunctionPass *CreateFoldKnownWorkGroupSizes() { return new FoldKnownWorkGroupSizesLPM(); }
} // namespace IGC
