/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <exception>
#include <cinttypes>
#include "../../3d/common/iStdLib/iStdLib.h"
#include "IGC/common/igc_debug.h"
#include "IGC/common/igc_regkeys.hpp"
#include "patch_g7.h"
#include "patch_g8.h"
#include "patch_g9.h"
#include "visa/include/RelocationInfo.h"
#include "../sp/sp_debug.h"
#include "Probe/Assertion.h"

namespace iOpenCL
{

void DebugProgramBinaryHeader(
    const iOpenCL::SProgramBinaryHeader* pHeader,
    std::string& output )
{
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "Program Binary Header:\n" );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMagic = %x\n", pHeader->Magic );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tVersion = %d\n", pHeader->Version );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tDevice = %d\n", pHeader->Device );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tGPUPointerSizeInBytes = %d\n", pHeader->GPUPointerSizeInBytes );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tNumberOfKernels = %d\n", pHeader->NumberOfKernels );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSteppingId = %d\n", pHeader->SteppingId );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tPatchListSize = %d\n", pHeader->PatchListSize );
}

void DebugKernelBinaryHeader_Gen7(
    const iOpenCL::SKernelBinaryHeaderGen7* pHeader,
    std::string& output )
{
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "Gen7 Kernel Binary Header:\n" );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCheckSum = %x\n", pHeader->CheckSum );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tShaderHashCode = %" PRIu64 "\n", pHeader->ShaderHashCode );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tKernelNameSize = %d\n", pHeader->KernelNameSize );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tPatchListSize = %d\n", pHeader->PatchListSize );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tKernelHeapSize = %d\n", pHeader->KernelHeapSize );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tGeneralStateHeapSize = %d\n", pHeader->GeneralStateHeapSize );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tDynamicStateHeapSize = %d\n", pHeader->DynamicStateHeapSize );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceStateHeapSize = %d\n", pHeader->SurfaceStateHeapSize );
}

void DebugPatchList(
    const void* pBuffer,
    const DWORD size,
    std::string& output )
{
    if (!IGC_IS_FLAG_ENABLED(DumpOCLProgramInfo))
        return;

    const BYTE* ptr = (const BYTE*)pBuffer;

    DWORD   remaining = size;

    while( remaining )
    {
        const iOpenCL::SPatchItemHeader* pHeader =
            (const iOpenCL::SPatchItemHeader*)ptr;

        switch( pHeader->Token )
        {
        // OpenCL Patch Tokens

        case iOpenCL::PATCH_TOKEN_GLOBAL_MEMORY_OBJECT_KERNEL_ARGUMENT:
            {
                const iOpenCL::SPatchGlobalMemoryObjectKernelArgument* pPatchItem =
                    (const iOpenCL::SPatchGlobalMemoryObjectKernelArgument*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_GLOBAL_MEMORY_OBJECT_KERNEL_ARGUMENT (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tArgumentNumber = %d\n",
                    pPatchItem->ArgumentNumber );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tOffset = %d\n",
                    pPatchItem->Offset );
            }
            break;

        case iOpenCL::PATCH_TOKEN_IMAGE_MEMORY_OBJECT_KERNEL_ARGUMENT:
            {
                const iOpenCL::SPatchImageMemoryObjectKernelArgument* pPatchItem =
                    (const iOpenCL::SPatchImageMemoryObjectKernelArgument*)pHeader;

                const char* type = nullptr;

                switch( pPatchItem->Type )
                {
                case iOpenCL::IMAGE_MEMORY_OBJECT_INVALID:
                    type = "IMAGE_MEMORY_OBJECT_INVALID";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_BUFFER:
                    type = "IMAGE_MEMORY_OBJECT_BUFFER";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_1D:
                    type = "IMAGE_MEMORY_OBJECT_1D";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_1D_ARRAY:
                    type = "IMAGE_MEMORY_OBJECT_1D_ARRAY";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_2D:
                    type = "IMAGE_MEMORY_OBJECT_2D";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY:
                    type = "IMAGE_MEMORY_OBJECT_2D_ARRAY";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_3D:
                    type = "IMAGE_MEMORY_OBJECT_3D";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_CUBE:
                    type = "IMAGE_MEMORY_OBJECT_CUBE";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_CUBE_ARRAY:
                    type = "IMAGE_MEMORY_OBJECT_CUBE_ARRAY";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_2D_DEPTH:
                    type = "IMAGE_MEMORY_OBJECT_2D_DEPTH";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY_DEPTH:
                    type = "IMAGE_MEMORY_OBJECT_2D_ARRAY_DEPTH";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_2D_MSAA:
                    type = "IMAGE_MEMORY_OBJECT_2D_MSAA";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_2D_MSAA_DEPTH:
                    type = "IMAGE_MEMORY_OBJECT_2D_MSAA_DEPTH";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY_MSAA:
                    type = "IMAGE_MEMORY_OBJECT_2D_ARRAY_MSAA";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_2D_ARRAY_MSAA_DEPTH:
                    type = "IMAGE_MEMORY_OBJECT_2D_ARRAY_MSAA_DEPTH";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_2D_MEDIA:
                    type = "IMAGE_MEMORY_OBJECT_2D_MEDIA";
                    break;
                case iOpenCL::IMAGE_MEMORY_OBJECT_2D_MEDIA_BLOCK:
                    type = "IMAGE_MEMORY_OBJECT_2D_MEDIA_BLOCK";
                    break;
                default:
                    type = "Unknown";
                    IGC_ASSERT(0);
                    break;
                };

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_IMAGE_MEMORY_OBJECT_KERNEL_ARGUMENT (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tArgumentNumber = %d\n",
                    pPatchItem->ArgumentNumber );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tType = %s\n",
                    type );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tOffset = %d\n",
                    pPatchItem->Offset );
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tLocationIndex1 = %d\n",
                    pPatchItem->LocationIndex);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tLocationIndex2 = %d\n",
                    pPatchItem->LocationIndex2);
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tWriteable = %s\n",
                    pPatchItem->Writeable ? "true" : "false" );
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tTransformable = %s\n",
                    pPatchItem->Transformable ? "true" : "false");
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tNeedBindlessHandle = %s\n",
                    pPatchItem->needBindlessHandle ? "true" : "false");
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tIsEmulationArgument = %s\n",
                    pPatchItem->IsEmulationArgument ? "true" : "false");
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tbtiOffset = %d\n",
                    pPatchItem->btiOffset);
            }
            break;

        case iOpenCL::PATCH_TOKEN_CONSTANT_MEMORY_OBJECT_KERNEL_ARGUMENT:
            {
                const iOpenCL::SPatchConstantMemoryObjectKernelArgument* pPatchItem =
                    (const iOpenCL::SPatchConstantMemoryObjectKernelArgument*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_CONSTANT_MEMORY_OBJECT_KERNEL_ARGUMENT (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tArgumentNumber = %d\n",
                    pPatchItem->ArgumentNumber );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tOffset = %d\n",
                    pPatchItem->Offset );
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tLocationIndex1 = %d\n",
                    pPatchItem->LocationIndex);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tLocationIndex2 = %d\n",
                    pPatchItem->LocationIndex2);
            }
            break;

        case iOpenCL::PATCH_TOKEN_ALLOCATE_GLOBAL_MEMORY_SURFACE_WITH_INITIALIZATION:
            {
                const iOpenCL::SPatchAllocateGlobalMemorySurfaceWithInitialization* pPatchItem =
                    (const iOpenCL::SPatchAllocateGlobalMemorySurfaceWithInitialization*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_ALLOCATE_GLOBAL_MEMORY_SURFACE_WITH_INITIALIZATION (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tGlobalBufferIndex = %d\n",
                    pPatchItem->GlobalBufferIndex );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tSurfaceStateHeapOffset = %d\n",
                    pPatchItem->Offset );
            }
            break;

        case iOpenCL::PATCH_TOKEN_ALLOCATE_CONSTANT_MEMORY_SURFACE_WITH_INITIALIZATION:
            {
                const iOpenCL::SPatchAllocateConstantMemorySurfaceWithInitialization* pPatchItem =
                        (const iOpenCL::SPatchAllocateConstantMemorySurfaceWithInitialization*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                        "PATCH_TOKEN_ALLOCATE_CONSTANT_MEMORY_SURFACE_WITH_INITIALIZATION (%08X) (size = %d)\n",
                        pPatchItem->Token,
                        pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tConstantBufferIndex = %d\n",
                    pPatchItem->ConstantBufferIndex );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tSurfaceStateHeapOffset = %d\n",
                    pPatchItem->Offset );
