/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef STRUCTURE_ALINGMENT_VERIFICATION
#pragma once
#endif

// reuse GFX enumeration types common to gen6 and gen7 
#include <sstream>
#include "usc.h"
#include "SurfaceFormats.h"
#include "ShaderTypesConst.h"


/******************************************************************************\
Serialization of CompilerOutput structures is affected by structure padding
added by C++ compiler. Each member of a structure and the structure itself
maybe padded if its size does not meet specific criteria.
For instance: if the structure contains two members: char and int, 3 bytes
will be added after the char so int starts at the address that is a multiple
of int size. Similar rule applies the entire structure.
The problem is that sometimes a slight difference occurs between C++
compilers. It happens when the structure inherits from another structure and
the base one needs padding. MS compiler adds padding between base and
derived one, while GCC does at the end of derived one. As the result sizes
on both compilers are the same, but internally, on byte basis, they are
different. If such construction is serialized on build from one compiler,
it cannot be properly deserialized on build form the second one.
Theoretically #pragma pack(1) could be used to prevent compilers from adding
padding, but some compilers have only partial support for it.

Therefore manual padding is added to prevent compilers from adding it
automatically. It is added between members as well as at the end of the
structures.

Padding is added by unnamed bitfields of size equal to the size of type of
bitfield, e.g.:

One byte padding is:

char : sizeof(char) * 8

To make it less obscure, three base paddings are defined:
PADDING_1_BYTE
PADDING_2_BYTES
PADDING_4_BYTES

And two additional, based on architecture:
PADDING_4_BYTES_x64_ONLY
PADDING_4_BYTES_x32_ONLY

The first one will add four bytes on 64bit builds only, while the second one
will do the same on 32bit builds.

There is a separate verification project "glsl_compile_time_verification",
which verifies in compile time whether manual padding is required.
\******************************************************************************/
#define PADDING_1_BYTE char : sizeof(char) * 8;
#define PADDING_2_BYTES short int : sizeof(short int) * 8;
#define PADDING_4_BYTES int : sizeof(int) * 8;

#ifdef _AMD64_
#define PADDING_4_BYTES_x64_ONLY    PADDING_4_BYTES;
#define PADDING_4_BYTES_x32_ONLY   
#else
#define PADDING_4_BYTES_x32_ONLY    PADDING_4_BYTES;
#define PADDING_4_BYTES_x64_ONLY
#endif

namespace IGC
{
    enum class PushConstantMode : unsigned int
    {
        DEFAULT = 0,
        SIMPLE = 1,
        GATHER = 2,
        NONE = 3,
    };
}

