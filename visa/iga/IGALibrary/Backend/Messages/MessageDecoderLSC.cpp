/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MessageDecoder.hpp"

#include <utility>

using namespace iga;

enum LscOp : uint32_t {
  LSC_LOAD = 0x00,
  LSC_LOAD_STRIDED = 0x01,
  LSC_LOAD_QUAD = 0x02, // aka load_cmask
  LSC_LOAD_BLOCK2D = 0x03,
  LSC_STORE = 0x04,
  LSC_STORE_STRIDED = 0x05,
  LSC_STORE_QUAD = 0x06, // aka store_cmask
  LSC_STORE_BLOCK2D = 0x07,
  //
  LSC_ATOMIC_IINC = 0x08,
  LSC_ATOMIC_IDEC = 0x09,
  LSC_ATOMIC_LOAD = 0x0A,
  LSC_ATOMIC_STORE = 0x0B,
  LSC_ATOMIC_IADD = 0x0C,
  LSC_ATOMIC_ISUB = 0x0D,
  LSC_ATOMIC_SMIN = 0x0E,
  LSC_ATOMIC_SMAX = 0x0F,
  LSC_ATOMIC_UMIN = 0x10,
  LSC_ATOMIC_UMAX = 0x11,
  LSC_ATOMIC_ICAS = 0x12,
  LSC_ATOMIC_FADD = 0x13,
  LSC_ATOMIC_FSUB = 0x14,
  LSC_ATOMIC_FMIN = 0x15,
  LSC_ATOMIC_FMAX = 0x16,
  LSC_ATOMIC_FCAS = 0x17,
  LSC_ATOMIC_AND = 0x18,
  LSC_ATOMIC_OR = 0x19,
  LSC_ATOMIC_XOR = 0x1A,
  //
  LSC_LOAD_STATUS = 0x1B,
  LSC_STORE_UNCOMPRESSED = 0x1C,
  LSC_CCS = 0x1D,
  //
  LSC_RSI = 0x1E,
  LSC_FENCE = 0x1F,
  //
  LSC_STORE_UNCOMPRESSED_QUAD = 0x20,
  //
  LSC_ATOMIC_APNDADD = 0x28,
  LSC_ATOMIC_APNDSUB = 0x29,
  LSC_ATOMIC_APNDSTORE = 0x2A,
  LSC_LOAD_QUAD_MSRT = 0x31,
  LSC_STORE_QUAD_MSRT = 0x32,
  //
  LSC_INVALID = 0xFFFFFFFF,
};

static const uint32_t LSC_AT_FLAT = 0x0;
static const uint32_t LSC_AT_BSS = 0x1;
static const uint32_t LSC_AT_SS = 0x2;
static const uint32_t LSC_AT_BTI = 0x3;

static const uint32_t LSC_A16 = 0x1;
static const uint32_t LSC_A32 = 0x2;
static const uint32_t LSC_A64 = 0x3;

static const uint32_t LSC_D8 = 0x0;
static const uint32_t LSC_D16 = 0x1;
static const uint32_t LSC_D32 = 0x2;
static const uint32_t LSC_D64 = 0x3;
static const uint32_t LSC_D8U32 = 0x4;
static const uint32_t LSC_D16U32 = 0x5;
static const uint32_t LSC_D16U32H = 0x6;

static const uint32_t LSC_V1 = 0x0;
static const uint32_t LSC_V2 = 0x1;
static const uint32_t LSC_V3 = 0x2;
static const uint32_t LSC_V4 = 0x3;
static const uint32_t LSC_V8 = 0x4;
static const uint32_t LSC_V16 = 0x5;
static const uint32_t LSC_V32 = 0x6;
static const uint32_t LSC_V64 = 0x7;

///////////////////////////////////////////////////////
// Cache Opt
// Value for bits[19:17]
static const uint32_t LSC_DF_DF = 0x0;
//
static const uint32_t LSC_UC_UC = 0x1;
//
static const uint32_t LSC_UC_CA = 0x2;
static const uint32_t LSC_UC_WB = 0x2;
//
static const uint32_t LSC_CA_UC = 0x3;
static const uint32_t LSC_WT_UC = 0x3;
//
static const uint32_t LSC_CA_CA = 0x4;
static const uint32_t LSC_WT_WB = 0x4;
//
static const uint32_t LSC_ST_UC = 0x5;
//
static const uint32_t LSC_ST_CA = 0x6;
static const uint32_t LSC_ST_WB = 0x6;
//
static const uint32_t LSC_RI_CA = 0x7;
static const uint32_t LSC_WB_WB = 0x7;

// Value for bits [19:16]
static const uint32_t LSC_UC_CC = 0x5;
static const uint32_t LSC_CA_CC = 0x9;
static const uint32_t LSC_RI_RI = 0xE;

// This handles LSC messages only
struct MessageDecoderLSC : MessageDecoder {
  MessageDecoderLSC(Platform _platform, SFID _sfid, ExecSize _execSize,
                    uint32_t exImmOffDesc, SendDesc _exDesc, SendDesc _desc,
                    DecodeResult &_result)
      : MessageDecoder(_platform, _sfid, _execSize, exImmOffDesc, _exDesc,
                       _desc, _result) {}

  // used by decodeLscMessage and subchildren
  std::string dataTypePrefixSyntax; // e.g. d32 or d16 or d32
  std::string vectorSuffixSyntax;   // e.g. x16t (for d16x16t) or .yzw
  std::string addrSizeSyntax;       // e.g. a32
  std::string cacheControlSyntax;   // e.g. ca.ca

  SendOp op = SendOp::INVALID;
  //
  int expectedExecSize = 1;
  //
  int addrSizeBits = 0;
  int dataSizeRegBits = 0, dataSizeMemBits = 0;
  int vectorSize = 1;
  MessageInfo::Attr extraAttrs = MessageInfo::Attr::NONE;

  // the symbol to return in the MessageInfo structure
  std::string symbolFromSyntax() const {
    std::stringstream sym;
    sym << result.syntax.mnemonic;
    if (!result.syntax.controls.empty())
      sym << result.syntax.controls;
    sym << "  ";
    if (!result.syntax.surface.empty())
      sym << result.syntax.surface;
    sym << "[";
    if (!result.syntax.scale.empty()) {
      sym << result.syntax.scale;
    }
    sym << "A";
    if (!result.syntax.immOffset.empty()) {
      sym << result.syntax.immOffset;
    }
    sym << "]";
    return sym.str();
  }

  ///////////////////////////////////////////////////////////////////////////
  void setCacheOpts(std::stringstream &sym, std::stringstream &descs,
                    CacheOpt &l1, CacheOpt &l3, CacheOpt _l1, CacheOpt _l3) {
    l1 = _l1;
    l3 = _l3;
    if (_l1 == CacheOpt::DEFAULT && _l3 == CacheOpt::DEFAULT) {
      descs << "use state settings for both L1 and L3";
      return;
    }
    auto emitCacheOpt = [&](CacheOpt c) {
      sym << '.';
      switch (c) {
      case CacheOpt::DEFAULT:
        sym << "df";
        descs << " uses default state settings";
        break;
      case CacheOpt::READINVALIDATE:
        sym << "ri";
        descs << " read-invalidate (last use)";
        break;
      case CacheOpt::CACHED:
        sym << "ca";
        descs << " cached";
        break;
      case CacheOpt::STREAMING:
        sym << "st";
        descs << " streaming";
        break;
      case CacheOpt::UNCACHED:
        sym << "uc";
        descs << " uncached (bypass)";
        break;
      case CacheOpt::WRITETHROUGH:
        sym << "wt";
        descs << " writethrough";
        break;
      case CacheOpt::WRITEBACK:
        sym << "wb";
        descs << " writeback";
        break;
      case CacheOpt::CONSTCACHED:
        sym << "cc";
        descs << " constant-cached";
        break;
      default:
        sym << "?";
        descs << " invalid";
        break;
      }
    };
    descs << "L1";
    emitCacheOpt(_l1);
    descs << "; L3";
    emitCacheOpt(_l3);
    descs << "";
  }

  // Descriptor Bits[19:16]: 4 bits of cache control
  bool decodeLscCacheControlBits16_19(SendOp sop, CacheOpt &l1, CacheOpt &l3) {
    // check if it's the 3 cases added for 4-bits Cache Control at [19:16]:
    // L1UC_L3CC, L1CA_L3CC, L1RI_L3RI
    std::stringstream sym, descs;
    l1 = l3 = CacheOpt::DEFAULT;
    bool isLoad = lookupSendOp(sop).isLoad();
    auto ccBits = getDescBits(16, 4);
    auto setCacheOptsWrapper = [&](CacheOpt _l1, CacheOpt _l3) {
      return setCacheOpts(sym, descs, l1, l3, _l1, _l3);
    };

    switch (ccBits) {
    case LSC_UC_CC:
      if (isLoad) {
        setCacheOptsWrapper(CacheOpt::UNCACHED, CacheOpt::CONSTCACHED);
        break;
      }
    case LSC_CA_CC:
      if (isLoad) {
        setCacheOptsWrapper(CacheOpt::CACHED, CacheOpt::CONSTCACHED);
        break;
      }
    case LSC_RI_RI:
      if (isLoad) {
        setCacheOptsWrapper(CacheOpt::READINVALIDATE, CacheOpt::READINVALIDATE);
        break;
      }
    default:
      return decodeLscCacheControlBits17_19(sop, l1, l3);
    }
    //
    cacheControlSyntax = sym.str();
    //
    addField("Caching", 16, 4, ccBits, descs.str());
    return true;
  }

