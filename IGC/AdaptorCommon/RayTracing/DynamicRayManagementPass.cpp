/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#include "RTBuilder.h"
#include "common/LLVMUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/CFG.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;
using namespace llvm;
using namespace std;
using namespace RTStackFormat;

//Lowering pass for Synchronous raytracing intrinsics known as TraceRayInline/RayQuery
class DynamicRayManagementPass : public FunctionPass
{
public:
    DynamicRayManagementPass() : FunctionPass(ID)
    {
        initializeDynamicRayManagementPassPass(*PassRegistry::getPassRegistry());
    }

    bool runOnFunction(Function& F) override;

    llvm::StringRef getPassName() const override
    {
        return "DynamicRayManagementPass";
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.addRequired<CodeGenContextWrapper>();
        AU.addRequired<llvm::DominatorTreeWrapperPass>();
        AU.addRequired<llvm::PostDominatorTreeWrapperPass>();
        AU.addRequired<LoopInfoWrapperPass>();
    }

    static char ID;

private:

    llvm::AllocaInst* FindAlloca(llvm::Value*);
    void FindLoadsFromAlloca(
        llvm::User* allocaUser,
        llvm::SmallVector< llvm::LoadInst*, 4>& foundLoads);

    bool AddDynamicRayManagement(Function& F);
    void HandleComplexControlFlow(Function& F);

    void HoistBeforeMostInnerLoop(
        BasicBlock*& dominatorBasicBlock,
        BasicBlock*& commonPostDominatorForRayQueryUsers);

    void HoistBeforeMostOuterLoop(
        BasicBlock*& dominatorBasicBlock,
        BasicBlock*& commonPostDominatorForRayQueryUsers);

    Instruction* FindReleaseInsertPoint(
        BasicBlock* commonPostDominatorForRayQueryUsers,
        const vector<AllocateRayQueryIntrinsic*>& allocateRayQueries,
        const std::unordered_set<llvm::AllocaInst*>& allocasForRayQueries);

    CodeGenContext*    m_CGCtx = nullptr;
    DominatorTree*     m_DT    = nullptr;
    PostDominatorTree* m_PDT   = nullptr;

    std::vector<std::tuple<RayQueryCheckIntrinsic*, RayQueryReleaseIntrinsic*>> m_RayQueryCheckReleasePairs;
};

char DynamicRayManagementPass::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-dynamic-ray-management-pass"
#define PASS_DESCRIPTION "DynamicRayManagementPass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(DynamicRayManagementPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(DynamicRayManagementPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)


bool DynamicRayManagementPass::runOnFunction(Function& F)
{
    m_CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    m_PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

    bool changed = false;

    // Dot not process further if:
    // 1. RayTracing is not supported on this platform.
    // 2. Shader does not use RayQuery at all.
    // 3. There are more than 1 exit block.
    if ((m_CGCtx->platform.supportRayTracing() == false) ||
        (!m_CGCtx->hasSyncRTCalls()) ||
        (getNumberOfExitBlocks(F) > 1))
    {
        return false;
    }

    changed = AddDynamicRayManagement(F);

    if (changed)
    {
        HandleComplexControlFlow(F);
    }

    DumpLLVMIR(m_CGCtx, "DynamicRayManagementPass");

    return changed;
}

// Find an Alloca in the chain of GEPS and Bitcasts.
// If the RayQuery object is written to the Alloca,
// the usages of that Alloca must be used to determine the actual
// last usage of RayQueryObject. This function recursively iterates through
// the chain of GEPS and Bitcasts to find if an Alloca is written to.
// Otherwise null pointer is returned.
llvm::AllocaInst* DynamicRayManagementPass::FindAlloca(llvm::Value* Instruction)
{
    if (llvm::GetElementPtrInst* gep = dyn_cast<llvm::GetElementPtrInst>(Instruction))
    {
        if (llvm::AllocaInst* allocaDest = dyn_cast<llvm::AllocaInst>(gep->getPointerOperand()))
        {
            return allocaDest;
        }
        else
        {
            return FindAlloca(gep->getPointerOperand());
        }
    }

    if (llvm::BitCastInst* bitcast = dyn_cast<llvm::BitCastInst>(Instruction))
    {
        if (llvm::AllocaInst* allocaDest = dyn_cast<llvm::AllocaInst>(bitcast->getOperand(0)))
        {
            return allocaDest;
        }
        else
        {
            return FindAlloca(bitcast->getOperand(0));
        }
    }

    return nullptr;
}

