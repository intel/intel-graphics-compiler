/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

// turn on/off GET_TIME_STATS and GET_SHADER_STATS for the reports.
#define GET_TIME_STATS 1

#if defined( _DEBUG )
    #define GET_SHADER_STATS 1
    #define PRINT_PER_SHADER_STATS 1
    #define GET_MEM_STATS 1
#elif defined( _INTERNAL )
    #define GET_SHADER_STATS 1
    #define PRINT_PER_SHADER_STATS 1
    #define GET_MEM_STATS 0
#endif

#include "common/Types.hpp"
#include "common/MemStats.h"

#include "AdaptorCommon/customApi.hpp"

#include <3d/common/iStdLib/utility.h>

#include <string>
#include <map>

namespace llvm
{
    class raw_ostream;
    class formatted_raw_ostream;
    class Function;
}

class ShaderStats;
class TimeStats;

// *******************************************************//
//                      SHADER STATS
// *******************************************************//

enum SHADER_STATS_ITEMS
{
#define DEFINE_SHADER_STAT( enumName, strinName ) enumName,
#include "shaderStats.h"
#undef DEFINE_SHADER_STAT
};

extern const char* g_cShaderStatItems[STATS_MAX_SHADER_STATS_ITEMS+1];

#if GET_SHADER_STATS

class ShaderStats
{
public:
    explicit ShaderStats()
        : m_shaderType(ShaderType::UNKNOWN),
          m_totalShaderCount(0),
          m_TotalSimd8(0),
          m_TotalSimd16(0),
          m_TotalSimd32(0)
    {
        memset(m_CompileShaderStats, 0, sizeof(int)* STATS_MAX_SHADER_STATS_ITEMS);
    }

    void printShaderStats(ShaderHash hash, ShaderType shaderType, const std::string &postFix);
    int  getShaderStats(SHADER_STATS_ITEMS compileInterval);
    void sumShaderStat(SHADER_STATS_ITEMS compileInterval, int count);
    void miscSumShaderStat(ShaderStats* sStats);
    void printSumShaderStats();
    ShaderType m_shaderType;

private:
    int m_CompileShaderStats[STATS_MAX_SHADER_STATS_ITEMS];
    int m_totalShaderCount;
    int m_TotalSimd8;
    int m_TotalSimd16;
    int m_TotalSimd32;
};


#if PRINT_PER_SHADER_STATS
#define COMPILER_SHADER_STATS_PRINT( shaderStats, shaderType, hash, postFix) \
    do \
    { \
    if( shaderStats ) \
        { \
        (shaderStats)->printShaderStats( hash, shaderType, postFix ); \
        } \
    } while (0)
#else
#define COMPILER_SHADER_STATS_PRINT( shaderStats, shaderType, hash, postFix ) \
    do { } while (0)
#endif

#define COMPILER_SHADER_STATS_SET( shaderStats, compileInterval, isacount ) \
    do \
    { \
        if( shaderStats ) \
        { \
            (shaderStats)->sumShaderStat( compileInterval, isacount ); \
        } \
    } while (0)

#define COMPILER_SHADER_STATS_SUM( sumShaderStats, shaderStats, shaderType ) \
    do \
    { \
    if( (sumShaderStats) && (shaderStats) ) \
        { \
            (sumShaderStats)->m_shaderType = shaderType; \
            for (int i=0;i<STATS_MAX_SHADER_STATS_ITEMS;i++) \
            { \
                (sumShaderStats)->sumShaderStat( \
                    static_cast<SHADER_STATS_ITEMS>(i), \
                    (shaderStats)->getShaderStats(static_cast<SHADER_STATS_ITEMS>(i)) ); \
            } \
            (sumShaderStats)->miscSumShaderStat(shaderStats); \
        } \
    } while (0)

#define COMPILER_SHADER_STATS_PRINT_SUM( sumShaderStats ) \
    do \
    { \
        if( sumShaderStats ) \
        { \
            (sumShaderStats)->printSumShaderStats(); \
        } \
    } while (0)

