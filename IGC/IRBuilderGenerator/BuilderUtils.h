/*========================== begin_copyright_notice ============================

Copyright (C) 2023-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#if defined(__clang__)
// normal global address space
#define GAS __attribute__((address_space(1)))
#define CAS __attribute__((address_space(2)))

typedef float _float3 __attribute__((ext_vector_type(3)));
typedef float _float8 __attribute__((ext_vector_type(8)));
typedef uint32_t _uint8 __attribute__((ext_vector_type(8)));
typedef uint32_t _uint4 __attribute__((ext_vector_type(4)));

#define CREATE_PRIVATE extern "C" [[clang::annotate("create")]] [[clang::annotate("private")]] __attribute__((noinline))
#define CREATE_PUBLIC extern "C" [[clang::annotate("create")]] [[clang::annotate("public")]] __attribute__((noinline))
#define TYPEOF extern "C" [[clang::annotate("type-of")]] __attribute__((noinline))
#define ALIGNOF extern "C" [[clang::annotate("align-of")]] __attribute__((noinline))
#define ATTR extern "C" __attribute__((noinline))
#define BUILTIN __attribute__((noinline))
#define PUREBUILTIN BUILTIN __attribute__((const))
#define IMPL static __attribute__((always_inline))
#else
// This file is only compiled via clang, it's not part of the build for
// IRBuilderGenerator. We just define this here so intellisense isn't broken
// when editing this file in VS.
#define GAS
#define CAS
struct _float3 {
  float s0, s1, s2;
};
struct _float8 {
  float s0, s1, s2, s3, s4, s5, s6, s7;
};
struct _uint8 {
  uint32_t s0, s1, s2, s3, s4, s5, s6, s7;
};
struct _uint4 {
  uint32_t s0, s1, s2, s3;
};
#define CREATE_PRIVATE
#define CREATE_PUBLIC
#define TYPEOF
#define ALIGNOF
#define ATTR
#define BUILTIN
#define PUREBUILTIN
#define IMPL
#define __restrict__
#endif // __clang__

// TODO: can do this more generically with std::tuple, but we need to get to
// clang 14 first.
template <typename T> struct fn_type_traits;

template <typename R, typename A> struct fn_type_traits<R(A)> {
  using RetTy = R;
  using ATy = A;
};

template <typename R, typename A, typename B> struct fn_type_traits<R(A, B)> {
  using RetTy = R;
  using ATy = A;
  using BTy = B;
};

template <typename R, typename A, typename B, typename C> struct fn_type_traits<R(A, B, C)> {
  using RetTy = R;
  using ATy = A;
  using BTy = B;
  using CTy = C;
};

template <typename R, typename A, typename B, typename C, typename D> struct fn_type_traits<R(A, B, C, D)> {
  using RetTy = R;
  using ATy = A;
  using BTy = B;
  using CTy = C;
  using DTy = D;
};

template <typename R, typename A, typename B, typename C, typename D, typename E>
struct fn_type_traits<R(A, B, C, D, E)> {
  using RetTy = R;
  using ATy = A;
  using BTy = B;
  using CTy = C;
  using DTy = D;
  using ETy = E;
};

template <typename R, typename A, typename B, typename C, typename D, typename E, typename F>
struct fn_type_traits<R(A, B, C, D, E, F)> {
  using RetTy = R;
  using ATy = A;
  using BTy = B;
  using CTy = C;
  using DTy = D;
  using ETy = E;
  using FTy = F;
};

#define IMPL_1ARG(F, GEN, FIMPL, A)                                                                                    \
  CREATE_PRIVATE auto F(fn_type_traits<decltype(FIMPL<GEN>)>::ATy A) { return FIMPL<GEN>(A); }

#define IMPL_2ARG(F, GEN, FIMPL, A, B)                                                                                 \
  CREATE_PRIVATE auto F(fn_type_traits<decltype(FIMPL<GEN>)>::ATy A, fn_type_traits<decltype(FIMPL<GEN>)>::BTy B) {    \
    return FIMPL<GEN>(A, B);                                                                                           \
  }

#define IMPL_3ARG(F, GEN, FIMPL, A, B, C)                                                                              \
  CREATE_PRIVATE auto F(fn_type_traits<decltype(FIMPL<GEN>)>::ATy A, fn_type_traits<decltype(FIMPL<GEN>)>::BTy B,      \
                        fn_type_traits<decltype(FIMPL<GEN>)>::CTy C) {                                                 \
    return FIMPL<GEN>(A, B, C);                                                                                        \
  }

#define IMPL_4ARG(F, GEN, FIMPL, A, B, C, D)                                                                           \
  CREATE_PRIVATE auto F(fn_type_traits<decltype(FIMPL<GEN>)>::ATy A, fn_type_traits<decltype(FIMPL<GEN>)>::BTy B,      \
                        fn_type_traits<decltype(FIMPL<GEN>)>::CTy C, fn_type_traits<decltype(FIMPL<GEN>)>::DTy D) {    \
    return FIMPL<GEN>(A, B, C, D);                                                                                     \
  }

#define IMPL_5ARG(F, GEN, FIMPL, A, B, C, D, E)                                                                        \
  CREATE_PRIVATE auto F(fn_type_traits<decltype(FIMPL<GEN>)>::ATy A, fn_type_traits<decltype(FIMPL<GEN>)>::BTy B,      \
                        fn_type_traits<decltype(FIMPL<GEN>)>::CTy C, fn_type_traits<decltype(FIMPL<GEN>)>::DTy D,      \
                        fn_type_traits<decltype(FIMPL<GEN>)>::ETy E) {                                                 \
    return FIMPL<GEN>(A, B, C, D, E);                                                                                  \
  }

#define IMPL_6ARG(F, GEN, FIMPL, A, B, C, D, E, F0)                                                                    \
  CREATE_PRIVATE auto F(fn_type_traits<decltype(FIMPL<GEN>)>::ATy A, fn_type_traits<decltype(FIMPL<GEN>)>::BTy B,      \
                        fn_type_traits<decltype(FIMPL<GEN>)>::CTy C, fn_type_traits<decltype(FIMPL<GEN>)>::DTy D,      \
                        fn_type_traits<decltype(FIMPL<GEN>)>::ETy E, fn_type_traits<decltype(FIMPL<GEN>)>::FTy F0) {   \
    return FIMPL<GEN>(A, B, C, D, E, F0);                                                                              \
  }

#define GEN_1ARG(F, Gen, A) IMPL_1ARG(F##_##Gen, Gen, F, A)

#define GEN_2ARG(F, Gen, A, B) IMPL_2ARG(F##_##Gen, Gen, F, A, B)

#define GEN_3ARG(F, Gen, A, B, C) IMPL_3ARG(F##_##Gen, Gen, F, A, B, C)

#define GEN_4ARG(F, Gen, A, B, C, D) IMPL_4ARG(F##_##Gen, Gen, F, A, B, C, D)

#define GEN_5ARG(F, Gen, A, B, C, D, E) IMPL_5ARG(F##_##Gen, Gen, F, A, B, C, D, E)

#define GEN_6ARG(F, Gen, A, B, C, D, E, F0) IMPL_6ARG(F##_##Gen, Gen, F, A, B, C, D, E, F0)

#define IMPL_ALL_1ARG(F, A)                                                                                            \
  GEN_1ARG(F, Xe, A)                                                                                                   \
  GEN_1ARG(F, Xe3, A)                                                                                                  \
  GEN_1ARG(F, Xe3PEff64, A)

#define IMPL_ALL_2ARG(F, A, B)                                                                                         \
  GEN_2ARG(F, Xe, A, B)                                                                                                \
  GEN_2ARG(F, Xe3, A, B)                                                                                               \
  GEN_2ARG(F, Xe3PEff64, A, B)

#define IMPL_ALL_3ARG(F, A, B, C)                                                                                      \
  GEN_3ARG(F, Xe, A, B, C)                                                                                             \
  GEN_3ARG(F, Xe3, A, B, C)                                                                                            \
  GEN_3ARG(F, Xe3PEff64, A, B, C)

#define IMPL_ALL_4ARG(F, A, B, C, D)                                                                                   \
  GEN_4ARG(F, Xe, A, B, C, D)                                                                                          \
  GEN_4ARG(F, Xe3, A, B, C, D)                                                                                         \
  GEN_4ARG(F, Xe3PEff64, A, B, C, D)

#define IMPL_ALL_5ARG(F, A, B, C, D, E)                                                                                \
  GEN_5ARG(F, Xe, A, B, C, D, E)                                                                                       \
  GEN_5ARG(F, Xe3, A, B, C, D, E)                                                                                      \
  GEN_5ARG(F, Xe3PEff64, A, B, C, D, E)

#define IMPL_ALL_6ARG(F, A, B, C, D, E, F0)                                                                            \
  GEN_6ARG(F, Xe, A, B, C, D, E, F0)                                                                                   \
  GEN_6ARG(F, Xe3, A, B, C, D, E, F0)                                                                                  \
  GEN_6ARG(F, Xe3PEff64, A, B, C, D, E, F0)

#define IMPL_ALL_1ARG_XE3PLUS(F, A)                                                                                    \
  GEN_1ARG(F, Xe3, A)                                                                                                  \
  GEN_1ARG(F, Xe3PEff64, A)

#define IMPL_ALL_2ARG_XE3PLUS(F, A, B)                                                                                 \
  GEN_2ARG(F, Xe3, A, B)                                                                                               \
  GEN_2ARG(F, Xe3PEff64, A, B)

#define IMPL_ALL_3ARG_XE3PLUS(F, A, B, C)                                                                              \
  GEN_3ARG(F, Xe3, A, B, C)                                                                                            \
  GEN_3ARG(F, Xe3PEff64, A, B, C)

#define IMPL_ALL_4ARG_XE3PLUS(F, A, B, C, D)                                                                           \
  GEN_4ARG(F, Xe3, A, B, C, D)                                                                                         \
  GEN_4ARG(F, Xe3PEff64, A, B, C, D)

#define IMPL_ALL_5ARG_XE3PLUS(F, A, B, C, D, E)                                                                        \
  GEN_5ARG(F, Xe3, A, B, C, D, E)                                                                                      \
  GEN_5ARG(F, Xe3PEff64, A, B, C, D, E)

#define IMPL_ALL_6ARG_XE3PLUS(F, A, B, C, D, E, F0)                                                                    \
  GEN_6ARG(F, Xe3, A, B, C, D, E, F0)                                                                                  \
  GEN_6ARG(F, Xe3PEff64, A, B, C, D, E, F0)

#define BITMASK(n) (~((0xffffffff) << (n)))
#define QWBITMASK(n) (~((0xffffffffffffffffull) << (n)))
#define BITMASK_RANGE(startbit, endbit) (BITMASK((endbit) + 1) & ~BITMASK(startbit))
