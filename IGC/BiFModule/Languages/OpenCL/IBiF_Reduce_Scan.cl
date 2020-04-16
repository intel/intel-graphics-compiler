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

#define DEFN_WORK_GROUP_REDUCE_S_ADD(type, abbr_type)                                        \
INLINE type OVERLOADABLE work_group_reduce_add(type x)                                       \
{                                                                                            \
    return __builtin_spirv_OpGroupIAdd_i32_i32_##abbr_type(Workgroup,GroupOperationReduce, as_u##type(x));\
}

#define DEFN_WORK_GROUP_REDUCE(type, op_name, new_name, abbr_type)                           \
INLINE type OVERLOADABLE work_group_reduce_##op_name(type x)                                 \
{                                                                                            \
    return __builtin_spirv_##new_name##_i32_i32_##abbr_type(Workgroup,GroupOperationReduce,x);\
}

DEFN_WORK_GROUP_REDUCE(int, max, OpGroupSMax, i32)
DEFN_WORK_GROUP_REDUCE(int, min, OpGroupSMin, i32)
DEFN_WORK_GROUP_REDUCE(uint, add, OpGroupIAdd, i32)
DEFN_WORK_GROUP_REDUCE(uint, max, OpGroupUMax, i32)
DEFN_WORK_GROUP_REDUCE(uint, min, OpGroupUMin, i32)
DEFN_WORK_GROUP_REDUCE(float, add, OpGroupFAdd, f32)
DEFN_WORK_GROUP_REDUCE(float, max, OpGroupFMax, f32)
DEFN_WORK_GROUP_REDUCE(float, min, OpGroupFMin, f32)
DEFN_WORK_GROUP_REDUCE_S_ADD(int, i32)
DEFN_WORK_GROUP_REDUCE_S_ADD(long, i64)
DEFN_WORK_GROUP_REDUCE(long, max, OpGroupSMax, i64)
DEFN_WORK_GROUP_REDUCE(long, min, OpGroupSMin, i64)
DEFN_WORK_GROUP_REDUCE(ulong, add, OpGroupIAdd, i64)
DEFN_WORK_GROUP_REDUCE(ulong, max, OpGroupUMax, i64)
DEFN_WORK_GROUP_REDUCE(ulong, min, OpGroupUMin, i64)
#ifdef cl_khr_fp16
DEFN_WORK_GROUP_REDUCE(half, add, OpGroupFAdd, f16)
DEFN_WORK_GROUP_REDUCE(half, max, OpGroupFMax, f16)
DEFN_WORK_GROUP_REDUCE(half, min, OpGroupFMin, f16)
#endif
#if defined(cl_khr_fp64)
DEFN_WORK_GROUP_REDUCE(double, add, OpGroupFAdd, f64)
DEFN_WORK_GROUP_REDUCE(double, max, OpGroupFMax, f64)
DEFN_WORK_GROUP_REDUCE(double, min, OpGroupFMin, f64)
#endif

#define DEFN_WORK_GROUP_SCAN_INCL_S_ADD(type, abbr_type)                                        \
INLINE type OVERLOADABLE work_group_scan_inclusive_add(type x)                                  \
{                                                                                               \
    return __builtin_spirv_OpGroupIAdd_i32_i32_##abbr_type(Workgroup,GroupOperationInclusiveScan, as_u##type(x));\
}

#define DEFN_WORK_GROUP_SCAN_INCL(type, op_name, new_name, abbr_type)                           \
INLINE type OVERLOADABLE work_group_scan_inclusive_##op_name(type x)                            \
{                                                                                               \
    return __builtin_spirv_##new_name##_i32_i32_##abbr_type(Workgroup,GroupOperationInclusiveScan,x);\
}

