/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ShaderTypesEnum.h"
#include "ShaderTypesConst.h"
#include "SurfaceFormats.h"
#include "usc_config.h"

/*****************************************************************************
MACRO: BITCOUNT
//  returns count of bits required to store set of (v) values in range <0;v-1>
*****************************************************************************/
#ifndef BITCOUNT
#define BITCOUNT(v) (((v)-1U)<65536U?((v)-1U)<256U?((v)-1U)<16U?((v)-1U)<4U?((v)-1U)<2U?1:2:((v)-1U)<\
8U?3:4:((v)-1U)<64U?((v)-1U)<32U?5:6:((v)-1U)<128U?7:8:((v)-1U)<4096U?((v)-1U)<1024U?((v)-1U)<512U?9:10\
:((v)-1U)<2048U?11:12:((v)-1U)<(1U<<14U)?((v)-1U)<8192U?13:14:((v)-1U)<32768U?15:16:((v)-1U)<(1U<<24U)\
?((v)-1U)<(1U<<20U)?((v)-1U)<262144U?((v)-1U)<131072U?17:18:((v)-1U)<524288U?19:20:((v)-1U)<(1U<<22U)?\
((v)-1U)<(1U<<21U)?21:22:((v)-1U)<(1U<<23U)?23:24:((v)-1U)<(1U<<28U)?((v)-1U)<(1U<<26U)?((v)-1U)<(1U<<25U\
)?25:26:((v)-1U)<(1U<<27U)?27:28:((v)-1U)<(1U<<30U)?((v)-1U)<(1U<<29U)?29:30:((v)-1U)<(1U<<31U)?31:32)
#endif

/*****************************************************************************\
compile-time USC_API_C_ASSERT
\*****************************************************************************/
#ifndef USC_API_C_ASSERT
#define USC_API_C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#endif

// verify that unsigned int is 32-bit
USC_API_C_ASSERT( sizeof( unsigned int )==4 );

namespace USC
{

/*****************************************************************************\
ENUM: Needed for Shader serialization and DeepCopy. Indicates which operation
      should be performed.
\*****************************************************************************/
enum IL_OP_TYPE
{
    IL_OP_DEEP_COPY,
    IL_OP_READ_FROM_STREAM,
    IL_OP_WRITE_TO_STREAM,
    IL_OP_COUNT_WRITE_BYTES
};

/*****************************************************************************\
ENUM: Needed for Shader serialization and DeepCopy of non trivial declarations.
      Indicates which declaration type should be processed.
\*****************************************************************************/
enum IL_DECL_TYPE
{
    IL_DECL_FUNCTION_TABLE,
    IL_DECL_INTERFACE
};

/*****************************************************************************\
ENUM: SHADER_OPERAND_PRECISION
\*****************************************************************************/
enum SHADER_OPERAND_PRECISION
{
    SHADER_OPERAND_PRECISION_DEFAULT = 0,
    SHADER_OPERAND_PRECISION_16      = 1, // for floats and signed int
    SHADER_OPERAND_PRECISION_8       = 2, // for signed int

