/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <algorithm>
#include <cinttypes>
#include <limits>
#include <string>

#include "cif/helpers/error.h"

namespace CIF {

namespace CoderHelpers {
enum Mode { Text = 0, Numeric = 1 };

template <Mode M> struct Traits;

template<typename T, T ... RestVals>
struct ConstexprArray;

template<typename T>
struct ConstexprArray<T>{
    static constexpr T Get(size_t Idx){
        return 0;
    }

    static constexpr T Size(){
        return 0;
    }
};

template<typename T, T Val, T ... RestVals>
struct ConstexprArray<T, Val, RestVals...>{
  static constexpr T Get(size_t Idx){
    return (Idx == 0) ? Val : ConstexprArray<T, RestVals...>::Get(Idx-1);
  }

  static constexpr size_t Size(){
      return 1 + ConstexprArray<T, RestVals...>::Size();
  }
};

template <> struct Traits<Text> {
  static constexpr uint32_t CodingSize = 5;
  static constexpr char FirstChar = 'A';
  static constexpr char LastChar = 'Z';
  static constexpr char SpecialCharsOffset = (LastChar - FirstChar) + 1;
  using SpecialChars = ConstexprArray<char, '-', '_', ':', '@', '.'>;
  static constexpr char SwitchModeMarker = (char)SpecialChars::Size();

  static_assert((static_cast<size_t>(1) << CodingSize) >
                    static_cast<size_t>(((LastChar - FirstChar) +
                                         SwitchModeMarker + 1)),
                "Invalid Encoding");

  static constexpr Mode OpositeTraits = Numeric;
};

template <> struct Traits<Numeric> {
  static constexpr uint32_t CodingSize = 4;
  static constexpr char FirstChar = '0';
  static constexpr char LastChar = '9';
  static constexpr char SpecialCharsOffset = (LastChar - FirstChar) + 1;
  using SpecialChars = ConstexprArray<char, '-', '_', ':', '#', '.'>;
  static constexpr char SwitchModeMarker = (char)SpecialChars::Size();

  static_assert((static_cast<size_t>(1) << CodingSize) >
                    ((LastChar - FirstChar) + SwitchModeMarker + 1),
                "Invalid Encoding");

  static constexpr Mode OpositeTraits = Text;
};

template <typename CurrTraits> inline bool IsSwitchMode(char v) {
  return false;
}

template <typename T, typename CurrTraits>
inline constexpr T EncSwitchMode(uint32_t offset) {
  return static_cast<T>(
             (CurrTraits::SpecialCharsOffset + CurrTraits::SwitchModeMarker))
         << offset;
}

template <typename CurrTraits>
inline constexpr bool IsSpecialChar(char v, uint32_t offset = 0) {
  return (offset >= CurrTraits::SpecialChars::Size())
             ? false
             : (v == CurrTraits::SpecialChars::Get(offset)) ||
                   IsSpecialChar<CurrTraits>(v, offset + 1);
}

template <typename CurrTraits> inline constexpr bool IsValid(char v) {
  return ((v >= CurrTraits::FirstChar) && (v <= CurrTraits::LastChar)) ||
         (IsSpecialChar<CurrTraits>(v));
}

template <typename T, typename CurrTraits>
inline constexpr T EncSpecialChar(char v, uint32_t bitOffset,
                                  uint32_t offset = 0) {
  return (offset >= CurrTraits::SpecialChars::Size())
             ? CIF::Abort<T>()
             : ((v == CurrTraits::SpecialChars::Get(offset))
                    ? (static_cast<T>(CurrTraits::SpecialCharsOffset + offset)
                       << bitOffset)
                    : EncSpecialChar<T, CurrTraits>(v, bitOffset, offset + 1));
}

template <typename T, typename CurrTraits>
inline constexpr T EncOneChar(char v, uint32_t bitOffset) {
  return (IsValid<CurrTraits>(v) == false)
             ? CIF::Abort<T>()
             : ((((v >= CurrTraits::FirstChar) && (v <= CurrTraits::LastChar))
                     ? (static_cast<T>(v - CurrTraits::FirstChar) << bitOffset)
                     : EncSpecialChar<T, CurrTraits>(v, bitOffset)));
}

// This is to prevent clang from blowing-up
constexpr uint32_t MaxTemplateDepth = 32;

template <typename T, typename CurrTraits, uint32_t Depth>
inline constexpr typename std::enable_if<Depth >= MaxTemplateDepth, T>::type EncTrailWithSwitchMode(uint32_t bitOffset) {
  return 0;
}

template <typename T, typename CurrTraits, uint32_t Depth>
inline constexpr typename std::enable_if<Depth < MaxTemplateDepth, T>::type EncTrailWithSwitchMode(uint32_t bitOffset) {
  return ((bitOffset + CurrTraits::CodingSize > sizeof(T) * 8))
             ? 0
             : EncSwitchMode<T, CurrTraits>(bitOffset) |
                   EncTrailWithSwitchMode<
                       T, CoderHelpers::Traits<CurrTraits::OpositeTraits>, Depth+1>(
                       bitOffset + CurrTraits::CodingSize);
}

template <typename T, typename CurrTraits, uint32_t Depth>
inline constexpr typename std::enable_if<Depth >= MaxTemplateDepth, T>::type Enc(const char *str, uint32_t strIt, uint32_t bitOffset) {
    return 0;
}

template <typename T, typename CurrTraits, uint32_t Depth>
inline constexpr typename std::enable_if<Depth < MaxTemplateDepth, T>::type Enc(const char *str, uint32_t strIt, uint32_t bitOffset) {
  return (str[strIt] == '\0')
             ? EncTrailWithSwitchMode<T, CurrTraits, 0>(bitOffset)
             : (((bitOffset + CurrTraits::CodingSize > sizeof(T) * 8))
                    ? CIF::Abort<T>()
                    : (IsValid<CurrTraits>(str[strIt])
                           ? (EncOneChar<T, CurrTraits>(str[strIt], bitOffset) |
                              Enc<T, CurrTraits, Depth+1>(str, strIt + 1,
                                                 bitOffset +
                                                     CurrTraits::CodingSize))
                           : (IsValid<CoderHelpers::Traits<
                                      CurrTraits::OpositeTraits>>(str[strIt])
                                  ? EncSwitchMode<T, CurrTraits>(bitOffset) |
                                        Enc<T, CoderHelpers::Traits<
                                                   CurrTraits::OpositeTraits>, Depth+1>(
                                            str, strIt,
                                            bitOffset + CurrTraits::CodingSize)
                                  : CIF::Abort<T>())));
}

template <typename T, typename CurrTraits>
inline constexpr char Extract(T val, uint32_t bitOffset) {
  return static_cast<char>((val >> bitOffset) &
                           static_cast<T>((1 << CurrTraits::CodingSize) - 1));
}

template <typename T, typename CurrTraits>
inline bool Dec(T val, uint32_t bitOffset, char &outC) {
  auto extr = Extract<T, CurrTraits>(val, bitOffset);
  if (extr < CurrTraits::SpecialCharsOffset) {
    outC = extr + CurrTraits::FirstChar;
    return false;
  }

  extr -= CurrTraits::SpecialCharsOffset;
  if (extr == CurrTraits::SwitchModeMarker) {
    return true;
  }

  outC = CurrTraits::SpecialChars::Get(extr);
  return false;
}

template<typename T>
inline constexpr T Min(T a, T b){
    return (a<b) ? a : b;
}

}

constexpr size_t CompileTimeStrlen(const char *s, size_t it = 0) {
  return (s[it] == '\0') ? it : CompileTimeStrlen(s, it + 1);
}

template <typename StorageType> struct Coder {
  static constexpr StorageType Enc(const char *str) {
    return CoderHelpers::Enc<StorageType,
                             CoderHelpers::Traits<CoderHelpers::Text>, 0>(str, 0, 0);
  }

  static std::string Dec(const StorageType &v) {
    constexpr uint32_t totalBits = sizeof(StorageType) * 8;

    uint32_t bitOffset = 0;
    size_t strIt = 0;
    bool numericMode = false;
    char str[totalBits /
             (CIF::CoderHelpers::Min<std::uint32_t>(
                 CoderHelpers::Traits<CoderHelpers::Text>::CodingSize,
                 CoderHelpers::Traits<CoderHelpers::Numeric>::CodingSize))] = {
        0};
    while (bitOffset < totalBits) {
      bool switchMode = false;
      if (numericMode) {
        if (bitOffset +
                CoderHelpers::Traits<CoderHelpers::Numeric>::CodingSize >
            totalBits) {
          break;
        }
        switchMode =
            CoderHelpers::Dec<StorageType,
                              CoderHelpers::Traits<CoderHelpers::Numeric>>(
                v, bitOffset, str[strIt]);
      } else {
        if (bitOffset + CoderHelpers::Traits<CoderHelpers::Text>::CodingSize >
            totalBits) {
          break;
        }
        switchMode =
            CoderHelpers::Dec<StorageType,
                              CoderHelpers::Traits<CoderHelpers::Text>>(
                v, bitOffset, str[strIt]);
      }

      bitOffset += numericMode
                       ? CoderHelpers::Traits<CoderHelpers::Numeric>::CodingSize
                       : CoderHelpers::Traits<CoderHelpers::Text>::CodingSize;

      if (switchMode) {
        numericMode = !numericMode;
      } else {
        ++strIt;
      }
    }
    return std::string{str};
  }
};
}
