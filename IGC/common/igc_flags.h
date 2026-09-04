/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Template:
// DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, flagAvailability)
//
// flagAvailability - The value is responsible for the availability of the flag depending on the build type.
//               DEBUG_ONLY - the flag is available only in debug builds.
//               RELEASE_DIAGNOSTIC - the flag is available in release builds only when
//                          IGC_OPTION__ENABLE_DIAGNOSTIC_FLAGS_IN_RELEASE is set to ON.
//               ALWAYS - the flag is available in both debug and release builds.

#include "common/EmUtils.h"

DECLARE_IGC_GROUP("VISA optimization")
DECLARE_IGC_REGKEY(DWORD, VISALTO, 0, "vISA LTO optimization flags. check LINKER_TYPE for more details", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, DisableSendS, false,
    "Setting this to 1/true adds a compiler switch to not generate sends commands, default is to enable sends ",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForcePreserveR0, false, "Setting this to true makes VISA preserve r0 in r0", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnablePreemption, true, "Enable generating preeemptable code (SKL+)", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForcePreemptionWA, false, "Force generating preemptable code across platforms", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableVISANoSchedule, false, "Enable VISA No-Schedule", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableVISAPreSched, true, "Enable VISA Pre-RA Scheduler", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableVISASBIDCounter, false, "Disable VISA SBID Counter feature", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, VISAPreSchedCtrl, 0,
                   "Configure Pre-RA Scheduler, default(0), logging(1), latency(2), pressure(4)", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, VISAPreSchedCtrlDpas, 0, "Special Pre-RA Scheduler configuration for kernels with dpas",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceVISAPreSched, false, "Force enabling of VISA Pre-RA Scheduler", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, VISAPreSchedRPThreshold, 0,
                   "Threshold to commit a pre-RA Scheduling without spills, 0 for the default", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, VISAPreSchedExtraGRF, 0,
                   "Bump up GRF number to make pre-RA Scheduling more greedy, 0 for the default", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, VISAScheduleStartBBID, 0, "The ID of BB which will be first scheduled", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, VISAScheduleEndBBID, 0, "The ID of BB which will be last scheduled", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, VISAPostScheduleStartBBID, 0, "The ID of BB which will be first scheduled", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, VISAPostScheduleEndBBID, 0, "The ID of BB which will be last scheduled", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, VISASpillAllowed, 256,
                   "Spill size allowed without increasing GRF number in VRT. Overridden by VISADynamicSpillAllowed.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, VISADynamicSpillAllowed, false,
                   "Let finalizer decide spill size allowed to not increase GRF number in VRT. "
                   "Enabling this option overrides VISASpillAllowed and increase the spill threshold "
                   "for simdness by VISADynamicSpillThresholdPercent. ",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, VISADynamicSpillThresholdPercent, 5,
                   "Percentage of the kernel's total instructions allowed to be spill/fill traffic "
                   "when VISADynamicSpillAllowed is set. Also used as the multiplier applied to the "
                   "SIMD spill threshold for simdness selection.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, VISADynamicSpillSamplerWeight, -1,
                   "Weight applied to each non-LSC sampler send when estimating memory pressure for "
                   "the dynamic spill threshold. Negative values raise the spill budget for "
                   "sampler-heavy kernels.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, VISASpillAllowed256GRF, 0, "Spill size allowed specifically for 256 GRF case", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, VISAGRFBumpUpNumber, 1,
                   "Sets the number of steps/configs which the RA will try to use (during retry) to compile the kernel",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, ForceAllowSmallSpill, 0,
                   "Allow small spills regardless of SIMD, API, or platform. The spill amount is set below", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, SIMD8_SpillThreshold, 2, "Percentage of instructions allowed for spilling on SIMD8",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, SIMD16_SpillThreshold, 1, "Percentage of instructions allowed for spilling on SIMD16",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, SIMD32_SpillThreshold, 1, "Percentage of instructions allowed for spilling on SIMD32",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, CSSIMD16_SpillThreshold, 1, "Percentage of instructions allowed for spilling on CS SIMD16",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, CSSIMD32_SpillThreshold, 1, "Percentage of instructions allowed for spilling on CS SIMD32",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, CSSIMD32_HighThresholdInstCount, 1000,
                   "Instructions count limit to allow higher spill threshold on CS SIMD32", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableCSEL, false, "disable csel peep-hole", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableFlagOpt, false, "Disable optimization cmp with logic op", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableIfCvt, false, "Disable ifcvt", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, LocalCSEForSendPayloadCopy, false, "Enable local CSE for the send payload mov intructions",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableVISANoBXMLEncoder, false, "Enable VISA No-BXML encoder", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableIGAEncoder, false, "Enable VISA IGA encoder", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableVISADumpCommonISA, false, "Enable VISA Dump Common ISA", ALWAYS)
