/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/Stats.hpp"
#include "Compiler/CodeGenPublic.h"
#include "common/debug/Dump.hpp"

#include <iStdLib/File.h>
#include <iStdLib/Timestamp.h>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/Debug.h>
#include "common/LLVMWarningsPop.hpp"

#include "common/secure_string.h"
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "Probe/Assertion.h"

#if GET_TIME_STATS
// Functions exposed by VISA lib API
extern "C" int64_t getTimerTicks(unsigned int idx);
extern "C" double getTimerCounts(unsigned int idx);
extern "C" void getTimerNames(char* timerName, unsigned int idx);
extern "C" unsigned int getTimerHits(unsigned int idx);
extern "C" unsigned int getTotalTimers();
#endif

namespace {
    static const unsigned g_cIndentSZ = 4; //<! number of spaces to indent by
}

const char* g_cShaderStatItems[STATS_MAX_SHADER_STATS_ITEMS+1] =
{
#define DEFINE_SHADER_STAT( enumName, stringName ) stringName,
#include "shaderStats.h"
#undef DEFINE_SHADER_STAT
};

#if GET_SHADER_STATS

int ShaderStats::getShaderStats( SHADER_STATS_ITEMS compileInterval )
{
    IGC_ASSERT(0 <= compileInterval);
    IGC_ASSERT(compileInterval < STATS_MAX_SHADER_STATS_ITEMS);
    return m_CompileShaderStats[compileInterval];
}

void ShaderStats::printSumShaderStats()
{
    std::string outputFile = IGC::Debug::GetShaderOutputFolder() + std::string("\\SQM\\")+"shaderStatSum.csv";
    std::string sqm_out = IGC::Debug::GetShaderOutputFolder() + std::string("\\SQM\\") + "sqmStats.txt";
    bool fileExist = 0, fileExist_sqm=0;

    FILE* fp = fopen(outputFile.c_str(), "r");
    if( fp )
    {
        fileExist = 1;
        fclose(fp);
    }

    FILE* fileName = fopen(outputFile.c_str(), "a");

    FILE* fp_sqm = fopen(sqm_out.c_str(), "r");
    if (fp_sqm)
    {
        fileExist_sqm = 1;
        fclose(fp_sqm);
    }

    FILE* fileName_sqm = fopen(sqm_out.c_str(), "a");

    if( !fileExist && fileName )
    {
        fprintf(fileName, "test label, shader type, shader count,");
        for (int i=0;i<STATS_MAX_SHADER_STATS_ITEMS;i++)
        {
            fprintf(fileName, "%s,", g_cShaderStatItems[i] );
        }
        fprintf(fileName, "\n");
    }

    if (fileName && m_totalShaderCount!=0)
    {
        fprintf(fileName, "%s,", IGC::Debug::GetShaderCorpusName() );
        fprintf(fileName, "%s,", ShaderTypeString[(int)m_shaderType] );
        fprintf(fileName, "%d,", m_totalShaderCount);
        for (int i=0;i<STATS_MAX_SHADER_STATS_ITEMS;i++)
        {
            fprintf(fileName, "%d,", m_CompileShaderStats[i] );
        }
        fprintf(fileName, "\n");

        fclose(fileName);
    }

    bool printToConsole = true;
    if (printToConsole && m_totalShaderCount!=0)
    {
        fprintf(fileName_sqm, "\n%d %s shaders\n", m_totalShaderCount, ShaderTypeString[(int)m_shaderType]);
        printf("\n%d %s shaders\n", m_totalShaderCount, ShaderTypeString[(int)m_shaderType]);
        if( m_CompileShaderStats[STATS_ISA_INST_COUNT] != 0 )
        {
            fprintf(fileName_sqm, "total SIMD8  isa count = %d\n", m_CompileShaderStats[STATS_ISA_INST_COUNT]);
            printf("total SIMD8  isa count = %d\n", m_CompileShaderStats[STATS_ISA_INST_COUNT] );
        }
        if( m_CompileShaderStats[STATS_ISA_INST_COUNT_SIMD16] != 0 )
        {
            fprintf(fileName_sqm, "total SIMD16 isa count = %d\n", m_CompileShaderStats[STATS_ISA_INST_COUNT_SIMD16]);
            printf("total SIMD16 isa count = %d\n", m_CompileShaderStats[STATS_ISA_INST_COUNT_SIMD16] );
        }
        if( m_CompileShaderStats[STATS_ISA_INST_COUNT_SIMD32] != 0 )
        {
            fprintf(fileName_sqm, "total SIMD32 isa count = %d\n", m_CompileShaderStats[STATS_ISA_INST_COUNT_SIMD32]);
            printf("total SIMD32 isa count = %d\n", m_CompileShaderStats[STATS_ISA_INST_COUNT_SIMD32] );
        }
        if (m_CompileShaderStats[STATS_ISA_SPILL8] != 0)
        {
            fprintf(fileName_sqm, "total SIMD8 spill count = %d\n", m_CompileShaderStats[STATS_ISA_SPILL8]);
            printf("total SIMD8 spill count = %d\n", m_CompileShaderStats[STATS_ISA_SPILL8]);
        }
        if (m_CompileShaderStats[STATS_ISA_SPILL16] != 0)
        {
            fprintf(fileName_sqm, "total SIMD16 spill count = %d\n", m_CompileShaderStats[STATS_ISA_SPILL16]);
            printf("total SIMD16 spill count = %d\n", m_CompileShaderStats[STATS_ISA_SPILL16]);
        }
        if (m_CompileShaderStats[STATS_ISA_SPILL32] != 0)
        {
            fprintf(fileName_sqm,"total SIMD32 spill count = %d\n", m_CompileShaderStats[STATS_ISA_SPILL32]);
            printf("total SIMD32 spill count = %d\n", m_CompileShaderStats[STATS_ISA_SPILL32]);
        }
        fprintf(fileName_sqm, "total SIMD8  shaders = %d\n", m_TotalSimd8);
        fprintf(fileName_sqm, "total SIMD16 shaders = %d\n", m_TotalSimd16);
        fprintf(fileName_sqm, "total SIMD32 shaders = %d\n", m_TotalSimd32);
        printf("total SIMD8  shaders = %d\n", m_TotalSimd8);
        printf("total SIMD16 shaders = %d\n", m_TotalSimd16);
        printf("total SIMD32 shaders = %d\n", m_TotalSimd32);
    }
    fclose(fileName);
}

void ShaderStats::printShaderStats( ShaderHash hash, ShaderType shaderType, const std::string &postFix)
{
    const std::string outputFilePath = IGC::Debug::GetShaderOutputFolder() + std::string("\\SQM\\") + IGC::Debug::GetShaderCorpusName() + "ShaderStats.csv";
    const char *outputFile = outputFilePath.c_str();

    bool fileExist = 0;

    FILE* fp = fopen(outputFile, "r");
    if (fp)
    {
        fileExist = 1;
        fclose(fp);
    }

    FILE* fileName = fopen(outputFile, "a");

    if (!fileExist)
    {
        fprintf(fileName, "shader,");
        for (int i = 0; i<STATS_MAX_SHADER_STATS_ITEMS; i++)
        {
            fprintf(fileName, "%s,", g_cShaderStatItems[i]);
        }
        fprintf(fileName, "\n");
    }

    std::string asmFileName;

        asmFileName =
            IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName())
            .Type(shaderType)
            .Hash(hash)
            .Extension("asm")
            .str();
        if (asmFileName.find_last_of("\\") != std::string::npos)
        {
            asmFileName = asmFileName.substr(asmFileName.find_last_of("\\") + 1, asmFileName.size());
        }


    fprintf(fileName, "%s,", asmFileName.c_str());
    for (int i = 0; i<STATS_MAX_SHADER_STATS_ITEMS; i++)
    {
        fprintf(fileName, "%d,", m_CompileShaderStats[i]);
    }

    fprintf(fileName, "\n");
    fclose(fileName);
}

