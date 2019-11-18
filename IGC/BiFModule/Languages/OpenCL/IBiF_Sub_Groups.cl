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

//===-  IBiF_Sub_Groups.cl -===============================================//
//
// This file defines OpenCL sub groups functions.
// Many of these functions are part of the KHR cl_khr_subgroups extension.
// Some of these functions are part of the Intel cl_intel_subgroups Extension.
//
//===----------------------------------------------------------------------===//

// Private helper functions:
uint __intel_get_local_size( void );
uint __intel_get_enqueued_local_size( void );
uint __intel_get_local_linear_id( void );
int OVERLOADABLE sub_group_reduce_add( int );

INLINE uint OVERLOADABLE get_max_sub_group_size( void )
{
    return __builtin_IB_get_simd_size();
}

INLINE uint OVERLOADABLE get_num_sub_groups( void )
{
    uint    totalWorkGroupSize =
    __intel_get_local_size() +
    get_max_sub_group_size() - 1;
    return totalWorkGroupSize / get_max_sub_group_size();
}

INLINE uint OVERLOADABLE get_sub_group_id( void )
{
    return __intel_get_local_linear_id() / get_max_sub_group_size();
}

INLINE uint OVERLOADABLE get_sub_group_size( void )
{
    uint    remainder =
                __intel_get_local_size() & ( get_max_sub_group_size() - 1 );
    bool    fullSubGroup =
                ( remainder == 0 ) ||
                ( get_sub_group_id() < get_num_sub_groups() - 1 );

    return fullSubGroup ? get_max_sub_group_size() : remainder;
}

INLINE uint OVERLOADABLE get_enqueued_num_sub_groups( void )
{
    uint    totalEnqueuedWorkGroupSize =
                __intel_get_enqueued_local_size() +
                get_max_sub_group_size() - 1;
    return totalEnqueuedWorkGroupSize / get_max_sub_group_size();
}


INLINE uint OVERLOADABLE get_sub_group_local_id( void )
{
    return __builtin_IB_get_simd_id();
}

INLINE void OVERLOADABLE sub_group_barrier( cl_mem_fence_flags flags )
{
    __builtin_spirv_OpControlBarrier_i32_i32_i32( Subgroup, 0, AcquireRelease );
}

#if(__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE void OVERLOADABLE sub_group_barrier( cl_mem_fence_flags flags, memory_scope scope )
{
    __builtin_spirv_OpControlBarrier_i32_i32_i32( Subgroup, 0, AcquireRelease );
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#ifdef cl_khr_fp16
INLINE half OVERLOADABLE intel_sub_group_shuffle( half x, uint c )
{
    return __builtin_IB_simd_shuffle_h( x, c );
}
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, half, half, uint )
#endif // cl_khr_fp16

INLINE ushort OVERLOADABLE intel_sub_group_shuffle( ushort x, uint c )
{
    return __builtin_IB_simd_shuffle_us( x, c );
}

INLINE short OVERLOADABLE intel_sub_group_shuffle( short x, uint c )
{
    return as_short( intel_sub_group_shuffle( as_ushort(x), c ) );
}

#ifdef cl_intel_subgroups_char
INLINE uchar OVERLOADABLE intel_sub_group_shuffle( uchar x, uint c )
{
    return as_uchar( __builtin_IB_simd_shuffle_b( as_char(x), c ) );
}

INLINE char OVERLOADABLE intel_sub_group_shuffle( char x, uint c )
{
    return as_char( intel_sub_group_shuffle( as_uchar(x), c ) );
}
#endif // cl_intel_subgroups_char

INLINE float OVERLOADABLE intel_sub_group_shuffle( float x, uint c )
{
    return __builtin_IB_simd_shuffle_f( x, c );
}

INLINE uint OVERLOADABLE intel_sub_group_shuffle( uint x, uint c )
{
    return __builtin_IB_simd_shuffle( x, c );
}

INLINE int OVERLOADABLE intel_sub_group_shuffle( int x, uint c )
{
    return as_int( intel_sub_group_shuffle( as_uint(x), c ) );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, float, float, uint )
#ifdef cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, char, char, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, uchar, uchar, uint )
#endif // cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, short, short, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, ushort, ushort, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, int, int, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, uint, uint, uint )

INLINE long OVERLOADABLE intel_sub_group_shuffle( long x, uint c )
{
    return as_long( intel_sub_group_shuffle( as_uint2(x), c ) );
}

