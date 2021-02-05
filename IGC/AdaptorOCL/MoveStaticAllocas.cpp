/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "MoveStaticAllocas.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-move-static-allocas"
#define PASS_DESCRIPTION "Move static allocas to entry basic block of the function"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(MoveStaticAllocas, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(MoveStaticAllocas, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char MoveStaticAllocas::ID = 0;

// This function is copied from LLVM's InlineFunction.
static bool allocaWouldBeStaticInEntry(const AllocaInst *AI) {
    return isa<Constant>(AI->getArraySize()) && !AI->isUsedWithInAlloca();
}

MoveStaticAllocas::MoveStaticAllocas() : FunctionPass(ID)
{
    initializeMoveStaticAllocasPass(*PassRegistry::getPassRegistry());
}

bool MoveStaticAllocas::runOnFunction(llvm::Function &F)
{
    std::vector<AllocaInst*> staticAllocas;

    // Note: we are also moving static allocas that are already in the entry BB.
    for (auto inst = inst_begin(F), endInst = inst_end(F); inst != endInst; ++inst) {
        if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(&*inst)) {
            if (allocaInst->isStaticAlloca() || allocaWouldBeStaticInEntry(allocaInst)) {
                staticAllocas.push_back(allocaInst);
            }
        }
    }

    for (auto I = staticAllocas.rbegin(), E = staticAllocas.rend(); I != E; ++I) {
        (*I)->moveBefore(&*F.getEntryBlock().getFirstInsertionPt());
    }

    return staticAllocas.size() > 0;
}

