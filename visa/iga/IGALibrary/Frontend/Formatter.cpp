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
#include "ged.h"
#include "Floats.hpp"
#include "Formatter.hpp"
#include "IRToString.hpp"
#include "SendDescriptorDecoding.hpp"
#include "../ErrorHandler.hpp"
#include "../IR/Instruction.hpp"
#include "../IR/Messages.hpp"
#include "../IR/Types.hpp"
#include "../Models/Models.hpp"
#include "../strings.hpp"
#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
#include "../Backend/GED/Interface.hpp"
#include "../Backend/Native/Interface.hpp"
#endif
#include "../Backend/GED/IGAToGEDTranslation.hpp"
#include "../Backend/Native/MInst.hpp"
#include "../api/iga.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <vector>


namespace iga
{

class Formatter : public BasicFormatter
{
protected:
    ErrorHandler&                errorHandler;
    const Model                 *model;
    const FormatOpts&            opts;
    struct ColumnPreferences     cols;
    const Instruction           *currInst;
    const uint8_t               *currInstBits = nullptr; // optional to render encoding inline

    ansi_esc ANSI_FADED;
    ansi_esc ANSI_REGISTER(RegName rnm) const  {
        return rnm == RegName::GRF_R ? ANSI_REGISTER_GRF : ANSI_REGISTER_ARF;
    }
    ansi_esc ANSI_REGISTER_GRF;
    ansi_esc ANSI_REGISTER_ARF;
    ansi_esc ANSI_FLAGMODIFIER;
    ansi_esc ANSI_IMMEDIATE;
    ansi_esc ANSI_MNEMONIC;
    ansi_esc ANSI_SUBFUNCTION;
    ansi_esc ANSI_COMMENT;


    void formatDstOp(const Instruction &i);
    void formatSrcOp(SourceIndex ix, const Instruction &i);
    void formatBareRegisterUnescaped(RegName regName, int regNum);
    void formatRegister(
        RegName rnm,
        RegRef reg,
        bool emitSubReg = true,
        bool fadeSubreg = false);
    void formatDstType(const OpSpec &os, Type type);

public:
    Formatter(
        ErrorHandler& err,
        std::ostream& out,
        const FormatOpts &fopts,
        const ColumnPreferences &colPrefs = ColumnPreferences())
        : BasicFormatter(fopts.printAnsi, out)
        , errorHandler(err)
        , opts(fopts)
        , cols(colPrefs)
        , currInst(nullptr)
    {
        model = Model::LookupModel(platform());
        IGA_ASSERT(model, "invalid model");
        // TODO: could make these mappable via environment variable
        //
        // export IGA_FormatAnsiRegisterArf="\033[38;2;138;43;211m"
        // export IGA_FormatAnsiMnemonic=...
        // export IGA_FormatAnsiComment=...
        //
        // (Put this in system.cpp; iga::LookupEnvVar(...))
        if (fopts.printAnsi) {
            ANSI_FADED          = "\033[38;2;128;128;128m";
            ANSI_REGISTER_GRF   = "\033[38;2;255;192;203m";
            ANSI_REGISTER_ARF   = "\033[38;2;255;218;185m";
            ANSI_IMMEDIATE      = "\033[38;2;221;160;221m";
            ANSI_FLAGMODIFIER   = "\033[38;2;221;160;221m";
            ANSI_MNEMONIC       = "\033[38;2;10;153;215m"; // Intel Blue
            ANSI_SUBFUNCTION    = "\033[38;2;176;224;230m";
            ANSI_COMMENT        = "\033[38;2;128;128;128m";
        }
    }


    Platform platform() const {
        return opts.platform;
    }


    // labels are of the form "L" + off, but we prefix a
    // "_N" for negative.
    // E.g. "L64" is PC 64.
    // E.g. "L_N16" is -16 ("(N)egative")
    static void getDefaultLabelDefinition(std::ostream& o, int32_t pc) {
        o << "L";
        if (pc < 0)
        {
            o << "_N";
            pc = -pc;
        }
        o << pc;
    }


    void formatLabel(int32_t pc) {
        bool labeled = false;
        if (opts.labeler) {
            const char *userLabel = (*opts.labeler)(pc, opts.labelerContext);
            if (userLabel) {
                emit(userLabel);
                labeled = true;
            }
        }

        // use a default label if there isn't a labeler or the user callback
        // returned nullptr
        if (!labeled) {
            if (opts.numericLabels) {
                // Raw numbers for labels
                emitDecimal(pc);
            } else {
                Formatter::getDefaultLabelDefinition(o, pc);
            }
        }
    }


    void formatKernel(
        const Kernel& k,
        const void *vbits)
    {
        currInstBits = (const uint8_t *)vbits;

        for (const Block *b : k.getBlockList()) {
            if (!opts.numericLabels) {
                formatLabel(b->getPC());
                emit(':');
                newline();
            }

            formatBlockContents(*b);
        }
    }


