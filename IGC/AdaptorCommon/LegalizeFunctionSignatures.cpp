/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2024 Intel Corporation

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
#include "llvmWrapper/IR/Function.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include "llvm/IR/InstIterator.h"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvmWrapper/Transforms/Utils/Cloning.h>
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
// 2. Illegal int and int vector arguments are transformed to legal types that are a power of two (i8, i16, i32, i64).
//
// 3. The "byval" struct arguments smaller than 128-bits are transformed to pass-by-value.
//   Note: The SPIRV calling convention states that structures cannot be passed by
//         value, thus all structs are transformed by SPIRV FE to be passed by reference.
//         However we can optimize small struct args by converting them into back into
//         pass-by-value so that they can be passed on GRF instead of spilling to stack memory.
//
// 4. The "sret" struct argument smaller than 64-bits are transformed to return value
//   Note: Similar to the previous point, SPIRV FE converts struct return values to pass-by-refernce
//         through the "sret" argument. For small structs that fit into the return GRF, we can undo
//         this transformation to pass them by value instead.
//
// See IGC StackCall ABI for details on stackcall calling conventions.

//****************************************************************//

static const unsigned int MAX_RETVAL_SIZE_IN_BITS = 64;
static const unsigned int MAX_STRUCT_SIZE_IN_BITS = 128;
static const unsigned int MAX_SUBROUTINE_STRUCT_SIZE_IN_BITS = 512;

enum ReturnOpt
{
    RETURN_DEFAULT = 0,
    RETURN_BY_REF,
    RETURN_STRUCT,
    RETURN_LEGAL_INT
};

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

// Check if an int or int-vector argument type is a power of two
inline bool isLegalIntVectorType(const Module& M, Type* ty)
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

inline Type* LegalizedIntVectorType(const Module& M, Type* ty)
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
        IGCLLVM::FixedVectorType::get(IntegerType::get(M.getContext(), newSize), (unsigned)cast<IGCLLVM::FixedVectorType>(ty)->getNumElements());
}

// Returns true for structs smaller than 'structSize' and only contains primitive types
inline bool isLegalStructType(const Module& M, StructType* sTy, unsigned structSize)
{
    const DataLayout& DL = M.getDataLayout();
    if (sTy && DL.getStructLayout(sTy)->getSizeInBits() <= structSize)
    {
        for (const auto* EltTy : sTy->elements())
        {
            // Check if all elements are primitive types
            if (!EltTy->isSingleValueType() || EltTy->isVectorTy())
                return false;
        }
        return true;
    }
    return false;
}

inline bool isLegalSignatureType(const Module& M, Type* ty, bool isStackCall)
{
    if (isStackCall)
    {
        if (ty->isStructTy())
        {
            return isLegalStructType(M, cast<StructType>(ty), MAX_STRUCT_SIZE_IN_BITS);
        }
        else if (ty->isArrayTy())
        {
            return false;
        }
    }
    // Are all subroutine types legal?
    return true;
}

inline bool isPromotableStructType(const Module& M, Type* pointeeType, bool isStackCall)
{
    if (IGC_IS_FLAG_DISABLED(EnableByValStructArgPromotion))
        return false;
    const unsigned int maxSize = isStackCall ? MAX_STRUCT_SIZE_IN_BITS : MAX_SUBROUTINE_STRUCT_SIZE_IN_BITS;
    if (isa<StructType>(pointeeType))
    {
        return isLegalStructType(M, cast<StructType>(pointeeType), maxSize);
    }
    return false;
}

// Check if a function's first argument has the "sret" attribute and is a promotable struct type
inline bool FunctionHasPromotableSRetArg(const Module& M, const Function* F)
{
    if (F->getReturnType()->isVoidTy() &&
        !F->arg_empty() &&
        F->arg_begin()->hasStructRetAttr() &&
        isPromotableStructType(M, F->arg_begin()->getParamStructRetType(), F->hasFnAttribute("visaStackCall")))
    {
        return true;
    }
    return false;
}

