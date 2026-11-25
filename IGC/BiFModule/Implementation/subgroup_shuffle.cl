/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// OpSubgroupShuffleUpINTEL
//
// Allows data to be transferred from an invocation in the subgroup with a lower SubgroupLocalInvocationId up to
// an invocation in the subgroup with a higher SubgroupLocalInvocationId.
#define DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(TYPE, TYPE_ABBR)                                                               \
  TYPE __attribute__((overloadable)) __spirv_SubgroupShuffleUpINTEL(TYPE Previous, TYPE Current, uint Value) {         \
    Value = __builtin_IB_get_simd_size() - Value;                                                                      \
    return __spirv_SubgroupShuffleDownINTEL(Previous, Current, Value);                                                 \
  }

// OpSubgroupShuffleXorINTEL
//
// Allows data to be transferred between invocations in a subgroup as a function of the invocation's
// SubgroupLocalInvocationId. The data that is returned for this invocation is the value of 'Data' for the invocation
// with SubgroupLocalInvocationId equal to this invocation's SubgroupLocalInvocationId XOR'd with the specified 'Value'.
#define DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(TYPE, TYPE_ABBR)                                                              \
  TYPE __attribute__((overloadable)) __spirv_SubgroupShuffleXorINTEL(TYPE Data, uint Value) {                          \
    Value = __builtin_IB_get_simd_id() ^ Value;                                                                        \
    return __spirv_SubgroupShuffleINTEL(Data, Value);                                                                  \
  }

#define DEFN_SUB_GROUP_SHUFFLE_XOR(TYPE, TYPE_ABBR)                                                                    \
  TYPE __attribute__((overloadable)) __spirv_GroupNonUniformShuffleXor(int Execution, TYPE x, uint c) {                \
    c = get_sub_group_local_id() ^ c;                                                                                  \
    return __spirv_GroupNonUniformShuffle(Execution, x, c);                                                            \
  }                                                                                                                    \
  GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffleXor, TYPE, int, TYPE, uint, i32, TYPE_ABBR, i32)

// OpSubgroupShuffleINTEL
//
// Allows data to be arbitrarily transferred between invocations in a subgroup.
// The data that is returned for this invocation is the value of 'Data' for the invocation identified by 'InvocationId'.
char __attribute__((overloadable)) __spirv_SubgroupShuffleINTEL(char Data, uint InvocationId) {
  if (BIF_FLAG_CTRL_GET(UseOOBChecks)) {
    InvocationId = InvocationId & (__spirv_BuiltInSubgroupMaxSize() - 1);
  }
  return __builtin_IB_simd_shuffle_c(Data, InvocationId);
}

short __attribute__((overloadable)) __spirv_SubgroupShuffleINTEL(short Data, uint InvocationId) {
  if (BIF_FLAG_CTRL_GET(UseOOBChecks)) {
    InvocationId = InvocationId & (__spirv_BuiltInSubgroupMaxSize() - 1);
  }
  return __builtin_IB_simd_shuffle_us(as_ushort(Data), InvocationId);
}

int __attribute__((overloadable)) __spirv_SubgroupShuffleINTEL(int Data, uint InvocationId) {
  if (BIF_FLAG_CTRL_GET(UseOOBChecks)) {
    InvocationId = InvocationId & (__spirv_BuiltInSubgroupMaxSize() - 1);
  }
  return __builtin_IB_simd_shuffle(as_uint(Data), InvocationId);
}

long __attribute__((overloadable)) __spirv_SubgroupShuffleINTEL(long Data, uint InvocationId) {
  int2 DataXY = as_int2(Data);
  int2 Result;
  Result.s0 = __spirv_SubgroupShuffleINTEL(DataXY.s0, InvocationId);
  Result.s1 = __spirv_SubgroupShuffleINTEL(DataXY.s1, InvocationId);
  return as_long(Result);
}

#ifdef cl_khr_fp16
half __attribute__((overloadable)) __spirv_SubgroupShuffleINTEL(half Data, uint InvocationId) {
  if (BIF_FLAG_CTRL_GET(UseOOBChecks)) {
    InvocationId = InvocationId & (__spirv_BuiltInSubgroupMaxSize() - 1);
  }
  return __builtin_IB_simd_shuffle_h(Data, InvocationId);
}
#endif

