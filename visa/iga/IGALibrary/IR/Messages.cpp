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
#include "Messages.hpp"
#include "../Backend/Messages/MessageDecoder.hpp"
#include "../Frontend/IRToString.hpp"

#include <sstream>
#include <string>

using namespace iga;


///////////////////////////////////////////////////////////////////////////////
static void deducePayloadSizes(
    PayloadLengths &lens,
    Platform p, SFID sfid, ExecSize execSize, uint32_t _desc)
{
    SendDesc desc(_desc), exDesc(0);

    const auto result =
        tryDecode(p, sfid, exDesc, desc, REGREF_INVALID, nullptr);
    if (!result) {
        return;
    }
    const MessageInfo &mi = result.info;

    const int REG_SIZE_BITS =
            256;
    const int FULL_EXEC_SIZE =
            16;

    int execElems = static_cast<int>(execSize);
    if (execElems < FULL_EXEC_SIZE/2)
        execElems = FULL_EXEC_SIZE/2;

    auto opIsGroup = [&] (SendOp opGroup) {
        return (static_cast<int>(result.info.op) &
            static_cast<int>(opGroup)) != 0;
    };
    auto isVectorMessageSimd1 = [&] () {
        bool isSimd1 = mi.isTransposed() ||
            mi.op == SendOp::LOAD_STRIDED ||
            mi.op == SendOp::STORE_STRIDED;
        return isSimd1;
    };

    // messages of the form:
    //    send*  null  reg  null ...
    // with src0 len of 1
    auto message = [&] (int dstLen = 0, int src0Len = 1, int src1Len = 0) {
        lens.dstLen = dstLen;
        lens.src0Len = src0Len;
        lens.src1Len = src1Len;
    };

    auto numAddrRegsForVector = [&] () {
        if (isVectorMessageSimd1()) {
            return 1;
        } else {
            int alen = execElems*mi.addrSizeBits/REG_SIZE_BITS;
            if (execElems*mi.addrSizeBits % REG_SIZE_BITS)
                alen++;
            return alen;
        }
    };

    auto handleVectorMessage = [&] (int atomicParams = -1) {
        int dlen = -1;
        if (mi.isTransposed()) {
            dlen = mi.elemSizeBitsRegFile*mi.elemsPerAddr/REG_SIZE_BITS;
            if ((mi.elemSizeBitsRegFile*mi.elemsPerAddr) % REG_SIZE_BITS != 0)
                dlen++;
        } else {
            dlen = execElems*mi.elemSizeBitsRegFile/REG_SIZE_BITS;
            if ((execElems*mi.elemSizeBitsRegFile) % REG_SIZE_BITS != 0)
                dlen++;
            dlen *= mi.elemsPerAddr;
        }
        //
        lens.src0Len = numAddrRegsForVector();
        if (SendOpIsLoad(mi.op)) {
            lens.dstLen = dlen;
            lens.src1Len = 0;
        } else if (SendOpIsStore(mi.op)) {
            lens.dstLen = 0;
            lens.src1Len = dlen;
        } else if (SendOpIsAtomic(mi.op)) {
            lens.dstLen = dlen;
            lens.src1Len = atomicParams;
        } else {
            IGA_ASSERT_FALSE("invalid message type");
        }
    };

    lens.uvrlod = mi.hasAttr(MessageInfo::TYPED);

    switch (mi.op) {
    ///////////////////////////////////////////////////////////////////////
    // easy and common cases
    case SendOp::LOAD:
    case SendOp::LOAD_STRIDED:
    case SendOp::LOAD_QUAD:
    case SendOp::STORE:
    case SendOp::STORE_STRIDED:
    case SendOp::STORE_QUAD:
        // normal vector message
        handleVectorMessage();
        break;
    ///////////////////////////////////////////////////////////////////////
    // slightly harder
    case SendOp::LOAD_STATUS:
        // returns one status register only
        // cheat so num addr regs and src1 are set;
        // then stomping the return value to reflect the 32b return value
        handleVectorMessage();
        lens.dstLen = 1;
        break;
    case SendOp::READ_STATE:
        // src0 is U, V, R, LOD 32b each (128b total)
        // dst is 1 reg (with room to spare)
        message(1, 1, 0);
        break;
    case SendOp::ATOMIC_LOAD:
    case SendOp::ATOMIC_IINC:
    case SendOp::ATOMIC_IDEC:
    case SendOp::ATOMIC_IPDEC:
        handleVectorMessage(0);
        break;
    case SendOp::ATOMIC_STORE:
    case SendOp::ATOMIC_AND:
    case SendOp::ATOMIC_XOR:
    case SendOp::ATOMIC_OR:
    case SendOp::ATOMIC_IADD:
    case SendOp::ATOMIC_ISUB:
    case SendOp::ATOMIC_IRSUB:
    case SendOp::ATOMIC_SMIN:
    case SendOp::ATOMIC_SMAX:
    case SendOp::ATOMIC_UMIN:
    case SendOp::ATOMIC_UMAX:
    case SendOp::ATOMIC_FADD:
    case SendOp::ATOMIC_FSUB:
    case SendOp::ATOMIC_FMIN:
    case SendOp::ATOMIC_FMAX:
        handleVectorMessage(1);
        break;
    case SendOp::ATOMIC_ICAS:
    case SendOp::ATOMIC_FCAS:
        handleVectorMessage(2);
        break;
    ///////////////////////////////////////////////////////////////////////
    // control messages that are 0, 1, 0
    case SendOp::BARRIER:
    case SendOp::MONITOR:
    case SendOp::UNMONITOR:
    case SendOp::SIGNAL_EVENT:
    case SendOp::EOT:
        message();
        break;
    ///////////////////////////////////////////////////////////////////////
    // control messages that return a register
    case SendOp::FENCE:
        message(1, 1, 0);
        break;
    case SendOp::WAIT:
        message(1, 1, 0);
        break;
    default:
        break;
    }
}


