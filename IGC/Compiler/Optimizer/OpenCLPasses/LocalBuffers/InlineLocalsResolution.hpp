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

        // It is convenient to represent the null pointer as the zero
        // bit-pattern. However, SLM address 0 is legal, and we want to be able
        // to use it.
        // To go around this, we use the fact only the low 16 bits ("low nibble")
        // of SLM addresses are significant, and set all valid pointers to have
        // a non-zero high nibble.
        static const unsigned int VALID_LOCAL_HIGH_BITS;

    protected:

        void collectInfoOnSharedLocalMem(llvm::Module&);
        void computeOffsetList(llvm::Module&, std::map<llvm::Function*, unsigned int>&);
        void traveseCGN(llvm::CallGraphNode&);

    private:

        llvm::MapVector<llvm::Function*, GlobalVariableSet> m_FuncToVarsMap;
        std::map<llvm::Function*, unsigned int> m_FuncToMemPoolSizeMap;
        std::set<llvm::Function*> m_chkSet;
        llvm::GlobalVariable* m_pGV;
    };

} // namespace IGC
