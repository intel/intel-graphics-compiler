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

#include "RegSet.hpp"
#include "../asserts.hpp"

#include <sstream>

using namespace iga;

// This module implements what awas the original intent for RegDeps.
// Unfortunately, that module coupled certain machine state with the
// dependencies that we don't want for things like DU analysis.
//
// FIXME: would be to
//    (1) rename RegDeps in SWSBSetter to something like DepState
//    (2) compose a RegSet within the DepState
//

const RegSetInfo *RegSetInfo::ALL[] = {
    &RS_GRF_R,
    &RS_ARF_A,
    &RS_ARF_ACC,
    &RS_ARF_F,
    &RS_ARF_CR,
    &RS_ARF_SR,
    &RS_ARF_IP,
    &RS_ARF_SP
};


bool RegSet::add(const RegSetInfo &rs, size_t off, size_t len)
{
    IGA_ASSERT(off + len <= (static_cast<size_t>(rs.numRegisters) * rs.bytesPerRegister),
        "register out of bounds");
    return bits.set(rs.startOffset + off, len);
}


bool RegSet::setDstRegion(
    RegName rn,
    RegRef rr,
    Region rgn,
    size_t execSize,
    size_t typeSizeBits)
{
    const RegSetInfo *rsi = RegSetInfo::lookup(rn);
    if (!rsi) {
        return false; // not a tracked register
    }
    size_t hz = static_cast<size_t>(rgn.getHz());
    // sets a region for a basic operand
    if (rgn.getHz() == Region::Horz::HZ_INVALID) {
        hz = 1;
    }

    size_t baseAddr = relativeAddressOf(*rsi, rr, typeSizeBits/8);
    bool added = false;
    for (size_t ch = 0; ch < execSize; ch++) {
        size_t offset = ch*hz*typeSizeBits/8;
        added |= add(*rsi, baseAddr + offset, typeSizeBits/8);
    }
    return added;
}

// sets a region for a basic operand
bool RegSet::setSrcRegion(
    RegName rn,
    RegRef rr,
    Region rgn,
    size_t execSize,
    size_t typeSizeBits)
{
    const RegSetInfo *rsi = RegSetInfo::lookup(rn);
    if (!rsi) {
        return false; // not a tracked register
    }

    bool changed = false;
    size_t v = 1, w = 1, h = 0; // e.g. old-style default regions
    if (rgn != Region::INVALID) {
        v = static_cast<size_t>(rgn.getVt());
        w = static_cast<size_t>(rgn.getWi());
        w = w == 0 ? 1 : w;
        h = static_cast<size_t>(rgn.getHz());
    }

    size_t rowBase = relativeAddressOf(*rsi, rr, typeSizeBits);
    size_t rows = execSize / w;
    rows = (rows != 0) ? rows : 1;
    for (size_t y = 0; y < rows; y++) {
        size_t offset = rowBase;
        for (size_t x = 0; x < w; x++) {
            changed |= add(*rsi, offset, typeSizeBits/8);
            offset += h * typeSizeBits/8;
        }
        rowBase += v * typeSizeBits/8;
    }
    return changed;
}

bool RegSet::addPredicationInputs(const Instruction &i, RegSet &rs)
{
    size_t execSize = static_cast<size_t>(i.getExecSize());
    size_t execOff = 4 * static_cast<size_t>(i.getChannelOffset());

    // does it read the flag register
    //  predication does this
    //  conditional modifier on 'sel' does this
    const Predication &pred = i.getPredication();
    const FlagModifier fm = i.getFlagModifier();
    bool readsFlagRegister =
        pred.function != PredCtrl::NONE ||
        i.getOp() == Op::SEL && fm != FlagModifier::NONE;
    if (readsFlagRegister) {
        // add the ARF offset from ExecMaskOffset
        // E.g.
        // (f1.0) op (16|M16) ...
        // is touching f1.1
        const RegRef &fr = i.getFlagReg();
        size_t fByteOff = (size_t)fr.regNum*RS_ARF_F.bytesPerRegister +
            (size_t)fr.subRegNum*2; // FIXME: magic number (needs some thought should be bytes per subreg)
        fByteOff += execOff/8; // move over by ARF offset
        return rs.bits.set(fByteOff + RS_ARF_F.startOffset, execSize/8);
    }
    return false;
}


