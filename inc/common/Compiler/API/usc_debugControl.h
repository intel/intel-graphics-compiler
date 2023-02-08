/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "usc_config.h"

#define USC_DBG_OFF                  (0x00000000)
#define USC_DBG_CRITICAL             (0x00000001)
#define USC_DBG_NORMAL               (0x00000002)
#define USC_DBG_VERBOSE              (0x00000004)
#define USC_DBG_VERBOSE_VERBOSITY    (0x00000040)

#define USC_DBG_FUNCTION             (0x80000000)

#define USC_DBG_INTERFACE                ((0x00010000) | USC_DBG_FUNCTION)
#define USC_DBG_HARDWARE                 ((0x00020000) | USC_DBG_VERBOSE)
#define USC_DBG_COMPILER                 ((0x00040000) | USC_DBG_VERBOSE)
#define USC_DBG_COMPILER_LIR_DUMP        ((0x00001000) | USC_DBG_VERBOSE)
#define USC_DBG_COMPILER_IR_TO_LIR_TRACE ((0x00002000))

namespace USC
{

#if defined (_DEBUG) || defined (_INTERNAL)
/*****************************************************************************\
    Debug structure for manual control over debug logging level.
\*****************************************************************************/
struct SDebugLevel
{
    unsigned int   Critical              : 1;
    unsigned int   Normal                : 1;
    unsigned int   Verbose               : 1;
    unsigned int   VerboseVerbosity      : 1;

    unsigned int   : 4;

    unsigned int   : 4;

    unsigned int   CompilerLirDump       : 1;
    unsigned int   CompilerIRToLIRTrace  : 1;
    unsigned int   : 2;

    unsigned int   Interface             : 1;
    unsigned int   Hardware              : 1;
    unsigned int   Compiler              : 1;

    unsigned int   : 5;

    unsigned int   : 4;

    unsigned int   : 3;
    unsigned int   Function              : 1;
};

/*****************************************************************************\
    Debug structure that holds MsgLevel together with break on assert option.
\*****************************************************************************/
struct SDebugControl
{
    bool            breakOnAssert;

    union
    {
        unsigned int       MsgLevelValue;
        SDebugLevel MsgLevel;
    };
};

 // Debug variable that holds MsgLevel together with break on assert option
 // for USC.
USC_API extern SDebugControl g_DebugControl;
#endif //_DEBUG

/*****************************************************************************\
    Debug structure for overriding USC behavior.
\*****************************************************************************/
struct SDebugVariables
{
    ///////////////////////////////////////////////////////////////////////////
    // Hardware Debug Controls
    ///////////////////////////////////////////////////////////////////////////
    unsigned int   PassThroughVertexShaderEnable           : 1;
    unsigned int   PassThroughPixelShaderEnable            : 1;

    unsigned int   MaxSoftwareGRFRegistersOverride;

    unsigned int   MaxVSThreadsOverride;
    unsigned int   MaxGSThreadsOverride;
    unsigned int   MaxGSThreadsOverrideWithSO;
    unsigned int   MaxPSThreadsOverride;
    unsigned int   MaxHSThreadsOverride;
    unsigned int   MaxDSThreadsOverride;
    unsigned int   MaxCSThreadsOverride;

    unsigned int   MaxVSURBEntriesOverride;
    unsigned int   MaxGSURBEntriesOverride;

    unsigned int   MaxShaderScratchSpaceOverride; // Overrides the limit on the maximum scratch space
                                                  // size used by all threads in each stage of 3D pipeline.

    unsigned int   GSExecutionMode;

    unsigned int   ExtendAllSourceLiveRanges;
    ///////////////////////////////////////////////////////////////////////////
    // Software Debug Controls
    ///////////////////////////////////////////////////////////////////////////
    unsigned int   BreakInCreateCompiler                   : 1;
    unsigned int   PrintVertexShaderKeyEnable              : 1;
    unsigned int   PrintGeometryShaderKeyEnable            : 1;
    unsigned int   PrintPixelShaderKeyEnable               : 1;
    unsigned int   ReportAssertEnable                      : 1;
    unsigned int   ImprovedIsaDisassemblyEnable            : 1;
    unsigned int   ShaderDumpEnable                        : 1;
    unsigned int   ShaderDumpToRamdisk                     : 1;
    unsigned int   PrintLirRegSetsEnable                   : 1;
    unsigned int   PrintFunctionBodyInstructionListEnable  : 1;
    unsigned int   ShaderDumpPidDisable                    : 1;
    unsigned int   ShaderDumpDuplicatesEnable              : 1;
    unsigned int   IsaInjectionEnable                      : 1;
    unsigned int   IsaLabelPrintingEnable                  : 1;
    unsigned int   EnableDebugFileDump                     : 1;
    unsigned int   AllDumpsInSingleFile                    : 1;
    unsigned int   OptimizationStatsEnable                 : 1;
    unsigned int   CompilationTimeDumpEnable               : 1;
    unsigned int   QualityMetricsEnable                    : 1;
    unsigned int   CodeStatsEnable                         : 1;
    unsigned int   MemoryStatsEnable                       : 1;
    unsigned int   MemoryStatsDetailedEnable               : 1;
    unsigned int   TimeStatsEnable                         : 1;
    unsigned int   DisablePixelShaderSampleInstructions    : 1;
    unsigned int   DisableVertexShaderSampleInstructions   : 1;
    unsigned int   ForceSampler2x2                         : 1;
    unsigned int   DisableRenderTargetSurfaceState         : 1;
    unsigned int   ForceSampleCToSample                    : 1;
    unsigned int   IllegalInstructionAfterEOTEnable        : 1;
    unsigned int   ResetUninitializedVariablesEnable       : 1;
    unsigned int   CompilerLogsInReleaseBuildEnable        : 1;

    unsigned int   ISASchedulerLogs;

    unsigned int   RetailMessageControl;

    unsigned int   FullLIRDumpHashCode;
    unsigned int   DisableMcsFastClear;
    unsigned int   DisableMcsMsaa;

    unsigned int   ShaderDebugHashCode;
    unsigned int   ShaderDebugExtra;
    unsigned int   ShaderDebugHashCodeInKernel             : 1;
    unsigned int   SystemThreadCacheFlushEnable            : 1;
    unsigned int   SystemThreadCacheFlushCountOverride;

    unsigned int   PrintSimdMaskInLIR                     : 1;

    unsigned int   KillPixelHangWorkaroundOverrideEnable;
    unsigned int   KillPixelHangWorkaroundOverride;     // 0-always enable message header for RT write messages
                                                        // 1-dispatch simd8 only when kill pixel enabled
                                                        // 2-thread count = 84(42 GT1) always
                                                        // 3-thread count = 84(42 GT1) when kill pixel enabled
    unsigned int   SendsSupportEnable                      : 1;