#if 0 // needed for CB2CR - need RT buy off.
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                        "\tInline CB Index = %d\n",
                        pPatchItem->InlineConstantBufferIndex );
#endif
            }
            break;

        case iOpenCL::PATCH_TOKEN_SAMPLER_KERNEL_ARGUMENT:
            {
                const iOpenCL::SPatchSamplerKernelArgument* pPatchItem =
                    (const iOpenCL::SPatchSamplerKernelArgument*)pHeader;

                const char* samplerType = nullptr;

                switch( pPatchItem->Type )
                {
                case iOpenCL::SAMPLER_OBJECT_TEXTURE:
                    samplerType = "Texture";
                    break;
                case iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_ERODE:
                    samplerType = "Erode";
                    break;
                case iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_DILATE:
                    samplerType = "Dilate";
                    break;
                case iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_MINMAXFILTER:
                    samplerType = "MinMaxFilter";
                    break;
                case iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_2DCONVOLVE:
                    samplerType = "2D Convolve";
                    break;
                case iOpenCL::SAMPLER_OBJECT_VME:
                    samplerType = "VME";
                    break;
                case iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_MINMAX:
                    samplerType = "MinMax";
                    break;
                case iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_CENTROID:
                    samplerType = "Centroid";
                    break;
                case iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_BOOL_CENTROID:
                    samplerType = "BoolCentroid";
                    break;
                case iOpenCL::SAMPLER_OBJECT_SAMPLE_8X8_BOOL_SUM:
                    samplerType = "BoolSum";
                    break;
                default:
                    samplerType = "Unknown";
                    break;
                };

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_SAMPLER_KERNEL_ARGUMENT (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tArgumentNumber = %d\n",
                    pPatchItem->ArgumentNumber );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tSamplerType = %s (%d)\n",
                    samplerType,
                    pPatchItem->Type );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tOffset = %d\n",
                    pPatchItem->Offset );
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tLocationIndex1 = %d\n",
                    pPatchItem->LocationIndex);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tLocationIndex2 = %d\n",
                    pPatchItem->LocationIndex2);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tNeedBindlessHandle = %s\n",
                    pPatchItem->needBindlessHandle ? "true" : "false");
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tIsEmulationArgument = %s\n",
                    pPatchItem->IsEmulationArgument ? "true" : "false");
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tbtiOffset = %d\n",
                    pPatchItem->btiOffset);
            }
            break;

        case iOpenCL::PATCH_TOKEN_DATA_PARAMETER_BUFFER:
            {
                const iOpenCL::SPatchDataParameterBuffer* pPatchItem =
                    (const iOpenCL::SPatchDataParameterBuffer*)pHeader;

                    ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                        "PATCH_TOKEN_DATA_PARAMETER_BUFFER (%08X) (size = %d)\n",
                        pPatchItem->Token,
                        pPatchItem->Size );

                    switch( pPatchItem->Type )
                    {
                    case iOpenCL::DATA_PARAMETER_KERNEL_ARGUMENT:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = KERNEL_ARGUMENT\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_LOCAL_WORK_SIZE:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = LOCAL_WORK_SIZE\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_GLOBAL_WORK_SIZE:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = GLOBAL_WORK_SIZE\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_NUM_WORK_GROUPS:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = NUM_WORK_GROUPS\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_WORK_DIMENSIONS:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = WORK_DIMENSIONS\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_LOCAL_ID:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = LOCAL_ID\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_RT_STACK_ID:
                        ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                            "\tType = RT_STACK_ID\n");
                        break;
                    case iOpenCL::DATA_PARAMETER_EXECUTION_MASK:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = EXECUTION_MASK\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_SUM_OF_LOCAL_MEMORY_OBJECT_ARGUMENT_SIZES:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = SUM_OF_LOCAL_MEMORY_OBJECT_ARGUMENT_SIZES\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_IMAGE_WIDTH:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = IMAGE_WIDTH\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_IMAGE_HEIGHT:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = IMAGE_HEIGHT\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_IMAGE_DEPTH:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = IMAGE_DEPTH\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_IMAGE_CHANNEL_DATA_TYPE:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = IMAGE_CHANNEL_DATA_TYPE\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_IMAGE_CHANNEL_ORDER:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = IMAGE_CHANNEL_ORDER\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_IMAGE_ARRAY_SIZE:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = IMAGE_ARRAY_SIZE\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_SAMPLER_ADDRESS_MODE:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = SAMPLER_ADDRESS_MODE\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_SAMPLER_NORMALIZED_COORDS:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = SAMPLER_NORMALIZED_COORDS\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_GLOBAL_WORK_OFFSET:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = GLOBAL_WORK_OFFSET\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_NUM_HARDWARE_THREADS:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = NUM_HARDWARE_THREADS\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_PRINTF_SURFACE_SIZE:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = PRINTF_SURFACE_SIZE\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_IMAGE_NUM_SAMPLES:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = IMAGE_NUM_SAMPLES\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_SAMPLER_COORDINATE_SNAP_WA_REQUIRED:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = SAMPLER_COORDINATE_SNAP_WA_REQUIRED\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_PARENT_EVENT:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = PARENT_EVENT\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_MAX_WORKGROUP_SIZE:
                        ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                            "\tType = MAX_WORKGROUP_SIZE\n");
                        break;
                    case iOpenCL::DATA_PARAMETER_PREFERRED_WORKGROUP_MULTIPLE:
                        ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                            "\tType = PREFERRED_WORKGROUP_MULTIPLE\n");
                        break;
                    case iOpenCL::DATA_PARAMETER_VME_MB_BLOCK_TYPE:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = VME_MB_BLOCK_TYPE\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_VME_SUBPIXEL_MODE:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = VME_SUBPIXEL_MODE\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_VME_SAD_ADJUST_MODE:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = VME_SAD_ADJUST_MODE\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_VME_SEARCH_PATH_TYPE:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = VME_SEARCH_PATH_TYPE\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_IMAGE_NUM_MIP_LEVELS:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = IMAGE_NUM_MIP_LEVELS\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_ENQUEUED_LOCAL_WORK_SIZE:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = ENQUEUED_LOCAL_WORK_SIZE\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS\n" );
                            break;
                    case iOpenCL::DATA_PARAMETER_LOCAL_MEMORY_STATELESS_WINDOW_SIZE:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = LOCAL_MEMORY_STATELESS_WINDOW_SIZE\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_PRIVATE_MEMORY_STATELESS_SIZE:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = PRIVATE_MEMORY_STATELESS_SIZE\n" );
                        break;
                    case iOpenCL::DATA_PARAMETER_OBJECT_ID:
                        ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                          "\tType = OBJECT_ID\n");
                       break;
                    case iOpenCL::DATA_PARAMETER_SIMD_SIZE:
                        ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                            "\tType = SIMD_SIZE\n");
                        break;
                    case iOpenCL::DATA_PARAMETER_CHILD_BLOCK_SIMD_SIZE:
                        ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                        "\tType = CHILD_BLOCK_SIMD_SIZE\n");
                        break;
                    case iOpenCL::DATA_PARAMETER_STAGE_IN_GRID_ORIGIN:
                        ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                        "\tType = STAGE_IN_GRID_ORIGIN\n");
                        break;
                    case iOpenCL::DATA_PARAMETER_STAGE_IN_GRID_SIZE:
                        ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                            "\tType = STAGE_IN_GRID_SIZE\n");
                        break;
                    case iOpenCL::DATA_PARAMETER_BUFFER_OFFSET:
                        ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                            "\tType = BUFFER_OFFSET\n");
                        break;
                    case iOpenCL::DATA_PARAMETER_BUFFER_STATEFUL:
                        ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                            "\tType = BUFFER_STATEFUL\n");
                        break;
                    case iOpenCL::DATA_PARAMETER_FLAT_IMAGE_BASEOFFSET:
                        ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                            "\tType = FLAT_IMAGE_BASEOFFSET\n");
                        break;
                    case iOpenCL::DATA_PARAMETER_FLAT_IMAGE_HEIGHT:
                        ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                            "\tType = FLAT_IMAGE_HEIGHT\n");
                        break;
                    case iOpenCL::DATA_PARAMETER_FLAT_IMAGE_WIDTH:
                        ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                            "\tType = FLAT_IMAGE_WIDTH\n");
                        break;
                    case iOpenCL::DATA_PARAMETER_FLAT_IMAGE_PITCH:
                        ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                            "\tType = FLAT_IMAGE_PITCH\n");
                        break;
                    case iOpenCL::DATA_PARAMETER_IMPL_ARG_BUFFER:
                        ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                            "\tType = IMPL_ARG_BUFFER\n");
                        break;
                    default:
                        ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                            "\tType = UNKNOWN_TYPE\n" );
                        break;
                    }

                    ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                        "\tArgumentNumber = %d\n",
                        pPatchItem->ArgumentNumber );
                    ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                        "\tOffset = %d\n",
                        pPatchItem->Offset );
                    ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                        "\tDataSize = %d\n",
                        pPatchItem->DataSize );
                    ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                        "\tSourceOffset = %d\n",
                        pPatchItem->SourceOffset );
                    ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                        "\tLocationIndex1 = %d\n",
                        pPatchItem->LocationIndex );
                    ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                        "\tLocationIndex2 = %d\n",
                        pPatchItem->LocationIndex2 );
                    ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                        "\tIsEmulationArgument = %s\n",
                        pPatchItem->IsEmulationArgument ? "true" : "false");
            }
            break;

        // Gen5.75 and Gen6 Allocation Tokens

        case iOpenCL::PATCH_TOKEN_ALLOCATE_LOCAL_SURFACE:
            {
                const iOpenCL::SPatchAllocateLocalSurface*  pPatchItem =
                    ( const iOpenCL::SPatchAllocateLocalSurface* )pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_ALLOCATE_LOCAL_SURFACE (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tOffset = %d\n",
                    pPatchItem->Offset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tTotalInlineLocalMemorySize = %d\n",
                    pPatchItem->TotalInlineLocalMemorySize );
            }
            break;

        case iOpenCL::PATCH_TOKEN_ALLOCATE_SCRATCH_SURFACE:
            {
                const iOpenCL::SPatchAllocateScratchSurface*  pPatchItem =
                    ( const iOpenCL::SPatchAllocateScratchSurface* )pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_ALLOCATE_SCRATCH_SURFACE (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tOffset = %d\n",
                    pPatchItem->Offset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tPerThreadScratchSpaceSize = %d\n",
                    pPatchItem->PerThreadScratchSpaceSize );
            }
            break;

        case iOpenCL::PATCH_TOKEN_ALLOCATE_PRIVATE_MEMORY:
             {
                const iOpenCL::SPatchAllocatePrivateMemorySurface*  pPatchItem =
                    ( const iOpenCL::SPatchAllocatePrivateMemorySurface* )pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_ALLOCATE_PRIVATE_MEMORY (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tOffset = %d\n",
                    pPatchItem->Offset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tPerThreadPrivateMemorySize = %d\n",
                    pPatchItem->PerThreadPrivateMemorySize );
            }
            break;

        case iOpenCL::PATCH_TOKEN_ALLOCATE_SIP_SURFACE:
            {
                const iOpenCL::SPatchAllocateSystemThreadSurface* pPatchItem =
                    ( const iOpenCL::SPatchAllocateSystemThreadSurface* )pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_ALLOCATE_SIP_SURFACE (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tOffset = %d\n",
                    pPatchItem->Offset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tPerThreadSystemThreadSurfaceSize = %d\n",
                    pPatchItem->PerThreadSystemThreadSurfaceSize );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tBTI = %d\n",
                    pPatchItem->BTI );
            }
            break;

        // Gen5.75 and Gen6 State Programming
        case iOpenCL::PATCH_TOKEN_STATE_SIP:
            {
                const iOpenCL::SPatchStateSIP* pPatchItem =
                    (const iOpenCL::SPatchStateSIP*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_STATE_SIP (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tSystemKernelOffset = %d\n",
                    pPatchItem->SystemKernelOffset );
            }
            break;

        case iOpenCL::PATCH_TOKEN_SAMPLER_STATE_ARRAY:
            {
                const iOpenCL::SPatchSamplerStateArray* pPatchItem =
                    (const iOpenCL::SPatchSamplerStateArray*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_SAMPLER_STATE_ARRAY (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tOffset = %d\n",
                    pPatchItem->Offset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tCount = %d\n",
                    pPatchItem->Count );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tBorderColorOffset = %d\n",
                    pPatchItem->BorderColorOffset );
            }
            break;

        case iOpenCL::PATCH_TOKEN_BINDING_TABLE_STATE:
            {
                const iOpenCL::SPatchBindingTableState* pPatchItem =
                    (const iOpenCL::SPatchBindingTableState*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_BINDING_TABLE_STATE (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tOffset = %d\n",
                    pPatchItem->Offset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tCount = %d\n",
                    pPatchItem->Count );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tSurfaceStateOffset = %d\n",
                    pPatchItem->SurfaceStateOffset );
            }
            break;

        // Gen6 State Programming
        case iOpenCL::PATCH_TOKEN_MEDIA_VFE_STATE:
            {
                const iOpenCL::SPatchMediaVFEState* pPatchItem =
                    (const iOpenCL::SPatchMediaVFEState*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_MEDIA_VFE_STATE (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tScratchSpaceOffset = %d\n",
                    pPatchItem->ScratchSpaceOffset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tPerThreadScratchSpace = %d\n",
                    pPatchItem->PerThreadScratchSpace );
            }
            break;

        case iOpenCL::PATCH_TOKEN_MEDIA_VFE_STATE_SLOT1:
        {
            const iOpenCL::SPatchMediaVFEState* pPatchItem =
                (const iOpenCL::SPatchMediaVFEState*)pHeader;

            ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                "PATCH_TOKEN_MEDIA_VFE_STATE_SLOT1 (%08X) (size = %d)\n",
                pPatchItem->Token,
                pPatchItem->Size);
            ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                "\tScratchSpaceOffset = %d\n",
                pPatchItem->ScratchSpaceOffset);
            ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                "\tPerThreadScratchSpaceSlot1 = %d\n",
                pPatchItem->PerThreadScratchSpace);
        }
        break;

        case iOpenCL::PATCH_TOKEN_MEDIA_INTERFACE_DESCRIPTOR_LOAD:
            {
                const iOpenCL::SPatchMediaInterfaceDescriptorLoad* pPatchItem =
                    (const iOpenCL::SPatchMediaInterfaceDescriptorLoad*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_MEDIA_INTERFACE_DESCRIPTOR_LOAD (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tInterfaceDescriptorDataOffset = %d\n",
                    pPatchItem->InterfaceDescriptorDataOffset );
            }
            break;

        case iOpenCL::PATCH_TOKEN_INTERFACE_DESCRIPTOR_DATA:
            {
                const iOpenCL::SPatchInterfaceDescriptorData* pPatchItem =
                    (const iOpenCL::SPatchInterfaceDescriptorData*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_INTERFACE_DESCRIPTOR_DATA (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tOffset = %d\n",
                    pPatchItem->Offset);
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tSamplerStateOffset = %d\n",
                    pPatchItem->SamplerStateOffset);
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tKernelOffset = %d\n",
                    pPatchItem->KernelOffset);
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tBindingTableOffset = %d\n",
                    pPatchItem->BindingTableOffset);
            }
            break;

            case iOpenCL::PATCH_TOKEN_THREAD_PAYLOAD:
            {
                const iOpenCL::SPatchThreadPayload* pPatchItem =
                    (const iOpenCL::SPatchThreadPayload*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_THREAD_PAYLOAD (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tHeaderPresent = %s\n",
                    pPatchItem->HeaderPresent ? "true" : "false" );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tLocalIDXPresent = %s\n",
                    pPatchItem->LocalIDXPresent ? "true" : "false" );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tLocalIDYPresent = %s\n",
                    pPatchItem->LocalIDYPresent ? "true" : "false" );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tLocalIDZPresent = %s\n",
                    pPatchItem->LocalIDZPresent ? "true" : "false" );
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tRTStackIDPresent = %s\n",
                    pPatchItem->RTStackIDPresent ? "true" : "false");
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tLocalIDFlattenedPresent = %s\n",
                    pPatchItem->LocalIDFlattenedPresent ? "true" : "false" );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tIndirectPayloadStorage = %s\n",
                    pPatchItem->IndirectPayloadStorage ? "true" : "false" );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tUnusedPerThreadConstantPresent = %s\n",
                    pPatchItem->UnusedPerThreadConstantPresent ? "true" : "false" );
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tGetLocalIDPresent = %s\n",
                    pPatchItem->GetLocalIDPresent ? "true" : "false");
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tGetGlobalOffsetPresent = %s\n",
                    pPatchItem->GetGlobalOffsetPresent ? "true" : "false");
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tGetGroupIDPresent = %s\n",
                    pPatchItem->GetGroupIDPresent ? "true" : "false");
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tStageInGridOriginPresent = %s\n",
                    pPatchItem->StageInGridOriginPresent ? "true" : "false");
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tStageInGridSizePresent = %s\n",
                    pPatchItem->StageInGridSizePresent ? "true" : "false");
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tOffsetToSkipPerThreadDataLoad = %d\n",
                    pPatchItem->OffsetToSkipPerThreadDataLoad);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                  "\tOffsetToSkipSetFFIDGP = %d\n",
                  pPatchItem->OffsetToSkipSetFFIDGP);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tPassInlineData = %s\n",
                    pPatchItem->PassInlineData ? "true" : "false");
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tgenerateLocalID = %s\n",
                    pPatchItem->generateLocalID ? "true" : "false");
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\temitLocalMask = %d\n",
                    pPatchItem->emitLocalMask);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\twalkOrder = %d\n",
                    pPatchItem->walkOrder);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\ttileY = %s\n",
                    pPatchItem->tileY ? "true" : "false");
            }
            break;

        case iOpenCL::PATCH_TOKEN_EXECUTION_ENVIRONMENT:
            {
                const iOpenCL::SPatchExecutionEnvironment* pPatchItem =
                    (const iOpenCL::SPatchExecutionEnvironment*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_EXECUTION_ENVIRONMENT (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tRequiredWorkGroupSizeX = %d\n",
                    pPatchItem->RequiredWorkGroupSizeX );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tRequiredWorkGroupSizeY = %d\n",
                    pPatchItem->RequiredWorkGroupSizeY );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tRequiredWorkGroupSizeZ = %d\n",
                    pPatchItem->RequiredWorkGroupSizeZ );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tLargestCompiledSIMDSize = %d\n",
                    pPatchItem->LargestCompiledSIMDSize );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tHasBarriers = %d\n",
                    pPatchItem->HasBarriers);
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDisableMidThreadPreemption = %s\n",
                    pPatchItem->DisableMidThreadPreemption ?
                    "true" : "false" );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tCompiledSIMD8 = %s\n",
                    pPatchItem->CompiledSIMD8 ? "true" : "false" );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tCompiledSIMD16 = %s\n",
                    pPatchItem->CompiledSIMD16 ? "true" : "false" );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tCompiledSIMD32 = %s\n",
                    pPatchItem->CompiledSIMD32 ? "true" : "false" );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tHasDeviceEnqueue = %s\n",
                    pPatchItem->HasDeviceEnqueue ? "true" : "false" );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tMayAccessUndeclaredResource = %s\n",
                    pPatchItem->MayAccessUndeclaredResource ? "true" : "false" );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tUsesFencesForReadWriteImages = %s\n",
                    pPatchItem->UsesFencesForReadWriteImages ? "true" : "false" );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tUsesStatelessSpillFill = %s\n",
                    pPatchItem->UsesStatelessSpillFill ? "true" : "false" );
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tUsesMultiScratchSpaces = %s\n",
                    pPatchItem->UsesMultiScratchSpaces ? "true" : "false");
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tIsCoherent = %s\n",
                    pPatchItem->IsCoherent ? "true" : "false" );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tSubgroupIndependentForwardProgressRequired = %s\n",
                    pPatchItem->SubgroupIndependentForwardProgressRequired ? "true" : "false" );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tCompiledSubGroupsNumber = %d\n",
                    pPatchItem->CompiledSubGroupsNumber );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tCompiledForGreaterThan4GBBuffers = %s\n",
                    pPatchItem->CompiledForGreaterThan4GBBuffers ? "true" : "false"  );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tHasGlobalAtomics = %s\n",
                    pPatchItem->HasGlobalAtomics ? "true" : "false");
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tNumGRFRequired = %d\n",
                    pPatchItem->NumGRFRequired);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tHasDPAS = %s\n",
                    pPatchItem->HasDPAS ? "true" : "false");
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tHasRTCalls = %s\n",
                    pPatchItem->HasRTCalls ? "true" : "false");
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tNumThreadsRequired = %d\n",
                    pPatchItem->NumThreadsRequired);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tNumStatelessWrites = %d\n",
                    pPatchItem->StatelessWritesCount);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tNumIndirectStateless = %d\n",
                    pPatchItem->IndirectStatelessCount);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tUseBindlessMode = %d\n",
                    pPatchItem->UseBindlessMode);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tHasStackCalls = %d\n",
                    pPatchItem->HasStackCalls);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tSIMDInfo = %lld\n",
                    pPatchItem->SIMDInfo);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tRequireDisableEUFusion = %s\n",
                    pPatchItem->RequireDisableEUFusion ? "true" : "false");
            }
            break;

        case iOpenCL::PATCH_TOKEN_DATA_PARAMETER_STREAM:
            {
                const iOpenCL::SPatchDataParameterStream* pPatchItem =
                    (const iOpenCL::SPatchDataParameterStream*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_DATA_PARAMETER_STREAM (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDataParameterStreamSize = %d\n",
                    pPatchItem->DataParameterStreamSize);
            }
            break;

        case iOpenCL::PATCH_TOKEN_KERNEL_ATTRIBUTES_INFO:
            {
                const iOpenCL::SPatchKernelAttributesInfo* pPatchItem =
                    (const iOpenCL::SPatchKernelAttributesInfo*)pHeader;

                const char* pStr = (const char*)pHeader + sizeof(iOpenCL::SPatchKernelAttributesInfo);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_KERNEL_ATTRIBUTES_INFO (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tAttributeSize        = %3d | value = \"%s\"\n",
                    pPatchItem->AttributesSize,
                    pStr );
            }
            break;

        case iOpenCL::PATCH_TOKEN_KERNEL_ARGUMENT_INFO:
            {
                const iOpenCL::SPatchKernelArgumentInfo* pPatchItem =
                    (const iOpenCL::SPatchKernelArgumentInfo*)pHeader;

                const char* pStr = (const char*)pHeader + sizeof(iOpenCL::SPatchKernelArgumentInfo);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_KERNEL_ARGUMENT_INFO (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tArgumentNumber       = %3d\n",
                    pPatchItem->ArgumentNumber );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tAddressQualifierSize = %3d | value = \"%s\"\n",
                    pPatchItem->AddressQualifierSize,
                    pStr );
                pStr += pPatchItem->AddressQualifierSize;
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tAccessQualifierSize  = %3d | value = \"%s\"\n",
                    pPatchItem->AccessQualifierSize,
                    pStr );
                pStr += pPatchItem->AccessQualifierSize;
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tArgumentNameSize     = %3d | value = \"%s\"\n",
                    pPatchItem->ArgumentNameSize,
                    pStr );
                pStr += pPatchItem->ArgumentNameSize;
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tTypeNameSize         = %3d | value = \"%s\"\n",
                    pPatchItem->TypeNameSize,
                    pStr );
                pStr += pPatchItem->TypeNameSize;
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tTypeQualifierSize    = %3d | value = \"%s\"\n",
                    pPatchItem->TypeQualifierSize,
                    pStr );
                pStr += pPatchItem->TypeQualifierSize;
            }
            break;

        case iOpenCL::PATCH_TOKEN_STRING:
            {
                const iOpenCL::SPatchString* pPatchItem =
                    (const iOpenCL::SPatchString*)pHeader;

                const char* pStr = (const char*)pHeader + sizeof(iOpenCL::SPatchString);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_STRING (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tString Index = %d\n",
                    pPatchItem->Index);
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tString Size = %d | value = \"%s\"\n",
                    pPatchItem->StringSize,
                    pStr );
            }
            break;

        case iOpenCL::PATCH_TOKEN_ALLOCATE_PRINTF_SURFACE:
             {
                const iOpenCL::SPatchAllocatePrintfSurface*  pPatchItem =
                    ( const iOpenCL::SPatchAllocatePrintfSurface* )pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_ALLOCATE_PRINTF_SURFACE (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tPrintfSurfaceIndex = %d\n",
                    pPatchItem->PrintfSurfaceIndex );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tOffset = %d\n",
                    pPatchItem->Offset );
            }
            break;

        case iOpenCL::PATCH_TOKEN_ALLOCATE_SYNC_BUFFER:
            {
                const iOpenCL::SPatchAllocateSyncBuffer* pPatchItem =
                    (const iOpenCL::SPatchAllocateSyncBuffer*)pHeader;

                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_ALLOCATE_SYNC_BUFFER (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tSurfaceStateHeapOffset = %d\n",
                    pPatchItem->SurfaceStateHeapOffset);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tDataParamOffset = %d\n",
                    pPatchItem->DataParamOffset);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tDataParamSize = %d\n",
                    pPatchItem->DataParamSize);
            }
            break;
        case iOpenCL::PATCH_TOKEN_ALLOCATE_RT_GLOBAL_BUFFER:
            {
                const iOpenCL::SPatchAllocateRTGlobalBuffer* pPatchItem =
                    (const iOpenCL::SPatchAllocateRTGlobalBuffer*)pHeader;

                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_ALLOCATE_RT_GLOBAL_BUFFER (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tSurfaceStateHeapOffset = %d\n",
                    pPatchItem->SurfaceStateHeapOffset);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tDataParamOffset = %d\n",
                    pPatchItem->DataParamOffset);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tDataParamSize = %d\n",
                    pPatchItem->DataParamSize);
            }
            break;

        // Stateless Tokens
        case iOpenCL::PATCH_TOKEN_STATELESS_GLOBAL_MEMORY_OBJECT_KERNEL_ARGUMENT:
            {
                const iOpenCL::SPatchStatelessGlobalMemoryObjectKernelArgument*  pPatchItem =
                    (const iOpenCL::SPatchStatelessGlobalMemoryObjectKernelArgument*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_STATELESS_GLOBAL_MEMORY_OBJECT_KERNEL_ARGUMENT (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tArgumentNumber = %d\n",
                    pPatchItem->ArgumentNumber );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tSurfaceStateHeapOffset = %d\n",
                    pPatchItem->SurfaceStateHeapOffset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDataParamOffset = %d\n",
                    pPatchItem->DataParamOffset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDataParamSize = %d\n",
                    pPatchItem->DataParamSize );
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tLocationIndex = %d\n",
                    pPatchItem->LocationIndex);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tLocationIndex2 = %d\n",
                    pPatchItem->LocationIndex2);
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tIsEmulationArgument = %s\n",
                    pPatchItem->IsEmulationArgument ? "true" : "false");
            }
            break;

        case iOpenCL::PATCH_TOKEN_STATELESS_CONSTANT_MEMORY_OBJECT_KERNEL_ARGUMENT:
            {
                const iOpenCL::SPatchStatelessConstantMemoryObjectKernelArgument*  pPatchItem =
                    (const iOpenCL::SPatchStatelessConstantMemoryObjectKernelArgument*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_STATELESS_CONSTANT_MEMORY_OBJECT_KERNEL_ARGUMENT (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tArgumentNumber = %d\n",
                    pPatchItem->ArgumentNumber );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tSurfaceStateHeapOffset = %d\n",
                    pPatchItem->SurfaceStateHeapOffset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDataParamOffset = %d\n",
                    pPatchItem->DataParamOffset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDataParamSize = %d\n",
                    pPatchItem->DataParamSize );
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tIsEmulationArgument = %s\n",
                    pPatchItem->IsEmulationArgument ? "true" : "false");
            }
            break;

        case iOpenCL::PATCH_TOKEN_ALLOCATE_STATELESS_GLOBAL_MEMORY_SURFACE_WITH_INITIALIZATION:
            {
                const iOpenCL::SPatchAllocateStatelessGlobalMemorySurfaceWithInitialization* pPatchItem =
                    (const iOpenCL::SPatchAllocateStatelessGlobalMemorySurfaceWithInitialization*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_ALLOCATE_STATELESS_GLOBAL_MEMORY_SURFACE_WITH_INITIALIZATION (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tIndex = %d\n",
                    pPatchItem->GlobalBufferIndex );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tSurfaceStateHeapOffset = %d\n",
                    pPatchItem->SurfaceStateHeapOffset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDataParamOffset = %d\n",
                    pPatchItem->DataParamOffset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDataParamSize = %d\n",
                    pPatchItem->DataParamSize );
            }
            break;

        case iOpenCL::PATCH_TOKEN_ALLOCATE_STATELESS_CONSTANT_MEMORY_SURFACE_WITH_INITIALIZATION:
            {
                const iOpenCL::SPatchAllocateStatelessConstantMemorySurfaceWithInitialization* pPatchItem =
                    (const iOpenCL::SPatchAllocateStatelessConstantMemorySurfaceWithInitialization*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_ALLOCATE_STATELESS_CONSTANT_MEMORY_SURFACE_WITH_INITIALIZATION (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tIndex = %d\n",
                    pPatchItem->ConstantBufferIndex );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tSurfaceStateHeapOffset = %d\n",
                    pPatchItem->SurfaceStateHeapOffset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDataParamOffset = %d\n",
                    pPatchItem->DataParamOffset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDataParamSize = %d\n",
                    pPatchItem->DataParamSize );
            }
            break;

        case iOpenCL::PATCH_TOKEN_ALLOCATE_STATELESS_PRINTF_SURFACE:
            {
                const iOpenCL::SPatchAllocateStatelessPrintfSurface*  pPatchItem =
                    (const iOpenCL::SPatchAllocateStatelessPrintfSurface*)pHeader;

                 ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_ALLOCATE_STATELESS_PRINTF_SURFACE (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tPrintfSurfaceIndex = %d\n",
                    pPatchItem->PrintfSurfaceIndex );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tSurfaceStateHeapOffset = %d\n",
                    pPatchItem->SurfaceStateHeapOffset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDataParamOffset = %d\n",
                    pPatchItem->DataParamOffset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDataParamSize = %d\n",
                    pPatchItem->DataParamSize );
            }
            break;

        case iOpenCL::PATCH_TOKEN_ALLOCATE_STATELESS_PRIVATE_MEMORY:
            {
                const iOpenCL::SPatchAllocateStatelessPrivateSurface* pPatchItem =
                    (const iOpenCL::SPatchAllocateStatelessPrivateSurface*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_ALLOCATE_STATELESS_PRIVATE_MEMORY (%08X) (size = %d )\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tSurfaceHeapOffset = %d\n",
                    pPatchItem->SurfaceStateHeapOffset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDataParamOffset = %d\n",
                    pPatchItem->DataParamOffset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDataParamSize = %d\n",
                    pPatchItem->DataParamSize );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tPerThreadPrivateMemorySize= %d\n",
                    pPatchItem->PerThreadPrivateMemorySize );
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tIsSimtThread= %d\n",
                    pPatchItem->IsSimtThread);
            }
            break;

        case iOpenCL::PATCH_TOKEN_CB_MAPPING:
            {
                const iOpenCL::SPatchConstantBufferMapping*  pPatchItem =
                    ( const iOpenCL::SPatchConstantBufferMapping* )pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_CB_MAPPING (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tConstantBuffeType = %s (%d)\n",
                    pPatchItem->ConstantBufferType == iOpenCL::CONSTANT_BUFFER_TYPE_KERNEL_ARGUMENT ?
                    "CONSTANT_BUFFER_TYPE_KERNEL_ARGUMENT" :
                    "CONSTANT_BUFFER_TYPE_INLINE",
                        pPatchItem->ConstantBufferType );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tConstantBufferIndex =  %d\n",
                    pPatchItem->ConstantBufferIndex );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tConstantBufferId =  %d\n",
                    pPatchItem->ConstantBufferId );
            }
            break;

        case iOpenCL::PATCH_TOKEN_CB2CR_GATHER_TABLE:
            {
#if 0
                const iOpenCL::SPatchCB2CRGatherTable*  pPatchItem =
                    ( const iOpenCL::SPatchCB2CRGatherTable* )pHeader;

                const USC::SConstantGatherEntry* pTable =
                    ( const USC::SConstantGatherEntry* )( pPatchItem + 1 );

                DWORD i = 0;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_CB2CR_GATHER_TABLE (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tNumberOfEntries = %d\n",
                    pPatchItem->NumberOfEntries );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tOffset = %d\n",
                    pPatchItem->Offset );

                for( i = 0; i < pPatchItem->NumberOfEntries; i++ )
                {
                    ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                        "\tEntry Number: %d\n",
                        i );
                    ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                        "\t\tConstantBufferIndex = %d\n",
                        pTable[i].GatherEntry.Fields.constantBufferIndex );
                    ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                        "\t\tChannelMask = 0x%X\n",
                        pTable[i].GatherEntry.Fields.channelMask );
                    ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                        "\t\tConstantBufferOffset = %d\n",
                        pTable[i].GatherEntry.Fields.constantBufferOffset );
                }
