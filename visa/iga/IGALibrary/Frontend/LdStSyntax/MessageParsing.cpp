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
#include "MessageParsing.hpp"
#include "Syntax.hpp"
#include "Tables.hpp"
#include "../../bits.hpp"

#include <cstring>

using namespace iga;


struct RegRange {
    RegName regName  = RegName::INVALID;
    int     regStart = 0;
    int     length   = 0;
};

// e.g. r13
// e.g. r13:f
// e.g. r13-14
static RegRange parseRegRange(GenParser &p)
{
    auto regStr = p.ConsumeIdentOrFail("register range");
    RegRange rr;
    const RegInfo *ri = nullptr;
    if (!p.LookupReg(regStr, ri, rr.regStart)) {
        p.Fail("invalid register range");
    }
    rr.regName = ri->regName;

    if (!p.Consume(SUB)) {
        rr.length = 1;
    } else {
        int highReg = 0;
        auto loc = p.NextLoc();
        if (!p.ConsumeIntLit(highReg)) {
            p.Fail(loc, "malformed register range");
        }
        if (highReg < rr.regStart) {
            p.Fail(loc, "malformed register range (high register must be >= low)");
        }
        rr.length = highReg - rr.regStart + 1;
    }
    return rr;
}

static MAddrModel parseAddressModel(GenParser &p)
{
    auto loc = p.NextLoc();
    auto addrModelSymbol = p.ConsumeIdentOrFail("address model");
    static const MAddrModel NON_BTI_MODELS[] = {
        MAddrModel::SLM,
        MAddrModel::A32_NC, MAddrModel::A32_CO, MAddrModel::A32_SO,
        MAddrModel::A64_NC, MAddrModel::A64_CO, MAddrModel::A64_SO,
    };
    // non bti or scratch surfaces
    for (MAddrModel am : NON_BTI_MODELS) {
        if (addrModelSymbol == SymbolFor(am)) {
            return am;
        }
    }
    // regular bti surface
    MAddrModel addrModel = MAddrModel::INVALID_ADDR_MODEL;
    if (addrModelSymbol == SymbolFor(MAddrModel::BTI_0) ||
        addrModelSymbol == SymbolFor(MAddrModel::SCRATCH_FAMILY))
    {
        // [0x4]...
        p.ConsumeOrFail(LBRACK,"expected [");
        auto surfIndexLoc = p.NextLoc();
        ImmVal val;
        if (!p.TryParseIntConstExpr(val, "surface index")) {
            p.Fail("expected surface index");
        }
        if (addrModelSymbol == SymbolFor(MAddrModel::BTI_0)) {
            if (val.s64 >= 0xEF || val.s64 < 0) {
                p.Fail(surfIndexLoc, "surface index is out of bounds");
            }
            addrModel = static_cast<MAddrModel>(MAddrModel::BTI_0 | (int)val.s64);
        } else if (addrModelSymbol == SymbolFor(MAddrModel::SCRATCH_FAMILY)) {
            if (val.s64 > 0xFFF || val.s64 < 0) {
                p.Fail(surfIndexLoc, "scratch index is out of bounds");
            }
            addrModel = static_cast<MAddrModel>(MAddrModel::SCRATCH_FAMILY | (int)val.s64);
        }
        p.ConsumeOrFail(RBRACK,"expected ]");
    } else {
        p.Fail(loc, "invalid address model");
    }
    return addrModel;
}

// EXAMPLES:
//   AddrModel [r13]
//   AddrModel [r13 + 0x100]
//   AddrModel [r13-14]
//   AddrModel [r13-14]:f
//   AddrModel [r13-14,r17]
//   AddrModel [r13,r14 + 0x100]
struct AddrOperand {
    Loc         addrModelLoc;
    MAddrModel  addrModel = INVALID_ADDR_MODEL;

    RegRange    addrReg0;
    Loc         addrReg0Loc;

    RegRange    addrReg1;
    Loc         addrReg1Loc;

    int32_t     addrOff = 0;
    Loc         addrOffLoc;

