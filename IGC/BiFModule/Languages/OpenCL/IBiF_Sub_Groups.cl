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
    __spirv_ControlBarrier( Subgroup, 0, AcquireRelease );
}

#if(__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE void OVERLOADABLE sub_group_barrier( cl_mem_fence_flags flags, memory_scope scope )
{
    __spirv_ControlBarrier( Subgroup, 0, AcquireRelease );
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
    return __spirv_SubgroupShuffleINTEL( as_##SPIRV_TYPE(x), c );  \
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
    return __spirv_SubgroupShuffleDownINTEL(as_##SPIRV_TYPE(cur), as_##SPIRV_TYPE(next), c);  \
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
    return __spirv_SubgroupShuffleUpINTEL( as_##SPIRV_TYPE(prev), as_##SPIRV_TYPE(cur), c); \
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
    return __spirv_SubgroupShuffleXorINTEL( as_##SPIRV_TYPE(x), c ); \
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
    return as_##TYPE(__spirv_GroupBroadcast(Subgroup,as_##SPV_TYPE(x),local_id)); \
}

#define  DEFN_INTEL_SUB_GROUP_BROADCAST(TYPE, SPV_TYPE, TYPE_ABBR)                                        \
INLINE TYPE OVERLOADABLE  intel_sub_group_broadcast( TYPE x, uint sub_group_local_id )                    \
{                                                                                                         \
    int3 local_id = (int3)(sub_group_local_id,0,0);                                                       \
    return __spirv_GroupBroadcast(Subgroup,as_##SPV_TYPE(x),local_id); \
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
    return __spirv_GroupAll(Subgroup, predicate);
}

INLINE int OVERLOADABLE sub_group_any( int predicate )
{
    return __spirv_GroupAny(Subgroup, predicate);
}

#if defined(cl_khr_subgroup_non_uniform_vote)
INLINE int OVERLOADABLE sub_group_elect()
{
    return __spirv_GroupNonUniformElect(Subgroup);
}

INLINE int OVERLOADABLE sub_group_non_uniform_all(int predicate)
{
    return __spirv_GroupNonUniformAll(Subgroup, predicate);
}

INLINE int OVERLOADABLE sub_group_non_uniform_any(int predicate)
{
    return __spirv_GroupNonUniformAny(Subgroup, predicate);
}

#define DEFN_SUB_GROUP_NON_UNIFORM_ALL_EQUAL(TYPE, SPV_TYPE, TYPE_ABBR)                                \
INLINE int OVERLOADABLE sub_group_non_uniform_all_equal(TYPE value)                                    \
{                                                                                                      \
    return __spirv_GroupNonUniformAllEqual(Subgroup, as_##SPV_TYPE(value)); \
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
    return as_##TYPE(__spirv_GroupNonUniformBroadcast(Subgroup, as_##SPV_TYPE(value), index)); \
}                                                                                                                               \
INLINE TYPE OVERLOADABLE sub_group_broadcast_first(TYPE value)                                                                  \
{                                                                                                                               \
    return as_##TYPE(__spirv_GroupNonUniformBroadcastFirst(Subgroup, as_##SPV_TYPE(value)));         \
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
    return __spirv_GroupNonUniformBallot(Subgroup, predicate);
}

INLINE int OVERLOADABLE sub_group_inverse_ballot(uint4 value)
{
    return __spirv_GroupNonUniformInverseBallot(Subgroup, value);
}

INLINE int OVERLOADABLE sub_group_ballot_bit_extract(uint4 value, uint index)
{
    return __spirv_GroupNonUniformBallotBitExtract(Subgroup, value, index);
}

INLINE uint OVERLOADABLE sub_group_ballot_bit_count(uint4 value)
{
    return __spirv_GroupNonUniformBallotBitCount(Subgroup, GroupOperationReduce, value);
}

INLINE uint OVERLOADABLE sub_group_ballot_inclusive_scan(uint4 value)
{
    return __spirv_GroupNonUniformBallotBitCount(Subgroup, GroupOperationInclusiveScan, value);
}

INLINE uint OVERLOADABLE sub_group_ballot_exclusive_scan(uint4 value)
{
    return __spirv_GroupNonUniformBallotBitCount(Subgroup, GroupOperationExclusiveScan, value);
}

INLINE uint OVERLOADABLE sub_group_ballot_find_lsb(uint4 value)
{
    return __spirv_GroupNonUniformBallotFindLSB(Subgroup, value);
}

INLINE uint OVERLOADABLE sub_group_ballot_find_msb(uint4 value)
{
    return __spirv_GroupNonUniformBallotFindMSB(Subgroup, value);
}

INLINE uint4 OVERLOADABLE get_sub_group_eq_mask()
{
    return as_uint4(__spirv_BuiltInSubgroupEqMask());
}

INLINE uint4 OVERLOADABLE get_sub_group_ge_mask()
{
    return as_uint4(__spirv_BuiltInSubgroupGeMask());
}

INLINE uint4 OVERLOADABLE get_sub_group_gt_mask()
{
    return as_uint4(__spirv_BuiltInSubgroupGtMask());
}

INLINE uint4 OVERLOADABLE get_sub_group_le_mask()
{
    return as_uint4(__spirv_BuiltInSubgroupLeMask());
}

INLINE uint4 OVERLOADABLE get_sub_group_lt_mask()
{
    return as_uint4(__spirv_BuiltInSubgroupLtMask());
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
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);     \
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
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);     \
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
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);         \
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
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);     \
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
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);                     \
    return INTERNAL_FUNC(id, coord);                                                    \
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
#define  DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(FUNC_NAME, TYPE, INTERNAL_FUNC)       \
INLINE TYPE OVERLOADABLE  FUNC_NAME( read_write image2d_t image, int2 coord )           \
{                                                                                       \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);                     \
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
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);                             \
    INTERNAL_FUNC(id, coord, data);                                                             \
}

#else
#define  DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(FUNC_NAME, TYPE, TYPE_ABBR, INTERNAL_FUNC)
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

#if SUPPORT_ACCESS_QUAL_OVERLOAD
#define  DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(FUNC_NAME, TYPE, INTERNAL_FUNC)     \
INLINE void OVERLOADABLE FUNC_NAME( read_write image2d_t image, int2 coord, TYPE data )\
{                                                                                      \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);                    \
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
DEFN_INTEL_SUB_GROUP_BLOCK_READ_IMAGE_RW(intel_sub_group_block_read_us16,  ushort16, __builtin_IB_simd_media_block_read_16_h)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_us,  ushort,  ushort, i16, __builtin_IB_simd_block_read_1_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_us2, ushort2, ushort, i16, __builtin_IB_simd_block_read_2_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_us4, ushort4, ushort, i16, __builtin_IB_simd_block_read_4_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_us8, ushort8, ushort, i16, __builtin_IB_simd_block_read_8_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(intel_sub_group_block_read_us16, ushort16, ushort, i16, __builtin_IB_simd_block_read_16_global_h)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_us,  ushort,  i16,   __builtin_IB_simd_media_block_write_1_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_us2, ushort2, v2i16, __builtin_IB_simd_media_block_write_2_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_us4, ushort4, v4i16, __builtin_IB_simd_media_block_write_4_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_us8, ushort8, v8i16, __builtin_IB_simd_media_block_write_8_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_WO(intel_sub_group_block_write_us16, ushort16, v16i16, __builtin_IB_simd_media_block_write_16_h)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_us,  ushort,  __builtin_IB_simd_media_block_write_1_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_us2, ushort2, __builtin_IB_simd_media_block_write_2_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_us4, ushort4, __builtin_IB_simd_media_block_write_4_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_us8, ushort8, __builtin_IB_simd_media_block_write_8_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE_RW(intel_sub_group_block_write_us16, ushort16, __builtin_IB_simd_media_block_write_16_h)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_us,  ushort,  ushort, i16, i16, __builtin_IB_simd_block_write_1_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_us2, ushort2, ushort, i16, v2i16, __builtin_IB_simd_block_write_2_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_us4, ushort4, ushort, i16, v4i16, __builtin_IB_simd_block_write_4_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_us8, ushort8, ushort, i16, v8i16, __builtin_IB_simd_block_write_8_global_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(intel_sub_group_block_write_us16, ushort16, ushort, i16, v16i16, __builtin_IB_simd_block_write_16_global_h)

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

