/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Instruction.hpp"
#include "Types.hpp"
#include "Checker/IRChecker.hpp"
#include "../Frontend/Formatter.hpp"

#include <sstream>

using namespace iga;

void Instruction::setSubfunction(Subfunction sf) {
    IGA_ASSERT(!getOpSpec().supportsSubfunction() || sf.isValid(),
        "instruction given invalid subfunction");
    IGA_ASSERT(getOpSpec().supportsSubfunction() || !sf.isValid(),
        "instruction forbids subfunction");
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
        if (ix == 0 && m_sendSrc0Length < 0)
            m_sendSrc0Length = 0;
        else if (ix == 1 && m_sendSrc1Length < 0)
            m_sendSrc1Length = 0;
    }
    m_srcs[ix].setDirectSource(srcMod, rnm, reg, rgn, type);
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
    RegName regName,
    RegRef reg,
    int16_t immediateOffset,
    Region rgn,
    Type type)
{
    unsigned ix = static_cast<unsigned>(srcIx);
    m_srcs[ix].setInidirectSource(srcMod, regName, reg, immediateOffset, rgn, type);
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


void Instruction::setSource(SourceIndex srcIx, const Operand &op)
{
    unsigned ix = static_cast<unsigned>(srcIx);
    if (getOpSpec().isSendOrSendsFamily() &&
        op.getKind() == Operand::Kind::DIRECT &&
        op.getDirRegName() == RegName::ARF_NULL)
    {
        // send with a null operand must have a 0 length
        // we only check this if we didn't get the length via the
        // descriptor
        if (ix == 0 && m_sendSrc0Length < 0)
            m_sendSrc0Length = 0;
        else if (ix == 1 && m_sendSrc1Length < 0)
            m_sendSrc1Length = 0;
    }
    m_srcs[ix] = op;
}

void Instruction::setExtMsgDesc(const SendDesc &msg) {
    m_exDesc = msg;
}
void Instruction::setMsgDesc(const SendDesc &msg) {
    m_desc = msg;
}


const Model &Instruction::model() const {
    return Model::LookupModelRef(platform());
}

SWSB::InstType Instruction::getSWSBInstType(SWSB_ENCODE_MODE mode) const {
    if (mode == SWSB_ENCODE_MODE::SWSBInvalidMode)
        return SWSB::InstType::UNKNOWN;

    if (getOpSpec().isSendOrSendsFamily())
        return SWSB::InstType::SEND;

    if (is(Op::MATH))
        return SWSB::InstType::MATH;


    if (getOpSpec().isDpasFamily()) {
        return SWSB::InstType::DPAS;
    }

    return SWSB::InstType::OTHERS;
}

bool Instruction::isMacro() const {
    return is(Op::MADM) || (is(Op::MATH) && IsMacro(m_sf.math));
}

bool Instruction::isDF() const {
    auto isDPType = [](Type ty) {
        return TypeIs64b(ty) && TypeIsFloating(ty);
    };
    if (m_dst.getType() != Type::INVALID)
        if (isDPType(m_dst.getType()))
            return true;
    for (size_t i = 0; i < getSourceCount(); ++i) {
        if (m_srcs[i].getType() != Type::INVALID) {
            if (isDPType(m_srcs[i].getType()))
                return true;
        }
    }
    return false;
}

bool Instruction::isMovWithLabel() const {
    return (getOp() == Op::MOV &&
        getSource(0).getKind() == Operand::Kind::LABEL);
}


void Instruction::validate() const
{
    iga::SanityCheckIR(*this);
}

std::string Instruction::str() const
{
    ErrorHandler eh;
    std::stringstream ss;
    FormatOpts fopt(model());
    fopt.setSWSBEncodingMode(
        Model::LookupModelRef(getOpSpec().platform).getSWSBEncodeMode());
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

SendDesc Instruction::getExtMsgDescriptor() const
{
    return m_exDesc;
}
SendDesc Instruction::getMsgDescriptor() const
{
    return m_desc;
}
