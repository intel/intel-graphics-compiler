/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LegalizeFunctionSignatures.h"
#include "common/debug/Debug.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvmWrapper/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include "llvm/IR/InstIterator.h"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/Types.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-legalize-function-signatures"
#define PASS_DESCRIPTION "Legalize calls to functions/subroutines and their signatures"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LegalizeFunctionSignatures, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(LegalizeFunctionSignatures, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char LegalizeFunctionSignatures::ID = 0;

LegalizeFunctionSignatures::LegalizeFunctionSignatures()
    : ModulePass(ID)
{
    initializeLegalizeFunctionSignaturesPass(*PassRegistry::getPassRegistry());
}

//*********************** PASS DESCRIPTION ***********************//

// This pass transforms functions and their callers to match IGC function call ABI.
//
// The following transformations are applied:
//
// 1. Return values larger than 64-bits are transformed to pass-by-reference.
//   Note: This means that the first argument of the function becomes a pointer allocated by the caller,
//         and the return value is stored to that pointer, and the function return type becomes void.
//
// 2. Illegal int and int vector arguments are transformed to legal types that are a power of two.
//
// 3. The "byval" struct arguments smaller than 64-bits are transformed to pass-by-value.
//   Note: We follow SPIRV calling convention here, which states that structures cannot be passed by
//         value, thus all structs are passed by reference. This transformation is done by the SPIRV
//         FE. However, we can optimize small structs by converting them into integers and pass them
//         by value to save on memory access.
//
// 4. The "sret" struct argument smaller than 64-bits are transformed to return value
//   Note: Similar to the previous point, SPIRV FE converts struct return values to pass-by-refernce
//         through the "sret" argument. For small structs that fit into the return GRF, we can undo
//         this transformation to pass them by return value instead.
//
// See IGC StackCall ABI for details on stackcall calling conventions.

//****************************************************************//

static const unsigned int MAX_STACKCALL_RETVAL_SIZE_IN_BITS = 64;
static const unsigned int MAX_STRUCT_ARGUMENT_SIZE_IN_BITS = 64;

bool LegalizeFunctionSignatures::runOnModule(Module& M)
{
    auto pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    // Creates a new function declaration with modified signature
    FixFunctionSignatures(M);
    // Transforms all callers of the old function to the new function
    FixFunctionUsers(M);
    // Clones the body and fix arguments for the new function
    FixFunctionBody(M);

    pMdUtils->save(M.getContext());
    return true;
}

// IGC stackcall ABI requires return values to be <= 64bits, since we don't support return value on stack.
// Stackcalls with return values > 64bits will need to be changed to pass-by-ref.
inline bool isLegalReturnType(Type* ty)
{
    // check return type size
    return ty->getPrimitiveSizeInBits() <= MAX_STACKCALL_RETVAL_SIZE_IN_BITS;
}

// Check if an int or int-vector argument type is a power of two
inline bool isLegalIntVectorType(Module& M, Type* ty)
{
    if (ty->isIntOrIntVectorTy())
    {
        unsigned size = (unsigned)M.getDataLayout().getTypeSizeInBits(ty->isIntegerTy() ? ty : cast<VectorType>(ty)->getElementType());
        switch (size)
        {
        case 8:
        case 16:
        case 32:
        case 64:
            return true;
        default:
            return false;
        }
    }
    return true;
}

inline Type* LegalizedIntVectorType(Module& M, Type* ty)
{
    IGC_ASSERT(ty && ty->isIntOrIntVectorTy());

    unsigned size = (unsigned)M.getDataLayout().getTypeSizeInBits(ty->isIntegerTy() ? ty : cast<VectorType>(ty)->getElementType());
    unsigned newSize = 0;

    // Upscale the size to the next supported legal size
    if (size <= 8) newSize = 8;
    else if (size <= 16) newSize = 16;
    else if (size <= 32) newSize = 32;
    else if (size <= 64) newSize = 64;
    else IGC_ASSERT_MESSAGE(0, "Currently don't support upscaling int sizes > 64 bits");

    return ty->isIntegerTy() ?
        cast<Type>(IntegerType::get(M.getContext(), newSize)) :
        IGCLLVM::FixedVectorType::get(IntegerType::get(M.getContext(), newSize), (unsigned)cast<VectorType>(ty)->getNumElements());
}

