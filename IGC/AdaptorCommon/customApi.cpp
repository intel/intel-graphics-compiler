/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorCommon/customApi.hpp"
#include <mutex>

#if defined(_WIN32) || defined(_WIN64)
#include "Windows.h"
#include <direct.h>
#include <wtypes.h>
#include <process.h>
#endif

#include "common/debug/Debug.hpp"
#include "common/Stats.hpp"
#include "common/igc_regkeys.hpp"
#include "common/SysUtils.hpp"
#include "common/secure_string.h" // strcpy_s()
#include "Probe/Assertion.h"

#if defined(IGC_DEBUG_VARIABLES)
#include "3d/common/iStdLib/File.h"
#endif

namespace {

#ifndef DRIVER_BUILD_ID
#define DRIVER_BUILD_ID "<undefined>"
#endif

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#if defined(_DEBUG)
#if defined(_INTERNAL)
#define CONFIGURATION DebugInternal
#else
#define CONFIGURATION Debug
#endif
#else
#if defined(_INTERNAL)
#define CONFIGURATION ReleaseInternal
#else
#define CONFIGURATION Release
#endif
#endif

#if defined(_WIN64)
#define BITWIDTH x64
#elif defined(_WIN32)
#define BITWIDTH Win32
#else
#define BITWIDTH Unknown
#endif

// We don't want this static string in the RELEASE builds because it can be discovered using the
// Windows SysInternals Strings tool. We don't want to leak such information. Unfortunately this
// means that RELEASE builds can't be queried for their version information. There is no middle ground.
#if defined(_DEBUG) || defined(_INTERNAL)
static const char g_cBuildInfo[] =
    "BUILD ID: " STRINGIFY(DRIVER_BUILD_ID) ", "
                                            "CONFIGURATION: " STRINGIFY(CONFIGURATION) " " STRINGIFY(BITWIDTH) "\0";
#else
static const char g_cBuildInfo[] = "\0";
#endif

#undef CONFIGURATION

static bool g_debugFlags[static_cast<int>(IGC::Debug::DebugFlag::END)] = {0};

static struct {
  bool dumpODS;
  bool dumpFile;
} g_dumpFlags[static_cast<int>(IGC::Debug::DumpType::END)] = {};

std::string g_shaderCorpusName;
std::string g_shaderOutputName;

} // namespace

namespace IGC {
namespace Debug {

#if defined(IGC_DEBUG_VARIABLES)

EnumStr IGC_DEBUG_API_CALL str(DebugFlag value) {
#define CASE(x)                                                                                                        \
  case DebugFlag::x:                                                                                                   \
    return STRINGIFY(x)
  switch (value) {
    CASE(DUMPS);
    CASE(DUMP_AFTER_PASSES);
    CASE(DUMP_TO_OUTS);
    CASE(DUMP_TO_OUTPUTDEBUGSTRING);
    CASE(OPTIMIZATION_STATS);
    CASE(TIME_STATS_SUM);
    CASE(TIME_STATS_PER_SHADER);
    CASE(TIME_STATS_COARSE);
    CASE(TIME_STATS_PER_PASS);
    CASE(MEM_STATS);
    CASE(MEM_STATS_DETAIL);
    CASE(SHADER_QUALITY_METRICS);
    CASE(SIMD8_ONLY);
    CASE(SIMD16_ONLY);
    CASE(SIMD32_ONLY);
    CASE(VISA_OUTPUT);
    CASE(VISA_BINARY);
    CASE(VISA_DUMPCOMMONISA);
    CASE(VISA_NOSCHEDULE);
    CASE(VISA_DOTALL);
    CASE(VISA_SLOWPATH);
    CASE(VISA_NOBXMLENCODER);
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "unknown DebugFlag");
    return "<unknown>";
  }
#undef CASE
}

EnumStr IGC_DEBUG_API_CALL str(DumpType value) {
#define CASE(x)                                                                                                        \
  case DumpType::x:                                                                                                    \
    return STRINGIFY(x)
  switch (value) {
    CASE(ASM_TEXT);
    CASE(ASM_BC);
    CASE(TRANSLATED_IR_TEXT);
    CASE(TRANSLATED_IR_BC);
    CASE(PASS_IR_TEXT);
    CASE(PASS_IR_BC);
    CASE(OptIR_TEXT);
    CASE(OptIR_BC);
    CASE(VISA_TEXT);
    CASE(VISA_BC);
    CASE(GENX_ISA_TEXT);
    CASE(GENX_ISA_BC);
    CASE(LLVM_OPT_STAT_TEXT);
    CASE(TIME_STATS_TEXT);
    CASE(TIME_STATS_CSV);
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "unknown DumpType");
    return "<unknown>";
  }
#undef CASE
}

