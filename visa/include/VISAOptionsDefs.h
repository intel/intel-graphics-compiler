/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef NULLSTR
#define NULLSTR ""
#endif
#ifndef UNUSED
#define UNUSED ""
#endif

// The available types are:
// DEF_VISA_OPTION(ENUM, ET_BOOL,       STRING, COMMENTS,   DEFAULT_VAL)
// Like above, except that for "-foo" it sets the option to true instead of
// flipping default value.
// DEF_VISA_OPTION(ENUM, ET_BOOL_TRUE,  STRING, COMMENTS,   DEFAULT_VAL)
// DEF_VISA_OPTION(ENUM, ET_INT32,      STRING, COMMENTS,   DEFAULT_VAL)
// DEF_VISA_OPTION(ENUM, ET_INT64,      STRING, COMMENTS,   DEFAULT_VAL)
// DEF_VISA_OPTION(ENUM, ET_CSTR,       STRING, COMMENTS,   DEFAULT_VAL)
// Note: ET_2xINT32 is a 64 bit value set using 2 int32 Hi32 Lo32
// DEF_VISA_OPTION(ENUM, ET_2xINT32,    STRING, COMMENTS,   DEFAULT_VAL)

//=== Debugging options ===
DEF_VISA_OPTION(vISA_DumpPasses, ET_BOOL, "-dumpPassesAll", UNUSED, false)
// subsumes the above (should replace; 0 = none, 1 = only when modifications are
// present, 2 = all)
DEF_VISA_OPTION(
    vISA_DumpPassesSubset, ET_INT32, "-dumpPassesSubset",
    "0 means none; 1 means only when modifications happen; 2 means all passes",
    0)
// dump out dot file for debugging
DEF_VISA_OPTION(vISA_DumpDot, ET_BOOL, "-dot", UNUSED, false)
DEF_VISA_OPTION(vISA_DumpDotAll, ET_BOOL, "-dotAll", UNUSED, false)
DEF_VISA_OPTION(vISA_DumpUseInternalName, ET_BOOL, "-dumpUseInternalName",
                "Apply to .g4/.dot dump file names only", false)
DEF_VISA_OPTION(vISA_DumpPerfStats, ET_BOOL, "-dumpVISAJsonStats",
                "dump the core stats to default json file name", false)
DEF_VISA_OPTION(vISA_DumpPerfStatsVerbose, ET_BOOL, "-dumpVISAJsonStatsVerbose",
                "dump the verbose stats to default json file name", false)
DEF_VISA_OPTION(vISA_DumpSendInfoStats, ET_BOOL, "-dumpVISASendInfoStats",
                "dumps the sendinfo stats with the stats.json file", false)
DEF_VISA_OPTION(vISA_ShaderStatsDumpless, ET_BOOL, "-noDumpShaderStats",
        "executes pathway for no dumping and running shader stats", false)
DEF_VISA_OPTION(VISA_FullIRVerify, ET_BOOL, "-fullIRVerify", UNUSED, false)
// dump each option while it is being set by setOption()
DEF_VISA_OPTION(vISA_dumpVISAOptions, ET_BOOL, "-dumpVisaOptions", UNUSED,
                false)
// dump all options after we have finished parsing
DEF_VISA_OPTION(vISA_dumpVISAOptionsAll, ET_BOOL, "-dumpVisaOptionsAll", UNUSED,
                false)
DEF_VISA_OPTION(vISA_Debug, ET_BOOL, "-debug", UNUSED, false)
DEF_VISA_OPTION(vISA_DebugParse, ET_BOOL, "-debugParse", UNUSED, false)
DEF_VISA_OPTION(vISA_DebugConsoleDump, ET_BOOL, "-dumpDebugConsoleOutput",
                UNUSED, false)
DEF_VISA_OPTION(vISA_EmitLocation, ET_BOOL, "-emitLocation", UNUSED, false)
DEF_VISA_OPTION(vISA_dumpRPE, ET_BOOL, "-dumpRPE", UNUSED, false)
DEF_VISA_OPTION(vISA_dumpLiveness, ET_BOOL, "-dumpLiveness", UNUSED, false)
DEF_VISA_OPTION(vISA_DumpUndefUsesFromLiveness, ET_BOOL,
                "-dumpUndefUsesFromLiveness", UNUSED, false)
DEF_VISA_OPTION(vISA_disableInstDebugInfo, ET_BOOL, "-disableInstDebugInfo",
                UNUSED, false)
DEF_VISA_OPTION(vISA_analyzeMove, ET_BOOL, "-analyzeMove", UNUSED, false)
DEF_VISA_OPTION(vISA_maxPTSSOverride, ET_INT32, "-maxPTSSOverride", UNUSED, 0)
DEF_VISA_OPTION(vISA_skipFDE, ET_BOOL, "-skipFDE", UNUSED, false)
DEF_VISA_OPTION(vISA_storeCE, ET_BOOL, "-storeCE", UNUSED, false)
// setting this flag makes VISA emit matching name for variable wrt visaasm file
// but this makes it impossible to emit correct elf, so this is strictly for
// debugging
DEF_VISA_OPTION(vISA_UseFriendlyNameInDbg, ET_BOOL, "-useFriendlyNameInDbg",
                UNUSED, false)
DEF_VISA_OPTION(vISA_EmitSrcFileLineToRPE, ET_BOOL, "-emitsrclinetorpe",
                "makes finalizer emit src line as comment to RPE dump", false)
DEF_VISA_OPTION(vISA_addSWSBInfo, ET_BOOL, "-addSWSBInfo", UNUSED, true)
DEF_VISA_OPTION(vISA_DumpRAIntfGraph, ET_BOOL, "-dumpintf", UNUSED, false)
DEF_VISA_OPTION(vISA_dumpRAMetadata, ET_BOOL_TRUE, "-dumpRAMetadata",
                "USAGE: when TRUE, emits a file containing metadata about "
                "physical assignments that is used for RA validation", false)
DEF_VISA_OPTION(vISA_DumpGenOffset, ET_BOOL, "-dumpgenoffset", UNUSED, false)
DEF_VISA_OPTION(vISA_ForceSpillVariables, ET_CSTR, "-forceSpillVariables",
                "USAGE: -forceSpillVariables dcl_id_1,dcl_id_2", NULL)
DEF_VISA_OPTION(vISA_ForceAssignRhysicalReg, ET_CSTR, "-forceAssignRhysicalReg",
                "USAGE: -forceAssignRhysicalReg [dcl_id]:1,[dcl_name]:2.2,... "
                "name will be used if both name and id of the decl are given",
                NULL)
DEF_VISA_OPTION(
    vISA_StopBeforePass, ET_CSTR, "-stopbefore",
    "For visa LIT test. It dumps g4 to stdout before the given pass"
    "and stops all the remaining passes.", NULL)
DEF_VISA_OPTION(
    vISA_StopAfterPass, ET_CSTR, "-stopafter",
    "For visa LIT test. It dumps g4 to stdout after the given pass"
    "and stops all the remaining passes.", NULL)
DEF_VISA_OPTION(vISA_asmToConsole, ET_BOOL, "-asmToConsole",
                "For visa lit test. It is used to dump .asm to stdout. It"
                "will override -output, if -output is present.", false)
DEF_VISA_OPTION(vISA_DebugOnly, ET_CSTR, "-debug-only", UNUSED, NULL)
DEF_VISA_OPTION(vISA_DisablePrefetchToL1Cache, ET_BOOL, "-disablePrefetchL1",
                "Disables L1 cached for prefetch messages", false)
DEF_VISA_OPTION(vISA_CopyA0ToDBG0, ET_BOOL, "-copyA0ToDBG0",
                "copy value of a0 used for extend msg descriptor of send to the dbg0 register", false)
DEF_VISA_OPTION(
    vISA_CopyMsg0ToDbg0, ET_BOOL, "-copyMsg0ToDbg0",
    "Copy value of msg0.2 used for Multi-Q AppQID to dbg0.0 register", false)

//=== Optimization options ===
DEF_VISA_OPTION(vISA_EnableAlways, ET_BOOL, NULLSTR, UNUSED, true)
DEF_VISA_OPTION(vISA_EnableSendFusion, ET_BOOL, "-enableSendFusion", UNUSED,
                false)
DEF_VISA_OPTION(vISA_EnableWriteFusion, ET_BOOL, "-enableWriteFusion", UNUSED,
                false)
DEF_VISA_OPTION(vISA_EnableAtomicFusion, ET_BOOL, "-enableAtomicFusion", UNUSED,
                false)
DEF_VISA_OPTION(vISA_RemovePartialMovs, ET_BOOL, "-partialMovsProp", UNUSED,
                false)
DEF_VISA_OPTION(vISA_LocalCopyProp, ET_BOOL, "-nocopyprop", UNUSED, true)
DEF_VISA_OPTION(vISA_LocalInstCombine, ET_BOOL, "-noinstcombine", UNUSED, true)
DEF_VISA_OPTION(vISA_LocalFlagOpt, ET_BOOL, "-noflagopt", UNUSED, true)
DEF_VISA_OPTION(vISA_LocalMACopt, ET_BOOL, "-nomacopt", UNUSED, true)
DEF_VISA_OPTION(vISA_LocalCleanMessageHeader, ET_BOOL, "-nomsgheaderopt",
                UNUSED, true)
DEF_VISA_OPTION(vISA_LocalRenameRegister, ET_BOOL, "-noregrenaming", UNUSED,
                true)
DEF_VISA_OPTION(vISA_LocalDefHoist, ET_BOOL, "-nodefhoist", UNUSED, true)
DEF_VISA_OPTION(vISA_FoldAddrImmed, ET_BOOL, "-nofoldaddrimmed", UNUSED, true)
DEF_VISA_OPTION(vISA_enableCSEL, ET_BOOL, "-disablecsel", UNUSED, true)
DEF_VISA_OPTION(vISA_OptReport, ET_BOOL, "-optreport", "DEPRECATED, is a nop",
                false)
