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
#ifndef IGC_INSTCOMBINE_INSTCOMBINE_H
#define IGC_INSTCOMBINE_INSTCOMBINE_H

#include <llvm/PassRegistry.h>
#include "llvm/Transforms/InstCombine/InstCombineWorklist.h"
#include "llvm/IR/PassManager.h"
#include "Compiler/InitializePasses.h"

#if LLVM_VERSION_MAJOR <= 7
namespace llvm {
    class FunctionPass;
}
#elif LLVM_VERSION_MAJOR >= 8
#include "llvm/Transforms/InstCombine/InstCombine.h"
#endif

namespace IGC
{
#if LLVM_VERSION_MAJOR <= 7
    class IGCInstructionCombiningPass : public llvm::FunctionPass {
        llvm::InstCombineWorklist Worklist;
        const bool ExpensiveCombines;

    public:
        static char ID; // Pass identification, replacement for typeid

        IGCInstructionCombiningPass(bool ExpensiveCombines = true)
            : FunctionPass(ID), ExpensiveCombines(ExpensiveCombines) {
            initializeIGCInstructionCombiningPassPass(*llvm::PassRegistry::getPassRegistry());
        }

        void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
        bool runOnFunction(llvm::Function &F) override;
    };


    llvm::FunctionPass* createIGCInstructionCombiningPass();
#elif LLVM_VERSION_MAJOR >= 8
    inline llvm::FunctionPass* createIGCInstructionCombiningPass()
    {
        return llvm::createInstructionCombiningPass(false);
    }
#endif
} // namespace IGC

#endif //IGC_INSTCOMBINE_INSTCOMBINE_H

