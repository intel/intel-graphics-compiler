/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/General/IndexFlattener.h"
#include "Probe/Assertion.h"
#include "llvm/IR/DerivedTypes.h"

namespace llvm {
namespace IndexFlattener {

// Returns the type of an aggregate's element at specific index. This is a
// generalization for structures and arrays.
static Type *getElementTypeOfAggregate(Type *AggrTy, unsigned Index) {
  IGC_ASSERT_MESSAGE(AggrTy->isAggregateType(), "unexpected type");
  if (isa<StructType>(AggrTy))
    return cast<StructType>(AggrTy)->getTypeAtIndex(Index);
  IGC_ASSERT_MESSAGE(Index < cast<ArrayType>(AggrTy)->getNumElements(),
                     "invalid array index");
  return cast<ArrayType>(AggrTy)->getElementType();
}

// Returns the number of elements of an aggregate. This is a generalization for
// structures and arrays.
static unsigned getNumElementsOfAggregate(Type *AggrTy) {
  IGC_ASSERT_MESSAGE(AggrTy->isAggregateType(), "unexpected type");
  if (isa<StructType>(AggrTy))
    return cast<StructType>(AggrTy)->getNumElements();
  return cast<ArrayType>(AggrTy)->getNumElements();
}

/***********************************************************************
 * getNumElements : get the number of non-aggregate elements in
 * the flattened aggregate. Returns 1 if it is not an aggregate type, but 0 for
 * void type.
 */
unsigned getNumElements(Type *Ty) {
  if (Ty->isAggregateType()) {
    unsigned NumElements = 0;
    for (unsigned i = 0, e = getNumElementsOfAggregate(Ty); i != e; ++i)
      NumElements += getNumElements(getElementTypeOfAggregate(Ty, i));
    return NumElements;
  }
  return !Ty->isVoidTy();
}

/***********************************************************************
 * getElementType : get type of aggregate element from
 *    flattened index
 *
 * Enter:   Ty = type, possibly aggregate type
 *          FlattenedIndex = flattened index in the aggregate, 0 if not
 *          aggregate
 *
 * Return:  type of that element
 */
Type *getElementType(Type *Ty, unsigned FlattenedIndex) {
  if (!Ty->isAggregateType())
    return Ty;
  SmallVector<unsigned, 4> Indices = unflatten(Ty, FlattenedIndex);
  IGC_ASSERT(FlattenedIndex == flatten(Ty, Indices));
  for (unsigned Index : Indices)
    Ty = getElementTypeOfAggregate(Ty, Index);
  return Ty;
}

/***********************************************************************
 * flatten : convert aggregate indices into a flattened index
 *
 * This involves scanning through the aggregate layout each time it is called.
 * If it is used a lot, it might benefit from some cacheing of the results.
 */
unsigned flatten(Type *AggrTy, ArrayRef<unsigned> Indices) {
  IGC_ASSERT_MESSAGE(AggrTy->isAggregateType(), "unexpected type");
  if (Indices.empty())
    return 0;
  unsigned Flattened = 0;
  for (unsigned i = 0; i < Indices[0]; ++i)
    Flattened += getNumElements(getElementTypeOfAggregate(AggrTy, i));
  Type *EltTy = getElementTypeOfAggregate(AggrTy, Indices[0]);
  if (EltTy->isAggregateType())
    Flattened += flatten(EltTy, Indices.drop_front());
  return Flattened;
}

/***********************************************************************
 * unflatten : convert flattened index into aggregate indices
 *
 * Enter:   AggrTy = aggregate type
 *          Flattened = flattened index in the aggregate
 *
 * Return:  vector with unflattened indices
 *
 * This involves scanning through the aggregate layout each time it is called.
 * If it is used a lot, it might benefit from some cacheing of the results.
 */
SmallVector<unsigned, 4> unflatten(Type *AggrTy, unsigned Flattened) {
  IGC_ASSERT_MESSAGE(AggrTy->isAggregateType(), "unexpected type");
  unsigned Index = 0, NumElems = getNumElementsOfAggregate(AggrTy);
  Type *EltTy = nullptr;
  for (; Index != NumElems; ++Index) {
    EltTy = getElementTypeOfAggregate(AggrTy, Index);
    auto EltSize = getNumElements(EltTy);
    if (EltSize > Flattened)
      break;
    Flattened -= EltSize;
  }
  SmallVector<unsigned, 4> Indices({Index});
  if (EltTy->isAggregateType()) {
    auto EltIndices = unflatten(EltTy, Flattened);
    Indices.append(EltIndices.begin(), EltIndices.end());
  }
  return Indices;
}

/***********************************************************************
 * flattenArg : flatten an arg in a function or call
 *
 * This calculates the total number of flattened indices used up by previous
 * args. If all previous args are not aggregate type, then this just returns the
 * arg index.
 */
unsigned flattenArg(FunctionType *FT, unsigned ArgIndex) {
  unsigned FlattenedIndex = 0;
  for (unsigned i = 0; i < ArgIndex; ++i)
    FlattenedIndex += getNumElements(FT->getParamType(i));
  return FlattenedIndex;
}

/***********************************************************************
 * getNumArgElements : get the number of non-aggregate elements
 * in all args of the function.
 */
unsigned getNumArgElements(FunctionType *FT) {
  return flattenArg(FT, FT->getNumParams());
}
} // namespace IndexFlattener
} // namespace llvm
