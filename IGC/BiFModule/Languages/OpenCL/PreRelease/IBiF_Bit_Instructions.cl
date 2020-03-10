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

//===-  IBiF_Bit_Instructions.cl -===================================================//
//
// This file contains definitions of functions which allow to use
// SPIRV bit instructions from OpenCL.
//
//===----------------------------------------------------------------------===//
#include "spirv.h"

#if defined(cl_intel_bit_instructions)

#define GEN_DEFINITION_BFI( FUNC, OP, TYPE1, TYPE2, ABBR_TYPE1, ABBR_TYPE2 ) \
INLINE TYPE1 OVERLOADABLE FUNC( TYPE1 base, TYPE1 insert, TYPE2 offset, TYPE2 count ) { \
    return __builtin_spirv_##OP##_##ABBR_TYPE1##_##ABBR_TYPE1##_##ABBR_TYPE2##_##ABBR_TYPE2(base, insert, offset, count); \
}

#define GEN_VECTOR_DEFINITION_BFI( FUNC, OP, TYPE1, TYPE2, ABBR_TYPE1, ABBR_TYPE2, VEC_SIZE) \
INLINE TYPE1##VEC_SIZE OVERLOADABLE FUNC( TYPE1##VEC_SIZE base, TYPE1##VEC_SIZE insert, TYPE2##VEC_SIZE offset, TYPE2##VEC_SIZE count ) { \
    return __builtin_spirv_##OP##_v##VEC_SIZE##ABBR_TYPE1##_v##VEC_SIZE##ABBR_TYPE1##_v##VEC_SIZE##ABBR_TYPE2##_v##VEC_SIZE##ABBR_TYPE2 \
        (base, insert, offset, count); \
}

#define GEN_DEFINITIONS_BFI( FUNC, OP ) \
    GEN_DEFINITION_BFI( FUNC, OP, uchar, uint, i8, i32 ) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, uchar, uint, i8, i32, 2) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, uchar, uint, i8, i32, 3) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, uchar, uint, i8, i32, 4) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, uchar, uint, i8, i32, 8) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, uchar, uint, i8, i32, 16) \
    GEN_DEFINITION_BFI( FUNC, OP, ushort, uint, i16, i32 ) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, ushort, uint, i16, i32, 2) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, ushort, uint, i16, i32, 3) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, ushort, uint, i16, i32, 4) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, ushort, uint, i16, i32, 8) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, ushort, uint, i16, i32, 16) \
    GEN_DEFINITION_BFI( FUNC, OP, uint, uint, i32, i32 ) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, uint, uint, i32, i32, 2) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, uint, uint, i32, i32, 3) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, uint, uint, i32, i32, 4) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, uint, uint, i32, i32, 8) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, uint, uint, i32, i32, 16) \
    GEN_DEFINITION_BFI( FUNC, OP, ulong, uint, i64, i32 ) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, ulong, uint, i64, i32, 2) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, ulong, uint, i64, i32, 3) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, ulong, uint, i64, i32, 4) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, ulong, uint, i64, i32, 8) \
    GEN_VECTOR_DEFINITION_BFI( FUNC, OP, ulong, uint, i64, i32, 16)

#define GEN_DEFINITION_BFE( FUNC, OP, TYPE1, TYPE2, ABBR_TYPE1, ABBR_TYPE2 ) \
INLINE TYPE1 OVERLOADABLE FUNC( TYPE1 base, TYPE2 offset, TYPE2 count ) { \
    return __builtin_spirv_##OP##_##ABBR_TYPE1##_##ABBR_TYPE2##_##ABBR_TYPE2(base, offset, count); \
}

#define GEN_VECTOR_DEFINITION_BFE( FUNC, OP, TYPE1, TYPE2, ABBR_TYPE1, ABBR_TYPE2, VEC_SIZE) \
INLINE TYPE1##VEC_SIZE OVERLOADABLE FUNC( TYPE1##VEC_SIZE base, TYPE2##VEC_SIZE offset, TYPE2##VEC_SIZE count ) { \
    return __builtin_spirv_##OP##_v##VEC_SIZE##ABBR_TYPE1##_v##VEC_SIZE##ABBR_TYPE2##_v##VEC_SIZE##ABBR_TYPE2 \
        (base, offset, count); \
}

