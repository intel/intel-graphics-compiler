
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

//*****************************************************************************/
// Generic Headers
//*****************************************************************************/

#include "IBiF_Header.cl"
#include "../../Headers/spirv.h"


//*****************************************************************************/
// Enable msaa_sharing functionality
//*****************************************************************************/
#ifdef cl_khr_gl_msaa_sharing
#pragma OPENCL EXTENSION cl_khr_gl_msaa_sharing : enable
#endif //cl_khr_gl_msaa_sharing

//*****************************************************************************/
// Vector load and store functions
//*****************************************************************************/
#include "IBiF_VLoadStore.cl"

//*****************************************************************************/
// Work-Item functions
//*****************************************************************************/
#include "IBiF_WIFuncs.cl"

//*****************************************************************************/
// Device Enqueue functions
//*****************************************************************************/
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#include "IBiF_Device_Enqueue_size_t.cl"
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
//*****************************************************************************/
// Work-group functions
//*****************************************************************************/

#define WG_BROADCAST_1D_DEFN(type,abbr)                                         \
INLINE type OVERLOADABLE work_group_broadcast(type a, size_t local_id) {        \
    if (sizeof(local_id) == 32) {                                               \
        uint3 LocalID = (uint3)(local_id,0,0);                                      \
        return __builtin_spirv_OpGroupBroadcast_i32_##abbr##_v3i32(Workgroup,a,LocalID);   \
    }                                                                           \
    else{                                                                       \
        ulong3 LocalID = (ulong3)(local_id,0,0);                                    \
        return __builtin_spirv_OpGroupBroadcast_i32_##abbr##_v3i64(Workgroup,a,LocalID);   \
    }                                                                           \
}

#define WG_BROADCAST_2D_DEFN(type,abbr)                                         \
INLINE type OVERLOADABLE work_group_broadcast(type a, size_t x, size_t y) {     \
    if (sizeof(x) == 32) {                                                      \
        uint3 LocalID = (uint3)(x,y,0);                                             \
        return __builtin_spirv_OpGroupBroadcast_i32_##abbr##_v3i32(Workgroup,a,LocalID);   \
    }                                                                           \
    else{                                                                       \
        ulong3 LocalID = (ulong3)(x,y,0);                                           \
        return __builtin_spirv_OpGroupBroadcast_i32_##abbr##_v3i64(Workgroup,a,LocalID);   \
    }                                                                           \
}

#define WG_BROADCAST_3D_DEFN(type,abbr)                                         \
INLINE type OVERLOADABLE work_group_broadcast(type a, size_t x, size_t y, size_t z) { \
    if (sizeof(x) == 32) {                                                      \
        uint3 LocalID = (uint3)(x,y,z);                                             \
        return __builtin_spirv_OpGroupBroadcast_i32_##abbr##_v3i32(Workgroup,a,LocalID);   \
    }                                                                           \
    else{                                                                       \
        ulong3 LocalID = (ulong3)(x,y,z);                                           \
        return __builtin_spirv_OpGroupBroadcast_i32_##abbr##_v3i64(Workgroup,a,LocalID);   \
    }                                                                           \
}

#define WG_BROADCAST_ALL_DEFN(type,abbr) \
WG_BROADCAST_1D_DEFN(type,abbr) \
WG_BROADCAST_2D_DEFN(type,abbr) \
WG_BROADCAST_3D_DEFN(type,abbr)

WG_BROADCAST_ALL_DEFN(int,i32)
WG_BROADCAST_ALL_DEFN(uint,i32)
WG_BROADCAST_ALL_DEFN(long,i64)
WG_BROADCAST_ALL_DEFN(ulong,i64)
WG_BROADCAST_ALL_DEFN(float,f32)
#ifdef cl_khr_fp16
WG_BROADCAST_ALL_DEFN(half,f16)
#endif
#if defined(cl_khr_fp64)
WG_BROADCAST_ALL_DEFN(double,f64)
#endif

//*****************************************************************************/
// Asynchronous copy functions
//*****************************************************************************/

#define ASYNC_COPY(dst, src, num_elements, evt, mangle_string)                  \
{                                                                               \
  if (sizeof(num_elements) == 32)                                               \
    return __builtin_spirv_OpGroupAsyncCopy_i32##mangle_string##_i32_i32_i64(Workgroup,dst,src,num_elements,0,evt);\
  else                                                                          \
    return __builtin_spirv_OpGroupAsyncCopy_i32##mangle_string##_i64_i64_i64(Workgroup,dst,src,num_elements,0,evt);\
}

#define ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, mangle_string)    \
{                                                                               \
  if (sizeof(num_elements) == 32)                                               \
    return __builtin_spirv_OpGroupAsyncCopy_i32##mangle_string##_i32_i32_i64(Workgroup,dst,src,num_elements,src_stride,evt);\
  else                                                                          \
    return __builtin_spirv_OpGroupAsyncCopy_i32##mangle_string##_i64_i64_i64(Workgroup,dst,src,num_elements,src_stride,evt);\
}

// ************************  Global to local.*********************************

// char
INLINE event_t OVERLOADABLE async_work_group_copy(__local char *dst, const __global char *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local uchar*)dst, (const global uchar*)src, num_elements, evt, _p3i8_p1i8)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local char *dst, const __global char *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local uchar*)dst, (const global uchar*)src, num_elements, src_stride, evt, _p3i8_p1i8)
}

// char2
INLINE event_t OVERLOADABLE async_work_group_copy(__local char2 *dst, const __global char2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local uchar2*)dst, (const global uchar2*)src, num_elements, evt, _p3v2i8_p1v2i8)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local char2 *dst, const __global char2 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local uchar2*)dst, (const global uchar2*)src, num_elements, src_stride, evt, _p3v2i8_p1v2i8)
}

// char3
INLINE event_t OVERLOADABLE async_work_group_copy(__local char3 *dst, const __global char3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local uchar3*)dst, (const global uchar3*)src, num_elements, evt, _p3v3i8_p1v3i8)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local char3 *dst, const __global char3 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local uchar3*)dst, (const global uchar3*)src, num_elements, src_stride, evt, _p3v3i8_p1v3i8)
}

