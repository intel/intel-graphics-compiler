/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_IRBUILDER_H
#define VC_UTILS_GENX_IRBUILDER_H

#include "vc/Utils/GenX/Region.h"

#include "Probe/Assertion.h"

#include <llvm/ADT/Twine.h>
#include <llvm/IR/IRBuilder.h>

namespace llvm {
class Value;
class CallInst;
} // namespace llvm

namespace vc {

// Produces IR that reads group thread ID from r0 register (r0.2[0:7]).
// The produced IR is valid only for target that have payload in memory (PIM).
llvm::Value *getGroupThreadIDForPIM(llvm::IRBuilder<> &IRB);

// Produces IR that reads a region \p R from a vISA variable \p Variable.
llvm::CallInst *
createReadVariableRegion(llvm::GlobalVariable &Variable,
                         const llvm::CMRegion &R, llvm::IRBuilder<> &IRB,
                         const llvm::Twine &Name = "rd.var.rgn");

// Produces IR that reads a whole vISA variable \p Variable.
inline llvm::CallInst *
createReadVariableRegion(llvm::GlobalVariable &Variable, llvm::IRBuilder<> &IRB,
                         const llvm::Twine &Name = "rd.var.rgn") {
  return createReadVariableRegion(Variable, {Variable.getValueType()}, IRB,
                                  Name);
}

// Produces IR that writes \p Input to a region \p R of a vISA variable
// \p Variable.
llvm::CallInst *createWriteVariableRegion(llvm::GlobalVariable &Variable,
                                          llvm::Value &Input,
                                          const llvm::CMRegion &R,
                                          llvm::IRBuilder<> &IRB);

// Produces IR that writes \p Input to a whole vISA variable \p Variable.
inline llvm::CallInst *createWriteVariableRegion(llvm::GlobalVariable &Variable,
                                                 llvm::Value &Input,
                                                 llvm::IRBuilder<> &IRB) {
  return createWriteVariableRegion(Variable, Input, {Variable.getValueType()},
                                   IRB);
}

} // namespace vc

#endif // VC_UTILS_GENX_IRBUILDER_H
