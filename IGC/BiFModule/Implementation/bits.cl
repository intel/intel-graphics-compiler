/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( __func, __rettype, __argtype1, __argtype2, __abbrargtype1, __abbrargtype2 ) \
    __rettype##2 __attribute__((overloadable)) __spirv_##__func( __argtype1##2 a, __argtype2 b, __argtype2 c) { \
        return ( __rettype##2 )( __spirv_##__func(a.s0, b, c), \
                                 __spirv_##__func(a.s1, b, c) ); \
    } \
    __rettype##3 __attribute__((overloadable)) __spirv_##__func( __argtype1##3 a, __argtype2 b, __argtype2 c) { \
        return ( __rettype##3 )( __spirv_##__func(a.s0, b, c), \
                                 __spirv_##__func(a.s1, b, c), \
                                 __spirv_##__func(a.s2, b, c) ); \
    } \
    __rettype##4 __attribute__((overloadable)) __spirv_##__func( __argtype1##4 a, __argtype2 b, __argtype2 c) { \
        return ( __rettype##4 )( __spirv_##__func(a.s0, b, c), \
                                 __spirv_##__func(a.s1, b, c), \
                                 __spirv_##__func(a.s2, b, c), \
                                 __spirv_##__func(a.s3, b, c) ); \
    } \
    __rettype##8 __attribute__((overloadable)) __spirv_##__func( __argtype1##8 a, __argtype2 b, __argtype2 c) { \
        return ( __rettype##8 )( __spirv_##__func(a.s0, b, c), \
                                 __spirv_##__func(a.s1, b, c), \
                                 __spirv_##__func(a.s2, b, c), \
                                 __spirv_##__func(a.s3, b, c), \
                                 __spirv_##__func(a.s4, b, c), \
                                 __spirv_##__func(a.s5, b, c), \
                                 __spirv_##__func(a.s6, b, c), \
                                 __spirv_##__func(a.s7, b, c) ); \
    } \
    __rettype##16 __attribute__((overloadable)) __spirv_##__func( __argtype1##16 a, __argtype2 b, __argtype2 c) { \
        return ( __rettype##16 )( __spirv_##__func(a.s0, b, c), \
                                  __spirv_##__func(a.s1, b, c), \
                                  __spirv_##__func(a.s2, b, c), \
                                  __spirv_##__func(a.s3, b, c), \
                                  __spirv_##__func(a.s4, b, c), \
                                  __spirv_##__func(a.s5, b, c), \
                                  __spirv_##__func(a.s6, b, c), \
                                  __spirv_##__func(a.s7, b, c), \
                                  __spirv_##__func(a.s8, b, c), \
                                  __spirv_##__func(a.s9, b, c), \
                                  __spirv_##__func(a.sa, b, c), \
                                  __spirv_##__func(a.sb, b, c), \
                                  __spirv_##__func(a.sc, b, c), \
                                  __spirv_##__func(a.sd, b, c), \
                                  __spirv_##__func(a.se, b, c), \
                                  __spirv_##__func(a.sf, b, c) ); \
    }

