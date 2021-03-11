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
#include "FormatterJSON.hpp"
#include "IRToString.hpp"
#include "../IR/Messages.hpp"

#include <unordered_map>

using namespace iga;

class JSONFormatter: public BasicFormatter
{
protected:
    const Model                         &model;
    const FormatOpts&                    opts;
    const uint8_t                       *bits;
    int                                  currIndent = 2;

    // map instruction ID to all def/use pairs
    using DUMap = std::unordered_map<int,std::vector<const Dep*>>;
    DUMap                                depDefs;
    DUMap                                depUses;
public:
    JSONFormatter(std::ostream &o, const FormatOpts &os, const void *bs)
        : BasicFormatter(false, o)
        , opts(os)
        , model(*Model::LookupModel(os.platform))
        , bits((const uint8_t *)bs)
    {
        o << std::boolalpha;
        if (opts.liveAnalysis) {
            for (const Dep &d : opts.liveAnalysis->deps) {
                if (d.def != nullptr) {
                    depDefs[d.def->getID()].emplace_back(&d);
                }
                if (d.use != nullptr) {
                    depUses[d.use->getID()].emplace_back(&d);
                }
            }
        }
    }
    Platform platform() const {return model.platform;}

    // emits an error value that'll blow up on JSON load
    void emitIrError(const char *what) {
        emit("IR.error(\"", what, "\")");
    }

    void emitEscaped(const std::string &s) {
        for (size_t i = 0; i < s.size(); i++) {
            switch (s[i]) {
            case '\n': emit("\\n"); break;
            case '\t': emit("\\t"); break;
            case '\a': emit("\\a"); break;
            case '\"': emit("\\\""); break;
            case '\\': emit("\\\\"); break;
            default:   emit(s[i]); break;
            }
        }
    }

    void emitIndent() {
        for (int i = 0; i < currIndent; i++)
            emit(' ');
    }
    void withIndent(const std::function<void ()> &f) {
        currIndent += 2;
        f();
        currIndent -= 2;
    }

    void emitLabel(int32_t pc) {
        bool printed = false;
        if (opts.labeler) {
            const char *usrL = opts.labeler(pc, opts.labelerContext);
            if (usrL) {
                printed = true;
                emit(usrL);
            }
        }
        if (!printed) {
            std::stringstream ss;
            ss << std::setw(4) << std::dec << std::setfill('0') << pc;
            emit("L", ss.str());
        }
    }

    void emitKernel(const Kernel &k) {
        emit("const listing = {\n");
        emit("  version:\"1.0\",");
        emit("  platform:\"", model.names[0].str(),"\",");
        emit("  insts:[\n");
        currIndent += 2;

        for (const Block *b : k.getBlockList()) {

            emitIndent();
            emit("{kind:\"L\", value:\"");
                emitLabel(b->getPC()); emit("\"}");
            if (b != k.getBlockList().back() || !b->getInstList().empty())
                emit(",");
            emit("\n");

            for (const Instruction *iP : b->getInstList()) {
                emitInst(*iP);
                if (iP != b->getInstList().back() || b != k.getBlockList().back())
                    emit(",");
                emit("\n");
            }
        }

        currIndent -= 2;
        emit("]\n");
        emit("\n");
    }

    std::vector<std::string> comments;

    void emitInst(const Instruction &i) {
        comments.clear();

        const bool isSend = i.getOpSpec().isSendOrSendsFamily();
        emitIndent();
        emit("{");
        emit("kind:\"I\"");
        emit(", id:", i.getID(), ", pc:", fmtHex(i.getPC()));

        if (opts.printLdSt) {
            if (!emitLdStInst(i)) {
                emitNormalInst(i);
            }
        } else {
            emitNormalInst(i);
        }

        emitInstOpts(i);
        emit(", comment:");
        if (comments.empty()) {
            emit("null");
        } else {
            emit("\"");
            for (size_t i = 0; i < comments.size(); i++) {
                if (i > 0)
                    emit("; ");
                emitEscaped(comments[i]);
            }
            emit("\"");
        }
        emit("}");
    }