namespace USC
{

/*****************************************************************************\
ENUM: TESSELLATOR_DOMAIN_TYPE
\*****************************************************************************/
enum TESSELLATOR_DOMAIN_TYPE
{
    TESSELLATOR_DOMAIN_QUAD,
    TESSELLATOR_DOMAIN_TRI,
    TESSELLATOR_DOMAIN_ISOLINE,
    NUM_TESSELLATOR_DOMAIN_TYPES
};

/*****************************************************************************\
ENUM: TESSELLATOR_PARTITIONING_TYPE
\*****************************************************************************/
enum TESSELLATOR_PARTITIONING_TYPE
{
    TESSELLATOR_PARTITIONING_INTEGER,
    TESSELLATOR_PARTITIONING_POW2,
    TESSELLATOR_PARTITIONING_FRACTIONAL_ODD,
    TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN,
    NUM_TESSELLATOR_PARTITIONING_TYPES
};

/*****************************************************************************\
ENUM: TESSELLATOR_OUTPUT_PRIMITIVE_TYPE
\*****************************************************************************/
enum TESSELLATOR_OUTPUT_PRIMITIVE_TYPE
{
    TESSELLATOR_OUTPUT_PRIMITIVE_POINT,
    TESSELLATOR_OUTPUT_PRIMITIVE_LINE,
    TESSELLATOR_OUTPUT_PRIMITIVE_TRIANGLE_CW,
    TESSELLATOR_OUTPUT_PRIMITIVE_TRIANGLE_CCW,
    NUM_TESSELLATOR_OUTPUT_PRIMITIVE_TYPES
};

/*****************************************************************************\
ENUM: GFX3DSTATE_GEOMETRY_SHADER_DISPATCH_MODE
\*****************************************************************************/
enum GFX3DSTATE_GEOMETRY_SHADER_DISPATCH_MODE
{
    GFX3DSTATE_GEOMETRY_SHADER_DISPATCH_MODE_SINGLE         = 0x0,
    GFX3DSTATE_GEOMETRY_SHADER_DISPATCH_MODE_DUAL_INSTANCE  = 0x1,
    GFX3DSTATE_GEOMETRY_SHADER_DISPATCH_MODE_DUAL_OBJECT    = 0x2,
    GFX3DSTATE_GEOMETRY_SHADER_DISPATCH_MODE_SIMD8          = 0x3
};

/*****************************************************************************\
ENUM: GFX3DSTATE_CONTROL_DATA_FORMAT
\*****************************************************************************/
enum GFX3DSTATE_CONTROL_DATA_FORMAT
{
    GFX3DSTATE_CONTROL_DATA_FORMAT_CUT  = 0x0,
    GFX3DSTATE_CONTROL_DATA_FORMAT_SID  = 0x1
};

/*****************************************************************************\
ENUM: GFX3DSTATE_EARLY_DEPTH_STENCIL_CONTROL
\*****************************************************************************/
enum GFX3DSTATE_EARLY_DEPTH_STENCIL_CONTROL
{
    GFX3DSTATE_EARLY_DEPTH_STENCIL_CONTROL_NORMAL   = 0x0,
    GFX3DSTATE_EARLY_DEPTH_STENCIL_CONTROL_PSEXEC   = 0x1,
    GFX3DSTATE_EARLY_DEPTH_STENCIL_CONTROL_PREPS    = 0x2
    // Reserved                                     = 0x3
};

/*****************************************************************************\
ENUM: GFX3DSTATE_COMPUTED_DEPTH_MODE
\*****************************************************************************/
enum GFX3DSTATE_COMPUTED_DEPTH_MODE
{
    GFX3DSTATE_COMPUTED_DEPTH_MODE_OFF          = 0x0,
    GFX3DSTATE_COMPUTED_DEPTH_MODE_ON           = 0x1,
    GFX3DSTATE_COMPUTED_DEPTH_MODE_ON_GE_SRC    = 0x2,
    GFX3DSTATE_COMPUTED_DEPTH_MODE_ON_LE_SRC    = 0x3
};

/*****************************************************************************\
ENUM: GFX3DSTATE_ROUNDING_MODE
\*****************************************************************************/
enum GFX3DSTATE_ROUNDING_MODE
{
    GFX3DSTATE_ROUNDING_MODE_ROUND_TO_NEAREST_EVEN  = 0x0,
    GFX3DSTATE_ROUNDING_MODE_ROUND_TO_POS_INF       = 0x1,
    GFX3DSTATE_ROUNDING_MODE_ROUND_TO_NEG_INF       = 0x2,
    GFX3DSTATE_ROUNDING_MODE_ROUND_TO_ZERO          = 0x3
};

/*****************************************************************************\
ENUM: GFXMEDIA_GPGPU_MODE
\*****************************************************************************/
enum GFXMEDIA_GPGPU_MODE
{
    GFXMEDIA_GPGPU_MODE_MEDIA   = 0x0,
    GFXMEDIA_GPGPU_MODE_GPGPU   = 0x1
};

/*****************************************************************************\
ENUM: GFXMEDIA_MMIO_ACCESS_CONTROL
\*****************************************************************************/
enum GFXMEDIA_MMIO_ACCESS_CONTROL
{
    GFXMEDIA_MMIO_ACCESS_CONTROL_NO_READWRITE   = 0x0,
    GFXMEDIA_MMIO_ACCESS_CONTROL_OA_READWRITE   = 0x1,
    GFXMEDIA_MMIO_ACCESS_CONTROL_ANY_READWRITE  = 0x2
};

/*****************************************************************************\
ENUM: GFXMEDIA_GPUWALKER_SIMDSIZE
\*****************************************************************************/
enum GFXMEDIA_GPUWALKER_SIMD
{
    GFXMEDIA_GPUWALKER_SIMD8    = 0x0,
    GFXMEDIA_GPUWALKER_SIMD16   = 0x1,
    GFXMEDIA_GPUWALKER_SIMD32   = 0x2
};

/*****************************************************************************\
Enum: GFX3DSTATE_SF_ATTRIBUTE_ACTIVE_COMPONENT
\*****************************************************************************/
enum  GFX3DSTATE_SF_ATTRIBUTE_ACTIVE_COMPONENT
{
    GFX3DSTATE_SF_ATTRIBUTE_ACTIVE_COMPONENT_DISABLED = 0x0,  // All components disabled
    GFX3DSTATE_SF_ATTRIBUTE_ACTIVE_COMPONENT_XY = 0x1,        // 2D attribute, z and w components disabled
    GFX3DSTATE_SF_ATTRIBUTE_ACTIVE_COMPONENT_XYZ = 0x2,       // 3D attribute, w components disabled
    GFX3DSTATE_SF_ATTRIBUTE_ACTIVE_COMPONENT_XYZW = 0x3,      // 4D attribute, no disabled components
};

enum GFX3DSTATE_PSEXTRA_INPUT_COVERAGE_MASK_MODE
{
    GFX3DSTATE_PSEXTRA_INPUT_COVERAGE_MASK_MODE_NONE,               // No Coverage
    GFX3DSTATE_PSEXTRA_INPUT_COVERAGE_MASK_MODE_NORMAL,             // OUTERCONSERVATIVE when conservative rasterization is enabled. 
                                                                    // Normal otherwise.
    GFX3DSTATE_PSEXTRA_INPUT_COVERAGE_MASK_MODE_INNERCONSERVATIVE,  // INNER conservative rasterization
    GFX3DSTATE_PSEXTRA_INPUT_COVERAGE_MASK_MODE_DEPTH_COVERAGE      // Depth coverage
};

/*****************************************************************************\
STRUCT: STypedUAVReadEmulationEntry
\*****************************************************************************/
struct STypedUAVReadEmulationEntry
{
    bool                    m_Valid;
    bool                    m_DoubleHorizontalSize;
    IGC::SURFACE_FORMAT     m_SubstituteFormat;
    unsigned int            m_UAVEmulationIndex;
};

/*****************************************************************************\
STRUCT: SInterfaceThisData
\*****************************************************************************/
struct STypedUAVReadEmulBTLayout
{
    STypedUAVReadEmulationEntry*    pTypedUAVReadEmulBTEntries;
    unsigned int                    TypedUAVReadEmulBTEntriesSize;
};

struct ConstantAddress
{
    unsigned int bufId = 0;
    unsigned int eltId = 0;
    int size = 0;

    void Serialize( std::stringstream& stringStream ) const
    {
        stringStream << bufId << eltId << size;
    }
};

bool operator < (const ConstantAddress &a, const ConstantAddress &b);

struct ConstantAddrValue
{
    ConstantAddress ca;
    bool anyValue;
    uint32_t value;
};

struct InlineDynConstants
{
    ConstantAddress ca;
    uint32_t value;

