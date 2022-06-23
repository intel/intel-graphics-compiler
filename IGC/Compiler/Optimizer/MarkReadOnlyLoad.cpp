/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
    bool isRO = false;

    if (LI.getPointerAddressSpace() == ADDRESS_SPACE_CONSTANT)
    {
        isRO = true;
    }
    else if (auto *srcPtr = TracePointerSource(LI.getPointerOperand()))
    {
        if (isa<GenIntrinsicInst>(srcPtr))
        {
            unsigned bufId = 0;
            BufferType bufTy;
            BufferAccessType accTy;
            bool needBufferOffset = false; // Unused

            // check whether we are doing read only access on buffer (e.g. on UAV)
            if (GetResourcePointerInfo(
                srcPtr, bufId, bufTy, accTy, needBufferOffset))
            {
                if (accTy == BufferAccessType::ACCESS_READ)
                {
                    isRO = true;
                }
            }
        }
    }

    if (isRO)
    {
        m_changed = true;
        LI.setMetadata(LLVMContext::MD_invariant_load, m_mdNode);
    }
}
