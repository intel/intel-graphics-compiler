/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//*****************************************************************************/
// Generic Header
//*****************************************************************************/
#include "IBiF_Header.h"

//*****************************************************************************/
// Explicit conversion functions
//*****************************************************************************/
#include "IBiF_Conversions.cl"

//*****************************************************************************/
// Misc. vector functions
//*****************************************************************************/
#include "IBiF_Shuffle.cl"

//*****************************************************************************/
// Device enqueue functions
//*****************************************************************************/
#include "IBiF_Device_Enqueue.cl"
#include "IBiF_Device_Enqueue_IGIL.cl"

//*****************************************************************************/
// Image Atomics (Intel Vendor Extension)
//*****************************************************************************/

//*****************************************************************************/
// Images
//*****************************************************************************/
#include "IBiF_Images.cl"

//*****************************************************************************/
// SPIRV Utils functions
//*****************************************************************************/
#include "IBiF_SPIRV_Utils.h"

//*****************************************************************************/
// Math builtin functions
//*****************************************************************************/
#include "IBiF_Math_Common.cl"

//*****************************************************************************/
// 64bit Math Emulation
//*****************************************************************************/
#ifdef __IGC_BUILD__
// #include "IGCBiF_Math_64bitDiv.cl"
#endif

//*****************************************************************************/
// Sub Group functions
//*****************************************************************************/
#include "IBiF_Sub_Groups.cl"
#include "IBiF_Sub_Group_Shuffle.cl"
#include "IBiF_Sub_Group_Shuffle_Relative.cl"
#include "IBiF_Sub_Group_WorkItem.cl"

//*****************************************************************************/
// Sub Group and Work Group Reduce and Scan functions
//*****************************************************************************/
#include "IBiF_Reduce_Scan.cl"

//*****************************************************************************/
// Atomic Functions
//*****************************************************************************/
#include "IBiF_Atomics.cl"

//*****************************************************************************/
// Intel production raytracing extension
//*****************************************************************************/
#include "IBiF_intel_rt_utils.cl"
#include "IBiF_intel_rt_production.cl"
#include "IBiF_Barrier.cl"
#include "IBiF_Pipe.cl"


//*****************************************************************************/
// Work-Item function
//*****************************************************************************/

// All other work-item functions have size_t in their signature so they have
// to be compiled in the size_t .bc files.  This is here so its code is not
// in both the 32-bit and 64-bit .bcs.
INLINE uint OVERLOADABLE get_work_dim() { return __builtin_IB_get_work_dim(); }

//*****************************************************************************/
// generic address space function overloads
//*****************************************************************************/

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE local void *__to_local(generic void *ptr) {
  return __spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((ptr), __generic char *), StorageWorkgroup);
}

INLINE private void *__to_private(generic void *ptr) {
  return __spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((ptr), __generic char *), StorageFunction);
}

INLINE global void *__to_global(generic void *ptr) {
  return __spirv_GenericCastToPtrExplicit_ToGlobal(__builtin_astype((ptr), __generic char *), StorageCrossWorkgroup);
}

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

//*****************************************************************************/
// Workgroup functions
//*****************************************************************************/

//*****************************************************************************/
// Pipe functions
//*****************************************************************************/

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

/////////////////////////////////////////////////////////////////////
// Basic Reads and Writes

/////////////////////////////////////////////////////////////////////
// Reservation Query

/////////////////////////////////////////////////////////////////////
// Work Item Reservations

/////////////////////////////////////////////////////////////////////
// Reads and Writes with Reservations
// The reservation functions lock the _pipe, so we don't need to
// re-lock here.

/////////////////////////////////////////////////////////////////////
// Work Group Reservations
//
bool __intel_is_first_work_group_item(void);

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

//*****************************************************************************
// Clock functions
//*****************************************************************************

#ifdef cl_khr_kernel_clock

// Device scope
#ifdef __opencl_c_kernel_clock_scope_device
__attribute__((always_inline)) ulong OVERLOADABLE clock_read_device(void) {
  return __spirv_ReadClockKHR_Rulong(Device);
}

__attribute__((always_inline)) uint2 OVERLOADABLE clock_read_hilo_device(void) {
  return __spirv_ReadClockKHR_Ruint2(Device);
}
#endif // __opencl_c_kernel_clock_scope_device

