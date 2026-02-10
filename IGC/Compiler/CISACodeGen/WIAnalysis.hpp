/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CISACodeGen/TranslationTable.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/PostDominators.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/InstrTypes.h>

#include <vector>
#include <common/Types.hpp>

namespace IGC {
class BranchInfo;
class WIAnalysis;

// This is a trick, since we cannot forward-declare enums embedded in class definitions.
//  The better solution is to completely hoist-out the WIDependency enum into a separate enum class
//  (c++ 11) and have it separate from WIAnalysis pass class. Nevertheless, that would require
//  updating many places in the current CodeGen code...
//  thus: WIAnalysis::WIDependancy ~ WIBaseClass::WIDependancy, the types are equal and do not require
//  conversion
class WIBaseClass {
public:
  /// @brief describes the type of dependency on the work item
  enum WIDependancy : uint8_t {
    UNIFORM_GLOBAL = 0,    /// Same for all work-items within a shader.
    UNIFORM_WORKGROUP = 1, /// Same for all work-items within a work group (compute).
    UNIFORM_THREAD = 2,    /// Same for all work-items within a HW thread.
    CONSECUTIVE = 3,       /// Elements are consecutive
    PTR_CONSECUTIVE = 4,   /// Elements are pointers which are consecutive
    STRIDED = 5,           /// Elements are in strides
    RANDOM = 6,            /// Unknown or non consecutive order
    NumDeps = 7,           /// Overall amount of dependencies
    INVALID = 8
  };
};

// Provide FastValueMapAttributeInfo for WIDependancy.
template <> struct FastValueMapAttributeInfo<WIBaseClass::WIDependancy> {
  static inline WIBaseClass::WIDependancy getEmptyAttribute() { return WIBaseClass::INVALID; }
};

class WIAnalysisRunner {
public:
  void init(llvm::Function *F, llvm::LoopInfo *LI, llvm::DominatorTree *DT, llvm::PostDominatorTree *PDT,
            IGCMD::MetaDataUtils *MDUtils, CodeGenContext *CGCtx, ModuleMetaData *ModMD, TranslationTable *TransTable,
            bool ForCodegen = true);

  WIAnalysisRunner(llvm::Function *F, llvm::LoopInfo *LI, llvm::DominatorTree *DT, llvm::PostDominatorTree *PDT,
                   IGCMD::MetaDataUtils *MDUtils, CodeGenContext *CGCtx, ModuleMetaData *ModMD,
                   TranslationTable *TransTable, bool ForCodegen = true) {
    init(F, LI, DT, PDT, MDUtils, CGCtx, ModMD, TransTable, ForCodegen);
  }

  WIAnalysisRunner() {}

  bool run();

  /// @brief Returns the type of dependency the instruction has on
  /// the work-item
  /// @param val llvm::Value to test
  /// @return Dependency kind
  WIBaseClass::WIDependancy whichDepend(const llvm::Value *val) const;

  /// @brief Returns True if 'val' is uniform
  /// @param val llvm::Value to test
  bool isUniform(const llvm::Value *val) const;
  bool isWorkGroupOrGlobalUniform(const llvm::Value *val) const;
  bool isGlobalUniform(const llvm::Value *val) const;

  /// incremental update of the dep-map on individual value
  /// without propagation. Exposed for later pass.
  void incUpdateDepend(const llvm::Value *val, WIBaseClass::WIDependancy dep) { m_depMap.SetAttribute(val, dep); }

  /// check if a value is defined inside divergent control-flow
  bool insideDivergentCF(const llvm::Value *val) const {
    return (llvm::isa<llvm::Instruction>(val) &&
            m_ctrlBranches.find(llvm::cast<llvm::Instruction>(val)->getParent()) != m_ctrlBranches.end());
  }

  /// check if a value is defined inside workgroup divergent control-flow.
  /// This will return false if the value is in the influence region
  /// of only global and workgroup uniform branches.
  bool insideWorkgroupDivergentCF(const llvm::Value *val) const;

  void releaseMemory() {
    m_ctrlBranches.clear();
    m_changed1.clear();
    m_changed2.clear();
    m_allocaDepMap.clear();
    m_storeDepMap.clear();
    m_depMap.clear();
    m_forcedUniforms.clear();
  }

  /// print - print m_deps in human readable form
  void print(llvm::raw_ostream &OS, const llvm::Module * = 0) const;

  /// dump - Dump the m_deps to a file.
  void dump() const;

  // helper for dumping WI info into files with lock
  void lock_print();

private:
  WIBaseClass::WIDependancy getCFDependency(const llvm::BasicBlock *BB) const;

  struct AllocaDep {
    std::vector<const llvm::StoreInst *> stores;
    std::vector<const llvm::IntrinsicInst *> lifetimes;
    bool assume_uniform = false;
  };