PayloadLengths::PayloadLengths(
    Platform p, SFID sfid, ExecSize execSize, uint32_t desc)
{
    deducePayloadSizes(*this, p, sfid, execSize, desc);
}


PayloadLengths::PayloadLengths(
    Platform p, ExecSize execSize, uint32_t desc, uint32_t exDesc)
{
    IGA_ASSERT(p <= Platform::GEN11, "wrong constructor for platform");
    SFID sfid = sfidFromEncoding(p, exDesc);
    deducePayloadSizes(*this, p, sfid, execSize, desc);
}


SFID iga::sfidFromEncoding(Platform p, uint32_t sfidBits)
{
    SFID sfid = SFID::INVALID;
    switch (sfidBits & 0xF) {
    case 0x0: sfid = SFID::NULL_; break;
    case 0x2: sfid = SFID::SMPL; break;
    case 0x3: sfid = SFID::GTWY; break;
    case 0x4: sfid = SFID::DC2;  break;
    case 0x5: sfid = SFID::RC;   break;
    case 0x6: sfid = SFID::URB;  break;
    case 0x7:
        sfid = SFID::TS;
        break;
    case 0x8:
        sfid = SFID::VME;
        break;
    case 0x9:
        sfid = SFID::DCRO;
        break;
    case 0xA: sfid = SFID::DC0;  break;
    case 0xB: sfid = SFID::PIXI; break;
    case 0xC: sfid = SFID::DC1;  break;
    case 0xD:
        sfid = SFID::CRE;
        break;
    default:
        sfid = SFID::INVALID;
    }
    return sfid;
}



std::string iga::format(SFID sfid)
{
    return ToSyntax(sfid);
}

///////////////////////////////////////////////////////////////////////////////
std::string MessageSyntax::str(
    std::string execInfo,
    std::string dataReg,
    std::string addrReg,
    std::string atmoicArgReg) const
{
    std::stringstream ss;

    auto emitAddr =
        [&]() {
            if (addrReg.empty())
                return;
            ss << " " << surface;
            ss << "[";
            ss << scale;
            ss << addrReg;
            ss << immOffset;
            ss << "]";
        };
    auto emitData =
        [&]() {
            if (!dataReg.empty()) {
                ss << " " << dataReg;
            }
        };
    auto emitExtraArg =
        [&]() {
            if (!atmoicArgReg.empty())
                ss << " " << atmoicArgReg;
        };

    ss << mnemonic << controls;
    if (!execInfo.empty())
        ss << " " << execInfo;

    if (isLoad()) {
        emitData();
        emitAddr();
    } else if (isStore()) {
        emitAddr();
        emitData();
    } else if (isAtomic()) {
        emitData();
        emitAddr();
        emitExtraArg();
    } else if (isControl()) {
        // not sure the cleanest way to do this
        emitData();
        emitAddr();
        emitExtraArg();
    }

    return ss.str();
}

