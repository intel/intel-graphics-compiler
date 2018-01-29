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
#include "Interface.hpp"
#include "LoadCommon.hpp"
#include "LoadScattered.hpp"
#include "LoadBlock.hpp"
#include "../IRToString.hpp"

using namespace iga;

// Descriptor Layout is
//  [31:29]      reserved
//  [28:25]      mlen
//  [24:20]      rlen
//  [19]         header present (sampler can use too)
//  [18][17:14]  message type (18 gets used for scratch for example)



bool DecodeMessagePattern::matches(
    const Model &model, uint32_t exDesc, uint32_t desc) const
{
    if (model.platform < platform) {
        return false;
    }
    uint64_t sfidBits = getBits(exDesc, 0, 4);
    if (sfidBits != static_cast<int>(sfid)) {
        return false;
    }
    uint64_t desc64 = ((uint64_t)exDesc << 32) | desc;
    for (const Subpattern &sp : subpatterns) {
        if (!sp.valid()) {
            break;
        }
        auto val = (uint32_t)getBits(desc64, sp.fieldStart, sp.fieldLength);
        if (val != sp.fieldValueEq) {
            return false;
        }
    }
    return true;
}

void DecodeMessagePattern::decode(
    const Model &model,
    const Instruction &i,
    uint32_t exDesc,
    uint32_t desc,
    Message &msg,
    std::string &error) const
{
    DecodeMessageInputs inps{model, i, exDesc, desc, *this};
    msg.family = family;
    msg.mode = mode;

    if (inps.hasAttribute(MessageAttr::HAS_IAR_13)) {
        msg.invalidateOnRead = testBit(inps.desc, 19);
    } else {
        msg.invalidateOnRead = false;
    }
    if (inps.hasAttribute(MessageAttr::SUPPORTS_HEADER)) {
        msg.invalidateOnRead = testBit(inps.desc, 13);
    } else {
        msg.invalidateOnRead = false;
    }
    const OpSpec &os = i.getOpSpec();
    msg.mnemonic = mnemonic;
    // TODO: should check these with the logical message sizes
    // see if they are correct
    msg.dstLen = getBits(desc, 20, 5);
    msg.src0Len = getBits(desc, 25, 4);
    msg.src1Len = getBits(exDesc, 6, 4);
    msg.isSplit =
        os.op == Op::SENDS || os.op == Op::SENDSC ||
        os.groupOp == Op::SEND || os.groupOp == Op::SENDC; 
    msg.isConditional =
        os.op == Op::SENDC ||
        os.op == Op::SENDSC ||
        os.groupOp == Op::SENDC;
    if (!decodeFunc(inps, msg, error)) {
        memset(&msg, 0, sizeof(msg));
        msg.family = Message::INVALID;
    }
}

bool DecodeMessageInputs::hasAttribute(MessageAttr attr) const {
    return (message.attrs & static_cast<int>(attr)) != 0;
}



#define SUBPAT(FOFF,FLEN,FVS)               {(FOFF),(FLEN),(FVS)}
#define MESSAGE_TYPE_14_18(MT)              SUBPAT(14,5,(MT)) // bits[18:14]
#define MESSAGE_SUBTYPE_9_8(ST)             SUBPAT(8,2,(ST)) // bits[9:8]
#define BLOCK_MESSAGE_SUBTYPE_13(ST)        SUBPAT(13,1,(ST)) // bits[13]
#define A64_BLOCK_MESSAGE_SUBTYPE_11_12(ST) SUBPAT(11,2,(ST)) // bits[12:11]
#define RESERVED_IS(OFF,LEN,VAL)            SUBPAT(OFF,LEN,VAL)
#define RESERVED(OFF,LEN)                   RESERVED_IS(OFF,LEN,0)
#define BTI(V)                              SUBPAT(0,8,(V)) // bits[7:0]

