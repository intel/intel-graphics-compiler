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
#include "../Backend/MessageInfo.hpp"

using namespace iga;

static uint32_t getMsgLength(uint32_t desc) {return getBits(desc, 25, 4);}
static uint32_t getHeaderBit(uint32_t desc) {return getBits(desc, 19, 1);}
static uint32_t getRespLength(uint32_t desc) {return getBits(desc, 20, 5);}
static uint32_t getSrc1LengthFromExDesc(uint32_t exDesc) {
    return getBits(exDesc, 6, 5);
}
static uint32_t getMiscMsgDescBits(uint32_t desc) {return getBits(desc, 8, 6);}

static void formatMessageInfoDescription(
    Platform p,
    SFID sfid,
    const OpSpec &os,
    const SendDescArg &exDesc,
    uint32_t desc,
    std::stringstream &ss)
{
    DiagnosticList ws, es;
    auto exDescImm =
        exDesc.type == SendDescArg::REG32A ? 0xFFFFFFFF : exDesc.imm;
    auto mi =
        MessageInfo::tryDecode(p, sfid, exDescImm, desc, ws, es, nullptr);
    if (!mi.description.empty()) {
            ss << " " << mi.description;
    } else {
        if (!es.empty())
            ss << es.back().second;
        ss << "?";
    }
}


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
        "hdc.dcro0", // 1001b data cache read only (renaming)
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


void iga::EmitSendDescriptorInfo(
    Platform p,
    const OpSpec &os,
    const SendDescArg &exDesc,
    uint32_t desc,
    std::stringstream &ss)
{
    uint32_t exDescImm = exDesc.type == SendDescArg::IMM ? exDesc.imm : 0;
    SFID sfid =
        p < Platform::GEN12P1 && exDesc.type == SendDescArg::REG32A ?
            SFID::INVALID : MessageInfo::sfidFromOp(p, os.op, exDescImm);
    bool exDescHasA0 = exDesc.type == SendDescArg::REG32A;
    bool src1LenIsInA0 = exDesc.type == SendDescArg::REG32A;

    //////////////////////////////////////
    // emit the: "wr:3h+2, rd:4" part
    ss << "wr:" << getMsgLength(desc);
    bool hasHeaderBit = true;
    if (hasHeaderBit && getHeaderBit(desc)) {
        ss << "h";
    }
    int src1Len = 0;
    if (exDesc.type == SendDescArg::IMM) {
        src1Len = getSrc1LengthFromExDesc(exDescImm);
    }
    if (src1Len) {
        ss << "+" << src1Len;
    } else if (exDescHasA0) {
        if (src1LenIsInA0) {
            ss << "+a0.#[10:6]";
        } else {
            ss << "+?";
        }
    }
    ss << ", rd:" << getRespLength(desc);
    ss << ";";
    //
    ////////////////////////////////
    // now the message description
    if (p < Platform::GEN12P1) {
        // pre-GEN12, emit the SFID first since it's not part of the op yet
        if (exDescHasA0) {
            ss << " sfid is in a0.#[3:0]:";
        } else { // no a0.0
            ss << " " << getSFIDString(p, exDescImm & 0xF) << ":";
        }
    }
    if (sfid != SFID::INVALID) {
        formatMessageInfoDescription(p, sfid, os, exDesc, desc, ss);
    } else {
        ss << "cannot determine SFID";
    }
}