INLINE ulong OVERLOADABLE intel_sub_group_shuffle( ulong x, uint c )
{
    return as_ulong( intel_sub_group_shuffle( as_uint2(x), c ) );
}

// Experimental support for 8-bit and 16-bit integer types:

INLINE uint OVERLOADABLE intel_sub_group_shuffle_down( uint cur, uint next, uint c )
{
    return __builtin_IB_simd_shuffle_down( cur, next, c );
}

INLINE int OVERLOADABLE intel_sub_group_shuffle_down( int cur, int next, uint c )
{
    return as_int( intel_sub_group_shuffle_down( as_uint(cur), as_uint(next), c ) );
}

INLINE ushort OVERLOADABLE intel_sub_group_shuffle_down( ushort cur, ushort next, uint c )
{
    return __builtin_IB_simd_shuffle_down_us( cur, next, c );
}

INLINE short OVERLOADABLE intel_sub_group_shuffle_down( short cur, short next, uint c )
{
    return as_short( intel_sub_group_shuffle_down( as_ushort(cur), as_ushort(next), c ) );
}

#ifdef cl_intel_subgroups_char
INLINE uchar OVERLOADABLE intel_sub_group_shuffle_down( uchar cur, uchar next, uint c )
{
    return __builtin_IB_simd_shuffle_down_uc( cur, next, c );
}

INLINE char OVERLOADABLE intel_sub_group_shuffle_down( char cur, char next, uint c )
{
    return as_char( intel_sub_group_shuffle_down( as_uchar(cur), as_uchar(next), c ) );
}
#endif // cl_intel_subgroups_char

INLINE float OVERLOADABLE intel_sub_group_shuffle_down( float cur, float next, uint c )
{
    return as_float( intel_sub_group_shuffle_down( as_uint(cur), as_uint(next), c ) );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, float, float, uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, short, short, uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, ushort, ushort, uint )
#ifdef cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, char, char, uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, uchar, uchar, uint )
#endif // cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, int, int, uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, uint, uint, uint )

INLINE long OVERLOADABLE intel_sub_group_shuffle_down( long cur, long next, uint c )
{
    return as_long( intel_sub_group_shuffle_down( as_int2(cur), as_int2(next), c ) );
}

INLINE ulong OVERLOADABLE intel_sub_group_shuffle_down( ulong cur, ulong next, uint c )
{
    return as_ulong( intel_sub_group_shuffle_down( as_int2(cur), as_int2(next), c ) );
}


#define DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(TYPE)                                                 \
INLINE TYPE OVERLOADABLE  intel_sub_group_shuffle_up( TYPE prev, TYPE cur, uint c )           \
{                                                                                             \
    c = get_max_sub_group_size() - c;                                                         \
    return intel_sub_group_shuffle_down( prev, cur, c );                                      \
}

#ifdef cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(uchar)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(char)
#endif // cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(ushort)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(short)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(uint)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(int)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(float)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(ulong)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(long)

GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_up, float, float, uint )
#ifdef cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_up, char, char, uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_up, uchar, uchar, uint )
#endif // cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_up, short, short, uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_up, ushort, ushort, uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_up, int, int, uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_up, uint, uint, uint )


#define DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(TYPE)                                  \
INLINE TYPE OVERLOADABLE  intel_sub_group_shuffle_xor( TYPE x, uint c )         \
{                                                                               \
    c = get_sub_group_local_id() ^ c;                                           \
    return intel_sub_group_shuffle( x, c );                                     \
}

#ifdef cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(uchar)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(char)
#endif // cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(ushort)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(short)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(uint)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(int)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(float)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(ulong)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(long)

GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, float, float, uint )
#ifdef cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, char, char, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, uchar, uchar, uint )
#endif // cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, short, short, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, ushort, ushort, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, int, int, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, uint, uint, uint )

#ifdef cl_khr_fp16

DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(half)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, half, half, uint )

#define SHUFFLE_AS_INTEGER_2_1(FNAME)\
  INLINE half OVERLOADABLE FNAME( half x, half y, uint c )\
  {\
      return as_half( FNAME( as_ushort(x), as_ushort(y), c ) );\
  }

#define SHUFFLE_AS_INTEGER_2_2(FNAME)\
  INLINE half2 OVERLOADABLE FNAME( half2 x, half2 y, uint c )\
  {\
      return as_half2( FNAME( as_uint(x), as_uint(y), c ) );\
  }