// Find a Load which is at the end of the chain of GEPS and Bitcasts.
// If the RayQuery object is loaded from the Alloca,
// the usages of that Load must be used to determine the actual
// last usage of RayQueryObject. This function recursively iterates through
// the chain of GEPS and Bitcasts to find all Loads from an Alloca
// RayQuery object was written to.
//
// NOTICE!
//
// This may return false positives if the same alloca is used for
// other data than RayQuery objects. As RayQueryRelease is always inserted
// after the very last usage of RayQuery Object, in the worst case scenario this could
// result in postponing the Release. It not handled currently as it
// is not sure if this is a real case scenario and it might be impossible to
// verify whether the access to the alloca (either by GEP or by Bitcast),
// is actually an access to the RayQuery object. GEP parameters may be
// a runtime values, untrackable in compile time.

void DynamicRayManagementPass::FindLoadsFromAlloca(
    llvm::User* allocaUser,
    llvm::SmallVector< llvm::LoadInst*, 4>& foundLoads)
{
    if (llvm::GetElementPtrInst* gep = dyn_cast<llvm::GetElementPtrInst>(allocaUser))
    {
        for(llvm::User* gepUser :  gep->users())
        {
            if (llvm::LoadInst* gepDest = dyn_cast<llvm::LoadInst>(gepUser))
            {
                foundLoads.push_back(gepDest);
            }
            else
            {
                FindLoadsFromAlloca(gepUser, foundLoads);
            }
        }
    }

    if (llvm::BitCastInst* bitCast = dyn_cast<llvm::BitCastInst>(allocaUser))
    {
        for (llvm::User* bitCastUser : bitCast->users())
        {
            if (llvm::LoadInst* bitCastDest = dyn_cast<llvm::LoadInst>(bitCastUser))
            {
                foundLoads.push_back(bitCastDest);
            }
            else
            {
                FindLoadsFromAlloca(bitCastUser, foundLoads);
            }
        }
    }
}