    void emitNormalInst(const Instruction &i) {
        std::string subfunc;
        switch (i.getOpSpec().op) {
        case Op::MATH: subfunc = ToSyntax(i.getSubfunction().math); break;
        case Op::SEND: subfunc = ToSyntax(i.getSubfunction().send); break;
        case Op::SENDC: subfunc = ToSyntax(i.getSubfunction().send); break;
        case Op::SYNC: subfunc = ToSyntax(i.getSubfunction().sync); break;
        default: break;
        }

        emitPredOpSubfuncExecInfoFlagReg(
            i, i.getOpSpec().mnemonic.str(), subfunc);
        // implicit accumulator def/uses
        emit(", acc:");
        if (opts.liveAnalysis) {
            emit("{");
            RegSet accIn(model);
            accIn.addSourceImplicitAccumulator(i);
            emitDepInputs(i, accIn);
            RegSet accOu(model);
            accOu.addDestinationImplicitAccumulator(i);
            emit(", "); emitDepOutputs(i, accOu);
            emit("}");
        } else {
            emit("null");
        }

        emit(",\n");

        withIndent([&] {
            emitIndent(); emit("dst:");
            emitDst(i); emit(",\n");
            emitIndent(); emit("srcs:[\n");
            withIndent([&] {
                for (unsigned srcIx = 0; srcIx < i.getSourceCount(); srcIx++) {
                    emitIndent(); emitSrc(i, srcIx);
                    if (srcIx != i.getSourceCount() - 1)
                        emit(",\n");
                }
                // if it's a send, create fake extra send parameters
                if (i.getOpSpec().isSendOrSendsFamily()) {
                    auto emitDesc = [&](SendDesc sd) {
                        emit(",\n"); emitIndent();
                        emit("{");
                        RegSet rs(model);
                        if (sd.type == SendDesc::Kind::REG32A) {
                            emitKindField(Operand::Kind::DIRECT);
                            emit(", reg:");
                            emitReg(RegName::ARF_A, sd.reg);
                            rs.setSrcRegion(RegName::ARF_A, sd.reg,
                                Region::SRC010, 1, 4);
                        } else {
                            emitKindField(Operand::Kind::IMMEDIATE);
                            emit(", value:");
                            emitHex(sd.imm);
                        }
                        emit(", rgn:null");
                        emit(", type:null");
                        emit(", "); emitDepInputs(i, rs);
                        emit("}");
                    };
                    emitDesc(i.getExtMsgDescriptor());
                    emitDesc(i.getMsgDescriptor());
                }
            });
            emit("\n"); emitIndent(); emit("]");
        });
    }


    /////////////////////////////////////////////////////////////////////////
    bool emitLdStInst(const Instruction &i) {
        const auto desc = i.getMsgDescriptor();
        if (desc.isReg()) {
            // given a register descriptor, we've no hope of decoding the op
            // TODO: once we have dataflow values maybe we can...
            return false;
        }
        const auto exDesc = i.getExtMsgDescriptor();
        RegRef indDesc = REGREF_INVALID;

        const auto sfid = i.getSendFc();
        const auto di =
            tryDecode(platform(), sfid, i.getExecSize(),
                exDesc, desc, indDesc, nullptr);
        if (!di) {
            // if message decode failed fallback to the canonical send syntax
            return false;
        } else if (!sendOpSupportsSyntax(platform(), di.info.op, sfid)) {
            // if decode succeeded but the syntax isn't supported, then bail
            return false;
        }

        emitPredOpSubfuncExecInfoFlagReg(
            i, di.syntax.mnemonic, di.syntax.controls);

        emit(",\n");
        withIndent([&] {
            emitIndent();
            if (!di.syntax.isStore()) {
                // there is an explicit destination
                const Operand &dst = i.getDestination();
                emit("dst:{");
                emitSendDataRegDst(i,
                    dst.getDirRegName(), dst.getDirRegRef().regNum,
                    i.getDstLength());
                emit("}");

            } else {
                emit("dst:null");
            }
            emit(",\n");

            /////////////////////////////////////////////
            // sources
            emitIndent(); emit("srcs:[\n");
            withIndent([&] {
                // everyone has src0 as an address
                emitIndent();
                emit("{kind:\"AD\"");
                //
                emit(", surf:");
                emitAddrSurfInfo(i, di.info);
                //
                emit(", scale:1");
                emit(", addr:");
                    emitSendPayloadSrc(i, 0);

                emit(", offset:");
                if (di.info.immediateOffset.isImm()) {
                    emit(di.info.immediateOffset.imm);
                } else {
                    // register is not permissable for offset right now but we
                    // plumb it to fit the IR
                    emitReg(RegName::ARF_A, di.info.immediateOffset.reg);
                }
                emit("},\n");

                //////////////////////////////////////
                // src1 (if not the old unary send)
                if (i.getSourceCount() > 1) {
                    emitIndent();
                    emitSendPayloadSrc(i, 1, "DA");
                    emit("\n");
                }
            });

            emitIndent(); emit("]"); // end of srcs:[..]
        });

        return true;
    }


