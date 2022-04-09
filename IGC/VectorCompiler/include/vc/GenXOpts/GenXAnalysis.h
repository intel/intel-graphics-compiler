/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file declares some hooks that are injected into llvm analysis library
// files to make them work with genx related stuff.
//
//===----------------------------------------------------------------------===//


#ifndef VC_GENXOPTS_GENXANALYSIS_H
#define VC_GENXOPTS_GENXANALYSIS_H

namespace llvm {

template <typename T> class ArrayRef;
class CallInst;
class Constant;
class DataLayout;
class Instruction;
class ImmutableCallSite;
class Type;
class Use;
class Value;
class Function;

/// canConstantFoldGenXIntrinsic - Return true if it is even possible to fold
/// a call to the specified GenX intrinsic.
bool canConstantFoldGenXIntrinsic(unsigned IID);

/// ConstantFoldGenXIntrinsic - Attempt to constant fold a call to the
/// specified GenX intrinsic with the specified arguments, returning null if
/// unsuccessful.
Constant *ConstantFoldGenXIntrinsic(unsigned IID, Type *RetTy,
                                    ArrayRef<Constant *> Operands,
                                    Instruction *CSInst, const DataLayout &DL);

/// ConstantFoldGenX - Attempt to constant fold genx-related instruction (intrinsic).
/// This function tries to fold operands and then tries to fold instruction
/// itself. Returns nullptr if folding was unsuccessful.
Constant *ConstantFoldGenX(Instruction *I, const DataLayout &DL);

/// Given a GenX intrinsic and a set of arguments, see if we can fold the
/// result.
///
/// If this call could not be simplified returns null.
Value *SimplifyGenXIntrinsic(unsigned IID, Type *RetTy, Use *ArgBegin,
                             Use *ArgEnd, const DataLayout &DL);

/// Given a GenX related instruction, see if we can fold the
/// result. This function tries simplification and then constant folding.
///
/// If this instruction could not be simplified returns null.
Value *SimplifyGenX(CallInst *I, const DataLayout &DL);

// simplifyWritesWithUndefInput - removes write instrinsics (currently wrregion,
// wrpredregion) that have undef as input value, replaces all uses with the old
// value. If this replacement produced new context (write intrinsic's input
// value was replaced with undef), those writes are cleaned up too. No writes
// with undef input should be left in the function \p F as the result.
//
// Returns whether the function was modified.
bool simplifyWritesWithUndefInput(Function &F);

//===----------------------------------------------------------------------===//
//
// getValueAlignmentInBytes - calculate alignment of value
// used in CMKernelArgOffset and GenXLiveness analysis
// defined in CMKernelArgOffset
//

unsigned getValueAlignmentInBytes(const Value &Val, const DataLayout &DL);

} // end namespace llvm

#endif // VC_GENXOPTS_GENXANALYSIS_H
