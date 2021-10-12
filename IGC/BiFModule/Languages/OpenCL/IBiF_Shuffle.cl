/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

INLINE char2 OVERLOADABLE shuffle(char2 v, uchar2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i8_v2i8, )( v, as_char2( m ) );
}

INLINE char2 OVERLOADABLE shuffle(char4 v, uchar2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i8_v2i8, )( v, as_char2( m ) );
}

INLINE char2 OVERLOADABLE shuffle(char8 v, uchar2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i8_v2i8, )( v, as_char2( m ) );
}

INLINE char2 OVERLOADABLE shuffle(char16 v, uchar2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i8_v2i8, )( v, as_char2( m ) );
}

INLINE char4 OVERLOADABLE shuffle(char2 v, uchar4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i8_v4i8, )( v, as_char4( m ) );
}

INLINE char4 OVERLOADABLE shuffle(char4 v, uchar4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i8_v4i8, )( v, as_char4( m ) );
}

INLINE char4 OVERLOADABLE shuffle(char8 v, uchar4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i8_v4i8, )( v, as_char4( m ) );
}

INLINE char4 OVERLOADABLE shuffle(char16 v, uchar4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i8_v4i8, )( v, as_char4( m ) );
}

INLINE char8 OVERLOADABLE shuffle(char2 v, uchar8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i8_v8i8, )( v, as_char8( m ) );
}

INLINE char8 OVERLOADABLE shuffle(char4 v, uchar8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i8_v8i8, )( v, as_char8( m ) );
}

INLINE char8 OVERLOADABLE shuffle(char8 v, uchar8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i8_v8i8, )( v, as_char8( m ) );
}

INLINE char8 OVERLOADABLE shuffle(char16 v, uchar8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i8_v8i8, )( v, as_char8( m ) );
}

INLINE char16 OVERLOADABLE shuffle(char2 v, uchar16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i8_v16i8, )( v, as_char16( m ) );
}

INLINE char16 OVERLOADABLE shuffle(char4 v, uchar16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i8_v16i8, )( v, as_char16( m ) );
}

INLINE char16 OVERLOADABLE shuffle(char8 v, uchar16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i8_v16i8, )( v, as_char16( m ) );
}

INLINE char16 OVERLOADABLE shuffle(char16 v, uchar16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i8_v16i8, )( v, as_char16( m ) );
}

INLINE uchar2 OVERLOADABLE shuffle(uchar2 v, uchar2 m) {
    return as_uchar2( SPIRV_OCL_BUILTIN(shuffle, _v2i8_v2i8, )( as_char2( v ), as_char2( m ) ) );
}

INLINE uchar2 OVERLOADABLE shuffle(uchar4 v, uchar2 m) {
    return as_uchar2( SPIRV_OCL_BUILTIN(shuffle, _v4i8_v2i8, )( as_char4( v ), as_char2( m ) ) );
}

INLINE uchar2 OVERLOADABLE shuffle(uchar8 v, uchar2 m) {
    return as_uchar2( SPIRV_OCL_BUILTIN(shuffle, _v8i8_v2i8, )( as_char8( v ), as_char2( m ) ) );
}

INLINE uchar2 OVERLOADABLE shuffle(uchar16 v, uchar2 m) {
    return as_uchar2( SPIRV_OCL_BUILTIN(shuffle, _v16i8_v2i8, )( as_char16( v ), as_char2( m ) ) );
}

INLINE uchar4 OVERLOADABLE shuffle(uchar2 v, uchar4 m) {
    return as_uchar4( SPIRV_OCL_BUILTIN(shuffle, _v2i8_v4i8, )( as_char2( v ), as_char4( m ) ) );
}

INLINE uchar4 OVERLOADABLE shuffle(uchar4 v, uchar4 m) {
    return as_uchar4( SPIRV_OCL_BUILTIN(shuffle, _v4i8_v4i8, )( as_char4( v ), as_char4( m ) ) );
}

INLINE uchar4 OVERLOADABLE shuffle(uchar8 v, uchar4 m) {
    return as_uchar4( SPIRV_OCL_BUILTIN(shuffle, _v8i8_v4i8, )( as_char8( v ), as_char4( m ) ) );
}

INLINE uchar4 OVERLOADABLE shuffle(uchar16 v, uchar4 m) {
    return as_uchar4( SPIRV_OCL_BUILTIN(shuffle, _v16i8_v4i8, )( as_char16( v ), as_char4( m ) ) );
}

INLINE uchar8 OVERLOADABLE shuffle(uchar2 v, uchar8 m) {
    return as_uchar8( SPIRV_OCL_BUILTIN(shuffle, _v2i8_v8i8, )( as_char2( v ), as_char8( m ) ) );
}

INLINE uchar8 OVERLOADABLE shuffle(uchar4 v, uchar8 m) {
    return as_uchar8( SPIRV_OCL_BUILTIN(shuffle, _v4i8_v8i8, )( as_char4( v ), as_char8( m ) ) );
}

INLINE uchar8 OVERLOADABLE shuffle(uchar8 v, uchar8 m) {
    return as_uchar8( SPIRV_OCL_BUILTIN(shuffle, _v8i8_v8i8, )( as_char8( v ), as_char8( m ) ) );
}

INLINE uchar8 OVERLOADABLE shuffle(uchar16 v, uchar8 m) {
    return as_uchar8( SPIRV_OCL_BUILTIN(shuffle, _v16i8_v8i8, )( as_char16( v ), as_char8( m ) ) );
}

INLINE uchar16 OVERLOADABLE shuffle(uchar2 v, uchar16 m) {
    return as_uchar16( SPIRV_OCL_BUILTIN(shuffle, _v2i8_v16i8, )( as_char2( v ), as_char16( m ) ) );
}

INLINE uchar16 OVERLOADABLE shuffle(uchar4 v, uchar16 m) {
    return as_uchar16( SPIRV_OCL_BUILTIN(shuffle, _v4i8_v16i8, )( as_char4( v ), as_char16( m ) ) );
}

INLINE uchar16 OVERLOADABLE shuffle(uchar8 v, uchar16 m) {
    return as_uchar16( SPIRV_OCL_BUILTIN(shuffle, _v8i8_v16i8, )( as_char8( v ), as_char16( m ) ) );
}

INLINE uchar16 OVERLOADABLE shuffle(uchar16 v, uchar16 m) {
    return as_uchar16( SPIRV_OCL_BUILTIN(shuffle, _v16i8_v16i8, )( as_char16( v ), as_char16( m ) ) );
}

INLINE short2 OVERLOADABLE shuffle(short2 v, ushort2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i16_v2i16, )( v, as_short2( m ) );
}

INLINE short2 OVERLOADABLE shuffle(short4 v, ushort2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i16_v2i16, )( v, as_short2( m ) );
}

INLINE short2 OVERLOADABLE shuffle(short8 v, ushort2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i16_v2i16, )( v, as_short2( m ) );
}

INLINE short2 OVERLOADABLE shuffle(short16 v, ushort2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i16_v2i16, )( v, as_short2( m ) );
}

INLINE short4 OVERLOADABLE shuffle(short2 v, ushort4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i16_v4i16, )( v, as_short4( m ) );
}

INLINE short4 OVERLOADABLE shuffle(short4 v, ushort4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i16_v4i16, )( v, as_short4( m ) );
}

INLINE short4 OVERLOADABLE shuffle(short8 v, ushort4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i16_v4i16, )( v, as_short4( m ) );
}

INLINE short4 OVERLOADABLE shuffle(short16 v, ushort4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i16_v4i16, )( v, as_short4( m ) );
}

INLINE short8 OVERLOADABLE shuffle(short2 v, ushort8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i16_v8i16, )( v, as_short8( m ) );
}

INLINE short8 OVERLOADABLE shuffle(short4 v, ushort8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i16_v8i16, )( v, as_short8( m ) );
}

INLINE short8 OVERLOADABLE shuffle(short8 v, ushort8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i16_v8i16, )( v, as_short8( m ) );
}

INLINE short8 OVERLOADABLE shuffle(short16 v, ushort8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i16_v8i16, )( v, as_short8( m ) );
}

INLINE short16 OVERLOADABLE shuffle(short2 v, ushort16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i16_v16i16, )( v, as_short16( m ) );
}

INLINE short16 OVERLOADABLE shuffle(short4 v, ushort16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i16_v16i16, )( v, as_short16( m ) );
}

INLINE short16 OVERLOADABLE shuffle(short8 v, ushort16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i16_v16i16, )( v, as_short16( m ) );
}

INLINE short16 OVERLOADABLE shuffle(short16 v, ushort16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i16_v16i16, )( v, as_short16( m ) );
}

INLINE ushort2 OVERLOADABLE shuffle(ushort2 v, ushort2 m) {
    return as_ushort2( SPIRV_OCL_BUILTIN(shuffle, _v2i16_v2i16, )( as_short2( v ), as_short2( m ) ) );
}

INLINE ushort2 OVERLOADABLE shuffle(ushort4 v, ushort2 m) {
    return as_ushort2( SPIRV_OCL_BUILTIN(shuffle, _v4i16_v2i16, )( as_short4( v ), as_short2( m ) ) );
}

INLINE ushort2 OVERLOADABLE shuffle(ushort8 v, ushort2 m) {
    return as_ushort2( SPIRV_OCL_BUILTIN(shuffle, _v8i16_v2i16, )( as_short8( v ), as_short2( m ) ) );
}

INLINE ushort2 OVERLOADABLE shuffle(ushort16 v, ushort2 m) {
    return as_ushort2( SPIRV_OCL_BUILTIN(shuffle, _v16i16_v2i16, )( as_short16( v ), as_short2( m ) ) );
}

INLINE ushort4 OVERLOADABLE shuffle(ushort2 v, ushort4 m) {
    return as_ushort4( SPIRV_OCL_BUILTIN(shuffle, _v2i16_v4i16, )( as_short2( v ), as_short4( m ) ) );
}

INLINE ushort4 OVERLOADABLE shuffle(ushort4 v, ushort4 m) {
    return as_ushort4( SPIRV_OCL_BUILTIN(shuffle, _v4i16_v4i16, )( as_short4( v ), as_short4( m ) ) );
}

INLINE ushort4 OVERLOADABLE shuffle(ushort8 v, ushort4 m) {
    return as_ushort4( SPIRV_OCL_BUILTIN(shuffle, _v8i16_v4i16, )( as_short8( v ), as_short4( m ) ) );
}

INLINE ushort4 OVERLOADABLE shuffle(ushort16 v, ushort4 m) {
    return as_ushort4( SPIRV_OCL_BUILTIN(shuffle, _v16i16_v4i16, )( as_short16( v ), as_short4( m ) ) );
}

INLINE ushort8 OVERLOADABLE shuffle(ushort2 v, ushort8 m) {
    return as_ushort8( SPIRV_OCL_BUILTIN(shuffle, _v2i16_v8i16, )( as_short2( v ), as_short8( m ) ) );
}

INLINE ushort8 OVERLOADABLE shuffle(ushort4 v, ushort8 m) {
    return as_ushort8( SPIRV_OCL_BUILTIN(shuffle, _v4i16_v8i16, )( as_short4( v ), as_short8( m ) ) );
}

INLINE ushort8 OVERLOADABLE shuffle(ushort8 v, ushort8 m) {
    return as_ushort8( SPIRV_OCL_BUILTIN(shuffle, _v8i16_v8i16, )( as_short8( v ), as_short8( m ) ) );
}

INLINE ushort8 OVERLOADABLE shuffle(ushort16 v, ushort8 m) {
    return as_ushort8( SPIRV_OCL_BUILTIN(shuffle, _v16i16_v8i16, )( as_short16( v ), as_short8( m ) ) );
}

INLINE ushort16 OVERLOADABLE shuffle(ushort2 v, ushort16 m) {
    return as_ushort16( SPIRV_OCL_BUILTIN(shuffle, _v2i16_v16i16, )( as_short2( v ), as_short16( m ) ) );
}

INLINE ushort16 OVERLOADABLE shuffle(ushort4 v, ushort16 m) {
    return as_ushort16( SPIRV_OCL_BUILTIN(shuffle, _v4i16_v16i16, )( as_short4( v ), as_short16( m ) ) );
}

INLINE ushort16 OVERLOADABLE shuffle(ushort8 v, ushort16 m) {
    return as_ushort16( SPIRV_OCL_BUILTIN(shuffle, _v8i16_v16i16, )( as_short8( v ), as_short16( m ) ) );
}

INLINE ushort16 OVERLOADABLE shuffle(ushort16 v, ushort16 m) {
    return as_ushort16( SPIRV_OCL_BUILTIN(shuffle, _v16i16_v16i16, )( as_short16( v ), as_short16( m ) ) );
}

INLINE int2 OVERLOADABLE shuffle(int2 v, uint2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i32_v2i32, )( v, as_int2( m ) );
}

INLINE int2 OVERLOADABLE shuffle(int4 v, uint2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i32_v2i32, )( v, as_int2( m ) );
}

INLINE int2 OVERLOADABLE shuffle(int8 v, uint2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i32_v2i32, )( v, as_int2( m ) );
}

INLINE int2 OVERLOADABLE shuffle(int16 v, uint2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i32_v2i32, )( v, as_int2( m ) );
}

INLINE int4 OVERLOADABLE shuffle(int2 v, uint4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i32_v4i32, )( v, as_int4( m ) );
}

INLINE int4 OVERLOADABLE shuffle(int4 v, uint4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i32_v4i32, )( v, as_int4( m ) );
}

INLINE int4 OVERLOADABLE shuffle(int8 v, uint4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i32_v4i32, )( v, as_int4( m ) );
}

INLINE int4 OVERLOADABLE shuffle(int16 v, uint4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i32_v4i32, )( v, as_int4( m ) );
}

INLINE int8 OVERLOADABLE shuffle(int2 v, uint8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i32_v8i32, )( v, as_int8( m ) );
}

INLINE int8 OVERLOADABLE shuffle(int4 v, uint8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i32_v8i32, )( v, as_int8( m ) );
}

INLINE int8 OVERLOADABLE shuffle(int8 v, uint8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i32_v8i32, )( v, as_int8( m ) );
}

INLINE int8 OVERLOADABLE shuffle(int16 v, uint8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i32_v8i32, )( v, as_int8( m ) );
}

INLINE int16 OVERLOADABLE shuffle(int2 v, uint16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i32_v16i32, )( v, as_int16( m ) );
}

INLINE int16 OVERLOADABLE shuffle(int4 v, uint16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i32_v16i32, )( v, as_int16( m ) );
}

INLINE int16 OVERLOADABLE shuffle(int8 v, uint16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i32_v16i32, )( v, as_int16( m ) );
}

INLINE int16 OVERLOADABLE shuffle(int16 v, uint16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i32_v16i32, )( v, as_int16( m ) );
}

INLINE uint2 OVERLOADABLE shuffle(uint2 v, uint2 m) {
    return as_uint2( SPIRV_OCL_BUILTIN(shuffle, _v2i32_v2i32, )( as_int2( v ), as_int2( m ) ) );
}

INLINE uint2 OVERLOADABLE shuffle(uint4 v, uint2 m) {
    return as_uint2( SPIRV_OCL_BUILTIN(shuffle, _v4i32_v2i32, )( as_int4( v ), as_int2( m ) ) );
}

INLINE uint2 OVERLOADABLE shuffle(uint8 v, uint2 m) {
    return as_uint2( SPIRV_OCL_BUILTIN(shuffle, _v8i32_v2i32, )( as_int8( v ), as_int2( m ) ) );
}

INLINE uint2 OVERLOADABLE shuffle(uint16 v, uint2 m) {
    return as_uint2( SPIRV_OCL_BUILTIN(shuffle, _v16i32_v2i32, )( as_int16( v ), as_int2( m ) ) );
}

INLINE uint4 OVERLOADABLE shuffle(uint2 v, uint4 m) {
    return as_uint4( SPIRV_OCL_BUILTIN(shuffle, _v2i32_v4i32, )( as_int2( v ), as_int4( m ) ) );
}

INLINE uint4 OVERLOADABLE shuffle(uint4 v, uint4 m) {
    return as_uint4( SPIRV_OCL_BUILTIN(shuffle, _v4i32_v4i32, )( as_int4( v ), as_int4( m ) ) );
}

INLINE uint4 OVERLOADABLE shuffle(uint8 v, uint4 m) {
    return as_uint4( SPIRV_OCL_BUILTIN(shuffle, _v8i32_v4i32, )( as_int8( v ), as_int4( m ) ) );
}

INLINE uint4 OVERLOADABLE shuffle(uint16 v, uint4 m) {
    return as_uint4( SPIRV_OCL_BUILTIN(shuffle, _v16i32_v4i32, )( as_int16( v ), as_int4( m ) ) );
}

INLINE uint8 OVERLOADABLE shuffle(uint2 v, uint8 m) {
    return as_uint8( SPIRV_OCL_BUILTIN(shuffle, _v2i32_v8i32, )( as_int2( v ), as_int8( m ) ) );
}

INLINE uint8 OVERLOADABLE shuffle(uint4 v, uint8 m) {
    return as_uint8( SPIRV_OCL_BUILTIN(shuffle, _v4i32_v8i32, )( as_int4( v ), as_int8( m ) ) );
}

INLINE uint8 OVERLOADABLE shuffle(uint8 v, uint8 m) {
    return as_uint8( SPIRV_OCL_BUILTIN(shuffle, _v8i32_v8i32, )( as_int8( v ), as_int8( m ) ) );
}

INLINE uint8 OVERLOADABLE shuffle(uint16 v, uint8 m) {
    return as_uint8( SPIRV_OCL_BUILTIN(shuffle, _v16i32_v8i32, )( as_int16( v ), as_int8( m ) ) );
}

INLINE uint16 OVERLOADABLE shuffle(uint2 v, uint16 m) {
    return as_uint16( SPIRV_OCL_BUILTIN(shuffle, _v2i32_v16i32, )( as_int2( v ), as_int16( m ) ) );
}

INLINE uint16 OVERLOADABLE shuffle(uint4 v, uint16 m) {
    return as_uint16( SPIRV_OCL_BUILTIN(shuffle, _v4i32_v16i32, )( as_int4( v ), as_int16( m ) ) );
}

INLINE uint16 OVERLOADABLE shuffle(uint8 v, uint16 m) {
    return as_uint16( SPIRV_OCL_BUILTIN(shuffle, _v8i32_v16i32, )( as_int8( v ), as_int16( m ) ) );
}

INLINE uint16 OVERLOADABLE shuffle(uint16 v, uint16 m) {
    return as_uint16( SPIRV_OCL_BUILTIN(shuffle, _v16i32_v16i32, )( as_int16( v ), as_int16( m ) ) );
}

INLINE long2 OVERLOADABLE shuffle(long2 v, ulong2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i64_v2i64, )( v, as_long2( m ) );
}

INLINE long2 OVERLOADABLE shuffle(long4 v, ulong2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i64_v2i64, )( v, as_long2( m ) );
}

INLINE long2 OVERLOADABLE shuffle(long8 v, ulong2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i64_v2i64, )( v, as_long2( m ) );
}

INLINE long2 OVERLOADABLE shuffle(long16 v, ulong2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i64_v2i64, )( v, as_long2( m ) );
}

INLINE long4 OVERLOADABLE shuffle(long2 v, ulong4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i64_v4i64, )( v, as_long4( m ) );
}

INLINE long4 OVERLOADABLE shuffle(long4 v, ulong4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i64_v4i64, )( v, as_long4( m ) );
}

INLINE long4 OVERLOADABLE shuffle(long8 v, ulong4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i64_v4i64, )( v, as_long4( m ) );
}

INLINE long4 OVERLOADABLE shuffle(long16 v, ulong4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i64_v4i64, )( v, as_long4( m ) );
}

INLINE long8 OVERLOADABLE shuffle(long2 v, ulong8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i64_v8i64, )( v, as_long8( m ) );
}

INLINE long8 OVERLOADABLE shuffle(long4 v, ulong8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i64_v8i64, )( v, as_long8( m ) );
}

INLINE long8 OVERLOADABLE shuffle(long8 v, ulong8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i64_v8i64, )( v, as_long8( m ) );
}

INLINE long8 OVERLOADABLE shuffle(long16 v, ulong8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i64_v8i64, )( v, as_long8( m ) );
}

INLINE long16 OVERLOADABLE shuffle(long2 v, ulong16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2i64_v16i64, )( v, as_long16( m ) );
}

INLINE long16 OVERLOADABLE shuffle(long4 v, ulong16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4i64_v16i64, )( v, as_long16( m ) );
}

INLINE long16 OVERLOADABLE shuffle(long8 v, ulong16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8i64_v16i64, )( v, as_long16( m ) );
}

INLINE long16 OVERLOADABLE shuffle(long16 v, ulong16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16i64_v16i64, )( v, as_long16( m ) );
}

INLINE ulong2 OVERLOADABLE shuffle(ulong2 v, ulong2 m) {
    return as_ulong2( SPIRV_OCL_BUILTIN(shuffle, _v2i64_v2i64, )( as_long2( v ), as_long2( m ) ) );
}

INLINE ulong2 OVERLOADABLE shuffle(ulong4 v, ulong2 m) {
    return as_ulong2( SPIRV_OCL_BUILTIN(shuffle, _v4i64_v2i64, )( as_long4( v ), as_long2( m ) ) );
}

INLINE ulong2 OVERLOADABLE shuffle(ulong8 v, ulong2 m) {
    return as_ulong2( SPIRV_OCL_BUILTIN(shuffle, _v8i64_v2i64, )( as_long8( v ), as_long2( m ) ) );
}

INLINE ulong2 OVERLOADABLE shuffle(ulong16 v, ulong2 m) {
    return as_ulong2( SPIRV_OCL_BUILTIN(shuffle, _v16i64_v2i64, )( as_long16( v ), as_long2( m ) ) );
}

INLINE ulong4 OVERLOADABLE shuffle(ulong2 v, ulong4 m) {
    return as_ulong4( SPIRV_OCL_BUILTIN(shuffle, _v2i64_v4i64, )( as_long2( v ), as_long4( m ) ) );
}

INLINE ulong4 OVERLOADABLE shuffle(ulong4 v, ulong4 m) {
    return as_ulong4( SPIRV_OCL_BUILTIN(shuffle, _v4i64_v4i64, )( as_long4( v ), as_long4( m ) ) );
}

INLINE ulong4 OVERLOADABLE shuffle(ulong8 v, ulong4 m) {
    return as_ulong4( SPIRV_OCL_BUILTIN(shuffle, _v8i64_v4i64, )( as_long8( v ), as_long4( m ) ) );
}

INLINE ulong4 OVERLOADABLE shuffle(ulong16 v, ulong4 m) {
    return as_ulong4( SPIRV_OCL_BUILTIN(shuffle, _v16i64_v4i64, )( as_long16( v ), as_long4( m ) ) );
}

INLINE ulong8 OVERLOADABLE shuffle(ulong2 v, ulong8 m) {
    return as_ulong8( SPIRV_OCL_BUILTIN(shuffle, _v2i64_v8i64, )( as_long2( v ), as_long8( m ) ) );
}

INLINE ulong8 OVERLOADABLE shuffle(ulong4 v, ulong8 m) {
    return as_ulong8( SPIRV_OCL_BUILTIN(shuffle, _v4i64_v8i64, )( as_long4( v ), as_long8( m ) ) );
}

INLINE ulong8 OVERLOADABLE shuffle(ulong8 v, ulong8 m) {
    return as_ulong8( SPIRV_OCL_BUILTIN(shuffle, _v8i64_v8i64, )( as_long8( v ), as_long8( m ) ) );
}