DEF_VISA_OPTION(vISA_removeRedundMov, ET_BOOL, "-keepRedundMov", UNUSED, true)
DEF_VISA_OPTION(vISA_MergeScalar, ET_BOOL, "-nomergescalar", UNUSED, true)
DEF_VISA_OPTION(vISA_EnableMACOpt, ET_BOOL, "-nomac", UNUSED, true)
DEF_VISA_OPTION(vISA_EnableDCE, ET_BOOL_TRUE, "-dce", UNUSED, false)
DEF_VISA_OPTION(vISA_DisableleHFOpt, ET_BOOL, "-disableHFOpt", UNUSED, false)
DEF_VISA_OPTION(vISA_enableUnsafeCP_DF, ET_BOOL, "-enableUnsafeCP_DF", UNUSED,
                false)
DEF_VISA_OPTION(vISA_EnableStructurizer, ET_BOOL, "-enableStructurizer", UNUSED,
                false)
DEF_VISA_OPTION(vISA_StructurizerCF, ET_BOOL, "-noSCF",
                "-noSCF: structurizer generates UCF only", true)
DEF_VISA_OPTION(vISA_EnableScalarJmp, ET_BOOL, "-noScalarJmp", UNUSED, true)
DEF_VISA_OPTION(vISA_KeepScalarJmp, ET_BOOL, "-keepScalarJmp",
                "-keepScalarJmp: don't convert scalar jmp to goto", false)
DEF_VISA_OPTION(vISA_enableCleanupBindless, ET_BOOL, "-cleanBindless", UNUSED,
                true)
DEF_VISA_OPTION(vISA_enableCleanupA0Movs, ET_BOOL, "-cleanupA0Movs", UNUSED,
                true)
DEF_VISA_OPTION(vISA_EnableSplitVariables, ET_BOOL, "-noSplitVariables", UNUSED,
                false)
DEF_VISA_OPTION(vISA_ChangeMoveType, ET_BOOL, "-ALTMode", UNUSED, true)
DEF_VISA_OPTION(vISA_accSubstitution, ET_BOOL, "-noAccSub", UNUSED, true)
DEF_VISA_OPTION(vISA_accSubBeforeRA, ET_BOOL, "-noAccSubBRA", UNUSED, true)
DEF_VISA_OPTION(vISA_enableAccSubBeforeRA, ET_BOOL, "-enableAccSubBRA", UNUSED, false)
DEF_VISA_OPTION(vISA_PreSchedForAcc, ET_BOOL, "-preSchedForAcc", UNUSED, false)
DEF_VISA_OPTION(vISA_emitMoreMoviCases, ET_BOOL, "-emitMoreMoviCases", UNUSED, false)
DEF_VISA_OPTION(
    vISA_EnableGatherWithImmPreRA, ET_INT32, "-gatherWithImmPreRA",
    "USAGE: -gatherWithImmPreRA <0|1|2|3> where 0 is disabled, 1 sampler is "
    "enabled, 2 other msg enabled, 3 always use s0.0 for send",
    2)
DEF_VISA_OPTION(vISA_allocateS0RoundRobin, ET_BOOL, "-allocateS0RR", UNUSED, true)
DEF_VISA_OPTION(vISA_s0SubBeforeScheduling, ET_BOOL, "-s0SubBeforeScheduling", UNUSED, true)
DEF_VISA_OPTION(vISA_doAccSubAfterSchedule, ET_BOOL, "-accSubPostSchedule",
                UNUSED, true)
DEF_VISA_OPTION(vISA_localizationForAccSub, ET_BOOL, "-localizeForACC", UNUSED,
                false)
DEF_VISA_OPTION(vISA_disableSrc2AccSub, ET_BOOL, "-disableSrc2AccSub", UNUSED,
                true)
DEF_VISA_OPTION(vISA_hasDoubleAcc, ET_BOOL, "-hasDoubleAcc", UNUSED, false)
DEF_VISA_OPTION(vISA_finiteMathOnly, ET_BOOL, "-finiteMathOnly",
                "If set, float operands do not have NaN/Inf", false)
DEF_VISA_OPTION(vISA_ifCvt, ET_BOOL, "-noifcvt", UNUSED, true)
DEF_VISA_OPTION(vISA_AutoGRFSelection, ET_BOOL_TRUE, "-autoGRFSelection",
                "Enable compiler heuristics for GRF selection", false)
DEF_VISA_OPTION(
    vISA_SpillAllowed, ET_INT32, "-spillAllowed",
    "USAGE: -spillAllowed <spillSize>.\n"
    "Spill size allowed without increasing GRF number in VRT."
    "0 means VRT will always bump up the GRF number to avoid spills",
    256)
DEF_VISA_OPTION(vISA_SpillAllowed256GRF, ET_INT32, "-spillAllowed256GRF",
                "USAGE: -spillAllowed256GRF <spillSize>.\n"
                "Override spill threshold for 256GRF config. If shader has "
                "spills < <spillSize>, #GRF is not bumped up. "
                "0 means no override",
                0)
DEF_VISA_OPTION(vISA_ForceGRFModeUp, ET_INT32, "-forceGRFModeUp",
                "USAGE: -forceGRFModeUp <k>.\n"
                "Set the GRF mode k higher than the one selected by default"
                "heuristics. 0 means no increase in GRF mode.",
                0)
DEF_VISA_OPTION(vISA_ScalarPipe, ET_INT32, "-scalarPipe",
                "USAGE: -scalarPipe <num>\n", 0)
DEF_VISA_OPTION(vISA_LVN, ET_BOOL, "-nolvn", UNUSED, true)
// only affects acc substitution for now
DEF_VISA_OPTION(vISA_numGeneralAcc, ET_INT32, "-numGeneralAcc",
                "USAGE: -numGeneralAcc <accNum>\n", 0)
DEF_VISA_OPTION(vISA_reassociate, ET_BOOL, "-noreassoc", UNUSED, true)
DEF_VISA_OPTION(vISA_split4GRFVar, ET_BOOL, "-no4GRFSplit", UNUSED, true)
DEF_VISA_OPTION(vISA_divergentBB, ET_BOOL, "-divergentBB", UNUSED, true)
DEF_VISA_OPTION(vISA_splitInstructions, ET_BOOL, "-noSplitInstructions", UNUSED,
                true)
DEF_VISA_OPTION(vISA_ignoreBFRounding, ET_BOOL, "-ignoreBFRounding", UNUSED,
                false)
DEF_VISA_OPTION(vISA_scheduleFenceCommit, ET_BOOL, "-fenceCommit", UNUSED, true)
DEF_VISA_OPTION(vISA_SkipRedundantFillInRMW, ET_BOOL, "-normwopt", UNUSED, true)
DEF_VISA_OPTION(vISA_ALTMode, ET_BOOL, "-nonALTMode", UNUSED, false)
DEF_VISA_OPTION(vISA_WideMulMadOpsEnable, ET_BOOL, "-wideMulMadOpsEn", UNUSED,
                false)
DEF_VISA_OPTION(vISA_CoalesceScalarMoves, ET_BOOL, "-enableCoalesceScalarMoves",
                UNUSED, false)
DEF_VISA_OPTION(vISA_OptimizeRedundantS0Movs, ET_BOOL_TRUE, "-optimizeRedundantS0Movs",
                UNUSED, true)
DEF_VISA_OPTION(vISA_enableSamplerLSCCaching, ET_BOOL_TRUE, "-samplerLSCCaching",
                "global flag [0|1] to control LSC caching for sampler", true)
DEF_VISA_OPTION(vISA_samplerLSCCachingThreshold, ET_INT32,
                "-samplerLSCCachingThreshold",
                "spill size threshold to disable LSC caching for sampler", 0)
DEF_VISA_OPTION(vISA_supportLSCImmScale, ET_INT32, "-supportLSCImmScale",
                "USAGE: -supportLSCImmScale <value>. Valid values are:\n"
                "0: Disable for both SLM and UGM-scratch; "
                "1: Enable for SLM; 2: Enable for UGM-scratch; "
                "3: Enable for both SLM and UGM-scratch.\n"
                "Default value is 3.",
                3)
DEF_VISA_OPTION(vISA_SinkBarrierWait, ET_BOOL_TRUE, "-sinkBarrierWait",
                "Barrier signal and wait are usually scheduled back to back. "
                "The option is used to sink barrier wait away from signal as "
                "far as possible.",
                false)
DEF_VISA_OPTION(vISA_dynamicAddrForExDescInLscSend, ET_BOOL_TRUE,
                "-dynamicAddrForExDescInLscSend",
                "Use unfixed address which is assigned by RA instead of a0.2 as "
                "the extend messaged descriptor in LSC send messages",
                true)
DEF_VISA_OPTION(vISA_enableInsertThryld, ET_BOOL_TRUE, "-enableInsertThryld",
                UNUSED,
                false)
DEF_VISA_OPTION(vISA_samplerTholdForThryld, ET_INT32, "-samplerTholdForThryld",
                UNUSED, 4)
DEF_VISA_OPTION(vISA_NonsamplerLoadTholdForThryld, ET_INT32,
                "-nonsamplerLoadTholdForThryld", UNUSED, 4)
DEF_VISA_OPTION(vISA_aluTholdForThryld, ET_INT32, "-aluTholdForThryld", UNUSED,
                12)

