/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_INTRINSICS_WRAPPER_H
#define VC_UTILS_INTRINSICS_WRAPPER_H

#include "vc/InternalIntrinsics/InternalIntrinsics.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"

namespace vc {
unsigned getAnyIntrinsicID(const llvm::Function *F);
unsigned getAnyIntrinsicID(const llvm::Value *V);
} // namespace vc

#endif /* end of include guard: VC_UTILS_INTRINSICS_WRAPPER_H */
