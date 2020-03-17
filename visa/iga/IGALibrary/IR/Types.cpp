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