//=== code gen options ===
DEF_VISA_OPTION(vISA_noSrc1Byte, ET_BOOL, "-nosrc1byte", UNUSED, false)
DEF_VISA_OPTION(vISA_expandPlane, ET_BOOL, "-expandPlane", UNUSED, false)
DEF_VISA_OPTION(vISA_FImmToHFImm, ET_BOOL, "-fiTohfi", UNUSED, false)
DEF_VISA_OPTION(vISA_cacheSamplerHeader, ET_BOOL, "-noSamplerHeaderCache",
                UNUSED, true)
DEF_VISA_OPTION(vISA_forceSamplerHeader, ET_BOOL, "-forceSamplerHeader", UNUSED,
                false)
DEF_VISA_OPTION(vISA_samplerHeaderWA, ET_BOOL, "-samplerHeaderWA", UNUSED,
                false)
DEF_VISA_OPTION(vISA_markSamplerMoves, ET_BOOL, "-markSamplerMoves", UNUSED,
                false)
DEF_VISA_OPTION(vISA_noncoherentStateless, ET_BOOL, "-ncstateless", UNUSED,
                false)
DEF_VISA_OPTION(vISA_enablePreemption, ET_BOOL, "-enablePreemption", UNUSED,
                false)
DEF_VISA_OPTION(vISA_enablePreemptionR0Only, ET_BOOL, "-enablePreemptionR0Only",
                UNUSED, false)
DEF_VISA_OPTION(VISA_EnableBarrierInstCounterBits, ET_BOOL,
                "-enableBarrierInstCounterBits", UNUSED, false)
DEF_VISA_OPTION(vISA_forceFPMAD, ET_BOOL, "-forcefmad", UNUSED, true)
DEF_VISA_OPTION(vISA_DisableMixMode, ET_BOOL, "-disableMixMode", UNUSED, false)
DEF_VISA_OPTION(vISA_DisableHFMath, ET_BOOL, "-disableHFMath", UNUSED, false)
DEF_VISA_OPTION(vISA_ForceMixMode, ET_BOOL, "-forceMixMode", UNUSED, false)
DEF_VISA_OPTION(vISA_UseSends, ET_BOOL, "-nosends", "DEPRECATED, is a nop", true)
DEF_VISA_OPTION(vISA_doAlign1Ternary, ET_BOOL, "-noalign1ternary", UNUSED, true)
DEF_VISA_OPTION(vISA_loadThreadPayload, ET_BOOL, "-noLoadPayload",
                "Indicates that the vISA finalizer should not generate "
                "kernel argument loading code", true)
DEF_VISA_OPTION(vISA_foldEOTtoPrevSend, ET_BOOL, "-foldEOT", UNUSED, false)
DEF_VISA_OPTION(vISA_hasRNEandDenorm, ET_BOOL, "-hasRNEandDenorm", UNUSED,
                false)
DEF_VISA_OPTION(vISA_forceNoFP64bRegioning, ET_BOOL, "-noFP64bRegion", UNUSED,
                false)
DEF_VISA_OPTION(vISA_noStitchExternFunc, ET_BOOL, "-noStitchExternFunc", UNUSED,
                true)
DEF_VISA_OPTION(vISA_autoLoadLocalID, ET_BOOL, "-autoLocalId", UNUSED, false)
DEF_VISA_OPTION(vISA_loadCrossThreadConstantData, ET_BOOL, "-loadCTCD", UNUSED,
                true)
DEF_VISA_OPTION(vISA_useInlineData, ET_BOOL, "-useInlineData",
                "Indicates that the compute walker command contains inline data "
                "for the first few kernel arguments (.input).  "
                "This affects the kernel payload loading sequence.",
                false)
DEF_VISA_OPTION(vISA_crossThreadDataAlignment, ET_INT32,
                "-crossThreadDataAlignment",
                "If .kernel_attr CrossThreadInputSize .. is absent, "
                "computation of cross kernel arguments aligns up to this value "
                "(so per-thread arguments that follow cross thread are aligned "
                "to this)",
                32)
DEF_VISA_OPTION(vISA_loadThreadPayloadStartReg, ET_INT32, "-setStartReg",
                "The start register at which to begin loading kernel arguments", 1)
DEF_VISA_OPTION(vISA_emitCrossThreadOffR0Reloc, ET_BOOL,
                "-emitCrossThreadOffR0Reloc",
                "Causes kernel argument loading to create a relocatable add "
                "to load from an offset past (e.g. for implicit_args).",
                false)
DEF_VISA_OPTION(vISA_renderTargetWriteSendReloc, ET_BOOL,
        "-renderTargetWriteSendReloc",
        "Enables adding offsets of all Render Target Write send instructions to the relocation table.", false)
DEF_VISA_OPTION(vISA_CodePatch, ET_INT32, "-codePatch", UNUSED, 0)
DEF_VISA_OPTION(vISA_Linker, ET_INT32, "-linker", UNUSED, 0)
DEF_VISA_OPTION(vISA_SSOShifter, ET_INT32, "-paddingSSOShifter", UNUSED, 0)
DEF_VISA_OPTION(vISA_SkipPaddingScratchSpaceSize, ET_INT32, "-skipPaddingScratchSpaceSize", UNUSED, 4096)
DEF_VISA_OPTION(vISA_enableInterleaveMacro, ET_BOOL, "-enableInterleaveMacro",
                UNUSED, false)
// This is a bitset(refer to enum VISALscImmOffOpts);
//  * The low bits [4:1] control LSC AddrType's:
//        (e.g. (1 << VISA_LSC_IMMOFF_ADDR_TYPE_FLAT) == 0x2 enables FLAT, 0x10
//        would be BTI)
//  * The bit [16:16] enables LSC offsets in kernel payload loading.
//  * The bit [17:17] enables LSC offsets in spill/fill.
//    This value also requires the underlying model the load uses also be
//    enabled:
//        (e.g. the bit enabling VISA_LSC_IMMOFF_ADDR_TYPE_BTI == 0x10 will need
//        to be set for BTI in kernel payload loading) (e.g. the bit enabling
//        VISA_LSC_IMMOFF_ADDR_TYPE_SS == 0x8 will need to be set for SS in
//        spill/fill)
//  * 0x3001E enables all address types and also enables use in kernel payload
//  loading and spill/fill.
DEF_VISA_OPTION(vISA_lscEnableImmOffsFor, ET_INT32, "-lscEnableImmOffsFor",
                "Bitset that enables LSC immediate offsets for various cases; "
                "Bits [...:1] control various LSC_ADDR_TYPE's numeric value.  "
                "Bit [16] allows offsets in kernel argument loading.  "
                "Bit [17] enables offsets in spill/fill codegen.  "
                "Confer with the type VISALscImmOffOpts.",
                0x3003E)
DEF_VISA_OPTION(vISA_lscEnableImmOffsetForA32Stateful, ET_BOOL,
                "-lscDisableImmOffsetForA32Stateful",
                "This is to control LSC immediate offset optimization "
                "in InstCombine pass to work around hardware bound checking "
                "problem of signed src0 for LSC SLM and A32 stateful messages.",
                false)
DEF_VISA_OPTION(vISA_PreserveR0InR0, ET_BOOL, "-preserver0", UNUSED, false)
DEF_VISA_OPTION(vISA_StackCallABIVer, ET_INT32, "-abiver", "DEPRECATED, is a nop", 1)
DEF_VISA_OPTION(vISA_LastCallerSavedGRF, ET_INT32, "-lastCallerSavedGRF",
                "***ABI breaking change***"
                "Last caller-save GRF; beyond this is callee saved partition.", 0)
// override spill/fill cache control. 0 is default (no override). Its values are
// enum LSC_L1_L3_CC, defined in igc/common/igc_regkeys_enums_defs.h or
// visa_igc_common_headers.h
DEF_VISA_OPTION(vISA_lscSpillLoadCCOverride, ET_INT32,
                "-lscSpillLoadCCOverride",
                "lsc load cache control option for spill", 0)
DEF_VISA_OPTION(vISA_lscSpillStoreCCOverride, ET_INT32,
                "-lscSpillStoreCCOverride",
                "lsc store cache control option for spill", 0)
DEF_VISA_OPTION(vISA_UseSBIDCntrFeature, ET_BOOL_TRUE, "-useSBIDCntr",
                "Enable SBID counter feature for send instructions in divergent loops", true)

//=== RA options ===
DEF_VISA_OPTION(vISA_RoundRobin, ET_BOOL, "-noroundrobin", UNUSED, true)
DEF_VISA_OPTION(vISA_PrintRegUsage, ET_BOOL, "-printregusage", UNUSED, false)
DEF_VISA_OPTION(vISA_IPA, ET_BOOL, "-noipa", UNUSED, true)
DEF_VISA_OPTION(vISA_LocalRA, ET_BOOL, "-nolocalra", UNUSED, true)
DEF_VISA_OPTION(vISA_LocalRARoundRobin, ET_BOOL, "-nolocalraroundrobin", UNUSED,
                true)
DEF_VISA_OPTION(vISA_ForceSpills, ET_BOOL, "-forcespills", UNUSED, false)
DEF_VISA_OPTION(vISA_NoIndirectForceSpills, ET_BOOL, "-noindirectforcespills",
                UNUSED, false)
DEF_VISA_OPTION(vISA_AbortOnSpill, ET_BOOL, "-abortonspill", UNUSED, false)
DEF_VISA_OPTION(vISA_VerifyRA, ET_BOOL, "-verifyra", UNUSED, false)
DEF_VISA_OPTION(vISA_LocalBankConflictReduction, ET_BOOL, "-nolocalBCR", UNUSED,
                true)
DEF_VISA_OPTION(vISA_FailSafeRA, ET_BOOL, "-nofailsafera", UNUSED, true)
DEF_VISA_OPTION(vISA_NewFailSafeRA, ET_BOOL, "-newfailsafera", UNUSED, false)
DEF_VISA_OPTION(vISA_FlagSpillCodeCleanup, ET_BOOL, "-disableFlagSpillClean",
                UNUSED, true)
