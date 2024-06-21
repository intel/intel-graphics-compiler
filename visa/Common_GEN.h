/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef COMMON_GEN_H
#define COMMON_GEN_H

// enums for various fields in message-specific descriptors
typedef enum {
  DC_OWORD_BLOCK_READ = 0,
  DC_ALIGNED_OWORD_BLOCK_READ = 1,
  DC_DWORD_SCATTERED_READ = 3,
  DC_BYTE_SCATTERED_READ = 4,
  DC_QWORD_SCATTERED_READ = 5,
  DC_UNTYPED_ATOMIC = 6,
  DC_MEMORY_FENCE = 7,
  DC_OWORD_BLOCK_WRITE = 8,
  DC_DWORD_SCATTERED_WRITE = 11,
  DC_BYTE_SCATTERED_WRITE = 12,
  DC_QWORD_SCATTERED_WRITE = 0xD,
} DATA_CACHE0_MESSAGES;

typedef enum {
  DC1_UNTYPED_SURFACE_READ = 0x1,
  DC1_UNTYPED_ATOMIC = 0x2,
  DC1_UNTYPED_HALF_INTEGER_ATOMIC = 0x3,
  DC1_MEDIA_BLOCK_READ = 0x4,
  DC1_TYPED_SURFACE_READ = 0x5,
  DC1_TYPED_ATOMIC = 0x6,
  DC1_TYPED_HALF_INTEGER_ATOMIC = 0x7,
  DC1_UNTYPED_SURFACE_WRITE = 0x9,
  DC1_MEDIA_BLOCK_WRITE = 0xA,
  DC1_TYPED_HALF_COUNTER_ATOMIC = 0xC,
  DC1_TYPED_SURFACE_WRITE = 0xD,
  DC1_A64_SCATTERED_READ = 0x10,
  DC1_A64_UNTYPED_SURFACE_READ = 0x11,
  DC1_A64_ATOMIC = 0x12,
  DC1_A64_UNTYPED_HALF_INTEGER_ATOMIC = 0x13,
  DC1_A64_BLOCK_READ = 0x14,
  DC1_A64_BLOCK_WRITE = 0x15,
  DC1_A64_UNTYPED_SURFACE_WRITE = 0x19,
  DC1_A64_SCATTERED_WRITE = 0x1A,
  DC1_UNTYPED_FLOAT_ATOMIC = 0x1B,
  DC1_UNTYPED_HALF_FLOAT_ATOMIC = 0x1C,
  DC1_A64_UNTYPED_FLOAT_ATOMIC = 0x1D,
  DC1_A64_UNTYPED_HALF_FLOAT_ATOMIC = 0x1E
} DATA_CACHE1_MESSAGES;

typedef enum {
  A64_BLOCK_MSG_OWORD_RW = 0x0,
  A64_BLOCK_MSG_OWORD_UNALIGNED_READ = 0x1
} A64_BLOCK_MSG_SUBTYPE;

typedef enum {
  DC2_UNTYPED_SURFACE_READ = (0x01) << 1,      // MT2R_US
  DC2_A64_SCATTERED_READ = (0x02) << 1,        // MT2R_A64_SB
  DC2_A64_UNTYPED_SURFACE_READ = (0x03) << 1,  // MT2R_A64_US
  DC2_BYTE_SCATTERED_READ = (0x04) << 1,       // MT2R_BS
  DC2_UNTYPED_SURFACE_WRITE = (0x09) << 1,     // MT2W_US
  DC2_A64_UNTYPED_SURFACE_WRITE = (0x0A) << 1, // MT2W_A64_US
  DC2_A64_SCATTERED_WRITE = (0x0B) << 1,       // MT2W_A64_SB
  DC2_BYTE_SCATTERED_WRITE = (0x0C) << 1       // MT2W_BS
} DATA_CACHE2_MESSAGES;

typedef enum {
  DC1_HWORD_BLOCK_READ = 0x0,
  DC1_HWORD_ALIGNED_BLOCK_READ = 0x1,
  DC1_HWORD_BLOCK_WRITE = 0x8,
  DC1_HWORD_ALIGNED_BLOCK_WRITE = 0x9
} HWORD_DATA_CACHE1_MESSAGES;

