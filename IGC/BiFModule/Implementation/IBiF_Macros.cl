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
        __num_elements_type index = __intel_LocalInvocationIndex();            \
        __num_elements_type step = __intel_WorkgroupSize();                               \
        for( ; index < uiNumElements; index += step ) {                     \
            dst[index] = src[index];                                        \
        }                                                                   \
    }

#define ASYNC_WORK_GROUP_STRIDED_COPY_G2L(dst, src, num_elements, src_stride, evt, __num_elements_type)  \
    {                                                                       \
        __num_elements_type uiNumElements = num_elements;                                  \
        __num_elements_type uiStride = src_stride;                                         \
        __num_elements_type dstIndex = __intel_LocalInvocationIndex();         \
        __num_elements_type dstStep = __intel_WorkgroupSize();                            \
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
        __num_elements_type srcIndex = __intel_LocalInvocationIndex();         \
        __num_elements_type srcStep = __intel_WorkgroupSize();                            \
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
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v2##__abbrrettype##_v2##__abbrargtype, _R##__rettype##2)( __argtype##2 x ) {                 \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1) );                              \
    }                                                                               \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v3##__abbrrettype##_v3##__abbrargtype, _R##__rettype##3)( __argtype##3 x ) {                 \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                               \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2) );                              \
    }                                                                               \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v4##__abbrrettype##_v4##__abbrargtype, _R##__rettype##4)( __argtype##4 x ) {                 \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s3) );                              \
    }                                                                            \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v8##__abbrrettype##_v8##__abbrargtype, _R##__rettype##8)( __argtype##8 x ) {                 \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s3),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s4),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s5),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s6),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s7) );                              \
    }                                                                               \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v16##__abbrrettype##_v16##__abbrargtype, _R##__rettype##16)( __argtype##16 x ) {              \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s3),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s4),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s5),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s6),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s7),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s8),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s9),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sa),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sb),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sc),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sd),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.se),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sf) );                             \
    }

#define GENERATE_CONVERSIONS_FUNCTIONS_ROUNDING( __func, __rettype, __argtype, __abbrrettype, __abbrargtype )      \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v2##__abbrrettype##_v2##__abbrargtype, _R##__rettype##2)( __argtype##2 x ) {                 \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1) );                              \
    }                                                                               \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v3##__abbrrettype##_v3##__abbrargtype, _R##__rettype##3)( __argtype##3 x ) {                 \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                               \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2) );                              \
    }                                                                               \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v4##__abbrrettype##_v4##__abbrargtype, _R##__rettype##4)( __argtype##4 x ) {                 \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s3) );                              \
    }                                                                               \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v8##__abbrrettype##_v8##__abbrargtype, _R##__rettype##8)( __argtype##8 x ) {                 \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s3),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s4),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s5),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s6),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s7) );                              \
    }                                                                               \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v16##__abbrrettype##_v16##__abbrargtype, _R##__rettype##16)( __argtype##16 x ) {              \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s3),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s4),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s5),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s6),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s7),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s8),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s9),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sa),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sb),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sc),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sd),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.se),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sf) );                             \
    }                                                                               \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTE_v2##__abbrrettype##_v2##__abbrargtype, _rte_R##__rettype##2)( __argtype##2 x ) {           \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s1) );                        \
    }                                                                               \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTE_v3##__abbrrettype##_v3##__abbrargtype, _rte_R##__rettype##3)( __argtype##3 x ) {           \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s2) );                        \
    }                                                                               \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTE_v4##__abbrrettype##_v4##__abbrargtype, _rte_R##__rettype##4)( __argtype##4 x ) {           \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s3) );                        \
    }                                                                               \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTE_v8##__abbrrettype##_v8##__abbrargtype, _rte_R##__rettype##8)( __argtype##8 x ) {           \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s3),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s4),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s5),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s6),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s7) );                        \
    }                                                                               \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTE_v16##__abbrrettype##_v16##__abbrargtype, _rte_R##__rettype##16)( __argtype##16 x ) {        \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s0),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s1),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s2),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s3),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s4),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s5),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s6),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s7),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s8),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s9),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.sa),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.sb),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.sc),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.sd),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.se),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.sf) );                       \
    }                                                                               \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTZ_v2##__abbrrettype##_v2##__abbrargtype, _rtz_R##__rettype##2)( __argtype##2 x ) {           \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s1) );                        \
    }                                                                                   \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTZ_v3##__abbrrettype##_v3##__abbrargtype, _rtz_R##__rettype##3)( __argtype##3 x ) {            \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s2) );                        \
    }                                                                                   \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTZ_v4##__abbrrettype##_v4##__abbrargtype, _rtz_R##__rettype##4)( __argtype##4 x ) {            \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s3) );                        \
    }                                                                       \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTZ_v8##__abbrrettype##_v8##__abbrargtype, _rtz_R##__rettype##8)( __argtype##8 x ) {           \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s3),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s4),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s5),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s6),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s7) );                        \
    }                                                                       \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTZ_v16##__abbrrettype##_v16##__abbrargtype, _rtz_R##__rettype##16)( __argtype##16 x ) {        \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s0),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s1),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s2),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s3),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s4),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s5),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s6),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s7),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s8),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s9),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.sa),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.sb),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.sc),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.sd),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.se),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.sf) );                       \
    }                                                                       \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTP_v2##__abbrrettype##_v2##__abbrargtype, _rtp_R##__rettype##2)( __argtype##2 x ) {           \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s1) );                        \
    }                                                                       \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTP_v3##__abbrrettype##_v3##__abbrargtype, _rtp_R##__rettype##3)( __argtype##3 x ) {           \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s2) );                        \
    }                                                                       \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTP_v4##__abbrrettype##_v4##__abbrargtype, _rtp_R##__rettype##4)( __argtype##4 x ) {           \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s3) );                        \
    }                                                                       \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTP_v8##__abbrrettype##_v8##__abbrargtype, _rtp_R##__rettype##8)( __argtype##8 x ) {           \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s3),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s4),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s5),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s6),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s7) );                        \
    }                                                                       \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTP_v16##__abbrrettype##_v16##__abbrargtype, _rtp_R##__rettype##16)( __argtype##16 x ) {        \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s0),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s1),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s2),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s3),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s4),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s5),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s6),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s7),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s8),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s9),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.sa),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.sb),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.sc),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.sd),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.se),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.sf) );                       \
    }                                                                       \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTN_v2##__abbrrettype##_v2##__abbrargtype, _rtn_R##__rettype##2)( __argtype##2 x ) {           \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s1) );                        \
    }                                                                       \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTN_v3##__abbrrettype##_v3##__abbrargtype, _rtn_R##__rettype##3)( __argtype##3 x ) {           \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s2) );                        \
    }                                                                       \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTN_v4##__abbrrettype##_v4##__abbrargtype, _rtn_R##__rettype##4)( __argtype##4 x ) {           \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s3) );                        \
    }                                                                       \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTN_v8##__abbrrettype##_v8##__abbrargtype, _rtn_R##__rettype##8)( __argtype##8 x ) {           \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s3),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s4),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s5),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s6),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s7) );                        \
    }                                                                       \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTN_v16##__abbrrettype##_v16##__abbrargtype, _rtn_R##__rettype##16)( __argtype##16 x ) {        \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s0),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s1),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s2),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s3),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s4),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s5),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s6),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s7),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s8),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s9),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.sa),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.sb),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.sc),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.sd),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.se),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.sf) );                       \
    }