// TODO: split this into sub arrays and distribute to various modules
static const DecodeMessagePattern LOAD_PATTERNS[] = {
    ///////////////////////////////////////////////////////////////////////////
    //
    // Scattered Reads
    //
    ///////////////////////////////////////////////////////////////////////////
    //
    // (pred)  ld.sca##.<ElemsPerChan>.<AddrModel> (ExecSize(/<SIMDMode>)?|ChOff) LdDst LdSrc InstOpts
    //         ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ just this part
    //
    {
        "A64 B scattered read", Platform::GEN9,
        Message::Family::SCATTERED, Message::Mode::READ,
        SFID::DC1, {RESERVED(29,3),MESSAGE_TYPE_14_18(0x10), MESSAGE_SUBTYPE_9_8(0)},
        "sct8", DecodeMessageScattered<AddrModel::A64_FAMILY,ScatteredMessage::DataType::BYTE,12>,
        MessageAttr::HAS_BTI_BITS | MessageAttr::HAS_IAR_13,
    },
    {
        "A64 DW scattered read", Platform::GEN9,
        Message::Family::SCATTERED, Message::Mode::READ,
        SFID::DC1, {RESERVED(29,3),MESSAGE_TYPE_14_18(0x10), MESSAGE_SUBTYPE_9_8(1)},
        "sct32", DecodeMessageScattered<AddrModel::A64_FAMILY,ScatteredMessage::DataType::DWORD,12>,
        MessageAttr::HAS_BTI_BITS | MessageAttr::HAS_IAR_13,
    },
    {
        "A64 QW scattered read", Platform::GEN9,
        Message::Family::SCATTERED, Message::Mode::READ,
        SFID::DC1, {RESERVED(29,3),MESSAGE_TYPE_14_18(0x10), MESSAGE_SUBTYPE_9_8(2)},
        "sct64", DecodeMessageScattered<AddrModel::A64_FAMILY,ScatteredMessage::DataType::QWORD,12>,
        MessageAttr::HAS_BTI_BITS | MessageAttr::HAS_IAR_13,
    },
    {
        "BTI/SLM/A32 B scattered read", Platform::GEN9,
        Message::Family::SCATTERED, Message::Mode::READ,
        SFID::DC0, {RESERVED(29,3), MESSAGE_TYPE_14_18(0x4), RESERVED(12,2), RESERVED(9,1)},
        "sct8", DecodeMessageScattered<AddrModel::A32_FAMILY,ScatteredMessage::DataType::BYTE,8>,
        MessageAttr::HAS_PACKING | MessageAttr::SUPPORTS_HEADER| MessageAttr::HAS_BTI_BITS,
    },
    {
        "BTI/SLM/A32 DW scattered read", Platform::GEN9,
        Message::Family::SCATTERED, Message::Mode::READ,
        SFID::DC0, {RESERVED(29,3),MESSAGE_TYPE_14_18(0x3), RESERVED_IS(9,1,1)},
        "sct32", DecodeMessageScattered<AddrModel::A32_FAMILY,ScatteredMessage::DataType::DWORD,8>,
        MessageAttr::SUPPORTS_HEADER | MessageAttr::HAS_BTI_BITS,
    },
    {
        "BTI/A32 CC DW scattered read", Platform::GEN9,
        Message::Family::SCATTERED, Message::Mode::READ,
        SFID::DCRO, {RESERVED(29,3),MESSAGE_TYPE_14_18(0x3)},
        "csct32", DecodeMessageScattered<AddrModel::A32_FAMILY,ScatteredMessage::DataType::DWORD,8>,
        MessageAttr::SUPPORTS_HEADER | MessageAttr::HAS_BTI_BITS | MessageAttr::IS_CONST,
    },

    ///////////////////////////////////////////////////////////////////////////
    //
    // Block Reads
    //
    ///////////////////////////////////////////////////////////////////////////
    //
    {
        "A64 Hword Block Read MSD", Platform::GEN9,
        Message::Family::BLOCK, Message::Mode::READ,
        SFID::DC1,
        {MESSAGE_TYPE_14_18(0x14),A64_BLOCK_MESSAGE_SUBTYPE_11_12(0x3)},
        "blk256", DecodeMessageBlock<AddrModel::A32_FAMILY,BlockMessage::SubType::DEFAULT,BlockMessage::Size::B256>,
        MessageAttr::SUPPORTS_IAR | MessageAttr::HAS_BTI_BITS,
    },
    {
        "A64 Oword Block Read MSD", Platform::GEN9,
        Message::Family::BLOCK, Message::Mode::READ,
        SFID::DC1,
        {MESSAGE_TYPE_14_18(0x14), A64_BLOCK_MESSAGE_SUBTYPE_11_12(0x0)},
        "blk128", DecodeMessageBlock<AddrModel::A32_FAMILY,BlockMessage::SubType::DEFAULT>,
        MessageAttr::HAS_BTI_BITS,
    },
    {
        "Oword Block Read MSD", Platform::GEN9,
        Message::Family::BLOCK, Message::Mode::READ,
        SFID::DC0,
        {MESSAGE_TYPE_14_18(0x0), BLOCK_MESSAGE_SUBTYPE_13(0x0)},
        "blk128", DecodeMessageBlock<AddrModel::A32_FAMILY,BlockMessage::SubType::DEFAULT>,
        MessageAttr::HAS_BTI_BITS | MessageAttr::HAS_IAR_13,
    },
    {
        "Oword Block Read MSD", Platform::GEN9,
        Message::Family::BLOCK, Message::Mode::READ,
        SFID::DC0,
        {MESSAGE_TYPE_14_18(0x0), BLOCK_MESSAGE_SUBTYPE_13(0x0),RESERVED_IS(9,1,0)},
        "blk128", DecodeMessageBlock<AddrModel::A64_FAMILY,BlockMessage::SubType::DEFAULT>,
        MessageAttr::HAS_BTI_BITS,
    },
    {
        "Oword Aligned Block Read MSD", Platform::GEN9,
        Message::Family::BLOCK, Message::Mode::READ,
        SFID::DC0,
        {MESSAGE_TYPE_14_18(0x1), BLOCK_MESSAGE_SUBTYPE_13(0x0)},
        "ablk128", DecodeMessageBlock<AddrModel::A32_FAMILY,BlockMessage::SubType::ALIGNED>,
        MessageAttr::HAS_BTI_BITS
    },
    {
        "Constant Cache Oword Block Read MSD", Platform::GEN9,
        Message::Family::BLOCK, Message::Mode::READ,
        SFID::DCRO,
        {MESSAGE_TYPE_14_18(0x0)},
        "cblk128", DecodeMessageBlock<AddrModel::A32_FAMILY,BlockMessage::SubType::ALIGNED>,
        MessageAttr::HAS_BTI_BITS | MessageAttr::IS_CONST
    },

    };



