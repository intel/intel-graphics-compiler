/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LowerInvokeSIMD.hpp"
#include "Compiler/IGCPassSupport.h"

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
    auto PTy = PointerType::get(FTy, cast<PointerType>(CI.getArgOperand(0)->getType())->getAddressSpace());
    m_Builder->SetInsertPoint(&CI);

    auto CastedPointer = m_Builder->CreateBitCast(CI.getArgOperand(0), PTy);
    auto NewCall = m_Builder->CreateCall(CastedPointer, ArgVals);
    CI.replaceAllUsesWith(NewCall);
    CI.eraseFromParent();
    m_changed = true;
}

bool LowerInvokeSIMD::runOnModule(Module& M) {
    IGCLLVM::IRBuilder<> builder(M.getContext());
    m_Builder = &builder;
    visit(M);
    return m_changed;
}
