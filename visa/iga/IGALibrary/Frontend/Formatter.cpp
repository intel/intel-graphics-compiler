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
#include "LdStSyntax/MessageFormatting.hpp"
#include "../ErrorHandler.hpp"
#include "../IR/Instruction.hpp"
#include "../IR/Types.hpp"
#include "../Models/Models.hpp"
#include "../strings.hpp"
#ifndef DISABLE_ENCODER_EXCEPTIONS
#include "../Backend/GED/Interface.hpp"
#include "../Backend/Native/Interface.hpp"
#endif
#include "../Backend/GED/IGAToGEDTranslation.hpp"
#include "../Backend/Native/MInst.hpp"
#include "../Backend/MessageInfo.hpp"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <vector>

// shows formatting output in realtime to stderr (for stepping through code)
// #define TRACE_EMIT

namespace iga
{
    static void EmitSendDescriptorInfoDSD(
        Platform p,
        const OpSpec &os,
        const SendDescArg &exDesc0,
        uint32_t desc,
        std::stringstream &ss);


class Formatter : public BasicFormatter
{
protected:
    ErrorHandler&                errorHandler;
    const Model                 *model;
    const FormatOpts&            opts;
    struct ColumnPreferences     cols;
    const Instruction           *currInst;
    const uint8_t               *bits = nullptr; // optional bits to render

    void warning(const char *msg) {
        if (currInst) {
            errorHandler.reportWarning(currInst->getLoc(), msg);
        } else {
            errorHandler.reportWarning(Loc::INVALID, msg);
        }
    }

    void formatDstOp(const OpSpec &os, const Operand &dst);
    void formatSrcOp(
        int srcIx,
        const OpSpec &os,
        const Instruction &inst,
        SourceIndex ix);
    void formatRegister(
        RegName rnm,
        RegRef reg,
        bool emitSubReg = true);
    void formatDstType(const OpSpec &os, Type type);

public:
    Formatter(
        ErrorHandler& err,
        std::ostream& out,
        const FormatOpts &fopts,
        const ColumnPreferences &colPrefs = ColumnPreferences())
        : BasicFormatter(out)
        , errorHandler(err)
        , opts(fopts)
        , cols(colPrefs)
        , currInst(nullptr)
    {
        model = Model::LookupModel(opts.platform);
        IGA_ASSERT(model, "invalid model");
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
        bits = (const uint8_t *)vbits;

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

            if (bits) {
                bits += i->hasInstOpt(InstOpt::COMPACTED) ? 8 : 16;
            }
        }
    }

    void formatInstructionBits(const Instruction& i, const void *vbits) {
        const uint32_t *bits = (const uint32_t *)vbits;
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
    }

    // for single instruction
    void formatInstruction(const Instruction& i, const void *vbits) {
        bits = (const uint8_t *)vbits;
        formatInstruction(i);
    }

    void formatInstruction(const Instruction& i) {
        currInst = &i;

        if (opts.printInstBits && bits != nullptr) {
            formatInstructionBits(i, bits);
        }

        if (opts.printLdSt && i.getOpSpec().isSendOrSendsFamily()) {
            // try new LD/ST format
            formatWithExperimentalSendSyntax(i, bits);
        } else {
            formatNormalInstructionBody(i, "", bits);
        }

        currInst = nullptr;
    }

    // returns tue if we were able to format the instruction
    void formatWithExperimentalSendSyntax(
        const Instruction& i,
        const void *bits)
    {
        auto fr = FormatLdStInstruction(*model, i);
        if (!fr.success()) {
            formatNormalInstructionBody(i, fr.errMessage, bits);
            return;
        }
        formatMaskAndPredication(
            i.getMaskCtrl(),
            i.getPredication(),
            i.getFlagReg());

        startColumn(cols.opCodeExecInfo + 12);
        emit(' ');
        emit(fr.opcode);
        emit(' ');
        formatExecInfo(i);
        finishColumn();

        emit(' ');
        startColumn(cols.dstOp + 4);
        emit(fr.dst);
        finishColumn();
        emit("  ");
        startColumn(cols.srcOp);
        emit(fr.src);
        finishColumn();

        formatInstOpts(i, fr.instOpts);

        formatEolComments(i, "");
    }

    void formatNormalInstructionBody(
        const Instruction& i,
        const std::string &debugSendDecode,
        const void *bits)
    {
        const OpSpec &os = i.getOpSpec();
        formatMaskAndPredication(
            i.getMaskCtrl(),
            i.getPredication(),
            i.getFlagReg());
        emit(' ');
        formatOpMnemonicExecInfo(i);
        emit(' ');
        formatFlagModifier(i);

        int nSrcs = i.getSourceCount();
        bool hasNonEmptyInstOpts = hasInstOptTokens(i);

        if (os.supportsDestination()) {
            // destination ops
            emit("  ");
            formatDstOp(os, i.getDestination());
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
                formatSrcOp(0, os, i, SourceIndex::SRC0);
            }
            if (nSrcs >= 2) {
                emit("  ");
                formatSrcOp(1, os, i, SourceIndex::SRC1);
            } else if (!os.isSendFamily()) {
                // ensure at least two columns worth of sources, with
                // an exception for send.  We poach those spaces for the
                // descriptors.
                startColumn(cols.srcOp);
                finishColumn();
            }
            if (nSrcs >= 3) {
                emit("  ");
                formatSrcOp(2, os, i, SourceIndex::SRC2);
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
                emit('f'); formatRegRef(f);
                emit(ToSyntax(p.function));
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
            emit(ToSyntax(coff));
            emit(')');
        }
    }

    void formatOpMnemonicExecInfo(const Instruction& i) {
        startColumn(cols.opCodeExecInfo);

        const OpSpec& os = i.getOpSpec();

        if (os.isSubop() &&
            (!opts.syntaxExtensions || shortOpWouldBeAmbiguous(os)))
        {
            // if we're a subop and we can not OR may not use the short name
            // then we have to use the full name.
            // EXAMPLES:
            //  add         os.isSubop() fails => take the other path
            //  math.idiv   would not be ambiguous => take the other path to idiv
            //  sync.nop    shortOpWouldBeAmbiguous() passes => take this
            //                path even if syntax extensions are on, thus
            //                getting sync.nop rather than the ambiguous nop
            emit(os.fullMnemonic);
        } else {
            emit(os.mnemonic);
        }

        if (os.supportsBranchCtrl() &&
            i.getBranchCtrl() == BranchCntrl::ON)
        {
            emit(".b");
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
            emit('(');
            emit(ToSyntax(i.getFlagModifier()));
            emit(')');
            emit('f'); formatRegRef(i.getFlagReg());
        }

        finishColumn();
    }