    /////////////////////////////////////////////////////////////////////////
    // op+subfunc+exec size and exec offset
    void emitPredOpSubfuncExecInfoFlagReg(
        const Instruction &i, std::string mnemonic, std::string subop)
    {
        emitPred(i);

        emit(", op:\"", mnemonic, "\"");
        if (subop.empty()) {
            emit(", subop:null");
        } else {
            auto sf = subop[0] == '.' ? subop.substr(1) : subop;
            emit(", subop:\"", sf, "\"");
        }

        emitExecInfo(i);

        if (i.hasFlagModifier()) {
            emit(", fm:{cond:\"",ToSyntax(i.getFlagModifier()),"\"");
            emit(", ");
            RegSet rs(model);
            rs.addFlagModifierOutputs(i);
            emitDepOutputs(i, rs);
            emit("}");
        } else {
            emit(", fm:null");
        }

        emitFlagReg(i);
    }

    void emitExecInfo(const Instruction &i) {
        emit(", es:", ToSyntax(i.getExecSize()));
        emit(", eo:", 4*(int)(i.getChannelOffset()));
    }

    // pred:{func:[null|""|".any"|".all"|...],inv:[true|false]"",wren:[true|false]}
    void emitPred(const Instruction &i) {
        Predication p = i.getPredication();
        emit(", pred:");
        if (i.hasPredication()) {
            emit("{");
            emit("inv:", p.inverse);
            emit(", func:\"", ToSyntax(p.function), "\"");
            emit(", ");
            RegSet rs(model); rs.addPredicationInputs(i);
            emitDepInputs(i, rs);
            emit("}");
            // TODO: handle SEL
        } else {
            emit("null");
        }
        //
        emit(", wren:", i.getMaskCtrl() == MaskCtrl::NOMASK);
    }

    // {flag:{reg:...}}
    void emitFlagReg(const Instruction &i) {
        emit(", freg:");
        if (i.hasPredication() || (i.hasFlagModifier() && !i.is(Op::SEL))) {
            emitReg(RegName::ARF_F, i.getFlagReg());
        } else {
            emit("null");
        }
    }

    // dst:{kind=..., REST, sat:T|F, defs:..., uses:...}
    //  REST: if kind == R (direct)
    //     reg:IR.Regs.r13 or Regs.r(r13,4)
    //     rgn:IR.Rgns.DST1
    //     type:IR.Types.D
    void emitDst(const Instruction &i) {
        const OpSpec &os = i.getOpSpec();
        if (os.isSendOrSendsFamily()) {
            emitSendDst(i);
            return;
        }
        const Operand &dst = i.getDestination();
        if (!i.getOpSpec().supportsDestination()) {
            emit("null");
            return;
        }

        emit("{");
        emitKindField(dst.getKind());
        switch (dst.getKind()) {
        case Operand::Kind::DIRECT:
            emit(", reg:");
                emitReg(dst.getDirRegName(), dst.getDirRegRef());
            break;
        case Operand::Kind::MACRO:
            emit(", reg:");
                emitReg(dst.getDirRegName(), dst.getDirRegRef());
            emit(", ");
            emitMathMacroExtField(dst.getMathMacroExt());
            break;
        case Operand::Kind::INDIRECT:
            emit(", areg:");
                emitReg(RegName::ARF_A, dst.getIndAddrReg());
            emit(", aoff:", dst.getIndImmAddr());
            break;
        default:
            break;
        }
        emit(", sat:", dst.getDstModifier() == DstModifier::SAT);
        emit(", rgn:");
        if (os.hasImplicitDstRegion(i.isMacro())) {
            emit("null");
        } else {
            emitRgn(i, dst.getRegion(), true);
        }
        emit(", type:"); emitType(dst.getType());
        // def/uses
        emit(", ");
        RegSet rs(model);
        rs.addDestinationOutputs(i);
        emitDepOutputs(i, rs);
        emit("}");
    }


