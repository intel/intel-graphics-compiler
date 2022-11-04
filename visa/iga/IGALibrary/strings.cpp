/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "bits.hpp"
#include "strings.hpp"
#include "common/secure_mem.h"
#include "common/secure_string.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>
#if _WIN32
#include <string_view>
#endif

using namespace iga;

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // omit min()/max() macros (favor std::min/std::max)
#endif // NOMINMAX
#endif // _WIN32


void iga::copyOutString(
  char *dst, size_t dstLen, size_t *nWritten, const char *src)
{
    size_t srcLen = iga::stringLength(src) + 1;
    size_t cpLen = dstLen < srcLen ? dstLen : srcLen;
    if (dst != nullptr) {
      memcpy_s(dst, cpLen, src, cpLen);
      dst[cpLen - 1] = 0;
    }
    if (nWritten)
      *nWritten = cpLen;
}


size_t iga::stringLength(const char *s)
{
#if _WIN32
  return std::string_view(s).size();
#else
  return ::strlen(s);
#endif
}

std::ostream &operator<<(std::ostream &os, hex h) {
  os << fmtHexDigits(h.value, h.cols);
  return os;
}

std::string iga::trimTrailingWs(const std::string &s) {
  int i = (int)s.size() - 1;
  while (i >= 0) {
    if (!isspace(s[i]))
      break;
    i--;
  }
  return s.substr(0, (size_t)i + 1);
}

std::vector<std::string> iga::toLines(const std::string &s) {
  std::vector<std::string> ls;
  std::stringstream ss(s);
  std::string to;

  while (std::getline(ss, to, '\n'))
    ls.emplace_back(to);

  return ls;
}

size_t iga::copyOut(char *buf, size_t bufCap, std::iostream &ios) {
  size_t sslen = (size_t)ios.tellp();
  if (!buf || bufCap == 0)
    return sslen + 1;
  ios.read(buf, bufCap);
  size_t eos = bufCap - 1 < sslen ? bufCap - 1 : sslen;
  buf[eos] = '\0';
  return sslen + 1;
}

void iga::fmtBinaryDigits(std::ostream &os, uint64_t val, int w) {
  if (w == 0) {
    // STL really needs this
    // gcc has __builtin_clzll, but let's ignore the #ifdef nonsense
    w = std::max<int>(1, findLeadingOne(val) + 1);
  }

  for (int i = w - 1; i >= 0; i--) {
    if (val & (1ull << (uint64_t)i)) {
      os << '1';
    } else {
      os << '0';
    }
  }
}
void iga::fmtBinary(std::ostream &os, uint64_t val, int w) {
  os << "0b";
  fmtBinaryDigits(os, val, w);
}

void iga::fmtHexDigits(std::ostream &os, uint64_t val, int w) {
  std::stringstream ss;
  if (w > 0) {
    ss << std::setw(w) << std::setfill('0') << std::hex << std::uppercase
       << val;
  } else {
    ss << std::hex << std::uppercase << val;
  }
  os << ss.str();
}
std::string iga::fmtHexDigits(uint64_t val, int w) {
  std::stringstream ss;
  fmtHexDigits(ss, val, w);
  return ss.str();
}

void iga::fmtHex(std::ostream &os, uint64_t val, int w) {
  os << "0x";
  fmtHexDigits(os, val, w);
}
std::string iga::fmtHex(uint64_t val, int w) {
  std::stringstream ss;
  fmtHex(ss, val, w);
  return ss.str();
}

std::string iga::fmtHexSigned(int64_t val, int w) {
  std::stringstream ss;
  fmtHexSigned(ss, val, w);
  return ss.str();
}

void iga::fmtHexSigned(std::ostream &os, int64_t val, int w) {
  if (val < 0) {
    os << "-";
    fmtHex(os, -val);
  } else {
    fmtHex(os, val);
  }
}
