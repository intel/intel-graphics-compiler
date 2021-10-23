/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MessageDecoder.hpp"

#include <utility>

using namespace iga;

enum LscOp : uint32_t {
    LSC_LOAD            = 0x00,
    LSC_LOAD_STRIDED    = 0x01,
    LSC_LOAD_QUAD       = 0x02, // aka load_cmask
    LSC_LOAD_BLOCK2D    = 0x03,
    LSC_STORE           = 0x04,
    LSC_STORE_STRIDED   = 0x05,
    LSC_STORE_QUAD      = 0x06, // aka store_cmask
    LSC_STORE_BLOCK2D   = 0x07,
    //
    LSC_ATOMIC_IINC     = 0x08,
    LSC_ATOMIC_IDEC     = 0x09,
    LSC_ATOMIC_LOAD     = 0x0A,
    LSC_ATOMIC_STORE    = 0x0B,
    LSC_ATOMIC_IADD     = 0x0C,
    LSC_ATOMIC_ISUB     = 0x0D,
    LSC_ATOMIC_SMIN     = 0x0E,
    LSC_ATOMIC_SMAX     = 0x0F,
    LSC_ATOMIC_UMIN     = 0x10,
    LSC_ATOMIC_UMAX     = 0x11,
    LSC_ATOMIC_ICAS     = 0x12,
    LSC_ATOMIC_FADD     = 0x13,
    LSC_ATOMIC_FSUB     = 0x14,
    LSC_ATOMIC_FMIN     = 0x15,
    LSC_ATOMIC_FMAX     = 0x16,
    LSC_ATOMIC_FCAS     = 0x17,
    LSC_ATOMIC_AND      = 0x18,
    LSC_ATOMIC_OR       = 0x19,
    LSC_ATOMIC_XOR      = 0x1A,
    //
    LSC_LOAD_STATUS     = 0x1B,
    LSC_STORE_UNCOMPRESSED = 0x1C,
    LSC_CCS             = 0x1D,
    //
    LSC_RSI             = 0x1E,
    LSC_FENCE           = 0x1F,
    //
    LSC_STORE_UNCOMPRESSED_QUAD = 0x20,
    //
    //
    LSC_INVALID         = 0xFFFFFFFF,
};

static const uint32_t LSC_AT_FLAT = 0x0;
static const uint32_t LSC_AT_BSS  = 0x1;
static const uint32_t LSC_AT_SS   = 0x2;
static const uint32_t LSC_AT_BTI  = 0x3;

static const uint32_t LSC_A16 = 0x1;
static const uint32_t LSC_A32 = 0x2;
static const uint32_t LSC_A64 = 0x3;

static const uint32_t LSC_D8      = 0x0;
static const uint32_t LSC_D16     = 0x1;
static const uint32_t LSC_D32     = 0x2;
static const uint32_t LSC_D64     = 0x3;
static const uint32_t LSC_D8U32   = 0x4;
static const uint32_t LSC_D16U32  = 0x5;
static const uint32_t LSC_D16U32H = 0x6;

static const uint32_t LSC_V1  = 0x0;
static const uint32_t LSC_V2  = 0x1;
static const uint32_t LSC_V3  = 0x2;
static const uint32_t LSC_V4  = 0x3;
static const uint32_t LSC_V8  = 0x4;
static const uint32_t LSC_V16 = 0x5;
static const uint32_t LSC_V32 = 0x6;
static const uint32_t LSC_V64 = 0x7;

const static uint32_t LSC_SCALE_NONE = 0x0;
const static uint32_t LSC_SCALE_1X   = 0x1;
const static uint32_t LSC_SCALE_2X   = 0x2;
const static uint32_t LSC_SCALE_4X   = 0x3;

///////////////////////////////////////////////////////
// Cache Opt
// Value for bits[19:17]
static const uint32_t LSC_DF_DF = 0x0;
//
static const uint32_t LSC_UC_UC = 0x1;
//
static const uint32_t LSC_UC_CA = 0x2;
static const uint32_t LSC_UC_WB = 0x2;
//
static const uint32_t LSC_CA_UC = 0x3;
static const uint32_t LSC_WT_UC = 0x3;
//
static const uint32_t LSC_CA_CA = 0x4;
static const uint32_t LSC_WT_WB = 0x4;
//
static const uint32_t LSC_ST_UC = 0x5;
//
static const uint32_t LSC_ST_CA = 0x6;
static const uint32_t LSC_ST_WB = 0x6;
//
static const uint32_t LSC_RI_CA = 0x7;
static const uint32_t LSC_WB_WB = 0x7;

// Value for bits [19:16]
static const uint32_t LSC_UC_CC = 0x5;
static const uint32_t LSC_CA_CC = 0x9;
static const uint32_t LSC_RI_RI = 0xE;


#if 0
struct LscMessageFormat {
    const char *mnemonic;
    const char *description;
    uint32_t    mask;
    uint32_t    op;
//
//    std::pair<Platform,const char *> docs[2];
};

//
static LscMessageFormat OPS[32] {
};
#endif


// This handles LSC messages only
struct MessageDecoderLSC : MessageDecoder {
    MessageDecoderLSC(
        Platform _platform,
        SFID _sfid,
        ExecSize _execSize,
        SendDesc _exDesc,
        SendDesc _desc,
        DecodeResult &_result)
        : MessageDecoder(
            _platform, _sfid, _execSize,
            _exDesc, _desc, _result)
    {
    }

    // used by decodeLscMessage and subchildren
    std::string dataTypePrefixSyntax; // e.g. d32 or d16 or d32
    std::string vectorSuffixSyntax; // e.g. x16t (for d16x16t) or .yzw
    std::string addrSizeSyntax; // e.g. a32
    std::string cacheControlSyntax; // e.g. ca.ca

    SendOp op = SendOp::INVALID;
    //
    int expectedExecSize = 1;
    //
    int addrSizeBits = 0;
    int dataSizeRegBits = 0, dataSizeMemBits = 0;
    int vectorSize = 1;
    MessageInfo::Attr extraAttrs = MessageInfo::Attr::NONE;