std::string MessageSyntax::sym() const
{
    std::stringstream ss;
    ss << mnemonic << controls;
    if (!surface.empty()) {
        ss << "." << surface;
    }
    if (!scale.empty() || !immOffset.empty()) {
        if (surface.empty())
            ss << ".flat";
        ss << "[" << scale << "A" << immOffset << "]";
    }
    return ss.str();
}

std::string iga::format(SendOp op)
{
#define MK_CASE(X) case SendOp::X: return #X
    switch (op) {
        MK_CASE(LOAD);
        MK_CASE(LOAD_STRIDED);
        MK_CASE(LOAD_QUAD);
        MK_CASE(LOAD_STATUS);
        //
        MK_CASE(STORE);
        MK_CASE(STORE_STRIDED);
        MK_CASE(STORE_QUAD);
        //
        MK_CASE(ATOMIC_LOAD);
        MK_CASE(ATOMIC_STORE);
        //
        MK_CASE(ATOMIC_AND);
        MK_CASE(ATOMIC_XOR);
        MK_CASE(ATOMIC_OR);
        //
        MK_CASE(ATOMIC_IINC);
        MK_CASE(ATOMIC_IDEC);
        MK_CASE(ATOMIC_IPDEC);
        MK_CASE(ATOMIC_IADD);
        MK_CASE(ATOMIC_ISUB);
        MK_CASE(ATOMIC_IRSUB);
        MK_CASE(ATOMIC_ICAS);
        //
        MK_CASE(ATOMIC_SMIN);
        MK_CASE(ATOMIC_SMAX);
        //
        MK_CASE(ATOMIC_UMIN);
        MK_CASE(ATOMIC_UMAX);
        //
        MK_CASE(ATOMIC_FADD);
        MK_CASE(ATOMIC_FSUB);
        MK_CASE(ATOMIC_FMIN);
        MK_CASE(ATOMIC_FMAX);
        MK_CASE(ATOMIC_FCAS);
        //
        MK_CASE(READ_STATE);
        //
        MK_CASE(FENCE);
        //
        MK_CASE(BARRIER);
        MK_CASE(MONITOR);
        MK_CASE(UNMONITOR);
        MK_CASE(WAIT);
        MK_CASE(SIGNAL_EVENT);
        MK_CASE(EOT);
        //
        //
        MK_CASE(SAMPLER_LOAD);
        MK_CASE(SAMPLER_FLUSH);
        //
        MK_CASE(RENDER_WRITE);
        MK_CASE(RENDER_READ);
        //
    default:
        std::stringstream ss;
        ss << "0x" << std::hex << (int)op << "?";
        return ss.str();
    }
#undef MK_CASE
}

std::string iga::format(CacheOpt op)
{
#define MK_CASE(X) case CacheOpt::X: return #X
    switch (op) {
        MK_CASE(DEFAULT);
        MK_CASE(READINVALIDATE);
        MK_CASE(CACHED);
        MK_CASE(UNCACHED);
    default:
        std::stringstream ss;
        ss << "0x" << std::hex << (int)op << "?";
        return ss.str();
    }
#undef MK_CASE
}

std::string iga::format(AddrType op)
{
#define MK_CASE(X) case AddrType::X: return #X
    switch (op) {
        MK_CASE(FLAT);
        MK_CASE(BTI);
    default:
        std::stringstream ss;
        ss << "0x" << std::hex << (int)op << "?";
        return ss.str();
    }
#undef MK_CASE
}




static bool isHDC(SFID sfid)
{
    switch (sfid) {
    case SFID::DCRO:
    case SFID::DC0:
    case SFID::DC1:
    case SFID::DC2:
        return true;
    default:
        return false;
    }
}



