/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "RegDeps.hpp"
#include "../asserts.hpp"
#include "../bits.hpp"

#include <limits>
#include <sstream>
#include <cstring>

using namespace iga;

static DEP_CLASS getClassFromPipeType(DEP_PIPE type, const OpSpec& opspec)
{
    if (opspec.is(Op::SYNC) || opspec.op == Op::ILLEGAL)
        return DEP_CLASS::OTHER;

    switch(type) {
        case DEP_PIPE::NONE:
        case DEP_PIPE::SHORT:
        case DEP_PIPE::LONG:
        case DEP_PIPE::CONTROL_FLOW:
            return DEP_CLASS::IN_ORDER;

        case DEP_PIPE::SEND:
        case DEP_PIPE::MATH:
            return DEP_CLASS::OUT_OF_ORDER;

        case DEP_PIPE::FLOAT:
        case DEP_PIPE::INTEGER:
        case DEP_PIPE::LONG64:
            return DEP_CLASS::IN_ORDER;

        case DEP_PIPE::DPAS:
            return DEP_CLASS::OUT_OF_ORDER;
    }
    return DEP_CLASS::NONE;
}

static void setDEPPipeClass_SingleDistPipe(DepSet &dep, const Instruction &inst)
{
    auto opsec = inst.getOpSpec();
    dep.setDepPipe(DEP_PIPE::SHORT);
    if (opsec.is(Op::MATH))
    {
        dep.setDepPipe(DEP_PIPE::MATH);
    }
    else if (opsec.isSendOrSendsFamily())
    {
        dep.setDepPipe(DEP_PIPE::SEND);
    }
    else if (opsec.isBranching())
    {
        dep.setDepPipe(DEP_PIPE::CONTROL_FLOW);
    }
    else
    {
        for (uint32_t i = 0; i < inst.getSourceCount(); ++i)
        {
            const auto &src = inst.getSource(i);
            if (src.getType() == Type::DF ||
                src.getType() == Type::Q ||
                src.getType() == Type::UQ)
            {
                dep.setDepPipe(DEP_PIPE::LONG);
                break;
            }
        }
        if (opsec.supportsDestination())
        {
            const auto &dst = inst.getDestination();
            if (dst.getType() == Type::DF   ||
                dst.getType() == Type::Q    ||
                dst.getType() == Type::UQ)
            {
                dep.setDepPipe(DEP_PIPE::LONG);
            }
        }
    }

    dep.setDepClass(getClassFromPipeType(dep.getDepPipe(), opsec));
}

// XeHP+
static void setSendPipeType(
    DEP_PIPE& pipe_type,
    const Instruction &inst,
    const Model &model)
{
    assert(inst.getOpSpec().isSendOrSendsFamily());
    pipe_type = DEP_PIPE::SEND;
}


// XeHP
static void setDEPPipeClass_ThreeDistPipe(
    DepSet &dep, const Instruction &inst, const Model &model)
{
    auto opsec = inst.getOpSpec();

    DEP_PIPE pipe_type = DEP_PIPE::NONE;
    if (opsec.is(Op::MATH))
    {
        pipe_type = DEP_PIPE::MATH;
    }
    else if (opsec.isSendOrSendsFamily())
    {
        setSendPipeType(pipe_type, inst, model);
    }
    else if (opsec.isDpasFamily())
    {
        pipe_type = DEP_PIPE::DPAS;
    }
    else if (opsec.isBranching())
    {
        pipe_type = DEP_PIPE::INTEGER;
    }
    else
    {
        // In order instruction:
        // if destination type is FP32/FP16/BF16 then it goes to float pipe,
        // if destination type is int32 / 16 / 8 it goes to integer pipe and
        // if destination or source type is int64/FP64 then it goes to long pipe

        // for conversion instructions float2int goes to integer pipe and int2float goes to float pipe,
        // anything2doublefloat goes to long pipe and doublefloat2anything goes to long pipe

        // If destination type is null then source0 data type will determine the pipe
        Type inst_type = Type::INVALID;
        if (opsec.supportsDestination())
            inst_type = inst.getDestination().getType();
        else if (inst.getSourceCount())
            inst_type = inst.getSource(0).getType();

        if (inst_type != Type::INVALID) {
            if (TypeIs64b(inst_type))
                pipe_type = DEP_PIPE::LONG64;
            else if (TypeIsFloating(inst_type))
                pipe_type = DEP_PIPE::FLOAT;
            else
                pipe_type = DEP_PIPE::INTEGER;
        }

        for (uint32_t i = 0; i < inst.getSourceCount(); ++i)
        {
            const auto & src = inst.getSource(i);
            if (TypeIs64b(src.getType()))
            {
                pipe_type = DEP_PIPE::LONG64;
                break;
            }
        }
    }

    // default set to Integer pipe (e.g. NOP)
    if (pipe_type == DEP_PIPE::NONE)
        pipe_type = DEP_PIPE::INTEGER;

    dep.setDepClass(getClassFromPipeType(pipe_type, opsec));
    dep.setDepPipe(pipe_type);
}



static void setDEPPipeClass(
    SWSB_ENCODE_MODE enc_mode, DepSet &dep, const Instruction &inst, const Model& model)
{
    if (enc_mode == SWSB_ENCODE_MODE::SingleDistPipe)
        setDEPPipeClass_SingleDistPipe(dep, inst);
    else if (enc_mode == SWSB_ENCODE_MODE::ThreeDistPipe)
        setDEPPipeClass_ThreeDistPipe(dep, inst, model);
}

DepSet::DepSet(const InstIDs& inst_id_counter, const DepSetBuilder& dsb)
    : m_instruction(nullptr),
      m_dType(DEP_TYPE::NONE),
      m_hasIndirect(false),
      m_hasSR(false),
      m_dPipe(DEP_PIPE::NONE),
      m_dClass(DEP_CLASS::NONE),
      m_InstIDs(inst_id_counter.global,
                inst_id_counter.inOrder,
                inst_id_counter.floatPipe,
                inst_id_counter.intPipe,
                inst_id_counter.longPipe
      ),
      m_DB(dsb)
{
    m_bucketList.reserve(4);
    bits = new BitSet<>(dsb.getTOTAL_BITS());
}

uint32_t DepSet::getDPASOpsPerChan(Type src1_ty, Type src2_ty)
{
    // get OPS_PER_CHAN, the number of dot product operations per dword channel, depending on element type
    // for src1, src2 region calculation, only hf, bf, ub, b, u4, s4, u2, s2 should be given
    // mixed mode of int and float at src1/src2 are not allowed
    if (src1_ty == Type::HF || src1_ty == Type::BF) {
        IGA_ASSERT(src1_ty == src2_ty, "src1/src2 must have the same type");
        return 2;
    }
    else {
        // if both src1 and src2 are int2 or int4, than ops_per_chan will be 8
        int src1_size = TypeSizeInBits(src1_ty);
        int src2_size = TypeSizeInBits(src2_ty);
        // Type: ub, b, u4, s4, u2, s2
        IGA_ASSERT((src1_size <= 8), "OPS_PER_CHAN: unsupported type of src1");
        IGA_ASSERT((src2_size <= 8), "OPS_PER_CHAN: unsupported type of src2");
        if ((src1_size == 2 || src1_size == 4) && (src2_size == 2 || src2_size == 4))
            return 8;
        return 4;
    }
}

