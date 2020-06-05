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
#include "MessageDecoder.hpp"

using namespace iga;

static const char *MDC_DS_MEANINGS[] {
    "DE1 (1 data element per address)",
    "DE2 (2 data elements per address)",
    "DE4 (4 data elements per address)",
    "DE8 (8 data elements per address)",
};

enum HDC : uint32_t {
    MSD0R_OWB      = 0x00, // oword block read (unaligned)
    MSD0R_OWAB     = 0x01, // oword/hword block read (aligned)
    MSD0W_OWB      = 0x08, // oword block write
    MSD0W_HWAB     = 0x09, // hword block write (aligned)
    MSD0R_OWDB     = 0x02, // dual block read
    MSD0W_OWDB     = 0x0A, // dual block write
    //
    MSD0R_BS       = 0x04, // byte gathered read
    MSD0W_BS       = 0x0C, // byte scattered write
    //
    MSD0R_DWS      = 0x03, // dword gathered read
    MSD0W_DWS      = 0x0B, // dword scattered write
    //
    MSD0R_QWS      = 0x05, // qword gathered write
    MSD0W_QWS      = 0x0D, // qword scattered write
    //
    MSD_MEMFENCE   = 0x07,
    //
    /////////////////////////////////////////
    MSD1R_US       = 0x01, // untyped read
    MSD1W_US       = 0x09, // untyped write
    //
    MSD1R_A64_US   = 0x11, // a64 untyped read
    MSD1W_A64_US   = 0x19, // a64 untyped write
    //
    MSD1R_A64_BS   = 0x10, // byte gathered read
    MSD1W_A64_BS   = 0x1A, // byte scattered write
    MSD1R_A64_DWS  = (0x1 << 8) | MSD1R_A64_BS,
    MSD1W_A64_DWS  = (0x1 << 8) | MSD1W_A64_BS,
    MSD1R_A64_QWS  = (0x2 << 8) | MSD1R_A64_BS,
    MSD1W_A64_QWS  = (0x2 << 8) | MSD1W_A64_BS,
    MSD1RS_A64_BS  = (0x3 << 8) | MSD1R_A64_BS,
    //
    MSD1R_A64_HWB  = 0x14,
    MSD1W_A64_HWB  = 0x15,
    //
    MSD1W_A64_DWAF = 0x1D,
    MSD1W_DWAF     = 0x1B,
    // MSD1W_A64_QWAI = (1 << 12) | 0x12,
    // MSD1W_A64_DWAI = (0 << 12) | 0x12,
    MSD1W_A64_AI   = 0x12,
    //
    MSD1W_DWAI     = 0x02,
    //
    MSD1R_MB       = 0x04,
    MSD1W_MB       = 0x0A,
    //
    MSD1R_TS       = 0x05,
    MSD1W_TS       = 0x0D,
    //
    MSD1A_DWAC     = 0x0B,
    MSD1A_DWTAI    = 0x06,
    //
};


struct MessageDecoderHDC : MessageDecoderLegacy {
    MessageDecoderHDC(
        Platform _platform, SFID _sfid,
        SendDesc _exDesc, SendDesc _desc, RegRef _indDesc,
        DecodeResult &_result)
        : MessageDecoderLegacy(
            _platform, _sfid, _exDesc, _desc, _indDesc, _result)
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    // decoder helpers
    int decodeXXX_HW(int off, int len, const char *fieldName) {
        // scratch's use of DataElements:MDC_DB_HW uses 2 bits
        // DataElements:MDC_DB_HW for A32 uses 3 bits (starting at bit 8)
        // the A64 version is always [10:8]
        auto bits = getDescBits(off,len);
        // 0 -> 1 block, 1 -> 2 blocks, 2 -> 4 blocks, 3 -> 8 blocks
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
        const char *descs = "???";
        // MDC_DB_OW and MDC_64_DB_OW
        // 1L, 1H, 2, 4, 8, 16
        switch (bits) {
        case 0: ows = 1; descs = "1L (accesses low half of GRF)"; break;
        case 1: ows = 1; descs = "1H (accesses high half of GRF)"; break;
        case 2: ows = 2; descs = "2 OWords"; break;
        case 3: ows = 4; descs = "4 OWords"; break;
        case 4: ows = 8; descs = "8 OWords"; break;
        default: break;
        }
        addField(fieldName, off, 3, bits, descs);
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
        addField("DataElements:MDC_DS", off, 2, bits, MDC_DS_MEANINGS[bits]);
        return (1 << (int)bits);
    }
    int MDC_A64_DS(int off) {
        auto bits = getDescBits(off,2);
        addField("DataElements:MDC_A64_DS", off, 2, bits, MDC_DS_MEANINGS[bits]);
        return (1 << (int)bits);
    }
    int decodeMDC_DWS_DS(int off) {
        auto bits = getDescBits(off,2);
        static const char *meanings[] {
            "Fill low 1 byte per DW",
            "Fill low 2 bytes per DW",
            "Fill all 4 bytes per DW",
            "???"
        };
        addField("DataElements:MDC_DWS_DS", off, 2, bits, meanings[bits]);
        return (1 << (int)bits);
    }