INLINE ulong8 OVERLOADABLE shuffle(ulong16 v, ulong8 m) {
    return as_ulong8( SPIRV_OCL_BUILTIN(shuffle, _v16i64_v8i64, )( as_long16( v ), as_long8( m ) ) );
}

INLINE ulong16 OVERLOADABLE shuffle(ulong2 v, ulong16 m) {
    return as_ulong16( SPIRV_OCL_BUILTIN(shuffle, _v2i64_v16i64, )( as_long2( v ), as_long16( m ) ) );
}

INLINE ulong16 OVERLOADABLE shuffle(ulong4 v, ulong16 m) {
    return as_ulong16( SPIRV_OCL_BUILTIN(shuffle, _v4i64_v16i64, )( as_long4( v ), as_long16( m ) ) );
}

INLINE ulong16 OVERLOADABLE shuffle(ulong8 v, ulong16 m) {
    return as_ulong16( SPIRV_OCL_BUILTIN(shuffle, _v8i64_v16i64, )( as_long8( v ), as_long16( m ) ) );
}

INLINE ulong16 OVERLOADABLE shuffle(ulong16 v, ulong16 m) {
    return as_ulong16( SPIRV_OCL_BUILTIN(shuffle, _v16i64_v16i64, )( as_long16( v ), as_long16( m ) ) );
}

INLINE float2 OVERLOADABLE shuffle(float2 v, uint2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2f32_v2i32, )( v, as_int2( m ) );
}

INLINE float2 OVERLOADABLE shuffle(float4 v, uint2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4f32_v2i32, )( v, as_int2( m ) );
}

INLINE float2 OVERLOADABLE shuffle(float8 v, uint2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8f32_v2i32, )( v, as_int2( m ) );
}

INLINE float2 OVERLOADABLE shuffle(float16 v, uint2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16f32_v2i32, )( v, as_int2( m ) );
}

INLINE float4 OVERLOADABLE shuffle(float2 v, uint4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2f32_v4i32, )( v, as_int4( m ) );
}

INLINE float4 OVERLOADABLE shuffle(float4 v, uint4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4f32_v4i32, )( v, as_int4( m ) );
}

INLINE float4 OVERLOADABLE shuffle(float8 v, uint4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8f32_v4i32, )( v, as_int4( m ) );
}

INLINE float4 OVERLOADABLE shuffle(float16 v, uint4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16f32_v4i32, )( v, as_int4( m ) );
}

INLINE float8 OVERLOADABLE shuffle(float2 v, uint8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2f32_v8i32, )( v, as_int8( m ) );
}

INLINE float8 OVERLOADABLE shuffle(float4 v, uint8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4f32_v8i32, )( v, as_int8( m ) );
}

INLINE float8 OVERLOADABLE shuffle(float8 v, uint8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8f32_v8i32, )( v, as_int8( m ) );
}

INLINE float8 OVERLOADABLE shuffle(float16 v, uint8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16f32_v8i32, )( v, as_int8( m ) );
}

INLINE float16 OVERLOADABLE shuffle(float2 v, uint16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2f32_v16i32, )( v, as_int16( m ) );
}

INLINE float16 OVERLOADABLE shuffle(float4 v, uint16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4f32_v16i32, )( v, as_int16( m ) );
}

INLINE float16 OVERLOADABLE shuffle(float8 v, uint16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8f32_v16i32, )( v, as_int16( m ) );
}

INLINE float16 OVERLOADABLE shuffle(float16 v, uint16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16f32_v16i32, )( v, as_int16( m ) );
}