// lowBound - start register address offset in byte
// UpBound - upper register address offset in byte
uint32_t DepSet::getDPASSrcDepUpBound(unsigned idx, Type srcType, uint32_t execSize,
    uint32_t lowBound, uint32_t systolicDepth, uint32_t repeatCount, uint32_t opsPerChan)
{
    auto typeSizeInBits = TypeSizeInBitsWithDefault(srcType, 32);
    // elements_size is the size of total elements to be calculated in one operation
    uint32_t elements_size = execSize * typeSizeInBits / 8;

    uint32_t upBound = lowBound;
    if (idx == 0)
        upBound += elements_size * repeatCount;
    else if (idx == 1)
        upBound += elements_size * opsPerChan * systolicDepth;
    else
        upBound +=
            (repeatCount - 1) * opsPerChan * 8 * typeSizeInBits / 8 +  /* start offset of the last repeated block */
            opsPerChan * systolicDepth * typeSizeInBits / 8;           /* size of used register in last repeated block */

    return upBound;
}

void DepSet::getDpasSrcDependency(
    const Instruction &inst, RegRangeListType &reg_range, RegRangeListType& extra_regs, const Model& model)
{
    uint32_t execSize = static_cast<uint32_t>(inst.getExecSize());

    IGA_ASSERT(execSize == (m_DB.getGRF_BYTES_PER_REG() / 4),
        "Invalid ExecSize for this op");

    // check src operand and add the dependency
    uint32_t repeatCount = GetDpasRepeatCount(inst.getDpasFc());
    uint32_t systolicDepth = GetDpasSystolicDepth(inst.getDpasFc());
    uint32_t ops_per_chan = getDPASOpsPerChan(inst.getSource(1).getType(), inst.getSource(2).getType());

    for (unsigned srcIx = 0; srcIx < inst.getSourceCount(); ++srcIx) {
        const Operand &op = inst.getSource(srcIx);
        // the src0 could be null, in that case no need to set the dependency
        if (srcIx == 0 && op.getDirRegName() == RegName::ARF_NULL) {
            // if src0 is null, set the reg range to max() to specify its actually empty
            reg_range.push_back(std::make_pair(
                std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max()));
            continue;
        }
        IGA_ASSERT(op.getDirRegName() == RegName::GRF_R, "GRF or null required on this op");

        // calculate register region
        auto tType = op.getType();
        auto typeSizeInBits = TypeSizeInBitsWithDefault(tType, 32);
        uint32_t lowBound = addressOf(op.getDirRegName(), op.getDirRegRef(), typeSizeInBits);
        uint32_t upBound = getDPASSrcDepUpBound(srcIx, tType, execSize, lowBound, systolicDepth, repeatCount, ops_per_chan);
        IGA_ASSERT(upBound >= lowBound,
            "source region footprint computation got it wrong: upBound is less than lowBound");

        uint32_t startRegNum = lowBound / m_DB.getGRF_BYTES_PER_REG();
        uint32_t upperRegNum = (upBound - 1) / m_DB.getGRF_BYTES_PER_REG();
        reg_range.push_back(std::make_pair(startRegNum, upperRegNum));
        // calculate extra_regs for HW workaround: src1 always have 8 register footpring
        if (model.platform == Platform::XE_HP
            ) {
            if (srcIx == 1) {
                uint32_t extraUpBound = lowBound + m_DB.getGRF_BYTES_PER_REG() * 8;
                uint32_t extraUpRegNum = (extraUpBound - 1) / m_DB.getGRF_BYTES_PER_REG();
                if (extraUpRegNum >= m_DB.getGRF_REGS())
                    IGA_FATAL("IGA RegDeps: DPAS src1 out of bounds due to HW WA");
                extra_regs.push_back(std::make_pair(startRegNum, extraUpRegNum));
            }
        }
    }
}

void DepSet::addDependency(const RegRangeType& reg_range)
{
    for (uint32_t regNum = reg_range.first; regNum <= reg_range.second; regNum++)
    {
        addGrf(regNum);
        addToBucket(regNum);
    }

    //Using one of the special registers to add write dependency in to special bucket
    //This way it will always check that implicit dependency
    m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
}

void DepSet::addDependency(const RegRangeListType& reg_range)
{
    for (auto pair : reg_range) {
        // when range is max(), which means it's null, skip it
        if (pair.first == std::numeric_limits<uint32_t>::max())
            continue;
        for (uint32_t regNum = pair.first; regNum <= pair.second; regNum++) {
            addGrf(regNum);
            addToBucket(regNum);
        }
    }

    //Using one of the special registers to add read dependency in to special bucket
    //This way it will always check that implicit dependency
    m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
}

static bool helper_isLastDpas(const Instruction &dpas, const Instruction &next_inst)
{
    if (!next_inst.getOpSpec().isDpasFamily())
        return true;

    // DPAS and DPASW should not be in the same macro
    if (next_inst.getOp() != dpas.getOp())
        return true;

    // DPAS with different CtrlMask (NoMask and no NoMask) cannot be in the same macro
    if (next_inst.getMaskCtrl() != dpas.getMaskCtrl())
        return true;

    // DPAS with different execution mask cannot be in the same macro
    if (next_inst.getChannelOffset() != dpas.getChannelOffset())
        return true;

    // dpas with different depth should not be in the same macro
    // Note that different repeat count is allowed in the same macro
    uint32_t dpasSystolicDepth = GetDpasSystolicDepth(dpas.getDpasFc());
    uint32_t nextSystolicDepth = GetDpasSystolicDepth(next_inst.getDpasFc());
    if (dpasSystolicDepth != nextSystolicDepth)
        return true;

    // dpas in the same macro must have the same datatypes in all src and dst
    assert(dpas.getSourceCount() == next_inst.getSourceCount());
    for (size_t i = 0; i < dpas.getSourceCount(); ++i) {
        if (dpas.getSource(i).getType() != next_inst.getSource(i).getType())
            return true;
    }
    if (dpas.getDestination().getType() != next_inst.getDestination().getType())
        return true;

    // dpas in the same macro must have the same src1 register
    if (dpas.getSource(1).getDirRegRef() != next_inst.getSource(1).getDirRegRef())
        return true;

    return false;
}

