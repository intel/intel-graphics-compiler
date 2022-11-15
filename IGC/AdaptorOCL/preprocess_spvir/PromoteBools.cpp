/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PromoteBools.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/IR/Type.h"
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

static void swapNames(Value* value1, Value* value2)
{
    if (value1->getType()->isVoidTy()
        || value2->getType()->isVoidTy()
        || (!value1->hasName() && !value2->hasName()))
    {
        return;
    }

    auto name1 = value1->getName().str();
    auto name2 = value2->getName().str();
    value1->setName(name2);
    value2->setName(name1);
}

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

    while (!promotionQueue.empty())
    {
        auto value = promotionQueue.front();
        promotionQueue.pop();
        if (!wasPromoted(value))
        {
            if (auto inst = dyn_cast<Instruction>(value))
            {
                if (wasPromoted(inst->getFunction()))
                {
                    continue;
                }
            }
            getOrCreatePromotedValue(value);
        }
    }

    cleanUp(module);

    return changed;
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
    auto erase = [&doNotRemove = this->doNotRemove](auto v) {
        if (doNotRemove.find(v) != doNotRemove.end())
        {
            return;
        }

        // Replace all v uses by undef. It allows us not to worry about
        // the order in which we delete unpromoted values.
        v->replaceAllUsesWith(UndefValue::get(v->getType()));
        v->eraseFromParent();
    };

    for (auto& it : promotedValuesCache)
    {
        if (auto function = dyn_cast<Function>(it.first))
        {
            swapNames(function, it.second);
            erase(function);
        }
        else if (auto globalVariable = dyn_cast<GlobalVariable>(it.first))
        {
            swapNames(globalVariable, it.second);
            erase(globalVariable);
        }
        else if (auto instruction = dyn_cast<Instruction>(it.first))
        {
            erase(instruction);
        }
    }

    SmallVector<Instruction*, 8> deadInstructions;
    for (auto& function : module)
    {
        for (auto& basicBlock : function)
        {
            for (auto it = basicBlock.rbegin(); it != basicBlock.rend(); ++it)
            {
                Instruction* instruction = &*it;
                if (isa<TruncInst>(instruction) || isa<ZExtInst>(instruction))
                {
                    if (instruction->hasNUses(0))
                    {
                        deadInstructions.push_back(instruction);
                    }
                }
            }
        }
    }

    for (auto& instruction : deadInstructions)
    {
        instruction->eraseFromParent();
    }
}

//------------------------------------------------------------------------------
//
// Checking if type needs promotion
//
//------------------------------------------------------------------------------
bool PromoteBools::typeNeedsPromotion(Type* type, DenseSet<Type*> visitedTypes)
{
    if (!type || visitedTypes.count(type))
    {
        return false;
    }

    visitedTypes.insert(type);

    if (type->isIntegerTy(1))
    {
        return true;
    }
    else if (auto vectorType = dyn_cast<VectorType>(type))
    {
        return typeNeedsPromotion(vectorType->getElementType(), visitedTypes);
    }
    else if (auto pointerType = dyn_cast<PointerType>(type))
    {
        return typeNeedsPromotion(IGCLLVM::getNonOpaquePtrEltTy(type), visitedTypes);
    }
    else if (auto arrayType = dyn_cast<ArrayType>(type))
    {
        return typeNeedsPromotion(arrayType->getElementType(), visitedTypes);
    }
    else if (auto structType = dyn_cast<StructType>(type))
    {
        return std::any_of(structType->element_begin(), structType->element_end(), [this, &visitedTypes](const auto& element) {
            return typeNeedsPromotion(element, visitedTypes);
        });
    }
    else if (auto functionType = dyn_cast<FunctionType>(type))
    {
        if (typeNeedsPromotion(functionType->getReturnType(), visitedTypes))
        {
            return true;
        }
        return std::any_of(functionType->param_begin(), functionType->param_end(), [this, &visitedTypes](const auto& element) {
            return typeNeedsPromotion(element, visitedTypes);
        });
    }

    return false;
}

