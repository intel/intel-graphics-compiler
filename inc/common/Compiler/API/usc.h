/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef STRUCTURE_ALINGMENT_VERIFICATION
#pragma once
#endif

#include "../../igfxfmid.h"
#include "usc_config.h"
#include "CppParserMacros.h"
#include "../../gtsysinfo.h"
#ifndef _USC_
#include "../../sku_wa.h"
#endif

// redefine simple types to avoid dependency on external headers
#if defined( _WIN32 )
    typedef unsigned long   DWORD;
    typedef unsigned long   ULONG, *PULONG;
#else
    typedef unsigned int    DWORD;
    typedef unsigned int    ULONG, *PULONG;
#endif

    typedef unsigned short  USHORT, *PUSHORT;
    typedef unsigned short  WORD, *PWORD;

// Note that this out of USC namespace part is used by .c files.

#ifdef __cplusplus
USC_PARAM()
typedef PLATFORM PLATFORM;
#endif // __cplusplus

// Slimmed version of the full GT_SYSTEM_INFO structure ( in inc/umKmInc/sharedata.h ).
USC_PARAM()
typedef struct _SUscGTSystemInfo
{
    // Fields from GT_SYSTEM_INFO structure which contains actual,current number of EU and number of Threads.
    unsigned int    EUCount;            // Total no. of enabled EUs.
    unsigned int    ThreadCount;        // Total no. of system threads available.
    unsigned int    SliceCount;         // Total no. of enabled slices
    unsigned int    SubSliceCount;      // Total no. of enabled subslices.
    unsigned int    SLMSizeInKb;        // SLM Size

    bool           IsDynamicallyPopulated;         // System details populated either via fuse reg. (TRUE) or hard-coded (FALSE)

    unsigned int   TotalPsThreadsWindowerRange;
    unsigned int   TotalVsThreads;
    unsigned int   TotalVsThreads_Pocs;
    unsigned int   TotalGsThreads;
    unsigned int   TotalDsThreads;
    unsigned int   TotalHsThreads;
    unsigned int   MaxEuPerSubSlice;
    unsigned int   EuCountPerPoolMax;
    unsigned int   EuCountPerPoolMin;
    unsigned int   MaxSlicesSupported;
    unsigned int   MaxSubSlicesSupported;
    unsigned int   CsrSizeInMb;
} SUscGTSystemInfo;

// This slimmed version of the full sku feature table ( in sku_wa.h ).
USC_PARAM()
typedef struct _SUscSkuFeatureTable
{
    //...//
    // flags 1 = available, 0 = not available

    // struct _sku_Core
    unsigned int   FtrDesktop                       : 1;  // Whether Desktop
    unsigned int   FtrChannelSwizzlingXOREnabled    : 1;  // Indicates Channel Swizzling XOR feature support
    //...//
    unsigned int   FtrGtBigDie                      : 1;  // Indicate Big Die Silicon
    unsigned int   FtrGtMediumDie                   : 1;  // Indicate Medium Die Silicon
    unsigned int   FtrGtSmallDie                    : 1;  // Indicate Small Die Silicon
    //...//
    unsigned int   FtrGT1                           : 1;  // Indicates GT1 part
    unsigned int   FtrGT1_5                         : 1;  // Indicates GT1.5 part
    unsigned int   FtrGT2                           : 1;  // Indicates GT2 part
    unsigned int   FtrGT3                           : 1;  // Indicates GT3 part
    unsigned int   FtrGT4                           : 1;  // Indicates GT4 part
    //...//
    unsigned int   FtrIVBM0M1Platform               : 1;  // Indicates whether the platform in IVB M0/M1
    unsigned int   FtrGTL                           : 1;  // Indicates GT Low-end performance part  - New for HSW
    unsigned int   FtrGTM                           : 1;  // Indicates GT Medium performance part   - New for HSW
    unsigned int   FtrGTH                           : 1;  // Indicates GT High-end performance part - New for HSW
    unsigned int   FtrSGTPVSKUStrapPresent          : 1;  // Switchable Graphics Present
    unsigned int   FtrGTA                           : 1;  // Indicates the platform is a Gen9 based LCLP Broxton platform A
    unsigned int   FtrGTC                           : 1;  // Indicates the platform is a Gen9 based LCLP Broxton platform C
    unsigned int   FtrGTX                           : 1;  // Indicates the platform is a Gen9 based LCLP Broxton platform X
    unsigned int   Ftr5Slice                        : 1;  // Indicates KBL 15x8 SKU
    //...//
    unsigned int   FtrGpGpuMidThreadLevelPreempt    : 1;  // Indicates thread level batch Preemption
    unsigned int   FtrIoMmuPageFaulting             : 1;  // Indicates when PageFaultind is enabled
    unsigned int   FtrWddm2Svm : 1;   // WDDMv2 SVM Model (Set in platform SKU files, but disabled by GMM as appropriate for given system.)
    unsigned int   FtrPooledEuEnabled : 1;

    unsigned int   FtrResourceStreamer : 1;
    unsigned int   FtrLocalMemory : 1;
} SUscSkuFeatureTable;

USC_PARAM()
typedef struct _SUscAilInfo
{
    unsigned int EnableWaCheckResourceFormatForNFSRivals          : 1;  // Enables the WaCheckResourceFormatForNFSRivals w/a based on UMD AIL
    unsigned int WaDisableUnsafeArithmeticOperationRefactoring    : 1;  // Holds the WaDisableUnsafeArithmeticOperationRefactoring w/a passed from UMD AIL
    unsigned int WaTrigFuncRangeReduction                         : 1;  // Compiler Workaround for affected games to do range reduction of trig functions
    unsigned int WaEnableTrivialEmulateSinCos                     : 1;  // Compiler Workaround for games that have issues with HW version of sin/cos
    unsigned int WaHiddenIndexableTempSlot                        : 1;  // Reserve extra space for indexable temp for out-of-bound access
} SUscAilInfo, SCompilerAilInfo;