  void decodeCacheControl(SendOp sop, CacheOpt &l1, CacheOpt &l3) {
    if (platform() >= Platform::XE2) {
      if (!decodeLscCacheControlBits16_19(sop, l1, l3))
        error(16, 4, "invalid cache options");
      return;
    }

    if (!decodeLscCacheControlBits17_19(sop, l1, l3))
      error(17, 3, "invalid cache options");
  }

  // Descriptor Bits[19:17]: 3 bits of cache control
  bool decodeLscCacheControlBits17_19(SendOp sop, CacheOpt &l1, CacheOpt &l3) {
    std::stringstream sym, descs;
    l1 = l3 = CacheOpt::DEFAULT;
    bool isLoad = lookupSendOp(sop).isLoad();
    auto ccBits = getDescBits(17, 3);
    auto setCacheOptsWrapper = [&](CacheOpt _l1, CacheOpt _l3) {
      return setCacheOpts(sym, descs, l1, l3, _l1, _l3);
    };
    switch (ccBits) {
    case LSC_DF_DF:
      setCacheOptsWrapper(CacheOpt::DEFAULT, CacheOpt::DEFAULT);
      break;
    case LSC_UC_UC:
      setCacheOptsWrapper(CacheOpt::UNCACHED, CacheOpt::UNCACHED);
      break;
    case LSC_UC_CA: // == LSC_UC_WB
      if (isLoad)
        setCacheOptsWrapper(CacheOpt::UNCACHED, CacheOpt::CACHED);
      else
        setCacheOptsWrapper(CacheOpt::UNCACHED, CacheOpt::WRITEBACK);
      break;
    case LSC_CA_UC: // == LSC_WT_UC
      if (isLoad)
        setCacheOptsWrapper(CacheOpt::CACHED, CacheOpt::UNCACHED);
      else
        setCacheOptsWrapper(CacheOpt::WRITETHROUGH, CacheOpt::UNCACHED);
      break;
    case LSC_CA_CA: // == LSC_WT_WB
      if (isLoad)
        setCacheOptsWrapper(CacheOpt::CACHED, CacheOpt::CACHED);
      else
        setCacheOptsWrapper(CacheOpt::WRITETHROUGH, CacheOpt::WRITEBACK);
      break;
    case LSC_ST_UC:
      setCacheOptsWrapper(CacheOpt::STREAMING, CacheOpt::UNCACHED);
      break;
    case LSC_ST_CA: // == LSC_ST_WB
      if (isLoad)
        setCacheOptsWrapper(CacheOpt::STREAMING, CacheOpt::CACHED);
      else
        setCacheOptsWrapper(CacheOpt::STREAMING, CacheOpt::WRITEBACK);
      break;
    case LSC_RI_CA:
      if (isLoad) {
        // atomic follows store semantics, so compare against load
        setCacheOptsWrapper(CacheOpt::READINVALIDATE, CacheOpt::CACHED);
      } else {
        setCacheOptsWrapper(CacheOpt::WRITEBACK, CacheOpt::WRITEBACK);
      }
      break;
    default:
      return false;
    }
    //
    cacheControlSyntax = sym.str();
    //
    addField("Caching", 17, 3, ccBits, descs.str());
    return true;
  }

  void decodeLscImmOff(uint32_t atBits) {
    bool exDescHasImmOff = platform() >= Platform::XE2;
    if (!exDescHasImmOff)
      return;
    bool isBlock2d = op == SendOp::LOAD_BLOCK2D || op == SendOp::STORE_BLOCK2D;
    // immediate offset bits go in SendDesc if ExDesc is imm
    // if ExDesc is Reg then they go in dedicated exImmOffDesc
    uint32_t baseOffBits = exDesc.isReg() ? exImmOffDesc : exDesc.imm;
    if (atBits == LSC_AT_FLAT) {
      // [31:12] are all S20
      if (isBlock2d) {
        addDoc(DocRef::EXDESC, "DP_EXT_DESC_2D", "67780");
        // ExDescImm[31:22] = XOff (S10)
        // ExDescImm[21:12] = YOff (S10)
        result.info.immediateOffsetBlock2dX =
            (int)getSignedBits((int)baseOffBits, 12, 10);
        result.info.immediateOffsetBlock2dY =
            (int)getSignedBits((int)baseOffBits, 22, 10);
        addField("ImmediateOffsetBlock2dX", 22 + 32, 10,
                 (uint32_t)result.info.immediateOffsetBlock2dX,
                 fmtHexSigned(result.info.immediateOffsetBlock2dX));
        addField("ImmediateOffsetBlock2dY", 12 + 32, 10,
                 (uint32_t)result.info.immediateOffsetBlock2dY,
                 fmtHexSigned(result.info.immediateOffsetBlock2dY));
        if (result.info.immediateOffsetBlock2dX != 0 ||
            result.info.immediateOffsetBlock2dY != 0) {
          result.syntax.immOffset =
              std::string("+(") +
              fmtHexSigned(result.info.immediateOffsetBlock2dX) + "," +
              fmtHexSigned(result.info.immediateOffsetBlock2dY) + ")";
        }
      } else {
        addDoc(DocRef::EXDESC, "EXDESC_FLAT", "64012");
        result.info.immediateOffset = getSignedBits((int)baseOffBits, 12, 20);
        if (result.info.immediateOffset > 0) {
          result.syntax.immOffset =
              "+" + fmtHexSigned(result.info.immediateOffset);
        } else if (result.info.immediateOffset < 0) {
          result.syntax.immOffset = fmtHexSigned(result.info.immediateOffset);
        }
        addField("ImmediateOffset", 32 + 12, 20,
                 (uint32_t)result.info.immediateOffset,
                 result.syntax.immOffset);
      }

    } else if ((atBits == LSC_AT_BSS || atBits == LSC_AT_SS) &&
               exDesc.isReg() && sfid == SFID::TGM) {
      addDoc(DocRef::EXDESC, "EXDESC_SURFACE", "64017");
    } else if ((atBits == LSC_AT_BSS || atBits == LSC_AT_SS) &&
               exDesc.isReg() && sfid == SFID::UGM) {
      addDoc(DocRef::EXDESC, "EXDESCIMM_BSSO_SSO", "70586");
      // SS/BSS 16b or 17b (UGM)
      // ExDescImm[31:19] are Offset[16:4]
      // ExDescImm[18:16] are unmapped (addr reg)
      // ExDescImm[15:12] are Offset[3:0]
      if (sfid != SFID::UGM && baseOffBits != 0) {
        error(32, 32, "bss/ss: immediate offset forbidden for non-ugm");
      }
      uint32_t packed = ((getBits(baseOffBits, 19, 31 - 19 + 1) << 4) |
                         getBits(baseOffBits, 12, 3 - 0 + 1));
      result.info.immediateOffset = getSignedBits(packed, 0, 17);
      if (result.info.immediateOffset > 0) {
        result.syntax.immOffset =
            "+" + fmtHexSigned(result.info.immediateOffset);
      } else if (result.info.immediateOffset < 0) {
        result.syntax.immOffset = fmtHexSigned(result.info.immediateOffset);
      }
      addField("ImmediateOffset[16:4]", 32 + 19, 16 - 4 + 1,
               getBits(packed, 4, 16 - 4 + 1), result.syntax.immOffset);
      addField("ImmediateOffset[3:0]", 32 + 12, 3 - 0 + 1,
               getBits(packed, 0, 3 - 0 + 1), result.syntax.immOffset);
      addField("Reserved", 32 + 16, 3, getBits(baseOffBits, 16, 3),
               "a0.subreg");
      if (getBits(baseOffBits, 16, 3)) {
        error(32 + 16, 3, "ExDesc[19:16] must be zeros");
      }
    } else if (atBits == LSC_AT_BSS || atBits == LSC_AT_SS) { // bss/ss imm
      addDoc(DocRef::EXDESC, "EXDESC_SURFACE", "64017");
    } else if (atBits == LSC_AT_BTI && exDesc.isReg()) {
      if (exImmOffDesc != 0)
        error(32 + 12, 12, "immediate offset forbidden for BTI reg");
    } else if (atBits == LSC_AT_BTI) {
      addDoc(DocRef::EXDESC, "EXDESC_BTI", "63997");
      // ExDesc[23:12] is immm off
      result.info.immediateOffset =
          getSignedBits((int32_t)baseOffBits, 12, 23 - 12 + 1);
      if (result.info.immediateOffset > 0) {
        result.syntax.immOffset =
            "+" + fmtHexSigned(result.info.immediateOffset);
      } else if (result.info.immediateOffset < 0) {
        result.syntax.immOffset = fmtHexSigned(result.info.immediateOffset);
      }
      addField("ImmediateOffset[11:0]", 32 + 12, 23 - 12 + 1,
               result.info.immediateOffset, result.syntax.immOffset);
    }  // else: BSS/SS with imm ex desc
  }

  static const int ADDRTYPE_LOC = 29;

