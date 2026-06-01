/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/igc_dump_paths.hpp"

#include <mutex>
#include <string>

#if defined(IGC_DEBUG_VARIABLES)

#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#include <process.h>
#endif

#if defined(ANDROID)
#include "common/SysUtils.hpp"
#endif

#include "common/secure_string.h"
#include "3d/common/iStdLib/File.h"
#include "Probe/Assertion.h"

#if !defined(IGC_FCL_BUILD)
#include "AdaptorCommon/customApi.hpp"
#endif

namespace {
std::string g_shaderOutputFolder;
}

namespace IGC {
namespace Debug {

static bool needMkDir() {
#ifdef IGC_FCL_BUILD
  // FCL always creates dump directories when dump is requested
  return true;
#else
  return IGC_IS_FLAG_ENABLED(DumpLLVMIR) || IGC_IS_FLAG_ENABLED(EnableCosDump) ||
         IGC_IS_FLAG_ENABLED(EnableVISAOutput) || IGC_IS_FLAG_ENABLED(EnableVISABinary) ||
         IGC_IS_FLAG_ENABLED(EnableVISADumpCommonISA) ||
         GetDebugFlag(DebugFlag::DUMP_AFTER_PASSES) || GetDebugFlag(DebugFlag::VISA_OUTPUT) ||
         GetDebugFlag(DebugFlag::VISA_BINARY) || GetDebugFlag(DebugFlag::VISA_DUMPCOMMONISA) ||
         IGC_IS_FLAG_ENABLED(EnableCapsDump) || IGC_IS_FLAG_ENABLED(ShaderOverride) ||
         IGC_IS_FLAG_ENABLED(GenerateOptionsFile);
#endif // IGC_FCL_BUILD
}

OutputFolderName GetBaseIGCOutputFolder() {
  static std::mutex m;
  std::lock_guard<std::mutex> lck(m);
  static std::string IGCBaseFolder;
  if (!IGCBaseFolder.empty()) {
    return IGCBaseFolder.c_str();
  }
#if defined(_WIN64) || defined(_WIN32)
  if (!IGC_IS_FLAG_ENABLED(DumpToCurrentDir) && !IGC_IS_FLAG_ENABLED(DumpToCustomDir)) {
    bool needMkdir = needMkDir();

    char dumpPath[256];

    sprintf_s(dumpPath, "c:\\Intel\\");
    if (GetFileAttributesA(dumpPath) != FILE_ATTRIBUTE_DIRECTORY && needMkdir) {
      _mkdir(dumpPath);
    }

    sprintf_s(dumpPath, "c:\\Intel\\IGC\\");
    if (GetFileAttributesA(dumpPath) != FILE_ATTRIBUTE_DIRECTORY && needMkdir) {
      _mkdir(dumpPath);
    }

    // Make sure we can write in the dump folder as the app may be sandboxed
    if (needMkdir) {
      int tmp_id = _getpid();
      std::string testFilename = std::string(dumpPath) + "testfile" + std::to_string(tmp_id);
      HANDLE testFile =
          CreateFileA(testFilename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
      if (testFile == INVALID_HANDLE_VALUE) {
        char temppath[256];
        if (GetTempPathA(sizeof(temppath), temppath) != 0) {
          sprintf_s(dumpPath, "%sIGC\\", temppath);
        }
      } else {
        CloseHandle(testFile);
      }
    }

    if (GetFileAttributesA(dumpPath) != FILE_ATTRIBUTE_DIRECTORY && needMkdir) {
      _mkdir(dumpPath);
    }

    IGCBaseFolder = dumpPath;
  } else if (IGC_IS_FLAG_ENABLED(DumpToCustomDir)) {
    std::string dumpPath = "c:\\Intel\\IGC\\"; // default if something goes wrong
    const char *custom_dir = IGC_GET_REGKEYSTRING(DumpToCustomDir);
    if (custom_dir != nullptr && strlen(custom_dir) > 0) {
      dumpPath = custom_dir;
    }

    char pathBuf[256];
    iSTD::CreateAppOutputDir(pathBuf, 256, dumpPath.c_str(), false, false, false);

    IGCBaseFolder = pathBuf;
  }
#elif defined ANDROID

  if (IGC_IS_FLAG_ENABLED(DumpToCurrentDir))
    return "";
  IGCBaseFolder = "/sdcard/intel/igc/";

#elif defined __linux__
  if (!IGC_IS_FLAG_ENABLED(DumpToCustomDir)) {
    IGCBaseFolder = "/tmp/IntelIGC/";
  } else {
    std::string dumpPath = "/tmp/IntelIGC/"; // default if something goes wrong
    const char *custom_dir = IGC_GET_REGKEYSTRING(DumpToCustomDir);
    if (custom_dir != nullptr && strlen(custom_dir) > 0) {
      dumpPath = custom_dir;
      dumpPath += "/";
    }

    char pathBuf[256];
    iSTD::CreateAppOutputDir(pathBuf, 256, dumpPath.c_str(), false, false, false);

    IGCBaseFolder = pathBuf;
  }
#endif
  return IGCBaseFolder.c_str();
}

OutputFolderName GetShaderOutputFolder() {
  static std::mutex m;
  std::lock_guard<std::mutex> lck(m);
  if (!g_shaderOutputFolder.empty() &&
      doesRegexMatch(g_shaderOutputFolder, IGC_GET_REGKEYSTRING(ShaderDumpRegexFilter))) {
    return g_shaderOutputFolder.c_str();
  }
#if defined(_WIN64) || defined(_WIN32)
  if (!IGC_IS_FLAG_ENABLED(DumpToCurrentDir) && !IGC_IS_FLAG_ENABLED(DumpToCustomDir)) {
    char dumpPath[256];
    sprintf_s(dumpPath, "%s", GetBaseIGCOutputFolder());
    char appPath[MAX_PATH] = {0};
    // check a process id and make an adequate directory for it:

    if (::GetModuleFileNameA(NULL, appPath, sizeof(appPath) - 1)) {
      std::string appPathStr = std::string(appPath);
      int pos = appPathStr.find_last_of("\\") + 1;

      if (IGC_IS_FLAG_ENABLED(ShaderDumpPidDisable)) {
        sprintf_s(dumpPath, "%s%s\\", dumpPath, appPathStr.substr(pos, MAX_PATH).c_str());
      } else {
        sprintf_s(dumpPath, "%s%s_%d\\", dumpPath, appPathStr.substr(pos, MAX_PATH).c_str(), _getpid());
      }
    } else {
      sprintf_s(dumpPath, "%sunknownProcess_%d\\", dumpPath, _getpid());
    }

    if (needMkDir() && doesRegexMatch(dumpPath, IGC_GET_REGKEYSTRING(ShaderDumpRegexFilter))) {
      if (GetFileAttributesA(dumpPath) != FILE_ATTRIBUTE_DIRECTORY) {
        _mkdir(dumpPath);
      }
      g_shaderOutputFolder = dumpPath;
    } else {
      // To make the path always invalid.
      g_shaderOutputFolder = "NUL\\";
    }
  } else if (IGC_IS_FLAG_ENABLED(DumpToCustomDir)) {
    char pathBuf[256];
    iSTD::CreateAppOutputDir(pathBuf, 256, GetBaseIGCOutputFolder(), false, true,
                             !IGC_IS_FLAG_ENABLED(ShaderDumpPidDisable));
    g_shaderOutputFolder = pathBuf;
  }
#elif defined ANDROID

  if (IGC_IS_FLAG_ENABLED(DumpToCurrentDir))
    return "";

  if (!SysUtils::CreateDir(GetBaseIGCOutputFolder(), true, IGC_IS_FLAG_DISABLED(ShaderDumpPidDisable),
                           &g_shaderOutputFolder))

    g_shaderOutputFolder = "";

#elif defined __linux__
  if (!IGC_IS_FLAG_ENABLED(DumpToCurrentDir) && g_shaderOutputFolder.empty() && !IGC_IS_FLAG_ENABLED(DumpToCustomDir)) {
    bool needMkdir =
        needMkDir() && doesRegexMatch(GetBaseIGCOutputFolder(), IGC_GET_REGKEYSTRING(ShaderDumpRegexFilter));

    char path[MAX_PATH] = {0};
    bool pidEnabled = !IGC_IS_FLAG_ENABLED(ShaderDumpPidDisable);

    if (needMkdir) {
      iSTD::CreateAppOutputDir(path, MAX_PATH, GetBaseIGCOutputFolder(), false, true, pidEnabled);
      g_shaderOutputFolder = path;
    } else {
      // To make the path always invalid.
      g_shaderOutputFolder = "/dev/null/";
    }
  } else if (IGC_IS_FLAG_ENABLED(DumpToCustomDir)) {
    char pathBuf[256];
    iSTD::CreateAppOutputDir(pathBuf, 256, GetBaseIGCOutputFolder(), false, false, false);
    g_shaderOutputFolder = pathBuf;
  }

#endif
  return g_shaderOutputFolder.c_str();
}

void IGC_DEBUG_API_CALL SetShaderOutputFolder(OutputFolderName name) { g_shaderOutputFolder = name; }

} // namespace Debug
} // namespace IGC

#endif // defined(IGC_DEBUG_VARIABLES)
