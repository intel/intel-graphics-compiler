/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "MessageDecoderE64.hpp"

#include <string_view>

#define LSC_LOOKUP_TABLE(FIELD, OFF, LEN, TBL)                                 \
  lookupFromTable(FIELD, OFF, LEN, TBL, sizeof(TBL) / sizeof(TBL[0]))

using namespace iga;

static const LscCacheValidValue LSC_CACHING_LD[]{
    LscCacheValidValue{0x0, CacheOpt::DEFAULT, CacheOpt::DEFAULT, CacheOpt::DEFAULT,
                       ".df.df.df", "L1 state, L2 MOCS, L3 MOCS"},
    LscCacheValidValue{0x2, CacheOpt::UNCACHED, CacheOpt::UNCACHED, CacheOpt::UNCACHED,
                       ".uc.uc.uc", "L1 uncached, L2 uncached, L3 uncached"},
    LscCacheValidValue{0x3, CacheOpt::UNCACHED, CacheOpt::UNCACHED, CacheOpt::CACHED,
                       ".uc.uc.ca", "L1 uncached, L2 uncached, L3 cached"},
    LscCacheValidValue{0x4, CacheOpt::UNCACHED, CacheOpt::CACHED, CacheOpt::UNCACHED,
                       ".uc.ca.uc", "L1 uncached, L2 cached, L3 uncached"},
    LscCacheValidValue{0x4, CacheOpt::UNCACHED, CacheOpt::CACHED, CacheOpt::UNCACHED,
                       ".uc.ca.uc", "L1 uncached, L2 cached, L3 uncached"},
    LscCacheValidValue{0x5, CacheOpt::UNCACHED, CacheOpt::CACHED, CacheOpt::CACHED,
                       ".uc.ca.ca", "L1 uncached, L2 cached, L3 cached"},
    LscCacheValidValue{0x6, CacheOpt::CACHED, CacheOpt::UNCACHED, CacheOpt::UNCACHED,
                       ".ca.uc.uc", "L1 cached, L2 uncached, L3 uncached"},
    LscCacheValidValue{0x7, CacheOpt::CACHED, CacheOpt::UNCACHED, CacheOpt::CACHED,
                       ".ca.uc.ca", "L1 cached, L2 uncached, L3 cached"},
    LscCacheValidValue{0x8, CacheOpt::CACHED, CacheOpt::CACHED, CacheOpt::UNCACHED,
                       ".ca.ca.uc", "L1 cached, L2 cached, L3 uncached"},
    LscCacheValidValue{0x9, CacheOpt::CACHED, CacheOpt::CACHED, CacheOpt::CACHED,
                       ".ca.ca.ca", "L1 cached, L2 cached, L3 cached"},
    LscCacheValidValue{0xA, CacheOpt::STREAMING, CacheOpt::UNCACHED, CacheOpt::UNCACHED,
                       ".st.uc.uc", "L1 streaming, L2 uncached, L3 uncached"},
    LscCacheValidValue{0xB, CacheOpt::STREAMING, CacheOpt::UNCACHED, CacheOpt::CACHED,
                       ".st.uc.ca", "L1 streaming, L2 uncached, L3 cached"},
    LscCacheValidValue{0xC, CacheOpt::STREAMING, CacheOpt::CACHED, CacheOpt::UNCACHED,
                       ".st.ca.uc", "L1 streaming, L2 cached, L3 uncached"},
    LscCacheValidValue{0xD, CacheOpt::STREAMING, CacheOpt::CACHED, CacheOpt::CACHED,
                       ".st.ca.ca", "L1 streaming, L2 cached, L3 cached"},
    LscCacheValidValue{0xE, CacheOpt::READINVALIDATE, CacheOpt::READINVALIDATE, CacheOpt::READINVALIDATE,
                       ".ri.ri.ri", "L1 read-invalidate, L2 read-invalidate, L3 read-invalidate"}
};

