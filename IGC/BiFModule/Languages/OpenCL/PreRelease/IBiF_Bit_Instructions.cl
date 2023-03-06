/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===-  IBiF_Bit_Instructions.cl -===================================================//
//
// This file contains definitions of functions which allow to use
// SPIRV bit instructions from OpenCL.
//
//===----------------------------------------------------------------------===//
#include "spirv.h"

#if defined(cl_khr_extended_bit_ops)

// ---- BFI ----

#define GEN_DEFINITION_BFI(FUNC, OP, TYPE, ABBR_TYPE, SPIRV_TYPE)                                                      \
  INLINE TYPE OVERLOADABLE FUNC(TYPE base, TYPE insert, uint offset, uint count) {                                     \
    return as_##TYPE(SPIRV_BUILTIN(OP, _##ABBR_TYPE##_##ABBR_TYPE##_i32_i32, )(                                        \
        as_##SPIRV_TYPE(base), as_##SPIRV_TYPE(insert), offset, count));                                               \
  }

#define GEN_VECTOR_DEFINITION_BFI(FUNC, OP, TYPE, ABBR_TYPE, SPIRV_TYPE, VEC_SIZE)                                     \
  INLINE TYPE##VEC_SIZE OVERLOADABLE FUNC(TYPE##VEC_SIZE base, TYPE##VEC_SIZE insert, uint offset, uint count) {       \
    return as_##TYPE##VEC_SIZE(                                                                                        \
        SPIRV_BUILTIN(OP, _v##VEC_SIZE##ABBR_TYPE##_v##VEC_SIZE##ABBR_TYPE##_i32_i32, )(     \
            as_##SPIRV_TYPE##VEC_SIZE(base), as_##SPIRV_TYPE##VEC_SIZE(insert), offset, count));                       \
  }

#define GEN_DEFINITIONS_BFI_ALL_WIDTHS(FUNC, OP, TYPE, ABBR_TYPE, SPIRV_TYPE)                                          \
  GEN_DEFINITION_BFI(FUNC, OP, TYPE, ABBR_TYPE, SPIRV_TYPE)                                                            \
  GEN_VECTOR_DEFINITION_BFI(FUNC, OP, TYPE, ABBR_TYPE, SPIRV_TYPE, 2)                                                  \
  GEN_VECTOR_DEFINITION_BFI(FUNC, OP, TYPE, ABBR_TYPE, SPIRV_TYPE, 3)                                                  \
  GEN_VECTOR_DEFINITION_BFI(FUNC, OP, TYPE, ABBR_TYPE, SPIRV_TYPE, 4)                                                  \
  GEN_VECTOR_DEFINITION_BFI(FUNC, OP, TYPE, ABBR_TYPE, SPIRV_TYPE, 8)                                                  \
  GEN_VECTOR_DEFINITION_BFI(FUNC, OP, TYPE, ABBR_TYPE, SPIRV_TYPE, 16)

#define GEN_DEFINITIONS_BFI(FUNC, OP)                                                                                  \
  GEN_DEFINITIONS_BFI_ALL_WIDTHS(FUNC, OP, uchar,  i8,  char)                                                          \
  GEN_DEFINITIONS_BFI_ALL_WIDTHS(FUNC, OP, ushort, i16, short)                                                         \
  GEN_DEFINITIONS_BFI_ALL_WIDTHS(FUNC, OP, uint,   i32, int)                                                           \
  GEN_DEFINITIONS_BFI_ALL_WIDTHS(FUNC, OP, ulong,  i64, long)                                                          \
  GEN_DEFINITIONS_BFI_ALL_WIDTHS(FUNC, OP, char,   i8,  char)                                                          \
  GEN_DEFINITIONS_BFI_ALL_WIDTHS(FUNC, OP, short,  i16, short)                                                         \
  GEN_DEFINITIONS_BFI_ALL_WIDTHS(FUNC, OP, int,    i32, int)                                                           \
  GEN_DEFINITIONS_BFI_ALL_WIDTHS(FUNC, OP, long,   i64, long)

// ---- UBFE/SBFE ----

#define GEN_DEFINITION_BFE( FUNC, OP, SPV_TYPE, TYPE1, TYPE2, ABBR_TYPE1, ABBR_TYPE2 )                                 \
INLINE SPV_TYPE OVERLOADABLE FUNC( TYPE1 base, TYPE2 offset, TYPE2 count ) {                                           \
    return SPIRV_BUILTIN(OP, _##ABBR_TYPE1##_##ABBR_TYPE2##_##ABBR_TYPE2, )                                            \
    ( as_##SPV_TYPE( base ), offset, count);                                                                           \
}

#define GEN_VECTOR_DEFINITION_BFE( FUNC, OP, SPV_TYPE, TYPE1, TYPE2, ABBR_TYPE1, ABBR_TYPE2, VEC_SIZE)                 \
INLINE SPV_TYPE##VEC_SIZE OVERLOADABLE FUNC( TYPE1##VEC_SIZE base, TYPE2 offset, TYPE2 count ) {                       \
    return SPIRV_BUILTIN(OP, _v##VEC_SIZE##ABBR_TYPE1##_##ABBR_TYPE2##_##ABBR_TYPE2, )                                 \
    ( as_##SPV_TYPE##VEC_SIZE( base ), offset, count);                                                                 \
}

#define GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, TYPE, SPV_TYPE, ABBR_TYPE )                                          \
    GEN_DEFINITION_BFE       ( FUNC, OP, TYPE, SPV_TYPE, uint, ABBR_TYPE, i32 )                                        \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, TYPE, SPV_TYPE, uint, ABBR_TYPE, i32, 2)                                      \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, TYPE, SPV_TYPE, uint, ABBR_TYPE, i32, 3)                                      \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, TYPE, SPV_TYPE, uint, ABBR_TYPE, i32, 4)                                      \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, TYPE, SPV_TYPE, uint, ABBR_TYPE, i32, 8)                                      \
    GEN_VECTOR_DEFINITION_BFE( FUNC, OP, TYPE, SPV_TYPE, uint, ABBR_TYPE, i32, 16)


