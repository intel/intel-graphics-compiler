/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __VLOAD_VSTORE_IMPL_H__
#define __VLOAD_VSTORE_IMPL_H__

//*****************************************************************************/
// vloadn
// "Reads n components from the address computed as (p + (offset * n)) and creates a vector result value from the n components."
//*****************************************************************************/

#define VLOADN_DEF(addressSpace, scalarType, numElements, offsetType, mangle)        \
    INLINE                            scalarType##numElements                        \
        __attribute__((overloadable)) __spirv_ocl_vloadn_R##scalarType##numElements( \
            offsetType offset, addressSpace scalarType *p, int n)                    \
    {                                                                                \
        const addressSpace scalarType *pOffset = p + offset * numElements;           \
        scalarType##numElements        ret;                                          \
        __builtin_IB_memcpy_##addressSpace##_to_private(                             \
            (private uchar *)&ret,                                                   \
            (addressSpace uchar *)pOffset,                                           \
            sizeof(scalarType) * numElements,                                        \
            sizeof(scalarType));                                                     \
        return ret;                                                                  \
    }

#define VLOADN_AS(addressSpace, scalarType, mang)              \
    VLOADN_DEF(addressSpace, scalarType, 2, long, i64_##mang)  \
    VLOADN_DEF(addressSpace, scalarType, 2, int, i32_##mang)   \
    VLOADN_DEF(addressSpace, scalarType, 3, long, i64_##mang)  \
    VLOADN_DEF(addressSpace, scalarType, 3, int, i32_##mang)   \
    VLOADN_DEF(addressSpace, scalarType, 4, long, i64_##mang)  \
    VLOADN_DEF(addressSpace, scalarType, 4, int, i32_##mang)   \
    VLOADN_DEF(addressSpace, scalarType, 8, long, i64_##mang)  \
    VLOADN_DEF(addressSpace, scalarType, 8, int, i32_##mang)   \
    VLOADN_DEF(addressSpace, scalarType, 16, long, i64_##mang) \
    VLOADN_DEF(addressSpace, scalarType, 16, int, i32_##mang)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define VLOADN_TYPE(TYPE, TYPEMANG)         \
    VLOADN_AS(global, TYPE, p1##TYPEMANG)   \
    VLOADN_AS(constant, TYPE, p2##TYPEMANG) \
    VLOADN_AS(local, TYPE, p3##TYPEMANG)    \
    VLOADN_AS(private, TYPE, p0##TYPEMANG)  \
    VLOADN_AS(generic, TYPE, p4##TYPEMANG)
#else
#define VLOADN_TYPE(TYPE, TYPEMANG)         \
    VLOADN_AS(global, TYPE, p1##TYPEMANG)   \
    VLOADN_AS(constant, TYPE, p2##TYPEMANG) \
    VLOADN_AS(local, TYPE, p3##TYPEMANG)    \
    VLOADN_AS(private, TYPE, p0##TYPEMANG)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

//*****************************************************************************/
// vstoren
// "Writes n components from the data vector value to the address computed as (p + (offset * n)),
//  where n is equal to the component count of the vector data."
//*****************************************************************************/

#define VSTOREN_DEF(addressSpace, scalarType, numElements, offsetType, mangle)       \
    INLINE void __attribute__((overloadable)) __spirv_ocl_vstoren(                   \
        scalarType##numElements data, offsetType offset, addressSpace scalarType *p) \
    {                                                                                \
        addressSpace scalarType *pOffset = p + offset * numElements;                 \
        scalarType##numElements  ret     = data;                                     \
        __builtin_IB_memcpy_private_to_##addressSpace(                               \
            (addressSpace uchar *)pOffset,                                           \
            (private uchar *)&ret,                                                   \
            sizeof(scalarType) * numElements,                                        \
            sizeof(scalarType));                                                     \
    }

#define VSTOREN_AS(addressSpace, scalarType, typemang, mang)                    \
    VSTOREN_DEF(addressSpace, scalarType, 2, long, v2##typemang##_i64_##mang)   \
    VSTOREN_DEF(addressSpace, scalarType, 2, int, v2##typemang##_i32_##mang)    \
    VSTOREN_DEF(addressSpace, scalarType, 3, long, v3##typemang##_i64_##mang)   \
    VSTOREN_DEF(addressSpace, scalarType, 3, int, v3##typemang##_i32_##mang)    \
    VSTOREN_DEF(addressSpace, scalarType, 4, long, v4##typemang##_i64_##mang)   \
    VSTOREN_DEF(addressSpace, scalarType, 4, int, v4##typemang##_i32_##mang)    \
    VSTOREN_DEF(addressSpace, scalarType, 8, long, v8##typemang##_i64_##mang)   \
    VSTOREN_DEF(addressSpace, scalarType, 8, int, v8##typemang##_i32_##mang)    \
    VSTOREN_DEF(addressSpace, scalarType, 16, long, v16##typemang##_i64_##mang) \
    VSTOREN_DEF(addressSpace, scalarType, 16, int, v16##typemang##_i32_##mang)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define VSTOREN_TYPE(TYPE, TYPEMANG)                  \
    VSTOREN_AS(global, TYPE, TYPEMANG, p1##TYPEMANG)  \
    VSTOREN_AS(local, TYPE, TYPEMANG, p3##TYPEMANG)   \
    VSTOREN_AS(private, TYPE, TYPEMANG, p0##TYPEMANG) \
    VSTOREN_AS(generic, TYPE, TYPEMANG, p4##TYPEMANG)
#else
#define VSTOREN_TYPE(TYPE, TYPEMANG)                 \
    VSTOREN_AS(global, TYPE, TYPEMANG, p1##TYPEMANG) \
    VSTOREN_AS(local, TYPE, TYPEMANG, p3##TYPEMANG)  \
    VSTOREN_AS(private, TYPE, TYPEMANG, p0##TYPEMANG)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#endif // __VLOAD_VSTORE_IMPL_H__
