/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "HandleSpirvDecorationMetadata.h"
#include "Compiler/IGCPassSupport.h"

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
IGC_INITIALIZE_PASS_END(HandleSpirvDecorationMetadata, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char HandleSpirvDecorationMetadata::ID = 0;

HandleSpirvDecorationMetadata::HandleSpirvDecorationMetadata() : ModulePass(ID)
{
    initializeHandleSpirvDecorationMetadataPass(*PassRegistry::getPassRegistry());
}

bool HandleSpirvDecorationMetadata::runOnModule(Module& module)
{
    bool modified = false;
    auto moduleMetadata = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

    for (auto& globalVariable : module.globals())
    {
        MDNode* spirvDecorations = globalVariable.getMetadata("spirv.Decorations");
        if (!spirvDecorations)
        {
            continue;
        }

        for (const auto& operand : spirvDecorations->operands())
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
                    modified |= handleDecoration(globalVariable, constantInt->getZExtValue(), node, moduleMetadata);
                }
            }
        }
    }

    if (updateModuleMetadata)
    {
        IGC::serialize(*moduleMetadata, &module);
    }

    return modified;
}

bool HandleSpirvDecorationMetadata::handleDecoration(GlobalVariable& globalVariable, uint64_t decorationId, MDNode* node, ModuleMetaData* moduleMetadata)
{
    // TODO: Use spirv/unified1/spirv.hpp if possible.
    switch (decorationId)
    {
        // IDecHostAccessINTEL
        case 6147: return handleHostAccessIntel(globalVariable, node, moduleMetadata);
        // Unknown decoration
        default: return false;
    }
}

bool HandleSpirvDecorationMetadata::handleHostAccessIntel(GlobalVariable& globalVariable, MDNode* node, ModuleMetaData* moduleMetadata)
{
    globalVariable.addAttribute("host_var_name", dyn_cast<MDString>(node->getOperand(2))->getString());

    moduleMetadata->capabilities.globalVariableDecorationsINTEL = true;
    updateModuleMetadata = true;

    return true;
}
