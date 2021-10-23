/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MessageDecoder.hpp"

#include <tuple>

using namespace iga;

struct MessageDecoderOther : MessageDecoderLegacy
{
    MessageDecoderOther(
        Platform _platform, SFID _sfid, ExecSize _execSize,
        SendDesc _exDesc, SendDesc _desc,
        DecodeResult &_result)
        : MessageDecoderLegacy(
            _platform, _sfid, _execSize,
            _exDesc, _desc, _result)
    {
    }

    void tryDecodeOther() {
        switch (sfid) {
        case SFID::GTWY: tryDecodeGTWY();   break;
        case SFID::RC:   tryDecodeRC();   break;
        case SFID::RTA:  tryDecodeRTA(); break;
        case SFID::BTD:  tryDecodeBTD(); break;
        case SFID::SMPL: tryDecodeSMPL(); break;
        case SFID::TS:   tryDecodeTS();   break;
        case SFID::URB:  tryDecodeURB();   break;
        default: error(0, 0, "invalid sfid");
        }
    }

    void tryDecodeGTWY();
    void tryDecodeRC();
    void tryDecodeRTA();
    void tryDecodeBTD();
    void tryDecodeSMPL();
    void tryDecodeTS();
    void tryDecodeURB();
}; // MessageDecodeOther


void MessageDecoderOther::tryDecodeGTWY() {

    //
    using GatewayElement = std::tuple<uint32_t,SendOp,const char *,const char *,const char *>;
    static constexpr GatewayElement GATEWAY_MESSAGES[] = {
        // before this TS had its own SFID
        {0x0, SendOp::EOT, nullptr, "54044", "57488"}, // MSD_TS_EOT
        {0x1, SendOp::SIGNAL, "33508", "47927", "57492"}, // MSD_SIGNAL_EVENT
        {0x2, SendOp::MONITOR, "33512", "47925", "57490"}, // MSD_MONITOR_EVENT
        {0x3, SendOp::UNMONITOR, "33513", "47926", "57491"}, // MSD_MONITOR_NO_EVENT
        // no 0x5
        {0x4, SendOp::BARRIER, "33524", "47924", "57489"}, // MSD_BARRIER
        {0x6, SendOp::WAIT, "33514", "47928", "57493"}, // MSD_WAIT_FOR_EVENT
    };

    const GatewayElement *ge = nullptr;
    uint32_t opBits = getDescBits(0, 3); // [2:0]
    for (const auto &g: GATEWAY_MESSAGES) {
        if (std::get<0>(g) == opBits) {
            ge = &g;
            break;
        }
    }
    if (ge == nullptr) {
        error(0, 2, "unsupported GTWY op");
        return;
    }
    std::stringstream sym, desc;
    const SendOpDefinition &sod = lookupSendOp(std::get<1>(*ge));
    if (!sod.isValid()) {
        // should be in the table
        error(0, 2, "INTERNAL ERROR: cannot find GTWY op");
        return;
    }

    sym << sod.mnemonic;
    desc << sod.description;
    int expectMlen = 0, expectRlen = 0;
    if (sod.op == SendOp::EOT) {
        if (platform() < Platform::XE_HPG) {
            error(0, 3, "Gateway EOT not available on this platform");
        }
        expectMlen = 1;
    }
    switch (sod.op) {
    case SendOp::SIGNAL:
    case SendOp::MONITOR: // C.f. MDP_EVENT
    case SendOp::UNMONITOR: // C.f. MDP_NO_EVENT
    case SendOp::BARRIER: // C.f. MDP_Barrier
    case SendOp::WAIT: // C.f. MDP_Timeout
        expectMlen = 1;
        break;
    default: break;
    }
    switch (sod.op) {
    case SendOp::BARRIER:
        // XeHP+ no register is returned
        expectRlen = platform() >= Platform::XE_HP ? 0 : 1;
        if (platform() >= Platform::XE) {
            decodeDescBitField("ActiveThreadsOnly", 11,
                "only active threads participate");
        }
        break;
    default: break;
    }

    addField("GatewayOpcode", 0, 3, opBits, desc.str());
    setSpecialOpX(
        sym.str(), desc.str(), sod.op,
        AddrType::INVALID, SendDesc(0),
        expectMlen, // mlen
        expectRlen, // rlen
        MessageInfo::Attr::NONE);
    decodeMDC_HF(); // all gateway messages forbid a header
    const char *doc =
        chooseDoc(std::get<2>(*ge), std::get<3>(*ge), std::get<4>(*ge));
    setDoc(doc);
}