//------------------------------------------------------------------------------
//
// Promoting types
//
//------------------------------------------------------------------------------
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
            getOrCreatePromotedType(IGCLLVM::getNonOpaquePtrEltTy(type)),
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
        // Check if promotion is needed because otherwise new structures
        // will be created even if promotion is not needed. The other data
        // types do not have this problem.
        if (typeNeedsPromotion(structType))
        {
            // Create an opaque type to handle recursive types
            auto name = structType->hasName() ? structType->getName().str() : "";
            structType->setName(name + ".unpromoted");

            auto newStructType = StructType::create(type->getContext(), name);
            promotedTypesCache[type] = newStructType;

            // Promote and update struct elements
            SmallVector<Type*, 8> elements;
            for (const auto& element : structType->elements())
            {
                elements.push_back(getOrCreatePromotedType(element));
            }
            newStructType->setBody(elements, structType->isPacked());

            newType = newStructType;
        }
    }
    else if (auto functionType = dyn_cast<FunctionType>(type))
    {
        auto returnType = getOrCreatePromotedType(functionType->getReturnType());
        SmallVector<Type*, 8> arguments;
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

//------------------------------------------------------------------------------
//
// Promoting values
//
//------------------------------------------------------------------------------
Value* PromoteBools::getOrCreatePromotedValue(Value* value)
{
    if (wasPromoted(value))
    {
        return promotedValuesCache[value];
    }

    Value* newValue = value;
    if (auto function = dyn_cast<Function>(value))
    {
        newValue = promoteFunction(function);
    }
    else if (auto globalVariable = dyn_cast<GlobalVariable>(value))
    {
        newValue = promoteGlobalVariable(globalVariable);
    }
    else if (auto constant = dyn_cast<Constant>(value))
    {
        newValue = promoteConstant(constant);
    }
    else if (auto addrSpaceCast = dyn_cast<AddrSpaceCastInst>(value))
    {
        newValue = promoteAddrSpaceCast(addrSpaceCast);
    }
    else if (auto alloca = dyn_cast<AllocaInst>(value))
    {
        newValue = promoteAlloca(alloca);
    }
    else if (auto bitcast = dyn_cast<BitCastInst>(value))
    {
        newValue = promoteBitCast(bitcast);
    }
    else if (auto call = dyn_cast<CallInst>(value))
    {
        newValue = promoteCall(call);
    }
    else if (auto extractValue = dyn_cast<ExtractValueInst>(value))
    {
        newValue = promoteExtractValue(extractValue);
    }
    else if (auto getElementPtr = dyn_cast<GetElementPtrInst>(value))
    {
        newValue = promoteGetElementPtr(getElementPtr);
    }
    else if (auto icmp = dyn_cast<ICmpInst>(value))
    {
        newValue = promoteICmp(icmp);
    }
    else if (auto insertValue = dyn_cast<InsertValueInst>(value))
    {
        newValue = promoteInsertValue(insertValue);
    }
    else if (auto load = dyn_cast<LoadInst>(value))
    {
        newValue = promoteLoad(load);
    }
    else if (auto store = dyn_cast<StoreInst>(value))
    {
        newValue = promoteStore(store);
    }
    else if (auto instruction = dyn_cast<Instruction>(value))
    {
        IRBuilder<> builder(instruction);
        for (auto& operand : instruction->operands())
        {
            if (wasPromoted(operand))
            {
                auto promoted = promotedValuesCache[operand];
                if (auto zext = dyn_cast<ZExtInst>(promoted))
                {
                    instruction->replaceUsesOfWith(operand, zext->getOperand(0));
                }
                else
                {
                    if (auto operandInst = dyn_cast<Instruction>(operand))
                    {
                        auto insertPoint = operandInst->getNextNode();
                        if (!insertPoint) {
                            insertPoint = operandInst->getParent()->getTerminator();
                        }
                        builder.SetInsertPoint(insertPoint);
                    }
                    auto trunc = builder.CreateTrunc(
                        promoted,
                        operand->getType(),
                        ""
                    );
                    instruction->replaceUsesOfWith(operand, trunc);
                }
            }
        }

        if (value->getType()->isIntegerTy(1))
        {
            newValue = new ZExtInst(
                instruction,
                Type::getInt8Ty(instruction->getContext()),
                "",
                instruction->getNextNode()
            );
            doNotRemove.insert(instruction);
        }
    }

    if (newValue != value)
    {
        promotedValuesCache[value] = newValue;
        for (const auto& user : value->users())
        {
            if (!wasPromoted(user))
            {
                promotionQueue.push(user);
            }
        }
    }
    return newValue;
}

bool PromoteBools::wasPromoted(llvm::Value* value)
{
    return promotedValuesCache.count(value);
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

    auto newFunction = Function::Create(
        dyn_cast<FunctionType>(getOrCreatePromotedType(function->getFunctionType())),
        function->getLinkage(),
        function->getName() + ".promoted",
        function->getParent()
    );
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
            IGCLLVM::CloneFunctionChangeType::GlobalChanges, returns);

        // Fix body
        for (auto& arg : function->args())
        {
            if (!typeNeedsPromotion(arg.getType()) || arg.hasNUses(0))
            {
                continue;
            }

            auto newArg = argsMap[&arg];

            SmallVector<Instruction*, 8> userInstructions;
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
        getOrCreatePromotedType(IGCLLVM::getNonOpaquePtrEltTy(globalVariable->getType())),
        globalVariable->isConstant(),
        globalVariable->getLinkage(),
        promoteConstant(globalVariable->getInitializer()),
        globalVariable->getName() + ".promoted",
        nullptr,
        GlobalValue::ThreadLocalMode::NotThreadLocal,
        globalVariable->getType()->getPointerAddressSpace());
}

