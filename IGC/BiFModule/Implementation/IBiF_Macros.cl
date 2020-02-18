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

// This file contains a set of macros that may be used for built-in functions.

#ifndef _IBIF_MACROS_
#define _IBIF_MACROS_

// Use this to grab a pointer to local memory whenever you
// are treating the local memory as automatic storage.
#define GET_MEMPOOL_PTR(_ptr, _type, _allocAllWorkgroups, _additionalElems) \
  __local _type* _ptr =                                                     \
    (__local _type*)__builtin_IB_AllocLocalMemPool(                         \
        _allocAllWorkgroups,                                                \
        _additionalElems,                                                   \
        sizeof(_type));

// Macro for async work copy implementation.
#define ASYNC_WORK_GROUP_COPY(dst, src, num_elements, evt, __num_elements_type)                  \
    {                                                                       \
        __num_elements_type uiNumElements = num_elements;                                  \
        __num_elements_type index = __spirv_BuiltInLocalInvocationIndex();            \
        __num_elements_type step = __spirv_WorkgroupSize();                               \
        for( ; index < uiNumElements; index += step ) {                     \
            dst[index] = src[index];                                        \
        }                                                                   \
    }

#define ASYNC_WORK_GROUP_STRIDED_COPY_G2L(dst, src, num_elements, src_stride, evt, __num_elements_type)  \
    {                                                                       \
        __num_elements_type uiNumElements = num_elements;                                  \
        __num_elements_type uiStride = src_stride;                                         \
        __num_elements_type dstIndex = __spirv_BuiltInLocalInvocationIndex();         \
        __num_elements_type dstStep = __spirv_WorkgroupSize();                            \
        __num_elements_type srcIndex = dstIndex * uiStride;                                \
        __num_elements_type srcStep = dstStep * uiStride;                                  \
        for( ; dstIndex < uiNumElements;                                    \
             dstIndex += dstStep, srcIndex += srcStep ) {                   \
            dst[dstIndex] = src[srcIndex];                                  \
        }                                                                   \
    }

#define ASYNC_WORK_GROUP_STRIDED_COPY_L2G(dst, src, num_elements, dst_stride, evt, __num_elements_type)  \
    {                                                                       \
        __num_elements_type uiNumElements = num_elements;                                  \
        __num_elements_type uiStride = dst_stride;                                         \
        __num_elements_type srcIndex = __spirv_BuiltInLocalInvocationIndex();         \
        __num_elements_type srcStep = __spirv_WorkgroupSize();                            \
        __num_elements_type dstIndex = srcIndex * uiStride;                                \
        __num_elements_type dstStep = srcStep * uiStride;                                  \
        for( ; srcIndex < uiNumElements;                                    \
             dstIndex += dstStep, srcIndex += srcStep ) {                   \
            dst[dstIndex] = src[srcIndex];                                  \
        }                                                                   \
    }


// Use this macro if the vector functions have different behavior than the
// scalar functions.

