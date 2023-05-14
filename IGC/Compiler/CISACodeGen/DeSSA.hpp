/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

//===-------- DeSSA.hpp - divide phi variables into congruent class -------===//
//
//  Intel Extention to LLVM core
//===----------------------------------------------------------------------===//
//
// This pass is originated from the StrongPHIElimination on the machine-ir.
// We have adopted it to work on llvm-ir. Also note that we have changed it
// from a transformation to an analysis, meaning which only divides phi-vars
// into congruent classes, and does NOT insert the copies. A separate code-gen
// pass can use this analysis to emit non-ssa target code.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "Compiler/CISACodeGen/LiveVars.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/PatternMatchPass.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Allocator.h>
#include <llvm/IR/CFG.h>
#include <llvm/Support/Debug.h>
#include <llvm/IR/Dominators.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/DenseSet.h>
#include "common/LLVMWarningsPop.hpp"
#include <map>
#include "Probe/Assertion.h"

namespace IGC {

    class CShader;
    class CVariable;
    class CodeGenPatternMatch;
    class CodeGenContextWrapper;

    class DeSSA : public llvm::FunctionPass {
    public:
        static char ID; // Pass identification, replacement for typeid

        DeSSA();
        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            AU.setPreservesAll();
            AU.addRequired<llvm::DominatorTreeWrapperPass>();
            AU.addRequired<WIAnalysis>();
            AU.addRequired<LiveVarsAnalysis>();
            AU.addRequired<CodeGenPatternMatch>();
            AU.addRequired<llvm::LoopInfoWrapperPass>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }
        bool runOnFunction(llvm::Function&) override;

        virtual void releaseMemory() override {
            Allocator.Reset();
            PHISrcDefs.clear();
            PHISrcArgs.clear();
            RegNodeMap.clear();
            InsEltMap.clear();
            AliasMap.clear();
        }

        virtual llvm::StringRef getPassName() const  override {
            return "DeSSA";
        }

        /// Look at the phi-union and insert-element union, find the root value.
        /// return 0 if reg is isolated, and not in any insert-element union
        llvm::Value* getRootValue(llvm::Value*, e_alignment* pAlign = 0) const;

        bool isIsolated(llvm::Value*) const;

        bool isUniform(llvm::Value* v) const {
            return (WIA->isUniform(v));
        }

        void getAllValuesInCongruentClass(
            llvm::Value* V,
            llvm::SmallVector<llvm::Value*, 8> & ValsInCC);

        /// print - print partitions in human readable form
        virtual void print(llvm::raw_ostream& OS, const llvm::Module* = 0) const override;

        /// dump - Dump the partitions to dbgs().
        void dump() const;

    private:
        /// This struct represents a single node in the union-find data structure
        /// representing the variable congruence classes. There is one difference
        /// from a normal union-find data structure. We steal two bits from the parent
        /// pointer . One of these bits is used to represent whether the register
        /// itself has been isolated, and the other is used to represent whether the
        /// PHI with that register as its destination has been isolated.
        ///
        /// Note that this leads to the strange situation where the leader of a
        /// congruence class may no longer logically be a member, due to being
        /// isolated.
        struct Node {
            Node(llvm::Value* v, int c, e_alignment align)
                : parent(this), next(this), prev(this), value(v)
                , rank(0), alignment(align), color(c)
            {
            }

            /// Find leader (representative of a congruent class) by path halving
            Node* getLeader();

            Node* parent;
            // double-linked circular list. All values are in the same congruent class
            // except those that have been isolated.
            Node* next;
            Node* prev;
            llvm::Value* value;
            unsigned rank;
            e_alignment alignment;

            // Unique one for each node. Used to represent the color
            // (in another word, id or label) of a congruent class.
            // Start from 1
            int color;
        };

        /// Get the union-root of a register. The root is 0 if the register has been
        /// isolated.
        llvm::Value* getRegRoot(llvm::Value*, e_alignment* pAlign = 0) const;

