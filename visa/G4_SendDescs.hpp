/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef G4_SEND_DESCS_HPP
#define G4_SEND_DESCS_HPP

#include "G4_MsgOpDefs.hpp"

#include <optional>
#include <ostream>
#include <string>
#include <tuple>
#include <utility>

namespace vISA {
enum class SendAccess {
  INVALID = 0,
  READ_ONLY,  // e.g. load, sampler operation
  WRITE_ONLY, // e.g. store, render target write
  READ_WRITE  // e.g. an atomic with return
};

std::string ToSymbol(vISA::SFID sfid);

//
// Various message operations
// This enumeration includes all SFIDs's messages that vISA should comprehend.
// Some of these work only in one SFID (e.g. sampler ops) and some work on
// several (e.g. load).
// Helper functions below this allow one to efficiently categorize an op.
// E.g. IsLoadOp(...)
enum class MsgOp {
  INVALID = 0,
#define DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, GROUP, ATTRS) \
  SYMBOL = ((ATTRS) << 16) | ((GROUP) << 8) | (ENCODING),
#include "G4_MsgOpDefs.hpp"
};
std::string ToSymbol(MsgOp);
static inline bool MsgOpIsLoad(MsgOp o) {
  return int(o) & (MSGOP_GROUP_LSC_LOAD << 8);
}
static inline bool MsgOpIsStore(MsgOp o) {
  return int(o) & (MSGOP_GROUP_LSC_STORE << 8);
}
static inline bool MsgOpIsAtomic(MsgOp o) {
  return int(o) & (MSGOP_GROUP_LSC_ATOMIC << 8);
}
static inline bool MsgOpIsLoadStoreAtomic(MsgOp o) {
  return MsgOpIsLoad(o) || MsgOpIsStore(o) || MsgOpIsAtomic(o);
}
static inline bool MsgOpIs2D(MsgOp op) {
  return op == MsgOp::LOAD_BLOCK2D || op == MsgOp::STORE_BLOCK2D;
}


static inline bool MsgOpIsApndCtrAtomic(MsgOp op) {
  return op == MsgOp::ATOMIC_ACADD || op == MsgOp::ATOMIC_ACSUB || op == MsgOp::ATOMIC_ACSTORE;
}

// does it have a data channel mask (e.g. load_quad)
bool MsgOpHasChMask(MsgOp);

uint32_t MsgOpEncode(MsgOp msgOp);
MsgOp MsgOpDecode(SFID sfid, uint32_t enc);

int MsgOpAtomicExtraArgs(MsgOp msgOp);

// Data size
enum class DataSize {
  INVALID = 0,
  D8,     // 8b
  D16,    // 16b
  D32,    // 32b
  D64,    // 64b
  D8U32,  // 8bit zero extended to 32bit
  D16U32, // 16bit zero extended to 32bit
};

std::string ToSymbol(DataSize d);
uint32_t GetDataSizeEncoding(DataSize ds);
uint32_t GetDataSizeBytesReg(DataSize ds); // returns 0 on invalid
uint32_t GetDataSizeBytesMem(DataSize ds); // returns 0 on invalid

// Data order
enum class DataOrder {
  INVALID         = 0x0,
  NONTRANSPOSE    = 0x1,
  TRANSPOSE       = 0x2,
  VNNI            = 0x3,
  TRANSPOSE_VNNI  = 0x4,
};

uint32_t GetDataOrderEncoding(DataOrder dord); // for non-block2d
uint32_t GetDataOrderEncoding2D(DataOrder dord); // for block2d

// Data elems
enum class VecElems { INVALID = 0, V1, V2, V3, V4, V8, V16, V32, V64 };

// data channel mask is a bitset of X, Y, Z, and W channels
// The ordinal values are direct mapping to encoding slots used
// in various fields
enum class DataChMask {
  // this is illegal in LSC but allowable possibly legal in other APIs
  EMPTY = 0,
  //
  // single channels
  X = 1 << 0,
  Y = 1 << 1,
  Z = 1 << 2,
  W = 1 << 3,
  //
  // dual channels
  XY = X | Y,
  XZ = X | Z,
  XW = X | W,
  YZ = Y | Z,
  YW = Y | W,
  ZW = Z | W,
  //
  // triple channels
  XYZ = X | Y | Z,
  XYW = X | Y | W,
  XZW = X | Z | W,
  YZW = Y | Z | W,
  //
  // quad channels
  XYZW = X | Y | Z | W,
};
// test encoding as [3:0]
static_assert(int(DataChMask::XYZW) == 0xF);
static_assert(int(DataChMask::YZW) == 0xE);

// composing channel masks
static inline DataChMask operator|(DataChMask c0, DataChMask c1) {
  return DataChMask(int(c0) | int(c1));
}

std::string ToSymbol(DataSize dsz, VecElems ve, DataOrder dord);
std::string ToSymbol(DataSize dsz, DataChMask chMask);

std::string ToSymbol(VecElems ve);
VecElems ToVecElems(int ves);
uint32_t GetVecElemsEncoding(VecElems ve);
int GetNumVecElems(VecElems ve);


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
  CC,  // cached as constant (load)
};