    NUM_SHADER_OPERAND_PRECISIONS
};

/*****************************************************************************\
operator: < for SHADER_OPERAND_PRECISION
\*****************************************************************************/
inline bool isOfLesserPrec( SHADER_OPERAND_PRECISION precLesser, SHADER_OPERAND_PRECISION precGreater )
{
    // intentionally reversed
    return static_cast<unsigned int>( precLesser ) > static_cast<unsigned int>( precGreater );
}

/*****************************************************************************\

Function:
    MinPrec

Description:
    Returns lesser of two given operand precisions. Logically lesser precision
    (e.g. 8 bit is less than 16 bit, which is less than default 32 bit) means
    greater value of enum SHADER_OPERAND_PRECISION.

Input:
    SHADER_OPERAND_PRECISION lhsPrec - First precision.
    SHADER_OPERAND_PRECISION rhsPrec - Second precision.

Output:
    SHADER_OPERAND_PRECISION - Result precision.

\*****************************************************************************/
inline SHADER_OPERAND_PRECISION MinPrec(
    SHADER_OPERAND_PRECISION lhsPrec,
    SHADER_OPERAND_PRECISION rhsPrec )
{
    return isOfLesserPrec( lhsPrec, rhsPrec ) ? lhsPrec : rhsPrec;
}

/*****************************************************************************\

Function:
    MaxPrec

Description:
    Returns greater of two given operand precisions. Logically greater precision
    (e.g. default 32 bit is greater than 16 bit, which is greater than 8 bit)
    means lesser value of enum SHADER_OPERAND_PRECISION.

Input:
    SHADER_OPERAND_PRECISION lhsPrec - First precision.
    SHADER_OPERAND_PRECISION rhsPrec - Second precision.

Output:
    SHADER_OPERAND_PRECISION - Result precision.

\*****************************************************************************/
inline SHADER_OPERAND_PRECISION MaxPrec(
    SHADER_OPERAND_PRECISION lhsPrec,
    SHADER_OPERAND_PRECISION rhsPrec )
{
    return isOfLesserPrec( lhsPrec, rhsPrec ) ? rhsPrec : lhsPrec;
}

/*****************************************************************************\
STRUCT: SShaderResourceDeclType
\*****************************************************************************/
struct SShaderSamplerDeclType
{
    unsigned int    SamplerType     : BITCOUNT( IGC::NUM_SHADER_SAMPLER_TYPES );
    unsigned int    LBound; // IGC::SHADER_VERSION_4_0
    unsigned int    UBound; // IGC::SHADER_VERSION_4_0
    unsigned int    Space;  // IGC::SHADER_VERSION_4_0
};

/*****************************************************************************\
STRUCT: SShaderResourceDeclType
\*****************************************************************************/
struct SShaderResourceDeclType
{
    unsigned int   ResourceType         : BITCOUNT( IGC::NUM_SHADER_RESOURCE_TYPES );
    unsigned int   SurfaceFormat        : BITCOUNT( IGC::NUM_SURFACE_FORMATS );
    unsigned int   UAVAccessMode        : BITCOUNT( IGC::NUM_SHADER_UAV_ACCESS_MODES );
    unsigned int   ReturnType           : BITCOUNT( IGC::NUM_SHADER_RESOURCE_RETURN_TYPES );
    unsigned int   AccessCoherency      : 1;
    unsigned int   RasterizerOrdered    : 1;
    unsigned int   IsVariable           : 1;
    unsigned int   Stride;
    unsigned int   ByteOrStructCount;
    unsigned int   Offset;
    unsigned int   Alignment;
    unsigned int   LBound; // IGC::SHADER_VERSION_4_0
    unsigned int   UBound; // IGC::SHADER_VERSION_4_0
    unsigned int   Space;  // IGC::SHADER_VERSION_4_0
};

/*****************************************************************************\
STRUCT: SShaderConstantBufferDeclType
\*****************************************************************************/
struct SShaderConstantBufferDeclType
{
    unsigned int   LBound; // IGC::SHADER_VERSION_4_0
    unsigned int   UBound; // IGC::SHADER_VERSION_4_0
    unsigned int   Space;  // IGC::SHADER_VERSION_4_0
    unsigned int   Size;   // IGC::SHADER_VERSION_4_0. Count of 16-byte vectors. 0 if not known.
};

/*****************************************************************************\
STRUCT: SShaderInputDeclType
\*****************************************************************************/
struct SShaderInputDeclType
{
    unsigned int   Mask                : IGC::NUM_SHADER_CHANNELS;
    unsigned int   InterpolationMode   : BITCOUNT( IGC::NUM_SHADER_INTERPOLATION_MODES );
    unsigned int   IsIndexed           : 1;

    union
    {
        struct
        {
            unsigned int X : 1;
            unsigned int Y : 1;
            unsigned int Z : 1;
            unsigned int W : 1;
        } Channel;

        unsigned int   Value   : IGC::NUM_SHADER_CHANNELS;
    } PrimIDMask;

    unsigned char  Usage[IGC::NUM_SHADER_CHANNELS];
    unsigned char  UsageIndex[IGC::NUM_SHADER_CHANNELS];

    bool HasUsesFullPrecision[IGC::NUM_SHADER_CHANNELS];
    bool HasUsesLowPrecision[IGC::NUM_SHADER_CHANNELS];
};

/*****************************************************************************\
STRUCT: SShaderOutputDeclType
\*****************************************************************************/
struct SShaderOutputDeclType
{
    unsigned int   Mask        : IGC::NUM_SHADER_CHANNELS;
    unsigned int   IsIndexed   : 1;
    unsigned int   IsInvariant : 1;

