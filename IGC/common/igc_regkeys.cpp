/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#include "igc_regkeys.hpp"
#if defined(IGC_DEBUG_VARIABLES)

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/CommandLine.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/SysUtils.hpp"
#include "3d/common/iStdLib/File.h"
#include "secure_mem.h"
#include "secure_string.h"

#include <string>
#include <cassert>
#include <utility>
#include <fstream>
#include <sstream>
#include <mutex>

// path for IGC registry keys
#define IGC_REGISTRY_KEY "SOFTWARE\\INTEL\\IGFX\\IGC"

SRegKeysList g_RegKeyList;


/*****************************************************************************\
ReadIGCEnv
\*****************************************************************************/
static bool ReadIGCEnv(
    const char*  pName,
    void*        pValue,
    unsigned int size )
{
    if( pName != NULL )
    {
        const char nameTag[] = "IGC_";
        std::string pKey = std::string(nameTag) + std::string(pName);

        const char* envVal = getenv(pKey.c_str());

        if( envVal != NULL )
        {
            if( size >= sizeof( unsigned int ) )
            {
                // Try integer conversion
                char* pStopped = nullptr;
                unsigned int *puVal = (unsigned int *)pValue;
                *puVal = strtoul(envVal, &pStopped, 0);
                if( pStopped == envVal + strlen(envVal) )
                {
                    return true;
                }
            }

            // Just return the string
            strncpy_s( (char*)pValue, size, envVal, size );

            return true;
        }
    }

    return false;
}

/*****************************************************************************\
ReadIGCRegistry
\*****************************************************************************/
static bool ReadIGCRegistry(
    const char*  pName,
    void*        pValue,
    unsigned int size )
{
    // All platforms can retrieve settings from environment
    if( ReadIGCEnv( pName, pValue, size ) )
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
        &uscKey );

    if( ERROR_SUCCESS == success )
    {
        DWORD dwSize = size;
        success = RegQueryValueExA(
            uscKey,
            pName,
            NULL,
            NULL,
            (LPBYTE)pValue,
            &dwSize );

        RegCloseKey( uscKey );
    }
    return ( ERROR_SUCCESS == success );
#endif // defined _WIN32

    return false;
}

