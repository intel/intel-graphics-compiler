/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef G4_SEND_DESCS_HPP
#define G4_SEND_DESCS_HPP

#include "Common_ISA.h"

#include <string>
#include <optional>
#include <ostream>
#include <utility>

namespace vISA
{
enum class SendAccess
{
    INVALID = 0,
    READ_ONLY, // e.g. load, sampler operation
    WRITE_ONLY, // e.g. store, render target write
    READ_WRITE // e.g. an atomic with return
};
static const int MSGOP_BUFFER_LOAD_GROUP   = 0x100;
static const int MSGOP_BUFFER_STORE_GROUP  = 0x200;
static const int MSGOP_BUFFER_ATOMIC_GROUP = 0x400;
static const int MSGOP_SAMPLE_GROUP = 0x600;
static const int MSGOP_GATHER_GROUP = 0x800;
static const int MSGOP_OTHER_GROUP  = 0x800;
//
// various message operations
enum class MsgOp {
    INVALID = 0,
    // load
    LOAD = MSGOP_BUFFER_LOAD_GROUP + 1,
    LOAD_STRIDED, // same as load, but 1 address (obeys exec mask)
    LOAD_QUAD, // e.g. untyped load (loading XYZW)
    LOAD_BLOCK2D,
    LOAD_STATUS,
    LOAD_QUAD_STATUS,
    // store
    STORE_GROUP = MSGOP_BUFFER_STORE_GROUP + 1,
    STORE,
    STORE_STRIDED,
    STORE_QUAD,
    STORE_BLOCK2D,
    //
    // atomics
    //
    ATOMIC_GROUP = MSGOP_BUFFER_ATOMIC_GROUP + 1,
    ATOMIC_IINC,
    ATOMIC_IDEC,
    ATOMIC_LOAD,
    ATOMIC_STORE,
    ATOMIC_IADD,
    ATOMIC_ISUB,
    ATOMIC_ICAS,
    ATOMIC_SMIN,
    ATOMIC_SMAX,
    ATOMIC_UMIN,
    ATOMIC_UMAX,
    ATOMIC_CAS,
    //
    ATOMIC_FADD,
    ATOMIC_FSUB,
    ATOMIC_FMIN,
    ATOMIC_FMAX,
    ATOMIC_FCAS,
    //
    //
    ATOMIC_AND,
    ATOMIC_XOR,
    ATOMIC_OR,
    // others ...
    READ_STATE_INFO,
    //
    FENCE,
    //
    // gateway operations
    BARRIER,
    NBARRIER,
    EOT,
    SAMPLE_GROUP = MSGOP_SAMPLE_GROUP + 1,
    SAMPLE,
    SAMPLE_B,
    SAMPLE_L,
    SAMPLE_C,
    SAMPLE_D,
    SAMPLE_B_C,
    SAMPLE_L_C,
    SAMPLE_KILLPIX,
    SAMPLE_D_C,
    SAMPLE_LZ,
    SAMPLE_C_LZ,
    GATHER_GROUP = MSGOP_GATHER_GROUP + 1,
    GATHER4,
    GATHER4_L,
    GATHER4_B,
    GATHER4_I,
    GATHER4_I_C,
    GATHER4_L_C,
    GATHER4_C,
};
std::string ToSymbol(MsgOp);
uint32_t GetMsgOpEncoding(MsgOp);
uint32_t GetSamplerMsgOpEncoding(MsgOp);
MsgOp ConvertLSCOpToMsgOp(LSC_OP op);
MsgOp ConvertSamplerOpToMsgOp(VISASampler3DSubOpCode op);

enum class LdStOrder {
    INVALID = 0,
    //
    SCALAR, // aka "transposed", typically SIMD1, should be (W)
    VECTOR, // normal vector message or atomic (honors the execution mask)
};
enum class AddrType {
    INVALID = 0,
    //
    FLAT,
    SS, BSS,
    BTI
};

// Data size
enum class DataSize {
    INVALID = 0,
    D8, // 8b
    D16, // 16b
    D32,// 32b
    D64, // 64b
    D8U32, // 8bit zero extended to 32bit
    D16U32, // 16bit zero extended to 32bit
};

std::string ToSymbol(DataSize d);
DataSize ConvertLSCDataSize(LSC_DATA_SIZE ds);
uint32_t GetDataSizeEncoding(DataSize ds);
size_t GetDataSizeInBytes(DataSize ds);

// Data order
enum class DataOrder {
    INVALID = 0,
    NONTRANSPOSE,
    TRANSPOSE
};

std::string ToSymbol(DataOrder dord);
DataOrder ConvertLSCDataOrder(LSC_DATA_ORDER dord);
uint32_t GetDataOrderEncoding(DataOrder dord);

// Data elems
enum class VecElems {
    INVALID = 0,
    V1,
    V2,
    V3,
    V4,
    V8,
    V16,
    V32,
    V64
};

std::string ToSymbol(VecElems ve);
VecElems ConvertLSCDataElems(LSC_DATA_ELEMS de);
uint32_t GetVecElemsEncoding(VecElems ve);
size_t GetNumVecElems(VecElems ve);

// data chmask
enum DataChMask
{
    INVALID = 0,
    X = 1 << 0,
    Y = 1 << 1,
    Z = 1 << 2,
    W = 1 << 3
};

size_t GetNumVecElemsQuad(int chMask);

// address size type
enum class AddrSizeType
{
  INVALID = 0,
  FLAT_A64_A32,
  FLAT_A64_A64,
  STATEFUL_A32,
  FLAT_A32_A32,
  GLOBAL_A32_A32,
  LOCAL_A32_A32,
};

std::string ToSymbol(AddrSizeType a);
AddrSizeType ConvertLSCAddrSizeType(LSC_ADDR_SIZE a);
uint32_t GetAddrSizeTypeEncoding(AddrSizeType a);

// Cache controls
// only certain combinations are legal
enum class Caching {
    // the invalid value for caching
    INVALID = 0,
    //
    CA, // cached (load)
    DF, // default (load/store)
    RI, // read-invalidate (load)
    WB, // writeback (store)
    UC, // uncached (load)
    ST, // streaming (load/store)
    WT, // writethrough (store)
};
std::string ToSymbol(Caching);
// default, default returns ""
std::string ToSymbol(Caching,Caching);
Caching ConvertLSCCacheOpt(LSC_CACHE_OPT co);
std::pair<Caching, Caching> ConvertLSCCacheOpts(LSC_CACHE_OPT col1, LSC_CACHE_OPT col3);

struct ImmOff {
    bool is2d;
    union {
        int immOff;
        struct {short immOffX, immOffY;};
    };
    ImmOff(int imm) : is2d(false), immOff(imm) { }
    ImmOff(short immX, short immY) : is2d(true), immOffX(immX), immOffY(immY) { }
    ImmOff() : ImmOff(0) { }
};

enum class LdStAttrs {
    NONE = 0,
    //
    // for atomic messages that don't indicate if the return value is used
    ATOMIC_RETURN   = 0x0001,
    //
    // for cases where the message does not imply if it is a scratch access
    SCRATCH_SURFACE = 0x0002,
};
static inline LdStAttrs operator|(LdStAttrs a0, LdStAttrs a1) {
    return LdStAttrs(int(a0) | int(a1));
}

// Abstraction for the nubmer of elements each address loads.
// Generally this is just a simple value (e.g. V4 would be 4), but we
// also support the channel mask nonsense added by LOAD_QUAD, STORE_QUAD.
struct ElemsPerAddr {
    // A friendly four-element bitset that is inductively closed and
    // correct under the custom | operator below
    enum class Chs {
        INVALID = 0,
        //
        X = 1,
        Y = 2,
        Z = 4,
        W = 8,
        //
        XY   = X | Y,
        XZ   = X     | Z,
        XW   = X         | W,
        XYZ  = X | Y | Z,
        XYW  = X | Y     | W,
        XZW  = X     | Z | W,
        XYZW = X | Y | Z | W,
        //
        YZ   =     Y | Z,
        YW   =     Y     | W,
        YZW  =     Y | Z | W,
        //
        ZW  =          Z | W,
    };

