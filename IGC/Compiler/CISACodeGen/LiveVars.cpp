/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

//===------------ LiveVars.cpp - Live Variable Analysis ---------*- C++ -*-===//
//
// Intel extension to LLVM core
//
//===----------------------------------------------------------------------===//
//
// This file implements the LiveVariables analysis pass. It originates from
// the same function in llvm3.0/codegen, however works on llvm-ir instead of
// the code-gen-level ir.
//
// This class computes live variables using a sparse implementation based on
// the llvm-ir SSA form.  This class uses the dominance properties of SSA form
// to efficiently compute live variables for virtual registers. Also Note that
// there is no physical register at llvm-ir level.
//
//===----------------------------------------------------------------------===//

#include "Compiler/CISACodeGen/LiveVars.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/debug/Debug.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/CFG.h"
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Debug.h>
#include <llvm/ADT/DepthFirstIterator.h>
#include "common/LLVMWarningsPop.hpp"
#include <algorithm>
#include "Probe/Assertion.h"
#include "helper.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::Debug;

Instruction*
LiveVars::LVInfo::findKill(const BasicBlock* MBB) const {
    for (unsigned i = 0, e = Kills.size(); i != e; ++i)
        if (Kills[i]->getParent() == MBB)
            return Kills[i];
    return NULL;
}

void LiveVars::LVInfo::print(raw_ostream& OS) const {
    OS << "  Alive in blocks: ";
    for (auto I = AliveBlocks.begin(), E = AliveBlocks.end(); I != E; ++I) {
        (*I)->print(OS);
        OS << ", ";
    }
    OS << "\n  Killed by:";
    if (Kills.empty())
        OS << " No instructions.\n";
    else {
        for (unsigned i = 0, e = Kills.size(); i != e; ++i) {
            OS << "\n    " << ": " << *Kills[i];
        }
        OS << "\n";
    }
}

void LiveVars::print(raw_ostream& OS, const Module*) const {
    for (DenseMap<Value*, LiveVars::LVInfo*>::const_iterator I = VirtRegInfo.begin(),
        E = VirtRegInfo.end(); I != E; ++I) {
        OS << "\n{";
        I->first->print(OS);
        I->second->print(OS);
        OS << "}";
    }
}

/// Destructor needs to release the allocated memory
LiveVars::~LiveVars()
{
    releaseMemory();
}

void LiveVars::releaseMemory() {
    // Free the LVInfo table
    VirtRegInfo.clear();
    PHIVarInfo.clear();
    DistanceMap.clear();
}

void LiveVars::preAllocMemory(Function& F)
{
    // Pre-allocate enough memory to avoid many allocations
    // Use the number of values (arg_size + the number of insts)
    // as the amount for pre-allocation. This size is super set
    // for both VirtRegInfo and DistanceMap. (This amount is about
    // 3%-5% more than the real size of VirtRegInfo.)
    size_t nVals = F.arg_size();
    for (auto& BB : F) {
        nVals += BB.size();
    }

    // Allocate a bit more than we need in case it needs grow.
    // Note that as DenseMap will automatically allocate when
    // the map is 3/4 full, so we take this into account.
    uint32_t mapCap1 = int_cast<uint32_t>((size_t)(nVals * 1.40f));
    // For PHIVarInfo, increase 10% only.
    uint32_t mapCap2 = int_cast<uint32_t>((size_t)(F.size() * 1.10f));
    DistanceMap.grow(mapCap1);
    VirtRegInfo.grow(mapCap1);
    PHIVarInfo.grow(mapCap2);
}

void LiveVars::dump() const {
    print(ods());
}