///////////////////////////////////////////////////////////////////////////////
// e.g. set valid bit, re-order fields etc...
static void postProcessDecode(
    SendDesc desc, SendDesc exDesc,
    DecodeResult &result, DecodedDescFields *fields)
{
    if (!result.errors.empty())
        result.info.attributeSet |= MessageInfo::VALID;
    if (fields) {
        std::sort(result.fields.begin(), result.fields.end(),
            [&] (const auto &f1, const auto &f2) {
                return std::get<0>(f1).offset >
                    std::get<0>(f2).offset;
            });
        *fields = result.fields;
        //
        // make sure there aren't unmapped bits
        // run through all 64 bits of [ExDesc:Desc] and find maximal spans
        // that have no owner field (breaking between descriptors)
        auto fieldOwnsBit =
            [&](int i) {
                for (const auto &fv : result.fields) {
                    const auto &f = std::get<0>(fv);
                    if (i >= f.offset && i < f.offset + f.length) {
                        return true;
                    }
                }
                return false;
            };

        int len = exDesc.isImm() ? 64 : 32; // 32 if a0
        auto testDescBit =
            [&] (int ix) {
            uint32_t val = desc.imm;
            if (ix >= 32) {
                ix -= 32;
                val = exDesc.imm;
            }
            return (val & (1 << ix)) != 0;
        };

        for (int i = 0; i < len;) {
            if (testDescBit(i) && !fieldOwnsBit(i)) {
                // beginning of an undefined field
                int undefLen = 1;
                while ((i + undefLen) % 32 != 0 &&
                    testDescBit(i + undefLen))
                {
                    // don't span undefined fields across ExDesc:Desc
                    undefLen++;
                }
                //
                result.warnings.emplace_back(
                    DescField(i, undefLen), "bits set in undefined field");
                i += undefLen;
                //
                // uncomment for linting bad descriptors
                // std::stringstream ss;
                // ss << "[" << i << "] = 1 not mapped by field";
                // IGA_ASSERT_FALSE(ss.str().c_str());
            } else {
                i++;
            }
        }
    }

    // set the syntax kind
    if (result && !result.syntax.mnemonic.empty()) {
        // !result.syntax.mnemonic.empty(): to filter out HDC stuff until
        // we get that working
        if (result.info.isLoad()) {
            result.syntax.layout = MessageSyntax::Layout::LOAD;
        } else if (result.info.isStore()) {
            result.syntax.layout = MessageSyntax::Layout::STORE;
        } else if (result.info.isAtomic()) {
            result.syntax.layout = MessageSyntax::Layout::ATOMIC;
        } else {
            result.syntax.layout = MessageSyntax::Layout::CONTROL;
        }
    }
} // postProcessDecode


DecodeResult iga::tryDecode(
    Platform platform, SFID sfid,
    SendDesc exDesc, SendDesc desc, RegRef indDesc,
    DecodedDescFields *fields)
{
    DecodeResult result;

    if (isHDC(sfid)) {
        decodeDescriptorsHDC(
            platform, sfid,
            exDesc, desc, indDesc,
            result);
    }
    else {
        decodeDescriptorsOther(
            platform, sfid,
            exDesc, desc, indDesc,
            result);
    }

    postProcessDecode(desc, exDesc, result, fields);

    return result;
}


SendOp iga::lookupSendOp(std::string mne)
{
    SendOp op = SendOp::INVALID;
    if (mne == "load") {
        op = SendOp::LOAD;
    } else if (mne == "load_quad") {
        op = SendOp::LOAD_QUAD;
    } else if (mne == "load_strided") {
        op = SendOp::LOAD_STRIDED;
    } else if (mne == "load_status") {
        op = SendOp::LOAD_STATUS;
        //////////////////////////////////////////////////
    } else if (mne == "store") {
        op = SendOp::STORE;
    } else if (mne == "store_quad") {
        op = SendOp::STORE_QUAD;
    } else if (mne == "store_strided") {
        op = SendOp::STORE_STRIDED;
        //////////////////////////////////////////////////
    } else if (mne == "atomic_load") {
        op = SendOp::ATOMIC_LOAD;
    } else if (mne == "atomic_store") {
        op = SendOp::ATOMIC_STORE;
    } else if (mne == "atomic_and") {
        op = SendOp::ATOMIC_AND;
    } else if (mne == "atomic_xor") {
        op = SendOp::ATOMIC_XOR;
    } else if (mne == "atomic_or") {
        op = SendOp::ATOMIC_OR;
    } else if (mne == "atomic_iinc") {
        op = SendOp::ATOMIC_IINC;
    } else if (mne == "atomic_idec") {
        op = SendOp::ATOMIC_IDEC;
    } else if (mne == "atomic_ipdec") {
        op = SendOp::ATOMIC_IPDEC;
    } else if (mne == "atomic_iadd") {
        op = SendOp::ATOMIC_IADD;
    } else if (mne == "atomic_isub") {
        op = SendOp::ATOMIC_ISUB;
    } else if (mne == "atomic_irsub") {
        op = SendOp::ATOMIC_IRSUB;
    } else if (mne == "atomic_icas") {
        op = SendOp::ATOMIC_ICAS;
    } else if (mne == "atomic_smin") {
        op = SendOp::ATOMIC_SMIN;
    } else if (mne == "atomic_smax") {
        op = SendOp::ATOMIC_SMAX;
    } else if (mne == "atomic_umin") {
        op = SendOp::ATOMIC_UMIN;
    } else if (mne == "atomic_umax") {
        op = SendOp::ATOMIC_UMAX;
    } else if (mne == "atomic_fadd") {
        op = SendOp::ATOMIC_FADD;
    } else if (mne == "atomic_fsub") {
        op = SendOp::ATOMIC_FSUB;
    } else if (mne == "atomic_fmin") {
        op = SendOp::ATOMIC_FMIN;
    } else if (mne == "atomic_fmax") {
        op = SendOp::ATOMIC_FMAX;
    } else if (mne == "atomic_fcas") {
        op = SendOp::ATOMIC_FCAS;
    } else if (mne == "read_state") {
        op = SendOp::READ_STATE;
    } else if (mne == "fence") {
        op = SendOp::FENCE;
    } else if (mne == "barrier") {
        op = SendOp::BARRIER;
    } else if (mne == "monitor") {
        op = SendOp::MONITOR;
    } else if (mne == "unmonitor") {
        op = SendOp::UNMONITOR;
    } else if (mne == "wait") {
        op = SendOp::WAIT;
    } else if (mne == "signal_event") {
        op = SendOp::SIGNAL_EVENT;
    } else if (mne == "eot") {
        op = SendOp::EOT;
    }
    // TODO: sampler and render messages
    return op;
}

