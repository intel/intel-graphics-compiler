/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "HandleSpirvDecorationMetadata.h"
#include "AdaptorOCL/Utils/CacheControlsHelper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"

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
#include <iostream>

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-handle-spirv-decoration-metadata"
#define PASS_DESCRIPTION "Handle spirv.Decoration metadata"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(HandleSpirvDecorationMetadata, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(HandleSpirvDecorationMetadata, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char HandleSpirvDecorationMetadata::ID = 0;

HandleSpirvDecorationMetadata::HandleSpirvDecorationMetadata() : ModulePass(ID)
{
    initializeHandleSpirvDecorationMetadataPass(*PassRegistry::getPassRegistry());
}

bool HandleSpirvDecorationMetadata::runOnModule(Module& module)
{
    m_Metadata = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    m_pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_Module = &module;

    handleInstructionsDecorations();
    handleGlobalVariablesDecorations();

    return m_changed;
}

void HandleSpirvDecorationMetadata::handleInstructionsDecorations()
{
    visit(m_Module);
}

void HandleSpirvDecorationMetadata::handleGlobalVariablesDecorations()
{
    for (auto& globalVariable : m_Module->globals())
    {
        auto spirvDecorations = parseSPIRVDecorationsFromMD(&globalVariable);
        for (auto& [DecorationId, MDNodes] : spirvDecorations)
        {
            switch (DecorationId)
            {
                // IDecHostAccessINTEL
                case 6147:
                {
                    IGC_ASSERT_MESSAGE(MDNodes.size() == 1,
                        "Only one HostAccessINTEL decoration can be applied to a single global variable!");
                    handleHostAccessIntel(globalVariable, *MDNodes.begin());
                    break;
                }
                default: continue;
            }
        }
    }
}

void HandleSpirvDecorationMetadata::handleHostAccessIntel(GlobalVariable& globalVariable, MDNode* node)
{
    globalVariable.addAttribute("host_var_name", dyn_cast<MDString>(node->getOperand(2))->getString());

    m_changed = true;
    m_Metadata->capabilities.globalVariableDecorationsINTEL = true;
}

void HandleSpirvDecorationMetadata::visitLoadInst(LoadInst& I)
{
    auto spirvDecorations = parseSPIRVDecorationsFromMD(I.getPointerOperand());
    for (auto& [DecorationId, MDNodes] : spirvDecorations)
    {
        switch (DecorationId)
        {
            // IDecCacheControlLoadINTEL
            case DecorationIdCacheControlLoad:
            {
                handleCacheControlINTEL<LoadCacheControl>(I, MDNodes);
                break;
            }
            default: continue;
        }
    }
}

void HandleSpirvDecorationMetadata::visitStoreInst(StoreInst& I)
{
    auto spirvDecorations = parseSPIRVDecorationsFromMD(I.getPointerOperand());
    for (auto& [DecorationId, MDNodes] : spirvDecorations)
    {
        switch (DecorationId)
        {
            // IDecCacheControlStoreINTEL
            case DecorationIdCacheControlStore:
            {
                handleCacheControlINTEL<StoreCacheControl>(I, MDNodes);
                break;
            }
            default: continue;
        }
    }
}

void HandleSpirvDecorationMetadata::visit2DBlockReadCallInst(CallInst& I, StringRef unmangledName)
{
    Value* ptr = I.getArgOperand(0);
    auto spirvDecorations = parseSPIRVDecorationsFromMD(ptr);
    for (auto& [DecorationId, MDNodes] : spirvDecorations)
    {
        switch (DecorationId)
        {
            // IDecCacheControlLoadINTEL
            case DecorationIdCacheControlLoad:
            {
                handleCacheControlINTELFor2DBlockIO<LoadCacheControl>(I, MDNodes, unmangledName);
                break;
            }
        }
    }
}

