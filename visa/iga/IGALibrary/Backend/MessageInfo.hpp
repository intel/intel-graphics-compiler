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
#ifndef IGA_MESSAGE_INFO_HPP
#define IGA_MESSAGE_INFO_HPP

#include "../api/iga_bxml_enums.hpp"
#include "../Backend/Native/Field.hpp"
#include "../IR/Types.hpp"

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace iga
{
    struct DescField {
        DescField(int _off, int _len) : off(_off), len(_len) { }
        int off, len;
    };

    using DiagnosticList = std::vector<std::pair<DescField,std::string>>;
    using DecodedDescFields =
        std::vector<std::tuple<Fragment,uint32_t,std::string>>;

    // This is generic list of send operations.  One or more messages may map
    // to the same element (hence why "generic").
    enum class SendOp {
        INVALID,
        ////////////////////////////////////
        // load ops
        IS_LOAD_OP = 0x1000, // group can be tested via this bit
        LOAD,
        LOAD_QUAD,
        LOAD_STRIDED,
        // special load operations
        LOAD_STATUS,
        LOAD_SURFACE_INFO,
        //
        ////////////////////////////////////
        // store ops
        IS_STORE_OP = 0x2000, // group can be tested via this bit
        STORE,
        STORE_QUAD,
        STORE_STRIDED,
        //
        ///////////////////////////////////
        // atomic ops
        ///////////////////////////////////
        // atomic bitwise
        IS_ATOMIC_OP = 0x4000, // group can be tested via this bit
        ATOMIC_LOAD, ATOMIC_STORE,
        ATOMIC_AND, ATOMIC_XOR, ATOMIC_OR,
        // atomic integer ops
        ATOMIC_IINC, ATOMIC_IDEC, ATOMIC_IPDEC,
        ATOMIC_IADD, ATOMIC_ISUB, ATOMIC_IRSUB,
        ATOMIC_ICAS,
        ATOMIC_SMIN, ATOMIC_SMAX, // signed
        ATOMIC_UMIN, ATOMIC_UMAX, // unsigned
        // floating point
        ATOMIC_FADD, ATOMIC_FSUB,
        ATOMIC_FMIN, ATOMIC_FMAX,
        ATOMIC_FCAS,
        //
        //
        READ_STATE, // e.g. surface info (NMS_RSI)
        //
        FENCE,
        //
        // gateway and thread-spawner events
        BARRIER,
        MONITOR,
        UNMONITOR,
        WAIT,
        SIGNAL,
        EOT,
        //
        // TODO: a domain expert should break this into better ops
        SAMPLER_LOAD,
        SAMPLER_FLUSH,
        //
        // TODO: a domain expert should break this into better ops
        RENDER_WRITE,
        RENDER_READ,
    };
    std::string format(SendOp op);

    enum class CacheOpt {
        // the default caching state from MOCS or wherever
        DEFAULT,
        // a read is the last use and the lines should be invalidated
        READINVALIDATE,
    };
    std::string format(CacheOpt op);

    enum class AddrType {
        // stateless
        FLAT,
        // stateless
        BTI,
    };
    std::string format(AddrType op);

    // The result of a message encoding
    struct MessageEncoding {
        SFID        sfid;
        int         src1Len;
        uint32_t    exDesc;
        uint32_t    desc;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generic processing of basic buffer messages.   This includes data port
    // vector reads and writes as well block and atomic messages.  There may be
    // certain obscure or legacy operations that are unsupported.
    //
    // usage:
    //   const G4_SendMsgDescriptor *msgDesc = ...
    //   if (auto sgi = msgDesc->getMessageInfo()) {
    //     process message e.g. sgi.hasAttr(MessageInfo::LOADS)
    //   } else {
    //     unsupported/unrecognized message type (e.g. sampler, gateway, ...)
    //   }
    //
    struct MessageInfo {
        enum Attr {
            VALID = 0x80000000,
            //
            // Set on atomic operations that return data
            ATOMIC_RETURNS  = 1 << 0,
            //
            // Set on HDC data port messages that are coherent.
            // (BTI 0xFF instead of 0xFD)
            COHERENT     = 1 << 1,
            //
            // For messages that uncompress data, expand into the high unit
            // instead of the low.
            EXPAND_HIGH  = 1 << 2,
            //
            // If the message supports a channel mask for vector size
            // (data elements)
            HAS_CHMASK   = 1 << 3,
            //
            // Indicates a scratch message
            SCRATCH      = 1 << 4,
            //
            // Indicates SLM
            SLM          = 1 << 5,
            //
            // Indicates the data is transposed during load or store
            TRANSPOSED   = 1 << 6,
            //
            // Indicates the message is a typed operation
            TYPED        = 1 << 7,
        };
        //
        // Queries a boolean property of this message.
        bool hasAttr(int attr) const{return (attributeSet & attr) != 0;}

        // The specific operation
        SendOp    op;

        // A bitset of the above attributes
        int       attributeSet;

        // Size (in bits) of one address
        int       addrSizeBits;
        //
        // The size (in bits) of each element once stored in the register file.
        int       elemSizeBitsRegFile;
        //
        // The size (in bits) of each element in memory.
        //
        // This can differ from elemSizeRegisterFile if there's any compression
        // effects.  For example, a byte gathering read will load one, two,
        // four bytes into each 32b slot leaving the upper bits for the one-
        // and two-byte case.
        //
        int       elemSizeBitsMemory;
        //
        // The number of vector elems or active channels in the cmask.
        // This is the number of elements each address loads.
        //
        // For example a legacy untyped load with a channel mask of XYZ
        // would be 3.  Some scatter/gather messages have a SIMT vector component
        // similar in funcntion.
        int       elemsPerAddr;
        //
        // For LOAD_QUAD and STORE_QUAD, this holds a bit mask of up to four
        // bits with which channels are *enabled* (not disabled).
        //
        // The elemsPerAddr field will still be set
        // (with the set cardinality here).
        int       channelsEnabled;
        //
        // The number of channels in the size of the operation.
        // (The "SIMD" size.)
        int       execWidth;

        // Caching options for the L1 cache (if supported)
        CacheOpt  cachingL1;
        //
        // Caching options for the L3 cache (if supported)
        // In some HDC messages this is bit 13.  Some parts don't
        // implement it though.
        CacheOpt  cachingL3;
        //
        // The surface addressing model
        AddrType  addrType;
        //
        // The surface identifier (if applicable).  E.g. BTI.
        uint32_t  surface;
        //
        // possible immediate offset if the encoding supports it
        int32_t   immediateOffset;
        //
        // A symbol value for this message (syntax).
        // The syntax is not complete and IGA will not consume it directly
        // (since some information isn't in necessarily in the descriptors).
        std::string symbol;
        //
        //
        // This may hold a useful string telling the user the exact message
        // matching this specification up decode (if it exists).  This is
        // indended for for debugging only; the value and behavior are subject
        // to change at any time. The nullptr is a possible value.
        std::string description;

        const char *docs = nullptr;
        //
        // A block message
        bool isBlock() const {
            return execWidth == 1 && (isLoad() || isStore());
        }
        //
        // A proper load message (not an atomic returning a value)
        bool isLoad() const {
            return (static_cast<int>(op) &
                static_cast<int>(SendOp::IS_LOAD_OP)) != 0;
        }
        //
        // A proper store message (not an atomic that is storing something)
        bool isStore() const {
            return (static_cast<int>(op) &
                static_cast<int>(SendOp::IS_STORE_OP)) != 0;
        }
        //
        // An atomic message (it may or may not return a value)
        bool isAtomic() const {
            return (static_cast<int>(op) &
                static_cast<int>(SendOp::IS_ATOMIC_OP)) != 0;
        }
        //
        // An atomic that returns a value
        bool isAtomicLoad() const {return hasAttr(ATOMIC_RETURNS);}

        // Enables the operation to be used within a predicate assignment.
        operator bool() const {return hasAttr(VALID);}

        // Decodes this information from an SFID and descriptors.
        // Note, the decoding can fail (not all messages are supported),
        // and it's incumbent on the caller to check the VALID bit.
        // (or function call overload).
        //  MessageInfo mi = MessageInfo::tryDecode(...);
        //  if (mi) {
        //      ... valid
        //
        static MessageInfo tryDecode(
            Platform p,
            SFID sfid,
            uint32_t exDesc,
            uint32_t desc,
            DiagnosticList &warnings,
            DiagnosticList &errors,
            DecodedDescFields *descDecodedField);

        static SFID sfidFromEncoding(Platform p, uint32_t sfidBits);
        static SFID sfidFromOp(Platform p, Op op, uint32_t exDesc);

        //
        // Do we enable abstract encoding?
        //
        // A problem is that as messages are refactored the function output may be
        // ambiguous (e.g. two messages with the same meaning).
        // E.g. An OW block read of 4 OWs == a HW block read with 2 HWs
        //
        //   E.g. SendFusion could flip SIMD for example
        //   MessageInfo mi = ...get from IR...;
        //   mi.simdSize = 16;
        //   if (tryEncode(mi)) {
        //       a SIMD16 version of the message exists!
        //   }
        //
        // std::vector<MessageEncodeing> tryEncode(
        //     const MessageInfo &mi,
        //     SFID sfid,
        //     std::string *err = nullptr);
    }; //class MessageInfo
} // iga

#endif