#define SHUFFLE_AS_INTEGER_2_4(FNAME)\
  INLINE half4 OVERLOADABLE FNAME( half4 x, half4 y, uint c )\
  {\
      return as_half4( FNAME( as_uint2(x), as_uint2(y), c ) );\
  }

#define SHUFFLE_AS_INTEGER_2_8(FNAME)\
  INLINE half8 OVERLOADABLE FNAME( half8 x, half8 y, uint c )\
  {\
      return as_half8( FNAME( as_uint4(x), as_uint4(y), c ) );\
  }

#define SHUFFLE_AS_INTEGER_2_16(FNAME)\
  INLINE half16 OVERLOADABLE FNAME( half16 x, half16 y, uint c )\
  {\
      return as_half16( FNAME( as_uint8(x), as_uint8(y), c ) );\
  }

#define OVERLOAD_AS_INTEGERS_2(FNAME)\
  SHUFFLE_AS_INTEGER_2_1(FNAME)\
  SHUFFLE_AS_INTEGER_2_2(FNAME)\
  SHUFFLE_AS_INTEGER_2_4(FNAME)\
  SHUFFLE_AS_INTEGER_2_8(FNAME)\
  SHUFFLE_AS_INTEGER_2_16(FNAME)

OVERLOAD_AS_INTEGERS_2(intel_sub_group_shuffle_down)
OVERLOAD_AS_INTEGERS_2(intel_sub_group_shuffle_up)

#undef OVERLOAD_AS_INTEGERS_2
#undef SHUFFLE_AS_INTEGER_2_1
#undef SHUFFLE_AS_INTEGER_2_2
#undef SHUFFLE_AS_INTEGER_2_4
#undef SHUFFLE_AS_INTEGER_2_8
#undef SHUFFLE_AS_INTEGER_2_16

#endif // cl_khr_fp16

#if defined(_metal_simdgroups)
// 8-bit types implementation for shuffle
INLINE uchar OVERLOADABLE intel_sub_group_shuffle( uchar x, uint c )
{
    return (uchar)(intel_sub_group_shuffle((ushort)(x),c));
}

INLINE char OVERLOADABLE intel_sub_group_shuffle( char x, uint c )
{
    return as_char(intel_sub_group_shuffle(as_uchar(x),c));
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, char, char, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, uchar, uchar, uint )
#endif

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE intel_sub_group_shuffle( double x, uint c )
{
    return __builtin_IB_simd_shuffle_df( x, c );
}

INLINE double OVERLOADABLE intel_sub_group_shuffle_down( double cur, double next, uint c )
{
    return as_double( intel_sub_group_shuffle_down( as_uint2(cur), as_uint2(next), c ) );
}

INLINE double OVERLOADABLE intel_sub_group_shuffle_up( double prev, double cur, uint c )
{
    c = get_max_sub_group_size() - c;
    return intel_sub_group_shuffle_down( prev, cur, c );
}

INLINE double OVERLOADABLE intel_sub_group_shuffle_xor( double x, uint c )
{
    c = get_sub_group_local_id() ^ c;
    return intel_sub_group_shuffle( x, c );
}

#endif // defined(cl_khr_fp64)

#define  DEFN_SUB_GROUP_BROADCAST(TYPE, TYPE_ABBR)                                         \
INLINE TYPE OVERLOADABLE  sub_group_broadcast( TYPE x, uint sub_group_local_id )           \
{                                                                                          \
    uint3 local_id = (uint3)(sub_group_local_id,0,0);                                      \
    return __builtin_spirv_OpGroupBroadcast_i32_##TYPE_ABBR##_v3i32(Subgroup,x,local_id);  \
}

#define  DEFN_INTEL_SUB_GROUP_BROADCAST(TYPE, TYPE_ABBR)                                   \
INLINE TYPE OVERLOADABLE  intel_sub_group_broadcast( TYPE x, uint sub_group_local_id )     \
{                                                                                          \
    uint3 local_id = (uint3)(sub_group_local_id,0,0);                                      \
    return __builtin_spirv_OpGroupBroadcast_i32_##TYPE_ABBR##_v3i32(Subgroup,x,local_id);  \
}

DEFN_SUB_GROUP_BROADCAST(int,    i32)
DEFN_SUB_GROUP_BROADCAST(uint,   i32)
DEFN_SUB_GROUP_BROADCAST(long,   i64)
DEFN_SUB_GROUP_BROADCAST(ulong,  i64)
DEFN_SUB_GROUP_BROADCAST(float,  f32)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_BROADCAST(half,   f16)
#endif // cl_khr_fp16
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_BROADCAST(double, f64)
#endif // cl_khr_fp64