#define GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT_NO_MANG( __func, __sfunc, __rettype, __argtype ) \
    __rettype##2 OVERLOADABLE __func( __argtype##2 x ) {                    \
        return (__rettype##2)( __sfunc(x.s0),                               \
                               __sfunc(x.s1) );                             \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func( __argtype##3 x ) {                    \
        return (__rettype##3)( __sfunc(x.s0),                               \
                               __sfunc(x.s1),                               \
                               __sfunc(x.s2) );                             \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func( __argtype##4 x ) {                    \
        return (__rettype##4)( __sfunc(x.s0),                               \
                               __sfunc(x.s1),                               \
                               __sfunc(x.s2),                               \
                               __sfunc(x.s3) );                             \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func( __argtype##8 x ) {                    \
        return (__rettype##8)( __sfunc(x.s0),                               \
                               __sfunc(x.s1),                               \
                               __sfunc(x.s2),                               \
                               __sfunc(x.s3),                               \
                               __sfunc(x.s4),                               \
                               __sfunc(x.s5),                               \
                               __sfunc(x.s6),                               \
                               __sfunc(x.s7) );                             \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func( __argtype##16 x ) {                  \
        return (__rettype##16)( __sfunc(x.s0),                              \
                                __sfunc(x.s1),                              \
                                __sfunc(x.s2),                              \
                                __sfunc(x.s3),                              \
                                __sfunc(x.s4),                              \
                                __sfunc(x.s5),                              \
                                __sfunc(x.s6),                              \
                                __sfunc(x.s7),                              \
                                __sfunc(x.s8),                              \
                                __sfunc(x.s9),                              \
                                __sfunc(x.sa),                              \
                                __sfunc(x.sb),                              \
                                __sfunc(x.sc),                              \
                                __sfunc(x.sd),                              \
                                __sfunc(x.se),                              \
                                __sfunc(x.sf) );                            \
    }

// Use this macro if the vector functions have the same behavior as the
// scalar function.
#define GENERATE_VECTOR_FUNCTIONS_1ARG_NO_MANG( __func, __rettype, __argtype )      \
    GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT_NO_MANG( __func, __func, __rettype, __argtype )

// Use this macro if the vector functions have different behavior than the
// scalar functions.

#define GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( __func, __sfunc, __rettype, __argtype, __abbrargtype) \
    __rettype##2 __func##_v2##__abbrargtype( __argtype##2 x ) {                 \
        return (__rettype##2)( __sfunc##_##__abbrargtype(x.s0),                               \
                               __sfunc##_##__abbrargtype(x.s1) );                             \
    }                                                                                     \
    __rettype##3 __func##_v3##__abbrargtype( __argtype##3 x ) {                 \
        return (__rettype##3)( __sfunc##_##__abbrargtype(x.s0),                               \
                               __sfunc##_##__abbrargtype(x.s1),                               \
                               __sfunc##_##__abbrargtype(x.s2) );                             \
    }                                                                                     \
    __rettype##4 __func##_v4##__abbrargtype( __argtype##4 x ) {                 \
        return (__rettype##4)( __sfunc##_##__abbrargtype(x.s0),                               \
                               __sfunc##_##__abbrargtype(x.s1),                               \
                               __sfunc##_##__abbrargtype(x.s2),                               \
                               __sfunc##_##__abbrargtype(x.s3) );                             \
    }                                                                                     \
    __rettype##8 __func##_v8##__abbrargtype( __argtype##8 x ) {                 \
        return (__rettype##8)( __sfunc##_##__abbrargtype(x.s0),                               \
                               __sfunc##_##__abbrargtype(x.s1),                               \
                               __sfunc##_##__abbrargtype(x.s2),                               \
                               __sfunc##_##__abbrargtype(x.s3),                               \
                               __sfunc##_##__abbrargtype(x.s4),                               \
                               __sfunc##_##__abbrargtype(x.s5),                               \
                               __sfunc##_##__abbrargtype(x.s6),                               \
                               __sfunc##_##__abbrargtype(x.s7) );                             \
    }                                                                                     \
    __rettype##16 __func##_v16##__abbrargtype( __argtype##16 x ) {              \
        return (__rettype##16)( __sfunc##_##__abbrargtype(x.s0),                              \
                                __sfunc##_##__abbrargtype(x.s1),                              \
                                __sfunc##_##__abbrargtype(x.s2),                              \
                                __sfunc##_##__abbrargtype(x.s3),                              \
                                __sfunc##_##__abbrargtype(x.s4),                              \
                                __sfunc##_##__abbrargtype(x.s5),                              \
                                __sfunc##_##__abbrargtype(x.s6),                              \
                                __sfunc##_##__abbrargtype(x.s7),                              \
                                __sfunc##_##__abbrargtype(x.s8),                              \
                                __sfunc##_##__abbrargtype(x.s9),                              \
                                __sfunc##_##__abbrargtype(x.sa),                              \
                                __sfunc##_##__abbrargtype(x.sb),                              \
                                __sfunc##_##__abbrargtype(x.sc),                              \
                                __sfunc##_##__abbrargtype(x.sd),                              \
                                __sfunc##_##__abbrargtype(x.se),                              \
                                __sfunc##_##__abbrargtype(x.sf) );                            \
    }

// Use this macro if the vector functions have the same behavior as the
// scalar function.
#define GENERATE_VECTOR_FUNCTIONS_1ARG( __func, __rettype, __argtype, __abbrargtype )      \
    GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( __func, __func, __rettype, __argtype, __abbrargtype )

// Use this macro if the scalar function is big.
// TODO: get rid of this #else case when the legalizer can
// fix up illegal types that GVN generates.
#define VECTOARRAY2(a, v) \
    a[0] = v.s0; \
    a[1] = v.s1;

#define ARRAYTOVEC2(v, a) \
    v.s0 = a[0]; \
    v.s1 = a[1];

#define VECTOARRAY3(a, v) \
    a[0] = v.s0; \
    a[1] = v.s1; \
    a[2] = v.s2;

#define ARRAYTOVEC3(v, a) \
    v.s0 = a[0]; \
    v.s1 = a[1]; \
    v.s2 = a[2];

#define VECTOARRAY4(a, v) \
    a[0] = v.s0; \
    a[1] = v.s1; \
    a[2] = v.s2; \
    a[3] = v.s3;

#define ARRAYTOVEC4(v, a) \
    v.s0 = a[0]; \
    v.s1 = a[1]; \
    v.s2 = a[2]; \
    v.s3 = a[3];

#define VECTOARRAY8(a, v) \
    a[0] = v.s0; \
    a[1] = v.s1; \
    a[2] = v.s2; \
    a[3] = v.s3; \
    a[4] = v.s4; \
    a[5] = v.s5; \
    a[6] = v.s6; \
    a[7] = v.s7;

#define ARRAYTOVEC8(v, a) \
    v.s0 = a[0]; \
    v.s1 = a[1]; \
    v.s2 = a[2]; \
    v.s3 = a[3]; \
    v.s4 = a[4]; \
    v.s5 = a[5]; \
    v.s6 = a[6]; \
    v.s7 = a[7];

#define VECTOARRAY16(a, v) \
    a[0] = v.s0; \
    a[1] = v.s1; \
    a[2] = v.s2; \
    a[3] = v.s3; \
    a[4] = v.s4; \
    a[5] = v.s5; \
    a[6] = v.s6; \
    a[7] = v.s7; \
    a[8] = v.s8; \
    a[9] = v.s9; \
    a[0xa] = v.sa; \
    a[0xb] = v.sb; \
    a[0xc] = v.sc; \
    a[0xd] = v.sd; \
    a[0xe] = v.se; \
    a[0xf] = v.sf;

#define ARRAYTOVEC16(v, a) \
    v.s0 = a[0]; \
    v.s1 = a[1]; \
    v.s2 = a[2]; \
    v.s3 = a[3]; \
    v.s4 = a[4]; \
    v.s5 = a[5]; \
    v.s6 = a[6]; \
    v.s7 = a[7]; \
    v.s8 = a[8]; \
    v.s9 = a[9]; \
    v.sa = a[0xa]; \
    v.sb = a[0xb]; \
    v.sc = a[0xc]; \
    v.sd = a[0xd]; \
    v.se = a[0xe]; \
    v.sf = a[0xf];

#define GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP_SIZE( __func, __rettype, __argtype, __vecSize, __abbrargtype ) \
    __rettype##__vecSize __func##_v##__vecSize##__abbrargtype( __argtype##__vecSize x ) {       \
        __rettype##__vecSize ret;                                                           \
        __argtype argx[__vecSize];                                                          \
        __rettype out[__vecSize];                                                           \
        VECTOARRAY##__vecSize(argx, x);                                                     \
        for(uint i = 0; i < __vecSize; i++) {                                               \
            out[i] = __func##_##__abbrargtype(argx[i]);                                     \
        }                                                                                   \
        ARRAYTOVEC##__vecSize(ret, out);                                                    \
        return ret;                                                                         \
    }

// Use this macro if the scalar function is big.
#define GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __func, __rettype, __argtype, __abbrargtype )     \
    GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP_SIZE( __func, __rettype, __argtype, 2, __abbrargtype ) \
    GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP_SIZE( __func, __rettype, __argtype, 3, __abbrargtype ) \
    GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP_SIZE( __func, __rettype, __argtype, 4, __abbrargtype ) \
    GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP_SIZE( __func, __rettype, __argtype, 8, __abbrargtype ) \
    GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP_SIZE( __func, __rettype, __argtype, 16, __abbrargtype )


// TODO: get rid of this #else case when the legalizer can
// fix up illegal types that GVN generates.
#define GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP_SIZE( __func, __rettype, __argtype, __ptrtype, __vecSize, __abbrargtype, __abbrptrtype ) \
    __rettype##__vecSize __func##_v##__vecSize##__abbrargtype##_p0v##__vecSize##__abbrptrtype( __argtype##__vecSize x, __private __ptrtype##__vecSize * y ) {    \
        __rettype##__vecSize ret;                                                                               \
        __argtype argx[__vecSize];                                                                              \
        __rettype out[__vecSize];                                                                               \
        __private __ptrtype* py_scalar = (__private __ptrtype*)y;                                               \
        VECTOARRAY##__vecSize(argx, x);                                                                         \
        for(uint i = 0; i < __vecSize; i++)                                                                     \
        {                                                                                                       \
            out[i] = __func##_##__abbrargtype##_p0##__abbrptrtype(argx[i], py_scalar + i);                      \
        }                                                                                                       \
        ARRAYTOVEC##__vecSize(ret, out);                                                                        \
        return ret;                                                                                             \
    }

#define GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP( __func, __rettype, __argtype, __ptrtype, __abbrargtype, __abbrptrtype )      \
    GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP_SIZE( __func, __rettype, __argtype, __ptrtype, 2, __abbrargtype, __abbrptrtype )  \
    GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP_SIZE( __func, __rettype, __argtype, __ptrtype, 3, __abbrargtype, __abbrptrtype )  \
    GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP_SIZE( __func, __rettype, __argtype, __ptrtype, 4, __abbrargtype, __abbrptrtype )  \
    GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP_SIZE( __func, __rettype, __argtype, __ptrtype, 8, __abbrargtype, __abbrptrtype )  \
    GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP_SIZE( __func, __rettype, __argtype, __ptrtype, 16, __abbrargtype, __abbrptrtype )

#define GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG_EXPLICIT( __func, __sfunc, __rettype, __argtype, __addressspc, __ptrtype, __abbrargtype, __abbrptrtype, __abbraddressspc ) \
    __rettype##2 __func##_v2##__abbrargtype##_##__abbraddressspc##v2##__abbrptrtype( __argtype##2 x, __addressspc __ptrtype##2 * y ) {                          \
        __ptrtype##2 a, b;                                                                                                   \
        a = __sfunc##_v2##__abbrargtype##_p0v2##__abbrptrtype(x, &b);                                            \
        y[0] = b;                                                                                                            \
        return a;                                                                                                            \
    }                                                                                                                        \
    __rettype##3 __func##_v3##__abbrargtype##_##__abbraddressspc##v3##__abbrptrtype( __argtype##3 x, __addressspc __ptrtype##3 * y ) {                                      \
        __ptrtype##3 a, b;                                                                                                   \
        a = __sfunc##_v3##__abbrargtype##_p0v3##__abbrptrtype(x, &b);                                            \
        y[0] = b;                                                                                                            \
        return a;                                                                                                            \
    }                                                                                                                        \
    __rettype##4 __func##_v4##__abbrargtype##_##__abbraddressspc##v4##__abbrptrtype( __argtype##4 x, __addressspc __ptrtype##4 * y ) {                                      \
        __ptrtype##4 a, b;                                                                                                   \
        a = __sfunc##_v4##__abbrargtype##_p0v4##__abbrptrtype(x, &b);                                            \
        y[0] = b;                                                                                                            \
        return a;                                                                                                            \
    }                                                                                                                        \
    __rettype##8 __func##_v8##__abbrargtype##_##__abbraddressspc##v8##__abbrptrtype( __argtype##8 x, __addressspc __ptrtype##8 * y ) {                                      \
        __ptrtype##8 a, b;                                                                                                   \
        a = __sfunc##_v8##__abbrargtype##_p0v8##__abbrptrtype(x, &b);                                            \
        y[0] = b;                                                                                                            \
        return a;                                                                                                            \
    }                                                                                                                        \
    __rettype##16 __func##_v16##__abbrargtype##_##__abbraddressspc##v16##__abbrptrtype( __argtype##16 x, __addressspc __ptrtype##16 * y ) {                                 \
        __ptrtype##16 a, b;                                                                                                  \
        a = __sfunc##_v16##__abbrargtype##_p0v16##__abbrptrtype(x, &b);                                            \
        y[0] = b;                                                                                                            \
        return a;                                                                                                            \
    }


# define GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __func, __rettype, __addressspc, __argptrtype, __abbrargtype, __abbraddressspc ) \
    GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG_EXPLICIT( __func, __func, __rettype, __argptrtype, __addressspc, __argptrtype, __abbrargtype, __abbrargtype, __abbraddressspc )

