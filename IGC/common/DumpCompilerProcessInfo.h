/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// @brief Dump process info writes certain process and environment information.

#pragma once

#ifdef _WIN32
#include <processenv.h>
#endif // _WIN32

#include <thread>
#include <mutex>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <fstream>

#include "common/SysUtils.hpp"

#if defined(_WIN32) || defined(__CYGWIN__) || defined(_WIN64)
    #include "VulkanFrontend/OS/Windows/SpirVos.h"
#elif defined(__linux__)
    #include "VulkanFrontend/OS/Linux/SpirVos.h"
#endif // windows

#include "VulkanFrontend/Common/macros.h"

namespace spv
{
//////////////////////////////////////////////////////////////////////////
/// @brief  Writes certain information about the process and the environment
struct DumpProcessInfo
{
    DELETE_COPY_MOVE(DumpProcessInfo);

    //////////////////////////////////////////////////////////////////////////
    /// @brief  Ctor - open the process info file for appending and dump prolog
    DumpProcessInfo(const std::string& filePath) :
        m_ProcessInfoFilePath(filePath)
    {
        std::ofstream processInfoFile(m_ProcessInfoFilePath, std::ios_base::app);

        if (processInfoFile)
        {
            std::string summary;

            // IGC library information
            summary += std::string("IGC library: ")
                + ((sizeof(void*) == 8) ? "64" : "32") + "-bit ";
#if defined(_DEBUG)
            summary += "Debug";
#elif defined(_INTERNAL)
            summary += "Release Internal";
#else
            summary += "Release";
#endif
            summary += "\n\n";

            // Driver build version
            summary += std::string("Build version: ");
            summary += IGC::Debug::GetVersionInfo();
            summary += "\n\n";

            // llvm build version
            summary += std::string("LLVM major version: ");
            summary += std::to_string(LLVM_VERSION_MAJOR);
            summary += "\n\n";

            // OS information
            summary += "OS: " + GetOSName() + "\n\n";

            // Process information
            summary += std::string("Current working directory: ")
                + getCurrentLocation() + "\n\n";
            summary += std::string("Path to executable: ")
                + IGC::SysUtils::GetProcessName() + "\n\n";
#ifdef _WIN32
            LPSTR commandLine = ::GetCommandLineA();
            IGC_ASSERT(commandLine);
            summary += std::string("Command line: ")
                + (commandLine ? commandLine : "") + "\n\n";
            summary += "Module list:\n";
            UpdateLoadedModuleList();
            summary += m_CompilationTimeModuleList;
            summary += "\n";
#endif

            // Registry and environment
#ifdef _WIN32
            auto MergeMaps = [](const Registry::RegFileContent& in, Registry::RegFileContent& out)
            {
                out.insert(in.begin(), in.end());
            };

            Registry::RegFileContent content;

            MergeMaps(Registry::ContentToRegFile(
                Registry::Read(HKEY_LOCAL_MACHINE, "SOFTWARE\\Intel\\Display\\CUICOM", true)),
                content);
            MergeMaps(Registry::ContentToRegFile(
                Registry::Read(HKEY_LOCAL_MACHINE, "SOFTWARE\\Intel\\IGFX", true)),
                content);
            MergeMaps(Registry::ContentToRegFile(
                Registry::Read(HKEY_LOCAL_MACHINE, "SOFTWARE\\Intel\\KMD", true, true)),
                content);
            MergeMaps(Registry::ContentToRegFile(
                Registry::Read(
                    HKEY_LOCAL_MACHINE,
                    "SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler", false)),
                content);
            MergeMaps(Registry::ContentToRegFile(
                Registry::Read(
                    HKEY_LOCAL_MACHINE,
                    "SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers",
                    std::string("TdrLevel"))),
                content);
            MergeMaps(Registry::ContentToRegFile(
                Registry::Read(
                    HKEY_LOCAL_MACHINE,
                    "SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers",
                    std::string("TdrDebugMode"))),
                content);
            MergeMaps(Registry::ContentToRegFile(
                Registry::Read(
                    HKEY_LOCAL_MACHINE,
                    "SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers",
                    std::string("TdrDelay"))),
                content);
            MergeMaps(Registry::ContentToRegFile(
                Registry::Read(
                    HKEY_LOCAL_MACHINE,
                    "SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers",
                    std::string("TdrDdiDelay"))),
                content);

            summary += "Registry:\n";
            for (const auto& key : content)
            {
                summary += key.first + "\n";
                for (const auto& value : content[key.first])
                {
                    summary += value + "\n";
                }
            }
#endif

            VariableMap variableMap;
            updateVariableMap(variableMap, { "IGC_", "VK_", "KNOB_", "RASTY_", "INTEL", "MESA", "VULKAN" },
                true /* isKeyPrefix */, false /* isCaseSensitive */);
            summary += "\nEnvironment:\n";
            for (const auto& pair : variableMap)
            {
                summary += pair.first + "=" + pair.second + "\n";
            }

            processInfoFile
                << "--- PID " << ::IGC::SysUtils::GetProcessId() << ": "
                << "IGC library has been loaded at " << getDateTime() << " ---" << std::endl
                << summary << std::endl << std::endl;
        }
        else
        {
            IGC_ASSERT_MESSAGE(false, "Unable to open file for appending.");
        }
    }

