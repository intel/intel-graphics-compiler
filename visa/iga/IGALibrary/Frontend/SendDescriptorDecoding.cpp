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
    RegRef indDesc,
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
    bool hasHeaderBit = true;
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
    if (p < Platform::GEN12P1) {
        // pre-GEN12, emit the SFID first since it's not part of the op yet
        if (exDesc.isReg()) {
            ss << "; sfid a0." << (int)exDesc.reg.subRegNum << "[3:0]";
        } else { // no a0.0
            ss << "; " << getSFIDString(p, exDesc.imm & 0xF);
        }
    }

    if (desc.isImm()) {
        const DecodeResult dr =
            tryDecode(p, sfid, exDesc, desc, indDesc, nullptr);
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

        if (dr.info.hasAttr(MessageInfo::HAS_UVRLOD) && src0Len > 0) {
            const int REG_FILE_BITS =
                    256;
            const int QUARTER_EXEC_BITS = REG_FILE_BITS/32/2/2;
            int elems = std::max<int>(QUARTER_EXEC_BITS, dr.info.execWidth);
            int regsForU =
                std::max<int>(1, elems*dr.info.addrSizeBits/REG_FILE_BITS);
            switch (src0Len/regsForU) {
            case 1: ss << "; u"; break;
            case 2: ss << "; u,v"; break;
            case 3: ss << "; u,v,r"; break;
            case 4: ss << "; u,v,r,lod"; break;
            }
        } // U,V,R,LOD
    }
} // iga::EmitSendDescriptorInfo