bool RegSet::addSourceInputs(const Instruction &i, RegSet &rs)
{
    size_t execSize = static_cast<size_t>(i.getExecSize());
    bool added = false;

    if (i.getOpSpec().isSendOrSendsFamily()) {
        // send register descriptors touch a0.#
        auto desc = i.getMsgDescriptor();
        if (desc.isReg()) {
            added |= rs.add(RS_ARF_A, desc.reg.subRegNum, 4);
        }
        auto exDesc = i.getExtMsgDescriptor();
        if (exDesc.isReg()) {
            added |= rs.add(RS_ARF_A, desc.reg.subRegNum, 4);
        }
    }

    // check all the source operands
    for (unsigned srcIx = 0, numSrcs = i.getSourceCount();
        srcIx < numSrcs;
        srcIx++)
    {
        const Operand &op = i.getSource(srcIx);
        auto tType = op.getType();
        auto typeSizeBits = TypeSizeInBitsWithDefault(tType,32);

        // possibly need to fixup the region
        Region rgn = op.getRegion();
        if (numSrcs == 3) {
            if (i.isMacro()) {
                // macros and sends are packed access only
                // madm (4) r3.acc1 r7.acc2 r9.acc4 r6.noacc
                rgn = Region::SRC110;
            } else {
                // ternary align 1 has some implicit regions that need filling in
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
        } else if (numSrcs == 2 && i.isMacro()) {
            // math macro
            rgn = Region::SRC110;
        } // not ternary, not macro

        switch (op.getKind()) {
        case Operand::Kind::DIRECT:
            added = true;
            if (i.getOpSpec().isSendOrSendsFamily()) {
                if (op.getDirRegName() == RegName::GRF_R) {
                    // send source GRF (not null reg)
                    int nregs = 0;
                    if (srcIx == 0) {
                        // mlen
                        nregs = i.getSrc0Length();
                        if (nregs < 0)
                            nregs = 31;
                    } else {
                        nregs = i.getSrc1Length();
                        if (nregs < 0)
                            nregs = 8;
                    }
                    uint16_t regNum = op.getDirRegRef().regNum;
                    for (int i = 0; i < nregs; i++) {
                        if ((regNum + i) >= RS_GRF_R.numRegisters) {
                            break;
                        }
                        rs.addFullReg(RS_GRF_R, regNum + i);
                    }
                }
            } else {
                rs.setSrcRegion(
                    op.getDirRegName(),
                    op.getDirRegRef(),
                    rgn,
                    execSize,
                    typeSizeBits);
            }
            break;
        case Operand::Kind::MACRO: {
            added = true;
            rs.setSrcRegion(
                op.getDirRegName(),
                op.getDirRegRef(),
                rgn,
                execSize,
                typeSizeBits);
            auto mme = op.getMathMacroExt();
            if (mme != MathMacroExt::NOMME && mme != MathMacroExt::INVALID) {
                int mmeRegNum = static_cast<int>(op.getMathMacroExt()) -
                    static_cast<int>(MathMacroExt::MME0);
                RegRef rr{static_cast<uint8_t>(mmeRegNum),0};
                rs.setSrcRegion(
                    RegName::ARF_ACC,
                    rr,
                    Region::SRC110,
                    execSize,
                    typeSizeBits);
            }
            break;
        }
        case Operand::Kind::INDIRECT: {
            added = true;
            auto rgn = op.getRegion();
            if (rgn.getVt() == Region::Vert::VT_VxH) {
                // VxH or Vx1 mode
                //   op (K)  dst   r[a0.0]<W,1>:w  (reads K/W elements)
                //   op (K)  dst   r[a0.0]<1,0>:w  (reads K/W elements)
                rs.setSrcRegion(
                    RegName::ARF_A,
                    op.getIndAddrReg(),
                    Region::SRC110,
                    execSize/rgn.w, //
                    16); // :w is 16-bits a piece
            } else {
                // uniform: consumes one value in a0
                // op (..)  dst   r[a0.0]<16,8,1>:w
                rs.setSrcRegion(
                    RegName::ARF_A,
                    op.getIndAddrReg(),
                    Region::SRC110,
                    1, // 1 element only
                    16); // :w is 16-bits a piece
            }
            // we can't do anything else for this
            break;
        }
        default:
            break;
        }
    }
    return added;
}

bool RegSet::addFlagModifierOutputs(const Instruction &i, RegSet &rs)
{
    int execOff = 4 * (static_cast<int>(i.getChannelOffset()));
    int execSize = static_cast<int>(i.getExecSize()); // 1 << (static_cast<int>(i.getExecSize()) - 1);
    const FlagModifier fm = i.getFlagModifier();
    bool writesFlagRegister =
        fm != FlagModifier::NONE &&
        i.getOp() != Op::SEL; // sel uses flag modifier as input
    if (writesFlagRegister) {
        const RegRef &fr = i.getFlagReg();
        int fByteOff = fr.regNum*RS_ARF_F.bytesPerRegister + fr.subRegNum*2; // 2 bytes per subreg
        fByteOff += execOff/8; // move over by ARF offset
        return rs.bits.set(static_cast<size_t>(RS_ARF_F.startOffset) + fByteOff, execSize/8);
    }
    return false;
}

bool RegSet::addDestinationOutputs(const Instruction &i, RegSet &rs)
{
    if (!i.getOpSpec().supportsDestination()) {
        return false;
    }
    bool added = false;

    unsigned execOff = 4 * (static_cast<int>(i.getChannelOffset()));
    int execSize = static_cast<int>(i.getExecSize());
    auto op = i.getDestination();
    auto tType = op.getType();
    auto typeSizeBits = TypeSizeInBitsWithDefault(tType, 32);

    if (i.hasInstOpt(InstOpt::ACCWREN) /* || i.getDestination().getDirRegName() == RegName::ARF_ACC*/) { // AccWrEn
        auto elemsPerAccReg = 8*RS_ARF_ACC.bytesPerRegister / typeSizeBits; // e.g. 8 subreg elems for :f
        RegRef ar(
            execOff / elemsPerAccReg,
            execOff % elemsPerAccReg);
        added |= rs.setDstRegion(
            RegName::ARF_ACC,
            ar,
            Region::DST1,
            execSize,
            typeSizeBits/8);
    }
    Region rgn = op.getRegion();
    switch (op.getKind()) {
    case Operand::Kind::DIRECT:
        // send target (a GRF, not null reg)
        if (i.getOpSpec().isSendOrSendsFamily() &&
            op.getDirRegName() == RegName::GRF_R)
        {
            int nregs = i.getDstLength();
            if (nregs < 0)
                nregs = 31;
            for (int ri = 0; ri < nregs; ri++) {
                uint16_t regNum = op.getDirRegRef().regNum;
                if ((regNum + ri) >= RS_GRF_R.numRegisters) {
                    break;
                }
                added |= rs.addFullReg(RS_GRF_R, regNum + ri);
            }
            rgn = Region::DST1;
        } else {
            // normal GRF target
            added |= rs.setDstRegion(
                op.getDirRegName(),
                op.getDirRegRef(),
                op.getRegion(),
                execSize,
                typeSizeBits/8);
        }
        break;
    case Operand::Kind::MACRO: {
        // math macro
        // GRF + ACC
        added |= rs.setDstRegion(
            op.getDirRegName(),
            op.getDirRegRef(),
            Region::DST1,
            execSize,
            typeSizeBits/8);
        auto MathMacroReg = op.getMathMacroExt();
        if (MathMacroReg != MathMacroExt::NOMME && MathMacroReg != MathMacroExt::INVALID) {
            // and the math macro register
            int mmeRegNum = static_cast<int>(MathMacroReg) - static_cast<int>(MathMacroExt::MME0);
            RegRef mmeRegRef = {static_cast<uint8_t>(mmeRegNum), 0};
            added |= rs.setDstRegion(
                RegName::ARF_MME,
                mmeRegRef,
                Region::DST1,
                execSize,
                typeSizeBits/8);
        }
        break;
    }
    case Operand::Kind::INDIRECT:
        // indirect destinations use a0
        //
        // writes use one a0 value
        added |= rs.setDstRegion(
            RegName::ARF_A,
            op.getIndAddrReg(),
            Region::DST1,
            1, // one element only
            2); // :w is 16-bits
        break;
    default:
        break;
    }
    return added;
}

void RegSet::formatShortReg(
    std::ostream &os,
    bool &first,
    const char *reg_name,
    size_t reg_num, // ((size_t)-1) means we ignore
    size_t reg_start,
    size_t reg_len) const
{
    auto emitComma =
        [&] (std::ostream &os) {
            if (first) {
                first = false;
            } else {
                os << ", ";
            }
        };
    if (bits.testAll(reg_start, reg_len)) {
        // full register is set
        emitComma(os);
        os << reg_name;
        if (reg_num != (size_t)-1)
            os << reg_num;
    } else if (bits.testAny(reg_start, reg_len)) {
        // partial register
        for (size_t ri = 0; ri < reg_len; ri++) {
            // find starting bit
            if (!bits.test(reg_start + ri)) {
                continue;
            }
            // ri is first set
            emitComma(os);
            os << reg_name;
            if (reg_num != (size_t)-1)
                os << reg_num;
            os << "." << ri;

            // find the ending position
            size_t end_ri = ri;
            while(end_ri < reg_start + reg_len && bits.test(reg_start + end_ri))
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
void RegSet::str(std::ostream &os) const
{
    bool first = true;
    os << "{";
    for (const RegSetInfo *rs : RegSetInfo::ALL) {
        if (rs->numRegisters == 1) {
            // e.g. ip, a0, sp
            formatShortReg(
                os,
                first,
                rs->syntax,
                (size_t)-1,
                rs->startOffset,
                rs->length);
        } else {
            // e.g. r# or acc#
            for (size_t i = 0; i < (size_t)rs->numRegisters; i++) {
                formatShortReg(
                    os,
                    first,
                    rs->syntax,
                    i,
                    rs->startOffset + rs->bytesPerRegister * i,
                    rs->bytesPerRegister);
            }
        }
    }
    os << "}";
}
std::string RegSet::str() const {
    std::stringstream ss;
    str(ss);
    return ss.str();
}