DEFN_WORK_GROUP_SCAN_INCL(int, max, OpGroupSMax, i32)
DEFN_WORK_GROUP_SCAN_INCL(int, min, OpGroupSMin, i32)
DEFN_WORK_GROUP_SCAN_INCL(uint, add, OpGroupIAdd, i32)
DEFN_WORK_GROUP_SCAN_INCL(uint, max, OpGroupUMax, i32)
DEFN_WORK_GROUP_SCAN_INCL(uint, min, OpGroupUMin, i32)
DEFN_WORK_GROUP_SCAN_INCL(float, add, OpGroupFAdd, f32)
DEFN_WORK_GROUP_SCAN_INCL(float, max, OpGroupFMax, f32)
DEFN_WORK_GROUP_SCAN_INCL(float, min, OpGroupFMin, f32)
DEFN_WORK_GROUP_SCAN_INCL_S_ADD(int, i32)
DEFN_WORK_GROUP_SCAN_INCL_S_ADD(long, i64)
DEFN_WORK_GROUP_SCAN_INCL(long, max, OpGroupSMax, i64)
DEFN_WORK_GROUP_SCAN_INCL(long, min, OpGroupSMin, i64)
DEFN_WORK_GROUP_SCAN_INCL(ulong, add, OpGroupIAdd, i64)
DEFN_WORK_GROUP_SCAN_INCL(ulong, max, OpGroupUMax, i64)
DEFN_WORK_GROUP_SCAN_INCL(ulong, min, OpGroupUMin, i64)
#ifdef cl_khr_fp16
DEFN_WORK_GROUP_SCAN_INCL(half, add, OpGroupFAdd, f16)
DEFN_WORK_GROUP_SCAN_INCL(half, max, OpGroupFMax, f16)
DEFN_WORK_GROUP_SCAN_INCL(half, min, OpGroupFMin, f16)
#endif
#if defined(cl_khr_fp64)
DEFN_WORK_GROUP_SCAN_INCL(double, add, OpGroupFAdd, f64)
DEFN_WORK_GROUP_SCAN_INCL(double, max, OpGroupFMax, f64)
DEFN_WORK_GROUP_SCAN_INCL(double, min, OpGroupFMin, f64)
#endif

#define DEFN_WORK_GROUP_SCAN_EXCL_S_ADD(type, abbr_type)                                        \
INLINE type OVERLOADABLE work_group_scan_exclusive_add(type x)                                  \
{                                                                                               \
    return __builtin_spirv_OpGroupIAdd_i32_i32_##abbr_type(Workgroup,GroupOperationExclusiveScan, as_u##type(x));\
}

#define DEFN_WORK_GROUP_SCAN_EXCL(type, op_name, new_name, abbr_type)                           \
INLINE type OVERLOADABLE work_group_scan_exclusive_##op_name(type x)                            \
{                                                                                               \
    return __builtin_spirv_##new_name##_i32_i32_##abbr_type(Workgroup,GroupOperationExclusiveScan,x);\
}

DEFN_WORK_GROUP_SCAN_EXCL(int, max, OpGroupSMax, i32)
DEFN_WORK_GROUP_SCAN_EXCL(int, min, OpGroupSMin, i32)
DEFN_WORK_GROUP_SCAN_EXCL(uint, add, OpGroupIAdd, i32)
DEFN_WORK_GROUP_SCAN_EXCL(uint, max, OpGroupUMax, i32)
DEFN_WORK_GROUP_SCAN_EXCL(uint, min, OpGroupUMin, i32)
DEFN_WORK_GROUP_SCAN_EXCL(float, add, OpGroupFAdd, f32)
DEFN_WORK_GROUP_SCAN_EXCL(float, max, OpGroupFMax, f32)
DEFN_WORK_GROUP_SCAN_EXCL(float, min, OpGroupFMin, f32)
DEFN_WORK_GROUP_SCAN_EXCL_S_ADD(int, i32)
DEFN_WORK_GROUP_SCAN_EXCL_S_ADD(long, i64)
DEFN_WORK_GROUP_SCAN_EXCL(long, max, OpGroupSMax, i64)
DEFN_WORK_GROUP_SCAN_EXCL(long, min, OpGroupSMin, i64)
DEFN_WORK_GROUP_SCAN_EXCL(ulong, add, OpGroupIAdd, i64)
DEFN_WORK_GROUP_SCAN_EXCL(ulong, max, OpGroupUMax, i64)
DEFN_WORK_GROUP_SCAN_EXCL(ulong, min, OpGroupUMin, i64)
#ifdef cl_khr_fp16
DEFN_WORK_GROUP_SCAN_EXCL(half, add, OpGroupFAdd, f16)
DEFN_WORK_GROUP_SCAN_EXCL(half, max, OpGroupFMax, f16)
DEFN_WORK_GROUP_SCAN_EXCL(half, min, OpGroupFMin, f16)
#endif
#if defined(cl_khr_fp64)
DEFN_WORK_GROUP_SCAN_EXCL(double, add, OpGroupFAdd, f64)
DEFN_WORK_GROUP_SCAN_EXCL(double, max, OpGroupFMax, f64)
DEFN_WORK_GROUP_SCAN_EXCL(double, min, OpGroupFMin, f64)
#endif