    void emitMathMacroExtField(MathMacroExt mme) {
        emit("mme:\"", ToSyntax(mme).substr(1), "\"");
    }


    void emitSrc(const Instruction &i, int srcIx) {
        const OpSpec &os = i.getOpSpec();
        if (os.isSendOrSendsFamily()) {
            emitSendSrc(i, srcIx);
            return;
        }
        const Operand &src = i.getSource(srcIx);
        emit("{");
        emitKindField(src.getKind());
        bool immOrLbl = false;
        switch (src.getKind()) {
        case Operand::Kind::DIRECT:
            emit(", reg:");
                emitReg(src.getDirRegName(), src.getDirRegRef());
            break;
        case Operand::Kind::MACRO:
            emit(", reg:");
                emitReg(src.getDirRegName(), src.getDirRegRef());
            emit(", ");
                emitMathMacroExtField(src.getMathMacroExt());
            emit(", rgn:");
                emitSrcRgn(i, srcIx);
            break;
        case Operand::Kind::INDIRECT:
        {
            emit(", areg:");
                emitReg(RegName::ARF_A, src.getIndAddrReg());
            emit(", aoff:", src.getIndImmAddr());
            break;
        }
        case Operand::Kind::IMMEDIATE:
        {
            emit(", value:");
            auto imm = src.getImmediateValue();
            switch (src.getType()) {
            case Type::UB:  emitHex(imm.u8); break;
            case Type::UW:  emitHex(imm.u16); break;
            case Type::UD:  emitHex(imm.u32); break;
            case Type::UQ:  emitHex(imm.u64); break;
            case Type::B:   emitDecimal(imm.s8); break;
            case Type::W:   emitDecimal(imm.s16); break;
            case Type::D:   emitDecimal(imm.s32); break;
            case Type::Q:   emitDecimal(imm.s64); break;
            case Type::HF:
                if (opts.hexFloats) {
                    emitHex(imm.u16);
                } else {
                    FormatFloat(o, imm.u16);
                }
                break;
            case Type::F:
                if (opts.hexFloats) {
                    emitHex(imm.u32);
                } else {
                    emitFloat(imm.f32);
                }
                break;
            case Type::DF:
                if (opts.hexFloats) {
                    emitHex(imm.u64);
                } else {
                    emitFloat(imm.f64);
                }
                break;
            case Type::V:    emitHex(imm.u32); break;
            case Type::UV:   emitHex(imm.u32); break;
            case Type::VF:   emitHex(imm.u32); break;
            default:
                emitIrError("invalid type for imm");
            }
            immOrLbl = true;
            break;
        }
        case Operand::Kind::LABEL:
        {
            const Block *b = src.getTargetBlock();
            emit(", target:\"");
            emitLabel(b ? b->getPC() : src.getImmediateValue().s32);
            emit("\"");
            immOrLbl = true;
            break;
        }
        default:
            break;
        }

        emit(", type:");
        if (os.hasImplicitSrcType(srcIx, immOrLbl)) {
            emit("null");
        } else {
            emitType(src.getType());
        }

        // even immediate fields will have this; it'll just be empty
        emit(", ");
        RegSet rs(model);
        rs.addSourceOperandInput(i, srcIx);
        emitDepInputs(i, rs);

        emit("}");
    }

    void emitSendDst(const Instruction &i) {
        const Operand &dst = i.getDestination();
        emit("{");
        if (i.getDstLength() >= 0) {
            // new style send (r13:4) (take bits from desc)
            emitSendDataRegDst(i,
                dst.getDirRegName(),
                dst.getDirRegRef().regNum,
                i.getDstLength());
        } else {
            emit("kind:\"RD\"");
            emit(", reg:");
                emitReg(dst.getDirRegName(), dst.getDirRegRef());
        }
        emit("}");
    }