    void formatBlockContents(const Block& b) {
        for (const auto &i : b.getInstList()) {
            formatInstruction(*i);
            newline();

            if (currInstBits) {
                currInstBits += i->hasInstOpt(InstOpt::COMPACTED) ? 8 : 16;
            }
        }
    }


    void formatInstructionBits(const Instruction& i, const void *vbits) {
        const uint32_t *bits = (const uint32_t *)vbits;
        emit(ANSI_COMMENT);
        o << std::hex;
        emit("/* [");
        o << setfill('0') << std::setw(4) << i.getPC();
        emit("] ");
        if (i.hasInstOpt(InstOpt::COMPACTED)) {
            emit("        ");
            emit(' ');
            emit("        ");
            emit(' ');
        } else {
            o << std::setw(8) << setfill('0') << bits[3];
            emit('`');
            o << std::setw(8) << setfill('0') << bits[2];
            emit('`');
        }
        o << std::setw(8) << setfill('0') << bits[1];
        emit('`');
        o << std::setw(8) << setfill('0') << bits[0];
        emit(" */ ");
        o << std::setfill(' ');
        o << std::dec;
        emit(ANSI_RESET);
    }


    // for single instruction
    void formatInstruction(const Instruction& i, const void *vbits) {
        currInstBits = (const uint8_t *)vbits;
        formatInstruction(i);
    }


    void formatInstruction(const Instruction& i) {
        currInst = &i;

        if (opts.printInstBits && currInstBits != nullptr) {
            formatInstructionBits(i, currInstBits);
        }

        const bool isSend = i.getOpSpec().isSendOrSendsFamily();
        if (isSend) {
            bool sendPrinted = false;
            if (!sendPrinted) {
                formatNormalInstructionBody(i, "");
            }
        } else {
            formatNormalInstructionBody(i, "");
        }

        currInst = nullptr;
    }

    bool formatLoadStoreSyntax(const Instruction& i);

    void formatNormalInstructionBody(
        const Instruction& i,
        const std::string& debugSendDecode)
    {

        formatInstructionPrefix(i);

        int nSrcs = i.getSourceCount();
        bool hasNonEmptyInstOpts = hasInstOptTokens(i);
        const OpSpec &os = i.getOpSpec();

        if (os.supportsDestination()) {
            // destination ops
            emit("  ");
            formatDstOp(i);
        } else {
            if (nSrcs > 0 || hasNonEmptyInstOpts) {
                // We only print this if there is something on the line following
                // this.  Checking this prevents useless spaces at the end of line.
                emitSpaces(cols.dstOp);
            }
        }

        // source ops
        if (nSrcs == 0) {
            // no source ops, but add spacing if we have instruction options
            // thus to prevent EOL spaces
            if (hasNonEmptyInstOpts) {
                emitSpaces(cols.srcOp);
            }
        } else {
            if (nSrcs >= 1) {
                emit("  ");
                formatSrcOp(SourceIndex::SRC0, i);
            }
            if (nSrcs >= 2) {
                emit("  ");
                formatSrcOp(SourceIndex::SRC1, i);
            } else if (!os.isSendFamily()) {
                // ensure at least two columns worth of sources, with
                // an exception for send.  We poach those spaces for the
                // descriptors.
                startColumn(cols.srcOp);
                finishColumn();
            }
            if (nSrcs >= 3) {
                emit("  ");
                formatSrcOp(SourceIndex::SRC2, i);
            }
        }

        // send descriptors
        if (os.isSendOrSendsFamily()) {
            emit("  ");
            formatSendDesc(i.getExtMsgDescriptor());
            emit("  ");
            formatSendDesc(i.getMsgDescriptor(), 8);
        }

        // instruction options
        std::vector<const char *> emptyExtraOpts;
        formatInstOpts(i, emptyExtraOpts);

        // EOL comments
        formatEolComments(i, debugSendDecode);
    }
private:


    void formatInstructionPrefix(const Instruction& i) {
        formatMaskAndPredication(i);
        emit(' ');
        formatOpMnemonicExecInfo(i);
        emit(' ');
        formatFlagModifier(i);
    }

    void formatMaskAndPredication(const Instruction &i) {
        formatMaskAndPredication(
            i.getMaskCtrl(),
            i.getPredication(),
            i.getFlagReg());
    }

    void formatMaskAndPredication(
        const MaskCtrl& mc,
        const Predication& p,
        const RegRef& f)
    {
        startColumn(cols.predication);
        bool hasPred = p.function != PredCtrl::NONE;
        if (mc == MaskCtrl::NOMASK || hasPred) {
            emit('(');
            if (mc == MaskCtrl::NOMASK) {
                emit(ToSyntax(mc));
                if (hasPred) {
                    emit('&');
                }
            }
            if (hasPred) {
                if (p.inverse) {
                    emit('~');
                }
                formatScalarRegRead(RegName::ARF_F, f);
                emitAnsi(ANSI_FLAGMODIFIER, ToSyntax(p.function));
            }
            emit(')');
        }

        finishColumn();
    }

