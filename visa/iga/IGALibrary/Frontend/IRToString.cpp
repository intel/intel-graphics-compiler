/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <ostream>
#include <sstream>
#include <string>

#include "../strings.hpp"
#include "IRToString.hpp"

using namespace iga;

std::string iga::ToSymbol(Platform x) {
  switch (x) {
    MAKE_CASE(Platform, INVALID);
    MAKE_CASE(Platform, GEN6);
    MAKE_CASE(Platform, GEN7);
    MAKE_CASE(Platform, GEN7P5);
    MAKE_CASE(Platform, GEN8);
    MAKE_CASE(Platform, GEN8LP);
    MAKE_CASE(Platform, GEN9);
    MAKE_CASE(Platform, GEN9LP);
    MAKE_CASE(Platform, GEN9P5);
    MAKE_CASE(Platform, GEN10);
    MAKE_CASE(Platform, GEN11);
    MAKE_CASE(Platform, XE);
    MAKE_CASE(Platform, XE_HP);
    MAKE_CASE(Platform, XE_HPG);
    MAKE_CASE(Platform, XE_HPC);
    MAKE_CASE(Platform, XE2);
    MAKE_CASE(Platform, XE3);
    MAKE_CASE(Platform, XE3P_XPC);
    MAKE_CASE(Platform, FUTURE);
    MAKE_DEFAULT_CASE(Platform);
  }
}

std::string iga::ToSyntax(MathFC sf) {
  switch (sf) {
  case MathFC::COS:
    return "cos";
  case MathFC::EXP:
    return "exp";
  case MathFC::FDIV:
    return "fdiv";
  case MathFC::IDIV:
    return "idiv";
  case MathFC::IQOT:
    return "iqot";
  case MathFC::IREM:
    return "irem";
  case MathFC::INV:
    return "inv";
  case MathFC::INVM:
    return "invm";
  case MathFC::LOG:
    return "log";
  case MathFC::POW:
    return "pow";
  case MathFC::RSQT:
    return "rsqt";
  case MathFC::RSQTM:
    return "rsqtm";
  case MathFC::SIN:
    return "sin";
  case MathFC::SQT:
    return "sqt";
  case MathFC::TANH:
    return "tanh";
  case MathFC::SIGM:
    return "sigm";
  default:
    return fmtHex(static_cast<uint32_t>(sf)) + "?";
  }
}
template <> MathFC iga::FromSyntax<MathFC>(const std::string& syn) {
  for (auto sf : ALL_MathFCs) {
    if (syn == ToSyntax(sf))
      return sf;
  }
  return MathFC::INVALID;
}

std::string iga::ToSyntax(SFID sfid) {
  switch (sfid) {
  case SFID::CRE:
    return "cre";
  case SFID::DC0:
    return "dc0";
  case SFID::DC1:
    return "dc1";
  case SFID::DC2:
    return "dc2";
  case SFID::DCRO:
    return "dcro";
  case SFID::GTWY:
    return "gtwy";
  case SFID::NULL_:
    return "null";
  case SFID::RC:
    return "rc";
  case SFID::PIXI:
    return "pixi";
  case SFID::SMPL:
    return "smpl";
  case SFID::TS:
    return "ts";
  case SFID::URB:
    return "urb";
  case SFID::VME:
    return "vme";
  case SFID::BTD:
    return "btd";
  case SFID::RTA:
    return "rta";
  case SFID::SLM:
    return "slm";
  case SFID::TGM:
    return "tgm";
  case SFID::UGM:
    return "ugm";
  case SFID::UGML:
    return "ugml";
  default:
    std::stringstream ss;
    ss << "SFID::0x" << std::hex << static_cast<int>(sfid) << "?";
    return ss.str();
  }
}
template <> SFID iga::FromSyntax<SFID>(const std::string& syn) {
  for (SFID sf : ALL_SFIDS) {
    if (syn == ToSyntax(sf))
      return sf;
  }
  return SFID::INVALID;
}


std::string iga::ToSyntax(SyncFC sfc) {
  switch (sfc) {
  case SyncFC::NOP:
    return "nop";
  case SyncFC::ALLRD:
    return "allrd";
  case SyncFC::ALLWR:
    return "allwr";
  case SyncFC::FLUSH:
    return "flush";
  case SyncFC::FENCE:
    return "fence";
  case SyncFC::BAR:
    return "bar";
  case SyncFC::HOST:
    return "host";
  default:
    return fmtHex(static_cast<uint32_t>(sfc)) + "?";
  }
}
template <> SyncFC iga::FromSyntax<SyncFC>(const std::string& syn) {
  for (auto sf : ALL_SyncFCs) {
    if (syn == ToSyntax(sf))
      return sf;
  }
  return SyncFC::INVALID;
}