#define COMPILER_SHADER_STATS_INIT( shaderStats ) \
    do \
    { \
        if (IGC_IS_FLAG_ENABLED(QualityMetricsEnable)) \
        { \
            IGC::Debug::SetDebugFlag(IGC::Debug::DebugFlag::SHADER_QUALITY_METRICS, true); \
        } \
        (shaderStats)  = nullptr; \
        if( IGC::Debug::GetDebugFlag( IGC::Debug::DebugFlag::SHADER_QUALITY_METRICS )) \
        { \
            (shaderStats) = new ShaderStats(); \
        } \
    } while (0)

#define COMPILER_SHADER_STATS_DEL( shaderStats ) \
    do \
    { \
        delete (shaderStats); \
        (shaderStats) = nullptr; \
    } while (0)

#else // GET_SHADER_STATS
#   define COMPILER_SHADER_STATS_SUM( sumShaderStats, shaderStats, shaderType ) do { } while (0)
#   define COMPILER_SHADER_STATS_SET( shaderStats, compileInterval, isacount ) do { } while (0)
#   define COMPILER_SHADER_STATS_PRINT( shaderStats, shaderType, hash, postFix ) do { } while (0)
#   define COMPILER_SHADER_STATS_PRINT_SUM( sumShaderStats ) do { } while (0)
#   define COMPILER_SHADER_STATS_INIT( shaderStats ) do { } while (0)
#   define COMPILER_SHADER_STATS_DEL( shaderStats ) do { } while (0)
#endif // GET_SHADER_STATS

// *******************************************************//
//                      TIME STATS
// *******************************************************//
enum COMPILE_TIME_INTERVALS
{
#define DEFINE_TIME_STAT( enumName, stringName, parentEnum, isVISA, isUnacc, isCoarseTimer, isDashBoardTimer ) enumName,
#include "timeStats.h"
#undef DEFINE_TIME_STAT
};

extern const char* g_cCompTimeIntervals[MAX_COMPILE_TIME_INTERVALS+1];

std::string str( COMPILE_TIME_INTERVALS cti );
COMPILE_TIME_INTERVALS interval(std::string const& str);
bool isVISATimer( COMPILE_TIME_INTERVALS cti );
bool isUnaccounted( COMPILE_TIME_INTERVALS cti );
bool isCoarseTimer( COMPILE_TIME_INTERVALS cti );
bool isDashboardTimer( COMPILE_TIME_INTERVALS cti );
COMPILE_TIME_INTERVALS parentInterval( COMPILE_TIME_INTERVALS cti );
int parentIntervalDepth( COMPILE_TIME_INTERVALS cti );

#if GET_TIME_STATS

struct PerPassTimeStat
{
    uint64_t PassClockStart = 0;
    uint64_t PassElapsedTime = 0;
    int PassHitCount = 0;
};

class TimeStats
{
public:
    TimeStats();
    ~TimeStats();

    /// Capture the VISA timer values for the most recent call to VISABuilder::compile()
    void recordVISATimers();

    /// Mark that a particular timer has started timing
    void recordTimerStart( COMPILE_TIME_INTERVALS compileInterval );
    /// Mark that a particular timer has finished timing
    void recordTimerEnd( COMPILE_TIME_INTERVALS compileInterval );

    /// Get the total elapsed time recorded for a particular timer
    uint64_t getCompileTime( COMPILE_TIME_INTERVALS compileInterval ) const;
    /// Get the number of times a particular timer was triggered
    uint64_t getCompileHit( COMPILE_TIME_INTERVALS compileInterval ) const;

    /// Print the accumulated times for a single shader
    void printTime( ShaderType type, ShaderHash hash, void* context) const;
    void printTime( ShaderType type, ShaderHash hash ) const;
    void printTime( ShaderType type, ShaderHash hash, UINT64 psoDDIHash) const;
    void printTime( ShaderType type, ShaderHash hash, void* context, UINT64 psoDDIHash) const;

    /// Print the aggregated times for multiple shaders
    void printSumTime() const;
    /// Print the times for all passes
    void printPerPassSumTime( llvm::raw_ostream& OS ) const;

