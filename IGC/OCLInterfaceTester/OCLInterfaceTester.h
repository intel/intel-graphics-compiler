/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <charconv>
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
  IllegalInputFormat = 10,  // A stdin line could not be parsed
  MissingLibPath = 11,      // A --lib argument was provided but no path was given
  InvalidLibPath = 12,      // A --lib argument was provided but the path was invalid
  WrongLibrary = 13         // The loaded library does not provide the interfaces the check needs
};

// The type of library to load
enum class LibraryType { Igc, Fcl };

// Named target platform selectable via --platform.
struct PlatformEntry {
  const char *name;
  PRODUCT_FAMILY product;
  GFXCORE_FAMILY core;
  const char *coreName;
};

// Pack a GMDID the way GFX_GMD_ID lays it out
inline uint32_t packGmdId(uint32_t arch, uint32_t release, uint32_t revision = 0) {
  GFX_GMD_ID id = {};
  id.GmdID.GMDArch = arch;
  id.GmdID.GMDRelease = release;
  id.GmdID.RevisionID = revision;
  return id.Value;
}

// The platform chosen by --platform, or nullptr if none was given
const PlatformEntry *getCurrentPlatform();

// The value given to --gmdid, or nullptr if the option was not used
const uint32_t *getGmdIdOverride();

// The core to report: --core when given, otherwise the platform's own core
GFXCORE_FAMILY getEffectiveCore();

// The --core spelling of a render core family, or "unknown" if it has none
const char *getCoreName(GFXCORE_FAMILY core);

// Apply the selected platform to a device-context Platform handle
template <class P> bool applyPlatform(P *platform) {
  const PlatformEntry *current = getCurrentPlatform();
  if (!current)
    return false;

  platform->SetProductFamily(static_cast<uint64_t>(current->product));
  platform->SetRenderCoreFamily(static_cast<uint64_t>(getEffectiveCore()));
  return true;
}

// Apply the GMDID given to --gmdid, or zero when the option was not used
template <class P> uint32_t applyRenderBlockID(P *platform) {
  const uint32_t *forced = getGmdIdOverride();
  const uint32_t value = forced ? *forced : 0;
  platform->SetRenderBlockID(value);
  return value;
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

// Convert a stdin value to a number. Reports a malformed value and returns false
inline bool parseValue(std::string_view text, uint64_t &val) {
  val = 0;
  auto res = std::from_chars(text.data(), text.data() + text.size(), val);
  if (text.empty() || res.ec != std::errc() || res.ptr != text.data() + text.size()) {
    std::cerr << "error: invalid value '" << text << "'\n";
    return false;
  }
  return true;
}

// Report an unrecognized stdin field from a forEachStdinField
inline bool unknownField(std::string_view key) {
  std::cerr << "error: unknown field '" << key << "'\n";
  return false;
}

// Parse lines from stdin from .test files
int forEachStdinField(std::function<bool(std::string_view key, std::string_view value)> callback);

using CheckFn = int (*)(CIF::CIFMain *);

struct CheckInfo {
  CheckFn run;
  const char *help;
  LibraryType libType;
};

inline std::map<std::string_view, CheckInfo> &registry() {
  static std::map<std::string_view, CheckInfo> r;
  return r;
}

struct Reg {
  Reg(std::string_view name, CheckFn run, const char *help, LibraryType lib) { registry()[name] = {run, help, lib}; }
};

#define IGC_CHECK(name, help) CHECK_IMPL(name, help, LibraryType::Igc)
#define FCL_CHECK(name, help) CHECK_IMPL(name, help, LibraryType::Fcl)

// Declare + self-register + open a check body in one line.
#define CHECK_IMPL(name, help, library)                                                                                \
  static int name(CIF::CIFMain *cif);                                                                                  \
  static const Reg reg_##name(#name, name, help, library);                                                             \
  static int name(CIF::CIFMain *cif)
