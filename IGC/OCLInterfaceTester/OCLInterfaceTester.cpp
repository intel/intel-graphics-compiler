/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "OCLInterfaceTester.h"

#include "cif/common/library_api.h"

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// Supported platforms and the render core family each one reports
// clang-format off
static const PlatformEntry platforms[] = {
    {"tgl",      IGFX_TIGERLAKE_LP, IGFX_GEN12LP_CORE, "gen12lp"},
    {"rkl",      IGFX_ROCKETLAKE,   IGFX_GEN12LP_CORE, "gen12lp"},
    {"adls",     IGFX_ALDERLAKE_S,  IGFX_GEN12LP_CORE, "gen12lp"},
    {"adlp",     IGFX_ALDERLAKE_P,  IGFX_GEN12LP_CORE, "gen12lp"},
    {"adln",     IGFX_ALDERLAKE_N,  IGFX_GEN12LP_CORE, "gen12lp"},
    {"dg1",      IGFX_DG1,          IGFX_GEN12LP_CORE, "gen12lp"},
    {"dg2",      IGFX_DG2,          IGFX_XE_HPG_CORE,  "xe_hpg"},
    {"pvc",      IGFX_PVC,          IGFX_XE_HPC_CORE,  "xe_hpc"},
    {"mtl",      IGFX_METEORLAKE,   IGFX_XE_HPG_CORE,  "xe_hpg"},
    {"arl",      IGFX_ARROWLAKE,    IGFX_XE_HPG_CORE,  "xe_hpg"},
    {"bmg",      IGFX_BMG,          IGFX_XE2_HPG_CORE, "xe2_hpg"},
    {"lnl",      IGFX_LUNARLAKE,    IGFX_XE2_HPG_CORE, "xe2_hpg"},
    {"ptl",      IGFX_PTL,          IGFX_XE3_CORE,     "xe3"},
    {"nvl_xe3g", IGFX_NVL_XE3G,     IGFX_XE3_CORE,     "xe3"},
    {"cri",      IGFX_CRI,          IGFX_XE3P_CORE,    "xe3p"},
    {"nvl",      IGFX_NVL,          IGFX_XE3P_CORE,    "xe3p"},
};
// clang-format on

static const PlatformEntry *g_currentPlatform = nullptr;
static std::optional<GFXCORE_FAMILY> g_coreOverride;
static uint32_t g_gmdIdOverride = 0;
static bool g_hasGmdIdOverride = false;

const PlatformEntry *getCurrentPlatform() { return g_currentPlatform; }

const uint32_t *getGmdIdOverride() { return g_hasGmdIdOverride ? &g_gmdIdOverride : nullptr; }

GFXCORE_FAMILY getEffectiveCore() {
  if (g_coreOverride)
    return *g_coreOverride;
  return g_currentPlatform ? g_currentPlatform->core : IGFX_UNKNOWN_CORE;
}

const char *getCoreName(GFXCORE_FAMILY core) {
  for (const auto &p : platforms)
    if (core == p.core)
      return p.coreName;
  return "unknown";
}

// Select the target platform by --platform name. Returns false if unknown
static bool selectPlatform(std::string_view name) {
  for (const auto &p : platforms)
    if (name == p.name) {
      g_currentPlatform = &p;
      return true;
    }
  return false;
}

// Select the render core family to report by --core name, overriding the one
// the platform would imply. Returns false if unknown
static bool selectCore(std::string_view name) {
  for (const auto &p : platforms)
    if (name == p.coreName) {
      g_coreOverride = p.core;
      return true;
    }
  return false;
}

// Set the render GMDID to report, from a --gmdid <arch.release[.revision]> spec.
// Reports the problem and returns false if the spec is malformed
static bool selectGmdId(std::string_view spec) {
  static constexpr unsigned maximums[3] = {(1u << 10) - 1, (1u << 8) - 1, (1u << 6) - 1};
  static constexpr const char *names[3] = {"arch", "release", "revision"};

  const std::string_view full = spec;
  unsigned parts[3] = {0, 0, 0};
  size_t n = 0;
  for (; n < 3 && !spec.empty(); ++n) {
    const size_t dot = spec.find('.');
    const std::string_view field = spec.substr(0, dot);
    const char *begin = field.data();
    const auto res = std::from_chars(begin, begin + field.size(), parts[n]);
    if (res.ec != std::errc() || res.ptr != begin + field.size()) {
      std::cerr << "error: invalid GMDID '" << full << "', expected <arch.release[.revision]>\n";
      return false;
    }
    if (parts[n] > maximums[n]) {
      std::cerr << "error: invalid GMDID '" << full << "': " << names[n] << " " << parts[n]
                << " does not fit in the GMDID field, the maximum is " << maximums[n] << "\n";
      return false;
    }
    spec = (dot == std::string_view::npos) ? std::string_view() : spec.substr(dot + 1);
  }
  // Arch and release are mandatory; a non-empty remainder means too many fields
  if (n < 2 || !spec.empty()) {
    std::cerr << "error: invalid GMDID '" << full << "', expected <arch.release[.revision]>\n";
    return false;
  }

  g_gmdIdOverride = packGmdId(parts[0], parts[1], parts[2]);
  g_hasGmdIdOverride = true;
  return true;
}