std::pair<DepSet*, DepSet*> DepSetBuilder::createDPASSrcDstDepSet(
    const InstList& insList, InstListIterator instIt, const InstIDs& inst_id_counter,
    size_t& dpasCnt, SWSB_ENCODE_MODE enc_mode) {

    // create DepSet for input
    DepSet* inps = new DepSet(inst_id_counter, *this);
    mAllDepSet.push_back(inps);
    inps->setDepType(DEP_TYPE::READ);
    setDEPPipeClass(enc_mode, *inps, **instIt, mPlatformModel);

    // create DepSet for output
    DepSet *oups = new DepSet(inst_id_counter, *this);
    mAllDepSet.push_back(oups);
    oups->setDepType(DEP_TYPE::WRITE);
    setDEPPipeClass(enc_mode, *oups, **instIt, mPlatformModel);

    // identify dpas macro
    dpasCnt = 1;
    Instruction* cur_inst = *instIt;

    typedef DepSet::RegRangeListType SrcRegRangeType;
    typedef DepSet::RegRangeType DstRegRangeType;

    // do the first instruction
    SrcRegRangeType src_range;
    SrcRegRangeType extra_src_range;
    inps->getDpasSrcDependency(*cur_inst, src_range, extra_src_range, mPlatformModel);
    inps->addDependency(src_range);
    inps->addDependency(extra_src_range);

    DstRegRangeType dst_range;
    oups->getDpasDstDependency(*cur_inst, dst_range);
    oups->addDependency(dst_range);

    // Track the used registers of src and dst by two BitSet. Dependency in the
    // macro is not allowed, if there is dependency then cannot be in the same macro
    // dpas only use grf so consider the grf only
    BitSet<> srcbits(getGRF_LEN());
    BitSet<> dstbits(getGRF_LEN());
    auto setBits = [this](BitSet<>& bit_set, uint32_t start_reg, uint32_t upper_reg) {
        // if the given register is max(), which means it's no register,
        // then no need to do anything
        if (start_reg == std::numeric_limits<uint32_t>::max() ||
            upper_reg == std::numeric_limits<uint32_t>::max())
            return;
        for (uint32_t i = start_reg; i <= upper_reg; ++i) {
            uint32_t grf_addr = i * getGRF_BYTES_PER_REG();
            bit_set.set(grf_addr, getGRF_BYTES_PER_REG());
        }
    };
    // set the bits for future check the dependency
    for (auto regs : src_range) {
        setBits(srcbits, regs.first, regs.second);
    }
    setBits(dstbits, dst_range.first, dst_range.second);

    // check if the given register ranges having intersection
    auto hasIntersect = [&setBits, this]
            (const DepSet::RegRangeType& rr1, const DepSet::RegRangeType& rr2) {
        BitSet<> rr1bits(getGRF_LEN());
        BitSet<> rr2bits(getGRF_LEN());
        setBits(rr1bits, rr1.first, rr1.second);
        setBits(rr2bits, rr2.first, rr2.second);

        return rr1bits.intersects(rr2bits);
    };

    // check if the having overlap on given register range.
    // If rr1 and rr2 footprint are all the same, then return true.
    // If rr1 and rr2 has overlap but not entirely the same, then return false.
    // If no dependency, return true
    auto hasEntireOverlapOrNoOverlap = [&setBits, &hasIntersect, this]
            (const DepSet::RegRangeType& rr1, const DepSet::RegRangeType& rr2) {
        BitSet<> rr1bits(getGRF_LEN());
        BitSet<> rr2bits(getGRF_LEN());
        setBits(rr1bits, rr1.first, rr1.second);
        setBits(rr2bits, rr2.first, rr2.second);

        if (hasIntersect(rr1, rr2))
            return rr1bits.equal(rr2bits);
        else
            return true;
    };

    // check if the instruction having internal dependency
    // Instruction having internal dependency on dst to src is not allowed to be
    // in a macro.
    // Only for dpas8x8, insternal dep on dst and src0 is allowed, but only when src0 and
    // dst memory footprin is entirely the same
    auto hasInternalDep = [&hasEntireOverlapOrNoOverlap, &hasIntersect](
                                const DstRegRangeType& dst_range,
                                const SrcRegRangeType& src_range,
                                bool isDepth8) {
        if (hasIntersect(dst_range, src_range[1]))
            return true;

        if (hasIntersect(dst_range, src_range[2]))
            return true;

        // if src0 is null, then must not have dependency between dst and src0, skip it
        if (src_range[0].first != std::numeric_limits<uint32_t>::max()) {
            if (!isDepth8 && hasIntersect(dst_range, src_range[0]))
                return true;

            // for depth 8 dpas, sr0 and dst having the same foot print is treated
            // as no internal dependency for other rep_count, having intersect is
            // internal dependency
            if (isDepth8 && !hasEntireOverlapOrNoOverlap(dst_range, src_range[0]))
                return true;
        }
        return false;
    };
    // if the first dpas has internal dependency, then itself form
    // a macro, otherwise keep checking until lastDpas
    if (!hasInternalDep(dst_range, src_range,
        GetDpasSystolicDepth(cur_inst->getDpasFc()) == 8))
    {

        // do the following instruction until the end of macro
        while (1) {
            ++instIt;
            if (instIt == insList.end())
                break;
            if (helper_isLastDpas(*cur_inst, **instIt))
                break;
            SrcRegRangeType new_src_range, new_extra_regs;
            DstRegRangeType new_dst_range;
            inps->getDpasSrcDependency(**instIt, new_src_range, new_extra_regs, mPlatformModel);
            oups->getDpasDstDependency(**instIt, new_dst_range);
            BitSet<> new_srcbits(getGRF_LEN());
            BitSet<> new_dstbits(getGRF_LEN());
            for (auto regs : new_src_range) {
                setBits(new_srcbits, regs.first, regs.second);
            }
            setBits(new_dstbits, new_dst_range.first, new_dst_range.second);

            // if the new instruction having internal dependency, then
            // cannot be part of this macro
            if (hasInternalDep(new_dst_range, new_src_range,
                GetDpasSystolicDepth(cur_inst->getDpasFc()) == 8))
                break;

            // if the new instruction having WAR/RAW/WAW to previous, then cannot be part of this macro
            if (srcbits.intersects(new_dstbits) ||
                dstbits.intersects(new_srcbits) ||
                dstbits.intersects(new_dstbits))
                break;

            // Add ATOMIC to dpas inside the macro, except for the last dpas
            cur_inst->addInstOpt(InstOpt::ATOMIC);

            cur_inst = *instIt;

            // add the new dependency
            inps->addDependency(new_src_range);
            inps->addDependency(new_extra_regs);
            oups->addDependency(new_dst_range);
            srcbits.add(new_srcbits);
            dstbits.add(new_dstbits);

            ++dpasCnt;
        }
    }
    // let the last instruciton in the macro represent this DepSet
    inps->m_instruction = cur_inst;
    oups->m_instruction = cur_inst;

    return std::make_pair(inps, oups);
}

