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

    // Promote functions
    for (auto& function : module.functions())
    {
        changed |= getOrCreatePromotedValue(&function) != &function;
    }

    // Promote global variables
    for (auto& globalVariable : module.globals())
    {
        changed |= getOrCreatePromotedValue(&globalVariable) != &globalVariable;
    }

    visit(module);
    cleanUp(module);

    return changed;
}

void PromoteBools::visitLoadInst(LoadInst& load)
{
    auto src = load.getOperand(0);

    // Check if promotion is needed
    if (!typeNeedsPromotion(src->getType()))
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
    auto trunc = builder.CreateTrunc(newLoad, Type::getInt1Ty(load.getContext()));
    load.replaceAllUsesWith(trunc);
    auto name = load.getName().str();
    load.eraseFromParent();
    newLoad->setName(name);

    changed = true;
}

void PromoteBools::visitStoreInst(StoreInst& store)
{
    auto src = store.getOperand(0);
    auto dst = store.getOperand(1);

    // Check if promotion is needed
    if (!typeNeedsPromotion(dst->getType()))
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
    auto name = store.getName().str();
    store.eraseFromParent();
    newStore->setName(name);

    changed = true;
}

void PromoteBools::visitCallInst(CallInst& call)
{
    auto function = call.getCalledFunction();
    auto functionType = call.getFunctionType();

    auto promotedValue = getOrCreatePromotedValue(function);
    if (!promotedValue)
    {
        return;
    }

    auto newFunction = dyn_cast<Function>(promotedValue);

    // Promotion is not needed.
    if (newFunction == function)
    {
        return;
    }

    IRBuilder<> builder(&call);
    std::vector<Value*> newCallArguments;
    for (auto& arg : call.args())
    {
        if (!arg->getType()->isIntegerTy(1))
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
    if (!functionType->getReturnType()->isIntegerTy(1))
    {
        call.replaceAllUsesWith(newCall);
    }
    else
    {
        auto trunc = builder.CreateTrunc(newCall, Type::getInt1Ty(call.getContext()));
        call.replaceAllUsesWith(trunc);
    }
    auto name = call.getName().str();
    call.eraseFromParent();
    newCall->setName(name);

    changed = true;
}

void PromoteBools::visitAllocaInst(AllocaInst& alloca)
{
    changed |= getOrCreatePromotedValue(&alloca) != &alloca;
}

bool PromoteBools::typeNeedsPromotion(Type* type)
{
    if (!type)
    {
        return false;
    }
    else if (type->isIntegerTy(1))
    {
        return true;
    }
    else if (auto vectorType = dyn_cast<VectorType>(type))
    {
        return typeNeedsPromotion(vectorType->getElementType());
    }
    else if (auto pointerType = dyn_cast<PointerType>(type))
    {
        return typeNeedsPromotion(type->getPointerElementType());
    }
    else if (auto arrayType = dyn_cast<ArrayType>(type))
    {
        return typeNeedsPromotion(arrayType->getElementType());
    }
    else if (auto structType = dyn_cast<StructType>(type))
    {
        return std::any_of(structType->element_begin(), structType->element_end(), [this](const auto& element) {
            return typeNeedsPromotion(element);
        });
    }
    else if (auto functionType = dyn_cast<FunctionType>(type))
    {
        if (typeNeedsPromotion(functionType->getReturnType()))
        {
            return true;
        }
        return std::any_of(functionType->param_begin(), functionType->param_end(), [this](const auto& element) {
            return typeNeedsPromotion(element);
        });
    }

    return false;
}

Value* PromoteBools::createZextIfNeeded(Value* argument, Instruction* insertBefore)
{
    auto trunc = dyn_cast<TruncInst>(argument);
    if (trunc && trunc->getSrcTy()->isIntegerTy(8))
    {
        return trunc->getOperand(0);
    }
    else
    {
        IRBuilder<> builder(insertBefore);
        return builder.CreateZExt(argument, Type::getInt8Ty(argument->getContext()));
    }
}

void PromoteBools::cleanUp(Module& module)
{
    auto renameAndClean = [](auto src, auto dst) {
        auto name = src->getName().str();
        src->eraseFromParent();
        dst->setName(name);
    };

    for (auto& it : promotedValuesCache)
    {
        if (auto globalVariable = dyn_cast<GlobalVariable>(it.first))
        {
            renameAndClean(globalVariable, it.second);
        }
        else if (auto function = dyn_cast<Function>(it.first))
        {
            renameAndClean(function, it.second);
        }
        else if (auto alloca = dyn_cast<AllocaInst>(it.first))
        {
            renameAndClean(alloca, it.second);
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

Type* PromoteBools::getOrCreatePromotedType(Type* type)
{
    if (promotedTypesCache.count(type))
    {
        return promotedTypesCache[type];
    }

    Type* newType = type;
    if (type->isIntegerTy(1))
    {
        newType = Type::getInt8Ty(type->getContext());
    }
    else if (auto vectorType = dyn_cast<VectorType>(type))
    {
        newType = VectorType::get(
            getOrCreatePromotedType(vectorType->getElementType()),
            vectorType->getElementCount());
    }
    else if (auto pointerType = dyn_cast<PointerType>(type))
    {
        newType = PointerType::get(
            getOrCreatePromotedType(type->getPointerElementType()),
            pointerType->getAddressSpace());
    }
    else if (auto arrayType = dyn_cast<ArrayType>(type))
    {
        newType = ArrayType::get(
            getOrCreatePromotedType(arrayType->getElementType()),
            arrayType->getNumElements());
    }
    else if (auto structType = dyn_cast<StructType>(type))
    {
        auto name = structType->hasName() ? structType->getName() : "";
        std::vector<Type*> elements;
        for (const auto& element : structType->elements())
        {
            elements.push_back(getOrCreatePromotedType(element));
        }
        newType = StructType::create(elements, name, structType->isPacked());
    }
    else if (auto functionType = dyn_cast<FunctionType>(type))
    {
        auto returnType = getOrCreatePromotedType(functionType->getReturnType());
        std::vector<Type*> arguments;
        for (auto& type : functionType->params())
        {
            arguments.push_back(getOrCreatePromotedType(type));
        }
        newType = FunctionType::get(returnType, arguments, functionType->isVarArg());
    }

    if (newType != type)
    {
        promotedTypesCache[type] = newType;
    }
    return newType;
}

Value* PromoteBools::getOrCreatePromotedValue(Value* value)
{
    if (promotedValuesCache.count(value))
    {
        return promotedValuesCache[value];
    }

    Value* newValue = value;
    if (auto globalVariable = dyn_cast<GlobalVariable>(value))
    {
        newValue = promoteGlobalVariable(globalVariable);
    }
    else if (auto function = dyn_cast<Function>(value))
    {
        newValue = promoteFunction(function);
    }
    else if (auto alloca = dyn_cast<AllocaInst>(value))
    {
        newValue = promoteAlloca(alloca);
    }

    if (newValue != value)
    {
        promotedValuesCache[value] = newValue;
    }
    return newValue;
}

Function* PromoteBools::promoteFunction(Function* function)
{
    if (!function
        || function->isIntrinsic()
        || PreprocessSPVIR::isSPVIR(function->getName())
        || !typeNeedsPromotion(function->getFunctionType()))
    {
        return function;
    }

    auto newFunctionType = dyn_cast<FunctionType>(getOrCreatePromotedType(function->getFunctionType()));
    auto newFunction = Function::Create(newFunctionType, function->getLinkage(), function->getName(), function->getParent());
    newFunction->setCallingConv(function->getCallingConv());
    newFunction->setAttributes(function->getAttributes());

    // Clone and fix function body
    if (!function->isDeclaration())
    {
        // Clone body
        SmallVector<ReturnInst*, 8> returns;
        ValueToValueMapTy argsMap;
        auto functionArgIt = function->arg_begin();
        auto newFunctionArgIt = newFunction->arg_begin();
        while (functionArgIt != function->arg_end())
        {
            newFunctionArgIt->setName(functionArgIt->getName());
            argsMap[&*functionArgIt++] = newFunctionArgIt++;
        }
        IGCLLVM::CloneFunctionInto(newFunction, function, argsMap,
            IGCLLVM::CloneFunctionChangeType::LocalChangesOnly, returns);

        // Fix body
        for (auto& arg : function->args())
        {
            if (!arg.getType()->isIntegerTy(1) || arg.hasNUses(0))
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

            auto trunc = new TruncInst(newArg, Type::getInt1Ty(function->getContext()), "", newFunction->getEntryBlock().getFirstNonPHI());
            for (auto& instruction : userInstructions)
            {
                instruction->replaceUsesOfWith(newArg, trunc);
            }
        }

        // Fix ret statements
        if (function->getReturnType()->isIntegerTy(1))
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

    return newFunction;
}

GlobalVariable* PromoteBools::promoteGlobalVariable(GlobalVariable* globalVariable)
{
    if (!globalVariable || !typeNeedsPromotion(globalVariable->getType()))
    {
        return globalVariable;
    }

    return new GlobalVariable(
        *globalVariable->getParent(),
        Type::getInt8Ty(globalVariable->getContext()),
        globalVariable->isConstant(),
        globalVariable->getLinkage(),
        ConstantInt::get(Type::getInt8Ty(globalVariable->getContext()), globalVariable->getInitializer()->isOneValue() ? 1 : 0),
        globalVariable->getName(),
        nullptr,
        GlobalValue::ThreadLocalMode::NotThreadLocal,
        globalVariable->getType()->getPointerAddressSpace());
}

AllocaInst* PromoteBools::promoteAlloca(AllocaInst* alloca)
{
    if (!alloca || !typeNeedsPromotion(alloca->getAllocatedType()))
    {
        return alloca;
    }

    auto newAlloca = new AllocaInst(
        getOrCreatePromotedType(alloca->getAllocatedType()),
        alloca->getType()->getAddressSpace(),
        alloca->isArrayAllocation() ? alloca->getArraySize() : nullptr,
        alloca->getName(),
        alloca);
    newAlloca->setAlignment(IGCLLVM::getAlign(*alloca));

    return newAlloca;
}