    // works on both channel masks and vector lengths
    int getCount() const;

    bool isChannelMask() const {return isChMask;}

    // asserts if not isChannelMask()
    Chs getMask() const;

    std::string str() const;

    ElemsPerAddr(int _count) : isChMask(false), count(_count) { }
    ElemsPerAddr(Chs chs) : isChMask(true), channels(chs) { }
private:
    bool isChMask;
    union {
        int count;
        Chs channels;
    };
}; // ElemsPerAddr
static inline ElemsPerAddr::Chs operator|(
    ElemsPerAddr::Chs c0, ElemsPerAddr::Chs c1)
{
    return ElemsPerAddr::Chs(int(c0) | int(c1));
}


class G4_Operand;
class IR_Builder;

// Base class for all send descriptors.
// (Note that G4_SendDesc could be reused by more than one instruction.)
class G4_SendDesc
{
    friend class G4_InstSend;

protected:
    // The execution size for this message.
    G4_ExecSize execSize;

    // Limit access to G4_InstSend and any derived classes.
    void setExecSize(G4_ExecSize v) { execSize = v; }

public:
    enum class Kind {
      INVALID,
      RAW, // G4_SendDescRaw
    };

    Kind        kind;

    SFID        sfid;

    const IR_Builder& irb;

    G4_SendDesc(Kind k, SFID _sfid, const IR_Builder& builder)
        : kind(k),
          sfid(_sfid),
          execSize(g4::SIMD_UNDEFINED),
          irb(builder) { }
    G4_SendDesc(Kind k, SFID _sfid, G4_ExecSize _execSize, const IR_Builder& builder)
        : kind(k),
          sfid(_sfid),
          execSize(_execSize),
          irb(builder)
    {}

