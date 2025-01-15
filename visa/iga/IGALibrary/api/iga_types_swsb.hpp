/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_TYPES_SWSB_HPP
#define _IGA_TYPES_SWSB_HPP

#include <stdint.h>

namespace iga {
enum class SWSB_STATUS {
  SUCCESS,
  ERROR_SET_ON_VARIABLE_LENGTH_ONLY,
  ERROR_INVALID_SBID_VALUE,
  ERROR_ENCODE_MODE, // invalid encoding mode
  UNKNOWN_INST_TYPE,
  ERROR_DECODE,
  NONE
};

// Note that we must not change the enum value after adding it. Otherwise
// it can cause compatibility issues to other tools using IGA.
enum class SWSB_ENCODE_MODE : uint32_t {
  SWSBInvalidMode = 0,
  // Xe: 1 distance pipe
  SingleDistPipe = 1,
  // XeHP/XeHPG: 3 distance pipes
  ThreeDistPipe = 2,
  // XeHPC (early variant): 4 distance pipes
  FourDistPipe = 3,
  // XeHPC variation: 4 distance pipes with Long pipe reduction
  FourDistPipeReduction = 6,
  // MTL: 3 distance pipe with DP operations in Math pipe
  ThreeDistPipeDPMath = 7,
};

static_assert((uint32_t)SWSB_ENCODE_MODE::SWSBInvalidMode == 0,
              "oops we need these enum values not to change");
static_assert((uint32_t)SWSB_ENCODE_MODE::SingleDistPipe == 1,
              "oops we need these enum values not to change");
static_assert((uint32_t)SWSB_ENCODE_MODE::ThreeDistPipe == 2,
              "oops we need these enum values not to change");
static_assert((uint32_t)SWSB_ENCODE_MODE::FourDistPipe == 3,
              "oops we need these enum values not to change");
static_assert((uint32_t)SWSB_ENCODE_MODE::FourDistPipeReduction == 6,
              "oops we need these enum values not to change");
static_assert((uint32_t)SWSB_ENCODE_MODE::ThreeDistPipeDPMath == 7,
              "oops we need these enum values not to change");

struct SWSB {
public:
  enum class DistType {
    NO_DIST,
    REG_DIST,
    REG_DIST_ALL,
    REG_DIST_FLOAT,
    REG_DIST_INT,
    REG_DIST_LONG,
    REG_DIST_MATH,       // XeHPC
  };

  enum class TokenType {
    NOTOKEN,
    SET,
    SRC,
    DST,
  };

  enum class InstType { UNKNOWN, DPAS, MATH, SEND, OTHERS };

  enum class SpecialToken {
    NONE,
    NOACCSBSET // XeHPC
  };

public:
  DistType distType;
  TokenType tokenType;
  uint32_t minDist; // distance to nearest register dependency
  uint32_t sbid; // swsb id. the barrier identifier (sbid) applies to all wait's
  SpecialToken spToken;

  constexpr SWSB()
      : distType(DistType::NO_DIST), tokenType(TokenType::NOTOKEN), minDist(0),
        sbid(0), spToken(SpecialToken::NONE) {}

  constexpr SWSB(DistType dt, TokenType tt, uint32_t dis, uint32_t id)
      : distType(dt), tokenType(tt), minDist(dis), sbid(id),
        spToken(SpecialToken::NONE) {}

  constexpr SWSB(SpecialToken st)
      : distType(DistType::NO_DIST), tokenType(TokenType::NOTOKEN), minDist(0),
        sbid(0), spToken(st) {}

  constexpr bool operator==(const SWSB &rhs) const {
    return distType == rhs.distType && tokenType == rhs.tokenType &&
           sbid == rhs.sbid && minDist == rhs.minDist && spToken == rhs.spToken;
  }

  constexpr bool operator!=(const SWSB &rhs) const { return !(*this == rhs); }

  constexpr bool hasSWSB() const {
    return (distType != DistType::NO_DIST) ||
           (tokenType != TokenType::NOTOKEN) || (spToken != SpecialToken::NONE);
  }

  constexpr bool hasBothDistAndToken() const {
    return (distType != DistType::NO_DIST) && (tokenType != TokenType::NOTOKEN);
  }

  constexpr bool hasDist() const { return distType != DistType::NO_DIST; }

  // if this swsb has token, not including SpecialToken
  constexpr bool hasToken() const { return tokenType != TokenType::NOTOKEN; }

  constexpr bool hasSpecialToken() const {
    return spToken != SpecialToken::NONE;
  }

  constexpr static bool isMathPipeInOrder(SWSB_ENCODE_MODE enMode) {
    return
        enMode == SWSB_ENCODE_MODE::FourDistPipe ||
        enMode == SWSB_ENCODE_MODE::FourDistPipeReduction;
  }

  /// decode swsb info to SWSB from the raw encoding into this object
  SWSB_STATUS decode(uint32_t swsbBits, SWSB_ENCODE_MODE enMode,
                     InstType instTy);

  /// encode - encode swsb to bianry
  uint32_t encode(SWSB_ENCODE_MODE enMode, InstType instTy) const;

  /// same as encode (for backwards compatibility)
  uint32_t getSWSBBinary(SWSB_ENCODE_MODE enMode, InstType instTy) const {
    return encode(enMode, instTy);
  }

  /// verify - verify if the SWSB is in the correct combination
  /// according to given instruction type and encoding mode
  /// return true on pass, false on fail
  bool verify(SWSB_ENCODE_MODE enMode, InstType instTy) const;

private:
  /// verifySpecialToken - a helper function to verify if the specialToken is
  /// valid Should only be called when hasSpecialToken is true return true on
  /// pass, false on fail
  bool verifySpecialToken(SWSB_ENCODE_MODE enMode) const;

  // clear all fields
  void clear();

  template <SWSB_ENCODE_MODE M>
  SWSB_STATUS decode(uint32_t swsbBits, InstType instTy);

  template <SWSB_ENCODE_MODE M> uint32_t encode(InstType instTy) const;

  template <SWSB_ENCODE_MODE M> bool verify(InstType instTy) const;
};
} // namespace iga
#endif // #ifndef _IGA_TYPES_SWSB_HPP