DepSet* DepSetBuilder::createSrcDepSet(const Instruction &i,
                                       const InstIDs& inst_id_counter,
                                       SWSB_ENCODE_MODE enc_mode)
{
    DepSet* inps = new DepSet(inst_id_counter, *this);
    mAllDepSet.push_back(inps);

    inps->m_instruction = &i;
    inps->setDepType(DEP_TYPE::READ);

    setDEPPipeClass(enc_mode, *inps, i, mPlatformModel);

    inps->setInputsFlagDep();
    inps->setInputsSrcDep();
    return inps;
}

void DepSet::addGrf(size_t reg)
{
    addGrfBytes(reg, 0, m_DB.getGRF_BYTES_PER_REG());
}

void DepSet::setInputsFlagDep()
{
    // does it read the flag register
    //  predication does this
    //  conditional modifier on 'sel' does this
    const Predication &pred = m_instruction->getPredication();
    const FlagModifier fm = m_instruction->getFlagModifier();
    bool readsFlagRegister =
        pred.function != PredCtrl::NONE ||
        m_instruction->getOp() == Op::SEL && fm != FlagModifier::NONE;
    if (readsFlagRegister)
    {
        // add the ARF offset from ExecMaskOffset
        // E.g.
        // (f1.0) op (16|M16) ...
        // is touching f1.1
        const RegRef &fr = m_instruction->getFlagReg();
        size_t fByteOff = (size_t)fr.regNum * m_DB.getARF_F_BYTES_PER_REG() +
            (size_t)fr.subRegNum * 2; // FIXME: magic number (needs some thought should be bytes per subreg)
        size_t execOff = 4 * (static_cast<size_t>(m_instruction->getChannelOffset()));
        fByteOff += execOff / 8; // move over by ARF offset
        size_t execSize = static_cast<size_t>(m_instruction->getExecSize());
        size_t addr = (size_t)m_DB.getARF_F_START() + fByteOff;
        addFBytes(addr, execSize / 8);
        m_bucketList.push_back(addr / m_DB.getBYTES_PER_BUCKET());
    }

    // immediate send descriptors
    if (m_instruction->getOpSpec().isSendOrSendsFamily()) {
        auto desc = m_instruction->getMsgDescriptor();
        if (desc.isReg()) {
            addA_D(desc.reg); // e.g. a0.0
        }
        auto exDesc = m_instruction->getExtMsgDescriptor();
        if (exDesc.isReg()) {
            addA_D(exDesc.reg); // e.g. a0.0
        }
    }
}

void DepSet::setInputsSrcDep()
{
    uint32_t execSize = static_cast<uint32_t>(m_instruction->getExecSize());

    // mac/mach has implicitly read to acc0
    if (m_instruction->getOp() == Op::MAC || m_instruction->getOp() == Op::MACH
        ) {
        setSrcRegion(
            RegName::ARF_ACC,
            RegRef(0, 0),
            Region::SRC110,
            execSize,
            16); // assume it's :w, though for acc access it actually does not matter,
                 // because its footprint will always count as acc0/acc1 pair
    }

    // For the instruction having no srcs, we still need to add ARF_CR to mark the dependency
    // This is for the case that:
    //      and (1|M0)               cr0.0<1>:ud   cr0.0<0;1,0>:ud   0xFFFFFFCF:ud    {A@1}
    //      nop                                                                       {A@1}
    // It's requested that the instruction following the one having architecture register (CR/CE/SR) access,
    // It must mark to sync with all pipes, even if it's a nop.
    // nop has no src and dst so we mark it here to force setting swsb if required
    //
    // For sync instructions we do not need to consider its dependency. Though it may still apply the above
    // case so still set ARF_CR to it.
    if (m_instruction->getSourceCount() == 0 ||
        m_instruction->getOpSpec().is(Op::SYNC)) {
        m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
        return;
    }

    // check all the source operands
    for (unsigned srcIx = 0, numSrcs = m_instruction->getSourceCount();
        srcIx < numSrcs;
        srcIx++)
    {
        const Operand &op = m_instruction->getSource(srcIx);
        auto tType = op.getType();
        uint32_t typeSizeInBits = TypeSizeInBitsWithDefault(tType,32);

        // possibly need to fixup the region
        Region rgn = op.getRegion();
        if (numSrcs == 3) {
            if (m_instruction->isMacro()) {
                // macros and sends are packed access only
                // madm (4) r3.acc1 r7.acc2 r9.acc4 r6.noacc
                rgn = Region::SRC110;
            } else {
                // ternary align 1 has some implicit regions that need filling in
                //
                // c.f. https://gfxspecs.intel.com/Predator/Home/Index/3017
                // "GEN10 Regioning Rules for Align1 Ternary Operations"
                //   1. Width is 1 when Vertical and Horizontal Strides are both zero (broadcast access).
                //   2. Width is equal to Vertical Stride when Horizontal Stride is zero.
                //   3. Width is equal to Vertical Stride/Horizontal Stride when both Strides are non-zero.
                //   4. Vertical Stride must not be zero if Horizontal Stride is non-zero.
                //      This implies Vertical Stride is always greater than Horizontal Stride.
                //   5. For Source 2, if Horizontal Stride is non-zero, then Width is the
                //      a register's width of elements (e.g. 8 for a 32-bit data type).
                //      Otherwise, if Horizontal Stride is 0, then so is the Vertical (and rule 1 applies).
                //      This means Vertical Stride is always 'Width' * 'Horizontal Stride'.
                if (srcIx < 2) {
                    // <V;H>
                    if (rgn.getVt() == Region::Vert::VT_0 && rgn.getHz() == Region::Horz::HZ_0) {
                        rgn.set(Region::Width::WI_1); // by #1
                    } else if (rgn.getHz() == Region::Horz::HZ_0) {
                        rgn.w = rgn.v; // by #2
                    } else if (rgn.getVt() != Region::Vert::VT_0 && rgn.getHz() != Region::Horz::HZ_0) {
                        rgn.w = rgn.v/rgn.h; // by #3
                    } else {
                        // error condition #4, just use vertical stride
                        // SPECIFY: should this be an assertion?
                        rgn.w = rgn.v;
                    }
                } else { // (srcIx == 2)
                    // <H>
                    // <H> -> <H;1,0>
                    // if (rgn.h == Region::HZ_0) {
                    //    rgn.w = Region::WI_1; // by #5 and #1
                    //} else {
                    //    rgn.w = static_cast<Region::Width>(GRF_BYTES_PER_REG/typeSize);  // by #5
                    //}
                    //rgn.v = static_cast<Region::Vert>(
                    //    static_cast<int>(rgn.w)*static_cast<int>(rgn.h)/typeSize); // by #5
                    rgn.set(
                        static_cast<Region::Vert>(static_cast<size_t>(rgn.h)),
                        Region::Width::WI_1,
                        Region::Horz::HZ_0);
                }
            }
        } else if (numSrcs == 2 && m_instruction->isMacro()) {
            // math macro
            rgn = Region::SRC110;
        } // not ternary, not macro

        switch (op.getKind()) {
        case Operand::Kind::DIRECT:
            if (m_instruction->getOpSpec().isSendOrSendsFamily()) {
                if (op.getDirRegName() == RegName::GRF_R) {
                    // send source GRF (not null reg)
                    int nregs =
                        (srcIx == 0) ?
                            m_instruction->getSrc0Length() :
                            m_instruction->getSrc1Length();
                    // if we can't tell the number of registers
                    // (e.g. the descriptor is in a register),
                    // then we must conservatively assume the worst (31)
                    if (nregs < 0)
                        nregs = 31;
                    uint32_t regNum = op.getDirRegRef().regNum;
                    for (uint32_t i = 0; i < (uint32_t)nregs; i++) {
                        if ((regNum + i) >= m_DB.getGRF_REGS()) {
                            break;
                        }
                        addGrf((size_t)regNum + i);
                        addToBucket(regNum + i);
                    }
                    addToBucket(m_DB.getBucketStart(RegName::ARF_CR));
                }
            } else {
                if (m_instruction->getOp() == Op::BRC) {
                    rgn = Region::SRC221;
                }

                setSrcRegion(
                    op.getDirRegName(),
                    op.getDirRegRef(),
                    rgn,
                    execSize,
                    typeSizeInBits);
            }
            break;
        case Operand::Kind::MACRO: {
            setSrcRegion(
                op.getDirRegName(),
                op.getDirRegRef(),
                rgn,
                execSize,
                typeSizeInBits);
            auto mme = op.getMathMacroExt();
            if (mme != MathMacroExt::NOMME && mme != MathMacroExt::INVALID) {
                int mmeNum = static_cast<int>(op.getMathMacroExt()) -
                    static_cast<int>(MathMacroExt::MME0);
                setSrcRegion(
                    RegName::ARF_ACC,
                    RegRef(mmeNum, 0),
                    Region::SRC110,
                    execSize,
                    typeSizeInBits);
            }
            break;
        }
        case Operand::Kind::INDIRECT: {
            setHasIndirect();
            setDepType(DEP_TYPE::READ_ALWAYS_INTERFERE);
            auto rgn = op.getRegion();
            if (rgn.getVt() == Region::Vert::VT_VxH) {
                // VxH or Vx1 mode
                //   op (K)  dst   r[a0.0]<W,1>:w  (reads K/W elements)
                //   op (K)  dst   r[a0.0]<1,0>:w  (reads K/W elements)
                setSrcRegion(
                    RegName::ARF_A,
                    op.getIndAddrReg(),
                    Region::SRC110,
                    execSize/rgn.w, //
                    2); // :w is 16-bits a piece
            } else {
                // uniform: consumes one value in a0
                // op (..)  dst   r[a0.0]<16,8,1>:w
                setSrcRegion(
                    RegName::ARF_A,
                    op.getIndAddrReg(),
                    Region::SRC110,
                    1, // 1 element only
                    2); // :w is 16-bits a piece
            }
            // we can't do anything else for this
            break;
        }
        default:
            break;
        }
    }
}