USC_PARAM()
typedef struct _SUscAdapterInfo
{
    SUscSkuFeatureTable UscSkuFeatureTable;
    SUscGTSystemInfo    UscGTSystemInfo;
    SUscAilInfo         UscAilInfo;
} SUscAdapterInfo;

#ifdef _USC_
#ifndef SKU_FEATURE_TABLE
#define SKU_FEATURE_TABLE  SUscSkuFeatureTable
#endif
#endif

//Updated interface structure that will be used by DX9, DX10 and DX12
USC_PARAM()
 typedef struct _SCompilerPlatformInfo
{
    GT_SYSTEM_INFO          sysInfo;
    SCompilerAilInfo        AilInfo;
    SUscSkuFeatureTable     uscSkuFeatureTable; // This slimmed version of the full sku feature table ( in sku_wa.h )
    SKU_FEATURE_TABLE       skuFeatureTable;
} SCompilerPlatformInfo;

#if defined ICBE_LHDM || defined ICBE_LINUX
#undef SKU_FEATURE_TABLE
#endif

#ifdef __cplusplus
namespace USC
{
typedef SCompilerPlatformInfo SUSCCompilerPlatformInfo;
/*****************************************************************************\

Function:
    InitializeUscAdapterInfo

Description:
    Initializes the USC (slim) adapter info structure by coping required fields
    from the big sku table and gtSystemInfo structure.
    This is a helper function for USC clients.

Input:
    BigSkuTable  bigSkuTable - sku feature table containing to initialize
                              USC sku feature table.

    BigGTSystemInfo bigGTSystemInfo - GT_SYSTEM_INFO containing actuall data about
                                      EU and Thread count.
Output:
    SUscAdapterInfo &uscAdpaterInfo - USC adapter info structure correctly
                                       initialized.

\*****************************************************************************/
template<typename BigSkuTable, typename BigGTSystemInfo>
inline void InitializeUscAdapterInfo(
    const BigSkuTable   &bigSkuTable,
    const BigGTSystemInfo &bigGTSystemInfo,
    SUscAdapterInfo &uscAdpaterInfo )
{
    uscAdpaterInfo.UscSkuFeatureTable.FtrDesktop         = bigSkuTable.FtrDesktop;          // Whether Desktop

    uscAdpaterInfo.UscSkuFeatureTable.FtrGtBigDie        = bigSkuTable.FtrGtBigDie;         // Indicates Big Die Silicon.
    uscAdpaterInfo.UscSkuFeatureTable.FtrGtMediumDie     = bigSkuTable.FtrGtMediumDie;      // Indicates Medium Die Silicon.
    uscAdpaterInfo.UscSkuFeatureTable.FtrGtSmallDie      = bigSkuTable.FtrGtSmallDie;       // Indicates Small Die Silicon.
    uscAdpaterInfo.UscSkuFeatureTable.FtrGT1             = bigSkuTable.FtrGT1;              // Indicates GT1 part.
    uscAdpaterInfo.UscSkuFeatureTable.FtrGT1_5           = bigSkuTable.FtrGT1_5;            // Indicates GT1.5 part.
    uscAdpaterInfo.UscSkuFeatureTable.FtrGT2             = bigSkuTable.FtrGT2;              // Indicates GT2 part.
    uscAdpaterInfo.UscSkuFeatureTable.FtrGT3             = bigSkuTable.FtrGT3;              // Indicates GT3 part.
    uscAdpaterInfo.UscSkuFeatureTable.FtrGT4             = bigSkuTable.FtrGT4;              // Indicates GT4 part.
    uscAdpaterInfo.UscSkuFeatureTable.FtrGTL             = bigSkuTable.FtrGT1;              // Indicates GT Low-end performance part.
    uscAdpaterInfo.UscSkuFeatureTable.FtrGTM             = bigSkuTable.FtrGT2;              // Indicates GT Medium performance part.
    uscAdpaterInfo.UscSkuFeatureTable.FtrGTH             = bigSkuTable.FtrGT3;              // Indicates GT High-end performance part.
    uscAdpaterInfo.UscSkuFeatureTable.FtrIVBM0M1Platform = bigSkuTable.FtrIVBM0M1Platform;  // Indicates whether the platform in IVB M0/M1
    uscAdpaterInfo.UscSkuFeatureTable.FtrGTA             = bigSkuTable.FtrGTA;            // Indicates a Gen9 based LCLP Broxton platform A.
    uscAdpaterInfo.UscSkuFeatureTable.FtrGTC             = bigSkuTable.FtrGTC;            // Indicates a Gen9 based LCLP Broxton platform C.
    uscAdpaterInfo.UscSkuFeatureTable.FtrGTX             = bigSkuTable.FtrGTX;            // Indicates a Gen9 based LCLP Broxton platform X.
    uscAdpaterInfo.UscSkuFeatureTable.Ftr5Slice          = bigSkuTable.Ftr5Slice;       // Indicates KBL 15x8 SKU  HALO Sku
    uscAdpaterInfo.UscSkuFeatureTable.FtrGpGpuMidThreadLevelPreempt = bigSkuTable.FtrGpGpuMidThreadLevelPreempt; //Indicates if preEmption is enabled (HSW+)
    uscAdpaterInfo.UscSkuFeatureTable.FtrIoMmuPageFaulting = bigSkuTable.FtrIoMmuPageFaulting; //Indicates if page faulting is enabled.
    uscAdpaterInfo.UscSkuFeatureTable.FtrWddm2Svm = bigSkuTable.FtrWddm2Svm;
    uscAdpaterInfo.UscSkuFeatureTable.FtrPooledEuEnabled = bigSkuTable.FtrPooledEuEnabled;
    uscAdpaterInfo.UscSkuFeatureTable.FtrResourceStreamer = bigSkuTable.FtrResourceStreamer;
    uscAdpaterInfo.UscSkuFeatureTable.FtrLocalMemory = bigSkuTable.FtrLocalMemory;

    uscAdpaterInfo.UscGTSystemInfo.EUCount = bigGTSystemInfo.EUCount;
    uscAdpaterInfo.UscGTSystemInfo.ThreadCount = bigGTSystemInfo.ThreadCount;
    uscAdpaterInfo.UscGTSystemInfo.SliceCount = bigGTSystemInfo.SliceCount;
    uscAdpaterInfo.UscGTSystemInfo.SubSliceCount = bigGTSystemInfo.SubSliceCount;
    uscAdpaterInfo.UscGTSystemInfo.SLMSizeInKb = bigGTSystemInfo.SLMSizeInKb;
    uscAdpaterInfo.UscGTSystemInfo.TotalPsThreadsWindowerRange = bigGTSystemInfo.TotalPsThreadsWindowerRange;
    uscAdpaterInfo.UscGTSystemInfo.TotalVsThreads = bigGTSystemInfo.TotalVsThreads;
    uscAdpaterInfo.UscGTSystemInfo.TotalVsThreads_Pocs = bigGTSystemInfo.TotalVsThreads_Pocs;
    uscAdpaterInfo.UscGTSystemInfo.TotalDsThreads = bigGTSystemInfo.TotalDsThreads;
    uscAdpaterInfo.UscGTSystemInfo.TotalGsThreads = bigGTSystemInfo.TotalGsThreads;
    uscAdpaterInfo.UscGTSystemInfo.TotalHsThreads = bigGTSystemInfo.TotalHsThreads;
    uscAdpaterInfo.UscGTSystemInfo.MaxEuPerSubSlice = bigGTSystemInfo.MaxEuPerSubSlice;
    uscAdpaterInfo.UscGTSystemInfo.EuCountPerPoolMax = bigGTSystemInfo.EuCountPerPoolMax;
    uscAdpaterInfo.UscGTSystemInfo.EuCountPerPoolMin = bigGTSystemInfo.EuCountPerPoolMin;
    uscAdpaterInfo.UscGTSystemInfo.MaxSlicesSupported = bigGTSystemInfo.MaxSlicesSupported;
    uscAdpaterInfo.UscGTSystemInfo.MaxSubSlicesSupported = bigGTSystemInfo.MaxSubSlicesSupported;
    uscAdpaterInfo.UscGTSystemInfo.IsDynamicallyPopulated = bigGTSystemInfo.IsDynamicallyPopulated;
    uscAdpaterInfo.UscGTSystemInfo.CsrSizeInMb = bigGTSystemInfo.CsrSizeInMb;
}

/*****************************************************************************\

Function:
    InitializeUscSkuTable

Description:
    Initializes the USC (slim) sku table by coping required fields from the big
    sku table. This is a helper function for USC clients.

Input:
    BigSkuTable  bigSkuTable - sku feature table containing to initialize
                              USC sku feature table.

Output:
    SUscSkuFeatureTable &uscSkuTable - USC sku feature table correctly
                                       initialized.

\*****************************************************************************/
// Some API clients (OCL,DXVA) do not provide yet GT_SYSTEM_INFO structure.
// Shader Compiler object must be created with "old" way i.e. usage of fixed EU,Thread count values.
// The InitializeUscSkuTable method is called and uscAdpaterInfo.UscGTSystemInfo is zeroed.

// This method should be removed in the future.
template<typename BigSkuTable>
inline void InitializeUscSkuTable(
    const BigSkuTable   &bigSkuTable,
    SUscSkuFeatureTable &uscSkuTable )
{
    uscSkuTable.FtrDesktop         = bigSkuTable.FtrDesktop;          // Whether Desktop
    uscSkuTable.FtrChannelSwizzlingXOREnabled = bigSkuTable.FtrChannelSwizzlingXOREnabled; // Indicates Channel Swizzling XOR feature support

    uscSkuTable.FtrGtBigDie        = bigSkuTable.FtrGtBigDie;         // Indicates Big Die Silicon.
    uscSkuTable.FtrGtMediumDie     = bigSkuTable.FtrGtMediumDie;      // Indicates Medium Die Silicon.
    uscSkuTable.FtrGtSmallDie      = bigSkuTable.FtrGtSmallDie;       // Indicates Small Die Silicon.
    uscSkuTable.FtrGT1             = bigSkuTable.FtrGT1;              // Indicates GT1 part.
    uscSkuTable.FtrGT1_5           = bigSkuTable.FtrGT1_5;            // Indicates GT1.5 part.
    uscSkuTable.FtrGT2             = bigSkuTable.FtrGT2;              // Indicates GT2 part.
    uscSkuTable.FtrGT3             = bigSkuTable.FtrGT3;              // Indicates GT3 part.
    uscSkuTable.FtrGT4             = bigSkuTable.FtrGT4;              // Indicates GT4 part.
    uscSkuTable.FtrGTL             = bigSkuTable.FtrGT1;              // Indicates GT Low-end performance part.
    uscSkuTable.FtrGTM             = bigSkuTable.FtrGT2;              // Indicates GT Medium performance part.
    uscSkuTable.FtrGTH             = bigSkuTable.FtrGT3;              // Indicates GT High-end performance part.
    uscSkuTable.FtrIVBM0M1Platform = bigSkuTable.FtrIVBM0M1Platform;  // Indicates whether the platform in IVB M0/M1
    uscSkuTable.FtrGTA            = bigSkuTable.FtrGTA;            // Indicates a Gen9 based LCLP Broxton platform A.
    uscSkuTable.FtrGTC            = bigSkuTable.FtrGTC;            // Indicates a Gen9 based LCLP Broxton platform C.
    uscSkuTable.FtrGTX            = bigSkuTable.FtrGTX;            // Indicates a Gen9 based LCLP Broxton platform X.
    uscSkuTable.Ftr5Slice         = bigSkuTable.Ftr5Slice;       // Indicates KBL 15x8 SKU  HALO Sku
    uscSkuTable.FtrGpGpuMidThreadLevelPreempt = bigSkuTable.FtrGpGpuMidThreadLevelPreempt; //Indicates if preEmption is enabled (HSW+)
    uscSkuTable.FtrIoMmuPageFaulting = bigSkuTable.FtrIoMmuPageFaulting; //Indicates if page faulting is enabled.
    uscSkuTable.FtrLocalMemory = bigSkuTable.FtrLocalMemory;
}


USC_PARAM()
enum OPTIMIZER_LEVEL
{
    OPTIMIZER_LEVEL_0,     // -o0 fast compilation
    OPTIMIZER_LEVEL_1,     // -o1 full compilation (default)
    OPTIMIZER_LEVEL_2,     // -o2 specialized compilation
    USC_PARAM_HIDE()
    NUM_OPTIMIZER_LEVELS
};

USC_PARAM()
enum SIMD_LEVEL
{
    SIMD_LEVEL_DEFAULT,     // request all SIMD compilations at once
    SIMD_LEVEL_LOW,         // request SIMD8 only
    SIMD_LEVEL_HIGH,        // request all higher SIMD modes
    USC_PARAM_HIDE()
    NUM_SIMD_LEVELS
};

enum SHADER_TYPE
{
    VERTEX_SHADER,
    GEOMETRY_SHADER,
    PIXEL_SHADER,
    HULL_SHADER,
    DOMAIN_SHADER,
    COMPUTE_SHADER,
    NUM_SHADER_TYPES
};

enum SIMD_MODE
{
    SIMD_MODE_8 = 0,
    SIMD_MODE_16,
    SIMD_MODE_32,
    SIMD_MODE_4x2,
    NUM_SIMD_MODES
};

enum PS_DISPATCH_TYPES
{
    PS_SIMD8_DISPATCH,
    PS_SIMD16_DISPATCH,
    PS_SIMD32_DISPATCH,
    NUM_PS_DISPATCH_TYPES
};

enum USC_CLIENT_TYPE
{
    USC_CLIENT_D3D9,
    USC_CLIENT_D3D10,
    USC_CLIENT_D3D12,
    USC_CLIENT_OGL,
    USC_CLIENT_OCL,
    NUM_USC_CLIENT_TYPES
};

struct SShaderStageBTLayout
{
    // systemThreadIdx should be the same for all shader stages.
    unsigned int   systemThreadIdx;

