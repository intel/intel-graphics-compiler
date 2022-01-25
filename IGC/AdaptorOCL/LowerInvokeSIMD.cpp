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
    IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(LowerInvokeSIMD, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char LowerInvokeSIMD::ID = 0;

namespace {
    static const char* FNATTR_REFERENCED_INDIRECTLY = "referenced-indirectly";
    static const char* FNATTR_INVOKE_SIMD_TARGET = "invoke_simd_target";
}

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

    CallInst* NewCall = nullptr;
    std::error_code EC;
    if (Function* Callee = dyn_cast<Function>(CI.getArgOperand(0))) {
        Function* NewFunc = nullptr;
        if (m_OldFuncToNewFuncMap.find(Callee) == m_OldFuncToNewFuncMap.end()) {
            std::string oldName = std::string(Callee->getName());
            Callee->setName(Callee->getName() + ".old");
            NewFunc = Function::Create(FTy, Callee->getLinkage(), oldName, *Callee->getParent());
            NewFunc->setAttributes(Callee->getAttributes());
            NewFunc->addFnAttr(FNATTR_INVOKE_SIMD_TARGET);
            if (NewFunc->hasFnAttribute(FNATTR_REFERENCED_INDIRECTLY)) {
                NewFunc->removeFnAttr(FNATTR_REFERENCED_INDIRECTLY);
            }
            NewFunc->setCallingConv(Callee->getCallingConv());
            m_OldFuncToNewFuncMap[Callee] = NewFunc;
        } else {
            NewFunc = m_OldFuncToNewFuncMap[Callee];
        }

        NewCall = m_Builder->CreateCall(NewFunc, ArgVals);
        detectUniformParams(Callee, *NewCall);

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

// Compare arguments of the original ESIMD function with argument types deduced from invoke_simd builtin and used in the NewCall.
// If an argument is same in both, it is treated as scalar on ESIMD path.
void LowerInvokeSIMD::detectUniformParams(const llvm::Function* ESIMDFunction, llvm::CallInst& NewCall) {
    auto ESIMDFuncType = ESIMDFunction->getFunctionType();
    auto SPMDFuncType = NewCall.getCalledFunction()->getFunctionType();
    int ParamNumber = 0;
    for (auto ESIMDParamType = ESIMDFuncType->param_begin(),
        SPMDParamType = SPMDFuncType->param_begin();
        ESIMDParamType != ESIMDFuncType->param_end() &&
        SPMDParamType != SPMDFuncType->param_end();
        ++ESIMDParamType, ++SPMDParamType, ++ParamNumber) {

        if (*ESIMDParamType == *SPMDParamType) {
            Value* args[3];
            args[0] = NewCall.getArgOperand(ParamNumber);
            args[1] = m_Builder->getInt32(0);
            args[2] = m_Builder->getInt32(0);

            // Get the first SIMD channel, as the spec says that all work items in the group must execute it.
            Function* simdShuffleFunc = GenISAIntrinsic::getDeclaration(NewCall.getModule(),
                GenISAIntrinsic::GenISA_WaveShuffleIndex, args[0]->getType());
            m_Builder->SetInsertPoint(&NewCall);
            auto ShuffleCall = m_Builder->CreateCall(simdShuffleFunc, args);

            NewCall.setArgOperand(ParamNumber, ShuffleCall);
        }
    }
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