void DepSet::setOutputsFlagDep()
{
    const FlagModifier fm = m_instruction->getFlagModifier();
    bool writesFlagRegister =
        fm != FlagModifier::NONE &&
        m_instruction->getOp() != Op::SEL; // sel uses flag modifier as input
    if (writesFlagRegister) {
        const RegRef &fr = m_instruction->getFlagReg();
        int fByteOff = fr.regNum * m_DB.getARF_F_BYTES_PER_REG() +
            fr.subRegNum * 2; // 2 bytes per subreg
        int execOff = 4 * (static_cast<int>(m_instruction->getChannelOffset()));
        fByteOff += execOff / 8; // move over by ARF offset
        int execSize = static_cast<int>(m_instruction->getExecSize()); //1 << (static_cast<int>(m_instruction->getExecSize()) - 1);
        size_t addr = (size_t)m_DB.getARF_F_START() + fByteOff;
        addFBytes(addr, execSize / 8);
        m_bucketList.push_back(addr / m_DB.getBYTES_PER_BUCKET());
    }
}

void DepSet::setOutputsDstcDep()
{
    int execOff = 4 * (static_cast<int>(m_instruction->getChannelOffset()));
    int execSize = static_cast<int>(m_instruction->getExecSize()); //1 << (static_cast<int>(m_instruction->getExecSize()) - 1);

    if (!m_instruction->getOpSpec().supportsDestination()) {
        // For the instruction having no srcs, we still need to add ARF_CR to mark the dependency
        // This is for the case that:
        //      and (1|M0)               cr0.0<1>:ud   cr0.0<0;1,0>:ud   0xFFFFFFCF:ud    {A@1}
        //      nop                                                                       {A@1}
        // It's requested that the instruction following the one having architecture register (CR/CE/SR) access,
        // It must mark to sync with all pipes, even if it's a nop.
        // nop has no src and dst so we mark it here to force setting swsb if required
        m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
        return;
    }

    const auto & op = m_instruction->getDestination();
    auto tType = op.getType();
    auto typeSizeBits = TypeSizeInBitsWithDefault(tType, 32);

    // Instructions having implicit write to acc
    if (m_instruction->hasInstOpt(InstOpt::ACCWREN) ||
        m_instruction->getOp() == Op::SUBB ||
        m_instruction->getOp() == Op::ADDC ||
        m_instruction->getOp() == Op::MACH) {
        auto elemsPerAccReg = 8 * m_DB.getARF_ACC_BYTES_PER_REG() / typeSizeBits; // e.g. 8 subreg elems for :f
        RegRef ar;
        ar.regNum = (uint16_t)(execOff / elemsPerAccReg);
        ar.subRegNum = (uint16_t)(execOff % elemsPerAccReg);
        setDstRegion(
            RegName::ARF_ACC,
            ar,
            Region::DST1,
            execSize,
            typeSizeBits);
    }

    Region rgn = op.getRegion();
    switch (op.getKind()) {
    case Operand::Kind::DIRECT:
        // send target (a GRF, not null reg)
        if (m_instruction->getOpSpec().isSendOrSendsFamily() &&
            op.getDirRegName() == RegName::GRF_R)
        {
            int nregs = m_instruction->getDstLength();
            // getDstLength return -1 when it's not able to deduce the length
            // we have to be conservative and use the max
            if (nregs < 0)
                nregs = 31;
            for (uint32_t i = 0; i < (uint32_t)nregs; i++) {
                uint32_t regNum = op.getDirRegRef().regNum;
                if ((regNum + i) >= m_DB.getGRF_REGS()) {
                    break;
                }

                addGrf((size_t)regNum + i);
                addToBucket(regNum + i);
            }
            rgn = Region::DST1;
            addToBucket(m_DB.getBucketStart(RegName::ARF_CR));

        }
        else if (m_instruction->getOpSpec().is(Op::MATH) &&
            op.getDirRegName() == RegName::GRF_R &&
            m_instruction->getMathFc() == MathFC::IDIV)
        {
            uint16_t regNum = op.getDirRegRef().regNum;
            addGrf(regNum);
            addToBucket(regNum);
            addGrf((size_t)regNum + 1);
            addToBucket((size_t)regNum + 1);
            addToBucket(m_DB.getBucketStart(RegName::ARF_CR));
        }
        else {
            // normal GRF target
            setDstRegion(
                op.getDirRegName(),
                op.getDirRegRef(),
                op.getRegion(),
                execSize,
                typeSizeBits);
        }
        break;
    case Operand::Kind::MACRO: {
        // math macro
        // GRF + ACC
        setDstRegion(
            op.getDirRegName(),
            op.getDirRegRef(),
            Region::DST1,
            execSize,
            typeSizeBits);
        auto mme = op.getMathMacroExt();
        if (mme != MathMacroExt::NOMME && mme != MathMacroExt::INVALID) {
            // and the accumultator
            int mmeNum = static_cast<int>(mme) -
                static_cast<int>(MathMacroExt::MME0);
            RegRef mmeReg{ static_cast<uint8_t>(mmeNum), 0 };
            setDstRegion(
                RegName::ARF_ACC,
                mmeReg,
                Region::DST1,
                execSize,
                typeSizeBits);
        }
        break;
    }
    case Operand::Kind::INDIRECT:
        setHasIndirect();

        // indirect destinations use a0
        //
        // writes use one a0 value
        setDstRegion(
            RegName::ARF_A,
            op.getIndAddrReg(),
            Region::DST1,
            1, // one element only
            16); // :w is 16-bits
        setDepType(DEP_TYPE::WRITE_ALWAYS_INTERFERE);

        break;
    default:
        break;
    }
}

