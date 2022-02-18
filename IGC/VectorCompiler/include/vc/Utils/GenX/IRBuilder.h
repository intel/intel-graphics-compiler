/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_IRBUILDER_H
#define VC_UTILS_GENX_IRBUILDER_H

#include "Probe/Assertion.h"

#include <llvm/IR/IRBuilder.h>

namespace llvm {
class Value;
} // namespace llvm

namespace vc {

// Produces IR that reads group thread ID from r0 register (r0.2[0:7]).
// The produced IR is valid only for target that have payload in memory (PIM).
llvm::Value *getGroupThreadIDForPIM(llvm::IRBuilder<> &IRB);

} // namespace vc

#endif // VC_UTILS_GENX_IRBUILDER_H