#define DEFN_SUB_GROUP_REDUCE_S_ADD(type, abbr_type)                                        \
INLINE type OVERLOADABLE sub_group_reduce_add(type x)                                       \
{                                                                                            \
    return __builtin_spirv_OpGroupIAdd_i32_i32_##abbr_type(Subgroup,GroupOperationReduce, as_u##type(x));\
}

#define DEFN_SUB_GROUP_REDUCE(type, op_name, new_name, abbr_type)                            \
INLINE type OVERLOADABLE sub_group_reduce_##op_name(type x)                                 \
{                                                                                            \
    return __builtin_spirv_##new_name##_i32_i32_##abbr_type(Subgroup,GroupOperationReduce,x);\
}

#define DEFN_INTEL_SUB_GROUP_REDUCE_S_ADD(type, abbr_type)                                        \
INLINE type OVERLOADABLE intel_sub_group_reduce_add(type x)                                       \
{                                                                                            \
    return __builtin_spirv_OpGroupIAdd_i32_i32_##abbr_type(Subgroup,GroupOperationReduce, as_u##type(x));\
}

#define DEFN_INTEL_SUB_GROUP_REDUCE(type, op_name, new_name, abbr_type)                            \
INLINE type OVERLOADABLE intel_sub_group_reduce_##op_name(type x)                                 \
{                                                                                            \
    return __builtin_spirv_##new_name##_i32_i32_##abbr_type(Subgroup,GroupOperationReduce,x);\
}

