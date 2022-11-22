/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "OCL/sp/sp_types.h"

namespace G6HWC
{

/*****************************************************************************\
STRUCT: SRange
\*****************************************************************************/
#ifdef __cplusplus
template<class Type>
struct SRange
{
    Type    Min;
    Type    Max;
};

template<class Type>
struct SRangeA
{
    Type    Min;
    Type    Max;

    SRangeA( const Type min, const Type max )
    {
        Min = min;
        Max = max;
    };
};
#endif

/*****************************************************************************\
STRUCT: S3DRender
\*****************************************************************************/
struct S3DRenderUnitCapabilities
{
    DWORD   IsBarycentricInterpolationSupported : 1;     // BIT(0)
    DWORD   IsInstructionCompactionSupported    : 1;     // BIT(1)
    DWORD                                       :14;
};

/*****************************************************************************\
STRUCT: S3DDataPort
\*****************************************************************************/
struct S3DDataPortCapabilities
{
    DWORD   OwordBlockTypes;
    DWORD   OwordBlockCount[3];
    DWORD   OwordDualBlockTypes;
    DWORD   OwordDualBlockCount[2];
};

/*****************************************************************************\
STRUCT: S3DKernelHardwareCapabilities
\*****************************************************************************/
struct S3DKernelHardwareCapabilities
{
    DWORD   NumUserClipPlanes;

    DWORD   NumHardwareGRFRegisters;
    DWORD   NumSoftwareGRFRegisters;
    DWORD   NumGRFRegistersPerBlock;
    DWORD   NumMRFRegisters;
    DWORD   NumFlagRegisters;

    DWORD   URBRowsPerSetupWrite;

    DWORD   EUCount;
    SRange<DWORD>   EUCountPerSubSlice;
    DWORD   SubSliceCount;
    DWORD   EUIDCount;
    DWORD   EUThreadsPerEU;
    DWORD   EUReservedGrfRegister;
    DWORD   EUReservedGrfSubRegister;
    DWORD   EUGrfMrfRegisterSizeInBytes;
    DWORD   EUIStackUnderflowException; // not used
    DWORD   EUIStackOverflowException;  // not used
    DWORD   EULStackUnderflowException; // not used
    DWORD   EULStackOverflowException;  // not used
    DWORD   EUIllegalOpcodeException;   // not used
    DWORD   EUSoftwareExceptionControl;
    DWORD   EUMaxSoftwareStackValue;
    DWORD   EUMaxHardwareStackValue;    // not used

    DWORD   EUStackSoftwareOverflow;
    DWORD   EUStackSoftwareUnderflow;
    DWORD   EUStackSoftwareI;
    DWORD   EUStackSoftwareL;
    DWORD   EUStackSoftwareFunctionCall;

    DWORD           EUJumpCountPerInstruction;
    SRange<int>     EUInstructionJumpLimit;
    SRange<DWORD>   ScratchSpacePerThread;

    DWORD           InstructionCacheSize;
    DWORD           RenderCacheSize;
    DWORD           KernelPointerAlignSize;

    DWORD           MaxAssignableBindingTableIndex;

    DWORD           StatelessModelBindingTableIndex;

    bool            HasSharedLocalMemory;
    DWORD           SharedLocalMemoryBindingTableIndex;

    bool            HasCoherentIAAccess;
    DWORD           CoherentIAAccessBindingTableIndex;

    S3DRenderUnitCapabilities RenderUnit;
    S3DDataPortCapabilities  DataPort;

    bool            HasDbgReg;

};


/*****************************************************************************\
STRUCT: SMediaHardwareCapabilities
\*****************************************************************************/
struct SMediaHardwareCapabilities
{
    DWORD   NumSamplersPerProgram;
    DWORD   NumSurfacesPerProgram;

    SRange<DWORD>   SamplerAnistropyRange;

    // TODO: these should be range per surface type
    DWORD   MaxSurface1DWidth;
    DWORD   MaxSurface1DHeight;
    DWORD   MaxSurface1DDepth;

    DWORD   MaxSurface2DWidth;
    DWORD   MaxSurface2DHeight;
    DWORD   MaxSurface2DDepth;

    DWORD   MaxSurface3DWidth;
    DWORD   MaxSurface3DHeight;
    DWORD   MaxSurface3DDepth;

    DWORD   MaxSurfaceCubeWidth;
    DWORD   MaxSurfaceCubeHeight;
    DWORD   MaxSurfaceCubeDepth;

    DWORD   MaxSurfaceLODs;

    DWORD   MaxBufferLength;

    SRange<float>   SampleLOD;
    SRange<float>   MipMapLODBias;

    SRange<DWORD>   URBEntryReadOffset;
    SRange<DWORD>   URBEntryReadLength;

    DWORD   MaxURBPayloadStartRegister;

    SRange<DWORD>   URBEntriesSize;
    DWORD           URBEntrySize;
    DWORD           URBRowSize;
    DWORD           URBSize;
    DWORD           URBAllocationGranularitySize;  // Bytes

    DWORD           L2CacheLineSize;

    DWORD            InstructionCachePrefetchSize;

    DWORD           SurfaceStatePointerAlignSize;
    DWORD           BindingTableStatePointerAlignSize;
    DWORD           SamplerStatePointerAlignSize;
    DWORD           KernelPointerAlignSize;
    DWORD           ScratchPointerAlignSize;
    DWORD           DefaultColorPointerAlignSize;
    DWORD           ConstantBufferPointerAlignSize;
    DWORD           InterfaceDescriptorDataAlignSize;

    DWORD           GeneralStateBaseAddressAlignSize;
    DWORD           SurfaceStateBaseAddressAlignSize;
    DWORD           DynamicStateBaseAddressAlignSize;
    DWORD           IndirectObjectBaseAddressAlignSize;
    DWORD           InstructionBaseAddressAlignSize;

    DWORD           SIPPointerAlignSize;

    S3DKernelHardwareCapabilities   KernelHwCaps;
};

/*****************************************************************************\
PROTOTYPE: InitializeCapsGen8
\*****************************************************************************/
void InitializeCapsGen8(
    SMediaHardwareCapabilities* pCaps );

}
