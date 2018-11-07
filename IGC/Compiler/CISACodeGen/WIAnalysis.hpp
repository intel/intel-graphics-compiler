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

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CISACodeGen/TranslationTable.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/PostDominators.h>
#include "common/LLVMWarningsPop.hpp"


#ifdef OCL_SPECIFIC
#include "RuntimeServices.h"
#include "SoaAllocaAnalysis.h"
#include "Logger.h"
#endif

#include <vector>

namespace IGC
{

/// @Brief, given a conditional branch and its immediate post dominator,
/// find its influence-region and partial joins within the influence region
class BranchInfo
{
public:
    BranchInfo(const llvm::TerminatorInst *inst, const llvm::BasicBlock *ipd);

    void print(llvm::raw_ostream &OS) const;

    const llvm::TerminatorInst *cbr;
    const llvm::BasicBlock *full_join;
    llvm::DenseSet<llvm::BasicBlock*> influence_region;
    llvm::SmallPtrSet<llvm::BasicBlock*, 4> partial_joins;
    llvm::BasicBlock *fork_blk;
};


//This is a trick, since we cannot forward-declare enums embedded in class definitions.
// The better solution is to completely hoist-out the WIDependency enum into a separate enum class
// (c++ 11) and have it separate from WIAnalysis pass class. Nevertheless, that would require
// updating many places in the current CodeGen code...
// thus: WIAnalysis::WIDependancy ~ WIBaseClass::WIDependancy, the types are equal and do not require
// conversion
class WIBaseClass
{
public:
    /// @brief describes the type of dependency on the work item
    enum WIDependancy {
        UNIFORM = 0, /// All elements in vector are constant
        CONSECUTIVE = 1, /// Elements are consecutive
        PTR_CONSECUTIVE = 2, /// Elements are pointers which are consecutive
        STRIDED = 3, /// Elements are in strides
        RANDOM = 4, /// Unknown or non consecutive order
        NumDeps = 5,  /// Overall amount of dependencies
        INVALID = 6
    };
};

// Provide FastValueMapAttributeInfo for WIDependancy.
template<> struct FastValueMapAttributeInfo<WIBaseClass::WIDependancy> {
    static inline WIBaseClass::WIDependancy getEmptyAttribute() { return WIBaseClass::INVALID; }
};

/// @brief Work Item Analysis class used to provide information on
///  individual instructions. The analysis class detects values which
///  depend in work-item and describe their dependency.
///  The algorithm used is recursive and new instructions are updated
///  according to their operands (which are already calculated).
///  original code for OCL vectorizer
///
///   adopt it for IGC,
///   - extend it to handle the divergent SIMD control-flow
///  - support GFX-specific intrinsic
class WIAnalysis : public llvm::FunctionPass, public WIBaseClass
{
public:
    static char ID; // Pass identification, replacement for typeid

    WIAnalysis();


    ~WIAnalysis() {}

    /// @brief Provides name of pass
    virtual llvm::StringRef getPassName() const override
    {
      return "WIAnalysis";
    }

    /// @brief LLVM llvm::Function pass entry
    /// @param F llvm::Function to transform
    /// @return True if changed
    virtual bool runOnFunction(llvm::Function &F) override;

    /// @brief Update dependency relations between all values
    void updateDeps();

    /// @brief backward update dependency based upon use
    void genSpecificBackwardUpdate();

    /// @brief mark the arguments dependency based on the metadata set
    void updateArgsDependency(llvm::Function *pF);

    /// The WIAnalysis follows pointer arithmetic
    ///  and Index arithmetic when calculating dependency
    ///  properties. If a part of the index is lost due to 
    ///  a transformation, it is acceptable.
    ///  This constant decides how many bits need to be 
    ///  preserved before we give up on the analysis.
    static const unsigned int MinIndexBitwidthToPreserve;

    /// @brief Returns the type of dependency the instruction has on
    /// the work-item
    /// @param val llvm::Value to test
    /// @return Dependency kind
    WIDependancy whichDepend(const llvm::Value* val);

    /// incremental update of the dep-map on individual value
    /// without propagation. Exposed for later pass.
    void incUpdateDepend(const llvm::Value* val, WIDependancy dep)
    {
      m_depMap.SetAttribute(val, dep);
    }

    /// check if a value is defined inside divergent control-flow
    bool insideDivergentCF(const llvm::Value* val)
    {
        return(llvm::isa<llvm::Instruction>(val) &&
            m_ctrlBranches.find(llvm::cast<llvm::Instruction>(val)->getParent()) != m_ctrlBranches.end());
    }

    virtual void releaseMemory() override
    {
      m_depMap.clear();
      m_changed1.clear();
      m_changed2.clear();
      m_ctrlBranches.clear();
      m_backwardList.clear();
    }

    /// print - print m_deps in human readable form
    virtual void print(llvm::raw_ostream &OS, const llvm::Module* = 0) const override;