#define GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( __func, __sfunc, __rettype, __argtype, __abbrargtype )         \
    __rettype##2 __func##_v2##__abbrargtype##_v2##__abbrargtype( __argtype##2 x, __argtype##2 y ) {        \
        return (__rettype##2)( __sfunc##_##__abbrargtype##_##__abbrargtype(x.s0, y.s0),                       \
                               __sfunc##_##__abbrargtype##_##__abbrargtype(x.s1, y.s1) );                           \
    }                                                                                               \
    __rettype##3 __func##_v3##__abbrargtype##_v3##__abbrargtype( __argtype##3 x, __argtype##3 y ) {        \
        return (__rettype##3)( __sfunc##_##__abbrargtype##_##__abbrargtype(x.s0, y.s0),                             \
                               __sfunc##_##__abbrargtype##_##__abbrargtype(x.s1, y.s1),                             \
                               __sfunc##_##__abbrargtype##_##__abbrargtype(x.s2, y.s2) );                           \
    }                                                                                               \
    __rettype##4 __func##_v4##__abbrargtype##_v4##__abbrargtype( __argtype##4 x, __argtype##4 y ) {        \
        return (__rettype##4)( __sfunc##_##__abbrargtype##_##__abbrargtype(x.s0, y.s0),                             \
                               __sfunc##_##__abbrargtype##_##__abbrargtype(x.s1, y.s1),                             \
                               __sfunc##_##__abbrargtype##_##__abbrargtype(x.s2, y.s2),                             \
                               __sfunc##_##__abbrargtype##_##__abbrargtype(x.s3, y.s3) );                           \
    }                                                                                               \
    __rettype##8 __func##_v8##__abbrargtype##_v8##__abbrargtype( __argtype##8 x, __argtype##8 y ) {        \
        return (__rettype##8)( __sfunc##_##__abbrargtype##_##__abbrargtype(x.s0, y.s0),                             \
                               __sfunc##_##__abbrargtype##_##__abbrargtype(x.s1, y.s1),                             \
                               __sfunc##_##__abbrargtype##_##__abbrargtype(x.s2, y.s2),                             \
                               __sfunc##_##__abbrargtype##_##__abbrargtype(x.s3, y.s3),                             \
                               __sfunc##_##__abbrargtype##_##__abbrargtype(x.s4, y.s4),                             \
                               __sfunc##_##__abbrargtype##_##__abbrargtype(x.s5, y.s5),                             \
                               __sfunc##_##__abbrargtype##_##__abbrargtype(x.s6, y.s6),                             \
                               __sfunc##_##__abbrargtype##_##__abbrargtype(x.s7, y.s7) );                           \
    }                                                                                               \
    __rettype##16 __func##_v16##__abbrargtype##_v16##__abbrargtype( __argtype##16 x, __argtype##16 y ) {     \
        return (__rettype##16)( __sfunc##_##__abbrargtype##_##__abbrargtype(x.s0, y.s0),                            \
                                __sfunc##_##__abbrargtype##_##__abbrargtype(x.s1, y.s1),                            \
                                __sfunc##_##__abbrargtype##_##__abbrargtype(x.s2, y.s2),                            \
                                __sfunc##_##__abbrargtype##_##__abbrargtype(x.s3, y.s3),                            \
                                __sfunc##_##__abbrargtype##_##__abbrargtype(x.s4, y.s4),                            \
                                __sfunc##_##__abbrargtype##_##__abbrargtype(x.s5, y.s5),                            \
                                __sfunc##_##__abbrargtype##_##__abbrargtype(x.s6, y.s6),                            \
                                __sfunc##_##__abbrargtype##_##__abbrargtype(x.s7, y.s7),                            \
                                __sfunc##_##__abbrargtype##_##__abbrargtype(x.s8, y.s8),                            \
                                __sfunc##_##__abbrargtype##_##__abbrargtype(x.s9, y.s9),                            \
                                __sfunc##_##__abbrargtype##_##__abbrargtype(x.sa, y.sa),                            \
                                __sfunc##_##__abbrargtype##_##__abbrargtype(x.sb, y.sb),                            \
                                __sfunc##_##__abbrargtype##_##__abbrargtype(x.sc, y.sc),                            \
                                __sfunc##_##__abbrargtype##_##__abbrargtype(x.sd, y.sd),                            \
                                __sfunc##_##__abbrargtype##_##__abbrargtype(x.se, y.se),                            \
                                __sfunc##_##__abbrargtype##_##__abbrargtype(x.sf, y.sf) );                          \
    }

#define GENERATE_VECTOR_FUNCTIONS_2ARGS( __func, __rettype, __argtype, __abbrargtype )     \
    GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( __func, __func, __rettype, __argtype, __abbrargtype )

#define GENERATE_VECTOR_FUNCTIONS_2ARGS_VV( __func, __rettype, __argtype0, __argtype1, __abbrargtype0, __abbrargtype1 ) \
    __rettype##2 __func##_v2##__abbrargtype0##_v2##__abbrargtype1( __argtype0##2 x, __argtype1##2 y ) {      \
        return (__rettype##2)( __func##_##__abbrargtype0##_##__abbrargtype1(x.s0, y.s0),                              \
                               __func##_##__abbrargtype0##_##__abbrargtype1(x.s1, y.s1) );                            \
    }                                                                                               \
    __rettype##3 __func##_v3##__abbrargtype0##_v3##__abbrargtype1( __argtype0##3 x, __argtype1##3 y ) {      \
        return (__rettype##3)( __func##_##__abbrargtype0##_##__abbrargtype1(x.s0, y.s0),                              \
                               __func##_##__abbrargtype0##_##__abbrargtype1(x.s1, y.s1),                              \
                               __func##_##__abbrargtype0##_##__abbrargtype1(x.s2, y.s2) );                            \
    }                                                                                               \
    __rettype##4 __func##_v4##__abbrargtype0##_v4##__abbrargtype1( __argtype0##4 x, __argtype1##4 y ) {      \
        return (__rettype##4)( __func##_##__abbrargtype0##_##__abbrargtype1(x.s0, y.s0),                              \
                               __func##_##__abbrargtype0##_##__abbrargtype1(x.s1, y.s1),                              \
                               __func##_##__abbrargtype0##_##__abbrargtype1(x.s2, y.s2),                              \
                               __func##_##__abbrargtype0##_##__abbrargtype1(x.s3, y.s3) );                            \
    }                                                                                               \
    __rettype##8 __func##_v8##__abbrargtype0##_v8##__abbrargtype1( __argtype0##8 x, __argtype1##8 y ) {      \
        return (__rettype##8)( __func##_##__abbrargtype0##_##__abbrargtype1(x.s0, y.s0),                              \
                               __func##_##__abbrargtype0##_##__abbrargtype1(x.s1, y.s1),                              \
                               __func##_##__abbrargtype0##_##__abbrargtype1(x.s2, y.s2),                              \
                               __func##_##__abbrargtype0##_##__abbrargtype1(x.s3, y.s3),                              \
                               __func##_##__abbrargtype0##_##__abbrargtype1(x.s4, y.s4),                              \
                               __func##_##__abbrargtype0##_##__abbrargtype1(x.s5, y.s5),                              \
                               __func##_##__abbrargtype0##_##__abbrargtype1(x.s6, y.s6),                              \
                               __func##_##__abbrargtype0##_##__abbrargtype1(x.s7, y.s7) );                            \
    }                                                                                               \
    __rettype##16 __func##_v16##__abbrargtype0##_v16##__abbrargtype1( __argtype0##16 x, __argtype1##16 y ) { \
        return (__rettype##16)( __func##_##__abbrargtype0##_##__abbrargtype1(x.s0, y.s0),                             \
                                __func##_##__abbrargtype0##_##__abbrargtype1(x.s1, y.s1),                             \
                                __func##_##__abbrargtype0##_##__abbrargtype1(x.s2, y.s2),                             \
                                __func##_##__abbrargtype0##_##__abbrargtype1(x.s3, y.s3),                             \
                                __func##_##__abbrargtype0##_##__abbrargtype1(x.s4, y.s4),                             \
                                __func##_##__abbrargtype0##_##__abbrargtype1(x.s5, y.s5),                             \
                                __func##_##__abbrargtype0##_##__abbrargtype1(x.s6, y.s6),                             \
                                __func##_##__abbrargtype0##_##__abbrargtype1(x.s7, y.s7),                             \
                                __func##_##__abbrargtype0##_##__abbrargtype1(x.s8, y.s8),                             \
                                __func##_##__abbrargtype0##_##__abbrargtype1(x.s9, y.s9),                             \
                                __func##_##__abbrargtype0##_##__abbrargtype1(x.sa, y.sa),                             \
                                __func##_##__abbrargtype0##_##__abbrargtype1(x.sb, y.sb),                             \
                                __func##_##__abbrargtype0##_##__abbrargtype1(x.sc, y.sc),                             \
                                __func##_##__abbrargtype0##_##__abbrargtype1(x.sd, y.sd),                             \
                                __func##_##__abbrargtype0##_##__abbrargtype1(x.se, y.se),                             \
                                __func##_##__abbrargtype0##_##__abbrargtype1(x.sf, y.sf) );                           \
    }

// TODO: get rid of this #else case when the legalizer can
// fix up illegal types that GVN generates.
#define GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP_SIZE( __func, __rettype, __argtype0, __argtype1, __vecSize, __abbrargtype0, __abbrargtype1 ) \
    __rettype##__vecSize __func##_v##__vecSize##__abbrargtype0##_v##__vecSize##__abbrargtype1( __argtype0##__vecSize x, __argtype1##__vecSize y ) {  \
        __rettype##__vecSize ret;                                                                            \
        __argtype0 argx[__vecSize];                                                                          \
        __argtype1 argy[__vecSize];                                                                          \
        __rettype  out[__vecSize];                                                                           \
        VECTOARRAY##__vecSize(argx, x);                                                                      \
        VECTOARRAY##__vecSize(argy, y);                                                                      \
        for(uint i = 0; i < __vecSize; i++) {                                                                \
            out[i] = __func##_##__abbrargtype0##_##__abbrargtype1(argx[i], argy[i]);                         \
        }                                                                                                    \
        ARRAYTOVEC##__vecSize(ret, out);                                                                     \
        return ret;                                                                                          \
    }

#define GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __func, __rettype, __argtype0, __argtype1, __abbrargtype0, __abbrargtype1 )    \
    GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP_SIZE( __func, __rettype, __argtype0, __argtype1, 2, __abbrargtype0, __abbrargtype1 ) \
    GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP_SIZE( __func, __rettype, __argtype0, __argtype1, 3, __abbrargtype0, __abbrargtype1 ) \
    GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP_SIZE( __func, __rettype, __argtype0, __argtype1, 4, __abbrargtype0, __abbrargtype1 ) \
    GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP_SIZE( __func, __rettype, __argtype0, __argtype1, 8, __abbrargtype0, __abbrargtype1 ) \
    GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP_SIZE( __func, __rettype, __argtype0, __argtype1, 16, __abbrargtype0, __abbrargtype1 )