// Check that the --lib argument is valid and points to a regular file
static bool isLibArgValid(std::string_view lib, std::string &resolvedPath) {
  if (lib.empty()) {
    std::cerr << "error: --lib requires a non-empty <path>\n";
    return false;
  }

  std::error_code ec;
  const auto path = std::filesystem::canonical(lib, ec);
  if (ec) {
    std::cerr << "error: cannot resolve '" << lib << "': " << ec.message() << "\n";
    return false;
  }

  if (!std::filesystem::is_regular_file(path, ec)) {
    std::cerr << "error: '" << lib << "' is not a regular file\n";
    return false;
  }
  resolvedPath = path.string();
  return true;
}

// Print help
static void printHelp(const char *testName) {
  std::cout << "IGCOCLInterfaceTester - test the NEO<->IGC CIF interfaces.\n";
  std::cout << "usage: " << testName << " [-h|--help] <check>\n";
  std::cout << "  <check> - name of the check to run (look below)\n";
  std::cout << "  --platform <name> - target platform for platform-dependent checks\n";
  std::cout << "  --gmdid <arch.release[.revision]> - render GMDID to report. Arch and release are\n";
  std::cout << "      mandatory, revision is optional and defaults to 0\n";
  std::cout << "  --core <name> - render core family to report, overriding the platform's own\n";
  std::cout << "  --lib <path> - path to the IGC library to load (default: " << IGC_TESTER_LIBRARY_PATH << ")\n";
  std::cout << "  --help, -h - print this help message\n";
  std::cout << "  Checks:\n";
  for (const auto &[name, info] : registry())
    std::cout << "    " << name << " - " << info.help << "\n";
  std::cout << "  Platforms:\n";
  for (const auto &p : platforms)
    std::cout << "    " << p.name << " (" << p.coreName << ")\n";
  // Core families are shared between platforms, so print each one only once.
  std::cout << "  Cores:\n";
  for (const auto &p : platforms) {
    bool alreadyPrinted = false;
    for (const auto &earlier : platforms) {
      if (&earlier == &p)
        break;
      alreadyPrinted = alreadyPrinted || earlier.core == p.core;
    }
    if (!alreadyPrinted)
      std::cout << "    " << p.coreName << "\n";
  }
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

  // libName by default is the path to the IGC library baked in at configure time
  // Can be overridden by setting via --lib argument
  std::string libName = IGC_TESTER_LIBRARY_PATH;

  // Parse options after the check name
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
    } else if (arg == "--core") {
      if (i + 1 >= argc) {
        std::cerr << "error: --core requires a <name> argument\n";
        return ExitCode::IllegalInputFormat;
      }
      if (!selectCore(argv[++i])) {
        std::cerr << "error: unknown core '" << argv[i] << "'\n";
        return ExitCode::IllegalInputFormat;
      }
    } else if (arg == "--gmdid") {
      if (i + 1 >= argc) {
        std::cerr << "error: --gmdid requires an <arch.release[.revision]> argument\n";
        return ExitCode::IllegalInputFormat;
      }
      if (!selectGmdId(argv[++i]))
        return ExitCode::IllegalInputFormat;
    } else if (arg == "--lib") {
      if (i + 1 >= argc) {
        std::cerr << "error: --lib requires a <path> argument\n";
        return ExitCode::MissingLibPath;
      }
      if (!isLibArgValid(argv[++i], libName)) {
        return ExitCode::InvalidLibPath;
      }
    } else {
      std::cerr << "error: unknown option '" << arg << "'\n";
      return ExitCode::UnknownOption;
    }
  }

  std::cerr << "info: using library '" << libName << "'\n";

  // Load the libigc
  auto cif = getCIFMain(libName.c_str());
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
