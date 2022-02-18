/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENERAL_IRBUILDER_H
#define VC_UTILS_GENERAL_IRBUILDER_H

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>

#include <numeric>

namespace llvm {
class DataLayout;
class GlobalVariable;
} // namespace llvm

namespace vc {

// Creates ptrtoint instruction that does no zero extension or truncation
// (destination type size is equal to the provided pointer size according to
// the provided data layout).
// Instructions are inserted via the provided \p Builder.
llvm::Value *createNopPtrToInt(llvm::Value &V, llvm::IRBuilder<> &Builder,
                               const llvm::DataLayout &DL);

// Creates a sequence of instructions that casts the provided value to \p DestTy
// type. The sequence is bitcast, or ptrtoint, or ptrtoint + bitcast depending
// on \p V and \p DestTy. \p DestTy type must be an integer or floating point
// type or a vector of such types. The sizes of \p V type and \p DestTy type
// must match according to the provided data layout \p DL.
// Instructions are inserted via the provided \p Builder.
llvm::Value *castToIntOrFloat(llvm::Value &V, llvm::Type &DestTy,
                              llvm::IRBuilder<> &Builder,
                              const llvm::DataLayout &DL);

// Creates a sequence of instructions that casts the provided value to \p DestTy
// type. The sequence is bitcast, or inttoptr, or bitcast + inttoptr depending
// on \p V and \p DestTy. \p V type must be an integer or floating point type
// or a vector of such types. The sizes of \p V type and \p DestTy type must
// match according to the provided data layout \p DL.
// Instructions are inserted via the provided \p Builder.
llvm::Value *castFromIntOrFloat(llvm::Value &V, llvm::Type &DestTy,
                                llvm::IRBuilder<> &Builder,
                                const llvm::DataLayout &DL);

// Creates floating point type with specified number of bits.
llvm::Type *getFloatNTy(llvm::IRBuilder<> &Builder, unsigned N);

// Cast one-element result of instruction  to scalar.
llvm::Instruction *fixDegenerateVector(llvm::Instruction &Inst,
                                       llvm::IRBuilder<> &Builder);

// Checks whether \p Op is an address space cast operator, which casts a
// pointer to generic address space pointer.
bool isCastToGenericAS(const llvm::Value &Op);

// Creates GEP constant expression that effectively implements implicit cast
// of an array to its first element pointer.
// \p Array must be a global variable of an array type.
llvm::Constant &castArrayToFirstElemPtr(llvm::GlobalVariable &Array);

bool isBitCastAllowed(llvm::Value &Val, llvm::Type &DstType);

// Creates a vector with \p VectorWidth elements that consist of values that
// are provided in \p Elements range.
// Arguments:
//    \p Elements - a range of llvm::Value* or elements that are convertable
//                  to llvm::Value*, all values must have the same type, the
//                  range size must match \p VectorWidth.
//    \p VectorWidth - a width of a requested vector.
//    \p IRB - IR builder used to produce the value.
//    \p Name - a name for the generated vector, and prefix for other
//              constructed value names.
template <typename ForwardRange>
llvm::Value *accumulateVector(ForwardRange &&Elements, int VectorWidth,
                              llvm::IRBuilder<> &IRB,
                              const llvm::Twine &Name = "vec") {
  llvm::Value *Result = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(
      (*Elements.begin())->getType(), VectorWidth));
  auto ElementsWithIndices = enumerate(Elements);
  Result = std::accumulate(
      ElementsWithIndices.begin(), ElementsWithIndices.end(), Result,
      [&IRB, &Name](llvm::Value *Accumulator, auto ElementWithIndex) {
        return IRB.CreateInsertElement(
            Accumulator, ElementWithIndex.value(), ElementWithIndex.index(),
            Name + ".insert." + llvm::Twine(ElementWithIndex.index()));
      });
  Result->setName(Name);
  return Result;
}

} // namespace vc

#endif // VC_UTILS_GENERAL_IRBUILDER_H
