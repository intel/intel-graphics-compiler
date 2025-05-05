/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
#include "common/Stats.hpp"
#include "common/LLVMUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/CFG.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/AddressArithmeticSinking.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"
#include <queue>

using namespace llvm;
using namespace IGC::Debug;

namespace IGC {

#define PASS_FLAG "igc-address-arith-sinking"
#define PASS_DESCRIPTION "IGC Address Arithmetic Sinking"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(AddressArithmeticSinking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(AddressArithmeticSinking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

// This pass aimed to reduce register pressure by moving address arithmetic closer to load/store

AddressArithmeticSinking::AddressArithmeticSinking(unsigned SinkingDepth) :
FunctionPass(ID), m_SinkingDepth(SinkingDepth)
{
    initializeAddressArithmeticSinkingPass(*PassRegistry::getPassRegistry());
}

bool AddressArithmeticSinking::runOnFunction(Function& F)
{
    auto ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (IGC_IS_FLAG_DISABLED(ForceAddressArithSinking) &&
        isOptDisabledForFunction(ctx->getModuleMetaData(), getPassName(), &F))
        return false;

    DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

    bool changed = false;
    for (po_iterator<DomTreeNode*> domIter = po_begin(DT->getRootNode()),
            domEnd = po_end(DT->getRootNode()); domIter != domEnd; ++domIter) {

        BasicBlock* BB = domIter->getBlock();
        changed |= ProcessBB(BB);
    }
    return changed;
}

static bool isAddressArithmetic(Instruction* I)
{
    if (isa<GetElementPtrInst>(I) ||
        isa<ExtractElementInst>(I) ||
        isa<InsertElementInst>(I) ||
        isa<InsertValueInst>(I) ||
        (isa<UnaryInstruction>(I) && !isa<LoadInst>(I)) ||
        isa<BinaryOperator>(I))
        return true;

    return false;
}

static void findArithmeticForPtr(Instruction* I, unsigned depth, SmallPtrSetImpl<Instruction*>& AddressArithmetic)
{
    std::queue<Instruction*> IQ;
    IQ.push(I);

    while (depth && !IQ.empty()) {
        Instruction* CurI = IQ.front();
        IQ.pop();
        for (Use &U : CurI->operands()) {
            auto UseI = dyn_cast<Instruction>(U.get());
            if(UseI == nullptr)
                continue;

            if (!isAddressArithmetic(UseI))
                continue;

            if (UseI->getParent() != I->getParent())
                continue;

            AddressArithmetic.insert(UseI);
            IQ.push(UseI);
        }
        depth--;
    }
}

bool AddressArithmeticSinking::ProcessBB(BasicBlock* BB)
{
    bool changed = false;

    SmallPtrSet<Instruction*, 16> AddressArithmetic;
    for (auto II = BB->rbegin(), IE = BB->rend(); II != IE; ) {
        Instruction* I = &*II++;

        if (isa<IntToPtrInst>(I)) {
            findArithmeticForPtr(I, m_SinkingDepth, AddressArithmetic);
            changed |= sink(I);
            continue;
        }

        if (AddressArithmetic.find(I) != AddressArithmetic.end())
            changed |= sink(I);

    }

    return changed;
}

BasicBlock* AddressArithmeticSinking::FindSinkTarget(Instruction* I)
{
    BasicBlock* tgtBB = nullptr;
    for (Value::user_iterator UI = I->user_begin(), UE = I->user_end(); UI != UE; ++UI) {
        Instruction* useInst = cast<Instruction>(*UI);
        BasicBlock* useBlock = useInst->getParent();
        if (PHINode * PN = dyn_cast<PHINode>(useInst)) {
            Use& U = UI.getUse();
            unsigned num = PHINode::getIncomingValueNumForOperand(U.getOperandNo());
            useBlock = PN->getIncomingBlock(num);
        }
        else {
            if (useBlock == I->getParent())
                return nullptr;
        }

        if (tgtBB != nullptr) {
            tgtBB = DT->findNearestCommonDominator(tgtBB, useBlock);
            if (tgtBB == nullptr)
                return nullptr;
        }
        else {
            tgtBB = useBlock;
        }
    }
    return tgtBB;
}

static Instruction* findFirstUse(BasicBlock* BB, const SmallPtrSetImpl<Instruction*>& usesInBB)
{
    for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II) {
        Instruction* Use = &(*II);
        if (usesInBB.count(Use))
            return Use;
    }
    return nullptr;
}

bool AddressArithmeticSinking::sink(Instruction* I)
{

    BasicBlock* tgtBB = FindSinkTarget(I);
    if (tgtBB == nullptr)
        return false;

    SmallPtrSet<Instruction*, 8> usesInBB;
    for (Value::user_iterator UI = I->user_begin(), UE = I->user_end(); UI != UE; ++UI) {
        Instruction* useInst = cast<Instruction>(*UI);
        BasicBlock* useBlock = useInst->getParent();
        if (useBlock == tgtBB)
            usesInBB.insert(useInst);
    }

    if (usesInBB.empty())
        I->moveBefore(tgtBB->getTerminator());
    else if (usesInBB.size() == 1) {
        Instruction* use = *(usesInBB.begin());
        if (!isa<PHINode>(use))
            I->moveBefore(use);
    }
    else {
        Instruction* firstUse = findFirstUse(tgtBB, usesInBB);
        IGC_ASSERT(firstUse);
        if (!isa<PHINode>(firstUse))
            I->moveBefore(firstUse);
    }

    return true;
}

char AddressArithmeticSinking::ID = 0;

} // namespace IGC
