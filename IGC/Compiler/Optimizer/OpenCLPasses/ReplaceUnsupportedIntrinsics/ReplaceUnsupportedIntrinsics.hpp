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
#include <llvm/IR/InstVisitor.h>
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
    class ReplaceUnsupportedIntrinsics : public llvm::FunctionPass, public llvm::InstVisitor<ReplaceUnsupportedIntrinsics>
    {
    public:
        static char ID;

        ReplaceUnsupportedIntrinsics();

        ~ReplaceUnsupportedIntrinsics() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "ReplaceUnsupportedIntrinsics";
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
        }

        void visitIntrinsicInst(llvm::IntrinsicInst& I);

    private:

        std::vector<llvm::IntrinsicInst*> m_instsToReplace;

    };

} // namespace IGC
