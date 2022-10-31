/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GEDBitProcessor.hpp"
#include "ged.h"

using namespace iga;

GEDBitProcessor::GEDBitProcessor(const Model &model, ErrorHandler &errHandler)
    : BitProcessor(errHandler), m_model(model) {}

bool GEDBitProcessor::isAlign16MathMacroRegisterCsrOperand(
    Operand::Kind opKind, RegName regName, uint16_t regNum) const {
  return isAlign16MathMacroRegisterCsrPlatform() &&
         opKind == Operand::Kind::DIRECT && regName == RegName::ARF_MME &&
         // on the above platforms mme0 is acc2 and from the spec we have
         // for save:
         //   mov(8) r113:ud acc2:ud      {NoMask}  //acc2 <<< ALIGN1 IMPLIED!
         //   mov(8) r114:ud acc2.yx:ud  {NoMask, Align16}     //acc3
         //   ... on up to acc9 with different ChSel
         // and for restore:
         //   mov(8) acc2:ud r100:ud      {NoMask}  //acc2 <<< ALIGN1 IMPLIED!
         //   mov(8) acc2.x:ud r101:ud    {NoMask, Align16}     //acc3
         //   ... on up to acc9 with different ChEn
         regNum > 0;
}

bool GEDBitProcessor::isAlign16MathMacroRegisterCsrPlatform() const {
  return m_model.platform >= Platform::GEN8 &&
         m_model.platform <= Platform::GEN9;
}