#ifdef cl_intel_subgroup_buffer_prefetch
#define DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(FUNC_POSTFIX, PTR_TYPE, PTR_TYPE_MANGLING, NUM_BYTES) \
void OVERLOADABLE intel_sub_group_block_prefetch_##FUNC_POSTFIX(const global PTR_TYPE* p)         \
{                                                                                                 \
    __spirv_SubgroupBlockPrefetchINTEL(p, NUM_BYTES);            \
}

DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(uc,   uchar,  i8, 1)
DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(uc2,  uchar,  i8, 2)
DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(uc4,  uchar,  i8, 4)
DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(uc8,  uchar,  i8, 8)
DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(uc16, uchar,  i8, 16)

DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(us,   ushort, i16, 2)
DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(us2,  ushort, i16, 4)
DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(us4,  ushort, i16, 8)
DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(us8,  ushort, i16, 16)
DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(us16, ushort, i16, 32)

DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(ui,   uint,   i32, 4)
DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(ui2,  uint,   i32, 8)
DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(ui4,  uint,   i32, 16)
DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(ui8,  uint,   i32, 32)

DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(ul,   ulong,  i64, 8)
DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(ul2,  ulong,  i64, 16)
DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(ul4,  ulong,  i64, 32)
DEFN_INTEL_SUB_GROUP_BLOCK_PREFETCH(ul8,  ulong,  i64, 64)
#endif // cl_intel_subgroup_buffer_prefetch

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
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(intel_sub_group_block_read_us16, ushort16, ushort, i16, __builtin_IB_simd_block_read_16_local_h)

DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_us,  ushort,  ushort, i16, i16,   __builtin_IB_simd_block_write_1_local_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_us2, ushort2, ushort, i16, v2i16, __builtin_IB_simd_block_write_2_local_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_us4, ushort4, ushort, i16, v4i16, __builtin_IB_simd_block_write_4_local_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_us8, ushort8, ushort, i16, v8i16, __builtin_IB_simd_block_write_8_local_h)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(intel_sub_group_block_write_us16, ushort16, ushort, i16, v16i16, __builtin_IB_simd_block_write_16_local_h)

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