    // the symbol to return in the MessageInfo structure
    std::string symbolFromSyntax() const {
        std::stringstream sym;
        sym << result.syntax.mnemonic;
        if (!result.syntax.controls.empty())
            sym << result.syntax.controls;
        sym << "  ";
        if (!result.syntax.surface.empty())
            sym << result.syntax.surface;
        sym << "[";
        if (!result.syntax.scale.empty()) {
            sym << result.syntax.scale;
        }
        sym << "A";
        if (!result.syntax.immOffset.empty()) {
            sym << result.syntax.immOffset;
        }
        sym << "]";
        return sym.str();
    }

    ///////////////////////////////////////////////////////////////////////////
    bool hasPayloadSizesInDesc() const {
        return true;
    }

    int lscAddrTypeOffset() const {
        int off = 29;

        return off;
    }

    void setCacheOpts(std::stringstream& sym, std::stringstream& descs,
        CacheOpt &l1, CacheOpt &l3, CacheOpt _l1, CacheOpt _l3) {
        l1 = _l1;
        l3 = _l3;
        if (_l1 == CacheOpt::DEFAULT && _l3 == CacheOpt::DEFAULT) {
            descs << "use state settings for both L1 and L3";
            return;
        }
        auto emitCacheOpt = [&] (CacheOpt c) {
            sym << '.';
            switch (c) {
            case CacheOpt::DEFAULT:
                sym << "df";
                descs << " uses default state settings";
                break;
            case CacheOpt::READINVALIDATE:
                sym << "ri";
                descs << " read-invalidate (last use)";
                break;
            case CacheOpt::CACHED:
                sym << "ca";
                descs << " cached";
                break;
            case CacheOpt::STREAMING:
                sym << "st";
                descs << " streaming";
                break;
            case CacheOpt::UNCACHED:
                sym << "uc";
                descs << " uncached (bypass)";
                break;
            case CacheOpt::WRITETHROUGH:
                sym << "wt";
                descs << " writethrough";
                break;
            case CacheOpt::WRITEBACK:
                sym << "wb";
                descs << " writeback";
                break;
            default:
                sym << "?";
                descs << " invalid";
                break;
            }
        };
        descs << "L1"; emitCacheOpt(_l1);
        descs << "; L3"; emitCacheOpt(_l3); descs << "";
    }


    void decodeLscCacheControl(
        SendOp sop,
        CacheOpt &l1,
        CacheOpt &l3)
    {

        if (!decodeLscCacheControlBits17_19(sop, l1, l3))
            error(17, 3, "invalid cache options");
    }

    // Descriptor Bits[19:17]: 3 bits of cache control
    bool decodeLscCacheControlBits17_19(
        SendOp sop,
        CacheOpt &l1,
        CacheOpt &l3)
    {
        std::stringstream sym, descs;
        l1 = l3 = CacheOpt::DEFAULT;
        bool isLoad = lookupSendOp(sop).isLoad();
        auto ccBits = getDescBits(17, 3);
        auto setCacheOptsWrapper = [&](CacheOpt _l1, CacheOpt _l3) {
            return setCacheOpts(sym, descs, l1, l3, _l1, _l3);
        };
        switch (ccBits) {
        case LSC_DF_DF:
            setCacheOptsWrapper(CacheOpt::DEFAULT, CacheOpt::DEFAULT);
            break;
        case LSC_UC_UC:
            setCacheOptsWrapper(CacheOpt::UNCACHED, CacheOpt::UNCACHED);
            break;
        case LSC_UC_CA: // == LSC_UC_WB
            if (isLoad)
                setCacheOptsWrapper(CacheOpt::UNCACHED, CacheOpt::CACHED);
            else
                setCacheOptsWrapper(CacheOpt::UNCACHED, CacheOpt::WRITEBACK);
            break;
        case LSC_CA_UC: // == LSC_WT_UC
            if (isLoad)
                setCacheOptsWrapper(CacheOpt::CACHED, CacheOpt::UNCACHED);
            else
                setCacheOptsWrapper(CacheOpt::WRITETHROUGH, CacheOpt::UNCACHED);
            break;
        case LSC_CA_CA: // == LSC_WT_WB
            if (isLoad)
                setCacheOptsWrapper(CacheOpt::CACHED, CacheOpt::CACHED);
            else
                setCacheOptsWrapper(CacheOpt::WRITETHROUGH, CacheOpt::WRITEBACK);
            break;
        case LSC_ST_UC:
            setCacheOptsWrapper(CacheOpt::STREAMING, CacheOpt::UNCACHED);
            break;
        case LSC_ST_CA: // == LSC_ST_WB
            if (isLoad)
                setCacheOptsWrapper(CacheOpt::STREAMING, CacheOpt::CACHED);
            else
                setCacheOptsWrapper(CacheOpt::STREAMING, CacheOpt::WRITEBACK);
            break;
        case LSC_RI_CA:
            if (isLoad) {
                // atomic follows store semantics, so compare against load
                setCacheOptsWrapper(CacheOpt::READINVALIDATE, CacheOpt::CACHED);
            } else {
                setCacheOptsWrapper(CacheOpt::WRITEBACK, CacheOpt::WRITEBACK);
            }
            break;
        default:
            return false;
        }
        //
        cacheControlSyntax = sym.str();
        //
        addField("Caching", 17, 3, ccBits, descs.str());
        return true;
    }


    void decodeLscImmOff(uint32_t atBits) {
    }