Message iga::TranslateSendToMessage(
    const Model &model,
    const Instruction &inst,
    std::string &error)
{
    Message msg;
    memset(&msg, 0, sizeof(msg));
    msg.family = Message::INVALID;

    const OpSpec &os = inst.getOpSpec();
    if (!os.isSendOrSendsFamily() ||
        os.op == Op::SENDC ||
        os.groupOp == Op::SENDC)
    {
        return msg;
    }

    auto extDescT = inst.getExtMsgDescriptor();
    auto descT = inst.getMsgDescriptor();
    if (extDescT.type == SendDescArg::REG32A ||
        descT.type == SendDescArg::REG32A)
    {
        return msg;
    }
    uint32_t exDesc = extDescT.imm;
    uint32_t desc = descT.imm;

    bool isGroupSend = os.groupOp == Op::SEND || os.groupOp == Op::SENDC;
    if (isGroupSend) {
        exDesc |= (uint32_t)os.functionControlValue;
    }

    // match the highest platform
    const DecodeMessagePattern *pDmp = nullptr;
    for (const DecodeMessagePattern &dmp : LOAD_PATTERNS) {
        if (dmp.matches(model, exDesc, desc)) {
            if (pDmp == nullptr || pDmp->platform < dmp.platform) {
                pDmp = &dmp;
            }
        }
    }
    if (pDmp) {
        pDmp->decode(model, inst, exDesc, desc, msg, error);
    } else {
        error = "no matching pattern found for descriptors";
        msg.family = Message::INVALID;
    }

    return msg;
}


