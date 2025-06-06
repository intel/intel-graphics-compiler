/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <GenISAIntrinsics/GenIntrinsicInst.h>
#include "WaveShuffleIndexSinking.hpp"
#include "Compiler/IGCPassSupport.h"
#include <igc_regkeys.hpp>
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Dominators.h>
#include "common/LLVMWarningsPop.hpp"

#define DEBUG_TYPE "igc-wave-shuffle-index-sinking"

using namespace IGC;
using namespace llvm;

namespace IGC
{
    class WaveShuffleIndexSinkingImpl
    {
        class ShuffleGroup
        {
            // Group of WaveShuffleIndex instructions with constant lane indexes that have one or more identical instructions after
            // ex.
            // %0 = ...
            // %1 = ...
            // %2 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 0, i32 0)
            // %3 = add i32 %2, %1
            // %4 = shl i32 %3, 2
            // %use_4 = call @f(%4)
            // %5 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 1, i32 0)
            // %6 = add i32 %5, %1
            // %7 = shl i32 %6, 2
            // %use_7 = call @f(%7)
            // %8 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 2, i32 0)
            // %9 = add i32 %8, %1
            // %10 = shl i32 %9, 2
            // %use_10 = call @f(%10)
            //
            // This can be transformed to the following since each WaveShuffleIndex is essentially a broadcast operation (from having a constant lane index)
            // Uniform (Constant) operands in operations following a WaveShuffleIndex can be hoisted to the source
            // Depending on the distributive properties of instructions, the shl in this example can be hoisted above the add, and afterwards hoisted before the WaveShuffleIndex
            // %0 = ...
            // %1 = ...
            // %2 = shl i32 %0, 2
            // %3 = shl i32 %1, 2
            // %4 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %2, i32 0, i32 0)
            // %5 = add i32 %3, %4
            // %use_4 = call @f(%5)
            // %6 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %2, i32 1, i32 0)
            // %7 = add i32 %3, %6
            // %use_7 = call @f(%7)
            // %8 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %2, i32 2, i32 0)
            // %9 = add i32 %3, %8
            // %use_10 = call @f(%9)
            //
            // This reduces the number of instructions in this particular ShuffleGroup from 3 * # of WaveShuffleIndex to 2 + 2 * # of WaveShuffleIndex
            // The more WaveShuffleIndex instructions in a ShuffleGroup, the more effective this transformation is
        public:
            ShuffleGroup( WaveShuffleIndexIntrinsic* shuffleInst )
            {
                ShuffleOps.push_back( shuffleInst );
            }

            // Attempt to match a new WaveShuffleIndex instruction to this ShuffleGroup
            bool match( WaveShuffleIndexIntrinsic* shuffleInst )
            {
                if( ShuffleOps.size() == 1 )
                {
                    // Attempting to match with fresh ShuffleGroup, match the maximal number of instructions
                    SmallVector<BinaryOperator*> InstChainA;
                    SmallVector<BinaryOperator*> InstChainB;
                    SmallVector<bool> NewHoistOrAnchorInstsIdx;
                    unsigned numHoistable = compareWaveShuffleIndexes( ShuffleOps.front(), shuffleInst, InstChainA, InstChainB, NewHoistOrAnchorInstsIdx );

                    if( numHoistable == 0 )
                    {
                        // Only match new shuffleInst with current ShuffleGroup if hoistable targets were found
                        return false;
                    }

                    // Update ShuffleGroup members
                    HoistOrAnchorInstsIdx = std::move(NewHoistOrAnchorInstsIdx);
                    InstChains.push_back( InstChainA );
                    InstChains.push_back( InstChainB );
                    ShuffleOps.push_back( shuffleInst );
                    return true;
                }
                else
                {
                    // Use the first chain in the existing ShuffleGroup to check if the new shuffleInst can fit into the ShuffleGroup
                    SmallVector<BinaryOperator*> NewInstChain;
                    SmallVector<bool> NewHoistOrAnchorInstsIdx;
                    unsigned numHoistable = compareWaveShuffleIndexes( ShuffleOps.front(), shuffleInst, InstChains.front(), NewInstChain, NewHoistOrAnchorInstsIdx );
                    if( numHoistable == 0 )
                    {
                        // Only match new shuffleInst with current ShuffleGroup if hoistable targets were found
                        return false;
                    }

                    // New shuffleInst fits, but NewInstChain.size() may be lesser than the existing instChains
                    // Reduce the hoistable instructions in the current group for now
                    // Truncated hoistable instructions will be processed in the next iteration when matching to a smaller ShuffleGroup
                    // (comprising of the existing InstChains and excluding NewInstChain)
                    for( auto& instChain : InstChains )
                    {
                        instChain.truncate( NewInstChain.size() );
                    }

                    // Update ShuffleGroup members
                    HoistOrAnchorInstsIdx = std::move(NewHoistOrAnchorInstsIdx); // this should be the same size as NewInstChain
                    InstChains.push_back( NewInstChain );
                    ShuffleOps.push_back( shuffleInst );
                    return true;
                }
            }

