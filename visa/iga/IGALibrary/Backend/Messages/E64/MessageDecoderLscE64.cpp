/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "../../../bits.hpp"
#include "MessageDecoderE64.hpp"

#include <tuple>

using namespace iga;

#define LSC_LOOKUP_TABLE(FIELD, OFF, LEN, TBL)                                 \
  lookupFromTable(FIELD, OFF, LEN, TBL, sizeof(TBL) / sizeof(TBL[0]))

///////////////////////////
struct LscOpDesc : SimpleValidValue<SendOp> {
  const char *type, *link;
  constexpr LscOpDesc(uint32_t enc, SendOp val,
                      const char *desc, const char *syn, const char *t,
                      const char *l)
      : SimpleValidValue<SendOp>(enc, val, desc, syn), type(t), link(l) {}
};
static constexpr LscOpDesc LSC_DP_OPCODE[]{
    LscOpDesc{0x00, SendOp::LOAD, "load", "load vector",
              "DP_LOAD", "71918"},
    LscOpDesc{0x02, SendOp::LOAD_QUAD, "load_quad", "load quad (load_cmask)",
              "DP_LOAD_CMASK", "71921"},
    LscOpDesc{0x03, SendOp::LOAD_BLOCK2D, "load_block2d", "load 2d block array",
              "DP_LOAD_2DBLOCK_ARRAY", "71920"},
    //
    LscOpDesc{0x04, SendOp::STORE, "store", "store vector",
              "DP_STORE", "71922"},
    LscOpDesc{0x06, SendOp::STORE_QUAD, "store_quad",
              "store quad (store_cmask)",
              "DP_STORE_CMASK", "71925"},
    LscOpDesc{0x07, SendOp::STORE_BLOCK2D, "store_block2d", "store 2d block",
              "DP_STORE_2DBLOCK", "71924"},
    //
    LscOpDesc{0x08, SendOp::ATOMIC_IINC, "atomic_iinc",
              "atomic integer increment",
              "DP_ATOMIC_INC", "71907"},
    LscOpDesc{0x09, SendOp::ATOMIC_IDEC, "atomic_idec",
              "atomic integer decrement",
              "DP_ATOMIC_DEC", "71901"},
    LscOpDesc{0x0A, SendOp::ATOMIC_LOAD, "atomic_load", "atomic load",
              "DP_ATOMIC_LOAD", "71908"},
    LscOpDesc{0x0B, SendOp::ATOMIC_STORE, "atomic_store",
              "atomic store (exchange)",
              "DP_STORE", "71922"},
    LscOpDesc{0x0C, SendOp::ATOMIC_IADD, "atomic_iadd", "atomic integer add",
              "DP_ATOMIC_ADD", "71898"},
    LscOpDesc{0x0D, SendOp::ATOMIC_ISUB, "atomic_isub",
              "atomic integer subtract",
              "DP_ATOMIC_SUB", "71913"},
    LscOpDesc{0x0E, SendOp::ATOMIC_SMIN, "atomic_smin",
              "atomic signed integer minimum",
              "DP_ATOMIC_MIN", "71910"},
    LscOpDesc{0x0F, SendOp::ATOMIC_SMAX, "atomic_smax",
              "atomic signed integer maximum",
              "DP_ATOMIC_MAX", "71909"},
    LscOpDesc{0x10, SendOp::ATOMIC_UMIN, "atomic_umin",
              "atomic unsigned integer minimum",
              "DP_ATOMIC_UMIN", "71915"},
    LscOpDesc{0x11, SendOp::ATOMIC_UMAX, "atomic_umax",
              "atomic unsigned integer maximum",
              "DP_ATOMIC_UMAX", "71914"},
    LscOpDesc{0x12, SendOp::ATOMIC_ICAS, "atomic_icas",
              "atomic integer compare and swap",
              "DP_ATOMIC_CMPXCHG", "71900"},
    LscOpDesc{0x13, SendOp::ATOMIC_FADD, "atomic_fadd", "atomic float add",
              "DP_ATOMIC_FADD", "71902"},
    LscOpDesc{0x14, SendOp::ATOMIC_FSUB, "atomic_fsub",
              "atomic float subtract",
              "DP_ATOMIC_FSUB","71906"},
    LscOpDesc{0x15, SendOp::ATOMIC_FMIN, "atomic_fmin", "atomic float min",
              "DP_ATOMIC_FMIN", "71905"},
    LscOpDesc{0x16, SendOp::ATOMIC_FMAX, "atomic_fmax", "atomic float max",
              "DP_ATOMIC_FMAX", "71904"},
    LscOpDesc{0x17, SendOp::ATOMIC_FCAS, "atomic_fcas",
              "atomic float compare and swap",
              "DP_ATOMIC_FCMPXCHG", "71903"},
    LscOpDesc{0x18, SendOp::ATOMIC_AND, "atomic_and", "atomic and",
              "DP_ATOMIC_AND", "71899"},
    LscOpDesc{0x19, SendOp::ATOMIC_OR, "atomic_or", "atomic or",
              "DP_ATOMIC_OR", "71911"},
    LscOpDesc{0x1A, SendOp::ATOMIC_XOR, "atomic_xor", "atomic xor",
              "DP_ATOMIC_XOR", "71916"},
    LscOpDesc{0x1B, SendOp::LOAD_STATUS, "load_status", "load status",
             "DP_LOAD_STATUS", "71935"},
    LscOpDesc{0x1C, SendOp::LOAD_QUAD_STATUS, "load_quad_status",
              "load quad status",
              "DP_LOAD_CMASK_STATUS", "71950"},
    // 0x1D CCS update is removed
    LscOpDesc{0x1E, SendOp::READ_STATE, "read_state",
              "read surface state info",
              "DP_RSI", "71937"},
    LscOpDesc{0x1F, SendOp::FENCE, "fence", "memory fence",
              "DP_FENCE", "71917"},
    // 0x20 STORE_UNCOMPRESSED_CMASK [REMOVED]
    //

    LscOpDesc{0x28, SendOp::ATOMIC_ACADD, "atomic_acadd",
              "atomic append counter add",
              "DP_APPENDCTR_ATOMIC_ADD", "71891"},
    LscOpDesc{0x29, SendOp::ATOMIC_ACSUB, "atomic_acsub",
              "atomic append counter subtract",
              "DP_APPENDCTR_ATOMIC_SUB", "71897"},
    LscOpDesc{0x2A, SendOp::ATOMIC_ACSTORE, "atomic_acstore",
              "atomic append counter store",
              "DP_APPENDCTR_ATOMIC_ST", "71896"},

    LscOpDesc{0x2D, SendOp::GET_WATCHPOINT, "get_watchpoint", "get watchpoint",
              "DP_GET_WATCHPOINT", "71942"},
    LscOpDesc{0x2E, SendOp::SET_WATCHPOINT, "set_watchpoint", "set watchpoint",
              "DP_SET_WATCHPOINT", "71943"},
    //
    // LscOpDesc{0x2F, SendOp::LOAD_ELEMENT, "load_element", "load element",
    //           "DP_LOAD_ELEMENT", "72107"},
    // LscOpDesc{0x30, SendOp::LOAD_ELEMENT_WITH_STATUS,
    //          "load_element_with_status", "load element with a status value",
    //          "DP_LOAD_ELEMENT_STATUS", "72155"},

    LscOpDesc{0x31, SendOp::LOAD_QUAD_MSRT, "load_quad_msrt",
              "load quad from multi-sample render target",
              "DP_LOAD_CMASK_MSRT", "73419"},
    LscOpDesc{0x32, SendOp::STORE_QUAD_MSRT, "store_quad_msrt",
              "store quad to multi-sample render target",
              "DP_STORE_CMASK_MSRT", "73420"},
    // TODO: BSPEC does not have a dedicated page for extended cache control at
    // the moment
    LscOpDesc{0x33, SendOp::EXTENDED_CACHE_CTRL, "extended_cache_control",
              "extended cache control",
              "DP_CCTRL", ""},
};