void ShaderStats::printOpcodeStats(ShaderHash hash, ShaderType shaderType, const std::string &postFix)
{
    /*const std::string opcodeFilePath = IGC::Debug::GetShaderOutputFolder() + std::string("\\SQM\\") + IGC::Debug::GetShaderCorpusName() + "OpcodeShaderStats.csv";
    const char *opcodeFile = opcodeFilePath.c_str();

    const std::string targetUnitFilePath = IGC::Debug::GetShaderOutputFolder() + std::string("\\SQM\\") + IGC::Debug::GetShaderCorpusName() + "TargetUnitShaderStats.csv";
    const char *targetUnitFile = targetUnitFilePath.c_str();

    const std::string listFilePath = IGC::Debug::GetShaderOutputFolder() + std::string("\\SQM\\") + IGC::Debug::GetShaderCorpusName() + "ShadersList.txt";
    const char *listUnitFile = listFilePath.c_str();

    FILE* opcodeFileName = fopen(opcodeFile, "a");
    FILE* targetUnitFileName = fopen(targetUnitFile, "a");
    FILE* listUnitFileName = fopen(listUnitFile, "a");

    std::string asmFileName;
    if (shaderType == ShaderType::OPENCL_SHADER)
    {
        asmFileName =
            IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName())
            .Type(shaderType)
            .Hash(hash)
            .PostFix(postFix)
            .Extension("asm")
            .str();
        if (asmFileName.find_last_of("\\") != std::string::npos)
        {
            asmFileName = asmFileName.substr(asmFileName.find_last_of("\\") + 1, asmFileName.size());
        }
    }
    else
    {
        asmFileName =
            IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName())
            .Type(shaderType)
            .Hash(hash)
            .Extension("asm")
            .str();
        if (asmFileName.find_last_of("\\") != std::string::npos)
        {
            asmFileName = asmFileName.substr(asmFileName.find_last_of("\\") + 1, asmFileName.size());
        }
    }


    fprintf(opcodeFileName, "%s","");
    fclose(opcodeFileName);

    fprintf(targetUnitFileName, "%s","");
    fclose(targetUnitFileName);

    fprintf(listUnitFileName, "%s\n", asmFileName.c_str());
    fclose(listUnitFileName);
    ?*/
}

void ShaderStats::parseIsaShader( ShaderHash hash, ShaderType shaderType, SIMDMode simd )
{

    std::string line, instStr;

    std::string asmFileName =
        IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName())
            .Type(shaderType)
            .Hash(hash)
            .SIMDSize(simd)
            .Extension("asm")
            .str();
    if (asmFileName.find_last_of("\\") != std::string::npos)
    {
        asmFileName = asmFileName.substr(asmFileName.find_last_of("\\") + 1, asmFileName.size());
    }

    std::ifstream asmFile(asmFileName.c_str());

    while( getline(asmFile, line) )
    {
        if( line == ".code" )
        {
            break;
        }
    }

    while( getline(asmFile, line) )
    {
        if( line == "" || line.find( "//", 0 ) == 0 || line == "main:" || line.find( "label", 0 ) == 0)
        {
            continue;
        }
        else if (line.find(" ", 0) == std::string::npos && line.find(":", 0) == line.size() - 1)
        {
            continue;
        }
        else if( line == ".end_code" )
        {
            break;
        }

        if( line.find("(",0 ) == 0 )
        {
            line = line.substr( line.find(")",0) + 2, line.length() );
        }

        instStr = line.substr( 0, line.find(" ", 0) );

        auto hasDot = instStr.find(".",0);
        if (hasDot != std::string::npos)
        {
            instStr = instStr.substr( 0, hasDot );
        }

        if( line.find("L",0) == 0  || line.find("_AUTO_LABEL", 0) == 0)
        {
            m_CompileShaderStats[STATS_ISA_BASIC_BLOCKS]++;
        }
        else if( instStr == "add" || instStr == "addc" || instStr == "avg" || instStr == "dp2" ||
            instStr == "dp3" || instStr == "dp4" || instStr == "dph" || instStr == "frc" ||
            instStr == "line" || instStr == "lrp" || instStr == "mac" || instStr == "mach" ||
            instStr == "mad" || instStr == "madm" || instStr == "math" || instStr == "mul" ||
            instStr == "pln" || instStr == "rndd" || instStr == "rnde" || instStr == "rndu" ||
            instStr == "rndz" || instStr == "sad2" || instStr == "sada2" || instStr == "subb" )
        {
            m_CompileShaderStats[STATS_ISA_ALU]++;
        }
        else if( instStr == "and" || instStr == "asr" || instStr == "bfe" || instStr == "bfi1" ||
            instStr == "bfi2" || instStr == "bfrev" || instStr == "cbit" || instStr == "fbh" ||
            instStr == "fbl" || instStr == "lzd" || instStr == "not" || instStr == "or" ||
            instStr == "shl" || instStr == "shr" || instStr == "xor" )
        {
            m_CompileShaderStats[STATS_ISA_LOGIC]++;
        }
        else if( instStr == "brc" || instStr == "brd" || instStr == "jumpi")
        {
            m_CompileShaderStats[STATS_ISA_THREADCF]++;
        }
        else if(instStr == "break" || instStr == "cont" || instStr == "while" ||
            instStr == "else" || instStr == "endif" || instStr == "if" )
        {
            m_CompileShaderStats[STATS_ISA_STRUCTCF]++;
        }
        else if( instStr == "goto" || instStr == "join" )
        {
            m_CompileShaderStats[STATS_ISA_GOTOJOIN]++;
        }
        else if( instStr == "call" || instStr == "calla" )
        {
            m_CompileShaderStats[STATS_ISA_CALL]++;
        }
        else if( instStr == "cmp" || instStr == "cmpn" || instStr == "csel" || instStr == "sel")
        {
            m_CompileShaderStats[STATS_ISA_SEL_CMP]++;
        }
        else if( instStr == "mov" || instStr == "movi" || instStr == "smov" )
        {
            m_CompileShaderStats[STATS_ISA_MOV]++;
        }
        else if( instStr == "send" || instStr == "sendc" || instStr == "sends" )
        {
            m_CompileShaderStats[STATS_ISA_SEND]++;
        }
        else if( instStr == "halt" || instStr == "illegal" || instStr == "nop" || instStr == "wait" ||
            instStr == "ret" )
        {
            m_CompileShaderStats[STATS_ISA_OTHERS]++;
        }
        else if( line.find( "_GOTO_TARGET", 0 ) == 0 )
        {
            ;
        }
        else
        {
            IGC_ASSERT(0);
        }
    }

    int statsIndex = STATS_ISA_INST_COUNT;
    if( simd == SIMDMode::SIMD16 )
    {
        statsIndex = STATS_ISA_INST_COUNT_SIMD16;
    }
    else if( simd == SIMDMode::SIMD32 )
    {
        statsIndex = STATS_ISA_INST_COUNT_SIMD32;
    }

    for( int i=STATS_ISA_ALU; i<STATS_MAX_SHADER_STATS_ITEMS; i++)
    {
        m_CompileShaderStats[statsIndex] += m_CompileShaderStats[i];
    }

    if( simd == SIMDMode::SIMD16 )
    {
        m_CompileShaderStats[STATS_ISA_INST_COUNT_SIMD16] -= m_CompileShaderStats[STATS_ISA_INST_COUNT];
    }
    else if( simd == SIMDMode::SIMD32 )
    {
        m_CompileShaderStats[STATS_ISA_INST_COUNT_SIMD32] -= m_CompileShaderStats[STATS_ISA_INST_COUNT];
    }
    asmFile.close();
}

void ShaderStats::sumShaderStat( SHADER_STATS_ITEMS compileInterval, int count )
{
    IGC_ASSERT(0 <= compileInterval);
    IGC_ASSERT(compileInterval < STATS_MAX_SHADER_STATS_ITEMS);
    m_CompileShaderStats[ compileInterval ] += count;
};

void ShaderStats::miscSumShaderStat(ShaderStats* sStats)
{
    ++m_totalShaderCount;
    if (sStats->getShaderStats(STATS_ISA_INST_COUNT) > 0)
    {
        ++m_TotalSimd8;
    }
    if (sStats->getShaderStats(STATS_ISA_INST_COUNT_SIMD16) > 0)
    {
        ++m_TotalSimd16;
    }
    if (sStats->getShaderStats(STATS_ISA_INST_COUNT_SIMD32) > 0)
    {
        ++m_TotalSimd32;
    }
}