#define GENERATE_VECTOR_FUNCTIONS_4ARGS_2TYPES_T1_T1_T2_T2( __func, __rettype, __argtype1, __argtype2, __abbrargtype1, __abbrargtype2 ) \
    __rettype##2  __attribute__((overloadable)) __spirv_##__func \
    ( __argtype1##2 a, __argtype1##2 b, u##__argtype2 c, u##__argtype2 d) { \
        return ( __rettype##2 )( __spirv_##__func(a.s0, b.s0, c, d), \
                                 __spirv_##__func(a.s1, b.s1, c, d) ); \
    } \
    __rettype##3 __attribute__((overloadable)) __spirv_##__func \
    ( __argtype1##3 a, __argtype1##3 b, u##__argtype2 c, u##__argtype2 d) { \
        return ( __rettype##3 )( __spirv_##__func(a.s0, b.s0, c, d), \
                                 __spirv_##__func(a.s1, b.s1, c, d), \
                                 __spirv_##__func(a.s2, b.s2, c, d) ); \
    } \
    __rettype##4 __attribute__((overloadable)) __spirv_##__func \
    ( __argtype1##4 a, __argtype1##4 b, u##__argtype2 c, u##__argtype2 d) { \
        return ( __rettype##4 )( __spirv_##__func(a.s0, b.s0, c, d), \
                                 __spirv_##__func(a.s1, b.s1, c, d), \
                                 __spirv_##__func(a.s2, b.s2, c, d), \
                                 __spirv_##__func(a.s3, b.s3, c, d) ); \
    } \
    __rettype##8 __attribute__((overloadable)) __spirv_##__func \
    ( __argtype1##8 a, __argtype1##8 b, u##__argtype2 c, u##__argtype2 d) { \
        return ( __rettype##8 )( __spirv_##__func(a.s0, b.s0, c, d), \
                                 __spirv_##__func(a.s1, b.s1, c, d), \
                                 __spirv_##__func(a.s2, b.s2, c, d), \
                                 __spirv_##__func(a.s3, b.s3, c, d), \
                                 __spirv_##__func(a.s4, b.s4, c, d), \
                                 __spirv_##__func(a.s5, b.s5, c, d), \
                                 __spirv_##__func(a.s6, b.s6, c, d), \
                                 __spirv_##__func(a.s7, b.s7, c, d) ); \
    } \
    __rettype##16 __attribute__((overloadable)) __spirv_##__func \
    ( __argtype1##16 a, __argtype1##16 b, u##__argtype2 c, u##__argtype2 d) { \
        return ( __rettype##16 )( __spirv_##__func(a.s0, b.s0, c, d), \
                                  __spirv_##__func(a.s1, b.s1, c, d), \
                                  __spirv_##__func(a.s2, b.s2, c, d), \
                                  __spirv_##__func(a.s3, b.s3, c, d), \
                                  __spirv_##__func(a.s4, b.s4, c, d), \
                                  __spirv_##__func(a.s5, b.s5, c, d), \
                                  __spirv_##__func(a.s6, b.s6, c, d), \
                                  __spirv_##__func(a.s7, b.s7, c, d), \
                                  __spirv_##__func(a.s8, b.s8, c, d), \
                                  __spirv_##__func(a.s9, b.s9, c, d), \
                                  __spirv_##__func(a.sa, b.sa, c, d), \
                                  __spirv_##__func(a.sb, b.sb, c, d), \
                                  __spirv_##__func(a.sc, b.sc, c, d), \
                                  __spirv_##__func(a.sd, b.sd, c, d), \
                                  __spirv_##__func(a.se, b.se, c, d), \
                                  __spirv_##__func(a.sf, b.sf, c, d) ); \
    }

// Bit Instructions

char __attribute__((overloadable)) __spirv_BitFieldInsert(char Base, char Insert, uint Offset, uint Count)
{
    return __builtin_IB_bfi(Count, Offset, (uint)Insert, (uint)Base);
}

short __attribute__((overloadable)) __spirv_BitFieldInsert(short Base, short Insert, uint Offset, uint Count)
{
    return __builtin_IB_bfi(Count, Offset, (uint)Insert, (uint)Base);
}

int __attribute__((overloadable)) __spirv_BitFieldInsert(int Base, int Insert, uint Offset, uint Count)
{
    // edge case: hardware reads width (count) from 5 bits, so it's unable to achieve 32
    const uint Result = __builtin_IB_bfi(Count, Offset, Insert, Base);
    return (Count == 32 && Offset == 0) ? Insert : Result;
}

long __attribute__((overloadable)) __spirv_BitFieldInsert(long Base, long Insert, uint Offset, uint Count)
{
    const int Size = 64;
    const uint OffsetPlusCount = Offset + Count;
    const uint SizeMinusOffset = Size - Offset;
    const uint SizeMinusCount = Size - Count;
    const uint Rest = Size - OffsetPlusCount;
    ulong xoo = (as_ulong(Base) >> OffsetPlusCount) << OffsetPlusCount;
    xoo = OffsetPlusCount == Size ? 0 : xoo;
    ulong oox = (as_ulong(Base) << SizeMinusOffset) >> SizeMinusOffset;
    oox = SizeMinusOffset == Size ? 0 : oox;
    ulong oxo = as_ulong(Insert) << SizeMinusCount;
    oxo = SizeMinusCount == Size ? 0 : oxo;
    oxo >>= Rest;
    oxo = Rest == Size ? 0 : oxo;
    const ulong Result = ( xoo | oxo | oox );
    return Count == 0 ? Base : Result;
}

GENERATE_VECTOR_FUNCTIONS_4ARGS_2TYPES_T1_T1_T2_T2( BitFieldInsert, char,  char,  int, i8,  i32 )
GENERATE_VECTOR_FUNCTIONS_4ARGS_2TYPES_T1_T1_T2_T2( BitFieldInsert, short, short, int, i16, i32 )
GENERATE_VECTOR_FUNCTIONS_4ARGS_2TYPES_T1_T1_T2_T2( BitFieldInsert, int,   int,   int, i32, i32 )
GENERATE_VECTOR_FUNCTIONS_4ARGS_2TYPES_T1_T1_T2_T2( BitFieldInsert, long,  long,  int, i64, i32 )

char __attribute__((overloadable)) __spirv_BitFieldSExtract(char Base, uint Offset, uint Count)
{
    return __builtin_IB_ibfe(Count, Offset, (uint)Base);
}

short __attribute__((overloadable)) __spirv_BitFieldSExtract(short Base, uint Offset, uint Count)
{
    return __builtin_IB_ibfe(Count, Offset, (uint)Base);
}

int __attribute__((overloadable)) __spirv_BitFieldSExtract(int Base, uint Offset, uint Count)
{
    // edge case: hardware reads width (count) from 5 bits, so it's unable to achieve 32
    const int Result = __builtin_IB_ibfe(Count, Offset, Base);
    return (Count == 32 && Offset == 0) ? Base : Result;
}

long __attribute__((overloadable)) __spirv_BitFieldSExtract(long Base, uint Offset, uint Count)
{
    const int Size = 64;
    const int Rest = Size - Offset - Count;
    const long Result = ((Base << Rest) >> (Size - Count));
    return Count == 0 ? 0 : Result;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( BitFieldSExtract, char,  char,  uint, i8,  i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( BitFieldSExtract, short, short, uint, i16, i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( BitFieldSExtract, int,   int,   uint, i32, i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( BitFieldSExtract, long,  long,  uint, i64, i32 )

uchar __attribute__((overloadable)) __spirv_BitFieldUExtract(uchar Base, uint Offset, uint Count)
{
    return __builtin_IB_ubfe(Count, Offset, (uint)Base);
}

ushort __attribute__((overloadable)) __spirv_BitFieldUExtract(ushort Base, uint Offset, uint Count)
{
    return __builtin_IB_ubfe(Count, Offset, (uint)Base);
}

uint __attribute__((overloadable)) __spirv_BitFieldUExtract(uint Base, uint Offset, uint Count)
{
    // edge case: hardware reads width (count) from 5 bits, so it's unable to achieve 32
    const uint Result = __builtin_IB_ubfe(Count, Offset, Base);
    return (Count ==  32 && Offset == 0) ? Base : Result;
}

ulong __attribute__((overloadable)) __spirv_BitFieldUExtract(ulong Base, uint Offset, uint Count)
{
    const int Size = 64;
    const int Rest = Size - Offset - Count;
    const ulong Result = ((Base << Rest) >> (Size - Count));
    return Count == 0 ? 0 : Result;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( BitFieldUExtract, uchar,  uchar,  uint, i8,  i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( BitFieldUExtract, ushort, ushort, uint, i16, i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( BitFieldUExtract, uint,   uint,   uint, i32, i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( BitFieldUExtract, ulong,  ulong,  uint, i64, i32 )

char __attribute__((overloadable)) __spirv_BitReverse(char Base)
{
    return  __builtin_IB_bfrev( (uint)Base) >> 24;
}

short __attribute__((overloadable)) __spirv_BitReverse(short Base)
{
    return  __builtin_IB_bfrev( (uint)Base) >> 16;
}

int __attribute__((overloadable)) __spirv_BitReverse(int Base)
{
    return __builtin_IB_bfrev(Base);
}

long __attribute__((overloadable)) __spirv_BitReverse(long Base)
{
    return ((ulong)__builtin_IB_bfrev((uint)Base) << 32) | (ulong)__builtin_IB_bfrev((Base >> 32));
}

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG( BitReverse, char,  char,  i8 )
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG( BitReverse, short, short, i16 )
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG( BitReverse, int,   int,   i32 )
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG( BitReverse, long,  long,  i64 )


uchar __attribute__((overloadable)) __spirv_BitCount(char Base)
{
    return __builtin_IB_popcount_1u8(as_uchar(Base));
}

ushort __attribute__((overloadable)) __spirv_BitCount(short Base)
{
    return __builtin_IB_popcount_1u16(as_ushort(Base));
}

uint __attribute__((overloadable)) __spirv_BitCount(int Base)
{
    return __builtin_IB_popcount_1u32(as_uint(Base));
}

ulong __attribute__((overloadable)) __spirv_BitCount(long Base)
{
    return __spirv_BitCount((int)(as_ulong(Base) >> 32)) + __spirv_BitCount((int)Base);
}

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG( BitCount, uchar,  char,  i8 )
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG( BitCount, ushort, short, i16 )
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG( BitCount, uint,   int,   i32 )
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG( BitCount, ulong,  long,  i64 )


