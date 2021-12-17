/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file contains the definitions for IGC builtin functions for OpenCL 1.2
// atomic functions.
//
// DEF_OCL_IGC_ATOMIC is defined in ResolveOCLAtomics.cpp
//
//-------------------------------------------------------------------------------------
//                           Name                                  Op          buf type
//-------------------------------------------------------------------------------------
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_add_global_i32", EATOMIC_IADD, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_add_local_i32", EATOMIC_IADD, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_sub_global_i32", EATOMIC_SUB, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_sub_local_i32", EATOMIC_SUB, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_xchg_global_i32", EATOMIC_XCHG, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_xchg_local_i32", EATOMIC_XCHG, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_min_global_i32", EATOMIC_IMIN, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_min_global_u32", EATOMIC_UMIN, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_min_global_f32", EATOMIC_FMIN, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_min_local_i32", EATOMIC_IMIN, SLM)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_min_local_u32", EATOMIC_UMIN, SLM)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_min_local_f32", EATOMIC_FMIN, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_max_global_i32", EATOMIC_IMAX, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_max_global_u32", EATOMIC_UMAX, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_max_global_f32", EATOMIC_FMAX, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_max_local_i32", EATOMIC_IMAX, SLM)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_max_local_u32", EATOMIC_UMAX, SLM)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_max_local_f32", EATOMIC_FMAX, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_and_global_i32", EATOMIC_AND, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_and_local_i32", EATOMIC_AND, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_or_global_i32", EATOMIC_OR, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_or_local_i32", EATOMIC_OR, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_xor_global_i32", EATOMIC_XOR, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_xor_local_i32", EATOMIC_XOR, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_inc_global_i32", EATOMIC_INC, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_inc_local_i32", EATOMIC_INC, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_dec_global_i32", EATOMIC_DEC, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_dec_local_i32", EATOMIC_DEC, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_cmpxchg_global_i32", EATOMIC_CMPXCHG, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_cmpxchg_global_f32", EATOMIC_FCMPWR, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_cmpxchg_local_i32", EATOMIC_CMPXCHG, SLM)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_cmpxchg_local_f32", EATOMIC_FCMPWR, SLM)

//64 bit Atomics
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_add_global_i64", EATOMIC_IADD64, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_sub_global_i64", EATOMIC_SUB64, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_xchg_global_i64", EATOMIC_XCHG64, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_min_global_i64", EATOMIC_IMIN64, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_min_global_u64", EATOMIC_UMIN64, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_min_global_f64", EATOMIC_FMIN, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_max_global_i64", EATOMIC_IMAX64, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_max_global_u64", EATOMIC_UMAX64, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_max_global_f64", EATOMIC_FMAX, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_and_global_i64", EATOMIC_AND64, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_or_global_i64", EATOMIC_OR64, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_xor_global_i64", EATOMIC_XOR64, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_inc_global_i64", EATOMIC_INC64, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_dec_global_i64", EATOMIC_DEC64, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_cmpxchg_global_i64", EATOMIC_CMPXCHG64, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_cmpxchg_global_f64", EATOMIC_FCMPWR, POINTER)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_add_global_i16", EATOMIC_IADD, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_add_local_i16", EATOMIC_IADD, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_sub_global_i16", EATOMIC_SUB, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_sub_local_i16", EATOMIC_SUB, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_xchg_global_i16", EATOMIC_XCHG, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_xchg_local_i16", EATOMIC_XCHG, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_min_global_i16", EATOMIC_IMIN, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_min_global_u16", EATOMIC_UMIN, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_min_global_f16", EATOMIC_FMIN, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_min_local_i16", EATOMIC_IMIN, SLM)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_min_local_u16", EATOMIC_UMIN, SLM)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_min_local_f16", EATOMIC_FMIN, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_max_global_i16", EATOMIC_IMAX, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_max_global_u16", EATOMIC_UMAX, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_max_global_f16", EATOMIC_FMAX, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_max_local_i16", EATOMIC_IMAX, SLM)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_max_local_u16", EATOMIC_UMAX, SLM)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_max_local_f16", EATOMIC_FMAX, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_and_global_i16", EATOMIC_AND, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_and_local_i16", EATOMIC_AND, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_or_global_i16", EATOMIC_OR, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_or_local_i16", EATOMIC_OR, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_xor_global_i16", EATOMIC_XOR, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_xor_local_i16", EATOMIC_XOR, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_inc_global_i16", EATOMIC_INC, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_inc_local_i16", EATOMIC_INC, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_dec_global_i16", EATOMIC_DEC, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_dec_local_i16", EATOMIC_DEC, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_cmpxchg_global_i16", EATOMIC_CMPXCHG, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_cmpxchg_global_f16", EATOMIC_FCMPWR, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_cmpxchg_local_i16", EATOMIC_CMPXCHG, SLM)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_cmpxchg_local_f16", EATOMIC_FCMPWR, SLM)

DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_add_global_f32", EATOMIC_FADD, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_sub_global_f32", EATOMIC_FSUB, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_add_global_f64", EATOMIC_FADD64, POINTER)
DEF_OCL_IGC_ATOMIC("__builtin_IB_atomic_cmpxchg_local_i64", EATOMIC_CMPXCHG64, POINTER)