  /// @brief Update dependency relations between all values
  void updateDeps();

  /// @brief mark the arguments dependency based on the metadata set
  void updateArgsDependency(llvm::Function *pF);

  /*! \name Dependency Calculation Functions
   *  \{ */
  /// @brief Calculate the dependency type for the instruction
  /// @param inst Instruction to inspect
  /// @return Type of dependency.
  void calculate_dep(const llvm::Value *val);
  WIBaseClass::WIDependancy calculate_dep(const llvm::BinaryOperator *inst);
  WIBaseClass::WIDependancy calculate_dep(const llvm::CallInst *inst);
  WIBaseClass::WIDependancy calculate_dep(const llvm::GetElementPtrInst *inst);
  WIBaseClass::WIDependancy calculate_dep(const llvm::PHINode *inst);
  WIBaseClass::WIDependancy calculate_dep(const llvm::SelectInst *inst);
  WIBaseClass::WIDependancy calculate_dep(const llvm::AllocaInst *inst);
  WIBaseClass::WIDependancy calculate_dep(const llvm::CastInst *inst);
  WIBaseClass::WIDependancy calculate_dep(const llvm::VAArgInst *inst);
  WIBaseClass::WIDependancy calculate_dep(const llvm::LoadInst *inst);

  WIBaseClass::WIDependancy calculate_dep_terminator(const IGCLLVM::TerminatorInst *inst);
  /*! \} */

  /// @brief do the trivial checking WI-dep
  /// @param I instruction to check
  /// @return Dependency type. Returns Uniform if all operands are
  ///         Uniform, Random otherwise
  WIBaseClass::WIDependancy calculate_dep_simple(const llvm::Instruction *I);

  /// @brief update the WI-dep from a divergent branch,
  ///        affected instructions are added to m_pChangedNew
  /// @param the divergent branch
  void update_cf_dep(const IGCLLVM::TerminatorInst *TI);

  /// @brief update the WI-dep for a sequence of insert-elements forming a vector
  ///        affected instructions are added to m_pChangedNew
  /// @param the insert-element instruction
  void updateInsertElements(const llvm::InsertElementInst *inst);

  /// @brief update the WI-dep for a insertValue chain to add affected
  ///        instructions to m_pChangedNew. This is to make sure if
  ///        one in the chain is RANDOM, all are RANDOM.
  /// @param the insert-element instruction
  void updateInsertValues(const llvm::InsertValueInst *Inst);

  /// @check phi divergence at a join-blk due to a divergent branch
  void updatePHIDepAtJoin(llvm::BasicBlock *blk, BranchInfo *brInfo);

  void updateDepMap(const llvm::Instruction *inst, WIBaseClass::WIDependancy dep);

  /// @brief Provide known dependency type for requested value
  /// @param val llvm::Value to examine
  /// @return Dependency type. Returns Uniform for unknown type
  WIBaseClass::WIDependancy getDependency(const llvm::Value *val);

  /// @brief return true if there is calculated dependency type for requested value
  /// @param val llvm::Value to examine
  /// @return true if value has dependency type, false otherwise.
  bool hasDependency(const llvm::Value *val) const;

  /// @brief return true if all uses of this value are marked RANDOM
  bool allUsesRandom(const llvm::Value *val);

  /// @brief return true if any of the use require the value to be uniform
  bool needToBeUniform(const llvm::Value *val);

  /// @brief return true is the instruction is simple and making it random is cheap
  bool isInstructionSimple(const llvm::Instruction *inst);

  /// @brief return true if all the source operands are defined outside the region
  bool isRegionInvariant(const llvm::Instruction *inst, BranchInfo *brInfo);

  /// @brief return true if instruction is used as lane ID in subgroup broadcast
  bool isUsedByWaveBroadcastAsLocalID(const llvm::Instruction *inst);

  /// @brief update dependency structure for Alloca
  bool TrackAllocaDep(const llvm::Value *I, AllocaDep &dep);

  void checkLocalIdUniform(llvm::Function *F, bool &IsLxUniform, bool &IsLyUniform, bool &IsLzUniform);

  void CS_checkLocalIDs(llvm::Function *F);

private:
  /// The WIAnalysis follows pointer arithmetic
  ///  and Index arithmetic when calculating dependency
  ///  properties. If a part of the index is lost due to
  ///  a transformation, it is acceptable.
  ///  This constant decides how many bits need to be
  ///  preserved before we give up on the analysis.
  static const unsigned int MinIndexBitwidthToPreserve;

  /// Stores an updated list of all dependencies
  /// for each block, store the list of diverging branches that affect it
  llvm::DenseMap<const llvm::BasicBlock *, llvm::SmallPtrSet<const llvm::Instruction *, 4>> m_ctrlBranches;

