/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===-  IBiF_Reduce_Scan.cl -=================================================//
//
// This file defines versions of the Scan and Reduction work group and
// sub groups functions.
//
//===----------------------------------------------------------------------===//

// Private helper functions:
uint __intel_get_local_size( void );
uint __intel_get_enqueued_local_size( void );
uint __intel_get_local_linear_id( void );

#define DEFN_WORK_GROUP_REDUCE_UNSIGNED_ADD(type, abbr_type)                                               \
INLINE u##type OVERLOADABLE work_group_reduce_add(u##type x)                                               \
{                                                                                                          \
    return SPIRV_BUILTIN(GroupIAdd, _i32_i32_##abbr_type, )(Workgroup,GroupOperationReduce, as_##type(x)); \
}

#define DEFN_WORK_GROUP_REDUCE(type, op_name, new_name, abbr_type)                           \
INLINE type OVERLOADABLE work_group_reduce_##op_name(type x)                                 \
{                                                                                            \
    return SPIRV_BUILTIN(new_name, _i32_i32_##abbr_type, )(Workgroup,GroupOperationReduce,x);\
}

DEFN_WORK_GROUP_REDUCE(int,    max, GroupSMax, i32)
DEFN_WORK_GROUP_REDUCE(int,    min, GroupSMin, i32)
DEFN_WORK_GROUP_REDUCE(int,    add, GroupIAdd, i32)
DEFN_WORK_GROUP_REDUCE(uint,   max, GroupUMax, i32)
DEFN_WORK_GROUP_REDUCE(uint,   min, GroupUMin, i32)
DEFN_WORK_GROUP_REDUCE(float,  add, GroupFAdd, f32)
DEFN_WORK_GROUP_REDUCE(float,  max, GroupFMax, f32)
DEFN_WORK_GROUP_REDUCE(float,  min, GroupFMin, f32)
DEFN_WORK_GROUP_REDUCE_UNSIGNED_ADD(int, i32)
DEFN_WORK_GROUP_REDUCE_UNSIGNED_ADD(long, i64)
DEFN_WORK_GROUP_REDUCE(long,   max, GroupSMax, i64)
DEFN_WORK_GROUP_REDUCE(long,   min, GroupSMin, i64)
DEFN_WORK_GROUP_REDUCE(long,   add, GroupIAdd, i64)
DEFN_WORK_GROUP_REDUCE(ulong,  max, GroupUMax, i64)
DEFN_WORK_GROUP_REDUCE(ulong,  min, GroupUMin, i64)
#ifdef cl_khr_fp16
DEFN_WORK_GROUP_REDUCE(half,   add, GroupFAdd, f16)
DEFN_WORK_GROUP_REDUCE(half,   max, GroupFMax, f16)
DEFN_WORK_GROUP_REDUCE(half,   min, GroupFMin, f16)
#endif
#if defined(cl_khr_fp64)
DEFN_WORK_GROUP_REDUCE(double, add, GroupFAdd, f64)
DEFN_WORK_GROUP_REDUCE(double, max, GroupFMax, f64)
DEFN_WORK_GROUP_REDUCE(double, min, GroupFMin, f64)
#endif

#define DEFN_WORK_GROUP_SCAN_INCL_UNSIGNED_ADD(type, abbr_type)                                                    \
INLINE u##type OVERLOADABLE work_group_scan_inclusive_add(u##type x)                                               \
{                                                                                                                  \
    return SPIRV_BUILTIN(GroupIAdd, _i32_i32_##abbr_type, )(Workgroup,GroupOperationInclusiveScan, as_##type(x));  \
}

#define DEFN_WORK_GROUP_SCAN_INCL(type, op_name, new_name, abbr_type)                                 \
INLINE type OVERLOADABLE work_group_scan_inclusive_##op_name(type x)                                  \
{                                                                                                     \
    return SPIRV_BUILTIN(new_name, _i32_i32_##abbr_type, )(Workgroup,GroupOperationInclusiveScan,x);  \
}

DEFN_WORK_GROUP_SCAN_INCL(int,    max, GroupSMax, i32)
DEFN_WORK_GROUP_SCAN_INCL(int,    min, GroupSMin, i32)
DEFN_WORK_GROUP_SCAN_INCL(int,    add, GroupIAdd, i32)
DEFN_WORK_GROUP_SCAN_INCL(uint,   max, GroupUMax, i32)
DEFN_WORK_GROUP_SCAN_INCL(uint,   min, GroupUMin, i32)
DEFN_WORK_GROUP_SCAN_INCL(float,  add, GroupFAdd, f32)
DEFN_WORK_GROUP_SCAN_INCL(float,  max, GroupFMax, f32)
DEFN_WORK_GROUP_SCAN_INCL(float,  min, GroupFMin, f32)
DEFN_WORK_GROUP_SCAN_INCL_UNSIGNED_ADD(int, i32)
DEFN_WORK_GROUP_SCAN_INCL_UNSIGNED_ADD(long, i64)
DEFN_WORK_GROUP_SCAN_INCL(long,   max, GroupSMax, i64)
DEFN_WORK_GROUP_SCAN_INCL(long,   min, GroupSMin, i64)
DEFN_WORK_GROUP_SCAN_INCL(long,   add, GroupIAdd, i64)
DEFN_WORK_GROUP_SCAN_INCL(ulong,  max, GroupUMax, i64)
DEFN_WORK_GROUP_SCAN_INCL(ulong,  min, GroupUMin, i64)
#ifdef cl_khr_fp16
DEFN_WORK_GROUP_SCAN_INCL(half,   add, GroupFAdd, f16)
DEFN_WORK_GROUP_SCAN_INCL(half,   max, GroupFMax, f16)
DEFN_WORK_GROUP_SCAN_INCL(half,   min, GroupFMin, f16)
#endif
#if defined(cl_khr_fp64)
DEFN_WORK_GROUP_SCAN_INCL(double, add, GroupFAdd, f64)
DEFN_WORK_GROUP_SCAN_INCL(double, max, GroupFMax, f64)
DEFN_WORK_GROUP_SCAN_INCL(double, min, GroupFMin, f64)
#endif

#define DEFN_WORK_GROUP_SCAN_EXCL_UNSIGNED_ADD(type, abbr_type)                                 \
INLINE u##type OVERLOADABLE work_group_scan_exclusive_add(u##type x)                            \
{                                                                                               \
    return SPIRV_BUILTIN(GroupIAdd, _i32_i32_##abbr_type, )(Workgroup,GroupOperationExclusiveScan, as_##type(x));\
}

#define DEFN_WORK_GROUP_SCAN_EXCL(type, op_name, new_name, abbr_type)                           \
INLINE type OVERLOADABLE work_group_scan_exclusive_##op_name(type x)                            \
{                                                                                               \
    return SPIRV_BUILTIN(new_name, _i32_i32_##abbr_type, )(Workgroup,GroupOperationExclusiveScan,x);\
}

DEFN_WORK_GROUP_SCAN_EXCL(int,    max, GroupSMax, i32)
DEFN_WORK_GROUP_SCAN_EXCL(int,    min, GroupSMin, i32)
DEFN_WORK_GROUP_SCAN_EXCL(int,    add, GroupIAdd, i32)
DEFN_WORK_GROUP_SCAN_EXCL(uint,   max, GroupUMax, i32)
DEFN_WORK_GROUP_SCAN_EXCL(uint,   min, GroupUMin, i32)
DEFN_WORK_GROUP_SCAN_EXCL(float,  add, GroupFAdd, f32)
DEFN_WORK_GROUP_SCAN_EXCL(float,  max, GroupFMax, f32)
DEFN_WORK_GROUP_SCAN_EXCL(float,  min, GroupFMin, f32)
DEFN_WORK_GROUP_SCAN_EXCL_UNSIGNED_ADD(int, i32)
DEFN_WORK_GROUP_SCAN_EXCL_UNSIGNED_ADD(long, i64)
DEFN_WORK_GROUP_SCAN_EXCL(long,   max, GroupSMax, i64)
DEFN_WORK_GROUP_SCAN_EXCL(long,   min, GroupSMin, i64)
DEFN_WORK_GROUP_SCAN_EXCL(long,   add, GroupIAdd, i64)
DEFN_WORK_GROUP_SCAN_EXCL(ulong,  max, GroupUMax, i64)
DEFN_WORK_GROUP_SCAN_EXCL(ulong,  min, GroupUMin, i64)
#ifdef cl_khr_fp16
DEFN_WORK_GROUP_SCAN_EXCL(half,   add, GroupFAdd, f16)
DEFN_WORK_GROUP_SCAN_EXCL(half,   max, GroupFMax, f16)
DEFN_WORK_GROUP_SCAN_EXCL(half,   min, GroupFMin, f16)
#endif
#if defined(cl_khr_fp64)
DEFN_WORK_GROUP_SCAN_EXCL(double, add, GroupFAdd, f64)
DEFN_WORK_GROUP_SCAN_EXCL(double, max, GroupFMax, f64)
DEFN_WORK_GROUP_SCAN_EXCL(double, min, GroupFMin, f64)
#endif

#define DEFN_SUB_GROUP_REDUCE_UNSIGNED_ADD(type, abbr_type)                                                \
INLINE u##type OVERLOADABLE sub_group_reduce_add(u##type x)                                                \
{                                                                                                          \
    return SPIRV_BUILTIN(GroupIAdd, _i32_i32_##abbr_type, )(Subgroup,GroupOperationReduce, as_##type(x));  \
}

#define DEFN_SUB_GROUP_REDUCE(type, op_name, new_name, abbr_type)                             \
INLINE type OVERLOADABLE sub_group_reduce_##op_name(type x)                                   \
{                                                                                             \
    return SPIRV_BUILTIN(new_name, _i32_i32_##abbr_type, )(Subgroup,GroupOperationReduce,x);  \
}

#define DEFN_INTEL_SUB_GROUP_REDUCE_UNSIGNED_ADD(type, abbr_type)                                          \
INLINE u##type OVERLOADABLE intel_sub_group_reduce_add(u##type x)                                          \
{                                                                                                          \
    return SPIRV_BUILTIN(GroupIAdd, _i32_i32_##abbr_type, )(Subgroup,GroupOperationReduce, as_##type(x));  \
}

#define DEFN_INTEL_SUB_GROUP_REDUCE(type, op_name, new_name, abbr_type)                       \
INLINE type OVERLOADABLE intel_sub_group_reduce_##op_name(type x)                             \
{                                                                                             \
    return SPIRV_BUILTIN(new_name, _i32_i32_##abbr_type, )(Subgroup,GroupOperationReduce,x);  \
}

// 8bit
#if defined(cl_intel_subgroups_char)
DEFN_INTEL_SUB_GROUP_REDUCE(char,  max, GroupSMax, i8)
DEFN_INTEL_SUB_GROUP_REDUCE(char,  min, GroupSMin, i8)
DEFN_INTEL_SUB_GROUP_REDUCE(char,  add, GroupIAdd, i8)
DEFN_INTEL_SUB_GROUP_REDUCE(uchar, max, GroupUMax, i8)
DEFN_INTEL_SUB_GROUP_REDUCE(uchar, min, GroupUMin, i8)
DEFN_INTEL_SUB_GROUP_REDUCE_UNSIGNED_ADD(char, i8)
#endif // defined(cl_intel_subgroups_char)
// 16bit
#if defined(cl_intel_subgroups_short)
DEFN_INTEL_SUB_GROUP_REDUCE(short,  max, GroupSMax, i16)
DEFN_INTEL_SUB_GROUP_REDUCE(short,  min, GroupSMin, i16)
DEFN_INTEL_SUB_GROUP_REDUCE(short,  add, GroupIAdd, i16)
DEFN_INTEL_SUB_GROUP_REDUCE(ushort, max, GroupUMax, i16)
DEFN_INTEL_SUB_GROUP_REDUCE(ushort, min, GroupUMin, i16)
DEFN_INTEL_SUB_GROUP_REDUCE_UNSIGNED_ADD(short, i16)
#endif // defined(cl_intel_subgroups_short)
#if defined(cl_khr_subgroup_extended_types)
// 8bit
DEFN_SUB_GROUP_REDUCE(char,  max, GroupSMax, i8)
DEFN_SUB_GROUP_REDUCE(char,  min, GroupSMin, i8)
DEFN_SUB_GROUP_REDUCE(char,  add, GroupIAdd, i8)
DEFN_SUB_GROUP_REDUCE(uchar, max, GroupUMax, i8)
DEFN_SUB_GROUP_REDUCE(uchar, min, GroupUMin, i8)
DEFN_SUB_GROUP_REDUCE_UNSIGNED_ADD(char, i8)
// 16bit
DEFN_SUB_GROUP_REDUCE(short,  max, GroupSMax, i16)
DEFN_SUB_GROUP_REDUCE(short,  min, GroupSMin, i16)
DEFN_SUB_GROUP_REDUCE(short,  add, GroupIAdd, i16)
DEFN_SUB_GROUP_REDUCE(ushort, max, GroupUMax, i16)
DEFN_SUB_GROUP_REDUCE(ushort, min, GroupUMin, i16)
DEFN_SUB_GROUP_REDUCE_UNSIGNED_ADD(short, i16)
#endif // defined(cl_khr_subgroup_extended_types)
// 32bit
DEFN_SUB_GROUP_REDUCE(int,   max, GroupSMax, i32)
DEFN_SUB_GROUP_REDUCE(int,   min, GroupSMin, i32)
DEFN_SUB_GROUP_REDUCE(int,   add, GroupIAdd, i32)
DEFN_SUB_GROUP_REDUCE(uint,  max, GroupUMax, i32)
DEFN_SUB_GROUP_REDUCE(uint,  min, GroupUMin, i32)
DEFN_SUB_GROUP_REDUCE(float, add, GroupFAdd, f32)
DEFN_SUB_GROUP_REDUCE(float, max, GroupFMax, f32)
DEFN_SUB_GROUP_REDUCE(float, min, GroupFMin, f32)
DEFN_SUB_GROUP_REDUCE_UNSIGNED_ADD(int, i32)
// 64bit
DEFN_SUB_GROUP_REDUCE(long,  max, GroupSMax, i64)
DEFN_SUB_GROUP_REDUCE(long,  min, GroupSMin, i64)
DEFN_SUB_GROUP_REDUCE(long,  add, GroupIAdd, i64)
DEFN_SUB_GROUP_REDUCE(ulong, max, GroupUMax, i64)
DEFN_SUB_GROUP_REDUCE(ulong, min, GroupUMin, i64)
DEFN_SUB_GROUP_REDUCE_UNSIGNED_ADD(long, i64)
// half & double
#ifdef cl_khr_fp16
DEFN_SUB_GROUP_REDUCE(half, add, GroupFAdd, f16)
DEFN_SUB_GROUP_REDUCE(half, max, GroupFMax, f16)
DEFN_SUB_GROUP_REDUCE(half, min, GroupFMin, f16)
#endif
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_REDUCE(double, add, GroupFAdd, f64)
DEFN_SUB_GROUP_REDUCE(double, max, GroupFMax, f64)
DEFN_SUB_GROUP_REDUCE(double, min, GroupFMin, f64)
#endif

#define DEFN_SUB_GROUP_SCAN_INCL_UNSIGNED_ADD(type, abbr_type)                                                    \
INLINE u##type OVERLOADABLE sub_group_scan_inclusive_add(u##type x)                                               \
{                                                                                                                 \
    return SPIRV_BUILTIN(GroupIAdd, _i32_i32_##abbr_type, )(Subgroup,GroupOperationInclusiveScan, as_##type(x));  \
}

#define DEFN_SUB_GROUP_SCAN_INCL(type, op_name, new_name, abbr_type)                                 \
INLINE type OVERLOADABLE sub_group_scan_inclusive_##op_name(type x)                                  \
{                                                                                                    \
    return SPIRV_BUILTIN(new_name, _i32_i32_##abbr_type, )(Subgroup,GroupOperationInclusiveScan,x);  \
}

#define DEFN_INTEL_SUB_GROUP_SCAN_INCL_UNSIGNED_ADD(type, abbr_type)                                              \
INLINE u##type OVERLOADABLE intel_sub_group_scan_inclusive_add(u##type x)                                         \
{                                                                                                                 \
    return SPIRV_BUILTIN(GroupIAdd, _i32_i32_##abbr_type, )(Subgroup,GroupOperationInclusiveScan, as_##type(x));  \
}

#define DEFN_INTEL_SUB_GROUP_SCAN_INCL(type, op_name, new_name, abbr_type)                           \
INLINE type OVERLOADABLE intel_sub_group_scan_inclusive_##op_name(type x)                            \
{                                                                                                    \
    return SPIRV_BUILTIN(new_name, _i32_i32_##abbr_type, )(Subgroup,GroupOperationInclusiveScan,x);  \
}

// 8bit
#if defined(cl_intel_subgroups_char)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(char,  max, GroupSMax, i8)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(char,  min, GroupSMin, i8)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(char,  add, GroupIAdd, i8)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(uchar, max, GroupUMax, i8)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(uchar, min, GroupUMin, i8)
DEFN_INTEL_SUB_GROUP_SCAN_INCL_UNSIGNED_ADD(char, i8)
#endif // defined(cl_intel_subgroups_char)
// 16bit
#if defined(cl_intel_subgroups_short)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(short,  max, GroupSMax, i16)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(short,  min, GroupSMin, i16)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(short,  add, GroupIAdd, i16)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(ushort, max, GroupUMax, i16)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(ushort, min, GroupUMin, i16)
DEFN_INTEL_SUB_GROUP_SCAN_INCL_UNSIGNED_ADD(short, i16)
#endif // defined(cl_intel_subgroups_short)
#if defined(cl_khr_subgroup_extended_types)
// 8bit
DEFN_SUB_GROUP_SCAN_INCL(char,  max, GroupSMax, i8)
DEFN_SUB_GROUP_SCAN_INCL(char,  min, GroupSMin, i8)
DEFN_SUB_GROUP_SCAN_INCL(char,  add, GroupIAdd, i8)
DEFN_SUB_GROUP_SCAN_INCL(uchar, max, GroupUMax, i8)
DEFN_SUB_GROUP_SCAN_INCL(uchar, min, GroupUMin, i8)
DEFN_SUB_GROUP_SCAN_INCL_UNSIGNED_ADD(char, i8)
// 16bit
DEFN_SUB_GROUP_SCAN_INCL(short,  max, GroupSMax, i16)
DEFN_SUB_GROUP_SCAN_INCL(short,  min, GroupSMin, i16)
DEFN_SUB_GROUP_SCAN_INCL(short,  add, GroupIAdd, i16)
DEFN_SUB_GROUP_SCAN_INCL(ushort, max, GroupUMax, i16)
DEFN_SUB_GROUP_SCAN_INCL(ushort, min, GroupUMin, i16)
DEFN_SUB_GROUP_SCAN_INCL_UNSIGNED_ADD(short, i16)
#endif defined(cl_khr_subgroup_extended_types)
// 32bit
DEFN_SUB_GROUP_SCAN_INCL(int,   max, GroupSMax, i32)
DEFN_SUB_GROUP_SCAN_INCL(int,   min, GroupSMin, i32)
DEFN_SUB_GROUP_SCAN_INCL(int,   add, GroupIAdd, i32)
DEFN_SUB_GROUP_SCAN_INCL(uint,  max, GroupUMax, i32)
DEFN_SUB_GROUP_SCAN_INCL(uint,  min, GroupUMin, i32)
DEFN_SUB_GROUP_SCAN_INCL(float, add, GroupFAdd, f32)
DEFN_SUB_GROUP_SCAN_INCL(float, max, GroupFMax, f32)
DEFN_SUB_GROUP_SCAN_INCL(float, min, GroupFMin, f32)
DEFN_SUB_GROUP_SCAN_INCL_UNSIGNED_ADD(int, i32)
// 64bit
DEFN_SUB_GROUP_SCAN_INCL(long,  max, GroupSMax, i64)
DEFN_SUB_GROUP_SCAN_INCL(long,  min, GroupSMin, i64)
DEFN_SUB_GROUP_SCAN_INCL(long,  add, GroupIAdd, i64)
DEFN_SUB_GROUP_SCAN_INCL(ulong, max, GroupUMax, i64)
DEFN_SUB_GROUP_SCAN_INCL(ulong, min, GroupUMin, i64)
DEFN_SUB_GROUP_SCAN_INCL_UNSIGNED_ADD(long, i64)
// half & double
#ifdef cl_khr_fp16
DEFN_SUB_GROUP_SCAN_INCL(half, add, GroupFAdd, f16)
DEFN_SUB_GROUP_SCAN_INCL(half, max, GroupFMax, f16)
DEFN_SUB_GROUP_SCAN_INCL(half, min, GroupFMin, f16)
#endif
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_SCAN_INCL(double, add, GroupFAdd, f64)
DEFN_SUB_GROUP_SCAN_INCL(double, max, GroupFMax, f64)
DEFN_SUB_GROUP_SCAN_INCL(double, min, GroupFMin, f64)
#endif


#define DEFN_SUB_GROUP_SCAN_EXCL_UNSIGNED_ADD(type, abbr_type)                                                    \
INLINE u##type OVERLOADABLE sub_group_scan_exclusive_add(u##type x)                                               \
{                                                                                                                 \
    return SPIRV_BUILTIN(GroupIAdd, _i32_i32_##abbr_type, )(Subgroup,GroupOperationExclusiveScan, as_##type(x));  \
}

#define DEFN_SUB_GROUP_SCAN_EXCL(type, op_name, new_name, abbr_type)                                 \
INLINE type OVERLOADABLE sub_group_scan_exclusive_##op_name(type x)                                  \
{                                                                                                    \
    return SPIRV_BUILTIN(new_name, _i32_i32_##abbr_type, )(Subgroup,GroupOperationExclusiveScan,x);  \
}

#define DEFN_INTEL_SUB_GROUP_SCAN_EXCL_UNSIGNED_ADD(type, abbr_type)                                              \
INLINE u##type OVERLOADABLE intel_sub_group_scan_exclusive_add(u##type x)                                         \
{                                                                                                                 \
    return SPIRV_BUILTIN(GroupIAdd, _i32_i32_##abbr_type, )(Subgroup,GroupOperationExclusiveScan, as_##type(x));  \
}

#define DEFN_INTEL_SUB_GROUP_SCAN_EXCL(type, op_name, new_name, abbr_type)                           \
INLINE type OVERLOADABLE intel_sub_group_scan_exclusive_##op_name(type x)                            \
{                                                                                                    \
    return SPIRV_BUILTIN(new_name, _i32_i32_##abbr_type, )(Subgroup,GroupOperationExclusiveScan,x);  \
}

// 8bit
#if defined(cl_intel_subgroups_char)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(char,  max, GroupSMax, i8)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(char,  min, GroupSMin, i8)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(char,  add, GroupIAdd, i8)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(uchar, max, GroupUMax, i8)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(uchar, min, GroupUMin, i8)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL_UNSIGNED_ADD(char, i8)
#endif // defined(cl_intel_subgroups_char)
// 16bit
#if defined(cl_intel_subgroups_short)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(short,  max, GroupSMax, i16)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(short,  min, GroupSMin, i16)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(short,  add, GroupIAdd, i16)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(ushort, max, GroupUMax, i16)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(ushort, min, GroupUMin, i16)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL_UNSIGNED_ADD(short, i16)
#endif // defined(cl_intel_subgroups_short)
#if defined(cl_khr_subgroup_extended_types)
// 8bit
DEFN_SUB_GROUP_SCAN_EXCL(char,  max, GroupSMax, i8)
DEFN_SUB_GROUP_SCAN_EXCL(char,  min, GroupSMin, i8)
DEFN_SUB_GROUP_SCAN_EXCL(char,  add, GroupIAdd, i8)
DEFN_SUB_GROUP_SCAN_EXCL(uchar, max, GroupUMax, i8)
DEFN_SUB_GROUP_SCAN_EXCL(uchar, min, GroupUMin, i8)
DEFN_SUB_GROUP_SCAN_EXCL_UNSIGNED_ADD(char, i8)
// 16bit
DEFN_SUB_GROUP_SCAN_EXCL(short,  max, GroupSMax, i16)
DEFN_SUB_GROUP_SCAN_EXCL(short,  min, GroupSMin, i16)
DEFN_SUB_GROUP_SCAN_EXCL(short,  add, GroupIAdd, i16)
DEFN_SUB_GROUP_SCAN_EXCL(ushort, max, GroupUMax, i16)
DEFN_SUB_GROUP_SCAN_EXCL(ushort, min, GroupUMin, i16)
DEFN_SUB_GROUP_SCAN_EXCL_UNSIGNED_ADD(short, i16)
#endif defined(cl_khr_subgroup_extended_types)
// 32bit
DEFN_SUB_GROUP_SCAN_EXCL(int,   max, GroupSMax, i32)
DEFN_SUB_GROUP_SCAN_EXCL(int,   min, GroupSMin, i32)
DEFN_SUB_GROUP_SCAN_EXCL(int,   add, GroupIAdd, i32)
DEFN_SUB_GROUP_SCAN_EXCL(uint,  max, GroupUMax, i32)
DEFN_SUB_GROUP_SCAN_EXCL(uint,  min, GroupUMin, i32)
DEFN_SUB_GROUP_SCAN_EXCL(float, add, GroupFAdd, f32)
DEFN_SUB_GROUP_SCAN_EXCL(float, max, GroupFMax, f32)
DEFN_SUB_GROUP_SCAN_EXCL(float, min, GroupFMin, f32)
DEFN_SUB_GROUP_SCAN_EXCL_UNSIGNED_ADD(int, i32)
// 64bit
DEFN_SUB_GROUP_SCAN_EXCL(long,  max, GroupSMax, i64)
DEFN_SUB_GROUP_SCAN_EXCL(long,  min, GroupSMin, i64)
DEFN_SUB_GROUP_SCAN_EXCL(long,  add, GroupIAdd, i64)
DEFN_SUB_GROUP_SCAN_EXCL(ulong, max, GroupUMax, i64)
DEFN_SUB_GROUP_SCAN_EXCL(ulong, min, GroupUMin, i64)
DEFN_SUB_GROUP_SCAN_EXCL_UNSIGNED_ADD(long, i64)
// half & double
#ifdef cl_khr_fp16
DEFN_SUB_GROUP_SCAN_EXCL(half, add, GroupFAdd, f16)
DEFN_SUB_GROUP_SCAN_EXCL(half, max, GroupFMax, f16)
DEFN_SUB_GROUP_SCAN_EXCL(half, min, GroupFMin, f16)
#endif
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_SCAN_EXCL(double, add, GroupFAdd, f64)
DEFN_SUB_GROUP_SCAN_EXCL(double, max, GroupFMax, f64)
DEFN_SUB_GROUP_SCAN_EXCL(double, min, GroupFMin, f64)
#endif

#if defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)
#define DEFN_SUB_GROUP_NON_UNIFORM_OPERATION(type, spv_type, operation, spv_operation, abbr_type, group_type, spv_group_type)
#define DEFN_SUB_GROUP_NON_UNIFORM_CLUSTERED_OPERATION(type, spv_type, operation, spv_operation, abbr_type, group_type, spv_group_type)

#if defined(cl_khr_subgroup_non_uniform_arithmetic)
#define DEFN_SUB_GROUP_NON_UNIFORM_OPERATION(type, spv_type, operation, spv_operation, abbr_type, group_type, spv_group_type)   \
INLINE type OVERLOADABLE sub_group_non_uniform_##group_type##_##operation(type x)                                               \
{                                                                                                                               \
    return SPIRV_BUILTIN(GroupNonUniform##spv_operation, _i32_i32_##abbr_type, )(Subgroup,spv_group_type,as_##spv_type(x));     \
}
#endif // defined(cl_khr_subgroup_non_uniform_arithmetic)

#if defined(cl_khr_subgroup_clustered_reduce)
#define DEFN_SUB_GROUP_NON_UNIFORM_CLUSTERED_OPERATION(type, spv_type, operation, spv_operation, abbr_type, group_type, spv_group_type)          \
INLINE type OVERLOADABLE sub_group_clustered_##group_type##_##operation(type x, uint clustersize)                                                \
{                                                                                                                                                \
    return SPIRV_BUILTIN(GroupNonUniform##spv_operation, _i32_i32_##abbr_type##_i32, )(Subgroup, spv_group_type, as_##spv_type(x), clustersize); \
}
#endif // defined(cl_khr_subgroup_clustered_reduce)

#define DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, spv_type, op_name, new_name, abbr_type)                                          \
DEFN_SUB_GROUP_NON_UNIFORM_OPERATION(type, spv_type, op_name, new_name, abbr_type, reduce, GroupOperationReduce)                     \
DEFN_SUB_GROUP_NON_UNIFORM_OPERATION(type, spv_type, op_name, new_name, abbr_type, scan_inclusive, GroupOperationInclusiveScan)      \
DEFN_SUB_GROUP_NON_UNIFORM_OPERATION(type, spv_type, op_name, new_name, abbr_type, scan_exclusive, GroupOperationExclusiveScan)      \
DEFN_SUB_GROUP_NON_UNIFORM_CLUSTERED_OPERATION(type, spv_type, op_name, new_name, abbr_type, reduce, GroupOperationClusteredReduce)

// ARITHMETIC OPERATIONS

// cl_khr_subgroup_non_uniform_arithmetic:
// gentype sub_group_non_uniform_reduce_add( gentype value )
// gentype sub_group_non_uniform_reduce_mul( gentype value )
// gentype sub_group_non_uniform_scan_inclusive_add( gentype value )
// gentype sub_group_non_uniform_scan_inclusive_mul( gentype value )
// gentype sub_group_non_uniform_scan_exclusive_add( gentype value )
// gentype sub_group_non_uniform_scan_exclusive_mul( gentype value )
// cl_khr_subgroup_clustered_reduce:
// gentype sub_group_clustered_reduce_add( gentype value, uint clustersize )
// gentype sub_group_clustered_reduce_mul( gentype value, uint clustersize )
#define DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(type, spv_type, type_sign, abbr_type)       \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, spv_type, add, type_sign##Add, abbr_type)  \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, spv_type, mul, type_sign##Mul, abbr_type)

DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(char,   char,   I, i8)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(uchar,  char,   I, i8)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(short,  short,  I, i16)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(ushort, short,  I, i16)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(int,    int,    I, i32)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(uint,   int,    I, i32)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(long,   long,   I, i64)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(ulong,  long,   I, i64)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(float,  float,  F, f32)
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(double, double, F, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(half,   half,   F, f16)
#endif // defined(cl_khr_fp16)

// cl_khr_subgroup_non_uniform_arithmetic:
// gentype sub_group_non_uniform_reduce_min( gentype value )
// gentype sub_group_non_uniform_reduce_max( gentype value )
// gentype sub_group_non_uniform_scan_inclusive_min( gentype value )
// gentype sub_group_non_uniform_scan_inclusive_max( gentype value )
// gentype sub_group_non_uniform_scan_exclusive_min( gentype value )
// gentype sub_group_non_uniform_scan_exclusive_max( gentype value )
// cl_khr_subgroup_clustered_reduce:
// gentype sub_group_clustered_reduce_min( gentype value, uint clustersize )
// gentype sub_group_clustered_reduce_max( gentype value, uint clustersize )

#define DEFN_SUB_GROUP_NON_UNIFORM_MIN_MAX(type, type_sign, abbr_type)             \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, type, min, type_sign##Min, abbr_type)  \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, type, max, type_sign##Max, abbr_type)

DEFN_SUB_GROUP_NON_UNIFORM_MIN_MAX(char,   S, i8)
DEFN_SUB_GROUP_NON_UNIFORM_MIN_MAX(uchar,  U, i8)
DEFN_SUB_GROUP_NON_UNIFORM_MIN_MAX(short,  S, i16)
DEFN_SUB_GROUP_NON_UNIFORM_MIN_MAX(ushort, U, i16)
DEFN_SUB_GROUP_NON_UNIFORM_MIN_MAX(int,    S, i32)
DEFN_SUB_GROUP_NON_UNIFORM_MIN_MAX(uint,   U, i32)
DEFN_SUB_GROUP_NON_UNIFORM_MIN_MAX(long,   S, i64)
DEFN_SUB_GROUP_NON_UNIFORM_MIN_MAX(ulong,  U, i64)
DEFN_SUB_GROUP_NON_UNIFORM_MIN_MAX(float,  F, f32)
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_NON_UNIFORM_MIN_MAX(double, F, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_NON_UNIFORM_MIN_MAX(half,   F, f16)
#endif // defined(cl_khr_fp16)

// BITWISE OPERATIONS

// gentype sub_group_non_uniform_reduce_and(gentype value )
// gentype sub_group_non_uniform_reduce_or(gentype value )
// gentype sub_group_non_uniform_reduce_xor(gentype value )
// gentype sub_group_non_uniform_scan_inclusive_and(gentype value )
// gentype sub_group_non_uniform_scan_inclusive_or(gentype value )
// gentype sub_group_non_uniform_scan_inclusive_xor(gentype value )
// gentype sub_group_non_uniform_scan_exclusive_and(gentype value )
// gentype sub_group_non_uniform_scan_exclusive_or(gentype value )
// gentype sub_group_non_uniform_scan_exclusive_xor(gentype value )
#define DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(type, spv_type, abbr_type)     \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, spv_type, and, BitwiseAnd, abbr_type)    \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, spv_type, or,  BitwiseOr,  abbr_type)    \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, spv_type, xor, BitwiseXor, abbr_type)

DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(char,   char, i8)
DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(uchar,  char, i8)
DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(short,  short, i16)
DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(ushort, short, i16)
DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(int,    int, i32)
DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(uint,   int, i32)
DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(long,   long, i64)
DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(ulong,  long, i64)

// LOGICAL OPERATIONS

// int sub_group_non_uniform_reduce_logical_and(int predicate)
// int sub_group_non_uniform_reduce_logical_or(int predicate)
// int sub_group_non_uniform_reduce_logical_xor(int predicate)
// int sub_group_non_uniform_reduce_scan_inclusive_logical_and(int predicate)
// int sub_group_non_uniform_reduce_scan_inclusive_logical_or(int predicate)
// int sub_group_non_uniform_reduce_scan_inclusive_logical_xor(int predicate)
// int sub_group_non_uniform_reduce_scan_exclusive_logical_and(int predicate)
// int sub_group_non_uniform_reduce_scan_exclusive_logical_or(int predicate)
// int sub_group_non_uniform_reduce_scan_exclusive_logical_xor(int predicate)
#define DEFN_SUB_GROUP_NON_UNIFORM_LOGICAL_OPERATIONS(type, abbr_type)                  \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, type, logical_and, LogicalAnd, abbr_type)   \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, type, logical_or,  LogicalOr,  abbr_type)   \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, type, logical_xor, LogicalXor, abbr_type)

// OpenCL C representation of logical operations uses int type,
// whereas SPIR-V representation uses Boolean type (i1) for the same parameter.
DEFN_SUB_GROUP_NON_UNIFORM_LOGICAL_OPERATIONS(int, i1)

#endif // defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)