            // Once entire ShuffleGroup is gathered, pre-process and find the instructions that are actually profitable to hoist
            // ex. #WS = 2, Inst Cost = #WS * 6 (WS inst + mul + add + shl + add + shl) = 12
            // %0 = ...
            // %1 = ...
            // %2 = ...
            // %3 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 0, i32 0)
            // %4 = mul i32 %3, 5 <- Hoistable
            // %5 = add i32 %4, %1 <- Anchor
            // %6 = shl i32 %5, 2 <- Hoistable, profitable to hoist past Anchor
            // %7 = add i32 %6, %2 <- Anchor
            // %8 = shl i32 %7, 3  <- Hoistable, not profitable to hoist past Anchor, demoted to Anchor
            // %9 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 1, i32 0)
            // %10 = mul i32 %9, 5 <- Hoistable
            // %11 = add i32 %10, %1 <- Anchor
            // %12 = shl i32 %11, 2 <- Hoistable past Anchor
            // %13 = add i32 %12, %2 <- Anchor
            // %14 = shl i32 %13, 3 <- Hoistable, not profitable to hoist past Anchor, demoted to Anchor
            //
            // Result: Inst Cost = 3 (mul + shl + shl) + #WS * 4 (WS inst + add + add + shl) = 11
            // %0 = ...
            // %1 = ...
            // %2 = ...
            // %3 = mul i32 %0, 5
            // %4 = shl i32 %3, 2
            // %5 = shl i32 %1, 2
            // %6 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %4, i32 0, i32 0)
            // %7 = add i32 %6, %5 <- Anchor
            // %8 = add i32 %7, %2 <- Anchor
            // %9 = shl i32 %8, 3 <- Demoted Anchor
            // %10 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %4, i32 1, i32 0)
            // %11 = add i32 %10, %5 <- Anchor
            // %12 = add i32 %11, %2 <- Anchor
            // %13 = shl i32 %12, 3 <- Demoted Anchor

            // Terms:
            // - Anchor: BinaryOperator of which one operand is the preceding value in the InstChain
            //           (or the WaveShuffle, if the Anchor is the first inst), and the other operand is a non-constant
            // - Hoistable: BinaryOperator of which one operand is the preceding value in the InstChain
            //              (or the WaveShuffle, if the Hoistable is the first inst), and the other operand is a constant
            // - Hoistable past Anchor: Through distributive properties, a Hoistable further along the InstChain
            //                          can have its operation distributed to the operands of an Anchor
            // - Profitable to Hoist past Anchor: A "Hoistable past Anchor" instruction that when hoisted, does not result
            //                                    in more overall instructions than pre-hoist
            unsigned preprocess()
            {
                // Nothing to check
                if( InstChains.empty() || InstChains.front().empty() )
                    return false;

                // Profitability:
                // when an instruction is fully hoisted,
                // - one new instruction added to the second operand of each preceding anchor instruction
                // - one new instruction added to the singular source of all the WaveShuffleIndex instructions in the ShuffleGroup
                // - one less instruction per WaveShuffleIndex instruction in the ShuffleGroup
                // so if more instructions needed to be added for each anchor than removed for each shuffle op, optimization is no longer profitable
                //
                // Based on the metric above, some instructions that are currently marked as hoist may need to be demoted to anchor
                unsigned previousAnchorCount = 0;
                unsigned numProfitableHoistable = 0;
                unsigned idx = 0;
                while( idx < HoistOrAnchorInstsIdx.size() )
                {

                    if( previousAnchorCount >= ShuffleOps.size() )
                    {
                        // not profitable anymore, demote all subsequent instructions to anchor regardless
                        HoistOrAnchorInstsIdx[ idx ] = false;
                        // no need to increment previousAnchorCount, all remaining iterations will enter this if block
                    }
                    else
                    {
                        if( !HoistOrAnchorInstsIdx[ idx ] )
                        {
                            previousAnchorCount++;
                        }
                        else
                        {
                            numProfitableHoistable++;
                        }
                    }
                    idx++;
                }

                return numProfitableHoistable;
            }