#endif // GET_SHADER_STATS

const char* g_cCompTimeIntervals[MAX_COMPILE_TIME_INTERVALS+1] =
{
#define DEFINE_TIME_STAT( enumName, stringName, parentEnum, isVISA, isUnacc, isCoarseTimer, isDashBoardTimer ) stringName,
#include "timeStats.h"
#undef DEFINE_TIME_STAT
};

std::string str(COMPILE_TIME_INTERVALS cti)
{
    switch (cti)
    {
#define DEFINE_TIME_STAT( enumName, stringName, parentEnum, isVISA, isUnacc, isCoarseTimer, isDashBoardTimer ) case enumName: return #enumName;
#include "timeStats.h"
#undef DEFINE_TIME_STAT
    default: IGC_ASSERT_MESSAGE(0, "unreachable"); break;
    }

    return "";
}

COMPILE_TIME_INTERVALS interval( std::string const& str )
{
#define DEFINE_TIME_STAT( enumName, stringName, parentEnum, isVISA, isUnacc, isCoarseTimer, isDashBoardTimer ) \
    if ( str == stringName ) \
    { \
        return enumName; \
    }
#include "timeStats.h"
#undef DEFINE_TIME_STAT
    //llvm::errs() << str;
    IGC_ASSERT_MESSAGE(0, "unreachable, unknown COMPILE_TIME_INTERVALS name");
    return MAX_COMPILE_TIME_INTERVALS;
}

bool isVISATimer( COMPILE_TIME_INTERVALS cti )
{
    switch (cti)
    {
#define DEFINE_TIME_STAT( enumName, stringName, parentEnum, isVISA, isUnacc, isCoarseTimer, isDashBoardTimer ) case enumName: return isVISA;
#include "timeStats.h"
#undef DEFINE_TIME_STAT
    default: IGC_ASSERT_MESSAGE(0, "unreachable"); break;
    }
    return false;
}

bool isUnaccounted( COMPILE_TIME_INTERVALS cti )
{
    switch (cti)
    {
#define DEFINE_TIME_STAT( enumName, stringName, parentEnum, isVISA, isUnacc, isCoarseTimer, isDashBoardTimer ) case enumName: return isUnacc;
#include "timeStats.h"
#undef DEFINE_TIME_STAT
    default: IGC_ASSERT_MESSAGE(0, "unreachable"); break;
    }
    return false;
}

bool isCoarseTimer( COMPILE_TIME_INTERVALS cti )
{
    switch (cti)
    {
#define DEFINE_TIME_STAT( enumName, stringName, parentEnum, isVISA, isUnacc, isCoarseTimer, isDashBoardTimer ) case enumName: return isCoarseTimer;
#include "timeStats.h"
#undef DEFINE_TIME_STAT
    default: IGC_ASSERT_MESSAGE(0, "unreachable"); break;
    }
    return true;
}

bool isDashboardTimer( COMPILE_TIME_INTERVALS cti )
{
    switch (cti)
    {
#define DEFINE_TIME_STAT( enumName, stringName, parentEnum, isVISA, isUnacc, isCoarseTimer, isDashBoardTimer ) case enumName: return isDashBoardTimer;
#include "timeStats.h"
#undef DEFINE_TIME_STAT
    default: IGC_ASSERT_MESSAGE(0, "unreachable"); break;
    }

    return true;
}

COMPILE_TIME_INTERVALS parentInterval( COMPILE_TIME_INTERVALS cti )
{
    switch (cti)
    {
#define DEFINE_TIME_STAT( enumName, stringName, parentEnum, isVISA, isUnacc, isCoarseTimer, isDashBoardTimer ) case enumName: return parentEnum;
#include "timeStats.h"
#undef DEFINE_TIME_STAT
    default: IGC_ASSERT_MESSAGE(0, "unreachable"); break;
    }
    return MAX_COMPILE_TIME_INTERVALS;
}

int parentIntervalDepth( COMPILE_TIME_INTERVALS cti )
{
    if ( cti == TIME_TOTAL )
    {
        return 0;
    }
    else
    {
        return 1 + parentIntervalDepth( parentInterval( cti ) );
    }
}

#if GET_TIME_STATS

TimeStats::TimeStats()
    : m_isPostProcessed(false)
    , m_totalShaderCount(0)
    , m_PassTotalTicks (0)
{
    std::fill(std::begin(m_wallclockStart), std::end(m_wallclockStart), 0);
    std::fill(std::begin(m_elapsedTime),    std::end(m_elapsedTime),    0);
    std::fill(std::begin(m_hitCount),       std::end(m_hitCount),       0);
    m_freq = iSTD::GetTimestampFrequency();

    m_PassTimeStatsMap.clear();
}

TimeStats::~TimeStats()
{
    m_PassTimeStatsMap.clear();
}

void TimeStats::recordVISATimers()
{
    // getTotalTimers() +1 because there is a unaccounted counter
    for (unsigned int i = 0; i < getTotalTimers(); ++i)
    {
        m_elapsedTime[TIME_VISA_TOTAL+i] += getTimerTicks(i);
        m_hitCount[TIME_VISA_TOTAL + i] = getTimerHits(i);
    }
}

void TimeStats::recordTimerStart( COMPILE_TIME_INTERVALS compileInterval )
{
    IGC_ASSERT(0 <= compileInterval);
    IGC_ASSERT(compileInterval < MAX_COMPILE_TIME_INTERVALS);
    m_wallclockStart[ compileInterval ] = iSTD::GetTimestampCounter();
}

void TimeStats::recordTimerEnd( COMPILE_TIME_INTERVALS compileInterval )
{
    IGC_ASSERT(0 <= compileInterval);
    IGC_ASSERT(compileInterval < MAX_COMPILE_TIME_INTERVALS);
    m_elapsedTime[ compileInterval ] += iSTD::GetTimestampCounter() - m_wallclockStart[ compileInterval ];
    m_hitCount[ compileInterval ]++;
}

uint64_t TimeStats::getCompileTime( COMPILE_TIME_INTERVALS compileInterval ) const
{
    IGC_ASSERT(0 <= compileInterval);
    IGC_ASSERT(compileInterval < MAX_COMPILE_TIME_INTERVALS);
    return m_elapsedTime[ compileInterval ];
}

uint64_t TimeStats::getCompileHit( COMPILE_TIME_INTERVALS compileInterval ) const
{
    IGC_ASSERT(0 <= compileInterval);
    IGC_ASSERT(compileInterval < MAX_COMPILE_TIME_INTERVALS);
    return m_hitCount[ compileInterval ];
}

void TimeStats::sumWith( const TimeStats* pOther )
{
    // Add pOther's compile time's to us
    for (int i=0;i<MAX_COMPILE_TIME_INTERVALS;i++)
    {
        m_elapsedTime[ i ] += pOther->m_elapsedTime[ i ];
        m_hitCount[ i ]    += pOther->m_hitCount[ i ];
    }

    m_totalShaderCount++;

    m_PassTotalTicks += pOther->m_PassTotalTicks;

    if (IGC_REGKEY_OR_FLAG_ENABLED(DumpTimeStatsPerPass, TIME_STATS_PER_PASS))
    {
        if (m_PassTimeStatsMap.empty())
        {
            m_PassTimeStatsMap.insert(pOther->m_PassTimeStatsMap.begin(), pOther->m_PassTimeStatsMap.end());
        }
        else
        {
            std::map<std::string, PerPassTimeStat>::const_iterator fromIter;
            std::map<std::string, PerPassTimeStat>::iterator toIter;

            for (fromIter = pOther->m_PassTimeStatsMap.begin(); fromIter != pOther->m_PassTimeStatsMap.end(); fromIter++)
            {
                toIter = m_PassTimeStatsMap.find(fromIter->first);
                if (toIter != m_PassTimeStatsMap.end())
                {
                    // If the pass is already included in the combined list, add the numbers
                    toIter->second.PassHitCount += fromIter->second.PassHitCount;
                    toIter->second.PassElapsedTime += fromIter->second.PassElapsedTime;
                }
                else
                {
                    // Add new pass from this run to the combined list
                    PerPassTimeStat stat;
                    stat.PassElapsedTime = fromIter->second.PassElapsedTime;
                    stat.PassHitCount = fromIter->second.PassHitCount;
                    m_PassTimeStatsMap.insert(std::pair<std::string, PerPassTimeStat>(fromIter->first, stat));
                }
            }
        }
    }
}