    //////////////////////////////////////////////////////////////////////////
    /// @brief  Dtor - open the process info file for appending and dump epilog
    ~DumpProcessInfo()
    {
        std::ofstream processInfoFile(m_ProcessInfoFilePath, std::ios_base::app);

        if (processInfoFile)
        {
            std::string summary;

#ifdef _WIN32
            // This list is updated at compilation time
            summary += "Module list:\n";
            summary += m_CompilationTimeModuleList;
            summary += "\n";
#endif
            summary += std::string("Compilation threads used: ") + std::to_string(m_ThreadMap.size()) + "\n";

            processInfoFile
                << "--- PID " << ::IGC::SysUtils::GetProcessId() << ": "
                << "IGC library is about to be unloaded at " << getDateTime() << " ---" << std::endl
                << summary << std::endl << std::endl;
        }
        else
        {
            IGC_ASSERT_MESSAGE(false, "Unable to open file for appending.");
        }
    }

    //////////////////////////////////////////////////////////////////////////
    /// @brief  To be called at compilation time; update certain loaded module list
    void UpdateLoadedModuleList()
    {
        const auto threadId = std::this_thread::get_id();
        if (m_ThreadMap.find(threadId) == m_ThreadMap.end())
        {
            const auto count = m_ThreadMap.size();
            m_ThreadMap[threadId] = count;
        }
#ifdef _WIN32
        static std::once_flag initFlag;
        std::call_once(initFlag, [&](){
            m_CompilationTimeModuleList.clear();
            m_CompilationTimeModuleList += std::string("Path to IGC library: ")
                + GetLoadedModulePath(m_igcDllName) + "\n";
            m_CompilationTimeModuleList += std::string("Path to Vulkan UMD library: ")
                + GetLoadedModulePath(m_vkUmdDllName) + "\n";
            m_CompilationTimeModuleList += std::string("Path to Vulkan library: ")
                + GetLoadedModulePath(m_vulkanDllName) + "\n";
            });
#endif
    }

    //////////////////////////////////////////////////////////////////////////
    /// @brief  Add a compilation thread id to the set
    void UpdateThreadSet(size_t counter)
    {
        m_ThreadCounter = counter;
    }

private: // types
    typedef std::map<std::string, std::string> VariableMap;

private: // data
    std::string m_ProcessInfoFilePath;

    std::string m_CompilationTimeModuleList;
    size_t m_ThreadCounter = 0;

#ifdef _WIN32
#ifdef _WIN64
    const char* const m_igcDllName = "igc64.dll";
    const char* const m_vkUmdDllName = "igvk64.dll";
#else
    const char* const m_igcDllName = "igc32.dll";
    const char* const m_vkUmdDllName = "igvk32.dll";
#endif
    const char* const m_vulkanDllName = "vulkan-1.dll";
#endif

    //////////////////////////////////////////////////////////////////////////
    /// @brief  Find a variable by name (or name's prefix) and, if present,
    ///         add it to the map.
    void updateVariableMap(VariableMap& map, std::initializer_list<const std::string> keys, bool isKeyPrefix, bool isCaseSensitive)
    {
        auto toLower = [](std::string str)
        {
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
            return str;
        };

        const auto environment = GetEnvironment();
        for (auto key : keys)
        {
            key = isCaseSensitive ? key : toLower(key);

            for (const auto& envVar : environment)
            {
                const std::string envVarName = isCaseSensitive ? envVar.first : toLower(envVar.first);
                if (envVarName.find(key) == 0)
                {
                    if (isKeyPrefix || envVarName.size() == key.size())
                    {
                        map[envVar.first] = envVar.second;
                    }
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    /// @brief  Get current date and time as a string.
    std::string getDateTime()
    {
        // ctime() is not thread-safe
        static std::mutex m;
        std::lock_guard<std::mutex> lck(m);

        const auto now = std::chrono::system_clock::now();
        const auto now_time = std::chrono::system_clock::to_time_t(now);
        const char* const timeStr = std::ctime(&now_time);
        if (timeStr && strnlen_s(timeStr, INT_MAX) > 0)
        {
            return std::string(timeStr, timeStr + strnlen_s(timeStr, INT_MAX) - 1);
        }
        IGC_ASSERT(0);
        return std::string();
    }

    // Thread id to ordinal number map
    std::map<std::thread::id, size_t> m_ThreadMap;
};

}
