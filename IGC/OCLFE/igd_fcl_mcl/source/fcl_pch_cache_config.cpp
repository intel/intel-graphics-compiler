/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "OCLFE/igd_fcl_mcl/headers/fcl_pch_cache_config.h"

#include "common/igc_regkeys.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/FileSystem.h"
#include "common/LLVMWarningsPop.hpp"

#include <cstdlib>

#if defined(_WIN32)
#include <Windows.h>
#else
#include <sys/stat.h>
#endif

namespace TC {
namespace {

bool DirExists(const std::string &path) {
#if defined(_WIN32)
  DWORD attrs = GetFileAttributesA(path.c_str());
  return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
  struct stat info{};
  return stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
#endif
}

std::string JoinPath(const std::string &base, const std::string &leaf) {
#if defined(_WIN32)
  const char separator = '\\';
#else
  const char separator = '/';
#endif
  if (base.empty() || base.back() == '/' || base.back() == '\\')
    return base + leaf;
  return base + separator + leaf;
}

std::string MakeFclPchCacheDir(const std::string &baseDir) {
  if (baseDir.empty())
    return {};

  const std::string cacheDir = JoinPath(baseDir, "igc_fcl_pch_cache");
  return llvm::sys::fs::create_directories(cacheDir) ? std::string() : cacheDir;
}

std::string GetDefaultFclPchCacheBaseDir() {
#if defined(_WIN32)
  const char *localAppData = std::getenv("LOCALAPPDATA");
  if (!localAppData || !*localAppData || !DirExists(localAppData))
    return {};

  return JoinPath(localAppData, "IntelIGC");
#else
  const char *xdgCacheHome = std::getenv("XDG_CACHE_HOME");
  if (xdgCacheHome && *xdgCacheHome && DirExists(xdgCacheHome))
    return xdgCacheHome;

  const char *home = std::getenv("HOME");
  return home && *home ? JoinPath(home, ".cache") : std::string();
#endif
}

std::string ComputeDefaultFclPchCacheDir() { return MakeFclPchCacheDir(GetDefaultFclPchCacheBaseDir()); }

} // namespace

std::string ResolveFclPchCacheDir() {
#if defined(IGC_DEBUG_VARIABLES)
  debugString overrideDir = {0};
  ReadIGCRegistry("FclPchCacheDirOverride", overrideDir, sizeof(overrideDir), IGCFlagType_debugString);
  if (overrideDir[0] != '\0')
    return DirExists(overrideDir) ? std::string(overrideDir) : std::string();

  return ComputeDefaultFclPchCacheDir();
#else
  return {};
#endif
}

unsigned long long ResolveFclPchCacheMaxSizeBytes() {
#if defined(IGC_DEBUG_VARIABLES)
  DWORD maxSizeMB = 0;
  ReadIGCRegistry("FclPchCacheMaxSizeMBOverride", &maxSizeMB, sizeof(maxSizeMB), IGCFlagType_int);
  return maxSizeMB == 0 ? 0ull : static_cast<unsigned long long>(maxSizeMB) * 1024ull * 1024ull;
#else
  return 0;
#endif
}

} // namespace TC