std::string ToSymbol(Caching);
// default, default returns ""
std::string ToSymbol(Caching, Caching);



struct ImmOff {
  bool is2d;
  union {
    int immOff;
    struct {
      short immOffX, immOffY;
    };
  };
  ImmOff(int imm) : is2d(false), immOff(imm) {}
  ImmOff(short immX, short immY) : is2d(true), immOffX(immX), immOffY(immY) {}
  ImmOff() : ImmOff(0) {}
};

enum class LdStAttrs {
  NONE = 0,
  //
  // for atomic messages that don't indicate if the return value is used
  ATOMIC_RETURN = 0x0001,
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
    XY = X | Y,
    XZ = X | Z,
    XW = X | W,
    XYZ = X | Y | Z,
    XYW = X | Y | W,
    XZW = X | Z | W,
    XYZW = X | Y | Z | W,
    //
    YZ = Y | Z,
    YW = Y | W,
    YZW = Y | Z | W,
    //
    ZW = Z | W,
  };

  // works on both channel masks and vector lengths
  int getCount() const;

  bool isChannelMask() const { return isChMask; }

  // asserts if not isChannelMask()
  Chs getMask() const;

  std::string str() const;

  ElemsPerAddr(int _count) : isChMask(false), count(_count) {}
  ElemsPerAddr(Chs chs) : isChMask(true), channels(chs) {}

private:
  bool isChMask;
  union {
    int count;
    Chs channels;
  };
}; // ElemsPerAddr
static inline ElemsPerAddr::Chs operator|(ElemsPerAddr::Chs c0,
                                          ElemsPerAddr::Chs c1) {
  return ElemsPerAddr::Chs(int(c0) | int(c1));
}

class G4_Operand;
class IR_Builder;

// Base class for all send descriptors.
// (Note that G4_SendDesc could be reused by more than one instruction.)
class G4_SendDesc {
  friend class G4_InstSend;

public:
  enum class Kind {
    INVALID,
    RAW, // G4_SendDescRaw
  };

protected:
  const Kind kind;

  const IR_Builder &irb;

  const SFID sfid;

  // The execution size for this message.
  G4_ExecSize execSize;

  // Limit access to G4_InstSend and any derived classes.
  void setExecSize(G4_ExecSize v) { execSize = v; }

public:
  G4_SendDesc(Kind k, SFID _sfid, const IR_Builder &builder)
      : kind(k), irb(builder), sfid(_sfid), execSize(g4::SIMD_UNDEFINED) {}
  G4_SendDesc(Kind k, SFID _sfid, G4_ExecSize _execSize,
              const IR_Builder &builder)
      : kind(k), irb(builder), sfid(_sfid), execSize(_execSize) {}

  SFID getSFID() const { return sfid; }

  G4_ExecSize getExecSize() const { return execSize; }

  bool isRaw() const { return kind == Kind::RAW; }
  //
  bool isHDC() const;
  bool isLSC() const;
  bool isSampler() const { return getSFID() == SFID::SAMPLER; }
  bool isGTWY() const {return getSFID() == SFID::GATEWAY;}
  //
  virtual bool isSLM() const = 0;
  virtual bool isBTS() const = 0; // BTS stateful
  virtual bool isTyped() const = 0;
  virtual bool isAtomic() const = 0;
  virtual bool isBarrier() const = 0;
  virtual bool isFence() const = 0;

