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
#ifndef IGA_FRONTEND_SENDS_SCATTERED
#define IGA_FRONTEND_SENDS_SCATTERED

#include "LoadCommon.hpp"

namespace iga
{
// template <ScatteredMessage::DataType D, ScatteredMessage::ElemsPerChan E, AddrModel A>
// void DecodeScattered(const DecodeMessageInputs &inps, Message &msg)

static const uint32_t BTI_A32_A64_C  = 0xFF;
static const uint32_t BTI_A32_A64_NC = 0xFD;
static const uint32_t BTI_SSO        = 0xFC;
static const uint32_t BTI_SLM        = 0xFE;

static bool decodeAddrModel(
    const DecodeMessageInputs &inps, Message &msg, AddrModel a64Ora32)
{
    uint32_t bti = getBits(inps.desc,0,8);
    if (bti == BTI_A32_A64_C || bti == BTI_A32_A64_NC) {
        msg.addrModel = (AddrModel)(bti | static_cast<int>(a64Ora32));
    } else if (bti == BTI_SLM) {
        msg.addrModel = AddrModel::SLM;
    } else if (bti == BTI_SSO) {
        msg.addrModel = (AddrModel)(bti | static_cast<int>(a64Ora32));
        msg.addrOffset = getBits(inps.exDesc, 16, 16);
    } else {
        msg.addrModel = (AddrModel)(bti | static_cast<int>(AddrModel::BTI_FAMILY));
    }
    return true;
}


template <AddrModel A, ScatteredMessage::DataType D, int SIMD_IX>
bool DecodeMessageScattered(
    const DecodeMessageInputs &inps,
    Message &msg,
    std::string &error)
{
    switch (getBits(inps.desc,10,2)) {
    case 0: msg.scattered.elemsPerChannel = ScatteredMessage::DE1; break;
    case 1: msg.scattered.elemsPerChannel = ScatteredMessage::DE2; break;
    case 2: msg.scattered.elemsPerChannel = ScatteredMessage::DE4; break;
    case 3: msg.scattered.elemsPerChannel = ScatteredMessage::DE8; break;
    default:
        error = "invalid DE (message width)";
        return false;
    }

    msg.scattered.dataType = D;
    msg.scattered.isConstant = inps.hasAttribute(MessageAttr::IS_CONST); // inps.isSFID(SFID::DCRO);
    msg.scattered.simdMode = testBit(inps.desc, SIMD_IX) == 0 ?
        VectorMessage::SM::SIMD8 : VectorMessage::SM::SIMD16;

    if (inps.hasAttribute(MessageAttr::HAS_PACKING)) {
        msg.scattered.packedData = testBit(inps.desc, 30);
        msg.scattered.packedAddresses = testBit(inps.desc, 29);
    } else {
        msg.scattered.packedData = false;
        msg.scattered.packedAddresses = false;
    }

    if (!decodeAddrModel(inps, msg, A)) {
        error = "invalid address model";
        return false;
    }

    return true;
}


} // namespace
#endif
