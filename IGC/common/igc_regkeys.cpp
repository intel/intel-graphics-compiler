/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "igc_regkeys.hpp"
#if defined(IGC_DEBUG_VARIABLES)

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/Support/CommandLine.h>
#include "common/LLVMWarningsPop.hpp"
#include "3d/common/iStdLib/File.h"
#include "secure_mem.h"
#include "secure_string.h"
#include "AdaptorCommon/customApi.hpp"

#if defined(_WIN64) || defined(_WIN32)
#include <devguid.h>  // for GUID_DEVCLASS_DISPLAY
#include <initguid.h> // for DEVPKEY_*
#include <Devpkey.h>  // for DEVPKEY_*
#include <SetupAPI.h>
#include <Shlwapi.h>
#include <algorithm>
#endif

#include <list>
#include <vector>
#include <string>
#include <utility>
#include <fstream>
#include <sstream>
#include <iostream>
#include <mutex>
#include <algorithm>
#include <optional>
#include "Probe/Assertion.h"
#include "common/Types.hpp"

// path for IGC registry keys
#define IGC_REGISTRY_KEY "SOFTWARE\\INTEL\\IGFX\\IGC"

SRegKeysList g_RegKeyList;

#if defined(_WIN64) || defined(_WIN32)

DEVICE_INFO::DEVICE_INFO(DEVINST deviceInstance)
{
    deviceID = 0;
    revisionID = 0;
    pciBus = 0;
    pciDevice = 0;
    pciFunction = 0;
    get_device_property(deviceInstance, CM_DRP_DEVICEDESC);
    get_device_property(deviceInstance, CM_DRP_COMPATIBLEIDS);
    get_device_property(deviceInstance, CM_DRP_DRIVER);
    get_device_property(deviceInstance, CM_DRP_LOCATION_INFORMATION);
}

void DEVICE_INFO::get_device_property(DEVINST deviceInstace, DWORD property)
{
    wchar_t propertyData[MAX_PATH];
    DWORD   propertyDataLength = 0;

    // First get the size of the data
    CONFIGRET status = CM_Get_DevNode_Registry_PropertyW(
        deviceInstace,
        property,
        NULL,
        NULL,
        &propertyDataLength,
        0);

    // Then fetch the actual property data
    if(CR_BUFFER_SMALL == status)
    {
        status = CM_Get_DevNode_Registry_PropertyW(
            deviceInstace,
            property,
            NULL,
            &propertyData[0],
            &propertyDataLength,
            0);
    }

    if(CR_SUCCESS == status)
    {
        // Convert from UTF-16 (wide char) to UTF-8
        DWORD convertedStringSize = WideCharToMultiByte(CP_UTF8, 0, &propertyData[0], sizeof(propertyData), NULL, 0, NULL, NULL);
        std::string propertyString(convertedStringSize, 0);
        WideCharToMultiByte(CP_UTF8, 0, &propertyData[0], sizeof(propertyData), &propertyString[0], convertedStringSize, NULL, NULL);

        switch(property)
        {
        case CM_DRP_COMPATIBLEIDS:
            // PCI\VEN_8086&DEV_1912&REV_06
            sscanf_s(propertyString.c_str(), "PCI\\VEN_8086&DEV_%x&REV_%x", &deviceID, &revisionID);
            break;
        case CM_DRP_DEVICEDESC:
            // Intel(R) HD Graphics 530
            description = propertyString.c_str();
            break;
        case CM_DRP_DRIVER:
            // {4d36e968-e325-11ce-bfc1-08002be10318}\0001
            driverRegistryPath = propertyString.c_str();
            break;
        case CM_DRP_LOCATION_INFORMATION:
            // PCI bus 0, device 2, function 0
            sscanf_s(propertyString.c_str(), "PCI bus %u, device %u, function %u", &pciBus, &pciDevice, &pciFunction);
            break;
        }
    }
    else if(CR_NO_SUCH_VALUE == status)
    {
        IGC_ASSERT_MESSAGE(0, "No such property value");
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "Failed to get DevNode property");
    }
}

/********************************************************************************************/
/* Function: ConvertDoubleNullTermStringToVector                                            */
/*                                                                                          */
/* Converts a Unicode string with embedded null characters                                  */
/* into a vector of strings                                                                 */
/*                                                                                          */
/********************************************************************************************/
static void ConvertDoubleNullTermStringToVector(std::vector<wchar_t>& input_str, std::vector<std::wstring>& output_vec)
{
    // Remove terminating null character to avoid adding empty string at the end
    input_str.resize(input_str.size() - 1);

    std::wstring str;
    std::for_each(input_str.cbegin(), input_str.cend(), [&](const wchar_t& w)
        {
            if (w != L'\0')
            {
                str += w;
            }
            else
            {
                output_vec.push_back(str);
                str = L"";
            }
        });
}


