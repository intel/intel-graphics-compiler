/*========================== begin_copyright_notice ============================

Copyright (C) 2020 Intel Corporation

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
    LSC_LDCC_L1_L2_L3_DEF      = 16,

    LSC_LDCC_L1UC_L2UC_L3UC    = 18, // Override to L1 uncached, L2 uncached, L3 uncached
    LSC_LDCC_L1UC_L2UC_L3C     = 19, // Override to L1 uncached, L2 uncached, L3 cached
    LSC_LDCC_L1UC_L2C_L3UC     = 20, // Override to L1 uncached, L2 cached, L3 uncached
    LSC_LDCC_L1UC_L2C_L3C      = 21, // Override to L1 uncached, L2 cached, L3 cached

    LSC_LDCC_L1C_L2UC_L3UC     = 22, // Override to L1 cached, L2 uncached, L3 uncached
    LSC_LDCC_L1C_L2UC_L3C      = 23, // Override to L1 cached, L2 uncached, L3 cached
    LSC_LDCC_L1C_L2C_L3UC      = 24, // Override to L1 cached, L2 cached, L3 uncached
    LSC_LDCC_L1C_L2C_L3C       = 25, // Override to L1 cached, L2 cached, L3 cached

    LSC_LDCC_L1S_L2UC_L3UC     = 26, // Override to L1 streaming load, L2 uncached, L3 uncached
    LSC_LDCC_L1S_L2UC_L3C      = 27, // Override to L1 streaming load, L2 uncached, L3 cached
    LSC_LDCC_L1S_L2C_L3UC      = 28, // Override to L1 streaming load, L2 cached, L3 uncached
    LSC_LDCC_L1S_L2C_L3C       = 29, // Override to L1 streaming load, L2 cached, L3 cached

    LSC_LDCC_L1IAR_L2IAR_L3IAR = 30, // Override to L1, L2, L3 invalidate-after-read
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
    LSC_STCC_L1_L2_L3_DEF   = 16,

    LSC_STCC_L1UC_L2UC_L3UC = 18, // Override to L1 uncached, L2 uncached, L3 uncached
    LSC_STCC_L1UC_L2UC_L3WB = 19, // Override to L1 uncached, L2 uncached, L3 written back
    LSC_STCC_L1UC_L2WB_L3UC = 20, // Override to L1 uncached, L2 written back, L3 uncached
    LSC_STCC_L1UC_L2WB_L3WB = 21, // Override to L1 uncached, L2 written back, L3 written back

    LSC_STCC_L1WT_L2UC_L3UC = 22, // Override to L1 written through, L2 uncached, L3 uncached
    LSC_STCC_L1WT_L2UC_L3WB = 23, // Override to L1 written through, L2 uncached, L3 written back
    LSC_STCC_L1WT_L2WB_L3UC = 24, // Override to L1 written through, L2 written back, L3 uncached
    LSC_STCC_L1WT_L2WB_L3WB = 25, // Override to L1 written through, L2 written back, L3 written back

    LSC_STCC_L1S_L2UC_L3UC  = 26, // Override to L1 streaming, L2 uncached, L3 uncached
    LSC_STCC_L1S_L2UC_L3WB  = 27, // Override to L1 streaming, L2 uncached, L3 written back
    LSC_STCC_L1S_L2WB_L3UC  = 28, // Override to L1 streaming, L2 written back, L3 uncached

    LSC_STCC_L1WB_L2UC_L3UC = 29, // Override to L1 written back, L2 uncached, L3 uncached
    LSC_STCC_L1WB_L2WB_L3UC = 30, // Override to L1 written back, L2 written back, L3 uncached
    LSC_STCC_L1WB_L2UC_L3WB = 31, // Override to L1 written back, L2 uncached, L3 written back
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
typedef enum {
    LSC_FS_THREAD_GROUP,
    LSC_FS_LOCAL,
    LSC_FS_TILE,
    LSC_FS_GPU,
    LSC_FS_GPUs,
    LSC_FS_SYSTEM_RELEASE,
    LSC_FS_SYSTEM_ACQUIRE
} LSC_FS;

// FT - Fence Type
typedef enum {
    LSC_FT_DEFAULT,
    LSC_FT_EVICT,
    LSC_FT_INVALIDATE,
    LSC_FT_DISCARD,
    LSC_FT_CLEAN,
    LSC_FT_L3
} LSC_FT;

void  __builtin_IB_lsc_fence_global_untyped(LSC_FS scope, LSC_FT flushType);             // Mem Port - UGM
void  __builtin_IB_lsc_fence_global_untyped_cross_tile(LSC_FS scope, LSC_FT flushType);  // Mem Port - UGML
void  __builtin_IB_lsc_fence_global_typed(LSC_FS scope, LSC_FT flushType);               // Mem Port - TGM
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

#ifdef cl_intel_subgroup_buffer_prefetch
// 1D Block prefetches
void __builtin_IB_lsc_simd_block_prefetch_uchar(const __global uchar*, enum LSC_LDCC cacheOpt);
void __builtin_IB_lsc_simd_block_prefetch_uchar2(const __global uchar*, enum LSC_LDCC cacheOpt);
void __builtin_IB_lsc_simd_block_prefetch_uchar4(const __global uchar*, enum LSC_LDCC cacheOpt);
void __builtin_IB_lsc_simd_block_prefetch_uchar8(const __global uchar*, enum LSC_LDCC cacheOpt);
void __builtin_IB_lsc_simd_block_prefetch_uchar16(const __global uchar*, enum LSC_LDCC cacheOpt);

void __builtin_IB_lsc_simd_block_prefetch_ushort(const __global ushort*, enum LSC_LDCC cacheOpt);
void __builtin_IB_lsc_simd_block_prefetch_ushort2(const __global ushort*, enum LSC_LDCC cacheOpt);
void __builtin_IB_lsc_simd_block_prefetch_ushort4(const __global ushort*, enum LSC_LDCC cacheOpt);
void __builtin_IB_lsc_simd_block_prefetch_ushort8(const __global ushort*, enum LSC_LDCC cacheOpt);
void __builtin_IB_lsc_simd_block_prefetch_ushort16(const __global ushort*, enum LSC_LDCC cacheOpt);

void __builtin_IB_lsc_simd_block_prefetch_uint(const __global uint*, enum LSC_LDCC cacheOpt);
void __builtin_IB_lsc_simd_block_prefetch_uint2(const __global uint*, enum LSC_LDCC cacheOpt);
void __builtin_IB_lsc_simd_block_prefetch_uint4(const __global uint*, enum LSC_LDCC cacheOpt);
void __builtin_IB_lsc_simd_block_prefetch_uint8(const __global uint*, enum LSC_LDCC cacheOpt);

void __builtin_IB_lsc_simd_block_prefetch_ulong(const __global ulong*, enum LSC_LDCC cacheOpt);
void __builtin_IB_lsc_simd_block_prefetch_ulong2(const __global ulong*, enum LSC_LDCC cacheOpt);
void __builtin_IB_lsc_simd_block_prefetch_ulong4(const __global ulong*, enum LSC_LDCC cacheOpt);
void __builtin_IB_lsc_simd_block_prefetch_ulong8(const __global ulong*, enum LSC_LDCC cacheOpt);
#endif // cl_intel_subgroup_buffer_prefetch

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
uint16 __builtin_IB_subgroup_block_read_flat_transpose_u32_k16(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord);
ulong8 __builtin_IB_subgroup_block_read_flat_transpose_u64_k8(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord);
#endif // cl_intel_subgroup_extended_block_read

//
// Naming convention
//
//   RETTY __builtin_IB_subgroup_block_read_cacheopts[_prefetch][_transpose][_transform]_<ETY>_m<M>k<K>v<V>[_as_RETTY] (...)
//
//   1. m<M>k<K>v<V>
//      It is the block shape specifier, where m and k denote block height and width, respectively. <M> and <K>
//      are height and width, respectively. v denotes the number of blocks and <V> is the number of blocks.
//      For example,
//        m32k16v4: 4 blocks and each block is of 32x16;
//           m16k8: 1 block of size 16x8.
//              k4: 1 block of size <sub group size>x4 (M = sub group size)
//   2. ETY
//      Element type of 2d block. It is one of u8 (8-bit), u16 (16-bit), u32 (32-bit), or u64 (64-bit).
//
//   3. _prefetch|_transpose|_transform
//      Optional. Not all combinations are valid
//   4. RETTY and optional [_as_RETTY]
//      RETTY is a type for each work-item (each lane). Its size is the total size divided by the number
//      of lanes. Its type can be any type whose size is the same as per-work-item size.
//      For example, m16k16 of u16 has 512 bytes (16x16x2). If divided evenly among 16 lanes, each lane
//      gets 32 bytes, which can be viewed as a return type of any of int4, float4, short8, ushort8, etc.
//      Which one to use is up to workload. For dpas, the return type is expected to match the dpas's
//      input type.
//
//      Normally, _as_RETTY is not needed. In case one workload requires more than one return types for
//      the same shape and element type, _as_RETTY is used to resolve prototype conflicts.
//        For example,
//          _transpose_u32_m8k16 with sub group size = 16, it can have int8 as return type for tf32 dpas's
//          matrix A operand.  It can also be ushort16 for int8 dpas's matrix A operand when int4 are always
//          stored in memory in the packed form. In this case, we can have
//               int8 _transpose_u32_m8k16 (...)
//            ushort8 _transpose_u32_m8k16_as_ushort8 (...)
//
//        Note that both builtins load the data with identical GRF layout, but are viewed differently for
//        each work-item.
//
//      With this optional [_as_RETTY], 2d block can be used conveniently for other non-dpas application
//      or simd32 kernels. IGC intrinsic is overloaded on return types, any return type should be okay.
//
// Selection of shape m<M>n<N>v<V>
//
//   Block 2d load instruction requires:
//     1. width needs to be padded to the next power of 2. For example, If k=3, it will use k=4 instead.
//        Padded elements are zero'ed.
//     2. the payload size for each block must be multiple of GRFs. If the block size isn't multiple of
//        GRF size, it is padded to the next GRF and the padded elements are zero'ed.
//
//     For example,  u32_m2k3v2,  each block is 2 x 4 (3 rounded up to 4) = 8 DW and takes half a GRF. It
//     has two blocks, the total payload size is 2 GRFs (32 DWs) with the upper 8 DWs are zero'ed for both
//     GRFs.
//
//   With this, the selection of m/k/v for builtins is to aovid any padding and make sure each block
//   is multiple of GRF size if there is more than one blocks.
//
// Note that the prototypes listed below are for reference and are not complete. The return types used
// are for kernels with sub group size 16. (If used for a simd32 kernel, the size of the return type should
// be halfed.)
//

#if defined(cl_intel_subgroup_extended_block_read_cacheopts) || defined(cl_intel_subgroup_2d_block_io)

// Macros for halving vector data types.
#define HALVE_TYPE(TYPE) HALVE_TYPE_##TYPE

#define HALVE_TYPE_uchar    uchar
#define HALVE_TYPE_uchar2   uchar
#define HALVE_TYPE_uchar4   uchar2
#define HALVE_TYPE_uchar8   uchar4
#define HALVE_TYPE_uchar16  uchar8
#define HALVE_TYPE_uchar32  uchar16
#define HALVE_TYPE_uchar64  uchar32
#define HALVE_TYPE_uchar128 uchar64

#define HALVE_TYPE_ushort   ushort
#define HALVE_TYPE_ushort2  ushort
#define HALVE_TYPE_ushort4  ushort2
#define HALVE_TYPE_ushort8  ushort4
#define HALVE_TYPE_ushort16 ushort8
#define HALVE_TYPE_ushort32 ushort16
#define HALVE_TYPE_ushort64 ushort32

#define HALVE_TYPE_uint   uint
#define HALVE_TYPE_uint2  uint
#define HALVE_TYPE_uint4  uint2
#define HALVE_TYPE_uint8  uint4
#define HALVE_TYPE_uint16 uint8
#define HALVE_TYPE_uint32 uint16
#define HALVE_TYPE_uint64 uint32

#define HALVE_TYPE_ulong   ulong
#define HALVE_TYPE_ulong2  ulong
#define HALVE_TYPE_ulong4  ulong2
#define HALVE_TYPE_ulong8  ulong4
#define HALVE_TYPE_ulong16 ulong8
#define HALVE_TYPE_ulong32 ulong16
#define HALVE_TYPE_ulong64 ulong32

// 2D block I/O operations are available on platforms with minimum subgroup size = 16.
// For subgroup size = 32, to load/store the same 2D block dimensions, vector data type
// in work item must be halved. If data type is not vector, type is unchanged, and half
// of work items in subgroup does not participate in 2D block access.
// Since prefetch has no return type, there is only a single function for both subgroup sizes.
#define DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(FUNC_NAME, TYPE)                                                                                              \
TYPE              FUNC_NAME       (long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt); \
HALVE_TYPE(TYPE)  FUNC_NAME##_sg32(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);

#define DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(FUNC_NAME, TYPE)                                                                                                             \
void FUNC_NAME       (long base_address, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, TYPE val,             enum LSC_STCC cache_control); \
void FUNC_NAME##_sg32(long base_address, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, HALVE_TYPE(TYPE) val, enum LSC_STCC cache_control); \

// 2d block read cacheopts
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m1k32v2, ushort2);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m2k32v2, ushort4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m4k32v2, ushort8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m8k32v2, ushort16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m1k16v2, ushort2);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m2k16v2, ushort4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m4k16v2, ushort8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m8k16v2, ushort16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transform_u8_k32, uint8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transform_u16_k16, uint8);

// equivalent to transpose_transform_u8_k32 and transpose_transform_u16_k16
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_k8, uint8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m16k8, uint8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m16k1, uint);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m16k2, uint2);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m16k4, uint4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m32k4, uint8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m32k8, uint16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u64_k4, ulong4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u64_m8k1, ulong);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u64_m8k2, ulong);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u64_m8k4, ulong4);

// transpose with width=8, for A matrix of tf32
//   One simd16 has data from two consecutive rows to match tf32 dpas
//
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m8k2, uint);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m8k4, uint2);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m8k8, uint4);


DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_k16, uint16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u32_m8k16, uint8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u64_k8, ulong8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u64_m8k8, ulong8);

//
// emulated transpose: A matrix of d8
//
// u8, m=32 and k=4,8,16
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u8_m32k4, ushort4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u8_m32k8, ushort8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u8_m32k16, ushort16);

// emulated transpose: A matrix of d16
//
// u16, m=16 and k=4,8,16
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u16_m16k4, ushort4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u16_m16k8, ushort8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transpose_u16_m16k16, ushort16);


// 2d block read prefetch
void __builtin_IB_subgroup_block_read_prefetch_u16_m1k8v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u16_m2k8v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u16_m4k8v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u16_m16k8v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);

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
void __builtin_IB_subgroup_block_read_prefetch_u64_m8k8v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u64_m16k8v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u64_m32k8v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_transpose_u32_k16(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_transpose_u64_k8(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_transpose_u64_m8k8(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
// 2d block wide prefetch
void  __builtin_IB_subgroup_block_read_prefetch_u8_m1k64v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m1k128v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m1k256v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m2k64v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m2k128v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m2k256v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m4k64v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m4k128v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m4k256v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m8k64v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m8k128v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m8k256v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m16k64v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m16k128v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m16k256v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m32k64v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m32k128v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m32k256v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);

void  __builtin_IB_subgroup_block_read_prefetch_u16_m1k32v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m1k64v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m1k128v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m2k32v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m2k64v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m2k128v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m4k32v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m4k64v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m4k128v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m8k32v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m8k64v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m8k128v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m16k32v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m16k64v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m16k128v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m32k32v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m32k64v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m32k128v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);

void  __builtin_IB_subgroup_block_read_prefetch_u32_m1k16v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m1k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m1k64v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m2k16v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m2k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m2k64v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m4k16v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m4k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m4k64v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m8k16v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m8k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m8k64v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m16k16v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m16k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m16k64v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m32k16v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m32k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u32_m32k64v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);

void  __builtin_IB_subgroup_block_read_prefetch_u64_m1k8v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m1k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m1k32v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m2k8v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m2k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m2k32v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m4k8v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m4k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m4k32v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m8k8v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m8k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m8k32v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m16k8v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m16k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m16k32v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m32k8v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m32k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u64_m32k32v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
#endif // cl_intel_subgroup_extended_block_read_cacheopts || cl_intel_subgroup_2d_block_io

#if defined(cl_intel_subgroup_extended_block_write_cacheopts) || defined(cl_intel_subgroup_2d_block_io)
// 2d block write cacheopts
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u8_m1k32v1, ushort);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u8_m2k32v1, ushort2);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u8_m4k32v1, ushort4);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u8_m8k32v1, ushort8);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u16_m1k16v1, ushort);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u16_m2k16v1, ushort2);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u16_m4k16v1, ushort4);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u16_m8k16v1, ushort8);
#endif // cl_intel_subgroup_extended_block_write_cacheopts || cl_intel_subgroup_2d_block_io

#ifdef cl_intel_subgroup_2d_block_io

typedef uchar uchar32 __attribute__((ext_vector_type(32)));
typedef uchar uchar64 __attribute__((ext_vector_type(64)));
typedef uchar uchar128 __attribute__((ext_vector_type(128)));

typedef ushort ushort32 __attribute__((ext_vector_type(32)));
typedef ushort ushort64 __attribute__((ext_vector_type(64)));

typedef uint uint32 __attribute__((ext_vector_type(32)));
typedef uint uint64 __attribute__((ext_vector_type(64)));

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m8k8v1, uchar4);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m8k8v2, uchar8);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m8k8v4, uchar16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m16k8v4, uchar32);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m32k8v4, uchar64);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m1k16v1, uchar);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m2k16v1, uchar2);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m4k16v1, uchar4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m8k16v1, uchar8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m16k16v1, uchar16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m32k16v1, uchar32);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m1k16v2, uchar2);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m2k16v2, uchar4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m4k16v2, uchar8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m8k16v2, uchar16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m16k16v2, uchar32);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m32k16v2, uchar64);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m1k16v4, uchar4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m2k16v4, uchar8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m4k16v4, uchar16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m8k16v4, uchar32);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m16k16v4, uchar64);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m32k16v4, uchar128);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m1k32v1, ushort);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m2k32v1, ushort2);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m4k32v1, ushort4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m8k32v1, ushort8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m16k32v1, ushort16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m32k32v1, ushort32);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m1k64v1, uint);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m2k64v1, uint2);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m4k64v1, uint4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m8k64v1, uint8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m16k64v1, uint16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m32k64v1, uint32);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m16k32v2, ushort32);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u8_m32k32v2, ushort64);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m2k8v1, ushort);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m4k8v1, ushort2);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m8k8v1, ushort4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m16k8v1, ushort8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m32k8v1, ushort16);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m2k8v2, ushort2);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m4k8v2, ushort4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m8k8v2, ushort8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m16k8v2, ushort16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m32k8v2, ushort32);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m2k8v4, ushort4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m4k8v4, ushort8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m8k8v4, ushort16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m16k8v4, ushort32);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m32k8v4, ushort64);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m1k16v1, ushort);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m2k16v1, ushort2);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m4k16v1, ushort4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m8k16v1, ushort8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m16k16v1, ushort16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m32k16v1, ushort32);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m16k16v2, ushort32);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m32k16v2, ushort64);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m1k32v1, uint);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m2k32v1, uint2);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m4k32v1, uint4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m8k32v1, uint8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m16k32v1, uint16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u16_m32k32v1, uint32);

void  __builtin_IB_subgroup_block_read_prefetch_u8_m16k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m32k32v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);

void  __builtin_IB_subgroup_block_read_prefetch_u8_m8k16v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m16k16v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m16k8v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);

void  __builtin_IB_subgroup_block_read_prefetch_u16_m16k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u16_m32k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);

void  __builtin_IB_subgroup_block_read_prefetch_u8_m1k32v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m2k32v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m4k32v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m8k32v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m16k32v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void  __builtin_IB_subgroup_block_read_prefetch_u8_m32k32v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);

void __builtin_IB_subgroup_block_read_prefetch_u8_m32k16v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u16_m1k16v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u16_m2k16v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u16_m4k16v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u16_m8k16v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u16_m16k16v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u16_m32k16v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m16k8v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m1k8v1, uint);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m2k8v1, uint);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m4k8v1, uint2);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m8k8v1, uint4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m16k8v1, uint8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m32k8v1, uint16);

void __builtin_IB_subgroup_block_read_prefetch_u32_m1k8v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m2k8v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m4k8v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m8k8v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m16k8v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m32k8v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m1k16v1, uint);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m2k16v1, uint2);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m4k16v1, uint4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m8k16v1, uint8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m16k16v1, uint16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m32k16v1, uint32);

void __builtin_IB_subgroup_block_read_prefetch_u32_m1k16v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m2k16v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m4k16v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m8k16v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m16k16v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m32k16v1(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m1k8v2, uint2);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m2k8v2, uint2);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m4k8v2, uint4);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m8k8v2, uint8);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m16k8v2, uint16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_u32_m32k8v2, uint32);

void __builtin_IB_subgroup_block_read_prefetch_u32_m1k8v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m2k8v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m4k8v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m8k8v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m16k8v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u32_m32k8v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);

DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u8_m8k8v1, uchar4);

DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u8_m1k16v1, uchar);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u8_m2k16v1, uchar2);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u8_m4k16v1, uchar4);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u8_m8k16v1, uchar8);

DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u16_m8k8v1, ushort4);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u16_m1k32v1, uint);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u16_m8k32v1, uint8);

DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u32_m8k4v1, uint2);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u32_m8k8v1, uint4);

DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u32_m1k16v1, uint);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u32_m2k16v1, uint2);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u32_m4k16v1, uint4);
DEFN_SUBGROUP_BLOCK_WRITE_CACHEOPTS(__builtin_IB_subgroup_block_write_cacheopts_u32_m8k16v1, uint8);

DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transform_u16_k32n16v1, uint16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transform_u16_k16n16v2, uint16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transform_u16_k32n16v2, uint32);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transform_u8_k32n16v2, uint16);
DEFN_SUBGROUP_BLOCK_READ_CACHEOPTS(__builtin_IB_subgroup_block_read_cacheopts_transform_u8_k32n16v4, uint32);

void __builtin_IB_subgroup_block_read_prefetch_u8_m32k16v2(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);
void __builtin_IB_subgroup_block_read_prefetch_u8_m32k16v4(long baseoffset, int width_minus_one, int height_minus_one, int pitch_minus_one, int2 coord, enum LSC_LDCC cacheOpt);

#endif // cl_intel_subgroup_2d_block_io

// experimental
#ifdef cl_intel_subgroup_extended_block_read
//
// 2d block read/write
//
// Requirement: block width/height/numBlocks must be compile time constant (they should be constant literals).
//              In addition, they must satisfy the following:
//   Read:
//                       x              y                     <--  coordinate
//                       k              m             v       <--  matrix dimension
//       Data Size  blockWidth    blockHeight  numBlocks      Restriction (blockWidth x numBlocks)
//          d8        4 - 64         1 - 32         1,2,4              <= 64
//         d16        2 - 32         1 - 32         1,2,4              <= 32
//         d32        1 - 16         1 - 32         1,2                <= 16
//         d64        1 - 8          1 - 32         1                  <= 8
//  write:
//       Data Size  blockWidth   blockHeight  numBlocks
//          d8        4 - 64         1 - 8          1
//         d16        2 - 32         1 - 8          1
//         d32        1 - 16         1 - 8          1
//         d64        1 - 8          1 - 8          1
//
//  Note:
//     1. data size isn't part of payload. It is included in each read/write builtins that uses this payload.
//        Expect payload's block dimension matches one specified in read/write builtin.
//     2. Return type is of integer vector. For dpas, 'A' matrix's element type shall be the same as one of
//        dpas 'A' operand (mostly short for subgroup size 16), so is B's type, which is mostly of int.
//        The key is to match dpas's operand type.
//     3. Not all supported builtins are listed here.
//     4. Argument order:  width goes before height, this is to be consistent with coordinate order (x for
//        width, y for height).
//
//  "Address payload" is a opaque, uniform variable, and is a placeholder for the real address payload.
//  Since updating builtins do not create a new address payload, they need a pointer type as address
//  payload's type to have a correct semantics in llvm IR. Here, "int*" is used as address payload's type.
//
// These two builtins creates a new address payload
int* __builtin_IB_subgroup_createBlock2DAddressPayload(long base, int width_minus_one, int height_minus_one, int pitch_minus_one,
        int blockX, int blockY, int blockWidth, int blockHeight, int numBlocks);
int* __builtin_IB_subgroup_copyBlock2DAddressPayload(int* AP);

// The following updates the existing address payload
void __builtin_IB_subgroup_addBlock2DAddressPayloadBlockX(int* addrPayload, int blockX);
void __builtin_IB_subgroup_addBlock2DAddressPayloadBlockY(int* addrPayload, int blockY);
void __builtin_IB_subgroup_setBlock2DAddressPayloadBlockX(int* addrPayload, int blockX);
void __builtin_IB_subgroup_setBlock2DAddressPayloadBlockY(int* addrPayload, int blockY);
void __builtin_IB_subgroup_setBlock2DAddressPayloadBase(int* addrPayload, long base);
void __builtin_IB_subgroup_setBlock2DAddressPayloadWidth(int* addrPayload, int width_minus_one);
void __builtin_IB_subgroup_setBlock2DAddressPayloadHeigth(int* addrPayload, int height_minus_one);
void __builtin_IB_subgroup_setBlock2DAddressPayloadPitch(int* addrPayload, int pitch_minus_one);

//
// 2d block read, expect addrPayload's block dimension to be the same as one specified
//
// A matrix uses short as its element type except double dpas, which uses int
short4  __builtin_IB_subgroup_block_read_ap_u8_m4k32v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short8  __builtin_IB_subgroup_block_read_ap_u8_m8k32v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short16 __builtin_IB_subgroup_block_read_ap_u8_m16k32v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short4  __builtin_IB_subgroup_block_read_ap_u8_m2k32v2(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short8  __builtin_IB_subgroup_block_read_ap_u8_m4k32v2(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short16 __builtin_IB_subgroup_block_read_ap_u8_m8k32v2(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short4  __builtin_IB_subgroup_block_read_ap_u16_m4k16v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short8  __builtin_IB_subgroup_block_read_ap_u16_m8k16v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short16 __builtin_IB_subgroup_block_read_ap_u16_m16k16v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short4  __builtin_IB_subgroup_block_read_ap_u16_m2k16v2(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short8  __builtin_IB_subgroup_block_read_ap_u16_m4k16v2(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short16 __builtin_IB_subgroup_block_read_ap_u16_m8k16v2(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short4  __builtin_IB_subgroup_block_read_ap_u32_m4k8v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short8  __builtin_IB_subgroup_block_read_ap_u32_m8k8v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short16 __builtin_IB_subgroup_block_read_ap_u32_m16k8v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short8  __builtin_IB_subgroup_block_read_ap_u32_m4k8v2(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short16 __builtin_IB_subgroup_block_read_ap_u32_m8k8v2(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int4    __builtin_IB_subgroup_block_read_ap_u64_m4k8v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
void    __builtin_IB_subgroup_block_write_ap_u8_m4k32v1(int* addrPayload, const int immX, const int immY, short4 val, enum LSC_LDCC cacheOpt);
void    __builtin_IB_subgroup_block_write_ap_u8_m8k32v1(int* addrPayload, const int immX, const int immY, short8 val, enum LSC_LDCC cacheOpt);
void    __builtin_IB_subgroup_block_write_ap_u8_m16k32v1(int* addrPayload, const int immX, const int immY, short16 val, enum LSC_LDCC cacheOpt);
void    __builtin_IB_subgroup_block_write_ap_u16_m4k16v1(int* addrPayload, const int immX, const int immY, short4 val, enum LSC_LDCC cacheOpt);
void    __builtin_IB_subgroup_block_write_ap_u16_m8k16v1(int* addrPayload, const int immX, const int immY, short8 val, enum LSC_LDCC cacheOpt);
void    __builtin_IB_subgroup_block_write_ap_u16_m16k16v1(int* addrPayload, const int immX, const int immY, short16 val, enum LSC_LDCC cacheOpt);
// prefetch
void    __builtin_IB_subgroup_block_read_ap_prefetch_u8_m4k32v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
void    __builtin_IB_subgroup_block_read_ap_prefetch_u8_m8k32v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
void    __builtin_IB_subgroup_block_read_ap_prefetch_u8_m16k32v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
//
// B matrix uses int as its element type except double dpas, which uses long
int4    __builtin_IB_subgroup_block_read_ap_u8_m4k64v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int8    __builtin_IB_subgroup_block_read_ap_u8_m8k64v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int16   __builtin_IB_subgroup_block_read_ap_u8_m16k64v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int4    __builtin_IB_subgroup_block_read_ap_u32_m4k16v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int8    __builtin_IB_subgroup_block_read_ap_u32_m8k16v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int16   __builtin_IB_subgroup_block_read_ap_u32_m16k16v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int8    __builtin_IB_subgroup_block_read_ap_u32_m4k16v2(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int16   __builtin_IB_subgroup_block_read_ap_u32_m8k16v2(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
long8   __builtin_IB_subgroup_block_read_ap_u64_m8k16v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
void    __builtin_IB_subgroup_block_write_ap_u8_m4k64v1(int* addrPayload, const int immX, const int immY, int4 val, enum LSC_LDCC cacheOpt);
void    __builtin_IB_subgroup_block_write_ap_u8_m8k64v1(int* addrPayload, const int immX, const int immY, int8 val, enum LSC_LDCC cacheOpt);
void    __builtin_IB_subgroup_block_write_ap_u8_m16k64v1(int* addrPayload, const int immX, const int immY, int16 val, enum LSC_LDCC cacheOpt);
void    __builtin_IB_subgroup_block_write_ap_u16_m8k32v1(int* addrPayload, const int immX, const int immY, int8 val, enum LSC_LDCC cacheOpt);
void    __builtin_IB_subgroup_block_write_ap_u32_m8k16v1(int* addrPayload, const int immX, const int immY, int8 val, enum LSC_LDCC cacheOpt);
// prefetch
void    __builtin_IB_subgroup_block_read_ap_prefetch_u8_m4k64v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
void    __builtin_IB_subgroup_block_read_ap_prefetch_u8_m8k64v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
void   __builtin_IB_subgroup_block_read_ap_prefetch_u8_m16k64v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);

//
// 2d block read with transform
//
// A matrix (short as element type)
short4  __builtin_IB_subgroup_block_read_ap_transform_u8_m16k8v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short8  __builtin_IB_subgroup_block_read_ap_transform_u8_m32k8v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short4  __builtin_IB_subgroup_block_read_ap_transform_u16_m8k8v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short8  __builtin_IB_subgroup_block_read_ap_transform_u16_m16k8v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
// B matrix (int as element type)
int4  __builtin_IB_subgroup_block_read_ap_transform_u8_m16k16v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int8  __builtin_IB_subgroup_block_read_ap_transform_u8_m32k16v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int16 __builtin_IB_subgroup_block_read_ap_transform_u8_m32k16v2(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int4  __builtin_IB_subgroup_block_read_ap_transform_u16_m8k16v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int8  __builtin_IB_subgroup_block_read_ap_transform_u16_m16k16v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int16 __builtin_IB_subgroup_block_read_ap_transform_u16_m32k16v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
//
// 2d block read with transpose
//
// A matrix
short4   __builtin_IB_subgroup_block_read_ap_transpose_u32_m8k4v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short8   __builtin_IB_subgroup_block_read_ap_transpose_u32_m8k8v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
short16  __builtin_IB_subgroup_block_read_ap_transpose_u32_m8k16v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int4     __builtin_IB_subgroup_block_read_ap_transpose_u64_m8k4v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
// B matrix
int4     __builtin_IB_subgroup_block_read_ap_transpose_u32_m16k4v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int8     __builtin_IB_subgroup_block_read_ap_transpose_u32_m16k8v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
int16    __builtin_IB_subgroup_block_read_ap_transpose_u32_m16k16v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);
long8    __builtin_IB_subgroup_block_read_ap_transpose_u64_m16k8v1(int* addrPayload, const int immX, const int immY, enum LSC_LDCC cacheOpt);

int8  __builtin_IB_subgroup_block_read_transpose_u64_m8k8(int* addrPayload, const int immX, const int immY);

#endif // cl_intel_subgroup_extended_block_read

#endif // IGCBIF_INTRINSICS_LSC_CL