void MessageDecoderOther::tryDecodeRC()
{
    // PSEUDO syntax
    //   rtw.{f16,f32}.{simd8,simd16,rep16,lo8ds,hi8ds}{.c}{.ps}{.lrts}{.sgh}
    //   rtr.{f32}.{simd8,simd16}{.ps}{.sgh}
    std::stringstream sym;

    static const uint32_t RT_READ = 0xD;
    static const uint32_t RT_WRITE = 0xC;
    static const uint32_t RTR_SIMD = 0x1;

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
        error(14, 4, "unsupported RC op");
    }
    addField("MessageTypeRC", 14, 4, mt, descSfx);

    std::stringstream descs;
    int bitSize =
        decodeDescBitField("DataSize", 30, "FP32", "FP16") == 0 ? 32 : 16;
    if (bitSize == 32) {
        descs << "full-precision";
        sym << ".f32";
    } else {
        descs << "half-precision";
        sym << ".f16";
        if (mt == RT_READ)
            warning(30, 1, "half-precision not supported on render target read");
    }
    descs << " " << descSfx;

    std::stringstream subopSymSs;
    auto subOpBits = getDescBits(8, 3);
    int execSize = 0;
    switch (mt) {
    case RT_WRITE:
        switch (subOpBits) {
        case 0x0:
            execSize = DEFAULT_EXEC_SIZE;
            subopSymSs << ".simd" << execSize;
            descs << " SIMD" << execSize;
            break;
        case 0x1:
            execSize = DEFAULT_EXEC_SIZE;
            subopSymSs << ".rep" << execSize;
            descs << " replicated SIMD" << execSize;
            break;
        case 0x2:
            execSize = DEFAULT_EXEC_SIZE/2;
            subopSymSs << ".lo" << execSize << "ds";
            descs << " of low SIMD" << execSize;
            break;
        case 0x3:
            execSize = DEFAULT_EXEC_SIZE/2;
            subopSymSs << ".hi" << execSize << "ds";
            descs << " of high SIMD" << execSize;
            break;
        case 0x4:
            execSize = DEFAULT_EXEC_SIZE/2;
            subopSymSs << ".simd" << execSize;
            descs << " SIMD" << execSize;
            break;
        default:
            sym << ".???";
            descs << "unknown write subop";
            error(8, 3, "unknown write subop");
            break;
        }
        break;
    case RT_READ:
        switch (subOpBits) {
        case 0x0:
            execSize = DEFAULT_EXEC_SIZE;
            subopSymSs << ".simd" << execSize;
            descs << " SIMD" << execSize;
            break;
        case 0x1:
            execSize = DEFAULT_EXEC_SIZE/2;
            subopSymSs << ".simd" << execSize;
            descs << " SIMD" << execSize;
            break;
        default:
            sym << ".???";
            descs << "unknown read subop";
            error(8, 3, "unknown read subop");
            break;
        }
        break;
    default:
        break;
    }
    addField("Subop", 8, 3, subOpBits, subopSymSs.str());
    sym << subopSymSs.str();

    if (mt == RT_WRITE) {
        auto pc =
            decodeDescBitField(
                "PerCoarsePixelPSOutputs", 18, "disabled", "enabled");
        if (pc) {
            descs << " with Per-Coarse Pixel PS outputs enable";
            sym << ".cpo";
        }
    }
    auto ps =
        decodeDescBitField("PerSamplePS", 13, "disabled", "enabled");
    if (ps) {
        descs << " with Per-Sample Pixel PS outputs enable";
        sym << ".psp";
    }

    if (mt == RT_WRITE) {
        auto lrts =
            decodeDescBitField("LastRenderTargetSelect", 12, "false", "true");
        if (lrts) {
            descs << "; last render target";
            sym << ".lrts";
        }
    }

    auto sgs =
        decodeDescBitField("SlotGroupSelect", 11, "SLOTGRP_LO","SLOTGRP_HI");
    if (sgs) {
        descs << " slot group high";
        sym << ".sgh";
    }

    uint32_t surfaceIndex = 0;
    (void)decodeDescField("BTS", 0, 8,
        [&](std::stringstream &ss, uint32_t value) {
            surfaceIndex = value;
            ss << "surface " << value;
            descs << " to surface " << value;
            sym << ".bti[" << value << "]";
        });

    MessageInfo &mi = result.info;
    mi.symbol = sym.str();
    mi.description = descs.str();
    mi.op = mt == RT_READ ? SendOp::RENDER_READ : SendOp::RENDER_WRITE;
    mi.execWidth = execSize;
    mi.elemSizeBitsMemory = mi.elemSizeBitsRegFile = bitSize;
    mi.addrSizeBits = 0;
    mi.surfaceId = surfaceIndex;
    mi.attributeSet = MessageInfo::Attr::NONE;

    decodeMDC_H2(); // all render target messages permit a dual-header
}

