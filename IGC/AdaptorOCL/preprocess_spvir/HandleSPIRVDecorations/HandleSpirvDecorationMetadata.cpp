/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "HandleSpirvDecorationMetadata.h"
#include "CacheControlsHelper.h"
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

DenseMap<uint64_t, SmallPtrSet<MDNode*, 4>> HandleSpirvDecorationMetadata::parseSPIRVDecorationsFromMD(Value* V)
{
    MDNode* spirvDecorationsMD = nullptr;
    if (auto* GV = dyn_cast<GlobalVariable>(V))
    {
        spirvDecorationsMD = GV->getMetadata("spirv.Decorations");
    }
    else if (auto* II = dyn_cast<Instruction>(V))
    {
        spirvDecorationsMD = II->getMetadata("spirv.Decorations");
    }
    else if (auto* A = dyn_cast<Argument>(V))
    {
        Function* F = A->getParent();
        auto* parameterMD = F->getMetadata("spirv.ParameterDecorations");

        if (parameterMD)
        {
            spirvDecorationsMD = cast<MDNode>(parameterMD->getOperand(A->getArgNo()));
        }
    }

    DenseMap<uint64_t, SmallPtrSet<MDNode*, 4>> spirvDecorations;
    if (spirvDecorationsMD)
    {
        for (const auto& operand : spirvDecorationsMD->operands())
        {
            auto node = dyn_cast<MDNode>(operand.get());
            if (node->getNumOperands() == 0)
            {
                continue;
            }

            if (auto value = dyn_cast<ValueAsMetadata>(node->getOperand(0)))
            {
                if (auto constantInt = dyn_cast<ConstantInt>(value->getValue()))
                {
                    uint64_t decorationId = constantInt->getZExtValue();

                    spirvDecorations[decorationId].insert(node);
                }
            }
        }
    }
    return spirvDecorations;
}

void HandleSpirvDecorationMetadata::visitLoadInst(LoadInst& I)
{
    auto spirvDecorations = parseSPIRVDecorationsFromMD(I.getPointerOperand());
    for (auto& [DecorationId, MDNodes] : spirvDecorations)
    {
        switch (DecorationId)
        {
            // IDecCacheControlLoadINTEL
            case 6442:
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
            case 6443:
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
    SmallDenseMap<CacheLevel, T> cacheControls = parseCacheControlsMD<T>(MDNodes);
    IGC_ASSERT(!cacheControls.empty());

    // SPV_INTEL_cache_controls extension specification states the following:
    // "Cache Level is an unsigned 32-bit integer telling the cache level to
    //  which the control applies. The value 0 indicates the cache level closest
    //  to the processing unit, the value 1 indicates the next furthest cache
    //  level, etc. If some cache level does not exist, the decoration is ignored."
    //
    // Therefore Cache Level equal to 0 maps to L1$ and Cache Level equal to 1 maps to L3$.
    // Other Cache Level values are ignored.
    auto L1CacheControl = getCacheControl(cacheControls, CacheLevel(0));
    auto L3CacheControl = getCacheControl(cacheControls, CacheLevel(1));

    if (!L1CacheControl && !L3CacheControl)
    {
        // Early exit if there are no cache controls set for cache levels that are controllable
        // by Intel GPUs.
        return;
    }

    LSC_L1_L3_CC defaultLSCCacheControls = static_cast<LSC_L1_L3_CC>(
        std::is_same_v<T, LoadCacheControl> ?
            m_pCtx->getModuleMetaData()->compOpt.LoadCacheDefault :
            m_pCtx->getModuleMetaData()->compOpt.StoreCacheDefault);

    auto [L1Default, L3Default] = mapToSPIRVCacheControl<T>(defaultLSCCacheControls);

    IGC_ASSERT(L1Default != T::Invalid && L3Default != T::Invalid);

    T newL1CacheControl = L1CacheControl ? L1CacheControl.value() : L1Default;
    T newL3CacheControl = L3CacheControl ? L3CacheControl.value() : L3Default;

    LSC_L1_L3_CC newLSCCacheControl =
        mapToLSCCacheControl(newL1CacheControl, newL3CacheControl);

    if (defaultLSCCacheControls == newLSCCacheControl)
    {
        // No need to set lsc.cache.ctrl metadata if requested cache controls are the same
        // as default cache controls.
        return;
    }

    if (newLSCCacheControl != LSC_CC_INVALID)
    {
        MDNode* CacheCtrlNode = MDNode::get(
            I.getContext(),
            ConstantAsMetadata::get(
                ConstantInt::get(Type::getInt32Ty(I.getContext()), newLSCCacheControl)));
        I.setMetadata("lsc.cache.ctrl", CacheCtrlNode);
        m_changed = true;
    }
    else
    {
        m_pCtx->EmitWarning("Unsupported cache controls configuration requested. Applying default configuration.");
    }
}
