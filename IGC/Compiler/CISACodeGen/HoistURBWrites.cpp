/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/BreadthFirstIterator.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/Analysis/CFG.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Verifier.h>
#include "common/LLVMWarningsPop.hpp"

#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
#include "common/Stats.hpp"
#include "common/LLVMUtils.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/HoistURBWrites.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

using namespace llvm;
namespace IGC
{
char HoistURBWrites::ID = 0;
#define PASS_FLAG "igc-hoist-urb-writes"
#define PASS_DESCRIPTION "Hoists URBWrite instructions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(HoistURBWrites, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(HoistURBWrites, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

// This pass hoists URBWrite instructions either earlier in instructions'
// current basic blocks or to a predecessor basic block.
// The reason for hoisting is that, in many cases, all outputs are written only
// at the end of the program, this causes the live ranges for the registers
// holding the output data to be long. Since live ranges are long, shaders can
// spill due to registers being used up to hold the output data. By hoisting
// URBWrite instructions spilling can be avoided.
//
// This pass does not hoist URBWrite instructions out of loops or selections
// and does not sink URBWrite instructions into loops or selections. The only
// exception is hoisting URB Writes out of `do {} while()` loops.
//
// The pass gathers all URB accesses in a function. Then the pass process all
// URBWrite instructions with non-constant operands (llvm::Instruction* or
// llvm::Argument*) using the following approach:
// 1. Check if URB Write has any non-constant arguments.
// 2. Iterate through all argument definitions and find the first insert point
//    that is dominated by all argument definitions.
// 3. For all aliased URB accesses from which the URBWrite is reachable
//    without passing through the insert point found in (2) find the nearest
//    common post-dominator that dominates the URBWrite and is post-dominated
//    by the URBWrite.
HoistURBWrites::HoistURBWrites() : llvm::FunctionPass(ID)
{
    initializeHoistURBWritesPass(*PassRegistry::getPassRegistry());
}

void HoistURBWrites::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.setPreservesCFG();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
    AU.addRequired<llvm::PostDominatorTreeWrapperPass>();
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.addPreserved<llvm::DominatorTreeWrapperPass>();
    AU.addPreserved<llvm::PostDominatorTreeWrapperPass>();
    AU.addPreserved<llvm::LoopInfoWrapperPass>();
}

bool HoistURBWrites::runOnFunction(Function& F)
{
    m_URBAccesses.clear();
    GatherURBAccesses(F);
    bool modified = Hoist();
    return modified;
}

// Check if an URBWrite instruction is potentially aliased by the other URB
// access.
// Note: this function does not check URB channels accessed, it assumes
// the full access mask.
static inline bool IsAliased(
    GenIntrinsicInst* urbWrite,
    GenIntrinsicInst* urbAccess)
{
    if (urbAccess->isGenIntrinsic(GenISAIntrinsic::GenISA_urbfence))
    {
        return true;
    }
    if (urbAccess->isGenIntrinsic(GenISAIntrinsic::GenISA_threadgroupbarrier))
    {
        return true;
    }

    std::pair<Value*, uint> baseAndOffset0 = GetURBBaseAndOffset(
        urbWrite->getOperand(0));
    bool isRead = urbAccess->isGenIntrinsic(GenISAIntrinsic::GenISA_URBRead);
    std::pair<Value*, uint> baseAndOffset1 = GetURBBaseAndOffset(
        urbAccess->getOperand(isRead ? 1 : 0));
    Value* const offset0 = std::get<0>(baseAndOffset0);
    const uint constOffset0 = std::get<1>(baseAndOffset0);
    Value* const offset1 = std::get<0>(baseAndOffset1);
    const uint constOffset1 = std::get<1>(baseAndOffset1);
    if ((offset0 == offset1 || offset0 == nullptr) &&
        (constOffset0 < constOffset1 && constOffset0 + 2 <= constOffset1))
    {
        return false;
    }
    if ((offset0 == offset1 || offset1 == nullptr) &&
        (constOffset1 < constOffset0 && constOffset1 + 2 <= constOffset0))
    {
        return false;
    }
    return true;
}

