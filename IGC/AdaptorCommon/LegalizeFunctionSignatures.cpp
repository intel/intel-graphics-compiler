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

bool LegalizeFunctionSignatures::runOnModule(Module& M)
{
    m_pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    m_pModule = &M;
    m_funcSignatureChanged = false;
    m_localAllocaCreated = false;

    FixFunctionSignatures();
    FixFunctionUsers();

    // Clean up
    for (auto inst : instsToErase)
    {
        inst->eraseFromParent();
    }
    for (auto it : oldToNewFuncMap)
    {
        Function* oldFunc = it.first;
        oldFunc->removeDeadConstantUsers();
        oldFunc->dropAllReferences();
        oldFunc->removeFromParent();
    }

    if (m_localAllocaCreated)
    {
        // We are creating local allocas for structs/arrays, so set this flag to true
        m_pContext->m_instrTypes.hasNonPrimitiveAlloca = true;
    }

    m_pMdUtils->save(M.getContext());
    return m_funcSignatureChanged;
}

// IGC stackcall ABI requires return values to be <= 64bits, since we don't support return value on stack.
// Stackcalls with return values > 64bits will need to be changed to pass-by-ref.
inline bool isLegalReturnType(Type* ty, bool isStackCall)
{
    // for functions, check return type and stackcall attribute
    if (ty->getPrimitiveSizeInBits() > 64 && isStackCall)
    {
        return false;
    }
    return true;
}