SendOpInfo iga::lookupSendOpInfo(SendOp op)
{
    SendOpInfo soi {false, -1, false};

    switch (op) {
    case SendOp::LOAD_STRIDED:
    case SendOp::STORE_STRIDED:
    case SendOp::READ_STATE:
        soi.isSingleRegAddr = true;
    default: break;
    }

    if (SendOpIsLoad(op)) {
        soi.hasDst = true;
        soi.src1Args = 0;
    } else if (SendOpIsStore(op)) {
        soi.hasDst = false;
        soi.src1Args = 1;
    } else {
        auto atomic = [&](int args) {
            soi.hasDst = true;
            soi.src1Args = args;
        };

        switch (op) {
        case SendOp::ATOMIC_LOAD:
        case SendOp::ATOMIC_IINC:
        case SendOp::ATOMIC_IDEC:
        case SendOp::ATOMIC_IPDEC:
            atomic(0);
            break;
        case SendOp::ATOMIC_STORE:
        case SendOp::ATOMIC_AND:
        case SendOp::ATOMIC_XOR:
        case SendOp::ATOMIC_OR:
        case SendOp::ATOMIC_IADD:
        case SendOp::ATOMIC_ISUB:
        case SendOp::ATOMIC_IRSUB:
        case SendOp::ATOMIC_SMIN:
        case SendOp::ATOMIC_SMAX:
        case SendOp::ATOMIC_UMIN:
        case SendOp::ATOMIC_UMAX:
        case SendOp::ATOMIC_FADD:
        case SendOp::ATOMIC_FSUB:
        case SendOp::ATOMIC_FMIN:
        case SendOp::ATOMIC_FMAX:
            atomic(1);
            break;
        case SendOp::ATOMIC_ICAS:
        case SendOp::ATOMIC_FCAS:
            atomic(2);
            break;
        case SendOp::FENCE:
            // some fences have dstinations...
            soi.hasDst = false;
            break;
        default: break;
        }
    }

    return soi;
}


bool iga::sendOpSupportsSyntax(Platform p, SendOp op, SFID sfid)
{
    bool supported = true;
    supported &=
        op == SendOp::LOAD ||
        op == SendOp::LOAD_STRIDED ||
        op == SendOp::LOAD_QUAD ||
        op == SendOp::STORE ||
        op == SendOp::STORE_STRIDED ||
        op == SendOp::STORE_QUAD ||
        SendOpIsAtomic(op);
    return supported;
}




bool iga::encodeDescriptors(
    Platform p,
    const VectorMessageArgs &vma,
    SendDesc &exDesc,
    SendDesc &desc,
    std::string &err)
{
    // TODO: support HDC here
    // encodeVectorMessageHDC(p, vma, exDesc, desc);
    err = "unsupported message for SFID";
    return false;
}
