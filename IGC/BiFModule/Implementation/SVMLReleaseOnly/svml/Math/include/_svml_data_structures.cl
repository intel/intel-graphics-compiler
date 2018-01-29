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


// SVML code
/*
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 1996-2015 Intel Corporation. All Rights Reserved.
//
*/


#ifndef __SVML_DATA_STRUCTURES_CL__
#define __SVML_DATA_STRUCTURES_CL__

/*
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 1996-2015 Intel Corporation. All Rights Reserved.
//
*/

typedef int VINT32;
typedef signed int VSINT32;
typedef unsigned int VUINT32;

typedef long VINT64;
typedef signed long VSINT64;
typedef unsigned long VUINT64;

typedef int int1_gen;
typedef long long1_gen;
typedef float float1_gen;
typedef float float1_ref;
typedef int int1_ref;
typedef int __int32;

typedef unsigned int  _iml_uint32_t;
typedef int           _iml_int32_t;
typedef unsigned long _iml_uint64_t;

typedef union
{
    uint hex;
    float fp;
} _iml_sp_union_t;

__attribute__((always_inline))
uint _castf32_u32(float a)
{
  return as_uint(a);
}

__attribute__((always_inline))
float _castu32_f32(uint a)
{
  return as_float(a);
}

#if defined(cl_khr_fp64)

typedef double double1_ref;

typedef struct tag_iml_dpdwords_t
{
  _iml_uint32_t lo_dword;
  _iml_uint32_t hi_dword;
} _iml_dpdwords_t;

typedef union
{
  _iml_uint32_t hex[2];
  _iml_dpdwords_t dwords;
  double fp;
} _iml_dp_union_t;

typedef union
{
  ulong hex;
  double fp;
} _lng_dbl_union_t;

__attribute__((always_inline))
double _castu64_f64(ulong a)
{
  return as_double(a);
}

__attribute__((always_inline))
ulong _castf64_u64(double a)
{
  return as_ulong(a);
}

__attribute__((always_inline))
double _nearbyint(double a)
{
  return __builtin_spirv_OpenCL_round_f64(a);
}
#endif

#endif // __SVML_DATA_STRUCTURES_CL__