void TimeStats::printTime( ShaderType type, ShaderHash hash) const
{
    printTime(type, hash, nullptr, 0);
}

void TimeStats::printTime(ShaderType type, ShaderHash hash, void* context) const
{
    printTime(type, hash, context , 0);
}

void TimeStats::printTime(ShaderType type, ShaderHash hash, UINT64 psoDDIHash) const
{
    printTime(type, hash, nullptr, psoDDIHash);
}

void TimeStats::printTime(ShaderType type, ShaderHash hash, void* context, UINT64 psoDDIHash) const
{
    TimeStats pp = postProcess();

    std::string shaderName = IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName()).Type(type).Hash(hash).StagedInfo(context).str().c_str();
    if (shaderName.find_last_of("\\") != std::string::npos)
    {
        shaderName = shaderName.substr(shaderName.find_last_of("\\") + 1, shaderName.size());
    }

    pp.printTimeCSV( shaderName, psoDDIHash);

    // Skip printing PerPass info to CSV for now
    //pp.printPerPassTimeCSV( shaderName );
    // Avoid printing the timestats for each shader to screen
    // pp.printSumTimeTable(llvm::dbgs());
}

void TimeStats::printSumTime() const
{
    // If using regkey to turn on timestats, CorpusName is not initialized properly
    if (strnlen_s(IGC::Debug::GetShaderCorpusName(), 1) == 0)
    {
        std::stringstream corpusName;
        corpusName << m_totalShaderCount << " shaders";
        IGC::Debug::SetShaderCorpusName(corpusName.str().c_str());
    }

    TimeStats pp = postProcess();

    bool dumpCoarse = IGC_REGKEY_OR_FLAG_ENABLED(DumpTimeStatsCoarse, TIME_STATS_COARSE);

    if ( dumpCoarse )
    {
        pp.printSumTimeCSV("c:\\Intel\\TimeStatCoarseSum.csv");
    }
    else
    {
        pp.printSumTimeCSV("c:\\Intel\\TimeStatSum.csv");
    }

    if (IGC_REGKEY_OR_FLAG_ENABLED(DumpTimeStatsPerPass, TIME_STATS_PER_PASS))
    {
        pp.printPerPassSumTime(llvm::dbgs());
        pp.printPerPassSumTimeCSV("c:\\Intel\\TimeStatPerPassSum.csv");
    }

    pp.printSumTimeTable(llvm::dbgs());
}

bool TimeStats::skipTimer( int i ) const
{
    const COMPILE_TIME_INTERVALS interval = static_cast<COMPILE_TIME_INTERVALS>(i);
    if( IGC_REGKEY_OR_FLAG_ENABLED(DumpTimeStatsCoarse, TIME_STATS_COARSE ) && !isCoarseTimer( interval ) )
    {
        return true;
    }
    if( !isDashboardTimer( interval ) )
    {
        return true;
    }
    return false;
}
void TimeStats::printSumTimeCSV(const char* outputFile) const
{
    IGC_ASSERT_MESSAGE(m_isPostProcessed, "Print functions should only be called on a Post-Processed TimeStats object");

    bool fileExist = false;

    FILE* fp = fopen(outputFile, "r");
    if( fp )
    {
        fileExist = true;
        fclose(fp);
    }

    FILE* fileName = fopen(outputFile, "a");

    if( !fileExist && fileName )
    {
        fprintf(fileName, "Frequency,%ju\n\n", m_freq);
        fprintf(fileName, "corpus name,shader count,");
        for (int i=0;i<MAX_COMPILE_TIME_INTERVALS;i++)
        {
            if( !skipTimer( i ) )
            {
                fprintf(fileName, "%s,", g_cCompTimeIntervals[i] );
            }
        }
        fprintf(fileName, "\n");
    }

    if( fileName )
    {
        fprintf(fileName, "%s,%d,", IGC::Debug::GetShaderCorpusName(), m_totalShaderCount );

        // print ticks
        for (int i=0;i<MAX_COMPILE_TIME_INTERVALS;i++)
        {
            if( !skipTimer( i ) )
            {
                fprintf(fileName, "%jd,", getCompileTime(static_cast<COMPILE_TIME_INTERVALS>(i)) );
            }
        }
        fprintf(fileName, "\n");

        if( !IGC_REGKEY_OR_FLAG_ENABLED(DumpTimeStatsCoarse, TIME_STATS_COARSE))
        {
            // print secs
            fprintf(fileName, "seconds,," );
            for (int i=0;i<MAX_COMPILE_TIME_INTERVALS;i++)
            {
                fprintf(fileName, "%f,", (double)getCompileTime(static_cast<COMPILE_TIME_INTERVALS>(i))/(double)m_freq);
            }
            fprintf(fileName, "\n");

            // print percentage
            fprintf(fileName, "percentage,," );
            for (int i=0;i<MAX_COMPILE_TIME_INTERVALS;i++)
            {
                fprintf(fileName, "%f,", (double)getCompileTime(static_cast<COMPILE_TIME_INTERVALS>(i))/(double)getCompileTime(TIME_TOTAL) * 100. );
            }
            fprintf(fileName, "\n");

            // print hit count
            fprintf(fileName, "hit,," );
            for (int i=0;i<MAX_COMPILE_TIME_INTERVALS;i++)
            {
                fprintf(fileName, "%ju,", m_hitCount[i] );
            }
            fprintf(fileName, "\n");
        }

        fclose(fileName);
    }
}

void TimeStats::printPerPassSumTimeCSV(const char* outputFile) const
{
    IGC_ASSERT_MESSAGE(m_isPostProcessed, "Print functions should only be called on a Post-Processed TimeStats object");

    if (m_PassTimeStatsMap.empty())
    {
        return;
    }

    bool fileExist = false;

    FILE* fp = fopen(outputFile, "r");
    if (fp)
    {
        fileExist = true;
        fclose(fp);
    }

    FILE* fileName = fopen(outputFile, "a");
    std::map<std::string, PerPassTimeStat>::const_iterator iter;

    if (!fileExist && fileName)
    {
        fprintf(fileName, "Frequency,%ju\n\n", m_freq);
        fprintf(fileName, "corpus name,passes count,Total,");

        for (iter = m_PassTimeStatsMap.begin(); iter != m_PassTimeStatsMap.end(); iter++)
        {
            fprintf(fileName, "%s,", iter->first.c_str());
        }
        fprintf(fileName, "\n");
    }

    if (fileName)
    {
        fprintf(fileName, "%s,%d,%ju,", IGC::Debug::GetShaderCorpusName(), (int)m_PassTimeStatsMap.size(), m_PassTotalTicks);

        for (iter = m_PassTimeStatsMap.begin(); iter != m_PassTimeStatsMap.end(); iter++)
        {
            fprintf(fileName, "%jd,", iter->second.PassElapsedTime);
        }

        fprintf(fileName, "\n");
        fclose(fileName);
    }
}

void TimeStats::recordPerPassTimerStart(std::string PassName)
{
    IGC_ASSERT(!PassName.empty());

    std::map<std::string, PerPassTimeStat>::iterator iter = m_PassTimeStatsMap.find(PassName);
    if (iter == m_PassTimeStatsMap.end())
    {
        PerPassTimeStat stat;
        stat.PassElapsedTime = 0;
        stat.PassHitCount = 0;
        stat.PassClockStart = iSTD::GetTimestampCounter();

        m_PassTimeStatsMap.insert(std::pair<std::string, PerPassTimeStat>(PassName, stat));
    }
    else
    {
        iter->second.PassClockStart = iSTD::GetTimestampCounter();
    }
}

