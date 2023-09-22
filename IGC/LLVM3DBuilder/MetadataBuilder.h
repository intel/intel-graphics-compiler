/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublicEnums.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
static const char* const NAMED_METADATA_COARSE_PHASE = "coarse_phase";
static const char* const NAMED_METADATA_PIXEL_PHASE = "pixel_phase";
static const char* const NAMED_METADATA_SAMPLE_PHASE = "sample_phase";

static const char* g_cRate[] =
{
    NAMED_METADATA_COARSE_PHASE,
    NAMED_METADATA_PIXEL_PHASE,
    NAMED_METADATA_SAMPLE_PHASE,
    "legacy",
};

class MetadataBuilder
{
public:
    MetadataBuilder(llvm::Module* module)
    {
        m_mod = module;
    }

    void SetShadingRate(llvm::Function* F, IGC::PixelShaderPhaseType rate)
    {
        // TODO: move to the Metadata API
        llvm::SmallVector<llvm::Metadata*, 2> metadata;
        llvm::NamedMDNode* shadingRate = F->getParent()->getOrInsertNamedMetadata(g_cRate[rate]);
        metadata.push_back(llvm::ValueAsMetadata::get(F));
        llvm::MDNode* phaseMetadata = llvm::MDNode::get(F->getContext(), metadata);
        shadingRate->addOperand(phaseMetadata);
    }

    void UpdateShadingRate(llvm::Function* oldF, llvm::Function* newF)
    {
        llvm::Module* module = oldF->getParent();

        for (int i = IGC::PSPHASE_COARSE; i < IGC::PSPHASE_LEGACY; i++)
        {
            llvm::NamedMDNode* mdNode;
            mdNode = module->getNamedMetadata(g_cRate[i]);
            if (mdNode != nullptr)
            {
                llvm::Function* mdF = llvm::mdconst::dyn_extract<llvm::Function>(
                    mdNode->getOperand(0)->getOperand(0));
                if (mdF == oldF)
                {
                    module->eraseNamedMetadata(mdNode);
                    SetShadingRate(newF, (PixelShaderPhaseType)i);
                }
            }
        }
    }

private:
    llvm::Module* m_mod;
};
}