std::string iga::ToSyntax(DpasFC sfc) {
  switch (sfc) {
  case DpasFC::F_1X1:
    return "1x1";
  case DpasFC::F_1X2:
    return "1x2";
  case DpasFC::F_1X3:
    return "1x3";
  case DpasFC::F_1X4:
    return "1x4";
  case DpasFC::F_1X5:
    return "1x5";
  case DpasFC::F_1X6:
    return "1x6";
  case DpasFC::F_1X7:
    return "1x7";
  case DpasFC::F_1X8:
    return "1x8";
  //
  case DpasFC::F_2X1:
    return "2x1";
  case DpasFC::F_2X2:
    return "2x2";
  case DpasFC::F_2X3:
    return "2x3";
  case DpasFC::F_2X4:
    return "2x4";
  case DpasFC::F_2X5:
    return "2x5";
  case DpasFC::F_2X6:
    return "2x6";
  case DpasFC::F_2X7:
    return "2x7";
  case DpasFC::F_2X8:
    return "2x8";
  //
  case DpasFC::F_4X1:
    return "4x1";
  case DpasFC::F_4X2:
    return "4x2";
  case DpasFC::F_4X3:
    return "4x3";
  case DpasFC::F_4X4:
    return "4x4";
  case DpasFC::F_4X5:
    return "4x5";
  case DpasFC::F_4X6:
    return "4x6";
  case DpasFC::F_4X7:
    return "4x7";
  case DpasFC::F_4X8:
    return "4x8";
  //
  case DpasFC::F_8X1:
    return "8x1";
  case DpasFC::F_8X2:
    return "8x2";
  case DpasFC::F_8X3:
    return "8x3";
  case DpasFC::F_8X4:
    return "8x4";
  case DpasFC::F_8X5:
    return "8x5";
  case DpasFC::F_8X6:
    return "8x6";
  case DpasFC::F_8X7:
    return "8x7";
  case DpasFC::F_8X8:
    return "8x8";
    //
  default:
    return fmtHex(static_cast<uint32_t>(sfc)) + "?";
  }
}
template <> DpasFC iga::FromSyntax<DpasFC>(const std::string& syn) {
  for (auto sf : ALL_DpasFCs) {
    if (syn == ToSyntax(sf))
      return sf;
  }
  return DpasFC::INVALID;
}
std::string iga::ToSyntax(ShuffleFC op) {
  switch (op) {
  case ShuffleFC::UP:
    return "up";
  case ShuffleFC::UPZ:
    return "upz";
  case ShuffleFC::DN:
    return "dn";
  case ShuffleFC::DNZ:
    return "dnx";
  case ShuffleFC::XOR:
    return "xor";
  case ShuffleFC::IDX:
    return "idx";
  case ShuffleFC::IDX4:
    return "idx4";
  default:
    return fmtHex(static_cast<uint32_t>(op)) + "?";
  }
}

template <> ShuffleFC iga::FromSyntax<ShuffleFC>(const std::string& symbol) {
  for (auto op : ALL_ShuffleFC) {
    if (symbol == ToSyntax(op)) {
      return op;
    }
  }
  return ShuffleFC::INVALID;
}

std::string iga::ToSyntax(LfsrFC op) {
  switch (op) {
  case LfsrFC::LFSR_b32:
    return "b32";
  case LfsrFC::LFSR_b16v2:
    return "b16v2";
  case LfsrFC::LFSR_b8v4:
    return "b8v4";
  default:
    return fmtHex(static_cast<uint32_t>(op)) + "?";
  }
}

template <> LfsrFC iga::FromSyntax<LfsrFC>(const std::string& str) {
  for (auto op : ALL_LfsrFC) {
    if (str == ToSyntax(op)) {
      return op;
    }
  }
  return LfsrFC::INVALID;
}

// dnscl src/dst type and mode strings
std::string iga::ToSyntax(ConvSrcDataType op) {
  switch (op) {
  case ConvSrcDataType::HF:
    return "hf";
  case ConvSrcDataType::BF:
    return "bf";
  default:
    return fmtHex(static_cast<uint32_t>(op)) + "?";
  }
}

template <> ConvSrcDataType iga::FromSyntax<ConvSrcDataType>(const std::string& str) {
  for (auto op : ALL_ConvSrcDataType) {
    if (str == ToSyntax(op)) {
      return op;
    }
  }
  return ConvSrcDataType::INVALID;
}

std::string iga::ToSyntax(ConvDstDataType op) {
  switch (op) {
  case ConvDstDataType::E2M1:
    return "e2m1";
  case ConvDstDataType::INT4:
    return "int4";
  default:
    return fmtHex(static_cast<uint32_t>(op)) + "?";
  }
}

template <> ConvDstDataType iga::FromSyntax<ConvDstDataType>(const std::string& str) {
  for (auto op : ALL_ConvDstDataType) {
    if (str == ToSyntax(op)) {
      return op;
    }
  }
  return ConvDstDataType::INVALID;
}

std::string iga::ToSyntax(DnsclMode op) {
  switch (op) {
    case DnsclMode::MODE0:
      return "mode0";
    case DnsclMode::MODE1:
      return "mode1";
    case DnsclMode::MODE2:
      return "mode2";
    case DnsclMode::MODE3:
      return "mode3";
    default:
      return fmtHex(static_cast<uint32_t>(op)) + "?";
  }
}

template <> DnsclMode iga::FromSyntax<DnsclMode>(const std::string& str) {
  for (auto op : ALL_DnsclMode) {
    if (str == ToSyntax(op)) {
      return op;
    }
  }
  return DnsclMode::INVALID;
}

std::string iga::ToSyntax(RoundingMode op) {
  switch (op) {
  case RoundingMode::SRAND:
    return "srnd";
  case RoundingMode::RNE:
    return "rne";
  default:
    return fmtHex(static_cast<uint32_t>(op)) + "?";
  }
}

template <> RoundingMode iga::FromSyntax<RoundingMode>(const std::string& str) {
  for (auto op : ALL_RoundingMode) {
    if (str == ToSyntax(op)) {
      return op;
    }
  }
  return RoundingMode::INVALID;
}

std::string iga::ToSyntax(DnsclFC dfc) {
  // DnsclFC syntax:
  // <ConvSrcDataType>to<ConvDstDataType>.<DnsclMode>.<RoundingMode>
  return ToSyntax(dfc.convSrcTy) + "to" + ToSyntax(dfc.convDstTy) + "." +
      ToSyntax(dfc.dnsclMode) + "." + ToSyntax(dfc.roundingMode);
}
