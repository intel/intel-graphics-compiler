/*========================== begin_copyright_notice ============================

Copyright (c) 2017-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "RegSet.hpp"
#include "../asserts.hpp"

#include <sstream>

using namespace iga;

// This module implements what was the original intent for RegDeps.
// Unfortunately, that module was coupled certain machine state and other
// dependencies that we don't want for simpler things like DU analysis.
//
// FIXME: would be to
//    (1) rename RegDeps in SWSBSetter to something like DepState
//    (2) compose a RegSet within the DepState
//

static size_t BytesForRegSet(RegName rn, const Model &m) {
    return (size_t)m.getRegCount(rn) * m.getBytesPerReg(rn);
}

static const RegName TRACKED[] {
    RegName::GRF_R,
    RegName::ARF_A,
    RegName::ARF_ACC,
    RegName::ARF_F,
};

BitSet<> *RegSet::bitSetForPtr(RegName rn)
{
    switch (rn) {
    case RegName::GRF_R:    return &bitsR;
    case RegName::ARF_A:    return &bitsA;
    case RegName::ARF_ACC:  return &bitsAcc;
    case RegName::ARF_F:    return &bitsF;
    default: break;
    }
    return nullptr;
}

BitSet<> &RegSet::bitSetFor(RegName rn) {
    auto bs = bitSetForPtr(rn);
    IGA_ASSERT(bs, "not a tracked bitset");
    return *bs;
}
const BitSet<> &RegSet::bitSetFor(RegName rn) const {
    return const_cast<RegSet *>(this)->bitSetFor(rn);
}

bool RegSet::isTrackedReg(RegName rn) const {
    return const_cast<RegSet *>(this)->bitSetForPtr(rn) != nullptr;
}

size_t RegSet::offsetOf(RegName rn, int reg)  const {
    return (size_t)reg * model->getBytesPerReg(rn) * 8;
}
size_t RegSet::offsetOf(RegName rn, RegRef rr, size_t typeSizeBits) const {
    return offsetOf(rn, rr.regNum) + rr.subRegNum * typeSizeBits;
}
size_t RegSet::offsetOf(RegName rn, RegRef rr, Type t)  const {
    size_t typeSizeBits = TypeSizeInBitsWithDefault(t, 8);
    return offsetOf(rn, rr, typeSizeBits);
}


RegSet::RegSet(const Model &m)
    : model(&m)
    , bitsR(BytesForRegSet(RegName::GRF_R, m))
    , bitsA(BytesForRegSet(RegName::ARF_A, m))
    , bitsAcc(BytesForRegSet(RegName::ARF_ACC, m))
    , bitsF(BytesForRegSet(RegName::ARF_F, m))
{ }

bool RegSet::operator==(const RegSet &rs) const {
    for (RegName rn : TRACKED) {
        if (bitSetFor(rn) != rs.bitSetFor(rn)) {
            return false;
        }
    }
    return true;
}
bool RegSet::empty() const {
    for (RegName rn : TRACKED)
        if (!bitSetFor(rn).empty())
            return false;
    return true;
}
void RegSet::reset() {
    for (RegName rn : TRACKED)
        bitSetFor(rn).reset();
}

bool RegSet::intersects(const RegSet &rhs) const {
    for (RegName rn : TRACKED)
        if (bitSetFor(rn).intersects(rhs.bitSetFor(rn)))
            return true;
    return false;
}

bool RegSet::destructiveUnion(const RegSet &rhs) {
    bool changed = false;
    for (RegName rn : TRACKED)
        changed |= bitSetFor(rn).add(rhs.bitSetFor(rn));
    return changed;
}

bool RegSet::destructiveSubtract(const RegSet &rhs) {
    bool changed = false;
    for (RegName rn : TRACKED)
        changed |= bitSetFor(rn).andNot(rhs.bitSetFor(rn));
    return changed;
}

bool RegSet::intersectInto(const RegSet &rhs, RegSet &into) const {
    bool notEmpty = false;
    for (RegName rn : TRACKED) {
        notEmpty |= bitSetFor(rn).intersectInto(
            rhs.bitSetFor(rn), into.bitSetFor(rn));
    }
    return notEmpty;
}

bool RegSet::addReg(RegName rn, int reg) {
    return addRegs(rn, reg, 1);
}

bool RegSet::addRegs(RegName rn, int reg, int n) {
    size_t bitsPerReg = 8 * (size_t)model->getBytesPerReg(rn);
    return add(rn, bitsPerReg * reg, bitsPerReg * n);
}

bool RegSet::add(RegName rn, size_t regFileOffBits, size_t numBits) {
    if (!isTrackedReg(rn)) {
        return false;
    }
    // coarsen analysis to byte
    // need std::max since numBits might be <8 (e.g. flag reg SIMD1)
    size_t offBytes = regFileOffBits / 8;
    size_t lenBytes = std::max<size_t>(numBits / 8, 1);
    BitSet<> &bs = bitSetFor(rn);
    if (offBytes + lenBytes <= BytesForRegSet(rn, *model)) {
        return bs.set(offBytes, lenBytes);
    } else {
        IGA_ASSERT_FALSE("access is out of bounds for register");
        return false;
    }
}

bool RegSet::add(RegName rn, RegRef rr, Type t)
{
    size_t typeSizeBits = TypeSizeInBitsWithDefault(t, 8);
    return add(rn, offsetOf(rn, rr, t), typeSizeBits);
}


bool RegSet::addFlagRegArf(RegRef fr, ExecSize es, ChannelOffset co) {
    size_t execSize = size_t(es);
    size_t execOff = 4 * size_t(co);

    // can't use subregisters here because subregisters are only a
    // word boundary.  E.g. (f1.1) op (M8|8) touches upper byte of f1.1
    // which doesn't have a syntax representation
    size_t off = offsetOf(RegName::ARF_F, fr, 16);

    RegRef frWithOff = fr;
    frWithOff.subRegNum += (uint16_t)execOff / 16; // 16b per subreg
    return add(RegName::ARF_F, off + execOff, execSize);
}


bool RegSet::addPredicationInputs(const Instruction &i)
{

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
        return addFlagRegArf(fr, i.getExecSize(), i.getChannelOffset());
    }
    return false;
}


bool RegSet::addSourceInputs(const Instruction &i)
{
    bool added = false;

    if (i.getOpSpec().isSendOrSendsFamily()) {
        // send register descriptors may touch a0.#
        auto desc = i.getMsgDescriptor();
        if (desc.isReg()) {
            // send is strange: a0.# is in word offsets but reads 32b
            // fake it by converting a0.4:uw to a0.2:ud and writing the
            // full 32b
            desc.reg.subRegNum /= 2;
            added |= add(RegName::ARF_A, desc.reg, Type::UD);
        }
        auto exDesc = i.getExtMsgDescriptor();
        if (exDesc.isReg()) {
            // see above
            exDesc.reg.subRegNum /= 2;
            added |= add(RegName::ARF_A, exDesc.reg, Type::UD);
        }
    }

    added |= addSourceImplicitAccumulator(i);

    added |= addDestinationInputs(i); // e.g. indirect access uses a0

    // check all the source operands
    for (unsigned srcIx = 0; srcIx < i.getSourceCount(); srcIx++) {
        added |= addSourceOperandInput(i, srcIx);
    }
    return added;
}


bool RegSet::addSourceOperandInput(const Instruction &i, int srcIx)
{
    size_t execSize = size_t(i.getExecSize());
    bool added = false;
    const Operand &op = i.getSource(srcIx);
    auto typeSizeBits = TypeSizeInBitsWithDefault(op.getType(), 32);

    // possibly need to fixup the region
    Region rgn = op.getRegion();
    if (i.getSourceCount() == 3) {
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
                    Region::Vert(size_t(rgn.h)),
                    Region::Width::WI_1,
                    Region::Horz::HZ_0);
            }
        }
    } else if (i.getSourceCount() == 2 && i.isMacro()) {
        // math macro
        rgn = Region::SRC110;
    } // not ternary, not macro ==> regions are good

    switch (op.getKind()) {
    case Operand::Kind::DIRECT:
        added = true;
        if (i.getOpSpec().isSendOrSendsFamily()) {
            if (op.getDirRegName() == RegName::GRF_R) {
                // send source GRF (not null reg)
                int nregs = 0;
                if (srcIx == 0) { // mlen
                    nregs = i.getSrc0Length();
                    if (nregs < 0)
                        nregs = 4; // assume the worst
                } else { // xlen
                    nregs = i.getSrc1Length();
                    if (nregs < 0)
                        nregs = 8; // assume the worst
                }
                int regNum = (int)op.getDirRegRef().regNum;
                addRegs(RegName::GRF_R, regNum, nregs);
            }
        } else {
            setSrcRegion(
                op.getDirRegName(),
                op.getDirRegRef(),
                rgn,
                execSize,
                typeSizeBits);
        }
        break;
    case Operand::Kind::MACRO: {
        added = true;
        setSrcRegion(
            op.getDirRegName(),
            op.getDirRegRef(),
            rgn,
            execSize,
            typeSizeBits);
        // Don't bother tracking MME registers, technically they are 2 bits
        // of exponent each, but they are mixed in with other ACC creating a
        // royal mess.
        //
        // auto mme = op.getMathMacroExt();
        // if (mme != MathMacroExt::NOMME && mme != MathMacroExt::INVALID) {
        //     int mmeRegNum = int(op.getMathMacroExt()) -
        //         int(MathMacroExt::MME0);
        //     rs.setSrcRegion(
        //         RegName::ARF_ACC,
        //         RegRef(mmeRegNum, 0),
        //         Region::SRC110,
        //         execSize,
        //         typeSizeBits);
        // }
        break;
    }
    case Operand::Kind::INDIRECT: {
        added = true;
        auto rgn = op.getRegion();
        if (rgn.getVt() == Region::Vert::VT_VxH) {
            // VxH or Vx1 mode
            //   op (K)  dst   r[a0.0]<W,1>:w  (reads K/W elements)
            //   op (K)  dst   r[a0.0]<1,0>:w  (reads K elements)
            setSrcRegion(
                RegName::ARF_A,
                op.getIndAddrReg(),
                Region::SRC110,
                execSize / rgn.w, //
                16); // :w is 16-bits a piece
        } else {
            // uniform: consumes one value in a0
            // op (..)  dst   r[a0.0]<16,8,1>:w
            setSrcRegion(
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
    return added;
}


bool RegSet::addSourceImplicitAccumulator(const Instruction &i)
{
    Type type = i.getDestination().getType();
    bool added = false;
    bool readsAcc = i.is(Op::MACH) ||  i.is(Op::MAC);
    bool readsAcc64 = i.is(Op::MACH);
    if (readsAcc) {
        if (readsAcc64) {
            type = Type::Q;
        }
        auto typeSizeBits = TypeSizeInBitsWithDefault(type, 32);
        auto execOff = 4 * int(i.getChannelOffset());
        auto elemsPerAccReg =
            8 * model->getBytesPerReg(RegName::ARF_ACC) / typeSizeBits;
        RegRef ar( // e.g. on SKL (8|M8) ... acc0 ... really means acc1
            execOff / elemsPerAccReg,
            execOff % elemsPerAccReg);
        added |= setSrcRegion(
            RegName::ARF_ACC,
            ar,
            Region::SRC110,
            size_t(i.getExecSize()),
            typeSizeBits);
    }
    return added;
}

bool RegSet::addDestinationInputs(const Instruction &i)
{
    if (!i.getOpSpec().supportsDestination()) {
        return false;
    }
    bool added = false;
    const auto &dst = i.getDestination();
    if (dst.getKind() == Operand::Kind::INDIRECT) {
        added |= setDstRegion(
            RegName::ARF_A,
            dst.getIndAddrReg(),
            Region::DST1,
            1, // one element only
            16); // :w is 16-bits
    }
    return added;
}

bool RegSet::addDestinationOutputs(const Instruction &i)
{
    if (!i.getOpSpec().supportsDestination()) {
        return false;
    }
    bool added = false;

    int execSize = int(i.getExecSize());
    const auto &op = i.getDestination();
    auto typeSizeBits = TypeSizeInBitsWithDefault(op.getType(), 32);

    added |= addDestinationImplicitAccumulator(i);

    Region rgn = op.getRegion();
    switch (op.getKind()) {
    case Operand::Kind::DIRECT:
        // send target (a GRF, not null reg)
        if (i.getOpSpec().isSendOrSendsFamily() &&
            op.getDirRegName() == RegName::GRF_R)
        {
            int nregs = i.getDstLength();
            if (nregs < 0) {
                nregs = 8; // assume the worst
                // NOTE: some special messages allow 32;
                // we won't handle those cases
            }
            added |= addRegs(RegName::GRF_R, op.getDirRegRef().regNum, nregs);
        } else {
            // normal GRF target
            added |= setDstRegion(
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
        added |= setDstRegion(
            op.getDirRegName(),
            op.getDirRegRef(),
            Region::DST1,
            execSize,
            typeSizeBits);
        // don't track MME registers (see RegSet::addSourceInputs for why)
        // auto MathMacroReg = op.getMathMacroExt();
        // if (MathMacroReg != MathMacroExt::NOMME && MathMacroReg != MathMacroExt::INVALID) {
        //     // and the math macro register
        //     int mmeRegNum = int(MathMacroReg) - int(MathMacroExt::MME0);
        //     RegRef mmeRegRef(mmeRegNum, 0);
        //     added |= rs.setDstRegion(
        //         RegName::ARF_MME,
        //          mmeRegRef,
        //         Region::DST1,
        //         execSize,
        //         typeSizeBits);
        // }
        break;
    }
    case Operand::Kind::INDIRECT:
        // this is an input! not an output
        // (use addDestinationInputs or part of addSourceInputs)
        //
        // indirect destinations use a single a0 value
        // (no fancy scattering)
        // added |= setDstRegion(
        //    RegName::ARF_A,
        //    op.getIndAddrReg(),
        //    Region::DST1,
        //    1, // one element only
        //    16); // :w is 16-bits
        break;
    default:
        break;
    }
    return added;
}


bool RegSet::addFlagModifierOutputs(const Instruction &i)
{
    const FlagModifier fm = i.getFlagModifier();
    bool writesFlagRegister = // sel uses flag modifier as input
        fm != FlagModifier::NONE && i.getOp() != Op::SEL;
    if (writesFlagRegister) {
        const RegRef &fr = i.getFlagReg();
        return addFlagRegArf(fr, i.getExecSize(), i.getChannelOffset());
    }
    return false;
}


bool RegSet::addDestinationImplicitAccumulator(const Instruction &i)
{
    bool added = false;
    bool writesAcc =
        i.hasInstOpt(InstOpt::ACCWREN) || i.is(Op::MACH) ||
        i.is(Op::ADDC) || i.is(Op::SUBB);
    if (writesAcc) {
        Type type = i.getDestination().getType();
        if (i.is(Op::MACH)) {
            type = Type::Q;
        }
        auto typeSizeBytes = TypeSizeInBitsWithDefault(type, 32);
        auto execOff = 4 * int(i.getChannelOffset());
        auto elemsPerAccReg =
            8 * model->getBytesPerReg(RegName::ARF_ACC) / typeSizeBytes;
        RegRef ar(execOff / elemsPerAccReg, execOff % elemsPerAccReg);
        added |= setDstRegion(
            RegName::ARF_ACC,
            ar,
            Region::DST1,
            size_t(i.getExecSize()),
            typeSizeBytes);
    }
    return added;
}


bool RegSet::setDstRegion(
    RegName rn,
    RegRef rr,
    Region rgn,
    size_t execSize,
    size_t typeSizeBits)
{
    if (!isTrackedReg(rn))
        return false;

    size_t hz = size_t(rgn.getHz());
    // sets a region for a basic operand
    if (rgn.getHz() == Region::Horz::HZ_INVALID) {
        hz = 1;
    }

    bool added = false;
    size_t baseOffBits =
        rr.regNum * model->getBytesPerReg(rn) * 8 +
            rr.subRegNum * typeSizeBits;
    for (size_t ch = 0; ch < execSize; ch++) {
        size_t offsetBits = ch * hz * typeSizeBits;
        added |= add(rn, baseOffBits + offsetBits, typeSizeBits);
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
    if (!isTrackedReg(rn))
        return false;

    bool changed = false;
    size_t v = 1, w = 1, h = 0; // e.g. old-style default regions
    if (rgn != Region::INVALID) {
        v = size_t(rgn.getVt());
        w = size_t(rgn.getWi());
        w = w == 0 ? 1 : w;
        h = size_t(rgn.getHz());
    }

    size_t rowBaseBits = offsetOf(rn, rr, typeSizeBits);
    size_t rows = execSize / w;
    rows = (rows != 0) ? rows : 1;
    for (size_t y = 0; y < rows; y++) {
        size_t offsetBits = rowBaseBits;
        for (size_t x = 0; x < w; x++) {
            changed |= add(rn, offsetBits, typeSizeBits);
            offsetBits += h * typeSizeBits;
        }
        rowBaseBits += v * typeSizeBits;
    }
    return changed;
}


// emits a shorter description... something like
//   {r0,r1[0-3],r2[16],r4[0-3,8-11,16-19,24-27]}
void RegSet::str(std::ostream &os) const
{
    bool first = true;
    os << "{";
    for (RegName rn : TRACKED) {
        const RegInfo *ri = model->lookupRegInfoByRegName(rn);
        auto emitReg = [&](size_t i) {
            if (first)
                first = false;
            else
                os << ",";
            os << ri->syntax;
            if (ri->hasRegNum()) {
                os << (int)i;
            }
        };

        size_t bytesPerReg = model->getBytesPerReg(rn);
        const BitSet<> &bs = bitSetFor(rn);
        for (size_t ri = 0; ri < model->getRegCount(rn); ri++) {
            size_t regOff = offsetOf(rn, (int)ri) / 8;
            if (bs.testAll(regOff, bytesPerReg)) {
                // full reg: e.g. "r13" or "r13:8"
                emitReg(ri);
                size_t len = 1;
                if (rn == RegName::GRF_R || rn == RegName::ARF_ACC) {
                    // for GRF and ACC permit payload-form syntax
                    // r10:8, acc0:2
                    while (ri + len < model->getRegCount(rn) &&
                        bs.testAll(
                            offsetOf(rn, (int)ri + (int)len) / 8, bytesPerReg))
                    {
                        len++;
                    }
                }
                if (len > 1) {
                    os << ":" << len;
                    ri += len - 1;
                }
            } else if (bs.testAny(regOff, bytesPerReg)) {
                // partial register
                //    r13[7]   (e.g. r13.7<0;1,0>:b)
                //    r13[0-3,8-11,..]  (e.g. r13.0<2;1,0>:d)
                emitReg(ri);
                os << '[';
                bool firstSubreg = true;
                for (size_t byte = 0; byte < bytesPerReg; byte++) {
                    if (!bs.test(regOff + byte)) {
                        continue;
                    }
                    if (firstSubreg) {
                        firstSubreg = false;
                    } else {
                        os << ',';
                    }
                    os << byte;
                    // find the end
                    size_t len = 1;
                    while (byte + len < bytesPerReg &&
                        bs.test(regOff + byte + len))
                        len++;
                    if (len > 1) {
                        os << '-' << byte + len - 1;
                        byte += len - 1;
                    }
                }
                os << ']';
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