bool DynamicRayManagementPass::AddDynamicRayManagement(Function& F)
{
    vector<AllocateRayQueryIntrinsic*>    allocateRayQueries;
    std::unordered_set<llvm::AllocaInst*> allocasForRayQueries;

    RTBuilder builder(&*F.getEntryBlock().begin(), *m_CGCtx);

    BasicBlock* commonPostDominatorForRayQueryUsers = nullptr;

    // Find all AllocateRayQueryIntrinsic in the function.
    for (Instruction& I : instructions(F))
    {
        if (AllocateRayQueryIntrinsic* allocateRayQueryIntrinsic = dyn_cast<AllocateRayQueryIntrinsic>(&I))
        {
            if (allocateRayQueryIntrinsic->hasNUses(0))
            {
                continue;
            }

            allocateRayQueries.push_back(allocateRayQueryIntrinsic);

            if (commonPostDominatorForRayQueryUsers == nullptr)
            {
                commonPostDominatorForRayQueryUsers = allocateRayQueryIntrinsic->getParent();
            }

            // If the result of AllocateRayQueryIntrinsic is written to the
            // Alloca, e.g. if and array of RayQueries is used, the last found
            // usage will be much sooner than actual end of RayQuery object lifespan.
            // To handle this, it is checked whether the AllocateRayQueryIntrinsic
            // user is a Store to Alloca.
            // commonPostDominatorForRayQueryUsers is updated to the last
            // usage of all these Allocas.
            //
            // NOTICE!
            //
            // This may return false positives if the same alloca is used for
            // other data than RayQuery objects. As RayQueryRelease is always inserted
            // after the very last usage of RayQuery Object, in the worst case scenario this could
            // result in postponing the Release. It not handled currently as it
            // is not sure if this is a real case scenario and it might be impossible to
            // verify whether the access to the alloca (either by GEP or by Bitcast),
            // is actually an access to the RayQuery object. GEP parameters may be
            // a runtime values, untrackable in compile time.
            for (User* rayQueryUser : allocateRayQueryIntrinsic->users())
            {
                commonPostDominatorForRayQueryUsers = m_PDT->findNearestCommonDominator(commonPostDominatorForRayQueryUsers, cast<Instruction>(rayQueryUser)->getParent());
                if (llvm::StoreInst* storeRayQuery = dyn_cast<llvm::StoreInst>(rayQueryUser))
                {
                    llvm::AllocaInst* allocaForRayQuery = FindAlloca(storeRayQuery->getPointerOperand());

                    if (allocaForRayQuery != nullptr)
                    {
                        // If the same Alloca is used for many RayQuery objects, check
                        // if it was already processed to avoid multiple looking for new
                        // commonPostDominatorForRayQueryUsers from the same Alloca.
                        // It it was not processed, was not in the allocasForRayQueries set,
                        // insert it there and try find new commonPostDominatorForRayQueryUsers.
                        if (allocasForRayQueries.find(allocaForRayQuery) == allocasForRayQueries.end())
                        {
                            allocasForRayQueries.insert(allocaForRayQuery);

                            for (User* allocaUser : allocaForRayQuery->users())
                            {
                                commonPostDominatorForRayQueryUsers = m_PDT->findNearestCommonDominator(commonPostDominatorForRayQueryUsers, cast<Instruction>(allocaUser)->getParent());
                            }
                        }
                    }
                }
            }
        }
    }

    if (allocateRayQueries.size() == 0)
    {
        return false;
    }

    // If we reached this point, the commonPostDominatorForRayQueryUsers
    // must be found.
    IGC_ASSERT(commonPostDominatorForRayQueryUsers != nullptr);

    // Find the Block which dominates all AllocateRayQueryIntrinsics
    // This will be the place to put RayQueryCheck().
    BasicBlock* dominatorBasicBlock = allocateRayQueries[0]->getParent();

    for (AllocateRayQueryIntrinsic* allocateRayQueryIntrinsic : allocateRayQueries)
    {
        dominatorBasicBlock = m_DT->findNearestCommonDominator(dominatorBasicBlock, allocateRayQueryIntrinsic->getParent());
    }

    if (IGC_IS_FLAG_DISABLED(EnableOuterLoopHoistingForRayQueryDynamicRayManagementMechanism))
    {
        // If the dominatorBasicBlock or commonPostDominatorForRayQueryUsers are
        // inside a loop or nested loops, move them outside.
        HoistBeforeMostOuterLoop(dominatorBasicBlock, commonPostDominatorForRayQueryUsers);
    }
    else
    {
        // If the dominatorBasicBlock is outside the loop commonPostDominatorForRayQueryUsers
        // is in, move up the commonPostDominatorForRayQueryUsers.
        HoistBeforeMostInnerLoop(dominatorBasicBlock, commonPostDominatorForRayQueryUsers);
    }

    // Find the first AllocateRayQueryIntrinsic in the block which
    // dominates all AllocateRayQueryIntrinsics. Insert Check instruction
    // immediately before this first AllocateRayQueryIntrinsic.
    Instruction* rayQueryCheckInsertPoint = nullptr;

    for (Instruction& I : dominatorBasicBlock->getInstList())
    {
        if (AllocateRayQueryIntrinsic* allocateRayQueryIntrinsic = dyn_cast<AllocateRayQueryIntrinsic>(&I))
        {
            rayQueryCheckInsertPoint = allocateRayQueryIntrinsic;
            break;
        }
    }

    if(rayQueryCheckInsertPoint != nullptr)
    {
        builder.SetInsertPoint(rayQueryCheckInsertPoint);
    }
    else
    {
        builder.SetInsertPoint(dominatorBasicBlock->getTerminator());
    }

    // RayQueryCheck intrinsic returns a value only to be passed to the corresponding
    // RayQueryRelease.
    RayQueryCheckIntrinsic* rayQueryCheck = builder.CreateRayQueryCheckIntrinsic();

    // Find the last usage of AllocateRayQueryIntrinsic to put Release
    // instruction immediately after it.
    Instruction* rayQueryReleaseInsertPoint = FindReleaseInsertPoint(commonPostDominatorForRayQueryUsers, allocateRayQueries, allocasForRayQueries);

    // Set the insert point for Release instruction.
    // It might be:
    // 1. The last usage of AllocateRayQueryIntrinsic. Release instruction is
    //    put after it.
    // 2. The Block which post dominates all the AllocateRayQueryIntrinsic uses.
    //    This happens if no user if found in this Block.
    //    Release is put at its beginning.
    if (rayQueryReleaseInsertPoint != nullptr)
    {
        IGC_ASSERT(rayQueryReleaseInsertPoint->getNextNode() != nullptr);
        builder.SetInsertPoint(rayQueryReleaseInsertPoint->getNextNode());
    }
    else if (commonPostDominatorForRayQueryUsers->hasNPredecessors(1))
    {
        // Single-exit loop case, common post-dominator has only one
        // predecessor that is a loop exiting block.
        builder.SetInsertPoint(commonPostDominatorForRayQueryUsers->getFirstNonPHI());
    }
    else
    {
        // If Release is put in the block which does not contain the last
        // AllocateRayQueryIntrinsic user, it might be reached from
        // blocks which are outside RayQuery path.
        //
        //          bb2 (RayQuery) -> (some other blocks)     ->
        //  bb1 ->                                                  commonPostDominator
        //          bb3 (non RayQuery) -> (some other blocks) ->
        //
        // In this case a new block is inserted between commonPostDominator
        // and its rayQuery related predecessors:
        //
        //          bb2 (RayQuery) -> (some other blocks)     -> newBlock ->
        //  bb1 ->                                                           commonPostDominator
        //          bb3 (non RayQuery) -> (some other blocks) ->
        //
        // This way there will be a block which will postDomiante only
        // RayQuery related blocks.

        // Find predecessors of commonPostDominatorForRayQueryUsers which can
        // be reached from blocks which contains AllocateRayQueryIntrinsics.
        llvm::SmallVector<BasicBlock*,4> blocksToBranchToNewPostDominator;

        for (BasicBlock* predecessor : predecessors(commonPostDominatorForRayQueryUsers))
        {
            for (AllocateRayQueryIntrinsic* allocateRayQueryIntrinsic : allocateRayQueries)
            {
                BasicBlock* allocateRayQueryBlock = allocateRayQueryIntrinsic->getParent();

                // Check if predecessor is the same as allocateRayQueryBlock. In that
                // case the Block is immediately, without reachability check, included
                // into the list of Blocks which will branch to the NewPostDominator.
                if (predecessor == allocateRayQueryBlock)
                {
                    blocksToBranchToNewPostDominator.push_back(predecessor);

                    // Stop searching through other allocateRayQueries, as
                    // currently processed block is already on the list.
                    break;
                }

                // Due to isPotentiallyReachable limitations, a precheck is done
                // to exclude the Blocks which Dominates the Blocks with
                // AllocateRayQuery intrinsics. Without it it may happen, if there are
                // a lot of Blocks, that the Block which dominates another one will
                // be marked as reachable from the one it dominates.
                //
                // This does not handles all the cases when isPotentiallyReachable enters conservative
                // mode. It may happen when the search hits the limit of 32 Blocks.
                //
                // if
                // {
                //     AllocateRayQuery_1
                //     TraceRayInlineHL_1
                //     TraceRayInlineCandidateType_1
                //
                //     if
                //     {
                //          AllocateRayQuery_2
                //          TraceRayInlineHL_2
                //          TraceRayInlineCandidateType_2
                //
                //          if
                //          {
                //              {} - >= 29 x Nested Blocks
                //          }
                //      }
                // }
                // else {}
                //
                // In that case 'else' Block will be marked as reachable from
                // the second 'if' Block because the search for the path from
                // 'if' to 'else' will exceed the isPotentiallyReachable internal
                // threshold. There is a LIT test which is expected to fail:
                // \tests\DynamicRayManagement\negative_isReachableLimits.ll
                // until this case is handled.

                if (m_DT->dominates(predecessor, allocateRayQueryBlock))
                {
                    continue;
                }

                // For each commonPostDominatorForRayQueryUsers's predecessor find if
                // there is a path through it to any of Blocks with AllocateRayQuery.

                // This vector will keep all the predecessors to be checked.
                // SetVector is used to keep only unique Blocks in order they
                // are added.
                llvm::SetVector<llvm::BasicBlock*> predecessorsList;

                // Start with current predecessor of commonPostDominatorForRayQueryUsers.
                // Adds its predecessors to the vector.
                for (BasicBlock* pred : predecessors(predecessor))
                {
                    predecessorsList.insert(pred);
                }

                // Iterate through the list of predecessors. Use direct indexing, as
                // the list will expand inside the loop. Every Block in the list is
                // checked if it's a Block with AllocateRayQuery. If it is, the path is
                // found, the predecessor of commonPostDominatorForRayQueryUsers is added
                // to the blocksToBranchToNewPostDominator and the search is over.
                // In other case, the predecessors of currently processed Block
                // are added to the list to be checked.
                for (size_t predecessorIndex = 0; predecessorIndex < predecessorsList.size(); ++predecessorIndex)
                {
                    if (predecessorsList[predecessorIndex] == allocateRayQueryBlock)
                    {
                        blocksToBranchToNewPostDominator.push_back(predecessor);

                        // Stop searching through other allocateRayQueries, as
                        // currently processed block is already on the list.
                        break;
                    }

                    for (BasicBlock* pred : predecessors(predecessorsList[predecessorIndex]))
                    {
                        predecessorsList.insert(pred);
                    }
                }
            }
        }

        // Create new BasicBlock, which will post dominate only
        // Blocks related to RayQuery.
        BasicBlock* blockWhichPostDominatesOnlyRayQueryUsers = llvm::SplitBlockPredecessors(commonPostDominatorForRayQueryUsers, blocksToBranchToNewPostDominator, "blockWhichPostDominatesOnlyRayQueryUsers", m_DT);

        // Insert an unconditional Branch from new blockWhichPostDominatesOnlyRayQueryUsers to
        // commonPostDominatorForRayQueryUsers, and set insert point before the Branch,
        // to put Release there.
        //builder.SetInsertPoint(builder.CreateBr(commonPostDominatorForRayQueryUsers));
        builder.SetInsertPoint(&*blockWhichPostDominatesOnlyRayQueryUsers->getFirstInsertionPt());

        // Update Dominator and PostDominator analyses after inserting a new block.
        m_PDT->recalculate(F);
    }

    // The third argument is a value returned by RayQueryCheck, it is used only
    // RayQueryCheck-Release pair identification.
    RayQueryReleaseIntrinsic* rayQueryRelease = builder.CreateRayQueryReleaseIntrinsic();

    // Add created RayQueryCheck-Release to the list, which
    // will be used during complex control flow handling.
    m_RayQueryCheckReleasePairs.push_back(std::make_tuple(rayQueryCheck, rayQueryRelease));

    return true;
}

