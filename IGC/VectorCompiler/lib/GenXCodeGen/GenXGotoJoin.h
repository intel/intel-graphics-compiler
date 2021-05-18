/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef TARGET_GENXGOTOJOIN_H
#define TARGET_GENXGOTOJOIN_H

namespace llvm {

class BasicBlock;
class CallInst;
class DominatorTree;
class Instruction;
class Value;

namespace genx {

// GotoJoin : class containing goto/join related utility functions
class GotoJoin {
public:

  // isEMValue : detect whether a value is an EM (execution mask)
  static bool isEMValue(Value *V);

  // findJoin : given a goto, find the join whose RM it modifies
  static CallInst *findJoin(CallInst *Goto);

  // isValidJoin : check that the block containing a join is valid
  static bool isValidJoin(CallInst *Join);

  // isBranchingJoinLabelBlock : check whether a block has a single join and
  //    is both a join label and a branching join
  static bool isBranchingJoinLabelBlock(BasicBlock *BB);

  // getBranchingBlockForJoinLabel : if BB is "true" successor of branching
  // block, return this branching block. If SkipCriticalEdgeSplitter is set,
  // empty critical edge splitter blocks are skipped.
  static BasicBlock *getBranchingBlockForBB(BasicBlock *BB,
                                            bool SkipCriticalEdgeSplitter);

  // isJoinLabel : see if the block is a join label
  static bool isJoinLabel(BasicBlock *BB, bool SkipCriticalEdgeSplitter = false);

  // isGotoBlock : see if a basic block is a goto block (hence branching), returning the goto if so
  static CallInst *isGotoBlock(BasicBlock *BB);

  // isBranchingJoinBlock : see if a basic block is a branching join block
  static CallInst *isBranchingJoinBlock(BasicBlock *BB);

  // isBranchingGotoJoinBlock : see if a basic block is a branching goto/join block
  static CallInst *isBranchingGotoJoinBlock(BasicBlock *BB);

  // getLegalInsertionPoint : ensure an insertion point is legal in the presence of SIMD CF
  static Instruction *getLegalInsertionPoint(Instruction *InsertBefore, DominatorTree *DomTree);

};

} // End genx namespace
} // End llvm namespace

#endif
