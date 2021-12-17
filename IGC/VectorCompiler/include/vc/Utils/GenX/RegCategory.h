/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_REGCATEGORY_H
#define VC_UTILS_GENX_REGCATEGORY_H

namespace vc {

// The encoding for register category, used in GenXCategory,
// GenXLiveness and GenXVisaRegAlloc.  It is an anonymous enum inside a class
// rather than a named enum so you don't need to cast to/from int.
namespace RegCategory {
enum Enum {
  None,
  General,
  Address,
  Predicate,
  Sampler,
  Surface,
  NumRealCategories,
  EM,
  RM,
  NumCategories
};
} // namespace RegCategory

} // namespace vc

#endif // VC_UTILS_GENX_REGCATEGORY_H
