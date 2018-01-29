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
    // Nothing!
}

#if(__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE void OVERLOADABLE sub_group_barrier( cl_mem_fence_flags flags, memory_scope scope )
{
    // Nothing!
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0


INLINE ushort OVERLOADABLE intel_sub_group_shuffle( ushort x, uint c )
{
    return as_ushort( __builtin_IB_simd_shuffle_h( as_half(x), c ) );
}

INLINE short OVERLOADABLE intel_sub_group_shuffle( short x, uint c )
{
    return as_short( intel_sub_group_shuffle( as_ushort(x), c ) );
}

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

INLINE float OVERLOADABLE intel_sub_group_shuffle_down( float cur, float next, uint c )
{
    return as_float( intel_sub_group_shuffle_down( as_uint(cur), as_uint(next), c ) );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, float, float, uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, short, short, uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, ushort, ushort, uint )
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

DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(ushort)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(short)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(uint)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(int)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(float)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(ulong)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(long)

GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_up, float, float, uint )
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

DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(ushort)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(short)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(uint)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(int)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(float)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(ulong)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(long)

GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, float, float, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, short, short, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, ushort, ushort, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, int, int, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, uint, uint, uint )

#ifdef cl_khr_fp16
#define SHUFFLE_AS_INTEGER_1_1(FNAME)\
  INLINE half OVERLOADABLE FNAME( half x, uint c )\
  {\
      return as_half( FNAME( as_ushort(x), c ) );\
  }

#define SHUFFLE_AS_INTEGER_1_2(FNAME)\
  INLINE half2 OVERLOADABLE FNAME( half2 x, uint c )\
  {\
      return as_half2( FNAME( as_uint(x), c ) );\
  }

#define SHUFFLE_AS_INTEGER_1_4(FNAME)\
  INLINE half4 OVERLOADABLE FNAME( half4 x, uint c )\
  {\
      return as_half4( FNAME( as_uint2(x), c ) );\
  }

#define SHUFFLE_AS_INTEGER_1_8(FNAME)\
  INLINE half8 OVERLOADABLE FNAME( half8 x, uint c )\
  {\
      return as_half8( FNAME( as_uint4(x), c ) );\
  }

#define SHUFFLE_AS_INTEGER_1_16(FNAME)\
  INLINE half16 OVERLOADABLE FNAME( half16 x, uint c )\
  {\
      return as_half16( FNAME( as_uint8(x), c ) );\
  }

#define OVERLOAD_AS_INTEGERS_1(FNAME)\
  SHUFFLE_AS_INTEGER_1_1(FNAME)\
  SHUFFLE_AS_INTEGER_1_2(FNAME)\
  SHUFFLE_AS_INTEGER_1_4(FNAME)\
  SHUFFLE_AS_INTEGER_1_8(FNAME)\
  SHUFFLE_AS_INTEGER_1_16(FNAME)

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

OVERLOAD_AS_INTEGERS_1(intel_sub_group_shuffle)
OVERLOAD_AS_INTEGERS_2(intel_sub_group_shuffle_down)
OVERLOAD_AS_INTEGERS_2(intel_sub_group_shuffle_up)
OVERLOAD_AS_INTEGERS_1(intel_sub_group_shuffle_xor)

#undef OVERLOAD_AS_INTEGERS_1
#undef SHUFFLE_AS_INTEGER_1_1
#undef SHUFFLE_AS_INTEGER_1_2
#undef SHUFFLE_AS_INTEGER_1_4
#undef SHUFFLE_AS_INTEGER_1_8
#undef SHUFFLE_AS_INTEGER_1_16

#undef OVERLOAD_AS_INTEGERS_2
#undef SHUFFLE_AS_INTEGER_2_1
#undef SHUFFLE_AS_INTEGER_2_2
#undef SHUFFLE_AS_INTEGER_2_4
#undef SHUFFLE_AS_INTEGER_2_8
#undef SHUFFLE_AS_INTEGER_2_16

#if defined(_metal_simdgroups)
// half3 implementation for shuffle (as ushort)
INLINE half3 OVERLOADABLE intel_sub_group_shuffle( half3 x, uint c )
{
    return as_half3( intel_sub_group_shuffle( as_ushort3(x), c ) );
}
#endif

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
    return as_double( intel_sub_group_shuffle( as_uint2(x), c ) );
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