/*********************************************************************************************/
/* GetPropertyFromDevice                                                                     */
/*                                                                                           */
/* This function can be used to request a value of any device property.                      */
/* There are many types of properties values. Refer to devpkey.h for more details.           */
/* Function SetupDiGetDeviceProperty() inside GetPropertyFromDevice() fills a property value */
/* in a correct format, but always returns the value as a pointer to a string of bytes.      */
/* The string of bytes must be casted outside of the function GetPropertyFromDevice()        */
/* to get a value in a format suitable for the given type of property.                       */
/*                                                                                           */
/*********************************************************************************************/
static bool GetPropertyFromDevice(
    DEVINST devInst,
    const DEVPROPKEY& PropertyKey,
    std::vector<BYTE>& PropertyData)
{
    unsigned long propertySize = 0;
    DEVPROPTYPE propertyType;

    // request a size, in bytes, required for a buffer in which property value will be stored.
    // CM_Get_DevNode_PropertyW() returns false and ERROR_INSUFFICIENT_BUFFER for the call
    CONFIGRET status = CM_Get_DevNode_PropertyW(devInst, &PropertyKey, &propertyType, NULL, &propertySize, 0);
    if (status != CR_BUFFER_SMALL)
    {
        IGC_ASSERT_MESSAGE(0, "CM_Get_DevNode_PropertyW() failed, status indicates error code");
        return false;
    }

    // allocate memory for the buffer
    PropertyData.clear();
    PropertyData.resize(propertySize, 0);

    // fill in the buffer with property value
    status = CM_Get_DevNode_PropertyW(devInst, &PropertyKey, &propertyType, &PropertyData[0], &propertySize, 0);
    if (status != CR_SUCCESS)
    {
        IGC_ASSERT_MESSAGE(0, "CM_Get_DevNode_PropertyW() failed, status indicates error code");
        PropertyData.clear();
        return false;
    }

    return true;
}

/********************************************************************************************/
/* ObtainDeviceInstance                                                                     */
/*                                                                                          */
/* Obtains a device instance for the Intel graphics adapter                                 */
/********************************************************************************************/
static HRESULT ObtainDeviceInstances(std::vector<DEVINST>* pDevInstances)
{
    CONFIGRET cr = CR_SUCCESS;

    ULONG DeviceIDListSize = 0;
    std::vector<wchar_t> DeviceIDList;
    std::vector<std::wstring> Devices;
    std::wstring IntelDeviceID;
    wchar_t DisplayDevClassGUID[40];

    do
    {
        if (pDevInstances == nullptr)
        {
            cr = CR_INVALID_POINTER;
            break;
        }

        StringFromGUID2(GUID_DEVCLASS_DISPLAY, DisplayDevClassGUID, sizeof(DisplayDevClassGUID));

        do
        {
            DeviceIDListSize = 0;
            cr = CM_Get_Device_ID_List_SizeW(
                &DeviceIDListSize,
                DisplayDevClassGUID,
                CM_GETIDLIST_FILTER_CLASS); // we don't want to use CM_GETIDLIST_FILTER_PRESENT in order to support safe mode.
            if (cr != CR_SUCCESS)
            {
                break;
            }

            if (DeviceIDList.max_size() >= DeviceIDListSize && DeviceIDListSize > 0)
                DeviceIDList.resize(DeviceIDListSize);
            else
            {
                return E_ABORT;
            }

            cr = CM_Get_Device_ID_ListW(
                DisplayDevClassGUID,
                DeviceIDList.data(),
                DeviceIDListSize,
                CM_GETIDLIST_FILTER_CLASS);

        } while (cr == CR_BUFFER_SMALL);

        if (cr != CR_SUCCESS)
        {
            break;
        }

        ConvertDoubleNullTermStringToVector(DeviceIDList, Devices);

        for (auto device : Devices)
        {
            if ((device.find(L"VEN_8086") != std::wstring::npos) ||
                (device.find(L"ven_8086") != std::wstring::npos))
            {
                // Found an intel device, add it to the list
                DEVINST devInst;
                CONFIGRET crLocateDevNode = CM_Locate_DevNodeW(
                    &devInst,
                    const_cast<wchar_t*>(device.c_str()),
                    CM_LOCATE_DEVNODE_NORMAL);
                if (crLocateDevNode == CR_NO_SUCH_DEVNODE)
                {
                    // There can be many drivers on the system from past installs.
                    // CR_NO_SUCH_DEVNODE is returned for drivers that don't match any devices.
                    // It's ok to skip these.
                    continue;
                }
                else
                {
                    if (crLocateDevNode == CR_SUCCESS)
                    {
                        pDevInstances->push_back(devInst);
                    }

                    // Update the overall status
                    cr |= crLocateDevNode;
                }
            }
        }

    } while (FALSE);

    return HRESULT_FROM_WIN32(CM_MapCrToWin32Err(cr, ERROR_FILE_NOT_FOUND));
}

/************************************************************************/
/* GetIntelDriverPaths                                                  */
/*                                                                      */
/* Build a list of Intel graphics driver installation paths.            */
/*                                                                      */
/************************************************************************/
static size_t GetIntelDriverPaths(std::vector<DEVINST>& drivers)
{
    drivers.clear();
    HRESULT hr = ObtainDeviceInstances(&drivers);
    if (FAILED(hr))
    {
        IGC_ASSERT_MESSAGE(0, "Failed to find any graphics device instances");
        return 0;
    }
    return drivers.size();
}