DECLARE_IGC_REGKEY(bool, DumpVISAASMToConsole, false, "Dump VISAASM to console and do early exit", ALWAYS)
DECLARE_IGC_REGKEY(bool, DumpASMToConsole, false, "Dump ASM to console and do early exit", ALWAYS)
DECLARE_IGC_REGKEY(bool, AddVISADumpDeclarationsToEnd, false,
                   "Add a comment with .decl section to the end of VISA console dump. Used in tests.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableVISABinary, false, "Enable VISA Binary", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableVISAOutput, false, "Enable VISA GenISA output", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableVISASlowpath, false, "Enable VISA Slowpath. Needed to dump .visaasm", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableVISADotAll, false, "Enable VISA DotAll. Dumps dot files for intermediate stages",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableVISADebug, false, "Runs VISA in debug mode, all optimizations disabled", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, EnableVISAStructurizer, 1,
                   "Enable/Disable VISA structurizer. See value defs in igc_flags.hpp.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableVISAJmpi, true, "Enable/Disable VISA generating jmpi (scalar jump).", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, ForceVISAStructurizer, false,
    "Force VISA structurizer for testing. Used on platforms in which we turns off SCF and use UCF by default",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableVISABoundsChecking, true, "Enable VISA bounds checking.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, MaxPerThreadScratchSpaceOverride, 0,
                   "Override the maximum per-thread scratch space limit for testing purposes. This setting simulates "
                   "hardware with constrained scratch memory and is propagated to both IGC and vISA. Note: vISA has "
                   "its own PTSS query function that will also respect this override",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, NoMaskWA, true, "Enable NoMask WA by using software-computed emask flag", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceNoMaskWA, false, "[tmp, testing] Force NoMaskWA on any platforms", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableCallUniform, true, "[tmp, testing] Ignore indirect call's uniform", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableCallWA, true, "Control call WA when EU fusion is on. 0: off; 1: on", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableMathDPASWA, false, "PVC math instruction running with DPAS issue", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, ForceSubReturn, true,
    "If a subroutine does not have a return, generate a dummy return if this key is set (to meet visa requirement)",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableKeepDpasMacro, false,
                   "If enabled, dpas macro sequence from input will not be broken up by visa scheduler", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, DisableMixMode, 0, "Disables mix mode in vISA BE.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, DisableHFMath, 0, "Disables HF math instructions.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(debugString, VISAOptions, 0, "Options to vISA. Space-separated options.", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, disableIGASyntax, false, "Disables GEN isa text output using IGA and new syntax.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, disableCompaction, false, "Disables compaction.", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, TotalGRFNum, 0, "Total GRF setting for both IGC-LLVM and vISA", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, TotalGRFNum4CS, 0,
                   "Total GRF setting for both IGC-LLVM and vISA, for ComputeShader-only experiment.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, ReservedRegisterNum, 0, "Reserve register number for spill cost testing.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ExpandPlane, false, "Enable pln to mad macro expansion.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableGatherRSFusionSyncWA, false,
                   "Disable WA for gather instruction when read suppression and EU fusion are enabled.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableBCR, false, "Enable bank conflict reduction.", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceBCR, false, "Force bank conflict reduction, no matter spill or not.", ALWAYS)
DECLARE_IGC_REGKEY(bool, BumpGRFForForceBCR, false, "Bump up GRF mode for force BCR.", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, BCRAluDensityThreshold, 10,
                   "Min percent of bank-conflict-candidate ALU instructions (2-/3-source ops) required to force BCR "
                   "for low register pressure OCL shaders. 0 disables the check.",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, BCRBumpGRFMaxRegPressure, 40,
                   "Max register pressure, in GRFs, for which force BCR with GRF mode bump is applied to OCL shaders.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableForceDebugSWSB, false,
                   "Enable force debugging functionality for software scoreboard generation", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, EnableSWSBInstStall, 0,
                   "Enable force stall to specific(start) instruction start for software scoreboard generation", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, EnableSWSBInstStallEnd, 0,
                   "Enable force stall to end instruction for software scoreboard generation", ALWAYS)
DECLARE_IGC_REGKEY(bool, SWSBMakeLocalWAR, false, "make WAR SBID dependence tracking BB local", ALWAYS)
DECLARE_IGC_REGKEY(bool, PVCSendWARWA, true, "enable PVC send WAR WA", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, WARSWSBLocalStart, 0, "WAR localization start BB", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, WARSWSBLocalEnd, 0, "WAR localization end BB", ALWAYS)
DECLARE_IGC_REGKEY(bool, SWSBReplaceARWithAW, false, "replace .src with .dst", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, EnableIndirectInstStart, 0,
                   "Enable the indirect sent, start with candidate of the id value specified by the key", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, EnableIndirectInstEnd, 0,
                   "Enable the indirect sent, end with candidate of the id value specified by the key", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, EnableSWSBTokenBarrier, 0,
                   "Enable force specific instruction as a barrier for software scoreboard generation", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, SWSBTokenNum, 0, "Total tokens used for SWSB.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableGroupScheduleForBC, false, "Enable bank conflict reduction in scheduling.", ALWAYS)
DECLARE_IGC_REGKEY(bool, SchedWithSendSrcReadCycle, false, "Scheduling with GRF read cycle from send.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableIGASWSB, false, "Use IGA for SWSB", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableRegDistDep, false, "distable regDist dependence", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableQuickTokenAlloc, false, "Insert dependence resolve for kernel stitching", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, EnableGatherWithImmPreRA, 0,
                   "0: disabled, 1: sampler is enabled, 2: other msg enabled, 3 always use s0.0 for send", ALWAYS)
DECLARE_IGC_REGKEY(bool, SetA0toTdrForSendc, false, "Set A0 to tdr0 before each sendc/sendsc", ALWAYS)
DECLARE_IGC_REGKEY(bool, ReplaceIndirectCallWithJmpi, false, "Replace indirect call with jmpi instruction (HW WA)",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceUniformSurfaceSampler, false, "Force surface and sampler operand to be uniform",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceUniformBuffer, false, "Force buffer operand to be uniform", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, AssumeUniformIndirectCall, false, "Assume indirect call is uniform to avoid looping code",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableHWGenerateThreadID, true,
                   "Enable new behavior of HW generating threadID for GPGPU pipe. XeHP and non-OCL only.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableHWGenerateThreadIDForTileY, true,
                   "Enable HW generating threadID for GPGPU pipe for TileY mode. XeHP and non-OCL only.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableNonOCLWalkOrderSel, true,
                   "Enable WalkOrder selection for HW generating threadID for GPGPU pipe. XeHP and non-OCL only.",
                   ALWAYS)
DECLARE_IGC_REGKEY(
    DWORD, EnablePassInlineData, 0,
    "1: Force pass 1st GRF of cross-thread payload as inline data; -1: Force disable passing inline data", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceInlineDataForXeHPC, false, "Force InlineData for XeHPC. For testing purposes.", ALWAYS)
DECLARE_IGC_REGKEY(
    bool, EnablePromoteI8, true,
    "Enable promoting i8 (char) to i16 on all ALU insts that does support i8. It's only for XeHPC+ for now.", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForcePromoteI8, false, "Force promoting i8 (char) to i16 on all ALU insts (for testing).",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DumpPromoteI8, false, "Dump useful info during promoting i8 to i16", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableQWRotateInstructions, true, "Enable QW type support for rotate instructions. PVC only.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DPASTokenReduction, false, "optimization to reduce the tokens used for DPAS instruction.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableAdd3, true, "Enable Add3. XeHP+ only", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableBfn, true, "Enable Bfn. XeHP+ only", ALWAYS)
DECLARE_IGC_REGKEY(bool, SeparateSpillPvtScratchSpace, false,
                   "Separate scratch spaces for spillfill and privatememory. XeHP and above only. Test only. Remove it "
                   "once stabalized.",
                   ALWAYS)
DECLARE_IGC_REGKEY(
    bool, EnableSeparateScratchWA, false,
    "Apply the workaround in slot0 and slot1 sizes when separating scratch spacesSeparate scratch space.", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableThreeALUPipes, false, "Disable three ALU Pipelines. XeHP only", ALWAYS)
DECLARE_IGC_REGKEY(bool, Enable16DWURBWrite, false, "Enable 16 Dword URB Write messages", ALWAYS)
DECLARE_IGC_REGKEY(bool, Enable16OWSLMBlockRW, true, "Enable 16 OWord (8 GRF) SLM block read/write message", ALWAYS)
DECLARE_IGC_REGKEY(bool, Enable64BMediaBlockRW, false, "Enable 64 byte wide media block read/write message", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableUntypedSurfRWofSS, true, "Enable untyped surface RW to scratch space. XeHP A0 only.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, GetSendAfterWriteDistance, false, "Get the after write dependence distance", ALWAYS)
DECLARE_IGC_REGKEY_ENUM(EnableOverfetching, -1,
                        "Controls the behavior of overfetching (prefetching 256B by LSC)"
                        "-1 - default behavior,"
                        " 0 - force disabled,"
                        " 1 - force enabled",
                        TRIBOOL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableReadStateToA64Read, false,
                   "Instead of using Read State info to fetch surface format etc use direct A64 read of Surface state "
                   "for Xe3P+ platforms",
                   DEBUG_ONLY)

DECLARE_IGC_REGKEY(DWORD, ForceHWThreadNumberPerEU, 0, "Total HW thread number per-EU.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, UseMathWithLUT, false,
                   "Use the implementations of cos, cospi, log, sin, sincos, and sinpi with Look-Up Tables (LUT).",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, GlobalSendVarSplit, false, "Enable global send variable splitting when we are about to spill",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, EnableSendFusion, 1,
                   "Enable(!=0)/disable(0)/force(2) send fusion. Valid for simd8 shader/kernel only.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableAtomicFusion, false,
                   "To enable/disable atomic send fusion (simd8 shaders). Valid if EnableSendFusion is on.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, AvoidDstSrcGRFOverlap, false,
                   "avoid GRF overlap for destination and source operands of an SIMD16/SIMD32 instruction ", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, AvoidSrc1Src2Overlap, false,
                   "avoid src1 and src2 GRF overlap to avoid the conflict without read suppression ", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, UseLinearScanRA, false, "use Linear Scan as default register allocation algorithm ",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableWriteCombine, false, "Disable write combine. PVC+ only", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, Force32bitConstantGEPLowering, false,
                   "Go back to old version of GEP lowering for constant address space. PVC only", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, GEPLoweringTruncOptEnabled, false,
                   "Enable using truncation to avoid recalculation in GEP lowering", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, NewSpillCostFunction, false, "Use new spill cost function in VISA RA", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableCoalesceScalarMoves, true, "Enable scalar moves to be coalesced into fewer moves",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, SpillCompressionThresholdOverride, 0,
                   "Set a threshold number (1K based) to run with spill compression", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableRemoveLoopDependency, false,
                   "Enable removing of fantom loop dependency introduced by SROA", ALWAYS)
DECLARE_IGC_GROUP("IGC Optimization")
// Default ON for LLVM 16+ (which has full New Pass Manager support), OFF for LLVM 15
// and older.
#if LLVM_VERSION_MAJOR / 16
DECLARE_IGC_REGKEY(bool, EnableOCLNewPassManager, true, "Enable the LLVM New Pass Manager for compute IGC passes",
                   ALWAYS)
#else
DECLARE_IGC_REGKEY(bool, EnableOCLNewPassManager, false, "Enable the LLVM New Pass Manager for compute IGC passes",
                   ALWAYS)
#endif
DECLARE_IGC_REGKEY(bool, AllowMem2Reg, false,
                   "Setting this to true makes IGC run mem2reg even when optimizations are disabled", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableIGCOptimizations, false,
                   "Setting this to 1/true adds a compiler switch to disables all the above IGC optimizations",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableLLVMGenericOptimizations, false, "Disable LLVM generic optimization passes", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableCodeSinking, false,
                   "Setting this to 1/true adds a compiler switch to disable code-sinking", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableCodeSinkingInputVec, false,
                   "Setting this to 1/true disable sinking inputVec inst (test)", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, DisableCodeSinkingLongLatencyInsts, false,
    "Setting this to 1/true disable sinking long latency instructions. (Currently, Sample instructions only)",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableSampleResultLatencySink, false,
                   "Sink the consumer chain of a long-latency sampler/ld/gather send toward its distant use to hide "
                   "send latency, leaving the send in place. May increase register pressure.",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, CodeSinkingMinSize, 32, "Don't sink if the number of instructions in the kernel is less",
                   DEBUG_ONLY)

// Code Loop Sinking
DECLARE_IGC_REGKEY(bool, DisableLoopSink, false, "Disable sinking in all loops", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceLoopSink, false, "Force sinking in all loops", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableLoadsLoopSink, true, "Allow sinking of loads in the loop", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceLoadsLoopSink, false, "Force sinking of loads in the loop from the beginning", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, PrepopulateLoadChainLoopSink, true,
                   "Check the loop for loop chains before sinking to use the existing chains in a heuristic",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableLoadChainLoopSink, true,
                   "Allow sinking of load address calculation when the load was sinked to the loop, even if the needed "
                   "regpressure is achieved (only single use instructions)",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, LoopSinkRegpressureMargin, 10,
                   "Sink into the loop until the pressure becomes less than #grf-margin", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, CodeLoopSinkingMinSize, 100,
                   "Don't sink in the loop if the number of instructions in the kernel is less", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, CodeSinkingLoadSchedulingInstr, 20,
                   "Instructions number to step to schedule loads in advance before the load use to cover latency. 0 "
                   "to insert it immediately before use",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, CodeSinking2dLoadSchedulingInstr, 5,
                   "Instructions number to step to schedule 2d loads in advance before the load use to cover latency. "
                   "0 to insert it immediately before use",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, LoopSinkMinSaveUniform, 6,
                   "If loop sink can have save more scalar (uniform) values than this Minimum, do it; otherwise, skip",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, LoopSinkMinSave, 1,
                   "If loop sink can have save more 32-bit values than this Minimum, do it; otherwise, skip",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, LoopSinkThresholdDelta, 30,
                   "Do loop sink If the estimated register pressure is higher than this + #avaialble registers",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, LoopSinkRollbackThreshold, 15,
                   "Rollback loop sinking if the estimated regpressure after the sinking is still higher than this + "
                   "#available registers, and the number of registers can be increased",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, LoopSinkEnableLoadsRescheduling, true, "Allow sinking the loads that are already in the loop",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, LoopSinkCoarserLoadsRescheduling, false,
                   "Try to reschedule multi-instruction load candidates in larger chunks", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, LoopSinkEnable2dBlockReads, true, "Allow sinking of the 2d block reads", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, LoopSinkEnableVectorShuffle, true, "Allow sinking of the lowered vector shuffle pattern",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, LoopSinkForceRollback, false, "Rollback every loop sinking change (for debug purposes only)",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, LoopSinkDisableRollback, false,
                   "Disable loopsink rollback completely (even in case of increased regpressure)", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, LoopSinkAvoidSplittingDPAS, true,
    "Sink before the whole DPAS sequence if the first use of the sinked instruction is not the first DPAS", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, LoopSinkForce2dBlockReadsMaxSink, true,
                   "Sink as much as possible in presence of 2d block loads", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, LoopSinkEnableLateRescheduling, false,
                   "Schedule more aggressively in the end if the needed regpressure is still not achieved", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, LoopSinkSkipDPASMacro, false, "If a dpas macro sequence is present, skip load sinking", ALWAYS)

// Load Splitting
DECLARE_IGC_REGKEY(bool, LS_enableLoadSplitting, true, "Enable load splitting pass.", ALWAYS)
DECLARE_IGC_REGKEY(bool, LS_ignoreSplitThreshold, false,
                   "If true, the pass splits loads regardless of the register pressure.", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, LS_minSplitSize_GRF, 1, "Minimal split size in GRFs.", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, LS_minSplitSize_E, 4, "Minimal split size in elements.", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, LS_splitThresholdDelta_GRF, 2,
                   "Register pressure must exceed total GRFs by this much for the load splitting to fire up.", ALWAYS)
DECLARE_IGC_REGKEY(bool, LS_onlyStrided, true, "If true, only strided loads are considered for splitting.", ALWAYS)

// Code Scheduling
DECLARE_IGC_REGKEY(bool, DisableCodeScheduling, false, "Disable local code scheduling", ALWAYS)
DECLARE_IGC_REGKEY(bool, CodeSchedulingOnlyRecompilation, false, "Enable code scheduling only on 2nd try", ALWAYS)

DECLARE_IGC_REGKEY(bool, EnableCodeSchedulingIfNoSpills, false, "Try rescheduling also when there are no spills",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, CodeSchedulingGreedyRPHigherRPCommit, false,
                   "If GreedyRP was chosen, commit it also if the estimated RP "
                   "is higher than the original schedule RP",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, CodeSchedulingMWOptimizedHigherRPCommit, true,
                   "If the new schedule is expected to have better latency hiding, "
                   "commit it also if the estimated RP is higher than the original schedule RP",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, CodeSchedulingForceMWOnly, false, "Force scheduling to consider only latency", ALWAYS)
DECLARE_IGC_REGKEY(bool, CodeSchedulingForceRPOnly, false, "Force scheduling to consider only register pressure",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, CodeSchedulingAttemptsLimit, 10, "Limit the number of scheduling attempts", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, CodeSchedulingRPMargin, 15,
                   "Schedule so that the register pressure is less than #grf - margin", ALWAYS)
DECLARE_IGC_REGKEY(bool, CodeSchedulingCommitGreedyRP, true,
                   "Commit greedy regpressure scheduling in case better "
                   "scheduling has not succeed",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, CodeSchedulingRPThreshold, 0,
                   "Do scheduling only if the original register pressure is "
                   "higher than #GRF - margin + threshold",
                   ALWAYS)

DECLARE_IGC_REGKEY(bool, DumpCodeScheduling, false, "Dump code scheduling", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, CodeSchedulingDumpLevel, 1, "Code scheduling dump verbosity level", ALWAYS)
DECLARE_IGC_REGKEY(bool, CodeSchedulingRenameAll, false, "Allow renaming all values for debug purposes", DEBUG_ONLY)
DECLARE_IGC_REGKEY(debugString, CodeSchedulingConfig, 0,
                   "Override the default scheduling config. Debug only - no backward compatibility", ALWAYS)

DECLARE_IGC_REGKEY(DWORD, DumpLatencyHidingEarly, 0,
                   "Dump latency hiding analysis after code scheduling (1=summary, 2=verbose with IR)", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, DumpLatencyHidingFinal, 0,
                   "Dump latency hiding analysis at end of pipeline (1=summary, 2=verbose with IR)", ALWAYS)

DECLARE_IGC_REGKEY(bool, EnableLoopHoistConstant, false,
                   "Enables pass to check for specific loop patterns where variables are constant across all but the "
                   "last iteration, and hoist them out of the loop.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableCodeHoisting, false,
                   "Setting this to 1/true adds a compiler switch to disable code-hoisting", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableDeSSA, true, "Setting this to 0/false adds a compiler switch to disable De-SSA",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableDeSSAWA, true, "[tmp]Keep some piece of code to avoid perf regression", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, DisablePayloadCoalescing, false,
    "Setting this to 1/true adds a compiler switch to disable payload coalescing optimization for all types",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, DisablePayloadCoalescing_RT, false,
    "Setting this to 1/true adds a compiler switch to disable payload coalescing optimization for RT only", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, DisablePayloadCoalescing_Sample, false,
    "Setting this to 1/true adds a compiler switch to disable payload coalescing optimization for Samplers only",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, DisablePayloadCoalescing_URB, false,
    "Setting this to 1/true adds a compiler switch to disable payload coalescing optimization for URB writes only",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, DisablePayloadCoalescing_AtomicTyped, false,
    "Setting this to 1/true adds a compiler switch to disable payload coalescing optimization for atomic typed only",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableUniformAnalysis, false,
                   "Setting this to 1/true adds a compiler switch to disable uniform_analysis", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableWorkGroupUniformGoto, false,
                   "Setting to 1 enables generating uniform goto for work group uniform [eu fusion only]", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, DisablePushConstant, 0,
                   "Bit mask to disable push constant per shader stages. bit0 = All, Bit 1 = VS, Bit 2 = HS, Bit 3 = "
                   "DS, Bit 4 = GS, Bit 5 = PS",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    DWORD, DisableAttributePush, 0,
    "Bit mask to disable push Attribute per shader stages. bit0 = All, Bit 1 = VS, Bit 2 = HS, Bit 3 = DS, Bit 4 = GS",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableSimplePushWithDynamicUniformBuffers, false,
                   "Disable Simple Push Constants Optimization for dynamic uniform buffers.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableStaticCheck, false, "Disable static check to push constants.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableStaticCheckForConstantFolding, true, "Disable static check to fold constants.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(int, forcePushConstantMode, 0,
                   "set the push constant mode, 0 is default behavior, 1 is simple push, 2 is gather constant, 3 is "
                   "none/pull constants",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableSimplePushSizeBasedOpimization, true,
                   "Enable the simplepush optimization to do push based on size", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableConstantCoalescing, false,
                   "Setting this to 1/true adds a compiler switch to disable constant coalesing", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableConstantCoalescingOutOfBoundsCheck, false,
                   "Setting this to 1/true adds a compiler switch to disable constant coalesing out of bounds check",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, DisableConstantCoalescingOfStatefulNonUniformLoads, false,
    "Disable merging non-uniform loads from stateful buffers. Note: does not affect merging to sampler loads",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableWideLSCConstantCoalescing, false,
                   "Let a non-uniform stateful D32/D64 constant coalescing chunk grow to the full LSC message width, "
                   "32 bytes per lane at SIMD16 on a 64-byte GRF platform, instead of one OWORD.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableTextureLoadCoalescing, false, "Enable merging non-uniform loads from bindless textures",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, UseHDCTypedReadForAllTextures, false,
                   "Setting this to use HDC message rather than sampler ld for texture read", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, UseHDCTypedReadForAllTypedBuffers, false,
                   "Setting this to use HDC message rather than sampler ld for buffer read", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableUniformTypedAccess, false, "Setting this will disable uniform typed access handling",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableURBWriteMerge, false,
                   "Setting this to 1/true adds a compiler switch to disable URB write merge", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, SetURBFullWriteGranularity, 0,
                   "Overrides the minimum access granularity for URB full writes."
                   "Valid values are 0, 16 and 32, value 0 means use default for the platform.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableMatchFloor, false,
                   "Setting this to 1/true adds a compiler switch to disable sub-frc = floor optimization", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableEmptyBlockRemoval, false,
                   "Setting this to 1/true adds a compiler switch to disable empty block optimization", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableSIMD32Slicing, false,
                   "Setting this to 1/true adds a compiler switch to disable emitting SIMD32 VISA code in slices",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ZeroInactiveLanesForWaveShuffle, false,
                   "Force-enable the ZeroInactiveLanesForWaveShuffle AIL: make WaveShuffleIndex read zero from "
                   "source lanes that are inactive instead of stale register contents. Useful for testing without "
                   "UMD AIL detection.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableMatchMad, false,
                   "Setting this to 1/true adds a compiler switch to disable mul+add = mad optimization", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, WaAllowMatchMadOptimizationforVS, false,
                   "Setting this to 1/true adds a compiler switch to enable mul+add = mad optimization for VS",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableLoadSinking, false,
                   "Setting this to 1/true adds a compiler switch to disable load sinking during retry", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableIntegerMad, true,
                   "Setting this to 1/true adds a compiler switch to enable integer mul+add = mad optimization",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableMatchPredAdd, false,
                   "Setting this to 1/true adds a compiler switch to disable pred+add = predAdd optimization",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableMatchSimpleAdd, false,
                   "Setting this to 1/true adds a compiler switch to disable simple cmp+and+add optimization",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableMatchPow, false,
                   "Setting this to 1/true adds a compiler switch to disable log2/mul/exp2 = pow optimization",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableIRVerification, false,
                   "Setting this to 1/true adds a compiler switch to disable IGC IR verification.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableJumpThreading, true,
                   "Setting this to 1/true adds a compiler switch to enable llvm jumpThreading pass.", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableLoopUnroll, false,
                   "Setting this to 1/true adds a compiler switch to disable loop unrolling.", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, RuntimeLoopUnrolling, 0,
                   "Setting this to switch on/off runtime loop unrolling. 0: default (on), 1: force on, 2: force off",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableIndVarSimplification, true, "Enables IndVarSimplification pass.", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableBranchSwaping, false,
                   "Setting this to 1/true adds a compiler switch to disable branch swapping.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableSynchronizationObjectCoalescingPass, false,
                   "Disable SynchronizationObjectCoalescing pass", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, EnableIndependentSharedMemoryFenceFunctionality, false,
    "Enable treating global memory fences as shared memory fences in SynchronizationObjectCoalescing pass", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, SynchronizationObjectCoalescingConfig, 0,
                   "Modify the default behavior of SynchronizationObjectCoalescing value is a bitmask bit0 – remove "
                   "fences in read barrier write scenario",
                   ALWAYS)
DECLARE_IGC_REGKEY(
    DWORD, DisableCoalescingSynchronizationObjectMask, 0,
    "The mask is casted to IGC::SyncInstMask and informs which synchronization objects should not be coalesced. Note "
    "that synchronization objects classified in multiple types are not disabled if any bit describing them is off.",
    ALWAYS)
DECLARE_IGC_REGKEY(
    bool, ReplaceAtomicFenceWithSourceValue, true,
    "Fences are required to maintain the order of atomic memory instructions. This flag will replace the fence with "
    "GenISA_source_value intrinsic which sources the result of atomic operation and still maintains the order.",
    ALWAYS)
DECLARE_IGC_REGKEY(bool, UnrollLoopForCodeSizeOnly, false,
                   "Only unroll the loop if it can reduce program size/register pressure. Ignore all other threshold "
                   "setting but still enable PromoteLoopUnrollwithAlloca due to high likelyhood to reduce size.",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, SetLoopUnrollThreshold, 0,
                   "Set the loop unroll threshold. Value 0 will use the default threshold.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, SetLoopUnrollThresholdForHighRegPressure, 200,
                   "Set the loop unroll threshold for shaders with high reg pressure.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, SetLoopUnrollMaxPercentThresholdBoostForHighRegPressure, 400,
                   "Set the loop unroll max allowed threshold boost in percentage for shaders with high reg pressure. "
                   "The LLVM internal value is 400.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY_ENUM(
    ForcePromoteLoopUnrollwithAlloca, -1,
    "Loop cost estimation assumes Load/Store who accesses Alloca with index deductible to loop count having 0 cost. "
    "Disable this flag makes them always cost something as well as disables dynamic threshold increase based on the "
    "size of alloca and number of GEP to the alloca in the loop, leading to the loop less likely to be unrolled."
    "-1 - default behavior, decided by platforms"
    " 0 - force disabled"
    " 1 - force enabled",
    TRIBOOL_OPTIONS, DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, PromoteLoopUnrollwithAllocaCountThreshold, 256,
                   "The loop trip count OR number of alloca elements cutoff to stop regkey "
                   "EnablePromoteLoopUnrollwithAlloca (Check regkey description).",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, SetRegisterPressureThresholdForLoopUnroll, 96,
                   "Set the register pressure threshold for limiting the loop unroll to smaller loops", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, SetSelectPHICountThresholdForUnrollAnalysis, 256,
                   "Skip LLVM's SCEV based full unroll cost analysis in functions with at least this many "
                   "select-like (two-way) PHIs, where ScalarEvolution recursion can overflow the stack. "
                   "Value 0 disables the limit.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, SetBranchSwapThreshold, 400, "Set the branch swaping threshold.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(debugString, LLVMCommandLine, 0, "applies LLVM command line", DEBUG_ONLY)
DECLARE_IGC_REGKEY(debugString, SelectiveHashOptions, 0, "applies options to hash range via string", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, EnablePingPongTextureOpt, true,
    "Enables the Ping Pong texture optimization which is used only for Compute Shaders for back to back dispatches",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, EnableAtomicBranch, 0,
                   "Bitmask to enable Atomic branch optimization that predicates the atomic with if/else. "
                   "Modes 1/0x40/0x80 skip the atomic based on the source value only (no memory read). "
                   "Modes 2/4/0x100/0x200 first read the current memory value and skip the atomic when it "
                   "would not change memory. "
                   "1: if Val == 0 skip iadd/sub/umax (source-only). "
                   "2: read memory, skip umax when memory is already >= Val. "
                   "4: read memory, skip umin when memory is already <= Val. "
                   "8: generate ld_lz for untyped atomics, otherwise load_ugm. "
                   "0x10: split stateless atomics. "
                   "0x20: also handle 64-bit atomics. "
                   "0x40: if Val == 0 skip AtomicOr (source-only). "
                   "0x80: if Val == all-ones skip AtomicAnd (source-only). "
                   "0x100: read memory, skip AtomicOr when it would set no new bits. "
                   "0x200: read memory, skip AtomicAnd when it would clear no bits. ",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableThreeWayLoadSpiltOpt, false, "Enable three way load spilt opt.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableTypedWriteZeroStoreCheck, false,
                   "Disables eliminating a potential zero store by a typed "
                   "write instruction (moving the instruction under a "
                   "if-statement to guarantee a non-zero store)",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    DWORD, EnableSamplerChannelReturn, 1,
    "Setting this to 1/true adds a compiler switch to enable using header to return selective channels from sampler."
    "Setting this to 2 makes it always use the selected channels, without heuristic.",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisablePromotePrivMem, false,
                   "Setting this to 1/true adds a compiler switch to disable IGC private array promotion", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableSimplifyGEP, true, "Enable IGC to simplify indices expr of GEP.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableCustomUnsafeOpt, false, "Disable IGC to run custom unsafe optimizations", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableReducePow, false, "Disable IGC to reduce pow instructions", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableSqrtOpt, false, "Prevent IGC from doing the optimization y*y = x if y = sqrt(x)",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableFastMath, false, "Enable fast math optimizations in IGC", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableFlattenSmallSwitch, false, "Disable the flatten small switch pass", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableIPConstantPropagation, false, "Disable Inter-procedrual constant propgation",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableSplitIndirectEEtoSel, true, "Enable the split indirect extractelement to icmp+sel pass",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, SplitIndirectEEtoSelThreshold, 8, "Split indirect extractelement cost threshold", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    DWORD, ExpandNonUniformInsertElementThreshold, 0,
    "Convert non-uniform insertelement instructions for fixed arrays of size less than or equal to this threshold",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, EnablePropagateCmpUniformity, 1,
                   "Enable propagation of compare-based uniformity: replace non-uniform/divergent values with uniform "
                   "values in dominated BBs as determined by WIAnalysis (1 enable, 2 enable and print)",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableImmConstantOpt, false, "Disable IGC IndirectICBPropagaion optimization", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, MaxImmConstantSizePushed, 256, "Set the max size of immediate constant buffer pushed",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RemoveUnusedTGMFence, false, "Remove TGM Fences that are not used/read", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableMCSOpt, false, "Disable IGC to run MCS optimization", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, MCSOptTwoStagesMode, false, "MCSOptimization gather all candidates than process", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableGatingSimilarSamples, false, "Disable Gating of similar sample instructions",
                   DEBUG_ONLY)

DECLARE_IGC_REGKEY(bool, EnableInterpreterPatternMatching, false,
                   "Enable Interpreter pattern matching and force retry if the pattern was found.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableSumFractions, false, "Enable SumFractions optimization in CustomUnsafeOptPass.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableExtractCommonMultiplier, false,
                   "Enable ExtractCommonMultiplier optimization in CustomUnsafeOptPass.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnablePowToLogMulExp, false,
                   "Enable pow to exp(log(x)*y) optimization in CustomUnsafeOptPass.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisablePullConstantHeuristics, true,
                   "Disable the heuristics to determine the no. push constants based on payload size.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, PayloadSizeThreshold, 11,
                   "Set the max payload size threshold for short shades that have PSD bottleneck.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, BlockPushConstantGRFThreshold, 0xFFFFFFFF,
                   "Set the maximum limit for block push constants i.e. UBO data pushed.\
                                                                Set to 0xFFFFFFFF to use the default threshold for the platform.\
                                                                Note that for small pixel shaders the PayloadSizeThreshold may be the limiting factor.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, PSSIMD32HeuristicFP16, true, "enable PS SIMD32 heuristic based on fp16 characteristic ",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, PSSIMD32HeuristicLoopAndDiscard, true,
                   "enable PS SIMD32 heuristic based on loop info and discard", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableBlendToDiscard, true, "Enable blend to discard based on blend state.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, UseTiledCSThreadOrder, true, "Use 4x4 disaptch for CS order when it seems beneficial",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceLinearWalkOnLinearUAV, false, "Force linear walk on linear UAV buffer", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceSupportsStaticRegSharing, false, "ForceSupportsStaticRegSharing", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceSupportsAutoGRFSelection, false, "ForceSupportsAutoGRFSelection", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableVRT, true, "Enable Variable Register per Thread", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, RovOpt, 3,
                   "Bitmask for ROV optimizations. 0 for all off, 1 for force fence flush none, 2 for setting "
                   "LSC_L1UC_L3C_WB, 3 for both opt on",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableLSCFence, true, "Enable LSC Fence in ConvertDXIL for the device has LSC", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, MinCompressionThreshold, 60,
                   "Set the minimum compression threshold that is desired (100 is disabling it)", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceLocalScopeEvictTGM, false, "Forces upgrading fence.tgm.local.none to evictions",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, EnableStatelessToStateful, true,
    "Enable Stateless To Stateful transformation for global and constant address space in OpenCL kernels", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableStatelessOffsetNarrowing, true,
                   "Enable narrowing of 64-bit stateless pointer arithmetic to 32-bit if offsets fit in 32 bits",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableStatefulToken, true,
                   "Enable to indicate ptr arguments are fully converted to stateful (temporary)", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableConstBaseGlobalBaseArg, false,
                   "Do no generate kernel implicit arguments: constBase and globalBase", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableGenUpdateCB, false, "Enable derived constant optimization.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableGenUpdateCBResInfo, false, "Enable derived constant optimization with resinfo.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceAddressArithSinking, false, "Force sinking address arithmetic closer to the usage",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, SetDefaultTileYWalk, true, "Use TileY walk as default for HW generating threadID", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceTileY, false, "Force TileY mode on DG2", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, EnableNewTileYCheck, 2, "Enable new TileY check. 0 - off, 1 - on, 2 - platform default",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, KeepTileYForFlattened, 2,
                   "Keep TileY for FlattenedThreadIdInGroup. 0 - off, 1 - on, 2 - platform default", DEBUG_ONLY)

DECLARE_IGC_REGKEY(bool, DisableDynamicTextureFolding, false, "Disable Dynamic Texture Folding", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableDynamicResInfoFolding, true, "Disable Dynamic ResInfo Instruction Folding", DEBUG_ONLY)

DECLARE_IGC_REGKEY(DWORD, EnableCodeAssumption, 1,
                   "If set (> 0), generate llvm.assume to help certain optimizations. It is OCL only for now. \
     Only 1 and 2 are valid. 2 will be 1 plus additional assumption. It also does other minor changes.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableHoistMulInLoop, true, "Hoist multiply with loop invirant out of loop, FP unsafe",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableGVN, true, "Enable LLVM global value numbering", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableLogicalAndToBranch, true, "Enable convert logical AND to conditional branch", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableSplitUnalignedVector, true, "Enable Splitting of unaligned vectors for loads and stores",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableTrigFuncRangeReduction, false, "reduce the sin and cosing function domain range", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableUnmaskedFunctions, true, "Enable unmaksed functions SYCL feature.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableStatefulAtomic, false, "Enable promoting stateless atomic to stateful atomic.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableHoistDp3, false, "Enable dp3 Hoisting.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceHoistDp3, false, "force dp3 Hoisting.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableBitcastedLoadNarrowing, false, "Enable narrowing of vector loads in bitcasts patterns.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableBitcastedLoadNarrowingToScalar, false,
                   "Enable narrowing of vector loads to scalar ones in bitcasts patterns.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableOptReportLoadNarrowing, false, "Generate opt report for narrowing of vector loads.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableGEPLSR, true, "Enables GEP Loop Strength Reduction pass", ALWAYS)
DECLARE_IGC_REGKEY(bool, RunGEPLSRAfterLICM, false, "Runs GEP Loop Strength Reduction pass after first LICM", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, GEPLSRThresholdRatio, 100,
                   "Ratio for register pressure threshold in GEP Loop Strength Reduction pass", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableLICMInvariantSwitchDispatchDetection, false,
                   "Enable detection of invariant switch dispatch in LICM.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableGEPLSRToPreheader, true,
                   "Enables reduction to loop's preheader in GEP Loop Strength Reduction pass", ALWAYS)
DECLARE_IGC_REGKEY(
    bool, EnableGEPLSRAnyIntBitWidth, false,
    "Enables reduction of SCEV with illegal integers. Requires legalization pass to clear up expanded code.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableGEPLSRMulExpr, true,
                   "Experimental: Enables reduction with constant, but unknown step if step contains multiplication.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableGEPLSRUnknownConstantStep, false,
                   "Experimental: Enables reduction with constant, but unknown step.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableGEPLSRStrictWrapAroundCheck, false,
                   "Experimental: Enable strict Wrap-around check, relaxed by default", ALWAYS)
DECLARE_IGC_REGKEY(bool, PrintWaveClusteredInterleave, false,
                   "(Debug) Print if WaveClusteredInterleave pattern was found.", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, FPRoundingModeCoalescingMaxDistance, 20,
                   "Max distance in instructions for reordering FP instructions with common rounding mode", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableDotAddToDp4aMerge, false, "Disable Dot and Add ops to Dp4a merge optimization.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableLoopSplitWidePHIs, false,
                   "Disable splitting of loop PHI values to eliminate subvector extract operations", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableBarrierControlFlowOptimizationPass, false,
                   "Enable barrier control flow optimization pass", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableBarrierSkipOptimization, false,
                   "Disable barrier skip optimization for small thread groups", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableWaveShuffleIndexSinking, true,
                   "Hoist identical instructions operating on WaveShuffleIndex instructions with the same source and a "
                   "constant lane/channel",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, WaveShuffleIndexSinkingMaxIterations, 3,
                   "Max number of iterations to run iterative WaveShuffleIndexSinking", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableWaveAllJointReduction, false, "Enable Joint Reduction Optimization.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnablePromoteToPredicatedMemoryAccess, false, "Enable predicated load/store if conversion.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableBranchToSelect, true,
                   "Enable flattening of small speculatable branch regions into selects", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, BranchToSelectMaxSpeculatedInsts, 30,
                   "Max instruction count of a single branch successor BranchToSelect will hoist. A backstop against "
                   "linearizing a pathologically large arm, not the profitability test -- for a divergent branch both "
                   "arms already execute under a lane mask, so profitability is decided on register pressure "
                   "(BranchToSelectMaxPressureDelta). Mirrors LLVM EarlyIfConversion's BlockInstrLimit.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, BranchToSelectMaxPressureDelta, 256,
                   "Max net register pressure one BranchToSelect fold may add, in bytes of register file at the "
                   "SIMD16 reference width: a divergent i32 weighs 64, a uniform i32 weighs 4, a divergent i64 128. "
                   "The default admits four divergent i32 values. A triangle scores 0 and always fits; a diamond is "
                   "charged the smaller of its two arms' live-out weights. Merge PHIs replaced 1:1 by a select are "
                   "not charged, so a chain of independent regions does not accumulate against this budget.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, BranchToSelectDivergentOnly, true,
                   "Restrict BranchToSelect to branches with a divergent (non work-item-uniform) condition. A uniform "
                   "branch is a scalar jump that runs only one arm, so flattening it just makes the not-taken arm's "
                   "work unconditional. Disable to flatten uniform branches too.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableIntDivRemIncrementReduction, true,
                   "Enable consecutive Int DivRem increment by constant optimization", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, DivRemIncrementCondBranchSimplify, false,
    "Create branches when simplifying consecutive udiv/urem groups increment dividend by constant greater than 1",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, SanitizeDivRemIncrementDivisorIsZero, false,
                   "Add ICmp comparison of divisor to zero to return -1 when performing optimization to avoid UB",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, GuardDivRemIncrementDividendOverflow, false,
                   "Check for no unsigned wrap flag on increment/decrement operation before optimizing", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableSamplerLoopSpeculation, false,
                   "Enable forced partial unrolling and speculative clustering of sampler loop iterations", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableResourceLoopDestLifeTimeStart, true,
                   "Enable lifetime_start set for destination in resource loop", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, EnableSamplerBackingByLSC, 0x0,
                   "Bit mask to enable sampler backing by LSC per shader. Bit 1 = VS, Bit 2 = HS, Bit 3 = "
                   "DS, Bit 4 = GS, Bit 5 = TS, BIT 6 = MESH, BIT 7 = PS, BIT 8 = CS, BIT 9 = OCL, BIT 10 = RT",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableSinkPointerConstAdd, true,
                   "Enable sinking of pointer constant additions closer to their use", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceHoistUDivURem, false,
                   "Always hoist UDiv/URem to common ancestor, even if it results in speculative execution", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, FPRangeAnalysisMaxDepth, 64, "Max recursive depth for FP range analysis", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableSimpleAluVectorizer, true,
                   "Enable coalescing of ALU SIMD1/SIMD2 uniform operations into wider SIMD", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableRedundantOpsCSE, true, "Enable redundant binary operator CSE pass", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableRedundantOpsCrossBBCSE, true, "Enable cross-basic-block CSE in RedundantOpsCSEPass",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, RedundantOpsIntraBBMaxDist, 64,
                   "Maximum instruction distance for inside-BB CSE in RedundantOpsCSEPass (0 = unlimited)", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, RedundantOpsCrossBBMaxDist, 100,
                   "Maximum instruction distance for cross-BB CSE in RedundantOpsCSEPass (0 = unlimited)", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, RedundantOpsCrossBBInstThreshold, 5000,
                   "Disable cross-BB CSE in RedundantOpsCSEPass when shader IR instruction count exceeds this "
                   "threshold (combined with NumBB threshold). 0 = never disable based on inst count.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, RedundantOpsCrossBBNumBBThreshold, 500,
                   "Disable cross-BB CSE in RedundantOpsCSEPass when shader BB count exceeds this threshold "
                   "(combined with inst count threshold). 0 = never disable based on BB count.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableMatchDecomposedHalfExtract, true, "Reconstruct LLVM16+ decomposed half extraction",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableStateIndexAddrChainCanonicalize, true,
                   "Enable surface state index address chain canonicalize pass", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, StateIndexAddrChainCanonicalizeInstThreshold, 3000,
                   "Disable StateIndexAddrChainCanonicalize when the function instruction count exceeds this "
                   "threshold. 0 = never disable based on inst count.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableWIPhiStructuralEquivalence, true,
                   "Keep a phi at a divergent join uniform when all its incomings are "
                   "shape-equivalent expressions (recursive, memory-free). Path-invariance "
                   "per lane; cross-lane uniformity is still decided by calculate_dep. "
                   "Catches GVN-PRE materialization at divergent joins.",
                   DEBUG_ONLY)

DECLARE_IGC_GROUP("Shader debugging")
DECLARE_IGC_REGKEY(bool, CopyA0ToDBG0, false, " Copy a0 used for extended msg descriptor to dbg0 to help debug",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, CopyMsg0ToDbg0, false, " Copy msg0.2 used for Multi-Q AppQID to dbg0 to help debug",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableDebugging, false, " Enable shader debugging for release internal", DEBUG_ONLY)
DECLARE_IGC_REGKEY_BITMASK(
    GenerateOptionsFile, 0,
    "Create Options.txt(usually for SIMD related bugs to narrow down shaders), in the shader dump folder.",
    SHADER_TYPE_MASKS, DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceDisableShaderDebugHashCodeInKernel, false,
                   "Disable hash code addition to the binary after EOT", ALWAYS)
DECLARE_IGC_REGKEY_UMD(bool, EnableHashMovsAtPrologue, false,
                       "Rather than after EOT, insert hash code movs at shader entry", ALWAYS)
DECLARE_IGC_REGKEY(bool, ShaderDebugHashCodeInKernel, false, "Add hash code to the binary", ALWAYS)
DECLARE_IGC_REGKEY(
    int, ShaderDebugHashCode, 0,
    "The driver will set a breakpoint in the first instruction of the shader which has the provided hash code.\
                                                                It works only when the value is different then 0 and SystemThreadEnable is set to TRUE.\
                                                                Ex: VS_asm2df26246434553ad_nos0000000000000000 , only the LowPart Need \
                                                                to be Enterd in Registry Ex : 0x434553ad ,i.e Lower 8 Hex Digits of the 16 Digit Hash Code \
                                                                for Compatibilty Reasons",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableZeroSomeARF, false,
                   "If set, insert mov inst to zero a0, acc, etc to assist HW debugging.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, ShaderDisableOptPassesAfter, 0,
                   "Will only run first N optimization passes, any further passes will be ignored. This flag can be "
                   "used to bisect optimization passes.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ShaderOverride, false,
                   "Will override any LLVM shader with matching name in c:\\Intel\\IGC\\ShaderOverride or "
                   "/tmp/IntelIGC/ShaderOverride",
                   RELEASE_DIAGNOSTIC)
DECLARE_IGC_REGKEY(bool, CompileOneAtTime, false,
                   "Compile only one kernel (out of many in llvm::module) at a time. Prints compiled kenrels names to "
                   "stdout. Useful to debug compilation time and crashes - it does not produce valid binary.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, SystemThreadEnable, false,
    "This key forces software to create a system thread. The system thread may still be created by software even \
                                                                if this control is set to false.The system thread is invoked if either the software requires \
                                                                exception handling or if kernel debugging is active and a breakpoint is hit.",
    DEBUG_ONLY)

DECLARE_IGC_REGKEY(bool, EnableSIPOverride, false, "This key forces load of SIP from a a Local File.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(debugString, SIPOverrideFilePath, 0,
                   "This key when enabled with EnableSIPOverride load of SIP from a specified path.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DumpPayloadToScratch, false,
                   "Setting this to 1/true dumps thread payload to scartch space. Used for  workloads which doesnt use "
                   "scartch space for other purposes",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, SToSProducesPositivePointer, false,
                   "This key is for StatelessToStateful optimization if the  user knows the pointer offset is postive "
                   "to the kernel argument.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableSupportBufferOffset, false,
                   "[debugging]For StatelessToStateful optimization [OCL], support implicit buffer offset argument "
                   "(same as -cl-intel-has-buffer-offset-arg).",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableOptionalBufferOffset, true,
                   "For StatelessToStateful optimization [OCL], if true, make buffer offset optional. Valid only if "
                   "buffer offset is supported.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableTestIGCBuiltin, false, "Enable testing igc builtin (precompiled kernels) using OCL.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, TestIGCPreCompiledFunctions, false, "Enable testing for precompiled kernels. [TEST ONLY]",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, ForceEmuKind, 0,
                   "Force emuKind used by PreCompiledFuncImport pass. This flag takes emulation kind value that is "
                   "defined in EmuKind enum in PreCompiledFuncImport.hpp [TEST ONLY]",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceCSSIMD32, false, "Force computer shader SIMD32 mode", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceCSSIMD16, false,
                   "Force computer shader SIMD16 mode if allowed, otherwise it will use SIMD32", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceRecompilation, false, "Force RetryManager to make recompilation", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, HandlePhiNodeInChannelPrune, false,
                   "During channel prune don't stop at phinode but look at it's users.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, ld2dmsInstsClubbingThreshold, 3,
                   "Do not club more than these ld2dms insts into the new BB during MCSOpt", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, Splitld2dmsAfterFirst, false,
                   "Instead of splitting after second ld2dms message, split after first to avoid waiting", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, ForcePerThreadPrivateMemorySize, 0,
                   "Useful for ensuring a certain amount of private memory when doing a shader override.", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, RetryManagerFirstStateId, 0,
                   "For debugging purposes, it can be useful to start on a particular id rather than id 0.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableSendSrcDstOverlapWA, false,
                   "Disable Send Source/destination overlap WA which is enabled for GEN10/GEN11 and whenever Wddm2Svm "
                   "is set in WATable",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    debugString, DisablePassToggles, 0,
    "Disable each IGC pass by setting the bit. HEXADECIMAL ONLY!. Ex: C0 is to disable pass 6 and pass 7.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ShaderDisplayAllPassesNames, false,
                   "Display to console all passes name with their ID and occurrence number.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    debugString, ShaderPassDisable, 0,
    "Disable specific passes eg. '9;17-19;239-;Error Check;ResolveOCLAtomics:2;Dead Code Elimination:3-5;BreakConstantExprPass:7-' \
                                                                disable pass 9, disable passes from 17 to 19, disable all passes after 238, disable all occurrences of pass Error Check, \
                                                                disable second occurrence of ResolveOCLAtomics, disable pass Dead Code Elimination occurrences from 3 to 5, \
                                                                disable all BreakConstantExprPass after his 6 occurrence \
                                                                To show a list of pass names and their occurrence set ShaderDisplayAllPassesNames.\
                                                                Must be used with ShaderDumpEnableAll flag.",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, PrintVerboseGenericControlFlowLog, 0,
                   "Forces compiler to print detailed log about additional control flow generated due to a presence of "
                   "generic memory operations",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceStatelessForQueueT, true,
                   "In OCL, force to use stateless memory to hold queue_t*. This is a legacy feature to be removed.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, ForceMemoryFenceBeforeEOT, false,
    "Forces inserting SLM or gloabal memory fence before EOT if shader writes to SLM or goblam memory respectively.",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, MSAAClearedKernel, 0, "Insert the discard code for MSAA_MSC_Cleared kernels. 2/4/8/16",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnablerReadSuppressionWA, true, "Enable read suppression WA for the send and indirect access",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableLSCFenceUGMBeforeEOT, true,
                   "Enable inserting fence.ugm.06.tile before EOT if a kernel has any write to UGM [XeHPC, PVC].",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableRTLSCFenceUGMBeforeEOT, true,
                   "[tmp]Enable inserting fence.ugm.06.tile before EOT for RT shader [XeHPC, PVC].", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, manualEnableRSWA, false, "Enable read suppression WA for the send and indirect access",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DPASReadSuppressionWA, true, "Enable read suppression WA for the send and indirect access",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableDivergentBarrierCheck, false,
                   "Uses WIAnalysis to find barriers in divergent flow control. May have false positives.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableBitcastExtractInsertPattern, true,
                   "Enable BitcastExtractInsertPattern in CustomSafeOptPass.", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, ForceLoosenSimd32Occu, 2,
                   "Control loosenSimd32occu return value. 0 - off, 1 - on, 2 - platform default", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceFunctionsToNop, false,
                   "Replace functions with immediate return to help narrow down shaders; use with Options.txt.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableWarnings, false, "Disable all warnings generated from IGC compiler", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableDuplicateWarnings, true, "Limit duplicate warnings to a single occurrence", ALWAYS)

DECLARE_IGC_GROUP("Shader dumping")
DECLARE_IGC_REGKEY(bool, EnableCosDump, false, "Enable cos dump", ALWAYS)
DECLARE_IGC_REGKEY(bool, DumpLLVMIR, false, "dump LLVM IR", ALWAYS)
DECLARE_IGC_REGKEY(bool, QualityMetricsEnable, false, "Enable Quality Metrics for IGC", ALWAYS)
DECLARE_IGC_REGKEY(bool, ShaderDumpEnable, false, "dump LLVM IR, visaasm, and GenISA", ALWAYS)
DECLARE_IGC_REGKEY(bool, ShaderDumpEnableAll, false,
                   "dump all LLVM IR passes, visaasm, and GenISA; force recomputation of analysis passes at the "
                   "beginning of every pass. ",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, ShaderDumpEnableG4, false,
                   "same as ShaderDumpEnable but adds G4 dumps (0 = off, 1 = some, 2 = all)", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, ShaderDumpEnableIGAJSON, false,
                   "adds IGA JSON output to shader dumps (0 = off, 1 = enabled, 2 = include def/use info but causes "
                   "longer compile times)",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ShaderDumpEnableRAMetadata, false, "adds RA Metadata file to shader dumps", ALWAYS)
DECLARE_IGC_REGKEY(
    bool, ShaderDumpInstNamer, false,
    "dump all unnamed LLVM IR instruction with variable names 'tmp' which makes easier for shaderoverriding", ALWAYS)
DECLARE_IGC_REGKEY(debugString, ShaderDumpRegexFilter, 0, "Only dump files matching the given regex", ALWAYS)
DECLARE_IGC_REGKEY_ENUM(ShaderDumpCollisionMode, 0, "What to do when file collision happens", FILENAME_COLLISION_MODES,
                        ALWAYS)
DECLARE_IGC_REGKEY(bool, DumpZEInfoToConsole, false, "Dump zeinfo to console", ALWAYS)
DECLARE_IGC_REGKEY(debugString, ProgbinDumpFileName, 0,
                   "Specify filename to use for dumping progbin file to current dir", ALWAYS)
DECLARE_IGC_REGKEY(bool, ElfDumpEnable, false, "dump ELF file", ALWAYS)
DECLARE_IGC_REGKEY(bool, ElfTempDumpEnable, false, "dump temporary ELF files", ALWAYS)
DECLARE_IGC_REGKEY(bool, SpvAsmDumpEnable, false, "Dump spvasm file", ALWAYS)
DECLARE_IGC_REGKEY(
    debugString, DebugDumpNamePrefix, 0,
    "Set a prefix to debug info dump filenames(with path) and drop hash info from them (for testing purposes)", ALWAYS)
DECLARE_IGC_REGKEY(bool, ShowFullVectorsInShaderDumps, false,
                   "print all elements of vectors in ShaderDumps, can dramatically increase ShaderDumps size", ALWAYS)
DECLARE_IGC_REGKEY(bool, PrintHexFloatInShaderDumpAsm, true, "print floats in hex in asm dump", ALWAYS)
DECLARE_IGC_REGKEY(bool, PrintInstOffsetInShaderDumpAsm, false, "print instruction offsets as comments in asm dump",
                   ALWAYS)
DECLARE_IGC_REGKEY(debugString, PrintAfter, 0,
                   "Take either all or comma/semicolon-separated list of pass names. If set, enable print LLVM IR "
                   "after the given pass is done (mimic llvm print-after)",
                   ALWAYS)
DECLARE_IGC_REGKEY(debugString, PrintBefore, 0,
                   "Take either all or comma/semicolon-separated list of pass names. If set, enable print LLVM IR "
                   "before the given pass is done (mimic llvm print-before)",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, PrintMDBeforeModule, false,
                   "Print metadata of the module at the beginning of the dump. Used for LIT tests.", ALWAYS)
DECLARE_IGC_REGKEY(bool, DumpUseShorterName, true, "If set, use an internal shader name(_entry_id) in dump file name",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableKernelNamesBasedHash, false,
                   "If set, use kernels' names to calculate the hash. Doesn't work on .cl dump's hash. Will overwrite "
                   "dumps if multiple modules have the same kernel names.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, InterleaveSourceShader, true, "Interleave the source shader in asm dump", ALWAYS)
DECLARE_IGC_REGKEY(bool, ShaderDumpPidDisable, false, "disabled adding PID to the name of shader dump directory",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DumpToCurrentDir, false, "dump shaders to the current directory", ALWAYS)
DECLARE_IGC_REGKEY(debugString, DumpToCustomDir, 0, "Dump shaders to custom directory. Parent directory must exist.",
                   ALWAYS)
DECLARE_IGC_REGKEY(debugString, ShaderOverrideFromDir, 0,
                   "Override shaders from a custom directory instead of the default DumpDir/ShaderOverride/. "
                   "The directory must exist. The OverrideLog.txt is also written here.",
                   RELEASE_DIAGNOSTIC)
DECLARE_IGC_REGKEY(bool, EnableShaderNumbering, false,
                   "Number shaders in the order they are dumped based on their hashes", ALWAYS)
DECLARE_IGC_REGKEY(bool, PrintToConsole, false, "dump to console", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableCapsDump, false, "Enable hardware caps dump", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableLivenessDump, false, "Enable dumping out liveness info on stderr.", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, ForceRPE, 0, "Force RPE (RegisterEstimator) computation if > 0. If 2, force RPE per inst.",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, RPEDumpLevel, 0,
                   "> 0 : dump info of register pressure estimate on stderr. See igc_flags.hpp level defs.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DumpVariableAlias, false, "Dump variable alias info, valid if EnableVariableAlias is on",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DumpResourceLoop, false, "dump resource loop detected by ResourceLoopAnalysis", ALWAYS)
DECLARE_IGC_REGKEY(bool, DumpDeSSA, false, "dump DeSSA info into file.", ALWAYS)
DECLARE_IGC_REGKEY(bool, DumpWIA, false, "dump WI (uniform) infomation into files in dump directory if set to true",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableScalarizerDebugLog, false, "print step by step scalarizer debug info.", ALWAYS)
DECLARE_IGC_REGKEY(bool, DumpTimeStats, false, "Timing of translation, code generation, finalizer, etc", ALWAYS)
DECLARE_IGC_REGKEY(bool, DumpTimeStatsCoarse, false,
                   "Only collect/dump coarse level time stats, i.e. skip opt detail timer for now", ALWAYS)
DECLARE_IGC_REGKEY(bool, DumpTimeStatsPerPass, false, "Collect Timing of IGC/LLVM passes", ALWAYS)
DECLARE_IGC_REGKEY(bool, DumpHasNonKernelArgLdSt, false, "Print if hasNonKernelArg load/store to stderr", ALWAYS)
DECLARE_IGC_REGKEY(bool, PrintPsoDdiHash, true, "Print psoDDIHash in TimeStats_Shaders.csv file", ALWAYS)
DECLARE_IGC_REGKEY(bool, ShaderDataBaseStats, false, "Enable gathering sends' sizes for shader statistics", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ShaderSendInfoRework, false, "Temporary Regkey for reworking sendinfo", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DumpLoopSink, false, "Dump debug info in LoopSink", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, LoopSinkDumpLevel, 1, "1, 2 or 3: Dump loop sink with the needed verbosity", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    debugString, ShaderDataBaseStatsFilePath, 0,
    "Path to a file with dumped shader stats additional data e.g. data available during compilation only", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableRemarks, false, "Enable remark for Divergent Barrier", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, AddExtraIntfInfo, false,
                   "Will add extra inteference info from .extraintf files from c:\\Intel\\IGC\\ShaderOverride",
                   DEBUG_ONLY)

DECLARE_IGC_GROUP("Debugging features")
DECLARE_IGC_REGKEY(debugString, ForceSpillVariables, 0,
                   "comma-separated string, each provide the declare id of variable which will be spilled", ALWAYS)
DECLARE_IGC_REGKEY(debugString, ForceAssignRhysicalReg, 0, "Force assigning dclId to phyiscal reg.", ALWAYS)
DECLARE_IGC_REGKEY(bool, InitializeUndefValueEnable, false,
                   "Setting this to 1/true initializes all undefs in URB payload to 0", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, InitializeRegistersEnable, false,
    "Setting this to 1/true initializes all GRFs, Flag and address registers to 0 at the beginning of the shader",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, InitializeAddressRegistersBeforeUse, false,
                   "Setting this to 1 (true) initializes address register to 0 before each use", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DebugSoftwareNeedsA0Reset, false, "Debug softwareNeedsA0Reset flag in AddrAdd testing",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, AvoidUsingR0R1, false, "Do not use r0 and r1 as generic usage registers", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableRelocations, false,
                   "Setting this to 1 (true) makes IGC emit relocatable ELF with debug info", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableWriteOldFPToStack, true,
                   "Setting this to 1 (true) writes the caller frame's frame-pointer to the start of callee's frame on "
                   "stack, to support stack walk",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ZeBinCompatibleDebugging, true,
                   "Setting this to 1 (true) enables embed debug info in zeBinary", ALWAYS)
DECLARE_IGC_REGKEY(bool, DebugInfoEnforceAmd64EM, false,
                   "Enforces elf file with the debug infomation to have eMachine set to AMD64", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DebugInfoValidation, false,
                   "Enable optional (strict) checks to detect debug information inconsistencies", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DumpDbgVarStorageInfo, false,
                   "Dump StorageOffset/StorageStride/IsStackBased entries from the debug-variable storage map",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, deadLoopForFloatException, false, "enable a dead loop if float exception happened", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableIEEEFloatExceptionTrap, false, "Enable CR0 IEEE float exception trap bit", ALWAYS)
DECLARE_IGC_REGKEY(debugString, ExtraOCLOptions, 0, "Extra options for OpenCL", ALWAYS)
DECLARE_IGC_REGKEY(debugString, ExtraOCLInternalOptions, 0, "Extra internal options for OpenCL", ALWAYS)
DECLARE_IGC_REGKEY(debugString, LibClangOverride, 0,
                   "Override opencl-clang library loaded by FCL. Accepts bare name, absolute path, or igc-clang for "
                   "the OS specific igc-clang prebuild name. Empty = use default.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, UseVISAVarNames, false,
                   "Make VISA generate names for virtual variables so they match with dbg file", ALWAYS)
DECLARE_IGC_REGKEY(bool, PrintDebugSettings, false, "Prints all non-default debug settings", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, UseMTInLLD, false, "Use multi-threading when linking multiple elf files", ALWAYS)
DECLARE_IGC_REGKEY(bool, NoCatchAllDebugLine, false,
                   "Don't emit special placeholder instruction to map VISA orphan instructions", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableTestSplitI64, false,
                   "Test legalization that split i64 store unnecessarily, to be deleted once test is done[temp]",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, ShaderDumpTranslationOnly, false,
                   "Dump LLVM IR right after translation from SPIRV to stderr and ignore all passes", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, UseVMaskPredicate, false, "Use VMask as predicate for subspan usage", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, UseVMaskPredicateForLoads, true, "Use VMask as predicate for subspan usage (loads only)",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, UseVMaskPredicateForIndirectMove, true,
                   "Use VMask as predicate for subspan usage (indirect mov only)", ALWAYS)
DECLARE_IGC_REGKEY(bool, StackOverflowDetection, false,
                   "Inserts checks for stack overflow when stack calls or VLAs are used. See documentation: "
                   "documentation/igc/StackOverflowDetection/StackOverflowDetection.md",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, BufferBoundsChecking, false, "Setting this to 1 (true) enables buffer bounds checking", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, MinimumValidAddress, 0,
                   "If it's greater than 0, it enables minimal valid address checking where the threshold is the given "
                   "value (in hex).",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, AssignZeroToUndefPhiNodes, false,
                   "Assigns a null value to such a phi node which has an undefined value during emitting vISA",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY_ENUM(InjectPrintfFlag, 0, "Inject printf debugging flag", INJECT_PRINTF_OPTIONS, ALWAYS)

DECLARE_IGC_GROUP("IGC Features")
DECLARE_IGC_REGKEY(bool, EnableOCLSIMD16, true, "Enable OCL SIMD16 mode", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableOCLSIMD32, true, "Enable OCL SIMD32 mode", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, ForceOCLSIMDWidth, 0,
                   "Force using SIMD width specified. 0 : no forcing. This overrides driver forced SIMD value(if any) "
                   "and runtime behaviour could be different if driver expects something fixed",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, OCLSIMD16SelectionMask, 6, "Select SIMD 16 heuristics. Valid values are 0, 1, 2 and 3",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableGPGPUIndirectPayload, false, "Disable OCL indirect GPGPU payload", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableMemOpt, false, "Disable MemOpt, merging load/store", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, MemOptGEPCanon, 2,
                   "[test] GEP canonicalization in MemOpt. 0 : enable; 1: disable; 2: disable only for OCL;", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableMemOpt2, false, "Disable MemOpt2", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, EnableExplicitCopyForByVal, true,
    "Enable generating an explicit copy (alloca + memcpy) in a caller for aggregate argumentes with byval attribute",
    ALWAYS)
DECLARE_IGC_REGKEY(DWORD, EnableLdStCombine, 1,
                   "Enable load/store combine pass if set to 1 (lsc message only) or 2; bit 3 = 1 [tmp for testing] : "
                   "enabled load combine (intend to replace memopt)",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableLdStCombinewithDummyLoad, false,
                   "Adds extra load instruction to increase the size of coalesced load", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, MaxStoreVectorSizeInBytes, 0,
                   "[LdStCombine] the max non-uniform vector size for the coalesced store. 0: compiler choice "
                   "(default, 16(4DW)); others: 4/8/16/32",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, MaxLoadVectorSizeInBytes, 0,
                   "[LdStCombine] the max non-uniform vector size for the coalesced load.  0: compiler choice "
                   "(default, 16(4DW)); others: 4/8/16/32",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableMergeStore, false,
                   "[temp]If EnableLdStCombine is on, disable mergestore (memopt) if this is set. Temp key for testing",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, MaxLiveOutThreshold, 0, "Max LiveOut Threshold in MemOpt2", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableScalarAtomics, false, "Disable the Scalar Atomics optimization", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableScalarTypedAtomics, true, "Enable the Scalar Typed Atomics optimization", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnablePromotePhiToSourceWidth, true,
                   "Promote a constant-guarded merge PHI to its narrowing cast's source (accumulator) "
                   "width to avoid cross-width register interference",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableScalarPhisMerger, true,
                   "enable optimization that merges scalar phi nodes into vector ones", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableVectorizer, true, "Enable IGCVectorizer pass", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerInsertElAsSeed, true, "IGCVectorizer treats every insert element as a seed", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, VectorizerDepWindowMultiplier, 8,
                   "Multiplier for the slice size to account for vectorizer dependency check window", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerCheckScalarizer, false, "Add scalariser after vectorizer to check performance",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, VectorizerList, -1, "Vectorize only one seed instruction with the provided number", ALWAYS)
DECLARE_IGC_REGKEY(debugString, VectorizerNameFilter, 0,
                   "Only run IGCVectorizer for functions matching the given regex", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerEnableVirtualSeeds, true, "Enable virtual seed creation", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableVectorEmitter, true, "Enable Vector Emission for a vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowI32, true, "Allow I32 versions of instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowFPTRUNC, true, "Allow FPTRUNC instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowFDIV, true, "Allow FDIV instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowFMUL, true, "Allow FMUL instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowFADD, true, "Allow FADD instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowFSUB, true, "Allow FSUB instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowEXP2, true, "Allow EXP2 instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowMAXNUM, true, "Allow MAXNUM instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowWAVEALL, true, "Allow WAVEALL instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowWAVEALLJoint, true, "Allow WAVEALL instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowWAVEBROADCAST, true, "Allow WAVEBROADCAST instructions inside vectorizer",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowCMP, true, "Allow CMP instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowUniformCMP, true, "Allow CMP instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowSelect, true, "Allow Select instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowUniformSelect, true, "Allow Select instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowSamePredSelect, false,
                   "Allow Select instructions with identical predicate inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowFMADMatching, true,
                   "Allow FADD and FMUL instructions to be matched later in the pattern match pass", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowBITCAST, true, "Allow BITCAST instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowMUL, true, "Allow MUL instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowADD, true, "Allow ADD instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerAllowSUB, true, "Allow SUB instructions inside vectorizer", ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerUniformValueVectorizationEnabled, true,
                   "Vector Emitter emits vectorized instruction for uniform values", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, CoalescerDepWindowSize, 100, "Window size to account for vectorizer dependency check window",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, CoalescerAllowBinary, true, "Allow binary instructions inside coalescer", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableOCLScalarizer, false, "Disable ScalarizeFunction pass in OCL pipeline", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisablePHIScalarization, false, "Disable scalarization of PHINode instructions", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableSelectiveScalarizer, false, "enable selective scalarizer on GPGPU path", ALWAYS)
DECLARE_IGC_REGKEY(bool, HoistPSConstBufferValues, true,
                   "Hoists up down converts for contant buffer accesses, so they an be vectorized more easily.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, allowLICM, true, "Enable LICM in IGC.", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, Decompose2DBlockFuncsMode, 2,
                   "Mode for decomposing 2D block functions in IGC, 1 enables legacy pass (Decompose2DBlockFuncs), "
                   "2 enables new pass with address payloads hoisting functionality before load scheduling, "
                   "3 enables new pass with address payloads hoisting functionality after load scheduling, "
                   "(Decompose2DBlockFuncsWithAddrHoisting), 0 disables both passes",
                   ALWAYS)
DECLARE_IGC_REGKEY(
    bool, allowImmOff2DBlockFuncs, false,
    "Allow compiler to decide to use immediate offsets in 2D block intrinsics in Decompose2DBlockFuncs pass.",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, AllowImmOff2DBlockFuncsAddrHoisting, true,
                   "Allow compiler to decide to use immediate offsets in 2D block intrinsics in "
                   "Decompose2DBlockFuncsWithHoisting pass.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, AllowPrefetchDecomposeWithHoisting, false,
    "Allow compiler to decide to use prefetch in 2D block intrinsics in Decompose2DBlockFuncsWithHoisting pass.",
    ALWAYS)
DECLARE_IGC_REGKEY(DWORD, AllowedSpillRegCount, 0, "Max allowed spill size without recompile", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, CSSpillThreshold2xGRFRetry, 3500, "Spill Threshold for CS to trigger 2xGRFRetry", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, LICMStatThreshold, 70, "LICM stat threshold to avoid retry SIMD16 for CS", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableTypeDemotion, true, "Enable Type Demotion", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnablePreRARematFlag, true, "Enable PreRA Rematerialization of Flag", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableGASResolver, true, "Enable GAS Resolver", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableGASKernelByValArgPtrInference, true,
                   "Infer the global address space for pointers contained by-value kernel args.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableLowerGPCallArg, true, "Enable pass to lower generic pointers in function arguments",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableGenericCastToPtrOpt, true,
                   "Enable simplification of GenericCastToPtrExplicit_ToGlobal calls", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableRecompilation, false, "Disable recompilation, skip retry stage", ALWAYS)
DECLARE_IGC_REGKEY(bool, SampleMultiversioning, false,
                   "Create branches aroung samplers which can be redundant with some values", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableSMRescheduling, false,
                   "Change instruction order to enable extra Sample Multiversioning cases", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableEarlyOutPatterns, false,
                   "Disable optimization trying to create an early out after sampleC messages", DEBUG_ONLY)
DECLARE_IGC_REGKEY_BITMASK(EarlyOutPatternSelectPS, 0xff, "Each bit selects a pattern match to enable/disable.",
                           EARLY_OUT_PS_PATTERNS, DEBUG_ONLY)
DECLARE_IGC_REGKEY_BITMASK(EarlyOutPatternSelectCS, 0xff, "Each bit selects a pattern match to enable/disable.",
                           EARLY_OUT_CS_PATTERNS, DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, OCLEnableReassociate, false, "Enable reassociation", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableOCLScratchPrivateMemory, true,
                   "Enable the use of scratch space for private memory [OCL only]", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableMaxWGSizeCalculation, true, "Enable max work group size calculation [OCL only]", ALWAYS)
DECLARE_IGC_REGKEY(bool, Enable64BitEmulation, false, "Enable 64-bit emulation", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, Enable64BitEmulationOnSelectedPlatform, true, "Enable 64-bit emulation on selected platforms",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, EnableConstIntDivReduction, 0x1,
                   "Enables strength reduction on integer division/remainder with constant divisors/moduli", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, EnableIntDivRemCombine, 0x0,
                   "Given div/rem pairs with same operands merged; replace rem with mul+sub on quotient; 0x3 (set "
                   "bit[1]) forces this on constant power of two divisors as well",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, Force32BitIntDivRemEmu, false,
                   "Force 32-bit Int Div/Rem emulation using fp64, ignored if no native fp64 support", ALWAYS)
DECLARE_IGC_REGKEY(
    bool, Force32BitIntDivRemEmuSP, false,
    "Force 32-bit Int Div/Rem emulation using fp32, ignored if Force32BitIntDivRemEmu is set and actually used", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableMullh, true, "Enable i32 mul in SAO layout", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableWideMulMad, true, "Enable wide (64-bit) mul and mad instructions", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableNativeFP32LocalAtomicAdd, true, "Enable native fp32 local atomic add", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableNativeTanh, true, "Enable native tanh instruction", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableFP64Dpas, false, "Enable fp64 dpas", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableFP4Dpas, true, "Enable fp4 dpas", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableOutOfBoundsBuiltinChecks, true, "Enable extra checks for OOB in builtins", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableNativeSinCos, true, "Enable native sin and cos", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceZeroTileID, false, "Use immediate 0 value as TileID instead of sr0.1 data", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RegenerateTileID, true, "Recalculate TileID from sr0.1 on each use", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RegenerateEngineID, false, "Recalculate EngineID from sr0.1 on each use", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableRecursionOpenCL, true, "Enable recursion with OpenCL user functions", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceDPEmulation, false, "Force double emulation for testing purpose", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, EnableDPEmulation, false,
    "Enforce double precision floating point operations emulation on platforms that do not support it natively", ALWAYS)
DECLARE_IGC_REGKEY(bool, DPEmuNeedI64Emu, true,
                   "Double Emulation needs I64 emulation. Unsetting it to disable I64 Emulation for testing.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceDisableDPToHFConvEmu, false,
                   "Force the compiler to disable an emulation for the conversion from fp64 to fp16 (use a native "
                   "(inaccurate) operations instead - fp64 to fp32 and then fp32 to fp16)",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, SelectiveLoopUnrollForDPEmu, true,
                   "Setting this to 0/false disable selective loop unrolling for DP emu.", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceSPDivEmulation, false, "Force SP Div emulation for testing purpose", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    DWORD, ForceI64DivRemEmu, 0,
    "Forces specific int64 div/rem emulation: 0 = platform default, 1 = int based, 2 = SP based, 3 = DP based",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableEmuFolding, true, "Enable emulation folding optimizations", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableAggresiveEmuFolding, false, "Enable aggressive folding optimizations", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableGen11TwoStackTSG, false, "Enable Two stack TSG gen11 feature", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, Enable16BitLDMCS, true, "Enable 16-bit ld_mcs on supported platforms", ALWAYS)
DECLARE_IGC_REGKEY_ENUM(EnableDualSIMD8, -1,
                        "Enable dual SIMD8 on supported platforms. "
                        "-1 - default behavior (platform default; an AIL may force-disable), "
                        "0 - force disabled, "
                        "1 - force enabled",
                        TRIBOOL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY(bool, RemoveLegacyOCLStatelessPrivateMemoryCases, false,
                   "Remove cases where OCL uses stateless private memory. XeHP and above only! [OCL only]", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnablePostCullPatchFIFOLP, true, "Enable Post-Cull Patch Decoupling FIFO. GEN12LP.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnablePostCullPatchFIFOHP, true, "Enable Post-Cull Patch Decoupling FIFO. XeHP.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableAIParameterCombiningWithLODBias, true,
                   "Enable AI parameter combining With LOD Bias parameter. XeHP", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableShrinkArrayAllocaPass, false, "Disables ShrinkArrayAllocaPass", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableAddRequiredMemoryFencesPass, false, "Disables AddRequiredMemoryFencesPass", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableL3FlushForGlobal, false, "Enable/disable flushing L3 cache for globals", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableSampleBMLODWA, true,
                   "Enable workaround for sample_b messages that use the mlod parameter", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableFallbackToBindless, true, "This key enables fallback to bindless mode on all shaders",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisablePromoteToDirectAS, false, "This key disables the PromoteResourceToDirectAS pass",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableAdvCodeMotion, true, "Enable advanced code motion", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, AdvCodeMotionControl, true, "Control bits to fine-tune advanced code motion", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableAdvRuntimeUnroll, true, "Enable advanced runtime unroll", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, AdvRuntimeUnrollCount, 0, "Advanced runtime unroll count", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableAdvMemOpt, true, "Enable advanced memory optimization", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, UniformMemOpt4OW, false, "increase uniform memory optimization from 2 owords to 4 owords",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableFunctionPointer, true, "Enables support for function pointers and indirect calls",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableSIMDVariantCompilation, false, "Enables compiling kernels in variant SIMD sizes",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceFFIDOverwrite, false, "Force overwriting ffid in sr0.0", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableReadGTPinInput, true,
                   "Enables setting GTPin context flags by reading the input to the compiler adapters", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceStaticToDynamic, false, "Force write of vertex count in GS", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableWaSampleLZ, false, "Disable The Sample Lz workaround and generate Sample LZ",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, OverrideRevIdForWA, 0xff,
                   "Enable this to override the stepping/RevId, default is a0 = 0, b0 = 1, c0 = 2, so on...",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, OverrideDeviceIdForWA, 0, "Enable this to override DeviceId ", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, OverrideProductFamilyForWA, 0,
                   "Enable this to override the product family, get the correct enum from igfxfmid.h", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, EnableImplicitArgAsIntrinsic, true,
    "Use GenISAIntrinsic instructions for supported implicit args instead of passing them as function arguments",
    ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableSamplerSupport, false, "Enables sampler messages generation for PVC.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableLSC, false, "Enables the new dataport encoding for LSC messages.", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceNoLSC, false, "Disables the new dataport encoding for LSC messages.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableMadLoopSlice, true, "Enables slicing of MAD chains in loops and acyclic blocks.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableGEPSimplification, true, "Enable GEP simplification", ALWAYS)
DECLARE_IGC_REGKEY(bool, TestGEPSimplification, false,
                   "[Test] Testing GEP simplification without actually lowering GEP. Used in lit test", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableSystemMemoryCachingInGPUForConstantBuffers, false,
                   "Disables caching system memory in GPU for loads from constant buffers", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableInsertingPairedResourcePointer, true,
                   "Enable to insert a bindless paired resource address into sampler headers in context of sampling "
                   "feedback resources",
                   ALWAYS)
DECLARE_IGC_REGKEY(
    bool, EnablePromotionToSampleMlod, true,
    "Enables promotion of sample and sample_c to sample_mlod and sample_c_mlod instructions when min lod is present",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableCorrectlyRoundedMacros, false,
                   "Tmp flag to disable correcly rounded macros for BMG+. This flag will be removed in the future.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY_ENUM(EnableLscSamplerRouting, -1,
                        "Enables conversion of LD to LD_L instructions. Xe2+"
                        "-1 - Platform default"
                        " 0 - Force enable conversion to LD_L. Disallow loads via LSC"
                        " 1 - Force disable conversion to LD_L. Allow loads via LSC",
                        TRIBOOL_OPTIONS, DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    DWORD, CheckCSSLMLimit, 2,
    "Check SLM or threads limit on compute shader to turn on Enable2xGRF on DG2+"
    "0 - off, 1 - SLM limit heuristic, 2 - platform based heuristic (XE2 - threads limit, others - SLM limit)",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableKernelCostInfo, false, "Enable collecting kernel cost info", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableKernelCostDebug, false, "Enable kernel cost info debuging", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableTileYForExperiments, false, "Enable TileY heuristics for experiments", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableDG2LSCSIMD8WA, true,
                   "Enables WA for DG2 LSC simd8 d32-v8/d64-v3/d64-v4. [temp, should be replaced with WA id",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableScratchMessageD64WA, false, "Enables WA to legalize D64 scratch messages to D32",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, LscImmOffsMatch, 1,
                   "Match address patterns that have an immediate offset for the vISA LSC API"
                   "(0 means off/no matching,"
                   " 1 means on/match for supported platforms (Xe2+) and APIs,"
                   " 2 means force on for all platforms (vISA will emulate the addition if HW lacks support) and APIs,"
                   " 3 is the same as 2 and additionally skip the check if A32 offset is a positive value;"
                   " also see LscImmOffsVisaOpts",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, LscImmOffsVisaOpts, 0x3003E,
                   "This maps to vISA_lscEnableImmOffsFor"
                   "(enables/disables immediate offsets for various address types; "
                   "see that option for semantics)",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableStatefulFolding, false,
                   "Turns off all folding for stateful messages (imm offset, scaling, and surface state idx)",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableStatefulScaleFolding, false, "Enables folding of shl into the scale of a stateful send",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceEnableSurfaceStateSizeReloc, false,
                   "Forces emission of surface state size as a relocation in compute path.", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableLSCForTypedUAV, false,
                   "Forces legacy HDC messages for typed UAV read/write."
                   "Temporary knob for XE2 bringup.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableLSCSIMD32TGMMessages, false,
                   "Forces splitting SIMD32 typed messages into 2xSIMD16."
                   "Only valid on XE2+.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, Enable_Wa1807084924, false, "Enable Wa_1807084924 regardless of the platfrom stepping", ALWAYS)
DECLARE_IGC_REGKEY(bool, Enable_Wa1507979211, false, "Enable Wa_1507979211 regardless of the platfrom stepping", ALWAYS)
DECLARE_IGC_REGKEY(bool, Enable_Wa14010017096, false, "Enable Wa_14010017096 regardless of the platfrom stepping",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, Enable_Wa22010487853, false, "Enable Wa_22010487853 regardless of the platfrom stepping",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, Enable_Wa22010493955, false, "Enable Wa_22010493955 regardless of the platfrom stepping",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnablePartialEmuI64, true, "Enable the partial I64 emulation for PVC-B, Xe2", ALWAYS)
DECLARE_IGC_REGKEY_ENUM(LscLoadCacheControlOverride, 0, "Overrides cache-control options for non-intrinsic LSC loads.",
                        LSC_CACHE_CTRL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY_ENUM(LscStoreCacheControlOverride, 0,
                        "Overrides cache-control options for non-intrinsic LSC stores.", LSC_CACHE_CTRL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY_ENUM(TgmLoadCacheControlOverride, 0,
                        "Overrides cache-control options for non-intrinsic LSC tgm loads.", LSC_CACHE_CTRL_OPTIONS,
                        ALWAYS)
DECLARE_IGC_REGKEY_ENUM(TgmStoreCacheControlOverride, 0,
                        "Overrides cache-control options for non-intrinsic LSC tgm stores.", LSC_CACHE_CTRL_OPTIONS,
                        ALWAYS)
DECLARE_IGC_REGKEY(bool, LscForceSpillNonStackcall, false, "Non-stack call kernels that spill will use LSC on DG2+",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableEmitMoreMoviCases, false,
                   "Enables emitting movi for waveShuffle cases using And to keep index within single register. "
                   "Temporarily kept for legacy tests use. Will be removed later.",
                   ALWAYS)
DECLARE_IGC_REGKEY_ENUM(SupportEmitMoreMoviCases, -1,
                        "Controls the behavior of emitSimdShuffle to emit more movi for waveShuffle cases "
                        "using And to keep index within single register."
                        "-1 - default enabled based on the platform choice"
                        " 0 - disabled"
                        " 1 - force enabled",
                        TRIBOOL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY(bool, ConvergentGradientsOnGenISA, false,
                   "Force-enable the ConvergentGradientsOnGenISA AIL: mark GenISA gradient intrinsics convergent so "
                   "code-motion passes cannot sink them across divergent branches. Useful for testing the workaround "
                   "without UMD AIL detection.",
                   ALWAYS)
DECLARE_IGC_REGKEY_ENUM(ForceRegisterAccessBoundsChecks, -1,
                        "Controls the behavior of RegisterAccessBoundsChecks, the pass that adds runtime bounds-checks "
                        "for vector-indexing instructions."
                        "-1 - default behavior, the pass is enabled based on the API type or AILs"
                        " 0 - force disabled"
                        " 1 - force enabled",
                        TRIBOOL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY(
    bool, EnableGlobalStateBuffer, true,
    "This key allows stack calls to read implicit args from side buffer. It also emits a relocatable add in VISA.",
    ALWAYS)
DECLARE_IGC_REGKEY(
    bool, LateInlineUnmaskedFunc, false,
    "Postpone inlining of Unmasked functions till end of CG to avoid code movement inside/outside of unmasked region",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceFormatConversionDG2Plus, false,
                   "Forces SW image format conversion for R10G10B10A2_UNORM, R11G11B10_FLOAT, R10G10B10A2_UINT image "
                   "formats on DG2+ platforms",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableDivergentBarrierWA, false,
                   "Generate continuation code to handle shaders that places barriers in divergent control flow",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DivergentBarrierUniformLoad, false,
                   "Optimize loads for spill/fill generated by DivergentBarrier with uniform analysis", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForcePrefetchToL1Cache, false, "Forces standard builtin prefetch to use L1 cache", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisablePrefetchToL1Cache, false, "Disable prefetch to L1 cache", ALWAYS)
DECLARE_IGC_REGKEY(bool, PreferSIMD32ForCompute, true, "Prefer SIMD32 for compute kernels", ALWAYS)
DECLARE_IGC_REGKEY(
    DWORD, PreferSIMD32ForComputeSubset, 0x28,
    "Prefer SIMD32 for a subset of compute kernels using bit mask, impacting also PS shaders with Ray Query", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, ForceGeomFFSIMDWidth, 32,
                   "SIMD mode for VS, GS, DS, HS; valid values: 0 = default, 8 = SIMD8, 16 = SIMD16, 32 = SIMD32",
                   ALWAYS)
// Enable SIMD32 pack format of PS.
DECLARE_IGC_REGKEY(bool, EnableSIMD32PackFormat, true, "Enable setting of SIMD32PackFormat control bit", ALWAYS)
// Enable SIMD32 pack format support of PS dual SIMD16, quad SIMD8 and quad SIMD8 dynamic.
DECLARE_IGC_REGKEY(DWORD, ForceGeomGRFModeUp, 0,
                   "Set the GRF mode # higher than the one selected by VRT default. VS, GS, DS, HS only", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, ForceGRFModeUp, 0,
                   "Set the GRF mode # higher than the one selected by VRT default. Pass it to vISA", ALWAYS)
// Disable VISA support for SIMD32 programming model.
DECLARE_IGC_REGKEY(bool, DisableOptimizeSIMD32, false, "Disable vISA_enableOptimizeSIMD32", ALWAYS)
// Enable 2x SIMD16 d32v8 URB Write messages with shuffle as WA for performance experiments.
DECLARE_IGC_REGKEY(bool, ForceXYZworkGroupWalkOrder, true, "Force X/Y/Z WorkGroup walk order", ALWAYS)
DECLARE_IGC_REGKEY(bool, Enable320and448GRFConfigsWithoutSendG, true,
                   "Enable vISA_enable320and448Vrt for 320/448GRF VRT configurations.", ALWAYS)
DECLARE_IGC_REGKEY(
    bool, ValidateSPIRVExtensionSupport, true,
    "When enabled, validate each SPIR-V OpExtension against device support and fail compilation if any are "
    "unsupported.",
    ALWAYS)
DECLARE_IGC_REGKEY(bool, PreservePaddingInAggregateArgumentsPass, true, "Preserve padding in AggregateArguments pass",
                   ALWAYS)

DECLARE_IGC_GROUP("Performance experiments")
DECLARE_IGC_REGKEY(DWORD, ManageableBarriersMode, 0,
                   "Set the ManageableBarriers mode in which should work"
                   "0 - Mix Mode of simple and dynamic ManageableBarriers"
                   "1 - Dynamic Mode Only, it will use SLM to store data related with barrier and use them in gateway "
                   "nbarrier instructions."
                   "2 - Simple Mode Only, it will use constant value in gateway nbarrier instructions (without SLM).",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceNonCoherentStatelessBTI, false,
                   "Enable gneeration of non cache coherent stateless messages", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceSendsSupportOnSKLA0, false, "Allow sends on SKL A0, may be unsafe", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableWaSendSEnableIndirectMsgDesc, false,
                   "Disable a C0 WA WaSendSEnableIndirectMsgDesc, may be unsafe", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableWaDisableSIMD16On3SrcInstr, false,
                   "Disable C0 WA WaDisableSIMD16On3SrcInstr, may be unsafe", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DiableWaSamplerNoMask, false, "Disable WA DiableWaSamplerNoMask", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, ForceDisableSrc0Alpha, false,
    "Force the compiler to skip sending src0 alpha. Only works if we are sure alpha to coverage and alpha test is off",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, FunctionControl, 0,
                   "Control function inlining/subroutine/stackcall. See value defs in igc_flags.hpp.", ALWAYS)
DECLARE_IGC_REGKEY(
    DWORD, SelectiveFunctionControl, 0,
    "Selectively enables FunctionControl for a list of line-separated function names in "
    "file specified by SelectiveFunctionControlFile or 'FunctionDebug.txt' in the IGC output dir, in that order."
    "When set by this flag, the functions in the list will override the default FunctionControl mode."
    "0 - Disable, 1 - Enable and read from SelectiveFunctionControlFile, 2 - Print all callable functions to file"
    "See comments in ProcessFuncAttributes.cpp for how to use this flag.",
    ALWAYS)
DECLARE_IGC_REGKEY(debugString, SelectiveFunctionControlFile, 0,
                   "Set file with path that'll be used by SelectiveFunctionControl", ALWAYS)
DECLARE_IGC_REGKEY(
    bool, EnableStackCallFuncCall, false,
    "If enabled, the default function call mode will be set to stack call. Otherwise, subroutine call is used.", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceStackCallForLargeKernel, false,
                   "When FunctionControl is default, force functions of kernels whose estimated size exceeds the "
                   "large-kernel threshold (KernelTotalSizeThreshold * LargeKernelThresholdMultiplier) to use stack "
                   "calls by default.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableByValStructArgPromotion, true,
                   "If enabled, byval/sret struct arguments are promoted to pass-by-value if possible.", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceInlineStackCallWithImplArg, false,
                   "If enabled, stack calls that uses implicit args will be force inlined.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableFunctionCloningControl, true,
                   "If enabled, limits function cloning by converting stackcalls to indirect calls based on the "
                   "FunctionCloningThreshold value.",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, FunctionCloningThreshold, 0,
                   "Limits the number of cloned functions when called from multiple function groups."
                   "If number of cloned functions exceeds the threshold, compile the function only once and use "
                   "address relocation instead."
                   "Setting this to '0' allows IGC to choose the default threshold.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableFastInstCombineForLargeKernels, false,
                   "If enabled, skip expensive InstCombine after MemOpt on large kernels and run cheaper cleanup "
                   "passes instead.",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, FastInstCombineLargeKernelThreshold, 300000,
                   "Instruction-count threshold to trigger fast InstCombine fallback after MemOpt.", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceLowestSIMDForStackCalls, true,
                   "If enabled, compile to the lowest allowed SIMD mode when stack calls or indirect calls are present",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, OCLInlineThreshold, 512, "Setting OCL inline thershold", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableAddingAlwaysAttribute, false, "Disable adding always attribute", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableInlining, false, "Disable inlining of all functions", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableDropTargetFunctions, false, "Enables pass for dropping targeted functions", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, VerboseDropTargetFunctions, false, "Enables verbose logging for dropping targeted functions",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, CrashOnDroppedFnAccess, false, "Enables crash on access to dropped functions", ALWAYS)
DECLARE_IGC_REGKEY(debugString, DropTargetFnListPath, 0, "Path to folder with lists of functions to drop", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableDropTargetBBs, false, "Enables pass for dropping targeted BBs", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, VerboseDropTargetBBs, false, "Enables verbose logging for dropping targeted BBs", DEBUG_ONLY)
DECLARE_IGC_REGKEY(debugString, DropTargetBBListPath, 0, "Path to folder with lists of BBs to drop", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, PrintFunctionSizeAnalysis, 0, "Print analysis data of function sizes", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, SubroutineThreshold, 110000, "Minimal kernel size to enable subroutines", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, SubroutineInlinerThreshold, 3000, "Subroutine inliner threshold", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableLargeFunctionCallMerging, true,
                   "Merge mutually exclusive calls to large functions to enable inlining", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ControlKernelTotalSize, true, "Control kernel total size", ALWAYS)
DECLARE_IGC_REGKEY(bool, StaticProfileGuidedTrimming, false, "Enable static analysis in the kernel trimming", ALWAYS)
DECLARE_IGC_REGKEY(debugString, SelectiveTrimming, 0, "Choose a specific function to trim", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableGreedyTrimming, false, "Find the optimal set of functions to trim", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableLeafCollapsing, false,
                   "Collapse leaf functions in order to avoid trimming small leaf functions", ALWAYS)
DECLARE_IGC_REGKEY(bool, UseFrequencyInfoForSPGT, true, "Consider frequency information for trimming functions", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableSizeContributionOptimization, false,
                   "Put more weight on a function when the potential size contirubion is big", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, SkipTrimmingOneCopyFunction, 3000,
                   "Don't trim a function whose size contribution is no more than its size", ALWAYS)
DECLARE_IGC_REGKEY(bool, LoopCountAwareTrimming, false,
                   "Take loop count into account in measuring the function size for trimming", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, MaxUnrollCountForFunctionSizeAnalysis, 16,
                   "The maximum number of loop unrolling assumed in function size analaysis", ALWAYS)
DECLARE_IGC_REGKEY(bool, ControlInlineImplicitArgs, true, "Avoid trimming functions with implicit args", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, ControlInlineTinySize, 200, "Tiny function size for controlling kernel total size", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, ControlInlineTinySizeForSPGT, 300, "Tiny function size for controlling kernel total size",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, SizeWeightForSPGT, 3, "Size weight for a trimming threshold", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, FrequencyWeightForSPGT, 2, "Frequency weight for a trimming threshold", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, PrintControlKernelTotalSize, 0, "Print Control kernel total size", ALWAYS)
DECLARE_IGC_REGKEY(bool, AddNoInlineToTrimmedFunctions, false, "Tell late passes not to inline trimmed functions",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceInlineExternalFunctions, false, "not to trim functions called from multiple kernels",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, KernelTotalSizeThreshold, 50000, "Trimming target of kernel total size", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, LargeKernelThresholdMultiplier, 12,
                   "Multipler to kernel threshold. When exceeded more agressive trimming will be performed", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, LargeKernelSmallFunctionLimit, 50,
                   "Size threshold for small function trimming for large kernels", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, TrimImplicitArgFunctionsForLargeKernels, true,
                   "When a kernel is still over threshold after trimming, allow trimming "
                   "functions that use implicit args (overriding their force-inline)",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, PartitionUnit, false, "Partition compilation unit", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, PrintPartitionUnit, 0, "Print information about compilation unit partitioning", ALWAYS)
DECLARE_IGC_REGKEY(bool, PartitionWithFastHybridRA, false, "Enable FastRA and HybridRA when partition is enabled",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, UnitSizeThreshold, 3000, "Compilation unit size threshold", ALWAYS)
DECLARE_IGC_REGKEY(bool, StaticProfileGuidedPartitioning, 0, "Enable static analysis in the partitioning algorithm.",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, PrintStaticProfileGuidedKernelSizeReduction, 0,
                   "Print information about static profile-guided trimming and partitioning", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, MetricForKernelSizeReduction, 2,
                   "Set 1 to active a normal distribution, 2 a long-tail distribution, and 4 an average%", ALWAYS)
DECLARE_IGC_REGKEY(bool, BlockFrequencySampling, true, "Use block frequencies to derive a distribution", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, ParameterForColdFuncThreshold, 80,
                   "C/10-STD for a normal distribution / low K% for a long-tail distribution", ALWAYS)
DECLARE_IGC_REGKEY(bool, ControlUnitSize, false, "Control compilation unit size by unit trimming", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, ExpandedUnitSizeThreshold, 50000, "Trimming target of compilation unit size", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, PrintControlUnitSize, 0, "Print information about unit trimming", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, StaticProfileGuidedSpillCostAnalysis, 8,
                   "Use static profile information to estimate spill cost, "
                   "1 for profile generation, 2 for profile transfer, 4 for profile embedding, "
                   "8 for spill computation, and 16 for enabling frequency-based spill selection ",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, StaticProfileGuidedSpillCostAnalysisScale, 4,
                   "Scale adjustment for static profile guided spill cost analysis", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, StaticProfileGuidedSpillCostAnalysisFunc, 1,
                   "Spill cost function where 0 is based on a new spill cost and 1 the existing one", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, PrintStaticProfileGuidedSpillCostAnalysis, 0, "Print debug messages for profile embedding",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableConstantPromotion, true, "Enable global constant data to register promotion", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, AllowNonLoopConstantPromotion, false,
                   "Allows promotion for constants not in loop (e.g. used once)", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, ConstantPromotionSize, 2, "Threshold in number of GRFs", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, ConstantPromotionCmpSelSize, 4, "Array size threshold for cmp-sel transform", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, FuseResourceLoop, true, "Enable fusing resource loops", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, MaxFuseResourceLoopSize, 2, "Maximum fuse resource loop size", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, MaxMemOpsInFuseResourceLoop, 4,
                   "Maximum number of non-memory ALU instructions allowed between memory operations in each fused "
                   "resource loop",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, FuseResourceLoopMinSize, 3,
                   "Minimum number of sends in a fusion group (groups smaller than this are not fused)", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, ResourceLoopUnrollIteration, 1,
                   "Unroll resource loop iterations (larger than 1): 1 (default) - no sub-iteration", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableResourceLoopUnrollExclusiveLoad, false,
                   "Disable visa ExclusiveLoad for the SBID in Unroll resource loop", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, ResourceLoopUnrollNested, 0,
                   "Unroll resource loop iterations (larger than 0): 0 (default) - no nested loop", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableResourceLoopUnrollNestedLsc, false, "Disable unroll nested for lsc load.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableResourceLoopUnrollNestedSampler, false, "Disable unroll nested for sampler.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableVariableReuse, true, "Enable local variable reuse", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableVariableAlias, true,
                   "Enable variable aliases (part of VariableReuse Pass, but separate functionality)", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, VectorAlias, 1,
                   "Vector aliasing control under EnableVariableAlias. Some features are still experimental", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, VectorAliasBBThreshold, 200,
                   "Max number of BBs of a function that VectorAlias will apply. VectorAlias will skip for funtions "
                   "beyond this threshold",
                   ALWAYS)
DECLARE_IGC_REGKEY(
    DWORD, ScalarAliasBBSizeThreshold, 500,
    "Max size of BB for which scalar aliasing will apply. Scalar aliasing will skip for BBs beyond this threshold",
    ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableExtractMask, false,
                   "When enabled, it is mostly for reducing response size of send messages.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, VariableReuseByteSize, 64, "The byte size threshold for variable reuse", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableSampleTailDeAlias, true,
                   "When a sample-ld return component escapes the sample's basic block (a long-lived tail) while "
                   "a sibling component dies inside the block, keep the tail in its own variable (do not "
                   "payload-coalesce it) so the payload declare dies early and its dead sibling GRFs are reclaimed.",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, SampleTailDeAliasRPThreshold, 100,
                   "Minimum register pressure as a percentage of the GRF file size to enable "
                   "sample tail de-aliasing. 0 disables the pressure gate (always fire when the "
                   "flag is enabled). Default 100 means fire only when maxRegPressure -GE- 100 percent of GRFs.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, SampleTailDeAliasSuppressAtPeakBlock, true,
                   "Peak-aware gate for sample tail de-aliasing. When enabled, suppress de-aliasing a sample/ld "
                   "tail whose def block is the function's highest register-pressure basic block, since the "
                   "de-alias copy would add its footprint on top of the still-live payload at the peak (a strict "
                   "loss). Disable to fire regardless of where the peak is.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, SampleTailDeAliasSuppressNonUniform, true,
                   "Resource-loop gate for sample tail de-aliasing. When enabled, suppress de-aliasing a sample/ld "
                   "tail whose resource (or, for a sample, sampler) is non-uniform: such sends are emitted wrapped "
                   "in a resource loop that keeps the whole response payload loop-carried live, so the de-alias copy "
                   "frees nothing and is pure additive pressure. Narrower than a loop-membership guard (targets loop "
                   "generators, not samples merely fused into a neighbor's loop). Disable to fire regardless.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableGather4cpoWA, true, "Enable WA transforming gather4cpo/gather4po into gather4c/gather4",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableIntelFast, false, "Enable intel fast, experimental flag.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, forceGlobalRA, false, "force global register allocator", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, disableVarSplit, false, "disable variable splitting", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, delayVarSplit, false, "delay local variable splitting", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, disableRemat, false, "disable re-materialization", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableDisableMidThreadPreemptionOpt, true, "Disable mid thread preemption", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, MidThreadPreemptionDisableThreshold, 600, "Threshold to disable mid thread preemption",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, forceSamplerHeader, false, "force sampler messages to use header", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, samplerHeaderWA, false, "enable sampler header to solve HW WA", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, cl_khr_srgb_image_writes, false, "Enable cl_khr_srgb_image_writes extension", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, MSAA16BitPayloadEnable, true,
                   "Enable support for MSAA 16 bit payload , a hardware DCN supporting this from ICL+ to improve perf "
                   "on MSAA workloads",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableInsertElementScalarCoalescing, false,
                   "Enable coalescing on the scalar operand of insertelement", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableMixIntOperands, true, "Enable generating mix-sized operands for int ALU", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, PixelShaderDoNotAbortOnSpill, false, "Do not abort on a spill", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, ForceScratchSpaceSize, 0, "Override Scratch Space Size in bytes for perf testing", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, SkipPaddingScratchSpaceSize, 4096,
                   "Skip adding padding when estimated scratch space size is smaller than or equal to this value",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    DWORD, ForcePixelShaderSIMDMode, 0,
    "Setting it to values def in igc.h will force SIMD mode compilation for pixel shaders. Note that only SIMD8 is "
    "compiled unless other ForcePixelShaderSIMD* are also selected. 1-SIMD8, 2-SIMD16,4-SIMD32",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableDynamicPolyPackingPolicies, true,
                   "Disable dynamic poly packing policies for Xe3+ platforms", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableUnifiedCoarseAndPixelDispatchRates, false,
                   "Enable unification of coarse and pixel dispatch rates on Xe3+ (HSD-14015289391): "
                   "consume the HW source-depth (PosZPixel) payload instead of the legacy manual "
                   "source-depth interpolation.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RequestStage2, true, "Enable staged compilation via requesting stage 2", DEBUG_ONLY)

DECLARE_IGC_REGKEY(bool, ExtraRetrySIMD16, false, "Enable extra simd16 with retry for STAGE1_BEST_PREF", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, SSOShifter, 9,
                   "Adjust ScratchSurfaceOffset with shl(hwtid, shifter). 0 menas disabling padding", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, DelayEmuInt64AddLimit, 0, "Delay emulating Int64 Add operations in vISA", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, CodePatch, 2, "Enable Pixel Shader code patching to directly emit code after stitching",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, CodePatchLimit, 0, "Debug CodePatch via limiting the number of shader been patched",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, CodePatchExperiments, 0, "Experiment with code patching when != 0", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, CodePatchFilter, 0x7, "Filter out unsupported patterns", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    DWORD, FirstStagedSIMD, 0,
    "Force Pixel shader to be 1: FastSIMD (SIMD8), 2: BestSIMD (SIMD16 or SIMD8), 3: FatestSIMD (SIMD8 opt off)",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, FastestS1Experiments, 0, "Select configs for fastest compilation by bits.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, ForceAddingStackcallKernelPrerequisites, false,
    "Force adding static overhead for stackcall to the kernel entry such as HWTID instructions for experiments", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableFastestLinearScan, false, "Disable LinearScanRA in FastestSIMD.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableFastestGopt, false, "Disable global optimizations for stage 1 shaders.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceFastestSIMD, false,
                   "Force PS, CS, VS to return lowest possible SIMD as fast as possible.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceBestSIMD, false, "Force pixel shader to return the best SIMD, either SIMD16 or SIMD8.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, EnableTCSHWBarriers, false,
    "Enable TCS pass with HW barriers support. Default TCS pass is TCS pass with multiple continuation functions.",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, ForceMCFBarriers, false,
    "Force TCS pass with MCF (SW) barriers support. Default TCS pass is TCS pass with multiple continuation functions.",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableAccSub, true, "Enable accumulator substitution", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnablePreRAAccSchedAndSub, false, "Enable accumulator substitution", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    DWORD, NumGeneralAcc, 0,
    "set the number [1-8] of general acc for accumulator substitution. 0 means using the platform-default value",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, HasDoubleAcc, false, "has doubled accumulators", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceSWCoalescingOfAtomicCounter, false, "Force software coalescing of atomic counter",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceMixMode, false, "force enable mix mode even on platforms that do not support it",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableFDIV, false, "Disable fdiv support", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EmulateFDIV, false, "Emulate fdiv instructions", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, UpConvertF16Sampler, true, "up-convert fp16 sampler message to return fp32", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DownConvertI32Sampler, false, "Convert i32 sampler messages to return i16.\
    This optimization can only be enabled for resources with 16bit integer format\
    or if it is known that the upper 16bits of data is always 0.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, FuseTypedWrite, false, "Enable fusing of simd8 typed write", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, EnableHalfPromotion, true,
    "Enable pass that replaces instructions using halfs with corresponding float counterparts for pre-SKL", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceHalfPromotion, false,
                   "Force enable pass that replaces instructions using halfs with corresponding float counterparts",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, ForceNoInfiniteLoops, false,
    "Limit # of loop iterations to UINT_MAX in while/for loops. Can be used to detect infinite loops in shaders",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, DisbleLocalFences, false,
    "On CNL+ we need to emit local fences. Setting this to true removes those. It may be functionaly not correct.",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, FastSpill, false,
                   "fast spill code gen. This may produce worse equality code for the spilling shader", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, EmulationFunctionControl, 0,
                   "FunctionControl on some DP emulation functions. It has the same value as FunctionControl.", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, InlinedEmulationThreshold, 125000, "Inlined instruction threshold for enabling subroutines",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(int, ByPassAllocaSizeHeuristic, 0,
                   "Force some Alloca to pass the pressure heuristic until the given size", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, MemOptWindowSize, 150,
                   "Size of the window in unit of instructions in which load/stores are allowed to be coalesced. Keep "
                   "it limited in order to avoid creating long liveranges. Default value is 150",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, RematFlowThreshold, 10,
                   "Proportion of the whole rematerialization targets to cutoff remat chain", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, RematChainLimit, 12,
                   "If number of instructions we've collected is more than this value, we bail on it", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    DWORD, RematRPELimit, 93,
    "Cutoff value for register estimator, lower than that, kernel won't be rematted, stated in percentages", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RematSingleFlowRematEnabled, true,
                   "Allow singleFlowRemat stage inside CloneAddressArithmeticPass", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RematEnable, false, "Enable clone address arithmetic pass not only on retry", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RematLog, false, "Dump Remat Log, useful for analyzing spills as well", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RematLogToErr, false, "Dump Remat Log, useful for analyzing spills as well", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, RematSameBBScope, false,
    "Confine rematerialization only to variables within the same BB, we won't pull down values from predeccors",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RematRespectUniformity, false, "Cutoff computation chain on uniform values", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RematAllowExtractElement, true, "Allow Extract Element to computation chain", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RematDataAllowCMP, true, "Allow rematerialization of cmp instructions", ALWAYS)
DECLARE_IGC_REGKEY(bool, RematReassocBefore, false,
                   "Enable short sequence of passes before clone address arithmetic pass to potentially decrese amount "
                   "of operations that will be rematerialized",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RematInstCombineBefore, false,
                   "Enable short sequence of passes before clone address arithmetic pass to potentially decrese amount "
                   "of operations that will be rematerialized",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RematAddrSpaceCastToUse, true,
                   "Allow rematerialization of inttoptr that are used inside AddrSpaceCastInst", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RematCallsOperand, true, "Allow rematerialization of inttoptr that are used as call's operand",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RematCollectCallArgs, true, "Allow collection of call arguments for rematerialization",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RematAllowOneUseLoad, false,
                   "Remat allow to move loads that have one use and it's inside the chain", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RematAllowLoads, false,
                   "Remat allow to move loads, no checks, exclusively for testing purposes", DEBUG_ONLY)
DECLARE_IGC_REGKEY_BITMASK(RematOptionsForRetry, 0,
                           "Options for CloneAddressArithmetic pass when recompiling shader. Valid for non-OpenCL only",
                           REMAT_MASK, DEBUG_ONLY)
DECLARE_IGC_REGKEY_BITMASK(RematOptionsForVRT, 0,
                           "Options for CloneAddressArithmetic pass when compiling shader. Valid for non-OpenCL only",
                           REMAT_MASK, DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DumpRegPressureEstimate, false, "Dump RegPressureEstimate to a file", DEBUG_ONLY)
DECLARE_IGC_REGKEY(debugString, DumpRegPressureEstimateFilter, 0,
                   "Only dump RegPressureEstimate for functions matching the given regex", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, AddressSpacePhiPropagation, true,
                   "Lower loads from PHI nodes into incoming nodes in case they cause extra address space casts.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, VectorizerLog, false, "Dump Vectorizer Log, usefull for analyzing vectorization issues",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, VectorizerLogToErr, false, "Dump Vectorizer Log to stdErr", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableReusingXYZWStoreConstPayload, true, "Enable reusing XYZW stores const payload",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableReusingLSCStoreConstPayload, false, "Enable reusing LSC stores const payload",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, AllowSIMD16DropForXE2Plus, true,
                   "Controls the switch for XE2 and XE3 simd16 drop, including the early RPE-based drop", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, EarlySIMD16DropForXE3Threshold, 256, "Threshold for the early drop to simd16 for XE3",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, OCLVRTSimd16DropSimd32High, 160,
                   "Drop SIMD32 to SIMD16 on VRT platforms when SIMD32 RPE exceeds this value", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, OCLVRTSimd16DropSimd16Low, 120, "...and SIMD16 RPE is below this value", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, RegPressureVerbocity, 2, "Different printing types", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, RetryRevertExcessiveSpillingKernelThreshold, 10000,
                   "Sets the threshold for Retry Manager to know which kernel is considered as Excessive Spilling and "
                   "applies different set of rules",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    DWORD, RetryRevertExcessiveSpillingKernelCoefficient, 102,
    "Sets the coefficient for Retry Manager to know whether we should revert back to a previously compiled kernel",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    DWORD, ForceSIMDRPELimit, 1000,
    "Cutoff value for register estimator, when higher than that kernel is switched to lower SIMD when possible",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, EarlyRetryLargeGRFThreshold, 500,
                   "Cutoff value for register estimation, when highter than that kernel skips first compilation stage "
                   "and goes to retry immediately for large GRF.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableLateRPRepublish, false,
                   "Experimental flag: republish metadata for early retry in the end of pipeline, "
                   "instead of reusing the existing if the existing shows that early recompilation is needed",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, EarlyRetryDefaultGRFThreshold, 190,
                   "Cutoff value for register estimation, when highter than that kernel skips first compilation stage "
                   "and goes to retry immediately for default GRF.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableOCL512GRFForDPAS, false,
                   "On OCL recompilation, lift the GRF ceiling to 512 for SIMD16 (not forced-SIMD32) DPAS "
                   "kernels",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableOCL512GRFForSIMD16, false,
                   "On OCL recompilation, lift the GRF ceiling to 512 for SIMD16 kernels: required/forced "
                   "sub-group size 16, or high register pressure that drops to SIMD16",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableCRIDefault512GRF, true, "Raise the default VRT GRF ceiling to 512 on CRI", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceNoFP64bRegioning, false, "force regioning rules for FP and 64b FPU instructions",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableA64WA, true, "Guarantee A64 load/store addres-hi is uniform", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableSamplerSplit, false, "Split Sampler 3d message to odd and even", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableEvaluateSamplerSplit, true,
                   "Split evaluate messages to sampler into either SIMD8 or SIMD1 messages", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, AllocaRAPressureThreshold, 500, "The threshold for the register pressure potential",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, AllocaSinkingOptNoneAllowance, 205,
                   "Amount of how much allowance is given for alloca sinking in case of optnone", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, HPCInstNumThreshold, 1000000, "The threshold for the register pressure potential", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, HPCGlobalInstNumThreshold, 500000, "The threshold for the register pressure potential",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, HPCFastCompilation, false, "Force to do fast compilation for HPC kernel", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableFastRAWA, true, "Disable Fast RA for hanging issues on large workloads", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, FastCompileRA, false, "Provide the fast compilatoin path for RA, fail safe at first iteration",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, HybridRAWithSpill, false, "Did Hybrid RA with Spill", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, SelectiveFastRA, false, "Apply fast RA with spills selectively using heuristics", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, RetryStackCallSpillCostThreshold, 5,
                   "Only retry if the percentage of spills (over total instructions) is more than this value",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, AllowStackCallRetry, 2,
                   "Enable/Disable retry when stack function spill. 0 - Don't allow, 1 - Allow retry on kernel group, "
                   "2 - Allow retry per function",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, PrintStackCallDebugInfo, false,
                   "Print all debug info to command line related to stack call debugging", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, StripDebugInfo, 0,
                   "Strip debug info from llvm IR lowered from input to IGC ."
                   "Possible values: 0 - dont strip, 1 - strip all, 2 - strip non-line info",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableGPUFenceScopeOnSingleTileGPUs, false,
                   "Allow the use of `GPU` fence scope on single-tile GPUs. By default the `TILE` scope is used "
                   "instead of `GPU` scope on single-tile GPUs.",
                   ALWAYS)
DECLARE_IGC_REGKEY(int, JointMatrixLoadStoreOpt, 3,
                   "Selects subgroup (0), or block read/write (1), or optimized block read/write (2), 2d block "
                   "read/write (3) implementation of Joint Matrix Load/Store built-ins",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableVector8LoadStore, false,
                   "Enable Vectorizer to generate 8x32i and 4x64i loads and stores", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableOpaquePointersBackend, false,
                   "[Experimental] Force opaque pointers' usage within IGC/LLVM passes", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ExcludeIRFromZEBinary, false, "Exclude IR sections from ZE binary", ALWAYS)
DECLARE_IGC_REGKEY(bool, AllocateZeroInitializedVarsInBss, true,
                   "Allocate zero initialized global variables in .bss section in ZEBinary", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, OverrideOCLMaxParamSize, 0,
                   "Override the value imposed on the kernel by CL_DEVICE_MAX_PARAMETER_SIZE. Value in bytes, if "
                   "value==0 no override happens.",
                   ALWAYS)

DECLARE_IGC_REGKEY(bool, EnableOptReportPrivateMemoryToSLM, false,
                   "[POC] Generate opt report file for moving private memory allocations to SLM.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceAllPrivateMemoryToSLM, false, "[POC] Force moving all private memory allocations to SLM.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(debugString, ForcePrivateMemoryToSLMOnBuffers, 0,
                   "[POC] Force moving private memory allocations to SLM, semicolon-separated list of buffers.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForcePrivateMemoryToGlobalOnGeneric, true,
                   "Force moving private memory allocations to global buffer when generic pointer is present", ALWAYS)
DECLARE_IGC_REGKEY(
    bool, DetectCastToGAS, true,
    "Check if the module contains local/private to GAS (Gerneric Address Space) cast, it also check internal flags",
    ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableProgrammableOffsetsMessageBitInHeader, false,
                   "Use pre-delta feature (legacy) method of passing MSB of PO messages opcode. ", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableEfficient64b, false,
                   "Enable efficient64b feature such as new inline data and new send messages and descriptor formats, "
                   "valid for xe3p+.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableSkipUnusedColorPayload, true,
                   "Enables skipping unused color phases of render target write.", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableResourceLoopNonUniformCmpLowerHalfDWOnly, true,
                   "Only compare the lower half of 64-bit resource address in the resource loop. This is to assume the "
                   "number of the resource in the heap will never exceed 2^32 limitation.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableSWManagedStack, false,
                   "Disables SW managed stack for RayQuery, the compiler will use legacy stack size and stackID "
                   "calculation, valid for xe3p+.",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, SWManagedStackNumStacks, 0,
                   "Forces the number of syncRT stacks per DSS. If explicitly set to 0, 2048 is assumed to maintain "
                   "backward compatibility.",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, EnableScalarPipe, 0,
                   "for scalar-pipe experiment, N specifies the number of scalar registers in Nx16 dwords", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableEngineID, false, "Disables usage of engine ID from ARF", ALWAYS)
DECLARE_IGC_REGKEY(bool, Enable32bSampler, true, "Enables 32b samplers", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, OverrideCsWalkOrderEnable, false, "Enable overriding compute walker walk order", ALWAYS)
DECLARE_IGC_REGKEY(int, OverrideCsWalkOrder, 0, "Override compute walker walk order", ALWAYS)
DECLARE_IGC_REGKEY(bool, OverrideCsTileLayoutEnable, false, "Enable overriding compute walker tile layout", ALWAYS)
DECLARE_IGC_REGKEY(int, OverrideCsTileLayout, 0, "Override compute walker tile layout enum class ThreadIDLayout",
                   ALWAYS)
DECLARE_IGC_REGKEY_ENUM(OverrideHWGenerateLID, -1,
                        "Override HW Generate Local ID setting"
                        "-1 - default behavior,"
                        " 0 - force disabled,"
                        " 1 - force enabled",
                        TRIBOOL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY(DWORD, MemCpyLoweringUnrollThreshold, 12,
                   "Min number of mem instructions that require non-unrolled loop when lowering memcpy", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, EnablePrivMemNewSOATranspose, 1,
                   "0 : disable new algo; 1 and up : enable new algo. "
                   "1 : enable new algo for structs and scalar (float/int) arrays; "
                   "2 : 1 plus new algo for array of dw[xn]/qw[xn],etc "
                   "3 : 2 plus new algo for array of complicated struct.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableSOAFallbackToOldAlgorithm, false,
                   "Enable fallback to old SOA algorithm when new algorithm is not applicable", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnablePrivMemNewSOAForScalarArrays, false,
                   "Enables new SOA algorithm also for scalar float/int arrays.", ALWAYS)
DECLARE_IGC_REGKEY(bool, NewSOATransposeForOpenCL, true,
                   "If true, EnablePrivMemNewSOATranspose only applies to OpenCL kernels. For testing purpose", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableSelectOfAllocaPtrSplit, false,
                   "If true, enables splitting SELECT instruction containing pointers "
                   "where one operand is alloca-derived (load duplication / store branching). "
                   "Enables SoA promotion for allocas otherwise blocked by SELECT pattern.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisablePredicatedLoadForAllocaPtrSelectSplit, false,
                   "If true, EnableSelectOfAllocaPtrSplit always emits regular loads, even when "
                   "private memory is in stateless global, instead of the predicated-loads. "
                   "For testing/debugging. May cause OOB reads in stateless global.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnablePHIOfAllocaPtrSplit, false,
                   "If true, enables splitting PHI instruction containing pointers "
                   "where at least one incoming value is alloca-derived (per-predecessor load + value phi). "
                   "Enables SoA promotion for allocas otherwise blocked by PHI pattern.",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, PHIOfAllocaPtrSplitMinSize, 64,
                   "Minimum alloca size in bytes to be considered worthwhile for the "
                   "EnablePHIOfAllocaPtrSplit pass.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableSOAPromotionDisablingHeuristic, false,
                   "Enable heuristic to disable SOA promotion when it may be not beneficial", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableSOAPromotion, false,
                   "If true, SOA cannot be used (private memory transposition). For testing purpose", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableFastMathConstantHandling, false, "Disable Fast Math Constant Handling", ALWAYS)
DECLARE_IGC_REGKEY_ENUM(SupportUniformPrivateMemorySpace, -1,
                        "Controls the behavior of PrivateMemoryResolution to emit uniform private memory allocas to "
                        "reduce the memory consumption."
                        "-1 - default behavior, the pass is enabled based on the API type or AILs"
                        " 0 - force disabled"
                        " 1 - force enabled",
                        TRIBOOL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY_ENUM(ShortImplicitPayloadHeader, -1,
                        "Controls the behavior of implicit kernel argument 'payloadHeader'."
                        "-1 - platform default"
                        " 0 - force old 8xi32 payloadHeader"
                        " 1 - force 3xi32 payloadHeader (global_id_offset only)",
                        TRIBOOL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY_ENUM(RemoveUnusedIdImplicitArguments, -1,
                        "Remove implicit arguments: global_id_offset (payloadHeader) and/or enqueued_local_size if "
                        "unused. Useful if kernel doesn't use global id."
                        "-1 - platform default"
                        " 0 - force disabled"
                        " 1 - force enabled",
                        TRIBOOL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY_ENUM(RemoveUnusedIdImplicitLocalIDs, -1,
                        "Remove implicit arguments localIDs if unused."
                        "-1 - platform default"
                        " 0 - force disabled"
                        " 1 - force enabled",
                        TRIBOOL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY(bool, RemoveImplicitScratchPointer, true,
                   "Allows skipping scratch pointer implicit kernel argument if unused. If false, arg is always added.",
                   ALWAYS)
DECLARE_IGC_REGKEY(int, RemoveImplicitScratchPointerInstThreshold, 2000,
                   "Maximum number of instructions in kernel for which scratch pointer is considered for removal.",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, ForceVRTGRFCeiling, 0,
                   "Override to set maximum GRF of VRT ceiling number for vISA. "
                   "The value can be from { 256, 320, 448, 512 }",
                   ALWAYS)

DECLARE_IGC_REGKEY(bool, AllowCrossBlockMatchMad, false,
                   "Enable cross basic block matching of mad instructions. This may lead to increased register "
                   "pressure, but in exchange, may reduce instruction count",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, AllowMultipleMulUsesMatchMad, false,
    "Enable a multiply instruction with multiple uses to be matched to a mad instruction. This essentially forces the "
    "recalculation of the intermediate multiply result for every potential mad instruction, which will have "
    "performance impacts but may reduce instruction count and register pressure in case both mul operands need to be "
    "live past the add/sub but the intermediate mul result does not.",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, AllowConstMadOpMovToReg, false,
                   "Enable matching of mad instruction if constant greater than 16-bits. This will generate a mov in "
                   "vISA for the constant operand due to it not fitting as an imm16 operand. At this point, the "
                   "generated asm likely will fall back onto mul+add for the main case where src1 is the constant",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, AllowNonMulMemOpMadChainReassoc, false,
    "Enable reassociation in fmul+fadd chain to pull last fadd operand to the front even when it is a result of a "
    "memory operation. This will match one more mad instruction for the group at the expense of shortening the "
    "dependency distance from a mem load of the fadd operand to the first fmul+fadd in the chain.",
    DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, MadChainReassocMaxDepth, 3, "Maximum depth of fmul+fadd tree to do reassociation for",
                   DEBUG_ONLY)
DECLARE_IGC_GROUP("Generating precompiled headers")
DECLARE_IGC_REGKEY(bool, ApplyConservativeRastWAHeader, true,
                   "Apply WaConservativeRasterization for the platforms enabled", DEBUG_ONLY)

DECLARE_IGC_GROUP("Raytracing Options")
DECLARE_IGC_REGKEY(bool, DisableEntryFences, false, "Don't emit the evict and invalidate fences for A0 WA", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableRayTracingTGMFence, false, "Enable tgm fence in RT workloads for debugging", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, RayTracingDumpYaml, false, "Dump yaml input/output files", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableCompressedRayIndices, false,
                   "Use an alternate form with bit twiddling to pack stack pointer and indices into two DWORDs", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceNullBVH, false, "Swap BVH with null pointer. Infinitely fast ray traversal.", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableFuseContinuations, false,
                   "If set, we will look for small duplicated continuations to merge into one.", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableMatchRegisterRegion, false, "Disable matching for debug purposes", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableEarlyRemat, false, "Disable quick remats to avoid some spills", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableLateRemat, false, "Disable quick remats to avoid some spills", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, RematThreshold, 6, "Tunes how aggresively we should remat values into continuations", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, ConstantCoalescingMaxBBDepthDelta, 0,
                   "How many basic block levels the merged chunk is allowed to stretch across, avoiding wide-load "
                   "coalescing. 0 disables the check. Gated on ConstantCoalescingDepthCheckMinBytes",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, ConstantCoalescingDepthCheckMinBytes, 0,
                   "Minimum merged-chunk size (in bytes) at which ConstantCoalescingMaxBBDepthDelta starts "
                   "rejecting cross-BB merges. Below this size the depth check is skipped on narrow merges. "
                   "0 disables the size gate.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, AllowSpillCompactionOnRetry, false, "Allow spill compaction on retry - may increase spills",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableRTGlobalsKnownValues, false,
                   "load MaxBVHLevels from RTGlobals rather than assumming = 2", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableRaytracingIntrinsicAttributes, false, "Turn off noalias and dereferenceable attributes",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisablePayloadSinking, false, "sink stores to payload into inlined continuations", ALWAYS)
DECLARE_IGC_REGKEY(bool, RayTracingKeepUDivRemWA, false, "Workaround till jitIsa supports cr0 for rtz conversions",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisablePromoteToScratch, false, "Use scratch space rather than SWStack when possible.", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableInvariantLoad, false, "Disabled !invariant_load metadata for raytracing shaders",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisablePreSplitOpts, false, "Disable last minute optimizations befoer shader splitting",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableStatefulRTStackAccess, false,
                   "do stateless rather than stateful accesses to the HW portion of the async stack", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableStatefulSWHotZoneAccess, false,
                   "do stateless rather than stateful accesses to the SW HotZone", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableStatefulSWStackAccess, false,
                   "do stateless rather than stateful accesses to the SW Stack", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableStatefulRTSyncStackAccess4RTShader, true,
                   "do stateless rather than stateful accesses to the HW portion of the sync stack. RT Shader only.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableStatefulRTSyncStackAccess4nonRTShader, true,
                   "do stateless rather than stateful accesses to the HW portion of the sync stack. nonRT Shader only.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableRTBindlessAccess, false,
                   "do bindful rather than bindless accesses to raytracing memory", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableRTStackOpts, false,
                   "Disable some optimizations that minimize reads/writes to the RTStack", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisablePrepareLoadsStores, false, "Disable preparation for MemOpt", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableRayTracingConstantCoalescing, false, "Disable coalescing", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableMergeAllocas, false, "Do not merge allocas prior to SplitAsyncPass", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableMergeAllocasPrivateMemory, false,
                   "Do not merge allocas prior to PrivateMemoryResolution", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableMergingOfMultipleAllocasWithOffset, true,
                   "Do not merge multiple smaller allocas under one larger one with different offsets.", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableMergingOfAllocasWithDifferentType, true, "Do not merge allocas of different types.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableRayTracingOptimizations, false, "Disable RayTracing Optimizations for debugging",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableLSCControlsForRayTracing, false,
                   "Disable different LSC Controls for HW and SW portions of the RTStack", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceRTStackLoadCacheCtrl, false,
                   "Enables RTStackLoadCacheCtrl regkey for custom lsc load cache controls in the RTStack", ALWAYS)
DECLARE_IGC_REGKEY_ENUM(RTStackLoadCacheCtrl, 0, "Load Cache Controls", LSC_CACHE_CTRL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceRTStackStoreCacheCtrl, false,
                   "Enables RTStackStoreCacheCtrl regkey for custom lsc store cache controls in the RTStack", ALWAYS)
DECLARE_IGC_REGKEY_ENUM(RTStackStoreCacheCtrl, 0, "Store Cache Controls", LSC_CACHE_CTRL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceSWStackLoadCacheCtrl, false,
                   "Enables SWStackLoadCacheCtrl regkey for custom lsc load cache controls in the SWStack", ALWAYS)
DECLARE_IGC_REGKEY_ENUM(SWStackLoadCacheCtrl, 0, "Load Cache Controls", LSC_CACHE_CTRL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceSWStackStoreCacheCtrl, false,
                   "Enables SWStackStoreCacheCtrl regkey for custom lsc store cache controls in the SWStack", ALWAYS)
DECLARE_IGC_REGKEY_ENUM(SWStackStoreCacheCtrl, 0, "Store Cache Controls", LSC_CACHE_CTRL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceSWHotZoneLoadCacheCtrl, false,
                   "Enables SWHotZoneLoadCacheCtrl regkey for custom lsc load cache controls in the SWHotZone", ALWAYS)
DECLARE_IGC_REGKEY_ENUM(SWHotZoneLoadCacheCtrl, 0, "Load Cache Controls", LSC_CACHE_CTRL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceSWHotZoneStoreCacheCtrl, false,
                   "Enables SWHotZoneStoreCacheCtrl regkey for custom lsc store cache controls in the SWHotZone",
                   ALWAYS)
DECLARE_IGC_REGKEY_ENUM(SWHotZoneStoreCacheCtrl, 0, "Store Cache Controls", LSC_CACHE_CTRL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceGenMemLoadCacheCtrl, false,
                   "Enables GenMemLoadCacheCtrl regkey for custom lsc load cache controls in other memory", ALWAYS)
DECLARE_IGC_REGKEY_ENUM(GenMemLoadCacheCtrl, 0, "Load Cache Controls", LSC_CACHE_CTRL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceGenMemStoreCacheCtrl, false,
                   "Enables GenMemStoreCacheCtrl regkey for custom lsc store cache controls in other memory", ALWAYS)
DECLARE_IGC_REGKEY_ENUM(GenMemStoreCacheCtrl, 0, "Store Cache Controls", LSC_CACHE_CTRL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceRTConstantBufferCacheCtrl, false,
                   "Enables RTConstantBufferCacheCtrl regkey for custom lsc load cache controls for constant buffers",
                   ALWAYS)
DECLARE_IGC_REGKEY_ENUM(RTConstantBufferCacheCtrl, 0, "Constant Buffer Load Cache Controls for raytracing shaders",
                        LSC_CACHE_CTRL_OPTIONS, ALWAYS)
DECLARE_IGC_REGKEY(
    bool, ForceGenMemDefaultCacheCtrl, false,
    "If enabled, no message specific cache ctrls are set on memory outside of RTStack, SWStack, and SWHotZone", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableRayQueryReturnOptimization, false, "RayQuery Return Optimization", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableRayQueryReturnOptimizationPackedStatus, false,
                   "RayQuery Return Optimization - Packed Status Return", ALWAYS)
DECLARE_IGC_REGKEY_BITMASK(UseNewInlineRaytracing, 4, "Use the new rayquery implementation for particular case",
                           NEW_INLINE_RAYTRACING_MASK, ALWAYS)
DECLARE_IGC_REGKEY(DWORD, AddDummySlotsForNewInlineRaytracing, 0,
                   "Add dummy rayquery slots when doing new inline raytracing", ALWAYS)
DECLARE_IGC_REGKEY(
    bool, UseCrossBlockLoadVectorizationForInlineRaytracing, true,
    "If enabled, will try to vectorize loads that are not adjacent to each other. May increase GRF pressure", ALWAYS)
DECLARE_IGC_REGKEY_ENUM(
    OverrideRayQueryThrottling, -1,
    "Controls rayquery throttling feature. 0: force disable, 1: force enable, -1: left for IGC to decide",
    TRIBOOL_OPTIONS, DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableRayQueryDynamicRayManagementMechanismForBarriers, false,
                   "Disable dynamic ray management mechanism for shaders with barriers", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableOuterLoopHoistingForRayQueryDynamicRayManagementMechanism, false,
                   "Disable dynamic ray management mechanism for shaders with barriers", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableProceedBasedApproachForRayQueryDynamicRayManagementMechanism, false,
                   "Disables proceed based approach for dynamic ray management mechanism", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableInvalidateRTStackAfterLastRead, true,
                   "Disables L1 cache invalidation after the last read of the RT stack. Affects rayqueries only",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableSWSubTriangleOpacityCullingEmulation, false,
                   "Software Sub-Triangle Opacity Culling emulation", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableRayTracingExtendedCacheControl, false,
                   "Disables the Extended Cache Control for Raytracing.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableRayTracingExtendedCacheControlTierI, false,
                   "Disables Extended Cache Control Tier I for Raytracing.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, ForceEnableRayTracingExtendedCacheControlTierI, false,
                   "Forces enabling Extended Cache Control Tier I, overriding Wa_14027487226.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, RayTracingExtendedCacheControlCachePolicyL2, 0,
                   "Sets the L2 cache policy for ExtendedCacheControl called in shader.\
                                                                                   Possible values:\
                                                                                   LSC_CACHE_OPT",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, RayTracingExtendedCacheControlCachePolicyL3, 0,
                   "Sets the L3 cache policy for ExtendedCacheControl called in shader.\
                                                                                   Possible values:\
                                                                                   LSC_CACHE_OPT",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableRayTracingMotionBlurSWEmulation, false, "Ray Tracing Motion Blur Software Emulation",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, RayTracingExtendedCacheControlCachePolicySyncStackL1, 0,
                   "Sets the L1 cache policy for ExtendedCacheControl called for SyncRayTracing.\
                                                                                   Possible values:\
                                                                                   LSC_CACHE_OPT",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableRayTracingSyncExtendedCacheControl, false,
                   "Disables clearing dirty bit in LSC for the SyncRayTracing HW stack.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableRayTracingSyncExtendedCacheControlForPotentialHit, false,
                   "Disables clearing dirty bit in LSC for the SyncRayTracing for the PotentialHit/ShortStack1/Ray1 "
                   "part of the HW stack.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableRayTracingSyncExtendedCacheControlFence, false,
                   "Disables adding a fence after the ECC messages for SyncRayTracing HW stack.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableNewRTStackLayoutOptimization, false,
                   "Ray Tracing New Stack Layout sync/async trace ray message optimization", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableNewBTDIndirect0DescriptorProgramming, true,
                   "Due to Bspec error globals pointer is always shifted by 6 bits in BTDIndirect0Descriptor.\
                                                                                 This flag enables BTDIndirect0Descriptor programming without this shift.",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableDoNotSendPayloadForCheckReleaseInEff64, true,
                   "According to Bspec payload should not be send for RayQuery Check/Release messages.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableWideTraceRay, false, "Disable SIMD16 style message payloads for send.rta", ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceRTCheckInstanceLeafPtr, true,
                   "Check MemHit::valid before loading GeometryIndex, PrimitiveIndex, etc.", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, RTInValidDefaultIndex, 0xFFFFFFFF,
                   "If MemHit::valid is false, the default value to return for some intrinsics like GeometryIndex or "
                   "PrimitiveIndex etc.",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceRTShortCircuitingOR, true,
                   "Only for specific test.... Short curcite OR condition if CommittedGeometryIndex is used", ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableLSCCacheOptimization, false,
                   "Optimize store instructions for utilizing the LSC-L1 cache", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, EnableSingleRQMemRayStore, true, "Store RayQuery MemRay[TOP] only once.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, TotalGRFNum4RQ, 0,
                   "Total GRF used for register allocation for RayQuery only. Test only. Delete later.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, ForceCSSimdSize4RQ, 0, "Force RayQuery compute shader simd size,\
                                                      valid values are 0 (not set), 8, 16 and 32\
                                                      ignored if produces invalid cofiguration, e.g. simd size too small for workgroup size",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, EnableRQHideLatency, false, "Hide RayQuery Proceed latency.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableShaderFusion, false, "Don't check for duplicate, renamed shaders", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableRTAliasAnalysis, false, "Disable Raytracing Alias Analysis", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableExamineRayFlag, false,
                   "Don't do IPO to see if we can fold control flow given knowledge of possible rayflag values",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableSpillReorder, false, "Disables reordering of spills to try to minmize spills in a loop",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisablePromoteContinuation, false,
                   "BTD-able continuations in the raygen may be moved to the shader identifier", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableRTMemDSE, false,
                   "Analyze stores to SWStack, etc. that aren't read before Stack ID Release", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableRTFenceElision, false, "Disable optimization to remove unneeded fences", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableDPSE, false, "Disable Dead PayloadStore Elimination.", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisablePredicatedStackIDRelease, false,
                   "Emit a single stack ID release at the end of the shader", ALWAYS)
DECLARE_IGC_REGKEY(bool, DisableCrossFillRemat, false, "Rematerialize values if they use already spilled values",
                   ALWAYS)
DECLARE_IGC_REGKEY(bool, ForceRTRetry, false, "Raytracing is compiled in the second retry state", DEBUG_ONLY)
DECLARE_IGC_REGKEY(
    bool, DisableRTRetryPickBetter, false,
    "Disables raytracing retry to pick the best compilation instead of always using the retry compilation.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableSWStackOffsetElision, false, "Avoid loading offseting when known at compile-time",
                   DEBUG_ONLY)
DECLARE_IGC_REGKEY(DWORD, OverrideTMax, 0, "Force TMax to the given value. When 0, do nothing.", DEBUG_ONLY)
DECLARE_IGC_REGKEY(bool, DisableLoadAsFenceOpInRaytracing, true,
                   "Disable load as fence op in raytracing (rayquery only)", DEBUG_ONLY)

DECLARE_IGC_GROUP("VectorCompiler Options")
DECLARE_IGC_REGKEY(bool, DisableEuFusion, false, "Require disable of EU fusion", ALWAYS)
DECLARE_IGC_REGKEY(bool, VCOptimizeNone, false, "Same as -optimize=none in vector compiler options", ALWAYS)
DECLARE_IGC_REGKEY(bool, VCStrictOptionParser, true, "Produce error on unknown API options in vector compiler", ALWAYS)
DECLARE_IGC_REGKEY(debugString, VCApiOptions, 0, "Extra API options for VC", ALWAYS)
DECLARE_IGC_REGKEY(debugString, VCInternalOptions, 0, "Extra Internal options to pass to VC", ALWAYS)
DECLARE_IGC_REGKEY(bool, VCLocalizeAccUsage, false, "Localization of possible accumulator usages for vISA RA", ALWAYS)
DECLARE_IGC_REGKEY(bool, VCDisableNonOverlappingRegionOpt, false, "Disable non-overlapping region optimization", ALWAYS)
DECLARE_IGC_REGKEY(bool, VCEnableExtraDebugLogging, false,
                   "Turns on extra debug output to trace IGC/VC-specific execution", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, VCNoOptFinalizerControl, 0, "Controls if finalizer is invoked with -debug flag", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, VCDisableLRCoalescingControl, 0, "Controls if LR coalescing", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, VCDisableExtraCoalescing, 0, "Disable extra coalescing", ALWAYS)
DECLARE_IGC_REGKEY(bool, VCSaveStackCallLinkage, false, "Do not override stack calls linkage as internal", ALWAYS)
DECLARE_IGC_REGKEY(bool, VCDirectCallsOnly, false, "Generate code under the assumption all unknown calls are direct",
                   ALWAYS)
DECLARE_IGC_REGKEY(DWORD, VCLoopUnrollThreshold, 0,
                   "Set the loop unroll threshold for VC. Value 0 will use the default threshold.", ALWAYS)
DECLARE_IGC_REGKEY(bool, VCIgnoreLoopUnrollThresholdOnPragma, false,
                   "Ignore threshold for loop unrolling when pragma is used", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, VCDepressurizerGRFThreshold, 2560, "Threshold for GRF pressure reduction", ALWAYS)
DECLARE_IGC_REGKEY(DWORD, VCDepressurizerFlagGRFTolerance, 3840, "Threshold for disabling flag pressure reduction",
                   ALWAYS)