// char4
INLINE event_t OVERLOADABLE async_work_group_copy(__local char4 *dst, const __global char4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local uchar4*)dst, (const global uchar4*)src, num_elements, evt, _p3v4i8_p1v4i8)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local char4 *dst, const __global char4 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local uchar4*)dst, (const global uchar4*)src, num_elements, src_stride, evt, _p3v4i8_p1v4i8)
}

// char8
INLINE event_t OVERLOADABLE async_work_group_copy(__local char8 *dst, const __global char8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local uchar8*)dst, (const global uchar8*)src, num_elements, evt, _p3v8i8_p1v8i8)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local char8 *dst, const __global char8 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local uchar8*)dst, (const global uchar8*)src, num_elements, src_stride, evt, _p3v8i8_p1v8i8)
}

// char16
INLINE event_t OVERLOADABLE async_work_group_copy(__local char16 *dst, const __global char16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local uchar16*)dst, (const global uchar16*)src, num_elements, evt, _p3v16i8_p1v16i8)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local char16 *dst, const __global char16 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local uchar16*)dst, (const global uchar16*)src, num_elements, src_stride, evt, _p3v16i8_p1v16i8)
}

// short
INLINE event_t OVERLOADABLE async_work_group_copy(__local short *dst, const __global short *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local ushort*)dst, (const global ushort*)src, num_elements, evt, _p3i16_p1i16)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local short *dst, const __global short *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local ushort*)dst, (const global ushort*)src, num_elements, src_stride, evt, _p3i16_p1i16)
}

// short2
INLINE event_t OVERLOADABLE async_work_group_copy(__local short2 *dst, const __global short2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local ushort2*)dst, (const global ushort2*)src, num_elements, evt, _p3v2i16_p1v2i16)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local short2 *dst, const __global short2 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local ushort2*)dst, (const global ushort2*)src, num_elements, src_stride, evt, _p3v2i16_p1v2i16)
}

// short3
INLINE event_t OVERLOADABLE async_work_group_copy(__local short3 *dst, const __global short3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local ushort3*)dst, (const global ushort3*)src, num_elements, evt, _p3v3i16_p1v3i16)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local short3 *dst, const __global short3 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local ushort3*)dst, (const global ushort3*)src, num_elements, src_stride, evt, _p3v3i16_p1v3i16)
}

// short4
INLINE event_t OVERLOADABLE async_work_group_copy(__local short4 *dst, const __global short4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local ushort4*)dst, (const global ushort4*)src, num_elements, evt, _p3v4i16_p1v4i16)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local short4 *dst, const __global short4 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local ushort4*)dst, (const global ushort4*)src, num_elements, src_stride, evt, _p3v4i16_p1v4i16)
}

// short8
INLINE event_t OVERLOADABLE async_work_group_copy(__local short8 *dst, const __global short8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local ushort8*)dst, (const global ushort8*)src, num_elements, evt, _p3v8i16_p1v8i16)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local short8 *dst, const __global short8 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local ushort8*)dst, (const global ushort8*)src, num_elements, src_stride, evt, _p3v8i16_p1v8i16)
}

// short16
INLINE event_t OVERLOADABLE async_work_group_copy(__local short16 *dst, const __global short16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local ushort16*)dst, (const global ushort16*)src, num_elements, evt, _p3v16i16_p1v16i16)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local short16 *dst, const __global short16 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local ushort16*)dst, (const global ushort16*)src, num_elements, src_stride, evt, _p3v16i16_p1v16i16)
}

// int
INLINE event_t OVERLOADABLE async_work_group_copy(__local int *dst, const __global int *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local uint*)dst, (const global uint*)src, num_elements, evt, _p3i32_p1i32)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local int *dst, const __global int *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local uint*)dst, (const global uint*)src, num_elements, src_stride, evt, _p3i32_p1i32)
}

// int2
INLINE event_t OVERLOADABLE async_work_group_copy(__local int2 *dst, const __global int2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local uint2*)dst, (const global uint2*)src, num_elements, evt, _p3v2i32_p1v2i32)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local int2 *dst, const __global int2 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local uint2*)dst, (const global uint2*)src, num_elements, src_stride, evt, _p3v2i32_p1v2i32)
}

// int3
INLINE event_t OVERLOADABLE async_work_group_copy(__local int3 *dst, const __global int3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local uint3*)dst, (const global uint3*)src, num_elements, evt, _p3v3i32_p1v3i32)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local int3 *dst, const __global int3 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local uint3*)dst, (const global uint3*)src, num_elements, src_stride, evt, _p3v3i32_p1v3i32)
}

// int4
INLINE event_t OVERLOADABLE async_work_group_copy(__local int4 *dst, const __global int4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local uint4*)dst, (const global uint4*)src, num_elements, evt, _p3v4i32_p1v4i32)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local int4 *dst, const __global int4 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local uint4*)dst, (const global uint4*)src, num_elements, src_stride, evt, _p3v4i32_p1v4i32)
}

// int8
INLINE event_t OVERLOADABLE async_work_group_copy(__local int8 *dst, const __global int8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local uint8*)dst, (const global uint8*)src, num_elements, evt, _p3v8i32_p1v8i32)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local int8 *dst, const __global int8 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local uint8*)dst, (const global uint8*)src, num_elements, src_stride, evt, _p3v8i32_p1v8i32)
}

// int16
INLINE event_t OVERLOADABLE async_work_group_copy(__local int16 *dst, const __global int16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local uint16*)dst, (const global uint16*)src, num_elements, evt, _p3v16i32_p1v16i32)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local int16 *dst, const __global int16 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local uint16*)dst, (const global uint16*)src, num_elements, src_stride, evt, _p3v16i32_p1v16i32)
}

// long
INLINE event_t OVERLOADABLE async_work_group_copy(__local long *dst, const __global long *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local ulong*)dst, (const global ulong*)src, num_elements, evt, _p3i64_p1i64)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local long *dst, const __global long *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local ulong*)dst, (const global ulong*)src, num_elements, src_stride, evt, _p3i64_p1i64)
}

