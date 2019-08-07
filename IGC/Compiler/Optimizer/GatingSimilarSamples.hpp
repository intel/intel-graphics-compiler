#pragma once
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


#include "../IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"


namespace IGC
{
    //===----------------------------------------------------------------------===//
    //
    // Optimization pass. 
    //
    // It detects sample instructions in a BB, that differ only by coordinates 
    // argument from a sample instruction used at the beginning of the BB.
    // In that case those similar sample instructions can be "gated": surrounded by if-then, where they 
    // are placed in the "if-then" block, and skipped otherwise.
    //
    //===----------------------------------------------------------------------===//
    class GatingSimilarSamples : public llvm::FunctionPass
    {
    public:
        static char ID;
        GatingSimilarSamples() : llvm::FunctionPass(ID)
        {
            initializeGatingSimilarSamplesPass(*llvm::PassRegistry::getPassRegistry());
        }
        virtual bool runOnFunction(llvm::Function& F) override;
        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<IGC::CodeGenContextWrapper>();
        }

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "GatingSimilarSamples";
        }

    private:
        llvm::BasicBlock* BB;
        llvm::Instruction* motionSample;
        llvm::Instruction* texelSample;
        llvm::Instruction* resultInst;

        //motion.xy will be the gating value
        llvm::Value* gatingValue_mul1; //motion.x
        llvm::Value* gatingValue_mul2; //motion.y
        std::vector<llvm::Instruction*> similarSampleInsts;
        bool areSampleInstructionsSimilar(llvm::Instruction*, llvm::Instruction*);
        bool checkAndSaveSimilarSampleInsts();
        bool setOrCmpGatingValue(llvm::Value*& gatingValueToCmp1, llvm::Instruction* mulInst, const llvm::Instruction* texelSampleInst);
        bool findAndSetCommonGatingValue();
    };

    llvm::FunctionPass* CreateGatingSimilarSamples();
}