#if defined(cl_intel_subgroups_short)
DEFN_INTEL_SUB_GROUP_BROADCAST(short,  i16)
DEFN_INTEL_SUB_GROUP_BROADCAST(ushort, i16)
#endif // cl_intel_subgroups_short


INLINE int OVERLOADABLE sub_group_all( int predicate )
{
    int value = ( predicate == 0 ) ? 1 : 0;
    value = sub_group_reduce_add( value );
    value = ( value == 0 ) ? 1 : 0;
    return value;
}

INLINE int OVERLOADABLE sub_group_any( int predicate )
{
    int value = ( predicate != 0 ) ? 1 : 0;
    value = sub_group_reduce_add( value );
    return value;
}

/////////////////////////////////////////////////////////////////////////////////////
//
// Media block read/write extension
//

//////////// Reads ////////////

// uchar

#define CALL(TYPE, LEN)                                  \
    int id = (int)__builtin_astype(image, global void*); \
    return __builtin_IB_media_block_read_##TYPE##LEN(    \
        id, src_offset, width, height);

OVERLOADABLE INLINE
uchar intel_sub_group_media_block_read_uc(
    int2 src_offset,
    int width,
    int height,
    read_only image2d_t image)
{
    CALL(uchar,)
}

OVERLOADABLE INLINE
uchar2 intel_sub_group_media_block_read_uc2(
    int2 src_offset,
    int width,
    int height,
    read_only image2d_t image)
{
    CALL(uchar, 2)
}

OVERLOADABLE INLINE
uchar4 intel_sub_group_media_block_read_uc4(
    int2 src_offset,
    int width,
    int height,
    read_only image2d_t image)
{
    CALL(uchar, 4)
}

OVERLOADABLE INLINE
uchar8 intel_sub_group_media_block_read_uc8(
    int2 src_offset,
    int width,
    int height,
    read_only image2d_t image)
{
    CALL(uchar, 8)
}

OVERLOADABLE INLINE
uchar16 intel_sub_group_media_block_read_uc16(
    int2 src_offset,
    int width,
    int height,
    read_only image2d_t image)
{
    CALL(uchar, 16)
}

// ushort

OVERLOADABLE INLINE
ushort intel_sub_group_media_block_read_us(
    int2 src_offset,
    int width,
    int height,
    read_only image2d_t image)
{
    CALL(ushort,)
}

OVERLOADABLE INLINE
ushort2 intel_sub_group_media_block_read_us2(
    int2 src_offset,
    int width,
    int height,
    read_only image2d_t image)
{
    CALL(ushort, 2)
}

OVERLOADABLE INLINE
ushort4 intel_sub_group_media_block_read_us4(
    int2 src_offset,
    int width,
    int height,
    read_only image2d_t image)
{
    CALL(ushort, 4)
}

OVERLOADABLE INLINE
ushort8 intel_sub_group_media_block_read_us8(
    int2 src_offset,
    int width,
    int height,
    read_only image2d_t image)
{
    CALL(ushort, 8)
}

OVERLOADABLE INLINE
ushort16 intel_sub_group_media_block_read_us16(
    int2 src_offset,
    int width,
    int height,
    read_only image2d_t image)
{
    CALL(ushort, 16)
}

// uint

OVERLOADABLE INLINE
uint intel_sub_group_media_block_read_ui(
    int2 src_offset,
    int width,
    int height,
    read_only image2d_t image)
{
    CALL(uint,)
}

OVERLOADABLE INLINE
uint2 intel_sub_group_media_block_read_ui2(
    int2 src_offset,
    int width,
    int height,
    read_only image2d_t image)
{
    CALL(uint, 2)
}

OVERLOADABLE INLINE
uint4 intel_sub_group_media_block_read_ui4(
    int2 src_offset,
    int width,
    int height,
    read_only image2d_t image)
{
    CALL(uint, 4)
}

OVERLOADABLE INLINE
uint8 intel_sub_group_media_block_read_ui8(
    int2 src_offset,
    int width,
    int height,
    read_only image2d_t image)
{
    CALL(uint, 8)
}

// read_write variants //

#if SUPPORT_ACCESS_QUAL_OVERLOAD
OVERLOADABLE INLINE
uchar intel_sub_group_media_block_read_uc(
    int2 src_offset,
    int width,
    int height,
    read_write image2d_t image)
{
    CALL(uchar,)
}

OVERLOADABLE INLINE
uchar2 intel_sub_group_media_block_read_uc2(
    int2 src_offset,
    int width,
    int height,
    read_write image2d_t image)
{
    CALL(uchar, 2)
}

