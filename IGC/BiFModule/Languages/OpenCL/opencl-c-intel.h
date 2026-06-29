/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// opencl-c-intel.h
//
// Intel-specific OpenCL built-in declarations that are NOT provided by the
// stock Clang OpenCL header (opencl-c.h) shipped with LLVM 14. The standard
// OpenCL built-ins are supplied by the Clang default header through the
// opencl-clang flow; this file only supplements it with the Intel extensions
// that Clang does not declare.
//
// Sliced verbatim from opencl_cth_released.h. Do not hand-edit declarations
// here; update opencl_cth_released.h and re-slice instead.
//
// Version gating:
//   - Blocks below are Intel-exclusive across stock Clang 14/16/17.
//   - cl_khr_subgroup_rotate is additionally guarded with
//     (__clang_major__ < 16) because stock Clang 16 and newer already
//     declare sub_group_rotate / sub_group_clustered_rotate.
//

#ifndef _OPENCL_C_INTEL_H_
#define _OPENCL_C_INTEL_H_

// Enable the optional FP extensions used by some declarations below (half /
// double by-value parameters and return types). Mirrors opencl_cth_released.h.
#if defined(cl_khr_fp16)
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#endif

#if defined(cl_khr_fp64)
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#endif

#ifdef cl_khr_kernel_clock
#pragma OPENCL EXTENSION cl_khr_kernel_clock : enable
#endif // cl_khr_kernel_clock

// Internal helper macros used by the Intel sub-group reduce/scan declarations
// below. Copied from opencl_cth_released.h (these are CTH-internal and are not
// provided by the stock Clang OpenCL header).
#define DECL_GROUP_ADD_MIN_MAX(prefix, type)                 \
    type __attribute__((overloadable)) prefix##_add(type x); \
    type __attribute__((overloadable)) prefix##_min(type x); \
    type __attribute__((overloadable)) prefix##_max(type x);