#ifdef cl_intel_subgroups_char
DEFN_SUB_GROUP_BROADCAST(char,   i8)
#endif // cl_intel_subgroups_char

#if defined(cl_intel_subgroups_short)
DEFN_INTEL_SUB_GROUP_BROADCAST(short,  i16)
DEFN_INTEL_SUB_GROUP_BROADCAST(ushort, i16)
#endif // cl_intel_subgroups_short

#ifdef cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_BROADCAST(char,  i8)
DEFN_INTEL_SUB_GROUP_BROADCAST(uchar, i8)
#endif // cl_intel_subgroups_char

INLINE int OVERLOADABLE sub_group_all( int predicate )
{
    return __builtin_spirv_OpGroupAll_i32_i1(Subgroup, predicate);
}

INLINE int OVERLOADABLE sub_group_any( int predicate )
{
    return __builtin_spirv_OpGroupAny_i32_i1(Subgroup, predicate);
}

/////////////////////////////////////////////////////////////////////////////////////
// Media block read/write extension

#define DEFN_MEDIA_BLOCK_READ_RO(TYPE, TYPE_POSTFIX, TYPE_ABBR, LEN)    \
OVERLOADABLE INLINE                                                     \
TYPE##LEN intel_sub_group_media_block_read_##TYPE_POSTFIX##LEN(         \
    int2 src_offset,                                                    \
    int width,                                                          \
    int height,                                                         \
    read_only image2d_t image)                                          \
{                                                                       \
    int id = (int)__builtin_astype(image, global void*);                \
    return __builtin_IB_media_block_read_##TYPE##LEN(                   \
        id, src_offset, width, height);                                 \
}                                                                       \
TYPE##LEN __builtin_spirv_intel_sub_group_media_block_read_             \
##TYPE_POSTFIX##LEN##_i64_v2i32_i32_i32(                                \
    ulong image, int2 src_offset, int width, int height)                \
{                                                                       \
    return __builtin_IB_media_block_read_##TYPE##LEN(                   \
        image, src_offset, width, height);                              \
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
#define DEFN_MEDIA_BLOCK_READ_RW(TYPE, TYPE_POSTFIX, TYPE_ABBR, LEN)    \
OVERLOADABLE INLINE                                                     \
TYPE##LEN intel_sub_group_media_block_read_##TYPE_POSTFIX##LEN(         \
    int2 src_offset,                                                    \
    int width,                                                          \
    int height,                                                         \
    read_write image2d_t image)                                         \
{                                                                       \
    int id = (int)__builtin_astype(image, global void*);                \
    return __builtin_IB_media_block_read_##TYPE##LEN(                   \
        id, src_offset, width, height);                                 \
}
#endif


#define DEFN_MEDIA_BLOCK_WRITE_WO(TYPE, TYPE_POSTFIX, TYPE_ABBR, LEN)       \
OVERLOADABLE INLINE                                                         \
void intel_sub_group_media_block_write_##TYPE_POSTFIX##LEN(                 \
    int2 src_offset,                                                        \
    int width,                                                              \
    int height,                                                             \
    TYPE##LEN pixels,                                                       \
    write_only image2d_t image)                                             \
{                                                                           \
    int id = (int)__builtin_astype(image, global void*);                    \
    __builtin_IB_media_block_write_##TYPE##LEN(                             \
        id, src_offset, width, height, pixels);                             \
}                                                                           \
void __builtin_spirv_intel_sub_group_media_block_write_                     \
##TYPE_POSTFIX##LEN##_i64_v2i32_i32_i32_##TYPE_ABBR(                        \
    ulong image, int2 src_offset, int width, int height, TYPE##LEN pixels)  \
{                                                                           \
    __builtin_IB_media_block_write_##TYPE##LEN(                             \
        image, src_offset, width, height, pixels);                          \
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
#define DEFN_MEDIA_BLOCK_WRITE_RW(TYPE, TYPE_POSTFIX, TYPE_ABBR, LEN)   \
OVERLOADABLE INLINE                                                     \
void intel_sub_group_media_block_write_##TYPE_POSTFIX##LEN(             \
    int2 src_offset,                                                    \
    int width,                                                          \
    int height,                                                         \
    TYPE##LEN pixels,                                                   \
    read_write image2d_t image)                                         \
{                                                                       \
    int id = (int)__builtin_astype(image, global void*);                \
    __builtin_IB_media_block_write_##TYPE##LEN(                         \
        id, src_offset, width, height, pixels);                         \
}
#endif


