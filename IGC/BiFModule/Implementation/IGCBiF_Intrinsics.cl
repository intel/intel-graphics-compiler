/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IBiF_Header.cl"

#ifndef IGCBIF_INTRINSICS_CL
#define IGCBIF_INTRINSICS_CL

#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

// Helper intrinsics for casting objects of builtin types
void* __attribute__((overloadable)) __builtin_IB_cast_object_to_generic_ptr(void* object);

#define DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(OBJ_TYPE) \
    void* __attribute__((overloadable)) __builtin_IB_cast_object_to_generic_ptr(OBJ_TYPE object);

#define DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_WITH_AQ(ACCESS_QUAL, OBJ_TYPE) \
    void* __attribute__((overloadable)) __builtin_IB_cast_object_to_generic_ptr(ACCESS_QUAL OBJ_TYPE object);

DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(clk_event_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(queue_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(sampler_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(read_only pipe int)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(write_only pipe int)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(private struct __spirv_AvcImeResultINTEL*)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(intel_sub_group_avc_ime_payload_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(private struct __spirv_AvcImeResultSingleReferenceStreamoutINTEL*)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(private struct __spirv_AvcImeResultDualReferenceStreamoutINTEL*)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(private struct __spirv_AvcImeSingleReferenceStreaminINTEL*)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(private struct __spirv_AvcImeDualReferenceStreaminINTEL*)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(private struct __spirv_AvcMceResultINTEL*)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(private struct __spirv_AvcImePayloadINTEL*)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(private struct __spirv_AvcRefPayloadINTEL*)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(private struct __spirv_AvcMcePayloadINTEL*)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(private struct __spirv_AvcRefResultINTEL*)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(private struct __spirv_AvcSicPayloadINTEL*)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(private struct __spirv_AvcSicResultINTEL*)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(intel_sub_group_avc_ime_single_reference_streamin_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(intel_sub_group_avc_ime_dual_reference_streamin_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(intel_sub_group_avc_ime_result_dual_reference_streamout_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(intel_sub_group_avc_ime_result_single_reference_streamout_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(intel_sub_group_avc_ime_result_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(intel_sub_group_avc_ref_payload_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(intel_sub_group_avc_ref_result_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(intel_sub_group_avc_sic_payload_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(intel_sub_group_avc_sic_result_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(intel_sub_group_avc_mce_payload_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(intel_sub_group_avc_mce_result_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(event_t)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR(reserve_id_t)


#define DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_FOR_AQ(ACCESS_QUAL)                          \
    DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_WITH_AQ(ACCESS_QUAL, image1d_t)                  \
    DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_WITH_AQ(ACCESS_QUAL, image2d_t)                  \
    DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_WITH_AQ(ACCESS_QUAL, image3d_t)                  \
    DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_WITH_AQ(ACCESS_QUAL, image1d_buffer_t)           \
    DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_WITH_AQ(ACCESS_QUAL, image1d_array_t)            \
    DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_WITH_AQ(ACCESS_QUAL, image2d_array_t)            \
    DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_WITH_AQ(ACCESS_QUAL, image2d_depth_t)            \
    DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_WITH_AQ(ACCESS_QUAL, image2d_msaa_t)             \
    DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_WITH_AQ(ACCESS_QUAL, image2d_msaa_depth_t)       \
    DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_WITH_AQ(ACCESS_QUAL, image2d_array_depth_t)      \
    DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_WITH_AQ(ACCESS_QUAL, image2d_array_msaa_t)       \
    DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_WITH_AQ(ACCESS_QUAL, image2d_array_msaa_depth_t)

DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_FOR_AQ(read_only)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_FOR_AQ(read_write)
DECL_IB_CAST_OBJECT_TO_GENERIC_PTR_FOR_AQ(write_only)


clk_event_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_clk_event(void* object);
constant struct __spirv_Sampler* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_sampler(void* object);
event_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_event(void* object);
generic clk_event_t* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_clk_event_ptr(void* object);
global struct __spirv_Image__void_1_0_0_0_0_0_0* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_image(void* object);
global struct __spirv_Pipe__0* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_pipe_ro(void* object);
global struct __spirv_Pipe__1* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_pipe_wo(void* object);
intel_sub_group_avc_ime_dual_reference_streamin_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_intel_sub_group_avc_ime_dual_reference_streamin_t(void* object);
intel_sub_group_avc_ime_payload_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_intel_sub_group_avc_ime_payload(void* object);
intel_sub_group_avc_ime_result_dual_reference_streamout_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_intel_sub_group_avc_ime_result_dual_reference_streamout_t(void* object);
intel_sub_group_avc_ime_result_single_reference_streamout_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_intel_sub_group_avc_ime_result_single_reference_streamout(void* object);
intel_sub_group_avc_ime_result_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_intel_sub_group_avc_ime_result(void* object);
intel_sub_group_avc_ime_single_reference_streamin_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_intel_sub_group_avc_ime_single_reference_streamin_t(void* object);
intel_sub_group_avc_mce_payload_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_intel_sub_group_avc_mce_payload(void* object);
intel_sub_group_avc_mce_result_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_intel_sub_group_avc_mce_result_t(void* object);
intel_sub_group_avc_ref_payload_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_intel_sub_group_avc_ref_payload_t(void* object);
intel_sub_group_avc_ref_result_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_intel_sub_group_avc_ref_result_t(void* object);
intel_sub_group_avc_sic_payload_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_intel_sub_group_avc_sic_payload_t(void* object);
intel_sub_group_avc_sic_result_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_intel_sub_group_avc_sic_result_t(void* object);
private struct __spirv_AvcImeDualReferenceStreaminINTEL* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_AvcImeDualReferenceStreaminINTEL(void* object);
private struct __spirv_AvcImePayloadINTEL* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_AvcImePayloadINTEL(void* object);
private struct __spirv_AvcImeResultDualReferenceStreamoutINTEL* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_AvcImeResultDualReferenceStreamoutINTEL(void* object);
private struct __spirv_AvcImeResultINTEL* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_AvcImeResultINTEL(void* object);
private struct __spirv_AvcImeResultSingleReferenceStreamoutINTEL* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_AvcImeResultSingleReferenceStreamoutINTEL(void* object);
private struct __spirv_AvcImeSingleReferenceStreaminINTEL* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_AvcImeSingleReferenceStreaminINTEL(void* object);
private struct __spirv_AvcMcePayloadINTEL* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_AvcMcePayloadINTEL(void* object);
private struct __spirv_AvcMceResultINTEL* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_AvcMceResultINTEL(void* object);
private struct __spirv_AvcRefPayloadINTEL* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_AvcRefPayloadINTEL(void* object);
private struct __spirv_AvcRefResultINTEL* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_AvcRefResultINTEL(void* object);
private struct __spirv_AvcSicPayloadINTEL* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_AvcSicPayloadINTEL(void* object);
private struct __spirv_AvcSicResultINTEL* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_AvcSicResultINTEL(void* object);
private struct __spirv_DeviceEvent* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_deviceevent(void* object);
private struct __spirv_Event* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_event(void* object);
private struct __spirv_Queue* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_queue(void* object);
private struct __spirv_ReserveId* __attribute__((overloadable)) __builtin_IB_convert_object_type_to_spirv_reserveid(void* object);
queue_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_queue(void* object);
reserve_id_t __attribute__((overloadable)) __builtin_IB_convert_object_type_to_ocl_reserveid(void* object);

// Access of image and sampler parameters

int    __builtin_IB_get_address_mode(long) __attribute__((const));
int    __builtin_IB_is_normalized_coords(long) __attribute__((const));
int    __builtin_IB_get_image1d_array_size(long) __attribute__((const));
int    __builtin_IB_get_image2d_array_size(long) __attribute__((const));
int    __builtin_IB_get_snap_wa_reqd(long) __attribute__((const));
int    __builtin_IB_get_image_height(long) __attribute__((const));
int    __builtin_IB_get_image_width(long) __attribute__((const));
int    __builtin_IB_get_image_depth(long) __attribute__((const));
int    __builtin_IB_get_image_channel_data_type(long) __attribute__((const));
int    __builtin_IB_get_image_srgb_channel_order(long) __attribute__((const));
int    __builtin_IB_get_image_channel_order(long) __attribute__((const));
int    __builtin_IB_get_image_num_samples(long) __attribute__((const));
int    __builtin_IB_get_image_num_mip_levels(long) __attribute__((const));

// Access image or sampler parameter. Argument should be pointer
// to SampledImage or VMEImageINTEL opaque type.
long __builtin_IB_get_image(global void*);
long __builtin_IB_get_sampler(global void*);

// Image sampling and loads
float4 __builtin_IB_OCL_1d_sample_l(long, long, float,  float);
float4 __builtin_IB_OCL_1darr_sample_l(long, long, float2,  float);
float4 __builtin_IB_OCL_2d_sample_l(long, long, float2, float);
float4 __builtin_IB_OCL_2darr_sample_l(long, long, float3, float);
float4 __builtin_IB_OCL_3d_sample_l(long, long, float3, float);

float4 __builtin_IB_OCL_1d_sample_d(long, long, float,  float, float);
float4 __builtin_IB_OCL_1darr_sample_d(long, long, float2,  float, float);
float4 __builtin_IB_OCL_2d_sample_d(long, long, float2, float2, float2);
float4 __builtin_IB_OCL_2darr_sample_d(long, long, float3, float2, float2);
float4 __builtin_IB_OCL_3d_sample_d(long, long, float3, float3, float3);

// versions that return uint for read_imageui
uint4 __builtin_IB_OCL_1d_sample_lui(long, long, float,  float);
uint4 __builtin_IB_OCL_1darr_sample_lui(long, long, float2,  float);
uint4 __builtin_IB_OCL_2d_sample_lui(long, long, float2, float);
uint4 __builtin_IB_OCL_2darr_sample_lui(long, long, float3, float);
uint4 __builtin_IB_OCL_3d_sample_lui(long, long, float3, float);

uint4 __builtin_IB_OCL_1d_sample_dui(long, long, float,  float, float);
uint4 __builtin_IB_OCL_1darr_sample_dui(long, long, float2,  float, float);
uint4 __builtin_IB_OCL_2d_sample_dui(long, long, float2, float2, float2);
uint4 __builtin_IB_OCL_2darr_sample_dui(long, long, float3, float2, float2);
uint4 __builtin_IB_OCL_3d_sample_dui(long, long, float3, float3, float3);

#define IMAGE_READS(ACC_QUAL) \
uint4 __builtin_IB_OCL_1d_ldui##ACC_QUAL(long, int,  int); \
uint4 __builtin_IB_OCL_1darr_ldui##ACC_QUAL(long, int2,  int); \
uint4 __builtin_IB_OCL_2d_ldui##ACC_QUAL(long, int2, int); \
uint4 __builtin_IB_OCL_2darr_ldui##ACC_QUAL(long, int3, int); \
uint4 __builtin_IB_OCL_3d_ldui##ACC_QUAL(long, int3, int); \
\
float4 __builtin_IB_OCL_1d_ld##ACC_QUAL(long, int,  int); \
float4 __builtin_IB_OCL_1darr_ld##ACC_QUAL(long, int2,  int); \
float4 __builtin_IB_OCL_2d_ld##ACC_QUAL(long, int2, int); \
float4 __builtin_IB_OCL_2darr_ld##ACC_QUAL(long, int3, int); \
float4 __builtin_IB_OCL_3d_ld##ACC_QUAL(long, int3, int);
IMAGE_READS(_rw)
IMAGE_READS(_ro)
IMAGE_READS() // no access qulifier is the same as ro.
#undef IMAGE_READS

float4 __builtin_IB_OCL_2d_ldmcs(long, int2);
float4 __builtin_IB_OCL_2darr_ldmcs(long, int4);
float4 __builtin_IB_OCL_2d_ld2dms(long, int2, int, float4);
uint4  __builtin_IB_OCL_2d_ld2dmsui(long, int2, int, float4);
float4 __builtin_IB_OCL_2darr_ld2dms(long, int4, int, float4);
uint4  __builtin_IB_OCL_2darr_ld2dmsui(long, int4, int, float4);

long __builtin_IB_convert_sampler_to_int(sampler_t);

// Convert Functions for pipes and samplers
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
__global void* __builtin_IB_convert_pipe_ro_to_intel_pipe(pipe int);
__global void* __builtin_IB_convert_pipe_wo_to_intel_pipe(write_only pipe int);
#endif

// Image writes
void     __builtin_IB_write_1darr_u1i(long, int2, uint, int);
void     __builtin_IB_write_1darr_u2i(long, int2, uint2, int);
void     __builtin_IB_write_1darr_u3i(long, int2, uint3, int);
void     __builtin_IB_write_1darr_u4i(long, int2, uint4, int);
void     __builtin_IB_write_1d_u1i(long, int, uint, int);
void     __builtin_IB_write_1d_u2i(long, int, uint2, int);
void     __builtin_IB_write_1d_u3i(long, int, uint3, int);
void     __builtin_IB_write_1d_u4i(long, int, uint4, int);
void     __builtin_IB_write_2darr_u1i(long, int3, uint, int);
void     __builtin_IB_write_2darr_u2i(long, int3, uint2, int);
void     __builtin_IB_write_2darr_u3i(long, int3, uint3, int);
void     __builtin_IB_write_2darr_u4i(long, int3, uint4, int);
void     __builtin_IB_write_2d_u1i(long, int2, uint, int);
void     __builtin_IB_write_2d_u2i(long, int2, uint2, int);
void     __builtin_IB_write_2d_u3i(long, int2, uint3, int);
void     __builtin_IB_write_2d_u4i(long, int2, uint4, int);
void     __builtin_IB_write_3d_u1i(long, int3, uint, int);
void     __builtin_IB_write_3d_u2i(long, int3, uint2, int);
void     __builtin_IB_write_3d_u3i(long, int3, uint3, int);
void     __builtin_IB_write_3d_u4i(long, int3, uint4, int);
void     __builtin_IB_write_2darr_f(long, int4, float4, int);
void     __builtin_IB_write_2d_f(long, int2, float4, int);

// Workgroup functions
local uchar* __builtin_IB_AllocLocalMemPool(bool allocAllWorkgroups, uint numAdditionalElements, uint elementSize);

// Memory fences
void     __builtin_IB_memfence(bool commitEnable, bool flushRW, bool flushConstant, bool flushTexture, bool flushIcache, bool isGlobal, bool invalidateL1, bool evictL1, uint scope);
void     __builtin_IB_flush_sampler_cache(void);
void     __builtin_IB_typedmemfence(bool invalidateCache);

// Barrier
void     __builtin_IB_thread_group_barrier(void) __attribute__((convergent));
void     __builtin_IB_thread_group_barrier_signal(void) __attribute__((convergent));
void     __builtin_IB_thread_group_barrier_wait(void) __attribute__((convergent));

// Workitem functions
uint     __builtin_IB_get_work_dim(void) __attribute__((const));
uint     __builtin_IB_get_group_id(uint) __attribute__((const));
uint     __builtin_IB_get_local_thread_id() __attribute__((const));
uint     __builtin_IB_get_global_offset(uint) __attribute__((const));
uint     __builtin_IB_get_local_size(uint) __attribute__((const));
uint     __builtin_IB_get_local_id_x(void) __attribute__((const));
uint     __builtin_IB_get_local_id_y(void) __attribute__((const));
uint     __builtin_IB_get_local_id_z(void) __attribute__((const));
uint     __builtin_IB_get_global_size(uint) __attribute__((const));
uint     __builtin_IB_get_num_groups(uint) __attribute__((const));
uint     __builtin_IB_get_enqueued_local_size(uint) __attribute__((const));
uint     __builtin_IB_get_region_group_size(int dim) __attribute__((const));
uint     __builtin_IB_get_region_group_wg_count(void) __attribute__((const));
__global volatile uchar* __builtin_IB_get_region_group_barrier_buffer(void) __attribute__((const));

// Double precision conversions
half      __builtin_IB_ftoh_rtn(float) __attribute__((const));
half      __builtin_IB_ftoh_rtp(float) __attribute__((const));
half      __builtin_IB_ftoh_rtz(float)  __attribute__((const));
#if defined(cl_khr_fp64)
#endif // defined(cl_khr_fp64)

// Debug/Testing Built-In Functions
ulong     __builtin_IB_read_cycle_counter(void) __attribute__((const));
void      __builtin_IB_source_value(uint reg);
uint      __builtin_IB_set_dbg_register(uint dgb0_0);
uint      __builtin_IB_movreg(uint reg) __attribute__((const));
uint      __builtin_IB_movflag(uint flag) __attribute__((const));
uint      __builtin_IB_movcr(uint reg) __attribute__((const));
uint      __builtin_IB_hw_thread_id(void) __attribute__((const));
uint      __builtin_IB_slice_id(void) __attribute__((const));
uint      __builtin_IB_subslice_id(void) __attribute__((const));
uint      __builtin_IB_dual_subslice_id(void) __attribute__((const));
uint      __builtin_IB_eu_id(void) __attribute__((const));
uint      __builtin_IB_get_sr0(uint DWNumber); // DWNumber=0|1|2|3
void      __builtin_IB_set_sr0(uint DWNumber, uint Value); // DWNumber=0|1|2|3
uint      __builtin_IB_eu_thread_id(void) __attribute__((const));
uint      __builtin_IB_hw_tile_id(void) __attribute__((const));
uint      __builtin_IB_hw_engine_id(void) __attribute__((const));
void      __builtin_IB_profile_snapshot(int point_type,int point_index) __attribute__((const));
void      __builtin_IB_profile_aggregated(int point_type,int point_index) __attribute__((const));
void      __builtin_IB_eu_thread_pause(uint value);

// int -> float operations
float __builtin_IB_itof_rtn(int);
float __builtin_IB_itof_rtp(int);
float __builtin_IB_itof_rtz(int);
float __builtin_IB_uitof_rtn(uint);
float __builtin_IB_uitof_rtp(uint);
float __builtin_IB_uitof_rtz(uint);

#if defined(cl_khr_fp64)
// long -> double operations
double __builtin_IB_itofp64_rtn(long);
double __builtin_IB_itofp64_rtp(long);
double __builtin_IB_itofp64_rtz(long);
double __builtin_IB_uitofp64_rtn(ulong);
double __builtin_IB_uitofp64_rtp(ulong);
double __builtin_IB_uitofp64_rtz(ulong);
#endif

// Native integer operations
uint     __builtin_IB_bfi(uint, uint, uint, uint) __attribute__((const));
uint     __builtin_IB_ibfe(uint, uint, uint) __attribute__((const));
uint     __builtin_IB_ubfe(uint, uint, uint) __attribute__((const));
uint     __builtin_IB_bfrev(uint) __attribute__((const));

char     __builtin_IB_popcount_1u8(char) __attribute__((const));
short    __builtin_IB_popcount_1u16(short) __attribute__((const));
int      __builtin_IB_popcount_1u32(int) __attribute__((const));

// Native math operations - float version
float    __builtin_IB_frnd_ne(float) __attribute__((const));
float    __builtin_IB_frnd_ni(float) __attribute__((const));
float    __builtin_IB_frnd_pi(float) __attribute__((const));
float    __builtin_IB_frnd_zi(float) __attribute__((const));
float    __builtin_IB_native_exp2f(float) __attribute__((const));
float    __builtin_IB_native_cosf(float) __attribute__((const));
float    __builtin_IB_native_log2f(float) __attribute__((const));
float    __builtin_IB_native_powrf(float, float) __attribute__((const));
float    __builtin_IB_native_sinf(float) __attribute__((const));
float    __builtin_IB_native_sqrtf(float) __attribute__((const));
float    __builtin_IB_fmax(float, float) __attribute__((const));
float    __builtin_IB_fmin(float, float) __attribute__((const));
half     __builtin_IB_HMAX(half, half) __attribute__((const));
half     __builtin_IB_HMIN(half, half) __attribute__((const));

// Native math operations - fp16 version
half     __builtin_IB_native_cosh(half) __attribute__((const));
half     __builtin_IB_native_exp2h(half) __attribute__((const));
half     __builtin_IB_native_log2h(half) __attribute__((const));
half     __builtin_IB_native_sinh(half) __attribute__((const));
half     __builtin_IB_native_sqrth(half) __attribute__((const));
half     __builtin_IB_fmah(half, half, half) __attribute__((const));

// Native math operations - fp64 version
#if defined(cl_khr_fp64)
double    __builtin_IB_native_sqrtd(double) __attribute__((const));
double    __builtin_IB_dmin(double, double) __attribute__((const));
double    __builtin_IB_dmax(double, double) __attribute__((const));
#endif

// Boolean function on three sources
short __builtin_IB_bfn_i16(short, short, short, uchar) __attribute__((const));
int   __builtin_IB_bfn_i32(int, int, int, uchar) __attribute__((const));

// Atomic operations
int      __builtin_IB_atomic_add_global_i32(__global int*, int);
int      __builtin_IB_atomic_add_local_i32(__local int*, int);
int      __builtin_IB_atomic_sub_global_i32(__global int*, int);
int      __builtin_IB_atomic_sub_local_i32(__local int*, int);
int      __builtin_IB_atomic_xchg_global_i32(__global int*, int);
int      __builtin_IB_atomic_xchg_local_i32(__local int*, int);
int      __builtin_IB_atomic_min_global_i32(__global int*, int);
uint     __builtin_IB_atomic_min_global_u32(__global uint*, uint);
float    __builtin_IB_atomic_min_global_f32(__global float*, float);
int      __builtin_IB_atomic_min_local_i32(__local int*, int);
uint     __builtin_IB_atomic_min_local_u32(__local uint*, uint);
float    __builtin_IB_atomic_min_local_f32(__local float*, float);
int      __builtin_IB_atomic_max_global_i32(__global int*, int);
uint     __builtin_IB_atomic_max_global_u32(__global uint*, uint);
float    __builtin_IB_atomic_max_global_f32(__global float*, float);
int      __builtin_IB_atomic_max_local_i32(__local int*, int);
uint     __builtin_IB_atomic_max_local_u32(__local uint*, uint);
float    __builtin_IB_atomic_max_local_f32(__local float*, float);
int      __builtin_IB_atomic_and_global_i32(__global int*, int);
int      __builtin_IB_atomic_and_local_i32(__local int*, int);
int      __builtin_IB_atomic_or_global_i32(__global int*, int);
int      __builtin_IB_atomic_or_local_i32(__local int*, int);
int      __builtin_IB_atomic_xor_global_i32(__global int*, int);
int      __builtin_IB_atomic_xor_local_i32(__local int*, int);
int      __builtin_IB_atomic_inc_global_i32(__global int*);
int      __builtin_IB_atomic_inc_local_i32(__local int*);
int      __builtin_IB_atomic_dec_global_i32(__global int*);
int      __builtin_IB_atomic_dec_local_i32(__local int*);
int      __builtin_IB_atomic_cmpxchg_global_i32(__global int*, int, int);
float    __builtin_IB_atomic_cmpxchg_global_f32(__global float*, float, float);
int      __builtin_IB_atomic_cmpxchg_local_i32(__local int*, int, int);
float    __builtin_IB_atomic_cmpxchg_local_f32(__local float*, float, float);

// Float Atomics
#if defined(cl_intel_global_float_atomics)
float    __builtin_IB_atomic_add_global_f32(__global float*, float);
float    __builtin_IB_atomic_sub_global_f32(__global float*, float);
#endif // defined(cl_intel_global_float_atomics)
// 64bit Atomic operations
#if defined(cl_intel_64bit_global_atomics_placeholder)
long     __builtin_IB_atomic_add_global_i64(__global long*, long);
long     __builtin_IB_atomic_sub_global_i64(__global long*, long);
long     __builtin_IB_atomic_xchg_global_i64(__global long*, long);
long     __builtin_IB_atomic_min_global_i64(__global long*, long);
ulong    __builtin_IB_atomic_min_global_u64(__global ulong*, ulong);
double   __builtin_IB_atomic_min_global_f64(__global double*, double);
long     __builtin_IB_atomic_max_global_i64(__global long*, long);
ulong    __builtin_IB_atomic_max_global_u64(__global ulong*, ulong);
double   __builtin_IB_atomic_max_global_f64(__global double*, double);
long     __builtin_IB_atomic_and_global_i64(__global long*, long);
long     __builtin_IB_atomic_or_global_i64(__global long*, long);
long     __builtin_IB_atomic_xor_global_i64(__global long*, long);
long     __builtin_IB_atomic_inc_global_i64(__global long*);
long     __builtin_IB_atomic_dec_global_i64(__global long*);
long     __builtin_IB_atomic_cmpxchg_global_i64(__global long*, long, long);
double   __builtin_IB_atomic_cmpxchg_global_f64(__global double*, double, double);
double   __builtin_IB_atomic_add_global_f64(__global double*, double);
#endif // defined(cl_intel_64bit_global_atomics_placeholder)

// Atomic operations
short    __builtin_IB_atomic_add_global_i16(__global short*, short);
short    __builtin_IB_atomic_add_local_i16(__local short*, short);
short    __builtin_IB_atomic_sub_global_i16(__global short*, short);
short    __builtin_IB_atomic_sub_local_i16(__local short*, short);
short    __builtin_IB_atomic_xchg_global_i16(__global short*, short);
short    __builtin_IB_atomic_xchg_local_i16(__local short*, short);
short    __builtin_IB_atomic_min_global_i16(__global short*, short);
ushort   __builtin_IB_atomic_min_global_u16(__global ushort*, ushort);
half     __builtin_IB_atomic_min_global_f16(__global half*, half);
short    __builtin_IB_atomic_min_local_i16(__local short*, short);
ushort   __builtin_IB_atomic_min_local_u16(__local ushort*, ushort);
half     __builtin_IB_atomic_min_local_f16(__local half*, half);
short    __builtin_IB_atomic_max_global_i16(__global short*, short);
ushort   __builtin_IB_atomic_max_global_u16(__global ushort*, ushort);
half     __builtin_IB_atomic_max_global_f16(__global half*, half);
short    __builtin_IB_atomic_max_local_i16(__local short*, short);
ushort   __builtin_IB_atomic_max_local_u16(__local ushort*, ushort);
half     __builtin_IB_atomic_max_local_f16(__local half*, half);
short    __builtin_IB_atomic_and_global_i16(__global short*, short);
short    __builtin_IB_atomic_and_local_i16(__local short*, short);
short    __builtin_IB_atomic_or_global_i16(__global short*, short);
short    __builtin_IB_atomic_or_local_i16(__local short*, short);
short    __builtin_IB_atomic_xor_global_i16(__global short*, short);
short    __builtin_IB_atomic_xor_local_i16(__local short*, short);
short    __builtin_IB_atomic_inc_global_i16(__global short*);
short    __builtin_IB_atomic_inc_local_i16(__local short*);
short    __builtin_IB_atomic_dec_global_i16(__global short*);
short    __builtin_IB_atomic_dec_local_i16(__local short*);
short    __builtin_IB_atomic_cmpxchg_global_i16(__global short*, short, short);
half     __builtin_IB_atomic_cmpxchg_global_f16(__global half*, half, half);
short    __builtin_IB_atomic_cmpxchg_local_i16(__local short*, short, short);
half     __builtin_IB_atomic_cmpxchg_local_f16(__local half*, half, half);

short    __builtin_IB_image_atomic_add_i16(long, int4, short);
short    __builtin_IB_image_atomic_sub_i16(long, int4, short);
short    __builtin_IB_image_atomic_xchg_i16(long, int4, short);
short    __builtin_IB_image_atomic_min_i16(long, int4, short);
ushort   __builtin_IB_image_atomic_min_u16(long, int4, ushort);
short    __builtin_IB_image_atomic_max_i16(long, int4, short);
ushort   __builtin_IB_image_atomic_max_u16(long, int4, ushort);
short    __builtin_IB_image_atomic_and_i16(long, int4, short);
short    __builtin_IB_image_atomic_or_i16(long, int4, short);
short    __builtin_IB_image_atomic_xor_i16(long, int4, short);
short    __builtin_IB_image_atomic_inc_i16(long, int4);
short    __builtin_IB_image_atomic_cmpxchg_i16(long, int4, short, short);

float    __builtin_IB_atomic_add_local_f32(__local float*, float);

half     __builtin_IB_atomic_add_local_f16(__local half*, half);
half     __builtin_IB_atomic_add_global_f16(__global half*, half);

ushort   __builtin_IB_atomic_add_global_bf16(__global ushort*, ushort);
ushort   __builtin_IB_atomic_sub_global_bf16(__global ushort*, ushort);
ushort   __builtin_IB_atomic_min_global_bf16(__global ushort*, ushort);
ushort   __builtin_IB_atomic_max_global_bf16(__global ushort*, ushort);

ushort   __builtin_IB_atomic_add_local_bf16(__local ushort*, ushort);
ushort   __builtin_IB_atomic_sub_local_bf16(__local ushort*, ushort);
ushort   __builtin_IB_atomic_min_local_bf16(__local ushort*, ushort);
ushort   __builtin_IB_atomic_max_local_bf16(__local ushort*, ushort);


void __builtin_IB_kmp_acquire_lock(int *);
void __builtin_IB_kmp_release_lock(int *);

int      __builtin_IB_image_atomic_add_i32(long, int4, int);
int      __builtin_IB_image_atomic_sub_i32(long, int4, int);
int      __builtin_IB_image_atomic_xchg_i32(long, int4, int);
int      __builtin_IB_image_atomic_min_i32(long, int4, int);
uint     __builtin_IB_image_atomic_min_u32(long, int4, uint);
int      __builtin_IB_image_atomic_max_i32(long, int4, int);
uint     __builtin_IB_image_atomic_max_u32(long, int4, uint);
int      __builtin_IB_image_atomic_and_i32(long, int4, int);
int      __builtin_IB_image_atomic_or_i32(long, int4, int);
int      __builtin_IB_image_atomic_xor_i32(long, int4, int);
int      __builtin_IB_image_atomic_inc_i32(long, int4);
int      __builtin_IB_image_atomic_dec_i32(long, int4);
int      __builtin_IB_image_atomic_cmpxchg_i32(long, int4, int, int);

void __builtin_IB_memcpy_global_to_private(private uchar *dst, global uchar *src, uint size, uint align);
void __builtin_IB_memcpy_constant_to_private(private uchar *dst, constant uchar *src, uint size, uint align);
void __builtin_IB_memcpy_local_to_private(private uchar *dst, local uchar *src, uint size, uint align);
void __builtin_IB_memcpy_private_to_private(private uchar *dst, private uchar *src, uint size, uint align);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void __builtin_IB_memcpy_generic_to_private(private uchar *dst, generic uchar *src, uint size, uint align);
#endif

void __builtin_IB_memcpy_private_to_global(global uchar *dst, private uchar *src, uint size, uint align);
void __builtin_IB_memcpy_private_to_constant(constant uchar *dst, private uchar *src, uint size, uint align);
void __builtin_IB_memcpy_private_to_local(local uchar *dst, private uchar *src, uint size, uint align);
void __builtin_IB_memcpy_private_to_private(private uchar *dst, private uchar *src, uint size, uint align);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void __builtin_IB_memcpy_private_to_generic(generic uchar *dst, private uchar *src, uint size, uint align);
#endif

// Correctly rounded sqrt and division
float   __builtin_IB_ieee_sqrt(float) __attribute__((const));
float   __builtin_IB_ieee_sqrt_rm(float, int) __attribute__((const));
float   __builtin_IB_ieee_divide(float, float) __attribute__((const));
float   __builtin_IB_ieee_divide_rm(float, float, int) __attribute__((const));

#if defined(cl_khr_fp64)
double   __builtin_IB_ieee_divide_f64(double, double) __attribute__((const));
#endif

// SIMD information
ushort __builtin_IB_simd_lane_id() __attribute__((const));

// an opaque handle pointing to a blob of registers.
typedef uint GRFHandle;

// legacy message phase builtins for old vme (not device side)
void __builtin_IB_set_message_phase_legacy_dw(uint messagePhases, uint phaseIndex, uint dwIndex, uint val);
void __builtin_IB_set_message_phase_legacy_uw(uint messagePhases, uint phaseIndex, uint dwIndex, ushort val);
void __builtin_IB_set_message_phase_legacy_ub(uint messagePhases, uint phaseIndex, uint dwIndex, uchar val);

void __builtin_IB_set_message_phase_legacy(uint messagePhases, uint phaseIndex, uint val);

// Message Phases manipulation
uint __builtin_IB_create_message_phases(uint numPhases);
uint2 __builtin_IB_create_message_phases_uint2(uint numPhases);
uint4 __builtin_IB_create_message_phases_uint4(uint numPhases);
uint8 __builtin_IB_create_message_phases_uint8(uint numPhases);

uint __builtin_IB_create_message_phases_no_init(uint numPhases);
uint2 __builtin_IB_create_message_phases_no_init_uint2(uint numPhases);
uint4 __builtin_IB_create_message_phases_no_init_uint4(uint numPhases);
uint8 __builtin_IB_create_message_phases_no_init_uint8(uint numPhases);

uint __builtin_IB_get_message_phase_dw(uint messagePhases, uint phaseIndex, uint dwIndex);
uint __builtin_IB_get_message_phase_dw_uint2(uint2 messagePhases, uint phaseIndex, uint dwIndex);
uint __builtin_IB_get_message_phase_dw_uint4(uint4 messagePhases, uint phaseIndex, uint dwIndex);
uint __builtin_IB_get_message_phase_dw_uint8(uint8 messagePhases, uint phaseIndex, uint dwIndex);

ulong __builtin_IB_get_message_phase_uq(uint messagePhases, uint phaseIndex, uint dwIndex);
ulong __builtin_IB_get_message_phase_uq_uint2(uint2 messagePhases, uint phaseIndex, uint dwIndex);
ulong __builtin_IB_get_message_phase_uq_uint4(uint4 messagePhases, uint phaseIndex, uint dwIndex);
ulong __builtin_IB_get_message_phase_uq_uint8(uint8 messagePhases, uint phaseIndex, uint dwIndex);

uint __builtin_IB_set_message_phase_dw(uint messagePhases, uint phaseIndex, uint dwIndex, uint val);
uint2 __builtin_IB_set_message_phase_dw_uint2(uint2 messagePhases, uint phaseIndex, uint dwIndex, uint val);
uint4 __builtin_IB_set_message_phase_dw_uint4(uint4 messagePhases, uint phaseIndex, uint dwIndex, uint val);
uint8 __builtin_IB_set_message_phase_dw_uint8(uint8 messagePhases, uint phaseIndex, uint dwIndex, uint val);

uint __builtin_IB_get_message_phase(uint messagePhases, uint phaseIndex);
uint __builtin_IB_get_message_phase_uint2(uint2 messagePhases, uint phaseIndex);
uint __builtin_IB_get_message_phase_uint4(uint4 messagePhases, uint phaseIndex);
uint __builtin_IB_get_message_phase_uint8(uint8 messagePhases, uint phaseIndex);

uint __builtin_IB_set_message_phase(uint messagePhases, uint phaseIndex, uint val);
uint2 __builtin_IB_set_message_phase_uint2(uint2 messagePhases, uint phaseIndex, uint val);
uint4 __builtin_IB_set_message_phase_uint4(uint4 messagePhases, uint phaseIndex, uint val);
uint8 __builtin_IB_set_message_phase_uint8(uint8 messagePhases, uint phaseIndex, uint val);

ushort __builtin_IB_get_message_phase_uw(uint messagePhases, uint phaseIndex, uint wIndex);
ushort __builtin_IB_get_message_phase_uw_uint2(uint2 messagePhases, uint phaseIndex, uint wIndex);
ushort __builtin_IB_get_message_phase_uw_uint4(uint4 messagePhases, uint phaseIndex, uint wIndex);
ushort __builtin_IB_get_message_phase_uw_uint8(uint8 messagePhases, uint phaseIndex, uint wIndex);

uint __builtin_IB_set_message_phase_uw(uint messagePhases, uint phaseIndex, uint dwIndex, ushort val);
uint2 __builtin_IB_set_message_phase_uw_uint2(uint2 messagePhases, uint phaseIndex, uint dwIndex, ushort val);
uint4 __builtin_IB_set_message_phase_uw_uint4(uint4 messagePhases, uint phaseIndex, uint dwIndex, ushort val);
uint8 __builtin_IB_set_message_phase_uw_uint8(uint8 messagePhases, uint phaseIndex, uint dwIndex, ushort val);

uchar __builtin_IB_get_message_phase_ub(uint messagePhases, uint phaseIndex, uint dwIndex);
uchar __builtin_IB_get_message_phase_ub_uint2(uint2 messagePhases, uint phaseIndex, uint dwIndex);
uchar __builtin_IB_get_message_phase_ub_uint4(uint4 messagePhases, uint phaseIndex, uint dwIndex);
uchar __builtin_IB_get_message_phase_ub_uint8(uint8 messagePhases, uint phaseIndex, uint dwIndex);

uint __builtin_IB_set_message_phase_ub(uint messagePhases, uint phaseIndex, uint dwIndex, uchar val);
uint2 __builtin_IB_set_message_phase_ub_uint2(uint2 messagePhases, uint phaseIndex, uint dwIndex, uchar val);
uint4 __builtin_IB_set_message_phase_ub_uint4(uint4 messagePhases, uint phaseIndex, uint dwIndex, uchar val);
uint8 __builtin_IB_set_message_phase_ub_uint8(uint8 messagePhases, uint phaseIndex, uint dwIndex, uchar val);

// Broadcast a phase value to all work-items in a sub-group
uchar  __builtin_IB_broadcast_message_phase_ub(uint messagePhases, uint phaseIndex, uint phaseSubindex, uint width);
uchar  __builtin_IB_broadcast_message_phase_ub_uint2(uint2 messagePhases, uint phaseIndex, uint phaseSubindex, uint width);
uchar  __builtin_IB_broadcast_message_phase_ub_uint4(uint4 messagePhases, uint phaseIndex, uint phaseSubindex, uint width);
uchar  __builtin_IB_broadcast_message_phase_ub_uint8(uint8 messagePhases, uint phaseIndex, uint phaseSubindex, uint width);

ushort __builtin_IB_broadcast_message_phase_uw(uint messagePhases, uint phaseIndex, uint phaseSubindex, uint width);
ushort __builtin_IB_broadcast_message_phase_uw_uint2(uint2 messagePhases, uint phaseIndex, uint phaseSubindex, uint width);
ushort __builtin_IB_broadcast_message_phase_uw_uint4(uint4 messagePhases, uint phaseIndex, uint phaseSubindex, uint width);
ushort __builtin_IB_broadcast_message_phase_uw_uint8(uint8 messagePhases, uint phaseIndex, uint phaseSubindex, uint width);

uint   __builtin_IB_broadcast_message_phase_dw(uint messagePhases, uint phaseIndex, uint phaseSubindex, uint width);
uint   __builtin_IB_broadcast_message_phase_dw_uint2(uint2 messagePhases, uint phaseIndex, uint phaseSubindex, uint width);
uint   __builtin_IB_broadcast_message_phase_dw_uint4(uint4 messagePhases, uint phaseIndex, uint phaseSubindex, uint width);
uint   __builtin_IB_broadcast_message_phase_dw_uint8(uint8 messagePhases, uint phaseIndex, uint phaseSubindex, uint width);

ulong  __builtin_IB_broadcast_message_phase_uq(uint messagePhases, uint phaseIndex, uint phaseSubindex, uint width);
ulong  __builtin_IB_broadcast_message_phase_uq_uint2(uint2 messagePhases, uint phaseIndex, uint phaseSubindex, uint width);
ulong  __builtin_IB_broadcast_message_phase_uq_uint4(uint4 messagePhases, uint phaseIndex, uint phaseSubindex, uint width);
ulong  __builtin_IB_broadcast_message_phase_uq_uint8(uint8 messagePhases, uint phaseIndex, uint phaseSubindex, uint width);

// Copy the value phase(s) to all work-items in a sub-group
ushort __builtin_IB_simd_get_message_phase_uw(uint messagePhases, uint phaseIndex, uint numPhases);
ushort __builtin_IB_simd_get_message_phase_uw_uint2(uint2 messagePhases, uint phaseIndex, uint numPhases);
ushort __builtin_IB_simd_get_message_phase_uw_uint4(uint4 messagePhases, uint phaseIndex, uint numPhases);
ushort __builtin_IB_simd_get_message_phase_uw_uint8(uint8 messagePhases, uint phaseIndex, uint numPhases);

ulong  __builtin_IB_simd_get_message_phase_uq(uint messagePhases, uint phaseIndex, uint numPhases);
ulong  __builtin_IB_simd_get_message_phase_uq_uint2(uint2 messagePhases, uint phaseIndex, uint numPhases);
ulong  __builtin_IB_simd_get_message_phase_uq_uint4(uint4 messagePhases, uint phaseIndex, uint numPhases);
ulong  __builtin_IB_simd_get_message_phase_uq_uint8(uint8 messagePhases, uint phaseIndex, uint numPhases);

uint __builtin_IB_simd_set_message_phase_ub(uint messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, uchar val);
uint2 __builtin_IB_simd_set_message_phase_ub_uint2(uint2 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, uchar val);
uint4 __builtin_IB_simd_set_message_phase_ub_uint4(uint4 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, uchar val);
uint8 __builtin_IB_simd_set_message_phase_ub_uint8(uint8 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, uchar val);

uint __builtin_IB_simd_set_message_phase_uw(uint messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ushort val);
uint2 __builtin_IB_simd_set_message_phase_uw_uint2(uint2 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ushort val);
uint4 __builtin_IB_simd_set_message_phase_uw_uint4(uint4 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ushort val);
uint8 __builtin_IB_simd_set_message_phase_uw_uint8(uint8 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ushort val);

uint __builtin_IB_simd_set_message_phase_dw(uint messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, uint val);
uint2 __builtin_IB_simd_set_message_phase_dw_uint2(uint2 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, uint val);
uint4 __builtin_IB_simd_set_message_phase_dw_uint4(uint4 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, uint val);
uint8 __builtin_IB_simd_set_message_phase_dw_uint8(uint8 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, uint val);

uint __builtin_IB_simd_set_message_phase_uq(uint messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val);
uint2 __builtin_IB_simd_set_message_phase_uq_uint2(uint2 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val);
uint4 __builtin_IB_simd_set_message_phase_uq_uint4(uint4 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val);
uint8 __builtin_IB_simd_set_message_phase_uq_uint8(uint8 messagePhases, uint phaseIndex, uint numPhases, uint subReg, uint numLanes, ulong val);

void __builtin_IB_simdMediaRegionCopy(GRFHandle dst, uint dbyteoffset, uint dstride, uint dnumelem,
                                      GRFHandle src, uint sbyteoffset, uint vstride, uint width, uint hstride, uint typesize, uint execsize, uint snumelem);

void __builtin_IB_extract_mv_and_sad(GRFHandle MVMin, GRFHandle SADMin, GRFHandle result, uint blockType);
void __builtin_IB_cmp_sads(GRFHandle MVCurr, GRFHandle SADCurr, GRFHandle MVMin, GRFHandle SADMin);

// VME
uint __builtin_IB_vme_mb_block_type() __attribute__((const));
uint __builtin_IB_vme_subpixel_mode() __attribute__((const));
uint __builtin_IB_vme_sad_adjust_mode() __attribute__((const));
uint __builtin_IB_vme_search_path_type() __attribute__((const));
void __builtin_IB_vme_send_ime(GRFHandle res, GRFHandle universalInputMsg, GRFHandle imeMsg, long srcImg, long refImg, uint ref0Coord, uint ref1Coord, uint costCenter);
void __builtin_IB_vme_send_fbr(GRFHandle res, GRFHandle universalInputMsg, GRFHandle fbrMsg, long srcImg, long refImg, uint interMbMode, uint subMbShape, uint subMbPredMode);
void __builtin_IB_vme_send_sic(GRFHandle res, GRFHandle universalInputMsg, GRFHandle sicMsg, long srcImg, long refImg0, long refImg1);

uint4 __builtin_IB_vme_send_ime_new_uint4_uint8(uint8 inputMsg, long srcImg, long fwdRefImg, long bwdRefImg, long accelerator, uint streamMode);
uint8 __builtin_IB_vme_send_ime_new_uint8_uint8(uint8 inputMsg, long srcImg, long fwdRefImg, long bwdRefImg, long accelerator, uint streamMode);
uint4 __builtin_IB_vme_send_ime_new_uint4_uint4(uint4 inputMsg, long srcImg, long fwdRefImg, long bwdRefImg, long accelerator, uint streamMode);
uint8 __builtin_IB_vme_send_ime_new_uint8_uint4(uint4 inputMsg, long srcImg, long fwdRefImg, long bwdRefImg, long accelerator, uint streamMode);

uint4 __builtin_IB_vme_send_fbr_new(uint4 inputMsg, long srcImg, long fwdRefImg, long bwdRefImg, long accelerator);
uint4 __builtin_IB_vme_send_sic_new(uint4 inputMsg, long srcImg, long fwdRefImg, long bwdRefImg, long accelerator);

uint  __builtin_IB_get_image_bti(uint img);

// ballot intrinsic
uint __builtin_IB_WaveBallot(bool p);
uint __builtin_IB_clustered_WaveBallot(bool p, uint cluster_size);

// VA
void   __builtin_IB_va_erode_64x4( __local uchar* dst, float2 coords, long srcImgId, int i_accelerator );
void   __builtin_IB_va_dilate_64x4( __local uchar* dst, float2 coords, long srcImgId, int i_accelerator );
void   __builtin_IB_va_minmaxfilter_16x4_SLM( __local uchar* dst, float2 coords, long srcImgId, int i_accelerator );
void   __builtin_IB_va_convolve_16x4_SLM( __local uchar* dst, float2 coords, long srcImgId, int i_accelerator );
void   __builtin_IB_va_minmax( __local uchar* dst, float2 coords, long srcImgId, int i_accelerator );
void   __builtin_IB_va_centroid( __local uchar* dst, float2 coords, int2 size, long srcImgId, int i_accelerator );
void   __builtin_IB_va_boolcentroid( __local uchar* dst, float2 coords, int2 size, long srcImgId, int i_accelerator );
void   __builtin_IB_va_boolsum( __local uchar* dst, float2 coords, int2 size, long srcImgId, int i_accelerator );
short4 __builtin_IB_va_convolve_16x4( float2 coords, long srcImgId, int i_accelerator );

// Device Enqueue
__global void* __builtin_IB_get_default_device_queue();
__global void* __builtin_IB_get_event_pool();
uint __builtin_IB_get_max_workgroup_size();
uint __builtin_IB_get_parent_event();
uint __builtin_IB_get_prefered_workgroup_multiple();

// Generic Address Space
__local   void* __builtin_IB_to_local(void*);
__private void* __builtin_IB_to_private(void*);

// Internal program hint
// facility for enforcing uniform property (@WIAnalysis) for
// a local array residing in thread-private memory
void __builtin_IB_assume_uniform(void*);

// SubGroup Functions
int     __builtin_IB_get_simd_size( void );
int     __builtin_IB_get_simd_id( void );
uint    __builtin_IB_simd_shuffle(         uint,   uint );
bool    __builtin_IB_simd_shuffle_b(       bool,   uint);
uchar   __builtin_IB_simd_shuffle_c(       uchar,  uint );
ushort  __builtin_IB_simd_shuffle_us(      ushort, uint );
float   __builtin_IB_simd_shuffle_f(       float,  uint );
half    __builtin_IB_simd_shuffle_h(       half,   uint );
double  __builtin_IB_simd_shuffle_df(      double, uint);
uint    __builtin_IB_simd_shuffle_down(    uint,   uint,   uint );
ushort  __builtin_IB_simd_shuffle_down_us( ushort, ushort, uint );
uchar   __builtin_IB_simd_shuffle_down_uc( uchar,  uchar,  uint );
uint    __builtin_IB_simd_broadcast(       uint,   uint );
bool    __builtin_IB_simd_broadcast_b(     bool,   uint );
uchar   __builtin_IB_simd_broadcast_c(     uchar,  uint );
ushort  __builtin_IB_simd_broadcast_us(    ushort, uint );
float   __builtin_IB_simd_broadcast_f(     float,  uint );
half    __builtin_IB_simd_broadcast_h(     half,   uint );
double  __builtin_IB_simd_broadcast_df(    double, uint );
void    __builtin_IB_sub_group_barrier();

// SubGroup clustered broadcast - for internal use
uint   __builtin_IB_simd_clustered_broadcast(    uint,   uint, uint );
bool   __builtin_IB_simd_clustered_broadcast_b(  bool,   uint, uint );
uchar  __builtin_IB_simd_clustered_broadcast_c(  uchar,  uint, uint );
ushort __builtin_IB_simd_clustered_broadcast_us( ushort, uint, uint );
float  __builtin_IB_simd_clustered_broadcast_f(  float,  uint, uint );
half   __builtin_IB_simd_clustered_broadcast_h(  half,   uint, uint );
double __builtin_IB_simd_clustered_broadcast_df( double, uint, uint );

// Block read : global address space
uint    __builtin_IB_simd_block_read_1_global( const __global uint* );
uint2   __builtin_IB_simd_block_read_2_global( const __global uint* );
uint4   __builtin_IB_simd_block_read_4_global( const __global uint* );
uint8   __builtin_IB_simd_block_read_8_global( const __global uint* );

ushort    __builtin_IB_simd_block_read_1_global_h( const __global ushort* );
ushort2   __builtin_IB_simd_block_read_2_global_h( const __global ushort* );
ushort4   __builtin_IB_simd_block_read_4_global_h( const __global ushort* );
ushort8   __builtin_IB_simd_block_read_8_global_h( const __global ushort* );
ushort16  __builtin_IB_simd_block_read_16_global_h( const __global ushort* );

uchar    __builtin_IB_simd_block_read_1_global_b( const __global uchar* );
uchar2   __builtin_IB_simd_block_read_2_global_b( const __global uchar* );
uchar4   __builtin_IB_simd_block_read_4_global_b( const __global uchar* );
uchar8   __builtin_IB_simd_block_read_8_global_b( const __global uchar* );
uchar16  __builtin_IB_simd_block_read_16_global_b( const __global uchar* );

ulong    __builtin_IB_simd_block_read_1_global_l( const __global ulong* );
ulong2   __builtin_IB_simd_block_read_2_global_l( const __global ulong* );
ulong4   __builtin_IB_simd_block_read_4_global_l( const __global ulong* );
ulong8   __builtin_IB_simd_block_read_8_global_l( const __global ulong* );

void    __builtin_IB_simd_block_write_1_global( __global uint*, uint );
void    __builtin_IB_simd_block_write_2_global( __global uint*, uint2 );
void    __builtin_IB_simd_block_write_4_global( __global uint*, uint4 );
void    __builtin_IB_simd_block_write_8_global( __global uint*, uint8 );

void    __builtin_IB_simd_block_write_1_global_h( __global ushort*, ushort );
void    __builtin_IB_simd_block_write_2_global_h( __global ushort*, ushort2 );
void    __builtin_IB_simd_block_write_4_global_h( __global ushort*, ushort4 );
void    __builtin_IB_simd_block_write_8_global_h( __global ushort*, ushort8 );
void    __builtin_IB_simd_block_write_16_global_h( __global ushort*, ushort16 );

void    __builtin_IB_simd_block_write_1_global_b( __global uchar*, uchar );
void    __builtin_IB_simd_block_write_2_global_b( __global uchar*, uchar2 );
void    __builtin_IB_simd_block_write_4_global_b( __global uchar*, uchar4 );
void    __builtin_IB_simd_block_write_8_global_b( __global uchar*, uchar8 );
void    __builtin_IB_simd_block_write_16_global_b( __global uchar*, uchar16 );

void    __builtin_IB_simd_block_write_1_global_l( __global ulong*, ulong );
void    __builtin_IB_simd_block_write_2_global_l( __global ulong*, ulong2 );
void    __builtin_IB_simd_block_write_4_global_l( __global ulong*, ulong4 );
void    __builtin_IB_simd_block_write_8_global_l( __global ulong*, ulong8 );

// 1D block reads and writes with cache control argument : global address space
uint    __builtin_IB_cache_controls_simd_block_read_1_global(const __global uint* base, uint cacheOpt);
uint2   __builtin_IB_cache_controls_simd_block_read_2_global(const __global uint* base, uint cacheOpt);
uint4   __builtin_IB_cache_controls_simd_block_read_4_global(const __global uint* base, uint cacheOpt);
uint8   __builtin_IB_cache_controls_simd_block_read_8_global(const __global uint* base, uint cacheOpt);

ushort    __builtin_IB_cache_controls_simd_block_read_1_global_h(const __global ushort* base, uint cacheOpt);
ushort2   __builtin_IB_cache_controls_simd_block_read_2_global_h(const __global ushort* base, uint cacheOpt);
ushort4   __builtin_IB_cache_controls_simd_block_read_4_global_h(const __global ushort* base, uint cacheOpt);
ushort8   __builtin_IB_cache_controls_simd_block_read_8_global_h(const __global ushort* base, uint cacheOpt);
ushort16  __builtin_IB_cache_controls_simd_block_read_16_global_h(const __global ushort* base, uint cacheOpt);

uchar    __builtin_IB_cache_controls_simd_block_read_1_global_b(const __global uchar* base, uint cacheOpt);
uchar2   __builtin_IB_cache_controls_simd_block_read_2_global_b(const __global uchar* base, uint cacheOpt);
uchar4   __builtin_IB_cache_controls_simd_block_read_4_global_b(const __global uchar* base, uint cacheOpt);
uchar8   __builtin_IB_cache_controls_simd_block_read_8_global_b(const __global uchar* base, uint cacheOpt);
uchar16  __builtin_IB_cache_controls_simd_block_read_16_global_b(const __global uchar* base, uint cacheOpt);

ulong    __builtin_IB_cache_controls_simd_block_read_1_global_l(const __global ulong* base, uint cacheOpt);
ulong2   __builtin_IB_cache_controls_simd_block_read_2_global_l(const __global ulong* base, uint cacheOpt);
ulong4   __builtin_IB_cache_controls_simd_block_read_4_global_l(const __global ulong* base, uint cacheOpt);
ulong8   __builtin_IB_cache_controls_simd_block_read_8_global_l(const __global ulong* base, uint cacheOpt);

void    __builtin_IB_cache_controls_simd_block_write_1_global(__global uint* base, uint val, uint cacheOpt);
void    __builtin_IB_cache_controls_simd_block_write_2_global(__global uint* base, uint2 val, uint cacheOpt);
void    __builtin_IB_cache_controls_simd_block_write_4_global(__global uint* base, uint4 val, uint cacheOpt);
void    __builtin_IB_cache_controls_simd_block_write_8_global(__global uint* base, uint8 val, uint cacheOpt);

void    __builtin_IB_cache_controls_simd_block_write_1_global_h(__global ushort* base, ushort val, uint cacheOpt);
void    __builtin_IB_cache_controls_simd_block_write_2_global_h(__global ushort* base, ushort2 val, uint cacheOpt);
void    __builtin_IB_cache_controls_simd_block_write_4_global_h(__global ushort* base, ushort4 val, uint cacheOpt);
void    __builtin_IB_cache_controls_simd_block_write_8_global_h(__global ushort* base, ushort8 val, uint cacheOpt);
void    __builtin_IB_cache_controls_simd_block_write_16_global_h(__global ushort* base, ushort16 val, uint cacheOpt);

void    __builtin_IB_cache_controls_simd_block_write_1_global_b(__global uchar* base, uchar val, uint cacheOpt);
void    __builtin_IB_cache_controls_simd_block_write_2_global_b(__global uchar* base, uchar2 val, uint cacheOpt);
void    __builtin_IB_cache_controls_simd_block_write_4_global_b(__global uchar* base, uchar4 val, uint cacheOpt);
void    __builtin_IB_cache_controls_simd_block_write_8_global_b(__global uchar* base, uchar8 val, uint cacheOpt);
void    __builtin_IB_cache_controls_simd_block_write_16_global_b(__global uchar* base, uchar16 val, uint cacheOpt);

void    __builtin_IB_cache_controls_simd_block_write_1_global_l(__global ulong* base, ulong val, uint cacheOpt);
void    __builtin_IB_cache_controls_simd_block_write_2_global_l(__global ulong* base, ulong2 val, uint cacheOpt);
void    __builtin_IB_cache_controls_simd_block_write_4_global_l(__global ulong* base, ulong4 val, uint cacheOpt);
void    __builtin_IB_cache_controls_simd_block_write_8_global_l(__global ulong* base, ulong8 val, uint cacheOpt);

// Block read : local address space
uint    __builtin_IB_simd_block_read_1_local( const __local uint* );
uint2   __builtin_IB_simd_block_read_2_local( const __local uint* );
uint4   __builtin_IB_simd_block_read_4_local( const __local uint* );
uint8   __builtin_IB_simd_block_read_8_local( const __local uint* );

ushort    __builtin_IB_simd_block_read_1_local_h( const __local ushort* );
ushort2   __builtin_IB_simd_block_read_2_local_h( const __local ushort* );
ushort4   __builtin_IB_simd_block_read_4_local_h( const __local ushort* );
ushort8   __builtin_IB_simd_block_read_8_local_h( const __local ushort* );
ushort16  __builtin_IB_simd_block_read_16_local_h( const __local ushort* );

uchar    __builtin_IB_simd_block_read_1_local_b( const __local uchar* );
uchar2   __builtin_IB_simd_block_read_2_local_b( const __local uchar* );
uchar4   __builtin_IB_simd_block_read_4_local_b( const __local uchar* );
uchar8   __builtin_IB_simd_block_read_8_local_b( const __local uchar* );
uchar16  __builtin_IB_simd_block_read_16_local_b( const __local uchar* );

ulong    __builtin_IB_simd_block_read_1_local_l( const __local ulong* );
ulong2   __builtin_IB_simd_block_read_2_local_l( const __local ulong* );
ulong4   __builtin_IB_simd_block_read_4_local_l( const __local ulong* );
ulong8   __builtin_IB_simd_block_read_8_local_l( const __local ulong* );

void    __builtin_IB_simd_block_write_1_local( __local uint*, uint );
void    __builtin_IB_simd_block_write_2_local( __local uint*, uint2 );
void    __builtin_IB_simd_block_write_4_local( __local uint*, uint4 );
void    __builtin_IB_simd_block_write_8_local( __local uint*, uint8 );

void    __builtin_IB_simd_block_write_1_local_h( __local ushort*, ushort );
void    __builtin_IB_simd_block_write_2_local_h( __local ushort*, ushort2 );
void    __builtin_IB_simd_block_write_4_local_h( __local ushort*, ushort4 );
void    __builtin_IB_simd_block_write_8_local_h( __local ushort*, ushort8 );
void    __builtin_IB_simd_block_write_16_local_h( __local ushort*, ushort16 );

void    __builtin_IB_simd_block_write_1_local_b( __local uchar*, uchar );
void    __builtin_IB_simd_block_write_2_local_b( __local uchar*, uchar2 );
void    __builtin_IB_simd_block_write_4_local_b( __local uchar*, uchar4 );
void    __builtin_IB_simd_block_write_8_local_b( __local uchar*, uchar8 );
void    __builtin_IB_simd_block_write_16_local_b( __local uchar*, uchar16 );

void    __builtin_IB_simd_block_write_1_local_l( __local ulong*, ulong );
void    __builtin_IB_simd_block_write_2_local_l( __local ulong*, ulong2 );
void    __builtin_IB_simd_block_write_4_local_l( __local ulong*, ulong4 );
void    __builtin_IB_simd_block_write_8_local_l( __local ulong*, ulong8 );

uint    __builtin_IB_simd_media_block_read_1( long, int2 );
uint2   __builtin_IB_simd_media_block_read_2( long, int2 );
uint4   __builtin_IB_simd_media_block_read_4( long, int2 );
uint8   __builtin_IB_simd_media_block_read_8( long, int2 );

ushort   __builtin_IB_simd_media_block_read_1_h( long, int2 );
ushort2  __builtin_IB_simd_media_block_read_2_h( long, int2 );
ushort4  __builtin_IB_simd_media_block_read_4_h( long, int2 );
ushort8  __builtin_IB_simd_media_block_read_8_h( long, int2 );
ushort16  __builtin_IB_simd_media_block_read_16_h( long, int2 );

uchar   __builtin_IB_simd_media_block_read_1_b( long, int2 );
uchar2  __builtin_IB_simd_media_block_read_2_b( long, int2 );
uchar4  __builtin_IB_simd_media_block_read_4_b( long, int2 );
uchar8  __builtin_IB_simd_media_block_read_8_b( long, int2 );
uchar16  __builtin_IB_simd_media_block_read_16_b( long, int2 );

ulong   __builtin_IB_simd_media_block_read_1_l( long, int2 );
ulong2  __builtin_IB_simd_media_block_read_2_l( long, int2 );
ulong4  __builtin_IB_simd_media_block_read_4_l( long, int2 );
ulong8  __builtin_IB_simd_media_block_read_8_l( long, int2 );

void    __builtin_IB_media_block_rectangle_read( long image, int2 coords, int blockWidth, int blockHeight, GRFHandle destination );

void    __builtin_IB_simd_media_block_write_1( long, int2, uint );
void    __builtin_IB_simd_media_block_write_2( long, int2, uint2 );
void    __builtin_IB_simd_media_block_write_4( long, int2, uint4 );
void    __builtin_IB_simd_media_block_write_8( long, int2, uint8 );

void    __builtin_IB_simd_media_block_write_1_h( long, int2, ushort );
void    __builtin_IB_simd_media_block_write_2_h( long, int2, ushort2 );
void    __builtin_IB_simd_media_block_write_4_h( long, int2, ushort4 );
void    __builtin_IB_simd_media_block_write_8_h( long, int2, ushort8 );
void    __builtin_IB_simd_media_block_write_16_h( long, int2, ushort16 );

void    __builtin_IB_simd_media_block_write_1_b( long, int2, uchar );
void    __builtin_IB_simd_media_block_write_2_b( long, int2, uchar2 );
void    __builtin_IB_simd_media_block_write_4_b( long, int2, uchar4 );
void    __builtin_IB_simd_media_block_write_8_b( long, int2, uchar8 );
void    __builtin_IB_simd_media_block_write_16_b( long, int2, uchar16 );

void    __builtin_IB_simd_media_block_write_1_l( long, int2, ulong );
void    __builtin_IB_simd_media_block_write_2_l( long, int2, ulong2 );
void    __builtin_IB_simd_media_block_write_4_l( long, int2, ulong4 );
void    __builtin_IB_simd_media_block_write_8_l( long, int2, ulong8 );

uchar   __builtin_IB_media_block_read_uchar(long image, int2 offset, int width, int height);
uchar2  __builtin_IB_media_block_read_uchar2(long image, int2 offset, int width, int height);
uchar4  __builtin_IB_media_block_read_uchar4(long image, int2 offset, int width, int height);
uchar8  __builtin_IB_media_block_read_uchar8(long image, int2 offset, int width, int height);
uchar16 __builtin_IB_media_block_read_uchar16(long image, int2 offset, int width, int height);

ushort   __builtin_IB_media_block_read_ushort(long image, int2 offset, int width, int height);
ushort2  __builtin_IB_media_block_read_ushort2(long image, int2 offset, int width, int height);
ushort4  __builtin_IB_media_block_read_ushort4(long image, int2 offset, int width, int height);
ushort8  __builtin_IB_media_block_read_ushort8(long image, int2 offset, int width, int height);
ushort16 __builtin_IB_media_block_read_ushort16(long image, int2 offset, int width, int height);

uint   __builtin_IB_media_block_read_uint(long image, int2 offset, int width, int height);
uint2  __builtin_IB_media_block_read_uint2(long image, int2 offset, int width, int height);
uint4  __builtin_IB_media_block_read_uint4(long image, int2 offset, int width, int height);
uint8  __builtin_IB_media_block_read_uint8(long image, int2 offset, int width, int height);

ulong   __builtin_IB_media_block_read_ulong(long image, int2 offset, int width, int height);
ulong2  __builtin_IB_media_block_read_ulong2(long image, int2 offset, int width, int height);
ulong4  __builtin_IB_media_block_read_ulong4(long image, int2 offset, int width, int height);
ulong8  __builtin_IB_media_block_read_ulong8(long image, int2 offset, int width, int height);

void __builtin_IB_media_block_write_uchar(long image, int2 offset, int width, int height, uchar pixels);
void __builtin_IB_media_block_write_uchar2(long image, int2 offset, int width, int height, uchar2 pixels);
void __builtin_IB_media_block_write_uchar4(long image, int2 offset, int width, int height, uchar4 pixels);
void __builtin_IB_media_block_write_uchar8(long image, int2 offset, int width, int height, uchar8 pixels);
void __builtin_IB_media_block_write_uchar16(long image, int2 offset, int width, int height, uchar16 pixels);

void __builtin_IB_media_block_write_ushort(long image, int2 offset, int width, int height, ushort pixels);
void __builtin_IB_media_block_write_ushort2(long image, int2 offset, int width, int height, ushort2 pixels);
void __builtin_IB_media_block_write_ushort4(long image, int2 offset, int width, int height, ushort4 pixels);
void __builtin_IB_media_block_write_ushort8(long image, int2 offset, int width, int height, ushort8 pixels);
void __builtin_IB_media_block_write_ushort16(long image, int2 offset, int width, int height, ushort16 pixels);

void __builtin_IB_media_block_write_uint(long image, int2 offset, int width, int height, uint pixels);
void __builtin_IB_media_block_write_uint2(long image, int2 offset, int width, int height, uint2 pixels);
void __builtin_IB_media_block_write_uint4(long image, int2 offset, int width, int height, uint4 pixels);
void __builtin_IB_media_block_write_uint8(long image, int2 offset, int width, int height, uint8 pixels);

void __builtin_IB_media_block_write_ulong(long image, int2 offset, int width, int height, ulong pixels);
void __builtin_IB_media_block_write_ulong2(long image, int2 offset, int width, int height, ulong2 pixels);
void __builtin_IB_media_block_write_ulong4(long image, int2 offset, int width, int height, ulong4 pixels);
void __builtin_IB_media_block_write_ulong8(long image, int2 offset, int width, int height, ulong8 pixels);

int __builtin_IB_dp4a_ss(int c, int a, int b, bool isSaturated) __attribute__((const));
int __builtin_IB_dp4a_uu(int c, int a, int b, bool isSaturated) __attribute__((const));
int __builtin_IB_dp4a_su(int c, int a, int b, bool isSaturated) __attribute__((const));
int __builtin_IB_dp4a_us(int c, int a, int b, bool isSaturated) __attribute__((const));

#define DECL_SUB_GROUP_OPERATION(type, type_abbr, operation, group_type)  \
type   __builtin_IB_sub_group_##group_type##_##operation##_##type_abbr(type x) __attribute__((const));

#define DECL_SUB_GROUP_CLUSTERED_OPERATION(type, type_abbr, operation, group_type)  \
type   __builtin_IB_sub_group_clustered_##group_type##_##operation##_##type_abbr(type x, int cluster_size) __attribute__((const));

#define DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, operation)  \
DECL_SUB_GROUP_OPERATION(type, type_abbr, operation, reduce)   \
DECL_SUB_GROUP_OPERATION(type, type_abbr, operation, scan)     \
DECL_SUB_GROUP_CLUSTERED_OPERATION(type, type_abbr, operation, reduce)

// ARITHMETIC OPERATIONS
// __builtin_IB_sub_group_reduce_IAdd/FAdd
// __builtin_IB_sub_group_scan_IAdd/FAdd
// __builtin_IB_sub_group_clustered_reduce_IAdd/FAdd
// __builtin_IB_sub_group_reduce_IMul/FMul
// __builtin_IB_sub_group_scan_IMul/FMul
// __builtin_IB_sub_group_clustered_reduce_IMul/FMul
#define DECL_SUB_GROUP_ADD_MUL(type, type_abbr, type_sign)  \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, type_sign##Add)  \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, type_sign##Mul)  \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, type_sign##MulKHR)

DECL_SUB_GROUP_ADD_MUL(char,   i8,  I)
DECL_SUB_GROUP_ADD_MUL(short,  i16, I)
DECL_SUB_GROUP_ADD_MUL(int,    i32, I)
DECL_SUB_GROUP_ADD_MUL(long,   i64, I)
DECL_SUB_GROUP_ADD_MUL(float,  f32, F)
#if defined(cl_khr_fp64)
DECL_SUB_GROUP_ADD_MUL(double, f64, F)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DECL_SUB_GROUP_ADD_MUL(half,   f16, F)
#endif // defined(cl_khr_fp16)

// __builtin_IB_sub_group_reduce_SMin/UMin/FMin
// __builtin_IB_sub_group_scan_SMin/UMin/FMin
// __builtin_IB_sub_group_clustered_reduce_SMin/UMin/FMin
// __builtin_IB_sub_group_reduce_SMax/UMax/FMax
// __builtin_IB_sub_group_scan_SMax/UMax/FMax
// __builtin_IB_sub_group_clustered_reduce_SMax/UMax/FMax
#define DECL_SUB_GROUP_MIN_MAX(type, type_abbr, type_sign)  \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, type_sign##Min)  \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, type_sign##Max)

DECL_SUB_GROUP_MIN_MAX(char,   i8, S)
DECL_SUB_GROUP_MIN_MAX(uchar,  i8, U)
DECL_SUB_GROUP_MIN_MAX(short,  i16, S)
DECL_SUB_GROUP_MIN_MAX(ushort, i16, U)
DECL_SUB_GROUP_MIN_MAX(int,    i32, S)
DECL_SUB_GROUP_MIN_MAX(uint,   i32, U)
DECL_SUB_GROUP_MIN_MAX(long,   i64, S)
DECL_SUB_GROUP_MIN_MAX(ulong,  i64, U)
DECL_SUB_GROUP_MIN_MAX(float,  f32, F)
#if defined(cl_khr_fp64)
DECL_SUB_GROUP_MIN_MAX(double, f64, F)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DECL_SUB_GROUP_MIN_MAX(half,   f16, F)
#endif // defined(cl_khr_fp16)

// BITWISE OPERATIONS
// __builtin_IB_sub_group_reduce_BitwiseAnd/Or/Xor
// __builtin_IB_sub_group_scan_BitwiseAnd/Or/Xor
// __builtin_IB_sub_group_clustered_reduce_BitwiseAnd/Or/Xor
#define DECL_BITWISE_OPERATIONS(type, type_abbr)           \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, BitwiseAnd)     \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, BitwiseOr)      \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, BitwiseXor)     \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, BitwiseAndKHR)  \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, BitwiseOrKHR)   \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, BitwiseXorKHR)

DECL_BITWISE_OPERATIONS(char,  i8)
DECL_BITWISE_OPERATIONS(short, i16)
DECL_BITWISE_OPERATIONS(int,   i32)
DECL_BITWISE_OPERATIONS(long,  i64)

// LOGICAL OPERATIONS
// __builtin_IB_sub_group_reduce_LogicalAnd/Or/Xor
// __builtin_IB_sub_group_scan_LogicalAnd/Or/Xor
// __builtin_IB_sub_group_clustered_reduce_LogicalAnd/Or/Xor
#define DECL_LOGICAL_OPERATIONS(type, type_abbr)           \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, LogicalAnd)     \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, LogicalOr)      \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, LogicalXor)     \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, LogicalAndKHR)  \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, LogicalOrKHR)   \
DECL_SUB_GROUP_ALL_GROUPS(type, type_abbr, LogicalXorKHR)

DECL_LOGICAL_OPERATIONS(bool, i1)

// __builtin_IB_sub_group_clustered_scan_IAdd/FAdd
//
// At the moment only Add operation is supported for clustered scan.
// If functionality is extended to match (non-clustered) scan, the macro
// should be moved to DECL_SUB_GROUP_ALL_GROUPS.
#define DECL_SUB_GROUP_CLUSTERED_ADD(type, type_abbr, type_sign) \
type __builtin_IB_sub_group_clustered_scan_##type_sign##Add_##type_abbr(type x, uint cluster_size) __attribute__((const));

DECL_SUB_GROUP_CLUSTERED_ADD(char,   i8,  I)
DECL_SUB_GROUP_CLUSTERED_ADD(short,  i16, I)
DECL_SUB_GROUP_CLUSTERED_ADD(int,    i32, I)
DECL_SUB_GROUP_CLUSTERED_ADD(long,   i64, I)
DECL_SUB_GROUP_CLUSTERED_ADD(float,  f32, F)
#if defined(cl_khr_fp64)
DECL_SUB_GROUP_CLUSTERED_ADD(double, f64, F)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DECL_SUB_GROUP_CLUSTERED_ADD(half,   f16, F)
#endif // defined(cl_khr_fp16)

// The following mul/fma with rtz is used internally for int div/rem emulation
// x * y, using round-to-zero
double __builtin_IB_mul_rtz_f64(double x, double y) __attribute__((const));
float __builtin_IB_mul_rtz_f32(float x, float y) __attribute__((const));
// x + y, using round-to-zero
double __builtin_IB_add_rtz_f64(double x, double y) __attribute__((const));
float __builtin_IB_add_rtz_f32(float x, float y) __attribute__((const));
// x * y + z, using round-to-zero
double __builtin_IB_fma_rtz_f64(double x, double y, double z) __attribute__((const));
float __builtin_IB_fma_rtz_f32(float x, float y, float z) __attribute__((const));
// x * y + z, using round-to-positive-infinity
double __builtin_IB_fma_rtp_f64(double x, double y, double z) __attribute__((const));
// x * y + z, using round-to-negative-infinity
double __builtin_IB_fma_rtn_f64(double x, double y, double z) __attribute__((const));

// system memory fence (PVC+)
void  __builtin_IB_system_memfence(bool fence_typed_memory);

// i64 CAS SLM atomic (PVC+)
long  __builtin_IB_atomic_cmpxchg_local_i64(__local long*, long, long);

#if defined(cl_intel_pvc_rt_validation) || defined(cl_intel_rt_production)
struct rtglobals_t;
typedef __global struct rtglobals_t *rtglobals_t;
struct rtfence_t;
typedef __private struct rtfence_t *rtfence_t;

rtfence_t __builtin_IB_intel_query_rt_fence(intel_ray_query_t rayquery);
rtglobals_t __builtin_IB_intel_query_rt_globals(intel_ray_query_t rayquery);
global void* __builtin_IB_intel_query_rt_stack(intel_ray_query_t rayquery);
uint __builtin_IB_intel_query_ctrl(intel_ray_query_t rayquery);
uint __builtin_IB_intel_query_bvh_level(intel_ray_query_t rayquery);

intel_ray_query_t __builtin_IB_intel_init_ray_query(
    rtfence_t fence,
    rtglobals_t rtglobals,
    global void* rtstack,
    uint ctrl,
    uint bvhLevel);

void __builtin_IB_intel_update_ray_query(
    intel_ray_query_t rayquery,
    rtfence_t fence,
    rtglobals_t rtglobals,
    global void* rtstack,
    uint ctrl,
    uint bvhLevel);

void* __builtin_IB_intel_get_rt_stack(rtglobals_t rt_dispatch_globals);
void* __builtin_IB_intel_get_thread_btd_stack(rtglobals_t rt_dispatch_globals);
void* __builtin_IB_intel_get_global_btd_stack(rtglobals_t rt_dispatch_globals);
rtfence_t __builtin_IB_intel_dispatch_trace_ray_query(
  rtglobals_t rt_dispatch_globals, uint bvh_level, uint traceTayCtrl);
void __builtin_IB_intel_rt_sync(rtfence_t fence);
global void* __builtin_IB_intel_get_rt_global_buffer();
#endif // defined(cl_intel_pvc_rt_validation) || defined(cl_intel_rt_production)

void    __builtin_IB_hdc_uncompressed_write_uchar(__global uchar *buf, uchar val);
float __builtin_IB_tanhf(float x);
float __builtin_IB_sigmf(float x);
half  __builtin_IB_tanhh(half x);
half  __builtin_IB_sigmh(half x);

ushort __builtin_bf16_sin(ushort) __attribute__((overloadable));
ushort __builtin_bf16_cos(ushort) __attribute__((overloadable));
ushort __builtin_bf16_log(ushort) __attribute__((overloadable));
ushort __builtin_bf16_exp(ushort) __attribute__((overloadable));
ushort __builtin_bf16_sqrt(ushort) __attribute__((overloadable));
ushort __builtin_bf16_tanh(ushort) __attribute__((overloadable));
ushort __builtin_bf16_inv(ushort) __attribute__((overloadable));
ushort __builtin_bf16_mul(ushort, ushort) __attribute__((overloadable));
ushort __builtin_bf16_mad(ushort, ushort, ushort) __attribute__((overloadable));
ushort __builtin_bf16_add(ushort, ushort) __attribute__((overloadable));
ushort __builtin_bf16_sub(ushort, ushort) __attribute__((overloadable));
ushort2 __builtin_bf16_sub(ushort2, ushort2) __attribute__((overloadable));
ushort3 __builtin_bf16_sub(ushort3, ushort3) __attribute__((overloadable));
ushort4 __builtin_bf16_sub(ushort4, ushort4) __attribute__((overloadable));
int __builtin_bf16_isless(ushort, ushort) __attribute__((overloadable));

#define LUT_int4_to_bfloat8  0
#define LUT_e2m1_to_bfloat8  1
#define LUT_int4_to_hfloat8  2
#define LUT_e2m1_to_hfloat8  3
#define LUT_int4_to_bfloat16 4
#define LUT_e2m1_to_bfloat16 5
#define LUT_int4_to_hfloat16 6
#define LUT_e2m1_to_hfloat16 7

// converts 4 bit values on LSB of source to hf8 or bf8, depending on the lookup table provided as first argument.
uchar   __builtin_IB_shfl_idx4_to_fp8(uint16 lut, char source) __attribute__((const));
uchar2  __builtin_IB_shfl_idx4_to_fp8_2(uint16 lut, char2 source) __attribute__((const));
uchar4  __builtin_IB_shfl_idx4_to_fp8_4(uint16 lut, char4 source) __attribute__((const));
uchar8  __builtin_IB_shfl_idx4_to_fp8_8(uint16 lut, char8 source) __attribute__((const));
uchar16 __builtin_IB_shfl_idx4_to_fp8_16(uint16 lut, char16 source) __attribute__((const));

// converts 4 bit values on LSB and MSB of source to hf8 or bf8, depending on the lookup table provided as first argument.
ushort   __builtin_IB_shfl_idx4_to_fp8_packed(uint16 lut, char source) __attribute__((const));
ushort2  __builtin_IB_shfl_idx4_to_fp8_2_packed(uint16 lut, char2 source) __attribute__((const));
ushort4  __builtin_IB_shfl_idx4_to_fp8_4_packed(uint16 lut, char4 source) __attribute__((const));
ushort8  __builtin_IB_shfl_idx4_to_fp8_8_packed(uint16 lut, char8 source) __attribute__((const));
ushort16 __builtin_IB_shfl_idx4_to_fp8_16_packed(uint16 lut, char16 source) __attribute__((const));

// converts 4 bit values on LSB of source to hf16 or bf16, depending on the lookup table provided as first argument.
ushort   __builtin_IB_shfl_idx4_to_fp16(uint16 lut, char source) __attribute__((const));
ushort2  __builtin_IB_shfl_idx4_to_fp16_2(uint16 lut, char2 source) __attribute__((const));
ushort4  __builtin_IB_shfl_idx4_to_fp16_4(uint16 lut, char4 source) __attribute__((const));
ushort8  __builtin_IB_shfl_idx4_to_fp16_8(uint16 lut, char8 source) __attribute__((const));
ushort16 __builtin_IB_shfl_idx4_to_fp16_16(uint16 lut, char16 source) __attribute__((const));

// converts 4 bit values on LSB and MSB of source to hf16 or bf16, depending on the lookup table provided as first argument.
uint   __builtin_IB_shfl_idx4_to_fp16_packed(uint16 lut, char source) __attribute__((const));
uint2  __builtin_IB_shfl_idx4_to_fp16_2_packed(uint16 lut, char2 source) __attribute__((const));
uint4  __builtin_IB_shfl_idx4_to_fp16_4_packed(uint16 lut, char4 source) __attribute__((const));
uint8  __builtin_IB_shfl_idx4_to_fp16_8_packed(uint16 lut, char8 source) __attribute__((const));
uint16 __builtin_IB_shfl_idx4_to_fp16_16_packed(uint16 lut, char16 source) __attribute__((const));

uint16 __builtin_IB_shfl_idx4_lut(int lut_index) __attribute__((const));

#include "IGCBiF_Intrinsics_Dpas.cl"
#include "IGCBiF_Intrinsics_Lsc.cl"

#endif // IGCBIF_INTRINSICS_CL