// 8bit
#if defined(cl_intel_subgroups_char)
DEFN_INTEL_SUB_GROUP_REDUCE(char,  max, OpGroupSMax, i8)
DEFN_INTEL_SUB_GROUP_REDUCE(char,  min, OpGroupSMin, i8)
DEFN_INTEL_SUB_GROUP_REDUCE(uchar, add, OpGroupIAdd, i8)
DEFN_INTEL_SUB_GROUP_REDUCE(uchar, max, OpGroupUMax, i8)
DEFN_INTEL_SUB_GROUP_REDUCE(uchar, min, OpGroupUMin, i8)
DEFN_INTEL_SUB_GROUP_REDUCE_S_ADD(char, i8)
#endif // defined(cl_intel_subgroups_char)
// 16bit
#if defined(cl_intel_subgroups_short)
DEFN_INTEL_SUB_GROUP_REDUCE(short,  max, OpGroupSMax, i16)
DEFN_INTEL_SUB_GROUP_REDUCE(short,  min, OpGroupSMin, i16)
DEFN_INTEL_SUB_GROUP_REDUCE(ushort, add, OpGroupIAdd, i16)
DEFN_INTEL_SUB_GROUP_REDUCE(ushort, max, OpGroupUMax, i16)
DEFN_INTEL_SUB_GROUP_REDUCE(ushort, min, OpGroupUMin, i16)
DEFN_INTEL_SUB_GROUP_REDUCE_S_ADD(short, i16)
#endif // defined(cl_intel_subgroups_short)
#if defined(cl_khr_subgroup_extended_types)
// 8bit
DEFN_SUB_GROUP_REDUCE(char,  max, OpGroupSMax, i8)
DEFN_SUB_GROUP_REDUCE(char,  min, OpGroupSMin, i8)
DEFN_SUB_GROUP_REDUCE(uchar, add, OpGroupIAdd, i8)
DEFN_SUB_GROUP_REDUCE(uchar, max, OpGroupUMax, i8)
DEFN_SUB_GROUP_REDUCE(uchar, min, OpGroupUMin, i8)
DEFN_SUB_GROUP_REDUCE_S_ADD(char, i8)
// 16bit
DEFN_SUB_GROUP_REDUCE(short,  max, OpGroupSMax, i16)
DEFN_SUB_GROUP_REDUCE(short,  min, OpGroupSMin, i16)
DEFN_SUB_GROUP_REDUCE(ushort, add, OpGroupIAdd, i16)
DEFN_SUB_GROUP_REDUCE(ushort, max, OpGroupUMax, i16)
DEFN_SUB_GROUP_REDUCE(ushort, min, OpGroupUMin, i16)
DEFN_SUB_GROUP_REDUCE_S_ADD(short, i16)
#endif // defined(cl_khr_subgroup_extended_types)
// 32bit
DEFN_SUB_GROUP_REDUCE(int,   max, OpGroupSMax, i32)
DEFN_SUB_GROUP_REDUCE(int,   min, OpGroupSMin, i32)
DEFN_SUB_GROUP_REDUCE(uint,  add, OpGroupIAdd, i32)
DEFN_SUB_GROUP_REDUCE(uint,  max, OpGroupUMax, i32)
DEFN_SUB_GROUP_REDUCE(uint,  min, OpGroupUMin, i32)
DEFN_SUB_GROUP_REDUCE(float, add, OpGroupFAdd, f32)
DEFN_SUB_GROUP_REDUCE(float, max, OpGroupFMax, f32)
DEFN_SUB_GROUP_REDUCE(float, min, OpGroupFMin, f32)
DEFN_SUB_GROUP_REDUCE_S_ADD(int, i32)
// 64bit
DEFN_SUB_GROUP_REDUCE(long,  max, OpGroupSMax, i64)
DEFN_SUB_GROUP_REDUCE(long,  min, OpGroupSMin, i64)
DEFN_SUB_GROUP_REDUCE(ulong, add, OpGroupIAdd, i64)
DEFN_SUB_GROUP_REDUCE(ulong, max, OpGroupUMax, i64)
DEFN_SUB_GROUP_REDUCE(ulong, min, OpGroupUMin, i64)
DEFN_SUB_GROUP_REDUCE_S_ADD(long, i64)
// half & double
#ifdef cl_khr_fp16
DEFN_SUB_GROUP_REDUCE(half, add, OpGroupFAdd, f16)
DEFN_SUB_GROUP_REDUCE(half, max, OpGroupFMax, f16)
DEFN_SUB_GROUP_REDUCE(half, min, OpGroupFMin, f16)
#endif
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_REDUCE(double, add, OpGroupFAdd, f64)
DEFN_SUB_GROUP_REDUCE(double, max, OpGroupFMax, f64)
DEFN_SUB_GROUP_REDUCE(double, min, OpGroupFMin, f64)
#endif

#define DEFN_SUB_GROUP_SCAN_INCL_S_ADD(type, abbr_type)                                        \
INLINE type OVERLOADABLE sub_group_scan_inclusive_add(type x)                                  \
{                                                                                               \
    return __builtin_spirv_OpGroupIAdd_i32_i32_##abbr_type(Subgroup,GroupOperationInclusiveScan, as_u##type(x));\
}