OVERLOADABLE INLINE
uchar4 intel_sub_group_media_block_read_uc4(
    int2 src_offset,
    int width,
    int height,
    read_write image2d_t image)
{
    CALL(uchar, 4)
}

OVERLOADABLE INLINE
uchar8 intel_sub_group_media_block_read_uc8(
    int2 src_offset,
    int width,
    int height,
    read_write image2d_t image)
{
    CALL(uchar, 8)
}

OVERLOADABLE INLINE
uchar16 intel_sub_group_media_block_read_uc16(
    int2 src_offset,
    int width,
    int height,
    read_write image2d_t image)
{
    CALL(uchar, 16)
}

// ushort

OVERLOADABLE INLINE
ushort intel_sub_group_media_block_read_us(
    int2 src_offset,
    int width,
    int height,
    read_write image2d_t image)
{
    CALL(ushort,)
}

OVERLOADABLE INLINE
ushort2 intel_sub_group_media_block_read_us2(
    int2 src_offset,
    int width,
    int height,
    read_write image2d_t image)
{
    CALL(ushort, 2)
}

OVERLOADABLE INLINE
ushort4 intel_sub_group_media_block_read_us4(
    int2 src_offset,
    int width,
    int height,
    read_write image2d_t image)
{
    CALL(ushort, 4)
}

OVERLOADABLE INLINE
ushort8 intel_sub_group_media_block_read_us8(
    int2 src_offset,
    int width,
    int height,
    read_write image2d_t image)
{
    CALL(ushort, 8)
}

OVERLOADABLE INLINE
ushort16 intel_sub_group_media_block_read_us16(
    int2 src_offset,
    int width,
    int height,
    read_write image2d_t image)
{
    CALL(ushort, 16)
}

// uint

OVERLOADABLE INLINE
uint intel_sub_group_media_block_read_ui(
    int2 src_offset,
    int width,
    int height,
    read_write image2d_t image)
{
    CALL(uint,)
}

OVERLOADABLE INLINE
uint2 intel_sub_group_media_block_read_ui2(
    int2 src_offset,
    int width,
    int height,
    read_write image2d_t image)
{
    CALL(uint, 2)
}

OVERLOADABLE INLINE
uint4 intel_sub_group_media_block_read_ui4(
    int2 src_offset,
    int width,
    int height,
    read_write image2d_t image)
{
    CALL(uint, 4)
}

OVERLOADABLE INLINE
uint8 intel_sub_group_media_block_read_ui8(
    int2 src_offset,
    int width,
    int height,
    read_write image2d_t image)
{
    CALL(uint, 8)
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

#undef CALL

#define CALL(TYPE, LEN)                                  \
    int id = (int)__builtin_astype(image, global void*); \
    __builtin_IB_media_block_write_##TYPE##LEN(   \
        id, src_offset, width, height, pixels);

//////////// Writes ////////////