            bool hoist(DenseMap<BasicBlock*, SmallVector<Instruction*, 4>>& MoveToCommonDominatorInstMap, DominatorTree& DT ) {
                // If there is no common dominator abort hoisting
                BasicBlock* CommonDominator = findCommonDominator( DT );
                if ( !CommonDominator ) return false;

                // Track the new source for all the ShuffleOps
                auto* prev = ShuffleOps.front()->getSrc();

                for( unsigned idx = 0; idx < HoistOrAnchorInstsIdx.size(); idx++ )
                {
                    bool moveToCommonDominator = false;
                    if( HoistOrAnchorInstsIdx[ idx ] )
                    {
                        // clone the inst to be hoisted
                        auto* hoistedInst = InstChains.front()[ idx ]->clone();
                        hoistedInst->setName( InstChains.front()[ idx ]->getName() + "_hoisted" );
                        hoistedInst->insertBefore( ShuffleOps.front() );

                        if ( CommonDominator != hoistedInst->getParent() )
                        {
                            moveToCommonDominator = true;
                            MoveToCommonDominatorInstMap[ CommonDominator ].emplace_back( hoistedInst );
                        }

                        // Replace the correct operand
                        auto* hoistedOp0 = hoistedInst->getOperand( 0 );
                        Instruction* hoistedOpPrev = ( idx == 0 ) ? cast<Instruction>( ShuffleOps.front() ) : InstChains.front()[ idx - 1 ];
                        unsigned chainOpIdx = 0; // Record which operand is the previous inst in the InstChain
                        if( hoistedOp0 == hoistedOpPrev )
                        {
                            hoistedInst->setOperand( 0, prev );
                        }
                        else
                        {
                            chainOpIdx = 1;
                            hoistedInst->setOperand( 1, prev );
                        }

                        prev = hoistedInst;

                        // Create copies for each anchor instruction further up the chain
                        for( unsigned anchorIdx = 0; anchorIdx < idx; anchorIdx++ )
                        {
                            // found anchor
                            if( !HoistOrAnchorInstsIdx[ anchorIdx ] )
                            {
                                // clone the inst to be hoisted
                                auto* anchorHoistedInst = hoistedInst->clone();
                                anchorHoistedInst->setName( hoistedInst->getName() + "_for_" + InstChains.front()[ anchorIdx ]->getName() );
                                anchorHoistedInst->insertBefore( InstChains.front()[ anchorIdx ] );

                                // Replace the correct operand
                                // ex.
                                // %0 = ...
                                // %1 = ...
                                // %2 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 0, i32 0)
                                // %3 = add i32 %2, %1 <- Anchor
                                // %4 = shl i32 %3, 2 <- Hoistable past Anchor
                                // Result:
                                // %0 = ...
                                // %1 = ...
                                // %2 = shl i32 %0, 2 <- hoistedInst (WaveShuffle path)
                                // %3 = shl i32 %1, 2 <- anchorHoistedInst (Anchor path)
                                // %4 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %2, i32 0, i32 0)
                                // %5 = add i32 %4, %2 <- Anchor
                                // Find the operand that originates from outside the chain to use in anchorHoistedInst
                                auto* anchorOp0 = InstChains.front()[ anchorIdx ]->getOperand( 0 );
                                auto* anchorOp1 = InstChains.front()[ anchorIdx ]->getOperand( 1 );
                                Instruction* anchorOpPrev = ( anchorIdx == 0 ) ? cast<Instruction>( ShuffleOps.front() ) : InstChains.front()[ anchorIdx - 1 ];
                                if( anchorOp0 == anchorOpPrev )
                                {
                                    anchorHoistedInst->setOperand( chainOpIdx, anchorOp1 );
                                }
                                else
                                {
                                    anchorHoistedInst->setOperand( chainOpIdx, anchorOp0 );
                                }

                                // Properly set the anchor instructions in all chains to use the new anchorHoistedInst
                                for( unsigned i = 0; i < ShuffleOps.size(); i++ )
                                {
                                    auto* anchorOp0 = InstChains[ i ][ anchorIdx ]->getOperand( 0 );
                                    Instruction* anchorOpPrev = ( anchorIdx == 0 ) ? cast<Instruction>( ShuffleOps[ i ] ) : InstChains[ i ][ anchorIdx - 1 ];
                                    if( anchorOp0 == anchorOpPrev )
                                    {
                                        InstChains[ i ][ anchorIdx ]->setOperand( 1, anchorHoistedInst );
                                    }
                                    else
                                    {
                                        InstChains[ i ][ anchorIdx ]->setOperand( 0, anchorHoistedInst );
                                    }
                                }

                                // If hoisted instruction is moved, it's safe to move anchor as well.
                                if ( moveToCommonDominator )
                                {
                                    MoveToCommonDominatorInstMap[ CommonDominator ].emplace_back( anchorHoistedInst );
                                }
                            }
                        }
                    }
                }

                // prev is last hoisted instruction, use as new src operand for all the shuffle ops in ShuffleGroup
                for( auto* waveShuffle : ShuffleOps )
                {
                    waveShuffle->setSrc( prev );
                }

                // Rewire all operations around the hoisted instructions
                // This means removing the hoisted instructions in the InstChains path
                // Done for all InstChains
                for (unsigned i = 0; i < InstChains.size(); i++ )
                {
                    int lastAnchorIdx = -1;
                    for (unsigned rewireIdx = 0; rewireIdx < HoistOrAnchorInstsIdx.size(); rewireIdx++ )
                    {
                        if( HoistOrAnchorInstsIdx[ rewireIdx ] )
                        {
                            // no-op for hoisted insts
                            continue;
                        }
                        else if( lastAnchorIdx + 1 == rewireIdx )
                        {
                            // already wired correctly, just increment
                            lastAnchorIdx++;
                        }
                        else
                        {
                            Instruction* lastAnchor = lastAnchorIdx == -1 ? cast<Instruction>(ShuffleOps[ i ]) : InstChains[ i ][ lastAnchorIdx ];
                            // operand to be replaced
                            Instruction* rewirePrev = InstChains[ i ][ rewireIdx - 1 ];
                            unsigned rewireOpIdx = InstChains[ i ][ rewireIdx ]->getOperand( 0 ) == rewirePrev ? 0 : 1;
                            InstChains[ i ][ rewireIdx ]->setOperand( rewireOpIdx, lastAnchor );
                            lastAnchorIdx = rewireIdx;
                        }
                    }

                    if( lastAnchorIdx != HoistOrAnchorInstsIdx.size() - 1 )
                    {
                        // one or more hoisted insts between last anchor and end of InstChain, one last rewire
                        Instruction* lastAnchor = lastAnchorIdx == -1 ? cast<Instruction>( ShuffleOps[ i ] ) : InstChains[ i ][ lastAnchorIdx ];
                        InstChains[ i ].back()->replaceAllUsesWith( lastAnchor );
                    }

                }

                for( auto& instChain : InstChains )
                {
                    for( unsigned i = 0; i < HoistOrAnchorInstsIdx.size(); i++ )
                    {
                        if( HoistOrAnchorInstsIdx[ i ] )
                            instChain[ i ]->eraseFromParent();
                    }
                }

                return true;
            }