INLINE char2 OVERLOADABLE shuffle2(char2 v0, char2 v1, uchar2 m) {
  char2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE char2 OVERLOADABLE shuffle2(char4 v0, char4 v1, uchar2 m) {
  char2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE char2 OVERLOADABLE shuffle2(char8 v0, char8 v1, uchar2 m) {
  char2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE char2 OVERLOADABLE shuffle2(char16 v0, char16 v1, uchar2 m) {
  char2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE char4 OVERLOADABLE shuffle2(char2 v0, char2 v1, uchar4 m) {
  char4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE char4 OVERLOADABLE shuffle2(char4 v0, char4 v1, uchar4 m) {
  char4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE char4 OVERLOADABLE shuffle2(char8 v0, char8 v1, uchar4 m) {
  char4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE char4 OVERLOADABLE shuffle2(char16 v0, char16 v1, uchar4 m) {
  char4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE char8 OVERLOADABLE shuffle2(char2 v0, char2 v1, uchar8 m) {
  char8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE char8 OVERLOADABLE shuffle2(char4 v0, char4 v1, uchar8 m) {
  char8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE char8 OVERLOADABLE shuffle2(char8 v0, char8 v1, uchar8 m) {
  char8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE char8 OVERLOADABLE shuffle2(char16 v0, char16 v1, uchar8 m) {
  char8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE char16 OVERLOADABLE shuffle2(char2 v0, char2 v1, uchar16 m) {
  char16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v1.s1 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v1.s1 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v1.s1 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v1.s1 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v1.s1 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x3) ? v1.s1 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE char16 OVERLOADABLE shuffle2(char4 v0, char4 v1, uchar16 m) {
  char16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v1.s3 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v1.s3 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v1.s3 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v1.s3 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v1.s3 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x5) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x6) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x7) ? v1.s3 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE char16 OVERLOADABLE shuffle2(char8 v0, char8 v1, uchar16 m) {
  char16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v1.s7 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0xb) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0xc) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0xd) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0xe) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0xf) ? v1.s7 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0xb) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0xc) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0xd) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0xe) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0xf) ? v1.s7 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0xb) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0xc) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0xd) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0xe) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0xf) ? v1.s7 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0xb) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0xc) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0xd) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0xe) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0xf) ? v1.s7 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x9) ? v1.s1 : ret.se;
  ret.se = (m.se == 0xa) ? v1.s2 : ret.se;
  ret.se = (m.se == 0xb) ? v1.s3 : ret.se;
  ret.se = (m.se == 0xc) ? v1.s4 : ret.se;
  ret.se = (m.se == 0xd) ? v1.s5 : ret.se;
  ret.se = (m.se == 0xe) ? v1.s6 : ret.se;
  ret.se = (m.se == 0xf) ? v1.s7 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0xb) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0xc) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0xd) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0xe) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE char16 OVERLOADABLE shuffle2(char16 v0, char16 v1, uchar16 m) {
  char16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v0.sa : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v0.sb : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v0.sc : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v0.sd : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v0.se : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v0.sf : ret.s8;
  ret.s8 = (m.s8 == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = (m.s8 == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = (m.s8 == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = (m.s8 == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = (m.s8 == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = (m.s8 == 0x1e) ? v1.se : ret.s8;
  ret.s8 = (m.s8 == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v0.sa : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v0.sb : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v0.sc : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v0.sd : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v0.se : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v0.sf : ret.s9;
  ret.s9 = (m.s9 == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = (m.s9 == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = (m.s9 == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = (m.s9 == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = (m.s9 == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = (m.s9 == 0x1e) ? v1.se : ret.s9;
  ret.s9 = (m.s9 == 0x1f) ? v1.sf : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v0.s8 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v0.s9 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v0.sa : ret.sa;
  ret.sa = (m.sa == 0xb) ? v0.sb : ret.sa;
  ret.sa = (m.sa == 0xc) ? v0.sc : ret.sa;
  ret.sa = (m.sa == 0xd) ? v0.sd : ret.sa;
  ret.sa = (m.sa == 0xe) ? v0.se : ret.sa;
  ret.sa = (m.sa == 0xf) ? v0.sf : ret.sa;
  ret.sa = (m.sa == 0x10) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x11) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x12) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x13) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0x14) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0x15) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0x16) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0x17) ? v1.s7 : ret.sa;
  ret.sa = (m.sa == 0x18) ? v1.s8 : ret.sa;
  ret.sa = (m.sa == 0x19) ? v1.s9 : ret.sa;
  ret.sa = (m.sa == 0x1a) ? v1.sa : ret.sa;
  ret.sa = (m.sa == 0x1b) ? v1.sb : ret.sa;
  ret.sa = (m.sa == 0x1c) ? v1.sc : ret.sa;
  ret.sa = (m.sa == 0x1d) ? v1.sd : ret.sa;
  ret.sa = (m.sa == 0x1e) ? v1.se : ret.sa;
  ret.sa = (m.sa == 0x1f) ? v1.sf : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v0.s8 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v0.s9 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v0.sa : ret.sb;
  ret.sb = (m.sb == 0xb) ? v0.sb : ret.sb;
  ret.sb = (m.sb == 0xc) ? v0.sc : ret.sb;
  ret.sb = (m.sb == 0xd) ? v0.sd : ret.sb;
  ret.sb = (m.sb == 0xe) ? v0.se : ret.sb;
  ret.sb = (m.sb == 0xf) ? v0.sf : ret.sb;
  ret.sb = (m.sb == 0x10) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x11) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x12) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x13) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0x14) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0x15) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0x16) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0x17) ? v1.s7 : ret.sb;
  ret.sb = (m.sb == 0x18) ? v1.s8 : ret.sb;
  ret.sb = (m.sb == 0x19) ? v1.s9 : ret.sb;
  ret.sb = (m.sb == 0x1a) ? v1.sa : ret.sb;
  ret.sb = (m.sb == 0x1b) ? v1.sb : ret.sb;
  ret.sb = (m.sb == 0x1c) ? v1.sc : ret.sb;
  ret.sb = (m.sb == 0x1d) ? v1.sd : ret.sb;
  ret.sb = (m.sb == 0x1e) ? v1.se : ret.sb;
  ret.sb = (m.sb == 0x1f) ? v1.sf : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v0.s8 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v0.s9 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v0.sa : ret.sc;
  ret.sc = (m.sc == 0xb) ? v0.sb : ret.sc;
  ret.sc = (m.sc == 0xc) ? v0.sc : ret.sc;
  ret.sc = (m.sc == 0xd) ? v0.sd : ret.sc;
  ret.sc = (m.sc == 0xe) ? v0.se : ret.sc;
  ret.sc = (m.sc == 0xf) ? v0.sf : ret.sc;
  ret.sc = (m.sc == 0x10) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x11) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x12) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x13) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0x14) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0x15) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0x16) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0x17) ? v1.s7 : ret.sc;
  ret.sc = (m.sc == 0x18) ? v1.s8 : ret.sc;
  ret.sc = (m.sc == 0x19) ? v1.s9 : ret.sc;
  ret.sc = (m.sc == 0x1a) ? v1.sa : ret.sc;
  ret.sc = (m.sc == 0x1b) ? v1.sb : ret.sc;
  ret.sc = (m.sc == 0x1c) ? v1.sc : ret.sc;
  ret.sc = (m.sc == 0x1d) ? v1.sd : ret.sc;
  ret.sc = (m.sc == 0x1e) ? v1.se : ret.sc;
  ret.sc = (m.sc == 0x1f) ? v1.sf : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v0.s8 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v0.s9 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v0.sa : ret.sd;
  ret.sd = (m.sd == 0xb) ? v0.sb : ret.sd;
  ret.sd = (m.sd == 0xc) ? v0.sc : ret.sd;
  ret.sd = (m.sd == 0xd) ? v0.sd : ret.sd;
  ret.sd = (m.sd == 0xe) ? v0.se : ret.sd;
  ret.sd = (m.sd == 0xf) ? v0.sf : ret.sd;
  ret.sd = (m.sd == 0x10) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x11) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x12) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x13) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0x14) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0x15) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0x16) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0x17) ? v1.s7 : ret.sd;
  ret.sd = (m.sd == 0x18) ? v1.s8 : ret.sd;
  ret.sd = (m.sd == 0x19) ? v1.s9 : ret.sd;
  ret.sd = (m.sd == 0x1a) ? v1.sa : ret.sd;
  ret.sd = (m.sd == 0x1b) ? v1.sb : ret.sd;
  ret.sd = (m.sd == 0x1c) ? v1.sc : ret.sd;
  ret.sd = (m.sd == 0x1d) ? v1.sd : ret.sd;
  ret.sd = (m.sd == 0x1e) ? v1.se : ret.sd;
  ret.sd = (m.sd == 0x1f) ? v1.sf : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v0.s8 : ret.se;
  ret.se = (m.se == 0x9) ? v0.s9 : ret.se;
  ret.se = (m.se == 0xa) ? v0.sa : ret.se;
  ret.se = (m.se == 0xb) ? v0.sb : ret.se;
  ret.se = (m.se == 0xc) ? v0.sc : ret.se;
  ret.se = (m.se == 0xd) ? v0.sd : ret.se;
  ret.se = (m.se == 0xe) ? v0.se : ret.se;
  ret.se = (m.se == 0xf) ? v0.sf : ret.se;
  ret.se = (m.se == 0x10) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x11) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x12) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x13) ? v1.s3 : ret.se;
  ret.se = (m.se == 0x14) ? v1.s4 : ret.se;
  ret.se = (m.se == 0x15) ? v1.s5 : ret.se;
  ret.se = (m.se == 0x16) ? v1.s6 : ret.se;
  ret.se = (m.se == 0x17) ? v1.s7 : ret.se;
  ret.se = (m.se == 0x18) ? v1.s8 : ret.se;
  ret.se = (m.se == 0x19) ? v1.s9 : ret.se;
  ret.se = (m.se == 0x1a) ? v1.sa : ret.se;
  ret.se = (m.se == 0x1b) ? v1.sb : ret.se;
  ret.se = (m.se == 0x1c) ? v1.sc : ret.se;
  ret.se = (m.se == 0x1d) ? v1.sd : ret.se;
  ret.se = (m.se == 0x1e) ? v1.se : ret.se;
  ret.se = (m.se == 0x1f) ? v1.sf : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v0.s8 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v0.s9 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v0.sa : ret.sf;
  ret.sf = (m.sf == 0xb) ? v0.sb : ret.sf;
  ret.sf = (m.sf == 0xc) ? v0.sc : ret.sf;
  ret.sf = (m.sf == 0xd) ? v0.sd : ret.sf;
  ret.sf = (m.sf == 0xe) ? v0.se : ret.sf;
  ret.sf = (m.sf == 0xf) ? v0.sf : ret.sf;
  ret.sf = (m.sf == 0x10) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x11) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x12) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x13) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0x14) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0x15) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0x16) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0x17) ? v1.s7 : ret.sf;
  ret.sf = (m.sf == 0x18) ? v1.s8 : ret.sf;
  ret.sf = (m.sf == 0x19) ? v1.s9 : ret.sf;
  ret.sf = (m.sf == 0x1a) ? v1.sa : ret.sf;
  ret.sf = (m.sf == 0x1b) ? v1.sb : ret.sf;
  ret.sf = (m.sf == 0x1c) ? v1.sc : ret.sf;
  ret.sf = (m.sf == 0x1d) ? v1.sd : ret.sf;
  ret.sf = (m.sf == 0x1e) ? v1.se : ret.sf;
  ret.sf = (m.sf == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

INLINE uchar2 OVERLOADABLE shuffle2(uchar2 v0, uchar2 v1, uchar2 m) {
  uchar2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE uchar2 OVERLOADABLE shuffle2(uchar4 v0, uchar4 v1, uchar2 m) {
  uchar2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE uchar2 OVERLOADABLE shuffle2(uchar8 v0, uchar8 v1, uchar2 m) {
  uchar2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE uchar2 OVERLOADABLE shuffle2(uchar16 v0, uchar16 v1, uchar2 m) {
  uchar2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE uchar4 OVERLOADABLE shuffle2(uchar2 v0, uchar2 v1, uchar4 m) {
  uchar4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE uchar4 OVERLOADABLE shuffle2(uchar4 v0, uchar4 v1, uchar4 m) {
  uchar4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE uchar4 OVERLOADABLE shuffle2(uchar8 v0, uchar8 v1, uchar4 m) {
  uchar4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE uchar4 OVERLOADABLE shuffle2(uchar16 v0, uchar16 v1, uchar4 m) {
  uchar4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE uchar8 OVERLOADABLE shuffle2(uchar2 v0, uchar2 v1, uchar8 m) {
  uchar8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE uchar8 OVERLOADABLE shuffle2(uchar4 v0, uchar4 v1, uchar8 m) {
  uchar8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE uchar8 OVERLOADABLE shuffle2(uchar8 v0, uchar8 v1, uchar8 m) {
  uchar8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE uchar8 OVERLOADABLE shuffle2(uchar16 v0, uchar16 v1, uchar8 m) {
  uchar8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE uchar16 OVERLOADABLE shuffle2(uchar2 v0, uchar2 v1, uchar16 m) {
  uchar16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v1.s1 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v1.s1 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v1.s1 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v1.s1 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v1.s1 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x3) ? v1.s1 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE uchar16 OVERLOADABLE shuffle2(uchar4 v0, uchar4 v1, uchar16 m) {
  uchar16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v1.s3 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v1.s3 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v1.s3 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v1.s3 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v1.s3 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x5) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x6) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x7) ? v1.s3 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE uchar16 OVERLOADABLE shuffle2(uchar8 v0, uchar8 v1, uchar16 m) {
  uchar16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v1.s7 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0xb) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0xc) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0xd) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0xe) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0xf) ? v1.s7 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0xb) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0xc) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0xd) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0xe) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0xf) ? v1.s7 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0xb) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0xc) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0xd) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0xe) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0xf) ? v1.s7 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0xb) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0xc) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0xd) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0xe) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0xf) ? v1.s7 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x9) ? v1.s1 : ret.se;
  ret.se = (m.se == 0xa) ? v1.s2 : ret.se;
  ret.se = (m.se == 0xb) ? v1.s3 : ret.se;
  ret.se = (m.se == 0xc) ? v1.s4 : ret.se;
  ret.se = (m.se == 0xd) ? v1.s5 : ret.se;
  ret.se = (m.se == 0xe) ? v1.s6 : ret.se;
  ret.se = (m.se == 0xf) ? v1.s7 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0xb) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0xc) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0xd) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0xe) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE uchar16 OVERLOADABLE shuffle2(uchar16 v0, uchar16 v1, uchar16 m) {
  uchar16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v0.sa : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v0.sb : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v0.sc : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v0.sd : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v0.se : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v0.sf : ret.s8;
  ret.s8 = (m.s8 == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = (m.s8 == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = (m.s8 == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = (m.s8 == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = (m.s8 == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = (m.s8 == 0x1e) ? v1.se : ret.s8;
  ret.s8 = (m.s8 == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v0.sa : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v0.sb : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v0.sc : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v0.sd : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v0.se : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v0.sf : ret.s9;
  ret.s9 = (m.s9 == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = (m.s9 == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = (m.s9 == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = (m.s9 == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = (m.s9 == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = (m.s9 == 0x1e) ? v1.se : ret.s9;
  ret.s9 = (m.s9 == 0x1f) ? v1.sf : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v0.s8 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v0.s9 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v0.sa : ret.sa;
  ret.sa = (m.sa == 0xb) ? v0.sb : ret.sa;
  ret.sa = (m.sa == 0xc) ? v0.sc : ret.sa;
  ret.sa = (m.sa == 0xd) ? v0.sd : ret.sa;
  ret.sa = (m.sa == 0xe) ? v0.se : ret.sa;
  ret.sa = (m.sa == 0xf) ? v0.sf : ret.sa;
  ret.sa = (m.sa == 0x10) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x11) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x12) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x13) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0x14) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0x15) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0x16) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0x17) ? v1.s7 : ret.sa;
  ret.sa = (m.sa == 0x18) ? v1.s8 : ret.sa;
  ret.sa = (m.sa == 0x19) ? v1.s9 : ret.sa;
  ret.sa = (m.sa == 0x1a) ? v1.sa : ret.sa;
  ret.sa = (m.sa == 0x1b) ? v1.sb : ret.sa;
  ret.sa = (m.sa == 0x1c) ? v1.sc : ret.sa;
  ret.sa = (m.sa == 0x1d) ? v1.sd : ret.sa;
  ret.sa = (m.sa == 0x1e) ? v1.se : ret.sa;
  ret.sa = (m.sa == 0x1f) ? v1.sf : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v0.s8 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v0.s9 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v0.sa : ret.sb;
  ret.sb = (m.sb == 0xb) ? v0.sb : ret.sb;
  ret.sb = (m.sb == 0xc) ? v0.sc : ret.sb;
  ret.sb = (m.sb == 0xd) ? v0.sd : ret.sb;
  ret.sb = (m.sb == 0xe) ? v0.se : ret.sb;
  ret.sb = (m.sb == 0xf) ? v0.sf : ret.sb;
  ret.sb = (m.sb == 0x10) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x11) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x12) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x13) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0x14) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0x15) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0x16) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0x17) ? v1.s7 : ret.sb;
  ret.sb = (m.sb == 0x18) ? v1.s8 : ret.sb;
  ret.sb = (m.sb == 0x19) ? v1.s9 : ret.sb;
  ret.sb = (m.sb == 0x1a) ? v1.sa : ret.sb;
  ret.sb = (m.sb == 0x1b) ? v1.sb : ret.sb;
  ret.sb = (m.sb == 0x1c) ? v1.sc : ret.sb;
  ret.sb = (m.sb == 0x1d) ? v1.sd : ret.sb;
  ret.sb = (m.sb == 0x1e) ? v1.se : ret.sb;
  ret.sb = (m.sb == 0x1f) ? v1.sf : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v0.s8 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v0.s9 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v0.sa : ret.sc;
  ret.sc = (m.sc == 0xb) ? v0.sb : ret.sc;
  ret.sc = (m.sc == 0xc) ? v0.sc : ret.sc;
  ret.sc = (m.sc == 0xd) ? v0.sd : ret.sc;
  ret.sc = (m.sc == 0xe) ? v0.se : ret.sc;
  ret.sc = (m.sc == 0xf) ? v0.sf : ret.sc;
  ret.sc = (m.sc == 0x10) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x11) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x12) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x13) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0x14) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0x15) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0x16) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0x17) ? v1.s7 : ret.sc;
  ret.sc = (m.sc == 0x18) ? v1.s8 : ret.sc;
  ret.sc = (m.sc == 0x19) ? v1.s9 : ret.sc;
  ret.sc = (m.sc == 0x1a) ? v1.sa : ret.sc;
  ret.sc = (m.sc == 0x1b) ? v1.sb : ret.sc;
  ret.sc = (m.sc == 0x1c) ? v1.sc : ret.sc;
  ret.sc = (m.sc == 0x1d) ? v1.sd : ret.sc;
  ret.sc = (m.sc == 0x1e) ? v1.se : ret.sc;
  ret.sc = (m.sc == 0x1f) ? v1.sf : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v0.s8 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v0.s9 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v0.sa : ret.sd;
  ret.sd = (m.sd == 0xb) ? v0.sb : ret.sd;
  ret.sd = (m.sd == 0xc) ? v0.sc : ret.sd;
  ret.sd = (m.sd == 0xd) ? v0.sd : ret.sd;
  ret.sd = (m.sd == 0xe) ? v0.se : ret.sd;
  ret.sd = (m.sd == 0xf) ? v0.sf : ret.sd;
  ret.sd = (m.sd == 0x10) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x11) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x12) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x13) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0x14) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0x15) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0x16) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0x17) ? v1.s7 : ret.sd;
  ret.sd = (m.sd == 0x18) ? v1.s8 : ret.sd;
  ret.sd = (m.sd == 0x19) ? v1.s9 : ret.sd;
  ret.sd = (m.sd == 0x1a) ? v1.sa : ret.sd;
  ret.sd = (m.sd == 0x1b) ? v1.sb : ret.sd;
  ret.sd = (m.sd == 0x1c) ? v1.sc : ret.sd;
  ret.sd = (m.sd == 0x1d) ? v1.sd : ret.sd;
  ret.sd = (m.sd == 0x1e) ? v1.se : ret.sd;
  ret.sd = (m.sd == 0x1f) ? v1.sf : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v0.s8 : ret.se;
  ret.se = (m.se == 0x9) ? v0.s9 : ret.se;
  ret.se = (m.se == 0xa) ? v0.sa : ret.se;
  ret.se = (m.se == 0xb) ? v0.sb : ret.se;
  ret.se = (m.se == 0xc) ? v0.sc : ret.se;
  ret.se = (m.se == 0xd) ? v0.sd : ret.se;
  ret.se = (m.se == 0xe) ? v0.se : ret.se;
  ret.se = (m.se == 0xf) ? v0.sf : ret.se;
  ret.se = (m.se == 0x10) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x11) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x12) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x13) ? v1.s3 : ret.se;
  ret.se = (m.se == 0x14) ? v1.s4 : ret.se;
  ret.se = (m.se == 0x15) ? v1.s5 : ret.se;
  ret.se = (m.se == 0x16) ? v1.s6 : ret.se;
  ret.se = (m.se == 0x17) ? v1.s7 : ret.se;
  ret.se = (m.se == 0x18) ? v1.s8 : ret.se;
  ret.se = (m.se == 0x19) ? v1.s9 : ret.se;
  ret.se = (m.se == 0x1a) ? v1.sa : ret.se;
  ret.se = (m.se == 0x1b) ? v1.sb : ret.se;
  ret.se = (m.se == 0x1c) ? v1.sc : ret.se;
  ret.se = (m.se == 0x1d) ? v1.sd : ret.se;
  ret.se = (m.se == 0x1e) ? v1.se : ret.se;
  ret.se = (m.se == 0x1f) ? v1.sf : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v0.s8 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v0.s9 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v0.sa : ret.sf;
  ret.sf = (m.sf == 0xb) ? v0.sb : ret.sf;
  ret.sf = (m.sf == 0xc) ? v0.sc : ret.sf;
  ret.sf = (m.sf == 0xd) ? v0.sd : ret.sf;
  ret.sf = (m.sf == 0xe) ? v0.se : ret.sf;
  ret.sf = (m.sf == 0xf) ? v0.sf : ret.sf;
  ret.sf = (m.sf == 0x10) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x11) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x12) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x13) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0x14) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0x15) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0x16) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0x17) ? v1.s7 : ret.sf;
  ret.sf = (m.sf == 0x18) ? v1.s8 : ret.sf;
  ret.sf = (m.sf == 0x19) ? v1.s9 : ret.sf;
  ret.sf = (m.sf == 0x1a) ? v1.sa : ret.sf;
  ret.sf = (m.sf == 0x1b) ? v1.sb : ret.sf;
  ret.sf = (m.sf == 0x1c) ? v1.sc : ret.sf;
  ret.sf = (m.sf == 0x1d) ? v1.sd : ret.sf;
  ret.sf = (m.sf == 0x1e) ? v1.se : ret.sf;
  ret.sf = (m.sf == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

INLINE short2 OVERLOADABLE shuffle2(short2 v0, short2 v1, ushort2 m) {
  short2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE short2 OVERLOADABLE shuffle2(short4 v0, short4 v1, ushort2 m) {
  short2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE short2 OVERLOADABLE shuffle2(short8 v0, short8 v1, ushort2 m) {
  short2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE short2 OVERLOADABLE shuffle2(short16 v0, short16 v1, ushort2 m) {
  short2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE short4 OVERLOADABLE shuffle2(short2 v0, short2 v1, ushort4 m) {
  short4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE short4 OVERLOADABLE shuffle2(short4 v0, short4 v1, ushort4 m) {
  short4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE short4 OVERLOADABLE shuffle2(short8 v0, short8 v1, ushort4 m) {
  short4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE short4 OVERLOADABLE shuffle2(short16 v0, short16 v1, ushort4 m) {
  short4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE short8 OVERLOADABLE shuffle2(short2 v0, short2 v1, ushort8 m) {
  short8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE short8 OVERLOADABLE shuffle2(short4 v0, short4 v1, ushort8 m) {
  short8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE short8 OVERLOADABLE shuffle2(short8 v0, short8 v1, ushort8 m) {
  short8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE short8 OVERLOADABLE shuffle2(short16 v0, short16 v1, ushort8 m) {
  short8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE short16 OVERLOADABLE shuffle2(short2 v0, short2 v1, ushort16 m) {
  short16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v1.s1 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v1.s1 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v1.s1 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v1.s1 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v1.s1 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x3) ? v1.s1 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE short16 OVERLOADABLE shuffle2(short4 v0, short4 v1, ushort16 m) {
  short16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v1.s3 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v1.s3 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v1.s3 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v1.s3 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v1.s3 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x5) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x6) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x7) ? v1.s3 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE short16 OVERLOADABLE shuffle2(short8 v0, short8 v1, ushort16 m) {
  short16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v1.s7 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0xb) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0xc) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0xd) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0xe) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0xf) ? v1.s7 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0xb) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0xc) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0xd) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0xe) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0xf) ? v1.s7 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0xb) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0xc) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0xd) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0xe) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0xf) ? v1.s7 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0xb) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0xc) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0xd) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0xe) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0xf) ? v1.s7 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x9) ? v1.s1 : ret.se;
  ret.se = (m.se == 0xa) ? v1.s2 : ret.se;
  ret.se = (m.se == 0xb) ? v1.s3 : ret.se;
  ret.se = (m.se == 0xc) ? v1.s4 : ret.se;
  ret.se = (m.se == 0xd) ? v1.s5 : ret.se;
  ret.se = (m.se == 0xe) ? v1.s6 : ret.se;
  ret.se = (m.se == 0xf) ? v1.s7 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0xb) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0xc) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0xd) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0xe) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE short16 OVERLOADABLE shuffle2(short16 v0, short16 v1, ushort16 m) {
  short16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v0.sa : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v0.sb : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v0.sc : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v0.sd : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v0.se : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v0.sf : ret.s8;
  ret.s8 = (m.s8 == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = (m.s8 == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = (m.s8 == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = (m.s8 == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = (m.s8 == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = (m.s8 == 0x1e) ? v1.se : ret.s8;
  ret.s8 = (m.s8 == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v0.sa : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v0.sb : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v0.sc : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v0.sd : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v0.se : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v0.sf : ret.s9;
  ret.s9 = (m.s9 == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = (m.s9 == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = (m.s9 == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = (m.s9 == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = (m.s9 == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = (m.s9 == 0x1e) ? v1.se : ret.s9;
  ret.s9 = (m.s9 == 0x1f) ? v1.sf : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v0.s8 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v0.s9 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v0.sa : ret.sa;
  ret.sa = (m.sa == 0xb) ? v0.sb : ret.sa;
  ret.sa = (m.sa == 0xc) ? v0.sc : ret.sa;
  ret.sa = (m.sa == 0xd) ? v0.sd : ret.sa;
  ret.sa = (m.sa == 0xe) ? v0.se : ret.sa;
  ret.sa = (m.sa == 0xf) ? v0.sf : ret.sa;
  ret.sa = (m.sa == 0x10) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x11) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x12) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x13) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0x14) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0x15) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0x16) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0x17) ? v1.s7 : ret.sa;
  ret.sa = (m.sa == 0x18) ? v1.s8 : ret.sa;
  ret.sa = (m.sa == 0x19) ? v1.s9 : ret.sa;
  ret.sa = (m.sa == 0x1a) ? v1.sa : ret.sa;
  ret.sa = (m.sa == 0x1b) ? v1.sb : ret.sa;
  ret.sa = (m.sa == 0x1c) ? v1.sc : ret.sa;
  ret.sa = (m.sa == 0x1d) ? v1.sd : ret.sa;
  ret.sa = (m.sa == 0x1e) ? v1.se : ret.sa;
  ret.sa = (m.sa == 0x1f) ? v1.sf : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v0.s8 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v0.s9 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v0.sa : ret.sb;
  ret.sb = (m.sb == 0xb) ? v0.sb : ret.sb;
  ret.sb = (m.sb == 0xc) ? v0.sc : ret.sb;
  ret.sb = (m.sb == 0xd) ? v0.sd : ret.sb;
  ret.sb = (m.sb == 0xe) ? v0.se : ret.sb;
  ret.sb = (m.sb == 0xf) ? v0.sf : ret.sb;
  ret.sb = (m.sb == 0x10) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x11) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x12) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x13) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0x14) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0x15) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0x16) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0x17) ? v1.s7 : ret.sb;
  ret.sb = (m.sb == 0x18) ? v1.s8 : ret.sb;
  ret.sb = (m.sb == 0x19) ? v1.s9 : ret.sb;
  ret.sb = (m.sb == 0x1a) ? v1.sa : ret.sb;
  ret.sb = (m.sb == 0x1b) ? v1.sb : ret.sb;
  ret.sb = (m.sb == 0x1c) ? v1.sc : ret.sb;
  ret.sb = (m.sb == 0x1d) ? v1.sd : ret.sb;
  ret.sb = (m.sb == 0x1e) ? v1.se : ret.sb;
  ret.sb = (m.sb == 0x1f) ? v1.sf : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v0.s8 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v0.s9 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v0.sa : ret.sc;
  ret.sc = (m.sc == 0xb) ? v0.sb : ret.sc;
  ret.sc = (m.sc == 0xc) ? v0.sc : ret.sc;
  ret.sc = (m.sc == 0xd) ? v0.sd : ret.sc;
  ret.sc = (m.sc == 0xe) ? v0.se : ret.sc;
  ret.sc = (m.sc == 0xf) ? v0.sf : ret.sc;
  ret.sc = (m.sc == 0x10) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x11) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x12) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x13) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0x14) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0x15) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0x16) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0x17) ? v1.s7 : ret.sc;
  ret.sc = (m.sc == 0x18) ? v1.s8 : ret.sc;
  ret.sc = (m.sc == 0x19) ? v1.s9 : ret.sc;
  ret.sc = (m.sc == 0x1a) ? v1.sa : ret.sc;
  ret.sc = (m.sc == 0x1b) ? v1.sb : ret.sc;
  ret.sc = (m.sc == 0x1c) ? v1.sc : ret.sc;
  ret.sc = (m.sc == 0x1d) ? v1.sd : ret.sc;
  ret.sc = (m.sc == 0x1e) ? v1.se : ret.sc;
  ret.sc = (m.sc == 0x1f) ? v1.sf : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v0.s8 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v0.s9 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v0.sa : ret.sd;
  ret.sd = (m.sd == 0xb) ? v0.sb : ret.sd;
  ret.sd = (m.sd == 0xc) ? v0.sc : ret.sd;
  ret.sd = (m.sd == 0xd) ? v0.sd : ret.sd;
  ret.sd = (m.sd == 0xe) ? v0.se : ret.sd;
  ret.sd = (m.sd == 0xf) ? v0.sf : ret.sd;
  ret.sd = (m.sd == 0x10) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x11) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x12) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x13) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0x14) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0x15) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0x16) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0x17) ? v1.s7 : ret.sd;
  ret.sd = (m.sd == 0x18) ? v1.s8 : ret.sd;
  ret.sd = (m.sd == 0x19) ? v1.s9 : ret.sd;
  ret.sd = (m.sd == 0x1a) ? v1.sa : ret.sd;
  ret.sd = (m.sd == 0x1b) ? v1.sb : ret.sd;
  ret.sd = (m.sd == 0x1c) ? v1.sc : ret.sd;
  ret.sd = (m.sd == 0x1d) ? v1.sd : ret.sd;
  ret.sd = (m.sd == 0x1e) ? v1.se : ret.sd;
  ret.sd = (m.sd == 0x1f) ? v1.sf : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v0.s8 : ret.se;
  ret.se = (m.se == 0x9) ? v0.s9 : ret.se;
  ret.se = (m.se == 0xa) ? v0.sa : ret.se;
  ret.se = (m.se == 0xb) ? v0.sb : ret.se;
  ret.se = (m.se == 0xc) ? v0.sc : ret.se;
  ret.se = (m.se == 0xd) ? v0.sd : ret.se;
  ret.se = (m.se == 0xe) ? v0.se : ret.se;
  ret.se = (m.se == 0xf) ? v0.sf : ret.se;
  ret.se = (m.se == 0x10) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x11) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x12) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x13) ? v1.s3 : ret.se;
  ret.se = (m.se == 0x14) ? v1.s4 : ret.se;
  ret.se = (m.se == 0x15) ? v1.s5 : ret.se;
  ret.se = (m.se == 0x16) ? v1.s6 : ret.se;
  ret.se = (m.se == 0x17) ? v1.s7 : ret.se;
  ret.se = (m.se == 0x18) ? v1.s8 : ret.se;
  ret.se = (m.se == 0x19) ? v1.s9 : ret.se;
  ret.se = (m.se == 0x1a) ? v1.sa : ret.se;
  ret.se = (m.se == 0x1b) ? v1.sb : ret.se;
  ret.se = (m.se == 0x1c) ? v1.sc : ret.se;
  ret.se = (m.se == 0x1d) ? v1.sd : ret.se;
  ret.se = (m.se == 0x1e) ? v1.se : ret.se;
  ret.se = (m.se == 0x1f) ? v1.sf : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v0.s8 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v0.s9 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v0.sa : ret.sf;
  ret.sf = (m.sf == 0xb) ? v0.sb : ret.sf;
  ret.sf = (m.sf == 0xc) ? v0.sc : ret.sf;
  ret.sf = (m.sf == 0xd) ? v0.sd : ret.sf;
  ret.sf = (m.sf == 0xe) ? v0.se : ret.sf;
  ret.sf = (m.sf == 0xf) ? v0.sf : ret.sf;
  ret.sf = (m.sf == 0x10) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x11) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x12) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x13) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0x14) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0x15) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0x16) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0x17) ? v1.s7 : ret.sf;
  ret.sf = (m.sf == 0x18) ? v1.s8 : ret.sf;
  ret.sf = (m.sf == 0x19) ? v1.s9 : ret.sf;
  ret.sf = (m.sf == 0x1a) ? v1.sa : ret.sf;
  ret.sf = (m.sf == 0x1b) ? v1.sb : ret.sf;
  ret.sf = (m.sf == 0x1c) ? v1.sc : ret.sf;
  ret.sf = (m.sf == 0x1d) ? v1.sd : ret.sf;
  ret.sf = (m.sf == 0x1e) ? v1.se : ret.sf;
  ret.sf = (m.sf == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

INLINE ushort2 OVERLOADABLE shuffle2(ushort2 v0, ushort2 v1, ushort2 m) {
  ushort2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE ushort2 OVERLOADABLE shuffle2(ushort4 v0, ushort4 v1, ushort2 m) {
  ushort2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE ushort2 OVERLOADABLE shuffle2(ushort8 v0, ushort8 v1, ushort2 m) {
  ushort2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE ushort2 OVERLOADABLE shuffle2(ushort16 v0, ushort16 v1, ushort2 m) {
  ushort2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE ushort4 OVERLOADABLE shuffle2(ushort2 v0, ushort2 v1, ushort4 m) {
  ushort4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE ushort4 OVERLOADABLE shuffle2(ushort4 v0, ushort4 v1, ushort4 m) {
  ushort4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE ushort4 OVERLOADABLE shuffle2(ushort8 v0, ushort8 v1, ushort4 m) {
  ushort4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE ushort4 OVERLOADABLE shuffle2(ushort16 v0, ushort16 v1, ushort4 m) {
  ushort4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE ushort8 OVERLOADABLE shuffle2(ushort2 v0, ushort2 v1, ushort8 m) {
  ushort8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE ushort8 OVERLOADABLE shuffle2(ushort4 v0, ushort4 v1, ushort8 m) {
  ushort8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE ushort8 OVERLOADABLE shuffle2(ushort8 v0, ushort8 v1, ushort8 m) {
  ushort8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE ushort8 OVERLOADABLE shuffle2(ushort16 v0, ushort16 v1, ushort8 m) {
  ushort8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE ushort16 OVERLOADABLE shuffle2(ushort2 v0, ushort2 v1, ushort16 m) {
  ushort16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v1.s1 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v1.s1 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v1.s1 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v1.s1 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v1.s1 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x3) ? v1.s1 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE ushort16 OVERLOADABLE shuffle2(ushort4 v0, ushort4 v1, ushort16 m) {
  ushort16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v1.s3 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v1.s3 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v1.s3 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v1.s3 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v1.s3 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x5) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x6) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x7) ? v1.s3 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE ushort16 OVERLOADABLE shuffle2(ushort8 v0, ushort8 v1, ushort16 m) {
  ushort16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v1.s7 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0xb) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0xc) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0xd) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0xe) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0xf) ? v1.s7 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0xb) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0xc) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0xd) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0xe) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0xf) ? v1.s7 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0xb) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0xc) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0xd) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0xe) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0xf) ? v1.s7 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0xb) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0xc) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0xd) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0xe) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0xf) ? v1.s7 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x9) ? v1.s1 : ret.se;
  ret.se = (m.se == 0xa) ? v1.s2 : ret.se;
  ret.se = (m.se == 0xb) ? v1.s3 : ret.se;
  ret.se = (m.se == 0xc) ? v1.s4 : ret.se;
  ret.se = (m.se == 0xd) ? v1.s5 : ret.se;
  ret.se = (m.se == 0xe) ? v1.s6 : ret.se;
  ret.se = (m.se == 0xf) ? v1.s7 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0xb) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0xc) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0xd) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0xe) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE ushort16 OVERLOADABLE shuffle2(ushort16 v0, ushort16 v1, ushort16 m) {
  ushort16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v0.sa : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v0.sb : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v0.sc : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v0.sd : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v0.se : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v0.sf : ret.s8;
  ret.s8 = (m.s8 == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = (m.s8 == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = (m.s8 == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = (m.s8 == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = (m.s8 == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = (m.s8 == 0x1e) ? v1.se : ret.s8;
  ret.s8 = (m.s8 == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v0.sa : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v0.sb : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v0.sc : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v0.sd : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v0.se : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v0.sf : ret.s9;
  ret.s9 = (m.s9 == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = (m.s9 == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = (m.s9 == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = (m.s9 == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = (m.s9 == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = (m.s9 == 0x1e) ? v1.se : ret.s9;
  ret.s9 = (m.s9 == 0x1f) ? v1.sf : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v0.s8 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v0.s9 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v0.sa : ret.sa;
  ret.sa = (m.sa == 0xb) ? v0.sb : ret.sa;
  ret.sa = (m.sa == 0xc) ? v0.sc : ret.sa;
  ret.sa = (m.sa == 0xd) ? v0.sd : ret.sa;
  ret.sa = (m.sa == 0xe) ? v0.se : ret.sa;
  ret.sa = (m.sa == 0xf) ? v0.sf : ret.sa;
  ret.sa = (m.sa == 0x10) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x11) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x12) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x13) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0x14) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0x15) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0x16) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0x17) ? v1.s7 : ret.sa;
  ret.sa = (m.sa == 0x18) ? v1.s8 : ret.sa;
  ret.sa = (m.sa == 0x19) ? v1.s9 : ret.sa;
  ret.sa = (m.sa == 0x1a) ? v1.sa : ret.sa;
  ret.sa = (m.sa == 0x1b) ? v1.sb : ret.sa;
  ret.sa = (m.sa == 0x1c) ? v1.sc : ret.sa;
  ret.sa = (m.sa == 0x1d) ? v1.sd : ret.sa;
  ret.sa = (m.sa == 0x1e) ? v1.se : ret.sa;
  ret.sa = (m.sa == 0x1f) ? v1.sf : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v0.s8 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v0.s9 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v0.sa : ret.sb;
  ret.sb = (m.sb == 0xb) ? v0.sb : ret.sb;
  ret.sb = (m.sb == 0xc) ? v0.sc : ret.sb;
  ret.sb = (m.sb == 0xd) ? v0.sd : ret.sb;
  ret.sb = (m.sb == 0xe) ? v0.se : ret.sb;
  ret.sb = (m.sb == 0xf) ? v0.sf : ret.sb;
  ret.sb = (m.sb == 0x10) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x11) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x12) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x13) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0x14) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0x15) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0x16) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0x17) ? v1.s7 : ret.sb;
  ret.sb = (m.sb == 0x18) ? v1.s8 : ret.sb;
  ret.sb = (m.sb == 0x19) ? v1.s9 : ret.sb;
  ret.sb = (m.sb == 0x1a) ? v1.sa : ret.sb;
  ret.sb = (m.sb == 0x1b) ? v1.sb : ret.sb;
  ret.sb = (m.sb == 0x1c) ? v1.sc : ret.sb;
  ret.sb = (m.sb == 0x1d) ? v1.sd : ret.sb;
  ret.sb = (m.sb == 0x1e) ? v1.se : ret.sb;
  ret.sb = (m.sb == 0x1f) ? v1.sf : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v0.s8 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v0.s9 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v0.sa : ret.sc;
  ret.sc = (m.sc == 0xb) ? v0.sb : ret.sc;
  ret.sc = (m.sc == 0xc) ? v0.sc : ret.sc;
  ret.sc = (m.sc == 0xd) ? v0.sd : ret.sc;
  ret.sc = (m.sc == 0xe) ? v0.se : ret.sc;
  ret.sc = (m.sc == 0xf) ? v0.sf : ret.sc;
  ret.sc = (m.sc == 0x10) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x11) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x12) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x13) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0x14) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0x15) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0x16) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0x17) ? v1.s7 : ret.sc;
  ret.sc = (m.sc == 0x18) ? v1.s8 : ret.sc;
  ret.sc = (m.sc == 0x19) ? v1.s9 : ret.sc;
  ret.sc = (m.sc == 0x1a) ? v1.sa : ret.sc;
  ret.sc = (m.sc == 0x1b) ? v1.sb : ret.sc;
  ret.sc = (m.sc == 0x1c) ? v1.sc : ret.sc;
  ret.sc = (m.sc == 0x1d) ? v1.sd : ret.sc;
  ret.sc = (m.sc == 0x1e) ? v1.se : ret.sc;
  ret.sc = (m.sc == 0x1f) ? v1.sf : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v0.s8 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v0.s9 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v0.sa : ret.sd;
  ret.sd = (m.sd == 0xb) ? v0.sb : ret.sd;
  ret.sd = (m.sd == 0xc) ? v0.sc : ret.sd;
  ret.sd = (m.sd == 0xd) ? v0.sd : ret.sd;
  ret.sd = (m.sd == 0xe) ? v0.se : ret.sd;
  ret.sd = (m.sd == 0xf) ? v0.sf : ret.sd;
  ret.sd = (m.sd == 0x10) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x11) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x12) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x13) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0x14) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0x15) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0x16) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0x17) ? v1.s7 : ret.sd;
  ret.sd = (m.sd == 0x18) ? v1.s8 : ret.sd;
  ret.sd = (m.sd == 0x19) ? v1.s9 : ret.sd;
  ret.sd = (m.sd == 0x1a) ? v1.sa : ret.sd;
  ret.sd = (m.sd == 0x1b) ? v1.sb : ret.sd;
  ret.sd = (m.sd == 0x1c) ? v1.sc : ret.sd;
  ret.sd = (m.sd == 0x1d) ? v1.sd : ret.sd;
  ret.sd = (m.sd == 0x1e) ? v1.se : ret.sd;
  ret.sd = (m.sd == 0x1f) ? v1.sf : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v0.s8 : ret.se;
  ret.se = (m.se == 0x9) ? v0.s9 : ret.se;
  ret.se = (m.se == 0xa) ? v0.sa : ret.se;
  ret.se = (m.se == 0xb) ? v0.sb : ret.se;
  ret.se = (m.se == 0xc) ? v0.sc : ret.se;
  ret.se = (m.se == 0xd) ? v0.sd : ret.se;
  ret.se = (m.se == 0xe) ? v0.se : ret.se;
  ret.se = (m.se == 0xf) ? v0.sf : ret.se;
  ret.se = (m.se == 0x10) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x11) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x12) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x13) ? v1.s3 : ret.se;
  ret.se = (m.se == 0x14) ? v1.s4 : ret.se;
  ret.se = (m.se == 0x15) ? v1.s5 : ret.se;
  ret.se = (m.se == 0x16) ? v1.s6 : ret.se;
  ret.se = (m.se == 0x17) ? v1.s7 : ret.se;
  ret.se = (m.se == 0x18) ? v1.s8 : ret.se;
  ret.se = (m.se == 0x19) ? v1.s9 : ret.se;
  ret.se = (m.se == 0x1a) ? v1.sa : ret.se;
  ret.se = (m.se == 0x1b) ? v1.sb : ret.se;
  ret.se = (m.se == 0x1c) ? v1.sc : ret.se;
  ret.se = (m.se == 0x1d) ? v1.sd : ret.se;
  ret.se = (m.se == 0x1e) ? v1.se : ret.se;
  ret.se = (m.se == 0x1f) ? v1.sf : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v0.s8 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v0.s9 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v0.sa : ret.sf;
  ret.sf = (m.sf == 0xb) ? v0.sb : ret.sf;
  ret.sf = (m.sf == 0xc) ? v0.sc : ret.sf;
  ret.sf = (m.sf == 0xd) ? v0.sd : ret.sf;
  ret.sf = (m.sf == 0xe) ? v0.se : ret.sf;
  ret.sf = (m.sf == 0xf) ? v0.sf : ret.sf;
  ret.sf = (m.sf == 0x10) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x11) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x12) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x13) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0x14) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0x15) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0x16) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0x17) ? v1.s7 : ret.sf;
  ret.sf = (m.sf == 0x18) ? v1.s8 : ret.sf;
  ret.sf = (m.sf == 0x19) ? v1.s9 : ret.sf;
  ret.sf = (m.sf == 0x1a) ? v1.sa : ret.sf;
  ret.sf = (m.sf == 0x1b) ? v1.sb : ret.sf;
  ret.sf = (m.sf == 0x1c) ? v1.sc : ret.sf;
  ret.sf = (m.sf == 0x1d) ? v1.sd : ret.sf;
  ret.sf = (m.sf == 0x1e) ? v1.se : ret.sf;
  ret.sf = (m.sf == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

INLINE int2 OVERLOADABLE shuffle2(int2 v0, int2 v1, uint2 m) {
  int2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE int2 OVERLOADABLE shuffle2(int4 v0, int4 v1, uint2 m) {
  int2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE int2 OVERLOADABLE shuffle2(int8 v0, int8 v1, uint2 m) {
  int2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE int2 OVERLOADABLE shuffle2(int16 v0, int16 v1, uint2 m) {
  int2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE int4 OVERLOADABLE shuffle2(int2 v0, int2 v1, uint4 m) {
  int4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE int4 OVERLOADABLE shuffle2(int4 v0, int4 v1, uint4 m) {
  int4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE int4 OVERLOADABLE shuffle2(int8 v0, int8 v1, uint4 m) {
  int4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE int4 OVERLOADABLE shuffle2(int16 v0, int16 v1, uint4 m) {
  int4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE int8 OVERLOADABLE shuffle2(int2 v0, int2 v1, uint8 m) {
  int8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE int8 OVERLOADABLE shuffle2(int4 v0, int4 v1, uint8 m) {
  int8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE int8 OVERLOADABLE shuffle2(int8 v0, int8 v1, uint8 m) {
  int8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE int8 OVERLOADABLE shuffle2(int16 v0, int16 v1, uint8 m) {
  int8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE int16 OVERLOADABLE shuffle2(int2 v0, int2 v1, uint16 m) {
  int16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v1.s1 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v1.s1 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v1.s1 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v1.s1 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v1.s1 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x3) ? v1.s1 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE int16 OVERLOADABLE shuffle2(int4 v0, int4 v1, uint16 m) {
  int16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v1.s3 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v1.s3 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v1.s3 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v1.s3 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v1.s3 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x5) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x6) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x7) ? v1.s3 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE int16 OVERLOADABLE shuffle2(int8 v0, int8 v1, uint16 m) {
  int16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v1.s7 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0xb) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0xc) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0xd) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0xe) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0xf) ? v1.s7 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0xb) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0xc) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0xd) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0xe) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0xf) ? v1.s7 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0xb) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0xc) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0xd) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0xe) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0xf) ? v1.s7 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0xb) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0xc) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0xd) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0xe) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0xf) ? v1.s7 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x9) ? v1.s1 : ret.se;
  ret.se = (m.se == 0xa) ? v1.s2 : ret.se;
  ret.se = (m.se == 0xb) ? v1.s3 : ret.se;
  ret.se = (m.se == 0xc) ? v1.s4 : ret.se;
  ret.se = (m.se == 0xd) ? v1.s5 : ret.se;
  ret.se = (m.se == 0xe) ? v1.s6 : ret.se;
  ret.se = (m.se == 0xf) ? v1.s7 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0xb) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0xc) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0xd) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0xe) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE int16 OVERLOADABLE shuffle2(int16 v0, int16 v1, uint16 m) {
  int16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v0.sa : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v0.sb : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v0.sc : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v0.sd : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v0.se : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v0.sf : ret.s8;
  ret.s8 = (m.s8 == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = (m.s8 == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = (m.s8 == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = (m.s8 == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = (m.s8 == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = (m.s8 == 0x1e) ? v1.se : ret.s8;
  ret.s8 = (m.s8 == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v0.sa : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v0.sb : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v0.sc : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v0.sd : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v0.se : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v0.sf : ret.s9;
  ret.s9 = (m.s9 == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = (m.s9 == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = (m.s9 == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = (m.s9 == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = (m.s9 == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = (m.s9 == 0x1e) ? v1.se : ret.s9;
  ret.s9 = (m.s9 == 0x1f) ? v1.sf : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v0.s8 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v0.s9 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v0.sa : ret.sa;
  ret.sa = (m.sa == 0xb) ? v0.sb : ret.sa;
  ret.sa = (m.sa == 0xc) ? v0.sc : ret.sa;
  ret.sa = (m.sa == 0xd) ? v0.sd : ret.sa;
  ret.sa = (m.sa == 0xe) ? v0.se : ret.sa;
  ret.sa = (m.sa == 0xf) ? v0.sf : ret.sa;
  ret.sa = (m.sa == 0x10) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x11) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x12) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x13) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0x14) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0x15) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0x16) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0x17) ? v1.s7 : ret.sa;
  ret.sa = (m.sa == 0x18) ? v1.s8 : ret.sa;
  ret.sa = (m.sa == 0x19) ? v1.s9 : ret.sa;
  ret.sa = (m.sa == 0x1a) ? v1.sa : ret.sa;
  ret.sa = (m.sa == 0x1b) ? v1.sb : ret.sa;
  ret.sa = (m.sa == 0x1c) ? v1.sc : ret.sa;
  ret.sa = (m.sa == 0x1d) ? v1.sd : ret.sa;
  ret.sa = (m.sa == 0x1e) ? v1.se : ret.sa;
  ret.sa = (m.sa == 0x1f) ? v1.sf : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v0.s8 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v0.s9 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v0.sa : ret.sb;
  ret.sb = (m.sb == 0xb) ? v0.sb : ret.sb;
  ret.sb = (m.sb == 0xc) ? v0.sc : ret.sb;
  ret.sb = (m.sb == 0xd) ? v0.sd : ret.sb;
  ret.sb = (m.sb == 0xe) ? v0.se : ret.sb;
  ret.sb = (m.sb == 0xf) ? v0.sf : ret.sb;
  ret.sb = (m.sb == 0x10) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x11) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x12) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x13) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0x14) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0x15) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0x16) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0x17) ? v1.s7 : ret.sb;
  ret.sb = (m.sb == 0x18) ? v1.s8 : ret.sb;
  ret.sb = (m.sb == 0x19) ? v1.s9 : ret.sb;
  ret.sb = (m.sb == 0x1a) ? v1.sa : ret.sb;
  ret.sb = (m.sb == 0x1b) ? v1.sb : ret.sb;
  ret.sb = (m.sb == 0x1c) ? v1.sc : ret.sb;
  ret.sb = (m.sb == 0x1d) ? v1.sd : ret.sb;
  ret.sb = (m.sb == 0x1e) ? v1.se : ret.sb;
  ret.sb = (m.sb == 0x1f) ? v1.sf : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v0.s8 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v0.s9 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v0.sa : ret.sc;
  ret.sc = (m.sc == 0xb) ? v0.sb : ret.sc;
  ret.sc = (m.sc == 0xc) ? v0.sc : ret.sc;
  ret.sc = (m.sc == 0xd) ? v0.sd : ret.sc;
  ret.sc = (m.sc == 0xe) ? v0.se : ret.sc;
  ret.sc = (m.sc == 0xf) ? v0.sf : ret.sc;
  ret.sc = (m.sc == 0x10) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x11) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x12) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x13) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0x14) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0x15) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0x16) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0x17) ? v1.s7 : ret.sc;
  ret.sc = (m.sc == 0x18) ? v1.s8 : ret.sc;
  ret.sc = (m.sc == 0x19) ? v1.s9 : ret.sc;
  ret.sc = (m.sc == 0x1a) ? v1.sa : ret.sc;
  ret.sc = (m.sc == 0x1b) ? v1.sb : ret.sc;
  ret.sc = (m.sc == 0x1c) ? v1.sc : ret.sc;
  ret.sc = (m.sc == 0x1d) ? v1.sd : ret.sc;
  ret.sc = (m.sc == 0x1e) ? v1.se : ret.sc;
  ret.sc = (m.sc == 0x1f) ? v1.sf : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v0.s8 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v0.s9 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v0.sa : ret.sd;
  ret.sd = (m.sd == 0xb) ? v0.sb : ret.sd;
  ret.sd = (m.sd == 0xc) ? v0.sc : ret.sd;
  ret.sd = (m.sd == 0xd) ? v0.sd : ret.sd;
  ret.sd = (m.sd == 0xe) ? v0.se : ret.sd;
  ret.sd = (m.sd == 0xf) ? v0.sf : ret.sd;
  ret.sd = (m.sd == 0x10) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x11) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x12) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x13) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0x14) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0x15) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0x16) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0x17) ? v1.s7 : ret.sd;
  ret.sd = (m.sd == 0x18) ? v1.s8 : ret.sd;
  ret.sd = (m.sd == 0x19) ? v1.s9 : ret.sd;
  ret.sd = (m.sd == 0x1a) ? v1.sa : ret.sd;
  ret.sd = (m.sd == 0x1b) ? v1.sb : ret.sd;
  ret.sd = (m.sd == 0x1c) ? v1.sc : ret.sd;
  ret.sd = (m.sd == 0x1d) ? v1.sd : ret.sd;
  ret.sd = (m.sd == 0x1e) ? v1.se : ret.sd;
  ret.sd = (m.sd == 0x1f) ? v1.sf : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v0.s8 : ret.se;
  ret.se = (m.se == 0x9) ? v0.s9 : ret.se;
  ret.se = (m.se == 0xa) ? v0.sa : ret.se;
  ret.se = (m.se == 0xb) ? v0.sb : ret.se;
  ret.se = (m.se == 0xc) ? v0.sc : ret.se;
  ret.se = (m.se == 0xd) ? v0.sd : ret.se;
  ret.se = (m.se == 0xe) ? v0.se : ret.se;
  ret.se = (m.se == 0xf) ? v0.sf : ret.se;
  ret.se = (m.se == 0x10) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x11) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x12) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x13) ? v1.s3 : ret.se;
  ret.se = (m.se == 0x14) ? v1.s4 : ret.se;
  ret.se = (m.se == 0x15) ? v1.s5 : ret.se;
  ret.se = (m.se == 0x16) ? v1.s6 : ret.se;
  ret.se = (m.se == 0x17) ? v1.s7 : ret.se;
  ret.se = (m.se == 0x18) ? v1.s8 : ret.se;
  ret.se = (m.se == 0x19) ? v1.s9 : ret.se;
  ret.se = (m.se == 0x1a) ? v1.sa : ret.se;
  ret.se = (m.se == 0x1b) ? v1.sb : ret.se;
  ret.se = (m.se == 0x1c) ? v1.sc : ret.se;
  ret.se = (m.se == 0x1d) ? v1.sd : ret.se;
  ret.se = (m.se == 0x1e) ? v1.se : ret.se;
  ret.se = (m.se == 0x1f) ? v1.sf : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v0.s8 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v0.s9 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v0.sa : ret.sf;
  ret.sf = (m.sf == 0xb) ? v0.sb : ret.sf;
  ret.sf = (m.sf == 0xc) ? v0.sc : ret.sf;
  ret.sf = (m.sf == 0xd) ? v0.sd : ret.sf;
  ret.sf = (m.sf == 0xe) ? v0.se : ret.sf;
  ret.sf = (m.sf == 0xf) ? v0.sf : ret.sf;
  ret.sf = (m.sf == 0x10) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x11) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x12) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x13) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0x14) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0x15) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0x16) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0x17) ? v1.s7 : ret.sf;
  ret.sf = (m.sf == 0x18) ? v1.s8 : ret.sf;
  ret.sf = (m.sf == 0x19) ? v1.s9 : ret.sf;
  ret.sf = (m.sf == 0x1a) ? v1.sa : ret.sf;
  ret.sf = (m.sf == 0x1b) ? v1.sb : ret.sf;
  ret.sf = (m.sf == 0x1c) ? v1.sc : ret.sf;
  ret.sf = (m.sf == 0x1d) ? v1.sd : ret.sf;
  ret.sf = (m.sf == 0x1e) ? v1.se : ret.sf;
  ret.sf = (m.sf == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

INLINE uint2 OVERLOADABLE shuffle2(uint2 v0, uint2 v1, uint2 m) {
  uint2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE uint2 OVERLOADABLE shuffle2(uint4 v0, uint4 v1, uint2 m) {
  uint2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE uint2 OVERLOADABLE shuffle2(uint8 v0, uint8 v1, uint2 m) {
  uint2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE uint2 OVERLOADABLE shuffle2(uint16 v0, uint16 v1, uint2 m) {
  uint2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE uint4 OVERLOADABLE shuffle2(uint2 v0, uint2 v1, uint4 m) {
  uint4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE uint4 OVERLOADABLE shuffle2(uint4 v0, uint4 v1, uint4 m) {
  uint4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE uint4 OVERLOADABLE shuffle2(uint8 v0, uint8 v1, uint4 m) {
  uint4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE uint4 OVERLOADABLE shuffle2(uint16 v0, uint16 v1, uint4 m) {
  uint4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE uint8 OVERLOADABLE shuffle2(uint2 v0, uint2 v1, uint8 m) {
  uint8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE uint8 OVERLOADABLE shuffle2(uint4 v0, uint4 v1, uint8 m) {
  uint8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE uint8 OVERLOADABLE shuffle2(uint8 v0, uint8 v1, uint8 m) {
  uint8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE uint8 OVERLOADABLE shuffle2(uint16 v0, uint16 v1, uint8 m) {
  uint8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE uint16 OVERLOADABLE shuffle2(uint2 v0, uint2 v1, uint16 m) {
  uint16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v1.s1 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v1.s1 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v1.s1 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v1.s1 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v1.s1 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x3) ? v1.s1 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE uint16 OVERLOADABLE shuffle2(uint4 v0, uint4 v1, uint16 m) {
  uint16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v1.s3 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v1.s3 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v1.s3 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v1.s3 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v1.s3 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x5) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x6) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x7) ? v1.s3 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE uint16 OVERLOADABLE shuffle2(uint8 v0, uint8 v1, uint16 m) {
  uint16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v1.s7 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0xb) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0xc) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0xd) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0xe) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0xf) ? v1.s7 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0xb) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0xc) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0xd) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0xe) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0xf) ? v1.s7 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0xb) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0xc) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0xd) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0xe) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0xf) ? v1.s7 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0xb) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0xc) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0xd) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0xe) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0xf) ? v1.s7 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x9) ? v1.s1 : ret.se;
  ret.se = (m.se == 0xa) ? v1.s2 : ret.se;
  ret.se = (m.se == 0xb) ? v1.s3 : ret.se;
  ret.se = (m.se == 0xc) ? v1.s4 : ret.se;
  ret.se = (m.se == 0xd) ? v1.s5 : ret.se;
  ret.se = (m.se == 0xe) ? v1.s6 : ret.se;
  ret.se = (m.se == 0xf) ? v1.s7 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0xb) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0xc) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0xd) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0xe) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE uint16 OVERLOADABLE shuffle2(uint16 v0, uint16 v1, uint16 m) {
  uint16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v0.sa : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v0.sb : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v0.sc : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v0.sd : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v0.se : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v0.sf : ret.s8;
  ret.s8 = (m.s8 == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = (m.s8 == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = (m.s8 == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = (m.s8 == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = (m.s8 == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = (m.s8 == 0x1e) ? v1.se : ret.s8;
  ret.s8 = (m.s8 == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v0.sa : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v0.sb : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v0.sc : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v0.sd : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v0.se : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v0.sf : ret.s9;
  ret.s9 = (m.s9 == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = (m.s9 == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = (m.s9 == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = (m.s9 == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = (m.s9 == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = (m.s9 == 0x1e) ? v1.se : ret.s9;
  ret.s9 = (m.s9 == 0x1f) ? v1.sf : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v0.s8 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v0.s9 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v0.sa : ret.sa;
  ret.sa = (m.sa == 0xb) ? v0.sb : ret.sa;
  ret.sa = (m.sa == 0xc) ? v0.sc : ret.sa;
  ret.sa = (m.sa == 0xd) ? v0.sd : ret.sa;
  ret.sa = (m.sa == 0xe) ? v0.se : ret.sa;
  ret.sa = (m.sa == 0xf) ? v0.sf : ret.sa;
  ret.sa = (m.sa == 0x10) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x11) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x12) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x13) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0x14) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0x15) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0x16) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0x17) ? v1.s7 : ret.sa;
  ret.sa = (m.sa == 0x18) ? v1.s8 : ret.sa;
  ret.sa = (m.sa == 0x19) ? v1.s9 : ret.sa;
  ret.sa = (m.sa == 0x1a) ? v1.sa : ret.sa;
  ret.sa = (m.sa == 0x1b) ? v1.sb : ret.sa;
  ret.sa = (m.sa == 0x1c) ? v1.sc : ret.sa;
  ret.sa = (m.sa == 0x1d) ? v1.sd : ret.sa;
  ret.sa = (m.sa == 0x1e) ? v1.se : ret.sa;
  ret.sa = (m.sa == 0x1f) ? v1.sf : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v0.s8 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v0.s9 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v0.sa : ret.sb;
  ret.sb = (m.sb == 0xb) ? v0.sb : ret.sb;
  ret.sb = (m.sb == 0xc) ? v0.sc : ret.sb;
  ret.sb = (m.sb == 0xd) ? v0.sd : ret.sb;
  ret.sb = (m.sb == 0xe) ? v0.se : ret.sb;
  ret.sb = (m.sb == 0xf) ? v0.sf : ret.sb;
  ret.sb = (m.sb == 0x10) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x11) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x12) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x13) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0x14) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0x15) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0x16) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0x17) ? v1.s7 : ret.sb;
  ret.sb = (m.sb == 0x18) ? v1.s8 : ret.sb;
  ret.sb = (m.sb == 0x19) ? v1.s9 : ret.sb;
  ret.sb = (m.sb == 0x1a) ? v1.sa : ret.sb;
  ret.sb = (m.sb == 0x1b) ? v1.sb : ret.sb;
  ret.sb = (m.sb == 0x1c) ? v1.sc : ret.sb;
  ret.sb = (m.sb == 0x1d) ? v1.sd : ret.sb;
  ret.sb = (m.sb == 0x1e) ? v1.se : ret.sb;
  ret.sb = (m.sb == 0x1f) ? v1.sf : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v0.s8 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v0.s9 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v0.sa : ret.sc;
  ret.sc = (m.sc == 0xb) ? v0.sb : ret.sc;
  ret.sc = (m.sc == 0xc) ? v0.sc : ret.sc;
  ret.sc = (m.sc == 0xd) ? v0.sd : ret.sc;
  ret.sc = (m.sc == 0xe) ? v0.se : ret.sc;
  ret.sc = (m.sc == 0xf) ? v0.sf : ret.sc;
  ret.sc = (m.sc == 0x10) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x11) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x12) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x13) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0x14) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0x15) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0x16) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0x17) ? v1.s7 : ret.sc;
  ret.sc = (m.sc == 0x18) ? v1.s8 : ret.sc;
  ret.sc = (m.sc == 0x19) ? v1.s9 : ret.sc;
  ret.sc = (m.sc == 0x1a) ? v1.sa : ret.sc;
  ret.sc = (m.sc == 0x1b) ? v1.sb : ret.sc;
  ret.sc = (m.sc == 0x1c) ? v1.sc : ret.sc;
  ret.sc = (m.sc == 0x1d) ? v1.sd : ret.sc;
  ret.sc = (m.sc == 0x1e) ? v1.se : ret.sc;
  ret.sc = (m.sc == 0x1f) ? v1.sf : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v0.s8 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v0.s9 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v0.sa : ret.sd;
  ret.sd = (m.sd == 0xb) ? v0.sb : ret.sd;
  ret.sd = (m.sd == 0xc) ? v0.sc : ret.sd;
  ret.sd = (m.sd == 0xd) ? v0.sd : ret.sd;
  ret.sd = (m.sd == 0xe) ? v0.se : ret.sd;
  ret.sd = (m.sd == 0xf) ? v0.sf : ret.sd;
  ret.sd = (m.sd == 0x10) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x11) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x12) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x13) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0x14) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0x15) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0x16) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0x17) ? v1.s7 : ret.sd;
  ret.sd = (m.sd == 0x18) ? v1.s8 : ret.sd;
  ret.sd = (m.sd == 0x19) ? v1.s9 : ret.sd;
  ret.sd = (m.sd == 0x1a) ? v1.sa : ret.sd;
  ret.sd = (m.sd == 0x1b) ? v1.sb : ret.sd;
  ret.sd = (m.sd == 0x1c) ? v1.sc : ret.sd;
  ret.sd = (m.sd == 0x1d) ? v1.sd : ret.sd;
  ret.sd = (m.sd == 0x1e) ? v1.se : ret.sd;
  ret.sd = (m.sd == 0x1f) ? v1.sf : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v0.s8 : ret.se;
  ret.se = (m.se == 0x9) ? v0.s9 : ret.se;
  ret.se = (m.se == 0xa) ? v0.sa : ret.se;
  ret.se = (m.se == 0xb) ? v0.sb : ret.se;
  ret.se = (m.se == 0xc) ? v0.sc : ret.se;
  ret.se = (m.se == 0xd) ? v0.sd : ret.se;
  ret.se = (m.se == 0xe) ? v0.se : ret.se;
  ret.se = (m.se == 0xf) ? v0.sf : ret.se;
  ret.se = (m.se == 0x10) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x11) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x12) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x13) ? v1.s3 : ret.se;
  ret.se = (m.se == 0x14) ? v1.s4 : ret.se;
  ret.se = (m.se == 0x15) ? v1.s5 : ret.se;
  ret.se = (m.se == 0x16) ? v1.s6 : ret.se;
  ret.se = (m.se == 0x17) ? v1.s7 : ret.se;
  ret.se = (m.se == 0x18) ? v1.s8 : ret.se;
  ret.se = (m.se == 0x19) ? v1.s9 : ret.se;
  ret.se = (m.se == 0x1a) ? v1.sa : ret.se;
  ret.se = (m.se == 0x1b) ? v1.sb : ret.se;
  ret.se = (m.se == 0x1c) ? v1.sc : ret.se;
  ret.se = (m.se == 0x1d) ? v1.sd : ret.se;
  ret.se = (m.se == 0x1e) ? v1.se : ret.se;
  ret.se = (m.se == 0x1f) ? v1.sf : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v0.s8 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v0.s9 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v0.sa : ret.sf;
  ret.sf = (m.sf == 0xb) ? v0.sb : ret.sf;
  ret.sf = (m.sf == 0xc) ? v0.sc : ret.sf;
  ret.sf = (m.sf == 0xd) ? v0.sd : ret.sf;
  ret.sf = (m.sf == 0xe) ? v0.se : ret.sf;
  ret.sf = (m.sf == 0xf) ? v0.sf : ret.sf;
  ret.sf = (m.sf == 0x10) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x11) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x12) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x13) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0x14) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0x15) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0x16) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0x17) ? v1.s7 : ret.sf;
  ret.sf = (m.sf == 0x18) ? v1.s8 : ret.sf;
  ret.sf = (m.sf == 0x19) ? v1.s9 : ret.sf;
  ret.sf = (m.sf == 0x1a) ? v1.sa : ret.sf;
  ret.sf = (m.sf == 0x1b) ? v1.sb : ret.sf;
  ret.sf = (m.sf == 0x1c) ? v1.sc : ret.sf;
  ret.sf = (m.sf == 0x1d) ? v1.sd : ret.sf;
  ret.sf = (m.sf == 0x1e) ? v1.se : ret.sf;
  ret.sf = (m.sf == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

INLINE long2 OVERLOADABLE shuffle2(long2 v0, long2 v1, ulong2 m) {
  long2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE long2 OVERLOADABLE shuffle2(long4 v0, long4 v1, ulong2 m) {
  long2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE long2 OVERLOADABLE shuffle2(long8 v0, long8 v1, ulong2 m) {
  long2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE long2 OVERLOADABLE shuffle2(long16 v0, long16 v1, ulong2 m) {
  long2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE long4 OVERLOADABLE shuffle2(long2 v0, long2 v1, ulong4 m) {
  long4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE long4 OVERLOADABLE shuffle2(long4 v0, long4 v1, ulong4 m) {
  long4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE long4 OVERLOADABLE shuffle2(long8 v0, long8 v1, ulong4 m) {
  long4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE long4 OVERLOADABLE shuffle2(long16 v0, long16 v1, ulong4 m) {
  long4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE long8 OVERLOADABLE shuffle2(long2 v0, long2 v1, ulong8 m) {
  long8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE long8 OVERLOADABLE shuffle2(long4 v0, long4 v1, ulong8 m) {
  long8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE long8 OVERLOADABLE shuffle2(long8 v0, long8 v1, ulong8 m) {
  long8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE long8 OVERLOADABLE shuffle2(long16 v0, long16 v1, ulong8 m) {
  long8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE long16 OVERLOADABLE shuffle2(long2 v0, long2 v1, ulong16 m) {
  long16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v1.s1 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v1.s1 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v1.s1 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v1.s1 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v1.s1 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x3) ? v1.s1 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE long16 OVERLOADABLE shuffle2(long4 v0, long4 v1, ulong16 m) {
  long16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v1.s3 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v1.s3 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v1.s3 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v1.s3 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v1.s3 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x5) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x6) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x7) ? v1.s3 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE long16 OVERLOADABLE shuffle2(long8 v0, long8 v1, ulong16 m) {
  long16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v1.s7 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0xb) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0xc) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0xd) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0xe) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0xf) ? v1.s7 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0xb) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0xc) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0xd) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0xe) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0xf) ? v1.s7 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0xb) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0xc) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0xd) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0xe) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0xf) ? v1.s7 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0xb) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0xc) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0xd) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0xe) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0xf) ? v1.s7 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x9) ? v1.s1 : ret.se;
  ret.se = (m.se == 0xa) ? v1.s2 : ret.se;
  ret.se = (m.se == 0xb) ? v1.s3 : ret.se;
  ret.se = (m.se == 0xc) ? v1.s4 : ret.se;
  ret.se = (m.se == 0xd) ? v1.s5 : ret.se;
  ret.se = (m.se == 0xe) ? v1.s6 : ret.se;
  ret.se = (m.se == 0xf) ? v1.s7 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0xb) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0xc) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0xd) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0xe) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE long16 OVERLOADABLE shuffle2(long16 v0, long16 v1, ulong16 m) {
  long16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v0.sa : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v0.sb : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v0.sc : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v0.sd : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v0.se : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v0.sf : ret.s8;
  ret.s8 = (m.s8 == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = (m.s8 == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = (m.s8 == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = (m.s8 == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = (m.s8 == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = (m.s8 == 0x1e) ? v1.se : ret.s8;
  ret.s8 = (m.s8 == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v0.sa : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v0.sb : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v0.sc : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v0.sd : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v0.se : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v0.sf : ret.s9;
  ret.s9 = (m.s9 == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = (m.s9 == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = (m.s9 == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = (m.s9 == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = (m.s9 == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = (m.s9 == 0x1e) ? v1.se : ret.s9;
  ret.s9 = (m.s9 == 0x1f) ? v1.sf : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v0.s8 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v0.s9 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v0.sa : ret.sa;
  ret.sa = (m.sa == 0xb) ? v0.sb : ret.sa;
  ret.sa = (m.sa == 0xc) ? v0.sc : ret.sa;
  ret.sa = (m.sa == 0xd) ? v0.sd : ret.sa;
  ret.sa = (m.sa == 0xe) ? v0.se : ret.sa;
  ret.sa = (m.sa == 0xf) ? v0.sf : ret.sa;
  ret.sa = (m.sa == 0x10) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x11) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x12) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x13) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0x14) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0x15) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0x16) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0x17) ? v1.s7 : ret.sa;
  ret.sa = (m.sa == 0x18) ? v1.s8 : ret.sa;
  ret.sa = (m.sa == 0x19) ? v1.s9 : ret.sa;
  ret.sa = (m.sa == 0x1a) ? v1.sa : ret.sa;
  ret.sa = (m.sa == 0x1b) ? v1.sb : ret.sa;
  ret.sa = (m.sa == 0x1c) ? v1.sc : ret.sa;
  ret.sa = (m.sa == 0x1d) ? v1.sd : ret.sa;
  ret.sa = (m.sa == 0x1e) ? v1.se : ret.sa;
  ret.sa = (m.sa == 0x1f) ? v1.sf : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v0.s8 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v0.s9 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v0.sa : ret.sb;
  ret.sb = (m.sb == 0xb) ? v0.sb : ret.sb;
  ret.sb = (m.sb == 0xc) ? v0.sc : ret.sb;
  ret.sb = (m.sb == 0xd) ? v0.sd : ret.sb;
  ret.sb = (m.sb == 0xe) ? v0.se : ret.sb;
  ret.sb = (m.sb == 0xf) ? v0.sf : ret.sb;
  ret.sb = (m.sb == 0x10) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x11) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x12) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x13) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0x14) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0x15) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0x16) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0x17) ? v1.s7 : ret.sb;
  ret.sb = (m.sb == 0x18) ? v1.s8 : ret.sb;
  ret.sb = (m.sb == 0x19) ? v1.s9 : ret.sb;
  ret.sb = (m.sb == 0x1a) ? v1.sa : ret.sb;
  ret.sb = (m.sb == 0x1b) ? v1.sb : ret.sb;
  ret.sb = (m.sb == 0x1c) ? v1.sc : ret.sb;
  ret.sb = (m.sb == 0x1d) ? v1.sd : ret.sb;
  ret.sb = (m.sb == 0x1e) ? v1.se : ret.sb;
  ret.sb = (m.sb == 0x1f) ? v1.sf : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v0.s8 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v0.s9 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v0.sa : ret.sc;
  ret.sc = (m.sc == 0xb) ? v0.sb : ret.sc;
  ret.sc = (m.sc == 0xc) ? v0.sc : ret.sc;
  ret.sc = (m.sc == 0xd) ? v0.sd : ret.sc;
  ret.sc = (m.sc == 0xe) ? v0.se : ret.sc;
  ret.sc = (m.sc == 0xf) ? v0.sf : ret.sc;
  ret.sc = (m.sc == 0x10) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x11) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x12) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x13) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0x14) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0x15) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0x16) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0x17) ? v1.s7 : ret.sc;
  ret.sc = (m.sc == 0x18) ? v1.s8 : ret.sc;
  ret.sc = (m.sc == 0x19) ? v1.s9 : ret.sc;
  ret.sc = (m.sc == 0x1a) ? v1.sa : ret.sc;
  ret.sc = (m.sc == 0x1b) ? v1.sb : ret.sc;
  ret.sc = (m.sc == 0x1c) ? v1.sc : ret.sc;
  ret.sc = (m.sc == 0x1d) ? v1.sd : ret.sc;
  ret.sc = (m.sc == 0x1e) ? v1.se : ret.sc;
  ret.sc = (m.sc == 0x1f) ? v1.sf : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v0.s8 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v0.s9 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v0.sa : ret.sd;
  ret.sd = (m.sd == 0xb) ? v0.sb : ret.sd;
  ret.sd = (m.sd == 0xc) ? v0.sc : ret.sd;
  ret.sd = (m.sd == 0xd) ? v0.sd : ret.sd;
  ret.sd = (m.sd == 0xe) ? v0.se : ret.sd;
  ret.sd = (m.sd == 0xf) ? v0.sf : ret.sd;
  ret.sd = (m.sd == 0x10) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x11) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x12) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x13) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0x14) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0x15) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0x16) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0x17) ? v1.s7 : ret.sd;
  ret.sd = (m.sd == 0x18) ? v1.s8 : ret.sd;
  ret.sd = (m.sd == 0x19) ? v1.s9 : ret.sd;
  ret.sd = (m.sd == 0x1a) ? v1.sa : ret.sd;
  ret.sd = (m.sd == 0x1b) ? v1.sb : ret.sd;
  ret.sd = (m.sd == 0x1c) ? v1.sc : ret.sd;
  ret.sd = (m.sd == 0x1d) ? v1.sd : ret.sd;
  ret.sd = (m.sd == 0x1e) ? v1.se : ret.sd;
  ret.sd = (m.sd == 0x1f) ? v1.sf : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v0.s8 : ret.se;
  ret.se = (m.se == 0x9) ? v0.s9 : ret.se;
  ret.se = (m.se == 0xa) ? v0.sa : ret.se;
  ret.se = (m.se == 0xb) ? v0.sb : ret.se;
  ret.se = (m.se == 0xc) ? v0.sc : ret.se;
  ret.se = (m.se == 0xd) ? v0.sd : ret.se;
  ret.se = (m.se == 0xe) ? v0.se : ret.se;
  ret.se = (m.se == 0xf) ? v0.sf : ret.se;
  ret.se = (m.se == 0x10) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x11) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x12) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x13) ? v1.s3 : ret.se;
  ret.se = (m.se == 0x14) ? v1.s4 : ret.se;
  ret.se = (m.se == 0x15) ? v1.s5 : ret.se;
  ret.se = (m.se == 0x16) ? v1.s6 : ret.se;
  ret.se = (m.se == 0x17) ? v1.s7 : ret.se;
  ret.se = (m.se == 0x18) ? v1.s8 : ret.se;
  ret.se = (m.se == 0x19) ? v1.s9 : ret.se;
  ret.se = (m.se == 0x1a) ? v1.sa : ret.se;
  ret.se = (m.se == 0x1b) ? v1.sb : ret.se;
  ret.se = (m.se == 0x1c) ? v1.sc : ret.se;
  ret.se = (m.se == 0x1d) ? v1.sd : ret.se;
  ret.se = (m.se == 0x1e) ? v1.se : ret.se;
  ret.se = (m.se == 0x1f) ? v1.sf : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v0.s8 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v0.s9 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v0.sa : ret.sf;
  ret.sf = (m.sf == 0xb) ? v0.sb : ret.sf;
  ret.sf = (m.sf == 0xc) ? v0.sc : ret.sf;
  ret.sf = (m.sf == 0xd) ? v0.sd : ret.sf;
  ret.sf = (m.sf == 0xe) ? v0.se : ret.sf;
  ret.sf = (m.sf == 0xf) ? v0.sf : ret.sf;
  ret.sf = (m.sf == 0x10) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x11) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x12) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x13) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0x14) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0x15) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0x16) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0x17) ? v1.s7 : ret.sf;
  ret.sf = (m.sf == 0x18) ? v1.s8 : ret.sf;
  ret.sf = (m.sf == 0x19) ? v1.s9 : ret.sf;
  ret.sf = (m.sf == 0x1a) ? v1.sa : ret.sf;
  ret.sf = (m.sf == 0x1b) ? v1.sb : ret.sf;
  ret.sf = (m.sf == 0x1c) ? v1.sc : ret.sf;
  ret.sf = (m.sf == 0x1d) ? v1.sd : ret.sf;
  ret.sf = (m.sf == 0x1e) ? v1.se : ret.sf;
  ret.sf = (m.sf == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

INLINE ulong2 OVERLOADABLE shuffle2(ulong2 v0, ulong2 v1, ulong2 m) {
  ulong2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE ulong2 OVERLOADABLE shuffle2(ulong4 v0, ulong4 v1, ulong2 m) {
  ulong2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE ulong2 OVERLOADABLE shuffle2(ulong8 v0, ulong8 v1, ulong2 m) {
  ulong2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE ulong2 OVERLOADABLE shuffle2(ulong16 v0, ulong16 v1, ulong2 m) {
  ulong2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE ulong4 OVERLOADABLE shuffle2(ulong2 v0, ulong2 v1, ulong4 m) {
  ulong4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE ulong4 OVERLOADABLE shuffle2(ulong4 v0, ulong4 v1, ulong4 m) {
  ulong4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE ulong4 OVERLOADABLE shuffle2(ulong8 v0, ulong8 v1, ulong4 m) {
  ulong4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE ulong4 OVERLOADABLE shuffle2(ulong16 v0, ulong16 v1, ulong4 m) {
  ulong4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE ulong8 OVERLOADABLE shuffle2(ulong2 v0, ulong2 v1, ulong8 m) {
  ulong8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE ulong8 OVERLOADABLE shuffle2(ulong4 v0, ulong4 v1, ulong8 m) {
  ulong8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE ulong8 OVERLOADABLE shuffle2(ulong8 v0, ulong8 v1, ulong8 m) {
  ulong8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE ulong8 OVERLOADABLE shuffle2(ulong16 v0, ulong16 v1, ulong8 m) {
  ulong8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE ulong16 OVERLOADABLE shuffle2(ulong2 v0, ulong2 v1, ulong16 m) {
  ulong16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v1.s1 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v1.s1 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v1.s1 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v1.s1 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v1.s1 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x3) ? v1.s1 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE ulong16 OVERLOADABLE shuffle2(ulong4 v0, ulong4 v1, ulong16 m) {
  ulong16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v1.s3 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v1.s3 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v1.s3 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v1.s3 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v1.s3 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x5) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x6) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x7) ? v1.s3 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE ulong16 OVERLOADABLE shuffle2(ulong8 v0, ulong8 v1, ulong16 m) {
  ulong16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v1.s7 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0xb) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0xc) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0xd) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0xe) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0xf) ? v1.s7 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0xb) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0xc) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0xd) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0xe) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0xf) ? v1.s7 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0xb) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0xc) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0xd) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0xe) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0xf) ? v1.s7 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0xb) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0xc) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0xd) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0xe) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0xf) ? v1.s7 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x9) ? v1.s1 : ret.se;
  ret.se = (m.se == 0xa) ? v1.s2 : ret.se;
  ret.se = (m.se == 0xb) ? v1.s3 : ret.se;
  ret.se = (m.se == 0xc) ? v1.s4 : ret.se;
  ret.se = (m.se == 0xd) ? v1.s5 : ret.se;
  ret.se = (m.se == 0xe) ? v1.s6 : ret.se;
  ret.se = (m.se == 0xf) ? v1.s7 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0xb) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0xc) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0xd) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0xe) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE ulong16 OVERLOADABLE shuffle2(ulong16 v0, ulong16 v1, ulong16 m) {
  ulong16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v0.sa : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v0.sb : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v0.sc : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v0.sd : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v0.se : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v0.sf : ret.s8;
  ret.s8 = (m.s8 == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = (m.s8 == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = (m.s8 == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = (m.s8 == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = (m.s8 == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = (m.s8 == 0x1e) ? v1.se : ret.s8;
  ret.s8 = (m.s8 == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v0.sa : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v0.sb : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v0.sc : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v0.sd : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v0.se : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v0.sf : ret.s9;
  ret.s9 = (m.s9 == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = (m.s9 == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = (m.s9 == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = (m.s9 == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = (m.s9 == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = (m.s9 == 0x1e) ? v1.se : ret.s9;
  ret.s9 = (m.s9 == 0x1f) ? v1.sf : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v0.s8 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v0.s9 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v0.sa : ret.sa;
  ret.sa = (m.sa == 0xb) ? v0.sb : ret.sa;
  ret.sa = (m.sa == 0xc) ? v0.sc : ret.sa;
  ret.sa = (m.sa == 0xd) ? v0.sd : ret.sa;
  ret.sa = (m.sa == 0xe) ? v0.se : ret.sa;
  ret.sa = (m.sa == 0xf) ? v0.sf : ret.sa;
  ret.sa = (m.sa == 0x10) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x11) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x12) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x13) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0x14) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0x15) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0x16) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0x17) ? v1.s7 : ret.sa;
  ret.sa = (m.sa == 0x18) ? v1.s8 : ret.sa;
  ret.sa = (m.sa == 0x19) ? v1.s9 : ret.sa;
  ret.sa = (m.sa == 0x1a) ? v1.sa : ret.sa;
  ret.sa = (m.sa == 0x1b) ? v1.sb : ret.sa;
  ret.sa = (m.sa == 0x1c) ? v1.sc : ret.sa;
  ret.sa = (m.sa == 0x1d) ? v1.sd : ret.sa;
  ret.sa = (m.sa == 0x1e) ? v1.se : ret.sa;
  ret.sa = (m.sa == 0x1f) ? v1.sf : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v0.s8 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v0.s9 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v0.sa : ret.sb;
  ret.sb = (m.sb == 0xb) ? v0.sb : ret.sb;
  ret.sb = (m.sb == 0xc) ? v0.sc : ret.sb;
  ret.sb = (m.sb == 0xd) ? v0.sd : ret.sb;
  ret.sb = (m.sb == 0xe) ? v0.se : ret.sb;
  ret.sb = (m.sb == 0xf) ? v0.sf : ret.sb;
  ret.sb = (m.sb == 0x10) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x11) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x12) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x13) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0x14) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0x15) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0x16) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0x17) ? v1.s7 : ret.sb;
  ret.sb = (m.sb == 0x18) ? v1.s8 : ret.sb;
  ret.sb = (m.sb == 0x19) ? v1.s9 : ret.sb;
  ret.sb = (m.sb == 0x1a) ? v1.sa : ret.sb;
  ret.sb = (m.sb == 0x1b) ? v1.sb : ret.sb;
  ret.sb = (m.sb == 0x1c) ? v1.sc : ret.sb;
  ret.sb = (m.sb == 0x1d) ? v1.sd : ret.sb;
  ret.sb = (m.sb == 0x1e) ? v1.se : ret.sb;
  ret.sb = (m.sb == 0x1f) ? v1.sf : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v0.s8 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v0.s9 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v0.sa : ret.sc;
  ret.sc = (m.sc == 0xb) ? v0.sb : ret.sc;
  ret.sc = (m.sc == 0xc) ? v0.sc : ret.sc;
  ret.sc = (m.sc == 0xd) ? v0.sd : ret.sc;
  ret.sc = (m.sc == 0xe) ? v0.se : ret.sc;
  ret.sc = (m.sc == 0xf) ? v0.sf : ret.sc;
  ret.sc = (m.sc == 0x10) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x11) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x12) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x13) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0x14) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0x15) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0x16) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0x17) ? v1.s7 : ret.sc;
  ret.sc = (m.sc == 0x18) ? v1.s8 : ret.sc;
  ret.sc = (m.sc == 0x19) ? v1.s9 : ret.sc;
  ret.sc = (m.sc == 0x1a) ? v1.sa : ret.sc;
  ret.sc = (m.sc == 0x1b) ? v1.sb : ret.sc;
  ret.sc = (m.sc == 0x1c) ? v1.sc : ret.sc;
  ret.sc = (m.sc == 0x1d) ? v1.sd : ret.sc;
  ret.sc = (m.sc == 0x1e) ? v1.se : ret.sc;
  ret.sc = (m.sc == 0x1f) ? v1.sf : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v0.s8 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v0.s9 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v0.sa : ret.sd;
  ret.sd = (m.sd == 0xb) ? v0.sb : ret.sd;
  ret.sd = (m.sd == 0xc) ? v0.sc : ret.sd;
  ret.sd = (m.sd == 0xd) ? v0.sd : ret.sd;
  ret.sd = (m.sd == 0xe) ? v0.se : ret.sd;
  ret.sd = (m.sd == 0xf) ? v0.sf : ret.sd;
  ret.sd = (m.sd == 0x10) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x11) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x12) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x13) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0x14) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0x15) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0x16) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0x17) ? v1.s7 : ret.sd;
  ret.sd = (m.sd == 0x18) ? v1.s8 : ret.sd;
  ret.sd = (m.sd == 0x19) ? v1.s9 : ret.sd;
  ret.sd = (m.sd == 0x1a) ? v1.sa : ret.sd;
  ret.sd = (m.sd == 0x1b) ? v1.sb : ret.sd;
  ret.sd = (m.sd == 0x1c) ? v1.sc : ret.sd;
  ret.sd = (m.sd == 0x1d) ? v1.sd : ret.sd;
  ret.sd = (m.sd == 0x1e) ? v1.se : ret.sd;
  ret.sd = (m.sd == 0x1f) ? v1.sf : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v0.s8 : ret.se;
  ret.se = (m.se == 0x9) ? v0.s9 : ret.se;
  ret.se = (m.se == 0xa) ? v0.sa : ret.se;
  ret.se = (m.se == 0xb) ? v0.sb : ret.se;
  ret.se = (m.se == 0xc) ? v0.sc : ret.se;
  ret.se = (m.se == 0xd) ? v0.sd : ret.se;
  ret.se = (m.se == 0xe) ? v0.se : ret.se;
  ret.se = (m.se == 0xf) ? v0.sf : ret.se;
  ret.se = (m.se == 0x10) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x11) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x12) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x13) ? v1.s3 : ret.se;
  ret.se = (m.se == 0x14) ? v1.s4 : ret.se;
  ret.se = (m.se == 0x15) ? v1.s5 : ret.se;
  ret.se = (m.se == 0x16) ? v1.s6 : ret.se;
  ret.se = (m.se == 0x17) ? v1.s7 : ret.se;
  ret.se = (m.se == 0x18) ? v1.s8 : ret.se;
  ret.se = (m.se == 0x19) ? v1.s9 : ret.se;
  ret.se = (m.se == 0x1a) ? v1.sa : ret.se;
  ret.se = (m.se == 0x1b) ? v1.sb : ret.se;
  ret.se = (m.se == 0x1c) ? v1.sc : ret.se;
  ret.se = (m.se == 0x1d) ? v1.sd : ret.se;
  ret.se = (m.se == 0x1e) ? v1.se : ret.se;
  ret.se = (m.se == 0x1f) ? v1.sf : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v0.s8 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v0.s9 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v0.sa : ret.sf;
  ret.sf = (m.sf == 0xb) ? v0.sb : ret.sf;
  ret.sf = (m.sf == 0xc) ? v0.sc : ret.sf;
  ret.sf = (m.sf == 0xd) ? v0.sd : ret.sf;
  ret.sf = (m.sf == 0xe) ? v0.se : ret.sf;
  ret.sf = (m.sf == 0xf) ? v0.sf : ret.sf;
  ret.sf = (m.sf == 0x10) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x11) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x12) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x13) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0x14) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0x15) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0x16) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0x17) ? v1.s7 : ret.sf;
  ret.sf = (m.sf == 0x18) ? v1.s8 : ret.sf;
  ret.sf = (m.sf == 0x19) ? v1.s9 : ret.sf;
  ret.sf = (m.sf == 0x1a) ? v1.sa : ret.sf;
  ret.sf = (m.sf == 0x1b) ? v1.sb : ret.sf;
  ret.sf = (m.sf == 0x1c) ? v1.sc : ret.sf;
  ret.sf = (m.sf == 0x1d) ? v1.sd : ret.sf;
  ret.sf = (m.sf == 0x1e) ? v1.se : ret.sf;
  ret.sf = (m.sf == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

INLINE float2 OVERLOADABLE shuffle2(float2 v0, float2 v1, uint2 m) {
  float2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE float2 OVERLOADABLE shuffle2(float4 v0, float4 v1, uint2 m) {
  float2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE float2 OVERLOADABLE shuffle2(float8 v0, float8 v1, uint2 m) {
  float2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE float2 OVERLOADABLE shuffle2(float16 v0, float16 v1, uint2 m) {
  float2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE float4 OVERLOADABLE shuffle2(float2 v0, float2 v1, uint4 m) {
  float4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE float4 OVERLOADABLE shuffle2(float4 v0, float4 v1, uint4 m) {
  float4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE float4 OVERLOADABLE shuffle2(float8 v0, float8 v1, uint4 m) {
  float4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE float4 OVERLOADABLE shuffle2(float16 v0, float16 v1, uint4 m) {
  float4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE float8 OVERLOADABLE shuffle2(float2 v0, float2 v1, uint8 m) {
  float8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE float8 OVERLOADABLE shuffle2(float4 v0, float4 v1, uint8 m) {
  float8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE float8 OVERLOADABLE shuffle2(float8 v0, float8 v1, uint8 m) {
  float8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE float8 OVERLOADABLE shuffle2(float16 v0, float16 v1, uint8 m) {
  float8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE float16 OVERLOADABLE shuffle2(float2 v0, float2 v1, uint16 m) {
  float16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v1.s1 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v1.s1 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v1.s1 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v1.s1 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v1.s1 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x3) ? v1.s1 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE float16 OVERLOADABLE shuffle2(float4 v0, float4 v1, uint16 m) {
  float16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v1.s3 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v1.s3 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v1.s3 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v1.s3 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v1.s3 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x5) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x6) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x7) ? v1.s3 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE float16 OVERLOADABLE shuffle2(float8 v0, float8 v1, uint16 m) {
  float16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v1.s7 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0xb) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0xc) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0xd) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0xe) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0xf) ? v1.s7 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0xb) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0xc) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0xd) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0xe) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0xf) ? v1.s7 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0xb) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0xc) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0xd) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0xe) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0xf) ? v1.s7 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0xb) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0xc) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0xd) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0xe) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0xf) ? v1.s7 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x9) ? v1.s1 : ret.se;
  ret.se = (m.se == 0xa) ? v1.s2 : ret.se;
  ret.se = (m.se == 0xb) ? v1.s3 : ret.se;
  ret.se = (m.se == 0xc) ? v1.s4 : ret.se;
  ret.se = (m.se == 0xd) ? v1.s5 : ret.se;
  ret.se = (m.se == 0xe) ? v1.s6 : ret.se;
  ret.se = (m.se == 0xf) ? v1.s7 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0xb) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0xc) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0xd) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0xe) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE float16 OVERLOADABLE shuffle2(float16 v0, float16 v1, uint16 m) {
  float16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v0.sa : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v0.sb : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v0.sc : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v0.sd : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v0.se : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v0.sf : ret.s8;
  ret.s8 = (m.s8 == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = (m.s8 == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = (m.s8 == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = (m.s8 == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = (m.s8 == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = (m.s8 == 0x1e) ? v1.se : ret.s8;
  ret.s8 = (m.s8 == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v0.sa : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v0.sb : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v0.sc : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v0.sd : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v0.se : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v0.sf : ret.s9;
  ret.s9 = (m.s9 == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = (m.s9 == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = (m.s9 == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = (m.s9 == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = (m.s9 == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = (m.s9 == 0x1e) ? v1.se : ret.s9;
  ret.s9 = (m.s9 == 0x1f) ? v1.sf : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v0.s8 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v0.s9 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v0.sa : ret.sa;
  ret.sa = (m.sa == 0xb) ? v0.sb : ret.sa;
  ret.sa = (m.sa == 0xc) ? v0.sc : ret.sa;
  ret.sa = (m.sa == 0xd) ? v0.sd : ret.sa;
  ret.sa = (m.sa == 0xe) ? v0.se : ret.sa;
  ret.sa = (m.sa == 0xf) ? v0.sf : ret.sa;
  ret.sa = (m.sa == 0x10) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x11) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x12) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x13) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0x14) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0x15) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0x16) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0x17) ? v1.s7 : ret.sa;
  ret.sa = (m.sa == 0x18) ? v1.s8 : ret.sa;
  ret.sa = (m.sa == 0x19) ? v1.s9 : ret.sa;
  ret.sa = (m.sa == 0x1a) ? v1.sa : ret.sa;
  ret.sa = (m.sa == 0x1b) ? v1.sb : ret.sa;
  ret.sa = (m.sa == 0x1c) ? v1.sc : ret.sa;
  ret.sa = (m.sa == 0x1d) ? v1.sd : ret.sa;
  ret.sa = (m.sa == 0x1e) ? v1.se : ret.sa;
  ret.sa = (m.sa == 0x1f) ? v1.sf : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v0.s8 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v0.s9 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v0.sa : ret.sb;
  ret.sb = (m.sb == 0xb) ? v0.sb : ret.sb;
  ret.sb = (m.sb == 0xc) ? v0.sc : ret.sb;
  ret.sb = (m.sb == 0xd) ? v0.sd : ret.sb;
  ret.sb = (m.sb == 0xe) ? v0.se : ret.sb;
  ret.sb = (m.sb == 0xf) ? v0.sf : ret.sb;
  ret.sb = (m.sb == 0x10) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x11) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x12) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x13) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0x14) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0x15) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0x16) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0x17) ? v1.s7 : ret.sb;
  ret.sb = (m.sb == 0x18) ? v1.s8 : ret.sb;
  ret.sb = (m.sb == 0x19) ? v1.s9 : ret.sb;
  ret.sb = (m.sb == 0x1a) ? v1.sa : ret.sb;
  ret.sb = (m.sb == 0x1b) ? v1.sb : ret.sb;
  ret.sb = (m.sb == 0x1c) ? v1.sc : ret.sb;
  ret.sb = (m.sb == 0x1d) ? v1.sd : ret.sb;
  ret.sb = (m.sb == 0x1e) ? v1.se : ret.sb;
  ret.sb = (m.sb == 0x1f) ? v1.sf : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v0.s8 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v0.s9 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v0.sa : ret.sc;
  ret.sc = (m.sc == 0xb) ? v0.sb : ret.sc;
  ret.sc = (m.sc == 0xc) ? v0.sc : ret.sc;
  ret.sc = (m.sc == 0xd) ? v0.sd : ret.sc;
  ret.sc = (m.sc == 0xe) ? v0.se : ret.sc;
  ret.sc = (m.sc == 0xf) ? v0.sf : ret.sc;
  ret.sc = (m.sc == 0x10) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x11) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x12) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x13) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0x14) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0x15) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0x16) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0x17) ? v1.s7 : ret.sc;
  ret.sc = (m.sc == 0x18) ? v1.s8 : ret.sc;
  ret.sc = (m.sc == 0x19) ? v1.s9 : ret.sc;
  ret.sc = (m.sc == 0x1a) ? v1.sa : ret.sc;
  ret.sc = (m.sc == 0x1b) ? v1.sb : ret.sc;
  ret.sc = (m.sc == 0x1c) ? v1.sc : ret.sc;
  ret.sc = (m.sc == 0x1d) ? v1.sd : ret.sc;
  ret.sc = (m.sc == 0x1e) ? v1.se : ret.sc;
  ret.sc = (m.sc == 0x1f) ? v1.sf : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v0.s8 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v0.s9 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v0.sa : ret.sd;
  ret.sd = (m.sd == 0xb) ? v0.sb : ret.sd;
  ret.sd = (m.sd == 0xc) ? v0.sc : ret.sd;
  ret.sd = (m.sd == 0xd) ? v0.sd : ret.sd;
  ret.sd = (m.sd == 0xe) ? v0.se : ret.sd;
  ret.sd = (m.sd == 0xf) ? v0.sf : ret.sd;
  ret.sd = (m.sd == 0x10) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x11) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x12) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x13) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0x14) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0x15) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0x16) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0x17) ? v1.s7 : ret.sd;
  ret.sd = (m.sd == 0x18) ? v1.s8 : ret.sd;
  ret.sd = (m.sd == 0x19) ? v1.s9 : ret.sd;
  ret.sd = (m.sd == 0x1a) ? v1.sa : ret.sd;
  ret.sd = (m.sd == 0x1b) ? v1.sb : ret.sd;
  ret.sd = (m.sd == 0x1c) ? v1.sc : ret.sd;
  ret.sd = (m.sd == 0x1d) ? v1.sd : ret.sd;
  ret.sd = (m.sd == 0x1e) ? v1.se : ret.sd;
  ret.sd = (m.sd == 0x1f) ? v1.sf : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v0.s8 : ret.se;
  ret.se = (m.se == 0x9) ? v0.s9 : ret.se;
  ret.se = (m.se == 0xa) ? v0.sa : ret.se;
  ret.se = (m.se == 0xb) ? v0.sb : ret.se;
  ret.se = (m.se == 0xc) ? v0.sc : ret.se;
  ret.se = (m.se == 0xd) ? v0.sd : ret.se;
  ret.se = (m.se == 0xe) ? v0.se : ret.se;
  ret.se = (m.se == 0xf) ? v0.sf : ret.se;
  ret.se = (m.se == 0x10) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x11) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x12) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x13) ? v1.s3 : ret.se;
  ret.se = (m.se == 0x14) ? v1.s4 : ret.se;
  ret.se = (m.se == 0x15) ? v1.s5 : ret.se;
  ret.se = (m.se == 0x16) ? v1.s6 : ret.se;
  ret.se = (m.se == 0x17) ? v1.s7 : ret.se;
  ret.se = (m.se == 0x18) ? v1.s8 : ret.se;
  ret.se = (m.se == 0x19) ? v1.s9 : ret.se;
  ret.se = (m.se == 0x1a) ? v1.sa : ret.se;
  ret.se = (m.se == 0x1b) ? v1.sb : ret.se;
  ret.se = (m.se == 0x1c) ? v1.sc : ret.se;
  ret.se = (m.se == 0x1d) ? v1.sd : ret.se;
  ret.se = (m.se == 0x1e) ? v1.se : ret.se;
  ret.se = (m.se == 0x1f) ? v1.sf : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v0.s8 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v0.s9 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v0.sa : ret.sf;
  ret.sf = (m.sf == 0xb) ? v0.sb : ret.sf;
  ret.sf = (m.sf == 0xc) ? v0.sc : ret.sf;
  ret.sf = (m.sf == 0xd) ? v0.sd : ret.sf;
  ret.sf = (m.sf == 0xe) ? v0.se : ret.sf;
  ret.sf = (m.sf == 0xf) ? v0.sf : ret.sf;
  ret.sf = (m.sf == 0x10) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x11) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x12) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x13) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0x14) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0x15) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0x16) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0x17) ? v1.s7 : ret.sf;
  ret.sf = (m.sf == 0x18) ? v1.s8 : ret.sf;
  ret.sf = (m.sf == 0x19) ? v1.s9 : ret.sf;
  ret.sf = (m.sf == 0x1a) ? v1.sa : ret.sf;
  ret.sf = (m.sf == 0x1b) ? v1.sb : ret.sf;
  ret.sf = (m.sf == 0x1c) ? v1.sc : ret.sf;
  ret.sf = (m.sf == 0x1d) ? v1.sd : ret.sf;
  ret.sf = (m.sf == 0x1e) ? v1.se : ret.sf;
  ret.sf = (m.sf == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

#if defined(cl_khr_fp16)

/// Half Shuffle functions
INLINE half2 OVERLOADABLE shuffle(half2 v, ushort2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2f16_v2i16, )( v, as_short2( m ) );
}