#define GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( __func, __rettype, __vargtype, __sargtype, __abbrvargtype, __abbrsargtype ) \
    __rettype##2 __func##_v2##__abbrvargtype##_##__abbrsargtype( __vargtype##2 x, __sargtype y ) {   \
        return (__rettype##2)( __func##_##__abbrvargtype##_##__abbrsargtype(x.s0, y),                             \
                               __func##_##__abbrvargtype##_##__abbrsargtype(x.s1, y) );                           \
    }                                                                                             \
    __rettype##3 __func##_v3##__abbrvargtype##_##__abbrsargtype( __vargtype##3 x, __sargtype y ) {   \
        return (__rettype##3)( __func##_##__abbrvargtype##_##__abbrsargtype(x.s0, y),                             \
                               __func##_##__abbrvargtype##_##__abbrsargtype(x.s1, y),                             \
                               __func##_##__abbrvargtype##_##__abbrsargtype(x.s2, y) );                           \
    }                                                                                             \
    __rettype##4 __func##_v4##__abbrvargtype##_##__abbrsargtype( __vargtype##4 x, __sargtype y ) {   \
        return (__rettype##4)( __func##_##__abbrvargtype##_##__abbrsargtype(x.s0, y),                             \
                               __func##_##__abbrvargtype##_##__abbrsargtype(x.s1, y),                             \
                               __func##_##__abbrvargtype##_##__abbrsargtype(x.s2, y),                             \
                               __func##_##__abbrvargtype##_##__abbrsargtype(x.s3, y) );                           \
    }                                                                                             \
    __rettype##8 __func##_v8##__abbrvargtype##_##__abbrsargtype( __vargtype##8 x, __sargtype y ) {   \
        return (__rettype##8)( __func##_##__abbrvargtype##_##__abbrsargtype(x.s0, y),                             \
                               __func##_##__abbrvargtype##_##__abbrsargtype(x.s1, y),                             \
                               __func##_##__abbrvargtype##_##__abbrsargtype(x.s2, y),                             \
                               __func##_##__abbrvargtype##_##__abbrsargtype(x.s3, y),                             \
                               __func##_##__abbrvargtype##_##__abbrsargtype(x.s4, y),                             \
                               __func##_##__abbrvargtype##_##__abbrsargtype(x.s5, y),                             \
                               __func##_##__abbrvargtype##_##__abbrsargtype(x.s6, y),                             \
                               __func##_##__abbrvargtype##_##__abbrsargtype(x.s7, y) );                           \
    }                                                                                             \
    __rettype##16 __func##_v16##__abbrvargtype##_##__abbrsargtype( __vargtype##16 x, __sargtype y ) {\
        return (__rettype##16)( __func##_##__abbrvargtype##_##__abbrsargtype(x.s0, y),                            \
                                __func##_##__abbrvargtype##_##__abbrsargtype(x.s1, y),                            \
                                __func##_##__abbrvargtype##_##__abbrsargtype(x.s2, y),                            \
                                __func##_##__abbrvargtype##_##__abbrsargtype(x.s3, y),                            \
                                __func##_##__abbrvargtype##_##__abbrsargtype(x.s4, y),                            \
                                __func##_##__abbrvargtype##_##__abbrsargtype(x.s5, y),                            \
                                __func##_##__abbrvargtype##_##__abbrsargtype(x.s6, y),                            \
                                __func##_##__abbrvargtype##_##__abbrsargtype(x.s7, y),                            \
                                __func##_##__abbrvargtype##_##__abbrsargtype(x.s8, y),                            \
                                __func##_##__abbrvargtype##_##__abbrsargtype(x.s9, y),                            \
                                __func##_##__abbrvargtype##_##__abbrsargtype(x.sa, y),                            \
                                __func##_##__abbrvargtype##_##__abbrsargtype(x.sb, y),                            \
                                __func##_##__abbrvargtype##_##__abbrsargtype(x.sc, y),                            \
                                __func##_##__abbrvargtype##_##__abbrsargtype(x.sd, y),                            \
                                __func##_##__abbrvargtype##_##__abbrsargtype(x.se, y),                            \
                                __func##_##__abbrvargtype##_##__abbrsargtype(x.sf, y) );                          \
    }

#define GENERATE_VECTOR_FUNCTIONS_2ARGS_SV( __func, __rettype, __sargtype, __vargtype, __abbrsargtype, __abbrvargtype ) \
    __rettype##2 __func##_##__abbrsargtype##_v2##__abbrvargtype( __sargtype x, __vargtype##2 y ) {   \
        return (__rettype##2)( __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s0),                             \
                               __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s1) );                           \
    }                                                                                             \
    __rettype##3 __func##_##__abbrsargtype##_v3##__abbrvargtype( __sargtype x, __vargtype##3 y ) {   \
        return (__rettype##3)( __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s0),                             \
                               __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s1),                             \
                               __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s2) );                           \
    }                                                                                             \
    __rettype##4 __func##_##__abbrsargtype##_v4##__abbrvargtype( __sargtype x, __vargtype##4 y ) {   \
        return (__rettype##4)( __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s0),                             \
                               __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s1),                             \
                               __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s2),                             \
                               __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s3) );                           \
    }                                                                                             \
    __rettype##8 __func##_##__abbrsargtype##_v8##__abbrvargtype( __sargtype x, __vargtype##8 y ) {   \
        return (__rettype##8)( __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s0),                             \
                               __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s1),                             \
                               __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s2),                             \
                               __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s3),                             \
                               __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s4),                             \
                               __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s5),                             \
                               __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s6),                             \
                               __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s7) );                           \
    }                                                                                             \
    __rettype##16 __func##_##__abbrsargtype##_v16##__abbrvargtype( __sargtype x, __vargtype##16 y ) {\
        return (__rettype##16)( __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s0),                            \
                                __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s1),                            \
                                __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s2),                            \
                                __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s3),                            \
                                __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s4),                            \
                                __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s5),                            \
                                __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s6),                            \
                                __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s7),                            \
                                __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s8),                            \
                                __func##_##__abbrsargtype##_##__abbrvargtype(x, y.s9),                            \
                                __func##_##__abbrsargtype##_##__abbrvargtype(x, y.sa),                            \
                                __func##_##__abbrsargtype##_##__abbrvargtype(x, y.sb),                            \
                                __func##_##__abbrsargtype##_##__abbrvargtype(x, y.sc),                            \
                                __func##_##__abbrsargtype##_##__abbrvargtype(x, y.sd),                            \
                                __func##_##__abbrsargtype##_##__abbrvargtype(x, y.se),                            \
                                __func##_##__abbrsargtype##_##__abbrvargtype(x, y.sf) );                          \
    }

#define GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __sfunc, __rettype, __argtype, __abbrrettype, __abbrargtype ) \
    __rettype##2 __builtin_spirv_OpenCL_select##_v2##__abbrrettype##_v2##__abbrrettype##_v2##__abbrargtype( __rettype##2 x, __rettype##2 y, __argtype##2 z ) {             \
        return (__rettype##2)( __sfunc(x.s0, y.s0, z.s0),                   \
                               __sfunc(x.s1, y.s1, z.s1) );                 \
    }                                                                                                     \
    __rettype##3 __builtin_spirv_OpenCL_select##_v3##__abbrrettype##_v3##__abbrrettype##_v3##__abbrargtype( __rettype##3 x, __rettype##3 y, __argtype##3 z ) {              \
        return (__rettype##3)( __sfunc(x.s0, y.s0, z.s0),                   \
                               __sfunc(x.s1, y.s1, z.s1),                   \
                               __sfunc(x.s2, y.s2, z.s2) );                 \
    }                                                                                                         \
    __rettype##4 __builtin_spirv_OpenCL_select##_v4##__abbrrettype##_v4##__abbrrettype##_v4##__abbrargtype( __rettype##4 x, __rettype##4 y, __argtype##4 z ) {              \
        return (__rettype##4)( __sfunc(x.s0, y.s0, z.s0),                   \
                               __sfunc(x.s1, y.s1, z.s1),                   \
                               __sfunc(x.s2, y.s2, z.s2),                   \
                               __sfunc(x.s3, y.s3, z.s3) );                 \
    }                                                                                                         \
    __rettype##8 __builtin_spirv_OpenCL_select##_v8##__abbrrettype##_v8##__abbrrettype##_v8##__abbrargtype( __rettype##8 x, __rettype##8 y, __argtype##8 z ) {              \
        return (__rettype##8)( __sfunc(x.s0, y.s0, z.s0),                   \
                               __sfunc(x.s1, y.s1, z.s1),                   \
                               __sfunc(x.s2, y.s2, z.s2),                   \
                               __sfunc(x.s3, y.s3, z.s3),                   \
                               __sfunc(x.s4, y.s4, z.s4),                   \
                               __sfunc(x.s5, y.s5, z.s5),                   \
                               __sfunc(x.s6, y.s6, z.s6),                   \
                               __sfunc(x.s7, y.s7, z.s7) );                 \
    }                                                                                                     \
    __rettype##16 __builtin_spirv_OpenCL_select##_v16##__abbrrettype##_v16##__abbrrettype##_v16##__abbrargtype( __rettype##16 x, __rettype##16 y, __argtype##16 z ) {      \
        return (__rettype##16)( __sfunc(x.s0, y.s0, z.s0),                  \
                                __sfunc(x.s1, y.s1, z.s1),                  \
                                __sfunc(x.s2, y.s2, z.s2),                  \
                                __sfunc(x.s3, y.s3, z.s3),                  \
                                __sfunc(x.s4, y.s4, z.s4),                  \
                                __sfunc(x.s5, y.s5, z.s5),                  \
                                __sfunc(x.s6, y.s6, z.s6),                  \
                                __sfunc(x.s7, y.s7, z.s7),                  \
                                __sfunc(x.s8, y.s8, z.s8),                  \
                                __sfunc(x.s9, y.s9, z.s9),                  \
                                __sfunc(x.sa, y.sa, z.sa),                  \
                                __sfunc(x.sb, y.sb, z.sb),                  \
                                __sfunc(x.sc, y.sc, z.sc),                  \
                                __sfunc(x.sd, y.sd, z.sd),                  \
                                __sfunc(x.se, y.se, z.se),                  \
                                __sfunc(x.sf, y.sf, z.sf) );                \
    }

