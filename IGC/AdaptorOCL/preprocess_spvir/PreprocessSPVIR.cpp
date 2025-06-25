/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PreprocessSPVIR.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Support/Regex.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/BuiltinTypes.h"

#include <regex>
#include <unordered_set>

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-preprocess-spvir"
#define PASS_DESCRIPTION "Adjust SPV-IR produced by Khronos SPIRV-LLVM Translator to be consumable by IGC BiFModule"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PreprocessSPVIR, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(PreprocessSPVIR, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PreprocessSPVIR::ID = 0;

PreprocessSPVIR::PreprocessSPVIR() : ModulePass(ID)
{
    initializePreprocessSPVIRPass(*PassRegistry::getPassRegistry());
}

void PreprocessSPVIR::createCallAndReplace(CallInst& oldCallInst, StringRef newFuncName, std::vector<Value*>& args )
{
    Function* F = oldCallInst.getCalledFunction();
    IGC_ASSERT(F);

    std::vector<Type*> argTypes;
    for (auto arg : args)
        argTypes.push_back(arg->getType());

    FunctionType* FT = FunctionType::get(oldCallInst.getType(), argTypes, false);
    auto* newFunction = cast<Function>(m_Module->getOrInsertFunction(newFuncName, FT, F->getAttributes()));
    newFunction->setCallingConv(F->getCallingConv());
    CallInst* newCall = CallInst::Create(newFunction, args, "", &oldCallInst);
    newCall->setCallingConv(oldCallInst.getCallingConv());
    newCall->setAttributes(oldCallInst.getAttributes());
    oldCallInst.replaceAllUsesWith(newCall);
}

// Replace functions like:
//  i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)*)
//  i32 @_Z18__spirv_ocl_printfPU3AS2ci(i8 addrspace(2)*, i32)
// With:
//  i32 @printf(i8 addrspace(2)*, ...)
//
// Khronos SPV-IR represents printf function as a non-variadic one. Since
// IGC supports clang-consistent representation of printf (which is unmangled,
// variadic function), all printf calls must get replaced.
void PreprocessSPVIR::visitOpenCLEISPrintf(llvm::CallInst& CI)
{
    FunctionType* FT = FunctionType::get(CI.getType(), Type::getInt8PtrTy(m_Module->getContext(), 2), true);
    Function* newPrintf = cast<Function>(m_Module->getOrInsertFunction("printf", FT));
    CI.setCalledFunction(newPrintf);

    m_changed = true;
}

bool PreprocessSPVIR::isSPVIR(StringRef funcName)
{
    bool is_regular_pattern = Regex("_Z[0-9]+__spirv_[A-Z].*").match(funcName);
    bool is_eis_pattern = Regex("_Z[0-9]+__spirv_ocl_[a-z].*").match(funcName);

    return is_regular_pattern || is_eis_pattern;
}

bool PreprocessSPVIR::hasArrayArg(llvm::Function& F)
{
    for (auto& Arg : F.args())
    {
        if (Arg.getType()->isArrayTy())
            return true;
    }
    return false;
}

void PreprocessSPVIR::processBuiltinsWithArrayArguments(llvm::Function& F)
{
    if (F.user_empty()) return;

    IGC_ASSERT(F.hasName());
    StringRef origName = F.getName();

    // add postfix to original function name, since new function with original
    // name and different arguments types is going to be created
    F.setName(origName + ".old");

    std::unordered_set<CallInst*> callInstsToErase;
    for (auto* user : F.users())
    {
        if (auto* CI = dyn_cast<CallInst>(user))
        {
            std::vector<Value*> newArgs;
            for (auto& arg : CI->args())
            {
                auto* T = arg->getType();
                if (!T->isArrayTy())
                {
                    // leave non-array arguments unchanged
                    newArgs.push_back(arg);
                    continue;
                }

                auto FBegin = CI->getFunction()->begin()->getFirstInsertionPt();
                auto* Alloca = new AllocaInst(T, 0, "", &(*FBegin));
                new StoreInst(arg, Alloca, false, CI);
                auto* Zero =
                    ConstantInt::getNullValue(Type::getInt32Ty(T->getContext()));
                Value* Index[] = { Zero, Zero };
                auto* GEP = GetElementPtrInst::CreateInBounds(T, Alloca, Index, "", CI);
                newArgs.push_back(GEP);
            }

            createCallAndReplace(*CI, origName, newArgs);
            callInstsToErase.insert(CI);
        }
    }

    for (auto* CI : callInstsToErase)
        CI->eraseFromParent();

    F.eraseFromParent();
    m_changed = true;
}

void PreprocessSPVIR::processBuiltinsWithArrayArguments()
{
    for (auto& F : make_early_inc_range(m_Module->functions()))
    {
        if (F.hasName() && F.isDeclaration())
        {
            StringRef Name = F.getName();
            if (hasArrayArg(F) && isSPVIR(Name))
            {
                if (Name.contains("BuildNDRange"))
                {
                    processBuiltinsWithArrayArguments(F);
                }
            }
        }
    }
}

void PreprocessSPVIR::visitCallInst(CallInst& CI)
{
    Function* F = CI.getCalledFunction();
    if (!F) return;

    StringRef Name = F->getName();
    if (!isSPVIR(Name)) return;

    if (Name.contains("printf"))
    {
        visitOpenCLEISPrintf(CI);
    }
}

bool PreprocessSPVIR::runOnModule(Module& M) {
    m_Module = static_cast<IGCLLVM::Module*>(&M);
    IRBuilder<> builder(M.getContext());
    m_Builder = &builder;

    // Change arguments with array type to pointer type to match signature
    // produced by Clang.
    processBuiltinsWithArrayArguments();

    visit(M);

    // Retype function arguments of OpenCL types represented as TargetExtTy to
    // use opaque pointers instead. This is necessary to match function
    // signatures generated by Clang, given that it only has partial TargetExtTy
    // support.
#if LLVM_VERSION_MAJOR >= 16
    retypeOpenCLTargetExtTyArgs(&M);
#endif

    return m_changed;
}