INLINE half2 OVERLOADABLE shuffle(half4 v, ushort2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4f16_v2i16, )( v, as_short2( m ) );
}

INLINE half2 OVERLOADABLE shuffle(half8 v, ushort2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8f16_v2i16, )( v, as_short2( m ) );
}

INLINE half2 OVERLOADABLE shuffle(half16 v, ushort2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16f16_v2i16, )( v, as_short2( m ) );
}

INLINE half4 OVERLOADABLE shuffle(half2 v, ushort4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2f16_v4i16, )( v, as_short4( m ) );
}

INLINE half4 OVERLOADABLE shuffle(half4 v, ushort4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4f16_v4i16, )( v, as_short4( m ) );
}

INLINE half4 OVERLOADABLE shuffle(half8 v, ushort4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8f16_v4i16, )( v, as_short4( m ) );
}

INLINE half4 OVERLOADABLE shuffle(half16 v, ushort4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16f16_v4i16, )( v, as_short4( m ) );
}

INLINE half8 OVERLOADABLE shuffle(half2 v, ushort8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2f16_v8i16, )( v, as_short8( m ) );
}

INLINE half8 OVERLOADABLE shuffle(half4 v, ushort8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4f16_v8i16, )( v, as_short8( m ) );
}

INLINE half8 OVERLOADABLE shuffle(half8 v, ushort8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8f16_v8i16, )( v, as_short8( m ) );
}