  AddrType decodeLscAddrType(SendDesc &surfId, bool allowsFlat = true) {
    surfId = 0;
    AddrType addrType = AddrType::FLAT;
    //
    const char *addrTypeMeaning = "?";
    //
    const auto atBits = getDescBits(ADDRTYPE_LOC, 2);
    //
    std::stringstream surfSyntax;
    switch (atBits) {
    case LSC_AT_FLAT:
      addrTypeMeaning = "Flat";
      addrType = AddrType::FLAT;
      if (!allowsFlat)
        error(ADDRTYPE_LOC, 2, "this message may not use FLAT address type");
      break;
    case LSC_AT_BSS:
    case LSC_AT_SS:
      if (atBits == LSC_AT_BSS) {
        addrTypeMeaning = "BSS";
        addrType = AddrType::BSS;
        surfSyntax << "bss";
      } else {
        addrTypeMeaning = "SS";
        addrType = AddrType::SS;
        surfSyntax << "ss";
      }
      if (exDesc.isImm()) {
        // XeHPG/XeHPC: we can pull this value out of ExDesc[31:11]
        int exDescOff = 11, len = 31 - exDescOff + 1;
        surfId = getDescBits(32 + exDescOff, len) << exDescOff;
        addField("SurfaceStateOffset", exDescOff, len, surfId.imm,
                 "immediate surface state offset");
        surfSyntax << "[" << iga::fmtHex(surfId.imm) << "]";
      } else {
        // XeHPG/XeHPC with reg surface state offset
        surfSyntax << "[a0." << (int)exDesc.reg.subRegNum << "]";
        surfId = exDesc;
      }
      break;
    case LSC_AT_BTI:
      addrTypeMeaning = "BTI";
      addrType = AddrType::BTI;
      if (exDesc.isImm()) {
        uint32_t bti = decodeExDescField(
            "BTI", 24, 8, [&](std::stringstream &ss, uint32_t bti) {
              ss << "bti[" << bti << "]";
            });
        surfSyntax << "bti[" << bti << "]";
        surfId = bti;
      } else {
        surfSyntax << "bti[a0." << (int)exDesc.reg.subRegNum << "]";
        surfId = exDesc;
      }
      break;
    default:
      addrTypeMeaning = "INVALID AddrType";
      addrType = AddrType::FLAT;
      surfSyntax << "?";
      error(ADDRTYPE_LOC, 2, "invalid address type");
      break;
    }
    result.syntax.surface = surfSyntax.str();

    /////////////////////////////
    // immediate offset
    decodeLscImmOff(atBits);
    //
    addField("AddrType", ADDRTYPE_LOC, 2, atBits, addrTypeMeaning);
    //
    return addrType;
  }

  void decodeLscAddrSize() {
    int addrSzBits = getDescBits(7, 2); // [8:7]
    std::stringstream asym;
    const char *aDesc = "";
    switch (addrSzBits) {
    case 1:
      asym << "a16";
      aDesc = "addresses are 16b";
      addrSizeBits = 16;
      break;
    case 2:
      asym << "a32";
      aDesc = "addresses are 32b";
      addrSizeBits = 32;
      break;
    case 3:
      asym << "a64";
      aDesc = "addresses are 64b";
      addrSizeBits = 64;
      break;
    default:
      asym << "a???";
      aDesc = "address size is invalid";
      error(7, 2, "invalid address size");
      break;
    }
    // result.syntax.addressType = ":" + asym.str();

    addrSizeSyntax = asym.str();
    //
    addField("AddrSize", 7, 2, addrSzBits, aDesc);
  }

  void decodeLscDataSize() {
    std::stringstream dsym;
    dataSizeRegBits = dataSizeMemBits = 0;
    std::string meaning;
    auto dtBits = getDescBits(9, 3);
    switch (dtBits) { // dat size [11:9]
    case LSC_D8:
      dataSizeRegBits = dataSizeMemBits = 8;
      dsym << "d8";
      meaning = "8b per data element";
      break;
    case LSC_D16:
      dataSizeRegBits = dataSizeMemBits = 16;
      meaning = "16b per data element";
      dsym << "d16";
      break;
    case LSC_D32:
      dataSizeRegBits = dataSizeMemBits = 32;
      dsym << "d32";
      meaning = "32b per data element";
      break;
    case LSC_D64:
      dataSizeRegBits = dataSizeMemBits = 64;
      dsym << "d64";
      meaning = "64b per data element";
      break;
    case LSC_D8U32:
      dataSizeRegBits = 32;
      dataSizeMemBits = 8;
      dsym << "d8u32";
      meaning = "load 8b into the low 8b of 32b register elements "
                "(upper bits are undefined)";
      if (platform() >= Platform::XE2)
        meaning = "load 8b and zero-extend to 32b per data element";
      break;
    case LSC_D16U32:
      dataSizeRegBits = 32;
      dataSizeMemBits = 16;
      dsym << "d16u32";
      meaning = "load 16b into the low 16b of 32b register elements "
                "(upper bits are undefined)";
      if (platform() >= Platform::XE2)
        meaning = "load 16b and zero-extend to 32b per data element";
      break;
    case LSC_D16U32H:
      dataSizeRegBits = 32;
      dataSizeMemBits = 16;
      extraAttrs |= MessageInfo::Attr::EXPAND_HIGH;
      dsym << "d16u32h";
      meaning = "load 16b into the high half of 32b register elements";
      break;
    default:
      dsym << "0x" << std::uppercase << std::hex << dtBits;
      meaning = "???";
    }
    //
    // result.syntax.dataType = ":" + dsym.str();
    dataTypePrefixSyntax = dsym.str();

    addField("DataSize", 9, 3, dtBits, meaning);
  }

  void decodeLscVecSize() {
    if (lookupSendOp(op).hasChMask()) {
      decodeLscVecSizeQuad();
    } else {
      decodeLscVecSizeNormal();
    }
  }

  void decodeLscVecSizeNormal() {
    std::stringstream vsym;

    uint32_t vecSzEncd = getDescBits(12, 3); // [14:12]
    switch (vecSzEncd) {
    case LSC_V1:
      vectorSize = 1;
      break;
    case LSC_V2:
      vectorSize = 2;
      break;
    case LSC_V3:
      vectorSize = 3;
      break;
    case LSC_V4:
      vectorSize = 4;
      break;
    case LSC_V8:
      vectorSize = 8;
      break;
    case LSC_V16:
      vectorSize = 16;
      break;
    case LSC_V32:
      vectorSize = 32;
      break;
    case LSC_V64:
      vectorSize = 64;
      break;
    default:
      vsym << "x?";
    }
    bool opIsBlock2d =
        op == SendOp::LOAD_BLOCK2D || op == SendOp::STORE_BLOCK2D;
    auto transposed = decodeDescBitField(
        "DataOrder", 15,
        "non-transposed (vector elements are in successive registers)",
        "transposed (vector elements are in the same register)");
    if (vectorSize > 1 || (transposed && !opIsBlock2d)) {
      vsym << 'x' << vectorSize;
    }
    if (transposed && op == SendOp::LOAD_STATUS) {
      error(15, 1, "data order must be non-transposed for this op");
    }
    std::stringstream vdesc;
    vdesc << "each address accesses " << vectorSize << " element";
    if (vectorSize != 1)
      vdesc << "s";
    if (!opIsBlock2d)
      addField("VecSize", 12, 3, vecSzEncd, vdesc.str());
    if (transposed) {
      vsym << 't';
      extraAttrs |= MessageInfo::Attr::TRANSPOSED;
      expectedExecSize = 1; // all transpose messages are SIMD1
    }

    if (op == SendOp::LOAD_BLOCK2D) {
      bool vnni =
          decodeDescBitField("Block2dVnniTransform", 7, "disabled", "enabled");
      if (vnni)
        vsym << 'v';
    }

    vectorSuffixSyntax = vsym.str();
  }

  void decodeLscVecSizeQuad() {
    // LSC channels *enabled* is the inverse of the old messages
    // because the old ChMask used in untyped old (scatter4/gather4)
    // was really a channel "disable" mask
    auto chEn = getDescBits(12, 4);
    vectorSize = 0;
    for (int i = 0; i < 4; ++i) {
      if ((1 << i) & chEn) {
        vectorSize++;
      }
    }
    extraAttrs |= MessageInfo::Attr::HAS_CHMASK;

    std::stringstream vsym;
    vsym << ".";
    if (chEn & 1)
      vsym << "x";
    if (chEn & 2)
      vsym << "y";
    if (chEn & 4)
      vsym << "z";
    if (chEn & 8)
      vsym << "w";
    vectorSuffixSyntax = vsym.str();

    addField("CompEn", 12, 4, chEn, vsym.str());
  }