#define DEFN_MEDIA_BLOCK(OPERATION, TYPE, TYPE_POSTFIX, TYPE_ABBR)      \
DEFN_MEDIA_BLOCK_##OPERATION(TYPE, TYPE_POSTFIX, TYPE_ABBR,  )          \
DEFN_MEDIA_BLOCK_##OPERATION(TYPE, TYPE_POSTFIX, v2##TYPE_ABBR, 2)          \
DEFN_MEDIA_BLOCK_##OPERATION(TYPE, TYPE_POSTFIX, v4##TYPE_ABBR, 4)          \
DEFN_MEDIA_BLOCK_##OPERATION(TYPE, TYPE_POSTFIX, v8##TYPE_ABBR, 8)          \
MEDIA_IO_HAS16( DEFN_MEDIA_BLOCK_##OPERATION(TYPE, TYPE_POSTFIX, v16##TYPE_ABBR, 16) )

#define MEDIA_IO_HAS16(x) x

DEFN_MEDIA_BLOCK(READ_RO,  uchar,  uc, i8)
DEFN_MEDIA_BLOCK(READ_RO,  ushort, us, i16)
DEFN_MEDIA_BLOCK(WRITE_WO, uchar,  uc, i8)
DEFN_MEDIA_BLOCK(WRITE_WO, ushort, us, i16)
#if SUPPORT_ACCESS_QUAL_OVERLOAD
DEFN_MEDIA_BLOCK(READ_RW,  uchar,  uc, i8)
DEFN_MEDIA_BLOCK(READ_RW,  ushort, us, i16)
DEFN_MEDIA_BLOCK(WRITE_RW, uchar,  uc, i8)
DEFN_MEDIA_BLOCK(WRITE_RW, ushort, us, i16)
#endif

// Integer block read/writes don't have 16 element version.
#undef MEDIA_IO_HAS16
#define MEDIA_IO_HAS16(x)
DEFN_MEDIA_BLOCK(READ_RO,  uint, ui, i32)
DEFN_MEDIA_BLOCK(WRITE_WO, uint, ui, i32)
#if SUPPORT_ACCESS_QUAL_OVERLOAD
DEFN_MEDIA_BLOCK(READ_RW,  uint, ui, i32)
DEFN_MEDIA_BLOCK(WRITE_RW, uint, ui, i32)
#endif
#undef MEDIA_IO_HAS16

/////////////////////////////////////////////////////////////////////////////////////
// Block Read/Write Functions

#define  DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(FUNC_NAME, TYPE, INTERNAL_FUNC)          \
INLINE TYPE OVERLOADABLE  FUNC_NAME( image2d_t image, int2 coord )                      \
{                                                                                       \
    int id = (int)__builtin_astype(image, __global void*);                              \
    return INTERNAL_FUNC(id, coord);                                                    \
}                                                                                       \
TYPE __builtin_spirv_##FUNC_NAME##_i64_v2i32(ulong image, int2 coord)                   \
{                                                                                       \
    return INTERNAL_FUNC(image, coord);                                                 \
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
#define  DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(FUNC_NAME, TYPE, INTERNAL_FUNC)       \
INLINE TYPE OVERLOADABLE  FUNC_NAME( read_write image2d_t image, int2 coord )           \
{                                                                                       \
    int id = (int)__builtin_astype(image, __global void*);                              \
    return INTERNAL_FUNC(id, coord);                                                    \
}
#else
#define  DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(FUNC_NAME, TYPE, INTERNAL_FUNC)
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

#define  DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(FUNC_NAME, TYPE, ELEM_TYPE, TYPE_ABBR, INTERNAL_FUNC)       \
INLINE TYPE OVERLOADABLE  FUNC_NAME( const __global ELEM_TYPE* p )                                          \
{                                                                                                           \
    return INTERNAL_FUNC((__global void *)p);                                                               \
}                                                                                                           \
TYPE __builtin_spirv_##FUNC_NAME##_p1##TYPE_ABBR(const __global ELEM_TYPE* p)                               \
{                                                                                                           \
    return INTERNAL_FUNC((__global void *)p);                                                               \
}


#if SUPPORT_ACCESS_QUAL_OVERLOAD
#define  DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(FUNC_NAME, TYPE, TYPE_ABBR, INTERNAL_FUNC)   \
INLINE void OVERLOADABLE FUNC_NAME( write_only image2d_t image, int2 coord, TYPE data )         \
{                                                                                               \
    int id = (int)__builtin_astype(image, __global void*);                                      \
    INTERNAL_FUNC(id, coord, data);                                                             \
}                                                                                               \
TYPE __builtin_spirv_##FUNC_NAME##_i64_v2i32_##TYPE_ABBR(ulong image, int2 coord, TYPE data)    \
{                                                                                               \
    INTERNAL_FUNC(image, coord, data);                                                          \
}

#else
#define  DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(FUNC_NAME, TYPE, TYPE_ABBR, INTERNAL_FUNC)
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

#if SUPPORT_ACCESS_QUAL_OVERLOAD
#define  DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(FUNC_NAME, TYPE, INTERNAL_FUNC)     \
INLINE void OVERLOADABLE FUNC_NAME( read_write image2d_t image, int2 coord, TYPE data )\
{                                                                                      \
    int id = (int)__builtin_astype(image, __global void*);                             \
    INTERNAL_FUNC(id, coord, data);                                                    \
}
#else
#define  DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(FUNC_NAME, TYPE, INTERNAL_FUNC)
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

#define  DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(FUNC_NAME, TYPE, ELEM_TYPE, PTR_TYPE, TYPE_ABBR, INTERNAL_FUNC)    \
INLINE void OVERLOADABLE  FUNC_NAME( __global ELEM_TYPE* p, TYPE data )                                             \
{                                                                                                                   \
    INTERNAL_FUNC(p, data);                                                                                         \
}                                                                                                                   \
TYPE __builtin_spirv_##FUNC_NAME##_p1##PTR_TYPE##_##TYPE_ABBR(__global ELEM_TYPE* p, TYPE data)                     \
{                                                                                                                   \
    INTERNAL_FUNC(p, data);                                                                                         \
}


DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read,  uint,  __builtin_IB_simd_media_block_read_1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read2, uint2, __builtin_IB_simd_media_block_read_2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read4, uint4, __builtin_IB_simd_media_block_read_4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read8, uint8, __builtin_IB_simd_media_block_read_8)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read,  uint,  __builtin_IB_simd_media_block_read_1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read2, uint2, __builtin_IB_simd_media_block_read_2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read4, uint4, __builtin_IB_simd_media_block_read_4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read8, uint8, __builtin_IB_simd_media_block_read_8)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read,  uint,  uint, i32, __builtin_IB_simd_block_read_1_global)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read2, uint2, uint, i32, __builtin_IB_simd_block_read_2_global)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read4, uint4, uint, i32, __builtin_IB_simd_block_read_4_global)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read8, uint8, uint, i32, __builtin_IB_simd_block_read_8_global)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write,  uint,  i32,   __builtin_IB_simd_media_block_write_1)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write2, uint2, v2i32, __builtin_IB_simd_media_block_write_2)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write4, uint4, v4i32, __builtin_IB_simd_media_block_write_4)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write8, uint8, v8i32, __builtin_IB_simd_media_block_write_8)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write,  uint,  __builtin_IB_simd_media_block_write_1)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write2, uint2, __builtin_IB_simd_media_block_write_2)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write4, uint4, __builtin_IB_simd_media_block_write_4)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write8, uint8, __builtin_IB_simd_media_block_write_8)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write,  uint,  uint, i32, i32, __builtin_IB_simd_block_write_1_global)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write2, uint2, uint, i32, v2i32, __builtin_IB_simd_block_write_2_global)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write4, uint4, uint, i32, v4i32, __builtin_IB_simd_block_write_4_global)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write8, uint8, uint, i32, v8i32, __builtin_IB_simd_block_write_8_global)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_us,   ushort,  __builtin_IB_simd_media_block_read_1_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_us2,  ushort2, __builtin_IB_simd_media_block_read_2_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_us4,  ushort4, __builtin_IB_simd_media_block_read_4_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_us8,  ushort8, __builtin_IB_simd_media_block_read_8_h)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_us,   ushort,  __builtin_IB_simd_media_block_read_1_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_us2,  ushort2, __builtin_IB_simd_media_block_read_2_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_us4,  ushort4, __builtin_IB_simd_media_block_read_4_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_us8,  ushort8, __builtin_IB_simd_media_block_read_8_h)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_us,  ushort,  ushort, i16, __builtin_IB_simd_block_read_1_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_us2, ushort2, ushort, i16, __builtin_IB_simd_block_read_2_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_us4, ushort4, ushort, i16, __builtin_IB_simd_block_read_4_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_us8, ushort8, ushort, i16, __builtin_IB_simd_block_read_8_global_h)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_us,  ushort,  i16,   __builtin_IB_simd_media_block_write_1_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_us2, ushort2, v2i16, __builtin_IB_simd_media_block_write_2_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_us4, ushort4, v4i16, __builtin_IB_simd_media_block_write_4_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_us8, ushort8, v8i16, __builtin_IB_simd_media_block_write_8_h)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_us,  ushort,  __builtin_IB_simd_media_block_write_1_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_us2, ushort2, __builtin_IB_simd_media_block_write_2_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_us4, ushort4, __builtin_IB_simd_media_block_write_4_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_us8, ushort8, __builtin_IB_simd_media_block_write_8_h)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_us,  ushort,  ushort, i16, i16, __builtin_IB_simd_block_write_1_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_us2, ushort2, ushort, i16, v2i16, __builtin_IB_simd_block_write_2_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_us4, ushort4, ushort, i16, v4i16, __builtin_IB_simd_block_write_4_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_us8, ushort8, ushort, i16, v8i16, __builtin_IB_simd_block_write_8_global_h)