static const LscCacheValidValue LSC_CACHING_ST[]{
    LscCacheValidValue{0x0, CacheOpt::DEFAULT, CacheOpt::DEFAULT, CacheOpt::DEFAULT,
                       ".df.df.df", "L1 state, L2 MOCS, L3 MOCS"},
    LscCacheValidValue{0x2, CacheOpt::UNCACHED, CacheOpt::UNCACHED, CacheOpt::UNCACHED,
                       ".uc.uc.uc", "L1 uncached, L2 uncached, L3 uncached"},
    LscCacheValidValue{0x3, CacheOpt::UNCACHED, CacheOpt::UNCACHED, CacheOpt::WRITEBACK,
                       ".uc.uc.wb", "L1 uncached, L2 uncached, L3 writeback"},
    LscCacheValidValue{0x4, CacheOpt::UNCACHED, CacheOpt::WRITEBACK, CacheOpt::UNCACHED,
                       ".uc.wb.uc", "L1 uncached, L2 writeback, L3 uncached"},
    LscCacheValidValue{0x5, CacheOpt::UNCACHED, CacheOpt::WRITEBACK, CacheOpt::WRITEBACK,
                       ".uc.wb.wb", "L1 uncached, L2 writeback, L3 writeback"},
    LscCacheValidValue{0x6, CacheOpt::WRITETHROUGH, CacheOpt::UNCACHED, CacheOpt::UNCACHED,
                       ".wt.uc.uc", "L1 writethrough, L2 uncached, L3 uncached"},
    LscCacheValidValue{0x7, CacheOpt::WRITETHROUGH, CacheOpt::UNCACHED, CacheOpt::WRITEBACK,
                       ".wt.uc.wb", "L1 writethrough, L2 uncached, L3 writeback"},
    LscCacheValidValue{0x8, CacheOpt::WRITETHROUGH, CacheOpt::WRITEBACK, CacheOpt::UNCACHED,
                       ".wt.wb.uc", "L1 writethrough, L2 writeback, L3 uncached"},
    LscCacheValidValue{0x9, CacheOpt::WRITETHROUGH, CacheOpt::WRITEBACK, CacheOpt::WRITEBACK,
                       ".wt.wb.wb", "L1 writethrough, L2 writeback, L3 writeback"},
    LscCacheValidValue{0xA, CacheOpt::STREAMING, CacheOpt::UNCACHED, CacheOpt::UNCACHED,
                       ".st.uc.uc", "L1 streaming, L2 uncached, L3 uncached"},
    LscCacheValidValue{0xB, CacheOpt::STREAMING, CacheOpt::UNCACHED, CacheOpt::WRITEBACK,
                       ".st.uc.wb", "L1 streaming, L2 uncached, L3 writeback"},
    LscCacheValidValue{0xC, CacheOpt::STREAMING, CacheOpt::WRITEBACK, CacheOpt::UNCACHED,
                       ".st.wb.uc", "L1 streaming, L2 writeback, L3 uncached"},
    LscCacheValidValue{0xD, CacheOpt::WRITEBACK, CacheOpt::UNCACHED, CacheOpt::UNCACHED,
                       ".wb.uc.uc", "L1 writeback, L2 uncached, L3 uncached"},
    LscCacheValidValue{0xE, CacheOpt::WRITEBACK, CacheOpt::WRITEBACK, CacheOpt::UNCACHED,
                       ".wb.wb.uc", "L1 writeback, L2 writeback, L3 uncached"},
    LscCacheValidValue{0xF, CacheOpt::WRITEBACK, CacheOpt::UNCACHED, CacheOpt::WRITEBACK,
                       ".wb.uc.wb", "L1 writeback, L2 uncached, L3 writeback"}
};