#define DEFN_SUB_GROUP_SCAN_INCL(type, op_name, new_name, abbr_type)                           \
INLINE type OVERLOADABLE sub_group_scan_inclusive_##op_name(type x)                            \
{                                                                                               \
    return __builtin_spirv_##new_name##_i32_i32_##abbr_type(Subgroup,GroupOperationInclusiveScan,x);\
}

#define DEFN_INTEL_SUB_GROUP_SCAN_INCL_S_ADD(type, abbr_type)                                        \
INLINE type OVERLOADABLE intel_sub_group_scan_inclusive_add(type x)                                  \
{                                                                                               \
    return __builtin_spirv_OpGroupIAdd_i32_i32_##abbr_type(Subgroup,GroupOperationInclusiveScan, as_u##type(x));\
}

#define DEFN_INTEL_SUB_GROUP_SCAN_INCL(type, op_name, new_name, abbr_type)                           \
INLINE type OVERLOADABLE intel_sub_group_scan_inclusive_##op_name(type x)                            \
{                                                                                               \
    return __builtin_spirv_##new_name##_i32_i32_##abbr_type(Subgroup,GroupOperationInclusiveScan,x);\
}

// 8bit
#if defined(cl_intel_subgroups_char)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(char,  max, OpGroupSMax, i8)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(char,  min, OpGroupSMin, i8)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(uchar, add, OpGroupIAdd, i8)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(uchar, max, OpGroupUMax, i8)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(uchar, min, OpGroupUMin, i8)
DEFN_INTEL_SUB_GROUP_SCAN_INCL_S_ADD(char, i8)
#endif // defined(cl_intel_subgroups_char)
// 16bit
#if defined(cl_intel_subgroups_short)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(short,  max, OpGroupSMax, i16)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(short,  min, OpGroupSMin, i16)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(ushort, add, OpGroupIAdd, i16)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(ushort, max, OpGroupUMax, i16)
DEFN_INTEL_SUB_GROUP_SCAN_INCL(ushort, min, OpGroupUMin, i16)
DEFN_INTEL_SUB_GROUP_SCAN_INCL_S_ADD(short, i16)
#endif // defined(cl_intel_subgroups_short)
#if defined(cl_khr_subgroup_extended_types)
// 8bit
DEFN_SUB_GROUP_SCAN_INCL(char,  max, OpGroupSMax, i8)
DEFN_SUB_GROUP_SCAN_INCL(char,  min, OpGroupSMin, i8)
DEFN_SUB_GROUP_SCAN_INCL(uchar, add, OpGroupIAdd, i8)
DEFN_SUB_GROUP_SCAN_INCL(uchar, max, OpGroupUMax, i8)
DEFN_SUB_GROUP_SCAN_INCL(uchar, min, OpGroupUMin, i8)
DEFN_SUB_GROUP_SCAN_INCL_S_ADD(char, i8)
// 16bit
DEFN_SUB_GROUP_SCAN_INCL(short,  max, OpGroupSMax, i16)
DEFN_SUB_GROUP_SCAN_INCL(short,  min, OpGroupSMin, i16)
DEFN_SUB_GROUP_SCAN_INCL(ushort, add, OpGroupIAdd, i16)
DEFN_SUB_GROUP_SCAN_INCL(ushort, max, OpGroupUMax, i16)
DEFN_SUB_GROUP_SCAN_INCL(ushort, min, OpGroupUMin, i16)
DEFN_SUB_GROUP_SCAN_INCL_S_ADD(short, i16)
#endif defined(cl_khr_subgroup_extended_types)
// 32bit
DEFN_SUB_GROUP_SCAN_INCL(int,   max, OpGroupSMax, i32)
DEFN_SUB_GROUP_SCAN_INCL(int,   min, OpGroupSMin, i32)
DEFN_SUB_GROUP_SCAN_INCL(uint,  add, OpGroupIAdd, i32)
DEFN_SUB_GROUP_SCAN_INCL(uint,  max, OpGroupUMax, i32)
DEFN_SUB_GROUP_SCAN_INCL(uint,  min, OpGroupUMin, i32)
DEFN_SUB_GROUP_SCAN_INCL(float, add, OpGroupFAdd, f32)
DEFN_SUB_GROUP_SCAN_INCL(float, max, OpGroupFMax, f32)
DEFN_SUB_GROUP_SCAN_INCL(float, min, OpGroupFMin, f32)
DEFN_SUB_GROUP_SCAN_INCL_S_ADD(int, i32)
// 64bit
DEFN_SUB_GROUP_SCAN_INCL(long,  max, OpGroupSMax, i64)
DEFN_SUB_GROUP_SCAN_INCL(long,  min, OpGroupSMin, i64)
DEFN_SUB_GROUP_SCAN_INCL(ulong, add, OpGroupIAdd, i64)
DEFN_SUB_GROUP_SCAN_INCL(ulong, max, OpGroupUMax, i64)
DEFN_SUB_GROUP_SCAN_INCL(ulong, min, OpGroupUMin, i64)
DEFN_SUB_GROUP_SCAN_INCL_S_ADD(long, i64)
// half & double
#ifdef cl_khr_fp16
DEFN_SUB_GROUP_SCAN_INCL(half, add, OpGroupFAdd, f16)
DEFN_SUB_GROUP_SCAN_INCL(half, max, OpGroupFMax, f16)
DEFN_SUB_GROUP_SCAN_INCL(half, min, OpGroupFMin, f16)
#endif
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_SCAN_INCL(double, add, OpGroupFAdd, f64)
DEFN_SUB_GROUP_SCAN_INCL(double, max, OpGroupFMax, f64)
DEFN_SUB_GROUP_SCAN_INCL(double, min, OpGroupFMin, f64)
#endif


