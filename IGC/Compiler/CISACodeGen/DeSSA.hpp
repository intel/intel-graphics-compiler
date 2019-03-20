/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
//===- StrongPHIElimination.cpp - Eliminate PHI nodes by inserting copies -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//===-------- DeSSA.cpp - divide phi variables into congruent class -------===//
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
      AU.addRequired<CodeGenPatternMatch>( );
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
    }

    virtual llvm::StringRef getPassName() const  override {
        return "DeSSA";
    }

    /// Look at the phi-union and insert-element union, find the root value.
    /// return 0 if reg is isolated, and not in any insert-element union
    llvm::Value* getRootValue(llvm::Value*, e_alignment *pAlign = 0) const;

    /// Get the union-root of the PHI dst-value. The root of a PHI-dst is 0 if 
    /// the PHI has been isolated, or the phi-dst has been reg-isolated
    llvm::Value* getPHIRoot(llvm::Instruction*) const;

    bool isPHIIsolated(llvm::Instruction*) const;

    bool isUniform(llvm::Value *v) const {
      return (WIA->whichDepend(v) == WIAnalysis::UNIFORM);
    }

	void getAllValuesInCongruentClass(
		llvm::Value* V,
		llvm::SmallVector<llvm::Value*, 8>& ValsInCC);

    /// print - print partitions in human readable form
    virtual void print(llvm::raw_ostream &OS, const llvm::Module* = 0) const override;

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
      enum Flags {
        kRegisterIsolatedFlag = 1,
        kPHIIsolatedFlag = 1
      };
      Node(llvm::Value *v, int c, e_alignment align)
          : next(this), prev(this), value(v)
          , rank(0), alignment(align), color(c)
      {
        parent.setPointer(this);
      }

      Node *getLeader();

      llvm::PointerIntPair<Node*, 2> parent;
	  // double-linked circular list. All values are in the same congruent class
	  // except those that have been isolated.
      Node *next;
      Node *prev;
      llvm::Value *value;
      unsigned rank;
      e_alignment alignment;

      // Unique one for each node. Used to represent the color
      // (in another word, id or label) of a congruent class.
      // Start from 1
      int color;
    };

    /// Add a register in a new congruence class containing only itself.
    void MapAddReg(llvm::MapVector<llvm::Value*, Node*> &map, llvm::Value *Val, e_alignment Align);
    /// Join the congruence classes of two registers. This function is biased
    /// towards the left argument, i.e. after
    ///
    /// addReg(r2);
    /// unionRegs(r1, r2);
    ///
    /// the leader of the unioned congruence class is the same as the leader of
    /// r1's congruence class prior to the union. This is actually relied upon
    /// in the copy insertion code.
    void MapUnionRegs(llvm::MapVector<llvm::Value*, Node*> &map, llvm::Value *, llvm::Value *);

    /// Get the union-root of a register. The root is 0 if the register has been
    /// isolated.
    llvm::Value* getRegRoot(llvm::Value*, e_alignment *pAlign = 0) const;

    /// Get the union-root of a PHI. it is the original root of its destination and
    /// all of its operands (before they were isolated if they were).
    llvm::Value* getOrigRoot(llvm::Instruction*) const;

    // Return color (>0) if V is in a congruent class; return 0 otherwise.
    int getRootColor(llvm::Value* V);

    // Isolate a register.
    void isolateReg(llvm::Value*);

    /// Isolate a PHI.
    void isolatePHI(llvm::Instruction*);

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
      llvm::DenseMap<int, llvm::Value*> &CurrentDominatingParent,
      llvm::DenseMap<llvm::Value*, llvm::Value*> &ImmediateDominatingParent);

    void SplitInterferencesForArgument(
      llvm::DenseMap<int, llvm::Value*> &CurrentDominatingParent,
      llvm::DenseMap<llvm::Value*, llvm::Value*> &ImmediateDominatingParent);

    void SplitInterferencesForAlignment();

    llvm::DominatorTree *DT;
    LiveVars *LV;
    WIAnalysis *WIA;
    llvm::LoopInfo *LI;
    CodeGenPatternMatch *CG;
    const llvm::DataLayout *DL;
    CodeGenContext* CTX;

    llvm::BumpPtrAllocator Allocator;
    // Color (label) assigned to each congruent class
    // start from 1. Make sure each Node has a different
    // color number.
    int CurrColor;

public:
    LiveVars *getLiveVars() const { return LV; }

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

	void addReg(llvm::Value* Val, e_alignment Align) {
		MapAddReg(RegNodeMap, Val, Align);
	}
	void unionRegs(llvm::Value* Val1, llvm::Value* Val2) {
		MapUnionRegs(RegNodeMap, Val1, Val2);
	}

    llvm::Value* getInsEltRoot(llvm::Value* Val) const;

    private:
    void CoalesceInsertElementsForBasicBlock(llvm::BasicBlock *blk);

    void InsEltMapAddValue(llvm::Value *Val) {
        if (InsEltMap.find(Val) == InsEltMap.end()) {
            InsEltMap[Val] = Val;
        }
    }
    void InsEltMapUnionValue(llvm::Value *SrcVal, llvm::Value *DefVal) {
        assert(InsEltMap.find(SrcVal) != InsEltMap.end());
        InsEltMap[DefVal] = InsEltMap[SrcVal];
    }
  };

  struct MIIndexCompare {
    MIIndexCompare(LiveVars *_lv) : LV(_lv) { }

    bool operator()(const llvm::Instruction *LHS, const llvm::Instruction *RHS) const {
      return LV->getDistance(LHS) < LV->getDistance(RHS);
    }

    LiveVars *LV;
  };

} // End CISA namespace