  //
  // This gives a general access type
  virtual SendAccess getAccessType() const = 0;
  bool isRead() const {
    return getAccessType() == SendAccess::READ_ONLY ||
           getAccessType() == SendAccess::READ_WRITE;
  }
  bool isWrite() const {
    return getAccessType() == SendAccess::WRITE_ONLY ||
           getAccessType() == SendAccess::READ_WRITE;
  }
  bool isReadWrite() const { return getAccessType() == SendAccess::READ_WRITE; }

  // Returns the nubmer of elements each address (or coordinate) accesses
  //   E.g. d32x2 would return 2 (a message that loads a pair per address)
  //   E.g. d32.xyz would return 3
  //   E.g. d32x64t would return 64
  virtual unsigned getElemsPerAddr() const = 0;
  //
  // Returns the size in bytes of each element.
  // E.g. a d32x2 returns 4 (d32 is 32b)
  // This is the size of the element in memory not the register file
  // (which might widen the result in GRF).
  virtual unsigned getElemSize() const = 0;
  //
  // retrieves the caching for L1
  virtual Caching getCachingL1() const = 0;
  //
  // generally in multiples of full GRFs, but a few exceptions such
  // as OWord and HWord operations may make this different
  virtual size_t getDstLenBytes() const = 0;
  virtual size_t getSrc0LenBytes() const = 0;
  virtual size_t getSrc1LenBytes() const = 0;
  //
  // dst/src0/src1 len in GRF unit.
  // Return the value encoded in the send messages
  virtual size_t getDstLenRegs() const = 0;
  virtual size_t getSrc0LenRegs() const = 0;
  virtual size_t getSrc1LenRegs() const = 0;
  //
  // true if the message is a scratch space access (e.g. scratch block read)
  virtual bool isScratch() const = 0;
  //
  bool isScratchRead() const { return isScratch() && isRead(); }
  bool isScratchWrite() const { return isScratch() && isWrite(); }
  //
  // message offset in terms of bytes
  //   e.g. scratch offset
  virtual std::optional<ImmOff> getOffset() const = 0;

  virtual std::string getDescription() const = 0;
};

////////////////////////////////////////////////////////////////////////////
class G4_SendDescRaw : public G4_SendDesc {
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
  bool isLscDescriptor = false;
  // sfid now stored separately from the ExDesc[4:0] since the new LSC format
  // no longer uses ExDesc for that information
  int src1Len;

  // only used for LSC Xe2 BSS/SS; BTI and FLAT use exDesc
  uint32_t exDescImmOff = 0;

  // Mimic SendDescLdSt. Valid only for LSC msg. It's set via setLdStAttr(), not
  // ctor (should be removed if lsc switchs to use SendDescLdSt
  LdStAttrs attrs = LdStAttrs::NONE;

public:
  static const int SLMIndex = 0xFE;

  G4_SendDescRaw(uint32_t fCtrl, uint32_t regs2rcv, uint32_t regs2snd, SFID fID,
                 uint16_t extMsgLength, uint32_t extFCtrl, SendAccess access,
                 G4_Operand *bti, G4_Operand *sti, const IR_Builder &builder);

  /// Construct a object with descriptor and extended descriptor values.
  /// used in IR_Builder::createSendMsgDesc(uint32_t desc, uint32_t extDesc,
  /// SendAccess access)
  G4_SendDescRaw(uint32_t desc, uint32_t extDesc, SendAccess access,
                 G4_Operand *bti, G4_Operand *sti, const IR_Builder &builder);

  /// Preferred constructor takes an explicit SFID and src1 length
  G4_SendDescRaw(SFID sfid, uint32_t desc, uint32_t extDesc, int src1Len,
                 SendAccess access, G4_Operand *bti, bool isValidFuncCtrl,
                 const IR_Builder &builder);

  // Preferred constructor takes an explicit SFID and src1 length
  // Need execSize, so it is created for a particular send.
  G4_SendDescRaw(SFID sfid, uint32_t desc, uint32_t extDesc, int src1Len,
                 SendAccess access, G4_Operand *bti, G4_ExecSize execSize,
                 bool isValidFuncCtrl, const IR_Builder &builder);

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