    void Serialize( std::stringstream& stringStream ) const
    {
        ca.Serialize( stringStream );

        stringStream << value;
    }
};

// Dynamic Constant Folding
struct DynamicConstFoldingInputs
{
    const InlineDynConstants* pInlineDynConstants = nullptr;
    unsigned int              m_inlineDynConstantsSize = 0;

    void Serialize( std::stringstream& stringStream ) const
    {
        for( unsigned int i = 0; i < m_inlineDynConstantsSize; i++ )
        {
            pInlineDynConstants[ i ].Serialize( stringStream );
        }
    }
};

// Constant Buffer to Constant Register gather entry 
struct SConstantGatherEntry 
{
    // ### DW3 3DSTATE_GATHER_CONSTANT_* ###
    union _GatherEntry
    {
        struct _Fields
        {
            unsigned short    constantBufferIndex  : 4;   // bits 3:0 
            unsigned short    channelMask          : 4;   // bits 7:4 
            unsigned short    constantBufferOffset : 8;   // bits 15:8  
        } Fields;
        unsigned short   Value;
    } GatherEntry;
};

/*****************************************************************************\
STRUCT: SComputeShaderNOSLayout
\*****************************************************************************/
struct SComputeShaderNOSLayout
{
    unsigned int runtimeVal_LoopCount;
    unsigned int runtimeVal_ResWidthOrHeight;
    unsigned int runtimeVal_ConstBufferSize;
};

struct SCompilerInputCommon
{
    DynamicConstFoldingInputs m_DcfInputs;
    void* m_pGTPinInput;
    const unsigned int* m_pShaderDebugInfo;

    IGC::PushConstantMode m_PushConstantMode;

    void Serialize( std::stringstream& shaderCacheBlob ) const
    {
        m_DcfInputs.Serialize( shaderCacheBlob );
        shaderCacheBlob << (unsigned int)m_PushConstantMode;

        // Assume that m_pGTPinInput and m_pShaderDebugInfo are not valid when caching
    }
};

/*****************************************************************************\
STRUCT: SCompilerInputCommon_Gen7
\*****************************************************************************/
struct SCompilerInputCommon_Gen7 : public SCompilerInputCommon
{
    bool secondCompile; // Set this flag to indicate to the compiler that this is the 2nd compilation of the kernel
    bool isRowMajor;
    int  numChannelsUsed;
    unsigned int shaderHash;

    void Serialize( std::stringstream& shaderCacheBlob ) const
    {
        SCompilerInputCommon::Serialize( shaderCacheBlob );
        shaderCacheBlob << secondCompile << isRowMajor << numChannelsUsed << shaderHash;
    }
};

static const SComputeShaderNOSLayout g_nosLayout = { 0, 1, 2 };

struct SCompilerOutputCommon_Gen7
{
    // DX10+ immediate constants defined in shader code; expected driver behavior:
    //    a) allocate an internal CB of size 'm_ImmediateConstantsSize' 
    //    b) copy 'm_pImmediateConstants' data to this CBbuffer
    //    c) bind internal CB to 'SBindingTableLayout.immediateConstantBufferIndex'
    void*           m_pImmediateConstants;
    unsigned int    m_ImmediateConstantsSize;       // if 0, immediate constants not used

    // DX11+ shader interface binding table; expected driver behavior:
    //    a) allocate an internal CB of size 'm_InterfaceConstantsSize'
    //    b) lock buffer for writting on SetShaderWithInterfaces() 
    //    c) call Populate*ShaderInterfaceData11() passing interface bind data
    //    d) unmap buffer and bind to 'SBindingTableLayout.interfaceConstantBufferIndex'
    unsigned int   m_InterfaceConstantsSize;        // if 0, interface buffer not used 
    void*          VFuncOffsets;               // call offsets to virtual functions used in kernel program

    // Helper field containing a pointer to the compiled shader object. 
    // IVB-specific. Should not be used on HSW+ platforms.
    void*   m_pShaderHandle;

    // Constant Buffer to Constant Register gather map
    SConstantGatherEntry*   m_pGatherConstants;
    // Number of entries in gather constants map. The number of entries is always even 
    // which makes the gather constants map size a multiple of unsigned int.
    unsigned int    m_GatherConstantsSize;          // if 0, gather map not used
    // Bitmap of valid constant buffers in the push constants gather.
    // Specifies which of the 16 constant buffers are used in the push constants gather.
    // If a bit is set it indicates the corresponding constant buffer is used. 
    // If a bit is clear it indicates the corresponding constant buffer is not used.
    // ### DW1 3DSTATE_GATHER_CONSTANT_* ###
    unsigned short  m_GatherConstantsBufferValid;   // if 0, gather buffer not used

    bool m_IsMessageTargetDataCacheDataPort;
    
    // USC enables this to indicate that it expects that the VE component packing
    // has been applied to the delivered thread's payload
    // ### Gen9+: (DW0, bit 9) 3DSTATE_VF ###
    bool ComponentPackingEnable;

    // ### DW1 3DSTATE_CONSTANT_* ###
    unsigned int    m_ConstantBuffer1ReadLength;    // Constant Buffer 1 Read Length (DW1, bit 31..16)
                                                    // In 256-bit units. If 0, gather map not used.

    // Mask of constant buffers accessed by kernel (if BIT#n==0, kernel does not access CB#n)
    // CB usage can change after applying ConstantBuffersToConstantRegisters optimization.
    unsigned int    m_ConstantBufferAccessed;

    // Bitmask that indicates which MSAA level is used for UAV load/store
    unsigned int m_MsaaUAVMask;

    // Additional UAV Binding Table Entries to be used for emulation
    // of Typed UAV loads from surface formats unsupported by hardware.
    unsigned int                    m_TypedUAVReadEmulationEntriesSize;
    STypedUAVReadEmulationEntry*    m_pTypedUAVReadEmulationEntries;

