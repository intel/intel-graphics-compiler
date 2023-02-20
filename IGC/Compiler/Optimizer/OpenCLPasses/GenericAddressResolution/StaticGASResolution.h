/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/CastToGASAnalysis.h"
#include "Compiler/InitializePasses.h"

using namespace llvm;
using namespace IGC;

namespace IGC {

    llvm::FunctionPass* createStaticGASResolution();

    class StaticGASResolution : public FunctionPass
    {
    public:
        static char ID;

        StaticGASResolution() : FunctionPass(ID)
        {
            initializeStaticGASResolutionPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function&) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CastToGASAnalysis>();
        }

        virtual StringRef getPassName() const override
        {
            return "StaticGASResolution";
        }
    private:
        GASInfo* m_GI = nullptr;
    };
} // End namespace IGC
