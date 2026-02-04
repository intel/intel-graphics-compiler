/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This is an auto-generated file. Please do not edit!
// If changes are needed here please reach out to the codeowners, thanks.

        WA_DECLARE(
        WADisableWriteCommitForPageFault,
        "Workaround",
        WA_BUG_TYPE_FUNCTIONAL,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaClearArfDependenciesBeforeEot,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaClearTDRRegBeforeEOTForNonPS,
        "Workaround",
        WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaConservativeRasterization,
        "Workaround",
        WA_BUG_TYPE_FUNCTIONAL,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN )

        WA_DECLARE(
        WaDisableDSDualPatchMode,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaDisableDSPushConstantsInFusedDownModeWithOnlyTwoSubslices,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaDisableEuBypassOnSimd16Float32,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaDisableIndirectDataForIndirectDispatch,
        "Workaround",
        WA_BUG_TYPE_HANG | WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaDisableMixedModeFdiv,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaDisableMixedModeLog,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaDisableMixedModePow,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaDisableSIMD16On3SrcInstr,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaDisableSendsSrc0DstOverlap,
        "Workaround",
        WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaDisableVSPushConstantsInFusedDownModeWithOnlyTwoSubslices,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaDisallow64BitImmMov,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaDispatchGRFHWIssueInGSAndHSUnit,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaDoNotPushConstantsForAllPulledGSTopologies,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaDstSubRegNumNotAllowedWithLowPrecPacked,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaEnablePooledEuFor2x6,
        "Workaround",
        WA_BUG_TYPE_FUNCTIONAL,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaFloatMixedModeSelNotAllowedWithPackedDestination,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaForceCB0ToBeZeroWhenSendingPC,
        "Workaround",
        WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_OGL)

        WA_DECLARE(
        WaForceMinMaxGSThreadCount,
        "Workaround",
        WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaHeaderRequiredOnSimd16Sample16bit,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaLimitSizeOfSDEPolyFifo,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaMipiDPOUnitClkGateEnable,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaMixModeSelInstDstNotPacked,
        "Workaround",
        WA_BUG_TYPE_FAIL,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaNoA32ByteScatteredStatelessMessages,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaNoSimd16TernarySrc0Imm,
        "Workaround",
        WA_BUG_TYPE_FAIL,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaOCLEnableFMaxFMinPlusZero,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaPruneModeWithIncorrectHsyncOffset,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaResetN0BeforeGatewayMessage,
        "Workaround",
        WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaReturnZeroforRTReadOutsidePrimitive,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaSPTMmioAccessSbi,
        "Workaround",
        WA_BUG_TYPE_FUNCTIONAL,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaSPTMmioReadFailure,
        "Workaround",
        WA_BUG_TYPE_FUNCTIONAL,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaSamplerResponseLengthMustBeGreaterThan1,
        "Workaround",
        WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaSendSEnableIndirectMsgDesc,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaSendsSrc1SizeLimitWhenEOT,
        "Workaround",
        WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaSrc1ImmHfNotAllowed,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        WaThreadSwitchAfterCall,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_13010473643,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14010017096,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_D3D | WA_COMPONENT_OGL)

        WA_DECLARE(
        Wa_14010198302,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14010595310,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14010875903,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14012420496,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14012437816,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14012504847,
        "Workaround",
        WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14012562260,
        "Workaround",
        WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14012688258,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14012688715,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14012760189,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION | WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14013297064,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14013341720,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION | WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14013672992,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14013677893,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14014414195,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_OGL)

        WA_DECLARE(
        Wa_14014595444,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14016243945,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14016880151,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14017131883,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_KMD)

        WA_DECLARE(
        Wa_14017322320,
        "Workaround",
        WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14017715663,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14018117913,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14018126777,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14019028097,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14020375314,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14021891663,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_14025275057,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_1406306137,
        "Workaround",
        WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_1406950495,
        "Workaround",
        WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_1409460247,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_1409909237,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_15010203763,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_1507979211,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_16011698357,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_16011859583,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_16011983264,
        "Workaround",
        WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_16012061344,
        "Workaround",
        WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_16012292205,
        "Workaround",
        WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_16012383669,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_16012725276,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_16013338947,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_1607871015,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_1608127078,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_1609337546,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_1609337769,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_18012201914,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_18012660806,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_D3D | WA_COMPONENT_OGL | WA_COMPONENT_KMD)

        WA_DECLARE(
        Wa_18013852970,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_18015444900,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_18023229625,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_18027439769,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_18035690555,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_18042479026,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_1807084924,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_1808850743,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_22010487853,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_22010493955,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_22010725011,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_22010811838,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_22011157800,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_22011647401,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_22012856258,
        "Workaround",
        WA_BUG_TYPE_PERF,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_22013689345,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION | WA_BUG_TYPE_HANG,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_22013880840,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_22014559856,
        "Workaround",
        WA_BUG_TYPE_CORRUPTION,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_22016140776,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_2201674230,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_22017182272,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_22019804511,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

        WA_DECLARE(
        Wa_220856683,
        "Workaround",
        WA_BUG_TYPE_FUNCTIONAL,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_D3D)

        WA_DECLARE(
        Wa_14026265758,
        "Workaround",
        WA_BUG_TYPE_UNKNOWN,
        WA_BUG_PERF_IMPACT_UNKNOWN, WA_COMPONENT_UNKNOWN)