    SFID getSFID() const {return sfid;}

    // execSize: need to set it in the ctor
    G4_ExecSize getExecSize() const { return execSize; }

    bool isRaw() const {return kind == Kind::RAW;}
    //
    bool isHDC() const;
    bool isLSC() const;
    bool isSampler() const {return getSFID() == SFID::SAMPLER;}
    //
    virtual bool isEOT() const = 0;
    virtual bool isSLM() const = 0;
    virtual bool isTyped() const = 0;
    virtual bool isAtomic() const = 0;
    virtual bool isBarrier() const = 0;
    virtual bool isFence() const = 0;
    virtual bool isHeaderPresent() const = 0;

    //
    // This gives a general access type
    virtual SendAccess getAccessType() const = 0;
    bool isRead() const {
        return
            getAccessType() == SendAccess::READ_ONLY ||
            getAccessType() == SendAccess::READ_WRITE;
    }
    bool isWrite() const {
        return
            getAccessType() == SendAccess::WRITE_ONLY ||
            getAccessType() == SendAccess::READ_WRITE;
    }
    bool isReadWrite() const {
        return getAccessType() == SendAccess::READ_WRITE;
    }
    // Returns the nubmer of elements each address (or coordinate)
    // accesses
    // E.g. d32x2 returns 2 (representing a message that loads
    // a pair per address).
    virtual unsigned getElemsPerAddr() const = 0;
    //
    // Returns the size in bytes of each element.
    // E.g. a d32x2 returns 4 (d32 is 32b)
    virtual unsigned getElemSize() const = 0;

    //
    // Returns the caching behavior of this message if known.
    // Returns Caching::INVALID if the message doesn't support caching
    // controls.
    virtual std::pair<Caching,Caching> getCaching() const = 0;
    Caching getCachingL1() const {return getCaching().first;}
    Caching getCachingL3() const {return getCaching().second;}
    virtual void setCaching(Caching l1, Caching l3) = 0;
    //
    // generally in multiples of full GRFs, but a few exceptions such
    // as OWord and HWord operations may make this different
    virtual size_t getDstLenBytes() const = 0;
    virtual size_t getSrc0LenBytes() const = 0;
    virtual size_t getSrc1LenBytes() const = 0;
    //
    // These round up to the nearest register.
    // For legacy uses (e.g. MessageLength, exMessageLength(), ...)
    // (e.g. an OWord block read will report 1 register)
    // Favor the get{Dst,Src0,Src1}LenBytes() methods.
    size_t getDstLenRegs() const;
    size_t getSrc0LenRegs() const;
    size_t getSrc1LenRegs() const;
    //
    // true if the message is a scratch space access (e.g. scratch block read)
    virtual bool isScratch() const = 0;
    //
    bool isScratchRead() const{return isScratch() && isRead();}
    bool isScratchWrite() const {return isScratch() && isWrite();}
    //
    // message offset in terms of bytes
    //   e.g. scratch offset
    virtual std::optional<ImmOff> getOffset() const = 0;