#define DEFN_SUB_GROUP_SCAN_EXCL_S_ADD(type, abbr_type)                                        \
INLINE type OVERLOADABLE sub_group_scan_exclusive_add(type x)                                  \
{                                                                                               \
    return __builtin_spirv_OpGroupIAdd_i32_i32_##abbr_type(Subgroup,GroupOperationExclusiveScan, as_u##type(x));\
}

#define DEFN_SUB_GROUP_SCAN_EXCL(type, op_name, new_name, abbr_type)                           \
INLINE type OVERLOADABLE sub_group_scan_exclusive_##op_name(type x)                            \
{                                                                                               \
    return __builtin_spirv_##new_name##_i32_i32_##abbr_type(Subgroup,GroupOperationExclusiveScan,x);\
}

#define DEFN_INTEL_SUB_GROUP_SCAN_EXCL_S_ADD(type, abbr_type)                                        \
INLINE type OVERLOADABLE intel_sub_group_scan_exclusive_add(type x)                                  \
{                                                                                               \
    return __builtin_spirv_OpGroupIAdd_i32_i32_##abbr_type(Subgroup,GroupOperationExclusiveScan, as_u##type(x));\
}

#define DEFN_INTEL_SUB_GROUP_SCAN_EXCL(type, op_name, new_name, abbr_type)                           \
INLINE type OVERLOADABLE intel_sub_group_scan_exclusive_##op_name(type x)                            \
{                                                                                               \
    return __builtin_spirv_##new_name##_i32_i32_##abbr_type(Subgroup,GroupOperationExclusiveScan,x);\
}

