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

/*****************************************************************************\
STRUCT: SRegKeyVariableMetaData
PURPOSE: Defines meta data for holding/defining regkey variables
\*****************************************************************************/
#pragma once
#include "IGC/common/igc_debug.h"
#include "IGC/common/igc_flags.hpp"
#include "common/SysUtils.hpp"
#include <string>

typedef char debugString[256];

#if defined( _DEBUG ) || defined( _INTERNAL )
#define IGC_DEBUG_VARIABLES
#endif

#if defined( __linux__ )
#define IGC_DEBUG_VARIABLES
#endif


#if defined(IGC_DEBUG_VARIABLES)
#include <vector>
struct HashRange
{
    unsigned long long start;
    unsigned long long end;
};

struct SRegKeyVariableMetaData
{
    union
    {
        unsigned    m_Value;
        debugString m_string;
    };
    std::vector<HashRange> hashes;
    virtual const char* GetName() const = 0;
    virtual ~SRegKeyVariableMetaData()
    {

    }
};

#if defined ( __linux__ ) && !defined( _DEBUG ) && !defined( _INTERNAL )
#define LINUX_RELEASE_MODE
#endif

/*****************************************************************************\
MACRO: IGC_REGKEY
PURPOSE: Declares a new regkey variable.
\*****************************************************************************/
#define IGC_REGKEY( dataType, regkeyName, defaultValue, description, releaseMode )    \
struct SRegKeyVariableMetaData_##regkeyName : public SRegKeyVariableMetaData \
{                                                   \
    SRegKeyVariableMetaData_##regkeyName()          \
    {                                               \
        m_Value = (unsigned)defaultValue;           \
    }                                               \
    const char* GetName() const                     \
    {                                               \
        return #regkeyName;                         \
    }                                               \
    unsigned GetDefault() const                     \
    {                                               \
        return (unsigned)defaultValue;              \
    }                                               \
    bool IsReleaseMode() const                      \
    {                                               \
        return releaseMode;                  \
    }                                               \
} regkeyName

// XMACRO defining the regkeys
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode) \
    IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode);
struct SRegKeysList
{
#include "igc_regkeys.def"
};
#undef DECLARE_IGC_REGKEY
bool CheckHashRange(const std::vector<HashRange>&);
extern SRegKeysList g_RegKeyList;
#if defined(LINUX_RELEASE_MODE)
#define IGC_GET_FLAG_VALUE( name )                 \
( ( CheckHashRange(g_RegKeyList.name.hashes) && g_RegKeyList.name.IsReleaseMode() ) ? g_RegKeyList.name.m_Value : g_RegKeyList.name.GetDefault())
#define IGC_IS_FLAG_ENABLED( name )                ( IGC_GET_FLAG_VALUE(name) != 0 )
#define IGC_IS_FLAG_DISABLED( name )               ( !IGC_IS_FLAG_ENABLED(name) )
#define IGC_SET_FLAG_VALUE( name, regkeyValue )    ( g_RegKeyList.name.m_Value = regkeyValue )
#define IGC_GET_REGKEYSTRING( name )               \
( ( CheckHashRange(g_RegKeyList.name.hashes) && g_RegKeyList.name.IsReleaseMode() ) ? g_RegKeyList.name.m_string : "" )
#else
#define IGC_GET_FLAG_VALUE( name )                 \
( CheckHashRange(g_RegKeyList.name.hashes) ? g_RegKeyList.name.m_Value : g_RegKeyList.name.GetDefault())
#define IGC_IS_FLAG_ENABLED( name )                ( IGC_GET_FLAG_VALUE(name) != 0 )
#define IGC_IS_FLAG_DISABLED( name )               ( !IGC_IS_FLAG_ENABLED(name) )
#define IGC_SET_FLAG_VALUE( name, regkeyValue )    ( g_RegKeyList.name.m_Value = regkeyValue )
#define IGC_GET_REGKEYSTRING( name )               \
( CheckHashRange(g_RegKeyList.name.hashes) ? g_RegKeyList.name.m_string : "" )
#endif

#define IGC_REGKEY_OR_FLAG_ENABLED( name, flag ) ( IGC_IS_FLAG_ENABLED(name) || IGC::Debug::GetDebugFlag(IGC::Debug::DebugFlag::flag))

#if defined(_WIN64) || defined(_WIN32)
struct DEVICE_INFO
    {
        std::string description;
        DWORD deviceID;
        DWORD revisionID;
        DWORD pciBus;
        DWORD pciDevice;
        DWORD pciFunction;
        std::string driverRegistryPath;

        DEVICE_INFO(DEVINST deviceInstance);

        void get_device_property(DEVINST deviceInstace, DWORD property);
    };
#endif

void DumpIGCRegistryKeyDefinitions();
void DumpIGCRegistryKeyDefinitions3(std::string driverRegistryPath, unsigned long pciBus, unsigned long pciDevice, unsigned long pciFunction);
void LoadRegistryKeys(const std::string& options = "", bool *RegFlagNameError = nullptr);
void SetCurrentDebugHash(unsigned long long hash);
#undef LINUX_RELEASE_MODE
#else
static inline void SetCurrentDebugHash(unsigned long long hash) {}
static inline void LoadRegistryKeys(const std::string& options = "", bool *RegFlagNameError=nullptr) {}
#define IGC_SET_FLAG_VALUE( name, regkeyValue ) ;
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode) \
    static const unsigned int regkeyName##default = (unsigned int)defaultValue;
namespace IGC
{
    class DebugVariable
    {
    public:
#include "igc_regkeys.def"
    };
};
#undef DECLARE_IGC_REGKEY

#define IGC_IS_FLAG_ENABLED( name )     (IGC::DebugVariable::name##default != 0)
#define IGC_IS_FLAG_DISABLED( name )    (IGC::DebugVariable::name##default == 0)
#define IGC_GET_FLAG_VALUE( name )      (IGC::DebugVariable::name##default)
#define IGC_GET_REGKEYSTRING( name )    ("")
#define IGC_REGKEY_OR_FLAG_ENABLED( name, flag ) ( IGC_IS_FLAG_ENABLED(name) || IGC::Debug::GetDebugFlag(IGC::Debug::DebugFlag::flag))
#endif