#define GENERATE_VECTOR_FUNCTIONS_3ARGS_EXPLICIT( __func, __sfunc, __rettype, __argtype, __abbrargtype ) \
    __rettype##2 __func##_v2##__abbrargtype##_v2##__abbrargtype##_v2##__abbrargtype( __argtype##2 x, __argtype##2 y, __argtype##2 z ) { \
        return (__rettype##2)( __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s0, y.s0, z.s0),                   \
                               __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s1, y.s1, z.s1) );                 \
    }                                                                       \
    __rettype##3 __func##_v3##__abbrargtype##_v3##__abbrargtype##_v3##__abbrargtype( __argtype##3 x, __argtype##3 y, __argtype##3 z ) { \
        return (__rettype##3)( __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s0, y.s0, z.s0),                   \
                               __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s1, y.s1, z.s1),                   \
                               __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s2, y.s2, z.s2) );                 \
    }                                                                       \
    __rettype##4 __func##_v4##__abbrargtype##_v4##__abbrargtype##_v4##__abbrargtype( __argtype##4 x, __argtype##4 y, __argtype##4 z ) { \
        return (__rettype##4)( __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s0, y.s0, z.s0),                   \
                               __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s1, y.s1, z.s1),                   \
                               __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s2, y.s2, z.s2),                   \
                               __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s3, y.s3, z.s3) );                 \
    }                                                                       \
    __rettype##8 __func##_v8##__abbrargtype##_v8##__abbrargtype##_v8##__abbrargtype( __argtype##8 x, __argtype##8 y, __argtype##8 z ) { \
        return (__rettype##8)( __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s0, y.s0, z.s0),                   \
                               __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s1, y.s1, z.s1),                   \
                               __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s2, y.s2, z.s2),                   \
                               __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s3, y.s3, z.s3),                   \
                               __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s4, y.s4, z.s4),                   \
                               __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s5, y.s5, z.s5),                   \
                               __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s6, y.s6, z.s6),                   \
                               __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s7, y.s7, z.s7) );                 \
    }                                                                       \
    __rettype##16 __func##_v16##__abbrargtype##_v16##__abbrargtype##_v16##__abbrargtype( __argtype##16 x, __argtype##16 y, __argtype##16 z ) { \
        return (__rettype##16)( __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s0, y.s0, z.s0),                  \
                                __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s1, y.s1, z.s1),                  \
                                __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s2, y.s2, z.s2),                  \
                                __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s3, y.s3, z.s3),                  \
                                __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s4, y.s4, z.s4),                  \
                                __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s5, y.s5, z.s5),                  \
                                __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s6, y.s6, z.s6),                  \
                                __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s7, y.s7, z.s7),                  \
                                __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s8, y.s8, z.s8),                  \
                                __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.s9, y.s9, z.s9),                  \
                                __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.sa, y.sa, z.sa),                  \
                                __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.sb, y.sb, z.sb),                  \
                                __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.sc, y.sc, z.sc),                  \
                                __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.sd, y.sd, z.sd),                  \
                                __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.se, y.se, z.se),                  \
                                __sfunc##_##__abbrargtype##_##__abbrargtype##_##__abbrargtype(x.sf, y.sf, z.sf) );                \
    }


#define GENERATE_VECTOR_FUNCTIONS_3ARGS( __func, __rettype, __argtype, __abbrargtype )     \
    GENERATE_VECTOR_FUNCTIONS_3ARGS_EXPLICIT( __func, __func, __rettype, __argtype, __abbrargtype )

#define GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( __func, __rettype, __vargtype, __sargtype, __abbrvargtype, __abbrsargtype ) \
    __rettype##2 __func##_v2##__abbrvargtype##_v2##__abbrvargtype##_##__abbrsargtype( __vargtype##2 x, __vargtype##2 y, __sargtype z ) { \
        return (__rettype##2)( __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s0, y.s0, z),                       \
                               __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s1, y.s1, z) );                     \
    }                                                                                                        \
    __rettype##3 __func##_v3##__abbrvargtype##_v3##__abbrvargtype##_##__abbrsargtype( __vargtype##3 x, __vargtype##3 y, __sargtype z ) { \
        return (__rettype##3)( __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s0, y.s0, z),                       \
                               __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s1, y.s1, z),                       \
                               __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s2, y.s2, z) );                     \
    }                                                                                                        \
    __rettype##4 __func##_v4##__abbrvargtype##_v4##__abbrvargtype##_##__abbrsargtype( __vargtype##4 x, __vargtype##4 y, __sargtype z ) { \
        return (__rettype##4)( __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s0, y.s0, z),                       \
                               __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s1, y.s1, z),                       \
                               __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s2, y.s2, z),                       \
                               __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s3, y.s3, z) );                     \
    }                                                                                                        \
    __rettype##8 __func##_v8##__abbrvargtype##_v8##__abbrvargtype##_##__abbrsargtype( __vargtype##8 x, __vargtype##8 y, __sargtype z ) { \
        return (__rettype##8)( __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s0, y.s0, z),                       \
                               __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s1, y.s1, z),                       \
                               __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s2, y.s2, z),                       \
                               __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s3, y.s3, z),                       \
                               __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s4, y.s4, z),                       \
                               __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s5, y.s5, z),                       \
                               __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s6, y.s6, z),                       \
                               __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s7, y.s7, z) );                     \
    }                                                                                                        \
    __rettype##16 __func##_v16##__abbrvargtype##_v16##__abbrvargtype##_##__abbrsargtype( __vargtype##16 x, __vargtype##16 y, __sargtype z ) { \
        return (__rettype##16)( __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s0, y.s0, z),                      \
                                __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s1, y.s1, z),                      \
                                __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s2, y.s2, z),                      \
                                __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s3, y.s3, z),                      \
                                __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s4, y.s4, z),                      \
                                __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s5, y.s5, z),                      \
                                __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s6, y.s6, z),                      \
                                __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s7, y.s7, z),                      \
                                __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s8, y.s8, z),                      \
                                __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.s9, y.s9, z),                      \
                                __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.sa, y.sa, z),                      \
                                __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.sb, y.sb, z),                      \
                                __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.sc, y.sc, z),                      \
                                __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.sd, y.sd, z),                      \
                                __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.se, y.se, z),                      \
                                __func##_##__abbrvargtype##_##__abbrvargtype##_##__abbrsargtype(x.sf, y.sf, z) );                    \
    }

#define GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS( __func, __rettype, __vargtype, __sargtype, __abbrvargtype, __abbrsargtype ) \
    __rettype##2 __func##_v2##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype( __vargtype##2 x, __sargtype y, __sargtype z ) { \
        return (__rettype##2)( __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s0, y, z),                          \
                               __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s1, y, z) );                        \
    }                                                                                                        \
    __rettype##3 __func##_v3##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype( __vargtype##3 x, __sargtype y, __sargtype z ) { \
        return (__rettype##3)( __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s0, y, z),                          \
                               __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s1, y, z),                          \
                               __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s2, y, z) );                        \
    }                                                                                                        \
    __rettype##4 __func##_v4##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype( __vargtype##4 x, __sargtype y, __sargtype z ) { \
        return (__rettype##4)( __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s0, y, z),                          \
                               __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s1, y, z),                          \
                               __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s2, y, z),                          \
                               __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s3, y, z) );                        \
    }                                                                                                        \
    __rettype##8 __func##_v8##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype( __vargtype##8 x, __sargtype y, __sargtype z ) { \
        return (__rettype##8)( __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s0, y, z),                          \
                               __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s1, y, z),                          \
                               __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s2, y, z),                          \
                               __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s3, y, z),                          \
                               __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s4, y, z),                          \
                               __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s5, y, z),                          \
                               __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s6, y, z),                          \
                               __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s7, y, z) );                        \
    }                                                                                                        \
    __rettype##16 __func##_v16##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype( __vargtype##16 x, __sargtype y, __sargtype z ) { \
        return (__rettype##16)( __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s0, y, z),                         \
                                __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s1, y, z),                         \
                                __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s2, y, z),                         \
                                __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s3, y, z),                         \
                                __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s4, y, z),                         \
                                __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s5, y, z),                         \
                                __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s6, y, z),                         \
                                __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s7, y, z),                         \
                                __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s8, y, z),                         \
                                __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.s9, y, z),                         \
                                __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.sa, y, z),                         \
                                __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.sb, y, z),                         \
                                __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.sc, y, z),                         \
                                __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.sd, y, z),                         \
                                __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.se, y, z),                         \
                                __func##_##__abbrvargtype##_##__abbrsargtype##_##__abbrsargtype(x.sf, y, z) );                       \
    }