#if  defined( _DEBUG )
void LiveVars::dump(Function* F)
{
    if (F->size() == 0)
    {
        return;
    }

    if (BBIds.size() == 0)
    {
        setupBBIds(F);
    }

    raw_ostream& OS = errs();
    for (DenseMap<Value*, LiveVars::LVInfo*>::const_iterator I = VirtRegInfo.begin(),
        E = VirtRegInfo.end(); I != E; ++I) {
        Value* V = I->first;
        LVInfo* LVI = I->second;
        OS << "\n{";
        V->print(OS);
        OS << "\n  Alive in blocks: ";
        for (auto I = LVI->AliveBlocks.begin(), E = LVI->AliveBlocks.end();
            I != E; ++I) {
            BasicBlock* BB = *I;
            if (BBIds.count(BB) > 0)
            {
                int id = BBIds[BB];
                OS << id << ", ";
            }
            else
            {
                OS << "<unknown>, ";
            }
        }
        OS << "\n  Killed by:";
        if (LVI->Kills.empty())
            OS << " No instructions.\n";
        else {
            for (unsigned i = 0, e = LVI->Kills.size(); i != e; ++i) {
                OS << "\n    " << ": " << *LVI->Kills[i];
            }
            OS << "\n";
        }
        OS << "}";
    }
}

void LiveVars::dump(int BBId)
{
    BasicBlock* BB = IdToBBs[BBId];
    if (BB)
    {
        BB->print(ods());
    }
}

void LiveVars::setupBBIds(Function* F)
{
    if (F)
    {
        int ix = 0;
        for (Function::iterator I = F->begin(), E = F->end();
            I != E; ++I)
        {
            BasicBlock* BB = &*I;
            BBIds.insert(std::make_pair(BB, ix));
            IdToBBs.insert(std::make_pair(ix, BB));
            ++ix;
        }
    }
}
#endif

/// getLVInfo - Get (possibly creating) a LVInfo object for the given vreg.
LiveVars::LVInfo& LiveVars::getLVInfo(Value* LV) {
    IGC_ASSERT(isa<Instruction>(LV) || isa<Argument>(LV));
    DenseMap<Value*, LiveVars::LVInfo*>::const_iterator it = VirtRegInfo.find(LV);
    if (it == VirtRegInfo.end()) {
        LiveVars::LVInfo* lvInfo = new (Allocator.Allocate()) LiveVars::LVInfo();
        lvInfo->uniform = (WIA && WIA->isUniform(LV));
        VirtRegInfo.insert(std::pair<Value*, LVInfo*>(LV, lvInfo));
        if (Instruction * inst = dyn_cast<Instruction>(LV)) {
            // if the value is an instruction, default it to dead
            lvInfo->Kills.push_back(inst);
        }
        return *(lvInfo);
    }
    return *(it->second);
}

void LiveVars::MarkVirtRegAliveInBlock(LiveVars::LVInfo& VRInfo,
    BasicBlock* DefBlock,
    BasicBlock* MBB,
    std::vector<BasicBlock*>& WorkList) {

    if (VRInfo.AliveBlocks.count(MBB))
        return;  // We already know the block is live

    // Check to see if this basic block is one of the killing blocks.  If so,
    // remove it.
    for (unsigned i = 0, e = VRInfo.Kills.size(); i != e; ++i)
        if (VRInfo.Kills[i]->getParent() == MBB) {
            VRInfo.Kills.erase(VRInfo.Kills.begin() + i);  // Erase entry
            break;
        }

    if (MBB == &(MF->getEntryBlock()) && DefBlock != nullptr)
        return;  // Only Arguments can be live-through entry block
    if (MBB == DefBlock)
        return;  // Terminate recursion

      // Mark the variable known alive in this bb
    VRInfo.AliveBlocks.insert(MBB);

    // Skip the simdPred if WIA is not available
    if (!WIA)
    {
        return;
    }

    bool hasNonUniformBranch = false;
    bool hasLayoutPred = true;
    for (pred_iterator PI = pred_begin(MBB), E = pred_end(MBB);
        PI != E; ++PI) {
        BasicBlock* PredBlk = *PI;
        //Before pushing check if the predecessor has already been marked
        if (VRInfo.AliveBlocks.count(PredBlk))
            continue;  // We already know the block is live

        WorkList.push_back(PredBlk);
        // Check if we need to update liveness for uniform variable
        // inside divergent control-flow
        if (PredBlk == MBB->getPrevNode())
            hasLayoutPred = false;
        if (hasLayoutPred && VRInfo.uniform) {
            Instruction* cbr = PredBlk->getTerminator();
            if (cbr && !WIA->isUniform(cbr))
                hasNonUniformBranch = true;
        }
    }
    if (hasLayoutPred && hasNonUniformBranch && VRInfo.uniform) {
        BasicBlock* simdPred = MBB->getPrevNode();
        while (simdPred && simdPred != DefBlock) {
            //check if it is marked live
            if (!VRInfo.AliveBlocks.count(simdPred))
                WorkList.push_back(simdPred);
            simdPred = simdPred->getPrevNode();
        }
    }
}