    void formatDstRegion(const OpSpec &os, const Region &rgn) {
        if (os.hasImplicitDstRegion()) {
            if (os.implicitDstRegion() != rgn) {
                warning("dst region is not binary normal");
            } else {
                return;
            }
        }
        emit('<');
        formatRgnHz(rgn.getHz());
        emit('>');
    }


    void formatSourceModifier(const OpSpec &os, SrcModifier sm) {
        if (sm != SrcModifier::NONE) {
            emit(ToSyntax(os.op, sm));
        }
    }


    void formatSourceRegion(
        int srcIx, const OpSpec &os, ExecSize execSize, const Operand &src)
    {
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
        if (os.hasImplicitSrcRegionEq(srcIx, model->platform, execSize, rgn) || os.isBranching()) {
            // e.g. on some platforms certain branches have an implicit <0;1,0>
            // meaning we can omit it if the IR matches
            return;
        }

        // region e.g. <8;8,1>
        if (os.isTernary()) {
            // align1 ternary has form
            // op (...) dst<H>  src0<v;h>  src1<v;h>  src2<h>
            if (srcIx < 2) {
                formatRegionVH(rgn);
            } else {
                formatRegionH(rgn);
            }
        } else {
            // all other operands use the full <v;w,h>
            // this also handles <w,h> (VxH or Vx1 mode)
            formatRegionVWH(rgn);
        }
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
        if (os.isSyncSubFunc() && op.getDirRegName() == RegName::ARF_NULL)
            return;
        const Type& type = op.getType();
        bool lblArg =
                op.getKind() == Operand::Kind::LABEL ||
                op.getKind() == Operand::Kind::IMMEDIATE;
        if (os.hasImplicitSrcType(srcIx, lblArg, model->platform)) {
            auto expectedType = os.implicitSrcType(srcIx, lblArg, model->platform);
            if (expectedType == type) {
                return;
            }
        }
        emit(ToSyntax(type));
    }


    void formatRegionVWH(Region rgn) {
        if (rgn == Region::INVALID) {
            emit("<Region::INVALID>");
            return;
        }
        emit('<');
        Region::Vert vt = rgn.getVt();
        if (vt != Region::Vert::VT_VxH) {
            formatRgnVt(vt);
            emit(';');
        }
        formatRgnW(rgn.getWi());
        emit(',');
        formatRgnHz(rgn.getHz());
        emit('>');
    }


    void formatRegionVH(Region rgn) {
        emit('<');
        formatRgnVt(rgn.getVt());
        emit(';');
        if (rgn.getWi() != Region::Width::WI_INVALID) {
            warning("on <v;h> region w must be Region::WI_INVALID");
        }
        formatRgnHz(rgn.getHz());
        emit('>');
    }


    void formatRegionH(Region rgn) {
        emit('<');
        if (rgn.getVt() != Region::Vert::VT_INVALID) {
            warning("on <h> region v must be Region::VT_INVALID");
        }
        if (rgn.getWi() != Region::Width::WI_INVALID) {
            warning("on <h> region w must be Region::WI_INVALID");
        }
        formatRgnHz(rgn.getHz());
        emit('>');
    }


    void formatSendDesc(const SendDescArg& sd, int cw = 0) {
        startColumn(cols.sendDesc);
        if (sd.type == SendDescArg::IMM) {
            emitHex(sd.imm, cw);
        } else {
            emit('a');
            formatRegRef(sd.reg);
        }

        finishColumn();
    }


    void EmitSendDescriptorInfo(
        Platform p,
        const OpSpec &i,
        const SendDescArg& exDesc,
        uint32_t desc,
        std::stringstream &ss);


    void EmitSendDescriptorInfoGED(
        Platform p,
        const OpSpec &i,
        const SendDescArg& ex_desc,
        uint32_t desc,
        std::stringstream &ss);

    // e.g. op (8) ... {Compacted}
    //                 ^^^^^^^^^^^
    void formatInstOpts(const Instruction &i, const std::vector<const char *> &instOpts);
    bool hasInstOptTokens(const Instruction &i) const {
        bool hasDepInfo =
            opts.platform >= Platform::GEN12P1 &&
            i.getSWSB().hasSWSB();
        return !i.getInstOpts().empty() || hasDepInfo;
    }

    virtual bool shouldPrintSFID(Platform p) const { return true; };

    void formatEolComments(
        const Instruction &i,
        const std::string &debugSendDecode)
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
                [&](const Dep &d) { return d.use == &i; },
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
            SendDescArg exDesc = i.getExtMsgDescriptor();
            SendDescArg desc = i.getMsgDescriptor();
            if (exDesc.type == SendDescArg::IMM &&
                desc.type == SendDescArg::IMM)
            {
                // send with immediate descriptors, we can decode this to
                // something more sane in comments
                if (opts.printLdSt) {
                    // tried to format with ld/st syntax and ...
                    semiColon.insert();
                    // success, show the descriptors for debugging
                    if (debugSendDecode.empty()) {
                        fmtHex(ss, exDesc.imm);
                        ss << "  ";
                        fmtHex(ss, desc.imm);
                    } else {
                        ss << debugSendDecode;
                    }
                } else {
                    // ld/st syntax not enabled
                    //
                    // use GED accessors to determine descriptor info
                    EmitSendDescriptorInfoGED(
                        model->platform,
                        i.getOpSpec(),
                        exDesc,
                        desc.imm,
                        ss);
                    //
                    // manual descriptor decoding
                    // EmitSendDescriptorInfo(
                    //     model->platform,
                    //     i.getOpSpec(),
                    //     exDesc,
                    //     desc.imm,
                    //     ss);
                    //
                    // Using the Xdsd framework only
                    // EmitSendDescriptorInfoDSD(
                    //     model->platform, i.getOpSpec(), exDesc, desc.imm, ss);
                }
            } else if (desc.type == SendDescArg::IMM) {
                // try to get the imm send desc info via GED
                EmitSendDescriptorInfoGED(
                    model->platform,
                    i.getOpSpec(),
                    exDesc,
                    desc.imm,
                    ss);
                //
                // EmitSendDescriptorInfo(
                //     model->platform,
                //     i.getOpSpec(),
                //     exDesc,
                //     desc.imm,
                //     ss);
                //
                // EmitSendDescriptorInfoDSD(
                //     model->platform, i.getOpSpec(), exDesc, desc.imm, ss);
            }
        }

        if (ss.tellp() > 0) {
            // only add the comment if we emitted something
            emit(" // ");
            emit(ss.str());
        }
    }

    void formatRegIndRef(const Operand& op) {
        emit("r[a");
        formatRegRef(op.getIndAddrReg());
        if (op.getIndImmAddr() != 0) {
            if (!opts.syntaxExtensions) {
                emit(',');
            } else if (op.getIndImmAddr() > 0) {
                emit(" + ");
            } else {
                emit(" - ");
            }
            emit(op.getIndImmAddr());
        }
        emit(']');
    }


    void formatRegRef(const RegRef& ref) {
        emit((int)ref.regNum, '.', (int)ref.subRegNum);
    }


    void formatRgnVt(Region::Vert vt) {
        switch (vt) {
        case Region::Vert::VT_0: emit('0'); break;
        case Region::Vert::VT_1: emit('1'); break;
        case Region::Vert::VT_2: emit('2'); break;
        case Region::Vert::VT_4: emit('4'); break;
        case Region::Vert::VT_8: emit('8'); break;
        case Region::Vert::VT_16: emit("16"); break;
        case Region::Vert::VT_32: emit("32"); break;
        default:
            emitBinary((int)vt);
            emit('?');
        }
    }
    void formatRgnHz(Region::Horz hz) {
        switch (hz) {
        case Region::Horz::HZ_0: emit('0'); break;
        case Region::Horz::HZ_1: emit('1'); break;
        case Region::Horz::HZ_2: emit('2'); break;
        case Region::Horz::HZ_4: emit('4'); break;
        default:
            emitBinary((int)hz);
            emit('?');
        }
    }
    void formatRgnW(Region::Width wi) {
        switch (wi) {
        case Region::Width::WI_1: emit('1'); break;
        case Region::Width::WI_2: emit('2'); break;
        case Region::Width::WI_4: emit('4'); break;
        case Region::Width::WI_8: emit('8'); break;
        case Region::Width::WI_16: emit("16"); break;
        default:
            emitBinary((int)wi);
            emit('?');
        }
    }
}; //end: class Formatter


