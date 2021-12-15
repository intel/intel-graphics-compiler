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

// Returns whether \p F is a function which signature cannot be changed.
// Indirectly called function signature cannot be changed in general case, so
// this function returns true for such function (though some optimizations may
// change it for some cases). False is returned for a kerenl.
bool isFixedSignatureFunc(const llvm::Function &F);

// Checks whether \p F is a definition and its signature cannot be changed.
inline bool isFixedSignatureDefinition(const llvm::Function &F) {
  if (F.isDeclaration())
    return false;
  return isFixedSignatureFunc(F);
}

} // namespace vc

#endif /* end of include guard: VC_UTILS_GENERAL_FUNCTION_ATTRS */