#define GEN_DEFINITIONS_UBFE( FUNC, OP ) \
    GEN_DEFINITION_BFE( FUNC, OP, uchar, uint, i8, i32 ) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, uchar, uint, i8, i32, 2) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, uchar, uint, i8, i32, 3) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, uchar, uint, i8, i32, 4) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, uchar, uint, i8, i32, 8) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, uchar, uint, i8, i32, 16) \
    GEN_DEFINITION_BFE( FUNC, OP, ushort, uint, i16, i32 ) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, ushort, uint, i16, i32, 2) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, ushort, uint, i16, i32, 3) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, ushort, uint, i16, i32, 4) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, ushort, uint, i16, i32, 8) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, ushort, uint, i16, i32, 16) \
    GEN_DEFINITION_BFE( FUNC, OP, uint, uint, i32, i32 ) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, uint, uint, i32, i32, 2) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, uint, uint, i32, i32, 3) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, uint, uint, i32, i32, 4) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, uint, uint, i32, i32, 8) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, uint, uint, i32, i32, 16) \
    GEN_DEFINITION_BFE( FUNC, OP, ulong, uint, i64, i32 ) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, ulong, uint, i64, i32, 2) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, ulong, uint, i64, i32, 3) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, ulong, uint, i64, i32, 4) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, ulong, uint, i64, i32, 8) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, ulong, uint, i64, i32, 16)

#define GEN_DEFINITIONS_SBFE( FUNC, OP ) \
    GEN_DEFINITION_BFE( FUNC, OP, char, uint, i8, i32 ) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, char, uint, i8, i32, 2) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, char, uint, i8, i32, 3) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, char, uint, i8, i32, 4) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, char, uint, i8, i32, 8) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, char, uint, i8, i32, 16) \
    GEN_DEFINITION_BFE( FUNC, OP, short, uint, i16, i32 ) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, short, uint, i16, i32, 2) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, short, uint, i16, i32, 3) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, short, uint, i16, i32, 4) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, short, uint, i16, i32, 8) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, short, uint, i16, i32, 16) \
    GEN_DEFINITION_BFE( FUNC, OP, int, uint, i32, i32 ) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, int, uint, i32, i32, 2) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, int, uint, i32, i32, 3) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, int, uint, i32, i32, 4) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, int, uint, i32, i32, 8) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, int, uint, i32, i32, 16) \
    GEN_DEFINITION_BFE( FUNC, OP, long, uint, i64, i32 ) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, long, uint, i64, i32, 2) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, long, uint, i64, i32, 3) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, long, uint, i64, i32, 4) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, long, uint, i64, i32, 8) \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, long, uint, i64, i32, 16)

#define GEN_DEFINITION_BFREV( FUNC, OP, TYPE, ABBR_TYPE ) \
INLINE TYPE OVERLOADABLE FUNC( TYPE base ) { \
    return __builtin_spirv_##OP##_##ABBR_TYPE(base); \
}

#define GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, TYPE, ABBR_TYPE, VEC_SIZE ) \
INLINE TYPE##VEC_SIZE OVERLOADABLE FUNC( TYPE##VEC_SIZE base ) { \
    return __builtin_spirv_##OP##_v##VEC_SIZE##ABBR_TYPE(base); \
}

#define GEN_DEFINITIONS_BFREV( FUNC, OP ) \
    GEN_DEFINITION_BFREV( FUNC, OP, uchar, i8) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, uchar, i8, 2) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, uchar, i8, 3) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, uchar, i8, 4) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, uchar, i8, 8) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, uchar, i8, 16) \
    GEN_DEFINITION_BFREV( FUNC, OP, ushort, i16) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, ushort, i16, 2) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, ushort, i16, 3) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, ushort, i16, 4) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, ushort, i16, 8) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, ushort, i16, 16 ) \
    GEN_DEFINITION_BFREV( FUNC, OP, uint, i32) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, uint, i32, 2) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, uint, i32, 3) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, uint, i32, 4) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, uint, i32, 8) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, uint, i32, 16) \
    GEN_DEFINITION_BFREV( FUNC, OP, ulong, i64) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, ulong, i64, 2) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, ulong, i64, 3) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, ulong, i64, 4) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, ulong, i64, 8) \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, ulong, i64, 16) \

GEN_DEFINITIONS_BFI( intel_bfi, OpBitFieldInsert )
GEN_DEFINITIONS_SBFE( intel_sbfe, OpBitFieldSExtract )
GEN_DEFINITIONS_UBFE( intel_ubfe, OpBitFieldUExtract )
GEN_DEFINITIONS_BFREV( intel_bfrev, OpBitReverse )

#endif // cl_intel_bit_instructions