    AddrType decodeLscAddrType(SendDesc& surfId, bool allowsFlat = true)
    {
        surfId = 0;
        AddrType addrType = AddrType::FLAT;
        //
        int addrTypeLoc = lscAddrTypeOffset();
        const char *addrTypeMeaning = "?";
        //
        const auto atBits = getDescBits(addrTypeLoc, 2);
        //
        std::stringstream surfSyntax;
        switch (atBits) {
        case LSC_AT_FLAT:
            addrTypeMeaning = "Flat";
            addrType = AddrType::FLAT;
            if (!allowsFlat)
                error(addrTypeLoc, 2,
                    "this message may not use FLAT address type");
            break;
        case LSC_AT_BSS:
        case LSC_AT_SS:
            if (atBits == LSC_AT_BSS) {
                addrTypeMeaning = "BSS";
                addrType = AddrType::BSS;
                surfSyntax << "bss";
            } else {
                addrTypeMeaning = "SS";
                addrType = AddrType::SS;
                surfSyntax << "ss";
            }
            if (exDesc.isImm()) {
                // XeHPG/XeHPC: we can pull this value out of ExDesc[31:11]
                int exDescOff = 11, len = 31 - exDescOff + 1;
                surfId = getDescBits(32 + exDescOff, len) << exDescOff;
                addField(
                    "SurfaceStateOffset", exDescOff, len,
                    surfId.imm, "immediate surface state offset");
                surfSyntax << "[" << iga::fmtHex(surfId.imm) << "]";
            } else {
                // XeHPG/XeHPC with reg surface state offset
                surfSyntax << "[a0." << (int)exDesc.reg.subRegNum << "]";
                surfId = exDesc;
            }
            break;
        case LSC_AT_BTI:
            addrTypeMeaning = "BTI";
            addrType = AddrType::BTI;
            if (exDesc.isImm()) {
                uint32_t bti = decodeExDescField("BTI", 24, 8,
                    [&] (std::stringstream &ss, uint32_t bti) {
                        ss << "bti[" << bti << "]";
                    });
                surfSyntax << "bti[" << bti << "]";
                surfId = bti;
            } else {
                surfSyntax << "bti[a0." << (int)exDesc.reg.subRegNum << "]";
                surfId = exDesc;
            }
            break;
        default:
            addrTypeMeaning = "INVALID AddrType";
            addrType = AddrType::FLAT;
            surfSyntax << "?";
            error(addrTypeLoc, 2, "invalid address type");
            break;
        }
        result.syntax.surface = surfSyntax.str();

        /////////////////////////////
        // immediate offset
        decodeLscImmOff(atBits);
        //
        addField("AddrType", lscAddrTypeOffset(), 2, atBits, addrTypeMeaning);
        //
        return addrType;
    }

    void decodeLscAddrSize() {
        int addrSzBits = getDescBits(7, 2); // [8:7]
        std::stringstream asym;
        const char *aDesc = "";
        switch (addrSzBits) {
        case 1:
            asym << "a16";
            aDesc = "addresses are 16b";
            addrSizeBits = 16;
            break;
        case 2:
            asym << "a32";
            aDesc = "addresses are 32b";
            addrSizeBits = 32;
            break;
        case 3:
            asym << "a64";
            aDesc = "addresses are 64b";
            addrSizeBits = 64;
            break;
        default:
            asym << "a???";
            aDesc = "address size is invalid";
            error(7, 2, "invalid address size");
            break;
        }
        // result.syntax.addressType = ":" + asym.str();

        addrSizeSyntax = asym.str();
        //
        addField("AddrSize", 7, 2, addrSzBits, aDesc);
    }

    void decodeLscDataSize() {
        std::stringstream dsym;
        dataSizeRegBits = dataSizeMemBits = 0;
        std::string meaning;
        auto dtBits = getDescBits(9,3);
        switch (dtBits) { // dat size [11:9]
        case LSC_D8:
            dataSizeRegBits = dataSizeMemBits = 8;
            dsym << "d8";
            meaning = "8b per data element";
            break;
        case LSC_D16:
            dataSizeRegBits = dataSizeMemBits = 16;
            meaning = "16b per data element";
            dsym << "d16";
            break;
        case LSC_D32:
            dataSizeRegBits = dataSizeMemBits = 32;
            dsym << "d32";
            meaning = "32b per data element";
            break;
        case LSC_D64:
            dataSizeRegBits = dataSizeMemBits = 64;
            dsym << "d64";
            meaning = "64b per data element";
            break;
        case LSC_D8U32:
            dataSizeRegBits = 32; dataSizeMemBits = 8;
            dsym << "d8u32";
            meaning = "load 8b into the low 8b of 32b register elements "
                "(upper bits are undefined)";
            break;
        case LSC_D16U32:
            dataSizeRegBits = 32; dataSizeMemBits = 16;
            dsym << "d16u32";
            meaning = "load 16b into the low 16b of 32b register elements "
                "(upper bits are undefined)";
            break;
        case LSC_D16U32H:
            dataSizeRegBits = 32; dataSizeMemBits = 16;
            extraAttrs |= MessageInfo::Attr::EXPAND_HIGH;
            dsym << "d16u32h";
            meaning =
                "load 16b into the high half of 32b register elements";
            break;
        default:
            dsym << "0x" << std::uppercase << std::hex << dtBits;
            meaning = "???";
        }
        //
        // result.syntax.dataType = ":" + dsym.str();
        dataTypePrefixSyntax = dsym.str();

        addField("DataSize", 9, 3, dtBits, meaning);
    }

    void decodeLscVecSize()
    {
        if (lookupSendOp(op).hasChMask()) {
            decodeLscVecSizeQuad();
        } else {
            decodeLscVecSizeNormal();
        }
    }

    void decodeLscVecSizeNormal() {
        std::stringstream vsym;

        uint32_t vecSzEncd = getDescBits(12, 3); // [14:12]
        switch (vecSzEncd) {
        case LSC_V1:  vectorSize =  1; break;
        case LSC_V2:  vectorSize =  2; break;
        case LSC_V3:  vectorSize =  3; break;
        case LSC_V4:  vectorSize =  4; break;
        case LSC_V8:  vectorSize =  8; break;
        case LSC_V16: vectorSize = 16; break;
        case LSC_V32: vectorSize = 32; break;
        case LSC_V64: vectorSize = 64; break;
        default:
            vsym << "x?";
        }
        bool opIsBlock2d =
            op == SendOp::LOAD_BLOCK2D || op == SendOp::STORE_BLOCK2D;
        auto transposed =
            decodeDescBitField(
                "DataOrder", 15,
                "non-transposed (vector elements are in successive registers)",
                "transposed (vector elements are in the same register)");
        if (vectorSize > 1 || transposed && !opIsBlock2d) {
            vsym << 'x' << vectorSize;
        }
        if (transposed && op == SendOp::LOAD_STATUS) {
            error(15, 1, "data order must be non-transposed for this op");
        }
        std::stringstream vdesc;
        vdesc << "each address accesses " << vectorSize << " element";
        if (vectorSize != 1)
            vdesc << "s";
        if (!opIsBlock2d)
            addField("VecSize", 12, 3, vecSzEncd, vdesc.str());
        if (transposed) {
            vsym << 't';
            extraAttrs |= MessageInfo::Attr::TRANSPOSED;
            expectedExecSize = 1; // all transpose messages are SIMD1
        }

        if (op == SendOp::LOAD_BLOCK2D) {
            bool vnni = decodeDescBitField(
                "Block2dVnniTransform", 7, "disabled", "enabled");
            if (vnni)
                vsym << 'v';
        }

        vectorSuffixSyntax = vsym.str();
    }