void MessageDecoderOther::tryDecodeRTA() {
    const int TRACERAY_MSD = 0x00;
    // const int COMP_MESH_MSD = 0x01; (strawman still)

    std::stringstream sym, descs;
    int opBits = getDescBits(14, 4); // [17:14]
    if (opBits == TRACERAY_MSD) {
        int simd = decodeMDC_SM2(8);
        bool rtIsSimd16 = false;
        if (rtIsSimd16)
            error(8, 1, "message must be SIMD16 on this platform");

        // int mlen = getDescBits(25, 4); // [28:25]
        sym << "trace_ray" << simd;
        descs << "simd" << simd << " trace ray";

        MessageInfo &mi = result.info;
        mi.symbol = sym.str();
        mi.description = descs.str();
        mi.op = SendOp::TRACE_RAY;
        // payload is 2 or 3 registers (SIMD8 vs SIMD16)
        // https://gfxspecs.intel.com/Predator/Home/Index/47937
        //   - "addr" the first with a uniform pointer to global data
        //   - "data" the second being ray payloads[7:0] (each 32b)
        //   - "data" if SIMD16, rays[15:6]
        // we treat this as a SIMD1 operation (logically)
        // containing SIMD data 32b elements
        mi.execWidth = simd;
        mi.elemSizeBitsRegFile = mi.elemSizeBitsMemory = 32;
        mi.elemsPerAddr = 1;
        //
        mi.addrSizeBits = 64;
        if (platform() >= Platform::XE_HPC) {
            // XeHPC has a bit set at Msg[128] as well
            // e.g. https://gfxspecs.intel.com/Predator/Home/Index/47937
            mi.addrSizeBits = 129;
        }
        mi.addrType = AddrType::FLAT;
        mi.surfaceId = 0;
        mi.attributeSet = MessageInfo::Attr::NONE;
        mi.docs = chooseDoc(nullptr, "47929", "57495");
        decodeMDC_HF();
    } else {
        error(14, 4, "unsupported RTA op");
    }
}

void MessageDecoderOther::tryDecodeBTD() {
    const uint32_t BTD_SPAWN_MSD = 0x01;

    std::stringstream sym, descs;
    uint32_t opBits = getDescBits(14, 4); // [17:14]
    if (opBits == BTD_SPAWN_MSD) {
        int simd = decodeMDC_SM2(8);

        sym << "spawn" << simd;
        descs << "bindless simd" << simd << " spawn thread";

        MessageInfo &mi = result.info;
        mi.op = SendOp::SPAWN;
        mi.description = descs.str();
        mi.symbol = sym.str();
        mi.execWidth = simd;
        mi.docs = chooseDoc(nullptr, "47923", "57487");
    } else {
        error(14, 4, "unsupported BTD op");
    }
    addField("MessageType", 14, 4, opBits, descs.str());
}

///////////////////////////////////////////////////////////////////////////////
enum class SamplerSIMD {
    INVALID = 0,
    SIMD8,
    SIMD16,
    SIMD32,
    SIMD32_64,
    SIMD8H,
    SIMD16H,
    SIMD32H,
    SIMD8_INTRET, // with integer return XeHPG+
    SIMD16_INTRET, // with integer return XeHPG+
};


// TGL ...
// 000 Reserved
// 001 SIMD8
// 010 SIMD16
// 011 SIMD32/64
// 100 Reserved
// 101 SIMD8H
// 110 SIMD16H
// 111 Reserved
//
// XeHPG+
// 000 SIMD8 + Integer Return
// 001 SIMD8
// 010 SIMD16
// 011 RESERVED
// 100 SIMD16 + Integer Return
// 101 SIMD8H
// 110 SIMD16H
// 111 Reserved

