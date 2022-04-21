/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Kernel.hpp"
#include "../IR/Messages.hpp"

using namespace iga;

Kernel::Kernel(const Model &model)
  : m_model(model)
  , m_mem(4096)
{
}

Kernel::~Kernel()
{
    // Since in a kernel blocks are allocated using the memory pool,
    // when the Kernel is freed, the memory pool is deleted and destructors
    // for Blocks are never called.  This means the InstList's memory pools
    // was never deleted and we need to do it here.
    //
    // Also, Instruction's fields with destructors need running
    // (e.g. m_comment)
    for (Block *bb : m_blocks) {
        bb->~Block();
    }
}

size_t Kernel::getInstructionCount() const
{
    size_t n = 0;
    for (const Block *b : getBlockList()) {
        n += b->getInstList().size();
    }
    return n;
}


Block *Kernel::createBlock()
{
    return new(&m_mem)Block();
}


void Kernel::appendBlock(Block *blk)
{
    m_blocks.push_back(blk);
}


Instruction *Kernel::createBasicInstruction(
    const OpSpec& os,
    const Predication& predOpnd,
    const RegRef& freg,
    ExecSize execSize,
    ChannelOffset chOff,
    MaskCtrl mc,
    FlagModifier condMod,
    Subfunction sf)
{
    Instruction *inst = new(&m_mem)Instruction(os, execSize, chOff, mc);
    inst->setSubfunction(sf);

    inst->setPredication(predOpnd);
    inst->setFlagModifier(condMod);
    inst->setFlagReg(freg);

    return inst;
}


Instruction *Kernel::createBranchInstruction(
    const OpSpec &os,
    const Predication &predOpnd,
    const RegRef &flagReg,
    ExecSize execSize,
    ChannelOffset chOff,
    MaskCtrl ectr,
    Subfunction sf)
{
    Instruction *inst = new(&m_mem)Instruction(
        os,
        execSize,
        chOff,
        ectr);
    if (os.supportsSubfunction())
        inst->setSubfunction(sf); // only branch control for now

    inst->setPredication(predOpnd);
    inst->setFlagReg(flagReg);

    return inst;
}


Instruction *Kernel::createSendInstruction(
    const OpSpec &op,
    SFID sfid,
    const Predication &predOpnd,
    const RegRef &flagReg,
    ExecSize execSize,
    ChannelOffset chOff,
    MaskCtrl ectr,
    const SendDesc &exDesc,
    const SendDesc &desc)
{
    Instruction *inst = new(&m_mem)Instruction(
        op,
        execSize,
        chOff,
        ectr);
    inst->setSubfunction(sfid);

    inst->setPredication(predOpnd);
    inst->setFlagReg(flagReg);

    inst->setExtMsgDesc(exDesc);
    inst->setMsgDesc(desc);

    ///////////////////////////////////////////////////////////////////////////
    // make a best effort to set payload lengths
    int dstLen = -1, src0Len = -1, src1Len = -1;
    //
    Platform p = inst->getOpSpec().platform;
    bool immDescsHaveLens = desc.isImm();
    if (immDescsHaveLens) {
        dstLen = (int)((desc.imm >> 20) & 0x1F);
    } else if (desc.isImm()) {
        // if we at least have the SFID and an immediate descriptor
        // try and deduce it from descriptor details we make a
        // best-effort for exDesc, but may not need it specifically
        PayloadLengths lens(
            p,
            sfid,
            inst->getExecSize(),
            desc.imm);
        dstLen = lens.dstLen;
    }
    inst->setDstLength(dstLen);
    //
    if (src0Len < 0 && immDescsHaveLens) {
        src0Len = (int)((desc.imm >> 25) & 0xF);
    }
    inst->setSrc0Length(src0Len);
    //
    if (src1Len < 0) {
        // copy Src1.Length if no one else set it
        bool immExDescHasSrc1Len = exDesc.isImm();
        // ExBSO case will be handled by decoder/parser
        //
        // Src1.Length is imm xlen ExDesc[10:6] when ...:
        //   - ICL/TGL/XeHP: IsImm()
        //   - XeHPG+: never (always EU[96:92] for imm descs)
        immExDescHasSrc1Len &= p < Platform::XE_HPG;
        if (immExDescHasSrc1Len) {
            src1Len = (exDesc.imm >> 6) & 0x1F;
        }
    }
    inst->setSrc1Length(src1Len);

    return inst;
}



Instruction *Kernel::createNopInstruction()
{
    Instruction *inst = new(&m_mem)Instruction(
        m_model.lookupOpSpec(Op::NOP),
        ExecSize::SIMD1,
        ChannelOffset::M0,
        MaskCtrl::NORMAL);

    const Predication predOpnd;
    inst->setPredication(predOpnd);
    inst->setFlagModifier(FlagModifier::NONE);
    inst->setFlagReg(REGREF_ZERO_ZERO);

    return inst;
}


Instruction *Kernel::createIllegalInstruction()
{
    Instruction *inst = new(&m_mem)Instruction(
        m_model.lookupOpSpec(Op::ILLEGAL),
        ExecSize::SIMD1,
        ChannelOffset::M0,
        MaskCtrl::NORMAL);

    const Predication predOpnd;
    inst->setPredication(predOpnd);
    inst->setFlagModifier(FlagModifier::NONE);
    inst->setFlagReg(REGREF_ZERO_ZERO);

    return inst;
}

static Instruction *createSyncInstruction(
    SWSB &sw, const OpSpec &ops, SyncFC sf, MemManager &mem)
{
    Instruction *inst = new(&mem)Instruction(
        ops,
        ExecSize::SIMD1,
        ChannelOffset::M0,
        MaskCtrl::NOMASK);
    inst->setSubfunction(sf);

    const Predication predOpnd;
    inst->setPredication(predOpnd);
    inst->setFlagModifier(FlagModifier::NONE);
    inst->setFlagReg(REGREF_ZERO_ZERO);
    inst->setSWSB(sw);
    inst->setSource(SourceIndex::SRC0, Operand::SRC_REG_NULL_UD);

    return inst;
}
Instruction *Kernel::createSyncNopInstruction(SWSB sw)
{
    return createSyncInstruction(sw,
        m_model.lookupOpSpec(Op::SYNC), SyncFC::NOP, m_mem);
}
Instruction *Kernel::createSyncAllRdInstruction(SWSB sw)
{
    return createSyncInstruction(sw,
        m_model.lookupOpSpec(Op::SYNC), SyncFC::ALLRD, m_mem);
}
Instruction *Kernel::createSyncAllWrInstruction(SWSB sw)
{
    return createSyncInstruction(sw,
        m_model.lookupOpSpec(Op::SYNC), SyncFC::ALLWR, m_mem);
}
