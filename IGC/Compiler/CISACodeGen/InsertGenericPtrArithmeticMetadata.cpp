/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The purpose of this pass is to add metadata to skip tagging generic pointers that are used
// for pointer arithmetic. We identify dependency of addrspacecast->inttoptr to recover the
// corresponding tag after the arithmetic is done.

#include "InsertGenericPtrArithmeticMetadata.hpp"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "insert-generic-ptr-arithmetic-metadata"
#define PASS_DESCRIPTION "Add metadata to skip tagging generic pointers that are used for pointer arithmetic"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(InsertGenericPtrArithmeticMetadata, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(InsertGenericPtrArithmeticMetadata, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char InsertGenericPtrArithmeticMetadata::ID = 0;

InsertGenericPtrArithmeticMetadata::InsertGenericPtrArithmeticMetadata() : FunctionPass(ID)
{
    initializeInsertGenericPtrArithmeticMetadataPass(*PassRegistry::getPassRegistry());
}

bool InsertGenericPtrArithmeticMetadata::runOnFunction(Function& F)
{
    m_context = &F.getContext();
    visit(F);
    return m_changed;
}

void InsertGenericPtrArithmeticMetadata::visitAddrSpaceCast(AddrSpaceCastInst& I) {
    unsigned totalNumUses = 0;
    unsigned numUsesArith = 0;

    if (I.getDestAddressSpace() == ADDRESS_SPACE_GENERIC)
    {
        totalNumUses = I.getNumUses();
        for (auto ui : I.users())
        {
            Instruction* useInst = dyn_cast<Instruction>(ui);
            PHINode* phi = dyn_cast<PHINode>(useInst);
            GetElementPtrInst* gepInst = dyn_cast<GetElementPtrInst>(useInst);

            if (phi)
            {
                useInst = useInst->user_back();
                gepInst = dyn_cast<GetElementPtrInst>(useInst);
                if (gepInst)
                    useInst = useInst->user_back();
            }
            else if (gepInst)
            {
                useInst = useInst->user_back();
                phi = dyn_cast<PHINode>(useInst);
                if (phi)
                    useInst = useInst->user_back();
            }

            PtrToIntInst* ptiInst = dyn_cast<PtrToIntInst>(useInst);
            if (ptiInst && ptiInst->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC)
            {
                if (!ptiInst->use_empty())
                {
                    Instruction* ptiUser = ptiInst->user_back();
                    // We only skip tags on generic pointers if there is an arithmetic operation
                    // after the addrspacecast->ptrtoint.
                    if (ptiUser->isBinaryOp() || ptiUser->isShift() || ptiUser->isBitwiseLogicOp())
                    {
                        numUsesArith++;
                    }
                }
            }
        }

        if (numUsesArith > 0 && numUsesArith == totalNumUses)
        {
            // Add metadata to avoid tagging when emitting addrspacecast
            MDNode* N = MDNode::get(*m_context, MDString::get(*m_context, "generic.arith"));
            I.setMetadata("generic.arith", N);
            m_changed = true;
        }
    }
}