void TimeStats::recordPerPassTimerEnd(std::string PassName)
{
    IGC_ASSERT(!PassName.empty());

    std::map<std::string, PerPassTimeStat>::iterator iter = m_PassTimeStatsMap.find(PassName);

    if (iter == m_PassTimeStatsMap.end())
    {
        std::cerr << "Invalid Pass " << PassName << std::endl;
    }
    else
    {
        uint64_t start = iter->second.PassClockStart;
        uint64_t elapsed = iSTD::GetTimestampCounter() - start;

        iter->second.PassHitCount += 1;
        iter->second.PassElapsedTime += elapsed;
        m_PassTotalTicks += elapsed;
    }
}

namespace {
    std::string str( double num, unsigned width, unsigned sigFigs )
    {
        char numStr[32];
#if defined( WIN32 ) || defined( _WIN64 )
        sprintf_s(numStr,                "%*.*f", width, sigFigs, num);
#else
        snprintf(numStr, sizeof(numStr), "%*.*f", width, sigFigs, num);
#endif
        return numStr;
    }

    std::string str( uint64_t num, unsigned width )
    {
        char numStr[32];
#if defined( WIN32 ) || defined( _WIN64 )
        sprintf_s(numStr,                "%*llu", width, num);
#else
        snprintf(numStr, sizeof(numStr), "%*llu", width, (long long unsigned int)num);
#endif
        return numStr;
    }
} // anonymous namespace

void TimeStats::printSumTimeTable( llvm::raw_ostream & OS ) const
{
    IGC_ASSERT_MESSAGE(m_isPostProcessed, "Print functions should only be called on a Post-Processed TimeStats object");

    llvm::formatted_raw_ostream FS(OS);

    FS << "\n";
    FS << "ShaderCount:  "  << m_totalShaderCount << " shaders\n";
    FS << "CorpusName:   \"" << IGC::Debug::GetShaderCorpusName() << "\"\n";
    FS << "Frequency:    "  << m_freq << " ticks/sec\n";

    const unsigned colWidth =  8;                        //<! Width of each of the data columns
    const unsigned startCol =  4;                        //<! Spacing to the left of the whole table
    const unsigned ticksCol = 50;                        //<! Location of the first character of the ticks column
    const unsigned secsCol  = ticksCol + colWidth + 2;   //<! Location of the first character of the seconds column
    const unsigned percCol  =  secsCol + colWidth + 2;   //<! Location of the first character of the percent column
    const unsigned hitCol   =  percCol + colWidth + 2;   //<! Location of the first character of the hit column

    // table header
    FS.PadToColumn(ticksCol) << "ticks";
    FS.PadToColumn( secsCol) << "seconds";
    FS.PadToColumn( percCol) << "percent";
    FS.PadToColumn(  hitCol) << "hits";
    FS << "\n";
    const std::string bar(colWidth+1,'-');
    FS.PadToColumn(ticksCol) << bar;
    FS.PadToColumn( secsCol) << bar;
    FS.PadToColumn( percCol) << bar;
    FS.PadToColumn(  hitCol) << bar;
    FS << "\n";

    // table body
    if (IGC_REGKEY_OR_FLAG_ENABLED(DumpTimeStatsCoarse, TIME_STATS_COARSE))
    {
        uint64_t timeNotInCoarse = getCompileTime(TIME_TOTAL);
        for (int i = 0; i < MAX_COMPILE_TIME_INTERVALS; i++)
        {
            const COMPILE_TIME_INTERVALS interval = static_cast<COMPILE_TIME_INTERVALS>(i);
            if (!skipTimer(i))
            {
                const uint64_t intervalTicks = getCompileTime(interval);
                if (intervalTicks)
                {
                    if (interval != TIME_TOTAL)
                    {
                        timeNotInCoarse -= intervalTicks;
                    }
                    FS.PadToColumn(startCol) << g_cCompTimeIntervals[interval];
                    FS.PadToColumn(ticksCol) << str(intervalTicks, colWidth);
                    FS.PadToColumn(secsCol) << str(intervalTicks / (double)m_freq, colWidth, 4);
                    FS.PadToColumn(percCol) << str(intervalTicks / (double)getCompileTime(TIME_TOTAL) * 100.0, colWidth, 2);
                    FS.PadToColumn(percCol) << str(m_hitCount[i], colWidth);
                    FS << "\n";
                }
            }
        }
        // print the "others" for coarse
        FS.PadToColumn(startCol) << "Others";
        FS.PadToColumn(ticksCol) << str(timeNotInCoarse, colWidth);
        FS.PadToColumn(secsCol) << str(timeNotInCoarse / (double)m_freq, colWidth, 4);
        FS.PadToColumn(percCol) << str(timeNotInCoarse / (double)getCompileTime(TIME_TOTAL) * 100.0, colWidth, 2);
        FS.PadToColumn(percCol) << str(0, colWidth);
        FS << "\n";
    }
    else
    {
        for (int i = 0; i < MAX_COMPILE_TIME_INTERVALS; i++)
        {
            const COMPILE_TIME_INTERVALS interval = static_cast<COMPILE_TIME_INTERVALS>(i);
            if (!skipTimer(i))
            {
                const uint64_t intervalTicks = getCompileTime(interval);
                if (intervalTicks)
                {
                    FS.PadToColumn(startCol).indent(2 * parentIntervalDepth(interval))
                        << g_cCompTimeIntervals[interval];
                    FS.PadToColumn(ticksCol) << str(intervalTicks, colWidth);
                    FS.PadToColumn(secsCol) << str(intervalTicks / (double)m_freq, colWidth, 4);
                    FS.PadToColumn(percCol) << str(intervalTicks / (double)getCompileTime(TIME_TOTAL) * 100.0, colWidth, 2);
                    FS.PadToColumn(percCol) << str(m_hitCount[i], colWidth);
                    FS << "\n";
                }
            }
        }
    }
    FS << "\n";

    FS.flush();
    OS.flush();
}

void TimeStats::printPerPassSumTime(llvm::raw_ostream& OS) const
{
    if (m_PassTimeStatsMap.empty())
    {
        return;
    }

    llvm::formatted_raw_ostream FS(OS);

    FS << "\n";
    FS << "PassesCount:  " << m_PassTimeStatsMap.size() << " IGC/LLVM passes\n";
    FS << "Total Ticks:  " << m_PassTotalTicks << " ticks\n";

    const unsigned colWidth = 8;                      //<! Width of each of the data columns
    const unsigned startCol = 6;                      //<! Spacing to the left of the whole table
    const unsigned ticksCol = 50;                     //<! Location of the first character of the ticks column
    const unsigned percCol = ticksCol + colWidth + 2; //<! Location of the first character of the percent column
    const unsigned hitCol = percCol + colWidth + 2;   //<! Location of the first character of the hit column

    // table header
    FS.PadToColumn(ticksCol) << "ticks";
    FS.PadToColumn(percCol)  << "percent";
    FS.PadToColumn(hitCol)   << "hits";
    FS << "\n";
    const std::string bar(colWidth + 1, '-');
    FS.PadToColumn(ticksCol) << bar;
    FS.PadToColumn(percCol) << bar;
    FS.PadToColumn(hitCol) << bar;
    FS << "\n";

    PerPassTimeStat stat;
    uint64_t ticks = 0;
    std::map<std::string, PerPassTimeStat>::const_iterator iter;

    for (iter = m_PassTimeStatsMap.begin(); iter != m_PassTimeStatsMap.end(); iter++)
    {
        stat = iter->second;
        ticks = stat.PassElapsedTime;

        // pass ticks % hit
        FS.PadToColumn(startCol) << iter->first.substr(0, ticksCol - startCol - 1);
        FS.PadToColumn(ticksCol) << str(ticks, colWidth);
        FS.PadToColumn(percCol) << str(ticks / (double)m_PassTotalTicks * 100.0, colWidth, 2);
        FS.PadToColumn(percCol) << str(stat.PassHitCount, colWidth);
        FS << "\n";
    }

    FS << "\n";
    FS.flush();
    OS.flush();
}