typedef enum {
  URB_WRITE_HWORD = 0,
  URB_WRITE_OWORD = 1,
  URB_READ_HWORD = 2,
  URB_READ_OWORD = 3,
  URB_ATOMIC_MOV = 4,
  URB_ATOMIC_INC = 5,
  URB_ATOMIC_ADD = 6,
  URB_SIMD8_WRITE = 7,
  URB_SIMD8_READ = 8
} URB_MESSAGES;

// SIMD Mode 2 Message Descriptor Control Field
typedef enum { MDC_SM2_SIMD8 = 0, MDC_SM2_SIMD16 = 1 } MDC_SM2;

// Reversed SIMD Mode 2 Message Descriptor Control Field
typedef enum { MDC_SM2R_SIMD8 = 1, MDC_SM2R_SIMD16 = 0 } MDC_SM2R;

// SIMD Mode 3 Message Descriptor Control Field
typedef enum {
  MDC_SM3_SIMD4x2 = 0,
  MDC_SM3_SIMD16 = 1,
  MDC_SM3_SIMD8 = 2
} MDC_SM3;

typedef enum {
  MDC_GW_OPEN_GATEWAY = 0,
  MDC_GW_CLOSE_GATEWAY = 1,
  MDC_GW_FORWARG_MSG = 2,
  MDC_GW_GET_TIMESTAMP = 3,
  MDC_GW_BARRIER_MSG = 4,
  MDC_GW_UPDATE_GATEWAY_STATE = 5
} MDC_GATEWAY_SUBFUNC;

typedef enum {
  MDC_SG3_SG4x2 = 0,
  MDC_SG3_SG8L = 1,
  MDC_SG3_SG8U = 2,
} MDC_SG3;

enum SamplerSIMDMode { SIMD8 = 1, SIMD16 = 2, SIMD32 = 3 };

#define A64_BLOCK_MSG_SUBTYPE_OFFSET 11

namespace vISA {
enum class SFID {
  NULL_SFID = 0,
  SAMPLER = 2,
  GATEWAY = 3,
  DP_DC2 = 4,
  DP_RC = 5,   // RENDER TARGET
  URB = 6,     // URB
  SPAWNER = 7, // THREAD SPAWNER
  VME = 8,     // VIDEO MOTION ESTIMATION
  DP_CC = 9,   // CONSTANT CACHE DATAPORT
  DP_DC0 = 10, // DATA CACHE DATAPORT
  DP_PI = 11,  // PIXEL INTERPOLATOR
  DP_DC1 = 12, // DATA CACHE DATAPORT1
  CRE = 13,    // CHECK & REFINEMENT ENGINE
  BTD = 16,    // bindless thread dispatcher
  RTHW = 17,   // ray trace HW accelerator
  TGM = 18,    // typed global memory
  SLM = 19,    // untyped shared local memory
  UGM = 20,    // untyped global memory
  UGML = 21,   // untyped global memory (low bandwidth)
};

inline int SFIDtoInt(SFID id) {
  if (id == SFID::BTD) {
    return 0x7;
  } else if (id == SFID::RTHW) {
    return 0x8;
  } else if (id == SFID::TGM) {
    return 0xD;
  } else if (id == SFID::SLM) {
    return 0xE;
  } else if (id == SFID::UGM) {
    return 0xF;
  }
  return static_cast<int>(id);
};

inline SFID intToSFID(int id, TARGET_PLATFORM platform) {
  if (platform >= Xe_DG2) {
    switch (id) {
    case 0x7:
      return SFID::BTD;
    case 0x8:
      return SFID::RTHW;
    case 0xD:
      return SFID::TGM;
    case 0xE:
      return SFID::SLM;
    case 0xF:
      return SFID::UGM;
    default:
      // fall through
      break;
    }
  }
  return static_cast<SFID>(id);
};
inline SFID LSC_SFID_To_SFID(LSC_SFID lscId) {
  switch (lscId) {
  case LSC_UGM:
    return SFID::UGM;
  case LSC_UGML:
    return SFID::UGML;
  case LSC_TGM:
    return SFID::TGM;
  case LSC_SLM:
    return SFID::SLM;
  default:
    vISA_ASSERT_UNREACHABLE("invalid SFID for untyped LSC message");
    return SFID::NULL_SFID;
  }
};

}; // namespace vISA
#endif // COMMON_GEN_H
