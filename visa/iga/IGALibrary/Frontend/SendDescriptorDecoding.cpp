/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SendDescriptorDecoding.hpp"
#include "../bits.hpp"
#include "../IR/Messages.hpp"
#include "../strings.hpp"

#include <algorithm>


using namespace iga;


static const char *getSFIDString(Platform p, uint32_t sfid)
{
    static const char *HSWBDW_SFIDS[] {
        "null",     // 0000b the null function
        nullptr,    // 0001b
        "sampler",  // 0010b new sampler
        "gateway",  // 0011b gateway

        "sampler",  // 0100b (old sampler encoding)
        "hdc.rc",   // 0101b render cache
        "hdc.urb",  // 0110b unified return buffer
        "spawner",  // 0111b thread spawner

        "vme",      // 1000b video motion estimation
        "hdc.ccdp", // 1001b constant cache
        "hdc.dc0",  // 1010b
        "pi",       // 1011b pixel interpolator

        "hdc.dc1",  // 1100b data-cache 1
        "cre",      // 1101b check and refinement
        nullptr,
        nullptr,
    };
    static const char *SKL_SFIDS[] {
        "null",     // 0000b the null function
        nullptr,    // 0001b
        "sampler",  // 0010b new sampler
        "gateway",  // 0011b gateway

        "hdc.dc2",  // 0100b data cache 2 (old sampler)
        "hdc.rc",   // 0101b render cache
        "hdc.urb",  // 0110b unified return buffer
        "spawner",  // 0111b thread spawner

        "vme",      // 1000b video motion estimation
        "hdc.dcro", // 1001b data cache read only (renaming)
        "hdc.dc0",  // 1010b
        "pi",       // 1011b pixel interpolator

        "hdc.dc1",  // 1100b data-cache 1
        "cre",      // 1101b check and refinement
        nullptr,
        nullptr,
    };
    const char **sfidTable = p < Platform::GEN9 ? HSWBDW_SFIDS : SKL_SFIDS;

    if (sfidTable[sfid] == nullptr) {
        return "?";
    }

    return sfidTable[sfid];
}


static uint32_t getHeaderBit(uint32_t desc) {return getBits(desc, 19, 1);}


void iga::EmitSendDescriptorInfo(
    Platform p,
    SFID sfid,
    ExecSize execSize,
    bool dstNonNull,
    int dstLen, int src0Len, int src1Len,
    const SendDesc &exDesc, const SendDesc &desc,
    std::stringstream &ss)
{
    DiagnosticList ws, es;

    //////////////////////////////////////
    // emit the: "wr:1h+2, rd:4" part
    ss << "wr:";
    if (src0Len >= 0) {
        ss << src0Len;
    } else if (desc.isReg()) {
        ss << "a0." << (int)desc.reg.subRegNum << "[28:25]";
    } else {
        ss << "?";
    }
    bool hasHeaderBit =
        sfid != SFID::UGM && sfid != SFID::UGML &&
        sfid != SFID::SLM && sfid != SFID::TGM;
    bool hasEncodedDstLen = true;
    if (hasHeaderBit && desc.isImm() && getHeaderBit(desc.imm)) {
        ss << "h";
    }
    //
    ss << "+";
    if (src1Len >= 0) {
        ss << src1Len;
    } else if (exDesc.isReg()) {
        ss << "a0." << (int)exDesc.reg.subRegNum << "[10:6]";
    } else {
        ss << "?";
    }
    ss << ", rd:";
    if (desc.isReg()) {
        ss << "a0." << (int)desc.reg.subRegNum << "[24:20]";
    } else if (hasEncodedDstLen) {
        ss << dstLen;
    } else {
        if (dstNonNull) {
            if (dstLen < 0) {
                PayloadLengths lens(p, sfid, execSize, desc.imm);
                dstLen = lens.dstLen;
            }
            if (dstLen >= 0) {
                ss << dstLen;
            } else {
                // we cannot decode this message and thus cannot infer the
                // destination length; so we just say so
                ss << "non-zero";
            }
        } else {
            ss << "0";
        }
    }
    //
    ////////////////////////////////
    // now the message description
    if (p < Platform::XE) {
        // pre-XE, emit the SFID first since it's not part of the op yet
        if (exDesc.isReg()) {
            ss << "; sfid a0." << (int)exDesc.reg.subRegNum << "[3:0]";
        } else { // no a0.0
            ss << "; " << getSFIDString(p, exDesc.imm & 0xF);
        }
    }

    if (desc.isImm()) {
        const DecodeResult dr =
            tryDecode(p, sfid, execSize,
                exDesc, desc, nullptr);
        if (dr.syntax.isValid()) {
            ss << "; " << dr.syntax.sym();
        } else if (!dr.info.description.empty()) {
            ss << "; " << dr.info.description;
        } else {
            if (!es.empty() &&
                es.back().second.find("unsupported sfid") == std::string::npos)
            {
                ss << "; " << es.back().second << "?";
            }
            // skip unsupported SFIDs
        }

        bool appendUvrLod =
            dr.info.hasAttr(MessageInfo::Attr::HAS_UVRLOD) && src0Len > 0;
        if (appendUvrLod) {
            // Deduce the number of typed coordinates included
            // (e.g. U, V, R, LOD)
            const int BITS_PER_REG = p >= Platform::XE_HPC ? 512 : 256;
            // We do this by assuming address coordinates are in GRF padded
            // SOA order:
            //   - U's registers, then V's, then R's, the LOD's
            //   - if a cooridate takes less than a full GRF, then we pad it up
            //     to that
            //   - not all decoded combinations are supported in hardware, but
            //     that's between the user and their deity; we're an assembler
            //     and can't track the endlessly changing spec
            // Typical usage is the default message and a GRF's worth of A32
            // elements as the SIMD size (e.g. 8 for TGL)
            //
            // bits for a full vector's worth of U's, V's, etc....
            const int regsPerCoord =
                std::max<int>(1,
                    dr.info.execWidth*dr.info.addrSizeBits/BITS_PER_REG);
            switch (src0Len/regsPerCoord) {
            case 1: ss << "; u"; break;
            case 2: ss << "; u,v"; break;
            case 3: ss << "; u,v,r"; break;
            case 4: ss << "; u,v,r,lod"; break;
            }
        } // U,V,R,LOD
    }
} // iga::EmitSendDescriptorInfo
