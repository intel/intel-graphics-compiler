/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "../../../bits.hpp"
#include "../MessageDecoder.hpp"

#include <functional>
#include <sstream>

namespace iga {
// field description for table-based lookups
struct ValidValue {
  uint32_t encoding;
  const char *desc, *sym;
  constexpr ValidValue(uint32_t enc, const char *_sym, const char *_desc)
      : encoding(enc), sym(_sym), desc(_desc) {}
}; // ValidValue<T>
template <typename T> struct SimpleValidValue : ValidValue {
  T value;
  constexpr SimpleValidValue(uint32_t enc, T val, const char *desc,
                             const char *sym)
      : ValidValue(enc, desc, sym), value(val) {}
};
struct LscCacheValidValue : ValidValue {
  CacheOpt l1, l2, l3;
  constexpr LscCacheValidValue(uint32_t enc, CacheOpt _l1, CacheOpt _l2, CacheOpt _l3,
                               const char *desc, const char *sym)
      : ValidValue(enc, desc, sym), l1(_l1), l2(_l2), l3(_l3) {}
}; // LscCacheValidValue
struct LscDataSizeValidValue : ValidValue {
  int bitsInMem, bitsInReg;
  constexpr LscDataSizeValidValue(uint32_t enc, int bim, int bir,
                                  const char *desc, const char *sym)
      : ValidValue(enc, desc, sym), bitsInMem(bim), bitsInReg(bir) {}
}; // LscDataSizeValidValue

static const int DEFAULT_EXEC_SIZE = 32;
static const int BYTES_PER_REGISTER = 64;
static const int BITS_PER_REGISTER = 8 * BYTES_PER_REGISTER;

struct MessageDecoderE64 {
  // inputs
  const Model &decodeModel;
  const SFID sfid;
  ExecSize execSize;
  int src0Len, src1Len;
  //
  uint64_t desc;
  RegRef indDesc0, indDesc1;

  // outputs
  DecodeResult &result;

  MessageDecoderE64(Platform _platform, SFID _sfid, ExecSize _instExecSize,
                    int _src0Len, int _src1Len, uint64_t _desc, RegRef id0,
                    RegRef id1, DecodeResult &_result);

  Platform platform() const { return model().platform; }

  const Model &model() const { return decodeModel; }

  bool platformInRange(Platform lo, Platform hi) const {
    return platform() >= lo && platform() <= hi;
  }

  void setDoc(const char *doc0, const char *doc1 = nullptr,
              const char *doc2 = nullptr, const char *doc3 = nullptr) {
    addDoc(DocRef::INVALID, nullptr, doc0);
    addDoc(DocRef::INVALID, nullptr, doc1);
    addDoc(DocRef::INVALID, nullptr, doc2);
    addDoc(DocRef::INVALID, nullptr, doc3);
  }
  // appends a documentation note to the next slot
  void addDoc(DocRef::Kind k, const char *name, const char *link) {
    if (k == DocRef::INVALID && !name && !link)
      return;
    result.info.refs.emplace_back(k, name, link);
  }
  void addDocs(
    DocRef::Kind k0, const char *nm0, const char *lnk0,
    DocRef::Kind k1, const char *nm1, const char *lnk1,
    DocRef::Kind k2 = DocRef::INVALID,
      const char *nm2 = nullptr, const char *lnk2 = nullptr,
    DocRef::Kind k3 = DocRef::INVALID,
      const char *nm3 = nullptr, const char *lnk3 = nullptr,
    DocRef::Kind k4 = DocRef::INVALID,
      const char *nm4 = nullptr, const char *lnk4 = nullptr)
  {
    addDoc(k0, nm0, lnk0);
    addDoc(k1, nm1, lnk1);
    addDoc(k2, nm2, lnk2);
    addDoc(k3, nm3, lnk3);
    addDoc(k4, nm4, lnk4);
  }

  /////////////////////////////////////////////////////////////
  // shared fields between LSC and other units
  const LscCacheValidValue *decodeCacheControlValue(SendOp sop);
  std::string decodeVectorSizeCmask(const char SYN[4]); // return syntax (.e.g ".xy")
  //
  // the following are used in LSC and sampler
  void decodeSsIdx();
  void decodeSmsIdx();
  void decodeUvrOffsets();
  std::string decodeSurfaceTypeHint();

  void constructSurfaceSyntax();

  void checkBytesLen(const char *what, int decBytes, int expectRegs) {
    if (decBytes < 0 || expectRegs < 0)
      return;
    int decRegs =
        decBytes == 0 ? 0 : std::max(1, decBytes / BYTES_PER_REGISTER);
    if (decRegs != expectRegs) {
      std::stringstream ss;
      ss << "this message takes " << decRegs << " " << what << " reg";
      if (decRegs != 1)
        ss << "s";
      error(0, 6, ss.str());
    }
  }