INLINE half8 OVERLOADABLE shuffle(half16 v, ushort8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16f16_v8i16, )( v, as_short8( m ) );
}

INLINE half16 OVERLOADABLE shuffle(half2 v, ushort16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2f16_v16i16, )( v, as_short16( m ) );
}

INLINE half16 OVERLOADABLE shuffle(half4 v, ushort16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4f16_v16i16, )( v, as_short16( m ) );
}

INLINE half16 OVERLOADABLE shuffle(half8 v, ushort16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8f16_v16i16, )( v, as_short16( m ) );
}

INLINE half16 OVERLOADABLE shuffle(half16 v, ushort16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16f16_v16i16, )( v, as_short16( m ) );
}

// Shuffle2
INLINE half2 OVERLOADABLE shuffle2(half2 v0, half2 v1, ushort2 m) {
  half2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE half2 OVERLOADABLE shuffle2(half4 v0, half4 v1, ushort2 m) {
  half2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE half2 OVERLOADABLE shuffle2(half8 v0, half8 v1, ushort2 m) {
  half2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE half2 OVERLOADABLE shuffle2(half16 v0, half16 v1, ushort2 m) {
  half2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE half4 OVERLOADABLE shuffle2(half2 v0, half2 v1, ushort4 m) {
  half4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE half4 OVERLOADABLE shuffle2(half4 v0, half4 v1, ushort4 m) {
  half4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE half4 OVERLOADABLE shuffle2(half8 v0, half8 v1, ushort4 m) {
  half4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE half4 OVERLOADABLE shuffle2(half16 v0, half16 v1, ushort4 m) {
  half4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE half8 OVERLOADABLE shuffle2(half2 v0, half2 v1, ushort8 m) {
  half8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE half8 OVERLOADABLE shuffle2(half4 v0, half4 v1, ushort8 m) {
  half8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE half8 OVERLOADABLE shuffle2(half8 v0, half8 v1, ushort8 m) {
  half8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE half8 OVERLOADABLE shuffle2(half16 v0, half16 v1, ushort8 m) {
  half8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE half16 OVERLOADABLE shuffle2(half2 v0, half2 v1, ushort16 m) {
  half16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v1.s1 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v1.s1 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v1.s1 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v1.s1 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v1.s1 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x3) ? v1.s1 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE half16 OVERLOADABLE shuffle2(half4 v0, half4 v1, ushort16 m) {
  half16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v1.s3 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v1.s3 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v1.s3 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v1.s3 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v1.s3 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x5) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x6) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x7) ? v1.s3 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE half16 OVERLOADABLE shuffle2(half8 v0, half8 v1, ushort16 m) {
  half16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v1.s7 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0xb) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0xc) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0xd) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0xe) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0xf) ? v1.s7 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0xb) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0xc) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0xd) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0xe) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0xf) ? v1.s7 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0xb) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0xc) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0xd) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0xe) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0xf) ? v1.s7 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0xb) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0xc) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0xd) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0xe) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0xf) ? v1.s7 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x9) ? v1.s1 : ret.se;
  ret.se = (m.se == 0xa) ? v1.s2 : ret.se;
  ret.se = (m.se == 0xb) ? v1.s3 : ret.se;
  ret.se = (m.se == 0xc) ? v1.s4 : ret.se;
  ret.se = (m.se == 0xd) ? v1.s5 : ret.se;
  ret.se = (m.se == 0xe) ? v1.s6 : ret.se;
  ret.se = (m.se == 0xf) ? v1.s7 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0xb) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0xc) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0xd) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0xe) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE half16 OVERLOADABLE shuffle2(half16 v0, half16 v1, ushort16 m) {
  half16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v0.sa : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v0.sb : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v0.sc : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v0.sd : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v0.se : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v0.sf : ret.s8;
  ret.s8 = (m.s8 == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = (m.s8 == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = (m.s8 == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = (m.s8 == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = (m.s8 == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = (m.s8 == 0x1e) ? v1.se : ret.s8;
  ret.s8 = (m.s8 == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v0.sa : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v0.sb : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v0.sc : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v0.sd : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v0.se : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v0.sf : ret.s9;
  ret.s9 = (m.s9 == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = (m.s9 == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = (m.s9 == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = (m.s9 == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = (m.s9 == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = (m.s9 == 0x1e) ? v1.se : ret.s9;
  ret.s9 = (m.s9 == 0x1f) ? v1.sf : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v0.s8 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v0.s9 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v0.sa : ret.sa;
  ret.sa = (m.sa == 0xb) ? v0.sb : ret.sa;
  ret.sa = (m.sa == 0xc) ? v0.sc : ret.sa;
  ret.sa = (m.sa == 0xd) ? v0.sd : ret.sa;
  ret.sa = (m.sa == 0xe) ? v0.se : ret.sa;
  ret.sa = (m.sa == 0xf) ? v0.sf : ret.sa;
  ret.sa = (m.sa == 0x10) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x11) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x12) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x13) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0x14) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0x15) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0x16) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0x17) ? v1.s7 : ret.sa;
  ret.sa = (m.sa == 0x18) ? v1.s8 : ret.sa;
  ret.sa = (m.sa == 0x19) ? v1.s9 : ret.sa;
  ret.sa = (m.sa == 0x1a) ? v1.sa : ret.sa;
  ret.sa = (m.sa == 0x1b) ? v1.sb : ret.sa;
  ret.sa = (m.sa == 0x1c) ? v1.sc : ret.sa;
  ret.sa = (m.sa == 0x1d) ? v1.sd : ret.sa;
  ret.sa = (m.sa == 0x1e) ? v1.se : ret.sa;
  ret.sa = (m.sa == 0x1f) ? v1.sf : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v0.s8 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v0.s9 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v0.sa : ret.sb;
  ret.sb = (m.sb == 0xb) ? v0.sb : ret.sb;
  ret.sb = (m.sb == 0xc) ? v0.sc : ret.sb;
  ret.sb = (m.sb == 0xd) ? v0.sd : ret.sb;
  ret.sb = (m.sb == 0xe) ? v0.se : ret.sb;
  ret.sb = (m.sb == 0xf) ? v0.sf : ret.sb;
  ret.sb = (m.sb == 0x10) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x11) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x12) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x13) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0x14) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0x15) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0x16) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0x17) ? v1.s7 : ret.sb;
  ret.sb = (m.sb == 0x18) ? v1.s8 : ret.sb;
  ret.sb = (m.sb == 0x19) ? v1.s9 : ret.sb;
  ret.sb = (m.sb == 0x1a) ? v1.sa : ret.sb;
  ret.sb = (m.sb == 0x1b) ? v1.sb : ret.sb;
  ret.sb = (m.sb == 0x1c) ? v1.sc : ret.sb;
  ret.sb = (m.sb == 0x1d) ? v1.sd : ret.sb;
  ret.sb = (m.sb == 0x1e) ? v1.se : ret.sb;
  ret.sb = (m.sb == 0x1f) ? v1.sf : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v0.s8 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v0.s9 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v0.sa : ret.sc;
  ret.sc = (m.sc == 0xb) ? v0.sb : ret.sc;
  ret.sc = (m.sc == 0xc) ? v0.sc : ret.sc;
  ret.sc = (m.sc == 0xd) ? v0.sd : ret.sc;
  ret.sc = (m.sc == 0xe) ? v0.se : ret.sc;
  ret.sc = (m.sc == 0xf) ? v0.sf : ret.sc;
  ret.sc = (m.sc == 0x10) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x11) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x12) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x13) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0x14) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0x15) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0x16) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0x17) ? v1.s7 : ret.sc;
  ret.sc = (m.sc == 0x18) ? v1.s8 : ret.sc;
  ret.sc = (m.sc == 0x19) ? v1.s9 : ret.sc;
  ret.sc = (m.sc == 0x1a) ? v1.sa : ret.sc;
  ret.sc = (m.sc == 0x1b) ? v1.sb : ret.sc;
  ret.sc = (m.sc == 0x1c) ? v1.sc : ret.sc;
  ret.sc = (m.sc == 0x1d) ? v1.sd : ret.sc;
  ret.sc = (m.sc == 0x1e) ? v1.se : ret.sc;
  ret.sc = (m.sc == 0x1f) ? v1.sf : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v0.s8 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v0.s9 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v0.sa : ret.sd;
  ret.sd = (m.sd == 0xb) ? v0.sb : ret.sd;
  ret.sd = (m.sd == 0xc) ? v0.sc : ret.sd;
  ret.sd = (m.sd == 0xd) ? v0.sd : ret.sd;
  ret.sd = (m.sd == 0xe) ? v0.se : ret.sd;
  ret.sd = (m.sd == 0xf) ? v0.sf : ret.sd;
  ret.sd = (m.sd == 0x10) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x11) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x12) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x13) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0x14) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0x15) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0x16) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0x17) ? v1.s7 : ret.sd;
  ret.sd = (m.sd == 0x18) ? v1.s8 : ret.sd;
  ret.sd = (m.sd == 0x19) ? v1.s9 : ret.sd;
  ret.sd = (m.sd == 0x1a) ? v1.sa : ret.sd;
  ret.sd = (m.sd == 0x1b) ? v1.sb : ret.sd;
  ret.sd = (m.sd == 0x1c) ? v1.sc : ret.sd;
  ret.sd = (m.sd == 0x1d) ? v1.sd : ret.sd;
  ret.sd = (m.sd == 0x1e) ? v1.se : ret.sd;
  ret.sd = (m.sd == 0x1f) ? v1.sf : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v0.s8 : ret.se;
  ret.se = (m.se == 0x9) ? v0.s9 : ret.se;
  ret.se = (m.se == 0xa) ? v0.sa : ret.se;
  ret.se = (m.se == 0xb) ? v0.sb : ret.se;
  ret.se = (m.se == 0xc) ? v0.sc : ret.se;
  ret.se = (m.se == 0xd) ? v0.sd : ret.se;
  ret.se = (m.se == 0xe) ? v0.se : ret.se;
  ret.se = (m.se == 0xf) ? v0.sf : ret.se;
  ret.se = (m.se == 0x10) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x11) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x12) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x13) ? v1.s3 : ret.se;
  ret.se = (m.se == 0x14) ? v1.s4 : ret.se;
  ret.se = (m.se == 0x15) ? v1.s5 : ret.se;
  ret.se = (m.se == 0x16) ? v1.s6 : ret.se;
  ret.se = (m.se == 0x17) ? v1.s7 : ret.se;
  ret.se = (m.se == 0x18) ? v1.s8 : ret.se;
  ret.se = (m.se == 0x19) ? v1.s9 : ret.se;
  ret.se = (m.se == 0x1a) ? v1.sa : ret.se;
  ret.se = (m.se == 0x1b) ? v1.sb : ret.se;
  ret.se = (m.se == 0x1c) ? v1.sc : ret.se;
  ret.se = (m.se == 0x1d) ? v1.sd : ret.se;
  ret.se = (m.se == 0x1e) ? v1.se : ret.se;
  ret.se = (m.se == 0x1f) ? v1.sf : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v0.s8 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v0.s9 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v0.sa : ret.sf;
  ret.sf = (m.sf == 0xb) ? v0.sb : ret.sf;
  ret.sf = (m.sf == 0xc) ? v0.sc : ret.sf;
  ret.sf = (m.sf == 0xd) ? v0.sd : ret.sf;
  ret.sf = (m.sf == 0xe) ? v0.se : ret.sf;
  ret.sf = (m.sf == 0xf) ? v0.sf : ret.sf;
  ret.sf = (m.sf == 0x10) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x11) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x12) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x13) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0x14) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0x15) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0x16) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0x17) ? v1.s7 : ret.sf;
  ret.sf = (m.sf == 0x18) ? v1.s8 : ret.sf;
  ret.sf = (m.sf == 0x19) ? v1.s9 : ret.sf;
  ret.sf = (m.sf == 0x1a) ? v1.sa : ret.sf;
  ret.sf = (m.sf == 0x1b) ? v1.sb : ret.sf;
  ret.sf = (m.sf == 0x1c) ? v1.sc : ret.sf;
  ret.sf = (m.sf == 0x1d) ? v1.sd : ret.sf;
  ret.sf = (m.sf == 0x1e) ? v1.se : ret.sf;
  ret.sf = (m.sf == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

#endif // defined(cl_khr_fp16)

#if defined(cl_khr_fp64)

/// Double Shuffle functions
INLINE double2 OVERLOADABLE shuffle(double2 v, ulong2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2f64_v2i64, )( v, as_long2( m ) );
}

