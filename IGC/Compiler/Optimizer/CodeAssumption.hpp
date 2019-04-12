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
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/PassSupport.h"
#include "llvm/IR/Instructions.h"
#include <llvm/IR/Function.h>
#include "llvm/IR/Module.h"
#include <llvm/Analysis/AssumptionCache.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    // forward decl
    namespace IGCMD {
        class MetaDataUtils;
    }

    //
    // CodeAssumption inserts llvm.assume to make sure some of code's
    // attributes holds. For example, OCL's get_global_id() will be
    // always positive, so we insert llvm.assume for its return value.
    // This llvm.assume will help value tracking (verifying whether an
    // value is positive or not). Currently, value tracking is used
    // by StatelessToStateful optimization.
    //
    class CodeAssumption : public llvm::ModulePass {
    public:
        static char ID;

        CodeAssumption() : ModulePass(ID), m_changed(false) {}

        bool runOnModule(llvm::Module &) override;

        void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        // APIs used directly
        static bool addAssumption(
            llvm::Function* F,
            llvm::AssumptionCache *AC);

        static bool IsSGIdUniform(IGCMD::MetaDataUtils* pMDU, ModuleMetaData *modMD, llvm::Function* F);

    private:
        bool m_changed;

        IGCMD::MetaDataUtils* m_pMDUtils;

        // Simple change to help uniform analysis (later).
        void uniformHelper(llvm::Module* M);

        // Add llvm.assume to assist other optimization such statelessToStateful
        void addAssumption(llvm::Module* M);

        // helpers
        static bool isPositiveIndVar(
            llvm::PHINode *PN,
            const llvm::DataLayout *DL,
            llvm::AssumptionCache *AC);
    };
}