void LiveVars::MarkVirtRegAliveInBlock(LiveVars::LVInfo& VRInfo,
    BasicBlock* DefBlock,
    BasicBlock* MBB) {
    std::vector<BasicBlock*> WorkList;
    MarkVirtRegAliveInBlock(VRInfo, DefBlock, MBB, WorkList);
    while (!WorkList.empty()) {
        BasicBlock* Pred = WorkList.back();
        WorkList.pop_back();
        MarkVirtRegAliveInBlock(VRInfo, DefBlock, Pred, WorkList);
    }
}

void LiveVars::HandleVirtRegUse(Value* VL, BasicBlock* MBB,
    Instruction* MI, bool ScanAllKills,
    bool ScanBBTopDown)
{
    LiveVars::LVInfo& VRInfo = getLVInfo(VL);
    VRInfo.NumUses++;

    // Check to see if this basic block is already a kill block.
    // - When we establish live-info for the first time, the uses are scanned in bottom-up fashion
    //   for every basic block, the kill for the current block is appended to
    //   the end of the kill-list, therefore we only need to check the last kill on the kill-list.
    // - When we update the live-info later on in top-down fashion, for example, when coalescing InsertElements
    //   in DeSSA, the existing kill for this block may be anywhere on the list, then we need to
    //   scan all the kills in order to replace the right one.
    if (ScanAllKills) {
        for (unsigned i = 0, e = VRInfo.Kills.size(); i != e; ++i) {
            if (VRInfo.Kills[i]->getParent() == MBB) {
                Instruction* killInst = VRInfo.Kills[i];
                IGC_ASSERT_MESSAGE(DistanceMap.count(killInst), "DistanceMap not set up yet.");
                IGC_ASSERT_MESSAGE(DistanceMap.count(MI), "DistanceMap not set up yet.");
                if (DistanceMap[killInst] < DistanceMap[MI]) {
                    VRInfo.Kills[i] = MI;
                }
                return;
            }
        }
    }
    else {
        if (!VRInfo.Kills.empty() && VRInfo.Kills.back()->getParent() == MBB) {
            if (ScanBBTopDown)
            {
                VRInfo.Kills.back() = MI;
            }
            else
            {
                // Initially the kill is the def itself.
                // replace it with the kill, otherwise, just ignore the new use because it is not the last.
                if (VRInfo.Kills.back() == VL) {
                    VRInfo.Kills.back() = MI;
                }
            }
            return;
        }

        for(size_t i = 0, e = VRInfo.Kills.size(); i < e; ++i)
        {
            IGC_ASSERT(nullptr != VRInfo.Kills[i]);
            IGC_ASSERT_MESSAGE((VRInfo.Kills[i]->getParent() != MBB), "entry should be at end!");
        }
    }

    // This situation can occur:
    //
    //     ,------.
    //     |      |
    //     |      v
    //     |   t2 = phi ... t1 ...
    //     |      |
    //     |      v
    //     |   t1 = ...
    //     |  ... = ... t1 ...
    //     |      |
    //     `------'
    //
    // where there is a use in a PHI node that's a predecessor to the defining
    // block. We don't want to mark all predecessors as having the value "alive"
    // in this case.
    if (isa<Instruction>(VL)) {
        if (MBB == cast<Instruction>(VL)->getParent())
            return;
    }
    // Add a new kill entry for this basic block. If this virtual register is
    // already marked as alive in this basic block, that means it is alive in at
    // least one of the successor blocks, it's not a kill.
    if (!VRInfo.AliveBlocks.count(MBB))
        VRInfo.Kills.push_back(MI);

    if (MBB == &(MF->getEntryBlock()))
        return;

    if (!WIA)
        return;

    // Update all dominating blocks to mark them as "known live".
    bool hasNonUniformBranch = false;
    bool hasLayoutPred = true;
    for (pred_iterator PI = pred_begin(MBB), E = pred_end(MBB);
        PI != E; ++PI) {
        BasicBlock* DefBlk = (isa<Instruction>(VL)) ?
            cast<Instruction>(VL)->getParent() : NULL;
        BasicBlock* PredBlk = *PI;
        MarkVirtRegAliveInBlock(VRInfo, DefBlk, PredBlk);
        Instruction* cbr = PredBlk->getTerminator();
        if (cbr && !WIA->isUniform(cbr))
            hasNonUniformBranch = true;
        if (PredBlk == MBB->getPrevNode())
            hasLayoutPred = false;
    }
    if (hasLayoutPred && hasNonUniformBranch && WIA->isUniform(VL)) {
        BasicBlock* simdPred = MBB->getPrevNode();
        BasicBlock* DefBlk = (isa<Instruction>(VL)) ?
            cast<Instruction>(VL)->getParent() : NULL;
        while (simdPred && simdPred != DefBlk) {
            MarkVirtRegAliveInBlock(VRInfo, DefBlk, simdPred);
            simdPred = simdPred->getPrevNode();
        }
    }
}

