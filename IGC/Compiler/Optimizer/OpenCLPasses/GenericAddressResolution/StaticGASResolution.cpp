/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "StaticGASResolution.h"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/CallGraph.h>
#include "common/LLVMWarningsPop.hpp"

FunctionPass *IGC::createStaticGASResolution() { return new StaticGASResolutionLPM(); }

char StaticGASResolutionLPM::ID = 0;

#define PASS_FLAG "static-gas-resolution"
#define PASS_DESC "Statically resolves memory accesses operating on generic pointers"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(StaticGASResolutionLPM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CastToGASAnalysis)
IGC_INITIALIZE_PASS_END(StaticGASResolutionLPM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

#if LLVM_VERSION_MAJOR >= 16
llvm::PreservedAnalyses StaticGASResolutionNPM::run(llvm::Module &M, llvm::ModuleAnalysisManager &AM) {
  CodeGenContext *ctx = AM.getResult<CodeGenContextAnalysis>(M).Ctx;
  CallGraph &CG = AM.getResult<llvm::CallGraphAnalysis>(M);

  // Compute the GASInfo inline (matches the legacy getAnalysis<CastToGASAnalysis>()).
  CastToGASAnalysis gasAnalysis;
  gasAnalysis.computeGASInfo(M, CG, ctx);
  GASInfo &GI = gasAnalysis.getGASInfo();

  StaticGASResolution impl;
  bool changed = false;
  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    changed |= impl.runOnFunction(F, &GI);
  }
  return changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16

bool StaticGASResolution::runOnFunction(llvm::Function &F, GASInfo *GI) {
  m_GI = GI;
  // Change GAS inst, such as ld/st, etc to global ld/st, etc.
  if (m_GI->canGenericPointToPrivate(F) || m_GI->canGenericPointToLocal(F))
    return false;

  // As AddrSpaceCast has been processed already in GASResolving,
  // here only handle non-addrspacecast ptr
  auto toSkip = [](Value *P) {
    if (PointerType *PtrTy = dyn_cast<PointerType>(P->getType())) {
      if (PtrTy->getAddressSpace() == ADDRESS_SPACE_GENERIC && !isa<AddrSpaceCastInst>(P)) {
        return false;
      }
    }
    return true;
  };

  IRBuilder<> IRB(F.getContext());
  bool changed = false;
  auto NI = inst_begin(F);
  for (auto FI = NI, FE = inst_end(F); FI != FE; FI = NI) {
    ++NI;

    Instruction *I = &(*FI);
    LoadInst *LI = dyn_cast<LoadInst>(I);
    StoreInst *SI = dyn_cast<StoreInst>(I);
    if (LI || SI) {
      Value *Ptr = LI ? LI->getPointerOperand() : SI->getPointerOperand();
      if (!toSkip(Ptr)) {
        PointerType *PtrTy = cast<PointerType>(Ptr->getType());
        PointerType *glbPtrTy = IGCLLVM::PointerType::get(PtrTy, ADDRESS_SPACE_GLOBAL);

        IRB.SetInsertPoint(I);
        Value *NewPtr = IRB.CreateAddrSpaceCast(Ptr, glbPtrTy);
        I->setOperand(LI ? 0 : 1, NewPtr);
        if (Instruction *tI = dyn_cast<Instruction>(NewPtr)) {
          tI->setDebugLoc(I->getDebugLoc());
        }

        changed = true;
      }
    }

    // When there is no Private/Local to Generic AS casting it is safe
    // to replace corresponding builtin calls by nullptr.
    CallInst *CI = dyn_cast<CallInst>(I);
    if (CI) {
      Function *pCalledFunc = CI->getCalledFunction();
      if (pCalledFunc &&
          (pCalledFunc->getName() == "__builtin_IB_to_private" || pCalledFunc->getName() == "__builtin_IB_to_local")) {
        IGC_ASSERT(IGCLLVM::getNumArgOperands(CI) == 1);
        Value *arg = CI->getArgOperand(0);
        [[maybe_unused]] PointerType *argType = dyn_cast<PointerType>(arg->getType());
        IGC_ASSERT(argType != nullptr);
        PointerType *dstType = dyn_cast<PointerType>(CI->getType());
        IGC_ASSERT(dstType != nullptr);

        auto argAS = cast<PointerType>(arg->getType())->getAddressSpace();
        if (argAS == ADDRESS_SPACE_GENERIC) {
          Value *newPtr = llvm::ConstantPointerNull::get(dstType);
          CI->replaceAllUsesWith(newPtr);
          CI->eraseFromParent();

          changed = true;
        }
      }
    }
  }

  return changed;
}
