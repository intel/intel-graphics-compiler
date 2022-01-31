/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( __func, __rettype, __argtype1, __argtype2, __abbrargtype1, __abbrargtype2 ) \
    __rettype##2 __func##_v2##__abbrargtype1##_v2##__abbrargtype2##_v2##__abbrargtype2( __argtype1##2 a, __argtype2##2 b, __argtype2##2 c) { \
        return ( __rettype##2 )( __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s0, b.s0, c.s0), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s1, b.s1, c.s1) ); \
    } \
    __rettype##3 __func##_v3##__abbrargtype1##_v3##__abbrargtype2##_v3##__abbrargtype2( __argtype1##3 a, __argtype2##3 b, __argtype2##3 c) { \
        return ( __rettype##3 )( __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s0, b.s0, c.s0), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s1, b.s1, c.s1), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s2, b.s2, c.s2) ); \
    } \
    __rettype##4 __func##_v4##__abbrargtype1##_v4##__abbrargtype2##_v4##__abbrargtype2( __argtype1##4 a, __argtype2##4 b, __argtype2##4 c) { \
        return ( __rettype##4 )( __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s0, b.s0, c.s0), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s1, b.s1, c.s1), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s2, b.s2, c.s2), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s3, b.s3, c.s3) ); \
    } \
    __rettype##8 __func##_v8##__abbrargtype1##_v8##__abbrargtype2##_v8##__abbrargtype2( __argtype1##8 a, __argtype2##8 b, __argtype2##8 c) { \
        return ( __rettype##8 )( __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s0, b.s0, c.s0), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s1, b.s1, c.s1), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s2, b.s2, c.s2), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s3, b.s3, c.s3), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s4, b.s4, c.s4), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s5, b.s5, c.s5), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s6, b.s6, c.s6), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s7, b.s7, c.s7) ); \
    } \
    __rettype##16 __func##_v16##__abbrargtype1##_v16##__abbrargtype2##_v16##__abbrargtype2( __argtype1##16 a, __argtype2##16 b, __argtype2##16 c) { \
        return ( __rettype##16 )( __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s0, b.s0, c.s0), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s1, b.s1, c.s1), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s2, b.s2, c.s2), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s3, b.s3, c.s3), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s4, b.s4, c.s4), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s5, b.s5, c.s5), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s6, b.s6, c.s6), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s7, b.s7, c.s7), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s8, b.s8, c.s8), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.s9, b.s9, c.s9), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.sa, b.sa, c.sa), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.sb, b.sb, c.sb), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.sc, b.sc, c.sc), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.sd, b.sd, c.sd), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.se, b.se, c.se), \
                                 __func##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2(a.sf, b.sf, c.sf) ); \
    }

