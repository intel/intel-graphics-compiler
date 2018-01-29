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

#include "../../../API/usc.h"
#include "../../../API/usc_gen7_types.h"
#include "../../../API/usc_gen8.h"

namespace USC
{

/*****************************************************************************\
STRUCT: SCompilerOutputComputeShaderOpenCL_Gen7
\*****************************************************************************/
USC_PARAM()
USC_PARAM_ADAPTER( "ocl" )
struct SCompilerOutputComputeShaderOpenCL_Gen7 :
    public SCompilerOutputCommon_Gen7
{
    void*           pKernelProgram;
    unsigned int    KernelProgramSize;

    bool            HasBarriers;
    bool            PromotedTPM;

    unsigned int    LargestCompiledSIMDSize;
    unsigned int    NumConcurrentLiveRanges;

    unsigned int    ConstantRegisterReadLength;
    unsigned int    ConstantNonOrthogonalStateReadLength;

    unsigned int    SumTPMSizes;
    unsigned int    SumFixedTGSMSizes;

    unsigned int    PerThreadScratchSpaceSize;

    bool            MayAccessUndeclaredResource;
};

/*****************************************************************************\
STRUCT: SSystemThreadKernelOutputOpenCL_Gen7
\*****************************************************************************/
struct SSystemThreadKernelOutputOpenCL_Gen7 :
    public SSystemThreadKernelOutput
{
    unsigned int    PerThreadSystemThreadSurfaceSize;
};


/*****************************************************************************\
STRUCT: SCompilerInputComputeShaderOpenCL_Gen8
\*****************************************************************************/
struct SCompilerInputComputeShaderOpenCL_Gen8
{
    bool                             NoSignedZeros;
    bool                             FiniteMathOnly;
    bool                             UnsafeMathOptimizations;
    bool                             MADEnable;
    bool                             StatefulCompilationEnableControl;
    unsigned int                     GPUPointerSizeInBytes;
    bool                             kernelDebugEnable;

    // DevBXT only+, irrelevant for other platforms
    bool         PooledEUMode;
    unsigned int EUThreadsInPoolMax;
};

/*****************************************************************************\
STRUCT: SCompilerOutputComputeShaderOpenCL_Gen8
\*****************************************************************************/
USC_PARAM()
USC_PARAM_ADAPTER( "ocl" )
struct SCompilerOutputComputeShaderOpenCL_Gen8
{
    void*           pKernelProgram;
    unsigned int    KernelProgramSize;

    bool            HasBarriers;
    bool            PromotedTPM;

    unsigned int    LargestCompiledSIMDSize;
    unsigned int    NumConcurrentLiveRanges;

    unsigned int    ConstantRegisterReadLength;
    unsigned int    ConstantNonOrthogonalStateReadLength;

    unsigned int    SumTPMSizes;
    unsigned int    SumFixedTGSMSizes;

    unsigned int    PerThreadScratchSpaceSize;

    bool            IsCompiledForIndirectPayload;

    // ISA to IL map.
    void*           m_pISA2ILMap[3];
    unsigned int    m_ISA2ILMapSize[3];
};

/*****************************************************************************\
STRUCT: SCompilerOutputComputeShaderOpenCL_Gen8_IGC
\*****************************************************************************/
struct SCompilerOutputComputeShaderOpenCL_Gen8_IGC
    : SCompilerOutputComputeShaderOpenCL_Gen8
{
    bool            HasBarrier;
};

/*****************************************************************************\
STRUCT: SSystemThreadKernelOutputOpenCL_Gen8
\*****************************************************************************/
struct SSystemThreadKernelOutputOpenCL_Gen8 :
    public SSystemThreadKernelOutput
{
    unsigned int    PerThreadSystemThreadSurfaceSize;
};


} // namespace USC
