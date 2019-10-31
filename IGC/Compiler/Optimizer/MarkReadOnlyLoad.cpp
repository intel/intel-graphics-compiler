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

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

namespace {

    class MarkReadOnlyLoad : public FunctionPass,
        public InstVisitor<MarkReadOnlyLoad>
    {
    public:
        static char ID;

        MarkReadOnlyLoad();
        ~MarkReadOnlyLoad() {}

        StringRef getPassName() const override
        {
            return "MarkReadOnlyLoadPass";
        }

        void getAnalysisUsage(AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.setPreservesCFG();
        }

        bool runOnFunction(Function& F) override;

        void visitLoadInst(LoadInst& LI);

    private:
        MDNode* m_mdNode;
        bool m_changed;
    };

} // namespace

namespace IGC {
    FunctionPass* createMarkReadOnlyLoadPass()
    {
        return new MarkReadOnlyLoad();
    }

} // namespace IGC

#define PASS_FLAG "mark-readonly-load"
#define PASS_DESCRIPTION "Mark readonly load"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(MarkReadOnlyLoad,
    PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
    IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
    IGC_INITIALIZE_PASS_END(MarkReadOnlyLoad,
        PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

    char MarkReadOnlyLoad::ID = 0;

MarkReadOnlyLoad::MarkReadOnlyLoad()
    : FunctionPass(ID)
{
    initializeMarkReadOnlyLoadPass(*PassRegistry::getPassRegistry());
}

bool MarkReadOnlyLoad::runOnFunction(Function& F)
{
    m_changed = false;
    m_mdNode = MDNode::get(F.getContext(), nullptr);

    visit(F);
    return m_changed;
}

void MarkReadOnlyLoad::visitLoadInst(LoadInst& LI)
{
    Value* ldPtr = LI.getPointerOperand();
    bool isRO = false;

    Value* srcPtr = TracePointerSource(ldPtr);
    if (srcPtr && isa<GenIntrinsicInst>(srcPtr))
    {
        unsigned bufId;
        BufferType bufTy;
        BufferAccessType accTy;
        bool needBufferOffset; // Unused

        // check whether we are doing read only access on buffer (e.g. on UAV)
        if (GetResourcePointerInfo(srcPtr, bufId, bufTy, accTy, needBufferOffset))
        {
            if (accTy == BufferAccessType::ACCESS_READ)
            {
                isRO = true;
            }
        }
    }

    if (isRO)
    {
        LI.setMetadata(LLVMContext::MD_invariant_load, m_mdNode);
    }
}