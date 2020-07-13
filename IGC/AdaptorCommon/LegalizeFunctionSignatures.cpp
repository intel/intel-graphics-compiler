/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "LegalizeFunctionSignatures.h"
#include "common/debug/Debug.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include "llvm/IR/InstIterator.h"
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
    llvm::IRBuilder<> builder(M.getContext());
    m_pBuilder = &builder;
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
        oldFunc->eraseFromParent();
    }

    if (m_localAllocaCreated)
    {
        // We are creating local allocas for structs/arrays, so set this flag to true
        m_pContext->m_instrTypes.hasNonPrimitiveAlloca = true;
    }

    m_pMdUtils->save(M.getContext());
    return m_funcSignatureChanged;
}

inline bool isLegalSignatureType(Type* ty)
{
    // Cannot pass struct/array by value
    if (ty->isStructTy() || ty->isArrayTy())
    {
        return false;
    }
    return true;
}

inline bool isLegalReturnType(Type* ty, bool isStackCall)
{
    // Cannot pass struct/array by value
    if (ty->isStructTy() || ty->isArrayTy())
    {
        return false;
    }
    // For stack calls, to avoid BE passing return value on stack,
    // convert returns sizes greater than 64bits to pass-by-argument
    if (ty->getPrimitiveSizeInBits() > 64 && isStackCall)
    {
        return false;
    }
    return true;
}