// Workgroup scope
#ifdef __opencl_c_kernel_clock_scope_work_group
__attribute__((always_inline)) ulong OVERLOADABLE clock_read_work_group(void) {
  return __spirv_ReadClockKHR_Rulong(Workgroup);
}

__attribute__((always_inline)) uint2 OVERLOADABLE clock_read_hilo_work_group(void) {
  return __spirv_ReadClockKHR_Ruint2(Workgroup);
}
#endif // __opencl_c_kernel_clock_scope_work_group

// Subgroup scope
#ifdef __opencl_c_kernel_clock_scope_sub_group
__attribute__((always_inline)) ulong OVERLOADABLE clock_read_sub_group(void) {
  return __spirv_ReadClockKHR_Rulong(Subgroup);
}

__attribute__((always_inline)) uint2 OVERLOADABLE clock_read_hilo_sub_group(void) {
  return __spirv_ReadClockKHR_Ruint2(Subgroup);
}
#endif // __opencl_c_kernel_clock_scope_sub_group

#endif // cl_khr_kernel_clock

// Internal Debug Built-Ins (TODO: implement for IGC?)

__attribute__((always_inline)) ulong OVERLOADABLE intel_get_cycle_counter(void) {
  return __spirv_ReadClockKHR_Rulong(0);
}

__attribute__((always_inline)) uint OVERLOADABLE intel_get_active_channel_mask(void) {
  return __builtin_IB_WaveBallot(true);
}

__attribute__((always_inline)) void OVERLOADABLE intel_source_value(uchar value) { __builtin_IB_source_value(value); }

__attribute__((always_inline)) void OVERLOADABLE intel_source_value(ushort value) { __builtin_IB_source_value(value); }

__attribute__((always_inline)) void OVERLOADABLE intel_source_value(uint value) { __builtin_IB_source_value(value); }

__attribute__((always_inline)) void OVERLOADABLE intel_source_value(ulong value) { __builtin_IB_source_value(value); }

// dbg0_0 will be written to dbg0.0
// the return result will be dbg0.1 (start IP of the thread)
__attribute__((always_inline)) uint OVERLOADABLE intel_set_dbg_register(uint dbg0_0) {
  return __builtin_IB_set_dbg_register(dbg0_0);
}

__attribute__((always_inline)) uint OVERLOADABLE intel_get_grf_register(uint value) {
  return __builtin_IB_movreg(value);
}

__attribute__((always_inline)) uint OVERLOADABLE intel_get_flag_register(uint flag) {
  return __builtin_IB_movflag(flag);
}

__attribute__((always_inline)) uint OVERLOADABLE intel_get_control_register(uint value) {
  return __builtin_IB_movcr(value);
}

__attribute__((always_inline)) uint OVERLOADABLE intel_get_hw_thread_id(void) { return __builtin_IB_hw_thread_id(); }

__attribute__((always_inline)) uint OVERLOADABLE intel_get_slice_id(void) { return __builtin_IB_slice_id(); }

__attribute__((always_inline)) uint OVERLOADABLE intel_get_subslice_id(void) { return __builtin_IB_subslice_id(); }

__attribute__((always_inline)) uint OVERLOADABLE intel_get_dual_subslice_id(void) {
  return __builtin_IB_dual_subslice_id();
}

__attribute__((always_inline)) uint OVERLOADABLE intel_get_eu_id(void) { return __builtin_IB_eu_id(); }

__attribute__((always_inline)) void OVERLOADABLE intel_profile_snapshot(int point_type, int point_index) {
  __builtin_IB_profile_snapshot(point_type, point_index);
}

__attribute__((always_inline)) void OVERLOADABLE intel_profile_aggregated(int point_type, int point_index) {
  __builtin_IB_profile_aggregated(point_type, point_index);
}

__attribute__((always_inline)) uint OVERLOADABLE intel_get_eu_thread_id(void) { return __builtin_IB_eu_thread_id(); }

__attribute__((always_inline)) void OVERLOADABLE intel_eu_thread_pause(uint value) {
  __builtin_IB_eu_thread_pause(value);
}

__attribute__((always_inline)) uint OVERLOADABLE intel_get_tile_id(void) { return __spirv_BuiltInSubDeviceIDINTEL(); }

// Temporary workaround on a test that uses __builtin_spirv_BuiltInSubDeviceIDINTEL
// instead of intel_get_tile_id.
__attribute__((always_inline)) uint __builtin_spirv_BuiltInSubDeviceIDINTEL(void) { return intel_get_tile_id(); }
