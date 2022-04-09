/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "types.h"
#include "utility.h"

#if defined(_M_IX86) || defined(_M_AMD64) || \
    defined(__i386__) || defined(__x86_64_)
#include "UFO/portable_cpuid.h"
#endif

namespace iSTD
{
/*****************************************************************************\
ENUM: CPU_INSTRUCTION_LEVEL
\*****************************************************************************/
enum CPU_INSTRUCTION_LEVEL
{
    CPU_INSTRUCTION_LEVEL_UNKNOWN,
    CPU_INSTRUCTION_LEVEL_MMX,
    CPU_INSTRUCTION_LEVEL_SSE,
    CPU_INSTRUCTION_LEVEL_SSE2,
    CPU_INSTRUCTION_LEVEL_SSE3,
    CPU_INSTRUCTION_LEVEL_SSE4,
    CPU_INSTRUCTION_LEVEL_SSE4_1,
    NUM_CPU_INSTRUCTION_LEVELS
};


/*****************************************************************************\
Inline Function:
    GetCpuInstructionLevel

Description:
    Returns the highest level of IA32 instruction extensions supported by the CPU
    ( i.e. SSE, SSE2, SSE4, etc )

Output:
    CPU_INSTRUCTION_LEVEL - highest level of IA32 instruction extension(s) supported
    by CPU

Notes:
    See Table 3-20 Vol2A SW Dev Manual for bit-field numbering
\*****************************************************************************/
inline CPU_INSTRUCTION_LEVEL GetCpuInstructionLevel( void )
{
#if defined(ANDROID) && defined(__SSE4_1__)
    return CPU_INSTRUCTION_LEVEL_SSE4_1;
#elif defined(_M_IX86) || defined(_M_AMD64) || \
      defined(__i386__) || defined(__x86_64_)
    int CPUInfo[4] = { 0, 0, 0, 0 };

    __cpuid(CPUInfo, 1);

    CPU_INSTRUCTION_LEVEL CpuInstructionLevel = CPU_INSTRUCTION_LEVEL_UNKNOWN;
    if( CPUInfo[2] & BIT(19) )
    {
        CpuInstructionLevel = CPU_INSTRUCTION_LEVEL_SSE4_1;
    }
    else if( CPUInfo[2] & BIT(0) )
    {
        CpuInstructionLevel = CPU_INSTRUCTION_LEVEL_SSE3;
    }
    else if( CPUInfo[3] & BIT(26) )
    {
        CpuInstructionLevel = CPU_INSTRUCTION_LEVEL_SSE2;
    }
    else if( CPUInfo[3] & BIT(25) )
    {
        CpuInstructionLevel = CPU_INSTRUCTION_LEVEL_SSE;
    }
    else if( CPUInfo[3] & BIT(23) )
    {
        CpuInstructionLevel = CPU_INSTRUCTION_LEVEL_MMX;
    }

    return CpuInstructionLevel;
#else
    return CPU_INSTRUCTION_LEVEL_UNKNOWN;
#endif
}

}//namespace iSTD
