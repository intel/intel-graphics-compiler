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

template<typename T>
void HandleSpirvDecorationMetadata::handleCacheControlINTEL(Instruction& I, SmallPtrSetImpl<MDNode*>& MDNodes)
{
    static_assert(std::is_same_v<T, LoadCacheControl> || std::is_same_v<T, StoreCacheControl>);
    CacheControlFromMDNodes cacheControl = resolveCacheControlFromMDNodes<T>(m_pCtx, MDNodes);
    if (cacheControl.isEmpty) return;
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