    void decodeLscVecSizeQuad() {
        // LSC channels *enabled* is the inverse of the old messages
        // because the old ChMask used in untyped old (scatter4/gather4)
        // was really a channel "disable" mask
        auto chEn = getDescBits(12, 4);
        vectorSize = 0;
        for (int i = 0; i < 4; ++i) {
            if ((1<<i) & chEn) {
                vectorSize++;
            }
        }
        extraAttrs |= MessageInfo::Attr::HAS_CHMASK;

        std::stringstream vsym;
        vsym << ".";
        if (chEn & 1)
            vsym << "x";
        if (chEn & 2)
            vsym << "y";
        if (chEn & 4)
            vsym << "z";
        if (chEn & 8)
            vsym << "w";
        vectorSuffixSyntax = vsym.str();

        addField("CompEn", 12, 4, chEn, vsym.str());
    }


    ///////////////////////////////////////////////////////////////////////////
    void decodeLscMessage(
        const char *doc,
        std::string msgDesc,
        SendOp lscOp)
    {
        const std::string symbol = ToSyntax(lscOp);
        op = lscOp;

        bool opSupportsUvr =
            lscOp == SendOp::LOAD_QUAD ||
            lscOp == SendOp::STORE_QUAD ||
            lscOp == SendOp::STORE_UNCOMPRESSED_QUAD;
        if (sfid == SFID::TGM && opSupportsUvr) {
            extraAttrs |= MessageInfo::Attr::HAS_UVRLOD;
        }

        addField("Opcode", 0, 6, getDescBits(0, 6), symbol);

        setDoc(doc);
        //
        if (hasPayloadSizesInDesc() &&
            exDesc.isImm() && (exDesc.imm & 0x7FF))
        {
            // bit 11 may or may not be available
            error(0, 12, "ExDesc[11:0] must be 0 on this platform");
        }
        //
        SendDesc surfaceId(0x0);
        AddrType addrType = decodeLscAddrType(surfaceId);
        //
        if (op == SendOp::LOAD_BLOCK2D || op == SendOp::STORE_BLOCK2D) {
            addrSizeBits = 64;
            addrSizeSyntax = "a64";
        } else {
            decodeLscAddrSize();
        }
        //
        decodeLscDataSize();
        //
        expectedExecSize =
            op == SendOp::LOAD_BLOCK2D || op == SendOp::STORE_BLOCK2D ? 1 :
                sfid == SFID::TGM ? DEFAULT_EXEC_SIZE/2 : DEFAULT_EXEC_SIZE;
        decodeLscVecSize();
        //
        if (sfid == SFID::TGM)
            extraAttrs |= MessageInfo::Attr::TYPED;
        if (sfid == SFID::SLM)
            extraAttrs |= MessageInfo::Attr::SLM;
        //
        CacheOpt l1 = CacheOpt::DEFAULT, l3 = CacheOpt::DEFAULT;
        const auto &opInfo = lookupSendOp(op);
        bool hasCc =
            opInfo.isLoad() || opInfo.isStore() || opInfo.isAtomic();
        if (sfid != SFID::SLM && hasCc) {
            decodeLscCacheControl(op, l1, l3);
        }
        //
        result.syntax.mnemonic = symbol;
        //
        result.syntax.controls += '.';
        result.syntax.controls += dataTypePrefixSyntax;
        result.syntax.controls += vectorSuffixSyntax;
        if (!addrSizeSyntax.empty()) {
            result.syntax.controls += '.';
            result.syntax.controls += addrSizeSyntax;
        }
        if (!cacheControlSyntax.empty()) {
            result.syntax.controls += cacheControlSyntax;
        }
        //
        setScatterGatherOpX(
            symbolFromSyntax(),
            msgDesc,
            op,
            addrType,
            surfaceId,
            l1,
            l3,
            addrSizeBits,
            dataSizeRegBits,
            dataSizeMemBits,
            vectorSize,
            int(instExecSize),
            extraAttrs);
        if (lookupSendOp(op).hasChMask()) {
            result.info.channelsEnabled = getDescBits(12, 4);
            if (result.info.channelsEnabled == 0)
                error(12, 4, "no channels enabled on quad message");
        }
    }


    void setLscAtomicMessage(
        const char *doc,
        std::string msgDesc,
        SendOp atOp)
    {
        extraAttrs |=
            getDescBits(20, 5) != 0 ?
                MessageInfo::Attr::ATOMIC_RETURNS : MessageInfo::Attr::NONE;
        if (sfid == SFID::TGM)
            extraAttrs |= MessageInfo::Attr::HAS_UVRLOD;
        decodeLscMessage(doc, msgDesc, atOp);
    }


