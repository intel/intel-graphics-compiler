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

// vim:ts=2:sw=2:et:

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/CallSite.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// OpenCL C specification states that a kernel function is just a regular
// function call if a __kernel function is called by another kernel function.
// But, it doesn't clarify what's the behaviro exactly. Under certain
// conditions, such a call may be ambuious without separating kernel function
// and user functions, e.g.
//
// __kernel __atribute__((reqd_work_group_size(1, 1, 1)))
// void bar(...) {
//   ...
// }
//  
// __kernel __attribute__((reqd_work_group_size(32, 1, 1)))
// void foo(...) {
//   ...
//   bar(...);
//   ...
// }
//
// Such ambiguity also exists if function call is enabled as well as if there
// are optimizations which may treat kernel function differently from user
// functions, such as inline buffer resolution, which will resolve the first
// local pointer argument at compilation time. If a function is used as both a
// kernel function and a user function. The first local pointer argument is
// ambugious for optimization to resolve it statically.
//
// OpenCL C++ specification already clarifies the issue and would not allow a
// kernel function to called from another kernel function. However, we still
// need to handle that for OCL 1.2 and OCL 2.0.
//
// This pass is added to clone a kernel function to a user function if it's
// called.
//

namespace {
    class KernelFunctionCloning : public ModulePass {
    public:
        static char ID;

        KernelFunctionCloning() : ModulePass(ID) {}

        bool runOnModule(Module&) override;

    private:
        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }
    };

} // End anonymous namespace

namespace IGC {

    ModulePass* createKernelFunctionCloningPass() {
        return new KernelFunctionCloning();
    }

#define PASS_FLAG "igc-kernel-function-cloning"
#define PASS_DESC "Clone kernel functions if it's called."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
    IGC_INITIALIZE_PASS_BEGIN(KernelFunctionCloning, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
        IGC_INITIALIZE_PASS_END(KernelFunctionCloning, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

} // End IGC namespace

char KernelFunctionCloning::ID = 0;

bool KernelFunctionCloning::runOnModule(Module& M) {
    MetaDataUtils* MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    // Collect kernel functions being called.
    SmallVector<Function*, 8> KernelsToClone;
    for (auto& F : M) {
        auto FII = MDU->findFunctionsInfoItem(&F);
        if (FII == MDU->end_FunctionsInfo())
            continue;
        // Check this kernell function is called.
        for (auto* U : F.users()) {
            ImmutableCallSite CS(U);
            if (!CS)
                continue;
            KernelsToClone.push_back(&F);
            break;
        }
    }

    // Clone it
    bool Changed = false;
    for (auto* F : KernelsToClone) {
        ValueToValueMapTy VMap;
        auto* NewF = CloneFunction(F, VMap);
        NewF->setLinkage(GlobalValue::InternalLinkage);
        if (!F->getParent()->getFunction(NewF->getName()))
            F->getParent()->getFunctionList().push_back(NewF);
        for (auto* U : F->users()) {
            CallSite CS(U);
            if (!CS)
                continue;
            CS.setCalledFunction(NewF);
        }
        Changed = true;
    }

    return Changed;
}