EnumStr IGC_DEBUG_API_CALL str(DumpLoc value) {
#define CASE(x)                                                                                                        \
  case DumpLoc::x:                                                                                                     \
    return STRINGIFY(x)
  switch (value) {
    CASE(ODS);
    CASE(FILE);
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "unknown DumpLoc");
    return "<unknown>";
  }
#undef CASE
}

void IGC_DEBUG_API_CALL SetCompilerOption(OptionFlag flag, debugString s) {
  switch (flag) {
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode)                               \
  case OptionFlag::OPTION_##regkeyName:                                                                                \
    strcpy_s(IGC_GET_REGKEY(regkeyName).m_string, sizeof(debugString), s);                                             \
    break;
#include "common/igc_regkeys.h"
#undef DECLARE_IGC_REGKEY
  default:
    break;
  }
}

void IGC_DEBUG_API_CALL SetCompilerOption(OptionFlag flag, int value) {
  switch (flag) {
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode)                               \
  case OptionFlag::OPTION_##regkeyName:                                                                                \
    IGC_GET_REGKEY(regkeyName).m_Value = value;                                                                        \
    break;
#include "common/igc_regkeys.h"
#undef DECLARE_IGC_REGKEY
  default:
    break;
  }
}

template <typename dataType, typename RegkeyT>
inline void SetCompilerOptionOpaqueHelper(RegkeyT &regkeyName, const dataType *data) {
  memcpy_s(&regkeyName.m_Value, sizeof(dataType), data, sizeof(dataType));
}

template <typename RegkeyT> inline void SetCompilerOptionOpaqueHelper(RegkeyT &regkeyName, debugString data) {
  strcpy_s(&regkeyName.m_string, sizeof(debugString), data);
}

void IGC_DEBUG_API_CALL SetCompilerOptionOpaque(OptionFlag flag, void *data) {
  switch (flag) {
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode)                               \
  case OptionFlag::OPTION_##regkeyName:                                                                                \
    SetCompilerOptionOpaqueHelper(IGC_GET_REGKEY(regkeyName), reinterpret_cast<dataType *>(data));                     \
    IGC_GET_REGKEY(regkeyName).isSet = true;                                                                           \
    break;
#include "common/igc_regkeys.h"
#undef DECLARE_IGC_REGKEY
  default:
    break;
  }
}

extern "C" void IGC_DEBUG_API_CALL SetCompilerOptionValue(const char *flagName, int value) {
  IGCFlag *flag = FindIGCFlagByName(flagName);
  if (!flag || !flag->IsNumber())
    return;

  flag->m_Value = value;
  flag->isSet = true;
}

extern "C" void IGC_DEBUG_API_CALL SetCompilerOptionString(const char *flagName, debugString s) {
  IGCFlag *flag = FindIGCFlagByName(flagName);
  if (!flag || !flag->IsString())
    return;

  strcpy_s(flag->m_string, sizeof(debugString), s);
  flag->isSet = true;
}

void IGC_DEBUG_API_CALL SetDebugFlag(DebugFlag flag, bool enabled) {
  IGC_ASSERT_EXIT_MESSAGE((0 <= static_cast<int>(flag)), "range sanity check");
  IGC_ASSERT_EXIT_MESSAGE((static_cast<int>(flag) < static_cast<int>(DebugFlag::END)), "range sanity check");

  g_debugFlags[static_cast<int>(flag)] = enabled;
}

bool IGC_DEBUG_API_CALL GetDebugFlag(DebugFlag flag) {
  IGC_ASSERT_EXIT_MESSAGE((0 <= static_cast<int>(flag)), "range sanity check");
  IGC_ASSERT_EXIT_MESSAGE((static_cast<int>(flag) < static_cast<int>(DebugFlag::END)), "range sanity check");

#if defined(_WIN32) || defined(_WIN64)
  // Disable Dump  for OS Applications
  if ((DebugFlag::VISA_OUTPUT == flag) || (DebugFlag::VISA_BINARY == flag) || (DebugFlag::VISA_DUMPCOMMONISA == flag)) {
    if (GetModuleHandleA("dwm.exe") || GetModuleHandleA("explorer.exe")) {
      return false;
    }
  }
#endif
  return g_debugFlags[static_cast<int>(flag)];
}