    /// Add other's statistics to this
    void sumWith( const TimeStats* pOther );

    /// Get the time elapsed in nanoseconds
    uint64_t getCompileTimeNS(COMPILE_TIME_INTERVALS compileInterval) const
    {
        return uint64_t(getCompileTime(compileInterval) /
                (double)m_freq * 1000000000.0);
    }

    /// Get the time elapsed in milliseconds
    double getCompileTimeMS(COMPILE_TIME_INTERVALS compileInterval) const
    {
        return getCompileTime(compileInterval) /
                (double)m_freq * 1000.0;
    }

    void recordPerPassTimerStart(std::string PassName);
    void recordPerPassTimerEnd(std::string PassName);

private:
    /// \deprecated Print aggregate times for multiple shaders in csv format
    void printSumTimeCSV( std::string const& fileName ) const;
    /// Print aggregate times for multiple shaders in human readable format
    void printSumTimeTable( llvm::raw_ostream & OS ) const;

    bool skipTimer( int i ) const;

    /// \deprecated Print the currently accumulated times in csv format
    void printTimeCSV( std::string const& corpusName, UINT64 psoDDIHash ) const;
    void printPerPassTimeCSV( std::string const& corpusName ) const;
    void printPerPassSumTimeCSV(std::string const& fileName) const;

    // Return a copy of *this, with Unaccounted timer values filled in
    TimeStats postProcess() const;

    bool  m_isPostProcessed;                              //!< Have the Unaccounted timers been calculated yet?
    int   m_totalShaderCount;                             //!< Total number of shaders for which time stats have been recorded
    uint64_t m_wallclockStart[MAX_COMPILE_TIME_INTERVALS];   //!< Most recent starting time of the timer
    uint64_t m_elapsedTime[MAX_COMPILE_TIME_INTERVALS];      //!< Running total of time measured by the timer
    uint64_t m_hitCount[MAX_COMPILE_TIME_INTERVALS];         //!< Number of times a timer was started
    uint64_t m_freq;

    // Per Pass timestats
    uint64_t m_PassTotalTicks;
    std::map<std::string, PerPassTimeStat> m_PassTimeStatsMap;
};

#define COMPILER_TIME_GETNS(pointer, timerName) \
    ((pointer) && (pointer)->m_compilerTimeStats) ? \
        (pointer)->m_compilerTimeStats->getCompileTimeNS(timerName) : 0

#define COMPILER_TIME_START( pointer, compileTimeInterval ) \
    do \
    { \
        if( (pointer) && (pointer)->m_compilerTimeStats ) \
        { \
                (pointer)->m_compilerTimeStats->recordTimerStart( compileTimeInterval );  \
        } \
    } while (0)
#define COMPILER_TIME_END( pointer, compileTimeInterval ) \
    do \
    { \
        if( (pointer) && (pointer)->m_compilerTimeStats ) \
        { \
                (pointer)->m_compilerTimeStats->recordTimerEnd( compileTimeInterval ); \
        } \
    } while (0)

#define COMPILER_TIME_PASS_START( pointer, name ) \
    do \
    { \
        if( (pointer) && (pointer)->m_compilerTimeStats ) \
        { \
                (pointer)->m_compilerTimeStats->recordPerPassTimerStart( name );  \
        } \
    } while (0)
#define COMPILER_TIME_PASS_END( pointer, name ) \
    do \
    { \
        if( (pointer) && (pointer)->m_compilerTimeStats ) \
        { \
                (pointer)->m_compilerTimeStats->recordPerPassTimerEnd( name ); \
        } \
    } while (0)

#define COMPILER_TIME_SUM( pointerDst, pointerSrc ) \
    do \
    { \
        if( (pointerSrc) && (pointerDst) && (pointerDst)->m_sumCompilerTimeStats ) \
        { \
            (pointerDst)->m_sumCompilerTimeStats->sumWith( (pointerSrc)->m_compilerTimeStats ); \
        } \
    } while (0)