// long2
INLINE event_t OVERLOADABLE async_work_group_copy(__local long2 *dst, const __global long2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local ulong2*)dst, (const global ulong2*)src, num_elements, evt, _p3v2i64_p1v2i64)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local long2 *dst, const __global long2 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local ulong2*)dst, (const global ulong2*)src, num_elements, src_stride, evt, _p3v2i64_p1v2i64)
}

// long3
INLINE event_t OVERLOADABLE async_work_group_copy(__local long3 *dst, const __global long3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local ulong3*)dst, (const global ulong3*)src, num_elements, evt, _p3v3i64_p1v3i64)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local long3 *dst, const __global long3 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local ulong3*)dst, (const global ulong3*)src, num_elements, src_stride, evt, _p3v3i64_p1v3i64)
}

// long4
INLINE event_t OVERLOADABLE async_work_group_copy(__local long4 *dst, const __global long4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local ulong4*)dst, (const global ulong4*)src, num_elements, evt, _p3v4i64_p1v4i64)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local long4 *dst, const __global long4 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local ulong4*)dst, (const global ulong4*)src, num_elements, src_stride, evt, _p3v4i64_p1v4i64)
}

// long8
INLINE event_t OVERLOADABLE async_work_group_copy(__local long8 *dst, const __global long8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local ulong8*)dst, (const global ulong8*)src, num_elements, evt, _p3v8i64_p1v8i64)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local long8 *dst, const __global long8 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local ulong8*)dst, (const global ulong8*)src, num_elements, src_stride, evt, _p3v8i64_p1v8i64)
}

// long16
INLINE event_t OVERLOADABLE async_work_group_copy(__local long16 *dst, const __global long16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((local ulong16*)dst, (const global ulong16*)src, num_elements, evt, _p3v16i64_p1v16i64)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local long16 *dst, const __global long16 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S((local ulong16*)dst, (const global ulong16*)src, num_elements, src_stride, evt, _p3v16i64_p1v16i64)
}

// uchar
INLINE event_t OVERLOADABLE async_work_group_copy(__local unsigned char *dst, const __global unsigned char *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3i8_p1i8);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local unsigned char *dst, const __global unsigned char *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3i8_p1i8);

}

// uchar2
INLINE event_t OVERLOADABLE async_work_group_copy(__local uchar2 *dst, const __global uchar2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v2i8_p1v2i8);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local uchar2 *dst, const __global uchar2 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v2i8_p1v2i8);

}

// uchar3
INLINE event_t OVERLOADABLE async_work_group_copy(__local uchar3 *dst, const __global uchar3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v3i8_p1v3i8);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local uchar3 *dst, const __global uchar3 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v3i8_p1v3i8);

}

// uchar4
INLINE event_t OVERLOADABLE async_work_group_copy(__local uchar4 *dst, const __global uchar4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v4i8_p1v4i8);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local uchar4 *dst, const __global uchar4 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v4i8_p1v4i8);

}

// uchar8
INLINE event_t OVERLOADABLE async_work_group_copy(__local uchar8 *dst, const __global uchar8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v8i8_p1v8i8);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local uchar8 *dst, const __global uchar8 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v8i8_p1v8i8);

}

// uchar16
INLINE event_t OVERLOADABLE async_work_group_copy(__local uchar16 *dst, const __global uchar16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v16i8_p1v16i8);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local uchar16 *dst, const __global uchar16 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v16i8_p1v16i8);

}




// ushort
INLINE event_t OVERLOADABLE async_work_group_copy(__local ushort *dst, const __global ushort *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3i16_p1i16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local ushort *dst, const __global ushort *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3i16_p1i16);

}

// ushort2
INLINE event_t OVERLOADABLE async_work_group_copy(__local ushort2 *dst, const __global ushort2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v2i16_p1v2i16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local ushort2 *dst, const __global ushort2 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v2i16_p1v2i16);

}

// ushort3
INLINE event_t OVERLOADABLE async_work_group_copy(__local ushort3 *dst, const __global ushort3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v3i16_p1v3i16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local ushort3 *dst, const __global ushort3 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v3i16_p1v3i16);

}

// ushort4
INLINE event_t OVERLOADABLE async_work_group_copy(__local ushort4 *dst, const __global ushort4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v4i16_p1v4i16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local ushort4 *dst, const __global ushort4 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v4i16_p1v4i16);

}

// ushort8
INLINE event_t OVERLOADABLE async_work_group_copy(__local ushort8 *dst, const __global ushort8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v8i16_p1v8i16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local ushort8 *dst, const __global ushort8 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v8i16_p1v8i16);

}

// ushort16
INLINE event_t OVERLOADABLE async_work_group_copy(__local ushort16 *dst, const __global ushort16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v16i16_p1v16i16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local ushort16 *dst, const __global ushort16 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v16i16_p1v16i16);

}

// uint
INLINE event_t OVERLOADABLE async_work_group_copy(__local unsigned int *dst, const __global unsigned int *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3i32_p1i32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local unsigned int *dst, const __global unsigned int *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3i32_p1i32);

}

// uint2
INLINE event_t OVERLOADABLE async_work_group_copy(__local uint2 *dst, const __global uint2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v2i32_p1v2i32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local uint2 *dst, const __global uint2 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v2i32_p1v2i32);

}

// uint3
INLINE event_t OVERLOADABLE async_work_group_copy(__local uint3 *dst, const __global uint3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v3i32_p1v3i32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local uint3 *dst, const __global uint3 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v3i32_p1v3i32);

}

// uint4
INLINE event_t OVERLOADABLE async_work_group_copy(__local uint4 *dst, const __global uint4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v4i32_p1v4i32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local uint4 *dst, const __global uint4 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v4i32_p1v4i32);

}

// uint8
INLINE event_t OVERLOADABLE async_work_group_copy(__local uint8 *dst, const __global uint8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v8i32_p1v8i32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local uint8 *dst, const __global uint8 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v8i32_p1v8i32);

}

// uint16
INLINE event_t OVERLOADABLE async_work_group_copy(__local uint16 *dst, const __global uint16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v16i32_p1v16i32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local uint16 *dst, const __global uint16 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v16i32_p1v16i32);

}