DEF_VISA_OPTION(vISA_GRFSpillCodeCleanup, ET_BOOL, "-spillCleanup", UNUSED,
                true)
DEF_VISA_OPTION(vISA_SpillSpaceCompression, ET_BOOL, "-nospillcompression",
                UNUSED, true)
DEF_VISA_OPTION(vISA_ForceSpillSpaceCompression, ET_BOOL, "-forcespillcompression",
                UNUSED, false)
DEF_VISA_OPTION(vISA_SpillSpaceCompressionThreshold, ET_INT32, "-spillcompressionThreshold",
                "USAGE: -spillcompressionThreshold <NUM>\n", 0)
DEF_VISA_OPTION(vISA_ConsiderLoopInfoInRA, ET_BOOL, "-noloopra", UNUSED, true)
DEF_VISA_OPTION(vISA_ReserveR0, ET_BOOL, "-reserveR0", UNUSED, false)
DEF_VISA_OPTION(vISA_SpiltLLR, ET_BOOL, "-nosplitllr", UNUSED, true)
DEF_VISA_OPTION(vISA_EnableGlobalScopeAnalysis, ET_BOOL,
                "-enableGlobalScopeAnalysis", UNUSED, false)
DEF_VISA_OPTION(vISA_LocalDeclareSplitInGlobalRA, ET_BOOL, "-noLocalSplit",
                UNUSED, true)
DEF_VISA_OPTION(vISA_DelayLocalDeclareSplitInGlobalRA, ET_BOOL, "-delayLocalSplit",
                UNUSED, false)
DEF_VISA_OPTION(vISA_DisableSpillCoalescing, ET_BOOL, "-nospillcleanup", UNUSED,
                false)
DEF_VISA_OPTION(vISA_GlobalSendVarSplit, ET_BOOL, "-globalSendVarSplit", UNUSED,
                false)
DEF_VISA_OPTION(vISA_NoRemat, ET_BOOL, "-noremat", UNUSED, false)
DEF_VISA_OPTION(vISA_ForceRemat, ET_BOOL, "-forceremat", UNUSED, false)
DEF_VISA_OPTION(vISA_ForceSplitOnSpill, ET_BOOL, "-forcesplitonspill",
                "Force split on spill heuristic. Only works if there's a"
                " loop in program that has a spilled variable.",
                false)
DEF_VISA_OPTION(vISA_SpillMemOffset, ET_INT32, "-spilloffset",
                "USAGE: -spilloffset <offset>\n", 0)
DEF_VISA_OPTION(vISA_ReservedGRFNum, ET_INT32, "-reservedGRFNum",
                "USAGE: -reservedGRFNum <regNum>\n", 0)
DEF_VISA_OPTION(vISA_ReservedFromFrontGRFNum, ET_INT32,
                "-reservedFromFrontGRFNum",
                "USAGE: -reservedFromFrontGRFNum <regNum>\n", 0)
DEF_VISA_OPTION(vISA_TotalGRFNum, ET_INT32, "-TotalGRFNum",
                "USAGE: -TotalGRFNum <regNum>\n", 0)
DEF_VISA_OPTION(vISA_MaxRAIterations, ET_INT32, "-maxRAIterations",
                "USAGE: -maxRAIterations <iterationNum>\n", 10)
DEF_VISA_OPTION(vISA_MinGRFNum, ET_INT32, "-minGRFNum",
                "Set the lower bound GRF for auto GRF selection."
                "USAGE: -minGRFNum <regNum>. 0 means no minimum GRF number.\n",
                0)
DEF_VISA_OPTION(vISA_MaxGRFNum, ET_INT32, "-maxGRFNum",
                "Set the upper bound GRF for auto GRF selection."
                "USAGE: -maxGRFNum <regNum>. 0 means no maximum GRF number.\n",
                0)
DEF_VISA_OPTION(vISA_RATrace, ET_BOOL, "-ratrace", UNUSED, false)
DEF_VISA_OPTION(vISA_FastSpill, ET_BOOL, "-fasterRA", UNUSED, false)
DEF_VISA_OPTION(vISA_AbortOnSpillThreshold, ET_INT32, "-abortOnSpill", UNUSED,
                0)
DEF_VISA_OPTION(vISA_enableBCR, ET_BOOL, "-enableBCR", UNUSED, false)
DEF_VISA_OPTION(vISA_forceBCR, ET_BOOL, "-forceBCR", UNUSED, false)
DEF_VISA_OPTION(vISA_bumpGRFForForceBCR, ET_BOOL, "-bumpGRFForForceBCR", UNUSED, false)
DEF_VISA_OPTION(vISA_twoSrcBundleBCR, ET_BOOL, "-twoSrcBundleBCR", UNUSED, false)
DEF_VISA_OPTION(vISA_NewAugmentation, ET_BOOL_TRUE, "-newaugmentation",
                "USAGE: -newaugmentation "
                "enable using augmentation with holes",
                true)


// clang-format off
// Enable bundle conflict reduction: put operands of instruction into different GRF bundles.
// Value: 0 disable, 1 dpas instruction, 2 non-dpas instructions, 3 all instructions
// clang-format on
DEF_VISA_OPTION(vISA_enableBundleCR, ET_INT32, "-enableBundleCR",
                "USAGE: -enableBundleCR <0|1|2|3>: 0, disable, 1 dpas "
                "instructions, 2 non-dpas instructions, 3 all",
                1)
DEF_VISA_OPTION(vISA_LraFFWindowSize, ET_INT32, "-lraFFWindowSize", UNUSED, 12)
DEF_VISA_OPTION(vISA_SplitGRFAlignedScalar, ET_BOOL, "-nosplitGRFalignedscalar",
                UNUSED, true)
DEF_VISA_OPTION(vISA_DoSplitOnSpill, ET_BOOL, "-nosplitonspill", UNUSED, true)
DEF_VISA_OPTION(vISA_IncSpillCostAllAddrTaken, ET_BOOL, "-allowaddrtakenspill",
                UNUSED, false)
DEF_VISA_OPTION(vISA_NewSpillCostFunction, ET_BOOL, "-newspillcost", UNUSED,
                false)
DEF_VISA_OPTION(vISA_NewSpillCostFunctionISPC, ET_BOOL, "-newspillcostispc", UNUSED,
                false)

DEF_VISA_OPTION(vISA_VerifyAugmentation, ET_BOOL, "-verifyaugmentation", UNUSED,
                false)
DEF_VISA_OPTION(vISA_DumpProgramWithLexicalId, ET_BOOL_TRUE, "-dumpcode",
                "USAGE: -dumpcode "
                "enable dumping code with lexical id; used for augmentation verification",
                false)
DEF_VISA_OPTION(vISA_VerifyExplicitSplit, ET_BOOL, "-verifysplit", UNUSED,
                false)
DEF_VISA_OPTION(vISA_DumpRegChart, ET_BOOL, "-dumpregchart", UNUSED, false)
DEF_VISA_OPTION(vISA_SpillAnalysis, ET_BOOL, "-spillanalysis", UNUSED, false)
DEF_VISA_OPTION(vISA_DynPerfModel, ET_BOOL, "-perfmodel", UNUSED, false)
DEF_VISA_OPTION(vISA_DumpAllBCInfo, ET_BOOL, "-dumpAllBCInfo", UNUSED, false)
DEF_VISA_OPTION(vISA_FreqBasedSpillCost, ET_INT32, "-freqBasedSpillCost", UNUSED, 0)
DEF_VISA_OPTION(vISA_FreqBasedSpillCostScale, ET_INT32, "-freqBasedSpillCostScale", UNUSED,4)
DEF_VISA_OPTION(vISA_FreqBasedSpillCostFunc, ET_INT32, "-freqBasedSpillCostFunc", UNUSED, 2)
DEF_VISA_OPTION(vISA_DumpFreqBasedSpillCost, ET_INT32, "-dumpFreqBasedSpillCost", UNUSED, 0)
DEF_VISA_OPTION(vISA_LinearScan, ET_BOOL, "-linearScan", UNUSED, false)
DEF_VISA_OPTION(vISA_LSFristFit, ET_BOOL, "-lsRoundRobin", UNUSED, true)
DEF_VISA_OPTION(vISA_verifyLinearScan, ET_BOOL, "-verifyLinearScan", UNUSED,
                false)
DEF_VISA_OPTION(vISA_boundsChecking, ET_BOOL, "-boundsChecking", UNUSED, false)
DEF_VISA_OPTION(vISA_PartitionWithFastHybridRA, ET_BOOL,
                "-partitionWithFastHybridRA", UNUSED, false)
DEF_VISA_OPTION(vISA_DumpLiveRanges, ET_BOOL, "-dumplrs", UNUSED, false)
DEF_VISA_OPTION(vISA_FailSafeRALimit, ET_INT32, "-failSafeRALimit", UNUSED, 3)
DEF_VISA_OPTION(vISA_DenseMatrixLimit, ET_INT32, "-denseMatrixLimit", UNUSED,
                0x800)
DEF_VISA_OPTION(vISA_FillConstOpt, ET_BOOL, "-nofillconstopt", UNUSED, true)
DEF_VISA_OPTION(vISA_GCRRInFF, ET_BOOL, "-GCRRinFF", UNUSED, false)
DEF_VISA_OPTION(vISA_IncrementalRA, ET_INT32, "-incrementalra",
                "USAGE: -incrementalra <0|1|2> where 0 is disabled, 1 is enabled, 2 is enabled with verification", 0)
