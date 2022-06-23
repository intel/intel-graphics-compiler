/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PromoteBools.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Support/Regex.h>
#include "common/LLVMWarningsPop.hpp"
#include "PreprocessSPVIR.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-promote-bools"
#define PASS_DESCRIPTION "Promote bools"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PromoteBools, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(PromoteBools, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PromoteBools::ID = 0;

PromoteBools::PromoteBools() : ModulePass(ID)
{
    initializePromoteBoolsPass(*PassRegistry::getPassRegistry());
}

bool PromoteBools::runOnModule(Module& module)
{
    changed = false;

    int1type = Type::getInt1Ty(module.getContext());
    int8type = Type::getInt8Ty(module.getContext());

    promoteFunctions(module);
    visit(module);
    cleanUp(module);

    return changed;
}

Value* PromoteBools::getOrCreatePromotedValue(Value* value)
{
    if (promotedValuesCache.count(value))
    {
        return promotedValuesCache[value];
    }

    Value* newValue = nullptr;
    if (auto GV = dyn_cast<GlobalVariable>(value))
    {
        newValue = new GlobalVariable(
            *GV->getParent(),
            int8type,
            GV->isConstant(),
            GV->getLinkage(),
            ConstantInt::get(int8type, GV->getInitializer()->isOneValue() ? 1 : 0),
            GV->getName(),
            nullptr,
            GlobalValue::ThreadLocalMode::NotThreadLocal,
            GV->getType()->getPointerAddressSpace());
    }
    else if (auto F = dyn_cast<Function>(value))
    {
        auto newReturnType = F->getReturnType() != int1type ? F->getReturnType() : int8type;
        std::vector<Type*> newArguments;
        for (auto& type : F->getFunctionType()->params())
        {
            newArguments.push_back(type != int1type ? type : int8type);
        }

        auto newFunctionType = FunctionType::get(newReturnType, newArguments, F->getFunctionType()->isVarArg());
        auto newFunction = Function::Create(newFunctionType, F->getLinkage(), F->getName(), F->getParent());
        newFunction->setCallingConv(F->getCallingConv());
        newFunction->setAttributes(F->getAttributes());

        // Clone and fix function body
        if (!F->isDeclaration())
        {
            // Clone body
            SmallVector<ReturnInst*, 8> returns;
            ValueToValueMapTy argsMap;
            auto functionArgIt = F->arg_begin();
            auto newFunctionArgIt = newFunction->arg_begin();
            while (functionArgIt != F->arg_end())
            {
                newFunctionArgIt->setName(functionArgIt->getName());
                argsMap[&*functionArgIt++] = newFunctionArgIt++;
            }
            IGCLLVM::CloneFunctionInto(newFunction, F, argsMap, false, returns);

            // Fix body
            for (auto& arg : F->args())
            {
                if (arg.getType() != int1type || arg.hasNUses(0))
                {
                    continue;
                }

                auto newArg = argsMap[&arg];

                std::vector<Instruction*> userInstructions;
                for (auto user : newArg->users())
                {
                    if (auto instruction = dyn_cast<Instruction>(user))
                    {
                        userInstructions.push_back(instruction);
                    }
                }

                auto trunc = new TruncInst(newArg, int1type, "", newFunction->getEntryBlock().getFirstNonPHI());
                for (auto& instruction : userInstructions)
                {
                    instruction->replaceUsesOfWith(newArg, trunc);
                }
            }

            // Fix ret statements
            if (F->getReturnType() == int1type)
            {
                for (auto& basicBlock : *newFunction)
                {
                    auto terminator = basicBlock.getTerminator();
                    if (auto ret = dyn_cast<ReturnInst>(terminator))
                    {
                        auto zext = createZextIfNeeded(ret->getReturnValue(), ret);
                        IRBuilder<> builder(ret);
                        auto newRet = builder.CreateRet(zext);
                        ret->replaceAllUsesWith(newRet);
                        ret->eraseFromParent();
                    }
                }
            }
        }

        newValue = newFunction;
    }
    else
    {
        // TODO: Handle another cases
        IGC_ASSERT_MESSAGE(0, "Unsupported type of value");
        return nullptr;
    }

    promotedValuesCache[value] = newValue;
    return newValue;
}