// ulong
INLINE event_t OVERLOADABLE async_work_group_copy(__local unsigned long *dst, const __global unsigned long *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3i64_p1i64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local unsigned long *dst, const __global unsigned long *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3i64_p1i64);

}

// ulong2
INLINE event_t OVERLOADABLE async_work_group_copy(__local ulong2 *dst, const __global ulong2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v2i64_p1v2i64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local ulong2 *dst, const __global ulong2 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v2i64_p1v2i64);

}

// ulong3
INLINE event_t OVERLOADABLE async_work_group_copy(__local ulong3 *dst, const __global ulong3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v3i64_p1v3i64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local ulong3 *dst, const __global ulong3 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v3i64_p1v3i64);

}

// ulong4
INLINE event_t OVERLOADABLE async_work_group_copy(__local ulong4 *dst, const __global ulong4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v4i64_p1v4i64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local ulong4 *dst, const __global ulong4 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v4i64_p1v4i64);

}

// ulong8
INLINE event_t OVERLOADABLE async_work_group_copy(__local ulong8 *dst, const __global ulong8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v8i64_p1v8i64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local ulong8 *dst, const __global ulong8 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v8i64_p1v8i64);

}

// ulong16
INLINE event_t OVERLOADABLE async_work_group_copy(__local ulong16 *dst, const __global ulong16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v16i64_p1v16i64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local ulong16 *dst, const __global ulong16 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v16i64_p1v16i64);

}



// float
INLINE event_t OVERLOADABLE async_work_group_copy(__local float *dst, const __global float *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3f32_p1f32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local float *dst, const __global float *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3f32_p1f32);

}

// float2
INLINE event_t OVERLOADABLE async_work_group_copy(__local float2 *dst, const __global float2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v2f32_p1v2f32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local float2 *dst, const __global float2 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v2f32_p1v2f32);

}

// float3
INLINE event_t OVERLOADABLE async_work_group_copy(__local float3 *dst, const __global float3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v3f32_p1v3f32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local float3 *dst, const __global float3 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v3f32_p1v3f32);

}

// float4
INLINE event_t OVERLOADABLE async_work_group_copy(__local float4 *dst, const __global float4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v4f32_p1v4f32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local float4 *dst, const __global float4 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v4f32_p1v4f32);

}

// float8
INLINE event_t OVERLOADABLE async_work_group_copy(__local float8 *dst, const __global float8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v8f32_p1v8f32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local float8 *dst, const __global float8 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v8f32_p1v8f32);

}

// float16
INLINE event_t OVERLOADABLE async_work_group_copy(__local float16 *dst, const __global float16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v16f32_p1v16f32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local float16 *dst, const __global float16 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v16f32_p1v16f32);

}

#if defined(cl_khr_fp16)

// half
INLINE event_t OVERLOADABLE async_work_group_copy(__local half *dst, const __global half *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3f16_p1f16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local half *dst, const __global half *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3f16_p1f16);

}

// half2
INLINE event_t OVERLOADABLE async_work_group_copy(__local half2 *dst, const __global half2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v2f16_p1v2f16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local half2 *dst, const __global half2 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v2f16_p1v2f16);

}

// half3
INLINE event_t OVERLOADABLE async_work_group_copy(__local half3 *dst, const __global half3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v3f16_p1v3f16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local half3 *dst, const __global half3 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v3f16_p1v3f16);

}

// half4
INLINE event_t OVERLOADABLE async_work_group_copy(__local half4 *dst, const __global half4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v4f16_p1v4f16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local half4 *dst, const __global half4 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v4f16_p1v4f16);

}

// half8
INLINE event_t OVERLOADABLE async_work_group_copy(__local half8 *dst, const __global half8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v8f16_p1v8f16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local half8 *dst, const __global half8 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v8f16_p1v8f16);

}

// half16
INLINE event_t OVERLOADABLE async_work_group_copy(__local half16 *dst, const __global half16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v16f16_p1v16f16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local half16 *dst, const __global half16 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v16f16_p1v16f16);

}

#endif // defined(cl_khr_fp16)

#if defined(cl_khr_fp64)

// double
INLINE event_t OVERLOADABLE async_work_group_copy(__local double *dst, const __global double *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3f64_p1f64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local double *dst, const __global double *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3f64_p1f64);

}

// double2
INLINE event_t OVERLOADABLE async_work_group_copy(__local double2 *dst, const __global double2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v2f64_p1v2f64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local double2 *dst, const __global double2 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v2f64_p1v2f64);

}

// double3
INLINE event_t OVERLOADABLE async_work_group_copy(__local double3 *dst, const __global double3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v3f64_p1v3f64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local double3 *dst, const __global double3 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v3f64_p1v3f64);

}

// double4
INLINE event_t OVERLOADABLE async_work_group_copy(__local double4 *dst, const __global double4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v4f64_p1v4f64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local double4 *dst, const __global double4 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v4f64_p1v4f64);

}

// double8
INLINE event_t OVERLOADABLE async_work_group_copy(__local double8 *dst, const __global double8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v8f64_p1v8f64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local double8 *dst, const __global double8 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v8f64_p1v8f64);

}

// double16
INLINE event_t OVERLOADABLE async_work_group_copy(__local double16 *dst, const __global double16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p3v16f64_p1v16f64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__local double16 *dst, const __global double16 *src, size_t num_elements, size_t src_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, src_stride, evt, _p3v16f64_p1v16f64);

}

#endif // defined(cl_khr_fp64)




// ************************************* Local to Global.*********************************************

// char
INLINE event_t OVERLOADABLE async_work_group_copy(__global char *dst, const __local char *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global uchar*)dst, (const local uchar*)src, num_elements, evt, _p1i8_p3i8)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global char *dst, const __local char *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global uchar*)dst, (const local uchar*)src, num_elements, dst_stride, evt, _p1i8_p3i8)
}

// char2
INLINE event_t OVERLOADABLE async_work_group_copy(__global char2 *dst, const __local char2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global uchar2*)dst, (const local uchar2*)src, num_elements, evt, _p1v2i8_p3v2i8)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global char2 *dst, const __local char2 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global uchar2*)dst, (const local uchar2*)src, num_elements, dst_stride, evt, _p1v2i8_p3v2i8)
}