DEF_VISA_OPTION(vISA_SplitAlignedScalarMinDist, ET_INT32,
                "-splitAlignedScalarMinDist",
                "dist threshold for controlling when to split aligned scalars in RA", 200)
DEF_VISA_OPTION(vISA_SplitAlignedScalarBloatPPT, ET_INT32,
                "-splitAlignedScalarBloatRatio",
                "instuction increase ppt (part per thousand) for controlling when to split aligned scalars in RA", 10)
//=== scheduler options ===
DEF_VISA_OPTION(vISA_LocalScheduling, ET_BOOL, "-noschedule", UNUSED, true)
DEF_VISA_OPTION(vISA_preRA_Schedule, ET_BOOL, "-nopresched", UNUSED, true)
DEF_VISA_OPTION(vISA_preRA_ScheduleForce, ET_BOOL, "-presched", UNUSED, false)
DEF_VISA_OPTION(vISA_preRA_ScheduleCtrl, ET_INT32, "-presched-ctrl",
                "USAGE: -presched-ctrl <ctrl>\n", 4)
DEF_VISA_OPTION(vISA_preRA_ScheduleRPThreshold, ET_INT32, "-presched-rp",
                "USAGE: -presched-rp <threshold>\n", 0)
DEF_VISA_OPTION(vISA_preRA_ScheduleExtraGRF, ET_INT32, "-presched-extra-grf",
                "USAGE: -presched-extra-grf <num>\n", 0)
DEF_VISA_OPTION(vISA_ScheduleStartBBID, ET_INT32, "-sched-start",
                "USAGE: -sched-start <BB ID>\n", 0)
DEF_VISA_OPTION(vISA_ScheduleEndBBID, ET_INT32, "-sched-end",
                "USAGE: -sched-end <BB ID>\n", 0)
DEF_VISA_OPTION(vISA_SpillCleanupStartBBID, ET_INT32, "-spill-cleanup-start",
                "USAGE: -spill-cleanup-start <BB ID>\n", 0)
DEF_VISA_OPTION(vISA_SpillCleanupEndBBID, ET_INT32, "-spill-cleanup-end",
                "USAGE: -spill-cleanup-end <BB ID>\n", 0xffffffff)
DEF_VISA_OPTION(vISA_preRA_MinRegThreshold, ET_INT32, "-minreg-rp",
                "USAGE: -minreg-rp <threshold>\n", 0)
DEF_VISA_OPTION(vISA_DumpSchedule, ET_BOOL, "-dumpSchedule", UNUSED, false)
DEF_VISA_OPTION(vISA_DumpDagDot, ET_BOOL, "-dumpDagDot", UNUSED, false)
DEF_VISA_OPTION(vISA_DumpDagTxt, ET_BOOL, "-dumpDagTxt", UNUSED, false)
DEF_VISA_OPTION(vISA_EnableNoDD, ET_BOOL, "-enable-noDD", UNUSED, false)
DEF_VISA_OPTION(vISA_DebugNoDD, ET_BOOL, "-debug-noDD", UNUSED, false)
DEF_VISA_OPTION(vISA_NoDDLookBack, ET_INT32, "-noDD-lookback",
                "USAGE: -noDD-lookback <NUM>\n", 3)
DEF_VISA_OPTION(vISA_EnableNoSrcDep, ET_BOOL, "-enable-noSrcDep", UNUSED, false)
DEF_VISA_OPTION(vISA_EnableNoSrcDepScen1, ET_BOOL, "-disable-noSrcDep-scen1",
                UNUSED, true)
DEF_VISA_OPTION(vISA_EnableNoSrcDepScen2, ET_BOOL, "-disable-noSrcDep-scen2",
                UNUSED, true)
DEF_VISA_OPTION(vISA_DumpNoSrcDep, ET_BOOL, "-dump-noSrcDep", UNUSED, false)
DEF_VISA_OPTION(vISA_stopNoSrcDepSetAt, ET_INT32, "-stop-noSrcDep-at",
                "Usage: -stop-noSrcDep-at <NUMBER>\n", UINT_MAX)
DEF_VISA_OPTION(vISA_FuseTypedWrites, ET_BOOL, "-nofuse-typedWrites", UNUSED,
                false)
DEF_VISA_OPTION(vISA_ReorderDPSendToDifferentBti, ET_BOOL, "-nodpsendreorder",
                UNUSED, true)
DEF_VISA_OPTION(vISA_WAWSubregHazardAvoidance, ET_BOOL,
                "-noWAWSubregHazardAvoidance", UNUSED, true)
DEF_VISA_OPTION(vISA_useMultiThreadedLatencies, ET_BOOL,
                "-dontUseMultiThreadedLatencies", UNUSED, true)
DEF_VISA_OPTION(vISA_SchedulerWindowSize, ET_INT32, "-schedulerwindow",
                "USAGE: -schedulerwindow <window-size>\n", 4096)
DEF_VISA_OPTION(vISA_HWThreadNumberPerEU, ET_INT32, "-HWThreadNumberPerEU",
                "USAGE: -HWThreadNumberPerEU <num>\n", 0)
DEF_VISA_OPTION(vISA_NoAtomicSend, ET_BOOL, "-noAtomicSend", UNUSED, false)
DEF_VISA_OPTION(vISA_ReadSuppressionDepth, ET_INT32, "-readSuppressionDepth",
                UNUSED, 0)
DEF_VISA_OPTION(vISA_ScheduleForReadSuppression, ET_BOOL,
                "-scheduleForReadSuppression", UNUSED, false)
DEF_VISA_OPTION(vISA_ScheduleFor2xSP, ET_BOOL, "-scheduleFor2xSP", UNUSED,
                false)
DEF_VISA_OPTION(vISA_SWSBBlockFor2xSP, ET_BOOL, "-SWSBBlockFor2xSP", UNUSED,
                false)
DEF_VISA_OPTION(vISA_LocalSchedulingStartBB, ET_INT32, "-scheduleStartBB",
                UNUSED, 0)
DEF_VISA_OPTION(vISA_LocalSchedulingEndBB, ET_INT32, "-scheduleEndBB", UNUSED,
                UINT_MAX)
DEF_VISA_OPTION(vISA_assumeL1Hit, ET_BOOL, "-assumeL1Hit", UNUSED, false)
DEF_VISA_OPTION(vISA_ignoreL1Hit, ET_BOOL, "-ignoreL1Hit",
                "Ignore LSC L1Hit cache option when calculating latency in scheduling", false)
DEF_VISA_OPTION(vISA_writeCombine, ET_BOOL, "-writeCombine", UNUSED, true)
DEF_VISA_OPTION(vISA_Q2FInIntegerPipe, ET_BOOL, "-Q2FInteger", UNUSED, false)
DEF_VISA_OPTION(vISA_LocalScheduleingStartKernel, ET_INT32,
                "-localScheduleStartKernel", UNUSED, 0)
DEF_VISA_OPTION(vISA_LocalScheduleingEndKernel, ET_INT32,
                "-localScheduleEndKernel", UNUSED, UINT_MAX)
DEF_VISA_OPTION(vISA_ScheduleACCDep, ET_BOOL, "-scheduleACCDep",
                "Enable scheduling with accurate ACC dependence, instead of "
                "coarse grained dependence",
                false)
DEF_VISA_OPTION(vISA_schedWithSendSrcReadCycle, ET_BOOL_TRUE,
                "-schedWithSendSrcReadCycle", UNUSED, false)
DEF_VISA_OPTION(vISA_ScheduleFor2xDpas, ET_BOOL_TRUE, "-scheduleFor2xDpas",
                UNUSED, false)
DEF_VISA_OPTION(vISA_DumpSendDepLatency, ET_INT32, "-dumpSendDepLatency",
                "USAGE: -dumpSendDepLatency <0|1|2|3|4...> where 0 is NODEP, 1 "
                "is RAW, 2 is RAW_MEMORY,... as defined in DepType",
                0)

//=== SWSB options ===
DEF_VISA_OPTION(vISA_USEL3HIT, ET_BOOL, "-SBIDL3Hit", UNUSED, false)
DEF_VISA_OPTION(vISA_EnableIGASWSB, ET_BOOL, "-IGASWSB", UNUSED, false)
DEF_VISA_OPTION(vISA_SWSBDepReduction, ET_BOOL, "-SWSBDepReduction", UNUSED,
                false)
DEF_VISA_OPTION(vISA_forceDebugSWSB, ET_BOOL, "-forceDebugSWSB", UNUSED, false)
DEF_VISA_OPTION(vISA_SWSBInstStall, ET_INT32, "-SWSBInstStall", UNUSED, 0)
DEF_VISA_OPTION(vISA_SWSBInstStallEnd, ET_INT32, "-SWSBInstStallEnd", UNUSED, 0)
DEF_VISA_OPTION(vISA_WARSWSBLocalStart, ET_INT32, "-WARSWSBLocalStart", UNUSED, 0)
DEF_VISA_OPTION(vISA_WARSWSBLocalEnd, ET_INT32, "-WARSWSBLocalEnd", UNUSED, 0)
DEF_VISA_OPTION(vISA_IndirectInstStart, ET_INT32, "-indirectInstStart", UNUSED, 0)
DEF_VISA_OPTION(vISA_IndirectInstEnd, ET_INT32, "-indirectInstEnd", UNUSED, 0)
DEF_VISA_OPTION(vISA_enableDpasFwd, ET_INT32, "-enableDpasFwd",
                "Enable Fwd setting for Dpas macros. "
                "USAGE: -enableDpasFwd <0|1>, where "
                "0 is disabling, 1 is enabling. "
                "If not set, the Fwd feature is "
                "enabled/disabled according to platform setting." , -1)
