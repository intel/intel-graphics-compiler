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
#pragma once

#if defined( _INTERNAL) || defined( _DEBUG )
    #define COMPILER_STATS_ENABLE 1
#endif

#include <cassert>
#include <unordered_map>
#include <map>
#include <sstream>
#include <memory>

class CompilerStats {
    // Internal class for storing data. Every statistic knows its type and keeps values for
    // each simd size (potenatially wastes memory but improves execution time).
    struct Statistic
    {
        // Supported data types:
        enum Type
        {
            type_bool,
            type_int64,
            type_double
        };

        // Supported simd sizes:
        enum SimdType
        {
            SIMD_GENERIC = 0,
            SIMD_8 = 1,
            SIMD_16 = 2,
            SIMD_32 = 3,
            NUM_SIMD_TYPES
        };

        // Store every type as 64-bit value.
        union Value
        {
            uint64_t    value = 0;
            bool        bool_value;
            int64_t     int64_value;
            double      double_value;
        };

        // Helper function to convert from SimdType enum to integer represenation (8/16/32). Returns 0 for generic simd.
        inline static int type_to_simd(SimdType simd_type);

        // Helper function to convert from integer;
        inline static SimdType simd_to_type(int simd);

        // Get stat value for simd size expressed as integer. Pass 0 to access generic.
        inline Value& operator[](int simd);
        inline const Value& operator[](int simd) const;

        // Members:
        Type    type;
        Value   values[NUM_SIMD_TYPES];
    };

#if COMPILER_STATS_ENABLE
    // Create empty statistic if does not exist, return existing if it does.
    inline Statistic& PrivateInit(const std::string& name, Statistic::Type type, int simd);

    // Try to find statistic. Return empty one if it does not exist.
    inline const Statistic& PrivateGet(const std::string& name, int simd) const;

    inline bool IsEnabled() const { return m_pState != nullptr; }

    struct State {
        std::unordered_map<std::string, Statistic> m_Stats;
        bool m_CollectOnlyInitialized = true;
    };

    std::shared_ptr<State> m_pState;
    Statistic m_ZeroStat;
#endif // COMPILER_STATS_ENABLE

public:
    static constexpr Statistic::Type type_bool     = Statistic::Type::type_bool;
    static constexpr Statistic::Type type_int64    = Statistic::Type::type_int64;
    static constexpr Statistic::Type type_double   = Statistic::Type::type_double;

    // Statistic collection is disabled by default.
    inline void Enable(bool collectOnlyInitialized);

    // Link statistics to the one from the provided object.
    inline void Link(CompilerStats& sharedData);

    // Merge the given Compiler Stats
    inline void MergeStats(CompilerStats& from, int simd);

    // Create statistic with value set to zero/false.
    inline void Init(const std::string& name, Statistic::Type type, int simd=0);

    // Set value to zero/false.
    inline void Reset(const std::string& name, int simd=0);

    // Set flag statistic to true.
    inline void SetFlag(const std::string& name, int simd=0);

    // Set value of integer statistic.
    inline void SetI64(const std::string& name, int64_t value, int simd=0);

    // Set value of floating point statistic.
    inline void SetF64(const std::string& name, double value, int simd=0);

    // Increase value of integer statistic.
    inline void IncreaseI64(const std::string& name, int64_t value, int simd=0);

    // Increase value of floating point statistic.
    inline void IncreaseF64(const std::string& name, double value, int simd=0);

    // Get value of a flag. Returns false if not found.
    inline bool GetFlag(const std::string& name, int simd=0) const;

    // Get value of an integer. Returns 0 if not found.
    inline int64_t GetI64(const std::string& name, int simd=0) const;

    // Get value of a floating point. Returns 0.0 if not found.
    inline double GetF64(const std::string& name, int simd=0) const;

    // Dump stats to csv format string.
    inline std::string ToCsv() const;
};

// Enable collection of statistics.
// collectOnlyInitialized - if true, statistic initialization is required, otherwise statistic update will be ignored
void CompilerStats::Enable(bool collectOnlyInitialized)
{
#if COMPILER_STATS_ENABLE
    if (!IsEnabled())
    {
        m_pState = std::make_shared<State>();
        m_pState->m_CollectOnlyInitialized = collectOnlyInitialized;
    }
#endif // COMPILER_STATS_ENABLE
}

// Link statistics to the one from the provided object.
void CompilerStats::Link(CompilerStats& sharedData)
{
#if COMPILER_STATS_ENABLE
    if (m_pState != nullptr)
    {
        m_pState.reset();
    }
    m_pState = sharedData.m_pState;
#endif // COMPILER_STATS_ENABLE
}