float __attribute__((overloadable)) __spirv_SubgroupShuffleINTEL(float Data, uint InvocationId) {
  if (BIF_FLAG_CTRL_GET(UseOOBChecks)) {
    InvocationId = InvocationId & (__spirv_BuiltInSubgroupMaxSize() - 1);
  }
  return __builtin_IB_simd_shuffle_f(Data, InvocationId);
}

#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_SubgroupShuffleINTEL(double Data, uint InvocationId) {
  if (BIF_FLAG_CTRL_GET(UseOOBChecks)) {
    InvocationId = InvocationId & (__spirv_BuiltInSubgroupMaxSize() - 1);
  }
  return __builtin_IB_simd_shuffle_df(Data, InvocationId);
}
#endif

GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleINTEL, char, char, uint, i8, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleINTEL, short, short, uint, i16, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleINTEL, int, int, uint, i32, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleINTEL, float, float, uint, f32, i32)

// OpSubgroupShuffleDownINTEL
//
// Allows data to be transferred from an invocation in the subgroup with a higher SubgroupLocalInvocationId down to
// a invocation in the subgroup with a lower SubgroupLocalInvocationId.
char __attribute__((overloadable)) __spirv_SubgroupShuffleDownINTEL(char Current, char Next, uint Delta) {
  return __builtin_IB_simd_shuffle_down_uc(Current, Next, Delta);
}

short __attribute__((overloadable)) __spirv_SubgroupShuffleDownINTEL(short Current, short Next, uint Delta) {
  return __builtin_IB_simd_shuffle_down_us(Current, Next, Delta);
}

int __attribute__((overloadable)) __spirv_SubgroupShuffleDownINTEL(int Current, int Next, uint Delta) {
  return __builtin_IB_simd_shuffle_down(Current, Next, Delta);
}

long __attribute__((overloadable)) __spirv_SubgroupShuffleDownINTEL(long Current, long Next, uint Delta) {
  int2 CurrentXY = as_int2(Current);
  int2 NextXY = as_int2(Next);
  int2 Result;
  Result.s0 = __spirv_SubgroupShuffleDownINTEL(CurrentXY.s0, NextXY.s0, Delta);
  Result.s1 = __spirv_SubgroupShuffleDownINTEL(CurrentXY.s1, NextXY.s1, Delta);
  return as_long(Result);
}

#ifdef cl_khr_fp16
half __attribute__((overloadable)) __spirv_SubgroupShuffleDownINTEL(half Current, half Next, uint Delta) {
  return as_half(__spirv_SubgroupShuffleDownINTEL(as_short(Current), as_short(Next), Delta));
}
#endif

float __attribute__((overloadable)) __spirv_SubgroupShuffleDownINTEL(float Current, float Next, uint Delta) {
  return as_float(__spirv_SubgroupShuffleDownINTEL(as_int(Current), as_int(Next), Delta));
}

#ifdef cl_khr_fp64
double __attribute__((overloadable)) __spirv_SubgroupShuffleDownINTEL(double Current, double Next, uint Delta) {
  return as_double(__spirv_SubgroupShuffleDownINTEL(as_long(Current), as_long(Next), Delta));
}
#endif

GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleDownINTEL, char, char, uint, i8, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleDownINTEL, short, short, uint, i16, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleDownINTEL, int, int, uint, i32, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleDownINTEL, float, float, uint, f32, i32)

DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(char, i8)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(short, i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(int, i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(long, i64)

#ifdef cl_khr_fp16
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(half, f16)
#endif

DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(float, f32)

#if defined(cl_khr_fp64)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(double, f64)
#endif

GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleUpINTEL, char, char, uint, i8, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleUpINTEL, short, short, uint, i16, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleUpINTEL, int, int, uint, i32, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleUpINTEL, float, float, uint, f32, i32)

DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(char, i8)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(short, i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(int, i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(long, i64)

#ifdef cl_khr_fp16
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(half, f16)
#endif

DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(float, f32)

#if defined(cl_khr_fp64)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(double, f64)
#endif

GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleXorINTEL, char, char, uint, i8, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleXorINTEL, short, short, uint, i16, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleXorINTEL, int, int, uint, i32, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleXorINTEL, float, float, uint, f32, i32)

#if defined(cl_khr_subgroup_shuffle)
char __attribute__((overloadable)) __spirv_GroupNonUniformShuffle(int Execution, char x, uint c) {
  if (Execution == Subgroup) {
    return as_uchar(__builtin_IB_simd_shuffle_c(as_char(x), c));
  }
  return 0;
}

short __attribute__((overloadable)) __spirv_GroupNonUniformShuffle(int Execution, short x, uint c) {
  if (Execution == Subgroup) {
    return __builtin_IB_simd_shuffle_us(x, c);
  }
  return 0;
}

int __attribute__((overloadable)) __spirv_GroupNonUniformShuffle(int Execution, int x, uint c) {
  if (Execution == Subgroup) {
    return __builtin_IB_simd_shuffle(x, c);
  }
  return 0;
}

long __attribute__((overloadable)) __spirv_GroupNonUniformShuffle(int Execution, long x, uint c) {
  if (Execution == Subgroup) {
    uint2 X = as_uint2(x);
    uint2 result = (uint2)(__builtin_IB_simd_shuffle(X.s0, c), __builtin_IB_simd_shuffle(X.s1, c));
    return as_ulong(result);
  }
  return 0;
}

float __attribute__((overloadable)) __spirv_GroupNonUniformShuffle(int Execution, float x, uint c) {
  if (Execution == Subgroup) {
    return __builtin_IB_simd_shuffle_f(x, c);
  }
  return 0;
}
#endif

#if defined(cl_khr_subgroup_shuffle)
#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_GroupNonUniformShuffle(int Execution, double x, uint c) {
  if (Execution == Subgroup) {
    if (BIF_FLAG_CTRL_GET(UseOOBChecks)) {
      c = c & (get_max_sub_group_size() - 1);
    }
    return __builtin_IB_simd_shuffle_df(x, c);
  }
  return 0;
}
#endif
#endif

#if defined(cl_khr_subgroup_shuffle)
#if defined(cl_khr_fp16)
half __attribute__((overloadable)) __spirv_GroupNonUniformShuffle(int Execution, half x, uint c) {
  if (Execution == Subgroup) {
    if (BIF_FLAG_CTRL_GET(UseOOBChecks)) {
      c = c & (get_max_sub_group_size() - 1);
    }
    return __builtin_IB_simd_shuffle_h(x, c);
  }
  return 0;
}
#endif
#endif

#if defined(cl_khr_subgroup_shuffle)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffle, char, int, char, uint, i32, i8, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffle, short, int, short, uint, i32, i16, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffle, int, int, int, uint, i32, i32, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffle, long, int, long, uint, i32, i64, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffle, float, int, float, uint, i32, f32, i32)
#endif

#if defined(cl_khr_subgroup_shuffle)
#if defined(cl_khr_fp64)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffle, double, int, double, uint, i32, f64, i32)
#endif
#endif

#if defined(cl_khr_subgroup_shuffle)
#if defined(cl_khr_fp16)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffle, half, int, half, uint, i32, f16, i32)
#endif
#endif

#if defined(cl_khr_subgroup_shuffle)
DEFN_SUB_GROUP_SHUFFLE_XOR(char, i8)
DEFN_SUB_GROUP_SHUFFLE_XOR(short, i16)
DEFN_SUB_GROUP_SHUFFLE_XOR(int, i32)
DEFN_SUB_GROUP_SHUFFLE_XOR(long, i64)
DEFN_SUB_GROUP_SHUFFLE_XOR(float, f32)
#endif

#if defined(cl_khr_subgroup_shuffle)
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_SHUFFLE_XOR(double, f64)
#endif
#endif

#if defined(cl_khr_subgroup_shuffle)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_SHUFFLE_XOR(half, f16)
#endif
#endif