void HandleSpirvDecorationMetadata::visit2DBlockWriteCallInst(CallInst& I, StringRef unmangledName)
{
    Value* ptr = I.getArgOperand(0);
    auto spirvDecorations = parseSPIRVDecorationsFromMD(ptr);
    for (auto& [DecorationId, MDNodes] : spirvDecorations)
    {
        switch (DecorationId)
        {
            // IDecCacheControlStoreINTEL
            case DecorationIdCacheControlStore:
            {
                handleCacheControlINTELFor2DBlockIO<StoreCacheControl>(I, MDNodes, unmangledName);
                break;
            }
        }
    }
}

void HandleSpirvDecorationMetadata::visitCallInst(CallInst& I)
{
    Function* F = I.getCalledFunction();
    if (!F) return;

    Regex pattern2DBlockRead(
        "_Z[0-9]+(intel_sub_group_2d_block_(prefetch|read|read_transform|read_transpose)_[0-9]+b_[0-9]+r[0-9]+x[0-9]+c)");
    Regex pattern2DBlockWrite(
        "_Z[0-9]+(intel_sub_group_2d_block_write_[0-9]+b_[0-9]+r[0-9]+x[0-9]+c)");

    SmallVector<StringRef, 4> Matches;
    StringRef funcName = F->getName();

    if (pattern2DBlockRead.match(funcName, &Matches))
    {
        visit2DBlockReadCallInst(I, Matches[1]);
    }
    else if (pattern2DBlockWrite.match(funcName, &Matches))
    {
        visit2DBlockWriteCallInst(I, Matches[1]);
    }
}

template<typename T>
void HandleSpirvDecorationMetadata::handleCacheControlINTEL(Instruction& I, SmallPtrSetImpl<MDNode*>& MDNodes)
{
    static_assert(std::is_same_v<T, LoadCacheControl> || std::is_same_v<T, StoreCacheControl>);
    CacheControlFromMDNodes cacheControl = resolveCacheControlFromMDNodes<T>(m_pCtx, MDNodes);
    if (cacheControl.isEmpty || cacheControl.isDefault) return;
    if (cacheControl.isInvalid)
    {
        m_pCtx->EmitWarning("Unsupported cache controls configuration requested. Applying default configuration.");
        return;
    }

    MDNode* CacheCtrlNode = MDNode::get(I.getContext(),
        ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(I.getContext()), cacheControl.value)));
    I.setMetadata("lsc.cache.ctrl", CacheCtrlNode);
    m_changed = true;
}

template<typename T>
void HandleSpirvDecorationMetadata::handleCacheControlINTELFor2DBlockIO(CallInst& I, SmallPtrSetImpl<MDNode*>& MDNodes, StringRef unmangledName)
{
    static_assert(std::is_same_v<T, LoadCacheControl> || std::is_same_v<T, StoreCacheControl>);
    CacheControlFromMDNodes cacheControl = resolveCacheControlFromMDNodes<T>(m_pCtx, MDNodes);
    if (cacheControl.isEmpty) return;
    if (cacheControl.isInvalid)
    {
        m_pCtx->EmitWarning("Unsupported cache controls configuration requested. Applying default configuration.");
        return;
    }

    Function* F = I.getCalledFunction();
    IGC_ASSERT(F);

    SmallVector<Value*, 4> args(I.args());
    args.push_back(ConstantInt::get(Type::getInt32Ty(I.getContext()), cacheControl.value));

    SmallVector<Type*, 4> argTypes;
    for (const auto& arg : args)
        argTypes.push_back(arg->getType());

    FunctionType* FT = FunctionType::get(I.getType(), argTypes, false);
    std::string newFuncName = "__internal_" + unmangledName.str() + "_cache_controls";
    auto newFunction = m_Module->getOrInsertFunction(newFuncName, FT);

    auto newCall = CallInst::Create(newFunction, args, "", &I);
    I.replaceAllUsesWith(newCall);
    I.eraseFromParent();

    // Cleanup unused function if all calls have been replaced with the internal version
    if (F->getNumUses() == 0)
        F->eraseFromParent();
}