  ///////////////////////////////////////////////////////////////////////////
  void decodeLscMessage(const std::string& msgDesc, SendOp lscOp) {
    const std::string symbol = ToSyntax(lscOp);
    op = lscOp;

    const SendOpDefinition &opInfo = lookupSendOp(lscOp);
    bool opSupportsUvr = lscOp == SendOp::LOAD_QUAD ||
                         lscOp == SendOp::STORE_QUAD ||
                         lscOp == SendOp::LOAD_BLOCK2D ||
                         lscOp == SendOp::STORE_BLOCK2D ||
                         lscOp == SendOp::LOAD_QUAD_MSRT ||
                         lscOp == SendOp::STORE_QUAD_MSRT ||
                         lscOp == SendOp::STORE_UNCOMPRESSED_QUAD ||
                         opInfo.isAtomic();
    if (sfid == SFID::TGM && opSupportsUvr) {
      extraAttrs |= MessageInfo::Attr::HAS_UVRLOD;
    }

    addField("Opcode", 0, 6, getDescBits(0, 6), symbol);

    if (exDesc.isImm() && (exDesc.imm & 0x7FF)) {
      // bit 11 may or may not be available
      error(0, 12, "ExDesc[11:0] must be 0 on this platform");
    }
    //
    SendDesc surfaceId(0x0);
    AddrType addrType = decodeLscAddrType(surfaceId);
    //
    if (op == SendOp::LOAD_BLOCK2D || op == SendOp::STORE_BLOCK2D) {
      addrSizeBits = 64;
      addrSizeSyntax = "a64";
    } else if (op == SendOp::ATOMIC_ACADD || op == SendOp::ATOMIC_ACSUB ||
               op == SendOp::ATOMIC_ACSTORE) {
      addrSizeBits = 32;
      addrSizeSyntax = "a32";
    } else {
      decodeLscAddrSize();
    }
    //
    decodeLscDataSize();
    //
    expectedExecSize = op == SendOp::LOAD_BLOCK2D || op == SendOp::STORE_BLOCK2D
                           ? 1
                       : sfid == SFID::TGM ? DEFAULT_EXEC_SIZE / 2
                                           : DEFAULT_EXEC_SIZE;
    decodeLscVecSize();
    //
    if (sfid == SFID::TGM)
      extraAttrs |= MessageInfo::Attr::TYPED;
    if (sfid == SFID::SLM)
      extraAttrs |= MessageInfo::Attr::SLM;
    //
    CacheOpt l1 = CacheOpt::DEFAULT, l3 = CacheOpt::DEFAULT;
    bool hasCc = opInfo.isLoad() || opInfo.isStore() || opInfo.isAtomic();
    if (sfid != SFID::SLM && hasCc) {
      decodeCacheControl(op, l1, l3);
    }
    //
    result.syntax.mnemonic = symbol;
    //
    result.syntax.controls += '.';
    result.syntax.controls += dataTypePrefixSyntax;
    result.syntax.controls += vectorSuffixSyntax;
    if (!addrSizeSyntax.empty()) {
      result.syntax.controls += '.';
      result.syntax.controls += addrSizeSyntax;
    }
    if (!cacheControlSyntax.empty()) {
      result.syntax.controls += cacheControlSyntax;
    }
    //
    setScatterGatherOpX(symbolFromSyntax(), msgDesc, op, addrType, surfaceId,
                        l1, l3, addrSizeBits, dataSizeRegBits, dataSizeMemBits,
                        vectorSize, int(instExecSize), extraAttrs);
    if (lookupSendOp(op).hasChMask()) {
      result.info.channelsEnabled = getDescBits(12, 4);
      if (result.info.channelsEnabled == 0)
        error(12, 4, "no channels enabled on quad message");
    }
  }

  void decodeLscMessageTypeBlock2D(std::string msgDesc, SendOp lscOp) {
    const std::string symbol = ToSyntax(lscOp);
    op = lscOp;
    extraAttrs |= MessageInfo::Attr::HAS_UVRLOD;
    expectedExecSize = 1;
    addField("Opcode", 0, 6, getDescBits(0, 6), symbol);

    SendDesc surfaceId(0x0);
    AddrType addrType = decodeLscAddrType(surfaceId);
    if (addrType != AddrType::BSS && addrType != AddrType::BTI) {
      error(ADDRTYPE_LOC, 2, "addr surface type must be BSS or BTI");
    }

    addrSizeBits = 32;
    addrSizeSyntax = format("a", addrSizeBits);

    dataSizeRegBits = dataSizeMemBits = 32; // ?
    dataTypePrefixSyntax = format("d", dataSizeMemBits);

    CacheOpt l1 = CacheOpt::DEFAULT, l3 = CacheOpt::DEFAULT;
    decodeCacheControl(op, l1, l3);

    result.syntax.mnemonic = symbol;
    result.syntax.controls += '.';
    result.syntax.controls += dataTypePrefixSyntax;
    result.syntax.controls += '.';
    result.syntax.controls += addrSizeSyntax;
    if (!cacheControlSyntax.empty()) {
      result.syntax.controls += cacheControlSyntax;
    }
    setScatterGatherOpX(symbolFromSyntax(), msgDesc, op, addrType, surfaceId,
                        l1, l3, addrSizeBits, dataSizeRegBits, dataSizeMemBits,
                        vectorSize, expectedExecSize, extraAttrs);
  }

  void decodeLscAtomicMessage(std::string msgDesc, SendOp atOp) {
    extraAttrs |= getDescBits(20, 5) != 0 ? MessageInfo::Attr::ATOMIC_RETURNS
                                          : MessageInfo::Attr::NONE;
    if (sfid == SFID::TGM)
      extraAttrs |= MessageInfo::Attr::HAS_UVRLOD;
    decodeLscMessage(msgDesc, atOp);
    addDocRefsVecCmask();
  }

  // similar to decodeLscAtomicMessage, but we disable syntax since
  // we don't wish to support it yet
  void decodeLscAtomicAppendMessage(const char *msgDesc, SendOp atOp) {
    decodeLscAtomicMessage(msgDesc, atOp);
    result.syntax.layout = MessageSyntax::Layout::INVALID;
  }

  // load/store/atomic etc... (non-cmask)
  void decodeLscVectorCmask(const char *name, SendOp op) {
    decodeLscMessage(name, op);
    addDocRefsVecCmask();
  }
  void addDocRefsVecCmask() {
    if (!result.errors.empty()) {
      return;
    }
    ///// data payloads
    auto addDataPayload = [&](DocRef::Kind kind) {
      const auto nullaryAtomic = result.info.op == SendOp::ATOMIC_LOAD ||
                                 result.info.op == SendOp::ATOMIC_IINC ||
                                 result.info.op == SendOp::ATOMIC_IDEC;
      if (nullaryAtomic && kind == DocRef::SRC1)
        return; // no payload

      const auto binaryAtomicSrc =
          kind == DocRef::SRC1 && (result.info.op == SendOp::ATOMIC_ICAS ||
                                   result.info.op == SendOp::ATOMIC_FCAS);

      if (result.info.elemSizeBitsRegFile == 32) {
        if (binaryAtomicSrc) {
          if (result.info.execWidth <= 8 && platform() <= Platform::XE_HPG) {
            addDocXe(kind, "D32_2SRC_ATM_PAYLOAD_SIMT8", "55499", nullptr);
          } else if (result.info.execWidth <= 16) {
            addDocXe(kind, "D32_2SRC_ATM_PAYLOAD_SIMT16", "54696", "64003");
          } else if (result.info.execWidth <= 32) {
            addDocXe(kind, "D32_2SRC_ATM_PAYLOAD", "54693", "64002");
          }
        } else if (result.info.execWidth <= 8 &&
                   platform() <= Platform::XE_HPG) {
          addDocXe(kind, "D32_2SRC_ATM_PAYLOAD_SIMT8", "55499", nullptr);
        } else if (result.info.execWidth <= 8 &&
                   platform() <= Platform::XE_HPG) {
          addDocXe(kind, "D32_PAYLOAD_SIMT8", "55497", nullptr);
        } else if (result.info.execWidth <= 16) {
          addDocXe(kind, "D32_PAYLOAD_SIMT16", "54146", "64000");
        } else {
          addDocXe(kind, "D32_PAYLOAD", "53574", "63999");
        }
      } else if (result.info.elemSizeBitsRegFile == 64) {
        if (binaryAtomicSrc) {
          if (result.info.execWidth <= 8 && platform() <= Platform::XE_HPG) {
            addDocXe(kind, "D64_2SRC_ATM_PAYLOAD_SIMT8", "55498", nullptr);
          } else if (result.info.execWidth <= 16) {
            addDocXe(kind, "D64_2SRC_ATM_PAYLOAD_SIMT16", "54695", "64008");
          } else if (result.info.execWidth <= 32) {
            addDocXe(kind, "D64_2SRC_ATM_PAYLOAD", "54692", "64007");
          }
        } else if (result.info.execWidth <= 8 &&
                   platform() <= Platform::XE_HPG) {
          addDocXe(kind, "D64_PAYLOAD_SIMT8", "75252", nullptr);
        } else if (result.info.execWidth <= 16) {
          addDocXe(kind, "D64_PAYLOAD_SIMT16", "54147", "64006");
        } else {
          addDocXe(kind, "D64_PAYLOAD", "53575", "64005");
        }
      }
    };

    if (result.info.isLoad() || result.info.isAtomic()) {
      addDataPayload(DocRef::DST);
    }

    ///// address payloads
    if (op == SendOp::LOAD_STRIDED || op == SendOp::STORE_STRIDED) {
      addDocXe(DocRef::SRC0, "ABLOCK_PAYLOAD", "53571", "63996");
    } else if (result.info.isTransposed()) {
      addDocXe(DocRef::SRC0, "A64_PAYLOAD_SIMT1", "55190", "63994");
    } else if (result.info.addrSizeBits == 32) {
      if (result.info.execWidth <= 16 && platform() <= Platform::XE_HPG) {
        addDocXe(DocRef::SRC0, "A32_PAYLOAD_SIMT8", "55496", nullptr);
      } else if (result.info.execWidth <= 16) {
        addDocXe(DocRef::SRC0, "A32_PAYLOAD_SIMT16", "54148", "63991");
      } else {
        addDocXe(DocRef::SRC0, "A32_PAYLOAD", "53569", "63990");
      }
    } else if (result.info.addrSizeBits == 64) {
      if (result.info.execWidth <= 16 && platform() <= Platform::XE_HPG) {
        // pending an update
        addDocXe(DocRef::SRC0, "A64_PAYLOAD_SIMT8", "75251", nullptr);
      } else if (result.info.execWidth <= 16) {
        addDocXe(DocRef::SRC0, "A64_PAYLOAD_SIMT16", "54149", "63995");
      } else {
        addDocXe(DocRef::SRC0, "A64_PAYLOAD", "53570", "63993");
      }
    }

    // src1
    if (result.info.isAtomic() || result.info.isStore()) {
      addDataPayload(DocRef::SRC1);
    }
  } // addDocRefsVecCmask