    unsigned int   minConstantBufferIdx;
    unsigned int   maxConstantBufferIdx;
    unsigned int   streamOutmputStatisticsIdx;
    unsigned int   minStreamOutputBufferIdx;
    unsigned int   maxStreamOutputBufferIdx;
    unsigned int   minUAVIdx;          // minRTorUAVIdx
    unsigned int   maxUAVIdx;          // maxRTorUAVIdx
    unsigned int   minUAVCounterIdx;   // pre-DEVHSW
    unsigned int   maxUAVCounterIdx;   // pre-DEVHSW
    unsigned int   JournalIdx;         // journal resource index used by Kernel Trace / Profiling query to measure kernel execution time
    unsigned int   JournalCounterIdx;  // journal counter resource index
    unsigned int   TGSMIdx;
    unsigned int   minColorBufferIdx;
    unsigned int   maxColorBufferIdx;
    unsigned int   minResourceIdx;
    unsigned int   maxResourceIdx;
    unsigned int   NULLSurfaceIdx;
    unsigned int   RasterizerInfoSurfaceIdx;    // Special SS for SampleInfo on rasterizer0 for OGL.
    unsigned int   TPMIdx;
    unsigned int   surfaceScratchIdx;
    unsigned int   maxBTsize;

    // Three following fields are offsets from minConstantBufferIdx:
    // NULL CB offset should be programmed right after the
    // last constant buffer index. Such programming will allow USC
    // to correctly clamp indexable CB indexes (when relative constant
    // buffer addressing is used in a shader) and out of
    // bounds reads will return 0. Incorrect programming of this
    // field may cause out of bounds accesses not to return 0.
    unsigned int   constantBufferNullBoundOffset;
    unsigned int   immediateConstantBufferOffset;
    unsigned int   interfaceConstantBufferOffset;