#endif
            }
            break;

        case iOpenCL::PATCH_TOKEN_ALLOCATE_STATELESS_EVENT_POOL_SURFACE:
            {
                const iOpenCL::SPatchAllocateStatelessEventPoolSurface*  pPatchItem =
                    ( const iOpenCL::SPatchAllocateStatelessEventPoolSurface* )pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_ALLOCATE_STATELESS_EVENT_POOL_SURFACE (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tEventPooolSurfaceIndex = %d\n",
                    pPatchItem->EventPoolSurfaceIndex );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tSurfaceStateHeapOffset = %d\n",
                    pPatchItem->SurfaceStateHeapOffset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDataParamOffset = %d\n",
                    pPatchItem->DataParamOffset );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tDataParamSize = %d\n",
                    pPatchItem->DataParamSize );

            }
            break;

        case iOpenCL::PATCH_TOKEN_ALLOCATE_STATELESS_DEFAULT_DEVICE_QUEUE_SURFACE:
        {
            const iOpenCL::SPatchAllocateStatelessDefaultDeviceQueueSurface*  pPatchItem =
                (const iOpenCL::SPatchAllocateStatelessDefaultDeviceQueueSurface*)pHeader;

            ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                "PATCH_TOKEN_ALLOCATE_STATELESS_DEFAULT_DEVICE_QUEUE_SURFACE (%08X) (size = %d)\n",
                pPatchItem->Token,
                pPatchItem->Size);
            ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                "\tSurfaceStateHeapOffset = %d\n",
                pPatchItem->SurfaceStateHeapOffset);
            ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                "\tDataParamOffset = %d\n",
                pPatchItem->DataParamOffset);
            ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                "\tDataParamSize = %d\n",
                pPatchItem->DataParamSize);

        }
        break;

        case iOpenCL::PATCH_TOKEN_STATELESS_DEVICE_QUEUE_KERNEL_ARGUMENT:
        {
            const iOpenCL::SPatchStatelessDeviceQueueKernelArgument*  pPatchItem =
                (const iOpenCL::SPatchStatelessDeviceQueueKernelArgument*)pHeader;

            ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                "PATCH_TOKEN_STATELESS_DEVICE_QUEUE_KERNEL_ARGUMENT (%08X) (size = %d)\n",
                pPatchItem->Token,
                pPatchItem->Size);
            ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                "\tArgumentNumber = %d\n",
                pPatchItem->ArgumentNumber);
            ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                "\tSurfaceStateHeapOffset = %d\n",
                pPatchItem->SurfaceStateHeapOffset);
            ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                "\tDataParamOffset = %d\n",
                pPatchItem->DataParamOffset);
            ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                "\tDataParamSize = %d\n",
                pPatchItem->DataParamSize);
            ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                "\tIsEmulationArgument = %s\n",
                pPatchItem->IsEmulationArgument ? "true" : "false");
        }
            break;

        case iOpenCL::PATCH_TOKEN_NULL_SURFACE_LOCATION:
            {
                const iOpenCL::SPatchNullSurfaceLocation*  pPatchItem =
                    ( const iOpenCL::SPatchNullSurfaceLocation* )pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_NULL_SURFACE_LOCATION (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tOffset = %d\n",
                    pPatchItem->Offset );
            }
            break;

        case iOpenCL::PATCH_TOKEN_CONSTRUCTOR_DESTRUCTOR_KERNEL_PROGRAM_BINARY_INFO:
            {
                const iOpenCL::SPatchKernelTypeProgramBinaryInfo* pPatchItem =
                    (const iOpenCL::SPatchKernelTypeProgramBinaryInfo*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_CONSTRUCTOR_DESTRUCTOR_KERNEL_PROGRAM_BINARY_INFO (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                const char* kType;
                if (pPatchItem->Type)
                    kType = "Destructor";
                else
                    kType = "Constructor";
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tType = %s\n",
                    kType);
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tInitializationDataSize = %d\n",
                    pPatchItem->InlineDataSize );
            }
            break;
        case iOpenCL::PATCH_TOKEN_ALLOCATE_CONSTANT_MEMORY_SURFACE_PROGRAM_BINARY_INFO:
            {
                const iOpenCL::SPatchAllocateConstantMemorySurfaceProgramBinaryInfo* pPatchItem =
                    (const iOpenCL::SPatchAllocateConstantMemorySurfaceProgramBinaryInfo*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_ALLOCATE_CONSTANT_MEMORY_SURFACE_PROGRAM_BINARY_INFO (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tBufferIndex = %d\n",
                    pPatchItem->ConstantBufferIndex );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tInitializationDataSize = %d\n",
                    pPatchItem->InlineDataSize );
            }
            break;

        case iOpenCL::PATCH_TOKEN_ALLOCATE_GLOBAL_MEMORY_SURFACE_PROGRAM_BINARY_INFO:
            {
                const iOpenCL::SPatchAllocateGlobalMemorySurfaceProgramBinaryInfo* pPatchItem =
                    (const iOpenCL::SPatchAllocateGlobalMemorySurfaceProgramBinaryInfo*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_ALLOCATE_GLOBAL_MEMORY_SURFACE_PROGRAM_BINARY_INFO (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tBufferType = %d\n",
                    pPatchItem->Type );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tBufferIndex = %d\n",
                    pPatchItem->GlobalBufferIndex );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tInitializationDataSize = %d\n",
                    pPatchItem->InlineDataSize );
            }
            break;

        case iOpenCL::PATCH_TOKEN_GLOBAL_POINTER_PROGRAM_BINARY_INFO:
            {
                const iOpenCL::SPatchGlobalPointerProgramBinaryInfo* pPatchItem =
                    (const iOpenCL::SPatchGlobalPointerProgramBinaryInfo*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_GLOBAL_POINTER_PROGRAM_BINARY_INFO (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tGlobalBufferIndex = %d\n",
                    pPatchItem->GlobalBufferIndex);
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tGlobalPointerOffset = %" PRIu64 "\n",
                    pPatchItem->GlobalPointerOffset);
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tBufferType = %d\n",
                    pPatchItem->BufferType);
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tBufferIndex = %d\n",
                    pPatchItem->BufferIndex);
            }
            break;

        case iOpenCL::PATCH_TOKEN_CONSTANT_POINTER_PROGRAM_BINARY_INFO:
            {
                const iOpenCL::SPatchConstantPointerProgramBinaryInfo* pPatchItem =
                    (const iOpenCL::SPatchConstantPointerProgramBinaryInfo*)pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_CONSTANT_POINTER_PROGRAM_BINARY_INFO (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tConstantBufferIndex = %d\n",
                    pPatchItem->ConstantBufferIndex);
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tConstantPointerOffset = %" PRIu64 "\n",
                    pPatchItem->ConstantPointerOffset);
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tBufferType = %d\n",
                    pPatchItem->BufferType);
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "\tBufferIndex = %d\n",
                    pPatchItem->BufferIndex);
            }
            break;

        case iOpenCL::PATCH_TOKEN_INLINE_VME_SAMPLER_INFO:
            {
                const iOpenCL::SPatchInlineVMESamplerInfo*  pPatchItem =
                    ( const iOpenCL::SPatchInlineVMESamplerInfo* )pHeader;

                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_INLINE_VME_SAMPLER_INFO (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size );
            }
            break;

        case iOpenCL::PATCH_TOKEN_GTPIN_FREE_GRF_INFO:
            {
                const iOpenCL::SPatchGtpinFreeGRFInfo* pPatchItem =
                    (const iOpenCL::SPatchGtpinFreeGRFInfo*)pHeader;

                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "PATCH_TOKEN_GTPIN_FREE_GRF_INFO (%08X) (size = %d)\n",
                    pPatchItem->Token,
                    pPatchItem->Size);

                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tBufferSize = %d\n",
                    pPatchItem->BufferSize);
            }
            break;
        case iOpenCL::PATCH_TOKEN_GTPIN_INFO:
            {
                const iOpenCL::SPatchItemHeader* pPatchItem = pHeader;

                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                             "PATCH_TOKEN_GTPIN_INFO (%08X) (size = %d)\n",
                             pPatchItem->Token,
                             pPatchItem->Size);
            }
            break;
        case iOpenCL::PATCH_TOKEN_PROGRAM_SYMBOL_TABLE:
            {
                const iOpenCL::SPatchFunctionTableInfo* pPatchItem =
                    (const iOpenCL::SPatchFunctionTableInfo*)pHeader;

                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                             "PATCH_TOKEN_PROGRAM_SYMBOL_TABLE (%08X) (size = %d)\n",
                             pPatchItem->Token,
                             pPatchItem->Size);

                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                             "\tNumEntries = %d\n",
                             pPatchItem->NumEntries);

                const vISA::GenSymEntry* entryPtr = (const vISA::GenSymEntry*) (ptr + sizeof(iOpenCL::SPatchFunctionTableInfo));
                for (unsigned i = 0; i < pPatchItem->NumEntries; i++)
                {
                    ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                        "\tSymbol(Entry %02d): %s\n\t  Offset = %d\n\t  Size = %d\n\t  Type = %d\n", i,
                        entryPtr->s_name,
                        entryPtr->s_offset,
                        entryPtr->s_size,
                        entryPtr->s_type);

                    entryPtr++;
                }
            }
            break;
        case iOpenCL::PATCH_TOKEN_PROGRAM_RELOCATION_TABLE:
            {
                const iOpenCL::SPatchFunctionTableInfo* pPatchItem =
                    (const iOpenCL::SPatchFunctionTableInfo*)pHeader;

                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                             "PATCH_TOKEN_PROGRAM_RELOCATION_TABLE (%08X) (size = %d)\n",
                             pPatchItem->Token,
                             pPatchItem->Size);

                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                             "\tNumEntries = %d\n",
                             pPatchItem->NumEntries);

                const vISA::GenRelocEntry* entryPtr = (const vISA::GenRelocEntry*) (ptr + sizeof(iOpenCL::SPatchFunctionTableInfo));
                for (unsigned i = 0; i < pPatchItem->NumEntries; i++)
                {
                    ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                        "\tRelocSymbol(Entry %02d): %s\n\t  Offset = %d\n\t  Type = %d\n", i,
                        entryPtr->r_symbol,
                        entryPtr->r_offset,
                        entryPtr->r_type);

                    entryPtr++;
                }
            }
            break;
        case iOpenCL::PATCH_TOKEN_GLOBAL_HOST_ACCESS_TABLE:
        {
            const iOpenCL::SPatchGlobalHostAccessTableInfo* pPatchItem =
                (const iOpenCL::SPatchGlobalHostAccessTableInfo*)pHeader;

            ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                "PATCH_TOKEN_GLOBAL_HOST_ACCESS_TABLE (%08X) (size = %d)\n",
                pPatchItem->Token,
                pPatchItem->Size);

            ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                "\tNumEntries = %d\n",
                pPatchItem->NumEntries);

            const vISA::HostAccessEntry* entryPtr = (const vISA::HostAccessEntry*)(ptr + sizeof(iOpenCL::SPatchFunctionTableInfo));
            for (unsigned i = 0; i < pPatchItem->NumEntries; i++)
            {
                ICBE_DPF_STR(output, GFXDBG_HARDWARE,
                    "\tSymbol(Entry %02d): \n\t  DeviceName = %s\n\t  HostName = %s\n", i,
                    entryPtr->device_name,
                    entryPtr->host_name);

                entryPtr++;
            }
        }
        break;

        default:
            {
                IGC_ASSERT(0);
                ICBE_DPF_STR( output, GFXDBG_HARDWARE,
                    "*** UNKNOWN TOKEN %08X (size = %d)***\n",
                    pHeader->Token,
                    pHeader->Size );
            }
            break;
        }

        ptr += pHeader->Size;
        remaining -= pHeader->Size;
    }
}

} // namespace
