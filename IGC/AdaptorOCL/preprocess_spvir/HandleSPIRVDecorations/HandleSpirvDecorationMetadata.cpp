/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "HandleSpirvDecorationMetadata.h"
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

    if (m_updateModuleMetadata)
    {
        IGC::serialize(*m_Metadata, &module);
    }

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
    m_updateModuleMetadata = true;
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
            default: continue;
        }
    }
}