    unsigned int    m_ShaderHash;
    unsigned int    m_ShaderOrdinal;
    unsigned int    m_CompileNum;

    // ISA to IL map.
    unsigned int    m_ISA2ILMapSize[3];
    void*           m_pISA2ILMap[3];
    
    // Bitmask of shader resources accessed by gather4 instructions
    // with green channel select and not accessed by any other then gather4
    // instruction type. This bitmask is a part of 
    // the WaGather4WithGreenChannelSelectOnR32G32Float workaround. 
    // DW0 - bitmask of resource indexes  0 - 31 
    // DW1 - bitmask of resource indexes 32 - 63
    // DW2 - bitmask of resource indexes 64 - 95
    // DW3 - bitmask of resource indexes 96 - 127
    unsigned int m_WaGather4WithGreenResourceMask[4];

    // Bitmask of shader resources accessed by sample_c instructions. This 
    // field is only used when shader compiler was created with the 
    // EnableWaCheckResourceFormatForNFSRivals bit set.
    unsigned int m_SampleWithComparisonResourceMask[4];

    int             m_UAVSlotsDeclared;             // true if one or more UAVs declared
    unsigned int    m_ResourceSlotMask[4];

    // Component(channel) mask provided in the least significant nibble of each table element
    // ### Gen9+: DW1-DW4 3DSTATE_VF_COMPONENT_PACKING  ###
    unsigned int ElementComponentDeliverMask[ NUM_VSHADER_INPUT_REGISTERS_PACKAGEABLE ];

    // Same as above mask but HW-agnostic. Used for cross-shader optimizations.
    unsigned int ElementComponentUseMask[ NUM_VSHADER_INPUT_REGISTERS_PACKAGEABLE ];

    // Bitmask of input registers that are *used* by the shader.
    // The field ElementComponentDeliverMask contains 4-bit nibbles. Subsequent 
    // nibbles are referring to subsequent bits set in this mask. In other words,
    // for bits cleared in this mask, nibbles are omitted from the field
    // ElementComponentDeliverMask (only nibbles for bits set here are present).
    unsigned int ElementDeliverMask;

    unsigned long long       m_UAVSlotsWAppendConsume;       // used as bitfield, each bit 
    // represent UAV slot that is 
    // referenced with Append/Consume.

    // Planar YUV formats NOS data.
    // For each texture with index 'i' declared as planar YUV by
    // SGen6PixelShaderKernelProgramCacheKey.SetPlanarYUVFormat(i, ...)
    // this table keeps resource numbers of the additional planes used by sampling.
    // If three separate planes are defined, Y is at the original texture index 
    // while indices of U and V are given in this table. When Y, V channels are 
    // packed in one plane, both indices are set to the same resource number.
    // E.g.:
    // For YV12:
    // m_planarTextureResourceIndex[i][0] -- index of V plane resource,
    // m_planarTextureResourceIndex[i][1] -- index of U plane resource.
    // for NV12:
    // m_planarTextureResourceIndex[i][0] -- index of interleaved U+V plane resource,
    // m_planarTextureResourceIndex[i][1] -- unused, same as [i][0]
    //
    // If no planar YUV format is defined, both table entries are set to i.
    unsigned int    m_planarTextureResourceIndex[NUM_TEXTURE_SLOTS][NUM_EXTRA_PLANES];

    // Max binding table index used for stateful, non-TGSM resources
    unsigned int m_MaxBindingTableIndex;

    // If we have indirect sampling and >16 samplers we need to use even slots only, so double the amount
    bool m_IsUsingDoubleSamplerSlots;

    // Indicates if the shader has any control flow
    bool m_hasControlFlow;

    bool m_UsesTextureFences;
    PADDING_1_BYTE

    // used by GenUpdateCB
    void*          m_ConstantBufferReplaceShaderPatterns;
    unsigned int   m_ConstantBufferReplaceShaderPatternsSize;
    unsigned int   m_ConstantBufferUsageMask;
    unsigned int   m_ConstantBufferReplaceSize;
    PADDING_4_BYTES_x64_ONLY
};

/*****************************************************************************\
\*****************************************************************************/
USC_PARAM()
struct SCompilerOutputVertexShader_Gen7 : public SCompilerOutputCommon_Gen7
{
    // ### DW1 3DSTATE_VS ###
    void*   m_pKernelProgram;           // Kernel Start Pointer         (DW1, bit 31..6)
    unsigned int    m_KernelProgramSize;

    // ### DW2 3DSTATE_VS ###
    int    m_SingleProgramFlow;        // Single Program Flow          (DW2, bit 31)
    unsigned int    m_SamplerCount;             // Sampler Count                (DW2, bit 29..27)
    unsigned int    m_BindingTableEntryCount;   // Binding Table Entry Count    (DW2, bit 25..18)
                                        // Gen7 and Gen7.5+ with HW binding table generation disabled.
    unsigned int    m_BindingTableEntryBitmap;  // Binding Table Entry Count    (DW2, bit 25..18)
                                        // Gen7.5+ with HW binding table generation enabled.
    GFX3DSTATE_FLOATING_POINT_MODE  m_FloatingPointMode; //Floating Point Mode (DW2, bit 16)

    // ### DW3 3DSTATE_VS ###
    unsigned int    m_PerThreadScratchSpace;    // Per-Thread Scratch Space     (DW3, bit 3..0)

    // ### DW4 3DSTATE_VS ###
    unsigned int    m_DispatchGRFStartRegister; // Dispatch GRF Start Register  (DW4, bit 24..20)
    unsigned int    m_VertexURBEntryReadLength; // Vertex URB Entry Read Length (DW4, bit 16..11)
    unsigned int    m_VertexURBEntryReadOffset; // Vertex URB Entry Read Offset (DW4, bit 9..4)
    
