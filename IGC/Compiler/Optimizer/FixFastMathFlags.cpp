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


//===----------------------------------------------------------------------===//
//
// The purpose of this pass is to fill in missing fast math flags for APIs
// that are using old LLVM IR and are lacking AllowContract and ApproxFunc 
// flags.
// Policy is to set those flags when AllowReassoc (former AllowUnsafeAlgebra)
// is enabled on opcode.
// 
//===----------------------------------------------------------------------===//


#include "FixFastMathFlags.hpp"
#include "IGCIRBuilder.h"
#include <llvm/IR/Function.h>

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/helper.h"

#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"


using namespace llvm;
using namespace IGC;

#define PASS_FLAG "fix-fast-math-flags"
#define PASS_DESCRIPTION "Fix FMF for APIs that are using legacy IR"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(FixFastMathFlags, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(FixFastMathFlags, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char FixFastMathFlags::ID = 0;

FixFastMathFlags::FixFastMathFlags() : FunctionPass(ID)
{
    initializeFixFastMathFlagsPass(*PassRegistry::getPassRegistry());
}

bool FixFastMathFlags::runOnFunction(Function &F)
{
    for (auto& bb : F)
    {
        for (auto& inst : bb)
        {
            if (isa<FPMathOperator>(inst))
            {
                if (inst.getFastMathFlags().allowReassoc())
                {
                    m_changed = true;
                    FastMathFlags flags = inst.getFastMathFlags();
                    flags.setAllowContract(true);
                    flags.setApproxFunc();
                    inst.setFastMathFlags(flags);
                }
            }
        }
    }
    visit(F);
    return m_changed;
}

void FixFastMathFlags::visitFCmpInst(FCmpInst &FC)
{
    if (isNaNCheck(FC)) {
        FastMathFlags FMF;
        FMF.clear();
        FC.copyFastMathFlags(FMF);
    }
}
