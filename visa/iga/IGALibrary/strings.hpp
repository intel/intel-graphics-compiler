/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_STRINGS_HPP
#define IGA_STRINGS_HPP

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _MSC_VER
#define strdup _strdup
#endif


#define IGA_MODEL_STRING(X) X


namespace iga {
// enables one to write:
//
// iga::format("the hex value is 0x", hex(value, 4), " ...");
//                                    ^^^
// omits the leading "0x"
struct hex {
  uint64_t value;
  short cols;
  hex(uint64_t _value, int _cols = 0) : value(_value), cols((short)_cols) {}
};
} // iga::

std::ostream &operator<<(std::ostream &os, iga::hex);

namespace iga {
///////////////////////////////////////////////////////////////////////////
//
size_t stringLength(const char *);

template <size_t N> size_t stringLength(const char str[N]) {
#if _WIN32
  return ::strnlen_s(str, N);
#else
  return ::strlen(str);
#endif
}



///////////////////////////////////////////////////////////////////////////
struct ModelString {
  const char *text;
  inline constexpr ModelString(const char *s) : text(s == nullptr ? "" : s) {}
  inline constexpr ModelString() : ModelString(nullptr) {}
  inline std::string str() const { return text; }
  inline operator std::string() const { return str(); }
};

///////////////////////////////////////////////////////////////////////////
// template-based string formatting
// format("foo: ", n, " ...");
template <typename... Ts>
static inline void iga_format_to_helper(std::ostream &) {}
template <typename T, typename... Ts>
static inline void iga_format_to_helper(std::ostream &os, const T& t, const Ts&... ts) {
  os << t;
  iga_format_to_helper(os, ts...);
}
template <typename... Ts> static inline std::string format(const Ts&... ts) {
  std::stringstream ss;
  iga_format_to_helper(ss, ts...);
  return ss.str();
}
template <typename... Ts>
static inline void formatTo(std::ostream &os, const Ts&... ts) {
  iga_format_to_helper(os, ts...);
}

///////////////////////////////////////////////////////////////////////////

// trim trailing whitespace
std::string trimTrailingWs(const std::string &s);

// converts a string to lines
std::vector<std::string> toLines(const std::string &s);

// copies the contents of 'os' into buf (safely)
// returns the required string size
size_t copyOut(char *buf, size_t bufCap, std::iostream &ios);
//
// Similar to 'copyOut' but for C-strings.
// Copies up to (strlen(src) + 1) characters out (i.e. includes the NUL).
// This will truncate the string and always include an NUL character.
void copyOutString(
  char *dst, size_t dstLen, size_t *nWritten, const char *src);

// formats a value into binary padding with 0's for a given width w
// (the value of 0 auto computes the minimal width)
// without affecting os's stream state
//   e.g. fmtBinaryDigits(os,0xB,5) => emits 01011
//   e.g. fmtBinaryDigits(os,0xB,0) => emits  1011
void fmtBinaryDigits(std::ostream &os, uint64_t val, int w = 0);
// same as fmtBinaryDigits, but prefixes an additional "0b"
// is careful to not muck up os's state
// w doesn't count the 0b prefix as part of the width
// e.g. fmtBinary(os,0xB,0) -> 0b1101
void fmtBinary(std::ostream &os, uint64_t val, int w = 0);
//
// formats to uppercase hex for a given width
// without affecting os's stream state
void fmtHexDigits(std::ostream &os, uint64_t val, int w = 0);
std::string fmtHexDigits(uint64_t val, int w = 0);

// same as fmtHexDigits, but prefixes an 0x
// does not change os's stream state
void fmtHex(std::ostream &os, uint64_t val, int w = 0);
// a helper that returns a string version
std::string fmtHex(uint64_t val, int w = 0);
//
// e.g. -0x4 rather than 0xFFF....FC
std::string fmtHexSigned(int64_t val, int w = 0);
void fmtHexSigned(std::ostream &os, int64_t val, int w = 0);

// This class simplifies formatting loops by dropping the first comma.
// One calls insert *before* each element being formatted.
//
// EXAMPLE:
//   Intercalator comma(os, ",");
//   for (...) {
//     comma.insert();
//     os << element;
//   }
class Intercalator {
  std::ostream &os;
  bool first = true;
  const char *sep;

public:
  Intercalator(std::ostream &_os, const char *_sep) : os(_os), sep(_sep) {}
  void insert() {
    if (first) {
      first = false;
    } else {
      os << sep;
    }
  }
};

// Same as below, but permits a filter predicate which only allows a
// subset of elements (identified by that predicate).
template <typename Container, typename Predicate, typename Formatter>
static void intercalate(std::ostream &os, const char *sep, const Container &ts,
                        const Predicate &filterT, const Formatter &formatT) {
  Intercalator separator(os, sep ? sep : "");
  for (const auto &t : ts) {
    if (filterT(t)) {
      separator.insert();
      formatT(t);
    }
  }
}
// This emits a container of elements to an output stream by calling
// a given formatter function, while automatically adding separators
// between elements.
//
// EXAMPLE: let foos be a std::vector<Foo> and os be a std::ostream
// Then we might have:
//   intercalate(os, ",", foos,
//      [&] (const Foo &f) {
//         os << ... format f ...
//      });
//
template <typename Container, typename Formatter>
static void intercalate(std::ostream &os, const char *sep, const Container &ts,
                        const Formatter &formatT) {
  Intercalator separator(os, sep ? sep : "");
  for (const auto &t : ts) {
    separator.insert();
    formatT(t);
  }
}

// Emits something like:
//   "foo"
//   "foo and bar"
//   "foo, bar, and baz"
template <typename T, typename FormatT>
static void commafyList(
    const char *conjunction, // e.g. "and" or "or"
    std::ostream &os, const std::vector<T> &elems,
    const FormatT &formatElem) // [](std::ostream &os, const T &elem) {...}
{
  switch (elems.size()) {
  case 1:
    formatElem(os, elems[0]);
    break;
  case 2:
    formatElem(os, elems[0]);
    os << " " << conjunction << " ";
    formatElem(os, elems[1]);
    break;
  default:
    for (size_t i = 0; i < elems.size(); i++) {
      if (i > 0)
        os << ", ";
      if (i == elems.size() - 1)
        os << conjunction << " ";
      formatElem(os, elems[i]);
    }
  }
}
template <typename T, typename FormatT>
static void commafyList(std::ostream &os, const std::vector<T> &elems,
                        const FormatT &formatElem) {
  commafyList("and", os, elems, formatElem);
}
template <typename T> std::string PadR(size_t k, const T &t) {
  std::stringstream ssCol;
  ssCol << t;
  for (size_t i = (size_t)ssCol.tellp(); i < k; i++)
    ssCol << ' ';
  return ssCol.str();
}
template <typename T> std::string PadL(size_t k, const T &t) {
  std::stringstream ssc;
  ssc << t;
  std::string col = ssc.str();

  std::stringstream ss;
  int n = (int)k - (int)col.size();
  for (int i = 0; i < n; i++) {
    ss << ' ';
  }
  ss << col;
  return ss.str();
}
} // namespace iga

#endif // IGA_STRINGS_HPP