#define GENERATE_VECTOR_FUNCTIONS_4ARGS_2TYPES_T1_T1_T2_T2( __func, __rettype, __argtype1, __argtype2, __abbrargtype1, __abbrargtype2 ) \
    __rettype##2  SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v2##__abbrargtype1##_v2##__abbrargtype1##_v2##__abbrargtype2##_v2##__abbrargtype2, ) \
    ( __argtype1##2 a, __argtype1##2 b, u##__argtype2##2 c, u##__argtype2##2 d) { \
        return ( __rettype##2 )( SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s0, b.s0, c.s0, d.s0), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s1, b.s1, c.s1, d.s1) ); \
    } \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v3##__abbrargtype1##_v3##__abbrargtype1##_v3##__abbrargtype2##_v3##__abbrargtype2, ) \
    ( __argtype1##3 a, __argtype1##3 b, u##__argtype2##3 c, u##__argtype2##3 d) { \
        return ( __rettype##3 )( SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s0, b.s0, c.s0, d.s0), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s1, b.s1, c.s1, d.s1), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s2, b.s2, c.s2, d.s2) ); \
    } \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v4##__abbrargtype1##_v4##__abbrargtype1##_v4##__abbrargtype2##_v4##__abbrargtype2, ) \
    ( __argtype1##4 a, __argtype1##4 b, u##__argtype2##4 c, u##__argtype2##4 d) { \
        return ( __rettype##4 )( SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s0, b.s0, c.s0, d.s0), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s1, b.s1, c.s1, d.s1), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s2, b.s2, c.s2, d.s2), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s3, b.s3, c.s3, d.s3) ); \
    } \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v8##__abbrargtype1##_v8##__abbrargtype1##_v8##__abbrargtype2##_v8##__abbrargtype2, ) \
    ( __argtype1##8 a, __argtype1##8 b, u##__argtype2##8 c, u##__argtype2##8 d) { \
        return ( __rettype##8 )( SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s0, b.s0, c.s0, d.s0), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s1, b.s1, c.s1, d.s1), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s2, b.s2, c.s2, d.s2), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s3, b.s3, c.s3, d.s3), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s4, b.s4, c.s4, d.s4), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s5, b.s5, c.s5, d.s5), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s6, b.s6, c.s6, d.s6), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s7, b.s7, c.s7, d.s7) ); \
    } \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v16##__abbrargtype1##_v16##__abbrargtype1##_v16##__abbrargtype2##_v16##__abbrargtype2, ) \
    ( __argtype1##16 a, __argtype1##16 b, u##__argtype2##16 c, u##__argtype2##16 d) { \
        return ( __rettype##16 )( SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s0, b.s0, c.s0, d.s0), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s1, b.s1, c.s1, d.s1), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s2, b.s2, c.s2, d.s2), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s3, b.s3, c.s3, d.s3), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s4, b.s4, c.s4, d.s4), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s5, b.s5, c.s5, d.s5), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s6, b.s6, c.s6, d.s6), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s7, b.s7, c.s7, d.s7), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s8, b.s8, c.s8, d.s8), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.s9, b.s9, c.s9, d.s9), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.sa, b.sa, c.sa, d.sa), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.sb, b.sb, c.sb, d.sb), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.sc, b.sc, c.sc, d.sc), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.sd, b.sd, c.sd, d.sd), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.se, b.se, c.se, d.se), \
                                 SPIRV_BUILTIN(__func, _##__abbrargtype1##_##__abbrargtype1##_##__abbrargtype2##_##__abbrargtype2, )(a.sf, b.sf, c.sf, d.sf) ); \
    }

// Bit Instructions

char SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _i8_i8_i32_i32, )(char Base, char Insert, uint Offset, uint Count)
{
    return __builtin_IB_bfi(Count, Offset, (uint)Insert, (uint)Base);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _i16_i16_i32_i32, )(short Base, short Insert, uint Offset, uint Count)
{
    return __builtin_IB_bfi(Count, Offset, (uint)Insert, (uint)Base);
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _i32_i32_i32_i32, )(int Base, int Insert, uint Offset, uint Count)
{
    // edge case: hardware reads width (count) from 5 bits, so it's unable to achieve 32
    const uint Result = __builtin_IB_bfi(Count, Offset, Insert, Base);
    return (Count == 32 && Offset == 0) ? Insert : Result;
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _i64_i64_i32_i32, )(long Base, long Insert, uint Offset, uint Count)
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

char __builtin_spirv_OpBitFieldSExtract_i8_i32_i32(char Base, uint Offset, uint Count)
{
    return __builtin_IB_ibfe(Count, Offset, (uint)Base);
}

short __builtin_spirv_OpBitFieldSExtract_i16_i32_i32(short Base, uint Offset, uint Count)
{
    return __builtin_IB_ibfe(Count, Offset, (uint)Base);
}

int __builtin_spirv_OpBitFieldSExtract_i32_i32_i32(int Base, uint Offset, uint Count)
{
    // edge case: hardware reads width (count) from 5 bits, so it's unable to achieve 32
    const int Result = __builtin_IB_ibfe(Count, Offset, Base);
    return (Count == 32 && Offset == 0) ? Base : Result;
}

long __builtin_spirv_OpBitFieldSExtract_i64_i32_i32(long Base, uint Offset, uint Count)
{
    const int Size = 64;
    const int Rest = Size - Offset - Count;
    const long Result = ((Base << Rest) >> (Size - Count));
    return Count == 0 ? 0 : Result;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( __builtin_spirv_OpBitFieldSExtract, char,  char,  uint, i8,  i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( __builtin_spirv_OpBitFieldSExtract, short, short, uint, i16, i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( __builtin_spirv_OpBitFieldSExtract, int,   int,   uint, i32, i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( __builtin_spirv_OpBitFieldSExtract, long,  long,  uint, i64, i32 )

uchar __builtin_spirv_OpBitFieldUExtract_i8_i32_i32(uchar Base, uint Offset, uint Count)
{
    return __builtin_IB_ubfe(Count, Offset, (uint)Base);
}

ushort __builtin_spirv_OpBitFieldUExtract_i16_i32_i32(ushort Base, uint Offset, uint Count)
{
    return __builtin_IB_ubfe(Count, Offset, (uint)Base);
}

uint __builtin_spirv_OpBitFieldUExtract_i32_i32_i32(uint Base, uint Offset, uint Count)
{
    // edge case: hardware reads width (count) from 5 bits, so it's unable to achieve 32
    const uint Result = __builtin_IB_ubfe(Count, Offset, Base);
    return (Count ==  32 && Offset == 0) ? Base : Result;
}

ulong __builtin_spirv_OpBitFieldUExtract_i64_i32_i32(ulong Base, uint Offset, uint Count)
{
    const int Size = 64;
    const int Rest = Size - Offset - Count;
    const ulong Result = ((Base << Rest) >> (Size - Count));
    return Count == 0 ? 0 : Result;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( __builtin_spirv_OpBitFieldUExtract, uchar,  uchar,  uint, i8,  i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( __builtin_spirv_OpBitFieldUExtract, ushort, ushort, uint, i16, i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( __builtin_spirv_OpBitFieldUExtract, uint,   uint,   uint, i32, i32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS_2TYPES_T1_T2_T2( __builtin_spirv_OpBitFieldUExtract, ulong,  ulong,  uint, i64, i32 )

uchar __builtin_spirv_OpBitReverse_i8(uchar Base)
{
    return  __builtin_IB_bfrev( (uint)Base) >> 24;
}

ushort __builtin_spirv_OpBitReverse_i16(ushort Base)
{
    return  __builtin_IB_bfrev( (uint)Base) >> 16;
}

uint __builtin_spirv_OpBitReverse_i32(uint Base)
{
    return __builtin_IB_bfrev(Base);
}

ulong __builtin_spirv_OpBitReverse_i64(ulong Base)
{
    return ((ulong)__builtin_IB_bfrev((uint)Base) << 32) | (ulong)__builtin_IB_bfrev((Base >> 32));
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpBitReverse, uchar, uchar, i8 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpBitReverse, ushort, ushort, i16 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpBitReverse, uint, uint, i32 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpBitReverse, ulong, ulong, i64 )


uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitCount, _i8, )(char Base)
{
    return __builtin_IB_popcount_1u8(as_uchar(Base));
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitCount, _i16, )(short Base)
{
    return __builtin_IB_popcount_1u16(as_ushort(Base));
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitCount, _i32, )(int Base)
{
    return __builtin_IB_popcount_1u32(as_uint(Base));
}

ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitCount, _i64, )(long Base)
{
    return SPIRV_BUILTIN(BitCount, _i32, )((int)(as_ulong(Base) >> 32)) + SPIRV_BUILTIN(BitCount, _i32, )((int)Base);
}

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG( BitCount, uchar,  char,  i8 )
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG( BitCount, ushort, short, i16 )
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG( BitCount, uint,   int,   i32 )
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG( BitCount, ulong,  long,  i64 )

