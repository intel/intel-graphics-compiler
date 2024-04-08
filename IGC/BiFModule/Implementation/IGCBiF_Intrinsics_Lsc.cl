/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCBIF_INTRINSICS_LSC_CL
#define IGCBIF_INTRINSICS_LSC_CL

////////////////////////////////////////////////////////////////////////
// LSC (Load/Store Cache) intrinsics
//
// This set of intrinsics maps access DG2/PVC+ LSC messages.
// The general scheme is that we use the SPIR-V data type to infer
// the LSC type and vector combination.  The following table represents
// the mapping for load and store messages.
//
// +-------------+----------+---------------------------------------
// | OpenCL Type | LSC Type |  Notes
// +-------------+----------+---------------------------------------
// | uchar+      |  D8U32   | 32b in the register file; 8b in memory
// | ushort+     |  D16U32  | 32b in the register file; 16b in memory
// | uint        |  D32 V1  |
// | uint2       |  D32 V2  |
// | uint3       |  D32 V3  |
// | uint4       |  D32 V4  |
// | uint8+      |  D32 V8  | May split into several submessages, unless half SIMD (B0+)
// | ulong       |  D64 V1  |
// | ulong2      |  D64 V2  |
// | ulong3+     |  D64 V3  | Will split into several submessages, unless half SIMD (B0+)
// | ulong4+     |  D64 V4  | Will split into several submessages
// | ulong8++    |  D64 V8  | Will split into several submessages
// +-------------+----------+---------------------------------------
//    + no native D8 and D16 load/store support exists at the moment;
//      also, upper bytes of DW may be garbage depending on platform (later platform fixes)
//    ++ can split into several messages
//
// *** Cache Controls ***
//   Cache controls are present in most messages.  Some have additional
//   constraints not listed here, which may vary per platform and stepping.
//
// *** Uniform Immediate Offset ***
//   An additional immediate offset (in elements) parameter is also provided.
//   Semantically this is no different than adding to the base pointer,
//   but hardware may be able to fuse the add into the message.
//   As the name implies, the argument must be uniform and immediate
//   (not variable).
//
// *** Other Data Types ***
//   Only unsigned types are needed for most load and store operations.
//   Use OpenCL reinterpretation functions to convert to signed and float
//   types (e.g. as_float4(...)).
//
//   Atomics favor unsigned for untyped integer operations such as bitwise ops,
//   (integer) add and so forth.  Again, use as_int(..) to convert.
//   For explict operations such as signed min/max and floating-point add/sub
//   (among others) signed and floating point types are used and no conversion
//   is necessary.
//
// *** Additional Restrictions ***
//   Hardware documentation contains additional constraints; platforms
//   and stepping may contain addition restrictions not enforced here.
//   In such cases, the results of the operation are undefined.

///////////////////////////////////////////////////////////////////////
// LSC Cache options
//    Those values are in API intrinsics and need to be explicit.
//    Those values should match exactly to ones used in IGC
//    (given in igc_regkeys_enums_defs.h).
///////////////////////////////////////////////////////////////////////
//
// Load message caching control
enum LSC_LDCC {
    LSC_LDCC_DEFAULT      = 0,
    LSC_LDCC_L1UC_L3UC    = 1,   // Override to L1 uncached and L3 uncached
    LSC_LDCC_L1UC_L3C     = 2,   // Override to L1 uncached and L3 cached
    LSC_LDCC_L1C_L3UC     = 3,   // Override to L1 cached and L3 uncached
    LSC_LDCC_L1C_L3C      = 4,   // Override to L1 cached and L3 cached
    LSC_LDCC_L1S_L3UC     = 5,   // Override to L1 streaming load and L3 uncached
    LSC_LDCC_L1S_L3C      = 6,   // Override to L1 streaming load and L3 cached
    LSC_LDCC_L1IAR_L3C    = 7,   // Override to L1 invalidate-after-read, and L3 cached
};

// Store message caching control (also used for atomics)
enum LSC_STCC {
    LSC_STCC_DEFAULT      = 0,
    LSC_STCC_L1UC_L3UC    = 1,   // Override to L1 uncached and L3 uncached
    LSC_STCC_L1UC_L3WB    = 2,   // Override to L1 uncached and L3 written back
    LSC_STCC_L1WT_L3UC    = 3,   // Override to L1 written through and L3 uncached
    LSC_STCC_L1WT_L3WB    = 4,   // Override to L1 written through and L3 written back
    LSC_STCC_L1S_L3UC     = 5,   // Override to L1 streaming and L3 uncached
    LSC_STCC_L1S_L3WB     = 6,   // Override to L1 streaming and L3 written back
    LSC_STCC_L1WB_L3WB    = 7,   // Override to L1 written through and L3 written back
};