    void emitSrcModifier(SrcModifier sm) {
        emit(", mods:",
            sm == SrcModifier::NEG ? "\"n\"" :
            sm == SrcModifier::ABS ? "\"a\"" :
            sm == SrcModifier::NEG_ABS ? "\"na\"" :
            "null");
    }

    void emitKindField(Operand::Kind k) {
        emit("kind:");
        switch (k) {
        case Operand::Kind::DIRECT:    emit("\"RD\""); break;
        case Operand::Kind::MACRO:     emit("\"RM\""); break;
        case Operand::Kind::INDIRECT:  emit("\"RI\""); break;
        case Operand::Kind::LABEL:     emit("\"LB\""); break;
        case Operand::Kind::IMMEDIATE: emit("\"IM\""); break;
        default: emitIrError("bad operand kind");
        }
    }

    void emitSrcRgn(const Instruction &i, int srcIx) {
        const Operand &src = i.getSource(srcIx);
        if (i.getOpSpec().hasImplicitSrcRegion(
            srcIx, i.getExecSize(), i.isMacro()))
        {
            emit("null");
        } else {
            emitRgn(i, src.getRegion(), false);
        }
    }

    void emitSendSrc(const Instruction &i, int srcIx) {
        const Operand &src = i.getSource(srcIx);

        if (srcIx == 0 && i.getSrc0Length() >= 0) {
            emit("{");
            RegRef dummy;
            const auto di =
                tryDecode(platform(), i.getSubfunction().send, i.getExecSize(),
                    i.getExtMsgDescriptor(), i.getMsgDescriptor(), dummy, nullptr);
            emit("kind:\"AD\"");
            emit(", surf:");
            if (di) {
                comments.push_back(di.info.description);
                emitAddrSurfInfo(i, di.info);
            } else {
                emit("null");
            }
            emit(", scale:1");
            emit(", addr:");
            emitSendPayloadSrc(i, 0);
            if (di && di.info.immediateOffset.isImm()) {
                // e.g. legacy scratch
                emit(", offset:", di.info.immediateOffset.imm);
            } else {
                // shouldn't be a reg, but we emit something
                emit(", offset:0");
            }
            emit("}");
        } else if (srcIx == 1 && i.getSrc1Length() >= 0) {
            emitSendPayloadSrc(i, 1, "DA");
        } else {
            // old send operand (treat as raw direct register access)
            emit("kind:\"RD\"");
            emit(", reg:");
                emitReg(src.getDirRegName(), src.getDirRegRef());
            emit(", rgn:null");
            emit(", type:null");
            // the best we can do is guess it's 1 without decoding the message
            emitSendPayloadDeps(i,
                src.getDirRegName(),
                src.getDirRegRef().regNum, 1, true);
        }
    }

    void emitAddrSurfInfo(const Instruction &i, const MessageInfo &mi) {
        emit("{type:");
        switch (mi.addrType) {
        case AddrType::FLAT: emit("\"flat\""); break;
        case AddrType::BTI:  emit("\"bti\""); break;
        default: emitIrError("invalid surface type");
        }

        emit(", offset:");
        RegSet surfOffDeps(model);
        if (mi.surfaceId.isReg()) {
            surfOffDeps.setSrcRegion(RegName::ARF_A,
                mi.surfaceId.reg,
                Region::SRC010, 1, 4);
            emitReg(RegName::ARF_A, mi.surfaceId.reg);
        } else {
            emitHex(mi.surfaceId.imm);
        }
        emit(", ");
        emitDepInputs(i, surfOffDeps);
        emit("}"); // end of surf:{...}
    }

    void emitSendPayloadSrc(const Instruction &i, int srcIx, const char *kind = nullptr) {
        const Operand &src = i.getSource(srcIx);
        RegName regName = src.getDirRegName();
        int regStart = src.getDirRegRef().regNum;
        int regCount = srcIx == 0 ? i.getSrc0Length() : i.getSrc1Length();
        emit("{");
        if (kind) {
            emit("kind:\"",kind,"\", ");
        }
        emit("reg:");
        emitReg(regName, regStart);
        emit(", len:", regCount);
        emit(", ");
        emitSendPayloadDeps(i, regName, regCount, regCount, true);
        emit("}");
    }