#define GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, __argtype, __abbrrettype, __abbrargtype )  \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v2##__abbrrettype##_v2##__abbrargtype, _R##__rettype##2)( __argtype##2 x ) {                 \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1) );                              \
    }                                                                               \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v3##__abbrrettype##_v3##__abbrargtype, _R##__rettype##3)( __argtype##3 x ) {                 \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                               \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2) );                              \
    }                                                                               \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v4##__abbrrettype##_v4##__abbrargtype, _R##__rettype##4)( __argtype##4 x ) {                 \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s3) );                              \
    }                                                                            \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v8##__abbrrettype##_v8##__abbrargtype, _R##__rettype##8)( __argtype##8 x ) {                 \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s3),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s4),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s5),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s6),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s7) );                              \
    }                                                                               \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v16##__abbrrettype##_v16##__abbrargtype, _R##__rettype##16)( __argtype##16 x ) {              \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s3),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s4),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s5),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s6),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s7),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s8),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s9),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sa),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sb),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sc),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sd),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.se),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sf) );                             \
    }                                                                               \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_v2##__abbrrettype##_v2##__abbrargtype, _sat_R##__rettype##2)( __argtype##2 x ) {           \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s1) );                        \
    }                                                                       \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_v3##__abbrrettype##_v3##__abbrargtype, _sat_R##__rettype##3)( __argtype##3 x ) {           \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s2) );                        \
    }                                                                       \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_v4##__abbrrettype##_v4##__abbrargtype, _sat_R##__rettype##4)( __argtype##4 x ) {           \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s3) );                        \
    }                                                                       \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_v8##__abbrrettype##_v8##__abbrargtype, _sat_R##__rettype##8)( __argtype##8 x ) {           \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s3),                          \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s4),                          \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s5),                          \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s6),                          \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s7) );                        \
    }                                                                       \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_v16##__abbrrettype##_v16##__abbrargtype, _sat_R##__rettype##16)( __argtype##16 x ) {        \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s0),                         \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s1),                         \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s2),                         \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s3),                         \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s4),                         \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s5),                         \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s6),                         \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s7),                         \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s8),                         \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s9),                         \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.sa),                         \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.sb),                         \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.sc),                         \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.sd),                         \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.se),                         \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.sf) );                       \
    }



    #define GENERATE_CONVERSIONS_FUNCTIONS_BOTH( __func, __rettype, __argtype, __abbrrettype, __abbrargtype )      \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v2##__abbrrettype##_v2##__abbrargtype, _R##__rettype##2)( __argtype##2 x ) {                 \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1) );                              \
    }                                                                               \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v3##__abbrrettype##_v3##__abbrargtype, _R##__rettype##3)( __argtype##3 x ) {                 \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                               \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2) );                              \
    }                                                                               \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v4##__abbrrettype##_v4##__abbrargtype, _R##__rettype##4)( __argtype##4 x ) {                 \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s3) );                              \
    }                                                                            \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v8##__abbrrettype##_v8##__abbrargtype, _R##__rettype##8)( __argtype##8 x ) {                 \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s3),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s4),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s5),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s6),                                \
                               SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s7) );                              \
    }                                                                               \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _v16##__abbrrettype##_v16##__abbrargtype, _R##__rettype##16)( __argtype##16 x ) {              \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s0),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s1),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s2),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s3),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s4),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s5),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s6),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s7),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s8),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.s9),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sa),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sb),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sc),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sd),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.se),                               \
                                SPIRV_BUILTIN(__func, _##__abbrrettype##_##__abbrargtype, _R##__rettype)(x.sf) );                             \
    }                                                                               \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTE_v2##__abbrrettype##_v2##__abbrargtype, _rte_R##__rettype##2)( __argtype##2 x ) {           \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s1) );                        \
    }                                                                               \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTE_v3##__abbrrettype##_v3##__abbrargtype, _rte_R##__rettype##3)( __argtype##3 x ) {           \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s2) );                        \
    }                                                                               \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTE_v4##__abbrrettype##_v4##__abbrargtype, _rte_R##__rettype##4)( __argtype##4 x ) {           \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s3) );                        \
    }                                                                               \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTE_v8##__abbrrettype##_v8##__abbrargtype, _rte_R##__rettype##8)( __argtype##8 x ) {           \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s3),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s4),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s5),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s6),                          \
                               SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s7) );                        \
    }                                                                               \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTE_v16##__abbrrettype##_v16##__abbrargtype, _rte_R##__rettype##16)( __argtype##16 x ) {        \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s0),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s1),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s2),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s3),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s4),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s5),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s6),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s7),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s8),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.s9),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.sa),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.sb),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.sc),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.sd),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.se),                         \
                                SPIRV_BUILTIN(__func, _RTE_##__abbrrettype##_##__abbrargtype, _rte_R##__rettype)(x.sf) );                       \
    }                                                                               \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTZ_v2##__abbrrettype##_v2##__abbrargtype, _rtz_R##__rettype##2)( __argtype##2 x ) {           \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s1) );                        \
    }                                                                                   \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTZ_v3##__abbrrettype##_v3##__abbrargtype, _rtz_R##__rettype##3)( __argtype##3 x ) {            \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s2) );                        \
    }                                                                                   \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTZ_v4##__abbrrettype##_v4##__abbrargtype, _rtz_R##__rettype##4)( __argtype##4 x ) {            \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s3) );                        \
    }                                                                       \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTZ_v8##__abbrrettype##_v8##__abbrargtype, _rtz_R##__rettype##8)( __argtype##8 x ) {           \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s3),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s4),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s5),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s6),                          \
                               SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s7) );                        \
    }                                                                       \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTZ_v16##__abbrrettype##_v16##__abbrargtype, _rtz_R##__rettype##16)( __argtype##16 x ) {        \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s0),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s1),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s2),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s3),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s4),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s5),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s6),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s7),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s8),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.s9),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.sa),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.sb),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.sc),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.sd),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.se),                         \
                                SPIRV_BUILTIN(__func, _RTZ_##__abbrrettype##_##__abbrargtype, _rtz_R##__rettype)(x.sf) );                       \
    }                                                                       \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTP_v2##__abbrrettype##_v2##__abbrargtype, _rtp_R##__rettype##2)( __argtype##2 x ) {           \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s1) );                        \
    }                                                                       \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTP_v3##__abbrrettype##_v3##__abbrargtype, _rtp_R##__rettype##3)( __argtype##3 x ) {           \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s2) );                        \
    }                                                                       \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTP_v4##__abbrrettype##_v4##__abbrargtype, _rtp_R##__rettype##4)( __argtype##4 x ) {           \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s3) );                        \
    }                                                                       \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTP_v8##__abbrrettype##_v8##__abbrargtype, _rtp_R##__rettype##8)( __argtype##8 x ) {           \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s3),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s4),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s5),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s6),                          \
                               SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s7) );                        \
    }                                                                       \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTP_v16##__abbrrettype##_v16##__abbrargtype, _rtp_R##__rettype##16)( __argtype##16 x ) {        \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s0),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s1),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s2),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s3),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s4),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s5),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s6),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s7),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s8),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.s9),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.sa),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.sb),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.sc),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.sd),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.se),                         \
                                SPIRV_BUILTIN(__func, _RTP_##__abbrrettype##_##__abbrargtype, _rtp_R##__rettype)(x.sf) );                       \
    }                                                                       \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTN_v2##__abbrrettype##_v2##__abbrargtype, _rtn_R##__rettype##2)( __argtype##2 x ) {           \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s1) );                        \
    }                                                                       \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTN_v3##__abbrrettype##_v3##__abbrargtype, _rtn_R##__rettype##3)( __argtype##3 x ) {           \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s2) );                        \
    }                                                                       \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTN_v4##__abbrrettype##_v4##__abbrargtype, _rtn_R##__rettype##4)( __argtype##4 x ) {           \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s3) );                        \
    }                                                                       \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTN_v8##__abbrrettype##_v8##__abbrargtype, _rtn_R##__rettype##8)( __argtype##8 x ) {           \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s0),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s1),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s2),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s3),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s4),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s5),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s6),                          \
                               SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s7) );                        \
    }                                                                       \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _RTN_v16##__abbrrettype##_v16##__abbrargtype, _rtn_R##__rettype##16)( __argtype##16 x ) {        \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s0),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s1),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s2),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s3),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s4),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s5),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s6),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s7),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s8),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.s9),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.sa),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.sb),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.sc),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.sd),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.se),                         \
                                SPIRV_BUILTIN(__func, _RTN_##__abbrrettype##_##__abbrargtype, _rtn_R##__rettype)(x.sf) );                       \
    }                                                                                                         \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTE_v2##__abbrrettype##_v2##__abbrargtype, _sat_rte_R##__rettype##2)( __argtype##2 x ) {       \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s1) );                    \
    }                                                                       \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTE_v3##__abbrrettype##_v3##__abbrargtype, _sat_rte_R##__rettype##3)( __argtype##3 x ) {       \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s1),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s2) );                    \
    }                                                                       \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTE_v4##__abbrrettype##_v4##__abbrargtype, _sat_rte_R##__rettype##4)( __argtype##4 x ) {       \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s1),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s2),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s3) );                    \
    }                                                                       \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTE_v8##__abbrrettype##_v8##__abbrargtype, _sat_rte_R##__rettype##8)( __argtype##8 x ) {       \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s1),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s2),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s3),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s4),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s5),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s6),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s7) );                    \
    }                                                                       \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTE_v16##__abbrrettype##_v16##__abbrargtype, _sat_rte_R##__rettype##16)( __argtype##16 x ) {    \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s0),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s1),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s2),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s3),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s4),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s5),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s6),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s7),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s8),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.s9),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.sa),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.sb),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.sc),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.sd),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.se),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTE_##__abbrrettype##_##__abbrargtype, _sat_rte_R##__rettype)(x.sf) );                   \
    }                                                                       \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTZ_v2##__abbrrettype##_v2##__abbrargtype, _sat_rtz_R##__rettype##2)( __argtype##2 x ) {       \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s1) );                    \
    }                                                                       \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTZ_v3##__abbrrettype##_v3##__abbrargtype, _sat_rtz_R##__rettype##3)( __argtype##3 x ) {       \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s1),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s2) );                    \
    }                                                                       \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTZ_v4##__abbrrettype##_v4##__abbrargtype, _sat_rtz_R##__rettype##4)( __argtype##4 x ) {       \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s1),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s2),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s3) );                    \
    }                                                                       \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTZ_v8##__abbrrettype##_v8##__abbrargtype, _sat_rtz_R##__rettype##8)( __argtype##8 x ) {       \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s1),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s2),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s3),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s4),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s5),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s6),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s7) );                    \
    }                                                                       \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTZ_v16##__abbrrettype##_v16##__abbrargtype, _sat_rtz_R##__rettype##16)( __argtype##16 x ) {    \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s0),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s1),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s2),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s3),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s4),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s5),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s6),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s7),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s8),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.s9),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.sa),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.sb),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.sc),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.sd),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.se),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTZ_##__abbrrettype##_##__abbrargtype, _sat_rtz_R##__rettype)(x.sf) );                   \
    }                                                                       \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTP_v2##__abbrrettype##_v2##__abbrargtype, _sat_rtp_R##__rettype##2)( __argtype##2 x ) {       \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s1) );                    \
    }                                                                       \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTP_v3##__abbrrettype##_v3##__abbrargtype, _sat_rtp_R##__rettype##3)( __argtype##3 x ) {       \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s1),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s2) );                    \
    }                                                                       \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTP_v4##__abbrrettype##_v4##__abbrargtype, _sat_rtp_R##__rettype##4)( __argtype##4 x ) {       \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s1),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s2),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s3) );                    \
    }                                                                       \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTP_v8##__abbrrettype##_v8##__abbrargtype, _sat_rtp_R##__rettype##8)( __argtype##8 x ) {       \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s1),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s2),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s3),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s4),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s5),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s6),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s7) );                    \
    }                                                                       \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTP_v16##__abbrrettype##_v16##__abbrargtype, _sat_rtp_R##__rettype##16)( __argtype##16 x ) {    \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s0),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s1),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s2),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s3),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s4),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s5),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s6),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s7),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s8),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.s9),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.sa),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.sb),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.sc),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.sd),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.se),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTP_##__abbrrettype##_##__abbrargtype, _sat_rtp_R##__rettype)(x.sf) );                   \
    }                                                                       \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTN_v2##__abbrrettype##_v2##__abbrargtype, _sat_rtn_R##__rettype##2)( __argtype##2 x ) {       \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s1) );                    \
    }                                                                       \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTN_v3##__abbrrettype##_v3##__abbrargtype, _sat_rtn_R##__rettype##3)( __argtype##3 x ) {       \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s1),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s2) );                    \
    }                                                                       \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTN_v4##__abbrrettype##_v4##__abbrargtype, _sat_rtn_R##__rettype##4)( __argtype##4 x ) {       \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s1),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s2),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s3) );                    \
    }                                                                       \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTN_v8##__abbrrettype##_v8##__abbrargtype, _sat_rtn_R##__rettype##8)( __argtype##8 x ) {       \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s1),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s2),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s3),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s4),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s5),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s6),                      \
                               SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s7) );                    \
    }                                                                       \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_RTN_v16##__abbrrettype##_v16##__abbrargtype, _sat_rtn_R##__rettype##16)( __argtype##16 x ) {    \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s0),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s1),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s2),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s3),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s4),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s5),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s6),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s7),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s8),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.s9),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.sa),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.sb),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.sc),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.sd),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.se),                     \
                                SPIRV_BUILTIN(__func, _Sat_RTN_##__abbrrettype##_##__abbrargtype, _sat_rtn_R##__rettype)(x.sf) );                   \
    }                                                                                                         \
    __rettype##2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_v2##__abbrrettype##_v2##__abbrargtype, _sat_R##__rettype##2)( __argtype##2 x ) {                   \
        return (__rettype##2)( SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s1) );                    \
    }                                                                       \
    __rettype##3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_v3##__abbrrettype##_v3##__abbrargtype, _sat_R##__rettype##3)( __argtype##3 x ) {       \
        return (__rettype##3)( SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s1),                      \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s2) );                    \
    }                                                                       \
    __rettype##4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_v4##__abbrrettype##_v4##__abbrargtype, _sat_R##__rettype##4)( __argtype##4 x ) {       \
        return (__rettype##4)( SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s1),                      \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s2),                      \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s3) );                    \
    }                                                                       \
    __rettype##8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_v8##__abbrrettype##_v8##__abbrargtype, _sat_R##__rettype##8)( __argtype##8 x ) {       \
        return (__rettype##8)( SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s0),                      \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s1),                      \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s2),                      \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s3),                      \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s4),                      \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s5),                      \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s6),                      \
                               SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s7) );                    \
    }                                                                       \
    __rettype##16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(__func, _Sat_v16##__abbrrettype##_v16##__abbrargtype, _sat_R##__rettype##16)( __argtype##16 x ) {    \
        return (__rettype##16)( SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s0),                     \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s1),                     \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s2),                     \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s3),                     \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s4),                     \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s5),                     \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s6),                     \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s7),                     \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s8),                     \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.s9),                     \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.sa),                     \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.sb),                     \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.sc),                     \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.sd),                     \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.se),                     \
                                SPIRV_BUILTIN(__func, _Sat_##__abbrrettype##_##__abbrargtype, _sat_R##__rettype)(x.sf) );                   \
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

// atomics

#define SEMANTICS_PRE_OP_NEED_FENCE ( Release | AcquireRelease | SequentiallyConsistent)

#define SEMANTICS_POST_OP_NEEDS_FENCE ( Acquire | AcquireRelease | SequentiallyConsistent)

#define SPINLOCK_START(addr_space) \
  { \
  volatile bool done = false; \
  while(!done) { \
       __builtin_IB_eu_thread_pause(32); \
       if(atomic_cmpxchg(__builtin_IB_get_##addr_space##_lock(), 0, 1) == 0) {

#define SPINLOCK_END(addr_space) \
            done = true; \
            atomic_store(__builtin_IB_get_##addr_space##_lock(), 0); \
  }}}

#define FENCE_PRE_OP(Scope, Semantics, isGlobal)                                      \
  if( ( (Semantics) & ( SEMANTICS_PRE_OP_NEED_FENCE ) ) > 0 )                         \
  {                                                                                   \
      bool flushL3 = (isGlobal) && ((Scope) == Device || (Scope) == CrossDevice);     \
      __intel_memfence_handler(flushL3, isGlobal, false);                             \
  }

#define FENCE_POST_OP(Scope, Semantics, isGlobal)                                     \
  if( ( (Semantics) & ( SEMANTICS_POST_OP_NEEDS_FENCE ) ) > 0 )                       \
  {                                                                                   \
      bool flushL3 = (isGlobal) && ((Scope) == Device || (Scope) == CrossDevice);     \
      __intel_memfence_handler(flushL3, isGlobal, false);                             \
  }

// This fencing scheme allows us to obey the memory model when coherency is
// enabled or disabled.  Because the L3$ has 2 pipelines (cohereny&atomics and
// non-coherant) the fences guarentee the memory model is followed when coherency
// is disabled.
//
// When coherency is enabled, though, all HDC traffic uses the same L3$ pipe so
// these fences would not be needed.  The compiler is agnostic to coherency
// being enabled or disbled so we asume the worst case.

  #define atomic_operation_1op( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value, isGlobal )   \
{                                                                                             \
    FENCE_PRE_OP((Scope), (Semantics), isGlobal)                                              \
    TYPE result = INTRINSIC( (Pointer), (Value) );                                            \
    FENCE_POST_OP((Scope), (Semantics), isGlobal)                                             \
    return result;                                                                            \
}

#define atomic_operation_1op_as_float( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value, isGlobal )\
{                                                                                             \
    FENCE_PRE_OP((Scope), (Semantics), isGlobal)                                              \
    TYPE result = as_float(INTRINSIC( (Pointer), (Value) ));                                  \
    FENCE_POST_OP((Scope), (Semantics), isGlobal)                                             \
    return result;                                                                            \
}

#define atomic_operation_1op_as_double( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value, isGlobal )\
{                                                                                             \
    FENCE_PRE_OP((Scope), (Semantics), isGlobal)                                              \
    TYPE result = as_double(INTRINSIC( (Pointer), (Value) ));                                  \
    FENCE_POST_OP((Scope), (Semantics), isGlobal)                                             \
    return result;                                                                            \
}

#define atomic_operation_1op_as_half( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value, isGlobal )\
{                                                                                             \
    FENCE_PRE_OP((Scope), (Semantics), isGlobal)                                              \
    TYPE result = as_half(INTRINSIC( (Pointer), (Value) ));                                  \
    FENCE_POST_OP((Scope), (Semantics), isGlobal)                                             \
    return result;                                                                            \
}

#define atomic_operation_0op( INTRINSIC, TYPE, Pointer, Scope, Semantics, isGlobal )          \
{                                                                                             \
    FENCE_PRE_OP((Scope), (Semantics), isGlobal)                                              \
    TYPE result = INTRINSIC( (Pointer) );                                                     \
    FENCE_POST_OP((Scope), (Semantics), isGlobal)                                             \
    return result;                                                                            \
}

#define atomic_cmpxhg( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value, Comp, isGlobal )\
{                                                                                         \
    FENCE_PRE_OP((Scope), (Semantics), isGlobal)                                          \
    TYPE result = INTRINSIC( (Pointer), (Comp), (Value) );                                \
    FENCE_POST_OP((Scope), (Semantics), isGlobal)                                         \
    return result;                                                                        \
}

#define atomic_cmpxhg_as_float( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value, Comp, isGlobal )\
{                                                                                         \
    FENCE_PRE_OP((Scope), (Semantics), isGlobal)                                          \
    TYPE result = as_float(INTRINSIC( (Pointer), (Comp), (Value) ));                      \
    FENCE_POST_OP((Scope), (Semantics), isGlobal)                                         \
    return result;                                                                        \
}

// conversions

#if defined(cl_khr_fp16)
#ifdef __IGC_BUILD__
#define SAT_CLAMP_HELPER_SIGN(TO, FROM, TONAME, TOA, INTTYPE, FROMA)        \
static TO clamp_sat_##TO##_##FROM(TO _R, FROM _T)                           \
{                                                                           \
  _R = __builtin_spirv_OpenCL_select_##TOA##_##TOA##_##INTTYPE(_R, (TO)TONAME##_MIN, SPIRV_BUILTIN(ConvertFToS, _##FROMA##_##TOA, _R##TO)((FROM)(_T < (FROM)TONAME##_MIN)));  \
  _R = __builtin_spirv_OpenCL_select_##TOA##_##TOA##_##INTTYPE(_R, (TO)TONAME##_MAX, SPIRV_BUILTIN(ConvertFToS, _##FROMA##_##TOA, _R##TO)((FROM)(_T > (FROM)TONAME##_MAX)));  \
  _R = __builtin_spirv_OpenCL_select_##TOA##_##TOA##_##INTTYPE(_R, (TO)0,SPIRV_BUILTIN(ConvertFToS, _##FROMA##_##TOA, _R##TO)((FROM)__builtin_spirv_OpIsNan##_##TOA(_T))); \
  return _R;                                                                \
}

#define SAT_CLAMP_HELPER_UNSIGNED(TO, FROM, TONAME, TOA, INTTYPE, FROMA)    \
static TO clamp_sat_##TO##_##FROM(TO _R, FROM _T)                           \
{                                                                           \
  _R = __builtin_spirv_OpenCL_select_##TOA##_##TOA##_##INTTYPE(_R, (TO)TONAME##_MIN, SPIRV_BUILTIN(ConvertFToU, _##FROMA##_##TOA, _R##TO)((FROM)(_T < (FROM)TONAME##_MIN)));  \
  _R = __builtin_spirv_OpenCL_select_##TOA##_##TOA##_##INTTYPE(_R, (TO)TONAME##_MAX, SPIRV_BUILTIN(ConvertFToU, _##FROMA##_##TOA, _R##TO)((FROM)(_T > (FROM)TONAME##_MAX)));  \
  _R = __builtin_spirv_OpenCL_select_##TOA##_##TOA##_##INTTYPE(_R, (TO)0, SPIRV_BUILTIN(ConvertFToU, _##FROMA##_##TOA, _R##TO)((FROM)__builtin_spirv_OpIsNan##_##TOA(_T))); \
  return _R;                                                                \
}
#endif
#endif

// group

#define ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, type)                         \
{                                                                                                    \
    if ( Stride == 0 )                                                                                \
    {                                                                                                \
        ASYNC_WORK_GROUP_COPY(Destination, Source, NumElements, Event, type)                        \
        return Event;                                                                                \
    }                                                                                                \
    else                                                                                            \
    {                                                                                                \
        ASYNC_WORK_GROUP_STRIDED_COPY_L2G(Destination, Source, NumElements, Stride, Event, type)    \
        return Event;                                                                                \
    }                                                                                                \
}

#define ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, type)                         \
{                                                                                                    \
    if ( Stride == 0 )                                                                                \
    {                                                                                                \
        ASYNC_WORK_GROUP_COPY(Destination, Source, NumElements, Event, type)                        \
        return Event;                                                                                \
    }                                                                                                \
    else                                                                                            \
    {                                                                                                \
        ASYNC_WORK_GROUP_STRIDED_COPY_G2L(Destination, Source, NumElements, Stride, Event, type)    \
        return Event;                                                                                \
    }                                                                                                \
}

#if defined(cl_khr_subgroup_non_uniform_vote)
#define DEFN_NON_UNIFORM_ALL_EQUAL(TYPE, TYPE_ABBR)                                                                         \
bool __builtin_spirv_OpGroupNonUniformAllEqual_i32_##TYPE_ABBR(uint Execution, TYPE Value)                                  \
{                                                                                                                           \
    if (Execution == Subgroup)                                                                                              \
    {                                                                                                                       \
        uint activeChannels = __builtin_IB_WaveBallot(true);                                                                \
        uint firstActive = __builtin_spirv_OpenCL_ctz_i32(activeChannels);                                                  \
                                                                                                                            \
        TYPE firstLaneValue = __builtin_spirv_OpGroupBroadcast_i32_##TYPE_ABBR##_i32(Execution, Value, firstActive);        \
        bool isSame = firstLaneValue == Value;                                                                              \
                                                                                                                            \
        uint4 equalChannels = __builtin_spirv_OpGroupNonUniformBallot_i32_i1(Execution, isSame);                            \
                                                                                                                            \
        if (equalChannels.x == activeChannels)                                                                              \
            return true;                                                                                                    \
        else                                                                                                                \
            return false;                                                                                                   \
    }                                                                                                                       \
    else                                                                                                                    \
        return false;                                                                                                       \
}
#endif

#define BROADCAST_WORKGROUP(type)                                                       \
{                                                                                       \
    GET_MEMPOOL_PTR(tmp, type, false, 1)                                                       \
    if( (__intel_LocalInvocationId(0) == LocalId.s0) &                                            \
        (__intel_LocalInvocationId(1) == LocalId.s1) &                                            \
        (__intel_LocalInvocationId(2) == LocalId.s2) )                                            \
    {                                                                                   \
        *tmp = Value;                                                                   \
    }                                                                                   \
    __builtin_spirv_OpControlBarrier_i32_i32_i32(Execution, 0, AcquireRelease | WorkgroupMemory);        \
    type ret = *tmp;                                                                    \
    __builtin_spirv_OpControlBarrier_i32_i32_i32(Execution, 0, AcquireRelease | WorkgroupMemory);        \
    return ret;                                                                         \
}

#define DEFN_SUB_GROUP_BROADCAST_VEC(__vargtype, __abbrvargtype)                                                                          \
GENERATE_VECTOR_FUNCTIONS_3ARGS_SVS(__builtin_spirv_OpGroupBroadcast, __vargtype, uint, __vargtype, uint3, i32, __abbrvargtype, v3i32)    \
GENERATE_VECTOR_FUNCTIONS_3ARGS_SVS(__builtin_spirv_OpGroupBroadcast, __vargtype, uint, __vargtype, ulong3, i32, __abbrvargtype, v3i64)   \
GENERATE_VECTOR_FUNCTIONS_3ARGS_SVS(__builtin_spirv_OpGroupBroadcast, __vargtype, uint, __vargtype, uint2, i32, __abbrvargtype, v2i32)    \
GENERATE_VECTOR_FUNCTIONS_3ARGS_SVS(__builtin_spirv_OpGroupBroadcast, __vargtype, uint, __vargtype, ulong2, i32, __abbrvargtype, v2i64)   \
GENERATE_VECTOR_FUNCTIONS_3ARGS_SVS(__builtin_spirv_OpGroupBroadcast, __vargtype, uint, __vargtype, uint, i32, __abbrvargtype, i32)       \
GENERATE_VECTOR_FUNCTIONS_3ARGS_SVS(__builtin_spirv_OpGroupBroadcast, __vargtype, uint, __vargtype, ulong, i32, __abbrvargtype, i64)

#if defined(cl_khr_subgroup_ballot)
#define DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE, TYPE_ABBR)                                                             \
TYPE __builtin_spirv_OpGroupNonUniformBroadcast_i32_##TYPE_ABBR##_i32(uint Execution, TYPE Value, uint Id)           \
{                                                                                                                    \
    return __builtin_spirv_OpGroupBroadcast_i32_##TYPE_ABBR##_i32(Execution, Value, Id);                             \
}                                                                                                                    \
TYPE __builtin_spirv_OpGroupNonUniformBroadcastFirst_i32_##TYPE_ABBR(uint Execution, TYPE Value)                     \
{                                                                                                                    \
    uint activeChannels = __builtin_IB_WaveBallot(true);                                                             \
    uint firstActive = __builtin_spirv_OpenCL_ctz_i32(activeChannels);                                               \
    return __builtin_spirv_OpGroupBroadcast_i32_##TYPE_ABBR##_i32(Execution, Value, firstActive);                    \
}

#define DEFN_NON_UNIFORM_BROADCAST(TYPE, TYPE_ABBR)             \
    DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE, TYPE_ABBR)            \
    DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE##2, v2##TYPE_ABBR)     \
    DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE##3, v3##TYPE_ABBR)     \
    DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE##4, v4##TYPE_ABBR)     \
    DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE##8, v8##TYPE_ABBR)     \
    DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE##16, v16##TYPE_ABBR)
#endif

#define DEFN_SUPPORTED_OPERATION(op_name, op, type) \
static type    OVERLOADABLE __intel_##op_name(type lhs, type rhs) { return lhs op rhs; }

#define DEFN_BINARY_OPERATIONS(type)    \
DEFN_SUPPORTED_OPERATION(and, &, type)  \
DEFN_SUPPORTED_OPERATION(or,  |, type)  \
DEFN_SUPPORTED_OPERATION(xor, ^, type)

#define DEFN_ARITH_OPERATIONS(type)    \
DEFN_SUPPORTED_OPERATION(mul, *, type) \
DEFN_SUPPORTED_OPERATION(add, +, type)

#define DEFN_WORK_GROUP_REDUCE(type, op, identity, X)                                       \
{                                                                                          \
    GET_MEMPOOL_PTR(data, type, true, 0)                                                     \
    uint lid = __intel_LocalInvocationIndex();                                        \
    uint lsize = __intel_WorkgroupSize();                                                 \
    data[lid] = X;                                                                       \
    __builtin_spirv_OpControlBarrier_i32_i32_i32(Execution, 0, AcquireRelease | WorkgroupMemory);         \
    uint mask = 1 << ( ((8 * sizeof(uint)) - __builtin_spirv_OpenCL_clz_i32(lsize - 1)) - 1) ;  \
    while( mask > 0 )                                                                    \
    {                                                                                    \
        uint c = lid ^ mask;                                                             \
        type other = ( c < lsize ) ? data[ c ] : identity;                                  \
        X = op( other, X );                                                                 \
        __builtin_spirv_OpControlBarrier_i32_i32_i32(Workgroup, 0, AcquireRelease | WorkgroupMemory);     \
        data[lid] = X;                                                              \
        __builtin_spirv_OpControlBarrier_i32_i32_i32(Workgroup, 0, AcquireRelease | WorkgroupMemory);     \
        mask >>= 1;                                                                      \
    }                                                                                    \
    type ret = data[0];                                                                  \
    __builtin_spirv_OpControlBarrier_i32_i32_i32(Workgroup, 0, AcquireRelease | WorkgroupMemory);         \
    return ret;                                                                           \
}


#define DEFN_WORK_GROUP_SCAN_INCL(type, op, identity, X)                                   \
{                                                                                          \
    GET_MEMPOOL_PTR(data, type, true, 0)                                                     \
    uint lid = __intel_LocalInvocationIndex();                                         \
    uint lsize = __intel_WorkgroupSize();                                                 \
    data[lid] = X;                                                                       \
    __builtin_spirv_OpControlBarrier_i32_i32_i32(Workgroup, 0,AcquireRelease |  WorkgroupMemory);         \
    uint offset = 1;                                                                      \
    while( offset < lsize )                                                              \
    {                                                                                    \
        type other = ( lid >= offset ) ?                                                  \
                     data[ lid - offset ] :                                               \
                     identity;                                                            \
        X = op( X, other );                                                              \
        __builtin_spirv_OpControlBarrier_i32_i32_i32(Workgroup, 0, AcquireRelease | WorkgroupMemory);     \
        data[lid] = X;                                                                   \
        __builtin_spirv_OpControlBarrier_i32_i32_i32(Workgroup, 0, AcquireRelease | WorkgroupMemory);     \
        offset <<= 1;                                                                     \
    }                                                                                    \
    return X;                                                                            \
}


#define DEFN_WORK_GROUP_SCAN_EXCL(type, op, identity, X)                                   \
{                                                                                         \
    GET_MEMPOOL_PTR(data, type, true, 1)                                                 \
    uint lid = __intel_LocalInvocationIndex();                                         \
    uint lsize = __intel_WorkgroupSize();                                                 \
    data[0] = identity;                                                                  \
    data[lid + 1] = X;                                                                   \
    __builtin_spirv_OpControlBarrier_i32_i32_i32(Workgroup, 0, AcquireRelease | WorkgroupMemory);         \
    X = data[lid];                                                                       \
    uint offset = 1;                                                                      \
    while( offset < lsize )                                                              \
    {                                                                                    \
        type other = ( lid >= offset ) ?                                                  \
                     data[ lid - offset ] :                                               \
                     identity;                                                            \
        X = op( X, other );                                                              \
        __builtin_spirv_OpControlBarrier_i32_i32_i32(Workgroup, 0, AcquireRelease | WorkgroupMemory);       \
        data[lid] = X;                                                                   \
        __builtin_spirv_OpControlBarrier_i32_i32_i32(Workgroup, 0, AcquireRelease | WorkgroupMemory);       \
        offset <<= 1;                                                                    \
    }                                                                                    \
    return X;                                                                            \
}

#define DEFN_SUB_GROUP_REDUCE(type, type_abbr, op, identity, X)                             \
{                                                                                         \
    uint sgsize = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )();                                 \
    if(sgsize == 8)    \
    {    \
        X = op((type)intel_sub_group_shuffle( X, 0 ),    \
                op((type)intel_sub_group_shuffle( X, 1 ),    \
                op((type)intel_sub_group_shuffle( X, 2 ),    \
                op((type)intel_sub_group_shuffle( X, 3 ),    \
                op((type)intel_sub_group_shuffle( X, 4 ),    \
                op((type)intel_sub_group_shuffle( X, 5 ),    \
                op((type)intel_sub_group_shuffle( X, 6 ),(type)intel_sub_group_shuffle( X, 7 )    \
                )))))));    \
    }    \
    else if(sgsize == 16)    \
    {    \
        X = op((type)intel_sub_group_shuffle( X, 0 ),    \
                op((type)intel_sub_group_shuffle( X, 1 ),    \
                op((type)intel_sub_group_shuffle( X, 2 ),    \
                op((type)intel_sub_group_shuffle( X, 3 ),    \
                op((type)intel_sub_group_shuffle( X, 4 ),    \
                op((type)intel_sub_group_shuffle( X, 5 ),    \
                op((type)intel_sub_group_shuffle( X, 6 ),    \
                op((type)intel_sub_group_shuffle( X, 7 ),    \
                op((type)intel_sub_group_shuffle( X, 8 ),    \
                op((type)intel_sub_group_shuffle( X, 9 ),    \
                op((type)intel_sub_group_shuffle( X, 10 ),    \
                op((type)intel_sub_group_shuffle( X, 11 ),    \
                op((type)intel_sub_group_shuffle( X, 12 ),    \
                op((type)intel_sub_group_shuffle( X, 13 ),    \
                op((type)intel_sub_group_shuffle( X, 14 ),(type)intel_sub_group_shuffle( X, 15 )    \
                )))))))))))))));    \
    }    \
    else if(sgsize == 32)    \
    {    \
        X = op((type)intel_sub_group_shuffle( X, 0 ),    \
                op((type)intel_sub_group_shuffle( X, 1 ),    \
                op((type)intel_sub_group_shuffle( X, 2 ),    \
                op((type)intel_sub_group_shuffle( X, 3 ),    \
                op((type)intel_sub_group_shuffle( X, 4 ),    \
                op((type)intel_sub_group_shuffle( X, 5 ),    \
                op((type)intel_sub_group_shuffle( X, 6 ),    \
                op((type)intel_sub_group_shuffle( X, 7 ),    \
                op((type)intel_sub_group_shuffle( X, 8 ),    \
                op((type)intel_sub_group_shuffle( X, 9 ),    \
                op((type)intel_sub_group_shuffle( X, 10 ),    \
                op((type)intel_sub_group_shuffle( X, 11 ),    \
                op((type)intel_sub_group_shuffle( X, 12 ),    \
                op((type)intel_sub_group_shuffle( X, 13 ),    \
                op((type)intel_sub_group_shuffle( X, 14 ),    \
                op((type)intel_sub_group_shuffle( X, 15 ),    \
                op((type)intel_sub_group_shuffle( X, 16 ),    \
                op((type)intel_sub_group_shuffle( X, 17 ),    \
                op((type)intel_sub_group_shuffle( X, 18 ),    \
                op((type)intel_sub_group_shuffle( X, 19 ),    \
                op((type)intel_sub_group_shuffle( X, 20 ),    \
                op((type)intel_sub_group_shuffle( X, 21 ),    \
                op((type)intel_sub_group_shuffle( X, 22 ),    \
                op((type)intel_sub_group_shuffle( X, 23 ),    \
                op((type)intel_sub_group_shuffle( X, 24 ),    \
                op((type)intel_sub_group_shuffle( X, 25 ),    \
                op((type)intel_sub_group_shuffle( X, 26 ),    \
                op((type)intel_sub_group_shuffle( X, 27 ),    \
                op((type)intel_sub_group_shuffle( X, 28 ),    \
                op((type)intel_sub_group_shuffle( X, 29 ),    \
                op((type)intel_sub_group_shuffle( X, 30 ),(type)intel_sub_group_shuffle( X, 31 )    \
                )))))))))))))))))))))))))))))));    \
    }    \
    else    \
    {    \
        uint sglid = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();                     \
        uint mask = 1 << ( ((8 * sizeof(uint)) - __builtin_spirv_OpenCL_clz_i32(sgsize - 1)) - 1 ); \
        while( mask > 0 )                                                                    \
        {                                                                                    \
            uint c = sglid ^ mask;                                                            \
            type other = ( c < sgsize ) ?                                                      \
                            intel_sub_group_shuffle( X, c ):                                       \
                            identity;                                                            \
            X = op( other, X );                                                                 \
            mask >>= 1;                                                                      \
        }    \
    }    \
    uint3 vec3;                                                                              \
    vec3.s0 = 0;                                                                           \
    return __builtin_spirv_OpGroupBroadcast_i32_##type_abbr##_v3i32(Subgroup, X, vec3 ); \
}


#define DEFN_SUB_GROUP_SCAN_INCL(type, type_abbr, op, identity, X)                        \
{                                                                                         \
    uint sgsize = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )();                                 \
    uint offset = 1;                                                                      \
    while( offset < sgsize )                                                             \
    {                                                                                    \
        type other = intel_sub_group_shuffle_up( (type)identity, X, offset );              \
        X = op( X, other );                                                              \
        offset <<= 1;                                                                    \
    }                                                                                    \
    return X;                                                                            \
}

#define DEFN_SUB_GROUP_SCAN_EXCL(type, type_abbr, op, identity, X)                        \
{                                                                                         \
    uint sgsize = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )();                                 \
    X = intel_sub_group_shuffle_up( (type)identity, X, 1 );                              \
    uint offset = 1;                                                                      \
    while( offset < sgsize )                                                             \
    {                                                                                    \
        type other = intel_sub_group_shuffle_up( (type)identity, X, offset );              \
        X = op( X, other );                                                              \
        offset <<= 1;                                                                    \
    }                                                                                    \
    return X;                                                                            \
}

