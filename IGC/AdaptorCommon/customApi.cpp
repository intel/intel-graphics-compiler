/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorCommon/customApi.hpp"
#include <mutex>

#if defined(_WIN32 )|| defined( _WIN64 )
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

#if defined( _DEBUG )
#   if defined( _INTERNAL )
#       define CONFIGURATION DebugInternal
#   else
#       define CONFIGURATION Debug
#   endif
#else
#   if defined( _INTERNAL )
#       define CONFIGURATION ReleaseInternal
#   else
#       define CONFIGURATION Release
#   endif
#endif

#if defined( _WIN64 )
#   define BITWIDTH x64
#elif defined( _WIN32 )
#   define BITWIDTH Win32
#else
#   define BITWIDTH Unknown
#endif

// We don't want this static string in the RELEASE builds because it can be discovered using the
// Windows SysInternals Strings tool. We don't want to leak such information. Unfortunately this
// means that RELEASE builds can't be queried for their version information. There is no middle ground.
#if defined( _DEBUG ) || defined( _INTERNAL )
static const char g_cBuildInfo[] =
    "BUILD ID: " STRINGIFY(DRIVER_BUILD_ID) ", "
    "CONFIGURATION: " STRINGIFY(CONFIGURATION) " " STRINGIFY(BITWIDTH) "\0";
#else
static const char g_cBuildInfo[] = "\0";
#endif

#undef CONFIGURATION

static bool g_debugFlags[ static_cast<int>( IGC::Debug::DebugFlag::END ) ] = {0};

static struct
{
    bool dumpODS;
    bool dumpFile;
} g_dumpFlags[ static_cast<int>( IGC::Debug::DumpType::END ) ] = { };

std::string g_shaderCorpusName;
std::string g_shaderOutputFolder;
std::string g_shaderOutputName;

}

namespace IGC
{
    namespace Debug
    {

#if defined( IGC_DEBUG_VARIABLES )

        EnumStr IGC_DEBUG_API_CALL str(DebugFlag value)
        {
#define CASE(x) case DebugFlag::x: return STRINGIFY(x)
            switch (value)
            {
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
            default : IGC_ASSERT_EXIT_MESSAGE(0, "unknown DebugFlag"); return "<unknown>";
            }
#undef CASE
        }

        EnumStr IGC_DEBUG_API_CALL str(DumpType value)
        {
#define CASE(x) case DumpType::x: return STRINGIFY(x)
            switch (value)
            {
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
            default : IGC_ASSERT_EXIT_MESSAGE(0, "unknown DumpType"); return "<unknown>";
            }
#undef CASE
        }

        EnumStr IGC_DEBUG_API_CALL str(DumpLoc value)
        {
#define CASE(x) case DumpLoc::x: return STRINGIFY(x)
            switch (value)
            {
                CASE(ODS);
                CASE(FILE);
            default : IGC_ASSERT_EXIT_MESSAGE(0, "unknown DumpLoc"); return "<unknown>";
            }
#undef CASE
        }

        void IGC_DEBUG_API_CALL SetCompilerOption(OptionFlag flag, debugString s)
        {
            switch(flag)
            {
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode) \
            case OptionFlag::OPTION_##regkeyName: \
            strcpy_s(g_RegKeyList.regkeyName.m_string, sizeof(debugString), s);   \
            break;
#include "common/igc_regkeys.h"
#undef DECLARE_IGC_REGKEY
            default:
                break;
            }
        }

