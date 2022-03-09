/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// IndexFlattener : aggregate index <-> flattened index conversion
/// ---------------------------------------------------------------
///
/// This namespace contains some utility functions to convert between indices of
/// aggregate (as found in an extractelement instruction) and a flattened index,
/// in which an aggregate containing further aggregates is flattened as if it is
/// a single struct containing just the non-aggregate elements.
///
/// SimpleValue uses this to encode and decode its flattened index.
/// Liveness and coalescing use flattenArg and getNumArgElements to calculate
/// live ranges for function args at the call sites.
///
//===----------------------------------------------------------------------===//

#ifndef VC_UTILS_GENX_INDEX_FLATTENER_H
#define VC_UTILS_GENX_INDEX_FLATTENER_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm {

class Type;
class FunctionType;

namespace IndexFlattener {
// getNumElements : get the number of non-aggregate elements in the flattened
// aggregate. Returns 1 if it is not an aggregate type, but 0 for void type.
unsigned getNumElements(Type *Ty);

// getElementType : get type of aggregate element from flattened index.
Type *getElementType(Type *Ty, unsigned FlattenedIndex);

// flatten : convert aggregate indices into a flattened index.
unsigned flatten(Type *AggrTy, ArrayRef<unsigned> Indices);

// unflatten : convert a flattened index back into normal aggregate indices.
SmallVector<unsigned, 4> unflatten(Type *AggrTy, unsigned FlattenedIndex);

// flattenArg : flatten an arg in a function or call, i.e. calculate the total
// number of flattened indices used up by previous args. If all previous args
// are not aggregate type, then this just returns the arg index.
unsigned flattenArg(FunctionType *FT, unsigned ArgIndex);

// getNumArgElements : get the number of non-aggregate elements in all args of
// the function.
unsigned getNumArgElements(FunctionType *FT);

} // end namespace IndexFlattener
} // end namespace llvm

#endif
