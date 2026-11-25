/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

uint OVERLOADABLE __intel_LocalInvocationId(uint dim);

// Broadcast Functions

// ***Note that technically the spec allows for 64 bit local Id's but currently our hardware cannot support
// this in which case we cast all ulong local Id's to uint's at this time.

#define BROADCAST_WORKGROUP(type)                                                                                      \
  {                                                                                                                    \
    GET_SAFE_MEMPOOL_PTR(tmp, type, false, 1)                                                                          \
    if ((__intel_LocalInvocationId(0) == LocalId.s0) & (__intel_LocalInvocationId(1) == LocalId.s1) &                  \
        (__intel_LocalInvocationId(2) == LocalId.s2)) {                                                                \
      *tmp = Value;                                                                                                    \
    }                                                                                                                  \
    __spirv_ControlBarrier(Execution, 0, AcquireRelease | WorkgroupMemory);                                            \
    type ret = *tmp;                                                                                                   \
    return ret;                                                                                                        \
  }

#define DEFN_SUB_GROUP_BROADCAST_VEC(__vargtype, __abbrvargtype)                                                       \
  GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupBroadcast, __vargtype, int, __vargtype, uint3, i32, __abbrvargtype,   \
                                            v3i32)                                                                     \
  GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupBroadcast, __vargtype, int, __vargtype, ulong3, i32, __abbrvargtype,  \
                                            v3i64)                                                                     \
  GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupBroadcast, __vargtype, int, __vargtype, uint2, i32, __abbrvargtype,   \
                                            v2i32)                                                                     \
  GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupBroadcast, __vargtype, int, __vargtype, ulong2, i32, __abbrvargtype,  \
                                            v2i64)                                                                     \
  GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupBroadcast, __vargtype, int, __vargtype, uint, i32, __abbrvargtype,    \
                                            i32)                                                                       \
  GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupBroadcast, __vargtype, int, __vargtype, ulong, i32, __abbrvargtype,   \
                                            i64)

bool __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, bool Value, uint3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(bool)
  } else if (Execution == Subgroup) {
    return __builtin_IB_simd_broadcast_b(Value, LocalId.s0);
  } else {
    return false;
  }
}

bool __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, bool Value, ulong3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(bool)
  } else if (Execution == Subgroup) {
    return __builtin_IB_simd_broadcast_b(Value, (uint)LocalId.s0);
  } else {
    return false;
  }
}

char __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, char Value, uint3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(uchar)
  } else if (Execution == Subgroup) {
    return __builtin_IB_simd_broadcast_c(Value, LocalId.s0);
  } else {
    return 0;
  }
}

char __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, char Value, ulong3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(uchar)
  } else if (Execution == Subgroup) {
    return __builtin_IB_simd_broadcast_c(Value, (uint)LocalId.s0);
  } else {
    return 0;
  }
}

short __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, short Value, uint3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(ushort)
  } else if (Execution == Subgroup) {
    return as_ushort(__builtin_IB_simd_broadcast_h(as_half(Value), LocalId.s0));
  } else {
    return 0;
  }
}

short __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, short Value, ulong3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(ushort)
  } else if (Execution == Subgroup) {
    return as_ushort(__builtin_IB_simd_broadcast_h(as_half(Value), (uint)LocalId.s0));
  } else {
    return 0;
  }
}

int __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, int Value, uint3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(uint)
  } else if (Execution == Subgroup) {
    return __builtin_IB_simd_broadcast(Value, LocalId.s0);
  } else {
    return 0;
  }
}

int __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, int Value, ulong3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(uint)
  } else if (Execution == Subgroup) {
    return __builtin_IB_simd_broadcast(Value, (uint)LocalId.s0);
  } else {
    return 0;
  }
}

long __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, long Value, uint3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(ulong)
  } else if (Execution == Subgroup) {
    return ((((ulong)__builtin_IB_simd_broadcast(Value >> 32, LocalId.s0)) << 32) |
            __builtin_IB_simd_broadcast((uint)Value, LocalId.s0));
  } else {
    return 0;
  }
}

long __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, long Value, ulong3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(ulong)
  } else if (Execution == Subgroup) {
    return ((((ulong)__builtin_IB_simd_broadcast(Value >> 32, (uint)LocalId.s0)) << 32) |
            __builtin_IB_simd_broadcast((uint)Value, (uint)LocalId.s0));
  } else {
    return 0;
  }
}

