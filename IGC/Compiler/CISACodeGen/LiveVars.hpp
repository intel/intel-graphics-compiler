/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

//===------------ LiveVars.h - Live Variable Analysis -----------*- C++ -*-===//
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

#ifndef LLVM_IEXT_LIVEVARS_H
#define LLVM_IEXT_LIVEVARS_H

#include "Compiler/CISACodeGen/WIAnalysis.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CFG.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Allocator.h>
#include "common/LLVMWarningsPop.hpp"

#include <map>

namespace IGC
{

    class LiveVars {
    public:
        // forward declaration
        struct LVInfo;

        typedef llvm::DenseMap<llvm::Value*, LVInfo*>  LVInfoMap;
        typedef LVInfoMap::iterator        iterator;
        typedef LVInfoMap::const_iterator  const_iterator;

        LiveVars() {}
        ~LiveVars();
        LiveVars(const LiveVars&) = delete;
        LiveVars& operator=(const LiveVars&) = delete;

        /// LVInfo - This represents the regions where a virtual register is live in
        /// the program.  We represent this with three different pieces of
        /// information: the set of blocks in which the instruction is live
        /// throughout, the set of blocks in which the instruction is actually used,
        /// and the set of non-phi instructions that are the last users of the value.
        ///
        /// In the common case where a value is defined and killed in the same block,
        /// There is one killing instruction, and AliveBlocks is empty.
        ///
        /// Otherwise, the value is live out of the block.  If the value is live
        /// throughout any blocks, these blocks are listed in AliveBlocks.  Blocks
        /// where the liveness range ends are not included in AliveBlocks, instead
        /// being captured by the Kills set.  In these blocks, the value is live into
        /// the block (unless the value is defined and killed in the same block) and
        /// lives until the specified instruction.  Note that there cannot ever be a
        /// value whose Kills set contains two instructions from the same basic block.
        ///
        /// PHI nodes complicate things a bit.  If a PHI node is the last user of a
        /// value in one of its predecessor blocks, it is not listed in the kills set,
        /// but does include the predecessor block in the AliveBlocks set (unless that
        /// block also defines the value).  This leads to the (perfectly sensical)
        /// situation where a value is defined in a block, and the last use is a phi
        /// node in the successor.  In this case, AliveBlocks is empty (the value is
        /// not live across any  blocks) and Kills is empty (phi nodes are not
        /// included). This is sensical because the value must be live to the end of
        /// the block, but is not live in any successor blocks.
        struct LVInfo {
            /// AliveBlocks - Set of blocks in which this value is alive completely
            /// through.
            llvm::SmallPtrSet<llvm::BasicBlock*, 16> AliveBlocks;
            ///std::set<llvm::BasicBlock*> AliveBlocks;

            /// NumUses - Number of uses of this register across the entire function.
            ///
            unsigned NumUses;

            /// Kills - List of llvm::Instruction's which are the last use of this
            /// virtual register (kill it) in their basic block.
            ///
            std::vector<llvm::Instruction*> Kills;

            bool uniform;

            LVInfo() : NumUses(0), uniform(false) {}

            /// removeKill - Delete a kill corresponding to the specified
            /// instruction. Returns true if there was a kill
            /// corresponding to this instruction, false otherwise.
            bool removeKill(llvm::Instruction* MI) {
                std::vector<llvm::Instruction*>::iterator
                    I = std::find(Kills.begin(), Kills.end(), MI);
                if (I == Kills.end())
                    return false;
                Kills.erase(I);
                return true;
            }

            /// findKill - Find a kill instruction in basic block. Return NULL if none is found.
            llvm::Instruction* findKill(const llvm::BasicBlock* MBB) const;

            /// isLiveIn - Is Reg live in to MBB? This means that Reg is live through
            /// MBB, or it is killed in BB. If Reg is only used by PHI instructions in
            /// MBB, it is not considered live in.
            bool isLiveIn(const llvm::BasicBlock& MBB, llvm::Value* LV);

            void print(llvm::raw_ostream& OS) const;
        }; // end of LVInfo

    private:
        /// VirtRegInfo - This list is a mapping from a llvm::value to
        /// its liveness information.
        ///
        LVInfoMap VirtRegInfo;

    private:   // Intermediate data structures
        llvm::Function* MF;
        WIAnalysis* WIA;
        llvm::SpecificBumpPtrAllocator<LVInfo> Allocator;