void TimeStats::printTimeCSV( std::string const& corpusName, UINT64 psoDDIHash ) const
{
    IGC_ASSERT_MESSAGE(m_isPostProcessed, "Print functions should only be called on a Post-Processed TimeStats object");

    std::string subFile = "TimeStat_";
    if (strnlen_s(IGC::Debug::GetShaderCorpusName(), 1) == 0)
        subFile += "Shaders";
    else
        subFile += IGC::Debug::GetShaderCorpusName();
    const std::string outputFilePath =
#if defined(__linux__)
    subFile + ".csv";
#else
    std::string("c:\\Intel\\") + subFile + ".csv";
#endif
    const char *outputFile = outputFilePath.c_str();

    bool fileExist = false;

    FILE* fp = fopen(outputFile, "r");
    if( fp )
    {
        fileExist = true;
        fclose(fp);
    }

    FILE* fileName = fopen(outputFile, "a");

    if( !fileExist && fileName)
    {
        fprintf(fileName, "Frequency:%ju,", m_freq);

        if (IGC_IS_FLAG_ENABLED(PrintPsoDdiHash))
            fprintf(fileName, "psoDDIHash,");

        for (int i=0;i<MAX_COMPILE_TIME_INTERVALS;i++)
        {
            if( !skipTimer(i) )
            {
                fprintf(fileName, "%s,", g_cCompTimeIntervals[i] );
            }
        }
        fprintf(fileName, "\n");
    }

    if (fileName)
    {
        fprintf(fileName, "%s.isa,", corpusName.c_str());

        if (IGC_IS_FLAG_ENABLED(PrintPsoDdiHash))
            fprintf(fileName, "%#jx,", psoDDIHash);

        for (int i = 0; i < MAX_COMPILE_TIME_INTERVALS; i++)
        {
            if (!skipTimer(i))
            {
                const COMPILE_TIME_INTERVALS interval = static_cast<COMPILE_TIME_INTERVALS>(i);
                fprintf(fileName, "%jd,", getCompileTime(interval));
            }
        }

        fprintf(fileName, "\n");
        fclose(fileName);
    }

}

void TimeStats::printPerPassTimeCSV(std::string const& corpusName) const
{
    IGC_ASSERT_MESSAGE(m_isPostProcessed, "Print functions should only be called on a Post-Processed TimeStats object");

    if (m_PassTimeStatsMap.empty())
    {
        return;
    }

    std::string subFile = "TimeStatPerPass_";
    if (strnlen_s(IGC::Debug::GetShaderCorpusName(), 1) == 0)
        subFile += "Shaders";
    else
        subFile += IGC::Debug::GetShaderCorpusName();
    const std::string outputFilePath = std::string("c:\\Intel\\") + subFile + ".csv";
    const char* outputFile = outputFilePath.c_str();
    bool fileExist = false;

    FILE* fp = fopen(outputFile, "r");
    if (fp)
    {
        fileExist = true;
        fclose(fp);
    }

    FILE* fileName = fopen(outputFile, "a");

    if (!fileExist && fileName)
    {
        fprintf(fileName, "Frequency:%ju,", m_freq);

        std::map<std::string, PerPassTimeStat>::const_iterator iter;
        fprintf(fileName, "Passes Count,Total,");
        for (iter = m_PassTimeStatsMap.begin(); iter != m_PassTimeStatsMap.end(); iter++)
        {
            fprintf(fileName, "%s,", iter->first.c_str());
        }
        fprintf(fileName, "\n");
    }

    if (fileName)
    {
        fprintf(fileName, "%s.isa,%d,%ju,", corpusName.c_str(), (int)m_PassTimeStatsMap.size(), m_PassTotalTicks);
        std::map<std::string, PerPassTimeStat>::const_iterator iter;

        for (iter = m_PassTimeStatsMap.begin(); iter != m_PassTimeStatsMap.end(); iter++)
        {
            fprintf(fileName, "%jd,", iter->second.PassElapsedTime);
        }

        fprintf(fileName, "\n");
        fclose(fileName);
    }
}

TimeStats TimeStats::postProcess() const
{
    TimeStats copy(*this);

    uint64_t childSum[MAX_COMPILE_TIME_INTERVALS+1];
    std::fill( std::begin(childSum), std::end(childSum), 0 );

    for (int i=0; i <MAX_COMPILE_TIME_INTERVALS; ++i)
    {
        const COMPILE_TIME_INTERVALS interval = static_cast<COMPILE_TIME_INTERVALS>(i);
        const COMPILE_TIME_INTERVALS parent = parentInterval( interval );

        if (m_elapsedTime[interval] == 0) {
            continue;
        }

        if ( isUnaccounted(interval) == false )
        {
            childSum[ parent ] += m_elapsedTime[ interval ];
        }
    }

    for (int i=0; i <MAX_COMPILE_TIME_INTERVALS; ++i)
    {
        const COMPILE_TIME_INTERVALS interval = static_cast<COMPILE_TIME_INTERVALS>(i);
        const COMPILE_TIME_INTERVALS parent = parentInterval( interval );

        if ( isUnaccounted(interval) == true )
        {
            copy.m_elapsedTime[ interval ] = m_elapsedTime[ parent ] - childSum[ parent ];
        }
    }

    copy.m_isPostProcessed = true;

    return copy;
}

#endif // GET_TIME_STATS

#if GET_MEM_STATS

void CMemoryReport::CreateMemStatsFiles()
{
    if( IGC::Debug::GetDebugFlag(IGC::Debug::DebugFlag::MEM_STATS ) )
    {
        sprintf_s(g_MemoryReport.m_CsvNameUsageSum, sizeof(m_CsvNameUsageSum),
            "c:\\Intel\\MemoryStatsSum.csv" );

        sprintf_s(g_MemoryReport.m_CsvNameUsage, sizeof(m_CsvNameUsage),
            "c:\\Intel\\%sMemoryStatsUsage.csv",
            IGC::Debug::GetShaderCorpusName() );

        sprintf_s(g_MemoryReport.m_CsvNameAllocs, sizeof(m_CsvNameAllocs),
            "c:\\Intel\\%sMemoryStatsAllocs.csv",
            IGC::Debug::GetShaderCorpusName() );

        sprintf_s(g_MemoryReport.m_CsvNameAllocsSubset, sizeof(m_CsvNameAllocsSubset),
            "c:\\Intel\\%sMemoryStatsAllocsSubset.csv",
            IGC::Debug::GetShaderCorpusName() );

        FILE* outputFileUsage           = (FILE*)iSTD::FileOpen( (const char*)g_MemoryReport.m_CsvNameUsage, "w" );
        FILE* outputFileAllocs          = (FILE*)iSTD::FileOpen( (const char*)g_MemoryReport.m_CsvNameAllocs, "w" );
        FILE* outputFileAllocsSubset    = (FILE*)iSTD::FileOpen( (const char*)g_MemoryReport.m_CsvNameAllocsSubset, "w" );
        // Write header
        iSTD::FileWrite( outputFileUsage, "Name,Global mem. peak" );
        iSTD::FileWrite( outputFileAllocs, "Name,Global num alloc." );
        iSTD::FileWrite( outputFileAllocsSubset, "Name,Type of Subset" );
        for( int i = 0; i < IGC::MAX_SHADER_MEMORY_SNAPSHOT; i++ )
        {
            if( g_MemoryReport.m_GrabDetailed || IGC::g_cShaderMemorySnapshot[ i ].IsMilestone )
            {
                iSTD::FileWrite( outputFileUsage, ",%s peak,%s end", IGC::g_cShaderMemorySnapshot[ i ].Name, IGC::g_cShaderMemorySnapshot[ i ].Name );
                iSTD::FileWrite( outputFileAllocs, ",%s allocs,%s curr. allocs", IGC::g_cShaderMemorySnapshot[ i ].Name, IGC::g_cShaderMemorySnapshot[ i ].Name );
                iSTD::FileWrite( outputFileAllocsSubset, ",%s allocs", IGC::g_cShaderMemorySnapshot[ i ].Name, IGC::g_cShaderMemorySnapshot[ i ].Name );
            }
        }
        iSTD::FileWrite( outputFileUsage, "\n" );
        iSTD::FileWrite( outputFileAllocs, "\n" );
        iSTD::FileWrite( outputFileAllocsSubset, "\n" );

        iSTD::FileClose( outputFileUsage );
        iSTD::FileClose( outputFileAllocs );
        iSTD::FileClose( outputFileAllocsSubset );
    }
}