// Promotes struct pointer to struct type
inline StructType* PromotedStructValueType(const Module& M, const Argument* arg)
{
    if (arg->getType()->isPointerTy())
    {
        if (arg->hasStructRetAttr() && arg->getParamStructRetType()->isStructTy())
        {
            return cast<StructType>(arg->getParamStructRetType());
        }
        else if (arg->hasByValAttr() && arg->getParamByValType()->isStructTy())
        {
            return cast<StructType>(arg->getParamByValType());
        }
    }
    IGC_ASSERT_MESSAGE(0, "Not implemented case");
    return nullptr;
}

// BE does not handle struct load/store, so instead store each element of the struct value to the GEP of the struct pointer
inline void StoreToStruct(IGCLLVM::IRBuilder<>& builder, Value* strVal, Value* strPtr)
{
    IGC_ASSERT(strPtr->getType()->isPointerTy());
    IGC_ASSERT(strVal->getType()->isStructTy());

    StructType* sTy = cast<StructType>(strVal->getType());
    for (unsigned i = 0; i < sTy->getNumElements(); i++)
    {
        Value* indices[] = { builder.getInt32(0), builder.getInt32(i) };
        Value* elementPtr = builder.CreateInBoundsGEP(strVal->getType(), strPtr, indices);
        Value* element = builder.CreateExtractValue(strVal, i);
        builder.CreateStore(element, elementPtr);
    }
}

// BE does not handle struct load/store, so instead load each element from the GEP struct pointer and insert it into the struct value
inline Value* LoadFromStruct(IGCLLVM::IRBuilder<>& builder, Value* strPtr, Type* ty)
{
    IGC_ASSERT(strPtr->getType()->isPointerTy());
    IGC_ASSERT(ty->isStructTy());

    Value* strVal = UndefValue::get(ty);
    StructType* sTy = cast<StructType>(strVal->getType());
    for (unsigned i = 0; i < sTy->getNumElements(); i++)
    {
        Value* indices[] = { builder.getInt32(0), builder.getInt32(i) };
        Value* elementPtr = builder.CreateInBoundsGEP(ty, strPtr, indices);
        Value* element = builder.CreateLoad(sTy->getElementType(i), elementPtr);
        strVal = builder.CreateInsertValue(strVal, element, i);
    }
    return strVal;
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

        // An internally-linked function that eventually gets inlined doesn't need this transformation
        if (pFunc->hasFnAttribute(llvm::Attribute::AlwaysInline) && pFunc->hasInternalLinkage())
            continue;

        if (pFunc->getName().empty())
        {
            // Empty function names can cause funny behavior later on
            // Always give it a name. If duplicates, LLVM will insert a unique tag
            pFunc->setName("__function__");
        }

        // For binary linking, calling a function outside the module is possible, so declaration
        // signatures has to be fixed as well
        if (pFunc->isDeclaration() &&
            !pFunc->hasFnAttribute("referenced-indirectly") &&
            !pFunc->hasFnAttribute("invoke_simd_target"))
        {
            continue;
        }

        ReturnOpt retTypeOption = ReturnOpt::RETURN_DEFAULT;
        bool fixArgType = false;
        std::vector<Type*> argTypes;

        bool isStackCall = pFunc->hasFnAttribute("visaStackCall");

        auto ai = pFunc->arg_begin();
        auto ei = pFunc->arg_end();

        bool functionHasPromotableSRetArg = FunctionHasPromotableSRetArg(M, pFunc);
        // Create the new function signature by replacing the illegal types
        if (functionHasPromotableSRetArg)
        {
            retTypeOption = ReturnOpt::RETURN_STRUCT;
            ai++; // Skip adding the first arg
        }
        else if (!isLegalSignatureType(M, pFunc->getReturnType(), isStackCall))
        {
            retTypeOption = ReturnOpt::RETURN_BY_REF;
            argTypes.push_back(PointerType::get(pFunc->getReturnType(), 0));
        }
        else if (!isLegalIntVectorType(M, pFunc->getReturnType()))
        {
            retTypeOption = ReturnOpt::RETURN_LEGAL_INT;
        }

        for (; ai != ei; ai++)
        {
            if (!isLegalIntVectorType(M, ai->getType()))
            {
                fixArgType = true;
                argTypes.push_back(LegalizedIntVectorType(M, ai->getType()));
            }
            else if (ai->hasByValAttr() &&
                isPromotableStructType(M, ai->getParamByValType(), isStackCall))
            {
                fixArgType = true;
                argTypes.push_back(PromotedStructValueType(M, ai));

                if (pFunc->isDeclaration())
                {
                  // Since this is declaration only, then FixFunctionBody() will not fix it
                  ai->removeAttr(llvm::Attribute::ByVal);

                  // When we're promoting struct pointer to struct,
                  // then we need to remove alignment attribute
                  if (ai->getType()->isPtrOrPtrVectorTy()) {
                    ai->removeAttr(llvm::Attribute::Alignment);
                  }
                }
            }
            else if (!isLegalSignatureType(M, ai->getType(), isStackCall))
            {
                fixArgType = true;
                argTypes.push_back(PointerType::get(ai->getType(), 0));
            }
            else
            {
                argTypes.push_back(ai->getType());
            }
        }

        if (retTypeOption != ReturnOpt::RETURN_DEFAULT || fixArgType)
        {
            // Clone function with new signature
            Type* returnType =
                retTypeOption == ReturnOpt::RETURN_BY_REF ? Type::getVoidTy(M.getContext()) :
                retTypeOption == ReturnOpt::RETURN_STRUCT ? PromotedStructValueType(M, pFunc->arg_begin()) :
                retTypeOption == ReturnOpt::RETURN_LEGAL_INT ? LegalizedIntVectorType(M, pFunc->getReturnType()) :
                pFunc->getReturnType();
            FunctionType* signature = FunctionType::get(returnType, argTypes, false);
            Function* pNewFunc = Function::Create(signature, pFunc->getLinkage(), pFunc->getName(), pFunc->getParent());
            pNewFunc->takeName(pFunc);
            pNewFunc->setCallingConv(pFunc->getCallingConv());

            // Copy attributes, but adjust their index if needed.
           CopyAttributesAndAdjustForSkippedFunctionArgs(pFunc, pNewFunc, functionHasPromotableSRetArg);

            // Since we need to pass in pointers to be dereferenced by the new function,
            // remove the memory attribute restrictions on read access.
            // Also we need to create allocas for these pointers, so set the flag to true
            if (retTypeOption == ReturnOpt::RETURN_BY_REF)
            {
                IGCLLVM::MemoryEffects ME(IGCLLVM::ModRefInfo::ModRef);
                IGCLLVM::setMemoryEffects(*pNewFunc, ME);
                pContext->m_instrTypes.hasNonPrimitiveAlloca = true;
            }

            // Map the old function to the new
            oldToNewFuncMap[pFunc] = pNewFunc;
        }
    }
}