// char3
INLINE event_t OVERLOADABLE async_work_group_copy(__global char3 *dst, const __local char3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global uchar3*)dst, (const local uchar3*)src, num_elements, evt, _p1v3i8_p3v3i8)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global char3 *dst, const __local char3 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global uchar3*)dst, (const local uchar3*)src, num_elements, dst_stride, evt, _p1v3i8_p3v3i8)
}

// char4
INLINE event_t OVERLOADABLE async_work_group_copy(__global char4 *dst, const __local char4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global uchar4*)dst, (const local uchar4*)src, num_elements, evt, _p1v4i8_p3v4i8)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global char4 *dst, const __local char4 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global uchar4*)dst, (const local uchar4*)src, num_elements, dst_stride, evt, _p1v4i8_p3v4i8)
}

// char8
INLINE event_t OVERLOADABLE async_work_group_copy(__global char8 *dst, const __local char8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global uchar8*)dst, (const local uchar8*)src, num_elements, evt, _p1v8i8_p3v8i8)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global char8 *dst, const __local char8 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global uchar8*)dst, (const local uchar8*)src, num_elements, dst_stride, evt, _p1v8i8_p3v8i8)
}

// char16
INLINE event_t OVERLOADABLE async_work_group_copy(__global char16 *dst, const __local char16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global uchar16*)dst, (const local uchar16*)src, num_elements, evt, _p1v16i8_p3v16i8)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global char16 *dst, const __local char16 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global uchar16*)dst, (const local uchar16*)src, num_elements, dst_stride, evt, _p1v16i8_p3v16i8)
}

// short
INLINE event_t OVERLOADABLE async_work_group_copy(__global short *dst, const __local short *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global ushort*)dst, (const local ushort*)src, num_elements, evt, _p1i16_p3i16)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global short *dst, const __local short *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global ushort*)dst, (const local ushort*)src, num_elements, dst_stride, evt, _p1i16_p3i16)
}

// short2
INLINE event_t OVERLOADABLE async_work_group_copy(__global short2 *dst, const __local short2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global ushort2*)dst, (const local ushort2*)src, num_elements, evt, _p1v2i16_p3v2i16)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global short2 *dst, const __local short2 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global ushort2*)dst, (const local ushort2*)src, num_elements, dst_stride, evt, _p1v2i16_p3v2i16)
}

// short3
INLINE event_t OVERLOADABLE async_work_group_copy(__global short3 *dst, const __local short3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global ushort3*)dst, (const local ushort3*)src, num_elements, evt, _p1v3i16_p3v3i16)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global short3 *dst, const __local short3 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global ushort3*)dst, (const local ushort3*)src, num_elements, dst_stride, evt, _p1v3i16_p3v3i16)
}

// short4
INLINE event_t OVERLOADABLE async_work_group_copy(__global short4 *dst, const __local short4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global ushort4*)dst, (const local ushort4*)src, num_elements, evt, _p1v4i16_p3v4i16)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global short4 *dst, const __local short4 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global ushort4*)dst, (const local ushort4*)src, num_elements, dst_stride, evt, _p1v4i16_p3v4i16)
}

// short8
INLINE event_t OVERLOADABLE async_work_group_copy(__global short8 *dst, const __local short8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global ushort8*)dst, (const local ushort8*)src, num_elements, evt, _p1v8i16_p3v8i16)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global short8 *dst, const __local short8 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global ushort8*)dst,(const local ushort8*) src, num_elements, dst_stride, evt, _p1v8i16_p3v8i16)
}

// short16
INLINE event_t OVERLOADABLE async_work_group_copy(__global short16 *dst, const __local short16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global ushort16*)dst, (const local ushort16*)src, num_elements, evt, _p1v16i16_p3v16i16)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global short16 *dst, const __local short16 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global ushort16*)dst, (const local ushort16*)src, num_elements, dst_stride, evt, _p1v16i16_p3v16i16)
}

// int
INLINE event_t OVERLOADABLE async_work_group_copy(__global int *dst, const __local int *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global uint*)dst, (const local uint*)src, num_elements, evt, _p1i32_p3i32)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global int *dst, const __local int *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global uint*)dst, (const local uint*)src, num_elements, dst_stride, evt, _p1i32_p3i32)
}

// int2
INLINE event_t OVERLOADABLE async_work_group_copy(__global int2 *dst, const __local int2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global uint2*)dst, (const local uint2*)src, num_elements, evt, _p1v2i32_p3v2i32)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global int2 *dst, const __local int2 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global uint2*)dst, (const local uint2*)src, num_elements, dst_stride, evt, _p1v2i32_p3v2i32)
}

// int3
INLINE event_t OVERLOADABLE async_work_group_copy(__global int3 *dst, const __local int3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global uint3*)dst, (const local uint3*)src, num_elements, evt, _p1v3i32_p3v3i32)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global int3 *dst, const __local int3 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global uint3*)dst, (const local uint3*)src, num_elements, dst_stride, evt, _p1v3i32_p3v3i32)
}

// int4
INLINE event_t OVERLOADABLE async_work_group_copy(__global int4 *dst, const __local int4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global uint4*)dst, (const local uint4*)src, num_elements, evt, _p1v4i32_p3v4i32)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global int4 *dst, const __local int4 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global uint4*)dst, (const local uint4*)src, num_elements, dst_stride, evt, _p1v4i32_p3v4i32)
}

// int8
INLINE event_t OVERLOADABLE async_work_group_copy(__global int8 *dst, const __local int8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global uint8*)dst, (const local uint8*)src, num_elements, evt, _p1v8i32_p3v8i32)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global int8 *dst, const __local int8 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global uint8*)dst, (const local uint8*)src, num_elements, dst_stride, evt, _p1v8i32_p3v8i32)
}

// int16
INLINE event_t OVERLOADABLE async_work_group_copy(__global int16 *dst, const __local int16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global uint16*)dst, (const local uint16*)src, num_elements, evt, _p1v16i32_p3v16i32)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global int16 *dst, const __local int16 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global uint16*)dst, (const local uint16*)src, num_elements, dst_stride, evt, _p1v16i32_p3v16i32)
}