    Type        addrType = Type::INVALID;
};

struct DataOperand {
    Loc         loc;
    RegRange    reg;
    Type        type = Type::INVALID;
};

struct LdStSyntax {
    AddrOperand   addr;
    DataOperand   data;
};

static void parseAddrOperand(
    GenParser &p,
    AddrOperand &a,
    bool unary)
{
    // EXAMPLES:
    //   AddrModel [r13]
    //   AddrModel [r13 + 0x100]
    //   AddrModel [r13-14]
    //   AddrModel [r13-14]:f
    //   AddrModel [r13-14,r17]
    //   AddrModel [r13,r14 + 0x100]
    a.addrModelLoc = p.NextLoc();
    a.addrModel = parseAddressModel(p);
    // e.g.  ...[r20-23]:f
    // e.g.  ...[r20,r21-22]:f
    p.ConsumeOrFail(LBRACK, "expected [");
    a.addrReg0Loc = p.NextLoc();
    a.addrReg0 = parseRegRange(p);
    if (p.Consume(COMMA)) {
        // e.g.  ...[r20,r21-22]:f
        //               ^^^^^^
        a.addrReg1Loc = p.NextLoc();
        if (unary) {
            p.Fail("unary send prohibits src1 argument");
        }
        a.addrReg1 = parseRegRange(p);
    } else {
        a.addrReg1Loc = Loc::INVALID;
        a.addrReg1.regName = RegName::ARF_NULL;
        a.addrReg1.regStart = 0;
        a.addrReg1.length = 0;
    }
    // offset
    // + 0x440]
    // ^^^^^^^
    if (p.LookingAt(Lexeme::ADD) || p.LookingAt(Lexeme::SUB)) {
        bool negate = p.LookingAt(Lexeme::SUB);
        p.Skip();

        auto immExprLoc = p.NextLoc();
        ImmVal offVal;
        if (!p.TryParseIntConstExpr(offVal, "surface offset")) {
            p.Fail(immExprLoc, "expected offset");
        }
        const int64_t MAX_OFF = 0xFFFF;
        if (offVal.s64 < 0 || offVal.s64 > MAX_OFF) {
            std::stringstream ss;
            ss << "address offset out of bounds for message type (>" << MAX_OFF << ")";
            p.Fail(immExprLoc, ss.str().c_str());
        }
        a.addrOff = (int32_t)offVal.s64;
        if (negate) {
            a.addrOff *= -1;
        }
    }
    p.ConsumeOrFail(RBRACK, "expected ]");
    a.addrType = p.ParseSendOperandTypeWithDefault(0);
}

static void parseDataOperand(
    GenParser &p,
    DataOperand &op)
{
    // EXAMPLES:
    //   r13
    //   r13-14
    //   r13-14:f
    op.loc = p.NextLoc();
    op.reg = parseRegRange(p);
    op.type = p.ParseSendOperandTypeWithDefault(0);
}

static void parseLdInstOperands(
    GenParser &p,
    LdStSyntax &opnds,
    bool unary)
{

    //   ld... (..)  r12-14 ...
    //   ld... (..)  r10:f  ...
    parseDataOperand(p, opnds.data);

    // Sources:
    //   ld... (..)  ... a64[r20]:f
    //   ld... (..)  ... surf[0x4][r20-23]:f
    //   ld... (..)  ... surf[0x4][r10,r20-23]:f
    //   ld... (..)  ... surf[2*4][r10,r20-23]:f
    parseAddrOperand(p, opnds.addr, unary);
}

static void parseStInstOperands(
    GenParser &p,
    LdStSyntax &opnds,
    bool unary)
{
    // Destination:
    //   st... (..)  a32c[r12-14]          ...
    //   st... (..)  surf[3][r10]:f        ...
    //   st... (..)  surf[3][r10+0x100]:f  ...
    parseAddrOperand(p, opnds.addr, unary);

    //   st... (..)  ....  r13
    //   st... (..)  ....  r13-14
    //   st... (..)  ....  r13-14:f
    parseDataOperand(p, opnds.data);
}

