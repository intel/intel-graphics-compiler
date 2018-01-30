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
#ifndef IGA_FRONTEND_SENDS_COMMON
#define IGA_FRONTEND_SENDS_COMMON

#include "Types.hpp"
#include "../../bits.hpp"
#include "../../IR/Instruction.hpp"

namespace iga {

// should mostly be only Message-level attributes
enum MessageAttr {
    NONE = 0,
    IS_HWORD = 1,
    IS_CONST = IS_HWORD << 1,
    SUPPORTS_IAR = IS_CONST << 1,
    SUPPORTS_HEADER = SUPPORTS_IAR << 1,
//    SM2_AT_BIT12 = SUPPORTS_HEADER << 1,
//    SM2_AT_BIT8 = SM2_AT_BIT12 << 1,
    HAS_BTI_BITS = SUPPORTS_HEADER << 1,
    HAS_IAR_13 = HAS_BTI_BITS << 1, // invalidate after read in in bit 13
    HAS_RET_STATUS_13 = HAS_IAR_13 << 1, // returns status in bit 13
    HAS_PACKING = HAS_RET_STATUS_13 << 1, // bits 30, 29 data and address
};

struct DecodeMessagePattern;


struct DecodeMessageInputs {
    const Model                 &model;
    const Instruction           &inst;
    uint32_t                     exDesc;
    uint32_t                     desc;
    const DecodeMessagePattern  &message;

    bool isSFID(SFID sfid) const {
        return getBits(exDesc, 0, 4) == static_cast<uint32_t>(sfid);
    }
    bool hasAttribute(MessageAttr attr) const;
};

typedef bool (*DecodeMessageFunc)(
    const DecodeMessageInputs &inps,
    Message &msg,
    std::string &error);

struct Subpattern {
    int fieldStart, fieldLength; // only valid if field
    uint32_t fieldValueEq; // permissiable value
    bool valid() const {return fieldLength != 0;}
};

struct DecodeMessagePattern {
    const char          *name;
    Platform             platform;
    Message::Family      family;
    Message::Mode        mode;
    SFID                 sfid;
    Subpattern           subpatterns[4]; // all valid subpatterns must pass
                                         // general pattern is
    const char          *mnemonic;
    DecodeMessageFunc    decodeFunc;
    uint32_t             attrs; // enum LdFormatAttr

    bool matches(
        const Model &m,
        uint32_t exDesc,
        uint32_t desc) const;
    void decode(
        const Model &model,
        const Instruction &inst,
        uint32_t exDesc,
        uint32_t desc,
        Message &msg,
        std::string &error) const;
};


} // namespace

#endif