            SmallVector<WaveShuffleIndexIntrinsic*> ShuffleOps; // all the WaveShuffleIndex instructions in the group
        private:
            BasicBlock* findCommonDominator( DominatorTree& DT )
            {
                BasicBlock* DomBB = ShuffleOps.front()->getParent();
                for ( auto& inst : ShuffleOps )
                {
                    BasicBlock* UseBB = inst->getParent();
                    DomBB = DT.findNearestCommonDominator( DomBB, UseBB );
                }

                return DomBB;
            }

            SmallVector<SmallVector<BinaryOperator*>> InstChains; // all common instructions shared by the shuffle ops, some can be hoisted
            SmallVector<bool> HoistOrAnchorInstsIdx; // Type of each Binary Operator in each InstChain: true - Hoistable/Hoistable past previous Anchors, false - Anchor
        }; //ShuffleGroup

    public:
        WaveShuffleIndexSinkingImpl( Function& F ) : F( F ) {}
        bool run();
    private:
        bool splitWaveShuffleIndexes();
        bool mergeWaveShuffleIndexes();
        bool moveToCommonDominator();
        void gatherShuffleGroups();
        bool sinkShuffleGroups();
        static unsigned compareWaveShuffleIndexes( WaveShuffleIndexIntrinsic* waveShuffleIndex, WaveShuffleIndexIntrinsic* newWaveShuffleIndex, SmallVector<BinaryOperator*>& InstChain, SmallVector<BinaryOperator*>& newInstChain, SmallVector<bool>& hoistOrAnchor );
        static bool isHoistable( BinaryOperator* inst );
        static bool isHoistableOverAnchor( BinaryOperator* instToHoist, BinaryOperator* anchorInst );
        Function& F;
        DominatorTree DT;
        DenseMap<BasicBlock*, SmallVector<Instruction*, 4>> MoveToCommonDominatorInstMap;
        DenseMap<Value*, SmallVector<ShuffleGroup, 4>> ShuffleGroupMap;
        DenseSet<WaveShuffleIndexIntrinsic*> Visited;
    };