#if defined(cl_intel_subgroup_extended_block_read)
#define  DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_FLAT(FUNC_NAME, TYPE, INTERNAL_FUNC)                  \
INLINE TYPE FUNC_NAME( __global void* base_address, int width, int height, int pitch, int2 coord ) \
{                                                                                                  \
    long baseoffset = as_long(base_address);                                                       \
    int width_minus_one = width - 1;                                                               \
    int height_minus_one = height - 1;                                                             \
    int pitch_minus_one = pitch - 1;                                                               \
    return INTERNAL_FUNC(baseoffset, width_minus_one, height_minus_one, pitch_minus_one, coord);   \
}
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_FLAT(intel_subgroup_block_read_u8_m1k32v2,   ushort2,  __builtin_IB_subgroup_block_read_flat_u8_m1k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_FLAT(intel_subgroup_block_read_u8_m2k32v2,   ushort4,  __builtin_IB_subgroup_block_read_flat_u8_m2k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_FLAT(intel_subgroup_block_read_u8_m4k32v2,   ushort8,  __builtin_IB_subgroup_block_read_flat_u8_m4k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_FLAT(intel_subgroup_block_read_u8_m8k32v2,   ushort16, __builtin_IB_subgroup_block_read_flat_u8_m8k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_FLAT(intel_subgroup_block_read_u16_m1k16v2,  ushort2,  __builtin_IB_subgroup_block_read_flat_u16_m1k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_FLAT(intel_subgroup_block_read_u16_m2k16v2,  ushort4,  __builtin_IB_subgroup_block_read_flat_u16_m2k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_FLAT(intel_subgroup_block_read_u16_m4k16v2,  ushort8,  __builtin_IB_subgroup_block_read_flat_u16_m4k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_FLAT(intel_subgroup_block_read_u16_m8k16v2,  ushort16, __builtin_IB_subgroup_block_read_flat_u16_m8k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_FLAT(intel_subgroup_block_read_transform_u8_k32,  uint8, __builtin_IB_subgroup_block_read_flat_transform_u8_k32)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_FLAT(intel_subgroup_block_read_transform_u16_k16, uint8, __builtin_IB_subgroup_block_read_flat_transform_u16_k16)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_FLAT(intel_subgroup_block_read_transpose_u32_k8,  uint8, __builtin_IB_subgroup_block_read_flat_transpose_u32_k8)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_FLAT(intel_subgroup_block_read_transpose_u64_k4,  ulong4,__builtin_IB_subgroup_block_read_flat_transpose_u64_k4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_FLAT(intel_subgroup_block_read_transpose_u32_k16, uint16, __builtin_IB_subgroup_block_read_flat_transpose_u32_k16)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_FLAT(intel_subgroup_block_read_transpose_u64_k8,  ulong8, __builtin_IB_subgroup_block_read_flat_transpose_u64_k8)
#endif // defined(cl_intel_subgroup_extended_block_read)

#if defined(cl_intel_subgroup_extended_block_read_cacheopts)
enum LSC_LDCC mapToInternalReadCacheControl(intel_read_cache_control cache_control)
{
    switch (cache_control)
    {
    case read_cache_control_default_intel:
        return LSC_LDCC_DEFAULT;
    case read_cache_control_l1_uncached_l3_uncached_intel:
        return LSC_LDCC_L1UC_L3UC;
    case read_cache_control_l1_uncached_l3_cached_intel:
        return LSC_LDCC_L1UC_L3C;
    case read_cache_control_l1_cached_l3_uncached_intel:
        return LSC_LDCC_L1C_L3UC;
    case read_cache_control_l1_cached_l3_cached_intel:
        return LSC_LDCC_L1C_L3C;
    case read_cache_control_l1_streaming_l3_uncached_intel:
        return LSC_LDCC_L1S_L3UC;
    case read_cache_control_l1_streaming_l3_cached_intel:
        return LSC_LDCC_L1S_L3C;
    case read_cache_control_l1_iar_l3_cached_intel:
        return LSC_LDCC_L1IAR_L3C;
    default:
        return LSC_LDCC_DEFAULT;
    }
}

#define  DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(FUNC_NAME, TYPE, INTERNAL_FUNC)                  \
INLINE TYPE FUNC_NAME( __global void* base_address, int width, int height, int pitch, int2 coord, intel_read_cache_control cache_control ) \
{                                                                                                  \
    long baseoffset = as_long(base_address);                                                       \
    int width_minus_one = width - 1;                                                               \
    int height_minus_one = height - 1;                                                             \
    int pitch_minus_one = pitch - 1;                                                               \
    enum LSC_LDCC cache_control_internal = mapToInternalReadCacheControl(cache_control);           \
    return INTERNAL_FUNC(baseoffset, width_minus_one, height_minus_one, pitch_minus_one, coord, cache_control_internal);   \
}
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_read_cacheopts_u8_m1k32v2,   ushort2,  __builtin_IB_subgroup_block_read_cacheopts_u8_m1k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_read_cacheopts_u8_m2k32v2,   ushort4,  __builtin_IB_subgroup_block_read_cacheopts_u8_m2k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_read_cacheopts_u8_m4k32v2,   ushort8,  __builtin_IB_subgroup_block_read_cacheopts_u8_m4k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_read_cacheopts_u8_m8k32v2,   ushort16, __builtin_IB_subgroup_block_read_cacheopts_u8_m8k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_read_cacheopts_u16_m1k16v2,  ushort2,  __builtin_IB_subgroup_block_read_cacheopts_u16_m1k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_read_cacheopts_u16_m2k16v2,  ushort4,  __builtin_IB_subgroup_block_read_cacheopts_u16_m2k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_read_cacheopts_u16_m4k16v2,  ushort8,  __builtin_IB_subgroup_block_read_cacheopts_u16_m4k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_read_cacheopts_u16_m8k16v2,  ushort16, __builtin_IB_subgroup_block_read_cacheopts_u16_m8k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_read_cacheopts_transform_u8_k32,  uint8, __builtin_IB_subgroup_block_read_cacheopts_transform_u8_k32)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_read_cacheopts_transform_u16_k16, uint8, __builtin_IB_subgroup_block_read_cacheopts_transform_u16_k16)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_read_cacheopts_transpose_u32_k8,  uint8, __builtin_IB_subgroup_block_read_cacheopts_transpose_u32_k8)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_read_cacheopts_transpose_u64_k4,  ulong4,__builtin_IB_subgroup_block_read_cacheopts_transpose_u64_k4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_read_cacheopts_transpose_u32_k16, uint16, __builtin_IB_subgroup_block_read_cacheopts_transpose_u32_k16)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_read_cacheopts_transpose_u64_k8,  ulong8, __builtin_IB_subgroup_block_read_cacheopts_transpose_u64_k8)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m1k32v2,   void,  __builtin_IB_subgroup_block_read_prefetch_u8_m1k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m2k32v2,   void,  __builtin_IB_subgroup_block_read_prefetch_u8_m2k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m4k32v2,   void,  __builtin_IB_subgroup_block_read_prefetch_u8_m4k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m8k32v2,   void,  __builtin_IB_subgroup_block_read_prefetch_u8_m8k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m1k16v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u16_m1k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m2k16v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u16_m2k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m4k16v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u16_m4k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m8k16v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u16_m8k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_transform_u8_k32,  void, __builtin_IB_subgroup_block_read_prefetch_transform_u8_k32)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_transform_u16_k16, void, __builtin_IB_subgroup_block_read_prefetch_transform_u16_k16)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_transpose_u32_k8,  void, __builtin_IB_subgroup_block_read_prefetch_transpose_u32_k8)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_transpose_u64_k4,  void, __builtin_IB_subgroup_block_read_prefetch_transpose_u64_k4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_transpose_u32_k16, void, __builtin_IB_subgroup_block_read_prefetch_transpose_u32_k16)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_transpose_u64_k8,  void, __builtin_IB_subgroup_block_read_prefetch_transpose_u64_k8)
// 2d block wide prefetch
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m1k64v4,   void,  __builtin_IB_subgroup_block_read_prefetch_u8_m1k64v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m1k128v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u8_m1k128v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m1k256v1,  void,  __builtin_IB_subgroup_block_read_prefetch_u8_m1k256v1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m2k64v4,   void,  __builtin_IB_subgroup_block_read_prefetch_u8_m2k64v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m2k128v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u8_m2k128v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m2k256v1,  void,  __builtin_IB_subgroup_block_read_prefetch_u8_m2k256v1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m4k64v4,   void,  __builtin_IB_subgroup_block_read_prefetch_u8_m4k64v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m4k128v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u8_m4k128v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m4k256v1,  void,  __builtin_IB_subgroup_block_read_prefetch_u8_m4k256v1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m8k64v4,   void,  __builtin_IB_subgroup_block_read_prefetch_u8_m8k64v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m8k128v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u8_m8k128v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u8_m8k256v1,  void,  __builtin_IB_subgroup_block_read_prefetch_u8_m8k256v1)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m1k32v4,  void,  __builtin_IB_subgroup_block_read_prefetch_u16_m1k32v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m1k64v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u16_m1k64v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m1k128v1, void,  __builtin_IB_subgroup_block_read_prefetch_u16_m1k128v1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m2k32v4,  void,  __builtin_IB_subgroup_block_read_prefetch_u16_m2k32v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m2k64v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u16_m2k64v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m2k128v1, void,  __builtin_IB_subgroup_block_read_prefetch_u16_m2k128v1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m4k32v4,  void,  __builtin_IB_subgroup_block_read_prefetch_u16_m4k32v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m4k64v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u16_m4k64v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m4k128v1, void,  __builtin_IB_subgroup_block_read_prefetch_u16_m4k128v1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m8k32v4,  void,  __builtin_IB_subgroup_block_read_prefetch_u16_m8k32v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m8k64v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u16_m8k64v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u16_m8k128v1, void,  __builtin_IB_subgroup_block_read_prefetch_u16_m8k128v1)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u32_m1k16v4,  void,  __builtin_IB_subgroup_block_read_prefetch_u32_m1k16v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u32_m1k32v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u32_m1k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u32_m1k64v1,  void,  __builtin_IB_subgroup_block_read_prefetch_u32_m1k64v1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u32_m2k16v4,  void,  __builtin_IB_subgroup_block_read_prefetch_u32_m2k16v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u32_m2k32v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u32_m2k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u32_m2k64v1,  void,  __builtin_IB_subgroup_block_read_prefetch_u32_m2k64v1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u32_m4k16v4,  void,  __builtin_IB_subgroup_block_read_prefetch_u32_m4k16v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u32_m4k32v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u32_m4k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u32_m4k64v1,  void,  __builtin_IB_subgroup_block_read_prefetch_u32_m4k64v1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u32_m8k16v4,  void,  __builtin_IB_subgroup_block_read_prefetch_u32_m8k16v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u32_m8k32v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u32_m8k32v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u32_m8k64v1,  void,  __builtin_IB_subgroup_block_read_prefetch_u32_m8k64v1)

DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u64_m1k8v4,   void,  __builtin_IB_subgroup_block_read_prefetch_u64_m1k8v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u64_m1k16v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u64_m1k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u64_m1k32v1,  void,  __builtin_IB_subgroup_block_read_prefetch_u64_m1k32v1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u64_m2k8v4,   void,  __builtin_IB_subgroup_block_read_prefetch_u64_m2k8v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u64_m2k16v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u64_m2k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u64_m2k32v1,  void,  __builtin_IB_subgroup_block_read_prefetch_u64_m2k32v1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u64_m4k8v4,   void,  __builtin_IB_subgroup_block_read_prefetch_u64_m4k8v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u64_m4k16v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u64_m4k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u64_m4k32v1,  void,  __builtin_IB_subgroup_block_read_prefetch_u64_m4k32v1)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u64_m8k8v4,   void,  __builtin_IB_subgroup_block_read_prefetch_u64_m8k8v4)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u64_m8k16v2,  void,  __builtin_IB_subgroup_block_read_prefetch_u64_m8k16v2)
DEFN_INTEL_SUB_GROUP_BLOCK_READ_LSC_CACHEOPTS(intel_subgroup_block_prefetch_u64_m8k32v1,  void,  __builtin_IB_subgroup_block_read_prefetch_u64_m8k32v1)
#endif // defined(cl_intel_subgroup_extended_block_read_cacheopts)

#if defined(cl_intel_subgroup_extended_block_write_cacheopts)
enum LSC_STCC mapToInternalWriteCacheControl(intel_write_cache_control cache_control)
{
    switch (cache_control)
    {
    case write_cache_control_default_intel:
        return LSC_STCC_DEFAULT;
    case write_cache_control_l1_uncached_l3_uncached_intel:
        return LSC_STCC_L1UC_L3UC;
    case write_cache_control_l1_uncached_l3_writeback_intel:
        return LSC_STCC_L1UC_L3WB;
    case write_cache_control_l1_writethrough_l3_uncached_intel:
        return LSC_STCC_L1WT_L3UC;
    case write_cache_control_l1_writethrough_l3_writeback_intel:
        return LSC_STCC_L1WT_L3WB;
    case write_cache_control_l1_streaming_l3_uncached_intel:
        return LSC_STCC_L1S_L3UC;
    case write_cache_control_l1_streaming_l3_writeback_intel:
        return LSC_STCC_L1S_L3WB;
    case write_cache_control_l1_writeback_l3_writeback_intel:
        return LSC_STCC_L1WB_L3WB;
    default:
        return LSC_STCC_DEFAULT;
    }
}