#ifdef cl_intel_pvc_lsc_validation
///////////////////////////////////////////////////////////////////////
// LSC Loads
///////////////////////////////////////////////////////////////////////
// global address space gathering load
uint    __builtin_IB_lsc_load_global_uchar_to_uint (const __global uchar  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D8U32
uint    __builtin_IB_lsc_load_global_ushort_to_uint(const __global ushort *base, int immElemOff, enum LSC_LDCC cacheOpt); //D16U32
uint    __builtin_IB_lsc_load_global_uint  (const __global uint   *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V1
uint2   __builtin_IB_lsc_load_global_uint2 (const __global uint2  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V2
uint3   __builtin_IB_lsc_load_global_uint3 (const __global uint3  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V3
uint4   __builtin_IB_lsc_load_global_uint4 (const __global uint4  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V4
uint8   __builtin_IB_lsc_load_global_uint8 (const __global uint8  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V8
ulong   __builtin_IB_lsc_load_global_ulong (const __global ulong  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V1
ulong2  __builtin_IB_lsc_load_global_ulong2(const __global ulong2 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V2
ulong3  __builtin_IB_lsc_load_global_ulong3(const __global ulong3 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V3
ulong4  __builtin_IB_lsc_load_global_ulong4(const __global ulong4 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V4
ulong8  __builtin_IB_lsc_load_global_ulong8(const __global ulong8 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V8

uint    __builtin_IB_lsc_load_cmask_global_uint  (const __global uint   *base, int elemOff, enum LSC_LDCC cacheOpt); //D32V1
uint    __builtin_IB_lsc_load_cmask_global_uint2 (const __global uint2  *base, int elemOff, enum LSC_LDCC cacheOpt); //D32V2
uint    __builtin_IB_lsc_load_cmask_global_uint3 (const __global uint3  *base, int elemOff, enum LSC_LDCC cacheOpt); //D32V3
uint    __builtin_IB_lsc_load_cmask_global_uint4 (const __global uint4  *base, int elemOff, enum LSC_LDCC cacheOpt); //D32V4
uint    __builtin_IB_lsc_load_cmask_global_ulong (const __global ulong  *base, int elemOff, enum LSC_LDCC cacheOpt); //D64V1
uint    __builtin_IB_lsc_load_cmask_global_ulong2(const __global ulong2 *base, int elemOff, enum LSC_LDCC cacheOpt); //D64V2
uint    __builtin_IB_lsc_load_cmask_global_ulong3(const __global ulong3 *base, int elemOff, enum LSC_LDCC cacheOpt); //D64V3
uint    __builtin_IB_lsc_load_cmask_global_ulong4(const __global ulong4 *base, int elemOff, enum LSC_LDCC cacheOpt); //D64V4

// global block load (one-dimensional)
uint    __builtin_IB_lsc_load_block_global_uchar_to_uint (const __global uchar  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D8U32
uint    __builtin_IB_lsc_load_block_global_ushort_to_uint(const __global ushort *base, int immElemOff, enum LSC_LDCC cacheOpt); //D16U32
uint    __builtin_IB_lsc_load_block_global_uint  (const __global uint   *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V1
uint2   __builtin_IB_lsc_load_block_global_uint2 (const __global uint2  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V2
uint3   __builtin_IB_lsc_load_block_global_uint3 (const __global uint3  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V3
uint4   __builtin_IB_lsc_load_block_global_uint4 (const __global uint4  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V4
uint8   __builtin_IB_lsc_load_block_global_uint8 (const __global uint8  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V8
ulong   __builtin_IB_lsc_load_block_global_ulong (const __global ulong  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V1
ulong2  __builtin_IB_lsc_load_block_global_ulong2(const __global ulong2 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V2
ulong3  __builtin_IB_lsc_load_block_global_ulong3(const __global ulong3 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V3
ulong4  __builtin_IB_lsc_load_block_global_ulong4(const __global ulong4 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V4
ulong8  __builtin_IB_lsc_load_block_global_ulong8(const __global ulong8 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V8


// local address space
uint    __builtin_IB_lsc_load_local_uchar_to_uint( const __local  uchar *base, int immElemOff); //D8U32
uint    __builtin_IB_lsc_load_local_ushort_to_uint(const __local ushort *base, int immElemOff); //D16U32
uint    __builtin_IB_lsc_load_local_uint  (const __local uint   *base, int immElemOff); //D32V1
uint2   __builtin_IB_lsc_load_local_uint2 (const __local uint2  *base, int immElemOff); //D32V2
uint3   __builtin_IB_lsc_load_local_uint3 (const __local uint3  *base, int immElemOff); //D32V3
uint4   __builtin_IB_lsc_load_local_uint4 (const __local uint4  *base, int immElemOff); //D32V4
uint8   __builtin_IB_lsc_load_local_uint8 (const __local uint8  *base, int immElemOff); //D32V8
ulong   __builtin_IB_lsc_load_local_ulong (const __local ulong  *base, int immElemOff); //D64V1
ulong2  __builtin_IB_lsc_load_local_ulong2(const __local ulong2 *base, int immElemOff); //D64V2
ulong3  __builtin_IB_lsc_load_local_ulong3(const __local ulong3 *base, int immElemOff); //D64V3
ulong4  __builtin_IB_lsc_load_local_ulong4(const __local ulong4 *base, int immElemOff); //D64V4
ulong8  __builtin_IB_lsc_load_local_ulong8(const __local ulong8 *base, int immElemOff); //D64V8

uint    __builtin_IB_lsc_load_cmask_global_uint  (const __global uint   *base, int elemOff, enum LSC_LDCC cacheOpt); //D32V1
uint    __builtin_IB_lsc_load_cmask_global_uint2 (const __global uint2  *base, int elemOff, enum LSC_LDCC cacheOpt); //D32V2
uint    __builtin_IB_lsc_load_cmask_global_uint3 (const __global uint3  *base, int elemOff, enum LSC_LDCC cacheOpt); //D32V3
uint    __builtin_IB_lsc_load_cmask_global_uint4 (const __global uint4  *base, int elemOff, enum LSC_LDCC cacheOpt); //D32V4
uint    __builtin_IB_lsc_load_cmask_global_ulong (const __global ulong  *base, int elemOff, enum LSC_LDCC cacheOpt); //D64V1
uint    __builtin_IB_lsc_load_cmask_global_ulong2(const __global ulong2 *base, int elemOff, enum LSC_LDCC cacheOpt); //D64V2
uint    __builtin_IB_lsc_load_cmask_global_ulong3(const __global ulong3 *base, int elemOff, enum LSC_LDCC cacheOpt); //D64V3
uint    __builtin_IB_lsc_load_cmask_global_ulong4(const __global ulong4 *base, int elemOff, enum LSC_LDCC cacheOpt); //D64V4

///////////////////////////////////////////////////////////////////////
// LSC Stores
///////////////////////////////////////////////////////////////////////
// global address space scattering store
void  __builtin_IB_lsc_store_global_uchar_from_uint (__global uchar  *base, int immElemOff, uint val, enum LSC_STCC cacheOpt); //D8U32
void  __builtin_IB_lsc_store_global_ushort_from_uint(__global ushort *base, int immElemOff, uint val, enum LSC_STCC cacheOpt); //D16U32
void  __builtin_IB_lsc_store_global_uint  (__global uint   *base, int immElemOff, uint   val, enum LSC_STCC cacheOpt); //D32V1
void  __builtin_IB_lsc_store_global_uint2 (__global uint2  *base, int immElemOff, uint2  val, enum LSC_STCC cacheOpt); //D32V2
void  __builtin_IB_lsc_store_global_uint3 (__global uint3  *base, int immElemOff, uint3  val, enum LSC_STCC cacheOpt); //D32V3
void  __builtin_IB_lsc_store_global_uint4 (__global uint4  *base, int immElemOff, uint4  val, enum LSC_STCC cacheOpt); //D32V4
void  __builtin_IB_lsc_store_global_uint8 (__global uint8  *base, int immElemOff, uint8  val, enum LSC_STCC cacheOpt); //D32V8
void  __builtin_IB_lsc_store_global_ulong (__global ulong  *base, int immElemOff, ulong  val, enum LSC_STCC cacheOpt); //D64V1
void  __builtin_IB_lsc_store_global_ulong2(__global ulong2 *base, int immElemOff, ulong2 val, enum LSC_STCC cacheOpt); //D64V2
void  __builtin_IB_lsc_store_global_ulong3(__global ulong3 *base, int immElemOff, ulong3 val, enum LSC_STCC cacheOpt); //D64V3
void  __builtin_IB_lsc_store_global_ulong4(__global ulong4 *base, int immElemOff, ulong4 val, enum LSC_STCC cacheOpt); //D64V4
void  __builtin_IB_lsc_store_global_ulong8(__global ulong8 *base, int immElemOff, ulong8 val, enum LSC_STCC cacheOpt); //D64V8

void  __builtin_IB_lsc_store_cmask_global_uint  (__global uint   *base, int elemOff, uint   val, enum LSC_STCC cacheOpt); //D32V1
void  __builtin_IB_lsc_store_cmask_global_uint2 (__global uint2  *base, int elemOff, uint2  val, enum LSC_STCC cacheOpt); //D32V2
void  __builtin_IB_lsc_store_cmask_global_uint3 (__global uint3  *base, int elemOff, uint3  val, enum LSC_STCC cacheOpt); //D32V3
void  __builtin_IB_lsc_store_cmask_global_uint4 (__global uint4  *base, int elemOff, uint4  val, enum LSC_STCC cacheOpt); //D32V4
void  __builtin_IB_lsc_store_cmask_global_ulong (__global ulong  *base, int elemOff, ulong  val, enum LSC_STCC cacheOpt); //D64V1
void  __builtin_IB_lsc_store_cmask_global_ulong2(__global ulong2 *base, int elemOff, ulong2 val, enum LSC_STCC cacheOpt); //D64V2
void  __builtin_IB_lsc_store_cmask_global_ulong3(__global ulong3 *base, int elemOff, ulong3 val, enum LSC_STCC cacheOpt); //D64V3
void  __builtin_IB_lsc_store_cmask_global_ulong4(__global ulong4 *base, int elemOff, ulong4 val, enum LSC_STCC cacheOpt); //D64V4

// global block store (one-dimensional)
void  __builtin_IB_lsc_store_block_global_uchar_from_uint (__global uchar  *base, int immElemOff, uint val, enum LSC_STCC cacheOpt); //D8U32
void  __builtin_IB_lsc_store_block_global_ushort_from_uint(__global ushort *base, int immElemOff, uint val, enum LSC_STCC cacheOpt); //D16U32
void  __builtin_IB_lsc_store_block_global_uint  (__global uint   *base, int immElemOff, uint   val, enum LSC_STCC cacheOpt); //D32V1
void  __builtin_IB_lsc_store_block_global_uint2 (__global uint2  *base, int immElemOff, uint2  val, enum LSC_STCC cacheOpt); //D32V2
void  __builtin_IB_lsc_store_block_global_uint3 (__global uint3  *base, int immElemOff, uint3  val, enum LSC_STCC cacheOpt); //D32V3
void  __builtin_IB_lsc_store_block_global_uint4 (__global uint4  *base, int immElemOff, uint4  val, enum LSC_STCC cacheOpt); //D32V4
void  __builtin_IB_lsc_store_block_global_uint8 (__global uint8  *base, int immElemOff, uint8  val, enum LSC_STCC cacheOpt); //D32V8
void  __builtin_IB_lsc_store_block_global_ulong (__global ulong  *base, int immElemOff, ulong  val, enum LSC_STCC cacheOpt); //D64V1
void  __builtin_IB_lsc_store_block_global_ulong2(__global ulong2 *base, int immElemOff, ulong2 val, enum LSC_STCC cacheOpt); //D64V2
void  __builtin_IB_lsc_store_block_global_ulong3(__global ulong3 *base, int immElemOff, ulong3 val, enum LSC_STCC cacheOpt); //D64V3
void  __builtin_IB_lsc_store_block_global_ulong4(__global ulong4 *base, int immElemOff, ulong4 val, enum LSC_STCC cacheOpt); //D64V4
void  __builtin_IB_lsc_store_block_global_ulong8(__global ulong8 *base, int immElemOff, ulong8 val, enum LSC_STCC cacheOpt); //D64V8

// local address space
void  __builtin_IB_lsc_store_local_uchar_from_uint (__local  uchar *base, int immElemOff, uint val); //D8U32
void  __builtin_IB_lsc_store_local_ushort_from_uint(__local ushort *base, int immElemOff, uint val); //D16U32
void  __builtin_IB_lsc_store_local_uint  (__local uint   *base, int immElemOff, uint   val); //D32V1
void  __builtin_IB_lsc_store_local_uint2 (__local uint2  *base, int immElemOff, uint2  val); //D32V2
void  __builtin_IB_lsc_store_local_uint3 (__local uint3  *base, int immElemOff, uint3  val); //D32V3
void  __builtin_IB_lsc_store_local_uint4 (__local uint4  *base, int immElemOff, uint4  val); //D32V4
void  __builtin_IB_lsc_store_local_uint8 (__local uint8  *base, int immElemOff, uint8  val); //D32V8
void  __builtin_IB_lsc_store_local_ulong (__local ulong  *base, int immElemOff, ulong  val); //D64V1
void  __builtin_IB_lsc_store_local_ulong2(__local ulong2 *base, int immElemOff, ulong2 val); //D64V2
void  __builtin_IB_lsc_store_local_ulong3(__local ulong3 *base, int immElemOff, ulong3 val); //D64V3
void  __builtin_IB_lsc_store_local_ulong4(__local ulong4 *base, int immElemOff, ulong4 val); //D64V4
void  __builtin_IB_lsc_store_local_ulong8(__local ulong8 *base, int immElemOff, ulong8 val); //D64V8

void  __builtin_IB_lsc_store_cmask_local_uint  (__local uint   *base, int elemOff, uint   val, enum LSC_STCC cacheOpt); //D32V1
void  __builtin_IB_lsc_store_cmask_local_uint2 (__local uint2  *base, int elemOff, uint2  val, enum LSC_STCC cacheOpt); //D32V2
void  __builtin_IB_lsc_store_cmask_local_uint3 (__local uint3  *base, int elemOff, uint3  val, enum LSC_STCC cacheOpt); //D32V3
void  __builtin_IB_lsc_store_cmask_local_uint4 (__local uint4  *base, int elemOff, uint4  val, enum LSC_STCC cacheOpt); //D32V4
void  __builtin_IB_lsc_store_cmask_local_ulong (__local ulong  *base, int elemOff, ulong  val, enum LSC_STCC cacheOpt); //D64V1
void  __builtin_IB_lsc_store_cmask_local_ulong2(__local ulong2 *base, int elemOff, ulong2 val, enum LSC_STCC cacheOpt); //D64V2
void  __builtin_IB_lsc_store_cmask_local_ulong3(__local ulong3 *base, int elemOff, ulong3 val, enum LSC_STCC cacheOpt); //D64V3
void  __builtin_IB_lsc_store_cmask_local_ulong4(__local ulong4 *base, int elemOff, ulong4 val, enum LSC_STCC cacheOpt); //D64V4

///////////////////////////////////////////////////////////////////////
// prefetching
///////////////////////////////////////////////////////////////////////
//
// LSC Pre-Fetch Load functions with CacheControls
//     global address space
void __builtin_IB_lsc_prefetch_global_uchar (const __global uchar  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D8U32
void __builtin_IB_lsc_prefetch_global_ushort(const __global ushort *base, int immElemOff, enum LSC_LDCC cacheOpt); //D16U32
void __builtin_IB_lsc_prefetch_global_uint  (const __global uint   *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V1
void __builtin_IB_lsc_prefetch_global_uint2 (const __global uint2  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V2
void __builtin_IB_lsc_prefetch_global_uint3 (const __global uint3  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V3
void __builtin_IB_lsc_prefetch_global_uint4 (const __global uint4  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V4
void __builtin_IB_lsc_prefetch_global_uint8 (const __global uint8  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V8
void __builtin_IB_lsc_prefetch_global_ulong (const __global ulong  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V1
void __builtin_IB_lsc_prefetch_global_ulong2(const __global ulong2 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V2
void __builtin_IB_lsc_prefetch_global_ulong3(const __global ulong3 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V3
void __builtin_IB_lsc_prefetch_global_ulong4(const __global ulong4 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V4
void __builtin_IB_lsc_prefetch_global_ulong8(const __global ulong8 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V8

// Global address space prefetch, returning a boolean status
// value per subgroup item.
// Returns True is returned if addr+immOff was in-bounds of the TRTT.
bool __builtin_IB_lsc_load_status_global_uchar (const __global uchar  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D8U32
bool __builtin_IB_lsc_load_status_global_ushort(const __global ushort *base, int immElemOff, enum LSC_LDCC cacheOpt); //D16U32
bool __builtin_IB_lsc_load_status_global_uint  (const __global uint   *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V1
bool __builtin_IB_lsc_load_status_global_uint2 (const __global uint2  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V2
bool __builtin_IB_lsc_load_status_global_uint3 (const __global uint3  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V3
bool __builtin_IB_lsc_load_status_global_uint4 (const __global uint4  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V4
bool __builtin_IB_lsc_load_status_global_uint8 (const __global uint8  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D32V8
bool __builtin_IB_lsc_load_status_global_ulong (const __global ulong  *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V1
bool __builtin_IB_lsc_load_status_global_ulong2(const __global ulong2 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V2
bool __builtin_IB_lsc_load_status_global_ulong3(const __global ulong3 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V3
bool __builtin_IB_lsc_load_status_global_ulong4(const __global ulong4 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V4
bool __builtin_IB_lsc_load_status_global_ulong8(const __global ulong8 *base, int immElemOff, enum LSC_LDCC cacheOpt); //D64V8

///////////////////////////////////////////////////////////////////////
// LSC Fence support
///////////////////////////////////////////////////////////////////////

// FS - Fence Scope
enum LSC_FS {
    LSC_FS_THREAD_GROUP,
    LSC_FS_LOCAL,
    LSC_FS_TILE,
    LSC_FS_GPU,
    LSC_FS_GPUs,
    LSC_FS_SYSTEM_RELEASE,
    LSC_FS_SYSTEM_ACQUIRE
};

// FT - Fence Type
enum LSC_FT {
    LSC_FT_DEFAULT,
    LSC_FT_EVICT,
    LSC_FT_INVALIDATE,
    LSC_FT_DISCARD,
    LSC_FT_CLEAN,
    LSC_FT_L3
};

void  __builtin_IB_lsc_fence_global_untyped(enum LSC_FS scope, enum LSC_FT flushType);   // Mem Port - UGM
void  __builtin_IB_lsc_fence_global_untyped_cross_tile(enum LSC_FS scope, enum LSC_FT flushType);  // Mem Port - UGML
void  __builtin_IB_lsc_fence_global_typed(enum LSC_FS scope, enum LSC_FT flushType);     // Mem Port - TGM
void  __builtin_IB_lsc_fence_local();                                                    // Mem Port - SLM
void  __builtin_IB_lsc_fence_evict_to_memory();                                          // Mem Port - UGM

///////////////////////////////////////////////////////////////////////
// LSC atomics
///////////////////////////////////////////////////////////////////////

//////////////////////////////
// floating point
//////////////////////////////

// FP32 global
float __builtin_IB_lsc_atomic_add_global_float(volatile __global float *base, int immElemOff, float val, enum LSC_STCC cacheOpt);
float __builtin_IB_lsc_atomic_sub_global_float(volatile __global float *base, int immElemOff, float val, enum LSC_STCC cacheOpt);
float __builtin_IB_lsc_atomic_min_global_float(volatile __global float *base, int immElemOff, float val, enum LSC_STCC cacheOpt);
float __builtin_IB_lsc_atomic_max_global_float(volatile __global float *base, int immElemOff, float val, enum LSC_STCC cacheOpt);
float __builtin_IB_lsc_atomic_cmpxchg_global_float(volatile __global float *base, int immElemOff, float cmp, float val, enum LSC_STCC cacheOpt);

// FP32 local
float __builtin_IB_lsc_atomic_min_local_float(volatile __local float *base, int immElemOff, float val);
float __builtin_IB_lsc_atomic_max_local_float(volatile __local float *base, int immElemOff, float val);
float __builtin_IB_lsc_atomic_cmpxchg_local_float(volatile __local float *base, int immElemOff, float cmp, float val);

// FP64 add,sub atomic support
double  __builtin_IB_lsc_atomic_add_global_double(volatile __global double *base, int immElemOff, double val, enum LSC_STCC cacheOpt);
double  __builtin_IB_lsc_atomic_sub_global_double(volatile __global double *base, int immElemOff, double val, enum LSC_STCC cacheOpt);

//////////////////////////////
// integer
//////////////////////////////

// LSC I16 atomics global
uint  __builtin_IB_lsc_atomic_inc_global_ushort_from_uint(volatile __global ushort *base, int immElemOff,           enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_dec_global_ushort_from_uint(volatile __global ushort *base, int immElemOff,           enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_add_global_ushort_from_uint(volatile __global ushort *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_sub_global_ushort_from_uint(volatile __global ushort *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
int   __builtin_IB_lsc_atomic_min_global_short_from_int  (volatile __global  short *base, int immElemOff,  int val, enum LSC_STCC cacheOpt);
int   __builtin_IB_lsc_atomic_max_global_short_from_int  (volatile __global  short *base, int immElemOff,  int val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_min_global_ushort_from_uint(volatile __global ushort *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_max_global_ushort_from_uint(volatile __global ushort *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_and_global_ushort_from_uint(volatile __global ushort *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_or_global_ushort_from_uint (volatile __global ushort *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_xor_global_ushort_from_uint(volatile __global ushort *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_load_global_ushort_from_uint(volatile __global ushort *base, int immElemOff,           enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_store_global_ushort_from_uint(volatile __global ushort *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_cmpxchg_global_ushort_from_uint(volatile __global ushort *base, int immElemOff, uint cmp, uint val, enum LSC_STCC cacheOpt);


// LSC I16 atomics local
uint  __builtin_IB_lsc_atomic_inc_local_ushort_from_uint(volatile __local ushort *base, int immElemOff);
uint  __builtin_IB_lsc_atomic_dec_local_ushort_from_uint(volatile __local ushort *base, int immElemOff);
uint  __builtin_IB_lsc_atomic_add_local_ushort_from_uint(volatile __local ushort *base, int immElemOff, uint val);
uint  __builtin_IB_lsc_atomic_sub_local_ushort_from_uint(volatile __local ushort *base, int immElemOff, uint val);
int   __builtin_IB_lsc_atomic_min_local_short_from_int  (volatile __local short  *base, int immElemOff, int val);
int   __builtin_IB_lsc_atomic_max_local_short_from_int  (volatile __local short  *base, int immElemOff, int val);
uint  __builtin_IB_lsc_atomic_min_local_ushort_from_uint(volatile __local ushort *base, int immElemOff, uint val);
uint  __builtin_IB_lsc_atomic_max_local_ushort_from_uint(volatile __local ushort *base, int immElemOff, uint val);
uint  __builtin_IB_lsc_atomic_and_local_ushort_from_uint(volatile __local ushort *base, int immElemOff, uint val);
uint  __builtin_IB_lsc_atomic_or_local_ushort_from_uint (volatile __local ushort *base, int immElemOff, uint val);
uint  __builtin_IB_lsc_atomic_xor_local_ushort_from_uint(volatile __local ushort *base, int immElemOff, uint val);
uint  __builtin_IB_lsc_atomic_load_local_ushort_from_uint(volatile __local ushort *base, int immElemOff);
uint  __builtin_IB_lsc_atomic_store_local_ushort_from_uint(volatile __local ushort *base, int immElemOff, uint val);
uint  __builtin_IB_lsc_atomic_cmpxchg_local_ushort_from_uint(volatile __local ushort *base, int immElemOff, uint cmp, uint val);


// LSC I32 atomics global
uint  __builtin_IB_lsc_atomic_inc_global_uint(volatile __global uint *base, int immElemOff,           enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_dec_global_uint(volatile __global uint *base, int immElemOff,           enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_add_global_uint(volatile __global uint *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_sub_global_uint(volatile __global uint *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
int   __builtin_IB_lsc_atomic_min_global_int (volatile __global  int *base, int immElemOff,  int val, enum LSC_STCC cacheOpt);
int   __builtin_IB_lsc_atomic_max_global_int (volatile __global  int *base, int immElemOff,  int val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_min_global_uint(volatile __global uint *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_max_global_uint(volatile __global uint *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_and_global_uint(volatile __global uint *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_or_global_uint (volatile __global uint *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_xor_global_uint(volatile __global uint *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_load_global_uint(volatile __global uint *base, int immElemOff,           enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_store_global_uint(volatile __global uint *base, int immElemOff, uint val, enum LSC_STCC cacheOpt);
uint  __builtin_IB_lsc_atomic_cmpxchg_global_uint(volatile __global uint *base, int immElemOff, uint cmp, uint val, enum LSC_STCC cacheOpt);


// LSC I32 atomics local
uint  __builtin_IB_lsc_atomic_inc_local_uint(volatile __local uint *base, int immElemOff);
uint  __builtin_IB_lsc_atomic_dec_local_uint(volatile __local uint *base, int immElemOff);
uint  __builtin_IB_lsc_atomic_add_local_uint(volatile __local uint *base, int immElemOff, uint val);
uint  __builtin_IB_lsc_atomic_sub_local_uint(volatile __local uint *base, int immElemOff, uint val);
int   __builtin_IB_lsc_atomic_min_local_int (volatile __local int  *base, int immElemOff,  int val);
int   __builtin_IB_lsc_atomic_max_local_int (volatile __local int  *base, int immElemOff,  int val);
uint  __builtin_IB_lsc_atomic_min_local_uint(volatile __local uint *base, int immElemOff, uint val);
uint  __builtin_IB_lsc_atomic_max_local_uint(volatile __local uint *base, int immElemOff, uint val);
uint  __builtin_IB_lsc_atomic_and_local_uint(volatile __local uint *base, int immElemOff, uint val);
uint  __builtin_IB_lsc_atomic_or_local_uint (volatile __local uint *base, int immElemOff, uint val);
uint  __builtin_IB_lsc_atomic_xor_local_uint(volatile __local uint *base, int immElemOff, uint val);
uint  __builtin_IB_lsc_atomic_load_local_uint(volatile __local uint *base, int immElemOff);
uint  __builtin_IB_lsc_atomic_store_local_uint(volatile __local uint *base, int immElemOff, uint val);
uint  __builtin_IB_lsc_atomic_cmpxchg_local_uint(volatile __local uint *base, int immElemOff, uint cmp, uint val);


// LSC I64 atomics global
ulong  __builtin_IB_lsc_atomic_inc_global_ulong(volatile __global ulong *base, int immElemOff,            enum LSC_STCC cacheOpt);
ulong  __builtin_IB_lsc_atomic_dec_global_ulong(volatile __global ulong *base, int immElemOff,            enum LSC_STCC cacheOpt);
ulong  __builtin_IB_lsc_atomic_add_global_ulong(volatile __global ulong *base, int immElemOff, ulong val, enum LSC_STCC cacheOpt);
ulong  __builtin_IB_lsc_atomic_sub_global_ulong(volatile __global ulong *base, int immElemOff, ulong val, enum LSC_STCC cacheOpt);
long   __builtin_IB_lsc_atomic_min_global_long (volatile __global  long *base, int immElemOff,  long val, enum LSC_STCC cacheOpt);
long   __builtin_IB_lsc_atomic_max_global_long (volatile __global  long *base, int immElemOff,  long val, enum LSC_STCC cacheOpt);
ulong  __builtin_IB_lsc_atomic_min_global_ulong(volatile __global ulong *base, int immElemOff, ulong val, enum LSC_STCC cacheOpt);
ulong  __builtin_IB_lsc_atomic_max_global_ulong(volatile __global ulong *base, int immElemOff, ulong val, enum LSC_STCC cacheOpt);
ulong  __builtin_IB_lsc_atomic_and_global_ulong(volatile __global ulong *base, int immElemOff, ulong val, enum LSC_STCC cacheOpt);
ulong  __builtin_IB_lsc_atomic_or_global_ulong (volatile __global ulong *base, int immElemOff, ulong val, enum LSC_STCC cacheOpt);
ulong  __builtin_IB_lsc_atomic_xor_global_ulong(volatile __global ulong *base, int immElemOff, ulong val, enum LSC_STCC cacheOpt);
ulong  __builtin_IB_lsc_atomic_load_global_ulong(volatile __global ulong *base, int immElemOff,            enum LSC_STCC cacheOpt);
ulong  __builtin_IB_lsc_atomic_store_global_ulong(volatile __global ulong *base, int immElemOff, ulong val, enum LSC_STCC cacheOpt);
ulong  __builtin_IB_lsc_atomic_cmpxchg_global_ulong(volatile __global ulong *base, int immElemOff, ulong cmp, ulong val, enum LSC_STCC cacheOpt);

// LSC I64 atomics local
ulong  __builtin_IB_lsc_atomic_cmpxchg_local_ulong(volatile __local ulong *base, int immElemOff, ulong cmp, ulong val);
#endif // cl_intel_pvc_lsc_validation

#ifdef cl_intel_subgroup_extended_block_read
// 2d block read
ushort2  __builtin_IB_subgroup_block_read_flat_u8_m1k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord);
ushort4  __builtin_IB_subgroup_block_read_flat_u8_m2k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord);
ushort8  __builtin_IB_subgroup_block_read_flat_u8_m4k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord);
ushort16 __builtin_IB_subgroup_block_read_flat_u8_m8k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord);
ushort2  __builtin_IB_subgroup_block_read_flat_u16_m1k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord);
ushort4  __builtin_IB_subgroup_block_read_flat_u16_m2k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord);
ushort8  __builtin_IB_subgroup_block_read_flat_u16_m4k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord);
ushort16 __builtin_IB_subgroup_block_read_flat_u16_m8k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord);
uint8 __builtin_IB_subgroup_block_read_flat_transform_u8_k32(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord);
uint8 __builtin_IB_subgroup_block_read_flat_transform_u16_k16(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord);
// equivalent to transpose_transform_u8_k32 and transpose_transform_u16_k16
uint8 __builtin_IB_subgroup_block_read_flat_transpose_u32_k8(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord);
ulong4 __builtin_IB_subgroup_block_read_flat_transpose_u64_k4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord);
#endif // cl_intel_subgroup_extended_block_read

#ifdef cl_intel_subgroup_extended_block_read_cacheopts
// 2d block read cacheopts
ushort2  __builtin_IB_subgroup_block_read_cacheopts_u8_m1k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
ushort4  __builtin_IB_subgroup_block_read_cacheopts_u8_m2k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
ushort8  __builtin_IB_subgroup_block_read_cacheopts_u8_m4k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
ushort16 __builtin_IB_subgroup_block_read_cacheopts_u8_m8k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
ushort2  __builtin_IB_subgroup_block_read_cacheopts_u16_m1k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
ushort4  __builtin_IB_subgroup_block_read_cacheopts_u16_m2k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
ushort8  __builtin_IB_subgroup_block_read_cacheopts_u16_m4k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
ushort16 __builtin_IB_subgroup_block_read_cacheopts_u16_m8k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
uint8 __builtin_IB_subgroup_block_read_cacheopts_transform_u8_k32(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
uint8 __builtin_IB_subgroup_block_read_cacheopts_transform_u16_k16(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
// 2d block write cacheopts
void __builtin_IB_subgroup_block_write_cacheopts_u8_m1k32v1(long base_address, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, ushort val, enum LSC_STCC cache_control);
void __builtin_IB_subgroup_block_write_cacheopts_u8_m2k32v1(long base_address, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, ushort2 val, enum LSC_STCC cache_control);
void __builtin_IB_subgroup_block_write_cacheopts_u8_m4k32v1(long base_address, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, ushort4 val, enum LSC_STCC cache_control);
void __builtin_IB_subgroup_block_write_cacheopts_u8_m8k32v1(long base_address, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, ushort8 val, enum LSC_STCC cache_control);
void __builtin_IB_subgroup_block_write_cacheopts_u16_m1k16v1(long base_address, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, ushort val, enum LSC_STCC cache_control);
void __builtin_IB_subgroup_block_write_cacheopts_u16_m2k16v1(long base_address, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, ushort2 val, enum LSC_STCC cache_control);
void __builtin_IB_subgroup_block_write_cacheopts_u16_m4k16v1(long base_address, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, ushort4 val, enum LSC_STCC cache_control);
void __builtin_IB_subgroup_block_write_cacheopts_u16_m8k16v1(long base_address, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, ushort8 val, enum LSC_STCC cache_control);
// equivalent to transpose_transform_u8_k32 and transpose_transform_u16_k16
uint8 __builtin_IB_subgroup_block_read_cacheopts_transpose_u32_k8(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
ulong4 __builtin_IB_subgroup_block_read_cacheopts_transpose_u64_k4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);

// 2d block read prefetch
void  __builtin_IB_subgroup_block_read_prefetch_u8_m1k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m2k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m4k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u8_m8k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m1k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m2k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m4k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u16_m8k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_transform_u8_k32(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_transform_u16_k16(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
// equivalent to transpose_transform_u8_k32 and transpose_transform_u16_k16
void __builtin_IB_subgroup_block_read_prefetch_transpose_u32_k8(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_transpose_u64_k4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
#endif // cl_intel_subgroup_extended_block_read_cacheopts

#endif // IGCBIF_INTRINSICS_LSC_CL