    class WaveShuffleIndexSinking: public FunctionPass
    {
    public:
        static char ID;
        WaveShuffleIndexSinking() : FunctionPass( ID ) {}

        StringRef getPassName() const override
        {
            return "WaveShuffleIndexSinking";
        }

        bool runOnFunction( Function& F ) override;
    };

    FunctionPass* createWaveShuffleIndexSinking()
    {
        return new WaveShuffleIndexSinking();
    }
}

// Split any WaveShuffleIndex instructions that have more than one user
// This may uncover more hoisting opportunities
// If none of the instructions were able to be hoisted, the split instructions will be merged back together at the end
bool WaveShuffleIndexSinkingImpl::splitWaveShuffleIndexes()
{
    bool Changed = false;
    SmallVector<WaveShuffleIndexIntrinsic*> InstsToSplit;
    for( auto& BB : F )
    {
        for( auto& I : BB )
        {
            if( auto* waveShuffleInst = dyn_cast<WaveShuffleIndexIntrinsic>( &I ) )
            {
                if( auto* constantChannel = dyn_cast<ConstantInt>( waveShuffleInst->getChannel() ) )
                {
                    // Do not split WaveShuffleIndex insts that do not have a constant index since they cannot be optimized by this pass anyways
                    if( !waveShuffleInst->getUniqueUndroppableUser() )
                    {
                        // More than one user, split to potentially uncover more chances sink each individual WaveShuffleIndex
                        Changed = true;
                        InstsToSplit.push_back( waveShuffleInst );
                    }
                }
            }
        }
    }

    for( auto* instToSplit : InstsToSplit )
    {
        SmallVector<std::pair<Instruction*, Instruction*>> ReplacementPairs;
        // Multiple users, split instruction
        for( auto* user : instToSplit->users() )
        {
            auto* userInst = cast<Instruction>( user );
            auto* clonedWaveShuffleInst = instToSplit->clone();
            clonedWaveShuffleInst->setName( instToSplit->getName() + "_clone" );
            clonedWaveShuffleInst->insertBefore( instToSplit );
            // Track replacement to perform after loop since iterators will be messed up if performed mid loop
            ReplacementPairs.emplace_back(userInst, clonedWaveShuffleInst );
        }

        for( auto& p : ReplacementPairs )
        {
            p.first->replaceUsesOfWith( instToSplit, p.second );
        }

        // Each user is now using a cloned instruction, original should be safe to remove
        if( instToSplit->isSafeToRemove() )
        {
            instToSplit->eraseFromParent();
        }
    }

    return Changed;
}