    void tryDecodeLsc() {
        int lscOp = getDescBits(0, 6); // Opcode[5:0]
        switch (lscOp) {
        case LSC_LOAD:
            decodeLscMessage(
                chooseDoc(nullptr, "53523", "63970"),
                "gathering load",
                SendOp::LOAD);
            break;
        case LSC_STORE:
            decodeLscMessage(
                chooseDoc(nullptr, "53523", "63980"),
                "scattering store",
                SendOp::STORE);
            break;
        case LSC_STORE_UNCOMPRESSED:
            decodeLscMessage(
                chooseDoc(nullptr, "53532", "63984"),
                "scattering store uncompressed",
                SendOp::STORE_UNCOMPRESSED);
            break;
        case LSC_STORE_UNCOMPRESSED_QUAD:
            decodeLscMessage(
                chooseDoc(nullptr, "55224", "63985"),
                "store quad uncompressed",
                SendOp::STORE_UNCOMPRESSED_QUAD);
            break;
        case LSC_LOAD_QUAD:
            decodeLscMessage(
                chooseDoc(nullptr, "53527", "63977"),
                "quad load (a.k.a. load_cmask)",
                SendOp::LOAD_QUAD);
            break;
        case LSC_STORE_QUAD:
            decodeLscMessage(
                chooseDoc(nullptr, "53527", "63983"),
                "quad store (a.k.a. store_cmask)",
                SendOp::STORE_QUAD);
            break;
        case LSC_LOAD_STRIDED:
            decodeLscMessage(
                chooseDoc(nullptr, "53525", "63976"),
                "strided load (a.k.a load_block)",
                SendOp::LOAD_STRIDED);
            break;
        case LSC_STORE_STRIDED:
            decodeLscMessage(
                chooseDoc(nullptr, "53526", "63982"),
                "strided store (a.k.a store_block)",
                SendOp::STORE_STRIDED);
            break;
        case LSC_LOAD_BLOCK2D:
            decodeLscMessage(
                chooseDoc(nullptr, "53680", "63972"),
                "block2d load",
                SendOp::LOAD_BLOCK2D);
            break;
        case LSC_STORE_BLOCK2D:
            decodeLscMessage(
                chooseDoc(nullptr, "53530", "63981"),
                "block2d store",
                SendOp::STORE_BLOCK2D);
            break;
        case LSC_ATOMIC_IINC:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53538", "63955"),
                "atomic integer increment",
                SendOp::ATOMIC_IINC);
            break;
        case LSC_ATOMIC_IDEC:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53539", "63949"),
                "atomic integer decrement",
                SendOp::ATOMIC_IDEC);
            break;
        case LSC_ATOMIC_LOAD:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53540", "63956"),
                "atomic load",
                SendOp::ATOMIC_LOAD);
            break;
        case LSC_ATOMIC_STORE:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53541", "63960"),
                "atomic store",
                SendOp::ATOMIC_STORE);
            break;
        case LSC_ATOMIC_IADD:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53542", "63946"),
                "atomic integer add",
                SendOp::ATOMIC_IADD);
            break;
        case LSC_ATOMIC_ISUB:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53543", "63961"),
                "atomic integer subtract",
                SendOp::ATOMIC_ISUB);
            break;
        case LSC_ATOMIC_SMIN:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53544", "63958"),
                "atomic signed-integer minimum",
                SendOp::ATOMIC_SMIN);
            break;
        case LSC_ATOMIC_SMAX:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53545", "63957"),
                "atomic signed-integer maximum",
                SendOp::ATOMIC_SMAX);
            break;
        case LSC_ATOMIC_UMIN:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53546", "63963"),
                "atomic unsigned-integer minimum",
                SendOp::ATOMIC_UMIN);
            break;
        case LSC_ATOMIC_UMAX:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53547", "63962"),
                "atomic unsigned-integer maximum",
                SendOp::ATOMIC_UMAX);
            break;
        case LSC_ATOMIC_ICAS:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53555", "63948"),
                "atomic integer compare and swap",
                SendOp::ATOMIC_ICAS);
            break;
        case LSC_ATOMIC_FADD:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53548", "63950"),
                "atomic float add",
                SendOp::ATOMIC_FADD);
            break;
        case LSC_ATOMIC_FSUB:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53549", "63954"),
                "atomic float subtract",
                SendOp::ATOMIC_FSUB);
            break;
        case LSC_ATOMIC_FMIN:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53550", "63953"),
                "atomic float minimum",
                SendOp::ATOMIC_FMIN);
            break;
        case LSC_ATOMIC_FMAX:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53551", "63952"),
                "atomic float maximum",
                SendOp::ATOMIC_FMAX);
            break;
        case LSC_ATOMIC_FCAS:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53556", "63951"),
                "atomic float compare and swap",
                SendOp::ATOMIC_FCAS);
            break;
        case LSC_ATOMIC_AND:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53552", "63947"),
                "atomic logical and",
                SendOp::ATOMIC_AND);
            break;
        case LSC_ATOMIC_OR:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53553", "63959"),
                "atomic logical or",
                SendOp::ATOMIC_OR);
            break;
        case LSC_ATOMIC_XOR:
            setLscAtomicMessage(
                chooseDoc(nullptr, "53554", "63964"),
                "atomic logical xor",
                SendOp::ATOMIC_XOR);
            break;
        case LSC_CCS:
            decodeLscCcs();
            break;
        case LSC_RSI: {
            addField("Opcode", 0, 6, getDescBits(0, 6), "read_state");
            setDoc(nullptr, "54000", "63979");
            //
            std::stringstream descs;
            descs << "read state information";
            result.syntax.mnemonic = "read_state";
            //
            SendDesc surfId = 0;
            auto at = decodeLscAddrType(surfId, false);
            //
            // XeHPG returns 2 GRF, XeHPC+ only 1
            // #54152
            int rlen = platform() == Platform::XE_HPG ? 2 : 1;
            setSpecialOpX(
                result.syntax.mnemonic,
                descs.str(),
                SendOp::READ_STATE,
                at,
                surfId,
                1, // mlen = 1 (U,V,R,LOD)
                rlen);
            result.info.addrSizeBits = 64;
            result.info.execWidth = 1;
            result.info.attributeSet |= MessageInfo::Attr::HAS_UVRLOD;
            result.info.attributeSet |= MessageInfo::Attr::TRANSPOSED;
            break;
        }
        case LSC_FENCE:
            decodeLscFence();
            break;
        case LSC_LOAD_STATUS:
            if (getDescBit(15)) {
                error(15, 1, "transpose forbidden on load_status");
            }
            if (getDescBits(20, 5) != 1) {
                error(20, 5, "load_status must have rlen (Desc[24:20] == 1)");
            }
            decodeLscMessage(
                chooseDoc(nullptr, "53531", "63978"),
                "load status",
                SendOp::LOAD_STATUS);
            break;
        default:
            addField("Opcode", 0, 6,
                getDescBits(0, 6), "invalid message opcode");
            error(0, 6, "unsupported message opcode");
            return;
        }
    }

    void decodeLscCcs() {
        addField("Opcode", 0, 6,
            static_cast<uint32_t>(LSC_CCS),
            "compression-state control");
        //
        std::stringstream descs;
        result.syntax.mnemonic = "ccs";
        descs << "compression-state control";
        auto ccsOpBits = getDescBits(17, 3);
        SendOp sop = SendOp::INVALID;
        std::string opDesc;
        switch (ccsOpBits) {
        case 0:
            sop = SendOp::CCS_PC;
            result.syntax.mnemonic += "_pc";
            opDesc = " page clear (64k)";
            setDoc(nullptr, "53536", "63965");
            break;
        case 1:
            sop = SendOp::CCS_SC;
            result.syntax.mnemonic += "_sc";
            opDesc = " sector clear (2-cachelines)";
            setDoc(nullptr, "53534", "63967");
            result.syntax.controls += vectorSuffixSyntax;
            break;
        case 2:
            sop = SendOp::CCS_PU;
            result.syntax.mnemonic += "_pu";
            opDesc = " page uncompress (64k)";
            setDoc(nullptr, "53537", "63966");
            break;
        case 3:
            sop = SendOp::CCS_SU;
            result.syntax.mnemonic += "_su";
            opDesc = " sector uncompress (2-cachelines)";
            setDoc(nullptr, "53535", "63968");
            result.syntax.controls += vectorSuffixSyntax;
            break;
        default: {
            std::stringstream ss;
            ss << ".0x" << std::hex << std::uppercase << ccsOpBits;
            result.syntax.controls += ss.str();
            opDesc = "invalid ccs sop";
            error(17, 3, "invalid ccs sop");
        }
        } // switch
        descs << opDesc;
        //
        addField("CcsOp", 17, 3, ccsOpBits, opDesc);
        //
        SendDesc surfId = 0;
        auto at = decodeLscAddrType(surfId);
        if (ccsOpBits == 0 || ccsOpBits == 2) {
            // page operations: pc, pu
            if (at != AddrType::FLAT)
                error(29, 2, "ccs_{pcc,pcu} requires FLAT address type");
            std::stringstream dummy;
            decodeLscAddrSize();
            if (addrSizeBits != 64)
                error(7, 2, "AddrSize must be A64");
            result.info.execWidth = 1;
            expectedExecSize = 1;
            // sector uncompress has addresses
            // FIXME: I could derive this via exec size and a64
            int mlen =
                ccsOpBits == 1 || ccsOpBits == 3 ?
                    4 : // A64_PAYLOAD_SIMT32 = 4 regs
                    1;  // A64_PAYLOAD_SIMT1  = 1 reg
            int rlen = 0;  // always 0
            setSpecialOpX(
                symbolFromSyntax(),
                descs.str(),
                sop,
                at,
                surfId,
                mlen,
                rlen);
        } else {
            // sector operations
            ///
            // these are vector messages
            expectedExecSize = DEFAULT_EXEC_SIZE;
            // const int SECTOR_SIZE_BITS = 128*8;
            // result.syntax.controls += ".d1024";
            result.syntax.controls += vectorSuffixSyntax;
            result.syntax.controls +=
                addrSizeBits == 64 ? ".a64" : ".a32";
            //
            setScatterGatherOp(
                symbolFromSyntax(),
                descs.str(),
                sop,
                at,
                surfId,
                addrSizeBits,
                0, // dateSize = 0; nothing returned
                vectorSize,
                DEFAULT_EXEC_SIZE,
                extraAttrs);
        }
    }

    void decodeLscFence() {
        addField("Opcode", 0, 6, getDescBits(0, 6), "fence");
        setDoc(nullptr, "53533", "63969");
        //
        std::stringstream descs;
        result.syntax.mnemonic = "fence";
        descs << "fence";
        //
        std::stringstream fenceOpts;
        addLscFenceFields(fenceOpts, descs);
        result.syntax.controls += fenceOpts.str();
        //
        setSpecialOpX(
            symbolFromSyntax(),
            descs.str(),
            SendOp::FENCE,
            AddrType::FLAT,
            0, // no surface
            1, // mlen = 1
            0); // rlen = 0
    }
}; // MessageDecoderLSC