    // Following field is an offset from minResourceIdx:
    // NULL resource offset should be programmed right after the
    // last shader resource index. Such programming will allow USC
    // to correctly clamp indexable resource indexes (when relative
    // shader resource addressing is used in a shader )and out of
    // bounds reads will return 0. Incorrect programming of this
    // field may cause out of bounds accesses not to return 0.
    unsigned int   resourceNullBoundOffset;

    // Passing this flags reroutes all BTI reads via Bindless heap from the shader
    // for SKL Bindless for DX Testing
    bool   BindLessBTIEnable;

    // Used to access the indirect draw arguments buffer.  Used by
    // geometry reordering optimization.
    unsigned int   indirectBufferOffset;
};

USC_PARAM()
struct SBindingTableLayout
{
    SShaderStageBTLayout m_Layout[NUM_SHADER_TYPES];
};

// global const to start with when defining custom BTI layout.
const SBindingTableLayout  g_cZeroBindingTableLayout  = {};
const SShaderStageBTLayout g_cZeroShaderStageBTLayout = {};

/*****************************************************************************\
DEFINE: GTDI_MAX_KI_OFFSETS
\*****************************************************************************/
#define GTDI_MAX_KI_OFFSETS 26

/*****************************************************************************\
DEFINE: GTDI_MAX_KI_AGGREGATED_OFFSETS
\*****************************************************************************/
#define GTDI_MAX_KI_AGGREGATED_OFFSETS 20

/*****************************************************************************\
ENUM: GTDI_KI_BUILD_TYPE_ENUM
\*****************************************************************************/
typedef enum GTDI_KI_BUILD_TYPE_ENUM
{
    GTDI_KERNEL_REGULAR                 = 0,   //use that value to switch off kernel build override
    GTDI_KERNEL_TRACE                   = 1,
    GTDI_KERNEL_PROFILE                 = 2,
    GTDI_KERNEL_ISA_PROFILE             = 3,
    GTDI_KERNEL_PROFILE_AGGREGATED      = 4,
    GTDI_KERNEL_ISA_PROFILE_AGGREGATED  = 5,
    GTDI_KERNEL_ISA_COUNTERS            = 6,
    GTDI_KERNEL_GT_PIN_COMPILER         = 7,
    GTDI_KERNEL_GPGPU_TRACE             = 8,
    GTDI_NUM_KERNEL_PROFILING_TYPES     = 9
} GTDI_KI_BUILD_TYPE;

/*****************************************************************************\
ENUM: GTDI_PROFILING_POINT_TYPE_ENUM
\*****************************************************************************/
typedef enum GTDI_KERNEL_PROFILING_POINT_TYPE_ENUM
{
    GTDI_PROFILING_POINT_TIMESTAMP_INTEL          =   1 << 0,
    GTDI_PROFILING_POINT_STALL_COUNTER_INTEL      =   1 << 1,
    GTDI_PROFILING_POINT_SAMPLER_MESSAGE_INTEL    =   1 << 2,
    GTDI_PROFILING_POINT_DATA_PORT_MESSAGE_INTEL  =   1 << 3,
    GTDI_PROFILING_POINT_WORKGROUP_ID_X_INTEL     =   1 << 4,
    GTDI_PROFILING_POINT_WORKGROUP_ID_Y_INTEL     =   1 << 5,
    GTDI_PROFILING_POINT_WORKGROUP_ID_Z_INTEL     =   1 << 6,
    GTDI_PROFILING_POINT_EXECUTION_MASK_INTEL     =   1 << 7,
    GTDI_PROFILING_POINT_HIT_COUNTER_INTEL        =   1 << 8
} GTDI_KERNEL_PROFILING_POINT_TYPE;

/*****************************************************************************\
STRUCT: STracingOptions
\*****************************************************************************/
struct STracingOptions
{
    unsigned int        InstrumentationType;
    unsigned int        KernelID;
    unsigned int        OffsetCount;
    unsigned int        Offsets[GTDI_MAX_KI_OFFSETS];
    unsigned int        OffsetType[GTDI_MAX_KI_OFFSETS];
    unsigned int        UseEUThreadMasks;
    unsigned int        PartitionCount;
    unsigned int        EUMask;
    unsigned int        ThreadCount;
    unsigned int        CfgID;
    unsigned int        GatherGatewayTimestamp;
    unsigned int        ShaderILCodeSize;
    void*               ShaderILCode;
    char*               KernelName;
};

/*****************************************************************************\
ENUM: GFX3DPRIMITIVE_TOPOLOGY_TYPE
\*****************************************************************************/
enum GFX3DPRIMITIVE_TOPOLOGY_TYPE
{
    GFX3DPRIM_POINTLIST         = 0x01,
    GFX3DPRIM_LINELIST          = 0x02,
    GFX3DPRIM_LINESTRIP         = 0x03,
    GFX3DPRIM_TRILIST           = 0x04,
    GFX3DPRIM_TRISTRIP          = 0x05,
    GFX3DPRIM_TRIFAN            = 0x06,
    GFX3DPRIM_QUADLIST          = 0x07,
    GFX3DPRIM_QUADSTRIP         = 0x08,
    GFX3DPRIM_LINELIST_ADJ      = 0x09,
    GFX3DPRIM_LINESTRIP_ADJ     = 0x0A,
    GFX3DPRIM_TRILIST_ADJ       = 0x0B,
    GFX3DPRIM_TRISTRIP_ADJ      = 0x0C,
    GFX3DPRIM_TRISTRIP_REVERSE  = 0x0D,
    GFX3DPRIM_POLYGON           = 0x0E,
    GFX3DPRIM_RECTLIST          = 0x0F,
    GFX3DPRIM_LINELOOP          = 0x10,
    GFX3DPRIM_POINTLIST_BF      = 0x11,
    GFX3DPRIM_LINESTRIP_CONT    = 0x12,
    GFX3DPRIM_LINESTRIP_BF      = 0x13,
    GFX3DPRIM_LINESTRIP_CONT_BF = 0x14,
    GFX3DPRIM_TRIFAN_NOSTIPPLE  = 0x16,
    GFX3DPRIM_PATCHLIST_1       = 0x20,
    GFX3DPRIM_PATCHLIST_2       = 0x21,
    GFX3DPRIM_PATCHLIST_3       = 0x22,
    GFX3DPRIM_PATCHLIST_4       = 0x23,
    GFX3DPRIM_PATCHLIST_5       = 0x24,
    GFX3DPRIM_PATCHLIST_6       = 0x25,
    GFX3DPRIM_PATCHLIST_7       = 0x26,
    GFX3DPRIM_PATCHLIST_8       = 0x27,
    GFX3DPRIM_PATCHLIST_9       = 0x28,
    GFX3DPRIM_PATCHLIST_10      = 0x29,
    GFX3DPRIM_PATCHLIST_11      = 0x2A,
    GFX3DPRIM_PATCHLIST_12      = 0x2B,
    GFX3DPRIM_PATCHLIST_13      = 0x2C,
    GFX3DPRIM_PATCHLIST_14      = 0x2D,
    GFX3DPRIM_PATCHLIST_15      = 0x2E,
    GFX3DPRIM_PATCHLIST_16      = 0x2F,
    GFX3DPRIM_PATCHLIST_17      = 0x30,
    GFX3DPRIM_PATCHLIST_18      = 0x31,
    GFX3DPRIM_PATCHLIST_19      = 0x32,
    GFX3DPRIM_PATCHLIST_20      = 0x33,
    GFX3DPRIM_PATCHLIST_21      = 0x34,
    GFX3DPRIM_PATCHLIST_22      = 0x35,
    GFX3DPRIM_PATCHLIST_23      = 0x36,
    GFX3DPRIM_PATCHLIST_24      = 0x37,
    GFX3DPRIM_PATCHLIST_25      = 0x38,
    GFX3DPRIM_PATCHLIST_26      = 0x39,
    GFX3DPRIM_PATCHLIST_27      = 0x3A,
    GFX3DPRIM_PATCHLIST_28      = 0x3B,
    GFX3DPRIM_PATCHLIST_29      = 0x3C,
    GFX3DPRIM_PATCHLIST_30      = 0x3D,
    GFX3DPRIM_PATCHLIST_31      = 0x3E,
    GFX3DPRIM_PATCHLIST_32      = 0x3F
};

/*****************************************************************************\
ENUM: GFX3DSTATE_PROGRAM_FLOW
\*****************************************************************************/
enum GFX3DSTATE_PROGRAM_FLOW
{
    GFX3DSTATE_PROGRAM_FLOW_MULTIPLE  = 0x0,
    GFX3DSTATE_PROGRAM_FLOW_SINGLE    = 0x1
};

/*****************************************************************************\
ENUM: GFX3DSTATE_FLOATING_POINT_MODE

Description:
    Indicates the floating point mode to be used by the hardware when running
    compiled kernel program.
\*****************************************************************************/
enum GFX3DSTATE_FLOATING_POINT_MODE
{
    GFX3DSTATE_FLOATING_POINT_IEEE_754        = 0x0,
    GFX3DSTATE_FLOATING_POINT_NON_IEEE_754    = 0x1
};


/*****************************************************************************\
ENUM: GFX3DSTATE_POSITIONXY_OFFSET
\*****************************************************************************/
enum GFX3DSTATE_POSITIONXY_OFFSET
{
    GFX3DSTATE_POSITIONXY_OFFSET_NONE       = 0x0,
    // Reserved                             = 0x1,
    GFX3DSTATE_POSITIONXY_OFFSET_CENTROID   = 0x2,
    GFX3DSTATE_POSITIONXY_OFFSET_SAMPLE     = 0x3
};

/*****************************************************************************\
ENUM: GFX3DSTATE_POSITIONZW_INTERPOLATION_MODE
\*****************************************************************************/
enum GFX3DSTATE_POSITIONZW_INTERPOLATION_MODE
{
    GFX3DSTATE_POSITIONZW_INTERPOLATION_PIXEL       = 0x0,
    // Reserved                                     = 0x1,
    GFX3DSTATE_POSITIONZW_INTERPOLATION_CENTROID    = 0x2,
    GFX3DSTATE_POSITIONZW_INTERPOLATION_SAMPLE      = 0x3
};


//////////////////////////////////////////////////////////////////////////////
enum RENDERSTATE_FOG_FUNCTION
{
    RENDERSTATE_FOG_VERTEX,
    RENDERSTATE_FOG_PIXEL_EXP,
    RENDERSTATE_FOG_PIXEL_EXP2,
    RENDERSTATE_FOG_PIXEL_LINEAR,
    NUM_RENDERSTATE_FOG_FUNCTIONS
};

//////////////////////////////////////////////////////////////////////////////
enum RENDERSTATE_FOG_SOURCE
{
    RENDERSTATE_FOG_SOURCE_Z,
    RENDERSTATE_FOG_SOURCE_W,
    RENDERSTATE_FOG_SOURCE_FOG_COORDINATE,
    NUM_RENDERSTATE_FOG_SOURCES
};

//////////////////////////////////////////////////////////////////////////////
enum RENDERSTATE_ALPHATEST_FORMAT
{
    RENDERSTATE_ALPHATEST_FORMAT_UNORM8,
    RENDERSTATE_ALPHATEST_FORMAT_FLOAT32,
    NUM_RENDERSTATE_ALPHATEST_FORMATS
};

/*****************************************************************************\
ENUM: MAPFILTER_TYPE
\*****************************************************************************/
enum MAPFILTER_TYPE
{
    MAPFILTER_TYPE_POINT,
    MAPFILTER_TYPE_LINEAR,
    NUM_MAPFILTER_TYPES
};

/*****************************************************************************\
ENUM: COMPARE_FUNCTION
\*****************************************************************************/
enum COMPARE_FUNCTION
{
    COMPARE_FUNCTION_ALWAYS,
    COMPARE_FUNCTION_NEVER,
    COMPARE_FUNCTION_LESS,
    COMPARE_FUNCTION_EQUAL,
    COMPARE_FUNCTION_LEQUAL,
    COMPARE_FUNCTION_GREATER,
    COMPARE_FUNCTION_NOTEQUAL,
    COMPARE_FUNCTION_GEQUAL,
    NUM_COMPARE_FUNCTIONS
};

/*****************************************************************************\
ENUM: STENCIL_PASS_OPERATION
\*****************************************************************************/
enum STENCIL_OPERATION
{
    STENCIL_OP_STENCILOP_KEEP = 0x0,
    STENCIL_OP_STENCILOP_ZERO = 0x1,
    STENCIL_OP_STENCILOP_REPLACE = 0x2,
    STENCIL_OP_STENCILOP_INCRSAT = 0x3,
    STENCIL_OP_STENCILOP_DECRSAT = 0x4,
    STENCIL_OP_STENCILOP_INCR = 0x5,
    STENCIL_OP_STENCILOP_DECR = 0x6,
    STENCIL_OP_STENCILOP_INVERT = 0x7
};

/*****************************************************************************\
STRUCT: SSWStencilParams
\*****************************************************************************/
struct SSWStencilParams
{
    unsigned char           CheckForSWStencil : 1;
    unsigned char           CompileForSWStencil : 1;
    COMPARE_FUNCTION        FrontFaceStencilFunc;
    COMPARE_FUNCTION        BackFaceStencilFunc;
    STENCIL_OPERATION       FrontFaceStencilPassOp;
    STENCIL_OPERATION  BackFaceStencilPassOp;