bool WaveShuffleIndexSinkingImpl::moveToCommonDominator()
{
    // hoisted intruction needs to be moved to common dominator BB.
    // If instructions in shuffle group are from different basic blocks
    // there is a risk of non-dominating all users.
    bool Changed = false;
    for ( auto& bb : MoveToCommonDominatorInstMap )
    {
        auto instrInsertPtr = ( &*bb.first->getFirstInsertionPt() );
        for ( auto& inst : bb.second )
        {
            inst->moveBefore( instrInsertPtr );
            Changed = true;
        }
    }

    return Changed;
}

// Merge WaveShuffleIndex instructions that have the same source operand and the same constant lane/channel operand
bool WaveShuffleIndexSinkingImpl::mergeWaveShuffleIndexes()
{
    // Map from Source to (Map from Lane to list of duplicate instructions)
    DenseMap<Value*, DenseMap<ConstantInt*, SmallVector<WaveShuffleIndexIntrinsic*>>> mergeMap;
    for( auto& BB : F )
    {
        for( auto& I : BB )
        {
            if( auto* waveShuffleInst = dyn_cast<WaveShuffleIndexIntrinsic>( &I ) )
            {
                if( auto* constantChannel = dyn_cast<ConstantInt>( waveShuffleInst->getChannel() ) )
                {
                    mergeMap[ waveShuffleInst->getSrc() ][ constantChannel ].push_back( waveShuffleInst );
                }
            }
        }
    }

    bool Changed = false;
    for( auto& srcToLaneAndInstsMap : mergeMap )
    {
        for( auto& laneToInstsMap : srcToLaneAndInstsMap.second )
        {
            // Only 1 WaveShuffleIndex using the same src with the same constant channel index, nothing to merge
            auto& duplicateInsts = laneToInstsMap.second;
            if( duplicateInsts.size() < 2 )
                continue;
            Changed = true;
            auto* mainShuffleIndex = duplicateInsts.front();

            // Find common dominator for main WaveShuffleIndex
            bool moveToCommonDominator = false;
            BasicBlock* DomBB = mainShuffleIndex->getParent();

            for ( unsigned i = 1; i < duplicateInsts.size(); i++ )
            {
                BasicBlock* UseBB = duplicateInsts[ i ]->getParent();
                DomBB = DT.findNearestCommonDominator( DomBB, UseBB );
            }

            if ( !DomBB )
            {
                // Do not merge if Common Dominator is not found
                Changed = false;
                continue;
            }

            moveToCommonDominator = DomBB != mainShuffleIndex->getParent() ? true : false;

            // replace uses of other WaveShuffleIndex with the first one
            for( unsigned i = 1; i < duplicateInsts.size(); i++ )
            {
                duplicateInsts[ i ]->replaceAllUsesWith( mainShuffleIndex );
                duplicateInsts[ i ]->eraseFromParent();
            }

            if (moveToCommonDominator)
            {
                MoveToCommonDominatorInstMap[ DomBB ].emplace_back( mainShuffleIndex );
            }
        }
    }

    return Changed;
}

// Find WaveShuffleIndex instructions and group them together based on common successor instructions
void WaveShuffleIndexSinkingImpl::gatherShuffleGroups()
{
    for( auto& BB : F )
    {
        for( auto& I : BB )
        {
            if( auto* waveShuffleInst = dyn_cast<WaveShuffleIndexIntrinsic>( &I ) )
            {
                if( Visited.count( waveShuffleInst ) || !isa<ConstantInt>( waveShuffleInst->getChannel() ) )
                {
                    // Processed in prior iteration and nothing changed or does not have a constant channel
                    // Save compute and do not re-process/ create a new ShuffleGroup
                    continue;
                }
                if( ShuffleGroupMap.count( waveShuffleInst->getSrc() ) )
                {
                    // Found existing group(s) with the same source, try to match with one of the groups
                    bool match = false;
                    for( auto& shuffleGroup : ShuffleGroupMap[ waveShuffleInst->getSrc() ] )
                    {
                        if( shuffleGroup.match( waveShuffleInst ) )
                        {
                            match = true;
                            break;
                        }
                    }

                    // create new ShuffleGroup since no suitable match was found
                    if( !match )
                    {
                        ShuffleGroupMap[ waveShuffleInst->getSrc() ].emplace_back( waveShuffleInst );
                    }
                }
                else
                {
                    // create new ShuffleGroup for broadcast operations
                    ShuffleGroupMap[ waveShuffleInst->getSrc() ].emplace_back( waveShuffleInst );
                }
            }
        }
    }
}

