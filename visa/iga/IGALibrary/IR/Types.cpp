/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "Types.hpp"
#include "Loc.hpp"

using namespace iga;

const Loc Loc::INVALID(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF);

#define REGION_VWH(V,W,H) {\
    static_cast<unsigned int>(Region::Vert::V), \
    static_cast<unsigned int>(Region::Width::W), \
    static_cast<unsigned int>(Region::Horz::H) \
    }

const Region Region::INVALID = REGION_VWH(VT_INVALID, WI_INVALID, HZ_INVALID);
const Region Region::DST1   = REGION_VWH(VT_INVALID,WI_INVALID,HZ_1);
// basic srcs
const Region Region::SRC010 = REGION_VWH(VT_0,WI_1,HZ_0);
const Region Region::SRC110 = REGION_VWH(VT_1,WI_1,HZ_0);
const Region Region::SRC221 = REGION_VWH(VT_2,WI_2,HZ_1);
const Region Region::SRC441 = REGION_VWH(VT_4,WI_4,HZ_1);
const Region Region::SRC881 = REGION_VWH(VT_8,WI_8,HZ_1);
const Region Region::SRCFF1 = REGION_VWH(VT_16,WI_16,HZ_1);
// ternary align1 src0 and src1
const Region Region::SRC0X0 = REGION_VWH(VT_0,WI_INVALID,HZ_0);
const Region Region::SRC1X0 = REGION_VWH(VT_1,WI_INVALID,HZ_0);
const Region Region::SRC2X1 = REGION_VWH(VT_2,WI_INVALID,HZ_1);
const Region Region::SRC4X1 = REGION_VWH(VT_4,WI_INVALID,HZ_1);
const Region Region::SRC8X1 = REGION_VWH(VT_8,WI_INVALID,HZ_1);
// ternary align src2
const Region Region::SRCXX0 = REGION_VWH(VT_INVALID,WI_INVALID,HZ_0);
const Region Region::SRCXX1 = REGION_VWH(VT_INVALID,WI_INVALID,HZ_1);
const Region Region::SRCXX2 = REGION_VWH(VT_INVALID,WI_INVALID,HZ_2);

/*
SFID iga::OpToSFID(Op op)
{
    switch (op) {
    case Op::SEND_CRE:
    case Op::SENDC_CRE:
        return SFID::CRE;
    case Op::SEND_DC0:
    case Op::SENDC_DC0:
        return SFID::DC0;
    case Op::SEND_DC1:
    case Op::SENDC_DC1:
        return SFID::DC1;
    case Op::SEND_DC2:
    case Op::SENDC_DC2:
        return SFID::DC2;
    case Op::SEND_DCRO:
    case Op::SENDC_DCRO:
        return SFID::DCRO;
    case Op::SEND_GTWY:
    case Op::SENDC_GTWY:
        return SFID::GTWY;
    case Op::SEND_NULL:
    case Op::SENDC_NULL:
        return SFID::NULL_;
    case Op::SEND_PIXI:
    case Op::SENDC_PIXI:
        return SFID::PIXI;
    case Op::SEND_RC:
    case Op::SENDC_RC:
        return SFID::RC;
    case Op::SEND_TS:
    case Op::SENDC_TS:
        return SFID::TS;
    case Op::SEND_URB:
    case Op::SENDC_URB:
        return SFID::URB;
    case Op::SEND_VME:
    case Op::SENDC_VME:
        return SFID::VME;
    default:
        return SFID::INVALID;
    }
}

Op iga::SFIDToOp(SFID sfid)
{
    switch (sfid) {
    case SFID::CRE:  return Op::SEND_CRE;
    case SFID::DC0:  return Op::SEND_DC0;
    case SFID::DC1:  return Op::SEND_DC1;
    case SFID::DC2:  return Op::SEND_DC2;
    case SFID::DCRO: return Op::SEND_DCRO;
    case SFID::GTWY: return Op::SEND_GTWY;
    case SFID::NULL_: return Op::SEND_NULL;
    case SFID::PIXI: return Op::SEND_PIXI;
    case SFID::RC:   return Op::SEND_RC;
    case SFID::SMPL: return Op::SEND_SMPL;
    case SFID::URB:  return Op::SEND_URB;
    case SFID::TS:   return Op::SEND_TS;
    case SFID::VME:  return Op::SEND_VME;
    default:
        return Op::INVALID;
    }
}
*/