void DynamicRayManagementPass::HandleComplexControlFlow(Function& F)
{
    RTBuilder builder(&*F.getEntryBlock().begin(), *m_CGCtx);

    // If the function contains barriers or asynch TraceRays or Callable shaders calls
    // or is a user defined function call RayQueryRelease must be called before them,
    // and GenISA_RayQueryCheck after to avoid deadlocks.
    for (Instruction& I : instructions(F))
    {
        if (isa<ContinuationHLIntrinsic>(&I) ||
            isBarrierIntrinsic(&I) ||
            isUserFunctionCall(&I))
        {
            // Look through all RaytQueryCheck-Release pairs, and check if the barrier/call
            // instruction is within any of pairs.
            for (uint32_t rayQueryCheckReleasePairIndex = 0; rayQueryCheckReleasePairIndex < m_RayQueryCheckReleasePairs.size(); ++rayQueryCheckReleasePairIndex)
            {
                auto [rayQueryCheckIntrinsic, rayQueryReleaseIntrinsic] = m_RayQueryCheckReleasePairs[rayQueryCheckReleasePairIndex];

                BasicBlock* insertBlock = I.getParent();

                if (m_DT->dominates(rayQueryCheckIntrinsic->getParent(), insertBlock) &&
                    m_PDT->dominates(rayQueryReleaseIntrinsic->getParent(), insertBlock))
                {
                    // If the DisableRayQueryDynamicRayManagementMechanismForExternalFunctionsCalls flag
                    // is enabled, remove Check/Release pairs which encapsulates any external function call.
                    if (IGC_IS_FLAG_ENABLED(DisableRayQueryDynamicRayManagementMechanismForExternalFunctionsCalls) &&
                        isa<ContinuationHLIntrinsic>(&I))
                    {
                        rayQueryReleaseIntrinsic->eraseFromParent();
                        rayQueryCheckIntrinsic->eraseFromParent();

                        // Remove the pair from the vector in case more Barriers or External
                        // calls are between them.
                        m_RayQueryCheckReleasePairs.erase(m_RayQueryCheckReleasePairs.begin() + rayQueryCheckReleasePairIndex);

                        break;
                    }

                    // If the DisableRayQueryDynamicRayManagementMechanismForBarriers flag
                    // is enabled, remove Check/Release pairs which encapsulates any Barrier.
                    if (IGC_IS_FLAG_ENABLED(DisableRayQueryDynamicRayManagementMechanismForBarriers) &&
                        isBarrierIntrinsic(&I))
                    {
                        rayQueryReleaseIntrinsic->eraseFromParent();
                        rayQueryCheckIntrinsic->eraseFromParent();

                        // Remove the pair from the vector in case more Barriers or External
                        // calls are between them.
                        m_RayQueryCheckReleasePairs.erase(m_RayQueryCheckReleasePairs.begin() + rayQueryCheckReleasePairIndex);

                        break;
                    }

                    // The barrier/call instruction is within the RayQueryCheck-Release pair.
                    // Insert RayQueryRelease before it, and RaytQueryCheck after, to re-enable
                    // Dynamic Ray Management.
                    builder.SetInsertPoint(&I);

                    RayQueryReleaseIntrinsic* rayQueryRelease = builder.CreateRayQueryReleaseIntrinsic();

                    builder.SetInsertPoint(I.getNextNode());

                    RayQueryCheckIntrinsic* rayQueryCheck = builder.CreateRayQueryCheckIntrinsic();

                    // Add new pair to the list.
                    m_RayQueryCheckReleasePairs.push_back(std::make_tuple(rayQueryCheck, rayQueryRelease));

                    // TODO: Make sure this is correct for the case:
                    //
                    // RayQueryCheck()
                    //
                    // if(non_uniform)
                    // {
                    //      TraceRay()
                    // }
                    //
                    // barrier()
                    //
                    // RayQueryRelease()

                    break;
                }
            }
        }
    }
}