void iga::decodeDescriptorsLSC(
    Platform platform, SFID sfid, ExecSize execSize,
    SendDesc exDesc, SendDesc desc,
    DecodeResult &result)
{
    MessageDecoderLSC md(
        platform, sfid, execSize,
        exDesc, desc,
        result);
    md.tryDecodeLsc();
}

// descriptor bits [19:17]: cache control
static bool encLdStVecCachingBits17_19(
    SendOp op,
    CacheOpt cachingL1, CacheOpt cachingL3,
    SendDesc &desc)
{
    const auto &opInfo = lookupSendOp(op);
    bool isLd = opInfo.isLoad();
    bool isSt = opInfo.isStore();
    bool isAt = opInfo.isAtomic();
    bool isStAt = isSt || isAt;
    auto ccMatches = [&](CacheOpt l1, CacheOpt l3, uint32_t enc) {
        if (l1 == cachingL1 && l3 == cachingL3) {
            desc.imm |= enc << 17;
            return true;
        }
        return false;
    };
    bool matched =
        ccMatches(CacheOpt::DEFAULT, CacheOpt::DEFAULT, LSC_DF_DF) ||
        //
        ccMatches(CacheOpt::UNCACHED, CacheOpt::UNCACHED, LSC_UC_UC) ||
        //
        (isLd &&
            ccMatches(CacheOpt::UNCACHED, CacheOpt::CACHED, LSC_UC_CA)) ||
        (isStAt &&
            ccMatches(CacheOpt::UNCACHED, CacheOpt::WRITEBACK, LSC_UC_WB)) ||
        //
        (isLd &&
            ccMatches(CacheOpt::CACHED, CacheOpt::UNCACHED, LSC_CA_UC)) ||
        (isSt &&
            ccMatches(
                CacheOpt::WRITETHROUGH, CacheOpt::UNCACHED, LSC_WT_UC)) ||
        //
        (isLd &&
            ccMatches(CacheOpt::CACHED, CacheOpt::CACHED, LSC_CA_CA)) ||
        (isSt &&
            ccMatches(
                CacheOpt::WRITETHROUGH, CacheOpt::WRITEBACK, LSC_WT_WB)) ||
        //
        ccMatches(CacheOpt::STREAMING, CacheOpt::UNCACHED, LSC_ST_UC) ||
        //
        (isLd &&
            ccMatches(CacheOpt::STREAMING, CacheOpt::CACHED, LSC_ST_CA)) ||
        (isSt &&
            ccMatches(CacheOpt::STREAMING, CacheOpt::WRITEBACK, LSC_ST_WB)) ||
        //
        (isLd &&
            ccMatches(
                CacheOpt::READINVALIDATE, CacheOpt::CACHED, LSC_RI_CA)) ||
        (isSt &&
            ccMatches(CacheOpt::WRITEBACK, CacheOpt::WRITEBACK, LSC_WB_WB));
    return matched;
}


static bool encLdStVecCaching(
    const Platform& p,
    SendOp op,
    CacheOpt cachingL1, CacheOpt cachingL3,
    SendDesc &desc)
{

    return encLdStVecCachingBits17_19(op, cachingL1, cachingL3, desc);
}