    /// dump - Dump the m_deps to dbgs().
    void dump() const;

private:
    struct AllocaDep
    {
        std::vector<const llvm::StoreInst*> stores;
    };
    /*! \name Dependency Calculation Functions
     *  \{ */
    /// @brief Calculate the dependency type for the instruction
    /// @param inst Instruction to inspect
    /// @return Type of dependency.
    void calculate_dep(const llvm::Value* val);
    WIDependancy calculate_dep(const llvm::BinaryOperator* inst);
    WIDependancy calculate_dep(const llvm::CallInst* inst);
    WIDependancy calculate_dep(const llvm::GetElementPtrInst* inst);
    WIDependancy calculate_dep(const llvm::PHINode* inst);
    WIDependancy calculate_dep(const llvm::TerminatorInst* inst);
    WIDependancy calculate_dep(const llvm::SelectInst* inst);
    WIDependancy calculate_dep(const llvm::AllocaInst* inst);
    WIDependancy calculate_dep(const llvm::CastInst* inst);
    WIDependancy calculate_dep(const llvm::VAArgInst* inst);
    WIDependancy calculate_dep(const llvm::LoadInst* inst);
    /*! \} */

    /// @brief do the trivial checking WI-dep
    /// @param I instruction to check
    /// @return Dependency type. Returns Uniform if all operands are 
    ///         Uniform, Random otherwise
    WIDependancy calculate_dep_simple(const llvm::Instruction *I);

    /// @brief update the WI-dep from a divergent branch,
    ///        affected instructions are added to m_pChangedNew
    /// @param the divergent branch
    void update_cf_dep(const llvm::TerminatorInst *TI);

    /// @brief update the WI-dep for a sequence of insert-elements forming a vector
    ///        affected instructions are added to m_pChangedNew
    /// @param the insert-element instruction
    void updateInsertElements(const llvm::InsertElementInst *inst);

    /// @check phi divergence at a join-blk due to a divergent branch
    void updatePHIDepAtJoin(llvm::BasicBlock *blk, BranchInfo *brInfo);

    void updateDepMap(const llvm::Instruction *inst, WIAnalysis::WIDependancy dep);

    /// @brief Provide known dependency type for requested value
    /// @param val llvm::Value to examine
    /// @return Dependency type. Returns Uniform for unknown type
    WIDependancy getDependency(const llvm::Value *val);

    /// @brief return true if there is calculated dependency type for requested value
    /// @param val llvm::Value to examine
    /// @return true if value has dependency type, false otherwise.
    bool hasDependency(const llvm::Value *val);
    
    /// @brief return true if all uses of this value are marked RANDOM
    bool allUsesRandom(const llvm::Value *val);

    /// @brief return true if any of the use require the value to be uniform
    bool needToBeUniform(const llvm::Value *val);

    /// @brief return true is the instruction is simple and making it random is cheap
    bool isInstructionSimple(const llvm::Instruction* inst);

    /// @brief return true if all the source operands are defined outside the region
    bool isRegionInvariant(const llvm::Instruction* inst, BranchInfo *brInfo, unsigned level);

    /// @brief update dependency structure for Alloca
    bool TrackAllocaDep(const llvm::Value* I, AllocaDep& dep);
    /// @brief  LLVM Interface
    /// @param AU Analysis
    /// WIAnalysis requires dominator and post dominator analysis
    /// WIAnalysis also requires BreakCriticalEdge because it assumes that 
    /// potential phi-moves will be placed at those blocks
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
      // Analysis pass preserve all
      AU.setPreservesAll();
#ifdef OCL_SPECIFIC
      AU.addRequired<SoaAllocaAnalysis>();
#endif
      AU.addRequired<llvm::DominatorTreeWrapperPass>();
      AU.addRequired<llvm::PostDominatorTreeWrapperPass>();
      AU.addRequired<MetaDataUtilsWrapper>();
      AU.addRequired<CodeGenContextWrapper>();
      AU.addRequired<TranslationTable>();
    }   

private:
#ifdef OCL_SPECIFIC
    // @brief pointer to Soa alloca analysis performed for this function
    SoaAllocaAnalysis *m_soaAllocaAnalysis;
    /// Runtime services pointer
    RuntimeServices * m_rtServices;
#endif
    /// Stores an updated list of all dependencies
    /// for each block, store the list of diverging branches that affect it
    llvm::DenseMap<const llvm::BasicBlock*, llvm::SmallPtrSet<const llvm::Instruction*, 4>> m_ctrlBranches;

    /// Iteratively one set holds the changed from the previous iteration and
    /// the other holds the new changed values from the current iteration.
    std::vector<const llvm::Value*> m_changed1;
    std::vector<const llvm::Value*> m_changed2;
    /// ptr to m_changed1, m_changed2 
    std::vector<const llvm::Value*> *m_pChangedOld;
    std::vector<const llvm::Value*> *m_pChangedNew;

    std::vector<const llvm::Instruction*> m_backwardList;

    llvm::Function *m_func;
    llvm::PostDominatorTree *PDT;
    IGC::IGCMD::MetaDataUtils *m_pMdUtils;

    // Allow access to all the store into an alloca if we were able to track it
    llvm::DenseMap<const llvm::AllocaInst*, AllocaDep> m_allocaDepMap;
    // reverse map to allow to know what alloca to update when store changes
    llvm::DenseMap<const llvm::StoreInst*, const llvm::AllocaInst*> m_storeDepMap;

    IGC::TranslationTable *m_pTT;
    IGC::FastValueMap<WIAnalysis::WIDependancy, FastValueMapAttributeInfo<WIBaseClass::WIDependancy>> m_depMap;
  };

} // namespace IGC
