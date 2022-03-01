/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "InstBuilder.hpp"

using namespace iga;

void InstBuilder::clearInstState()
{
    m_predication.function = PredCtrl::NONE;
    m_predication.inverse = false;

    m_flagReg = REGREF_ZERO_ZERO;

    m_opSpec = nullptr;

    m_execSize = ExecSize::SIMD1;
    m_chOff = ChannelOffset::M0;
    m_subfunc = InvalidFC::INVALID; // invalid
    m_maskCtrl = MaskCtrl::NORMAL;

    m_flagModifier = FlagModifier::NONE;

    m_dstModifier = DstModifier::NONE;

    m_dst.reset();
    for (auto &m_src : m_srcs)
        m_src.reset();

    m_nSrcs = 0;

    m_exDesc.imm = 0;
    m_desc.imm = 0;

    m_sendSrc0Len = m_sendSrc1Len = -1;

    m_instOpts.clear();

    m_depInfo = SWSBInfo();

    m_comment.clear();
}

void InstBuilder::validateScrImmType(Type ty, const Loc& loc)
{
}

void InstBuilder::validateOperandInfo(const OperandInfo &opInfo)
{
#ifdef _DEBUG
    // some sanity validation
    switch (opInfo.kind) {
    case Operand::Kind::DIRECT:
    case Operand::Kind::MACRO:
    case Operand::Kind::INDIRECT:
    case Operand::Kind::IMMEDIATE:
    case Operand::Kind::LABEL:
        break;
    default:
        IGA_ASSERT_FALSE("OperandInfo::kind: invalid value");
        break;
    }
#endif
}