void Formatter::formatDstType(const OpSpec &os, Type type)
{
    if (os.isBranching() && model->supportsSimplifiedBranches()) {
        return; // doesn't support types
    }
    if (os.hasImplicitDstType(opts.platform)) {
        if (os.implicitDstType(opts.platform) != type) {
            emit(ToSyntax(type));
        } // else: it's already in in binary normal form
    } else {
        // type
        emit(ToSyntax(type));
    }
}


void Formatter::formatRegister(
    RegName rnm,
    RegRef reg,
    bool emitSubReg)
{
    const Model *m = Model::LookupModel(opts.platform);
    const RegInfo *ri = m->lookupRegInfoByRegName(rnm);
    if (ri == nullptr) {
        emit("INVALID");
        return;
    }
    emit(ri->syntax);
    if (rnm == RegName::ARF_NULL && reg.subRegNum == 0) {
        // just "null", note: null4 would be bad IR,
        // so we'd continue so they get that output
        return;
    }

    if (reg.regNum != 0 || ri->hasRegNum()) {
        emit((int)reg.regNum); // cast to force integral printing (not char)
    }
    // show the subreg if:
    //  - caller demands it (e.g. it's a nonsend) AND
    //       the register chosen has subregisters (e.g. not ce and null)
    //  - OR it's non-zero (either bad IR or something's there)
    if (emitSubReg && ri->hasSubregs() || reg.subRegNum != 0) {
        emit('.');
        emit((int)reg.subRegNum);
    }
}

void Formatter::formatDstOp(const OpSpec &os, const Operand &dst) {
    startColumn(os.isSendOrSendsFamily() ?
        cols.sendDstOp : cols.dstOp);

    if (dst.getDstModifier() == DstModifier::SAT) {
        emit("(sat)");
    }

    switch (dst.getKind()) {
    case Operand::Kind::DIRECT:
    case Operand::Kind::MACRO:
        formatRegister(
            dst.getDirRegName(),
            dst.getDirRegRef(),
            os.hasDstSubregister(opts.platform));
        if (dst.getKind() == Operand::Kind::MACRO) {
            emit(ToSyntax(dst.getMathMacroExt()));
        }
        break;
    case Operand::Kind::INDIRECT:
        formatRegIndRef(dst);
        break;
    default:
        emit("Operand::Kind::?");
    }

    Region dstRgn = dst.getRegion();
    if (!os.hasImplicitDstRegion() ||
        (dstRgn != os.implicitDstRegion() && dstRgn != Region::INVALID))
    {
        // some instructions don't have dst regions
        formatDstRegion(os, dst.getRegion());
    }
    formatDstType(os, dst.getType());

    finishColumn();
}

void Formatter::formatSrcOp(
    int srcIx,
    const OpSpec &os,
    const Instruction &inst,
    SourceIndex ix)
{
    const Operand &src = inst.getSource(ix);

    startColumn(os.isSendOrSendsFamily() ? cols.sendSrcOp : cols.srcOp);

    switch (src.getKind()) {
    case Operand::Kind::DIRECT: {
        formatSourceModifier(os, src.getSrcModifier());
        formatRegister(
            src.getDirRegName(),
            src.getDirRegRef(),
            os.hasSrcSubregister(srcIx, opts.platform));
        formatSourceRegion(srcIx, os, inst.getExecSize(), src);
        break;
    }
    case Operand::Kind::MACRO: {
        formatSourceModifier(os, src.getSrcModifier());
        formatRegister(
            src.getDirRegName(),
            src.getDirRegRef(),
            false);
        emit(ToSyntax(src.getMathMacroExt()));
        formatSourceRegion(srcIx, os, inst.getExecSize(), src);
        break;
    }
    case Operand::Kind::INDIRECT:
        formatSourceModifier(os, src.getSrcModifier());
        formatRegIndRef(src);
        formatSourceRegion(srcIx, os, inst.getExecSize(), src);
        break;
    case Operand::Kind::IMMEDIATE:
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
            // packed float format
            //    each of four 8 - bit immediate vector elements has 1 sign bit,
            //        3 bits for the biased exponent(bias of 3), and 4 bits for the significand :
        case Type::VF:
            emitHex(src.getImmediateValue().u32);
            break;
        default:
            emit("???");
        };
        break;
    case Operand::Kind::LABEL: {
        const Block *b = src.getTargetBlock();
        formatLabel(b ?
            b->getPC() : src.getImmediateValue().s32);
        break;
    }
    default: emit("???");
    }

    if (!model->supportsSimplifiedBranches() ||
        src.getKind() != Operand::Kind::LABEL)
    {
        formatSourceType(srcIx, os, src);
    }
    finishColumn();
} // end formatSrcOp()

