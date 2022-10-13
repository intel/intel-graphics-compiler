/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#if !defined(__IGFXHWEUISAICL_H__)
#define __IGFXHWEUISAICL_H__

namespace G11HDL {
typedef enum tagDSTTYPE {
  DSTTYPE_UD = 0x0, // Unsigned Doubleword integer
  DSTTYPE_D = 0x1,  // signed Doubleword integer
  DSTTYPE_UW = 0x2, // Unsigned Word integer
  DSTTYPE_W = 0x3,  // signed Word integer
  DSTTYPE_UB = 0x4, // Unsigned Byte integer
  DSTTYPE_B = 0x5,  // signed Byte integer
  DSTTYPE_UQ = 0x6, // Unsigned Quadword integer
  DSTTYPE_Q = 0x7,  // signed Quadword integer
  DSTTYPE_HF = 0x8, // Half Float (16-bit)
  DSTTYPE_F = 0x9,  // single precision Float (32-bit)
  DSTTYPE_DF = 0xA, // Double precision Float (64-bit)
} DSTTYPE;

typedef enum tagSRCTYPE {
  SRCTYPE_UD = 0x0, // Unsigned Doubleword
  SRCTYPE_D = 0x1,  // signed Doubleword
  SRCTYPE_UW = 0x2, // Unsigned Word integer
  SRCTYPE_W = 0x3,  // signed Word integer
  SRCTYPE_UB = 0x4, // unsigned Byte integer
  SRCTYPE_B = 0x5,  // signed Byte integer
  SRCTYPE_UQ = 0x6, // Unsigned Quadword integer
  SRCTYPE_Q = 0x7,  // signed Quadword integer
  SRCTYPE_HF = 0x8, // Half Float (16-bit)
  SRCTYPE_F = 0x9,  // single precision Float (32-bit)
  SRCTYPE_DF = 0xA, // Double precision Float (64-bit)
} SRCTYPE;

typedef enum tagSRCIMMTYPE {
  SRCIMMTYPE_UD = 0x0, // Unsigned Doubleword
  SRCIMMTYPE_D = 0x1,  // signed Doubleword
  SRCIMMTYPE_UW = 0x2, // Unsigned Word integer
  SRCIMMTYPE_W = 0x3,  // signed Word integer
  SRCIMMTYPE_UV = 0x4, // Packed Unsigned Half-Byte Integer Vector, 8 x 4-Bit
                       // Unsigned Integer.
  SRCIMMTYPE_V =
      0x5, // Packed Signed Half-Byte Integer Vector, 8 x 4-Bit Signed Integer
  SRCIMMTYPE_UQ =
      0x6, // Unsigned Quadword integer// Double precision Float (64-bit)
  SRCIMMTYPE_Q = 0x7,  // signed Quadword integer
  SRCIMMTYPE_HF = 0x8, // Half Float (16-bit)
  SRCIMMTYPE_F = 0x9,  // single precision Float (32-bit)
  SRCIMMTYPE_DF = 0xA, // Double precision Float (64-bit)
  SRCIMMTYPE_VF = 0xB, // Packed Restricted Float Vector, 4 x 8-Bit Restricted
                       // Precision Floating-Point Number
} SRCIMMTYPE;
} // namespace G11HDL
#endif