    // ### DW5 3DSTATE_VS ###
    unsigned int    m_MaxNumberThreads;         // Maximum Number Of Threads    (DW5, bit 31..25) 

    // ### DW1 3DSTATE_SBE ###
    unsigned int   m_SBEVertexURBEntryReadOffset; // Vertex URB Entry Read Offset in 256bit values (DW1, bit 9..4)

    // Other    
    unsigned int    m_URBAllocationSize;
    unsigned int    m_URBEntryWriteLength;
    unsigned int    m_URBEntriesPerHandle;

    int             m_HasInstanceID;
    int             m_HasVertexID;

    unsigned int    m_InstanceIDIndex;
    unsigned int    m_VertexIDIndex;

    unsigned int    m_InstanceIDMask;
    unsigned int    m_VertexIDMask;

    unsigned int    m_UserClipDistancesMask;
    unsigned int    m_UserCullDistancesMask;
    unsigned int    m_AntiAliasTextureCoordinateId;
    unsigned int    m_VsMaxNumInputRegisters;

    int             m_DeclaresVPAIndex;
    int             m_DeclaresRTAIndex;

    unsigned int    m_InstructionCount;

    PADDING_4_BYTES_x32_ONLY
};

/*****************************************************************************\
\*****************************************************************************/
USC_PARAM()
struct SCompilerOutputHullShader_Gen7 : public SCompilerOutputCommon_Gen7
{
    // ### DW1 3DSTATE_HS ###
    unsigned int   m_SamplerCount;             //Sampler Count                 (DW1, bit 29..27)
    unsigned int   m_BindingTableEntryCount;   //Binding Table Entry Count     (DW1, bit 25..18)
                                        //Gen7 and Gen7.5+ with HW binding table generation disabled.
    unsigned int   m_BindingTableEntryBitmap;  //Binding Table Entry Count     (DW2, bit 25..18)
                                        //Gen7.5+ with HW binding table generation enabled.
    unsigned int   m_MaxNumberThreads;         //Maximum Number Of Threads     (DW1, bit 6..0) 

    // ### DW2 3DSTATE_HS ###
    int    m_HSEnable;                 //HS Enable                     (DW2, bit 31)
                                        //Statistics Enable             (DW2, bit 29)
    unsigned int   m_InstanceCount;            //InstanceCount                 (DW2, bit 7..0)

    // ### DW3 3DSTATE_HS ###
    void*   m_pKernelProgram;           //Kernel Start Pointer          (DW3, bit 31..6)
    unsigned int   m_KernelProgramSize;

    // ### DW4 3DSTATE_HS ###
    unsigned int   m_PerThreadScratchSpace;    //Per-Thread Scratch Space      (DW4, bit 3..0)

    // ### DW5 3DSTATE_HS ###
    int    m_SingleProgramFlow;        //Single Program Flow           (DW5, bit 27)
    int    m_IncludeVertexHandles;     //Include Vertex Handles        (DW5, bit 24)
    unsigned int   m_DispatchGRFStartRegister; //Dispatch GRF Start Register   (DW5, bit 23..19)
    unsigned int   m_VertexURBEntryReadLength; //Vertex URB Entry Read Length  (DW5, bit 16..11)
    unsigned int   m_VertexURBEntryReadOffset; //Vertex URB Entry Read Offset  (DW5, bit 9..4)

    // Other
    int    m_HasNOSDefaultTesselationFactors;
    unsigned int   m_URBAllocationSize;
    unsigned int   m_URBEntryWriteLength;
    unsigned int   m_URBEntriesPerHandle;

    int    m_AttributePullModelUsed;
    unsigned int   m_PatchConstantURBSize;
    unsigned int   m_NumberOutputControlPoints;
    unsigned int   m_NumberInputControlPoints;

    // Only used when VS is skipped and VF used directly in HS:
    int             m_HasInstanceID;
    unsigned int    m_InstanceIDIndex;
    unsigned int    m_InstanceIDMask;

    // ### 3DSTATE_TE related fields ###
    TESSELLATOR_PARTITIONING_TYPE      m_Partitioning;
    TESSELLATOR_OUTPUT_PRIMITIVE_TYPE  m_OutputPrimitive;
    TESSELLATOR_DOMAIN_TYPE            m_Domain;
    float                                       m_MaxTessFactor;
    unsigned int                                m_InstructionCount;

    PADDING_4_BYTES_x64_ONLY
};

/*****************************************************************************\
\*****************************************************************************/
USC_PARAM()
struct SCompilerOutputDomainShader_Gen7 : public SCompilerOutputCommon_Gen7
{
    // ### DW1 3DSTATE_DS ###
    void*   m_pKernelProgram;           //Kernel Start Pointer          (DW1, bit 31..6)
    unsigned int   m_KernelProgramSize;

    // ### DW2 3DSTATE_DS ###
    int    m_SingleProgramFlow;        //Single Program Flow           (DW2, bit 31)
    unsigned int   m_SamplerCount;             //Sampler Count                 (DW2, bit 29..27)
    unsigned int   m_BindingTableEntryCount;   //Binding Table Entry Count     (DW2, bit 25..18)
                                        //Gen7 and Gen7.5+ with HW binding table generation disabled.
    unsigned int   m_BindingTableEntryBitmap;  //Binding Table Entry Count     (DW2, bit 25..18)
                                        //Gen7.5+ with HW binding table generation enabled.

    // ### DW3 3DSTATE_DS ###
    unsigned int   m_PerThreadScratchSpace;    //Per-Thread Scratch Space      (DW3, bit 3..0)