void Formatter::formatInstOpts(
    const Instruction &i,
    const std::vector<const char *> &extraInstOpts)
{
    const auto &iopts = i.getInstOpts();

    bool hasDepInfo = false;
    bool hasSendInfo = false;

    const auto &di = i.getSWSB();
    hasDepInfo =
        (opts.platform >= Platform::GEN12P1) && di.hasSWSB();
    if (iopts.empty() && !hasDepInfo && !hasSendInfo && extraInstOpts.empty()) {
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
    if ((!iopts.empty() || hasSendInfo) && (hasDepInfo || !extraInstOpts.empty())) {
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

///////////////////////////////////////////////////////////////////////////////
//
// Public interfaces into the kernel

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


#ifndef DISABLE_ENCODER_EXCEPTIONS
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
    const iga::Model *model =
        iga::Model::LookupModel(static_cast<iga::Platform>(opts.platform));

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


// TODO: remove
// void FormatProgramSegment(
//    ErrorHandler &e,
//    std::ostream &o,
//    const Kernel &k,
//    uint32_t segStartPc,
//    uint32_t segNumInstructions,
//    LabelerFunction labeler,
//    void *labelerEnv)
//{
//    Formatter f(e, o, k.getPlatform(), labeler, labelerEnv);
//    f.formatKernelSegment(k, segStartPc, segNumInstructions);
//}


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
        if (missInfo.parent) {
            ss << missInfo.parent->fullMnemonic <<
                ".0x" << std::hex << missInfo.opcode << "?";
        } else {
            ss << "OP[" << std::hex << "]" << missInfo.opcode << "?";
        }
        s = ss.str();
    } else {
        s = os.fullMnemonic;
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




static uint32_t getBitField(uint32_t value, int ix, int len) {
    // shift is only well-defined for values <32, use 0xFFFFFFFF
    uint32_t mask = len >= 32 ? 0xFFFFFFFF : (1<<(uint32_t)len) - 1;
    return (value >> (ix % 32)) & mask;
}

static uint32_t getMsgLength(uint32_t desc)                     { return getBitField(desc, 25, 4); }
static uint32_t getHeaderBit(uint32_t desc)                     { return getBitField(desc, 19, 1); }
static uint32_t getRespLength(uint32_t desc)                    { return getBitField(desc, 20, 5); }
static uint32_t getFC(uint32_t desc)                            { return getBitField(desc, 0, 18); }
static uint32_t getMessageTypeDC0(uint32_t desc)                { return getBitField(desc, 14, 4); }
static uint32_t getMessageTypeCC(uint32_t desc)                 { return getBitField(desc, 14, 4); }
static uint32_t getMessageTypeDC1(uint32_t desc)                { return getBitField(desc, 14, 4); }
static uint32_t getScratchSpaceGRFsSize(uint32_t desc)          { return getBitField(desc, 12, 2); }
static uint32_t getScratchSpaceAddressOffset(uint32_t desc)     { return getBitField(desc, 0, 12); }
static uint32_t getMiscMsgDescBits(uint32_t desc)               { return getBitField(desc, 8, 6); }

static uint32_t getSFID(uint32_t exDesc)                        { return getBitField(exDesc, 0, 4); }
static uint32_t getSplitSendMsgLength(uint32_t exDesc)          { return getBitField(exDesc, 6, 4); }
static uint32_t getSrc1LengthFromExDesc(uint32_t exDesc)        { return getBitField(exDesc, 6, 5); }
static uint32_t getStatelessAddress(uint32_t exDesc)            { return getBitField(exDesc, 16, 16); }
static uint32_t getBindlessSurfaceBaseAddress(uint32_t exDesc)  { return getBitField(exDesc, 12, 19); }

static void formatMessageInfoDescription(
    Platform p,
    SFID sfid,
    const OpSpec &os,
    const SendDescArg &exDesc0,
    uint32_t desc,
    std::stringstream &ss)
{
    DiagnosticList ws, es;
    auto exDesc =
        exDesc0.type == SendDescArg::REG32A ? 0xFFFFFFFF : exDesc0.imm;
    auto mi =
        MessageInfo::tryDecode(p, sfid, exDesc, desc, ws, es, nullptr);
    if (!mi.description.empty()) {
            // other stuff is hairier; the description will have better info
            ss << " " << mi.description;
    } else {
        ss << "?";
    }
}
static void EmitSendDescriptorInfoDSD(
    Platform p,
    const OpSpec &os,
    const SendDescArg &exDesc0,
    uint32_t desc,
    std::stringstream &ss)
{
    uint32_t exDesc = exDesc0.type == SendDescArg::IMM ? exDesc0.imm : 0;
    SFID sfid = MessageInfo::sfidFromOp(p, os.op, exDesc);

    //////////////////////////////////////
    // emit the: "wr:3h+2, rd:4" part
    ss << "wr:" << getMsgLength(desc);
    bool hasHeaderBit = true;
    if (hasHeaderBit && getHeaderBit(desc)) {
        ss << "h";
    }
    int src1Len = 0;
    if (exDesc0.type == SendDescArg::IMM) {
        src1Len = getSrc1LengthFromExDesc(exDesc);
    }
    if (src1Len) {
        ss << "+" << src1Len;
    } else if (exDesc0.type == SendDescArg::REG32A) {
        ss << "+?";
    }
    ss << ", rd:" << getRespLength(desc) << ";";
    //
    /////////////////////////
    // now the message description
    formatMessageInfoDescription(p, sfid, os, exDesc0, desc, ss);
}

static const char *getSFIDString(Platform p, uint32_t sfid)
{
    static const char *HSWBDW_SFIDS[] = {
        "null",     // 0000b the null function
        nullptr,    // 0001b
        "sampler",  // 0010b new sampler
        "gateway",  // 0011b gateway

        "sampler",  // 0100b (old sampler encoding)
        "hdc.rc",   // 0101b render cache
        "hdc.urb",  // 0110b unified return buffer
        "spawner",  // 0111b thread spawner

        "vme",      // 1000b video motion estimation
        "hdc.ccdp", // 1001b constant cache
        "hdc.dc0",  // 1010b
        "pi",       // 1011b pixel interpolator

        "hdc.dc1",  // 1100b data-cache 1
        "cre",      // 1101b check and refinement
        nullptr,
        nullptr,
    };
    static const char *SKL_SFIDS[] = {
        HSWBDW_SFIDS[0],
        nullptr,
        HSWBDW_SFIDS[2],
        HSWBDW_SFIDS[3],

        "hdc.dc2",
        HSWBDW_SFIDS[5],
        HSWBDW_SFIDS[6],
        HSWBDW_SFIDS[7],

        HSWBDW_SFIDS[8],
        "hdc.dcro0", // 1001b data cache read only
        HSWBDW_SFIDS[10],
        HSWBDW_SFIDS[11],

        HSWBDW_SFIDS[12],
        HSWBDW_SFIDS[13],
        HSWBDW_SFIDS[14],
        HSWBDW_SFIDS[15],
    };
    const char **sfidTable = p < Platform::GEN9 ? HSWBDW_SFIDS : SKL_SFIDS;

    if (!sfidTable)
    {
        return nullptr;
    }

    return sfidTable[sfid];
}

static const char* MessageTypeEnumerationDisassembly[64] =
{
    "Scratch Block Read", // 0
    "Scratch Block Write", // 1
    "OWord Block Read", // 2
    "Unaligned OWord Block Read", // 3
    "OWord Dual Block Read", // 4
    "DWord Scattered Read", // 5
    "Byte Scattered Read", // 6
    "Memory Fence", // 7
    "OWord Block Write", // 8
    "OWord Dual Block Write", // 9
    "DWord Scattered Write", // 10
    "Byte Scattered Write", // 11
    "Transpose Read", // 12
    "Untyped Surface Read", // 13
    "Untyped Atomic Integer Operation", // 14
    "Untyped Atomic Integer Operation SIMD4x2", // 15
    "Media Block Read", // 16
    "Typed Surface Read", // 17
    "Typed Atomic Integer Operation", // 18
    "Typed Atomic Integer Operation SIMD4x2", // 19
    "Untyped Surface Write", // 20
    "Media Block Write", // 21
    "Typed Atomic Counter Operation", // 22
    "Typed Atomic Counter Operation SIMD4x2", // 23
    "Typed Surface Write", // 24
    "A64 Scattered Read", // 25
    "A64 Untyped Surface Read", // 26
    "A64 Untyped Atomic Integer Operation", // 27
    "A64 Untyped Atomic Integer Operation SIMD4x2", // 28
    "A64 Block Read", // 29
    "A64 Block Write", // 30
    "A64 Untyped Surface Write", // 31
    "A64 Scattered Write", // 32
    "Untyped Surface Read", // 33
    "A64 Scattered Read", // 34
    "A64 Untyped Surface Read", // 35
    "Byte Scattered Read", // 36
    "Untyped Surface Write", // 37
    "A64 Untyped Surface Write", // 38
    "A64 Scattered Write", // 39
    "Byte Scattered Write", // 40
    "Oword Block Read Constant Cache", // 41
    "Unaligned Oword Block Read Constant Cache", // 42
    "Oword Dual Block Read Constant Cache", // 43
    "Dword Scattered Read Constant Cache", // 44
    "Unaligned Oword Block Read Sampler Cache", // 45
    "Media Block Read Sampler Cache", // 46
    "Read Surface Info", // 47
    "Render Target Write", // 48
    "Render Target Read", // 49
    "Media Block Read", // 50
    "Typed Surface Read", // 51
    "Typed Atomic Operation", // 52
    "Memory Fence", // 53
    "Media Block Write", // 54
    "Typed Surface Write", // 55
    "Untyped Surface Read", // 56
    "Untyped Atomic Operation", // 57
    "Untyped Surface Write", // 58
    "A64 Untyped Atomic Float Add", // 59
    "Untyped Atomic Float Operation", // 60
    "A64 Untyped Atomic Float Operation", // 61
    "A64 Untyped Atomic Float Operation SIMD4x2", // 62
    NULL // 63
}; // MessageTypeEnumerationDisassembly[]
static const char* msgTypeExpandedString(GED_MESSAGE_TYPE msgType)
{
    const char * msgTypeString = nullptr;
    if (msgType >= 63)
    {
        msgTypeString = GED_GetMessageTypeString(msgType);
    }
    else
    {
        msgTypeString = MessageTypeEnumerationDisassembly[msgType];
    }
    return msgTypeString;
}

static const GED_RETURN_VALUE constructPartialGEDSendInstruction(
    ged_ins_t* ins, GED_MODEL gedP, GED_OPCODE op,
    bool supportsExMsgDesc, uint32_t exMsgDesc, uint32_t msgDesc)
{
    GED_RETURN_VALUE status = GED_InitEmptyIns(gedP, ins, op);

    status = GED_SetExecSize(ins, 16);

    if (supportsExMsgDesc && status == GED_RETURN_VALUE_SUCCESS) {
        status = GED_SetExDescRegFile(ins, GED_REG_FILE_IMM);
    }

    if (status == GED_RETURN_VALUE_SUCCESS) {
        status = GED_SetExMsgDesc(ins, exMsgDesc);
    }

    if (status == GED_RETURN_VALUE_SUCCESS) {
        status = GED_SetDescRegFile(ins, GED_REG_FILE_IMM);
    }

    if (status == GED_RETURN_VALUE_SUCCESS) {
        status = GED_SetMsgDesc(ins, msgDesc);
    }
    return status;
}

static const GED_RETURN_VALUE constructPartialGEDSendInstructionGen12(
    ged_ins_t* ins, GED_MODEL gedP, GED_OPCODE op,
    bool supportsExMsgDesc, uint32_t exMsgDesc, uint32_t msgDesc, GED_SFID sfid)
{
    GED_RETURN_VALUE status = GED_InitEmptyIns(gedP, ins, op);

    status = GED_SetExecSize(ins, 16);

    if (supportsExMsgDesc && status == GED_RETURN_VALUE_SUCCESS) {
            status = GED_SetExDescRegFile(ins, GED_REG_FILE_IMM);
    }

    if (status == GED_RETURN_VALUE_SUCCESS) {
        status = GED_SetSFID(ins, sfid);
    }

    if (status == GED_RETURN_VALUE_SUCCESS) {
        status = GED_SetExMsgDesc(ins, exMsgDesc);
    }

    if (status == GED_RETURN_VALUE_SUCCESS) {
        status = GED_SetDescRegFile(ins, GED_REG_FILE_IMM);
    }

    if (status == GED_RETURN_VALUE_SUCCESS) {
        status = GED_SetMsgDesc(ins, msgDesc);
    }
    return status;
}



static const uint32_t SAMPLER_ENGINE            = 2;
static const uint32_t SFID_DP_DC2               = 4; //SKL+
static const uint32_t SFID_SAMPLER_CACHE        = 4; //[SNB,BDW]
static const uint32_t SFID_DP_RC                = 5;
static const uint32_t SFID_SPAWNER              = 7;
static const uint32_t SFID_DP_CC                = 9; //[SNB,BDW]
static const uint32_t SFID_DP_DCRO              = 9; //SKL+
static const uint32_t SFID_DP_DC0               = 10;
static const uint32_t SFID_DP_DC1               = 12;

/*
extern uint32_t GED_CALLCONV GED_GetMessageLength(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);
extern uint32_t GED_CALLCONV GED_GetResponseLength(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);
extern uint32_t GED_CALLCONV GED_GetExMesgLength(ged_ins_t* ins, GED_RETURN_VALUE* result);
extern GED_HEADER_PRESENT GED_CALLCONV GED_GetHeaderPresent(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);

extern GED_MESSAGE_TYPE GED_CALLCONV GED_GetMessageTypeDP_SAMPLER(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);
extern GED_MESSAGE_TYPE GED_CALLCONV GED_GetMessageTypeDP_RC(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);
extern GED_MESSAGE_TYPE GED_CALLCONV GED_GetMessageTypeDP_DCRO(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);
extern GED_MESSAGE_TYPE GED_CALLCONV GED_GetMessageTypeDP_CC(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);

extern GED_MESSAGE_TYPE GED_CALLCONV GED_GetMessageTypeDP_DC2(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);
extern GED_MESSAGE_TYPE GED_CALLCONV GED_GetMessageTypeDP_DC1(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);

extern uint32_t GED_CALLCONV GED_GetMessageTypeDP0Category(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);
extern GED_MESSAGE_TYPE GED_CALLCONV GED_GetMessageTypeDP_DC0ScratchBlock(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);
extern GED_MESSAGE_TYPE GED_CALLCONV GED_GetMessageTypeDP_DC0Legacy(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);
extern GED_MESSAGE_TYPE GED_CALLCONV GED_GetMessageTypeDP_DC0(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);


const char* GED_GetMessageTypeString(GED_MESSAGE_TYPE MessageTypeValue)

extern uint32_t GED_CALLCONV GED_GetMessageTypeDP0Category(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);

extern uint32_t GED_CALLCONV GED_GetFuncControl(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);
extern GED_SUB_FUNC_ID GED_CALLCONV GED_GetSubFuncID(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);
extern uint32_t GED_CALLCONV GED_GetBindingTableIndex(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);

extern GED_BLOCK_SIZE GED_CALLCONV GED_GetBlockSize(const uint32_t msgDesc, const GED_MODEL modelId, GED_RETURN_VALUE* result);

extern GED_SFID GED_CALLCONV GED_GetSFID(ged_ins_t* ins, GED_RETURN_VALUE* result);
const char* SFIDEnumeration[16]
extern const char* GED_CALLCONV GED_GetSFIDString(GED_SFID SFIDValue);
*/
void Formatter::EmitSendDescriptorInfoGED(
    Platform p,
    const OpSpec &os,
    const SendDescArg& ex_desc,
    uint32_t desc,
    std::stringstream &ss)
{
    if (!os.isSendOrSendsFamily()) {
        return;
    }
    GED_RETURN_VALUE getRetVal = GED_RETURN_VALUE_INVALID_FIELD;
    GED_MODEL gedP = IGAToGEDTranslation::lowerPlatform(p);

    GED_OPCODE gedOP = GED_OPCODE_INVALID;
    if (os.isSendFamily()) {
        gedOP = GED_OPCODE_send;
    } else {
        gedOP = GED_OPCODE_sends;
    }

    ged_ins_t gedInst;
    bool has_ged_inst = false;
    GED_SFID gedSFID = GED_SFID_INVALID;
    bool sfidSupportsHeader = true;


    // For GEN12, extract sfid from subfunction, and construct the ged instruction
    // with the given sfid. Otherwise (for platform < GEN12), SFID is part of exDesc.
    if (p >= Platform::GEN12P1) {
        // construct the entire GED inst, if failed, only get the SFID
        if (ex_desc.type == SendDescArg::IMM) {
            getRetVal = constructPartialGEDSendInstructionGen12(
                &gedInst, gedP, gedOP, os.isSendsFamily(), ex_desc.imm, desc,
                IGAToGEDTranslation::lowerSFID(os.op));
            if (getRetVal == GED_RETURN_VALUE_SUCCESS)
                has_ged_inst = true;
        }
        if (getRetVal != GED_RETURN_VALUE_SUCCESS) {
            gedSFID = IGAToGEDTranslation::lowerSFID(os.op);
        }
    }
    if (p <= Platform::GEN11) {
        // if ex_desc is imm, construct the entire GED inst
        if (ex_desc.type == SendDescArg::IMM)
            getRetVal = constructPartialGEDSendInstruction(
                &gedInst, gedP, gedOP, os.isSendsFamily(), ex_desc.imm, desc);
        if (getRetVal == GED_RETURN_VALUE_SUCCESS)
            has_ged_inst = true;
    }

    // try to get sfid from gedInst
    if (has_ged_inst && gedSFID != GED_SFID_INVALID) {
        gedSFID = GED_GetSFID(&gedInst, &getRetVal);
    }

    // try to get sfid string
    const char *gedsfidString = nullptr;
    if (gedSFID != GED_SFID_INVALID) {
        gedsfidString = GED_GetSFIDString(gedSFID);
    }

    // emit sfid string
    // Note that <=GEN11 if the exmsg is not imm, we're not able to
    // extract the sfid
    if (gedsfidString && gedSFID != GED_SFID_INVALID && p <= Platform::GEN11) {
        ss << " " << gedsfidString;
    }

    // emit desc message
    uint32_t msgLength = GED_GetMessageLength(desc, gedP, &getRetVal);
    if (getRetVal != GED_RETURN_VALUE_SUCCESS) {
        ss << " wr:" << "INVALID";
    } else {
        ss << " wr:" << msgLength;
    }

    if (sfidSupportsHeader) {
        GED_HEADER_PRESENT headerPresent = GED_GetHeaderPresent(desc, gedP, &getRetVal);
        if (headerPresent == GED_HEADER_PRESENT_INVALID ||
            getRetVal != GED_RETURN_VALUE_SUCCESS)
        {
            ss << "h:INVALID";
        } else if (headerPresent == GED_HEADER_PRESENT_yes) {
            ss << "h";
        }
    }

    // emit src1 length
    uint32_t src1Len = 0;
    bool hasSrc1Len = false;
    if (os.isSendsFamily() && has_ged_inst) {
        src1Len = GED_GetExMsgLength(&gedInst, &getRetVal);
        if (getRetVal != GED_RETURN_VALUE_SUCCESS) {
            // in the case that we're able to construct ged inst, the
            // ex_desc must be imm
            src1Len = getSplitSendMsgLength(ex_desc.imm);
        }
        hasSrc1Len = true;
    }

    // get src1 length if ex_desc is imm
    // (at this case we must have ged instruction being constructed)
    if (p >= Platform::GEN12P1 && has_ged_inst) {
        getRetVal = GED_RETURN_VALUE_INVALID_MODEL;
        hasSrc1Len = true;
        if (getRetVal != GED_RETURN_VALUE_SUCCESS) {
            if (ex_desc.type == SendDescArg::IMM) {
                src1Len = getSrc1LengthFromExDesc(ex_desc.imm);
            } else {
                // otherwise we're not able to get the src1 length
                hasSrc1Len = false;
            }
        }
    }

    if (hasSrc1Len)
        ss << "+" << src1Len;
    else
        ss << "+?";

    uint32_t respLength = GED_GetResponseLength(desc, gedP, &getRetVal);
    if (getRetVal != GED_RETURN_VALUE_SUCCESS) {
        ss << ", rd:" << "INVALID";
    } else {
        ss << ", rd:" << respLength;
    }


    ss << ", ";
    bool foundMessage = true;
    bool isScratch = false;
    GED_MESSAGE_TYPE msgType = GED_MESSAGE_TYPE_INVALID;

    if (gedSFID != GED_SFID_INVALID && gedSFID != GED_SFID_NULL) {
        switch (gedSFID)
        {
        case GED_SFID_SAMPLER:    ///< all
            break;
        case GED_SFID_GATEWAY:    ///< all
            break;
            /// includes: GEN11
        case GED_SFID_DP_DC2:     ///< GEN10, GEN9
            msgType = GED_GetMessageTypeDP_DC2(desc, gedP, &getRetVal);
            break;
        case GED_SFID_DP_RC:      ///< all
            msgType = GED_GetMessageTypeDP_RC(desc, gedP, &getRetVal);
            break;
        case GED_SFID_URB:        ///< all
            break;
        case GED_SFID_SPAWNER:    ///< all
            break;
        case GED_SFID_VME:        ///< all
            break;
            /// includes: GEN11
        case GED_SFID_DP_DCRO:    ///< GEN10, GEN9
            msgType = GED_GetMessageTypeDP_DCRO(desc, gedP, &getRetVal);
            break;
        case GED_SFID_DP_DC0:     ///< all
            if (p <= iga::Platform::GEN7P5) {
                //IVB,HSW
                msgType = GED_GetMessageTypeDP_DC0(desc, gedP, &getRetVal);
            } else {
                //Starting with BDW
                if (GED_GetMessageTypeDP0Category(desc, gedP, &getRetVal) == 0) {
                    msgType = GED_GetMessageTypeDP_DC0Legacy(desc, gedP, &getRetVal);
                }
                else {
                    msgType = GED_GetMessageTypeDP_DC0ScratchBlock(desc, gedP, &getRetVal);
                    isScratch = true;
                }
            }
            break;
        case GED_SFID_PI:         ///< all
            break;
        /// includes: GEN11
        case GED_SFID_DP_DC1:     ///< GEN7.5, GEN8, GEN8.1, GEN9, GEN10
            msgType = GED_GetMessageTypeDP_DC1(desc, gedP, &getRetVal);
            break;
            /// includes: GEN11
        case GED_SFID_CRE:        ///< GEN7.5, GEN8, GEN8.1, GEN9, GEN10
            break;
        case GED_SFID_DP_SAMPLER: ///< GEN7, GEN7.5, GEN8, GEN8.1
            msgType = GED_GetMessageTypeDP_SAMPLER(desc, gedP, &getRetVal);
            break;
        case GED_SFID_DP_CC:      ///< GEN7, GEN7.5, GEN8, GEN8.1
            msgType = GED_GetMessageTypeDP_CC(desc, gedP, &getRetVal);
            break;
        default:
            break;
        }
    } else if (ex_desc.type == SendDescArg::IMM) {
        // Fail to get GED SFID, try to extract from the ex message
        uint32_t sfid = getSFID(ex_desc.imm);
        if (p >= Platform::GEN12P1)
            sfid = os.functionControlValue;

        if (sfid == SFID_SAMPLER_CACHE && p < iga::Platform::GEN9) {
            msgType = GED_GetMessageTypeDP_SAMPLER(desc, gedP, &getRetVal);
        } else if (sfid == SFID_DP_RC) {
            msgType = GED_GetMessageTypeDP_RC(desc, gedP, &getRetVal);
        } else if (sfid == SFID_DP_CC) {
            // SFID_DP_DCRO and SFID_DP_CC have the same value: Constant Cache Data Port
            if (p < iga::Platform::GEN9) {
                msgType = GED_GetMessageTypeDP_CC(desc, gedP, &getRetVal);
            }
            else {
                msgType = GED_GetMessageTypeDP_DCRO(desc, gedP, &getRetVal);
            }
        } else if (sfid == SFID_DP_DC0) { // dp0 SFID_DP_DC0 Data Cache Data Port
            if (p <= iga::Platform::GEN7P5) {
                //IVB,HSW
                msgType = GED_GetMessageTypeDP_DC0(desc, gedP, &getRetVal);
            } else {
                // Starting with BDW
                // Starting with BDW
                if (GED_GetMessageTypeDP0Category(desc, gedP, &getRetVal) == 0) {
                    msgType = GED_GetMessageTypeDP_DC0Legacy(desc, gedP, &getRetVal);
                } else {
                    msgType = GED_GetMessageTypeDP_DC0ScratchBlock(desc, gedP, &getRetVal);
                    isScratch = true;
                }
            }
        } else if (sfid == SFID_DP_DC1) { // dp1 SFID_DP_DC1 Data Cache Data Port 1
            msgType = GED_GetMessageTypeDP_DC1(desc, gedP, &getRetVal);
        } else if (sfid == SFID_DP_DC2) { // dp1 SFID_DP_DC1 Data Cache Data Port 1
            msgType = GED_GetMessageTypeDP_DC2(desc, gedP, &getRetVal);
        } else {
            foundMessage = false;
        }
    } // end exDesc is imm

    if (msgType == GED_MESSAGE_TYPE_INVALID ||
        getRetVal != GED_RETURN_VALUE_SUCCESS)
    {
        foundMessage = false;
    }

    if (foundMessage) {
        const char * msgTypeString = msgTypeExpandedString(msgType);
        if (msgTypeString) {
            ss << msgTypeString;
        } else {
            ss << msgType;
        }
    }

    if (!foundMessage) {
        //
        // uint32_t fc = GED_GetFuncControl(desc, gedP, &getRetVal);
        // if (getRetVal != GED_RETURN_VALUE_SUCCESS) {
        //     ss << "fc: INVALID";
        // } else {
        //     ss << "fc: 0x" << std::hex << fc;
        // }
        //
        // Fallback to new MessageInfo decoder interface
        auto sfid = MessageInfo::sfidFromOp(p, os.op, ex_desc.imm);
        formatMessageInfoDescription(p, sfid, os, ex_desc, desc, ss);
    } else if (isScratch) {
        uint32_t off = 32*getScratchSpaceAddressOffset(desc);
        ss << " from scratch offset 0x" << std::hex << off;
    } else {
        ss << " msc:" << getMiscMsgDescBits(desc);
        ss << ", to ";
        uint32_t surf = GED_GetBindingTableIndex(desc, gedP, &getRetVal);
        if (getRetVal != GED_RETURN_VALUE_SUCCESS) {
            ss << "INVALID BTI";
        } else {
            if (surf == 254)
                ss << "SLM";
            // 255 - A32_A64 Specifies a A32 or A64 Stateless access that is locally coherent (coherent within a thread group)
            // 253 - A32_A64_NC Specifies a A32 or A64 Stateless access that is non-coherent (coherent within a thread).
            else if ((surf == 255 || surf == 253) && ex_desc.type == SendDescArg::IMM)
                ss << "global memory";
            // Bindless Surface Base address bits 25:6 in extMsgDes[12:31]
            else if (surf == 252 && ex_desc.type == SendDescArg::IMM)
                fmtHex(ss, getBindlessSurfaceBaseAddress(ex_desc.imm));
            else
                ss << "bti " << std::dec << surf;
        }
    }
    return;
}

void Formatter::EmitSendDescriptorInfo(
    Platform p,
    const OpSpec &i,
    const SendDescArg& exDesc0,
    uint32_t desc,
    std::stringstream &ss)
{
    auto exDesc = exDesc0.imm;
    if (!i.isSendOrSendsFamily()) {
        return;
    }

    uint32_t sfid = getSFID(exDesc);
    ss << " ";
    const char * sfidString = getSFIDString(p, exDesc);
    if (sfidString) {
        ss << sfidString;
    } else {
        ss << "sf:" << sfid;
    }

    ss << "  wr:" << getMsgLength(desc);
    if (getHeaderBit(desc)) {
        ss << "h";
    }
    if (i.isSendsFamily()) {
        ss << "+" << getSplitSendMsgLength(exDesc);
    }
    ss << ", rd:" << getRespLength(desc);

    uint32_t fc = getFC(desc);
    ss << ", ";
    bool foundMessage = false;
    bool isScratch = false;
    if (p < Platform::GEN9) {
        if (sfid == SFID_DP_CC) { // ccdp SFID_DP_CC Constant Cache Data Port
            foundMessage = true;
            switch (getMessageTypeCC(desc)) {
            case 0: ss << "rd.blk.ow (constant)"; break;
            case 1: ss << "rd.blk.ow (constant unaligned)"; break;
            case 2: ss << "rd.blk.ow (constant dual)"; break;
            case 3: ss << "rd.sca.dw (constant)"; break;
            default:
                foundMessage = false;
            }
        } else if (sfid == SFID_DP_DC0) { // dp0 SFID_DP_DC0 Data Cache Data Port
            uint32_t message = getMessageTypeDC0(desc); // [17:14]
            if (getBitField(desc, 18, 1) == 0) {
                static const char *legacy_table[] = {
                    // 0XXXX (legacy messages)
                    // 00XXX (reads + fences)
                    "rd.bkl.ow",
                    "rd.bkl.ow (unaligned)",
                    "rd.blk.ow (dual)",
                    "rd.sca.dw",  // 0 011b

                    "rd.sca.b",   // 0 100b
                    nullptr,
                    nullptr,
                    "memfence", // 0 111b

                    // 00XXX (writes + fences)
                    "wr.blk.ow",   // 1 000b
                    nullptr,
                    "wr.bkl.ow (dual)", // 1 010b
                    "wr.sca.dw",

                    "wr.sca.b", // 1 100b
                    nullptr,
                    nullptr,
                    nullptr,
                };
                if (legacy_table[message]) {
                    ss << legacy_table[message];
                    foundMessage = true;
                }
            } else {
                // 1XXXX (scratch messages)
                // 10XXX (scratch reads)
                // [R/W] [OW/DW] [INV] [RESV] [BLKSZ]
                auto rw = getBitField(desc, 17, 1) ? "wr" : "rd";
                auto ty = getBitField(desc, 16, 1) ? "dw" : "ow";
                ss << rw << "." << "scr" << ty;
                if (getBitField(desc, 15, 1)) {
                    ss << " (invalidate after read)";
                }
                isScratch = true;
            }
        } else if (sfid == SFID_DP_DC1) { // dp1 SFID_DP_DC1 Data Cache Data Port 1
            const char *msg = nullptr;
            switch (getMessageTypeDC1(desc)) {
            case 0:  msg = "rd.trans"; break;
            case 1:  msg = "rd.usurf"; break;
            case 2:  msg = "at.usurf"; break;
            case 3:  msg = "at.usurf (simd4x2)"; break;
            case 4:  msg = "rd.bkl.media"; break;
            case 5:  msg = "rd.tsurf"; break;
            case 6:  msg = "at.tsurf"; break;
            case 7:  msg = "at.tsurf (simd4x2)"; break;
            // case 8:  reserved?
            case 9:  msg = "wr.usurf"; break;
            case 10: msg = "wr.bkl.media"; break;
            case 11: msg = "at.cnt"; break;
            case 12: msg = "at.cnt (simd4x2)"; break;
            case 13: msg = "wr.tsurf"; break;
            // everything else reserved
            default: break;
            }
            if (msg) {
                ss << msg;
                foundMessage = true;
            }
        }
    }
    if (!foundMessage) {
        ss << "fc: 0x" << std::hex << fc;
    } else if (isScratch) {
        ss << " (";
        switch (getBitField(desc, 12, 2)) {
        case 0: ss << "1grf"; break;
        case 1: ss << "2grfs"; break;
        case 2: ss << "4grfs"; break;
        case 3: ss << "8grfs"; break;
        }
        uint32_t off = getBitField(desc, 0, 12);
        ss << " from 0x" << std::hex << off << ")";
    } else {
        ss << " msc:" << getBitField(desc, 8, 6);
        ss << ", to ";
        uint32_t surf = getBitField(desc, 0, 8);
        if (surf == 254)
            ss << "SLM";
        else if (surf == 255 || surf == 253)
            fmtHex(ss, getBitField(exDesc, 16, 16));
        else
            ss << "#" << surf;
    }
}

} // end: namespace *iga
