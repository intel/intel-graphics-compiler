/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include <stdint.h>
#include "patch_list.h"

#pragma pack( push, 1 )

namespace iOpenCL
{
/*****************************************************************************\
STRUCT: SPatchStateSIP
\*****************************************************************************/
struct SPatchStateSIP :
       SPatchItemHeader
{
    uint32_t   SystemKernelOffset;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchStateSIP ) == ( 4 + sizeof( SPatchItemHeader ) ) , "The size of SPatchStateSIP is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchSamplerStateArray
\*****************************************************************************/
struct SPatchSamplerStateArray :
       SPatchItemHeader
{
    uint32_t   Offset;
    uint32_t   Count;
    uint32_t   BorderColorOffset;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchSamplerStateArray ) == ( 12 + sizeof( SPatchItemHeader ) ) , "The size of SPatchSamplerStateArray is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchBindingTableState
\*****************************************************************************/
struct SPatchBindingTableState :
       SPatchItemHeader
{
    uint32_t   Offset;
    uint32_t   Count;
    uint32_t   SurfaceStateOffset;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchBindingTableState ) == ( 12 + sizeof( SPatchItemHeader ) ) , "The size of SPatchBindingTableState is not what is expected" );


/*****************************************************************************\
STRUCT: SPatchAllocateScratchSurface
\*****************************************************************************/
struct SPatchAllocateScratchSurface :
       SPatchItemHeader
{
    uint32_t   Offset;
    uint32_t   PerThreadScratchSpaceSize;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateScratchSurface ) == ( 8 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocateScratchSurface is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchAllocatePrivateMemorySurface
\*****************************************************************************/
struct SPatchAllocatePrivateMemorySurface :
       SPatchItemHeader
{
    uint32_t   Offset;
    uint32_t   PerThreadPrivateMemorySize;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocatePrivateMemorySurface ) == ( 8 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocatePrivateMemorySurface is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchAllocateSystemThreadSurface
\*****************************************************************************/
struct SPatchAllocateSystemThreadSurface :
       SPatchItemHeader
{
    uint32_t   Offset;
    uint32_t   PerThreadSystemThreadSurfaceSize;
    uint32_t   BTI;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateSystemThreadSurface ) == ( 12 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocateSystemThreadSurface is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchAllocateSurfaceWithInitialization
\*****************************************************************************/
struct SPatchAllocateSurfaceWithInitialization :
       SPatchItemHeader
{
    uint32_t   Offset;
    uint32_t   InitializationDataSize;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateSurfaceWithInitialization ) == ( 8 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocateSurfaceWithInitialization is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchAllocateConstantMemorySurfaceWithInitialization
\*****************************************************************************/
struct SPatchAllocateConstantMemorySurfaceWithInitialization :
       SPatchItemHeader
{
    uint32_t   ConstantBufferIndex;
    uint32_t   Offset;
#if 0 // needed for CB2CR - need RT buy off.
    uint32_t   InlineConstantBufferIndex;
#endif
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateConstantMemorySurfaceWithInitialization ) == ( 8 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocateConstantMemorySurfaceWithInitialization is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchAllocateLocalSurface
\*****************************************************************************/
struct SPatchAllocateLocalSurface :
       SPatchItemHeader
{
    uint32_t   Offset;
    uint32_t   TotalInlineLocalMemorySize;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateLocalSurface ) == ( 8 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocateLocalSurface is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchThreadPayload
\*****************************************************************************/
struct SPatchThreadPayload :
       SPatchItemHeader
{
    uint32_t HeaderPresent;
    uint32_t LocalIDXPresent;
    uint32_t LocalIDYPresent;
    uint32_t LocalIDZPresent;
    uint32_t LocalIDFlattenedPresent;
    uint32_t IndirectPayloadStorage;
    uint32_t UnusedPerThreadConstantPresent;
    uint32_t GetLocalIDPresent;
    uint32_t GetGroupIDPresent;
    uint32_t GetGlobalOffsetPresent;
    uint32_t StageInGridOriginPresent;
    uint32_t StageInGridSizePresent;
    uint32_t OffsetToSkipPerThreadDataLoad;
    uint32_t OffsetToSkipSetFFIDGP;
    uint32_t PassInlineData;
    uint32_t RTStackIDPresent;
    uint32_t generateLocalID;
    uint32_t emitLocalMask;
    uint32_t walkOrder;
    uint32_t tileY;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert(sizeof(SPatchThreadPayload) == (80 + sizeof(SPatchItemHeader)), "The size of SPatchThreadPayload is not what is expected");

/*****************************************************************************\
STRUCT: SPatchExecutionEnvironment
\*****************************************************************************/
struct SPatchExecutionEnvironment :
       SPatchItemHeader
{
    uint32_t    RequiredWorkGroupSizeX;
    uint32_t    RequiredWorkGroupSizeY;
    uint32_t    RequiredWorkGroupSizeZ;
    uint32_t    LargestCompiledSIMDSize;
    uint32_t    CompiledSubGroupsNumber;
    uint32_t    HasBarriers;
    uint32_t    DisableMidThreadPreemption;
    uint32_t    CompiledSIMD8;
    uint32_t    CompiledSIMD16;
    uint32_t    CompiledSIMD32;
    uint32_t    HasDeviceEnqueue;
    uint32_t    MayAccessUndeclaredResource;
    uint32_t    UsesFencesForReadWriteImages;
    uint32_t    UsesStatelessSpillFill;
    uint32_t    UsesMultiScratchSpaces;
    uint32_t    IsCoherent;
    uint32_t    IsInitializer;
    uint32_t    IsFinalizer;
    uint32_t    SubgroupIndependentForwardProgressRequired;
    uint32_t    CompiledForGreaterThan4GBBuffers;
    uint32_t    NumGRFRequired;
    uint32_t    WorkgroupWalkOrderDims; // dim0 : [0 : 1]; dim1 : [2 : 3]; dim2 : [4 : 5]
    uint32_t    HasGlobalAtomics;
    uint32_t    HasDPAS;
    uint32_t    HasRTCalls; // Raytracing extensions used in kernel.
    uint32_t    NumThreadsRequired;
    uint32_t    StatelessWritesCount;
    uint32_t    IndirectStatelessCount;
    uint32_t    UseBindlessMode;
    uint32_t    HasStackCalls;
    uint64_t    SIMDInfo;
    uint32_t    RequireDisableEUFusion;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert(sizeof(SPatchExecutionEnvironment) == (132 + sizeof(SPatchItemHeader)), "The size of SPatchExecutionEnvironment is not what is expected");

/*****************************************************************************\
STRUCT: SPatchString
\*****************************************************************************/
struct SPatchString :
       SPatchItemHeader
{
    uint32_t   Index;
    uint32_t   StringSize;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchString ) == ( 8 + sizeof( SPatchItemHeader ) ) , "The size of SPatchString is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchAllocatePrintfSurface
\*****************************************************************************/
struct SPatchAllocatePrintfSurface :
       SPatchItemHeader
{
    uint32_t   PrintfSurfaceIndex;
    uint32_t   Offset;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocatePrintfSurface ) == ( 8 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocatePrintfSurface is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchAllocateGlobalMemorySurfaceWithInitialization
\*****************************************************************************/
struct SPatchAllocateGlobalMemorySurfaceWithInitialization :
       SPatchItemHeader
{
    uint32_t   GlobalBufferIndex;
    uint32_t   Offset;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateGlobalMemorySurfaceWithInitialization ) == ( 8 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocateGlobalMemorySurfaceWithInitialization is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchCB2CRGatherTable
\*****************************************************************************/
struct SPatchCB2CRGatherTable :
        SPatchItemHeader
{
    uint32_t   NumberOfEntries;
    uint32_t   Offset;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchCB2CRGatherTable ) == ( 8 + sizeof( SPatchItemHeader ) ) , "The size of SPatchCB2CRGatherTable is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchCB2KernelArgument
\*****************************************************************************/
struct SPatchConstantBufferMapping :
        SPatchItemHeader
{
    uint32_t   ConstantBufferType;
    uint32_t   ConstantBufferIndex;
    uint32_t   ConstantBufferId;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchConstantBufferMapping ) == ( 12 + sizeof( SPatchItemHeader ) ) , "The size of SPatchConstantBufferMapping is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchStatelessGlobalMemoryObjectKernelArgument
\*****************************************************************************/
struct SPatchStatelessGlobalMemoryObjectKernelArgument :
       SPatchItemHeader
{
    uint32_t   ArgumentNumber;
    uint32_t   SurfaceStateHeapOffset;
    uint32_t   DataParamOffset;
    uint32_t   DataParamSize;
    uint32_t   LocationIndex;
    uint32_t   LocationIndex2;
    uint32_t   IsEmulationArgument;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchStatelessGlobalMemoryObjectKernelArgument ) == ( 28 + sizeof( SPatchItemHeader ) ) , "The size of SPatchStatelessGlobalMemoryObjectKernelArgument is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchStatelessConstantMemoryObjectKernelArgument
\*****************************************************************************/
struct SPatchStatelessConstantMemoryObjectKernelArgument :
       SPatchItemHeader
{
    uint32_t   ArgumentNumber;
    uint32_t   SurfaceStateHeapOffset;
    uint32_t   DataParamOffset;
    uint32_t   DataParamSize;
    uint32_t   LocationIndex;
    uint32_t   LocationIndex2;
    uint32_t   IsEmulationArgument;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchStatelessConstantMemoryObjectKernelArgument ) == ( 28 + sizeof( SPatchItemHeader ) ) , "The size of SPatchStatelessConstantMemoryObjectKernelArgument is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchAllocateStatelessGlobalMemorySurfaceWithInitialization
\*****************************************************************************/
struct SPatchAllocateStatelessGlobalMemorySurfaceWithInitialization :
    SPatchItemHeader
{
    uint32_t   GlobalBufferIndex;
    uint32_t   SurfaceStateHeapOffset;
    uint32_t   DataParamOffset;
    uint32_t   DataParamSize;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateStatelessGlobalMemorySurfaceWithInitialization ) == ( 16 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocateStatelessGlobalMemorySurfaceWithInitialization is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchAllocateStatelessConstantMemorySurfaceWithInitialization
\*****************************************************************************/
struct SPatchAllocateStatelessConstantMemorySurfaceWithInitialization :
    SPatchItemHeader
{
    uint32_t   ConstantBufferIndex;
    uint32_t   SurfaceStateHeapOffset;
    uint32_t   DataParamOffset;
    uint32_t   DataParamSize;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateStatelessConstantMemorySurfaceWithInitialization ) == ( 16 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocateStatelessConstantMemorySurfaceWithInitialization is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchAllocateGlobalMemorySurfaceProgramBinaryInfo
\*****************************************************************************/
struct SPatchAllocateGlobalMemorySurfaceProgramBinaryInfo :
    SPatchItemHeader
{
    uint32_t   Type;
    uint32_t   GlobalBufferIndex;
    uint32_t   InlineDataSize;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateGlobalMemorySurfaceProgramBinaryInfo ) == ( 12 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocateGlobalMemorySurfaceProgramBinaryInfo is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchKernelTypeProgramBinaryInfo
\*****************************************************************************/
struct SPatchKernelTypeProgramBinaryInfo :
    SPatchItemHeader
{
    uint32_t   Type; // constructor or destructor
    uint32_t   InlineDataSize;  // size of kernel name for constructor/desctructor
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchKernelTypeProgramBinaryInfo ) == ( 8 + sizeof( SPatchItemHeader ) ) , "The size of SPatchKernelTypeProgramBinaryInfo is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchInlineVMESamplerInfo
\*****************************************************************************/
struct SPatchInlineVMESamplerInfo :
       SPatchItemHeader
{
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchInlineVMESamplerInfo ) == ( sizeof( SPatchItemHeader ) ) , "The size of SPatchInlineVMESamplerInfo is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchAllocateConstantMemorySurfaceProgramBinaryInfo
\*****************************************************************************/
struct SPatchAllocateConstantMemorySurfaceProgramBinaryInfo :
    SPatchItemHeader
{
    uint32_t   ConstantBufferIndex;
    uint32_t   InlineDataSize;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateConstantMemorySurfaceProgramBinaryInfo ) == ( 8 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocateConstantMemorySurfaceProgramBinaryInfo is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchGlobalPointerProgramBinaryInfo
\*****************************************************************************/
struct SPatchGlobalPointerProgramBinaryInfo :
    SPatchItemHeader
{
    uint32_t   GlobalBufferIndex;
    uint64_t   GlobalPointerOffset;
    uint32_t   BufferType;
    uint32_t   BufferIndex;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchGlobalPointerProgramBinaryInfo ) == ( 20 + sizeof( SPatchItemHeader ) ) , "The size of SPatchGlobalPointerProgramBinaryInfo is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchConstantPointerProgramBinaryInfo
\*****************************************************************************/
struct SPatchConstantPointerProgramBinaryInfo :
    SPatchItemHeader
{
    uint32_t   ConstantBufferIndex;
    uint64_t   ConstantPointerOffset;
    uint32_t   BufferType;
    uint32_t   BufferIndex;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchConstantPointerProgramBinaryInfo ) == ( 20 + sizeof( SPatchItemHeader ) ) , "The size of SPatchConstantPointerProgramBinaryInfo is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchAllocateStatelessPrintfSurface
\*****************************************************************************/
struct SPatchAllocateStatelessPrintfSurface :
       SPatchItemHeader
{
    uint32_t   PrintfSurfaceIndex;
    uint32_t   SurfaceStateHeapOffset;
    uint32_t   DataParamOffset;
    uint32_t   DataParamSize;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateStatelessPrintfSurface ) == ( 16 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocateStatelessPrintfSurface is not what is expected" );

struct SPatchAllocateSyncBuffer :
       SPatchItemHeader
{
    uint32_t   SurfaceStateHeapOffset;
    uint32_t   DataParamOffset;
    uint32_t   DataParamSize;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateSyncBuffer ) == ( 12 + sizeof( SPatchItemHeader ) ), "The size of SPatchAllocateSyncBuffer is not what is expected" );

// Raytracing
struct SPatchAllocateRTGlobalBuffer :
    SPatchItemHeader
{
    uint32_t   SurfaceStateHeapOffset;
    uint32_t   DataParamOffset;
    uint32_t   DataParamSize;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateRTGlobalBuffer ) == ( 12 + sizeof( SPatchItemHeader ) ), "The size of SPatchAllocateRTGlobalBuffer is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchAllocateStatelessPrivateSurface
\*****************************************************************************/
struct SPatchAllocateStatelessPrivateSurface :
       SPatchItemHeader
{
    uint32_t   SurfaceStateHeapOffset;
    uint32_t   DataParamOffset;
    uint32_t   DataParamSize;
    uint32_t   PerThreadPrivateMemorySize;
    // if IsSimtThread is true, API assumes it is allocated per-simt-thread
    // else, API assumes it's allocated per-hardware-thread
    uint32_t   IsSimtThread;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateStatelessPrivateSurface ) == ( 20 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocateStatelessPrivateSurface is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchMediaVFEState
\*****************************************************************************/
struct SPatchMediaVFEState :
       SPatchItemHeader
{
    uint32_t   ScratchSpaceOffset;
    uint32_t   PerThreadScratchSpace;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchMediaVFEState ) == ( 8 + sizeof( SPatchItemHeader ) ) , "The size of SPatchMediaVFEState is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchAllocateStatelessEventPoolSurface
\*****************************************************************************/
struct SPatchAllocateStatelessEventPoolSurface :
       SPatchItemHeader
{
    uint32_t   EventPoolSurfaceIndex;
    uint32_t   SurfaceStateHeapOffset;
    uint32_t   DataParamOffset;
    uint32_t   DataParamSize;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateStatelessEventPoolSurface ) == ( 16 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocateStatelessEventPoolSurface is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchAllocateStatelessDefaultDeviceQueueSurface
\*****************************************************************************/
struct SPatchAllocateStatelessDefaultDeviceQueueSurface :
       SPatchItemHeader
{
    uint32_t   SurfaceStateHeapOffset;
    uint32_t   DataParamOffset;
    uint32_t   DataParamSize;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchAllocateStatelessDefaultDeviceQueueSurface ) == ( 12 + sizeof( SPatchItemHeader ) ) , "The size of SPatchAllocateStatelessDefaultDeviceQueueSurface is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchStatelessDeviceQueueKernelArgument
\*****************************************************************************/
struct SPatchStatelessDeviceQueueKernelArgument :
       SPatchItemHeader
{
    uint32_t   ArgumentNumber;
    uint32_t   SurfaceStateHeapOffset;
    uint32_t   DataParamOffset;
    uint32_t   DataParamSize;
    uint32_t   LocationIndex;
    uint32_t   LocationIndex2;
    uint32_t   IsEmulationArgument;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchStatelessDeviceQueueKernelArgument ) == ( 28 + sizeof( SPatchItemHeader ) ) , "The size of SPatchStatelessDeviceQueueKernelArgument is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchNullSurfaceLocation
\*****************************************************************************/
struct SPatchNullSurfaceLocation :
       SPatchItemHeader
{
    uint32_t   Offset;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert( sizeof( SPatchNullSurfaceLocation ) == ( 4 + sizeof( SPatchItemHeader ) ) , "The size of SPatchNullSurfaceLocation is not what is expected" );

/*****************************************************************************\
STRUCT: SPatchGtpinFreeGRFInfo
\*****************************************************************************/
struct SPatchGtpinFreeGRFInfo :
    SPatchItemHeader
{
    uint32_t   BufferSize;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert(sizeof(SPatchGtpinFreeGRFInfo) == (4 + sizeof(SPatchItemHeader)), "The size of SPatchGtpinFreeGRFInfo is not what is expected");

/*****************************************************************************\
 STRUCT: SPatchFunctionTableInfo
 \*****************************************************************************/
struct SPatchFunctionTableInfo :
    SPatchItemHeader
{
    uint32_t   NumEntries;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert(sizeof(SPatchFunctionTableInfo) == (4 + sizeof(SPatchItemHeader)), "The size of SPatchFunctionTableInfo is not what is expected");

/*****************************************************************************\
 STRUCT: SPatchGlobalHostAccessTableInfo
 \*****************************************************************************/
struct SPatchGlobalHostAccessTableInfo :
    SPatchItemHeader
{
    uint32_t   NumEntries;
};

// Update CURRENT_ICBE_VERSION when modifying the patch list
static_assert(sizeof(SPatchGlobalHostAccessTableInfo) == (4 + sizeof(SPatchItemHeader)), "The size of SPatchGlobalHostAccessTableInfo is not what is expected");

} // namespace
#pragma pack( pop )
