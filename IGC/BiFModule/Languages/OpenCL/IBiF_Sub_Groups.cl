/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )( Subgroup, 0, AcquireRelease );
}

#if(__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE void OVERLOADABLE sub_group_barrier( cl_mem_fence_flags flags, memory_scope scope )
{
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )( Subgroup, 0, AcquireRelease );
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// gentype intel_sub_group_shuffle(gentype data, uint sub_group_local_id )
//
// Allows data to be arbitrarily transferred between work items in a subgroup.
// The data that is returned for this work item is the value of data for the work item
// identified by sub_group_local_id.
#define DEFN_INTEL_SUB_GROUP_SHUFFLE(TYPE, SPIRV_TYPE, TYPE_ABBR)                               \
INLINE TYPE OVERLOADABLE intel_sub_group_shuffle( TYPE x, uint c )                              \
{                                                                                               \
    return SPIRV_BUILTIN(SubgroupShuffleINTEL, _##TYPE_ABBR##_i32, )( as_##SPIRV_TYPE(x), c );  \
}

#ifdef cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE(char,   char,    i8)
DEFN_INTEL_SUB_GROUP_SHUFFLE(uchar,  char,    i8)
#endif // cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE(short,  short,   i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE(ushort, short,   i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE(int,    int,     i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE(uint,   int,     i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE(long,   long,    i64)
DEFN_INTEL_SUB_GROUP_SHUFFLE(ulong,  long,    i64)
#ifdef cl_khr_fp16
DEFN_INTEL_SUB_GROUP_SHUFFLE(half,   half,    f16)
#endif // cl_khr_fp16
DEFN_INTEL_SUB_GROUP_SHUFFLE(float,  float,   f32)
#ifdef cl_khr_fp64
DEFN_INTEL_SUB_GROUP_SHUFFLE(double, double,  f64)
#endif // cl_khr_fp64

#ifdef cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, char,   char,   uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, uchar,  uchar,  uint )
#endif // cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, short,  short,  uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, ushort, ushort, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, int,    int,    uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, uint,   uint,   uint )
#ifdef cl_khr_fp16
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, half,   half,   uint )
#endif // cl_khr_fp16
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle, float,  float,  uint )

// gentype intel_sub_group_shuffle_down( gentype current, gentype next, uint delta )
//
// Allows data to be transferred from a work item in the subgroup with a higher sub_group_local_id
// down to a work item in the subgroup with a lower sub_group_local_id.
#define DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(TYPE, SPIRV_TYPE, TYPE_ABBR)                                                                   \
INLINE TYPE OVERLOADABLE intel_sub_group_shuffle_down(TYPE cur, TYPE next, uint c)                                                       \
{                                                                                                                                        \
    return SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _##TYPE_ABBR##_##TYPE_ABBR##_i32, )(as_##SPIRV_TYPE(cur), as_##SPIRV_TYPE(next), c);  \
}

#ifdef cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(char,   char,   i8)
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(uchar,  char,   i8)
#endif // cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(short,  short,  i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(ushort, short,  i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(int,    int,    i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(uint,   int,    i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(long,   long,   i64)
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(ulong,  long,   i64)
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(float,  float,  f32)
#ifdef cl_khr_fp64
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(double, double, f64)
#endif // cl_khr_fp64

#ifdef cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, char,   char,   uint)
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, uchar,  uchar,  uint)
#endif // cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, short,  short,  uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, ushort, ushort, uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, int,    int,    uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, uint,   uint,   uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_down, float,  float,  uint )