#define COMPILER_TIME_SUM2( pointerDst, pointerSrc ) \
    do \
    { \
        if( (pointerDst) && (pointerSrc) ) \
        { \
            (pointerDst)->sumWith( pointerSrc ); \
        } \
    } while (0)

#define COMPILER_TIME_SUM3( pointerDst, pointerSrc, shaderStage ) \
    do \
    { \
        if( (pointerSrc) && (pointerDst) && (pointerDst)->m_sumCompilerTimeStats) \
        { \
            (pointerDst)->m_sumCompilerTimeStats[shaderStage].sumWith( (pointerSrc)->m_compilerTimeStats ); \
        } \
    } while (0)

#define COMPILER_TIME_SUM_PRINT( pointer ) \
    do \
    { \
        if( (pointer) && (pointer)->m_sumCompilerTimeStats ) \
        { \
            if ( IGC_REGKEY_OR_FLAG_ENABLED(DumpTimeStats, TIME_STATS_SUM ) ) \
            { \
                (pointer)->m_sumCompilerTimeStats->printSumTime(); \
            } \
        } \
    } while (0)

#define COMPILER_TIME_SUM_PRINT2( pointer, size ) \
    do \
    { \
        if( (pointer) && (pointer)->m_sumCompilerTimeStats ) \
        { \
            if ( IGC_REGKEY_OR_FLAG_ENABLED(DumpTimeStats, TIME_STATS_SUM ) ) \
            { \
                for(int i = 1; i < size; i++) \
                { \
                    if((pointer)->m_sumCompilerTimeStats[i].getCompileTime(TIME_TOTAL) == 0) \
                        continue; \
                    llvm::dbgs() << ShaderTypeString[i] << "Compile Time"; \
                    (pointer)->m_sumCompilerTimeStats[i].printSumTime(); \
                } \
            } \
        } \
    } while (0)

#define COMPILER_TIME_INIT( pointer, statName )     \
    do                                              \
    {                                               \
        /* Check for pointer to avoid mem leak */   \
        if ((pointer) && !((pointer)->statName))    \
        {                                           \
            (pointer)->statName = new TimeStats();  \
        }                                           \
    } while (0)

#define COMPILER_TIME_INIT2( pointer, statName, size)     \
    do                                              \
    {                                               \
        /* Check for pointer to avoid mem leak */   \
        if ((pointer) && !((pointer)->statName))    \
        {                                           \
            (pointer)->statName = new TimeStats[size];  \
        }                                           \
    } while (0)

#define COMPILER_TIME_DEL( pointer, statName ) \
    do \
    { \
        if( pointer) \
        { \
            delete (pointer)->statName; \
            (pointer)->statName = nullptr; \
        } \
    } while (0)

#define COMPILER_TIME_DEL2( pointer, statName ) \
    do \
    { \
        if( pointer) \
        { \
            delete [](pointer)->statName; \
            (pointer)->statName = nullptr; \
        } \
    } while (0)

#define COMPILER_TIME_DEL_NON_NULL( pointer, statName ) \
    do \
    { \
        delete (pointer)->statName; \
        (pointer)->statName = nullptr; \
    } while (0)

#define COMPILER_TIME_PRINT( pointer, ...) \
    do \
    { \
        if( (pointer) && (pointer)->m_compilerTimeStats ) \
        { \
            if ( IGC::Debug::GetDebugFlag( IGC::Debug::DebugFlag::TIME_STATS_PER_SHADER ) ) \
            { \
                (pointer)->m_compilerTimeStats->printTime( \
                    __VA_ARGS__ ); \
            } \
        } \
    } while (0)

#else // GET_TIME_STATS

#   define COMPILER_TIME_START( pointer, value ) do { } while (0)
#   define COMPILER_TIME_END( pointer, value ) do { } while (0)
#   define COMPILER_TIME_PRINT( pointer, shaderType, shaderhash ) do { } while (0)
#   define COMPILER_TIME_SUM( pointerDst, pointerSrc ) do { } while (0)
#   define COMPILER_TIME_SUM2( pointerDst, pointerSrc ) do { } while (0)
#   define COMPILER_TIME_SUM_PRINT( pointer ) do { } while (0)
#   define COMPILER_TIME_INIT( pointer, statName ) do { } while (0)
#   define COMPILER_TIME_DEL( pointer, statName ) do { } while (0)
#   define COMPILER_TIME_DEL_NON_NULL( pointer, statName ) do { } while (0)