// 8bit
#if defined(cl_intel_subgroups_char)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(char,  max, OpGroupSMax, i8)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(char,  min, OpGroupSMin, i8)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(uchar, add, OpGroupIAdd, i8)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(uchar, max, OpGroupUMax, i8)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(uchar, min, OpGroupUMin, i8)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL_S_ADD(char, i8)
#endif // defined(cl_intel_subgroups_char)
// 16bit
#if defined(cl_intel_subgroups_short)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(short,  max, OpGroupSMax, i16)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(short,  min, OpGroupSMin, i16)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(ushort, add, OpGroupIAdd, i16)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(ushort, max, OpGroupUMax, i16)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL(ushort, min, OpGroupUMin, i16)
DEFN_INTEL_SUB_GROUP_SCAN_EXCL_S_ADD(short, i16)
#endif // defined(cl_intel_subgroups_short)
#if defined(cl_khr_subgroup_extended_types)
// 8bit
DEFN_SUB_GROUP_SCAN_EXCL(char, max, OpGroupSMax, i8)
DEFN_SUB_GROUP_SCAN_EXCL(char, min, OpGroupSMin, i8)
DEFN_SUB_GROUP_SCAN_EXCL(uchar, add, OpGroupIAdd, i8)
DEFN_SUB_GROUP_SCAN_EXCL(uchar, max, OpGroupUMax, i8)
DEFN_SUB_GROUP_SCAN_EXCL(uchar, min, OpGroupUMin, i8)
DEFN_SUB_GROUP_SCAN_EXCL_S_ADD(char, i8)
// 16bit
DEFN_SUB_GROUP_SCAN_EXCL(short, max, OpGroupSMax, i16)
DEFN_SUB_GROUP_SCAN_EXCL(short, min, OpGroupSMin, i16)
DEFN_SUB_GROUP_SCAN_EXCL(ushort, add, OpGroupIAdd, i16)
DEFN_SUB_GROUP_SCAN_EXCL(ushort, max, OpGroupUMax, i16)
DEFN_SUB_GROUP_SCAN_EXCL(ushort, min, OpGroupUMin, i16)
DEFN_SUB_GROUP_SCAN_EXCL_S_ADD(short, i16)
#endif defined(cl_khr_subgroup_extended_types)
// 32bit
DEFN_SUB_GROUP_SCAN_EXCL(int,   max, OpGroupSMax, i32)
DEFN_SUB_GROUP_SCAN_EXCL(int,   min, OpGroupSMin, i32)
DEFN_SUB_GROUP_SCAN_EXCL(uint,  add, OpGroupIAdd, i32)
DEFN_SUB_GROUP_SCAN_EXCL(uint,  max, OpGroupUMax, i32)
DEFN_SUB_GROUP_SCAN_EXCL(uint,  min, OpGroupUMin, i32)
DEFN_SUB_GROUP_SCAN_EXCL(float, add, OpGroupFAdd, f32)
DEFN_SUB_GROUP_SCAN_EXCL(float, max, OpGroupFMax, f32)
DEFN_SUB_GROUP_SCAN_EXCL(float, min, OpGroupFMin, f32)
DEFN_SUB_GROUP_SCAN_EXCL_S_ADD(int, i32)
// 64bit
DEFN_SUB_GROUP_SCAN_EXCL(long,  max, OpGroupSMax, i64)
DEFN_SUB_GROUP_SCAN_EXCL(long,  min, OpGroupSMin, i64)
DEFN_SUB_GROUP_SCAN_EXCL(ulong, add, OpGroupIAdd, i64)
DEFN_SUB_GROUP_SCAN_EXCL(ulong, max, OpGroupUMax, i64)
DEFN_SUB_GROUP_SCAN_EXCL(ulong, min, OpGroupUMin, i64)
DEFN_SUB_GROUP_SCAN_EXCL_S_ADD(long, i64)
// half & double
#ifdef cl_khr_fp16
DEFN_SUB_GROUP_SCAN_EXCL(half, add, OpGroupFAdd, f16)
DEFN_SUB_GROUP_SCAN_EXCL(half, max, OpGroupFMax, f16)
DEFN_SUB_GROUP_SCAN_EXCL(half, min, OpGroupFMin, f16)
#endif
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_SCAN_EXCL(double, add, OpGroupFAdd, f64)
DEFN_SUB_GROUP_SCAN_EXCL(double, max, OpGroupFMax, f64)
DEFN_SUB_GROUP_SCAN_EXCL(double, min, OpGroupFMin, f64)
#endif

#if defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)
#define DEFN_SUB_GROUP_NON_UNIFORM_OPERATION(type, operation, spv_operation, abbr_type, group_type, spv_group_type)
#define DEFN_SUB_GROUP_NON_UNIFORM_CLUSTERED_OPERATION(type, operation, spv_operation, abbr_type, group_type, spv_group_type)