    int decodeMDC_SM2R(int off) {
        int bits =
            decodeDescBitField("SimdMode:MDC_SM2R", off, "SIMD16", "SIMD8");
        return bits ? 8 : 16;
    }
    int decodeMDC_SM3(int off) {
        auto bits = getDescBits(off,2);
        int simd = 0;
        const char *simdStr = "?";
        switch (bits) {
        case 1: simd = 16; simdStr = "SIMD16"; break;
        case 2: simd = 8;  simdStr = "SIMD8"; break;
        default: error(off,2,"invalid MDC_SM3"); break;
        }
        addField("SimdMode:MDC_SM3", off, 2, bits, simdStr);
        return simd;
    }
    int decodeMDC_CMASK() {
        auto bits = getDescBits(8, 4);
        std::stringstream ss;
        int vecLen = 0;
        if (bits == 0xF) {
            error(8, 4, "channel mask must have one element not disabled");
        }
        for (int i = 0; i < 4; i++) {
            if (((1 << i) & bits) == 0) {
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
        addField("ChannelDisableMask:MDC_CMASK", 8, 4, bits, ss.str());
        return vecLen;
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

    CacheOpt decodeMDC_IAR() {
        int bits = decodeDescBitField("MDC_IAR", 13, "disabled", "enabled");
        return bits ? CacheOpt::READINVALIDATE : CacheOpt::DEFAULT;
    }

    ///////////////////////////////////////////////////////////////////////////
    //

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
        CacheOpt caching = CacheOpt::DEFAULT;

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
                ss << "+" << 32*getDescBits(0, 12);
            } else if (bti == SLM_BTI) {
                ss << "slm";
                addrType = AddrType::FLAT;
                surfaceId = 0;
                extraAttrs |= MessageInfo::SLM;
            } else if (bti == COHERENT_BTI || bti == NONCOHERENT_BTI) {
                ss << "stateless";
                if (bti != COHERENT_BTI) {
                    ss << "_incoherent";
                    caching = CacheOpt::CACHED;
                } else {
                    caching = CacheOpt::UNCACHED;
                }
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
                caching = CacheOpt::CACHED;
            } else {
                caching = CacheOpt::UNCACHED;
            }
            addrType = AddrType::FLAT;
            if (bti != COHERENT_BTI && bti != NONCOHERENT_BTI)
                error(0, 8, "must have 0xFF or 0xFD BTI");
        }
        ss << ".a" << addrSizeBits;
        if (bitsPerElemReg == bitsPerElemMem) {
            ss << ".d" << bitsPerElemReg;
        } else {
            ss << ".d" << bitsPerElemMem << "u" << bitsPerElemReg;
        }
        int chEnMask = 0;
        if (SendOpHasCmask(op)) {
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
            if (elemsPerAddr > 1 || (extraAttrs & MessageInfo::TRANSPOSED)) {
                ss << "x" << elemsPerAddr;
            }
        }

        // if DC2, ExDesc[31:16] is immediate offset
        if (sfid == SFID::DC2 && exDesc.isImm())
            result.info.immediateOffset = exDesc.imm >> 16;
        setScatterGatherOpX(
            ss.str(),
            msgDesc,
            op,
            addrType,
            SendDesc(surfaceId),
            caching,
            caching,
            addrSizeBits,
            bitsPerElemReg, bitsPerElemMem,
            elemsPerAddr,
            simd,
            extraAttrs);
        result.info.channelsEnabled = chEnMask;
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
        if (owBits == 1)
            ss << "H";
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
        int blockCountOffset,
        int blockCountLen,
        int extraAttrs)
    {
        extraAttrs |= MessageInfo::TRANSPOSED;

        int elems =
            addrSize == 64 ?
            decodeMDC_A64_DB_HW(blockCountOffset) :
            decodeMDC_DB_HW(blockCountOffset, blockCountLen);
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
        //
        appendCMask(msgDesc);
        //
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

    void appendCMask(std::string &msgDesc) {
        auto cMaskBits = getDescBits(8, 4);
        msgDesc += " with ";
        for (int i = 0; i < 4; i++) {
            if ((cMaskBits & (1 << i)) == 0) {
                // NOTE: legacy untyped message channel masks are a
                // disable bit
                msgDesc += "xyzw"[i];
            }
        }
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
            msgDesc += " returning status";
            msgSym += "_wstatus";
        }
        //
        appendCMask(msgDesc);
        //
        setHdcMessage(
            msgSym,
            msgDesc,
            isRead ? SendOp::LOAD_QUAD : SendOp::STORE_QUAD,
            32, // addrSize
            32, // dataSize
            decodeMDC_CMASK(),
            DEFAULT_EXEC_SIZE/2,
            MessageInfo::HAS_CHMASK|MessageInfo::HAS_UVRLOD);
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
        if (decodeDescBitField(
            "ReturnDataControl", 13, "no return value", "returns new value"))
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
    } // setHdcFloatAtomicMessage

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
        auto aopBits = getDescBits(8, 4);
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
        if (decodeDescBitField(
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
    } // setHdcIntAtomicMessage

    ///////////////////////////////////////////////////////////////////////////
    void tryDecode() {
        switch (sfid) {
        case SFID::DCRO: tryDecodeDCRO(); break;
        case SFID::DC0:  tryDecodeDC0();  break;
        case SFID::DC1:  tryDecodeDC1();  break;
            //////////////////////////////////////////////////////////////
            // DC2 shouldn't be used after SKL
        case SFID::DC2: error(0, 32, "unsupported DC2 op"); break;
        default:
            error(0, 0, "unsupported sfid");
            return;
        }
    }

    void tryDecodeDCRO();
    void tryDecodeDC0();
    void tryDecodeDC0BSRW(bool isRead);
    void tryDecodeDC0AlignedBlock();
    void tryDecodeDC0Memfence();
    void tryDecodeDC0ScratchBlock();
    void tryDecodeDC1();
}; // MessageDecoderHDC



void MessageDecoderHDC::tryDecodeDCRO() {
    const int msgType = decodeDescField("MType", 14, 5,
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
        result.info.cachingL3 = decodeMDC_IAR();
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
        result.info.cachingL3 = decodeMDC_IAR();
        decodeExpected(9, 1, "LegacySimdMode", 1);
        decodeMDC_H();
        setDoc("7084");
        break;
    default:
        error(14, 5, "unsupported DCRO op");
        return;
    }
}

void MessageDecoderHDC::tryDecodeDC0()
{
    const int msgType = getDescBits(14, 5);
    switch (msgType) {
    case MSD0R_OWB: // oword block read (unaligned)
        addField("MessageType", 14, 5, msgType, "oword block read");
        setHdcOwBlock(
            "load_block",
            "oword block read",
            SendOp::LOAD,
            32, // all 32b addresses
            0);
        setDoc("7028");
        decodeMDC_HR();
        break;
    case MSD0R_OWAB: // aligned hword/oword block read
        tryDecodeDC0AlignedBlock();
        break;
    case MSD0W_OWB: // hword/oword block write
        addField("MessageType", 14, 5, msgType, "oword block write");
        setHdcOwBlock(
            "store_block",
            "oword block write",
            SendOp::STORE,
            32, // all 32b addresses
            0);
        setDoc("7032");
        decodeMDC_HR();
        break;
    case MSD0W_HWAB: // hword aligned block write
        addField("MessageType", 14, 5, msgType, "hword aligned block write");
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
    case MSD0R_OWDB: // oword dual block read
        addField("MessageType", 14, 5, msgType, "oword dual block read");
        result.info.description = "oword dual block read";
        setDoc("7029");
        decodeMDC_HR();
        error(14,5, "oword dual block read decode not supported");
        return;
    case MSD0W_OWDB: // oword dual block write
        addField("MessageType", 14, 5, msgType, "oword dual block write");
        result.info.description = "oword dual block write";
        setDoc("7033");
        decodeMDC_HR();
        error(14,5,"oword dual block write decode not supported");
        return;
        //
    case MSD0R_BS: // byte gathered read
    case MSD0W_BS: // byte scattered write
        tryDecodeDC0BSRW(msgType == MSD0R_BS);
        break;
    //
    case MSD0R_DWS: // dword gathered read
    case MSD0W_DWS: // dword scattered write
    {
        bool isRead = msgType == MSD0R_DWS;
        const char *msgName = isRead ?
            "dword gathering read" : "dword scattering write";
        addField("MessageType", 14, 5, msgType, msgName);
        //
        int elemsPerAddr = decodeMDC_DWS_DS(10);
        std::stringstream ss;
        ss << msgName;
        if (elemsPerAddr != 1)
            ss << msgName << " x" << elemsPerAddr;
        //
        setHdcMessage(
            isRead ? "load" : "store",
            ss.str(),
            isRead ? SendOp::LOAD : SendOp::STORE,
            32,
            32,
            elemsPerAddr,
            decodeMDC_SM2(8),
            0);
        result.info.cachingL3 = decodeMDC_IAR();
        setDoc(isRead ? "7067" : "7069");
        decodeExpected(9, 1, "LegacySimdMode", 1);
        decodeMDC_H();
        break;
    }
    case MSD_MEMFENCE:
        tryDecodeDC0Memfence();
        break;
    default:
        if (getDescBit(18) == 1) {
            tryDecodeDC0ScratchBlock();
        } else {
            addField("MessageType", 14, 5, msgType, "???");
            error(14, 5, "unsupported dc0 op");
            return;
        }
    } // end switch legacy DC0
}

void MessageDecoderHDC::tryDecodeDC0BSRW(bool isRead)
{
    const uint32_t msgType = getDescBits(14, 5);
    const char *msgName = isRead ?
        "byte gathering read" : "byte scattering write";

    addField("MessageType", 14, 5, msgType, msgName);

    std::stringstream descs;
    descs << msgName;
    int memBytes = decodeMDC_DS(10);
    if (memBytes == 1)
        descs << " 8b";
    else if (memBytes == 2)
        descs << " 16b";
    else if (memBytes == 4)
        descs << " 32b";

    //
    // "byte" scattered always consumes a DW of GRF per channel,
    // but DWS_DS controls how many bytes are loaded per address
    // that might be 1, 2, 4 all packed into one DW.
    // So think of:
    //     DWS_DS == 0 (byte) as u8 aligned to u32 (upper bits undefined)
    //     DWS_DS == 1 (word) as u16 aligned to u32 (upper bits undefined)
    //     DWS_DS == 2 (dword) as u32
    setHdcMessageX(
        isRead ? "load" : "store",
        descs.str(),
        isRead ? SendOp::LOAD : SendOp::STORE,
        32, // 32b addrs
        32, // each channel occupies a DW in the reg file
        8*memBytes, // in memory it can be 1, 2, or 4 bytes
        1, // vector size always 1
        decodeMDC_SM2(8),
        0);
    result.info.cachingL3 = decodeMDC_IAR();
    setDoc(isRead ? "7066" : "7068");
    decodeMDC_H();
}

void MessageDecoderHDC::tryDecodeDC0AlignedBlock()
{
    const int msgType = getDescBits(14, 5);
    const char *descs ="aligned block read";
    const char *doc = "7030";
    bool isHw = false;
    addField("MessageType", 14, 5, msgType, descs);
    if (isHw) {
        setHdcHwBlock(
            "aligned_load_block256",
            "hword aligned block read",
            SendOp::LOAD,
            32, // all 32b addresses
            8, 3, // [10:8]
            0);
    } else {
        setHdcOwBlock(
            "aligned_load_block128",
            "oword aligned block read",
            SendOp::LOAD,
            32, // all 32b addresses
            0);
    }
    setDoc(doc);
    decodeMDC_HR();
}

void MessageDecoderHDC::tryDecodeDC0Memfence()
{
    const int msgType = getDescBits(14, 5);
    // memory fence
    addField("MessageType", 14, 5, msgType, "fence");
    //
    std::stringstream sym, descs;
    uint32_t surfId = getDescBits(0,8);
    int extraAttrs = 0;
    if (decodeDescBitField("Commit", 13,
        "off (return immediately)",
        "on (wait for fence commit)"))
    {
        sym << "sync_";
        descs << "synchronized ";
    }
    if (surfId == SLM_BTI) {
        (void)decodeBTI(32); // add the field
        sym << "slm_fence";
        descs << "SLM fence";
        extraAttrs |= MessageInfo::SLM;
    } else if (surfId == 0) {
        sym << "global_fence";
        descs << "global fence";
        if (getDescBits(9, 4) && getDescBit(8)) {
            error(8, 1, "L3 implies L1 flush");
        }
        descs << " flushing";
        if (decodeDescBitField("L1Flush",8,"Flush L3","FLush L1") != 0) {
            sym << ".l1";
            descs << " L1";
        }
        if (decodeDescField("L3 Flush Targets",9,4)) {
            if (getDescBits(9, 4) == 0xF) {
                descs << " all L3 data";
                sym << ".dcti";// data, const?, text, inst
            } else {
                int n = 0;
                descs << " L3";
                sym << ".";
                if (getDescBit(12)) {
                    sym << "d";
                    descs << " r/w data";
                }
                if (getDescBit(11)) {
                    if (n++ > 0)
                        descs << ",";
                    sym << "c";
                    descs << " constant data";
                }
                if (getDescBit(10)) {
                    if (n++ > 0)
                        descs << ",";
                    sym << "t";
                    descs << " texture data";
                }
                if (getDescBit(9)) {
                    if (n++ > 0)
                        descs << ",";
                    sym << "i";
                    descs << " instruction data";
                }
            }
        }
    } else {
        error(0, 8, "invalid BTI for fence (must be 0x0 or 0xFE)");
    }
    setSpecialOpX(
        sym.str(),
        descs.str(),
        SendOp::FENCE,
        AddrType::FLAT,
        SendDesc(surfId),
        1,
        1,
        extraAttrs);
    setDoc("7049");
    decodeMDC_HR();
}

void MessageDecoderHDC::tryDecodeDC0ScratchBlock()
{
    const int msgType = getDescBits(14, 5);
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
    uint32_t hwOff = 32*
        decodeDescField("HWordOffset", 0, 12,
            [&] (std::stringstream &ss, uint32_t val) {
                ss << val << " HWords from scratch base";
            });
    result.info.immediateOffset = hwOff;
    if (platform() < Platform::GEN10) {
        decodeDescBitField("ChannelMode", 16, "OWord", "DWord");
    }
    setDoc(isRead ? "7027" : "7031");

    decodeMDC_HR();
}

#if 0
experimental work in a table-based approach

struct Doc {
    Platform platform;
    const char *doc;