static SamplerSIMD decodeSimdSize(
    Platform p,
    uint32_t simdBits,
    int &simdSize,
    std::stringstream &syms,
    std::stringstream &descs)
{
    SamplerSIMD simdMode = SamplerSIMD::INVALID;
    switch (simdBits) {
    case 0:
        if (p >= Platform::XE_HPC) {
            simdMode = SamplerSIMD::SIMD16_INTRET;
            syms << "simd16_iret";
            descs << "simd16 with integer return";
            simdSize = 16;
            break;
        } else if (p >= Platform::XE_HPG) {
            simdMode = SamplerSIMD::SIMD8_INTRET;
            syms << "simd8_iret";
            descs << "simd8 with integer return";
            simdSize = 8;
            break;
        }
        simdMode = SamplerSIMD::INVALID;
        syms << "???";
        descs << "unknown simd mode";
        simdSize = 1;
        return simdMode;
    case 1:
        if (p >= Platform::XE_HPC) {
            simdMode = SamplerSIMD::SIMD16;
            simdSize = 16;
            descs << "simd16";
            syms << "simd16";
            break;
        }
        simdMode = SamplerSIMD::SIMD8;
        simdSize = 8;
        descs << "simd8";
        syms << "simd8";
        break;
    case 2:
        if (p >= Platform::XE_HPC) {
            simdMode = SamplerSIMD::SIMD32;
            simdSize = 32;
            descs << "simd32";
            syms << "simd32";
            break;
        }
        simdMode = SamplerSIMD::SIMD16;
        simdSize = 16;
        descs << "simd16";
        syms << "simd16";
        break;
    case 3:
        if (p >= Platform::XE_HPC) {
            simdMode = SamplerSIMD::INVALID;
            syms << "???";
            descs << "unknown simd mode";
            simdSize = 1;
            break;
        }
        simdMode = SamplerSIMD::SIMD32_64;
        descs << "simd32/64";
        syms << "simd32";
        simdSize = 32;
        break;
    case 4:
        if (p >= Platform::XE_HPC) {
            simdMode = SamplerSIMD::SIMD16_INTRET;
            syms << "simd32_iret";
            descs << "simd32 with integer return";
            simdSize = 32;
            break;
        } else if (p >= Platform::XE_HPG) {
            syms << "simd16_iret";
            descs << "simd16 with integer return";
            simdMode = SamplerSIMD::SIMD16_INTRET;
            simdSize = 16;
            break;
        }
        simdMode = SamplerSIMD::INVALID;
        syms << "???";
        descs << "unknown simd mode";
        simdSize = 1;
        break;
    case 5:
        if (p >= Platform::XE_HPC) {
            simdMode = SamplerSIMD::SIMD16H;
            simdSize = 16;
            syms << "simd16h";
            descs << "simd16 high";
            break;
        }
        simdMode = SamplerSIMD::SIMD8H;
        simdSize = 8;
        syms << "simd8h";
        descs << "simd8 high";
        break;
    case 6:
        if (p >= Platform::XE_HPC) {
            simdMode = SamplerSIMD::SIMD32H;
            simdSize = 32;
            syms << "simd32h";
            descs << "simd32 high";
            break;
        }
        simdMode = SamplerSIMD::SIMD16H;
        syms << "simd16h";
        descs << "simd16 high";
        simdSize = 16;
        break;
    default:
        break;
    } // switch

    return simdMode;
}




enum class SamplerParam {
    NONE = 0,
    AI, // array index
    BIAS,
    BIAS_AI,
    DUDX, // u derivative with respect to x?
    DUDY, // u derivative with respect to y?
    DUMMY, // dummy parameter for messages with no arguments
           // sampler send requires src0 to be something
    DVDX, // v derivative with respect to x?
    DVDY, // v derivative with respect to y?
    LOD, // level of detail
    LOD_AI, // legel of detail ... array index
    MCS0, // multi compartment sampler buffer 0
    MCS1, // multi compartment sampler buffer 1
    MCS2, // multi compartment sampler buffer 3
    MCS3, // multi compartment sampler buffer 3
    MLOD, // ... level of detail
    MLOD_R,
    R, // r-coordinate
    REF,
    SI,
    U, // u-coordinate
    V, // v-coordinate
};

