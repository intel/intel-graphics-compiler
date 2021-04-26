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

// Arithmetic Instructions

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v2f64_v2f64, )(double2 Vector1, double2 Vector2)
{
    return __builtin_spirv_OpenCL_mad_f64_f64_f64(Vector1.x,  Vector2.x, (Vector1.y * Vector2.y));
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v3f64_v3f64, )(double3 Vector1, double3 Vector2)
{
    return __builtin_spirv_OpenCL_mad_f64_f64_f64(Vector1.x, Vector2.x,
           __builtin_spirv_OpenCL_mad_f64_f64_f64(Vector1.y, Vector2.y, (Vector1.z * Vector2.z)));
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v4f64_v4f64, )(double4 Vector1, double4 Vector2)
{
    return __builtin_spirv_OpenCL_mad_f64_f64_f64(Vector1.x, Vector2.x,
           __builtin_spirv_OpenCL_mad_f64_f64_f64(Vector1.y, Vector2.y,
           __builtin_spirv_OpenCL_mad_f64_f64_f64(Vector1.z, Vector2.z,
                                           (Vector1.w * Vector2.w))));
}