  static uint32_t createExtDesc(SFID funcID, bool isEot = false) {
    return createExtDesc(funcID, isEot, 0, 0);
  }

  static uint32_t createMRTExtDesc(bool src0Alpha, uint8_t RTIndex, bool isEOT,
                                   uint32_t extMsgLen, uint16_t extFuncCtrl) {
    ExtDescData data;
    data.value = 0;
    data.layout.funcID = SFIDtoInt(SFID::DP_RC);
    data.layout.RTIndex = RTIndex;
    data.layout.src0Alpha = src0Alpha;
    data.layout.eot = isEOT;
    data.layout.extMsgLength = extMsgLen;
    data.layout.extFuncCtrl = extFuncCtrl;
    return data.value;
  }

  static uint32_t createExtDesc(SFID funcID, bool isEot, unsigned extMsgLen,
                                unsigned extFCtrl = 0) {
    ExtDescData data;
    data.value = 0;
    data.layout.funcID = SFIDtoInt(funcID);
    data.layout.eot = isEot;
    data.layout.extMsgLength = extMsgLen;
    data.layout.extFuncCtrl = extFCtrl;
    return data.value;
  }

  static uint32_t createDesc(uint32_t fc, bool headerPresent,
                             unsigned msgLength, unsigned rspLength) {
    DescData data;
    data.value = fc;
    data.layout.headerPresent = headerPresent;
    data.layout.msgLength = static_cast<uint16_t>(msgLength);
    data.layout.rspLength = static_cast<uint16_t>(rspLength);
    return data.value;
  }

  SFID getFuncId() const { return sfid; }

  uint32_t getFuncCtrl() const { return desc.layout.funcCtrl; }

  // tests ExDesc[5] on legacy platforms
  // NOTE: use G4_InstSend::isEOT(); EOT is an instruction option now
  bool hasLegacyEoT() const {return extDesc.layout.eot;}

  ////////////////////////////////////////////////////////////////////////
  // LSC-related operations
  bool isLscOp() const { return isLscDescriptor; }

  // TODO: update to use types defined in this file rather than
  // these front-end vISA interface types
  LSC_OP getLscOp() const {
    vASSERT(isLscOp());
    return static_cast<LSC_OP>(desc.value & 0x3F);
  }
  LSC_ADDR_TYPE getLscAddrType() const;
  int getLscAddrSizeBytes() const; // e.g. a64 => 8
  LSC_DATA_ORDER getLscDataOrder() const;
  LSC_FENCE_OP getLscFenceOp() const;

  int getLscImmOff() const;
  void setLscImmOff(int off);
  bool canSetLscImmOff(int off) const {
    return trySetLscImmOff(off, nullptr, nullptr);
  }
  bool trySetLscImmOff(int off, const char **whyFailed,
                       G4_SendDescRaw *rawDesc = nullptr) const;

  // query methods common for all raw sends
  uint16_t ResponseLength() const;
  uint16_t MessageLength() const { return desc.layout.msgLength; }
  uint16_t extMessageLength() const { return (uint16_t)src1Len; }
  void setExtMessageLength(uint16_t l) { src1Len = l; }
  bool isDataPortRead() const { return accessType != SendAccess::WRITE_ONLY; }
  bool isDataPortWrite() const { return accessType != SendAccess::READ_ONLY; }
  SendAccess getAccess() const { return accessType; }
  bool isValidFuncCtrl() const { return funcCtrlValid; }
  bool isHeaderPresent() const;
  void setHeaderPresent(bool val);

  ///////////////////////////////////////////////////////////////////////
  // for HDC messages only (DC0/DC1/DC2/DP_DCRO(aka DP_CC))
  //