// Returns true for structures <= 64-bits and only contain primitive types
inline bool isPromotableStructType(Module& M, Type* ty)
{
    const DataLayout& DL = M.getDataLayout();
    if (ty->isPointerTy() &&
        ty->getPointerElementType()->isStructTy() &&
        DL.getTypeSizeInBits(ty->getPointerElementType()) <= MAX_STRUCT_ARGUMENT_SIZE_IN_BITS)
    {
        for (const auto* EltTy : cast<StructType>(ty->getPointerElementType())->elements())
        {
            // Check if all elements are primitive types
            if (!EltTy->isSingleValueType())
                return false;
        }
        return true;
    }
    return false;
}

// Check if a function's first argument has the "sret" attribute and is a promotable struct type
inline bool FunctionHasPromotableSRetArg(Module& M, Function* F)
{
    if (F->getReturnType()->isVoidTy() &&
        !F->arg_empty() &&
        F->arg_begin()->hasStructRetAttr() &&
        isPromotableStructType(M, F->arg_begin()->getType()))
    {
        return true;
    }
    return false;
}

inline Type* PromotedStructValueType(Module& M, Type* ty)
{
    IGC_ASSERT(ty->isPointerTy() && ty->getPointerElementType()->isStructTy());

    const DataLayout& DL = M.getDataLayout();
    unsigned size = (unsigned)DL.getTypeSizeInBits(ty->getPointerElementType());
    Type* convertedType = nullptr;

    if (size <= 32) convertedType = IntegerType::get(M.getContext(), 32);
    else if (size <= 64) convertedType = IntegerType::get(M.getContext(), 64);
    else IGC_ASSERT_MESSAGE(0, "Does not support promoting structures > 64bits");

    return convertedType;
}

void LegalizeFunctionSignatures::FixFunctionSignatures(Module& M)
{
    auto pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    auto pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    for (auto& FI : M)
    {
        Function* pFunc = &FI;

        // Ignore the entry function
        if (isEntryFunc(pMdUtils, pFunc))
            continue;

        // For binary linking, calling a function outside the module is possible, so declaration
        // signatures has to be fixed as well
        if (pFunc->isDeclaration() && !pFunc->hasFnAttribute("referenced-indirectly"))
        {
            continue;
        }

        bool legalizeReturnType = false;
        bool promoteSRetType = false;
        bool fixArgType = false;
        std::vector<Type*> argTypes;

        bool isStackCall = pFunc->hasFnAttribute("visaStackCall");

        auto ai = pFunc->arg_begin();
        auto ei = pFunc->arg_end();

        // Create the new function signature by replacing the illegal types
        if (isStackCall && !isLegalReturnType(pFunc->getReturnType()))
        {
            legalizeReturnType = true;
            argTypes.push_back(PointerType::get(pFunc->getReturnType(), 0));
        }
        else if (isStackCall && FunctionHasPromotableSRetArg(M, pFunc))
        {
            promoteSRetType = true;
            ai++; // Skip adding the first arg
        }

        for (; ai != ei; ai++)
        {
            if (!isLegalIntVectorType(M, ai->getType()))
            {
                fixArgType = true;
                argTypes.push_back(LegalizedIntVectorType(M, ai->getType()));
            }
            else if (isStackCall &&
                ai->hasByValAttr() &&
                isPromotableStructType(M, ai->getType()))
            {
                fixArgType = true;
                argTypes.push_back(PromotedStructValueType(M, ai->getType()));
            }
            else
            {
                argTypes.push_back(ai->getType());
            }
        }

        if (!legalizeReturnType && !promoteSRetType && !fixArgType)
        {
            // Nothing to fix
            continue;
        }

        // Clone function with new signature
        Type* returnType = legalizeReturnType ? Type::getVoidTy(M.getContext()) :
            promoteSRetType ? PromotedStructValueType(M, pFunc->arg_begin()->getType()) :
            pFunc->getReturnType();
        FunctionType* signature = FunctionType::get(returnType, argTypes, false);
        Function* pNewFunc = Function::Create(signature, pFunc->getLinkage(), pFunc->getName(), pFunc->getParent());
        pNewFunc->takeName(pFunc);
        pNewFunc->setCallingConv(pFunc->getCallingConv());

        // Since we need to pass in pointers to be dereferenced by the new function, remove the "readnone" attribute
        // Also we need to create allocas for these pointers, so set the flag to true
        if (legalizeReturnType)
        {
            pNewFunc->removeFnAttr(llvm::Attribute::ReadNone);
            pNewFunc->removeFnAttr(llvm::Attribute::ReadOnly);
            pContext->m_instrTypes.hasNonPrimitiveAlloca = true;
        }

        // Map the old function to the new
        oldToNewFuncMap[pFunc] = pNewFunc;
    }
}

