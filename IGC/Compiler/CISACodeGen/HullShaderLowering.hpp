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
} // namespace IGC
