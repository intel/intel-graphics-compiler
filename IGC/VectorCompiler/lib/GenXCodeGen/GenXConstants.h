/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef LIB_GENXCODEGEN_GENXCONSTANTS_H
#define LIB_GENXCODEGEN_GENXCONSTANTS_H

#include "GenXSubtarget.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"

#include "Probe/Assertion.h"

namespace llvm {
namespace genx {

// ConstantLoader : class to insert instruction(s) to load a constant
class ConstantLoader {
  friend bool loadNonSimpleConstants(
    Instruction *Inst, const GenXSubtarget &Subtarget, const DataLayout &DL,
    SmallVectorImpl<Instruction *> *AddedInstructions);

  Constant *C;
  Instruction *User;

  const GenXSubtarget &Subtarget;
  const DataLayout &DL;

  // NewC != nullptr signals that we should replace C with NewC in User
  // nothing to do otherwise
  Constant *NewC = nullptr;
  // AddedInstructions: a vector that the caller has requested any added
  // instructions to be pushed in to.
  SmallVectorImpl<Instruction *> *AddedInstructions;
  // Info from analyzing for possible packed vector constant.
  int64_t PackedIntScale = 0;  // amount to scale packed int vector by
  int64_t PackedIntAdjust; // amount to adjust by, special casing 0 or -8
                           //  when PackedIntScale is 1
  unsigned PackedIntMax;   // max value in packed vector, used when scale is
                           //  1 and adjust is 0 to tell whether it would fit
                           //  in 0..7
  bool PackedFloat = false;

public:
  // Constructor
  // User = the instruction that uses the constant. If this is genx.constanti,
  //        then a packed vector constant can be an isSimple() constant even
  //        when the element type is not i16. Also used to disallow a packed
  //        vector constant in a logic op. If User==0 then it is assumed that
  //        a packed vector constant with an element type other than i16 is OK.
  // AddedInstructions = vector to add new instructions to when loading a
  //        non simple constant, so the caller can see all the newly added
  //        instructions.
  ConstantLoader(Constant *C, const GenXSubtarget &InSubtarget,
                 const DataLayout &InDL, Instruction *User = nullptr,
                 SmallVectorImpl<Instruction *> *AddedInstructions = nullptr)
      : C(C), User(User), Subtarget(InSubtarget), DL(InDL),
        AddedInstructions(AddedInstructions) {
    IGC_ASSERT_MESSAGE(!C->getType()->isAggregateType(),
                       "aggregate types are not supported by constant loader");
    analyze();
  }

  Instruction *load(Instruction *InsertBefore);
  Instruction *loadBig(Instruction *InsertBefore);

  bool isBigSimple() const;
  bool isSimple() const;
  bool isLegalSize() const;

private:
  bool allowI64Ops() const;

  void analyze();
  void analyzeForPackedInt(unsigned NumElements);
  void analyzeForPackedFloat(unsigned NumElements);

  bool isPackedIntVector() const;
  bool isPackedFloatVector() const;

  Constant *getConsolidatedConstant(Constant *C);
  unsigned getRegionBits(unsigned NeededBits, unsigned OptionalBits,
                         unsigned VecWidth);

  bool needFixingSimple() const { return NewC; }
  void fixSimple(int OperandIdx);

  Instruction *loadNonSimple(Instruction *InsertBefore);
  Instruction *loadSplatConstant(Instruction *InsertPt);
  Instruction *loadNonPackedIntConst(Instruction *InsertPt);
};

// Some instructions force their operands to be constants.
// Check here if operand of instruction must be constant.
inline bool opMustBeConstant(Instruction *I, unsigned OpNum) {
  // Mask of shufflevector should always be constant.
  if (isa<ShuffleVectorInst>(I))
    return OpNum == 2;
  return false;
}

// Check whether types of two contsants are bitcastable and
// then check constants' contents. Most part of implementation is taken
// from FunctionComparator::cmpConstants
bool areConstantsEqual(const Constant *C1, const Constant *C2);

// Remove all genx.constant* intrinsics that have non-constant source
bool cleanupConstantLoads(Function *F);

// Load a constant using the llvm.genx.constant intrinsic.
inline Instruction *
loadConstant(Constant *C, Instruction *InsertBefore,
             const GenXSubtarget &Subtarget, const DataLayout &DL,
             SmallVectorImpl<Instruction *> *AddedInstructions = nullptr) {
  return ConstantLoader(C, Subtarget, DL, nullptr, AddedInstructions)
      .load(InsertBefore);
}

// Load non-simple constants used in an instruction.
bool loadNonSimpleConstants(
    Instruction *Inst, const GenXSubtarget &Subtarget, const DataLayout &DL,
    SmallVectorImpl<Instruction *> *AddedInstructions = nullptr);

bool loadConstantsForInlineAsm(
    CallInst *Inst, const GenXSubtarget &Subtarget, const DataLayout &DL,
    SmallVectorImpl<Instruction *> *AddedInstructions = nullptr);

// Load constants used in an instruction.
bool loadConstants(Instruction *Inst, const GenXSubtarget &Subtarget,
                   const DataLayout &DL);

// Load constants used in phi nodes in a function.
bool loadPhiConstants(Function &F, DominatorTree *DT,
                      const GenXSubtarget &Subtarget, const DataLayout &DL,
                      bool ExcludePredicate = false);

// Check if constant vector is replicated, e.g.
// Original vector:  1, 1, 1, 1, 0, 0, 0, 0
// Replicate vector: 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0
bool isReplicatedConstantVector(const ConstantVector *Orig,
                                const ConstantVector *ReplicateCandidate);

} // namespace genx
} // namespace llvm

#endif // LIB_GENXCODEGEN_GENXCONSTANTS_H