#define GENERATE_VECTOR_FUNCTIONS_3ARGS_SSV( __func, __rettype, __sargtype, __vargtype, __abbrsargtype, __abbrvargtype ) \
    __rettype##2 __func##_##__abbrsargtype##_##__abbrsargtype##_v2##__abbrvargtype( __sargtype x, __sargtype y, __vargtype##2 z ) { \
        return (__rettype##2)( __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s0),                          \
                               __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s1) );                        \
    }                                                                                                        \
    __rettype##3 __func##_##__abbrsargtype##_##__abbrsargtype##_v3##__abbrvargtype( __sargtype x, __sargtype y, __vargtype##3 z ) { \
        return (__rettype##3)( __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s0),                          \
                               __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s1),                          \
                               __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s2) );                        \
    }                                                                                                        \
    __rettype##4 __func##_##__abbrsargtype##_##__abbrsargtype##_v4##__abbrvargtype( __sargtype x, __sargtype y, __vargtype##4 z ) { \
        return (__rettype##4)( __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s0),                          \
                               __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s1),                          \
                               __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s2),                          \
                               __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s3) );                        \
    }                                                                                                        \
    __rettype##8 __func##_##__abbrsargtype##_##__abbrsargtype##_v8##__abbrvargtype( __sargtype x, __sargtype y, __vargtype##8 z ) { \
        return (__rettype##8)( __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s0),                          \
                               __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s1),                          \
                               __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s2),                          \
                               __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s3),                          \
                               __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s4),                          \
                               __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s5),                          \
                               __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s6),                          \
                               __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s7) );                        \
    }                                                                                                        \
    __rettype##16 __func##_##__abbrsargtype##_##__abbrsargtype##_v16##__abbrvargtype( __sargtype x, __sargtype y, __vargtype##16 z ) { \
        return (__rettype##16)( __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s0),                         \
                                __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s1),                         \
                                __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s2),                         \
                                __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s3),                         \
                                __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s4),                         \
                                __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s5),                         \
                                __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s6),                         \
                                __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s7),                         \
                                __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s8),                         \
                                __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.s9),                         \
                                __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.sa),                         \
                                __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.sb),                         \
                                __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.sc),                         \
                                __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.sd),                         \
                                __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.se),                         \
                                __func##_##__abbrsargtype##_##__abbrsargtype##_##__abbrvargtype(x, y, z.sf) );                       \
    }

#define GENERATE_VECTOR_FUNCTIONS_3ARGS_SVS( __func, __rettype, __sarg0type, __vargtype, __sarg1type, __abbrsarg0type, __abbrvargtype,__abbrsarg1type ) \
    __rettype##2 __func##_##__abbrsarg0type##_v2##__abbrvargtype##_##__abbrsarg1type( __sarg0type x, __vargtype##2 y, __sarg1type z ) { \
        return (__rettype##2)( __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s0, z),                          \
                               __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s1, z) );                        \
    }                                                                                                        \
    __rettype##3 __func##_##__abbrsarg0type##_v3##__abbrvargtype##_##__abbrsarg1type( __sarg0type x, __vargtype##3 y, __sarg1type z ) { \
        return (__rettype##3)( __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s0, z),                          \
                               __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s1, z),                          \
                               __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s2, z) );                        \
    }                                                                                                        \
    __rettype##4 __func##_##__abbrsarg0type##_v4##__abbrvargtype##_##__abbrsarg1type( __sarg0type x, __vargtype##4 y, __sarg1type z ) { \
        return (__rettype##4)( __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s0, z),                          \
                               __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s1, z),                          \
                               __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s2, z),                          \
                               __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s3, z) );                        \
    }                                                                                                        \
    __rettype##8 __func##_##__abbrsarg0type##_v8##__abbrvargtype##_##__abbrsarg1type( __sarg0type x, __vargtype##8 y, __sarg1type z ) { \
        return (__rettype##8)( __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s0, z),                          \
                               __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s1, z),                          \
                               __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s2, z),                          \
                               __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s3, z),                          \
                               __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s4, z),                          \
                               __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s5, z),                          \
                               __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s6, z),                          \
                               __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s7, z) );                        \
    }                                                                                                        \
    __rettype##16 __func##_##__abbrsarg0type##_v16##__abbrvargtype##_##__abbrsarg1type( __sarg0type x, __vargtype##16 y, __sarg1type z ) { \
        return (__rettype##16)( __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s0, z),                         \
                                __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s1, z),                         \
                                __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s2, z),                         \
                                __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s3, z),                         \
                                __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s4, z),                         \
                                __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s5, z),                         \
                                __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s6, z),                         \
                                __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s7, z),                         \
                                __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s8, z),                         \
                                __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.s9, z),                         \
                                __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.sa, z),                         \
                                __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.sb, z),                         \
                                __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.sc, z),                         \
                                __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.sd, z),                         \
                                __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.se, z),                         \
                                __func##_##__abbrsarg0type##_##__abbrvargtype##_##__abbrsarg1type(x, y.sf, z) );                       \
    }

#define GENERATE_CONVERSIONS_FUNCTIONS_VECTORS( __func, __rettype, __argtype, __abbrrettype, __abbrargtype )      \
    __rettype##2 __func##_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {                 \
        return (__rettype##2)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1) );                              \
    }                                                                               \
    __rettype##3 __func##_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {                 \
        return (__rettype##3)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                               \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s2) );                              \
    }                                                                               \
    __rettype##4 __func##_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {                 \
        return (__rettype##4)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s2),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s3) );                              \
    }                                                                            \
    __rettype##8 __func##_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {                 \
        return (__rettype##8)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s2),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s3),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s4),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s5),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s6),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s7) );                              \
    }                                                                               \
    __rettype##16 __func##_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {              \
        return (__rettype##16)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s1),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s2),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s3),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s4),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s5),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s6),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s7),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s8),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s9),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sa),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sb),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sc),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sd),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.se),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sf) );                             \
    }

#define GENERATE_CONVERSIONS_FUNCTIONS_ROUNDING( __func, __rettype, __argtype, __abbrrettype, __abbrargtype )      \
    __rettype##2 __func##_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {                 \
        return (__rettype##2)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1) );                              \
    }                                                                               \
    __rettype##3 __func##_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {                 \
        return (__rettype##3)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                               \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s2) );                              \
    }                                                                               \
    __rettype##4 __func##_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {                 \
        return (__rettype##4)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s2),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s3) );                              \
    }                                                                               \
    __rettype##8 __func##_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {                 \
        return (__rettype##8)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s2),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s3),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s4),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s5),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s6),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s7) );                              \
    }                                                                               \
    __rettype##16 __func##_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {              \
        return (__rettype##16)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s1),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s2),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s3),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s4),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s5),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s6),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s7),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s8),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s9),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sa),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sb),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sc),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sd),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.se),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sf) );                             \
    }                                                                               \
    __rettype##2 __func##_RTE_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {           \
        return (__rettype##2)( __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s1) );                        \
    }                                                                               \
    __rettype##3 __func##_RTE_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {           \
        return (__rettype##3)( __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s2) );                        \
    }                                                                               \
    __rettype##4 __func##_RTE_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {           \
        return (__rettype##4)( __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s3) );                        \
    }                                                                               \
    __rettype##8 __func##_RTE_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {           \
        return (__rettype##8)( __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s3),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s4),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s5),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s6),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s7) );                        \
    }                                                                               \
    __rettype##16 __func##_RTE_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {        \
        return (__rettype##16)( __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s0),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s1),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s2),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s3),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s4),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s5),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s6),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s7),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s8),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s9),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.sa),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.sb),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.sc),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.sd),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.se),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.sf) );                       \
    }                                                                               \
    __rettype##2 __func##_RTZ_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {           \
        return (__rettype##2)( __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s1) );                        \
    }                                                                                   \
    __rettype##3 __func##_RTZ_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {            \
        return (__rettype##3)( __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s2) );                        \
    }                                                                                   \
    __rettype##4 __func##_RTZ_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {            \
        return (__rettype##4)( __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s3) );                        \
    }                                                                       \
    __rettype##8 __func##_RTZ_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {           \
        return (__rettype##8)( __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s3),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s4),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s5),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s6),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s7) );                        \
    }                                                                       \
    __rettype##16 __func##_RTZ_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {        \
        return (__rettype##16)( __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s0),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s1),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s2),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s3),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s4),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s5),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s6),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s7),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s8),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s9),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.sa),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.sb),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.sc),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.sd),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.se),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.sf) );                       \
    }                                                                       \
    __rettype##2 __func##_RTP_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {           \
        return (__rettype##2)( __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s1) );                        \
    }                                                                       \
    __rettype##3 __func##_RTP_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {           \
        return (__rettype##3)( __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s2) );                        \
    }                                                                       \
    __rettype##4 __func##_RTP_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {           \
        return (__rettype##4)( __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s3) );                        \
    }                                                                       \
    __rettype##8 __func##_RTP_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {           \
        return (__rettype##8)( __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s3),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s4),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s5),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s6),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s7) );                        \
    }                                                                       \
    __rettype##16 __func##_RTP_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {        \
        return (__rettype##16)( __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s0),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s1),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s2),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s3),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s4),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s5),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s6),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s7),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s8),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s9),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.sa),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.sb),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.sc),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.sd),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.se),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.sf) );                       \
    }                                                                       \
    __rettype##2 __func##_RTN_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {           \
        return (__rettype##2)( __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s1) );                        \
    }                                                                       \
    __rettype##3 __func##_RTN_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {           \
        return (__rettype##3)( __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s2) );                        \
    }                                                                       \
    __rettype##4 __func##_RTN_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {           \
        return (__rettype##4)( __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s3) );                        \
    }                                                                       \
    __rettype##8 __func##_RTN_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {           \
        return (__rettype##8)( __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s3),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s4),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s5),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s6),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s7) );                        \
    }                                                                       \
    __rettype##16 __func##_RTN_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {        \
        return (__rettype##16)( __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s0),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s1),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s2),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s3),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s4),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s5),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s6),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s7),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s8),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s9),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.sa),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.sb),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.sc),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.sd),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.se),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.sf) );                       \
    }