void LiveVars::HandleVirtRegDef(Instruction* MI)
{
    LiveVars::LVInfo& VRInfo = getLVInfo(MI);
    if (VRInfo.AliveBlocks.empty())
        // If vr is not alive in any block, then defaults to dead.
        VRInfo.Kills.push_back(MI);
}

void LiveVars::initDistance(Function& F)
{
    DistanceMap.clear();

    for (auto& BB : F)
    {
        unsigned Dist = 0;
        for (auto& II : BB) {
            Instruction* MI = &II;
            if (isDbgIntrinsic(MI)) {
                continue;
            }
            DistanceMap.insert(std::make_pair(MI, Dist++));
        }
    }
}

void LiveVars::ComputeLiveness(Function* mf, WIAnalysis* wia)
{
    releaseMemory();
    MF = mf;
    WIA = wia;

    preAllocMemory(*MF);

    // First, set up DistanceMap
    // save distance map
    initDistance(*MF);

    analyzePHINodes(*mf);
    BasicBlock* Entry = &(*MF->begin());
    typedef df_iterator_default_set<BasicBlock*, 16> VisitedTy;
    VisitedTy Visited;

    for (df_ext_iterator<BasicBlock*, VisitedTy>
        DFI = df_ext_begin(Entry, Visited), E = df_ext_end(Entry, Visited);
        DFI != E; ++DFI) {
        BasicBlock* MBB = *DFI;
        // Handle any virtual assignments from PHI nodes which might be at the
        // bottom of this basic block.  We check all of our successor blocks to see
        // if they have PHI nodes, and if so, we simulate an assignment at the end
        // of the current block.
        auto it = PHIVarInfo.find(MBB);
        if (it != PHIVarInfo.end()) {
            SmallVector<Value*, 4> & VarInfoVec = it->second;

            for (SmallVector<Value*, 4>::iterator I = VarInfoVec.begin(),
                E = VarInfoVec.end(); I != E; ++I) {
                // Mark it alive only in the block we are representing.
                Value* DefV = *I;
                BasicBlock* DefBlk = (isa<Instruction>(DefV)) ?
                    cast<Instruction>(DefV)->getParent() : NULL;
                MarkVirtRegAliveInBlock(getLVInfo(DefV), DefBlk, MBB);
            }
        }
    }
}

