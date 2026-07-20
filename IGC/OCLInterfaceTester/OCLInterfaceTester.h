/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <string_view>

#include "igfxfmid.h"
#include "cif/common/cif_main.h"
#include "cif/builtins/memory/buffer/buffer.h"

enum ExitCode {
  Success = 0,              // Success
  MissingCheckName = 1,     // Missing 'check' name for IGCOCLInterfaceTester
  LoadFailure = 2,          // Failed to load the IGC library or failed to find the CIFMain
  UnknownCheck = 3,         // Unknown check name provided to IGCOCLInterfaceTester
  UnsupportedInterface = 4, // The requested interface is not supported by the IGC library
  FailedToGetInterface = 5, // Failed to get the requested interface from the IGC library
  UnknownPlatform = 6,      // Unknown --platform name provided
  MissingPlatform = 7,      // A check requires --platform but none was provided
  MissingInput = 8,         // A required Field=Value input was not provided on stdin
  UnknownOption = 9,        // An unrecognized command-line option was provided
  IllegalInputFormat = 10   // A stdin line could not be parsed
};

// Named target platform selectable via --platform.
struct PlatformEntry {
  const char *name;
  PRODUCT_FAMILY product;
  GFXCORE_FAMILY core;
};

// The platform chosen by --platform, or nullptr if none was given
const PlatformEntry *getCurrentPlatform();

// Apply the selected platform to a device-context Platform handle
template <class P> bool applyPlatform(P *platform) {
  const PlatformEntry *current = getCurrentPlatform();
  if (!current)
    return false;

  platform->SetProductFamily(static_cast<uint64_t>(current->product));
  platform->SetRenderCoreFamily(static_cast<uint64_t>(current->core));
  return true;
}

// Read a CIF buffer and return it as a std::string_view.
inline std::string_view readBuf(CIF::Builtins::BufferSimple *buf) {
  const char *mem = buf->GetMemory<char>();
  return mem ? std::string_view(mem, buf->GetSize<char>()) : std::string_view();
}

// Trim ASCII whitespace from both ends of a string_view.
inline std::string_view trim(std::string_view s) {
  const char *ws = " \t\r\n\f\v";
  const size_t b = s.find_first_not_of(ws);
  if (b == std::string_view::npos)
    return {};
  return s.substr(b, s.find_last_not_of(ws) - b + 1);
}

// Report an unrecognized stdin field from a forEachStdinField
inline bool unknownField(std::string_view key) {
  std::cerr << "error: unknown field '" << key << "'\n";
  return false;
}

// Parse lines from stdin from .test files
int forEachStdinField(std::function<bool(std::string_view key, uint64_t val)> callback);

using CheckFn = int (*)(CIF::CIFMain *);

struct CheckInfo {
  CheckFn run;
  const char *help;
};

inline std::map<std::string_view, CheckInfo> &registry() {
  static std::map<std::string_view, CheckInfo> r;
  return r;
}

struct Reg {
  Reg(std::string_view name, CheckFn run, const char *help) { registry()[name] = {run, help}; }
};

// Declare + self-register + open a check body in one line.
#define CHECK(name, help)                                                                                              \
  static int name(CIF::CIFMain *cif);                                                                                  \
  static const Reg reg_##name(#name, name, help);                                                                      \
  static int name(CIF::CIFMain *cif)
