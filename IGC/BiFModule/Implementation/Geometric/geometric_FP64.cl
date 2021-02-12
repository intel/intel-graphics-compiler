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

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


// cross

INLINE double3 __builtin_spirv_OpenCL_cross_v3f64_v3f64(double3 p0, double3 p1 ){
    double3 result;
    result.x = __builtin_spirv_OpenCL_fma_f64_f64_f64(p0.y, p1.z, -p0.z * p1.y );
    result.y = __builtin_spirv_OpenCL_fma_f64_f64_f64(p0.z, p1.x, -p0.x * p1.z );
    result.z = __builtin_spirv_OpenCL_fma_f64_f64_f64(p0.x, p1.y, -p0.y * p1.x );

    return result;
}

INLINE double4 __builtin_spirv_OpenCL_cross_v4f64_v4f64(double4 p0, double4 p1 ){
    double4 result;
    result.xyz = __builtin_spirv_OpenCL_cross_v3f64_v3f64( p0.xyz, p1.xyz );

    result.w = (half) 0.0;

    return result;
}

// distance

INLINE double __builtin_spirv_OpenCL_distance_f64_f64(double p0, double p1 ){
    return __builtin_spirv_OpenCL_length_f64( p0 - p1 );
}

INLINE double __builtin_spirv_OpenCL_distance_v2f64_v2f64(double2 p0, double2 p1 ){
    return __builtin_spirv_OpenCL_length_v2f64( p0 - p1 );
}

INLINE double __builtin_spirv_OpenCL_distance_v3f64_v3f64(double3 p0, double3 p1 ){
    return __builtin_spirv_OpenCL_length_v3f64( p0 - p1 );
}

INLINE double __builtin_spirv_OpenCL_distance_v4f64_v4f64(double4 p0, double4 p1 ){
    return __builtin_spirv_OpenCL_length_v4f64( p0 - p1 );
}