#define  DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LSC_CACHEOPTS(FUNC_NAME, TYPE, INTERNAL_FUNC)                  \
INLINE void FUNC_NAME( __global void* base_address, int width, int height, int pitch, int2 coord, TYPE val, intel_write_cache_control cache_control ) \
{                                                                                                  \
    long baseoffset = as_long(base_address);                                                       \
    int width_minus_one = width - 1;                                                               \
    int height_minus_one = height - 1;                                                             \
    int pitch_minus_one = pitch - 1;                                                               \
    enum LSC_STCC cache_control_internal = mapToInternalWriteCacheControl(cache_control);          \
    INTERNAL_FUNC(baseoffset, width_minus_one, height_minus_one, pitch_minus_one, coord, val, cache_control_internal);   \
}
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LSC_CACHEOPTS(intel_subgroup_block_write_cacheopts_u8_m1k32v1,   ushort,  __builtin_IB_subgroup_block_write_cacheopts_u8_m1k32v1)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LSC_CACHEOPTS(intel_subgroup_block_write_cacheopts_u8_m2k32v1,   ushort2, __builtin_IB_subgroup_block_write_cacheopts_u8_m2k32v1)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LSC_CACHEOPTS(intel_subgroup_block_write_cacheopts_u8_m4k32v1,   ushort4, __builtin_IB_subgroup_block_write_cacheopts_u8_m4k32v1)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LSC_CACHEOPTS(intel_subgroup_block_write_cacheopts_u8_m8k32v1,   ushort8, __builtin_IB_subgroup_block_write_cacheopts_u8_m8k32v1)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LSC_CACHEOPTS(intel_subgroup_block_write_cacheopts_u16_m1k16v1,  ushort,  __builtin_IB_subgroup_block_write_cacheopts_u16_m1k16v1)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LSC_CACHEOPTS(intel_subgroup_block_write_cacheopts_u16_m2k16v1,  ushort2, __builtin_IB_subgroup_block_write_cacheopts_u16_m2k16v1)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LSC_CACHEOPTS(intel_subgroup_block_write_cacheopts_u16_m4k16v1,  ushort4, __builtin_IB_subgroup_block_write_cacheopts_u16_m4k16v1)
DEFN_INTEL_SUB_GROUP_BLOCK_WRITE_LSC_CACHEOPTS(intel_subgroup_block_write_cacheopts_u16_m8k16v1,  ushort8, __builtin_IB_subgroup_block_write_cacheopts_u16_m8k16v1)
#endif // defined(cl_intel_subgroup_extended_block_write_cacheopts)

#if defined(cl_intel_subgroup_2d_block_io)

// To define new cl_intel_subgroup_2d_block_io built-in + support new matching SPV_INTEL_2d_block_io built-in together, use
// DEFN_INTEL_SUB_GROUP_2D_BLOCK_ macros

// To support new SPV_INTEL_2d_block_io only without matching cl_intel_subgroup_2d_block_io built-in, use
// DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_ macros

#define DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ_CACHE_CONTROLS(FUNC_NAME, INTERNAL_DST_TYPE, INTERNAL_FUNC)                               \
INLINE void __internal_##FUNC_NAME##_cache_controls(__global void* base_address, int width, int height, int pitch, int2 coord, __private void* destination, enum LSC_LDCC cache_controls) \
{                                                                                                                                             \
    long baseoffset = as_long(base_address);                                                                                                  \
    int width_minus_one = width - 1;                                                                                                          \
    int height_minus_one = height - 1;                                                                                                        \
    int pitch_minus_one = pitch - 1;                                                                                                          \
    INTERNAL_DST_TYPE ret = INTERNAL_FUNC(baseoffset, width_minus_one, height_minus_one, pitch_minus_one, coord, cache_controls);             \
    *(__private INTERNAL_DST_TYPE*)destination = ret;                                                                                         \
}

#define DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(FUNC_NAME, INTERNAL_DST_TYPE, INTERNAL_FUNC)                                              \
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ_CACHE_CONTROLS(FUNC_NAME,        INTERNAL_DST_TYPE,             INTERNAL_FUNC)                    \
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ_CACHE_CONTROLS(FUNC_NAME##_sg32, HALVE_TYPE(INTERNAL_DST_TYPE), INTERNAL_FUNC##_sg32)

// The same 2D block dimensions use different data type per work item
// depending on the subgroup size. Define unique functions for each variant.
#define  DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(FUNC_NAME, DST_PTR_TYPE, INTERNAL_DST_TYPE, INTERNAL_FUNC)                                        \
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(FUNC_NAME, INTERNAL_DST_TYPE, INTERNAL_FUNC)                                                      \
INLINE void OVERLOADABLE FUNC_NAME(__global void* base_address, int width, int height, int pitch, int2 coord, __private DST_PTR_TYPE* destination) \
{                                                                                                                                             \
    __internal_##FUNC_NAME##_cache_controls(base_address, width, height, pitch, coord, (__private void *)destination, LSC_LDCC_DEFAULT);      \
}                                                                                                                                             \
INLINE void OVERLOADABLE FUNC_NAME##_sg32(__global void* base_address, int width, int height, int pitch, int2 coord, __private DST_PTR_TYPE* destination) \
{                                                                                                                                             \
    __internal_##FUNC_NAME##_sg32_cache_controls(base_address, width, height, pitch, coord, (__private void *)destination, LSC_LDCC_DEFAULT); \
}

// type d8, block width 8, array length 4
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_16r8x4c,   uchar32,    __builtin_IB_subgroup_block_read_cacheopts_u8_m16k8v4)

// type d8, block width 16, array length 1
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_1r16x1c,   uchar,      __builtin_IB_subgroup_block_read_cacheopts_u8_m1k16v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_2r16x1c,   uchar2,     __builtin_IB_subgroup_block_read_cacheopts_u8_m2k16v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_4r16x1c,   uchar4,     __builtin_IB_subgroup_block_read_cacheopts_u8_m4k16v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_8r16x1c,   uchar8,     __builtin_IB_subgroup_block_read_cacheopts_u8_m8k16v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_16r16x1c,  uchar16,    __builtin_IB_subgroup_block_read_cacheopts_u8_m16k16v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_32r16x1c,  uchar32,    __builtin_IB_subgroup_block_read_cacheopts_u8_m32k16v1)
// type d8, block width 16, array length 2
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_1r16x2c,   uchar2,     __builtin_IB_subgroup_block_read_cacheopts_u8_m1k16v2)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_2r16x2c,   uchar4,     __builtin_IB_subgroup_block_read_cacheopts_u8_m2k16v2)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_4r16x2c,   uchar8,     __builtin_IB_subgroup_block_read_cacheopts_u8_m4k16v2)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_8r16x2c,   uchar16,    __builtin_IB_subgroup_block_read_cacheopts_u8_m8k16v2)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_16r16x2c,  uchar32,    __builtin_IB_subgroup_block_read_cacheopts_u8_m16k16v2)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_32r16x2c,  uchar64,    __builtin_IB_subgroup_block_read_cacheopts_u8_m32k16v2)
// type d8, block width 16, array length 4
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_1r16x4c,   uchar4,     __builtin_IB_subgroup_block_read_cacheopts_u8_m1k16v4)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_2r16x4c,   uchar8,     __builtin_IB_subgroup_block_read_cacheopts_u8_m2k16v4)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_4r16x4c,   uchar16,    __builtin_IB_subgroup_block_read_cacheopts_u8_m4k16v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_8r16x4c,            uchar,      uchar32,  __builtin_IB_subgroup_block_read_cacheopts_u8_m8k16v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_16r16x4c,           uchar,      uchar64,  __builtin_IB_subgroup_block_read_cacheopts_u8_m16k16v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_32r16x4c,           uchar,      uchar128, __builtin_IB_subgroup_block_read_cacheopts_u8_m32k16v4)
// type d8, block width 32, array length 1
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_1r32x1c,            ushort,     ushort,   __builtin_IB_subgroup_block_read_cacheopts_u8_m1k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_2r32x1c,            ushort,     ushort2,  __builtin_IB_subgroup_block_read_cacheopts_u8_m2k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_4r32x1c,            ushort,     ushort4,  __builtin_IB_subgroup_block_read_cacheopts_u8_m4k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_8r32x1c,            ushort,     ushort8,  __builtin_IB_subgroup_block_read_cacheopts_u8_m8k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_16r32x1c,           ushort,     ushort16, __builtin_IB_subgroup_block_read_cacheopts_u8_m16k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_32r32x1c,           ushort,     ushort32, __builtin_IB_subgroup_block_read_cacheopts_u8_m32k32v1)
// type d8, block width 32, array length 2
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_1r32x2c,            ushort,     ushort2,  __builtin_IB_subgroup_block_read_cacheopts_u8_m1k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_2r32x2c,            ushort,     ushort4,  __builtin_IB_subgroup_block_read_cacheopts_u8_m2k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_4r32x2c,            ushort,     ushort8,  __builtin_IB_subgroup_block_read_cacheopts_u8_m4k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_8r32x2c,            ushort,     ushort16, __builtin_IB_subgroup_block_read_cacheopts_u8_m8k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_16r32x2c,           ushort,     ushort32, __builtin_IB_subgroup_block_read_cacheopts_u8_m16k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_32r32x2c,           ushort,     ushort64, __builtin_IB_subgroup_block_read_cacheopts_u8_m32k32v2)
// type d8, block width 64, array length 1
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_1r64x1c,   uint,      __builtin_IB_subgroup_block_read_cacheopts_u8_m1k64v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_2r64x1c,   uint2,     __builtin_IB_subgroup_block_read_cacheopts_u8_m2k64v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_4r64x1c,   uint4,     __builtin_IB_subgroup_block_read_cacheopts_u8_m4k64v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_8r64x1c,   uint8,     __builtin_IB_subgroup_block_read_cacheopts_u8_m8k64v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_16r64x1c,  uint16,    __builtin_IB_subgroup_block_read_cacheopts_u8_m16k64v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_8b_32r64x1c,  uint32,    __builtin_IB_subgroup_block_read_cacheopts_u8_m32k64v1)

