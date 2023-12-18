/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Analysis/CallGraph.h>
#include "common/LLVMWarningsPop.hpp"

#include <map>
#include <set>
#include <unordered_set>

namespace IGC
{
    typedef llvm::SetVector<llvm::GlobalVariable*> GlobalVariableSet;

    /// @brief  This pass resolves references to inline local address space variables
    class InlineLocalsResolution : public llvm::ModulePass
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        InlineLocalsResolution();

        /// @brief  Destructor
        ~InlineLocalsResolution() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "InlineLocalsResolutionPass";
        }

        /// @brief  Main entry point.
        /// @param  M The destination module.
        virtual bool runOnModule(llvm::Module& M) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<llvm::CallGraphWrapperPass>();
        }

    protected:

        void filterGlobals(llvm::Module&);
        bool unusedGlobal(llvm::Value* V, std::unordered_set<llvm::Value*>& unusedNodes);
        void collectInfoOnSharedLocalMem(llvm::Module&);
        void computeOffsetList(llvm::Module&, llvm::MapVector<llvm::Function*, unsigned int>&);
        void traverseCGN(const llvm::CallGraphNode&);

    private:

        llvm::MapVector<llvm::Function*, GlobalVariableSet> m_FuncToVarsMap;
        llvm::MapVector<llvm::Function*, unsigned int> m_FuncToMemPoolSizeMap;
        std::set<llvm::Function*> m_chkSet;
        llvm::GlobalVariable* m_pGV;
    };

} // namespace IGC