/// analyzePHINodes - Gather information about the PHI nodes in here. In
/// particular, we want to map the variable information of a virtual register
/// which is used in a PHI node. We map that to the BB the vreg is coming from.
///
void LiveVars::analyzePHINodes(const Function& Fn) {
    for (Function::const_iterator I = Fn.begin(), E = Fn.end();
        I != E; ++I) {
        for (BasicBlock::const_iterator BBI = I->begin(), BBE = I->end();
            BBI != BBE; ++BBI) {
            const PHINode* phi = dyn_cast<PHINode>(BBI);
            if (!phi)
                break;
            for (unsigned i = 0, e = phi->getNumOperands(); i != e; ++i) {
                BasicBlock* PBB = phi->getIncomingBlock(i);
                Value* VL = phi->getOperand(i);
                if (isa<Instruction>(VL) || isa<Argument>(VL)) {
                    SmallVector<Value*, 4> & VV = PHIVarInfo[PBB];
                    VV.push_back(phi->getOperand(i));
                }
            }
        }
    }
}

bool LiveVars::LVInfo::isLiveIn(const BasicBlock& MBB, Value* VL) {

    // Reg is live-through.
    if (AliveBlocks.count((BasicBlock*)& MBB))
        return true;

    // Registers defined in MBB cannot be live in.
    const Instruction* Def = dyn_cast<Instruction>(VL);
    if (Def && Def->getParent() == &MBB)
        return false;

    // Reg was not defined in MBB, was it killed here?
    if (findKill(&MBB))
        return true;
    return false;
}

bool LiveVars::isLiveAt(Value* VL, Instruction* MI) {
    BasicBlock* MBB = MI->getParent();
    LVInfo& info = getLVInfo(VL);
    // Reg is live-through.
    if (info.AliveBlocks.count(MBB))
        return true;

    // Registers defined in MBB cannot be live in.
    const Instruction* Def = dyn_cast<Instruction>(VL);
    if (Def && Def->getParent() == MBB) {
        if (getDistance(Def) > getDistance(MI)) {
            return false;
        }
        else if (info.AliveBlocks.empty() && info.Kills.empty()) {
            // handle that special case: def is the current block
            // and phi in the immed-successor block is the last use
            return true;
        }
    }

    // Reg was not defined in MBB, was it killed here?
    Instruction* kill = info.findKill(MBB);
    if (kill) {
        return getDistance(kill) > getDistance(MI);
    }

    if (isLiveOut(VL, *MBB)) {
        return true;
    }

    return false;
}

bool LiveVars::isLiveOut(Value* VL, const BasicBlock& MBB) {
    LiveVars::LVInfo& VI = getLVInfo(VL);

    if (isa<Instruction>(VL) && VI.AliveBlocks.empty())
    {
        // a local value does not live out of any BB.
        // (Originally, this function might be for checking non-local
        //  value. Adding this code to make it work for any value.)
        Instruction* I = cast<Instruction>(VL);
        if (VI.Kills.size() == 1) {
            BasicBlock* killBB = VI.Kills[0]->getParent();
            if (killBB == I->getParent()) {
                return false;
            }
        }
    }

    // Loop over all of the successors of the basic block, checking to see if
    // the value is either live in the block, or if it is killed in the block.
    SmallVector<const BasicBlock*, 8> OpSuccBlocks;
    for (IGCLLVM::const_succ_iterator SI = succ_begin(&MBB), E = succ_end(&MBB); SI != E; ++SI) {
        const BasicBlock* SuccMBB = *SI;
        // Is it alive in this successor?
        if (VI.AliveBlocks.count((BasicBlock*)SuccMBB))
            return true;
        OpSuccBlocks.push_back(SuccMBB);
    }

    // Check to see if this value is live because there is a use in a successor
    // that kills it.
    switch (OpSuccBlocks.size()) {
    case 1: {
        const BasicBlock* SuccMBB = OpSuccBlocks[0];
        for (unsigned i = 0, e = VI.Kills.size(); i != e; ++i)
            if (VI.Kills[i]->getParent() == SuccMBB)
                return true;
        break;
    }
    case 2: {
        const BasicBlock* SuccMBB1 = OpSuccBlocks[0], * SuccMBB2 = OpSuccBlocks[1];
        for (unsigned i = 0, e = VI.Kills.size(); i != e; ++i)
            if (VI.Kills[i]->getParent() == SuccMBB1 ||
                VI.Kills[i]->getParent() == SuccMBB2)
                return true;
        break;
    }
    default:
        std::sort(OpSuccBlocks.begin(), OpSuccBlocks.end());
        for (unsigned i = 0, e = VI.Kills.size(); i != e; ++i)
            if (std::binary_search(OpSuccBlocks.begin(), OpSuccBlocks.end(),
                VI.Kills[i]->getParent()))
                return true;
    }
    return false;
}