DepSet* DepSetBuilder::createDstDepSet(
    const Instruction &i, const InstIDs& inst_id_counter,
    SWSB_ENCODE_MODE enc_mode)
{
    DepSet *oups = new DepSet(inst_id_counter, *this);
    mAllDepSet.push_back(oups);

    oups->m_instruction = &i;
    setDEPPipeClass(enc_mode, *oups, i, mPlatformModel);

    oups->setDepType(DEP_TYPE::WRITE);

    oups->setOutputsFlagDep();
    oups->setOutputsDstcDep();
    return oups;
}


DepSet* DepSetBuilder::createMathDstWADepSet(
    const Instruction &i, const InstIDs& inst_id_counter,
    SWSB_ENCODE_MODE enc_mode) {
    DepSet *oups = new DepSet(inst_id_counter, *this);
    mAllDepSet.push_back(oups);

    oups->m_instruction = &i;
    setDEPPipeClass(enc_mode, *oups, i, mPlatformModel);

    oups->setDepType(DEP_TYPE::WRITE);
    oups->setMathWAOutputsDstcDep();

    return oups;
}

void DepSet::setMathWAOutputsDstcDep()
{
    size_t execSize = static_cast<size_t>(m_instruction->getExecSize()); //1 << (static_cast<int>(m_instruction->getExecSize()) - 1);

    const auto & op = m_instruction->getDestination();
    auto tType = op.getType();
    auto typeSizeBits = TypeSizeInBitsWithDefault(tType, 32);

    Region rgn = op.getRegion();
    switch (op.getKind()) {
    case Operand::Kind::DIRECT:
    {
        RegName rn = op.getDirRegName();
        if (rn != RegName::GRF_R)
            return;
        RegRef rr = op.getDirRegRef();
        m_dType = DEP_TYPE::WRITE;

        size_t hz = static_cast<size_t>(rgn.getHz());
        // sets a region for a basic operand
        size_t grfAddr = addressOf(rn, rr, typeSizeBits);
        if (grfAddr >= m_DB.getTOTAL_BITS())
            return;

        size_t lowBound = m_DB.getTOTAL_BITS();
        size_t upperBound = 0;

        // caculate the access registers range from region
        for (size_t ch = 0; ch < execSize; ch++) {
            size_t offset = ch * hz*typeSizeBits / 8;
            size_t start = grfAddr + offset;
            //bits.set(start, typeSizeBits / 8);

            if (start < lowBound) {
                lowBound = start;
            }
            if (start + typeSizeBits / 8 > upperBound) {
                upperBound = start + typeSizeBits / 8;
            }
        }
        size_t startRegNum = lowBound / m_DB.getBYTES_PER_BUCKET();
        size_t upperRegNum = (upperBound - 1) / m_DB.getBYTES_PER_BUCKET();

        for (size_t i = startRegNum; i <= upperRegNum; i++)
        {
            // set the entire grf
            addGrf(i);
            m_bucketList.push_back(i);
        }
        break;
    }
    default:
        break;
    }
}

void DepSet::getDpasDstDependency(const Instruction &inst, RegRangeType& reg_range) {
    uint32_t execSize = static_cast<uint32_t>(inst.getExecSize());

    const auto & op = inst.getDestination();
    auto tType = op.getType();
    uint32_t typeSizeBits = TypeSizeInBitsWithDefault(tType, 32);
    IGA_ASSERT(op.getDirRegName() == RegName::GRF_R, "DPAS with non-GRF");

    // calculated used register region low and upper bound
    uint32_t lowBound = addressOf(op.getDirRegName(), op.getDirRegRef(), typeSizeBits);
    // elements_size is the size of total elements to be calculated in one operation
    uint32_t elements_size = execSize * typeSizeBits / 8;

    // For dpas, the destination region depends on Reapeat count
    uint32_t repeatCount = GetDpasRepeatCount(inst.getDpasFc());
    uint32_t upperBound = lowBound + elements_size * repeatCount;

    uint32_t startRegNum = lowBound / m_DB.getGRF_BYTES_PER_REG();
    uint32_t upperRegNum = (upperBound - 1) / m_DB.getGRF_BYTES_PER_REG();

    reg_range.first = startRegNum;
    reg_range.second = upperRegNum;
}

bool static const isSpecial(RegName rn)
{
    // The special registers (architecture registers) are CR/SR/CE
    // All the other ARF should have explicitly access so no need
    // the special handling on it
    // Others are SP, IP, IM, DBG
    if (rn == RegName::ARF_CR ||
        rn == RegName::ARF_SR ||
        rn == RegName::ARF_CE)
    {
        return true;
    }
    return false;
}

