/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "OCLInterfaceTester.h"

#include "cif/common/library_api.h"

#include <charconv>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// Supported platforms. Add a line here to support a new platform
static const PlatformEntry platforms[] = {
    {"tgl", IGFX_TIGERLAKE_LP, IGFX_GEN12LP_CORE},
    {"dg2", IGFX_DG2, IGFX_XE_HPG_CORE},
    {"pvc", IGFX_PVC, IGFX_XE_HPC_CORE},
    {"mtl", IGFX_METEORLAKE, IGFX_XE_HPG_CORE},
    {"arl", IGFX_ARROWLAKE, IGFX_XE_HPG_CORE},
    {"bmg", IGFX_BMG, IGFX_XE2_HPG_CORE},
    {"lnl", IGFX_LUNARLAKE, IGFX_XE2_HPG_CORE},
    {"ptl", IGFX_PTL, IGFX_XE3_CORE},
    {"cri", IGFX_CRI, IGFX_XE3P_CORE},
    {"nvl", IGFX_NVL, IGFX_XE3P_CORE},
};

static const PlatformEntry *g_currentPlatform = nullptr;

const PlatformEntry *getCurrentPlatform() { return g_currentPlatform; }

// Select the target platform by --platform name. Returns false if unknown
static bool selectPlatform(std::string_view name) {
  for (const auto &p : platforms)
    if (name == p.name) {
      g_currentPlatform = &p;
      return true;
    }
  return false;
}

// Print help
static void printHelp(const char *testName) {
  std::cout << "IGCOCLInterfaceTester - test the NEO<->IGC CIF interfaces.\n";
  std::cout << "usage: " << testName << " [-h|--help] <check>\n";
  std::cout << "  <check> - name of the check to run (look below)\n";
  std::cout << "  --platform <name> - target platform for platform-dependent checks\n";
  std::cout << "  --help, -h - print this help message\n";
  std::cout << "  Checks:\n";
  for (const auto &[name, info] : registry())
    std::cout << "    " << name << " - " << info.help << "\n";
  std::cout << "  Platforms:\n";
  for (const auto &p : platforms)
    std::cout << "    " << p.name << "\n";
}

// Load the IGC library and return a CIFMain pointer, using the platform's native
// dynamic loader (LoadLibrary on Windows, dlopen elsewhere)
// Use UPtr_t to save the CIFMain pointer and ensure it is released when it goes out of scope.
static CIF::RAII::UPtr_t<CIF::CIFMain> getCIFMain(const char *libName) {
  const char *symbolName = CIF::CreateCIFMainFuncName;
  void *address = nullptr;

#if defined(_WIN32)
  HMODULE handle = LoadLibraryA(libName);
  if (handle == nullptr) {
    std::cerr << "error: failed to load '" << libName << "': error " << GetLastError() << "\n";
    return nullptr;
  }
  address = reinterpret_cast<void *>(GetProcAddress(handle, symbolName));
#else
  void *handle = dlopen(libName, RTLD_LAZY);
  if (handle == nullptr) {
    const char *err = dlerror();
    std::cerr << "error: failed to load '" << libName << "': " << (err ? err : "unknown error") << "\n";
    return nullptr;
  }
  address = dlsym(handle, symbolName);
#endif

  auto cifMain = reinterpret_cast<CIF::CreateCIFMainFunc_t>(address);
  if (!cifMain) {
    std::cerr << "error: '" << libName << "' does not export '" << symbolName << "'\n";
    return nullptr;
  }

  return CIF::RAII::UPtr(cifMain());
}

// Parse "Field=Value" lines from stdin from .test files and invoke
// callback(key, value) for each. Blank lines and lines that start with '#' are ignored
int forEachStdinField(std::function<bool(std::string_view key, uint64_t val)> callback) {
  // Read all of stdin
  std::string input((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
  std::string_view data = input;

  for (size_t pos = 0; pos <= data.size();) {
    const size_t nl = data.find('\n', pos);
    const size_t end = (nl == std::string_view::npos) ? data.size() : nl;
    std::string_view line = trim(data.substr(pos, end - pos));
    pos = (nl == std::string_view::npos) ? data.size() + 1 : nl + 1;

    if (line.empty() || line.front() == '#')
      continue;

    const size_t eq = line.find('=');
    std::string_view key = trim(line.substr(0, eq));
    std::string_view valStr = (eq == std::string_view::npos) ? std::string_view() : trim(line.substr(eq + 1));

    uint64_t val = 0;
    auto res = std::from_chars(valStr.data(), valStr.data() + valStr.size(), val);
    if (valStr.empty() || res.ec != std::errc() || res.ptr != valStr.data() + valStr.size()) {
      std::cerr << "error: invalid value in line '" << line << "'\n";
      return ExitCode::IllegalInputFormat;
    }
    if (!callback(key, val)) {
      return ExitCode::IllegalInputFormat;
    }
  }

  return ExitCode::Success;
}

int main(int argc, char **argv) {

  if (argc < 2) {
    std::cerr << "error: missing check name for IGCOCLInterfaceTester - provide appropriate check\n";
    std::cerr << "usage: " << argv[0] << " <check>\n";
    return ExitCode::MissingCheckName;
  }

  std::string_view firstArg = argv[1];
  if (firstArg == "--help" || firstArg == "-h") {
    printHelp(argv[0]);
    return ExitCode::Success;
  }

  std::string_view check = argv[1];

  // Parse options after the check name. Currently only --platform <name>.
  for (int i = 2; i < argc; ++i) {
    std::string_view arg = argv[i];
    if (arg == "--platform") {
      if (i + 1 >= argc) {
        std::cerr << "error: --platform requires a <name> argument\n";
        return ExitCode::MissingPlatform;
      }
      if (!selectPlatform(argv[++i])) {
        std::cerr << "error: unknown platform '" << argv[i] << "'\n";
        return ExitCode::UnknownPlatform;
      }
    } else {
      std::cerr << "error: unknown option '" << arg << "'\n";
      return ExitCode::UnknownOption;
    }
  }

  // Load the libigc whose absolute path was baked in at configure time.
  auto cif = getCIFMain(IGC_TESTER_LIBRARY_PATH);
  if (!cif) {
    return ExitCode::LoadFailure;
  }

  auto it = registry().find(check);
  if (it == registry().end()) {
    std::cerr << "error: unknown check '" << check << "'\n";
    return ExitCode::UnknownCheck;
  }
  return it->second.run(cif.get());
}