INLINE double2 OVERLOADABLE shuffle(double4 v, ulong2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4f64_v2i64, )( v, as_long2( m ) );
}

INLINE double2 OVERLOADABLE shuffle(double8 v, ulong2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8f64_v2i64, )( v, as_long2( m ) );
}

INLINE double2 OVERLOADABLE shuffle(double16 v, ulong2 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16f64_v2i64, )( v, as_long2( m ) );
}

INLINE double4 OVERLOADABLE shuffle(double2 v, ulong4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2f64_v4i64, )( v, as_long4( m ) );
}

INLINE double4 OVERLOADABLE shuffle(double4 v, ulong4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4f64_v4i64, )( v, as_long4( m ) );
}

INLINE double4 OVERLOADABLE shuffle(double8 v, ulong4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8f64_v4i64, )( v, as_long4( m ) );
}

INLINE double4 OVERLOADABLE shuffle(double16 v, ulong4 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16f64_v4i64, )( v, as_long4( m ) );
}

INLINE double8 OVERLOADABLE shuffle(double2 v, ulong8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2f64_v8i64, )( v, as_long8( m ) );
}

INLINE double8 OVERLOADABLE shuffle(double4 v, ulong8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4f64_v8i64, )( v, as_long8( m ) );
}

INLINE double8 OVERLOADABLE shuffle(double8 v, ulong8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8f64_v8i64, )( v, as_long8( m ) );
}