#define WORK_GROUP_SWITCH(type, op, identity, X, Operation)                                 \
{                                                                                         \
    switch(Operation){                                                                     \
        case GroupOperationReduce:                                                         \
            DEFN_WORK_GROUP_REDUCE(type, op, identity, X)                                 \
            break;                                                                         \
        case GroupOperationInclusiveScan:                                                 \
            DEFN_WORK_GROUP_SCAN_INCL(type, op, identity, X)                             \
            break;                                                                         \
        case GroupOperationExclusiveScan:                                                 \
            DEFN_WORK_GROUP_SCAN_EXCL(type, op, identity, X)                             \
            break;                                                                         \
        default:                                                                         \
            return 0;                                                                    \
            break;                                                                         \
    }                                                                                     \
}

#define SUB_GROUP_SWITCH(type, type_abbr, op, identity, X, Operation)                     \
{                                                                                         \
    switch(Operation){                                                                     \
        case GroupOperationReduce:                                                         \
            DEFN_SUB_GROUP_REDUCE(type, type_abbr, op, identity, X)                         \
            break;                                                                         \
        case GroupOperationInclusiveScan:                                                 \
            DEFN_SUB_GROUP_SCAN_INCL(type, type_abbr, op, identity, X)                     \
            break;                                                                         \
        case GroupOperationExclusiveScan:                                                 \
            DEFN_SUB_GROUP_SCAN_EXCL(type, type_abbr, op, identity, X)                     \
            break;                                                                         \
        default:                                                                         \
            return 0;                                                                     \
            break;                                                                         \
    }                                                                                     \
}