#define DECL_GROUP_REDUCE_SCAN(prefix, type)              \
    DECL_GROUP_ADD_MIN_MAX(prefix##_reduce, type)         \
    DECL_GROUP_ADD_MIN_MAX(prefix##_scan_exclusive, type) \
    DECL_GROUP_ADD_MIN_MAX(prefix##_scan_inclusive, type)

// ===== cl_intel_bfloat16_conversions =====
#if defined(cl_intel_bfloat16_conversions)
ushort __attribute__((overloadable))  intel_convert_bfloat16_as_ushort(float source);
ushort2 __attribute__((overloadable)) intel_convert_bfloat162_as_ushort2(float2 source);
ushort3 __attribute__((overloadable)) intel_convert_bfloat163_as_ushort3(float3 source);
ushort4 __attribute__((overloadable)) intel_convert_bfloat164_as_ushort4(float4 source);
ushort8 __attribute__((overloadable)) intel_convert_bfloat168_as_ushort8(float8 source);
ushort16 __attribute__((overloadable))
intel_convert_bfloat1616_as_ushort16(float16 source);

float __attribute__((overloadable))  intel_convert_as_bfloat16_float(ushort source);
float2 __attribute__((overloadable)) intel_convert_as_bfloat162_float2(ushort2 source);
float3 __attribute__((overloadable)) intel_convert_as_bfloat163_float3(ushort3 source);
float4 __attribute__((overloadable)) intel_convert_as_bfloat164_float4(ushort4 source);
float8 __attribute__((overloadable)) intel_convert_as_bfloat168_float8(ushort8 source);
float16 __attribute__((overloadable))
intel_convert_as_bfloat1616_float16(ushort16 source);
#endif // defined(cl_intel_bfloat16_conversions)

// ===== cl_intel_subgroups_half =====
#ifdef cl_intel_subgroups_half
#ifdef __opencl_c_images
ushort __attribute__((overloadable))
intel_sub_group_block_read_half(read_only image2d_t image, int2 coord);
ushort2 __attribute__((overloadable))
intel_sub_group_block_read2_half(read_only image2d_t image, int2 coord);
ushort4 __attribute__((overloadable))
intel_sub_group_block_read4_half(read_only image2d_t image, int2 coord);
ushort8 __attribute__((overloadable))
intel_sub_group_block_read8_half(read_only image2d_t image, int2 coord);
ushort16 __attribute__((overloadable))
intel_sub_group_block_read16_half(read_only image2d_t image, int2 coord);

#ifdef __opencl_c_read_write_images
ushort __attribute__((overloadable))
intel_sub_group_block_read_half(read_write image2d_t image, int2 coord);
ushort2 __attribute__((overloadable))
intel_sub_group_block_read2_half(read_write image2d_t image, int2 coord);
ushort4 __attribute__((overloadable))
intel_sub_group_block_read4_half(read_write image2d_t image, int2 coord);
ushort8 __attribute__((overloadable))
intel_sub_group_block_read8_half(read_write image2d_t image, int2 coord);
ushort16 __attribute__((overloadable))
intel_sub_group_block_read16_half(read_write image2d_t image, int2 coord);

void __attribute__((overloadable))
intel_sub_group_block_write(read_write image2d_t image, int2 coord, ushort data);
void __attribute__((overloadable))
intel_sub_group_block_write2(read_write image2d_t image, int2 coord, ushort2 data);
void __attribute__((overloadable))
intel_sub_group_block_write4(read_write image2d_t image, int2 coord, ushort4 data);
void __attribute__((overloadable))
intel_sub_group_block_write8(read_write image2d_t image, int2 coord, ushort8 data);
void __attribute__((overloadable))
intel_sub_group_block_write16(read_write image2d_t image, int2 coord, ushort16 data);
#endif // __opencl_c_read_write_images
#endif //__opencl_c_images

ushort __attribute__((overloadable)) intel_sub_group_block_read(const __global ushort *p);
ushort2 __attribute__((overloadable))
intel_sub_group_block_read2(const __global ushort *p);
ushort4 __attribute__((overloadable))
intel_sub_group_block_read4(const __global ushort *p);
ushort8 __attribute__((overloadable))
intel_sub_group_block_read8(const __global ushort *p);
ushort16 __attribute__((overloadable))
intel_sub_group_block_read16(const __global ushort *p);

#ifdef __opencl_c_images
void __attribute__((overloadable))
intel_sub_group_block_write(image2d_t image, int2 coord, ushort data);
void __attribute__((overloadable))
intel_sub_group_block_write2(image2d_t image, int2 coord, ushort2 data);
void __attribute__((overloadable))
intel_sub_group_block_write4(image2d_t image, int2 coord, ushort4 data);
void __attribute__((overloadable))
intel_sub_group_block_write8(image2d_t image, int2 coord, ushort8 data);
void __attribute__((overloadable))
intel_sub_group_block_write16(image2d_t image, int2 coord, ushort16 data);
#endif //__opencl_c_images

void __attribute__((overloadable))
intel_sub_group_block_write(__global ushort *p, ushort data);
void __attribute__((overloadable))
intel_sub_group_block_write2(__global ushort *p, ushort2 data);
void __attribute__((overloadable))
intel_sub_group_block_write4(__global ushort *p, ushort4 data);
void __attribute__((overloadable))
intel_sub_group_block_write8(__global ushort *p, ushort8 data);
#endif // cl_intel_subgroups_half

// ===== cl_intel_subgroups_short (partial overlap with stock Clang; adds 16-wide + image variants) =====
#ifdef cl_intel_subgroups_short
short __attribute__((overloadable))
intel_sub_group_broadcast(short x, uint sub_group_local_id);
short2 __attribute__((overloadable))
intel_sub_group_broadcast(short2 x, uint sub_group_local_id);
short3 __attribute__((overloadable))
intel_sub_group_broadcast(short3 x, uint sub_group_local_id);
short4 __attribute__((overloadable))
intel_sub_group_broadcast(short4 x, uint sub_group_local_id);
short8 __attribute__((overloadable))
intel_sub_group_broadcast(short8 x, uint sub_group_local_id);

ushort __attribute__((overloadable))
intel_sub_group_broadcast(ushort x, uint sub_group_local_id);
ushort2 __attribute__((overloadable))
intel_sub_group_broadcast(ushort2 x, uint sub_group_local_id);
ushort3 __attribute__((overloadable))
intel_sub_group_broadcast(ushort3 x, uint sub_group_local_id);
ushort4 __attribute__((overloadable))
intel_sub_group_broadcast(ushort4 x, uint sub_group_local_id);
ushort8 __attribute__((overloadable))
intel_sub_group_broadcast(ushort8 x, uint sub_group_local_id);

short __attribute__((overloadable))   intel_sub_group_shuffle(short x, uint c);
short2 __attribute__((overloadable))  intel_sub_group_shuffle(short2 x, uint c);
short3 __attribute__((overloadable))  intel_sub_group_shuffle(short3 x, uint c);
short4 __attribute__((overloadable))  intel_sub_group_shuffle(short4 x, uint c);
short8 __attribute__((overloadable))  intel_sub_group_shuffle(short8 x, uint c);
short16 __attribute__((overloadable)) intel_sub_group_shuffle(short16 x, uint c);

ushort __attribute__((overloadable))   intel_sub_group_shuffle(ushort x, uint c);
ushort2 __attribute__((overloadable))  intel_sub_group_shuffle(ushort2 x, uint c);
ushort3 __attribute__((overloadable))  intel_sub_group_shuffle(ushort3 x, uint c);
ushort4 __attribute__((overloadable))  intel_sub_group_shuffle(ushort4 x, uint c);
ushort8 __attribute__((overloadable))  intel_sub_group_shuffle(ushort8 x, uint c);
ushort16 __attribute__((overloadable)) intel_sub_group_shuffle(ushort16 x, uint c);

short __attribute__((overloadable))
intel_sub_group_shuffle_down(short cur, short next, uint c);
short2 __attribute__((overloadable))
intel_sub_group_shuffle_down(short2 cur, short2 next, uint c);
short3 __attribute__((overloadable))
intel_sub_group_shuffle_down(short3 cur, short3 next, uint c);
short4 __attribute__((overloadable))
intel_sub_group_shuffle_down(short4 cur, short4 next, uint c);
short8 __attribute__((overloadable))
intel_sub_group_shuffle_down(short8 cur, short8 next, uint c);
short16 __attribute__((overloadable))
intel_sub_group_shuffle_down(short16 cur, short16 next, uint c);

ushort __attribute__((overloadable))
intel_sub_group_shuffle_down(ushort cur, ushort next, uint c);
ushort2 __attribute__((overloadable))
intel_sub_group_shuffle_down(ushort2 cur, ushort2 next, uint c);
ushort3 __attribute__((overloadable))
intel_sub_group_shuffle_down(ushort3 cur, ushort3 next, uint c);
ushort4 __attribute__((overloadable))
intel_sub_group_shuffle_down(ushort4 cur, ushort4 next, uint c);
ushort8 __attribute__((overloadable))
intel_sub_group_shuffle_down(ushort8 cur, ushort8 next, uint c);
ushort16 __attribute__((overloadable))
intel_sub_group_shuffle_down(ushort16 cur, ushort16 next, uint c);

short __attribute__((overloadable))
intel_sub_group_shuffle_up(short cur, short next, uint c);
short2 __attribute__((overloadable))
intel_sub_group_shuffle_up(short2 cur, short2 next, uint c);
short3 __attribute__((overloadable))
intel_sub_group_shuffle_up(short3 cur, short3 next, uint c);
short4 __attribute__((overloadable))
intel_sub_group_shuffle_up(short4 cur, short4 next, uint c);
short8 __attribute__((overloadable))
intel_sub_group_shuffle_up(short8 cur, short8 next, uint c);
short16 __attribute__((overloadable))
intel_sub_group_shuffle_up(short16 cur, short16 next, uint c);

ushort __attribute__((overloadable))
intel_sub_group_shuffle_up(ushort cur, ushort next, uint c);
ushort2 __attribute__((overloadable))
intel_sub_group_shuffle_up(ushort2 cur, ushort2 next, uint c);
ushort3 __attribute__((overloadable))
intel_sub_group_shuffle_up(ushort3 cur, ushort3 next, uint c);
ushort4 __attribute__((overloadable))
intel_sub_group_shuffle_up(ushort4 cur, ushort4 next, uint c);
ushort8 __attribute__((overloadable))
intel_sub_group_shuffle_up(ushort8 cur, ushort8 next, uint c);
ushort16 __attribute__((overloadable))
intel_sub_group_shuffle_up(ushort16 cur, ushort16 next, uint c);

short __attribute__((overloadable))   intel_sub_group_shuffle_xor(short x, uint c);
short2 __attribute__((overloadable))  intel_sub_group_shuffle_xor(short2 x, uint c);
short3 __attribute__((overloadable))  intel_sub_group_shuffle_xor(short3 x, uint c);
short4 __attribute__((overloadable))  intel_sub_group_shuffle_xor(short4 x, uint c);
short8 __attribute__((overloadable))  intel_sub_group_shuffle_xor(short8 x, uint c);
short16 __attribute__((overloadable)) intel_sub_group_shuffle_xor(short16 x, uint c);

ushort __attribute__((overloadable))   intel_sub_group_shuffle_xor(ushort x, uint c);
ushort2 __attribute__((overloadable))  intel_sub_group_shuffle_xor(ushort2 x, uint c);
ushort3 __attribute__((overloadable))  intel_sub_group_shuffle_xor(ushort3 x, uint c);
ushort4 __attribute__((overloadable))  intel_sub_group_shuffle_xor(ushort4 x, uint c);
ushort8 __attribute__((overloadable))  intel_sub_group_shuffle_xor(ushort8 x, uint c);
ushort16 __attribute__((overloadable)) intel_sub_group_shuffle_xor(ushort16 x, uint c);

DECL_GROUP_REDUCE_SCAN(intel_sub_group, short)
DECL_GROUP_REDUCE_SCAN(intel_sub_group, ushort)

#ifdef __opencl_c_images
ushort __attribute__((overloadable))
intel_sub_group_block_read_us(read_only image2d_t image, int2 coord);
ushort2 __attribute__((overloadable))
intel_sub_group_block_read_us2(read_only image2d_t image, int2 coord);
ushort4 __attribute__((overloadable))
intel_sub_group_block_read_us4(read_only image2d_t image, int2 coord);
ushort8 __attribute__((overloadable))
intel_sub_group_block_read_us8(read_only image2d_t image, int2 coord);
ushort16 __attribute__((overloadable))
intel_sub_group_block_read_us16(read_only image2d_t image, int2 coord);

#ifdef __opencl_c_read_write_images
ushort __attribute__((overloadable))
intel_sub_group_block_read_us(read_write image2d_t image, int2 coord);
ushort2 __attribute__((overloadable))
intel_sub_group_block_read_us2(read_write image2d_t image, int2 coord);
ushort4 __attribute__((overloadable))
intel_sub_group_block_read_us4(read_write image2d_t image, int2 coord);
ushort8 __attribute__((overloadable))
intel_sub_group_block_read_us8(read_write image2d_t image, int2 coord);
ushort16 __attribute__((overloadable))
intel_sub_group_block_read_us16(read_write image2d_t image, int2 coord);

void __attribute__((overloadable))
intel_sub_group_block_write_us(read_write image2d_t image, int2 coord, ushort data);
void __attribute__((overloadable))
intel_sub_group_block_write_us2(read_write image2d_t image, int2 coord, ushort2 data);
void __attribute__((overloadable))
intel_sub_group_block_write_us4(read_write image2d_t image, int2 coord, ushort4 data);
void __attribute__((overloadable))
intel_sub_group_block_write_us8(read_write image2d_t image, int2 coord, ushort8 data);
void __attribute__((overloadable))
intel_sub_group_block_write_us16(read_write image2d_t image, int2 coord, ushort16 data);
#endif // __opencl_c_read_write_images
#endif //__opencl_c_images

ushort __attribute__((overloadable))
intel_sub_group_block_read_us(const __global ushort *p);
ushort2 __attribute__((overloadable))
intel_sub_group_block_read_us2(const __global ushort *p);
ushort4 __attribute__((overloadable))
intel_sub_group_block_read_us4(const __global ushort *p);
ushort8 __attribute__((overloadable))
intel_sub_group_block_read_us8(const __global ushort *p);
ushort16 __attribute__((overloadable))
intel_sub_group_block_read_us16(const __global ushort *p);

#ifdef __opencl_c_images
void __attribute__((overloadable))
intel_sub_group_block_write_us(write_only image2d_t image, int2 coord, ushort data);
void __attribute__((overloadable))
intel_sub_group_block_write_us2(write_only image2d_t image, int2 coord, ushort2 data);
void __attribute__((overloadable))
intel_sub_group_block_write_us4(write_only image2d_t image, int2 coord, ushort4 data);
void __attribute__((overloadable))
intel_sub_group_block_write_us8(write_only image2d_t image, int2 coord, ushort8 data);
void __attribute__((overloadable))
intel_sub_group_block_write_us16(write_only image2d_t image, int2 coord, ushort16 data);
#endif //__opencl_c_images

void __attribute__((overloadable))
intel_sub_group_block_write_us(__global ushort *p, ushort data);
void __attribute__((overloadable))
intel_sub_group_block_write_us2(__global ushort *p, ushort2 data);
void __attribute__((overloadable))
intel_sub_group_block_write_us4(__global ushort *p, ushort4 data);
void __attribute__((overloadable))
intel_sub_group_block_write_us8(__global ushort *p, ushort8 data);
void __attribute__((overloadable))
intel_sub_group_block_write_us16(__global ushort *p, ushort16 data);

#endif // cl_intel_subgroups_short

// ===== cl_intel_subgroups_char =====
#ifdef cl_intel_subgroups_char
char __attribute__((overloadable))
intel_sub_group_broadcast(char x, uint sub_group_local_id);
char2 __attribute__((overloadable))
intel_sub_group_broadcast(char2 x, uint sub_group_local_id);
char3 __attribute__((overloadable))
intel_sub_group_broadcast(char3 x, uint sub_group_local_id);
char4 __attribute__((overloadable))
intel_sub_group_broadcast(char4 x, uint sub_group_local_id);
char8 __attribute__((overloadable))
intel_sub_group_broadcast(char8 x, uint sub_group_local_id);

uchar __attribute__((overloadable))
intel_sub_group_broadcast(uchar x, uint sub_group_local_id);
uchar2 __attribute__((overloadable))
intel_sub_group_broadcast(uchar2 x, uint sub_group_local_id);
uchar3 __attribute__((overloadable))
intel_sub_group_broadcast(uchar3 x, uint sub_group_local_id);
uchar4 __attribute__((overloadable))
intel_sub_group_broadcast(uchar4 x, uint sub_group_local_id);
uchar8 __attribute__((overloadable))
intel_sub_group_broadcast(uchar8 x, uint sub_group_local_id);

char __attribute__((overloadable))   intel_sub_group_shuffle(char x, uint c);
char2 __attribute__((overloadable))  intel_sub_group_shuffle(char2 x, uint c);
char3 __attribute__((overloadable))  intel_sub_group_shuffle(char3 x, uint c);
char4 __attribute__((overloadable))  intel_sub_group_shuffle(char4 x, uint c);
char8 __attribute__((overloadable))  intel_sub_group_shuffle(char8 x, uint c);
char16 __attribute__((overloadable)) intel_sub_group_shuffle(char16 x, uint c);

uchar __attribute__((overloadable))   intel_sub_group_shuffle(uchar x, uint c);
uchar2 __attribute__((overloadable))  intel_sub_group_shuffle(uchar2 x, uint c);
uchar3 __attribute__((overloadable))  intel_sub_group_shuffle(uchar3 x, uint c);
uchar4 __attribute__((overloadable))  intel_sub_group_shuffle(uchar4 x, uint c);
uchar8 __attribute__((overloadable))  intel_sub_group_shuffle(uchar8 x, uint c);
uchar16 __attribute__((overloadable)) intel_sub_group_shuffle(uchar16 x, uint c);

char __attribute__((overloadable))
intel_sub_group_shuffle_down(char cur, char next, uint c);
char2 __attribute__((overloadable))
intel_sub_group_shuffle_down(char2 cur, char2 next, uint c);
char3 __attribute__((overloadable))
intel_sub_group_shuffle_down(char3 cur, char3 next, uint c);
char4 __attribute__((overloadable))
intel_sub_group_shuffle_down(char4 cur, char4 next, uint c);
char8 __attribute__((overloadable))
intel_sub_group_shuffle_down(char8 cur, char8 next, uint c);
char16 __attribute__((overloadable))
intel_sub_group_shuffle_down(char16 cur, char16 next, uint c);

uchar __attribute__((overloadable))
intel_sub_group_shuffle_down(uchar cur, uchar next, uint c);
uchar2 __attribute__((overloadable))
intel_sub_group_shuffle_down(uchar2 cur, uchar2 next, uint c);
uchar3 __attribute__((overloadable))
intel_sub_group_shuffle_down(uchar3 cur, uchar3 next, uint c);
uchar4 __attribute__((overloadable))
intel_sub_group_shuffle_down(uchar4 cur, uchar4 next, uint c);
uchar8 __attribute__((overloadable))
intel_sub_group_shuffle_down(uchar8 cur, uchar8 next, uint c);
uchar16 __attribute__((overloadable))
intel_sub_group_shuffle_down(uchar16 cur, uchar16 next, uint c);

char __attribute__((overloadable))
intel_sub_group_shuffle_up(char cur, char next, uint c);
char2 __attribute__((overloadable))
intel_sub_group_shuffle_up(char2 cur, char2 next, uint c);
char3 __attribute__((overloadable))
intel_sub_group_shuffle_up(char3 cur, char3 next, uint c);
char4 __attribute__((overloadable))
intel_sub_group_shuffle_up(char4 cur, char4 next, uint c);
char8 __attribute__((overloadable))
intel_sub_group_shuffle_up(char8 cur, char8 next, uint c);
char16 __attribute__((overloadable))
intel_sub_group_shuffle_up(char16 cur, char16 next, uint c);

uchar __attribute__((overloadable))
intel_sub_group_shuffle_up(uchar cur, uchar next, uint c);
uchar2 __attribute__((overloadable))
intel_sub_group_shuffle_up(uchar2 cur, uchar2 next, uint c);
uchar3 __attribute__((overloadable))
intel_sub_group_shuffle_up(uchar3 cur, uchar3 next, uint c);
uchar4 __attribute__((overloadable))
intel_sub_group_shuffle_up(uchar4 cur, uchar4 next, uint c);
uchar8 __attribute__((overloadable))
intel_sub_group_shuffle_up(uchar8 cur, uchar8 next, uint c);
uchar16 __attribute__((overloadable))
intel_sub_group_shuffle_up(uchar16 cur, uchar16 next, uint c);

char __attribute__((overloadable))   intel_sub_group_shuffle_xor(char x, uint c);
char2 __attribute__((overloadable))  intel_sub_group_shuffle_xor(char2 x, uint c);
char3 __attribute__((overloadable))  intel_sub_group_shuffle_xor(char3 x, uint c);
char4 __attribute__((overloadable))  intel_sub_group_shuffle_xor(char4 x, uint c);
char8 __attribute__((overloadable))  intel_sub_group_shuffle_xor(char8 x, uint c);
char16 __attribute__((overloadable)) intel_sub_group_shuffle_xor(char16 x, uint c);

uchar __attribute__((overloadable))   intel_sub_group_shuffle_xor(uchar x, uint c);
uchar2 __attribute__((overloadable))  intel_sub_group_shuffle_xor(uchar2 x, uint c);
uchar3 __attribute__((overloadable))  intel_sub_group_shuffle_xor(uchar3 x, uint c);
uchar4 __attribute__((overloadable))  intel_sub_group_shuffle_xor(uchar4 x, uint c);
uchar8 __attribute__((overloadable))  intel_sub_group_shuffle_xor(uchar8 x, uint c);
uchar16 __attribute__((overloadable)) intel_sub_group_shuffle_xor(uchar16 x, uint c);

DECL_GROUP_REDUCE_SCAN(intel_sub_group, char)
DECL_GROUP_REDUCE_SCAN(intel_sub_group, uchar)

#ifdef __opencl_c_images
uchar __attribute__((overloadable))
intel_sub_group_block_read_uc(read_only image2d_t image, int2 coord);
uchar2 __attribute__((overloadable))
intel_sub_group_block_read_uc2(read_only image2d_t image, int2 coord);
uchar4 __attribute__((overloadable))
intel_sub_group_block_read_uc4(read_only image2d_t image, int2 coord);
uchar8 __attribute__((overloadable))
intel_sub_group_block_read_uc8(read_only image2d_t image, int2 coord);
uchar16 __attribute__((overloadable))
intel_sub_group_block_read_uc16(read_only image2d_t image, int2 coord);

#ifdef __opencl_c_read_write_images
uchar __attribute__((overloadable))
intel_sub_group_block_read_uc(read_write image2d_t image, int2 coord);
uchar2 __attribute__((overloadable))
intel_sub_group_block_read_uc2(read_write image2d_t image, int2 coord);
uchar4 __attribute__((overloadable))
intel_sub_group_block_read_uc4(read_write image2d_t image, int2 coord);
uchar8 __attribute__((overloadable))
intel_sub_group_block_read_uc8(read_write image2d_t image, int2 coord);
uchar16 __attribute__((overloadable))
intel_sub_group_block_read_uc16(read_write image2d_t image, int2 coord);

void __attribute__((overloadable))
intel_sub_group_block_write_uc(read_write image2d_t image, int2 coord, uchar data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc2(read_write image2d_t image, int2 coord, uchar2 data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc4(read_write image2d_t image, int2 coord, uchar4 data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc8(read_write image2d_t image, int2 coord, uchar8 data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc16(read_write image2d_t image, int2 coord, uchar16 data);
#endif // __opencl_c_read_write_images
#endif //__opencl_c_images

uchar __attribute__((overloadable))
intel_sub_group_block_read_uc(const __global uchar *p);
uchar2 __attribute__((overloadable))
intel_sub_group_block_read_uc2(const __global uchar *p);
uchar4 __attribute__((overloadable))
intel_sub_group_block_read_uc4(const __global uchar *p);
uchar8 __attribute__((overloadable))
intel_sub_group_block_read_uc8(const __global uchar *p);
uchar16 __attribute__((overloadable))
intel_sub_group_block_read_uc16(const __global uchar *p);

#ifdef __opencl_c_images
void __attribute__((overloadable))
intel_sub_group_block_write_uc(write_only image2d_t image, int2 coord, uchar data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc2(write_only image2d_t image, int2 coord, uchar2 data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc4(write_only image2d_t image, int2 coord, uchar4 data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc8(write_only image2d_t image, int2 coord, uchar8 data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc16(write_only image2d_t image, int2 coord, uchar16 data);
#endif //__opencl_c_images

void __attribute__((overloadable))
intel_sub_group_block_write_uc(__global uchar *p, uchar data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc2(__global uchar *p, uchar2 data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc4(__global uchar *p, uchar4 data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc8(__global uchar *p, uchar8 data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc16(__global uchar *p, uchar16 data);

#endif // cl_intel_subgroups_char

// ===== cl_intel_subgroups_long =====
#ifdef cl_intel_subgroups_long
#ifdef __opencl_c_images
ulong __attribute__((overloadable))
intel_sub_group_block_read_ul(read_only image2d_t image, int2 coord);
ulong2 __attribute__((overloadable))
intel_sub_group_block_read_ul2(read_only image2d_t image, int2 coord);
ulong4 __attribute__((overloadable))
intel_sub_group_block_read_ul4(read_only image2d_t image, int2 coord);
ulong8 __attribute__((overloadable))
intel_sub_group_block_read_ul8(read_only image2d_t image, int2 coord);

#ifdef __opencl_c_read_write_images
ulong __attribute__((overloadable))
intel_sub_group_block_read_ul(read_write image2d_t image, int2 coord);
ulong2 __attribute__((overloadable))
intel_sub_group_block_read_ul2(read_write image2d_t image, int2 coord);
ulong4 __attribute__((overloadable))
intel_sub_group_block_read_ul4(read_write image2d_t image, int2 coord);
ulong8 __attribute__((overloadable))
intel_sub_group_block_read_ul8(read_write image2d_t image, int2 coord);

void __attribute__((overloadable))
intel_sub_group_block_write_ul(read_write image2d_t image, int2 coord, ulong data);
void __attribute__((overloadable))
intel_sub_group_block_write_ul2(read_write image2d_t image, int2 coord, ulong2 data);
void __attribute__((overloadable))
intel_sub_group_block_write_ul4(read_write image2d_t image, int2 coord, ulong4 data);
void __attribute__((overloadable))
intel_sub_group_block_write_ul8(read_write image2d_t image, int2 coord, ulong8 data);
#endif // __opencl_c_read_write_images
#endif //__opencl_c_images

ulong __attribute__((overloadable))
intel_sub_group_block_read_ul(const __global ulong *p);
ulong2 __attribute__((overloadable))
intel_sub_group_block_read_ul2(const __global ulong *p);
ulong4 __attribute__((overloadable))
intel_sub_group_block_read_ul4(const __global ulong *p);
ulong8 __attribute__((overloadable))
intel_sub_group_block_read_ul8(const __global ulong *p);

#ifdef __opencl_c_images
void __attribute__((overloadable))
intel_sub_group_block_write_ul(write_only image2d_t image, int2 coord, ulong data);
void __attribute__((overloadable))
intel_sub_group_block_write_ul2(write_only image2d_t image, int2 coord, ulong2 data);
void __attribute__((overloadable))
intel_sub_group_block_write_ul4(write_only image2d_t image, int2 coord, ulong4 data);
void __attribute__((overloadable))
intel_sub_group_block_write_ul8(write_only image2d_t image, int2 coord, ulong8 data);
#endif //__opencl_c_images

void __attribute__((overloadable))
intel_sub_group_block_write_ul(__global ulong *p, ulong data);
void __attribute__((overloadable))
intel_sub_group_block_write_ul2(__global ulong *p, ulong2 data);
void __attribute__((overloadable))
intel_sub_group_block_write_ul4(__global ulong *p, ulong4 data);
void __attribute__((overloadable))
intel_sub_group_block_write_ul8(__global ulong *p, ulong8 data);

#endif // cl_intel_subgroups_long

// ===== cl_intel_subgroup_local_block_io =====
#ifdef cl_intel_subgroup_local_block_io

uint __attribute__((overloadable))  intel_sub_group_block_read(const __local uint *p);
uint2 __attribute__((overloadable)) intel_sub_group_block_read2(const __local uint *p);
uint4 __attribute__((overloadable)) intel_sub_group_block_read4(const __local uint *p);
uint8 __attribute__((overloadable)) intel_sub_group_block_read8(const __local uint *p);

void __attribute__((overloadable))
intel_sub_group_block_write(__local uint *p, uint data);
void __attribute__((overloadable))
intel_sub_group_block_write2(__local uint *p, uint2 data);
void __attribute__((overloadable))
intel_sub_group_block_write4(__local uint *p, uint4 data);
void __attribute__((overloadable))
intel_sub_group_block_write8(__local uint *p, uint8 data);

ushort __attribute__((overloadable))
intel_sub_group_block_read_us(const __local ushort *p);
ushort2 __attribute__((overloadable))
intel_sub_group_block_read_us2(const __local ushort *p);
ushort4 __attribute__((overloadable))
intel_sub_group_block_read_us4(const __local ushort *p);
ushort8 __attribute__((overloadable))
intel_sub_group_block_read_us8(const __local ushort *p);

void __attribute__((overloadable))
intel_sub_group_block_write_us(__local ushort *p, ushort data);
void __attribute__((overloadable))
intel_sub_group_block_write_us2(__local ushort *p, ushort2 data);
void __attribute__((overloadable))
intel_sub_group_block_write_us4(__local ushort *p, ushort4 data);
void __attribute__((overloadable))
intel_sub_group_block_write_us8(__local ushort *p, ushort8 data);

#ifdef cl_intel_subgroups_char
uchar __attribute__((overloadable)) intel_sub_group_block_read_uc(const __local uchar *p);
uchar2 __attribute__((overloadable))
intel_sub_group_block_read_uc2(const __local uchar *p);
uchar4 __attribute__((overloadable))
intel_sub_group_block_read_uc4(const __local uchar *p);
uchar8 __attribute__((overloadable))
intel_sub_group_block_read_uc8(const __local uchar *p);
uchar16 __attribute__((overloadable))
intel_sub_group_block_read_uc16(const __local uchar *p);

void __attribute__((overloadable))
intel_sub_group_block_write_uc(__local uchar *p, uchar data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc2(__local uchar *p, uchar2 data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc4(__local uchar *p, uchar4 data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc8(__local uchar *p, uchar8 data);
void __attribute__((overloadable))
intel_sub_group_block_write_uc16(__local uchar *p, uchar16 data);
#endif // cl_intel_subgroups_char

#ifdef cl_intel_subgroups_long
ulong __attribute__((overloadable)) intel_sub_group_block_read_ul(const __local ulong *p);
ulong2 __attribute__((overloadable))
intel_sub_group_block_read_ul2(const __local ulong *p);
ulong4 __attribute__((overloadable))
intel_sub_group_block_read_ul4(const __local ulong *p);
ulong8 __attribute__((overloadable))
intel_sub_group_block_read_ul8(const __local ulong *p);

void __attribute__((overloadable))
intel_sub_group_block_write_ul(__local ulong *p, ulong data);
void __attribute__((overloadable))
intel_sub_group_block_write_ul2(__local ulong *p, ulong2 data);
void __attribute__((overloadable))
intel_sub_group_block_write_ul4(__local ulong *p, ulong4 data);
void __attribute__((overloadable))
intel_sub_group_block_write_ul8(__local ulong *p, ulong8 data);
#endif // cl_intel_subgroups_long

#endif // cl_intel_subgroup_local_block_io

// ===== cl_intel_subgroup_buffer_prefetch =====
#ifdef cl_intel_subgroup_buffer_prefetch
void __attribute__((overloadable))
intel_sub_group_block_prefetch_uc(const __global uchar *p);
void __attribute__((overloadable))
intel_sub_group_block_prefetch_uc2(const __global uchar *p);
void __attribute__((overloadable))
intel_sub_group_block_prefetch_uc4(const __global uchar *p);
void __attribute__((overloadable))
intel_sub_group_block_prefetch_uc8(const __global uchar *p);
void __attribute__((overloadable))
intel_sub_group_block_prefetch_uc16(const __global uchar *p);

void __attribute__((overloadable))
intel_sub_group_block_prefetch_us(const __global ushort *p);
void __attribute__((overloadable))
intel_sub_group_block_prefetch_us2(const __global ushort *p);
void __attribute__((overloadable))
intel_sub_group_block_prefetch_us4(const __global ushort *p);
void __attribute__((overloadable))
intel_sub_group_block_prefetch_us8(const __global ushort *p);
void __attribute__((overloadable))
intel_sub_group_block_prefetch_us16(const __global ushort *p);

void __attribute__((overloadable))
intel_sub_group_block_prefetch_ui(const __global uint *p);
void __attribute__((overloadable))
intel_sub_group_block_prefetch_ui2(const __global uint *p);
void __attribute__((overloadable))
intel_sub_group_block_prefetch_ui4(const __global uint *p);
void __attribute__((overloadable))
intel_sub_group_block_prefetch_ui8(const __global uint *p);

void __attribute__((overloadable))
intel_sub_group_block_prefetch_ul(const __global ulong *p);
void __attribute__((overloadable))
intel_sub_group_block_prefetch_ul2(const __global ulong *p);
void __attribute__((overloadable))
intel_sub_group_block_prefetch_ul4(const __global ulong *p);
void __attribute__((overloadable))
intel_sub_group_block_prefetch_ul8(const __global ulong *p);
#endif // cl_intel_subgroup_buffer_prefetch

// ===== cl_intel_media_block_io =====
#ifdef cl_intel_media_block_io

// Media Block read/write extension

//read
#ifdef __opencl_c_images
uchar __attribute__((overloadable)) intel_sub_group_media_block_read_uc(
    int2 src_offset, int width, int height, read_only image2d_t image);
uchar2 __attribute__((overloadable)) intel_sub_group_media_block_read_uc2(
    int2 src_offset, int width, int height, read_only image2d_t image);
uchar4 __attribute__((overloadable)) intel_sub_group_media_block_read_uc4(
    int2 src_offset, int width, int height, read_only image2d_t image);
uchar8 __attribute__((overloadable)) intel_sub_group_media_block_read_uc8(
    int2 src_offset, int width, int height, read_only image2d_t image);
uchar16 __attribute__((overloadable)) intel_sub_group_media_block_read_uc16(
    int2 src_offset, int width, int height, read_only image2d_t image);

ushort __attribute__((overloadable)) intel_sub_group_media_block_read_us(
    int2 src_offset, int width, int height, read_only image2d_t image);
ushort2 __attribute__((overloadable)) intel_sub_group_media_block_read_us2(
    int2 src_offset, int width, int height, read_only image2d_t image);
ushort4 __attribute__((overloadable)) intel_sub_group_media_block_read_us4(
    int2 src_offset, int width, int height, read_only image2d_t image);
ushort8 __attribute__((overloadable)) intel_sub_group_media_block_read_us8(
    int2 src_offset, int width, int height, read_only image2d_t image);
ushort16 __attribute__((overloadable)) intel_sub_group_media_block_read_us16(
    int2 src_offset, int width, int height, read_only image2d_t image);

uint __attribute__((overloadable)) intel_sub_group_media_block_read_ui(
    int2 src_offset, int width, int height, read_only image2d_t image);
uint2 __attribute__((overloadable)) intel_sub_group_media_block_read_ui2(
    int2 src_offset, int width, int height, read_only image2d_t image);
uint4 __attribute__((overloadable)) intel_sub_group_media_block_read_ui4(
    int2 src_offset, int width, int height, read_only image2d_t image);
uint8 __attribute__((overloadable)) intel_sub_group_media_block_read_ui8(
    int2 src_offset, int width, int height, read_only image2d_t image);

#ifdef __opencl_c_read_write_images
uchar __attribute__((overloadable)) intel_sub_group_media_block_read_uc(
    int2 src_offset, int width, int height, read_write image2d_t image);
uchar2 __attribute__((overloadable)) intel_sub_group_media_block_read_uc2(
    int2 src_offset, int width, int height, read_write image2d_t image);
uchar4 __attribute__((overloadable)) intel_sub_group_media_block_read_uc4(
    int2 src_offset, int width, int height, read_write image2d_t image);
uchar8 __attribute__((overloadable)) intel_sub_group_media_block_read_uc8(
    int2 src_offset, int width, int height, read_write image2d_t image);
uchar16 __attribute__((overloadable)) intel_sub_group_media_block_read_uc16(
    int2 src_offset, int width, int height, read_write image2d_t image);

ushort __attribute__((overloadable)) intel_sub_group_media_block_read_us(
    int2 src_offset, int width, int height, read_write image2d_t image);
ushort2 __attribute__((overloadable)) intel_sub_group_media_block_read_us2(
    int2 src_offset, int width, int height, read_write image2d_t image);
ushort4 __attribute__((overloadable)) intel_sub_group_media_block_read_us4(
    int2 src_offset, int width, int height, read_write image2d_t image);
ushort8 __attribute__((overloadable)) intel_sub_group_media_block_read_us8(
    int2 src_offset, int width, int height, read_write image2d_t image);
ushort16 __attribute__((overloadable)) intel_sub_group_media_block_read_us16(
    int2 src_offset, int width, int height, read_write image2d_t image);

uint __attribute__((overloadable)) intel_sub_group_media_block_read_ui(
    int2 src_offset, int width, int height, read_write image2d_t image);
uint2 __attribute__((overloadable)) intel_sub_group_media_block_read_ui2(
    int2 src_offset, int width, int height, read_write image2d_t image);
uint4 __attribute__((overloadable)) intel_sub_group_media_block_read_ui4(
    int2 src_offset, int width, int height, read_write image2d_t image);
uint8 __attribute__((overloadable)) intel_sub_group_media_block_read_ui8(
    int2 src_offset, int width, int height, read_write image2d_t image);
#endif // __opencl_c_read_write_images

// write

void __attribute__((overloadable)) intel_sub_group_media_block_write_uc(
    int2 src_offset, int width, int height, uchar pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc2(
    int2 src_offset, int width, int height, uchar2 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc4(
    int2 src_offset, int width, int height, uchar4 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc8(
    int2 src_offset, int width, int height, uchar8 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc16(
    int2 src_offset, int width, int height, uchar16 pixels, write_only image2d_t image);

void __attribute__((overloadable)) intel_sub_group_media_block_write_us(
    int2 src_offset, int width, int height, ushort pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us2(
    int2 src_offset, int width, int height, ushort2 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us4(
    int2 src_offset, int width, int height, ushort4 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us8(
    int2 src_offset, int width, int height, ushort8 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us16(
    int2 src_offset, int width, int height, ushort16 pixels, write_only image2d_t image);

void __attribute__((overloadable)) intel_sub_group_media_block_write_ui(
    int2 src_offset, int width, int height, uint pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui2(
    int2 src_offset, int width, int height, uint2 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui4(
    int2 src_offset, int width, int height, uint4 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui8(
    int2 src_offset, int width, int height, uint8 pixels, write_only image2d_t image);

#ifdef __opencl_c_read_write_images
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc(
    int2 src_offset, int width, int height, uchar pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc2(
    int2 src_offset, int width, int height, uchar2 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc4(
    int2 src_offset, int width, int height, uchar4 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc8(
    int2 src_offset, int width, int height, uchar8 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc16(
    int2 src_offset, int width, int height, uchar16 pixels, read_write image2d_t image);

void __attribute__((overloadable)) intel_sub_group_media_block_write_us(
    int2 src_offset, int width, int height, ushort pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us2(
    int2 src_offset, int width, int height, ushort2 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us4(
    int2 src_offset, int width, int height, ushort4 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us8(
    int2 src_offset, int width, int height, ushort8 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us16(
    int2 src_offset, int width, int height, ushort16 pixels, read_write image2d_t image);

void __attribute__((overloadable)) intel_sub_group_media_block_write_ui(
    int2 src_offset, int width, int height, uint pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui2(
    int2 src_offset, int width, int height, uint2 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui4(
    int2 src_offset, int width, int height, uint4 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui8(
    int2 src_offset, int width, int height, uint8 pixels, read_write image2d_t image);
#endif // __opencl_c_read_write_images
#endif //__opencl_c_images

#endif // cl_intel_media_block_io

// ===== cl_intel_subgroup_2d_block_io =====
#ifdef cl_intel_subgroup_2d_block_io

////////////////////////////////////////////////////////////////
// 2D Block Load, 8-bit data, Rows in [1, 2, 4, 8], Columns in [32x2]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_8b_1r32x2c(
    __global void    *base_address,
    int               width,
    int               height,
    int               pitch,
    int2              coord,
    __private ushort *destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_8b_2r32x2c(
    __global void    *base_address,
    int               width,
    int               height,
    int               pitch,
    int2              coord,
    __private ushort *destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_8b_4r32x2c(
    __global void    *base_address,
    int               width,
    int               height,
    int               pitch,
    int2              coord,
    __private ushort *destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_8b_8r32x2c(
    __global void    *base_address,
    int               width,
    int               height,
    int               pitch,
    int2              coord,
    __private ushort *destination);

void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_1r32x2c(
    __global void *base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_2r32x2c(
    __global void *base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_4r32x2c(
    __global void *base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_8r32x2c(
    __global void *base_address, int width, int height, int pitch, int2 coord);

////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// 2D Block Load, 16-bit data, Rows in [1, 2, 4, 8], Columns in [16x2]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_16b_1r16x2c(
    __global void    *base_address,
    int               width,
    int               height,
    int               pitch,
    int2              coord,
    __private ushort *destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_16b_2r16x2c(
    __global void    *base_address,
    int               width,
    int               height,
    int               pitch,
    int2              coord,
    __private ushort *destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_16b_4r16x2c(
    __global void    *base_address,
    int               width,
    int               height,
    int               pitch,
    int2              coord,
    __private ushort *destination);
void __attribute__((overloadable)) intel_sub_group_2d_block_read_16b_8r16x2c(
    __global void    *base_address,
    int               width,
    int               height,
    int               pitch,
    int2              coord,
    __private ushort *destination);

void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_16b_1r16x2c(
    __global void *base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_16b_2r16x2c(
    __global void *base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_16b_4r16x2c(
    __global void *base_address, int width, int height, int pitch, int2 coord);
void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_16b_8r16x2c(
    __global void *base_address, int width, int height, int pitch, int2 coord);

////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// 2D Block Load with VNNI Transform, 8-bit data, Rows in [32*], Columns in [16]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_transform_8b_32r16x1c(
    __global void  *base_address,
    int             width,
    int             height,
    int             pitch,
    int2            coord,
    __private uint *destination);

void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_8b_32r16x1c(
    __global void *base_address, int width, int height, int pitch, int2 coord);

////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// 2D Block Load with VNNI Transform, 16-bit data, Rows in [16, 32], Columns in [16]:

void __attribute__((overloadable)) intel_sub_group_2d_block_read_transform_16b_16r16x1c(
    __global void  *base_address,
    int             width,
    int             height,
    int             pitch,
    int2            coord,
    __private uint *destination);

void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_16b_16r16x1c(
    __global void *base_address, int width, int height, int pitch, int2 coord);

// 32 row version implemented in opencl_cth_pre_release.h

////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// 2D Block Load with Transpose, 32-bit data, Rows in [16], Columns in [1, 2, 4, 8]:

// 1, 2 and 4 columns versions TBD

void __attribute__((overloadable)) intel_sub_group_2d_block_read_transpose_32b_16r8x1c(
    __global void  *base_address,
    int             width,
    int             height,
    int             pitch,
    int2            coord,
    __private uint *destination);

void __attribute__((overloadable)) intel_sub_group_2d_block_prefetch_32b_16r8x1c(
    __global void *base_address, int width, int height, int pitch, int2 coord);

////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// 2D Block Store, 8-bit data, Rows in [1, 2, 4, 8], Columns in [32]:

void __attribute__((overloadable)) intel_sub_group_2d_block_write_8b_1r32x1c(
    __global void  *base_address,
    int             width,
    int             height,
    int             pitch,
    int2            coord,
    private ushort *val);
void __attribute__((overloadable)) intel_sub_group_2d_block_write_8b_2r32x1c(
    __global void  *base_address,
    int             width,
    int             height,
    int             pitch,
    int2            coord,
    private ushort *val);
void __attribute__((overloadable)) intel_sub_group_2d_block_write_8b_4r32x1c(
    __global void  *base_address,
    int             width,
    int             height,
    int             pitch,
    int2            coord,
    private ushort *val);
void __attribute__((overloadable)) intel_sub_group_2d_block_write_8b_8r32x1c(
    __global void  *base_address,
    int             width,
    int             height,
    int             pitch,
    int2            coord,
    private ushort *val);

////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// 2D Block Store, 16-bit data, Rows in [1, 2, 4, 8], Columns in [16]:

void __attribute__((overloadable)) intel_sub_group_2d_block_write_16b_1r16x1c(
    __global void  *base_address,
    int             width,
    int             height,
    int             pitch,
    int2            coord,
    private ushort *val);
void __attribute__((overloadable)) intel_sub_group_2d_block_write_16b_2r16x1c(
    __global void  *base_address,
    int             width,
    int             height,
    int             pitch,
    int2            coord,
    private ushort *val);
void __attribute__((overloadable)) intel_sub_group_2d_block_write_16b_4r16x1c(
    __global void  *base_address,
    int             width,
    int             height,
    int             pitch,
    int2            coord,
    private ushort *val);
void __attribute__((overloadable)) intel_sub_group_2d_block_write_16b_8r16x1c(
    __global void  *base_address,
    int             width,
    int             height,
    int             pitch,
    int2            coord,
    private ushort *val);

////////////////////////////////////////////////////////////////

#endif //defined(cl_intel_subgroup_2d_block_io)

// ===== cl_khr_subgroup_rotate (Clang 14 only; 16+ provides it) =====
#if (__clang_major__ < 16)
#if defined(cl_khr_subgroup_rotate)
#define DECL_SUB_GROUP_ROTATE(TYPE)                                             \
    TYPE __attribute__((overloadable)) sub_group_rotate(TYPE value, int delta); \
    TYPE __attribute__((overloadable)) sub_group_clustered_rotate(              \
        TYPE value, int delta, uint clustersize);

DECL_SUB_GROUP_ROTATE(char)
DECL_SUB_GROUP_ROTATE(uchar)
DECL_SUB_GROUP_ROTATE(short)
DECL_SUB_GROUP_ROTATE(ushort)
DECL_SUB_GROUP_ROTATE(int)
DECL_SUB_GROUP_ROTATE(uint)
DECL_SUB_GROUP_ROTATE(long)
DECL_SUB_GROUP_ROTATE(ulong)
DECL_SUB_GROUP_ROTATE(float)
#if defined(cl_khr_fp64)
DECL_SUB_GROUP_ROTATE(double)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DECL_SUB_GROUP_ROTATE(half)
#endif // defined (cl_khr_fp16)
#endif // defined(cl_khr_subgroup_rotate)
#endif // (__clang_major__ < 16)

// ===== cl_intel_simd_operations_placeholder =====
#if defined(cl_intel_simd_operations_placeholder)
// SIMD Operations

#define intel_get_simd_size get_max_sub_group_size
#define intel_get_simd_id get_sub_group_local_id

#define intel_simd_shuffle intel_sub_group_shuffle

#define intel_simd_shuffle_down intel_sub_group_shuffle_down

#define intel_simd_shuffle_up intel_sub_group_shuffle_up

#define intel_simd_media_block_read intel_sub_group_block_read
#define intel_simd_media_block_read2 intel_sub_group_block_read2
#define intel_simd_media_block_read4 intel_sub_group_block_read4
#define intel_simd_media_block_read8 intel_sub_group_block_read8

#define intel_simd_block_read intel_sub_group_block_read
#define intel_simd_block_read2 intel_sub_group_block_read2
#define intel_simd_block_read4 intel_sub_group_block_read4
#define intel_simd_block_read8 intel_sub_group_block_read8

#define intel_simd_media_block_write intel_sub_group_block_write
#define intel_simd_media_block_write2 intel_sub_group_block_write2
#define intel_simd_media_block_write4 intel_sub_group_block_write4
#define intel_simd_media_block_write8 intel_sub_group_block_write8

#define intel_simd_block_write intel_sub_group_block_write
#define intel_simd_block_write2 intel_sub_group_block_write2
#define intel_simd_block_write4 intel_sub_group_block_write4
#define intel_simd_block_write8 intel_sub_group_block_write8

#endif

// ===== cl_intel_subgroup_half2_placeholder =====
#if defined(cl_intel_subgroup_half2_placeholder)
uint __attribute__((overloadable)) intel_sub_group_half2_add(uint a, uint b);
uint __attribute__((overloadable)) intel_sub_group_half2_sub(uint a, uint b);
uint __attribute__((overloadable)) intel_sub_group_half2_mul(uint a, uint b);
uint __attribute__((overloadable)) intel_sub_group_half2_mad(uint a, uint b, uint c);

short2 __attribute__((overloadable)) intel_sub_group_half2_isequal(uint a, uint b);
short2 __attribute__((overloadable)) intel_sub_group_half2_isnotequal(uint a, uint b);
short2 __attribute__((overloadable)) intel_sub_group_half2_isgreater(uint a, uint b);
short2 __attribute__((overloadable)) intel_sub_group_half2_isgreaterequal(uint a, uint b);
short2 __attribute__((overloadable)) intel_sub_group_half2_isless(uint a, uint b);
short2 __attribute__((overloadable)) intel_sub_group_half2_islessequal(uint a, uint b);
#endif

// ===== cl_intel_device_side_vme_enable / advanced VME =====
#ifdef cl_intel_device_side_advanced_vme_enable
#ifndef cl_intel_device_side_vme_enable
#define cl_intel_device_side_vme_enable
#endif
#endif

#ifdef cl_intel_device_side_vme_enable
// VME and VME Accelerators
//
// This is sufficient to improve the runtime's toolchain for the VME
// built-in kernel, however it is not sufficient to expose a general-purpose
// device-side VME built-in function extension.  The main issues are:
//  - Need a separate type for the VME image2d_t.  The current method
//    of overloading image2d_t and changing this to a media surface
//    decl based on the way the image is used will not work with
//    separate compilation.
//  - Similarly, need a separate type for the VME accelerator vs.
//    reusing sampler_t.
//  - The VME ref image must be at a specific binding table index
//    relative to the VME src image.  At present, there is no ability
//    to enforce this.
//  - Need to spec how a VME accelerator declaration impacts the number
//    or type of samplers or other accelerators.
uint __attribute__((overloadable)) intel_get_accelerator_mb_block_type(sampler_t a);
uint __attribute__((overloadable)) intel_get_accelerator_mb_sad_sdjust_mode(sampler_t a);
uint __attribute__((overloadable)) intel_get_accelerator_mb_sub_pixel_mode(sampler_t a);
uint __attribute__((overloadable)) intel_get_accelerator_mb_search_path_type(sampler_t a);

#ifdef __opencl_c_images
void __attribute__((overloadable)) intel_work_group_vme_mb_query(
    __local uint *dst,
    int2          srcCoord,
    int2          refCoord,
    image2d_t     srcImage,
    image2d_t     refImage,
    sampler_t     a);
#endif //__opencl_c_images

#endif

// added to fix build issue with clang separation. TODO: figure out why this did not carry over from OpenCL/Frontend/Languages in the first place
#ifdef cl_intel_device_side_advanced_vme_enable
// Advanced VME and VME Accelerators extension
uint __attribute__((overloadable))
intel_get_accelerator_mb_search_block_type(sampler_t a);

uint __attribute__((overloadable)) intel_get_accelerator_mb_skip_block_type(sampler_t a);

#ifdef __opencl_c_images
void __attribute__((overloadable)) intel_work_group_vme_mb_multi_query_8(
    __local uint *dst,
    uint          countWGRefCoords,
    uint          searchCostPenalty,
    uint2         searchCostTable,
    int2          srcCoord,
    int2          refCoord,
    image2d_t     srcImage,
    image2d_t     refImage,
    sampler_t     a);

void __attribute__((overloadable)) intel_work_group_vme_mb_multi_query_4(
    __local uint *dst,
    uint          countWGRefCoords,
    uint          searchCostPenalty,
    uint2         searchCostTable,
    int2          srcCoord,
    int2          refCoord,
    image2d_t     srcImage,
    image2d_t     refImage,
    sampler_t     a);

void __attribute__((overloadable)) intel_work_group_vme_mb_multi_check_16x16(
    __local uint *dst,
    uint          countWGSkipCoords,
    uint          computeIntra,
    uint          edgesIntra,
    int2          srcCoord,
    int           skipCoord,
    image2d_t     srcImage,
    image2d_t     refImage,
    image2d_t     edgeSrcImage,
    sampler_t     a);

void __attribute__((overloadable)) intel_work_group_vme_mb_multi_bidir_check_16x16(
    __local uint *dst,
    uint          countWGSkipCoords,
    uint          computeIntra,
    uint          edgesIntra,
    int2          srcCoord,
    uchar         bidir_weight,
    uchar         skipModes,
    int           skipCoord,
    image2d_t     srcImage,
    image2d_t     refFwdImage,
    image2d_t     refBwdImage,
    image2d_t     edgeSrcImage,
    sampler_t     vmeAccelerator);

void __attribute__((overloadable)) intel_work_group_vme_mb_multi_check_8x8(
    __local uint *dst,
    uint          countWGSkipCoords,
    uint          computeIntra,
    uint          edgesIntra,
    int2          srcCoord,
    int4          skipCoord,
    image2d_t     srcImage,
    image2d_t     refImage,
    image2d_t     edgeSrcImage,
    sampler_t     a);

void __attribute__((overloadable)) intel_work_group_vme_mb_multi_bidir_check_8x8(
    __local uint *dst,
    uint          countWGSkipCoords,
    uint          computeIntra,
    uint          edgesIntra,
    int2          srcCoord,
    uchar         bidir_weight,
    uchar         skipModes,
    int2          skipCoord,
    image2d_t     srcImage,
    image2d_t     refFwdImage,
    image2d_t     refBwdImage,
    image2d_t     edgeSrcImage,
    sampler_t     vmeAccelerator);
#endif //__opencl_c_images

#endif

// ===== cl_intel_split_work_group_barrier =====
#ifdef cl_intel_split_work_group_barrier
void __attribute__((overloadable))
intel_work_group_barrier_arrive(cl_mem_fence_flags flags);
void __attribute__((overloadable))
intel_work_group_barrier_wait(cl_mem_fence_flags flags);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void __attribute__((overloadable))
intel_work_group_barrier_arrive(cl_mem_fence_flags flags, memory_scope scope);
void __attribute__((overloadable))
intel_work_group_barrier_wait(cl_mem_fence_flags flags, memory_scope scope);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // cl_intel_split_work_group_barrier

// ===== cl_intel_rt_production =====
#ifdef cl_intel_rt_production
// ----------- Raytracing production API code -----------

// --- Opaque types ---
typedef private struct intel_ray_query_opaque_t *intel_ray_query_t;
typedef global struct intel_raytracing_acceleration_structure_opaque_t
    *intel_raytracing_acceleration_structure_t;

// --- Enum and struct definitions ---

typedef enum // intel_ray_flags_t
{
    intel_ray_flags_none = 0x00,
    intel_ray_flags_force_opaque =
        0x01, // forces geometry to be opaque (no anyhit shader invokation)
    intel_ray_flags_force_non_opaque =
        0x02, // forces geometry to be non-opqaue (invoke anyhit shader)
    intel_ray_flags_accept_first_hit_and_end_search =
        0x04, // terminates traversal on the first hit found (shadow rays)
    intel_ray_flags_skip_closest_hit_shader =
        0x08, // skip execution of the closest hit shader
    intel_ray_flags_cull_back_facing_triangles =
        0x10, // back facing triangles to not produce a hit
    intel_ray_flags_cull_front_facing_triangles =
        0x20,                               // front facing triangles do not produce a hit
    intel_ray_flags_cull_opaque     = 0x40, // opaque geometry does not produce a hit
    intel_ray_flags_cull_non_opaque = 0x80, // non-opaque geometry does not produce a hit
    intel_ray_flags_skip_triangles = 0x100, // treat all triangle intersections as misses.
    intel_ray_flags_skip_procedural_primitives =
        0x200, // skip execution of intersection shaders
} intel_ray_flags_t;

typedef enum intel_hit_type_t
{
    intel_hit_type_committed_hit = 0,
    intel_hit_type_potential_hit = 1,
} intel_hit_type_t;

typedef struct // intel_float2
{
    float x, y;
} intel_float2;

typedef struct // intel_float3
{
    float x, y, z;
} intel_float3;

typedef struct // intel_float4x3
{
    intel_float3 vx, vy, vz, p;
} intel_float4x3;

typedef struct // intel_ray_desc_t
{
    intel_float3      origin;
    intel_float3      direction;
    float             tmin;
    float             tmax;
    uint              mask;
    intel_ray_flags_t flags;
} intel_ray_desc_t;

// if traversal returns one can test if a triangle or procedural is hit
typedef enum // intel_candidate_type_t
{
    intel_candidate_type_triangle,
    intel_candidate_type_procedural
} intel_candidate_type_t;

// --- API functions ---

// initialize a ray query
intel_ray_query_t intel_ray_query_init(
    intel_ray_desc_t ray, intel_raytracing_acceleration_structure_t accel);

// setup for instance traversal using a transformed ray and bottom-level AS
void intel_ray_query_forward_ray(
    intel_ray_query_t                         query,
    intel_ray_desc_t                          ray,
    intel_raytracing_acceleration_structure_t accel);

// commit the potential hit
void intel_ray_query_commit_potential_hit(intel_ray_query_t query);

// commit the potential hit and override hit distance and UVs
void intel_ray_query_commit_potential_hit_override(
    intel_ray_query_t query, float override_hit_distance, intel_float2 override_uv);

// start traversal of a ray query
void intel_ray_query_start_traversal(intel_ray_query_t query);

// Synchronize ray_query execution. If a ray was traversed,
// this must be called prior to accessing the ray query.
void intel_ray_query_sync(intel_ray_query_t query);

// Signal that a ray query will not be used further. This is the moral
// equivalent of a delete. This function does an implicit sync.
void intel_ray_query_abandon(intel_ray_query_t query);

// read hit information during shader execution
uint  intel_get_hit_bvh_level(intel_ray_query_t query, intel_hit_type_t hit_type);
float intel_get_hit_distance(intel_ray_query_t query, intel_hit_type_t hit_type);
intel_float2
     intel_get_hit_barycentrics(intel_ray_query_t query, intel_hit_type_t hit_type);
bool intel_get_hit_front_face(intel_ray_query_t query, intel_hit_type_t hit_type);
uint intel_get_hit_geometry_id(intel_ray_query_t query, intel_hit_type_t hit_type);
uint intel_get_hit_primitive_id(intel_ray_query_t query, intel_hit_type_t hit_type);
uint intel_get_hit_triangle_primitive_id(
    intel_ray_query_t query,
    intel_hit_type_t  hit_type); // fast path for triangles
uint intel_get_hit_procedural_primitive_id(
    intel_ray_query_t query,
    intel_hit_type_t  hit_type); // fast path for procedurals
uint intel_get_hit_instance_id(intel_ray_query_t query, intel_hit_type_t hit_type);
uint intel_get_hit_instance_user_id(intel_ray_query_t query, intel_hit_type_t hit_type);
intel_float4x3
intel_get_hit_world_to_object(intel_ray_query_t query, intel_hit_type_t hit_type);
intel_float4x3
intel_get_hit_object_to_world(intel_ray_query_t query, intel_hit_type_t hit_type);

intel_candidate_type_t
intel_get_hit_candidate(intel_ray_query_t query, intel_hit_type_t hit_type);

// fetch triangle vertices for a hit
void intel_get_hit_triangle_vertices(
    intel_ray_query_t query, intel_float3 vertices_out[3], intel_hit_type_t hit_type);

// Read ray-data. This is used to read transformed rays produced by HW
// instancing pipeline during any-hit or intersection shader execution.
intel_float3      intel_get_ray_origin(intel_ray_query_t query, uint bvh_level);
intel_float3      intel_get_ray_direction(intel_ray_query_t query, uint bvh_level);
float             intel_get_ray_tmin(intel_ray_query_t query, uint bvh_level);
intel_ray_flags_t intel_get_ray_flags(intel_ray_query_t query, uint bvh_level);
int               intel_get_ray_mask(intel_ray_query_t query, uint bvh_level);

// Test whether traversal has terminated.  If false, the ray has reached
// a procedural leaf or a non-opaque triangle leaf, and requires shader
// processing.
bool intel_is_traversal_done(intel_ray_query_t query);

// if traversal is done one can test for the presence of a committed hit to
// either invoke miss or closest hit shader
bool intel_has_committed_hit(intel_ray_query_t query);
#endif // cl_intel_rt_production

// ===== cl_khr_kernel_clock =====
#if defined(cl_khr_kernel_clock)

#if defined(__opencl_c_kernel_clock_scope_device)
// Clock functions - device scope
ulong __attribute__((overloadable)) clock_read_device(void);
uint2 __attribute__((overloadable)) clock_read_hilo_device(void);
#endif // __opencl_c_kernel_clock_scope_device

#if defined(__opencl_c_kernel_clock_scope_work_group)
// Clock functions - work group scope
ulong __attribute__((overloadable)) clock_read_work_group(void);
uint2 __attribute__((overloadable)) clock_read_hilo_work_group(void);
#endif // __opencl_c_kernel_clock_scope_work_group

#if defined(__opencl_c_kernel_clock_scope_sub_group)
// Clock functions - sub group scope
ulong __attribute__((overloadable)) clock_read_sub_group(void);
uint2 __attribute__((overloadable)) clock_read_hilo_sub_group(void);
#endif // __opencl_c_kernel_clock_scope_sub_group

#endif // cl_khr_kernel_clock

#endif // _OPENCL_C_INTEL_H_