    // even if they ask for short names, some ops may be ambiguous that way
    // e.g. "sync.nop" may not be shortened to "nop" because then it would
    // parse back as the wrong thing
    bool shortOpWouldBeAmbiguous(const OpSpec &us) const {
        for (const OpSpec *them : model->ops()) {
            if (us.op != them->op &&
                strcmp(us.mnemonic,them->mnemonic) == 0)
            {
                return true;
            }
        }
        return false;
    }

    void formatExecInfo(const Instruction& i) {
        const OpSpec& os = i.getOpSpec();
        if (!os.hasImpicitEm()) {
            auto es = i.getExecSize();
            auto coff = i.getChannelOffset();
            emit('(');
            emit(ToSyntax(es));
            emit('|');
            emitAnsi(coff == ChannelOffset::M0, ANSI_FADED,
                ToSyntax(coff));
            emit(')');
        }
    }

    void formatOpMnemonicExecInfo(const Instruction& i) {
        startColumn(cols.opCodeExecInfo);

        const OpSpec& os = i.getOpSpec();

        emitAnsi(ANSI_MNEMONIC, os.mnemonic);

        std::string subfunc;
        switch (os.op) {
        // case Op::SEL:
        // TODO: with syntax extensions we could treat
        //    sel (lt)f1.1 ...
        //  as
        //    sel.lt ... f1.1
        //
        //  and
        //
        //    (f0.1) sel ...
        //    (~f1.1) sel ...
        //    (~f1.1.any*) sel ...
        //  as
        //    sel.esc   ... f0.1
        //    sel.any*  ... f0.1
        case Op::MATH:   subfunc = (ToSyntax(i.getMathFc())); break;
        case Op::SEND:
        case Op::SENDC:
            if (platform() >= Platform::GEN12P1) {
                subfunc = ToSyntax(i.getSendFc());
            } // else part of ex_desc
            break;
        case Op::SYNC:   subfunc = ToSyntax(i.getSyncFc()); break;
        default:
            if (os.supportsBranchCtrl() &&
                i.getBranchCtrl() == BranchCntrl::ON)
            {
                subfunc += "b";
            }
            break;
        }

        if (!subfunc.empty()) {
            emit('.');
            emitAnsi(ANSI_SUBFUNCTION, subfunc);
        }

        emit(' ');
        formatExecInfo(i);

        finishColumn();
    }


    void formatFlagModifier(const Instruction &i) {
        startColumn(cols.flagMod);
        // should be check for suportsFlagModifier, but looks like our
        // models are wrong on math instruction.
        /*&& i.getOpSpec().supportsFlagModifier()*/
        if (i.hasFlagModifier() && !i.getOpSpec().isSendOrSendsFamily()) {
            emitAnsi(ANSI_FLAGMODIFIER,
                '(', ToSyntax(i.getFlagModifier()), ')');

            if (i.is(Op::SEL)) {
                // select actually uses the conditional modifier as a read
                // source, not a write: insanity
                formatScalarRegRead(RegName::ARF_F, i.getFlagReg());
            } else {
                formatScalarRegWrite(RegName::ARF_F, i.getFlagReg());
            }
        }

        finishColumn();
    }


    void formatDstRegion(const Region &rgn) {
        bool isVector = rgn.getHz() == Region::Horz::HZ_1;
        emitAnsi(isVector, ANSI_FADED, ToSyntax(rgn));
    }


    void formatSourceModifier(const OpSpec &os, SrcModifier sm) {
        if (sm != SrcModifier::NONE) {
            emit(ToSyntax(os.op, sm));
        }
    }


    void formatSourceRegion(SourceIndex srcIx, const Instruction &i) {
        int srcIxVal = static_cast<int>(srcIx);
        const OpSpec &os = i.getOpSpec();
        const Operand &src = i.getSource(srcIx);
        // breaks reversibiliy with:
        //  movi (8|M0)    r37.0<1>:uw   r[a0.0,64]<8;8,1>:uw  null<0;1,0>:ud
        // writes null out as null:ud and parses it as <1;1,0>
        // either need to complicate the default regioning rules by defaulting
        //  null:ud to <0;1,0>
        // but this makes default region a function of the register used
        // (additional complexity to those already complex rules)
        //
        // if (src.getDirRegName() == RegName::ARF_NULL) {
        //  // drop the region on all null operands
        // return;
        // }
        const Region &rgn = src.getRegion();
        if (os.hasImplicitSrcRegion(srcIxVal, i.getExecSize(), i.isMacro()) ||
            os.isBranching())
        {
            // e.g. on some platforms certain branches have an implicit <0;1,0>
            // meaning we can omit it if the IR matches
            return;
        }

        // region e.g. <8;8,1> or <1;0> ternary src0/src1 <1> for tern src2
        bool isVector = false, isScalar = false;
        if (os.isTernary()) {
            // align1 ternary has form
            // op (...) dst<H>  src0<v;h>  src1<v;h>  src2<h>
            if (srcIxVal < 2) {
                isVector = rgn == Region::SRC1X0;
                isScalar = rgn == Region::SRC0X0;
            } else {
                isVector = rgn == Region::SRCXX1;
                isScalar = rgn == Region::SRCXX0;
            }
        } else {
            // all other operands use the full <v;w,h>
            // this also handles <w,h> (VxH or Vx1 mode)
            isVector = rgn == Region::SRC110;
            isScalar = rgn == Region::SRC010;
        }
        bool fadeRegion =
            (i.getExecSize() == ExecSize::SIMD1 && isScalar) || isVector;
        emitAnsi(fadeRegion, ANSI_FADED, ToSyntax(rgn));
    }


