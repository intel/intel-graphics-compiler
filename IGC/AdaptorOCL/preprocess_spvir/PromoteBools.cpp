/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PromoteBools.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/IR/Type.h"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Support/Regex.h>
#include "common/LLVMWarningsPop.hpp"
#include "PreprocessSPVIR.h"
#include "BiFManager/BiFManagerHandler.hpp"

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

static bool isIntegerTy(Value* value, int bitWidth)
{
    if (auto vectorType = dyn_cast<VectorType>(value->getType()))
    {
        return vectorType->getElementType()->isIntegerTy(bitWidth);
    }
    return value->getType()->isIntegerTy(bitWidth);
}

static Type* createDemotedType(Type* type)
{
    Type* newType = type;
    if (type->isIntegerTy(8))
    {
        newType = Type::getInt1Ty(type->getContext());
    }
    else if (auto vectorType = dyn_cast<VectorType>(type))
    {
        newType = VectorType::get(
            createDemotedType(vectorType->getElementType()),
            vectorType->getElementCount());
    }
    return newType;
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

Value* PromoteBools::convertI1ToI8(Value* value, Instruction* insertBefore)
{
    if (!isIntegerTy(value, 1))
    {
        return value;
    }

    auto trunc = dyn_cast<TruncInst>(value);
    if (trunc && isIntegerTy(trunc->getOperand(0), 8))
    {
        return trunc->getOperand(0);
    }

    IRBuilder<> builder(insertBefore);
    auto zext = builder.CreateZExt(value, getOrCreatePromotedType(value->getType()));
    if (isa<Instruction>(zext) && isa<Instruction>(value))
    {
        dyn_cast<Instruction>(zext)->setDebugLoc(dyn_cast<Instruction>(value)->getDebugLoc());
    }
    return zext;
}

Value* PromoteBools::convertI8ToI1(Value* value, Instruction* insertBefore)
{
    if (!isIntegerTy(value, 8))
    {
        return value;
    }

    auto zext = dyn_cast<ZExtInst>(value);
    if (zext && isIntegerTy(zext->getOperand(0), 1))
    {
        return zext->getOperand(0);
    }

    IRBuilder<> builder(insertBefore);
    auto trunc = builder.CreateTrunc(value, createDemotedType(value->getType()));
    if (isa<Instruction>(trunc) && isa<Instruction>(value))
    {
        dyn_cast<Instruction>(trunc)->setDebugLoc(dyn_cast<Instruction>(value)->getDebugLoc());
    }
    return trunc;
}

Value* PromoteBools::castTo(Value* value, Type* desiredType, Instruction* insertBefore)
{
    if (value->getType() == desiredType)
    {
        return value;
    }

    if (isIntegerTy(value, 8) && desiredType->isIntegerTy(1))
    {
        return convertI8ToI1(value, insertBefore);
    }
    else if (isIntegerTy(value, 1) && desiredType->isIntegerTy(8))
    {
        return convertI1ToI8(value, insertBefore);
    }

    IRBuilder<> builder(insertBefore);
    return builder.CreateBitCast(value, desiredType);
}

void PromoteBools::cleanUp(Module& module)
{
    auto erase = [](auto v) {
        // Replace all v uses by undef. It allows us not to worry about
        // the order in which we delete unpromoted values.
        v->replaceAllUsesWith(UndefValue::get(v->getType()));
        v->eraseFromParent();
    };

    for (auto& it : promotedValuesCache)
    {
        // Workaround. If there's a ConstantExpr cast of a function/globalVariable
        // and the function/globalVariable has been promoted (so the cast has beed promoted too)
        // and the function/globalVariable is processed in this loop before the cast
        // then the unpromoted cast will be broken during processing the function/globalVariable
        // by replaceAllUsesWith in erase function. When the loop reaches the cast
        // it.first is broken and crash will occur.
        if (isa<ConstantExpr>(it.second))
        {
            continue;
        }

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
        erase(instruction);
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
        return IGCLLVM::isOpaquePointerTy(pointerType) ? false :
            typeNeedsPromotion(IGCLLVM::getNonOpaquePtrEltTy(type), visitedTypes);       // Legacy code: getNonOpaquePtrEltTy
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
        newType = IGCLLVM::isOpaquePointerTy(pointerType) ? pointerType :
            PointerType::get(
            getOrCreatePromotedType(IGCLLVM::getNonOpaquePtrEltTy(type)),       // Legacy code: getNonOpaquePtrEltTy
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
bool PromoteBools::wasPromoted(llvm::Value* value)
{
    return promotedValuesCache.count(value);
}

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
    else if (auto inlineAsm = dyn_cast<InlineAsm>(value))
    {
        newValue = promoteInlineAsm(inlineAsm);
    }
    else if (auto insertValue = dyn_cast<InsertValueInst>(value))
    {
        newValue = promoteInsertValue(insertValue);
    }
    else if (auto load = dyn_cast<LoadInst>(value))
    {
        newValue = promoteLoad(load);
    }
    else if (auto phi = dyn_cast<PHINode>(value))
    {
        newValue = promotePHI(phi);
    }
    else if (auto store = dyn_cast<StoreInst>(value))
    {
        newValue = promoteStore(store);
    }
    else if (auto inttoptr = dyn_cast<IntToPtrInst>(value))
    {
        newValue = promoteIntToPtr(inttoptr);
    }
    else if (auto extractElement = dyn_cast<ExtractElementInst>(value))
    {
        newValue = promoteExtractElement(extractElement);
    }
    else if (auto insertElement = dyn_cast<InsertElementInst>(value))
    {
        newValue = promoteInsertElement(insertElement);
    }
    else if (auto instruction = dyn_cast<Instruction>(value))
    {
        for (auto& operand : instruction->operands())
        {
            if (wasPromoted(operand))
            {
                auto promoted = getOrCreatePromotedValue(operand);
                if (auto zext = dyn_cast<ZExtInst>(promoted))
                {
                    instruction->replaceUsesOfWith(operand, zext->getOperand(0));
                }
                else
                {
                    auto insertBefore = instruction;
                    if (auto operandInst = dyn_cast<Instruction>(operand))
                    {
                        insertBefore = operandInst->getNextNode();
                        while (isa_and_nonnull<PHINode>(insertBefore))
                        {
                            insertBefore = insertBefore->getNextNode();
                        }
                        if (!insertBefore)
                        {
                            insertBefore = operandInst->getParent()->getTerminator();
                        }
                    }
                    auto trunc = convertI8ToI1(promoted, insertBefore);
                    if (isa<Instruction>(trunc) && isa<Instruction>(promoted))
                    {
                        cast<Instruction>(trunc)->setDebugLoc(cast<Instruction>(promoted)->getDebugLoc());
                    }
                    instruction->replaceUsesOfWith(operand, trunc);
                }
            }
        }

        if (isIntegerTy(value, 1))
        {
            auto clone = instruction->clone();
            clone->insertBefore(instruction);
            instruction->replaceAllUsesWith(clone);
            newValue = convertI1ToI8(clone, instruction);
        }
    }

    if (newValue != value)
    {
        promotedValuesCache[value] = newValue;
        auto ty = value->getType();
        if (!IGCLLVM::isOpaquePointerTy(ty) && ty == newValue->getType()) {
          value->replaceAllUsesWith(newValue);
        } else {
          for (const auto &user : value->users()) {
            if (!wasPromoted(user)) {
              promotionQueue.push(user);
            }
          }
        }
    }
    return newValue;
}

template<typename T>
void PromoteBools::setPromotedAttributes(T* newCallOrFunc, const AttributeList& attributeList)
{
    auto getPromoted = [this, &newCallOrFunc](llvm::Attribute attr)
        {
            if (attr.isTypeAttribute())
            {
                return attr.getWithNewType(newCallOrFunc->getContext(),
                    getOrCreatePromotedType(attr.getValueAsType()));
            }
            else
            {
                return attr;
            }
        };

    // set function attributes
    for (const auto& attr : attributeList.getFnAttrs())
    {
        newCallOrFunc->addFnAttr(getPromoted(attr));
    }

    for (const auto& attr : attributeList.getRetAttrs())
    {
        newCallOrFunc->addRetAttr(getPromoted(attr));
    }

    // set params' attributes
    for (size_t i = 0; i < newCallOrFunc->arg_size(); i++)
    {
        if (!attributeList.hasParamAttrs(i))
        {
            continue;
        }

        for (const auto& attr : attributeList.getParamAttrs(i))
        {
            newCallOrFunc->addParamAttr(i, getPromoted(attr));
        }
    }
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

#if !defined(WDDM_ANDROID_IGC)
    if (BiFManager::BiFManagerHandler::IsBiF(function)
        || function->getName().startswith("__builtin_IB_"))
    {
        return function;
    }
#endif

    auto newFunction = Function::Create(
        dyn_cast<FunctionType>(getOrCreatePromotedType(function->getFunctionType())),
        function->getLinkage(),
        function->getName() + ".promoted",
        function->getParent()
    );

    newFunction->setCallingConv(function->getCallingConv());

    AttributeList attributeList = function->getAttributes();
    setPromotedAttributes(newFunction, attributeList);

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

        // Create a copy of the promoted attributes list so that
        // we can reset it after CloneFunctionInto
        AttributeList newAttributeList = newFunction->getAttributes();

        IGCLLVM::CloneFunctionInto(newFunction, function, argsMap,
            IGCLLVM::CloneFunctionChangeType::GlobalChanges, returns);

        // CloneFunctionInto set the potentially unpromoted attrs again.
        newFunction->setAttributes(newAttributeList);

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

            auto cast = castTo(newArg, arg.getType(), newFunction->getEntryBlock().getFirstNonPHI());
            for (auto& instruction : userInstructions)
            {
                instruction->replaceUsesOfWith(newArg, cast);
            }
        }

        // Fix ret statements
        if (function->getReturnType() != newFunction->getReturnType())
        {
            for (auto& basicBlock : *newFunction)
            {
                auto terminator = basicBlock.getTerminator();
                if (auto ret = dyn_cast<ReturnInst>(terminator))
                {
                    auto cast = castTo(ret->getReturnValue(), newFunction->getReturnType(), ret);
                    IRBuilder<> builder(ret);
                    auto newRet = builder.CreateRet(cast);
                    if (auto inst = dyn_cast<Instruction>(cast))
                    {
                        newRet->setDebugLoc(inst->getDebugLoc());
                    }
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

    auto newGlobalVariable = new GlobalVariable(
        *globalVariable->getParent(),
        getOrCreatePromotedType(globalVariable->getValueType()),
        globalVariable->isConstant(),
        globalVariable->getLinkage(),
        promoteConstant(globalVariable->getInitializer()),
        globalVariable->getName() + ".promoted",
        nullptr,
        GlobalValue::ThreadLocalMode::NotThreadLocal,
        globalVariable->getType()->getPointerAddressSpace());

    // Clone metadatas
    SmallVector<std::pair<unsigned, MDNode*>, 8> metadatas;
    globalVariable->getAllMetadata(metadatas);
    for (const auto& metadata : metadatas)
    {
        newGlobalVariable->addMetadata(metadata.first, *metadata.second);
    }
    return newGlobalVariable;
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
        if (IGCLLVM::isOpaquePointerTy(constantPointerNull->getType()))
        {
            return constant;
        }
        if (!typeNeedsPromotion(IGCLLVM::getNonOpaquePtrEltTy(constantPointerNull->getType())))       // Legacy code: getNonOpaquePtrEltTy
        {
            return constant;
        }

        auto newPointerElementType = getOrCreatePromotedType(IGCLLVM::getNonOpaquePtrEltTy(constantPointerNull->getType()));       // Legacy code: getNonOpaquePtrEltTy
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
    else if (auto constantExpr = dyn_cast<ConstantExpr>(constant))
    {
        if (constantExpr->isCast() && isa<GlobalValue>(constantExpr->getOperand(0)))
        {
            return ConstantExpr::getCast(
                constantExpr->getOpcode(),
                dyn_cast<Constant>(getOrCreatePromotedValue(constantExpr->getOperand(0))),
                constantExpr->getType()
            );
        }
        return constantExpr;
    }

    return constant;
}

AddrSpaceCastInst* PromoteBools::promoteAddrSpaceCast(AddrSpaceCastInst* addrSpaceCast)
{
    if (!addrSpaceCast || (!wasPromotedAnyOf(addrSpaceCast->operands()) && !typeNeedsPromotion(addrSpaceCast->getDestTy())))
    {
        return addrSpaceCast;
    }

    auto newAddrSpaceCast = new AddrSpaceCastInst(
        getOrCreatePromotedValue(addrSpaceCast->getOperand(0)),
        getOrCreatePromotedType(addrSpaceCast->getDestTy()),
        "",
        addrSpaceCast
    );
    newAddrSpaceCast->setDebugLoc(addrSpaceCast->getDebugLoc());
    return newAddrSpaceCast;
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
        alloca
    );
    newAlloca->setAlignment(IGCLLVM::getAlign(*alloca));
    newAlloca->setDebugLoc(alloca->getDebugLoc());
    return newAlloca;
}

Value* PromoteBools::promoteBitCast(BitCastInst* bitcast)
{
    if (!bitcast || (!wasPromotedAnyOf(bitcast->operands()) && !typeNeedsPromotion(bitcast->getDestTy())))
    {
        return bitcast;
    }

    auto newType = getOrCreatePromotedType(bitcast->getDestTy());
    if (bitcast->getSrcTy() == newType)
    {
        return bitcast->getOperand(0);
    }

    auto newBitcast = new BitCastInst(
        getOrCreatePromotedValue(bitcast->getOperand(0)),
        getOrCreatePromotedType(bitcast->getDestTy()),
        "",
        bitcast
    );
    newBitcast->setDebugLoc(bitcast->getDebugLoc());
    return newBitcast;
}

CallInst* PromoteBools::promoteIndirectCallOrInlineAsm(CallInst* call)
{
    IGC_ASSERT(call->isIndirectCall() || call->isInlineAsm());
    auto operand = call->getCalledOperand();
    auto functionType = call->getFunctionType();

    if (!wasPromotedAnyOf(call->args()) && !typeNeedsPromotion(functionType))
    {
        return call;
    }

    SmallVector<Value*, 8> newCallArguments;
    for (auto& arg : call->args())
    {
        newCallArguments.push_back(convertI1ToI8(getOrCreatePromotedValue(arg), call));
    }

    auto newCall = CallInst::Create(
        llvm::cast<llvm::FunctionType>(getOrCreatePromotedType(functionType)),
        getOrCreatePromotedValue(operand),
        newCallArguments,
        "",
        call
    );
    newCall->setCallingConv(call->getCallingConv());
    setPromotedAttributes(newCall, call->getAttributes());
    newCall->setDebugLoc(call->getDebugLoc());
    return newCall;
}

CallInst* PromoteBools::promoteCall(CallInst* call)
{
    if (!call)
    {
        return nullptr;
    }

    if (call->isIndirectCall() || call->isInlineAsm())
    {
        return promoteIndirectCallOrInlineAsm(call);
    }

    auto function = call->getCalledFunction();
    auto functionType = call->getFunctionType();

    if (!function || (!wasPromotedAnyOf(call->args()) && !typeNeedsPromotion(functionType)))
    {
        return call;
    }

    SmallVector<Value*, 8> newCallArguments;
    auto newFunction = function;

    // If the function wasn't promoted and therefore the signature is not changed
    // then we must convert promoted operands to the original types.
    if (!wasPromoted(function))
    {
        if (!wasPromotedAnyOf(call->args()))
        {
            return call;
        }

        for (auto& arg : call->args())
        {
            Value* newArg = arg;
            if (wasPromoted(newArg))
            {
                newArg = getOrCreatePromotedValue(newArg);
                if (isIntegerTy(arg, 1) && !isIntegerTy(newArg, 1))
                {
                    newArg = convertI8ToI1(newArg, call);
                }
            }

            newCallArguments.push_back(newArg);
        }
    }
    else
    {
        if (wasPromoted(newFunction) || typeNeedsPromotion(functionType))
        {
            newFunction = cast<Function>(getOrCreatePromotedValue(function));
        }

        for (auto& arg : call->args())
        {
            newCallArguments.push_back(convertI1ToI8(getOrCreatePromotedValue(arg), call));
        }
    }

    auto newCall = CallInst::Create(
        newFunction->getFunctionType(),
        newFunction,
        newCallArguments,
        "",
        call
    );
    newCall->setCallingConv(call->getCallingConv());
    setPromotedAttributes(newCall, call->getAttributes());
    newCall->setDebugLoc(call->getDebugLoc());
    return newCall;
}

ExtractValueInst* PromoteBools::promoteExtractValue(ExtractValueInst* extractValue)
{
    if (!extractValue)
    {
        return nullptr;
    }

    auto aggregateOp = extractValue->getAggregateOperand();
    if (!wasPromotedAnyOf(extractValue->operands()) && !typeNeedsPromotion(aggregateOp->getType()))
    {
        return extractValue;
    }

    auto newExtractValue = ExtractValueInst::Create(
        getOrCreatePromotedValue(aggregateOp),
        extractValue->getIndices(),
        "",
        extractValue
    );
    newExtractValue->setDebugLoc(extractValue->getDebugLoc());
    return newExtractValue;
}

GetElementPtrInst* PromoteBools::promoteGetElementPtr(GetElementPtrInst* getElementPtr)
{
    if (!getElementPtr || (!wasPromotedAnyOf(getElementPtr->operands()) && !typeNeedsPromotion(getElementPtr->getResultElementType())))
    {
        return getElementPtr;
    }

    auto promotedOperand = getOrCreatePromotedValue(getElementPtr->getPointerOperand());
    auto promotedType = getOrCreatePromotedType(getElementPtr->getSourceElementType());
    auto indices = SmallVector<Value*, 8>(getElementPtr->idx_begin(), getElementPtr->idx_end());

    auto newGetElementPtr = GetElementPtrInst::Create(
        promotedType,
        promotedOperand,
        indices,
        "",
        getElementPtr
    );
    newGetElementPtr->setDebugLoc(getElementPtr->getDebugLoc());
    newGetElementPtr->setIsInBounds(getElementPtr->isInBounds());
    return newGetElementPtr;
}

Value* PromoteBools::promoteICmp(ICmpInst* icmp)
{
    if (!icmp)
    {
        return nullptr;
    }

    auto op0 = icmp->getOperand(0);
    auto op1 = icmp->getOperand(1);

    auto promotedOp0 = convertI1ToI8(getOrCreatePromotedValue(op0), icmp);
    auto promotedOp1 = convertI1ToI8(getOrCreatePromotedValue(op1), icmp);

    auto newICmp = new ICmpInst(
        icmp,
        icmp->getPredicate(),
        promotedOp0,
        promotedOp1,
        ""
    );
    newICmp->setDebugLoc(icmp->getDebugLoc());
    return convertI1ToI8(newICmp, icmp);
}

InlineAsm* PromoteBools::promoteInlineAsm(InlineAsm* inlineAsm)
{
    if (!inlineAsm || !typeNeedsPromotion(inlineAsm->getFunctionType()))
    {
        return inlineAsm;
    }

    return InlineAsm::get(
        dyn_cast<FunctionType>(getOrCreatePromotedType(inlineAsm->getFunctionType())),
        inlineAsm->getAsmString(),
        inlineAsm->getConstraintString(),
        inlineAsm->hasSideEffects(),
        inlineAsm->isAlignStack(),
        inlineAsm->getDialect()
    );
}

InsertValueInst* PromoteBools::promoteInsertValue(InsertValueInst* insertValue)
{
    if (!insertValue)
    {
        return nullptr;
    }

    auto aggregateOp = insertValue->getAggregateOperand();
    auto insertedValueOp = insertValue->getInsertedValueOperand();
    if (!wasPromotedAnyOf(insertValue->operands()) && !typeNeedsPromotion(aggregateOp->getType()) && !typeNeedsPromotion(insertedValueOp->getType()))
    {
        return insertValue;
    }

    auto newAggregateOp = typeNeedsPromotion(aggregateOp->getType()) ? getOrCreatePromotedValue(aggregateOp) : aggregateOp;
    auto newInsertedValueOp = typeNeedsPromotion(insertedValueOp->getType()) ? getOrCreatePromotedValue(insertedValueOp) : insertedValueOp;

    auto newInsertValue = InsertValueInst::Create(
        newAggregateOp,
        newInsertedValueOp,
        insertValue->getIndices(),
        "",
        insertValue
    );
    newInsertValue->setDebugLoc(insertValue->getDebugLoc());
    return newInsertValue;
}

LoadInst* PromoteBools::promoteLoad(LoadInst* load)
{
    if (!load)
    {
        return nullptr;
    }

    auto src = load->getOperand(0);

    if (!wasPromotedAnyOf(load->operands()) && !typeNeedsPromotion(src->getType()))
    {
        return load;
    }

    auto newSrc = getOrCreatePromotedValue(src);
    auto newType = getOrCreatePromotedType(load->getType());
    auto newLoad = new LoadInst(
        newType,
        newSrc,
        "",
        load
    );
    newLoad->setAlignment(IGCLLVM::getAlign(*load));
    newLoad->setDebugLoc(load->getDebugLoc());
    return newLoad;
}

llvm::PHINode* PromoteBools::promotePHI(llvm::PHINode* phi)
{
    if (!phi || visitedPHINodes.count(phi) || (!wasPromotedAnyOf(phi->operands()) && !typeNeedsPromotion(phi->getType())))
    {
        return phi;
    }

    visitedPHINodes.insert(phi);

    auto newPhi = PHINode::Create(
        getOrCreatePromotedType(phi->getType()),
        phi->getNumIncomingValues(),
        "",
        phi
    );

    for (unsigned i = 0; i < phi->getNumIncomingValues(); ++i)
    {
        auto newIncomingValue = phi->getIncomingValue(i);
        if (wasPromoted(newIncomingValue) || typeNeedsPromotion(newIncomingValue->getType()))
        {
            newIncomingValue = getOrCreatePromotedValue(newIncomingValue);
            if (isIntegerTy(newIncomingValue, 1))
            {
                newIncomingValue = convertI1ToI8(newIncomingValue, phi->getIncomingBlock(i)->getTerminator());
            }
        }

        newPhi->addIncoming(
            newIncomingValue,
            phi->getIncomingBlock(i)
        );
    }
    newPhi->setDebugLoc(phi->getDebugLoc());
    return newPhi;
}

StoreInst* PromoteBools::promoteStore(StoreInst* store)
{
    if (!store)
    {
        return nullptr;
    }

    auto src = store->getOperand(0);
    auto dst = store->getOperand(1);

    if (!wasPromotedAnyOf(store->operands()) && !typeNeedsPromotion(src->getType()) && !typeNeedsPromotion(dst->getType()))
    {
        return store;
    }

    auto promotedSrc = convertI1ToI8(getOrCreatePromotedValue(src), store);
    auto promotedDst = convertI1ToI8(getOrCreatePromotedValue(dst), store);

    auto newStore = new StoreInst(
        promotedSrc,
        promotedDst,
        store->isVolatile(),
        store
    );
    newStore->setAlignment(IGCLLVM::getAlign(*store));
    newStore->setDebugLoc(store->getDebugLoc());
    return newStore;
}

IntToPtrInst* PromoteBools::promoteIntToPtr(IntToPtrInst* inttoptr)
{
    if (!inttoptr || (!wasPromotedAnyOf(inttoptr->operands()) && !typeNeedsPromotion(inttoptr->getDestTy())))
    {
        return inttoptr;
    }

    auto newIntToPtr = new IntToPtrInst(
        getOrCreatePromotedValue(inttoptr->getOperand(0)),
        getOrCreatePromotedType(inttoptr->getDestTy()),
        "",
        inttoptr
    );
    newIntToPtr->setDebugLoc(inttoptr->getDebugLoc());
    return newIntToPtr;
}

ExtractElementInst* PromoteBools::promoteExtractElement(ExtractElementInst* extractElement)
{
    if (!extractElement || (!wasPromotedAnyOf(extractElement->operands()) && !typeNeedsPromotion(extractElement->getType())))
    {
        return extractElement;
    }

    auto newExtractElem = ExtractElementInst::Create(
        getOrCreatePromotedValue(extractElement->getVectorOperand()),
        getOrCreatePromotedValue(extractElement->getIndexOperand()),
        "",
        extractElement
    );
    newExtractElem->setDebugLoc(extractElement->getDebugLoc());
    return newExtractElem;
}

InsertElementInst* PromoteBools::promoteInsertElement(InsertElementInst* insertElement)
{
    if (!insertElement || (!wasPromotedAnyOf(insertElement->operands()) && !typeNeedsPromotion(insertElement->getType())))
    {
        return insertElement;
    }

    auto newInsertElem = InsertElementInst::Create(
        getOrCreatePromotedValue(insertElement->getOperand(0)),
        getOrCreatePromotedValue(insertElement->getOperand(1)),
        getOrCreatePromotedValue(insertElement->getOperand(2)),
        "",
        insertElement
    );
    newInsertElem->setDebugLoc(insertElement->getDebugLoc());
    return newInsertElem;
}