// This function will move Release call outside the loop
// if the Check is also outside it.
//
// Before:
//
// RayQuery<> q;
// Check()
// q.TraceRayInline(...);
//
// do {
//     bool p = q.Proceed();
//     Release()
//         if (!p)
//             break;
//     ...
// } while (true);
//
// After:
//
// RayQuery<> q;
// Check()
// q.TraceRayInline(...);
//
// do {
//     bool p = q.Proceed();
//         if (!p)
//             break;
//     ...
// } while (true);
// Release()
//
void DynamicRayManagementPass::HoistBeforeMostInnerLoop(
    BasicBlock*& dominatorBasicBlock,
    BasicBlock*& commonPostDominatorForRayQueryUsers)
{
    LoopInfo* LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

    Loop* loopForDominator = LI->getLoopFor(dominatorBasicBlock);
    Loop* loopForPostDominator = LI->getLoopFor(commonPostDominatorForRayQueryUsers);
    Loop* currentLoop = nullptr;

    // If the PostDominator is not in the loop, there is nothing to be hoisted.
    if (loopForPostDominator != nullptr)
    {
        // Iterate through the loops the PostDominator is in and
        // find the first loop the Dominator is NOT in.
        // That would be the loop Release must be hoisted to.
        while (loopForPostDominator != nullptr)
        {
            // Stop searching if found the loop
            // where the Dominator is.
            if (loopForPostDominator == loopForDominator)
            {
                break;
            }

            // Remember current loop, and check whether it is
            // in another loop.
            currentLoop = loopForPostDominator;
            loopForPostDominator = loopForPostDominator->getParentLoop();
        }

        // If currentLoop is null at this point, it means
        // both Dominator and PostDominator are already in the same loop.
        // Nothing more left to do.
        if (currentLoop != nullptr)
        {
            // Get all exit blocks for the found loop.
            SmallVector<BasicBlock*, 4> exitBlocks;
            currentLoop->getExitBlocks(exitBlocks);

            commonPostDominatorForRayQueryUsers = nullptr;

            // Iterate through the exit blocks, and found their common post
            // dominator block.
            for (BasicBlock* exitBlock : exitBlocks)
            {
                if (commonPostDominatorForRayQueryUsers == nullptr)
                {
                    commonPostDominatorForRayQueryUsers = exitBlock;
                    continue;
                }

                commonPostDominatorForRayQueryUsers = m_PDT->findNearestCommonDominator(commonPostDominatorForRayQueryUsers, exitBlock);
            }
        }
    }

    // Finally find a block which post dominates new Dominator and PostDominator,
    // and update PostDominator.
    // Use as final PostDominator.
    commonPostDominatorForRayQueryUsers = m_PDT->findNearestCommonDominator(commonPostDominatorForRayQueryUsers, dominatorBasicBlock);
}

