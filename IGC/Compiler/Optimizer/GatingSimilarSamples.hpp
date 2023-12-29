/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/IGCPassSupport.h"
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
        llvm::BasicBlock* BB = nullptr;
        llvm::Instruction* motionSample = nullptr;
        llvm::Instruction* texelSample = nullptr;
        llvm::Instruction* resultInst = nullptr;

        //motion.xy will be the gating value
        llvm::Value* gatingValue_mul1 = nullptr; //motion.x
        llvm::Value* gatingValue_mul2 = nullptr; //motion.y
        std::vector<llvm::Instruction*> similarSampleInsts{};
        bool areSampleInstructionsSimilar(llvm::Instruction*, llvm::Instruction*);
        bool checkAndSaveSimilarSampleInsts();
        bool setOrCmpGatingValue(llvm::Value*& gatingValueToCmp1, llvm::Instruction* mulInst, const llvm::Instruction* texelSampleInst);
        bool findAndSetCommonGatingValue();
    };

    llvm::FunctionPass* CreateGatingSimilarSamples();
}
