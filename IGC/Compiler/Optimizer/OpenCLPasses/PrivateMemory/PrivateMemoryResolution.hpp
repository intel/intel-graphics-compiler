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

#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class ModuleAllocaInfo;

    /// @brief  PrivateMemoryResolution pass used for resolving private memory alloca instructions.
    ///         This is done by resolving the alloca instructions.
    ///         This pass depends on the PrivateMemoryUsageAnalysis and
    ///         AddImplicitArgs passes running before it.
    /// @Author Marina Yatsina
    class PrivateMemoryResolution : public llvm::ModulePass
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        PrivateMemoryResolution();

        /// @brief  Destructor
        ~PrivateMemoryResolution() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "PrivateMemoryResolution";
        }

        /// @brief  Adds the analysis required by this pass
        virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

        /// @brief  Finds all alloca instructions, replaces them with by an llvm sequences.
        ///         and creates for each function a metadata that represents the total
        ///         amount of private memory needed by each work item.
        /// @param  M The Module to process.
        bool runOnModule(llvm::Module &M) override;

        /// @brief  Resolve collected alloca instructions.
        /// @return true if there were resolved alloca, false otherwise.
        bool resolveAllocaInstuctions(bool stackCall);

        /// Initialize setup like UseScratchSpacePrivateMemory.
        bool safeToUseScratchSpace(llvm::Module &M) const;

    private:
        /// @brief  The module level alloca information
        ModuleAllocaInfo *m_ModAllocaInfo;

        /// @brief - Metadata API 
        IGCMD::MetaDataUtils *m_pMdUtils;

        /// @brief - Current processed function
        llvm::Function *m_currFunction;
    };

} // namespace IGC