using LscVecSize = SimpleValidValue<int>;
static const LscVecSize LSC_VEC_SIZE[]{
    LscVecSize{0x0, 1, "x1", "access 1 element per address"},
    LscVecSize{0x1, 2, "x2", "access 2 elements per address"},
    LscVecSize{0x2, 3, "x3", "access 3 element per address"},
    LscVecSize{0x3, 4, "x4", "access 4 element per address"},
    LscVecSize{0x4, 8, "x8", "access 8 element per address"},
    LscVecSize{0x5, 16, "x16", "access 16 element per address"},
    LscVecSize{0x6, 32, "x32", "access 32 element per address"},
    LscVecSize{0x7, 64, "x64", "access 64 element per address"}};

static const LscDataSizeValidValue LSC_DATA_SIZE[]{
    LscDataSizeValidValue{0x0, 8, 8, ".d8", "8b"},
    LscDataSizeValidValue{0x1, 16, 16, ".d16", "16b"},
    LscDataSizeValidValue{0x2, 32, 32, ".d32", "32b"},
    LscDataSizeValidValue{0x3, 64, 64, ".d64", "64b"},
    LscDataSizeValidValue{0x4, 8, 32, ".d8u32",
                          "8b zero-extended/truncated to 32b in GRF"},
    LscDataSizeValidValue{0x5, 16, 32, ".d16u32",
                          "16b zero-extended/truncated to 32b in GRF"}};

///////////////////////////////////////////////////////////////////////////////
// This handles LSC messages only
struct MessageDecoderLscE64 : MessageDecoderE64 {
  MessageDecoderLscE64(Platform _platform, SFID _sfid, ExecSize _execSize,
                       int src0Len, int src1Len, uint64_t _desc, RegRef id0,
                       RegRef id1, DecodeResult &_result)
      : MessageDecoderE64(_platform, _sfid, _execSize, src0Len, src1Len,
                          _desc, id0, id1, _result) {}

  void decode();

private:
  // enables us to decode stuff in a more flexible order
  // and then we can blindly assembly the syntax at the end
  std::string dataTypePrefixSyntax; // e.g. .d32
  std::string vectorSuffixSyntax;   // e.g. x16t (for d16x16t) or .yzw
  std::string addrSizeSyntax;       // e.g. .a32
  std::string cacheControlSyntax;   // e.g. .ca.ca.ca
  std::string ovfSyntax;            // e.g. .ovf
  std::string extCacheControlSizeSyntax; // e.g. .64B
  std::string extCacheControlOpSyntax; // e.g .set, .reset

  SendOp op = SendOp::INVALID;

  /////////////////////////////////////////////////////////////////////////////
  void decodeUntyped(const SendOpDefinition &sOpInfo);

  void addImplicitAddrTypeSURF();
  void addDocsForAddrPayloadsUntypedLoadStoreAtomic();

  void decodeDP_LOAD_STORE_STATELESS_DESC(const SendOpDefinition &sOpInfo);
  void decodeDP_LOAD_STORE_STATEFUL_DESC(const SendOpDefinition &sOpInfo);

  void decodeDP_ATOMIC_STATELESS_DESC(const SendOpDefinition &sOpInfo);
  void decodeDP_ATOMIC_STATEFUL_DESC(const SendOpDefinition &sOpInfo);

  void decodeDP_CMASK_STATELESS_DESC(const SendOpDefinition &sOpInfo);
  void decodeDP_CMASK_STATEFUL_DESC(const SendOpDefinition &sOpInfo);

  void decodeDP_2DBLOCK_DESC(const SendOpDefinition &sOpInfo);

  void decodeDP_FENCE_DESC(const SendOpDefinition &sOpInfo);

  void decodeTyped(const SendOpDefinition &sOpInfo);
  void decodeDP_TYPED_CMASK_DESC(const SendOpDefinition &sOpInfo);
  void decodeDP_TYPED_ATOMIC_DESC(const SendOpDefinition &sOpInfo);
  void decodeDP_RSI_DESC(const SendOpDefinition &sOpInfo);

  void decodeDP_WATCHPOINT_DESC();
  void decodeDP_EXTENDED_CACHE_CTRL_DESC(const SendOpDefinition &sOpInfo);
  ////////////////////////////////////
  // helpers
  void computePayloadSizesLoadStoreAtomic(const SendOpDefinition &sOpInfo);

  void decodeAddrTypeSize();
  void decodeAddrTypeUGM();
  void decodeAddrTypeSLM();
  void decodeAddrTypeURB();
  void decodeAddrTypeSTATEFUL();
  //
  void decodeAddrScaling();
  //
  void decodeDataSize();
  void decodeDataSizeWithRefs(const SendOpDefinition &sOpInfo);
  void decodeVectorSize();
  std::string decodeVectorSizeElems();
  const LscDataSizeValidValue *decodeDataSizeValue();

  //
  void decodeCacheControl(SendOp sop);
  void decodeOverfetch();
  //
  void decodeImmOffAddr(int off, int len);
  void decodeImmOffCoord2D();
  //
  // the symbol to return in the MessageInfo structure
  std::string symbolFromSyntax() const;

  void decodeCacheControlSize();
  void decodeCacheControlOperation();
}; // MessageDecoderLscE64

void MessageDecoderLscE64::decode() {
  [[maybe_unused]] auto isValidSFID = [](SFID sfid) {
    if (sfid == SFID::UGM || sfid == SFID::TGM || sfid == SFID::SLM ||
        sfid == SFID::URB)
      return true;
    return false;
  };
  IGA_ASSERT(isValidSFID(sfid), "unexpected SFID");

  const LscOpDesc *opDesc = LSC_LOOKUP_TABLE("SendOp", 0, 6, LSC_DP_OPCODE);
  addReserved(6, 1);
  if (opDesc == nullptr)
    return;

  if (opDesc->type || opDesc->link)
    addDoc(DocRef::MSGOP, opDesc->type, opDesc->link);

  result.info.op = opDesc->value;
  result.syntax.mnemonic = opDesc->sym;
  result.syntax.controls = ".";
  result.syntax.controls += ToSyntax(sfid);

  const SendOpDefinition &sOpInfo = lookupSendOp(opDesc->value);
  if (!sOpInfo.isValid())
    return;
  if (sOpInfo.hasChMask()) {
    result.info.addAttr(MessageInfo::Attr::HAS_CHMASK);
  }

  // no LSC messages use ind1
  if (indDesc1 != REGREF_INVALID) {
    error(0, 6, "IND1 is forbidden on this message");
  }

  if (sfid == SFID::TGM) {
    result.info.addAttr(MessageInfo::Attr::TYPED);
    decodeTyped(sOpInfo);
  } else {
    decodeUntyped(sOpInfo);
  }

  result.syntax.controls += dataTypePrefixSyntax + vectorSuffixSyntax +
                            addrSizeSyntax + ovfSyntax + cacheControlSyntax +
                            extCacheControlOpSyntax + extCacheControlSizeSyntax;

  constructSurfaceSyntax();

  result.info.symbol = sOpInfo.mnemonic;
  result.info.description = symbolFromSyntax();
}

