/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LowerInvokeSIMD.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

#include <unordered_set>

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-lower-invoke-simd"
#define PASS_DESCRIPTION "Lower calls to invoke_simd DPCPP builtins"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LowerInvokeSIMD, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(LowerInvokeSIMD, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char LowerInvokeSIMD::ID = 0;

LowerInvokeSIMD::LowerInvokeSIMD() : ModulePass(ID)
{
    initializeLowerInvokeSIMDPass(*PassRegistry::getPassRegistry());
}

// Searches for invoke_simd calls and replaces them with calls to
// function pointer, that is the first arg of the call.

void LowerInvokeSIMD::visitCallInst(CallInst& CI)
{
    Function* F = CI.getCalledFunction();
    if (!F) return;
    if (!F->getName().contains("__builtin_invoke_simd")) return;

    // First argument is a function pointer. We need to bitcast it to lowered type.
    // The type will be deducted from this invocation.

    SmallVector<Type*, 8> ArgTys;
    SmallVector<Value*, 8> ArgVals;

    // Skip first param, as it is the function pointer to call.
    for (unsigned i = 1; i < F->getFunctionType()->getNumParams(); ++i) {
        ArgTys.push_back(F->getFunctionType()->getParamType(i));
        ArgVals.push_back(CI.getArgOperand(i));
    }
    auto FTy = FunctionType::get(F->getFunctionType()->getReturnType(), ArgTys, false);
    m_Builder->SetInsertPoint(&CI);

    Value* NewCall = nullptr;
    std::error_code EC;
    if (Function* Callee = dyn_cast<Function>(CI.getArgOperand(0))) {
        std::string oldName = std::string(Callee->getName());
        Callee->setName(Callee->getName() + ".old");
        auto newFunc = Function::Create(FTy, Callee->getLinkage(), oldName, *Callee->getParent());
        newFunc->setAttributes(Callee->getAttributes());
        newFunc->addFnAttr("invoke_simd_target");
        newFunc->setCallingConv(Callee->getCallingConv());
        NewCall = m_Builder->CreateCall(newFunc, ArgVals);
        m_OldFuncToNewFuncMap[Callee] = newFunc;

        // The callee will be a direct call, but the definition will be in the ESIMD visaasm,
        // that will be linked later in VISALinkerDriver. This module needs to be compiled only
        // to visaasm.
        CodeGenContext* Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        Ctx->m_compileToVISAOnly = true;
    } else {
      auto PTy = PointerType::get(
          FTy,
          cast<PointerType>(CI.getArgOperand(0)->getType())->getAddressSpace());
      auto CastedPointer = m_Builder->CreateBitCast(CI.getArgOperand(0), PTy);
      NewCall = m_Builder->CreateCall(CastedPointer, ArgVals);
    }
    CI.replaceAllUsesWith(NewCall);
    CI.eraseFromParent();
    m_changed = true;
}

bool LowerInvokeSIMD::runOnModule(Module& M) {
    IGCLLVM::IRBuilder<> builder(M.getContext());
    m_Builder = &builder;
    m_changed = false;
    m_OldFuncToNewFuncMap.clear();
    visit(M);

    // If there are uses of vc functions outside invoke_simd calls (e.g. function pointer is taken),
    // replace the old functions with new.
    for (auto it : m_OldFuncToNewFuncMap) {
        Function* OldFunc = it.first;
        Function* NewFunc = it.second;
        for (auto& use : OldFunc->uses()) {
            if (!isa<Instruction>(use.getUser())) continue;
            Instruction* User = cast<Instruction>(use.getUser());
            m_Builder->SetInsertPoint(cast<Instruction>(User));
            auto CastedPointer = m_Builder->CreateBitCast(NewFunc, OldFunc->getType());
            use.set(CastedPointer);
        }
    }

    return m_changed;
}