// Determine whether it is safe to hoist an URBWrite earlier in its basic block
// or into a predecessor basic block. If URBWrite can be hoisted the function
// returns instruction that the URBWrite can be hoisted before.
Instruction*
HoistURBWrites::IsSafeToHoistURBWriteInstruction(
    GenIntrinsicInst* urbWrite)
{
    DominatorTree* DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    PostDominatorTree* PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
    LoopInfo* LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

    // Lambda returns the immediate dominator basic block.
    auto GetIDom = [DT](
        BasicBlock* bb)->BasicBlock *
    {
        IGC_ASSERT(DT->getNode(bb) != nullptr);
        DomTreeNode* node = DT->getNode(bb)->getIDom();
        return node ? node->getBlock() : nullptr;
    };

    // `dominatedByAllArg` will contain the first insert point dominated by all
    // argument definitions.
    Instruction* dominatedByAllArgs = nullptr;
    for (const Use& arg : urbWrite->args())
    {
        BasicBlock* entryBB = &(urbWrite->getParent()->getParent()->getEntryBlock());
        Instruction* entryInst = &(*entryBB->getFirstInsertionPt());
        if (isa<Instruction>(&arg) || isa<Argument>(&arg))
        {
            Instruction* inst = isa<Argument>(&arg) ?
                entryInst : cast<Instruction>(&arg)->getNextNode();
            if (dominatedByAllArgs == nullptr ||
                DT->dominates(dominatedByAllArgs, inst))
            {
                dominatedByAllArgs = inst;
            }
        }
    }
    if (dominatedByAllArgs == nullptr)
    {
        // All arguments are constants or undefs.
        return nullptr;
    }
    // Find the first basic block that is dominated by all argument definitions
    // and that dominates the URBWrite and is post-dominated by the URBWrite.
    BasicBlock* uwBB = urbWrite->getParent();
    BasicBlock* iDomBB = uwBB;
    while (GetIDom(iDomBB) &&
        PDT->dominates(uwBB, GetIDom(iDomBB)) &&
        DT->dominates(dominatedByAllArgs->getParent(), GetIDom(iDomBB)))
    {
        iDomBB = GetIDom(iDomBB);
    }
    // `insertBefore` will contain the location to move the URBWrite instruction
    // before
    Instruction* insertBefore = iDomBB == dominatedByAllArgs->getParent() ?
        dominatedByAllArgs : &(*iDomBB->getFirstInsertionPt());

    // At this point `insertBefore` is the earliest (in program-order) valid
    // insert point that is dominated by all argument definitions and is
    // post-dominated by the URBWrite.
    // Check for potentially aliased URB access "between" the earliest
    // insert point and the URBWrite. For each such URB access find the
    // first post-dominating block that dominates the URBWrite and is
    // post-dominated by the URBWrite.
    SmallPtrSet<BasicBlock*, 8> argDefExclusionSet{ insertBefore->getParent() };
    for (GenIntrinsicInst* urbAccess : m_URBAccesses)
    {
        BasicBlock* otherBB = urbAccess->getParent();
        if (urbAccess == urbWrite ||
            (insertBefore->getParent() == uwBB && otherBB != uwBB) ||
            !IsAliased(urbWrite, urbAccess))
        {
            continue;
        }
        if (!isPotentiallyReachable(urbAccess, urbWrite, &argDefExclusionSet, DT, LI) &&
            otherBB != insertBefore->getParent()) // argDefExclusionSet contains current insert block
        {
            // Ignore any aliased URB access if the URBWrite is not reachable
            // from it.
            continue;
        }
        if (otherBB != uwBB)
        {
            otherBB = PDT->findNearestCommonDominator(
                otherBB,
                insertBefore->getParent());
        }
        if (otherBB && PDT->dominates(uwBB, otherBB))
        {
            Instruction* newInsertBefore = otherBB == urbAccess->getParent() ?
                urbAccess->getNextNode() :
                &(*otherBB->getFirstInsertionPt());
            if (newInsertBefore == urbWrite)
            {
                // URBWrite is the first instruction in the basic block.
                return nullptr;
            }
            if (DT->dominates(newInsertBefore, urbWrite) &&
                DT->dominates(insertBefore, newInsertBefore))
            {
                insertBefore = newInsertBefore;
                argDefExclusionSet.insert(insertBefore->getParent());
            }
            continue;
        }
        // In all other cases do not move the URBWrite out of its current
        // basic block.
        insertBefore = &(*uwBB->getFirstInsertionPt());
        break;
    }
    // Do not sink URB Writes into a `do {} while()` loop but allow hoisting
    // out of `do {} while()` loops.
    while (LI->getLoopFor(insertBefore->getParent()) &&
           !LI->getLoopFor(insertBefore->getParent())->contains(uwBB) &&
           LI->getLoopFor(insertBefore->getParent()) != LI->getLoopFor(uwBB))
    {
        BasicBlock* exitBB = LI->getLoopFor(insertBefore->getParent())->getUniqueExitBlock();
        insertBefore = exitBB ?
            &(*exitBB->getFirstInsertionPt()) :
            &(*uwBB->getFirstInsertionPt());
    }
    IGC_ASSERT(insertBefore != nullptr);
    return insertBefore;
}

// Check all URBWrite instructions in program order and hoist them closer to
// their argument definitions.
bool HoistURBWrites::Hoist()
{
    bool modified = false;
    // m_URBAccesses is in dominator tree breadth first order.
    for (GenIntrinsicInst* intr : m_URBAccesses)
    {
        if (!isURBWriteIntrinsic(intr))
        {
            continue;
        }
        // `insertBefore` contains the location to move the URBWrite
        // instruction before
        Instruction* insertBefore = IsSafeToHoistURBWriteInstruction(intr);
        if (insertBefore && insertBefore != intr->getNextNode())
        {
            if (isa<PHINode>(insertBefore))
            {
                intr->moveBefore(insertBefore->getParent()->getFirstNonPHIOrDbgOrLifetime());
            }
            else
            {
                intr->moveBefore(insertBefore);
            }
            modified = true;
        }
    }
    return modified;
}

void HoistURBWrites::GatherURBAccesses(llvm::Function& F)
{
    DominatorTree* DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

    // Process all basic blocks in dominator tree breadth first traversal.
    for (auto domIter = bf_begin(DT->getRootNode()),
        domEnd = bf_end(DT->getRootNode());
        domIter != domEnd;
        ++domIter)
    {
        BasicBlock* BB = domIter->getBlock();
        if (BB == nullptr)
        {
            continue;
        }
        for (auto II = BB->begin(), IE = BB->end();
            II != IE;
            ++II)
        {
            GenIntrinsicInst* intrin = dyn_cast<GenIntrinsicInst>(&*II);
            if (intrin &&
                (intrin->isGenIntrinsic(GenISAIntrinsic::GenISA_URBRead) ||
                 intrin->isGenIntrinsic(GenISAIntrinsic::GenISA_URBReadOutput) ||
                 intrin->isGenIntrinsic(GenISAIntrinsic::GenISA_urbfence) ||
                 intrin->isGenIntrinsic(GenISAIntrinsic::GenISA_threadgroupbarrier) ||
                 intrin->isGenIntrinsic(GenISAIntrinsic::GenISA_URBWrite)))
            {
                m_URBAccesses.push_back(intrin);
            }
        }
    }
}
} // namespace IGC