// If the Rayquery is executed or created in the loop, make sure
// Check and Release are put outside the most outer loop.
// This functions updates Dominator and PostDominator blocks.
void DynamicRayManagementPass::HoistBeforeMostOuterLoop(
    BasicBlock*& dominatorBasicBlock,
    BasicBlock*& commonPostDominatorForRayQueryUsers)
{
    LoopInfo* LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

    Loop* loopForDominator = LI->getLoopFor(dominatorBasicBlock);
    Loop* loopForPostDominator = LI->getLoopFor(commonPostDominatorForRayQueryUsers);

    // Move Dominator outside the nested loops
    // to keep the LSC locked for all the RayQuery objects lifetime.
    if (loopForDominator != nullptr)
    {
        Loop* parentLoop = loopForDominator->getParentLoop();

        // Find the outer most loop.
        while (parentLoop != nullptr)
        {
            loopForDominator = parentLoop;
            parentLoop = loopForDominator->getParentLoop();
        }

        // Reset the Dominator, as it will be set to a new
        // value in the code below.
        dominatorBasicBlock = nullptr;

        // Find the block which dominates all entering blocks
        // for the most outer loop.
        for (BasicBlock* loopPredessesor : predecessors(loopForDominator->getHeader()))
        {
            // Initially use the first entering as the dominator.
            if (dominatorBasicBlock == nullptr)
            {
                dominatorBasicBlock = loopPredessesor;
                continue;
            }

            // This will find the block which dominates all entries of the loop.
            // TODO: But it could be optimized:
            // Create a BasicBlock which branches to the header and redirect all
            // entries to branch to this new BasicBlock and place Check in it.
            dominatorBasicBlock = m_DT->findNearestCommonDominator(dominatorBasicBlock, loopPredessesor);
        }
    }

    // Move PostDominator after all nested loops.
    if (loopForPostDominator != nullptr)
    {
        Loop* parentLoop = loopForPostDominator->getParentLoop();

        while (parentLoop != nullptr)
        {
            loopForPostDominator = parentLoop;
            parentLoop = loopForPostDominator->getParentLoop();
        }

        IGC_ASSERT(loopForPostDominator != nullptr);

        SmallVector<BasicBlock*, 4> exitBlocks;
        loopForPostDominator->getExitBlocks(exitBlocks);

        for (BasicBlock* exitBlock : exitBlocks)
        {
            commonPostDominatorForRayQueryUsers = m_PDT->findNearestCommonDominator(commonPostDominatorForRayQueryUsers, exitBlock);
        }
    }

    // Finally find a block which post dominates new Dominator and PostDominator,
    // and update PostDominator.
    // Use as final PostDominator.
    commonPostDominatorForRayQueryUsers = m_PDT->findNearestCommonDominator(commonPostDominatorForRayQueryUsers, dominatorBasicBlock);
}