void LegalizeFunctionSignatures::FixFunctionBody(Module& M)
{
    for (auto iter : oldToNewFuncMap)
    {
        Function* pFunc = iter.first;
        Function* pNewFunc = iter.second;

        if (!pFunc->isDeclaration())
        {
            auto DL = M.getDataLayout();
            ValueToValueMapTy VMap;
            llvm::SmallVector<llvm::ReturnInst*, 8> Returns;
            auto OldArgIt = pFunc->arg_begin();
            auto NewArgIt = pNewFunc->arg_begin();
            bool legalizeReturnType = false;
            bool promoteSRetType = false;
            bool isStackCall = pFunc->hasFnAttribute("visaStackCall");
            Value* tempAllocaForSRetPointer = nullptr;

            if (isStackCall && !isLegalReturnType(pFunc->getReturnType())) {
                legalizeReturnType = true;
                ++NewArgIt; // Skip first argument that we added.
            }
            else if (isStackCall && FunctionHasPromotableSRetArg(M, pFunc)) {
                promoteSRetType = true;
            }

            // Fix the usages of arguments that have changed
            BasicBlock* EntryBB = BasicBlock::Create(M.getContext(), "", pNewFunc);
            IGCLLVM::IRBuilder<> builder(EntryBB);
            for (; OldArgIt != pFunc->arg_end(); ++OldArgIt)
            {
                if (OldArgIt == pFunc->arg_begin() && promoteSRetType)
                {
                    // If the first arg is 'sret' and promotable to return value, create a temp
                    // alloca instruction to store its value first. We will load that value at function return.
                    // SROA pass should remove this alloca later.
                    tempAllocaForSRetPointer = builder.CreateAlloca(PromotedStructValueType(M, OldArgIt->getType()));
                    tempAllocaForSRetPointer = builder.CreateBitCast(tempAllocaForSRetPointer, OldArgIt->getType());
                    VMap[&*OldArgIt] = tempAllocaForSRetPointer;
                    continue;
                }

                NewArgIt->setName(OldArgIt->getName());
                if (!isLegalIntVectorType(M, OldArgIt->getType()))
                {
                    // trunc argument back to original type
                    Value* trunc = builder.CreateTrunc(&*NewArgIt, OldArgIt->getType());
                    VMap[&*OldArgIt] = trunc;
                }
                else if (isStackCall &&
                    OldArgIt->hasByValAttr() &&
                    isPromotableStructType(M, OldArgIt->getType()))
                {
                    // remove "byval" attrib since it is now pass-by-value
                    NewArgIt->removeAttr(llvm::Attribute::ByVal);
                    // store int argument data back into struct pointer type
                    Value* newArgPtr = builder.CreateAlloca(NewArgIt->getType());
                    builder.CreateStore(&*NewArgIt, newArgPtr);
                    newArgPtr = builder.CreateBitCast(newArgPtr, OldArgIt->getType());
                    VMap[&*OldArgIt] = newArgPtr;
                }
                else
                {
                    // No change, map old arg to new arg
                    VMap[&*OldArgIt] = &*NewArgIt;
                }
                ++NewArgIt;
            }

            // Clone the old function body into the new
            CloneFunctionInto(pNewFunc, pFunc, VMap, true, Returns);

            // Merge the BB for when extra instructions were created
            BasicBlock* ClonedEntryBB = cast<BasicBlock>(VMap[&*pFunc->begin()]);
            builder.CreateBr(ClonedEntryBB);
            MergeBlockIntoPredecessor(ClonedEntryBB);

            // Now fix the return values
            if (legalizeReturnType)
            {
                // Add "noalias" and "sret" to the return argument
                auto retArg = pNewFunc->arg_begin();
                retArg->addAttr(llvm::Attribute::NoAlias);
                retArg->addAttr(llvm::Attribute::StructRet);
                // Loop through all return instructions and store the old return value into the arg0 pointer
                const auto ptrSize = DL.getPointerSize();
                for (auto RetInst : Returns)
                {
                    IGCLLVM::IRBuilder<> builder(RetInst);
                    Type* retTy = RetInst->getReturnValue()->getType();
                    Value* returnedValPtr = builder.CreateAlloca(retTy);
                    builder.CreateStore(RetInst->getReturnValue(), returnedValPtr);
                    auto size = DL.getTypeAllocSize(retTy);
                    builder.CreateMemCpy(&*pNewFunc->arg_begin(), returnedValPtr, size, ptrSize);
                    builder.CreateRetVoid();
                    RetInst->eraseFromParent();
                }
            }
            else if (promoteSRetType)
            {
                // For "sret" returns, we load from the temp alloca created earlier, cast it to
                // the correct int type and return that instead.
                for (auto RetInst : Returns)
                {
                    IGCLLVM::IRBuilder<> builder(RetInst);
                    Argument* sretArg = pFunc->arg_begin();
                    Value* retVal = builder.CreateBitCast(tempAllocaForSRetPointer, PointerType::get(PromotedStructValueType(M, sretArg->getType()), 0));
                    retVal = builder.CreateLoad(retVal);
                    builder.CreateRet(retVal);
                    RetInst->eraseFromParent();
                }
            }
        }

        // Now that all instructions are transferred to the new func, delete the old func
        pFunc->removeDeadConstantUsers();
        pFunc->dropAllReferences();
        pFunc->removeFromParent();
    }
}