// uchar

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_uc(
    int2 src_offset,
    int width,
    int height,
    uchar pixels,
    write_only image2d_t image)
{
    CALL(uchar,)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_uc2(
    int2 src_offset,
    int width,
    int height,
    uchar2 pixels,
    write_only image2d_t image)
{
    CALL(uchar, 2)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_uc4(
    int2 src_offset,
    int width,
    int height,
    uchar4 pixels,
    write_only image2d_t image)
{
    CALL(uchar, 4)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_uc8(
    int2 src_offset,
    int width,
    int height,
    uchar8 pixels,
    write_only image2d_t image)
{
    CALL(uchar, 8)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_uc16(
    int2 src_offset,
    int width,
    int height,
    uchar16 pixels,
    write_only image2d_t image)
{
    CALL(uchar, 16)
}

// ushort

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_us(
    int2 src_offset,
    int width,
    int height,
    ushort pixels,
    write_only image2d_t image)
{
    CALL(ushort,)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_us2(
    int2 src_offset,
    int width,
    int height,
    ushort2 pixels,
    write_only image2d_t image)
{
    CALL(ushort, 2)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_us4(
    int2 src_offset,
    int width,
    int height,
    ushort4 pixels,
    write_only image2d_t image)
{
    CALL(ushort, 4)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_us8(
    int2 src_offset,
    int width,
    int height,
    ushort8 pixels,
    write_only image2d_t image)
{
    CALL(ushort, 8)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_us16(
    int2 src_offset,
    int width,
    int height,
    ushort16 pixels,
    write_only image2d_t image)
{
    CALL(ushort, 16)
}

// uint

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_ui(
    int2 src_offset,
    int width,
    int height,
    uint pixels,
    write_only image2d_t image)
{
    CALL(uint,)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_ui2(
    int2 src_offset,
    int width,
    int height,
    uint2 pixels,
    write_only image2d_t image)
{
    CALL(uint, 2)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_ui4(
    int2 src_offset,
    int width,
    int height,
    uint4 pixels,
    write_only image2d_t image)
{
    CALL(uint, 4)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_ui8(
    int2 src_offset,
    int width,
    int height,
    uint8 pixels,
    write_only image2d_t image)
{
    CALL(uint, 8)
}

// read_write variants //

#if SUPPORT_ACCESS_QUAL_OVERLOAD
OVERLOADABLE INLINE
void intel_sub_group_media_block_write_uc(
    int2 src_offset,
    int width,
    int height,
    uchar pixels,
    read_write image2d_t image)
{
    CALL(uchar,)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_uc2(
    int2 src_offset,
    int width,
    int height,
    uchar2 pixels,
    read_write image2d_t image)
{
    CALL(uchar, 2)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_uc4(
    int2 src_offset,
    int width,
    int height,
    uchar4 pixels,
    read_write image2d_t image)
{
    CALL(uchar, 4)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_uc8(
    int2 src_offset,
    int width,
    int height,
    uchar8 pixels,
    read_write image2d_t image)
{
    CALL(uchar, 8)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_uc16(
    int2 src_offset,
    int width,
    int height,
    uchar16 pixels,
    read_write image2d_t image)
{
    CALL(uchar, 16)
}

// ushort

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_us(
    int2 src_offset,
    int width,
    int height,
    ushort pixels,
    read_write image2d_t image)
{
    CALL(ushort,)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_us2(
    int2 src_offset,
    int width,
    int height,
    ushort2 pixels,
    read_write image2d_t image)
{
    CALL(ushort, 2)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_us4(
    int2 src_offset,
    int width,
    int height,
    ushort4 pixels,
    read_write image2d_t image)
{
    CALL(ushort, 4)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_us8(
    int2 src_offset,
    int width,
    int height,
    ushort8 pixels,
    read_write image2d_t image)
{
    CALL(ushort, 8)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_us16(
    int2 src_offset,
    int width,
    int height,
    ushort16 pixels,
    read_write image2d_t image)
{
    CALL(ushort, 16)
}

// uint

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_ui(
    int2 src_offset,
    int width,
    int height,
    uint pixels,
    read_write image2d_t image)
{
    CALL(uint,)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_ui2(
    int2 src_offset,
    int width,
    int height,
    uint2 pixels,
    read_write image2d_t image)
{
    CALL(uint, 2)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_ui4(
    int2 src_offset,
    int width,
    int height,
    uint4 pixels,
    read_write image2d_t image)
{
    CALL(uint, 4)
}

OVERLOADABLE INLINE
void intel_sub_group_media_block_write_ui8(
    int2 src_offset,
    int width,
    int height,
    uint8 pixels,
    read_write image2d_t image)
{
    CALL(uint, 8)
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

/////////////////////////////////////////////////////////////////////////////////////
// Block Read/Write Functions

#define  DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(FUNC_NAME, TYPE, INTERNAL_FUNC)         \
INLINE TYPE OVERLOADABLE  FUNC_NAME( image2d_t image, int2 coord )                     \
{                                                                                      \
    int id = (int)__builtin_astype(image, __global void*);                             \
    return INTERNAL_FUNC(id, coord);                                                   \
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
#define  DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(FUNC_NAME, TYPE, INTERNAL_FUNC)      \
INLINE TYPE OVERLOADABLE  FUNC_NAME( read_write image2d_t image, int2 coord )          \
{                                                                                      \
    int id = (int)__builtin_astype(image, __global void*);                             \
    return INTERNAL_FUNC(id, coord);                                                   \
}
#else
#define  DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(FUNC_NAME, TYPE, INTERNAL_FUNC)
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

#define  DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(FUNC_NAME, TYPE, ELEM_TYPE,  INTERNAL_FUNC)    \
INLINE TYPE OVERLOADABLE  FUNC_NAME( const __global ELEM_TYPE* p )                             \
{                                                                                              \
    return INTERNAL_FUNC((__global void *)p);                                                  \
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
#define  DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(FUNC_NAME, TYPE, INTERNAL_FUNC)        \
INLINE void OVERLOADABLE FUNC_NAME( write_only image2d_t image, int2 coord, TYPE data )\
{                                                                                      \
    int id = (int)__builtin_astype(image, __global void*);                             \
    INTERNAL_FUNC(id, coord, data);                                                    \
}
#else
#define  DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(FUNC_NAME, TYPE, INTERNAL_FUNC)
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

#define  DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(FUNC_NAME, TYPE, ELEM_TYPE, INTERNAL_FUNC)  \
INLINE void OVERLOADABLE  FUNC_NAME( __global ELEM_TYPE* p, TYPE data )                      \
{                                                                                            \
    INTERNAL_FUNC(p, data);                                                                  \
}

DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read,  uint,  __builtin_IB_simd_media_block_read_1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read2, uint2, __builtin_IB_simd_media_block_read_2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read4, uint4, __builtin_IB_simd_media_block_read_4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read8, uint8, __builtin_IB_simd_media_block_read_8)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read,  uint,  __builtin_IB_simd_media_block_read_1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read2, uint2, __builtin_IB_simd_media_block_read_2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read4, uint4, __builtin_IB_simd_media_block_read_4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read8, uint8, __builtin_IB_simd_media_block_read_8)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read,  uint,  uint, __builtin_IB_simd_block_read_1_global)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read2, uint2, uint, __builtin_IB_simd_block_read_2_global)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read4, uint4, uint, __builtin_IB_simd_block_read_4_global)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read8, uint8, uint, __builtin_IB_simd_block_read_8_global)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write,  uint,  __builtin_IB_simd_media_block_write_1)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write2, uint2, __builtin_IB_simd_media_block_write_2)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write4, uint4, __builtin_IB_simd_media_block_write_4)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write8, uint8, __builtin_IB_simd_media_block_write_8)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write,  uint,  __builtin_IB_simd_media_block_write_1)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write2, uint2, __builtin_IB_simd_media_block_write_2)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write4, uint4, __builtin_IB_simd_media_block_write_4)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write8, uint8, __builtin_IB_simd_media_block_write_8)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write,  uint,  uint, __builtin_IB_simd_block_write_1_global)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write2, uint2, uint, __builtin_IB_simd_block_write_2_global)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write4, uint4, uint, __builtin_IB_simd_block_write_4_global)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write8, uint8, uint, __builtin_IB_simd_block_write_8_global)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_us,   ushort,  __builtin_IB_simd_media_block_read_1_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_us2,  ushort2, __builtin_IB_simd_media_block_read_2_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_us4,  ushort4, __builtin_IB_simd_media_block_read_4_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(intel_sub_group_block_read_us8,  ushort8, __builtin_IB_simd_media_block_read_8_h)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_us,   ushort,  __builtin_IB_simd_media_block_read_1_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_us2,  ushort2, __builtin_IB_simd_media_block_read_2_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_us4,  ushort4, __builtin_IB_simd_media_block_read_4_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_us8,  ushort8, __builtin_IB_simd_media_block_read_8_h)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_us,  ushort,  ushort, __builtin_IB_simd_block_read_1_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_us2, ushort2, ushort, __builtin_IB_simd_block_read_2_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_us4, ushort4, ushort, __builtin_IB_simd_block_read_4_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_us8, ushort8, ushort, __builtin_IB_simd_block_read_8_global_h)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_us,  ushort,  __builtin_IB_simd_media_block_write_1_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_us2, ushort2, __builtin_IB_simd_media_block_write_2_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_us4, ushort4, __builtin_IB_simd_media_block_write_4_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_us8, ushort8, __builtin_IB_simd_media_block_write_8_h)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_us,  ushort,  __builtin_IB_simd_media_block_write_1_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_us2, ushort2, __builtin_IB_simd_media_block_write_2_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_us4, ushort4, __builtin_IB_simd_media_block_write_4_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_us8, ushort8, __builtin_IB_simd_media_block_write_8_h)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_us,  ushort,  ushort, __builtin_IB_simd_block_write_1_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_us2, ushort2, ushort, __builtin_IB_simd_block_write_2_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_us4, ushort4, ushort, __builtin_IB_simd_block_write_4_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_us8, ushort8, ushort, __builtin_IB_simd_block_write_8_global_h)