  void tryDecodeLsc() {
    int lscOp = getDescBits(0, 6); // Opcode[5:0]
    switch (lscOp) {
    case LSC_LOAD:
      addDocRefDESC("DP_LOAD", "53523", "63970");
      decodeLscVectorCmask("gathering load", SendOp::LOAD);
      break;
    case LSC_STORE:
      addDocRefDESC("DP_STORE", "53524", "63980");
      decodeLscVectorCmask("scattering store", SendOp::STORE);
      break;
    case LSC_STORE_UNCOMPRESSED:
      addDocRefDESC("DP_STORE_UNCOMPRESSED", "53532", "63984");
      decodeLscMessage("scattering store uncompressed",
                       SendOp::STORE_UNCOMPRESSED);
      break;
    case LSC_STORE_UNCOMPRESSED_QUAD:
      addDocRefDESC("DP_STORE_UC_CMASK", "55224", "63985");
      decodeLscVectorCmask("store quad uncompressed",
                           SendOp::STORE_UNCOMPRESSED_QUAD);
      break;
    case LSC_LOAD_QUAD:
      addDocRefDESC("DP_LOAD_CMASK", "53527", "63977");
      decodeLscVectorCmask("quad load (a.k.a. load_cmask)", SendOp::LOAD_QUAD);
      break;
    case LSC_STORE_QUAD:
      addDocRefDESC("DP_STORE_CMASK", "53527", "63983");
      decodeLscVectorCmask("quad store (a.k.a. store_cmask)",
                           SendOp::STORE_QUAD);
      break;
    case LSC_LOAD_STRIDED:
      addDocRefDESC("DP_LOAD_BLOCK", "53525", "63976");
      decodeLscVectorCmask("strided load (a.k.a load_block)",
                           SendOp::LOAD_STRIDED);
      break;
    case LSC_STORE_STRIDED:
      addDocRefDESC("DP_STORE_BLOCK", "53526", "63982");
      decodeLscVectorCmask("strided store (a.k.a store_block)",
                           SendOp::STORE_STRIDED);
      break;
    case LSC_LOAD_BLOCK2D:
      if (sfid == SFID::TGM) {
        addDocRefDESCXE2("DP_LOAD_TYPED_2DBLOCK", "65280");
        addDocXe(DocRef::SRC0, "Typed_2DBlock_Payload", nullptr, "65282");
        decodeLscMessageTypeBlock2D("block2d load", SendOp::LOAD_BLOCK2D);
      } else {
        addDocRefDESC("DP_LOAD_2DBLOCK_ARRAY", "53680", "63972");
        addDocXe(DocRef::SRC0, "A2DBLOCK_PAYLOAD", "53567", "63986");
        decodeLscMessage("block2d load", SendOp::LOAD_BLOCK2D);
      }
      break;
    case LSC_STORE_BLOCK2D:
      if (sfid == SFID::TGM) {
        addDocRefDESCXE2("DP_STORE_TYPED_2DBLOCK", "65281");
        decodeLscMessageTypeBlock2D("block2d store", SendOp::STORE_BLOCK2D);
        break;
      } else {
        addDocRefDESC("DP_STORE_2DBLOCK", "53530", "63981");
        decodeLscMessage("block2d store", SendOp::STORE_BLOCK2D);
      }
      break;
    case LSC_ATOMIC_IINC:
      addDocRefDESC("DP_ATOMIC_INC", "53538", "63955");
      decodeLscAtomicMessage("atomic integer increment", SendOp::ATOMIC_IINC);
      break;
    case LSC_ATOMIC_IDEC:
      addDocRefDESC("DP_ATOMIC_DEC", "53539", "63949");
      decodeLscAtomicMessage("atomic integer decrement", SendOp::ATOMIC_IDEC);
      break;
    case LSC_ATOMIC_LOAD:
      addDocRefDESC("DP_ATOMIC_LOAD", "53540", "63956");
      decodeLscAtomicMessage("atomic load", SendOp::ATOMIC_LOAD);
      break;
    case LSC_ATOMIC_STORE:
      addDocRefDESC("DP_ATOMIC_STORE", "53541", "63960");
      decodeLscAtomicMessage("atomic store", SendOp::ATOMIC_STORE);
      break;
    case LSC_ATOMIC_IADD:
      addDocRefDESC("DP_ATOMIC_ADD", "53542", "63946");
      decodeLscAtomicMessage("atomic integer add", SendOp::ATOMIC_IADD);
      break;
    case LSC_ATOMIC_ISUB:
      addDocRefDESC("DP_ATOMIC_SUB", "53543", "63961");
      decodeLscAtomicMessage("atomic integer subtract", SendOp::ATOMIC_ISUB);
      break;
    case LSC_ATOMIC_SMIN:
      addDocRefDESC("DP_ATOMIC_MIN", "53544", "63958");
      decodeLscAtomicMessage("atomic signed-integer minimum", SendOp::ATOMIC_SMIN);
      break;
    case LSC_ATOMIC_SMAX:
      addDocRefDESC("DP_ATOMIC_MAX", "53545", "63957");
      decodeLscAtomicMessage("atomic signed-integer maximum", SendOp::ATOMIC_SMAX);
      break;
    case LSC_ATOMIC_UMIN:
      addDocRefDESC("DP_ATOMIC_UMIN", "53546", "63963");
      decodeLscAtomicMessage("atomic unsigned-integer minimum",
                          SendOp::ATOMIC_UMIN);
      break;
    case LSC_ATOMIC_UMAX:
      addDocRefDESC("DP_ATOMIC_UMAX", "53547", "63962");
      decodeLscAtomicMessage("atomic unsigned-integer maximum",
                          SendOp::ATOMIC_UMAX);
      break;
    case LSC_ATOMIC_ICAS:
      addDocRefDESC("DP_ATOMIC_CMPXCHG", "53555", "63948");
      decodeLscAtomicMessage("atomic integer compare and swap",
                          SendOp::ATOMIC_ICAS);
      break;
    case LSC_ATOMIC_FADD:
      addDocRefDESC("DP_ATOMIC_FADD", "53548", "63950");
      decodeLscAtomicMessage("atomic float add", SendOp::ATOMIC_FADD);
      break;
    case LSC_ATOMIC_FSUB:
      addDocRefDESC("DP_ATOMIC_FSUB", "53549", "63954");
      decodeLscAtomicMessage("atomic float subtract", SendOp::ATOMIC_FSUB);
      break;
    case LSC_ATOMIC_FMIN:
      addDocRefDESC("DP_ATOMIC_FMIN", "53550", "63953");
      decodeLscAtomicMessage("atomic float minimum", SendOp::ATOMIC_FMIN);
      break;
    case LSC_ATOMIC_FMAX:
      addDocRefDESC("DP_ATOMIC_FMAX", "53551", "63952");
      decodeLscAtomicMessage("atomic float maximum", SendOp::ATOMIC_FMAX);
      break;
    case LSC_ATOMIC_FCAS:
      addDocRefDESC("DP_ATOMIC_FCMPXCHG", "DP_XXX", "63951");
      decodeLscAtomicMessage("atomic float compare and swap", SendOp::ATOMIC_FCAS);
      break;
    case LSC_ATOMIC_AND:
      addDocRefDESC("DP_ATOMIC_AND", "53552", "63947");
      decodeLscAtomicMessage("atomic logical and", SendOp::ATOMIC_AND);
      break;
    case LSC_ATOMIC_OR:
      addDocRefDESC("DP_ATOMIC_OR", "53553", "63959");
      decodeLscAtomicMessage("atomic logical or", SendOp::ATOMIC_OR);
      break;
    case LSC_ATOMIC_XOR:
      addDocRefDESC("DP_ATOMIC_XOR", "53554", "63964");
      decodeLscAtomicMessage("atomic logical xor", SendOp::ATOMIC_XOR);
      break;
    case LSC_ATOMIC_APNDADD:
      addDocRefDESCXE2("DP_APPENDCTR_ATOMIC_ADD", "68353");
      decodeLscAtomicAppendMessage("atomic counter append add",
                                   SendOp::ATOMIC_ACADD);
      break;
    case LSC_ATOMIC_APNDSUB:
      addDocRefDESCXE2("DP_APPENDCTR_ATOMIC_SUB", "68353");
      decodeLscAtomicAppendMessage("atomic counter append sub",
                                   SendOp::ATOMIC_ACSUB);
      break;
    case LSC_ATOMIC_APNDSTORE:
      addDocRefDESCXE2("DP_APPENDCTR_ATOMIC_ST", "68354");
      decodeLscAtomicAppendMessage("atomic counter append store",
                                   SendOp::ATOMIC_ACSTORE);
      break;
    case LSC_LOAD_QUAD_MSRT:
      addDocRefDESCXE2("DP_LOAD_CMASK_MSRT", "73733");
      decodeLscMessage("load quad from multi-sample render target",
                       SendOp::LOAD_QUAD_MSRT);
      break;
    case LSC_STORE_QUAD_MSRT:
      addDocRefDESCXE2("DP_STORE_CMASK_MSRT", "73734");
      decodeLscMessage("store quad to multi-sample render target",
                       SendOp::STORE_QUAD_MSRT);
      break;
    case LSC_CCS:
      decodeLscCcs();
      break;
    case LSC_RSI: {
      addDocRefDESC("DP_RSI", "54000", "63979");
      addDocXe(DocRef::DST, "DP_STATE_INFO_PAYLOAD", "54152", "64015");
      addDocXe(DocRef::SRC0, "DP_ASTATE_INFO_PAYLOAD", "55018",  "64014");

      addField("Opcode", 0, 6, getDescBits(0, 6), "read_state");
      //
      std::stringstream descs;
      descs << "read state information";
      result.syntax.mnemonic = "read_state";
      //
      SendDesc surfId = 0;
      auto at = decodeLscAddrType(surfId, false);
      //
      // XeHPG returns 2 GRF, XeHPC+ only 1
      // #54152
      int rlen = platform() == Platform::XE_HPG ? 2 : 1;
      setSpecialOpX(result.syntax.mnemonic, descs.str(), SendOp::READ_STATE, at,
                    surfId,
                    1, // mlen = 1 (U,V,R,LOD)
                    rlen);
      result.info.addrSizeBits = 64;
      result.info.execWidth = 1;
      result.info.attributeSet |= MessageInfo::Attr::HAS_UVRLOD;
      result.info.attributeSet |= MessageInfo::Attr::TRANSPOSED;
      break;
    }
    case LSC_FENCE:
      addDocRefDESC("DP_FENCE", "53533", "63969");
      decodeLscFence();
      break;
    case LSC_LOAD_STATUS:
      addDocRefDESC("DP_LOAD_STATUS", "53531", "63978");
      addDocXe(DocRef::DST, "DP_STATUS_PAYLOAD", "55018", "64016");
      if (getDescBit(15)) {
        error(15, 1, "transpose forbidden on load_status");
      }
      if (getDescBits(20, 5) != 1) {
        error(20, 5, "load_status must have rlen (Desc[24:20] == 1)");
      }
      decodeLscMessage("load status", SendOp::LOAD_STATUS);
      break;
    default:
      addField("Opcode", 0, 6, getDescBits(0, 6), "invalid message opcode");
      error(0, 6, "unsupported message opcode");
      return;
    }
  }