    // ### DW4 3DSTATE_DS ###
    unsigned int   m_DispatchGRFStartRegister; //Dispatch GRF Start Register   (DW4, bit 24..20)
    unsigned int   m_PatchURBEntryReadLength;  //Patch URB Entry Read Length   (DW4, bit 17..11)
    unsigned int   m_PatchURBEntryReadOffset;  //Patch URB Entry Read Offset   (DW4, bit 9..4)

    // ### DW5 3DSTATE_DS ###
    unsigned int   m_MaxNumberThreads;         //Maximum Number Of Threads     (DW5, bit 31..25)
                                        //Statistics Enable             (DW5, bit 10)
    int    m_ComputeWAttribute;        //Compute W Coordinate Enable   (DW5, bit 2)
    int    m_DSCacheDisable;           //DS Cache Disable              (DW5, bit 1)
    int    m_DSEnable;                 //DS Enable                     (DW5, bit 0)

    // ### DW1 3DSTATE_SBE ###
    unsigned int   m_SBEVertexURBEntryReadOffset; // Vertex URB Entry Read Offset in 256bit values (DW1, bit 9..4)

    // Other
    unsigned int    m_URBAllocationSize;
    unsigned int    m_URBEntryWriteLength;
    unsigned int    m_URBEntriesPerHandle;

    unsigned int    m_UserClipDistancesMask;
    unsigned int    m_UserCullDistancesMask;

    // ### 3DSTATE_TE related fields ###
    TESSELLATOR_DOMAIN_TYPE                m_Domain;
    TESSELLATOR_PARTITIONING_TYPE          m_Partitioning;
    TESSELLATOR_OUTPUT_PRIMITIVE_TYPE      m_OutputPrimitive;
    unsigned int    m_InstructionCount;

    bool            m_DeclaresVPAIndex;
    bool            m_DeclaresRTAIndex;

    PADDING_2_BYTES
    PADDING_4_BYTES_x32_ONLY
};

/*****************************************************************************\
\*****************************************************************************/
USC_PARAM()
struct SCompilerOutputGeometryShader_Gen7 : public SCompilerOutputCommon_Gen7
{
    // ### DW1 3DSTATE_GS ###
    void*   m_pKernelProgram[2];        //Kernel Start Pointer          (DW1, bit 31..6)
                                        //[Rendering Disabled|Enabled]
    unsigned int   m_KernelProgramSize[2];     //[Rendering Disabled|Enabled]

    // ### DW2 3DSTATE_GS ###
    int    m_SingleProgramFlow;        //Single Program Flow           (DW2, bit 31)
    unsigned int   m_SamplerCount;             //Sampler Count                 (DW2, bit 29..27)
    unsigned int   m_BindingTableEntryCount;   //Binding Table Entry Count     (DW2, bit 25..18)
                                        //Gen7 and Gen7.5+ with HW binding table generation disabled.
    unsigned int   m_BindingTableEntryBitmap;  //Binding Table Entry Count     (DW2, bit 25..18)
                                        //Gen7.5+ with HW binding table generation enabled.

    // ### DW3 3DSTATE_GS ###
    unsigned int   m_PerThreadScratchSpace;    //Per-Thread Scratch Space      (DW3, bit 3..0)

    // ### DW4 3DSTATE_GS ###    
    unsigned int   m_OutputVertexSize[2];      //Output Vertex Size            (DW4, bit 28..23)
                                        //[Rendering Disabled|Enabled]
    GFX3DPRIMITIVE_TOPOLOGY_TYPE   m_OutputTopology;  //Output Topology (DW4, bit 22..17)
    unsigned int   m_VertexEntryReadLength;    //Vertex URB Entry Read Length  (DW4, bit 16..11)
    int    m_IncludeVertexHandles;     //Include Vertex Handles        (DW4, bit 10)
    unsigned int   m_VertexEntryReadOffset;    //Vertex URB Entry Read Offset  (DW4, bit 9..4)
    unsigned int   m_DispatchGRFStartRegister; //Dispatch GRF Start Register   (DW4, bit 3..0)      
    
    // ### DW5 3DSTATE_GS ###
    unsigned int   m_MaxNumberThreads;         //Maximum Number Of Threads     (DW5, bit 31..25) 
    GFX3DSTATE_CONTROL_DATA_FORMAT m_ControlDataFormat; //Control Data Format (DW5, bit 24) 
    unsigned int   m_ControlDataHeaderSize;    //Control Data Header Size      (DW5, bit 23..20) 
    unsigned int   m_InstanceControl;          //Instance Control              (DW5, bit 19..15)
    unsigned int   m_DefaultStreamId;          //Default Stream ID             (DW5, bit 14..13)      
    GFX3DSTATE_GEOMETRY_SHADER_DISPATCH_MODE m_DispatchMode; //Control Data Format (DW5, bit 12..11) 
                                        //Statistics Enable             (DW5, bit 10)
                                        //GS Invocations Increment Value(DW5, bit 9..5)
    int    m_IncludePrimitiveIdEnable; //Include PrimitiveId Enable    (DW5, bit 4)
                                        //Rendering Enable Hint         (DW5, bit 3)
    int    m_ReorderEnable;            //Reorder Enable                (DW5, bit 2)
    int    m_DiscardAdjacencyEnable;   //Discard Adjacency Enable      (DW5, bit 1)
    int    m_GSEnable;                 //GS Enable                     (DW5, bit 0)                                            

    // ### DW1 3DSTATE_SBE ###
    unsigned int    m_SBEVertexURBEntryReadOffset; // Vertex URB Entry Read Offset in 256bit values (DW1, bit 9..4)

