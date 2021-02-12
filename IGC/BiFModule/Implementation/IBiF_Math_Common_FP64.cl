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
//    Copyright (C) 1996-2010 Intel Corporation. All Rights Reserved.
//
*/

extern __constant int __FastRelaxedMath;

OVERLOADABLE int __intel_relaxed_isnan(double x );
OVERLOADABLE int __intel_relaxed_isinf(double x );
OVERLOADABLE int __intel_relaxed_isfinite(double x );
OVERLOADABLE int __intel_relaxed_isnormal(double x );

// Common file for intrinsics implementation used in Math library
#include "IBiF_Intrinsics_Impl_FP64.cl"

// Common
#include "Common/common_FP64.cl"

// Geometric
#include "Geometric/geometric_FP64.cl"
#include "ExternalLibraries/libclc/normalize_FP64.cl"
#include "ExternalLibraries/libclc/length_FP64.cl"

// Math
#include "Math/math_FP64.cl"

// Native
#include "Native/native_FP64.cl"

// Relational
#include "Relational/relational_FP64.cl"
