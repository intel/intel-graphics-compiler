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
#include "../IR/Loc.hpp"

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
    // to the same element (hence why "generic").  In other words, two
    // different units (maybe different hardware generate the same message).
    enum class SendOp {
        INVALID,
        ///////////////////////////////////////////////////////////////////////
        // load ops
        IS_LOAD_OP = 0x1000, // group can be tested via this bit
        // vector load (can be transposed)
        //   examples: dword gather read
        LOAD,
        // similar to vector but with a component mask (cmask, XYZW, RGBA)
        //   examples: a64 untyped read in DC1, a32 typed read...
        LOAD_QUAD,
        // block load
        //   examples: OWord block load, HWord scratch block load
        LOAD_STRIDED,
        //
        // special load operations
        LOAD_STATUS,
        //
        // e.g. surface info (NMS_RSI)
        READ_STATE,
        //
        ///////////////////////////////////////////////////////////////////////
        // store ops
        IS_STORE_OP = 0x2000, // group can be tested via this bit
        // vector store (can be transposed)
        //   examples: dword scattered write
        STORE,
        // vector store (can be transposed)
        //   examples: untyped write
        STORE_QUAD,
        // vector store (can be transposed)
        //   examples: block write
        STORE_STRIDED,
        //
        ///////////////////////////////////////////////////////////////////////
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
        ///////////////////////////////////////////////////////////////////////
        IS_OTHER_OP = 0x8000, // group can be tested via this bit
        //
        FENCE,
        //
        // gateway and thread-spawner events
        BARRIER,
        MONITOR,
        UNMONITOR,
        WAIT,
        SIGNAL_EVENT,
        EOT,
        //
        // TODO: a domain expert should break this into better ops
        // TODO: all sampler loads should go into the load category
        SAMPLER_LOAD,
        SAMPLER_FLUSH,
        //
        // TODO: a domain expert should break this into better ops
        // TODO: all loads should be moved up to IS_LOAD and reads IS_STORE
        RENDER_WRITE,
        RENDER_READ,
    };
    static inline bool SendOpIsLoad(SendOp op) {
        return (static_cast<int>(op) &
            static_cast<int>(SendOp::IS_LOAD_OP)) != 0;
    }
    static inline bool SendOpIsStore(SendOp op) {
        return (static_cast<int>(op) &
            static_cast<int>(SendOp::IS_STORE_OP)) != 0;
    }
    static inline bool SendOpIsAtomic(SendOp op) {
        return (static_cast<int>(op) &
            static_cast<int>(SendOp::IS_ATOMIC_OP)) != 0;
    }
    static inline bool SendOpIsOther(SendOp op) {
        return (static_cast<int>(op) &
            static_cast<int>(SendOp::IS_OTHER_OP)) != 0;
    }
    static inline bool SendOpHasCmask(SendOp op) {
        return op == SendOp::LOAD_QUAD
            || op == SendOp::STORE_QUAD
            ;
    }

    std::string format(SendOp op);

    enum class CacheOpt {
        // the default caching state from MOCS or wherever
        DEFAULT,
        //
        // a read is the last use and the lines should be invalidated
        //   e.g. bit 13 in some HDC messages
        READINVALIDATE,
        //
        // indicates a load should be cached
        //
        // e.g. in the BTI system 0xFD is used for this (non-coherent)
        CACHED,
        //
        // indicates a message that bypasses the cache (and evicts the entry
        // if present)
        //
        // e.g. in the BTI system 0xFF is used for this (coherent)
        UNCACHED,
    };
    std::string format(CacheOpt op);

    enum class AddrType {
        // stateless
        FLAT,
        // stateless
        BTI,
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
            // For messages that uncompress data, expand into the high unit
            // instead of the low.
            EXPAND_HIGH     = 1 << 1,
            //
            // If the message supports a channel mask for vector size
            // (data elements)
            HAS_CHMASK      = 1 << 2,
            //
            // Indicates the message is has U, V, R, and LOD coordinates
            // as part of the address payloads.
            HAS_UVRLOD      = 1 << 3,
            //
            // Indicates a scratch message
            SCRATCH         = 1 << 4,
            //
            // Indicates SLM
            SLM             = 1 << 5,
            //
            // Indicates the data is transposed during load or store
            TRANSPOSED      = 1 << 6,
            //
            // Indicates the message is a typed operation
            TYPED           = 1 << 7,
        };
        //
        // Queries a boolean property of this message.
        bool hasAttr(int attr) const{return (attributeSet & attr) != 0;}

        // The specific operation
        SendOp       op;

        // A bitset of the above attributes
        int          attributeSet;

        // Size (in bits) of one address
        int          addrSizeBits;
        //
        // The size (in bits) of each element once stored in the register file.
        int          elemSizeBitsRegFile;
        //
        // The size (in bits) of each element in memory.
        //
        // This can differ from elemSizeRegisterFile if there's any compression
        // effects.  For example, a byte gathering read will load one, two,
        // four bytes into each 32b slot leaving the upper bits for the one-
        // and two-byte case.
        //
        int          elemSizeBitsMemory;
        //
        // The number of vector elems or active channels in the cmask.
        // This is the number of elements each address loads.
        //
        // For example a legacy untyped load with a channel mask of XYZ
        // would be 3.  Some scatter/gather messages have a SIMT vector
        // component similar in funcntion.
        int          elemsPerAddr;
        //
        // For LOAD_QUAD and STORE_QUAD, this holds a bit mask of up to four
        // bits with which channels are *enabled* (not disabled).
        //
        // The elemsPerAddr field will still be set
        // (with the set cardinality here).
        int          channelsEnabled;
        //
        // The number of channels in the size of the operation.
        // (The "SIMD" size.)
        int          execWidth;

        // Caching options for the L1 cache (if supported)
        CacheOpt     cachingL1;
        //
        // Caching options for the L3 cache (if supported)
        // In some HDC messages this is bit 13.  Some parts don't
        // implement it though.
        CacheOpt     cachingL3;
        //
        // The surface addressing model
        AddrType     addrType;
        //
        // The surface identifier (if applicable).  E.g. BTI.
        SendDesc     surfaceId;
        //
        // possible immediate offset if the encoding supports it
        SendDesc     immediateOffset;
        //
        // A symbol value for this message (syntax).
        // The syntax is not complete and IGA will not consume it directly
        // (since some information isn't in necessarily in the descriptors).
        std::string  symbol;
        //
        //
        // This may hold a useful string telling the user the exact message
        // matching this specification up decode (if it exists).  This is
        // indended for for debugging only; the value and behavior are subject
        // to change at any time. The nullptr is a possible value.
        std::string  description;

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
        //
        // A transpose data layout
        bool isTransposed() const {return hasAttr(TRANSPOSED);}

        // Enables the operation to be used within a predicate assignment.
        operator bool() const {return hasAttr(VALID);}
        //
        // Do we enable abstract encoding?
        //
        // A problem is that as messages are refactored the function output
        // may be ambiguous (e.g. two messages with the same meaning).
        // E.g. An OW block read of 4 OWs == a HW block read with 2 HWs
        //
        //   E.g. SendFusion could flip SIMD for example
        //   MessageInfo mi = ...get from IR...;
        //   mi.simdSize = 16;
        //   if (tryEncode(mi)) {
        //       a SIMD16 version of the message exists!
        //   }
        //
        // std::vector<MessageEncoding> tryEncode(
        //     const MessageInfo &mi,
        //     SFID sfid,
        //     std::string *err = nullptr);
    }; //class MessageInfo

    SFID sfidFromEncoding(Platform p, uint32_t sfidBits);

    // The payload lengths for a given message.
    // Negative values indicate that the value is unknown or could be variable.
    struct PayloadLengths {
        int dstLen = -1;
        int src0Len = -1, src1Len = -1;
        bool uvrlod = false;

        PayloadLengths() { }

        // This attempts to deduce payload lengths from the given inputs.
        PayloadLengths(
            Platform p,
            SFID sfid,
            ExecSize execSize,
            uint32_t desc);

        // Same as above, but given a Op::SEND*
        //
        // For older platforms this will extract the SFID from exDesc
        PayloadLengths(
            Platform p,
            ExecSize execSize,
            uint32_t desc,
            uint32_t exDesc);
    };

    // Attempts to deduce the destination length for a given message.
    // For messages where a dst == null is permitted and implies 0,
    // be careful to check that before calling this.
    //
    // Returns -1 if unable to decode (not all messages are supported).
    //


    // Organizes various parts of syntax during decoding.  This enables
    // formatters to interleave real registers.
    //
    // LOAD: load, load_strided, load_cmask, fence
    //    MNEMONIC SFID CONTROLS
    //         $datareg DATATYPE
    //         SURFACE [ SCALE $addrreg IMMOFFSET ] ADDRTYPE
    // ATOMIC: atomic_iinc
    //    MNEMONIC SFID CONTROLS
    //         $datareg DATATYPE
    //         SURFACE [ SCALE $addrreg IMMOFFSET ] ADDRTYPE
    //         $atomic-arg
    // STORE: store, store_strided
    //    MNEMONIC SFID CONTROLS
    //         SURFACE [ SCALE $addrreg IMMOFFSET ] ADDRTYPE
    //         $datareg DATATYPE
    // CONTROL: e.g. barrier
    //    MNEMONIC SFID CONTROLS
    //         SURFACE [ SCALE $addrreg IMMOFFSET ] ADDRTYPE
    //         $datareg DATATYPE
    //
    struct MessageSyntax {
        enum class Layout {
            // unset
            INVALID = 0,
            // e.g. load, load_strided, load_blocks2d
            LOAD,
            // e.g. store, store_quad, store_strided
            STORE,
            // e.g. atomic_iinc, atomic_fadd
            ATOMIC,
            // e.g. barrier or eot (fence is LOAD since it returns a reg)
            CONTROL
        };
        Layout layout = Layout::INVALID;

        bool isValid() const {return layout != Layout::INVALID;}
        //
        bool isLoad() const {return layout == Layout::LOAD;}
        bool isStore() const {return layout == Layout::STORE;}
        bool isAtomic() const {return layout == Layout::ATOMIC;}
        bool isControl() const {return layout == Layout::CONTROL;}

        std::string   mnemonic; // e.g. "load" or "load_block"
        // SFID          sfid;     // e.g. "ugm"
        std::string   controls; // e.g. ".ugm.ca.ca"

        // address stuff
        std::string   surface;     // e.g. "bti<0x2>", "flat", or "bti<a0.4>"
        std::string   scale;       // e.g. 4*...
        std::string   immOffset;   // e.g. -0x40
        ///////////// part of control
        // std::string   addressType; // e.g. ":a32"
        //
        // data stuff
        // std::string   dataType; // e.g. ":d32x4", or ":d32.xyw", or ":d32x2t"

        // emits rough syntax
        std::string str(
            std::string execInfo,
            std::string dataReg,
            std::string addrReg,
            std::string atmoicArgReg) const;

        // orders the operands in a symbolic form
        // (suffixes surface, scaling and offset)
        std::string sym() const;
    }; // MessageSyntax

    std::string format(AddrType op);
    std::string format(SFID sfid);

    //
    // Returns the resulting information of a message decode
    struct DecodeResult {
        MessageInfo        info;
        MessageSyntax      syntax;
        DiagnosticList     warnings;
        DiagnosticList     errors;
        DecodedDescFields  fields;

        operator bool() const {return errors.empty();}
    };

    // Attempts to decode the descriptor for
    DecodeResult tryDecode(
        Platform p, SFID sfid,
        SendDesc exDesc, SendDesc desc, RegRef indDesc,
        DecodedDescFields *fields);

    ///////////////////////////////////////////////////////////////////////////
    // parsing support
    SendOp     lookupSendOp(std::string mnemonic);
    //
    struct SendOpInfo {
        bool hasDst;
        int src1Args; // e.g. atomics
        bool isSingleRegAddr; // e.g. a block load
    };
    SendOpInfo lookupSendOpInfo(SendOp op);
    // returns true if the SFID on a given platform is eligible for symbolic
    // translate for load/store syntax; this function enables us to
    // incrementally enable load/store syntax
    bool       sendOpSupportsSyntax(Platform p, SendOp op, SFID sfid);
    //
    //
    struct VectorMessageArgs {
        SFID           sfid = SFID::INVALID;
        SendOp         op = SendOp::INVALID;
        int            execSize = 0;
        CacheOpt       cachingL1 = CacheOpt::DEFAULT;
        CacheOpt       cachingL3 = CacheOpt::DEFAULT;
        //
        AddrType       addrType = AddrType::FLAT;
        SendDesc       addrSurface = 0; // a0 or imm
        int            addrScale = 1; // must be 1x, V*D, 2*V*2, or 4*V*D
        int            addrSize = 0;
        SendDesc       addrOffset = 0; // a0 or imm
        //
        int            dataSizeReg = 0;
        int            dataSizeMem = 0;
        bool           dataSizeExpandHigh = 0; // d8u32h
        // bool        dataSizeSignExtend; // d8s16 (someday)
        union {
            struct {
                short  dataVectorSize;
                bool   dataTranspose;
            };
            //
            // used for LOAD_QUAD, STORE_QUAD, ...
            // e.g. this might map to to dc1 untyped read/write ops
            uint32_t   dataComponentMask = 0; // x,y,z,w (x-enabled is bit 0, ...)
        };

        ///////////////////////////////////////////////////////////////////////
        // creates an initial empty message
        constexpr VectorMessageArgs() { }

        // returns the logical count of elements per address
        // for simple vector messages, it's dataVectorSize, but for
        // messages with a component mask, it's the number of enabled channels
        int elementsPerAddress() const {
            if (SendOpHasCmask(op)) {
                int n = 0;
                for (int i = 0; i < 4; i++)
                    n += (((1<<i) & dataComponentMask) ? 1 : 0);
                return n;
            } else {
                return dataVectorSize;
            }
        }

        bool isLoad() const {
            return static_cast<int>(op) & static_cast<int>(SendOp::IS_LOAD_OP);
        }
        bool isStore() const {
            return static_cast<int>(op) &
                static_cast<int>(SendOp::IS_STORE_OP);
        }
        bool isAtomic() const {
            return static_cast<int>(op) &
                static_cast<int>(SendOp::IS_ATOMIC_OP);
        }
        bool hasCMask() const {
            return SendOpHasCmask(op);
        }
    }; // VectorMessageArgs

    bool encodeDescriptors(
        Platform p,
        const VectorMessageArgs &vma,
        SendDesc &exDesc,
        SendDesc &desc,
        std::string &err);

} // iga::

#endif