#define DEFN_UNIFORM_GROUP_FUNC(func, type, type_abbr, op, identity)                             \
type  __builtin_spirv_OpGroup##func##_i32_i32_##type_abbr(uint Execution, uint Operation, type X)       \
{                                                                                                \
    if (Execution == Workgroup)                                                                  \
    {                                                                                            \
        WORK_GROUP_SWITCH(type, op, identity, X, Operation)                                      \
    }                                                                                            \
    else if (Execution == Subgroup)                                                              \
    {                                                                                             \
        if (sizeof(X) < 8 || __UseNative64BitSubgroupBuiltin)                                            \
        {                                                                                        \
            if (Operation == GroupOperationReduce)                                                     \
            {                                                                                         \
                return __builtin_IB_sub_group_reduce_##func##_##type_abbr(X);                         \
            }                                                                                         \
            else if (Operation == GroupOperationInclusiveScan)                                         \
            {                                                                                         \
                return op(X, __builtin_IB_sub_group_scan_##func##_##type_abbr(X));                     \
            }                                                                                         \
            else if (Operation == GroupOperationExclusiveScan)                                         \
            {                                                                                         \
                return __builtin_IB_sub_group_scan_##func##_##type_abbr(X);                           \
            }                                                                                         \
        }                                                                                         \
        else {                                                                                     \
            SUB_GROUP_SWITCH(type, type_abbr, op, identity, X, Operation)                         \
        }                                                                                         \
        return 0;                                                                                 \
    }                                                                                            \
    else                                                                                         \
    {                                                                                            \
        return 0;                                                                                \
    }                                                                                            \
}

#if defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)

#define DEFN_SUB_GROUP_REDUCE_NON_UNIFORM(type, type_abbr, op, identity, X)                         \
{                                                                                                   \
    uint activeChannels = __builtin_IB_WaveBallot(true);                                            \
    uint firstActive = __builtin_spirv_OpenCL_ctz_i32(activeChannels);                              \
                                                                                                    \
    type result = identity;                                                                         \
    while (activeChannels)                                                                          \
    {                                                                                               \
        uint activeId = __builtin_spirv_OpenCL_ctz_i32(activeChannels);                             \
                                                                                                    \
        type value = intel_sub_group_shuffle(X, activeId);                                          \
        result = op(value, result);                                                                 \
                                                                                                    \
        uint disable = 1 << activeId;                                                               \
        activeChannels ^= disable;                                                                  \
    }                                                                                               \
                                                                                                    \
    uint3 vec3;                                                                                     \
    vec3.s0 = firstActive;                                                                          \
    X = __builtin_spirv_OpGroupBroadcast_i32_##type_abbr##_v3i32(Subgroup, result, vec3);           \
}

#define DEFN_SUB_GROUP_SCAN_INCL_NON_UNIFORM(type, type_abbr, op, identity, X)                      \
{                                                                                                   \
    uint sglid = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();                                \
    uint activeChannels = __builtin_IB_WaveBallot(true);                                            \
    uint activeId = __builtin_spirv_OpenCL_ctz_i32(activeChannels);                                 \
    activeChannels ^= 1 << activeId;                                                                \
    while (activeChannels)                                                                          \
    {                                                                                               \
        type value = intel_sub_group_shuffle(X, activeId);                                          \
        activeId = __builtin_spirv_OpenCL_ctz_i32(activeChannels);                                  \
        if (sglid == activeId)                                                                      \
            X = op(value, X);                                                                       \
        activeChannels ^= 1 << activeId;                                                            \
    }                                                                                               \
}

#define DEFN_SUB_GROUP_SCAN_EXCL_NON_UNIFORM(type, type_abbr, op, identity, X)                       \
{                                                                                                    \
    uint sglid = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();                                 \
    uint activeChannels = __builtin_IB_WaveBallot(true);                                             \
    type result = identity;                                                                          \
    while (activeChannels)                                                                           \
    {                                                                                                \
        uint activeId = __builtin_spirv_OpenCL_ctz_i32(activeChannels);                              \
        if (sglid == activeId)                                                                       \
        {                                                                                            \
            type value = X;                                                                          \
            X = result;                                                                              \
            result = op(result, value);                                                              \
        }                                                                                            \
        result = sub_group_shuffle(result, activeId);                                                \
        activeChannels ^= 1 << activeId;                                                             \
    }                                                                                                \
}

#define DEFN_SUB_GROUP_CLUSTERED_REDUCE(type, type_abbr, op, identity, X, ClusterSize)                         \
{                                                                                                              \
    uint clusterIndex = 0;                                                                                     \
    uint activeChannels = __builtin_IB_WaveBallot(true);                                                       \
    uint numActive = __builtin_spirv_OpenCL_popcount_i32(activeChannels);                                      \
    uint numClusters = numActive / ClusterSize;                                                                \
                                                                                                               \
    for (uint clusterIndex = 0; clusterIndex < numClusters; clusterIndex++)                                    \
    {                                                                                                          \
        uint Counter = ClusterSize;                                                                            \
        uint Ballot = activeChannels;                                                                          \
        uint clusterBallot = 0;                                                                                \
        while (Counter--)                                                                                      \
        {                                                                                                      \
            uint trailingOne = 1 << __builtin_spirv_OpenCL_ctz_i32(Ballot);                                    \
            clusterBallot |= trailingOne;                                                                      \
            Ballot ^= trailingOne;                                                                             \
        }                                                                                                      \
        uint active = __builtin_spirv_OpGroupNonUniformInverseBallot_i32_v4i32(Subgroup, clusterBallot);       \
        if (active)                                                                                            \
        {                                                                                                      \
            DEFN_SUB_GROUP_REDUCE_NON_UNIFORM(type, type_abbr, op, identity, X)                                \
        }                                                                                                      \
        activeChannels ^= clusterBallot;                                                                       \
    }                                                                                                          \
}

#define SUB_GROUP_SWITCH_NON_UNIFORM(type, type_abbr, op, identity, X, Operation, ClusterSize) \
{                                                                                              \
    switch (Operation){                                                                        \
        case GroupOperationReduce:                                                             \
            DEFN_SUB_GROUP_REDUCE_NON_UNIFORM(type, type_abbr, op, identity, X)                \
            break;                                                                             \
        case GroupOperationInclusiveScan:                                                      \
            DEFN_SUB_GROUP_SCAN_INCL_NON_UNIFORM(type, type_abbr, op, identity, X)             \
            break;                                                                             \
        case GroupOperationExclusiveScan:                                                      \
            DEFN_SUB_GROUP_SCAN_EXCL_NON_UNIFORM(type, type_abbr, op, identity, X)             \
            break;                                                                             \
        case GroupOperationClusteredReduce:                                                    \
            DEFN_SUB_GROUP_CLUSTERED_REDUCE(type, type_abbr, op, identity, X, ClusterSize)     \
            break;                                                                             \
        default:                                                                               \
            return 0;                                                                          \
            break;                                                                             \
    }                                                                                          \
}

// ClusterSize is an optional parameter
#define DEFN_NON_UNIFORM_GROUP_FUNC(func, type, type_abbr, op, identity)                                                             \
type  __builtin_spirv_OpGroupNonUniform##func##_i32_i32_##type_abbr##_i32(uint Execution, uint Operation, type X, uint ClusterSize)  \
{                                                                                                                                    \
    if (Execution == Subgroup)                                                                                                       \
    {                                                                                                                                \
        if (sizeof(X) < 8 || __UseNative64BitSubgroupBuiltin)                                                                        \
        {                                                                                                                            \
            if (Operation == GroupOperationReduce)                                                                                   \
            {                                                                                                                        \
                return __builtin_IB_sub_group_reduce_##func##_##type_abbr(X);                                                        \
            }                                                                                                                        \
            else if (Operation == GroupOperationInclusiveScan)                                                                       \
            {                                                                                                                        \
                return op(X, __builtin_IB_sub_group_scan_##func##_##type_abbr(X));                                                   \
            }                                                                                                                        \
            else if (Operation == GroupOperationExclusiveScan)                                                                       \
            {                                                                                                                        \
                return __builtin_IB_sub_group_scan_##func##_##type_abbr(X);                                                          \
            }                                                                                                                        \
            else if (Operation == GroupOperationClusteredReduce)                                                                     \
            {                                                                                                                        \
                return __builtin_IB_sub_group_clustered_reduce_##func##_##type_abbr(X, ClusterSize);                                 \
            }                                                                                                                        \
        }                                                                                                                            \
        else {                                                                                                                       \
            SUB_GROUP_SWITCH_NON_UNIFORM(type, type_abbr, op, identity, X, Operation, ClusterSize)                                   \
            return X;                                                                                                                \
        }                                                                                                                            \
        return 0;                                                                                                                    \
    }                                                                                                                                \
    else                                                                                                                             \
    {                                                                                                                                \
        return 0;                                                                                                                    \
    }                                                                                                                                \
}                                                                                                                                    \
type  __builtin_spirv_OpGroupNonUniform##func##_i32_i32_##type_abbr(uint Execution, uint Operation, type X)                          \
{                                                                                                                                    \
    return __builtin_spirv_OpGroupNonUniform##func##_i32_i32_##type_abbr##_i32(Execution, Operation, X, 0);                          \
}

#endif

#if defined(cl_khr_subgroup_shuffle)
#define DEFN_SUB_GROUP_SHUFFLE_XOR(TYPE, TYPE_ABBR)                                                             \
TYPE __builtin_spirv_OpGroupNonUniformShuffleXor_i32_##TYPE_ABBR##_i32(uint Execution, TYPE x, uint c)          \
{                                                                                                               \
    c = get_sub_group_local_id() ^ c;                                                                           \
    return __builtin_spirv_OpGroupNonUniformShuffle_i32_##TYPE_ABBR##_i32(Execution, x, c);                     \
}
#endif

#if defined(cl_khr_subgroup_shuffle_relative)
#define DEFN_NON_UNIFORM_SHUFFLE_UP(TYPE, TYPE_ABBR)                                                                        \
TYPE __builtin_spirv_OpGroupNonUniformShuffleUp_i32_##TYPE_ABBR##_i32(uint Execution, TYPE x, uint c)                       \
{                                                                                                                           \
    if (Execution == Subgroup)                                                                                              \
    {                                                                                                                       \
        return intel_sub_group_shuffle_up((TYPE) 0, x, c);                                                                  \
    }                                                                                                                       \
    return 0;                                                                                                               \
}
#endif

// vloadvstore

#define VLOAD_MACRO(addressSpace, scalarType, numElements, offsetType, mangle)                                                                                \
INLINE scalarType##numElements __builtin_spirv_OpenCL_vload##numElements##_##mangle(offsetType offset, const addressSpace scalarType *p)                             \
{                                                                                                                                                             \
  const addressSpace scalarType *pOffset = p + offset * numElements;                                                                                          \
  scalarType##numElements ret;                                                                                                                                \
  __builtin_IB_memcpy_##addressSpace##_to_private((private uchar*)&ret, (addressSpace uchar*)pOffset, sizeof(scalarType) * numElements, sizeof(scalarType));  \
  return ret;                                                                                                                                                 \
}

#define VSTORE_MACRO(addressSpace, scalarType, numElements, offsetType, mangle)                                                                               \
INLINE void __builtin_spirv_OpenCL_vstore##numElements##_##mangle(scalarType##numElements data, offsetType offset, addressSpace scalarType *p)                       \
{                                                                                                                                                             \
  addressSpace scalarType *pOffset = p + offset * numElements;                                                                                                \
  scalarType##numElements ret = data;                                                                                                                         \
  __builtin_IB_memcpy_private_to_##addressSpace((addressSpace uchar*)pOffset, (private uchar*)&ret, sizeof(scalarType) * numElements, sizeof(scalarType));    \
}

#define __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, MANGSIZE, SIZETYPE, HASVEC, rnd, MANGSRC, srcType, numElements)                               \
INLINE void __builtin_spirv_OpenCL_vstore_half##numElements##rnd##_##HASVEC##numElements##MANGSRC##_##MANGSIZE##_p##ASNUM##f16(srcType##numElements data, \
                                                           SIZETYPE offset,                                                                        \
                                                           addressSpace half* p) {                                                                 \
  __builtin_spirv_OpenCL_vstore##numElements##_##HASVEC##numElements##f16##_##MANGSIZE##_p##ASNUM##f16(__intel_spirv_##srcType##2half##rnd(data), offset, p);   \
}

#define __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, MANGSIZE, SIZETYPE, HASVEC, rnd, MANGSRC, srcType, step, numElements)                         \
INLINE void __builtin_spirv_OpenCL_vstorea_half##numElements##rnd##_##HASVEC##numElements##MANGSRC##_##MANGSIZE##_p##ASNUM##f16(srcType##numElements data, \
                                                        SIZETYPE offset,                                                                            \
                                                        addressSpace half* p) {                                                                     \
  addressSpace half##numElements *pHalf = (addressSpace half##numElements *)(p + offset * step);                                                    \
  *pHalf = __intel_spirv_##srcType##2half##rnd(data);                                                                                                     \
}
