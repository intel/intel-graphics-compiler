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
#pragma once
#include "Compiler/CodeGenPublicEnums.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
static const char* g_cRate[] =
{
    "coarse_phase",
    "pixel_phase",
    "sample_phase",
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