static std::string ToSymbol(SamplerParam sp) {
    switch (sp) {
    case SamplerParam::AI:       return "ai";
    case SamplerParam::BIAS:     return "bias";
    case SamplerParam::BIAS_AI:  return "bias_ai";
    case SamplerParam::DUDX:     return "dudx";
    case SamplerParam::DUDY:     return "dudy";
    case SamplerParam::DUMMY:    return "dummy";
    case SamplerParam::DVDX:     return "dvdx";
    case SamplerParam::DVDY:     return "dvdy";
    case SamplerParam::LOD:      return "lod";
    case SamplerParam::LOD_AI:   return "lod_ai";
    case SamplerParam::MCS0:     return "mcs0";
    case SamplerParam::MCS1:     return "mcs1";
    case SamplerParam::MCS2:     return "mcs2";
    case SamplerParam::MCS3:     return "mcs3";
    case SamplerParam::MLOD:     return "mlod";
    case SamplerParam::MLOD_R:   return "mlod_r";
    case SamplerParam::R:        return "r";
    case SamplerParam::REF:      return "ref";
    case SamplerParam::SI:       return "si";
    case SamplerParam::U:        return "u";
    case SamplerParam::V:        return "v";
    default:                     return "?";
    }
}

struct SamplerMessageDescription {
    const char *mnemonic;
    SamplerParam params[8];

    constexpr SamplerMessageDescription(
        const char *mne,
        SamplerParam p0 = SamplerParam::NONE,
        SamplerParam p1 = SamplerParam::NONE,
        SamplerParam p2 = SamplerParam::NONE,
        SamplerParam p3 = SamplerParam::NONE,
        SamplerParam p4 = SamplerParam::NONE,
        SamplerParam p5 = SamplerParam::NONE,
        SamplerParam p6 = SamplerParam::NONE,
        SamplerParam p7 = SamplerParam::NONE)
        : mnemonic(mne)
        , params{p0, p1, p2, p3, p4, p5, p6, p7}
    {
    }

    int countParams() const {
        int n = 0;
        for (SamplerParam sp : params) {
            if (sp == SamplerParam::NONE)
                break;
        }
        return n;
    }

    std::string describe(int src0Len) const {
        std::stringstream ss;
        ss << mnemonic;
        bool first = true;
        int n = 0;
        for (SamplerParam sp : params) {
            if (sp == SamplerParam::NONE)
                break;
            if (first) {
                first = false;
                ss << ":";
            } else {
                ss << "+";
            }
            ss << ToSymbol(sp);
            // for optional parameters
            if (src0Len >= 0 && ++n >= src0Len)
                break;
        }
        return ss.str();
    }
};


