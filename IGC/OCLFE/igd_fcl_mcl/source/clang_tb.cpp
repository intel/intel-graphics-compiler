/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../headers/clang_tb.h"
#include "../headers/common_clang.h"
#include "../headers/RegistryAccess.h"

#include "../headers/resource.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "common/LLVMWarningsPop.hpp"
#include "iStdLib/utility.h"

#include "secure_mem.h"
#include "secure_string.h"
#include "AdaptorCommon/customApi.hpp"

#include <mutex>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <iomanip>

#include "3d/common/iStdLib/File.h"
#include "Probe/Assertion.h"

#if defined( _DEBUG ) || defined( _INTERNAL )
#define IGC_DEBUG_VARIABLES
#endif


#if defined(IGC_DEBUG_VARIABLES)

// Code for reading IGC regkeys "ShaderDumpEnable", "DumpToCurrentDir", "ShaderDumpPidDisable".
// Code for shader dump directory name scheme.
// Code is copied from IGC project. This duplication is undesirable in the long term.
// IGC is expected to put this code in single file, without unncessary llvm (and other dependencies).
// Then FCL will just include this single file to avoid code duplication and maintainability issues.

#if defined(_WIN32 )|| defined( _WIN64 )
#include <direct.h>
#include <process.h>
#endif

#if defined __linux__
#include "iStdLib/File.h"
#endif

namespace {
    std::string g_shaderOutputFolder;
}

namespace FCL
{
    namespace Debug
    {
        static std::mutex stream_mutex;

        void DumpLock()
        {
            stream_mutex.lock();
        }

        void DumpUnlock()
        {
            stream_mutex.unlock();
        }
    }

#define IGC_REGISTRY_KEY "SOFTWARE\\INTEL\\IGFX\\IGC"

    typedef char FCLdebugString[256];

    int32_t FCLShDumpEn = 0;
    int32_t FCLDumpToCurrDir = 0;
    int32_t FCLDumpToCustomDir = 0;
    int32_t FCLShDumpPidDis = 0;
    int32_t FCLEnableKernelNamesBasedHash = 0;
    int32_t FCLEnvKeysRead = 0;
    std::string RegKeysFlagsFromOptions = "";


    /*****************************************************************************\
    FCLReadIGCEnv
    \*****************************************************************************/
    static bool FCLReadIGCEnv(
        const char*  pName,
        void*        pValue,
        unsigned int size)
    {
        if (pName != NULL)
        {
            const char nameTag[] = "IGC_";
            std::string pKey = std::string(nameTag) + std::string(pName);

            const char* envVal = getenv(pKey.c_str());

            if (envVal != NULL)
            {
                if (size >= sizeof(unsigned int))
                {
                    // Try integer conversion
                    char* pStopped = nullptr;
                    unsigned int *puVal = (unsigned int *)pValue;
                    *puVal = strtoul(envVal, &pStopped, 0);
                    if (pStopped == envVal + std::string(envVal).length())
                    {
                        return true;
                    }
                }

                // Just return the string
                strncpy_s((char*)pValue, size, envVal, size);

                return true;
            }
        }

        return false;
    }

    /*****************************************************************************\
    FCLReadIGCRegistry
    \*****************************************************************************/
    static bool FCLReadIGCRegistry(
        const char*  pName,
        void*        pValue,
        unsigned int size)
    {
        // All platforms can retrieve settings from environment
        if (FCLReadIGCEnv(pName, pValue, size))
        {
            return true;
        }

#if defined _WIN32
        LONG success = ERROR_SUCCESS;
        HKEY uscKey;

        success = RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            IGC_REGISTRY_KEY,
            0,
            KEY_READ,
            &uscKey);

        if (ERROR_SUCCESS == success)
        {
            DWORD dwSize = size;
            success = RegQueryValueExA(
                uscKey,
                pName,
                NULL,
                NULL,
                (LPBYTE)pValue,
                &dwSize);

            RegCloseKey(uscKey);
        }
        return (ERROR_SUCCESS == success);
#endif // defined _WIN32

