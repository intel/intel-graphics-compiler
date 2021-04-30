/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
#define ASYNC_WORK_GROUP_COPY(dst, src, num_elements, evt)                  \
    {                                                                       \
        uint uiNumElements = num_elements;                                  \
        uint index = __intel_get_local_linear_id();                         \
        uint step = __intel_get_local_size();                               \
        for( ; index < uiNumElements; index += step ) {                     \
            dst[index] = src[index];                                        \
        }                                                                   \
    }

#define ASYNC_WORK_GROUP_STRIDED_COPY_G2L(dst, src, num_elements, src_stride, evt)  \
    {                                                                       \
        uint uiNumElements = num_elements;                                  \
        uint uiStride = src_stride;                                         \
        uint dstIndex = __intel_get_local_linear_id();                      \
        uint dstStep = __intel_get_local_size();                            \
        uint srcIndex = dstIndex * uiStride;                                \
        uint srcStep = dstStep * uiStride;                                  \
        for( ; dstIndex < uiNumElements;                                    \
             dstIndex += dstStep, srcIndex += srcStep ) {                   \
            dst[dstIndex] = src[srcIndex];                                  \
        }                                                                   \
    }

#define ASYNC_WORK_GROUP_STRIDED_COPY_L2G(dst, src, num_elements, dst_stride, evt)  \
    {                                                                       \
        uint uiNumElements = num_elements;                                  \
        uint uiStride = dst_stride;                                         \
        uint srcIndex = __intel_get_local_linear_id();                      \
        uint srcStep = __intel_get_local_size();                            \
        uint dstIndex = srcIndex * uiStride;                                \
        uint dstStep = srcStep * uiStride;                                  \
        for( ; srcIndex < uiNumElements;                                    \
             dstIndex += dstStep, srcIndex += srcStep ) {                   \
            dst[dstIndex] = src[srcIndex];                                  \
        }                                                                   \
    }

// Use this macro if the vector functions have different behavior than the
// scalar functions.

#define GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( __func, __sfunc, __rettype, __argtype ) \
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
#define GENERATE_VECTOR_FUNCTIONS_1ARG( __func, __rettype, __argtype )      \
    GENERATE_VECTOR_FUNCTIONS_1ARG_EXPLICIT( __func, __func, __rettype, __argtype )

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

#define GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP_SIZE( __func, __rettype, __argtype, __vecSize ) \
    __rettype##__vecSize OVERLOADABLE __func( __argtype##__vecSize x ) {                    \
        __rettype##__vecSize ret;                                                           \
        __argtype argx[__vecSize];                                                          \
        __rettype out[__vecSize];                                                           \
        VECTOARRAY##__vecSize(argx, x);                                                     \
        for(uint i = 0; i < __vecSize; i++) {                                               \
            out[i] = __func(argx[i]);                                                       \
        }                                                                                   \
        ARRAYTOVEC##__vecSize(ret, out);                                                    \
        return ret;                                                                         \
    }

// Use this macro if the scalar function is big.
#define GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __func, __rettype, __argtype )     \
    GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP_SIZE( __func, __rettype, __argtype, 2 ) \
    GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP_SIZE( __func, __rettype, __argtype, 3 ) \
    GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP_SIZE( __func, __rettype, __argtype, 4 ) \
    GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP_SIZE( __func, __rettype, __argtype, 8 ) \
    GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP_SIZE( __func, __rettype, __argtype, 16 )


// TODO: get rid of this #else case when the legalizer can
// fix up illegal types that GVN generates.
#define GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP_SIZE( __func, __rettype, __argtype, __ptrtype, __vecSize )  \
    __rettype##__vecSize OVERLOADABLE __func( __argtype##__vecSize x, __private __ptrtype##__vecSize * y ) {    \
        __rettype##__vecSize ret;                                                                               \
        __argtype argx[__vecSize];                                                                              \
        __rettype out[__vecSize];                                                                               \
        __private __ptrtype* py_scalar = (__private __ptrtype*)y;                                               \
        VECTOARRAY##__vecSize(argx, x);                                                                         \
        for(uint i = 0; i < __vecSize; i++)                                                                     \
        {                                                                                                       \
            out[i] = __func(argx[i], py_scalar + i);                                                            \
        }                                                                                                       \
        ARRAYTOVEC##__vecSize(ret, out);                                                                        \
        return ret;                                                                                             \
    }

#define GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP( __func, __rettype, __argtype, __ptrtype )      \
    GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP_SIZE( __func, __rettype, __argtype, __ptrtype, 2)   \
    GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP_SIZE( __func, __rettype, __argtype, __ptrtype, 3)   \
    GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP_SIZE( __func, __rettype, __argtype, __ptrtype, 4)   \
    GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP_SIZE( __func, __rettype, __argtype, __ptrtype, 8)   \
    GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP_SIZE( __func, __rettype, __argtype, __ptrtype, 16)

#define GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG_EXPLICIT( __func, __sfunc, __rettype, __argtype, __addressspc, __ptrtype ) \
    __rettype##2 OVERLOADABLE __func( __argtype##2 x, __addressspc __ptrtype##2 * y ) {                                      \
        __ptrtype##2 a, b;                                                                                                   \
        a = __sfunc(x, &b);                                                                                                  \
        y[0] = b;                                                                                                            \
        return a;                                                                                                            \
    }                                                                                                                        \
    __rettype##3 OVERLOADABLE __func( __argtype##3 x, __addressspc __ptrtype##3 * y ) {                                      \
        __ptrtype##3 a, b;                                                                                                   \
        a = __sfunc(x, &b);                                                                                                  \
        y[0] = b;                                                                                                            \
        return a;                                                                                                            \
    }                                                                                                                        \
    __rettype##4 OVERLOADABLE __func( __argtype##4 x, __addressspc __ptrtype##4 * y ) {                                      \
        __ptrtype##4 a, b;                                                                                                   \
        a = __sfunc(x, &b);                                                                                                  \
        y[0] = b;                                                                                                            \
        return a;                                                                                                            \
    }                                                                                                                        \
    __rettype##8 OVERLOADABLE __func( __argtype##8 x, __addressspc __ptrtype##8 * y ) {                                      \
        __ptrtype##8 a, b;                                                                                                   \
        a = __sfunc(x, &b);                                                                                                  \
        y[0] = b;                                                                                                            \
        return a;                                                                                                            \
    }                                                                                                                        \
    __rettype##16 OVERLOADABLE __func( __argtype##16 x, __addressspc __ptrtype##16 * y ) {                                   \
        __ptrtype##16 a, b;                                                                                                  \
        a = __sfunc(x, &b);                                                                                                  \
        y[0] = b;                                                                                                            \
        return a;                                                                                                            \
    }