        // Return color (>0) if V is in a congruent class; return 0 otherwise.
        // (This is needed during traversal of algo. The color is used as the
        // reprentative of a congruent class that remains unchanged during traversal.)
        int getRootColor(llvm::Value* V);

        // Isolate a register.
        void isolateReg(llvm::Value*);

        /// Is it isolated (single-valued congruent class)
        bool isIsolated(Node* N) const { return (N == N->next); }

        // Split node from its existing congurent class, and
        // node itself becomes a new single-value congruent class
        void splitNode(Node* ND);

        /// Traverses a basic block, splitting any interferences found between
        /// registers in the same congruence class. It takes two DenseMaps as
        /// arguments that it also updates: CurrentDominatingParent, which maps
        /// a color to the register in that congruence class whose definition was
        /// most recently seen, and ImmediateDominatingParent, which maps a register
        /// to the register in the same congruence class that most immediately
        /// dominates it.
        ///
        /// This function assumes that it is being called in a depth-first traversal
        /// of the dominator tree.
        void SplitInterferencesForBasicBlock(
            llvm::BasicBlock*,
            llvm::DenseMap<int, llvm::Value*>& CurrentDominatingParent,
            llvm::DenseMap<llvm::Value*, llvm::Value*>& ImmediateDominatingParent);

        void SplitInterferencesForArgument(
            llvm::DenseMap<int, llvm::Value*>& CurrentDominatingParent,
            llvm::DenseMap<llvm::Value*, llvm::Value*>& ImmediateDominatingParent);

        void SplitInterferencesForAlignment();

        llvm::DominatorTree* DT;
        LiveVars* LV;
        WIAnalysis* WIA;
        llvm::LoopInfo* LI;
        CodeGenPatternMatch* CG;
        const llvm::DataLayout* DL;
        CodeGenContext* CTX;
        llvm::Function* m_F;  // Current Function

        llvm::BumpPtrAllocator Allocator;

        // Color (label) assigned to each congruent class
        // start from 1. Make sure each Node has a different
        // color number.
        int CurrColor;

    public:
        LiveVars* getLiveVars() const { return LV; }

        llvm::MapVector<llvm::Value*, Node*> RegNodeMap;

        // Maps a basic block to a list of its defs of registers that appear as PHI
        // sources.
        llvm::DenseMap<llvm::BasicBlock*, std::vector<llvm::Instruction*> > PHISrcDefs;
        llvm::SetVector<llvm::Value*> PHISrcArgs;

        // Maps a color to a pair of a llvm::Instruction* and a virtual register, which
        // is the operand of that PHI corresponding to the current basic block.
        llvm::DenseMap<int, std::pair<llvm::Instruction*, llvm::Value*> > CurrentPHIForColor;

        // Implement reuse for InsertElement only
        // Hierarchical coalescing:
        // - step 1, union insert-elements into trees, update the liveness
        // - step 2, build phi-union, the root of the InsElt-union-tree can be added to the phi-union.
        // - step 3, detect interferences of every phi-union, a set of insert-elements are associated
        //           with a single-node in phi-union. When being isolated, they are isolated together
        llvm::MapVector<llvm::Value*, llvm::Value*> InsEltMap;

        // Value Alias map
        //   This is used for maitaining aliases among values. It maps a value, called 'aliaser',
        //   to its 'aliasee' (denoted as alias(aliaer, aliasee). This map has the following
        //   properties:
        //       1. No alias chain, that is, the map does not have both alias(v0, v1) and
        //          alias(v1, v2) with v0 != V1 != v2.
        //
        //          Note that for each aliasee, say V,  alias(V, V) is in map for convenience to
        //          indicate V is aliasee and to help setup aliasing entry of the map.
        //       2. The aliasee's liveness info has been updated to be the union of all its aliasers.
        //          In this way, only aliasee will be used in DeSSA node.
        //
        // The DeSSA/Coalescing procedure:
        //   1. Follow Dominance tree to set up alias map. While setting up alias map,
        //      update liveness for aliasee.
        //   2. Make sure InsEltMap only use aliasee
        //   3. Make sure DeSSA node only use aliasee.
        llvm::DenseMap<llvm::Value*, llvm::Value*> AliasMap;

