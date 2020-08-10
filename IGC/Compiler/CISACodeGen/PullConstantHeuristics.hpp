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
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include <llvm/IR/InstVisitor.h>
#include "llvm/IR/Instruction.h"
#include <llvm/PassRegistry.h>
#include "common/LLVMWarningsPop.hpp"

#include <map>

namespace IGC
{
    class PullConstantHeuristics : public llvm::ModulePass
    {
    public:
        PullConstantHeuristics() : llvm::ModulePass(ID)
        {
            initializePullConstantHeuristicsPass(*llvm::PassRegistry::getPassRegistry());
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<IGC::CodeGenContextWrapper>();
            AU.setPreservesAll();
        }

        bool runOnModule(llvm::Module& M) override;
        unsigned getPSDBottleNeckThreshold(const llvm::Function& F);

        unsigned getPushConstantThreshold(llvm::Function* F)
        {
            if (thresholdMap.find(F) == thresholdMap.end())
            {//pass must have been disabled so Map is not populated. Return default 31.
                const unsigned pushConstantGRFThreshold = IGC_GET_FLAG_VALUE(BlockPushConstantGRFThreshold);
                return pushConstantGRFThreshold;
            }
            else
            {
                return thresholdMap[F];
            }

        }
        static char ID;
    private:
        std::map<llvm::Function*, unsigned> thresholdMap;
    };

}