void LiveVars::Calculate(Function* mf, WIAnalysis* wia)
{
    releaseMemory();
    MF = mf;
    WIA = wia;

    preAllocMemory(*MF);

    initDistance(*MF);

    analyzePHINodes(*mf);

    BasicBlock* Entry = &(*MF->begin());
    typedef df_iterator_default_set<BasicBlock*, 16> VisitedTy;
    VisitedTy Visited;
    for (df_ext_iterator<BasicBlock*, VisitedTy>
        DFI = df_ext_begin(Entry, Visited), DFE = df_ext_end(Entry, Visited);
        DFI != DFE; ++DFI)
    {
        BasicBlock* MBB = *DFI;

        // Loop over all of the instructions, processing them.
        for (BasicBlock::iterator I = MBB->begin(), E = MBB->end();
            I != E; ++I)
        {
            Instruction* MI = &(*I);

            // Unless it is a PHI node.  In this case, ONLY process the DEF, not any
            // of the uses.  They will be handled in other basic blocks.
            if (!isa<PHINode>(MI)) {
                // Process all uses.
                for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
                    Value* V = MI->getOperand(i);
                    if (isa<Instruction>(V) || isa<Argument>(V))
                    {
                        HandleVirtRegUse(V, MBB, MI, false, true);
                    }
                }
            }

            // Since we will handle a value only if it is used, no need to explicitly
            // handle def here. And getLVInfo() will handle the def when handling its
            // first use.  So as a result commenting the following out, no LV is created
            // for a dead instruction, which makes sense. Note that this is different
            // from the llvm core, as llvm core getVarInfo is different from getLVInfo()!
#if 0
            // Process all defs.
            if (!MI->getType()->isVoidTy())
            {
                HandleVirtRegDef(MI);
            }
#endif
        }

        // Handle any virtual assignments from PHI nodes which might be at the
        // bottom of this basic block.  We check all of our successor blocks to see
        // if they have PHI nodes, and if so, we simulate an assignment at the end
        // of the current block.
        auto it = PHIVarInfo.find(MBB);
        if (it != PHIVarInfo.end()) {
            SmallVector<Value*, 4> & VarInfoVec = it->second;

            for (SmallVector<Value*, 4>::iterator I = VarInfoVec.begin(),
                E = VarInfoVec.end(); I != E; ++I) {
                // Mark it alive only in the block we are representing.
                Value* DefV = *I;
                BasicBlock* DefBlk = (isa<Instruction>(DefV)) ?
                    cast<Instruction>(DefV)->getParent() : NULL;
                MarkVirtRegAliveInBlock(getLVInfo(DefV), DefBlk, MBB);
#if VECTOR_COALESCING == 0
                // special treatment for ExtractElement
                Instruction * EEI = dyn_cast<ExtractElementInst>(DefV);
                if (EEI) {
                    DefV = EEI->getOperand(0);
                    if (isa<Instruction>(DefV) || isa<Argument>(DefV))
                    {
                        DefBlk = (isa<Instruction>(DefV)) ?
                            cast<Instruction>(DefV)->getParent() : NULL;
                        MarkVirtRegAliveInBlock(getLVInfo(DefV), DefBlk, MBB);
                    }
                }
#endif
            }
        }
    }
}