std::string getNewRegistryPath(DEVINST deviceInstance)
{
    DEVICE_INFO devInfo(deviceInstance);
    return devInfo.driverRegistryPath;
}
#endif

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
                if (envVal[0] == '0' && std::tolower(envVal[1]) == 'b')
                {
                    // Binary literals, like in C++14
                    // Example: 0b0110'11'01
                    std::string str(envVal + 2); // -> 0110'11'01
                    // Remove optional C++14 digit separators and squeeze the result
                    str.erase(std::remove(str.begin(), str.end(), '\''), str.end()); // -> 01101101
                    std::size_t pos = 0;
                    int val = std::stoi(str, &pos, 2);
                    if (pos > 0 && pos == str.size())
                    {
                        *reinterpret_cast<unsigned int*>(pValue) = int_cast<unsigned int>(val);
                        return true;
                    }
                    else
                    {
                        // Like "0b", "0b1EFF", "0b''"
                        IGC_ASSERT_MESSAGE(0, "Invalid binary literal.");
                    }
                }
                else
                {
                    char* pStopped = nullptr;
                    unsigned int* puVal = (unsigned int*)pValue;
                    *puVal = strtoul(envVal, &pStopped, 0);
                    if (pStopped == envVal + std::strlen(envVal))
                    {
                        return true;
                    }
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
bool ReadIGCRegistry(
    const char*  pName,
    void*        pValue,
    unsigned int size ,
    bool readFromEnv)
{
    // All platforms can retrieve settings from environment
    if( readFromEnv && ReadIGCEnv( pName, pValue, size ))
    {
        return true;
    }

#if defined _WIN32
    LONG success = ERROR_SUCCESS;
    static std::vector<std::string> registrykeypaths;
    if (registrykeypaths.empty())
    {
        registrykeypaths.push_back(IGC_REGISTRY_KEY);
        std::vector<DEVINST> drivers;
        std::list<std::string> registrypaths;
        GetIntelDriverPaths(drivers);
        for (auto driverInfo : drivers)
        {
            std::string driverStoreRegKeyPath = getNewRegistryPath(driverInfo);
            std::string registryKeyPath = "SYSTEM\\ControlSet001\\Control\\Class\\" + driverStoreRegKeyPath + "\\IGC";
            registrykeypaths.push_back(registryKeyPath);
        }
    }

    for (const std::string& registrykeypath : registrykeypaths)
    {
        HKEY uscKey;
        success = RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            registrykeypath.c_str(),
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

        if (ERROR_SUCCESS == success)
        {
            return true;
        }
    }
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
    if (strcmp(flagType, "int") == 0)
        return "DWORD";
    else if(strcmp(flagType, "debugString") == 0)
        return "string";
    return flagType;
}

template <typename T>
static std::string ConvertDefault(
    const std::string &Type, T Val, const std::string &ValStr)
{
    if (Type == "bool")
    {
        return std::to_string(static_cast<bool>(Val) ? 1U : 0U);
    }
    else if (Type == "string")
    {
        if (Val == 0)
            return "";
        else if constexpr (std::is_convertible_v<T, std::string>)
            return Val;
    }
    return ValStr;
}

static void DumpIGCRegistryKeyDefinitions(
    const char* registryKeyPath, const char* xmlPath)
{
#ifdef _WIN32
    // Create the directory path
    iSTD::DirectoryCreate("C:\\Intel");
    iSTD::DirectoryCreate("C:\\Intel\\IGfx");
    iSTD::DirectoryCreate("C:\\Intel\\IGfx\\GfxRegistryManager");
    iSTD::DirectoryCreate("C:\\Intel\\IGfx\\GfxRegistryManager\\Keys");

    // Create the XML file to hold the debug variable definitions
    FILE* fp = fopen(xmlPath, "w");

    if (fp == NULL)
    {
        return;
    }

#define DECLARE_IGC_REGKEY( dataType, regkeyName, defaultValue, descriptionText, releaseMode )             \
    fprintf(fp, "    <Key name=\"%s\" type=\"%s\" location=\"%s\" default=\"%s\" description=\"%s\" />\n", \
        #regkeyName,                                                                                       \
        ConvertType(#dataType),                                                                            \
        registryKeyPath,                                                                                   \
        ConvertDefault(ConvertType(#dataType), defaultValue, #defaultValue).c_str(),                       \
        descriptionText);
#define DECLARE_IGC_REGKEY_ENUM(regkeyName, defaultValue, description, values, releaseMode) \
    DECLARE_IGC_REGKEY(enum, regkeyName, defaultValue, description "[VALUES]" values, releaseMode)
#define DECLARE_IGC_REGKEY_BITMASK(regkeyName, defaultValue, description, values, releaseMode) \
    DECLARE_IGC_REGKEY(bitmask, regkeyName, defaultValue, description "[VALUES]" values, releaseMode)
#define DECLARE_IGC_GROUP( groupName ) \
    if(!firstGroup)                    \
    {                                  \
        fprintf(fp, "  </Group>\n");   \
    }                                  \
    firstGroup = false;                \
    fprintf(fp, "  <Group name=\"%s\">\n", groupName);

    bool firstGroup = true;
    // Generate the XML
    fprintf(fp, "<RegistryKeys>\n");
#include "igc_regkeys.h"
    fprintf(fp, "  </Group>\n");
    fprintf(fp, "</RegistryKeys>\n");

    fclose(fp);
    fp = NULL;

#undef DECLARE_IGC_REGKEY
#undef DECLARE_IGC_GROUP
#undef DECLARE_IGC_REGKEY_ENUM
#undef DECLARE_IGC_REGKEY_BITMASK

#endif // _WIN32
}

void DumpIGCRegistryKeyDefinitions()
{
    constexpr const char* registryKeyPath = "HKLM\\" IGC_REGISTRY_KEY;
    constexpr const char* xmlPath =
        "C:\\Intel\\IGfx\\GfxRegistryManager\\Keys\\IGC.xml";

    DumpIGCRegistryKeyDefinitions(registryKeyPath, xmlPath);
}

void DumpIGCRegistryKeyDefinitions3(std::string driverRegistryPath, unsigned long pciBus, unsigned long pciDevice, unsigned long pciFunction)
{
    if (driverRegistryPath.empty())
    {
        IGC_ASSERT_MESSAGE(0, "Failed to find the driver registry path, cannot create the debug variable XML file.");
        return;
    }

    const std::string registryKeyPath =
        "HKLM\\SYSTEM\\ControlSet001\\Control\\Class\\" + driverRegistryPath + "\\IGC";

    const std::string xmlPath =
        "C:\\Intel\\IGfx\\GfxRegistryManager\\Keys\\IGC." +
        std::to_string(pciBus)      + "." +
        std::to_string(pciDevice)   + "." +
        std::to_string(pciFunction) + ".xml";

    DumpIGCRegistryKeyDefinitions(registryKeyPath.c_str(), xmlPath.c_str());
}

static void checkAndSetIfKeyHasNoDefaultValue(SRegKeyVariableMetaData* pRegKeyVariable)
{
    // Todo: need to interpret value based on its type
    if (pRegKeyVariable->m_Value != pRegKeyVariable->GetDefault())
    {
        pRegKeyVariable->SetToNonDefaultValue();
    }
}

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
    const std::string&     optionValue,
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
    size_t pos = sPos + std::strlen(regkeyName);
    pos = (pos < optionValue.size()) ? pos : std::string::npos;
    std::string vstring, exactNameStr;
    if(pos != std::string::npos && optionValue.at(pos) == '=')
    {
        // Get value for this option, ',' is a value separator.
        sPos = pos + 1;
        pos = optionValue.find(',', sPos);
        size_t len = (pos == std::string::npos) ? pos : (pos - sPos);
        vstring = optionValue.substr(sPos, len);
        exactNameStr = optionValue.substr(0, optionValue.find('='));
    }
    if (!exactNameStr.empty())
    {
        bool reqKeysNotEqual = (exactNameStr.compare(regkeyName) != 0) ? true : false;
        if (reqKeysNotEqual)
            return;
    }

    bool isBool = (strcmp(dataTypeName, "bool") == 0);
    bool isInt = (strcmp(dataTypeName, "int") == 0) ||
        (strcmp(dataTypeName, "DWORD") == 0);
    bool isString = (strcmp(dataTypeName, "debugString") == 0);
    if(isBool)
    {
        // ignore if there is no value
        if(vstring.size() == 1)
        {
            bool bval = (vstring.at(0) == '0') ? false : true;
            *((bool*)pRegkeyVar) = bval;
            isKeySet = true;
        }
    }
    else if(isInt)
    {
        // ignore if there is no value
        if(!vstring.empty())
        {
            // assume vstring has the valid value!
            int intval;
            if (vstring.find("0x", 0) == std::string::npos)
                intval = stoi(vstring, nullptr, 10);
            else
                intval = stoi(vstring, nullptr, 16);
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
            strcpy_s(s, sizeof(debugString), vstring.c_str());
            isKeySet = true;
        }
    }
}

static const std::string GetOptionFilePath()
{
#if defined(_WIN64) || defined(_WIN32)
    char path[256] = "c:\\Intel\\IGC\\debugFlags\\";
    std::string testFilename = std::string(path) + "testfile";
    HANDLE testFile =
       CreateFileA(testFilename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (testFile == INVALID_HANDLE_VALUE)
    {
        char temppath[256];
        if (GetTempPathA(sizeof(temppath), temppath) != 0)
        {
            sprintf_s(path, "%sIGC\\debugFlags\\", temppath);
        }
    }
    else
    {
        CloseHandle(testFile);
    }

    return path;
#else
    return "/tmp/IntelIGC/debugFlags/";
#endif
}

static const std::string GetOptionFile()
{
    std::string fname = "Options.txt";
    return (GetOptionFilePath() + fname);
}

static
std::optional<std::pair<HashRange::Type, llvm::StringRef>>
parseHashType(llvm::StringRef line)
{
    using namespace llvm;
    size_t Loc = line.find_first_of(':');
    if (Loc == StringRef::npos)
        return {};

    auto Ty = StringSwitch<std::optional<HashRange::Type>>(line.substr(0, Loc))
               .Cases("hash", "asmhash", HashRange::Type::Asm)
               .Case("psohash", HashRange::Type::Pso)
               .Default({});
    if (!Ty)
        return {};

    return { std::make_pair(*Ty, line.substr(Loc + 1)) };
}

static
std::optional<std::pair<bool, llvm::StringRef>>
detectEntryPoints(llvm::StringRef &line)
{
    using namespace llvm;
    size_t Loc = line.find_first_of(':');
    if (Loc == StringRef::npos)
        return {};
    auto key = line.substr(0, Loc);

    bool isEntryPoint = false;
    if (key == "entry_point") {
        isEntryPoint = true;
    }

    return { std::make_pair(isEntryPoint, line.substr(Loc + 1)) };
}

// parses this syntax:
// Each hash may be optionally prefixed with "0x" (e.g., 0xaaaaaaaaaaaaaaaa)
// hash:abcdabcdabcdabcd-ffffffffffffffff,aaaaaaaaaaaaaaaa
// asmhash:abcdabcdabcdabcd-ffffffffffffffff,aaaaaaaaaaaaaaaa
// psohash:abcdabcdabcdabcd-ffffffffffffffff,aaaaaaaaaaaaaaaa
static void ParseHashRange(llvm::StringRef line, std::vector<HashRange>& ranges)
{
    using namespace llvm;
    auto Result = parseHashType(line);
    if (!Result)
        return;
    auto parseAsInt = [](StringRef S) {
        unsigned Radix = S.startswith("0x") ? 0 : 16;
        uint64_t Result;
        bool Err = S.getAsInteger(Radix, Result);
        IGC_ASSERT(!Err);
        return Result;
    };
    // re-initialize ranges
    ranges.clear();
    auto [HashType, vString] = *Result;
    do {
        auto [token, RHS] = vString.split(',');
        size_t dash = token.find('-');
        StringRef start = token.substr(0, dash);
        StringRef end = (dash == StringRef::npos) ?
            start : token.substr(dash + 1);
        HashRange range = {};
        range.start = parseAsInt(start);
        range.end   = parseAsInt(end);
        range.Ty    = HashType;
        ranges.push_back(range);
        vString = RHS;
    } while (!vString.empty());
}

// parses this syntax:
// each seperated by comma
// entry_point:MicropolyRasterize, CS
static void ParseEntryPoint(llvm::StringRef line, std::vector<EntryPoint>& entry_points)
{
    auto Result = detectEntryPoints(line);
    if (!Result)
        return;
    auto [isEntryPoint, vString] = *Result;
    if (!isEntryPoint)
        return;
    // re-initialize entries
    entry_points.clear();
    do {
        auto [token, RHS] = vString.split(',');
        EntryPoint entry_point {};
        entry_point.entry_point_name = token.str();
        entry_points.push_back(entry_point);
        vString = RHS;
    } while (!vString.empty());
}

static void setIGCKeyOnHash(
    std::vector<HashRange>& hashes, const unsigned value,
    SRegKeyVariableMetaData* var)
{
    // hashes can be empty if the var is not set via Options.txt
    for (size_t i = 0; i < hashes.size(); i++)
        var->hashes.push_back(hashes[i]);
    var->Set();
    var->m_Value = value;
}

static void setIGCKeyOnEntryPoints(
    std::vector<EntryPoint>& entry_points, const unsigned value,
    SRegKeyVariableMetaData* var)
{
    // hashes can be empty if the var is not set via Options.txt
    var->entry_points.insert(var->entry_points.end(), entry_points.begin(), entry_points.end());
    var->Set();
    var->m_Value = value;
}

// Implicitly set the subkeys
static void setImpliedIGCKeys()
{
    IGC_SET_IMPLIED_REGKEY(DisableIGCOptimizations, 1, DisableLLVMGenericOptimizations, true);
    IGC_SET_IMPLIED_REGKEY(DisableIGCOptimizations, 1, DisableCodeSinking, true);
    IGC_SET_IMPLIED_REGKEY(DisableIGCOptimizations, 1, EnableDeSSA, false);
    //disable now until we figure out the issue
    //IGC_SET_IMPLIED_REGKEY(DisableIGCOptimizations, DisablePayloadCoalescing, true);
    IGC_SET_IMPLIED_REGKEY(DisableIGCOptimizations, 1, EnableVISANoSchedule, true);
    IGC_SET_IMPLIED_REGKEY(DisableIGCOptimizations, 1, DisableUniformAnalysis, true);
    IGC_SET_IMPLIED_REGKEY(DisableIGCOptimizations, 1, DisablePushConstant, true);
    IGC_SET_IMPLIED_REGKEY(DisableIGCOptimizations, 1, DisableConstantCoalescing, true);
    IGC_SET_IMPLIED_REGKEY(DisableIGCOptimizations, 1, DisableURBWriteMerge, true);
    IGC_SET_IMPLIED_REGKEY(DisableIGCOptimizations, 1, DisableCodeHoisting, true);
    IGC_SET_IMPLIED_REGKEY(DisableIGCOptimizations, 1, DisableEmptyBlockRemoval, true);
    IGC_SET_IMPLIED_REGKEY(DisableIGCOptimizations, 1, DisableSIMD32Slicing, true);
    IGC_SET_IMPLIED_REGKEY(DisableIGCOptimizations, 1, DisableCSEL, true);
    IGC_SET_IMPLIED_REGKEY(DisableIGCOptimizations, 1, DisableFlagOpt, true);
    IGC_SET_IMPLIED_REGKEY(DisableIGCOptimizations, 1, DisableScalarAtomics, true);

    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisablePayloadSinking, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisablePromoteToScratch, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableEarlyRemat, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableLateRemat, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableRTGlobalsKnownValues, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisablePreSplitOpts, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableInvariantLoad, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableRTStackOpts, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisablePrepareLoadsStores, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableRayTracingConstantCoalescing, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableMatchRegisterRegion, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableFuseContinuations, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableRaytracingIntrinsicAttributes, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableShaderFusion, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableExamineRayFlag, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableSpillReorder, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableCrossFillRemat, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisablePromoteContinuation, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableRTAliasAnalysis, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableRTFenceElision, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableRTMemDSE, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, RematThreshold, 0);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableDPSE, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableSWStackOffsetElision, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableInvalidateRTStackAfterLastRead, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableMergeAllocas, true);
    IGC_SET_IMPLIED_REGKEY(DisableRayTracingOptimizations, 1, DisableLoadAsFenceOpInRaytracing, true);

    IGC_SET_IMPLIED_REGKEY(ForceRTRetry, 1, RetryManagerFirstStateId, 1);

    IGC_SET_IMPLIED_REGKEY(DumpVISAASMToConsole, 1, EnableVISASlowpath, true);

    IGC_SET_IMPLIED_REGKEY(ShaderDumpEnableAll, 1, ShaderDumpEnable, true);
    IGC_SET_IMPLIED_REGKEY(ShaderDumpEnableAll, 1, EnableVISASlowpath, true);
    IGC_SET_IMPLIED_REGKEY(ShaderDumpEnable, 1, DumpLLVMIR, true);
    IGC_SET_IMPLIED_REGKEY(ShaderDumpEnable, 1, EnableCosDump, true);
    IGC_SET_IMPLIED_REGKEY(ShaderDumpEnable, 1, DumpOCLProgramInfo, true);
    IGC_SET_IMPLIED_REGKEY(ShaderDumpEnable, 1, EnableVISAOutput, true);
    IGC_SET_IMPLIED_REGKEY(ShaderDumpEnable, 1, EnableVISABinary, true);
    IGC_SET_IMPLIED_REGKEY(ShaderDumpEnable, 1, EnableVISADumpCommonISA, true);
    IGC_SET_IMPLIED_REGKEY(ShaderDumpEnable, 1, EnableCapsDump, true);
    IGC_SET_IMPLIED_REGKEY(ShaderDumpEnable, 1, DumpPatchTokens, true);
    IGC_SET_IMPLIED_REGKEY(ShaderDumpEnable, 1, RayTracingDumpYaml, true);

    IGC_SET_IMPLIED_REGKEY(DumpTimeStatsPerPass, 1, DumpTimeStats, true);
    IGC_SET_IMPLIED_REGKEY(DumpTimeStatsCoarse,  1, DumpTimeStats, true);
    if (IGC_IS_FLAG_ENABLED(DumpTimeStatsPerPass) ||
        IGC_IS_FLAG_ENABLED(DumpTimeStatsCoarse) ||
        IGC_IS_FLAG_ENABLED(DumpTimeStats))
    {
        // Need to turn on this setting so per-shader .csv is generated
        IGC::Debug::SetDebugFlag(IGC::Debug::DebugFlag::TIME_STATS_PER_SHADER, true);
    }

    IGC_SET_IMPLIED_REGKEY(ForceOCLSIMDWidth, 32, EnableOCLSIMD32, true);
    IGC_SET_IMPLIED_REGKEY(ForceOCLSIMDWidth, 32, EnableOCLSIMD16, false);
    IGC_SET_IMPLIED_REGKEY(ForceOCLSIMDWidth, 16, EnableOCLSIMD32, false);
    IGC_SET_IMPLIED_REGKEY(ForceOCLSIMDWidth, 16, EnableOCLSIMD16, true);
    IGC_SET_IMPLIED_REGKEY(ForceOCLSIMDWidth,  8, EnableOCLSIMD32, false);
    IGC_SET_IMPLIED_REGKEY(ForceOCLSIMDWidth,  8, EnableOCLSIMD16, false);
}

void setImpliedRegkey(SRegKeyVariableMetaData& name,
    const bool set,
    SRegKeyVariableMetaData& subname,
    const unsigned subvalue)
{
    if (set)
    {
        setIGCKeyOnHash(name.hashes, subvalue, &subname);
        setIGCKeyOnEntryPoints(name.entry_points, subvalue, &subname);
    }
}

static void declareIGCKey(
    const std::string& line, const char* dataType, const char* regkeyName,
    std::vector<HashRange>& hashes, std::vector<EntryPoint>& entry_points, SRegKeyVariableMetaData* regKey)
{
    bool isSet = false;
    debugString value = { 0 };
    setRegkeyFromOption(line, dataType, regkeyName, &value, isSet);
    if (isSet && !hashes.empty())
    {
        std::cout << std::endl << "** hashes ";
        for (size_t i = 0; i < hashes.size(); i++)
        {
            memcpy_s(hashes[i].m_string, sizeof(value), value, sizeof(value));
            regKey->hashes.push_back(hashes[i]);
            if (hashes[i].end == hashes[i].start)
                std::cout << std::hex << std::showbase << hashes[i].start << ", ";
            else
                std::cout << std::hex << std::showbase << hashes[i].start << "-" << hashes[i].end << ", ";
        }
        std::cout << std::endl;

        std::cout << "** regkey " << line << std::endl;
        regKey->Set();
        memcpy_s(regKey->m_string, sizeof(value), value, sizeof(value));
    }
    if (isSet && !entry_points.empty())
    {
        std::cout << std::endl << "** entry point ";
        for (auto entry_point : entry_points)
        {
            strcpy_s(entry_point.m_string,sizeof(value), value);
            regKey->entry_points.push_back(entry_point);
            std::cout << entry_point.entry_point_name << ", ";
        }
        std::cout << std::endl;

        std::cout << "** regkey " << line << std::endl;
        regKey->Set();
        memcpy_s(regKey->m_string, sizeof(value), value, sizeof(value));
    }
}

static void LoadDebugFlagsFromFile()
{
    std::ifstream input(GetOptionFile());
    std::string line;
    std::vector<HashRange> hashes;
    std::vector<EntryPoint> entry_points;

    if (input.is_open())
        std::cout << std::endl << "** DebugFlags " << GetOptionFile() << " is opened" << std::endl;

    while (std::getline(input, line)) {
        if (line.empty() || line.front() == '#')
            continue;
        ParseHashRange(line, hashes);
        ParseEntryPoint(line, entry_points);
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode)         \
{                                                                                   \
    declareIGCKey(line, #dataType, #regkeyName, hashes, entry_points, &(g_RegKeyList.regkeyName));\
}
#include "igc_regkeys.h"
DECLARE_IGC_REGKEY(bool, ShaderDumpEnable, false, "dump LLVM IR, visaasm, and GenISA", true)

#undef DECLARE_IGC_REGKEY

    }
    setImpliedIGCKeys();
}

static void LoadDebugFlagsFromString(const char* input)
{
    if (!input || input[0] == '\0') {
        return;
    }
    std::istringstream istream(input);
    std::string line;
    std::vector<HashRange> hashes;
    std::vector<EntryPoint>entry_points;

    std::cout << std::endl << "** DebugFlags " << input << " is applied" << std::endl;

    while (std::getline(istream, line, ';')) {
        if (line.empty())
            continue;
        ParseHashRange(line, hashes);
        ParseEntryPoint(line, entry_points);
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode)         \
{                                                                                   \
    declareIGCKey(line, #dataType, #regkeyName, hashes, entry_points, &(g_RegKeyList.regkeyName));\
}
#include "igc_regkeys.h"
#undef DECLARE_IGC_REGKEY

    }
    setImpliedIGCKeys();
}

void appendToOptionsLogFile(std::string const &message)
{
    std::string logPath = GetOptionFilePath();
    logPath.append("Options_log.txt");
    std::ofstream os(logPath.c_str(), std::ios::app);
    if (os.is_open())
    {
        auto now = std::chrono::system_clock::now();
        auto now_time = std::chrono::system_clock::to_time_t(now);
        std::string stime = ctime(&now_time);
        std::replace(stime.begin(), stime.end(), '\n', '\t');
        os << stime << message << std::endl;
    }
    os.close();
}

static thread_local std::vector<std::string> g_CurrentEntryPointNames;

static thread_local ShaderHash g_CurrentShaderHash;
void SetCurrentDebugHash(const ShaderHash& hash)
{
    g_CurrentShaderHash = hash;
}

// ray tracing shader can have multiple access point
void SetCurrentEntryPoints(const std::vector<std::string>& entry_points)
{
    g_CurrentEntryPointNames = entry_points;
}

void ClearCurrentEntryPoints()
{
    g_CurrentEntryPointNames.clear();
}


bool CheckHashRange(SRegKeyVariableMetaData& varname)
{
    if (varname.hashes.empty()) {
        if (varname.entry_points.empty()) {
            return true;
        }
        else {
            return false;
        }
    }
    if (!g_CurrentShaderHash.is_set())
    {
        std::string msg = "Warning: hash not calculated yet; IGC_GET_FLAG_VALUE(" + std::string(varname.GetName()) + ") returned default value";
        appendToOptionsLogFile(msg);
    }

    for (auto &it : varname.hashes)
    {
        unsigned long long CurrHash = it.getHashVal(g_CurrentShaderHash);
        if (CurrHash >= it.start && CurrHash <= it.end)
        {
            varname.m_Value = it.m_Value;
            constexpr uint32_t Len = 100;
            char msg[Len];
            int size = snprintf(msg, Len, "Shader %#0llx: %s=%d", CurrHash, varname.GetName(), it.m_Value);
            if (size >= 0 && size < Len)
            {
                appendToOptionsLogFile(msg);
            }

            return true;
        }
    }
    return false;
}

bool CheckEntryPoint(SRegKeyVariableMetaData& varname)
{
    //return false;
    if (varname.entry_points.empty())
    {
        if (varname.hashes.empty())
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    if (g_CurrentEntryPointNames.size() == 0)
    {
        std::string msg = "Warning: entry point not set yet; IGC_GET_FLAG_VALUE(" + std::string(varname.GetName()) + ") returned default value";
        appendToOptionsLogFile(msg);
    }

    {
        //looping entry point recorded by regkey metadata
        for (auto& it : varname.entry_points)
        {
            //looping each entry point of current shader
            for (std::string& CurrEntryPoint : g_CurrentEntryPointNames)
            {
                if (CurrEntryPoint == it.entry_point_name)
                {
                    varname.m_Value = it.m_Value;
                    return true;
                }
            }
        }
    }
    return false;
}

/*****************************************************************************\

Function:
    GetRegistryKeyValue

Description:
    Reads a registry variable from the registry

Input:
    key
    buffer
    bufferSize

Output:
    read status

\*****************************************************************************/
extern "C" bool IGC_DEBUG_API_CALL GetRegistryKeyValue(const char* key, void* buffer, uint32_t bufferSize)
{
    bool isSet = ReadIGCRegistry(
        key,
        buffer,
        bufferSize);
    return isSet;
}

static void LoadFromRegKeyOrEnvVarOrOptions(
    const std::string& options = "",
    bool* RegFlagNameError = nullptr)
{
    SRegKeyVariableMetaData* pRegKeyVariable = (SRegKeyVariableMetaData*)&g_RegKeyList;
    constexpr unsigned NUM_REGKEY_ENTRIES =
        sizeof(SRegKeysList) / sizeof(SRegKeyVariableMetaData);
    for (DWORD i = 0; i < NUM_REGKEY_ENTRIES; i++)
    {
        debugString value = { 0 };
        const char* name = pRegKeyVariable[i].GetName();
        std::string nameWithEqual = name;
        nameWithEqual = nameWithEqual + "=";

        bool isSet = GetRegistryKeyValue(
            name,
            &value,
            sizeof(value));

        if (isSet)
        {
            memcpy_s(pRegKeyVariable[i].m_string, sizeof(value), value, sizeof(value));

            pRegKeyVariable[i].Set();
            checkAndSetIfKeyHasNoDefaultValue(&pRegKeyVariable[i]);
        }

        debugString valueFromOptions = { 0 };
        std::size_t found = options.find(nameWithEqual);

        if (found != std::string::npos)
        {
            std::size_t foundComma = options.find(',', found);
            if (foundComma != std::string::npos)
            {
                if (found == 0 || options[found - 1] == ' ' || options[found - 1] == ',')
                {
                    std::string token = options.substr(found + nameWithEqual.size(), foundComma - (found + nameWithEqual.size()));
                    unsigned int size = sizeof(value);
                    void* pValueFromOptions = &valueFromOptions;

                    const char* envValFromOptions = token.c_str();
                    bool valueIsInt = false;
                    if (envValFromOptions != NULL)
                    {
                        if (size >= sizeof(unsigned int))
                        {
                            // Try integer conversion
                            char* pStopped = nullptr;
                            unsigned int* puValFromOptions = (unsigned int*)pValueFromOptions;
                            *puValFromOptions = strtoul(envValFromOptions, &pStopped, 0);
                            if (pStopped == envValFromOptions + std::strlen(envValFromOptions))
                            {
                                valueIsInt = true;
                            }
                        }
                        if (!valueIsInt)
                        {
                            // Just return the string
                            strncpy_s((char*)pValueFromOptions, size, envValFromOptions, size);
                        }
                    }
                    memcpy_s(pRegKeyVariable[i].m_string, sizeof(valueFromOptions), valueFromOptions, sizeof(valueFromOptions));
                    pRegKeyVariable[i].Set();
                    checkAndSetIfKeyHasNoDefaultValue(&pRegKeyVariable[i]);
                }
                else if (found > 0 && options[found - 1] != ' ' && options[found - 1] != ',')
                {
                    // some keywords are a substring of another keywords
                    // for example, "ControlKernelTotalSize" and "PrintControlKernelTotalSize"
                    // so we need to check it and skip executing the keyword if it is not the expected one
                    continue;
                }
                else if(RegFlagNameError != nullptr)
                {
                    *RegFlagNameError = true;
                }
            }
        }
    }
    if (IGC_IS_FLAG_ENABLED(PrintDebugSettings))
    {
        // Using std::cout caused crashes in SYCL environment so we use fprintf instead as a work around.
        std::ostringstream debugOutputStream;

        for (DWORD i = 0; i < NUM_REGKEY_ENTRIES; i++)
        {
            if (pRegKeyVariable[i].m_isSetToNonDefaultValue)
                debugOutputStream << pRegKeyVariable[i].GetName() << " " << pRegKeyVariable[i].m_Value << '\n';
        }

        fprintf(stdout, "%s", debugOutputStream.str().c_str());
    }
}

/*****************************************************************************\

Function:
    InitializeRegKeys

Description:
    Initialize global-scoped registry variables

Input:
    None

Output:
    None

\*****************************************************************************/
void InitializeRegKeys()
{
    static std::mutex loadFlags;
    static volatile bool flagsSet = false;
    std::lock_guard<std::mutex> lock(loadFlags);

    if (!flagsSet)
    {
        flagsSet = true;
        setImpliedIGCKeys();

#if !defined(_DEBUG)
        if (IGC_IS_FLAG_ENABLED(EnableDebugging))
#endif
        {
            //DumpIGCRegistryKeyDefinitions();
            LoadDebugFlagsFromFile();
            LoadDebugFlagsFromString(IGC_GET_REGKEYSTRING(SelectiveHashOptions));
        }
        if (IGC_IS_FLAG_ENABLED(LLVMCommandLine))
        {
            std::vector<char*> args;
            args.push_back((char*)("IGC"));
            ParseCStringVector(args, IGC_GET_REGKEYSTRING(LLVMCommandLine));
            llvm::cl::ParseCommandLineOptions(args.size(), &args[0]);
        }

        setImpliedIGCKeys();
    }
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
void LoadRegistryKeys(const std::string& options, bool *RegFlagNameError)
{
    // only load the debug flags once before compiling to avoid any multi-threading issue
    static std::mutex loadFlags;
    static volatile bool flagsSet = false;
    std::lock_guard<std::mutex> lock(loadFlags);

    if(!flagsSet)
    {
        flagsSet = true;
        LoadFromRegKeyOrEnvVarOrOptions(options, RegFlagNameError);
        InitializeRegKeys();
    }
}

// Get all keys that have been set explicitly with a non-default value. Return
// all of them via arguments:
//     KeyValuePairs:
//          key0   value0
//          key1   value1
//          ......
//     OptionsKeys:
//          key0=value0,key1=value1,...
// Note OptionsKeys is used in igcstandalone's -option.
void GetKeysSetExplicitly(std::string* KeyValuePairs, std::string* OptionKeys)
{
    if (!KeyValuePairs && !OptionKeys)
        return;

    std::stringstream pairs;
    std::stringstream optionkeys;
    SRegKeyVariableMetaData* pRegKeyVariable = (SRegKeyVariableMetaData*)&g_RegKeyList;
    unsigned NUM_REGKEY_ENTRIES = sizeof(SRegKeysList) / sizeof(SRegKeyVariableMetaData);
    bool isFirst = true;
    for (DWORD i = 0; i < NUM_REGKEY_ENTRIES; i++)
    {
        if (pRegKeyVariable[i].m_isSetToNonDefaultValue == false)
        {
            continue;
        }
        const char* key = pRegKeyVariable[i].GetName();

        // Ignore some dump keys
        if (strcmp("ShaderDumpEnableAll", key) == 0 ||
            strcmp("ShaderDumpEnable", key) == 0 ||
            strcmp("ShaderDumpRegexFilter", key) == 0 ||
            strcmp("DumpToCurrentDir", key) == 0 ||
            strcmp("EnableCosDump", key) == 0 ||
            strcmp("DumpToCustomDir", key) == 0 ||
            strcmp("EnableDxbcDump", key) == 0 ||
            strcmp("EnableDxAsmDump", key) == 0)
        {
            continue;
        }

        unsigned value = pRegKeyVariable[i].m_Value;
        pairs << "    " << key << "    " << value << "\n";
        if (!isFirst)
        {
            optionkeys << ",";
        }
        optionkeys << key << "=" << value;
        isFirst = false;
    }

    if (isFirst)
    {
        // No key set
        return;
    }

    if (KeyValuePairs)
    {
        *KeyValuePairs = pairs.str();
    }
    if (OptionKeys)
    {
        *OptionKeys = optionkeys.str();
    }
}
#endif