#define GEN_DEFINITIONS_UBFE( FUNC, OP )                                                                               \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, uchar,  uchar,  i8  )                                                    \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, ushort, ushort, i16 )                                                    \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, uint,   uint,   i32 )                                                    \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, ulong,  ulong,  i64 )                                                    \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, uchar,  char,   i8  )                                                    \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, ushort, short,  i16 )                                                    \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, uint,   int,    i32 )                                                    \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, ulong,  long,   i64 )

#define GEN_DEFINITIONS_SBFE( FUNC, OP )                                                                               \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, char,  uchar,  i8  )                                                     \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, short, ushort, i16 )                                                     \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, int,   uint,   i32 )                                                     \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, long,  ulong,  i64 )                                                     \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, char,  char,   i8  )                                                     \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, short, short,  i16 )                                                     \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, int,   int,    i32 )                                                     \
    GEN_DEFINITIONS_BFE_ALL_WIDTHS( FUNC, OP, long,  long,   i64 )

// ---- BFREV ----

#define GEN_DEFINITION_BFREV( FUNC, OP, TYPE, SPIRV_TYPE, ABBR_TYPE )                                                  \
INLINE TYPE OVERLOADABLE FUNC( TYPE base ) {                                                                           \
    return as_##TYPE( SPIRV_BUILTIN(OP, _##ABBR_TYPE, )( as_##SPIRV_TYPE( base ) ) );                                  \
}

#define GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, TYPE, SPIRV_TYPE, ABBR_TYPE, VEC_SIZE )                                 \
INLINE TYPE##VEC_SIZE OVERLOADABLE FUNC( TYPE##VEC_SIZE base ) {                                                       \
    return as_##TYPE##VEC_SIZE( SPIRV_BUILTIN(OP, _v##VEC_SIZE##ABBR_TYPE, )( as_##SPIRV_TYPE##VEC_SIZE ( base ) ) );  \
}

#define GEN_DEFINITIONS_BFREV_ALL_WIDTHS( FUNC, OP, TYPE, SPIRV_TYPE, ABBR_TYPE )  \
    GEN_DEFINITION_BFREV       ( FUNC, OP, TYPE, SPIRV_TYPE, ABBR_TYPE )           \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, TYPE, SPIRV_TYPE, ABBR_TYPE, 2)         \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, TYPE, SPIRV_TYPE, ABBR_TYPE, 3)         \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, TYPE, SPIRV_TYPE, ABBR_TYPE, 4)         \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, TYPE, SPIRV_TYPE, ABBR_TYPE, 8)         \
    GEN_VECTOR_DEFINITION_BFREV( FUNC, OP, TYPE, SPIRV_TYPE, ABBR_TYPE, 16)

#define GEN_DEFINITIONS_BFREV( FUNC, OP )                               \
    GEN_DEFINITIONS_BFREV_ALL_WIDTHS( FUNC, OP, uchar,  char,  i8  )    \
    GEN_DEFINITIONS_BFREV_ALL_WIDTHS( FUNC, OP, ushort, short, i16 )    \
    GEN_DEFINITIONS_BFREV_ALL_WIDTHS( FUNC, OP, uint,   int,   i32 )    \
    GEN_DEFINITIONS_BFREV_ALL_WIDTHS( FUNC, OP, ulong,  long,  i64 )    \
    GEN_DEFINITIONS_BFREV_ALL_WIDTHS( FUNC, OP, char,   char,  i8  )    \
    GEN_DEFINITIONS_BFREV_ALL_WIDTHS( FUNC, OP, short,  short, i16 )    \
    GEN_DEFINITIONS_BFREV_ALL_WIDTHS( FUNC, OP, int,    int,   i32 )    \
    GEN_DEFINITIONS_BFREV_ALL_WIDTHS( FUNC, OP, long,   long,  i64 )

// Generate all

GEN_DEFINITIONS_BFI( bitfield_insert, BitFieldInsert )
GEN_DEFINITIONS_SBFE( bitfield_extract_signed, BitFieldSExtract )
GEN_DEFINITIONS_UBFE( bitfield_extract_unsigned, BitFieldUExtract )
GEN_DEFINITIONS_BFREV( bit_reverse, BitReverse )

GEN_DEFINITIONS_BFI( intel_bfi, BitFieldInsert )
GEN_DEFINITIONS_SBFE( intel_sbfe, BitFieldSExtract )
GEN_DEFINITIONS_UBFE( intel_ubfe, BitFieldUExtract )
GEN_DEFINITIONS_BFREV( intel_bfrev, BitReverse )

#endif // cl_khr_extended_bit_ops