    // Returns the associated surface (if any)
    // This can be a BTI (e.g. a0 register or G4_Imm if immediate)
    virtual G4_Operand *getSurface() const = 0;

    virtual std::string getDescription() const = 0;

    // Sets the EOT bit of the descriptor,
    // returns false if the descriptor forbids EOT
    virtual bool setEOT() = 0;
};


///////////////////////////////////////////////////////////////////////////////
// high-level load/store descriptors
// TODO: remove this, make descriptor that maintains encoding
// for newer-style descriptors and offers abstract access
// to all the things that are fields below
//
struct G4_SendDescLdSt : G4_SendDesc {
    // The message op
    MsgOp op;

    // E.g. flat, bti, ...
    AddrType addrType;
    //
    // e.g. 64 for A64, 32 for A32
    int addrBits;
    //
    // Untyped linear acccesses will have 1 (general case)
    // Typed messages can have 1-4.
    // Any two dimensions block opreations will have 2.
    int addrDims;

    //
    // The number of bits per element loaded from memory by this message.
    //   E.g. D32 would be 32
    //   E.g. D8U32 ==> 8
    int elemBitsMem;
    //
    // The number of bits each element occupies in the register file
    // This is generally the same as memory with minor exceptions for aligned
    // loads (e.g. byte scattered loads that align bytes or words to 32b)
    // e.g. D8U32 has 32 for this and 8 for `elemBitsMem`
    int elemBitsReg;
    //
    // The number of elements we load per address.
    //   e.g. D32V4 would be 4
    //   for load_quad/store_quad this will be 1, 2, 3, 4
    //     e.g. V3 would imply .XYZ
    //   exotic strided load_quad stuff like .ZW is illegal
    //   i.e. we only support .X, .XY, .XYZ, and .XYZW
    int elemPerAddr;
    //
    // The data order message kind
    LdStOrder order; // "transposed" ==> 1 address

    // The caching options for L1 and L3
    Caching l1, l3;

    // an immediate offset to add to each element (if supported)
    ImmOff immOff;

    // For messages that take a surface (e.g. a0.#)
    //  This must be either:
    //    - nullptr for FLAT messages such as stateless global memory or SLM
    //    - reference to an a0.# register
    //    - an immediate value (for an immediate BTI)
    //    - for BSS/SS this is a reference to an a0 register
    G4_Operand *surface = nullptr;

    // Other miscellaneous attributes that cannot be deduced from the
    // information above
    LdStAttrs attrs;

    // In some rare cases we must explicitly override message payload sizes.
    //    1. prefetches use a null dst and set rLen = 0
    //    2. e.g. block2d puts block sizes in a header
    short overrideDstLengthBytesValue = -1;
    short overrideSrc0LengthBytesValue = -1;
    short overrideSrc1LengthBytesValue = -1;

    G4_SendDescLdSt(
        SFID sfid,
        MsgOp _op,
        G4_ExecSize _execSize,
        //
        // addr params
        AddrType at, int _addrBits, int _addrDims,
        //
        // data params
        int elemBitsMem, int elemBitsReg, int elemsPerAddr,
        LdStOrder _order,
        //
        // ext info
        Caching _l1, Caching _l3,
        G4_Operand *surf,
        ImmOff _immOff,
        LdStAttrs _attrs,
        const IR_Builder& builder);

    void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

    ///////////////////////////////////////////////////////////////////////////
    // queries (getters)

    // Checks if a message has a valid encoding on the current platform
    // an optional error diagnostic can give you a failure hint.
    bool canEncode(std::string *error = nullptr) const;

    bool hasAttrs(LdStAttrs a) const {
        return (int(a) & int(attrs)) == int(a);
    }

    virtual size_t getSrc0LenBytes() const override;
    virtual size_t getSrc1LenBytes() const override;
    virtual size_t getDstLenBytes() const override;

    virtual SendAccess getAccessType() const override;
    virtual std::pair<Caching,Caching> getCaching() const override {return std::make_pair(l1, l3);}
    virtual void setCaching(Caching l1, Caching l3) override;
    //
    virtual std::optional<ImmOff> getOffset() const override {return immOff;}
    virtual G4_Operand *getSurface() const override {return surface;}
    //
    virtual bool isEOT() const override {return false;}
    virtual bool isSLM() const override;
    virtual bool isAtomic() const override;
    virtual bool isBarrier() const override {return false;}
    virtual bool isFence() const override {return false;}
    virtual bool isScratch() const override {return hasAttrs(LdStAttrs::SCRATCH_SURFACE);}
    virtual bool isTyped() const override;