INLINE double8 OVERLOADABLE shuffle(double16 v, ulong8 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16f64_v8i64, )( v, as_long8( m ) );
}

INLINE double16 OVERLOADABLE shuffle(double2 v, ulong16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v2f64_v16i64, )( v, as_long16( m ) );
}

INLINE double16 OVERLOADABLE shuffle(double4 v, ulong16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v4f64_v16i64, )( v, as_long16( m ) );
}

INLINE double16 OVERLOADABLE shuffle(double8 v, ulong16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v8f64_v16i64, )( v, as_long16( m ) );
}

INLINE double16 OVERLOADABLE shuffle(double16 v, ulong16 m) {
    return SPIRV_OCL_BUILTIN(shuffle, _v16f64_v16i64, )( v, as_long16( m ) );
}

// Shuffle2
INLINE double2 OVERLOADABLE shuffle2(double2 v0, double2 v1, ulong2 m) {
  double2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  return ret;
}

INLINE double2 OVERLOADABLE shuffle2(double4 v0, double4 v1, ulong2 m) {
  double2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  return ret;
}

INLINE double2 OVERLOADABLE shuffle2(double8 v0, double8 v1, ulong2 m) {
  double2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  return ret;
}

INLINE double2 OVERLOADABLE shuffle2(double16 v0, double16 v1, ulong2 m) {
  double2 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  return ret;
}

INLINE double4 OVERLOADABLE shuffle2(double2 v0, double2 v1, ulong4 m) {
  double4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  return ret;
}

INLINE double4 OVERLOADABLE shuffle2(double4 v0, double4 v1, ulong4 m) {
  double4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  return ret;
}

INLINE double4 OVERLOADABLE shuffle2(double8 v0, double8 v1, ulong4 m) {
  double4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  return ret;
}

INLINE double4 OVERLOADABLE shuffle2(double16 v0, double16 v1, ulong4 m) {
  double4 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  return ret;
}

INLINE double8 OVERLOADABLE shuffle2(double2 v0, double2 v1, ulong8 m) {
  double8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  return ret;
}

INLINE double8 OVERLOADABLE shuffle2(double4 v0, double4 v1, ulong8 m) {
  double8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  return ret;
}

INLINE double8 OVERLOADABLE shuffle2(double8 v0, double8 v1, ulong8 m) {
  double8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  return ret;
}

INLINE double8 OVERLOADABLE shuffle2(double16 v0, double16 v1, ulong8 m) {
  double8 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  return ret;
}

INLINE double16 OVERLOADABLE shuffle2(double2 v0, double2 v1, ulong16 m) {
  double16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v1.s1 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v1.s1 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v1.s1 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v1.s1 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v1.s1 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v1.s1 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v1.s1 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v1.s1 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v1.s1 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v1.s1 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v1.s1 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v1.s1 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v1.s1 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v1.s1 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x3) ? v1.s1 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v1.s1 : ret.sf;

  return ret;
}

INLINE double16 OVERLOADABLE shuffle2(double4 v0, double4 v1, ulong16 m) {
  double16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v1.s3 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v1.s3 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v1.s3 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v1.s3 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v1.s3 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v1.s3 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v1.s3 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v1.s3 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v1.s3 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v1.s3 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v1.s3 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v1.s3 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v1.s3 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v1.s3 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x5) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x6) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x7) ? v1.s3 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v1.s3 : ret.sf;

  return ret;
}

INLINE double16 OVERLOADABLE shuffle2(double8 v0, double8 v1, ulong16 m) {
  double16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v1.s7 : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v1.s7 : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v1.s7 : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v1.s7 : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v1.s7 : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v1.s7 : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v1.s7 : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v1.s7 : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v1.s7 : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v1.s7 : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0xb) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0xc) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0xd) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0xe) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0xf) ? v1.s7 : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0xb) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0xc) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0xd) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0xe) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0xf) ? v1.s7 : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0xb) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0xc) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0xd) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0xe) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0xf) ? v1.s7 : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0xb) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0xc) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0xd) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0xe) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0xf) ? v1.s7 : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x9) ? v1.s1 : ret.se;
  ret.se = (m.se == 0xa) ? v1.s2 : ret.se;
  ret.se = (m.se == 0xb) ? v1.s3 : ret.se;
  ret.se = (m.se == 0xc) ? v1.s4 : ret.se;
  ret.se = (m.se == 0xd) ? v1.s5 : ret.se;
  ret.se = (m.se == 0xe) ? v1.s6 : ret.se;
  ret.se = (m.se == 0xf) ? v1.s7 : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0xb) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0xc) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0xd) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0xe) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0xf) ? v1.s7 : ret.sf;

  return ret;
}