static void decodeSendMessage(
    uint32_t opBits, SamplerSIMD simdMode,
    SendOp &sendOp, std::string &symbol, std::string &desc, int &params)
{
    if (simdMode != SamplerSIMD::SIMD32_64) {
        switch (opBits) {
        case 0x00:
            symbol = "sample";
            desc = "sample";
            params = 4;
            break;
        case 0x01:
            symbol = "sample_b";
            desc = "sample+LOD bias";
            params = 5;
            break;
        case 0x02:
            symbol = "sample_l";
            desc = "sample override LOD";
            params = 5;
            break;
        case 0x03:
            symbol = "sample_c";
            desc = "sample compare";
            params = 5;
            break;
        case 0x04:
            symbol = "sample_d";
            desc = "sample gradient";
            params = 10;
            break;
        case 0x05:
            symbol = "sample_b_c";
            desc = "sample compare+LOD bias";
            params = 6;
            break;
        case 0x06:
            symbol = "sample_l_c";
            desc = "sample compare+override LOD";
            params = 6;
            break;
        case 0x07:
            symbol = "sample_ld";
            desc = "sample load";
            params = 4;
            break;
        case 0x08:
            symbol = "sample_gather4";
            desc = "sample gather4";
            params = 4;
            break;
        case 0x09:
            symbol = "sample_lod";
            desc = "sample override lod";
            params = 4;
            break;
        case 0x0A:
            symbol = "sample_resinfo";
            desc = "sample res info";
            params = 1;
            break;
        case 0x0B:
            symbol = "sample_info";
            desc = "sample info";
            params = 0;
            break;
        case 0x0C:
            symbol = "sample_killpix";
            desc = "sample+killpix";
            params = 3;
            break;
        case 0x10:
            symbol = "sample_gather4_c";
            desc = "sample gather4+compare";
            params = 5;
            break;
        case 0x11:
            symbol = "sample_gather4_po";
            desc = "sample gather4+pixel offset";
            params = 5;
            break;
        case 0x12:
            symbol = "sample_gather4_po_c";
            desc = "sample gather4 pixel offset+compare";
            params = 6;
            break;
            // case 0x13: //skipped
        case 0x14:
            symbol = "sample_d_c";
            desc = "sample derivatives+compare";
            params = 11;
            break;
        case 0x16:
            symbol = "sample_min";
            desc = "sample min";
            params = 2;
            break;
        case 0x17:
            symbol = "sample_max";
            desc = "sample max";
            params = 2;
            break;
        case 0x18:
            symbol = "sample_lz";
            desc = "sample with lod forced to 0";
            params = 4;
            break;
        case 0x19:
            symbol = "sample_c_lz";
            desc = "sample compare+with lod forced to 0";
            params = 5;
            break;
        case 0x1A:
            symbol = "sample_ld_lz";
            desc = "sample load with lod forced to 0";
            params = 3;
            break;
            // case 0x1B:
        case 0x1C:
            symbol = "sample_ld2dms_w";
            desc = "sample ld2 multi-sample wide";
            params = 7;
            break;
        case 0x1D:
            symbol = "sample_ld_mcs";
            desc = "sample load mcs auxilary data";
            params = 4;
            break;
        case 0x1E:
            symbol = "sample_ld2dms";
            desc = "sample load multi-sample";
            params = 6;
            break;
        case 0x1F:
            symbol = "sample_ld2ds";
            desc = "sample multi-sample without mcs";
            params = 6;
            break;
        default:
            symbol = std::to_string(opBits) + "?";
            desc = "?";
            break;
        }
    } else {
        switch (opBits) {
        case 0x00:
            symbol = "sample_unorm";
            desc = "sample unorm";
            params = 4; // relatively sure
            break;
        case 0x02:
            symbol = "sample_unorm_killpix";
            desc = "sample unorm+killpix";
            params = 4; // no idea???
            break;
        case 0x08:
            symbol = "sample_deinterlace";
            desc = "sample deinterlace";
            params = 4; // no idea???
            break;
        case 0x0C:
            // yes: this appears to be replicated
            symbol = "sample_unorm_media";
            desc = "sample unorm for media";
            params = 4; // no idea???
            break;
        case 0x0A:
            // yes: this appears to be replicated
            symbol = "sample_unorm_killpix_media";
            desc = "sample unorm+killpix for media";
            params = 4; // no idea???
            break;
        case 0x0B:
            symbol = "sample_8x8";
            desc = "sample 8x8";
            params = 4; // no idea???
            break;
        case 0x1F:
            symbol = "sample_flush";
            desc = "sampler cache flush";
            sendOp = SendOp::SAMPLER_FLUSH;
            params = 0; // certain
            break;
        default:
            symbol = std::to_string(opBits) + "?";
            desc = "?";
            sendOp = SendOp::INVALID;
            break;
        }
    }
}

void MessageDecoderOther::tryDecodeSMPL()
{
    setDoc("12484", "43860", "57022");
    std::stringstream syms, descs;

    auto simd2 = decodeDescBitField("SIMD[2]", 29, "", "");
    auto simd01 = getDescBits(17, 2);
    uint32_t simdBits = simd01|(simd2<<2);
    int simdSize = 0;
    const SamplerSIMD simdMode = decodeSimdSize(
        platform(), simdBits, simdSize, syms, descs);
    if (simdMode == SamplerSIMD::INVALID) {
        error(17, 2, "invalid SIMD mode");
    }
    addField("SIMD[1:0]", 17, 2, simd01, descs.str());

    bool is16bData = decodeDescBitField("ReturnFormat", 30, "32b", "16b") != 0;
    if (is16bData) {
        syms << "_16";
        descs << " 16b";
    }
    syms << "_";
    descs << " ";

    SendOp sendOp = SendOp::SAMPLER_LOAD;
    int params = 0;
    std::string symbol, desc;
    auto opBits = getDescBits(12, 5);
    decodeSendMessage(opBits, simdMode,
        sendOp, symbol, desc, params);
    syms << symbol;

    (void)addField("SamplerMessageType", 12, 5, opBits, desc);
    descs << desc;

    auto six = decodeDescField("SamplerIndex", 8, 4,
        [&](std::stringstream &ss, uint32_t six) {ss << "sampler " << six;});
    descs << " using sampler index " << six;
    syms << "[" << six << "," << decodeBTI(32) << "]";

    setScatterGatherOp(
        syms.str(),
        descs.str(),
        sendOp,
        AddrType::INVALID,
        getDescBits(0,8),
        32,
        is16bData ? 16 : 32,
        params,
        simdSize,
        MessageInfo::Attr::NONE);
    decodeMDC_H(); // header is optional in the sampler
}