        // For each basic-block, we have a vector of phi-uses if there are phi-insts
        // in a successor-block
        llvm::DenseMap<llvm::BasicBlock*, llvm::SmallVector<llvm::Value*, 4>> PHIVarInfo;

        // DistanceMap - Keep track the distance of an Instr from the start of the
        // current basic block.
        llvm::DenseMap<llvm::Instruction*, unsigned> DistanceMap;

        /// analyzePHINodes - Gather information about the PHI nodes in here. In
        /// particular, we want to map the variable information of a virtual
        /// register which is used in a PHI node. We map that to the BB the vreg
        /// is coming from.
        void analyzePHINodes(const llvm::Function& MF);

        /// Initialize DistanceMap
        void initDistance(llvm::Function& F);

    public:

        /// Can be called to release memory when the object won't be used anymore.
        void releaseMemory();

        /// Pre-allocate memory for llvm::DenseMap to avoid many small allocations
        void preAllocMemory(llvm::Function& F);

        /// getLVInfo - Return the LVInfo structure for the specified VIRTUAL
        /// register.
        LVInfo& getLVInfo(llvm::Value* LV);

        /// Get the relative location of an instruction within a basic block
        unsigned getDistance(const llvm::Instruction* MI) {
            return DistanceMap[(llvm::Instruction*)MI];
        }

        void MarkVirtRegAliveInBlock(LVInfo& VRInfo, llvm::BasicBlock* DefBlock,
            llvm::BasicBlock* BB);
        void MarkVirtRegAliveInBlock(LVInfo& VRInfo, llvm::BasicBlock* DefBlock,
            llvm::BasicBlock* BB,
            std::vector<llvm::BasicBlock*>& WorkList);

        // ScanBBTopDown: true if instructions of a BB is scanned top-down
        void HandleVirtRegUse(llvm::Value* LV, llvm::BasicBlock* MBB, llvm::Instruction* MI,
            bool ScanAllUses = false, bool ScanBBTopDown = false);
        void HandleVirtRegDef(llvm::Instruction* MI);

        void ComputeLiveness(llvm::Function*, WIAnalysis*);

        // Calculate liveness info for all live variables
        // (same as runOnMachineFunction() of llvm LiveVariables pass
        void Calculate(llvm::Function* F, WIAnalysis* WIA = nullptr);

        iterator begin() { return VirtRegInfo.begin(); }
        const_iterator begin() const { return VirtRegInfo.begin(); }
        iterator end() { return VirtRegInfo.end(); }
        const_iterator end() const { return VirtRegInfo.end(); }

        bool isLiveIn(llvm::Value* LV, const llvm::BasicBlock& MBB) {
            return getLVInfo(LV).isLiveIn(MBB, LV);
        }
        bool isLiveAt(llvm::Value* LV, llvm::Instruction* MI);

        /// isLiveOut - Determine if Reg is live out from MBB, when not considering
        /// PHI nodes. This means that Reg is either killed by a successor block or
        /// passed through one.
        bool isLiveOut(llvm::Value* LV, const llvm::BasicBlock& MBB);

        /// Merge LVInfo of "fromV" into V's LVInfo (ignore the def of "fromV")
        /// Note that this is used after LVInfo has been constructed already.
        void mergeUseFrom(llvm::Value* V, llvm::Value* fromV);

        /// If two values' live ranges overlap, return true.
        bool hasInterference(llvm::Value* V0, llvm::Value* V1);

        /// print - Convert to human readable form
        void print(llvm::raw_ostream& OS, const llvm::Module* = nullptr) const;

        /// dump - Dump the LVInfo to dbgs().
        void dump() const;

#if defined( _DEBUG )
        // for debugging
        llvm::DenseMap<llvm::BasicBlock*, int> BBIds;
        llvm::DenseMap<int, llvm::BasicBlock*> IdToBBs;

        void setupBBIds(llvm::Function* F);
        void dump(llvm::Function* F);
        void dump(int BBId);
#endif

    };

    /// \brief Analysis pass which computes a LiveVars.
    class LiveVarsAnalysis : public llvm::FunctionPass {
        LiveVars LV;

    public:
        static char ID;
        LiveVarsAnalysis();
        LiveVars& getLiveVars() { return LV; }
        const LiveVars& getLiveVars() const { return LV; }
        bool runOnFunction(llvm::Function& F) override;
        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.setPreservesAll();
        }
        void releaseMemory() override { LV.releaseMemory(); }
    };

} // End CISA namespace

#endif
