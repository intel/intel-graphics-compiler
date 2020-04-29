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
#include "spirv.h"

// This is a workaround for unexpected behaviour of SPIRV-LLVM Translator:
// OpExtInst %float fmin_common  -->  _Z11fmin_commonff
// Whereas expected (clang-consistent) behaviour is:
// OpExtInst %float fmin_common  -->  _Z3minff
// It doesn't affect functionallity, it's just naming matter.

INLINE float OVERLOADABLE fmin_common(float x, float y)
{
    return __builtin_spirv_OpenCL_fmin_common_f32_f32(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS(fmin_common, float, float)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(fmin_common, float, float, float)

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE fmin_common(double x, double y)
{
    return __builtin_spirv_OpenCL_fmin_common_f64_f64(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS(fmin_common, double, double)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(fmin_common, double, double, double)

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE fmin_common(half x, half y)
{
    return __builtin_spirv_OpenCL_fmin_common_f16_f16(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS(fmin_common, half, half)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(fmin_common, half, half, half)

#endif // defined(cl_khr_fp16)