Constant* PromoteBools::promoteConstant(Constant* constant)
{
    if (!constant)
    {
        return nullptr;
    }

    if (isa<UndefValue>(constant))
    {
        return UndefValue::get(getOrCreatePromotedType(constant->getType()));
    }
    else if (isa<ConstantAggregateZero>(constant))
    {
        return ConstantAggregateZero::get(getOrCreatePromotedType(constant->getType()));
    }
    else if (auto constantInteger = dyn_cast<ConstantInt>(constant))
    {
        if (!typeNeedsPromotion(constantInteger->getType()))
        {
            return constant;
        }

        return ConstantInt::get(Type::getInt8Ty(constant->getContext()), constant->isOneValue() ? 1 : 0);
    }
    else if (auto constantPointerNull = dyn_cast<ConstantPointerNull>(constant))
    {
        if (!typeNeedsPromotion(IGCLLVM::getNonOpaquePtrEltTy(constantPointerNull->getType())))
        {
            return constant;
        }

        auto newPointerElementType = getOrCreatePromotedType(IGCLLVM::getNonOpaquePtrEltTy(constantPointerNull->getType()));
        return ConstantPointerNull::get(PointerType::get(newPointerElementType, constantPointerNull->getType()->getAddressSpace()));
    }
    else if (auto constantVector = dyn_cast<ConstantVector>(constant))
    {
        if (!typeNeedsPromotion(constantVector->getType()))
        {
            return constant;
        }

        SmallVector<Constant*, 8> values;
        for (unsigned i = 0; i < constantVector->getType()->getNumElements(); ++i)
        {
            values.push_back(promoteConstant(constantVector->getAggregateElement(i)));
        }
        return ConstantVector::get(values);
    }
    else if (auto constantArray = dyn_cast<ConstantArray>(constant))
    {
        if (!typeNeedsPromotion(constantArray->getType()))
        {
            return constant;
        }

        SmallVector<Constant*, 8> values;
        for (unsigned i = 0; i < constantArray->getType()->getNumElements(); ++i)
        {
            values.push_back(promoteConstant(constantArray->getAggregateElement(i)));
        }

        auto newType = getOrCreatePromotedType(constantArray->getType());
        return ConstantArray::get(dyn_cast<ArrayType>(newType), values);
    }
    else if (auto constantStruct = dyn_cast<ConstantStruct>(constant))
    {
        if (!typeNeedsPromotion(constantStruct->getType()))
        {
            return constant;
        }

        SmallVector<Constant*, 8> values;
        for (unsigned i = 0; i < constantStruct->getType()->getNumElements(); ++i)
        {
            values.push_back(promoteConstant(constantStruct->getAggregateElement(i)));
        }

        auto newType = getOrCreatePromotedType(constantStruct->getType());
        return ConstantStruct::get(dyn_cast<StructType>(newType), values);
    }

    return constant;
}

AddrSpaceCastInst* PromoteBools::promoteAddrSpaceCast(AddrSpaceCastInst* addrSpaceCast)
{
    if (!addrSpaceCast || !typeNeedsPromotion(addrSpaceCast->getDestTy()))
    {
        return addrSpaceCast;
    }

    return new AddrSpaceCastInst(
        getOrCreatePromotedValue(addrSpaceCast->getOperand(0)),
        getOrCreatePromotedType(addrSpaceCast->getDestTy()),
        "",
        addrSpaceCast
    );
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
        "",
        alloca);
    newAlloca->setAlignment(IGCLLVM::getAlign(*alloca));

    return newAlloca;
}

Value* PromoteBools::promoteBitCast(BitCastInst* bitcast)
{
    if (!bitcast || !typeNeedsPromotion(bitcast->getDestTy()))
    {
        return bitcast;
    }

    auto newType = getOrCreatePromotedType(bitcast->getDestTy());
    if (bitcast->getSrcTy() == newType)
    {
        return bitcast->getOperand(0);
    }

    return new BitCastInst(
        getOrCreatePromotedValue(bitcast->getOperand(0)),
        getOrCreatePromotedType(bitcast->getDestTy()),
        "",
        bitcast
    );
}

