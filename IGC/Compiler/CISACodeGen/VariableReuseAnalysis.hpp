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
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/PatternMatchPass.hpp"
#include "Compiler/CISACodeGen/DeSSA.hpp"
#include "Compiler/CISACodeGen/CoalescingEngine.hpp"
#include "Compiler/CISACodeGen/BlockCoalescing.hpp"


#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Dominators.h"
#include "llvm/ADT/TinyPtrVector.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstVisitor.h>
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/RegisterEstimator.hpp"

namespace IGC {

struct SSubVector
{
    // Denote a sub-vector of BaseVector starting at StartElementOffset.
    //
    // It is used as an aliasee in the pair <Value, SSubVector>, thus the
    // size of the sub-vector is the size of Value (aliaser) of this pair.
    // (If needed, add the number of elements in SSubVector.)
    llvm::Value* Val;           // Either scalar or sub-vector
    llvm::Value* BaseVector;
    short  StartElementOffset;  // in the unit of BaseVector's element type
};

//  Represent a Vector's element at index = EltIx.
struct SVecElement {
    llvm::Value* Vec;
    int          EltIx;

    SVecElement() : Vec(nullptr), EltIx(-1) {}
};

/// RPE based analysis for querying variable reuse status.
///
/// Let two instructions DInst and UInst be defined in the same basic block,
///
/// DInst = ...
/// UInst = DInst op Other
///
/// and assume it is legal to use the same CVariable for DInst and UInst. This
/// analysis determines if this reuse will be applied or not. When overall
/// register pressure is low, this decision could be most aggressive. When DInst
/// and UInst are acrossing a high pressure region (defined below), then the
/// reuse will only be applied less aggressively.
///
/// Denote by RPE(x) the estimated register pressure at point x. Let Threshold
/// be a predefined threshold constant. We say pair (DInst, UInst) is crossing a
/// high register pressure region if
///
/// (1) RPE(x) >= Threshold for any x between DInst and UInst (inclusive), or
/// (2) RPE(x) >= Threshold for any use x of UInst.
///
class VariableReuseAnalysis : public llvm::FunctionPass,
                              public llvm::InstVisitor<VariableReuseAnalysis>
{
public:
  static char ID;

  VariableReuseAnalysis();
  ~VariableReuseAnalysis() {}

  typedef llvm::DenseMap<llvm::Value*, SSubVector> ValueAliasMapTy;
  typedef llvm::DenseMap<llvm::Value*, llvm::TinyPtrVector<llvm::Value*> > AliasRootMapTy;
  typedef llvm::SmallVector<SVecElement, 32> VecEltTy;
  typedef llvm::SmallVector<llvm::Value*, 32> ValueVectorTy;

  virtual bool runOnFunction(llvm::Function &F) override;

  // Need to perform this after WI/LiveVars/DeSSA/CoalescingEnging.
  // (todo: check if coalescing can be merged into dessa completely)
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    // AU.addRequired<RegisterEstimator>();
    AU.setPreservesAll();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
    AU.addRequired<WIAnalysis>();
    AU.addRequired<LiveVarsAnalysis>();
    AU.addRequired<CodeGenPatternMatch>();
    AU.addRequired<DeSSA>();
    AU.addRequired<CoalescingEngine>();
    AU.addRequired<BlockCoalescing>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  llvm::StringRef getPassName() const override {
    return "VariableReuseAnalysis";
  }

  /// Initialize per-function states. In particular, check if the entire function
  /// has a low pressure.
  void BeginFunction(llvm::Function *F, unsigned SimdSize) {
    m_SimdSize = (uint16_t)SimdSize;
    if (m_RPE) {
      if (m_RPE->isGRFPressureLow(m_SimdSize))
        m_IsFunctionPressureLow = Status::True;
      else
        m_IsFunctionPressureLow = Status::False;
    }
  }

  bool isCurFunctionPressureLow() const {
    return m_IsFunctionPressureLow == Status::True;
  }

  bool isCurBlockPressureLow() const {
    return m_IsBlockPressureLow == Status::True;
  }

  /// RAII class to initialize and cleanup basic block level cache.
  class EnterBlockRAII {
  public:
    explicit EnterBlockRAII(VariableReuseAnalysis *VRA, llvm::BasicBlock *BB)
        : VRA(VRA) {
      VRA->BeginBlock(BB);
    }
    ~EnterBlockRAII() { VRA->EndBlock(); }
    VariableReuseAnalysis *VRA;
  };
  friend class EnterBlockRAII;

  // Check use instruction's legality and its pressure impact.
  bool checkUseInst(llvm::Instruction *UInst, LiveVars *LV);

  // Check def instruction's legality and its pressure impact.
  bool checkDefInst(llvm::Instruction *DInst, llvm::Instruction *UInst,
                    LiveVars *LV);

  bool isLocalValue(llvm::Value* V);
 
  bool aliasHasInterference(llvm::Value* Aliaser, llvm::Value* Aliasee);
  bool hasInterference(llvm::Value* V0, llvm::Value* V1);

  // Visitor
  void visitCallInst(llvm::CallInst& I);
  void visitCastInst(llvm::CastInst& I);
  void visitInsertElementInst(llvm::InsertElementInst& I);
  void visitExtractElementInst(llvm::ExtractElementInst& I);

  bool isAliasedValue(llvm::Value *V) {
      return (isAliaser(V) || isAliasee(V));
  }
  bool isAliaser(llvm::Value* V);
  bool isAliasee(llvm::Value* V);
  int getCongruentClassSize(llvm::Value* V);
  bool isSameSizeValue(llvm::Value* V0, llvm::Value* V1);

  // getRootValue():
  //   return dessa root value; if dessa root value
  //   is null, return itself.
  llvm::Value* getRootValue(llvm::Value* V);
  // getAliasRootValue()
  //   return alias root value if it exists, itself otherwise.   
  llvm::Value *getAliasRootValue(llvm::Value* V);

  /// printAlias - print value aliasing info in human readable form
  void printAlias(llvm::raw_ostream &OS, const llvm::Function* F = nullptr) const;
  /// dumpAalias - dump alias info to dbgs().
  void dumpAlias() const;

  // Collect aliases from subVector to base vector.
  ValueAliasMapTy m_ValueAliasMap; // aliaser -> aliasee
  // Reverse of m_ValueAliasMap.
  AliasRootMapTy    m_AliasRootMap;    // aliasee -> all its aliasers.

  // No need to emit code for instructions in this map due to aliasing
  llvm::DenseMap <llvm::Instruction*, int> m_HasBecomeNoopInsts;

  // For emitting livetime start to visa to assist liveness analysis
  //   1. m_LifetimeAt1stDefInBB :  aliasee -> BB
  //        Once a first def is encounted, add lifetime start and clear
  //        this map entry afterwards.
  //   2. m_LifetimeAtEndOfBB :  BB -> set of values
  //        Add lifetime start for all values in the set at the end of BB.
  llvm::DenseMap<llvm::Value*, llvm::BasicBlock*> m_LifetimeAt1stDefOfBB;
  llvm::DenseMap<llvm::BasicBlock*, llvm::TinyPtrVector<llvm::Value*> > m_LifetimeAtEndOfBB;

private:
  void reset() {
    m_SimdSize = 0;
    m_IsFunctionPressureLow = Status::Undef;
    m_IsBlockPressureLow = Status::Undef;
    m_ValueAliasMap.clear();
    m_AliasRootMap.clear();
    m_HasBecomeNoopInsts.clear();
  }

  // Initialize per-block states. In particular, check if the entire block has a
  // low pressure.
  void BeginBlock(llvm::BasicBlock *BB) {
    assert(m_SimdSize != 0);
    if (m_RPE) {
        CodeGenContext* context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        uint32_t BBPresure = m_RPE->getMaxLiveGRFAtBB(BB, m_SimdSize);
        if (BBPresure <= context->getNumGRFPerThread())
            m_IsBlockPressureLow = Status::True;
        else
            m_IsBlockPressureLow = Status::False;
    }
  }

  // Cleanup per-block states.
  void EndBlock() { m_IsBlockPressureLow = Status::Undef; }

  void visitLiveInstructions(llvm::Function* F);

  void postProcessing();

  // Return true if this instruction can be converted to an alias
  bool canBeAlias(llvm::CastInst* I);

  // If V has been payload-coalesced, return true.
  bool hasBeenPayloadCoalesced(llvm::Value *V) {
      return (m_coalescingEngine->GetValueCCTupleMapping(V) != nullptr);
  }

  void mergeVariables(llvm::Function *F);

  // Add entry to alias map.
  bool addAlias(
      llvm::Value* Aliaser,
      SSubVector& SVD);

  // Insert entry in maps and update maps. Invoked by addAlias().
  void insertAliasPair(llvm::Value* Aliaser, SSubVector& SV);

  // Returns true for the following pattern:
  //   a = extractElement <vectorType> EEI_Vec, <constant EEI_ix>
  //   b = insertElement  <vectorType> V1,  E,  <constant IEI_ix>
  // where EEI_ix and IEI_ix are constants; Return false otherwise.
  bool getVectorIndicesIfConstant(
      llvm::InsertElementInst* IEI,
      int& IEI_ix,
      llvm::Value*& EEI_Vec,
      int&EEI_ix);

  bool checkAndGetAllInsertElements(
      llvm::InsertElementInst* FirstIEI,
      ValueVectorTy& AllIEIs,
      VecEltTy& AllElts);

  bool IsExtractFrom(
      VecEltTy& AllElts,
      llvm::InsertElementInst* FirstIEI,
      llvm::InsertElementInst* LastIEI,
      SSubVector& SV);

  bool IsInsertTo(
      VecEltTy& AllElts,
      llvm::InsertElementInst* FirstIEI,
      llvm::InsertElementInst* LastIEI,
      llvm::SmallVector<SSubVector, 4>& SVs);

  void getAllValues(
      llvm::SmallVector<llvm::Value*, 8>& AllValues,
      llvm::Value* Aliasee);

  CodeGenContext* m_pCtx;
  WIAnalysis* m_WIA;
  LiveVars* m_LV;
  DeSSA* m_DeSSA;
  CodeGenPatternMatch* m_PatternMatch;
  CoalescingEngine* m_coalescingEngine;
  llvm::DominatorTree *m_DT;
  const llvm::DataLayout* m_DL;


  /// Current Function; set on entry to runOnFunction
  /// and unset on exit to runOnFunction
  llvm::Function* m_F;

  // The register pressure estimator (optional).
  RegisterEstimator *m_RPE;

  // Results may be cached at kernel level or basic block level. Use the
  // following enum to indicate cached flag status.
  enum class Status : int8_t {
    Undef = -1,
    False = 0,
    True = 1
  };

  // Per SIMD-compilation constant. Each compilation needs to initialize the
  // SIMD mode.
  uint16_t m_SimdSize;

  // When this function has low register pressure, reuse can be applied
  // aggressively without checking each individual def-use pair.
  Status m_IsFunctionPressureLow;

  // When this block has low register pressure, reuse can be applied
  // aggressively without checking each individual def-use pair.
  Status m_IsBlockPressureLow;

  // Temporaries
  //SmallPtrSet<llvm::Instruction*, 16> m_Visited;
  ValueAliasMapTy m_ExtractFrom;
  ValueAliasMapTy m_insertTo;
};

llvm::FunctionPass *createVariableReuseAnalysisPass();

} // namespace IGC