static const LscCacheValidValue LSC_CACHING_AT[]{
    LscCacheValidValue{0x0, CacheOpt::DEFAULT, CacheOpt::DEFAULT, CacheOpt::DEFAULT,
                       ".df.df.df", "L1 state, L2 MOCS, L3 MOCS"},
    LscCacheValidValue{0x2, CacheOpt::UNCACHED, CacheOpt::UNCACHED, CacheOpt::UNCACHED,
                       ".uc.uc.uc", "L1 uncached, L2 uncached, L3 uncached"},
    LscCacheValidValue{0x3, CacheOpt::UNCACHED, CacheOpt::UNCACHED, CacheOpt::WRITEBACK,
                       ".uc.uc.wb", "L1 uncached, L2 uncached, L3 writeback"},
    LscCacheValidValue{0x4, CacheOpt::UNCACHED, CacheOpt::WRITEBACK, CacheOpt::UNCACHED,
                       ".uc.wb.uc", "L1 uncached, L2 writeback, L3 uncached"}
};

///////////////////////////////////////////////////////////////////////////////
MessageDecoderE64::MessageDecoderE64(Platform _platform, SFID _sfid,
                                     ExecSize _instExecSize,
                                     int _src0Len, int _src1Len,
                                     uint64_t _desc, RegRef id0,
                                     RegRef id1, DecodeResult &_result)
    : decodeModel(Model::LookupModelRef(_platform)), sfid(_sfid),
      execSize(_instExecSize), src0Len(_src0Len), src1Len(_src1Len),
      desc(_desc), indDesc0(id0), indDesc1(id1),
      //
      result(_result)
//
{
  result.info.execWidth =
      _instExecSize != ExecSize::INVALID ? int(_instExecSize) : 0;
}

// Descriptor Bits[19:16]: 4 bits of cache control
const LscCacheValidValue *
MessageDecoderE64::decodeCacheControlValue(SendOp sop) {
  ///////////////////////////////////////////////////////
  // Cache Opt
  // Value for bits[19:17]
  const auto ccBits = getDescBits(16, 4);
  const auto &opInfo = lookupSendOp(sop);
  auto searchTable = [&](const LscCacheValidValue *table,
                         size_t tlen) -> const LscCacheValidValue * {
    for (size_t i = 0; i < tlen; i++) {
      if (table[i].encoding == ccBits) {
        addField("Caching", 16, 4, ccBits, table[i].desc);
        return &table[i];
      }
    }
    return nullptr;
  };

  const LscCacheValidValue *desc = nullptr;
  if (opInfo.isLoad() || opInfo.isSample() || opInfo.isCCtrl()) {
    desc = searchTable(LSC_CACHING_LD,
                       sizeof(LSC_CACHING_LD) / sizeof(LSC_CACHING_LD[0]));
  } else if (opInfo.isStore()) {
    desc = searchTable(LSC_CACHING_ST,
                       sizeof(LSC_CACHING_ST) / sizeof(LSC_CACHING_ST[0]));
  } else if (opInfo.isAtomic()) {
    desc = searchTable(LSC_CACHING_AT,
                       sizeof(LSC_CACHING_AT) / sizeof(LSC_CACHING_AT[0]));
  }
  if (desc == nullptr) {
    error(16, 4, "invalid caching option");
  }
  //
  return desc;
} // decodeLscCacheControl

std::string MessageDecoderE64::decodeVectorSizeCmask(const char SYN[4]) {
  auto mask = getDescBits(7, 4);
  std::stringstream sss;
  sss << ".";
  std::stringstream mss;
  int epa = 0;
  for (int i = 0; i < 4; i++) {
    if ((1 << i) & mask) {
      if (epa != 0)
        mss << ", ";
      epa++;
      mss << SYN[i];
      sss << SYN[i];
    }
  }
  if (mask == 0)
    sss << "<empty>";
  result.info.elemsPerAddr = epa;
  result.info.channelsEnabled = mask;

  std::string s = epa == 1 ? "" : "s";
  addField("ComponentMask", 7, 4, mask, "load channel" + s + " " + mss.str());
  return sss.str();
}

