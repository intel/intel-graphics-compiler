/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

Copyright (c) 2014 Advanced Micro Devices, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

============================= end_copyright_notice ===========================*/

// Bool vector to integer vector conversions shared by libclc sources.

#ifndef __BOOL_CONVERSIONS_H__
#define __BOOL_CONVERSIONS_H__

static int2 INLINE OVERLOADABLE __convert_int2(__bool2 b) { return -(int2)(b.x, b.y); }

static int3 INLINE OVERLOADABLE __convert_int3(__bool3 b) { return -(int3)(b.x, b.y, b.z); }

static int4 INLINE OVERLOADABLE __convert_int4(__bool4 b) { return -(int4)(b.x, b.y, b.z, b.w); }

static int8 INLINE OVERLOADABLE __convert_int8(__bool8 b) { return (int8)(__convert_int4(b.lo), __convert_int4(b.hi)); }

static int16 INLINE OVERLOADABLE __convert_int16(__bool16 b) {
  return (int16)(__convert_int8(b.lo), __convert_int8(b.hi));
}

#if defined(cl_khr_fp64)

static long2 INLINE OVERLOADABLE __convert_long2(__bool2 b) { return -(long2)(b.x, b.y); }

static long3 INLINE OVERLOADABLE __convert_long3(__bool3 b) { return -(long3)(b.x, b.y, b.z); }

static long4 INLINE OVERLOADABLE __convert_long4(__bool4 b) { return -(long4)(b.x, b.y, b.z, b.w); }

static long8 INLINE OVERLOADABLE __convert_long8(__bool8 b) {
  return (long8)(__convert_long4(b.lo), __convert_long4(b.hi));
}

static long16 INLINE OVERLOADABLE __convert_long16(__bool16 b) {
  return (long16)(__convert_long8(b.lo), __convert_long8(b.hi));
}

#endif

static short2 INLINE OVERLOADABLE __convert_short2(__bool2 b) { return -(short2)(b.x, b.y); }

static short3 INLINE OVERLOADABLE __convert_short3(__bool3 b) { return -(short3)(b.x, b.y, b.z); }

static short4 INLINE OVERLOADABLE __convert_short4(__bool4 b) { return -(short4)(b.x, b.y, b.z, b.w); }

static short8 INLINE OVERLOADABLE __convert_short8(__bool8 b) {
  return (short8)(__convert_short4(b.lo), __convert_short4(b.hi));
}

static short16 INLINE OVERLOADABLE __convert_short16(__bool16 b) {
  return (short16)(__convert_short8(b.lo), __convert_short8(b.hi));
}

#endif // __BOOL_CONVERSIONS_H__