// Run profitability function and decide whether to sink ShuffleGroups or not
bool WaveShuffleIndexSinkingImpl::sinkShuffleGroups()
{
    bool Changed = false;
    for( auto& kvp : ShuffleGroupMap )
    {
        for( auto& shuffleGroup : kvp.second )
        {
            unsigned numProfitableToHoist = shuffleGroup.preprocess();
            if( numProfitableToHoist > 0 )
            {
                // Pre-process found profitable instructions left to hoist
                Changed |= shuffleGroup.hoist( MoveToCommonDominatorInstMap, DT );
            }
            else
            {
                // No-op, mark all WaveShuffleInst in the current shuffle group as visited
                for( auto* waveShuffleInst : shuffleGroup.ShuffleOps )
                {
                    Visited.insert( waveShuffleInst );
                }
            }
        }
    }
    return Changed;
}

unsigned WaveShuffleIndexSinkingImpl::compareWaveShuffleIndexes( WaveShuffleIndexIntrinsic* waveShuffleIndex,
    WaveShuffleIndexIntrinsic* newWaveShuffleIndex, SmallVector<BinaryOperator*>& InstChain,
    SmallVector<BinaryOperator*>& NewInstChain, SmallVector<bool>& hoistOrAnchor )
{
    // Only search up to the number of existing instructions in InstChain, if it is prepopulated
    // InstChain will be pre-populated if newWaveShuffleIndex is being compared to a developed ShuffleGroup (two or more shuffle ops in group)
    std::optional<unsigned> limit;
    bool EmptyStartingInstChain = InstChain.empty();
    if( !EmptyStartingInstChain )
    {
        limit = InstChain.size();
    }
    Instruction* curInstA = waveShuffleIndex;
    Instruction* curInstB = newWaveShuffleIndex;
    unsigned idx = 0;
    unsigned numHoistable = 0;
    while( curInstA->hasOneUse() && curInstB->hasOneUse()  && ( !limit.has_value() || idx < limit ) )
    {
        // Only attempt to search past BinaryOperator for now
        auto* instA = dyn_cast<BinaryOperator>( curInstA->getUniqueUndroppableUser() );
        auto* instB = dyn_cast<BinaryOperator>( curInstB->getUniqueUndroppableUser() );
        if( !instA || !instB )
            break;

        if( !instA->isSameOperationAs( instB ) )
            break;

        // Check that both operands match
        auto* opA0 = instA->getOperand( 0 );
        auto* opA1 = instA->getOperand( 1 );
        auto* opB0 = instB->getOperand( 0 );
        auto* opB1 = instB->getOperand( 1 );

        if( instA->isCommutative() )
        {
            // covers all four cases
            // ex.
            // add i32 %ws1, %a  | add i32 %a, %ws1
            // ...               | ...
            // add i32 %ws2, %a  | add i32 %a, %ws2
            //-------------------|-----------------
            // add i32 %ws1, %a  | add i32 %a, %ws1
            // ...               | ...
            // add i32 %a, %ws2  | add i32 %ws2, %a
            if( !( opA0 == curInstA && opB0 == curInstB && opA1 == opB1 ) && !( opA0 == curInstA && opB1 == curInstB && opA1 == opB0 ) &&
                !( opA1 == curInstA && opB0 == curInstB && opA0 == opB1 ) && !( opA1 == curInstA && opB1 == curInstB && opA0 == opB0 ) )
                break;
        }
        else
        {
            // covers the 2 cases in row 1 above
            if( !( opA0 == curInstA && opB0 == curInstB && opA1 == opB1 ) && !( opA1 == curInstA && opB1 == curInstB && opA0 == opB0 ) )
                break;
        }

        if( isHoistable( instA ) )
        {
            bool canHoistPastAnchor = true;
            // start checking from last instruction
            for( int i = hoistOrAnchor.size()-1; i >= 0; i-- )
            {
                // hoistOrAnchor[i] is an anchor and cannot hoist instA over an anchor
                if( !hoistOrAnchor[i] && !isHoistableOverAnchor( instA, InstChain[ i ] ) )
                {
                    canHoistPastAnchor = false;
                }
            }

            if( canHoistPastAnchor )
            {
                numHoistable++;
                hoistOrAnchor.push_back( true );
            }
            else
            {
                hoistOrAnchor.push_back( false );
            }
        }
        else
        {
            hoistOrAnchor.push_back( false );
        }

        if( !limit.has_value() )
        {
            // Only update InstChain if it was a fresh vector
            InstChain.push_back( instA );
        }
        NewInstChain.push_back( instB );
        curInstA = instA;
        curInstB = instB;
        idx++;
    }

    return numHoistable;
}