# define GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __func, __rettype, __addressspc, __argptrtype ) \
    GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG_EXPLICIT( __func, __func, __rettype, __argptrtype, __addressspc, __argptrtype )

#define GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( __func, __sfunc, __rettype, __argtype ) \
    __rettype##2 OVERLOADABLE __func( __argtype##2 x, __argtype##2 y ) {    \
        return (__rettype##2)( __sfunc(x.s0, y.s0),                         \
                               __sfunc(x.s1, y.s1) );                       \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func( __argtype##3 x, __argtype##3 y ) {    \
        return (__rettype##3)( __sfunc(x.s0, y.s0),                         \
                               __sfunc(x.s1, y.s1),                         \
                               __sfunc(x.s2, y.s2) );                       \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func( __argtype##4 x, __argtype##4 y ) {    \
        return (__rettype##4)( __sfunc(x.s0, y.s0),                         \
                               __sfunc(x.s1, y.s1),                         \
                               __sfunc(x.s2, y.s2),                         \
                               __sfunc(x.s3, y.s3) );                       \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func( __argtype##8 x, __argtype##8 y ) {    \
        return (__rettype##8)( __sfunc(x.s0, y.s0),                         \
                               __sfunc(x.s1, y.s1),                         \
                               __sfunc(x.s2, y.s2),                         \
                               __sfunc(x.s3, y.s3),                         \
                               __sfunc(x.s4, y.s4),                         \
                               __sfunc(x.s5, y.s5),                         \
                               __sfunc(x.s6, y.s6),                         \
                               __sfunc(x.s7, y.s7) );                       \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func( __argtype##16 x, __argtype##16 y ) { \
        return (__rettype##16)( __sfunc(x.s0, y.s0),                        \
                                __sfunc(x.s1, y.s1),                        \
                                __sfunc(x.s2, y.s2),                        \
                                __sfunc(x.s3, y.s3),                        \
                                __sfunc(x.s4, y.s4),                        \
                                __sfunc(x.s5, y.s5),                        \
                                __sfunc(x.s6, y.s6),                        \
                                __sfunc(x.s7, y.s7),                        \
                                __sfunc(x.s8, y.s8),                        \
                                __sfunc(x.s9, y.s9),                        \
                                __sfunc(x.sa, y.sa),                        \
                                __sfunc(x.sb, y.sb),                        \
                                __sfunc(x.sc, y.sc),                        \
                                __sfunc(x.sd, y.sd),                        \
                                __sfunc(x.se, y.se),                        \
                                __sfunc(x.sf, y.sf) );                      \
    }

#define GENERATE_VECTOR_FUNCTIONS_2ARGS( __func, __rettype, __argtype )     \
    GENERATE_VECTOR_FUNCTIONS_2ARGS_EXPLICIT( __func, __func, __rettype, __argtype )

#define GENERATE_VECTOR_FUNCTIONS_2ARGS_VV( __func, __rettype, __argtype0, __argtype1 ) \
    __rettype##2 OVERLOADABLE __func( __argtype0##2 x, __argtype1##2 y ) {  \
        return (__rettype##2)( __func(x.s0, y.s0),                          \
                               __func(x.s1, y.s1) );                        \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func( __argtype0##3 x, __argtype1##3 y ) {  \
        return (__rettype##3)( __func(x.s0, y.s0),                          \
                               __func(x.s1, y.s1),                          \
                               __func(x.s2, y.s2) );                        \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func( __argtype0##4 x, __argtype1##4 y ) {  \
        return (__rettype##4)( __func(x.s0, y.s0),                          \
                               __func(x.s1, y.s1),                          \
                               __func(x.s2, y.s2),                          \
                               __func(x.s3, y.s3) );                        \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func( __argtype0##8 x, __argtype1##8 y ) {  \
        return (__rettype##8)( __func(x.s0, y.s0),                          \
                               __func(x.s1, y.s1),                          \
                               __func(x.s2, y.s2),                          \
                               __func(x.s3, y.s3),                          \
                               __func(x.s4, y.s4),                          \
                               __func(x.s5, y.s5),                          \
                               __func(x.s6, y.s6),                          \
                               __func(x.s7, y.s7) );                        \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func( __argtype0##16 x, __argtype1##16 y ) { \
        return (__rettype##16)( __func(x.s0, y.s0),                         \
                                __func(x.s1, y.s1),                         \
                                __func(x.s2, y.s2),                         \
                                __func(x.s3, y.s3),                         \
                                __func(x.s4, y.s4),                         \
                                __func(x.s5, y.s5),                         \
                                __func(x.s6, y.s6),                         \
                                __func(x.s7, y.s7),                         \
                                __func(x.s8, y.s8),                         \
                                __func(x.s9, y.s9),                         \
                                __func(x.sa, y.sa),                         \
                                __func(x.sb, y.sb),                         \
                                __func(x.sc, y.sc),                         \
                                __func(x.sd, y.sd),                         \
                                __func(x.se, y.se),                         \
                                __func(x.sf, y.sf) );                       \
    }

// TODO: get rid of this #else case when the legalizer can
// fix up illegal types that GVN generates.
#define GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP_SIZE( __func, __rettype, __argtype0, __argtype1, __vecSize ) \
    __rettype##__vecSize OVERLOADABLE __func( __argtype0##__vecSize x, __argtype1##__vecSize y ) {           \
        __rettype##__vecSize ret;                                                                            \
        __argtype0 argx[__vecSize];                                                                          \
        __argtype1 argy[__vecSize];                                                                          \
        __rettype  out[__vecSize];                                                                           \
        VECTOARRAY##__vecSize(argx, x);                                                                      \
        VECTOARRAY##__vecSize(argy, y);                                                                      \
        for(uint i = 0; i < __vecSize; i++) {                                                                \
            out[i] = __func(argx[i], argy[i]);                                                               \
        }                                                                                                    \
        ARRAYTOVEC##__vecSize(ret, out);                                                                     \
        return ret;                                                                                          \
    }

#define GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __func, __rettype, __argtype0, __argtype1 )    \
    GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP_SIZE( __func, __rettype, __argtype0, __argtype1, 2) \
    GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP_SIZE( __func, __rettype, __argtype0, __argtype1, 3) \
    GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP_SIZE( __func, __rettype, __argtype0, __argtype1, 4) \
    GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP_SIZE( __func, __rettype, __argtype0, __argtype1, 8) \
    GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP_SIZE( __func, __rettype, __argtype0, __argtype1, 16)


#define GENERATE_VECTOR_FUNCTIONS_2ARGS_VS( __func, __rettype, __vargtype, __sargtype ) \
    __rettype##2 OVERLOADABLE __func( __vargtype##2 x, __sargtype y ) {     \
        return (__rettype##2)( __func(x.s0, y),                             \
                               __func(x.s1, y) );                           \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func( __vargtype##3 x, __sargtype y ) {     \
        return (__rettype##3)( __func(x.s0, y),                             \
                               __func(x.s1, y),                             \
                               __func(x.s2, y) );                           \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func( __vargtype##4 x, __sargtype y ) {     \
        return (__rettype##4)( __func(x.s0, y),                             \
                               __func(x.s1, y),                             \
                               __func(x.s2, y),                             \
                               __func(x.s3, y) );                           \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func( __vargtype##8 x, __sargtype y ) {     \
        return (__rettype##8)( __func(x.s0, y),                             \
                               __func(x.s1, y),                             \
                               __func(x.s2, y),                             \
                               __func(x.s3, y),                             \
                               __func(x.s4, y),                             \
                               __func(x.s5, y),                             \
                               __func(x.s6, y),                             \
                               __func(x.s7, y) );                           \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func( __vargtype##16 x, __sargtype y ) {   \
        return (__rettype##16)( __func(x.s0, y),                            \
                                __func(x.s1, y),                            \
                                __func(x.s2, y),                            \
                                __func(x.s3, y),                            \
                                __func(x.s4, y),                            \
                                __func(x.s5, y),                            \
                                __func(x.s6, y),                            \
                                __func(x.s7, y),                            \
                                __func(x.s8, y),                            \
                                __func(x.s9, y),                            \
                                __func(x.sa, y),                            \
                                __func(x.sb, y),                            \
                                __func(x.sc, y),                            \
                                __func(x.sd, y),                            \
                                __func(x.se, y),                            \
                                __func(x.sf, y) );                          \
    }

#define GENERATE_VECTOR_FUNCTIONS_2ARGS_SV( __func, __rettype, __sargtype, __vargtype ) \
    __rettype##2 OVERLOADABLE __func( __sargtype x, __vargtype##2 y ) {     \
        return (__rettype##2)( __func(x, y.s0),                             \
                               __func(x, y.s1) );                           \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func( __sargtype x, __vargtype##3 y ) {     \
        return (__rettype##3)( __func(x, y.s0),                             \
                               __func(x, y.s1),                             \
                               __func(x, y.s2) );                           \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func( __sargtype x, __vargtype##4 y ) {     \
        return (__rettype##4)( __func(x, y.s0),                             \
                               __func(x, y.s1),                             \
                               __func(x, y.s2),                             \
                               __func(x, y.s3) );                           \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func( __sargtype x, __vargtype##8 y ) {     \
        return (__rettype##8)( __func(x, y.s0),                             \
                               __func(x, y.s1),                             \
                               __func(x, y.s2),                             \
                               __func(x, y.s3),                             \
                               __func(x, y.s4),                             \
                               __func(x, y.s5),                             \
                               __func(x, y.s6),                             \
                               __func(x, y.s7) );                           \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func( __sargtype x, __vargtype##16 y ) {   \
        return (__rettype##16)( __func(x, y.s0),                            \
                                __func(x, y.s1),                            \
                                __func(x, y.s2),                            \
                                __func(x, y.s3),                            \
                                __func(x, y.s4),                            \
                                __func(x, y.s5),                            \
                                __func(x, y.s6),                            \
                                __func(x, y.s7),                            \
                                __func(x, y.s8),                            \
                                __func(x, y.s9),                            \
                                __func(x, y.sa),                            \
                                __func(x, y.sb),                            \
                                __func(x, y.sc),                            \
                                __func(x, y.sd),                            \
                                __func(x, y.se),                            \
                                __func(x, y.sf) );                          \
    }

#define GENERATE_VECTOR_FUNCTIONS_3ARGS( __func, __rettype, __argtype )     \
    GENERATE_VECTOR_FUNCTIONS_3ARGS_EXPLICIT( __func, __func, __rettype, __argtype )

#define GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __sfunc, __rettype, __argtype ) \
    __rettype##2 OVERLOADABLE select( __rettype##2 x, __rettype##2 y, __argtype##2 z ) { \
        return (__rettype##2)( __sfunc(x.s0, y.s0, z.s0),                   \
                               __sfunc(x.s1, y.s1, z.s1) );                 \
    }                                                                       \
    __rettype##3 OVERLOADABLE select( __rettype##3 x, __rettype##3 y, __argtype##3 z ) { \
        return (__rettype##3)( __sfunc(x.s0, y.s0, z.s0),                   \
                               __sfunc(x.s1, y.s1, z.s1),                   \
                               __sfunc(x.s2, y.s2, z.s2) );                 \
    }                                                                       \
    __rettype##4 OVERLOADABLE select( __rettype##4 x, __rettype##4 y, __argtype##4 z ) { \
        return (__rettype##4)( __sfunc(x.s0, y.s0, z.s0),                   \
                               __sfunc(x.s1, y.s1, z.s1),                   \
                               __sfunc(x.s2, y.s2, z.s2),                   \
                               __sfunc(x.s3, y.s3, z.s3) );                 \
    }                                                                       \
    __rettype##8 OVERLOADABLE select( __rettype##8 x, __rettype##8 y, __argtype##8 z ) { \
        return (__rettype##8)( __sfunc(x.s0, y.s0, z.s0),                   \
                               __sfunc(x.s1, y.s1, z.s1),                   \
                               __sfunc(x.s2, y.s2, z.s2),                   \
                               __sfunc(x.s3, y.s3, z.s3),                   \
                               __sfunc(x.s4, y.s4, z.s4),                   \
                               __sfunc(x.s5, y.s5, z.s5),                   \
                               __sfunc(x.s6, y.s6, z.s6),                   \
                               __sfunc(x.s7, y.s7, z.s7) );                 \
    }                                                                       \
    __rettype##16 OVERLOADABLE select( __rettype##16 x, __rettype##16 y, __argtype##16 z ) { \
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

#define GENERATE_VECTOR_FUNCTIONS_3ARGS_EXPLICIT( __func, __sfunc, __rettype, __argtype ) \
    __rettype##2 OVERLOADABLE __func( __argtype##2 x, __argtype##2 y, __argtype##2 z ) { \
        return (__rettype##2)( __sfunc(x.s0, y.s0, z.s0),                   \
                               __sfunc(x.s1, y.s1, z.s1) );                 \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func( __argtype##3 x, __argtype##3 y, __argtype##3 z ) { \
        return (__rettype##3)( __sfunc(x.s0, y.s0, z.s0),                   \
                               __sfunc(x.s1, y.s1, z.s1),                   \
                               __sfunc(x.s2, y.s2, z.s2) );                 \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func( __argtype##4 x, __argtype##4 y, __argtype##4 z ) { \
        return (__rettype##4)( __sfunc(x.s0, y.s0, z.s0),                   \
                               __sfunc(x.s1, y.s1, z.s1),                   \
                               __sfunc(x.s2, y.s2, z.s2),                   \
                               __sfunc(x.s3, y.s3, z.s3) );                 \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func( __argtype##8 x, __argtype##8 y, __argtype##8 z ) { \
        return (__rettype##8)( __sfunc(x.s0, y.s0, z.s0),                   \
                               __sfunc(x.s1, y.s1, z.s1),                   \
                               __sfunc(x.s2, y.s2, z.s2),                   \
                               __sfunc(x.s3, y.s3, z.s3),                   \
                               __sfunc(x.s4, y.s4, z.s4),                   \
                               __sfunc(x.s5, y.s5, z.s5),                   \
                               __sfunc(x.s6, y.s6, z.s6),                   \
                               __sfunc(x.s7, y.s7, z.s7) );                 \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func( __argtype##16 x, __argtype##16 y, __argtype##16 z ) { \
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

#define GENERATE_VECTOR_FUNCTIONS_3ARGS( __func, __rettype, __argtype )     \
    GENERATE_VECTOR_FUNCTIONS_3ARGS_EXPLICIT( __func, __func, __rettype, __argtype )

#define GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS( __func, __rettype, __vargtype, __sargtype ) \
    __rettype##2 OVERLOADABLE __func( __vargtype##2 x, __vargtype##2 y, __sargtype z ) { \
        return (__rettype##2)( __func(x.s0, y.s0, z),                       \
                               __func(x.s1, y.s1, z) );                     \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func( __vargtype##3 x, __vargtype##3 y, __sargtype z ) { \
        return (__rettype##3)( __func(x.s0, y.s0, z),                       \
                               __func(x.s1, y.s1, z),                       \
                               __func(x.s2, y.s2, z) );                     \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func( __vargtype##4 x, __vargtype##4 y, __sargtype z ) { \
        return (__rettype##4)( __func(x.s0, y.s0, z),                       \
                               __func(x.s1, y.s1, z),                       \
                               __func(x.s2, y.s2, z),                       \
                               __func(x.s3, y.s3, z) );                     \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func( __vargtype##8 x, __vargtype##8 y, __sargtype z ) { \
        return (__rettype##8)( __func(x.s0, y.s0, z),                       \
                               __func(x.s1, y.s1, z),                       \
                               __func(x.s2, y.s2, z),                       \
                               __func(x.s3, y.s3, z),                       \
                               __func(x.s4, y.s4, z),                       \
                               __func(x.s5, y.s5, z),                       \
                               __func(x.s6, y.s6, z),                       \
                               __func(x.s7, y.s7, z) );                     \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func( __vargtype##16 x, __vargtype##16 y, __sargtype z ) { \
        return (__rettype##16)( __func(x.s0, y.s0, z),                      \
                                __func(x.s1, y.s1, z),                      \
                                __func(x.s2, y.s2, z),                      \
                                __func(x.s3, y.s3, z),                      \
                                __func(x.s4, y.s4, z),                      \
                                __func(x.s5, y.s5, z),                      \
                                __func(x.s6, y.s6, z),                      \
                                __func(x.s7, y.s7, z),                      \
                                __func(x.s8, y.s8, z),                      \
                                __func(x.s9, y.s9, z),                      \
                                __func(x.sa, y.sa, z),                      \
                                __func(x.sb, y.sb, z),                      \
                                __func(x.sc, y.sc, z),                      \
                                __func(x.sd, y.sd, z),                      \
                                __func(x.se, y.se, z),                      \
                                __func(x.sf, y.sf, z) );                    \
    }

#define GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS( __func, __rettype, __vargtype, __sargtype ) \
    __rettype##2 OVERLOADABLE __func( __vargtype##2 x, __sargtype y, __sargtype z ) { \
        return (__rettype##2)( __func(x.s0, y, z),                          \
                               __func(x.s1, y, z) );                        \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func( __vargtype##3 x, __sargtype y, __sargtype z ) { \
        return (__rettype##3)( __func(x.s0, y, z),                          \
                               __func(x.s1, y, z),                          \
                               __func(x.s2, y, z) );                        \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func( __vargtype##4 x, __sargtype y, __sargtype z ) { \
        return (__rettype##4)( __func(x.s0, y, z),                          \
                               __func(x.s1, y, z),                          \
                               __func(x.s2, y, z),                          \
                               __func(x.s3, y, z) );                        \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func( __vargtype##8 x, __sargtype y, __sargtype z ) { \
        return (__rettype##8)( __func(x.s0, y, z),                          \
                               __func(x.s1, y, z),                          \
                               __func(x.s2, y, z),                          \
                               __func(x.s3, y, z),                          \
                               __func(x.s4, y, z),                          \
                               __func(x.s5, y, z),                          \
                               __func(x.s6, y, z),                          \
                               __func(x.s7, y, z) );                        \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func( __vargtype##16 x, __sargtype y, __sargtype z ) { \
        return (__rettype##16)( __func(x.s0, y, z),                         \
                                __func(x.s1, y, z),                         \
                                __func(x.s2, y, z),                         \
                                __func(x.s3, y, z),                         \
                                __func(x.s4, y, z),                         \
                                __func(x.s5, y, z),                         \
                                __func(x.s6, y, z),                         \
                                __func(x.s7, y, z),                         \
                                __func(x.s8, y, z),                         \
                                __func(x.s9, y, z),                         \
                                __func(x.sa, y, z),                         \
                                __func(x.sb, y, z),                         \
                                __func(x.sc, y, z),                         \
                                __func(x.sd, y, z),                         \
                                __func(x.se, y, z),                         \
                                __func(x.sf, y, z) );                       \
    }

#define GENERATE_VECTOR_FUNCTIONS_3ARGS_SSV( __func, __rettype, __sargtype, __vargtype ) \
    __rettype##2 OVERLOADABLE __func( __sargtype x, __sargtype y, __vargtype##2 z ) { \
        return (__rettype##2)( __func(x, y, z.s0),                          \
                               __func(x, y, z.s1) );                        \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func( __sargtype x, __sargtype y, __vargtype##3 z ) { \
        return (__rettype##3)( __func(x, y, z.s0),                          \
                               __func(x, y, z.s1),                          \
                               __func(x, y, z.s2) );                        \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func( __sargtype x, __sargtype y, __vargtype##4 z ) { \
        return (__rettype##4)( __func(x, y, z.s0),                          \
                               __func(x, y, z.s1),                          \
                               __func(x, y, z.s2),                          \
                               __func(x, y, z.s3) );                        \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func( __sargtype x, __sargtype y, __vargtype##8 z ) { \
        return (__rettype##8)( __func(x, y, z.s0),                          \
                               __func(x, y, z.s1),                          \
                               __func(x, y, z.s2),                          \
                               __func(x, y, z.s3),                          \
                               __func(x, y, z.s4),                          \
                               __func(x, y, z.s5),                          \
                               __func(x, y, z.s6),                          \
                               __func(x, y, z.s7) );                        \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func( __sargtype x, __sargtype y, __vargtype##16 z ) { \
        return (__rettype##16)( __func(x, y, z.s0),                         \
                                __func(x, y, z.s1),                         \
                                __func(x, y, z.s2),                         \
                                __func(x, y, z.s3),                         \
                                __func(x, y, z.s4),                         \
                                __func(x, y, z.s5),                         \
                                __func(x, y, z.s6),                         \
                                __func(x, y, z.s7),                         \
                                __func(x, y, z.s8),                         \
                                __func(x, y, z.s9),                         \
                                __func(x, y, z.sa),                         \
                                __func(x, y, z.sb),                         \
                                __func(x, y, z.sc),                         \
                                __func(x, y, z.sd),                         \
                                __func(x, y, z.se),                         \
                                __func(x, y, z.sf) );                       \
    }

#define GENERATE_CONVERSIONS_FUNCTIONS( __func, __rettype, __argtype )      \
    __rettype##2 OVERLOADABLE __func##2( __argtype##2 x ) {                 \
        return (__rettype##2)( __func(x.s0),                                \
                               __func(x.s1) );                              \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func##3( __argtype##3 x ) {                 \
        return (__rettype##3)( __func(x.s0),                                \
                               __func(x.s1),                                \
                               __func(x.s2) );                              \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func##4( __argtype##4 x ) {                 \
        return (__rettype##4)( __func(x.s0),                                \
                               __func(x.s1),                                \
                               __func(x.s2),                                \
                               __func(x.s3) );                              \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func##8( __argtype##8 x ) {                 \
        return (__rettype##8)( __func(x.s0),                                \
                               __func(x.s1),                                \
                               __func(x.s2),                                \
                               __func(x.s3),                                \
                               __func(x.s4),                                \
                               __func(x.s5),                                \
                               __func(x.s6),                                \
                               __func(x.s7) );                              \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func##16( __argtype##16 x ) {              \
        return (__rettype##16)( __func(x.s0),                               \
                                __func(x.s1),                               \
                                __func(x.s2),                               \
                                __func(x.s3),                               \
                                __func(x.s4),                               \
                                __func(x.s5),                               \
                                __func(x.s6),                               \
                                __func(x.s7),                               \
                                __func(x.s8),                               \
                                __func(x.s9),                               \
                                __func(x.sa),                               \
                                __func(x.sb),                               \
                                __func(x.sc),                               \
                                __func(x.sd),                               \
                                __func(x.se),                               \
                                __func(x.sf) );                             \
    }                                                                       \
    __rettype##2 OVERLOADABLE __func##2##_rte( __argtype##2 x ) {           \
        return (__rettype##2)( __func##_rte(x.s0),                          \
                               __func##_rte(x.s1) );                        \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func##3##_rte( __argtype##3 x ) {           \
        return (__rettype##3)( __func##_rte(x.s0),                          \
                               __func##_rte(x.s1),                          \
                               __func##_rte(x.s2) );                        \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func##4##_rte( __argtype##4 x ) {           \
        return (__rettype##4)( __func##_rte(x.s0),                          \
                               __func##_rte(x.s1),                          \
                               __func##_rte(x.s2),                          \
                               __func##_rte(x.s3) );                        \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func##8##_rte( __argtype##8 x ) {           \
        return (__rettype##8)( __func##_rte(x.s0),                          \
                               __func##_rte(x.s1),                          \
                               __func##_rte(x.s2),                          \
                               __func##_rte(x.s3),                          \
                               __func##_rte(x.s4),                          \
                               __func##_rte(x.s5),                          \
                               __func##_rte(x.s6),                          \
                               __func##_rte(x.s7) );                        \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func##16##_rte( __argtype##16 x ) {        \
        return (__rettype##16)( __func##_rte(x.s0),                         \
                                __func##_rte(x.s1),                         \
                                __func##_rte(x.s2),                         \
                                __func##_rte(x.s3),                         \
                                __func##_rte(x.s4),                         \
                                __func##_rte(x.s5),                         \
                                __func##_rte(x.s6),                         \
                                __func##_rte(x.s7),                         \
                                __func##_rte(x.s8),                         \
                                __func##_rte(x.s9),                         \
                                __func##_rte(x.sa),                         \
                                __func##_rte(x.sb),                         \
                                __func##_rte(x.sc),                         \
                                __func##_rte(x.sd),                         \
                                __func##_rte(x.se),                         \
                                __func##_rte(x.sf) );                       \
    }                                                                       \
    __rettype##2 OVERLOADABLE __func##2##_rtz( __argtype##2 x ) {           \
        return (__rettype##2)( __func##_rtz(x.s0),                          \
                               __func##_rtz(x.s1) );                        \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func##3##_rtz( __argtype##3 x ) {           \
        return (__rettype##3)( __func##_rtz(x.s0),                          \
                               __func##_rtz(x.s1),                          \
                               __func##_rtz(x.s2) );                        \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func##4##_rtz( __argtype##4 x ) {           \
        return (__rettype##4)( __func##_rtz(x.s0),                          \
                               __func##_rtz(x.s1),                          \
                               __func##_rtz(x.s2),                          \
                               __func##_rtz(x.s3) );                        \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func##8##_rtz( __argtype##8 x ) {           \
        return (__rettype##8)( __func##_rtz(x.s0),                          \
                               __func##_rtz(x.s1),                          \
                               __func##_rtz(x.s2),                          \
                               __func##_rtz(x.s3),                          \
                               __func##_rtz(x.s4),                          \
                               __func##_rtz(x.s5),                          \
                               __func##_rtz(x.s6),                          \
                               __func##_rtz(x.s7) );                        \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func##16##_rtz( __argtype##16 x ) {        \
        return (__rettype##16)( __func##_rtz(x.s0),                         \
                                __func##_rtz(x.s1),                         \
                                __func##_rtz(x.s2),                         \
                                __func##_rtz(x.s3),                         \
                                __func##_rtz(x.s4),                         \
                                __func##_rtz(x.s5),                         \
                                __func##_rtz(x.s6),                         \
                                __func##_rtz(x.s7),                         \
                                __func##_rtz(x.s8),                         \
                                __func##_rtz(x.s9),                         \
                                __func##_rtz(x.sa),                         \
                                __func##_rtz(x.sb),                         \
                                __func##_rtz(x.sc),                         \
                                __func##_rtz(x.sd),                         \
                                __func##_rtz(x.se),                         \
                                __func##_rtz(x.sf) );                       \
    }                                                                       \
    __rettype##2 OVERLOADABLE __func##2##_rtp( __argtype##2 x ) {           \
        return (__rettype##2)( __func##_rtp(x.s0),                          \
                               __func##_rtp(x.s1) );                        \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func##3##_rtp( __argtype##3 x ) {           \
        return (__rettype##3)( __func##_rtp(x.s0),                          \
                               __func##_rtp(x.s1),                          \
                               __func##_rtp(x.s2) );                        \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func##4##_rtp( __argtype##4 x ) {           \
        return (__rettype##4)( __func##_rtp(x.s0),                          \
                               __func##_rtp(x.s1),                          \
                               __func##_rtp(x.s2),                          \
                               __func##_rtp(x.s3) );                        \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func##8##_rtp( __argtype##8 x ) {           \
        return (__rettype##8)( __func##_rtp(x.s0),                          \
                               __func##_rtp(x.s1),                          \
                               __func##_rtp(x.s2),                          \
                               __func##_rtp(x.s3),                          \
                               __func##_rtp(x.s4),                          \
                               __func##_rtp(x.s5),                          \
                               __func##_rtp(x.s6),                          \
                               __func##_rtp(x.s7) );                        \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func##16##_rtp( __argtype##16 x ) {        \
        return (__rettype##16)( __func##_rtp(x.s0),                         \
                                __func##_rtp(x.s1),                         \
                                __func##_rtp(x.s2),                         \
                                __func##_rtp(x.s3),                         \
                                __func##_rtp(x.s4),                         \
                                __func##_rtp(x.s5),                         \
                                __func##_rtp(x.s6),                         \
                                __func##_rtp(x.s7),                         \
                                __func##_rtp(x.s8),                         \
                                __func##_rtp(x.s9),                         \
                                __func##_rtp(x.sa),                         \
                                __func##_rtp(x.sb),                         \
                                __func##_rtp(x.sc),                         \
                                __func##_rtp(x.sd),                         \
                                __func##_rtp(x.se),                         \
                                __func##_rtp(x.sf) );                       \
    }                                                                       \
    __rettype##2 OVERLOADABLE __func##2##_rtn( __argtype##2 x ) {           \
        return (__rettype##2)( __func##_rtn(x.s0),                          \
                               __func##_rtn(x.s1) );                        \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func##3##_rtn( __argtype##3 x ) {           \
        return (__rettype##3)( __func##_rtn(x.s0),                          \
                               __func##_rtn(x.s1),                          \
                               __func##_rtn(x.s2) );                        \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func##4##_rtn( __argtype##4 x ) {           \
        return (__rettype##4)( __func##_rtn(x.s0),                          \
                               __func##_rtn(x.s1),                          \
                               __func##_rtn(x.s2),                          \
                               __func##_rtn(x.s3) );                        \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func##8##_rtn( __argtype##8 x ) {           \
        return (__rettype##8)( __func##_rtn(x.s0),                          \
                               __func##_rtn(x.s1),                          \
                               __func##_rtn(x.s2),                          \
                               __func##_rtn(x.s3),                          \
                               __func##_rtn(x.s4),                          \
                               __func##_rtn(x.s5),                          \
                               __func##_rtn(x.s6),                          \
                               __func##_rtn(x.s7) );                        \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func##16##_rtn( __argtype##16 x ) {        \
        return (__rettype##16)( __func##_rtn(x.s0),                         \
                                __func##_rtn(x.s1),                         \
                                __func##_rtn(x.s2),                         \
                                __func##_rtn(x.s3),                         \
                                __func##_rtn(x.s4),                         \
                                __func##_rtn(x.s5),                         \
                                __func##_rtn(x.s6),                         \
                                __func##_rtn(x.s7),                         \
                                __func##_rtn(x.s8),                         \
                                __func##_rtn(x.s9),                         \
                                __func##_rtn(x.sa),                         \
                                __func##_rtn(x.sb),                         \
                                __func##_rtn(x.sc),                         \
                                __func##_rtn(x.sd),                         \
                                __func##_rtn(x.se),                         \
                                __func##_rtn(x.sf) );                       \
    }

#define GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, __argtype )  \
    __rettype##2 OVERLOADABLE __func##2##_sat( __argtype##2 x ) {           \
        return (__rettype##2)( __func##_sat(x.s0),                          \
                               __func##_sat(x.s1) );                        \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func##3##_sat( __argtype##3 x ) {           \
        return (__rettype##3)( __func##_sat(x.s0),                          \
                               __func##_sat(x.s1),                          \
                               __func##_sat(x.s2) );                        \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func##4##_sat( __argtype##4 x ) {           \
        return (__rettype##4)( __func##_sat(x.s0),                          \
                               __func##_sat(x.s1),                          \
                               __func##_sat(x.s2),                          \
                               __func##_sat(x.s3) );                        \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func##8##_sat( __argtype##8 x ) {           \
        return (__rettype##8)( __func##_sat(x.s0),                          \
                               __func##_sat(x.s1),                          \
                               __func##_sat(x.s2),                          \
                               __func##_sat(x.s3),                          \
                               __func##_sat(x.s4),                          \
                               __func##_sat(x.s5),                          \
                               __func##_sat(x.s6),                          \
                               __func##_sat(x.s7) );                        \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func##16##_sat( __argtype##16 x ) {        \
        return (__rettype##16)( __func##_sat(x.s0),                         \
                                __func##_sat(x.s1),                         \
                                __func##_sat(x.s2),                         \
                                __func##_sat(x.s3),                         \
                                __func##_sat(x.s4),                         \
                                __func##_sat(x.s5),                         \
                                __func##_sat(x.s6),                         \
                                __func##_sat(x.s7),                         \
                                __func##_sat(x.s8),                         \
                                __func##_sat(x.s9),                         \
                                __func##_sat(x.sa),                         \
                                __func##_sat(x.sb),                         \
                                __func##_sat(x.sc),                         \
                                __func##_sat(x.sd),                         \
                                __func##_sat(x.se),                         \
                                __func##_sat(x.sf) );                       \
    }                                                                       \
    __rettype##2 OVERLOADABLE __func##2##_sat_rte( __argtype##2 x ) {       \
        return (__rettype##2)( __func##_sat_rte(x.s0),                      \
                               __func##_sat_rte(x.s1) );                    \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func##3##_sat_rte( __argtype##3 x ) {       \
        return (__rettype##3)( __func##_sat_rte(x.s0),                      \
                               __func##_sat_rte(x.s1),                      \
                               __func##_sat_rte(x.s2) );                    \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func##4##_sat_rte( __argtype##4 x ) {       \
        return (__rettype##4)( __func##_sat_rte(x.s0),                      \
                               __func##_sat_rte(x.s1),                      \
                               __func##_sat_rte(x.s2),                      \
                               __func##_sat_rte(x.s3) );                    \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func##8##_sat_rte( __argtype##8 x ) {       \
        return (__rettype##8)( __func##_sat_rte(x.s0),                      \
                               __func##_sat_rte(x.s1),                      \
                               __func##_sat_rte(x.s2),                      \
                               __func##_sat_rte(x.s3),                      \
                               __func##_sat_rte(x.s4),                      \
                               __func##_sat_rte(x.s5),                      \
                               __func##_sat_rte(x.s6),                      \
                               __func##_sat_rte(x.s7) );                    \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func##16##_sat_rte( __argtype##16 x ) {    \
        return (__rettype##16)( __func##_sat_rte(x.s0),                     \
                                __func##_sat_rte(x.s1),                     \
                                __func##_sat_rte(x.s2),                     \
                                __func##_sat_rte(x.s3),                     \
                                __func##_sat_rte(x.s4),                     \
                                __func##_sat_rte(x.s5),                     \
                                __func##_sat_rte(x.s6),                     \
                                __func##_sat_rte(x.s7),                     \
                                __func##_sat_rte(x.s8),                     \
                                __func##_sat_rte(x.s9),                     \
                                __func##_sat_rte(x.sa),                     \
                                __func##_sat_rte(x.sb),                     \
                                __func##_sat_rte(x.sc),                     \
                                __func##_sat_rte(x.sd),                     \
                                __func##_sat_rte(x.se),                     \
                                __func##_sat_rte(x.sf) );                   \
    }                                                                       \
    __rettype##2 OVERLOADABLE __func##2##_sat_rtz( __argtype##2 x ) {       \
        return (__rettype##2)( __func##_sat_rtz(x.s0),                      \
                               __func##_sat_rtz(x.s1) );                    \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func##3##_sat_rtz( __argtype##3 x ) {       \
        return (__rettype##3)( __func##_sat_rtz(x.s0),                      \
                               __func##_sat_rtz(x.s1),                      \
                               __func##_sat_rtz(x.s2) );                    \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func##4##_sat_rtz( __argtype##4 x ) {       \
        return (__rettype##4)( __func##_sat_rtz(x.s0),                      \
                               __func##_sat_rtz(x.s1),                      \
                               __func##_sat_rtz(x.s2),                      \
                               __func##_sat_rtz(x.s3) );                    \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func##8##_sat_rtz( __argtype##8 x ) {       \
        return (__rettype##8)( __func##_sat_rtz(x.s0),                      \
                               __func##_sat_rtz(x.s1),                      \
                               __func##_sat_rtz(x.s2),                      \
                               __func##_sat_rtz(x.s3),                      \
                               __func##_sat_rtz(x.s4),                      \
                               __func##_sat_rtz(x.s5),                      \
                               __func##_sat_rtz(x.s6),                      \
                               __func##_sat_rtz(x.s7) );                    \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func##16##_sat_rtz( __argtype##16 x ) {    \
        return (__rettype##16)( __func##_sat_rtz(x.s0),                     \
                                __func##_sat_rtz(x.s1),                     \
                                __func##_sat_rtz(x.s2),                     \
                                __func##_sat_rtz(x.s3),                     \
                                __func##_sat_rtz(x.s4),                     \
                                __func##_sat_rtz(x.s5),                     \
                                __func##_sat_rtz(x.s6),                     \
                                __func##_sat_rtz(x.s7),                     \
                                __func##_sat_rtz(x.s8),                     \
                                __func##_sat_rtz(x.s9),                     \
                                __func##_sat_rtz(x.sa),                     \
                                __func##_sat_rtz(x.sb),                     \
                                __func##_sat_rtz(x.sc),                     \
                                __func##_sat_rtz(x.sd),                     \
                                __func##_sat_rtz(x.se),                     \
                                __func##_sat_rtz(x.sf) );                   \
    }                                                                       \
    __rettype##2 OVERLOADABLE __func##2##_sat_rtp( __argtype##2 x ) {       \
        return (__rettype##2)( __func##_sat_rtp(x.s0),                      \
                               __func##_sat_rtp(x.s1) );                    \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func##3##_sat_rtp( __argtype##3 x ) {       \
        return (__rettype##3)( __func##_sat_rtp(x.s0),                      \
                               __func##_sat_rtp(x.s1),                      \
                               __func##_sat_rtp(x.s2) );                    \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func##4##_sat_rtp( __argtype##4 x ) {       \
        return (__rettype##4)( __func##_sat_rtp(x.s0),                      \
                               __func##_sat_rtp(x.s1),                      \
                               __func##_sat_rtp(x.s2),                      \
                               __func##_sat_rtp(x.s3) );                    \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func##8##_sat_rtp( __argtype##8 x ) {       \
        return (__rettype##8)( __func##_sat_rtp(x.s0),                      \
                               __func##_sat_rtp(x.s1),                      \
                               __func##_sat_rtp(x.s2),                      \
                               __func##_sat_rtp(x.s3),                      \
                               __func##_sat_rtp(x.s4),                      \
                               __func##_sat_rtp(x.s5),                      \
                               __func##_sat_rtp(x.s6),                      \
                               __func##_sat_rtp(x.s7) );                    \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func##16##_sat_rtp( __argtype##16 x ) {    \
        return (__rettype##16)( __func##_sat_rtp(x.s0),                     \
                                __func##_sat_rtp(x.s1),                     \
                                __func##_sat_rtp(x.s2),                     \
                                __func##_sat_rtp(x.s3),                     \
                                __func##_sat_rtp(x.s4),                     \
                                __func##_sat_rtp(x.s5),                     \
                                __func##_sat_rtp(x.s6),                     \
                                __func##_sat_rtp(x.s7),                     \
                                __func##_sat_rtp(x.s8),                     \
                                __func##_sat_rtp(x.s9),                     \
                                __func##_sat_rtp(x.sa),                     \
                                __func##_sat_rtp(x.sb),                     \
                                __func##_sat_rtp(x.sc),                     \
                                __func##_sat_rtp(x.sd),                     \
                                __func##_sat_rtp(x.se),                     \
                                __func##_sat_rtp(x.sf) );                   \
    }                                                                       \
    __rettype##2 OVERLOADABLE __func##2##_sat_rtn( __argtype##2 x ) {       \
        return (__rettype##2)( __func##_sat_rtn(x.s0),                      \
                               __func##_sat_rtn(x.s1) );                    \
    }                                                                       \
    __rettype##3 OVERLOADABLE __func##3##_sat_rtn( __argtype##3 x ) {       \
        return (__rettype##3)( __func##_sat_rtn(x.s0),                      \
                               __func##_sat_rtn(x.s1),                      \
                               __func##_sat_rtn(x.s2) );                    \
    }                                                                       \
    __rettype##4 OVERLOADABLE __func##4##_sat_rtn( __argtype##4 x ) {       \
        return (__rettype##4)( __func##_sat_rtn(x.s0),                      \
                               __func##_sat_rtn(x.s1),                      \
                               __func##_sat_rtn(x.s2),                      \
                               __func##_sat_rtn(x.s3) );                    \
    }                                                                       \
    __rettype##8 OVERLOADABLE __func##8##_sat_rtn( __argtype##8 x ) {       \
        return (__rettype##8)( __func##_sat_rtn(x.s0),                      \
                               __func##_sat_rtn(x.s1),                      \
                               __func##_sat_rtn(x.s2),                      \
                               __func##_sat_rtn(x.s3),                      \
                               __func##_sat_rtn(x.s4),                      \
                               __func##_sat_rtn(x.s5),                      \
                               __func##_sat_rtn(x.s6),                      \
                               __func##_sat_rtn(x.s7) );                    \
    }                                                                       \
    __rettype##16 OVERLOADABLE __func##16##_sat_rtn( __argtype##16 x ) {    \
        return (__rettype##16)( __func##_sat_rtn(x.s0),                     \
                                __func##_sat_rtn(x.s1),                     \
                                __func##_sat_rtn(x.s2),                     \
                                __func##_sat_rtn(x.s3),                     \
                                __func##_sat_rtn(x.s4),                     \
                                __func##_sat_rtn(x.s5),                     \
                                __func##_sat_rtn(x.s6),                     \
                                __func##_sat_rtn(x.s7),                     \
                                __func##_sat_rtn(x.s8),                     \
                                __func##_sat_rtn(x.s9),                     \
                                __func##_sat_rtn(x.sa),                     \
                                __func##_sat_rtn(x.sb),                     \
                                __func##_sat_rtn(x.sc),                     \
                                __func##_sat_rtn(x.sd),                     \
                                __func##_sat_rtn(x.se),                     \
                                __func##_sat_rtn(x.sf) );                   \
    }

#define GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES( __func, __rettype )       \
    GENERATE_CONVERSIONS_FUNCTIONS( __func, __rettype, char )               \
    GENERATE_CONVERSIONS_FUNCTIONS( __func, __rettype, short )              \
    GENERATE_CONVERSIONS_FUNCTIONS( __func, __rettype, int )                \
    GENERATE_CONVERSIONS_FUNCTIONS( __func, __rettype, long )               \
    GENERATE_CONVERSIONS_FUNCTIONS( __func, __rettype, uchar )              \
    GENERATE_CONVERSIONS_FUNCTIONS( __func, __rettype, ushort )             \
    GENERATE_CONVERSIONS_FUNCTIONS( __func, __rettype, uint )               \
    GENERATE_CONVERSIONS_FUNCTIONS( __func, __rettype, ulong )              \
    GENERATE_CONVERSIONS_FUNCTIONS( __func, __rettype, float )

#define GENERATE_CONVERSIONS_FUNCTIONS_SAT_ALL_TYPES( __func, __rettype )   \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, char )           \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, short )          \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, int )            \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, long )           \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, uchar )          \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, ushort )         \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, uint )           \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, ulong )          \
    GENERATE_CONVERSIONS_FUNCTIONS_SAT( __func, __rettype, float )

#endif //_IBIF_HEADER_