#if defined(cl_khr_subgroup_non_uniform_arithmetic)
#define DEFN_SUB_GROUP_NON_UNIFORM_OPERATION(type, operation, spv_operation, abbr_type, group_type, spv_group_type)    \
INLINE type OVERLOADABLE sub_group_non_uniform_##group_type##_##operation(type x)                                      \
{                                                                                                                      \
    return __builtin_spirv_OpGroupNonUniform##spv_operation##_i32_i32_##abbr_type(Subgroup,spv_group_type,x);          \
}
#endif // defined(cl_khr_subgroup_non_uniform_arithmetic)

#if defined(cl_khr_subgroup_clustered_reduce)
#define DEFN_SUB_GROUP_NON_UNIFORM_CLUSTERED_OPERATION(type, operation, spv_operation, abbr_type, group_type, spv_group_type)       \
INLINE type OVERLOADABLE sub_group_clustered_##group_type##_##operation(type x, uint clustersize)                                   \
{                                                                                                                                   \
    return __builtin_spirv_OpGroupNonUniform##spv_operation##_i32_i32_##abbr_type##_i32(Subgroup, spv_group_type, x, clustersize);  \
}
#endif // defined(cl_khr_subgroup_clustered_reduce)

#define DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, op_name, new_name, abbr_type)                                          \
DEFN_SUB_GROUP_NON_UNIFORM_OPERATION(type, op_name, new_name, abbr_type, reduce, GroupOperationReduce)                     \
DEFN_SUB_GROUP_NON_UNIFORM_OPERATION(type, op_name, new_name, abbr_type, scan_inclusive, GroupOperationInclusiveScan)      \
DEFN_SUB_GROUP_NON_UNIFORM_OPERATION(type, op_name, new_name, abbr_type, scan_exclusive, GroupOperationExclusiveScan)      \
DEFN_SUB_GROUP_NON_UNIFORM_CLUSTERED_OPERATION(type, op_name, new_name, abbr_type, reduce, GroupOperationClusteredReduce)

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
#define DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(type, type_sign, abbr_type)       \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, add, type_sign##Add, abbr_type)  \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, mul, type_sign##Mul, abbr_type)

DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(char,   I, i8)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(uchar,  I, i8)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(short,  I, i16)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(ushort, I, i16)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(int,    I, i32)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(uint,   I, i32)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(long,   I, i64)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(ulong,  I, i64)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(float,  F, f32)
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(double, F, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_NON_UNIFORM_ADD_MUL(half,   F, f16)
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

#define DEFN_SUB_GROUP_NON_UNIFORM_MIN_MAX(type, type_sign, abbr_type)       \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, min, type_sign##Min, abbr_type)  \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, max, type_sign##Max, abbr_type)

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
#define DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(type, abbr_type)     \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, and, BitwiseAnd, abbr_type)    \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, or,  BitwiseOr,  abbr_type)    \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, xor, BitwiseXor, abbr_type)

DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(char,   i8)
DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(uchar,  i8)
DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(short,  i16)
DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(ushort, i16)
DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(int,    i32)
DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(uint,   i32)
DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(long,   i64)
DEFN_SUB_GROUP_NON_UNIFORM_BITWISE_OPERATIONS(ulong,  i64)

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
#define DEFN_SUB_GROUP_NON_UNIFORM_LOGICAL_OPERATIONS(type, abbr_type)            \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, logical_and, LogicalAnd, abbr_type)   \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, logical_or,  LogicalOr,  abbr_type)   \
DEFN_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(type, logical_xor, LogicalXor, abbr_type)

// OpenCL C representation of logical operations uses int type,
// whereas SPIR-V representation uses Boolean type (i1) for the same parameter.
DEFN_SUB_GROUP_NON_UNIFORM_LOGICAL_OPERATIONS(int, i1)

#endif // defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)