// long
INLINE event_t OVERLOADABLE async_work_group_copy(__global long *dst, const __local long *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global ulong*)dst, (const local ulong*)src, num_elements, evt, _p1i64_p3i64)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global long *dst, const __local long *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global ulong*)dst, (const local ulong*)src, num_elements, dst_stride, evt, _p1i64_p3i64)
}

// long2
INLINE event_t OVERLOADABLE async_work_group_copy(__global long2 *dst, const __local long2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global ulong2*)dst, (const local ulong2*)src, num_elements, evt, _p1v2i64_p3v2i64)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global long2 *dst, const __local long2 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global ulong2*)dst, (const local ulong2*)src, num_elements, dst_stride, evt, _p1v2i64_p3v2i64)
}

// long3
INLINE event_t OVERLOADABLE async_work_group_copy(__global long3 *dst, const __local long3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global ulong3*)dst, (const local ulong3*)src, num_elements, evt, _p1v3i64_p3v3i64)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global long3 *dst, const __local long3 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global ulong3*)dst, (const local ulong3*)src, num_elements, dst_stride, evt, _p1v3i64_p3v3i64)
}

// long4
INLINE event_t OVERLOADABLE async_work_group_copy(__global long4 *dst, const __local long4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global ulong4*)dst, (const local ulong4*)src, num_elements, evt, _p1v4i64_p3v4i64)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global long4 *dst, const __local long4 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global ulong4*)dst, (const local ulong4*)src, num_elements, dst_stride, evt, _p1v4i64_p3v4i64)
}

// long8
INLINE event_t OVERLOADABLE async_work_group_copy(__global long8 *dst, const __local long8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global ulong8*)dst, (const local ulong8*)src, num_elements, evt, _p1v8i64_p3v8i64)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global long8 *dst, const __local long8 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global ulong8*)dst, (const local ulong8*)src, num_elements, dst_stride, evt, _p1v8i64_p3v8i64)
}

// long16
INLINE event_t OVERLOADABLE async_work_group_copy(__global long16 *dst, const __local long16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY((global ulong16*)dst, (const local ulong16*)src, num_elements, evt, _p1v16i64_p3v16i64)
}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global long16 *dst, const __local long16 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S((global ulong16*)dst, (const local ulong16*)src, num_elements, dst_stride, evt, _p1v16i64_p3v16i64)
}

// uchar
INLINE event_t OVERLOADABLE async_work_group_copy(__global unsigned char *dst, const __local unsigned char *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1i8_p3i8);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global unsigned char *dst, const __local unsigned char *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1i8_p3i8);

}

// uchar2
INLINE event_t OVERLOADABLE async_work_group_copy(__global uchar2 *dst, const __local uchar2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v2i8_p3v2i8);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global uchar2 *dst, const __local uchar2 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v2i8_p3v2i8);

}

// uchar3
INLINE event_t OVERLOADABLE async_work_group_copy(__global uchar3 *dst, const __local uchar3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v3i8_p3v3i8);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global uchar3 *dst, const __local uchar3 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v3i8_p3v3i8);

}

// uchar4
INLINE event_t OVERLOADABLE async_work_group_copy(__global uchar4 *dst, const __local uchar4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v4i8_p3v4i8);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global uchar4 *dst, const __local uchar4 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v4i8_p3v4i8);

}

// uchar8
INLINE event_t OVERLOADABLE async_work_group_copy(__global uchar8 *dst, const __local uchar8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v8i8_p3v8i8);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global uchar8 *dst, const __local uchar8 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v8i8_p3v8i8);

}

// uchar16
INLINE event_t OVERLOADABLE async_work_group_copy(__global uchar16 *dst, const __local uchar16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v16i8_p3v16i8);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global uchar16 *dst, const __local uchar16 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v16i8_p3v16i8);

}




// ushort
INLINE event_t OVERLOADABLE async_work_group_copy(__global ushort *dst, const __local ushort *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1i16_p3i16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global ushort *dst, const __local ushort *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1i16_p3i16);

}

// ushort2
INLINE event_t OVERLOADABLE async_work_group_copy(__global ushort2 *dst, const __local ushort2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v2i16_p3v2i16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global ushort2 *dst, const __local ushort2 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v2i16_p3v2i16);

}

// ushort3
INLINE event_t OVERLOADABLE async_work_group_copy(__global ushort3 *dst, const __local ushort3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v3i16_p3v3i16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global ushort3 *dst, const __local ushort3 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v3i16_p3v3i16);

}

// ushort4
INLINE event_t OVERLOADABLE async_work_group_copy(__global ushort4 *dst, const __local ushort4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v4i16_p3v4i16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global ushort4 *dst, const __local ushort4 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v4i16_p3v4i16);

}

// ushort8
INLINE event_t OVERLOADABLE async_work_group_copy(__global ushort8 *dst, const __local ushort8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v8i16_p3v8i16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global ushort8 *dst, const __local ushort8 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v8i16_p3v8i16);

}

// ushort16
INLINE event_t OVERLOADABLE async_work_group_copy(__global ushort16 *dst, const __local ushort16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v16i16_p3v16i16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global ushort16 *dst, const __local ushort16 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v16i16_p3v16i16);

}

// uint
INLINE event_t OVERLOADABLE async_work_group_copy(__global unsigned int *dst, const __local unsigned int *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1i32_p3i32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global unsigned int *dst, const __local unsigned int *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1i32_p3i32);

}

// uint2
INLINE event_t OVERLOADABLE async_work_group_copy(__global uint2 *dst, const __local uint2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v2i32_p3v2i32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global uint2 *dst, const __local uint2 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v2i32_p3v2i32);

}

// uint3
INLINE event_t OVERLOADABLE async_work_group_copy(__global uint3 *dst, const __local uint3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v3i32_p3v3i32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global uint3 *dst, const __local uint3 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v3i32_p3v3i32);

}

// uint4
INLINE event_t OVERLOADABLE async_work_group_copy(__global uint4 *dst, const __local uint4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v4i32_p3v4i32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global uint4 *dst, const __local uint4 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v4i32_p3v4i32);

}