void PromoteBools::visitLoadInst(LoadInst& load)
{
    auto src = load.getOperand(0);
    auto srcType = src->getType();

    // Check if promotion is needed
    if (srcType->getPointerElementType() != int1type)
    {
        return;
    }

    // Promote
    auto newSrc = getOrCreatePromotedValue(src);
    if (!newSrc)
    {
        return;
    }

    IRBuilder<> builder(&load);
    auto newLoad = builder.CreateLoad(newSrc->getType()->getPointerElementType(), newSrc);
    newLoad->setAlignment(IGCLLVM::getAlign(load));
    auto trunc = builder.CreateTrunc(newLoad, int1type);
    load.replaceAllUsesWith(trunc);
    load.eraseFromParent();

    changed = true;
}

void PromoteBools::visitStoreInst(StoreInst& store)
{
    auto src = store.getOperand(0);
    auto dst = store.getOperand(1);
    auto dstType = dst->getType();

    // Check if promotion is needed
    if (dstType->getPointerElementType() != int1type)
    {
        return;
    }

    // Promote
    auto newDst = getOrCreatePromotedValue(dst);
    if (!newDst)
    {
        return;
    }

    auto zext = createZextIfNeeded(src, &store);
    IRBuilder<> builder(&store);
    auto newStore = builder.CreateStore(zext, newDst);
    newStore->setAlignment(IGCLLVM::getAlign(store));
    store.replaceAllUsesWith(newStore);
    store.eraseFromParent();

    changed = true;
}

void PromoteBools::visitCallInst(CallInst& call)
{
    auto function = call.getCalledFunction();
    auto functionType = call.getFunctionType();

    // Check if promotion is needed
    if (!functionNeedsPromotion(function))
    {
        return;
    }

    // Promote
    auto newFunction = dyn_cast<Function>(getOrCreatePromotedValue(function));
    if (!newFunction)
    {
        return;
    }

    IRBuilder<> builder(&call);
    std::vector<Value*> newCallArguments;
    for (auto& arg : call.args())
    {
        if (arg->getType() != int1type)
        {
            newCallArguments.push_back(arg);
        }
        else
        {
            auto zext = createZextIfNeeded(arg, &call);
            newCallArguments.push_back(zext);
        }
    }

    auto newCall = builder.CreateCall(newFunction->getFunctionType(), newFunction, newCallArguments);
    if (functionType->getReturnType() != int1type)
    {
        call.replaceAllUsesWith(newCall);
    }
    else
    {
        auto trunc = builder.CreateTrunc(newCall, int1type);
        call.replaceAllUsesWith(trunc);
    }
    call.eraseFromParent();

    changed = true;
}

bool PromoteBools::functionNeedsPromotion(Function* function)
{
    if (!function || function->isIntrinsic() || PreprocessSPVIR::isSPVIR(function->getName()))
    {
        return false;
    }

    bool needsPromotion = function->getFunctionType()->getReturnType() == int1type;
    for (const auto& type : function->getFunctionType()->params())
    {
        needsPromotion |= type == int1type;
    }

    return needsPromotion;
}

Value* PromoteBools::createZextIfNeeded(Value* argument, Instruction* insertBefore)
{
    auto trunc = dyn_cast<TruncInst>(argument);
    if (trunc && trunc->getSrcTy() == int8type)
    {
        return trunc->getOperand(0);
    }
    else
    {
        IRBuilder<> builder(insertBefore);
        return builder.CreateZExt(argument, int8type);
    }
}

void PromoteBools::promoteFunctions(Module& module)
{
    for (auto& function : module)
    {
        if (functionNeedsPromotion(&function))
        {
            getOrCreatePromotedValue(&function);
            changed = true;
        }
    }
}

void PromoteBools::cleanUp(Module& module)
{
    auto renameAndClean = [](auto first, auto second) {
        auto name = first->getName().str();
        first->eraseFromParent();
        second->setName(name);
    };

    for (auto& it : promotedValuesCache)
    {
        if (auto GV = dyn_cast<GlobalVariable>(it.first))
        {
            renameAndClean(GV, it.second);
        }
        if (auto F = dyn_cast<Function>(it.first))
        {
            renameAndClean(F, it.second);
        }
    }

    std::vector<Instruction*> deadTruncs;
    for (auto& function : module)
    {
        for (auto& basicBlock : function)
        {
            for (auto it = basicBlock.rbegin(); it != basicBlock.rend(); ++it)
            {
                Instruction* instruction = &*it;
                if (isa<TruncInst>(instruction))
                {
                    if (instruction->hasNUses(0))
                    {
                        deadTruncs.push_back(instruction);
                    }
                }
            }
        }
    }

    for (auto& trunc : deadTruncs)
    {
        trunc->eraseFromParent();
    }
}