void MessageDecoderE64::decodeSsIdx() {
  IGA_ASSERT(result.info.addrType == AddrType::SURF,
             "SS_IDX only exists on surface messages");
  auto enc = getDescBits(22, 5);
  std::stringstream mss;
  mss << "+" << enc
      << " SURFACE_STATE entries "
         "to state addr in IND0";
  addField("Surface State Index", 22, 5, enc, mss.str());
  result.info.surfaceStateIndex = enc;
}

void MessageDecoderE64::decodeSmsIdx() {
  IGA_ASSERT(result.info.addrType == AddrType::SURF,
             "SMS_IDX only exists on sampler messages");
  auto enc = getDescBits(27, 3);
  std::stringstream mss;
  mss << "+" << enc
      << " SAMPLER_STATE entries "
         "to state addr in IND1";
  addField("Sampler State Index", 27, 3, enc, mss.str());
  result.info.samplerStateIndex = enc;
}

void MessageDecoderE64::decodeUvrOffsets() {
  auto decodeCoord = [&](const char *field, char which, int off) {
    auto encU = getDescBits(off, 4);
    int encS = getSignedBits<int32_t>(encU, 0, 4);
    std::stringstream mss;
    mss << "add " << encS << " to " << which << " coordinates";
    addField(field, off, 4, encU, mss.str());
    return encS;
  };
  result.info.uvrImmediateOffsets[0] = decodeCoord("U Offset", 'U', 38);
  result.info.uvrImmediateOffsets[1] = decodeCoord("V Offset", 'V', 34);
  result.info.uvrImmediateOffsets[2] = decodeCoord("R Offset", 'R', 30);
  if (result.info.uvrImmediateOffsets[0] ||
      result.info.uvrImmediateOffsets[1] ||
      result.info.uvrImmediateOffsets[2]) {
    std::stringstream ss;
    ss << "(";
    for (int i = 0; i < 3; i++) {
      if (i > 0)
        ss << ",";
      ss << result.info.uvrImmediateOffsets[i];
    }
    ss << ")";
    result.syntax.immOffset += ss.str();
  }
}

std::string MessageDecoderE64::decodeSurfaceTypeHint() {
  std::string sym{};
  decodeField("SurfaceTypeHint", 11, 3,
    [&] (uint32_t val, std::stringstream &ss) {
      auto set = [&](std::string_view syn, std::string_view desc) {
        ss << desc;
        sym = std::string(syn);
      };
      switch (val) {
      case 0: set("", "NO_HINT"); break;
      case 1: set(".buf", "BUFFER"); break;
      case 2: set(".t1d", "1D_1DARRAY"); break;
      case 3: set(".t2d", "2D_2DARRAY"); break;
      case 4: set(".3d", "3D"); break;
      case 5: set(".cube", "CUBE"); break;
      case 6: set(".cubea", "CUBEARRAY"); break;
      default:
        set("." + fmtHex(val) + "?", "???");
        break;
      }
      result.syntax.controls += sym;
    });
  return sym;
}

void MessageDecoderE64::constructSurfaceSyntax() {
  if (result.info.addrType != AddrType::SURF) {
    return;
  }
  std::stringstream sss;
  sss << "surf[";
  if (indDesc0 == REGREF_INVALID) {
    sss << "?";
  } else {
    sss << "s" << indDesc0.regNum << "." << indDesc0.subRegNum;
  }
  if (result.info.surfaceStateIndex) {
    sss << "," << result.info.surfaceStateIndex;
  }
  sss << "]";

  if (indDesc1 != REGREF_INVALID) {
    sss << "[";
    sss << ",";
    sss << "s" << indDesc1.regNum << "." << indDesc1.subRegNum;
    if (result.info.samplerStateIndex) {
      sss << "," << result.info.samplerStateIndex;
    }
    sss << "]";
  }
  result.syntax.surface = sss.str();
}
