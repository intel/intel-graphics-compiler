#pragma once
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/WIAnalysis.hpp"

namespace IGC {

    class UniformAssumptions : public llvm::FunctionPass, public llvm::InstVisitor<UniformAssumptions>
    {

    public:
        static char ID; // Pass identification

        UniformAssumptions();

        virtual bool runOnFunction(llvm::Function& F) override;

        void visitCallInst(llvm::CallInst& CI);

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<WIAnalysis>();
            AU.addRequired<CodeGenContextWrapper>();
        }

    private:
        static const int s_cMaxRecursion = 16;

        bool IsAssumedUniform(llvm::Value* V, int recursionLevel = 0) const;
        void MakeUniformResourceOperand(llvm::Value* V, llvm::CallInst* CI);
        void HoistAssumptions(llvm::Function& F);
        void CollectAssumptions(llvm::Function& F);
        void OptimizeResourceAccesses(llvm::Function& F);

        bool m_changed = false;
        WIAnalysis* m_WIAnalysis = nullptr;
        CodeGenContext* m_pCodeGenContext = nullptr;

        bool m_collectingAssumptions = false;
        llvm::SmallVector<llvm::GenIntrinsicInst*, 8> m_assumptions;
    };

}

