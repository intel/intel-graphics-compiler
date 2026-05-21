/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_CMPPREDICATE_H
#define IGCLLVM_IR_CMPPREDICATE_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instructions.h>

#if LLVM_VERSION_MAJOR >= 20
#include "llvm/IR/CmpPredicate.h"
#endif

#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {

#if LLVM_VERSION_MAJOR < 20
using ICmpInstPredicate = llvm::ICmpInst::Predicate;
#else
using ICmpInstPredicate = llvm::CmpPredicate;
#endif

#if LLVM_VERSION_MAJOR < 20
using CmpInstPredicate = llvm::CmpInst::Predicate;
#else
using CmpInstPredicate = llvm::CmpPredicate;
#endif

#if LLVM_VERSION_MAJOR < 20
using FCmpInstPredicate = llvm::FCmpInst::Predicate;
#else
using FCmpInstPredicate = llvm::CmpPredicate;
#endif

} // namespace IGCLLVM

#endif