void DepSet::setSrcRegion(
    RegName rn,
    RegRef rr,
    Region rgn,
    uint32_t execSize,
    uint32_t typeSizeBits)
{
    // Previously we also skip acc register with subRegNum < 2 ==> WHY?
    // The conditio is (rn == RegName::ARF_ACC && rr.subRegNum < 2)
    if (!isRegTracked(rn)) {
        // In the case that an instruction has only non-tracked register access,
        // we still want it to mark swsb if its previous instruction having
        // architecture register access, so add the special bucket ARF_CR anyway
        // in case of swsb is required
        // e.g.
        // (W) mov(1|M0)   sr0.2<1>:ud   0xFFFFFFFF:ud   {A@1}
        // (W) mov(1|M0)   f0.0<1>:ud    0x0 : ud        {A@1}
        // the second mov required A@1 be set
        m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
        return;
    }

    uint32_t v = 1, w = 1, h = 0; // e.g. old-style default regions
    if (rgn != Region::INVALID) {
        v = static_cast<uint32_t>(rgn.getVt());
        w = static_cast<uint32_t>(rgn.getWi());
        w = w == 0 ? 1 : w;
        h = static_cast<uint32_t>(rgn.getHz());
    }

    uint32_t lowBound = m_DB.getTOTAL_BITS();
    uint32_t upperBound = 0;

    // sets a region for a basic operand
    uint32_t rowBase = addressOf(rn, rr, typeSizeBits);
    if (rowBase >= m_DB.getTOTAL_BITS()) // tm0 or something
        return;

    // the acc0/acc1 could have overlapping. (same as acc2/acc3, acc4/acc5, ...)
    // To be conservative we always mark them together when
    // one of it is used
    // This is to resolve the acc arrangement difference between int and float
    // E.g. acc3.3:f and acc2.7:w have an overlap
    if (rn == RegName::ARF_ACC) {
        RegRef tmp_rr;
        tmp_rr.regNum = (rr.regNum % 2) == 0 ? rr.regNum : rr.regNum - 1;
        tmp_rr.subRegNum = 0;
        lowBound = addressOf(rn, tmp_rr, typeSizeBits);
        // reg access cross two acc
        upperBound = lowBound + 2 * m_DB.getARF_A_BYTES_PER_REG();
        bits->set(lowBound, 2 * (size_t)m_DB.getARF_A_BYTES_PER_REG());
    } else {
        uint32_t rows = execSize / w;
        rows = (rows != 0) ? rows : 1;
        //size_t bytesTouched = 0;
        for (uint32_t y = 0; y < rows; y++) {
            uint32_t offset = rowBase;
            for (uint32_t x = 0; x < w; x++) {
                bits->set(offset, typeSizeBits / 8);
                if (offset < lowBound) {
                    lowBound = offset;
                }
                if (offset + typeSizeBits / 8 > upperBound) {
                    upperBound = offset + typeSizeBits / 8;
                }
                offset += h * typeSizeBits / 8;
                //bytesTouched += typeSize;
            }
            rowBase += v * typeSizeBits / 8;
        }
    }

    if (isRegTracked(rn))
    {
        //size_t extentTouched = upperBound - lowBound;
        IGA_ASSERT(upperBound >= lowBound,
            "source region footprint computation got it wrong: upperBound is less than lowBound");
        uint32_t startRegNum = lowBound / m_DB.getBYTES_PER_BUCKET();
        uint32_t upperRegNum = (upperBound - 1) / m_DB.getBYTES_PER_BUCKET();

        for (uint32_t i = startRegNum; i <= upperRegNum; i++)
        {
            //note: bucket start is already included in 'i' calculation
            //used to be: m_bucketList.push_back(i + DepSet::getBucketStart(rn));
            m_bucketList.push_back(i);
        }
    }

    // Special registers has side effects so even if there is no direct interference
    // subsequent instrucion might depend on it. Also the prior instrucions may depends on it
    // so force to sync all pipes to this instruction and the following instruction
    if (isSpecial(rn))
    {
        setHasSR();
        m_dType = DEP_TYPE::READ_ALWAYS_INTERFERE;
    }
    else
    {
        //Using one of the special registers to add write dependency in to special bucket
        //This way it will always check that implicit dependency
        m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
    }
}


void DepSet::setDstRegion(
    RegName rn,
    RegRef rr,
    Region rgn,
    uint32_t execSize,
    uint32_t typeSizeBits)
{
    // Previously we also skip acc register with subRegNum < 2 ==> WHY?
    // The conditio is (rn == RegName::ARF_ACC && rr.subRegNum < 2)
    if (!isRegTracked(rn)) {
        // In the case that an instruction has only non-tracked register access,
        // we still want it to mark swsb if its previous instruction having
        // architecture register access, so add the special bucket ARF_CR anyway
        // in case of swsb is required
        // e.g.
        // (W) mov(1|M0)   sr0.2<1>:ud   0xFFFFFFFF:ud   {A@1}
        // (W) mov(1|M0)   f0.0<1>:ud    0x0 : ud        {A@1}
        // the second mov required A@1 be set
        m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
        return;
    }

    m_dType = DEP_TYPE::WRITE;

    uint32_t hz = static_cast<uint32_t>(rgn.getHz());
    // sets a region for a basic operand
    uint32_t grfAddr = addressOf(rn, rr, typeSizeBits);
    if (grfAddr >= m_DB.getTOTAL_BITS())
        return;

    uint32_t lowBound = m_DB.getTOTAL_BITS();
    uint32_t upperBound = 0;

    // the acc0/acc1 could have overlapping. (same as acc2/acc3, acc4/acc5, ...)
    // To be conservative we always mark them together when
    // one of it is used
    // This is to resolve the acc arrangement difference between int and float
    // E.g. acc3.3:f and acc2.7:w have an overlap
    if (rn == RegName::ARF_ACC) {
        RegRef tmp_rr;
        tmp_rr.regNum = (rr.regNum % 2) == 0 ? rr.regNum : rr.regNum - 1;
        tmp_rr.subRegNum = 0;
        lowBound = addressOf(rn, tmp_rr, typeSizeBits);

        // reg access cross two acc
        upperBound = lowBound + 2 * m_DB.getARF_A_BYTES_PER_REG();
        bits->set(lowBound, 2 * (size_t)m_DB.getARF_A_BYTES_PER_REG());
    } else {
        // otherwise caculate the access registers range from region
        for (uint32_t ch = 0; ch < execSize; ch++) {
            uint32_t offset = ch * hz*typeSizeBits / 8;
            uint32_t start = grfAddr + offset;
            bits->set(start, typeSizeBits / 8);

            if (start < lowBound) {
                lowBound = start;
            }
            if (start + typeSizeBits / 8 > upperBound) {
                upperBound = start + typeSizeBits / 8;
            }
        }
    }

    if (isRegTracked(rn))
    {
        uint32_t startRegNum = lowBound / m_DB.getBYTES_PER_BUCKET();
        uint32_t upperRegNum = (upperBound - 1) / m_DB.getBYTES_PER_BUCKET();

        for (uint32_t i = startRegNum; i <= upperRegNum; i++)
        {
            //note: bucket start is already included in 'i' calculation
            //used to be: m_bucketList.push_back(i + DepSet::getBucketStart(rn));
            m_bucketList.push_back(i);
        }
    }

    // Special registers has side effects so even if there is no direct interference
    // subsequent instrucion might depend on it. Also the prior instrucions may depends on it
    // so force to sync all pipes to this instruction and the following instruction
    if (isSpecial(rn))
    {
        setHasSR();
        m_dType = DEP_TYPE::WRITE_ALWAYS_INTERFERE;
    }
    else
    {
        //Using one of the special registers to add write dependency in to special bucket
        //This way it will always check that implicit dependency
        m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
    }
}

