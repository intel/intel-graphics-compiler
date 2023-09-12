/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../Models/Models.hpp"
#include "Types.hpp"
#include "Loc.hpp"

using namespace iga;

const Loc Loc::INVALID(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);

#define REGION_VWH(V, W, H)                                                    \
  {                                                                            \
    static_cast<unsigned int>(Region::Vert::V),                                \
        static_cast<unsigned int>(Region::Width::W),                           \
        static_cast<unsigned int>(Region::Horz::H)                             \
  }

const Region Region::INVALID = REGION_VWH(VT_INVALID, WI_INVALID, HZ_INVALID);
const Region Region::DST1 = REGION_VWH(VT_INVALID, WI_INVALID, HZ_1);
const Region Region::DST2 = REGION_VWH(VT_INVALID, WI_INVALID, HZ_2);
const Region Region::DST4 = REGION_VWH(VT_INVALID, WI_INVALID, HZ_4);
// basic srcs
const Region Region::SRC010  = REGION_VWH(VT_0, WI_1, HZ_0);
const Region Region::SRC110  = REGION_VWH(VT_1, WI_1, HZ_0);
const Region Region::SRC210  = REGION_VWH(VT_2, WI_1, HZ_0);
const Region Region::SRC410  = REGION_VWH(VT_4, WI_1, HZ_0);
const Region Region::SRC810  = REGION_VWH(VT_8, WI_1, HZ_0);
const Region Region::SRC1610 = REGION_VWH(VT_16, WI_1, HZ_0);
const Region Region::SRC221  = REGION_VWH(VT_2, WI_2, HZ_1);
const Region Region::SRC441  = REGION_VWH(VT_4, WI_4, HZ_1);
const Region Region::SRC881  = REGION_VWH(VT_8, WI_8, HZ_1);
const Region Region::SRCFF1  = REGION_VWH(VT_16, WI_16, HZ_1);
// ternary align1 src0 and src1
const Region Region::SRC0X0 = REGION_VWH(VT_0, WI_INVALID, HZ_0);
const Region Region::SRC1X0 = REGION_VWH(VT_1, WI_INVALID, HZ_0);
const Region Region::SRC2X1 = REGION_VWH(VT_2, WI_INVALID, HZ_1);
const Region Region::SRC4X1 = REGION_VWH(VT_4, WI_INVALID, HZ_1);
const Region Region::SRC8X1 = REGION_VWH(VT_8, WI_INVALID, HZ_1);
const Region Region::SRC4X2 = REGION_VWH(VT_4, WI_INVALID, HZ_2);
const Region Region::SRC8X4 = REGION_VWH(VT_8, WI_INVALID, HZ_4);
// ternary align src2
const Region Region::SRCXX0 = REGION_VWH(VT_INVALID, WI_INVALID, HZ_0);
const Region Region::SRCXX1 = REGION_VWH(VT_INVALID, WI_INVALID, HZ_1);
const Region Region::SRCXX2 = REGION_VWH(VT_INVALID, WI_INVALID, HZ_2);

Platform iga::ToPlatform(iga_gen_t gen) {
  // for binary compatibilty we accept the enum values from pre Xe-renaming
  // platforms (e.g IGA_GEN12p1 is GEN_VER(12,1), but we now name XE_VER(1,0)
  switch (gen) {
  case iga_gen_t::IGA_GEN12p1:
    gen = iga_gen_t::IGA_XE;
    break;
  default:
    break;
  }

  const auto *m = Model::LookupModel(Platform(gen));
  return m ? m->platform : iga::Platform::INVALID;
}