    void emitSendDataRegDst(
        const Instruction &i, RegName dataReg, int regNum, int regLen)
    {
        emit("kind:\"DA\"");
        emitSendPayloadFields(dataReg, regNum, regLen);
        emit(", ");
        emitSendPayloadDeps(i, dataReg, regNum, regLen, false);
    }

    void emitSendPayloadFields(RegName reg, int regNum, int numRegs)
    {
        emit(", reg:");
        emitReg(reg, regNum);
        emit(", len:", numRegs);
    }
    void emitSendPayloadDeps(
        const Instruction &i,
        RegName reg, int regNum, int numRegs, bool isRead)
    {
        RegSet rs(model);
        rs.addRegs(reg, regNum, numRegs);
        if (isRead) {
            emitDepInputs(i, rs);
        } else {
            emitDepOutputs(i, rs);
        }
    }

    void emitRgn(const Instruction &i, Region r, bool dst = false) {
        if (!dst) {
            // fix ternary regions
            if (r.getVt() == Region::Vert::VT_INVALID &&
                r.getWi() == Region::Width::WI_INVALID)
            {
                // ternary src2
                //    officially it was: w = 0 if h is 0 or E; v = w * h
                // use normalized regions here to simplify
                //   <0> ==> <0;1,0>
                //   <K> ==> <K;1,0>
                auto hz = r.getHz();
                // auto es = i.getExecSize();
                if (hz == Region::Horz::HZ_0) {
                    r = Region::SRC010;
                } else {
                    r = Region::SRC110;
                    r.set(Region::Vert(hz));
                }
            } else if (r.getWi() == Region::Width::WI_INVALID) {
                // ternary src0/src1
                //  w = 1 when v & h are 0
                //    = v when h = 0
                //    = v/h otherwise
                auto vt = r.getVt();
                auto hz = r.getHz();
                if (vt == Region::Vert::VT_0 && hz == Region::Horz::HZ_0) {
                    r.set(Region::Width::WI_1);
                } else if (hz == Region::Horz::HZ_0) {
                    r.set(Region::Width(int(vt)));
                } else {
                    r.set(Region::Width(int(vt)/int(hz)));
                }
            }
        }
        emitRgn(r, dst);
    }
    void emitRgn(Region r, bool dst = false) {
        emit("IR.Rgns.");
        if (dst) {
            switch (r.getHz()) {
            case Region::Horz::HZ_1: emit("d1"); break;
            case Region::Horz::HZ_2: emit("d2"); break;
            case Region::Horz::HZ_4: emit("d4"); break;
            default: emit("IR.error(\"invalid dst region\")");
            }
        }
        else
        {
            if (r == Region::SRC010) {
                emit("s0_1_0");
            } else if (r == Region::SRC110) {
                emit("s1_1_0");
            } else if (r == Region::SRC210) {
                emit("s2_1_0");
            } else if (r == Region::SRC410) {
                emit("s4_1_0");
            } else if (r.getVt() == Region::Vert::VT_VxH) {
                // <w,h> region for indrect access
                // let the JS library sort it out
                emit("sVxH(", int(r.getWi()), ",",
                    int(r.getHz()), ")");
            } else {
                emit("sVWH(",
                    int(r.getVt()), ",",
                    int(r.getWi()), ",",
                    int(r.getHz()), ")");
            }
        }
    }

    void emitType(Type t) {
        if (t == Type::INVALID) {
            emit("null");
            return;
        }
        emit("IR.Types."); // e.g. IR.Types.UQ
        auto s = ToSyntax(t); // ":uq"
        for (size_t i = 1; i < s.size(); i++)
            emit((char)toupper(s[i]));
    }