void CompilerStats::MergeStats(CompilerStats& from, int simd)
{
#if COMPILER_STATS_ENABLE
    for (auto elem : from.m_pState->m_Stats)
    {
        auto name = elem.first;
        auto type = elem.second.type;
        auto f = m_pState->m_Stats.find(name);
        if (f == m_pState->m_Stats.end())
        {
            Statistic& s = PrivateInit(name, type, simd);
            s[simd].value = elem.second[simd].value;
        }
        else
        {
            assert(type == f->second.type);
            // If flag, do OR rather than overwrite. This is because we want
            // to preserve the bool stats from an earlier try at this simd.
            if (type == Statistic::type_bool)
            {
                f->second[simd].value |= elem.second[simd].value;
            }
            else
            {
                f->second[simd].value = elem.second[simd].value;
            }
        }
    }
#endif // COMPILER_STATS_ENABLE
}

// Create statistic with value set to zero/false.
void CompilerStats::Init(const std::string& name, Statistic::Type type, int simd)
{
#if COMPILER_STATS_ENABLE
    if (IsEnabled())
    {
        auto f = m_pState->m_Stats.find(name);
        if(f == m_pState->m_Stats.end())
        {
            PrivateInit(name, type, simd);
        }
    }
#endif // COMPILER_STATS_ENABLE
}

// Set value to zero/false.
void CompilerStats::Reset(const std::string& name, int simd)
{
#if COMPILER_STATS_ENABLE
    if (IsEnabled())
    {
        auto f = m_pState->m_Stats.find(name);
        if(f != m_pState->m_Stats.end())
        {
            f->second[simd].value = 0;
        }
    }
#endif // COMPILER_STATS_ENABLE
}

// Set flag statistic to true.
// If statistic does not exist, create new one.
void CompilerStats::SetFlag(const std::string& name, int simd)
{
#if COMPILER_STATS_ENABLE
    if (IsEnabled())
    {
        auto f = m_pState->m_Stats.find(name);
        if (f == m_pState->m_Stats.end())
        {
            if (!m_pState->m_CollectOnlyInitialized)
            {
                Statistic& s = PrivateInit(name, Statistic::type_bool, simd);
                s[simd].bool_value = true;
            }
        }
        else
        {
            Statistic& s = f->second;
            assert(s.type == Statistic::type_bool);
            s[simd].bool_value = true;
        }
    }
#endif // COMPILER_STATS_ENABLE
}

// Set value of integer statistic.
// If statistic does not exist, create new one.
void CompilerStats::SetI64(const std::string& name, int64_t value, int simd)
{
#if COMPILER_STATS_ENABLE
    if (IsEnabled())
    {
        auto f = m_pState->m_Stats.find(name);
        if (f == m_pState->m_Stats.end())
        {
            if (!m_pState->m_CollectOnlyInitialized)
            {
                Statistic& s = PrivateInit(name, Statistic::type_int64, simd);
                s[simd].int64_value = value;
            }
        }
        else
        {
            Statistic& s = f->second;
            assert(s.type == Statistic::type_int64);
            s[simd].int64_value = value;
        }
    }
#endif // COMPILER_STATS_ENABLE
}

// Set value of floating point statistic.
// If statistic does not exist, create new one.
void CompilerStats::SetF64(const std::string& name, double value, int simd)
{
#if COMPILER_STATS_ENABLE
    if (IsEnabled())
    {
        auto f = m_pState->m_Stats.find(name);
        if (f == m_pState->m_Stats.end())
        {
            if (!m_pState->m_CollectOnlyInitialized)
            {
                Statistic& s = PrivateInit(name, Statistic::type_double, simd);
                s[simd].double_value = value;
            }
        }
        else
        {
            Statistic& s = f->second;
            assert(s.type == Statistic::type_double);
            s[simd].double_value = value;
        }
    }
#endif // COMPILER_STATS_ENABLE
}

// Increase value of integer statistic.
// If statistic does not exist, create new one.
void CompilerStats::IncreaseI64(const std::string& name, int64_t value, int simd)
{
#if COMPILER_STATS_ENABLE
    if (IsEnabled())
    {
        auto f = m_pState->m_Stats.find(name);
        if (f == m_pState->m_Stats.end())
        {
            if (!m_pState->m_CollectOnlyInitialized)
            {
                Statistic& s = PrivateInit(name, Statistic::type_int64, simd);
                s[simd].int64_value += value;
            }
        }
        else
        {
            Statistic& s = f->second;
            assert(s.type == Statistic::type_int64);
            s[simd].int64_value += value;
        }
    }
#endif // COMPILER_STATS_ENABLE
}