inline bool isLegalIntVectorType(Module& M, Type* ty)
{
    if (ty->isVectorTy() && ty->getVectorElementType()->isIntegerTy())
    {
        unsigned size = (unsigned)M.getDataLayout().getTypeSizeInBits(ty->getVectorElementType());
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
    IGC_ASSERT(nullptr != oldTy->getVectorElementType());
    IGC_ASSERT(oldTy->getVectorElementType()->isIntegerTy());

    const unsigned size = (unsigned)M.getDataLayout().getTypeSizeInBits(oldTy->getVectorElementType());
    unsigned newSize = 0;

    // Upscale the size to the next supported legal size
    if (size <= 8) newSize = 8;
    else if (size <= 16) newSize = 16;
    else if (size <= 32) newSize = 32;
    else if (size <= 64) newSize = 64;
    else IGC_ASSERT_MESSAGE(0, "Currently don't support upscaling int sizes > 64 bits");

    return VectorType::get(IntegerType::get(M.getContext(), newSize), oldTy->getVectorNumElements());
}

void LegalizeFunctionSignatures::FixFunctionSignatures()
{
    for (auto& FI : *m_pModule)
    {
        Function* pFunc = &FI;

        // Ignore the entry function
        if (isEntryFunc(m_pMdUtils, pFunc))
            continue;

        bool isIndirectCall = pFunc->hasFnAttribute("IndirectlyCalled");
        bool isStackCall = pFunc->hasFnAttribute("visaStackCall");

        // For binary linking, calling a function outside the module is possible, so declaration
        // signatures has to be fixed as well
        if (pFunc->isDeclaration() && !isIndirectCall)
        {
            continue;
        }

        bool fixReturnType = false;
        bool fixArgumentType = false;
        bool fixIntVectorType = false;
        std::vector<Type*> argTypes;

        // Create the new function signature by replacing the illegal types
        if (!isLegalReturnType(pFunc->getReturnType(), isStackCall))
        {
            fixReturnType = true;
            argTypes.push_back(PointerType::get(pFunc->getReturnType(), 0));
        }
        for (auto ai = pFunc->arg_begin(), ei = pFunc->arg_end(); ai != ei; ai++)
        {
            if (!isLegalSignatureType(ai->getType()))
            {
                fixArgumentType = true;
                argTypes.push_back(PointerType::get(ai->getType(), 0));
            }
            else if (!isLegalIntVectorType(*m_pModule, ai->getType()))
            {
                fixIntVectorType = true;
                argTypes.push_back(LegalizedIntVectorType(*m_pModule, ai->getType()));
            }
            else
            {
                argTypes.push_back(ai->getType());
            }
        }

        if (!fixReturnType && !fixArgumentType && !fixIntVectorType)
        {
            // Nothing to fix
            continue;
        }

        // Clone function with new signature
        Type* returnType = fixReturnType ? m_pBuilder->getVoidTy() : pFunc->getReturnType();
        Function* pNewFunc = CloneFunctionSignature(returnType, argTypes, pFunc);

        // Splice the old function directly into the new function body
        pNewFunc->getBasicBlockList().splice(pNewFunc->begin(), pFunc->getBasicBlockList());
        pNewFunc->takeName(pFunc);

        // Map the old function to the new
        oldToNewFuncMap[pFunc] = pNewFunc;

        // Since we need to pass in pointers to be dereferenced by the new function, remove the "readnone" attribute
        // Also we need to create allocas for these pointers, so set the flag to true
        if (fixReturnType || fixArgumentType)
        {
            pNewFunc->removeFnAttr(llvm::Attribute::ReadNone);
            m_localAllocaCreated = true;
        }
        // If we need to write to the return pointer, remove the "readonly" attribute as well
        if (fixReturnType) pNewFunc->removeFnAttr(llvm::Attribute::ReadOnly);

        if (!pNewFunc->isDeclaration())
        {
            m_pBuilder->SetInsertPoint(&(*pNewFunc->getEntryBlock().begin()));

            auto argINew = pNewFunc->arg_begin();

            if (fixReturnType)
            {
                IGC_ASSERT(argINew != pNewFunc->arg_end());
                IGC_ASSERT(argINew->getType()->isPointerTy());
                IGC_ASSERT(argINew->getType()->getPointerElementType() == pFunc->getReturnType());
                // Instead of returning the illegal type, we loop through all the return instructions and
                // replace them with a store into the argument pointer. The caller will then use the value stored in
                // this pointer instead of the returned value from the old function.
                for (auto bi = pNewFunc->begin(), be = pNewFunc->end(); bi != be; bi++)
                {
                    for (auto ii = bi->begin(), ie = bi->end(); ii != ie; ii++)
                    {
                        if (ReturnInst* retVal = dyn_cast<ReturnInst>(ii))
                        {
                            m_pBuilder->SetInsertPoint(retVal);
                            m_pBuilder->CreateStore(retVal->getOperand(0), &*argINew);
                            m_pBuilder->CreateRetVoid();
                            instsToErase.push_back(retVal);
                        }
                    }
                }
                argINew++;
            }

            // Fix the usages of arguments that have changed
            for (auto argI = pFunc->arg_begin(), argE = pFunc->arg_end(); argI != argE; argI++, argINew++)
            {
                if (argI->getType() != argINew->getType())
                {
                    if (!isLegalSignatureType(argI->getType()))
                    {
                        // Load from the pointer first before using
                        IGC_ASSERT(argINew->getType()->isPointerTy());
                        IGC_ASSERT(argINew->getType()->getPointerElementType() == argI->getType());
                        Value* load = m_pBuilder->CreateLoad(&*argINew);
                        argI->replaceAllUsesWith(load);

                        // Add byval attribute for struct/array value passing
                        argINew->addAttr(llvm::Attribute::ByVal);
                    }
                    else if (!isLegalIntVectorType(*m_pModule, argI->getType()))
                    {
                        // trunc argument back to original type
                        Value* trunc = m_pBuilder->CreateTrunc(&*argINew, argI->getType());
                        argI->replaceAllUsesWith(trunc);
                    }
                }
                else
                {
                    argI->replaceAllUsesWith(&*argINew);
                }
            }
        }
    }

    // If we get here, we know a function's signature has been modified
    m_funcSignatureChanged = true;
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
                m_pBuilder->SetInsertPoint(inst);
                Value* pCast = m_pBuilder->CreatePointerCast(pNewFunc, pFunc->getFunctionType());
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
    Function* callerFunc = callInst->getParent()->getParent();
    Function* calledFunc = callInst->getCalledFunction();
    SmallVector<Value*, 16> callArgs;
    bool needChange = false;

    bool isIndirectCall = !calledFunc || calledFunc->hasFnAttribute("IndirectlyCalled");
    bool isStackCall = isIndirectCall || calledFunc->hasFnAttribute("visaStackCall");

    // Check return type
    Value* returnPtr = nullptr;
    if (!isLegalReturnType(callInst->getType(), isStackCall))
    {
        // Create an alloca for the return type
        m_pBuilder->SetInsertPoint(&(*callerFunc->getEntryBlock().begin()));
        returnPtr = m_pBuilder->CreateAlloca(callInst->getType());
        callArgs.push_back(returnPtr);
        needChange = true;
    }

    // Check call operands if it needs to be replaced
    for (unsigned i = 0; i < callInst->getNumArgOperands(); i++)
    {
        Value* arg = callInst->getArgOperand(i);
        if (!isLegalSignatureType(arg->getType()))
        {
            // Create an alloca for each illegal argument and store the value
            // before calling the new function
            m_pBuilder->SetInsertPoint(&(*callerFunc->getEntryBlock().begin()));
            Value* fixedPtr = m_pBuilder->CreateAlloca(arg->getType());
            m_pBuilder->SetInsertPoint(callInst);
            m_pBuilder->CreateStore(arg, fixedPtr);
            callArgs.push_back(fixedPtr);
            needChange = true;
        }
        else if (!isLegalIntVectorType(*m_pModule, arg->getType()))
        {
            // extend the illegal int to a legal type
            m_pBuilder->SetInsertPoint(callInst);
            Value* extend = m_pBuilder->CreateZExt(arg, LegalizedIntVectorType(*m_pModule, arg->getType()));
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
        m_pBuilder->SetInsertPoint(callInst);

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
            Value* calledValue = callInst->getCalledValue();
            newCalledValue = m_pBuilder->CreatePointerCast(calledValue, PointerType::get(newFnTy, 0));
        }
        else
        {
            // Directly call the new function pointer
            IGC_ASSERT(oldToNewFuncMap.find(calledFunc) != oldToNewFuncMap.end());
            newCalledValue = oldToNewFuncMap[calledFunc];
        }

        // Create the new call instruction
        CallInst* newCallInst = m_pBuilder->CreateCall(newCalledValue, callArgs);
        newCallInst->setCallingConv(callInst->getCallingConv());

        if (returnPtr)
        {
            // Load the return value from the arg pointer before using it
            Value* load = m_pBuilder->CreateLoad(returnPtr);
            callInst->replaceAllUsesWith(load);
        }
        else
        {
            callInst->replaceAllUsesWith(newCallInst);
        }

        instsToErase.push_back(callInst);
    }
}

Function* LegalizeFunctionSignatures::CloneFunctionSignature(Type* ReturnType,
    std::vector<llvm::Type*>& argTypes,
    llvm::Function* pOldFunc)
{
    // Create function with the new signature
    FunctionType* signature = FunctionType::get(ReturnType, argTypes, false);
    Function* pNewFunc = Function::Create(signature, pOldFunc->getLinkage(), pOldFunc->getName(), pOldFunc->getParent());
    pNewFunc->copyAttributesFrom(pOldFunc);
    pNewFunc->setSubprogram(pOldFunc->getSubprogram());
    pOldFunc->setSubprogram(nullptr);

    // Copy the name of the old arguments
    auto newIter = pNewFunc->arg_begin();
    for (auto ai = pOldFunc->arg_begin(), ae = pOldFunc->arg_end(); ai != ae; ai++, newIter++)
    {
        newIter->setName(ai->getName());
    }

    // Clone the function metadata and remove the old one
    auto oldFuncIter = m_pMdUtils->findFunctionsInfoItem(pOldFunc);
    if (oldFuncIter != m_pMdUtils->end_FunctionsInfo())
    {
        m_pMdUtils->setFunctionsInfoItem(pNewFunc, oldFuncIter->second);
        m_pMdUtils->eraseFunctionsInfoItem(oldFuncIter);
    }

    auto modMD = m_pContext->getModuleMetaData();
    if (modMD->FuncMD.find(pOldFunc) != modMD->FuncMD.end())
    {
        modMD->FuncMD[pNewFunc] = modMD->FuncMD[pOldFunc];
        modMD->FuncMD.erase(pOldFunc);
    }

    return pNewFunc;
}