    virtual std::string getDescription() const override {return str();}
    void str(std::ostream &os) const;
    std::string str() const;

    ///////////////////////////////////////////////////////////////////////////
    // mutators (setters)
    void overrideSrc0LengthBytes(size_t lenBytes) {
        MUST_BE_TRUE(lenBytes == (size_t)-1 || lenBytes <= (1 << 15) - 1,
            "value out of range");
        overrideSrc0LengthBytesValue = lenBytes;
    }
    void overrideSrc1LengthBytes(size_t lenBytes) {
        MUST_BE_TRUE(lenBytes == (size_t)-1 || lenBytes <= (1 << 15) - 1,
            "value out of range");
        overrideSrc1LengthBytesValue = lenBytes;
    }
    void overrideDstLengthBytes(size_t lenBytes) {
        MUST_BE_TRUE(lenBytes == (size_t)-1 || lenBytes <= (1 << 15) - 1,
            "value out of range");
        overrideDstLengthBytesValue = lenBytes;
    }
}; // G4_SendDescLdSt

////////////////////////////////////////////////////////////////////////////
class G4_SendDescRaw : public G4_SendDesc
{
private:
    /// Structure describes a send message descriptor. Only expose
    /// several data fields; others are unnamed.
    struct MsgDescLayout {
        uint32_t funcCtrl : 19;     // Function control (bit 0:18)
        uint32_t headerPresent : 1; // Header present (bit 19)
        uint32_t rspLength : 5;     // Response length (bit 20:24)
        uint32_t msgLength : 4;     // Message length (bit 25:28)
        uint32_t simdMode2 : 1;     // 16-bit input (bit 29)
        uint32_t returnFormat : 1;  // 16-bit return (bit 30)
        uint32_t EOT : 1;           // EOT
    };

    /// View a message descriptor in two different ways:
    /// - as a 32-bit unsigned integer
    /// - as a structure
    /// This simplifies the implementation of extracting subfields.
    union DescData {
        uint32_t value;
        MsgDescLayout layout;
    } desc;

    /// Structure describes an extended send message descriptor.
    /// Only expose several data fields; others are unnamed.
    struct ExtendedMsgDescLayout {
        uint32_t funcID : 4;       // bit 0:3
        uint32_t unnamed1 : 1;     // bit 4
        uint32_t eot : 1;          // bit 5
        uint32_t extMsgLength : 5; // bit 6:10
        uint32_t cps : 1;          // bit 11
        uint32_t RTIndex : 3;      // bit 12-14
        uint32_t src0Alpha : 1;    // bit 15
        uint32_t extFuncCtrl : 16; // bit 16:31
    };

    /// View an extended message descriptor in two different ways:
    /// - as a 32-bit unsigned integer
    /// - as a structure
    /// This simplifies the implementation of extracting subfields.
    union ExtDescData {
        uint32_t value;
        ExtendedMsgDescLayout layout;
    } extDesc;

    SendAccess accessType;

    /// Whether funcCtrl is valid
    bool funcCtrlValid;

    // sampler surface pointer?
    G4_Operand *m_sti;
    G4_Operand *m_bti; // BTI or other surface pointer

    /// indicates this message is an LSC message
    bool        isLscDescriptor = false;
    // sfid now stored separately from the ExDesc[4:0] since the new LSC format
    // no longer uses ExDesc for that information
    int         src1Len;
    bool        eotAfterMessage = false;

public:
    static const int SLMIndex = 0xFE;

    G4_SendDescRaw(
        uint32_t fCtrl, uint32_t regs2rcv, uint32_t regs2snd,
        SFID fID, uint16_t extMsgLength, uint32_t extFCtrl,
        SendAccess access, G4_Operand* bti, G4_Operand* sti, const IR_Builder& builder);

    /// Construct a object with descriptor and extended descriptor values.
    /// used in IR_Builder::createSendMsgDesc(uint32_t desc, uint32_t extDesc, SendAccess access)
    G4_SendDescRaw(
        uint32_t desc, uint32_t extDesc, SendAccess access,
        G4_Operand* bti, G4_Operand* sti, const IR_Builder& builder);

