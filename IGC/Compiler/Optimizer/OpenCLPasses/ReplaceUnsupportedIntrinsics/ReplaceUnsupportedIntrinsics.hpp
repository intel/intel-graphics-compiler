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

#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace llvm
{
    class LLVMContext;
    class Instruction;
    class Value;
    class MemCpyInst;
    class MemSetInst;
    class MemMoveInst;
    class IntrinsicInst;
    class Type;
}

namespace IGC
{
    /// ReplaceIntrinsics pass lowers calls to unsupported intrinsics functions.
    // Two llvm instrinsics are replaced llvm.memcpy and llvm.memset. Both appear in SPIR spec.
    class ReplaceUnsupportedIntrinsics : public llvm::ModulePass
    {
    public:
        static char ID;

        ReplaceUnsupportedIntrinsics();

        ~ReplaceUnsupportedIntrinsics() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "ReplaceUnsupportedIntrinsics";
        }

        virtual bool runOnModule(llvm::Module &M) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
        {
        }

    private:
        void groupI8Stream(
            llvm::LLVMContext& C, uint32_t NumI8, uint32_t Align,
            uint32_t& NumI32, llvm::Type** Vecs, uint32_t& L);

        llvm::Value *replicateScalar(
            llvm::Value *ScalarVal, llvm::Type* Ty,
            llvm::Instruction *InsertBefore);

        void replaceMemcpy(llvm::MemCpyInst* MC);
        void replaceMemset(llvm::MemSetInst* MS);
        void replaceMemMove(llvm::MemMoveInst* MM);
        void replaceExpect(llvm::IntrinsicInst* EX);

        // Get the largest of power-of-2 value that is <= C
        // AND that can divide C.
        uint32_t getLargestPowerOfTwo(uint32_t C) {
            // If C == 0 (shouldn't happen), return a big one.
            return (C == 0) ? 4096 : (C & (~C + 1));
        }
    };

} // namespace IGC
