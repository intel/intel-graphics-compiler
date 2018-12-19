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


#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/DenseMap.h"
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
	// BaseVector is a vector type. It denotes a sub-vector
	// of BaseVector starting at StartElementOffset.
	llvm::Value* BaseVector;
	short  StartElementOffset;
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

  virtual bool runOnFunction(llvm::Function &F) override;

  // Need to perform this after WI/LiveVars/DeSSA/CoalescingEnging.
  // (todo: check if coalescing can be merged into dessa completely)
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    // AU.addRequired<RegisterEstimator>();
    AU.setPreservesAll();
	AU.addRequired<WIAnalysis>();
	AU.addRequired<LiveVarsAnalysis>();
	AU.addRequired<CodeGenPatternMatch>();
	AU.addRequired<DeSSA>();
    AU.addRequired<CoalescingEngine>();
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
 
  bool hasInterference(llvm::Value* V0, llvm::Value* V1);

  // Visitor
  void visitCallInst(llvm::CallInst& I);
  void visitCastInst(llvm::CastInst& I);


  // Collect aliases from subVector to base vector. The map's key is
  // assumed to be an independent value (not in any congruent class)
  // originally. Of course, after aliasing, it must be in a congruent
  // class.
  ValueAliasMapTy m_ValueAliasMap;

private:
  void reset() {
    m_SimdSize = 0;
    m_IsFunctionPressureLow = Status::Undef;
    m_IsBlockPressureLow = Status::Undef;
	m_ValueAliasMap.clear();
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

  CodeGenContext* m_pCtx;
  WIAnalysis* m_WIA;
  LiveVars* m_LV;
  DeSSA* m_DeSSA;
  CodeGenPatternMatch* m_PatternMatch;
  CoalescingEngine* m_coalescingEngine;

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
};

llvm::FunctionPass *createVariableReuseAnalysisPass();

} // namespace IGC
