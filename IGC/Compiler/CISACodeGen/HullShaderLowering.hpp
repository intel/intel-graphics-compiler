/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include <map>

namespace IGC
{
    class HullShaderProperties
    {
    public:
        HullShaderProperties();
        unsigned int m_pOutputControlPointCount; // number of output control points
        unsigned int m_pInputControlPointCount; // number of input control points
        /// This variable holds the URB handle for the output vertices.
        unsigned int m_pMaxInputSignatureCount; // number of attributes associated with each input
        unsigned int m_pMaxOutputSignatureCount; // number of attributes associated with each output
        unsigned int m_pMaxPatchConstantSignatureDeclarations; // number of patch constant declarations

        HullShaderDispatchModes m_pShaderDispatchMode;
        bool m_HasClipCullAsInput;
        unsigned m_ForcedDispatchMask; // if this value is != 0, it is used as a dispatch mask in HS.

        unsigned int GetMaxInputPushed() const;
    };

    class CollectHullShaderProperties : public llvm::ImmutablePass
    {
    public:
        CollectHullShaderProperties();
        static char ID;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
        }

        void gatherInformation(llvm::Function* kernel);

        const HullShaderProperties& GetProperties() { return m_hsProps; }
    protected:
        HullShaderDispatchModes DetermineDispatchMode(llvm::Function* kernel) const;
        unsigned GetForcedDispatchMask(llvm::Function* kernel) const;
        HullShaderProperties m_hsProps;
    };

    llvm::FunctionPass* createHullShaderLoweringPass();
    void initializeHullShaderLoweringPass(llvm::PassRegistry&);
} // namespace IGC
