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
                    iga::EmitSendDescriptorInfo(
                        model->platform,
                        i.getOpSpec(),
                        exDesc,
                        desc.imm,
                        ss);
                }
            } else if (desc.type == SendDescArg::IMM) {
                // try to get the imm send desc info via GED
                iga::EmitSendDescriptorInfo(
                    model->platform,
                    i.getOpSpec(),
                    exDesc,
                    desc.imm,
                    ss);
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
            int16_t val = op.getIndImmAddr();
            if (!opts.syntaxExtensions) {
                emit(',');
            } else if (op.getIndImmAddr() > 0) {
                emit(" + ");
            } else {
                emit(" - ");
                val = -val;
            }
            emit(val);
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

} // end: namespace *iga
