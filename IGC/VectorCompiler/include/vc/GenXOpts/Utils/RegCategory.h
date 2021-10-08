/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_GENXOPTS_UTILS_REGCATEGORY_H
#define VC_GENXOPTS_UTILS_REGCATEGORY_H

namespace llvm {
namespace genx {

// The encoding for register category, used in GenXCategory,
// GenXLiveness and GenXVisaRegAlloc.  It is an anonymous enum inside a class
// rather than a named enum so you don't need to cast to/from int.
struct RegCategory {
  enum {
    NONE,
    GENERAL,
    ADDRESS,
    PREDICATE,
    SAMPLER,
    SURFACE,
    NUMREALCATEGORIES,
    EM,
    RM,
    NUMCATEGORIES
  };
};

} // namespace genx
} // namespace llvm

#endif // VC_GENXOPTS_UTILS_REGCATEGORY_H
