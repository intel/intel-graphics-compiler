/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The purpose of this pass is to add metadata to skip tagging generic pointers that are used
// for pointer arithmetic. We identify dependency of addrspacecast->inttoptr to recover the
// corresponding tag after the arithmetic is done.

#include "InsertGenericPtrArithmeticMetadata.hpp"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

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

bool InsertGenericPtrArithmeticMetadata::hasOnlyArithmeticUses(Instruction* I) {
    if (I->use_empty())
        return true;

    bool onlyArithmetic = true;
    for (auto* user : I->users())
    {
        // early return if we already detected non-arithmetic operations
        if (!onlyArithmetic) return false;

        if (isa<GetElementPtrInst>(user))
        {
            onlyArithmetic = hasOnlyArithmeticUses(cast<Instruction>(user));
        }
        else if (auto* phi = dyn_cast<PHINode>(user))
        {
            if (m_visitedPHIs.find(phi) != m_visitedPHIs.end())
            {
                continue;
            }
            m_visitedPHIs.insert(phi);
            onlyArithmetic = hasOnlyArithmeticUses(cast<Instruction>(user));
        }
        else if (auto* pti = dyn_cast<PtrToIntInst>(user))
        {
            IGC_ASSERT(pti->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC);
            if (pti->use_empty()) continue;

            for (auto* ptiUser : pti->users())
            {
                if (auto* inst = dyn_cast<Instruction>(ptiUser))
                {
                    onlyArithmetic = (inst->isBinaryOp() || inst->isShift() || inst->isBitwiseLogicOp());
                }
            }
        }
        else
        {
            // Since we don't recognize the user, lets safely assume that it's not an arithmetic operation
            onlyArithmetic = false;
        }
    }

    return onlyArithmetic;
}

void InsertGenericPtrArithmeticMetadata::visitAddrSpaceCast(AddrSpaceCastInst& I) {
    if (I.getDestAddressSpace() == ADDRESS_SPACE_GENERIC)
    {
        if (hasOnlyArithmeticUses(&I))
        {
            // Add metadata to avoid tagging when emitting addrspacecast
            MDNode* N = MDNode::get(*m_context, MDString::get(*m_context, "generic.arith"));
            I.setMetadata("generic.arith", N);
            m_changed = true;
        }
        m_visitedPHIs.clear();
    }
}