  void decodeLscCcs() {
    addField("Opcode", 0, 6, static_cast<uint32_t>(LSC_CCS),
             "compression-state control");
    //
    std::stringstream descs;
    result.syntax.mnemonic = "ccs";
    descs << "compression-state control";
    auto ccsOpBits = getDescBits(17, 3);
    SendOp sop = SendOp::INVALID;
    std::string opDesc;
    switch (ccsOpBits) {
    case 0:
      sop = SendOp::CCS_PC;
      result.syntax.mnemonic += "_pc";
      opDesc = " page clear (64k)";
      addDocRefDESC("DP_CCS_PAGE_CLEAR", "53536", "63965");
      break;
    case 1:
      sop = SendOp::CCS_SC;
      result.syntax.mnemonic += "_sc";
      opDesc = " sector clear (2-cachelines)";
      addDocRefDESC("DP_CCS_SEC_CLEAR", "53534", "63967");
      result.syntax.controls += vectorSuffixSyntax;
      break;
    case 2:
      sop = SendOp::CCS_PU;
      result.syntax.mnemonic += "_pu";
      opDesc = " page uncompress (64k)";
      addDocRefDESC("DP_CCS_PAGE_UNCOMPRESS", "53537", "63966");
      break;
    case 3:
      sop = SendOp::CCS_SU;
      result.syntax.mnemonic += "_su";
      opDesc = " sector uncompress (2-cachelines)";
      addDocRefDESC("DP_CCS_SEC_UNCOMPRESS", "53535", "63968");
      result.syntax.controls += vectorSuffixSyntax;
      break;
    default: {
      std::stringstream ss;
      ss << ".0x" << std::hex << std::uppercase << ccsOpBits;
      result.syntax.controls += ss.str();
      opDesc = "invalid ccs sop";
      error(17, 3, "invalid ccs sop");
    }
    } // switch
    descs << opDesc;
    //
    addField("CcsOp", 17, 3, ccsOpBits, opDesc);
    //
    SendDesc surfId = 0;
    auto at = decodeLscAddrType(surfId);
    if (ccsOpBits == 0 || ccsOpBits == 2) {
      // page operations: pc, pu
      if (at != AddrType::FLAT)
        error(29, 2, "ccs_{pcc,pcu} requires FLAT address type");
      std::stringstream dummy;
      decodeLscAddrSize();
      if (addrSizeBits != 64)
        error(7, 2, "AddrSize must be A64");
      result.info.execWidth = 1;
      expectedExecSize = 1;
      // sector uncompress has addresses
      // FIXME: I could derive this via exec size and a64
      int mlen =
          ccsOpBits == 1 || ccsOpBits == 3 ? 4 : // A64_PAYLOAD_SIMT32 = 4 regs
              1;                                 // A64_PAYLOAD_SIMT1  = 1 reg
      int rlen = 0;                              // always 0
      setSpecialOpX(symbolFromSyntax(), descs.str(), sop, at, surfId, mlen,
                    rlen);
    } else {
      // sector operations
      ///
      // these are vector messages
      expectedExecSize = DEFAULT_EXEC_SIZE;
      // const int SECTOR_SIZE_BITS = 128*8;
      // result.syntax.controls += ".d1024";
      result.syntax.controls += vectorSuffixSyntax;
      result.syntax.controls += addrSizeBits == 64 ? ".a64" : ".a32";
      //
      setScatterGatherOp(symbolFromSyntax(), descs.str(), sop, at, surfId,
                         addrSizeBits,
                         0, // dateSize = 0; nothing returned
                         vectorSize, DEFAULT_EXEC_SIZE, extraAttrs);
    }
  }

  void decodeLscFence() {
    addField("Opcode", 0, 6, getDescBits(0, 6), "fence");
    //
    std::stringstream descs;
    result.syntax.mnemonic = "fence";
    descs << "fence";
    //
    std::stringstream fenceOpts;
    addLscFenceFields(fenceOpts, descs);
    result.syntax.controls += fenceOpts.str();
    //
    setSpecialOpX(symbolFromSyntax(), descs.str(), SendOp::FENCE,
                  AddrType::FLAT,
                  0,  // no surface
                  1,  // mlen = 1
                  0); // rlen = 0
  }
}; // MessageDecoderLSC

void iga::decodeDescriptorsLSC(Platform platform, SFID sfid, ExecSize execSize,
                               uint32_t exImmOffDesc, SendDesc exDesc,
                               SendDesc desc, DecodeResult &result) {
  MessageDecoderLSC md(platform, sfid, execSize, exImmOffDesc, exDesc, desc,
                       result);
  md.tryDecodeLsc();
}

// descriptor bits [19:17]: cache control
static bool encLdStVecCachingBits17_19(SendOp op, CacheOpt cachingL1,
                                       CacheOpt cachingL3, SendDesc &desc) {
  const auto &opInfo = lookupSendOp(op);
  bool isLd = opInfo.isLoad();
  bool isSt = opInfo.isStore();
  bool isAt = opInfo.isAtomic();
  bool isStAt = isSt || isAt;
  auto ccMatches = [&](CacheOpt l1, CacheOpt l3, uint32_t enc) {
    if (l1 == cachingL1 && l3 == cachingL3) {
      desc.imm |= enc << 17;
      return true;
    }
    return false;
  };
  bool matched =
      ccMatches(CacheOpt::DEFAULT, CacheOpt::DEFAULT, LSC_DF_DF) ||
      //
      ccMatches(CacheOpt::UNCACHED, CacheOpt::UNCACHED, LSC_UC_UC) ||
      //
      (isLd && ccMatches(CacheOpt::UNCACHED, CacheOpt::CACHED, LSC_UC_CA)) ||
      (isStAt &&
       ccMatches(CacheOpt::UNCACHED, CacheOpt::WRITEBACK, LSC_UC_WB)) ||
      //
      (isLd && ccMatches(CacheOpt::CACHED, CacheOpt::UNCACHED, LSC_CA_UC)) ||
      (isSt &&
       ccMatches(CacheOpt::WRITETHROUGH, CacheOpt::UNCACHED, LSC_WT_UC)) ||
      //
      (isLd && ccMatches(CacheOpt::CACHED, CacheOpt::CACHED, LSC_CA_CA)) ||
      (isSt &&
       ccMatches(CacheOpt::WRITETHROUGH, CacheOpt::WRITEBACK, LSC_WT_WB)) ||
      //
      ccMatches(CacheOpt::STREAMING, CacheOpt::UNCACHED, LSC_ST_UC) ||
      //
      (isLd && ccMatches(CacheOpt::STREAMING, CacheOpt::CACHED, LSC_ST_CA)) ||
      (isSt &&
       ccMatches(CacheOpt::STREAMING, CacheOpt::WRITEBACK, LSC_ST_WB)) ||
      //
      (isLd &&
       ccMatches(CacheOpt::READINVALIDATE, CacheOpt::CACHED, LSC_RI_CA)) ||
      (isSt && ccMatches(CacheOpt::WRITEBACK, CacheOpt::WRITEBACK, LSC_WB_WB));
  return matched;
}