size_t Message::numAddrRegisters() const
{
    size_t addrRegs = 0;
    switch (family) {
    case Message::SCATTERED: addrRegs = scattered.numAddrRegisters(); break;
    case Message::BLOCK:     addrRegs = block.numAddrRegisters(); break;
    default: return (size_t)-1;
    }

    size_t header = headerPresent ? 1 : 0;
    return addrRegs + header;
}

size_t Message::numDataRegisters() const
{
    switch (family) {
    case Message::SCATTERED: return scattered.numAddrRegisters();
    case Message::BLOCK:     return block.numAddrRegisters();
    default: return (size_t)-1;
    }
}

static void formatAddressModel(
    const Message &m,
    BasicFormatter &fmtr)
{
    switch (m.addrModel) {
    case AddrModel::A64_C:   fmtr.emit(".a64"); break;
    case AddrModel::A64_NC:  fmtr.emit(".nc64"); break;
    case AddrModel::A64_SSO: fmtr.emit(".sso64"); break;
    case AddrModel::A32_C:   fmtr.emit(".a32"); break;
    case AddrModel::A32_NC:  fmtr.emit(".nc32"); break;
    case AddrModel::A32_SSO: fmtr.emit(".sso64"); break;
    case AddrModel::SLM:     fmtr.emit(".slm"); break;
    case AddrModel::SCRATCH: fmtr.emit(".scratch"); break;
    default:
        if ((m.addrModel & AddrModel::BTI_FAMILY) == AddrModel::BTI_FAMILY) {
            fmtr.emit(".bt[");
            fmtr.emitDecimal((m.addrModel & 0xFF));
            fmtr.emit("]");
        } else {
            fmtr.emit(".<<AddrModel::", (int)m.addrModel, ">");
        }
    }
}

// e.g.  ld.sca32.x4.a64 ...
//       ld.blk128.x4.scratch ...
void Message::formatOpcode(const Instruction &i, BasicFormatter &fmtr) const {
    if (mode == Message::Mode::READ) {
        fmtr.emit("ld");
    } else {
        fmtr.emit("st");
    }
    if (isSplit) {
        fmtr.emit("s");
    }
    if (isConditional) {
        fmtr.emit("c");
    }
    fmtr.emit(".");
    fmtr.emit(mnemonic);

    switch (family) {
    case Family::SCATTERED:
        fmtr.emit(".");
        switch (scattered.elemsPerChannel) {
        case ScatteredMessage::ElemsPerChan::DE1: fmtr.emit("x1"); break;
        case ScatteredMessage::ElemsPerChan::DE2: fmtr.emit("x2"); break;
        case ScatteredMessage::ElemsPerChan::DE4: fmtr.emit("x4"); break;
        case ScatteredMessage::ElemsPerChan::DE8: fmtr.emit("x8"); break;
        default: fmtr.emit("?");
        }
        break;
    case Family::BLOCK:
    default:
        fmtr.emit("<Family::",(int)family,">");
        break;
    }

    formatAddressModel(*this, fmtr);
}

void Message::formatExecInfo(const Instruction &i, BasicFormatter &fmtr) const
{
    fmtr.emit("  (");
    fmtr.emit(ToSyntax(i.getExecSize()));
    int simdSize =
        family == Message::Family::SCATTERED ? (int)scattered.simdMode :
        family == Message::Family::SURFACE ? (int)surface.simdMode :
        0;
    if (simdSize != 0) {
        // message has a SIMD mode, we are hoping it matches the ExecSize
        // or we have to print both
        int isaExecSize = static_cast<int>(i.getExecSize());
        if (simdSize != isaExecSize) {
            fmtr.emit("/",simdSize);
        }
    }
    fmtr.emit(ToSyntax(i.getChannelOffset()),")");
}


static void formatRegRange(
    const Operand &op,
    size_t len,
    BasicFormatter &fmtr)
{
    if (op.getDirRegName() == RegName::ARF_NULL) {
        fmtr.emit("null");
        if ((int)op.getDirRegRef().regNum != 0) {
            fmtr.emit((int)op.getDirRegRef().regNum);
        }
    } else {
        if (op.getDirRegName() == RegName::GRF_R) {
            fmtr.emit("r",(int)op.getDirRegRef().regNum);
        } else {
            fmtr.emit("???",(int)op.getDirRegRef().regNum);
        }
        if (len > 1) {
            fmtr.emit("-");
            fmtr.emit((int)op.getDirRegRef().regNum + len - 1);
        }
    }
}

