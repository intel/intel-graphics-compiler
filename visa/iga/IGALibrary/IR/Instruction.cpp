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
#include "Instruction.hpp"
#include "Types.hpp"
#include "IRChecker.hpp"
#include "../Frontend/Formatter.hpp"

#include <sstream>

using namespace iga;

void Instruction::setSubfunction(Subfunction sf) {
    IGA_ASSERT(!getOpSpec().supportsSubfunction() || sf.isValid(),
        "Instruction requires subfunction (and none set)");
    IGA_ASSERT(getOpSpec().supportsSubfunction() || !sf.isValid(),
        "Instruction given subfunction (and forbids one)");
    m_sf = sf;
}


void Instruction::setDirectDestination(
    DstModifier dstMod,
    RegName rnm,
    RegRef reg,
    Region::Horz rgnH,
    Type type)
{
    if (getOpSpec().isSendOrSendsFamily() &&
        m_sendDstLength < 0 &&
        rnm == RegName::ARF_NULL)
    {
        m_sendDstLength = 0;
    }
    m_dst.setDirectDestination(dstMod, rnm, reg, rgnH, type);
}


void Instruction::setMacroDestination(
    DstModifier dstMod,
    RegName rnm,
    RegRef reg,
    MathMacroExt mme,
    Region::Horz rgnHz,
    Type type)
{
    m_dst.setMacroDestination(dstMod, rnm, reg, mme, rgnHz, type);
}


void Instruction::setInidirectDestination(
    DstModifier dstMod,
    RegRef reg,
    int16_t addrImmOff,
    Region::Horz rgnH,
    Type type)
{
    m_dst.setInidirectDestination(dstMod, reg, addrImmOff, rgnH, type);
}


void Instruction::setDirectSource(
    SourceIndex srcIx,
    SrcModifier srcMod,
    RegName rnm,
    RegRef reg,
    Region rgn,
    Type type)
{
    unsigned ix = static_cast<unsigned>(srcIx);
    if (getOpSpec().isSendOrSendsFamily() &&
        rnm == RegName::ARF_NULL)
    {
        // send with a null operand must have a 0 length
        // we only check this if we didn't get the length via the
        // descriptor
        if (ix == 0 && m_sendSrc0Len < 0)
            m_sendSrc0Len = 0;
        else if (ix == 1 && m_sendSrc1Length < 0)
            m_sendSrc1Length = 0;
    }
    m_srcs[ix].setDirectSource(srcMod, rnm, reg, rgn, type);
}


void Instruction::setSource(
    SourceIndex srcIx,
    const Operand &op)
{
    unsigned ix = static_cast<unsigned>(srcIx);
    if (getOpSpec().isSendOrSendsFamily() &&
        op.getKind() == Operand::Kind::DIRECT &&
        op.getDirRegName() == RegName::ARF_NULL)
    {
        // send with a null operand must have a 0 length
        // we only check this if we didn't get the length via the
        // descriptor
        if (ix == 0 && m_sendSrc0Len < 0)
            m_sendSrc0Len = 0;
        else if (ix == 1 && m_sendSrc1Length < 0)
            m_sendSrc1Length = 0;
    }
    m_srcs[ix] = op;
}


void Instruction::setMacroSource(
    SourceIndex srcIx,
    SrcModifier srcMod,
    RegName rName,
    RegRef reg,
    MathMacroExt acc,
    Region rgn,
    Type type)
{
    unsigned ix = static_cast<unsigned>(srcIx);
    m_srcs[ix].setMacroSource(srcMod, rName, reg, acc, rgn, type);
}

void Instruction::setInidirectSource(
    SourceIndex srcIx,
    SrcModifier srcMod,
    RegRef reg,
    int16_t immediateOffset,
    Region rgn,
    Type type)
{
    unsigned ix = static_cast<unsigned>(srcIx);
    m_srcs[ix].setInidirectSource(srcMod, reg, immediateOffset, rgn, type);
}


void Instruction::setImmediateSource(
    SourceIndex srcIx, const ImmVal &val, Type type)
{
    unsigned ix = static_cast<unsigned>(srcIx);
    m_srcs[ix].setImmediateSource(val, type);
}


void Instruction::setLabelSource(SourceIndex srcIx, int32_t pc, Type type)
{
    unsigned ix = static_cast<unsigned>(srcIx);
    m_srcs[ix].setLabelSource(pc, type);
}
void Instruction::setLabelSource(SourceIndex srcIx, Block *block, Type type)
{
    unsigned ix = static_cast<unsigned>(srcIx);
    m_srcs[ix].setLabelSource(block, type);
}


bool Instruction::isMacro() const {
    return is(Op::MADM) || (is(Op::MATH) && IsMacro(m_sf.math));
}


bool Instruction::isMovWithLabel() const {
    return (getOp() == Op::MOV &&
        getSource(0).getKind() == Operand::Kind::LABEL);
}

void Instruction::validate() const
{
    iga::SanityCheckIR(*this);
}


std::string Instruction::str(Platform pltfm) const
{
    ErrorHandler eh;
    std::stringstream ss;
    // TODO: see if we can wrestle the Platform out of this interface
    //  (or pass it as an argument to str())
    FormatOpts fopt(pltfm);
    fopt.setSWSBEncodingMode(Model::LookupModel(pltfm)->getSWSBEncodeMode());
    FormatInstruction(eh, ss, fopt, *this);
    return ss.str();
}

static unsigned getSourceCountBrc(const Instruction &i)
{
    // brc (..) IMM IMM
    // brc (..) REG
    switch (i.getSource(0).getKind()) {
    case Operand::Kind::DIRECT:
    case Operand::Kind::INDIRECT:
        return 1;
    default:
        return 2;
    }
}

unsigned Instruction::getSourceCount() const
{
    // BRC can have 1 or 2 operands, everyone else is simple
    if (is(Op::BRC)) {
        return getSourceCountBrc(*this);
    } else {
        return getOpSpec().getSourceCount(getSubfunction());
    }
}