// uint8
INLINE event_t OVERLOADABLE async_work_group_copy(__global uint8 *dst, const __local uint8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v8i32_p3v8i32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global uint8 *dst, const __local uint8 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v8i32_p3v8i32);

}

// uint16
INLINE event_t OVERLOADABLE async_work_group_copy(__global uint16 *dst, const __local uint16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v16i32_p3v16i32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global uint16 *dst, const __local uint16 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v16i32_p3v16i32);

}

// ulong
INLINE event_t OVERLOADABLE async_work_group_copy(__global unsigned long *dst, const __local unsigned long *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1i64_p3i64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global unsigned long *dst, const __local unsigned long *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1i64_p3i64);

}

// ulong2
INLINE event_t OVERLOADABLE async_work_group_copy(__global ulong2 *dst, const __local ulong2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v2i64_p3v2i64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global ulong2 *dst, const __local ulong2 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v2i64_p3v2i64);

}

// ulong3
INLINE event_t OVERLOADABLE async_work_group_copy(__global ulong3 *dst, const __local ulong3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v3i64_p3v3i64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global ulong3 *dst, const __local ulong3 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v3i64_p3v3i64);

}

// ulong4
INLINE event_t OVERLOADABLE async_work_group_copy(__global ulong4 *dst, const __local ulong4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v4i64_p3v4i64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global ulong4 *dst, const __local ulong4 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v4i64_p3v4i64);

}

// ulong8
INLINE event_t OVERLOADABLE async_work_group_copy(__global ulong8 *dst, const __local ulong8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v8i64_p3v8i64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global ulong8 *dst, const __local ulong8 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v8i64_p3v8i64);

}

// ulong16
INLINE event_t OVERLOADABLE async_work_group_copy(__global ulong16 *dst, const __local ulong16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v16i64_p3v16i64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global ulong16 *dst, const __local ulong16 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v16i64_p3v16i64);

}


// float
INLINE event_t OVERLOADABLE async_work_group_copy(__global float *dst, const __local float *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1f32_p3f32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global float *dst, const __local float *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1f32_p3f32);

}

// float2
INLINE event_t OVERLOADABLE async_work_group_copy(__global float2 *dst, const __local float2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v2f32_p3v2f32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global float2 *dst, const __local float2 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v2f32_p3v2f32);

}

// float3
INLINE event_t OVERLOADABLE async_work_group_copy(__global float3 *dst, const __local float3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v3f32_p3v3f32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global float3 *dst, const __local float3 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v3f32_p3v3f32);

}

// float4
INLINE event_t OVERLOADABLE async_work_group_copy(__global float4 *dst, const __local float4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v4f32_p3v4f32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global float4 *dst, const __local float4 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v4f32_p3v4f32);

}

// float8
INLINE event_t OVERLOADABLE async_work_group_copy(__global float8 *dst, const __local float8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v8f32_p3v8f32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global float8 *dst, const __local float8 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v8f32_p3v8f32);

}

// float16
INLINE event_t OVERLOADABLE async_work_group_copy(__global float16 *dst, const __local float16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v16f32_p3v16f32);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global float16 *dst, const __local float16 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v16f32_p3v16f32);

}

#if defined(cl_khr_fp16)

// half
INLINE event_t OVERLOADABLE async_work_group_copy(__global half *dst, const __local half *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1f16_p3f16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global half *dst, const __local half *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1f16_p3f16);

}

// half2
INLINE event_t OVERLOADABLE async_work_group_copy(__global half2 *dst, const __local half2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v2f16_p3v2f16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global half2 *dst, const __local half2 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v2f16_p3v2f16);

}

// half3
INLINE event_t OVERLOADABLE async_work_group_copy(__global half3 *dst, const __local half3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v3f16_p3v3f16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global half3 *dst, const __local half3 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v3f16_p3v3f16);

}

// half4
INLINE event_t OVERLOADABLE async_work_group_copy(__global half4 *dst, const __local half4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v4f16_p3v4f16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global half4 *dst, const __local half4 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v4f16_p3v4f16);

}

// half8
INLINE event_t OVERLOADABLE async_work_group_copy(__global half8 *dst, const __local half8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v8f16_p3v8f16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global half8 *dst, const __local half8 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v8f16_p3v8f16);

}

// half16
INLINE event_t OVERLOADABLE async_work_group_copy(__global half16 *dst, const __local half16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v16f16_p3v16f16);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global half16 *dst, const __local half16 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v16f16_p3v16f16);

}

#endif // defined(cl_khr_fp16)

#if defined(cl_khr_fp64)

// double
INLINE event_t OVERLOADABLE async_work_group_copy(__global double *dst, const __local double *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1f64_p3f64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global double *dst, const __local double *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1f64_p3f64);

}

// double2
INLINE event_t OVERLOADABLE async_work_group_copy(__global double2 *dst, const __local double2 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v2f64_p3v2f64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global double2 *dst, const __local double2 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v2f64_p3v2f64);

}

// double3
INLINE event_t OVERLOADABLE async_work_group_copy(__global double3 *dst, const __local double3 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v3f64_p3v3f64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global double3 *dst, const __local double3 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v3f64_p3v3f64);

}

// double4
INLINE event_t OVERLOADABLE async_work_group_copy(__global double4 *dst, const __local double4 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v4f64_p3v4f64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global double4 *dst, const __local double4 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v4f64_p3v4f64);

}

// double8
INLINE event_t OVERLOADABLE async_work_group_copy(__global double8 *dst, const __local double8 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v8f64_p3v8f64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global double8 *dst, const __local double8 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v8f64_p3v8f64);

}

// double16
INLINE event_t OVERLOADABLE async_work_group_copy(__global double16 *dst, const __local double16 *src, size_t num_elements, event_t evt) {
  ASYNC_COPY(dst, src, num_elements, evt, _p1v16f64_p3v16f64);

}

INLINE event_t OVERLOADABLE async_work_group_strided_copy(__global double16 *dst, const __local double16 *src, size_t num_elements, size_t dst_stride, event_t evt) {
  ASYNC_COPY_S(dst, src, num_elements, dst_stride, evt, _p1v16f64_p3v16f64);

}

