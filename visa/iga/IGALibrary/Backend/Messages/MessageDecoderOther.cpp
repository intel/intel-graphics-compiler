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

struct MessageDecoderOther : MessageDecoderLegacy
{
    MessageDecoderOther(
        Platform _platform, SFID _sfid,
        SendDesc _exDesc, SendDesc _desc, RegRef _indDesc,
        DecodeResult &_result)
        : MessageDecoderLegacy(
            _platform, _sfid, _exDesc, _desc, _indDesc, _result)
    {
    }

    void tryDecodeOther() {
        switch (sfid) {
        case SFID::GTWY: tryDecodeGTWY();   break;
        case SFID::RC:   tryDecodeRC();   break;
        case SFID::SMPL: tryDecodeSMPL(); break;
        case SFID::TS:   tryDecodeTS();   break;
        case SFID::URB:  tryDecodeURB();   break;
        default: error(0, 0, "invalid sfid");
        }
    }

    void tryDecodeGTWY();
    void tryDecodeRC();
    void tryDecodeSMPL();
    void tryDecodeTS();
    void tryDecodeURB();
}; // MessageDecodeOther


void MessageDecoderOther::tryDecodeGTWY() {
    auto choosePage = [&](const char *gen12, const char *genX = nullptr) {
        return gen12 ? gen12 : genX;
    };
    std::stringstream sym, descs;
    int opBits = getDescBits(0, 3); // [2:0]
    int expectMlen = 0;
    const char *doc = nullptr;
    SendOp sendOp = SendOp::INVALID;
    switch (opBits) {
    case 1:
        sym << "signal_event";
        descs << "signal event";
        expectMlen = 1; //
        sendOp = SendOp::SIGNAL_EVENT;
        doc = choosePage("54036", "57494");
        break;
    case 2:
        sym << "monitor";
        descs << "monitor event";
        expectMlen = 1; // C.f. MDP_EVENT
        sendOp = SendOp::MONITOR;
        doc = choosePage("47925", "57490");
        break;
    case 3:
        sym << "unmonitor";
        descs << "unmonitor event";
        expectMlen = 1; // C.f. MDP_NO_EVENT
        sendOp = SendOp::UNMONITOR;
        doc = choosePage("47926", "57491");
        break;
    case 4:
        sym << "barrier";
        descs << "barrier";
        expectMlen = 1; // C.f. MDP_Barrier
        sendOp = SendOp::BARRIER;
        doc = choosePage("47924", "57489");
        break;
    case 6:
        sym << "wait";
        descs << "wait for event";
        sendOp = SendOp::WAIT;
        expectMlen = 1; // C.f. MDP_Timeout
        doc = choosePage("47928", "57493");
        break;
    default:
        error(0, 2, "unsupported GTWY op");
    }
    addField("GatewayOpcode", 0, 3, opBits, descs.str());
    setSpecialOpX(
        sym.str(), descs.str(), sendOp,
        AddrType::FLAT, SendDesc(0),
        1, // mlen
        0, // rlen
        0);
    decodeMDC_HF(); // all gateway messages forbid a header
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
    mi.attributeSet = 0;

    decodeMDC_H2(); // all render target messages permit a dual-header
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
        simdMode = SamplerSIMD::INVALID;
        syms << "???";
        descs << "unknown simd mode";
        simdSize = 1;
        return simdMode;
    case 1:
        simdMode = SamplerSIMD::SIMD8;
        simdSize = 8;
        descs << "simd8";
        syms << "simd8";
        break;
    case 2:
        simdMode = SamplerSIMD::SIMD16;
        simdSize = 16;
        descs << "simd16";
        syms << "simd16";
        break;
    case 3:
        simdMode = SamplerSIMD::SIMD32_64;
        descs << "simd32/64";
        syms << "simd32";
        simdSize = 32;
        break;
    case 4:
        simdMode = SamplerSIMD::INVALID;
        syms << "???";
        descs << "unknown simd mode";
        simdSize = 1;
        break;
    case 5:
        simdMode = SamplerSIMD::SIMD8H;
        simdSize = 8;
        syms << "simd8h";
        descs << "simd8 high";
        break;
    case 6:
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

void MessageDecoderOther::tryDecodeSMPL()
{
    setDoc(platform() >= Platform::GEN12P1 ? "43860" : "12484");
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
    const char *messageDesc = "";
    auto opBits = getDescBits(12,5);
    if (simdMode != SamplerSIMD::SIMD32_64) {
        switch (opBits) {
        case 0x00:
            syms << "sample";
            messageDesc = "sample";
            params = 4;
            break;
        case 0x01:
            syms << "sample_b";
            messageDesc = "sample+LOD bias";
            params = 5;
            break;
        case 0x02:
            syms << "sample_l";
            messageDesc = "sample override LOD";
            params = 5;
            break;
        case 0x03:
            syms << "sample_c";
            messageDesc = "sample compare";
            params = 5;
            break;
        case 0x04:
            syms << "sample_d";
            messageDesc = "sample gradient";
            params = 10;
            break;
        case 0x05:
            syms << "sample_b_c";
            messageDesc = "sample compare+LOD bias";
            params = 6;
            break;
        case 0x06:
            syms << "sample_l_c";
            messageDesc = "sample compare+override LOD";
            params = 6;
            break;
        case 0x07:
            syms << "sample_ld";
            messageDesc = "sample load";
            params = 4;
            break;
        case 0x08:
            syms << "sample_gather4";
            messageDesc = "sample gather4";
            params = 4;
            break;
        case 0x09:
            syms << "sample_lod";
            messageDesc = "sample override lod";
            params = 4;
            break;
        case 0x0A:
            syms << "sample_resinfo";
            messageDesc = "sample res info";
            params = 1;
            break;
        case 0x0B:
            syms << "sample_info";
            messageDesc = "sample info";
            params = 0;
            break;
        case 0x0C:
            syms << "sample_killpix";
            messageDesc = "sample+killpix";
            params = 3;
            break;
        case 0x10:
            syms << "sample_gather4_c";
            messageDesc = "sample gather4+compare";
            params = 5;
            break;
        case 0x11:
            syms << "sample_gather4_po";
            messageDesc = "sample gather4+pixel offset";
            params = 5;
            break;
        case 0x12:
            syms << "sample_gather4_po_c";
            messageDesc = "sample gather4 pixel offset+compare";
            params = 6;
            break;
            // case 0x13: //skipped
        case 0x14:
            syms << "sample_d_c";
            messageDesc = "sample derivatives+compare";
            params = 11;
            break;
        case 0x16:
            syms << "sample_min";
            messageDesc = "sample min";
            params = 2;
            break;
        case 0x17:
            syms << "sample_max";
            messageDesc = "sample max";
            params = 2;
            break;
        case 0x18:
            syms << "sample_lz";
            messageDesc = "sample with lod forced to 0";
            params = 4;
            break;
        case 0x19:
            syms << "sample_c_lz";
            messageDesc = "sample compare+with lod forced to 0";
            params = 5;
            break;
        case 0x1A:
            syms << "sample_ld_lz";
            messageDesc = "sample load with lod forced to 0";
            params = 3;
            break;
            // case 0x1B:
        case 0x1C:
            syms << "sample_ld2dms_w";
            messageDesc = "sample ld2 multi-sample wide";
            params = 7;
            break;
        case 0x1D:
            syms << "sample_ld_mcs";
            messageDesc = "sample load mcs auxilary data";
            params = 4;
            break;
        case 0x1E:
            syms << "sample_ld2dms";
            messageDesc = "sample load multi-sample";
            params = 6;
            break;
        case 0x1F:
            syms << "sample_ld2ds";
            messageDesc = "sample multi-sample without mcs";
            params = 6;
            break;
        default:
            syms << "sample_" << std::hex << std::uppercase << opBits << "?";
            messageDesc = "?";
            break;
        }
    } else {
        switch (opBits) {
        case 0x00:
            syms << "sample_unorm";
            messageDesc = "sample unorm";
            params = 4; // relatively sure
            break;
        case 0x02:
            syms << "sample_unorm_killpix";
            messageDesc = "sample unorm+killpix";
            params = 4; // no idea???
            break;
        case 0x08:
            syms << "sample_deinterlace";
            messageDesc = "sample deinterlace";
            params = 4; // no idea???
            break;
        case 0x0C:
            // yes: this appears to be replicated
            syms << "sample_unorm_media";
            messageDesc = "sample unorm for media";
            params = 4; // no idea???
            break;
        case 0x0A:
            // yes: this appears to be replicated
            syms << "sample_unorm_killpix_media";
            messageDesc = "sample unorm+killpix for media";
            params = 4; // no idea???
            break;
        case 0x0B:
            syms << "sample_8x8";
            messageDesc = "sample 8x8";
            params = 4; // no idea???
            break;
        case 0x1F:
            syms << "sample_flush";
            messageDesc = "sampler cache flush";
            sendOp = SendOp::SAMPLER_FLUSH;
            params = 0; // certain
            break;
        default:
            syms << "sample_" << std::hex << std::uppercase << opBits << "?";
            messageDesc = "?";
            sendOp = SendOp::INVALID;
            break;
        }
    }
    (void)addField("SamplerMessageType",12,5,opBits,messageDesc);
    descs << messageDesc;

    auto six = decodeDescField("SamplerIndex", 8, 4,
        [&](std::stringstream &ss, uint32_t six){ss << "sampler " << six;});
    descs << " using sampler index " << six;
    syms << "[" << six << "," << decodeBTI(32) << "]";

    AddrType addrType = AddrType::BTI;
    setScatterGatherOp(
        syms.str(),
        descs.str(),
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

void MessageDecoderOther::tryDecodeTS()
{
    if (getDescBits(0, 3) == 0) {
        setSpecialOpX(
            "eot", "end of thread", SendOp::EOT,
            AddrType::FLAT, 0,
            1, // mlen
            0, // rlen
            0);
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
        0);
    if (opBits == 7 || opBits == 8)
        result.info.immediateOffset = off;
}


///////////////////////////////////////////////////////////////////////////////
void iga::decodeDescriptorsOther(
    Platform platform, SFID sfid,
    SendDesc exDesc, SendDesc desc, RegRef indDesc,
    DecodeResult &result)
{
    MessageDecoderOther mdo(platform, sfid, exDesc, desc, indDesc, result);
    mdo.tryDecodeOther();
}