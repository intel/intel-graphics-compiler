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
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  This pass fixes the usage of GetBufferPtr, remove the combination of GetBufferPtr and GetElementPtr
    class FixResourcePtr : public llvm::FunctionPass
    {
    public:
        /// Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        FixResourcePtr();

        /// @brief  Destructor
        ~FixResourcePtr() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "FixResourcePtrPass";
        }

        /// @brief  Main entry point.
        /// @param  F The destination function.
        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
        }

    private:

        llvm::Value* ResolveBufferIndex(llvm::Value* bufferIndex, llvm::Value* vectorIndex = nullptr);

        void RemoveGetBufferPtr(llvm::GenIntrinsicInst* bufPtr, llvm::Value* bufIdx);

        void FindGetElementPtr(llvm::Instruction* bufPtr, llvm::Instruction* searchPtr);

        void FindLoadStore(llvm::Instruction* bufPtr, llvm::Instruction* eltPtr, llvm::Instruction* searchPtr);

        llvm::Value* GetByteOffset(llvm::Instruction* eltPtr);

        llvm::Value* CreateLoadIntrinsic(llvm::LoadInst* inst, llvm::Instruction* bufPtr, llvm::Value* offsetVal);

        llvm::Value* CreateStoreIntrinsic(llvm::StoreInst* inst, llvm::Instruction* bufPtr, llvm::Value* offsetVal);

        void FixAddressSpaceInAllUses(llvm::Value* ptr, uint newAS, uint oldAS);

        /// @brief  Indicates if the pass changed the processed function
        bool m_changed;
        /// Function we are processing
        llvm::Function* curFunc;
        /// Need data-layout for fixing pointer
        const llvm::DataLayout* DL;
        /// agent to modify the llvm-ir
        llvm::IRBuilder<>* builder;
        /// list of clean up after change
        std::vector<llvm::Instruction*> eraseList;
    };

} // namespace IGC