    ///////////////////////////////////////////////////////////////////////////
    // Compiler Debug Controls
    ///////////////////////////////////////////////////////////////////////////
    unsigned int   SystemThreadEnable                      : 1;
    unsigned int   InstructionCompactionEnable             : 1;    // TODO: change to Override
    unsigned int   SIMD32CompileOverride                   : 1;
    unsigned int   SIMD32CompileEnable                     : 1;
    unsigned int   SIMD32CompileForce                      : 1;
    unsigned int   SIMD32AllowSpillFill                    : 1;
    unsigned int   SIMD16CompileOverride                   : 1;
    unsigned int   SIMD16CompileEnable                     : 1;
    unsigned int   SIMD16CompileForce                      : 1;
    unsigned int   SIMD8CompileOverride                    : 1;
    unsigned int   SIMD8CompileEnable                      : 1;
    unsigned int   CSSIMD32CompileForce                    : 1;
    unsigned int   CSSIMD16CompileForce                    : 1;
    unsigned int   CSSIMD8CompileForce                     : 1;
    unsigned int   HeaderBypassDisable                     : 1;
    unsigned int   DisableOptimizeDependencies             : 1;
    unsigned int   DisableMergePatchConstantShaders        : 1;
    unsigned int   DisableInstanceUnroll                   : 1;
    unsigned int   HSIndexedControlPointShaderEnable       : 1;
    unsigned int   HSUseSingleHardwareThreadEnable         : 1;
    unsigned int   ForceVSPullModel                        : 1;
    unsigned int   ForceGSPullModel                        : 1;
    unsigned int   ForceHSPullModel                        : 1;
    unsigned int   ForceDSPullModel                        : 1;
    unsigned int   ForceSampleUVtoZero                     : 1;
    unsigned int   ForceSkipValueNumberingForSample        : 1;
    unsigned int   VSSIMD4x2CompileForce                   : 1;
    unsigned int   HSSIMD4x2CompileForce                   : 1;
    unsigned int   DSSIMD4x2CompileForce                   : 1;
    unsigned int   GSSIMD4x2CompileForce                   : 1;
    unsigned int   VSSIMD8CompileForce                     : 1;
    unsigned int   HSSIMD8CompileForce                     : 1;
    unsigned int   DSSIMD8CompileForce                     : 1; // For Gen9+ this mean SIMD8 single- and dual-patch compilation instead of SIMD4x2.
    unsigned int   DSSIMD8SPCompileForce                   : 1; // For Gen9+ only, this mean only single-patch compilation.
    unsigned int   GSSIMD8CompileForce                     : 1;
    unsigned int   VSSIMD8WithFullEpilogEnable             : 1;
    unsigned int   GSSIMD8WithSpillFillCompileEnable       : 1;
    unsigned int   HSSIMD8DualPatchModeCompileEnable       : 1;
    unsigned int   HSSIMD8EightPatchModeCompileEnable      : 1;
    unsigned int   DSSIMD8SingleOrDualPatchModeCompileEnable : 1;
    unsigned int   DisableGPGPUIndirectPayload             : 1;
    unsigned int   LowPrecisionOverride                    : 1;
    unsigned int   LowPrecisionEnable                      : 1;
    unsigned int   SampleLdWithNativeZeroLodDisable        : 1;
    unsigned int   SIMDExpansionEnable                     : 1;
    unsigned int   OCLSurfaceStateSpillFillEnable          : 1;
    unsigned int   DisableHSBarrierRead                    : 1;
    unsigned int   ClearVHEnable                           : 1;
    unsigned int   ForceSamplerPointer                     : 1;
    unsigned int   ForceDoubleSamplerSlots                 : 1;
    unsigned int   URBPaddingSIMD8Disable                  : 1;
    unsigned int   ForceSendTGSMFence                      : 1;
    unsigned int   DisableRSPushConstants                  : 1;
    unsigned int   DisableRSPushConstantsCompute           : 1;
    unsigned int   SWStencilPushConstantEnable             : 1;
    unsigned int   AppendPaddingToISA                      : 1;
    unsigned int   CallToSubroutineCallForceEnable         : 1;

    unsigned int   ForceCompilerInlineFunctionPolicy;

    unsigned int   MaxHSPatchConstantShaders;
    unsigned int   MaxSIMD16PixelShaderInstructionCount;
    unsigned int   MaxSIMD32PixelShaderInstructionCount;

    unsigned int   SIMD32CpiThreshold;
    unsigned int   OCLSIMD32InstructionThreshold;
    unsigned int   OCLSIMD32LIRThreshold;
    unsigned int   OCLSIMD32TPMThreshold;
    unsigned int   MaxSIMD16ComputeShaderInstructionCount;
    unsigned int   MaxSIMD32ComputeShaderInstructionCount;

    unsigned int   SamplerOffsetWorkaroundMinSampleCount;
    unsigned int   L3WAMaxShortVSLength;
    unsigned int   DoLlvmOptimize;
    unsigned int   ShaderTestFlagsFound;
    unsigned int   MaxInstructionsToFixPrecision;

    unsigned int   ForceShortenURBDataLenght :1;
    unsigned int   DisableILReflectOpts;
    unsigned int   DisableSIMD32DueEarlyEOTAfterDiscard :1;
    unsigned int   EnableResourceIndexAlwaysNonUniform :1;
    // USCTester and USCLauncher use different compiler settings, e.g. USCTester does not support CB2CR on other platforms than HSW
    // This is needed to pass this information from USCTester.
    unsigned int   DisableUSCTesterUnsupportedFeatures  : 1;