    // Other
    unsigned int    m_URBAllocationSize;
    unsigned int    m_URBEntryWriteLength; 
    unsigned int    m_URBEntriesPerHandle;

    unsigned int    m_UserClipDistancesMask;
    unsigned int    m_UserCullDistancesMask;
    unsigned int    m_MaxOutputVertexCount;

    unsigned int    m_InstructionCount;

    bool            m_DeclaresVPAIndex;
    bool            m_DeclaresRTAIndex;

    PADDING_2_BYTES
    PADDING_4_BYTES
};

/*****************************************************************************\
\*****************************************************************************/
USC_PARAM()
struct SCompilerOutputPixelShader_Gen7 : public SCompilerOutputCommon_Gen7
{
    // ### DW1, DW6, DW7 3DSTATE_PS ###
                                        
    void*           m_pKernelProgram[NUM_PS_DISPATCH_TYPES];
    unsigned int    m_KernelProgramSize[NUM_PS_DISPATCH_TYPES];
    int             m_EnablePixelDispatch[NUM_PS_DISPATCH_TYPES];

    unsigned int    m_NumberOfSFOutputAttributes;

    // ### DW2 3DSTATE_PS ###
    GFX3DSTATE_FLOATING_POINT_MODE  m_FloatingPointMode;             // (DW2, bit 16)
    unsigned int    m_BindingTableEntryCount;        // Binding Table Entry Count    (DW2, bit 25..18)
                                        // Gen7 and Gen7.5+ with HW binding table generation disabled.
    unsigned int    m_BindingTableEntryBitmap;  // Binding Table Entry Count    (DW2, bit 25..18)
                                        // Gen7.5+ with HW binding table generation enabled.
    unsigned int    m_SamplerCount;             // Sampler Count                (DW2, bit 29..27)

    int             m_VectorMaskEnable; // Gen8+ need to know if we use VMASK or DMASK for pixel dispatch

    GFX3DSTATE_PROGRAM_FLOW  m_SingleProgramFlow;              // (DW2, bit 31)

    // ### DW3 3DSTATE_PS ###
    unsigned int    m_PerThreadScratchSpace;    // Per-Thread Scratch Space     (DW3, bit 3..0)

    // ### DW4 3DSTATE_PS ###
    GFX3DSTATE_POSITIONXY_OFFSET   m_PositionXYOffset;               // (DW4, bit 4..3)
    int             m_HasOMaskOutput;           // OMask Present to RT          (DW4, bit 9)
    int             m_AttributeEnable;          // Attribute Enable             (DW4, bit 10)
    int             m_PushConstantEnable;       // Push Constant Enable         (DW4, bit 11)
    unsigned int    m_SampleMask;               // Sample Mask, for Gen7.5 only (DW4, bit 19..12) 
    unsigned int    m_MaxNumberThreads;         // Maximum Number Of Threads    (DW4, bit 31..23) 

    // ### DW5 3DSTATE_PS ###
                                        // Dispatch GRF Start Registers For Constant/Setup Data
    unsigned int    m_DispatchGRFStartRegForConstSetupData[NUM_PS_DISPATCH_TYPES]; // (DW5, bit 22..16, 14..8, 6..0)

    // ### DW1 3DSTATE_WM ###
    int    m_UsesInputCoverageMask;                                 // (DW1, bit 10)
    int    m_RequiresBarycentricPerspectivePixelLocation;           // (DW1, bit 11)
    int    m_RequiresBarycentricPerspectiveCentroid;                // (DW1, bit 12)
    int    m_RequiresBarycentricPerspectiveSample;                  // (DW1, bit 13)
    int    m_RequiresBarycentricNonPerspectivePixelLocation;        // (DW1, bit 14)
    int    m_RequiresBarycentricNonPerspectiveCentroid;             // (DW1, bit 15)
    int    m_RequiresBarycentricNonPerspectiveSample;               // (DW1, bit 16)

    GFX3DSTATE_POSITIONZW_INTERPOLATION_MODE  m_PositionZWInterpolationMode; // (DW1, bit 18..17)
    int    m_UsesSourceW;             // PS Uses Source W              (DW1, bit 19)
    int    m_UsesSourceDepth;         // PS Uses Source Depth          (DW1, bit 20)
    GFX3DSTATE_EARLY_DEPTH_STENCIL_CONTROL  m_EarlyDepthStencilControl; //  (DW1, bit 22..21)
    GFX3DSTATE_COMPUTED_DEPTH_MODE  m_ComputedDepthMode;             // (DW1, bit 24..23)
    int    m_KillsPixel;              // PS Kill Pixel                 (DW1, bit 25)

    int             m_HasStoreOrAtomicInstructions;
    unsigned int    m_RenderTargetMask;

    // ### DW2 3DSTATE_WM ###
    int    m_UAVOnly;                                               // (DW2, bit 30)

    // Other
    int             m_KernelIsPerSample;
    int             m_HasNOSInputSampleIndex;
    int             m_HasNOSUnlitCentroidInterpolation;
    int             m_HasPrimitiveIdInput;
    unsigned int    m_PrimitiveIdIndex;
    int             m_OverrideX;
    int             m_OverrideY;
    int             m_OverrideZ;
    int             m_OverrideW;
    unsigned int    m_SamplersUsageMask;

    unsigned int    m_ConstantInterpolationEnableMask;

    // Fields used by the sample_c workaround.
    // IVB-specific
    int             m_HasSampleCmpWaCandidates; // shader kernel has sample_c instructions eligible for the wa
    int             m_SampleCmpWaRequiresSingleLODResources; // sampled resources must have only 1 LOD or have MIP filter disabled
    int             m_SampleCmpWaSampler; // index of the sampler used by sample_c instructions
    unsigned int    m_SampleCmpWaResourcesMask[4]; // bitmap of resources sampled by sample_c instructions
    