// parses a regular instruction option or tries for a Ld/St option
//   (if one exists that fits)
static void parseInstOptOrLdStOpt(
    GenParser &p,
    const MFormat &mf,
    InstOptSet &instOpts,
    uint32_t &desc)
{
    auto tk = p.Next();
    if (!p.TryParseInstOptOrDepInfo(instOpts)) {
        std::string ldStOpt = p.ConsumeIdentOrFail("instruction option");
        bool foundOption = false;

        // run through the format fields looking for an instruction option
        // that matches
        for (const MField &f : mf.fields) {
            if (f.type == nullptr) {
                break; // end of the field array
            }
            if (f.type->mapping == MFieldMapping::MFieldMappingInstOpt &&
                ldStOpt == f.type->instOptSymbol)
            {
                // found the instruction option attribute this fills
                setBits<uint32_t>(desc, f.off, 1, 1);
                foundOption = true;
                break;
            }
        }
        if (!foundOption) {
            p.Fail(tk.loc, "invalid instruction option");
        }
    }
}

static void parseInstOpts(
    GenParser &p,
    const MFormat &mf,
    uint32_t &desc)
{
    InstOptSet instOpts;
    instOpts.clear();

    if (p.Consume(LBRACE)) {
        if (!p.LookingAt(RBRACE))
            parseInstOptOrLdStOpt(p, mf, instOpts, desc); // else could be an empty list "{}"
        while (p.Consume(COMMA)) {
            parseInstOptOrLdStOpt(p, mf, instOpts, desc);
        }
        p.ConsumeOrFail(RBRACE,"expected }");
    }

    p.m_handler.InstOpts(instOpts);
}


static const OpSpec *lookupOpSpec(
    const Model &model,
    bool cond,
    bool unry,
    SFID sfid,
    uint32_t &exDesc)
{
    Op sendOp;
    bool sendIsBinary = !model.supportsUnarySend();
    if (cond) {
        if (unry) {
            sendOp = Op::SENDC;
        } else {
            sendOp = sendIsBinary ? Op::SENDC : Op::SENDSC;
        }
    } else {
        if (unry) {
            sendOp = Op::SEND;
        } else {
            sendOp = sendIsBinary ? Op::SEND : Op::SENDS;
        }
    }

    auto os = &model.lookupOpSpec(sendOp);
    if (os->format == OpSpec::GROUP) {
        // SFID is a subop
        os = &model.lookupGroupSubOp(sendOp, (uint32_t)sfid);
    } else {
        // SFID is part of ExDesc
        exDesc |= (uint32_t)sfid;
    }
    return os;
}