DEF_VISA_OPTION(vISA_SWSBTokenBarrier, ET_INT32, "-SWSBTokenBarrier", UNUSED, 0)
DEF_VISA_OPTION(vISA_EnableSwitch, ET_BOOL, "-enableSwitch", UNUSED, false)
DEF_VISA_OPTION(vISA_EnableISBIDBUNDLE, ET_BOOL, "-SBIDBundle", UNUSED, false)
DEF_VISA_OPTION(vISA_EnableGroupScheduleForBC, ET_BOOL, "-groupScheduleForBC",
                UNUSED, true)
DEF_VISA_OPTION(vISA_SWSBTokenNum, ET_INT32, "-SWSBTokenNum",
                "USAGE: -SWSBTokenNum <tokenNum>\n", 0)
DEF_VISA_OPTION(vISA_EnableSendTokenReduction, ET_BOOL, "-SendTokenReduction",
                UNUSED, false)
DEF_VISA_OPTION(vISA_GlobalTokenAllocation, ET_BOOL, "-globalTokenAllocation",
                UNUSED, false)
DEF_VISA_OPTION(vISA_QuickTokenAllocation, ET_BOOL, "-quickTokenAllocation",
                UNUSED, false)
DEF_VISA_OPTION(vISA_DistPropTokenAllocation, ET_BOOL,
                "-distPropTokenAllocation", UNUSED, false)
DEF_VISA_OPTION(vISA_byteGranulairySendDep, ET_BOOL, "-byteGranulairySendDep", UNUSED, false)
DEF_VISA_OPTION(vISA_SWSBStitch, ET_BOOL, "-SWSBStitch", UNUSED, false)
DEF_VISA_OPTION(vISA_SBIDDepLoc, ET_BOOL, "-SBIDDepLoc", UNUSED, false)
DEF_VISA_OPTION(vISA_DumpSBID, ET_BOOL, "-dumpSBID", UNUSED, false)
DEF_VISA_OPTION(vISA_AssignTokenUsingStdSort, ET_BOOL,
                "-assignSWSBTokUsingStdSort", UNUSED, false)
DEF_VISA_OPTION(vISA_IgnoreCFInstInSIMDCF, ET_BOOL, "-ignoreCFInstInSIMDCF",
                UNUSED, false)

DEF_VISA_OPTION(vISA_EnableALUThreePipes, ET_BOOL, "-threeALUPipes", UNUSED,
                true)
DEF_VISA_OPTION(vISA_EnableDPASTokenReduction, ET_BOOL, "-DPASTokenReduction",
                UNUSED, false)
DEF_VISA_OPTION(vISA_EnableDPASBundleConflictReduction, ET_BOOL,
                "-DPASBundleReduction", UNUSED, true)
DEF_VISA_OPTION(vISA_NoDPASMacro, ET_BOOL, "-noDPASMacro", UNUSED, false)
DEF_VISA_OPTION(vISA_forceDPASMacro, ET_BOOL, "-forceDPASMacro", "DEPRECATED, is a nop", false)
DEF_VISA_OPTION(vISA_KeepDPASMacroInSchedule, ET_BOOL, "-keepDPASMacroInSchedule", UNUSED, false)
DEF_VISA_OPTION(vISA_scheduleforDPASMacro, ET_BOOL, "-scheduleforDPASMacro",
                UNUSED, false)
DEF_VISA_OPTION(vISA_TrueDepOnly, ET_BOOL, "-trueDepOnly", UNUSED, false)
DEF_VISA_OPTION(vISA_SplitMov64, ET_INT32, "-SplitMov64",
                "USAGE: -SplitMov64 (0|1|2)\n", 0)
DEF_VISA_OPTION(vISA_has4DeepSystolic, ET_BOOL, "-has4DeepSystolic", UNUSED,
                false)

DEF_VISA_OPTION(vISA_PVCSendWARWA, ET_BOOL_TRUE, "-PVCSendWARWA",
                "enable Send WAR walkaround for PVC", false)
DEF_VISA_OPTION(vISA_SWSBMakeLocalWAR, ET_BOOL_TRUE, "-SWSBMakeLocalWAR",
                "Enable WAR Sync at the end of BB", false)
DEF_VISA_OPTION(vISA_SWSBReplaceARWithAW, ET_BOOL_TRUE, "-SWSBReplaceARWithAW",
                "replace .src with .dst", false)
DEF_VISA_OPTION(vISA_PredicatedFdivSqrt, ET_INT32, "-predicatedfdivsqrt",
                "USAGE: -predicatedfdivsqrt 0(if)|1(predicated)|2(auto)", 2)
DEF_VISA_OPTION(vISA_FastCompileRA, ET_BOOL, "-fastCompileRA", UNUSED, false)
DEF_VISA_OPTION(vISA_HybridRAWithSpill, ET_BOOL, "-hybridRAWithSpill", UNUSED,
                false)
DEF_VISA_OPTION(vISA_SelectiveFastRA, ET_BOOL, "-selectiveFastRA", UNUSED,
                false)
DEF_VISA_OPTION(vISA_SelectiveRAInstThreshold, ET_INT32, "-selectiveRAInstThreshold",
                UNUSED, 131072) // 128*1024
DEF_VISA_OPTION(vISA_SelectiveRAGlobaVarRatioThreshold, ET_CSTR, "-selectiveRAGVRatioThreshold",
                UNUSED, "0.16")
DEF_VISA_OPTION(vISA_EnableSwapAccSub, ET_BOOL, "-swapAccSub", UNUSED, true)
DEF_VISA_OPTION(vISA_EnableRRAccSub, ET_BOOL, "-roundRobinAccSub", UNUSED,
                false)
DEF_VISA_OPTION(vISA_EURegionRemoval, ET_BOOL_TRUE, "-fixNonMovRegion", UNUSED,
                false)
DEF_VISA_OPTION(vISA_AllowSrcCRegion, ET_BOOL_TRUE, "-allowSrcCRegion", UNUSED,
                false)
DEF_VISA_OPTION(vISA_RelaxQWRegion, ET_BOOL_TRUE, "-relaxQWRegion", UNUSED,
                false)
DEF_VISA_OPTION(vISA_BalanceIntMov, ET_INT32, "-balanceIntMov",
                "USAGE: -balanceIntMov <ratio>\n", -1)
DEF_VISA_OPTION(vISA_GAReArchBugFix, ET_BOOL_TRUE, "-GAReArchBugFix", UNUSED,
                false)
DEF_VISA_OPTION(vISA_EnableInt32DstMulMad, ET_BOOL_TRUE,
                "-enableInt32DstMulMad", UNUSED, false)
DEF_VISA_OPTION(vISA_EnableInt32MULLH, ET_BOOL_TRUE, "-enableInt32Mullh",
                UNUSED, false)
// Apply the new ACC usage restructions and add the ACC usage in new
// instructions and data types
DEF_VISA_OPTION(vISA_GAReArchACC, ET_BOOL_TRUE, "-GAReArchACC",
                "To enable new ACC usage rules and restrictions", true)
//=== binary emission options ===
DEF_VISA_OPTION(vISA_Compaction, ET_BOOL, "-nocompaction", UNUSED, true)
DEF_VISA_OPTION(vISA_BXMLEncoder, ET_BOOL, "-nobxmlencoder", UNUSED, true)
DEF_VISA_OPTION(vISA_IGAEncoder, ET_BOOL, "-IGAEncoder",
                "forces use of IGA encoder (default on some platforms)",
                false)
//=== asm/isaasm/isa emission options ===
DEF_VISA_OPTION(vISA_outputToFile, ET_BOOL, "-output", UNUSED, false)
DEF_VISA_OPTION(vISA_SymbolReg, ET_BOOL, "-symbolreg", "DEPRECATED, is a nop", false)
DEF_VISA_OPTION(vISA_PrintASMCount, ET_BOOL, "-printasmcount", UNUSED, false)
DEF_VISA_OPTION(vISA_GenerateBinary, ET_BOOL, "-binary", UNUSED, false)
DEF_VISA_OPTION(vISA_GenerateISAASM, ET_BOOL, "-dumpcommonisa", UNUSED, false)
DEF_VISA_OPTION(vISA_GenerateCombinedISAASM, ET_BOOL, "-dumpcombinedcisa",
                "Emit isaasm of all kernels and functions into a combined file "
                "in a compilation", false)
DEF_VISA_OPTION(vISA_ISAASMToConsole, ET_BOOL, "-isaasmToConsole",
                "The option can be used with -dumpcommonisa to make finalizer"
                "emit isaasm to stdout instead of file and do early exit", false)
DEF_VISA_OPTION(vISA_AddISAASMDeclarationsToEnd, ET_BOOL, "-isaasmAddDeclarationsAtEnd",
                "Add a comment with .decl section to the end of isaasm console dump. Used in tests.", false)
DEF_VISA_OPTION(vISA_DumpIsaVarNames, ET_BOOL, "-dumpisavarnames", UNUSED, true)
DEF_VISA_OPTION(vISA_UniqueLabels, ET_BOOL, "-uniqueLabel", UNUSED, false)
DEF_VISA_OPTION(vISA_ShaderDumpRegexFilter, ET_CSTR, "-shaderDumpRegexFilter",
                "USAGE: -shaderDumpRegexFilter <regex>\n", NULL)
DEF_VISA_OPTION(vISA_DumpvISA, ET_BOOL, "-dumpvisa",
                "-dumpvisa is deprecated and will be removed.", false)
DEF_VISA_OPTION(vISA_StripComments, ET_BOOL, "-stripcomments", UNUSED, false)
DEF_VISA_OPTION(vISA_dumpNewSyntax, ET_BOOL, "-disableIGASyntax", UNUSED, true)
DEF_VISA_OPTION(vISA_NumGenBinariesWillBePatched, ET_INT32,
                "-numGenBinariesWillBePatched",
                "USAGE: missing number of gen binaries that will be patched.\n",
                0)