/*****************************************************************************\

Function:
    DumpIGCRegistryKeyDefinitions

Description:
    Dumps registry key definitions to an XML file.

Input:
    none

Output:
    none

\*****************************************************************************/
static const char* ConvertType(const char* flagType)
{
    if(strcmp(flagType, "bool") == 0)
    {
        return "bool";
    }
    if((strcmp(flagType, "int") == 0) || (strcmp(flagType, "DWORD") == 0))
    {
        return "DWORD";
    }
    if(strcmp(flagType, "debugString") == 0)
    {
        return "string";
    }
    return flagType;
}
#define DECLARE_IGC_REGKEY( dataType, regkeyName, defaultValue, descriptionText, releaseMode ) \
    fprintf(fp, "    <Key name=\"%s\" type=\"%s\" location=\"%s\\%s\" description=\"%s\" />\n", \
        #regkeyName,                                                                             \
        ConvertType(#dataType),                                                                               \
        "HKLM\\" IGC_REGISTRY_KEY,                                                              \
        "",                                                                                     \
        descriptionText);
#define DECLARE_IGC_GROUP( groupName ) \
    if(!firstGroup)                    \
    {                                  \
        fprintf(fp, "  </Group>\n");   \
    }                                  \
    firstGroup = false;                \
    fprintf(fp, "  <Group name=\"%s\">\n", groupName);
void DumpIGCRegistryKeyDefinitions()
{
#ifdef _WIN32
    // Create the directory path
    iSTD::DirectoryCreate("C:\\Intel");
    iSTD::DirectoryCreate("C:\\Intel\\IGfx");
    iSTD::DirectoryCreate("C:\\Intel\\IGfx\\GfxRegistryManager");
    iSTD::DirectoryCreate("C:\\Intel\\IGfx\\GfxRegistryManager\\Keys");

    // Create the XML file to hold the debug variable definitions
    FILE* fp = fopen("C:\\Intel\\IGfx\\GfxRegistryManager\\Keys\\IGC.xml", "w");

    if (fp == NULL)
    {
        return;
    }
    bool firstGroup = true;
    // Generate the XML
    fprintf(fp, "<RegistryKeys>\n");
#include "igc_regkeys.def"
    fprintf(fp, "  </Group>\n");
    fprintf(fp, "</RegistryKeys>\n");

    fclose(fp);
    fp = NULL;
#endif
}
#undef DECLARE_IGC_REGKEY
#undef DECLARE_IGC_GROUP

/// Function taken from LLVM CommandLine.cpp file
/// ParseCStringVector - Break INPUT up wherever one or more
/// whitespace characters are found, and store the resulting tokens in
/// OUTPUT. The tokens stored in OUTPUT are dynamically allocated
/// using strdup(), so it is the caller's responsibility to free()
/// them later.
///
static void ParseCStringVector(std::vector<char *> &OutputVector,
    const char *Input) {
    // Characters which will be treated as token separators:
    llvm::StringRef Delims = " \v\f\t\r\n";

    llvm::StringRef WorkStr(Input);
    while(!WorkStr.empty()) {
        // If the first character is a delimiter, strip them off.
        if(Delims.find(WorkStr[0]) != llvm::StringRef::npos) {
            size_t Pos = WorkStr.find_first_not_of(Delims);
            if(Pos == llvm::StringRef::npos) Pos = WorkStr.size();
            WorkStr = WorkStr.substr(Pos);
            continue;
        }

        // Find position of first delimiter.
        size_t Pos = WorkStr.find_first_of(Delims);
        if(Pos == llvm::StringRef::npos) Pos = WorkStr.size();

        // Everything from 0 to Pos is the next word to copy.
        char *NewStr = (char*)malloc(Pos + 1);
        memcpy_s(NewStr, Pos + 1, WorkStr.data(), Pos);
        NewStr[Pos] = 0;
        OutputVector.push_back(NewStr);

        WorkStr = WorkStr.substr(Pos);
    }
}

static void setRegkeyFromOption(
    std::string&     optionValue,
    const char* dataTypeName,
    const char* regkeyName,
    void*       pRegkeyVar,
    bool&       isKeySet)
{
    size_t sPos = optionValue.find(regkeyName);
    if(sPos == std::string::npos)
    {
        return;
    }
    size_t pos = sPos + strlen(regkeyName);
    pos = (pos < optionValue.size()) ? pos : std::string::npos;
    std::string vstring;
    if(pos != std::string::npos && optionValue.at(pos) == '=')
    {
        // Get value for this option, ',' is a value separator.
        sPos = pos + 1;
        pos = optionValue.find(',', sPos);
        size_t len = (pos == std::string::npos) ? pos : (pos - sPos);
        vstring = optionValue.substr(sPos, len);
    }

    bool isBool = (strcmp(dataTypeName, "bool") == 0);
    bool isInt = (strcmp(dataTypeName, "int") == 0) ||
        (strcmp(dataTypeName, "DWORD") == 0);
    bool isString = (strcmp(dataTypeName, "debugString") == 0);
    if(isBool)
    {
        bool bval = true;
        if(vstring.size() == 1)
        {
            bval = (vstring.at(0) == '0') ? false : true;
        }
        *((bool*)pRegkeyVar) = bval;
        isKeySet = true;
    }
    else if(isInt)
    {
        // ignore if there is no value
        if(!vstring.empty())
        {
            // assume vstring has the valid value!
            int intval = atoi(vstring.c_str());
            *((int*)pRegkeyVar) = intval;
            isKeySet = true;
        }
    }
    else if(isString)
    {
        // ignore if there is no value
        if(!vstring.empty())
        {
            char* s = (char*)pRegkeyVar;
            strcpy_s(s, vstring.size(), vstring.c_str());
            isKeySet = true;
        }
    }
}

static const char* GetOptionFile()
{
#if defined(_WIN64) || defined(_WIN32)
    return "c:\\Intel\\IGC\\debugFlags\\Options.txt";
#else
    return "/tmp/IntelIGC/debugFlags/Options.txt";
#endif
}

// parses this syntax:
// hash:abcdabcdabcdabcd-ffffffffffffffff,aaaaaaaaaaaaaaaa
static void ParseHashRange(std::string line, std::vector<HashRange>& ranges)
{
    size_t sPos = line.find("hash:");
    if(sPos == std::string::npos)
    {
        return;
    }
    // re-initialize ranges
    ranges.clear();
    std::string vString = line.substr(sPos + 5);
    std::string token;
    std::stringstream sline(vString);
    while(std::getline(sline, token, ','))
    {
        size_t dash = token.find('-');
        std::string start = token.substr(0, dash);
        std::string end;
        if(dash == std::string::npos)
        {
            end = start;
        }
        else
        {
            end = token.substr(dash + 1);
        }
        HashRange range;
        range.start = std::stoull(start, nullptr, 16);
        range.end = std::stoull(end, nullptr, 16);
        ranges.push_back(range);
    }

}

static void declareIGCKey(std::string& line, const char* dataType, const char* regkeyName, std::vector<HashRange>& hashes, SRegKeyVariableMetaData* regKey)
{
    bool isSet = false;
    debugString value = { 0 };
    setRegkeyFromOption(line, dataType, regkeyName, &value, isSet);
    if (isSet)
    {
        memcpy_s(regKey->m_string, sizeof(value), value, sizeof(value));
        regKey->hashes = hashes;
    }
}

static void LoadDebugFlagsFromFile()
{
    std::ifstream input(GetOptionFile());
    std::string line;
    std::vector<HashRange> hashes;

    while (std::getline(input, line)) {
        ParseHashRange(line, hashes);
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode)         \
{                                                                                   \
    declareIGCKey(line, #dataType, #regkeyName, hashes, &(g_RegKeyList.regkeyName));\
}
#include "igc_regkeys.def"
#undef DECLARE_IGC_REGKEY

    }
}

thread_local unsigned long long g_CurrentShaderHash = 0;
bool CheckHashRange(const std::vector<HashRange>& hashes)
{
    if(hashes.empty())
    {
        return true;
    }
    for(auto it : hashes)
    {
        if(g_CurrentShaderHash >= it.start && g_CurrentShaderHash <= it.end)
        {
            return true;
        }
    }
    return false;
}

static void LoadFromRegKeyOrEnvVar()
{
    SRegKeyVariableMetaData* pRegKeyVariable = (SRegKeyVariableMetaData*)&g_RegKeyList;
    unsigned NUM_REGKEY_ENTRIES = sizeof(SRegKeysList) / sizeof(SRegKeyVariableMetaData);
    for (DWORD i = 0; i < NUM_REGKEY_ENTRIES; i++)
    {
        debugString value = { 0 };
        const char* name = pRegKeyVariable[i].GetName();

        bool isSet = ReadIGCRegistry(
            name,
            &value,
            sizeof(value));

        if (isSet)
        {
            memcpy_s(pRegKeyVariable[i].m_string, sizeof(value), value, sizeof(value));
        }
    }
}

void SetCurrentDebugHash(unsigned long long hash)
{
    g_CurrentShaderHash = hash;
}

/*****************************************************************************\

Function:
    LoadRegistryKeys

Description:
    Loads registry variables from the registry

Input:
    None

Output:
    None

\*****************************************************************************/
void LoadRegistryKeys( void )
{
    // only load the debug flags once before compiling to avoid any multi-threading issue
    static std::mutex loadFlags;
    static volatile bool flagsSet = false;
    loadFlags.lock();
    if(!flagsSet)
    {
        flagsSet = true;
        // dump out IGC.xml for the registry manager
        DumpIGCRegistryKeyDefinitions();
        LoadDebugFlagsFromFile();
        LoadFromRegKeyOrEnvVar();

        if(IGC_IS_FLAG_ENABLED(LLVMCommandLine))
        {
            std::vector<char*> args;
            args.push_back((char *)("IGC"));
            ParseCStringVector(args, IGC_GET_REGKEYSTRING(LLVMCommandLine));
            llvm::cl::ParseCommandLineOptions(args.size(), &args[0]);
        }

        if(IGC_IS_FLAG_ENABLED(DisableIGCOptimizations))
        {
            IGC_SET_FLAG_VALUE(DisableLLVMGenericOptimizations, true);
            IGC_SET_FLAG_VALUE(DisableCodeSinking, true);
            IGC_SET_FLAG_VALUE(DisableDeSSA, true);
            //disable now until we figure out the issue
            //IGC_SET_FLAG_VALUE(DisablePayloadCoalescing, true);
            IGC_SET_FLAG_VALUE(DisableSendS, true);
            IGC_SET_FLAG_VALUE(EnableVISANoSchedule, true);
            IGC_SET_FLAG_VALUE(DisableUniformAnalysis, true);
            IGC_SET_FLAG_VALUE(DisablePushConstant, true);
            IGC_SET_FLAG_VALUE(DisableConstantCoalescing, true);
            IGC_SET_FLAG_VALUE(DisableURBWriteMerge, true);
            IGC_SET_FLAG_VALUE(DisableCodeHoisting, true);
            IGC_SET_FLAG_VALUE(DisableEmptyBlockRemoval, true);
            IGC_SET_FLAG_VALUE(DisableSIMD32Slicing, true);
            IGC_SET_FLAG_VALUE(DisableCSEL, true);
            IGC_SET_FLAG_VALUE(DisableFlagOpt, true);
            IGC_SET_FLAG_VALUE(DisableScalarAtomics, true);
        }


        if(IGC_IS_FLAG_ENABLED(ShaderDumpEnableAll))
        {
            IGC_SET_FLAG_VALUE(ShaderDumpEnable, true);
            IGC_SET_FLAG_VALUE(EnableVISASlowpath, true);
            IGC_SET_FLAG_VALUE(EnableVISADumpCommonISA, true);
        }

        if(IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
        {
            IGC_SET_FLAG_VALUE(DumpLLVMIR, true);
            IGC_SET_FLAG_VALUE(EnableCosDump, true);
            IGC_SET_FLAG_VALUE(DumpOCLProgramInfo, true);
            IGC_SET_FLAG_VALUE(EnableVISAOutput, true);
            IGC_SET_FLAG_VALUE(EnableVISABinary, true);
            IGC_SET_FLAG_VALUE(EnableVISADumpCommonISA, true);
            IGC_SET_FLAG_VALUE(EnableCapsDump, true);
            IGC_SET_FLAG_VALUE(DumpPatchTokens, true);
        }


        switch(IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth))
        {
        case 32:
            IGC_SET_FLAG_VALUE(EnableOCLSIMD32, true);
            IGC_SET_FLAG_VALUE(EnableOCLSIMD16, false);
            break;
        case 16:
            IGC_SET_FLAG_VALUE(EnableOCLSIMD32, false);
            IGC_SET_FLAG_VALUE(EnableOCLSIMD16, true);
            break;
        case 8:
            IGC_SET_FLAG_VALUE(EnableOCLSIMD32, false);
            IGC_SET_FLAG_VALUE(EnableOCLSIMD16, false);
            break;
        default:
            // Non-valid value is ignored (using default).
            IGC_SET_FLAG_VALUE(ForceOCLSIMDWidth, 0);
        }
    }
    loadFlags.unlock();
}
#endif