// type d16, block width 8, array length 1
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_2r8x1c,   ushort,     __builtin_IB_subgroup_block_read_cacheopts_u16_m2k8v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_4r8x1c,   ushort2,    __builtin_IB_subgroup_block_read_cacheopts_u16_m4k8v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_8r8x1c,   ushort4,    __builtin_IB_subgroup_block_read_cacheopts_u16_m8k8v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_16r8x1c,  ushort8,    __builtin_IB_subgroup_block_read_cacheopts_u16_m16k8v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_32r8x1c,  ushort16,   __builtin_IB_subgroup_block_read_cacheopts_u16_m32k8v1)

// type d16, block width 8, array length 2
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_2r8x2c,   ushort2,    __builtin_IB_subgroup_block_read_cacheopts_u16_m2k8v2)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_4r8x2c,   ushort4,    __builtin_IB_subgroup_block_read_cacheopts_u16_m4k8v2)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_8r8x2c,   ushort8,    __builtin_IB_subgroup_block_read_cacheopts_u16_m8k8v2)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_16r8x2c,  ushort16,   __builtin_IB_subgroup_block_read_cacheopts_u16_m16k8v2)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_32r8x2c,  ushort32,   __builtin_IB_subgroup_block_read_cacheopts_u16_m32k8v2)

// type d16, block width 8, array length 4
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_2r8x4c,   ushort4,   __builtin_IB_subgroup_block_read_cacheopts_u16_m2k8v4)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_4r8x4c,   ushort8,   __builtin_IB_subgroup_block_read_cacheopts_u16_m4k8v4)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_8r8x4c,   ushort16,  __builtin_IB_subgroup_block_read_cacheopts_u16_m8k8v4)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_16r8x4c,  ushort32,  __builtin_IB_subgroup_block_read_cacheopts_u16_m16k8v4)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_32r8x4c,  ushort64,  __builtin_IB_subgroup_block_read_cacheopts_u16_m32k8v4)

// type d16, block width 16, array length 1
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_1r16x1c,           ushort,     ushort,   __builtin_IB_subgroup_block_read_cacheopts_u16_m1k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_2r16x1c,           ushort,     ushort2,  __builtin_IB_subgroup_block_read_cacheopts_u16_m2k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_4r16x1c,           ushort,     ushort4,  __builtin_IB_subgroup_block_read_cacheopts_u16_m4k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_8r16x1c,           ushort,     ushort8,  __builtin_IB_subgroup_block_read_cacheopts_u16_m8k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_16r16x1c,          ushort,     ushort16, __builtin_IB_subgroup_block_read_cacheopts_u16_m16k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_32r16x1c,          ushort,     ushort32, __builtin_IB_subgroup_block_read_cacheopts_u16_m32k16v1)
// type d16, block width 16, array length 2
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_1r16x2c,           ushort,     ushort2,  __builtin_IB_subgroup_block_read_cacheopts_u16_m1k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_2r16x2c,           ushort,     ushort4,  __builtin_IB_subgroup_block_read_cacheopts_u16_m2k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_4r16x2c,           ushort,     ushort8,  __builtin_IB_subgroup_block_read_cacheopts_u16_m4k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_8r16x2c,           ushort,     ushort16, __builtin_IB_subgroup_block_read_cacheopts_u16_m8k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_16r16x2c,          ushort,     ushort32, __builtin_IB_subgroup_block_read_cacheopts_u16_m16k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_32r16x2c,          ushort,     ushort64, __builtin_IB_subgroup_block_read_cacheopts_u16_m32k16v2)
// type d16, block width 32, array length 1
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_1r32x1c,   uint,      __builtin_IB_subgroup_block_read_cacheopts_u16_m1k32v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_2r32x1c,   uint2,     __builtin_IB_subgroup_block_read_cacheopts_u16_m2k32v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_4r32x1c,   uint4,     __builtin_IB_subgroup_block_read_cacheopts_u16_m4k32v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_8r32x1c,   uint8,     __builtin_IB_subgroup_block_read_cacheopts_u16_m8k32v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_16r32x1c,  uint16,    __builtin_IB_subgroup_block_read_cacheopts_u16_m16k32v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_16b_32r32x1c,  uint32,    __builtin_IB_subgroup_block_read_cacheopts_u16_m32k32v1)
// type d32, block width 8, array length 1
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_1r8x1c,            uint,       uint,     __builtin_IB_subgroup_block_read_cacheopts_u32_m1k8v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_2r8x1c,            uint,       uint,     __builtin_IB_subgroup_block_read_cacheopts_u32_m2k8v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_4r8x1c,            uint,       uint2,    __builtin_IB_subgroup_block_read_cacheopts_u32_m4k8v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_8r8x1c,            uint,       uint4,    __builtin_IB_subgroup_block_read_cacheopts_u32_m8k8v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_16r8x1c,           uint,       uint8,    __builtin_IB_subgroup_block_read_cacheopts_u32_m16k8v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_32r8x1c,           uint,       uint16,   __builtin_IB_subgroup_block_read_cacheopts_u32_m32k8v1)
// type d32, block width 8, array length 2
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_1r8x2c,            uint,       uint2,    __builtin_IB_subgroup_block_read_cacheopts_u32_m1k8v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_2r8x2c,            uint,       uint2,    __builtin_IB_subgroup_block_read_cacheopts_u32_m2k8v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_4r8x2c,            uint,       uint4,    __builtin_IB_subgroup_block_read_cacheopts_u32_m4k8v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_8r8x2c,            uint,       uint8,    __builtin_IB_subgroup_block_read_cacheopts_u32_m8k8v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_16r8x2c,           uint,       uint16,   __builtin_IB_subgroup_block_read_cacheopts_u32_m16k8v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_32r8x2c,           uint,       uint32,   __builtin_IB_subgroup_block_read_cacheopts_u32_m32k8v2)
// type d32, block width 16, array length 1
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_1r16x1c,           uint,       uint,     __builtin_IB_subgroup_block_read_cacheopts_u32_m1k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_2r16x1c,           uint,       uint2,    __builtin_IB_subgroup_block_read_cacheopts_u32_m2k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_4r16x1c,           uint,       uint4,    __builtin_IB_subgroup_block_read_cacheopts_u32_m4k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_8r16x1c,           uint,       uint8,    __builtin_IB_subgroup_block_read_cacheopts_u32_m8k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_16r16x1c,          uint,       uint16,   __builtin_IB_subgroup_block_read_cacheopts_u32_m16k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_32b_32r16x1c,          uint,       uint32,   __builtin_IB_subgroup_block_read_cacheopts_u32_m32k16v1)


DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transform_8b_32r16x1c,  uint,    uint8,    __builtin_IB_subgroup_block_read_cacheopts_transform_u8_k32)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transform_8b_32r16x2c,  uint,    uint16,    __builtin_IB_subgroup_block_read_cacheopts_transform_u8_k32n16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transform_8b_32r16x4c,  uint,    uint32,    __builtin_IB_subgroup_block_read_cacheopts_transform_u8_k32n16v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transform_16b_16r16x1c, uint,    uint8,    __builtin_IB_subgroup_block_read_cacheopts_transform_u16_k16)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transform_16b_32r16x1c,  uint,   uint16,    __builtin_IB_subgroup_block_read_cacheopts_transform_u16_k32n16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transform_16b_16r16x2c,  uint,   uint16,    __builtin_IB_subgroup_block_read_cacheopts_transform_u16_k16n16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transform_16b_32r16x2c,  uint,   uint32,    __builtin_IB_subgroup_block_read_cacheopts_transform_u16_k32n16v2)

DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transpose_32b_8r2x1c,   uint,     __builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m8k2)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transpose_32b_8r4x1c,   uint2,    __builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m8k4)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transpose_32b_8r8x1c,   uint4,    __builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m8k8)

DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transpose_32b_16r1x1c,  uint,     __builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m16k1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transpose_32b_16r2x1c,  uint2,    __builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m16k2)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transpose_32b_16r4x1c,  uint4,    __builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m16k4)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transpose_64b_8r1x1c, ulong,  __builtin_IB_subgroup_block_read_cacheopts_transpose_u64_m8k1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transpose_64b_8r2x1c, ulong, __builtin_IB_subgroup_block_read_cacheopts_transpose_u64_m8k2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transpose_64b_8r4x1c,   ulong, ulong4, __builtin_IB_subgroup_block_read_cacheopts_transpose_u64_m8k4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transpose_32b_16r8x1c,  uint,    uint8,    __builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m16k8)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transpose_32b_32r8x1c,  uint,    uint16,   __builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m32k8)


DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transpose_32b_16r16x1c, uint,  uint16, __builtin_IB_subgroup_block_read_cacheopts_transpose_u32_k16)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transpose_64b_16r8x1c, ulong,  ulong8, __builtin_IB_subgroup_block_read_cacheopts_transpose_u64_k8)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_READ(intel_sub_group_2d_block_read_transpose_64b_8r8x1c,  ulong,  ulong8, __builtin_IB_subgroup_block_read_cacheopts_transpose_u64_m8k8)


#define  DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(FUNC_NAME, INTERNAL_FUNC)                                                                   \
INLINE void __internal_##FUNC_NAME##_cache_controls( __global void* base_address, int width, int height, int pitch, int2 coord, enum LSC_LDCC cache_controls) \
{                                                                                                                                                    \
    long baseoffset = as_long(base_address);                                                                                                         \
    int width_minus_one = width - 1;                                                                                                                 \
    int height_minus_one = height - 1;                                                                                                               \
    int pitch_minus_one = pitch - 1;                                                                                                                 \
    return INTERNAL_FUNC(baseoffset, width_minus_one, height_minus_one, pitch_minus_one, coord, cache_controls);                                     \
}

#define  DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(FUNC_NAME, INTERNAL_FUNC)                                                                            \
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(FUNC_NAME, INTERNAL_FUNC)                                                                            \
INLINE void OVERLOADABLE FUNC_NAME( __global void* base_address, int width, int height, int pitch, int2 coord)                                       \
{                                                                                                                                                    \
    return __internal_##FUNC_NAME##_cache_controls(base_address, width, height, pitch, coord, LSC_LDCC_DEFAULT);                                              \
}

DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_1r32x1c,    __builtin_IB_subgroup_block_read_prefetch_u8_m1k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_2r32x1c,    __builtin_IB_subgroup_block_read_prefetch_u8_m2k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_4r32x1c,    __builtin_IB_subgroup_block_read_prefetch_u8_m4k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_8r32x1c,    __builtin_IB_subgroup_block_read_prefetch_u8_m8k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_16r32x1c,   __builtin_IB_subgroup_block_read_prefetch_u8_m16k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_32r32x1c,   __builtin_IB_subgroup_block_read_prefetch_u8_m32k32v1)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_1r32x2c,    __builtin_IB_subgroup_block_read_prefetch_u8_m1k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_2r32x2c,    __builtin_IB_subgroup_block_read_prefetch_u8_m2k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_4r32x2c,    __builtin_IB_subgroup_block_read_prefetch_u8_m4k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_8r32x2c,    __builtin_IB_subgroup_block_read_prefetch_u8_m8k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_16r32x2c,   __builtin_IB_subgroup_block_read_prefetch_u8_m16k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_32r32x2c,   __builtin_IB_subgroup_block_read_prefetch_u8_m32k32v2)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_8r16x4c,    __builtin_IB_subgroup_block_read_prefetch_u8_m8k16v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_16r16x4c,   __builtin_IB_subgroup_block_read_prefetch_u8_m16k16v4)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_32r16x1c,   __builtin_IB_subgroup_block_read_prefetch_u8_m32k16v1)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_1r16x1c,   __builtin_IB_subgroup_block_read_prefetch_u16_m1k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_2r16x1c,   __builtin_IB_subgroup_block_read_prefetch_u16_m2k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_4r16x1c,   __builtin_IB_subgroup_block_read_prefetch_u16_m4k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_8r16x1c,   __builtin_IB_subgroup_block_read_prefetch_u16_m8k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_16r16x1c,  __builtin_IB_subgroup_block_read_prefetch_u16_m16k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_32r16x1c,  __builtin_IB_subgroup_block_read_prefetch_u16_m32k16v1)

DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_1r8x1c,    __builtin_IB_subgroup_block_read_prefetch_u16_m1k8v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_2r8x1c,    __builtin_IB_subgroup_block_read_prefetch_u16_m2k8v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_4r8x1c,    __builtin_IB_subgroup_block_read_prefetch_u16_m4k8v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_16r8x1c,   __builtin_IB_subgroup_block_read_prefetch_u16_m16k8v1)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_1r16x2c,   __builtin_IB_subgroup_block_read_prefetch_u16_m1k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_2r16x2c,   __builtin_IB_subgroup_block_read_prefetch_u16_m2k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_4r16x2c,   __builtin_IB_subgroup_block_read_prefetch_u16_m4k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_8r16x2c,   __builtin_IB_subgroup_block_read_prefetch_u16_m8k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_16r16x2c,  __builtin_IB_subgroup_block_read_prefetch_u16_m16k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_32r16x2c,  __builtin_IB_subgroup_block_read_prefetch_u16_m32k16v2)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_1r8x1c,    __builtin_IB_subgroup_block_read_prefetch_u32_m1k8v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_2r8x1c,    __builtin_IB_subgroup_block_read_prefetch_u32_m2k8v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_4r8x1c,    __builtin_IB_subgroup_block_read_prefetch_u32_m4k8v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_8r8x1c,    __builtin_IB_subgroup_block_read_prefetch_u32_m8k8v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_16r8x1c,   __builtin_IB_subgroup_block_read_prefetch_u32_m16k8v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_32r8x1c,   __builtin_IB_subgroup_block_read_prefetch_u32_m32k8v1)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_1r16x1c,  __builtin_IB_subgroup_block_read_prefetch_u32_m1k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_2r16x1c,  __builtin_IB_subgroup_block_read_prefetch_u32_m2k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_4r16x1c,  __builtin_IB_subgroup_block_read_prefetch_u32_m4k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_8r16x1c,  __builtin_IB_subgroup_block_read_prefetch_u32_m8k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_16r16x1c, __builtin_IB_subgroup_block_read_prefetch_u32_m16k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_32r16x1c, __builtin_IB_subgroup_block_read_prefetch_u32_m32k16v1)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_1r8x2c,    __builtin_IB_subgroup_block_read_prefetch_u32_m1k8v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_2r8x2c,    __builtin_IB_subgroup_block_read_prefetch_u32_m2k8v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_4r8x2c,    __builtin_IB_subgroup_block_read_prefetch_u32_m4k8v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_8r8x2c,    __builtin_IB_subgroup_block_read_prefetch_u32_m8k8v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_16r8x2c,   __builtin_IB_subgroup_block_read_prefetch_u32_m16k8v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_32r8x2c,   __builtin_IB_subgroup_block_read_prefetch_u32_m32k8v2)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_32r16x2c,   __builtin_IB_subgroup_block_read_prefetch_u8_m32k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_32r16x4c,   __builtin_IB_subgroup_block_read_prefetch_u8_m32k16v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_32r8x1c,   __builtin_IB_subgroup_block_read_prefetch_u64_m32k8v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_16r8x1c,   __builtin_IB_subgroup_block_read_prefetch_u64_m16k8v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_8r8x1c,    __builtin_IB_subgroup_block_read_prefetch_u64_m8k8v1)

// 2d block wide prefetch
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_1r64x4c,    __builtin_IB_subgroup_block_read_prefetch_u8_m1k64v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_1r128x2c,   __builtin_IB_subgroup_block_read_prefetch_u8_m1k128v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_1r256x1c,   __builtin_IB_subgroup_block_read_prefetch_u8_m1k256v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_2r64x4c,    __builtin_IB_subgroup_block_read_prefetch_u8_m2k64v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_2r128x2c,   __builtin_IB_subgroup_block_read_prefetch_u8_m2k128v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_2r256x1c,   __builtin_IB_subgroup_block_read_prefetch_u8_m2k256v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_4r64x4c,    __builtin_IB_subgroup_block_read_prefetch_u8_m4k64v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_4r128x2c,   __builtin_IB_subgroup_block_read_prefetch_u8_m4k128v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_4r256x1c,   __builtin_IB_subgroup_block_read_prefetch_u8_m4k256v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_8r64x4c,    __builtin_IB_subgroup_block_read_prefetch_u8_m8k64v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_8r128x2c,   __builtin_IB_subgroup_block_read_prefetch_u8_m8k128v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_8r256x1c,   __builtin_IB_subgroup_block_read_prefetch_u8_m8k256v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_16r64x4c,   __builtin_IB_subgroup_block_read_prefetch_u8_m16k64v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_16r128x2c,  __builtin_IB_subgroup_block_read_prefetch_u8_m16k128v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_16r256x1c,  __builtin_IB_subgroup_block_read_prefetch_u8_m16k256v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_32r64x4c,   __builtin_IB_subgroup_block_read_prefetch_u8_m32k64v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_32r128x2c,  __builtin_IB_subgroup_block_read_prefetch_u8_m32k128v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_32r256x1c,  __builtin_IB_subgroup_block_read_prefetch_u8_m32k256v1)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_1r32x4c,   __builtin_IB_subgroup_block_read_prefetch_u16_m1k32v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_1r64x2c,   __builtin_IB_subgroup_block_read_prefetch_u16_m1k64v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_1r128x1c,  __builtin_IB_subgroup_block_read_prefetch_u16_m1k128v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_2r32x4c,   __builtin_IB_subgroup_block_read_prefetch_u16_m2k32v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_2r64x2c,   __builtin_IB_subgroup_block_read_prefetch_u16_m2k64v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_2r128x1c,  __builtin_IB_subgroup_block_read_prefetch_u16_m2k128v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_4r32x4c,   __builtin_IB_subgroup_block_read_prefetch_u16_m4k32v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_4r64x2c,   __builtin_IB_subgroup_block_read_prefetch_u16_m4k64v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_4r128x1c,  __builtin_IB_subgroup_block_read_prefetch_u16_m4k128v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_8r32x4c,   __builtin_IB_subgroup_block_read_prefetch_u16_m8k32v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_8r64x2c,   __builtin_IB_subgroup_block_read_prefetch_u16_m8k64v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_8r128x1c,  __builtin_IB_subgroup_block_read_prefetch_u16_m8k128v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_16r32x4c,  __builtin_IB_subgroup_block_read_prefetch_u16_m16k32v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_16r64x2c,  __builtin_IB_subgroup_block_read_prefetch_u16_m16k64v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_16r128x1c, __builtin_IB_subgroup_block_read_prefetch_u16_m16k128v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_32r32x4c,  __builtin_IB_subgroup_block_read_prefetch_u16_m32k32v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_32r64x2c,  __builtin_IB_subgroup_block_read_prefetch_u16_m32k64v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_16b_32r128x1c, __builtin_IB_subgroup_block_read_prefetch_u16_m32k128v1)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_1r16x4c,   __builtin_IB_subgroup_block_read_prefetch_u32_m1k16v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_1r32x2c,   __builtin_IB_subgroup_block_read_prefetch_u32_m1k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_1r64x1c,   __builtin_IB_subgroup_block_read_prefetch_u32_m1k64v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_2r16x4c,   __builtin_IB_subgroup_block_read_prefetch_u32_m2k16v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_2r32x2c,   __builtin_IB_subgroup_block_read_prefetch_u32_m2k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_2r64x1c,   __builtin_IB_subgroup_block_read_prefetch_u32_m2k64v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_4r16x4c,   __builtin_IB_subgroup_block_read_prefetch_u32_m4k16v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_4r32x2c,   __builtin_IB_subgroup_block_read_prefetch_u32_m4k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_4r64x1c,   __builtin_IB_subgroup_block_read_prefetch_u32_m4k64v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_8r16x4c,   __builtin_IB_subgroup_block_read_prefetch_u32_m8k16v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_8r32x2c,   __builtin_IB_subgroup_block_read_prefetch_u32_m8k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_8r64x1c,   __builtin_IB_subgroup_block_read_prefetch_u32_m8k64v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_16r16x4c,  __builtin_IB_subgroup_block_read_prefetch_u32_m16k16v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_16r32x2c,  __builtin_IB_subgroup_block_read_prefetch_u32_m16k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_16r64x1c,  __builtin_IB_subgroup_block_read_prefetch_u32_m16k64v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_32r16x4c,  __builtin_IB_subgroup_block_read_prefetch_u32_m32k16v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_32r32x2c,  __builtin_IB_subgroup_block_read_prefetch_u32_m32k32v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_32b_32r64x1c,  __builtin_IB_subgroup_block_read_prefetch_u32_m32k64v1)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_1r8x4c,    __builtin_IB_subgroup_block_read_prefetch_u64_m1k8v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_1r16x2c,   __builtin_IB_subgroup_block_read_prefetch_u64_m1k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_1r32x1c,   __builtin_IB_subgroup_block_read_prefetch_u64_m1k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_2r8x4c,    __builtin_IB_subgroup_block_read_prefetch_u64_m2k8v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_2r16x2c,   __builtin_IB_subgroup_block_read_prefetch_u64_m2k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_2r32x1c,   __builtin_IB_subgroup_block_read_prefetch_u64_m2k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_4r8x4c,    __builtin_IB_subgroup_block_read_prefetch_u64_m4k8v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_4r16x2c,   __builtin_IB_subgroup_block_read_prefetch_u64_m4k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_4r32x1c,   __builtin_IB_subgroup_block_read_prefetch_u64_m4k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_8r8x4c,    __builtin_IB_subgroup_block_read_prefetch_u64_m8k8v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_8r16x2c,   __builtin_IB_subgroup_block_read_prefetch_u64_m8k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_8r32x1c,   __builtin_IB_subgroup_block_read_prefetch_u64_m8k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_16r8x4c,   __builtin_IB_subgroup_block_read_prefetch_u64_m16k8v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_16r16x2c,  __builtin_IB_subgroup_block_read_prefetch_u64_m16k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_16r32x1c,  __builtin_IB_subgroup_block_read_prefetch_u64_m16k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_32r8x4c,   __builtin_IB_subgroup_block_read_prefetch_u64_m32k8v4)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_32r16x2c,  __builtin_IB_subgroup_block_read_prefetch_u64_m32k16v2)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_64b_32r32x1c,  __builtin_IB_subgroup_block_read_prefetch_u64_m32k32v1)