// encLdStVecCachingBits17_19 - Encode 4-bits cache opt at descriptor[19:16]
// Cache opt on XE2+ is 4 bits field comparing to 3 bits on pre-XE2
// XE2 is [19:16] and preXE2 is [19:17]
static bool encLdStVecCachingBits16_19(SendOp op, CacheOpt cachingL1,
                                       CacheOpt cachingL3, SendDesc &desc) {
  // special handle L1RI_L3CA which is not allowed in 4-bits-cache-opt mode
  if (cachingL1 == CacheOpt::READINVALIDATE &&
      cachingL3 == CacheOpt::CONSTCACHED)
    return false;

  // bits in [19:17] are the same as encLdStVecCachingBits17_19
  if (encLdStVecCachingBits17_19(op, cachingL1, cachingL3, desc))
    return true;

  // otherwise, check the new combination (all for load only)
  // L1UC_L3CC, L1CA_L3CC, L1RI_L3RI
  const auto &opInfo = lookupSendOp(op);
  bool isLd = opInfo.isLoad();
  auto ccMatches = [&](CacheOpt l1, CacheOpt l3, uint32_t enc) {
    if (l1 == cachingL1 && l3 == cachingL3) {
      desc.imm |= enc << 16;
      return true;
    }
    return false;
  };
  bool matched =
      (isLd &&
       ccMatches(CacheOpt::UNCACHED, CacheOpt::CONSTCACHED, LSC_UC_CC)) ||
      (isLd && ccMatches(CacheOpt::CACHED, CacheOpt::CONSTCACHED, LSC_CA_CC)) ||
      (isLd && ccMatches(CacheOpt::READINVALIDATE, CacheOpt::READINVALIDATE,
                         LSC_RI_RI));
  return matched;
}

static bool encLdStVecImmOffset(Platform p, const VectorMessageArgs &vma,
                                uint32_t &exImmOffDesc, SendDesc &exDesc,
                                std::string &err) {
  const bool isBlock2d =
      vma.op == SendOp::LOAD_BLOCK2D || vma.op == SendOp::STORE_BLOCK2D;
  auto fitsInBitSize = [](int bits, int offset) {
    return offset >= -(1 << (bits - 1)) && offset <= (1 << (bits - 1)) - 1;
  };
  uint32_t encodedOffset = 0;
  if (isBlock2d && vma.addrOffsetX * vma.dataSizeMem % 32 != 0) {
    err = "address offset-y must be 32b aligned";
    return false;
  } else if (isBlock2d && vma.addrOffsetY * vma.dataSizeMem % 32 != 0) {
    err = "address offset-x must be 32b aligned";
    return false;
  } else if (!isBlock2d && vma.addrOffset % 4 != 0) {
    err = "address offset must be 32b aligned";
    return false;
  } else if (vma.addrType == AddrType::FLAT) {
    if (isBlock2d) {
      if (vma.sfid != SFID::UGM) {
        err = "address offset only defined for flat ugm";
        return false;
      } else if (!fitsInBitSize(10, vma.addrOffsetX)) {
        err = "address offset-x only exceeds 10b";
        return false;
      } else if (!fitsInBitSize(10, vma.addrOffsetY)) {
        err = "address offset-y only exceeds 10b";
        return false;
      }
      // ExDescImm[31:22] = Y-offset
      // ExDescImm[21:12] = X-offset
      encodedOffset |= ((uint32_t)vma.addrOffsetY & 0x3FF) << 22;
      encodedOffset |= ((uint32_t)vma.addrOffsetX & 0x3FF) << 12;
    } else if (!fitsInBitSize(20, vma.addrOffset)) {
      err = "address offset exceeds 20b";
      return false;
    } else {
      // ExDescImm[31:12] = base offset
      encodedOffset = (uint32_t)vma.addrOffset << 12;
    }
  } else if (isBlock2d) {
    err = "block2d immediate offset only valid for flat";
    return false;
  } else if (vma.addrType == AddrType::BTI && !vma.addrSurface.isReg()) {
    if (!fitsInBitSize(12, vma.addrOffset)) {
      err = "bti address offset exceeds 12b";
      return false;
    }
    encodedOffset = ((uint32_t)(vma.addrOffset & 0x00FFF) << 12);
  } else if (vma.addrSurface.isReg()) {
    if (vma.sfid != SFID::UGM) {
      err = "unsupported SFID for ExDescReg + BaseOff";
      return false;
    } else if (!fitsInBitSize(17, vma.addrOffset)) {
      err = "address offset of ss/bss (.ugm) exceeds 17b";
      return false;
    }
    //
    // ExDesc[31:19] = BaseOffset[16:4]
    // ExDesc[15:12] = BaseOffset[3:0]
    encodedOffset |= getBits((uint32_t)vma.addrOffset, 4, 13) << 19;
    encodedOffset |= getBits((uint32_t)vma.addrOffset, 0, 4) << 12;
  } else {
    // BSS/SS with imm offset
    err = "address offset forbidden bti/ss/bss with imm ExDesc surface";
    return false;
  }
  // needs to write to exImmOffDesc only if ExDesc.IsReg
  if (vma.addrSurface.isReg()) {
    exImmOffDesc = encodedOffset;
  } else {
    exDesc = encodedOffset;
  }
  return true;
}

static bool encLdStVecCaching(const Platform &p, SendOp op, CacheOpt cachingL1,
                              CacheOpt cachingL3, SendDesc &desc) {
  if (p >= Platform::XE2)
    return encLdStVecCachingBits16_19(op, cachingL1, cachingL3, desc);

  return encLdStVecCachingBits17_19(op, cachingL1, cachingL3, desc);
}