    constexpr Doc() : Doc(Platform::INVALID, nullptr) { }
    constexpr Doc(const char *d) : Doc(Platform::GEN6, d) { }
    constexpr Doc(Platform p, const char *d) : platform(p), doc(d) { }

    constexpr Doc(Doc &) = default;
    constexpr Doc(Doc &&) = default;
    constexpr Doc &operator=(const Doc &) = default;
};
static constexpr Doc INVALID_DOC;


struct Format {
    uint32_t encoding;
    uint32_t encodingMask;

    const char *mnemonic;
    const char *description;
    const char *canonicalName;

    SendOp op;
    int addrBits;
    int dataBitsRegfile;
    int extraAttrs;

    Doc docs[2];

    constexpr Format(
        const char *mne,
        const char *desc,
        const char *canon,
        uint32_t enc,
        uint32_t encMask,
        SendOp op,
        int aSize,
        int dSize,
        const Doc &d1 = Doc(),
        const Doc &d2 = Doc())
        : mnemonic(mne)
        , description(desc)
        , canonicalName(canon)
        , encoding(enc)
        , encodingMask(encMask)
        , addrBits(aSize)
        , dataBitsRegfile(dSize)
        , extraAttrs(0)
    {
        docs[0] = d1;
        docs[1] = d2;
    }
};

static constexpr uint32_t DC1_OP_MASK = getFieldMask<uint32_t>(14, 4);

static constexpr Format HDC_OP(
    const char *mne,
    const char *desc,
    const char *canon,
    uint32_t enc,
    int addrSize,
    const Doc &d1 = Doc(),
    const Doc &d2 = Doc())
{
    return Format(mne, desc, canon, enc, DC1_OP_MASK, addrSize, d1, d2);
}


static constexpr Format DC1_OPS[] {
    HDC_OP("load_untyped", "untyped surface read",
        "MSD1R_US", 0x01, 32, "7088"),
    HDC_OP("store_untyped", "untyped surface write",
        "MSD1W_US", 0x09, 32, "7091"),
    HDC_OP("load_untyped", "a64 untyped surface read",
        "MSD1R_A64_US", 0x11, 64, "7086"),
    HDC_OP("store_untyped", "a64 untyped surface write",
        "MSD1W_A64_US", 0x19, 64, "7089"),
};
#endif


void MessageDecoderHDC::tryDecodeDC1() {
    const int msgType = getDescBits(14, 5);
    switch (msgType)
    {
    case MSD1R_US: // untyped surface read
    case MSD1W_US: // untyped surface write
    {
        const char *msgName = msgType == 0x01 ?
            "untyped surface read" : "untyped surface write";
        addField("MessageType", 14, 5, msgType, msgName);
        //
        setHdcUntypedSurfaceMessage(
            msgName,
            msgType == MSD1R_US,
            32,
            0);
        setDoc(msgType == MSD1R_US ? "7088" : "7091");
        decodeMDC_H();
        break;
    }
    case MSD1R_A64_US: // a64 untype surface read
    case MSD1W_A64_US: // a64 untype surface write
    {
        const char *msgName = msgType == MSD1R_A64_US ?
            "a64 untyped surface read" : "a64 untyped surface write";
        addField("MessageType", 14, 5, msgType, msgName);
        //
        setHdcUntypedSurfaceMessage(
            msgName,
            msgType == MSD1R_A64_US,
            64, // 8B addrs
            0);
        setDoc(msgType == MSD1R_A64_US ? "7086" : "7089");
        decodeMDC_HF();
        break;
    }
    case MSD1R_A64_BS: // a64 gathering read (byte/dw/qw)
    case MSD1W_A64_BS: // a64 scattered write (byte/dw/qw)
    {
        // 0 is byte-scattered, the others (DW/QW) are true SIMT
        int subType = getDescBits(8, 2);
        bool isRead = msgType == MSD1R_A64_BS;
        const auto BYTE_SUBTYPE = 0x0, BSRWSR_SUBTYPE = 0x3;
        if (subType == BYTE_SUBTYPE || subType == BSRWSR_SUBTYPE) {
            // c.f. handling above with non-A64 version of BSR
            const char *msgName = msgType == MSD1R_A64_BS ?
                "a64 byte gathering read" : "a64 byte scattering write";
            addField("MessageType", 14, 5, msgType, msgName);
            //
            if (subType == BYTE_SUBTYPE) {
                addField("SubType", 8, 2, subType, "Byte");
                setDoc(msgType == MSD1R_A64_BS ? "7070" : "7073");
            } else { // subType == 0x3
                addField("SubType", 8, 2, subType, "Byte with Status Return");
                setDoc(msgType == MSD1R_A64_BS ? "19316" : nullptr);
                if (msgType == MSD1W_A64_BS)
                    error(14, 5,
                        "a64 byte scattering with status return message");
            }
            //
            std::stringstream descs;
            descs << msgName;
            int bExt = MDC_A64_DS(10);
            if (bExt == 1)
                descs << " 8b";
            else if (bExt == 2)
                descs << " 16b";
            else if (bExt == 4)
                descs << " 32b";
            //
            setHdcMessageX(
                isRead ? "load" : "store",
                descs.str(),
                isRead ? SendOp::LOAD : SendOp::STORE,
                64, // A64
                32, // widens to DW (similar to non-A64 version)
                8*bExt, // bits from memory
                1, //
                decodeMDC_SM2(12),
                0);
        } else {
            const auto DW_SUBTYPE = 0x1;
            const auto QW_SUBTYPE = 0x2;
            bool isDword = subType == DW_SUBTYPE; // else QW
            // unlike non-A64 version, this variant supports DW and QW
            // in the same message type, the MDC_A64_DS is treated as a
            // vector length
            const char *msgTypeName = msgType == MSD1R_A64_BS ?
                "a64 gathering read" : "a64 scattering write";
            addField("MessageType", 14, 5, msgType, msgTypeName);
            //
            const char *msgName =
                isDword ?
                    (isRead ?
                        "a64 dword gathering read" :
                        "a64 dword scattering write") :
                    (isRead ?
                        "a64 qword gathering read" :
                        "a64 qword scattering write");
            //
            int elemsPerAddr = decodeMDC_DWS_DS(10);
            std::stringstream ss;
            if (elemsPerAddr != 1)
                ss << msgName << " x" << elemsPerAddr;
            //
            addField(
                "SubType", 8, 2, subType, subType == 1 ? "DWord" : "QWord");
            //
            setHdcMessage(
                isRead ? "load" : "store",
                msgName,
                isRead ? SendOp::LOAD : SendOp::STORE,
                64,
                isDword ? 32 : 64,
                elemsPerAddr, // true vector
                decodeMDC_SM2(12),
                0);
            if (isDword) {
                setDoc(isRead ? "7071" : "7074");
            } else {
                setDoc(isRead ? "7072" : "7075");
            }
        }
        result.info.cachingL3 = decodeMDC_IAR();
        decodeMDC_HF();
        break;
    }
    case MSD1R_A64_HWB: // a64 [un]aligned (hword|oword) block read
    case MSD1W_A64_HWB: // a64 [un]aligned (hword|oword) block write
    {
        bool isRead = msgType == MSD1R_A64_HWB;

        addField("MessageType", 14, 5, msgType,
            isRead ? "a64 block read" :"a64 block write");
        bool isHword = false;
        bool isUnaligned = false;
        auto subType = decodeDescField("SubType", 11, 2,
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
                    error(11, 2, "a64 dual block read/write unsupported");
                    break;
                }
            });

        if (isHword && isUnaligned)
            setDoc(isRead ? "7034" : "7038");
        else if (isHword && !isUnaligned) {
            bool supported = false;
            if (!supported)
                error(11,2,"HWord aligned unsupported on this platform");
        } else if (!isHword && isUnaligned) {
            setDoc(isRead ? "7037" : "33440"); // MSD1W_A64_OWAB
        } else if (!isHword && !isUnaligned) {
            setDoc(isRead ? "7039" : "7039"); // MSD1W_A64_OWB
        } else if (subType == 2) {
            setDoc(isRead ? "7036" : "7040");
            result.info.description = "a64 dual block ";
            result.info.description += (isRead ? " read" : " write");
            result.info.symbol =
                isRead ? "MSD1R_A64_OWDB" : "MSD1W_A64_OWDB";
            result.info.addrSizeBits = 64;
            decodeMDC_HR();
            break; // dual block (error)
        }
        //
        std::string msgSym = isRead ? "load_block" : "store_block";
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
                isRead ? SendOp::LOAD : SendOp::STORE,
                64, // 64b addr
                8, 3, // offset of HWs
                0);
        } else {
            setHdcOwBlock(
                msgSym,
                msgDesc,
                isRead ? SendOp::LOAD : SendOp::STORE,
                64, // 64b addr
                0);
        }
        result.info.cachingL3 = decodeMDC_IAR();
        decodeMDC_HR(); // all block require a header
        break;
    }
    case MSD1W_A64_DWAF: // a64 untyped fp32 atomic
        setHdcFloatAtomicMessage(
            "a64 float atomic",
            64,
            32,
            "7126",
            "7118");
        decodeMDC_HF();
        break;

    case MSD1W_DWAF: // a32 untyped fp32 atomic
        setHdcFloatAtomicMessage(
            "float atomic",
            32,
            32,
            "7130",
            "7122");
        decodeMDC_H();
        break;
    case MSD1W_A64_AI: // a64 untyped atomic int{32,64}
    {
        // the encoding repurposes the SIMD size as the data size and the
        // message and thus the SIMD size is always fixed
        int simd = 8;
        const char *msgName = "a64 untyped atomic int32";
        auto dataWidthCtrl =
            decodeDescBitField("DataWidth", 12, "32b", "64b");
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
    case MSD1W_DWAI: // atomic int32
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
    case MSD1R_MB: // media block read
    case MSD1W_MB: // media block write
    {
        std::stringstream sym, descs;
        sym << "mb";
        descs << "media block ";
        int bytesTransmitted = 0;
        if (msgType == MSD1R_MB) {
            descs << "read";
            sym << "rd";
            bytesTransmitted = 2*DEFAULT_EXEC_SIZE*getDescBits(20,5);
        } else {
            descs << "write";
            sym << "wr";
            int mlen = getDescBits(25, 4);
            if (mlen == 0) {
                error(25, 4, "mlen == 0 on write");
            } else {
                bytesTransmitted = 2*DEFAULT_EXEC_SIZE*(mlen - 1);
            }
        }
        int vlso = getDescBits(8, 3); // [10:8] is vert. line stride overr.
        if (vlso) {
            descs << " with vertical line stride ";
            int n = 0;
            if (vlso & 0x2) {
                descs << "override";
                n++;
            }
            if (vlso & 0x1) {
                if ((vlso & 0x2) == 0)
                    descs << " and ";
                else if (n > 0)
                    descs << ",";
                descs << "skip";
                n++;
            }
            if (vlso & 0x2) {
                if (n >= 2)
                    descs << ", and ";
                else if (n > 0)
                    descs << " and ";
                descs << "offset";
                n++;
                if ((vlso & 0) == 0) {
                    warning(8, 3,
                        "stride offset meaningless when override not set");
                }
            }
        }
        setHdcMessage(sym.str(), descs.str(),
            msgType == 0x4 ? SendOp::LOAD : SendOp::STORE,
            32, 8, bytesTransmitted, 1,
            MessageInfo::TRANSPOSED);
        setDoc(msgType == MSD1R_MB ? "7046" : "7048");
        decodeMDC_HR();
        break;
    }
    case MSD1R_TS: // typed surface read
        setHdcTypedSurfaceMessage(true, "7087");
        decodeMDC_H();
        break;
    case MSD1W_TS: // typed surface write
        setHdcTypedSurfaceMessage(false, "7090");
        decodeMDC_H();
        break;
    case MSD1A_DWAC:
        setHdcIntAtomicMessage(
            "typed",
            "atomic 32-bit counter",
            32, // addrSize
            32, // dataSize
            decodeMDC_SM2R(12),
            "7109", "7099");
        decodeMDC_HR();
        result.info.attributeSet |= MessageInfo::HAS_UVRLOD;
        break;
    case MSD1A_DWTAI:
    {
        bool sgh = decodeMDC_SG2();
        setHdcIntAtomicMessage(
            sgh ? "typed_sgh" : "typed" ,
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


void iga::decodeDescriptorsHDC(
    Platform platform, SFID sfid,
    SendDesc exDesc, SendDesc desc, RegRef indDesc,
    DecodeResult &result)
{
    MessageDecoderHDC mdo(platform, sfid, exDesc, desc, indDesc, result);
    mdo.tryDecode();
}