bool WaveShuffleIndexSinkingImpl::isHoistable( BinaryOperator* inst )
{
    // One operand has to be a constant, representing uniformity and allowing the operation to be performed on all simd lanes prior to broadcast operation
    return isa<ConstantInt>( inst->getOperand( 0 ) ) || isa<ConstantFP>( inst->getOperand( 0 ) ) ||
        isa<ConstantInt>( inst->getOperand( 1 ) ) || isa<ConstantFP>( inst->getOperand( 1 ) );
}

// Combination of leftDistributesOverRight and rightDistributesOverLeft from LLVM InstCombining.cpp
bool WaveShuffleIndexSinkingImpl::isHoistableOverAnchor( BinaryOperator* instToHoist, BinaryOperator* anchorInst )
{
    if( instToHoist->isCommutative() )
    {
        Instruction::BinaryOps FirstOp = anchorInst->getOpcode();
        Instruction::BinaryOps SecondOp = instToHoist->getOpcode();

        // X & (Y | Z) <--> (X & Y) | (X & Z)
        // X & (Y ^ Z) <--> (X & Y) ^ (X & Z)
        // In practice, FirstOp is unlikely to be And, Or, or Xor as they would themselves be hoistable and thus, never an anchor inst
        if( SecondOp == Instruction::And )
            return FirstOp == Instruction::Or || FirstOp == Instruction::Xor;

        // X | (Y & Z) <--> (X | Y) & (X | Z)
        if( SecondOp == Instruction::Or )
            return FirstOp == Instruction::And;

        // X * (Y + Z) <--> (X * Y) + (X * Z)
        // X * (Y - Z) <--> (X * Y) - (X * Z)
        if( SecondOp == Instruction::Mul )
            return FirstOp == Instruction::Add || FirstOp == Instruction::Sub;

        return false;
    }
    else
    {
        return anchorInst->isBitwiseLogicOp() || instToHoist->isShift();
    }
}

bool WaveShuffleIndexSinkingImpl::run()
{
    DT.recalculate(F);
    bool Changed = splitWaveShuffleIndexes();

    unsigned numIters = 0;
    while( numIters < IGC_GET_FLAG_VALUE( WaveShuffleIndexSinkingMaxIterations ) )
    {
        gatherShuffleGroups();
        if( sinkShuffleGroups() )
        {
            Changed = true;
        }
        else
        {
            break;
        }

        numIters++;
        ShuffleGroupMap.clear();
    }
    Changed |= mergeWaveShuffleIndexes();
    Changed |= moveToCommonDominator();
    return Changed;
}


bool WaveShuffleIndexSinking::runOnFunction( Function& F )
{
    WaveShuffleIndexSinkingImpl WorkerInstance( F );
    return WorkerInstance.run();
}

char WaveShuffleIndexSinking::ID = 0;

#define PASS_FLAG "igc-wave-shuffle-index-sinking"
#define PASS_DESCRIPTION "WaveShuffleIndexSinking"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN( WaveShuffleIndexSinking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS )
IGC_INITIALIZE_PASS_END( WaveShuffleIndexSinking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS )