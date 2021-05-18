/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGNORERAUWVALUEMAP_H
#define IGNORERAUWVALUEMAP_H
#include "llvm/IR/ValueMap.h"

namespace llvm {

// Configuration for ValueMap that ignores RAUW, instead of moving the map
// entry.
template<typename ValueTy>
struct IgnoreRAUWValueMapConfig : public ValueMapConfig<ValueTy> {
  enum { FollowRAUW = false };
};

} // End llvm namespace

#endif // ndef IGNORERAUWVALUEMAP_H