        // If an inst is an aliaser and no need to generate code
        // due to aliasing, it will be added in this map.
        llvm::DenseMap<llvm::Value*, int> NoopAliasMap;

        /// If there is no node for Val, create a new one.
        void addReg(llvm::Value* Val, e_alignment Align);

        int getGRFSize() const { return CTX->platform.getGRFSize(); }

        /// union-by-rank:
        ///   Join the congruence classes of two registers by attaching
        ///   a shorter tree to a taller tree. If they have the same height,
        ///   attaching Val2 to Val1. Note that unionRegs() expects that
        ///   nodes for Val1 and Val2 have been created already.
        void unionRegs(llvm::Value* Val1, llvm::Value* Val2) {
            unionRegs(RegNodeMap[Val1], RegNodeMap[Val2]);
        }

        // For a value, return its representative value that is used
        // to create dessa node, which is its aliasee's InsElt root.
        llvm::Value* getNodeValue(llvm::Value* V) const {
            llvm::Value* aliasee = getAliasee(V);
            return getInsEltRoot(aliasee);
        }

        llvm::Value* getInsEltRoot(llvm::Value* Val) const;
        llvm::Value* getAliasee(llvm::Value* V) const;
        bool isAliasee(llvm::Value* V) const;
        bool isAliaser(llvm::Value* V) const;
        bool isNoopAliaser(llvm::Value* V) const;
        bool isSingleValued(llvm::Value* V) const;
        bool interfere(llvm::Value* V0, llvm::Value* V1);
        bool aliasInterfere(llvm::Value* V0, llvm::Value* V1);
        bool alignInterfere(e_alignment a1, e_alignment a2);

    private:
        void CoalesceInsertElements();

        void InsEltMapAddValue(llvm::Value* Val) {
            if (InsEltMap.find(Val) == InsEltMap.end()) {
                InsEltMap[Val] = Val;
            }
        }
        void InsEltMapUnionValue(llvm::Value* SrcVal, llvm::Value* DefVal) {
            IGC_ASSERT(InsEltMap.find(SrcVal) != InsEltMap.end());
            InsEltMap[DefVal] = InsEltMap[SrcVal];
        }

        void unionRegs(Node* N1, Node* N2);

        void CoalesceAliasInst();
        int checkInsertElementAlias(
            llvm::InsertElementInst* IEI,
            llvm::SmallVector<llvm::Value*, 16> & AllIEIs);
        void coalesceAliasInsertValue(llvm::InsertValueInst* theIVI);

        // Add Val->Val into aliasMap if it is not in the map yet.
        // Return Val's aliasee.
        void AddAlias(llvm::Value* Val) {
            if (AliasMap.find(Val) == AliasMap.end())
                AliasMap[Val] = Val;
        }

        // If V is an arg or a needed inst (by patternmatch),
        // return true; otherwise, return false;
        bool isArgOrNeededInst(llvm::Value* V) {
            if (llvm::Instruction * I = llvm::dyn_cast<llvm::Instruction>(V))
            {
                return CG->NeedInstruction(*I);
            }
            return llvm::isa<llvm::Argument>(V);
        }
    };

    struct MIIndexCompare {
        MIIndexCompare(LiveVars* _lv) : LV(_lv) { }

        bool operator()(const llvm::Instruction* LHS, const llvm::Instruction* RHS) const {
            return LV->getDistance(LHS) < LV->getDistance(RHS);
        }

        LiveVars* LV;
    };

} // End CISA namespace
