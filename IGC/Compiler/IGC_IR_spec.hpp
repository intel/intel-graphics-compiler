/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

//
// This file contains description of LLVM types, instructions and intrinsic functions
// allowed by IGC IR. This specification header is included in VerificationPass.hpp
// and VerificationPass.cpp.
//

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

// LLVM intrinsics supported by IGC IR
//------------------------------------
IGC_IR_LLVM_INTRINSIC(sqrt)
IGC_IR_LLVM_INTRINSIC(fabs)
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
#if LLVM_VERSION_MAJOR >= 8
IGC_IR_LLVM_INTRINSIC(fshl)
IGC_IR_LLVM_INTRINSIC(fshr)
#endif
#if LLVM_VERSION_MAJOR >= 9
IGC_IR_LLVM_INTRINSIC(usub_sat)
IGC_IR_LLVM_INTRINSIC(ssub_sat)
#endif
#if LLVM_VERSION_MAJOR >= 10
IGC_IR_LLVM_INTRINSIC(uadd_sat)
IGC_IR_LLVM_INTRINSIC(sadd_sat)
#endif

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
#if LLVM_VERSION_MAJOR >= 10
IGC_IR_LLVM_INSTRUCTION(FNeg)
#endif

// Specific verification for LLVM Instructions
//--------------------------------------------
SPECIFIC_INSTRUCTION_VERIFIER(Call, verifyInstCall)
SPECIFIC_INSTRUCTION_VERIFIER(InsertElement, verifyVectorInst)
SPECIFIC_INSTRUCTION_VERIFIER(ExtractElement, verifyVectorInst)
