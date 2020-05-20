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
#pragma once

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/Pass.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>

#include "llvmWrapper/IR/InstrTypes.h"

#include <vector>

namespace llvm {
// foward declare the initializer
void initializeWIAnalysisPass(PassRegistry &);
} // namespace llvm

namespace pktz {
/// @Brief, given a conditional branch and its immediate post dominator,
/// find its influence-region and partial joins within the influence region
class BranchInfo {
public:
  BranchInfo(const IGCLLVM::TerminatorInst *inst, const llvm::BasicBlock *ipd);

  void print(llvm::raw_ostream &OS) const;
  void dump() const { print(llvm::dbgs()); }

  const IGCLLVM::TerminatorInst *cbr;
  const llvm::BasicBlock *full_join;
  llvm::DenseSet<llvm::BasicBlock *> influence_region;
  llvm::SmallPtrSet<llvm::BasicBlock *, 4> partial_joins;
  llvm::BasicBlock *fork_blk;
};

/// @brief Work Item Analysis class used to provide information on
///  individual instructions. The analysis class detects values which
///  depend in work-item and describe their dependency.
///  The algorithm used is recursive and new instructions are updated
///  according to their operands (which are already calculated).
/// @Author: Nadav Rotem, who wrote the original code for OCL vectorizer
///
/// @Author: Gang Chen, adopt it for IGC,
///          - extend it to handle the divergent SIMD control-flow
///          - support GFX-specific intrinsic
class WIAnalysis : public llvm::FunctionPass {
public:
  static char ID; // Pass identification, replacement for typeid

  WIAnalysis();

  ~WIAnalysis() {}

  /// @brief LLVM llvm::Function pass entry
  /// @param F llvm::Function to transform
  /// @return True if changed
  virtual bool runOnFunction(llvm::Function &F);

  /// @brief Update dependency relations between all values
  void updateDeps();

  /// @brief backward update dependency based upon use
  void backwardUpdate();

  /// @brief initialize value dependence
  void initDependency(llvm::Function *pF);

  /// @brief describes the type of dependency on the work item
  enum WIDependancy {
    UNIFORM = 0,         /// All elements in vector are constant
    // stride-value between 1 and 1023
    RANDOM = 1024,        /// if stride >= 1024, treat as random      
  };

  /// The WIAnalysis follows pointer arithmetic
  ///  and Index arithmetic when calculating dependency
  ///  properties. If a part of the index is lost due to
  ///  a transformation, it is acceptable.
  ///  This constant decides how many bits need to be
  ///  preserved before we give up on the analysis.
  static const unsigned int MinIndexBitwidthToPreserve;

  /// @brief Returns true if the analysis has a dependency
  //         for the instruction, false otherwise
  /// @param val llvm::Value to test
  /// @return Validity of dependency
  bool validDepend(const llvm::Value *val);

  /// @brief Returns the type of dependency the instruction has on
  /// the work-item
  /// @param val llvm::Value to test
  /// @return Dependency kind
  WIDependancy whichDepend(const llvm::Value *val);

  /// @brief Inform analysis that instruction was invalidated
  /// as pointer may later be reused
  /// @param val llvm::Value to invalidate
  void invalidateDepend(const llvm::Value *val);

  /// incremental update of the dep-map on individual value
  /// without propagation. Exposed for later pass.
  void incUpdateDepend(const llvm::Value *val, WIDependancy dep) {
    m_deps[val] = dep;
  }

  /// check if a value stay uniform when we add a use in the given block
  /// If the value is not uniform to begin with, query returns true.
  bool stayUniformIfUsedAt(const llvm::Value *val, llvm::BasicBlock *blk);

  /// check if a value is defined inside divergent control-flow
  bool insideDivergentCF(const llvm::Value *val) {
    return (
        llvm::isa<llvm::Instruction>(val) &&
        m_ctrlBranches.find(llvm::cast<llvm::Instruction>(val)->getParent()) !=
            m_ctrlBranches.end());
  }

  /// @brief Checks if all of the control flow in the analyzed function is
  /// uniform.
  /// @param F function to check
  /// @return True if masks are needed
  bool isControlFlowUniform(const llvm::Function *F);

  virtual void releaseMemory() {
    m_deps.clear();
    m_changed1.clear();
    m_changed2.clear();
    m_ctrlBranches.clear();
    m_backwardList.clear();
  }