    SSWStencilParams()
    {
        CheckForSWStencil = 0;
        CompileForSWStencil = 0;
        FrontFaceStencilFunc = COMPARE_FUNCTION_ALWAYS;
        BackFaceStencilFunc = COMPARE_FUNCTION_ALWAYS;
        FrontFaceStencilPassOp = STENCIL_OP_STENCILOP_KEEP;
        BackFaceStencilPassOp = STENCIL_OP_STENCILOP_KEEP;
    }
};

struct NOSParams
{
    SSWStencilParams* pSWStencilParams;

    NOSParams()
    {
        pSWStencilParams = 0;
    }
};

/*****************************************************************************\
ENUM: INPUT_COVERAGE_MASK_MODE
\*****************************************************************************/
enum INPUT_COVERAGE_MASK_MODE
{
    INPUT_COVERAGE_MASK_MODE_NORMAL,
    INPUT_COVERAGE_MASK_MODE_INNER,
    INPUT_COVERAGE_MASK_MODE_DEPTH
};

/*****************************************************************************\
ENUM: SYSTEM_THREAD_MODE

Description:
    Enum type bitmask describing the System Thread mode. The System Thread might
    support shader debugging and/or the Context Save Restore (CSR) subroutine
    called GPGPU preemption.
\*****************************************************************************/
typedef enum SYSTEM_THREAD_MODE_ENUM
{
    SYSTEM_THREAD_MODE_NONE         = 0x0,
    SYSTEM_THREAD_MODE_DEBUG        = 0x1,
    SYSTEM_THREAD_MODE_CSR          = 0x2,
    SYSTEM_THREAD_MODE_DEBUG_LOCAL  = 0x4,
    SYSTEM_THREAD_MODE_CSR_64B      = 0x5
} SYSTEM_THREAD_MODE;

/*****************************************************************************\
\*****************************************************************************/
struct SSystemThreadKernelOutput
{
    void*                           m_pKernelProgram;           // Kernel Start Pointer
    unsigned int                    m_KernelProgramSize;
    unsigned int                    m_SystemThreadScratchSpace; // Scratch Space size in bytes
    unsigned int                    m_SystemThreadResourceSize; // Resource size in bytes
    void*                           m_pStateSaveAreaHeader;     // State save area header
    unsigned int                    m_StateSaveAreaHeaderSize;  // State save aread header size in bytes
};

/*****************************************************************************\
ENUM: USC_SHADER_COMPILER_CONTROLS

      IL level optimizations.

\*****************************************************************************/
USC_PARAM()
enum USC_SHADER_COMPILER_CONTROLS
{
    IndirectTemporaryRemovalEnable,
    CallCndTranslationEnable,
    LoopUnrollingEnable,
    ILPatternMatchingEnable,
    ConditionalExpressionSimplificationEnable,
    TrivialSwitchRemovalEnable,
    TrivialIfRemovalEnable,
    EarlyEOTAfterDiscardEnable,
    SwitchTranslationEnable,
    EarlyReturnRemovalEnable,
    InlineSubstitutionEnable,
    CallToSubroutineCallEnable,
    ConstantBufferToConstantRegisterEnable,
    ConstantBufferToConstantRegisterLDRAWEnable,
    ParseOpcodesToPrecModifierEnable,
    IndexedTempGRFCachingEnable,
    ILConstantFoldingEnable,
    ILConstantFoldingAggressive,
    PrintfExpansionEnable,
    TranslateVendorExtensionsEnable,
    RemoveDeadOutputEnable,
    PerformImageSerializeEnable,
    PointerLoadToIndexedLoad,

