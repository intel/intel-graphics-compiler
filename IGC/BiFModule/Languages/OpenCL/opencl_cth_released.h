/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _OPENCL_CTH_
#define _OPENCL_CTH_

#define FP_FAST_FMAF

#pragma GCC system_header //Allows for typedef redefinitions to be allowed in this file.

typedef size_t uintptr_t;
#define NULL 0

// All of our devices support cl_khr_byte_adderssable_store, so #define it
// if it isn't already #defined.
#if !defined(cl_khr_byte_addressable_store)
  #define cl_khr_byte_addressable_store
#endif

#ifdef cl_khr_3d_image_writes
#pragma OPENCL EXTENSION cl_khr_3d_image_writes : enable
#endif

// All of our devices support cl_intel_subgroups, so #define it if
// it isn't already #defined.
// TODO: Move these entry points to the PCH file?
#if !defined(cl_intel_subgroups)
  #define cl_intel_subgroups
#endif

// All of our devices support cl_intel_required_subgroup_size, so #define it if
// it isn't already #defined.
#if !defined(cl_intel_required_subgroup_size)
  #define cl_intel_required_subgroup_size
#endif

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define CLK_sRGB              0x10BF
#define CLK_sRGBA             0x10C1
#define CLK_sRGBx             0x10C0
#define CLK_sBGRA             0x10C2
#endif

#if defined(cl_khr_fp16)
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#endif

#if defined(cl_khr_fp64)
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#endif

#ifdef cl_khr_gl_msaa_sharing
#pragma OPENCL EXTENSION cl_khr_gl_msaa_sharing : enable
#endif //cl_khr_gl_msaa_sharing

#if defined(cl_khr_fp64)
#define FP_FAST_FMA
#endif

#if __OPENCL_C_VERSION__ != CL_VERSION_3_0
  #ifndef __opencl_c_images
    #define __opencl_c_images 1
  #endif
#endif
// OCL 3.0 feature macros. Define them for OCL-C 2.0
#if (__OPENCL_C_VERSION__ == CL_VERSION_2_0)

#define __opencl_c_pipes
#define __opencl_c_3d_image_writes
#define __opencl_c_atomic_order_acq_rel
#define __opencl_c_atomic_order_seq_cst
#define __opencl_c_atomic_scope_device
#define __opencl_c_atomic_scope_all_devices
#define __opencl_c_device_enqueue
#define __opencl_c_generic_address_space
#define __opencl_c_program_scope_global_variables
#define __opencl_c_read_write_images
#define __opencl_c_subgroups
#define __opencl_c_work_group_collective_functions

#endif // (__OPENCL_C_VERSION__ == CL_VERSION_2_0)

// Async copies from global to local memory, local to global memory, and prefetch

/**
* event_t async_work_group_copy (
* __global gentype *dst,
* const __local gentype *src,
* size_t num_elements,
* event_t event)
* Perform an async copy of num_elements
* gentype elements from src to dst. The async
* copy is performed by all work-items in a workgroup
* and this built-in function must therefore
* be encountered by all work-items in a workgroup
* executing the kernel with the same
* argument values; otherwise the results are
* undefined.
* Returns an event object that can be used by
* wait_group_events to wait for the async copy
* to finish. The event argument can also be used
* to associate the async_work_group_copy with
* a previous async copy allowing an event to be
* shared by multiple async copies; otherwise event
* should be zero.
* If event argument is non-zero, the event object
* supplied in event argument will be returned.
* This function does not perform any implicit
* synchronization of source data such as using a
* barrier before performing the copy.
*/
event_t __attribute__((overloadable)) async_work_group_copy(__local char *dst, const __global char *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local uchar *dst, const __global uchar *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local short *dst, const __global short *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local ushort *dst, const __global ushort *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local int *dst, const __global int *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local uint *dst, const __global uint *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local long *dst, const __global long *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local ulong *dst, const __global ulong *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local float *dst, const __global float *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local char2 *dst, const __global char2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local uchar2 *dst, const __global uchar2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local short2 *dst, const __global short2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local ushort2 *dst, const __global ushort2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local int2 *dst, const __global int2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local uint2 *dst, const __global uint2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local long2 *dst, const __global long2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local ulong2 *dst, const __global ulong2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local float2 *dst, const __global float2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local char3 *dst, const __global char3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local uchar3 *dst, const __global uchar3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local short3 *dst, const __global short3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local ushort3 *dst, const __global ushort3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local int3 *dst, const __global int3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local uint3 *dst, const __global uint3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local long3 *dst, const __global long3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local ulong3 *dst, const __global ulong3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local float3 *dst, const __global float3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local char4 *dst, const __global char4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local uchar4 *dst, const __global uchar4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local short4 *dst, const __global short4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local ushort4 *dst, const __global ushort4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local int4 *dst, const __global int4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local uint4 *dst, const __global uint4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local long4 *dst, const __global long4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local ulong4 *dst, const __global ulong4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local float4 *dst, const __global float4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local char8 *dst, const __global char8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local uchar8 *dst, const __global uchar8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local short8 *dst, const __global short8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local ushort8 *dst, const __global ushort8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local int8 *dst, const __global int8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local uint8 *dst, const __global uint8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local long8 *dst, const __global long8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local ulong8 *dst, const __global ulong8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local float8 *dst, const __global float8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local char16 *dst, const __global char16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local uchar16 *dst, const __global uchar16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local short16 *dst, const __global short16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local ushort16 *dst, const __global ushort16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local int16 *dst, const __global int16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local uint16 *dst, const __global uint16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local long16 *dst, const __global long16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local ulong16 *dst, const __global ulong16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local float16 *dst, const __global float16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global char *dst, const __local char *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global uchar *dst, const __local uchar *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global short *dst, const __local short *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global ushort *dst, const __local ushort *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global int *dst, const __local int *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global uint *dst, const __local uint *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global long *dst, const __local long *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global ulong *dst, const __local ulong *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global float *dst, const __local float *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global char2 *dst, const __local char2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global uchar2 *dst, const __local uchar2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global short2 *dst, const __local short2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global ushort2 *dst, const __local ushort2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global int2 *dst, const __local int2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global uint2 *dst, const __local uint2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global long2 *dst, const __local long2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global ulong2 *dst, const __local ulong2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global float2 *dst, const __local float2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global char3 *dst, const __local char3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global uchar3 *dst, const __local uchar3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global short3 *dst, const __local short3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global ushort3 *dst, const __local ushort3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global int3 *dst, const __local int3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global uint3 *dst, const __local uint3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global long3 *dst, const __local long3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global ulong3 *dst, const __local ulong3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global float3 *dst, const __local float3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global char4 *dst, const __local char4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global uchar4 *dst, const __local uchar4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global short4 *dst, const __local short4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global ushort4 *dst, const __local ushort4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global int4 *dst, const __local int4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global uint4 *dst, const __local uint4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global long4 *dst, const __local long4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global ulong4 *dst, const __local ulong4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global float4 *dst, const __local float4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global char8 *dst, const __local char8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global uchar8 *dst, const __local uchar8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global short8 *dst, const __local short8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global ushort8 *dst, const __local ushort8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global int8 *dst, const __local int8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global uint8 *dst, const __local uint8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global long8 *dst, const __local long8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global ulong8 *dst, const __local ulong8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global float8 *dst, const __local float8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global char16 *dst, const __local char16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global uchar16 *dst, const __local uchar16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global short16 *dst, const __local short16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global ushort16 *dst, const __local ushort16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global int16 *dst, const __local int16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global uint16 *dst, const __local uint16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global long16 *dst, const __local long16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global ulong16 *dst, const __local ulong16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global float16 *dst, const __local float16 *src, size_t num_elements, event_t event);


/**
* Perform an async gather of num_elements
* gentype elements from src to dst. The
* src_stride is the stride in elements for each
* gentype element read from src. The dst_stride
* is the stride in elements for each gentype
* element written to dst. The async gather is
* performed by all work-items in a work-group.
* This built-in function must therefore be
* encountered by all work-items in a work-group
* executing the kernel with the same argument
* values; otherwise the results are undefined.
* Returns an event object that can be used by
* wait_group_events to wait for the async copy
* to finish. The event argument can also be used
* to associate the
* async_work_group_strided_copy with a
* previous async copy allowing an event to be
* shared by multiple async copies; otherwise event
* should be zero.
* If event argument is non-zero, the event object
* supplied in event argument will be returned.
* This function does not perform any implicit
* synchronization of source data such as using a
* barrier before performing the copy.
*/
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local char *dst, const __global char *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local uchar *dst, const __global uchar *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local short *dst, const __global short *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local ushort *dst, const __global ushort *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local int *dst, const __global int *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local uint *dst, const __global uint *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local long *dst, const __global long *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local ulong *dst, const __global ulong *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local float *dst, const __global float *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local char2 *dst, const __global char2 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local uchar2 *dst, const __global uchar2 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local short2 *dst, const __global short2 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local ushort2 *dst, const __global ushort2 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local int2 *dst, const __global int2 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local uint2 *dst, const __global uint2 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local long2 *dst, const __global long2 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local ulong2 *dst, const __global ulong2 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local float2 *dst, const __global float2 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local char3 *dst, const __global char3 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local uchar3 *dst, const __global uchar3 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local short3 *dst, const __global short3 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local ushort3 *dst, const __global ushort3 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local int3 *dst, const __global int3 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local uint3 *dst, const __global uint3 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local long3 *dst, const __global long3 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local ulong3 *dst, const __global ulong3 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local float3 *dst, const __global float3 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local char4 *dst, const __global char4 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local uchar4 *dst, const __global uchar4 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local short4 *dst, const __global short4 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local ushort4 *dst, const __global ushort4 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local int4 *dst, const __global int4 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local uint4 *dst, const __global uint4 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local long4 *dst, const __global long4 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local ulong4 *dst, const __global ulong4 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local float4 *dst, const __global float4 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local char8 *dst, const __global char8 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local uchar8 *dst, const __global uchar8 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local short8 *dst, const __global short8 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local ushort8 *dst, const __global ushort8 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local int8 *dst, const __global int8 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local uint8 *dst, const __global uint8 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local long8 *dst, const __global long8 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local ulong8 *dst, const __global ulong8 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local float8 *dst, const __global float8 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local char16 *dst, const __global char16 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local uchar16 *dst, const __global uchar16 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local short16 *dst, const __global short16 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local ushort16 *dst, const __global ushort16 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local int16 *dst, const __global int16 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local uint16 *dst, const __global uint16 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local long16 *dst, const __global long16 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local ulong16 *dst, const __global ulong16 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local float16 *dst, const __global float16 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global char *dst, const __local char *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global uchar *dst, const __local uchar *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global short *dst, const __local short *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global ushort *dst, const __local ushort *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global int *dst, const __local int *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global uint *dst, const __local uint *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global long *dst, const __local long *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global ulong *dst, const __local ulong *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global float *dst, const __local float *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global char2 *dst, const __local char2 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global uchar2 *dst, const __local uchar2 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global short2 *dst, const __local short2 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global ushort2 *dst, const __local ushort2 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global int2 *dst, const __local int2 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global uint2 *dst, const __local uint2 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global long2 *dst, const __local long2 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global ulong2 *dst, const __local ulong2 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global float2 *dst, const __local float2 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global char3 *dst, const __local char3 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global uchar3 *dst, const __local uchar3 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global short3 *dst, const __local short3 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global ushort3 *dst, const __local ushort3 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global int3 *dst, const __local int3 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global uint3 *dst, const __local uint3 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global long3 *dst, const __local long3 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global ulong3 *dst, const __local ulong3 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global float3 *dst, const __local float3 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global char4 *dst, const __local char4 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global uchar4 *dst, const __local uchar4 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global short4 *dst, const __local short4 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global ushort4 *dst, const __local ushort4 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global int4 *dst, const __local int4 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global uint4 *dst, const __local uint4 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global long4 *dst, const __local long4 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global ulong4 *dst, const __local ulong4 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global float4 *dst, const __local float4 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global char8 *dst, const __local char8 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global uchar8 *dst, const __local uchar8 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global short8 *dst, const __local short8 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global ushort8 *dst, const __local ushort8 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global int8 *dst, const __local int8 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global uint8 *dst, const __local uint8 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global long8 *dst, const __local long8 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global ulong8 *dst, const __local ulong8 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global float8 *dst, const __local float8 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global char16 *dst, const __local char16 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global uchar16 *dst, const __local uchar16 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global short16 *dst, const __local short16 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global ushort16 *dst, const __local ushort16 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global int16 *dst, const __local int16 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global uint16 *dst, const __local uint16 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global long16 *dst, const __local long16 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global ulong16 *dst, const __local ulong16 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global float16 *dst, const __local float16 *src, size_t num_elements, size_t dst_stride, event_t event);

/**
* Prefetch num_elements * sizeof(gentype)
* bytes into the global cache. The prefetch
* instruction is applied to a work-item in a workgroup
* and does not affect the functional
* behavior of the kernel.
*/
void __attribute__((overloadable)) prefetch(const __global void *p, size_t num_elements);

//
//Maximum supported size of a program scope global variable
//
#ifdef __opencl_c_program_scope_global_variables

#define CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE 0x10000

#endif // __opencl_c_program_scope_global_variables

// Pipes
#ifdef __opencl_c_pipes
#define INTEL_PIPE_RESERVE_ID_VALID_BIT (1U << 30)
#define CLK_NULL_RESERVE_ID (__builtin_astype(((void*)(~INTEL_PIPE_RESERVE_ID_VALID_BIT)), reserve_id_t))
bool __attribute__((overloadable)) is_valid_reserve_id(reserve_id_t reserve_id);
#endif // __opencl_c_pipes

// OpenCL 2.0 - Execution Model
#ifdef __opencl_c_device_enqueue
ndrange_t __attribute__((overloadable)) ndrange_1D(size_t);
ndrange_t __attribute__((overloadable)) ndrange_1D(size_t, size_t );
ndrange_t __attribute__((overloadable)) ndrange_1D(size_t, size_t, size_t );

ndrange_t __attribute__((overloadable)) ndrange_2D(const size_t[2]);
ndrange_t __attribute__((overloadable)) ndrange_2D(const size_t[2], const size_t[2]);
ndrange_t __attribute__((overloadable)) ndrange_2D(const size_t[2], const size_t[2], const size_t[2]);

ndrange_t __attribute__((overloadable)) ndrange_3D(const size_t[3]);
ndrange_t __attribute__((overloadable)) ndrange_3D(const size_t[3], const size_t[3]);
ndrange_t __attribute__((overloadable)) ndrange_3D(const size_t[3], const size_t[3], const size_t[3]);

void __attribute__((overloadable)) retain_event(clk_event_t);

void __attribute__((overloadable)) release_event(clk_event_t);

void __attribute__((overloadable)) set_user_event_status( clk_event_t e, int state );

void __attribute__((overloadable)) capture_event_profiling_info(clk_event_t, clk_profiling_info, __global void* value);

#endif // __opencl_c_device_enqueue

//
// c1x atomics definitions
//
#ifdef __opencl_c_generic_address_space

#define ATOMIC_VAR_INIT(x) (x)

#define ATOMIC_FLAG_INIT 0

// double atomics not supported -- requires extensions cl_khr_int64_base_atomics and cl_khr_int64_extended_atomics

// atomic_init()

#define atomic_init_prototype_addrspace(TYPE, ADDRSPACE) \
void __attribute__((overloadable)) atomic_init(volatile ADDRSPACE atomic_##TYPE *object, TYPE value);

#define atomic_init_prototype(TYPE) \
atomic_init_prototype_addrspace(TYPE, generic)

atomic_init_prototype(int)
atomic_init_prototype(uint)
atomic_init_prototype(float)

// atomic_work_item_fence()

void __attribute__((overloadable)) atomic_work_item_fence(cl_mem_fence_flags flags, memory_order order, memory_scope scope);

#if (__LLVM_VERSION_MAJOR__ < 14)
// atomic_fetch()

#define atomic_fetch_explicit_prototype_addrspace(KEY, TYPE, OPTYPE, ADDRSPACE) \
TYPE __attribute__((overloadable)) atomic_fetch_##KEY##_explicit(volatile ADDRSPACE atomic_##TYPE *object, OPTYPE operand, memory_order order); \
TYPE __attribute__((overloadable)) atomic_fetch_##KEY##_explicit(volatile ADDRSPACE atomic_##TYPE *object, OPTYPE operand, memory_order order, memory_scope scope);

#if defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)
#define atomic_fetch_prototype_addrspace(KEY, TYPE, OPTYPE, ADDRSPACE) \
TYPE __attribute__((overloadable)) atomic_fetch_##KEY(volatile ADDRSPACE atomic_##TYPE *object, OPTYPE operand); \
atomic_fetch_explicit_prototype_addrspace(KEY, TYPE, OPTYPE, ADDRSPACE)
#else // defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)
#define atomic_fetch_prototype_addrspace(KEY, TYPE, OPTYPE, ADDRSPACE) \
atomic_fetch_explicit_prototype_addrspace(KEY, TYPE, OPTYPE, ADDRSPACE)
#endif // defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)

#define atomic_fetch_prototype(KEY, TYPE, OPTYPE) \
atomic_fetch_prototype_addrspace(KEY, TYPE, OPTYPE, generic)

#define atomic_fetch_supported_prototype(KEY) \
atomic_fetch_prototype(KEY, int, int) \
atomic_fetch_prototype(KEY, uint, uint) \
atomic_fetch_prototype(KEY, uint, int) // the (size_t, ptrdiff_t) variety for 32-bit

atomic_fetch_supported_prototype(add)
atomic_fetch_supported_prototype(sub)
atomic_fetch_supported_prototype(or)
atomic_fetch_supported_prototype(xor)
atomic_fetch_supported_prototype(and)

#undef atomic_fetch_supported_prototype

// atomic_fetch_min/max() [min and max are not c11 functions]

int __attribute__((overloadable)) atomic_fetch_min_explicit(volatile generic atomic_int *object, int operand, memory_order order);
int __attribute__((overloadable)) atomic_fetch_min_explicit(volatile generic atomic_int *object, int operand, memory_order order, memory_scope scope);

uint __attribute__((overloadable)) atomic_fetch_min_explicit(volatile generic atomic_uint *object, uint operand, memory_order order);
uint __attribute__((overloadable)) atomic_fetch_min_explicit(volatile generic atomic_uint *object, uint operand, memory_order order, memory_scope scope);

uint __attribute__((overloadable)) atomic_fetch_min_explicit(volatile generic atomic_uint *object, int operand, memory_order order);
uint __attribute__((overloadable)) atomic_fetch_min_explicit(volatile generic atomic_uint *object, int operand, memory_order order, memory_scope scope);

int __attribute__((overloadable)) atomic_fetch_max_explicit(volatile generic atomic_int *object, int operand, memory_order order);
int __attribute__((overloadable)) atomic_fetch_max_explicit(volatile generic atomic_int *object, int operand, memory_order order, memory_scope scope);

uint __attribute__((overloadable)) atomic_fetch_max_explicit(volatile generic atomic_uint *object, uint operand, memory_order order);
uint __attribute__((overloadable)) atomic_fetch_max_explicit(volatile generic atomic_uint *object, uint operand, memory_order order, memory_scope scope);

uint __attribute__((overloadable)) atomic_fetch_max_explicit(volatile generic atomic_uint *object, int operand, memory_order order);
uint __attribute__((overloadable)) atomic_fetch_max_explicit(volatile generic atomic_uint *object, int operand, memory_order order, memory_scope scope);

#if defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)
int __attribute__((overloadable)) atomic_fetch_min(volatile generic atomic_int *object, int operand);
uint __attribute__((overloadable)) atomic_fetch_min(volatile generic atomic_uint *object, uint operand);
uint __attribute__((overloadable)) atomic_fetch_min(volatile generic atomic_uint *object, int operand);
int __attribute__((overloadable)) atomic_fetch_max(volatile generic atomic_int *object, int operand);
uint __attribute__((overloadable)) atomic_fetch_max(volatile generic atomic_uint *object, uint operand);
uint __attribute__((overloadable)) atomic_fetch_max(volatile generic atomic_uint *object, int operand);
#endif // defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)

#endif // (__LLVM_VERSION_MAJOR__ < 14)

// atomic_store()

#define atomic_store_explicit_prototype_addrspace(TYPE, ADDRSPACE) \
void __attribute__((overloadable)) atomic_store_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired, memory_order order); \
void __attribute__((overloadable)) atomic_store_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired, memory_order order, memory_scope scope);

#if defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)
#define atomic_store_prototype_addrspace(TYPE, ADDRSPACE) \
void __attribute__((overloadable)) atomic_store(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired); \
atomic_store_explicit_prototype_addrspace(TYPE, ADDRSPACE)
#else // defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)
#define atomic_store_prototype_addrspace(TYPE, ADDRSPACE) \
atomic_store_explicit_prototype_addrspace(TYPE, ADDRSPACE)
#endif // defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)

#define atomic_store_prototype(TYPE) \
atomic_store_prototype_addrspace(TYPE, generic)

atomic_store_prototype(int)
atomic_store_prototype(uint)
atomic_store_prototype(float)

// atomic_load()

#define atomic_load_explicit_prototype_addrspace(TYPE, ADDRSPACE) \
TYPE __attribute__((overloadable)) atomic_load_explicit(volatile ADDRSPACE atomic_##TYPE *object, memory_order order); \
TYPE __attribute__((overloadable)) atomic_load_explicit(volatile ADDRSPACE atomic_##TYPE *object, memory_order order, memory_scope scope);

#if defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)
#define atomic_load_prototype_addrspace(TYPE, ADDRSPACE) \
TYPE __attribute__((overloadable)) atomic_load(volatile ADDRSPACE atomic_##TYPE *object); \
atomic_load_explicit_prototype_addrspace(TYPE, ADDRSPACE)
#else // defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)
#define atomic_load_prototype_addrspace(TYPE, ADDRSPACE) \
atomic_load_explicit_prototype_addrspace(TYPE, ADDRSPACE)
#endif // defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)

#define atomic_load_prototype(TYPE) \
atomic_load_prototype_addrspace(TYPE, generic)

atomic_load_prototype(int)
atomic_load_prototype(uint)
atomic_load_prototype(float)

// atomic_exchange()

#define atomic_exchange_explicit_prototype_addrspace(TYPE, ADDRSPACE) \
TYPE __attribute__((overloadable)) atomic_exchange_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired, memory_order order); \
TYPE __attribute__((overloadable)) atomic_exchange_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired, memory_order order, memory_scope scope);

#if defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)
#define atomic_exchange_prototype_addrspace(TYPE, ADDRSPACE) \
TYPE __attribute__((overloadable)) atomic_exchange(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired); \
atomic_exchange_explicit_prototype_addrspace(TYPE, ADDRSPACE)
#else // defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)
#define atomic_exchange_prototype_addrspace(TYPE, ADDRSPACE) \
atomic_exchange_explicit_prototype_addrspace(TYPE, ADDRSPACE)
#endif // defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)

#define atomic_exchange_prototype(TYPE) \
atomic_exchange_prototype_addrspace(TYPE, generic)

atomic_exchange_prototype(int)
atomic_exchange_prototype(uint)
atomic_exchange_prototype(float)

// atomic_compare_exchange_strong() and atomic_compare_exchange_weak()

#define atomic_compare_exchange_strength_explicit_prototype_addrspace(TYPE, ADDRSPACE, ADDRSPACE2, STRENGTH) \
bool __attribute__((overloadable)) atomic_compare_exchange_##STRENGTH##_explicit(volatile ADDRSPACE atomic_##TYPE *object, ADDRSPACE2 TYPE *expected, TYPE desired, memory_order success, memory_order failure); \
bool __attribute__((overloadable)) atomic_compare_exchange_##STRENGTH##_explicit(volatile ADDRSPACE atomic_##TYPE *object, ADDRSPACE2 TYPE *expected, TYPE desired, memory_order success, memory_order failure, memory_scope scope);

#if defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)
#define atomic_compare_exchange_strength_prototype_addrspace(TYPE, ADDRSPACE, ADDRSPACE2, STRENGTH) \
bool __attribute__((overloadable)) atomic_compare_exchange_##STRENGTH(volatile ADDRSPACE atomic_##TYPE *object, ADDRSPACE2 TYPE *expected, TYPE desired); \
atomic_compare_exchange_strength_explicit_prototype_addrspace(TYPE, ADDRSPACE, ADDRSPACE2, STRENGTH)
#else // defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)
#define atomic_compare_exchange_strength_prototype_addrspace(TYPE, ADDRSPACE, ADDRSPACE2, STRENGTH) \
atomic_compare_exchange_strength_explicit_prototype_addrspace(TYPE, ADDRSPACE, ADDRSPACE2, STRENGTH)
#endif // defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)

#define atomic_compare_exchange_strength_prototype(TYPE, STRENGTH) \
atomic_compare_exchange_strength_prototype_addrspace(TYPE, generic, generic, STRENGTH)

atomic_compare_exchange_strength_prototype(int, strong)
atomic_compare_exchange_strength_prototype(uint, strong)
atomic_compare_exchange_strength_prototype(int, weak)
atomic_compare_exchange_strength_prototype(uint, weak)
atomic_compare_exchange_strength_prototype(float, strong)
atomic_compare_exchange_strength_prototype(float, weak)

// atomic_flag_test_and_set() and atomic_flag_clear()

#define atomic_flag_explicit_prototype_addrspace(ADDRSPACE, FUNCTYPE, RET) \
RET __attribute__((overloadable)) atomic_flag_##FUNCTYPE##_explicit(volatile ADDRSPACE atomic_flag *object, memory_order order); \
RET __attribute__((overloadable)) atomic_flag_##FUNCTYPE##_explicit(volatile ADDRSPACE atomic_flag *object, memory_order order, memory_scope scope);

#if defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)
#define atomic_flag_prototype_addrspace(ADDRSPACE, FUNCTYPE, RET) \
RET __attribute__((overloadable)) atomic_flag_##FUNCTYPE(volatile ADDRSPACE atomic_flag *object); \
atomic_flag_explicit_prototype_addrspace(ADDRSPACE, FUNCTYPE, RET)
#else // defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)
#define atomic_flag_prototype_addrspace(ADDRSPACE, FUNCTYPE, RET) \
atomic_flag_explicit_prototype_addrspace(ADDRSPACE, FUNCTYPE, RET)
#endif // defined(__opencl_c_atomic_order_seq_cst) && defined(__opencl_c_atomic_scope_device)

#define atomic_flag_prototype(FUNCTYPE, RET) \
atomic_flag_prototype_addrspace(generic, FUNCTYPE, RET)

atomic_flag_prototype(test_and_set, bool)
atomic_flag_prototype(clear, void)

// undef to not leak into user code
#undef atomic_flag_prototype
#undef atomic_flag_prototype_addrspace
#undef atomic_compare_exchange_strength_prototype
#undef atomic_compare_exchange_strength_prototype_addrspace
#undef atomic_exchange_prototype
#undef atomic_exchange_prototype_addrspace
#undef atomic_load_prototype
#undef atomic_load_prototype_addrspace
#undef atomic_store_prototype
#undef atomic_store_prototype_addrspace
#undef atomic_fetch_prototype
#undef atomic_fetch_prototype_addrspace
#undef atomic_init_prototype
#undef atomic_init_prototype_addrspace

#endif // __opencl_c_generic_address_space


#if defined(cl_intel_64bit_global_atomics_placeholder)
long __attribute__((overloadable)) atomic_add(volatile __global long *p, long val);
long __attribute__((overloadable)) atomic_sub(volatile __global long *p, long val);
long __attribute__((overloadable)) atomic_xchg(volatile __global long *p, long val);
long __attribute__((overloadable)) atomic_min(volatile __global long *p, long val);
unsigned long __attribute__((overloadable)) atomic_min(volatile __global unsigned long *p, unsigned long val);
long __attribute__((overloadable)) atomic_max(volatile __global long *p, long val);
unsigned long __attribute__((overloadable)) atomic_max(volatile __global unsigned long *p, unsigned long val);
long __attribute__((overloadable)) atomic_and(volatile __global long *p, long val);
long __attribute__((overloadable)) atomic_or(volatile __global long *p, long val);
long __attribute__((overloadable)) atomic_xor(volatile __global long *p, long val);
long __attribute__((overloadable)) atomic_inc(volatile __global long *p, long val);
long __attribute__((overloadable)) atomic_dec(volatile __global long *p, long val);
long __attribute__((overloadable)) atomic_cmpxchg(volatile __global long *p, long val);

long __attribute__((overloadable)) atom_add(volatile __global long *p, long val);
long __attribute__((overloadable)) atom_sub(volatile __global long *p, long val);
long __attribute__((overloadable)) atom_xchg(volatile __global long *p, long val);
long __attribute__((overloadable)) atom_min(volatile __global long *p, long val);
unsigned long __attribute__((overloadable)) atom_min(volatile __global unsigned long *p, unsigned long val);
long __attribute__((overloadable)) atom_max(volatile __global long *p, long val);
unsigned long __attribute__((overloadable)) atom_max(volatile __global unsigned long *p, unsigned long val);
long __attribute__((overloadable)) atom_and(volatile __global long *p, long val);
long __attribute__((overloadable)) atom_or(volatile __global long *p, long val);
long __attribute__((overloadable)) atom_xor(volatile __global long *p, long val);
long __attribute__((overloadable)) atom_inc(volatile __global long *p, long val);
long __attribute__((overloadable)) atom_dec(volatile __global long *p, long val);
long __attribute__((overloadable)) atom_cmpxchg(volatile __global long *p, long val);
#endif // defined(cl_intel_64bit_global_atomics_placeholder)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void __attribute__((overloadable)) work_group_barrier(cl_mem_fence_flags flags, memory_scope scope);
void __attribute__((overloadable)) work_group_barrier(cl_mem_fence_flags flags);
#endif

#define DECL_GROUP_ADD_MIN_MAX(prefix, type) \
type __attribute__((overloadable)) prefix##_add(type x); \
type __attribute__((overloadable)) prefix##_min(type x); \
type __attribute__((overloadable)) prefix##_max(type x);

#define DECL_GROUP_REDUCE_SCAN(prefix, type) \
DECL_GROUP_ADD_MIN_MAX(prefix##_reduce, type) \
DECL_GROUP_ADD_MIN_MAX(prefix##_scan_exclusive, type) \
DECL_GROUP_ADD_MIN_MAX(prefix##_scan_inclusive, type)

// Workgroup builtins
#ifdef __opencl_c_work_group_collective_functions

int __attribute__((overloadable)) work_group_all(int predicate);
int __attribute__((overloadable)) work_group_any(int predicate);

#define WG_BROADCAST_1D_DECL(type) \
type __attribute__((overloadable)) work_group_broadcast(type a, size_t local_id);
#define WG_BROADCAST_2D_DECL(type) \
type __attribute__((overloadable)) work_group_broadcast(type a, size_t x, size_t y);
#define WG_BROADCAST_3D_DECL(type) \
type __attribute__((overloadable)) work_group_broadcast(type a, size_t x, size_t y, size_t z);

#define WG_BROADCAST_ALL_DECL(type) \
WG_BROADCAST_1D_DECL(type) \
WG_BROADCAST_2D_DECL(type) \
WG_BROADCAST_3D_DECL(type)

#ifdef cl_khr_fp16
WG_BROADCAST_ALL_DECL(half)
#endif
WG_BROADCAST_ALL_DECL(int)
WG_BROADCAST_ALL_DECL(uint)
WG_BROADCAST_ALL_DECL(long)
WG_BROADCAST_ALL_DECL(ulong)
WG_BROADCAST_ALL_DECL(float)
#if defined(cl_khr_fp64)
WG_BROADCAST_ALL_DECL(double)
#endif

#ifdef cl_khr_fp16
DECL_GROUP_REDUCE_SCAN(work_group, half)
#endif
DECL_GROUP_REDUCE_SCAN(work_group, int)
DECL_GROUP_REDUCE_SCAN(work_group, uint)
DECL_GROUP_REDUCE_SCAN(work_group, long)
DECL_GROUP_REDUCE_SCAN(work_group, ulong)
DECL_GROUP_REDUCE_SCAN(work_group, float)
#if defined(cl_khr_fp64)
DECL_GROUP_REDUCE_SCAN(work_group, double)
#endif

#endif // __opencl_c_work_group_collective_functions

/// OCL 2.0 built-ins
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
// Workitem builtins
size_t __attribute__((overloadable)) get_enqueued_local_size(uint dimindx);
size_t __attribute__((overloadable)) get_global_linear_id(void);
size_t __attribute__((overloadable)) get_local_linear_id(void);

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


#ifdef __opencl_c_generic_address_space
/**
 * Queue a memory fence to ensure correct ordering of memory
 * operations between work-items of a work-group to
 * image memory.
 */
#define CLK_IMAGE_MEM_FENCE  0x04

cl_mem_fence_flags __attribute__((overloadable)) get_fence (const void *ptr);
cl_mem_fence_flags __attribute__((overloadable)) get_fence (void *ptr);

#endif // __opencl_c_generic_address_space


// vload

#define __CLFN_DECL_F_VLOAD_SCALAR_HELPER(F, addressSpace, rtype, itype)    \
    rtype __attribute__((overloadable)) F(size_t offset, const addressSpace itype* p);
#define __CLFN_DECL_F_VLOAD_VECTOR_HELPER(F, vsize, addressSpace, rtype, itype) \
    rtype##vsize __attribute__((overloadable)) F##vsize(size_t offset, const addressSpace itype* p);

// for n = 2, 3, 4, 8, 16, but not n = 1
#define __CLFN_DECL_F_VLOAD_ALLN_HELPER(F, addressSpace, rtype, itype)      \
    __CLFN_DECL_F_VLOAD_VECTOR_HELPER(F,  2, addressSpace, rtype, itype)    \
    __CLFN_DECL_F_VLOAD_VECTOR_HELPER(F,  3, addressSpace, rtype, itype)    \
    __CLFN_DECL_F_VLOAD_VECTOR_HELPER(F,  4, addressSpace, rtype, itype)    \
    __CLFN_DECL_F_VLOAD_VECTOR_HELPER(F,  8, addressSpace, rtype, itype)    \
    __CLFN_DECL_F_VLOAD_VECTOR_HELPER(F, 16, addressSpace, rtype, itype)

// for scalar + n = 2, 3, 4, 8, 16
#define __CLFN_DECL_F_VLOAD_SCALAR_ALLN_HELPER(F, addressSpace, rtype, itype)   \
    __CLFN_DECL_F_VLOAD_SCALAR_HELPER(F, addressSpace, rtype, itype)    \
    __CLFN_DECL_F_VLOAD_VECTOR_HELPER(F,  2, addressSpace, rtype, itype)    \
    __CLFN_DECL_F_VLOAD_VECTOR_HELPER(F,  3, addressSpace, rtype, itype)    \
    __CLFN_DECL_F_VLOAD_VECTOR_HELPER(F,  4, addressSpace, rtype, itype)    \
    __CLFN_DECL_F_VLOAD_VECTOR_HELPER(F,  8, addressSpace, rtype, itype)    \
    __CLFN_DECL_F_VLOAD_VECTOR_HELPER(F, 16, addressSpace, rtype, itype)

#ifdef __opencl_c_generic_address_space
    // OpenCL 2.0 must generate declarations for the generic address space in
    // addition to the explicit address spaces.

#if 0
    // vloadn supports all n
    #define __CLFN_DECL_F_VLOADN(F, rtype, itype)   \
        __CLFN_DECL_F_VLOAD_ALLN_HELPER(F, __generic,  rtype, itype)    \
        __CLFN_DECL_F_VLOAD_ALLN_HELPER(F, __global,   rtype, itype)    \
        __CLFN_DECL_F_VLOAD_ALLN_HELPER(F, __constant, rtype, itype)    \
        __CLFN_DECL_F_VLOAD_ALLN_HELPER(F, __local,    rtype, itype)    \
        __CLFN_DECL_F_VLOAD_ALLN_HELPER(F, __private,  rtype, itype)

    // vload_half supports scalar + all n
    #define __CLFN_DECL_F_VLOADN_HALF(F, rtype, itype)  \
        __CLFN_DECL_F_VLOAD_SCALAR_ALLN_HELPER(F, __generic,  rtype, itype) \
        __CLFN_DECL_F_VLOAD_SCALAR_ALLN_HELPER(F, __global,   rtype, itype) \
        __CLFN_DECL_F_VLOAD_SCALAR_ALLN_HELPER(F, __constant, rtype, itype) \
        __CLFN_DECL_F_VLOAD_SCALAR_ALLN_HELPER(F, __local,    rtype, itype) \
        __CLFN_DECL_F_VLOAD_SCALAR_ALLN_HELPER(F, __private,  rtype, itype)
#else
    // Ideally we'd have prototypes for all address spaces, but right now this
    // produces an "ambiguous call" error when loading from a non-const explicit
    // address pointer.  This seems to be because CLANG could convert from
    // a non-const pointer to a const pointer, or from an explicit address space
    // to a const generic address space pointer.  If this ever gets fixed we
    // can add the explicit address spaces back, which may generate better code.

    // vloadn supports all n
    #define __CLFN_DECL_F_VLOADN(F, rtype, itype)   \
        __CLFN_DECL_F_VLOAD_ALLN_HELPER(F, __generic,  rtype, itype)    \
        __CLFN_DECL_F_VLOAD_ALLN_HELPER(F, __constant, rtype, itype)

    // vload_half supports scalar + all n
    #define __CLFN_DECL_F_VLOADN_HALF(F, rtype, itype)  \
        __CLFN_DECL_F_VLOAD_SCALAR_ALLN_HELPER(F, __generic,  rtype, itype) \
        __CLFN_DECL_F_VLOAD_SCALAR_ALLN_HELPER(F, __constant, rtype, itype)

#endif

#else // __opencl_c_generic_address_space

    // Pre-OpenCL 2.0 must only generate declarations for the explicit address
    // spaces.

    // vloadn supports all n
    #define __CLFN_DECL_F_VLOADN(F, rtype, itype)   \
        __CLFN_DECL_F_VLOAD_ALLN_HELPER(F, __global,   rtype, itype)    \
        __CLFN_DECL_F_VLOAD_ALLN_HELPER(F, __constant, rtype, itype)    \
        __CLFN_DECL_F_VLOAD_ALLN_HELPER(F, __local,    rtype, itype)    \
        __CLFN_DECL_F_VLOAD_ALLN_HELPER(F, __private,  rtype, itype)

    // vload_half supports scalar + all n
    #define __CLFN_DECL_F_VLOADN_HALF(F, rtype, itype)  \
        __CLFN_DECL_F_VLOAD_SCALAR_ALLN_HELPER(F, __global,   rtype, itype) \
        __CLFN_DECL_F_VLOAD_SCALAR_ALLN_HELPER(F, __constant, rtype, itype) \
        __CLFN_DECL_F_VLOAD_SCALAR_ALLN_HELPER(F, __local,    rtype, itype) \
        __CLFN_DECL_F_VLOAD_SCALAR_ALLN_HELPER(F, __private,  rtype, itype)

#endif // __opencl_c_generic_address_space

// vload gentypes are char, uchar, short, ushort, int, uint, long, ulong, float
__CLFN_DECL_F_VLOADN(vload, char, char)
__CLFN_DECL_F_VLOADN(vload, uchar, uchar)
__CLFN_DECL_F_VLOADN(vload, short, short)
__CLFN_DECL_F_VLOADN(vload, ushort, ushort)
__CLFN_DECL_F_VLOADN(vload, int, int)
__CLFN_DECL_F_VLOADN(vload, uint, uint)
__CLFN_DECL_F_VLOADN(vload, long, long)
__CLFN_DECL_F_VLOADN(vload, ulong, ulong)
__CLFN_DECL_F_VLOADN(vload, float, float)
#ifdef cl_khr_fp16
__CLFN_DECL_F_VLOADN_HALF(vload, half, half)
#endif
#if defined(cl_khr_fp64)
__CLFN_DECL_F_VLOADN(vload, double, double)
#endif

// vload_half supports half -> float
__CLFN_DECL_F_VLOADN_HALF(vload_half, float, half)
__CLFN_DECL_F_VLOADN_HALF(vloada_half, float, half)

// vstore

#define __CLFN_DECL_F_VSTORE_SCALAR_HELPER(F, addressSpace, itype, rtype)   \
    void __attribute__((overloadable)) F(itype data, size_t offset, addressSpace rtype* p);
#define __CLFN_DECL_F_VSTORE_VECTOR_HELPER(F, vsize, addressSpace, itype, rtype)    \
    void __attribute__((overloadable)) F##vsize(itype##vsize data, size_t offset, addressSpace rtype* p);
#define __CLFN_DECL_F_VSTORE_SCALAR_ROUND_HELPER(F, rmode, addressSpace, itype, rtype)  \
    void __attribute__((overloadable)) F##rmode(itype data, size_t offset, addressSpace rtype* p);
#define __CLFN_DECL_F_VSTORE_VECTOR_ROUND_HELPER(F, rmode, vsize, addressSpace, itype, rtype)   \
    void __attribute__((overloadable)) F##vsize##rmode(itype##vsize data, size_t offset, addressSpace rtype* p);

// for n = 2, 3, 4, 8, 16, but not n = 1.
#define __CLFN_DECL_F_VSTORE_ALLN_HELPER(F, addressSpace, itype, rtype) \
    __CLFN_DECL_F_VSTORE_VECTOR_HELPER(F,  2, addressSpace, itype, rtype)   \
    __CLFN_DECL_F_VSTORE_VECTOR_HELPER(F,  3, addressSpace, itype, rtype)   \
    __CLFN_DECL_F_VSTORE_VECTOR_HELPER(F,  4, addressSpace, itype, rtype)   \
    __CLFN_DECL_F_VSTORE_VECTOR_HELPER(F,  8, addressSpace, itype, rtype)   \
    __CLFN_DECL_F_VSTORE_VECTOR_HELPER(F, 16, addressSpace, itype, rtype)

// for scalar, for rounding modes = (none), _rte, _rtz, _rtp, _rtn
#define __CLFN_DECL_F_VSTORE_SCALAR_ALLROUND_HELPER(F, addressSpace, itype, rtype)  \
    __CLFN_DECL_F_VSTORE_SCALAR_HELPER(F, addressSpace, itype, rtype)   \
    __CLFN_DECL_F_VSTORE_SCALAR_ROUND_HELPER(F, _rte, addressSpace, itype, rtype)   \
    __CLFN_DECL_F_VSTORE_SCALAR_ROUND_HELPER(F, _rtz, addressSpace, itype, rtype)   \
    __CLFN_DECL_F_VSTORE_SCALAR_ROUND_HELPER(F, _rtp, addressSpace, itype, rtype)   \
    __CLFN_DECL_F_VSTORE_SCALAR_ROUND_HELPER(F, _rtn, addressSpace, itype, rtype)

// for n, rounding modes = (none), _rte, _rtz, _rtp, _rtn
#define __CLFN_DECL_F_VSTOREN_ALLROUND_HELPER(F, vsize, addressSpace, itype, rtype) \
    __CLFN_DECL_F_VSTORE_VECTOR_HELPER(F, vsize, addressSpace, itype, rtype)    \
    __CLFN_DECL_F_VSTORE_VECTOR_ROUND_HELPER(F, _rte, vsize, addressSpace, itype, rtype)    \
    __CLFN_DECL_F_VSTORE_VECTOR_ROUND_HELPER(F, _rtz, vsize, addressSpace, itype, rtype)    \
    __CLFN_DECL_F_VSTORE_VECTOR_ROUND_HELPER(F, _rtp, vsize, addressSpace, itype, rtype)    \
    __CLFN_DECL_F_VSTORE_VECTOR_ROUND_HELPER(F, _rtn, vsize, addressSpace, itype, rtype)

// for all rounding modes, for scalar + n = 2, 3, 4, 8, 16
#define __CLFN_DCL_VSTORE_SCALAR_ALLN_ALLROUND_HELPER(F, addressSpace, itype, rtype)    \
    __CLFN_DECL_F_VSTORE_SCALAR_ALLROUND_HELPER(F, addressSpace, itype, rtype)  \
    __CLFN_DECL_F_VSTOREN_ALLROUND_HELPER(F,  2, addressSpace, itype, rtype)    \
    __CLFN_DECL_F_VSTOREN_ALLROUND_HELPER(F,  3, addressSpace, itype, rtype)    \
    __CLFN_DECL_F_VSTOREN_ALLROUND_HELPER(F,  4, addressSpace, itype, rtype)    \
    __CLFN_DECL_F_VSTOREN_ALLROUND_HELPER(F,  8, addressSpace, itype, rtype)    \
    __CLFN_DECL_F_VSTOREN_ALLROUND_HELPER(F, 16, addressSpace, itype, rtype)

#ifdef __opencl_c_generic_address_space
    // OpenCL 2.0 must generate declarations for the generic address space in
    // addition to the explicit address spaces.

    // vstoren supports all n
    #define __CLFN_DECL_F_VSTOREN(F, itype, rtype)  \
        __CLFN_DECL_F_VSTORE_ALLN_HELPER(F, __generic,  itype, rtype)   \
        __CLFN_DECL_F_VSTORE_ALLN_HELPER(F, __global,   itype, rtype)   \
        __CLFN_DECL_F_VSTORE_ALLN_HELPER(F, __local,    itype, rtype)   \
        __CLFN_DECL_F_VSTORE_ALLN_HELPER(F, __private,  itype, rtype)

    // vstore_half supports all rounding modes, for scalar + all n
    #define __CLFN_DECL_F_VSTOREN_HALF(F, itype, rtype) \
        __CLFN_DCL_VSTORE_SCALAR_ALLN_ALLROUND_HELPER(F, __generic,  itype, rtype)  \
        __CLFN_DCL_VSTORE_SCALAR_ALLN_ALLROUND_HELPER(F, __global,   itype, rtype)  \
        __CLFN_DCL_VSTORE_SCALAR_ALLN_ALLROUND_HELPER(F, __local,    itype, rtype)  \
        __CLFN_DCL_VSTORE_SCALAR_ALLN_ALLROUND_HELPER(F, __private,  itype, rtype)

#else // __opencl_c_generic_address_space
    // Pre-OpenCL 2.0 must only generate declarations for the explicit address
    // spaces.

    // vstoren supports all n
    #define __CLFN_DECL_F_VSTOREN(F, itype, rtype)  \
        __CLFN_DECL_F_VSTORE_ALLN_HELPER(F, __global,   itype, rtype)   \
        __CLFN_DECL_F_VSTORE_ALLN_HELPER(F, __local,    itype, rtype)   \
        __CLFN_DECL_F_VSTORE_ALLN_HELPER(F, __private,  itype, rtype)

    // vstore_half supports all rounding modes, for scalar + all n
    #define __CLFN_DECL_F_VSTOREN_HALF(F, itype, rtype) \
        __CLFN_DCL_VSTORE_SCALAR_ALLN_ALLROUND_HELPER(F, __global,   itype, rtype)  \
        __CLFN_DCL_VSTORE_SCALAR_ALLN_ALLROUND_HELPER(F, __local,    itype, rtype)  \
        __CLFN_DCL_VSTORE_SCALAR_ALLN_ALLROUND_HELPER(F, __private,  itype, rtype)

#endif // __opencl_c_generic_address_space

// vstore gentypes are char, uchar, short, ushort, int, uint, long, ulong, float
__CLFN_DECL_F_VSTOREN(vstore, char, char)
__CLFN_DECL_F_VSTOREN(vstore, uchar, uchar)
__CLFN_DECL_F_VSTOREN(vstore, short, short)
__CLFN_DECL_F_VSTOREN(vstore, ushort, ushort)
__CLFN_DECL_F_VSTOREN(vstore, int, int)
__CLFN_DECL_F_VSTOREN(vstore, uint, uint)
__CLFN_DECL_F_VSTOREN(vstore, long, long)
__CLFN_DECL_F_VSTOREN(vstore, ulong, ulong)
__CLFN_DECL_F_VSTOREN(vstore, float, float)
#ifdef cl_khr_fp16
__CLFN_DECL_F_VSTOREN_HALF(vstore, half, half)
#endif
#if defined(cl_khr_fp64)
__CLFN_DECL_F_VSTOREN(vstore, double, double)
#endif

// vstore_half supports float -> half and optionally double -> half
__CLFN_DECL_F_VSTOREN_HALF(vstore_half, float, half)
__CLFN_DECL_F_VSTOREN_HALF(vstorea_half, float, half)
#if defined(cl_khr_fp64)
__CLFN_DECL_F_VSTOREN_HALF(vstore_half, double, half)
__CLFN_DECL_F_VSTOREN_HALF(vstorea_half, double, half)
#endif

// Printf support
#if (__OPENCL_C_VERSION__ >= CL_VERSION_1_2)
  int printf(__constant const char* st, ...);
#endif

#ifdef __opencl_c_images
#if (__OPENCL_C_VERSION__ >= CL_VERSION_1_1)
/**
 * Write color value to location specified by coordinate
 * (x, y, z) in the 3D image object specified by image.
 * Appropriate data format conversion to the specified
 * image format is done before writing the color value.
 * x & y are considered to be unnormalized coordinates
 * and must be in the range 0 ... image width - 1, and 0
 * ... image height - 1.
 * write_imagef can only be used with image objects
 * created with image_channel_data_type set to one of
 * the pre-defined packed formats or set to
 * CL_SNORM_INT8, CL_UNORM_INT8,
 * CL_SNORM_INT16, CL_UNORM_INT16,
 * CL_HALF_FLOAT or CL_FLOAT. Appropriate data
 * format conversion will be done to convert channel
 * data from a floating-point value to actual data format
 * in which the channels are stored.
 * write_imagei can only be used with image objects
 * created with image_channel_data_type set to one of
 * the following values:
 * CL_SIGNED_INT8,
 * CL_SIGNED_INT16 and
 * CL_SIGNED_INT32.
 * write_imageui can only be used with image objects
 * created with image_channel_data_type set to one of
 * the following values:
 * CL_UNSIGNED_INT8,
 * CL_UNSIGNED_INT16 and
 * CL_UNSIGNED_INT32.
 * The behavior of write_imagef, write_imagei and
 * write_imageui for image objects created with
 * image_channel_data_type values not specified in
 * the description above or with (x, y) coordinate
 * values that are not in the range (0 ... image width -
 * 1, 0 ... image height - 1), respectively, is undefined.
 */

#if defined(__opencl_c_3d_image_writes) || defined(cl_khr_3d_image_writes)
void __attribute__((overloadable)) write_imagef(write_only image3d_t image, int4 coord, float4 color);
void __attribute__((overloadable)) write_imagei(write_only image3d_t image, int4 coord, int4 color);
void __attribute__((overloadable)) write_imageui(write_only image3d_t image, int4 coord, uint4 color);
#endif // defined(__opencl_c_3d_image_writes) || defined(cl_khr_3d_image_writes)
#endif

#if (__OPENCL_C_VERSION__ >= CL_VERSION_1_2)

  //struct _image1d_t;
  //typedef struct _image1d_t* image1d_t;
  //struct _image1d_array_t;
  //typedef struct _image1d_array_t* image1d_array_t;

  //struct _image1d_buffer_t;
  //typedef struct _image1d_buffer_t* image1d_buffer_t;

  //struct _image2d_array_t;
  //typedef struct _image2d_array_t* image2d_array_t;

  //struct _image2d_array_msaa_t;
  //typedef struct _image2d_array_msaa_t* image2d_array_msaa_t;

  //struct _image2d_array_depth_t;
  //typedef struct _image2d_array_depth_t* image2d_array_depth_t;

  /**
   * Use the coordinate (x) to do an element lookup in
   * the 1D image object specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  float4 __attribute__((overloadable)) read_imagef(read_only image1d_t image, sampler_t sampler, int coord);
  float4 __attribute__((overloadable)) read_imagef(read_only image1d_t image, sampler_t sampler, float coord);

  /**
   * Use the coordinate (coord.y) to index into the
   * 1D image array object specified by image_array
   * and (coord.x) to do an element lookup in
   * the 1D image object specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  // 1D image arrays
  float4 __attribute__((overloadable)) read_imagef(read_only image1d_array_t image_array, sampler_t sampler, int2 coord);
  float4 __attribute__((overloadable)) read_imagef(read_only image1d_array_t image_array, sampler_t sampler, float2 coord);

/**
   * Use the coordinate (coord.z) to index into the
   * 2D image array object specified by image_array
   * and (coord.x, coord.y) to do an element lookup in
   * the 2D image object specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  // 2D image arrays
  float4 __attribute__((overloadable)) read_imagef(read_only image2d_array_t image_array, sampler_t sampler, int4 coord);
  float4 __attribute__((overloadable)) read_imagef(read_only image2d_array_t image_array, sampler_t sampler, float4 coord);


  /**
   * Use the coordinate (x) to do an element lookup in
   * the 1D image object specified by image.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  int4 __attribute__((overloadable)) read_imagei(read_only image1d_t image, sampler_t sampler, int coord);
  int4 __attribute__((overloadable)) read_imagei(read_only image1d_t image, sampler_t sampler, float coord);
  uint4 __attribute__((overloadable)) read_imageui(read_only image1d_t image, sampler_t sampler, int coord);
  uint4 __attribute__((overloadable)) read_imageui(read_only image1d_t image, sampler_t sampler, float coord);

  /**
   * Use the coordinate (coord.y) to index into the
   * 1D image array object and (coord.x) to do an
   * element lookup in the 1D image object specified.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  //1D image arrays
  int4 __attribute__((overloadable)) read_imagei(read_only image1d_array_t image_array, sampler_t sampler, int2 coord);
  int4 __attribute__((overloadable)) read_imagei(read_only image1d_array_t image_array, sampler_t sampler, float2 coord);
  uint4 __attribute__((overloadable)) read_imageui(read_only image1d_array_t image_array, sampler_t sampler, int2 coord);
  uint4 __attribute__((overloadable)) read_imageui(read_only image1d_array_t image_array, sampler_t sampler, float2 coord);

  /**
   * Use the coordinate (coord.z) to index into the
   * 2D image array object and (coord.x, coord.y) to do an
   * element lookup in the 2D image object specified.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  //2D image arrays
  int4 __attribute__((overloadable)) read_imagei(read_only image2d_array_t image_array, sampler_t sampler, int4 coord);
  int4 __attribute__((overloadable)) read_imagei(read_only image2d_array_t image_array, sampler_t sampler, float4 coord);
  uint4 __attribute__((overloadable)) read_imageui(read_only image2d_array_t image_array, sampler_t sampler, int4 coord);
  uint4 __attribute__((overloadable)) read_imageui(read_only image2d_array_t image_array, sampler_t sampler, float4 coord);

  /**
   * Write color value to location specified by coordinate
   * (x) in the 1D image object specified by image.
   * Appropriate data format conversion to the specified
   * image format is done before writing the color value.
   * x is considered to be unnormalized coordinates
   * and must be in the range 0 ... image width - 1.
   * write_imagef can only be used with image objects
   * created with image_channel_data_type set to one of
   * the pre-defined packed formats or set to
   * CL_SNORM_INT8, CL_UNORM_INT8,
   * CL_SNORM_INT16, CL_UNORM_INT16,
   * CL_HALF_FLOAT or CL_FLOAT. Appropriate data
   * format conversion will be done to convert channel
   * data from a floating-point value to actual data format
   * in which the channels are stored.
   * write_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * write_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * The behavior of write_imagef, write_imagei and
   * write_imageui for image objects created with
   * image_channel_data_type values not specified in
   * the description above or with (x) coordinate
   * values that are not in the range (0 ... image width -
   * 1), respectively, is undefined.
   */
  void __attribute__((overloadable)) write_imagef(write_only image1d_t image, int coord, float4 color);
  void __attribute__((overloadable)) write_imagei(write_only image1d_t image, int coord, int4 color);
  void __attribute__((overloadable)) write_imageui(write_only image1d_t image, int coord, uint4 color);

  /**
   * Write color value to location specified by coordinate
   * (x) in the 1D image buffer object specified by image
   * buffer. Appropriate data format conversion to the
   * specified image buffer format is done before writing
   * the color value.x is considered to be unnormalized
   * coordinates and must be in the range 0 ... image width - 1.
   * write_imagef can only be used with image buffer objects
   * created with image_channel_data_type set to one of
   * the pre-defined packed formats or set to
   * CL_SNORM_INT8, CL_UNORM_INT8,
   * CL_SNORM_INT16, CL_UNORM_INT16,
   * CL_HALF_FLOAT or CL_FLOAT. Appropriate data
   * format conversion will be done to convert channel
   * data from a floating-point value to actual data format
   * in which the channels are stored.
   * write_imagei can only be used with image buffer objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * write_imageui can only be used with image buffer objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * The behavior of write_imagef, write_imagei and
   * write_imageui for image buffer objects created with
   * image_channel_data_type values not specified in
   * the description above or with (x) coordinate
   * values that are not in the range (0 ... image width -
   * 1), respectively, is undefined.
   */
  void __attribute__((overloadable)) write_imagef(write_only image1d_buffer_t image, int coord, float4 color);
  void __attribute__((overloadable)) write_imagei(write_only image1d_buffer_t image, int coord, int4 color);
  void __attribute__((overloadable)) write_imageui(write_only image1d_buffer_t image, int coord, uint4 color);

  /**
   * Write color value to location specified by coordinate
   * (coord.x) in the 1D image object specified by index
   * (coord.y) of the 1D image array object image_array.
   * Appropriate data format conversion to the specified
   * image format is done before writing the color value.
   * x is considered to be unnormalized coordinates
   * and must be in the range 0 ... image width - 1.
   * write_imagef can only be used with image objects
   * created with image_channel_data_type set to one of
   * the pre-defined packed formats or set to
   * CL_SNORM_INT8, CL_UNORM_INT8,
   * CL_SNORM_INT16, CL_UNORM_INT16,
   * CL_HALF_FLOAT or CL_FLOAT. Appropriate data
   * format conversion will be done to convert channel
   * data from a floating-point value to actual data format
   * in which the channels are stored.
   * write_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * write_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * The behavior of write_imagef, write_imagei and
   * write_imageui for image objects created with
   * image_channel_data_type values not specified in
   * the description above or with (x) coordinate
   * values that are not in the range (0 ... image width -
   * 1), respectively, is undefined.
   */

  //1D image arrays
  void __attribute__((overloadable)) write_imagef(write_only image1d_array_t image_array, int2 coord, float4 color);
  void __attribute__((overloadable)) write_imagei(write_only image1d_array_t image_array, int2 coord, int4 color);
  void __attribute__((overloadable)) write_imageui(write_only image1d_array_t image_array, int2 coord, uint4 color);

  /**
   * Write color value to location specified by coordinate
   * (coord.x, coord.y) in the 2D image object specified by index
   * (coord.z) of the 2D image array object image_array.
   * Appropriate data format conversion to the specified
   * image format is done before writing the color value.
   * (coord.x, coord.y) are considered to be unnormalized
   * coordinates and must be in the range 0 ... image width
   * - 1. write_imagef can only be used with image objects
   * created with image_channel_data_type set to one of
   * the pre-defined packed formats or set to
   * CL_SNORM_INT8, CL_UNORM_INT8,
   * CL_SNORM_INT16, CL_UNORM_INT16,
   * CL_HALF_FLOAT or CL_FLOAT. Appropriate data
   * format conversion will be done to convert channel
   * data from a floating-point value to actual data format
   * in which the channels are stored.
   * write_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * write_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * The behavior of write_imagef, write_imagei and
   * write_imageui for image objects created with
   * image_channel_data_type values not specified in
   * the description above or with (x) coordinate
   * values that are not in the range (0 ... image width -
   * 1), respectively, is undefined.
   */

  //2D image arrays
  void __attribute__((overloadable)) write_imagef(write_only image2d_array_t image_array, int4 coord, float4 color);
  void __attribute__((overloadable)) write_imagei(write_only image2d_array_t image_array, int4 coord, int4 color);
  void __attribute__((overloadable)) write_imageui(write_only image2d_array_t image_array, int4 coord, uint4 color);

  /**
   * Use coord.xy and sample to do an element
   * lookup in the 2D multi-sample image layer
   * identified by index coord.z in the 2D multi-sample
   * image array specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  // 2D multisample image arrays

#ifdef cl_khr_gl_msaa_sharing
  float4 __attribute__((overloadable)) read_imagef(read_only image2d_array_msaa_t image, int4 coord, int sample);
  int4 __attribute__((overloadable)) read_imagei(read_only image2d_array_msaa_t image, int4 coord, int sample);
  uint4 __attribute__((overloadable)) read_imageui(read_only image2d_array_msaa_t image, int4 coord, int sample);
#endif
  /**
   * Use coord.xy and sample to do an element
   * lookup in the 2D multi-sample image layer
   * identified by index coord.z in the 2D multi-sample
   * image array specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */
#ifdef cl_khr_gl_msaa_sharing
  float __attribute__((overloadable)) read_imagef(read_only image2d_array_msaa_depth_t image, int4 coord, int sample);
#endif
  /**
   * Use coord.xy to do an element lookup in the
   * 2D depth image identified by index coord.z in the 2D
   * image array specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  // 2D depth image arrays
  float __attribute__((overloadable)) read_imagef(read_only image2d_array_depth_t image, sampler_t sampler, float4 coord);
  float __attribute__((overloadable)) read_imagef(read_only image2d_array_depth_t image, sampler_t sampler, int4 coord);
  float __attribute__((overloadable)) read_imagef(read_only image2d_array_depth_t image, int4 coord);

  /**
   * Write color value to location specified by coordinate
   * (coord.x, coord.y) in the 2D image object specified by
   * image.
   * Appropriate data format conversion to the specified
   * image format is done before writing the color value.
   * (coord.x, coord.y) are considered to be unnormalized
   * coordinates and must be in the range 0 ... image width
   * - 1. write_imagef can only be used with image objects
   * created with image_channel_data_type set to one of
   * the pre-defined packed formats or set to
   * CL_SNORM_INT8, CL_UNORM_INT8,
   * CL_SNORM_INT16, CL_UNORM_INT16,
   * CL_HALF_FLOAT or CL_FLOAT. Appropriate data
   * format conversion will be done to convert channel
   * data from a floating-point value to actual data format
   * in which the channels are stored.
   * write_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * write_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * The behavior of write_imagef, write_imagei and
   * write_imageui for image objects created with
   * image_channel_data_type values not specified in
   * the description above or with (x) coordinate
   * values that are not in the range (0 ... image width -
   * 1), respectively, is undefined.
   */
  void __attribute__((overloadable)) write_imagef(write_only image2d_depth_t image, int2 coord, float color);
  void __attribute__((overloadable)) write_imagef(write_only image2d_array_depth_t image, int4 coord, float color);

  /**
   * Return the number of samples associated with image
   */

#ifdef cl_khr_gl_msaa_sharing
  int __attribute__((overloadable)) get_image_num_samples(image2d_array_msaa_t image);
#endif
  /**
   * Return the image width.
   */

  int __attribute__((overloadable)) get_image_width(image1d_t image);
  int __attribute__((overloadable)) get_image_width(image1d_buffer_t image);
  int __attribute__((overloadable)) get_image_width(image1d_array_t image);
  int __attribute__((overloadable)) get_image_width(image2d_array_t image);
  int __attribute__((overloadable)) get_image_width(image2d_array_depth_t image);
#ifdef cl_khr_gl_msaa_sharing
  int __attribute__((overloadable)) get_image_width(image2d_array_msaa_t image);
  int __attribute__((overloadable)) get_image_width(image2d_array_msaa_depth_t image);
#endif
  /**
   * Return the image height.
   */

  int __attribute__((overloadable)) get_image_height(image2d_array_t image);
  int __attribute__((overloadable)) get_image_height(image2d_array_depth_t image);
#ifdef cl_khr_gl_msaa_sharing
  int __attribute__((overloadable)) get_image_height(image2d_array_msaa_t image);
  int __attribute__((overloadable)) get_image_height(image2d_array_msaa_depth_t image);
#endif
  /**
   * Return the image array size.
   */

  size_t __attribute__((overloadable)) get_image_array_size(image1d_array_t image_array);
  size_t __attribute__((overloadable)) get_image_array_size(image2d_array_t image_array);
  size_t __attribute__((overloadable)) get_image_array_size(image2d_array_depth_t image_array);
#ifdef cl_khr_gl_msaa_sharing
  size_t __attribute__((overloadable)) get_image_array_size(image2d_array_msaa_t image_array);
  size_t __attribute__((overloadable)) get_image_array_size(image2d_array_msaa_depth_t image_array);
#endif
  /**
   * Return the channel data type. Valid values are:
   * CLK_SNORM_INT8
   * CLK_SNORM_INT16
   * CLK_UNORM_INT8
   * CLK_UNORM_INT16
   * CLK_UNORM_SHORT_565
   * CLK_UNORM_SHORT_555
   * CLK_UNORM_SHORT_101010
   * CLK_SIGNED_INT8
   * CLK_SIGNED_INT16
   * CLK_SIGNED_INT32
   * CLK_UNSIGNED_INT8
   * CLK_UNSIGNED_INT16
   * CLK_UNSIGNED_INT32
   * CLK_HALF_FLOAT
   * CLK_FLOAT
   */

  int __attribute__((overloadable)) get_image_channel_data_type(image1d_t image);
  int __attribute__((overloadable)) get_image_channel_data_type(image1d_buffer_t image);
  int __attribute__((overloadable)) get_image_channel_data_type(image1d_array_t image);
  int __attribute__((overloadable)) get_image_channel_data_type(image2d_array_t image);
  int __attribute__((overloadable)) get_image_channel_data_type(image2d_array_depth_t image);
#ifdef cl_khr_gl_msaa_sharing
  int __attribute__((overloadable)) get_image_channel_data_type(image2d_array_msaa_t image);
  int __attribute__((overloadable)) get_image_channel_data_type(image2d_array_msaa_depth_t image);
#endif

  /**
   * Return the image channel order. Valid values are:
   * CLK_A
   * CLK_R
   * CLK_Rx
   * CLK_RG
   * CLK_RGx
   * CLK_RA
   * CLK_RGB
   * CLK_RGBx
   * CLK_RGBA
   * CLK_ARGB
   * CLK_BGRA
   * CLK_INTENSITY
   * CLK_LUMINANCE
   * CLK_sRGB
   * CLK_sRGBA
   * CLK_sRGBx
   * CLK_sBGRA
   */
  int __attribute__((overloadable)) get_image_channel_order(image1d_t image);
  int __attribute__((overloadable)) get_image_channel_order(image1d_buffer_t image);
  int __attribute__((overloadable)) get_image_channel_order(image1d_array_t image);
  int __attribute__((overloadable)) get_image_channel_order(image2d_array_t image);
  int __attribute__((overloadable)) get_image_channel_order(image2d_array_depth_t image);
#ifdef cl_khr_gl_msaa_sharing
  int __attribute__((overloadable)) get_image_channel_order(image2d_array_msaa_t image);
  int __attribute__((overloadable)) get_image_channel_order(image2d_array_msaa_depth_t image);
#endif

  /**
   * Return the 2D image width and height as an int2
   * type. The width is returned in the x component, and
   * the height in the y component.
   */
  int2 __attribute__((overloadable)) get_image_dim(image2d_array_t image);
  int2 __attribute__((overloadable)) get_image_dim(image2d_array_depth_t image);
#ifdef cl_khr_gl_msaa_sharing
  int2 __attribute__((overloadable)) get_image_dim(image2d_array_msaa_t image);
  int2 __attribute__((overloadable)) get_image_dim(image2d_array_msaa_depth_t image);
#endif

  /**
  * Sampler-less Image Access
  */

  float4 __attribute__((overloadable)) read_imagef(read_only image1d_t image, int coord);
  int4 __attribute__((overloadable)) read_imagei(read_only image1d_t image, int coord);
  uint4 __attribute__((overloadable)) read_imageui(read_only image1d_t image, int coord);

  float4 __attribute__((overloadable)) read_imagef(read_only image1d_buffer_t image, int coord);
  int4 __attribute__((overloadable)) read_imagei(read_only image1d_buffer_t image, int coord);
  uint4 __attribute__((overloadable)) read_imageui(read_only image1d_buffer_t image, int coord);

  float4 __attribute__((overloadable)) read_imagef(read_only image1d_array_t image, int2 coord);
  int4 __attribute__((overloadable)) read_imagei(read_only image1d_array_t image, int2 coord);
  uint4 __attribute__((overloadable)) read_imageui(read_only image1d_array_t image, int2 coord);

  float4 __attribute__((overloadable)) read_imagef(read_only image2d_t image, int2 coord);
  int4 __attribute__((overloadable)) read_imagei(read_only image2d_t image, int2 coord);
  uint4 __attribute__((overloadable)) read_imageui(read_only image2d_t image, int2 coord);

  float4 __attribute__((overloadable)) read_imagef(read_only image2d_array_t image, int4 coord);
  int4 __attribute__((overloadable)) read_imagei(read_only image2d_array_t image, int4 coord);
  uint4 __attribute__((overloadable)) read_imageui(read_only image2d_array_t image, int4 coord);

  float4 __attribute__((overloadable)) read_imagef(read_only image3d_t image, int4 coord);
  int4 __attribute__((overloadable)) read_imagei(read_only image3d_t image, int4 coord);
  uint4 __attribute__((overloadable)) read_imageui(read_only image3d_t image, int4 coord);

#endif
#endif //__opencl_c_images

#ifndef __RENDERSCRIPTBUILD__
  // work-item functions
  /**
  * Returns the number of global work-items specified for
  * dimension identified by dimindx. This value is given by
  * the global_work_size argument to
  * clEnqueueNDRangeKernel. Valid values of dimindx
  * are 0 to get_work_dim() - 1. For other values of
  * dimindx, get_global_size() returns 1.
  * For clEnqueueTask, this always returns 1.
  */
  size_t __attribute__((overloadable)) get_global_size(uint dimindx);

  /**
  * Returns the unique global work-item ID value for
  * dimension identified by dimindx. The global work-item
  * ID specifies the work-item ID based on the number of
  * global work-items specified to execute the kernel. Valid
  * values of dimindx are 0 to get_work_dim() - 1. For
  * other values of dimindx, get_global_id() returns 0.
  * For clEnqueueTask, this returns 0.
  */
  size_t __attribute__((overloadable)) get_global_id(uint dimindx);

  /**
  * Returns the number of local work-items specified in
  * dimension identified by dimindx. This value is given by
  * the local_work_size argument to
  * clEnqueueNDRangeKernel if local_work_size is not
  * NULL; otherwise the OpenCL implementation chooses
  * an appropriate local_work_size value which is returned
  * by this function. Valid values of dimindx are 0 to
  * get_work_dim() - 1. For other values of dimindx,
  * get_local_size() returns 1.
  * For clEnqueueTask, this always returns 1.
  */
  size_t __attribute__((overloadable)) get_local_size(uint dimindx);

  /**
  * Returns the unique local work-item ID i.e. a work-item
  * within a specific work-group for dimension identified by
  * dimindx. Valid values of dimindx are 0 to
  * get_work_dim() - 1. For other values of dimindx,
  * get_local_id() returns 0.
  * For clEnqueueTask, this returns 0.
  */
  size_t __attribute__((overloadable)) get_local_id(uint dimindx);

  /**
  * Returns the number of work-groups that will execute a
  * kernel for dimension identified by dimindx.
  * Valid values of dimindx are 0 to get_work_dim() - 1.
  * For other values of dimindx, get_num_groups () returns
  * 1.
  * For clEnqueueTask, this always returns 1.
  */
  size_t __attribute__((overloadable)) get_num_groups(uint dimindx);

  /**
  * get_group_id returns the work-group ID which is a
  * number from 0 .. get_num_groups(dimindx) - 1.
  * Valid values of dimindx are 0 to get_work_dim() - 1.
  * For other values, get_group_id() returns 0.
  * For clEnqueueTask, this returns 0.
  */
  size_t __attribute__((overloadable)) get_group_id(uint dimindx);

  /**
  * get_global_offset returns the offset values specified in
  * global_work_offset argument to
  * clEnqueueNDRangeKernel.
  * Valid values of dimindx are 0 to get_work_dim() - 1.
  * For other values, get_global_offset() returns 0.
  * For clEnqueueTask, this returns 0.
  */
  size_t __attribute__((overloadable)) get_global_offset(uint dimindx);
#else
  /**
  * Returns the number of global work-items specified for
  * dimension identified by dimindx. This value is given by
  * the global_work_size argument to
  * clEnqueueNDRangeKernel. Valid values of dimindx
  * are 0 to get_work_dim() - 1. For other values of
  * dimindx, get_global_size() returns 1.
  * For clEnqueueTask, this always returns 1.
  */
  uint get_global_size(uint dimindx);

  /**
  * Returns the unique global work-item ID value for
  * dimension identified by dimindx. The global work-item
  * ID specifies the work-item ID based on the number of
  * global work-items specified to execute the kernel. Valid
  * values of dimindx are 0 to get_work_dim() - 1. For
  * other values of dimindx, get_global_id() returns 0.
  * For clEnqueueTask, this returns 0.
  */
  uint get_global_id(uint dimindx);

  /**
  * Returns the number of local work-items specified in
  * dimension identified by dimindx. This value is given by
  * the local_work_size argument to
  * clEnqueueNDRangeKernel if local_work_size is not
  * NULL; otherwise the OpenCL implementation chooses
  * an appropriate local_work_size value which is returned
  * by this function. Valid values of dimindx are 0 to
  * get_work_dim() - 1. For other values of dimindx,
  * get_local_size() returns 1.
  * For clEnqueueTask, this always returns 1.
  */
  uint get_local_size(uint dimindx);

  /**
  * Returns the unique local work-item ID i.e. a work-item
  * within a specific work-group for dimension identified by
  * dimindx. Valid values of dimindx are 0 to
  * get_work_dim() - 1. For other values of dimindx,
  * get_local_id() returns 0.
  * For clEnqueueTask, this returns 0.
  */
  uint get_local_id(uint dimindx);

  /**
  * Returns the number of work-groups that will execute a
  * kernel for dimension identified by dimindx.
  * Valid values of dimindx are 0 to get_work_dim() - 1.
  * For other values of dimindx, get_num_groups () returns
  * 1.
  * For clEnqueueTask, this always returns 1.
  */
  uint get_num_groups(uint dimindx);

  /**
  * get_group_id returns the work-group ID which is a
  * number from 0 .. get_num_groups(dimindx) - 1.
  * Valid values of dimindx are 0 to get_work_dim() - 1.
  * For other values, get_group_id() returns 0.
  * For clEnqueueTask, this returns 0.
  */
  uint get_group_id(uint dimindx);

  /**
  * get_global_offset returns the offset values specified in
  * global_work_offset argument to
  * clEnqueueNDRangeKernel.
  * Valid values of dimindx are 0 to get_work_dim() - 1.
  * For other values, get_global_offset() returns 0.
  * For clEnqueueTask, this returns 0.
  */
  uint get_global_offset(uint dimindx);
#endif

#ifdef __opencl_c_images
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

  /**
   * Use the coordinate (x) to do an element lookup in
   * the mip-level specified by the Level-of-Detail (lod)
   * in the 1D image object specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  float4 __attribute__((overloadable)) read_imagef(read_only image1d_t image, sampler_t sampler, float coord, float lod);

  /**
   * Use the coordinate (x) to do an element lookup in
   * the mip-level specified by the Level-of-Detail (lod)
   * in the 1D image object specified by image.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  int4 __attribute__((overloadable)) read_imagei(read_only image1d_t image, sampler_t sampler, float coord, float lod);
  uint4 __attribute__((overloadable)) read_imageui(read_only image1d_t image, sampler_t sampler, float coord, float lod);

  /**
   * Use the coordinate (coord.y) to index into the
   * 1D image array object specified by image_array
   * and (coord.x) and mip-level specified by lod
   * to do an element lookup in the 1D image array
   * specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  // 1D image arrays
  float4 __attribute__((overloadable)) read_imagef(read_only image1d_array_t image_array, sampler_t sampler, float2 coord, float lod);

  /**
   * Use the coordinate (coord.y) to index into the
   * 1D image array object and (coord.x) and mip-level
   * specified by lod to do an element lookup in the
   * 1D image array specified by image.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  //1D image arrays

  int4 __attribute__((overloadable)) read_imagei(read_only image1d_array_t image_array, sampler_t sampler, float2 coord, float lod);
  uint4 __attribute__((overloadable)) read_imageui(read_only image1d_array_t image_array, sampler_t sampler, float2 coord, float lod);

  /**
   * Use the coordinate (x, y) to do an element lookup in
   * in the mip-level specified by lod in the
   * 2D image object specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  float4 __attribute__((overloadable)) read_imagef(read_only image2d_t image, sampler_t sampler, float2 coord, float lod);

  /**
   * Use the coordinate (x, y) to do an element lookup in
   * the mip-level specified by lod in the
   * 2D image object specified by image.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  int4 __attribute__((overloadable)) read_imagei(read_only image2d_t image, sampler_t sampler, float2 coord, float lod);
  uint4 __attribute__((overloadable)) read_imageui(read_only image2d_t image, sampler_t sampler, float2 coord, float lod);

  /**
   * Use the coordinate (cood.xy) to do an element
   * lookup in the mip-level specified by lod in
   * the 2D depth image specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */
  float __attribute__((overloadable)) read_imagef(read_only image2d_depth_t image, sampler_t sampler, float2 coord, float lod);

/**
   * Use the coordinate (coord.z) and the mip-level
   * specified by lod to index into the 2D image
   * array object specified by image_array
   * and (coord.x, coord.y) to do an element lookup in
   * the 2D image object specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  // 2D image arrays
  float4 __attribute__((overloadable)) read_imagef(read_only image2d_array_t image_array, sampler_t sampler, float4 coord, float lod);

  /**
   * Use the coordinate (coord.z) and the mip-level
   * specified by lod to index into the 2D image
   * array object specified by image_array
   * and (coord.x, coord.y) to do an element lookup in
   * the 2D image object specified by image.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  //2D image arrays
  int4 __attribute__((overloadable)) read_imagei(read_only image2d_array_t image_array, sampler_t sampler, float4 coord, float lod);
  uint4 __attribute__((overloadable)) read_imageui(read_only image2d_array_t image_array, sampler_t sampler, float4 coord, float lod);

    /**
    * Use the coordinate (coord.z) and the mip-level
   * specified by lod to index into the 2D image
   * array object specified by image_array
   * and (coord.x, coord.y) to do an element lookup in
   * the 2D image object specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  // 2D depth image arrays
  float __attribute__((overloadable)) read_imagef(read_only image2d_array_depth_t image, sampler_t sampler, float4 coord, float lod);

  /**
   * Use the coordinate (coord.x, coord.y, coord.z) to do
   * an element lookup in thein the mip-level specified by lod in the
   * 3D image object specified
   * by image. coord.w is ignored.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description are undefined.
   */

  float4 __attribute__((overloadable)) read_imagef(read_only image3d_t image, sampler_t sampler, float4 coord, float lod);

  /**
   * Use the coordinate (coord.x, coord.y, coord.z) to do
   * an element lookup in thein the mip-level specified by lod in the
   * 3D image object specified
   * by image. coord.w is ignored.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  int4 __attribute__((overloadable)) read_imagei(read_only image3d_t image, sampler_t sampler, float4 coord, float lod);
  uint4 __attribute__((overloadable)) read_imageui(read_only image3d_t image, sampler_t sampler, float4 coord, float lod);


 /**
   * Read Image support for mipmaps using gradients for
   * LOD computation
   */

 /**
   * Use gradients to compute the LOD Read Image support
   * for mipmaps using gradients for LOD computation
   * and coordinate (x) to do an element lookup in
   * the mip-level specified by the lod
   * in the 1D image object specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  float4 __attribute__((overloadable)) read_imagef(read_only image1d_t image, sampler_t sampler, float coord, float gradientX, float gradientY);

  /**
   * Use gradients to compute the LOD
   * and coordinate (x) to do an element lookup in
   * the mip-level specified by the lod
   * in the 1D image object specified by image.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  int4 __attribute__((overloadable)) read_imagei(read_only image1d_t image, sampler_t sampler, float coord, float gradientX, float gradientY);
  uint4 __attribute__((overloadable)) read_imageui(read_only image1d_t image, sampler_t sampler, float coord, float gradientX, float gradientY);

  /**
   * Use gradients to compute the LOD
   * and the coordinate (coord.y) to index into the
   * 1D image array object specified by image_array
   * and (coord.x) and mip-level specified by lod
   * to do an element lookup in the 1D image array
   * specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  // 1D image arrays
  float4 __attribute__((overloadable)) read_imagef(read_only image1d_array_t image_array, sampler_t sampler, float2 coord, float gradientX, float gradientY);

  /**
   * Use gradients to compute the LOD
   * and the coordinate (coord.y) to index into the
   * 1D image array object and (coord.x) and mip-level
   * specified by lod to do an element lookup in the
   * 1D image array specified by image.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  //1D image arrays

  int4 __attribute__((overloadable)) read_imagei(read_only image1d_array_t image_array, sampler_t sampler, float2 coord, float gradientX, float gradientY);
  uint4 __attribute__((overloadable)) read_imageui(read_only image1d_array_t image_array, sampler_t sampler, float2 coord, float gradientX, float gradientY);

  /**
   * Use gradients to compute the LOD
   * and use the coordinate (x, y) to do an element lookup in
   * in the mip-level specified by lod in the
   * 2D image object specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  float4 __attribute__((overloadable)) read_imagef(read_only image2d_t image, sampler_t sampler, float2 coord, float2 gradientX, float2 gradientY);

  /**
   * Use gradients to compute the LOD
   * and use the coordinate (x, y) to do an element lookup in
   * the mip-level specified by lod in the
   * 2D image object specified by image.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  int4 __attribute__((overloadable)) read_imagei(read_only image2d_t image, sampler_t sampler, float2 coord, float2 gradientX, float2 gradientY);
  uint4 __attribute__((overloadable)) read_imageui(read_only image2d_t image, sampler_t sampler, float2 coord, float2 gradientX, float2 gradientY);

  /**
   * Use gradients to compute the LOD
   * and use the coordinate (cood.xy) to do an element
   * lookup in the mip-level specified by lod in
   * the 2D depth image specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  float __attribute__((overloadable)) read_imagef(read_only image2d_depth_t image, sampler_t sampler, float2 coord, float2 gradientX, float2 gradientY);

 /**
   * Use gradients to compute the LOD
   * and use the coordinate (coord.z) and the mip-level
   * specified by lod to index into the 2D image
   * array object specified by image_array
   * and (coord.x, coord.y) to do an element lookup in
   * the 2D image object specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  // 2D image arrays
  float4 __attribute__((overloadable)) read_imagef(read_only image2d_array_t image_array, sampler_t sampler, float4 coord, float2 gradientX, float2 gradientY);

  /**
   * Use gradients to compute the LOD
   * and use the coordinate (coord.z) and the mip-level
   * specified by lod to index into the 2D image
   * array object specified by image_array
   * and (coord.x, coord.y) to do an element lookup in
   * the 2D image object specified by image.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  //2D image arrays
  int4 __attribute__((overloadable)) read_imagei(read_only image2d_array_t image_array, sampler_t sampler, float4 coord, float2 gradientX, float2 gradientY);
  uint4 __attribute__((overloadable)) read_imageui(read_only image2d_array_t image_array, sampler_t sampler, float4 coord, float2 gradientX, float2 gradientY);

  /**
   * Use gradients to compute the LOD
   * and use the coordinate (coord.z) and the mip-level
   * specified by lod to index into the 2D image
   * array object specified by image_array
   * and (coord.x, coord.y) to do an element lookup in
   * the 2D image object specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  // 2D depth image arrays
  float __attribute__((overloadable)) read_imagef(read_only image2d_array_depth_t image, sampler_t sampler, float4 coord, float2 gradientX, float2 gradientY);

  /**
   * Use gradients to compute the LOD
   * and use the coordinate (coord.x, coord.y, coord.z) to do
   * an element lookup in thein the mip-level specified by lod in the
   * 3D image object specified
   * by image. coord.w is ignored.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description are undefined.
   */

  float4 __attribute__((overloadable)) read_imagef(read_only image3d_t image, sampler_t sampler, float4 coord, float4 gradientX, float4 gradientY);

  /**
   * Use gradients to compute the LOD
   * and use the coordinate (coord.x, coord.y, coord.z) to do
   * an element lookup in thein the mip-level specified by lod in the
   * 3D image object specified
   * by image. coord.w is ignored.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  int4 __attribute__((overloadable)) read_imagei(read_only image3d_t image, sampler_t sampler, float4 coord, float4 gradientX, float4 gradientY);
  uint4 __attribute__((overloadable)) read_imageui(read_only image3d_t image, sampler_t sampler, float4 coord, float4 gradientX, float4 gradientY);


 /**
   * Read Image support for mipmaps using specified LOD
   */

 /**
   * Use coordinate (x) to do an element lookup in
   * the mip-level specified by the lod
   * in the 1D image object specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  float4 __attribute__((overloadable)) read_imagef(read_only image1d_t image, sampler_t sampler, float coord, float lod);

  /**
   * Use coordinate (x) to do an element lookup in
   * the mip-level specified by the lod
   * in the 1D image object specified by image.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  int4 __attribute__((overloadable)) read_imagei(read_only image1d_t image, sampler_t sampler, float coord, float lod);
  uint4 __attribute__((overloadable)) read_imageui(read_only image1d_t image, sampler_t sampler, float coord, float lod);

  /**
   * Use coordinate (coord.y) to index into the
   * 1D image array object specified by image_array
   * and (coord.x) and mip-level specified by lod
   * to do an element lookup in the 1D image array
   * specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  // 1D image arrays
  float4 __attribute__((overloadable)) read_imagef(read_only image1d_array_t image_array, sampler_t sampler, float2 coord, float lod);

  /**
   * Use the coordinate (coord.y) to index into the
   * 1D image array object and (coord.x) and mip-level
   * specified by lod to do an element lookup in the
   * 1D image array specified by image.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  //1D image arrays

  int4 __attribute__((overloadable)) read_imagei(read_only image1d_array_t image_array, sampler_t sampler, float2 coord, float lod);
  uint4 __attribute__((overloadable)) read_imageui(read_only image1d_array_t image_array, sampler_t sampler, float2 coord, float lod);

  /**
   * Use the coordinate (x, y) to do an element lookup in
   * in the mip-level specified by lod in the
   * 2D image object specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  float4 __attribute__((overloadable)) read_imagef(read_only image2d_t image, sampler_t sampler, float2 coord, float lod);

  /**
   * Use the coordinate (x, y) to do an element lookup in
   * the mip-level specified by lod in the
   * 2D image object specified by image.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  int4 __attribute__((overloadable)) read_imagei(read_only image2d_t image, sampler_t sampler, float2 coord, float lod);
  uint4 __attribute__((overloadable)) read_imageui(read_only image2d_t image, sampler_t sampler, float2 coord, float lod);

  /**
   * Use the coordinate (cood.xy) to do an element
   * lookup in the mip-level specified by lod in
   * the 2D depth image specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  float __attribute__((overloadable)) read_imagef(read_only image2d_depth_t image, sampler_t sampler, float2 coord, float lod);

 /**
   * Use the coordinate (coord.z) and the mip-level
   * specified by lod to index into the 2D image
   * array object specified by image_array
   * and (coord.x, coord.y) to do an element lookup in
   * the 2D image object specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  // 2D image arrays
  float4 __attribute__((overloadable)) read_imagef(read_only image2d_array_t image_array, sampler_t sampler, float4 coord, float lod);

  /**
   * Use the coordinate (coord.z) and the mip-level
   * specified by lod to index into the 2D image
   * array object specified by image_array
   * and (coord.x, coord.y) to do an element lookup in
   * the 2D image object specified by image.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  //2D image arrays
  int4 __attribute__((overloadable)) read_imagei(read_only image2d_array_t image_array, sampler_t sampler, float4 coord, float lod);
  uint4 __attribute__((overloadable)) read_imageui(read_only image2d_array_t image_array, sampler_t sampler, float4 coord, float lod);

  /**
   * Use gradients to compute the LOD
   * and use the coordinate (coord.z) and the mip-level
   * specified by lod to index into the 2D image
   * array object specified by image_array
   * and (coord.x, coord.y) to do an element lookup in
   * the 2D image object specified by image.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description above are undefined.
   */

  // 2D depth image arrays
  float __attribute__((overloadable)) read_imagef(read_only image2d_array_depth_t image, sampler_t sampler, float4 coord, float lod);

  /**
   * Use gradients to compute the LOD
   * and use the coordinate (coord.x, coord.y, coord.z) to do
   * an element lookup in thein the mip-level specified by lod in the
   * 3D image object specified
   * by image. coord.w is ignored.
   * read_imagef returns floating-point values in the
   * range [0.0 ... 1.0] for image objects created with
   * image_channel_data_type set to one of the predefined
   * packed formats or CL_UNORM_INT8, or
   * CL_UNORM_INT16.
   * read_imagef returns floating-point values in the
   * range [-1.0 ... 1.0] for image objects created with
   * image_channel_data_type set to CL_SNORM_INT8,
   * or CL_SNORM_INT16.
   * read_imagef returns floating-point values for image
   * objects created with image_channel_data_type set to
   * CL_HALF_FLOAT or CL_FLOAT.
   * The read_imagef calls that take integer coordinates
   * must use a sampler with filter mode set to
   * CLK_FILTER_NEAREST, normalized coordinates set
   * to CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   * Values returned by read_imagef for image objects
   * with image_channel_data_type values not specified
   * in the description are undefined.
   */

  float4 __attribute__((overloadable)) read_imagef(read_only image3d_t image, sampler_t sampler, float4 coord, float lod);

  /**
   * Use gradients to compute the LOD
   * and use the coordinate (coord.x, coord.y, coord.z) to do
   * an element lookup in thein the mip-level specified by lod in the
   * 3D image object specified
   * by image. coord.w is ignored.
   * read_imagei and read_imageui return
   * unnormalized signed integer and unsigned integer
   * values respectively. Each channel will be stored in a
   * 32-bit integer.
   * read_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imagei
   * are undefined.
   * read_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * If the image_channel_data_type is not one of the
   * above values, the values returned by read_imageui
   * are undefined.
   * The read_image{i|ui} calls support a nearest filter
   * only. The filter_mode specified in sampler
   * must be set to CLK_FILTER_NEAREST; otherwise
   * the values returned are undefined.
   * Furthermore, the read_image{i|ui} calls that take
   * integer coordinates must use a sampler with
   * normalized coordinates set to
   * CLK_NORMALIZED_COORDS_FALSE and
   * addressing mode set to
   * CLK_ADDRESS_CLAMP_TO_EDGE,
   * CLK_ADDRESS_CLAMP or CLK_ADDRESS_NONE;
   * otherwise the values returned are undefined.
   */

  int4 __attribute__((overloadable)) read_imagei(read_only image3d_t image, sampler_t sampler, float4 coord, float lod);
  uint4 __attribute__((overloadable)) read_imageui(read_only image3d_t image, sampler_t sampler, float4 coord, float lod);


  // Write Image Functions

  // 1D writes with mipmap support
  /**
   * Write color value to location specified by coordinate
   * (x) in the mip-level specified by lod 2D image
   * object specified by image.
   * Appropriate data format conversion to the specified
   * image format is done before writing the color value.
   * x & y are considered to be unnormalized coordinates
   * and must be in the range 0 ... image width
   * of mip-level specified by lod - 1, and 0
   * ... image height of mip-level specified lod - 1.
   * write_imagef can only be used with image objects
   * created with image_channel_data_type set to one of
   * the pre-defined packed formats or set to
   * CL_SNORM_INT8, CL_UNORM_INT8,
   * CL_SNORM_INT16, CL_UNORM_INT16,
   * CL_HALF_FLOAT or CL_FLOAT. Appropriate data
   * format conversion will be done to convert channel
   * data from a floating-point value to actual data format
   * in which the channels are stored.
   * write_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * write_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * The behavior of write_imagef, write_imagei and
   * write_imageui for image objects created with
   * image_channel_data_type values not specified in
   * the description above or with (x) coordinate
   * values that are not in the range (0 ... image width -
   * 1), respectively, is undefined.
   */
  void __attribute__((overloadable)) write_imagef(write_only image1d_t image, int coord, int lod, float4 color);
  void __attribute__((overloadable)) write_imagei(write_only image1d_t image, int coord, int lod, int4 color);
  void __attribute__((overloadable)) write_imageui(write_only image1d_t image, int coord, int lod, uint4 color);

  /**
   * Write color value to location specified by coord.x in
   * the 1D image identified by coord.y and mip-level
   * lod in the 1D image array specified by image.
   * Appropriate data format conversion to the specified
   * image format is done before writing the color value.
   * coord.x and coord.y are considered to be
   * unnormalized coordinates and must be in the range 0
   * ... image width of the mip-level specified by lod - 1
   * and 0  image number of layers - 1   * write_imagef can only be used with image objects
   * created with image_channel_data_type set to one of
   * the pre-defined packed formats or set to
   * CL_SNORM_INT8, CL_UNORM_INT8,
   * CL_SNORM_INT16, CL_UNORM_INT16,
   * CL_HALF_FLOAT or CL_FLOAT. Appropriate data
   * format conversion will be done to convert channel
   * data from a floating-point value to actual data format
   * in which the channels are stored.
   * write_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * write_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * The behavior of write_imagef, write_imagei and
   * write_imageui for image objects created with
   * image_channel_data_type values not specified in
   * the description above or with (x) coordinate
   * values that are not in the range (0 ... image width -
   * 1), respectively, is undefined.
   */

  //1D image arrays writes with mipmap support
  void __attribute__((overloadable)) write_imagef(write_only image1d_array_t image_array, int2 coord, int lod, float4 color);
  void __attribute__((overloadable)) write_imagei(write_only image1d_array_t image_array, int2 coord, int lod, int4 color);
  void __attribute__((overloadable)) write_imageui(write_only image1d_array_t image_array, int2 coord, int lod, uint4 color);

  // 2D writes with mipmap support
  /**
   * Write color value to location specified by coordinate
   * (x, y) in the mip-level specified by lod 2D image
   * object specified by image.
   * Appropriate data format conversion to the specified
   * image format is done before writing the color value.
   * x & y are considered to be unnormalized coordinates
   * and must be in the range 0 ... image width
   * of mip-level specified by lod - 1, and 0
   * ... image height of mip-level specified lod - 1.
   * write_imagef can only be used with image objects
   * created with image_channel_data_type set to one of
   * the pre-defined packed formats or set to
   * CL_SNORM_INT8, CL_UNORM_INT8,
   * CL_SNORM_INT16, CL_UNORM_INT16,
   * CL_HALF_FLOAT or CL_FLOAT. Appropriate data
   * format conversion will be done to convert channel
   * data from a floating-point value to actual data format
   * in which the channels are stored.
   * write_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * write_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * The behavior of write_imagef, write_imagei and
   * write_imageui for image objects created with
   * image_channel_data_type values not specified in
   * the description above or with (x, y) coordinate
   * values that are not in the range (0 ... image width -
   * 1, 0 ... image height - 1), respectively, is undefined.
   */
  void __attribute__((overloadable)) write_imagef(write_only image2d_t image, int2 coord, int lod, float4 color);
  void __attribute__((overloadable)) write_imagei(write_only image2d_t image, int2 coord, int lod, int4 color);
  void __attribute__((overloadable)) write_imageui(write_only image2d_t image, int2 coord, int lod, uint4 color);

  /**
   * Write color value to location specified by coordinate
   * (x, y) in the mip-level specified by lod 2D image
   * object specified by image.
   * Appropriate data format conversion to the specified
   * image format is done before writing the color value.
   * x & y are considered to be unnormalized coordinates
   * and must be in the range 0 ... image width
   * of mip-level specified by lod - 1, and 0
   * ... image height of mip-level specified lod - 1.
   * write_imagef can only be used with image objects
   * created with image_channel_data_type set to one of
   * the pre-defined packed formats or set to
   * CL_SNORM_INT8, CL_UNORM_INT8,
   * CL_SNORM_INT16, CL_UNORM_INT16,
   * CL_HALF_FLOAT or CL_FLOAT. Appropriate data
   * format conversion will be done to convert channel
   * data from a floating-point value to actual data format
   * in which the channels are stored.
   * write_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * write_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * The behavior of write_imagef, write_imagei and
   * write_imageui for image objects created with
   * image_channel_data_type values not specified in
   * the description above or with (x) coordinate
   * values that are not in the range (0 ... image width -
   * 1), respectively, is undefined.
   */

  //2D image arrays
  void __attribute__((overloadable)) write_imagef(write_only image2d_array_t image_array, int4 coord, int lod, float4 color);
  void __attribute__((overloadable)) write_imagei(write_only image2d_array_t image_array, int4 coord, int lod, int4 color);
  void __attribute__((overloadable)) write_imageui(write_only image2d_array_t image_array, int4 coord, int lod, uint4 color);

  /**
   * Write color value to location specified by coordinate
   * (x, y) in the mip-level specified by lod 2D image
   * object specified by image.
   * Appropriate data format conversion to the specified
   * image format is done before writing the color value.
   * x & y are considered to be unnormalized coordinates
   * and must be in the range 0 ... image width
   * of mip-level specified by lod - 1, and 0
   * ... image height of mip-level specified lod - 1.
   * write_imagef can only be used with image objects
   * created with image_channel_data_type set to one of
   * the pre-defined packed formats or set to
   * CL_SNORM_INT8, CL_UNORM_INT8,
   * CL_SNORM_INT16, CL_UNORM_INT16,
   * CL_HALF_FLOAT or CL_FLOAT. Appropriate data
   * format conversion will be done to convert channel
   * data from a floating-point value to actual data format
   * in which the channels are stored.
   * write_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * write_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * The behavior of write_imagef, write_imagei and
   * write_imageui for image objects created with
   * image_channel_data_type values not specified in
   * the description above or with (x) coordinate
   * values that are not in the range (0 ... image width -
   * 1), respectively, is undefined.
   */
  void __attribute__((overloadable)) write_imagef(write_only image2d_depth_t image, int2 coord, int lod, float color);
  void __attribute__((overloadable)) write_imagef(write_only image2d_array_depth_t image, int4 coord, int lod, float color);

  // 3D image write support with mipmaps
  /**
   * Write color value to location specified by coordinate
   * (x, y, z) in the mip-level specified by lod 3D image
   * object specified by image.
   * Appropriate data format conversion to the specified
   * image format is done before writing the color value.
   * x & y are considered to be unnormalized coordinates
   * and must be in the range 0 ... image width
   * of mip-level specified by lod - 1, and 0
   * ... image height of mip-level specified lod - 1.
   * write_imagef can only be used with image objects
   * created with image_channel_data_type set to one of
   * the pre-defined packed formats or set to
   * CL_SNORM_INT8, CL_UNORM_INT8,
   * CL_SNORM_INT16, CL_UNORM_INT16,
   * CL_HALF_FLOAT or CL_FLOAT. Appropriate data
   * format conversion will be done to convert channel
   * data from a floating-point value to actual data format
   * in which the channels are stored.
   * write_imagei can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_SIGNED_INT8,
   * CL_SIGNED_INT16 and
   * CL_SIGNED_INT32.
   * write_imageui can only be used with image objects
   * created with image_channel_data_type set to one of
   * the following values:
   * CL_UNSIGNED_INT8,
   * CL_UNSIGNED_INT16 and
   * CL_UNSIGNED_INT32.
   * The behavior of write_imagef, write_imagei and
   * write_imageui for image objects created with
   * image_channel_data_type values not specified in
   * the description above or with (x, y) coordinate
   * values that are not in the range (0 ... image width -
   * 1, 0 ... image height - 1), respectively, is undefined.
   */
#if defined(__opencl_c_3d_image_writes) || defined(cl_khr_3d_image_writes)
  void __attribute__((overloadable)) write_imagef(write_only image3d_t image, int4 coord, int lod, float4 color);
  void __attribute__((overloadable)) write_imagei(write_only image3d_t image, int4 coord, int lod, int4 color);
  void __attribute__((overloadable)) write_imageui(write_only image3d_t image, int4 coord, int lod, uint4 color);
#endif // defined(__opencl_c_3d_image_writes) || defined(cl_khr_3d_image_writes)

  /**
   * Return the image miplevels.
   */

  int __attribute__((overloadable)) get_image_num_mip_levels(image1d_t image);
  int __attribute__((overloadable)) get_image_num_mip_levels(image2d_t image);
  int __attribute__((overloadable)) get_image_num_mip_levels(image3d_t image);

  int __attribute__((overloadable)) get_image_num_mip_levels(image1d_array_t image);
  int __attribute__((overloadable)) get_image_num_mip_levels(image2d_array_t image);
  int __attribute__((overloadable)) get_image_num_mip_levels(image2d_array_depth_t image);
  int __attribute__((overloadable)) get_image_num_mip_levels(image2d_depth_t image);

#endif
#endif //__opencl_c_images

#if (__OPENCL_C_VERSION__ >= CL_VERSION_1_2)
char __attribute__((overloadable)) popcount(char x);
uchar __attribute__((overloadable)) popcount(uchar x);
char2 __attribute__((overloadable)) popcount(char2 x);
uchar2 __attribute__((overloadable)) popcount(uchar2 x);
char3 __attribute__((overloadable)) popcount(char3 x);
uchar3 __attribute__((overloadable)) popcount(uchar3 x);
char4 __attribute__((overloadable)) popcount(char4 x);
uchar4 __attribute__((overloadable)) popcount(uchar4 x);
char8 __attribute__((overloadable)) popcount(char8 x);
uchar8 __attribute__((overloadable)) popcount(uchar8 x);
char16 __attribute__((overloadable)) popcount(char16 x);
uchar16 __attribute__((overloadable)) popcount(uchar16 x);
short __attribute__((overloadable)) popcount(short x);
ushort __attribute__((overloadable)) popcount(ushort x);
short2 __attribute__((overloadable)) popcount(short2 x);
ushort2 __attribute__((overloadable)) popcount(ushort2 x);
short3 __attribute__((overloadable)) popcount(short3 x);
ushort3 __attribute__((overloadable)) popcount(ushort3 x);
short4 __attribute__((overloadable)) popcount(short4 x);
ushort4 __attribute__((overloadable)) popcount(ushort4 x);
short8 __attribute__((overloadable)) popcount(short8 x);
ushort8 __attribute__((overloadable)) popcount(ushort8 x);
short16 __attribute__((overloadable)) popcount(short16 x);
ushort16 __attribute__((overloadable)) popcount(ushort16 x);
int __attribute__((overloadable)) popcount(int x);
uint __attribute__((overloadable)) popcount(uint x);
int2 __attribute__((overloadable)) popcount(int2 x);
uint2 __attribute__((overloadable)) popcount(uint2 x);
int3 __attribute__((overloadable)) popcount(int3 x);
uint3 __attribute__((overloadable)) popcount(uint3 x);
int4 __attribute__((overloadable)) popcount(int4 x);
uint4 __attribute__((overloadable)) popcount(uint4 x);
int8 __attribute__((overloadable)) popcount(int8 x);
uint8 __attribute__((overloadable)) popcount(uint8 x);
int16 __attribute__((overloadable)) popcount(int16 x);
uint16 __attribute__((overloadable)) popcount(uint16 x);
long __attribute__((overloadable)) popcount(long x);
ulong __attribute__((overloadable)) popcount(ulong x);
long2 __attribute__((overloadable)) popcount(long2 x);
ulong2 __attribute__((overloadable)) popcount(ulong2 x);
long3 __attribute__((overloadable)) popcount(long3 x);
ulong3 __attribute__((overloadable)) popcount(ulong3 x);
long4 __attribute__((overloadable)) popcount(long4 x);
ulong4 __attribute__((overloadable)) popcount(ulong4 x);
long8 __attribute__((overloadable)) popcount(long8 x);
ulong8 __attribute__((overloadable)) popcount(ulong8 x);
long16 __attribute__((overloadable)) popcount(long16 x);
ulong16 __attribute__((overloadable)) popcount(ulong16 x);
#endif

//#if (__OPENCL_C_VERSION__ > CL_VERSION_1_2)
char __attribute__((overloadable)) ctz(char x);
uchar __attribute__((overloadable)) ctz(uchar x);
char2 __attribute__((overloadable)) ctz(char2 x);
uchar2 __attribute__((overloadable)) ctz(uchar2 x);
char3 __attribute__((overloadable)) ctz(char3 x);
uchar3 __attribute__((overloadable)) ctz(uchar3 x);
char4 __attribute__((overloadable)) ctz(char4 x);
uchar4 __attribute__((overloadable)) ctz(uchar4 x);
char8 __attribute__((overloadable)) ctz(char8 x);
uchar8 __attribute__((overloadable)) ctz(uchar8 x);
char16 __attribute__((overloadable)) ctz(char16 x);
uchar16 __attribute__((overloadable)) ctz(uchar16 x);
short __attribute__((overloadable)) ctz(short x);
ushort __attribute__((overloadable)) ctz(ushort x);
short2 __attribute__((overloadable)) ctz(short2 x);
ushort2 __attribute__((overloadable)) ctz(ushort2 x);
short3 __attribute__((overloadable)) ctz(short3 x);
ushort3 __attribute__((overloadable)) ctz(ushort3 x);
short4 __attribute__((overloadable)) ctz(short4 x);
ushort4 __attribute__((overloadable)) ctz(ushort4 x);
short8 __attribute__((overloadable)) ctz(short8 x);
ushort8 __attribute__((overloadable)) ctz(ushort8 x);
short16 __attribute__((overloadable)) ctz(short16 x);
ushort16 __attribute__((overloadable)) ctz(ushort16 x);
int __attribute__((overloadable)) ctz(int x);
uint __attribute__((overloadable)) ctz(uint x);
int2 __attribute__((overloadable)) ctz(int2 x);
uint2 __attribute__((overloadable)) ctz(uint2 x);
int3 __attribute__((overloadable)) ctz(int3 x);
uint3 __attribute__((overloadable)) ctz(uint3 x);
int4 __attribute__((overloadable)) ctz(int4 x);
uint4 __attribute__((overloadable)) ctz(uint4 x);
int8 __attribute__((overloadable)) ctz(int8 x);
uint8 __attribute__((overloadable)) ctz(uint8 x);
int16 __attribute__((overloadable)) ctz(int16 x);
uint16 __attribute__((overloadable)) ctz(uint16 x);
long __attribute__((overloadable)) ctz(long x);
ulong __attribute__((overloadable)) ctz(ulong x);
long2 __attribute__((overloadable)) ctz(long2 x);
ulong2 __attribute__((overloadable)) ctz(ulong2 x);
long3 __attribute__((overloadable)) ctz(long3 x);
ulong3 __attribute__((overloadable)) ctz(ulong3 x);
long4 __attribute__((overloadable)) ctz(long4 x);
ulong4 __attribute__((overloadable)) ctz(ulong4 x);
long8 __attribute__((overloadable)) ctz(long8 x);
ulong8 __attribute__((overloadable)) ctz(ulong8 x);
long16 __attribute__((overloadable)) ctz(long16 x);
ulong16 __attribute__((overloadable)) ctz(ulong16 x);
//#endif

////////////////////////////////////////////////////////////////////////////////////
////              cl_khr_fp16 - extension support
////
////////////////////////////////////////////////////////////////////////////////////

#ifdef cl_khr_fp16

#define HALF_DIG 3
#define HALF_MANT_DIG 11
#define HALF_MAX_10_EXP +4
#define HALF_MAX_EXP +16
#define HALF_MIN_10_EXP -4
#define HALF_MIN_EXP -13
#define HALF_RADIX 2
#define HALF_MAX ((0x1.ffcp15h))
#define HALF_MIN ((0x1.0p-14h))
#define HALF_EPSILON ((0x1.0p-10h))

#define M_E_H         2.71828182845904523536028747135266250h
#define M_LOG2E_H     1.44269504088896340735992468100189214h
#define M_LOG10E_H    0.434294481903251827651128918916605082h
#define M_LN2_H       0.693147180559945309417232121458176568h
#define M_LN10_H      2.30258509299404568401799145468436421h
#define M_PI_H        3.14159265358979323846264338327950288h
#define M_PI_2_H      1.57079632679489661923132169163975144h
#define M_PI_4_H      0.785398163397448309615660845819875721h
#define M_1_PI_H      0.318309886183790671537767526745028724h
#define M_2_PI_H      0.636619772367581343075535053490057448h
#define M_2_SQRTPI_H  1.12837916709551257389615890312154517h
#define M_SQRT2_H     1.41421356237309504880168872420969808h
#define M_SQRT1_2_H   0.707106781186547524400844362104849039h


event_t __attribute__((overloadable)) async_work_group_copy(__local half *dst, const __global half *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local half2 *dst, const __global half2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local half3 *dst, const __global half3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local half4 *dst, const __global half4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local half8 *dst, const __global half8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local half16 *dst, const __global half16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global half *dst, const __local half *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global half2 *dst, const __local half2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global half3 *dst, const __local half3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global half4 *dst, const __local half4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global half8 *dst, const __local half8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global half16 *dst, const __local half16 *src, size_t num_elements, event_t event);

event_t __attribute__((overloadable)) async_work_group_strided_copy(__local half *dst, const __global half *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local half2 *dst, const __global half2 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local half3 *dst, const __global half3 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local half4 *dst, const __global half4 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local half8 *dst, const __global half8 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local half16 *dst, const __global half16 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global half *dst, const __local half *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global half2 *dst, const __local half2 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global half3 *dst, const __local half3 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global half4 *dst, const __local half4 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global half8 *dst, const __local half8 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global half16 *dst, const __local half16 *src, size_t num_elements, size_t dst_stride, event_t event);

half __attribute__((overloadable)) native_exp(half x);
half2 __attribute__((overloadable)) native_exp(half2 x);
half3 __attribute__((overloadable)) native_exp(half3 x);
half4 __attribute__((overloadable)) native_exp(half4 x);
half8 __attribute__((overloadable)) native_exp(half8 x);
half16 __attribute__((overloadable)) native_exp(half16 x);
half __attribute__((overloadable)) native_exp10(half x);
half2 __attribute__((overloadable)) native_exp10(half2 x);
half3 __attribute__((overloadable)) native_exp10(half3 x);
half4 __attribute__((overloadable)) native_exp10(half4 x);
half8 __attribute__((overloadable)) native_exp10(half8 x);
half16 __attribute__((overloadable)) native_exp10(half16 x);
half __attribute__((overloadable)) native_exp2(half x);
half2 __attribute__((overloadable)) native_exp2(half2 x);
half3 __attribute__((overloadable)) native_exp2(half3 x);
half4 __attribute__((overloadable)) native_exp2(half4 x);
half8 __attribute__((overloadable)) native_exp2(half8 x);
half16 __attribute__((overloadable)) native_exp2(half16 x);
half __attribute__((overloadable)) native_log(half x);
half2 __attribute__((overloadable)) native_log(half2 x);
half3 __attribute__((overloadable)) native_log(half3 x);
half4 __attribute__((overloadable)) native_log(half4 x);
half8 __attribute__((overloadable)) native_log(half8 x);
half16 __attribute__((overloadable)) native_log(half16 x);
half __attribute__((overloadable)) native_log2(half x);
half2 __attribute__((overloadable)) native_log2(half2 x);
half3 __attribute__((overloadable)) native_log2(half3 x);
half4 __attribute__((overloadable)) native_log2(half4 x);
half8 __attribute__((overloadable)) native_log2(half8 x);
half16 __attribute__((overloadable)) native_log2(half16 x);
half __attribute__((overloadable)) native_powr(half x, half y);
half2 __attribute__((overloadable)) native_powr(half2 x, half2 y);
half3 __attribute__((overloadable)) native_powr(half3 x, half3 y);
half4 __attribute__((overloadable)) native_powr(half4 x, half4 y);
half8 __attribute__((overloadable)) native_powr(half8 x, half8 y);
half16 __attribute__((overloadable)) native_powr(half16 x, half16 y);
half __attribute__((overloadable)) native_recip(half x);
half2 __attribute__((overloadable)) native_recip(half2 x);
half3 __attribute__((overloadable)) native_recip(half3 x);
half4 __attribute__((overloadable)) native_recip(half4 x);
half8 __attribute__((overloadable)) native_recip(half8 x);
half16 __attribute__((overloadable)) native_recip(half16 x);
half __attribute__((overloadable)) native_rsqrt(half x);
half2 __attribute__((overloadable)) native_rsqrt(half2 x);
half3 __attribute__((overloadable)) native_rsqrt(half3 x);
half4 __attribute__((overloadable)) native_rsqrt(half4 x);
half8 __attribute__((overloadable)) native_rsqrt(half8 x);
half16 __attribute__((overloadable)) native_rsqrt(half16 x);
half __attribute__((overloadable)) native_sqrt(half x);
half2 __attribute__((overloadable)) native_sqrt(half2 x);
half3 __attribute__((overloadable)) native_sqrt(half3 x);
half4 __attribute__((overloadable)) native_sqrt(half4 x);
half8 __attribute__((overloadable)) native_sqrt(half8 x);
half16 __attribute__((overloadable)) native_sqrt(half16 x);


#endif

////////////////////////////////////////////////////////////////////////////////////
////              cl_khr_fp64 - extension support
////
////////////////////////////////////////////////////////////////////////////////////

#if defined(cl_khr_fp64)

#define DBL_DIG 15
#define DBL_MANT_DIG 53
#define DBL_MAX_10_EXP +308
#define DBL_MAX_EXP +1024
#define DBL_MIN_10_EXP -307
#define DBL_MIN_EXP -1021
#define DBL_RADIX 2
#define DBL_MAX 0x1.fffffffffffffp1023
#define DBL_MIN 0x1.0p-1022
#define DBL_EPSILON 0x1.0p-52

#define M_E           0x1.5bf0a8b145769p+1
#define M_LOG2E       0x1.71547652b82fep+0
#define M_LOG10E      0x1.bcb7b1526e50ep-2
#define M_LN2         0x1.62e42fefa39efp-1
#define M_LN10        0x1.26bb1bbb55516p+1
#define M_PI          0x1.921fb54442d18p+1
#define M_PI_2        0x1.921fb54442d18p+0
#define M_PI_4        0x1.921fb54442d18p-1
#define M_1_PI        0x1.45f306dc9c883p-2
#define M_2_PI        0x1.45f306dc9c883p-1
#define M_2_SQRTPI    0x1.20dd750429b6dp+0
#define M_SQRT2       0x1.6a09e667f3bcdp+0
#define M_SQRT1_2     0x1.6a09e667f3bcdp-1

// Conversions

char __attribute__((overloadable)) convert_char(double);
char __attribute__((overloadable)) convert_char_rte(double);
char __attribute__((overloadable)) convert_char_rtn(double);
char __attribute__((overloadable)) convert_char_rtp(double);
char __attribute__((overloadable)) convert_char_rtz(double);
char __attribute__((overloadable)) convert_char_sat(double);
char __attribute__((overloadable)) convert_char_sat_rte(double);
char __attribute__((overloadable)) convert_char_sat_rtn(double);
char __attribute__((overloadable)) convert_char_sat_rtp(double);
char __attribute__((overloadable)) convert_char_sat_rtz(double);
char2 __attribute__((overloadable)) convert_char2(double2);
char2 __attribute__((overloadable)) convert_char2_rte(double2);
char2 __attribute__((overloadable)) convert_char2_rtn(double2);
char2 __attribute__((overloadable)) convert_char2_rtp(double2);
char2 __attribute__((overloadable)) convert_char2_rtz(double2);
char2 __attribute__((overloadable)) convert_char2_sat(double2);
char2 __attribute__((overloadable)) convert_char2_sat_rte(double2);
char2 __attribute__((overloadable)) convert_char2_sat_rtn(double2);
char2 __attribute__((overloadable)) convert_char2_sat_rtp(double2);
char2 __attribute__((overloadable)) convert_char2_sat_rtz(double2);
char3 __attribute__((overloadable)) convert_char3(double3);
char3 __attribute__((overloadable)) convert_char3_rte(double3);
char3 __attribute__((overloadable)) convert_char3_rtn(double3);
char3 __attribute__((overloadable)) convert_char3_rtp(double3);
char3 __attribute__((overloadable)) convert_char3_rtz(double3);
char3 __attribute__((overloadable)) convert_char3_sat(double3);
char3 __attribute__((overloadable)) convert_char3_sat_rte(double3);
char3 __attribute__((overloadable)) convert_char3_sat_rtn(double3);
char3 __attribute__((overloadable)) convert_char3_sat_rtp(double3);
char3 __attribute__((overloadable)) convert_char3_sat_rtz(double3);
char4 __attribute__((overloadable)) convert_char4(double4);
char4 __attribute__((overloadable)) convert_char4_rte(double4);
char4 __attribute__((overloadable)) convert_char4_rtn(double4);
char4 __attribute__((overloadable)) convert_char4_rtp(double4);
char4 __attribute__((overloadable)) convert_char4_rtz(double4);
char4 __attribute__((overloadable)) convert_char4_sat(double4);
char4 __attribute__((overloadable)) convert_char4_sat_rte(double4);
char4 __attribute__((overloadable)) convert_char4_sat_rtn(double4);
char4 __attribute__((overloadable)) convert_char4_sat_rtp(double4);
char4 __attribute__((overloadable)) convert_char4_sat_rtz(double4);
char8 __attribute__((overloadable)) convert_char8(double8);
char8 __attribute__((overloadable)) convert_char8_rte(double8);
char8 __attribute__((overloadable)) convert_char8_rtn(double8);
char8 __attribute__((overloadable)) convert_char8_rtp(double8);
char8 __attribute__((overloadable)) convert_char8_rtz(double8);
char8 __attribute__((overloadable)) convert_char8_sat(double8);
char8 __attribute__((overloadable)) convert_char8_sat_rte(double8);
char8 __attribute__((overloadable)) convert_char8_sat_rtn(double8);
char8 __attribute__((overloadable)) convert_char8_sat_rtp(double8);
char8 __attribute__((overloadable)) convert_char8_sat_rtz(double8);
char16 __attribute__((overloadable)) convert_char16(double16);
char16 __attribute__((overloadable)) convert_char16_rte(double16);
char16 __attribute__((overloadable)) convert_char16_rtn(double16);
char16 __attribute__((overloadable)) convert_char16_rtp(double16);
char16 __attribute__((overloadable)) convert_char16_rtz(double16);
char16 __attribute__((overloadable)) convert_char16_sat(double16);
char16 __attribute__((overloadable)) convert_char16_sat_rte(double16);
char16 __attribute__((overloadable)) convert_char16_sat_rtn(double16);
char16 __attribute__((overloadable)) convert_char16_sat_rtp(double16);
char16 __attribute__((overloadable)) convert_char16_sat_rtz(double16);

uchar __attribute__((overloadable)) convert_uchar(double);
uchar __attribute__((overloadable)) convert_uchar_rte(double);
uchar __attribute__((overloadable)) convert_uchar_rtn(double);
uchar __attribute__((overloadable)) convert_uchar_rtp(double);
uchar __attribute__((overloadable)) convert_uchar_rtz(double);
uchar __attribute__((overloadable)) convert_uchar_sat(double);
uchar __attribute__((overloadable)) convert_uchar_sat_rte(double);
uchar __attribute__((overloadable)) convert_uchar_sat_rtn(double);
uchar __attribute__((overloadable)) convert_uchar_sat_rtp(double);
uchar __attribute__((overloadable)) convert_uchar_sat_rtz(double);
uchar2 __attribute__((overloadable)) convert_uchar2(double2);
uchar2 __attribute__((overloadable)) convert_uchar2_rte(double2);
uchar2 __attribute__((overloadable)) convert_uchar2_rtn(double2);
uchar2 __attribute__((overloadable)) convert_uchar2_rtp(double2);
uchar2 __attribute__((overloadable)) convert_uchar2_rtz(double2);
uchar2 __attribute__((overloadable)) convert_uchar2_sat(double2);
uchar2 __attribute__((overloadable)) convert_uchar2_sat_rte(double2);
uchar2 __attribute__((overloadable)) convert_uchar2_sat_rtn(double2);
uchar2 __attribute__((overloadable)) convert_uchar2_sat_rtp(double2);
uchar2 __attribute__((overloadable)) convert_uchar2_sat_rtz(double2);
uchar3 __attribute__((overloadable)) convert_uchar3(double3);
uchar3 __attribute__((overloadable)) convert_uchar3_rte(double3);
uchar3 __attribute__((overloadable)) convert_uchar3_rtn(double3);
uchar3 __attribute__((overloadable)) convert_uchar3_rtp(double3);
uchar3 __attribute__((overloadable)) convert_uchar3_rtz(double3);
uchar3 __attribute__((overloadable)) convert_uchar3_sat(double3);
uchar3 __attribute__((overloadable)) convert_uchar3_sat_rte(double3);
uchar3 __attribute__((overloadable)) convert_uchar3_sat_rtn(double3);
uchar3 __attribute__((overloadable)) convert_uchar3_sat_rtp(double3);
uchar3 __attribute__((overloadable)) convert_uchar3_sat_rtz(double3);
uchar4 __attribute__((overloadable)) convert_uchar4(double4);
uchar4 __attribute__((overloadable)) convert_uchar4_rte(double4);
uchar4 __attribute__((overloadable)) convert_uchar4_rtn(double4);
uchar4 __attribute__((overloadable)) convert_uchar4_rtp(double4);
uchar4 __attribute__((overloadable)) convert_uchar4_rtz(double4);
uchar4 __attribute__((overloadable)) convert_uchar4_sat(double4);
uchar4 __attribute__((overloadable)) convert_uchar4_sat_rte(double4);
uchar4 __attribute__((overloadable)) convert_uchar4_sat_rtn(double4);
uchar4 __attribute__((overloadable)) convert_uchar4_sat_rtp(double4);
uchar4 __attribute__((overloadable)) convert_uchar4_sat_rtz(double4);
uchar8 __attribute__((overloadable)) convert_uchar8(double8);
uchar8 __attribute__((overloadable)) convert_uchar8_rte(double8);
uchar8 __attribute__((overloadable)) convert_uchar8_rtn(double8);
uchar8 __attribute__((overloadable)) convert_uchar8_rtp(double8);
uchar8 __attribute__((overloadable)) convert_uchar8_rtz(double8);
uchar8 __attribute__((overloadable)) convert_uchar8_sat(double8);
uchar8 __attribute__((overloadable)) convert_uchar8_sat_rte(double8);
uchar8 __attribute__((overloadable)) convert_uchar8_sat_rtn(double8);
uchar8 __attribute__((overloadable)) convert_uchar8_sat_rtp(double8);
uchar8 __attribute__((overloadable)) convert_uchar8_sat_rtz(double8);
uchar16 __attribute__((overloadable)) convert_uchar16(double16);
uchar16 __attribute__((overloadable)) convert_uchar16_rte(double16);
uchar16 __attribute__((overloadable)) convert_uchar16_rtn(double16);
uchar16 __attribute__((overloadable)) convert_uchar16_rtp(double16);
uchar16 __attribute__((overloadable)) convert_uchar16_rtz(double16);
uchar16 __attribute__((overloadable)) convert_uchar16_sat(double16);
uchar16 __attribute__((overloadable)) convert_uchar16_sat_rte(double16);
uchar16 __attribute__((overloadable)) convert_uchar16_sat_rtn(double16);
uchar16 __attribute__((overloadable)) convert_uchar16_sat_rtp(double16);
uchar16 __attribute__((overloadable)) convert_uchar16_sat_rtz(double16);

short __attribute__((overloadable)) convert_short(double);
short __attribute__((overloadable)) convert_short_rte(double);
short __attribute__((overloadable)) convert_short_rtn(double);
short __attribute__((overloadable)) convert_short_rtp(double);
short __attribute__((overloadable)) convert_short_rtz(double);
short __attribute__((overloadable)) convert_short_sat(double);
short __attribute__((overloadable)) convert_short_sat_rte(double);
short __attribute__((overloadable)) convert_short_sat_rtn(double);
short __attribute__((overloadable)) convert_short_sat_rtp(double);
short __attribute__((overloadable)) convert_short_sat_rtz(double);
short2 __attribute__((overloadable)) convert_short2(double2);
short2 __attribute__((overloadable)) convert_short2_rte(double2);
short2 __attribute__((overloadable)) convert_short2_rtn(double2);
short2 __attribute__((overloadable)) convert_short2_rtp(double2);
short2 __attribute__((overloadable)) convert_short2_rtz(double2);
short2 __attribute__((overloadable)) convert_short2_sat(double2);
short2 __attribute__((overloadable)) convert_short2_sat_rte(double2);
short2 __attribute__((overloadable)) convert_short2_sat_rtn(double2);
short2 __attribute__((overloadable)) convert_short2_sat_rtp(double2);
short2 __attribute__((overloadable)) convert_short2_sat_rtz(double2);
short3 __attribute__((overloadable)) convert_short3(double3);
short3 __attribute__((overloadable)) convert_short3_rte(double3);
short3 __attribute__((overloadable)) convert_short3_rtn(double3);
short3 __attribute__((overloadable)) convert_short3_rtp(double3);
short3 __attribute__((overloadable)) convert_short3_rtz(double3);
short3 __attribute__((overloadable)) convert_short3_sat(double3);
short3 __attribute__((overloadable)) convert_short3_sat_rte(double3);
short3 __attribute__((overloadable)) convert_short3_sat_rtn(double3);
short3 __attribute__((overloadable)) convert_short3_sat_rtp(double3);
short3 __attribute__((overloadable)) convert_short3_sat_rtz(double3);
short4 __attribute__((overloadable)) convert_short4(double4);
short4 __attribute__((overloadable)) convert_short4_rte(double4);
short4 __attribute__((overloadable)) convert_short4_rtn(double4);
short4 __attribute__((overloadable)) convert_short4_rtp(double4);
short4 __attribute__((overloadable)) convert_short4_rtz(double4);
short4 __attribute__((overloadable)) convert_short4_sat(double4);
short4 __attribute__((overloadable)) convert_short4_sat_rte(double4);
short4 __attribute__((overloadable)) convert_short4_sat_rtn(double4);
short4 __attribute__((overloadable)) convert_short4_sat_rtp(double4);
short4 __attribute__((overloadable)) convert_short4_sat_rtz(double4);
short8 __attribute__((overloadable)) convert_short8(double8);
short8 __attribute__((overloadable)) convert_short8_rte(double8);
short8 __attribute__((overloadable)) convert_short8_rtn(double8);
short8 __attribute__((overloadable)) convert_short8_rtp(double8);
short8 __attribute__((overloadable)) convert_short8_rtz(double8);
short8 __attribute__((overloadable)) convert_short8_sat(double8);
short8 __attribute__((overloadable)) convert_short8_sat_rte(double8);
short8 __attribute__((overloadable)) convert_short8_sat_rtn(double8);
short8 __attribute__((overloadable)) convert_short8_sat_rtp(double8);
short8 __attribute__((overloadable)) convert_short8_sat_rtz(double8);
short16 __attribute__((overloadable)) convert_short16(double16);
short16 __attribute__((overloadable)) convert_short16_rte(double16);
short16 __attribute__((overloadable)) convert_short16_rtn(double16);
short16 __attribute__((overloadable)) convert_short16_rtp(double16);
short16 __attribute__((overloadable)) convert_short16_rtz(double16);
short16 __attribute__((overloadable)) convert_short16_sat(double16);
short16 __attribute__((overloadable)) convert_short16_sat_rte(double16);
short16 __attribute__((overloadable)) convert_short16_sat_rtn(double16);
short16 __attribute__((overloadable)) convert_short16_sat_rtp(double16);
short16 __attribute__((overloadable)) convert_short16_sat_rtz(double16);

ushort __attribute__((overloadable)) convert_ushort(double);
ushort __attribute__((overloadable)) convert_ushort_rte(double);
ushort __attribute__((overloadable)) convert_ushort_rtn(double);
ushort __attribute__((overloadable)) convert_ushort_rtp(double);
ushort __attribute__((overloadable)) convert_ushort_rtz(double);
ushort __attribute__((overloadable)) convert_ushort_sat(double);
ushort __attribute__((overloadable)) convert_ushort_sat_rte(double);
ushort __attribute__((overloadable)) convert_ushort_sat_rtn(double);
ushort __attribute__((overloadable)) convert_ushort_sat_rtp(double);
ushort __attribute__((overloadable)) convert_ushort_sat_rtz(double);
ushort2 __attribute__((overloadable)) convert_ushort2(double2);
ushort2 __attribute__((overloadable)) convert_ushort2_rte(double2);
ushort2 __attribute__((overloadable)) convert_ushort2_rtn(double2);
ushort2 __attribute__((overloadable)) convert_ushort2_rtp(double2);
ushort2 __attribute__((overloadable)) convert_ushort2_rtz(double2);
ushort2 __attribute__((overloadable)) convert_ushort2_sat(double2);
ushort2 __attribute__((overloadable)) convert_ushort2_sat_rte(double2);
ushort2 __attribute__((overloadable)) convert_ushort2_sat_rtn(double2);
ushort2 __attribute__((overloadable)) convert_ushort2_sat_rtp(double2);
ushort2 __attribute__((overloadable)) convert_ushort2_sat_rtz(double2);
ushort3 __attribute__((overloadable)) convert_ushort3(double3);
ushort3 __attribute__((overloadable)) convert_ushort3_rte(double3);
ushort3 __attribute__((overloadable)) convert_ushort3_rtn(double3);
ushort3 __attribute__((overloadable)) convert_ushort3_rtp(double3);
ushort3 __attribute__((overloadable)) convert_ushort3_rtz(double3);
ushort3 __attribute__((overloadable)) convert_ushort3_sat(double3);
ushort3 __attribute__((overloadable)) convert_ushort3_sat_rte(double3);
ushort3 __attribute__((overloadable)) convert_ushort3_sat_rtn(double3);
ushort3 __attribute__((overloadable)) convert_ushort3_sat_rtp(double3);
ushort3 __attribute__((overloadable)) convert_ushort3_sat_rtz(double3);
ushort4 __attribute__((overloadable)) convert_ushort4(double4);
ushort4 __attribute__((overloadable)) convert_ushort4_rte(double4);
ushort4 __attribute__((overloadable)) convert_ushort4_rtn(double4);
ushort4 __attribute__((overloadable)) convert_ushort4_rtp(double4);
ushort4 __attribute__((overloadable)) convert_ushort4_rtz(double4);
ushort4 __attribute__((overloadable)) convert_ushort4_sat(double4);
ushort4 __attribute__((overloadable)) convert_ushort4_sat_rte(double4);
ushort4 __attribute__((overloadable)) convert_ushort4_sat_rtn(double4);
ushort4 __attribute__((overloadable)) convert_ushort4_sat_rtp(double4);
ushort4 __attribute__((overloadable)) convert_ushort4_sat_rtz(double4);
ushort8 __attribute__((overloadable)) convert_ushort8(double8);
ushort8 __attribute__((overloadable)) convert_ushort8_rte(double8);
ushort8 __attribute__((overloadable)) convert_ushort8_rtn(double8);
ushort8 __attribute__((overloadable)) convert_ushort8_rtp(double8);
ushort8 __attribute__((overloadable)) convert_ushort8_rtz(double8);
ushort8 __attribute__((overloadable)) convert_ushort8_sat(double8);
ushort8 __attribute__((overloadable)) convert_ushort8_sat_rte(double8);
ushort8 __attribute__((overloadable)) convert_ushort8_sat_rtn(double8);
ushort8 __attribute__((overloadable)) convert_ushort8_sat_rtp(double8);
ushort8 __attribute__((overloadable)) convert_ushort8_sat_rtz(double8);
ushort16 __attribute__((overloadable)) convert_ushort16(double16);
ushort16 __attribute__((overloadable)) convert_ushort16_rte(double16);
ushort16 __attribute__((overloadable)) convert_ushort16_rtn(double16);
ushort16 __attribute__((overloadable)) convert_ushort16_rtp(double16);
ushort16 __attribute__((overloadable)) convert_ushort16_rtz(double16);
ushort16 __attribute__((overloadable)) convert_ushort16_sat(double16);
ushort16 __attribute__((overloadable)) convert_ushort16_sat_rte(double16);
ushort16 __attribute__((overloadable)) convert_ushort16_sat_rtn(double16);
ushort16 __attribute__((overloadable)) convert_ushort16_sat_rtp(double16);
ushort16 __attribute__((overloadable)) convert_ushort16_sat_rtz(double16);

int __attribute__((overloadable)) convert_int(double);
int __attribute__((overloadable)) convert_int_rte(double);
int __attribute__((overloadable)) convert_int_rtn(double);
int __attribute__((overloadable)) convert_int_rtp(double);
int __attribute__((overloadable)) convert_int_rtz(double);
int __attribute__((overloadable)) convert_int_sat(double);
int __attribute__((overloadable)) convert_int_sat_rte(double);
int __attribute__((overloadable)) convert_int_sat_rtn(double);
int __attribute__((overloadable)) convert_int_sat_rtp(double);
int __attribute__((overloadable)) convert_int_sat_rtz(double);
int2 __attribute__((overloadable)) convert_int2(double2);
int2 __attribute__((overloadable)) convert_int2_rte(double2);
int2 __attribute__((overloadable)) convert_int2_rtn(double2);
int2 __attribute__((overloadable)) convert_int2_rtp(double2);
int2 __attribute__((overloadable)) convert_int2_rtz(double2);
int2 __attribute__((overloadable)) convert_int2_sat(double2);
int2 __attribute__((overloadable)) convert_int2_sat_rte(double2);
int2 __attribute__((overloadable)) convert_int2_sat_rtn(double2);
int2 __attribute__((overloadable)) convert_int2_sat_rtp(double2);
int2 __attribute__((overloadable)) convert_int2_sat_rtz(double2);
int3 __attribute__((overloadable)) convert_int3(double3);
int3 __attribute__((overloadable)) convert_int3_rte(double3);
int3 __attribute__((overloadable)) convert_int3_rtn(double3);
int3 __attribute__((overloadable)) convert_int3_rtp(double3);
int3 __attribute__((overloadable)) convert_int3_rtz(double3);
int3 __attribute__((overloadable)) convert_int3_sat(double3);
int3 __attribute__((overloadable)) convert_int3_sat_rte(double3);
int3 __attribute__((overloadable)) convert_int3_sat_rtn(double3);
int3 __attribute__((overloadable)) convert_int3_sat_rtp(double3);
int3 __attribute__((overloadable)) convert_int3_sat_rtz(double3);
int4 __attribute__((overloadable)) convert_int4(double4);
int4 __attribute__((overloadable)) convert_int4_rte(double4);
int4 __attribute__((overloadable)) convert_int4_rtn(double4);
int4 __attribute__((overloadable)) convert_int4_rtp(double4);
int4 __attribute__((overloadable)) convert_int4_rtz(double4);
int4 __attribute__((overloadable)) convert_int4_sat(double4);
int4 __attribute__((overloadable)) convert_int4_sat_rte(double4);
int4 __attribute__((overloadable)) convert_int4_sat_rtn(double4);
int4 __attribute__((overloadable)) convert_int4_sat_rtp(double4);
int4 __attribute__((overloadable)) convert_int4_sat_rtz(double4);
int8 __attribute__((overloadable)) convert_int8(double8);
int8 __attribute__((overloadable)) convert_int8_rte(double8);
int8 __attribute__((overloadable)) convert_int8_rtn(double8);
int8 __attribute__((overloadable)) convert_int8_rtp(double8);
int8 __attribute__((overloadable)) convert_int8_rtz(double8);
int8 __attribute__((overloadable)) convert_int8_sat(double8);
int8 __attribute__((overloadable)) convert_int8_sat_rte(double8);
int8 __attribute__((overloadable)) convert_int8_sat_rtn(double8);
int8 __attribute__((overloadable)) convert_int8_sat_rtp(double8);
int8 __attribute__((overloadable)) convert_int8_sat_rtz(double8);
int16 __attribute__((overloadable)) convert_int16(double16);
int16 __attribute__((overloadable)) convert_int16_rte(double16);
int16 __attribute__((overloadable)) convert_int16_rtn(double16);
int16 __attribute__((overloadable)) convert_int16_rtp(double16);
int16 __attribute__((overloadable)) convert_int16_rtz(double16);
int16 __attribute__((overloadable)) convert_int16_sat(double16);
int16 __attribute__((overloadable)) convert_int16_sat_rte(double16);
int16 __attribute__((overloadable)) convert_int16_sat_rtn(double16);
int16 __attribute__((overloadable)) convert_int16_sat_rtp(double16);
int16 __attribute__((overloadable)) convert_int16_sat_rtz(double16);

uint __attribute__((overloadable)) convert_uint(double);
uint __attribute__((overloadable)) convert_uint_rte(double);
uint __attribute__((overloadable)) convert_uint_rtn(double);
uint __attribute__((overloadable)) convert_uint_rtp(double);
uint __attribute__((overloadable)) convert_uint_rtz(double);
uint __attribute__((overloadable)) convert_uint_sat(double);
uint __attribute__((overloadable)) convert_uint_sat_rte(double);
uint __attribute__((overloadable)) convert_uint_sat_rtn(double);
uint __attribute__((overloadable)) convert_uint_sat_rtp(double);
uint __attribute__((overloadable)) convert_uint_sat_rtz(double);
uint2 __attribute__((overloadable)) convert_uint2(double2);
uint2 __attribute__((overloadable)) convert_uint2_rte(double2);
uint2 __attribute__((overloadable)) convert_uint2_rtn(double2);
uint2 __attribute__((overloadable)) convert_uint2_rtp(double2);
uint2 __attribute__((overloadable)) convert_uint2_rtz(double2);
uint2 __attribute__((overloadable)) convert_uint2_sat(double2);
uint2 __attribute__((overloadable)) convert_uint2_sat_rte(double2);
uint2 __attribute__((overloadable)) convert_uint2_sat_rtn(double2);
uint2 __attribute__((overloadable)) convert_uint2_sat_rtp(double2);
uint2 __attribute__((overloadable)) convert_uint2_sat_rtz(double2);
uint3 __attribute__((overloadable)) convert_uint3(double3);
uint3 __attribute__((overloadable)) convert_uint3_rte(double3);
uint3 __attribute__((overloadable)) convert_uint3_rtn(double3);
uint3 __attribute__((overloadable)) convert_uint3_rtp(double3);
uint3 __attribute__((overloadable)) convert_uint3_rtz(double3);
uint3 __attribute__((overloadable)) convert_uint3_sat(double3);
uint3 __attribute__((overloadable)) convert_uint3_sat_rte(double3);
uint3 __attribute__((overloadable)) convert_uint3_sat_rtn(double3);
uint3 __attribute__((overloadable)) convert_uint3_sat_rtp(double3);
uint3 __attribute__((overloadable)) convert_uint3_sat_rtz(double3);
uint4 __attribute__((overloadable)) convert_uint4(double4);
uint4 __attribute__((overloadable)) convert_uint4_rte(double4);
uint4 __attribute__((overloadable)) convert_uint4_rtn(double4);
uint4 __attribute__((overloadable)) convert_uint4_rtp(double4);
uint4 __attribute__((overloadable)) convert_uint4_rtz(double4);
uint4 __attribute__((overloadable)) convert_uint4_sat(double4);
uint4 __attribute__((overloadable)) convert_uint4_sat_rte(double4);
uint4 __attribute__((overloadable)) convert_uint4_sat_rtn(double4);
uint4 __attribute__((overloadable)) convert_uint4_sat_rtp(double4);
uint4 __attribute__((overloadable)) convert_uint4_sat_rtz(double4);
uint8 __attribute__((overloadable)) convert_uint8(double8);
uint8 __attribute__((overloadable)) convert_uint8_rte(double8);
uint8 __attribute__((overloadable)) convert_uint8_rtn(double8);
uint8 __attribute__((overloadable)) convert_uint8_rtp(double8);
uint8 __attribute__((overloadable)) convert_uint8_rtz(double8);
uint8 __attribute__((overloadable)) convert_uint8_sat(double8);
uint8 __attribute__((overloadable)) convert_uint8_sat_rte(double8);
uint8 __attribute__((overloadable)) convert_uint8_sat_rtn(double8);
uint8 __attribute__((overloadable)) convert_uint8_sat_rtp(double8);
uint8 __attribute__((overloadable)) convert_uint8_sat_rtz(double8);
uint16 __attribute__((overloadable)) convert_uint16(double16);
uint16 __attribute__((overloadable)) convert_uint16_rte(double16);
uint16 __attribute__((overloadable)) convert_uint16_rtn(double16);
uint16 __attribute__((overloadable)) convert_uint16_rtp(double16);
uint16 __attribute__((overloadable)) convert_uint16_rtz(double16);
uint16 __attribute__((overloadable)) convert_uint16_sat(double16);
uint16 __attribute__((overloadable)) convert_uint16_sat_rte(double16);
uint16 __attribute__((overloadable)) convert_uint16_sat_rtn(double16);
uint16 __attribute__((overloadable)) convert_uint16_sat_rtp(double16);
uint16 __attribute__((overloadable)) convert_uint16_sat_rtz(double16);

long __attribute__((overloadable)) convert_long(double);
long __attribute__((overloadable)) convert_long_rte(double);
long __attribute__((overloadable)) convert_long_rtn(double);
long __attribute__((overloadable)) convert_long_rtp(double);
long __attribute__((overloadable)) convert_long_rtz(double);
long __attribute__((overloadable)) convert_long_sat(double);
long __attribute__((overloadable)) convert_long_sat_rte(double);
long __attribute__((overloadable)) convert_long_sat_rtn(double);
long __attribute__((overloadable)) convert_long_sat_rtp(double);
long __attribute__((overloadable)) convert_long_sat_rtz(double);
long2 __attribute__((overloadable)) convert_long2(double2);
long2 __attribute__((overloadable)) convert_long2_rte(double2);
long2 __attribute__((overloadable)) convert_long2_rtn(double2);
long2 __attribute__((overloadable)) convert_long2_rtp(double2);
long2 __attribute__((overloadable)) convert_long2_rtz(double2);
long2 __attribute__((overloadable)) convert_long2_sat(double2);
long2 __attribute__((overloadable)) convert_long2_sat_rte(double2);
long2 __attribute__((overloadable)) convert_long2_sat_rtn(double2);
long2 __attribute__((overloadable)) convert_long2_sat_rtp(double2);
long2 __attribute__((overloadable)) convert_long2_sat_rtz(double2);
long3 __attribute__((overloadable)) convert_long3(double3);
long3 __attribute__((overloadable)) convert_long3_rte(double3);
long3 __attribute__((overloadable)) convert_long3_rtn(double3);
long3 __attribute__((overloadable)) convert_long3_rtp(double3);
long3 __attribute__((overloadable)) convert_long3_rtz(double3);
long3 __attribute__((overloadable)) convert_long3_sat(double3);
long3 __attribute__((overloadable)) convert_long3_sat_rte(double3);
long3 __attribute__((overloadable)) convert_long3_sat_rtn(double3);
long3 __attribute__((overloadable)) convert_long3_sat_rtp(double3);
long3 __attribute__((overloadable)) convert_long3_sat_rtz(double3);
long4 __attribute__((overloadable)) convert_long4(double4);
long4 __attribute__((overloadable)) convert_long4_rte(double4);
long4 __attribute__((overloadable)) convert_long4_rtn(double4);
long4 __attribute__((overloadable)) convert_long4_rtp(double4);
long4 __attribute__((overloadable)) convert_long4_rtz(double4);
long4 __attribute__((overloadable)) convert_long4_sat(double4);
long4 __attribute__((overloadable)) convert_long4_sat_rte(double4);
long4 __attribute__((overloadable)) convert_long4_sat_rtn(double4);
long4 __attribute__((overloadable)) convert_long4_sat_rtp(double4);
long4 __attribute__((overloadable)) convert_long4_sat_rtz(double4);
long8 __attribute__((overloadable)) convert_long8(double8);
long8 __attribute__((overloadable)) convert_long8_rte(double8);
long8 __attribute__((overloadable)) convert_long8_rtn(double8);
long8 __attribute__((overloadable)) convert_long8_rtp(double8);
long8 __attribute__((overloadable)) convert_long8_rtz(double8);
long8 __attribute__((overloadable)) convert_long8_sat(double8);
long8 __attribute__((overloadable)) convert_long8_sat_rte(double8);
long8 __attribute__((overloadable)) convert_long8_sat_rtn(double8);
long8 __attribute__((overloadable)) convert_long8_sat_rtp(double8);
long8 __attribute__((overloadable)) convert_long8_sat_rtz(double8);
long16 __attribute__((overloadable)) convert_long16(double16);
long16 __attribute__((overloadable)) convert_long16_rte(double16);
long16 __attribute__((overloadable)) convert_long16_rtn(double16);
long16 __attribute__((overloadable)) convert_long16_rtp(double16);
long16 __attribute__((overloadable)) convert_long16_rtz(double16);
long16 __attribute__((overloadable)) convert_long16_sat(double16);
long16 __attribute__((overloadable)) convert_long16_sat_rte(double16);
long16 __attribute__((overloadable)) convert_long16_sat_rtn(double16);
long16 __attribute__((overloadable)) convert_long16_sat_rtp(double16);
long16 __attribute__((overloadable)) convert_long16_sat_rtz(double16);

ulong __attribute__((overloadable)) convert_ulong(double);
ulong __attribute__((overloadable)) convert_ulong_rte(double);
ulong __attribute__((overloadable)) convert_ulong_rtn(double);
ulong __attribute__((overloadable)) convert_ulong_rtp(double);
ulong __attribute__((overloadable)) convert_ulong_rtz(double);
ulong __attribute__((overloadable)) convert_ulong_sat(double);
ulong __attribute__((overloadable)) convert_ulong_sat_rte(double);
ulong __attribute__((overloadable)) convert_ulong_sat_rtn(double);
ulong __attribute__((overloadable)) convert_ulong_sat_rtp(double);
ulong __attribute__((overloadable)) convert_ulong_sat_rtz(double);
ulong2 __attribute__((overloadable)) convert_ulong2(double2);
ulong2 __attribute__((overloadable)) convert_ulong2_rte(double2);
ulong2 __attribute__((overloadable)) convert_ulong2_rtn(double2);
ulong2 __attribute__((overloadable)) convert_ulong2_rtp(double2);
ulong2 __attribute__((overloadable)) convert_ulong2_rtz(double2);
ulong2 __attribute__((overloadable)) convert_ulong2_sat(double2);
ulong2 __attribute__((overloadable)) convert_ulong2_sat_rte(double2);
ulong2 __attribute__((overloadable)) convert_ulong2_sat_rtn(double2);
ulong2 __attribute__((overloadable)) convert_ulong2_sat_rtp(double2);
ulong2 __attribute__((overloadable)) convert_ulong2_sat_rtz(double2);
ulong3 __attribute__((overloadable)) convert_ulong3(double3);
ulong3 __attribute__((overloadable)) convert_ulong3_rte(double3);
ulong3 __attribute__((overloadable)) convert_ulong3_rtn(double3);
ulong3 __attribute__((overloadable)) convert_ulong3_rtp(double3);
ulong3 __attribute__((overloadable)) convert_ulong3_rtz(double3);
ulong3 __attribute__((overloadable)) convert_ulong3_sat(double3);
ulong3 __attribute__((overloadable)) convert_ulong3_sat_rte(double3);
ulong3 __attribute__((overloadable)) convert_ulong3_sat_rtn(double3);
ulong3 __attribute__((overloadable)) convert_ulong3_sat_rtp(double3);
ulong3 __attribute__((overloadable)) convert_ulong3_sat_rtz(double3);
ulong4 __attribute__((overloadable)) convert_ulong4(double4);
ulong4 __attribute__((overloadable)) convert_ulong4_rte(double4);
ulong4 __attribute__((overloadable)) convert_ulong4_rtn(double4);
ulong4 __attribute__((overloadable)) convert_ulong4_rtp(double4);
ulong4 __attribute__((overloadable)) convert_ulong4_rtz(double4);
ulong4 __attribute__((overloadable)) convert_ulong4_sat(double4);
ulong4 __attribute__((overloadable)) convert_ulong4_sat_rte(double4);
ulong4 __attribute__((overloadable)) convert_ulong4_sat_rtn(double4);
ulong4 __attribute__((overloadable)) convert_ulong4_sat_rtp(double4);
ulong4 __attribute__((overloadable)) convert_ulong4_sat_rtz(double4);
ulong8 __attribute__((overloadable)) convert_ulong8(double8);
ulong8 __attribute__((overloadable)) convert_ulong8_rte(double8);
ulong8 __attribute__((overloadable)) convert_ulong8_rtn(double8);
ulong8 __attribute__((overloadable)) convert_ulong8_rtp(double8);
ulong8 __attribute__((overloadable)) convert_ulong8_rtz(double8);
ulong8 __attribute__((overloadable)) convert_ulong8_sat(double8);
ulong8 __attribute__((overloadable)) convert_ulong8_sat_rte(double8);
ulong8 __attribute__((overloadable)) convert_ulong8_sat_rtn(double8);
ulong8 __attribute__((overloadable)) convert_ulong8_sat_rtp(double8);
ulong8 __attribute__((overloadable)) convert_ulong8_sat_rtz(double8);
ulong16 __attribute__((overloadable)) convert_ulong16(double16);
ulong16 __attribute__((overloadable)) convert_ulong16_rte(double16);
ulong16 __attribute__((overloadable)) convert_ulong16_rtn(double16);
ulong16 __attribute__((overloadable)) convert_ulong16_rtp(double16);
ulong16 __attribute__((overloadable)) convert_ulong16_rtz(double16);
ulong16 __attribute__((overloadable)) convert_ulong16_sat(double16);
ulong16 __attribute__((overloadable)) convert_ulong16_sat_rte(double16);
ulong16 __attribute__((overloadable)) convert_ulong16_sat_rtn(double16);
ulong16 __attribute__((overloadable)) convert_ulong16_sat_rtp(double16);
ulong16 __attribute__((overloadable)) convert_ulong16_sat_rtz(double16);

float __attribute__((overloadable)) convert_float(double);
float __attribute__((overloadable)) convert_float_rte(double);
float __attribute__((overloadable)) convert_float_rtn(double);
float __attribute__((overloadable)) convert_float_rtp(double);
float __attribute__((overloadable)) convert_float_rtz(double);
float2 __attribute__((overloadable)) convert_float2(double2);
float2 __attribute__((overloadable)) convert_float2_rte(double2);
float2 __attribute__((overloadable)) convert_float2_rtn(double2);
float2 __attribute__((overloadable)) convert_float2_rtp(double2);
float2 __attribute__((overloadable)) convert_float2_rtz(double2);
float3 __attribute__((overloadable)) convert_float3(double3);
float3 __attribute__((overloadable)) convert_float3_rte(double3);
float3 __attribute__((overloadable)) convert_float3_rtn(double3);
float3 __attribute__((overloadable)) convert_float3_rtp(double3);
float3 __attribute__((overloadable)) convert_float3_rtz(double3);
float4 __attribute__((overloadable)) convert_float4(double4);
float4 __attribute__((overloadable)) convert_float4_rte(double4);
float4 __attribute__((overloadable)) convert_float4_rtn(double4);
float4 __attribute__((overloadable)) convert_float4_rtp(double4);
float4 __attribute__((overloadable)) convert_float4_rtz(double4);
float8 __attribute__((overloadable)) convert_float8(double8);
float8 __attribute__((overloadable)) convert_float8_rte(double8);
float8 __attribute__((overloadable)) convert_float8_rtn(double8);
float8 __attribute__((overloadable)) convert_float8_rtp(double8);
float8 __attribute__((overloadable)) convert_float8_rtz(double8);
float16 __attribute__((overloadable)) convert_float16(double16);
float16 __attribute__((overloadable)) convert_float16_rte(double16);
float16 __attribute__((overloadable)) convert_float16_rtn(double16);
float16 __attribute__((overloadable)) convert_float16_rtp(double16);
float16 __attribute__((overloadable)) convert_float16_rtz(double16);

double __attribute__((overloadable)) convert_double(char);
double __attribute__((overloadable)) convert_double(double);
double __attribute__((overloadable)) convert_double(float);
double __attribute__((overloadable)) convert_double(int);
double __attribute__((overloadable)) convert_double(long);
double __attribute__((overloadable)) convert_double(short);
double __attribute__((overloadable)) convert_double(uchar);
double __attribute__((overloadable)) convert_double(uint);
double __attribute__((overloadable)) convert_double(ulong);
double __attribute__((overloadable)) convert_double(ushort);
double __attribute__((overloadable)) convert_double_rte(char);
double __attribute__((overloadable)) convert_double_rte(double);
double __attribute__((overloadable)) convert_double_rte(float);
double __attribute__((overloadable)) convert_double_rte(int);
double __attribute__((overloadable)) convert_double_rte(long);
double __attribute__((overloadable)) convert_double_rte(short);
double __attribute__((overloadable)) convert_double_rte(uchar);
double __attribute__((overloadable)) convert_double_rte(uint);
double __attribute__((overloadable)) convert_double_rte(ulong);
double __attribute__((overloadable)) convert_double_rte(ushort);
double __attribute__((overloadable)) convert_double_rtn(char);
double __attribute__((overloadable)) convert_double_rtn(double);
double __attribute__((overloadable)) convert_double_rtn(float);
double __attribute__((overloadable)) convert_double_rtn(int);
double __attribute__((overloadable)) convert_double_rtn(long);
double __attribute__((overloadable)) convert_double_rtn(short);
double __attribute__((overloadable)) convert_double_rtn(uchar);
double __attribute__((overloadable)) convert_double_rtn(uint);
double __attribute__((overloadable)) convert_double_rtn(ulong);
double __attribute__((overloadable)) convert_double_rtn(ushort);
double __attribute__((overloadable)) convert_double_rtp(char);
double __attribute__((overloadable)) convert_double_rtp(double);
double __attribute__((overloadable)) convert_double_rtp(float);
double __attribute__((overloadable)) convert_double_rtp(int);
double __attribute__((overloadable)) convert_double_rtp(long);
double __attribute__((overloadable)) convert_double_rtp(short);
double __attribute__((overloadable)) convert_double_rtp(uchar);
double __attribute__((overloadable)) convert_double_rtp(uint);
double __attribute__((overloadable)) convert_double_rtp(ulong);
double __attribute__((overloadable)) convert_double_rtp(ushort);
double __attribute__((overloadable)) convert_double_rtz(char);
double __attribute__((overloadable)) convert_double_rtz(double);
double __attribute__((overloadable)) convert_double_rtz(float);
double __attribute__((overloadable)) convert_double_rtz(int);
double __attribute__((overloadable)) convert_double_rtz(long);
double __attribute__((overloadable)) convert_double_rtz(short);
double __attribute__((overloadable)) convert_double_rtz(uchar);
double __attribute__((overloadable)) convert_double_rtz(uint);
double __attribute__((overloadable)) convert_double_rtz(ulong);
double __attribute__((overloadable)) convert_double_rtz(ushort);
double __attribute__((overloadable)) convert_double_sat(char);
double __attribute__((overloadable)) convert_double_sat(double);
double __attribute__((overloadable)) convert_double_sat(float);
double __attribute__((overloadable)) convert_double_sat(int);
double __attribute__((overloadable)) convert_double_sat(long);
double __attribute__((overloadable)) convert_double_sat(short);
double __attribute__((overloadable)) convert_double_sat(uchar);
double __attribute__((overloadable)) convert_double_sat(uint);
double __attribute__((overloadable)) convert_double_sat(ulong);
double __attribute__((overloadable)) convert_double_sat(ushort);
double __attribute__((overloadable)) convert_double_sat_rte(char);
double __attribute__((overloadable)) convert_double_sat_rte(double);
double __attribute__((overloadable)) convert_double_sat_rte(float);
double __attribute__((overloadable)) convert_double_sat_rte(int);
double __attribute__((overloadable)) convert_double_sat_rte(long);
double __attribute__((overloadable)) convert_double_sat_rte(short);
double __attribute__((overloadable)) convert_double_sat_rte(uchar);
double __attribute__((overloadable)) convert_double_sat_rte(uint);
double __attribute__((overloadable)) convert_double_sat_rte(ulong);
double __attribute__((overloadable)) convert_double_sat_rte(ushort);
double __attribute__((overloadable)) convert_double_sat_rtn(char);
double __attribute__((overloadable)) convert_double_sat_rtn(double);
double __attribute__((overloadable)) convert_double_sat_rtn(float);
double __attribute__((overloadable)) convert_double_sat_rtn(int);
double __attribute__((overloadable)) convert_double_sat_rtn(long);
double __attribute__((overloadable)) convert_double_sat_rtn(short);
double __attribute__((overloadable)) convert_double_sat_rtn(uchar);
double __attribute__((overloadable)) convert_double_sat_rtn(uint);
double __attribute__((overloadable)) convert_double_sat_rtn(ulong);
double __attribute__((overloadable)) convert_double_sat_rtn(ushort);
double __attribute__((overloadable)) convert_double_sat_rtp(char);
double __attribute__((overloadable)) convert_double_sat_rtp(double);
double __attribute__((overloadable)) convert_double_sat_rtp(float);
double __attribute__((overloadable)) convert_double_sat_rtp(int);
double __attribute__((overloadable)) convert_double_sat_rtp(long);
double __attribute__((overloadable)) convert_double_sat_rtp(short);
double __attribute__((overloadable)) convert_double_sat_rtp(uchar);
double __attribute__((overloadable)) convert_double_sat_rtp(uint);
double __attribute__((overloadable)) convert_double_sat_rtp(ulong);
double __attribute__((overloadable)) convert_double_sat_rtp(ushort);
double __attribute__((overloadable)) convert_double_sat_rtz(char);
double __attribute__((overloadable)) convert_double_sat_rtz(double);
double __attribute__((overloadable)) convert_double_sat_rtz(float);
double __attribute__((overloadable)) convert_double_sat_rtz(int);
double __attribute__((overloadable)) convert_double_sat_rtz(long);
double __attribute__((overloadable)) convert_double_sat_rtz(short);
double __attribute__((overloadable)) convert_double_sat_rtz(uchar);
double __attribute__((overloadable)) convert_double_sat_rtz(uint);
double __attribute__((overloadable)) convert_double_sat_rtz(ulong);
double __attribute__((overloadable)) convert_double_sat_rtz(ushort);
double2 __attribute__((overloadable)) convert_double2(char2);
double2 __attribute__((overloadable)) convert_double2(double2);
double2 __attribute__((overloadable)) convert_double2(float2);
double2 __attribute__((overloadable)) convert_double2(int2);
double2 __attribute__((overloadable)) convert_double2(long2);
double2 __attribute__((overloadable)) convert_double2(short2);
double2 __attribute__((overloadable)) convert_double2(uchar2);
double2 __attribute__((overloadable)) convert_double2(uint2);
double2 __attribute__((overloadable)) convert_double2(ulong2);
double2 __attribute__((overloadable)) convert_double2(ushort2);
double2 __attribute__((overloadable)) convert_double2_rte(char2);
double2 __attribute__((overloadable)) convert_double2_rte(double2);
double2 __attribute__((overloadable)) convert_double2_rte(float2);
double2 __attribute__((overloadable)) convert_double2_rte(int2);
double2 __attribute__((overloadable)) convert_double2_rte(long2);
double2 __attribute__((overloadable)) convert_double2_rte(short2);
double2 __attribute__((overloadable)) convert_double2_rte(uchar2);
double2 __attribute__((overloadable)) convert_double2_rte(uint2);
double2 __attribute__((overloadable)) convert_double2_rte(ulong2);
double2 __attribute__((overloadable)) convert_double2_rte(ushort2);
double2 __attribute__((overloadable)) convert_double2_rtn(char2);
double2 __attribute__((overloadable)) convert_double2_rtn(double2);
double2 __attribute__((overloadable)) convert_double2_rtn(float2);
double2 __attribute__((overloadable)) convert_double2_rtn(int2);
double2 __attribute__((overloadable)) convert_double2_rtn(long2);
double2 __attribute__((overloadable)) convert_double2_rtn(short2);
double2 __attribute__((overloadable)) convert_double2_rtn(uchar2);
double2 __attribute__((overloadable)) convert_double2_rtn(uint2);
double2 __attribute__((overloadable)) convert_double2_rtn(ulong2);
double2 __attribute__((overloadable)) convert_double2_rtn(ushort2);
double2 __attribute__((overloadable)) convert_double2_rtp(char2);
double2 __attribute__((overloadable)) convert_double2_rtp(double2);
double2 __attribute__((overloadable)) convert_double2_rtp(float2);
double2 __attribute__((overloadable)) convert_double2_rtp(int2);
double2 __attribute__((overloadable)) convert_double2_rtp(long2);
double2 __attribute__((overloadable)) convert_double2_rtp(short2);
double2 __attribute__((overloadable)) convert_double2_rtp(uchar2);
double2 __attribute__((overloadable)) convert_double2_rtp(uint2);
double2 __attribute__((overloadable)) convert_double2_rtp(ulong2);
double2 __attribute__((overloadable)) convert_double2_rtp(ushort2);
double2 __attribute__((overloadable)) convert_double2_rtz(char2);
double2 __attribute__((overloadable)) convert_double2_rtz(double2);
double2 __attribute__((overloadable)) convert_double2_rtz(float2);
double2 __attribute__((overloadable)) convert_double2_rtz(int2);
double2 __attribute__((overloadable)) convert_double2_rtz(long2);
double2 __attribute__((overloadable)) convert_double2_rtz(short2);
double2 __attribute__((overloadable)) convert_double2_rtz(uchar2);
double2 __attribute__((overloadable)) convert_double2_rtz(uint2);
double2 __attribute__((overloadable)) convert_double2_rtz(ulong2);
double2 __attribute__((overloadable)) convert_double2_rtz(ushort2);
double2 __attribute__((overloadable)) convert_double2_sat(char2);
double2 __attribute__((overloadable)) convert_double2_sat(double2);
double2 __attribute__((overloadable)) convert_double2_sat(float2);
double2 __attribute__((overloadable)) convert_double2_sat(int2);
double2 __attribute__((overloadable)) convert_double2_sat(long2);
double2 __attribute__((overloadable)) convert_double2_sat(short2);
double2 __attribute__((overloadable)) convert_double2_sat(uchar2);
double2 __attribute__((overloadable)) convert_double2_sat(uint2);
double2 __attribute__((overloadable)) convert_double2_sat(ulong2);
double2 __attribute__((overloadable)) convert_double2_sat(ushort2);
double2 __attribute__((overloadable)) convert_double2_sat_rte(char2);
double2 __attribute__((overloadable)) convert_double2_sat_rte(double2);
double2 __attribute__((overloadable)) convert_double2_sat_rte(float2);
double2 __attribute__((overloadable)) convert_double2_sat_rte(int2);
double2 __attribute__((overloadable)) convert_double2_sat_rte(long2);
double2 __attribute__((overloadable)) convert_double2_sat_rte(short2);
double2 __attribute__((overloadable)) convert_double2_sat_rte(uchar2);
double2 __attribute__((overloadable)) convert_double2_sat_rte(uint2);
double2 __attribute__((overloadable)) convert_double2_sat_rte(ulong2);
double2 __attribute__((overloadable)) convert_double2_sat_rte(ushort2);
double2 __attribute__((overloadable)) convert_double2_sat_rtn(char2);
double2 __attribute__((overloadable)) convert_double2_sat_rtn(double2);
double2 __attribute__((overloadable)) convert_double2_sat_rtn(float2);
double2 __attribute__((overloadable)) convert_double2_sat_rtn(int2);
double2 __attribute__((overloadable)) convert_double2_sat_rtn(long2);
double2 __attribute__((overloadable)) convert_double2_sat_rtn(short2);
double2 __attribute__((overloadable)) convert_double2_sat_rtn(uchar2);
double2 __attribute__((overloadable)) convert_double2_sat_rtn(uint2);
double2 __attribute__((overloadable)) convert_double2_sat_rtn(ulong2);
double2 __attribute__((overloadable)) convert_double2_sat_rtn(ushort2);
double2 __attribute__((overloadable)) convert_double2_sat_rtp(char2);
double2 __attribute__((overloadable)) convert_double2_sat_rtp(double2);
double2 __attribute__((overloadable)) convert_double2_sat_rtp(float2);
double2 __attribute__((overloadable)) convert_double2_sat_rtp(int2);
double2 __attribute__((overloadable)) convert_double2_sat_rtp(long2);
double2 __attribute__((overloadable)) convert_double2_sat_rtp(short2);
double2 __attribute__((overloadable)) convert_double2_sat_rtp(uchar2);
double2 __attribute__((overloadable)) convert_double2_sat_rtp(uint2);
double2 __attribute__((overloadable)) convert_double2_sat_rtp(ulong2);
double2 __attribute__((overloadable)) convert_double2_sat_rtp(ushort2);
double2 __attribute__((overloadable)) convert_double2_sat_rtz(char2);
double2 __attribute__((overloadable)) convert_double2_sat_rtz(double2);
double2 __attribute__((overloadable)) convert_double2_sat_rtz(float2);
double2 __attribute__((overloadable)) convert_double2_sat_rtz(int2);
double2 __attribute__((overloadable)) convert_double2_sat_rtz(long2);
double2 __attribute__((overloadable)) convert_double2_sat_rtz(short2);
double2 __attribute__((overloadable)) convert_double2_sat_rtz(uchar2);
double2 __attribute__((overloadable)) convert_double2_sat_rtz(uint2);
double2 __attribute__((overloadable)) convert_double2_sat_rtz(ulong2);
double2 __attribute__((overloadable)) convert_double2_sat_rtz(ushort2);
double3 __attribute__((overloadable)) convert_double3(char3);
double3 __attribute__((overloadable)) convert_double3(double3);
double3 __attribute__((overloadable)) convert_double3(float3);
double3 __attribute__((overloadable)) convert_double3(int3);
double3 __attribute__((overloadable)) convert_double3(long3);
double3 __attribute__((overloadable)) convert_double3(short3);
double3 __attribute__((overloadable)) convert_double3(uchar3);
double3 __attribute__((overloadable)) convert_double3(uint3);
double3 __attribute__((overloadable)) convert_double3(ulong3);
double3 __attribute__((overloadable)) convert_double3(ushort3);
double3 __attribute__((overloadable)) convert_double3_rte(char3);
double3 __attribute__((overloadable)) convert_double3_rte(double3);
double3 __attribute__((overloadable)) convert_double3_rte(float3);
double3 __attribute__((overloadable)) convert_double3_rte(int3);
double3 __attribute__((overloadable)) convert_double3_rte(long3);
double3 __attribute__((overloadable)) convert_double3_rte(short3);
double3 __attribute__((overloadable)) convert_double3_rte(uchar3);
double3 __attribute__((overloadable)) convert_double3_rte(uint3);
double3 __attribute__((overloadable)) convert_double3_rte(ulong3);
double3 __attribute__((overloadable)) convert_double3_rte(ushort3);
double3 __attribute__((overloadable)) convert_double3_rtn(char3);
double3 __attribute__((overloadable)) convert_double3_rtn(double3);
double3 __attribute__((overloadable)) convert_double3_rtn(float3);
double3 __attribute__((overloadable)) convert_double3_rtn(int3);
double3 __attribute__((overloadable)) convert_double3_rtn(long3);
double3 __attribute__((overloadable)) convert_double3_rtn(short3);
double3 __attribute__((overloadable)) convert_double3_rtn(uchar3);
double3 __attribute__((overloadable)) convert_double3_rtn(uint3);
double3 __attribute__((overloadable)) convert_double3_rtn(ulong3);
double3 __attribute__((overloadable)) convert_double3_rtn(ushort3);
double3 __attribute__((overloadable)) convert_double3_rtp(char3);
double3 __attribute__((overloadable)) convert_double3_rtp(double3);
double3 __attribute__((overloadable)) convert_double3_rtp(float3);
double3 __attribute__((overloadable)) convert_double3_rtp(int3);
double3 __attribute__((overloadable)) convert_double3_rtp(long3);
double3 __attribute__((overloadable)) convert_double3_rtp(short3);
double3 __attribute__((overloadable)) convert_double3_rtp(uchar3);
double3 __attribute__((overloadable)) convert_double3_rtp(uint3);
double3 __attribute__((overloadable)) convert_double3_rtp(ulong3);
double3 __attribute__((overloadable)) convert_double3_rtp(ushort3);
double3 __attribute__((overloadable)) convert_double3_rtz(char3);
double3 __attribute__((overloadable)) convert_double3_rtz(double3);
double3 __attribute__((overloadable)) convert_double3_rtz(float3);
double3 __attribute__((overloadable)) convert_double3_rtz(int3);
double3 __attribute__((overloadable)) convert_double3_rtz(long3);
double3 __attribute__((overloadable)) convert_double3_rtz(short3);
double3 __attribute__((overloadable)) convert_double3_rtz(uchar3);
double3 __attribute__((overloadable)) convert_double3_rtz(uint3);
double3 __attribute__((overloadable)) convert_double3_rtz(ulong3);
double3 __attribute__((overloadable)) convert_double3_rtz(ushort3);
double3 __attribute__((overloadable)) convert_double3_sat(char3);
double3 __attribute__((overloadable)) convert_double3_sat(double3);
double3 __attribute__((overloadable)) convert_double3_sat(float3);
double3 __attribute__((overloadable)) convert_double3_sat(int3);
double3 __attribute__((overloadable)) convert_double3_sat(long3);
double3 __attribute__((overloadable)) convert_double3_sat(short3);
double3 __attribute__((overloadable)) convert_double3_sat(uchar3);
double3 __attribute__((overloadable)) convert_double3_sat(uint3);
double3 __attribute__((overloadable)) convert_double3_sat(ulong3);
double3 __attribute__((overloadable)) convert_double3_sat(ushort3);
double3 __attribute__((overloadable)) convert_double3_sat_rte(char3);
double3 __attribute__((overloadable)) convert_double3_sat_rte(double3);
double3 __attribute__((overloadable)) convert_double3_sat_rte(float3);
double3 __attribute__((overloadable)) convert_double3_sat_rte(int3);
double3 __attribute__((overloadable)) convert_double3_sat_rte(long3);
double3 __attribute__((overloadable)) convert_double3_sat_rte(short3);
double3 __attribute__((overloadable)) convert_double3_sat_rte(uchar3);
double3 __attribute__((overloadable)) convert_double3_sat_rte(uint3);
double3 __attribute__((overloadable)) convert_double3_sat_rte(ulong3);
double3 __attribute__((overloadable)) convert_double3_sat_rte(ushort3);
double3 __attribute__((overloadable)) convert_double3_sat_rtn(char3);
double3 __attribute__((overloadable)) convert_double3_sat_rtn(double3);
double3 __attribute__((overloadable)) convert_double3_sat_rtn(float3);
double3 __attribute__((overloadable)) convert_double3_sat_rtn(int3);
double3 __attribute__((overloadable)) convert_double3_sat_rtn(long3);
double3 __attribute__((overloadable)) convert_double3_sat_rtn(short3);
double3 __attribute__((overloadable)) convert_double3_sat_rtn(uchar3);
double3 __attribute__((overloadable)) convert_double3_sat_rtn(uint3);
double3 __attribute__((overloadable)) convert_double3_sat_rtn(ulong3);
double3 __attribute__((overloadable)) convert_double3_sat_rtn(ushort3);
double3 __attribute__((overloadable)) convert_double3_sat_rtp(char3);
double3 __attribute__((overloadable)) convert_double3_sat_rtp(double3);
double3 __attribute__((overloadable)) convert_double3_sat_rtp(float3);
double3 __attribute__((overloadable)) convert_double3_sat_rtp(int3);
double3 __attribute__((overloadable)) convert_double3_sat_rtp(long3);
double3 __attribute__((overloadable)) convert_double3_sat_rtp(short3);
double3 __attribute__((overloadable)) convert_double3_sat_rtp(uchar3);
double3 __attribute__((overloadable)) convert_double3_sat_rtp(uint3);
double3 __attribute__((overloadable)) convert_double3_sat_rtp(ulong3);
double3 __attribute__((overloadable)) convert_double3_sat_rtp(ushort3);
double3 __attribute__((overloadable)) convert_double3_sat_rtz(char3);
double3 __attribute__((overloadable)) convert_double3_sat_rtz(double3);
double3 __attribute__((overloadable)) convert_double3_sat_rtz(float3);
double3 __attribute__((overloadable)) convert_double3_sat_rtz(int3);
double3 __attribute__((overloadable)) convert_double3_sat_rtz(long3);
double3 __attribute__((overloadable)) convert_double3_sat_rtz(short3);
double3 __attribute__((overloadable)) convert_double3_sat_rtz(uchar3);
double3 __attribute__((overloadable)) convert_double3_sat_rtz(uint3);
double3 __attribute__((overloadable)) convert_double3_sat_rtz(ulong3);
double3 __attribute__((overloadable)) convert_double3_sat_rtz(ushort3);
double4 __attribute__((overloadable)) convert_double4(char4);
double4 __attribute__((overloadable)) convert_double4(double4);
double4 __attribute__((overloadable)) convert_double4(float4);
double4 __attribute__((overloadable)) convert_double4(int4);
double4 __attribute__((overloadable)) convert_double4(long4);
double4 __attribute__((overloadable)) convert_double4(short4);
double4 __attribute__((overloadable)) convert_double4(uchar4);
double4 __attribute__((overloadable)) convert_double4(uint4);
double4 __attribute__((overloadable)) convert_double4(ulong4);
double4 __attribute__((overloadable)) convert_double4(ushort4);
double4 __attribute__((overloadable)) convert_double4_rte(char4);
double4 __attribute__((overloadable)) convert_double4_rte(double4);
double4 __attribute__((overloadable)) convert_double4_rte(float4);
double4 __attribute__((overloadable)) convert_double4_rte(int4);
double4 __attribute__((overloadable)) convert_double4_rte(long4);
double4 __attribute__((overloadable)) convert_double4_rte(short4);
double4 __attribute__((overloadable)) convert_double4_rte(uchar4);
double4 __attribute__((overloadable)) convert_double4_rte(uint4);
double4 __attribute__((overloadable)) convert_double4_rte(ulong4);
double4 __attribute__((overloadable)) convert_double4_rte(ushort4);
double4 __attribute__((overloadable)) convert_double4_rtn(char4);
double4 __attribute__((overloadable)) convert_double4_rtn(double4);
double4 __attribute__((overloadable)) convert_double4_rtn(float4);
double4 __attribute__((overloadable)) convert_double4_rtn(int4);
double4 __attribute__((overloadable)) convert_double4_rtn(long4);
double4 __attribute__((overloadable)) convert_double4_rtn(short4);
double4 __attribute__((overloadable)) convert_double4_rtn(uchar4);
double4 __attribute__((overloadable)) convert_double4_rtn(uint4);
double4 __attribute__((overloadable)) convert_double4_rtn(ulong4);
double4 __attribute__((overloadable)) convert_double4_rtn(ushort4);
double4 __attribute__((overloadable)) convert_double4_rtp(char4);
double4 __attribute__((overloadable)) convert_double4_rtp(double4);
double4 __attribute__((overloadable)) convert_double4_rtp(float4);
double4 __attribute__((overloadable)) convert_double4_rtp(int4);
double4 __attribute__((overloadable)) convert_double4_rtp(long4);
double4 __attribute__((overloadable)) convert_double4_rtp(short4);
double4 __attribute__((overloadable)) convert_double4_rtp(uchar4);
double4 __attribute__((overloadable)) convert_double4_rtp(uint4);
double4 __attribute__((overloadable)) convert_double4_rtp(ulong4);
double4 __attribute__((overloadable)) convert_double4_rtp(ushort4);
double4 __attribute__((overloadable)) convert_double4_rtz(char4);
double4 __attribute__((overloadable)) convert_double4_rtz(double4);
double4 __attribute__((overloadable)) convert_double4_rtz(float4);
double4 __attribute__((overloadable)) convert_double4_rtz(int4);
double4 __attribute__((overloadable)) convert_double4_rtz(long4);
double4 __attribute__((overloadable)) convert_double4_rtz(short4);
double4 __attribute__((overloadable)) convert_double4_rtz(uchar4);
double4 __attribute__((overloadable)) convert_double4_rtz(uint4);
double4 __attribute__((overloadable)) convert_double4_rtz(ulong4);
double4 __attribute__((overloadable)) convert_double4_rtz(ushort4);
double4 __attribute__((overloadable)) convert_double4_sat(char4);
double4 __attribute__((overloadable)) convert_double4_sat(double4);
double4 __attribute__((overloadable)) convert_double4_sat(float4);
double4 __attribute__((overloadable)) convert_double4_sat(int4);
double4 __attribute__((overloadable)) convert_double4_sat(long4);
double4 __attribute__((overloadable)) convert_double4_sat(short4);
double4 __attribute__((overloadable)) convert_double4_sat(uchar4);
double4 __attribute__((overloadable)) convert_double4_sat(uint4);
double4 __attribute__((overloadable)) convert_double4_sat(ulong4);
double4 __attribute__((overloadable)) convert_double4_sat(ushort4);
double4 __attribute__((overloadable)) convert_double4_sat_rte(char4);
double4 __attribute__((overloadable)) convert_double4_sat_rte(double4);
double4 __attribute__((overloadable)) convert_double4_sat_rte(float4);
double4 __attribute__((overloadable)) convert_double4_sat_rte(int4);
double4 __attribute__((overloadable)) convert_double4_sat_rte(long4);
double4 __attribute__((overloadable)) convert_double4_sat_rte(short4);
double4 __attribute__((overloadable)) convert_double4_sat_rte(uchar4);
double4 __attribute__((overloadable)) convert_double4_sat_rte(uint4);
double4 __attribute__((overloadable)) convert_double4_sat_rte(ulong4);
double4 __attribute__((overloadable)) convert_double4_sat_rte(ushort4);
double4 __attribute__((overloadable)) convert_double4_sat_rtn(char4);
double4 __attribute__((overloadable)) convert_double4_sat_rtn(double4);
double4 __attribute__((overloadable)) convert_double4_sat_rtn(float4);
double4 __attribute__((overloadable)) convert_double4_sat_rtn(int4);
double4 __attribute__((overloadable)) convert_double4_sat_rtn(long4);
double4 __attribute__((overloadable)) convert_double4_sat_rtn(short4);
double4 __attribute__((overloadable)) convert_double4_sat_rtn(uchar4);
double4 __attribute__((overloadable)) convert_double4_sat_rtn(uint4);
double4 __attribute__((overloadable)) convert_double4_sat_rtn(ulong4);
double4 __attribute__((overloadable)) convert_double4_sat_rtn(ushort4);
double4 __attribute__((overloadable)) convert_double4_sat_rtp(char4);
double4 __attribute__((overloadable)) convert_double4_sat_rtp(double4);
double4 __attribute__((overloadable)) convert_double4_sat_rtp(float4);
double4 __attribute__((overloadable)) convert_double4_sat_rtp(int4);
double4 __attribute__((overloadable)) convert_double4_sat_rtp(long4);
double4 __attribute__((overloadable)) convert_double4_sat_rtp(short4);
double4 __attribute__((overloadable)) convert_double4_sat_rtp(uchar4);
double4 __attribute__((overloadable)) convert_double4_sat_rtp(uint4);
double4 __attribute__((overloadable)) convert_double4_sat_rtp(ulong4);
double4 __attribute__((overloadable)) convert_double4_sat_rtp(ushort4);
double4 __attribute__((overloadable)) convert_double4_sat_rtz(char4);
double4 __attribute__((overloadable)) convert_double4_sat_rtz(double4);
double4 __attribute__((overloadable)) convert_double4_sat_rtz(float4);
double4 __attribute__((overloadable)) convert_double4_sat_rtz(int4);
double4 __attribute__((overloadable)) convert_double4_sat_rtz(long4);
double4 __attribute__((overloadable)) convert_double4_sat_rtz(short4);
double4 __attribute__((overloadable)) convert_double4_sat_rtz(uchar4);
double4 __attribute__((overloadable)) convert_double4_sat_rtz(uint4);
double4 __attribute__((overloadable)) convert_double4_sat_rtz(ulong4);
double4 __attribute__((overloadable)) convert_double4_sat_rtz(ushort4);
double8 __attribute__((overloadable)) convert_double8(char8);
double8 __attribute__((overloadable)) convert_double8(double8);
double8 __attribute__((overloadable)) convert_double8(float8);
double8 __attribute__((overloadable)) convert_double8(int8);
double8 __attribute__((overloadable)) convert_double8(long8);
double8 __attribute__((overloadable)) convert_double8(short8);
double8 __attribute__((overloadable)) convert_double8(uchar8);
double8 __attribute__((overloadable)) convert_double8(uint8);
double8 __attribute__((overloadable)) convert_double8(ulong8);
double8 __attribute__((overloadable)) convert_double8(ushort8);
double8 __attribute__((overloadable)) convert_double8_rte(char8);
double8 __attribute__((overloadable)) convert_double8_rte(double8);
double8 __attribute__((overloadable)) convert_double8_rte(float8);
double8 __attribute__((overloadable)) convert_double8_rte(int8);
double8 __attribute__((overloadable)) convert_double8_rte(long8);
double8 __attribute__((overloadable)) convert_double8_rte(short8);
double8 __attribute__((overloadable)) convert_double8_rte(uchar8);
double8 __attribute__((overloadable)) convert_double8_rte(uint8);
double8 __attribute__((overloadable)) convert_double8_rte(ulong8);
double8 __attribute__((overloadable)) convert_double8_rte(ushort8);
double8 __attribute__((overloadable)) convert_double8_rtn(char8);
double8 __attribute__((overloadable)) convert_double8_rtn(double8);
double8 __attribute__((overloadable)) convert_double8_rtn(float8);
double8 __attribute__((overloadable)) convert_double8_rtn(int8);
double8 __attribute__((overloadable)) convert_double8_rtn(long8);
double8 __attribute__((overloadable)) convert_double8_rtn(short8);
double8 __attribute__((overloadable)) convert_double8_rtn(uchar8);
double8 __attribute__((overloadable)) convert_double8_rtn(uint8);
double8 __attribute__((overloadable)) convert_double8_rtn(ulong8);
double8 __attribute__((overloadable)) convert_double8_rtn(ushort8);
double8 __attribute__((overloadable)) convert_double8_rtp(char8);
double8 __attribute__((overloadable)) convert_double8_rtp(double8);
double8 __attribute__((overloadable)) convert_double8_rtp(float8);
double8 __attribute__((overloadable)) convert_double8_rtp(int8);
double8 __attribute__((overloadable)) convert_double8_rtp(long8);
double8 __attribute__((overloadable)) convert_double8_rtp(short8);
double8 __attribute__((overloadable)) convert_double8_rtp(uchar8);
double8 __attribute__((overloadable)) convert_double8_rtp(uint8);
double8 __attribute__((overloadable)) convert_double8_rtp(ulong8);
double8 __attribute__((overloadable)) convert_double8_rtp(ushort8);
double8 __attribute__((overloadable)) convert_double8_rtz(char8);
double8 __attribute__((overloadable)) convert_double8_rtz(double8);
double8 __attribute__((overloadable)) convert_double8_rtz(float8);
double8 __attribute__((overloadable)) convert_double8_rtz(int8);
double8 __attribute__((overloadable)) convert_double8_rtz(long8);
double8 __attribute__((overloadable)) convert_double8_rtz(short8);
double8 __attribute__((overloadable)) convert_double8_rtz(uchar8);
double8 __attribute__((overloadable)) convert_double8_rtz(uint8);
double8 __attribute__((overloadable)) convert_double8_rtz(ulong8);
double8 __attribute__((overloadable)) convert_double8_rtz(ushort8);
double8 __attribute__((overloadable)) convert_double8_sat(char8);
double8 __attribute__((overloadable)) convert_double8_sat(double8);
double8 __attribute__((overloadable)) convert_double8_sat(float8);
double8 __attribute__((overloadable)) convert_double8_sat(int8);
double8 __attribute__((overloadable)) convert_double8_sat(long8);
double8 __attribute__((overloadable)) convert_double8_sat(short8);
double8 __attribute__((overloadable)) convert_double8_sat(uchar8);
double8 __attribute__((overloadable)) convert_double8_sat(uint8);
double8 __attribute__((overloadable)) convert_double8_sat(ulong8);
double8 __attribute__((overloadable)) convert_double8_sat(ushort8);
double8 __attribute__((overloadable)) convert_double8_sat_rte(char8);
double8 __attribute__((overloadable)) convert_double8_sat_rte(double8);
double8 __attribute__((overloadable)) convert_double8_sat_rte(float8);
double8 __attribute__((overloadable)) convert_double8_sat_rte(int8);
double8 __attribute__((overloadable)) convert_double8_sat_rte(long8);
double8 __attribute__((overloadable)) convert_double8_sat_rte(short8);
double8 __attribute__((overloadable)) convert_double8_sat_rte(uchar8);
double8 __attribute__((overloadable)) convert_double8_sat_rte(uint8);
double8 __attribute__((overloadable)) convert_double8_sat_rte(ulong8);
double8 __attribute__((overloadable)) convert_double8_sat_rte(ushort8);
double8 __attribute__((overloadable)) convert_double8_sat_rtn(char8);
double8 __attribute__((overloadable)) convert_double8_sat_rtn(double8);
double8 __attribute__((overloadable)) convert_double8_sat_rtn(float8);
double8 __attribute__((overloadable)) convert_double8_sat_rtn(int8);
double8 __attribute__((overloadable)) convert_double8_sat_rtn(long8);
double8 __attribute__((overloadable)) convert_double8_sat_rtn(short8);
double8 __attribute__((overloadable)) convert_double8_sat_rtn(uchar8);
double8 __attribute__((overloadable)) convert_double8_sat_rtn(uint8);
double8 __attribute__((overloadable)) convert_double8_sat_rtn(ulong8);
double8 __attribute__((overloadable)) convert_double8_sat_rtn(ushort8);
double8 __attribute__((overloadable)) convert_double8_sat_rtp(char8);
double8 __attribute__((overloadable)) convert_double8_sat_rtp(double8);
double8 __attribute__((overloadable)) convert_double8_sat_rtp(float8);
double8 __attribute__((overloadable)) convert_double8_sat_rtp(int8);
double8 __attribute__((overloadable)) convert_double8_sat_rtp(long8);
double8 __attribute__((overloadable)) convert_double8_sat_rtp(short8);
double8 __attribute__((overloadable)) convert_double8_sat_rtp(uchar8);
double8 __attribute__((overloadable)) convert_double8_sat_rtp(uint8);
double8 __attribute__((overloadable)) convert_double8_sat_rtp(ulong8);
double8 __attribute__((overloadable)) convert_double8_sat_rtp(ushort8);
double8 __attribute__((overloadable)) convert_double8_sat_rtz(char8);
double8 __attribute__((overloadable)) convert_double8_sat_rtz(double8);
double8 __attribute__((overloadable)) convert_double8_sat_rtz(float8);
double8 __attribute__((overloadable)) convert_double8_sat_rtz(int8);
double8 __attribute__((overloadable)) convert_double8_sat_rtz(long8);
double8 __attribute__((overloadable)) convert_double8_sat_rtz(short8);
double8 __attribute__((overloadable)) convert_double8_sat_rtz(uchar8);
double8 __attribute__((overloadable)) convert_double8_sat_rtz(uint8);
double8 __attribute__((overloadable)) convert_double8_sat_rtz(ulong8);
double8 __attribute__((overloadable)) convert_double8_sat_rtz(ushort8);
double16 __attribute__((overloadable)) convert_double16(char16);
double16 __attribute__((overloadable)) convert_double16(double16);
double16 __attribute__((overloadable)) convert_double16(float16);
double16 __attribute__((overloadable)) convert_double16(int16);
double16 __attribute__((overloadable)) convert_double16(long16);
double16 __attribute__((overloadable)) convert_double16(short16);
double16 __attribute__((overloadable)) convert_double16(uchar16);
double16 __attribute__((overloadable)) convert_double16(uint16);
double16 __attribute__((overloadable)) convert_double16(ulong16);
double16 __attribute__((overloadable)) convert_double16(ushort16);
double16 __attribute__((overloadable)) convert_double16_rte(char16);
double16 __attribute__((overloadable)) convert_double16_rte(double16);
double16 __attribute__((overloadable)) convert_double16_rte(float16);
double16 __attribute__((overloadable)) convert_double16_rte(int16);
double16 __attribute__((overloadable)) convert_double16_rte(long16);
double16 __attribute__((overloadable)) convert_double16_rte(short16);
double16 __attribute__((overloadable)) convert_double16_rte(uchar16);
double16 __attribute__((overloadable)) convert_double16_rte(uint16);
double16 __attribute__((overloadable)) convert_double16_rte(ulong16);
double16 __attribute__((overloadable)) convert_double16_rte(ushort16);
double16 __attribute__((overloadable)) convert_double16_rtn(char16);
double16 __attribute__((overloadable)) convert_double16_rtn(double16);
double16 __attribute__((overloadable)) convert_double16_rtn(float16);
double16 __attribute__((overloadable)) convert_double16_rtn(int16);
double16 __attribute__((overloadable)) convert_double16_rtn(long16);
double16 __attribute__((overloadable)) convert_double16_rtn(short16);
double16 __attribute__((overloadable)) convert_double16_rtn(uchar16);
double16 __attribute__((overloadable)) convert_double16_rtn(uint16);
double16 __attribute__((overloadable)) convert_double16_rtn(ulong16);
double16 __attribute__((overloadable)) convert_double16_rtn(ushort16);
double16 __attribute__((overloadable)) convert_double16_rtp(char16);
double16 __attribute__((overloadable)) convert_double16_rtp(double16);
double16 __attribute__((overloadable)) convert_double16_rtp(float16);
double16 __attribute__((overloadable)) convert_double16_rtp(int16);
double16 __attribute__((overloadable)) convert_double16_rtp(long16);
double16 __attribute__((overloadable)) convert_double16_rtp(short16);
double16 __attribute__((overloadable)) convert_double16_rtp(uchar16);
double16 __attribute__((overloadable)) convert_double16_rtp(uint16);
double16 __attribute__((overloadable)) convert_double16_rtp(ulong16);
double16 __attribute__((overloadable)) convert_double16_rtp(ushort16);
double16 __attribute__((overloadable)) convert_double16_rtz(char16);
double16 __attribute__((overloadable)) convert_double16_rtz(double16);
double16 __attribute__((overloadable)) convert_double16_rtz(float16);
double16 __attribute__((overloadable)) convert_double16_rtz(int16);
double16 __attribute__((overloadable)) convert_double16_rtz(long16);
double16 __attribute__((overloadable)) convert_double16_rtz(short16);
double16 __attribute__((overloadable)) convert_double16_rtz(uchar16);
double16 __attribute__((overloadable)) convert_double16_rtz(uint16);
double16 __attribute__((overloadable)) convert_double16_rtz(ulong16);
double16 __attribute__((overloadable)) convert_double16_rtz(ushort16);
double16 __attribute__((overloadable)) convert_double16_sat(char16);
double16 __attribute__((overloadable)) convert_double16_sat(double16);
double16 __attribute__((overloadable)) convert_double16_sat(float16);
double16 __attribute__((overloadable)) convert_double16_sat(int16);
double16 __attribute__((overloadable)) convert_double16_sat(long16);
double16 __attribute__((overloadable)) convert_double16_sat(short16);
double16 __attribute__((overloadable)) convert_double16_sat(uchar16);
double16 __attribute__((overloadable)) convert_double16_sat(uint16);
double16 __attribute__((overloadable)) convert_double16_sat(ulong16);
double16 __attribute__((overloadable)) convert_double16_sat(ushort16);
double16 __attribute__((overloadable)) convert_double16_sat_rte(char16);
double16 __attribute__((overloadable)) convert_double16_sat_rte(double16);
double16 __attribute__((overloadable)) convert_double16_sat_rte(float16);
double16 __attribute__((overloadable)) convert_double16_sat_rte(int16);
double16 __attribute__((overloadable)) convert_double16_sat_rte(long16);
double16 __attribute__((overloadable)) convert_double16_sat_rte(short16);
double16 __attribute__((overloadable)) convert_double16_sat_rte(uchar16);
double16 __attribute__((overloadable)) convert_double16_sat_rte(uint16);
double16 __attribute__((overloadable)) convert_double16_sat_rte(ulong16);
double16 __attribute__((overloadable)) convert_double16_sat_rte(ushort16);
double16 __attribute__((overloadable)) convert_double16_sat_rtn(char16);
double16 __attribute__((overloadable)) convert_double16_sat_rtn(double16);
double16 __attribute__((overloadable)) convert_double16_sat_rtn(float16);
double16 __attribute__((overloadable)) convert_double16_sat_rtn(int16);
double16 __attribute__((overloadable)) convert_double16_sat_rtn(long16);
double16 __attribute__((overloadable)) convert_double16_sat_rtn(short16);
double16 __attribute__((overloadable)) convert_double16_sat_rtn(uchar16);
double16 __attribute__((overloadable)) convert_double16_sat_rtn(uint16);
double16 __attribute__((overloadable)) convert_double16_sat_rtn(ulong16);
double16 __attribute__((overloadable)) convert_double16_sat_rtn(ushort16);
double16 __attribute__((overloadable)) convert_double16_sat_rtp(char16);
double16 __attribute__((overloadable)) convert_double16_sat_rtp(double16);
double16 __attribute__((overloadable)) convert_double16_sat_rtp(float16);
double16 __attribute__((overloadable)) convert_double16_sat_rtp(int16);
double16 __attribute__((overloadable)) convert_double16_sat_rtp(long16);
double16 __attribute__((overloadable)) convert_double16_sat_rtp(short16);
double16 __attribute__((overloadable)) convert_double16_sat_rtp(uchar16);
double16 __attribute__((overloadable)) convert_double16_sat_rtp(uint16);
double16 __attribute__((overloadable)) convert_double16_sat_rtp(ulong16);
double16 __attribute__((overloadable)) convert_double16_sat_rtp(ushort16);
double16 __attribute__((overloadable)) convert_double16_sat_rtz(char16);
double16 __attribute__((overloadable)) convert_double16_sat_rtz(double16);
double16 __attribute__((overloadable)) convert_double16_sat_rtz(float16);
double16 __attribute__((overloadable)) convert_double16_sat_rtz(int16);
double16 __attribute__((overloadable)) convert_double16_sat_rtz(long16);
double16 __attribute__((overloadable)) convert_double16_sat_rtz(short16);
double16 __attribute__((overloadable)) convert_double16_sat_rtz(uchar16);
double16 __attribute__((overloadable)) convert_double16_sat_rtz(uint16);
double16 __attribute__((overloadable)) convert_double16_sat_rtz(ulong16);
double16 __attribute__((overloadable)) convert_double16_sat_rtz(ushort16);

double __attribute__((overloadable)) acos(double);
double2 __attribute__((overloadable)) acos(double2);
double3 __attribute__((overloadable)) acos(double3);
double4 __attribute__((overloadable)) acos(double4);
double8 __attribute__((overloadable)) acos(double8);
double16 __attribute__((overloadable)) acos(double16);

double __attribute__((overloadable)) acosh(double);
double2 __attribute__((overloadable)) acosh(double2);
double3 __attribute__((overloadable)) acosh(double3);
double4 __attribute__((overloadable)) acosh(double4);
double8 __attribute__((overloadable)) acosh(double8);
double16 __attribute__((overloadable)) acosh(double16);

double __attribute__((overloadable)) acospi(double x);
double2 __attribute__((overloadable)) acospi(double2 x);
double3 __attribute__((overloadable)) acospi(double3 x);
double4 __attribute__((overloadable)) acospi(double4 x);
double8 __attribute__((overloadable)) acospi(double8 x);
double16 __attribute__((overloadable)) acospi(double16 x);

double __attribute__((overloadable)) asin(double);
double2 __attribute__((overloadable)) asin(double2);
double3 __attribute__((overloadable)) asin(double3);
double4 __attribute__((overloadable)) asin(double4);
double8 __attribute__((overloadable)) asin(double8);
double16 __attribute__((overloadable)) asin(double16);

double __attribute__((overloadable)) asinh(double);
double2 __attribute__((overloadable)) asinh(double2);
double3 __attribute__((overloadable)) asinh(double3);
double4 __attribute__((overloadable)) asinh(double4);
double8 __attribute__((overloadable)) asinh(double8);
double16 __attribute__((overloadable)) asinh(double16);

double __attribute__((overloadable)) asinpi(double x);
double2 __attribute__((overloadable)) asinpi(double2 x);
double3 __attribute__((overloadable)) asinpi(double3 x);
double4 __attribute__((overloadable)) asinpi(double4 x);
double8 __attribute__((overloadable)) asinpi(double8 x);
double16 __attribute__((overloadable)) asinpi(double16 x);

double __attribute__((overloadable)) atan(double y_over_x);
double2 __attribute__((overloadable)) atan(double2 y_over_x);
double3 __attribute__((overloadable)) atan(double3 y_over_x);
double4 __attribute__((overloadable)) atan(double4 y_over_x);
double8 __attribute__((overloadable)) atan(double8 y_over_x);
double16 __attribute__((overloadable)) atan(double16 y_over_x);

double __attribute__((overloadable)) atan2(double y, double x);
double2 __attribute__((overloadable)) atan2(double2 y, double2 x);
double3 __attribute__((overloadable)) atan2(double3 y, double3 x);
double4 __attribute__((overloadable)) atan2(double4 y, double4 x);
double8 __attribute__((overloadable)) atan2(double8 y, double8 x);
double16 __attribute__((overloadable)) atan2(double16 y, double16 x);

double __attribute__((overloadable)) atanh(double);
double2 __attribute__((overloadable)) atanh(double2);
double3 __attribute__((overloadable)) atanh(double3);
double4 __attribute__((overloadable)) atanh(double4);
double8 __attribute__((overloadable)) atanh(double8);
double16 __attribute__((overloadable)) atanh(double16);

double __attribute__((overloadable)) atanpi(double x);
double2 __attribute__((overloadable)) atanpi(double2 x);
double3 __attribute__((overloadable)) atanpi(double3 x);
double4 __attribute__((overloadable)) atanpi(double4 x);
double8 __attribute__((overloadable)) atanpi(double8 x);
double16 __attribute__((overloadable)) atanpi(double16 x);

double __attribute__((overloadable)) atan2pi(double y, double x);
double2 __attribute__((overloadable)) atan2pi(double2 y, double2 x);
double3 __attribute__((overloadable)) atan2pi(double3 y, double3 x);
double4 __attribute__((overloadable)) atan2pi(double4 y, double4 x);
double8 __attribute__((overloadable)) atan2pi(double8 y, double8 x);
double16 __attribute__((overloadable)) atan2pi(double16 y, double16 x);

double __attribute__((overloadable)) cbrt(double);
double2 __attribute__((overloadable)) cbrt(double2);
double3 __attribute__((overloadable)) cbrt(double3);
double4 __attribute__((overloadable)) cbrt(double4);
double8 __attribute__((overloadable)) cbrt(double8);
double16 __attribute__((overloadable)) cbrt(double16);

double __attribute__((overloadable)) ceil(double);
double2 __attribute__((overloadable)) ceil(double2);
double3 __attribute__((overloadable)) ceil(double3);
double4 __attribute__((overloadable)) ceil(double4);
double8 __attribute__((overloadable)) ceil(double8);
double16 __attribute__((overloadable)) ceil(double16);

double __attribute__((overloadable)) copysign(double x, double y);
double2 __attribute__((overloadable)) copysign(double2 x, double2 y);
double3 __attribute__((overloadable)) copysign(double3 x, double3 y);
double4 __attribute__((overloadable)) copysign(double4 x, double4 y);
double8 __attribute__((overloadable)) copysign(double8 x, double8 y);
double16 __attribute__((overloadable)) copysign(double16 x, double16 y);

double __attribute__((overloadable)) cos(double);
double2 __attribute__((overloadable)) cos(double2);
double3 __attribute__((overloadable)) cos(double3);
double4 __attribute__((overloadable)) cos(double4);
double8 __attribute__((overloadable)) cos(double8);
double16 __attribute__((overloadable)) cos(double16);

double __attribute__((overloadable)) cosh(double);
double2 __attribute__((overloadable)) cosh(double2);
double3 __attribute__((overloadable)) cosh(double3);
double4 __attribute__((overloadable)) cosh(double4);
double8 __attribute__((overloadable)) cosh(double8);
double16 __attribute__((overloadable)) cosh(double16);

double __attribute__((overloadable)) cospi(double x);
double2 __attribute__((overloadable)) cospi(double2 x);
double3 __attribute__((overloadable)) cospi(double3 x);
double4 __attribute__((overloadable)) cospi(double4 x);
double8 __attribute__((overloadable)) cospi(double8 x);
double16 __attribute__((overloadable)) cospi(double16 x);

double __attribute__((overloadable)) erfc(double);
double2 __attribute__((overloadable)) erfc(double2);
double3 __attribute__((overloadable)) erfc(double3);
double4 __attribute__((overloadable)) erfc(double4);
double8 __attribute__((overloadable)) erfc(double8);
double16 __attribute__((overloadable)) erfc(double16);

double __attribute__((overloadable)) erf(double);
double2 __attribute__((overloadable)) erf(double2);
double3 __attribute__((overloadable)) erf(double3);
double4 __attribute__((overloadable)) erf(double4);
double8 __attribute__((overloadable)) erf(double8);
double16 __attribute__((overloadable)) erf(double16);

double __attribute__((overloadable)) exp(double x);
double2 __attribute__((overloadable)) exp(double2 x);
double3 __attribute__((overloadable)) exp(double3 x);
double4 __attribute__((overloadable)) exp(double4 x);
double8 __attribute__((overloadable)) exp(double8 x);
double16 __attribute__((overloadable)) exp(double16 x);

double __attribute__((overloadable)) exp2(double);
double2 __attribute__((overloadable)) exp2(double2);
double3 __attribute__((overloadable)) exp2(double3);
double4 __attribute__((overloadable)) exp2(double4);
double8 __attribute__((overloadable)) exp2(double8);
double16 __attribute__((overloadable)) exp2(double16);

double __attribute__((overloadable)) exp10(double);
double2 __attribute__((overloadable)) exp10(double2);
double3 __attribute__((overloadable)) exp10(double3);
double4 __attribute__((overloadable)) exp10(double4);
double8 __attribute__((overloadable)) exp10(double8);
double16 __attribute__((overloadable)) exp10(double16);

double __attribute__((overloadable)) expm1(double x);
double2 __attribute__((overloadable)) expm1(double2 x);
double3 __attribute__((overloadable)) expm1(double3 x);
double4 __attribute__((overloadable)) expm1(double4 x);
double8 __attribute__((overloadable)) expm1(double8 x);
double16 __attribute__((overloadable)) expm1(double16 x);

double __attribute__((overloadable)) fabs(double);
double2 __attribute__((overloadable)) fabs(double2);
double3 __attribute__((overloadable)) fabs(double3);
double4 __attribute__((overloadable)) fabs(double4);
double8 __attribute__((overloadable)) fabs(double8);
double16 __attribute__((overloadable)) fabs(double16);

double __attribute__((overloadable)) fdim(double x, double y);
double2 __attribute__((overloadable)) fdim(double2 x, double2 y);
double3 __attribute__((overloadable)) fdim(double3 x, double3 y);
double4 __attribute__((overloadable)) fdim(double4 x, double4 y);
double8 __attribute__((overloadable)) fdim(double8 x, double8 y);
double16 __attribute__((overloadable)) fdim(double16 x, double16 y);

double __attribute__((overloadable)) floor(double);
double2 __attribute__((overloadable)) floor(double2);
double3 __attribute__((overloadable)) floor(double3);
double4 __attribute__((overloadable)) floor(double4);
double8 __attribute__((overloadable)) floor(double8);
double16 __attribute__((overloadable)) floor(double16);

double __attribute__((overloadable)) fma(double a, double b, double c);
double2 __attribute__((overloadable)) fma(double2 a, double2 b, double2 c);
double3 __attribute__((overloadable)) fma(double3 a, double3 b, double3 c);
double4 __attribute__((overloadable)) fma(double4 a, double4 b, double4 c);
double8 __attribute__((overloadable)) fma(double8 a, double8 b, double8 c);
double16 __attribute__((overloadable)) fma(double16 a, double16 b, double16 c);

double __attribute__((overloadable)) fmax(double x, double y);
double2 __attribute__((overloadable)) fmax(double2 x, double2 y);
double3 __attribute__((overloadable)) fmax(double3 x, double3 y);
double4 __attribute__((overloadable)) fmax(double4 x, double4 y);
double8 __attribute__((overloadable)) fmax(double8 x, double8 y);
double16 __attribute__((overloadable)) fmax(double16 x, double16 y);
double2 __attribute__((overloadable)) fmax(double2 x, double y);
double3 __attribute__((overloadable)) fmax(double3 x, double y);
double4 __attribute__((overloadable)) fmax(double4 x, double y);
double8 __attribute__((overloadable)) fmax(double8 x, double y);
double16 __attribute__((overloadable)) fmax(double16 x, double y);

double __attribute__((overloadable)) fmin(double x, double y);
double2 __attribute__((overloadable)) fmin(double2 x, double2 y);
double3 __attribute__((overloadable)) fmin(double3 x, double3 y);
double4 __attribute__((overloadable)) fmin(double4 x, double4 y);
double8 __attribute__((overloadable)) fmin(double8 x, double8 y);
double16 __attribute__((overloadable)) fmin(double16 x, double16 y);
double2 __attribute__((overloadable)) fmin(double2 x, double y);
double3 __attribute__((overloadable)) fmin(double3 x, double y);
double4 __attribute__((overloadable)) fmin(double4 x, double y);
double8 __attribute__((overloadable)) fmin(double8 x, double y);
double16 __attribute__((overloadable)) fmin(double16 x, double y);

double __attribute__((overloadable)) fmod(double x, double y);
double2 __attribute__((overloadable)) fmod(double2 x, double2 y);
double3 __attribute__((overloadable)) fmod(double3 x, double3 y);
double4 __attribute__((overloadable)) fmod(double4 x, double4 y);
double8 __attribute__((overloadable)) fmod(double8 x, double8 y);
double16 __attribute__((overloadable)) fmod(double16 x, double16 y);

double __attribute__((overloadable)) hypot(double x, double y);
double2 __attribute__((overloadable)) hypot(double2 x, double2 y);
double3 __attribute__((overloadable)) hypot(double3 x, double3 y);
double4 __attribute__((overloadable)) hypot(double4 x, double4 y);
double8 __attribute__((overloadable)) hypot(double8 x, double8 y);
double16 __attribute__((overloadable)) hypot(double16 x, double16 y);

int __attribute__((overloadable)) ilogb(double x);
int2 __attribute__((overloadable)) ilogb(double2 x);
int3 __attribute__((overloadable)) ilogb(double3 x);
int4 __attribute__((overloadable)) ilogb(double4 x);
int8 __attribute__((overloadable)) ilogb(double8 x);
int16 __attribute__((overloadable)) ilogb(double16 x);

double __attribute__((overloadable)) ldexp(double x, int n);
double2 __attribute__((overloadable)) ldexp(double2 x, int2 n);
double3 __attribute__((overloadable)) ldexp(double3 x, int3 n);
double4 __attribute__((overloadable)) ldexp(double4 x, int4 n);
double8 __attribute__((overloadable)) ldexp(double8 x, int8 n);
double16 __attribute__((overloadable)) ldexp(double16 x, int16 n);
double2 __attribute__((overloadable)) ldexp(double2 x, int n);
double3 __attribute__((overloadable)) ldexp(double3 x, int n);
double4 __attribute__((overloadable)) ldexp(double4 x, int n);
double8 __attribute__((overloadable)) ldexp(double8 x, int n);
double16 __attribute__((overloadable)) ldexp(double16 x, int n);

double __attribute__((overloadable)) lgamma(double x);
double2 __attribute__((overloadable)) lgamma(double2 x);
double3 __attribute__((overloadable)) lgamma(double3 x);
double4 __attribute__((overloadable)) lgamma(double4 x);
double8 __attribute__((overloadable)) lgamma(double8 x);
double16 __attribute__((overloadable)) lgamma(double16 x);

double __attribute__((overloadable)) log(double);
double2 __attribute__((overloadable)) log(double2);
double3 __attribute__((overloadable)) log(double3);
double4 __attribute__((overloadable)) log(double4);
double8 __attribute__((overloadable)) log(double8);
double16 __attribute__((overloadable)) log(double16);

double __attribute__((overloadable)) log2(double);
double2 __attribute__((overloadable)) log2(double2);
double3 __attribute__((overloadable)) log2(double3);
double4 __attribute__((overloadable)) log2(double4);
double8 __attribute__((overloadable)) log2(double8);
double16 __attribute__((overloadable)) log2(double16);

double __attribute__((overloadable)) log10(double);
double2 __attribute__((overloadable)) log10(double2);
double3 __attribute__((overloadable)) log10(double3);
double4 __attribute__((overloadable)) log10(double4);
double8 __attribute__((overloadable)) log10(double8);
double16 __attribute__((overloadable)) log10(double16);

double __attribute__((overloadable)) log1p(double x);
double2 __attribute__((overloadable)) log1p(double2 x);
double3 __attribute__((overloadable)) log1p(double3 x);
double4 __attribute__((overloadable)) log1p(double4 x);
double8 __attribute__((overloadable)) log1p(double8 x);
double16 __attribute__((overloadable)) log1p(double16 x);

double __attribute__((overloadable)) logb(double x);
double2 __attribute__((overloadable)) logb(double2 x);
double3 __attribute__((overloadable)) logb(double3 x);
double4 __attribute__((overloadable)) logb(double4 x);
double8 __attribute__((overloadable)) logb(double8 x);
double16 __attribute__((overloadable)) logb(double16 x);

double __attribute__((overloadable)) mad(double a, double b, double c);
double2 __attribute__((overloadable)) mad(double2 a, double2 b, double2 c);
double3 __attribute__((overloadable)) mad(double3 a, double3 b, double3 c);
double4 __attribute__((overloadable)) mad(double4 a, double4 b, double4 c);
double8 __attribute__((overloadable)) mad(double8 a, double8 b, double8 c);
double16 __attribute__((overloadable)) mad(double16 a, double16 b, double16 c);

double __attribute__((overloadable)) maxmag(double x, double y);
double2 __attribute__((overloadable)) maxmag(double2 x, double2 y);
double3 __attribute__((overloadable)) maxmag(double3 x, double3 y);
double4 __attribute__((overloadable)) maxmag(double4 x, double4 y);
double8 __attribute__((overloadable)) maxmag(double8 x, double8 y);
double16 __attribute__((overloadable)) maxmag(double16 x, double16 y);

double __attribute__((overloadable)) minmag(double x, double y);
double2 __attribute__((overloadable)) minmag(double2 x, double2 y);
double3 __attribute__((overloadable)) minmag(double3 x, double3 y);
double4 __attribute__((overloadable)) minmag(double4 x, double4 y);
double8 __attribute__((overloadable)) minmag(double8 x, double8 y);
double16 __attribute__((overloadable)) minmag(double16 x, double16 y);

double __attribute__((overloadable)) nan(ulong nancode);
double2 __attribute__((overloadable)) nan(ulong2 nancode);
double3 __attribute__((overloadable)) nan(ulong3 nancode);
double4 __attribute__((overloadable)) nan(ulong4 nancode);
double8 __attribute__((overloadable)) nan(ulong8 nancode);
double16 __attribute__((overloadable)) nan(ulong16 nancode);

double __attribute__((overloadable)) nextafter(double x, double y);
double2 __attribute__((overloadable)) nextafter(double2 x, double2 y);
double3 __attribute__((overloadable)) nextafter(double3 x, double3 y);
double4 __attribute__((overloadable)) nextafter(double4 x, double4 y);
double8 __attribute__((overloadable)) nextafter(double8 x, double8 y);
double16 __attribute__((overloadable)) nextafter(double16 x, double16 y);

double __attribute__((overloadable)) pow(double x, double y);
double2 __attribute__((overloadable)) pow(double2 x, double2 y);
double3 __attribute__((overloadable)) pow(double3 x, double3 y);
double4 __attribute__((overloadable)) pow(double4 x, double4 y);
double8 __attribute__((overloadable)) pow(double8 x, double8 y);
double16 __attribute__((overloadable)) pow(double16 x, double16 y);

double __attribute__((overloadable)) pown(double x, int y);
double2 __attribute__((overloadable)) pown(double2 x, int2 y);
double3 __attribute__((overloadable)) pown(double3 x, int3 y);
double4 __attribute__((overloadable)) pown(double4 x, int4 y);
double8 __attribute__((overloadable)) pown(double8 x, int8 y);
double16 __attribute__((overloadable)) pown(double16 x, int16 y);

double __attribute__((overloadable)) powr(double x, double y);
double2 __attribute__((overloadable)) powr(double2 x, double2 y);
double3 __attribute__((overloadable)) powr(double3 x, double3 y);
double4 __attribute__((overloadable)) powr(double4 x, double4 y);
double8 __attribute__((overloadable)) powr(double8 x, double8 y);
double16 __attribute__((overloadable)) powr(double16 x, double16 y);

double __attribute__((overloadable)) remainder(double x, double y);
double2 __attribute__((overloadable)) remainder(double2 x, double2 y);
double3 __attribute__((overloadable)) remainder(double3 x, double3 y);
double4 __attribute__((overloadable)) remainder(double4 x, double4 y);
double8 __attribute__((overloadable)) remainder(double8 x, double8 y);
double16 __attribute__((overloadable)) remainder(double16 x, double16 y);

double __attribute__((overloadable)) rint(double);
double2 __attribute__((overloadable)) rint(double2);
double3 __attribute__((overloadable)) rint(double3);
double4 __attribute__((overloadable)) rint(double4);
double8 __attribute__((overloadable)) rint(double8);
double16 __attribute__((overloadable)) rint(double16);

double __attribute__((overloadable)) rootn(double x, int y);
double2 __attribute__((overloadable)) rootn(double2 x, int2 y);
double3 __attribute__((overloadable)) rootn(double3 x, int3 y);
double4 __attribute__((overloadable)) rootn(double4 x, int4 y);
double8 __attribute__((overloadable)) rootn(double8 x, int8 y);
double16 __attribute__((overloadable)) rootn(double16 x, int16 y);

double __attribute__((overloadable)) round(double x);
double2 __attribute__((overloadable)) round(double2 x);
double3 __attribute__((overloadable)) round(double3 x);
double4 __attribute__((overloadable)) round(double4 x);
double8 __attribute__((overloadable)) round(double8 x);
double16 __attribute__((overloadable)) round(double16 x);

double __attribute__((overloadable)) rsqrt(double);
double2 __attribute__((overloadable)) rsqrt(double2);
double3 __attribute__((overloadable)) rsqrt(double3);
double4 __attribute__((overloadable)) rsqrt(double4);
double8 __attribute__((overloadable)) rsqrt(double8);
double16 __attribute__((overloadable)) rsqrt(double16);

double __attribute__((overloadable)) sin(double);
double2 __attribute__((overloadable)) sin(double2);
double3 __attribute__((overloadable)) sin(double3);
double4 __attribute__((overloadable)) sin(double4);
double8 __attribute__((overloadable)) sin(double8);
double16 __attribute__((overloadable)) sin(double16);

double __attribute__((overloadable)) sinh(double);
double2 __attribute__((overloadable)) sinh(double2);
double3 __attribute__((overloadable)) sinh(double3);
double4 __attribute__((overloadable)) sinh(double4);
double8 __attribute__((overloadable)) sinh(double8);
double16 __attribute__((overloadable)) sinh(double16);

double __attribute__((overloadable)) sinpi(double x);
double2 __attribute__((overloadable)) sinpi(double2 x);
double3 __attribute__((overloadable)) sinpi(double3 x);
double4 __attribute__((overloadable)) sinpi(double4 x);
double8 __attribute__((overloadable)) sinpi(double8 x);
double16 __attribute__((overloadable)) sinpi(double16 x);

double __attribute__((overloadable)) sqrt(double);
double2 __attribute__((overloadable)) sqrt(double2);
double3 __attribute__((overloadable)) sqrt(double3);
double4 __attribute__((overloadable)) sqrt(double4);
double8 __attribute__((overloadable)) sqrt(double8);
double16 __attribute__((overloadable)) sqrt(double16);

double __attribute__((overloadable)) tan(double);
double2 __attribute__((overloadable)) tan(double2);
double3 __attribute__((overloadable)) tan(double3);
double4 __attribute__((overloadable)) tan(double4);
double8 __attribute__((overloadable)) tan(double8);
double16 __attribute__((overloadable)) tan(double16);

double __attribute__((overloadable)) tanh(double);
double2 __attribute__((overloadable)) tanh(double2);
double3 __attribute__((overloadable)) tanh(double3);
double4 __attribute__((overloadable)) tanh(double4);
double8 __attribute__((overloadable)) tanh(double8);
double16 __attribute__((overloadable)) tanh(double16);

double __attribute__((overloadable)) tanpi(double x);
double2 __attribute__((overloadable)) tanpi(double2 x);
double3 __attribute__((overloadable)) tanpi(double3 x);
double4 __attribute__((overloadable)) tanpi(double4 x);
double8 __attribute__((overloadable)) tanpi(double8 x);
double16 __attribute__((overloadable)) tanpi(double16 x);

double __attribute__((overloadable)) tgamma(double);
double2 __attribute__((overloadable)) tgamma(double2);
double3 __attribute__((overloadable)) tgamma(double3);
double4 __attribute__((overloadable)) tgamma(double4);
double8 __attribute__((overloadable)) tgamma(double8);
double16 __attribute__((overloadable)) tgamma(double16);

double __attribute__((overloadable)) trunc(double);
double2 __attribute__((overloadable)) trunc(double2);
double3 __attribute__((overloadable)) trunc(double3);
double4 __attribute__((overloadable)) trunc(double4);
double8 __attribute__((overloadable)) trunc(double8);
double16 __attribute__((overloadable)) trunc(double16);

double __attribute__((overloadable)) native_cos(double x);
double2 __attribute__((overloadable)) native_cos(double2 x);
double3 __attribute__((overloadable)) native_cos(double3 x);
double4 __attribute__((overloadable)) native_cos(double4 x);
double8 __attribute__((overloadable)) native_cos(double8 x);
double16 __attribute__((overloadable)) native_cos(double16 x);

double __attribute__((overloadable)) native_divide(double x, double y);
double2 __attribute__((overloadable)) native_divide(double2 x, double2 y);
double3 __attribute__((overloadable)) native_divide(double3 x, double3 y);
double4 __attribute__((overloadable)) native_divide(double4 x, double4 y);
double8 __attribute__((overloadable)) native_divide(double8 x, double8 y);
double16 __attribute__((overloadable)) native_divide(double16 x, double16 y);

double __attribute__((overloadable)) native_exp(double x);
double2 __attribute__((overloadable)) native_exp(double2 x);
double3 __attribute__((overloadable)) native_exp(double3 x);
double4 __attribute__((overloadable)) native_exp(double4 x);
double8 __attribute__((overloadable)) native_exp(double8 x);
double16 __attribute__((overloadable)) native_exp(double16 x);



double __attribute__((overloadable)) native_exp10(double x);
double2 __attribute__((overloadable)) native_exp10(double2 x);
double3 __attribute__((overloadable)) native_exp10(double3 x);
double4 __attribute__((overloadable)) native_exp10(double4 x);
double8 __attribute__((overloadable)) native_exp10(double8 x);
double16 __attribute__((overloadable)) native_exp10(double16 x);


double __attribute__((overloadable)) native_exp2(double x);
double2 __attribute__((overloadable)) native_exp2(double2 x);
double3 __attribute__((overloadable)) native_exp2(double3 x);
double4 __attribute__((overloadable)) native_exp2(double4 x);
double8 __attribute__((overloadable)) native_exp2(double8 x);
double16 __attribute__((overloadable)) native_exp2(double16 x);


double __attribute__((overloadable)) native_log(double x);
double2 __attribute__((overloadable)) native_log(double2 x);
double3 __attribute__((overloadable)) native_log(double3 x);
double4 __attribute__((overloadable)) native_log(double4 x);
double8 __attribute__((overloadable)) native_log(double8 x);
double16 __attribute__((overloadable)) native_log(double16 x);


double __attribute__((overloadable)) native_log10(double x);
double2 __attribute__((overloadable)) native_log10(double2 x);
double3 __attribute__((overloadable)) native_log10(double3 x);
double4 __attribute__((overloadable)) native_log10(double4 x);
double8 __attribute__((overloadable)) native_log10(double8 x);
double16 __attribute__((overloadable)) native_log10(double16 x);

double __attribute__((overloadable)) native_log2(double x);
double2 __attribute__((overloadable)) native_log2(double2 x);
double3 __attribute__((overloadable)) native_log2(double3 x);
double4 __attribute__((overloadable)) native_log2(double4 x);
double8 __attribute__((overloadable)) native_log2(double8 x);
double16 __attribute__((overloadable)) native_log2(double16 x);


double __attribute__((overloadable)) native_powr(double x, double y);
double2 __attribute__((overloadable)) native_powr(double2 x, double2 y);
double3 __attribute__((overloadable)) native_powr(double3 x, double3 y);
double4 __attribute__((overloadable)) native_powr(double4 x, double4 y);
double8 __attribute__((overloadable)) native_powr(double8 x, double8 y);
double16 __attribute__((overloadable)) native_powr(double16 x, double16 y);


double __attribute__((overloadable)) native_recip(double x);
double2 __attribute__((overloadable)) native_recip(double2 x);
double3 __attribute__((overloadable)) native_recip(double3 x);
double4 __attribute__((overloadable)) native_recip(double4 x);
double8 __attribute__((overloadable)) native_recip(double8 x);
double16 __attribute__((overloadable)) native_recip(double16 x);


double __attribute__((overloadable)) native_rsqrt(double x);
double2 __attribute__((overloadable)) native_rsqrt(double2 x);
double3 __attribute__((overloadable)) native_rsqrt(double3 x);
double4 __attribute__((overloadable)) native_rsqrt(double4 x);
double8 __attribute__((overloadable)) native_rsqrt(double8 x);
double16 __attribute__((overloadable)) native_rsqrt(double16 x);


double __attribute__((overloadable)) native_sin(double x);
double2 __attribute__((overloadable)) native_sin(double2 x);
double3 __attribute__((overloadable)) native_sin(double3 x);
double4 __attribute__((overloadable)) native_sin(double4 x);
double8 __attribute__((overloadable)) native_sin(double8 x);
double16 __attribute__((overloadable)) native_sin(double16 x);

double __attribute__((overloadable)) native_sqrt(double x);
double2 __attribute__((overloadable)) native_sqrt(double2 x);
double3 __attribute__((overloadable)) native_sqrt(double3 x);
double4 __attribute__((overloadable)) native_sqrt(double4 x);
double8 __attribute__((overloadable)) native_sqrt(double8 x);
double16 __attribute__((overloadable)) native_sqrt(double16 x);


double __attribute__((overloadable)) native_tan(double x);
double2 __attribute__((overloadable)) native_tan(double2 x);
double3 __attribute__((overloadable)) native_tan(double3 x);
double4 __attribute__((overloadable)) native_tan(double4 x);
double8 __attribute__((overloadable)) native_tan(double8 x);
double16 __attribute__((overloadable)) native_tan(double16 x);

// Common Functions

double __attribute__((overloadable)) clamp(double x, double minval, double maxval);
double2 __attribute__((overloadable)) clamp(double2 x, double2 minval, double2 maxval);
double3 __attribute__((overloadable)) clamp(double3 x, double3 minval, double3 maxval);
double4 __attribute__((overloadable)) clamp(double4 x, double4 minval, double4 maxval);
double8 __attribute__((overloadable)) clamp(double8 x, double8 minval, double8 maxval);
double16 __attribute__((overloadable)) clamp(double16 x, double16 minval, double16 maxval);
double2 __attribute__((overloadable)) clamp(double2 x, double minval, double maxval);
double3 __attribute__((overloadable)) clamp(double3 x, double minval, double maxval);
double4 __attribute__((overloadable)) clamp(double4 x, double minval, double maxval);
double8 __attribute__((overloadable)) clamp(double8 x, double minval, double maxval);
double16 __attribute__((overloadable)) clamp(double16 x, double minval, double maxval);

double __attribute__((overloadable)) degrees(double radians);
double2 __attribute__((overloadable)) degrees(double2 radians);
double3 __attribute__((overloadable)) degrees(double3 radians);
double4 __attribute__((overloadable)) degrees(double4 radians);
double8 __attribute__((overloadable)) degrees(double8 radians);
double16 __attribute__((overloadable)) degrees(double16 radians);

double __attribute__((overloadable)) max(double x, double y);
double2 __attribute__((overloadable)) max(double2 x, double2 y);
double3 __attribute__((overloadable)) max(double3 x, double3 y);
double4 __attribute__((overloadable)) max(double4 x, double4 y);
double8 __attribute__((overloadable)) max(double8 x, double8 y);
double16 __attribute__((overloadable)) max(double16 x, double16 y);
double2 __attribute__((overloadable)) max(double2 x, double y);
double3 __attribute__((overloadable)) max(double3 x, double y);
double4 __attribute__((overloadable)) max(double4 x, double y);
double8 __attribute__((overloadable)) max(double8 x, double y);
double16 __attribute__((overloadable)) max(double16 x, double y);

double __attribute__((overloadable)) min(double x, double y);
double2 __attribute__((overloadable)) min(double2 x, double2 y);
double3 __attribute__((overloadable)) min(double3 x, double3 y);
double4 __attribute__((overloadable)) min(double4 x, double4 y);
double8 __attribute__((overloadable)) min(double8 x, double8 y);
double16 __attribute__((overloadable)) min(double16 x, double16 y);
double2 __attribute__((overloadable)) min(double2 x, double y);
double3 __attribute__((overloadable)) min(double3 x, double y);
double4 __attribute__((overloadable)) min(double4 x, double y);
double8 __attribute__((overloadable)) min(double8 x, double y);
double16 __attribute__((overloadable)) min(double16 x, double y);

double __attribute__((overloadable)) mix(double x, double y, double a);
double2 __attribute__((overloadable)) mix(double2 x, double2 y, double2 a);
double3 __attribute__((overloadable)) mix(double3 x, double3 y, double3 a);
double4 __attribute__((overloadable)) mix(double4 x, double4 y, double4 a);
double8 __attribute__((overloadable)) mix(double8 x, double8 y, double8 a);
double16 __attribute__((overloadable)) mix(double16 x, double16 y, double16 a);
double2 __attribute__((overloadable)) mix(double2 x, double2 y, double a);
double3 __attribute__((overloadable)) mix(double3 x, double3 y, double a);
double4 __attribute__((overloadable)) mix(double4 x, double4 y, double a);
double8 __attribute__((overloadable)) mix(double8 x, double8 y, double a);
double16 __attribute__((overloadable)) mix(double16 x, double16 y, double a);

double __attribute__((overloadable)) radians(double degrees);
double2 __attribute__((overloadable)) radians(double2 degrees);
double3 __attribute__((overloadable)) radians(double3 degrees);
double4 __attribute__((overloadable)) radians(double4 degrees);
double8 __attribute__((overloadable)) radians(double8 degrees);
double16 __attribute__((overloadable)) radians(double16 degrees);

double __attribute__((overloadable)) step(double edge, double x);
double2 __attribute__((overloadable)) step(double2 edge, double2 x);
double3 __attribute__((overloadable)) step(double3 edge, double3 x);
double4 __attribute__((overloadable)) step(double4 edge, double4 x);
double8 __attribute__((overloadable)) step(double8 edge, double8 x);
double16 __attribute__((overloadable)) step(double16 edge, double16 x);
double2 __attribute__((overloadable)) step(double edge, double2 x);
double3 __attribute__((overloadable)) step(double edge, double3 x);
double4 __attribute__((overloadable)) step(double edge, double4 x);
double8 __attribute__((overloadable)) step(double edge, double8 x);
double16 __attribute__((overloadable)) step(double edge, double16 x);

double __attribute__((overloadable)) smoothstep(double edge0, double edge1, double x);
double2 __attribute__((overloadable)) smoothstep(double2 edge0, double2 edge1, double2 x);
double3 __attribute__((overloadable)) smoothstep(double3 edge0, double3 edge1, double3 x);
double4 __attribute__((overloadable)) smoothstep(double4 edge0, double4 edge1, double4 x);
double8 __attribute__((overloadable)) smoothstep(double8 edge0, double8 edge1, double8 x);
double16 __attribute__((overloadable)) smoothstep(double16 edge0, double16 edge1, double16 x);
double2 __attribute__((overloadable)) smoothstep(double edge0, double edge1, double2 x);
double3 __attribute__((overloadable)) smoothstep(double edge0, double edge1, double3 x);
double4 __attribute__((overloadable)) smoothstep(double edge0, double edge1, double4 x);
double8 __attribute__((overloadable)) smoothstep(double edge0, double edge1, double8 x);
double16 __attribute__((overloadable)) smoothstep(double edge0, double edge1, double16 x);

double __attribute__((overloadable)) sign(double x);
double2 __attribute__((overloadable)) sign(double2 x);
double3 __attribute__((overloadable)) sign(double3 x);
double4 __attribute__((overloadable)) sign(double4 x);
double8 __attribute__((overloadable)) sign(double8 x);
double16 __attribute__((overloadable)) sign(double16 x);

// Geometric Functions

double4 __attribute__((overloadable)) cross(double4 p0, double4 p1);
double3 __attribute__((overloadable)) cross(double3 p0, double3 p1);

double __attribute__((overloadable)) dot(double p0, double p1);
double __attribute__((overloadable)) dot(double2 p0, double2 p1);
double __attribute__((overloadable)) dot(double3 p0, double3 p1);
double __attribute__((overloadable)) dot(double4 p0, double4 p1);

double __attribute__((overloadable)) distance(double p0, double p1);
double __attribute__((overloadable)) distance(double2 p0, double2 p1);
double __attribute__((overloadable)) distance(double3 p0, double3 p1);
double __attribute__((overloadable)) distance(double4 p0, double4 p1);

double __attribute__((overloadable)) length(double p);
double __attribute__((overloadable)) length(double2 p);
double __attribute__((overloadable)) length(double3 p);
double __attribute__((overloadable)) length(double4 p);

double __attribute__((overloadable)) normalize(double p);
double2 __attribute__((overloadable)) normalize(double2 p);
double3 __attribute__((overloadable)) normalize(double3 p);
double4 __attribute__((overloadable)) normalize(double4 p);

// TODO: fp64 fast_distance, fast_length, fast_normalize?

// Relational Functions

int __attribute__((overloadable)) isequal(double x, double y);
long2 __attribute__((overloadable)) isequal(double2 x, double2 y);
long3 __attribute__((overloadable)) isequal(double3 x, double3 y);
long4 __attribute__((overloadable)) isequal(double4 x, double4 y);
long8 __attribute__((overloadable)) isequal(double8 x, double8 y);
long16 __attribute__((overloadable)) isequal(double16 x, double16 y);

int __attribute__((overloadable)) isnotequal(double x, double y);
long2 __attribute__((overloadable)) isnotequal(double2 x, double2 y);
long3 __attribute__((overloadable)) isnotequal(double3 x, double3 y);
long4 __attribute__((overloadable)) isnotequal(double4 x, double4 y);
long8 __attribute__((overloadable)) isnotequal(double8 x, double8 y);
long16 __attribute__((overloadable)) isnotequal(double16 x, double16 y);

int __attribute__((overloadable)) isgreater(double x, double y);
long2 __attribute__((overloadable)) isgreater(double2 x, double2 y);
long3 __attribute__((overloadable)) isgreater(double3 x, double3 y);
long4 __attribute__((overloadable)) isgreater(double4 x, double4 y);
long8 __attribute__((overloadable)) isgreater(double8 x, double8 y);
long16 __attribute__((overloadable)) isgreater(double16 x, double16 y);

int __attribute__((overloadable)) isgreaterequal(double x, double y);
long2 __attribute__((overloadable)) isgreaterequal(double2 x, double2 y);
long3 __attribute__((overloadable)) isgreaterequal(double3 x, double3 y);
long4 __attribute__((overloadable)) isgreaterequal(double4 x, double4 y);
long8 __attribute__((overloadable)) isgreaterequal(double8 x, double8 y);
long16 __attribute__((overloadable)) isgreaterequal(double16 x, double16 y);

int __attribute__((overloadable)) isless(double x, double y);
long2 __attribute__((overloadable)) isless(double2 x, double2 y);
long3 __attribute__((overloadable)) isless(double3 x, double3 y);
long4 __attribute__((overloadable)) isless(double4 x, double4 y);
long8 __attribute__((overloadable)) isless(double8 x, double8 y);
long16 __attribute__((overloadable)) isless(double16 x, double16 y);

int __attribute__((overloadable)) islessequal(double x, double y);
long2 __attribute__((overloadable)) islessequal(double2 x, double2 y);
long3 __attribute__((overloadable)) islessequal(double3 x, double3 y);
long4 __attribute__((overloadable)) islessequal(double4 x, double4 y);
long8 __attribute__((overloadable)) islessequal(double8 x, double8 y);
long16 __attribute__((overloadable)) islessequal(double16 x, double16 y);

int __attribute__((overloadable)) islessgreater(double x, double y);
long2 __attribute__((overloadable)) islessgreater(double2 x, double2 y);
long3 __attribute__((overloadable)) islessgreater(double3 x, double3 y);
long4 __attribute__((overloadable)) islessgreater(double4 x, double4 y);
long8 __attribute__((overloadable)) islessgreater(double8 x, double8 y);
long16 __attribute__((overloadable)) islessgreater(double16 x, double16 y);

int __attribute__((overloadable)) isfinite(double);
long2 __attribute__((overloadable)) isfinite(double2);
long3 __attribute__((overloadable)) isfinite(double3);
long4 __attribute__((overloadable)) isfinite(double4);
long8 __attribute__((overloadable)) isfinite(double8);
long16 __attribute__((overloadable)) isfinite(double16);

int __attribute__((overloadable)) isinf(double);
long2 __attribute__((overloadable)) isinf(double2);
long3 __attribute__((overloadable)) isinf(double3);
long4 __attribute__((overloadable)) isinf(double4);
long8 __attribute__((overloadable)) isinf(double8);
long16 __attribute__((overloadable)) isinf(double16);

int __attribute__((overloadable)) isnan(double);
long2 __attribute__((overloadable)) isnan(double2);
long3 __attribute__((overloadable)) isnan(double3);
long4 __attribute__((overloadable)) isnan(double4);
long8 __attribute__((overloadable)) isnan(double8);
long16 __attribute__((overloadable)) isnan(double16);

int __attribute__((overloadable)) isnormal(double);
long2 __attribute__((overloadable)) isnormal(double2);
long3 __attribute__((overloadable)) isnormal(double3);
long4 __attribute__((overloadable)) isnormal(double4);
long8 __attribute__((overloadable)) isnormal(double8);
long16 __attribute__((overloadable)) isnormal(double16);

int __attribute__((overloadable)) isordered(double x, double y);
long2 __attribute__((overloadable)) isordered(double2 x, double2 y);
long3 __attribute__((overloadable)) isordered(double3 x, double3 y);
long4 __attribute__((overloadable)) isordered(double4 x, double4 y);
long8 __attribute__((overloadable)) isordered(double8 x, double8 y);
long16 __attribute__((overloadable)) isordered(double16 x, double16 y);

int __attribute__((overloadable)) isunordered(double x, double y);
long2 __attribute__((overloadable)) isunordered(double2 x, double2 y);
long3 __attribute__((overloadable)) isunordered(double3 x, double3 y);
long4 __attribute__((overloadable)) isunordered(double4 x, double4 y);
long8 __attribute__((overloadable)) isunordered(double8 x, double8 y);
long16 __attribute__((overloadable)) isunordered(double16 x, double16 y);

int __attribute__((overloadable)) signbit(double);
long2 __attribute__((overloadable)) signbit(double2);
long3 __attribute__((overloadable)) signbit(double3);
long4 __attribute__((overloadable)) signbit(double4);
long8 __attribute__((overloadable)) signbit(double8);
long16 __attribute__((overloadable)) signbit(double16);

double __attribute__((overloadable)) bitselect(double a, double b, double c);
double2 __attribute__((overloadable)) bitselect(double2 a, double2 b, double2 c);
double3 __attribute__((overloadable)) bitselect(double3 a, double3 b, double3 c);
double4 __attribute__((overloadable)) bitselect(double4 a, double4 b, double4 c);
double8 __attribute__((overloadable)) bitselect(double8 a, double8 b, double8 c);
double16 __attribute__((overloadable)) bitselect(double16 a, double16 b, double16 c);

double __attribute__((overloadable)) select(double a, double b, long c);
double2 __attribute__((overloadable)) select(double2 a, double2 b, long2 c);
double3 __attribute__((overloadable)) select(double3 a, double3 b, long3 c);
double4 __attribute__((overloadable)) select(double4 a, double4 b, long4 c);
double8 __attribute__((overloadable)) select(double8 a, double8 b, long8 c);
double16 __attribute__((overloadable)) select(double16 a, double16 b, long16 c);
double __attribute__((overloadable)) select(double a, double b, ulong c);
double2 __attribute__((overloadable)) select(double2 a, double2 b, ulong2 c);
double3 __attribute__((overloadable)) select(double3 a, double3 b, ulong3 c);
double4 __attribute__((overloadable)) select(double4 a, double4 b, ulong4 c);
double8 __attribute__((overloadable)) select(double8 a, double8 b, ulong8 c);
double16 __attribute__((overloadable)) select(double16 a, double16 b, ulong16 c);

// Async Copies and Prefetch

event_t __attribute__((overloadable)) async_work_group_copy(__local double *dst, const __global double *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local double2 *dst, const __global double2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local double3 *dst, const __global double3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local double4 *dst, const __global double4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local double8 *dst, const __global double8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__local double16 *dst, const __global double16 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global double *dst, const __local double *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global double2 *dst, const __local double2 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global double3 *dst, const __local double3 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global double4 *dst, const __local double4 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global double8 *dst, const __local double8 *src, size_t num_elements, event_t event);
event_t __attribute__((overloadable)) async_work_group_copy(__global double16 *dst, const __local double16 *src, size_t num_elements, event_t event);

event_t __attribute__((overloadable)) async_work_group_strided_copy(__local double *dst, const __global double *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local double2 *dst, const __global double2 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local double3 *dst, const __global double3 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local double4 *dst, const __global double4 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local double8 *dst, const __global double8 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__local double16 *dst, const __global double16 *src, size_t num_elements, size_t src_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global double *dst, const __local double *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global double2 *dst, const __local double2 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global double3 *dst, const __local double3 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global double4 *dst, const __local double4 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global double8 *dst, const __local double8 *src, size_t num_elements, size_t dst_stride, event_t event);
event_t __attribute__((overloadable)) async_work_group_strided_copy(__global double16 *dst, const __local double16 *src, size_t num_elements, size_t dst_stride, event_t event);

// Miscellaneous Vector Instructions

double2   __attribute__((overloadable)) shuffle(double2 x, ulong2 mask);
double2   __attribute__((overloadable)) shuffle(double4 x, ulong2 mask);
double2   __attribute__((overloadable)) shuffle(double8 x, ulong2 mask);
double2   __attribute__((overloadable)) shuffle(double16 x, ulong2 mask);

double4   __attribute__((overloadable)) shuffle(double2 x, ulong4 mask);
double4   __attribute__((overloadable)) shuffle(double4 x, ulong4 mask);
double4   __attribute__((overloadable)) shuffle(double8 x, ulong4 mask);
double4   __attribute__((overloadable)) shuffle(double16 x, ulong4 mask);

double8   __attribute__((overloadable)) shuffle(double2 x, ulong8 mask);
double8   __attribute__((overloadable)) shuffle(double4 x, ulong8 mask);
double8   __attribute__((overloadable)) shuffle(double8 x, ulong8 mask);
double8   __attribute__((overloadable)) shuffle(double16 x, ulong8 mask);

double16   __attribute__((overloadable)) shuffle(double2 x, ulong16 mask);
double16   __attribute__((overloadable)) shuffle(double4 x, ulong16 mask);
double16   __attribute__((overloadable)) shuffle(double8 x, ulong16 mask);
double16   __attribute__((overloadable)) shuffle(double16 x, ulong16 mask);

double2   __attribute__((overloadable)) shuffle2(double2 x, double2 y, ulong2 mask);
double2   __attribute__((overloadable)) shuffle2(double4 x, double4 y, ulong2 mask);
double2   __attribute__((overloadable)) shuffle2(double8 x, double8 y, ulong2 mask);
double2   __attribute__((overloadable)) shuffle2(double16 x, double16 y, ulong2 mask);

double4   __attribute__((overloadable)) shuffle2(double2 x, double2 y, ulong4 mask);
double4   __attribute__((overloadable)) shuffle2(double4 x, double4 y, ulong4 mask);
double4   __attribute__((overloadable)) shuffle2(double8 x, double8 y, ulong4 mask);
double4   __attribute__((overloadable)) shuffle2(double16 x, double16 y, ulong4 mask);

double8   __attribute__((overloadable)) shuffle2(double2 x, double2 y, ulong8 mask);
double8   __attribute__((overloadable)) shuffle2(double4 x, double4 y, ulong8 mask);
double8   __attribute__((overloadable)) shuffle2(double8 x, double8 y, ulong8 mask);
double8   __attribute__((overloadable)) shuffle2(double16 x, double16 y, ulong8 mask);

double16   __attribute__((overloadable)) shuffle2(double2 x, double2 y, ulong16 mask);
double16   __attribute__((overloadable)) shuffle2(double4 x, double4 y, ulong16 mask);
double16   __attribute__((overloadable)) shuffle2(double8 x, double8 y, ulong16 mask);
double16   __attribute__((overloadable)) shuffle2(double16 x, double16 y, ulong16 mask);

#endif

////////////////////////////////////////////////////////////////////////////////////
////              fp16 / fp64 conversions
////
////////////////////////////////////////////////////////////////////////////////////

#if defined(cl_khr_fp64) && defined(cl_khr_fp16)
half __attribute__((overloadable)) convert_half(double);
half __attribute__((overloadable)) convert_half_rte(double);
half __attribute__((overloadable)) convert_half_rtn(double);
half __attribute__((overloadable)) convert_half_rtp(double);
half __attribute__((overloadable)) convert_half_rtz(double);
half2 __attribute__((overloadable)) convert_half2(double2);
half2 __attribute__((overloadable)) convert_half2_rte(double2);
half2 __attribute__((overloadable)) convert_half2_rtn(double2);
half2 __attribute__((overloadable)) convert_half2_rtp(double2);
half2 __attribute__((overloadable)) convert_half2_rtz(double2);
half3 __attribute__((overloadable)) convert_half3(double3);
half3 __attribute__((overloadable)) convert_half3_rte(double3);
half3 __attribute__((overloadable)) convert_half3_rtn(double3);
half3 __attribute__((overloadable)) convert_half3_rtp(double3);
half3 __attribute__((overloadable)) convert_half3_rtz(double3);
half4 __attribute__((overloadable)) convert_half4(double4);
half4 __attribute__((overloadable)) convert_half4_rte(double4);
half4 __attribute__((overloadable)) convert_half4_rtn(double4);
half4 __attribute__((overloadable)) convert_half4_rtp(double4);
half4 __attribute__((overloadable)) convert_half4_rtz(double4);
half8 __attribute__((overloadable)) convert_half8(double8);
half8 __attribute__((overloadable)) convert_half8_rte(double8);
half8 __attribute__((overloadable)) convert_half8_rtn(double8);
half8 __attribute__((overloadable)) convert_half8_rtp(double8);
half8 __attribute__((overloadable)) convert_half8_rtz(double8);
half16 __attribute__((overloadable)) convert_half16(double16);
half16 __attribute__((overloadable)) convert_half16_rte(double16);
half16 __attribute__((overloadable)) convert_half16_rtn(double16);
half16 __attribute__((overloadable)) convert_half16_rtp(double16);
half16 __attribute__((overloadable)) convert_half16_rtz(double16);

double __attribute__((overloadable)) convert_double(half);
double __attribute__((overloadable)) convert_double_rte(half);
double __attribute__((overloadable)) convert_double_rtn(half);
double __attribute__((overloadable)) convert_double_rtp(half);
double __attribute__((overloadable)) convert_double_rtz(half);
double2 __attribute__((overloadable)) convert_double2(half2);
double2 __attribute__((overloadable)) convert_double2_rte(half2);
double2 __attribute__((overloadable)) convert_double2_rtn(half2);
double2 __attribute__((overloadable)) convert_double2_rtp(half2);
double2 __attribute__((overloadable)) convert_double2_rtz(half2);
double3 __attribute__((overloadable)) convert_double3(half3);
double3 __attribute__((overloadable)) convert_double3_rte(half3);
double3 __attribute__((overloadable)) convert_double3_rtn(half3);
double3 __attribute__((overloadable)) convert_double3_rtp(half3);
double3 __attribute__((overloadable)) convert_double3_rtz(half3);
double4 __attribute__((overloadable)) convert_double4(half4);
double4 __attribute__((overloadable)) convert_double4_rte(half4);
double4 __attribute__((overloadable)) convert_double4_rtn(half4);
double4 __attribute__((overloadable)) convert_double4_rtp(half4);
double4 __attribute__((overloadable)) convert_double4_rtz(half4);
double8 __attribute__((overloadable)) convert_double8(half8);
double8 __attribute__((overloadable)) convert_double8_rte(half8);
double8 __attribute__((overloadable)) convert_double8_rtn(half8);
double8 __attribute__((overloadable)) convert_double8_rtp(half8);
double8 __attribute__((overloadable)) convert_double8_rtz(half8);
double16 __attribute__((overloadable)) convert_double16(half16);
double16 __attribute__((overloadable)) convert_double16_rte(half16);
double16 __attribute__((overloadable)) convert_double16_rtn(half16);
double16 __attribute__((overloadable)) convert_double16_rtp(half16);
double16 __attribute__((overloadable)) convert_double16_rtz(half16);
#endif

#if defined(cl_intel_bfloat16_conversions)
ushort __attribute__((overloadable)) intel_convert_bfloat16_as_ushort(float source);
ushort2 __attribute__((overloadable)) intel_convert_bfloat162_as_ushort2(float2 source);
ushort3 __attribute__((overloadable)) intel_convert_bfloat163_as_ushort3(float3 source);
ushort4 __attribute__((overloadable)) intel_convert_bfloat164_as_ushort4(float4 source);
ushort8 __attribute__((overloadable)) intel_convert_bfloat168_as_ushort8(float8 source);
ushort16 __attribute__((overloadable)) intel_convert_bfloat1616_as_ushort16(float16 source);

float __attribute__((overloadable)) intel_convert_as_bfloat16_float(ushort source);
float2 __attribute__((overloadable)) intel_convert_as_bfloat162_float2(ushort2 source);
float3 __attribute__((overloadable)) intel_convert_as_bfloat163_float3(ushort3 source);
float4 __attribute__((overloadable)) intel_convert_as_bfloat164_float4(ushort4 source);
float8 __attribute__((overloadable)) intel_convert_as_bfloat168_float8(ushort8 source);
float16  __attribute__((overloadable)) intel_convert_as_bfloat1616_float16(ushort16 source);
#endif // defined(cl_intel_bfloat16_conversions)

#if defined(cl_intel_simd_operations_placeholder) || defined(cl_intel_subgroups) || defined(cl_khr_subgroups) || defined(__opencl_c_subgroups)
// Shared Sub Group Functions
uint    __attribute__((overloadable)) get_sub_group_size( void );
uint    __attribute__((overloadable)) get_max_sub_group_size( void );
uint    __attribute__((overloadable)) get_num_sub_groups( void );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
uint    __attribute__((overloadable)) get_enqueued_num_sub_groups( void );
#endif
uint    __attribute__((overloadable)) get_sub_group_id( void );
uint    __attribute__((overloadable)) get_sub_group_local_id( void );

void    __attribute__((overloadable)) sub_group_barrier( cl_mem_fence_flags flags );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void    __attribute__((overloadable)) sub_group_barrier( cl_mem_fence_flags flags, memory_scope scope );
#endif

int     __attribute__((overloadable)) sub_group_all( int predicate );
int     __attribute__((overloadable)) sub_group_any( int predicate );

#if defined(cl_khr_subgroup_non_uniform_vote)
int     __attribute__((overloadable)) sub_group_elect(void);
int     __attribute__((overloadable)) sub_group_non_uniform_all(int predicate);
int     __attribute__((overloadable)) sub_group_non_uniform_any(int predicate);
int     __attribute__((overloadable)) sub_group_non_uniform_all_equal(char value);
int     __attribute__((overloadable)) sub_group_non_uniform_all_equal(uchar value);
int     __attribute__((overloadable)) sub_group_non_uniform_all_equal(short value);
int     __attribute__((overloadable)) sub_group_non_uniform_all_equal(ushort value);
int     __attribute__((overloadable)) sub_group_non_uniform_all_equal(int value);
int     __attribute__((overloadable)) sub_group_non_uniform_all_equal(uint value);
int     __attribute__((overloadable)) sub_group_non_uniform_all_equal(long value);
int     __attribute__((overloadable)) sub_group_non_uniform_all_equal(ulong value);
int     __attribute__((overloadable)) sub_group_non_uniform_all_equal(float value);
#if defined(cl_khr_fp64)
int     __attribute__((overloadable)) sub_group_non_uniform_all_equal(double value);
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
int     __attribute__((overloadable)) sub_group_non_uniform_all_equal(half value);
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_non_uniform_vote)

#if defined(cl_khr_subgroup_ballot)
#define DECL_NON_UNIFORM_BROADCAST_BASE(GENTYPE)                                                          \
    GENTYPE __attribute__((overloadable)) sub_group_non_uniform_broadcast(GENTYPE value, uint index);     \
    GENTYPE __attribute__((overloadable)) sub_group_broadcast_first(GENTYPE value);

#define DECL_NON_UNIFORM_BROADCAST(TYPE)        \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE)       \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE##2)    \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE##3)    \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE##4)    \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE##8)    \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE##16)

DECL_NON_UNIFORM_BROADCAST(char)
DECL_NON_UNIFORM_BROADCAST(uchar)
DECL_NON_UNIFORM_BROADCAST(short)
DECL_NON_UNIFORM_BROADCAST(ushort)
DECL_NON_UNIFORM_BROADCAST(int)
DECL_NON_UNIFORM_BROADCAST(uint)
DECL_NON_UNIFORM_BROADCAST(long)
DECL_NON_UNIFORM_BROADCAST(ulong)
DECL_NON_UNIFORM_BROADCAST(float)
#if defined(cl_khr_fp64)
DECL_NON_UNIFORM_BROADCAST(double)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DECL_NON_UNIFORM_BROADCAST(half)
#endif // defined(cl_khr_fp16)

uint4  __attribute__((overloadable)) sub_group_ballot(int predicate);
int    __attribute__((overloadable)) sub_group_inverse_ballot(uint4 value);
int    __attribute__((overloadable)) sub_group_ballot_bit_extract(uint4 value, uint index);
uint   __attribute__((overloadable)) sub_group_ballot_bit_count(uint4 value);
uint   __attribute__((overloadable)) sub_group_ballot_inclusive_scan(uint4 value);
uint   __attribute__((overloadable)) sub_group_ballot_exclusive_scan(uint4 value);
uint   __attribute__((overloadable)) sub_group_ballot_find_lsb(uint4 value);
uint   __attribute__((overloadable)) sub_group_ballot_find_msb(uint4 value);
uint4  __attribute__((overloadable)) get_sub_group_eq_mask(void);
uint4  __attribute__((overloadable)) get_sub_group_ge_mask(void);
uint4  __attribute__((overloadable)) get_sub_group_gt_mask(void);
uint4  __attribute__((overloadable)) get_sub_group_le_mask(void);
uint4  __attribute__((overloadable)) get_sub_group_lt_mask(void);
#endif // defined(cl_khr_subgroup_ballot)

int     __attribute__((overloadable)) sub_group_broadcast( int   x, uint sub_group_local_id );
uint    __attribute__((overloadable)) sub_group_broadcast( uint  x, uint sub_group_local_id );
long    __attribute__((overloadable)) sub_group_broadcast( long  x, uint sub_group_local_id );
ulong   __attribute__((overloadable)) sub_group_broadcast( ulong x, uint sub_group_local_id );
float   __attribute__((overloadable)) sub_group_broadcast( float x, uint sub_group_local_id );

DECL_GROUP_REDUCE_SCAN(sub_group, int)
DECL_GROUP_REDUCE_SCAN(sub_group, uint)
DECL_GROUP_REDUCE_SCAN(sub_group, long)
DECL_GROUP_REDUCE_SCAN(sub_group, ulong)
DECL_GROUP_REDUCE_SCAN(sub_group, float)

#ifdef cl_khr_fp16
half    __attribute__((overloadable)) sub_group_broadcast( half x, uint sub_group_local_id );
DECL_GROUP_REDUCE_SCAN(sub_group, half)
#endif

#if defined(cl_khr_fp64)
double  __attribute__((overloadable)) sub_group_broadcast( double x, uint sub_group_local_id );
DECL_GROUP_REDUCE_SCAN(sub_group, double)
#endif
#endif

#if defined(cl_khr_subgroup_extended_types)

char    __attribute__((overloadable)) sub_group_broadcast( char   x, uint sub_group_local_id );
uchar   __attribute__((overloadable)) sub_group_broadcast( uchar  x, uint sub_group_local_id );
short   __attribute__((overloadable)) sub_group_broadcast( short  x, uint sub_group_local_id );
ushort  __attribute__((overloadable)) sub_group_broadcast( ushort x, uint sub_group_local_id );

#define DECL_SUB_GROUP_BROADCAST_VEC(type) \
type##2  __attribute__((overloadable)) sub_group_broadcast( type##2  x, uint sub_group_local_id ); \
type##3  __attribute__((overloadable)) sub_group_broadcast( type##3  x, uint sub_group_local_id ); \
type##4  __attribute__((overloadable)) sub_group_broadcast( type##4  x, uint sub_group_local_id ); \
type##8  __attribute__((overloadable)) sub_group_broadcast( type##8  x, uint sub_group_local_id ); \
type##16 __attribute__((overloadable)) sub_group_broadcast( type##16 x, uint sub_group_local_id );
DECL_SUB_GROUP_BROADCAST_VEC(char)
DECL_SUB_GROUP_BROADCAST_VEC(uchar)
DECL_SUB_GROUP_BROADCAST_VEC(short)
DECL_SUB_GROUP_BROADCAST_VEC(ushort)
DECL_SUB_GROUP_BROADCAST_VEC(int)
DECL_SUB_GROUP_BROADCAST_VEC(uint)
DECL_SUB_GROUP_BROADCAST_VEC(long)
DECL_SUB_GROUP_BROADCAST_VEC(ulong)
DECL_SUB_GROUP_BROADCAST_VEC(float)
#ifdef cl_khr_fp16
DECL_SUB_GROUP_BROADCAST_VEC(half)
#endif
#if defined(cl_khr_fp64)
DECL_SUB_GROUP_BROADCAST_VEC(double)
#endif

DECL_GROUP_REDUCE_SCAN(sub_group, char)
DECL_GROUP_REDUCE_SCAN(sub_group, uchar)
DECL_GROUP_REDUCE_SCAN(sub_group, short)
DECL_GROUP_REDUCE_SCAN(sub_group, ushort)

#endif // defined(cl_khr_subgroup_extended_types)

#if defined(cl_khr_subgroups)
// Any KHR-specific Sub Group Functions will go here.

// TODO: Pipe Built-Ins
// These need to have variadic prototypes, since the packet type could be anything.
//reserve_id_t    __attribute((overloadable)) sub_group_reserve_read_pipe( pipe gentype pipe, uint num_packets);
//reserve_id_t    __attribute((overloadable)) sub_group_reserve_write_pipe( pipe gentype pipe, uint num_packets);
//void    __attribute((overloadable)) sub_group_commit_read_pipe( pipe gentype pipe, reserve_id_t reserve_id );
//void    __attribute((overloadable)) sub_group_commit_write_pipe( pipe gentype pipe, reserve_id_t reserve_id );
#endif // cl_khr_subgroups

#if defined(cl_intel_simd_operations_placeholder) || defined(cl_intel_subgroups) || defined(cl_khr_subgroups)
// Intel Sub Group Functions
float   __attribute__((overloadable)) intel_sub_group_shuffle( float  x, uint c );
float2  __attribute__((overloadable)) intel_sub_group_shuffle( float2 x, uint c );
float3  __attribute__((overloadable)) intel_sub_group_shuffle( float3 x, uint c );
float4  __attribute__((overloadable)) intel_sub_group_shuffle( float4 x, uint c );
float8  __attribute__((overloadable)) intel_sub_group_shuffle( float8 x, uint c );
float16 __attribute__((overloadable)) intel_sub_group_shuffle( float16 x, uint c );

int     __attribute__((overloadable)) intel_sub_group_shuffle( int  x, uint c );
int2    __attribute__((overloadable)) intel_sub_group_shuffle( int2 x, uint c );
int3    __attribute__((overloadable)) intel_sub_group_shuffle( int3 x, uint c );
int4    __attribute__((overloadable)) intel_sub_group_shuffle( int4 x, uint c );
int8    __attribute__((overloadable)) intel_sub_group_shuffle( int8 x, uint c );
int16   __attribute__((overloadable)) intel_sub_group_shuffle( int16 x, uint c );

uint    __attribute__((overloadable)) intel_sub_group_shuffle( uint  x, uint c );
uint2   __attribute__((overloadable)) intel_sub_group_shuffle( uint2 x, uint c );
uint3   __attribute__((overloadable)) intel_sub_group_shuffle( uint3 x, uint c );
uint4   __attribute__((overloadable)) intel_sub_group_shuffle( uint4 x, uint c );
uint8   __attribute__((overloadable)) intel_sub_group_shuffle( uint8 x, uint c );
uint16  __attribute__((overloadable)) intel_sub_group_shuffle( uint16 x, uint c );

long    __attribute__((overloadable)) intel_sub_group_shuffle( long x, uint c );
ulong   __attribute__((overloadable)) intel_sub_group_shuffle( ulong x, uint c );

float   __attribute__((overloadable)) intel_sub_group_shuffle_down( float  cur, float  next, uint c );
float2  __attribute__((overloadable)) intel_sub_group_shuffle_down( float2 cur, float2 next, uint c );
float3  __attribute__((overloadable)) intel_sub_group_shuffle_down( float3 cur, float3 next, uint c );
float4  __attribute__((overloadable)) intel_sub_group_shuffle_down( float4 cur, float4 next, uint c );
float8  __attribute__((overloadable)) intel_sub_group_shuffle_down( float8 cur, float8 next, uint c );
float16 __attribute__((overloadable)) intel_sub_group_shuffle_down( float16 cur, float16 next, uint c );

int     __attribute__((overloadable)) intel_sub_group_shuffle_down( int  cur, int  next, uint c );
int2    __attribute__((overloadable)) intel_sub_group_shuffle_down( int2 cur, int2 next, uint c );
int3    __attribute__((overloadable)) intel_sub_group_shuffle_down( int3 cur, int3 next, uint c );
int4    __attribute__((overloadable)) intel_sub_group_shuffle_down( int4 cur, int4 next, uint c );
int8    __attribute__((overloadable)) intel_sub_group_shuffle_down( int8 cur, int8 next, uint c );
int16   __attribute__((overloadable)) intel_sub_group_shuffle_down( int16 cur, int16 next, uint c );

uint    __attribute__((overloadable)) intel_sub_group_shuffle_down( uint  cur, uint  next, uint c );
uint2   __attribute__((overloadable)) intel_sub_group_shuffle_down( uint2 cur, uint2 next, uint c );
uint3   __attribute__((overloadable)) intel_sub_group_shuffle_down( uint3 cur, uint3 next, uint c );
uint4   __attribute__((overloadable)) intel_sub_group_shuffle_down( uint4 cur, uint4 next, uint c );
uint8   __attribute__((overloadable)) intel_sub_group_shuffle_down( uint8 cur, uint8 next, uint c );
uint16  __attribute__((overloadable)) intel_sub_group_shuffle_down( uint16 cur, uint16 next, uint c );

long    __attribute__((overloadable)) intel_sub_group_shuffle_down( long prev, long cur, uint c );
ulong   __attribute__((overloadable)) intel_sub_group_shuffle_down( ulong prev, ulong cur, uint c );

float   __attribute__((overloadable)) intel_sub_group_shuffle_up( float  prev, float  cur, uint c );
float2  __attribute__((overloadable)) intel_sub_group_shuffle_up( float2 prev, float2 cur, uint c );
float3  __attribute__((overloadable)) intel_sub_group_shuffle_up( float3 prev, float3 cur, uint c );
float4  __attribute__((overloadable)) intel_sub_group_shuffle_up( float4 prev, float4 cur, uint c );
float8  __attribute__((overloadable)) intel_sub_group_shuffle_up( float8 prev, float8 cur, uint c );
float16 __attribute__((overloadable)) intel_sub_group_shuffle_up( float16 prev, float16 cur, uint c );

int     __attribute__((overloadable)) intel_sub_group_shuffle_up( int  prev, int  cur, uint c );
int2    __attribute__((overloadable)) intel_sub_group_shuffle_up( int2 prev, int2 cur, uint c );
int3    __attribute__((overloadable)) intel_sub_group_shuffle_up( int3 prev, int3 cur, uint c );
int4    __attribute__((overloadable)) intel_sub_group_shuffle_up( int4 prev, int4 cur, uint c );
int8    __attribute__((overloadable)) intel_sub_group_shuffle_up( int8 prev, int8 cur, uint c );
int16   __attribute__((overloadable)) intel_sub_group_shuffle_up( int16 prev, int16 cur, uint c );

uint    __attribute__((overloadable)) intel_sub_group_shuffle_up( uint  prev, uint  cur, uint c );
uint2   __attribute__((overloadable)) intel_sub_group_shuffle_up( uint2 prev, uint2 cur, uint c );
uint3   __attribute__((overloadable)) intel_sub_group_shuffle_up( uint3 prev, uint3 cur, uint c );
uint4   __attribute__((overloadable)) intel_sub_group_shuffle_up( uint4 prev, uint4 cur, uint c );
uint8   __attribute__((overloadable)) intel_sub_group_shuffle_up( uint8 prev, uint8 cur, uint c );
uint16  __attribute__((overloadable)) intel_sub_group_shuffle_up( uint16 prev, uint16 cur, uint c );

long    __attribute__((overloadable)) intel_sub_group_shuffle_up( long prev, long cur, uint c );
ulong   __attribute__((overloadable)) intel_sub_group_shuffle_up( ulong prev, ulong cur, uint c );

float   __attribute__((overloadable)) intel_sub_group_shuffle_xor( float  x, uint c );
float2  __attribute__((overloadable)) intel_sub_group_shuffle_xor( float2 x, uint c );
float3  __attribute__((overloadable)) intel_sub_group_shuffle_xor( float3 x, uint c );
float4  __attribute__((overloadable)) intel_sub_group_shuffle_xor( float4 x, uint c );
float8  __attribute__((overloadable)) intel_sub_group_shuffle_xor( float8 x, uint c );
float16 __attribute__((overloadable)) intel_sub_group_shuffle_xor( float16 x, uint c );

int     __attribute__((overloadable)) intel_sub_group_shuffle_xor( int  x, uint c );
int2    __attribute__((overloadable)) intel_sub_group_shuffle_xor( int2 x, uint c );
int3    __attribute__((overloadable)) intel_sub_group_shuffle_xor( int3 x, uint c );
int4    __attribute__((overloadable)) intel_sub_group_shuffle_xor( int4 x, uint c );
int8    __attribute__((overloadable)) intel_sub_group_shuffle_xor( int8 x, uint c );
int16   __attribute__((overloadable)) intel_sub_group_shuffle_xor( int16 x, uint c );

uint    __attribute__((overloadable)) intel_sub_group_shuffle_xor( uint  x, uint c );
uint2   __attribute__((overloadable)) intel_sub_group_shuffle_xor( uint2 x, uint c );
uint3   __attribute__((overloadable)) intel_sub_group_shuffle_xor( uint3 x, uint c );
uint4   __attribute__((overloadable)) intel_sub_group_shuffle_xor( uint4 x, uint c );
uint8   __attribute__((overloadable)) intel_sub_group_shuffle_xor( uint8 x, uint c );
uint16  __attribute__((overloadable)) intel_sub_group_shuffle_xor( uint16 x, uint c );

long    __attribute__((overloadable)) intel_sub_group_shuffle_xor( long x, uint c );
ulong   __attribute__((overloadable)) intel_sub_group_shuffle_xor( ulong x, uint c );

#ifdef __opencl_c_images
uint    __attribute__((overloadable)) intel_sub_group_block_read( read_only image2d_t image, int2 coord );
uint2   __attribute__((overloadable)) intel_sub_group_block_read2( read_only image2d_t image, int2 coord );
uint4   __attribute__((overloadable)) intel_sub_group_block_read4( read_only image2d_t image, int2 coord );
uint8   __attribute__((overloadable)) intel_sub_group_block_read8( read_only image2d_t image, int2 coord );
#endif //__opencl_c_images

uint    __attribute__((overloadable)) intel_sub_group_block_read( const __global uint* p );
uint2   __attribute__((overloadable)) intel_sub_group_block_read2( const __global uint* p );
uint4   __attribute__((overloadable)) intel_sub_group_block_read4( const __global uint* p );
uint8   __attribute__((overloadable)) intel_sub_group_block_read8( const __global uint* p );

#define  intel_sub_group_block_read_ui    intel_sub_group_block_read
#define  intel_sub_group_block_read_ui2   intel_sub_group_block_read2
#define  intel_sub_group_block_read_ui4   intel_sub_group_block_read4
#define  intel_sub_group_block_read_ui8   intel_sub_group_block_read8

#define  intel_sub_group_block_write_ui   intel_sub_group_block_write
#define  intel_sub_group_block_write_ui2  intel_sub_group_block_write2
#define  intel_sub_group_block_write_ui4  intel_sub_group_block_write4
#define  intel_sub_group_block_write_ui8  intel_sub_group_block_write8

#ifdef __opencl_c_images
void    __attribute__((overloadable)) intel_sub_group_block_write( image2d_t image, int2 coord, uint data );
void    __attribute__((overloadable)) intel_sub_group_block_write2( image2d_t image, int2 coord, uint2 data );
void    __attribute__((overloadable)) intel_sub_group_block_write4( image2d_t image, int2 coord, uint4 data );
void    __attribute__((overloadable)) intel_sub_group_block_write8( image2d_t image, int2 coord, uint8 data );

void    __attribute__((overloadable)) intel_sub_group_block_write(write_only image2d_t image, int2 coord, uint data);
void    __attribute__((overloadable)) intel_sub_group_block_write2(write_only image2d_t image, int2 coord, uint2 data);
void    __attribute__((overloadable)) intel_sub_group_block_write4(write_only image2d_t image, int2 coord, uint4 data);
void    __attribute__((overloadable)) intel_sub_group_block_write8(write_only image2d_t image, int2 coord, uint8 data);

#ifdef __opencl_c_read_write_images
uint    __attribute__((overloadable)) intel_sub_group_block_read(read_write image2d_t image, int2 coord);
uint2   __attribute__((overloadable)) intel_sub_group_block_read2(read_write image2d_t image, int2 coord);
uint4   __attribute__((overloadable)) intel_sub_group_block_read4(read_write image2d_t image, int2 coord);
uint8   __attribute__((overloadable)) intel_sub_group_block_read8(read_write image2d_t image, int2 coord);

void    __attribute__((overloadable)) intel_sub_group_block_write(read_write image2d_t image, int2 coord, uint data);
void    __attribute__((overloadable)) intel_sub_group_block_write2(read_write image2d_t image, int2 coord, uint2 data);
void    __attribute__((overloadable)) intel_sub_group_block_write4(read_write image2d_t image, int2 coord, uint4 data);
void    __attribute__((overloadable)) intel_sub_group_block_write8(read_write image2d_t image, int2 coord, uint8 data);
#endif // __opencl_c_read_write_images
#endif //__opencl_c_images

void    __attribute__((overloadable)) intel_sub_group_block_write( __global uint* p, uint data );
void    __attribute__((overloadable)) intel_sub_group_block_write2( __global uint* p, uint2 data );
void    __attribute__((overloadable)) intel_sub_group_block_write4( __global uint* p, uint4 data );
void    __attribute__((overloadable)) intel_sub_group_block_write8( __global uint* p, uint8 data );

#ifdef cl_intel_subgroups_half
#ifdef __opencl_c_images
ushort   __attribute__((overloadable)) intel_sub_group_block_read_half(read_only image2d_t image, int2 coord);
ushort2  __attribute__((overloadable)) intel_sub_group_block_read2_half(read_only image2d_t image, int2 coord);
ushort4  __attribute__((overloadable)) intel_sub_group_block_read4_half(read_only image2d_t image, int2 coord);
ushort8  __attribute__((overloadable)) intel_sub_group_block_read8_half(read_only image2d_t image, int2 coord);
ushort16 __attribute__((overloadable)) intel_sub_group_block_read16_half(read_only image2d_t image, int2 coord);

#ifdef __opencl_c_read_write_images
ushort   __attribute__((overloadable)) intel_sub_group_block_read_half(read_write image2d_t image, int2 coord);
ushort2  __attribute__((overloadable)) intel_sub_group_block_read2_half(read_write image2d_t image, int2 coord);
ushort4  __attribute__((overloadable)) intel_sub_group_block_read4_half(read_write image2d_t image, int2 coord);
ushort8  __attribute__((overloadable)) intel_sub_group_block_read8_half(read_write image2d_t image, int2 coord);
ushort16 __attribute__((overloadable)) intel_sub_group_block_read16_half(read_write image2d_t image, int2 coord);

void    __attribute__((overloadable)) intel_sub_group_block_write(read_write image2d_t image, int2 coord, ushort data);
void    __attribute__((overloadable)) intel_sub_group_block_write2(read_write image2d_t image, int2 coord, ushort2 data);
void    __attribute__((overloadable)) intel_sub_group_block_write4(read_write image2d_t image, int2 coord, ushort4 data);
void    __attribute__((overloadable)) intel_sub_group_block_write8(read_write image2d_t image, int2 coord, ushort8 data);
void    __attribute__((overloadable)) intel_sub_group_block_write16(read_write image2d_t image, int2 coord, ushort16 data);
#endif // __opencl_c_read_write_images
#endif //__opencl_c_images

ushort   __attribute__((overloadable)) intel_sub_group_block_read(const __global ushort* p);
ushort2  __attribute__((overloadable)) intel_sub_group_block_read2(const __global ushort* p);
ushort4  __attribute__((overloadable)) intel_sub_group_block_read4(const __global ushort* p);
ushort8  __attribute__((overloadable)) intel_sub_group_block_read8(const __global ushort* p);
ushort16 __attribute__((overloadable)) intel_sub_group_block_read16(const __global ushort* p);

#ifdef __opencl_c_images
void    __attribute__((overloadable)) intel_sub_group_block_write(image2d_t image, int2 coord, ushort data);
void    __attribute__((overloadable)) intel_sub_group_block_write2(image2d_t image, int2 coord, ushort2 data);
void    __attribute__((overloadable)) intel_sub_group_block_write4(image2d_t image, int2 coord, ushort4 data);
void    __attribute__((overloadable)) intel_sub_group_block_write8(image2d_t image, int2 coord, ushort8 data);
void    __attribute__((overloadable)) intel_sub_group_block_write16(image2d_t image, int2 coord, ushort16 data);
#endif //__opencl_c_images

void    __attribute__((overloadable)) intel_sub_group_block_write(__global ushort* p, ushort data);
void    __attribute__((overloadable)) intel_sub_group_block_write2(__global ushort* p, ushort2 data);
void    __attribute__((overloadable)) intel_sub_group_block_write4(__global ushort* p, ushort4 data);
void    __attribute__((overloadable)) intel_sub_group_block_write8(__global ushort* p, ushort8 data);
#endif // cl_intel_subgroups_half
#ifdef cl_intel_subgroups_short
short    __attribute__((overloadable)) intel_sub_group_broadcast( short  x, uint sub_group_local_id );
short2   __attribute__((overloadable)) intel_sub_group_broadcast( short2 x, uint sub_group_local_id );
short3   __attribute__((overloadable)) intel_sub_group_broadcast( short3 x, uint sub_group_local_id );
short4   __attribute__((overloadable)) intel_sub_group_broadcast( short4 x, uint sub_group_local_id );
short8   __attribute__((overloadable)) intel_sub_group_broadcast( short8 x, uint sub_group_local_id );

ushort   __attribute__((overloadable)) intel_sub_group_broadcast( ushort  x, uint sub_group_local_id );
ushort2  __attribute__((overloadable)) intel_sub_group_broadcast( ushort2 x, uint sub_group_local_id );
ushort3  __attribute__((overloadable)) intel_sub_group_broadcast( ushort3 x, uint sub_group_local_id );
ushort4  __attribute__((overloadable)) intel_sub_group_broadcast( ushort4 x, uint sub_group_local_id );
ushort8  __attribute__((overloadable)) intel_sub_group_broadcast( ushort8 x, uint sub_group_local_id );

short    __attribute__((overloadable)) intel_sub_group_shuffle( short   x, uint c );
short2   __attribute__((overloadable)) intel_sub_group_shuffle( short2  x, uint c );
short3   __attribute__((overloadable)) intel_sub_group_shuffle( short3  x, uint c );
short4   __attribute__((overloadable)) intel_sub_group_shuffle( short4  x, uint c );
short8   __attribute__((overloadable)) intel_sub_group_shuffle( short8  x, uint c );
short16  __attribute__((overloadable)) intel_sub_group_shuffle( short16 x, uint c);

ushort   __attribute__((overloadable)) intel_sub_group_shuffle( ushort   x, uint c );
ushort2  __attribute__((overloadable)) intel_sub_group_shuffle( ushort2  x, uint c );
ushort3  __attribute__((overloadable)) intel_sub_group_shuffle( ushort3  x, uint c );
ushort4  __attribute__((overloadable)) intel_sub_group_shuffle( ushort4  x, uint c );
ushort8  __attribute__((overloadable)) intel_sub_group_shuffle( ushort8  x, uint c );
ushort16 __attribute__((overloadable)) intel_sub_group_shuffle( ushort16 x, uint c );

short    __attribute__((overloadable)) intel_sub_group_shuffle_down( short   cur, short   next, uint c );
short2   __attribute__((overloadable)) intel_sub_group_shuffle_down( short2  cur, short2  next, uint c );
short3   __attribute__((overloadable)) intel_sub_group_shuffle_down( short3  cur, short3  next, uint c );
short4   __attribute__((overloadable)) intel_sub_group_shuffle_down( short4  cur, short4  next, uint c );
short8   __attribute__((overloadable)) intel_sub_group_shuffle_down( short8  cur, short8  next, uint c );
short16  __attribute__((overloadable)) intel_sub_group_shuffle_down( short16 cur, short16 next, uint c );

ushort   __attribute__((overloadable)) intel_sub_group_shuffle_down( ushort   cur, ushort   next, uint c );
ushort2  __attribute__((overloadable)) intel_sub_group_shuffle_down( ushort2  cur, ushort2  next, uint c );
ushort3  __attribute__((overloadable)) intel_sub_group_shuffle_down( ushort3  cur, ushort3  next, uint c );
ushort4  __attribute__((overloadable)) intel_sub_group_shuffle_down( ushort4  cur, ushort4  next, uint c );
ushort8  __attribute__((overloadable)) intel_sub_group_shuffle_down( ushort8  cur, ushort8  next, uint c );
ushort16 __attribute__((overloadable)) intel_sub_group_shuffle_down( ushort16 cur, ushort16 next, uint c );

short    __attribute__((overloadable)) intel_sub_group_shuffle_up( short   cur, short   next, uint c );
short2   __attribute__((overloadable)) intel_sub_group_shuffle_up( short2  cur, short2  next, uint c );
short3   __attribute__((overloadable)) intel_sub_group_shuffle_up( short3  cur, short3  next, uint c );
short4   __attribute__((overloadable)) intel_sub_group_shuffle_up( short4  cur, short4  next, uint c );
short8   __attribute__((overloadable)) intel_sub_group_shuffle_up( short8  cur, short8  next, uint c );
short16  __attribute__((overloadable)) intel_sub_group_shuffle_up( short16 cur, short16 next, uint c );

ushort   __attribute__((overloadable)) intel_sub_group_shuffle_up( ushort   cur, ushort   next, uint c );
ushort2  __attribute__((overloadable)) intel_sub_group_shuffle_up( ushort2  cur, ushort2  next, uint c );
ushort3  __attribute__((overloadable)) intel_sub_group_shuffle_up( ushort3  cur, ushort3  next, uint c );
ushort4  __attribute__((overloadable)) intel_sub_group_shuffle_up( ushort4  cur, ushort4  next, uint c );
ushort8  __attribute__((overloadable)) intel_sub_group_shuffle_up( ushort8  cur, ushort8  next, uint c );
ushort16 __attribute__((overloadable)) intel_sub_group_shuffle_up( ushort16 cur, ushort16 next, uint c );

short    __attribute__((overloadable)) intel_sub_group_shuffle_xor( short   x, uint c );
short2   __attribute__((overloadable)) intel_sub_group_shuffle_xor( short2  x, uint c );
short3   __attribute__((overloadable)) intel_sub_group_shuffle_xor( short3  x, uint c );
short4   __attribute__((overloadable)) intel_sub_group_shuffle_xor( short4  x, uint c );
short8   __attribute__((overloadable)) intel_sub_group_shuffle_xor( short8  x, uint c );
short16  __attribute__((overloadable)) intel_sub_group_shuffle_xor( short16 x, uint c );

ushort   __attribute__((overloadable)) intel_sub_group_shuffle_xor( ushort   x, uint c );
ushort2  __attribute__((overloadable)) intel_sub_group_shuffle_xor( ushort2  x, uint c );
ushort3  __attribute__((overloadable)) intel_sub_group_shuffle_xor( ushort3  x, uint c );
ushort4  __attribute__((overloadable)) intel_sub_group_shuffle_xor( ushort4  x, uint c );
ushort8  __attribute__((overloadable)) intel_sub_group_shuffle_xor( ushort8  x, uint c );
ushort16 __attribute__((overloadable)) intel_sub_group_shuffle_xor( ushort16 x, uint c );

DECL_GROUP_REDUCE_SCAN(intel_sub_group, short)
DECL_GROUP_REDUCE_SCAN(intel_sub_group, ushort)

#ifdef __opencl_c_images
ushort   __attribute__((overloadable)) intel_sub_group_block_read_us( read_only image2d_t image, int2 coord );
ushort2  __attribute__((overloadable)) intel_sub_group_block_read_us2( read_only image2d_t image, int2 coord );
ushort4  __attribute__((overloadable)) intel_sub_group_block_read_us4( read_only image2d_t image, int2 coord );
ushort8  __attribute__((overloadable)) intel_sub_group_block_read_us8( read_only image2d_t image, int2 coord );
ushort16 __attribute__((overloadable)) intel_sub_group_block_read_us16( read_only image2d_t image, int2 coord );

#ifdef __opencl_c_read_write_images
ushort   __attribute__((overloadable)) intel_sub_group_block_read_us(read_write image2d_t image, int2 coord);
ushort2  __attribute__((overloadable)) intel_sub_group_block_read_us2(read_write image2d_t image, int2 coord);
ushort4  __attribute__((overloadable)) intel_sub_group_block_read_us4(read_write image2d_t image, int2 coord);
ushort8  __attribute__((overloadable)) intel_sub_group_block_read_us8(read_write image2d_t image, int2 coord);
ushort16 __attribute__((overloadable)) intel_sub_group_block_read_us16(read_write image2d_t image, int2 coord);

void    __attribute__((overloadable)) intel_sub_group_block_write_us(read_write image2d_t image, int2 coord, ushort  data);
void    __attribute__((overloadable)) intel_sub_group_block_write_us2(read_write image2d_t image, int2 coord, ushort2 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_us4(read_write image2d_t image, int2 coord, ushort4 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_us8(read_write image2d_t image, int2 coord, ushort8 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_us16(read_write image2d_t image, int2 coord, ushort16 data);
#endif // __opencl_c_read_write_images
#endif //__opencl_c_images

ushort    __attribute__((overloadable)) intel_sub_group_block_read_us(  const __global ushort* p );
ushort2   __attribute__((overloadable)) intel_sub_group_block_read_us2( const __global ushort* p );
ushort4   __attribute__((overloadable)) intel_sub_group_block_read_us4( const __global ushort* p );
ushort8   __attribute__((overloadable)) intel_sub_group_block_read_us8( const __global ushort* p );
ushort16  __attribute__((overloadable)) intel_sub_group_block_read_us16( const __global ushort* p );

#ifdef __opencl_c_images
void    __attribute__((overloadable)) intel_sub_group_block_write_us(write_only image2d_t image, int2 coord, ushort  data);
void    __attribute__((overloadable)) intel_sub_group_block_write_us2(write_only image2d_t image, int2 coord, ushort2 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_us4(write_only image2d_t image, int2 coord, ushort4 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_us8(write_only image2d_t image, int2 coord, ushort8 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_us16(write_only image2d_t image, int2 coord, ushort16 data);
#endif //__opencl_c_images

void    __attribute__((overloadable)) intel_sub_group_block_write_us(  __global ushort* p, ushort  data );
void    __attribute__((overloadable)) intel_sub_group_block_write_us2( __global ushort* p, ushort2 data );
void    __attribute__((overloadable)) intel_sub_group_block_write_us4( __global ushort* p, ushort4 data );
void    __attribute__((overloadable)) intel_sub_group_block_write_us8( __global ushort* p, ushort8 data );
void    __attribute__((overloadable)) intel_sub_group_block_write_us16( __global ushort* p, ushort16 data );

#endif // cl_intel_subgroups_short
#ifdef cl_intel_subgroups_char
char    __attribute__((overloadable)) intel_sub_group_broadcast(char  x, uint sub_group_local_id);
char2   __attribute__((overloadable)) intel_sub_group_broadcast(char2 x, uint sub_group_local_id);
char3   __attribute__((overloadable)) intel_sub_group_broadcast(char3 x, uint sub_group_local_id);
char4   __attribute__((overloadable)) intel_sub_group_broadcast(char4 x, uint sub_group_local_id);
char8   __attribute__((overloadable)) intel_sub_group_broadcast(char8 x, uint sub_group_local_id);

uchar   __attribute__((overloadable)) intel_sub_group_broadcast(uchar  x, uint sub_group_local_id);
uchar2  __attribute__((overloadable)) intel_sub_group_broadcast(uchar2 x, uint sub_group_local_id);
uchar3  __attribute__((overloadable)) intel_sub_group_broadcast(uchar3 x, uint sub_group_local_id);
uchar4  __attribute__((overloadable)) intel_sub_group_broadcast(uchar4 x, uint sub_group_local_id);
uchar8  __attribute__((overloadable)) intel_sub_group_broadcast(uchar8 x, uint sub_group_local_id);

char    __attribute__((overloadable)) intel_sub_group_shuffle(char   x, uint c);
char2   __attribute__((overloadable)) intel_sub_group_shuffle(char2  x, uint c);
char3   __attribute__((overloadable)) intel_sub_group_shuffle(char3  x, uint c);
char4   __attribute__((overloadable)) intel_sub_group_shuffle(char4  x, uint c);
char8   __attribute__((overloadable)) intel_sub_group_shuffle(char8  x, uint c);
char16  __attribute__((overloadable)) intel_sub_group_shuffle(char16 x, uint c);

uchar   __attribute__((overloadable)) intel_sub_group_shuffle(uchar   x, uint c);
uchar2  __attribute__((overloadable)) intel_sub_group_shuffle(uchar2  x, uint c);
uchar3  __attribute__((overloadable)) intel_sub_group_shuffle(uchar3  x, uint c);
uchar4  __attribute__((overloadable)) intel_sub_group_shuffle(uchar4  x, uint c);
uchar8  __attribute__((overloadable)) intel_sub_group_shuffle(uchar8  x, uint c);
uchar16 __attribute__((overloadable)) intel_sub_group_shuffle(uchar16 x, uint c);

char    __attribute__((overloadable)) intel_sub_group_shuffle_down(char   cur, char   next, uint c);
char2   __attribute__((overloadable)) intel_sub_group_shuffle_down(char2  cur, char2  next, uint c);
char3   __attribute__((overloadable)) intel_sub_group_shuffle_down(char3  cur, char3  next, uint c);
char4   __attribute__((overloadable)) intel_sub_group_shuffle_down(char4  cur, char4  next, uint c);
char8   __attribute__((overloadable)) intel_sub_group_shuffle_down(char8  cur, char8  next, uint c);
char16  __attribute__((overloadable)) intel_sub_group_shuffle_down(char16 cur, char16 next, uint c);

uchar   __attribute__((overloadable)) intel_sub_group_shuffle_down(uchar   cur, uchar   next, uint c);
uchar2  __attribute__((overloadable)) intel_sub_group_shuffle_down(uchar2  cur, uchar2  next, uint c);
uchar3  __attribute__((overloadable)) intel_sub_group_shuffle_down(uchar3  cur, uchar3  next, uint c);
uchar4  __attribute__((overloadable)) intel_sub_group_shuffle_down(uchar4  cur, uchar4  next, uint c);
uchar8  __attribute__((overloadable)) intel_sub_group_shuffle_down(uchar8  cur, uchar8  next, uint c);
uchar16 __attribute__((overloadable)) intel_sub_group_shuffle_down(uchar16 cur, uchar16 next, uint c);

char    __attribute__((overloadable)) intel_sub_group_shuffle_up(char   cur, char   next, uint c);
char2   __attribute__((overloadable)) intel_sub_group_shuffle_up(char2  cur, char2  next, uint c);
char3   __attribute__((overloadable)) intel_sub_group_shuffle_up(char3  cur, char3  next, uint c);
char4   __attribute__((overloadable)) intel_sub_group_shuffle_up(char4  cur, char4  next, uint c);
char8   __attribute__((overloadable)) intel_sub_group_shuffle_up(char8  cur, char8  next, uint c);
char16  __attribute__((overloadable)) intel_sub_group_shuffle_up(char16 cur, char16 next, uint c);

uchar   __attribute__((overloadable)) intel_sub_group_shuffle_up(uchar   cur, uchar   next, uint c);
uchar2  __attribute__((overloadable)) intel_sub_group_shuffle_up(uchar2  cur, uchar2  next, uint c);
uchar3  __attribute__((overloadable)) intel_sub_group_shuffle_up(uchar3  cur, uchar3  next, uint c);
uchar4  __attribute__((overloadable)) intel_sub_group_shuffle_up(uchar4  cur, uchar4  next, uint c);
uchar8  __attribute__((overloadable)) intel_sub_group_shuffle_up(uchar8  cur, uchar8  next, uint c);
uchar16 __attribute__((overloadable)) intel_sub_group_shuffle_up(uchar16 cur, uchar16 next, uint c);

char    __attribute__((overloadable)) intel_sub_group_shuffle_xor(char   x, uint c);
char2   __attribute__((overloadable)) intel_sub_group_shuffle_xor(char2  x, uint c);
char3   __attribute__((overloadable)) intel_sub_group_shuffle_xor(char3  x, uint c);
char4   __attribute__((overloadable)) intel_sub_group_shuffle_xor(char4  x, uint c);
char8   __attribute__((overloadable)) intel_sub_group_shuffle_xor(char8  x, uint c);
char16  __attribute__((overloadable)) intel_sub_group_shuffle_xor(char16 x, uint c);

uchar   __attribute__((overloadable)) intel_sub_group_shuffle_xor(uchar   x, uint c);
uchar2  __attribute__((overloadable)) intel_sub_group_shuffle_xor(uchar2  x, uint c);
uchar3  __attribute__((overloadable)) intel_sub_group_shuffle_xor(uchar3  x, uint c);
uchar4  __attribute__((overloadable)) intel_sub_group_shuffle_xor(uchar4  x, uint c);
uchar8  __attribute__((overloadable)) intel_sub_group_shuffle_xor(uchar8  x, uint c);
uchar16 __attribute__((overloadable)) intel_sub_group_shuffle_xor(uchar16 x, uint c);

DECL_GROUP_REDUCE_SCAN(intel_sub_group, char)
DECL_GROUP_REDUCE_SCAN(intel_sub_group, uchar)

#ifdef __opencl_c_images
uchar   __attribute__((overloadable)) intel_sub_group_block_read_uc(read_only image2d_t image, int2 coord);
uchar2  __attribute__((overloadable)) intel_sub_group_block_read_uc2(read_only image2d_t image, int2 coord);
uchar4  __attribute__((overloadable)) intel_sub_group_block_read_uc4(read_only image2d_t image, int2 coord);
uchar8  __attribute__((overloadable)) intel_sub_group_block_read_uc8(read_only image2d_t image, int2 coord);
uchar16  __attribute__((overloadable)) intel_sub_group_block_read_uc16(read_only image2d_t image, int2 coord);

#ifdef __opencl_c_read_write_images
uchar   __attribute__((overloadable)) intel_sub_group_block_read_uc(read_write image2d_t image, int2 coord);
uchar2  __attribute__((overloadable)) intel_sub_group_block_read_uc2(read_write image2d_t image, int2 coord);
uchar4  __attribute__((overloadable)) intel_sub_group_block_read_uc4(read_write image2d_t image, int2 coord);
uchar8  __attribute__((overloadable)) intel_sub_group_block_read_uc8(read_write image2d_t image, int2 coord);
uchar16  __attribute__((overloadable)) intel_sub_group_block_read_uc16(read_write image2d_t image, int2 coord);

void    __attribute__((overloadable)) intel_sub_group_block_write_uc(read_write image2d_t image, int2 coord, uchar  data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc2(read_write image2d_t image, int2 coord, uchar2 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc4(read_write image2d_t image, int2 coord, uchar4 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc8(read_write image2d_t image, int2 coord, uchar8 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc16(read_write image2d_t image, int2 coord, uchar16 data);
#endif // __opencl_c_read_write_images
#endif //__opencl_c_images

uchar    __attribute__((overloadable)) intel_sub_group_block_read_uc(const __global uchar* p);
uchar2   __attribute__((overloadable)) intel_sub_group_block_read_uc2(const __global uchar* p);
uchar4   __attribute__((overloadable)) intel_sub_group_block_read_uc4(const __global uchar* p);
uchar8   __attribute__((overloadable)) intel_sub_group_block_read_uc8(const __global uchar* p);
uchar16   __attribute__((overloadable)) intel_sub_group_block_read_uc16(const __global uchar* p);

#ifdef __opencl_c_images
void    __attribute__((overloadable)) intel_sub_group_block_write_uc(write_only image2d_t image, int2 coord, uchar  data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc2(write_only image2d_t image, int2 coord, uchar2 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc4(write_only image2d_t image, int2 coord, uchar4 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc8(write_only image2d_t image, int2 coord, uchar8 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc16(write_only image2d_t image, int2 coord, uchar16 data);
#endif //__opencl_c_images

void    __attribute__((overloadable)) intel_sub_group_block_write_uc(__global uchar* p, uchar  data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc2(__global uchar* p, uchar2 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc4(__global uchar* p, uchar4 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc8(__global uchar* p, uchar8 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc16(__global uchar* p, uchar16 data);

#endif // cl_intel_subgroups_char

#ifdef cl_intel_subgroups_long
#ifdef __opencl_c_images
ulong   __attribute__((overloadable)) intel_sub_group_block_read_ul(read_only image2d_t image, int2 coord);
ulong2  __attribute__((overloadable)) intel_sub_group_block_read_ul2(read_only image2d_t image, int2 coord);
ulong4  __attribute__((overloadable)) intel_sub_group_block_read_ul4(read_only image2d_t image, int2 coord);
ulong8  __attribute__((overloadable)) intel_sub_group_block_read_ul8(read_only image2d_t image, int2 coord);

#ifdef __opencl_c_read_write_images
ulong   __attribute__((overloadable)) intel_sub_group_block_read_ul(read_write image2d_t image, int2 coord);
ulong2  __attribute__((overloadable)) intel_sub_group_block_read_ul2(read_write image2d_t image, int2 coord);
ulong4  __attribute__((overloadable)) intel_sub_group_block_read_ul4(read_write image2d_t image, int2 coord);
ulong8  __attribute__((overloadable)) intel_sub_group_block_read_ul8(read_write image2d_t image, int2 coord);

void    __attribute__((overloadable)) intel_sub_group_block_write_ul(read_write image2d_t image, int2 coord, ulong  data);
void    __attribute__((overloadable)) intel_sub_group_block_write_ul2(read_write image2d_t image, int2 coord, ulong2 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_ul4(read_write image2d_t image, int2 coord, ulong4 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_ul8(read_write image2d_t image, int2 coord, ulong8 data);
#endif // __opencl_c_read_write_images
#endif //__opencl_c_images

ulong    __attribute__((overloadable)) intel_sub_group_block_read_ul(const __global ulong* p);
ulong2   __attribute__((overloadable)) intel_sub_group_block_read_ul2(const __global ulong* p);
ulong4   __attribute__((overloadable)) intel_sub_group_block_read_ul4(const __global ulong* p);
ulong8   __attribute__((overloadable)) intel_sub_group_block_read_ul8(const __global ulong* p);

#ifdef __opencl_c_images
void    __attribute__((overloadable)) intel_sub_group_block_write_ul(write_only image2d_t image, int2 coord, ulong  data);
void    __attribute__((overloadable)) intel_sub_group_block_write_ul2(write_only image2d_t image, int2 coord, ulong2 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_ul4(write_only image2d_t image, int2 coord, ulong4 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_ul8(write_only image2d_t image, int2 coord, ulong8 data);
#endif //__opencl_c_images

void    __attribute__((overloadable)) intel_sub_group_block_write_ul(__global ulong* p, ulong  data);
void    __attribute__((overloadable)) intel_sub_group_block_write_ul2(__global ulong* p, ulong2 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_ul4(__global ulong* p, ulong4 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_ul8(__global ulong* p, ulong8 data);

#endif // cl_intel_subgroups_long


#ifdef cl_intel_subgroup_local_block_io

uint    __attribute__((overloadable)) intel_sub_group_block_read(  const __local uint* p );
uint2   __attribute__((overloadable)) intel_sub_group_block_read2( const __local uint* p );
uint4   __attribute__((overloadable)) intel_sub_group_block_read4( const __local uint* p );
uint8   __attribute__((overloadable)) intel_sub_group_block_read8( const __local uint* p );

void    __attribute__((overloadable)) intel_sub_group_block_write(  __local uint* p, uint data );
void    __attribute__((overloadable)) intel_sub_group_block_write2( __local uint* p, uint2 data );
void    __attribute__((overloadable)) intel_sub_group_block_write4( __local uint* p, uint4 data );
void    __attribute__((overloadable)) intel_sub_group_block_write8( __local uint* p, uint8 data );

ushort    __attribute__((overloadable)) intel_sub_group_block_read_us(  const __local ushort* p );
ushort2   __attribute__((overloadable)) intel_sub_group_block_read_us2( const __local ushort* p );
ushort4   __attribute__((overloadable)) intel_sub_group_block_read_us4( const __local ushort* p );
ushort8   __attribute__((overloadable)) intel_sub_group_block_read_us8( const __local ushort* p );

void    __attribute__((overloadable)) intel_sub_group_block_write_us(  __local ushort* p, ushort  data );
void    __attribute__((overloadable)) intel_sub_group_block_write_us2( __local ushort* p, ushort2 data );
void    __attribute__((overloadable)) intel_sub_group_block_write_us4( __local ushort* p, ushort4 data );
void    __attribute__((overloadable)) intel_sub_group_block_write_us8( __local ushort* p, ushort8 data );

#ifdef cl_intel_subgroups_char
uchar    __attribute__((overloadable)) intel_sub_group_block_read_uc(  const __local uchar* p);
uchar2   __attribute__((overloadable)) intel_sub_group_block_read_uc2( const __local uchar* p);
uchar4   __attribute__((overloadable)) intel_sub_group_block_read_uc4( const __local uchar* p);
uchar8   __attribute__((overloadable)) intel_sub_group_block_read_uc8( const __local uchar* p);
uchar16  __attribute__((overloadable)) intel_sub_group_block_read_uc16(const __local uchar* p);

void    __attribute__((overloadable)) intel_sub_group_block_write_uc(  __local uchar* p, uchar  data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc2( __local uchar* p, uchar2 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc4( __local uchar* p, uchar4 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc8( __local uchar* p, uchar8 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_uc16(__local uchar* p, uchar16 data);
#endif // cl_intel_subgroups_char

#ifdef cl_intel_subgroups_long
ulong    __attribute__((overloadable)) intel_sub_group_block_read_ul( const __local ulong* p);
ulong2   __attribute__((overloadable)) intel_sub_group_block_read_ul2(const __local ulong* p);
ulong4   __attribute__((overloadable)) intel_sub_group_block_read_ul4(const __local ulong* p);
ulong8   __attribute__((overloadable)) intel_sub_group_block_read_ul8(const __local ulong* p);

void    __attribute__((overloadable)) intel_sub_group_block_write_ul( __local ulong* p, ulong  data);
void    __attribute__((overloadable)) intel_sub_group_block_write_ul2(__local ulong* p, ulong2 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_ul4(__local ulong* p, ulong4 data);
void    __attribute__((overloadable)) intel_sub_group_block_write_ul8(__local ulong* p, ulong8 data);
#endif // cl_intel_subgroups_long

#endif // cl_intel_subgroup_local_block_io

#if defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)
#define DECL_SUB_GROUP_NON_UNIFORM_OPERATION(TYPE, GROUP_TYPE, OPERATION)
#define DECL_SUB_GROUP_NON_UNIFORM_CLUSTERED_OPERATION(TYPE, GROUP_TYPE, OPERATION)

#if defined(cl_khr_subgroup_non_uniform_arithmetic)
#define DECL_SUB_GROUP_NON_UNIFORM_OPERATION(TYPE, GROUP_TYPE, OPERATION) \
TYPE    __attribute__((overloadable)) sub_group_non_uniform_##GROUP_TYPE##_##OPERATION(TYPE value);
#endif // defined(cl_khr_subgroup_non_uniform_arithmetic)

#if defined(cl_khr_subgroup_clustered_reduce)
#define DECL_SUB_GROUP_NON_UNIFORM_CLUSTERED_OPERATION(TYPE, GROUP_TYPE, OPERATION) \
TYPE    __attribute__((overloadable)) sub_group_clustered_##GROUP_TYPE##_##OPERATION(TYPE value, uint clustersize);
#endif // defined(cl_khr_subgroup_clustered_reduce)

#define DECL_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(TYPE, OPERATION)           \
DECL_SUB_GROUP_NON_UNIFORM_OPERATION(TYPE, reduce, OPERATION)            \
DECL_SUB_GROUP_NON_UNIFORM_OPERATION(TYPE, scan_inclusive, OPERATION)    \
DECL_SUB_GROUP_NON_UNIFORM_OPERATION(TYPE, scan_exclusive, OPERATION)    \
DECL_SUB_GROUP_NON_UNIFORM_CLUSTERED_OPERATION(TYPE, reduce, OPERATION)

// ARITHMETIC OPERATIONS
// gentype sub_group_non_uniform_GROUP_TYPE_add(gentype value)
// gentype sub_group_non_uniform_GROUP_TYPE_min(gentype value)
// gentype sub_group_non_uniform_GROUP_TYPE_max(gentype value)
// gentype sub_group_non_uniform_GROUP_TYPE_mul(gentype value)
#define DECL_SUB_GROUP_ARITHMETIC_OPERATIONS(TYPE)   \
DECL_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(TYPE, add)     \
DECL_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(TYPE, min)     \
DECL_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(TYPE, max)     \
DECL_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(TYPE, mul)

DECL_SUB_GROUP_ARITHMETIC_OPERATIONS(char)
DECL_SUB_GROUP_ARITHMETIC_OPERATIONS(uchar)
DECL_SUB_GROUP_ARITHMETIC_OPERATIONS(short)
DECL_SUB_GROUP_ARITHMETIC_OPERATIONS(ushort)
DECL_SUB_GROUP_ARITHMETIC_OPERATIONS(int)
DECL_SUB_GROUP_ARITHMETIC_OPERATIONS(uint)
DECL_SUB_GROUP_ARITHMETIC_OPERATIONS(long)
DECL_SUB_GROUP_ARITHMETIC_OPERATIONS(ulong)
DECL_SUB_GROUP_ARITHMETIC_OPERATIONS(float)
#if defined(cl_khr_fp64)
DECL_SUB_GROUP_ARITHMETIC_OPERATIONS(double)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DECL_SUB_GROUP_ARITHMETIC_OPERATIONS(half)
#endif // defined(cl_khr_fp16)

// BITWISE OPERATIONS
// gentype sub_group_non_uniform_GROUP_TYPE_and(gentype value)
// gentype sub_group_non_uniform_GROUP_TYPE_or(gentype value)
// gentype sub_group_non_uniform_GROUP_TYPE_xor(gentype value)
#define DECL_SUB_GROUP_BITWISE_OPERATIONS(TYPE)    \
DECL_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(TYPE, and)   \
DECL_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(TYPE, or)    \
DECL_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(TYPE, xor)

DECL_SUB_GROUP_BITWISE_OPERATIONS(char)
DECL_SUB_GROUP_BITWISE_OPERATIONS(uchar)
DECL_SUB_GROUP_BITWISE_OPERATIONS(short)
DECL_SUB_GROUP_BITWISE_OPERATIONS(ushort)
DECL_SUB_GROUP_BITWISE_OPERATIONS(int)
DECL_SUB_GROUP_BITWISE_OPERATIONS(uint)
DECL_SUB_GROUP_BITWISE_OPERATIONS(long)
DECL_SUB_GROUP_BITWISE_OPERATIONS(ulong)

// LOGICAL OPERATIONS
// int sub_group_non_uniform_GROUP_TYPE_logical_and(int predicate)
// int sub_group_non_uniform_GROUP_TYPE_logical_or(int predicate)
// int sub_group_non_uniform_GROUP_TYPE_logical_xor(int predicate)
#define DECL_SUB_GROUP_BITWISE_OPERATIONS(TYPE)             \
DECL_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(TYPE, logical_and)    \
DECL_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(TYPE, logical_or)     \
DECL_SUB_GROUP_NON_UNIFORM_ALL_GROUPS(TYPE, logical_xor)

DECL_SUB_GROUP_BITWISE_OPERATIONS(int)

#endif // defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)

#ifdef cl_intel_media_block_io

// Media Block read/write extension

//read
#ifdef __opencl_c_images
uchar __attribute__((overloadable)) intel_sub_group_media_block_read_uc(int2 src_offset, int width, int height, read_only image2d_t image);
uchar2 __attribute__((overloadable)) intel_sub_group_media_block_read_uc2(int2 src_offset, int width, int height, read_only image2d_t image);
uchar4 __attribute__((overloadable)) intel_sub_group_media_block_read_uc4(int2 src_offset, int width, int height, read_only image2d_t image);
uchar8 __attribute__((overloadable)) intel_sub_group_media_block_read_uc8(int2 src_offset, int width, int height, read_only image2d_t image);
uchar16 __attribute__((overloadable)) intel_sub_group_media_block_read_uc16(int2 src_offset, int width, int height, read_only image2d_t image);

ushort __attribute__((overloadable)) intel_sub_group_media_block_read_us(int2 src_offset, int width, int height, read_only image2d_t image);
ushort2 __attribute__((overloadable)) intel_sub_group_media_block_read_us2(int2 src_offset, int width, int height, read_only image2d_t image);
ushort4 __attribute__((overloadable)) intel_sub_group_media_block_read_us4(int2 src_offset, int width, int height, read_only image2d_t image);
ushort8 __attribute__((overloadable)) intel_sub_group_media_block_read_us8(int2 src_offset, int width, int height, read_only image2d_t image);
ushort16 __attribute__((overloadable)) intel_sub_group_media_block_read_us16(int2 src_offset, int width, int height, read_only image2d_t image);

uint __attribute__((overloadable)) intel_sub_group_media_block_read_ui(int2 src_offset, int width, int height, read_only image2d_t image);
uint2 __attribute__((overloadable)) intel_sub_group_media_block_read_ui2(int2 src_offset, int width, int height, read_only image2d_t image);
uint4 __attribute__((overloadable)) intel_sub_group_media_block_read_ui4(int2 src_offset, int width, int height, read_only image2d_t image);
uint8 __attribute__((overloadable)) intel_sub_group_media_block_read_ui8(int2 src_offset, int width, int height, read_only image2d_t image);

#ifdef __opencl_c_read_write_images
uchar __attribute__((overloadable)) intel_sub_group_media_block_read_uc(int2 src_offset, int width, int height, read_write image2d_t image);
uchar2 __attribute__((overloadable)) intel_sub_group_media_block_read_uc2(int2 src_offset, int width, int height, read_write image2d_t image);
uchar4 __attribute__((overloadable)) intel_sub_group_media_block_read_uc4(int2 src_offset, int width, int height, read_write image2d_t image);
uchar8 __attribute__((overloadable)) intel_sub_group_media_block_read_uc8(int2 src_offset, int width, int height, read_write image2d_t image);
uchar16 __attribute__((overloadable)) intel_sub_group_media_block_read_uc16(int2 src_offset, int width, int height, read_write image2d_t image);

ushort __attribute__((overloadable)) intel_sub_group_media_block_read_us(int2 src_offset, int width, int height, read_write image2d_t image);
ushort2 __attribute__((overloadable)) intel_sub_group_media_block_read_us2(int2 src_offset, int width, int height, read_write image2d_t image);
ushort4 __attribute__((overloadable)) intel_sub_group_media_block_read_us4(int2 src_offset, int width, int height, read_write image2d_t image);
ushort8 __attribute__((overloadable)) intel_sub_group_media_block_read_us8(int2 src_offset, int width, int height, read_write image2d_t image);
ushort16 __attribute__((overloadable)) intel_sub_group_media_block_read_us16(int2 src_offset, int width, int height, read_write image2d_t image);

uint __attribute__((overloadable)) intel_sub_group_media_block_read_ui(int2 src_offset, int width, int height, read_write image2d_t image);
uint2 __attribute__((overloadable)) intel_sub_group_media_block_read_ui2(int2 src_offset, int width, int height, read_write image2d_t image);
uint4 __attribute__((overloadable)) intel_sub_group_media_block_read_ui4(int2 src_offset, int width, int height, read_write image2d_t image);
uint8 __attribute__((overloadable)) intel_sub_group_media_block_read_ui8(int2 src_offset, int width, int height, read_write image2d_t image);
#endif // __opencl_c_read_write_images

// write

void __attribute__((overloadable)) intel_sub_group_media_block_write_uc(int2 src_offset, int width, int height, uchar pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc2(int2 src_offset, int width, int height, uchar2 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc4(int2 src_offset, int width, int height, uchar4 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc8(int2 src_offset, int width, int height, uchar8 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc16(int2 src_offset, int width, int height, uchar16 pixels, write_only image2d_t image);

void __attribute__((overloadable)) intel_sub_group_media_block_write_us(int2 src_offset, int width, int height, ushort pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us2(int2 src_offset, int width, int height, ushort2 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us4(int2 src_offset, int width, int height, ushort4 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us8(int2 src_offset, int width, int height, ushort8 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us16(int2 src_offset, int width, int height, ushort16 pixels, write_only image2d_t image);

void __attribute__((overloadable)) intel_sub_group_media_block_write_ui(int2 src_offset, int width, int height, uint pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui2(int2 src_offset, int width, int height, uint2 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui4(int2 src_offset, int width, int height, uint4 pixels, write_only image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui8(int2 src_offset, int width, int height, uint8 pixels, write_only image2d_t image);

#ifdef __opencl_c_read_write_images
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc(int2 src_offset, int width, int height, uchar pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc2(int2 src_offset, int width, int height, uchar2 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc4(int2 src_offset, int width, int height, uchar4 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc8(int2 src_offset, int width, int height, uchar8 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_uc16(int2 src_offset, int width, int height, uchar16 pixels, read_write image2d_t image);

void __attribute__((overloadable)) intel_sub_group_media_block_write_us(int2 src_offset, int width, int height, ushort pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us2(int2 src_offset, int width, int height, ushort2 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us4(int2 src_offset, int width, int height, ushort4 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us8(int2 src_offset, int width, int height, ushort8 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_us16(int2 src_offset, int width, int height, ushort16 pixels, read_write image2d_t image);

void __attribute__((overloadable)) intel_sub_group_media_block_write_ui(int2 src_offset, int width, int height, uint pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui2(int2 src_offset, int width, int height, uint2 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui4(int2 src_offset, int width, int height, uint4 pixels, read_write image2d_t image);
void __attribute__((overloadable)) intel_sub_group_media_block_write_ui8(int2 src_offset, int width, int height, uint8 pixels, read_write image2d_t image);
#endif // __opencl_c_read_write_images
#endif //__opencl_c_images

#endif // cl_intel_media_block_io

#ifdef cl_khr_fp16
half    __attribute__((overloadable)) intel_sub_group_shuffle( half x, uint c );
half    __attribute__((overloadable)) intel_sub_group_shuffle_down( half prev, half cur, uint c );
half    __attribute__((overloadable)) intel_sub_group_shuffle_up( half prev, half cur, uint c );
half    __attribute__((overloadable)) intel_sub_group_shuffle_xor( half x, uint c );
#endif

#if defined(cl_khr_fp64)
double  __attribute__((overloadable)) intel_sub_group_shuffle( double x, uint c );
double  __attribute__((overloadable)) intel_sub_group_shuffle_down( double prev, double cur, uint c );
double  __attribute__((overloadable)) intel_sub_group_shuffle_up( double prev, double cur, uint c );
double  __attribute__((overloadable)) intel_sub_group_shuffle_xor( double x, uint c );
#endif

#endif // defined(cl_intel_simd_operations_placeholder) || defined(cl_intel_subgroups) || defined(cl_khr_subgroups)

#if defined(cl_khr_subgroup_shuffle)
#define DECL_SUB_GROUP_SHUFFLE(TYPE)                                                  \
TYPE    __attribute__((overloadable)) sub_group_shuffle(TYPE value, uint index);      \
TYPE    __attribute__((overloadable)) sub_group_shuffle_xor(TYPE value, uint mask);

DECL_SUB_GROUP_SHUFFLE(char)
DECL_SUB_GROUP_SHUFFLE(uchar)
DECL_SUB_GROUP_SHUFFLE(int)
DECL_SUB_GROUP_SHUFFLE(uint)
DECL_SUB_GROUP_SHUFFLE(long)
DECL_SUB_GROUP_SHUFFLE(ulong)
DECL_SUB_GROUP_SHUFFLE(float)
#if defined(cl_khr_fp64)
DECL_SUB_GROUP_SHUFFLE(double)
#endif // defined(cl_khr_fp64)
#if defined (cl_khr_fp16)
DECL_SUB_GROUP_SHUFFLE(half)
#endif // defined (cl_khr_fp16)
#endif // defined(cl_khr_subgroup_shuffle)

#if defined(cl_khr_subgroup_shuffle_relative)
#define DECL_SUB_GROUP_SHUFFLE_RELATIVE(TYPE)                                          \
TYPE    __attribute__((overloadable)) sub_group_shuffle_up(TYPE value, uint delta);    \
TYPE    __attribute__((overloadable)) sub_group_shuffle_down(TYPE value, uint delta);

DECL_SUB_GROUP_SHUFFLE_RELATIVE(char)
DECL_SUB_GROUP_SHUFFLE_RELATIVE(uchar)
DECL_SUB_GROUP_SHUFFLE_RELATIVE(int)
DECL_SUB_GROUP_SHUFFLE_RELATIVE(uint)
DECL_SUB_GROUP_SHUFFLE_RELATIVE(long)
DECL_SUB_GROUP_SHUFFLE_RELATIVE(ulong)
DECL_SUB_GROUP_SHUFFLE_RELATIVE(float)
#if defined(cl_khr_fp64)
DECL_SUB_GROUP_SHUFFLE_RELATIVE(double)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DECL_SUB_GROUP_SHUFFLE_RELATIVE(half)
#endif // defined (cl_khr_fp16)
#endif // defined(cl_khr_subgroup_shuffle_relative)

#if defined(cl_intel_simd_operations_placeholder)
// SIMD Operations

#define intel_get_simd_size             get_max_sub_group_size
#define intel_get_simd_id               get_sub_group_local_id

#define intel_simd_shuffle              intel_sub_group_shuffle

#define intel_simd_shuffle_down         intel_sub_group_shuffle_down

#define intel_simd_shuffle_up           intel_sub_group_shuffle_up

#define intel_simd_media_block_read     intel_sub_group_block_read
#define intel_simd_media_block_read2    intel_sub_group_block_read2
#define intel_simd_media_block_read4    intel_sub_group_block_read4
#define intel_simd_media_block_read8    intel_sub_group_block_read8

#define intel_simd_block_read           intel_sub_group_block_read
#define intel_simd_block_read2          intel_sub_group_block_read2
#define intel_simd_block_read4          intel_sub_group_block_read4
#define intel_simd_block_read8          intel_sub_group_block_read8

#define intel_simd_media_block_write    intel_sub_group_block_write
#define intel_simd_media_block_write2   intel_sub_group_block_write2
#define intel_simd_media_block_write4   intel_sub_group_block_write4
#define intel_simd_media_block_write8   intel_sub_group_block_write8

#define intel_simd_block_write          intel_sub_group_block_write
#define intel_simd_block_write2         intel_sub_group_block_write2
#define intel_simd_block_write4         intel_sub_group_block_write4
#define intel_simd_block_write8         intel_sub_group_block_write8

#endif

#if defined(cl_intel_subgroup_half2_placeholder)
uint __attribute__((overloadable)) intel_sub_group_half2_add( uint a, uint b );
uint __attribute__((overloadable)) intel_sub_group_half2_sub( uint a, uint b );
uint __attribute__((overloadable)) intel_sub_group_half2_mul( uint a, uint b );
uint __attribute__((overloadable)) intel_sub_group_half2_mad( uint a, uint b, uint c );

short2 __attribute__((overloadable)) intel_sub_group_half2_isequal( uint a, uint b );
short2 __attribute__((overloadable)) intel_sub_group_half2_isnotequal( uint a, uint b );
short2 __attribute__((overloadable)) intel_sub_group_half2_isgreater( uint a, uint b );
short2 __attribute__((overloadable)) intel_sub_group_half2_isgreaterequal( uint a, uint b );
short2 __attribute__((overloadable)) intel_sub_group_half2_isless( uint a, uint b );
short2 __attribute__((overloadable)) intel_sub_group_half2_islessequal( uint a, uint b );
#endif

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
uint __attribute__((overloadable)) intel_get_accelerator_mb_block_type(
    sampler_t a );
uint __attribute__((overloadable)) intel_get_accelerator_mb_sad_sdjust_mode(
    sampler_t a );
uint __attribute__((overloadable)) intel_get_accelerator_mb_sub_pixel_mode(
    sampler_t a );
uint __attribute__((overloadable)) intel_get_accelerator_mb_search_path_type(
    sampler_t a );

#ifdef cl_khr_integer_dot_product

uint __attribute__((overloadable)) dot(ushort2, ushort2);
int __attribute__((overloadable)) dot(ushort2, short2);
int __attribute__((overloadable)) dot(short2, ushort2);
int __attribute__((overloadable)) dot(short2, short2);

uint __attribute__((overloadable)) dot(uchar4, uchar4);
int __attribute__((overloadable)) dot(char4, char4);
int __attribute__((overloadable)) dot(char4, uchar4);
int __attribute__((overloadable)) dot(uchar4, char4);

uint __attribute__((overloadable)) dot_4x8packed_uu_uint(uint, uint);
int __attribute__((overloadable)) dot_4x8packed_ss_int(uint, uint);
int __attribute__((overloadable)) dot_4x8packed_us_int(uint, uint);
int __attribute__((overloadable)) dot_4x8packed_su_int(uint, uint);

uint __attribute__((overloadable)) dot_acc_sat(uchar4, uchar4, uint);
int __attribute__((overloadable)) dot_acc_sat(char4, char4, int);
int __attribute__((overloadable)) dot_acc_sat(char4, uchar4, int);
int __attribute__((overloadable)) dot_acc_sat(uchar4, char4, int);

uint __attribute__((overloadable)) dot_acc_sat(ushort2, ushort2, uint);
int __attribute__((overloadable)) dot_acc_sat(short2, short2, int);
int __attribute__((overloadable)) dot_acc_sat(short2, ushort2, int);
int __attribute__((overloadable)) dot_acc_sat(ushort2, short2, int);

uint __attribute__((overloadable)) dot_acc_sat_4x8packed_uu_uint(uint, uint, uint);
int __attribute__((overloadable)) dot_acc_sat_4x8packed_ss_int(uint, uint, int);
int __attribute__((overloadable)) dot_acc_sat_4x8packed_us_int(uint, uint, int);
int __attribute__((overloadable)) dot_acc_sat_4x8packed_su_int(uint, uint, int);

#endif

#ifdef __opencl_c_images
void __attribute__((overloadable)) intel_work_group_vme_mb_query(
    __local uint* dst,
    int2 srcCoord,
    int2 refCoord,
    image2d_t srcImage,
    image2d_t refImage,
    sampler_t a );
#endif //__opencl_c_images

#endif

// added to fix build issue with clang separation. TODO: figure out why this did not carry over from OpenCL/Frontend/Languages in the first place
#ifdef cl_intel_device_side_advanced_vme_enable
// Advanced VME and VME Accelerators extension
uint __attribute__((overloadable)) intel_get_accelerator_mb_search_block_type(
    sampler_t a );

uint __attribute__((overloadable)) intel_get_accelerator_mb_skip_block_type(
    sampler_t a );

#ifdef __opencl_c_images
void __attribute__((overloadable)) intel_work_group_vme_mb_multi_query_8(
    __local uint* dst,
    uint countWGRefCoords,
    uint searchCostPenalty,
    uint2 searchCostTable,
    int2 srcCoord,
    int2 refCoord,
    image2d_t srcImage,
    image2d_t refImage,
    sampler_t a );

void __attribute__((overloadable)) intel_work_group_vme_mb_multi_query_4(
    __local uint* dst,
    uint countWGRefCoords,
    uint searchCostPenalty,
    uint2 searchCostTable,
    int2 srcCoord,
    int2 refCoord,
    image2d_t srcImage,
    image2d_t refImage,
    sampler_t a);

void __attribute__((overloadable)) intel_work_group_vme_mb_multi_check_16x16(
    __local uint* dst,
    uint countWGSkipCoords,
    uint computeIntra,
    uint edgesIntra,
    int2 srcCoord,
    int  skipCoord,
    image2d_t srcImage,
    image2d_t refImage,
    image2d_t edgeSrcImage,
    sampler_t a );

void __attribute__((overloadable)) intel_work_group_vme_mb_multi_bidir_check_16x16(
    __local uint* dst,
    uint countWGSkipCoords,
    uint computeIntra,
    uint edgesIntra,
    int2 srcCoord,
    uchar bidir_weight,
    uchar skipModes,
    int  skipCoord,
    image2d_t srcImage,
    image2d_t refFwdImage,
    image2d_t refBwdImage,
    image2d_t edgeSrcImage,
    sampler_t vmeAccelerator);

void __attribute__((overloadable)) intel_work_group_vme_mb_multi_check_8x8(
    __local uint* dst,
    uint countWGSkipCoords,
    uint computeIntra,
    uint edgesIntra,
    int2 srcCoord,
    int4 skipCoord,
    image2d_t srcImage,
    image2d_t refImage,
    image2d_t edgeSrcImage,
    sampler_t a );

void __attribute__((overloadable)) intel_work_group_vme_mb_multi_bidir_check_8x8(
    __local uint* dst,
    uint countWGSkipCoords,
    uint computeIntra,
    uint edgesIntra,
    int2 srcCoord,
    uchar bidir_weight,
    uchar skipModes,
    int2 skipCoord,
    image2d_t srcImage,
    image2d_t refFwdImage,
    image2d_t refBwdImage,
    image2d_t edgeSrcImage,
    sampler_t vmeAccelerator);
#endif //__opencl_c_images

#endif
// end of build workaround for clang separation
// Disable any extensions we may have enabled previously.
#pragma OPENCL EXTENSION all : disable

#if defined(cl_khr_fp64) && ( __OPENCL_C_VERSION__ >= CL_VERSION_1_2 )
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#endif

// Clang requires this pragma to be enabled if subgroup functions are to be used.
// Not all tests follow this requirement, leave it enabled for transition period until they are fixed.
#if defined(cl_khr_subgroups)
#pragma OPENCL EXTENSION cl_khr_subgroups : enable
#endif


#endif // #ifndef _OPENCL_CTH_

#ifdef cl_intel_split_work_group_barrier
void __attribute__((overloadable)) intel_work_group_barrier_arrive(cl_mem_fence_flags flags);
void __attribute__((overloadable)) intel_work_group_barrier_wait(cl_mem_fence_flags flags);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void __attribute__((overloadable)) intel_work_group_barrier_arrive(cl_mem_fence_flags flags, memory_scope scope);
void __attribute__((overloadable)) intel_work_group_barrier_wait(cl_mem_fence_flags flags, memory_scope scope);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // cl_intel_split_work_group_barrier

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

typedef enum
{
    intel_raytracing_ext_flag_ray_query = 1 << 0,   // true if ray queries are supported
} intel_raytracing_ext_flag_t;

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

// check supported ray tracing features
intel_raytracing_ext_flag_t intel_get_raytracing_ext_flag();

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