// Increase value of floating point statistic.
// If statistic does not exist, create new one.
void CompilerStats::IncreaseF64(const std::string& name, double value, int simd)
{
#if COMPILER_STATS_ENABLE
    if (IsEnabled())
    {
        auto f = m_pState->m_Stats.find(name);
        if (f == m_pState->m_Stats.end())
        {
            if (!m_pState->m_CollectOnlyInitialized)
            {
                Statistic& s = PrivateInit(name, Statistic::type_double, simd);
                s[simd].double_value += value;
            }
        }
        else
        {
            Statistic& s = f->second;
            assert(s.type == Statistic::type_double);
            s[simd].double_value += value;
        }
    }
#endif // COMPILER_STATS_ENABLE
}

// Get value of a flag. Returns false if not found.
bool CompilerStats::GetFlag(const std::string& name, int simd) const
{
#if COMPILER_STATS_ENABLE
    return PrivateGet(name, simd)[simd].bool_value;
#else
    return false;
#endif // COMPILER_STATS_ENABLE
}

// Get value of an integer. Returns 0 if not found.
int64_t CompilerStats::GetI64(const std::string& name, int simd) const
{
#if COMPILER_STATS_ENABLE
    return PrivateGet(name, simd)[simd].int64_value;
#else
    return 0;
#endif // COMPILER_STATS_ENABLE
}

// Get value of a floating point. Returns 0.0 if not found.
double CompilerStats::GetF64(const std::string& name, int simd) const
{
#if COMPILER_STATS_ENABLE
    return PrivateGet(name, simd)[simd].double_value;
#else
    return 0.0;
#endif // COMPILER_STATS_ENABLE
}

// Dump stats to csv format string.
std::string CompilerStats::ToCsv() const
{
#if COMPILER_STATS_ENABLE
    if (IsEnabled() == false)
        return "";

    std::stringstream ss;
    ss << "Name,Generic,Simd8,Simd16,Simd32,\n";
    std::map<std::string, Statistic> ordered(m_pState->m_Stats.begin(), m_pState->m_Stats.end());
    for (auto elem : ordered)
    {
        ss << elem.first << ",";
        for(int simd_type = 0; simd_type < Statistic::SimdType::NUM_SIMD_TYPES; simd_type++)
        {
            switch (elem.second.type)
            {
                case Statistic::Type::type_bool:
                    ss << elem.second.values[simd_type].bool_value << ",";
                    break;
                case Statistic::Type::type_int64:
                    ss << elem.second.values[simd_type].int64_value << ",";
                    break;
                case Statistic::Type::type_double:
                    ss << elem.second.values[simd_type].double_value << ",";
                    break;
            }
        }
        ss << "\n";
    }
    return ss.str();
#else
    return "";
#endif // COMPILER_STATS_ENABLE
}

#if COMPILER_STATS_ENABLE
// Create empty statistic if does not exist, return existing if it does.
CompilerStats::Statistic& CompilerStats::PrivateInit(const std::string& name, Statistic::Type type, int simd)
{
    Statistic s;
    s.type = type;
    m_pState->m_Stats[name] = s;
    return m_pState->m_Stats[name];
}

// Try to find statistic. Return empty one if it does not exist.
const CompilerStats::Statistic& CompilerStats::PrivateGet(const std::string& name, int simd) const
{
    auto f = m_pState->m_Stats.find(name);
    if(f != m_pState->m_Stats.end())
    {
        return f->second;
    }
    return m_ZeroStat;
}
#endif // COMPILER_STATS_ENABLE


// Helper function to convert from SimdType enum to integer represenation (8/16/32). Returns 0 for generic simd.
int CompilerStats::Statistic::type_to_simd(CompilerStats::Statistic::SimdType simd_type)
{
    switch (simd_type)
    {
        case SIMD_8:        return 8;
        case SIMD_16:       return 16;
        case SIMD_32:       return 32;
        case SIMD_GENERIC:  return 0;
        default:            return 0;
    }
}

// Helper function to convert from integer represenation (8/16/32) to SimdType enum. Returns SIMD_GENERIC for unexpected values.
CompilerStats::Statistic::SimdType CompilerStats::Statistic::simd_to_type(int simd)
{
    switch (simd)
    {
        case 8:     return SIMD_8;
        case 16:    return SIMD_16;
        case 32:    return SIMD_32;
        case 0:     return SIMD_GENERIC;
        default:    return SIMD_GENERIC;
    }
}

// Get stat value for simd size expressed as integer. Pass 0 to access generic.
CompilerStats::Statistic::Value& CompilerStats::Statistic::operator[](int simd)
{
    const SimdType simd_type = simd_to_type(simd);
    return values[simd_type];
}

// Get stat value for simd size expressed as integer. Pass 0 to access generic.
const CompilerStats::Statistic::Value& CompilerStats::Statistic::operator[](int simd) const
{
    const SimdType simd_type = simd_to_type(simd);
    return values[simd_type];
}