// gentype intel_sub_group_shuffle_up(gentype previous, gentype current, uint delta )
//
// Allows data to be transferred from a work item in the subgroup with a lower sub_group_local_id
// up to a work item in the subgroup with a higher sub_group_local_id.
#define DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(TYPE, SPIRV_TYPE, TYPE_ABBR)                                                                   \
INLINE TYPE OVERLOADABLE  intel_sub_group_shuffle_up( TYPE prev, TYPE cur, uint c )                                                    \
{                                                                                                                                      \
    return SPIRV_BUILTIN(SubgroupShuffleUpINTEL, _##TYPE_ABBR##_##TYPE_ABBR##_i32, )( as_##SPIRV_TYPE(prev), as_##SPIRV_TYPE(cur), c); \
}

#ifdef cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(char,   char,   i8)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(uchar,  char,   i8)
#endif // cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(short,  short,  i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(ushort, short,  i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(int,    int,    i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(uint,   int,    i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(long,   long,   i64)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(ulong,  long,   i64)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(float,  float,  f32)
#ifdef cl_khr_fp64
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(double, double, f64)
#endif // cl_khr_fp64

#ifdef cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_up, char,   char,   uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_up, uchar,  uchar,  uint )
#endif // cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_up, short,  short,  uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_up, ushort, ushort, uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_up, int,    int,    uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_up, uint,   uint,   uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( intel_sub_group_shuffle_up, float,  float,  uint )

// gentype intel_sub_group_shuffle_xor( gentype data, uint value )
//
// The data that is returned for this work item is the value of data for the work item with
// sub_group_local_id equal to this work item's sub_group_local_id XOR'd with the specified value.
#define DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(TYPE, SPIRV_TYPE, TYPE_ABBR)                             \
INLINE TYPE OVERLOADABLE  intel_sub_group_shuffle_xor( TYPE x, uint c )                           \
{                                                                                                 \
    return SPIRV_BUILTIN(SubgroupShuffleXorINTEL, _##TYPE_ABBR##_i32, )( as_##SPIRV_TYPE(x), c ); \
}

#ifdef cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(char,   char,   i8)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(uchar,  char,   i8)
#endif // cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(short,  short,  i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(ushort, short,  i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(int,    int,    i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(uint,   int,    i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(long,   long,   i64)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(ulong,  long,   i64)
#ifdef cl_khr_fp16
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(half,   half,   f16)
#endif // cl_khr_fp16
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(float,  float,  f32)
#ifdef cl_khr_fp64
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(double, double, f64)
#endif // cl_khr_fp64

#ifdef cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, char,   char,   uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, uchar,  uchar,  uint )
#endif // cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, short,  short,  uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, ushort, ushort, uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, int,    int,    uint )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, uint,   uint,   uint )
#ifdef cl_khr_fp16
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, half,   half,   uint)
#endif // cl_khr_fp16
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( intel_sub_group_shuffle_xor, float,  float,  uint )

#ifdef cl_khr_fp16

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

#define  DEFN_SUB_GROUP_BROADCAST(TYPE, SPV_TYPE, TYPE_ABBR)                                                         \
INLINE TYPE OVERLOADABLE  sub_group_broadcast( TYPE x, uint sub_group_local_id )                                     \
{                                                                                                                    \
    int3 local_id = (int3)(sub_group_local_id,0,0);                                                                  \
    return as_##TYPE(SPIRV_BUILTIN(GroupBroadcast, _i32_##TYPE_ABBR##_v3i32, )(Subgroup,as_##SPV_TYPE(x),local_id)); \
}

#define  DEFN_INTEL_SUB_GROUP_BROADCAST(TYPE, SPV_TYPE, TYPE_ABBR)                                        \
INLINE TYPE OVERLOADABLE  intel_sub_group_broadcast( TYPE x, uint sub_group_local_id )                    \
{                                                                                                         \
    int3 local_id = (int3)(sub_group_local_id,0,0);                                                       \
    return SPIRV_BUILTIN(GroupBroadcast, _i32_##TYPE_ABBR##_v3i32, )(Subgroup,as_##SPV_TYPE(x),local_id); \
}

DEFN_SUB_GROUP_BROADCAST(int,   int,  i32)
DEFN_SUB_GROUP_BROADCAST(uint,  int,  i32)
DEFN_SUB_GROUP_BROADCAST(long,  long,  i64)
DEFN_SUB_GROUP_BROADCAST(ulong, long,  i64)
DEFN_SUB_GROUP_BROADCAST(float, float, f32)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_BROADCAST(half,  half, f16)
#endif // cl_khr_fp16
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_BROADCAST(double, double, f64)
#endif // cl_khr_fp64

#if defined(cl_khr_subgroup_extended_types)
DEFN_SUB_GROUP_BROADCAST(char,   char, i8)
DEFN_SUB_GROUP_BROADCAST(uchar,  char, i8)
DEFN_SUB_GROUP_BROADCAST(short,  short, i16)
DEFN_SUB_GROUP_BROADCAST(ushort, short, i16)

#define DEFN_SUB_GROUP_BROADCAST_VEC(TYPE, SPV_TYPE, TYPE_ABBR) \
DEFN_SUB_GROUP_BROADCAST(TYPE##2,  SPV_TYPE##2,  v2##TYPE_ABBR) \
DEFN_SUB_GROUP_BROADCAST(TYPE##3,  SPV_TYPE##3,  v3##TYPE_ABBR) \
DEFN_SUB_GROUP_BROADCAST(TYPE##4,  SPV_TYPE##4,  v4##TYPE_ABBR) \
DEFN_SUB_GROUP_BROADCAST(TYPE##8,  SPV_TYPE##8,  v8##TYPE_ABBR) \
DEFN_SUB_GROUP_BROADCAST(TYPE##16, SPV_TYPE##16, v16##TYPE_ABBR)

DEFN_SUB_GROUP_BROADCAST_VEC(char,   char,   i8)
DEFN_SUB_GROUP_BROADCAST_VEC(uchar,  char,   i8)
DEFN_SUB_GROUP_BROADCAST_VEC(short,  short,  i16)
DEFN_SUB_GROUP_BROADCAST_VEC(ushort, short,  i16)
DEFN_SUB_GROUP_BROADCAST_VEC(int,    int,    i32)
DEFN_SUB_GROUP_BROADCAST_VEC(uint,   int,    i32)
DEFN_SUB_GROUP_BROADCAST_VEC(long,   long,   i64)
DEFN_SUB_GROUP_BROADCAST_VEC(ulong,  long,   i64)
DEFN_SUB_GROUP_BROADCAST_VEC(float,  float,  f32)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_BROADCAST_VEC(half,   half,   f16)
#endif // defined(cl_khr_fp16)
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_BROADCAST_VEC(double, double, f64)
#endif // defined(cl_khr_fp64)
#endif

#if defined(cl_intel_subgroups_short)
DEFN_INTEL_SUB_GROUP_BROADCAST(short,  short, i16)
DEFN_INTEL_SUB_GROUP_BROADCAST(ushort, short, i16)
#endif // cl_intel_subgroups_short

#ifdef cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_BROADCAST(char,  char, i8)
DEFN_INTEL_SUB_GROUP_BROADCAST(uchar, char, i8)
#endif // cl_intel_subgroups_char

INLINE int OVERLOADABLE sub_group_all( int predicate )
{
    return SPIRV_BUILTIN(GroupAll, _i32_i1, )(Subgroup, predicate);
}

INLINE int OVERLOADABLE sub_group_any( int predicate )
{
    return SPIRV_BUILTIN(GroupAny, _i32_i1, )(Subgroup, predicate);
}

#if defined(cl_khr_subgroup_non_uniform_vote)
INLINE int OVERLOADABLE sub_group_elect()
{
    return SPIRV_BUILTIN(GroupNonUniformElect, _i32, )(Subgroup);
}

INLINE int OVERLOADABLE sub_group_non_uniform_all(int predicate)
{
    return SPIRV_BUILTIN(GroupNonUniformAll, _i32_i1, )(Subgroup, predicate);
}

INLINE int OVERLOADABLE sub_group_non_uniform_any(int predicate)
{
    return SPIRV_BUILTIN(GroupNonUniformAny, _i32_i1, )(Subgroup, predicate);
}

#define DEFN_SUB_GROUP_NON_UNIFORM_ALL_EQUAL(TYPE, SPV_TYPE, TYPE_ABBR)                                \
INLINE int OVERLOADABLE sub_group_non_uniform_all_equal(TYPE value)                                    \
{                                                                                                      \
    return SPIRV_BUILTIN(GroupNonUniformAllEqual, _i32_##TYPE_ABBR, )(Subgroup, as_##SPV_TYPE(value)); \
}

DEFN_SUB_GROUP_NON_UNIFORM_ALL_EQUAL(char,   char, i8)
DEFN_SUB_GROUP_NON_UNIFORM_ALL_EQUAL(uchar,  char, i8)
DEFN_SUB_GROUP_NON_UNIFORM_ALL_EQUAL(short,  short, i16)
DEFN_SUB_GROUP_NON_UNIFORM_ALL_EQUAL(ushort, short, i16)
DEFN_SUB_GROUP_NON_UNIFORM_ALL_EQUAL(int,    int, i32)
DEFN_SUB_GROUP_NON_UNIFORM_ALL_EQUAL(uint,   int, i32)
DEFN_SUB_GROUP_NON_UNIFORM_ALL_EQUAL(long,   long, i64)
DEFN_SUB_GROUP_NON_UNIFORM_ALL_EQUAL(ulong,  long, i64)
DEFN_SUB_GROUP_NON_UNIFORM_ALL_EQUAL(float,  float, f32)
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_NON_UNIFORM_ALL_EQUAL(double, double, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_NON_UNIFORM_ALL_EQUAL(half,   half, f16)
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_non_uniform_vote)

#if defined(cl_khr_subgroup_ballot)

#define DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE, SPV_TYPE, TYPE_ABBR)                                                              \
INLINE TYPE OVERLOADABLE sub_group_non_uniform_broadcast(TYPE value, uint index)                                                \
{                                                                                                                               \
    return as_##TYPE(SPIRV_BUILTIN(GroupNonUniformBroadcast, _i32_##TYPE_ABBR##_i32, )(Subgroup, as_##SPV_TYPE(value), index)); \
}                                                                                                                               \
INLINE TYPE OVERLOADABLE sub_group_broadcast_first(TYPE value)                                                                  \
{                                                                                                                               \
    return as_##TYPE(SPIRV_BUILTIN(GroupNonUniformBroadcastFirst, _i32_##TYPE_ABBR, )(Subgroup, as_##SPV_TYPE(value)));         \
}

#define DEFN_NON_UNIFORM_BROADCAST(TYPE, SPV_TYPE, TYPE_ABBR)            \
DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE, SPV_TYPE, TYPE_ABBR)               \
DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE##2, SPV_TYPE##2, v2##TYPE_ABBR)     \
DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE##3, SPV_TYPE##3, v3##TYPE_ABBR)     \
DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE##4, SPV_TYPE##4, v4##TYPE_ABBR)     \
DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE##8, SPV_TYPE##8, v8##TYPE_ABBR)     \
DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE##16, SPV_TYPE##16, v16##TYPE_ABBR)

DEFN_NON_UNIFORM_BROADCAST(char,   char,   i8)
DEFN_NON_UNIFORM_BROADCAST(uchar,  char,   i8)
DEFN_NON_UNIFORM_BROADCAST(short,  short,  i16)
DEFN_NON_UNIFORM_BROADCAST(ushort, short,  i16)
DEFN_NON_UNIFORM_BROADCAST(int,    int,    i32)
DEFN_NON_UNIFORM_BROADCAST(uint,   int,    i32)
DEFN_NON_UNIFORM_BROADCAST(long,   long,   i64)
DEFN_NON_UNIFORM_BROADCAST(ulong,  long,   i64)
DEFN_NON_UNIFORM_BROADCAST(float,  float,  f32)
#if defined(cl_khr_fp64)
DEFN_NON_UNIFORM_BROADCAST(double, double, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_BROADCAST(half,   half,   f16)
#endif // defined(cl_khr_fp16)

INLINE uint4 OVERLOADABLE sub_group_ballot(int predicate)
{
    return SPIRV_BUILTIN(GroupNonUniformBallot, _i32_i1, )(Subgroup, predicate);
}

INLINE int OVERLOADABLE sub_group_inverse_ballot(uint4 value)
{
    return SPIRV_BUILTIN(GroupNonUniformInverseBallot, _i32_v4i32, )(Subgroup, value);
}

INLINE int OVERLOADABLE sub_group_ballot_bit_extract(uint4 value, uint index)
{
    return SPIRV_BUILTIN(GroupNonUniformBallotBitExtract, _i32_v4i32_i32, )(Subgroup, value, index);
}

INLINE uint OVERLOADABLE sub_group_ballot_bit_count(uint4 value)
{
    return SPIRV_BUILTIN(GroupNonUniformBallotBitCount, _i32_i32_v4i32, )(Subgroup, GroupOperationReduce, value);
}

INLINE uint OVERLOADABLE sub_group_ballot_inclusive_scan(uint4 value)
{
    return SPIRV_BUILTIN(GroupNonUniformBallotBitCount, _i32_i32_v4i32, )(Subgroup, GroupOperationInclusiveScan, value);
}

INLINE uint OVERLOADABLE sub_group_ballot_exclusive_scan(uint4 value)
{
    return SPIRV_BUILTIN(GroupNonUniformBallotBitCount, _i32_i32_v4i32, )(Subgroup, GroupOperationExclusiveScan, value);
}

INLINE uint OVERLOADABLE sub_group_ballot_find_lsb(uint4 value)
{
    return SPIRV_BUILTIN(GroupNonUniformBallotFindLSB, _i32_v4i32, )(Subgroup, value);
}

INLINE uint OVERLOADABLE sub_group_ballot_find_msb(uint4 value)
{
    return SPIRV_BUILTIN(GroupNonUniformBallotFindMSB, _i32_v4i32, )(Subgroup, value);
}

INLINE uint4 OVERLOADABLE get_sub_group_eq_mask()
{
    return as_uint4(SPIRV_BUILTIN_NO_OP(BuiltInSubgroupEqMask, , )());
}

INLINE uint4 OVERLOADABLE get_sub_group_ge_mask()
{
    return as_uint4(SPIRV_BUILTIN_NO_OP(BuiltInSubgroupGeMask, , )());
}

INLINE uint4 OVERLOADABLE get_sub_group_gt_mask()
{
    return as_uint4(SPIRV_BUILTIN_NO_OP(BuiltInSubgroupGtMask, , )());
}

INLINE uint4 OVERLOADABLE get_sub_group_le_mask()
{
    return as_uint4(SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLeMask, , )());
}

INLINE uint4 OVERLOADABLE get_sub_group_lt_mask()
{
    return as_uint4(SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLtMask, , )());
}

#endif // defined(cl_khr_subgroup_ballot)


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

#define  DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(FUNC_NAME, TYPE, ELEM_TYPE, TYPE_ABBR, INTERNAL_FUNC)       \
INLINE TYPE OVERLOADABLE  FUNC_NAME( const __local ELEM_TYPE* p )                                          \
{                                                                                                          \
    return INTERNAL_FUNC(p);                                                                               \
}                                                                                                          \

#if SUPPORT_ACCESS_QUAL_OVERLOAD
#define  DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(FUNC_NAME, TYPE, TYPE_ABBR, INTERNAL_FUNC)   \
INLINE void OVERLOADABLE FUNC_NAME( write_only image2d_t image, int2 coord, TYPE data )         \
{                                                                                               \
    int id = (int)__builtin_astype(image, __global void*);                                      \
    INTERNAL_FUNC(id, coord, data);                                                             \
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

#define  DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(FUNC_NAME, TYPE, ELEM_TYPE, PTR_TYPE, TYPE_ABBR, INTERNAL_FUNC)    \
INLINE void OVERLOADABLE  FUNC_NAME( __local ELEM_TYPE* p, TYPE data )                                             \
{                                                                                                                  \
    INTERNAL_FUNC(p, data);                                                                                        \
}                                                                                                                  \

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


#ifdef cl_intel_subgroup_local_block_io
//
// SLM simd block read/write
//
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read,  uint,  uint, i32, __builtin_IB_simd_block_read_1_local)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read2, uint2, uint, i32, __builtin_IB_simd_block_read_2_local)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read4, uint4, uint, i32, __builtin_IB_simd_block_read_4_local)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read8, uint8, uint, i32, __builtin_IB_simd_block_read_8_local)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write,  uint,  uint, i32, i32,   __builtin_IB_simd_block_write_1_local)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write2, uint2, uint, i32, v2i32, __builtin_IB_simd_block_write_2_local)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write4, uint4, uint, i32, v4i32, __builtin_IB_simd_block_write_4_local)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write8, uint8, uint, i32, v8i32, __builtin_IB_simd_block_write_8_local)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read_us,  ushort,  ushort, i16, __builtin_IB_simd_block_read_1_local_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read_us2, ushort2, ushort, i16, __builtin_IB_simd_block_read_2_local_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read_us4, ushort4, ushort, i16, __builtin_IB_simd_block_read_4_local_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read_us8, ushort8, ushort, i16, __builtin_IB_simd_block_read_8_local_h)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_us,  ushort,  ushort, i16, i16,   __builtin_IB_simd_block_write_1_local_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_us2, ushort2, ushort, i16, v2i16, __builtin_IB_simd_block_write_2_local_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_us4, ushort4, ushort, i16, v4i16, __builtin_IB_simd_block_write_4_local_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_us8, ushort8, ushort, i16, v8i16, __builtin_IB_simd_block_write_8_local_h)

