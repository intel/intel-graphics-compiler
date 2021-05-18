/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_CM_CL_SUPPORT_BUILTIN_TRANSLATOR_H
#define VC_CM_CL_SUPPORT_BUILTIN_TRANSLATOR_H

#include <llvm/IR/Module.h>

namespace cmcl {

// Translates CM-CL builtin into legal for VC backend IR
// (e.g. into vc-intrinsics).
bool translateBuiltins(llvm::Module &M);

} // namespace cmcl

#endif // VC_CM_CL_SUPPORT_BUILTIN_TRANSLATOR_H