#endif // defined(cl_khr_fp64)

//*****************************************************************************/
// get_image_array_size() overloads
//*****************************************************************************/

INLINE size_t OVERLOADABLE get_image_array_size(read_only image1d_array_t image) {
    int id = (int)__builtin_astype(image, __global void*);
    return __builtin_IB_get_image_array_size(id);
}

INLINE size_t OVERLOADABLE get_image_array_size(read_only image2d_array_t image) {
    int id = (int)__builtin_astype(image, __global void*);
    return __builtin_IB_get_image_array_size(id);
}

INLINE size_t OVERLOADABLE get_image_array_size(read_only image2d_array_depth_t image) {
    int id = (int)__builtin_astype(image, __global void*);
    return __builtin_IB_get_image_array_size(id);
}

INLINE size_t OVERLOADABLE get_image_array_size(read_only image2d_array_msaa_t image) {
    int id = (int)__builtin_astype(image, __global void*);
    return __builtin_IB_get_image_array_size(id);
}

INLINE size_t OVERLOADABLE get_image_array_size(read_only image2d_array_msaa_depth_t image) {
    int id = (int)__builtin_astype(image, __global void*);
    return __builtin_IB_get_image_array_size(id);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE size_t OVERLOADABLE get_image_array_size(write_only image1d_array_t image) {
    int id = (int)__builtin_astype(image, __global void*);
    return __builtin_IB_get_image_array_size(id);
}

INLINE size_t OVERLOADABLE get_image_array_size(write_only image2d_array_t image) {
    int id = (int)__builtin_astype(image, __global void*);
    return __builtin_IB_get_image_array_size(id);
}

INLINE size_t OVERLOADABLE get_image_array_size(write_only image2d_array_depth_t image) {
    int id = (int)__builtin_astype(image, __global void*);
    return __builtin_IB_get_image_array_size(id);
}

INLINE size_t OVERLOADABLE get_image_array_size(write_only image2d_array_msaa_t image) {
    int id = (int)__builtin_astype(image, __global void*);
    return __builtin_IB_get_image_array_size(id);
}

INLINE size_t OVERLOADABLE get_image_array_size(write_only image2d_array_msaa_depth_t image) {
    int id = (int)__builtin_astype(image, __global void*);
    return __builtin_IB_get_image_array_size(id);
}

INLINE size_t OVERLOADABLE get_image_array_size(read_write image1d_array_t image) {
    int id = (int)__builtin_astype(image, __global void*);
    return __builtin_IB_get_image_array_size(id);
}

INLINE size_t OVERLOADABLE get_image_array_size(read_write image2d_array_t image) {
    int id = (int)__builtin_astype(image, __global void*);
    return __builtin_IB_get_image_array_size(id);
}

INLINE size_t OVERLOADABLE get_image_array_size(read_write image2d_array_depth_t image) {
    int id = (int)__builtin_astype(image, __global void*);
    return __builtin_IB_get_image_array_size(id);
}

INLINE size_t OVERLOADABLE get_image_array_size(read_write image2d_array_msaa_t image) {
    int id = (int)__builtin_astype(image, __global void*);
    return __builtin_IB_get_image_array_size(id);
}

INLINE size_t OVERLOADABLE get_image_array_size(read_write image2d_array_msaa_depth_t image) {
    int id = (int)__builtin_astype(image, __global void*);
    return __builtin_IB_get_image_array_size(id);
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

//*****************************************************************************/
// prefetch function overloads
//*****************************************************************************/

// The OpenCL spec, technically does not allow a void* version of prefetch(),
// but it allows us to not implemented type specific versions because any pointer
// can be implicitly cast to void*.
//
INLINE void OVERLOADABLE prefetch(const __global void *p, size_t num_elements) {
  return;
}

// The following are needed for backwards compatibility - old binaries may have
// type specific versions of these functions - implemented below.

INLINE void OVERLOADABLE prefetch(const __global char *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global char2 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global char3 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global char4 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global char8 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global char16 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global uchar *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global uchar2 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global uchar3 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global uchar4 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global uchar8 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global uchar16 *p, size_t num_elements) {
  return;
}

INLINE void OVERLOADABLE prefetch(const __global short *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global short2 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global short3 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global short4 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global short8 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global short16 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global ushort *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global ushort2 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global ushort3 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global ushort4 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global ushort8 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global ushort16 *p, size_t num_elements) {
  return;
}

INLINE void OVERLOADABLE prefetch(const __global int *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global int2 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global int3 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global int4 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global int8 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global int16 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global uint *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global uint2 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global uint3 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global uint4 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global uint8 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global uint16 *p, size_t num_elements) {
  return;
}

INLINE void OVERLOADABLE prefetch(const __global long *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global long2 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global long3 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global long4 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global long8 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global long16 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global ulong *p, size_t num_elements) {
 return;
}
INLINE void OVERLOADABLE prefetch(const __global ulong2 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global ulong3 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global ulong4 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global ulong8 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global ulong16 *p, size_t num_elements) {
  return;
}

INLINE void OVERLOADABLE prefetch(const __global float *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global float2 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global float3 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global float4 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global float8 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global float16 *p, size_t num_elements) {
  return;
}

#if defined(cl_khr_fp16)
INLINE void OVERLOADABLE prefetch(const __global half *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global half2 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global half3 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global half4 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global half8 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global half16 *p, size_t num_elements) {
  return;
}
#endif

#if defined(cl_khr_fp64)
INLINE void OVERLOADABLE prefetch(const __global double *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global double2 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global double3 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global double4 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global double8 *p, size_t num_elements) {
  return;
}
INLINE void OVERLOADABLE prefetch(const __global double16 *p, size_t num_elements) {
  return;
}
#endif


// This vload function is added because of the clang 4.0 itanium mangling update
// See http://llvm.org/viewvc/llvm-project?view=revision&revision=262414

INLINE char16 _Z7vload16jPKU3AS1h(size_t offset, const __global char *p)
{
    return vload16(offset,p);
}

///////// overloads for backwards compatibility with old clang/llvm 3.0

