/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

// Set packing alignment to a single byte
#pragma pack(1)

namespace G6HWC
{

/*****************************************************************************\
STRUCT: SGfxSamplerIndirectState (SAMPLER_INDIRECT_STATE)
\*****************************************************************************/
struct SGfxSamplerIndirectState
{
    // DWORD 0
    float   BorderColorRed;

    // DWORD 1
    float   BorderColorGreen;

    // DWORD 2
    float   BorderColorBlue;

    // DWORD 3
    float   BorderColorAlpha;

    // DWORDs 4-11 should all be set to zero:
    DWORD   _Unused1;
    DWORD   _Unused2;
    DWORD   _Unused3;
    DWORD   _Unused4;
    DWORD   _Unused5;
    DWORD   _Unused6;
    DWORD   _Unused7;
    DWORD   _Unused8;
};

static_assert(SIZE32(SGfxSamplerIndirectState) == 12);

}  // namespace G6HWC

// Reset packing alignment to project default
#pragma pack()
