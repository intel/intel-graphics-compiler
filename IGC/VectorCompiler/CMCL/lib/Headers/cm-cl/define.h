/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef CM_CL_DEFINE_H
#define CM_CL_DEFINE_H

#define CM_INLINE __attribute__((always_inline))
#define CM_NOINLINE __attribute__((noinline))
#define CM_NODEBUG __attribute__((nodebug))
#define _CM_BUILTIN_ __declspec(cm_builtin)

namespace cm {
namespace atomic {

enum class operation {
  add = 0x0,
  sub = 0x1,
  min = 0x2,
  max = 0x3,
  xchg = 0x4,
  andl = 0x5,
  orl = 0x6,
  xorl = 0x7,
  minsint = 0x8,
  maxsint = 0x9,
  load = 0xA,
  store = 0xB,
  cmpxchg = 0xC
};

} // namespace atomic

namespace tag {

// Tag for fast math.
struct fast_t final {};
constexpr fast_t fast{};

} // namespace tag

} // namespace cm

#endif // CM_CL_DEFINE_H