static void encodeDescriptors(
    GenParser &p,
    Loc opLoc,
    const MFormat *format,
    const std::vector<std::pair<Loc,std::string>> &args,
    const LdStSyntax &opnds,
    ExecSize execSize,
    bool conditional,
    bool unary)
{
    const Model &model = p.m_model;
    auto &handler = p.m_handler;

    uint32_t desc = format->opcodeValue; // starts with hardcoded things (e.g. MT)
    uint32_t exDesc = 0;

    // FIXME: this is sloppy, we should do this in the caller function
    parseInstOpts(p, *format, desc);

    const OpSpec *opSpec =
        lookupOpSpec(
            model,
            conditional,
            unary,
            format->sfid,
            exDesc);
    p.m_opSpec = opSpec;
    handler.InstOp(opSpec);

    // fetch the default regions for dst and src0
    Region::Horz dstHz = Region::Horz::HZ_1;
    if (opSpec->hasImplicitDstRegion()) {
        dstHz = opSpec->implicitDstRegion().getHz();
    }
    Region s0rgn = Region::INVALID;
    if (opSpec->hasImplicitSrcRegion(0, model.platform, execSize)) {
        s0rgn = opSpec->implicitSrcRegion(0, model.platform, execSize);
    }
    Type s0Ty = Type::INVALID;
    if (opSpec->hasImplicitSrcType(0,false,model.platform)) {
        s0Ty = opSpec->implicitSrcType(0, false, model.platform);
    }

    int actualNumAddrRegs = 0;
    Loc addrRegLoc;
    int actualNumDataRegs = 0;
    Loc dataRegLoc;
    if (format->kind == MKind::LOAD) {
        // ld...    dst:dt  [src0]:st
        //    <==>
        //   send...  dst:dt  src0:st  null
        //
        //
        // set rlen (the "dst")
        int rlen = opnds.data.reg.length;
        int mlen = opnds.addr.addrReg0.length;
        int xlen = opnds.addr.addrReg1.length;
        if (unary) {
            // we pack this into one whole send into contiguous registers
            // i.e. it should encode as GEN9-era
            //    send (...)  dst    (addrs1:addrs2)
            xlen = 0;
            mlen += xlen;
        }
        setBits<uint32_t>(desc, 20, 5, (uint32_t)rlen);
        setBits<uint32_t>(desc, 25, 4, (uint32_t)mlen);
        setBits<uint32_t>(exDesc, 6, 4, (uint32_t)xlen);

        addrRegLoc = opnds.addr.addrReg0Loc;
        actualNumAddrRegs =
            opnds.addr.addrReg0.length + opnds.addr.addrReg1.length;
        dataRegLoc = opnds.data.loc;
        actualNumDataRegs = opnds.data.reg.length;

        handler.InstDstOpRegDirect(
            opnds.data.loc,
            opnds.data.reg.regName,
            (uint8_t)opnds.data.reg.regStart,
            dstHz,
            opnds.data.type);
        handler.InstSrcOpRegDirect(
            0,
            opnds.addr.addrReg0Loc,
            opnds.addr.addrReg0.regName,
            opnds.addr.addrReg0.regStart,
            s0rgn,
            opnds.addr.addrType);
        // ld...    dst:dt  [src0,src1]:st
        //    <==>
        //   send...  dst:dt  src0:st  src1
        //
        if (!unary) {
            // FIXME: is this right???
            handler.InstSrcOpRegDirect(
                1,
                opnds.addr.addrReg1Loc,
                opnds.addr.addrReg1.regName,
                opnds.addr.addrReg1.regStart,
                Region::INVALID,
                Type::INVALID);
        }
    } else {
        // st...    [src0]:st   src1
        //    <==>
        //   send...  null  src0:st  src1
        //
        int rlen = 0;
        int mlen = opnds.addr.addrReg0.length;
        int xlen = opnds.data.reg.length;
        if (unary) {
            // st...    [src0]:st   src1
            //    <==>
            //   send...  null  (src0:src1):st
            mlen += xlen;
            xlen = 0;
            p.FailF(opLoc, "unary store (stp) not supported yet");
        }
        setBits<uint32_t>(desc, 20, 5, (uint32_t)rlen);
        setBits<uint32_t>(desc, 25, 4, (uint32_t)mlen);
        setBits<uint32_t>(exDesc, 6, 4, (uint32_t)xlen);

        addrRegLoc = opnds.addr.addrReg0Loc;
        actualNumAddrRegs =
            opnds.addr.addrReg0.length + opnds.addr.addrReg1.length;
        dataRegLoc = opnds.data.loc;
        actualNumDataRegs = opnds.data.reg.length;

        // st...    [src0]   src1:dt
        //    <==>
        //   send...  null:dt  src0  src1
        //
        // We preserve the destination type since source
        // types don't exist.
        handler.InstDstOpRegDirect(
            opLoc,
            RegName::ARF_NULL,
            0,
            dstHz,
            opnds.data.type);
        handler.InstSrcOpRegDirect(
            0,
            opnds.addr.addrReg0Loc,
            opnds.addr.addrReg0.regName,
            opnds.addr.addrReg0.regStart,
            s0rgn,
            s0Ty);
        handler.InstSrcOpRegDirect(
            1,
            opnds.data.loc,
            opnds.data.reg.regName,
            opnds.data.reg.regStart,
            Region::INVALID,
            Type::INVALID);
    }

    // encode the arguments
    int argIx = 0;
    for (auto ix : format->argIndices) {
        if (ix == -1) {
            break; // end of the arguments
        }
        const MField &f = format->fields[ix];
        if ((size_t)argIx >= args.size()) {
            p.FailF(opLoc, "missing %s argument", f.type->name);
        }
        auto arg = args[argIx++];
        uint32_t val;
        if (!f.type->subopParse(arg.second,val)) {
            p.FailF(arg.first, "invalid %s", f.type->name);
        }
        setBits<uint32_t>(desc, f.off, f.type->length, val);
    }

    for (const MField &f : format->fields) {
        if (f.type == nullptr) {
            break; // end of the field array
        }
        if (f.type->mapping == MFieldMapping::MFieldMappingInstOpt) {
            // we handled this above
        } else if (f.type->mapping == MFieldMapping::MFieldMappingSubOp) {
            // we handled this above
        } else if (f.type->mapping == MFieldMapping::MFieldMappingHeader) {
            // e.g. the header
        } else if (f.type->mapping == MFieldMapping::MFieldMappingExecSize) {
            uint32_t val;
            f.type->execSizeEncode(execSize, val);
            setBits<uint32_t>(desc, f.off, f.type->length, val);
        } else if (f.type->mapping == MFieldMapping::MFieldMappingAddrModel) {
            // field length can differ for BTI and scratch, so we must
            // compute the mask here instead of hardcoding 0xFF
            uint32_t mask = getFieldMaskUnshifted<uint32_t>(f.type->length);
            setBits<uint32_t>(
                desc,
                f.off,
                f.type->length,
                (opnds.addr.addrModel & mask));
        } else {
            IGA_ASSERT_FALSE("unhandled field type in send message assembly");
        }
    }
    int expectedNumAddrRegs = 0;
    if (!ComputeMessageAddressRegisters(
        format,
        (int)execSize,
        desc,
        expectedNumAddrRegs))
    {
        p.Fail(addrRegLoc,
            "unable to compute number of address registers for message");
    }
    if (expectedNumAddrRegs + 1 == actualNumAddrRegs) {
        // one of the registers must be the header
        const MField *headerField = FindHeaderField(*format);
        if (headerField == nullptr) {
            p.Fail(addrRegLoc,
                "address range includes header (extra reg), but format "
                "does not allow for it");
        }
        setBits<uint32_t>(desc, 19, 1, 1);
    } else if (expectedNumAddrRegs == actualNumAddrRegs) {
        setBits<uint32_t>(desc, 19, 1, 0);
    } else {
        // something is fishy...
        p.WarningF(addrRegLoc,
            "expected %d address registers",
            expectedNumAddrRegs);
    }

    int expectedNumDataRegs = 0;
    if (ComputeMessageDataRegisters(
        format,
        (int)execSize,
        desc,
        expectedNumDataRegs))
    {
        if (actualNumDataRegs != expectedNumDataRegs) {
            p.WarningF(dataRegLoc,
                "expected %d data registers",
                expectedNumDataRegs);
        }
    }

    // set the extended offset size
    setBits<uint32_t>(exDesc, 16, 16, opnds.addr.addrOff);

    // set the descriptors!
    SendDescArg exDescArg;
    exDescArg.type = SendDescArg::IMM;
    exDescArg.imm = exDesc;
    SendDescArg descArg;
    descArg.type = SendDescArg::IMM;
    descArg.imm = desc;
    handler.InstSendDescs(
        opLoc, exDescArg,
        opLoc, descArg);
}


