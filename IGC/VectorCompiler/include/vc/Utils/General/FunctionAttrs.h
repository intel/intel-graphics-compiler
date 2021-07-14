/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENERAL_FUNCTION_ATTRS
#define VC_UTILS_GENERAL_FUNCTION_ATTRS

#include <llvm/IR/Function.h>

namespace vc {

// Transfers DISubporgram node from one function to another
void transferDISubprogram(llvm::Function &From, llvm::Function &To);

// Transfers name and calling convention from one function to another
// and set custom attributes
void transferNameAndCCWithNewAttr(const llvm::AttributeList Attrs,
                                  llvm::Function &From, llvm::Function &To);
} // namespace vc

#endif /* end of include guard: VC_UTILS_GENERAL_FUNCTION_ATTRS */