// Find the last usage of AllocateRayQueryIntrinsic to put Release
// instruction immediately after it.
Instruction* DynamicRayManagementPass::FindReleaseInsertPoint(
    BasicBlock* commonPostDominatorForRayQueryUsers,
    const vector<AllocateRayQueryIntrinsic*>& allocateRayQueries,
    const std::unordered_set<llvm::AllocaInst*>& allocasForRayQueries)
{
    // Start from the last instruction in the Block that post dominates all
    // uses.
    for (Instruction& instruction : reverse(commonPostDominatorForRayQueryUsers->getInstList()))
    {
        // For each instruction the Block check if it is a user of any
        // AllocateRayQueryIntrinsic. Stop at first match.
        for (AllocateRayQueryIntrinsic* allocateRayQueryIntrinsic : allocateRayQueries)
        {
            for (User* rayQueryUser : allocateRayQueryIntrinsic->users())
            {
                if ((&instruction) == rayQueryUser)
                {
                    return &instruction;
                }
            }
        }

        // If the result of AllocateRayQueryIntrinsic was written to the
        // Alloca, e.g. if and array of RayQueries is used, the last found
        // usage will be much sooner than actual end of RayQuery object lifespan.
        // To handle this, it is checked whether the Alloca to which
        // RayQuery Objects was written to is a source for a Load instruction.
        // If currently processed instruction is a user of such Load,
        // it is the last user of RayQuery Object in this Block.
        //
        // NOTICE!
        //
        // This may return false positives if the same alloca is used for
        // other data than RayQuery objects. As RayQueryRelease is always inserted
        // after the very last usage of RayQuery Object, in the worst case scenario this could
        // result in postponing the Release. It not handled currently as it
        // is not sure if this is a real case scenario and it might be impossible to
        // verify whether the access to the alloca (either by GEP or by Bitcast),
        // is actually an access to the RayQuery object. GEP parameters may be
        // a runtime values, untrackable in compile time.
        for (AllocaInst* allocaForRayQueryIntrinsic : allocasForRayQueries)
        {
            for (User* allocaUser : allocaForRayQueryIntrinsic->users())
            {
                llvm::SmallVector< llvm::LoadInst*, 4> loadsFromAlloca;

                FindLoadsFromAlloca(allocaUser, loadsFromAlloca);

                for (llvm::LoadInst* loadFromAlloca : loadsFromAlloca)
                {
                    for (User* loadFromAllocaUser : loadFromAlloca->users())
                    {
                        if ((&instruction) == loadFromAllocaUser)
                        {
                            return &instruction;
                        }
                    }
                }
            }
        }
    }

    return nullptr;
}

namespace IGC
{
    Pass* CreateDynamicRayManagementPass()
    {
        return new DynamicRayManagementPass();
    }
}