    unsigned int    m_InstructionCount[NUM_PS_DISPATCH_TYPES];

    bool            m_HigherSIMDRecommended;        // True if a compilation in higher SIMD can be beneficial.
    bool            m_HasSampleInfoInstruction;      // True if pixel shader uses samplepos instruction.
    
    // Used by SWStencil
    bool            m_IsSWStencilPossible;
    bool            m_NeedMSAARate;

    PADDING_4_BYTES_x64_ONLY;
};

/*****************************************************************************\
\*****************************************************************************/
USC_PARAM()
struct SCompilerOutputComputeShader_Gen7 : public SCompilerOutputCommon_Gen7

{
    // ### DW0 INTERFACE_DESCRIPTOR_DATA ###
    void*   m_pKernelProgram;           // Kernel Start Pointer         (DW0, bit 31..6)
    unsigned int   m_KernelProgramSize;

    // ### DW1 INTERFACE_DESCRIPTOR_DATA ###
    GFX3DSTATE_FLOATING_POINT_MODE   m_FloatingPointMode;            // (DW1, bit 16)
    GFX3DSTATE_PROGRAM_FLOW   m_SingleProgramFlow;                   // (DW1, bit 18)
    
    // ### DW2 INTERFACE_DESCRIPTOR_DATA ###
    unsigned int   m_SamplerCount;             // Sampler Count                (DW2, bit 4..2)

    // ### DW3 INTERFACE_DESCRIPTOR_DATA ###
    unsigned int   m_BindingTableEntryCount;        // Binding Table Entry Count    (DW3, bit 4..0)

    // ### DW4 INTERFACE_DESCRIPTOR_DATA ###
    unsigned int   m_CurbeReadOffset;          // Constant URB Entry Read Offset (DW4, bit 15..0)
    unsigned int   m_CurbeReadLength;          // Constant URB Entry Read Length (DW4, bit 31..16)

    // ### DW5 INTERFACE_DESCRIPTOR_DATA ###
    unsigned int   m_PhysicalThreadsInGroup;   // Number of Threads in GPGPU Thread Group
                                                                     // (DW5, bit 7..0)
    unsigned int   m_BarrierReturnByte;        // Barrier Return Byte          (DW5  bit 15..8)
    int    m_BarrierUsed;              // Barrier Enable               (DW5, bit 21)
    GFX3DSTATE_ROUNDING_MODE   m_RoundingMode;      // Rounding Mode    (DW5, bit 23..22)
    unsigned int   m_BarrierReturnGrfOffset;   // Barrier Return GRF Offset    (DW5, bit 31..24)
    
    // ### DW6 INTERFACE_DESCRIPTOR_DATA [DevHSW] ###
    unsigned int   m_ThreadConstantDataReadLength; // [DevHSW] Cross-Thread Constant Data Read Length                                            
                                            //                          (DW6  bit 7..0)
                                            // [PreDevHSW] Per Thread Constant Data in 256bit units

    // ### DW1 MEDIA_VFE_STATE ###
    unsigned int   m_PerThreadScratchSpace;    // Per-Thread Scratch Space     (DW1, bit 3..0)

    // ### DW2 MEDIA_VFE_STATE ###
    GFXMEDIA_GPGPU_MODE    m_GPGPUMode; // GPGPU Mode                   (DW2, bit 2)
    GFXMEDIA_MMIO_ACCESS_CONTROL   m_GtwMMIOAccess;                  // (DW2, bit 4..3) 
    int    m_FastPreempt;              // Fast Preempt                 (DW2, bit 5) 
    int    m_GtwBypass;                // Bypass Gateway Control       (DW2, bit 6) 
    int    m_GtwResetTimer;            // Reset Gateway Timer          (DW2, bit 7) 
    unsigned int   m_URBEntriesNum;            // Number of URB Entries        (DW2, bit 15..8) 
    unsigned int   m_MaxNumberThreads;         // Maximum Number Of Threads    (DW2, bit 31..16)

    // ### DW3 MEDIA_VFE_STATE ###
    unsigned int   m_URBEntryAllocationSize;   // URB Entry Allocation Size    (DW3, bit 31..16)

    // ### DW2 MEDIA_CURBE_LOAD ###
    unsigned int   m_CurbeTotalDataLength;     // CURBE Total Data Length      (DW2, bit 16..0)

    // ### DW3 MEDIA_CURBE_LOAD ###
    unsigned int   m_CurbeDataOffset;          // CURBE Data Start Address     (DW3, bit 31..0)

    // ### DW2 GPGPU_WALKER ###
    GFXMEDIA_GPUWALKER_SIMD   m_SimdWidth;    // SIMD size              (DW2, bit 31..30)

    // Other (driver has to interpret fields listed below, not for direct copy to HW command).
    unsigned int   m_TgsmTotalByteCount;
    unsigned int   m_ThreadGroupSize;          // Number of threads in declared thread group

    // This member indicates which channel do we use
    // for threads' dispatch in Compute Shaders.
    unsigned int m_CSHThreadDispatchChannel;

    void*   m_pThreadPayloadData;       // Thread payload data to be sent in a CURBE. Size = m_CurbeTotalDataLength

    // Is set if compiled for Indirect thread payload
    bool    m_CompiledForIndirectPayload;

    bool    m_DispatchAlongY;

    bool m_performSecondCompile;    // Indicate to the driver if a second compilation is needed for CS.
    bool m_rowMajor;                // Indicate whether this is a row major or column major optimization

    unsigned int    m_InstructionCount;

    int  m_numChannelsUsed;         // Indicate the number of channels loaded from each resource.

    PADDING_4_BYTES
};

} // namespace USC