DEF_VISA_OPTION(vISA_noLdStAsmSyntax, ET_BOOL, "-noLdStAsmSyntax",
                "Disables IGA's load/store syntax in shader dumps "
                "(generates raw send instructions).",
                false)
DEF_VISA_OPTION(vISA_ExtraIntfFile, ET_CSTR, "-extraIntfFile",
                "USAGE: File Name with extra interference info.\n", NULL)
DEF_VISA_OPTION(vISA_AddExtraIntfInfo, ET_BOOL, NULLSTR, UNUSED, false)
DEF_VISA_OPTION(vISA_OutputIsaasmName, ET_CSTR, "-outputIsaasmName",
                "USAGE: specify the name for the combined .isaasm file", NULL)
DEF_VISA_OPTION(vISA_LabelStr, ET_CSTR, "-uniqueLabels",
                "Label String is not provided for the -uniqueLabels option.",
                NULL)
DEF_VISA_OPTION(VISA_AsmFileName, ET_CSTR, "-asmOutput",
                "USAGE: -asmOutput <FILE>\n", NULL)
DEF_VISA_OPTION(vISA_DecodeDbg, ET_CSTR, "-decodedbg",
                "USAGE: -decodedbg <dbg filename>\n", NULL)
DEF_VISA_OPTION(vISA_DecodeRAMetadata, ET_BOOL_TRUE, "-decodeRAMetadata",
                "USAGE: decodes a file containing RA metadata and "
                "outputs it to console in human-readable format", false)
DEF_VISA_OPTION(vISA_encoderFile, ET_CSTR, "-encoderStatisticsFile",
                "USAGE: -encoderStatisticsFile <reloc file>\n",
                "encoderStatistics.csv")
DEF_VISA_OPTION(vISA_DumpRegInfo, ET_BOOL, "-dumpRegInfo", UNUSED, false)
DEF_VISA_OPTION(vISA_PrintHexFloatInAsm, ET_BOOL, "-printHexFloatInAsm",
                "Makes device assembly report immediate float operands in "
                "hex instead of float.",
                false)
DEF_VISA_OPTION(vISA_PrintInstOffsetInAsm, ET_BOOL_TRUE, "-printInstOffsetInAsm",
                "Enables printing instruction offsets as comments in shader dump.",
                false)
DEF_VISA_OPTION(
    vISA_dumpIgaJson, ET_INT32, "-dumpIgaJson",
    "Emits a .json file (peer to .asm) with final IGA output in JSON format; "
    "1 enables basic output, 2 enables with def/use dataflow information "
    "(may increase compile time significantly for large shaders)",
    0)
DEF_VISA_OPTION(vISA_ParseBuildOptions, ET_BOOL, "-parseBuildOptions", UNUSED,
                false)

//=== misc options ===
DEF_VISA_OPTION(vISA_PlatformSet, ET_INT32, NULLSTR, UNUSED, -1 /*GENX_NONE*/)
DEF_VISA_OPTION(vISA_NoVerifyvISA, ET_BOOL, "-noverifyCISA", UNUSED, false)
DEF_VISA_OPTION(vISA_InitPayload, ET_BOOL, "-initializePayload", UNUSED, false)
DEF_VISA_OPTION(vISA_AvoidUsingR0R1, ET_BOOL, "-avoidR0R1", UNUSED, false)
DEF_VISA_OPTION(vISA_isParseMode, ET_BOOL, NULLSTR, UNUSED, false)
DEF_VISA_OPTION(vISA_ReRAPostSchedule, ET_BOOL, "-rerapostschedule",
                "DEPRECATED, is a nop", false)
DEF_VISA_OPTION(vISA_GTPinReRA, ET_BOOL, "-GTPinReRA", "DEPRECATED, is a nop", false)
DEF_VISA_OPTION(vISA_GetFreeGRFInfo, ET_BOOL, "-getfreegrfinfo", UNUSED, false)
DEF_VISA_OPTION(vISA_GTPinScratchAreaSize, ET_INT32, "-GTPinScratchAreaSize",
                UNUSED, 0)
DEF_VISA_OPTION(vISA_GTPinGetIndirRef, ET_BOOL, "-GTPinIndirRef", UNUSED, false)
DEF_VISA_OPTION(vISA_LSCBackupMode, ET_BOOL, "-LSCBackupMode", UNUSED, false)
DEF_VISA_OPTION(vISA_InjectEntryFences, ET_BOOL, "-InjectEntryFences", UNUSED,
                false)
DEF_VISA_OPTION(vISA_LSCEnableHalfSIMD, ET_BOOL, "-enableHalfLSC",
                "Indicates that the platform has half-size non-transpose "
                "LSC messages.  E.g. A SIMD4 non-transpose only rounds up to "
                "SIMD16 payloads (on platforms that support to SIMD32).",
                false)
DEF_VISA_OPTION(vISA_lscNonStackSpill, ET_BOOL, "-lscNonStackSpill", UNUSED,
                false)
DEF_VISA_OPTION(vISA_scatterSpill, ET_BOOL, "-scatterSpill",
                "Use LSC scatter store for spills to avoid RMW", false)
// native int64 adder was removed and then added back (adder lacks saturation)
// the int64 shifter was never removed
DEF_VISA_OPTION(vISA_HasNoInt64Add, ET_BOOL, "-hasNoInt64Add", UNUSED, false)
// To enable special kernel cost model as part of performance stats
DEF_VISA_OPTION(vISA_KernelCostInfo, ET_BOOL, "-kernelCostInfo",
                "Collect perf stats based on special kernel Cost Model.", false)
DEF_VISA_OPTION(vISA_dumpKCI, ET_BOOL, "-dumpKCI",
                "Dump info collected by kernelCostInfo.", false)
DEF_VISA_OPTION(vISA_dumpDetailKCI, ET_BOOL, "-dumpDetailKCI",
                "Dump detailed info collected by kernelCostInfo.", false)
DEF_VISA_OPTION(vISA_dumpKCIForLit, ET_BOOL, "-dumpKCIForLit",
                "Dump kernel cost info for lit testing only", false)
// Corresponds to something slightly different in IGC than vISA_HasInt64Add
// (C.f. Platform.hpp:hasPartialInt64Support)
DEF_VISA_OPTION(vISA_HasPartialInt64, ET_BOOL, "-partialInt64", UNUSED, false)

DEF_VISA_OPTION(vISA_EnableDPASBFHFH, ET_BOOL, "-enableDPASBFHF", UNUSED, false)
DEF_VISA_OPTION(vISA_EnableMathDPASWA, ET_BOOL, "-enableMathDPASWA", UNUSED,
                false)
DEF_VISA_OPTION(vISA_skipFenceCommit, ET_BOOL, "-skipFenceCommit", UNUSED,
                false)
DEF_VISA_OPTION(vISA_removeFence, ET_BOOL, "-removeFence",
                "Remove fence if no write in a kernel", false)
DEF_VISA_OPTION(vISA_skipGitHash, ET_BOOL, "-noGitHash",
                "Do not emit git hash in .asm", false)
DEF_VISA_OPTION(vISA_SendAWProfiling, ET_BOOL, "-sendAWProfiling",
                "Emit after write profiling data in .asm", false)

DEF_VISA_OPTION(vISA_EnableKernelArgument, ET_BOOL, "-enableKernelArguments",
                "Enables XE3 Kernel Arguments", false)

//=== HW Workarounds ===
DEF_VISA_OPTION(vISA_clearScratchWritesBeforeEOT, ET_BOOL,
                "-waClearScratchWrite", UNUSED, false)
DEF_VISA_OPTION(vISA_clearHDCWritesBeforeEOT, ET_BOOL, "-waClearHDCWrite",
                UNUSED, false)
DEF_VISA_OPTION(vISA_clearLSCUGMWritesBeforeEOT, ET_BOOL, "-waLscUgmFence",
                UNUSED, false)
DEF_VISA_OPTION(vISA_setA0toTdrForSendc, ET_BOOL, "-setA0toTdrForSendc", UNUSED,
                false)
DEF_VISA_OPTION(vISA_addFFIDProlog, ET_BOOL, "-noFFIDProlog", UNUSED, true)
DEF_VISA_OPTION(vISA_setFFID, ET_INT32, "-setFFID", "USAGE: -setFFID <ffid>\n",
                FFID_INVALID)
DEF_VISA_OPTION(vISA_replaceIndirectCallWithJmpi, ET_BOOL,
                "-replaceIndirectCallWithJmpi", UNUSED, false)
DEF_VISA_OPTION(vISA_noMaskWA, ET_BOOL, "-noMaskWA", UNUSED, false)
DEF_VISA_OPTION(vISA_forceNoMaskWA, ET_BOOL, "-forceNoMaskWA", UNUSED, false)
DEF_VISA_OPTION(vISA_DPASFuseRSWA, ET_BOOL, "-DPASFuseRSWA", UNUSED, false)
DEF_VISA_OPTION(vISA_gatherRSFusionSyncWA, ET_BOOL,
                "-disableGatherRSFusionSyncWA",
                "Disable the WA for the out of sync issue for gather instruction in "
                "EU fusion, when read suppression is enabled",
                true)
DEF_VISA_OPTION(vISA_noMaskWAOnFuncEntry, ET_BOOL, "-noMaskWAOnFuncEntry",
                UNUSED, true)
DEF_VISA_OPTION(vISA_fusedCallWA, ET_INT32, "-fusedCallWA",
                "EU Fusion call ww: 0: no wa; 1: sw wa w/o hw fix; 2: sw wa "
                "with partial HW fix",
                0)
DEF_VISA_OPTION(vISA_fusedCallUniform, ET_BOOL, "-fusedCallUniform",
                "true: fused call is uniform; false otherwise.", false)