CallInst* PromoteBools::promoteCall(CallInst* call)
{
    auto function = call->getCalledFunction();
    auto functionType = call->getFunctionType();

    if (!function || !typeNeedsPromotion(functionType))
    {
        return call;
    }

    auto newFunction = dyn_cast<Function>(getOrCreatePromotedValue(function));
    if (newFunction == function)
    {
        return call;
    }

    SmallVector<Value*, 8> newCallArguments;
    for (auto& arg : call->args())
    {
        newCallArguments.push_back(getOrCreatePromotedValue(arg));
    }

    return CallInst::Create(
        newFunction->getFunctionType(),
        newFunction,
        newCallArguments,
        "",
        call
    );
}

ExtractValueInst* PromoteBools::promoteExtractValue(ExtractValueInst* extractValue)
{
    if (!extractValue)
    {
        return extractValue;
    }

    auto aggregateOp = extractValue->getAggregateOperand();
    if (!typeNeedsPromotion(aggregateOp->getType()))
    {
        return extractValue;
    }

    return ExtractValueInst::Create(
        getOrCreatePromotedValue(aggregateOp),
        extractValue->getIndices(),
        "",
        extractValue
    );
}

GetElementPtrInst* PromoteBools::promoteGetElementPtr(GetElementPtrInst* getElementPtr)
{
    if (!getElementPtr || !typeNeedsPromotion(getElementPtr->getResultElementType()))
    {
        return getElementPtr;
    }

    auto promotedOperand = getOrCreatePromotedValue(getElementPtr->getPointerOperand());
    auto indices = SmallVector<Value*, 8>(getElementPtr->idx_begin(), getElementPtr->idx_end());

    return GetElementPtrInst::Create(
        IGCLLVM::getNonOpaquePtrEltTy(promotedOperand->getType()),
        promotedOperand,
        indices,
        "",
        getElementPtr
    );
}

ICmpInst* PromoteBools::promoteICmp(ICmpInst* icmp)
{
    if (!icmp)
    {
        return icmp;
    }

    auto op0 = icmp->getOperand(0);
    auto op1 = icmp->getOperand(1);

    if (!typeNeedsPromotion(op0->getType()))
    {
        return icmp;
    }

    return new ICmpInst(
        icmp,
        icmp->getPredicate(),
        getOrCreatePromotedValue(op0),
        getOrCreatePromotedValue(op1),
        ""
    );
}

InsertValueInst* PromoteBools::promoteInsertValue(InsertValueInst* insertValue)
{
    if (!insertValue)
    {
        return insertValue;
    }

    auto aggregateOp = insertValue->getAggregateOperand();
    auto insertedValueOp = insertValue->getInsertedValueOperand();
    if (!typeNeedsPromotion(aggregateOp->getType()) && !typeNeedsPromotion(insertedValueOp->getType()))
    {
        return insertValue;
    }

    auto newAggregateOp = typeNeedsPromotion(aggregateOp->getType()) ? getOrCreatePromotedValue(aggregateOp) : aggregateOp;
    auto newInsertedValueOp = typeNeedsPromotion(insertedValueOp->getType()) ? getOrCreatePromotedValue(insertedValueOp) : insertedValueOp;

    return InsertValueInst::Create(
        newAggregateOp,
        newInsertedValueOp,
        insertValue->getIndices(),
        "",
        insertValue
    );
}

LoadInst* PromoteBools::promoteLoad(LoadInst* load)
{
    if (!load)
    {
        return load;
    }

    auto src = load->getOperand(0);

    if (!wasPromoted(src) && !typeNeedsPromotion(src->getType()))
    {
        return load;
    }

    auto newSrc = getOrCreatePromotedValue(src);
    auto newLoad = new LoadInst(
        IGCLLVM::getNonOpaquePtrEltTy(newSrc->getType()),
        newSrc,
        "",
        load
    );
    newLoad->setAlignment(IGCLLVM::getAlign(*load));

    return newLoad;
}

StoreInst* PromoteBools::promoteStore(StoreInst* store)
{
    if (!store)
    {
        return store;
    }

    auto src = store->getOperand(0);
    auto dst = store->getOperand(1);

    if (!wasPromoted(src) && !wasPromoted(dst) && !typeNeedsPromotion(src->getType()))
    {
        return store;
    }

    auto newStore = new StoreInst(
        getOrCreatePromotedValue(src),
        getOrCreatePromotedValue(dst),
        store->isVolatile(),
        store
    );
    newStore->setAlignment(IGCLLVM::getAlign(*store));

    return newStore;
}