void LegalizeFunctionSignatures::FixFunctionUsers(Module& M)
{
    std::vector<CallInst*> callsToFix;

    // Check for all users of the old function and replace with the new signature
    for (auto it : oldToNewFuncMap)
    {
        Function* pFunc = it.first;
        Function* pNewFunc = it.second;

        for (auto ui = pFunc->user_begin(), ei = pFunc->user_end(); ui != ei; ++ui)
        {
            CallInst* callInst = dyn_cast<CallInst>(*ui);
            if (callInst && callInst->getCalledFunction() == pFunc)
            {
                // Find the callers of the transformed functions
                callsToFix.push_back(callInst);
            }
            else if (Instruction* inst = dyn_cast<Instruction>(*ui))
            {
                // Any other uses can be replaced with a pointer cast
                IGCLLVM::IRBuilder<> builder(inst);
                Value* pCast = builder.CreatePointerCast(pNewFunc, pFunc->getFunctionType());
                inst->replaceUsesOfWith(pFunc, pCast);
            }
        }
    }

    // Find all indirect calls that may require transformations
    for (auto& FI : M)
    {
        for (auto I = inst_begin(FI), E = inst_end(FI); I != E; I++)
        {
            if (CallInst* callInst = dyn_cast<CallInst>(&*I))
            {
                if (!callInst->isInlineAsm() && !callInst->getCalledFunction())
                {
                    callsToFix.push_back(callInst);
                }
            }
        }
    }

    for (auto call : callsToFix)
    {
        FixCallInstruction(M, call);
    }
}