#ifdef cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_uc,   uchar,  __builtin_IB_simd_media_block_read_1_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_uc2,  uchar2, __builtin_IB_simd_media_block_read_2_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_uc4,  uchar4, __builtin_IB_simd_media_block_read_4_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_uc8,  uchar8, __builtin_IB_simd_media_block_read_8_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_uc16,  uchar16, __builtin_IB_simd_media_block_read_16_b)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_uc,   uchar,  __builtin_IB_simd_media_block_read_1_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_uc2,  uchar2, __builtin_IB_simd_media_block_read_2_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_uc4,  uchar4, __builtin_IB_simd_media_block_read_4_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_uc8,  uchar8, __builtin_IB_simd_media_block_read_8_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_uc16,  uchar16, __builtin_IB_simd_media_block_read_16_b)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_uc,  uchar,  uchar, i8, __builtin_IB_simd_block_read_1_global_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_uc2, uchar2, uchar, i8, __builtin_IB_simd_block_read_2_global_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_uc4, uchar4, uchar, i8, __builtin_IB_simd_block_read_4_global_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_uc8, uchar8, uchar, i8, __builtin_IB_simd_block_read_8_global_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_uc16, uchar16, uchar, i8, __builtin_IB_simd_block_read_16_global_b)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_uc,  uchar,  i8, __builtin_IB_simd_media_block_write_1_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_uc2, uchar2, v2i8, __builtin_IB_simd_media_block_write_2_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_uc4, uchar4, v4i8, __builtin_IB_simd_media_block_write_4_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_uc8, uchar8, v8i8, __builtin_IB_simd_media_block_write_8_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_uc16, uchar16, v16i8, __builtin_IB_simd_media_block_write_16_b)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_uc,  uchar,  __builtin_IB_simd_media_block_write_1_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_uc2, uchar2, __builtin_IB_simd_media_block_write_2_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_uc4, uchar4, __builtin_IB_simd_media_block_write_4_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_uc8, uchar8, __builtin_IB_simd_media_block_write_8_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_uc16, uchar16, __builtin_IB_simd_media_block_write_16_b)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_uc,  uchar,  uchar, i8, i8, __builtin_IB_simd_block_write_1_global_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_uc2, uchar2, uchar, i8, v2i8, __builtin_IB_simd_block_write_2_global_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_uc4, uchar4, uchar, i8, v4i8, __builtin_IB_simd_block_write_4_global_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_uc8, uchar8, uchar, i8, v8i8, __builtin_IB_simd_block_write_8_global_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_uc16, uchar16, uchar, i8, v16i8, __builtin_IB_simd_block_write_16_global_b)
#endif // cl_intel_subgroups_char

