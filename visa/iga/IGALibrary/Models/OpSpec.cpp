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
#include "OpSpec.hpp"
#include "../asserts.hpp"

using namespace iga;


bool OpSpec::hasImpicitEm() const {
    switch (op) {
    case Op::SYNC:
    case Op::NOP:
    case Op::ILLEGAL:
    case Op::JMPI:
    case Op::WAIT:
        return true;
    default:
        return false;
    }
}


bool OpSpec::hasDstSubregister(bool isMacro) const {
    return !isMacro && !isSendOrSendsFamily();
}


Region OpSpec::implicitDstRegion(bool isMacro) const {
    Region rgn;
    bool hasRgn = implicitDstRegion(rgn, isMacro);
    IGA_ASSERT(hasRgn, "dst does not have an implicit region");
    return rgn;
}


bool OpSpec::implicitDstRegion(Region &rgn, bool isMacro) const
{
    // TODO: pull from BXML tables
    if (isSendOrSendsFamily()) {
        rgn = Region::DST1;
        return true;
    } else if (isTypedBranch()) {
        // call and ret have an implicit <1> as well
        rgn = Region::DST1;
        return true;
    } else if (isMacro) {
        // e.g. madm and math.invm/sqrtm
        rgn = Region::DST1;
        return true;
    } else {
        rgn = Region::INVALID;
        return false;
    }
}


bool OpSpec::implicitDstTypeVal(Type &type) const {
    if (isSendFamily() && platform >= Platform::GEN8) {
        type = Type::UD;
        if (platform >= Platform::GEN12P1) {
            type = Type::UB;
        }
        return true;
    }
    type = Type::INVALID;
    return false;
}

bool OpSpec::hasSrcSubregister(int srcOpIx, bool isMacro) const
{
    // send instructions and math macros (including madm) don't emit
    // subregisters
    return !isSendOrSendsFamily() && !isMacro;
}

bool OpSpec::hasImplicitSrcRegion(
    int srcOpIx, ExecSize es, bool isMacro) const
{
    return implicitSrcRegion(srcOpIx, es, isMacro) != Region::INVALID;
}
Region OpSpec::implicitSrcRegion(
    int srcOpIx,
    ExecSize execSize,
    bool isMacro) const
{
    // TODO: fold this into implicitSrcRegionPtr and elide the macro hacking
    //
    // TODO: this needs to work off the table from BXML
    if (isSendFamily() && platform < Platform::GEN12P1) {
        return Region::SRC010;
    } else if (isMacro) {
        if (isTernary()) {
            // ternary macro: e.g. madm
            if (srcOpIx == 2) {
                return Region::SRCXX1;
            } else {
                if (execSize == ExecSize::SIMD1) {
                    return Region::SRC0X0;
                } else if (platform >= Platform::GEN12P1) {
                    return Region::SRC1X0;
                } else {
                    return Region::SRC2X1;
                }
            }
        } else {
            // basic macro: e.g. math.invm ...
            if (execSize == ExecSize::SIMD1) {
                return Region::SRC010;
            } else if (platform >= Platform::GEN12P1) {
                return Region::SRC110;
            } else {
                return Region::SRC221;
            }
        }
    } else if (isSendFamily() || isSendsFamily()) {
        // no regions on send's
        return Region::SRC010;
    }
    else {
        if (platform >= Platform::GEN12P1 && isBranching())
            return Region::SRC110;

        if (srcOpIx == 0) {
            switch (op) {
            case Op::JMPI:
            case Op::CALL:
            case Op::CALLA:
            case Op::BRD:
                return Region::SRC010;
                // GED won't let us set 221
            case Op::BRC:
                return Region::SRC221;
            case Op::RET: {
                if (platform >= Platform::GEN12P1)
                    return Region::SRC010;
                else
                    return Region::SRC221;
            }
            case Op::SYNC:
                return Region::SRC010;
            default:
                ; // fallthrough to return nullptr
            }
        } else if (srcOpIx == 1) {
            // Encoder encodes Src0 into Src1, we have to lie here;
            // <2;2,1> gets manually set in Src0 explicitly by
            // the encoder, this is just for Src1
            switch (op) {
            case Op::BRC:
            case Op::CALL:
            case Op::CALLA:
                return Region::SRC010;
            default:
                ; // fallthrough to return nullptr
            }
        }
        return Region::INVALID;
    }
}