  //////////////////////////////////////
  // calling these functions on non-HDC may assert
  uint16_t getExtFuncCtrl() const {
    vISA_ASSERT(isHDC(), "getExtFuncCtrl on non-HDC message");
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
  uint16_t getHWordScratchRWOffset() const {
    vISA_ASSERT(isHWordScratchRW(), "Message is not scratch space R/W.");
    return (getFuncCtrl() & 0xFFFu);
  }

  bool isLSCScratchRW() const {
    if (isLscDescriptor) {
      return hasAttrs(LdStAttrs::SCRATCH_SURFACE);
    }
    return false;
  }

  bool isHWordScratchRW() const {
    if (isLscDescriptor || !isValidFuncCtrl())
      return false;
    // legacy DC0 scratch msg: bit[18] = 1
    return getSFID() == SFID::DP_DC0 && ((getFuncCtrl() & 0x40000u) != 0);
  }

  bool isScratchRW() const { return isHWordScratchRW() || isLSCScratchRW(); }
  bool isHWordScratchRead() const {
    return isHWordScratchRW() && (getFuncCtrl() & 0x20000u) == 0;
  }
  bool isHWordScratchWrite() const {
    return isHWordScratchRW() && (getFuncCtrl() & 0x20000u) != 0;
  }
  // in terms of HWords (1, 2, 4, or 8)
  uint16_t getHWScratchRWSize() const {
    vISA_ASSERT(isHWordScratchRW(), "Message is not scratch space R/W.");
    uint16_t bitV = ((getFuncCtrl() & 0x3000u) >> 12);
    return 0x1 << bitV;
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


  // c.f. with G4_SendDescRaw::exDescImmOff
  void setExDescImmOff(uint32_t immOff) {
    vISA_ASSERT(immOff == 0 || getSFID() == SFID::UGM,
                 "this field is only legal given UGM");
    exDescImmOff = immOff;
  }
  uint32_t getExDescImmOff() const { return exDescImmOff; }
  bool isLSCTyped() const { return isTyped() && isLSC(); }
  // atomic write or explicit barrier
  bool isBarrierOrAtomic() const { return isAtomicMessage() || isBarrier(); }

  // Returns the associated surface (if any)
  // This can be a BTI (e.g. a0 register or G4_Imm if immediate)
  const G4_Operand *getBti() const { return m_bti; }
  G4_Operand *getBti() { return m_bti; }
  const G4_Operand *getSti() const { return m_sti; }
  G4_Operand *getSti() { return m_sti; }

  // In rare cases we must update the surface pointer
  // The send instructions also keeps a copy of the ExDesc parameter
  // as a proper source operand (e.g. for dataflow algorithms).
  // When they update their copy, they need to do the same for us.
  void setSurface(G4_Operand *newSurf) { m_bti = newSurf; }

  uint32_t getDesc() const { return desc.value; }
  uint32_t getExtendedDesc() const { return extDesc.value; }

  // LSC only
  void setLdStAttr(LdStAttrs aVal) { attrs = aVal; }
  bool hasAttrs(LdStAttrs a) const { return (int(a) & int(attrs)) == int(a); }

  std::string getDescription() const override;

  // Return data size of either dst or src1 in bytes for LSC
  // load/store instructions
  uint32_t getDataSizeInBytesLscLdStInst(Gen4_Operand_Number opnd_num) const;

private:
  void setBindingTableIdx(unsigned idx);

public:
  ///////////////////////////////////////////////////////////////////////////
  // for the generic interface
  virtual size_t getSrc0LenBytes() const override;
  virtual size_t getDstLenBytes() const override;
  virtual size_t getSrc1LenBytes() const override;

  virtual size_t getDstLenRegs() const override { return ResponseLength(); }
  virtual size_t getSrc0LenRegs() const override { return MessageLength(); }
  virtual size_t getSrc1LenRegs() const override;
  //
  virtual SendAccess getAccessType() const override { return accessType; }
  //
  // Returns the caching behavior of this message if known.
  // Returns Caching::INVALID if the message doesn't support caching
  // controls.
  Caching getCachingL1() const override { return getCaching().first; }
  Caching getCachingL3() const { return getCaching().second; }
  std::pair<Caching, Caching> getCaching() const;
  void setCaching(Caching l1, Caching l3);
  //
  // If the message has an immediate address offset,
  // this returns that offset.  The offset is in bytes.
  virtual std::optional<ImmOff> getOffset() const override;

  virtual bool isSLM() const override { return isSLMMessage(); }
  virtual bool isBTS() const override;
  virtual bool isAtomic() const override { return isAtomicMessage(); }
  virtual bool isBarrier() const override;
  virtual bool isFence() const override;
  virtual bool isScratch() const override { return isScratchRW(); }
  virtual bool isTyped() const override;
  //
}; // G4_SendDescRaw

} // namespace vISA

#endif // G4_SEND_DESCS_HPP
