/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
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

        llvm::StringRef getPassName() const override {
            return "CodeAssumption";
        }

        bool runOnModule(llvm::Module&) override;

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        // APIs used directly
        static bool addAssumption(
            llvm::Function* F,
            llvm::AssumptionCache* AC);

        static bool IsSGIdUniform(IGCMD::MetaDataUtils* pMDU, ModuleMetaData* modMD, llvm::Function* F);

    private:
        bool m_changed{};

        IGCMD::MetaDataUtils* m_pMDUtils = nullptr;

        // Simple change to help uniform analysis (later).
        void uniformHelper(llvm::Module* M);

        // Add llvm.assume to assist other optimization such statelessToStateful
        void addAssumption(llvm::Module* M);

        // helpers
        static bool isPositiveIndVar(
            llvm::PHINode* PN,
            const llvm::DataLayout* DL,
            llvm::AssumptionCache* AC);
    };
}