    void emitInstOpts(const Instruction &i) {
        static const InstOpt ALL_INST_OPTS[] {
            InstOpt::ACCWREN,
            InstOpt::ATOMIC,
            InstOpt::BREAKPOINT,
            InstOpt::COMPACTED,
            InstOpt::EOT,
            InstOpt::NOCOMPACT,
            InstOpt::NODDCHK,
            InstOpt::NODDCLR,
            InstOpt::NOPREEMPT,
            InstOpt::NOSRCDEPSET,
            InstOpt::SWITCH,
            InstOpt::SERIALIZE,
        };

        InstOptSet ios = i.getInstOpts();
        emit(", opts:[");
        bool first = true;
        auto emitSeparator = [&]() {
            if (first)
                first = false;
            else
                emit(",");
        };
        for (size_t i = 0;
            i < sizeof(ALL_INST_OPTS)/sizeof(ALL_INST_OPTS[0]) && !ios.empty();
            i++)
        {
            if (ios.contains(ALL_INST_OPTS[i])) {
                emitSeparator();
                ios.remove(ALL_INST_OPTS[i]);
                emit("\"", ToSyntax(ALL_INST_OPTS[i]), "\"");
            }
        }

        const auto &di = i.getSWSB();
        if (di.hasDist() || di.hasToken()) {
            auto emitPipeDist = [&] (const char *pfx) {
                emitSeparator();
                emit("\"", pfx, "@", (int)di.minDist, "\"");
            };
            switch (di.distType) {
            case SWSB::DistType::REG_DIST:        emitPipeDist(""); break;
            default:
                break;
            }

            auto emitSwsbEvent = [&] (const char *sfx) {
                emitSeparator();
                emit("\"$", (int)di.sbid, sfx, "\"");
            };
            switch (di.tokenType) {
            case SWSB::TokenType::DST:  emitSwsbEvent(".dst"); break;
            case SWSB::TokenType::SRC:  emitSwsbEvent(".src"); break;
            case SWSB::TokenType::SET:  emitSwsbEvent(""); break;
            default:
                break;
            }
        }
        emit("]");
    }

    void emitReg(RegName rn, RegRef rr) {
        bool hasShortName =
            rn == RegName::GRF_R ||
            rn == RegName::ARF_A || rn == RegName::ARF_ACC ||
            rn == RegName::ARF_NULL;
        if (rr.subRegNum == 0 && hasShortName) {
            // IR.Regs.r13 is a direct reference
            const RegInfo *ri = model.lookupRegInfoByRegName(rn);
            if (ri == nullptr) {
                emitIrError("unsupported RegName");
            } else if (ri->hasRegNum()) {
                emit("IR.Regs.", ToSyntax(rn), rr.regNum);
            } else if (rn == RegName::ARF_NULL) {
                emit("IR.Regs.null_0"); // since "null" is a keyword
            } else {
                emit("IR.Regs.", ToSyntax(rn));
            }
        } else if (rn == RegName::ARF_F) {
            emit("IR.Regs.f", rr.regNum, "_", rr.subRegNum);
        } else {
            // IR.Regs.reg("r",13,4) creates a new object to r13.4
            emit("IR.Regs.reg(\"", ToSyntax(rn), "\",", rr.regNum, ",",
                rr.subRegNum, ")");
        }
    }
    void emitReg(RegName rn, int regNum, int sr = 0) {
        emitReg(rn, RegRef((int16_t)regNum, (int16_t)sr));
    }

    /////////////////////////////////////////////////////////////////////////
    // call this for operands that are instruction outputs
    void emitDepOutputs(const Instruction &i, const RegSet &rs) {
        // 'i' is writing 'rs' emit all pairs where this is the
        // def and report them as uess
        emit("uses:[");
        if (opts.liveAnalysis) {
            emit("]");
            return;
        }
        // the users of 'rs' are all those that use this set
        bool first = true;
        for (const Dep *d : depDefs[i.getID()]) {
            if (d->use != nullptr && rs.intersects(d->values)) {
                if (first)
                    first = false;
                else
                    emit(",");
                emit(d->use->getID());
            }
        }
        emit("]");
    }

    // call this for operands that are instruction inputs
    void emitDepInputs(const Instruction &i, const RegSet &rs) {
        // 'i' is writing 'rs' emit all pairs where this is the
        // def and report them as uess
        emit("defs:[");
        if (opts.liveAnalysis == nullptr) {
            emit("]");
            return;
        }
        // the users of 'rs' are all those that use this set
        bool first = true;
        for (const Dep *d : depUses[i.getID()]) {
            if (d->def != nullptr && rs.intersects(d->values)) {
                if (first)
                    first = false;
                else
                    emit(",");
                emit(d->def->getID());
            }
        }
        emit("]");
    }
};

void iga::FormatJSON(
    std::ostream &o,
    const FormatOpts &opts,
    const Kernel &k,
    const void *bits)
{
    JSONFormatter(o, opts, bits).emitKernel(k);
}