#ifdef cl_intel_subgroups_long
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_ul,   ulong,  __builtin_IB_simd_media_block_read_1_l)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_ul2,  ulong2, __builtin_IB_simd_media_block_read_2_l)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_ul4,  ulong4, __builtin_IB_simd_media_block_read_4_l)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_ul8,  ulong8, __builtin_IB_simd_media_block_read_8_l)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_ul,   ulong,  __builtin_IB_simd_media_block_read_1_l)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_ul2,  ulong2, __builtin_IB_simd_media_block_read_2_l)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_ul4,  ulong4, __builtin_IB_simd_media_block_read_4_l)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_ul8,  ulong8, __builtin_IB_simd_media_block_read_8_l)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_ul,  ulong,  ulong, i64, __builtin_IB_simd_block_read_1_global_l)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_ul2, ulong2, ulong, i64, __builtin_IB_simd_block_read_2_global_l)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_ul4, ulong4, ulong, i64, __builtin_IB_simd_block_read_4_global_l)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_ul8, ulong8, ulong, i64, __builtin_IB_simd_block_read_8_global_l)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_ul,  ulong,  i64,   __builtin_IB_simd_media_block_write_1_l)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_ul2, ulong2, v2i64, __builtin_IB_simd_media_block_write_2_l)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_ul4, ulong4, v4i64, __builtin_IB_simd_media_block_write_4_l)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_ul8, ulong8, v8i64, __builtin_IB_simd_media_block_write_8_l)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_ul,  ulong,  __builtin_IB_simd_media_block_write_1_l)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_ul2, ulong2, __builtin_IB_simd_media_block_write_2_l)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_ul4, ulong4, __builtin_IB_simd_media_block_write_4_l)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_ul8, ulong8, __builtin_IB_simd_media_block_write_8_l)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_ul,  ulong,  ulong, i64, i64, __builtin_IB_simd_block_write_1_global_l)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_ul2, ulong2, ulong, i64, v2i64, __builtin_IB_simd_block_write_2_global_l)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_ul4, ulong4, ulong, i64, v4i64, __builtin_IB_simd_block_write_4_global_l)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_ul8, ulong8, ulong, i64, v8i64, __builtin_IB_simd_block_write_8_global_l)
#endif // cl_intel_subgroups_long