uint32_t DepSet::addressOf(
    RegName rnm, const RegRef &rr, uint32_t typeSizeBits) const
{
    switch (rnm) {
    case RegName::GRF_R:
        return m_DB.getGRF_START() + rr.regNum*m_DB.getGRF_BYTES_PER_REG() + rr.subRegNum*typeSizeBits/8;
    case RegName::ARF_A:
        return m_DB.getARF_A_START() + rr.regNum*m_DB.getARF_A_BYTES_PER_REG() + rr.subRegNum*typeSizeBits/8;
    case RegName::ARF_ACC:
        return m_DB.getARF_ACC_START() +  rr.regNum*m_DB.getARF_ACC_BYTES_PER_REG() + rr.subRegNum*typeSizeBits/8;
    case RegName::ARF_F:
        return m_DB.getARF_F_START() + rr.regNum*m_DB.getARF_F_BYTES_PER_REG() + rr.subRegNum*typeSizeBits/8;
    case RegName::ARF_CR:
    case RegName::ARF_SR:
    case RegName::ARF_CE:
        return m_DB.getARF_SPECIAL_START() + rr.regNum*m_DB.getARF_SPECIAL_BYTES_PER_REG() + rr.subRegNum*typeSizeBits/8;
    default:
        return m_DB.getTOTAL_BITS();
    }
}

bool DepSet::isRegTracked(RegName rnm) const
{
    switch (rnm) {
    case RegName::GRF_R:
    case RegName::ARF_A:
    case RegName::ARF_ACC:
    case RegName::ARF_F:
        return true;
    default:
        return false || isSpecial(rnm);
    }
}

void DepSet::addGrfBytes(size_t reg, size_t subRegBytes, size_t num_bytes)
{
    size_t grf_addr = m_DB.getGRF_START() + reg* m_DB.getGRF_BYTES_PER_REG() + subRegBytes;
    if (grf_addr < m_DB.getGRF_START() ||
        (grf_addr + num_bytes > (size_t)m_DB.getGRF_START() + m_DB.getGRF_LEN())) {
        IGA_FATAL("RegDeps: GRF index is out of bounds");
    }
    bits->set(grf_addr, num_bytes);
}

void DepSet::addABytes(size_t reg, size_t subregBytes, size_t num_bytes)
{
    IGA_ASSERT(reg >= 0 && reg < m_DB.getARF_A_REGS(),
        "a# register out of bounds");
    IGA_ASSERT(subregBytes >= 0 && subregBytes < m_DB.getARF_A_BYTES_PER_REG(),
        "a# sub register byte out of bounds");
    IGA_ASSERT(subregBytes >= 0 && subregBytes < m_DB.getARF_A_BYTES_PER_REG(),
        "a# sub register byte out of bounds");
    size_t addr = m_DB.getARF_A_START() + m_DB.getARF_A_BYTES_PER_REG()*reg + subregBytes;
    IGA_ASSERT(addr < (size_t)m_DB.getARF_A_START() + m_DB.getARF_A_LEN(),
        "a# byte address out of bounds");
    bits->set(addr, num_bytes);
}
void DepSet::addFBytes(size_t fByteOff, size_t num_bytes)
{
    bits->set(fByteOff, num_bytes);
}

bool DepSet::destructiveSubtract(const DepSet &rhs)
{
    return bits->andNot(*rhs.bits);
}


static void emitComma(std::ostream &os, bool &first)
{
    if (first) {
        first = false;
    } else {
        os << ", ";
    }
}
void DepSet::formatShortReg(
    std::ostream &os,
    bool &first,
    const char *reg_name,
    size_t reg_num, // ((size_t)-1) means we ignore
    size_t reg_start,
    size_t reg_len) const
{
    if (bits->testAll(reg_start, reg_len)) {
        // full register is set
        emitComma(os,first);
        os << reg_name;
        if (reg_num != (size_t)-1)
            os << reg_num;
    } else if (bits->testAny(reg_start, reg_len)) {
        // partial register
        for (size_t ri = 0; ri < reg_len; ri++) {
            // find starting bit
            if (!bits->test(reg_start + ri)) {
                continue;
            }
            // ri is first set
            emitComma(os,first);
            os << reg_name;
            if (reg_num != (size_t)-1)
                os << reg_num;
            os << "." << ri;

            // find the ending position
            size_t end_ri = ri;
            while(end_ri < reg_start + reg_len && bits->test(reg_start + end_ri))
                end_ri++;
            // e.g. r23.0-3 (a full DWORD)
            if (end_ri > ri + 1) // case of 1 byte omits range r0.0 (for :b)
                os << '-' << (end_ri - 1);
            // else: e.g. r23.0 (just one byte)
            ri = end_ri; // will move to the next element
        } // for
    } // else: nothing set in this register
}
// emits a shorter description... something like
// {r0,r1.0-3,r7}
void DepSet::str(std::ostream &os) const
{
    bool first = true;
    os << "{";

    for (size_t ri = 0;
        ri < m_DB.getGRF_REGS();
        ri++)
    {
        formatShortReg(
            os, first,
            "r", ri,
            m_DB.getGRF_START() + ri* m_DB.getGRF_BYTES_PER_REG(),
            m_DB.getGRF_BYTES_PER_REG());
    }

    for (uint32_t ai = 0; ai < m_DB.getARF_A_REGS(); ai++) {
        formatShortReg(
            os, first,
            "a", ai,
            m_DB.getARF_A_START() + (size_t)ai* m_DB.getARF_A_BYTES_PER_REG(),
            m_DB.getARF_A_BYTES_PER_REG());
    }

    for (uint32_t acci = 0; acci < m_DB.getARF_ACC_REGS(); acci++) {
        formatShortReg(
            os, first,
            "acc", acci,
            m_DB.getARF_ACC_START() + (size_t)acci * m_DB.getARF_ACC_BYTES_PER_REG(),
            m_DB.getARF_ACC_BYTES_PER_REG());
    }

    for (uint32_t fi = 0; fi < m_DB.getARF_F_REGS(); fi++) {
        formatShortReg(os, first,
            "f", fi,
            m_DB.getARF_F_START() + (size_t)fi * m_DB.getARF_F_BYTES_PER_REG(),
            m_DB.getARF_F_BYTES_PER_REG());
    }

    os << "}";
}
std::string DepSet::str() const {
    std::stringstream ss;
    str(ss);
    return ss.str();
}
