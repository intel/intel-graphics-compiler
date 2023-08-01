/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/vector.h>

using namespace cm;

namespace {

// Value are taken from LSC_OP enum
// Source/visa/include/visa_igc_common_header.h
enum class AtomicOp : char {
  Add = 0x0C,
  Sub = 0x0D,
  Xchg = 0x0B,
  And = 0x18,
  Or = 0x19,
  Xor = 0x1A,
  SMin = 0x0E,
  SMax = 0x0F,
  UMin = 0x10,
  UMax = 0x11,
  Dec = 0x09,
  Inc = 0x08,
  Load = 0x0A,
};

template <int N>
CM_NODEBUG CM_INLINE vector<uint64_t, N>
__impl_atomic_local_binop(mask<N> pred, AtomicOp op, char l1cachecontrol,
                          char l3cachecontrol, int base, vector<int, N> index,
                          short scale, int offset, vector<uint64_t, N> src,
                          vector<uint64_t, N> passthru) {
  vector<int, N> addr = base + index * scale + offset;
  vector<uint64_t, N> laddr = addr;
  vector<uint64_t, N> orig =
      detail::__cm_cl_gather(3, laddr.cl_vector(), sizeof(uint64_t),
                             pred.cl_vector(), passthru.cl_vector());

  // Value should be equal to LSC_ATOMIC_ICAS from
  // Source/visa/include/visa_igc_common_header.h
  constexpr char OpcodeICAS = 0x12;

  // Value should be equal to LSC_ADDR_SIZE_32b from
  // Source/visa/include/visa_igc_common_header.h
  constexpr char AddrSize = 2;

  // Value should be equal to LSC_DATA_SIZE_64b from
  // Source/visa/include/visa_igc_common_header.h
  constexpr char DataSize = 4;

  do {
    vector<uint64_t, N> newval = orig;
    switch (op) {
    case AtomicOp::Add:
      newval += src;
      break;
    case AtomicOp::Sub:
      newval -= src;
      break;
    case AtomicOp::And:
      newval &= src;
      break;
    case AtomicOp::Or:
      newval |= src;
      break;
    case AtomicOp::Xor:
      newval ^= src;
      break;
    case AtomicOp::Xchg:
      newval = src;
      break;
    case AtomicOp::SMin: {
      vector<int64_t, N> ssrc = src.template format<int64_t>();
      vector<int64_t, N> snewval = newval.template format<int64_t>();
      newval.merge(src, ssrc < snewval);
    } break;
    case AtomicOp::SMax: {
      vector<int64_t, N> ssrc = src.template format<int64_t>();
      vector<int64_t, N> snewval = newval.template format<int64_t>();
      newval.merge(src, ssrc > snewval);
    } break;
    case AtomicOp::UMin:
      newval.merge(src, src < newval);
      break;
    case AtomicOp::UMax:
      newval.merge(src, src > newval);
      break;
    case AtomicOp::Inc:
      newval = newval + 1;
      break;
    case AtomicOp::Dec:
      newval = newval - 1;
      break;
    case AtomicOp::Load:
      break;
    default:
      break;
    }

    vector<uint64_t, N> res = detail::__cm_cl_vector_atomic_slm(
        pred.cl_vector(), OpcodeICAS, AddrSize, DataSize, l1cachecontrol,
        l3cachecontrol, 0, addr.cl_vector(), 1, 0, orig.cl_vector(),
        newval.cl_vector(), orig.cl_vector());
    pred &= res != orig;
    orig = res;
  } while (pred.any());

  return orig;
}

} // namespace

#define ATOMIC(WIDTH)                                                          \
  CM_NODEBUG CM_INLINE extern "C" cl_vector<uint64_t, WIDTH>                   \
      __vc_builtin_atomic_slm_v##WIDTH##i64(                                   \
          cl_vector<char, WIDTH> pred, AtomicOp op, char l1cachecontrol,       \
          char l3cachecontrol, int base, cl_vector<int, WIDTH> index,          \
          short scale, int offset, cl_vector<uint64_t, WIDTH> src,             \
          cl_vector<uint64_t, WIDTH> passthru) {                               \
    mask<WIDTH> vpred{pred};                                                   \
    vector<int, WIDTH> vindex{index};                                          \
    vector<uint64_t, WIDTH> vsrc{src};                                         \
    vector<uint64_t, WIDTH> vpassthru{passthru};                               \
    return __impl_atomic_local_binop<WIDTH>(vpred, op, l1cachecontrol,         \
                                            l3cachecontrol, base, vindex,      \
                                            scale, offset, vsrc, vpassthru)    \
        .cl_vector();                                                          \
  }

ATOMIC(1)
ATOMIC(2)
ATOMIC(4)
ATOMIC(8)
ATOMIC(16)
ATOMIC(32)