bool iga::ParseLdStInst(ExecSize dftExecSize, GenParser &p)
{
    // (f0.0)  ld.sc8.x4 (8) ...
    //         ^
    if (!p.LookingAt(Lexeme::IDENT)) {
        return false; // fall back to "invalid mnemonic"
    }

    auto ldStLoc = p.NextLoc();
    auto ldSt = p.ConsumeIdentOrFail();
    auto ldStPfx = ldSt.substr(0,2);
    MKind kind;
    if (ldStPfx == "ld") {
        kind = MKind::LOAD;
    } else if (ldStPfx == "st") {
        kind = MKind::STORE;
    } else if (ldStPfx == "at") {
        kind = MKind::ATOMIC;
    } else {
        // fall back to "invalid mnemonic"
        return false;
    }

    bool conditional = false;
    bool unary = false;
    auto ldStfx = ldSt.substr(2);
    if (ldStfx == "") {
        conditional = false;
        unary = false;
    } else if (ldStfx == "c") {
        conditional = true;
        unary = false;
    } else if (ldStfx == "p") {
        conditional = false;
        unary = true;
    } else if (ldStfx == "cp") {
        conditional = true;
        unary = true;
    } else {
        p.Fail(ldStLoc, "malformed load/store mnemonic suffix");
    }
    if (unary && !p.m_model.supportsUnarySend()) {
        p.Fail(ldStLoc, "platform does not support packed (unary) send");
    }

    // (f0.0)  ld.sc8.x4 (8) ...
    //           ^
    p.ConsumeOrFail(Lexeme::DOT, "expected .");
    auto opLoc = p.NextLoc();
    auto op = p.ConsumeIdentOrFail("ld/st operation name");


    // (f0.0)  ld.sc8.x4 (8) ...
    //               ^^^
    std::vector<std::pair<Loc,std::string>> opArgs;
    while (p.Consume(Lexeme::DOT)) {
        auto loc = p.NextLoc();
        auto arg = p.ConsumeIdentOrFail();
        opArgs.emplace_back(loc, arg);
    }

    ExecSize execSize;
    ChannelOffset chOff;
    p.ParseExecInfo(dftExecSize, execSize, chOff);

    LdStSyntax opnds;
    memset(&opnds, 0, sizeof(opnds));
    opnds.addr.addrReg0.regName = RegName::ARF_NULL;
    opnds.addr.addrReg1.regName = RegName::ARF_NULL;
    opnds.data.reg.regName = RegName::ARF_NULL;
    if (kind == MKind::LOAD) {
        parseLdInstOperands(p, opnds, unary);
    } else if (kind == MKind::STORE) {
        parseStInstOperands(p, opnds, unary);
    } else {
        p.Fail("unhandled ld/st operation kind");
    }

    const MFormat *onlyMissesAddrModel = nullptr;

    /////////////////////////////////////////////////
    // resolve message and encode descriptors
    auto matches = [&] (const MFormat &mf) {
        if (mf.platform > p.m_model.platform) {
            return false; // this message is too new (e.g. GEN10 message on 9)
        }
        if (mf.kind != kind) {
            return false; // e.g. this is a "st", we wanted a "ld"
        }
        if (strcmp(mf.mnemonic,op.c_str()) != 0) {
            return false; // mnemonic mismatch
        }
        onlyMissesAddrModel = &mf;
        return ((opnds.addr.addrModel & mf.addrModel) != 0);
    };
    size_t mLen;
    const MFormat *mArr = GetMFormatTable(mLen);
    const MFormat *format = nullptr;
    for (size_t mIx = 0; mIx < mLen; mIx++) {
        if (matches(mArr[mIx])) {
            if (format == nullptr || mArr[mIx].platform >= format->platform) {
                // first match OR newer platform
                // note: matches(..) enforces (>=minPlatform)
                if (format != nullptr) {
                    // debug check for multiple matches based on op only
                    IGA_ASSERT(mArr[mIx].platform > format->platform,
                        "syntax matches multiple messages");
                }
                format = mArr + mIx;
            }
            // need this case, can prefer a newer platform
            // else if (format != nullptr) {
            //    IGA_ASSERT_FALSE("syntax matches multiple formats");
            // }
        }
    } // end table loop

    if (format == nullptr) {
        if (onlyMissesAddrModel) {
            p.Fail(opLoc, "invalid address model for this message");
        } else {
            p.Fail(opLoc, "invalid message");
        }
        return true; // unreachable
    }

    // encode the instruction
    encodeDescriptors(
        p, opLoc, format, opArgs, opnds, execSize, conditional, unary);

    return true;
}