/*****************************************************************************\

Function: CMemoryReport::DumpMemoryStats

Description:
    Saves memory statistics to already created file.

Input:
    None

Output:
    None

\*****************************************************************************/
void CMemoryReport::DumpMemoryStats( ShaderType type, ShaderHash hash )
{
    if( IGC::Debug::GetDebugFlag(IGC::Debug::DebugFlag::MEM_STATS ) )
    {
        m_type = type;
        CopyToSummary();

        FILE* csvFileGlobal = (FILE*)iSTD::FileOpen( (const char*) g_MemoryReport.m_CsvNameUsage, "a" );
        FILE* csvFileAllocs = (FILE*)iSTD::FileOpen( (const char*) g_MemoryReport.m_CsvNameAllocs, "a" );
        FILE* csvFileAllocsSubset = (FILE*)iSTD::FileOpen( (const char*) g_MemoryReport.m_CsvNameAllocsSubset, "a" );

        std::string shaderName = IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName()).Type(type).Hash(hash).str().c_str();
        if (shaderName.find_last_of("\\") != std::string::npos)
        {
            shaderName = shaderName.substr(shaderName.find_last_of("\\") + 1, shaderName.size());
        }

        sprintf_s(m_DumpMemoryStatsFileName, sizeof(m_DumpMemoryStatsFileName),
            "%s", shaderName.c_str());

        iSTD::FileWrite(  csvFileGlobal, "%s,%u", m_DumpMemoryStatsFileName, g_MemoryReport.m_Stat.HeapUsedPeak/1024 );
        iSTD::FileWrite(  csvFileAllocs, "%s,%u", m_DumpMemoryStatsFileName, g_MemoryReport.m_Stat.NumAllocations );

        for( int i = 0; i < IGC::MAX_SHADER_MEMORY_SNAPSHOT; i++ )
        {
            if( g_MemoryReport.m_GrabDetailed || IGC::g_cShaderMemorySnapshot[ i ].IsMilestone )
            {
                iSTD::FileWrite( csvFileGlobal, ",%d,%d", g_MemoryReport.m_Snapshots[ i ].SnapHeapUsedAbsolutePeak/1024,
                    g_MemoryReport.m_Snapshots[ i ].HeapUsed/1024 );
                iSTD::FileWrite( csvFileAllocs, ",%d,%d", g_MemoryReport.m_Snapshots[ i ].NumSnapAllocations,
                    g_MemoryReport.m_Snapshots[ i ].NumCurrSnapAllocationsPeak );
            }
        }
        iSTD::FileWrite( csvFileGlobal, "\n" );
        iSTD::FileWrite( csvFileAllocs, "\n" );

        for ( int i = 0; i < IGC::SMAT_NUM_OF_TYPES; i++ )
        {
            iSTD::FileWrite( csvFileAllocsSubset, "%s,%s", m_DumpMemoryStatsFileName,
                IGC::g_cShaderMemoryAllocsType[ i ].Name );
            for( int j = 0; j < IGC::MAX_SHADER_MEMORY_SNAPSHOT; j++ )
            {
                if( g_MemoryReport.m_GrabDetailed || IGC::g_cShaderMemorySnapshot[ j ].IsMilestone )
                {
                    iSTD::FileWrite( csvFileAllocsSubset, ",%d", g_MemoryReport.m_Snapshots[ j ].NumSnapAllocationsType[ i ]);
                }
            }
            iSTD::FileWrite( csvFileAllocsSubset, "\n" );
        }
        iSTD::FileClose( csvFileGlobal );
        iSTD::FileClose( csvFileAllocs );
        iSTD::FileClose( csvFileAllocsSubset );
    }
}

CMemoryReport g_MemoryReport;

/*****************************************************************************\

Function: MemUsageSnapshot

Description: Helper global function for setting usage snapshot

Input: IGC::SHADER_MEMORY_SNAPSHOT phase

Output: None

\*****************************************************************************/
void MemUsageSnapshot( IGC::SHADER_MEMORY_SNAPSHOT phase )
{
    if( IGC::Debug::GetDebugFlag(IGC::Debug::DebugFlag::MEM_STATS ) )
    {
        if( phase == IGC::SMS_COMPILE_START )
        {
            g_MemoryReport.UsageReset();
        }
        g_MemoryReport.UsageSnapshot( phase );
    }
}

/*****************************************************************************\

Function: CMemoryReport::CMemoryReport

Description: CMemoryReport constructor

Input: None

Output: None

\*****************************************************************************/
CMemoryReport::CMemoryReport()
{
    m_GrabDetailed = false;
    m_SnapCnt = 0;
    m_LastSnapHeapUsed = 0;

    for( int sumI = 0; sumI<MAX_MEMORY_SUMMARY_ITEM; sumI++ )
    {
        m_SummaryDump[sumI].clear();
    }
}

/*****************************************************************************\

Function: CMemoryReport::~CMemoryReport

Description: CMemoryReport constructor

Input: None

Output: None

\*****************************************************************************/
CMemoryReport::~CMemoryReport()
{
}

/*****************************************************************************\

Function:
    CMemoryReport::MallocMemInstrumentation

Description:
    Instrumentation for malloc

Input:
    size_t _Size - size of allocated memory

Output:
    None

\*****************************************************************************/
void CMemoryReport::MallocMemInstrumentation( size_t _Size )
{
    ++g_MemoryReport.m_Stat.NumAllocations;
    ++g_MemoryReport.m_Stat.NumSnapAllocations;
    ++g_MemoryReport.m_Stat.NumCurrSnapAllocationsPeak;
    ++g_MemoryReport.m_Stat.NumCurrAllocations;

    for (int i = 0; i < IGC::SMAT_NUM_OF_TYPES; i++)
    {
        if ( _Size <= IGC::g_cShaderMemoryAllocsType[ i ].max_size)
        {
            ++g_MemoryReport.m_Stat.NumSnapAllocationsType[ i ];
            break;
        }
    }

    g_MemoryReport.m_Stat.HeapUsed += reinterpret_cast<int&>(_Size);
    g_MemoryReport.m_Stat.NumCurrAllocationsPeak =
        iSTD::Max<DWORD>( g_MemoryReport.m_Stat.NumCurrAllocationsPeak,
                          g_MemoryReport.m_Stat.NumCurrAllocations );
    g_MemoryReport.m_Stat.NumCurrSnapAllocationsPeak =
        iSTD::Max<DWORD>( g_MemoryReport.m_Stat.NumCurrSnapAllocationsPeak,
                          g_MemoryReport.m_Stat.NumCurrAllocations );
    g_MemoryReport.m_Stat.HeapUsedPeak =
        iSTD::Max<DWORD>( g_MemoryReport.m_Stat.HeapUsedPeak,
                          g_MemoryReport.m_Stat.HeapUsed > 0 ? g_MemoryReport.m_Stat.HeapUsed : 0 );

    g_MemoryReport.m_Stat.SnapHeapUsed += reinterpret_cast<int&>(_Size);
    g_MemoryReport.m_Stat.SnapHeapUsedPeak =
        iSTD::Max<DWORD>( g_MemoryReport.m_Stat.SnapHeapUsedPeak,
                          g_MemoryReport.m_Stat.SnapHeapUsed > 0 ? g_MemoryReport.m_Stat.SnapHeapUsed : 0 );
}

/*****************************************************************************\

Function: CMemoryReport::FreeMemInstrumentation

Description:
    Instrumentation for free instruction

Input:
    size_t size - size of freed memory

Output: None

\*****************************************************************************/
void CMemoryReport::FreeMemInstrumentation( size_t size )
{
    ++g_MemoryReport.m_Stat.NumReleases;
    ++g_MemoryReport.m_Stat.NumSnapReleases;

    g_MemoryReport.m_Stat.HeapUsed -= reinterpret_cast<int&>(size);
    g_MemoryReport.m_Stat.SnapHeapUsed -= reinterpret_cast<int&>(size);

    if( g_MemoryReport.m_Stat.NumCurrAllocations != 0 )
    {
        --g_MemoryReport.m_Stat.NumCurrAllocations;
    }
}