void LegalizeFunctionSignatures::FixCallInstruction(Module& M, CallInst* callInst)
{
    Function* calledFunc = callInst->getCalledFunction();
    SmallVector<Value*, 16> callArgs;
    bool legalizeReturnType = false;
    bool promoteSRetType = false;
    bool fixArgType = false;
    bool isStackCall = !calledFunc || calledFunc->hasFnAttribute("visaStackCall");

    auto DL = M.getDataLayout();

    SmallVector<AttributeSet, 8> ArgAttrVec;
    AttributeList PAL = callInst->getAttributes();

    unsigned opNum = 0;

    // Check return type
    Value* returnPtr = nullptr;
    if (isStackCall && !isLegalReturnType(callInst->getType()))
    {
        // Create an alloca for the return type
        IGCLLVM::IRBuilder<> builder(callInst);
        returnPtr = builder.CreateAlloca(callInst->getType());
        callArgs.push_back(returnPtr);
        // Add "noalias" and "sret" to return value operand at callsite
        AttributeSet retAttrib;
        retAttrib = retAttrib.addAttribute(M.getContext(), llvm::Attribute::NoAlias);
        retAttrib = retAttrib.addAttribute(M.getContext(), llvm::Attribute::StructRet);
        ArgAttrVec.push_back(retAttrib);
        legalizeReturnType = true;
    }
    else if (isStackCall &&
        callInst->getType()->isVoidTy() &&
        callInst->getNumArgOperands() > 0 &&
        callInst->paramHasAttr(0, llvm::Attribute::StructRet) &&
        isPromotableStructType(M, callInst->getArgOperand(0)->getType()))
    {
        opNum++; // Skip the first call operand
        promoteSRetType = true;
    }

    // Check call operands if it needs to be replaced
    for (; opNum < callInst->getNumArgOperands(); opNum++)
    {
        Value* arg = callInst->getArgOperand(opNum);
        if (!isLegalIntVectorType(M, arg->getType()))
        {
            // extend the illegal int to a legal type
            IGCLLVM::IRBuilder<> builder(callInst);
            Value* extend = builder.CreateZExt(arg, LegalizedIntVectorType(M, arg->getType()));
            callArgs.push_back(extend);
            ArgAttrVec.push_back(AttributeSet());
            fixArgType = true;
        }
        else if (isStackCall &&
            callInst->paramHasAttr(opNum, llvm::Attribute::ByVal) &&
            isPromotableStructType(M, arg->getType()))
        {
            // cast and load from the int pointer instead of struct
            IGCLLVM::IRBuilder<> builder(callInst);
            Type* aggType = arg->getType()->getPointerElementType();
            Type* intType = IntegerType::get(M.getContext(), (unsigned)DL.getTypeSizeInBits(aggType));
            Value* newOp = builder.CreateBitCast(arg, PointerType::get(intType, 0));
            newOp = builder.CreateLoad(newOp);
            newOp = builder.CreateZExt(newOp, PromotedStructValueType(M, arg->getType()));
            callArgs.push_back(newOp);
            ArgAttrVec.push_back(AttributeSet());
            fixArgType = true;
        }
        else
        {
            // legal argument
            callArgs.push_back(arg);
            ArgAttrVec.push_back(PAL.getParamAttributes(opNum));
        }
    }

    if (legalizeReturnType || promoteSRetType || fixArgType)
    {
        IGCLLVM::IRBuilder<> builder(callInst);
        Value* newCalledValue = nullptr;
        if (!calledFunc)
        {
            // Indirect call, cast the pointer type
            std::vector<Type*> argTypes;
            for (auto arg : callArgs)
            {
                argTypes.push_back(arg->getType());
            }
            Type* retType = legalizeReturnType ? Type::getVoidTy(callInst->getContext()) :
                promoteSRetType ? PromotedStructValueType(M, callInst->getArgOperand(0)->getType()) :
                callInst->getType();
            FunctionType* newFnTy = FunctionType::get(retType, argTypes, false);
            Value* calledValue = IGCLLVM::getCalledValue(callInst);
            newCalledValue = builder.CreatePointerCast(calledValue, PointerType::get(newFnTy, 0));
        }
        else
        {
            // Directly call the new function pointer
            IGC_ASSERT(oldToNewFuncMap.find(calledFunc) != oldToNewFuncMap.end());
            newCalledValue = oldToNewFuncMap[calledFunc];
        }

        // Create the new call instruction
        CallInst* newCallInst = builder.CreateCall(newCalledValue, callArgs);
        newCallInst->setCallingConv(callInst->getCallingConv());
        newCallInst->setAttributes(AttributeList::get(M.getContext(), PAL.getFnAttributes(), PAL.getRetAttributes(), ArgAttrVec));
        newCallInst->setDebugLoc(callInst->getDebugLoc());

        if (legalizeReturnType)
        {
            // Load the return value from the arg pointer before using it
            IGC_ASSERT(returnPtr);
            Value* load = builder.CreateLoad(returnPtr);
            callInst->replaceAllUsesWith(load);
        }
        else if (promoteSRetType)
        {
            // Store the new function's int return value into the original pointer.
            // SROA will remove the original alloca later and directly use the return value.
            Type* aggType = callInst->getArgOperand(0)->getType()->getPointerElementType();
            Type* intType = IntegerType::get(M.getContext(), (unsigned)DL.getTypeSizeInBits(aggType));
            Value* retVal = builder.CreateTrunc(newCallInst, intType);
            Value* allocaPtr = builder.CreateBitCast(callInst->getArgOperand(0), PointerType::get(intType, 0));
            builder.CreateStore(retVal, allocaPtr);
        }
        else
        {
            callInst->replaceAllUsesWith(newCallInst);
        }
        // Remove the old call
        callInst->eraseFromParent();
    }
}