    // PatternMatch controls.
    FMulFAddToFMad,
    MovFCmpBranchToMovFCmp,
    MovCndToMov,
    LDStructuredScalarToVector,
    FLogFExp2ScalarToVector,
    MovFCmpToFMax,
    IfDiscardToDiscardCND,
    IfBranchFlattening,
    ContinueCndRemoval,
    FCmpGtToFMaxOrFCmpLtToFMin,
    PreserveFunctionRetMovs,

    MaxLoopUnrollLength,
    MaxConstantBufferPairs,
    PartialUnrollFactor,
    ProcessDynamicResourceIndexingEnable,
    IfConversionLength,

    USC_PARAM_HIDE()
    NUM_USC_SHADER_COMPILER_CONTROLS
};

/*****************************************************************************\
ENUM: USC_KERNEL_COMPILER_CONTROLS

      LLIR/ISA level optimizations.

\*****************************************************************************/
USC_PARAM()
enum USC_KERNEL_COMPILER_CONTROLS
{
    ValueNumberingEnable,
    BlockLoadDirectConstantsEnable,
    BlockLoadIndirectConstantsEnable,
    BlockLoadScatteredConstantsEnable,
    OptimizeReplicateEnable,
    Optimize64bitReplicateEnable,
    ReorderInstructionsEnable,
    ClusterSamplesEnable,
    DeferredInterpolationEnable,
    AtomicReorderInstructionsEnable,
    CoalescingEnable,
    CoalesceCopiesEnable,
    CoalesceBitcastsEnable,
    CoalesceSplitsEnable,
    CoalesceJoinsEnable,
    CoalesceMultiplePayloadsEnable,
    CoalesceHeadersLastEnable,
    OptimizeResourceLoadsEnable,
    ISASchedulingEnable,
    Reduce64To32ALUBitEnable,
    Reduce32To8ALUBitEnable,
    Reduce64To32ALUTopDownPassBitEnable,
    Reduce64To32ALUBottomUpPassBitEnable,
    Reduce64To32ALUSplitPassBitEnable,
    MergeSplitJoinDpEnable,
    FoldUnpacksEnable,
    ConstantFoldingEnable,
    LoopInvariantCodeMotionEnable,
    InputMarkingEnable,
    DispatchDetectionEnable,
    SimdReductionEnable,
    LocallyScalarSimdReductionEnable,
    CPLoadBufferOptimizationEnable,
    RoundRobinRegisterAllocationEnable,
    PatternMatchReplaceEnable,
    EuBypassEnable,
    GRFBankAlignmentEnable,
    OptimizeValuesNamespaceEnable,
    UrbAtomicsEnable,
    ScalarAtomicEnable,
    ComputeToAccumulatorEnable,
    OptimizeSimd8MovsEnable,
    AlignedPointerDetectionEnable,
    BlockLoadGloballyScalarPointerEnable,
    ChannelPropagationEnable,
    CoalesceLdEnable,
    CoalesceLdThreadEnable,
    CoalesceLdCrossLaneEnable,
    CoalesceStoreEnable,
    CutNonspillableLiveRangesEnable,
    DecreaseGRFPressureIfSpilledEnable,
    CodeSinkingEnable,
    MovPropagationEnable,
    CondModPropagationEnable,
    ImmediatesToConstantBufferEnable,
    ImmediatesToConstantBufferOptimizeALUEnable,
    PointerALUOptimizationEnable,
    KillAfterDiscardEnable,
    RematerializationEnable,
    RegionPreSchedulingEnable,
    PruningEnable,
    DeadBranchRemovalEnable,
    NoSrcDepSetEnable,
    ShaderHWInputPackingEnable,
    ShaderDeclarationPackingEnable,
    TPMPromotionEnable,
    SSAAllocatorEnable,
    SSAAllocator1BBOnly,
    SSAAllocatorSIMD8Only,
    GotoJoinOptEnable,
    GotoAroundGotoMergeEnable,
    StatefulCompilationEnable,
    AtomicDstRemovalEnable,
    MergeSimd8SamplerCBLoadsToSimd16Enable,
    SoftwareFp16PayloadEnable,
    SplitQuadTo32bitForALUEnable,
    SIMD32DivergentLoopHeuristicEnable,
    SIMD32SampleCountHeuristicEnable,
    SIMD32ConcurrentValuesHeuristicEnable,
    SIMD32ExtraHeuristicsEnable,