void MessageDecoderLscE64::decodeUntyped(const SendOpDefinition &sOpInfo) {
  if (sfid == SFID::SLM) {
    result.info.addAttr(MessageInfo::Attr::SLM);
  }

  bool stateful = false;
  if (sOpInfo.op != SendOp::FENCE && sOpInfo.op != SendOp::EXTENDED_CACHE_CTRL) {
    // only UGM, URB, SLM message that lacks AddrTypeSize
    decodeAddrTypeSize();
    stateful = result.info.addrType == AddrType::SURF;
  }
  if (sOpInfo.op == SendOp::EXTENDED_CACHE_CTRL) {
    // extended cache control has A64 address type
    addrSizeSyntax = ".a64";
  }

  switch (sOpInfo.op) {
  case SendOp::LOAD:
  case SendOp::LOAD_STATUS:
  case SendOp::STORE:
    if (stateful) {
      decodeDP_LOAD_STORE_STATEFUL_DESC(sOpInfo);
    } else {
      decodeDP_LOAD_STORE_STATELESS_DESC(sOpInfo);
    }
    break;
  case SendOp::LOAD_QUAD:
  case SendOp::LOAD_QUAD_STATUS:
  case SendOp::STORE_QUAD:
    if (stateful) {
      decodeDP_CMASK_STATEFUL_DESC(sOpInfo);
    } else {
      decodeDP_CMASK_STATELESS_DESC(sOpInfo);
    }
    break;
  case SendOp::LOAD_BLOCK2D:
  case SendOp::STORE_BLOCK2D:
    if (stateful) {
      error(14, 2, "AddrTypeSize must be FLAT_A64_A64 for block2d");
    }
    decodeDP_2DBLOCK_DESC(sOpInfo);
    break;
  case SendOp::FENCE:
    decodeDP_FENCE_DESC(sOpInfo);
    break;
  case SendOp::GET_WATCHPOINT:
  case SendOp::SET_WATCHPOINT:
    decodeDP_WATCHPOINT_DESC();
    break;
  case SendOp::EXTENDED_CACHE_CTRL:
    decodeDP_EXTENDED_CACHE_CTRL_DESC(sOpInfo);
    break;
  default:
    if (sOpInfo.isAtomic()) {
      if (stateful) {
        decodeDP_ATOMIC_STATEFUL_DESC(sOpInfo);
      } else {
        decodeDP_ATOMIC_STATELESS_DESC(sOpInfo);
      }
    } else {
      error(0, 6, "unhandled SendOp");
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
void MessageDecoderLscE64::addImplicitAddrTypeSURF() {
  result.info.addrType = AddrType::SURF;
  result.info.addrSizeBits = 32;
  result.info.uniformBaseAddrSizeBits = 0;
  result.info.surfaceState = indDesc0;
  addDoc(DocRef::IND0, "DP_INDIR0_STATEFUL_DESC", "72036");
}

void MessageDecoderLscE64::addDocsForAddrPayloadsUntypedLoadStoreAtomic() {
  if (result.info.addrType == AddrType::SURF && indDesc0 != REGREF_INVALID) {
    addDoc(DocRef::IND0, "DP_INDIR0_STATEFUL_DESC", "72036");
  } else {
    addDoc(DocRef::IND0, "DP_INDIR0_STATELESS_DESC", "71955");
  }

  const auto transpose = getDescBits(10, 1) != 0;
  if (transpose) {
    // same for all transpose even stateful etc...
    // "If the address size is A16 or A32, then the
    // most significant bits will be unused."
    addDoc(DocRef::SRC0, "A64_PAYLOAD_SIMT1", "71967");
  } else if (execSize == ExecSize::INVALID || execSize == ExecSize::SIMD32) {
    if (result.info.addrSizeBits == 32) {
      addDoc(DocRef::SRC0, "A32_PAYLOAD", "71964");
    } else {
      addDoc(DocRef::SRC0, "A64_PAYLOAD", "71966");
    }
  } else {
    if (result.info.addrSizeBits == 32) {
      addDoc(DocRef::SRC0, "A32_PAYLOAD_SIMT16", "71965");
    } else {
      addDoc(DocRef::SRC0, "A64_PAYLOAD_SIMT16", "71968");
    }
  }
}

void MessageDecoderLscE64::decodeDP_LOAD_STORE_STATELESS_DESC(
  const SendOpDefinition &sOpInfo)
{
  addDoc(DocRef::DESC, "DP_LOAD_STORE_STATELESS_DESC", "71885");
  addDocsForAddrPayloadsUntypedLoadStoreAtomic();

  // Already decoded:
  //   [5:0] Opcode
  //   [6] reserved
  //   [15:14] AddrTypeSize
  // New fields:
  //   [10:10] Transpose
  //   [9:7] VectorSize
  decodeVectorSize(); // includes transpose
  //   [13:11] DataSize
  decodeDataSizeWithRefs(sOpInfo);
  //   [19:16] CacheOverride
  decodeCacheControl(sOpInfo.op);
  //   [20] Reserved
  addReserved(20, 1);
  //   [21] Overfetch
  decodeOverfetch();
  //   [43:22] GlobalOffset
  decodeImmOffAddr(22, 43 - 22 + 1);
  //   [45:44] OffsetScaling
  decodeAddrScaling();
  //   [46] Reserved
  addReserved(46, 1);

  computePayloadSizesLoadStoreAtomic(sOpInfo);
}

///////////////////////////////////////////////////////////////////////////////
void MessageDecoderLscE64::decodeDP_LOAD_STORE_STATEFUL_DESC(
  const SendOpDefinition &sOpInfo)
{
  addDoc(DocRef::DESC, "DP_LOAD_STORE_STATEFUL_DESC", "72045");
  addDocsForAddrPayloadsUntypedLoadStoreAtomic();

  // Already decoded:
  //   [5:0] Opcode
  //   [6] reserved
  //   [15:14] AddrTypeSize
  // New fields:
  //   [10:10] Transpose
  //   [9:7] VectorSize
  decodeVectorSize(); // includes transpose
  //   [13:11] DataSize
  decodeDataSizeWithRefs(sOpInfo);
  //   [19:16] CacheOverride
  decodeCacheControl(sOpInfo.op);
  //   [20] Reserved
  addReserved(20, 1);
  //   [21] Overfetch
  decodeOverfetch();
  //   [26:22] Surface State Index
  decodeSsIdx();
  //   [43:27] GlobalOffset
  decodeImmOffAddr(27, 43 - 27 + 1);
  //   [45:44] OffsetScaling
  decodeAddrScaling();
  //   [46] Reserved
  addReserved(46, 1);

  computePayloadSizesLoadStoreAtomic(sOpInfo);
}

void MessageDecoderLscE64::decodeDP_ATOMIC_STATELESS_DESC(
  const SendOpDefinition &sOpInfo)
{
  addDoc(DocRef::DESC, "DP_ATOMIC_STATELESS_DESC", "71876");
  addDocsForAddrPayloadsUntypedLoadStoreAtomic();

  // Already decoded:
  //   [5:0] Opcode
  //   [6] reserved
  //   [15:14] AddrTypeSize
  // New fields:
  //   [10:10] Transpose (forbidden in atmoics)
  //   [9:7] VectorSize
  decodeVectorSize();
  //   [13:11] DataSize
  decodeDataSizeWithRefs(sOpInfo);
  if (result.info.isTransposed())
    error(10, 1, "transpose forbidding on atomic operations");
  //   [19:16] CacheOverride
  decodeCacheControl(sOpInfo.op);
  //   [21:20] Reserved (overfetch forbidding since L1 is always .uc)
  addReserved(20, 2);
  //   [43:22] GlobalOffset
  decodeImmOffAddr(22, 43 - 22 + 1);
  //   [45:44] OffsetScaling
  decodeAddrScaling();
  //   [46] Reserved
  addReserved(46, 1);

  computePayloadSizesLoadStoreAtomic(sOpInfo);
}

void MessageDecoderLscE64::decodeDP_ATOMIC_STATEFUL_DESC(
  const SendOpDefinition &sOpInfo)
{
  addDoc(DocRef::DESC, "DP_ATOMIC_STATEFUL_DESC", "72047");
  addDocsForAddrPayloadsUntypedLoadStoreAtomic();

  // Already decoded:
  //   [5:0] Opcode
  //   [6] reserved
  //   [15:14] AddrTypeSize
  // New fields:
  //   [10:10] Transpose (forbidden in atmoics)
  //   [9:7] VectorSize
  decodeVectorSize(); // includes transpose
  //   [13:11] DataSize
  decodeDataSizeWithRefs(sOpInfo);
  //   [19:16] CacheOverride
  decodeCacheControl(sOpInfo.op);
  //   [20] Reserved (overfetch forbidden since L1 requires .uc)
  addReserved(20, 2);
  //   [26:22] Surface State Index
  decodeSsIdx();
  //   [43:27] GlobalOffset
  decodeImmOffAddr(27, 43 - 27 + 1);
  //   [45:44] OffsetScaling
  decodeAddrScaling();
  //   [46] Reserved
  addReserved(46, 1);

  computePayloadSizesLoadStoreAtomic(sOpInfo);
}

void MessageDecoderLscE64::decodeDP_CMASK_STATELESS_DESC(
  const SendOpDefinition &sOpInfo) {
  addDoc(DocRef::DESC, "DP_CMASK_STATELESS_DESC", "71874");
  addDocsForAddrPayloadsUntypedLoadStoreAtomic();

  // Already decoded:
  //   [5:0] Opcode
  //   [6] reserved
  //   [15:14] AddrTypeSize
  // New fields:
  //   [10:7] ChMask
  decodeVectorSize();
  //   [13:11] DataSize
  decodeDataSizeWithRefs(sOpInfo);
  //   [19:16] CacheOverride
  decodeCacheControl(sOpInfo.op);
  //   [20] Reserved
  addReserved(20, 1);
  //   [21] Overfetch
  decodeOverfetch();
  //   [43:22] GlobalOffset
  decodeImmOffAddr(22, 43 - 22 + 1);
  //   [45:44] OffsetScaling
  decodeAddrScaling();
  //   [46] Reserved
  addReserved(46, 1);

  computePayloadSizesLoadStoreAtomic(sOpInfo);
}

void MessageDecoderLscE64::decodeDP_CMASK_STATEFUL_DESC(
  const SendOpDefinition &sOpInfo) {
  addDoc(DocRef::DESC, "DP_CMASK_STATEFUL_DESC", "72046");
  addDocsForAddrPayloadsUntypedLoadStoreAtomic();

  // Already decoded:
  //   [5:0] Opcode
  //   [6] reserved
  //   [15:14] AddrTypeSize
  // New fields:
  //   [10:7] ChMask
  decodeVectorSize();
  //   [13:11] DataSize
  decodeDataSizeWithRefs(sOpInfo);
  //   [19:16] CacheOverride
  decodeCacheControl(sOpInfo.op);
  //   [20] Reserved
  addReserved(20, 1);
  //   [21] Overfetch
  decodeOverfetch();
  //   [26:22] Surface State Index
  decodeSsIdx();
  //   [43:27] GlobalOffset
  decodeImmOffAddr(27, 43 - 27 + 1);
  //   [45:44] OffsetScaling
  decodeAddrScaling();
  //   [46] Reserved
  addReserved(46, 1);

  computePayloadSizesLoadStoreAtomic(sOpInfo);
}

void MessageDecoderLscE64::decodeDP_2DBLOCK_DESC(
  const SendOpDefinition &sOpInfo)
{
  addDoc(DocRef::DESC, "DP_2DBLOCK_DESC", "71884");
  if (sfid == SFID::TGM) {
    addDoc(DocRef::SRC0, "ATYPED2DBLOCK_PAYLOAD", "71983");
  } else {
    addDoc(DocRef::SRC0, "A2DBLOCK_PAYLOAD", "71984");
  }

  // Already decoded:
  //   [5:0] Opcode
  //   [6] Reserved
  //   [15:14] AddrTypeSize (for untyped)
  if (sfid == SFID::TGM) {
    decodeAddrTypeSTATEFUL(); // TGM stateful
  }
  // New fields:
  //   [8:7] Reserved
  addReserved(7, 2);
  //   [10] Transpose
  decodeField("Transpose", 10, 1, [&](uint32_t val, std::stringstream &ss) {
      if (val)
        vectorSuffixSyntax += "t";
    });
  //   [9] VNNI
  decodeField("VNNI", 9, 1, [&](uint32_t val, std::stringstream &ss) {
      if (val)
        vectorSuffixSyntax += "v";
    });
  //   [13:11] DataSize
  decodeDataSize();
  //   [19:16] CacheOverride
  decodeCacheControl(sOpInfo.op);
  //   [21:20] Reserved
  addReserved(20, 2);
  //   [45:22] (GlobalOffsetX,GlobalOffsetY)
  decodeImmOffCoord2D();
  //   [46] Reserved
  addReserved(46, 1);

  computePayloadSizesLoadStoreAtomic(sOpInfo);
}

void MessageDecoderLscE64::decodeDP_FENCE_DESC(
  const SendOpDefinition &sOpInfo)
{
  addDoc(DocRef::DESC, "DP_FENCE_DESC", "71877");

  if (result.info.execWidth > 1)
    error(0, 6, "ExecSize should be 1");
  else if (result.info.execWidth <= 0)
    result.info.execWidth = 1;

  addReserved(7, 1);

  std::string ftMeaning, ftSym;
  auto ft = getDescBits(8, 3);
  switch (ft) {
  case 0x0:
    ftMeaning = "None";
    ftSym = "none";
    break;
  case 0x1:
    ftMeaning = "Evict (for R/W evict dirty (M to I) and invalidate clean; "
                "for R/O invalide clean)";
    ftSym = "evict";
    break;
  case 0x2:
    ftMeaning = "Invalidate (Invalidate clean lines)";
    ftSym = "invalidate";
    break;
  case 0x3:
    ftMeaning = "Discard (for R/W invalidate dirty (M to I) without "
                "writeback to next level; nop for R/O)";
    ftSym = "discard";
    break;
  case 0x4:
    ftMeaning = "Clean (for R/W writeback dirty to next level but keep in "
                "cache as clean (M to V); nop for R/O)";
    ftSym = "clean";
    break;
  default:
    ftMeaning = "???";
    ftSym = "?";
    error(8, 3, "invalid flush op");
    break;
  }
  result.syntax.controls += "." + ftSym;
  addField("DP_FLUSH_TYPE", 8, 3, ft, ftMeaning);

  std::string scMeaning, scSym;
  auto sc = getDescBits(11, 3);
  switch (sc) {
  case 0x0:
    scMeaning = "Threadgroup";
    scSym = "group";
    break;
  case 0x1:
    scMeaning = "Local (subslice)";
    scSym = "core";
    break;
  case 0x2:
    scMeaning = "Tile";
    scSym = "tile";
    break;
  case 0x3:
    scMeaning = "GPU";
    scSym = "gpu";
    break;
  case 0x4:
    scMeaning = "GPUs";
    scSym = "gpus";
    break;
  case 0x5:
    scMeaning = "System Release";
    scSym = "sysrel";
    break;
  case 0x6:
    scMeaning = "System Acquire";
    scSym = "sysacq";
    break;
  default:
    scMeaning = "???";
    scSym = "?";
    error(8, 3, "invalid flush scope");
    break;
  }
  result.syntax.controls += "." + scSym;
  addField("DP_FENCE_SCOPE", 11, 3, sc, scMeaning);

  addReserved(14, 46 - 14 + 1);

  result.syntax.layout = MessageSyntax::Layout::CONTROL;

  result.info.dstLenBytes = 0;
  result.info.src0LenBytes = BYTES_PER_REGISTER;
  result.info.src1LenBytes = 0;
}

void MessageDecoderLscE64::decodeTyped(const SendOpDefinition &sOpInfo) {
  bool opSupportsUvr = sOpInfo.op == SendOp::LOAD_QUAD ||
                       sOpInfo.op == SendOp::STORE_QUAD ||
                       sOpInfo.op == SendOp::LOAD_QUAD_MSRT ||
                       sOpInfo.op == SendOp::STORE_QUAD_MSRT ||
                       sOpInfo.op == SendOp::LOAD_QUAD_STATUS ||
                       sOpInfo.op == SendOp::LOAD_BLOCK2D ||
                       sOpInfo.op == SendOp::STORE_BLOCK2D ||
                       sOpInfo.op == SendOp::STORE_UNCOMPRESSED_QUAD ||
                       sOpInfo.isAtomic();
  if (opSupportsUvr) {
    result.info.attributeSet |= MessageInfo::Attr::HAS_UVRLOD;
  }

  switch (sOpInfo.op) {
  case SendOp::FENCE:
    decodeDP_FENCE_DESC(sOpInfo);
    break;
  case SendOp::READ_STATE:
    decodeDP_RSI_DESC(sOpInfo);
    break;
  case SendOp::LOAD_QUAD:
  case SendOp::STORE_QUAD:
  case SendOp::LOAD_QUAD_MSRT:
  case SendOp::STORE_QUAD_MSRT:
  case SendOp::LOAD_QUAD_STATUS:
    decodeDP_TYPED_CMASK_DESC(sOpInfo);
    break;
  case SendOp::LOAD_BLOCK2D:
  case SendOp::STORE_BLOCK2D:
    decodeDP_2DBLOCK_DESC(sOpInfo);
    break;
  default:
    if (sOpInfo.isAtomic()) {
      decodeDP_TYPED_ATOMIC_DESC(sOpInfo);
    } else {
      error(0, 6, "invalid op for TGM");
    }
  }
}

void MessageDecoderLscE64::decodeDP_TYPED_CMASK_DESC(
  const SendOpDefinition &sOpInfo)
{
  addDoc(DocRef::DESC, "DP_TYPED_CMASK_DESC", "71882");
  if (execSize == ExecSize::SIMD32 || execSize == ExecSize::INVALID) {
    addDoc(DocRef::SRC0, "A32_PAYLOAD_TYPED", "71953");
  } else {
    addDoc(DocRef::SRC0, "A32_PAYLOAD_TYPED_SIMT16", "71952");
  }

  addImplicitAddrTypeSURF();

  // Already decoded:
  //   [5:0] Opcode
  //   [6] reserved (EnableLSCBacking)
  // New fields:
  //   [13:11] SurfaceTypeHint
  decodeSurfaceTypeHint();
  //   [10:7] ChMask
  decodeVectorSize();
  //   [14] AddressInputFormat
  decodeField("AddressInputFormat", 14, 1,
    [&] (uint32_t val, std::stringstream &ss) {
      addrSizeSyntax = (val == 0 ? ".a32" : ".a16");
      result.info.addrSizeBits = (val == 0 ? 32 : 16);
      ss << (val == 0 ? "AIF32" : "AIF16");
    });
  //   [15] DataReturnFormat
  decodeField("DataReturnFormat", 15, 1,
    [&] (uint32_t val, std::stringstream &ss) {
      dataTypePrefixSyntax = (val == 0 ? ".d32" : ".d16");
      result.info.elemSizeBitsRegFile = (val == 0 ? 32 : 16);
      result.info.elemSizeBitsMemory = result.info.elemSizeBitsRegFile;
      ss << (val == 0 ? "DRF32" : "DRF16");
    });
  //   [19:16] CacheOverride
  decodeCacheControl(sOpInfo.op);
  //   [21:20] Reserved
  addReserved(20, 2);
  //   [26:22] Surface State Index
  decodeSsIdx();
  //   [29:27] Reserved
  addReserved(27, 3);
  //   [41:30] UVR offsets (R, V, U)
  decodeUvrOffsets();
  //   [46:42] Reserved (no IND1 either for TGM)
  addReserved(42, 46 - 42 + 1);

  computePayloadSizesLoadStoreAtomic(sOpInfo);
}

void MessageDecoderLscE64::decodeDP_TYPED_ATOMIC_DESC(
  const SendOpDefinition &sOpInfo)
{
  addDoc(DocRef::DESC, "DP_TYPED_ATOMIC_DESC", "71883");
  if (execSize == ExecSize::SIMD32 || execSize == ExecSize::INVALID) {
    addDoc(DocRef::SRC0, "A32_PAYLOAD_TYPED", "71953");
  } else {
    addDoc(DocRef::SRC0, "A32_PAYLOAD_TYPED_SIMT16", "71952");
  }

  addImplicitAddrTypeSURF();

  // Already decoded:
  //   [5:0] Opcode
  //   [6] reserved
  // New fields:
  //   [10:10] Transpose (forbidden in atmoics)
  //   [9:7] VectorSize DP_VECTOR_SIZE
  decodeVectorSize(); // should be V1 and non-transpose
  //   [13:11] DataSize DP_DATA_SIZE
  decodeDataSizeWithRefs(sOpInfo);
  if (result.info.elemsPerAddr != 1)
    error(9, 3, "atomics should be V1");
  if (result.info.isTransposed())
    error(9, 3, "atomics must be non-transposed");
  //   [15:14] AddrTypeSize DP_GLOBAL_ADDRESS_TYPE_SIZE
  decodeAddrTypeSTATEFUL();
  //   [19:16] CacheOverride DP_CACHE_ATOMIC
  decodeCacheControl(sOpInfo.op);
  //   [21:20] Reserved
  addReserved(20, 2);
  //   [26:22] Surface State Index
  decodeSsIdx();
  //   [29:27] Reserved
  addReserved(27, 3);
  //   [41:30] UVR offsets (R, V, U)
  decodeUvrOffsets();
  //   [46:42] Reserved
  addReserved(42, 46 - 42 + 1);

  computePayloadSizesLoadStoreAtomic(sOpInfo);
}
void MessageDecoderLscE64::decodeDP_RSI_DESC(
  const SendOpDefinition &sOpInfo)
{
  addDocs(DocRef::DESC, "DP_RSI_DESC", "71938",
          DocRef::SRC0, "DP_ASTATE_INFO_PAYLOAD", "71980",
          DocRef::DST, "DP_STATE_INFO_PAYLOAD", "71990");

  addImplicitAddrTypeSURF();

  // Already decoded:
  //   [5:0] Opcode
  //   [6] reserved
  // New fields:
  //   [9:7] RSI Sub-Opcode
  decodeField("RSISubOpcode", 7, 3, [&](uint32_t val, std::stringstream &ss) {
      if (val != 0)
        error(7, 3, "only 0 supported as RSI subop");
    });
  //   [13:10] Reserved
  addReserved(10, 4);
  //   [15:14] AddrTypeSize DP_GLOBAL_ADDRESS_TYPE_SIZE
  decodeAddrTypeSTATEFUL();
  //   [21:16] Reserved
  addReserved(16, 21 - 16 + 1);
  //   [26:22] Surface State Index
  decodeSsIdx();
  //   [46:27] Reserved
  addReserved(27, 46 - 27 + 1);

  result.info.dstLenBytes = BYTES_PER_REGISTER;
  result.info.src0LenBytes = 16; // 4 DW (U, V, R, LoD) (apparently)
  result.info.src1LenBytes = 0;
}

void MessageDecoderLscE64::decodeDP_EXTENDED_CACHE_CTRL_DESC(
    const SendOpDefinition &sOpInfo) {
  addDoc(DocRef::DESC, "DP_CCTRL_DESC", "74342");

  //  [19:16] cache policy
  //  [15:13] reserved
  //  [12:11] cache control size
  //  [10:7] cache control operation
  //  [6] reserved

  decodeCacheControl(sOpInfo.op);
  addReserved(13, 3);
  decodeCacheControlSize();
  decodeCacheControlOperation();
  addReserved(6, 1);
}


void MessageDecoderLscE64::decodeDP_WATCHPOINT_DESC() {
  bool isGet = result.info.op == SendOp::GET_WATCHPOINT;
  if (isGet) {
    addDocs(DocRef::DESC, "DP_WATCHPOINT_DESC", "71944",
            DocRef::SRC0, "DP_GET_WATCHPOINT_PAYLOAD", "71979");
  } else {
    addDocs(DocRef::DESC, "DP_WATCHPOINT_DESC", "71944",
            DocRef::SRC0, "DP_GET_WATCHPOINT_PAYLOAD", "71981");
  }
  if (result.info.execWidth <= 0) {
    result.info.execWidth = 1;
  } else if (result.info.execWidth != 1) {
    error(0, 6, "ExecSize should be 1");
  }
  if (indDesc0 != REGREF_INVALID) {
    error(0, 6, "IND0 is forbidden on this message");
  }

  decodeField("WatchpointRegister", 7, 3,
              [&](uint32_t enc, std::stringstream &ss) {
                ss << "register " << enc;
                std::stringstream wss;
                wss << "." << addrSizeSyntax << ".w" << enc;
                result.syntax.controls += wss.str();
              });
  // addReserved(6, 1);  (caller adds)
  addReserved(10, 4);
  // addrType (handled by caller)
  addReserved(16, 46 - 16 + 1);

  result.info.dstLenBytes = 0;
  if (isGet) {
    result.info.dstLenBytes = 10; // [74:0]
  }
  result.info.src0LenBytes = 0;
  result.info.src1LenBytes = 0;
  if (!isGet) {
    result.info.src1LenBytes = 10; // [74:0]
  }
}

////////////////////////////////////////////////////////////////////////////
void MessageDecoderLscE64::computePayloadSizesLoadStoreAtomic(
  const SendOpDefinition &sOpInfo)
{
  bool isBlock2d = result.info.op == SendOp::LOAD_BLOCK2D ||
                   result.info.op == SendOp::STORE_BLOCK2D;
  bool isLoadStatus = result.info.op == SendOp::LOAD_STATUS ||
                      result.info.op == SendOp::LOAD_QUAD_STATUS;

  if (result.info.execWidth < 0) {
    if (result.info.isTransposed() || isBlock2d) {
      result.info.execWidth = 1;
    } else {
      result.info.execWidth = 32;
    }
    // guess execution size
    warning(0, 6, "assuming ExecSize (", result.info.execWidth, ")");
  }

  int dataBytes = 0;

  if (isBlock2d) {
    dataBytes = -1; // unknown (buried in the payload header)
    if (sfid == SFID::TGM) {
      result.info.src0LenBytes = 30;
    } else {
      result.info.src0LenBytes = 31; // payload slightly larger for ugm
    }
  } else if (isLoadStatus) {
    dataBytes = 2 * 64; // 1 bit per dword => 2 GRF (HW seems to round up to SIMD32)
    int effectiveExecSize = std::max(16, result.info.execWidth);
    result.info.src0LenBytes = result.info.addrSizeBits / 8 * effectiveExecSize;
  } else if (result.info.isTransposed()) {
    result.info.src0LenBytes = result.info.addrSizeBits / 8;
    dataBytes = result.info.elemSizeBitsRegFile * result.info.elemsPerAddr / 8;
  } else {
    // things like SIMD4 will round up to SIMD16 at least
    // (SIMD1 transpose does not but is handled above)
    int effectiveExecSize = std::max(16, result.info.execWidth);
    result.info.src0LenBytes = result.info.addrSizeBits / 8 * effectiveExecSize;
    int dataBytesPerComp =
        result.info.elemSizeBitsRegFile * result.info.execWidth / 8;
    if (dataBytesPerComp < BYTES_PER_REGISTER && result.info.elemsPerAddr > 1) {
      // e.g. SIMT4 with v2 => round up to full registers since the
      // layout is oddball
      dataBytesPerComp = BYTES_PER_REGISTER;
    }
    dataBytes = dataBytesPerComp * result.info.elemsPerAddr;
  }

  // for TGM, if src0Len was given; then override the size
  // e.g. :6 on SIMD32 typed would imply u, v, r
  if (sfid == SFID::TGM && src0Len > 0)
    result.info.src0LenBytes = src0Len * BYTES_PER_REGISTER;

  if (result.info.isLoad()) {
    result.info.dstLenBytes = dataBytes;
    result.info.src1LenBytes = 0;
  } else if (result.info.isStore()) {
    result.info.dstLenBytes = 0;
    result.info.src1LenBytes = dataBytes;
  } else if (result.info.isAtomic()) {
    result.info.dstLenBytes = dataBytes;
    result.info.src1LenBytes = dataBytes * sOpInfo.numAtomicArgs();
  }

  // append counter takes no address but the data payload in src0
  bool isAppendCounter = sOpInfo.op == SendOp::ATOMIC_ACADD ||
                         sOpInfo.op == SendOp::ATOMIC_ACSUB ||
                         sOpInfo.op == SendOp::ATOMIC_ACSTORE;
  if (isAppendCounter) {
    result.info.src0LenBytes = dataBytes;
    result.info.src1LenBytes = 0;
  }
}

void MessageDecoderLscE64::decodeAddrTypeSize() {
  if (sfid == SFID::UGM) {
    decodeAddrTypeUGM();
  } else if (sfid == SFID::SLM) {
    decodeAddrTypeSLM();
  } else if (sfid == SFID::URB) {
    decodeAddrTypeURB();
  } else if (sfid == SFID::TGM) {
    decodeAddrTypeSTATEFUL();
  } else {
    IGA_ASSERT_FALSE("sfid should be LSC untyped");
  }
}
static std::string determineId0Meaning(const RegRef ind0,
                                       const char *symbol,
                                       const char *indicesAre) {
  std::stringstream ss;
  ss << symbol << ": " << indicesAre;
  if (ind0 != REGREF_INVALID) {
    ss << " with "
          "uniform a64 in "
          "s"
       << ind0.regNum << "." << ind0.subRegNum
       << " added to each";
  }
  return ss.str();
}

enum GlbAddrType {
  FLAT_A64_A32U = 0,
  FLAT_A64_A32S = 1,
  FLAT_A64_A64 = 2,
  STATEFUL_A32 = 3,
};

void MessageDecoderLscE64::decodeAddrTypeSTATEFUL() {
  auto enc = getDescBits(14, 2);
  std::string meaning;
  switch (enc) {
  case GlbAddrType::STATEFUL_A32:
    addrSizeSyntax = ".a32";
    meaning = "STATEFUL_A32 (id0 is surface state pointer whose "
              "surface base address is added to a32 indices)";
    break;
  default:
    meaning = "?";
    error(14, 2, "invalid address type");
  }
  addField("AddrTypeSize", 14, 2, enc, meaning);
}

void MessageDecoderLscE64::decodeAddrTypeUGM() {
  auto enc = getDescBits(14, 2);
  std::string meaning;
  switch (enc) {
  case GlbAddrType::FLAT_A64_A32U:
    addrSizeSyntax = ".a32u";
    result.info.addrType = AddrType::FLAT;
    result.info.addrSizeBits = 32;
    result.info.uniformBase = indDesc0;
    result.info.uniformBaseAddrSizeBits = 64;
    meaning = determineId0Meaning(indDesc0, "FLAT_A64_A32U",
                                  "a32 unsigned offsets (zero-extended to a64)");
    break;
  case GlbAddrType::FLAT_A64_A32S:
    addrSizeSyntax = ".a32s";
    result.info.addrType = AddrType::FLAT;
    result.info.addrSizeBits = 32;
    result.info.uniformBase = indDesc0;
    result.info.uniformBaseAddrSizeBits = 64;
    result.info.signExtendIndices = true;
    meaning = determineId0Meaning(indDesc0, "FLAT_A64_A32S",
                                  "a32 signed offsets (sign-extened to a64)");
    break;
  case GlbAddrType::FLAT_A64_A64:
    addrSizeSyntax = ".a64";
    result.info.addrType = AddrType::FLAT;
    result.info.addrSizeBits = 64;
    result.info.uniformBaseAddrSizeBits = 64;
    result.info.uniformBase = indDesc0;
    meaning = determineId0Meaning(indDesc0, "FLAT_A64_A64", "a64 offsets");
    break;
  case GlbAddrType::STATEFUL_A32:
    addrSizeSyntax = ".a32";
    result.info.addrType = AddrType::SURF;
    result.info.addrSizeBits = 32;
    result.info.uniformBaseAddrSizeBits = 0;
    result.info.surfaceState = indDesc0;
    meaning = "STATEFUL_A32 (id0 is a surface state pointer, and the a32 "
              "offset are added to that surface state's base pointer)";
    break;
  default:
    error(14, 2, "invalid address type");
  }
  addField("DP_ADDR_TYPE_SIZE", 14, 2, enc, meaning);
}

void MessageDecoderLscE64::decodeAddrTypeSLM() {
  auto enc = getDescBits(14, 2);
  enum {
    FLAT_A32_A32 = 0,
  };

  result.info.addrType = AddrType::FLAT;
  result.info.addrSizeBits = 32;
  result.info.uniformBase = indDesc0;
  result.info.uniformBaseAddrSizeBits = 32;

  addrSizeSyntax = ".a32";

  std::string meaning;
  switch (enc) {
  case FLAT_A32_A32:
    meaning = determineId0Meaning(indDesc0, "FLAT_A32_A32", "a32 offsets");
    break;
  default:
    error(14, 2, "invalid address type");
  }

  addField("DP_ADDR_TYPE_SIZE", 14, 2, enc, meaning);
}

void MessageDecoderLscE64::decodeAddrTypeURB() {
  auto enc = getDescBits(14, 2);
  enum {
    GLOBAL_A32_A32 = 0,
  };

  result.info.addrType = AddrType::FLAT;
  result.info.addrSizeBits = 32;
  result.info.uniformBase = indDesc0;
  result.info.uniformBaseAddrSizeBits = 32;

  addrSizeSyntax = ".a32";

  std::string meaning;
  switch (enc) {
  case GLOBAL_A32_A32:
    meaning = "GLOBAL_A32_A32 "
              "(id0 is a uniform a32 global base added to a32 indices)";
    break;
  default:
    error(14, 2, "invalid address type");
  }

  addField("DP_ADDR_TYPE_SIZE", 14, 2, enc, meaning);
}

void MessageDecoderLscE64::decodeAddrScaling() {
  if (result.info.elemSizeBitsMemory == 0)
    error(44, 2, "need to decode data size first");
  if (result.info.elemsPerAddr == 0 &&
      // a chmask can have message chan have 0 channels (e.g. render target)
      // that doesn't hit this path, but we shouldn't assert unless
      // we really need to
     !result.info.hasAttr(MessageInfo::Attr::HAS_CHMASK))
    error(44, 2, "need to decode vector size first");
  if (result.info.elemsPerAddr == 0)
    warning(7, 2, "empty chmask in LSC message");
  //
  auto enc = getDescBits(44, 2);
  std::stringstream meaning;
  if (enc == 0) {
    meaning << "unscaled indices";
    result.info.addrScaling = 1;
  } else if (result.info.elemsPerAddr == 0) {
    meaning << "(undefined behavior)";
  } else {
    auto sc = (1 << (enc - 1));
    auto vsz = result.info.elemsPerAddr;
    auto scBytes = result.info.elemSizeBitsMemory / 8 * vsz;
    meaning << "scale by " << sc << "X data*vecsize (" << scBytes << ")";
    if (scBytes > 32) {
      error(44, 2, "total scale amount must be <= 32B");
    }
    if ((vsz & (vsz - 1)) != 0) {
      warning(44, 2,
              "vector size must be a power of two to scale "
              "(V3 not supported)");
    }
    result.info.addrScaling = scBytes;
    result.syntax.scale += fmtHexSigned(scBytes) + "*";
  }
  addField("Offset Scaling", 44, 2, enc, meaning.str());
}

void MessageDecoderLscE64::decodeDataSize() {
  const auto *dszDesc = decodeDataSizeValue();
  if (dszDesc) {
    dataTypePrefixSyntax = dszDesc->sym;
  } else {
    dataTypePrefixSyntax = ".d??";
  }
}

void MessageDecoderLscE64::decodeDataSizeWithRefs(
  const SendOpDefinition &sOpInfo)
{
  decodeDataSize();

  auto addDataPayloadD8U32 = [&]() {
    auto kind = result.info.isLoad() ? DocRef::DST : DocRef::SRC1;
    if (execSize == ExecSize::INVALID || execSize == ExecSize::SIMD32)
      addDoc(kind, "D8U32_PAYLOAD", "71978");
    else
      addDoc(kind, "D8U32_PAYLOAD_SIMT16", "74736");
  };
  auto addDataPayloadD16U32 = [&]() {
    auto kind = result.info.isLoad() ? DocRef::DST : DocRef::SRC1;
    if (execSize == ExecSize::INVALID || execSize == ExecSize::SIMD32)
      addDoc(kind, "D16U32_PAYLOAD", "71969");
    else
      addDoc(kind, "D16U32_PAYLOAD_SIMT16", "71993");
  };
  auto addDataPayloadD32 = [&]() {
    auto kind = result.info.isLoad() ? DocRef::DST : DocRef::SRC1;
    if (execSize == ExecSize::INVALID || execSize == ExecSize::SIMD32)
      addDoc(kind, "D32_PAYLOAD", "71970");
    else
      addDoc(kind, "D32_PAYLOAD_SIMT16", "71971");
  };
  auto addDataPayloadD64 = [&]() {
    auto kind = result.info.isLoad() ? DocRef::DST : DocRef::SRC1;
    if (execSize == ExecSize::INVALID || execSize == ExecSize::SIMD32)
      addDoc(kind, "D64_PAYLOAD", "71974");
    else
      addDoc(kind, "D64_PAYLOAD_SIMT16", "71975");
  };

  bool d16u32 =
      result.info.elemSizeBitsMemory == 16 &&
      result.info.elemSizeBitsRegFile == 32;
  bool d8u32 =
      result.info.elemSizeBitsMemory == 8 &&
      result.info.elemSizeBitsRegFile == 32;
  if (sOpInfo.isAtomic()) {
    if (d16u32) {
      addDataPayloadD16U32(); // dst return payload
      if (sOpInfo.numAtomicArgs() == 2) { // ternary atomics
        if (execSize == ExecSize::INVALID || execSize == ExecSize::SIMD32)
          addDoc(DocRef::SRC1, "D16U32_2SRC_ATM_PAYLOAD", "71996");
        else
          addDoc(DocRef::SRC1, "D16U32_2SRC_ATM_PAYLOAD_SIMT16", "71997");
      } // else binary atomic has same format for src1 as dst; unary lacks src1
    } else if (result.info.elemSizeBitsRegFile == 32) {
      addDataPayloadD32(); // return payload
      if (sOpInfo.numAtomicArgs() == 2) {
        if (execSize == ExecSize::INVALID || execSize == ExecSize::SIMD32)
          addDoc(DocRef::SRC1, "D32_2SRC_ATM_PAYLOAD", "71972");
        else
          addDoc(DocRef::SRC1, "D32_2SRC_ATM_PAYLOAD_SIMT16", "71973");
      } // else binary atomic has same format for src1 as dst; unary lacks src1
    } else if (result.info.elemSizeBitsRegFile == 64) {
      addDataPayloadD64(); // dst return payload
      if (sOpInfo.numAtomicArgs() == 2) {
        if (execSize == ExecSize::INVALID || execSize == ExecSize::SIMD32)
          addDoc(DocRef::SRC1, "D64_2SRC_ATM_PAYLOAD", "71976");
        else
          addDoc(DocRef::SRC1, "D64_2SRC_ATM_PAYLOAD_SIMT16", "71977");
      } // else src1 has same format as dst or is absent
    }
  } else if (result.info.isTransposed()) {
    auto kind = result.info.isLoad() ? DocRef::DST : DocRef::SRC1;
    if (result.info.elemSizeBitsRegFile == 32) {
      addDoc(kind, "D32_TRANSPOSED_PAYLOAD", "71988");
    } else if (result.info.elemSizeBitsRegFile == 64) {
      addDoc(kind, "D64_TRANSPOSED_PAYLOAD", "71989");
    }
  } else { // non-atomic (load or store)
    if (d8u32) {
      addDataPayloadD8U32();
    } else if (d16u32) {
      addDataPayloadD16U32();
    } else if (result.info.elemSizeBitsRegFile == 32) {
      addDataPayloadD32();
    } else if (result.info.elemSizeBitsRegFile == 64) {
      addDataPayloadD64();
    }
  }
}

void MessageDecoderLscE64::decodeVectorSize() {
  if (result.info.hasAttr(MessageInfo::Attr::HAS_CHMASK)) {
    vectorSuffixSyntax = decodeVectorSizeCmask("xyzw");
  } else {
    vectorSuffixSyntax = decodeVectorSizeElems();
  }
}

std::string MessageDecoderLscE64::decodeVectorSizeElems() {
  const auto *e = LSC_LOOKUP_TABLE("Vector Size", 7, 3, LSC_VEC_SIZE);
  if (e == nullptr) {
    return "?";
  }
  auto trnps = getDescBits(10, 1);
  addField("Transpose", 10, 1, trnps,
           trnps ? "Transposed" : "Non-transposed");
  result.info.elemsPerAddr = e->value;

  std::string sym = (e->value != 1) ? e->sym : "";

  if (trnps) {
    if (result.info.execWidth == 0)
      result.info.execWidth = 1;

    sym += "t"; // append t to get "x16t"
    result.info.addAttr(MessageInfo::Attr::TRANSPOSED);
    switch (result.info.op) {
    case SendOp::LOAD:
    case SendOp::LOAD_BLOCK2D:
    case SendOp::LOAD_STATUS:
    case SendOp::STORE:
    case SendOp::STORE_BLOCK2D:
      break;
    default:
      warning(10, 1, "transpose not supported on this send op");
    }
    if (execSize != ExecSize::SIMD1) {
      warning(10, 1, "transpose messages should be SIMD1");
    }
  }

  return sym;
}

const LscDataSizeValidValue *MessageDecoderLscE64::decodeDataSizeValue() {
  const auto *e = LSC_LOOKUP_TABLE("Data Size", 11, 3, LSC_DATA_SIZE);
  if (e != nullptr) {
    result.info.elemSizeBitsMemory = e->bitsInMem;
    result.info.elemSizeBitsRegFile = e->bitsInReg;
  }
  return e;
}

void MessageDecoderLscE64::decodeCacheControl(SendOp sop) {
  const auto *ccDesc = decodeCacheControlValue(sop);
  if (ccDesc) {
    if (ccDesc->encoding != 0)
      cacheControlSyntax = ccDesc->sym;

    result.info.cachingL1 = ccDesc->l1;
    result.info.cachingL2 = ccDesc->l2;
    result.info.cachingL3 = ccDesc->l3;
  } else {
    cacheControlSyntax = ".?";
  }
}

void MessageDecoderLscE64::decodeOverfetch() {
  decodeField("Overfetch", 21, 1, [&] (uint32_t val, std::stringstream &ss) {
      ss << (val ? "LSC opportunistically fetches up to 256B" : "");
      if (val)
        ovfSyntax = ".ovf";
    });
}

enum CacheControlSize {
  CCSIZE64B = 0,
  CCSIZE128B = 1,
  CCSIZE192B = 2,
  CCSIZE256B = 3,
};


void MessageDecoderLscE64::decodeCacheControlSize() {
  auto enc = getDescBits(11, 2);
  std::string meaning;
  switch(enc) {
    case CacheControlSize::CCSIZE64B:
      meaning = "Operation affects single 64B cache line";
      extCacheControlSizeSyntax = ".64B";
      break;
    case CacheControlSize::CCSIZE128B:
      meaning = "Operation affects 128B; two cache lines";
      extCacheControlSizeSyntax = ".128B";
      break;
    case CacheControlSize::CCSIZE192B:
      meaning = "Operation affects 192B; three cache lines";
      extCacheControlSizeSyntax = ".192B";
      break;
    case CacheControlSize::CCSIZE256B:
      meaning = "Operation affects 256B; four cache lines";
      extCacheControlSizeSyntax = ".256B";
      break;
  }
  addField("CacheControlSize", 11, 2, enc, meaning);
}

enum CacheControlOperation {
  SET = 0,
  RESET = 1,
};

void MessageDecoderLscE64::decodeCacheControlOperation() {
  auto enc = getDescBits(7, 4);
  std::string meaning;
  switch(enc) {
    case CacheControlOperation::SET:
      meaning = "Set dirty bits to one, set data to zero";
      extCacheControlOpSyntax = ".set";
      break;
    case CacheControlOperation::RESET:
      meaning = "Set dirty bits to zero";
      extCacheControlOpSyntax = ".reset";
      break;
    default:
      meaning = "??";
      break;
  }
  addField("CacheControlOperation", 7, 4, enc, meaning);
}


void MessageDecoderLscE64::decodeImmOffAddr(int off, int len) {
  if (result.info.elemSizeBitsMemory == 0) {
    error(off, len, "invalid data size (cannot compute imm off size)");
    return; // errors
  }
  int immOff = getDescBitsSigned(off, len);
  // encoded as elements, scale to bytes
  result.info.immediateOffset = immOff * result.info.elemSizeBitsMemory / 8;

  if (result.info.immediateOffset != 0) {
    result.syntax.immOffset = fmtHexSigned(result.info.immediateOffset);
  }

  std::stringstream mss;
  mss << "add " << immOff << " elements to each index";
  addField("AddrImmOff", off, len, getDescBits(off, len), mss.str());
}

void MessageDecoderLscE64::decodeImmOffCoord2D() {
  if (result.info.elemSizeBitsMemory == 0) {
    error(22, 12, "cannot decode imm off with invalid data size");
    return;
  }
  auto decodeCoordOff = [&](const char *field, const char *which, int off) {
    uint32_t immOffU = getDescBits(off, 12);
    int immOffElems = getSignedBits<int32_t>(immOffU, 0, 12);
    //
    std::stringstream mss;
    mss << "add " << immOffElems << " elements to " << which << "-coordinate";
    addField(field, off, 12, immOffU, mss.str());
    return immOffElems;
  };
  result.info.immediateOffsetBlock2dX =
      decodeCoordOff("CoordImmOff2D.X", "X", 22);
  result.info.immediateOffsetBlock2dY =
      decodeCoordOff("CoordImmOff2D.Y", "Y", 34);
  if (result.info.immediateOffsetBlock2dX ||
      result.info.immediateOffsetBlock2dY) {
    std::stringstream ss;
    ss << "(";
    ss << fmtHexSigned(result.info.immediateOffsetBlock2dX);
    ss << ",";
    ss << fmtHexSigned(result.info.immediateOffsetBlock2dY);
    ss << ")";
    result.syntax.immOffset = ss.str();
  }
}

///////////////////////////////////////////////////////////////////////////////
std::string MessageDecoderLscE64::symbolFromSyntax() const {
  std::stringstream sym;
  sym << result.syntax.mnemonic;
  if (!result.syntax.controls.empty())
    sym << result.syntax.controls;
  if (!result.syntax.surface.empty()) {
   sym << " ";
   sym << result.syntax.surface;
  } else if (!result.syntax.isControl()) {
   sym << " ";
  }

  if (!result.syntax.isControl()) {
    sym << "[";
    bool printed = false;
    if (indDesc0 != REGREF_INVALID && result.info.addrType == AddrType::FLAT) {
      sym << "s" << indDesc0.regNum << "." << indDesc0.subRegNum;
      printed = true;
    }
    if (result.info.src0LenBytes > 0) {
      if (printed)
        sym <<  "+";
      if (!result.syntax.scale.empty()) {
        sym << result.syntax.scale;
      }
      sym << "src0";
      printed = true;
    }
    if (!result.syntax.immOffset.empty()) {
      if (result.syntax.immOffset[0] != '-')
        sym << "+";
      sym << result.syntax.immOffset;
    }
    sym << "]";
  } else {
    sym << " src0";
  }

  return sym.str();
} // symbolFromSyntax

void iga::DecodeMessageLscE64(Platform platform, SFID sfid, ExecSize execSize,
                              int src0Len, int src1Len, uint64_t desc,
                              RegRef id0, RegRef id1, DecodeResult &result) {
  MessageDecoderLscE64 mdec {platform, sfid, execSize, src0Len, src1Len,
                             desc, id0, id1, result};
  mdec.decode();
  if (result.errors.empty()) {
    mdec.checkBytesLen("src0", result.info.src0LenBytes, src0Len);
    mdec.checkBytesLen("src1", result.info.src1LenBytes, src1Len);
  }
}
