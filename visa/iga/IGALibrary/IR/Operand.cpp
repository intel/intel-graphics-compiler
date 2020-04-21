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

#include "Operand.hpp"


using namespace iga;


void Operand::setDirectDestination(
    DstModifier dstMod,
    RegName rName,
    const RegRef &reg,
    const Region::Horz &rgnHz,
    Type type)
{
    m_kind = Operand::Kind::DIRECT;

    m_regMathMacro = MathMacroExt::INVALID;
    m_regOpDstMod = dstMod;
    m_regOpReg = reg;
    m_regOpRgn.setDstHz(rgnHz);
    m_regOpName = rName;
    m_type = type;
}


void Operand::setMacroDestination(
    DstModifier dstMod,
    RegName r,
    const RegRef &reg,
    MathMacroExt mme,
    Region::Horz rgnHz,
    Type type)
{
    m_kind = Operand::Kind::MACRO;

    m_regOpDstMod = dstMod;
    m_regOpReg = reg;
    m_regMathMacro = mme;
    m_regOpName = r;
    m_regOpRgn.setDstHz(rgnHz);
    m_type = type;
}


void Operand::setInidirectDestination(
    DstModifier dstMod,
    const RegRef &reg,
    int16_t immediateOffset,
    const Region::Horz &rgnHz,
    Type type)
{
    m_kind = Operand::Kind::INDIRECT;

    m_regMathMacro = MathMacroExt::INVALID;
    m_regOpDstMod = dstMod;
    m_regOpReg = reg;
    m_regOpRgn.setDstHz(rgnHz);
    m_regOpName = RegName::GRF_R;
    m_regOpIndOff = immediateOffset;
    m_type = type;
}


void Operand::setImmediateSource(
    const ImmVal &val, Type type)
{
    m_kind = Operand::Kind::IMMEDIATE;

    m_regMathMacro = MathMacroExt::INVALID;
    m_immValue = val;
    m_type = type;
}


void Operand::setInidirectSource(
    SrcModifier srcMod,
    const RegRef &reg,
    int16_t immediateOffset,
    const Region &rgn,
    Type type)
{
    m_kind = Operand::Kind::INDIRECT;

    m_regMathMacro = MathMacroExt::INVALID;
    m_regOpSrcMod = srcMod;
    m_regOpReg = reg;
    m_regOpRgn = rgn;
    m_regOpName = RegName::GRF_R;
    m_regOpIndOff = immediateOffset;
    m_type = type;
}


void Operand::setDirectSource(
    SrcModifier srcMod,
    RegName r,
    const RegRef &reg,
    const Region &rgn,
    Type type)
{
    m_kind = Operand::Kind::DIRECT;

    m_regMathMacro = MathMacroExt::INVALID;
    m_regOpSrcMod = srcMod;
    m_regOpName = r;
    m_regOpReg = reg;
    m_regOpRgn = rgn;
    m_type = type;
}


void Operand::setMacroSource(
    SrcModifier srcMod,
    RegName r,
    const RegRef &reg,
    MathMacroExt acc,
    Region rgn,
    Type type)
{
    m_kind = Operand::Kind::MACRO;

    m_regOpSrcMod = srcMod;
    m_regOpName = r;
    m_regOpReg = reg;
    m_regMathMacro = acc;
    m_regOpRgn = rgn;
    m_type = type;
}


void Operand::setLabelSource(Block *blk, Type type)
{
    m_kind = Operand::Kind::LABEL;

    m_lblBlock = blk;
    m_type = type;
}


void Operand::setLabelSource(int32_t jipOrUip, Type type)
{
    m_kind = Operand::Kind::LABEL;

    m_immValue = jipOrUip;
    m_type = type;
}


static constexpr RegRef R0_0(0, 0);

const Operand Operand::DST_REG_IP_D(
    DstModifier::NONE,
    RegName::ARF_IP,
    R0_0,
    Region::Horz::HZ_1,
    Type::D);

const Operand Operand::SRC_REG_IP_D(
    SrcModifier::NONE,
    RegName::ARF_IP,
    R0_0,
    Region::SRC010,
    Type::D);


const Operand Operand::DST_REG_IP_UD(
    DstModifier::NONE,
    RegName::ARF_IP,
    R0_0,
    Region::Horz::HZ_1,
    Type::UD);

const Operand Operand::SRC_REG_IP_UD(
    SrcModifier::NONE,
    RegName::ARF_IP,
    R0_0,
    Region::SRC010,
    Type::UD);

const Operand Operand::DST_REG_NULL_UD(
    DstModifier::NONE,
    RegName::ARF_NULL,
    R0_0,
    Region::Horz::HZ_1,
    Type::UD);

const Operand Operand::SRC_REG_NULL_UD(
    SrcModifier::NONE,
    RegName::ARF_NULL,
    R0_0,
    Region::SRC010,
    Type::UD);

const Operand Operand::SRC_REG_NULL_UB(
    SrcModifier::NONE,
    RegName::ARF_NULL,
    R0_0,
    Region::SRC010,
    Type::UB);