    void formatSourceType(
        int srcIx,
        const OpSpec& os,
        const Operand &op)
    {
        if ((os.isBranching() && model->supportsSimplifiedBranches())) {
            // doesn't support types
            return;
        }
        if (os.is(Op::SYNC) && op.isNull())
            return;
        const Type& type = op.getType();
        bool lblArg =
                op.getKind() == Operand::Kind::LABEL ||
                op.getKind() == Operand::Kind::IMMEDIATE;
        if (os.hasImplicitSrcType(srcIx, lblArg)) {
            auto expectedType = os.implicitSrcType(srcIx, lblArg);
            if (expectedType == type) {
                return;
            }
        }

        // fade the type if it's uniform
        bool isUniformType =
            currInst && currInst->getDestination().getType() == type;
        emitAnsi(isUniformType, ANSI_FADED,ToSyntax(type));
    }


    void formatSendDesc(const SendDesc& sd, int cw = 0) {
        startColumn(cols.sendDesc);

        emit(ANSI_SUBFUNCTION);
        if (sd.isImm()) {
            emitHex(sd.imm, cw);
        } else {
            emit('a');
            formatRegRef(sd.reg);
        }
        emit(ANSI_RESET);

        finishColumn();
    }
    void formatSendExDesc(const SendDesc& sd, int cw = 0) {
        startColumn(cols.sendDesc);
        if (sd.isImm()) {
            emit(ANSI_IMMEDIATE);
            emitHex(sd.imm, cw);
        } else {
            emit(ANSI_REGISTER_ARF);
            emit('a');
            formatRegRef(sd.reg);
        }
        emit(ANSI_RESET);

        finishColumn();
    }


    // e.g. op (8) ... {Compacted}
    //                 ^^^^^^^^^^^
    void formatInstOpts(
        const Instruction &i, const std::vector<const char *> &instOpts);
    void formatInstOpts(const Instruction &i) {
        std::vector<const char *> emptyExtraOpts;
        formatInstOpts(i, emptyExtraOpts);
    }


    bool hasInstOptTokens(const Instruction &i) const {
        bool hasDepInfo =
            platform() >= Platform::GEN12P1 && i.getSWSB().hasSWSB();
        return !i.getInstOpts().empty() || hasDepInfo;
    }

    void formatEolComments(
        const Instruction &i,
        const std::string &debugSendDecode = "",
        bool decodeSendDesc = true)
    {
        std::stringstream ss;

        // separate all comments with a semicolon
        Intercalator semiColon(ss, "; ");

        if (opts.printInstPc) {
            semiColon.insert();
            ss << "[" << i.getPC() << "]: #" << i.getID();
        } else if (opts.liveAnalysis) {
            semiColon.insert();
            ss << "#" << i.getID();
        }

        if (opts.liveAnalysis) {
            // -Xprint-deps
            semiColon.insert();
            // emit all live ranges where this instruction is the use
            // this will indicate the source instruction
            intercalate(ss, ",", opts.liveAnalysis->deps,
                [&](const Dep &d) {return d.use == &i;},
                [&](const Dep &d) {
                    if (d.useType == Dep::READ) {
                        ss << "RAW";
                    } else {
                        ss << "WAW";
                    }
                    ss << " from #" << d.def->getID() << " ";
                    d.live.str(ss);
                    ss << " @" << d.minInOrderDist;
                });
        }

        const std::string &comment = i.getComment();
        if (!comment.empty()) {
            // custom comment attached to the instruction by some processor
            semiColon.insert();
            ss << comment;
        }

        if (i.getOpSpec().isSendOrSendsFamily()) {
            // send with immediate descriptors, we can decode this to
            // something more sane in comments
            const SendDesc exDesc = i.getExtMsgDescriptor(),
                              desc = i.getMsgDescriptor();
            //
            if (decodeSendDesc) {
                // ld/st syntax not enabled
                RegRef indDesc {0, 0};
                EmitSendDescriptorInfo(
                    platform(),
                    i.getSendFc(),
                    i.getExecSize(),
                    !i.getDestination().isNull(),
                    i.getDstLength(), i.getSrc0Length(), i.getSrc1Length(),
                    exDesc, desc, indDesc,
                    ss);
            } else if (opts.printLdSt) {
                // tried to format with ld/st syntax and ...
                if (ss.tellp() != 0) {
                    semiColon.insert();
                }
                // success, show the descriptors for debugging
                if (debugSendDecode.empty()) {
                    if (exDesc.isImm())
                        fmtHex(ss, exDesc.imm);
                    else
                        ss << "???";
                    ss << "  ";
                    if (desc.isImm())
                        fmtHex(ss, desc.imm);
                    else
                        ss << "???";
                }
            }
            if (!debugSendDecode.empty()) {
                semiColon.insert();
                ss << debugSendDecode;
            }
        }

        if (ss.tellp() > 0) {
            // only add the comment if we emitted something
            emitAnsi(ANSI_COMMENT, " // ", ss.str());
        }
    }