#define GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, __argtype, __abbrrettype, __abbrargtype )  \
    __rettype##2 __func##_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {                 \
        return (__rettype##2)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1) );                              \
    }                                                                               \
    __rettype##3 __func##_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {                 \
        return (__rettype##3)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                               \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s2) );                              \
    }                                                                               \
    __rettype##4 __func##_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {                 \
        return (__rettype##4)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s2),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s3) );                              \
    }                                                                            \
    __rettype##8 __func##_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {                 \
        return (__rettype##8)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s2),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s3),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s4),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s5),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s6),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s7) );                              \
    }                                                                               \
    __rettype##16 __func##_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {              \
        return (__rettype##16)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s1),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s2),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s3),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s4),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s5),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s6),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s7),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s8),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s9),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sa),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sb),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sc),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sd),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.se),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sf) );                             \
    }                                                                               \
    __rettype##2 __func##_Sat_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {           \
        return (__rettype##2)( __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s1) );                        \
    }                                                                       \
    __rettype##3 __func##_Sat_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {           \
        return (__rettype##3)( __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s2) );                        \
    }                                                                       \
    __rettype##4 __func##_Sat_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {           \
        return (__rettype##4)( __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s3) );                        \
    }                                                                       \
    __rettype##8 __func##_Sat_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {           \
        return (__rettype##8)( __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s3),                          \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s4),                          \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s5),                          \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s6),                          \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s7) );                        \
    }                                                                       \
    __rettype##16 __func##_Sat_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {        \
        return (__rettype##16)( __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s0),                         \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s1),                         \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s2),                         \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s3),                         \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s4),                         \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s5),                         \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s6),                         \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s7),                         \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s8),                         \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s9),                         \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.sa),                         \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.sb),                         \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.sc),                         \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.sd),                         \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.se),                         \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.sf) );                       \
    }



    #define GENERATE_CONVERSIONS_FUNCTIONS_BOTH( __func, __rettype, __argtype, __abbrrettype, __abbrargtype )      \
    __rettype##2 __func##_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {                 \
        return (__rettype##2)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1) );                              \
    }                                                                               \
    __rettype##3 __func##_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {                 \
        return (__rettype##3)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                               \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s2) );                              \
    }                                                                               \
    __rettype##4 __func##_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {                 \
        return (__rettype##4)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s2),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s3) );                              \
    }                                                                            \
    __rettype##8 __func##_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {                 \
        return (__rettype##8)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s1),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s2),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s3),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s4),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s5),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s6),                                \
                               __func##_##__abbrrettype##_##__abbrargtype(x.s7) );                              \
    }                                                                               \
    __rettype##16 __func##_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {              \
        return (__rettype##16)( __func##_##__abbrrettype##_##__abbrargtype(x.s0),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s1),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s2),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s3),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s4),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s5),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s6),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s7),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s8),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.s9),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sa),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sb),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sc),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sd),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.se),                               \
                                __func##_##__abbrrettype##_##__abbrargtype(x.sf) );                             \
    }                                                                               \
    __rettype##2 __func##_RTE_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {           \
        return (__rettype##2)( __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s1) );                        \
    }                                                                               \
    __rettype##3 __func##_RTE_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {           \
        return (__rettype##3)( __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s2) );                        \
    }                                                                               \
    __rettype##4 __func##_RTE_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {           \
        return (__rettype##4)( __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s3) );                        \
    }                                                                               \
    __rettype##8 __func##_RTE_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {           \
        return (__rettype##8)( __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s3),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s4),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s5),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s6),                          \
                               __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s7) );                        \
    }                                                                               \
    __rettype##16 __func##_RTE_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {        \
        return (__rettype##16)( __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s0),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s1),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s2),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s3),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s4),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s5),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s6),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s7),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s8),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.s9),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.sa),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.sb),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.sc),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.sd),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.se),                         \
                                __func##_RTE_##__abbrrettype##_##__abbrargtype(x.sf) );                       \
    }                                                                               \
    __rettype##2 __func##_RTZ_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {           \
        return (__rettype##2)( __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s1) );                        \
    }                                                                                   \
    __rettype##3 __func##_RTZ_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {            \
        return (__rettype##3)( __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s2) );                        \
    }                                                                                   \
    __rettype##4 __func##_RTZ_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {            \
        return (__rettype##4)( __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s3) );                        \
    }                                                                       \
    __rettype##8 __func##_RTZ_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {           \
        return (__rettype##8)( __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s3),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s4),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s5),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s6),                          \
                               __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s7) );                        \
    }                                                                       \
    __rettype##16 __func##_RTZ_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {        \
        return (__rettype##16)( __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s0),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s1),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s2),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s3),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s4),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s5),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s6),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s7),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s8),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.s9),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.sa),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.sb),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.sc),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.sd),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.se),                         \
                                __func##_RTZ_##__abbrrettype##_##__abbrargtype(x.sf) );                       \
    }                                                                       \
    __rettype##2 __func##_RTP_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {           \
        return (__rettype##2)( __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s1) );                        \
    }                                                                       \
    __rettype##3 __func##_RTP_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {           \
        return (__rettype##3)( __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s2) );                        \
    }                                                                       \
    __rettype##4 __func##_RTP_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {           \
        return (__rettype##4)( __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s3) );                        \
    }                                                                       \
    __rettype##8 __func##_RTP_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {           \
        return (__rettype##8)( __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s3),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s4),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s5),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s6),                          \
                               __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s7) );                        \
    }                                                                       \
    __rettype##16 __func##_RTP_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {        \
        return (__rettype##16)( __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s0),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s1),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s2),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s3),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s4),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s5),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s6),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s7),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s8),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.s9),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.sa),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.sb),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.sc),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.sd),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.se),                         \
                                __func##_RTP_##__abbrrettype##_##__abbrargtype(x.sf) );                       \
    }                                                                       \
    __rettype##2 __func##_RTN_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {           \
        return (__rettype##2)( __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s1) );                        \
    }                                                                       \
    __rettype##3 __func##_RTN_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {           \
        return (__rettype##3)( __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s2) );                        \
    }                                                                       \
    __rettype##4 __func##_RTN_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {           \
        return (__rettype##4)( __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s3) );                        \
    }                                                                       \
    __rettype##8 __func##_RTN_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {           \
        return (__rettype##8)( __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s0),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s1),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s2),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s3),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s4),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s5),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s6),                          \
                               __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s7) );                        \
    }                                                                       \
    __rettype##16 __func##_RTN_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {        \
        return (__rettype##16)( __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s0),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s1),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s2),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s3),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s4),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s5),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s6),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s7),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s8),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.s9),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.sa),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.sb),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.sc),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.sd),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.se),                         \
                                __func##_RTN_##__abbrrettype##_##__abbrargtype(x.sf) );                       \
    }                                                                                                         \
    __rettype##2 __func##_Sat_RTE_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {       \
        return (__rettype##2)( __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s1) );                    \
    }                                                                       \
    __rettype##3 __func##_Sat_RTE_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {       \
        return (__rettype##3)( __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s1),                      \
                               __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s2) );                    \
    }                                                                       \
    __rettype##4 __func##_Sat_RTE_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {       \
        return (__rettype##4)( __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s1),                      \
                               __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s2),                      \
                               __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s3) );                    \
    }                                                                       \
    __rettype##8 __func##_Sat_RTE_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {       \
        return (__rettype##8)( __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s1),                      \
                               __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s2),                      \
                               __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s3),                      \
                               __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s4),                      \
                               __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s5),                      \
                               __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s6),                      \
                               __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s7) );                    \
    }                                                                       \
    __rettype##16 __func##_Sat_RTE_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {    \
        return (__rettype##16)( __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s0),                     \
                                __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s1),                     \
                                __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s2),                     \
                                __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s3),                     \
                                __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s4),                     \
                                __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s5),                     \
                                __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s6),                     \
                                __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s7),                     \
                                __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s8),                     \
                                __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.s9),                     \
                                __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.sa),                     \
                                __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.sb),                     \
                                __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.sc),                     \
                                __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.sd),                     \
                                __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.se),                     \
                                __func##_Sat_RTE_##__abbrrettype##_##__abbrargtype(x.sf) );                   \
    }                                                                       \
    __rettype##2 __func##_Sat_RTZ_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {       \
        return (__rettype##2)( __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s1) );                    \
    }                                                                       \
    __rettype##3 __func##_Sat_RTZ_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {       \
        return (__rettype##3)( __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s1),                      \
                               __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s2) );                    \
    }                                                                       \
    __rettype##4 __func##_Sat_RTZ_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {       \
        return (__rettype##4)( __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s1),                      \
                               __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s2),                      \
                               __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s3) );                    \
    }                                                                       \
    __rettype##8 __func##_Sat_RTZ_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {       \
        return (__rettype##8)( __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s1),                      \
                               __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s2),                      \
                               __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s3),                      \
                               __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s4),                      \
                               __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s5),                      \
                               __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s6),                      \
                               __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s7) );                    \
    }                                                                       \
    __rettype##16 __func##_Sat_RTZ_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {    \
        return (__rettype##16)( __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s0),                     \
                                __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s1),                     \
                                __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s2),                     \
                                __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s3),                     \
                                __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s4),                     \
                                __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s5),                     \
                                __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s6),                     \
                                __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s7),                     \
                                __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s8),                     \
                                __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.s9),                     \
                                __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.sa),                     \
                                __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.sb),                     \
                                __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.sc),                     \
                                __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.sd),                     \
                                __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.se),                     \
                                __func##_Sat_RTZ_##__abbrrettype##_##__abbrargtype(x.sf) );                   \
    }                                                                       \
    __rettype##2 __func##_Sat_RTP_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {       \
        return (__rettype##2)( __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s1) );                    \
    }                                                                       \
    __rettype##3 __func##_Sat_RTP_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {       \
        return (__rettype##3)( __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s1),                      \
                               __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s2) );                    \
    }                                                                       \
    __rettype##4 __func##_Sat_RTP_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {       \
        return (__rettype##4)( __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s1),                      \
                               __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s2),                      \
                               __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s3) );                    \
    }                                                                       \
    __rettype##8 __func##_Sat_RTP_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {       \
        return (__rettype##8)( __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s1),                      \
                               __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s2),                      \
                               __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s3),                      \
                               __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s4),                      \
                               __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s5),                      \
                               __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s6),                      \
                               __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s7) );                    \
    }                                                                       \
    __rettype##16 __func##_Sat_RTP_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {    \
        return (__rettype##16)( __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s0),                     \
                                __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s1),                     \
                                __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s2),                     \
                                __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s3),                     \
                                __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s4),                     \
                                __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s5),                     \
                                __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s6),                     \
                                __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s7),                     \
                                __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s8),                     \
                                __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.s9),                     \
                                __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.sa),                     \
                                __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.sb),                     \
                                __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.sc),                     \
                                __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.sd),                     \
                                __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.se),                     \
                                __func##_Sat_RTP_##__abbrrettype##_##__abbrargtype(x.sf) );                   \
    }                                                                       \
    __rettype##2 __func##_Sat_RTN_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {       \
        return (__rettype##2)( __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s1) );                    \
    }                                                                       \
    __rettype##3 __func##_Sat_RTN_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {       \
        return (__rettype##3)( __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s1),                      \
                               __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s2) );                    \
    }                                                                       \
    __rettype##4 __func##_Sat_RTN_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {       \
        return (__rettype##4)( __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s1),                      \
                               __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s2),                      \
                               __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s3) );                    \
    }                                                                       \
    __rettype##8 __func##_Sat_RTN_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {       \
        return (__rettype##8)( __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s1),                      \
                               __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s2),                      \
                               __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s3),                      \
                               __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s4),                      \
                               __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s5),                      \
                               __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s6),                      \
                               __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s7) );                    \
    }                                                                       \
    __rettype##16 __func##_Sat_RTN_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {    \
        return (__rettype##16)( __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s0),                     \
                                __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s1),                     \
                                __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s2),                     \
                                __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s3),                     \
                                __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s4),                     \
                                __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s5),                     \
                                __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s6),                     \
                                __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s7),                     \
                                __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s8),                     \
                                __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.s9),                     \
                                __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.sa),                     \
                                __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.sb),                     \
                                __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.sc),                     \
                                __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.sd),                     \
                                __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.se),                     \
                                __func##_Sat_RTN_##__abbrrettype##_##__abbrargtype(x.sf) );                   \
    }                                                                                                         \
    __rettype##2 __func##_Sat_v2##__abbrrettype##_v2##__abbrargtype( __argtype##2 x ) {                   \
        return (__rettype##2)( __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s1) );                    \
    }                                                                       \
    __rettype##3 __func##_Sat_v3##__abbrrettype##_v3##__abbrargtype( __argtype##3 x ) {       \
        return (__rettype##3)( __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s1),                      \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s2) );                    \
    }                                                                       \
    __rettype##4 __func##_Sat_v4##__abbrrettype##_v4##__abbrargtype( __argtype##4 x ) {       \
        return (__rettype##4)( __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s1),                      \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s2),                      \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s3) );                    \
    }                                                                       \
    __rettype##8 __func##_Sat_v8##__abbrrettype##_v8##__abbrargtype( __argtype##8 x ) {       \
        return (__rettype##8)( __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s0),                      \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s1),                      \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s2),                      \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s3),                      \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s4),                      \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s5),                      \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s6),                      \
                               __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s7) );                    \
    }                                                                       \
    __rettype##16 __func##_Sat_v16##__abbrrettype##_v16##__abbrargtype( __argtype##16 x ) {    \
        return (__rettype##16)( __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s0),                     \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s1),                     \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s2),                     \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s3),                     \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s4),                     \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s5),                     \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s6),                     \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s7),                     \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s8),                     \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.s9),                     \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.sa),                     \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.sb),                     \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.sc),                     \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.sd),                     \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.se),                     \
                                __func##_Sat_##__abbrrettype##_##__abbrargtype(x.sf) );                   \
    }