    unsigned char  Usage[IGC::NUM_SHADER_CHANNELS];
    unsigned char  UsageIndex[IGC::NUM_SHADER_CHANNELS];
};

/*****************************************************************************\
STRUCT: SShaderOpcodeCaps
\*****************************************************************************/
struct SShaderOpcodeCaps
{
    IGC::SHADER_OPCODE opcode;
    IGC::SHADER_VERSION_TYPE  Version;
    bool SupportsPredicate;
    bool SupportsResource;
    bool SupportsComparison;
    bool SupportsConditional;
};

/*****************************************************************************\
STRUCT: SCallSiteUsage
\*****************************************************************************/
struct SCallSiteUsage
{
    unsigned int   usageCnt    : 16;
    unsigned int   tramIndex   : 16;
};

USC_API_C_ASSERT( sizeof(SCallSiteUsage) == sizeof(unsigned int) );

/*****************************************************************************\
STRUCT: SShaderInterfaceDeclType
\*****************************************************************************/
struct SShaderInterfaceDeclType
{
    unsigned int   originalID          : 8;    // max 253 interfaces allowed
    unsigned int   arraySize           : 8;    // can't have more instances than max number of interfaces
    unsigned int   numCallSites        : 16;   // max number of call sites is 4096
    unsigned int   numFunctionTables;
    unsigned int*  pFunctionTables;
    unsigned int*  pTargets;
    SCallSiteUsage* pCallSitesUsage;    // Call sites usage / trampoline indices
    unsigned int   constantBufferOffset;       // Offset for interface jump table in CB

    unsigned int*  pInternalBuffer;            // this is for function tables and call targets storage
};

/*****************************************************************************\
STRUCT: SShaderFunctionTableDeclType
\*****************************************************************************/
struct SShaderFunctionTableDeclType
{
    unsigned int   numFunctionBodies;
    unsigned int*  pFunctionBodies;
    unsigned int*  pJumpOffsets;

    unsigned int*  pInternalBuffer;
};

/*****************************************************************************\
STRUCT: SVFuncCallOffsets

Description:

    Structure stores offsets to a single virtual function for multiple
    dispatch modes.

\*****************************************************************************/
struct SVFuncCallOffsets
{
    unsigned int m_Simd8Offset;
    unsigned int m_Simd16Offset;
    unsigned int m_Simd32Offset;
};

/*****************************************************************************\
STRUCT: SOfflineCompileData

Description:

    Offline compilation data

\*****************************************************************************/
struct SOfflineCompileData
{
    void*   pProgramData;
    unsigned int   programSize;

    struct _Common
    {
        unsigned int   BindingTableCount;
    } Common;

    struct _PS
    {
        bool    ReqBarycentricPerspectivePixelLocation;
        bool    ReqBarycentricPerspectiveCentroid;
        bool    ReqBarycentricPerspectiveSample;
        bool    ReqBarycentricNonPerspectivePixelLocation;
        bool    ReqBarycentricNonPerspectiveCentroid;
        bool    ReqBarycentricNonPerspectiveSample;

        bool    CompiledFor32PixelDispatch;
        bool    CompiledFor16PixelDispatch;
        bool    CompiledFor8PixelDispatch;

        unsigned int   SIMD32ProgramSize;
        unsigned int   SIMD16ProgramSize;
        unsigned int   SIMD8ProgramSize;

        unsigned int   SIMD32DispatchGRFStartRegister;
        unsigned int   SIMD16DispatchGRFStartRegister;
        unsigned int   SIMD8DispatchGRFStartRegister;
    } PS;
};

bool OpcodeSupportsPredicate(IGC::SHADER_OPCODE opcode);
bool OpcodeSupportsResource(IGC::SHADER_OPCODE opcode);
bool OpcodeSupportsComparison(IGC::SHADER_OPCODE opcode);
bool OpcodeSupportsConditional(IGC::SHADER_OPCODE opcode);

/*****************************************************************************\
STRUCT: SInterfaceThisData
\*****************************************************************************/
struct SInterfaceThisData
{
    unsigned int   ConstantBufferID;
    unsigned int   ConstantBufferOffest;
    unsigned int   BaseTextureIndex;
    unsigned int   BaseSamplerIndex;
};

/*****************************************************************************\
STRUCT: SInterfacesBindingData
\*****************************************************************************/
struct SInterfacesBindingData
{
    unsigned int        NumInstances;
    SInterfaceThisData  ThisData[ NUM_SHADER_INTERFACES ];
    unsigned int        FunctionTables[ NUM_SHADER_INTERFACES ];
};

} //namespace USC

namespace iSTD
{

template<typename T>
struct IsArrayTypeSupported;

template<>
struct IsArrayTypeSupported< USC::SHADER_OPERAND_PRECISION > { enum { value = true }; };

} // namespace iSTD