    void formatRegIndRef(const Operand& op) {
        emitAnsi(ANSI_REGISTER_GRF, "r");
        emit("[");

        formatScalarRegRead(RegName::ARF_A, op.getIndAddrReg());

        if (op.getIndImmAddr() != 0) {
            int16_t val = op.getIndImmAddr();
            if (!opts.syntaxExtensions) {
                emit(',');
            } else if (op.getIndImmAddr() > 0) {
                emit("+");
            } else {
                emit("-");
                val = -val;
            }
            emitAnsi(ANSI_IMMEDIATE, val);
        }
        emit(']');
    }


    void formatScalarRegWrite(RegName rnm, RegRef rr) {
        // we may distinguish in the future
        formatScalarRegRead(rnm, rr);
    }
    void formatScalarRegRead(RegName rnm, RegRef rr) {
        const auto *rinfo = model->lookupRegInfoByRegName(rnm);
        if (rinfo) {
            emit(ANSI_REGISTER(rnm));
            emit(rinfo->syntax);
        } else {
            emit("???");
        }
        formatRegRef(rr);
        emit(ANSI_RESET);
    }

    void formatRegRef(const RegRef& ref) {
        emit((int)ref.regNum, '.', (int)ref.subRegNum);
    }
}; //end: class Formatter


void Formatter::formatDstType(const OpSpec &os, Type type)
{
    if (os.isBranching() && model->supportsSimplifiedBranches()) {
        return; // doesn't support types
    }
    if (os.hasImplicitDstType()) {
        if (os.implicitDstType() != type) {
            emit(ToSyntax(type));
        } // else: it's already in in binary normal form
    } else {
        // type
        emit(ToSyntax(type));
    }
}




void Formatter::formatBareRegisterUnescaped(RegName regName, int regNum)
{
    const RegInfo *ri = model->lookupRegInfoByRegName(regName);
    if (ri == nullptr) {
        emit("RegName::???");
        return;
    }

    emit(ri->syntax);

    if (regNum != 0 || ri->hasRegNum()) {
        // some registers don't have numbers
        // e.g. null or ce
        emit(regNum);
    }
}

void Formatter::formatRegister(
    RegName regName,
    RegRef reg,
    bool emitSubReg,
    bool isSIMT)
{
    emit(ANSI_REGISTER(regName));

    // show the subreg if:
    //  - caller demands it (e.g. it's a nonsend) AND
    //       the register chosen has subregisters (e.g. not ce and null)
    //  - OR it's non-zero (either bad IR or something's there)
    const RegInfo *ri = model->lookupRegInfoByRegName(regName);
    if (ri == nullptr) {
        emit("RegName::???");
        return;
    }
    formatBareRegisterUnescaped(regName, (int)reg.regNum);

    if (emitSubReg && ri->hasSubregs() || reg.subRegNum != 0) {
        if (isSIMT)
            emit(ANSI_FADED); // a boring SIMT access => fade the text
        emit('.');
        emit((int)reg.subRegNum);
    }

    emit(ANSI_RESET);
}

void Formatter::formatDstOp(const Instruction &i)
{
    const OpSpec &os = i.getOpSpec();
    const Operand &dst = i.getDestination();

    startColumn(os.isSendOrSendsFamily() ? cols.sendDstOp : cols.dstOp);

    if (dst.getDstModifier() == DstModifier::SAT) {
        emit("(sat)");
    }

    switch (dst.getKind()) {
    case Operand::Kind::DIRECT:
    {
        bool isSIMT =
            i.getExecSize() > ExecSize::SIMD1 && // non-scalar op
            dst.getRegion() == Region::DST1 && // packed (non-strided) write
            dst.getDirRegRef().subRegNum == 0; // writing to the mid. of reg.
        formatRegister(
            dst.getDirRegName(),
            dst.getDirRegRef(),
            os.hasDstSubregister(i.isMacro()),
            isSIMT);
        break;
    }
    case Operand::Kind::MACRO:
        formatRegister(
            dst.getDirRegName(),
            dst.getDirRegRef(),
            os.hasDstSubregister(true));
        emitAnsi(ANSI_REGISTER(RegName::ARF_MME),
            ToSyntax(dst.getMathMacroExt()));
        break;
    case Operand::Kind::INDIRECT:
        formatRegIndRef(dst);
        break;
    default:
        emit("Operand::Kind::?");
    }

    Region dstRgn = dst.getRegion();
    if (!os.hasImplicitDstRegion(i.isMacro()) ||
        (dstRgn != os.implicitDstRegion(i.isMacro()) &&
            dstRgn != Region::INVALID))
    {
        // some instructions don't have dst regions
        formatDstRegion(dst.getRegion());
    }
    formatDstType(os, dst.getType());

    finishColumn();
}

void Formatter::formatSrcOp(
    SourceIndex srcIx,
    const Instruction &i)
{
    const Operand &src = i.getSource(srcIx);
    const OpSpec &os = i.getOpSpec();

    startColumn(os.isSendOrSendsFamily() ? cols.sendSrcOp : cols.srcOp);

    switch (src.getKind()) {
    case Operand::Kind::DIRECT: {
        bool hasSubreg =
            os.hasSrcSubregister(static_cast<int>(srcIx), i.isMacro());
        bool isSimt =
            i.getExecSize() > ExecSize::SIMD1 &&
                (src.getRegion() != Region::SRC110 ||
                 src.getRegion() != Region::SRC1X0 ||
                 src.getRegion() != Region::SRCXX1) &&
            src.getDirRegRef().subRegNum == 0;
        formatSourceModifier(os, src.getSrcModifier());
        formatRegister(
            src.getDirRegName(),
            src.getDirRegRef(),
            hasSubreg,
            isSimt);
        formatSourceRegion(srcIx, i);
        break;
    }
    case Operand::Kind::MACRO: {
        formatSourceModifier(os, src.getSrcModifier());
        formatRegister(
            src.getDirRegName(),
            src.getDirRegRef(),
            false);
        emit(ToSyntax(src.getMathMacroExt()));
        formatSourceRegion(srcIx, i);;
        break;
    }
    case Operand::Kind::INDIRECT:
        formatSourceModifier(os, src.getSrcModifier());
        formatRegIndRef(src);
        formatSourceRegion(srcIx, i);
        break;
    case Operand::Kind::IMMEDIATE:
        emit(ANSI_IMMEDIATE);
        switch (src.getType()) {
        case Type::B:
            emitDecimal(src.getImmediateValue().s8);
            break;
        case Type::W:
            emitDecimal(src.getImmediateValue().s16);
            break;
        case Type::D:
            emitDecimal(src.getImmediateValue().s32);
            break;
        case Type::Q:
            emitDecimal(src.getImmediateValue().s64);
            break;
        case Type::UB:
            emitHex(src.getImmediateValue().u8);
            break;
        case Type::UW:
            emitHex(src.getImmediateValue().u16);
            break;
        case Type::UD:
            emitHex(src.getImmediateValue().u32);
            break;
        case Type::UQ:
            emitHex(src.getImmediateValue().u64);
            break;
        case Type::HF:
            if (opts.hexFloats) {
                emitHex(src.getImmediateValue().u16);
            } else {
                FormatFloat(o, src.getImmediateValue().u16);
            }
            break;
        case Type::F:
            if (opts.hexFloats) {
                emitHex(src.getImmediateValue().u32);
            } else {
                emitFloat(src.getImmediateValue().f32);
            }
            break;
        case Type::DF:
            if (opts.hexFloats) {
                emitHex(src.getImmediateValue().u64);
            } else {
                emitFloat(src.getImmediateValue().f64);
            }
            break;
        case Type::V:
            emitHex(src.getImmediateValue().u32);
            break;
        case Type::UV:
            emitHex(src.getImmediateValue().u32);
            break;
        case Type::VF:
            // packed float format
            //  each of four 8 - bit immediate vector elements has 1 sign bit,
            //  3 bits for the biased exponent (bias of 3), and 4 bits
            //  for the significand
            emitHex(src.getImmediateValue().u32);
            break;
        default:
            emit("???");
        };
        emit(ANSI_RESET);
        break;
    case Operand::Kind::LABEL: {
        emit(ANSI_IMMEDIATE);
        const Block *b = src.getTargetBlock();
        formatLabel(b ?
            b->getPC() : src.getImmediateValue().s32);
        emit(ANSI_RESET);
        break;
    }
    default: emit("???");
    }

    if (!model->supportsSimplifiedBranches() ||
        src.getKind() != Operand::Kind::LABEL)
    {
        formatSourceType(static_cast<int>(srcIx), os, src);
    }
    finishColumn();
} // end formatSrcOp()


void Formatter::formatInstOpts(
    const Instruction &i,
    const std::vector<const char *> &extraInstOpts)
{
    const auto &iopts = i.getInstOpts();

    bool hasDepInfo = false;

    const auto &di = i.getSWSB();
    hasDepInfo =
        (platform() >= Platform::GEN12P1) && di.hasSWSB();
    if (iopts.empty() &&
        !hasDepInfo &&
        extraInstOpts.empty()) {
        return; // early out (no braces)
    }

    emit(" {");
    ToSyntaxNoBraces(o, iopts);
    // extra options germane to such ld/st syntax
    for (size_t opIx = 0; opIx < extraInstOpts.size(); opIx++) {
        if (opIx > 0) {
            emit(", ");
        }
        emit(extraInstOpts[opIx]);
    }
    if (!iopts.empty() && (hasDepInfo || !extraInstOpts.empty())) {
        // something was output, prepend a , for the depinfo
        emit(", ");
    }

    if (hasDepInfo) {
        switch (di.distType) {
        case SWSB::DistType::REG_DIST:
            emit("@");
            emit((int)di.minDist);
            break;
        default:
            break;
        }

        if (di.hasBothDistAndToken())
            emit(", ");

        switch (di.tokenType) {
        case SWSB::TokenType::DST:
            emit("$");
            emit((int)di.sbid);
            emit(".dst");
            break;
        case SWSB::TokenType::SRC:
            emit("$");
            emit((int)di.sbid);
            emit(".src");
            break;
        case SWSB::TokenType::SET:
            emit("$");
            emit((int)di.sbid);
            break;
        default:
            break;
        }
    }
    emit('}');
} // end formatInstOpts


bool Formatter::formatLoadStoreSyntax(const Instruction& i) {
    const auto desc = i.getMsgDescriptor();
    if (desc.isReg()) {
        // given a register descriptor, we've no hope of decoding the op
        return false;
    }
    const auto exDesc = i.getExtMsgDescriptor();
    RegRef indDesc = REGREF_INVALID;

    const auto sfid = i.getSendFc();
    const auto di =
        tryDecode(platform(), sfid, exDesc, desc, indDesc, nullptr);
    if (!di) {
        // if decode failed fallback to the canonical send syntax
        return false;
    } else if (!sendOpSupportsSyntax(platform(), di.info.op, sfid)) {
        // if decode succeeded, but the syntax isn't supported, then back off
        return false;
    }

    formatMaskAndPredication(i);

    const auto syntax = di.syntax;
    startColumn(cols.opCodeExecInfo + 12);
    emit(' ');
    emit(ANSI_MNEMONIC);
    emit(syntax.mnemonic); // e.g. load
    emit(ANSI_SUBFUNCTION);
    emit(syntax.controls); // e.g. .ugm.d32.a64.uc.ca
    emit(ANSI_RESET);
    emit(' ');
    formatExecInfo(i);
    finishColumn();

    auto emitPayload =
        [&] (const Operand &op, int len) {
            emit(ANSI_REGISTER(op.getDirRegName()));
            //
            formatBareRegisterUnescaped(
                op.getDirRegName(), (int)op.getDirRegRef().regNum);
            if (len >= 0) {
                emit(ANSI_FADED);
                emit(':');
                emitDecimal(len);
            }
            emit(ANSI_RESET);
            //
        };

    // emits the destination data register part (for load and atomic)
    auto emitDstDataReg =
        [&] () {
            startColumn(cols.dstOp);
            //
            int len = i.getDstLength();
            if (i.getDestination().getDirRegName() == RegName::ARF_NULL) {
            }
            emitPayload(i.getDestination(), len);
            //
            finishColumn();
        };

    // emits the [...]:2:a64 part
    auto emitAddrReg = [&] (int cols) {
        startColumn(cols);
        emit(syntax.surface);
        emit('[');
        emit(syntax.scale);
        //
        emitPayload(i.getSource(0), i.getSrc0Length());
        //
        emit(syntax.immOffset);
        emit(']');
        finishColumn();
    };

    emit("  ");
    if (syntax.isLoad()) {
        emitDstDataReg();
        emit(' ');
        emitAddrReg(cols.srcOp);
        //
    } else if (syntax.isStore()) {
        emitAddrReg(cols.dstOp);
        emit(' ');
        startColumn(cols.srcOp);
        emitPayload(i.getSource(1), i.getSrc1Length());
        finishColumn();
        //
    } else if (syntax.isAtomic()) {
        // atomic_fadd....d32.a64 ... null  [r10:2]   r12:2
        emitDstDataReg();
        emit(' ');
        emitAddrReg(cols.srcOp);
        emit(' ');
        startColumn(cols.srcOp);
        emitPayload(i.getSource(1), i.getSrc1Length());
        finishColumn();
        //
    } else {
        const auto &dst = i.getDestination();
        if (dst.getDirRegName() != RegName::ARF_NULL) {
            emitDstDataReg();
            emit(' ');
        }
        emitAddrReg(cols.srcOp);
    }

    emit(' ');

    // instruction options
    formatInstOpts(i);

    // EOL comments
    //
    // provide the descriptor for debugging
    std::stringstream ss;
    if (exDesc.isImm()) {
        ss << "ex_desc:" << fmtHex(exDesc.imm);
    } else { // exDesc.isReg()
        ss << "ex_desc:" << (int)exDesc.reg.subRegNum;
    }
    ss << "; desc:" << fmtHex(desc.imm);
    // provide the rlen=... (Dst.Length)

    formatEolComments(i, ss.str(), false);

    return true;
}


///////////////////////////////////////////////////////////////////////////////
//
// Public interfaces into the kernel
void FormatOpts::addApiOpts(uint32_t fmtOpts)
{
    numericLabels =
        (fmtOpts & IGA_FORMATTING_OPT_NUMERIC_LABELS) != 0;
    syntaxExtensions =
        (fmtOpts & IGA_FORMATTING_OPT_SYNTAX_EXTS) != 0;
    hexFloats =
        (fmtOpts & IGA_FORMATTING_OPT_PRINT_HEX_FLOATS) != 0;
    printInstPc =
        (fmtOpts & IGA_FORMATTING_OPT_PRINT_PC) != 0;
    printInstBits =
        (fmtOpts & IGA_FORMATTING_OPT_PRINT_BITS) != 0;
    printInstDeps =
        (fmtOpts & IGA_FORMATTING_OPT_PRINT_DEPS) != 0;
    printLdSt =
        (fmtOpts & IGA_FORMATTING_OPT_PRINT_LDST) != 0;
    printAnsi =
        (fmtOpts & IGA_FORMATTING_OPT_PRINT_ANSI) != 0;
}

void FormatKernel(
    ErrorHandler& e,
    std::ostream& o,
    const FormatOpts& opts,
    const Kernel& k,
    const void *bits)
{
    IGA_ASSERT(k.getModel().platform == opts.platform,
        "kernel and options must have same platform");
    Formatter f(e, o, opts);
    f.formatKernel(k, (const uint8_t *)bits);
}


void FormatInstruction(
    ErrorHandler& e,
    std::ostream& o,
    const FormatOpts& opts,
    const Instruction& i,
    const void *bits)
{
    Formatter f(e, o, opts);
    f.formatInstruction(i, (const uint8_t *)bits);
}


#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
void FormatInstruction(
    ErrorHandler &e,
    std::ostream &o,
    const FormatOpts &opts,
    const void *bits)
{
    FormatInstruction(e, o, opts, 0, bits);
}
void FormatInstruction(
    ErrorHandler &e,
    std::ostream &o,
    const FormatOpts &opts,
    size_t startPc,
    const void *bits,
    bool useNativeDecoder)
{
    const iga::Model *model = iga::Model::LookupModel(opts.platform);

    size_t instLen = ((const MInst *)bits)->isCompact() ? 8 : 16;

    DecoderOpts dopts;
    dopts.useNumericLabels = true;
    iga::Kernel *k = nullptr;
    if (useNativeDecoder && iga::native::IsDecodeSupported(*model, dopts)) {
        k = iga::native::Decode(*model, dopts, e, bits, instLen);
    } else {
        k = iga::ged::Decode(*model, dopts, e, bits, instLen);
    }
    if (e.hasErrors()) {
        for (const auto &err : e.getErrors()) {
            o << err.message << "\n";
        }
    }
    if (k) {
        const auto &bl = k->getBlockList();
        if (bl.size() > 0) {
            const auto &il = bl.front()->getInstList();
            if (il.size() > 0) {
                Instruction &i = *il.front();
                i.setPC((int32_t)startPc);
                FormatInstruction(e, o, opts, i, bits);
            }
        }
        delete k;
    }
}
#endif


void GetDefaultLabelName(
    std::ostream &o,
    int32_t pc)
{
    Formatter::getDefaultLabelDefinition(o, pc);
}


///////////////////////////////////////////////////////////////////////////////
// Following functions generally used for testing, debugging, or
// producing friendly diagnostics

///////////////////////////////////////////////////////////////////////////////
// The opname should be resolved
std::string FormatOpName(const iga::Model &model, const void *bits)
{
    std::string s;
    iga::OpSpecMissInfo missInfo = {0};
    const iga::OpSpec &os = model.lookupOpSpecFromBits(bits, missInfo);

    if (!os.isValid()) {
        std::stringstream ss;
        ss << "OP[" << std::hex << "]" << missInfo.opcode << "?";
        s = ss.str();
    } else {
        s = os.mnemonic;
    }

    s += ':';
    while (s.length() < 8)
        s += ' ';
    return s;
}
std::string FormatOpBits(const iga::Model &model, const void *bitsV)
{
    const uint8_t *bits = (const uint8_t *)bitsV;
    bool compact = ((const iga::MInst *)bits)->isCompact();
    int iLen = compact ? 8 : 16;

    std::stringstream ss;
    ss << iga::PadR(12, FormatOpName(model, bitsV));
    for (int i = 0; i < iLen; i++) {
        if (i > 0)
            ss << ' ';
        ss << std::hex << std::setw(2) << std::setfill('0') <<
            (uint32_t)bits[i];
        if (i == 7) {
            ss << ' ';
        }
    }
    return ss.str();
}

} // end: namespace *iga