#define GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_SignedToFloat( __func, __rettype, __abbrrettype)       \
    GENERATE_CONVERSIONS_FUNCTIONS_ROUNDING( __func, __rettype, char, __abbrrettype, i8 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_ROUNDING( __func, __rettype, short, __abbrrettype, i16 )         \
    GENERATE_CONVERSIONS_FUNCTIONS_ROUNDING( __func, __rettype, int, __abbrrettype,  i32 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_ROUNDING( __func, __rettype, long, __abbrrettype, i64 )

#define GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_UnsignedToFloat( __func, __rettype, __abbrrettype)       \
    GENERATE_CONVERSIONS_FUNCTIONS_ROUNDING( __func, __rettype, uchar, __abbrrettype, i8 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_ROUNDING( __func, __rettype, ushort, __abbrrettype, i16 )         \
    GENERATE_CONVERSIONS_FUNCTIONS_ROUNDING( __func, __rettype, uint, __abbrrettype, i32 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_ROUNDING( __func, __rettype, ulong, __abbrrettype, i64 )

#if defined(cl_khr_fp64)
#define GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Float( __func, __rettype, __abbrrettype)       \
    GENERATE_CONVERSIONS_FUNCTIONS_ROUNDING( __func, __rettype, half, __abbrrettype, f16 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_ROUNDING( __func, __rettype, float, __abbrrettype, f32 )         \
    GENERATE_CONVERSIONS_FUNCTIONS_ROUNDING( __func, __rettype, double, __abbrrettype, f64 )
#else
#define GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Float( __func, __rettype, __abbrrettype)       \
    GENERATE_CONVERSIONS_FUNCTIONS_ROUNDING( __func, __rettype, half, __abbrrettype, f16 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_ROUNDING( __func, __rettype, float, __abbrrettype, f32 )
#endif // defined(cl_khr_fp64)

#define GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Signed( __func, __rettype, __abbrrettype)       \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, char, __abbrrettype, i8 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, short, __abbrrettype, i16 )         \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, int, __abbrrettype, i32 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, long, __abbrrettype, i64 )

#define GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Unsigned( __func, __rettype, __abbrrettype)       \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, uchar, __abbrrettype, i8 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, ushort, __abbrrettype, i16 )         \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, uint, __abbrrettype, i32 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, ulong, __abbrrettype, i64 )

#if defined(cl_khr_fp64)
#define GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( __func, __rettype, __abbrrettype)       \
    GENERATE_CONVERSIONS_FUNCTIONS_BOTH( __func, __rettype, half, __abbrrettype, f16 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_BOTH( __func, __rettype, float, __abbrrettype, f32 )         \
    GENERATE_CONVERSIONS_FUNCTIONS_BOTH( __func, __rettype, double, __abbrrettype, f64 )
#else
#define GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( __func, __rettype, __abbrrettype)       \
    GENERATE_CONVERSIONS_FUNCTIONS_BOTH( __func, __rettype, half, __abbrrettype, f16 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_BOTH( __func, __rettype, float, __abbrrettype, f32 )
#endif // defined(cl_khr_fp64)

#define GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_SignedToUnsigned( __func, __rettype, __abbrrettype)       \
    GENERATE_CONVERSIONS_FUNCTIONS_VECTORS( __func, __rettype, char, __abbrrettype, i8 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_VECTORS( __func, __rettype, short, __abbrrettype, i16 )         \
    GENERATE_CONVERSIONS_FUNCTIONS_VECTORS( __func, __rettype, int, __abbrrettype, i32 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_VECTORS( __func, __rettype, long, __abbrrettype, i64 )

#define GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_UnsignedToSigned( __func, __rettype, __abbrrettype)       \
    GENERATE_CONVERSIONS_FUNCTIONS_VECTORS( __func, __rettype, uchar, __abbrrettype, i8 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_VECTORS( __func, __rettype, ushort, __abbrrettype, i16 )         \
    GENERATE_CONVERSIONS_FUNCTIONS_VECTORS( __func, __rettype, uint, __abbrrettype, i32 )           \
    GENERATE_CONVERSIONS_FUNCTIONS_VECTORS( __func, __rettype, ulong, __abbrrettype, i64 )

#endif //_IBIF_HEADER_