bool LiveVars::hasInterference(llvm::Value* V0, llvm::Value* V1)
{
    // Skip Constant
    if (isa<Constant>(V0) || isa<Constant>(V1)) {
        return false;
    }

    Instruction* I0 = dyn_cast<Instruction>(V0);
    Instruction* I1 = dyn_cast<Instruction>(V1);
    if (!I0 && !I1) {
        return true;
    }

    if (!I0) {
        // V0 must be argument. Use the first inst in Entry
        I0 = MF->getEntryBlock().getFirstNonPHIOrDbg();
    }
    if (!I1) {
        // V1 must be argument. Use the first inst in Entry
        I1 = MF->getEntryBlock().getFirstNonPHIOrDbg();
    }

    if (isLiveAt(V0, I1) || isLiveAt(V1, I0)) {
        return true;
    }

    return false;
}

// Merge LVInfo for "fromV" into V's.
//
// This function is used after LVInfo has been constructed already and
// we want to merge "fromV" into V. This function will have V's LVInfo
// updated to reflect such a merging.
void LiveVars::mergeUseFrom(Value* V, Value* fromV)
{
    // Normally, both VitrRegInfo.count(V) and VirtRegInfo.count(fromV) are
    // non-zero, meaning their LVInfo have been constructed (they are not
    // dead). However, we see inline asm case in which fromV is dead in term
    // of llvm value usage and therefore no LVInfo (as inline asm call has
    // side-effect, the inline asm is not dead). For this reason, we relax
    // assertion to require at least one has LVInfo.
    IGC_ASSERT_MESSAGE(VirtRegInfo.count(V) || VirtRegInfo.count(fromV),
        "MergeUseFrom should be used after LVInfo have been constructed!");

    LVInfo& LVI = getLVInfo(V);
    LVInfo& fromLVI = getLVInfo(fromV);
    uint32_t newNumUses = LVI.NumUses + fromLVI.NumUses;

    IGC_ASSERT_MESSAGE(LVI.uniform == fromLVI.uniform, "ICE: cannot merge uniform with non-uniform values!");

    // Use V's defining BB, not fromV's
    Instruction* defInst = dyn_cast<Instruction>(V);
    BasicBlock* defBB = defInst ? defInst->getParent() : nullptr;

    // For each AliveBlock of fromV, add it to V's
    for (auto I : fromLVI.AliveBlocks) {
        BasicBlock* BB = I;
        MarkVirtRegAliveInBlock(LVI, defBB, BB);
    }

    // For each kill, add it into V's LVInfo
    for (auto KI : fromLVI.Kills) {
        Instruction* inst = KI;
        BasicBlock* instBB = inst->getParent();

        // Must set ScanAllKills as we are doing updating.
        HandleVirtRegUse(V, instBB, inst, true);
    }

    // Special case. fromV is used in the phi.
    //
    //    If a value is defined in BB and its last use is in
    //    a PHI in succ(BB), both its AliveBlocks and Kills
    //    are empty (see LiveVars.hpp)
    // For example:
    //       BB:
    //           fromV = bitcast V
    //       succ_BB:
    //           phi = fromV
    //
    // In this case, we will need to make sure BB is marked as alive.
    if (Instruction * I = dyn_cast<Instruction>(fromV))
    {
        for (User* user : I->users())
        {
            if (dyn_cast<PHINode>(user))
            {
                BasicBlock* BB = I->getParent();
                MarkVirtRegAliveInBlock(LVI, defBB, BB);
                break;
            }
        }
    }

    LVI.NumUses = newNumUses;
}


IGC_INITIALIZE_PASS_BEGIN(LiveVarsAnalysis, "LiveVarsAnalysis", "LiveVarsAnalysis", false, true)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(LiveVarsAnalysis, "LiveVarsAnalysis", "LiveVarsAnalysis", false, true)

char LiveVarsAnalysis::ID = 0;

LiveVarsAnalysis::LiveVarsAnalysis() : FunctionPass(ID) {
    initializeLiveVarsAnalysisPass(*PassRegistry::getPassRegistry());
}

bool LiveVarsAnalysis::runOnFunction(Function& F) {
    // WIAnalysis skips functions that are not recorded.
    auto pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo()) {
        return false;
    }

    auto WIA = getAnalysisIfAvailable<WIAnalysis>();
    LV.ComputeLiveness(&F, WIA);
    return false;
}