bool OpSpec::isTypedBranch() const {
    // TODO: reduce this set as we are able to normalize bits
    return op == Op::BRD ||
        op == Op::BRC ||
        op == Op::RET ||
        op == Op::JMPI ||
        op == Op::CALL ||
        op == Op::CALLA;
}


bool OpSpec::supportsAccWrEn() const  {
    return
        !supportsBranchCtrl() &&
        !isBranching() &&
        !isSendOrSendsFamily() &&
        !is(Op::NOP) &&
        !is(Op::ILLEGAL);
}


bool OpSpec::supportsSubfunction() const {
    // TODO: this should be generated via the BXML generator
    // and accessed via
    return supportsBranchCtrl()
        || op == Op::MATH
        || op == Op::SEND || op == Op::SENDC
        || op == Op::SENDS || op == Op::SENDSC
        || op == Op::SEND
        || op == Op::SYNC
        ;
}


unsigned OpSpec::getSourceCount(Subfunction sf) const {
    if (is(Op::MATH)) {
        IGA_ASSERT(sf.isValid(), "invalid math function");
        return GetSourceCount(sf.math);
    } else if (format & UNARY)
        return 1;
    else if (format & BINARY)
        return 2;
    else if (format & TERNARY)
        return 3;
    else
        return 0;
}


bool OpSpec::isVariableLatency() const {
    return isSendOrSendsFamily()
        || is(Op::MATH)
        ;
}


bool OpSpec::isFixedLatency() const {
    switch (op) {
    case Op::SYNC:
    case Op::ILLEGAL:
    case Op::NOP:
        // these get shot down after DepChk
        return false;
    default:
        return !isVariableLatency();
    }
}


bool OpSpec::implicitSrcTypeVal(
    int srcOpIx,
    bool isImmOrLbl,
    Type& type) const
{
    // TODO: pull from BXML data (ideally somehow in the syntax)
    if (isTypedBranch()) {
        // branches no longer take types in GEN12
        if (platform >= Platform::GEN12P1) {
            type = Type::INVALID;
            return true;
        }
        // e.g. jmpi, call, or brc
        //   jmpi  r12.3:d
        //   brd   r12.3:d
        // we make the
        if (op == Op::BRC) {
            //   brc   r12.3[:d]   null[:ud]
            //   brc   LABEL[:d]   LABEL[:d]
            type = srcOpIx == 0 || isImmOrLbl ? Type::D : Type::UD;
        } else {
            type = Type::D;
        }
        return true;
    } else if (isBranching()) {
        // if, else, endif, while, break, cont, goto, join, ...
        // let GED pick the defaults
        type = Type::INVALID;
        return true;
    } else if (isSendFamily() && platform < Platform::GEN12P1) {
        // TRB: we don't print the type on send instructions unless it's
        // not :ud, this allows us to phase out types on send operands
        // while meaningless, apparently there is a requirement on SKL
        // requiring the sampler to read the type from the operand.
        //
        // Types on sends are totally gone in GEN12.
        type = Type::UD;
        return true;
    } else if (isSendOrSendsFamily()) {
        // for sends src0 is :ud, src1 has no type bits
        if (platform < Platform::GEN12P1) {
            type = srcOpIx == 0 ? Type::UD : Type::INVALID;
        }
        else {
            type = Type::UB;
        }
        return true;
    } else if (is(Op::SYNC)) {
        // sync imm32          has ud type
        // sync reg32          type is required
        // sync null           type is ommitted
        if (isImmOrLbl) {
            type = Type::UD;
            return true;
        }
        type = Type::INVALID;
        return false;
    } else {
        type = Type::INVALID;
        return false;
    }
}


SWSB::InstType OpSpec::getSWSBInstType() const {
    SWSB::InstType instType = SWSB::InstType::OTHERS;
    if (isSendOrSendsFamily())
        instType = SWSB::InstType::SEND;
    else if (op == Op::MATH)
        instType = SWSB::InstType::MATH;
    return instType;
}