    /// Preferred constructor takes an explicit SFID and src1 length
    G4_SendDescRaw(
        SFID sfid,
        uint32_t desc,
        uint32_t extDesc,
        int src1Len,
        SendAccess access,
        G4_Operand *bti,
        bool isValidFuncCtrl,
        const IR_Builder& builder);

    // Preferred constructor takes an explicit SFID and src1 length
    // Need execSize, so it is created for a particular send.
    G4_SendDescRaw(
        SFID sfid,
        uint32_t desc,
        uint32_t extDesc,
        int src1Len,
        SendAccess access,
        G4_Operand* bti,
        G4_ExecSize execSize,
        bool isValidFuncCtrl,
        const IR_Builder& builder);

    void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

    static uint32_t createExtDesc(SFID funcID, bool isEot = false)
    {
        return createExtDesc(funcID, isEot, 0, 0);
    }

    static uint32_t createMRTExtDesc(bool src0Alpha, uint8_t RTIndex, bool isEOT, uint32_t extMsgLen, uint16_t extFuncCtrl)
    {
        ExtDescData data;
        data.value = 0;
        data.layout.funcID = SFIDtoInt(SFID::DP_WRITE);
        data.layout.RTIndex = RTIndex;
        data.layout.src0Alpha = src0Alpha;
        data.layout.eot = isEOT;
        data.layout.extMsgLength = extMsgLen;
        data.layout.extFuncCtrl = extFuncCtrl;
        return data.value;
    }

    static uint32_t createExtDesc(SFID funcID,
        bool isEot,
        unsigned extMsgLen,
        unsigned extFCtrl = 0)
    {
        ExtDescData data;
        data.value = 0;
        data.layout.funcID = SFIDtoInt(funcID);
        data.layout.eot = isEot;
        data.layout.extMsgLength = extMsgLen;
        data.layout.extFuncCtrl = extFCtrl;
        return data.value;
    }

    static uint32_t createDesc(uint32_t fc, bool headerPresent,
        unsigned msgLength, unsigned rspLength)
    {
        DescData data;
        data.value = fc;
        data.layout.headerPresent = headerPresent;
        data.layout.msgLength = static_cast<uint16_t>(msgLength);
        data.layout.rspLength = static_cast<uint16_t>(rspLength);
        return data.value;
    }

    SFID      getFuncId() const {return sfid;}

    uint32_t getFuncCtrl() const {
        return desc.layout.funcCtrl;
    }

    ////////////////////////////////////////////////////////////////////////
    // LSC-related operations
    bool isLscOp() const {return isLscDescriptor;}

    // TODO: update to use types defined in this file rather than
    // these front-end vISA interface types
    LSC_OP getLscOp() const {
        assert(isLscOp());
        return static_cast<LSC_OP>(desc.value & 0x3F);
    }
    LSC_ADDR_TYPE getLscAddrType() const;
    int getLscAddrSizeBytes() const; // e.g. a64 => 8
    LSC_DATA_ORDER getLscDataOrder() const;

    bool isEOTInst() const { return eotAfterMessage; }

    virtual bool setEOT() override;

    // query methods common for all raw sends
    uint16_t ResponseLength() const;
    uint16_t MessageLength() const { return desc.layout.msgLength; }
    uint16_t extMessageLength() const { return (uint16_t)src1Len; }
    bool isDataPortRead() const { return accessType != SendAccess::WRITE_ONLY; }
    bool isDataPortWrite() const { return accessType != SendAccess::READ_ONLY; }
    SendAccess getAccess() const { return accessType; }
    bool isValidFuncCtrl() const { return funcCtrlValid; }
    void setHeaderPresent(bool val);

    ///////////////////////////////////////////////////////////////////////
    // for HDC messages only (DC0/DC1/DC2/DP_DCRO(aka DP_CC))
    //

    //////////////////////////////////////
    // calling these functions on non-HDC may assert
    uint16_t getExtFuncCtrl() const {
        MUST_BE_TRUE(isHDC(), "getExtFuncCtrl on non-HDC message");
        return extDesc.layout.extFuncCtrl;
    }
    uint32_t getHdcMessageType() const;
    bool isAtomicMessage() const;
    uint16_t getHdcAtomicOp() const;

    bool isSLMMessage() const;
    unsigned getEnabledChannelNum() const;

