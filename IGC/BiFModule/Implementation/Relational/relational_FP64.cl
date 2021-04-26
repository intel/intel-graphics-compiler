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


// bitselect

INLINE
double __builtin_spirv_OpenCL_bitselect_f64_f64_f64( double a,
                                              double b,
                                              double c )
{
    return as_double( __builtin_spirv_OpenCL_bitselect_i64_i64_i64(as_long(a), as_long(b), as_long(c)) );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_bitselect, double, double, f64 )

// isfinite

int OVERLOADABLE __intel_relaxed_isfinite( double x )
{
    int result = SPIRV_BUILTIN(IsFinite, _f64, )(x);
    // This could check for -cl-finite-math-only, not -cl-fast-relaxed-math.
    return __FastRelaxedMath ? 1 : result;
}

// isinf

INLINE int OVERLOADABLE __intel_relaxed_isinf( double x )
{
    int result = SPIRV_BUILTIN(IsInf, _f64, )(x);
    // This could check for -cl-finite-math-only, not -cl-fast-relaxed-math.
    return __FastRelaxedMath ? 0 : result;
}

// isnan

INLINE int OVERLOADABLE __intel_relaxed_isnan( double x )
{
    int result = SPIRV_BUILTIN(IsNan, _f64, )(x);
    // This could check for -cl-finite-math-only, not -cl-fast-relaxed-math.
    return __FastRelaxedMath ? 0 : result;
}

// select

INLINE double __builtin_spirv_OpenCL_select_f64_f64_i64( double a, double b, ulong c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE double __intel_vector_select_helper( double a, double b, ulong c )
{
    return as_long(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, double, ulong, f64, i64 )