    // *******  Switches affecting floating point math optimizations ******
    NoSignedZerosEnable,           // allow optimizations to disregard the sign of zero
    FiniteMathOnlyEnable,          // assume floating point arguments and results are never Inf, NaN values
    UnsafeMathOptimizationsEnable, // allow algebraically equivalent transformations which may not hold in IEEE 754 arithmetic, e.g. (x+y)-y -> x

    // Required controls.
    TrackParallelInterferences,
    ForceUnalignedPointerReadDetectionEnable,
    ForceUnalignedPointerWriteDetectionEnable,
    DivergentPointerEnable,
    StructuralAnalysisEnable,

    // Numeric controls.
    IndirectCBOptimizationMode,
    ImmediatesToConstantBufferMinImmediates,
    MaxNumOfMulInstructionsPerPowUnwind,
    MulWeightOfSqrtInstructionInPowUnwind,
    MulWeightOfInvInstructionInPowUnwind,

    // Decomposer controls.
    DecomposeFDivToFRcpFMul,

    // LIR Pattern Match controls - they're all dependent from PatternMatchReplaceEnable.
    // i.e. PatternMatchReplaceEnable set to 0 will disable them all.
    PMRChannelMatchEnable,
    PMRPowerMatchEnable,
    PMREUBypassMatchEnable,
    PMRComparisonMatchEnable,
    PMRFlowControlMatchEnable,
    PMRMultiplyMatchEnable,
    PMRMulMadMatchEnable,
    PMRSqrtMatchEnable,
    PMRFDivMatchEnable,
    PMRSelectMatchEnable,
    PMRMinMaxMatchEnable,
    PMRMulDivMatchEnable,
    PMRFDP3MatchEnable,
    PMRFDP4ToHMatchEnable,
    PMRMov0FDPMatchEnable,
    PMRMadMatchEnable,    // equal to old MadPatternMatchReplaceEnable
    PMRBfeMatchEnable,
    PMRLrpMatchEnable,
    PMRTrivialLrpMatchEnable,
    PMRMovLrpToAddMadMatchEnable,
    PMRBfiMatchEnable,
    PMRShrShlMatchEnable,
    PMRAddShlMatchEnable,
    PMRAddAddMatchEnable,
    PMRAndShiftMatchEnable,
    PMRJOIN_DPMatchEnable,
    PMRConvert64bitTo32bit,
    PMRGetValueFromActiveChannelMatchEnable,
    PMRCselMatchEnable,
    PMRReplicateComponentMatchEnable,
    PMRFDPHMatchEnable,
    PMRPackMatchEnable,
    PMRFFRCMatchEnable,
    PMRConstantPropagationMatchEnable,
    PMRTrivialPOWMatchEnable,
    PMRMulAddToMulMatchEnable,
    PMRVecImmScalarMatchEnable,
    PMRMovTwoLowPrecImmEnable,
    PMRIntConvertToBitcastEnable,
    PMRHoistBitcastsEnable,
    PMRMediaBlockReadPackMatchEnable,
    PMRAverageMatchEnable,
    PMRPropIntConvMatchEnable,
    PMRConvToMovMatchEnable,
    PMRPropagateRedundantPackEnable,
    PMRHoistSaturateEnable,
    PMRMergeWordByteUnpacksEnable,
    PMRMergeWordUnpacksEnable,
    PMRPropLowPrecEnable,

    USC_PARAM_HIDE()
    NUM_USC_KERNEL_COMPILER_CONTROLS
};

} // namespace USC

#endif // __cplusplus
