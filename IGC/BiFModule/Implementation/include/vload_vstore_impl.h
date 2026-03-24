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

#define VLOADN_DEF(addressSpace, scalarType, numElements, offsetType, rmangling)    \
    INLINE                            scalarType##numElements                       \
        __attribute__((overloadable)) __spirv_ocl_vloadn_R##rmangling##numElements( \
            offsetType offset, const addressSpace scalarType *p, int n)             \
    {                                                                               \
        const addressSpace scalarType *pOffset = p + offset * numElements;          \
        scalarType##numElements        ret;                                         \
        __builtin_IB_memcpy_##addressSpace##_to_private(                            \
            (private uchar *)&ret,                                                  \
            (addressSpace uchar *)pOffset,                                          \
            sizeof(scalarType) * numElements,                                       \
            sizeof(scalarType));                                                    \
        return ret;                                                                 \
    }

#define VLOADN_AS(addressSpace, scalarType, rmangling)         \
    VLOADN_DEF(addressSpace, scalarType, 2, ulong, rmangling)  \
    VLOADN_DEF(addressSpace, scalarType, 2, uint, rmangling)   \
    VLOADN_DEF(addressSpace, scalarType, 3, ulong, rmangling)  \
    VLOADN_DEF(addressSpace, scalarType, 3, uint, rmangling)   \
    VLOADN_DEF(addressSpace, scalarType, 4, ulong, rmangling)  \
    VLOADN_DEF(addressSpace, scalarType, 4, uint, rmangling)   \
    VLOADN_DEF(addressSpace, scalarType, 8, ulong, rmangling)  \
    VLOADN_DEF(addressSpace, scalarType, 8, uint, rmangling)   \
    VLOADN_DEF(addressSpace, scalarType, 16, ulong, rmangling) \
    VLOADN_DEF(addressSpace, scalarType, 16, uint, rmangling)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define VLOADN_TYPE(TYPE, RMANGLING)     \
    VLOADN_AS(global, TYPE, RMANGLING)   \
    VLOADN_AS(constant, TYPE, RMANGLING) \
    VLOADN_AS(local, TYPE, RMANGLING)    \
    VLOADN_AS(private, TYPE, RMANGLING)  \
    VLOADN_AS(generic, TYPE, RMANGLING)
#else
#define VLOADN_TYPE(TYPE, RMANGLING)     \
    VLOADN_AS(global, TYPE, RMANGLING)   \
    VLOADN_AS(constant, TYPE, RMANGLING) \
    VLOADN_AS(local, TYPE, RMANGLING)    \
    VLOADN_AS(private, TYPE, RMANGLING)
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

#define VSTOREN_AS(addressSpace, scalarType, typemang, mang)                     \
    VSTOREN_DEF(addressSpace, scalarType, 2, ulong, v2##typemang##_i64_##mang)   \
    VSTOREN_DEF(addressSpace, scalarType, 2, uint, v2##typemang##_i32_##mang)    \
    VSTOREN_DEF(addressSpace, scalarType, 3, ulong, v3##typemang##_i64_##mang)   \
    VSTOREN_DEF(addressSpace, scalarType, 3, uint, v3##typemang##_i32_##mang)    \
    VSTOREN_DEF(addressSpace, scalarType, 4, ulong, v4##typemang##_i64_##mang)   \
    VSTOREN_DEF(addressSpace, scalarType, 4, uint, v4##typemang##_i32_##mang)    \
    VSTOREN_DEF(addressSpace, scalarType, 8, ulong, v8##typemang##_i64_##mang)   \
    VSTOREN_DEF(addressSpace, scalarType, 8, uint, v8##typemang##_i32_##mang)    \
    VSTOREN_DEF(addressSpace, scalarType, 16, ulong, v16##typemang##_i64_##mang) \
    VSTOREN_DEF(addressSpace, scalarType, 16, uint, v16##typemang##_i32_##mang)

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
