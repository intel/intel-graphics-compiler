/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PrologueShaders.h"
#include "Probe/Assertion.h"

// The raygen shader is not invoked directly by the compute walker but rather
// we launch this short prologue shader first which fetches the address of the
// raygen shader then explicitly sets the ip register to start executing it.

// iga64 file.asm -p=dg2 -a -o output.krn
static constexpr unsigned char prologueShader_XeHP[] =
/// (W)      send.dc1 (1|M0)          r3       r2      null#0  0x0         0x41401FF  {$0} //  wr:2+0, rd:1, A64 Scattered Read msc:1, to global memory
/// (W)      sync.nop                                  null                           {Compacted}
/// (W)      and (1|M0)               r3.0<1>:ud    r3.0<0;1,0>:ud    0xFFFFFFC0:ud   {$0.dst}
/// (W)      mov (1|M0)               ip<1>:ud      r3.0<0;1,0>:ud                    {I@1}
{ 0x31, 0x40, 0x00, 0x80, 0x00, 0x00, 0x0c, 0x03, 0x14, 0x02, 0xfe, 0xc3, 0x00, 0x00, 0x00, 0x01,
  0x01, 0x00, 0x00, 0xe2, 0x01, 0x00, 0x01, 0x00, 0x65, 0x20, 0x00, 0x80, 0x20, 0x82, 0x05, 0x03,
  0x04, 0x03, 0x00, 0x02, 0xc0, 0xff, 0xff, 0xff, 0x61, 0x19, 0x00, 0x80, 0x20, 0x02, 0x01, 0xa0,
  0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


namespace IGC {
    void getPrologueShader(
        PRODUCT_FAMILY eProductFamily,
        void*& pKernel,
        unsigned& size)
    {
        switch (eProductFamily)
        {
        case IGFX_DG2:
        case IGFX_PVC:
            pKernel = (void*)prologueShader_XeHP;
            size    = sizeof(prologueShader_XeHP);
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "not supported yet!");
            break;
        }
    }
}