        return false;
    }


    bool getFCLIGCBinaryKey(const char *keyName)
    {
        FCLdebugString value = { 0 };

        bool isSet = FCLReadIGCRegistry(
            keyName,
            &value,
            sizeof(value));
        isSet = isSet;

        return(value[0] != 0);
    }


    void FCLReadKeysFromEnv()
    {
        if (!FCLEnvKeysRead)
        {
            FCLShDumpEn            = getFCLIGCBinaryKey("ShaderDumpEnable") || (RegKeysFlagsFromOptions.find("ShaderDumpEnable=1") != std::string::npos);
            FCLDumpToCurrDir    = getFCLIGCBinaryKey("DumpToCurrentDir") || (RegKeysFlagsFromOptions.find("DumpToCurrentDir=1") != std::string::npos);
            FCLDumpToCustomDir  = getFCLIGCBinaryKey("DumpToCustomDir") || (RegKeysFlagsFromOptions.find("DumpToCustomDir=") != std::string::npos);
            FCLShDumpPidDis        = getFCLIGCBinaryKey("ShaderDumpPidDisable") || (RegKeysFlagsFromOptions.find("ShaderDumpPidDisable=1") != std::string::npos);
            FCLEnableKernelNamesBasedHash = getFCLIGCBinaryKey("EnableKernelNamesBasedHash") || (RegKeysFlagsFromOptions.find("EnableKernelNamesBasedHash=1") != std::string::npos);
            FCLEnvKeysRead = 1;
        }
    }

    bool GetFCLShaderDumpEnable()
    {
        FCLReadKeysFromEnv();
        return FCLShDumpEn;
    }

    bool GetFCLShaderDumpPidDisable()
    {
        FCLReadKeysFromEnv();
        return FCLShDumpPidDis;
    }

    bool GetFCLDumpToCurrentDir()
    {
        FCLReadKeysFromEnv();
        return FCLDumpToCurrDir;
    }

    bool GetFCLDumpToCustomDir()
    {
        FCLReadKeysFromEnv();
        return FCLDumpToCustomDir;
    }

    bool GetFCLEnableKernelNamesBasedHash()
    {
        FCLReadKeysFromEnv();
        return FCLEnableKernelNamesBasedHash;
    }

    OutputFolderName  GetBaseIGCOutputFolder()
    {
#if defined(IGC_DEBUG_VARIABLES)
        static std::mutex m;
        std::lock_guard<std::mutex> lck(m);
        static std::string IGCBaseFolder;
        if (IGCBaseFolder != "")
        {
            return IGCBaseFolder.c_str();
        }
#   if defined(_WIN64) || defined(_WIN32)
        if (!FCL_IGC_IS_FLAG_ENABLED(DumpToCurrentDir) && !FCL_IGC_IS_FLAG_ENABLED(DumpToCustomDir))
        {
            bool needMkdir = 1;
            char dumpPath[256];

            sprintf_s(dumpPath, "c:\\Intel\\IGC\\");

            if (GetFileAttributesA(dumpPath) != FILE_ATTRIBUTE_DIRECTORY && needMkdir)
            {
                _mkdir(dumpPath);
            }

            // Make sure we can write in the dump folder as the app may be sandboxed
            if (needMkdir)
            {
                int tmp_id = _getpid();
                std::string testFilename = std::string(dumpPath) + "testfile" + std::to_string(tmp_id);
                HANDLE testFile =
                    CreateFileA(testFilename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
                if (testFile == INVALID_HANDLE_VALUE)
                {
                    char temppath[256];
                    if (GetTempPathA(sizeof(temppath), temppath) != 0)
                    {
                        sprintf_s(dumpPath, "%sIGC\\", temppath);
                    }
                }
                else
                {
                    CloseHandle(testFile);
                }
            }

            if (GetFileAttributesA(dumpPath) != FILE_ATTRIBUTE_DIRECTORY && needMkdir)
            {
                _mkdir(dumpPath);
            }

            IGCBaseFolder = dumpPath;
        }
        else if (FCL_IGC_IS_FLAG_ENABLED(DumpToCustomDir))
        {
            std::string dumpPath = "c:\\Intel\\IGC\\";        // default if something goes wrong
            char custom_dir[256];
            std::string DumpToCustomDirFlagNameWithEqual = "DumpToCustomDir=";
            std::size_t found = RegKeysFlagsFromOptions.find(DumpToCustomDirFlagNameWithEqual);
            FCLReadIGCRegistry("DumpToCustomDir", custom_dir, sizeof(custom_dir));
            if (std::string(custom_dir).length() > 0 && (found == std::string::npos))
            {
                dumpPath = custom_dir;
            }
            else
            {
                std::size_t foundComma = RegKeysFlagsFromOptions.find(',', found);
                if (foundComma != std::string::npos)
                {
                    std::string token = RegKeysFlagsFromOptions.substr(found + DumpToCustomDirFlagNameWithEqual.size(), foundComma - (found + DumpToCustomDirFlagNameWithEqual.size()));
                    if (token.size() > 0)
                    {
                        dumpPath = token;
                    }
                }
            }

            char pathBuf[256];
            iSTD::CreateAppOutputDir(pathBuf, 256, dumpPath.c_str(), false, false, false);

            IGCBaseFolder = pathBuf;
        }
#elif defined __linux__
        if (!FCL_IGC_IS_FLAG_ENABLED(DumpToCustomDir))
        {
            IGCBaseFolder = "/tmp/IntelIGC/";
        }
        else
        {
            std::string dumpPath = "/tmp/IntelIGC/";        // default if something goes wrong
            const size_t maxLen = 255;
            char custom_dir[ maxLen + 1] = { 0 };
            std::string DumpToCustomDirFlagNameWithEqual = "DumpToCustomDir=";
            std::size_t found = RegKeysFlagsFromOptions.find(DumpToCustomDirFlagNameWithEqual);
            FCLReadIGCRegistry("DumpToCustomDir", custom_dir, maxLen);
            if (std::string(custom_dir).length() > 0 && (found == std::string::npos))
            {
                IGC_ASSERT_MESSAGE(std::string(custom_dir).length() < maxLen, "custom_dir path too long");
                dumpPath = custom_dir;
                dumpPath += "/";
            }
            else
            {
                std::size_t foundComma = RegKeysFlagsFromOptions.find(',', found);
                if (foundComma != std::string::npos)
                {
                    std::string token = RegKeysFlagsFromOptions.substr(found + DumpToCustomDirFlagNameWithEqual.size(), foundComma - (found + DumpToCustomDirFlagNameWithEqual.size()));
                    if (token.size() > 0)
                    {
                        dumpPath = token;
                    }
                }
            }

            char pathBuf[256];
            iSTD::CreateAppOutputDir(pathBuf, 256, dumpPath.c_str(), false, false, false);

            IGCBaseFolder = pathBuf;
        }
#endif
        return IGCBaseFolder.c_str();
#else
        return "";
#endif
    }

    OutputFolderName  GetShaderOutputFolder()
    {
#if defined(IGC_DEBUG_VARIABLES)
        static std::mutex m;
        std::lock_guard<std::mutex> lck(m);
        if (g_shaderOutputFolder != "")
        {
            return g_shaderOutputFolder.c_str();
        }
#   if defined(_WIN64) || defined(_WIN32)
        if (!FCL_IGC_IS_FLAG_ENABLED(DumpToCurrentDir) && !FCL_IGC_IS_FLAG_ENABLED(DumpToCustomDir))
        {
            char dumpPath[256];
            sprintf_s(dumpPath, "%s", GetBaseIGCOutputFolder());
            char appPath[MAX_PATH] = { 0 };
            // check a process id and make an adequate directory for it:

            if (::GetModuleFileNameA(NULL, appPath, sizeof(appPath) - 1))
            {
                std::string appPathStr = std::string(appPath);
                int pos = appPathStr.find_last_of("\\") + 1;

                if (FCL_IGC_IS_FLAG_ENABLED(ShaderDumpPidDisable))
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

            if (GetFileAttributesA(dumpPath) != FILE_ATTRIBUTE_DIRECTORY)
            {
                _mkdir(dumpPath);
            }

            g_shaderOutputFolder = dumpPath;
        }
        else if (FCL_IGC_IS_FLAG_ENABLED(DumpToCustomDir))
        {
            char pathBuf[256];
            iSTD::CreateAppOutputDir(pathBuf, 256, GetBaseIGCOutputFolder(), false, true, !FCL_IGC_IS_FLAG_ENABLED(ShaderDumpPidDisable));
            g_shaderOutputFolder = pathBuf;
        }
#elif defined __linux__
        if (!FCL_IGC_IS_FLAG_ENABLED(DumpToCurrentDir) && g_shaderOutputFolder == "" && !FCL_IGC_IS_FLAG_ENABLED(DumpToCustomDir))
        {
            bool needMkdir = true;

            char path[MAX_PATH] = { 0 };
            bool pidEnabled = !FCL_IGC_IS_FLAG_ENABLED(ShaderDumpPidDisable);

            if (needMkdir)
            {
                iSTD::CreateAppOutputDir(
                    path,
                    MAX_PATH,
                    GetBaseIGCOutputFolder(),
                    false,
                    true,
                    pidEnabled);
            }

            g_shaderOutputFolder = path;
        }
        else if (FCL_IGC_IS_FLAG_ENABLED(DumpToCustomDir))
        {
            char pathBuf[256];
            iSTD::CreateAppOutputDir(pathBuf, 256, GetBaseIGCOutputFolder(), false, false, false);
            g_shaderOutputFolder = pathBuf;
        }

#endif
        return g_shaderOutputFolder.c_str();
#else
        return "";
#endif
    }

} // namespace FCL
 /// pk this ends here
#endif

#ifndef WIN32
#include <dlfcn.h>
#include <stdexcept>
#endif

#if defined(_WIN32)
#include <Windows.h>
#include "DriverStore.h"
#endif

using namespace llvm;
using namespace std;

// ElfReader related typedefs
using namespace CLElfLib;


void ElfReaderDP(CElfReader* pElfReader)
{
    if (pElfReader)
        CElfReader::Delete(pElfReader);
}
typedef unique_ptr<CElfReader, decltype(&ElfReaderDP)> CElfReaderPtr;

// ClangFE related typedefs
using namespace Intel::OpenCL::ClangFE;
void ReleaseDP(IOCLFEBinaryResult* pT)
{
    if (pT)
        pT->Release();
}

typedef unique_ptr<IOCLFEBinaryResult, decltype(&ReleaseDP) > IOCLFEBinaryResultPtr;

namespace TC
{
    constexpr bool is64bit = sizeof(void*) == sizeof(uint64_t);
    //Misc utility functions used only in the current module
    namespace Utils
    {
        // Replace \0 in input string with \n. This works around an issue in
        // Clang where the error message is not generated for inputs that contain
        // a non-ending \0
        char* NormalizeString(char* input, uint32_t size)
        {
            for (uint32_t i = 0; i < size; i++)
            {
                if (input[i] == '\0')
                {
                    input[i] = '\n';
                }
            }
            input[size - 1] = '\0';
            return input;
        }

        //Translates the ClangFE results to STB Output results
        void FillOutputArgs(IOCLFEBinaryResult* pFEBinaryResult, STB_TranslateOutputArgs* pOutputArgs, std::string& exceptString)
        {
            // fill the result structure
            pOutputArgs->ErrorStringSize = (uint32_t)std::string(pFEBinaryResult->GetErrorLog()).length();
            if (pOutputArgs->ErrorStringSize > 0)
            {
                TC::CClangTranslationBlock::SetErrorString(pFEBinaryResult->GetErrorLog(), pOutputArgs);
            }
            else
            {
                pOutputArgs->pErrorString = NULL;
            }
            pOutputArgs->OutputSize = (uint32_t)pFEBinaryResult->GetIRSize();

            // we have to copy the result due to unfortunate design of STB_TranslateOutputArg interface
            // the better design would be for TranslateXXX calls to be responsible to allocate the outputArgs
            // interface entirely, and the client to be responsible to call outputArgs->release() to free it.
            // This way the implementation of TranslateXXX could be free to return inherited from outputArgs
            // class which could glue the outputArgs with other internal interfaces (like the one returned from
            // ::Compile method for example) without any buffer copy
            if (pOutputArgs->OutputSize > 0)
            {
                pOutputArgs->pOutput = (char*)malloc(pFEBinaryResult->GetIRSize());

                if (!pOutputArgs->pOutput)
                {
                    //throw std::bad_alloc();
                    exceptString = "bad_alloc";
                    return;
                }

                memcpy_s(pOutputArgs->pOutput,
                    pFEBinaryResult->GetIRSize(),
                    pFEBinaryResult->GetIR(),
                    pFEBinaryResult->GetIRSize());
            }
        }
    }//namespace Utils


    struct OCLVersionNumberMapping
    {
        const char*        version;
        unsigned int number;
    };

    // Input parameters to the 3 function
    struct TranslateClangArgs
    {
        TranslateClangArgs() :
            pszProgramSource(NULL),
            pPCHBuffer(NULL),
            uiPCHBufferSize(0),
            b32bit(!is64bit)
        {
        }

        // A pointer to main program's source (assumed one nullterminated string)
        const char*     pszProgramSource;
        // array of additional input headers to be passed in memory
        std::vector<const char*> inputHeaders;
        // array of input headers names corresponding to pInputHeaders
        std::vector<const char*> inputHeadersNames;
        // optional pointer to the pch buffer
        const char*     pPCHBuffer;
        // size of the pch buffer
        size_t          uiPCHBufferSize;
        // OpenCL application supplied options
        std::string     options;
        // optional extra options string usually supplied by runtime
        std::string     optionsEx;
        // requested OCL version
        std::string     oclVersion;
        // build for 32 bit
        bool            b32bit;
    };

    // Initialize static mutex object to be shared with all threads
    //llvm::sys::Mutex CClangTranslationBlock::m_Mutex(/* recursive = */ true);

    /*****************************************************************************\

    Function:
    CClangTranslationBlock::Create

    Description:

    Input:

    Output:

    \*****************************************************************************/
    bool CClangTranslationBlock::Create(
        const STB_CreateArgs* pCreateArgs,
        STB_TranslateOutputArgs* pOutputArgs,
        CClangTranslationBlock* &pTranslationBlock)
    {
        bool    success = true;

        pTranslationBlock = new CClangTranslationBlock();

        if (pTranslationBlock)
        {
            success = pTranslationBlock->Initialize(pCreateArgs);

#ifdef _WIN32
            if (true == success)
            {
                // Both Win32 and Win64
                // load dependency only on RS
                // Load the Common Clang library
                CCModuleStruct &CCModule = pTranslationBlock->m_CCModule;
                if (GetWinVer() >= OS_WIN_RS)
                {
                    CCModule.pModule = LoadDependency(CCModule.pModuleName);
                }
                else
                {
                    CCModule.pModule = LoadLibraryA(CCModule.pModuleName);
                }
                if (NULL != CCModule.pModule)
                {
                    CCModule.pCompile = (CCModuleStruct::PFcnCCCompile)GetProcAddress((HMODULE)CCModule.pModule, "Compile");
                    success = CCModule.pCompile != NULL;
                }
                else
                {
                    SetErrorString("Error: Opencl-clang library not found.", pOutputArgs);
                    success = false;
                }
            }
#endif

            if (!success)
            {
                CClangTranslationBlock::Delete(pTranslationBlock);
            }
        }
        else
        {
            success = false;
        }

        return success;
    }

    /*****************************************************************************\

    Function:
    CClangTranslationBlock::Delete

    Description:

    Input:

    Output:

    \*****************************************************************************/
    void CClangTranslationBlock::Delete(
        CClangTranslationBlock* &pTranslationBlock)
    {
#ifdef _WIN32
        // Unload the Common Clang library
        if (pTranslationBlock->m_CCModule.pModule) {
            // Both Win32 and Win64
            FreeLibrary((HMODULE)pTranslationBlock->m_CCModule.pModule);
        }
#endif

        delete pTranslationBlock;
        pTranslationBlock = NULL;
    }

    /*****************************************************************************\

    Function:
    CClangTranslationBlock::SetErrorString

    Description:
    Given an error string, mallocs memory for the string and sets the
    appropriate STB_TranslateOutputArgs fields.

    Input:

    Output:

    \*****************************************************************************/
    void CClangTranslationBlock::SetErrorString(const char *pErrorString, STB_TranslateOutputArgs* pOutputArgs)
    {
        IGC_ASSERT(pErrorString != NULL);
        IGC_ASSERT(pOutputArgs != NULL);
        size_t strSize = std::string(pErrorString).length() + 1;
        pOutputArgs->ErrorStringSize = (uint32_t)strSize;
        pOutputArgs->pErrorString = (char*)malloc(strSize);
        memcpy_s(pOutputArgs->pErrorString, strSize - 1, pErrorString, strSize - 1);
        pOutputArgs->pErrorString[strSize - 1] = '\0';
    }

    /*****************************************************************************\

    Function:
    CClangTranslationBlock::GetOclApiVersion

    Description:
    Parses the given internal options and return the OCL Version to be used
    for Clang compilation. If OCL version was not specified in internal options
    returns the default OCL version for the device

    Input:

    Output:

    \*****************************************************************************/
    std::string CClangTranslationBlock::GetOclApiVersion(const char* pInternalOptions) const
    {
        static const char* OCL_VERSION_OPT = "-ocl-version=";
        static size_t OCL_VERSION_OPT_SIZE = std::string(OCL_VERSION_OPT).length();

        if (pInternalOptions)
        {
            const char* pszOpt = strstr(pInternalOptions, OCL_VERSION_OPT);
            if (NULL != pszOpt)
            {
                // we are in control of internal option - assertion test the validity
                IGC_ASSERT(std::string(pszOpt + OCL_VERSION_OPT_SIZE).length() >= 3);
                return std::string(pszOpt + OCL_VERSION_OPT_SIZE, 3);
            }
        }

        return m_OCL_Ver;
    }

    /*****************************************************************************\

    Function:
    EnforceOCLCVersion

    Description:
    In case the '-force-cl-std' options was specified, check that the user
    requested OCL C version isn't higher than the supported OCL version.
    exception is thrown otherwise

    Input:

    Output:

    \*****************************************************************************/
    unsigned int GetOclCVersionFromOptions(const char* pOptions, const char* pInternalOptions,
        const std::string& oclVersion /*OCL runtime API version*/,
        std::string& exceptString)
    {
        exceptString.clear();

        if (pOptions == nullptr) {
            return 0; // no options (i.e. no options from client application)
        }

        std::string optName = "-cl-std="; // opt that we are looking for
        unsigned int device_version = std::stoi(oclVersion);

        const char* optSubstring = strstr(pOptions, optName.c_str());
        if (optSubstring == nullptr) {
            return 0; // -cl-std not specified
        }

        bool validate = true;
        if ((pInternalOptions != nullptr) && (strstr(pInternalOptions, "-force-cl-std") != nullptr)) {
            // we're forcing the -cl-std version internally, so no need for validating it
            validate = false;
        }

        const char * optValue = optSubstring + optName.size();
        const char * end = optValue + std::string(optValue).length();

        // parse
        const std::string invalidFormatMessage = "Invalid format of -cl-std option, expected -cl-std=CLMAJOR.MINOR";
        auto isNumeric = [](char v) { return (v >= '0') && (v <= '9'); };
        if (false == ((end - optValue >= 5) && (optValue[0] == 'C') && (optValue[1] == 'L') && isNumeric(optValue[2])
            && (optValue[3] == '.') && isNumeric(optValue[4])
            )
            ) {
            exceptString = invalidFormatMessage;
            return 0;
        }

        unsigned int retVersion = 0;
        // subverions
        if ((end - optValue >= 7) && (optValue[5] != ' ')) {
            if ((optValue[5] == '.') || isNumeric(optValue[6])) {
                retVersion += optValue[6] - '0';
            }
            else if (isNumeric(optValue[5])) {
                retVersion += optValue[5] - '0';
            }
            else {
                exceptString = invalidFormatMessage;
                return 0;
            }
        }

        retVersion += 100 * (optValue[2] - '0') + 10 * (optValue[4] - '0');

        if (validate == false) {
            return retVersion;
        }

        if (device_version < retVersion) {
            exceptString = "-cl-std OpenCLC version greater than OpenCL (API) version";
            return 0;
        }

        return retVersion;
    }

    /*****************************************************************************\

    Function:
    IsBuildingFor32bit

    Description:
    Return true if clang should generate 32bit code

    Input:

    Output:

    \*****************************************************************************/
    bool IsBuildingFor32bit(const char* pInternalOptions)
    {
        // Detect pointer size from internal option string. Default to using the
        // architecture type if the string is unavailable.
        if (pInternalOptions != NULL)
        {
            if (strstr(pInternalOptions, "-m32") != NULL)
            {
                return true;
            }

            if (strstr(pInternalOptions, "-m64") != NULL)
            {
                return false;
            }
        }

        return !is64bit;
    }

  /*****************************************************************************\

  Function:
  AreVMETypesDefined

  Description:
  Returns true if CommonClang used on current OS has VME types defined.

  \*****************************************************************************/
  bool AreVMETypesDefined()
  {
#ifdef VME_TYPES_DEFINED
#if VME_TYPES_DEFINED
    return true;
#else
    return false;
#endif
#endif
    return true;
  }

    /*****************************************************************************\

    Function:
    CClangTranslationBlock::GetTranslateClangArgs

    Description:
    Prepares the arguments for the TranslateClang method for the given text input

    Input:

    Output:

    \*****************************************************************************/
    void CClangTranslationBlock::GetTranslateClangArgs(char* pInput,
        uint32_t    uiInputSize,
        const char* pOptions,
        const char* pInternalOptions,
        TranslateClangArgs* pClangArgs,
        std::string& exceptString)
    {
        pClangArgs->pszProgramSource = Utils::NormalizeString(pInput, uiInputSize);
        pClangArgs->pPCHBuffer = NULL;
        pClangArgs->uiPCHBufferSize = 0;
        pClangArgs->oclVersion = GetOclApiVersion(pInternalOptions);
        pClangArgs->b32bit = IsBuildingFor32bit(pInternalOptions);

        if (pOptions)
        {
            pClangArgs->options.assign(pOptions);
        }
#if defined(IGC_DEBUG_VARIABLES)
        char debugOptions[1024];
        if (FCL::FCLReadIGCRegistry("ExtraOCLOptions", debugOptions, sizeof(debugOptions)))
        {
            if (!pClangArgs->options.empty())
                pClangArgs->options += ' ';
            pClangArgs->options += debugOptions;
        }
#endif

        GetOclCVersionFromOptions(pOptions, pInternalOptions, pClangArgs->oclVersion, exceptString);
        EnsureProperPCH(pClangArgs, pInternalOptions, exceptString);
    }

    /*****************************************************************************\

    Function:
    CClangTranslationBlock::GetTranslateClangArgs

    Description:
    Parses the given ELF binary to prepare the arguments for the TranslateClang

    Input:

    Output:

    \*****************************************************************************/
    void CClangTranslationBlock::GetTranslateClangArgs(CElfReader* pElfReader,
        const char* pOptions,
        const char* pInternalOptions,
        TranslateClangArgs* pClangArgs,
        std::string& exceptString)
    {
        IGC_ASSERT_MESSAGE(pElfReader, "pElfReader is invalid");

        const SElfHeader* pHeader = pElfReader->GetElfHeader();
        IGC_ASSERT_MESSAGE(pHeader->Type == EH_TYPE_OPENCL_SOURCE, "OPENCL_SOURCE elf type is expected");

        // First section should be an OpenCL source code
        const SElfSectionHeader* pSectionHeader = pElfReader->GetSectionHeader(1);
        IGC_ASSERT_MESSAGE(NULL != pSectionHeader, "pSectionHeader cannot be NULL");

        if (pSectionHeader->Type == SH_TYPE_OPENCL_SOURCE)
        {
            char *pData = NULL;
            size_t uiDataSize = 0;
            pElfReader->GetSectionData(1, pData, uiDataSize);

            if (pData != NULL)
            {
                IGC_ASSERT_MESSAGE(pData[uiDataSize - 1] == '\0', "Program source is not null terminated");
                pClangArgs->pszProgramSource = pData;
            }
        }

        // Other sections could be runtime supplied header files
        for (unsigned i = 2; i < pHeader->NumSectionHeaderEntries; ++i)
        {
            const SElfSectionHeader* pSectionHeader = pElfReader->GetSectionHeader(i);

            if ((pSectionHeader != NULL) && (pSectionHeader->Type == SH_TYPE_OPENCL_HEADER))
            {
                char* pData = NULL;
                size_t uiDataSize = 0;
                pElfReader->GetSectionData(i, pData, uiDataSize);

                if (pData != NULL)
                {
                    IGC_ASSERT_MESSAGE(pData[uiDataSize - 1] == '\0', "Header source is not null terminated");
                    pClangArgs->inputHeaders.push_back(pData);
                    pClangArgs->inputHeadersNames.push_back(pElfReader->GetSectionName(i));
                }
            }
        }

        if (pOptions)
        {
            pClangArgs->options.assign(pOptions);
        }

        pClangArgs->oclVersion = GetOclApiVersion(pInternalOptions);
        pClangArgs->b32bit = IsBuildingFor32bit(pInternalOptions);

        EnsureProperPCH(pClangArgs, pInternalOptions, exceptString);
    }

    std::string FormatExtensionsString(const std::vector<std::string> &extensions)
    {
        std::stringstream output;

        if (!extensions.empty())
        {
            output << "-cl-ext=-all";
            for (const std::string &extension : extensions)
                output << ",+" << extension;
        }

        output.flush();
        return output.str();
    }

    // Extracts a substring starting with "prefix" and ending with space.
    std::string GetSubstring(const std::string& str, const std::string& prefix) {
        size_t start_pos = 0, end_pos = 0;
        std::string returnString = "";

        while ((start_pos = str.find(prefix, end_pos)) != std::string::npos) {
          end_pos = str.find(' ', start_pos);
          returnString += str.substr(start_pos, end_pos - start_pos) + " ";
        }

        return returnString;
    }

    std::string GetCDefinesFromInternalOptions(const char *pInternalOptions) {
        if (pInternalOptions == nullptr) {
            return std::string{};
        }

        std::string internalDefines = "";

        const char * beg = strstr(pInternalOptions, "-D");
        const char * end = nullptr;
        while (beg != nullptr) {
            if ((beg == pInternalOptions) || (beg[-1] == ' ')) {
                end = strstr(beg, " ");
                if (end == nullptr) {
                    if (beg[2] != '\0') {
                        // more than just -D
                        internalDefines += std::string(beg);
                    }
                    break;
                }
                else {
                    if (end - beg > 2) {
                        internalDefines += std::string(beg, end);
                    }
                }
                internalDefines += " ";
            }
            else {
                end = beg + 2; // -D
            }

            beg = strstr(end, "-D");
        }

        return internalDefines;
    }

    // The expected extensions input string is in a form:
    // -cl-ext=-all,+supported_ext_name,+second_supported_ext_name
    // -cl-feature=+__opencl_c_3d_image_writes,+__opencl_c_atomic_order_acq_rel
    std::string GetCDefinesForEnableList(llvm::StringRef enableListStr, unsigned int oclStd, const StringRef prefix) {

      std::string definesStr;

      // check for the last occurence of prefix, as it invalidates previous occurances.
      size_t pos = enableListStr.rfind(prefix);
      if (pos == llvm::StringRef::npos) {
        // If this string does not exist the input string does not contain valid extension list
        // or it has all extensions disabled (-all without colon afterwards).
        return definesStr;
      }

      enableListStr = enableListStr.substr(pos+prefix.size());

      // string with defines should be similar in size, ",+" will change to "-D".
      definesStr.reserve(enableListStr.size());

      llvm::SmallVector<StringRef, 0> v;
      enableListStr.split(v, ',');

      for (auto ext : v) {
        if (ext.consume_front("+")) {
          if (ext.equals("cl_intel_device_side_avc_motion_estimation")) {
            // If the user provided -cl-std option we need to add the define only if it's 1.2 and above.
            // This is because clang will not allow declarations of extension's functions which use avc types otherwise.
            if (!(oclStd >= 120 || oclStd == 0)) continue;
          }
          definesStr.append(" -D").append(ext.str());
        }
      }

      return definesStr;
    }

    /*****************************************************************************\

    Function:
    CClangTranslationBlock::EnsureProperPCH

    Description:
    Ensures that the given TranslateClang arguments has the proper CTH and PCH
    headers specified

    Input:

    Output:

    \*****************************************************************************/
    void CClangTranslationBlock::EnsureProperPCH(TranslateClangArgs* pArgs, const char *pInternalOptions, std::string& exceptString)
    {
        unsigned long CTHeaderSize = 0;
        m_cthBuffer = llvm::LoadCharBufferFromResource(IDR_CTH_H, "H", CTHeaderSize);
        IGC_ASSERT_MESSAGE(m_cthBuffer, "Error loading Opencl_cth.h");

        if (m_cthBuffer)
        {
            // Process the CT Header
            IGC_ASSERT_MESSAGE(CTHeaderSize > 0, "Resource for the CT Header is empty");
            IGC_ASSERT_MESSAGE(m_cthBuffer[CTHeaderSize - 1] == '\0', "Resource for the CT Header is not null terminated");

            pArgs->inputHeaders.push_back(m_cthBuffer);
            pArgs->inputHeadersNames.push_back("CTHeader.h");
            pArgs->optionsEx.append(" -include CTHeader.h");
        }
    }

    /**********************************************************************\

    Function:
    GetParam

    Description:
    takes first parameter from Head
    and assigns the rest of parameters to Tail
    Head  : input string with list / output with head
    Tail  : output string

    returns Head or NULL if Head is empty

    \**********************************************************************/
    char *GetParam(char *Head, char *Tail) {
        static char Delim = ' ';
        static char Slash = '\\';
        char QuoteDouble = 0;
        char QuoteSingle = 0;
        char PrevChar = 0;
        char *Pos = NULL;
        int Length = 0;

        if (Head == NULL)
        {
            *Tail = 0;
            goto ERROR_HANDLER;
        }

        QuoteDouble = Delim;
        QuoteSingle = Delim;
        Pos = Head;
        Length = (int)std::string(Head).length();

        while (*Pos == Delim)
        {
            Pos++;
            Length--;
        }
        if (*Pos == 0)
        {
            *Tail = 0;
            Head = NULL;
            goto ERROR_HANDLER;
        }

        switch (*Pos)
        {
        case '\"':
            QuoteDouble = *Pos;
            break;
        case '\'':
            QuoteSingle = *Pos;
            break;
        default: break;
        }

        do
        {
            PrevChar = *Pos;
            Pos++;
            Length--;
            switch (*Pos)
            {
            case '\"':
                if (PrevChar != Slash)
                {
                    if (QuoteDouble == Delim)
                    {
                        QuoteDouble = *Pos; // Open quote string
                    }
                    else
                    {
                        QuoteDouble = Delim; // Close quote string
                    }
                }
                break;
            case '\'':
                if (PrevChar != Slash)
                {
                    if (QuoteSingle == Delim)
                    {
                        QuoteSingle = *Pos; // Open quote string
                    }
                    else
                    {
                        QuoteSingle = Delim; // Close quote string
                    }
                }
                break;
            default: break;
            }
        } while ((*Pos != 0)
            && ((*Pos != Delim)
                || ((*Pos == Delim) && ((QuoteDouble != Delim) || (QuoteSingle != Delim)))));
        if (*Pos == Delim)
        {
            *Pos = 0; // finish Head
            Pos++; // skip zero
            while (*Pos == Delim)
            {
                Pos++; // skip multiple delimiters
                Length--;
            }
            if (*Pos != 0)
            {
                strcpy_s(Tail, Length, Pos); // Assign the rest of params to Tail
            }
            else
            {
                *Tail = 0;
            }
        }
        else
        {
            *Tail = 0;
        }

        while (*Head == Delim)
        {
            Head++;
        }

    ERROR_HANDLER:
        return Head;
    }

    /**********************************************************************\

    Class Function:
    BuildOptionsAreValid

    Description:
    Walks through options and makes sure they're ok

    \**********************************************************************/

    int BuildOptionsAreValid(const std::string& options, std::string& exceptString)
    {
        int  retVal = 0;

        char*   nextTok = NULL;
        char*   pParam = NULL;
        char*   pBuffer = NULL;
        bool    ignoreNextToken = false;
        bool    checkBinaryType = false;
        bool    isCommonOption = false;

        if (!options.empty())
        {
            size_t  optionsSize = options.size() + 1;
            //alocate memory for pBuffer and nextTok
            pBuffer = new char[optionsSize];
            nextTok = new char[optionsSize];

            strncpy_s(pBuffer, optionsSize, options.c_str(), optionsSize);
            pParam = GetParam(pBuffer, nextTok);

            if (pParam)
            {
                do
                {
                    if (checkBinaryType)
                    {
                        // If "spir" does not follow the -x option, we must fail
                        if ((strcmp(pParam, "spir") && strcmp(pParam, "spir64")))
                        {
                            // Invalid option - break out of the loop and return
                            // CL_INVALID_BUILD_OPTIONS
                            retVal = -43;
                            std::string invalidOption(pParam);

                            exceptString += "\nUnrecognized build options: " + invalidOption;
                            break;
                        }

                        // reset
                        checkBinaryType = false;
                    }
                    else if (ignoreNextToken == false)
                    {
                        // Check for common Intel OpenCL CPU/GPU options
                        isCommonOption =
                            (strcmp(pParam, "-cl-single-precision-constant") == 0) ||
                            (strcmp(pParam, "-cl-denorms-are-zero") == 0) ||
                            (strcmp(pParam, "-cl-fp32-correctly-rounded-divide-sqrt") == 0) ||
                            (strcmp(pParam, "-cl-opt-disable") == 0) ||
                            (strcmp(pParam, "-ze-opt-disable") == 0) ||
                            (strcmp(pParam, "-cl-strict-aliasing") == 0) ||
                            (strcmp(pParam, "-cl-mad-enable") == 0) ||
                            (strcmp(pParam, "-cl-no-signed-zeros") == 0) ||
                            (strcmp(pParam, "-cl-unsafe-math-optimizations") == 0) ||
                            (strcmp(pParam, "-cl-finite-math-only") == 0) ||
                            (strcmp(pParam, "-cl-fast-relaxed-math") == 0) ||
                            (strcmp(pParam, "-cl-match-sincospi") == 0) ||
                            (strcmp(pParam, "-w") == 0) ||
                            (strcmp(pParam, "-Werror") == 0) ||
                            (strcmp(pParam, "-cl-std=CL1.1") == 0) ||
                            (strcmp(pParam, "-cl-std=CL1.2") == 0) ||
                            (strcmp(pParam, "-cl-std=CL2.0") == 0) ||
                            (strcmp(pParam, "-cl-std=CL2.1") == 0) ||
                            (strcmp(pParam, "-cl-std=CL3.0") == 0) ||
                            (strcmp(pParam, "-cl-uniform-work-group-size") == 0) || //it's work only for OCL version greater than 1.2
                            (strcmp(pParam, "-cl-kernel-arg-info") == 0) ||
                            (strncmp(pParam, "-x", 2) == 0) ||
                            (strncmp(pParam, "-D", 2) == 0) ||
                            (strncmp(pParam, "-I", 2) == 0) ||
                            (strncmp(pParam, "-spir-std=", 10) == 0) ||
                            (strcmp(pParam, "-gline-tables-only") == 0) ||
                            (strcmp(pParam, "-triple") == 0) || //used in NEO
                            (strcmp(pParam, "-dwarf-column-info") == 0) ||
                            (strcmp(pParam, "-cl-intel-no-prera-scheduling") == 0) || //temporary options
                            (strcmp(pParam, "-igc_opts") == 0) || //temporary options
                            (strcmp(pParam, "-cl-intel-gtpin-rera") == 0) || //temporary options
                            (strcmp(pParam, "-cl-intel-256-GRF-per-thread") == 0) || //temporary options
                            (strcmp(pParam, "-ze-opt-256-GRF-per-thread") == 0) || //temporary options
                            (strcmp(pParam, "-ze-exp-register-file-size") == 0) || //temporary options
                            (strcmp(pParam, "-ze-opt-large-register-file") == 0) || //temporary options
                            (strcmp(pParam, "-cl-intel-large-grf-kernel") == 0) ||
                            (strcmp(pParam, "-ze-opt-large-grf-kernel") == 0) ||
                            (strcmp(pParam, "-cl-intel-regular-grf-kernel") == 0) ||
                            (strcmp(pParam, "-ze-opt-regular-grf-kernel") == 0) ||
                            (strcmp(pParam, "-ze-opt-disable-recompilation") == 0) || //temporary options
                            (strcmp(pParam, "-cl-intel-num-thread-per-eu") == 0) || //temporary options
                            (strncmp(pParam, "-cl-intel-reqd-eu-thread-count", 30) == 0) ||
                            (strcmp(pParam, "-cl-replace-global-offsets-by-zero") == 0) || //temporary options
                            (strcmp(pParam, "-cl-kernel-debug-enable") == 0) || //temporary options
                            (strcmp(pParam, "-cl-include-sip-csr") == 0) || //temporary options
                            (strcmp(pParam, "-cl-include-sip-kernel-debug") == 0) || //temporary options
                            (strcmp(pParam, "-cl-include-sip-kernel-local-debug") == 0) || //temporary options
                            (strcmp(pParam, "-cl-intel-use-32bit-ptr-arith") == 0) || //temporary options
                            (strcmp(pParam, "-cl-intel-force-disable-4GB-buffer") == 0) || //temporary options
                            (strcmp(pParam, "-cl-intel-greater-than-4GB-buffer-required") == 0) || //temporary options
                            (strcmp(pParam, "-ze-opt-greater-than-4GB-buffer-required") == 0) || //temporary options
                            (strcmp(pParam, "-cl-intel-has-buffer-offset-arg") == 0) || //temporary options
                            (strcmp(pParam, "-ze-opt-has-buffer-offset-arg") == 0) || //temporary options
                            (strcmp(pParam, "-cl-intel-buffer-offset-arg-required") == 0) || //temporary options
                            (strcmp(pParam, "-ze-opt-buffer-offset-arg-required") == 0) || //temporary options
                            (strcmp(pParam, "-cl-force-global-mem-allocation") == 0) || // temp
                            (strcmp(pParam, "-ze-force-global-mem-allocation") == 0) || // temp
                            (strcmp(pParam, "-cl-no-local-to-generic") == 0) || // temp
                            (strcmp(pParam, "-ze-no-local-to-generic") == 0) || // temp
                            (strcmp(pParam, "-cl-intel-debug-info") == 0) ||
                            (strncmp(pParam, "-dump-opt-llvm", 14) == 0) ||
                            (strcmp(pParam, "-cl-no-subgroup-ifp") == 0) ||
                            (strcmp(pParam, "-cl-intel-disable-a64WA") == 0) || //temporary options
                            (strcmp(pParam, "-ze-gtpin-rera") == 0) || //used by GTPin
                            (strcmp(pParam, "-ze-gtpin-grf-info") == 0) || //used by GTPin
                            (strcmp(pParam, "-ze-gtpin-scratch-area-size") == 0) || //used by GTPin
                            (strcmp(pParam, "-ze-opt-enable-auto-large-GRF-mode") == 0) ||
                            (strcmp(pParam, "-cl-intel-enable-auto-large-GRF-mode") == 0) ||
                            (strcmp(pParam, "-ze-skip-fde") == 0) ||
                            (strcmp(pParam, "-ze-no-fusedCallWA") == 0) ||
                            (strcmp(pParam, "-ze-disable-compaction") == 0) ||
                            (strcmp(pParam, "-cl-poison-unsupported-fp64-kernels") == 0) || //used by fp64 poisoning
                            (strcmp(pParam, "-ze-poison-unsupported-fp64-kernels") == 0) || //used by fp64 poisoning
                            (strcmp(pParam, "-cl-fp64-gen-emu") == 0) || //used by fp64 emulation
                            (strcmp(pParam, "-ze-fp64-gen-emu") == 0) || //used by fp64 emulation
                            (strcmp(pParam, "-cl-fp64-gen-conv-emu") == 0) || //used by fp64 conversion emulation
                            (strcmp(pParam, "-ze-fp64-gen-conv-emu") == 0) || //used by fp64 conversion emulation
                            (strcmp(pParam, "-Xfinalizer") == 0) || // used to pass options to visa finalizer
                            (strcmp(pParam, "-cl-intel-enable-ieee-float-exception-trap") == 0) || // used to enable IEEE float exception trap
                            (strcmp(pParam, "-ze-intel-enable-ieee-float-exception-trap") == 0) || // used to enable IEEE float exception trap
                            (strcmp(pParam, "-cl-intel-static-profile-guided-trimming") == 0) || //used to enable profile-guided trimming
                            (strcmp(pParam, "-ze-opt-static-profile-guided-trimming") == 0); //used to enable profile-guided trimming

                        if (isCommonOption)
                        {
                            // check to see if they used a space immediately after
                            // the define/include. If they did...
                            if ((strcmp(pParam, "-D") == 0) ||
                                (strcmp(pParam, "-I") == 0) ||
                                (strcmp(pParam, "-igc_opts") == 0))
                            {
                                // ignore next token as it is the define/include
                                ignoreNextToken = true;
                            }
                            else if (strcmp(pParam, "-x") == 0)
                            {
                                // we need to check the next parameter for "spir"
                                checkBinaryType = true;
                            }
                            else if (strcmp(pParam, "-triple") == 0)
                            {
                                checkBinaryType = true;
                            }
                            else if (strcmp(pParam, "-cl-intel-num-thread-per-eu") == 0 ||
                                     strcmp(pParam, "-cl-intel-reqd-eu-thread-count") == 0 ||
                                     strcmp(pParam, "-ze-gtpin-scratch-area-size") == 0 ||
                                     strcmp(pParam, "-ze-exp-register-file-size") == 0)
                            {
                                // Next token is N, so ignore it
                                ignoreNextToken = true;
                            }
                            else if (
                                (strcmp(pParam, "-cl-intel-large-grf-kernel") == 0) ||
                                (strcmp(pParam, "-ze-opt-large-grf-kernel") == 0) ||
                                (strcmp(pParam, "-cl-intel-regular-grf-kernel") == 0) ||
                                (strcmp(pParam, "-ze-opt-regular-grf-kernel") == 0))
                            {
                                // Next token is kernel name substring, ignore it
                                ignoreNextToken = true;
                            }
                            else if (strcmp(pParam, "-Xfinalizer") == 0)
                            {
                                // if -Xfinalizer is used, ignore next token as it is the option for visa
                                ignoreNextToken = true;
                            }
                        }
                        // Check for Intel OpenCL CPU options
                        // OCL Kernel Profiler requires "-g" to create debug information for instrumented kernels.
                        // Without those information OCL Profiler is unable to associate OpenC code with IL instructions.
                        else if ((strcmp(pParam, "-g") == 0) ||
                            (strcmp(pParam, "-profiler") == 0) ||
                            (strncmp(pParam, "-s", 2) == 0))
                        {
                            if (strcmp(pParam, "-s") == 0)
                            {
                                // ignore next token as it is the source path
                                ignoreNextToken = true;
                            }
                        }
                        else
                        {
                            // Invalid option - break out of the loop and return
                            // CL_INVALID_BUILD_OPTIONS
                            retVal = -43;
                            std::string invalidOption(pParam);

                            exceptString += "\nUnrecognized build options: " + invalidOption;
                            break;
                        }
                    }
                    else
                    {
                        ignoreNextToken = false;
                    }
                    strncpy_s(pBuffer, optionsSize, nextTok, optionsSize);
                } while ((pParam = GetParam(pBuffer, nextTok)) != NULL);
            }

            delete[] pBuffer;
            delete[] nextTok;
        }
        return retVal;
    }



    /*****************************************************************************\

    Function:
    CClangTranslationBlock::TranslateClang

    Description:
    Translates from CL to LL/BC

    Input:

    Output:

    \*****************************************************************************/
    bool CClangTranslationBlock::TranslateClang(const TranslateClangArgs* pInputArgs,
        STB_TranslateOutputArgs* pOutputArgs, std::string& exceptString, const char* pInternalOptions)
    {
        // additional clang options
        std::string optionsEx = pInputArgs->optionsEx;
        std::string options = pInputArgs->options;
        optionsEx.append(" -disable-llvm-optzns -fblocks -I. -D__ENABLE_GENERIC__=1");

        switch (m_OutputFormat)
        {
        case TB_DATA_FORMAT_LLVM_TEXT:
            optionsEx += " -emit-llvm";
            break;
        case TB_DATA_FORMAT_LLVM_BINARY:
            optionsEx += " -emit-llvm-bc";
            break;
        case TB_DATA_FORMAT_SPIR_V:
            optionsEx += " -emit-spirv";
            break;
        default:
            break;
        }

        if (AreVMETypesDefined()) {
          optionsEx += " -D__VME_TYPES_DEFINED__";
        }

        if (options.find("-triple") == std::string::npos) {
            // if target triple not explicitly set
            if (pInputArgs->b32bit)
            {
                optionsEx += " -D__32bit__=1";
                options += " -triple spir";
            }
            else
            {
                options += " -triple spir64";
            }
        }

        if (options.find("-gline-tables-only") != std::string::npos)
        {
            optionsEx += " -debug-info-kind=line-tables-only -dwarf-version=4";
        }

        // Possibly override the opaque/typed pointers' setting based on what's
        // passed from the build system.
        // TODO: Consider introducing a separate FCL environment variable for
        // tweaking this, similarly to IGC_EnableOpaquePointersBackend.
        optionsEx += " ";
        optionsEx += __IGC_OPAQUE_POINTERS_DEFAULT_ARG_CLANG;

        std::string extensionsFromInternalOptions = GetSubstring(pInternalOptions, "-cl-ext=");

        std::string extensions;

        // if extensions list is passed in via internal options, it will override the default ones.
        if (extensionsFromInternalOptions.size() != 0)
        {
            extensions = extensionsFromInternalOptions;
        }
        else
        {
            extensions = FormatExtensionsString(m_Extensions);
        }

        optionsEx += " " + extensions;

        unsigned int oclStd = GetOclCVersionFromOptions(pInputArgs->options.data(), nullptr, pInputArgs->oclVersion, exceptString);
        // get additional -D flags from internal options
        optionsEx += " " + GetCDefinesFromInternalOptions(pInternalOptions);
        optionsEx += " " + GetCDefinesForEnableList(extensions, oclStd, "-cl-ext=-all,");

        optionsEx += " -D__ENDIAN_LITTLE__";

        optionsEx += " -D__LLVM_VERSION_MAJOR__=" + to_string(LLVM_VERSION_MAJOR);

        // Workaround for Clang issue.
        // Clang always defines __IMAGE_SUPPORT__ macro for SPIR target, even if device doesn't support it.
        if (optionsEx.find("__IMAGE_SUPPORT__") == std::string::npos) {
            optionsEx += " -U__IMAGE_SUPPORT__";
        }

        // TODO: Workaround - remove after some time to be consistent with LLVM15+ behavior
        optionsEx += " -Wno-error=implicit-int";

        IOCLFEBinaryResult *pResultPtr = NULL;
        int res = 0;
        {
#ifdef _WIN32
            static std::mutex cclangMtx;
            std::lock_guard<std::mutex> lck(cclangMtx);
            res = m_CCModule.pCompile(
#else
            res = Compile(
#endif
                pInputArgs->pszProgramSource,
                (const char**)pInputArgs->inputHeaders.data(),
                (unsigned int)pInputArgs->inputHeaders.size(),
                (const char**)pInputArgs->inputHeadersNames.data(),
                NULL,
                0,
                options.c_str(),
                optionsEx.c_str(),
                pInputArgs->oclVersion.c_str(),
                &pResultPtr);
        }
        if (0 != BuildOptionsAreValid(options, exceptString)) res = -43;

        Utils::FillOutputArgs(pResultPtr, pOutputArgs, exceptString);
        if (!exceptString.empty()) // str != "" => there was an exception. skip further code and return.
        {
            return false;
        }

        // if -dump-opt-llvm is enabled dump the llvm output to the file
        size_t dumpOptPosition = options.find("-dump-opt-llvm");
        if ((0 == res) && dumpOptPosition != std::string::npos)
        {
            std::string dumpFileName;
            std::istringstream iss(options.substr(dumpOptPosition));
            iss >> dumpFileName;
            size_t equalSignPosition = dumpFileName.find('=');
            if (equalSignPosition != std::string::npos)
            {
                dumpFileName = dumpFileName.substr(equalSignPosition + 1);
                // dump the archive
                FILE* file = fopen(dumpFileName.c_str(), "wb");
                if (file != NULL)
                {
                    fwrite(pOutputArgs->pOutput, pOutputArgs->OutputSize, 1, file);
                    fclose(file);
                }
            }
            else
            {
                SetErrorString("\nWarning: File name not specified with the -dump-opt-llvm option.\n", pOutputArgs);
            }
        }

        pResultPtr->Release();

        return (0 == res);
    }

    /*****************************************************************************\

    Function:
    CClangTranslationBlock::ReturnSuppliedIR

    Description: Extract the IR from the input arguments and supply it unmodified
    to the output

    Input:

    Output:

    \*****************************************************************************/
    bool CClangTranslationBlock::ReturnSuppliedIR(const STB_TranslateInputArgs* pInputArgs,
        STB_TranslateOutputArgs* pOutputArgs)
    {
        bool success = true;

        pOutputArgs->ErrorStringSize = 0;
        pOutputArgs->pErrorString = NULL;
        pOutputArgs->OutputSize = 0;
        pOutputArgs->pOutput = NULL;

        CElfReader* pElfReader = CElfReader::Create(pInputArgs->pInput, pInputArgs->InputSize);
        RAIIElf ElfObj(pElfReader);

        if (!pElfReader || !pElfReader->IsValidElf(pInputArgs->pInput, pInputArgs->InputSize))
        {
            SetErrorString("Invalid input/output passed to library", pOutputArgs);
            return false;
        }

        const SElfHeader* pHeader = pElfReader->GetElfHeader();
        IGC_ASSERT(pHeader != NULL);

        for (unsigned i = 1; i < pHeader->NumSectionHeaderEntries; i++)
        {
            const SElfSectionHeader* pSectionHeader = pElfReader->GetSectionHeader(i);
            IGC_ASSERT(pSectionHeader != NULL);
            if (pSectionHeader == NULL)
            {
                SetErrorString("No section header", pOutputArgs);
                return false;
            }

            if ((pSectionHeader->Type == SH_TYPE_OPENCL_LLVM_ARCHIVE) ||
                (pSectionHeader->Type == SH_TYPE_OPENCL_LLVM_BINARY))
            {
                char *pData = NULL;
                size_t dataSize = 0;
                const unsigned char *pBufStart;

                if (pOutputArgs->pOutput != NULL)
                {
                    SetErrorString("Multiple inputs passed to library", pOutputArgs);
                    success = false;
                    break;
                }

                pElfReader->GetSectionData(i, pData, dataSize);
                pBufStart = (const unsigned char *)pData;

                if (llvm::isBitcode(pBufStart, pBufStart + pHeader->ElfHeaderSize))
                {
                    pOutputArgs->OutputSize = dataSize;
                    pOutputArgs->pOutput = (char*)malloc(dataSize);

                    if (pOutputArgs->pOutput == NULL)
                    {
                        SetErrorString("Error allocating memory", pOutputArgs);
                        success = false;
                        break;
                    }

                    memcpy_s(pOutputArgs->pOutput, dataSize, pBufStart, dataSize);
                }
                else
                {
                    SetErrorString("Invalid input/output passed to library", pOutputArgs);
                    success = false;
                    break;
                }
            }
        }

        return success;
    }

    ///
    // Process the translation of ELF input type
    //
    bool CClangTranslationBlock::TranslateElf(const STB_TranslateInputArgs* pInputArgs,
        STB_TranslateOutputArgs* pOutputArgs,
        std::string& exceptString)
    {
        CElfReaderPtr pElfReader(CElfReader::Create(pInputArgs->pInput, pInputArgs->InputSize), ElfReaderDP);

        if (!pElfReader.get())
        {
            SetErrorString("CElfReader::Create returned NULL\n", pOutputArgs);
            return false;
        }

        if (!pElfReader->IsValidElf(pInputArgs->pInput, pInputArgs->InputSize))
        {
            //throw invalid_input_param ("Wrong ELF format");
            exceptString = "Wrong ELF format";
            return false;
        }
        // Elf is valid, so it is safe to access the header
        E_EH_TYPE ehType = *(const E_EH_TYPE *)&pElfReader->GetElfHeader()->Type;

        //{ { TB_DATA_FORMAT_ELF, TB_DATA_FORMAT_LLVM_BINARY } },
        if (m_OutputFormat == TB_DATA_FORMAT_LLVM_BINARY || m_OutputFormat == TB_DATA_FORMAT_SPIR_V)
        {
            switch (ehType)
            {
            case EH_TYPE_OPENCL_SOURCE:
            {
                TranslateClangArgs args;
                GetTranslateClangArgs(pElfReader.get(),
                    pInputArgs->pOptions,
                    pInputArgs->pInternalOptions,
                    &args,
                    exceptString);
                bool success = TranslateClang(&args, pOutputArgs, exceptString, pInputArgs->pInternalOptions);
                if (exceptString.empty())
                {
                    return success;
                }
                else
                {
                    return false;
                }
            }

            case EH_TYPE_OPENCL_OBJECTS:
                if (strstr(pInputArgs->pOptions, "-x spir") == NULL)
                {
                    exceptString = "Unsupported ELF container";
                    return false;
                }

                return ReturnSuppliedIR(pInputArgs, pOutputArgs);

            default:
                exceptString = "Unsupported ELF header type";
                return false;
            }
        }
        else
        {
            exceptString = "Unsupported output format";
        }

        return false;
    }

    /*****************************************************************************\

    Function:
    CClangTranslationBlock::Translate

    Description:

    Input:

    Output:

    \*****************************************************************************/
    bool CClangTranslationBlock::Translate(const STB_TranslateInputArgs* pInputArgs,
        STB_TranslateOutputArgs* pOutputArgs)
    {
        // Setup a MutexGuard so that it's automatically released if it goes out of
        // scope
        //llvm::MutexGuard mutexGuard(m_Mutex);
        //resetOptionOccurrence();

        std::string exceptString;
        switch (m_InputFormat)
        {
            // Processing the translations from OCL to LLVM
            //{{ TB_DATA_FORMAT_OCL_TEXT,      TB_DATA_FORMAT_LLVM_TEXT }} ,
            //{{ TB_DATA_FORMAT_OCL_TEXT,      TB_DATA_FORMAT_LLVM_BINARY }},
        case TB_DATA_FORMAT_OCL_TEXT:
        {
            TranslateClangArgs args;
            GetTranslateClangArgs(pInputArgs->pInput,
                pInputArgs->InputSize,
                pInputArgs->pOptions,
                pInputArgs->pInternalOptions,
                &args,
                exceptString);
            bool successTC = TranslateClang(&args, pOutputArgs, exceptString, pInputArgs->pInternalOptions);

#if defined(IGC_DEBUG_VARIABLES)
            if (pInputArgs->pOptions != nullptr)
            {
                const std::string optionsWithFlags = pInputArgs->pOptions;
                std::size_t found = optionsWithFlags.find("-igc_opts");
                if (found != std::string::npos)
                {
                    std::size_t foundFirstSingleQuote = optionsWithFlags.find('\'', found);
                    std::size_t foundSecondSingleQuote = optionsWithFlags.find('\'', foundFirstSingleQuote + 1);
                    if (foundFirstSingleQuote != std::string::npos && foundSecondSingleQuote != std::string::npos)
                    {
                        FCL::RegKeysFlagsFromOptions = optionsWithFlags.substr(foundFirstSingleQuote + 1, foundSecondSingleQuote - foundFirstSingleQuote - 1);
                        FCL::RegKeysFlagsFromOptions = FCL::RegKeysFlagsFromOptions + ',';
                    }
                }
            }

            if (FCL_IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
            {
                // Works for all OSes. Creates dir if necessary.
                const char *pOutputFolder = FCL::GetShaderOutputFolder();
                stringstream ss;
                const char* pBuffer = pInputArgs->pInput;
                UINT  bufferSize = pInputArgs->InputSize;

                if (FCL_IGC_IS_FLAG_ENABLED(EnableKernelNamesBasedHash))
                    // The spirv parser cannot be used here - we would have to include it in the Makefile
                    // which is simply not worth it. Without it there's no good and reliable way to get
                    // the kernel names for the hash generation, so the warning is to be emitted instead.
                {
                    std::string errorString("");
                    if (pOutputArgs->pErrorString)
                        errorString = pOutputArgs->pErrorString;
                    errorString.append("warning: EnableKernelNamesBasedHash flag doesn't affect .cl dump's hash\n");
                    SetErrorString(errorString.c_str(), pOutputArgs);
                }

                // Create hash based on cclang binary output (currently llvm binary; later also spirv).
                // Hash computed in fcl needs to be same as the one computed in igc.
                // This is to ensure easy matching .cl files dumped in fcl with .ll/.dat/.asm/... files dumped in igc.
                QWORD hash = iSTD::Hash(reinterpret_cast<const DWORD *>(pOutputArgs->pOutput), (DWORD)(pOutputArgs->OutputSize) / 4);

                ss << pOutputFolder;
                ss << "OCL_"
                    << "asm"
                    << std::hex
                    << std::setfill('0')
                    << std::setw(sizeof(hash) * CHAR_BIT / 4)
                    << hash
                    << std::dec
                    << std::setfill(' ')
                    << ".cl";

                FILE* pFile = NULL;
                fopen_s(&pFile, ss.str().c_str(), "wb");
                if (pFile)
                {
                    fwrite(pBuffer, 1, bufferSize, pFile);
                    fclose(pFile);
                }
            }
#endif

            if (exceptString.empty())
            {
                return successTC;
            }
            //else continue to process "catch"
            break;
        }
        // Processing the translations from ELF to LLVM
        //{ { TB_DATA_FORMAT_ELF, TB_DATA_FORMAT_LLVM_BINARY } },
        case TB_DATA_FORMAT_ELF:
        {
            bool successTE = TranslateElf(pInputArgs, pOutputArgs, exceptString);
            if (exceptString.empty())
            {
                return successTE;
            }
            //else continue to process exception
            break;
        }
        default:
        {
            exceptString = "Unsupported input format";
            //continue to process exception
        }
        }

        // exceptString != "" => there was an exception. we get here only if there is an exception
        {
            if (exceptString.compare("bad_alloc") == 0)
            {
                SetErrorString("fcl: Allocation failure", pOutputArgs);
                return false;
            }
            else
            {
                SetErrorString(exceptString.c_str(), pOutputArgs);
                return false;
            }
        }
    }

    /*****************************************************************************\

    Function:
    CClangTranslationBlock::FreeAllocations

    Description:

    Input:

    Output:

    \*****************************************************************************/
    bool CClangTranslationBlock::FreeAllocations(
        STB_TranslateOutputArgs* pOutputArgs)
    {
        pOutputArgs->ErrorStringSize = 0;
        if (pOutputArgs->pErrorString != NULL)
        {
            free(pOutputArgs->pErrorString);
            pOutputArgs->pErrorString = NULL;
        }

        pOutputArgs->OutputSize = 0;
        if (pOutputArgs->pOutput != NULL)
        {
            free(pOutputArgs->pOutput);
            pOutputArgs->pOutput = NULL;
        }

        return true;
    }

    /*****************************************************************************\

    Function:
    CClangTranslationBlock constructor

    Description:

    Input:

    Output:

    \*****************************************************************************/
    CClangTranslationBlock::CClangTranslationBlock(void) :
        m_GlobalData()
    {
    }

    /*****************************************************************************\

    Function:
    CClangTranslationBlock destructor

    Description:

    Input:

    Output:

    \*****************************************************************************/
    CClangTranslationBlock::~CClangTranslationBlock(void)
    {
    }


    /*****************************************************************************\

    Function:
    CClangTranslationBlock::Initialize

    Description:

    Input:

    Output:

    \*****************************************************************************/
    bool CClangTranslationBlock::Initialize(const STB_CreateArgs* pCreateArgs)
    {
        if (pCreateArgs == NULL)
        {
            return false;
        }

        unsigned int i;
        for (i = 0; i < std::size(g_cClangTranslationCodes); i++)
        {
            // Some quick checks to ensure that the input and output types
            // are compatible with this translation blocks
            if ((pCreateArgs->TranslationCode.Type.Input == g_cClangTranslationCodes[i].Type.Input) &&
                (pCreateArgs->TranslationCode.Type.Output == g_cClangTranslationCodes[i].Type.Output))
            {
                break;
            }
        }

        if (i >= std::size(g_cClangTranslationCodes))
        {
            return false;
        }

        // Find out what GPU platform the driver is running on
        if (pCreateArgs->pCreateData != nullptr) {
        }
        else {
            // assume m_OCL_Ver, etc. will be set-up later.
            SGlobalData globDataTmp = { 0 };
            m_GlobalData = globDataTmp;
            m_HWPlatform = IGFX_UNKNOWN;
            m_CoreFamily = IGFX_UNKNOWN_CORE;
            m_OCL_Ver = "120";
        }
        m_InputFormat = pCreateArgs->TranslationCode.Type.Input;
        m_OutputFormat = pCreateArgs->TranslationCode.Type.Output;

        return true;
    }
} // namespace TC
