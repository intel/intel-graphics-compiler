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
#ifndef IGA_FRONTEND_SENDS_BLOCK
#define IGA_FRONTEND_SENDS_BLOCK


#include "LoadCommon.hpp"

namespace iga
{

template <
    AddrModel A,
    BlockMessage::SubType T,
    BlockMessage::Size S = BlockMessage::B128>
bool DecodeMessageBlock(
    const DecodeMessageInputs &inps,
    Message &msg,
    std::string &error)
{
    msg.block.subType = T;
    msg.block.blockSize = S;
    if (S == BlockMessage::B128) {
        switch (getBits(inps.desc,8,3)) {
        case 0: msg.block.elems = BlockMessage::Elems::X1L; break;
        case 1: msg.block.elems = BlockMessage::Elems::X1H; break;
        case 2: msg.block.elems = BlockMessage::Elems::X2; break;
        case 3: msg.block.elems = BlockMessage::Elems::X4; break;
        case 4: msg.block.elems = BlockMessage::Elems::X8; break;
        default:
            error = "invalid number of elements";
            return false;
        }
    } else {
        switch (getBits(inps.desc,8,3)) {
        case 1: msg.block.elems = BlockMessage::Elems::X1; break;
        case 2: msg.block.elems = BlockMessage::Elems::X2; break;
        case 3: msg.block.elems = BlockMessage::Elems::X4; break;
        case 4: msg.block.elems = BlockMessage::Elems::X8; break;
        default:
            error = "invalid number of elements";
            return false;
        }
    }

    if (!decodeAddrModel(inps, msg, A)) {
        error = "invalid address model";
        return false;
    }

    return true;
}

static size_t computeDataSize(const BlockMessage &bm)
{
    size_t n = 0;
    switch (bm.elems) {
    case BlockMessage::X1:  // HWord
    case BlockMessage::X1L: // OWord
    case BlockMessage::X1H: n = 1; break; // OWord
    case BlockMessage::X2:  n = 2; break;
    case BlockMessage::X4:  n = 4; break;
    case BlockMessage::X8:  n = 8; break;
    }
    if (bm.blockSize == BlockMessage::B128) {
        n /= 2;
    }
    return n;
}

size_t BlockMessage::numAddrRegisters() const {
    // header is always required, gets computed by Message::numAddrRegisters
    return 0;
}

size_t BlockMessage::numDataRegisters() const
{
    // FIXME: once writes are supported
    return computeDataSize(*this);
}


} // namespace

#endif