void IGC_DEBUG_API_CALL SetDumpFlag(DumpType type, DumpLoc loc, bool enabled) {
  IGC_ASSERT_EXIT_MESSAGE((0 <= static_cast<int>(type)), "range sanity check");
  IGC_ASSERT_EXIT_MESSAGE((static_cast<int>(type) < static_cast<int>(DumpType::END)), "range sanity check");

  switch (loc) {
  case DumpLoc::ODS:
    g_dumpFlags[static_cast<int>(type)].dumpODS = enabled;
    break;
  case DumpLoc::FILE:
    g_dumpFlags[static_cast<int>(type)].dumpFile = enabled;
    break;
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "unreachable");
    break;
  }
}

bool IGC_DEBUG_API_CALL GetDumpFlag(DumpType type, DumpLoc loc) {
  IGC_ASSERT_EXIT_MESSAGE((0 <= static_cast<int>(type)), "range sanity check");
  IGC_ASSERT_EXIT_MESSAGE((static_cast<int>(type) < static_cast<int>(DumpType::END)), "range sanity check");

#if defined(_WIN32) || defined(_WIN64)
  // Disable Dump  for OS Applications
  if (GetModuleHandleA("dwm.exe") || GetModuleHandleA("explorer.exe")) {
    return false;
  }
#endif
  switch (loc) {
  case DumpLoc::ODS:
    return g_dumpFlags[static_cast<int>(type)].dumpODS;
  case DumpLoc::FILE:
    return g_dumpFlags[static_cast<int>(type)].dumpFile;
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "unreachable");
    return false;
  }
}

void IGC_DEBUG_API_CALL SetShaderCorpusName(CorpusName name) { g_shaderCorpusName = name; }

CorpusName IGC_DEBUG_API_CALL GetShaderCorpusName() { return g_shaderCorpusName.c_str(); }

void IGC_DEBUG_API_CALL SetShaderOutputName(OutputName name) { g_shaderOutputName = name; }

std::string &GetShaderOverridePathString() {
  static std::string path = []() -> std::string {
    if (IGC_IS_FLAG_DISABLED(ShaderOverride)) {
      return "";
    }
    const char *customDir = IGC_GET_REGKEYSTRING(ShaderOverrideFromDir);
    if (customDir != nullptr && strnlen_s(customDir, sizeof(debugString)) > 0) {
      std::string p = customDir;
      if (!p.empty() && p.back() != '/' && p.back() != '\\') {
        p += '/';
      }
      return p;
    }
    return std::string(GetBaseIGCOutputFolder()) + "ShaderOverride/";
  }();
  return path;
}

void IGC_DEBUG_API_CALL SetShaderOverridePath(OutputFolderName pOutputFolderName) {
  static std::mutex m;
  std::lock_guard<std::mutex> lck(m);
  std::string &path = GetShaderOverridePathString();
  if (pOutputFolderName != nullptr) {
    path = pOutputFolderName;
  } else {
    path = "";
  }
}

OutputFolderName IGC_DEBUG_API_CALL GetShaderOverridePath() {
  // Return overridePath even though ShaderOverride is not set.
  std::string &overridePath = GetShaderOverridePathString();
  return overridePath.c_str();
}

OutputName IGC_DEBUG_API_CALL GetFunctionDebugFile() {
  if (IGC_GET_FLAG_VALUE(SelectiveFunctionControl) != 0) {
    static std::mutex m;
    std::lock_guard<std::mutex> lck(m);
    // If a custom file is specified by SelectiveFunctionControlFile
    // then use that for SelectiveFunctionControl. Otherwise,
    // fallback to FunctionDebug.txt in IGC output folder.
    static std::string functionDebugFilePath = IGC_GET_REGKEYSTRING(SelectiveFunctionControlFile);
    if (functionDebugFilePath == "") {
      functionDebugFilePath = std::string(GetBaseIGCOutputFolder()) + "FunctionDebug.txt";
    }
    return functionDebugFilePath.c_str();
  }
  return "";
}

OutputName IGC_DEBUG_API_CALL GetShaderOutputName() { return g_shaderOutputName.c_str(); }

VersionInfo IGC_DEBUG_API_CALL GetVersionInfo() { return g_cBuildInfo; }
#else // defined( IGC_DEBUG_VARIABLES )
      // C extern inline needs extern definition
extern "C" void IGC_DEBUG_API_CALL SetCompilerOptionValue(const char *flagName, int value);
extern "C" void IGC_DEBUG_API_CALL SetCompilerOptionString(const char *flagName, debugString s);

void IGC_DEBUG_API_CALL SetCompilerOptionOpaque(OptionFlag flag, void *data);


#endif // defined( IGC_DEBUG_VARIABLES )
} // namespace Debug
} // namespace IGC
