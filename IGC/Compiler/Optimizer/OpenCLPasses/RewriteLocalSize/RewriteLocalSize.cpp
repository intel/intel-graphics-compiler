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
//
// As per the OCL 2.0 spec for get_enqueued_local_size:
//
// "Returns the same value as that returned by get_local_size(dimindx) if
//  the kernel is executed
//  with a uniform work-group size."
//
// This pass is only invoked when -cl-uniform-work-group-size is present.
// In that case, get_local_size(x) == get_enqueued_local_size(x).
//
// So we will rewrite all of the get_local_size(x) so that we only have
// get_enqueued_local_size(x).  Those calls may further be folded for kernels
// that utilize __attribute__((reqd_work_group_size(X,Y,Z))).
//

#include "Compiler/Optimizer/OpenCLPasses/RewriteLocalSize/RewriteLocalSize.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncsAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvmWrapper/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-rewrite-local-size"
#define PASS_DESCRIPTION "converts get_local_size() to get_enqueued_local_size()"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RewriteLocalSize, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(RewriteLocalSize, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char RewriteLocalSize::ID = 0;

RewriteLocalSize::RewriteLocalSize() : ModulePass(ID)
{
    initializeRewriteLocalSizePass(*PassRegistry::getPassRegistry());
}

bool RewriteLocalSize::runOnModule(Module& M)
{
    Function* LS = M.getFunction(WIFuncsAnalysis::GET_LOCAL_SIZE);
    if (!LS)
        return false;

    Function* ELS = M.getFunction(WIFuncsAnalysis::GET_ENQUEUED_LOCAL_SIZE);
    if (!ELS)
        LS->setName(WIFuncsAnalysis::GET_ENQUEUED_LOCAL_SIZE);
    else
        LS->replaceAllUsesWith(ELS);

    return true;
}
