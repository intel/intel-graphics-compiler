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
#include "../Frontend/Formatter.hpp"
#include "IRChecker.hpp"
#include <sstream>

using namespace iga;

void Instruction::setDirectDestination(
    DstModifier dstMod,
    RegName rType,
    RegRef reg,
    Region::Horz rgnH,
    Type type)
{
    m_dst.setDirectDestination(dstMod, rType, reg, rgnH, type);
}


void Instruction::setMacroDestination(
    DstModifier dstMod,
    RegName rName,
    RegRef reg,
    MathMacroExt mme,
    Region::Horz rgnHz,
    Type type)
{
    m_dst.setMacroDestination(dstMod, rName, reg, mme, rgnHz, type);
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
    RegName rType,
    RegRef reg,
    Region rgn,
    Type type)
{
    unsigned ix = static_cast<unsigned>(srcIx);
    m_srcs[ix].setDirectSource(srcMod, rType, reg, rgn, type);
}


void Instruction::setSource(
    SourceIndex srcIx,
    const Operand &op)
{
    unsigned ix = static_cast<unsigned>(srcIx);
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
    fopt.setSWSBEncodingMode(iga::Model::LookupModel(pltfm)->getSWSBEncodeMode());
    FormatInstruction(eh, ss, fopt, *this);
    return ss.str();
}

unsigned Instruction::getSourceCountBrc() const
{
    // brc (..) IMM IMM
    // brc (..) REG
    switch (getSource(0).getKind()) {
    case Operand::Kind::DIRECT:
    case Operand::Kind::INDIRECT:
        return 1;
    default:
        return 2;
    }
}