inline bool isLegalIntVectorType(Module& M, Type* ty)
{
    if (ty->isVectorTy() && cast<VectorType>(ty)->getElementType()->isIntegerTy())
    {
        unsigned size = (unsigned)M.getDataLayout().getTypeSizeInBits(cast<VectorType>(ty)->getElementType());
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

inline Type* LegalizedIntVectorType(Module& M, const Type* const oldTy)
{
    IGC_ASSERT(nullptr != oldTy);
    IGC_ASSERT(oldTy->isVectorTy());
    IGC_ASSERT(nullptr != cast<VectorType>(oldTy)->getElementType());
    IGC_ASSERT(cast<VectorType>(oldTy)->getElementType()->isIntegerTy());

    const unsigned size = (unsigned)M.getDataLayout().getTypeSizeInBits(cast<VectorType>(oldTy)->getElementType());
    unsigned newSize = 0;

    // Upscale the size to the next supported legal size
    if (size <= 8) newSize = 8;
    else if (size <= 16) newSize = 16;
    else if (size <= 32) newSize = 32;
    else if (size <= 64) newSize = 64;
    else IGC_ASSERT_MESSAGE(0, "Currently don't support upscaling int sizes > 64 bits");

    return IGCLLVM::FixedVectorType::get(IntegerType::get(M.getContext(), newSize), (unsigned)cast<VectorType>(oldTy)->getNumElements());
}

void LegalizeFunctionSignatures::FixFunctionSignatures()
{
    for (auto& FI : *m_pModule)
    {
        Function* pFunc = &FI;

        // Ignore the entry function
        if (isEntryFunc(m_pMdUtils, pFunc))
            continue;

        // For binary linking, calling a function outside the module is possible, so declaration
        // signatures has to be fixed as well
        if (pFunc->isDeclaration() && !pFunc->hasFnAttribute("referenced-indirectly"))
        {
            continue;
        }

        bool fixReturnType = false;
        bool fixIntVectorType = false;
        std::vector<Type*> argTypes;

        // Create the new function signature by replacing the illegal types
        if (!isLegalReturnType(pFunc->getReturnType(), pFunc->hasFnAttribute("visaStackCall")))
        {
            fixReturnType = true;
            argTypes.push_back(PointerType::get(pFunc->getReturnType(), 0));
        }
        for (auto ai = pFunc->arg_begin(), ei = pFunc->arg_end(); ai != ei; ai++)
        {
            if (!isLegalIntVectorType(*m_pModule, ai->getType()))
            {
                fixIntVectorType = true;
                argTypes.push_back(LegalizedIntVectorType(*m_pModule, ai->getType()));
            }
            else
            {
                argTypes.push_back(ai->getType());
            }
        }

        if (!fixReturnType && !fixIntVectorType)
        {
            // Nothing to fix
            continue;
        }

        // Clone function with new signature
        Type* returnType = fixReturnType ? Type::getVoidTy(m_pModule->getContext()) : pFunc->getReturnType();
        FunctionType* signature = FunctionType::get(returnType, argTypes, false);
        Function* pNewFunc = Function::Create(signature, pFunc->getLinkage(), pFunc->getName(), pFunc->getParent());
        pNewFunc->takeName(pFunc);
        pNewFunc->setCallingConv(pFunc->getCallingConv());

        // Map the old function to the new
        oldToNewFuncMap[pFunc] = pNewFunc;

        // Since we need to pass in pointers to be dereferenced by the new function, remove the "readnone" attribute
        // Also we need to create allocas for these pointers, so set the flag to true
        if (fixReturnType)
        {
            pNewFunc->removeFnAttr(llvm::Attribute::ReadNone);
            pNewFunc->removeFnAttr(llvm::Attribute::ReadOnly);
            m_localAllocaCreated = true;
        }

        if (!pFunc->isDeclaration())
        {
            ValueToValueMapTy VMap;
            llvm::SmallVector<llvm::ReturnInst*, 8> Returns;
            auto OldArgIt = pFunc->arg_begin();
            auto NewArgIt = pNewFunc->arg_begin();

            if (fixReturnType)
                ++NewArgIt; // Skip first argument that we added.

            // Fix the usages of arguments that have changed
            BasicBlock* EntryBB = BasicBlock::Create(m_pModule->getContext(), "", pNewFunc);
            IGCLLVM::IRBuilder<> builder(EntryBB);
            for (; OldArgIt != pFunc->arg_end(); ++OldArgIt, ++NewArgIt)
            {
                NewArgIt->setName(OldArgIt->getName());
                if (!isLegalIntVectorType(*m_pModule, OldArgIt->getType()))
                {
                    // trunc argument back to original type
                    Value* trunc = builder.CreateTrunc(&*NewArgIt, OldArgIt->getType());
                    VMap[&*OldArgIt] = trunc;
                }
                else
                {
                    // No change, map old arg to new arg
                    VMap[&*OldArgIt] = &*NewArgIt;
                }
            }

            // Clone the old function body into the new
            CloneFunctionInto(pNewFunc, pFunc, VMap, true, Returns);

            // Merge the BB for when extra instructions were created
            BasicBlock* ClonedEntryBB = cast<BasicBlock>(VMap[&*pFunc->begin()]);
            builder.CreateBr(ClonedEntryBB);
            MergeBlockIntoPredecessor(ClonedEntryBB);

            // Now fix the return values
            if (fixReturnType)
            {
                // Add "noalias" and "sret" to the return argument
                auto retArg = pNewFunc->arg_begin();
                retArg->addAttr(llvm::Attribute::NoAlias);
                retArg->addAttr(llvm::Attribute::StructRet);
                // Loop through all return instructions and store the old return value into the arg0 pointer
                auto DL = m_pModule->getDataLayout();
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
        }
    }

    // We know a function signature changed if the map is not empty
    m_funcSignatureChanged = !oldToNewFuncMap.empty();
}

void LegalizeFunctionSignatures::FixFunctionUsers()
{
    // Check for all users of the old function and replace with the new signature
    for (auto it : oldToNewFuncMap)
    {
        Function* pFunc = it.first;
        Function* pNewFunc = it.second;

        for (auto ui = pFunc->user_begin(), ei = pFunc->user_end(); ui != ei; ui++)
        {
            CallInst* callInst = dyn_cast<CallInst>(*ui);
            if (callInst && callInst->getCalledFunction() == pFunc)
            {
                // Replace direct call of any modified functions
                FixCallInstruction(callInst);
            }
            else if (Instruction* inst = dyn_cast<Instruction>(*ui))
            {
                // Any other uses can be replaced with a pointer cast
                IGCLLVM::IRBuilder<> builder(inst);
                Value* pCast = builder.CreatePointerCast(pNewFunc, pFunc->getFunctionType());
                inst->replaceUsesOfWith(pFunc, pCast);
                instsToErase.push_back(inst);
            }
        }
    }

    // Fix all indirect calls
    for (auto& FI : *m_pModule)
    {
        for (auto I = inst_begin(FI), E = inst_end(FI); I != E; I++)
        {
            if (CallInst* callInst = dyn_cast<CallInst>(&*I))
            {
                if (!callInst->isInlineAsm() && !callInst->getCalledFunction())
                {
                    FixCallInstruction(callInst);
                }
            }
        }
    }
}

void LegalizeFunctionSignatures::FixCallInstruction(CallInst* callInst)
{
    Function* calledFunc = callInst->getCalledFunction();
    SmallVector<Value*, 16> callArgs;
    bool needChange = false;

    // Check return type
    Value* returnPtr = nullptr;
    if (!isLegalReturnType(callInst->getType(), !calledFunc || calledFunc->hasFnAttribute("visaStackCall")))
    {
        // Create an alloca for the return type
        IGCLLVM::IRBuilder<> builder(callInst);
        returnPtr = builder.CreateAlloca(callInst->getType());
        callArgs.push_back(returnPtr);
        needChange = true;
    }

    // Check call operands if it needs to be replaced
    for (unsigned i = 0; i < callInst->getNumArgOperands(); i++)
    {
        Value* arg = callInst->getArgOperand(i);
        if (!isLegalIntVectorType(*m_pModule, arg->getType()))
        {
            // extend the illegal int to a legal type
            IGCLLVM::IRBuilder<> builder(callInst);
            Value* extend = builder.CreateZExt(arg, LegalizedIntVectorType(*m_pModule, arg->getType()));
            callArgs.push_back(extend);
            needChange = true;
        }
        else
        {
            // legal argument
            callArgs.push_back(arg);
        }
    }

    if (needChange)
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
            Type* retType = returnPtr ? Type::getVoidTy(callInst->getContext()) : callInst->getType();
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

        if (returnPtr)
        {
            // Add "noalias" and "sret" to return value operand at callsite
            newCallInst->addAttribute(1, llvm::Attribute::AttrKind::NoAlias);
            newCallInst->addAttribute(1, llvm::Attribute::AttrKind::StructRet);

            // Load the return value from the arg pointer before using it
            Value* load = builder.CreateLoad(returnPtr);
            callInst->replaceAllUsesWith(load);
        }
        else
        {
            callInst->replaceAllUsesWith(newCallInst);
        }

        instsToErase.push_back(callInst);
    }
}