    // Returns the nubmer of elements each address (or coordinate)
    // accesses
    // E.g. d32x2 returns 2 (representing a message that loads
    // a pair per address).
    virtual unsigned getElemsPerAddr() const override;
    //
    // Returns the size in bytes of each element.
    // E.g. a d32x2 returns 4 (d32 is 32b)
    //
    // This is the size in the register file not memory.
    virtual unsigned getElemSize() const override;

    bool isOwordLoad() const;
    // OW1H ==> implies 2 (but shouldn't be used)
    // asserts isOwordLoad()
    unsigned getOwordsAccessed() const;

    bool isHdcTypedSurfaceWrite() const;

    // return offset in unit of HWords
    uint16_t getScratchRWOffset() const
    {
        MUST_BE_TRUE(isScratchRW(), "Message is not scratch space R/W.");
        return (getFuncCtrl() & 0xFFFu);
    }

    bool isScratchRW() const
    {
        // legacy DC0 scratch msg: bit[18] = 1
        return getSFID() == SFID::DP_DC0 && ((getFuncCtrl() & 0x40000u) != 0);
    }
    bool isScratchRead() const
    {
        return isScratchRW() && (getFuncCtrl() & 0x20000u) == 0;
    }
    bool isScratchWrite() const
    {
        return isScratchRW() && (getFuncCtrl() & 0x20000u) != 0;
    }
    // in terms of HWords (1, 2, 4, or 8)
    uint16_t getScratchRWSize() const
    {
        MUST_BE_TRUE(isScratchRW(), "Message is not scratch space R/W.");
        uint16_t bitV = ((getFuncCtrl() & 0x3000u) >> 12);
        return  0x1 << bitV;
    }
    bool isByteScatterRW() const;
    bool isDWScatterRW() const;
    bool isQWScatterRW() const;
    bool isUntypedRW() const;

    bool isA64Message() const;

    // for sampler mesasges only
    bool isSampler() const { return getFuncId() == SFID::SAMPLER; }
    bool isCPSEnabled() const { return extDesc.layout.cps != 0; }
    uint32_t getSamplerMessageType() const;
    bool is16BitInput() const;
    bool is16BitReturn() const;


    bool isLSCTyped() const {return isTyped() && isLSC();}
    // atomic write or explicit barrier
    bool isBarrierOrAtomic() const {
        return isAtomicMessage() || isBarrier();
    }

    const G4_Operand *getBti() const {return m_bti;}
    G4_Operand *getBti()       {return m_bti;}
    const G4_Operand *getSti() const {return m_sti;}
    G4_Operand *getSti()       {return m_sti;}

    // In rare cases we must update the surface pointer
    // The send instructions also keeps a copy of the ExDesc parameter
    // as a proper source operand (e.g. for dataflow algorithms).
    // When they update their copy, they need to do the same for us.
    void setSurface(G4_Operand *newSurf) {m_bti = newSurf;}

    uint32_t getDesc() const { return desc.value; }
    uint32_t getExtendedDesc() const { return extDesc.value; }

    std::string getDescription() const override;
private:
    void setBindingTableIdx(unsigned idx);

public:
    ///////////////////////////////////////////////////////////////////////////
    // for the generic interface
    virtual size_t getSrc0LenBytes() const override;
    virtual size_t getDstLenBytes() const override;
    virtual size_t getSrc1LenBytes() const override;
    //
    virtual SendAccess getAccessType() const override {return accessType;}
    virtual std::pair<Caching,Caching> getCaching() const override;
    virtual void setCaching(Caching l1, Caching l3) override;
    //
    // If the message has an immediate address offset,
    // this returns that offset.  The offset is in bytes.
    virtual std::optional<ImmOff> getOffset() const override;

    virtual G4_Operand *getSurface() const override {return m_bti;}
    //
    virtual bool isEOT() const override {return isEOTInst();}
    virtual bool isSLM() const override {return isSLMMessage();}
    virtual bool isAtomic() const override {return isAtomicMessage();}
    virtual bool isBarrier() const override;
    virtual bool isFence() const override;
    virtual bool isHeaderPresent() const override;
    virtual bool isScratch() const override {return isScratchRW();}
    virtual bool isTyped() const override;
    //
}; // G4_SendDescRaw

} // vISA::

#endif // G4_SEND_DESCS_HPP