  /////////////////////////////////////////////////////////////
  template <typename T>
  const T *lookupFromTable(const char *field, int off, int len, const T *table,
                           size_t tableLen) {
    const auto enc = getDescBits(off, len);
    for (size_t i = 0; i < tableLen; i++) {
      if (table[i].encoding == enc) {
        addField(field, off, len, enc, table[i].desc);
        return &table[i];
      }
    }
    error(off, len, std::string("invalid ") + field);
    return nullptr;
  }

  /////////////////////////////////////////////////////////////
  // diagnostics
  template <typename T1, typename T2 = const char *, typename T3 = const char *>
  void addDiag(DiagnosticList &dl, int off, int len, T1 t1, T2 t2 = "",
               T3 t3 = "") {
    std::stringstream ss;
    ss << t1 << t2 << t3;
    dl.emplace_back(DescField(off, len), ss.str());
  }
  template <typename T1, typename T2 = const char *, typename T3 = const char *>
  void warning(int off, int len, T1 t1, T2 t2 = "", T3 t3 = "") {
    addDiag(result.warnings, off, len, t1, t2, t3);
  }
  template <typename T1, typename T2 = const char *, typename T3 = const char *>
  void error(int off, int len, T1 t1, T2 t2 = "", T3 t3 = "") {
    addDiag(result.errors, off, len, t1, t2, t3);
  }

  /////////////////////////////////////////////////////////////
  // peeks at a field without adding it
  uint32_t getDescBits(int off, int len) const {
    IGA_ASSERT(len <= 32, "field too large (use getDescBits64)");
    return (uint32_t)getDescBits64(off, len);
  }
  int32_t getDescBitsSigned(int off, int len) const {
    return getSignedBits<int32_t>(getDescBits(off, len), 0, len);
  }
  uint64_t getDescBits64(int off, int len) const {
    uint64_t bits = desc;
    uint64_t mask = len == 64 ? 0xFFFFFFFFFFFFFFFFull : ((1ull << len) - 1);
    return ((bits >> off) & mask);
  }

  uint32_t getDescBit(int off) const { return getDescBits(off, 1) != 0; }

  // normally use getDescBitsField, but in cases where you've already
  // decoded, the meaning and just want to record the result
  void addField(const char *fieldName, int off, int len, uint64_t val,
                const std::string& meaning) {
    Fragment f(fieldName, off, len);
    addFrag(f, val, meaning);
  }
  void addReserved(int off, int len) {
    Fragment f("Reserved", off, len);
    addFrag(f, getDescBits64(off, len), "");
  }
  void addFrag(Fragment f, uint64_t val, const std::string& meaning) {
    for (const auto &fvs : result.fields) {
      const auto &f1 = std::get<0>(fvs);
      if (f1.overlaps(f)) {
        // uncomment for debugging
        std::stringstream ss;
        ss << "overlapped fields: " << f1.name << " and " << f.name;
        warning(f.offset, f.length, ss.str());
        // IGA_ASSERT_FALSE(ss.str().c_str());
        return; // replicated access (don't record again)
      }
    }
    result.fields.emplace_back(f, (uint32_t)val, meaning);
  }

  ///////////////////////////////////////////////////////////////////////////
  // decoder helpers
  template <typename T>
  T decodeField(const char *fieldName, int off, int len,
                std::function<T(uint32_t, std::stringstream &)> dec) {
    auto enc = getDescBits(off, len);
    std::stringstream ss;
    T v = dec(enc, ss);
    addField(fieldName, off, len, enc, ss.str());
    return v;
  }
  void decodeField(const char *fieldName, int off, int len,
                   std::function<void(uint32_t, std::stringstream &)> dec) {
    auto enc = getDescBits(off, len);
    std::stringstream ss;
    dec(enc, ss);
    addField(fieldName, off, len, enc, ss.str());
  }
  bool decodeExpected(int off, int len, const char *fieldName,
                      uint32_t expected) {
    auto val = getDescBits(off, len);
    if (val != expected) {
      warning(off, len, "field should be ", expected);
    }
    addField(fieldName, off, len, (uint32_t)val, "");
    return val == expected;
  }
}; // MessageDecoderE64

// MessageDecoderLscE64.cpp
void DecodeMessageLscE64(Platform _platform, SFID _sfid, ExecSize _execSize,
                         int src0Len, int src1Len, uint64_t _desc, RegRef id0,
                         RegRef id1, DecodeResult &_result);
// MessageDecoderSamplerE64.cpp
void DecodeMessageSamplerE64(Platform _platform, SFID _sfid, ExecSize _execSize,
                             int src0Len, int src1Len, uint64_t _desc,
                             RegRef id0, RegRef id1, DecodeResult &_result);
void DecodeMessageOtherE64(Platform _platform, SFID _sfid, ExecSize _execSize,
                           int src0Len, int src1Len, uint64_t _desc,
                           RegRef id0, RegRef id1, DecodeResult &_result);
} // namespace iga