INLINE double16 OVERLOADABLE shuffle2(double16 v0, double16 v1, ulong16 m) {
  double16 ret = 0;
  ret.s0 = (m.s0 == 0x0) ? v0.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x1) ? v0.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x2) ? v0.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x3) ? v0.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x4) ? v0.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x5) ? v0.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x6) ? v0.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x7) ? v0.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x8) ? v0.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x9) ? v0.s9 : ret.s0;
  ret.s0 = (m.s0 == 0xa) ? v0.sa : ret.s0;
  ret.s0 = (m.s0 == 0xb) ? v0.sb : ret.s0;
  ret.s0 = (m.s0 == 0xc) ? v0.sc : ret.s0;
  ret.s0 = (m.s0 == 0xd) ? v0.sd : ret.s0;
  ret.s0 = (m.s0 == 0xe) ? v0.se : ret.s0;
  ret.s0 = (m.s0 == 0xf) ? v0.sf : ret.s0;
  ret.s0 = (m.s0 == 0x10) ? v1.s0 : ret.s0;
  ret.s0 = (m.s0 == 0x11) ? v1.s1 : ret.s0;
  ret.s0 = (m.s0 == 0x12) ? v1.s2 : ret.s0;
  ret.s0 = (m.s0 == 0x13) ? v1.s3 : ret.s0;
  ret.s0 = (m.s0 == 0x14) ? v1.s4 : ret.s0;
  ret.s0 = (m.s0 == 0x15) ? v1.s5 : ret.s0;
  ret.s0 = (m.s0 == 0x16) ? v1.s6 : ret.s0;
  ret.s0 = (m.s0 == 0x17) ? v1.s7 : ret.s0;
  ret.s0 = (m.s0 == 0x18) ? v1.s8 : ret.s0;
  ret.s0 = (m.s0 == 0x19) ? v1.s9 : ret.s0;
  ret.s0 = (m.s0 == 0x1a) ? v1.sa : ret.s0;
  ret.s0 = (m.s0 == 0x1b) ? v1.sb : ret.s0;
  ret.s0 = (m.s0 == 0x1c) ? v1.sc : ret.s0;
  ret.s0 = (m.s0 == 0x1d) ? v1.sd : ret.s0;
  ret.s0 = (m.s0 == 0x1e) ? v1.se : ret.s0;
  ret.s0 = (m.s0 == 0x1f) ? v1.sf : ret.s0;

  ret.s1 = (m.s1 == 0x0) ? v0.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x1) ? v0.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x2) ? v0.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x3) ? v0.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x4) ? v0.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x5) ? v0.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x6) ? v0.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x7) ? v0.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x8) ? v0.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x9) ? v0.s9 : ret.s1;
  ret.s1 = (m.s1 == 0xa) ? v0.sa : ret.s1;
  ret.s1 = (m.s1 == 0xb) ? v0.sb : ret.s1;
  ret.s1 = (m.s1 == 0xc) ? v0.sc : ret.s1;
  ret.s1 = (m.s1 == 0xd) ? v0.sd : ret.s1;
  ret.s1 = (m.s1 == 0xe) ? v0.se : ret.s1;
  ret.s1 = (m.s1 == 0xf) ? v0.sf : ret.s1;
  ret.s1 = (m.s1 == 0x10) ? v1.s0 : ret.s1;
  ret.s1 = (m.s1 == 0x11) ? v1.s1 : ret.s1;
  ret.s1 = (m.s1 == 0x12) ? v1.s2 : ret.s1;
  ret.s1 = (m.s1 == 0x13) ? v1.s3 : ret.s1;
  ret.s1 = (m.s1 == 0x14) ? v1.s4 : ret.s1;
  ret.s1 = (m.s1 == 0x15) ? v1.s5 : ret.s1;
  ret.s1 = (m.s1 == 0x16) ? v1.s6 : ret.s1;
  ret.s1 = (m.s1 == 0x17) ? v1.s7 : ret.s1;
  ret.s1 = (m.s1 == 0x18) ? v1.s8 : ret.s1;
  ret.s1 = (m.s1 == 0x19) ? v1.s9 : ret.s1;
  ret.s1 = (m.s1 == 0x1a) ? v1.sa : ret.s1;
  ret.s1 = (m.s1 == 0x1b) ? v1.sb : ret.s1;
  ret.s1 = (m.s1 == 0x1c) ? v1.sc : ret.s1;
  ret.s1 = (m.s1 == 0x1d) ? v1.sd : ret.s1;
  ret.s1 = (m.s1 == 0x1e) ? v1.se : ret.s1;
  ret.s1 = (m.s1 == 0x1f) ? v1.sf : ret.s1;

  ret.s2 = (m.s2 == 0x0) ? v0.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x1) ? v0.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x2) ? v0.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x3) ? v0.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x4) ? v0.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x5) ? v0.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x6) ? v0.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x7) ? v0.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x8) ? v0.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x9) ? v0.s9 : ret.s2;
  ret.s2 = (m.s2 == 0xa) ? v0.sa : ret.s2;
  ret.s2 = (m.s2 == 0xb) ? v0.sb : ret.s2;
  ret.s2 = (m.s2 == 0xc) ? v0.sc : ret.s2;
  ret.s2 = (m.s2 == 0xd) ? v0.sd : ret.s2;
  ret.s2 = (m.s2 == 0xe) ? v0.se : ret.s2;
  ret.s2 = (m.s2 == 0xf) ? v0.sf : ret.s2;
  ret.s2 = (m.s2 == 0x10) ? v1.s0 : ret.s2;
  ret.s2 = (m.s2 == 0x11) ? v1.s1 : ret.s2;
  ret.s2 = (m.s2 == 0x12) ? v1.s2 : ret.s2;
  ret.s2 = (m.s2 == 0x13) ? v1.s3 : ret.s2;
  ret.s2 = (m.s2 == 0x14) ? v1.s4 : ret.s2;
  ret.s2 = (m.s2 == 0x15) ? v1.s5 : ret.s2;
  ret.s2 = (m.s2 == 0x16) ? v1.s6 : ret.s2;
  ret.s2 = (m.s2 == 0x17) ? v1.s7 : ret.s2;
  ret.s2 = (m.s2 == 0x18) ? v1.s8 : ret.s2;
  ret.s2 = (m.s2 == 0x19) ? v1.s9 : ret.s2;
  ret.s2 = (m.s2 == 0x1a) ? v1.sa : ret.s2;
  ret.s2 = (m.s2 == 0x1b) ? v1.sb : ret.s2;
  ret.s2 = (m.s2 == 0x1c) ? v1.sc : ret.s2;
  ret.s2 = (m.s2 == 0x1d) ? v1.sd : ret.s2;
  ret.s2 = (m.s2 == 0x1e) ? v1.se : ret.s2;
  ret.s2 = (m.s2 == 0x1f) ? v1.sf : ret.s2;

  ret.s3 = (m.s3 == 0x0) ? v0.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x1) ? v0.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x2) ? v0.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x3) ? v0.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x4) ? v0.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x5) ? v0.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x6) ? v0.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x7) ? v0.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x8) ? v0.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x9) ? v0.s9 : ret.s3;
  ret.s3 = (m.s3 == 0xa) ? v0.sa : ret.s3;
  ret.s3 = (m.s3 == 0xb) ? v0.sb : ret.s3;
  ret.s3 = (m.s3 == 0xc) ? v0.sc : ret.s3;
  ret.s3 = (m.s3 == 0xd) ? v0.sd : ret.s3;
  ret.s3 = (m.s3 == 0xe) ? v0.se : ret.s3;
  ret.s3 = (m.s3 == 0xf) ? v0.sf : ret.s3;
  ret.s3 = (m.s3 == 0x10) ? v1.s0 : ret.s3;
  ret.s3 = (m.s3 == 0x11) ? v1.s1 : ret.s3;
  ret.s3 = (m.s3 == 0x12) ? v1.s2 : ret.s3;
  ret.s3 = (m.s3 == 0x13) ? v1.s3 : ret.s3;
  ret.s3 = (m.s3 == 0x14) ? v1.s4 : ret.s3;
  ret.s3 = (m.s3 == 0x15) ? v1.s5 : ret.s3;
  ret.s3 = (m.s3 == 0x16) ? v1.s6 : ret.s3;
  ret.s3 = (m.s3 == 0x17) ? v1.s7 : ret.s3;
  ret.s3 = (m.s3 == 0x18) ? v1.s8 : ret.s3;
  ret.s3 = (m.s3 == 0x19) ? v1.s9 : ret.s3;
  ret.s3 = (m.s3 == 0x1a) ? v1.sa : ret.s3;
  ret.s3 = (m.s3 == 0x1b) ? v1.sb : ret.s3;
  ret.s3 = (m.s3 == 0x1c) ? v1.sc : ret.s3;
  ret.s3 = (m.s3 == 0x1d) ? v1.sd : ret.s3;
  ret.s3 = (m.s3 == 0x1e) ? v1.se : ret.s3;
  ret.s3 = (m.s3 == 0x1f) ? v1.sf : ret.s3;

  ret.s4 = (m.s4 == 0x0) ? v0.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x1) ? v0.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x2) ? v0.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x3) ? v0.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x4) ? v0.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x5) ? v0.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x6) ? v0.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x7) ? v0.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x8) ? v0.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x9) ? v0.s9 : ret.s4;
  ret.s4 = (m.s4 == 0xa) ? v0.sa : ret.s4;
  ret.s4 = (m.s4 == 0xb) ? v0.sb : ret.s4;
  ret.s4 = (m.s4 == 0xc) ? v0.sc : ret.s4;
  ret.s4 = (m.s4 == 0xd) ? v0.sd : ret.s4;
  ret.s4 = (m.s4 == 0xe) ? v0.se : ret.s4;
  ret.s4 = (m.s4 == 0xf) ? v0.sf : ret.s4;
  ret.s4 = (m.s4 == 0x10) ? v1.s0 : ret.s4;
  ret.s4 = (m.s4 == 0x11) ? v1.s1 : ret.s4;
  ret.s4 = (m.s4 == 0x12) ? v1.s2 : ret.s4;
  ret.s4 = (m.s4 == 0x13) ? v1.s3 : ret.s4;
  ret.s4 = (m.s4 == 0x14) ? v1.s4 : ret.s4;
  ret.s4 = (m.s4 == 0x15) ? v1.s5 : ret.s4;
  ret.s4 = (m.s4 == 0x16) ? v1.s6 : ret.s4;
  ret.s4 = (m.s4 == 0x17) ? v1.s7 : ret.s4;
  ret.s4 = (m.s4 == 0x18) ? v1.s8 : ret.s4;
  ret.s4 = (m.s4 == 0x19) ? v1.s9 : ret.s4;
  ret.s4 = (m.s4 == 0x1a) ? v1.sa : ret.s4;
  ret.s4 = (m.s4 == 0x1b) ? v1.sb : ret.s4;
  ret.s4 = (m.s4 == 0x1c) ? v1.sc : ret.s4;
  ret.s4 = (m.s4 == 0x1d) ? v1.sd : ret.s4;
  ret.s4 = (m.s4 == 0x1e) ? v1.se : ret.s4;
  ret.s4 = (m.s4 == 0x1f) ? v1.sf : ret.s4;

  ret.s5 = (m.s5 == 0x0) ? v0.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x1) ? v0.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x2) ? v0.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x3) ? v0.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x4) ? v0.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x5) ? v0.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x6) ? v0.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x7) ? v0.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x8) ? v0.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x9) ? v0.s9 : ret.s5;
  ret.s5 = (m.s5 == 0xa) ? v0.sa : ret.s5;
  ret.s5 = (m.s5 == 0xb) ? v0.sb : ret.s5;
  ret.s5 = (m.s5 == 0xc) ? v0.sc : ret.s5;
  ret.s5 = (m.s5 == 0xd) ? v0.sd : ret.s5;
  ret.s5 = (m.s5 == 0xe) ? v0.se : ret.s5;
  ret.s5 = (m.s5 == 0xf) ? v0.sf : ret.s5;
  ret.s5 = (m.s5 == 0x10) ? v1.s0 : ret.s5;
  ret.s5 = (m.s5 == 0x11) ? v1.s1 : ret.s5;
  ret.s5 = (m.s5 == 0x12) ? v1.s2 : ret.s5;
  ret.s5 = (m.s5 == 0x13) ? v1.s3 : ret.s5;
  ret.s5 = (m.s5 == 0x14) ? v1.s4 : ret.s5;
  ret.s5 = (m.s5 == 0x15) ? v1.s5 : ret.s5;
  ret.s5 = (m.s5 == 0x16) ? v1.s6 : ret.s5;
  ret.s5 = (m.s5 == 0x17) ? v1.s7 : ret.s5;
  ret.s5 = (m.s5 == 0x18) ? v1.s8 : ret.s5;
  ret.s5 = (m.s5 == 0x19) ? v1.s9 : ret.s5;
  ret.s5 = (m.s5 == 0x1a) ? v1.sa : ret.s5;
  ret.s5 = (m.s5 == 0x1b) ? v1.sb : ret.s5;
  ret.s5 = (m.s5 == 0x1c) ? v1.sc : ret.s5;
  ret.s5 = (m.s5 == 0x1d) ? v1.sd : ret.s5;
  ret.s5 = (m.s5 == 0x1e) ? v1.se : ret.s5;
  ret.s5 = (m.s5 == 0x1f) ? v1.sf : ret.s5;

  ret.s6 = (m.s6 == 0x0) ? v0.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x1) ? v0.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x2) ? v0.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x3) ? v0.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x4) ? v0.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x5) ? v0.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x6) ? v0.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x7) ? v0.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x8) ? v0.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x9) ? v0.s9 : ret.s6;
  ret.s6 = (m.s6 == 0xa) ? v0.sa : ret.s6;
  ret.s6 = (m.s6 == 0xb) ? v0.sb : ret.s6;
  ret.s6 = (m.s6 == 0xc) ? v0.sc : ret.s6;
  ret.s6 = (m.s6 == 0xd) ? v0.sd : ret.s6;
  ret.s6 = (m.s6 == 0xe) ? v0.se : ret.s6;
  ret.s6 = (m.s6 == 0xf) ? v0.sf : ret.s6;
  ret.s6 = (m.s6 == 0x10) ? v1.s0 : ret.s6;
  ret.s6 = (m.s6 == 0x11) ? v1.s1 : ret.s6;
  ret.s6 = (m.s6 == 0x12) ? v1.s2 : ret.s6;
  ret.s6 = (m.s6 == 0x13) ? v1.s3 : ret.s6;
  ret.s6 = (m.s6 == 0x14) ? v1.s4 : ret.s6;
  ret.s6 = (m.s6 == 0x15) ? v1.s5 : ret.s6;
  ret.s6 = (m.s6 == 0x16) ? v1.s6 : ret.s6;
  ret.s6 = (m.s6 == 0x17) ? v1.s7 : ret.s6;
  ret.s6 = (m.s6 == 0x18) ? v1.s8 : ret.s6;
  ret.s6 = (m.s6 == 0x19) ? v1.s9 : ret.s6;
  ret.s6 = (m.s6 == 0x1a) ? v1.sa : ret.s6;
  ret.s6 = (m.s6 == 0x1b) ? v1.sb : ret.s6;
  ret.s6 = (m.s6 == 0x1c) ? v1.sc : ret.s6;
  ret.s6 = (m.s6 == 0x1d) ? v1.sd : ret.s6;
  ret.s6 = (m.s6 == 0x1e) ? v1.se : ret.s6;
  ret.s6 = (m.s6 == 0x1f) ? v1.sf : ret.s6;

  ret.s7 = (m.s7 == 0x0) ? v0.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x1) ? v0.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x2) ? v0.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x3) ? v0.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x4) ? v0.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x5) ? v0.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x6) ? v0.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x7) ? v0.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x8) ? v0.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x9) ? v0.s9 : ret.s7;
  ret.s7 = (m.s7 == 0xa) ? v0.sa : ret.s7;
  ret.s7 = (m.s7 == 0xb) ? v0.sb : ret.s7;
  ret.s7 = (m.s7 == 0xc) ? v0.sc : ret.s7;
  ret.s7 = (m.s7 == 0xd) ? v0.sd : ret.s7;
  ret.s7 = (m.s7 == 0xe) ? v0.se : ret.s7;
  ret.s7 = (m.s7 == 0xf) ? v0.sf : ret.s7;
  ret.s7 = (m.s7 == 0x10) ? v1.s0 : ret.s7;
  ret.s7 = (m.s7 == 0x11) ? v1.s1 : ret.s7;
  ret.s7 = (m.s7 == 0x12) ? v1.s2 : ret.s7;
  ret.s7 = (m.s7 == 0x13) ? v1.s3 : ret.s7;
  ret.s7 = (m.s7 == 0x14) ? v1.s4 : ret.s7;
  ret.s7 = (m.s7 == 0x15) ? v1.s5 : ret.s7;
  ret.s7 = (m.s7 == 0x16) ? v1.s6 : ret.s7;
  ret.s7 = (m.s7 == 0x17) ? v1.s7 : ret.s7;
  ret.s7 = (m.s7 == 0x18) ? v1.s8 : ret.s7;
  ret.s7 = (m.s7 == 0x19) ? v1.s9 : ret.s7;
  ret.s7 = (m.s7 == 0x1a) ? v1.sa : ret.s7;
  ret.s7 = (m.s7 == 0x1b) ? v1.sb : ret.s7;
  ret.s7 = (m.s7 == 0x1c) ? v1.sc : ret.s7;
  ret.s7 = (m.s7 == 0x1d) ? v1.sd : ret.s7;
  ret.s7 = (m.s7 == 0x1e) ? v1.se : ret.s7;
  ret.s7 = (m.s7 == 0x1f) ? v1.sf : ret.s7;

  ret.s8 = (m.s8 == 0x0) ? v0.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x1) ? v0.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x2) ? v0.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x3) ? v0.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x4) ? v0.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x5) ? v0.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x6) ? v0.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x7) ? v0.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x8) ? v0.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x9) ? v0.s9 : ret.s8;
  ret.s8 = (m.s8 == 0xa) ? v0.sa : ret.s8;
  ret.s8 = (m.s8 == 0xb) ? v0.sb : ret.s8;
  ret.s8 = (m.s8 == 0xc) ? v0.sc : ret.s8;
  ret.s8 = (m.s8 == 0xd) ? v0.sd : ret.s8;
  ret.s8 = (m.s8 == 0xe) ? v0.se : ret.s8;
  ret.s8 = (m.s8 == 0xf) ? v0.sf : ret.s8;
  ret.s8 = (m.s8 == 0x10) ? v1.s0 : ret.s8;
  ret.s8 = (m.s8 == 0x11) ? v1.s1 : ret.s8;
  ret.s8 = (m.s8 == 0x12) ? v1.s2 : ret.s8;
  ret.s8 = (m.s8 == 0x13) ? v1.s3 : ret.s8;
  ret.s8 = (m.s8 == 0x14) ? v1.s4 : ret.s8;
  ret.s8 = (m.s8 == 0x15) ? v1.s5 : ret.s8;
  ret.s8 = (m.s8 == 0x16) ? v1.s6 : ret.s8;
  ret.s8 = (m.s8 == 0x17) ? v1.s7 : ret.s8;
  ret.s8 = (m.s8 == 0x18) ? v1.s8 : ret.s8;
  ret.s8 = (m.s8 == 0x19) ? v1.s9 : ret.s8;
  ret.s8 = (m.s8 == 0x1a) ? v1.sa : ret.s8;
  ret.s8 = (m.s8 == 0x1b) ? v1.sb : ret.s8;
  ret.s8 = (m.s8 == 0x1c) ? v1.sc : ret.s8;
  ret.s8 = (m.s8 == 0x1d) ? v1.sd : ret.s8;
  ret.s8 = (m.s8 == 0x1e) ? v1.se : ret.s8;
  ret.s8 = (m.s8 == 0x1f) ? v1.sf : ret.s8;

  ret.s9 = (m.s9 == 0x0) ? v0.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x1) ? v0.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x2) ? v0.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x3) ? v0.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x4) ? v0.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x5) ? v0.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x6) ? v0.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x7) ? v0.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x8) ? v0.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x9) ? v0.s9 : ret.s9;
  ret.s9 = (m.s9 == 0xa) ? v0.sa : ret.s9;
  ret.s9 = (m.s9 == 0xb) ? v0.sb : ret.s9;
  ret.s9 = (m.s9 == 0xc) ? v0.sc : ret.s9;
  ret.s9 = (m.s9 == 0xd) ? v0.sd : ret.s9;
  ret.s9 = (m.s9 == 0xe) ? v0.se : ret.s9;
  ret.s9 = (m.s9 == 0xf) ? v0.sf : ret.s9;
  ret.s9 = (m.s9 == 0x10) ? v1.s0 : ret.s9;
  ret.s9 = (m.s9 == 0x11) ? v1.s1 : ret.s9;
  ret.s9 = (m.s9 == 0x12) ? v1.s2 : ret.s9;
  ret.s9 = (m.s9 == 0x13) ? v1.s3 : ret.s9;
  ret.s9 = (m.s9 == 0x14) ? v1.s4 : ret.s9;
  ret.s9 = (m.s9 == 0x15) ? v1.s5 : ret.s9;
  ret.s9 = (m.s9 == 0x16) ? v1.s6 : ret.s9;
  ret.s9 = (m.s9 == 0x17) ? v1.s7 : ret.s9;
  ret.s9 = (m.s9 == 0x18) ? v1.s8 : ret.s9;
  ret.s9 = (m.s9 == 0x19) ? v1.s9 : ret.s9;
  ret.s9 = (m.s9 == 0x1a) ? v1.sa : ret.s9;
  ret.s9 = (m.s9 == 0x1b) ? v1.sb : ret.s9;
  ret.s9 = (m.s9 == 0x1c) ? v1.sc : ret.s9;
  ret.s9 = (m.s9 == 0x1d) ? v1.sd : ret.s9;
  ret.s9 = (m.s9 == 0x1e) ? v1.se : ret.s9;
  ret.s9 = (m.s9 == 0x1f) ? v1.sf : ret.s9;

  ret.sa = (m.sa == 0x0) ? v0.s0 : ret.sa;
  ret.sa = (m.sa == 0x1) ? v0.s1 : ret.sa;
  ret.sa = (m.sa == 0x2) ? v0.s2 : ret.sa;
  ret.sa = (m.sa == 0x3) ? v0.s3 : ret.sa;
  ret.sa = (m.sa == 0x4) ? v0.s4 : ret.sa;
  ret.sa = (m.sa == 0x5) ? v0.s5 : ret.sa;
  ret.sa = (m.sa == 0x6) ? v0.s6 : ret.sa;
  ret.sa = (m.sa == 0x7) ? v0.s7 : ret.sa;
  ret.sa = (m.sa == 0x8) ? v0.s8 : ret.sa;
  ret.sa = (m.sa == 0x9) ? v0.s9 : ret.sa;
  ret.sa = (m.sa == 0xa) ? v0.sa : ret.sa;
  ret.sa = (m.sa == 0xb) ? v0.sb : ret.sa;
  ret.sa = (m.sa == 0xc) ? v0.sc : ret.sa;
  ret.sa = (m.sa == 0xd) ? v0.sd : ret.sa;
  ret.sa = (m.sa == 0xe) ? v0.se : ret.sa;
  ret.sa = (m.sa == 0xf) ? v0.sf : ret.sa;
  ret.sa = (m.sa == 0x10) ? v1.s0 : ret.sa;
  ret.sa = (m.sa == 0x11) ? v1.s1 : ret.sa;
  ret.sa = (m.sa == 0x12) ? v1.s2 : ret.sa;
  ret.sa = (m.sa == 0x13) ? v1.s3 : ret.sa;
  ret.sa = (m.sa == 0x14) ? v1.s4 : ret.sa;
  ret.sa = (m.sa == 0x15) ? v1.s5 : ret.sa;
  ret.sa = (m.sa == 0x16) ? v1.s6 : ret.sa;
  ret.sa = (m.sa == 0x17) ? v1.s7 : ret.sa;
  ret.sa = (m.sa == 0x18) ? v1.s8 : ret.sa;
  ret.sa = (m.sa == 0x19) ? v1.s9 : ret.sa;
  ret.sa = (m.sa == 0x1a) ? v1.sa : ret.sa;
  ret.sa = (m.sa == 0x1b) ? v1.sb : ret.sa;
  ret.sa = (m.sa == 0x1c) ? v1.sc : ret.sa;
  ret.sa = (m.sa == 0x1d) ? v1.sd : ret.sa;
  ret.sa = (m.sa == 0x1e) ? v1.se : ret.sa;
  ret.sa = (m.sa == 0x1f) ? v1.sf : ret.sa;

  ret.sb = (m.sb == 0x0) ? v0.s0 : ret.sb;
  ret.sb = (m.sb == 0x1) ? v0.s1 : ret.sb;
  ret.sb = (m.sb == 0x2) ? v0.s2 : ret.sb;
  ret.sb = (m.sb == 0x3) ? v0.s3 : ret.sb;
  ret.sb = (m.sb == 0x4) ? v0.s4 : ret.sb;
  ret.sb = (m.sb == 0x5) ? v0.s5 : ret.sb;
  ret.sb = (m.sb == 0x6) ? v0.s6 : ret.sb;
  ret.sb = (m.sb == 0x7) ? v0.s7 : ret.sb;
  ret.sb = (m.sb == 0x8) ? v0.s8 : ret.sb;
  ret.sb = (m.sb == 0x9) ? v0.s9 : ret.sb;
  ret.sb = (m.sb == 0xa) ? v0.sa : ret.sb;
  ret.sb = (m.sb == 0xb) ? v0.sb : ret.sb;
  ret.sb = (m.sb == 0xc) ? v0.sc : ret.sb;
  ret.sb = (m.sb == 0xd) ? v0.sd : ret.sb;
  ret.sb = (m.sb == 0xe) ? v0.se : ret.sb;
  ret.sb = (m.sb == 0xf) ? v0.sf : ret.sb;
  ret.sb = (m.sb == 0x10) ? v1.s0 : ret.sb;
  ret.sb = (m.sb == 0x11) ? v1.s1 : ret.sb;
  ret.sb = (m.sb == 0x12) ? v1.s2 : ret.sb;
  ret.sb = (m.sb == 0x13) ? v1.s3 : ret.sb;
  ret.sb = (m.sb == 0x14) ? v1.s4 : ret.sb;
  ret.sb = (m.sb == 0x15) ? v1.s5 : ret.sb;
  ret.sb = (m.sb == 0x16) ? v1.s6 : ret.sb;
  ret.sb = (m.sb == 0x17) ? v1.s7 : ret.sb;
  ret.sb = (m.sb == 0x18) ? v1.s8 : ret.sb;
  ret.sb = (m.sb == 0x19) ? v1.s9 : ret.sb;
  ret.sb = (m.sb == 0x1a) ? v1.sa : ret.sb;
  ret.sb = (m.sb == 0x1b) ? v1.sb : ret.sb;
  ret.sb = (m.sb == 0x1c) ? v1.sc : ret.sb;
  ret.sb = (m.sb == 0x1d) ? v1.sd : ret.sb;
  ret.sb = (m.sb == 0x1e) ? v1.se : ret.sb;
  ret.sb = (m.sb == 0x1f) ? v1.sf : ret.sb;

  ret.sc = (m.sc == 0x0) ? v0.s0 : ret.sc;
  ret.sc = (m.sc == 0x1) ? v0.s1 : ret.sc;
  ret.sc = (m.sc == 0x2) ? v0.s2 : ret.sc;
  ret.sc = (m.sc == 0x3) ? v0.s3 : ret.sc;
  ret.sc = (m.sc == 0x4) ? v0.s4 : ret.sc;
  ret.sc = (m.sc == 0x5) ? v0.s5 : ret.sc;
  ret.sc = (m.sc == 0x6) ? v0.s6 : ret.sc;
  ret.sc = (m.sc == 0x7) ? v0.s7 : ret.sc;
  ret.sc = (m.sc == 0x8) ? v0.s8 : ret.sc;
  ret.sc = (m.sc == 0x9) ? v0.s9 : ret.sc;
  ret.sc = (m.sc == 0xa) ? v0.sa : ret.sc;
  ret.sc = (m.sc == 0xb) ? v0.sb : ret.sc;
  ret.sc = (m.sc == 0xc) ? v0.sc : ret.sc;
  ret.sc = (m.sc == 0xd) ? v0.sd : ret.sc;
  ret.sc = (m.sc == 0xe) ? v0.se : ret.sc;
  ret.sc = (m.sc == 0xf) ? v0.sf : ret.sc;
  ret.sc = (m.sc == 0x10) ? v1.s0 : ret.sc;
  ret.sc = (m.sc == 0x11) ? v1.s1 : ret.sc;
  ret.sc = (m.sc == 0x12) ? v1.s2 : ret.sc;
  ret.sc = (m.sc == 0x13) ? v1.s3 : ret.sc;
  ret.sc = (m.sc == 0x14) ? v1.s4 : ret.sc;
  ret.sc = (m.sc == 0x15) ? v1.s5 : ret.sc;
  ret.sc = (m.sc == 0x16) ? v1.s6 : ret.sc;
  ret.sc = (m.sc == 0x17) ? v1.s7 : ret.sc;
  ret.sc = (m.sc == 0x18) ? v1.s8 : ret.sc;
  ret.sc = (m.sc == 0x19) ? v1.s9 : ret.sc;
  ret.sc = (m.sc == 0x1a) ? v1.sa : ret.sc;
  ret.sc = (m.sc == 0x1b) ? v1.sb : ret.sc;
  ret.sc = (m.sc == 0x1c) ? v1.sc : ret.sc;
  ret.sc = (m.sc == 0x1d) ? v1.sd : ret.sc;
  ret.sc = (m.sc == 0x1e) ? v1.se : ret.sc;
  ret.sc = (m.sc == 0x1f) ? v1.sf : ret.sc;

  ret.sd = (m.sd == 0x0) ? v0.s0 : ret.sd;
  ret.sd = (m.sd == 0x1) ? v0.s1 : ret.sd;
  ret.sd = (m.sd == 0x2) ? v0.s2 : ret.sd;
  ret.sd = (m.sd == 0x3) ? v0.s3 : ret.sd;
  ret.sd = (m.sd == 0x4) ? v0.s4 : ret.sd;
  ret.sd = (m.sd == 0x5) ? v0.s5 : ret.sd;
  ret.sd = (m.sd == 0x6) ? v0.s6 : ret.sd;
  ret.sd = (m.sd == 0x7) ? v0.s7 : ret.sd;
  ret.sd = (m.sd == 0x8) ? v0.s8 : ret.sd;
  ret.sd = (m.sd == 0x9) ? v0.s9 : ret.sd;
  ret.sd = (m.sd == 0xa) ? v0.sa : ret.sd;
  ret.sd = (m.sd == 0xb) ? v0.sb : ret.sd;
  ret.sd = (m.sd == 0xc) ? v0.sc : ret.sd;
  ret.sd = (m.sd == 0xd) ? v0.sd : ret.sd;
  ret.sd = (m.sd == 0xe) ? v0.se : ret.sd;
  ret.sd = (m.sd == 0xf) ? v0.sf : ret.sd;
  ret.sd = (m.sd == 0x10) ? v1.s0 : ret.sd;
  ret.sd = (m.sd == 0x11) ? v1.s1 : ret.sd;
  ret.sd = (m.sd == 0x12) ? v1.s2 : ret.sd;
  ret.sd = (m.sd == 0x13) ? v1.s3 : ret.sd;
  ret.sd = (m.sd == 0x14) ? v1.s4 : ret.sd;
  ret.sd = (m.sd == 0x15) ? v1.s5 : ret.sd;
  ret.sd = (m.sd == 0x16) ? v1.s6 : ret.sd;
  ret.sd = (m.sd == 0x17) ? v1.s7 : ret.sd;
  ret.sd = (m.sd == 0x18) ? v1.s8 : ret.sd;
  ret.sd = (m.sd == 0x19) ? v1.s9 : ret.sd;
  ret.sd = (m.sd == 0x1a) ? v1.sa : ret.sd;
  ret.sd = (m.sd == 0x1b) ? v1.sb : ret.sd;
  ret.sd = (m.sd == 0x1c) ? v1.sc : ret.sd;
  ret.sd = (m.sd == 0x1d) ? v1.sd : ret.sd;
  ret.sd = (m.sd == 0x1e) ? v1.se : ret.sd;
  ret.sd = (m.sd == 0x1f) ? v1.sf : ret.sd;

  ret.se = (m.se == 0x0) ? v0.s0 : ret.se;
  ret.se = (m.se == 0x1) ? v0.s1 : ret.se;
  ret.se = (m.se == 0x2) ? v0.s2 : ret.se;
  ret.se = (m.se == 0x3) ? v0.s3 : ret.se;
  ret.se = (m.se == 0x4) ? v0.s4 : ret.se;
  ret.se = (m.se == 0x5) ? v0.s5 : ret.se;
  ret.se = (m.se == 0x6) ? v0.s6 : ret.se;
  ret.se = (m.se == 0x7) ? v0.s7 : ret.se;
  ret.se = (m.se == 0x8) ? v0.s8 : ret.se;
  ret.se = (m.se == 0x9) ? v0.s9 : ret.se;
  ret.se = (m.se == 0xa) ? v0.sa : ret.se;
  ret.se = (m.se == 0xb) ? v0.sb : ret.se;
  ret.se = (m.se == 0xc) ? v0.sc : ret.se;
  ret.se = (m.se == 0xd) ? v0.sd : ret.se;
  ret.se = (m.se == 0xe) ? v0.se : ret.se;
  ret.se = (m.se == 0xf) ? v0.sf : ret.se;
  ret.se = (m.se == 0x10) ? v1.s0 : ret.se;
  ret.se = (m.se == 0x11) ? v1.s1 : ret.se;
  ret.se = (m.se == 0x12) ? v1.s2 : ret.se;
  ret.se = (m.se == 0x13) ? v1.s3 : ret.se;
  ret.se = (m.se == 0x14) ? v1.s4 : ret.se;
  ret.se = (m.se == 0x15) ? v1.s5 : ret.se;
  ret.se = (m.se == 0x16) ? v1.s6 : ret.se;
  ret.se = (m.se == 0x17) ? v1.s7 : ret.se;
  ret.se = (m.se == 0x18) ? v1.s8 : ret.se;
  ret.se = (m.se == 0x19) ? v1.s9 : ret.se;
  ret.se = (m.se == 0x1a) ? v1.sa : ret.se;
  ret.se = (m.se == 0x1b) ? v1.sb : ret.se;
  ret.se = (m.se == 0x1c) ? v1.sc : ret.se;
  ret.se = (m.se == 0x1d) ? v1.sd : ret.se;
  ret.se = (m.se == 0x1e) ? v1.se : ret.se;
  ret.se = (m.se == 0x1f) ? v1.sf : ret.se;

  ret.sf = (m.sf == 0x0) ? v0.s0 : ret.sf;
  ret.sf = (m.sf == 0x1) ? v0.s1 : ret.sf;
  ret.sf = (m.sf == 0x2) ? v0.s2 : ret.sf;
  ret.sf = (m.sf == 0x3) ? v0.s3 : ret.sf;
  ret.sf = (m.sf == 0x4) ? v0.s4 : ret.sf;
  ret.sf = (m.sf == 0x5) ? v0.s5 : ret.sf;
  ret.sf = (m.sf == 0x6) ? v0.s6 : ret.sf;
  ret.sf = (m.sf == 0x7) ? v0.s7 : ret.sf;
  ret.sf = (m.sf == 0x8) ? v0.s8 : ret.sf;
  ret.sf = (m.sf == 0x9) ? v0.s9 : ret.sf;
  ret.sf = (m.sf == 0xa) ? v0.sa : ret.sf;
  ret.sf = (m.sf == 0xb) ? v0.sb : ret.sf;
  ret.sf = (m.sf == 0xc) ? v0.sc : ret.sf;
  ret.sf = (m.sf == 0xd) ? v0.sd : ret.sf;
  ret.sf = (m.sf == 0xe) ? v0.se : ret.sf;
  ret.sf = (m.sf == 0xf) ? v0.sf : ret.sf;
  ret.sf = (m.sf == 0x10) ? v1.s0 : ret.sf;
  ret.sf = (m.sf == 0x11) ? v1.s1 : ret.sf;
  ret.sf = (m.sf == 0x12) ? v1.s2 : ret.sf;
  ret.sf = (m.sf == 0x13) ? v1.s3 : ret.sf;
  ret.sf = (m.sf == 0x14) ? v1.s4 : ret.sf;
  ret.sf = (m.sf == 0x15) ? v1.s5 : ret.sf;
  ret.sf = (m.sf == 0x16) ? v1.s6 : ret.sf;
  ret.sf = (m.sf == 0x17) ? v1.s7 : ret.sf;
  ret.sf = (m.sf == 0x18) ? v1.s8 : ret.sf;
  ret.sf = (m.sf == 0x19) ? v1.s9 : ret.sf;
  ret.sf = (m.sf == 0x1a) ? v1.sa : ret.sf;
  ret.sf = (m.sf == 0x1b) ? v1.sb : ret.sf;
  ret.sf = (m.sf == 0x1c) ? v1.sc : ret.sf;
  ret.sf = (m.sf == 0x1d) ? v1.sd : ret.sf;
  ret.sf = (m.sf == 0x1e) ? v1.se : ret.sf;
  ret.sf = (m.sf == 0x1f) ? v1.sf : ret.sf;

  return ret;
}

#endif // defined(cl_khr_fp64)
