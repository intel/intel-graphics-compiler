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
#include "GEDBitProcessor.hpp"
#include "ged.h"

using namespace iga;

GEDBitProcessor::GEDBitProcessor(
    const Model &model,
    ErrorHandler &errHandler)
    : BitProcessor(errHandler)
    , m_model(model)
{
}

bool GEDBitProcessor::isAlign16MathMacroRegisterCsrOperand(
    Operand::Kind opKind, RegName regName, uint16_t regNum) const
{
    return
        isAlign16MathMacroRegisterCsrPlatform() &&
        opKind == Operand::Kind::DIRECT &&
        regName == RegName::ARF_MME &&
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

bool GEDBitProcessor::isAlign16MathMacroRegisterCsrPlatform() const
{
    return m_model.platform >= Platform::GEN8 &&
        m_model.platform <= Platform::GEN9;
}