DEF_VISA_OPTION(vISA_DstSrcOverlapWA, ET_BOOL, "-dstSrcOverlapWA", UNUSED, true)
DEF_VISA_OPTION(vISA_Src1Src2OverlapWA, ET_BOOL, "-src1Src2OverlapWA", UNUSED,
                false)
DEF_VISA_OPTION(vISA_noSendSrcDstOverlap, ET_BOOL, "-noSendSrcDstOverlap",
                UNUSED, false)
DEF_VISA_OPTION(vISA_enableCloneSampleInst, ET_BOOL, "-cloneSampleInst", UNUSED,
                false)
DEF_VISA_OPTION(vISA_cloneEvaluateSampleInst, ET_BOOL,
                "-cloneEvaluateSampleInst", UNUSED, false)
DEF_VISA_OPTION(vISA_expandMulPostSchedule, ET_BOOL, "-expandMulPostSchedule",
                UNUSED, true)
DEF_VISA_OPTION(vISA_expandMadwPostSchedule, ET_BOOL, "-expandMadwPostSchedule",
                UNUSED, true)
DEF_VISA_OPTION(vISA_disableRegDistDep, ET_BOOL, "-disableRegDistDep", UNUSED,
                false)
DEF_VISA_OPTION(vISA_disableRegDistAllDep, ET_BOOL, "-disableRegDistAllDep",
                UNUSED, false)
DEF_VISA_OPTION(vISA_forceSrc0ToQwForQwShlWA, ET_BOOL,
                "-forceSrc0ToQwForQwShlWA", UNUSED, false)
DEF_VISA_OPTION(vISA_forceNoMaskOnM0, ET_BOOL, "-forceNoMaskOnM0",
                "Convert any NoMask instruction with non-M0 mask offset "
                "to M0 if possible",
                true)
DEF_VISA_OPTION(vISA_addEmaskSetupProlog, ET_BOOL, "-noEmaskSetupProlog",
                "Add a prolog code to set up emask", true)
DEF_VISA_OPTION(vISA_LSCFenceWA, ET_BOOL, "-LSCFenceWA", UNUSED, false)
DEF_VISA_OPTION(vISA_ActiveThreadsOnlyBarrier, ET_BOOL,
                "-activeThreadsOnlyBarrier",
                "This enables the active-only bit in workgroup barriers. "
                "With this option exited threads are not counted in expected "
                "arrival count total and will not cause hangs.", false)
DEF_VISA_OPTION(vISA_SplitBarrierID1, ET_BOOL,
                "-splitbarrierid1",
                "This flag switch ID of the split barrier to 1. "
                "After that change, the workgroupbarrier and splitbarrer"
                "can work togheter and will not cause hangs.", false)
DEF_VISA_OPTION(vISA_RestrictSrc1ByteSwizzle, ET_BOOL,
                "-restrictSrc1ByteSwizzle",
                "Enable the WA to restrict src1 byte swizzle case", false)
DEF_VISA_OPTION(vISA_enableBarrierWA, ET_BOOL,
                "-enableBarrierWA",
                "enable barrier WA which inserts instructions to check the "
                "arrival of the notification in n0.0 brfore sync.bar",
                false)
DEF_VISA_OPTION(vISA_AddIEEEExceptionTrap, ET_BOOL, "-addIEEEExTrap",
                "Add IEEE exception trap which inserts an infinite loop "
                "before EOT to catch any IEEE exception",
                false)
DEF_VISA_OPTION(vISA_noIndirectSrcForCompressedInstWA, ET_BOOL,
                "-noIndirectSrcForCompressedInstWA",
                "Disable WA for fixing compressed instructions with indirect "
                "src0 and cross-grf dst",
                false)
DEF_VISA_OPTION(vISA_TGMDoubleFenceWA, ET_BOOL, "-tgmDoubleFenceWA",
                "enable the WA to double any TGM fence instruction with "
                "flush-type is not FLUSH_NONE",
                false)

//=== HW debugging options ===
DEF_VISA_OPTION(vISA_GenerateDebugInfo, ET_BOOL, "-generateDebugInfo", UNUSED,
                false)
DEF_VISA_OPTION(vISA_setStartBreakPoint, ET_BOOL, "-setstartbp", UNUSED, false)
DEF_VISA_OPTION(vISA_InsertHashMovs, ET_BOOL, NULLSTR, UNUSED, false)
DEF_VISA_OPTION(vISA_InsertDummyMovForHWRSWA, ET_BOOL, "-insertRSDummyMov",
                UNUSED, true)
DEF_VISA_OPTION(vISA_GenerateKernelInfo, ET_BOOL, "-generateKernelInfo", UNUSED,
                false)
DEF_VISA_OPTION(vISA_ManualEnableRSWA, ET_BOOL, "-manualEnableRSWA", UNUSED,
                false)
DEF_VISA_OPTION(vISA_InsertDummyMovForDPASRSWA, ET_BOOL,
                "-insertDPASRSDummyMov", UNUSED, true)
DEF_VISA_OPTION(vISA_InsertDummyCompactInst, ET_BOOL, "-insertDummyCompactInst",
                UNUSED, false)
DEF_VISA_OPTION(vISA_SwapSrc1Src2OfMadForCompaction, ET_BOOL, "-disableSwapSrc1Src2OfMadForCompaction",
                UNUSED, true)
DEF_VISA_OPTION(vISA_AsmFileNameOverridden, ET_BOOL, NULLSTR, UNUSED, false)
DEF_VISA_OPTION(vISA_HashVal, ET_2xINT32, "-hashmovs",
                "USAGE: -hashmovs hi32 lo32\n", 0)
DEF_VISA_OPTION(vISA_HashVal1, ET_2xINT32, "-hashmovs1",
                "USAGE: -hashmovs1 hi32 lo32\n", 0)
DEF_VISA_OPTION(vISA_HashMovsAtPrologue, ET_BOOL, "-hashatprologue", UNUSED,
                false)
DEF_VISA_OPTION(vISA_AddKernelID, ET_BOOL, "-addKernelID", UNUSED, false)
DEF_VISA_OPTION(vISA_zeroSomeARF, ET_BOOL, "-zeroSomeARF",
                "Zero address reg, acc, etc. on entry to kernel.", false)
DEF_VISA_OPTION(vISA_dumpPayload, ET_BOOL, "-dumpPayload", UNUSED, false)
DEF_VISA_OPTION(vISA_ScratchAllocForStackInKB, ET_INT32,
                "-scratchAllocForStackInKB", UNUSED, 128)

DEF_VISA_OPTION(vISA_dumpToCurrentDir, ET_BOOL, "-dumpToCurrentDir", UNUSED,
                false)
DEF_VISA_OPTION(vISA_dumpTimer, ET_BOOL, "-timestats", UNUSED, false)
DEF_VISA_OPTION(vISA_ShaderDataBaseStats, ET_BOOL, "--sdbStats", UNUSED, false)
DEF_VISA_OPTION(vISA_ShaderDataBaseStatsFilePath, ET_CSTR, "-sdbStatsFile",
                UNUSED, NULL)
DEF_VISA_OPTION(vISA_3DOption, ET_BOOL, "-3d", UNUSED, false)
DEF_VISA_OPTION(vISA_Stepping, ET_CSTR, "-stepping",
                "USAGE: missing stepping string. ", NULL)
DEF_VISA_OPTION(vISA_Platform, ET_CSTR, "-platform",
                "USAGE: missing platform string. ", NULL)
DEF_VISA_OPTION(vISA_HasEarlyGRFRead, ET_BOOL_TRUE, "-earlyGRFRead", UNUSED, false)
DEF_VISA_OPTION(vISA_EnableProgrammableOffsetsMessageBitInHeader, ET_BOOL,
                NULLSTR, UNUSED, false)
DEF_VISA_OPTION(vISA_staticProfiling, ET_BOOL, "-staticProfiling", UNUSED, true)
DEF_VISA_OPTION(vISA_staticBBProfiling, ET_BOOL, "-staticBBProfiling", UNUSED, false)
DEF_VISA_OPTION(vISA_hasMulMacRSIssue, ET_BOOL, "-hasMulMacRSIssue", UNUSED, false)
DEF_VISA_OPTION(
    vISA_threadSchedPolicy, ET_INT32, "-threadSchedPolicy",
    "HW thread scheduling policy: 0: single thread first; 1: round robin;", 0)
DEF_VISA_OPTION(vISA_SendQueueEntries, ET_INT32, "-sendQueueEntries", UNUSED, 0)
DEF_VISA_OPTION(vISA_SendQueueSched, ET_BOOL, "-sendQueueSched", UNUSED, false)
DEF_VISA_OPTION(vISA_multiplePipeSched, ET_BOOL, "-multiplePipeSched", UNUSED, false)
DEF_VISA_OPTION(vISA_enableEfficient64b, ET_BOOL, "-enableEfficient64b",
                "Enable generation of efficient64b send instructions", false)
DEF_VISA_OPTION(vISA_enableOverfetch, ET_BOOL, "-disableOverfetch",
                "Disable setting overfetch to load's message descriptor", true)
DEF_VISA_OPTION(vISA_enableOptimizeSIMD32, ET_BOOL_TRUE, "-optimizeSIMD32",
                "Optimize for SIMD32", true)
DEF_VISA_OPTION(vISA_enable320and448Vrt, ET_BOOL_TRUE, "-enable320and448Vrt",
                "Enable VRT config 320/448 GRFs", false)
DEF_VISA_OPTION(vISA_GRFBumpUpNumber, ET_INT32, "-GRFBumpUpNumber",
                "Sets the number of steps/configs which the RA will try to use (during retry) to compile the kernel",
                1)
