/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENERAL_BREAK_CONST_H
#define VC_UTILS_GENERAL_BREAK_CONST_H

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Value.h>

namespace vc {
// breakConstantVector : break vector of constexprs into a sequence of
//                       InsertElementInsts.
// CV - vector to break
// CurInst - Instruction CV is a part of
// InsertPt - point to insert new instructions at
// Return the last InsertElementInst in the resulting chain,
// or nullptr if there're no constexprs in CV.
llvm::Value *breakConstantVector(llvm::ConstantVector *CV,
                                 llvm::Instruction *CurInst,
                                 llvm::Instruction *InsertPt);
// breakConstantExprs : break constant expressions in instruction I.
// Return true if any modifications have been made, false otherwise.
bool breakConstantExprs(llvm::Instruction *I);
// breakConstantExprs : break constant expressions in function F.
// Return true if any modifications have been made, false otherwise.
bool breakConstantExprs(llvm::Function *F);

} // namespace vc

#endif // VC_UTILS_GENERAL_BREAK_CONST_H
