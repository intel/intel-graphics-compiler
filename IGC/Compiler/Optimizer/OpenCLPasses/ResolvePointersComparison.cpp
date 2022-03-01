/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ResolvePointersComparison.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/PatternMatch.h>
#include "common/LLVMWarningsPop.hpp"


using namespace llvm;
using namespace llvm::PatternMatch;
using namespace IGC;

#define PASS_FLAG "resolve-pointers-comparison"
#define PASS_DESCRIPTION "Transform icmp(ptrtoint(ptr0),ptrtoint(ptr1)) pattern into icmp(ptr0, ptr1)"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ResolvePointersComparison, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ResolvePointersComparison, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ResolvePointersComparison::ID = 0;

ResolvePointersComparison::ResolvePointersComparison() : FunctionPass(ID)
{
    initializeResolvePointersComparisonPass(*PassRegistry::getPassRegistry());
}

bool ResolvePointersComparison::runOnFunction(Function& F)
{
    visit(F);
    return m_changed;
}

// SPIRV specification (before SPIRV 1.4) does not implement an opcode to compare
// pointers. Since SPIRV comparison opcodes operate only on integers, SPIRVWriter
// produces OpConvertPtrToU (for each cmp operand) which converts pointer to unsigned integer.
//
// Described behavior of SPIRVWriter, results in SPIRVReader producing the following LLVM IR:
// %op0 = ptrtoint i8 addrspace(3)* %ptr0 to i64
// %op1 = ptrtoint i8 addrspace(3)* %ptr1 to i64
// %0 = icmp eq i64 %op0, %op1
//
// ResolvePointersComparison pass transforms it into:
// %0 = icmp eq i8 addrspace(3)* %ptr0, %ptr1

void ResolvePointersComparison::visitICmpInst(ICmpInst& I)
{
    CmpInst::Predicate Pred;
    Value* Ptr0 = nullptr;
    Value* Ptr1 = nullptr;
    auto PtrCmpPattern = m_ICmp(Pred, m_PtrToInt(m_Value(Ptr0)), m_PtrToInt(m_Value(Ptr1)));

    if (!match(&I, PtrCmpPattern))
        return;

    if (Ptr0->getType() != Ptr1->getType())
        return;

    auto NewCmp = CmpInst::Create(Instruction::ICmp, Pred, Ptr0, Ptr1, "", &I);
    NewCmp->setDebugLoc(I.getDebugLoc());
    I.replaceAllUsesWith(NewCmp);
    I.eraseFromParent();

    m_changed = true;
}