// ld....  r12-13:d   [r12-13]:d
//         ^^^^^^^^
static void formatLoadDst(
    Platform p,
    const Instruction &i,
    const Message &m,
    BasicFormatter &fmtr)
{
    const Operand &dst = i.getDestination();
//    formatRegRange(dst, m.numAddrRegisters(), fmtr);
    formatRegRange(dst, m.dstLen, fmtr);
    if (!i.getOpSpec().hasImplicitDstType()) {
        fmtr.emit(ToSyntax(dst.getType()));
    }
}
// ld....  r12-13:d   [r12-13]:d
//                    ^^^^^^^^^^
static void formatLoadSrcs(
    Platform p,
    const Instruction &i,
    const Message &m,
    BasicFormatter &fmtr)
{
    if (m.family == Message::BLOCK && m.addrModel == AddrModel::SCRATCH) {
        fmtr.emit("scratch");
    }
    fmtr.emit("[");

    const Operand &src0 = i.getSource(0);
    const Operand &src1 = i.getSource(1);
    formatRegRange(src0, m.src0Len, fmtr);
//    formatRegRange(src0, m.numAddrRegisters(), fmtr);
    if (i.getSourceCount() > 1) {
        fmtr.emit(":");
        formatRegRange(src1, m.src1Len, fmtr);
//        formatRegRange(src1, m.numDataRegisters(), fmtr);
    }

    if (m.family == Message::BLOCK && m.addrModel == AddrModel::SCRATCH) {
        fmtr.emitHex(m.addrOffset);
    } else if (m.addrOffset != 0) {
        fmtr.emit(" + ");
        fmtr.emitHex(m.addrOffset);
    }
    fmtr.emit("]");

    // TYPES:
    //
    // ugly as all heck for the split case
    //    [r12-14]:f         for send
    //    [r14:r12-14]:f:ud  for sends
    const OpSpec &os = i.getOpSpec();
    if (!os.hasImplicitSrcType(0,false,p)) {
        fmtr.emit(ToSyntax(src0.getType()));
    }
    if (i.getSourceCount() > 1) {
        if (!os.hasImplicitSrcType(1, false, p)) {
            fmtr.emit(ToSyntax(src1.getType()));
        }
    }
}


// st..... [r12-13]:d   r14-17:d
//         ^^^^^^^^^^
static void formatStoreDst(
    Platform p,
    const Instruction &i,
    const Message &m,
    BasicFormatter &fmtr)
{
    fmtr.emit("[TODO]");
}
void Message::formatDst(
    Platform p,
    const Instruction &i,
    BasicFormatter &fmtr) const
{
    if (mode == Mode::READ) {
        formatLoadDst(p, i, *this, fmtr);
    } else {
        formatStoreDst(p, i, *this, fmtr);
    }
}

// st..... [r12-13]:d   r14-17:d
//                      ^^^^^^^^
static void formatStoreSrcs(
    Platform p,
    const Instruction &i,
    const Message &m,
    BasicFormatter &fmtr)
{
    fmtr.emit("TODO");
}
void Message::formatSrcs(
    Platform p,
    const Instruction &i,
    BasicFormatter &fmtr) const
{
    if (mode == Mode::READ) {
        formatLoadSrcs(p, i, *this, fmtr);
    } else {
        formatStoreSrcs(p, i, *this, fmtr);
    }
}

/*
void Message::formatExtraInstOpts(
    const Instruction &i,
    BasicFormatter &fmtr,
    bool &somethingEmitted) const
{
    auto emitOption = [&](const char *opt) {
        if (somethingEmitted) {
            fmtr.emit(",");
        }
        fmtr.emit(opt);
        somethingEmitted = true;
    };
    if (invalidateOnRead) {
        emitOption("InvalidateOnRead");
    }
}
*/

void Message::formatExtraInstOpts( // TODO: hack
    const Instruction &i,
    std::vector<const char *> &output) const
{
    if (invalidateOnRead) {
        output.push_back("InvalidateOnRead");
    }
}