  /// print - print m_deps in human readable form
  virtual void print(llvm::raw_ostream &OS, const llvm::Module * = 0) const;
  void dump() const { print(llvm::dbgs()); }

private:
  /*! \name Dependency Calculation Functions
   *  \{ */
  /// @brief Calculate the dependency type for the instruction
  /// @param inst Instruction to inspect
  /// @return Type of dependency.
  void calculate_dep(const llvm::Value *val);
  WIDependancy calculate_dep(const llvm::BinaryOperator *inst);
  WIDependancy calculate_dep(const llvm::CallInst *inst);
  WIDependancy calculate_dep(const llvm::GetElementPtrInst *inst);
  WIDependancy calculate_dep(const llvm::PHINode *inst);
  WIDependancy calculate_dep(const llvm::Instruction *inst);
  WIDependancy calculate_dep(const llvm::SelectInst *inst);
  WIDependancy calculate_dep(const llvm::AllocaInst *inst);
  WIDependancy calculate_dep(const llvm::CastInst *inst);
  WIDependancy calculate_dep(const llvm::VAArgInst *inst);
  WIDependancy calculate_dep(const llvm::LoadInst *inst);
  /*! \} */

  WIDependancy clampDepend(int stride) {
    if (stride < 0 || stride >= RANDOM)
      return RANDOM;
    return (WIDependancy)stride;
  }
  /// @brief do the trivial checking WI-dep
  /// @param I instruction to check
  /// @return Dependency type. Returns Uniform if all operands are
  ///         Uniform, Random otherwise
  WIDependancy calculate_dep_simple(const llvm::Instruction *I);

  /// @brief update the WI-dep from a divergent branch,
  ///        affected instructions are added to m_pChangedNew
  /// @param the divergent branch
  void update_cf_dep(const llvm::Instruction *TI);

  /// @check phi divergence at a join-blk due to a divergent branch
  void updatePHIDepAtJoin(llvm::BasicBlock *blk, BranchInfo *brInfo);

  void updateDepMap(const llvm::Instruction *inst,
                    WIAnalysis::WIDependancy dep);

  /// @brief Provide known dependency type for requested value
  /// @param val llvm::Value to examine
  /// @return Dependency type. Returns Uniform for unknown type
  WIDependancy getDependency(const llvm::Value *val);

  /// @brief return true if there is calculated dependency type for requested
  /// value
  /// @param val llvm::Value to examine
  /// @return true if value has dependency type, false otherwise.
  bool hasDependency(const llvm::Value *val);

  /// @brief return true if all uses of this value are marked UNIFORM
  bool allUsesUniform(const llvm::Value *val);

  /// @brief return true is the instruction is simple and making it random is
  /// cheap
  bool isInstructionSimple(const llvm::Instruction *inst);

  /// @brief  LLVM Interface
  /// @param AU Analysis
  /// WIAnalysis requires dominator and post dominator analysis
  /// WIAnalysis also requires BreakCriticalEdge because it assumes that
  /// potential phi-moves will be placed at those blocks
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    // Analysis pass preserve all
    AU.setPreservesAll();

    AU.addRequired<llvm::DominatorTreeWrapperPass>();
    AU.addRequired<llvm::PostDominatorTreeWrapperPass>();
  }

private:
  /// Stores an updated list of all dependencies
  llvm::DenseMap<const llvm::Value *, WIDependancy> m_deps;
  /// for each block, store the list of diverging branches that affect it
  llvm::DenseMap<const llvm::BasicBlock *,
                 llvm::SmallPtrSet<const llvm::Instruction *, 4>>
      m_ctrlBranches;

  /// Iteratively one set holds the changed from the previous iteration and
  /// the other holds the new changed values from the current iteration.
  std::vector<const llvm::Value *> m_changed1;
  std::vector<const llvm::Value *> m_changed2;
  /// ptr to m_changed1, m_changed2
  std::vector<const llvm::Value *> *m_pChangedOld;
  std::vector<const llvm::Value *> *m_pChangedNew;

  std::vector<const llvm::Instruction*> m_backwardList;

  llvm::Function *m_func = nullptr;
  llvm::DominatorTree *DT = nullptr;
  llvm::PostDominatorTree *PDT = nullptr;
};

} // end of namespace pktz

