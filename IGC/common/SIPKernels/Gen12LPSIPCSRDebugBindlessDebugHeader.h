/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/


#pragma once

#include "StateSaveAreaHeader.h"

namespace SIP {
struct Gen8 {
    // The *_ALIGN fields below are padding of the SIP between
    // the registers set.
    //

    static constexpr size_t GR_COUNT = 128;
    static constexpr size_t GR_ELEMENTS = 1;
    static constexpr size_t GR_ELEMENT_SIZE = 32;
    static constexpr size_t GR_ALIGN = 0;

    static constexpr size_t A0_COUNT = 1;
    static constexpr size_t A0_ELEMENTS = 16;
    static constexpr size_t A0_ELEMENT_SIZE = 2;
    static constexpr size_t A0_ALIGN = 0;

    static constexpr size_t F_COUNT = 2;
    static constexpr size_t F_ELEMENTS = 2;
    static constexpr size_t F_ELEMENT_SIZE = 2;
    static constexpr size_t F_ALIGN = 20;

    static constexpr size_t EXEC_MASK_COUNT = 1;
    static constexpr size_t EXEC_MASK_ELEMENTS = 1;
    static constexpr size_t EXEC_MASK_ELEMENT_SIZE = 4;
    static constexpr size_t EXEC_MASK_ALIGN = 0;

    static constexpr size_t SR_COUNT = 2;
    static constexpr size_t SR_ELEMENTS = 4;
    static constexpr size_t SR_ELEMENT_SIZE = 4;
    static constexpr size_t SR_ALIGN = 0;

    static constexpr size_t CR_COUNT = 1;
    static constexpr size_t CR_ELEMENTS = 4;
    static constexpr size_t CR_ELEMENT_SIZE = 4;
    static constexpr size_t CR_ALIGN = 16;

    static constexpr size_t IP_COUNT = 1;
    static constexpr size_t IP_ELEMENTS = 1;
    static constexpr size_t IP_ELEMENT_SIZE = 4;
    static constexpr size_t IP_ALIGN = 28;

    static constexpr size_t N_COUNT = 1;
    static constexpr size_t N_ELEMENTS = 3;
    static constexpr size_t N_ELEMENT_SIZE = 4;
    static constexpr size_t N_ALIGN = 20;

    static constexpr size_t TDR_COUNT = 1;
    static constexpr size_t TDR_ELEMENTS = 8;
    static constexpr size_t TDR_ELEMENT_SIZE = 2;
    static constexpr size_t TDR_ALIGN = 16;

    static constexpr size_t ACC_COUNT = 10;
    static constexpr size_t ACC_ELEMENTS = 8;
    static constexpr size_t ACC_ELEMENT_SIZE = 4;
    static constexpr size_t ACC_ALIGN = 0;

    static constexpr size_t TM0_COUNT = 1;
    static constexpr size_t TM0_ELEMENTS = 4;
    static constexpr size_t TM0_ELEMENT_SIZE = 4;
    static constexpr size_t TM0_ALIGN = 16;

    static constexpr size_t CE_COUNT = 1;
    static constexpr size_t CE_ELEMENTS = 1;
    static constexpr size_t CE_ELEMENT_SIZE = 4;
    static constexpr size_t CE_ALIGN = 28;

    static constexpr size_t SP_COUNT = 1;
    static constexpr size_t SP_ELEMENTS = 2;
    static constexpr size_t SP_ELEMENT_SIZE = 8;
    static constexpr size_t SP_ALIGN = 0;

    static constexpr size_t DBG_COUNT = 1;
    static constexpr size_t DBG_ELEMENTS = 1;
    static constexpr size_t DBG_ELEMENT_SIZE = 4;
    static constexpr size_t DBG_ALIGN = 0;

    static constexpr size_t VERSION_COUNT = 1;
    static constexpr size_t VERSION_ELEMENTS = 1;
    static constexpr size_t VERSION_ELEMENT_SIZE = 12;
    static constexpr size_t VERSION_ALIGN = 0;