#define  DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_WRITE_CACHE_CONTROLS(FUNC_NAME, INTERNAL_DST_TYPE, INTERNAL_FUNC)       \
INLINE void __internal_##FUNC_NAME##_cache_controls(__global void* base_address, int width, int height, int pitch, int2 coord, private void* val, enum LSC_LDCC cache_controls) \
{                                                                                                                       \
    long baseoffset = as_long(base_address);                                                                            \
    int width_minus_one = width - 1;                                                                                    \
    int height_minus_one = height - 1;                                                                                  \
    int pitch_minus_one = pitch - 1;                                                                                    \
    INTERNAL_FUNC(baseoffset, width_minus_one, height_minus_one, pitch_minus_one, coord, *(private INTERNAL_DST_TYPE*)val, cache_controls); \
}

#define DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_WRITE(FUNC_NAME, INTERNAL_DST_TYPE, INTERNAL_FUNC)                                  \
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_WRITE_CACHE_CONTROLS(FUNC_NAME,        INTERNAL_DST_TYPE,             INTERNAL_FUNC)        \
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_WRITE_CACHE_CONTROLS(FUNC_NAME##_sg32, HALVE_TYPE(INTERNAL_DST_TYPE), INTERNAL_FUNC##_sg32)

// The same 2D block dimensions use different data type per work item
// depending on the subgroup size. Define unique functions for each variant.
#define  DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(FUNC_NAME, DST_PTR_TYPE, INTERNAL_DST_TYPE, INTERNAL_FUNC)                 \
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_WRITE(FUNC_NAME, INTERNAL_DST_TYPE, INTERNAL_FUNC)                               \
INLINE void OVERLOADABLE FUNC_NAME(__global void* base_address, int width, int height, int pitch, int2 coord, private DST_PTR_TYPE* val) \
{                                                                                                                       \
    __internal_##FUNC_NAME##_cache_controls(base_address, width, height, pitch, coord, (private void*) val, LSC_LDCC_DEFAULT); \
}                                                                                                                       \
INLINE void OVERLOADABLE FUNC_NAME##_sg32(__global void* base_address, int width, int height, int pitch, int2 coord, private DST_PTR_TYPE* val) \
{                                                                                                                                               \
    __internal_##FUNC_NAME##_sg32_cache_controls(base_address, width, height, pitch, coord, (private void*) val, LSC_LDCC_DEFAULT);             \
}

DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_8b_8r8x1c, uchar4, __builtin_IB_subgroup_block_write_cacheopts_u8_m8k8v1)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_8b_1r16x1c,   uchar, uchar,   __builtin_IB_subgroup_block_write_cacheopts_u8_m1k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_8b_2r16x1c,   uchar, uchar2,  __builtin_IB_subgroup_block_write_cacheopts_u8_m2k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_8b_4r16x1c,   uchar, uchar4,  __builtin_IB_subgroup_block_write_cacheopts_u8_m4k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_8b_8r16x1c,   uchar, uchar8,  __builtin_IB_subgroup_block_write_cacheopts_u8_m8k16v1)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_8b_1r32x1c,   ushort, ushort,  __builtin_IB_subgroup_block_write_cacheopts_u8_m1k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_8b_2r32x1c,   ushort, ushort2, __builtin_IB_subgroup_block_write_cacheopts_u8_m2k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_8b_4r32x1c,   ushort, ushort4, __builtin_IB_subgroup_block_write_cacheopts_u8_m4k32v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_8b_8r32x1c,   ushort, ushort8, __builtin_IB_subgroup_block_write_cacheopts_u8_m8k32v1)

DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_16b_8r8x1c, ushort4, __builtin_IB_subgroup_block_write_cacheopts_u16_m8k8v1)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_16b_1r16x1c,  ushort, ushort,  __builtin_IB_subgroup_block_write_cacheopts_u16_m1k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_16b_2r16x1c,  ushort, ushort2, __builtin_IB_subgroup_block_write_cacheopts_u16_m2k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_16b_4r16x1c,  ushort, ushort4, __builtin_IB_subgroup_block_write_cacheopts_u16_m4k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_16b_8r16x1c,  ushort, ushort8, __builtin_IB_subgroup_block_write_cacheopts_u16_m8k16v1)

DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_16b_1r32x1c, uint,  __builtin_IB_subgroup_block_write_cacheopts_u16_m1k32v1)
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_16b_8r32x1c, uint8, __builtin_IB_subgroup_block_write_cacheopts_u16_m8k32v1)

DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_32b_8r4x1c, uint2, __builtin_IB_subgroup_block_write_cacheopts_u32_m8k4v1)

DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_32b_8r8x1c, uint4, __builtin_IB_subgroup_block_write_cacheopts_u32_m8k8v1)

DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_32b_1r16x1c,   uint, uint,   __builtin_IB_subgroup_block_write_cacheopts_u32_m1k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_32b_2r16x1c,   uint, uint2,  __builtin_IB_subgroup_block_write_cacheopts_u32_m2k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_32b_4r16x1c,   uint, uint4,  __builtin_IB_subgroup_block_write_cacheopts_u32_m4k16v1)
DEFN_INTEL_SUB_GROUP_2D_BLOCK_WRITE(intel_sub_group_2d_block_write_32b_8r16x1c,   uint, uint8,  __builtin_IB_subgroup_block_write_cacheopts_u32_m8k16v1)
#endif // defined(cl_intel_subgroup_2d_block_io)

//OpSubgroup2DBlockPrefetchINTEL Element size: 1, Block Width: 8, Block Height: 16, Block Count: 4
DEFN_INTERNAL_INTEL_SUB_GROUP_2D_BLOCK_PREFETCH(intel_sub_group_2d_block_prefetch_8b_16r8x4c,   __builtin_IB_subgroup_block_read_prefetch_u8_m16k8v4)

#if defined(cl_khr_subgroup_shuffle)
#define DEFN_SUB_GROUP_SHUFFLE(TYPE, SPV_TYPE, TYPE_ABBR)                                                             \
INLINE TYPE OVERLOADABLE sub_group_shuffle(TYPE value, uint index)                                                    \
{                                                                                                                     \
    return __spirv_GroupNonUniformShuffle(Subgroup, as_##SPV_TYPE(value), index);    \
}                                                                                                                     \
INLINE TYPE OVERLOADABLE sub_group_shuffle_xor(TYPE value, uint mask)                                                 \
{                                                                                                                     \
    return __spirv_GroupNonUniformShuffleXor(Subgroup, as_##SPV_TYPE(value), mask);  \
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
    return __spirv_GroupNonUniformShuffleUp(Subgroup, as_##SPV_TYPE(value), delta);    \
}                                                                                                                       \
INLINE TYPE OVERLOADABLE sub_group_shuffle_down(TYPE value, uint delta)                                                 \
{                                                                                                                       \
    return __spirv_GroupNonUniformShuffleDown(Subgroup, as_##SPV_TYPE(value), delta);  \
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
