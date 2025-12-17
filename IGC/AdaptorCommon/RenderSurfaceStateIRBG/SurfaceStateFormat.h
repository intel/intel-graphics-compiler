/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <stdint.h>

namespace IGC {

struct RENDER_SURFACE_STATE {
  // DWORDs 0-7 (Bit Groups 0-7)
  uint32_t DWord0;
  uint32_t DWord1;
  uint32_t DWord2;
  uint32_t DWord3;
  uint32_t DWord4;
  uint32_t DWord5;
  uint32_t DWord6;
  uint32_t DWord7;

  // DWORDs 8-9 (Bit Group 8) - Surface Base Address
  uint64_t SurfaceBaseAddress;

  // DWORDs 10-15 (Bit Groups 10-15)
  uint32_t DWord10;
  uint32_t DWord11;
  uint32_t DWord12;
  uint32_t DWord13;
  uint32_t DWord14;
  uint32_t DWord15;
};

static_assert(sizeof(RENDER_SURFACE_STATE) == 64, "RENDER_SURFACE_STATE must be 64 bytes");

} // namespace IGC