  /// Iteratively one set holds the changed from the previous iteration and
  /// the other holds the new changed values from the current iteration.
  std::vector<const llvm::Value *> m_changed1{};
  std::vector<const llvm::Value *> m_changed2{};
  /// ptr to m_changed1, m_changed2
  std::vector<const llvm::Value *> *m_pChangedOld = nullptr;
  std::vector<const llvm::Value *> *m_pChangedNew = nullptr;

  /// <summary>
  ///  hold the vector-defs that are promoted from an uniform alloca
  ///  therefore, need to be forced into uniform no matter what.
  /// </summary>
  std::vector<const llvm::Value *> m_forcedUniforms{};

  llvm::Function *m_func = nullptr;
  llvm::LoopInfo *LI = nullptr;
  llvm::DominatorTree *DT = nullptr;
  llvm::PostDominatorTree *PDT = nullptr;
  IGC::IGCMD::MetaDataUtils *m_pMdUtils = nullptr;
  IGC::CodeGenContext *m_CGCtx = nullptr;
  IGC::ModuleMetaData *m_ModMD = nullptr;
  IGC::TranslationTable *m_TT = nullptr;

  // Allow access to all the store into an alloca if we were able to track it
  llvm::DenseMap<const llvm::AllocaInst *, AllocaDep> m_allocaDepMap;
  // reverse map to allow to know what alloca to update when store changes
  llvm::DenseMap<const llvm::StoreInst *, const llvm::AllocaInst *> m_storeDepMap;

  IGC::FastValueMap<WIBaseClass::WIDependancy, FastValueMapAttributeInfo<WIBaseClass::WIDependancy>> m_depMap;

  // For dumpping WIA info per each invocation
  static llvm::DenseMap<const llvm::Function *, int> m_funcInvocationId;

  // Is this analysis providing information that could be used for late
  // stage codegen, or is it just used to determine uniformity early on?
  bool ForCodegen = true;

  // Caching CS local ID's uniformness
  bool m_localIDxUniform = false;
  bool m_localIDyUniform = false;
  bool m_localIDzUniform = false;
};

/// @brief Work Item Analysis class used to provide information on
///  individual instructions. The analysis class detects values which
///  depend in work-item and describe their dependency.
///  The algorithm used is recursive and new instructions are updated
///  according to their operands (which are already calculated).
///  original code for OCL vectorizer
///
class WIAnalysis : public llvm::FunctionPass, public WIBaseClass {
public:
  static char ID; // Pass identification, replacement for typeid

  WIAnalysis();

  ~WIAnalysis() {}

  /// @brief Provides name of pass
  llvm::StringRef getPassName() const override { return "WIAnalysis"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    // Analysis pass preserve all
    AU.setPreservesAll();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
    AU.addRequired<llvm::PostDominatorTreeWrapperPass>();
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<TranslationTable>();
  }

  /// @brief LLVM llvm::Function pass entry
  /// @param F llvm::Function to transform
  /// @return True if changed
  bool runOnFunction(llvm::Function &F) override;

  /// print - print m_deps in human readable form
  void print(llvm::raw_ostream &OS, const llvm::Module * = 0) const override;

  /// dump - Dump the m_deps to dbgs().
  void dump() const;

public:
  /// @brief Returns the type of dependency the instruction has on
  /// the work-item
  /// @param val llvm::Value to test
  /// @return Dependency kind
  WIDependancy whichDepend(const llvm::Value *val);

  /// @brief Returns True if 'val' is uniform
  /// @param val llvm::Value to test
  bool isUniform(const llvm::Value *val) const; // Return true for any uniform
  bool isWorkGroupOrGlobalUniform(const llvm::Value *val);
  bool isGlobalUniform(const llvm::Value *val);

  /// incremental update of the dep-map on individual value
  /// without propagation. Exposed for later pass.
  void incUpdateDepend(const llvm::Value *val, WIDependancy dep);

  /// check if a value is defined inside divergent control-flow
  bool insideDivergentCF(const llvm::Value *val) const;

  /// check if a value is defined inside workgroup divergent control-flow
  /// This will return false if the value is in the influence region
  /// of only global and workgroup uniform branches.
  bool insideWorkgroupDivergentCF(const llvm::Value *val) const;

  void releaseMemory() override { Runner.releaseMemory(); }

  /// Return true if Dep is any of uniform dependancy.
  static bool isDepUniform(WIDependancy Dep) {
    return Dep == WIDependancy::UNIFORM_GLOBAL || Dep == WIDependancy::UNIFORM_WORKGROUP ||
           Dep == WIDependancy::UNIFORM_THREAD;
  }
  WIAnalysisRunner Runner;
};

} // namespace IGC
