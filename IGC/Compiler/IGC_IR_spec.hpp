/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file contains description of LLVM types, instructions and intrinsic functions
// allowed by IGC IR. This specification header is included in VerificationPass.hpp
// and VerificationPass.cpp.
//

#include "llvm/Config/llvm-config.h"

//=======================================================================================
// TYPES

// Floating-point types
//--------------------------
//             name    size
//--------------------------
IGC_IR_FP_TYPE(half, 16)
IGC_IR_FP_TYPE(float, 32)
IGC_IR_FP_TYPE(double, 64)

// Vector sizes
//------------------------------------
//                 name       size
//                         in elements
//------------------------------------
IGC_IR_VECTOR_TYPE(x1, 1)
IGC_IR_VECTOR_TYPE(x2, 2)
IGC_IR_VECTOR_TYPE(x3, 3)
IGC_IR_VECTOR_TYPE(x4, 4)
IGC_IR_VECTOR_TYPE(x5, 5)
IGC_IR_VECTOR_TYPE(x6, 6)
IGC_IR_VECTOR_TYPE(x7, 7)
IGC_IR_VECTOR_TYPE(x8, 8)
IGC_IR_VECTOR_TYPE(x9, 9)
IGC_IR_VECTOR_TYPE(x10, 10)
IGC_IR_VECTOR_TYPE(x11, 11)
IGC_IR_VECTOR_TYPE(x12, 12)
IGC_IR_VECTOR_TYPE(x13, 13)
IGC_IR_VECTOR_TYPE(x14, 14)
IGC_IR_VECTOR_TYPE(x15, 15)
IGC_IR_VECTOR_TYPE(x16, 16)
IGC_IR_VECTOR_TYPE(x32, 32)
IGC_IR_VECTOR_TYPE(x64, 64)
IGC_IR_VECTOR_TYPE(x128, 128)

// LLVM intrinsics supported by IGC IR
//------------------------------------
IGC_IR_LLVM_INTRINSIC(sqrt)
IGC_IR_LLVM_INTRINSIC(fabs)
IGC_IR_LLVM_INTRINSIC(copysign)
IGC_IR_LLVM_INTRINSIC(ceil)
IGC_IR_LLVM_INTRINSIC(floor)
IGC_IR_LLVM_INTRINSIC(trunc)
IGC_IR_LLVM_INTRINSIC(sin)
IGC_IR_LLVM_INTRINSIC(cos)
IGC_IR_LLVM_INTRINSIC(exp2)
IGC_IR_LLVM_INTRINSIC(log2)
IGC_IR_LLVM_INTRINSIC(ctpop)
IGC_IR_LLVM_INTRINSIC(pow)
IGC_IR_LLVM_INTRINSIC(fma)
IGC_IR_LLVM_INTRINSIC(ctlz)
IGC_IR_LLVM_INTRINSIC(memcpy)
IGC_IR_LLVM_INTRINSIC(memset)
IGC_IR_LLVM_INTRINSIC(memmove)
IGC_IR_LLVM_INTRINSIC(lifetime_start)
IGC_IR_LLVM_INTRINSIC(lifetime_end)
IGC_IR_LLVM_INTRINSIC(expect)
IGC_IR_LLVM_INTRINSIC(dbg_declare)
IGC_IR_LLVM_INTRINSIC(dbg_value)
IGC_IR_LLVM_INTRINSIC(uadd_with_overflow)
IGC_IR_LLVM_INTRINSIC(sadd_with_overflow)
IGC_IR_LLVM_INTRINSIC(usub_with_overflow)
IGC_IR_LLVM_INTRINSIC(ssub_with_overflow)
IGC_IR_LLVM_INTRINSIC(umul_with_overflow)
IGC_IR_LLVM_INTRINSIC(smul_with_overflow)
IGC_IR_LLVM_INTRINSIC(assume)
IGC_IR_LLVM_INTRINSIC(bswap)
IGC_IR_LLVM_INTRINSIC(maxnum)
IGC_IR_LLVM_INTRINSIC(minnum)
IGC_IR_LLVM_INTRINSIC(canonicalize)
IGC_IR_LLVM_INTRINSIC(stacksave)
IGC_IR_LLVM_INTRINSIC(stackrestore)
IGC_IR_LLVM_INTRINSIC(bitreverse)
IGC_IR_LLVM_INTRINSIC(lround)
IGC_IR_LLVM_INTRINSIC(llround)
IGC_IR_LLVM_INTRINSIC(fshl)
IGC_IR_LLVM_INTRINSIC(fshr)
IGC_IR_LLVM_INTRINSIC(usub_sat)
IGC_IR_LLVM_INTRINSIC(ssub_sat)
IGC_IR_LLVM_INTRINSIC(uadd_sat)
IGC_IR_LLVM_INTRINSIC(sadd_sat)
IGC_IR_LLVM_INTRINSIC(abs)
IGC_IR_LLVM_INTRINSIC(smax)
IGC_IR_LLVM_INTRINSIC(smin)
IGC_IR_LLVM_INTRINSIC(umax)
IGC_IR_LLVM_INTRINSIC(umin)
IGC_IR_LLVM_INTRINSIC(experimental_noalias_scope_decl)

// LLVM instructions allowed in IGC IR
//------------------------------------

#include "Compiler/CISACodeGen/opCode.h"

// Additional instructions not included in opCode.h
IGC_IR_LLVM_INSTRUCTION(Switch)
IGC_IR_LLVM_INSTRUCTION(ShuffleVector)
IGC_IR_LLVM_INSTRUCTION(ExtractValue)
IGC_IR_LLVM_INSTRUCTION(InsertValue)
IGC_IR_LLVM_INSTRUCTION(Unreachable)
IGC_IR_LLVM_INSTRUCTION(AddrSpaceCast)
IGC_IR_LLVM_INSTRUCTION(FNeg)
IGC_IR_LLVM_INSTRUCTION(Freeze)

// Specific verification for LLVM Instructions
//--------------------------------------------
SPECIFIC_INSTRUCTION_VERIFIER(Call, verifyInstCall)
SPECIFIC_INSTRUCTION_VERIFIER(InsertElement, verifyVectorInst)
SPECIFIC_INSTRUCTION_VERIFIER(ExtractElement, verifyVectorInst)