#ifdef cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read_uc,   uchar,   uchar, i8, __builtin_IB_simd_block_read_1_local_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read_uc2,  uchar2,  uchar, i8, __builtin_IB_simd_block_read_2_local_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read_uc4,  uchar4,  uchar, i8, __builtin_IB_simd_block_read_4_local_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read_uc8,  uchar8,  uchar, i8, __builtin_IB_simd_block_read_8_local_b)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read_uc16, uchar16, uchar, i8, __builtin_IB_simd_block_read_16_local_b)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_uc,   uchar,   uchar, i8, i8,    __builtin_IB_simd_block_write_1_local_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_uc2,  uchar2,  uchar, i8, v2i8,  __builtin_IB_simd_block_write_2_local_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_uc4,  uchar4,  uchar, i8, v4i8,  __builtin_IB_simd_block_write_4_local_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_uc8,  uchar8,  uchar, i8, v8i8,  __builtin_IB_simd_block_write_8_local_b)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_uc16, uchar16, uchar, i8, v16i8, __builtin_IB_simd_block_write_16_local_b)
#endif // cl_intel_subgroups_char

#ifdef cl_intel_subgroups_long
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read_ul,  ulong,  ulong, i64, __builtin_IB_simd_block_read_1_local_l)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read_ul2, ulong2, ulong, i64, __builtin_IB_simd_block_read_2_local_l)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read_ul4, ulong4, ulong, i64, __builtin_IB_simd_block_read_4_local_l)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read_ul8, ulong8, ulong, i64, __builtin_IB_simd_block_read_8_local_l)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_ul,  ulong,  ulong, i64, i64, __builtin_IB_simd_block_write_1_local_l)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_ul2, ulong2, ulong, i64, v2i64, __builtin_IB_simd_block_write_2_local_l)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_ul4, ulong4, ulong, i64, v4i64, __builtin_IB_simd_block_write_4_local_l)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_ul8, ulong8, ulong, i64, v8i64, __builtin_IB_simd_block_write_8_local_l)
#endif // cl_intel_subgroups_long

