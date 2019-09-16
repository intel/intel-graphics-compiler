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
#include "MessageInfo.hpp"
#include "Native/Field.hpp"
#include "../asserts.hpp"

#include <algorithm>
#include <functional>
#include <sstream>
#include <tuple>
#include <vector>

using namespace iga;

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
    MK_CASE(SIGNAL);
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
// #define MK_CASE(X) case CacheOpt::X: return "CacheOpt::" #X
#define MK_CASE(X) case CacheOpt::X: return #X
    switch (op) {
    MK_CASE(DEFAULT);
    MK_CASE(READINVALIDATE);
    default:
        std::stringstream ss;
        ss << "0x" << std::hex << (int)op << "?";
        return ss.str();
    }
#undef MK_CASE
}

std::string iga::format(AddrType op)
{
// #define MK_CASE(X) case CacheOpt::X: return "CacheOpt::" #X
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

using DescFieldFormatter = std::function<void (std::stringstream &,uint32_t)>;
static void NO_DECODE(std::stringstream &,uint32_t) { }

static const char *MDC_DS_MEANINGS[] {
    "DE1 (1 data element per addr.)",
    "DE2 (2 data elements per addr.)",
    "DE4 (4 data elements per addr.)",
    "DE8 (8 data elements per addr.)",
};

struct DescDecoder {
    // inputs
    const Platform         platform;
    const SFID             sfid;
    const uint32_t         desc, exDesc;

    // outputs
    MessageInfo            mi { };
    DiagnosticList        &warnings;
    DiagnosticList        &errors;
    DecodedDescFields      fields;

    // some constants to make some of the program more declarative
    const int SLM_BTI = 0xFE;
    const int COHERENT_BTI = 0xFF;
    const int NONCOHERENT_BTI = 0xFD;
    const int DEFAULT_EXEC_SIZE;
    const int BITS_PER_REGISTER;

    DescDecoder(
        Platform _platform,
        SFID _sfid,
        uint32_t _exDesc,
        uint32_t _desc,
        DiagnosticList &_warnings,
        DiagnosticList &_errors)
        : platform(_platform)
        , sfid(_sfid)
        , desc(_desc)
        , exDesc(_exDesc)
        , warnings(_warnings)
        , errors(_errors)
        , DEFAULT_EXEC_SIZE(16)
        , BITS_PER_REGISTER(256)
    {
        mi.op = SendOp::INVALID;
        mi.cachingL3 = mi.cachingL1 = CacheOpt::DEFAULT;
        mi.elemSizeBitsRegFile = mi.elemSizeBitsMemory = 0;
        mi.channelsEnabled = mi.elemsPerAddr = 0;
        mi.execWidth = 0;
        mi.attributeSet = 0;
        mi.addrType = AddrType::FLAT;
        mi.surface = 0;
        mi.immediateOffset = 0;
        mi.docs = nullptr;
    }

    void setDoc(const char *doc) {
        mi.docs = doc;
    }

    /////////////////////////////////////////////////////////////
    // diagnostics

    template <
        typename T1,
        typename T2 = const char *,
        typename T3 = const char *>
    void addDiag(
        DiagnosticList &dl,
        int off, int len,
        T1 t1,
        T2 t2 = "",
        T3 t3 = "")
    {
        std::stringstream ss;
        ss << t1 << t2 << t3;
        dl.emplace_back(DescField(off,len), ss.str());
    }
    template <typename T1,
        typename T2 = const char *, typename T3 = const char *>
    void warning(int off, int len, T1 t1, T2 t2 = "", T3 t3 = "") {
        addDiag(warnings, off, len, t1, t2, t3);
    }
    template <typename T1,
        typename T2 = const char *, typename T3 = const char *>
    void error(int off, int len, T1 t1, T2 t2 = "", T3 t3 = "") {
        addDiag(errors, off, len, t1, t2, t3);
    }

    bool hasExDesc() const {
        return exDesc != 0xFFFFFFFF;
    }

    // TODO: phase out
    // offsets 32 to 64 fetch exDesc
    uint32_t getDescBits(int off, int len) const {
        uint32_t bits = desc;
        if (off >= 32) {
            off -= 32;
            bits = exDesc;
        }
        uint32_t mask = len == 32 ? 0xFFFFFFFF : ((1 << len) - 1);
        return (int)((bits >> off) & mask);
    }
    // TODO: phase out
    uint32_t getDescBit(int off) const {
        return getDescBits(off,1) != 0;
    }

    uint32_t getExDescBitsField(
        const char *fieldName,
        int off,
        int len,
        DescFieldFormatter fmtMeaning = NO_DECODE)
    {
        auto val = getDescBits(off+32, len);
        std::stringstream ss;
        fmtMeaning(ss, val);
        addField(fieldName, off+32, len, val, ss.str());
        return val;
    }
    uint32_t getDescBitsField(
        const char *fieldName,
        int off,
        int len,
        DescFieldFormatter fmtMeaning = NO_DECODE)
    {
        auto val = getDescBits(off, len);
        std::stringstream ss;
        fmtMeaning(ss, val);
        addField(fieldName, off, len, val, ss.str());
        return val;
    }
    uint32_t getDescBitField(
        const char *fieldName,
        int off,
        const char *zero,
        const char *one)
    {
        return getDescBitsField(fieldName, off, 1,
            [&] (std::stringstream &ss,uint32_t val) {
                ss << (val ? one : zero);
            });
    }
    uint32_t getDescBitField(
        const char *fieldName,
        int off,
        const char *one)
    {
        return getDescBitField(fieldName, off, "", one);
    }

    // normally use getDescBitsField, but in cases where you've already
    // decoded, the meaning and just want to record the result
    void addField(
        const char *fieldName,
        int off,
        int len,
        uint32_t val,
        std::string meaning)
    {
        Field f {fieldName, off, len};
        for (const auto &fvs : fields) {
            auto f1 = std::get<0>(fvs);
            if (f1.overlaps(f)) {
                // uncomment for debugging
                // std::stringstream ss;
                // ss << "overlapped fields: " << f1.name << " and " << f.name;
                // IGA_ASSERT_FALSE(ss.str().c_str());
                return; // replicated access (don't record again)
            }
        }
        fields.emplace_back(f, val, meaning);
    }


    ///////////////////////////////////////////////////////////////////////////
    // decoder helpers
    bool decodeExpected(
        int off,
        int len,
        const char *fieldName,
        uint32_t expected)
    {
        auto val = getDescBits(off, len);
        if (val != expected) {
            warning(off, len, "field should be ", expected);
        }
        addField(fieldName, off, len, val, "");
        return val == expected;
    }
    int decodeXXX_HW(int off, int len, const char *fieldName) {
        // scratch's use of DataElements:MDC_DB_HW uses 2 bits
        // DataElements:MDC_DB_HW for A32 uses 3 bits (starting at bit 8)
        // the A64 version is always [10:8]
        auto bits = getDescBits(off,len);
        // 0 -> 1 block, 1 -> 2 blocks, 2 -> 4 blocks, ...
        int val = (int)(1 << bits);
        std::stringstream ss;
        ss << val << " 256b blocks";
        addField(fieldName, off, len, getDescBits(off,len), ss.str());
        return val;
    }
    int decodeMDC_A64_DB_HW(int off) {
      return decodeXXX_HW(off, 3, "DataElements:MDC_A64_DB_HW");
    }
    int decodeMDC_DB_HW(int off, int len) {
      return decodeXXX_HW(off, len, "DataElements:MDC_DB_HW");
    }
    //
    int decodeXXX_OW(int off, const char *fieldName) {
        int bits = getDescBits(off,3);
        int ows = 0;
        const char *desc = "???";
        // MDC_DB_OW and MDC_64_DB_OW
        // 1L, 1H, 2, 4, 8, 16
        switch (bits) {
        case 0: ows = 1; desc = "1L (1 Oword accessed in low half of GRF)"; break;
        case 1: ows = 1; desc = "1H (1 Oword accessed in high half of GRF)"; break;
        case 2: ows = 2; desc = "2 OWords"; break;
        case 3: ows = 4; desc = "4 OWords"; break;
        case 4: ows = 8; desc = "8 OWords"; break;
        default: break;
        }
        addField(fieldName,off,3,bits,desc);
        return ows;
    }
    int decodeMDC_A64_DB_OW(int off) {
        return decodeXXX_OW(off, "DataElements:MDC_A64_DB_OW");
    }
    int decodeMDC_DB_OW(int off) {
        return decodeXXX_OW(off, "DataElements:MDC_DB_OW");
    }

    int decodeMDC_DS(int off) {
        auto bits = getDescBits(off,2);
        addField("DataElements:MDC_DS",off,2,bits,MDC_DS_MEANINGS[bits]);
        return (1 << (int)bits);
    }
    int MDC_A64_DS(int off) {
        auto bits = getDescBits(off,2);
        addField("DataElements:MDC_A64_DS",off,2,bits,MDC_DS_MEANINGS[bits]);
        return (1 << (int)bits);
    }
    int decodeMDC_DWS_DS(int off) {
        auto bits = getDescBits(off,2);
        static const char *meanings[]{
            "Fill 1 byte per DW",
            "Fill 2 bytes per DW",
            "Fill all 4 bytes per DW",
            "???"
        };
        addField("DataElements:MDC_DWS_DS",off,2,bits,meanings[bits]);
        return (1 << (int)bits);
    }
    int decodeMDC_SM2(int off) {
        // yeah SM2 is really 1 bit (2 means two values)
        int bits = getDescBitField("SimdMode:MDC_SM2",off,"SIMD8","SIMD16");
        return bits ? 16 : 8;
    }
    int decodeMDC_SM2R(int off) {
        int bits = getDescBitField("SimdMode:MDC_SM2R",off,"SIMD16","SIMD8");
        return bits ? 8 : 16;
    }
    int decodeMDC_SM3(int off) {
        auto bits = getDescBits(off,2);
        int simd = 0;
        const char *simdStr = "?";
        switch (bits) {
        case 1: simd = 8; simdStr = "SIMD8"; break;
        case 2: simd = 16; simdStr = "SIMD16"; break;
        default: error(off,2,"invalid MDC_SM3"); break;
        }
        addField("SimdMode:MDC_SM3",off,2,bits,simdStr);
        return simd;
    }
    int decodeMDC_CMASK() {
        auto bits = getDescBits(8,4);
        std::stringstream ss;
        int vecLen = 0;
        if (bits == 0xF) {
            error(8,4,"channel mask must have one element not disabled");
        }
        for (int i = 0; i < 4; i++) {
            if (((1<<i)&bits) == 0) {
                if (vecLen > 0)
                    ss << ", ";
                ss << "XYZW"[i];
                vecLen++;
            }
        }
        if (vecLen == 0)
            ss << "no channels enabled";
        else
            ss << " enabled";
        addField("ChannelDisableMask:MDC_CMASK",8,4,bits,ss.str());
        return vecLen;
    }

    /////////////////////////////////////////////////////
    // "header" decoding
    bool decodeMDC_H() { // optional
        return getDescBitField(
            "Header",
            19,
            "absent",
            "included") != 0;
    }
    bool decodeMDC_H2() {
        return getDescBitField(
            "DualHeader", 19, "absent",
            "included (two register header)") != 0;
    }
    void decodeMDC_HF() {
        if (getDescBit(19) != 0)
            warning(19, 1, "this message forbids a header (and it's included)");
    }
    void decodeMDC_HR() {
        if (!decodeMDC_H())
            warning(19, 1, "this message requires a header (and it's absent)");
    }

    // returns true if high slot group
    bool decodeMDC_SG3() {
        auto bits = getDescBits(12,2);
        const char *sym = "??";
        switch (bits) {
        case 0: sym = "SG4x2"; break;
        case 1:
            sym = "SG8L";
            break;
        case 2: sym = "SG8U"; break;
        default:
            error(12,2,"invalid slot group value");
        }
        addField("SlotGroup:MDC_SG3", 12, 2, bits, sym);
        return bits == 2;
    }
    bool decodeMDC_SG2() {
        auto bits = getDescBits(12,1);
        const char *sym = "??";
        switch (bits) {
        case 0:
            sym = "SG8L";
            break;
        case 1: sym = "SG8U"; break;
        default: break;
        }
        addField("SlotGroup:MDC_SG2", 12, 1, bits, sym);
        return bits == 1;
    }

    int decodeBTI(int addrBits) {
        int bti = (int)getDescBits(0,8);
        std::stringstream ss;
        ss << "surface " << bti;
        if (bti == SLM_BTI) {
            ss << " (SLM)";
        } else if (bti == COHERENT_BTI) {
            if (addrBits == 64)
                ss << "A64 ";
            else
                ss << "A32 ";
            ss << " (coherent stateless)";
        } else if (bti == NONCOHERENT_BTI) {
            if (addrBits == 64)
                ss << " A64";
            else
                ss << " A32";
            ss << " (incoherent stateless)";
        } else if (bti == 0xFC) {
            ss << " (SSO)";
        }
        addField("BTI",0,8,bti,ss.str());
        return bti;
    }

    CacheOpt decodeMDC_IAR() {
        int bits = getDescBitField("MDC_IAR",13,"disabled","enabled");
        return bits ? CacheOpt::READINVALIDATE : CacheOpt::DEFAULT;
    }

    ///////////////////////////////////////////////////////////////////////////
    // the most generic setter
    void setScatterGatherOpX(
        std::string msgSym,
        std::string msgImpl,
        SendOp op,
        AddrType addrType,
        uint32_t surfaceId,
        CacheOpt l1,
        CacheOpt l3,
        int addrSize,
        int bitsPerElemReg, int bitsPerElemMem,
        int elemsPerAddr,
        int simd,
        int extraAttrs = 0)
    {
        mi.symbol = msgSym;
        mi.description = msgImpl;
        mi.op = op;
        mi.cachingL1 = l1;
        mi.cachingL3 = l3;
        mi.addrType = addrType;
        mi.surface = surfaceId;
        if (errors.empty())
            mi.attributeSet |= MessageInfo::VALID;
        mi.attributeSet |= extraAttrs;
        mi.addrSizeBits = addrSize;
        mi.elemSizeBitsRegFile = bitsPerElemReg;
        mi.elemSizeBitsMemory = bitsPerElemMem;
        mi.elemsPerAddr = elemsPerAddr;
        mi.channelsEnabled = 0;
        mi.execWidth = simd;
    }

    void setScatterGatherOp(
        std::string msgSym,
        std::string msgDesc,
        SendOp op,
        AddrType addrType,
        uint32_t surfaceId,
        int addrSize,
        int bitsPerElem,
        int elemsPerAddr,
        int simd,
        int extraAttrs = 0)
    {
        setScatterGatherOpX(
            msgSym,
            msgDesc,
            op,
            addrType,
            surfaceId,
            CacheOpt::DEFAULT,
            CacheOpt::DEFAULT,
            addrSize,
            bitsPerElem, bitsPerElem,
            elemsPerAddr,
            simd,
            extraAttrs);
    }

    // allows different data sizes in mem and reg
    void setHdcMessageX(
      std::string msgSym,
      std::string msgDesc,
      SendOp op,
      int addrSizeBits,
      int bitsPerElemReg,
      int bitsPerElemMem,
      int elemsPerAddr,
      int simd,
      int extraAttrs)
    {
        std::stringstream ss;
        ss << "hdc_";
        ss << msgSym; // e.g. "load"
        if (simd == 8 || simd == 16)
            ss << "_simd" << simd;

        ss << ".";
        int bti = 0;
        if (!(extraAttrs & MessageInfo::SCRATCH)) {
            bti = decodeBTI(addrSizeBits);
        }
        AddrType addrType = AddrType::BTI;
        uint32_t surfaceId = 0;
        if (addrSizeBits == 32) {
            if (extraAttrs & MessageInfo::SCRATCH) {
                ss << "scratch";
                ss << "+" << 32*getDescBits(0,12);
            } else if (bti == SLM_BTI) {
                ss << "slm";
                addrType = AddrType::FLAT;
                surfaceId = 0;
                extraAttrs |= MessageInfo::SLM;
            } else if (bti == COHERENT_BTI || bti == NONCOHERENT_BTI) {
                ss << "stateless";
                if (bti != COHERENT_BTI) {
                  ss << "_incoherent";
                }
                extraAttrs |= bti == COHERENT_BTI ?
                    MessageInfo::COHERENT : 0;
                addrType = AddrType::FLAT;
            } else {
                addrType = AddrType::BTI;
                surfaceId = bti;
                ss << "bti[" << bti << "]";
            }
        } else if (addrSizeBits == 64) {
            ss << "stateless";
            if (bti != COHERENT_BTI) {
              ss << "_incoherent";
            }
            addrType = AddrType::FLAT;
            if (bti == COHERENT_BTI)
                extraAttrs |= MessageInfo::COHERENT;
            else if (bti != NONCOHERENT_BTI)
                error(0, 8, "must have 0xFF or 0xFD BTI");
        }
        ss << ".a" << addrSizeBits;
        if (bitsPerElemReg == bitsPerElemMem) {
            ss << ".d" << bitsPerElemReg;
        } else {
            ss << ".d" << bitsPerElemMem << "c" << bitsPerElemReg;
        }
        int chEnMask = 0;
        if (op == SendOp::LOAD_QUAD || op == SendOp::STORE_QUAD) {
            ss << ".";
            // in legacy HDC messages, this is a channel disable mask
            auto chDisabled = getDescBits(8,4);
            chEnMask = ~chDisabled & 0xF;
            if ((chDisabled & 0x1) == 0) {
                ss << 'x';
            }
            if ((chDisabled & 0x2) == 0) {
                ss << 'y';
            }
            if ((chDisabled & 0x4) == 0) {
                ss << 'z';
            }
            if ((chDisabled & 0x8) == 0) {
                ss << 'w';
            }
        } else {
            if (elemsPerAddr > 1 ||
                (extraAttrs & MessageInfo::TRANSPOSED))
            {
                ss << "x" << elemsPerAddr;
            }
        }

        // TODO: if DC2, ExDesc[31:16] is immediate offset
        if (sfid == SFID::DC2)
            mi.immediateOffset = exDesc >> 16;
        setScatterGatherOpX(
            ss.str(),
            msgDesc,
            op,
            addrType,
            surfaceId,
            CacheOpt::DEFAULT,
            CacheOpt::DEFAULT,
            addrSizeBits,
            bitsPerElemReg, bitsPerElemMem,
            elemsPerAddr,
            simd,
            extraAttrs);
        mi.channelsEnabled = chEnMask;
    }
    // data size is same in mem and reg (typical case)
    void setHdcMessage(
        std::string msgSym,
        std::string msgDesc,
        SendOp op,
        int addrSize,
        int bitsPerElem,
        int elemsPerAddr,
        int execSize,
        int extraAttrs)
    {
        setHdcMessageX(
            msgSym,
            msgDesc,
            op,
            addrSize,
            bitsPerElem, bitsPerElem,
            elemsPerAddr,
            execSize,
            extraAttrs);
    }

    void setHdcOwBlock(
        std::string msgSym,
        std::string msgDesc,
        SendOp op,
        int addrSize,
        int extraAttrs)
    {
        auto owBits = getDescBits(8,3);
        // if there's only one OW, we pad up to 1 GRF (since it's LO or HI)
        // otherwise, we'll have 2, 4, ... all GRF multiples
        int regBlockSize = owBits < 2 ? 256 : 128;
        extraAttrs |= MessageInfo::TRANSPOSED;
        if (owBits == 1)
            extraAttrs |= MessageInfo::EXPAND_HIGH;

        int elems = addrSize == 64 ?
            decodeMDC_A64_DB_OW(8) :
            decodeMDC_DB_OW(8);
        std::stringstream ss;
        ss << msgDesc << " x" << elems;
        msgDesc = ss.str();

        setHdcMessageX(
            msgSym,
            msgDesc,
            op,
            addrSize,
            regBlockSize, 128,
            elems,
            1, // SIMD
            extraAttrs);
    }

    void setHdcHwBlock(
        std::string msgSym,
        std::string msgDesc,
        SendOp op,
        int addrSize,
        int blocksCountOffset,
        int blockCountLen,
        int extraAttrs)
    {
        extraAttrs |= MessageInfo::TRANSPOSED;

        int elems =
            addrSize == 64 ?
                decodeMDC_A64_DB_HW(blocksCountOffset) :
                decodeMDC_DB_HW(blocksCountOffset, blockCountLen);
        std::stringstream ss;
        ss << msgDesc << " x" << elems;
        msgDesc = ss.str();

        setHdcMessageX(
            msgSym,
            msgDesc,
            op,
            addrSize,
            256, 256,
            elems,
            1, // SIMD
            extraAttrs);
    }

    void setHdcUntypedSurfaceMessage(
        std::string msgDesc,
        bool isRead,
        int addrSizeBits,
        int extraAttrs)
    {
        std::string msgSym = isRead ? "untyped_load" : "untyped_store";
        extraAttrs |= MessageInfo::HAS_CHMASK;
        setHdcMessage(
            msgSym,
            msgDesc,
            isRead ? SendOp::LOAD_QUAD : SendOp::STORE_QUAD,
            addrSizeBits,
            32,
            decodeMDC_CMASK(),
            decodeMDC_SM3(12),
            extraAttrs);
    }

    void setHdcTypedSurfaceMessage(
        bool isRead,
        const char *doc,
        bool returnsStatus = false)
    {
        std::string msgDesc = isRead ?
            "typed surface read" : "typed surface write";
        addField("MessageType", 14, 5, getDescBits(14, 5), msgDesc);

        std::string msgSym = isRead ? "typed_load" : "typed_store";
        if (decodeMDC_SG3()) {
            msgDesc += " (high slot group)";
            msgSym += "_sgh";
        }
        if (returnsStatus) {
          msgDesc = " returning status";
          msgSym += "_wstatus";
        }

        setHdcMessage(
            msgSym,
            msgDesc,
            isRead ? SendOp::LOAD_QUAD : SendOp::STORE_QUAD,
            32, // addrSize
            32, // dataSize
            decodeMDC_CMASK(),
            DEFAULT_EXEC_SIZE/2,
            MessageInfo::HAS_CHMASK);
        setDoc(doc);
    }

    void setHdcFloatAtomicMessage(
        const char *msgNameDesc, int addrSize, int dataSize,
        const char *docNoRet, const char *docWiRet)
    {
        addField("MessageType", 14, 5, getDescBits(14, 5), msgNameDesc);

        std::string msgSym = "atomic_float?";
        std::string msgDesc;
        SendOp op = SendOp::INVALID;
        auto atBits = getDescBits(8, 3);
        switch (atBits) {
        case 0x1:
            op = SendOp::ATOMIC_FMAX;
            msgSym ="atomic_fmax";
            msgDesc = "max";
            break;
        case 0x2:
            op = SendOp::ATOMIC_FMIN;
            msgSym ="atomic_fmin";
            msgDesc = "min";
            break;
        case 0x3: op = SendOp::ATOMIC_FCAS;
            msgSym ="atomic_fcas";
            msgDesc = "fp-compare and swap ";
            break;
        default:
            error(8, 3, " (unknown float op)"); // fallthrough
        }
        std::stringstream ssDesc;
        ssDesc << msgNameDesc << " " << msgDesc << " (" << dataSize << "b)";
        addField("AtomicOp:MDC_AOP", 8, 3, atBits, ssDesc.str());

        int extraAttrs = 0;
        if (getDescBitField("ReturnDataControl", 13, "no return value",
            "returns new value"))
        {
            extraAttrs |= MessageInfo::ATOMIC_RETURNS;
            msgSym += "_ret";
            ssDesc << " with return";
            setDoc(docWiRet);
        }
        else
        {
            setDoc(docNoRet);
        }
        if (op != SendOp::INVALID) {
            setHdcMessage(
                msgSym,
                ssDesc.str(),
                op,
                addrSize,
                dataSize,
                1,
                decodeMDC_SM2R(12),
                extraAttrs);
        }
    }

    void setHdcIntAtomicMessage(
        const char *typedUntyped, // "typed" or "untyped"
        const char *msgDesc0,
        int addrSize,
        int dataSize,
        int simdSize,
        const char *docNoRet,
        const char *docWiRet)
    {
        std::stringstream msgSym;
        msgSym << typedUntyped << "_";
        std::stringstream msgDesc;
        msgDesc << typedUntyped << " " << msgDesc0;

        addField("MessageType", 14, 5, getDescBits(14, 5), msgDesc.str());

        std::string mOpName;
        SendOp op = SendOp::INVALID;
        auto aopBits = getDescBits(8,4);
        std::string opDesc;
        switch (aopBits) {
        // again with case 0x0 they wedged in a 64b CAS as part of the
        // 32b message (note there's also a QW atomic message)
        case 0x0: op =
            SendOp::ATOMIC_ICAS;
            mOpName = "atomic_icas";
            opDesc = "64b integer compare and swap";
            dataSize = 64;
            break;
        // The rest are 32b (or 16b)
        case 0x1:
            op = SendOp::ATOMIC_AND;
            mOpName = "atomic_and";
            opDesc = "logical AND";
            break;
        case 0x2:
            op = SendOp::ATOMIC_OR;
            mOpName = "atomic_or";
            opDesc = "logical OR";
            break;
        case 0x3:
            op = SendOp::ATOMIC_XOR;
            mOpName = "atomic_xor";
            opDesc = "logical XOR";
            break;
        case 0x4:
            op = SendOp::ATOMIC_STORE;
            mOpName = "atomic_store";
            opDesc = "store";
            break;
        case 0x5:
            op = SendOp::ATOMIC_IINC;
            mOpName = "atomic_iinc";
            opDesc = "integer increment";
            break;
        case 0x6:
            op = SendOp::ATOMIC_IDEC;
            mOpName = "atomic_idec";
            opDesc = "integer decrement";
            break;
        case 0xF:
            op = SendOp::ATOMIC_IPDEC;
            mOpName = "atomic_iipdec";
            opDesc = "integer pre-decrement (returns pre-decrement value)";
            break;
        case 0x7:
            op = SendOp::ATOMIC_IADD;
            mOpName = "atomic_iadd";
            opDesc = "integer add";
            break;
        case 0x8:
            op = SendOp::ATOMIC_ISUB;
            mOpName = "atomic_isub";
            opDesc = "integer subtract";
            break;
        case 0x9:
            op = SendOp::ATOMIC_IRSUB;
            mOpName = "atomic_irsub";
            opDesc = "commuted integer subtract";
            break;
        case 0xA:
            op = SendOp::ATOMIC_SMAX;
            mOpName = "atomic_smax";
            opDesc = "signed-integer max";
            break;
        case 0xB:
            op = SendOp::ATOMIC_SMIN;
            mOpName = "atomic_smin";
            opDesc = "signed-integer min";
            break;
        case 0xC:
            op = SendOp::ATOMIC_UMAX;
            mOpName = "atomic_umax";
            opDesc = "unsigned-integer max";
            break;
        case 0xD:
            op = SendOp::ATOMIC_UMIN;
            mOpName = "atomic_umin";
            opDesc = "unsigned-integer min";
            break;
        case 0xE:
            op = SendOp::ATOMIC_ICAS;
            mOpName = "atomic_icas";
            opDesc = "integer compare and swap (non-64b)";
            break;
        //
        default:
        {
            std::stringstream ss;
            ss << "0x" << std::uppercase << std::hex <<
                getDescBits(8,4) << "?";
            mOpName = ss.str();
            opDesc = mOpName;
            error(8, 4, " unknown int atomic op");
        }
        }
        msgSym << mOpName;
        //
        addField("AtomicIntegerOp", 8, 4, aopBits, opDesc);
        //
        int extraAttrs = 0;
        if (getDescBitField(
            "ReturnDataControl", 13, "no return value", "returns new value"))
        {
            extraAttrs |= MessageInfo::ATOMIC_RETURNS;
            msgSym << "_ret";
            setDoc(docWiRet);
        } else {
            setDoc(docNoRet);
        }
        if (op != SendOp::INVALID) {
            msgDesc << " " << opDesc;
            setHdcMessage(
                msgSym.str(),
                msgDesc.str(),
                op,
                addrSize,
                dataSize,
                1,
                simdSize,
                extraAttrs);
        }
    }

    // for miscellaneous stuff such as fences and whatnot
    //
    // treat the payloads as full register units and set the op to SIMD1
    void setSpecialOp(
        std::string msgSym,
        std::string msgDesc,
        SendOp op,
        AddrType addrType,
        uint32_t surfaceId,
        int mlen,
        int rlen,
        int extraAttrs = 0)
    {
        mi.symbol = msgSym;
        mi.description = msgDesc;
        mi.op = op;
        mi.cachingL1 = CacheOpt::DEFAULT;
        mi.cachingL3 = CacheOpt::DEFAULT;
        mi.addrType = addrType;
        mi.surface = surfaceId;
        mi.addrSizeBits = mlen*BITS_PER_REGISTER;
        // e.g. SIMD16 platforms are 256b (two full registers)
        mi.elemSizeBitsRegFile = rlen*BITS_PER_REGISTER;
        mi.elemSizeBitsMemory = mi.elemSizeBitsRegFile;
        mi.channelsEnabled = 0;
        mi.elemsPerAddr = 1;
        mi.execWidth = 1;
        mi.attributeSet = extraAttrs | MessageInfo::VALID;
    }
    void setSpecialOp(
        std::string msgSym,
        std::string msgDesc,
        SendOp op,
        AddrType addrType,
        uint32_t surfaceId,
        int extraAttrs = 0)
    {
        setSpecialOp(
            msgSym,
            msgDesc,
            op,
            addrType,
            surfaceId,
            (int)getDescBits(25,4),
            (int)getDescBits(20,5),
            extraAttrs);
    }


    ///////////////////////////////////////////////////////////////////////////
    void tryDecode();
    void tryDecodeDCRO();
    void tryDecodeDC0();
    void tryDecodeDC1();
    void tryDecodeURB();
    void tryDecodeGTWY();
    void tryDecodeRC();
    void tryDecodeSampler();
}; // class DescDecoder


iga::MessageInfo iga::MessageInfo::tryDecode(
    Platform platform,
    SFID sfid,
    uint32_t exDesc,
    uint32_t desc,
    DiagnosticList &warnings,
    DiagnosticList &errors,
    DecodedDescFields *descDecodedField)
{
    DescDecoder dd(platform, sfid, exDesc, desc, warnings, errors);
    dd.tryDecode();
    if (errors.empty())
        dd.mi.attributeSet |= MessageInfo::VALID;
    if (descDecodedField) {
        std::sort(dd.fields.begin(), dd.fields.end(),
            [&] (const auto &f1, const auto &f2) {
                return std::get<0>(f1).offset >
                    std::get<0>(f2).offset;
            });
        *descDecodedField = dd.fields;
        //
        // make sure there aren't unmapped bits
        // (as this decoder solidifies we can convert this to a warning)
        // dd.warning("bit set in reserved field");
        int len = dd.hasExDesc() ? 64 : 32;
        for (int i = 0; i < len; i++) {
            auto bit = i >= 32 ? (exDesc & (1<<(i-32))) : (desc & (1<<(i)));
            if (bit) {
                bool bitMapped = false;
                for (const auto &fv : dd.fields) {
                    const auto &f = std::get<0>(fv);
                    if (i >= f.offset && i < f.offset + f.length) {
                        bitMapped = true;
                        break;
                    }
                }
                if (!bitMapped) {
                    // uncomment for debugging
                    // std::stringstream ss;
                    // ss << "[" << i << "] = 1 not mapped by field";
                    // IGA_ASSERT_FALSE(ss.str().c_str());
                    dd.warning(i, 1, "bits set in undefined field");
                }
            }
        }
    }
    return dd.mi;
}

void DescDecoder::tryDecodeDCRO() {
    const int msgType = getDescBitsField("MType", 14, 5,
        [&] (std::stringstream &ss, uint32_t op) {
            switch (op) {
            case 0x00: ss << "constant oword block read"; break;
            case 0x01: ss << "constant unaligned oword block read"; break;
            case 0x03: ss << "constant dword gathering read"; break;
            default: ss << "?"; break;
            }
        });
    switch (msgType) {
    case 0x00: // constant           oword block read
    case 0x01: // constant unaligned oword block read
        setHdcOwBlock(
            msgType == 0x00 ?
              "const_load_block" :
              "unaligned_const_load_block",
            msgType == 0x00 ?
              "constant oword block read" :
              "constant unaligned oword block read",
            SendOp::LOAD,
            32, // 32b address
            0);
        mi.cachingL3 = decodeMDC_IAR();
        decodeMDC_HR();
        setDoc(msgType == 0x00 ? "7041" : "7043");
        break;
    case 0x03: // constant dword gathering read
        setHdcMessage(
            "const_load",
            "constant dword gathering read",
            SendOp::LOAD,
            32,
            32,
            decodeMDC_DWS_DS(10),
            decodeMDC_SM2(8),
            0);
        mi.cachingL3 = decodeMDC_IAR();
        decodeExpected(9, 1, "LegacySimdMode", 1);
        decodeMDC_H();
        setDoc("7084");
        break;
    default:
        error(14, 5, "unsupported DCRO op");
        return;
    }
}

void DescDecoder::tryDecodeDC0()
{
    const int msgType = getDescBits(14,5);
    switch (msgType) {
    case 0x00: // oword block read
        addField("MessageType",14,5,msgType,"oword block read");
        setHdcOwBlock(
            "load_block",
            "oword block read",
            SendOp::LOAD,
            32, // all 32b addresses
            0);
        setDoc("7028");
        decodeMDC_HR();
        break;
    case 0x01: // aligned hword/oword block read
        addField("MessageType",14,5,msgType,"aligned oword block read");
        setHdcOwBlock(
            "aligned_load_block",
            "aligned oword block read",
            SendOp::LOAD,
            32, // all 32b addresses
            0);
        setDoc("7030");
        decodeMDC_HR();
        break;
    case 0x08: // hword/oword block write
        addField("MessageType",14,5,msgType,"oword block write");
        setHdcOwBlock(
            "store_block",
            "oword block write",
            SendOp::STORE,
            32, // all 32b addresses
            0);
        setDoc("7032");
        decodeMDC_HR();
        break;
    case 0x09: // hword aligned block write
        addField("MessageType",14,5,msgType,"hword aligned block write");
        setHdcOwBlock(
            "aligned_store_block",
            "aligned oword block write",
            SendOp::STORE,
            32, // all 32b addresses
            0);
        setDoc("20862");
        decodeMDC_HR();
        break;
    //
    case 0x02: // oword dual block read
        addField("MessageType",14,5,msgType,"oword dual block read");
        mi.description = "oword dual block read";
        setDoc("7029");
        decodeMDC_HR();
        error(14,5, "oword dual block read decode not supported");
        return;
    case 0x0A: // oword dual block write
        addField("MessageType",14,5,msgType,"oword dual block write");
        mi.description = "oword dual block write";
        setDoc("7033");
        decodeMDC_HR();
        error(14,5,"oword dual block write decode not supported");
        return;
    //
    case 0x04: // byte scattered read (should be "gathering")
    case 0x0C: // byte scattered write
    {
        const char *msgName = msgType == 0x04 ?
            "byte gathering read" : "byte scattering write";
        addField("MessageType",14,5,msgType,msgName);
        //
        // "byte" scattered always consumes a DW of GRF per channel,
        // but DWS_DS controls how many bytes are loaded per address
        // that might be 1, 2, 4 all packed into one DW.
        // So think of:
        //     DWS_DS == 0 (byte) as u8 zext to u32
        //     DWS_DS == 1 (word) as u16 zext to u32
        //     DWS_DS == 2 (dword) as u32 zext to u32
        setHdcMessageX(
            msgType == 0x04 ? "load" : "store",
            msgType == 0x04 ?
                "byte gathering read" : "byte scattering write",
            msgType == 0x04 ? SendOp::LOAD : SendOp::STORE,
            32, // 32b addrs
            32, // each channel occupies a DW in the reg file
            8*decodeMDC_DS(10), // in memory it can be 1, 2, or 4 bytes
            1, // vector size always 1
            decodeMDC_SM2(8),
            0);
        mi.cachingL3 = decodeMDC_IAR();
        setDoc(msgType == 0x04 ? "7066" : "7068");
        decodeMDC_H();
        break;
    }
    //
    case 0x03: // dword scattered read
    case 0x0B: // dword scattered write
    {
        const char *msgName = msgType == 0x03 ?
            "dword gathering read" : "dword scattering write";
        addField("MessageType", 14, 5, msgType, msgName);
        //
        setHdcMessage(
            msgType == 0x03 ? "load" : "store",
            msgName,
            msgType == 0x03 ? SendOp::LOAD : SendOp::STORE,
            32,
            32,
            decodeMDC_DWS_DS(10),
            decodeMDC_SM2(8),
            0
          );
        mi.cachingL3 = decodeMDC_IAR();
        setDoc(msgType == 0x03 ? "7067" : "7069");
        decodeExpected(9, 1, "LegacySimdMode", 1);
        decodeMDC_H();
        break;
    }
    case 0x07: {
        // memory fence
        addField("MessageType", 14, 5, msgType, "fence");
        //
        std::stringstream sym, desc;
        uint32_t surfId = getDescBits(0,8);
        int extraAttrs = 0;
        if (getDescBitField("Commit",13,
            "off (return immediately)",
            "on (wait for fence commit)"))
        {
            sym << "sync_";
            desc << "synchronized ";
        }
        if (surfId == SLM_BTI) {
            (void)decodeBTI(32); // add the field
            sym << "slm_fence";
            desc << "SLM fence";
            extraAttrs |= MessageInfo::SLM;
        } else if (surfId == 0) {
            sym << "global_fence";
            desc << "global fence";
            if (getDescBits(9,4) && getDescBit(8)) {
                error(8,1,"L3 implies L1 flush");
            }
            desc << " flushing";
            if (getDescBitField("L1Flush",8,"Flush L3","FLush L1") != 0) {
                sym << ".l1";
                desc << " L1";
            }
            if (getDescBitsField("L3 Flush Targets",9,4)) {
                if (getDescBits(9,4) == 0xF) {
                    desc << " all L3 data";
                    sym << ".dcti";
                } else {
                    int n = 0;
                    desc << " L3";
                    sym << ".";
                    if (getDescBit(12)) {
                        sym << "d";
                        desc << " r/w data";
                    }
                    if (getDescBit(11)) {
                        if (n++ > 0)
                            desc << ",";
                        sym << "c";
                        desc << " constant data";
                    }
                    if (getDescBit(10)) {
                        if (n++ > 0)
                            desc << ",";
                        sym << "t";
                        desc << " texture data";
                    }
                    if (getDescBit(9)) {
                        if (n++ > 0)
                            desc << ",";
                        sym << "i";
                        desc << " instruction data";
                    }
                }
            }
        } else {
            error(0, 8, "invalid BTI for fence (must be 0x0 or 0xFE)");
        }
        setSpecialOp(
            sym.str(),
            desc.str(),
            SendOp::FENCE,
            AddrType::FLAT,
            surfId,
            extraAttrs);
        setDoc("7049");
        decodeMDC_HR();
        break;
    }
    default:
        if (getDescBit(18) == 1) {
            // scratch read
            // scratch write
            //
            // scratch block is a muddy situation
            // they used to have ChannelMode at 16, so we want to ignore that
            // and only use bits
            bool isRead = getDescBit(17) == 0;
            const char *msgName = isRead ?
                "hword scratch block read" :  "hword scratch block write";
            addField("MessageType", 14, 5, msgType, msgName);
            //
            setHdcHwBlock(
                isRead ? "load_block" : "store_block",
                msgName,
                isRead ? SendOp::LOAD : SendOp::STORE,
                32, // r0.5
                12, 2, // [13:12] num HWs
                MessageInfo::SCRATCH);
            // scratch offset [11:0] (reg aligned)
            mi.immediateOffset = 32*
                getDescBitsField("HWordOffset", 0, 12,
                    [&](std::stringstream &ss, uint32_t val) {
                        ss << val << " HWords from scratch base";
                    });
            if (platform < Platform::GEN10) {
                getDescBitField("ChannelMode", 16, "OWord", "DWord");
            }
            setDoc(isRead ? "7027" : "7031");
            decodeMDC_HR();
        } else {
           addField("MessageType", 14, 5, msgType, "???");
           error(14, 5, "unsupported dc0 op");
           return;
        }
    } // end switch legacy DC0
}

void DescDecoder::tryDecodeDC1() {
    const int msgType = getDescBits(14,5);
    switch (msgType)
    {
    case 0x01: // untyped surface read
    case 0x09: // untyped surface write
    {
        const char *msgName = msgType == 0x1 ?
                "untyped surface read" : "untyped surface write";
        addField("MessageType", 14, 5, msgType, msgName);
        //
        setHdcUntypedSurfaceMessage(
            msgName,
            msgType == 0x1,
            32,
            0);
        setDoc(msgType == 0x11 ? "7088" : "7091");
        decodeMDC_H();
        break;
    }
    case 0x11: // a64 untype surface read
    case 0x19: // a64 untype surface write
    {
        const char *msgName = msgType == 0x1 ?
                "a64 untyped surface read" : "a64 untyped surface write";
        addField("MessageType",14,5,msgType,msgName);
        //
        setHdcUntypedSurfaceMessage(
            msgName,
            msgType == 0x11,
            64, // 8B addrs
            0);
        setDoc(msgType == 0x11 ? "7086" : "7089");
        decodeMDC_HF();
        break;
    }
    case 0x10: // a64 gathering read (byte/dw/qw)
    case 0x1A: // a64 scattered write (byte/dw/qw)
    {
        // 0 is byte-scattered, the others (DW/QW) are true SIMT
        int subType = getDescBits(8,2);
        if (subType == 0x0 || subType == 0x3) {
            // c.f. handling above with non-A64 version of BSR
            const char *msgName = msgType == 0x10 ?
                "a64 byte gathering read" : "a64 byte scattering write";
            addField("MessageType",14,5,msgType,msgName);
            if (subType == 0x0) {
                addField("SubType",8,2,subType,"Byte");
                setDoc(msgType == 0x10 ? "7070" : "7073");
            } else { // subType == 0x3
                addField("SubType",8,2,subType,"Byte with Status Return");
                setDoc(msgType == 0x10 ? "19316" : nullptr);
                if (msgType == 0x1A)
                    error(14,5,
                        "a64 byte scattering with status return message");
            }
            //
            setHdcMessageX(
                msgType == 0x10 ? "load" : "store",
                msgName,
                msgType == 0x10 ? SendOp::LOAD : SendOp::STORE,
                64, // A64
                32, // widens to DW (similar to non-A64 version)
                8*MDC_A64_DS(10), // bits from memory
                1, //
                decodeMDC_SM2(12),
                0);
        } else {
            // unlike non-A64 version, this variant supports DW and QW
            // in the same message type, the MDC_A64_DS is treated as a
            // vector length
            const char *msgTypeName = msgType == 0x10 ?
              "a64 gathering read" : "a64 scattering write";
             addField("MessageType",14,5,msgType,msgTypeName);
            //
            const char *msgName =
                subType == 1 ?
                    msgType == 0x10 ?
                        "a64 dword gathering read" :
                        "a64 dword scattering write" :
                    msgType == 0x10 ?
                        "a64 qword gathering read" :
                        "a64 qword scattering write";
            //
            addField("SubType",8,2,subType,subType == 1 ? "DWord" : "QWord");
            //
            setHdcMessage(
                msgType == 0x10 ? "load" : "store",
               msgName,
                msgType == 0x10 ? SendOp::LOAD : SendOp::STORE,
                64,
                subType == 1 ? 32 : 64,
                decodeMDC_DWS_DS(10), // true vector
                decodeMDC_SM2(12),
                0);
            if (subType == 0x1) {
                setDoc(msgType == 0x10 ? "7071" : "7074");
            } else if (subType == 0x2) {
                setDoc(msgType == 0x10 ? "7072" : "7075");
            }
        }
        mi.cachingL3 = decodeMDC_IAR();
        decodeMDC_HF();
        break;
    }
    case 0x14: // a64 (unaligned,hword|oword) block read
    case 0x15: // a64 (unaligned,hword|oword) block write
    {
        addField("MessageType",14,5,msgType,
            msgType == 0x14 ? "a64 block read" :"a64 block write");
        bool isHword = false;
        bool isUnaligned = false;
        auto subType = getDescBitsField("SubType",11,2,
            [&] (std::stringstream &ss, uint32_t val) {
                switch (val) {
                case 0x0:
                    isUnaligned = true;
                    ss << "oword unaligned";
                    break;
                case 0x1:
                    ss << "oword aligned";
                    break;
                case 0x3:
                    isHword = true;
                    isUnaligned = true;
                    ss << "hword unaligned";
                    break;
                  //
                default:
                    ss << "dual block";
                    isUnaligned = true;
                    error(11,2,"a64 dual block read/write unsupported");
                    break;
                }
            });

        if (isHword && isUnaligned)
            setDoc(msgType == 0x14 ? "7034" : "7038"); // MSD1W_A64_HWB
        else if (isHword && !isUnaligned) {
            bool supported = false;
            if (!supported)
                error(11,2,"HWord aligned unsupported on this platform");
        } else if (!isHword && isUnaligned) {
            setDoc(msgType == 0x14 ? "7037" : "33440"); // MSD1W_A64_OWAB
        } else if (!isHword && !isUnaligned) {
            setDoc(msgType == 0x14 ? "7039" : "7039"); // MSD1W_A64_OWB
        } else if (subType == 2) {
            setDoc(msgType == 0x14 ? "7036" : "7040");
            mi.description = "a64 dual block ";
            mi.description += (msgType == 0x14 ? " read" : " write");
            mi.symbol = msgType == 0x14 ? "MSD1R_A64_OWDB" : "MSD1W_A64_OWDB";
            mi.addrSizeBits = 64;
            decodeMDC_HR();
            break; // dual block (error)
        }
        //
        std::string msgSym = msgType == 0x14 ? "load_block" : "store_block";
        std::string msgDesc = "a64";
        if (isUnaligned) {
            msgDesc += " unaligned";
        } else {
            msgDesc += " aligned";
            msgSym += "_aligned";
        }
        if (isHword) {
            msgDesc += " hword";
        } else {
            msgDesc += " oword";
        }
        msgDesc += " block";
        if (msgType == 0x14) {
            msgDesc += " read";
        } else {
            msgDesc += " write";
        }
        if (isHword) {
            setHdcHwBlock(
                msgSym,
                msgDesc,
                msgType == 0x14 ? SendOp::LOAD : SendOp::STORE,
                64, // 64b addr
                8, 3, // offset of HWs
                0);
        } else {
            setHdcOwBlock(
                msgSym,
                msgDesc,
                msgType == 0x14 ? SendOp::LOAD : SendOp::STORE,
                64, // 64b addr
                0);
        }
        mi.cachingL3 = decodeMDC_IAR();
        decodeMDC_HR(); // all block require a header
        break;
    }
    case 0x1D: // a64 untyped fp32 atomic
        //
        setHdcFloatAtomicMessage(
            "a64 float atomic",
            64,
            32,
            "7126",
            "7118");
        decodeMDC_HF();
        break;

    case 0x1B: // a32 untyped fp32 atomic
        //
        setHdcFloatAtomicMessage(
            "float atomic",
            32,
            32,
            "7130",
            "7122");
        decodeMDC_H();
        break;
    case 0x12: // a64 untyped atomic int{32,64}
    {
        // the encoding repurposes the SIMD size as the data size and the
        // message and thus the SIMD size is always fixed
        int simd = 8;
        const char *msgName = "a64 untyped atomic int32";
        auto dataWidthCtrl =
            getDescBitField("DataWidth", 12, "32b", "64b");
        //
        setHdcIntAtomicMessage(
            "untyped",
            msgName,
            64,
            dataWidthCtrl ? 64 : 32,
            simd,
            dataWidthCtrl ? "7161" : "7155",
            dataWidthCtrl ? "7143" : "7137");
        decodeMDC_HF();
        break;
    }
    case 0x02: // atomic int32
    {
        const char *msgName = "untyped atomic int32";
        addField("MessageType", 14, 5, msgType, msgName);
        //
        setHdcIntAtomicMessage(
            "untyped",
            "untyped atomic int32",
            32,
            32,
            decodeMDC_SM2R(12),
            "7167",
            "7149");
        decodeMDC_H();
        break;
    }
    case 0x04: // media block read
    case 0x0A: // media block write
    {
        std::stringstream sym, desc;
        sym << "mb";
        desc << "media block ";
        int bytesTransmitted = 0;
        if (msgType == 0x04) {
            desc << "read";
            sym << "rd";
            bytesTransmitted = 2*DEFAULT_EXEC_SIZE*getDescBits(20,5);
        } else {
            desc << "write";
            sym << "wr";
            int mlen = getDescBits(25,4);
            if (mlen == 0) {
                error(25,4,"mlen == 0 on write");
            } else {
                bytesTransmitted = 2*DEFAULT_EXEC_SIZE*(mlen - 1);
            }
        }
        int vlso = getDescBits(8,3); // [10:8] is vert. line stride overr.
        if (vlso) {
            desc << " with vertical line stride ";
            int n = 0;
            if (vlso & 0x2) {
                desc << "override";
                n++;
            }
            if (vlso & 0x1) {
                if ((vlso & 0x2) == 0)
                    desc << " and ";
                else if (n > 0)
                    desc << ",";
                desc << "skip";
                n++;
            }
            if (vlso & 0x2) {
                if (n >= 2)
                    desc << ", and ";
                else if (n > 0)
                    desc << " and ";
                desc << "offset";
                n++;
                if ((vlso & 0) == 0) {
                    warning(8,3,
                        "stride offset meaningless when override not set");
                }
            }
        }
        setHdcMessage(sym.str(), desc.str(),
            msgType == 0x4 ? SendOp::LOAD : SendOp::STORE,
            32, 8, bytesTransmitted, 1,
            MessageInfo::TRANSPOSED);
        setDoc(msgType == 0x04 ? "7046" : "7048");
        decodeMDC_HR();
        break;
    }
    case 0x05: // typed surface read
        setHdcTypedSurfaceMessage(true, "7087");
        decodeMDC_H();
        break;
    case 0x0D: // typed surface write
        setHdcTypedSurfaceMessage(false, "7090");
        decodeMDC_H();
        break;
    case 0x0B:
        setHdcIntAtomicMessage(
            "typed",
            "atomic 32-bit counter",
            32, // addrSize
            32, // dataSize
            decodeMDC_SM2R(12),
            "7109","7099");
        decodeMDC_HR();
        break;
    case 0x06:
    {
        bool sgh = decodeMDC_SG2();
        setHdcIntAtomicMessage(
            sgh ?
                "typed_sgh" :
                "typed" ,
            sgh ?
                "atomic 32-bit integer (slot group high)" :
                "atomic 32-bit integer",
            32, // addrSize
            32, // dataSize
            DEFAULT_EXEC_SIZE/2,
            "7113", "7103");
        decodeMDC_H();
        break;
    }
    default:
        error(14, 5, "unsupported DC1 op");
        return;
    } // DC1 switch
}

void DescDecoder::tryDecodeURB() {
    std::stringstream sym, desc;
    SendOp op;
    int simd = 8;
    int addrSize = 32;
    int dataSize = 32; // MH_URB_HANDLE has this an array of MHC_URB_HANDLE
    auto opBits = getDescBits(0,4); // [3:0]
    auto chMaskPresent = getDescBit(15) != 0; // [15]
    auto decodeGUO =
        [&]() {
            return getDescBitsField("GlobalUrbOffset", 4, 11,
                [&](std::stringstream &ss, uint32_t val) {
                    ss << val << " (in owords)";
                }); // [14:4]
        };
    auto decodePSO =
        [&]() {
            return getDescBitField(
                "PerSlotOffsetPresent", 17, "per-slot offset in payload") != 0;
        };
    int rlen = getDescBits(20,5);
    // int mlen = getDescBits(25,4);
    int xlen = getDescBits(32+6,5);
    int elemsPerAddr = 1;
    int off = 0;
    switch (opBits) {
    case 7:
        op = SendOp::STORE;
        off = 8*decodeGUO();
        addField("ChannelMaskEnable", 15, 1, getDescBit(15),
            chMaskPresent ? "enabled" : "disabled");
        sym << "MSDUW_DWS";
        desc << "urb dword ";
        if (chMaskPresent)
            desc << "masked ";
        desc << "write";
        if (decodePSO())
            desc << " with per-slot offset enabled";
        elemsPerAddr = xlen != 0 ? xlen : 1; // "SIMD8 URB Dword Read message. Reads 1..8 Dwords, based on RLEN."
        setDoc(chMaskPresent ? "44779" : "44778");
        decodeMDC_HR();
        break;
    case 8:
        op = SendOp::LOAD;
        off = 8*decodeGUO();
        sym << "MSDUR_DWS";
        desc << "urb dword read";
        if (decodePSO())
            desc << " with per-slot offset enabled";
        elemsPerAddr = rlen;
        setDoc("44777");
        decodeMDC_HR();
        break;
    default:
        error(0, 4, "unsupported URB op");
        return;
    }
    addField("URBOpcode", 0, 4, opBits, sym.str() + " (" + desc.str() + ")");

    setScatterGatherOp(
        sym.str(),
        desc.str(),
        op,
        AddrType::FLAT,
        0,
        addrSize,
        dataSize,
        elemsPerAddr,
        simd,
        0);
    if (opBits == 7 || opBits == 8)
        mi.immediateOffset = off;
}

void DescDecoder::tryDecodeGTWY() {
    std::stringstream sym, desc;
    int opBits = getDescBits(0,3); // [2:0]
    int expectMlen = 0;
    SendOp sendOp = SendOp::INVALID;
    switch (opBits) {
    case 1:
        sym << "signal";
        desc << "signal event";
        expectMlen = 1; //
        sendOp = SendOp::SIGNAL;
        break;
    case 2:
        sym << "monitor";
        desc << "monitor event";
        expectMlen = 1; // C.f. MDP_EVENT
        sendOp = SendOp::MONITOR;
        break;
    case 3:
        sym << "unmonitor";
        desc << "unmonitor event";
        expectMlen = 1; // C.f. MDP_NO_EVENT
        sendOp = SendOp::UNMONITOR;
        break;
    case 4:
        sym << "barrier";
        desc << "barrier";
        expectMlen = 1; // C.f. MDP_Barrier
        sendOp = SendOp::BARRIER;
        break;
    case 6:
        sym << "wait";
        desc << "wait for event";
        sendOp = SendOp::WAIT;
        expectMlen = 1; // C.f. MDP_Timeout
        break;
    default:
        error(0,2,"unsupported GTWY op");
    }
    addField("GatewayOpcode", 0, 3, opBits, desc.str());
    setSpecialOp(
        sym.str(), desc.str(), sendOp,
        AddrType::FLAT, 0,
        1, // mlen
        0, // rlen
        0);
    decodeMDC_HF(); // all gateway messages forbid a header
}


void DescDecoder::tryDecodeRC()
{
    // PSEUDO syntax
    //   rtw.{f16,f32}.{simd8,simd16,rep16,lo8ds,hi8ds}{.c}{.ps}{.lrts}{.sgh}
    //   rtr.{f32}.{simd8,simd16}{.ps}{.sgh}
    std::stringstream sym;

    static const uint32_t RT_READ = 0xD;
    static const uint32_t RT_WRITE = 0xC;

    std::string descSfx;
    sym << "rt";
    auto mt = getDescBits(14, 4);
    switch (mt) {
    case RT_WRITE:
        sym << "w";
        descSfx = "render target write";
        break;
    case RT_READ:
        sym << "r";
        descSfx = "render target read";
        break;
    default:
        descSfx = "unknown render target op";
        error(14,4,"unsupported RC op");
    }
    addField("MessageTypeRC", 14, 4, mt, descSfx);

    std::stringstream desc;
    int bitSize = getDescBitField("DataSize",30,"FP32","FP16") == 0 ? 32 : 16;
    if (bitSize == 32) {
        desc << "full-precision";
        sym << ".f32";
    } else {
        desc << "half-precision";
        if (mt == 0xD)
            warning(30,1,"half-precision not supported on render target read");
        sym << ".f16";
    }
    desc << " " << descSfx;

    std::string subopSym;
    auto subOpBits = getDescBits(8,3);
    int execSize = 0;
    switch (mt) {
    case RT_WRITE:
        switch (subOpBits) {
        case 0x0:
            subopSym = ".simd16";
            desc << " SIMD16";
            execSize = 16;
            break;
        case 0x1:
            subopSym = ".rep16";
            desc << " replicated SIMD16";
            execSize = 16;
            break;
        case 0x2:
            subopSym = ".lo8ds";
            desc << " of low SIMD8";
            execSize = 8;
            break;
        case 0x3:
            subopSym = ".hi8ds";
            desc << " of high SIMD8";
            execSize = 8;
            break;
        case 0x4:
            subopSym = ".simd8";
            desc << " SIMD8";
            execSize = 8;
            break;
        default:
            sym << ".???";
            desc << "unknown write subop";
            error(8,3,"unknown write subop");
            break;
        }
        break;
    case RT_READ:
        switch (subOpBits) {
        case 0x0: subopSym = ".simd16"; desc << " SIMD16"; execSize = 16; break;
        case 0x1: subopSym = ".simd8"; desc << " SIMD8"; execSize = 8; break;
        default:
            sym << ".???";
            desc << "unknown read subop";
            error(8,3,"unknown read subop");
            break;
        }
        break;
    default:
        break;
    }
    addField("Subop",8,3,subOpBits,subopSym);
    sym << subopSym;

    if (mt == RT_WRITE) {
        auto pc =
            getDescBitField("PerCoarsePixelPSOutputs",18,
                "disabled","enabled");
        if (pc) {
            desc << " with Per-Coarse Pixel PS outputs enable";
            sym << ".cpo";
        }
    }
    auto ps =
        getDescBitField("PerSamplePS",13,
            "disabled","enabled");
    if (ps) {
        desc << " with Per-Sample Pixel PS outputs enable";
        sym << ".psp";
    }

    if (mt == RT_WRITE) {
        auto lrts =
            getDescBitField("LastRenderTargetSelect",12,
                "false","true");
        if (lrts) {
            desc << "; last render target";
            sym << ".lrts";
        }
    }

    auto sgs =
        getDescBitField("SlotGroupSelect",11,
            "SLOTGRP_LO","SLOTGRP_HI");
    if (sgs) {
        desc << " slot group high";
        sym << ".sgh";
    }

    uint32_t surfaceIndex = 0;
    getDescBitsField("BTS", 0, 8, [&] (std::stringstream &ss, uint32_t value) {
        surfaceIndex = value;
        ss << "surface " << value;
        desc << " to surface " << value;
        sym << ".bti[" << value << "]";
    });

    mi.symbol = sym.str();
    mi.description = desc.str();
    mi.op = mt == RT_READ ? SendOp::RENDER_READ : SendOp::RENDER_WRITE;
    mi.execWidth = execSize;
    mi.elemSizeBitsMemory = mi.elemSizeBitsRegFile = bitSize;
    mi.addrSizeBits = 0;
    mi.surface = surfaceIndex;
    mi.attributeSet = 0;
    decodeMDC_H2(); // all render target messages permit a dual-header
}

enum SamplerSIMD {
    SIMD8,
    SIMD16,
    SIMD32_64,
    SIMD8H,
    SIMD16H,
};

void DescDecoder::tryDecodeSampler()
{
    SamplerSIMD simd;
    auto simd2 = getDescBitField("SIMD[2]",29,"","");
    auto simd01 = getDescBits(17,2);
    uint32_t simdBits = simd01|(simd2<<2);
    setDoc("12484");
    std::stringstream sym, desc;
    int simdSize = 0;

    switch (simdBits) {
    case 0:
        error(17, 2, "invalid sampler SIMD mode");
        return;
    case 1:
        simd = SIMD8;
        simdSize = 8;
        desc << "simd8";
        sym << "simd8";
        break;
    case 2:
        simd = SIMD16;
        simdSize = 16;
        desc << "simd16";
        sym << "simd16";
        break;
    case 3:
        simd = SIMD32_64;
        desc << "simd32/64";
        sym << "simd32";
        simdSize = 32;
        break;
    case 4:
        error(17,2,"invalid SIMD mode");
        return;
    case 5:
        simd = SIMD8H;
        simdSize = 8;
        sym << "simd8h";
        desc << "simd8 high";
        break;
    case 6:
        simd = SIMD16H;
        sym << "simd16h";
        desc << "simd16 high";
        simdSize = 16;
        break;
    default:
        error(17,2,"invalid SIMD mode");
        return;
    } // switch
    addField("SIMD[1:0]",17,2,simd01,desc.str());

    bool is16bData = getDescBitField("ReturnFormat",30,"32b","16b") != 0;
    if (is16bData) {
        sym << "_16";
        desc << " 16b";
    }
    sym << "_";
    desc << " ";

    SendOp sendOp = SendOp::SAMPLER_LOAD;
    int params = 0;
    const char *messageDesc = "";
    auto opBits = getDescBits(12,5);
    if (simd != SIMD32_64) {
        switch (opBits) {
        case 0x00:
            sym << "sample";
            messageDesc = "sample";
            params = 4;
            break;
        case 0x01:
            sym << "sample_b";
            messageDesc = "sample+LOD bias";
            params = 5;
            break;
        case 0x02:
            sym << "sample_l";
            messageDesc = "sample override LOD";
            params = 5;
            break;
        case 0x03:
            sym << "sample_c";
            messageDesc = "sample compare";
            params = 5;
            break;
        case 0x04:
            sym << "sample_d";
            messageDesc = "sample gradient";
            params = 10;
            break;
        case 0x05:
            sym << "sample_b_c";
            messageDesc = "sample compare+LOD bias";
            params = 6;
            break;
        case 0x06:
            sym << "sample_l_c";
            messageDesc = "sample compare+override LOD";
            params = 6;
            break;
        case 0x07:
            sym << "sample_ld";
            messageDesc = "sample load";
            params = 4;
            break;
        case 0x08:
            sym << "sample_gather4";
            messageDesc = "sample gather4";
            params = 4;
            break;
        case 0x09:
            sym << "sample_lod";
            messageDesc = "sample override lod";
            params = 4;
            break;
        case 0x0A:
            sym << "sample_resinfo";
            messageDesc = "sample res info";
            params = 1;
            break;
        case 0x0B:
            sym << "sample_info";
            messageDesc = "sample info";
            params = 0;
            break;
        case 0x0C:
            sym << "sample_killpix";
            messageDesc = "sample+killpix";
            params = 3;
            break;
        case 0x10:
            sym << "sample_gather4_c";
            messageDesc = "sample gather4+compare";
            params = 5;
            break;
        case 0x11:
            sym << "sample_gather4_po";
            messageDesc = "sample gather4+pixel offset";
            params = 5;
            break;
        case 0x12:
            sym << "sample_gather4_po_c";
            messageDesc = "sample gather4 pixel offset+compare";
            params = 6;
            break;
        // case 0x13: //skipped
        case 0x14:
            sym << "sample_d_c";
            messageDesc = "sample derivatives+compare";
            params = 11;
            break;
        case 0x16:
            sym << "sample_min";
            messageDesc = "sample min";
            params = 2;
            break;
        case 0x17:
            sym << "sample_max";
            messageDesc = "sample max";
            params = 2;
            break;
        case 0x18:
            sym << "sample_lz";
            messageDesc = "sample with lod forced to 0";
            params = 4;
            break;
        case 0x19:
            sym << "sample_c_lz";
            messageDesc = "sample compare+with lod forced to 0";
            params = 5;
            break;
        case 0x1A:
            sym << "sample_ld_lz";
            messageDesc = "sample load with lod forced to 0";
            params = 3;
            break;
        // case 0x1B:
        case 0x1C:
            sym << "sample_ld2dms_w";
            messageDesc = "sample ld2 multi-sample wide";
            params = 7;
            break;
        case 0x1D:
            sym << "sample_ld_mcs";
            messageDesc = "sample load mcs auxilary data";
            params = 4;
            break;
        case 0x1E:
            sym << "sample_ld2dms";
            messageDesc = "sample load multi-sample";
            params = 6;
            break;
        case 0x1F:
            sym << "sample_ld2ds";
            messageDesc = "sample multi-sample without mcs";
            params = 6;
            break;
        default:
            sym << "sample_" << std::hex << std::uppercase << opBits << "?";
            messageDesc = "?";
            break;
        }
    } else {
        switch (opBits) {
        case 0x00:
            sym << "sample_unorm";
            messageDesc = "sample unorm";
            params = 4; // relatively sure
            break;
        case 0x02:
            sym << "sample_unorm_killpix";
            messageDesc = "sample unorm+killpix";
            params = 4; // no idea???
            break;
        case 0x08:
            sym << "sample_deinterlace";
            messageDesc = "sample deinterlace";
            params = 4; // no idea???
            break;
        case 0x0C:
            // yes: this appears to be replicated
            sym << "sample_unorm_media";
            messageDesc = "sample unorm for media";
            params = 4; // no idea???
            break;
        case 0x0A:
            // yes: this appears to be replicated
            sym << "sample_unorm_killpix_media";
            messageDesc = "sample unorm+killpix for media";
            params = 4; // no idea???
            break;
        case 0x0B:
            sym << "sample_8x8";
            messageDesc = "sample 8x8";
            params = 4; // no idea???
            break;
        case 0x1F:
            sym << "sample_flush";
            messageDesc = "sampler cache flush";
            sendOp = SendOp::SAMPLER_FLUSH;
            params = 0; // certain
            break;
        default:
            sym << "sample_" << std::hex << std::uppercase << opBits << "?";
            messageDesc = "?";
            sendOp = SendOp::INVALID;
            break;
        }
    }
    (void)addField("SamplerMessageType",12,5,opBits,messageDesc);
    desc << messageDesc;

    auto six = getDescBitsField("SamplerIndex",8,4,
        [&](std::stringstream &ss, uint32_t six){ss << "sampler " << six;});
    desc << " using sampler index " << six;
    sym << "[" << six << "," << decodeBTI(32) << "]";

    AddrType addrType = AddrType::BTI;
    setScatterGatherOp(
        sym.str(),
        desc.str(),
        sendOp,
        addrType,
        getDescBits(0,8),
        32,
        is16bData ? 16 : 32,
        params,
        simdSize,
        0
      );
    decodeMDC_H(); // header is optional in the sampler
}


void DescDecoder::tryDecode() {
    getDescBitsField("Mlen",25,4, [](std::stringstream &ss, uint32_t val) {
        ss << val << " address registers written";
    });
    getDescBitsField("Rlen",20,5, [](std::stringstream &ss, uint32_t val) {
        ss << val << " registers read back";
    });

    bool showXlen = true;
    if (showXlen) {
        getDescBitsField("Xlen", 32+6, 5,
            [] (std::stringstream &ss, uint32_t val) {
                ss << val << " data registers written";
            });
    }

    switch (sfid) {
    case SFID::DCRO: tryDecodeDCRO(); break;
    case SFID::DC0:  tryDecodeDC0();  break;
    case SFID::DC1:  tryDecodeDC1();  break;
    case SFID::URB:  tryDecodeURB();  break;
    //////////////////////////////////////////////////////////////
    // DC2 shouldn't be used after SKL
    case SFID::DC2: error(0, 32, "unsupported DC2 op"); break;
    case SFID::GTWY: tryDecodeGTWY();  break;
    case SFID::TS:
        if (getDescBits(0,3) == 0) {
            setSpecialOp(
                "eot", "end of thread", SendOp::EOT,
                AddrType::FLAT, 0,
                1, // mlen
                0, // rlen
                0);
        } else {
            error(0,32,"unsupported TS op");
        }
        break;
    case SFID::RC:
        tryDecodeRC();
        break;
    case SFID::SMPL:
        tryDecodeSampler();
        break;
    default:
        error(0, 0, "unsupported sfid");
        return;
    }
}

SFID iga::MessageInfo::sfidFromEncoding(Platform p, uint32_t sfidBits)
{
    SFID sfid = SFID::INVALID;
    switch (sfidBits & 0xF) {
    case 0x0: sfid = SFID::NULL_; break;
    case 0x2: sfid = SFID::SMPL; break;
    case 0x3: sfid = SFID::GTWY; break;
    case 0x4: sfid = SFID::DC2;  break;
    case 0x5: sfid = SFID::RC;  break;
    case 0x6: sfid = SFID::URB;  break;
    case 0x7:
        sfid = SFID::TS;
        break;
    case 0x8:
        sfid = SFID::VME;
        break;
    case 0x9: sfid = SFID::DCRO; break;
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

SFID iga::MessageInfo::sfidFromOp(Platform p, Op op, uint32_t exDesc)
{
    switch (op) {
    case Op::SEND:
    case Op::SENDC:
    case Op::SENDS:
    case Op::SENDSC:
        return sfidFromEncoding(p, exDesc & 0xF);
    default:
        return SFID::INVALID;
    }
}