    uint8_t grf[GR_COUNT * GR_ELEMENTS * GR_ELEMENT_SIZE + GR_ALIGN];
    uint8_t a0[A0_COUNT * A0_ELEMENTS * A0_ELEMENT_SIZE + A0_ALIGN];
    uint8_t f[F_COUNT * F_ELEMENTS * F_ELEMENT_SIZE + F_ALIGN];
    uint8_t execmask[EXEC_MASK_COUNT * EXEC_MASK_ELEMENTS * EXEC_MASK_ELEMENT_SIZE + EXEC_MASK_ALIGN];
    uint8_t sr[SR_COUNT * SR_ELEMENTS * SR_ELEMENT_SIZE + SR_ALIGN];
    uint8_t cr[CR_COUNT * CR_ELEMENTS * CR_ELEMENT_SIZE + CR_ALIGN];
    uint8_t ip[IP_COUNT * IP_ELEMENTS * IP_ELEMENT_SIZE + IP_ALIGN];
    uint8_t n[N_COUNT * N_ELEMENTS * N_ELEMENT_SIZE + N_ALIGN];
    uint8_t tdr[TDR_COUNT * TDR_ELEMENTS * TDR_ELEMENT_SIZE + TDR_ALIGN];
    int8_t acc[ACC_COUNT * ACC_ELEMENTS * ACC_ELEMENT_SIZE + ACC_ALIGN];
    uint8_t tm0[TM0_COUNT * TM0_ELEMENTS * TM0_ELEMENT_SIZE + TM0_ALIGN];
    uint8_t ce[CE_COUNT * CE_ELEMENTS * CE_ELEMENT_SIZE + CE_ALIGN];
    uint8_t sp[SP_COUNT * SP_ELEMENTS * SP_ELEMENT_SIZE + SP_ALIGN];
    uint8_t dbg[DBG_COUNT * DBG_ELEMENTS * DBG_ELEMENT_SIZE + DBG_ALIGN];
    uint8_t version[VERSION_COUNT * VERSION_ELEMENTS * VERSION_ELEMENT_SIZE + VERSION_ALIGN];
};

struct StateSaveAreaHeader Gen12LPSIPCSRDebugBindlessDebugHeader = {
	{ "tssarea", 0, {1, 0, 0}, sizeof(StateSaveAreaHeader), {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}, // versionHeader
	{  //regHeader
       0,      // num_slices
	   0,      // num_subslices_per_slice
	   0,      // num_eus_per_subslice
	   0,      // num_threads_per_eu
	   0,      // state_area_offset
	   0x1800, // state_save_size
	   0,      // slm_area_offset
	   0,      // slm_bank_size
	   0,      // slm_bank_valid
       offsetof(struct Gen8, version), // sr_magic_offset
       {offsetof(struct Gen8, grf), Gen8::GR_COUNT, 0, Gen8::GR_ELEMENTS * Gen8::GR_ELEMENT_SIZE}, // grf
       {offsetof(struct Gen8, a0), Gen8::A0_COUNT, 0, Gen8::A0_ELEMENTS * Gen8::A0_ELEMENT_SIZE},  // addr
       {offsetof(struct Gen8, f), Gen8::F_COUNT, 0, Gen8::F_ELEMENTS * Gen8::F_ELEMENT_SIZE},  // flag
       {offsetof(struct Gen8, execmask), Gen8::EXEC_MASK_COUNT, 0, Gen8::EXEC_MASK_ELEMENTS * Gen8::EXEC_MASK_ELEMENT_SIZE}, // emask
       {offsetof(struct Gen8, sr), Gen8::SR_COUNT, 0, Gen8::SR_ELEMENTS * Gen8::SR_ELEMENT_SIZE}, // sr
       {offsetof(struct Gen8, cr), Gen8::CR_COUNT, 0, Gen8::CR_ELEMENTS * Gen8::CR_ELEMENT_SIZE}, // cr
       {offsetof(struct Gen8, n), Gen8::N_COUNT, 0, Gen8::N_ELEMENTS * Gen8::N_ELEMENT_SIZE}, // notification
       {offsetof(struct Gen8, tdr), Gen8::TDR_COUNT, 0, Gen8::TDR_ELEMENTS * Gen8::TDR_ELEMENT_SIZE}, // tdr
       {offsetof(struct Gen8, acc), Gen8::ACC_COUNT, 0, Gen8::ACC_ELEMENTS * Gen8::ACC_ELEMENT_SIZE}, // acc
       {0, 0, 0, 0}, // mme
       {offsetof(struct Gen8, tm0), Gen8::TM0_COUNT, 0, Gen8::TM0_ELEMENTS * Gen8::TM0_ELEMENT_SIZE}, // tm0
       {offsetof(struct Gen8, ce), Gen8::CE_COUNT, 0, Gen8::CE_ELEMENTS * Gen8::CE_ELEMENT_SIZE}, // ce
       {offsetof(struct Gen8, sp),Gen8::SP_COUNT, 0, Gen8::SP_ELEMENTS * Gen8::SP_ELEMENT_SIZE}, // sp
       {offsetof(struct Gen8, dbg), Gen8::DBG_COUNT, 0, Gen8::SP_ELEMENTS * Gen8::SP_ELEMENT_SIZE}, // dbg
       {0, 0, 0, 0} // cmd
	}
};
}  // namespace SIP