static bool encLdStVec(
    Platform p,
    const VectorMessageArgs &vma,
    SendDesc &exDesc,
    SendDesc &desc,
    std::string &err)
{
    desc = 0x0;
    exDesc = 0x0;
    //
    bool hasCMask = false;
    switch (vma.op) {
    case SendOp::LOAD:            desc.imm |= LSC_LOAD; break;
    case SendOp::LOAD_STRIDED:    desc.imm |= LSC_LOAD_STRIDED; break;
    case SendOp::LOAD_QUAD:
        desc.imm |= LSC_LOAD_QUAD;
        hasCMask = true;
        break;
    case SendOp::LOAD_BLOCK2D:    desc.imm |= LSC_LOAD_BLOCK2D; break;
    //
    case SendOp::STORE:           desc.imm |= LSC_STORE; break;
    case SendOp::STORE_STRIDED:   desc.imm |= LSC_STORE_STRIDED; break;
    case SendOp::STORE_QUAD:
        desc.imm |= LSC_STORE_QUAD;
        hasCMask = true;
        break;
    case SendOp::STORE_UNCOMPRESSED:
        desc.imm |= LSC_STORE_UNCOMPRESSED;
        break;
    case SendOp::STORE_UNCOMPRESSED_QUAD:
        desc.imm |= LSC_STORE_UNCOMPRESSED_QUAD;
        hasCMask = true;
        break;
    case SendOp::STORE_BLOCK2D:   desc.imm |= LSC_STORE_BLOCK2D; break;
    //
    case SendOp::ATOMIC_AND:      desc.imm |= LSC_ATOMIC_AND;   break;
    case SendOp::ATOMIC_FADD:     desc.imm |= LSC_ATOMIC_FADD;  break;
    case SendOp::ATOMIC_FCAS:     desc.imm |= LSC_ATOMIC_FCAS;  break;
    case SendOp::ATOMIC_FMAX:     desc.imm |= LSC_ATOMIC_FMAX;  break;
    case SendOp::ATOMIC_FMIN:     desc.imm |= LSC_ATOMIC_FMIN;  break;
    case SendOp::ATOMIC_FSUB:     desc.imm |= LSC_ATOMIC_FSUB;  break;
    case SendOp::ATOMIC_IADD:     desc.imm |= LSC_ATOMIC_IADD;  break;
    case SendOp::ATOMIC_ICAS:     desc.imm |= LSC_ATOMIC_ICAS;  break;
    case SendOp::ATOMIC_IDEC:     desc.imm |= LSC_ATOMIC_IDEC;  break;
    case SendOp::ATOMIC_IINC:     desc.imm |= LSC_ATOMIC_IINC;  break;
    case SendOp::ATOMIC_ISUB:     desc.imm |= LSC_ATOMIC_ISUB;  break;
    case SendOp::ATOMIC_LOAD:     desc.imm |= LSC_ATOMIC_LOAD;  break;
    case SendOp::ATOMIC_OR:       desc.imm |= LSC_ATOMIC_OR;    break;
    case SendOp::ATOMIC_SMAX:     desc.imm |= LSC_ATOMIC_SMAX;  break;
    case SendOp::ATOMIC_SMIN:     desc.imm |= LSC_ATOMIC_SMIN;  break;
    case SendOp::ATOMIC_STORE:    desc.imm |= LSC_ATOMIC_STORE; break;
    case SendOp::ATOMIC_UMAX:     desc.imm |= LSC_ATOMIC_UMAX;  break;
    case SendOp::ATOMIC_UMIN:     desc.imm |= LSC_ATOMIC_UMIN;  break;
    case SendOp::ATOMIC_XOR:      desc.imm |= LSC_ATOMIC_XOR;   break;
    default:
        err = "unsupported op";
        return false;
    }
    bool isBlock2d =
        vma.op == SendOp::LOAD_BLOCK2D || vma.op == SendOp::STORE_BLOCK2D;
    bool isBlock2dTyped = isBlock2d && vma.sfid == SFID::TGM;
    bool isBlock2dUntyped = isBlock2d && vma.sfid != SFID::TGM;
    bool hasAddrSizeField = !isBlock2d;

    //
    ////////////////////////////////////////
    // data size
    uint32_t dszEnc = LSC_D8;
    if (isBlock2dTyped &&
        (vma.dataSizeReg != 32 || vma.dataSizeMem != 32))
    {
        err = "block2d.tgm must be d32";
        return false;
    }
    if (vma.dataSizeMem == vma.dataSizeReg) {
        switch (vma.dataSizeMem) {
        case  8: dszEnc = LSC_D8; break;
        case 16: dszEnc = LSC_D16; break;
        case 32: dszEnc = LSC_D32; break;
        case 64: dszEnc = LSC_D64; break;
        default: err = "invalid data size"; return false;
        }
    } else if (vma.dataSizeMem == 8 && vma.dataSizeReg == 32) {
        dszEnc = LSC_D8U32;
    } else if (vma.dataSizeMem == 16 && vma.dataSizeReg == 32) {
        if (vma.dataSizeExpandHigh) {
            dszEnc = LSC_D16U32H;
        } else {
            dszEnc = LSC_D16U32;
        }
    } else {
        err = "invalid data type";
        return false;
    }
    if (!isBlock2dTyped)
        desc.imm |= dszEnc << 9;
    //
    ////////////////////////////////////////
    // vector size
    if (hasCMask) {
        if (vma.dataComponentMask & ~0xF) {
            err = "invalid component mask";
            return false;
        }
        desc.imm |= vma.dataComponentMask << 12;
    } else if (isBlock2d) {
        if (isBlock2dTyped && vma.dataVnni) {
            err = "block2d.tgm forbids VNNI";
            return false;
        } else if (isBlock2dTyped && vma.dataTranspose) {
            err = "block2d.tgm forbids transpose data order";
            return false;
        }
        if (vma.dataVnni)
            desc.imm |= 1 << 7;
        if (vma.dataTranspose)
            desc.imm |= 1 << 15;
    } else {
        uint32_t vecEnc = LSC_V1;
        switch (vma.dataVectorSize) {
        case  1: vecEnc = LSC_V1; break;
        case  2: vecEnc = LSC_V2; break;
        case  3: vecEnc = LSC_V3; break;
        case  4: vecEnc = LSC_V4; break;
        case  8: vecEnc = LSC_V8; break;
        case 16: vecEnc = LSC_V16; break;
        case 32: vecEnc = LSC_V32; break;
        case 64: vecEnc = LSC_V64; break;
        default: err = "invalid vector size"; break;
        }
        if (vma.isAtomic() && vma.dataVectorSize != 1) {
            err = "atomics do not support vector operations";
            return false;
        }
        if (vma.dataVnni) {
            err = "vnni only valid on block2d operations";
            return false;
        }
        //
        desc.imm |= vecEnc << 12;
        //
        if (vma.dataTranspose) {
            desc.imm |= 1 << 15;
            //
            if (vma.isAtomic()) {
                err = "atomics do not support transpose operations";
                return false;
            }
        }
    } // end vec non-cmask case
    //
    ////////////////////////////////////////
    // caching options
    if (vma.isAtomic() &&
        vma.cachingL1 != CacheOpt::DEFAULT &&
        vma.cachingL1 != CacheOpt::UNCACHED)
    {
        err = "atomic L1 must be an uncached option";
        return false;
    } else {
        if (!encLdStVecCaching(p, vma.op, vma.cachingL1, vma.cachingL3, desc)) {
            err = "invalid cache-control combination";
            return false;
        }
    }
    //
    ////////////////////////////////////////
    // address size
    uint32_t asEnc = 0x0;
    switch (vma.addrSize) {
    case 16: asEnc = LSC_A16; break;
    case 32: asEnc = LSC_A32; break;
    case 64: asEnc = LSC_A64; break;
    default:
        err = "unsupported address size";
        return false;
    }
    if (isBlock2dTyped && vma.addrSize != 32) {
        err = "block2d.typed address size must be A32";
        return false;
    }
    if (isBlock2dUntyped && vma.addrSize != 64) {
        err = "block2d untyped address size must be A64";
        return false;
    }
    if (hasAddrSizeField) {
        desc.imm |= asEnc << 7;
    }
    //
    ////////////////////////////////////////
    // address type
    uint32_t atEnc = 0x0;
    switch (vma.addrType) {
    case AddrType::FLAT: atEnc = LSC_AT_FLAT; break;
    case AddrType::BSS:  atEnc = LSC_AT_BSS; break;
    case AddrType::SS:   atEnc = LSC_AT_SS; break;
    case AddrType::BTI:  atEnc = LSC_AT_BTI; break;
    default:
        err = "unsupported address type";
        return false;
    }
    if (isBlock2dTyped && vma.addrType == AddrType::FLAT) {
        err = "block2d.typed forbids flat address";
        return false;
    }
    desc.imm |= atEnc << 29;
    //
    // store the surface
    if (vma.addrType != AddrType::FLAT) {
        // use exDesc
        if (vma.addrType == AddrType::BTI && !vma.addrSurface.isReg()) {
            exDesc = vma.addrSurface.imm << 24;
        } else {
            exDesc = vma.addrSurface;
        }
    }
    //
    if (vma.addrType != AddrType::FLAT && vma.sfid == SFID::SLM) {
        err = "SLM requires flat address type";
        return false;
    }
    ////////////////////////////////////////
    // address scale factor
    if (vma.addrScale != 1) {
        if (true) { // disable if address scaling is ever added
            err = "address scaling not supported on this platform";
            return false;
        }
        int vlen = vma.elementsPerAddress();
        int bytesPerElem = vma.dataSizeMem * vlen / 8;
        uint32_t addrScEnc = LSC_SCALE_NONE;
        if (vma.addrScale > 32) {
            err = "scale value is too large";
            return false;
        } else if (vma.addrScale == bytesPerElem) {
            addrScEnc = LSC_SCALE_1X;
        } else if (vma.addrScale == 2*bytesPerElem) {
            addrScEnc = LSC_SCALE_2X;
        } else if (vma.addrScale == 4*bytesPerElem) {
            addrScEnc = LSC_SCALE_4X;
        } else {
            std::stringstream ss;
            ss <<
                "invalid scaling factor (must be " <<
                1*bytesPerElem << ", " <<
                2*bytesPerElem << ", or " <<
                4*bytesPerElem << ")";
            err = ss.str();
            return false;
        }
        desc.imm |= addrScEnc << 22;
    }
    //
    ////////////////////////////////////////
    // address immediate offset
    bool hasAddrImmOffset = vma.addrOffset != 0;
    hasAddrImmOffset |= vma.addrOffsetX != 0;
    hasAddrImmOffset |= vma.addrOffsetY != 0;
    if (hasAddrImmOffset) {
        bool platformSupportsAddrOff = false;
        if (platformSupportsAddrOff) {
            err = "address immediate offset not supported on this platform";
            return false;
        }

    } // else: addrOffset == 0

    ////////////////////////////////////////
    // set the surface object
    if (vma.addrType == AddrType::FLAT) {
        // IR normalization
        if (!vma.addrSurface.isImm() || vma.addrSurface.imm != 0) {
            err = "malformed IR: flat address model must have surface = 0";
            return false;
        }
    }

    // XeHPG+ have surface in ExDesc
    if (vma.addrType == AddrType::BTI && vma.addrSurface.isImm()) {
        // BTI takes the high byte
        if (vma.addrSurface.imm > 0xFF) {
            err = "surface index too large for BTI";
            return false;
        }
        exDesc.imm |= vma.addrSurface.imm << 24;
    } else if (vma.addrType != AddrType::FLAT) {
        uint32_t ZERO_MASK = 0xFFF;
        std::string highBit = "11";

        // if BTI reg or BSS/SS reg/imm with just copy
        // BSS/SS with imm, value is already aligned
        if (vma.addrType != AddrType::BTI &&
            vma.addrSurface.isImm() &&
            (vma.addrSurface.imm & ZERO_MASK) != 0)
        {
            err = "BSS/SS with immediate descriptor require "
                "ExDesc[" + highBit + ":0] to be 0";
            return false;
        }
        exDesc = vma.addrSurface;
    }
    //
    return true;
}


bool iga::encodeDescriptorsLSC(
    Platform p,
    const VectorMessageArgs &vma,
    SendDesc &exDesc,
    SendDesc &desc,
    std::string &err)
{
    if (!sendOpSupportsSyntax(p, vma.op, vma.sfid)) {
        err = "unsupported message for SFID";
        return false;
    }
    return encLdStVec(p, vma,
        exDesc, desc, err);
}
