/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INSTVISITOR_H
#define IGCLLVM_IR_INSTVISITOR_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/InstVisitor.h"

#if LLVM_VERSION_MAJOR == 8

// Convert visitTerminatorInst delegate name (from llvm version < 7) to new signature visitTerminator
#define visitTerminatorInst(ARG) visitTerminator(ARG)

#endif


#endif
