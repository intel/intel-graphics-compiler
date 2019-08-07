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
#include "llvm/Config/llvm-config.h"
#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/DataLayout.h>
#include "common/LLVMWarningsPop.hpp"

#include <vector>

namespace IGC
{

    /// @brief Inserts debugging information for OpenCL debugging.
    class ImplicitGlobalId : public llvm::ModulePass
    {
        enum GlobalOrLocal
        {
            Global = 0,
            Local = 1,
            WorkItem = 2
        };

    public:
        static char ID;

        /// @brief C'tor
        ImplicitGlobalId();

        /// @brief D'tor
        ~ImplicitGlobalId() {}

        /// @brief Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "ImplicitGlobalId";
        }

        /// @brief execute pass on given module
        /// @param M module to update
        /// @returns True if module was modified
        virtual bool runOnModule(llvm::Module& M) override;

        /// @brief Inform about usage/modification/dependency of this pass
        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
        }

    private:
        /// @brief execute pass on given function
        /// @param F Function to modify
        /// @returns True if function was modified
        virtual bool runOnFunction(llvm::Function& F);

        /// @brief execute pass on given basic block
        /// @param alloca0/1/2 are allocas for each dimension
        /// @param insertBefore Instruction to insert other instructions before
        void runOnBasicBlock(llvm::AllocaInst* alloca0, llvm::AllocaInst* alloca1, llvm::AllocaInst* alloca2, llvm::Instruction* insertBefore, GlobalOrLocal wi);

        /// @brief Adds instructions to the beginning of the given function to compute the
        ///  global/local IDs for 3 dimensions. Fills in the FunctionContext.
        /// @param pFunc Function to modify
        void insertComputeIds(llvm::Function* pFunc);

        /// @brief Gets or create int/long long debug info tyoe
        /// returns DIType int/long long DIType
        llvm::DIType* getOrCreateIntDIType();

        /// @brief Iterates instructions in a basic block and tries to find the first
        ///  instruction with scope and loc information and return these.
        /// @param BB Basic block to get the scope and loc from
        /// @param scope scope information
        /// @param loc debug location
        /// @returns bool True if a scope and loc were found
        bool getBBScope(const llvm::BasicBlock& BB, llvm::DIScope*& scope, llvm::DebugLoc& loc);

        llvm::Value* CreateGetId(llvm::IRBuilder<>& B, GlobalOrLocal wi);

    private:
        /// This holds the processed module
        llvm::Module* m_pModule;

        /// This holds the debug info builder
        std::unique_ptr<llvm::DIBuilder> m_pDIB;

        /// This holds the processed module context
        llvm::LLVMContext* m_pContext;

        /// This holds debug information for a module which can be queried
        llvm::DebugInfoFinder m_DbgInfoFinder;

        unsigned int m_uiSizeT;
    };

} // namespace IGC