// SPIR-V builtins implementation for intel_subgroups

// Second param is always an uint, so take only first as an argument to the macro.
#define DEFN_SPIRV_INTEL_SUBGROUP_2PARAMS(FUNC_NAME, TYPE_ABBR, TYPE)       \
TYPE __builtin_spirv_##FUNC_NAME##_##TYPE_ABBR##_i32(TYPE x, uint y) {      \
    return FUNC_NAME(x,y);                                                  \
}

#define DEFN_SPIRV_INTEL_SUBGROUP_3PARAMS(FUNC_NAME, TYPE_ABBR, TYPE) \
TYPE __builtin_spirv_##FUNC_NAME##_##TYPE_ABBR##_##TYPE_ABBR##_i32(TYPE x, TYPE y, uint z) {    \
    return FUNC_NAME(x,y,z);                                                                    \
}

#define DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(TYPE_ABBR, TYPE)                    \
    DEFN_SPIRV_INTEL_SUBGROUP_2PARAMS(intel_sub_group_shuffle, TYPE_ABBR, TYPE)         \
    DEFN_SPIRV_INTEL_SUBGROUP_2PARAMS(intel_sub_group_shuffle_xor, TYPE_ABBR, TYPE)     \
    DEFN_SPIRV_INTEL_SUBGROUP_3PARAMS(intel_sub_group_shuffle_down, TYPE_ABBR, TYPE)    \
    DEFN_SPIRV_INTEL_SUBGROUP_3PARAMS(intel_sub_group_shuffle_up, TYPE_ABBR, TYPE)

DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(i32, uint)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(i8, uchar)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(i16, ushort)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(f32, float)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v2f32, float2)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v3f32, float3)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v4f32, float4)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v8f32, float8)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v16f32, float16)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v2i32, uint2)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v3i32, uint3)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v4i32, uint4)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v8i32, uint8)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v16i32, uint16)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v2i16, ushort2)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v3i16, ushort3)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v4i16, ushort4)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v8i16, ushort8)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v16i16, ushort16)
#ifdef cl_intel_subgroups_char
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v2i8, uchar2)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v3i8, uchar3)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v4i8, uchar4)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v8i8, uchar8)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(v16i8, uchar16)
#endif // cl_intel_subgroups_char

DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(i64, ulong)
#if defined(cl_khr_fp16)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(f16, half)
#endif
#if defined(cl_khr_fp64)
DEFN_SPIRV_INTEL_SUBGROUP_SHUFFLE_FUNCTIONS(f64, double)
#endif
