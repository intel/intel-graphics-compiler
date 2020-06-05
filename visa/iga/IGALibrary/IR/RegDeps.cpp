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

#include "RegDeps.hpp"
#include "../asserts.hpp"
#include "../bits.hpp"

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
            auto src = inst.getSource(i);
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
            auto dst = inst.getDestination();
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


static void setDEPPipeClass(
    SWSB_ENCODE_MODE enc_mode, DepSet &dep, const Instruction &inst, const Model& model)
{
    if (enc_mode == SWSB_ENCODE_MODE::SingleDistPipe)
        setDEPPipeClass_SingleDistPipe(dep, inst);
}

DepSet::DepSet(const InstIDs& inst_id_counter, const DepSetBuilder& dsb)
    : m_instruction(nullptr),
      m_dType(DEP_TYPE::NONE),
      m_hasIndirect(false),
      m_hasSR(false),
      m_dPipe(DEP_PIPE::NONE),
      m_dClass(DEP_CLASS::NONE),
      m_InstIDs(inst_id_counter.global,
                inst_id_counter.inOrder
      ),
      m_DB(dsb)
{
    m_bucketList.reserve(4);
    bits = new BitSet<>(dsb.getTOTAL_BITS());
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

    auto op = m_instruction->getDestination();
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

    auto op = m_instruction->getDestination();
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
    RegName rnm, const RegRef &rr, uint32_t typeSizeBits)
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

bool DepSet::isRegTracked(RegName rnm)
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