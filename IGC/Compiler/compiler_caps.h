/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "inc/common/sku_wa.h"
#include "usc.h"
#include "3d/common/iStdLib/types.h"

/*****************************************************************************\
STRUCT: S3DRender
\*****************************************************************************/
struct S3DRenderUnitCapabilities
{
    unsigned int   IsBarycentricInterpolationSupported : 1;     // BIT(0)
    unsigned int   IsInstructionCompactionSupported : 1;     // BIT(1)
    unsigned int : 14;
};

/*****************************************************************************\
STRUCT: S3DDataPort
\*****************************************************************************/
struct S3DDataPortCapabilities
{
    unsigned int   OwordBlockTypes;
    unsigned int   OwordBlockCount[3];
    unsigned int   OwordDualBlockTypes;
    unsigned int   OwordDualBlockCount[2];
};

/*****************************************************************************\
STRUCT: S3DKernelHardwareCapabilities
\*****************************************************************************/
struct S3DKernelHardwareCapabilities
{
    unsigned int   SliceCount;
    unsigned int   SubSliceCount;
    unsigned int   ThreadCount;
    unsigned int   EUCount;
    unsigned int   EUCountPerSubSlice;
    unsigned int   EUThreadsPerEU;
    unsigned int   EUCountPerPoolMax;
    unsigned int   KernelPointerAlignSize;
    unsigned int   CsrSizeInMb;
};

namespace IGC
{
    /*****************************************************************************\
    STRUCT: S3DHardwareCapabilities
    \*****************************************************************************/
    struct SCompilerHwCaps
    {
        unsigned int   VertexShaderThreads;
        unsigned int   VertexShaderThreadsPosh;
        unsigned int   HullShaderThreads;
        unsigned int   DomainShaderThreads;
        unsigned int   GeometryShaderThreads;
        unsigned int   PixelShaderThreadsWindowerRange;
        unsigned int   MediaShaderThreads;
        unsigned int   SharedLocalMemoryBlockSize;

        S3DKernelHardwareCapabilities   KernelHwCaps;
    };

    struct OCLCaps {
        // minimal value for MaxParameterSize specified by OpenCL spec
        static const unsigned MINIMAL_MAX_PARAMETER_SIZE = 1024;

        // maximum size (in bytes) of the arguments that can be passed to OCL kernel
        uint32_t MaxParameterSize = 2048;

    };

    class CPlatform;

    void SetCompilerCaps(const SUscSkuFeatureTable* pSkuFeatureTable, CPlatform* platform);
    void SetCompilerCaps(SKU_FEATURE_TABLE* pSkuFeatureTable, CPlatform* platform);

}