        void IGC_DEBUG_API_CALL SetCompilerOption( OptionFlag flag, int value )
        {
            switch(flag)
            {
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode) \
            case OptionFlag::OPTION_##regkeyName: \
                g_RegKeyList.regkeyName.m_Value = value;   \
            break;
#include "common/igc_regkeys.h"
#undef DECLARE_IGC_REGKEY
            default:
                break;
            }
        }

        template<typename dataType, typename RegkeyT>
        inline void SetCompilerOptionOpaqueHelper(RegkeyT& regkeyName, const dataType* data)
        {
            memcpy_s(&regkeyName.m_Value, sizeof(dataType), data, sizeof(dataType));
        }

        template<typename RegkeyT>
        inline void SetCompilerOptionOpaqueHelper(RegkeyT& regkeyName, debugString data)
        {
            strcpy_s(&regkeyName.m_string, sizeof(debugString), data);
        }

        void IGC_DEBUG_API_CALL SetCompilerOptionOpaque(OptionFlag flag, void* data)
        {
            switch (flag)
            {
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode) \
            case OptionFlag::OPTION_##regkeyName: \
                SetCompilerOptionOpaqueHelper(g_RegKeyList.regkeyName, reinterpret_cast<dataType*>(data)); \
            break;
#include "common/igc_regkeys.h"
#undef DECLARE_IGC_REGKEY
            default:
                break;
            }
        }

        extern "C" void IGC_DEBUG_API_CALL SetCompilerOptionValue( const char* flagName, int value )
        {
            if (!flagName)
            {
                return;
            }

            SRegKeyVariableMetaData* pRegKeyVariable = (SRegKeyVariableMetaData*)&g_RegKeyList;
            unsigned NUM_REGKEY_ENTRIES = sizeof(SRegKeysList) / sizeof(SRegKeyVariableMetaData);
            for (DWORD i = 0; i < NUM_REGKEY_ENTRIES; i++)
            {
                const char* name = pRegKeyVariable[i].GetName();

                if (!strcmp(flagName, name))
                {
                    pRegKeyVariable[i].m_Value = value;
                    pRegKeyVariable[i].Set();
                    break;
                }
            }
        }

        extern "C" void IGC_DEBUG_API_CALL SetCompilerOptionString( const char* flagName, debugString s )
        {
            if (!flagName)
            {
                return;
            }

            SRegKeyVariableMetaData* pRegKeyVariable = (SRegKeyVariableMetaData*)&g_RegKeyList;
            unsigned NUM_REGKEY_ENTRIES = sizeof(SRegKeysList) / sizeof(SRegKeyVariableMetaData);
            for (DWORD i = 0; i < NUM_REGKEY_ENTRIES; i++)
            {
                const char* name = pRegKeyVariable[i].GetName();

                if (!strcmp(flagName, name))
                {
                    strcpy_s(pRegKeyVariable[i].m_string,sizeof(debugString), s);
                    pRegKeyVariable[i].Set();
                    break;
                }
            }
        }

        void IGC_DEBUG_API_CALL SetDebugFlag(DebugFlag flag, bool enabled)
        {
            IGC_ASSERT_EXIT_MESSAGE((0 <= static_cast<int>(flag)), "range sanity check");
            IGC_ASSERT_EXIT_MESSAGE((static_cast<int>(flag) < static_cast<int> (DebugFlag::END)), "range sanity check");

            g_debugFlags[ static_cast<int>( flag ) ] = enabled;
        }

        bool IGC_DEBUG_API_CALL GetDebugFlag( DebugFlag flag )
        {
            IGC_ASSERT_EXIT_MESSAGE((0 <= static_cast<int>(flag)), "range sanity check");
            IGC_ASSERT_EXIT_MESSAGE((static_cast<int>(flag) < static_cast<int> (DebugFlag::END)), "range sanity check");

#if defined(_WIN32 )|| defined( _WIN64 )
            //Disable Dump  for OS Applications
            if ( (DebugFlag::VISA_OUTPUT == flag) ||
                 (DebugFlag::VISA_BINARY == flag)  ||
                 (DebugFlag::VISA_DUMPCOMMONISA == flag)
                )
            {
                if (GetModuleHandleA("dwm.exe") || GetModuleHandleA("explorer.exe"))
                {
                    return false;
                }

            }
#endif
            return g_debugFlags[ static_cast<int>(flag) ];
        }

        void IGC_DEBUG_API_CALL SetDumpFlag(DumpType type, DumpLoc loc, bool enabled)
        {
            IGC_ASSERT_EXIT_MESSAGE((0 <= static_cast<int>(type)), "range sanity check");
            IGC_ASSERT_EXIT_MESSAGE((static_cast<int>(type) < static_cast<int> (DumpType::END)), "range sanity check");

            switch (loc)
            {
            case DumpLoc::ODS  : g_dumpFlags[ static_cast<int>(type) ].dumpODS  = enabled; break;
            case DumpLoc::FILE : g_dumpFlags[ static_cast<int>(type) ].dumpFile = enabled; break;
            default            : IGC_ASSERT_EXIT_MESSAGE(0, "unreachable"); break;
            }
        }

        bool IGC_DEBUG_API_CALL GetDumpFlag( DumpType type, DumpLoc loc )
        {
            IGC_ASSERT_EXIT_MESSAGE((0 <= static_cast<int>(type)), "range sanity check");
            IGC_ASSERT_EXIT_MESSAGE((static_cast<int>(type) < static_cast<int> (DumpType::END)), "range sanity check");

#if defined(_WIN32 )|| defined( _WIN64 )
            //Disable Dump  for OS Applications
            if (GetModuleHandleA("dwm.exe") || GetModuleHandleA("explorer.exe"))
            {
                return false;
            }
#endif
            switch (loc)
            {
            case DumpLoc::ODS  : return g_dumpFlags[ static_cast<int>(type) ].dumpODS;
            case DumpLoc::FILE : return g_dumpFlags[ static_cast<int>(type) ].dumpFile;
            default            : IGC_ASSERT_EXIT_MESSAGE(0, "unreachable"); return false;
            }
        }

        void IGC_DEBUG_API_CALL SetShaderCorpusName(CorpusName name)
        {
            g_shaderCorpusName = name;
        }

        CorpusName IGC_DEBUG_API_CALL GetShaderCorpusName()
        {
            return g_shaderCorpusName.c_str();
        }

        void IGC_DEBUG_API_CALL SetShaderOutputFolder(OutputFolderName name)
        {
            g_shaderOutputFolder = name;
        }
        void IGC_DEBUG_API_CALL SetShaderOutputName( OutputName name )
        {
            g_shaderOutputName = name;
        }

        bool needMkDir()
        {
            return IGC_IS_FLAG_ENABLED(DumpLLVMIR)              ||
                   IGC_IS_FLAG_ENABLED(EnableCosDump)           ||
                   IGC_IS_FLAG_ENABLED(EnableVISAOutput)        ||
                   IGC_IS_FLAG_ENABLED(EnableVISABinary)        ||
                   IGC_IS_FLAG_ENABLED(EnableVISADumpCommonISA) ||
                   GetDebugFlag(DebugFlag::DUMP_AFTER_PASSES)   ||
                   GetDebugFlag(DebugFlag::VISA_OUTPUT)         ||
                   GetDebugFlag(DebugFlag::VISA_BINARY)         ||
                   GetDebugFlag(DebugFlag::VISA_DUMPCOMMONISA)  ||
                   IGC_IS_FLAG_ENABLED(EnableCapsDump)          ||
                   IGC_IS_FLAG_ENABLED(ShaderOverride)          ||
                   IGC_IS_FLAG_ENABLED(GenerateOptionsFile);
        }

        OutputFolderName IGC_DEBUG_API_CALL GetBaseIGCOutputFolder()
        {
            static std::mutex m;
            std::lock_guard<std::mutex> lck(m);
            static std::string IGCBaseFolder;
            if(IGCBaseFolder != "")
            {
                return IGCBaseFolder.c_str();
            }
#   if defined(_WIN64) || defined(_WIN32)
            if (!IGC_IS_FLAG_ENABLED(DumpToCurrentDir) && !IGC_IS_FLAG_ENABLED(DumpToCustomDir))
            {
                bool needMkdir = needMkDir();

                char dumpPath[256];

                sprintf_s(dumpPath, "c:\\Intel\\");
                if (GetFileAttributesA(dumpPath) != FILE_ATTRIBUTE_DIRECTORY && needMkdir)
                {
                    _mkdir(dumpPath);
                }

                sprintf_s(dumpPath, "c:\\Intel\\IGC\\");
                if(GetFileAttributesA(dumpPath) != FILE_ATTRIBUTE_DIRECTORY && needMkdir)
                {
                    _mkdir(dumpPath);
                }

                // Make sure we can write in the dump folder as the app may be sandboxed
                if(needMkdir)
                {
                    int tmp_id = _getpid();
                    std::string testFilename = std::string(dumpPath) + "testfile" + std::to_string(tmp_id);
                    HANDLE testFile =
                        CreateFileA(testFilename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
                    if(testFile == INVALID_HANDLE_VALUE)
                    {
                        char temppath[256];
                        if(GetTempPathA(sizeof(temppath), temppath) != 0)
                        {
                            sprintf_s(dumpPath, "%sIGC\\", temppath);
                        }
                    }
                    else
                    {
                        CloseHandle(testFile);
                    }
                }

                if(GetFileAttributesA(dumpPath) != FILE_ATTRIBUTE_DIRECTORY && needMkdir)
                {
                    _mkdir(dumpPath);
                }

                IGCBaseFolder = dumpPath;
            }
            else if (IGC_IS_FLAG_ENABLED(DumpToCustomDir))
            {
                std::string dumpPath = "c:\\Intel\\IGC\\";        // default if something goes wrong
                const char* custom_dir = IGC_GET_REGKEYSTRING(DumpToCustomDir);
                if (custom_dir != nullptr && strlen(custom_dir) > 0)
                {
                    dumpPath = custom_dir;
                }

                char pathBuf[256];
                iSTD::CreateAppOutputDir(pathBuf, 256, dumpPath.c_str(), false, false, false);

                IGCBaseFolder = pathBuf;
            }
#elif defined ANDROID

            if(IGC_IS_FLAG_ENABLED(DumpToCurrentDir))
                return "";
            IGCBaseFolder = "/sdcard/intel/igc/";

#elif defined __linux__
        if (!IGC_IS_FLAG_ENABLED(DumpToCustomDir))
        {
            IGCBaseFolder = "/tmp/IntelIGC/";
        }
        else
        {
            std::string dumpPath = "/tmp/IntelIGC/";        // default if something goes wrong
            const char* custom_dir = IGC_GET_REGKEYSTRING(DumpToCustomDir);
            if (custom_dir != nullptr && strlen(custom_dir) > 0)
            {
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

        std::string& GetShaderOverridePathString()
        {
            static std::string path = IGC_IS_FLAG_ENABLED(ShaderOverride) ?
                std::string(GetBaseIGCOutputFolder()) + "ShaderOverride/" :
                "";
            return path;
        }

        void IGC_DEBUG_API_CALL SetShaderOverridePath(OutputFolderName pOutputFolderName)
        {
            static std::mutex m;
            std::lock_guard<std::mutex> lck(m);
            std::string& path = GetShaderOverridePathString();
            if (pOutputFolderName != nullptr)
            {
                path = pOutputFolderName;
            }
            else
            {
                path = "";
            }
        }

        OutputFolderName IGC_DEBUG_API_CALL GetShaderOverridePath()
        {
            // Return overridePath even though ShaderOverride is not set.
            std::string& overridePath = GetShaderOverridePathString();
            return overridePath.c_str();
        }

        OutputFolderName IGC_DEBUG_API_CALL GetShaderOutputFolder()
        {
            static std::mutex m;
            std::lock_guard<std::mutex> lck(m);
            if(g_shaderOutputFolder != "" && doesRegexMatch(g_shaderOutputFolder, IGC_GET_REGKEYSTRING(ShaderDumpRegexFilter)))
            {
                return g_shaderOutputFolder.c_str();
            }
#   if defined(_WIN64) || defined(_WIN32)
            if (!IGC_IS_FLAG_ENABLED(DumpToCurrentDir) && !IGC_IS_FLAG_ENABLED(DumpToCustomDir))
            {
                char dumpPath[256];
                sprintf_s(dumpPath, "%s", GetBaseIGCOutputFolder());
                char appPath[MAX_PATH] = { 0 };
                // check a process id and make an adequate directory for it:

                if (::GetModuleFileNameA(NULL, appPath, sizeof(appPath)-1))
                {
                    std::string appPathStr = std::string(appPath);
                    int pos = appPathStr.find_last_of("\\") + 1;

                    if (IGC_IS_FLAG_ENABLED(ShaderDumpPidDisable))
                    {
                        sprintf_s(dumpPath, "%s%s\\", dumpPath, appPathStr.substr(pos, MAX_PATH).c_str());
                    }
                    else
                    {
                        sprintf_s(dumpPath, "%s%s_%d\\", dumpPath, appPathStr.substr(pos, MAX_PATH).c_str(), _getpid());
                    }
                }
                else
                {
                    sprintf_s(dumpPath, "%sunknownProcess_%d\\", dumpPath, _getpid());
                }

                if (needMkDir() && doesRegexMatch(dumpPath, IGC_GET_REGKEYSTRING(ShaderDumpRegexFilter)))
                {
                    if (GetFileAttributesA(dumpPath) != FILE_ATTRIBUTE_DIRECTORY)
                    {
                        _mkdir(dumpPath);
                    }
                    g_shaderOutputFolder = dumpPath;
                }
                else
                {
                    // To make the path always invalid.
                    g_shaderOutputFolder = "NUL\\";
                }
            }
            else if (IGC_IS_FLAG_ENABLED(DumpToCustomDir))
            {
                char pathBuf[256];
                iSTD::CreateAppOutputDir(pathBuf, 256, GetBaseIGCOutputFolder(), false, true, !IGC_IS_FLAG_ENABLED(ShaderDumpPidDisable));
                g_shaderOutputFolder = pathBuf;
            }
#elif defined ANDROID

            if (IGC_IS_FLAG_ENABLED(DumpToCurrentDir))
                return "";

            if (!SysUtils::CreateDir(GetBaseIGCOutputFolder(), true, IGC_IS_FLAG_DISABLED(ShaderDumpPidDisable), &g_shaderOutputFolder))

                g_shaderOutputFolder = "";

#elif defined __linux__
            if (!IGC_IS_FLAG_ENABLED(DumpToCurrentDir) && g_shaderOutputFolder == "" && !IGC_IS_FLAG_ENABLED(DumpToCustomDir))
            {
                bool needMkdir = needMkDir() && doesRegexMatch(GetBaseIGCOutputFolder(), IGC_GET_REGKEYSTRING(ShaderDumpRegexFilter));

                char path[MAX_PATH] = { 0 };
                bool pidEnabled = IGC_IS_FLAG_DISABLED(ShaderDumpPidDisable);

                if (needMkdir)
                {
                    iSTD::CreateAppOutputDir(
                        path,
                        MAX_PATH,
                        GetBaseIGCOutputFolder(),
                        false,
                        true,
                        pidEnabled);
                    g_shaderOutputFolder = path;
                }
                else
                {
                    // To make the path always invalid.
                    g_shaderOutputFolder = "/dev/null/";
                }
            }
            else if (IGC_IS_FLAG_ENABLED(DumpToCustomDir))
            {
                char pathBuf[256];
                iSTD::CreateAppOutputDir(pathBuf, 256, GetBaseIGCOutputFolder(), false, false, false);
                g_shaderOutputFolder = pathBuf;
            }

#endif
            return g_shaderOutputFolder.c_str();
        }

        OutputName IGC_DEBUG_API_CALL GetFunctionDebugFile()
        {
            if (IGC_GET_FLAG_VALUE(SelectiveFunctionControl) != 0)
            {
                static std::mutex m;
                std::lock_guard<std::mutex> lck(m);
                // If a custom file is specified by SelectiveFunctionControlFile
                // then use that for SelectiveFunctionControl. Otherwise,
                // fallback to FunctionDebug.txt in IGC output folder.
                static std::string functionDebugFilePath =
                    IGC_GET_REGKEYSTRING(SelectiveFunctionControlFile);
                if (functionDebugFilePath == "")
                {
                    functionDebugFilePath = std::string(GetBaseIGCOutputFolder()) + "FunctionDebug.txt";
                }
                return functionDebugFilePath.c_str();
            }
            return "";
        }

        OutputName IGC_DEBUG_API_CALL GetShaderOutputName()
        {
            return g_shaderOutputName.c_str();
        }

        VersionInfo IGC_DEBUG_API_CALL GetVersionInfo()
        {
            return g_cBuildInfo;
        }
#else  // defined( IGC_DEBUG_VARIABLES )
        // C extern inline needs extern definition
        extern "C" void IGC_DEBUG_API_CALL SetCompilerOptionValue(const char* flagName, int value);
        extern "C" void IGC_DEBUG_API_CALL SetCompilerOptionString(const char* flagName, debugString s);

        void IGC_DEBUG_API_CALL SetCompilerOptionOpaque(OptionFlag flag, void* data);


#endif // defined( IGC_DEBUG_VARIABLES )
    }
}