#endif // cl_intel_subgroup_local_block_io


#if defined(cl_khr_subgroup_shuffle)
#define DEFN_SUB_GROUP_SHUFFLE(TYPE, SPV_TYPE, TYPE_ABBR)                                                             \
INLINE TYPE OVERLOADABLE sub_group_shuffle(TYPE value, uint index)                                                    \
{                                                                                                                     \
    return SPIRV_BUILTIN(GroupNonUniformShuffle, _i32_##TYPE_ABBR##_i32, )(Subgroup, as_##SPV_TYPE(value), index);    \
}                                                                                                                     \
INLINE TYPE OVERLOADABLE sub_group_shuffle_xor(TYPE value, uint mask)                                                 \
{                                                                                                                     \
    return SPIRV_BUILTIN(GroupNonUniformShuffleXor, _i32_##TYPE_ABBR##_i32, )(Subgroup, as_##SPV_TYPE(value), mask);  \
}

DEFN_SUB_GROUP_SHUFFLE(char,   char,   i8)
DEFN_SUB_GROUP_SHUFFLE(uchar,  char,   i8)
DEFN_SUB_GROUP_SHUFFLE(short,  short,  i16)
DEFN_SUB_GROUP_SHUFFLE(ushort, short,  i16)
DEFN_SUB_GROUP_SHUFFLE(int,    int,    i32)
DEFN_SUB_GROUP_SHUFFLE(uint,   int,    i32)
DEFN_SUB_GROUP_SHUFFLE(long,   long,   i64)
DEFN_SUB_GROUP_SHUFFLE(ulong,  long,   i64)
DEFN_SUB_GROUP_SHUFFLE(float,  float,  f32)
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_SHUFFLE(double, double, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_SHUFFLE(half,   half,   f16)
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_shuffle)

#if defined(cl_khr_subgroup_shuffle_relative)
#define DEFN_SUB_GROUP_SHUFFLE_RELATIVE(TYPE, SPV_TYPE, TYPE_ABBR)                                                      \
INLINE TYPE OVERLOADABLE sub_group_shuffle_up(TYPE value, uint delta)                                                   \
{                                                                                                                       \
    return SPIRV_BUILTIN(GroupNonUniformShuffleUp, _i32_##TYPE_ABBR##_i32, )(Subgroup, as_##SPV_TYPE(value), delta);    \
}                                                                                                                       \
INLINE TYPE OVERLOADABLE sub_group_shuffle_down(TYPE value, uint delta)                                                 \
{                                                                                                                       \
    return SPIRV_BUILTIN(GroupNonUniformShuffleDown, _i32_##TYPE_ABBR##_i32, )(Subgroup, as_##SPV_TYPE(value), delta);  \
}

DEFN_SUB_GROUP_SHUFFLE_RELATIVE(char,   char,   i8)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(uchar,  char,   i8)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(short,  short,  i16)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(ushort, short,  i16)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(int,    int,    i32)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(uint,   int,    i32)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(long,   long,   i64)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(ulong,  long,   i64)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(float,  float,  f32)
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(double, double, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(half,   half,   f16)
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_shuffle_relative)