half __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, half Value, uint3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(half)
  } else if (Execution == Subgroup) {
    return __builtin_IB_simd_broadcast_h(Value, (uint)LocalId.s0);
  } else {
    return 0;
  }
}

half __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, half Value, ulong3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(half)
  } else if (Execution == Subgroup) {
    return as_half2(__builtin_IB_simd_broadcast((uint)(as_short(Value)), (uint)LocalId.s0)).x;
  } else {
    return 0;
  }
}

float __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, float Value, uint3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(float)
  } else if (Execution == Subgroup) {
    return __builtin_IB_simd_broadcast_f(Value, LocalId.s0);
  } else {
    return 0;
  }
}

float __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, float Value, ulong3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(float)
  } else if (Execution == Subgroup) {
    return __builtin_IB_simd_broadcast_f(Value, (uint)LocalId.s0);
  } else {
    return 0;
  }
}

#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, double Value, uint3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(double)
  } else if (Execution == Subgroup) {
    return __builtin_IB_simd_broadcast_df(Value, LocalId.s0);
  } else {
    return 0;
  }
}

double __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, double Value, ulong3 LocalId) {
  if (Execution == Workgroup) {
    BROADCAST_WORKGROUP(double)
  } else if (Execution == Subgroup) {
    return __builtin_IB_simd_broadcast_df(Value, (uint)LocalId.s0);
  } else {
    return 0;
  }
}
#endif

bool __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, bool Value, uint2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId.s0, LocalId.s1, 0));
}

bool __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, bool Value, ulong2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId.s0, LocalId.s1, 0));
}

char __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, char Value, uint2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId.s0, LocalId.s1, 0));
}

char __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, char Value, ulong2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId.s0, LocalId.s1, 0));
}

short __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, short Value, uint2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId.s0, LocalId.s1, 0));
}

short __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, short Value, ulong2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId.s0, LocalId.s1, 0));
}

int __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, int Value, uint2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId.s0, LocalId.s1, 0));
}

int __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, int Value, ulong2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId.s0, LocalId.s1, 0));
}

long __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, long Value, uint2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId.s0, LocalId.s1, 0));
}

long __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, long Value, ulong2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId.s0, LocalId.s1, 0));
}

half __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, half Value, uint2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId.s0, LocalId.s1, 0));
}

half __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, half Value, ulong2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId.s0, LocalId.s1, 0));
}

float __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, float Value, uint2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId.s0, LocalId.s1, 0));
}

float __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, float Value, ulong2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId.s0, LocalId.s1, 0));
}

#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, double Value, uint2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId.s0, LocalId.s1, 0));
}

double __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, double Value, ulong2 LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId.s0, LocalId.s1, 0));
}
#endif

bool __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, bool Value, uint LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId, 0, 0));
}

bool __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, bool Value, ulong LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId, 0, 0));
}

char __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, char Value, uint LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId, 0, 0));
}

char __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, char Value, ulong LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId, 0, 0));
}

short __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, short Value, uint LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId, 0, 0));
}

short __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, short Value, ulong LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId, 0, 0));
}

int __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, int Value, uint LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId, 0, 0));
}

int __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, int Value, ulong LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId, 0, 0));
}

long __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, long Value, uint LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId, 0, 0));
}

long __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, long Value, ulong LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId, 0, 0));
}

half __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, half Value, uint LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId, 0, 0));
}

half __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, half Value, ulong LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId, 0, 0));
}

float __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, float Value, uint LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId, 0, 0));
}

float __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, float Value, ulong LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId, 0, 0));
}

#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, double Value, uint LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (uint3)(LocalId, 0, 0));
}

double __attribute__((overloadable)) __spirv_GroupBroadcast(int Execution, double Value, ulong LocalId) {
  return __spirv_GroupBroadcast(Execution, Value, (ulong3)(LocalId, 0, 0));
}
#endif

DEFN_SUB_GROUP_BROADCAST_VEC(char, i8)
DEFN_SUB_GROUP_BROADCAST_VEC(short, i16)
DEFN_SUB_GROUP_BROADCAST_VEC(int, i32)
DEFN_SUB_GROUP_BROADCAST_VEC(long, i64)
DEFN_SUB_GROUP_BROADCAST_VEC(float, f32)

#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_BROADCAST_VEC(half, f16)
#endif

#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_BROADCAST_VEC(double, f64)
#endif