void MessageDecoderOther::tryDecodeTS()
{
    if (getDescBits(0, 3) == 0) {
        setSpecialOpX(
            "eot", "end of thread", SendOp::EOT,
            AddrType::FLAT, 0,
            1, // mlen
            0, // rlen
            MessageInfo::Attr::NONE);
    } else {
        error(0, 32, "unsupported TS op");
    }
}

void MessageDecoderOther::tryDecodeURB()
{
    std::stringstream sym, descs;
    SendOp op;
    int simd = DEFAULT_EXEC_SIZE/2;
    int addrSize = 32;
    int dataSize = 32; // MH_URB_HANDLE has this an array of MHC_URB_HANDLE
    auto opBits = getDescBits(0, 4); // [3:0]
    auto chMaskPresent = getDescBit(15) != 0; // [15]
    auto decodeGUO =
        [&]() {
            return decodeDescField("GlobalUrbOffset", 4, 11,
                [&](std::stringstream &ss, uint32_t val) {
                    ss << val << " (in owords)";
                }); // [14:4]
        };
    auto decodePSO =
        [&]() {
            return decodeDescBitField(
                "PerSlotOffsetPresent", 17, "per-slot offset in payload") != 0;
        };
    int rlen = getDescBits(20, 5);
    // int mlen = getDescBits(25,4);
    int xlen = getDescBits(32+6, 5);
    int elemsPerAddr = 1;
    int off = 0;
    switch (opBits) {
    case 7:
        op = SendOp::STORE;
        off = 8*decodeGUO();
        addField("ChannelMaskEnable", 15, 1, getDescBit(15),
            chMaskPresent ? "enabled" : "disabled");
        sym << "MSDUW_DWS";
        descs << "urb dword ";
        if (chMaskPresent)
            descs << "masked ";
        descs << "write";
        if (decodePSO())
            descs << " with per-slot offset enabled";
        // "SIMD8 URB Dword Read message. Reads 1..8 Dwords, based on RLEN."
        elemsPerAddr = xlen != 0 ? xlen : 1;
        setDoc(chMaskPresent ? "44779" : "44778");
        decodeMDC_HR();
        break;
    case 8:
        op = SendOp::LOAD;
        off = 8*decodeGUO();
        sym << "MSDUR_DWS";
        descs << "urb dword read";
        if (decodePSO())
            descs << " with per-slot offset enabled";
        elemsPerAddr = rlen;
        setDoc("44777");
        decodeMDC_HR();
        break;
    case 9:
        op = SendOp::FENCE;
        sym << "MSD_URBFENCE";
        descs << "urb fence";
        simd = 1;
        dataSize = 0;
        setDoc("53422");
        break;
    default:
        error(0, 4, "unsupported URB op");
        return;
    }
    addField("URBOpcode", 0, 4, opBits, sym.str() + " (" + descs.str() + ")");

    setScatterGatherOp(
        sym.str(),
        descs.str(),
        op,
        AddrType::FLAT,
        0,
        addrSize,
        dataSize,
        elemsPerAddr,
        simd,
        MessageInfo::Attr::NONE);
    if (opBits == 7 || opBits == 8)
        result.info.immediateOffset = off;
}


///////////////////////////////////////////////////////////////////////////////
void iga::decodeDescriptorsOther(
    Platform platform, SFID sfid, ExecSize _execSize,
    SendDesc exDesc, SendDesc desc,
    DecodeResult &result)
{
    MessageDecoderOther mdo(
        platform, sfid, _execSize,
        exDesc, desc, result);
    mdo.tryDecodeOther();
}
