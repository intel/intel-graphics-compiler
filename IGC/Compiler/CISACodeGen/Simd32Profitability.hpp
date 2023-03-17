/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"

namespace IGC
{
    namespace IGCMD {
        class MetaDataUtils;
    }

    /// @brief  This pass implements a heuristic to determine whether SIMD32 is profitable.
    class Simd32ProfitabilityAnalysis : public llvm::FunctionPass
    {
    public:
        static char ID;

        Simd32ProfitabilityAnalysis();

        ~Simd32ProfitabilityAnalysis() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "Simd32Profitability";
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesAll();
            AU.addRequired<WIAnalysis>();
            AU.addRequired<llvm::LoopInfoWrapperPass>();
            AU.addRequired<llvm::PostDominatorTreeWrapperPass>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        bool isSimd32Profitable() const { return m_isSimd32Profitable; }
        bool isSimd16Profitable() const { return m_isSimd16Profitable; }

    private:
        llvm::Function* F;
        llvm::PostDominatorTree* PDT;
        llvm::LoopInfo* LI;
        IGCMD::MetaDataUtils* pMdUtils;
        WIAnalysis* WI;
        bool m_isSimd32Profitable;
        bool m_isSimd16Profitable;

        unsigned getLoopCyclomaticComplexity();
        bool checkSimd32Profitable(CodeGenContext*);
        bool checkSimd16Profitable(CodeGenContext*);

        unsigned estimateLoopCount(llvm::Loop* L);
        unsigned estimateLoopCount_CASE1(llvm::Loop* L);
        unsigned estimateLoopCount_CASE2(llvm::Loop* L);

        bool checkPSSimd32Profitable();

        void print(llvm::raw_ostream& OS) const;
    };

} // namespace IGC