/*****************************************************************************\

Function: CMemoryReport::UsageReset

Description:
    Resets statistics.

Input: None

Output: None

\*****************************************************************************/
void CMemoryReport::UsageReset()
{
    if( IGC::Debug::GetDebugFlag(IGC::Debug::DebugFlag::MEM_STATS ) )
    {
        MemStat cleanStat = {};
        m_Stat = cleanStat;

        memset( m_Snapshots, 0, sizeof( *m_Snapshots ) * IGC::MAX_SHADER_MEMORY_SNAPSHOT );
        m_SnapCnt = 1;   // reserve [0] for 0-based deltas
    }
}

/*****************************************************************************\

Function: CMemoryReport::UsageSnapshot

Description:
    Grab memory stat snapshot and save it under given phase.

Input: IGC::SHADER_MEMORY_SNAPSHOT phase

Output: None

\*****************************************************************************/
void CMemoryReport::UsageSnapshot(
    IGC::SHADER_MEMORY_SNAPSHOT phase )
{
    if( IGC::Debug::GetDebugFlag(IGC::Debug::DebugFlag::MEM_STATS ) )
    {
        if( m_GrabDetailed || IGC::g_cShaderMemorySnapshot[ phase ].IsMilestone )
        {
            m_SnapCnt++;
            if( phase < IGC::MAX_SHADER_MEMORY_SNAPSHOT )
            {
                m_LastSnapHeapUsed = m_LastSnapHeapUsed > 0 ? m_LastSnapHeapUsed : 0;
                // Calculate absolute peak in this snapshot.
                m_Stat.SnapHeapUsedAbsolutePeak = m_LastSnapHeapUsed + m_Stat.SnapHeapUsedPeak;

                m_Snapshots[ phase ] = m_Stat;

                m_LastSnapHeapUsed = m_Stat.HeapUsed;
                m_Stat.SnapHeapUsed = 0;
                m_Stat.SnapHeapUsedPeak = 0;
                m_Stat.NumCurrSnapAllocationsPeak = 0;
                m_Stat.NumSnapAllocations = 0;

                for (int i = 0; i < IGC::SMAT_NUM_OF_TYPES; i++)
                {
                    g_MemoryReport.m_Stat.NumSnapAllocationsType[ i ] = 0;
                }
                m_Stat.NumSnapReleases = 0;
            }
        }
    }
}

/*****************************************************************************\

Function: CMemoryReport::SetDetailed

Description:
    Enables or disables detailed stats aquisition.

Input: bool Enabled

Output: None

\*****************************************************************************/
void CMemoryReport::SetDetailed( bool Enabled )
{
    m_GrabDetailed = Enabled;
}

void CMemoryReport::CopyToSummary()
{
    m_SummaryDump[SMSUM_HeapUsedPeak].push_back( m_Stat.HeapUsedPeak/1024 );
    m_SummaryDump[SMSUM_NumAllocations].push_back( m_Stat.NumAllocations );
    for( int i = 0; i < IGC::MAX_SHADER_MEMORY_SNAPSHOT; i++ )
    {
        m_SummaryDump[ i + SMSUM_SnapHeapUsedAbsolutePeak ].push_back( m_Snapshots[ i ].SnapHeapUsedAbsolutePeak/1024 );
        m_SummaryDump[ i + SMSUM_NumSnapAllocations ].push_back( m_Snapshots[ i ].NumSnapAllocations );
    }
}

const char* CMemoryReport::ShaderTypeText()
{
    if( m_type == ShaderType::PIXEL_SHADER )
    {
        return "PS";
    }
    else if( m_type == ShaderType::VERTEX_SHADER )
    {
        return "VS";
    }
    else if( m_type == ShaderType::GEOMETRY_SHADER )
    {
        return "GS";
    }
    else if( m_type == ShaderType::HULL_SHADER )
    {
        return "HS";
    }
    else if( m_type == ShaderType::DOMAIN_SHADER )
    {
        return "DS";
    }
    else if( m_type == ShaderType::COMPUTE_SHADER )
    {
        return "CS";
    }
    else if( m_type == ShaderType::OPENCL_SHADER )
    {
        return "OCL";
    }
    return "";
}

void CMemoryReport::DumpSummaryStats()
{
    if( IGC::Debug::GetDebugFlag(IGC::Debug::DebugFlag::MEM_STATS ) )
    {

        bool fileExist = false;

        FILE* fileExistFP = fopen( m_CsvNameUsageSum, "r" );
        if( fileExistFP )
        {
            fileExist = true;
            fclose( fileExistFP );
        }

        FILE* csvSummary = fopen( m_CsvNameUsageSum, "a" );
        if( !fileExist )
        {
            // Write header
            fprintf( csvSummary, ",,Global mem. peak" );
            for( int i = 0; i < IGC::MAX_SHADER_MEMORY_SNAPSHOT; i++ )
            {
                if( m_GrabDetailed || IGC::g_cShaderMemorySnapshot[ i ].IsMilestone )
                {
                    fprintf( csvSummary, ",%s peak", IGC::g_cShaderMemorySnapshot[ i ].Name );
                }
            }
            fprintf( csvSummary, ",Shader Type\n" );
        }

        for( int i = 0; i < SMSUM_NumAllocations; i++ )
        {
            m_SummaryDump[ i ].sort();
        }

        fprintf(csvSummary, "%s,max,", IGC::Debug::GetShaderCorpusName());
        for (int i = 0; i < SMSUM_NumAllocations; i++)
        {
            fprintf(csvSummary, "%d,", m_SummaryDump[i].back());
        }
        fprintf(csvSummary, "%s\n", ShaderTypeText());

        fprintf( csvSummary, "%s,max,", IGC::Debug::GetShaderCorpusName() );
        for( int i = 0; i < SMSUM_NumAllocations; i++ )
        {
            fprintf( csvSummary, "%d,", m_SummaryDump[i].back() );
        }
        fprintf( csvSummary, "%s\n",  ShaderTypeText() );

        fprintf( csvSummary, "%s,average,", IGC::Debug::GetShaderCorpusName() );

        int shaderCount = m_SummaryDump[0].size();
        for( int i = 0; i < SMSUM_NumAllocations; i++ )
        {
            iter = m_SummaryDump[i].begin();
            int tempSum = 0;
            // find average
            for( int i=0;i<shaderCount;i++ )
            {
                tempSum += *iter;
                iter++;
            }
            fprintf( csvSummary, "%.02f,", (float)tempSum/shaderCount );

            // find median
            // for( int i=0;i<shaderCount/2;i++ )
            // {
            //     iter++;
            // }
            // fprintf( csvSummary, "%d,", *iter );
        }
        fprintf( csvSummary, "%s\n",  ShaderTypeText() );

        fprintf( csvSummary, "%s,median,", IGC::Debug::GetShaderCorpusName() );

        for( int i = 0; i < SMSUM_NumAllocations; i++ )
        {
            iter = m_SummaryDump[i].begin();
            // find median
            for( int i=0;i<shaderCount/2;i++ )
            {
                iter++;
            }
            fprintf( csvSummary, "%d,", *iter );
        }
        fprintf( csvSummary, "%s\n",  ShaderTypeText() );

        fprintf( csvSummary, "%s,sum,", IGC::Debug::GetShaderCorpusName() );
        for( int i = SMSUM_NumAllocations; i < MAX_MEMORY_SUMMARY_ITEM; i++ )
        {
            unsigned totalAllocs = 0;

            for( iter = m_SummaryDump[i].begin(); iter != m_SummaryDump[i].end(); iter++ )
            {
                totalAllocs += *iter;
            }
            fprintf( csvSummary, "%d,", totalAllocs );
        }
        fprintf( csvSummary, "%s\n",  ShaderTypeText() );

        fclose( csvSummary );
    }
}

#endif //GET_MEM_STATS