void LegalizeFunctionSignatures::CopyAttributesAndAdjustForSkippedFunctionArgs(llvm::Function* pFunc, llvm::Function* pNewFunc, bool functionHasPromotableSRetArg)
{
  auto originalAttrs = pFunc->getAttributes();
  SmallVector<AttributeSet, 4> NewArgAttrs(pFunc->arg_size());

  bool shouldSkipFirstArg = functionHasPromotableSRetArg
    && pFunc->hasParamAttribute(0, llvm::Attribute::StructRet);

  if (shouldSkipFirstArg)
  {
    for (auto& Arg : pFunc->args())
    {
      if (Arg.getArgNo() == 0)
        continue;

      NewArgAttrs[Arg.getArgNo() - 1] = originalAttrs.getParamAttrs(Arg.getArgNo());
    }
  }
  else
  {
    for (auto& Arg : pFunc->args())
    {
      NewArgAttrs[Arg.getArgNo()] = originalAttrs.getParamAttrs(Arg.getArgNo());
    }
  }

  auto newAttrs = llvm::AttributeList::get(pFunc->getContext(), originalAttrs.getFnAttrs(), originalAttrs.getRetAttrs(), NewArgAttrs);
  pNewFunc->setAttributes(newAttrs);
}

void LegalizeFunctionSignatures::FixFunctionBody(Module& M)
{
    for (const auto& iter : oldToNewFuncMap)
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
            ReturnOpt retTypeOption = ReturnOpt::RETURN_DEFAULT;
            bool isStackCall = pFunc->hasFnAttribute("visaStackCall");
            Value* tempAllocaForSRetPointer = nullptr;
            Type* tempAllocaForSRetPointerTy = nullptr;
            llvm::SmallVector<llvm::Argument*, 8> ArgByVal;

            if (FunctionHasPromotableSRetArg(M, pFunc)) {
                retTypeOption = ReturnOpt::RETURN_STRUCT;
            }
            else if (!isLegalSignatureType(M, pFunc->getReturnType(), isStackCall)) {
                retTypeOption = ReturnOpt::RETURN_BY_REF;
                ++NewArgIt; // Skip first argument that we added.
            }
            else if (!isLegalIntVectorType(M, pFunc->getReturnType())) {
                retTypeOption = ReturnOpt::RETURN_LEGAL_INT;
            }

            // Fix the usages of arguments that have changed
            BasicBlock* EntryBB = BasicBlock::Create(M.getContext(), "", pNewFunc);
            IGCLLVM::IRBuilder<> builder(EntryBB);
            for (; OldArgIt != pFunc->arg_end(); ++OldArgIt)
            {
                if (OldArgIt == pFunc->arg_begin() && retTypeOption == ReturnOpt::RETURN_STRUCT)
                {
                    // Create a temp alloca to map the old argument. This will be removed later by SROA.
                    tempAllocaForSRetPointerTy = PromotedStructValueType(M, OldArgIt);
                    tempAllocaForSRetPointer = builder.CreateAlloca(tempAllocaForSRetPointerTy);
                    tempAllocaForSRetPointer = builder.CreateAddrSpaceCast(tempAllocaForSRetPointer, OldArgIt->getType());
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
                else if (OldArgIt->hasByValAttr() &&
                    isPromotableStructType(M, OldArgIt->getParamByValType(), isStackCall))
                {
                    AllocaInst* newArgPtr = builder.CreateAlloca(OldArgIt->getParamByValType());
                    // remove "byval" attrib since it is now pass-by-value
                    NewArgIt->removeAttr(llvm::Attribute::ByVal);
                    StoreToStruct(builder, &*NewArgIt, newArgPtr);
                    // cast back to original addrspace
                    IGC_ASSERT(OldArgIt->getType()->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC ||
                        OldArgIt->getType()->getPointerAddressSpace() == ADDRESS_SPACE_PRIVATE);
                    llvm::Value* castedNewArgPtr = builder.CreateAddrSpaceCast(newArgPtr, OldArgIt->getType());
                    VMap[&*OldArgIt] = castedNewArgPtr;
                }
                else if (!isLegalSignatureType(M, OldArgIt->getType(), isStackCall))
                {
                    // Load from pointer arg
                    Value* load = builder.CreateLoad(OldArgIt->getType(), &*NewArgIt);
                    VMap[&*OldArgIt] = load;
                    llvm::Attribute byValAttr = llvm::Attribute::getWithByValType(M.getContext(), OldArgIt->getType());
                    NewArgIt->addAttr(byValAttr);
                }
                else
                {
                    // No change, map old arg to new arg
                    VMap[&*OldArgIt] = &*NewArgIt;
                }
                ++NewArgIt;
            }

            // Clone the old function body into the new
            IGCLLVM::CloneFunctionInto(pNewFunc, pFunc, VMap,
                IGCLLVM::CloneFunctionChangeType::GlobalChanges, Returns);

            // Merge the BB for when extra instructions were created
            BasicBlock* ClonedEntryBB = cast<BasicBlock>(VMap[&*pFunc->begin()]);
            builder.CreateBr(ClonedEntryBB);
            MergeBlockIntoPredecessor(ClonedEntryBB);

            // Now fix the return values
            if (retTypeOption == ReturnOpt::RETURN_BY_REF)
            {
                // Add the 'noalias' and 'sret' attribute to arg0
                auto retArg = pNewFunc->arg_begin();
                retArg->addAttr(llvm::Attribute::NoAlias);
                retArg->addAttr(llvm::Attribute::getWithStructRetType(M.getContext(), pFunc->getReturnType()));

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
            else if (retTypeOption == ReturnOpt::RETURN_STRUCT)
            {
                // For "sret" returns, we load from the temp alloca created earlier and return the loaded value instead
                for (auto RetInst : Returns)
                {
                    IGCLLVM::IRBuilder<> builder(RetInst);
                    Value* retVal = LoadFromStruct(builder, tempAllocaForSRetPointer, tempAllocaForSRetPointerTy);
                    builder.CreateRet(retVal);
                    RetInst->eraseFromParent();
                }
            }
            else if (retTypeOption == ReturnOpt::RETURN_LEGAL_INT)
            {
                // Extend illegal int returns to legal type
                for (auto RetInst : Returns)
                {
                    IGCLLVM::IRBuilder<> builder(RetInst);
                    Value* retVal = RetInst->getReturnValue();
                    Type* retTy = retVal->getType();
                    retVal = builder.CreateZExt(retVal, LegalizedIntVectorType(M, retTy));
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
    for (const auto& it : oldToNewFuncMap)
    {
        Function* pFunc = it.first;
        Function* pNewFunc = it.second;

        std::vector<User*> pFuncUses(pFunc->user_begin(), pFunc->user_end());
        for (auto ui : pFuncUses)
        {
            CallInst* callInst = dyn_cast<CallInst>(ui);
            if (callInst && callInst->getCalledFunction() == pFunc)
            {
                // Find the callers of the transformed functions
                callsToFix.push_back(callInst);
            }
            else if (Instruction* inst = dyn_cast<Instruction>(ui))
            {
                // Any other uses can be replaced with a pointer cast
                IGCLLVM::IRBuilder<> builder(inst);
                Value* pCast = builder.CreatePointerCast(pNewFunc, pFunc->getType());
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
    ReturnOpt retTypeOption = ReturnOpt::RETURN_DEFAULT;
    bool fixArgType = false;
    bool isStackCall = !calledFunc || calledFunc->hasFnAttribute("visaStackCall");

    auto DL = M.getDataLayout();

    SmallVector<AttributeSet, 8> ArgAttrVec;
    AttributeList PAL = callInst->getAttributes();

    unsigned opNum = 0;

    // Check return type
    Value* returnPtr = nullptr;
    if (callInst->getType()->isVoidTy() &&
        IGCLLVM::getNumArgOperands(callInst) > 0 &&
        callInst->paramHasAttr(0, llvm::Attribute::StructRet) &&
        isPromotableStructType(M, callInst->getParamAttr(0, llvm::Attribute::StructRet).getValueAsType(), isStackCall))
    {
        opNum++; // Skip the first call operand
        retTypeOption = ReturnOpt::RETURN_STRUCT;
    }
    else if (!isLegalSignatureType(M, callInst->getType(), isStackCall))
    {
        // Create an alloca for the return type
        IGCLLVM::IRBuilder<> builder(callInst);
        returnPtr = builder.CreateAlloca(callInst->getType());
        callArgs.push_back(returnPtr);
        // Add "noalias" and "sret" to return value operand at callsite
        AttrBuilder ArgAttrs(M.getContext());
        ArgAttrs.addAttribute(llvm::Attribute::NoAlias);
        ArgAttrs.addStructRetAttr(callInst->getType());
        ArgAttrVec.push_back(AttributeSet::get(M.getContext(), ArgAttrs));
        retTypeOption = ReturnOpt::RETURN_BY_REF;
    }
    else if (!isLegalIntVectorType(M, callInst->getType()))
    {
        retTypeOption = ReturnOpt::RETURN_LEGAL_INT;
    }

    // Check call operands if it needs to be replaced
    for (; opNum < IGCLLVM::getNumArgOperands(callInst); opNum++)
    {
        Value* arg = callInst->getArgOperand(opNum);
        if (!isLegalIntVectorType(M, arg->getType()))
        {
            // extend the illegal int to a legal type
            IGCLLVM::IRBuilder<> builder(callInst);
            Value* extend = builder.CreateZExt(callInst->getOperand(opNum), LegalizedIntVectorType(M, arg->getType()));
            callArgs.push_back(extend);
            ArgAttrVec.push_back(AttributeSet());
            fixArgType = true;
        }
        else if (callInst->paramHasAttr(opNum, llvm::Attribute::ByVal) &&
            isPromotableStructType(M, callInst->getParamByValType(opNum), isStackCall))
        {
            // Map the new operand to the loaded value of the struct pointer
            IGCLLVM::IRBuilder<> builder(callInst);
            Value* newOp = LoadFromStruct(builder, callInst->getOperand(opNum), callInst->getParamByValType(opNum));
            callArgs.push_back(newOp);
            ArgAttrVec.push_back(AttributeSet());
            fixArgType = true;
        }
        else if (!isLegalSignatureType(M, arg->getType(), isStackCall))
        {
            // Create and store operand as an alloca, then pass as argument
            IGCLLVM::IRBuilder<> builder(callInst);
            Value* allocaV = builder.CreateAlloca(arg->getType());
            builder.CreateStore(callInst->getOperand(opNum), allocaV);
            callArgs.push_back(allocaV);
            auto byValAttr = llvm::Attribute::getWithByValType(M.getContext(), arg->getType());
            auto argAttrs = AttributeSet::get(M.getContext(), { byValAttr });
            ArgAttrVec.push_back(argAttrs);
            fixArgType = true;
        }
        else
        {
            // legal argument
            callArgs.push_back(arg);
            ArgAttrVec.push_back(PAL.getParamAttrs(opNum));
        }
    }

    if (retTypeOption != ReturnOpt::RETURN_DEFAULT || fixArgType)
    {
        IGCLLVM::IRBuilder<> builder(callInst);
        Value* newCalledValue = nullptr;
        FunctionType* newFnTy = nullptr;
        if (!calledFunc)
        {
            // Indirect call, cast the pointer type
            std::vector<Type*> argTypes;
            for (auto arg : callArgs)
            {
                argTypes.push_back(arg->getType());
            }
            Type* retType =
                retTypeOption == ReturnOpt::RETURN_BY_REF ? Type::getVoidTy(callInst->getContext()) :
                retTypeOption == ReturnOpt::RETURN_STRUCT ? PromotedStructValueType(M, callInst->getFunction()->getArg(0)) :
                retTypeOption == ReturnOpt::RETURN_LEGAL_INT ? LegalizedIntVectorType(M, callInst->getType()) :
                callInst->getType();
            newFnTy = FunctionType::get(retType, argTypes, false);
            Value* calledValue = IGCLLVM::getCalledValue(callInst);
            newCalledValue = builder.CreatePointerCast(calledValue, PointerType::get(newFnTy, 0));
        }
        else
        {
            // Directly call the new function pointer
            IGC_ASSERT(oldToNewFuncMap.find(calledFunc) != oldToNewFuncMap.end());
            newCalledValue = oldToNewFuncMap[calledFunc];
            newFnTy = cast<Function>(newCalledValue)->getFunctionType();
        }

        // Create the new call instruction
        CallInst* newCallInst = builder.CreateCall(newFnTy, newCalledValue, callArgs);
        newCallInst->setCallingConv(callInst->getCallingConv());
        newCallInst->setAttributes(AttributeList::get(M.getContext(), PAL.getFnAttrs(), PAL.getRetAttrs(), ArgAttrVec));
        newCallInst->setDebugLoc(callInst->getDebugLoc());

        if (retTypeOption == ReturnOpt::RETURN_BY_REF)
        {
            // Load the return value from the arg pointer before using it
            IGC_ASSERT(returnPtr);
            Value* load = builder.CreateLoad(cast<AllocaInst>(returnPtr)->getAllocatedType(), returnPtr);
            callInst->replaceAllUsesWith(load);
        }
        else if (retTypeOption == ReturnOpt::RETURN_STRUCT)
        {
            // Store the struct value into the orginal pointer operand
            StoreToStruct(builder, newCallInst, callInst->getArgOperand(0));
        }
        else if (retTypeOption == ReturnOpt::RETURN_LEGAL_INT)
        {
            // Truncate legal type back into original value
            Value* trunc = builder.CreateTrunc(newCallInst, callInst->getType());
            callInst->replaceAllUsesWith(trunc);
        }
        else
        {
            callInst->replaceAllUsesWith(newCallInst);
        }
        // Remove the old call
        callInst->eraseFromParent();
    }
}