#endif // GET_TIME_STATS

#if GET_MEM_STATS
#include <list>
#include <algorithm>

enum MEMORY_SUMMARY_ITEM
{
    // starting enum for mem usage peak
    SMSUM_HeapUsedPeak = 0,
    SMSUM_SnapHeapUsedAbsolutePeak,
    // starting enum for alloc
    SMSUM_NumAllocations = IGC::MAX_SHADER_MEMORY_SNAPSHOT+1,
    SMSUM_NumSnapAllocations,
    MAX_MEMORY_SUMMARY_ITEM = IGC::MAX_SHADER_MEMORY_SNAPSHOT*2+2,
};

class CMemoryReport
{
public:
    struct MemStat
    {
        unsigned int NumAllocations;
        unsigned int NumReleases;
        unsigned int NumSnapAllocations;
        unsigned int NumSnapReleases;
        unsigned int NumCurrSnapAllocationsPeak;
        unsigned int NumCurrAllocations;
        unsigned int NumCurrAllocationsPeak;
        int          HeapUsed;
        unsigned int HeapUsedPeak;

        int          SnapHeapUsed;
        unsigned int SnapHeapUsedPeak;
        unsigned int SnapHeapUsedAbsolutePeak;
        unsigned int NumSnapAllocationsType[ IGC::SMAT_NUM_OF_TYPES ];
    };

    CMemoryReport( void );
    ~CMemoryReport( void );

    static void MallocMemInstrumentation( size_t _Size );
    static void FreeMemInstrumentation( size_t size );

    void CreateDumpFiles( char &dumpDirectory );
    void UsageReset();
    void UsageSnapshot( IGC::SHADER_MEMORY_SNAPSHOT phase );
    void CreateMemStatsFiles();
    void DumpMemoryStats( ShaderType type, ShaderHash hash );
    void DumpStats( const char *shaderName );
    void SetDetailed( bool Enable );
    void CopyToSummary();
    void DumpSummaryStats();
    const char* ShaderTypeText();

    char m_CsvNameUsageSum[ 1024 ];
    char m_CsvNameUsage[ 1024 ];
    char m_CsvNameAllocs[ 1024 ];
    char m_CsvNameAllocsSubset[ 1024 ];
    char m_DumpDirectory[ 1024 ];
    char m_DumpMemoryStatsFileName[ 1024 ];

    bool m_GrabDetailed;
    MemStat m_Stat;
    MemStat m_Snapshots[ IGC::MAX_SHADER_MEMORY_SNAPSHOT ];

protected:
    int m_SnapCnt;
    int m_LastSnapHeapUsed;
    ShaderType m_type;

    std::list<unsigned> m_SummaryDump[MAX_MEMORY_SUMMARY_ITEM];
    std::list<unsigned>::iterator iter;
};

extern CMemoryReport g_MemoryReport;
// Helper functions
void MemUsageSnapshot( IGC::SHADER_MEMORY_SNAPSHOT phase );
void UsageReset();

#   define MEM_SNAPSHOT( phase )    MemUsageSnapshot((IGC::SHADER_MEMORY_SNAPSHOT)phase )
#   define MEM_USAGERESET           g_MemoryReport.UsageReset()
#   define MEM_INIT                 g_MemoryReport.CreateMemStatsFiles()
#   define MEM_DUMP( type, hash )   g_MemoryReport.DumpMemoryStats( type, hash )
#   define MEM_DUMP_SUMMARY         g_MemoryReport.DumpSummaryStats()

#else

#   define MEM_SNAPSHOT( phase )    do { } while (0)
#   define MEM_USAGERESET           do { } while (0)
#   define MEM_INIT                 do { } while (0)
#   define MEM_DUMP( type, hash )   do { } while (0)
#   define MEM_DUMP_SUMMARY         do { } while (0)
#endif