static bool encLdStVec(Platform p, const VectorMessageArgs &vma,
                       uint32_t &exImmOffDesc,
                       SendDesc &exDesc, SendDesc &desc, std::string &err) {
  desc = 0x0;
  exDesc = 0x0;
  //
  bool hasCMask = false;
  switch (vma.op) {
  case SendOp::LOAD:
    desc.imm |= LSC_LOAD;
    break;
  case SendOp::LOAD_STRIDED:
    desc.imm |= LSC_LOAD_STRIDED;
    break;
  case SendOp::LOAD_QUAD:
    desc.imm |= LSC_LOAD_QUAD;
    hasCMask = true;
    break;
  case SendOp::LOAD_BLOCK2D:
    desc.imm |= LSC_LOAD_BLOCK2D;
    break;
  //
  case SendOp::STORE:
    desc.imm |= LSC_STORE;
    break;
  case SendOp::STORE_STRIDED:
    desc.imm |= LSC_STORE_STRIDED;
    break;
  case SendOp::STORE_QUAD:
    desc.imm |= LSC_STORE_QUAD;
    hasCMask = true;
    break;
  case SendOp::STORE_UNCOMPRESSED:
    desc.imm |= LSC_STORE_UNCOMPRESSED;
    break;
  case SendOp::STORE_UNCOMPRESSED_QUAD:
    desc.imm |= LSC_STORE_UNCOMPRESSED_QUAD;
    hasCMask = true;
    break;
  case SendOp::STORE_BLOCK2D:
    desc.imm |= LSC_STORE_BLOCK2D;
    break;
  //
  case SendOp::ATOMIC_AND:
    desc.imm |= LSC_ATOMIC_AND;
    break;
  case SendOp::ATOMIC_FADD:
    desc.imm |= LSC_ATOMIC_FADD;
    break;
  case SendOp::ATOMIC_FCAS:
    desc.imm |= LSC_ATOMIC_FCAS;
    break;
  case SendOp::ATOMIC_FMAX:
    desc.imm |= LSC_ATOMIC_FMAX;
    break;
  case SendOp::ATOMIC_FMIN:
    desc.imm |= LSC_ATOMIC_FMIN;
    break;
  case SendOp::ATOMIC_FSUB:
    desc.imm |= LSC_ATOMIC_FSUB;
    break;
  case SendOp::ATOMIC_IADD:
    desc.imm |= LSC_ATOMIC_IADD;
    break;
  case SendOp::ATOMIC_ICAS:
    desc.imm |= LSC_ATOMIC_ICAS;
    break;
  case SendOp::ATOMIC_IDEC:
    desc.imm |= LSC_ATOMIC_IDEC;
    break;
  case SendOp::ATOMIC_IINC:
    desc.imm |= LSC_ATOMIC_IINC;
    break;
  case SendOp::ATOMIC_ISUB:
    desc.imm |= LSC_ATOMIC_ISUB;
    break;
  case SendOp::ATOMIC_LOAD:
    desc.imm |= LSC_ATOMIC_LOAD;
    break;
  case SendOp::ATOMIC_OR:
    desc.imm |= LSC_ATOMIC_OR;
    break;
  case SendOp::ATOMIC_SMAX:
    desc.imm |= LSC_ATOMIC_SMAX;
    break;
  case SendOp::ATOMIC_SMIN:
    desc.imm |= LSC_ATOMIC_SMIN;
    break;
  case SendOp::ATOMIC_STORE:
    desc.imm |= LSC_ATOMIC_STORE;
    break;
  case SendOp::ATOMIC_UMAX:
    desc.imm |= LSC_ATOMIC_UMAX;
    break;
  case SendOp::ATOMIC_UMIN:
    desc.imm |= LSC_ATOMIC_UMIN;
    break;
  case SendOp::ATOMIC_XOR:
    desc.imm |= LSC_ATOMIC_XOR;
    break;
  case SendOp::ATOMIC_ACADD:
    desc.imm |= LSC_ATOMIC_APNDADD;
    break;
  case SendOp::ATOMIC_ACSUB:
    desc.imm |= LSC_ATOMIC_APNDSUB;
    break;
  case SendOp::ATOMIC_ACSTORE:
    desc.imm |= LSC_ATOMIC_APNDSTORE;
    break;
  case SendOp::LOAD_QUAD_MSRT:
    desc.imm |= LSC_LOAD_QUAD_MSRT;
    break;
  case SendOp::STORE_QUAD_MSRT:
    desc.imm |= LSC_STORE_QUAD_MSRT;
    break;
  default:
    err = "unsupported op";
    return false;
  }
  bool isBlock2d =
      vma.op == SendOp::LOAD_BLOCK2D || vma.op == SendOp::STORE_BLOCK2D;
  bool isBlock2dTyped = isBlock2d && vma.sfid == SFID::TGM;
  bool isBlock2dUntyped = isBlock2d && vma.sfid != SFID::TGM;
  bool hasAddrSizeField = !isBlock2d;
  bool isAtomicAddCounter = vma.op == SendOp::ATOMIC_ACADD ||
                            vma.op == SendOp::ATOMIC_ACSUB ||
                            vma.op == SendOp::ATOMIC_ACSTORE;
  hasAddrSizeField &= !isAtomicAddCounter;

  //
  ////////////////////////////////////////
  // data size
  uint32_t dszEnc = LSC_D8;
  if (isBlock2dTyped && (vma.dataSizeReg != 32 || vma.dataSizeMem != 32)) {
    err = "block2d.tgm must be d32";
    return false;
  }
  if (vma.dataSizeMem == vma.dataSizeReg) {
    switch (vma.dataSizeMem) {
    case 8:
      dszEnc = LSC_D8;
      break;
    case 16:
      dszEnc = LSC_D16;
      break;
    case 32:
      dszEnc = LSC_D32;
      break;
    case 64:
      dszEnc = LSC_D64;
      break;
    default:
      err = "invalid data size";
      return false;
    }
  } else if (vma.dataSizeMem == 8 && vma.dataSizeReg == 32) {
    dszEnc = LSC_D8U32;
  } else if (vma.dataSizeMem == 16 && vma.dataSizeReg == 32) {
    if (vma.dataSizeExpandHigh) {
      dszEnc = LSC_D16U32H;
    } else {
      dszEnc = LSC_D16U32;
    }
  } else {
    err = "invalid data type";
    return false;
  }
  if (!isBlock2dTyped)
    desc.imm |= dszEnc << 9;
  //
  ////////////////////////////////////////
  // vector size
  if (hasCMask) {
    if (vma.dataComponentMask & ~0xF) {
      err = "invalid component mask";
      return false;
    }
    desc.imm |= vma.dataComponentMask << 12;
  } else if (isBlock2d) {
    if (isBlock2dTyped && vma.dataVnni) {
      err = "block2d.tgm forbids VNNI";
      return false;
    } else if (isBlock2dTyped && vma.dataTranspose) {
      err = "block2d.tgm forbids transpose data order";
      return false;
    }
    if (vma.dataVnni)
      desc.imm |= 1 << 7;
    if (vma.dataTranspose)
      desc.imm |= 1 << 15;
  } else {
    uint32_t vecEnc = LSC_V1;
    switch (vma.dataVectorSize) {
    case 1:
      vecEnc = LSC_V1;
      break;
    case 2:
      vecEnc = LSC_V2;
      break;
    case 3:
      vecEnc = LSC_V3;
      break;
    case 4:
      vecEnc = LSC_V4;
      break;
    case 8:
      vecEnc = LSC_V8;
      break;
    case 16:
      vecEnc = LSC_V16;
      break;
    case 32:
      vecEnc = LSC_V32;
      break;
    case 64:
      vecEnc = LSC_V64;
      break;
    default:
      err = "invalid vector size";
      break;
    }
    if (vma.isAtomic() && vma.dataVectorSize != 1) {
      err = "atomics do not support vector operations";
      return false;
    }
    if (vma.dataVnni) {
      err = "vnni only valid on block2d operations";
      return false;
    }
    //
    desc.imm |= vecEnc << 12;
    //
    if (vma.dataTranspose) {
      desc.imm |= 1 << 15;
      //
      if (vma.isAtomic()) {
        err = "atomics do not support transpose operations";
        return false;
      }
    }
  } // end vec non-cmask case
  //
  ////////////////////////////////////////
  // caching options
  if (vma.isAtomic() && vma.cachingL1 != CacheOpt::DEFAULT &&
      vma.cachingL1 != CacheOpt::UNCACHED) {
    err = "atomic L1 must be an uncached option";
    return false;
  } else {
    if (!encLdStVecCaching(p, vma.op, vma.cachingL1, vma.cachingL3, desc)) {
      err = "invalid cache-control combination";
      return false;
    }
  }
  //
  ////////////////////////////////////////
  // address size
  uint32_t asEnc = 0x0;
  switch (vma.addrSize) {
  case 16:
    asEnc = LSC_A16;
    break;
  case 32:
    asEnc = LSC_A32;
    break;
  case 64:
    asEnc = LSC_A64;
    break;
  default:
    err = "unsupported address size";
    return false;
  }
  if (isBlock2dTyped && vma.addrSize != 32) {
    err = "block2d.typed address size must be A32";
    return false;
  }
  if (isBlock2dUntyped && vma.addrSize != 64) {
    err = "block2d untyped address size must be A64";
    return false;
  }
  if (hasAddrSizeField) {
    desc.imm |= asEnc << 7;
  }
  //
  ////////////////////////////////////////
  // address type
  uint32_t atEnc = 0x0;
  switch (vma.addrType) {
  case AddrType::FLAT:
    atEnc = LSC_AT_FLAT;
    break;
  case AddrType::BSS:
    atEnc = LSC_AT_BSS;
    break;
  case AddrType::SS:
    atEnc = LSC_AT_SS;
    break;
  case AddrType::BTI:
    atEnc = LSC_AT_BTI;
    break;
  default:
    err = "unsupported address type";
    return false;
  }
  if (isBlock2dTyped && vma.addrType == AddrType::FLAT) {
    err = "block2d.typed forbids flat address";
    return false;
  }
  desc.imm |= atEnc << 29;
  //
  // store the surface
  if (vma.addrType != AddrType::FLAT) {
    // use exDesc
    if (vma.addrType == AddrType::BTI && !vma.addrSurface.isReg()) {
      exDesc = vma.addrSurface.imm << 24;
    } else {
      exDesc = vma.addrSurface;
    }
  }
  //
  if (vma.addrType != AddrType::FLAT && vma.sfid == SFID::SLM) {
    err = "SLM requires flat address type";
    return false;
  }
  //
  ////////////////////////////////////////
  // address immediate offset
  bool hasAddrImmOffset = vma.addrOffset != 0;
  hasAddrImmOffset |= vma.addrOffsetX != 0;
  hasAddrImmOffset |= vma.addrOffsetY != 0;
  if (hasAddrImmOffset) {
    if (p < Platform::XE2) {
      err = "address immediate offset not supported on this platform";
      return false;
    }

    if (!encLdStVecImmOffset(p, vma, exImmOffDesc, exDesc, err))
      return false;
  }  // else: addrOffset == 0

  ////////////////////////////////////////
  // set the surface object
  if (vma.addrType == AddrType::FLAT) {
    // IR normalization
    if (!vma.addrSurface.isImm() || vma.addrSurface.imm != 0) {
      err = "malformed IR: flat address model must have surface = 0";
      return false;
    }
  }

  // XeHPG+ have surface in ExDesc
  if (vma.addrType == AddrType::BTI && vma.addrSurface.isImm()) {
    // BTI takes the high byte
    if (vma.addrSurface.imm > 0xFF) {
      err = "surface index too large for BTI";
      return false;
    }
    exDesc.imm |= vma.addrSurface.imm << 24;
  } else if (vma.addrType != AddrType::FLAT) {
    uint32_t ZERO_MASK = 0xFFF;
    std::string highBit = "11";
    if (p >= Platform::XE2) {
      ZERO_MASK = 0x7FF;
      highBit = "10";
    }

    // if BTI reg or BSS/SS reg/imm with just copy
    // BSS/SS with imm, value is already aligned
    if (vma.addrType != AddrType::BTI && vma.addrSurface.isImm() &&
        (vma.addrSurface.imm & ZERO_MASK) != 0) {
      err = "BSS/SS with immediate descriptor require "
            "ExDesc[" +
            highBit + ":0] to be 0";
      return false;
    }
    exDesc = vma.addrSurface;
  }
  //
  return true;
}

bool iga::encodeDescriptorsLSC(Platform p, const VectorMessageArgs &vma,
                               uint32_t &exImmOffDesc, SendDesc &exDesc,
                               SendDesc &desc, std::string &err) {
  if (!sendOpSupportsSyntax(p, vma.op, vma.sfid)) {
    err = "unsupported message for SFID";
    return false;
  }
  return encLdStVec(p, vma, exImmOffDesc, exDesc, desc, err);
}
