/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"

#include <unordered_map>

namespace IGC
{
    enum Accuracy
    {
        HIGH_ACCURACY,
        LOW_ACCURACY,
        ENHANCED_PRECISION
    };

    class AccuracyDecoratedCallsBiFResolution : public llvm::ModulePass, public llvm::InstVisitor<AccuracyDecoratedCallsBiFResolution>
    {
    public:
        // Accuracy --> Builtin (3 entries in each AccurateBuiltins)
        typedef std::unordered_map<Accuracy, std::string> AccurateBuiltins;

        static char ID;

        AccuracyDecoratedCallsBiFResolution();
        ~AccuracyDecoratedCallsBiFResolution() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "AccuracyDecoratedCallsBiFResolution";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
        }

        void initNameToBuiltinMap();

        void visitCallInst(llvm::CallInst& callInst);
        void visitBinaryOperator(llvm::BinaryOperator& inst);

        virtual bool runOnModule(llvm::Module& M) override;

    private:
        bool m_changed = false;
        llvm::Module* m_Module = nullptr;
        // m_nameToBuiltin["_Z15__spirv_ocl_sinf"][ENHANCED_PRECISION] --> "__ocl_svml_sinf_ep"
        std::unordered_map<std::string, AccurateBuiltins> m_nameToBuiltin{};

        llvm::Function* getOrInsertNewFunc(
            const llvm::StringRef oldFuncName,
            llvm::Type* funcType,
            const llvm::ArrayRef<llvm::Value*> args,
            const llvm::AttributeList attributes,
            llvm::CallingConv::ID callingConv,
            const llvm::StringRef maxErrorStr,
            const llvm::Instruction* currInst
        );
        std::string getFunctionName(const llvm::StringRef oldFuncName, Accuracy accuracy, const llvm::Instruction* currInst) const;
        Accuracy getAccuracy(double maxError, double cutOff, const llvm::Instruction* currInst) const;
    };
}