    ///////////////////////////////////////////////////////////////////////////
    // USC WA Override Controls
    ///////////////////////////////////////////////////////////////////////////
    unsigned int    WaDisable_WIZUnit_ScratchSpace;
    unsigned int    WaClampSamplerArrayIndex;
    unsigned int    WaForcePushConstantEvenReadLength;
    unsigned int    WaDisableHeaderBypassForPSDepthOutput;
    unsigned int    WaForceHeaderForKillPixInstructions;
    unsigned int    WaDecomposePowToLogMulExp;
    unsigned int    WaDisableMadSrc0Replicate;
    unsigned int    WaSamplerChannelDisables;
    unsigned int    WaUAVTypedWriteOverlapping;
    unsigned int    WaUAVTypedReadOverwritesAllDestChannels;
    unsigned int    WaUseOnlySIMD8UntypedRead;
    unsigned int    WaUVoffsetToZeroForLd2dms;
    unsigned int    WaPerThreadScratchSpaceInGPGPUIncrease;
    unsigned int    WaUseFDP3InsteadOfFDP2;
    unsigned int    WaApplyAbsToFrcInstruction;
    unsigned int    WaForPlusInfRoundingModeFrcInstruction;
    unsigned int    WaEmulateIBFEInstrucion;
    unsigned int    WaShaderCalculateResourceOffsets;
    unsigned int    WaUseThreadPayloadCompression;
    unsigned int    WaUnlitCentroidInterpolation;
    unsigned int    WaAddCallToEOTEnabled;
    unsigned int    WaZeroUnusedSampleDGradientsParameters;
    unsigned int    WaAlphaToOneOGL;
    unsigned int    WaOGLGSVertexReordering;
    unsigned int    WaOGLGSVertexReorderingTriStripAdjOnly;
    unsigned int    WaXScaledFormatConversion;
    unsigned int    WaCMPInstFlagDepClearedEarly;
    unsigned int    WaScalarAtomic;
    unsigned int    WaL3UseSamplerForLoadDualConstant;
    unsigned int    WaL3UseSamplerForVectorLoadScatter;
    unsigned int    Wa1DSurfaceSIMD4x2ArrayIndexInRAddress;
    unsigned int    WaBreakSimd16TernaryInstructionsIntoSimd8;
    unsigned int    WaGSPullModelForPatchlistInputTopology;
    unsigned int    WaGSSingleDispatchModeForTriangleInput;
    unsigned int    WaForceSIMD8ForBFIInstruction;
    unsigned int    WaBreakSimd16InstWhenAccIsUsedIntoSimd8;
    unsigned int    WaEnableDummyMovInGpgpuContextSave;
    unsigned int    WaClearNotificationRegInGpgpuContextSave;
    unsigned int    WaNotifRegSwapInGpGpuContextRestore;
    unsigned int    WaStoreSlmOffsetInSRDuringGpGpuPreemption;
    unsigned int    WaSampleGCToSampleLC;
    unsigned int    WaCMPInstNullDstForcesThreadSwitch;
    unsigned int    WaCMPInstNullDstBreaksR0Scoreboarding;
    unsigned int    WaForceHeaderForDualSourceBlendHi;
    unsigned int    WaForceHSPullModel;
    unsigned int    WaInsertNopToHaltDestination;
    unsigned int    WaForceTypeConvertF32To16ToAlign1;
    unsigned int    WaAdditionalMovWhenSrc1ModOnMulMach;
    unsigned int    WaForceMulSrc1WordToAlign1;
    unsigned int    WaCallForcesThreadSwitch;
    unsigned int    WaThreadSwitchAfterCall;
    unsigned int    WANOPBeetweenIndirectAdressingAndBranch;
    unsigned int    WaA32StatelessMessagesRequireHeader;
    unsigned int    WaNoA32ByteScatteredStatelessMessages;
    unsigned int    WaUrbAtomics;
    unsigned int    WaBasicCompilationForDPInstructions;
    unsigned int    WaHalfFloatSelNotAllowedWithSourceModifiers;
    unsigned int    WaLowPrecWriteRTOnlyFloat;
    unsigned int    WaBreakF32MixedModeIntoSimd8;
    unsigned int    WaDisableDSDualPatchMode;
    unsigned int    WaDisableNoSrcDepSetBeforeEOTSend;
    unsigned int    WaDstSubRegNumNotAllowedWithLowPrecPacked;
    unsigned int    WaHeaderRequiredOnSimd16Sample16bit;
    unsigned int    WaLodRequiredOnTypedMsaaUav;
    unsigned int    AccWrEnNotAllowedToAcc1With16bit;
    unsigned int    WaSendsSrc1Length0NotAllowed;
    unsigned int    WaDisableEuBypassOnSimd16Float16;
    unsigned int    WaDisableEuBypassOnSimd16Float32;
    unsigned int    WaStructuredBufferAsRawBufferOverride;
    unsigned int    WaSrc1ImmHfNotAllowed;
    unsigned int    WaCselUnsupported;
    unsigned int    WaDisableDSCaching;
    unsigned int    WaDisallow64BitImmMov;
    unsigned int    WaDisallowDFImmMovWithSimd8;
    unsigned int    WaEmitVtxWhenOutVtxCntIsZero;
    unsigned int    WaFixCentroidInterpolationRTIR16X;
    unsigned int    WaDisableIndirectDataForGPGPUWalker;
    unsigned int    WaDisableIndirectDataAndFlushGPGPUWalker;
    unsigned int    WaDisablePushConstantHSGS;
    unsigned int    WaEnableAllWriteChannelMask;
    unsigned int    WaGather4WithGreenChannelSelectOnR32G32Float;
    unsigned int    WaPreventHSTessLevelsInterference;
    unsigned int    WaAvoidDomainShaderCacheStall;
    unsigned int    WaClearArfDependenciesBeforeEot;
    unsigned int    WaDoNotPushConstantsForAllPulledGSTopologies;
    unsigned int    WaDisableDbg0Register;
    unsigned int    WaGrfDependecyClearInGpgpuContextRestore;
    unsigned int    WaRestoreFCandMSGRegistersFromUpperOword;
    unsigned int    WaRestoreFC4RegisterDW0fromDW1;
    unsigned int    WaRestoreFc0RegistersWithOffset;
    unsigned int    WaGrfDepClearOnOutstandingSamplerInGpgpuContextSave;
    unsigned int    WaStoreAcc2to9InAlign16InGpgpuContextSave;
    unsigned int    WaGrfScoreboardClearInGpgpuContextSave;
    unsigned int    WaClearFlowControlGpgpuContextSave;
    unsigned int    WaClearCr0SpfInGpgpuContextRestore;
    unsigned int    WaSendsSrc1SizeLimitWhenEOT;
    unsigned int    WaDisableMixedModeLog;
    unsigned int    WaDisableMixedModePow;
    unsigned int    WaDisableMixedModeFdiv;
    unsigned int    WaFloatMixedModeSelNotAllowedWithPackedDestination;
    unsigned int    WaDisableNativeDWxDWMultiplication;
    unsigned int    WaForceMinMaxGSThreadCount;
    unsigned int    WaIntegerDivisionSourceModifierNotSupported;
    unsigned int    WaDisableLowPrecisionWriteRTRepData;
    unsigned int    WAResetN0AfterRenderTargetRead;

