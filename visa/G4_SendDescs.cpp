/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BuildIR.h"
#include "G4_IR.hpp"
#include "IGC/common/StringMacros.hpp"

#include <iomanip>
#include <sstream>
#include <limits>
#include <optional>

using namespace vISA;

static uint64_t getBitFieldMask(int off, int len) {
  uint64_t mask = len == 64 ?
    std::numeric_limits<uint64_t>::max() : (1ull << len) - 1;
  return mask << off;
}
static uint64_t getBitField(uint64_t bits, int off, int len) {
  return ((bits & getBitFieldMask(off, len)) >> off);
}
[[maybe_unused]]
static uint64_t getSignedBitField(uint64_t bits, int off, int len) {
  auto shlToTopSignBit = 64 - off - len;
  return (int64_t)(bits << shlToTopSignBit) >> (shlToTopSignBit + off);
}
[[maybe_unused]]
static uint64_t putBitField(uint64_t bits, int off, int len, uint64_t val) {
  const uint64_t mask = getBitFieldMask(off, len);
  return (bits & ~mask) | (mask & (val << off));
}
static std::string fmtSignedHex(int64_t val) {
  std::stringstream ss;
  if (val < 0) {
    ss << "-";
    val = -val;
  }
  ss << "0x" << std::uppercase << std::hex << val;
  return ss.str();
}
static std::string fmtSignedHexTerm(int64_t val) {
  if (val == 0)
    return "";
  std::stringstream ss;
  if (val > 0)
    ss << "+" << fmtSignedHex(val);
  else
    ss << fmtSignedHex(val); // will prefix -
  return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
std::string vISA::ToSymbol(MsgOp op) {
  switch (op) {
#define DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, GROUP, ATTRS) \
  case (MsgOp::SYMBOL): return (SYNTAX);
#include "G4_MsgOpDefs.hpp"
  default: return "MsgOp::<" + fmtSignedHex(int(op)) + "?>";
  }
}

#define DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, GROUP, ATTRS) \
  static_assert(((ENCODING) & ~0x3F) == 0,                                    \
                #SYMBOL ": MsgOp encoding overflowed");                       \
  static_assert(((GROUP) & ~0xFFF) == 0,                                      \
                #SYMBOL ": MsgOp group overflowed");                          \
  static_assert(((GROUP) & ((GROUP) - 1)) == 0,                               \
                #SYMBOL ": MsgOp should belong to only one group");           \
  static_assert(((ATTRS) & ~0xFFFF) == 0,                                     \
                #SYMBOL ": MsgOp attrs overflowed");
#include "G4_MsgOpDefs.hpp"

std::string vISA::ToSymbol(vISA::SFID sfid) {
  switch (sfid) {
  case SFID::UGM:
    return "ugm";
  case SFID::UGML:
    return "ugml";
  case SFID::SLM:
    return "slm";
  case SFID::TGM:
    return "tgm";
  case SFID::URB:
    return "urb";
  //
  case SFID::DP_DC0:
    return "dc0";
  case SFID::DP_DC1:
    return "dc1";
  case SFID::DP_DC2:
    return "dc2";
  case SFID::DP_CC:
    return "dcro";
  case SFID::DP_RC:
    return "rc";
  //
  case SFID::RTHW:
    return "rta";
  case SFID::BTD:
    return "btd";
  //
  case SFID::GATEWAY:
    return "gtwy";
  case SFID::SAMPLER:
    return "smpl";
  case SFID::NULL_SFID:
    return "null";
  case SFID::CRE:
    return "cre";
  default:
    return "SFID::<" + fmtSignedHex(int(sfid)) + "?>";
  }
}

[[maybe_unused]]
static std::string ToSymbolDataSize(int reg, int mem) {
  if (reg == mem)
    return "d" + std::to_string(reg);
  return "d" + std::to_string(mem) + "u" + std::to_string(reg);
}

bool vISA::MsgOpHasChMask(MsgOp op) {
  switch (op) {
  case MsgOp::LOAD_QUAD:
  case MsgOp::STORE_QUAD:
  case MsgOp::LOAD_QUAD_MSRT:
  case MsgOp::STORE_QUAD_MSRT:
    return true;
  default:
    break;
  }
  return false;
}

uint32_t vISA::MsgOpEncode(MsgOp m) {
  switch (m) {
#define DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, GROUP, ATTRS) \
  case (MsgOp::SYMBOL): return (ENCODING);
#include "G4_MsgOpDefs.hpp"
  default:
    vISA_ASSERT_UNREACHABLE("Invalid msg op");
    return 0xFFFFFFFF; // return all 1's to try and generate an error (0 is load in LSC)
  }
}

MsgOp vISA::MsgOpDecode(SFID sfid, uint32_t enc) {
  switch (sfid)
  {
  //////////////////////////////
  // decode LSC
  case SFID::UGM:
  case SFID::UGML:
  case SFID::TGM:
  case SFID::SLM:
  // URB in LSC is only Xe2+ but MsgOp should only be used for URB for
  // Xe2+ (so no dynamic platform check is needed)
  case SFID::URB:
    switch (enc)
    {
#define DEFINE_G4_MSGOP_LSC_LOAD(SYMBOL, SYNTAX, ENCODING, ATTRS) \
    case ENCODING: return MsgOp::SYMBOL;
#define DEFINE_G4_MSGOP_LSC_STORE(SYMBOL, SYNTAX, ENCODING, ATTRS) \
    case ENCODING: return MsgOp::SYMBOL;
#define DEFINE_G4_MSGOP_LSC_ATOMIC(SYMBOL, SYNTAX, ENCODING, ATTRS) \
    case ENCODING: return MsgOp::SYMBOL;
#define DEFINE_G4_MSGOP_LSC_OTHER(SYMBOL, SYNTAX, ENCODING, ATTRS) \
    case ENCODING: return MsgOp::SYMBOL;
#include "G4_MsgOpDefs.hpp"
    default: break; // fallthrough to INVALID
    }
    break;

  //////////////////////////////
  // decode gatway
  case SFID::GATEWAY:
    switch (enc)
    {
#define DEFINE_G4_MSGOP_GTWY(SYMBOL, SYNTAX, ENCODING, ATTRS) \
    case ENCODING: return MsgOp::SYMBOL;
#include "G4_MsgOpDefs.hpp"
    default: break; // fallthrough to INVALID
    }
    break;

  //////////////////////////////
  // decode sampler
  case SFID::SAMPLER:
    switch (enc)
    {
#define DEFINE_G4_MSGOP_SMPL_NORMAL(SYMBOL, SYNTAX, ENCODING, ATTRS) \
    case ENCODING: return MsgOp::SYMBOL;
#define DEFINE_G4_MSGOP_SMPL_GATHER(SYMBOL, SYNTAX, ENCODING, ATTRS) \
    case ENCODING: return MsgOp::SYMBOL;
#include "G4_MsgOpDefs.hpp"
    default: break; // fallthrough to INVALID
    }
    break;

  //////////////////////////////
  // decode render target
  case SFID::DP_RC:
    switch (enc)
    {
#define DEFINE_G4_MSGOP_RENDER(SYMBOL, SYNTAX, ENCODING, ATTRS) \
    case ENCODING: return MsgOp::SYMBOL;
#include "G4_MsgOpDefs.hpp"
    default: break; // fallthrough to INVALID
    }
    break;

  //////////////////////////////
  // decode render target
  case SFID::RTHW:
    switch (enc)
    {
#define DEFINE_G4_MSGOP_RTA(SYMBOL, SYNTAX, ENCODING, ATTRS) \
    case ENCODING: return MsgOp::SYMBOL;
#include "G4_MsgOpDefs.hpp"
    default: break; // fallthrough to INVALID
    }
    break;

  //////////////////////////////
  // decode render target
  case SFID::BTD:
    switch (enc)
    {
#define DEFINE_G4_MSGOP_BTD(SYMBOL, SYNTAX, ENCODING, ATTRS) \
    case ENCODING: return MsgOp::SYMBOL;
#include "G4_MsgOpDefs.hpp"
    default: break; // fallthrough to INVALID
    }
    break;
  default:
    break; // invalid SFID; fallthrough
  }
  return MsgOp::INVALID;
}

int vISA::MsgOpAtomicExtraArgs(MsgOp msgOp) {
  if ((unsigned(msgOp) >> 16) & MSGOP_ATTRS_ATOMIC_UNARY) {
    return 0;
  } else if ((unsigned(msgOp) >> 16) & MSGOP_ATTRS_ATOMIC_BINARY) {
    return 1;
  } else if ((unsigned(msgOp) >> 16) & MSGOP_ATTRS_ATOMIC_TERNARY) {
    return 2;
  } else {
    vISA_ASSERT_UNREACHABLE("expected atomic op");
    return 0;
  }
}

// data size
std::string vISA::ToSymbol(DataSize d) {
  switch (d) {
  case DataSize::D8:
    return "d8";
  case DataSize::D16:
    return "d16";
  case DataSize::D32:
    return "d32";
  case DataSize::D64:
    return "d64";
  case DataSize::D8U32:
    return "d8u32";
  case DataSize::D16U32:
    return "d16u32";
  default:
    return "d?";
  }
}

uint32_t vISA::GetDataSizeEncoding(DataSize ds) {
  switch (ds) {
  case DataSize::D8:
    return 0;
  case DataSize::D16:
    return 1;
  case DataSize::D32:
    return 2;
  case DataSize::D64:
    return 3;
  case DataSize::D8U32:
    return 4;
  case DataSize::D16U32:
    return 5;
  default:
    vISA_ASSERT_UNREACHABLE("invalid data size");
  }
  return 0;
}
uint32_t vISA::GetDataSizeBytesReg(DataSize ds)
{
  switch (ds) {
  case DataSize::D8:
    return 1;
  case DataSize::D16:
    return 2;
  case DataSize::D32:
  case DataSize::D8U32:
  case DataSize::D16U32:
    return 4;
  case DataSize::D64:
    return 8;
  default:
    break;
  }
  return 0;
}
uint32_t vISA::GetDataSizeBytesMem(DataSize ds)
{
  switch (ds) {
  case DataSize::D8:
  case DataSize::D8U32:
    return 1;
  case DataSize::D16:
  case DataSize::D16U32:
    return 2;
  case DataSize::D32:
    return 4;
  case DataSize::D64:
    return 8;
  default:
    break;
  }
  return 0;
}
uint32_t vISA::GetDataOrderEncoding(DataOrder dord) {
  switch (dord) {
  case DataOrder::NONTRANSPOSE:
    return 0;
  case DataOrder::TRANSPOSE:
    return 1;
  default:
    vISA_ASSERT_UNREACHABLE("invalid data order");
  }
  return 0;
}
uint32_t vISA::GetDataOrderEncoding2D(DataOrder dord) {
  // for block2d Desc[10:9] (combining data order with vnni)
  switch (dord) {
  case DataOrder::NONTRANSPOSE: // non-transpose non-vnni
    return 0x0;
  case DataOrder::VNNI: // non-transpose+vnni
    return 0x1;
  case DataOrder::TRANSPOSE:
    return 0x2;
  case DataOrder::TRANSPOSE_VNNI:
    return 0x3;
  default:
    vISA_ASSERT_UNREACHABLE("invalid data order");
  }
  return 0;
}

std::string vISA::ToSymbol(DataSize dsz, VecElems ve, DataOrder dord) {
  std::stringstream ss;
  ss << ToSymbol(dsz);
  if (ve != VecElems::V1 || dord == DataOrder::TRANSPOSE)
    ss << "x" << ToSymbol(ve);
  switch (dord) {
  case DataOrder::NONTRANSPOSE:
    break;
  case DataOrder::TRANSPOSE:
    ss << "t";
    break;
  case DataOrder::VNNI:
    ss << "v";
    break;
  case DataOrder::TRANSPOSE_VNNI:
    ss << "tv";
    break;
  default:
    ss << "?";
  }
  return ss.str();
}
std::string vISA::ToSymbol(DataSize dsz, DataChMask chMask) {
  std::stringstream ss;
  ss << ToSymbol(dsz) << ".";
  for (int ch = 0; ch < 4; ch++) {
    if (int(chMask) & (1 << ch)) {
      ss << "xyzw"[ch];
    }
  }
  return ss.str();
}

// data elems
std::string vISA::ToSymbol(VecElems v) {
  switch (v) {
  case VecElems::V1:
    return "1";
  case VecElems::V2:
    return "2";
  case VecElems::V3:
    return "3";
  case VecElems::V4:
    return "4";
  case VecElems::V8:
    return "8";
  case VecElems::V16:
    return "16";
  case VecElems::V32:
    return "32";
  case VecElems::V64:
    return "64";
  default:
    return "?";
  }
}

VecElems vISA::ToVecElems(int ves) {
  switch (ves) {
  case 1: return VecElems::V1;
  case 2: return VecElems::V2;
  case 3: return VecElems::V3;
  case 4: return VecElems::V4;
  case 8: return VecElems::V8;
  case 16: return VecElems::V16;
  case 32: return VecElems::V32;
  case 64: return VecElems::V64;
  default: return VecElems::INVALID;
  }
}
uint32_t vISA::GetVecElemsEncoding(VecElems ve) {
  switch (ve) {
  case VecElems::V1:
    return 0;
  case VecElems::V2:
    return 1;
  case VecElems::V3:
    return 2;
  case VecElems::V4:
    return 3;
  case VecElems::V8:
    return 4;
  case VecElems::V16:
    return 5;
  case VecElems::V32:
    return 6;
  case VecElems::V64:
    return 7;
  default:
    vISA_ASSERT_UNREACHABLE("invalid vector elements");
  }
  return 0;
}
int vISA::GetNumVecElems(VecElems ves) {
  switch (ves) {
  case VecElems::V1: return 1;
  case VecElems::V2: return 2;
  case VecElems::V3: return 3;
  case VecElems::V4: return 4;
  case VecElems::V8: return 8;
  case VecElems::V16: return 16;
  case VecElems::V32: return 32;
  case VecElems::V64: return 64;
  default: return 0;
  }
}


std::string vISA::ToSymbol(Caching c) {
  switch (c) {
  case Caching::CA:
    return ".ca";
  case Caching::DF:
    return ".df";
  case Caching::RI:
    return ".ri";
  case Caching::ST:
    return ".st";
  case Caching::WB:
    return ".wb";
  case Caching::WT:
    return ".wt";
  case Caching::UC:
    return ".uc";
  case Caching::CC:
    return ".cc";
  default:
    return "?";
  }
}

std::string vISA::ToSymbol(Caching l1, Caching l3) {
  if (l1 == Caching::DF && l3 == Caching::DF)
    return "";
  else
    return ToSymbol(l1) + ToSymbol(l3);
}
int ElemsPerAddr::getCount() const {
  if (!isChannelMask())
    return count;
  return ((int(channels) & int(Chs::X)) ? 1 : 0) +
         ((int(channels) & int(Chs::Y)) ? 1 : 0) +
         ((int(channels) & int(Chs::Z)) ? 1 : 0) +
         ((int(channels) & int(Chs::W)) ? 1 : 0);
}

ElemsPerAddr::Chs ElemsPerAddr::getMask() const {
  vISA_ASSERT(isChannelMask(), "must be a channel mask vector");
  return channels;
}

std::string ElemsPerAddr::str() const {
  if (isChannelMask()) {
    if (channels == Chs::INVALID)
      return ".?";
    // e.g. .xyz
    std::string s = ".";
    if (int(channels) & int(Chs::X))
      s += 'x';
    if (int(channels) & int(Chs::Y))
      s += 'y';
    if (int(channels) & int(Chs::Z))
      s += 'z';
    if (int(channels) & int(Chs::W))
      s += 'w';
    return s;
  } else {
    // e.g. x4 (note absence of a dot)
    return "x" + std::to_string(count);
  }
}

///////////////////////////////////////////////////////////////////////////////
// G4_SendDesc implementations
///////////////////////////////////////////////////////////////////////////////

bool G4_SendDesc::isHDC() const {
  auto funcID = getSFID();
  return funcID == SFID::DP_DC0 || funcID == SFID::DP_DC1 ||
         funcID == SFID::DP_DC2 || funcID == SFID::DP_CC;
}

bool G4_SendDesc::isLSC() const {
  switch (getSFID()) {
  case SFID::UGM:
  case SFID::UGML:
  case SFID::TGM:
  case SFID::SLM:
    return true;
  case SFID::URB:
    return irb.getPlatform() >= TARGET_PLATFORM::Xe2;
  default:
    break;
  }
  return false;
}


///////////////////////////////////////////////////////////////////////////////
// G4_SendDescRaw implementations
///////////////////////////////////////////////////////////////////////////////
G4_SendDescRaw::G4_SendDescRaw(uint32_t fCtrl, uint32_t regs2rcv,
                               uint32_t regs2snd, SFID fID, uint16_t extMsgLen,
                               uint32_t extFCtrl, SendAccess access,
                               G4_Operand *bti, G4_Operand *sti,
                               const IR_Builder &builder)
    : G4_SendDesc(G4_SendDesc::Kind::RAW, fID, builder) {
  // All unnamed bits should be passed with those control bits.
  // Otherwise, need to be set individually.
  desc.value = fCtrl;

  desc.layout.rspLength = regs2rcv;
  desc.layout.msgLength = regs2snd;

  extDesc.value = 0;
  extDesc.layout.funcID = SFIDtoInt(fID);
  extDesc.layout.extMsgLength = extMsgLen;
  extDesc.layout.extFuncCtrl = extFCtrl;

  src1Len = extMsgLen; // ExDesc[10:6] on some platforms; in EU ISA on others

  accessType = access;
  funcCtrlValid = true;

  m_bti = bti;
  m_sti = sti;

  if (m_bti && m_bti->isImm()) {
    setBindingTableIdx((unsigned)m_bti->asImm()->getInt());
  }
  if (m_sti && m_sti->isImm()) {
    desc.value |= (((unsigned)m_sti->asImm()->getInt()) << 8); // [11:8]
  }

  [[maybe_unused]]
  uint32_t totalMaxLength = builder.getMaxSendMessageLength();
  vISA_ASSERT(extDesc.layout.extMsgLength + desc.layout.msgLength <
                   totalMaxLength,
               "combined message length may not exceed the maximum");
}

G4_SendDescRaw::G4_SendDescRaw(uint32_t descBits, uint32_t extDescBits,
                               SendAccess access, G4_Operand *bti,
                               G4_Operand *sti, const IR_Builder &builder)
    : G4_SendDesc(G4_SendDesc::Kind::RAW,
                  intToSFID(extDescBits & 0xF, builder.getPlatform()),
                  builder), // [3:0]
      accessType(access), funcCtrlValid(true), m_sti(sti), m_bti(bti) {
  desc.value = descBits;
  extDesc.value = extDescBits;
  src1Len = (extDescBits >> 6) & 0x1F;  // [10:6]

  if (bti && bti->isImm()) {
    setBindingTableIdx((unsigned)bti->asImm()->getInt());
  }
  if (sti && sti->isImm()) {
    desc.value |= (((unsigned)m_sti->asImm()->getInt()) << 8); // [11:8]
  }
}

G4_SendDescRaw::G4_SendDescRaw(SFID _sfid, uint32_t _desc, uint32_t _extDesc,
                               int _src1Len, SendAccess access, G4_Operand *bti,
                               bool isValidFuncCtrl, const IR_Builder &builder)
    : G4_SendDescRaw(_sfid, _desc, _extDesc, _src1Len, access, bti,
                     g4::SIMD_UNDEFINED, isValidFuncCtrl, builder) {}

G4_SendDescRaw::G4_SendDescRaw(SFID _sfid, uint32_t _desc, uint32_t _extDesc,
                               int _src1Len, SendAccess access, G4_Operand *bti,
                               G4_ExecSize execSize, bool isValidFuncCtrl,
                               const IR_Builder &builder)
    : G4_SendDesc(G4_SendDesc::Kind::RAW, _sfid, execSize, builder),
      accessType(access), funcCtrlValid(isValidFuncCtrl), m_sti(nullptr), m_bti(bti)
{
  isLscDescriptor = _sfid == SFID::UGM || _sfid == SFID::UGML ||
                    _sfid == SFID::SLM || _sfid == SFID::TGM;
  if (irb.getPlatform() >= Xe2) {
    isLscDescriptor |= _sfid == SFID::URB;
  }

  // ensure ExDesc[10:6] also holds src1Len
  // see the note above (other constructor) about DG2 descriptors and
  // ExDesc[10:6]
  _extDesc |= ((_src1Len & 0x1F) << 6);
  desc.value = _desc;
  extDesc.value = _extDesc;
  src1Len = _src1Len;
}

uint32_t G4_SendDescRaw::getHdcMessageType() const {
  vISA_ASSERT(isHDC(), "not an HDC message");
  return (desc.value >> 14) & 0x1F;
}

LSC_ADDR_TYPE G4_SendDescRaw::getLscAddrType() const {
  vISA_ASSERT(isLscOp(), "must be LSC op");
  const int LSC_ADDR_TYPE_OFFSET = 29;
  const uint32_t LSC_ADDR_TYPE_MASK = 0x3;
  const uint32_t rawDescBits = getDesc();
  auto addrTypeBits =
      ((rawDescBits >> LSC_ADDR_TYPE_OFFSET) & LSC_ADDR_TYPE_MASK);
  return LSC_ADDR_TYPE(addrTypeBits + 1);
}

int G4_SendDescRaw::getLscAddrSizeBytes() const {
  vISA_ASSERT(isLscOp(), "must be LSC op");
  auto op = getLscOp();
  switch (op) {
  case LSC_LOAD:
  case LSC_LOAD_STRIDED:
  case LSC_LOAD_QUAD:
  case LSC_STORE:
  case LSC_STORE_STRIDED:
  case LSC_STORE_QUAD:
    break;
  case LSC_LOAD_BLOCK2D:
  case LSC_STORE_BLOCK2D:
    return getSFID() == SFID::TGM ? 4 : 8;
  default:
    if (op < LSC_ATOMIC_IINC && op > LSC_ATOMIC_XOR) {
      return 0;
    }
  }
  // it's a good op with an AddrType field in [8:7]
  switch ((getDesc() >> 7) & 0x3) {
  case 1:
    return 2;
  case 2:
    return 4;
  case 3:
    return 8;
  default:
    break;
  }
  return 0;
}

LSC_DATA_ORDER G4_SendDescRaw::getLscDataOrder() const {
  vISA_ASSERT(isLscOp(), "must be LSC op");
  auto op = getLscOp();
  if (op == LSC_LOAD_QUAD || op == LSC_STORE_QUAD)
    return LSC_DATA_ORDER_NONTRANSPOSE;
  if ((getDesc() >> 15) & 0x1) {
    return LSC_DATA_ORDER_TRANSPOSE;
  } else {
    return LSC_DATA_ORDER_NONTRANSPOSE;
  }
}

LSC_FENCE_OP G4_SendDescRaw::getLscFenceOp() const {
  vISA_ASSERT(isLscOp(), "must be LSC op");
  vISA_ASSERT(isFence(), "must be fence op");
  return static_cast<LSC_FENCE_OP>((desc.value >> 12) & 0x7);
}

int G4_SendDescRaw::getLscImmOff() const {
  vISA_ASSERT(isLscOp(), "must be LSC op");
  if (getSFID() == SFID::TGM)
    return 0;
  switch (getLscAddrType()) {
  case LSC_ADDR_TYPE_BSS:
  case LSC_ADDR_TYPE_SS: {
    // [31:19][18:16][15:12]
    // [16:4][MBZs][3:0]
    if (getBti() == nullptr) {
      return 0; // no offset if using imm bss/ss
    }
    uint32_t bits = getExDescImmOff();
    uint32_t packed = (bits & 0xFFFC0000) | ((bits & 0xF000) << 3);
    int off = (int32_t)packed >> (12 + 3);
    return off;
  }
  case LSC_ADDR_TYPE_BTI:
    if (getBti()) {
      return 0; // no offset if using reg BTI
    }
    return ((int32_t)getExtendedDesc() << 8) >> (8 + 12);
  case LSC_ADDR_TYPE_FLAT:
    return ((int32_t)getExtendedDesc()) >> 12;
  default:
    break;
  }
  return 0;
}

void G4_SendDescRaw::setLscImmOff(int immOff) {
  const char *err = "???";
  if (!trySetLscImmOff(immOff, &err, this)) {
    vISA_ASSERT(false, err);
  }
}
bool G4_SendDescRaw::trySetLscImmOff(int immOff, const char **whyFailed,
                                     G4_SendDescRaw *rawDesc) const {
  // Xe2 supports an signed immediate offset
  //   - must be DW aligned but value is in signed bytes
  //   - not TGM (only UGM, SLM, URB, ...)
  //   - enabled for BTI [23:12] and flat [31:12]
  //     things aren't well defined given BTI if ExDesc.IsReg
  //   - ExDesc must be an immediate field, not an a0.# register
  //      The spec says: "Must programmed with an immediate value in EU SEND
  //      instruction."
  //       (and I confirmed this was the meaning)
  // Xe2 extends this support for BSS/SS, but only
  //      if ExDesc is a register (we also get most of the ExDescImm bits)
  auto failed = [&](const char *err) {
    if (whyFailed)
      *whyFailed = err;
    return false;
  };
  if (!isLscOp()) {
    return failed("wrong type descriptor");
  } else if (irb.getPlatform() < Xe2) {
    return failed("not supported on this platform");
  } else if (getSFID() == SFID::TGM) {
    return failed("cannot promote on TGM");
  }
  switch (getLscOp()) {
  case LSC_LOAD:
  case LSC_LOAD_QUAD:
  case LSC_LOAD_STRIDED:
  case LSC_STORE:
  case LSC_STORE_QUAD:
  case LSC_STORE_STRIDED:
    break;
  default:
    if (getLscOp() < LSC_ATOMIC_IINC && getLscOp() > LSC_ATOMIC_XOR) {
      return failed("unsupported op");
    }
    break;
  }
  if (immOff % 4 != 0) {
    return failed("imm offset not DW aligned");
  }
  // ensure it fits in range
  auto fitsIn = [&](int bits) {
    return immOff >= -(1LL << (bits - 1)) && immOff <= (1LL << (bits - 1)) - 1;
  };
  auto addrType = getLscAddrType();
  switch (addrType) {
  case LSC_ADDR_TYPE_BSS:
  case LSC_ADDR_TYPE_SS:
    if (getBti() == nullptr) {
      return failed("this addr type requires reg exdesc");
    }
    if (!fitsIn(17))
      return failed("imm offset too large");
    if (rawDesc) {
      uint32_t encddUnshifted =
          (((uint32_t)immOff & ~0xF) << 3) | ((uint32_t)immOff & 0xF);
      rawDesc->setExDescImmOff((uint32_t)(encddUnshifted << 12));
    }
    break;
  case LSC_ADDR_TYPE_BTI:
    if (getBti()) {
      return failed("this addr type requires imm bti");
    }
    if (!fitsIn(12))
      return failed("imm offset too large");
    if (rawDesc)
      rawDesc->extDesc.value = (rawDesc->extDesc.value & 0xFF000000) |
                               (0x00FFF000 & ((uint32_t)immOff << 12));
    break;
  case LSC_ADDR_TYPE_FLAT:
    if (!fitsIn(20))
      return failed("imm offset too large");
    if (rawDesc)
      rawDesc->extDesc.value = ((uint32_t)immOff << 12);
    break;
  default:
    return false;
  }
  return true;
}

static bool isHdcIntAtomicMessage(SFID funcID, uint16_t msgType,
                                  const IR_Builder &irb) {
  if (funcID != SFID::DP_DC1)
    return false;

  if (msgType == DC1_UNTYPED_ATOMIC || msgType == DC1_A64_ATOMIC) {
    return true;
  }
  if (irb.getPlatform() >= GENX_SKL) {
    if (msgType == DC1_TYPED_ATOMIC)
      return true;
  }
  if (irb.getPlatformGeneration() >= PlatformGen::XE) {
    if (msgType == DC1_TYPED_HALF_INTEGER_ATOMIC ||
        msgType == DC1_TYPED_HALF_COUNTER_ATOMIC ||
        msgType == DC1_UNTYPED_HALF_INTEGER_ATOMIC ||
        msgType == DC1_A64_UNTYPED_HALF_INTEGER_ATOMIC)
      return true;
  }
  return false;
}

static bool isHdcFloatAtomicMessage(SFID funcID, uint16_t msgType,
                                    const IR_Builder &irb) {
  if (funcID != SFID::DP_DC1)
    return false;

  if (irb.getPlatform() >= GENX_SKL) {
    if (msgType == DC1_UNTYPED_FLOAT_ATOMIC ||
        msgType == DC1_A64_UNTYPED_FLOAT_ATOMIC)
      return true;
  }
  if (irb.getPlatformGeneration() >= PlatformGen::XE) {
    if (msgType == DC1_UNTYPED_HALF_FLOAT_ATOMIC ||
        msgType == DC1_A64_UNTYPED_HALF_FLOAT_ATOMIC)
      return true;
  }
  return false;
}

bool G4_SendDescRaw::isAtomicMessage() const {
  if (isLscOp() && (desc.value & 0x3F) >= LSC_ATOMIC_IINC &&
      (desc.value & 0x3F) <= LSC_ATOMIC_XOR) {
    return true;
  }

  auto funcID = getSFID();
  if (!isHDC())
    return false; // guard getMessageType() on SFID without a message type
  uint16_t msgType = getHdcMessageType();
  return isHdcIntAtomicMessage(funcID, msgType, irb) ||
         isHdcFloatAtomicMessage(funcID, msgType, irb);
}

uint16_t G4_SendDescRaw::getHdcAtomicOp() const {
  vISA_ASSERT(isHDC(), "must be HDC message");
  vISA_ASSERT(isAtomicMessage(), "getting atomicOp from non-atomic message!");
  uint32_t funcCtrl = getFuncCtrl();
  if (isHdcIntAtomicMessage(getSFID(), getHdcMessageType(), irb)) {
    // bits: 11:8
    return (uint16_t)((funcCtrl >> 8) & 0xF);
  }

  // must be float Atomic
  // bits: 10:8
  return (int16_t)((funcCtrl >> 8) & 0x7);
}

bool G4_SendDescRaw::isSLMMessage() const {
  if (getSFID() == SFID::DP_DC2) {
    uint32_t msgType = getHdcMessageType();
    if ((msgType == DC2_UNTYPED_SURFACE_WRITE ||
         msgType == DC2_BYTE_SCATTERED_WRITE) &&
        (getFuncCtrl() & 0x80)) {
      return true;
    }
  }

  if (getSFID() == SFID::DP_DC2 || getSFID() == SFID::DP_DC1 ||
      getSFID() == SFID::DP_DC0) {
    if ((getDesc() & 0xFF) == SLMIndex) {
      return true;
    }
  }

  if (m_bti && m_bti->isImm() && m_bti->asImm()->getInt() == SLMIndex) {
    return true;
  }

  return getSFID() == SFID::SLM;
}

uint16_t G4_SendDescRaw::ResponseLength() const {
  // the loadblock2DArray message may return up to 32 GRF.
  // Since we don't have enough bits to encode 32, block2d creates an exception
  // where 31 means 31 or 32 (HW detects). SW must know the actual size is 32
  // for data-flow/RA/SWSB to function correctly though. fortunately it doesn't
  // look like 31 is a valid value for this message, we just treat 31 as 32
  bool isLoadBlock2DArray = isLscOp() && getLscOp() == LSC_LOAD_BLOCK2D;
  if (desc.layout.rspLength == 31 && isLoadBlock2DArray) {
    return 32;
  }
  return desc.layout.rspLength;
}

bool G4_SendDescRaw::isHeaderPresent() const {
  if (isLscOp())
    return false;

  return desc.layout.headerPresent == 1;
}

void G4_SendDescRaw::setHeaderPresent(bool val) {
  vISA_ASSERT(!isLscOp(), "LSC ops don't have headers");
  desc.layout.headerPresent = val;
}

void G4_SendDescRaw::setBindingTableIdx(unsigned idx) {
  if (isLscOp()) {
    extDesc.value |= (idx << 24);
    return;
  }
  desc.value |= idx;
}

uint32_t G4_SendDescRaw::getSamplerMessageType() const {
  vISA_ASSERT(isSampler(), "wrong descriptor type for method");
  return (getFuncCtrl() >> 12) & 0x1f;
}

bool G4_SendDescRaw::is16BitInput() const {
  vISA_ASSERT(!isLscOp(), "wrong descriptor type for method");
  // TODO: could use this for LSC messages too potentially
  return desc.layout.simdMode2 == 1;
}

bool G4_SendDescRaw::is16BitReturn() const {
  vISA_ASSERT(!isLscOp(), "wrong descriptor type for method");
  return desc.layout.returnFormat == 1;
}

bool G4_SendDescRaw::isByteScatterRW() const {
  auto funcID = getSFID();
  switch (funcID) {
  case SFID::DP_DC0:
    switch (getHdcMessageType()) {
    case DC_BYTE_SCATTERED_READ:
    case DC_BYTE_SCATTERED_WRITE:
      return true;
    default:
      break;
    }
    break;
  case SFID::DP_DC1:
    switch (getHdcMessageType()) {
    case DC1_A64_SCATTERED_READ:
    case DC1_A64_SCATTERED_WRITE:
      return (getElemSize() == 1);
    default:
      break;
    }
    break;
  case SFID::DP_DC2:
    switch (getHdcMessageType()) {
    case DC2_A64_SCATTERED_READ:
    case DC2_A64_SCATTERED_WRITE:
      return (getElemSize() == 1);
    case DC2_BYTE_SCATTERED_READ:
    case DC2_BYTE_SCATTERED_WRITE:
      return true;
    default:
      break;
    }
    break;
  default:
    break;
  }
  return false;
}

bool G4_SendDescRaw::isDWScatterRW() const {
  auto funcID = getSFID();
  switch (funcID) {
  case SFID::DP_DC0:
    switch (getHdcMessageType()) {
    case DC_DWORD_SCATTERED_READ:
    case DC_DWORD_SCATTERED_WRITE:
      return true;
    default:
      break;
    }
    break;
  case SFID::DP_DC1:
    switch (getHdcMessageType()) {
    case DC1_A64_SCATTERED_READ:
    case DC1_A64_SCATTERED_WRITE:
      return (getElemSize() == 4);
    default:
      break;
    }
    break;
  case SFID::DP_DC2:
    switch (getHdcMessageType()) {
    case DC2_A64_SCATTERED_READ:
    case DC2_A64_SCATTERED_WRITE:
      return (getElemSize() == 4);
    default:
      break;
    }
    break;
  default:
    break;
  }
  return false;
}

bool G4_SendDescRaw::isQWScatterRW() const {
  auto funcID = getSFID();
  switch (funcID) {
  case SFID::DP_DC0:
    switch (getHdcMessageType()) {
    case DC_QWORD_SCATTERED_READ:
    case DC_QWORD_SCATTERED_WRITE:
      return true;
    default:
      break;
    }
    break;
  case SFID::DP_DC1:
    switch (getHdcMessageType()) {
    case DC1_A64_SCATTERED_READ:
    case DC1_A64_SCATTERED_WRITE:
      return (getElemSize() == 8);
    default:
      break;
    }
    break;
  case SFID::DP_DC2:
    switch (getHdcMessageType()) {
    case DC2_A64_SCATTERED_READ:
    case DC2_A64_SCATTERED_WRITE:
      return (getElemSize() == 8);
    default:
      break;
    }
    break;
  default:
    break;
  }
  return false;
}

bool G4_SendDescRaw::isUntypedRW() const {
  auto funcID = getSFID();
  switch (funcID) {
  case SFID::DP_DC1:
    switch (getHdcMessageType()) {
    case DC1_UNTYPED_SURFACE_READ:
    case DC1_UNTYPED_SURFACE_WRITE:
    case DC1_A64_UNTYPED_SURFACE_READ:
    case DC1_A64_UNTYPED_SURFACE_WRITE:
      return true;
    default:
      break;
    }
    break;
  case SFID::DP_DC2:
    switch (getHdcMessageType()) {
    case DC2_UNTYPED_SURFACE_READ:
    case DC2_UNTYPED_SURFACE_WRITE:
    case DC2_A64_UNTYPED_SURFACE_READ:
    case DC2_A64_UNTYPED_SURFACE_WRITE:
      return true;
    default:
      break;
    }
    break;
  default:
    break;
  }
  return false;
}

bool G4_SendDescRaw::isA64Message() const {
  if (!isHDC()) {
    return false;
  }

  uint32_t msgType = getHdcMessageType();
  auto funcID = getSFID();
  switch (funcID) {
  case SFID::DP_DC1: {
    switch (msgType) {
    default:
      break;
    case DC1_A64_SCATTERED_READ:
    case DC1_A64_UNTYPED_SURFACE_READ:
    case DC1_A64_ATOMIC:
    case DC1_A64_BLOCK_READ:
    case DC1_A64_BLOCK_WRITE:
    case DC1_A64_UNTYPED_SURFACE_WRITE:
    case DC1_A64_SCATTERED_WRITE:
    case DC1_A64_UNTYPED_FLOAT_ATOMIC:
    case DC1_A64_UNTYPED_HALF_INTEGER_ATOMIC:
    case DC1_A64_UNTYPED_HALF_FLOAT_ATOMIC:
      return true;
    }
    break;
  }
  case SFID::DP_DC2: {
    switch (msgType) {
    default:
      break;
    case DC2_A64_SCATTERED_READ:
    case DC2_A64_UNTYPED_SURFACE_READ:
    case DC2_A64_UNTYPED_SURFACE_WRITE:
    case DC2_A64_SCATTERED_WRITE:
      return true;
    }
    break;
  }
  default:
    break;
  }
  return false;
}

static int getNumEnabledChannels(uint32_t chDisableBits) {
  switch (chDisableBits) {
  case 0x7:
  case 0xB:
  case 0xD:
  case 0xE:
    return 1;
  case 0x3:
  case 0x5:
  case 0x6:
  case 0x9:
  case 0xA:
  case 0xC:
    return 2;
  case 0x1:
  case 0x2:
  case 0x4:
  case 0x8:
    return 3;
  case 0x0:
    return 4;
  case 0xF:
    return 0;
  default:
    vISA_ASSERT_UNREACHABLE("Illegal Channel Mask Number");
  }
  return 0;
}

#define MSG_BLOCK_SIZE_OFFSET 8
unsigned G4_SendDescRaw::getEnabledChannelNum() const {
  // TODO: should further scope this to typed/untyped
  vISA_ASSERT(isHDC(), "message does not have field ChannelEnable");
  uint32_t funcCtrl = getFuncCtrl();
  return getNumEnabledChannels((funcCtrl >> MSG_BLOCK_SIZE_OFFSET) & 0xF);
}

unsigned G4_SendDescRaw::getElemsPerAddr() const {
  if (isHDC()) {
    uint32_t funcCtrl = getFuncCtrl();

    const int MSG_BLOCK_NUMBER_OFFSET = 10;
    funcCtrl = (funcCtrl >> MSG_BLOCK_NUMBER_OFFSET) & 0x3;
    switch (funcCtrl) {
    case SVM_BLOCK_NUM_1:
      return 1;
    case SVM_BLOCK_NUM_2:
      return 2;
    case SVM_BLOCK_NUM_4:
      return 4;
    case SVM_BLOCK_NUM_8:
      return 8;
    default:
      vISA_ASSERT(false,
                   "Illegal SVM block number (should be 1, 2, 4, or 8).");
    }
  } else if (isLSC()) {
    auto op = getLscOp();
    switch (op) {
    case LSC_STORE_QUAD:
    case LSC_LOAD_QUAD: {
      int elems = 0;
      // bits [15:12] are the channel mask
      auto cmask = (getDesc() >> 12) & 0xF;
      for (int i = 0; i < 4; i++, cmask >>= 1) {
        elems += (cmask & 1);
      }
      break;
    }
    case LSC_LOAD:
    case LSC_LOAD_STRIDED:
    case LSC_STORE:
    case LSC_STORE_STRIDED:
      // bits [14:12] are the vector size
      switch ((getDesc() >> 12) & 0x7) {
      case 0:
        return 1;
      case 1:
        return 2;
      case 2:
        return 3;
      case 3:
        return 4;
      case 4:
        return 8;
      case 5:
        return 16;
      case 6:
        return 32;
      case 7:
        return 64;
      }
      break;
    case LSC_LOAD_BLOCK2D:
    case LSC_STORE_BLOCK2D:
      // unsupported
      return 0;
    default:
      if (op >= LSC_ATOMIC_IINC && op <= LSC_ATOMIC_XOR) {
        return 1; // atomics are always 1
      } else {
        return 0;
      }
    }
    return 1;
  } // TODO: others e.g. sampler
  return 0;
}

unsigned G4_SendDescRaw::getElemSize() const {
  if (isHDC()) {
    // FIXME: this should be checking for DC1 (SVM? only???)
    // Move HDC decode logic from Augmentation to here
    uint32_t funcCtrl = getFuncCtrl();

    funcCtrl = (funcCtrl >> MSG_BLOCK_SIZE_OFFSET) & 0x3;
    switch (funcCtrl) {
    case SVM_BLOCK_TYPE_BYTE:
      return 1;
    case SVM_BLOCK_TYPE_DWORD:
      return 4;
    case SVM_BLOCK_TYPE_QWORD:
      return 8;
    default:
      vISA_ASSERT_UNREACHABLE("Illegal SVM block size (should be 1, 4, or 8).");
    }
    return 0;
  } else if (isLSC()) {
    if (getSFID() == SFID::TGM)
      return 4; // typed always accesses 4B
    // UGM, SLM, or something else untyped
    auto op = getLscOp();
    switch (op) {
    case LSC_LOAD:
    case LSC_LOAD_STRIDED:
    case LSC_LOAD_QUAD:
    case LSC_LOAD_BLOCK2D:
    case LSC_STORE:
    case LSC_STORE_STRIDED:
    case LSC_STORE_QUAD:
    case LSC_STORE_BLOCK2D:
      break; // supported
    default:
      if (op < LSC_ATOMIC_IINC && op > LSC_ATOMIC_XOR) {
        vISA_ASSERT(
            false,
            "unexpected receiver (unsupported descriptor type) ==> fix this");
        return 0;
      } // else supported
    }
    // bits [11:9] are data size
    switch ((getDesc() >> 9) & 0x7) {
    case 0:
      return 1; // d8 (block2d only)
    case 1:
      return 2; // d16 (block2d only)
    case 3:
      return 8; // d64
    default:
      return 4; // d32, d8u32, ... all 32b in register file
    }
  } else if (getSFID() == SFID::SAMPLER) {
    return is16BitReturn() ? 2 : 4;
    // TODO: render target
    // TODO: other unsupported things like barrier and fence should just return
    // 0 without asserting?
  } else {
    vISA_ASSERT(
        false,
        "unexpected receiver (unsupported descriptor type) ==> fix this");
    return 0;
  }
}

bool G4_SendDescRaw::isOwordLoad() const {
  if (!isHDC() || !isValidFuncCtrl()) {
    return false;
  }
  uint32_t funcCtrl = getFuncCtrl();
  auto funcID = getSFID();
  static int DC0_MSG_TYPE_OFFSET = 14;
  static int DC1_MSG_SUBTYPE_OFFSET = 12; // [31:12]
  uint16_t msgType = (funcCtrl >> DC0_MSG_TYPE_OFFSET) & 0x1F;
  uint16_t dc1MsgSubType = (funcCtrl >> DC1_MSG_SUBTYPE_OFFSET) & 0x3;
  // bits [18:14] are message type
  // (included 18 because that is set for scratch)
  static const uint32_t MSD0R_OWAB = 0x0;      // DC0
  static const uint32_t MSD0R_OWB = 0x0;       // DC0
  static const uint32_t MSD_CC_OWAB = 0x1;     // DC_CC
  static const uint32_t MSD_CC_OWB = 0x0;      // DC_CC
  static const uint32_t MSD1R_A64_OWB = 0x14;  // DC1 A64 [13:12] == 1
  [[maybe_unused]]
  static const uint32_t MSD1R_A64_OWAB = 0x14; // DC1 A64 [13:12] == 0
  bool isDc0Owb =
      funcID == SFID::DP_DC0 && (msgType == MSD0R_OWAB || msgType == MSD0R_OWB);
  bool isCcOwb = funcID == SFID::DP_CC &&
                 (msgType == MSD_CC_OWAB || msgType == MSD_CC_OWB);
  bool isDc1A64Owb = funcID == SFID::DP_DC1 && (msgType == MSD1R_A64_OWB) &&
                     // st==2, 3 don't have mappings that I can find, but just
                     // to be safe force 0 or 1 (which are unalgined vs aligned)
                     (dc1MsgSubType == 0 || dc1MsgSubType == 1);
  return isDc0Owb || isCcOwb || isDc1A64Owb;
}

unsigned G4_SendDescRaw::getOwordsAccessed() const {
  vISA_ASSERT(isOwordLoad(), "must be OWord message");
  // This encoding holds for the DP_DC0, DP_CC, and DP_DC1 (A64 block)
  // element count.
  auto owEnc = (getFuncCtrl() >> 8) & 0x7; // Desc[10:8] is OW count
  if (owEnc == 0) {
    return 1; // OW1L (low half of GRF)
  } else if (owEnc == 1) {
    // for OW1H (high half of GRF): treat as full 32B
    // (this control probably isn't ever be used and was removed in Xe)
    return 1;
  } else {
    // 2 = OW2, 3 == OW4, 4 == OW8, 5 == OW16
    return 2 << (owEnc - 2);
  }
}

bool G4_SendDescRaw::isHdcTypedSurfaceWrite() const {
  return isHDC() && getHdcMessageType() == DC1_TYPED_SURFACE_WRITE;
}

std::string G4_SendDescRaw::getDescription() const {
  // Return plain text string of type of msg, ie "oword read", "oword write",
  // "media rd", etc.
  const G4_SendDescRaw *msgDesc = this;
  unsigned int category;

  switch (msgDesc->getSFID()) {
  case SFID::SAMPLER:
    return "sampler";
  case SFID::GATEWAY:
    return "gateway";
  case SFID::DP_DC2:
    switch (getHdcMessageType()) {
    case DC2_UNTYPED_SURFACE_READ:
      return "scaled untyped surface read";
    case DC2_A64_SCATTERED_READ:
      return "scaled A64 scatter read";
    case DC2_A64_UNTYPED_SURFACE_READ:
      return "scaled A64 untyped surface read";
    case DC2_BYTE_SCATTERED_READ:
      return "scaled byte scattered read";
    case DC2_UNTYPED_SURFACE_WRITE:
      return "scaled untyped surface write";
    case DC2_A64_UNTYPED_SURFACE_WRITE:
      return "scaled A64 untyped surface write";
    case DC2_A64_SCATTERED_WRITE:
      return "scaled A64 scattered write";
    case DC2_BYTE_SCATTERED_WRITE:
      return "scaled byte scattede write";
    default:
      return "unrecognized DC2 message";
    }
  case SFID::DP_RC:
    switch ((getFuncCtrl() >> 14) & 0x1F) {
    case 0xc:
      return "render target write";
    case 0xd:
      return "render target read";
    default:
      return "unrecognized RT message";
    }
    break;
  case SFID::URB:
    return "urb";
  case SFID::SPAWNER:
    return "thread spawner";
  case SFID::VME:
    return "vme";
  case SFID::DP_CC:
    switch (getHdcMessageType()) {
    case 0x0:
      return "oword block read";
    case 0x1:
      return "unaligned oword block read";
    case 0x2:
      return "oword dual block read";
    case 0x3:
      return "dword scattered read";
    default:
      return "unrecognized DCC message";
    }
  case SFID::DP_DC0:
    category = (msgDesc->getFuncCtrl() >> 18) & 0x1;
    if (category == 0) {
      // legacy data port
      bool hword = (msgDesc->getFuncCtrl() >> 13) & 0x1;
      switch (getHdcMessageType()) {
      case 0x0:
        return hword ? "hword block read" : "oword block read";
      case 0x1:
        return hword ? "hword aligned block read"
                     : "unaligned oword block read";
      case 0x2:
        return "oword dual block read";
      case 0x3:
        return "dword scattered read";
      case 0x4:
        return "byte scattered read";
      case 0x7:
        return "memory fence";
      case 0x8:
        return hword ? "hword block write" : "oword block write";
      case 0x9:
        return "hword aligned block write";
      case 0xa:
        return "oword dual block write";
      case 0xb:
        return "dword scattered write";
      case 0xc:
        return "byte scattered write";
      case 0x5:
        return "qword gather";
      case 0xd:
        return "qword scatter";
      default:
        return "unrecognized DC0 message";
      }
    } else {
      // scratch
      int bits = (msgDesc->getFuncCtrl() >> 17) & 0x1;

      if (bits == 0)
        return "scratch read";
      else
        return "scratch write";
    }
    break;
  case SFID::DP_PI:
    return "dp_pi";
  case SFID::DP_DC1:
    switch (getHdcMessageType()) {
    case 0x0:
      return "transpose read";
    case 0x1:
      return "untyped surface read";
    case 0x2:
      return "untyped atomic operation";
    case 0x3:
      return "untyped atomic operation simd4x2";
    case 0x4:
      return "media block read";
    case 0x5:
      return "typed surface read";
    case 0x6:
      return "typed atomic operation";
    case 0x7:
      return "typed atomic operation simd4x2";
    case 0x8:
      return "untyped atomic float add";
    case 0x9:
      return "untyped surface write";
    case 0xa:
      return "media block write (non-iecp)";
    case 0xb:
      return "atomic counter operation";
    case 0xc:
      return "atomic counter operation simd4x2";
    case 0xd:
      return "typed surface write";
    case 0x10:
      return "a64 gathering read";
    case 0x11:
      return "a64 untyped surface read";
    case 0x12:
      return "a64 untyped atomic operation";
    case 0x13:
      return "a64 untyped atomic operation simd4x2";
    case 0x14:
      return "a64 block read";
    case 0x15:
      return "a64 block write";
    case 0x18:
      return "a64 untyped atomic float add";
    case 0x19:
      return "a64 untyped surface write";
    case 0x1a:
      return "a64 scattered write";
    default:
      return "unrecognized DC1 message";
    }
    break;
  case SFID::CRE:
    return "cre";
  case SFID::SLM:
  case SFID::TGM:
  case SFID::UGM:
  case SFID::UGML: {
    LscOpInfo opInfo{};
    if (LscOpInfoFind((LSC_OP)(desc.value & 0x3F), opInfo)) { // Desc[5:0]
      std::stringstream ss;
      if (opInfo.isLoad() || opInfo.isStore() || opInfo.isAtomic()) {
        std::string sop = opInfo.mnemonic; // lsc_load
        if (sop.substr(0, 4) == "lsc_")
          sop = sop.substr(4); // lsc_load => load
        ss << sop << "." << ToSymbol(sfid); // leave out .ugm or .slm
        if (opInfo.hasChMask()) {
          auto cmask = (int)getBitField(desc.value, 12, 4);
          ss << ".";
          for (int i = 0; i < 4;i ++)
            if (cmask & (1 << i))
              ss << "xyzw"[i];
        } else {
          auto dsz = getBitField(desc.value, 9, 3);
          switch (dsz) {
          case 0: ss << ".d8"; break;
          case 1: ss << ".d16"; break;
          case 2: ss << ".d32"; break;
          case 3: ss << ".d64"; break;
          case 4: ss << ".d8u32"; break;
          case 5: ss << ".d16u32"; break;
          default: ss << ".d??"; break;
          }
          auto vec = getBitField(desc.value, 12, 3);
          switch (vec) {
          case 0: break;
          case 1: ss << "x2"; break;
          case 2: ss << "x3"; break;
          case 3: ss << "x4"; break;
          case 4: ss << "x8"; break;
          case 5: ss << "x16"; break;
          case 6: ss << "x32"; break;
          case 7: ss << "x64"; break;
          default: ss << "x?"; break;
          }
          if (getBitField(desc.value, 15, 1))
            ss << "t";
        }
        bool hasImpliedA32 = false, hasImpliedA64 = false;

        hasImpliedA32 =
            opInfo.isApndCtrAtomic() ||
            (opInfo.isBlock2D() && sfid == SFID::TGM);
        hasImpliedA32 =
            (opInfo.isBlock2D() && sfid == SFID::UGM);

        if (hasImpliedA32) {
          ss << ".a32";
        } else if (hasImpliedA64) {
          ss << ".a64";
        } else {
          switch (getBitField(desc.value, 7, 2)) {
          case 2: ss << ".a32"; break;
          case 3: ss << ".a64"; break;
          default: ss << ".a??"; break;
          // certain messages have hardcoded or implied address sizes, and
          // this will report .a?? for those, but good enough for internal debug
          // for now
          }
        }
        auto [l1,l3] = getCaching();
        ss << ToSymbol(l1, l3);

        switch (getBitField(desc.value, 29, 2)) {
        case 0: ss << " flat[A"; break;
        case 1: ss << " bss[..][A"; break;
        case 2: ss << " ss[..][A"; break;
        case 3: ss << " bti[..][A"; break;
        default: ss << " ???[A"; break;
        }
        if (opInfo.isBlock2D()) {
          uint32_t bits = getExtendedDesc();
          int immOffX = (int)(bits << 10) >> (12 + 10);
          int immOffY = (int)(bits      ) >> (12 + 10);
          ss << "+(" <<
            fmtSignedHex(immOffX) << "," <<
            fmtSignedHex(immOffY) << ")";
        } else if (auto immOff = getOffset()) {
          ss << fmtSignedHexTerm(immOff->immOff);
        }
        ss << "]";
      }
      return ss.str();
    } else {
      const char *invalid = "lsc (invalid operation)";
      return invalid;
    }
  }
  default:
    return "--";
  }
  return NULL;
}

size_t G4_SendDescRaw::getSrc0LenBytes() const {
  return MessageLength() * (size_t)irb.getGRFSize();
}

uint32_t G4_SendDescRaw::getDataSizeInBytesLscLdStInst(
    Gen4_Operand_Number opnd_num) const {
  vISA_ASSERT(opnd_num == Opnd_dst || opnd_num == Opnd_src1,
              "expect Opnd_dst or Opnd_src1");
  uint32_t dataBytes = opnd_num == Opnd_dst
                           ? (ResponseLength() * irb.getGRFSize())
                           : (src1Len * irb.getGRFSize());
  if (getLscDataOrder() == LSC_DATA_ORDER_NONTRANSPOSE) {
    // Non-transpose
    // If vecSize > 1, make the data size GRF-aligned for simplicity.
    // Otherwise, the data size is the exact bytes accessed by HW.
    if (getElemsPerAddr() <= 1)
      dataBytes = execSize * getElemSize();
  } else {
    // Transpose
    dataBytes = getElemsPerAddr() * getElemSize();
  }
  return dataBytes;
}

size_t G4_SendDescRaw::getDstLenBytes() const {
  uint32_t dstBytes = ResponseLength() * irb.getGRFSize();
  if (isHWordScratchRW() && ResponseLength() != 0) {
    dstBytes = 32 * getHWScratchRWSize(); // HWords
  } else if (isOwordLoad()) {
    dstBytes = 16 * getOwordsAccessed(); // OWords
  } else if (isLscDescriptor) {
    // LSC messages
    auto op = getLscOp();
    switch (op) {
    case LSC_OP::LSC_LOAD:
      if (ResponseLength() != 0)
        dstBytes = getDataSizeInBytesLscLdStInst(Opnd_dst);
      break;
    // TODO: handle other LSC op codes
    default:
      break;
    }
  }
  return dstBytes;
}

size_t G4_SendDescRaw::getSrc1LenBytes() const {
  if (isLscDescriptor) {
    uint32_t src1LenBytes = src1Len * irb.getGRFSize();
    auto op = getLscOp();
    switch (op) {
    case LSC_OP::LSC_STORE:
      src1LenBytes = getDataSizeInBytesLscLdStInst(Opnd_src1);
      break;
    // TODO: handle other LSC op codes
    default:
      // use the default value
      break;
    }
    return src1LenBytes;
  }

  if (isHWordScratchRW() && extMessageLength() != 0) {
    return 32 * getHWScratchRWSize(); // HWords
  }
  // we could support OW store here, but no one seems to need that and
  // we are phasing this class out; so ignore it for now

  return extMessageLength() * (size_t)irb.getGRFSize();
}

size_t G4_SendDescRaw::getSrc1LenRegs() const {
  if (isLscDescriptor)
    return src1Len;
  else
    return extMessageLength();
}

bool G4_SendDescRaw::isFence() const {
  if (isLscOp())
    return (desc.value & 0x3F) == LSC_FENCE;

  SFID sfid = getSFID();
  unsigned FC = getFuncCtrl();

  // Memory Fence
  if (sfid == SFID::DP_DC0 && ((FC >> 14) & 0x1F) == DC_MEMORY_FENCE) {
    return true;
  }

  // Sampler cache flush
  if (sfid == SFID::SAMPLER && ((FC >> 12) & 0x1F) == 0x1F) {
    return true;
  }

  return false;
}
bool G4_SendDescRaw::isBarrier() const {
  auto funcID = getSFID();
  uint32_t funcCtrl = getFuncCtrl();
  return funcID == SFID::GATEWAY && (funcCtrl & 0xFF) == 0x4;
}

bool G4_SendDescRaw::isBTS() const {
  if (isLscOp()) {
    switch (getLscAddrType()) {
    case LSC_ADDR_TYPE_BSS:
    case LSC_ADDR_TYPE_SS:
    case LSC_ADDR_TYPE_BTI:
      return true;
    default:
      break;
    }
  } else {
    const G4_Operand *BTI = getBti();
    uint32_t BTIImm = 0;
    if (BTI && !BTI->isImm()) {
      // Desc in reg, should be stateful.
      return true;
    } else if (BTI) {
      BTIImm = (BTI->asImm()->getInt() & 0xFF);
    } else {
      BTIImm = (getDesc() & 0xFF);
    }
    constexpr uint32_t BSS_BTI = 252;
    constexpr uint32_t BTI_MAX = 240;
    if (BTIImm <= BTI_MAX || BTIImm == BSS_BTI)
      return true;
  }
  return false;
}

std::optional<ImmOff> G4_SendDescRaw::getOffset() const {
  if (isLscOp()) {
    // technically unavailable until XE2, but this is binary compatible,
    // so just always decode it
    int signedOff = 0;
    if (uint32_t immOffBits = getExDescImmOff()) {
      // offset is stored separately for BSS/SS since it overlaps other
      // ExDesc bits ExDescImm[31:18][15:12]
      uint32_t packed =
          (immOffBits & 0xFFFC0000) | ((immOffBits & 0xF000) << 3);
      signedOff = (int)packed >> (12 + 3);
    } else {
      // offset is stowed in ExDesc
      const int LSC_ADDR_TYPE_OFFSET = 29;
      const uint32_t LSC_ADDR_TYPE_MASK = 0x3;
      const uint32_t addrType =
          (getDesc() >> LSC_ADDR_TYPE_OFFSET) & LSC_ADDR_TYPE_MASK;
      const auto exDescBits = getExtendedDesc();
      const uint32_t LSC_ADDR_TYPE_BTI = 3;
      const uint32_t LSC_ADDR_TYPE_FLAT = 0;
      if (addrType == LSC_ADDR_TYPE_BTI) {
        signedOff = ((int)exDescBits << 8) >> (8 + 12);
      } else if (addrType == LSC_ADDR_TYPE_FLAT) {
        signedOff = (int)exDescBits >> 12;
      }
    }
    return ImmOff(signedOff);
  } else if (isHWordScratchRW()) {
    // HWord scratch message
    return ImmOff(getHWordScratchRWOffset() * 32);
  }
  return std::nullopt;
}

[[maybe_unused]]
static Caching cachingToG4(LSC_CACHE_OPT co) {
  switch (co) {
  case LSC_CACHING_DEFAULT:
    return Caching::DF;
  case LSC_CACHING_CACHED:
    return Caching::CA;
  case LSC_CACHING_READINVALIDATE:
    return Caching::RI;
  case LSC_CACHING_WRITEBACK:
    return Caching::WB;
  case LSC_CACHING_UNCACHED:
    return Caching::UC;
  case LSC_CACHING_STREAMING:
    return Caching::ST;
  case LSC_CACHING_WRITETHROUGH:
    return Caching::WT;
  case LSC_CACHING_CONSTCACHED:
    return Caching::CC;
  default:
    break;
  }
  return Caching::INVALID;
}

// decode caching from Desc[19:17]
static std::pair<Caching, Caching> decodeCaching3(bool isLoad,
                                                  uint32_t descBits) {
  auto mk = [&](Caching l1IfLd, Caching l3IfLd, Caching l1IfStAt,
                Caching l3IfStAt) {
    return isLoad ? std::make_pair(l1IfLd, l3IfLd)
                  : std::make_pair(l1IfStAt, l3IfStAt);
  };

  // Decode caching field from in [19:17]
  uint32_t ccBits = (descBits >> 17) & 0x7;
  switch (ccBits) {
  case 0:
    return mk(Caching::DF, Caching::DF, Caching::DF, Caching::DF);
  case 1:
    return mk(Caching::UC, Caching::UC, Caching::UC, Caching::UC);
  case 2:
    return mk(Caching::UC, Caching::CA, Caching::UC, Caching::WB);
  case 3:
    return mk(Caching::CA, Caching::UC, Caching::WT, Caching::UC);
  case 4:
    return mk(Caching::CA, Caching::CA, Caching::WT, Caching::WB);
  case 5:
    return mk(Caching::ST, Caching::UC, Caching::ST, Caching::UC);
  case 6:
    return mk(Caching::ST, Caching::CA, Caching::ST, Caching::WB);
  case 7:
    return mk(Caching::RI, Caching::CA, Caching::WB, Caching::WB);
  }
  return std::make_pair(Caching::INVALID, Caching::INVALID);
}

// decode caching from Desc[19:16] (4 bit field: XE2+)
static std::pair<Caching, Caching> decodeCaching4(bool isLoad,
                                                  uint32_t descBits) {
  auto mk = [&](Caching l1IfLd, Caching l3IfLd, Caching l1IfStAt,
                Caching l3IfStAt) {
    return isLoad ? std::make_pair(l1IfLd, l3IfLd)
                  : std::make_pair(l1IfStAt, l3IfStAt);
  };

  // Decode caching field from in [19:16]
  uint32_t ccBits = (descBits >> 16) & 0xF;
  switch (ccBits) {
  case 0:
    return mk(Caching::DF, Caching::DF, Caching::DF, Caching::DF);
  case 2:
    return mk(Caching::UC, Caching::UC, Caching::UC, Caching::UC);
  case 4:
    return mk(Caching::UC, Caching::CA, Caching::UC, Caching::WB);
  case 5:
    return mk( // new entry for L3 constant cache
        Caching::UC, Caching::CC, Caching::INVALID, Caching::INVALID);
  case 6:
    return mk(Caching::CA, Caching::UC, Caching::WT, Caching::UC);
  case 8:
    return mk(Caching::CA, Caching::CA, Caching::WT, Caching::WB);
  case 9:
    return mk( // new entry for L3 constant cache
        Caching::CA, Caching::CC, Caching::INVALID, Caching::INVALID);
  case 10:
    return mk(Caching::ST, Caching::UC, Caching::ST, Caching::UC);
  case 12:
    return mk(Caching::ST, Caching::CA, Caching::ST, Caching::WB);
  case 14:
    return mk(Caching::RI, Caching::RI, Caching::WB, Caching::WB);
  }
  return std::make_pair(Caching::INVALID, Caching::INVALID);
}

std::pair<Caching, Caching> G4_SendDescRaw::getCaching() const {
  if (!isLscOp()) {
    return std::make_pair(Caching::INVALID, Caching::INVALID);
  }
  const auto opInfo = LscOpInfoGet(getLscOp());
  if (opInfo.isOther()) {
    return std::make_pair(Caching::INVALID, Caching::INVALID);
  }

  auto ccPair = irb.getPlatform() < Xe2
                    ? decodeCaching3(opInfo.isLoad(), getDesc())
                    : decodeCaching4(opInfo.isLoad(), getDesc());
  vISA_ASSERT(ccPair.first != Caching::INVALID &&
                  ccPair.second != Caching::INVALID,
              "unexpected invalid caching options (corrupt descriptor?)");
  return ccPair;
}

static LSC_CACHE_OPT toVisaCachingOpt(Caching c) {
  switch (c) {
  case Caching::DF:
    return LSC_CACHING_DEFAULT;
  case Caching::UC:
    return LSC_CACHING_UNCACHED;
  case Caching::CA:
    return LSC_CACHING_CACHED;
  case Caching::WB:
    return LSC_CACHING_WRITEBACK;
  case Caching::WT:
    return LSC_CACHING_WRITETHROUGH;
  case Caching::ST:
    return LSC_CACHING_STREAMING;
  case Caching::RI:
    return LSC_CACHING_READINVALIDATE;
  case Caching::CC:
    return LSC_CACHING_CONSTCACHED;
  default:
    vISA_ASSERT_UNREACHABLE("invalid cache option");
    return (LSC_CACHE_OPT)-1;
  }
}

void G4_SendDescRaw::setCaching(Caching l1, Caching l3) {
  if (!isLscOp()) {
    vISA_ASSERT((l1 == Caching::INVALID && l3 == Caching::INVALID) ||
                     (l1 == Caching::DF && l3 == Caching::DF),
                 "invalid caching options for platform*SFID");
  }
  const auto opInfo = LscOpInfoGet(getLscOp());
  vISA_ASSERT(!opInfo.isOther(), "invalid LSC message kind for caching op");
  LSC_CACHE_OPTS visaCopts{};
  visaCopts.l1 = toVisaCachingOpt(l1);
  visaCopts.l3 = toVisaCachingOpt(l3);

  uint32_t cacheEnc = 0;
  uint32_t fieldMask = (0x7 << 17);
  bool isBits17_19 = true;
  isBits17_19 = (irb.getPlatform() < Xe2);
  fieldMask = isBits17_19 ? (0x7 << 17) : (0xF << 16);
  [[maybe_unused]] bool success =
      LscTryEncodeCacheOpts(opInfo, visaCopts, cacheEnc, isBits17_19);
  vISA_ASSERT(success, "failed to set caching options");
  desc.value &= ~fieldMask;
  desc.value |= cacheEnc;
}

static bool isDc1OpTyped(uint32_t desc) {
  uint32_t mty = (desc >> 14) & 0x1F;
  switch (mty) {
  case DC1_TYPED_SURFACE_WRITE:
  case DC1_TYPED_SURFACE_READ:
  case DC1_TYPED_ATOMIC:
  case DC1_TYPED_HALF_INTEGER_ATOMIC:
    return true;
  default:
    break;
  }
  return false;
}

bool G4_SendDescRaw::isTyped() const {
  return getSFID() == SFID::DP_DC1 && isDc1OpTyped(getDesc());
}