    unsigned int    WADisableWriteCommitForPageFault;   // Disables Write commit for page fault
    unsigned int    WaDisableDeepLoopsUnrolling;
    unsigned int    EnableRegSizeConsistencyCheck   :1; // Enables RegSizeConsistencyCheck
    unsigned int    EnableUntypedSrfRead    :1;
};

#if ( defined _DEBUG ) || ( defined _INTERNAL )
//  Debug variable for overriding USC behavior.
USC_API extern SDebugVariables g_DebugVariables;
#endif // defined _DEBUG || defined _INTERNAL

/*****************************************************************************\
MACRO: USC_RV_Declare
PURPOSE: Declares a new entry in the global registry table.
\*****************************************************************************/
#define USC_RV_Declare(type, name, value, subkey, description)                             \
struct SRegistryEntry##name : public SRegistryEntry                                            \
{                                                                                               \
    SRegistryEntry##name()                                                                      \
    : SRegistryEntry(value, false, #name, subkey, #type, description, USC_RV_SetGroup) \
{                                                                                           \
}                                                                                           \
} name

#define DGB_VAR_INITIAL_VALUE   0xCCCCCCCC
/*****************************************************************************\
MACRO: USC_RV_USCWAOverrideControl
PURPOSE: Declares a new control debug variable which can be use to override
workaround related to USC.
\*****************************************************************************/
#define USC_RV_USCWAOverrideControl( name )  \
    USC_RV_Declare( DWORD, name, DGB_VAR_INITIAL_VALUE, "", "" )

/*****************************************************************************\
MACRO: USC_RV_DeclareCompilerControl
PURPOSE: Declares a new compiler control debug variable meta data, one per
shader unit
\*****************************************************************************/
#define USC_RV_DeclareCompilerControl( type, name, value, subkey, description )  \
    USC_RV_Declare( type, VS_##name, value, subkey, description ); \
    USC_RV_Declare( type, GS_##name, value, subkey, description ); \
    USC_RV_Declare( type, PS_##name, value, subkey, description ); \
    USC_RV_Declare( type, HS_##name, value, subkey, description ); \
    USC_RV_Declare( type, DS_##name, value, subkey, description ); \
    USC_RV_Declare( type, CS_##name, value, subkey, description )

/*****************************************************************************\
STRUCT:  SRegistryVariable
PURPOSE: Defines data for holding/defining registry variables
\*****************************************************************************/
struct SRegistryEntry
{
    SRegistryEntry(
        unsigned int in_value,
        bool         in_isSet,
        const char*  in_pName,
        const char*  in_pSubKey,
        const char*  in_pType,
        const char*  in_pDescription,
        const char*  in_pGroup)
        : Value(in_value),
        IsSet(in_isSet),
        pName(in_pName),
        pSubKey(in_pSubKey),
        pType(in_pType),
        pDescription(in_pDescription),
        pGroup(in_pGroup)
    {
    }

    unsigned int Value;
    bool         IsSet;
    const char*  pName;
    const char*  pSubKey;
    const char*  pType;
    const char*  pDescription;
    const char*  pGroup;
};
/*****************************************************************************\
STRUCT: Global definition for the USC registry variables.
\*****************************************************************************/
struct SUSCRegistryVariables
{
    // Only use USC_RV_Declare macros in this structure.  Any other data declaration will break the
    // functionality when parsing this data.

#define USC_RV_SetGroup "DebugVariables"
    USC_RV_Declare( bool, PassThroughVertexShaderEnable, false, "", " Doesn't do anything, just pass through the default values" );
    USC_RV_Declare( bool, PassThroughPixelShaderEnable , false, "", " Doesn't do anything, just pass through the default values" );
    USC_RV_Declare( dword, MaxSoftwareGRFRegistersOverride, 0, "", " Sets maximum number of software registers(max is 128)" );
    USC_RV_Declare( dword, MaxVSThreadsOverride , 0, "", " Max number of Vertex Shader thread" );
    USC_RV_Declare( dword, MaxGSThreadsOverride , 0, "", " Max number of Geometry Shader thread" );
    USC_RV_Declare( dword, MaxGSThreadsOverrideWithSO   , 0, "", " Max number of Geometry Shader threads when stream output is enabled" );
    USC_RV_Declare( dword, MaxPSThreadsOverride , 0, "", " Max number  of Pixel Shader thread" );
    USC_RV_Declare( dword, MaxHSThreadsOverride , 0, "", " Max number  of Hull Shader thread " );
    USC_RV_Declare( dword, MaxDSThreadsOverride , 0, "", " Max number  of Domain Shader thread " );
    USC_RV_Declare( dword, MaxCSThreadsOverride , 0, "", " Max number  of Compute Shader thread " );
    USC_RV_Declare( dword, MaxVSURBEntriesOverride, 0, "", " Max number  of Vertex Shader URB entries" );
    USC_RV_Declare( dword, MaxGSURBEntriesOverride, 0, "", " Max number  of Geometry Shader URB entries" );
    USC_RV_Declare( dword, MaxShaderScratchSpaceOverride, 0, "", " Maximum amount of scratch space to be used by all threads. " );
    USC_RV_Declare( dword, GSExecutionMode, 0xffffffff, "", " Mode of GS compilation. Set 0 for single mode or 1 for dual mode. " );
    USC_RV_Declare( dword, ExtendAllSourceLiveRanges, 0, "", "" );

    USC_RV_Declare( bool, IllegalInstructionAfterEOTEnable, false, "", "Enables debug/release internal driver to insert illegal instruction at the end of each kernel. " );
    USC_RV_Declare( bool, BreakInCreateCompiler, false, "", "Will cause halt on breakpoint situated in CreateCompiler. " );
    USC_RV_Declare( bool, PrintVertexShaderKeyEnable   , false, "", "Enables the printing of different vertex shader keys" );
    USC_RV_Declare( bool, PrintGeometryShaderKeyEnable , false, "", "Enables the printing of different geometry shader keys" );
    USC_RV_Declare( bool, PrintPixelShaderKeyEnable    , false, "", "Enables the printing of different pixel shader keys " );
    USC_RV_Declare( bool, ShaderDumpEnable     , false, "", "Enables debug/release driver to dump IL and ISA shaders to GHAL3D_OUTPUT_DIRECTORY" );
    USC_RV_Declare( bool, ShaderDumpToRamdisk, false, "", "Enables debug/release driver to dump IL and ISA shaders to ramdisk defined in system environmental variable. Works only with ShaderDumpEnable flag, used only for Jenkins automatization purposes. " );
    USC_RV_Declare( bool, PrintLirRegSetsEnable, false, "", "Enables/Disables printing LIR reg sets (e.g. Live-In, Live-Out, etc) in LIR shader dumps" );
    USC_RV_Declare( bool, PrintFunctionBodyInstructionListEnable, false, "", "" );
    USC_RV_Declare( bool, ShaderDumpPidDisable     , false, "", "Enables adding PID to the name of shader dump directory" );
    USC_RV_Declare( bool, ShaderDumpDuplicatesEnable   , false, "", "Enables dumping duplicate shader dumps. Works only with ShaderDumpEnable flag." );
    USC_RV_Declare( bool, IsaInjectionEnable   , false, "", "Enables loading of kernels precompiled with IsaASM." );
    USC_RV_Declare( bool, IsaLabelPrintingEnable   , true, "", "Enables using Labels instead of JIP and UIP numbers." );
    USC_RV_Declare( bool, EnableDebugFileDump  , false, "", "Enables dumping debug output to file. " );
    USC_RV_Declare( bool, AllDumpsInSingleFile, false, "", "All dumps will be dumped into one file. ");
    USC_RV_Declare( bool, OptimizationStatsEnable, false, "", "Enables counting optimizations statistics. " );
    USC_RV_Declare( bool, CompilationTimeDumpEnable    , false, "", "Enables compilation time dump. " );
    USC_RV_Declare( bool, QualityMetricsEnable , false, "", "Enables computing Shader Quality Metrics. " );
    USC_RV_Declare( bool, TimeStatsEnable, false, "", "Enables computing time statisics. " );
    USC_RV_Declare( bool, DisablePixelShaderSampleInstructions, false, "", "Disables sample Pixel shader instructions for debug purposes" );
    USC_RV_Declare( bool, ForceSampler2x2, false, "", "" );
    USC_RV_Declare( bool, DisableRenderTargetSurfaceState, false, "", "" );
    USC_RV_Declare( bool, ForceSampleCToSample, false, "", "" );
    USC_RV_Declare( bool, ResetUninitializedVariablesEnable, false, "", "Enables initialization of all uninitialized variables" );
    USC_RV_Declare( bool, CompilerLogsInReleaseBuildEnable, false, "", "Enables some compiler logs in release-internal builds" );
    USC_RV_Declare( dword, ISASchedulerLogs, 0, "", "Enables ISA scheduler logs: 1 - timings only, 2 - summary file " );
    USC_RV_Declare( dword, RetailMessageControl, 0, "", "" );
    USC_RV_Declare( dword, FullLIRDumpHashCode  , 0, "", "" );
    USC_RV_Declare( bool, DisableMcsFastClear  , false, "", "Runs without color fast clear" );
    USC_RV_Declare( bool, DisableMcsMsaa, false, "", "Runs without MSAA compression" );
    USC_RV_Declare( dword, ShaderDebugHashCode  , 0, "", "The driver will set a breakpoint in the first instruction of the shader which has the provided hash code. It works only when the value is different then 0 and SystemThreadEnable is set to TRUE." );
    USC_RV_Declare( dword, ShaderDebugExtra, 0, "", "Bits 0-5: flags for setting breakpoint in VS,HS,DS,GS,PS,CS. Bits 16-31 contain zero-based number of ISA instruction to set a breakpoint. Remained bits are unused.");
    USC_RV_Declare( bool, ShaderDebugHashCodeInKernel, false, "", "" );
    USC_RV_Declare( bool, SystemThreadCacheFlushDisable, false, "", "" );
    USC_RV_Declare( dword, SystemThreadCacheFlushCountOverride , 0, "", "" );
    USC_RV_Declare( bool, PrintSimdMaskInLIR, false, "", "Prints the destination SIMD mask in the LIR dump" );
    USC_RV_Declare( bool, KillPixelHangWorkaroundOverrideEnable, false, "", "" );
    USC_RV_Declare( dword, KillPixelHangWorkaroundOverride, 0, "", "" );
    USC_RV_Declare( bool, SendsSupportEnable, false, "", "Enables Sends instruction support for Gen9+." );

    USC_RV_Declare( bool, SystemThreadEnable, false, "", "This key forces software to create a system thread. The system thread may still be created by software even if this control is set to false. The system thread is invoked if either the software requires exception handling or if kernel debugging is active and a breakpoint is hit." );
    USC_RV_Declare( bool, InstructionCompactionEnable, true, "", "Enables instruction compaction capability" );
    USC_RV_Declare( bool, SIMD32CompileOverride, false, "", "Overrides current SIMD32 settings with regkey settings for Pixel Shaders." );
    USC_RV_Declare( bool, SIMD32CompileEnable, true, "", "Enables/Disables SIMD32 Compile. If we compile SIMD32 we also need to compile SIMD16 Note: JIT compiler may use additional heuristics which will suppress SIMD32 kernels with this settings (for example SIMD32 kernels will not be generated if there is already SIMD16 kernel which hides IO latencies)." );
    USC_RV_Declare( bool, SIMD32CompileForce, false, "", "Forces SIMD32 compile, all JIT heuristics are bypassed. SIMD32 kernels will not be generated only if SIMD32AllowSpillFill flag is set to FALSE and SIMD32 kernel would require spill-fills." );
    USC_RV_Declare( bool, SIMD32AllowSpillFill, false, "", "Allow / disallow SIMD32 compilation with spill-fills" );
    USC_RV_Declare( bool, SIMD16CompileOverride, false, "", "Overrides current SIMD16 settings with regkey settings" );
    USC_RV_Declare( bool, SIMD16CompileEnable, true, "", "Enables/Disables SIMD16 Compile" );
    USC_RV_Declare( bool, SIMD16CompileForce, false, "", "Forces SIMD16 compile " );
    USC_RV_Declare( bool, SIMD8CompileOverride, false, "", "Overrides current SIMD8 settings with regkey settings" );
    USC_RV_Declare( bool, SIMD8CompileEnable, true, "", "Enables/Disables SIMD8 Compile" );
    USC_RV_Declare( bool, CSSIMD32CompileForce, false, "", "Forces SIMD32 compilation of compute shaders (may result in spill-fills)." );
    USC_RV_Declare( bool, CSSIMD16CompileForce, false, "", "Forces SIMD16 compilation of compute shaders (may result in spill-fills)." );
    USC_RV_Declare( bool, CSSIMD8CompileForce, false, "", "Forces SIMD8 compilation of compute shaders (may result in spill-fills)." );
    USC_RV_Declare( bool, HeaderBypassDisable, false, "", "" );
    USC_RV_Declare( bool, DisableOptimizeDependencies, false, "", "" );
    USC_RV_Declare( bool, DisableMergePatchConstantShaders, false, "", "Set to disable merging Patch Constant Hull Shaders" );
    USC_RV_Declare( bool, DisableInstanceUnroll, false, "", "Set to disable Hull Shader instance unrolling optimization" );
    USC_RV_Declare( bool, HSIndexedControlPointShaderEnable, true, "", "Enable/Disable Hull Shader pass-through Control Point phase to use indexing" );
    USC_RV_Declare( bool, HSUseSingleHardwareThreadEnable , true, "", "Enable/Disable Hull Shader single thread compile heuristic" );
    USC_RV_Declare( bool, ForceVSPullModel  , false, "", "" );
    USC_RV_Declare( bool, ForceGSPullModel  , false, "", "" );
    USC_RV_Declare( bool, ForceHSPullModel  , false, "", "" );
    USC_RV_Declare( bool, ForceDSPullModel  , false, "", "" );
    USC_RV_Declare( bool, ForceSampleUVtoZero, false, "", "" );
    USC_RV_Declare( bool, ForceSkipValueNumberingForSample, false, "", "" );
    USC_RV_Declare( bool, VSSIMD4x2CompileForce, false, "", "" );
    USC_RV_Declare( bool, HSSIMD4x2CompileForce, false, "", "" );
    USC_RV_Declare( bool, DSSIMD4x2CompileForce, false, "", "" );
    USC_RV_Declare( bool, GSSIMD4x2CompileForce, false, "", "" );
    USC_RV_Declare( bool, VSSIMD8CompileForce, false, "", "" );
    USC_RV_Declare( bool, HSSIMD8CompileForce, false, "", "" );
    USC_RV_Declare( bool, DSSIMD8CompileForce, false, "", "" );
    USC_RV_Declare( bool, DSSIMD8SPCompileForce, false, "", "" );
    USC_RV_Declare( bool, GSSIMD8CompileForce, false, "", "" );
    USC_RV_Declare( bool, VSSIMD8WithFullEpilogEnable, false, "", "Enabling this causes sending all parts of Vertex Header and position as paramter for DX9 from VS" );
    USC_RV_Declare( bool, HSSIMD8DualPatchModeCompileEnable, false, "", "" );
    USC_RV_Declare( bool, HSSIMD8EightPatchModeCompileEnable, false, "", "For Gen9+ this mean SIMD8 single- and dual-patch compilation instead of SIMD4x2" );
    USC_RV_Declare( bool, DSSIMD8SingleOrDualPatchModeCompileEnable, false, "", "For Gen9+ only, this mean only single-patch compilation" );
    USC_RV_Declare( dword, MaxHSPatchConstantShaders  , 0, "", "Set the upper limit for the number of Patch Constant Hull Shaders" );
    USC_RV_Declare( dword, MaxSIMD16PixelShaderInstructionCount, 8192, "", "SIMD16 kernels will be disabled if their instruction count exceeds the default number" );
    USC_RV_Declare( dword, MaxSIMD32PixelShaderInstructionCount, 8192, "", "SIMD32 kernels will be disabled if their instruction count exceeds the default number" );
    USC_RV_Declare( dword, SIMD32CpiThreshold, 150, "", "(0 for VLV2) SIMD32 kernels will be disabled if SIMD16 CPI will be lower than number of threads per EU on given platform times this number/100" );
    USC_RV_Declare( dword, OCLSIMD32InstructionThreshold, 2048, "", "Compute Shader Compiler will attempt SIMD32 if the number of IL instructions is below this number.  Currently used by OpenCL only." );
    USC_RV_Declare( dword, OCLSIMD32LIRThreshold, 65, "", "Compute Shader Compiler will attempt SIMD32 with any extra heuristic checks if the number of LIR instructions is below this number.  Currently used by OpenCL only." );
    USC_RV_Declare( dword, OCLSIMD32TPMThreshold, 256, "", "Compute Shader Compiler will attempt SIMD32 if the TPM (thread private memory) used by the shader is less than or equal to this number." );
    USC_RV_Declare( dword, MaxSIMD16ComputeShaderInstructionCount, 8192, "", "" );
    USC_RV_Declare( dword, MaxSIMD32ComputeShaderInstructionCount  , 4096, "", "" );
    USC_RV_Declare( dword, SamplerOffsetWorkaroundMinSampleCount, 4, "", "" );
    USC_RV_Declare( dword, L3WAMaxShortVSLength, 0xffff, "", "" );
    USC_RV_Declare( bool, DoLlvmOptimize, false, "", "" );
    USC_RV_Declare( bool, DisableGPGPUIndirectPayload, false, "", "" );
    USC_RV_Declare( bool, LowPrecisionOverride, false, "", " Overrides support for low precision values" );
    USC_RV_Declare( bool, LowPrecisionEnable, true, "", " Enables support for low precision values" );
    USC_RV_Declare( bool, SampleLdWithNativeZeroLodDisable, false, "", " Disables usage of native HW sample_lz, sample_c_lz_and ld_lz. Instead sample, sample_c and ld with LOD 0 are used." );
    USC_RV_Declare( bool, SIMDExpansionEnable, true, "", "When enabled simd expansion will be performed while compiling shader to higher simd (compile time optimization)" );
    USC_RV_Declare( bool, OCLSurfaceStateSpillFillEnable, false, "", "Set to enable OpenCL spill/fills to go to a surface instead of stateless memory." );
    USC_RV_Declare( dword, MaxInstructionsToFixPrecision, 2, "", " Maximum number of instructions that can have degraded performence due to precision change before compiler decides to insert converting MOV" );
    USC_RV_Declare( bool, ForceShortenURBDataLenght, false, "", "Forces shorten URBData lenght" );
    USC_RV_Declare( bool, DisableILReflectOpts, false, "", "Disable T-Rex Reflection Optimization");
    USC_RV_Declare( bool, DisableSIMD32DueEarlyEOTAfterDiscard, false, "","Disable SIMD32 Due Early EOT After Discard");
    USC_RV_Declare( bool, EnableResourceIndexAlwaysNonUniform, false, "", "D3d12 - Treat all indexing as they are maked non-uniform");
    USC_RV_Declare( bool, EnableRegSizeConsistencyCheck, true, "","Enables RegSizeConsistencyCheck");
    USC_RV_Declare( bool, EnableUntypedSrfRead, false, "", "Common optimization for all shaders. Use untyped surface read message to load scattered constant buffer.");
    USC_RV_Declare( bool, DisableHSBarrierRead, false, "", "Removes pre-barrier URB read intended to force URB sync." );
    USC_RV_Declare( bool, ClearVHEnable, false, "", "Clear VH DW0..DW7 for VS,DS,GS." );
    USC_RV_Declare( bool, ForceSamplerPointer, false, "", "Forces sampler instructions to use sampler table pointer " );
    USC_RV_Declare( bool, ForceDoubleSamplerSlots, false, "", "Forces sampler instructions to use double spaced slots in sampler table access" );
    USC_RV_Declare( bool, URBPaddingSIMD8Disable, false, "", "Disables URB padding for shaders compiled in SIMD8 mode." );
    USC_RV_Declare( bool, ForceSendTGSMFence, false, "", "Send a memory fence for TGSM/SLM even if this it not required for the device." );
    USC_RV_Declare( bool, DisableRSPushConstants, false, "", "Control whether CBs for RC/RD/samplers are used for graphics stages. Must be kept in line with IGFX\\D3D_\\DisableRSPushConstants." );
    USC_RV_Declare( bool, DisableRSPushConstantsCompute, false, "", "Control whether CBs for RC/RD/samplers are used for compute stage. Must be kept in line with (as opposite to) IGFX\\D3D_\\UseRTLPushConstantsCompute." );
    USC_RV_Declare( bool, SWStencilPushConstantEnable, false, "", "Whether SW stencil should use push or pull constants" );
    USC_RV_Declare( bool, CallToSubroutineCallForceEnable, false, "", "Forces replacing simple CALL with SUBROUTINE_CALL" );
    USC_RV_Declare( dword, ForceCompilerInlineFunctionPolicy, 0, "", "Inling function policies: 0 - do not force, 1 - never, 2 - always, 3 - heuristic, 4 - when possible" );
#undef USC_RV_SetGroup

#define USC_RV_SetGroup "USCWAOverrideControl"
    USC_RV_USCWAOverrideControl( WaDisable_WIZUnit_ScratchSpace );
    USC_RV_USCWAOverrideControl( WaClampSamplerArrayIndex );
    USC_RV_USCWAOverrideControl( WaForcePushConstantEvenReadLength );
    USC_RV_USCWAOverrideControl( WaDisableHeaderBypassForPSDepthOutput );
    USC_RV_USCWAOverrideControl( WaForceHeaderForKillPixInstructions );
    USC_RV_USCWAOverrideControl( WaDecomposePowToLogMulExp );
    USC_RV_USCWAOverrideControl( WaDisableMadSrc0Replicate );
    USC_RV_USCWAOverrideControl( WaSamplerChannelDisables );
    USC_RV_USCWAOverrideControl( WaUAVTypedWriteOverlapping );
    USC_RV_USCWAOverrideControl( WaUAVTypedReadOverwritesAllDestChannels );
    USC_RV_USCWAOverrideControl( WaUseOnlySIMD8UntypedRead );
    USC_RV_USCWAOverrideControl( WaUVoffsetToZeroForLd2dms );
    USC_RV_USCWAOverrideControl( WaPerThreadScratchSpaceInGPGPUIncrease );
    USC_RV_USCWAOverrideControl( WaUseFDP3InsteadOfFDP2 );
    USC_RV_USCWAOverrideControl( WaApplyAbsToFrcInstruction );
    USC_RV_USCWAOverrideControl( WaForPlusInfRoundingModeFrcInstruction );
    USC_RV_USCWAOverrideControl( WaEmulateIBFEInstrucion );
    USC_RV_USCWAOverrideControl( WaShaderCalculateResourceOffsets );
    USC_RV_USCWAOverrideControl( WaUseThreadPayloadCompression );
    USC_RV_USCWAOverrideControl( WaUnlitCentroidInterpolation );
    USC_RV_USCWAOverrideControl( WaAddCallToEOTEnabled );
    USC_RV_USCWAOverrideControl( WaZeroUnusedSampleDGradientsParameters );
    USC_RV_USCWAOverrideControl( WaAlphaToOneOGL );
    USC_RV_USCWAOverrideControl( WaOGLGSVertexReordering );
    USC_RV_USCWAOverrideControl( WaOGLGSVertexReorderingTriStripAdjOnly );
    USC_RV_USCWAOverrideControl( WaXScaledFormatConversion );
    USC_RV_USCWAOverrideControl( WaCMPInstFlagDepClearedEarly );
    USC_RV_USCWAOverrideControl( WaScalarAtomic );
    USC_RV_USCWAOverrideControl( WaL3UseSamplerForLoadDualConstant );
    USC_RV_USCWAOverrideControl( WaL3UseSamplerForVectorLoadScatter );
    USC_RV_USCWAOverrideControl( Wa1DSurfaceSIMD4x2ArrayIndexInRAddress );
    USC_RV_USCWAOverrideControl( WaBreakSimd16TernaryInstructionsIntoSimd8 );
    USC_RV_USCWAOverrideControl( WaGSPullModelForPatchlistInputTopology );
    USC_RV_USCWAOverrideControl( WaGSSingleDispatchModeForTriangleInput );
    USC_RV_USCWAOverrideControl( WaForceSIMD8ForBFIInstruction );
    USC_RV_USCWAOverrideControl( WaBreakSimd16InstWhenAccIsUsedIntoSimd8 );
    USC_RV_USCWAOverrideControl( WaEnableDummyMovInGpgpuContextSave );
    USC_RV_USCWAOverrideControl( WaClearNotificationRegInGpgpuContextSave );
    USC_RV_USCWAOverrideControl( WaNotifRegSwapInGpGpuContextRestore );
    USC_RV_USCWAOverrideControl( WaStoreSlmOffsetInSRDuringGpGpuPreemption );
    USC_RV_USCWAOverrideControl( WaSampleGCToSampleLC );
    USC_RV_USCWAOverrideControl( WaCMPInstNullDstForcesThreadSwitch );
    USC_RV_USCWAOverrideControl( WaCMPInstNullDstBreaksR0Scoreboarding );
    USC_RV_USCWAOverrideControl( WaForceHeaderForDualSourceBlendHi );
    USC_RV_USCWAOverrideControl( WaForceHSPullModel );
    USC_RV_USCWAOverrideControl( WaInsertNopToHaltDestination );
    USC_RV_USCWAOverrideControl( WaForceTypeConvertF32To16ToAlign1 );
    USC_RV_USCWAOverrideControl( WaAdditionalMovWhenSrc1ModOnMulMach );
    USC_RV_USCWAOverrideControl( WaForceMulSrc1WordToAlign1 );
    USC_RV_USCWAOverrideControl( WaCallForcesThreadSwitch );
    USC_RV_USCWAOverrideControl( WaThreadSwitchAfterCall );
    USC_RV_USCWAOverrideControl( WANOPBeetweenIndirectAdressingAndBranch );
    USC_RV_USCWAOverrideControl( WaA32StatelessMessagesRequireHeader );
    USC_RV_USCWAOverrideControl( WaNoA32ByteScatteredStatelessMessages );
    USC_RV_USCWAOverrideControl( WaUrbAtomics );
    USC_RV_USCWAOverrideControl( WaBasicCompilationForDPInstructions );
    USC_RV_USCWAOverrideControl( WaHalfFloatSelNotAllowedWithSourceModifiers );
    USC_RV_USCWAOverrideControl( WaLowPrecWriteRTOnlyFloat );
    USC_RV_USCWAOverrideControl( WaBreakF32MixedModeIntoSimd8 );
    USC_RV_USCWAOverrideControl( WaDisableDSDualPatchMode );
    USC_RV_USCWAOverrideControl( WaDisallow64BitImmMov );
    USC_RV_USCWAOverrideControl( WaDisallowDFImmMovWithSimd8 );
    USC_RV_USCWAOverrideControl( WaDisableNoSrcDepSetBeforeEOTSend );
    USC_RV_USCWAOverrideControl( WaDstSubRegNumNotAllowedWithLowPrecPacked );
    USC_RV_USCWAOverrideControl( WaHeaderRequiredOnSimd16Sample16bit );
    USC_RV_USCWAOverrideControl( WaLodRequiredOnTypedMsaaUav );
    USC_RV_USCWAOverrideControl( AccWrEnNotAllowedToAcc1With16bit );
    USC_RV_USCWAOverrideControl( WaSendsSrc1Length0NotAllowed );
    USC_RV_USCWAOverrideControl( WaDisableEuBypassOnSimd16Float16 );
    USC_RV_USCWAOverrideControl( WaDisableEuBypassOnSimd16Float32 );
    USC_RV_USCWAOverrideControl( WaStructuredBufferAsRawBufferOverride );
    USC_RV_USCWAOverrideControl( WaSrc1ImmHfNotAllowed );
    USC_RV_USCWAOverrideControl( WaCselUnsupported );
    USC_RV_USCWAOverrideControl( WaDisableDSCaching );
    USC_RV_USCWAOverrideControl( WaEmitVtxWhenOutVtxCntIsZero );
    USC_RV_USCWAOverrideControl( WaFixCentroidInterpolationRTIR16X );
    USC_RV_USCWAOverrideControl( WaDisableIndirectDataForGPGPUWalker );
    USC_RV_USCWAOverrideControl( WaDisableIndirectDataAndFlushGPGPUWalker );
    USC_RV_USCWAOverrideControl( WaDisablePushConstantHSGS );
    USC_RV_USCWAOverrideControl( WaEnableAllWriteChannelMask );
    USC_RV_USCWAOverrideControl( WaGather4WithGreenChannelSelectOnR32G32Float );
    USC_RV_USCWAOverrideControl( WaPreventHSTessLevelsInterference );
    USC_RV_USCWAOverrideControl( WaAvoidDomainShaderCacheStall );
    USC_RV_USCWAOverrideControl( WaClearArfDependenciesBeforeEot );
    USC_RV_USCWAOverrideControl( WaDoNotPushConstantsForAllPulledGSTopologies );
    USC_RV_USCWAOverrideControl( WaDisableDbg0Register );
    USC_RV_USCWAOverrideControl( WaGrfDependecyClearInGpgpuContextRestore );
    USC_RV_USCWAOverrideControl( WaRestoreFCandMSGRegistersFromUpperOword );
    USC_RV_USCWAOverrideControl( WaRestoreFC4RegisterDW0fromDW1 );
    USC_RV_USCWAOverrideControl( WaRestoreFc0RegistersWithOffset );
    USC_RV_USCWAOverrideControl( WaGrfDepClearOnOutstandingSamplerInGpgpuContextSave );
    USC_RV_USCWAOverrideControl( WaStoreAcc2to9InAlign16InGpgpuContextSave );
    USC_RV_USCWAOverrideControl( WaGrfScoreboardClearInGpgpuContextSave );
    USC_RV_USCWAOverrideControl( WaClearFlowControlGpgpuContextSave );
    USC_RV_USCWAOverrideControl( WaClearCr0SpfInGpgpuContextRestore );
    USC_RV_USCWAOverrideControl( WaSendsSrc1SizeLimitWhenEOT );
    USC_RV_USCWAOverrideControl( WaDisableMixedModeLog );
    USC_RV_USCWAOverrideControl( WaDisableMixedModePow );
    USC_RV_USCWAOverrideControl( WaDisableMixedModeFdiv );
    USC_RV_USCWAOverrideControl( WaFloatMixedModeSelNotAllowedWithPackedDestination );
    USC_RV_USCWAOverrideControl( WaDisableNativeDWxDWMultiplication );
    USC_RV_USCWAOverrideControl( WaForceMinMaxGSThreadCount );
    USC_RV_USCWAOverrideControl( WaIntegerDivisionSourceModifierNotSupported );
    USC_RV_USCWAOverrideControl( WaDisableLowPrecisionWriteRTRepData );
    USC_RV_USCWAOverrideControl( WADisableWriteCommitForPageFault );
    USC_RV_USCWAOverrideControl( WaDisableDeepLoopsUnrolling );
#undef USC_RV_SetGroup

//NOTE: no default values in first step of this implementation.
#define USC_RV_SetGroup "ShaderCompilerControls"
    USC_RV_DeclareCompilerControl( bool,  IndirectTemporaryRemovalEnable,              false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  CallCndTranslationEnable,                    false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  LoopUnrollingEnable,                         false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  ILPatternMatchingEnable,                     false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  ConditionalExpressionSimplificationEnable,   false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  TrivialSwitchRemovalEnable,                  false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  TrivialIfRemovalEnable,                      false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  EarlyEOTAfterDiscardEnable,                  false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  SwitchTranslationEnable,                     false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  EarlyReturnRemovalEnable,                    false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  InlineSubstitutionEnable,                    false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  CallToSubroutineCallEnable,                  false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  ConstantBufferToConstantRegisterEnable,      false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  ConstantBufferToConstantRegisterLDRAWEnable, false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  ParseOpcodesToPrecModifierEnable,            true,  "", "" );
    USC_RV_DeclareCompilerControl( bool,  IndexedTempGRFCachingEnable,                 false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  ILConstantFoldingEnable,                     false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  ILConstantFoldingAggressive,                 false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  PrintfExpansionEnable,                       false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  TranslateVendorExtensionsEnable,             false, "", "" );
    USC_RV_DeclareCompilerControl( bool,  RemoveDeadOutputEnable,                      false, "", "" );
    USC_RV_DeclareCompilerControl( dword, MaxLoopUnrollLength,                         0,      "", "" );
    USC_RV_DeclareCompilerControl( dword, MaxConstantBufferPairs,                      0,      "", "" );
    USC_RV_DeclareCompilerControl( dword, PartialUnrollFactor,                         0,      "", "" );
    USC_RV_DeclareCompilerControl( bool,  ProcessDynamicResourceIndexingEnable,        true,    "", "" );
    USC_RV_DeclareCompilerControl( dword, IfConversionLength,                          0,      "", "" );
#undef USC_RV_SetGroup

#define USC_RV_SetGroup "SShaderPatternMatchControls"
    USC_RV_DeclareCompilerControl( bool,    FMulFAddToFMad,                       false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    MovFCmpBranchToMovFCmp,               false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    MovCndToMov,                          false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    LDStructuredScalarToVector,           false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    FLogFExp2ScalarToVector,              false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    MovFCmpToFMax,                        false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    FCmpGtToFMaxOrFCmpLtToFMin,           false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    PreserveFunctionRetMovs,              false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    IfDiscardToDiscardCND,                false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    IfBranchFlattening,                   false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    ContinueCndRemoval,                   false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    LDRawPtrToIndexedCB,                  false, "", "" );
#undef USC_RV_SetGroup

#define USC_RV_SetGroup "KernelCompilerControls"
    USC_RV_DeclareCompilerControl( bool,    ValueNumberingEnable,                 false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    BlockLoadDirectConstantsEnable,       false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    BlockLoadIndirectConstantsEnable,     false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    BlockLoadScatteredConstantsEnable,    false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    OptimizeReplicateEnable,              false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    Optimize64bitReplicateEnable,         false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    ReorderInstructionsEnable,            false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    ClusterSamplesEnable,                 false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    DeferredInterpolationEnable,          false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    AtomicReorderInstructionsEnable,      false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    CoalescingEnable,                     false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    CoalesceCopiesEnable,                 false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    CoalesceBitcastsEnable,               false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    CoalesceSplitsEnable,                 false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    CoalesceJoinsEnable,                  false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    CoalesceMultiplePayloadsEnable,       false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    CoalesceHeadersLastEnable,            false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    ImmediatesToConstantBufferEnable,     false, "", "" );
    USC_RV_DeclareCompilerControl( dword,   IndirectCBOptimizationMode,           0,     "", "" );
    USC_RV_DeclareCompilerControl( dword,   ImmediatesToConstantBufferMinImmediates, 5,  "", "" );
    USC_RV_DeclareCompilerControl( dword,   MaxNumOfMulInstructionsPerPowUnwind,     3,  "", "" );
    USC_RV_DeclareCompilerControl( dword,   MulWeightOfSqrtInstructionInPowUnwind,   2,  "", "" );
    USC_RV_DeclareCompilerControl( dword,   MulWeightOfInvInstructionInPowUnwind,    2,  "", "" );
    USC_RV_DeclareCompilerControl( bool,    OptimizeResourceLoadsEnable,          false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    ISASchedulingEnable,                  false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    Reduce64To32ALUBitEnable,             false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    Reduce32To8ALUBitEnable,              false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    Reduce64To32ALUTopDownPassBitEnable,  false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    Reduce64To32ALUBottomUpPassBitEnable, false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    Reduce64To32ALUSplitPassBitEnable,    false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    MergeSplitJoinDpEnable,               false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    FoldUnpacksEnable,                    false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    ConstantFoldingEnable,                false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    LoopInvariantCodeMotionEnable,        false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    InputMarkingEnable,                   false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    DispatchDetectionEnable,              false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    SimdReductionEnable,                  false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    LocallyScalarSimdReductionEnable,     false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    CPLoadBufferOptimizationEnable,       false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    RoundRobinRegisterAllocationEnable,   false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    PatternMatchReplaceEnable,            false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    EuBypassEnable,                       false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    GRFBankAlignmentEnable,               false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    OptimizeValuesNamespaceEnable,        false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    UrbAtomicsEnable,                     false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    ScalarAtomicEnable,                   false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    ComputeToAccumulatorEnable,           false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    OptimizeSimd8MovsEnable,              false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    AlignedPointerDetectionEnable,        false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    BlockLoadGloballyScalarPointerEnable, false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    ChannelPropagationEnable,             false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    CoalesceLdThreadEnable,               false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    CoalesceLdCrossLaneEnable,            false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    CoalesceLdEnable,                     false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    CoalesceStoreEnable,                  false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    CutNonspillableLiveRangesEnable,      false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    DecreaseGRFPressureIfSpilledEnable,   false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    CodeSinkingEnable,                    false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    MovPropagationEnable,                 false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    CondModPropagationEnable,             false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    RematerializationEnable,              false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    RegionPreSchedulingEnable,            false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    PruningEnable,                        false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    DeadBranchRemovalEnable,              false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    NoSrcDepSetEnable,                    false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    ShaderHWInputPackingEnable,           false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    ShaderDeclarationPackingEnable,       false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    GotoJoinOptEnable,                    false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    GotoAroundGotoMergeEnable,            false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    StatefulCompilationEnable,            false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    AtomicDstRemovalEnable,               false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    MergeSimd8SamplerCBLoadsToSimd16Enable, false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    SoftwareFp16PayloadEnable,              false, "", "");
    USC_RV_DeclareCompilerControl( bool,    SplitQuadTo32bitForALUEnable,           false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    SIMD32DivergentLoopHeuristicEnable,     false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    SIMD32SampleCountHeuristicEnable,       false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    SIMD32ConcurrentValuesHeuristicEnable,false, "", "" );
    USC_RV_DeclareCompilerControl( bool,    SIMD32ExtraHeuristicsEnable,          false, "", "" );

    USC_RV_DeclareCompilerControl( bool, PMRChannelMatchEnable,                   false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRPowerMatchEnable,                     false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMREUBypassMatchEnable,                  false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRComparisonMatchEnable,                false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRFlowControlMatchEnable,               false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRMultiplyMatchEnable,                  false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRMulMadMatchEnable,                    false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRSqrtMatchEnable,                      false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRFDivMatchEnable,                      false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRSelectMatchEnable,                    false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRMinMaxMatchEnable,                    false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRMulDivMatchEnable,                    false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRFDP3MatchEnable,                      false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRFDP4ToHMatchEnable,                   false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRMov0FDPMatchEnable,                   false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRMadMatchEnable,                       false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRBfeMatchEnable,                       false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRLrpMatchEnable,                       false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRTrivialLrpMatchEnable,                false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRMovLrpToAddMadMatchEnable,            false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRBfiMatchEnable,                       false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRShrShlMatchEnable,                    false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRAddShlMatchEnable,                    false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRAddAddMatchEnable,                    false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRAndShiftMatchEnable,                  false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRJOIN_DPMatchEnable,                   false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRConvert64bitTo32bit,                  false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRGetValueFromActiveChannelMatchEnable, false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRCselMatchEnable,                      false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRReplicateComponentMatchEnable,        false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRFDPHMatchEnable,                      false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRPackMatchEnable,                      false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRFFRCMatchEnable,                      false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRConstantPropagationMatchEnable,       true, "", "Enables Constant Propagation" );
    USC_RV_DeclareCompilerControl( bool, PMRTrivialPOWMatchEnable,                false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRMulAddToMulMatchEnable,               false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRVecImmScalarMatchEnable,              false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRMovTwoLowPrecImmEnable,               false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRIntConvertToBitcastEnable,            false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRHoistBitcastsEnable,                  false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRMediaBlockReadPackMatchEnable,        false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRAverageMatchEnable,                   false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRPropIntConvMatchEnable,               false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRConvToMovMatchEnable,                 false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRPropagateRedundantPackEnable,         false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRHoistSaturateEnable,                  false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRMergeWordByteUnpacksEnable,           false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRMergeWordUnpacksEnable,               false, "", "" );
    USC_RV_DeclareCompilerControl( bool, PMRPropLowPrecEnable,                    false, "", "" );
    USC_RV_DeclareCompilerControl( bool, DecomposeFDivToFRcpFMul,                 false, "", "" );

#undef USC_RV_SetGroup

};
const unsigned int NUM_REGISTRY_VARIABLES = sizeof(SUSCRegistryVariables) / sizeof(SRegistryEntry);

} //namespace USC

/*****************************************************************************\
DEFINE: GHAL3D_OUTPUT_DIRECTORY and USC_ISA_INJECTION_DIRECTORY
\*****************************************************************************/
#if defined(_WIN32)
    #define GHAL3D_OUTPUT_DIRECTORY "\\Intel\\USC\\"
#elif defined(ANDROID)
    #define GHAL3D_OUTPUT_DIRECTORY "/sdcard/IntelUSC/"
    #define USC_ISA_INJECTION_DIRECTORY "/sdcard/IntelUSC/IsaInjection/"
#else
    #define GHAL3D_OUTPUT_DIRECTORY "/tmp/IntelUSC/"
    #define USC_ISA_INJECTION_DIRECTORY "/tmp/IntelUSC/IsaInjection/"
#endif
