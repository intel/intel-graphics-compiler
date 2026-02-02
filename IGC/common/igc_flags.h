/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Template:
// DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode)
//
// releaseMode - The value is responsible for the availability of the flag in the Linux release

#include "common/EmUtils.h"

DECLARE_IGC_GROUP("VISA optimization")
DECLARE_IGC_REGKEY(DWORD, VISALTO, 0, "vISA LTO optimization flags. check LINKER_TYPE for more details", false)
DECLARE_IGC_REGKEY(
    bool, DisableSendS, false,
    "Setting this to 1/true adds a compiler switch to not generate sends commands, default is to enable sends ", false)
DECLARE_IGC_REGKEY(bool, ForcePreserveR0, false, "Setting this to true makes VISA preserve r0 in r0", true)
DECLARE_IGC_REGKEY(bool, EnablePreemption, true, "Enable generating preeemptable code (SKL+)", false)
DECLARE_IGC_REGKEY(bool, ForcePreemptionWA, false, "Force generating preemptable code across platforms", true)
DECLARE_IGC_REGKEY(bool, EnableVISANoSchedule, false, "Enable VISA No-Schedule", true)
DECLARE_IGC_REGKEY(bool, EnableVISAPreSched, true, "Enable VISA Pre-RA Scheduler", true)
DECLARE_IGC_REGKEY(bool, DisableVISASBIDCounter, false, "Disable VISA SBID Counter feature", true)
DECLARE_IGC_REGKEY(DWORD, VISAPreSchedCtrl, 0,
                   "Configure Pre-RA Scheduler, default(0), logging(1), latency(2), pressure(4)", true)
DECLARE_IGC_REGKEY(DWORD, VISAPreSchedCtrlDpas, 0, "Special Pre-RA Scheduler configuration for kernels with dpas", true)
DECLARE_IGC_REGKEY(bool, ForceVISAPreSched, false, "Force enabling of VISA Pre-RA Scheduler", false)
DECLARE_IGC_REGKEY(DWORD, VISAPreSchedRPThreshold, 0,
                   "Threshold to commit a pre-RA Scheduling without spills, 0 for the default", false)
DECLARE_IGC_REGKEY(DWORD, VISAPreSchedExtraGRF, 0,
                   "Bump up GRF number to make pre-RA Scheduling more greedy, 0 for the default", false)
DECLARE_IGC_REGKEY(DWORD, VISAScheduleStartBBID, 0, "The ID of BB which will be first scheduled", false)
DECLARE_IGC_REGKEY(DWORD, VISAScheduleEndBBID, 0, "The ID of BB which will be last scheduled", false)
DECLARE_IGC_REGKEY(DWORD, VISAPostScheduleStartBBID, 0, "The ID of BB which will be first scheduled", false)
DECLARE_IGC_REGKEY(DWORD, VISAPostScheduleEndBBID, 0, "The ID of BB which will be last scheduled", false)
DECLARE_IGC_REGKEY(DWORD, VISASpillAllowed, 256, "Spill size allowed without increasing GRF number in VRT", false)
DECLARE_IGC_REGKEY(DWORD, VISASpillAllowed256GRF, 0, "Spill size allowed specifically for 256 GRF case", false)
DECLARE_IGC_REGKEY(DWORD, ForceAllowSmallSpill, 0,
                   "Allow small spills regardless of SIMD, API, or platform. The spill amount is set below", false)
DECLARE_IGC_REGKEY(DWORD, SIMD8_SpillThreshold, 2, "Percentage of instructions allowed for spilling on SIMD8", false)
DECLARE_IGC_REGKEY(DWORD, SIMD16_SpillThreshold, 1, "Percentage of instructions allowed for spilling on SIMD16", false)
DECLARE_IGC_REGKEY(DWORD, SIMD32_SpillThreshold, 1, "Percentage of instructions allowed for spilling on SIMD32", false)
DECLARE_IGC_REGKEY(DWORD, CSSIMD16_SpillThreshold, 1, "Percentage of instructions allowed for spilling on CS SIMD16",
                   false)
DECLARE_IGC_REGKEY(DWORD, CSSIMD32_SpillThreshold, 1, "Percentage of instructions allowed for spilling on CS SIMD32",
                   false)
DECLARE_IGC_REGKEY(DWORD, CSSIMD32_HighThresholdInstCount, 1000,
                   "Instructions count limit to allow higher spill threshold on CS SIMD32", false)
DECLARE_IGC_REGKEY(bool, DisableCSEL, false, "disable csel peep-hole", false)
DECLARE_IGC_REGKEY(bool, DisableFlagOpt, false, "Disable optimization cmp with logic op", false)
DECLARE_IGC_REGKEY(bool, DisableIfCvt, false, "Disable ifcvt", false)
DECLARE_IGC_REGKEY(bool, EnableVISANoBXMLEncoder, false, "Enable VISA No-BXML encoder", false)
DECLARE_IGC_REGKEY(bool, EnableIGAEncoder, false, "Enable VISA IGA encoder", false)
DECLARE_IGC_REGKEY(bool, EnableVISADumpCommonISA, false, "Enable VISA Dump Common ISA", true)
DECLARE_IGC_REGKEY(bool, DumpVISAASMToConsole, false, "Dump VISAASM to console and do early exit", true)
DECLARE_IGC_REGKEY(bool, DumpASMToConsole, false, "Dump ASM to console and do early exit", true)
DECLARE_IGC_REGKEY(bool, AddVISADumpDeclarationsToEnd, false,
                   "Add a comment with .decl section to the end of VISA console dump. Used in tests.", true)
DECLARE_IGC_REGKEY(bool, EnableVISABinary, false, "Enable VISA Binary", true)
DECLARE_IGC_REGKEY(bool, EnableVISAOutput, false, "Enable VISA GenISA output", true)
DECLARE_IGC_REGKEY(bool, EnableVISASlowpath, false, "Enable VISA Slowpath. Needed to dump .visaasm", true)
DECLARE_IGC_REGKEY(bool, EnableVISADotAll, false, "Enable VISA DotAll. Dumps dot files for intermediate stages", false)
DECLARE_IGC_REGKEY(bool, EnableVISADebug, false, "Runs VISA in debug mode, all optimizations disabled", false)
DECLARE_IGC_REGKEY(DWORD, EnableVISAStructurizer, 1,
                   "Enable/Disable VISA structurizer. See value defs in igc_flags.hpp.", false)
DECLARE_IGC_REGKEY(bool, EnableVISAJmpi, true, "Enable/Disable VISA generating jmpi (scalar jump).", false)
DECLARE_IGC_REGKEY(
    bool, ForceVISAStructurizer, false,
    "Force VISA structurizer for testing. Used on platforms in which we turns off SCF and use UCF by default", false)
DECLARE_IGC_REGKEY(bool, EnableVISABoundsChecking, true, "Enable VISA bounds checking.", false)
DECLARE_IGC_REGKEY(DWORD, MaxPerThreadScratchSpaceOverride, 0,
                   "Override the maximum per-thread scratch space limit for testing purposes. This setting simulates "
                   "hardware with constrained scratch memory and is propagated to both IGC and vISA. Note: vISA has "
                   "its own PTSS query function that will also respect this override",
                   true)
DECLARE_IGC_REGKEY(bool, NoMaskWA, true, "Enable NoMask WA by using software-computed emask flag", false)
DECLARE_IGC_REGKEY(bool, ForceNoMaskWA, false, "[tmp, testing] Force NoMaskWA on any platforms", false)
DECLARE_IGC_REGKEY(bool, EnableCallUniform, true, "[tmp, testing] Ignore indirect call's uniform", true)
DECLARE_IGC_REGKEY(bool, EnableCallWA, true, "Control call WA when EU fusion is on. 0: off; 1: on", true)
DECLARE_IGC_REGKEY(bool, EnableMathDPASWA, false, "PVC math instruction running with DPAS issue", false)
DECLARE_IGC_REGKEY(
    bool, ForceSubReturn, true,
    "If a subroutine does not have a return, generate a dummy return if this key is set (to meet visa requirement)",
    false)
DECLARE_IGC_REGKEY(bool, EnableKeepDpasMacro, false,
                   "If enabled, dpas macro sequence from input will not be broken up by visa scheduler", false)
DECLARE_IGC_REGKEY(DWORD, UnifiedSendCycle, 0, "Using unified send cycle.", false)
DECLARE_IGC_REGKEY(DWORD, DisableMixMode, 0, "Disables mix mode in vISA BE.", false)
DECLARE_IGC_REGKEY(DWORD, DisableHFMath, 0, "Disables HF math instructions.", false)
DECLARE_IGC_REGKEY(debugString, VISAOptions, 0, "Options to vISA. Space-separated options.", true)
DECLARE_IGC_REGKEY(DWORD, disableIGASyntax, false, "Disables GEN isa text output using IGA and new syntax.", false)
DECLARE_IGC_REGKEY(DWORD, disableCompaction, false, "Disables compaction.", true)
DECLARE_IGC_REGKEY(DWORD, TotalGRFNum, 0, "Total GRF setting for both IGC-LLVM and vISA", true)
DECLARE_IGC_REGKEY(DWORD, TotalGRFNum4CS, 0,
                   "Total GRF setting for both IGC-LLVM and vISA, for ComputeShader-only experiment.", false)
DECLARE_IGC_REGKEY(DWORD, ReservedRegisterNum, 0, "Reserve register number for spill cost testing.", false)
DECLARE_IGC_REGKEY(bool, ExpandPlane, false, "Enable pln to mad macro expansion.", false)
DECLARE_IGC_REGKEY(bool, DisableGatherRSFusionSyncWA, false,
                   "Disable WA for gather instruction when read suppression and EU fusion are enabled.", true)
DECLARE_IGC_REGKEY(bool, EnableBCR, false, "Enable bank conflict reduction.", true)
DECLARE_IGC_REGKEY(bool, ForceBCR, false, "Force bank conflict reduction, no matter spill or not.", true)
DECLARE_IGC_REGKEY(bool, BumpGRFForForceBCR, false, "Bump up GRF mode for force BCR.", true)
DECLARE_IGC_REGKEY(bool, EnableForceDebugSWSB, false,
                   "Enable force debugging functionality for software scoreboard generation", true)
DECLARE_IGC_REGKEY(DWORD, EnableSWSBInstStall, 0,
                   "Enable force stall to specific(start) instruction start for software scoreboard generation", true)
DECLARE_IGC_REGKEY(DWORD, EnableSWSBInstStallEnd, 0,
                   "Enable force stall to end instruction for software scoreboard generation", true)
DECLARE_IGC_REGKEY(bool, SWSBMakeLocalWAR, false, "make WAR SBID dependence tracking BB local", true)
DECLARE_IGC_REGKEY(bool, PVCSendWARWA, true, "enable PVC send WAR WA", true)
DECLARE_IGC_REGKEY(DWORD, WARSWSBLocalStart, 0, "WAR localization start BB", true)
DECLARE_IGC_REGKEY(DWORD, WARSWSBLocalEnd, 0, "WAR localization end BB", true)
DECLARE_IGC_REGKEY(bool, SWSBReplaceARWithAW, false, "replace .src with .dst", true)
DECLARE_IGC_REGKEY(DWORD, EnableIndirectInstStart, 0,
                   "Enable the indirect sent, start with candidate of the id value specified by the key", true)
DECLARE_IGC_REGKEY(DWORD, EnableIndirectInstEnd, 0,
                   "Enable the indirect sent, end with candidate of the id value specified by the key", true)
DECLARE_IGC_REGKEY(DWORD, EnableSWSBTokenBarrier, 0,
                   "Enable force specific instruction as a barrier for software scoreboard generation", true)
DECLARE_IGC_REGKEY(DWORD, SWSBTokenNum, 0, "Total tokens used for SWSB.", true)
DECLARE_IGC_REGKEY(bool, EnableGroupScheduleForBC, false, "Enable bank conflict reduction in scheduling.", true)
DECLARE_IGC_REGKEY(bool, SchedWithSendSrcReadCycle, false, "Scheduling with GRF read cycle from send.", true)
DECLARE_IGC_REGKEY(bool, EnableIGASWSB, false, "Use IGA for SWSB", true)
DECLARE_IGC_REGKEY(bool, EnableSWSBStitch, false, "Insert dependence resolve for kernel stitching", true)
DECLARE_IGC_REGKEY(bool, DisableRegDistDep, false, "distable regDist dependence", true)
DECLARE_IGC_REGKEY(bool, EnableQuickTokenAlloc, false, "Insert dependence resolve for kernel stitching", true)
DECLARE_IGC_REGKEY(DWORD, EnableGatherWithImmPreRA, 0,
                   "0: disabled, 1: sampler is enabled, 2: other msg enabled, 3 always use s0.0 for send", true)
DECLARE_IGC_REGKEY(bool, SetA0toTdrForSendc, false, "Set A0 to tdr0 before each sendc/sendsc", true)
DECLARE_IGC_REGKEY(bool, ReplaceIndirectCallWithJmpi, false, "Replace indirect call with jmpi instruction (HW WA)",
                   true)
DECLARE_IGC_REGKEY(bool, ForceUniformSurfaceSampler, false, "Force surface and sampler operand to be uniform", false)
DECLARE_IGC_REGKEY(bool, ForceUniformBuffer, false, "Force buffer operand to be uniform", false)
DECLARE_IGC_REGKEY(bool, AssumeUniformIndirectCall, false, "Assume indirect call is uniform to avoid looping code",
                   false)
DECLARE_IGC_REGKEY(bool, EnableHWGenerateThreadID, true,
                   "Enable new behavior of HW generating threadID for GPGPU pipe. XeHP and non-OCL only.", true)
DECLARE_IGC_REGKEY(bool, EnableHWGenerateThreadIDForTileY, true,
                   "Enable HW generating threadID for GPGPU pipe for TileY mode. XeHP and non-OCL only.", true)
DECLARE_IGC_REGKEY(bool, EnableNonOCLWalkOrderSel, true,
                   "Enable WalkOrder selection for HW generating threadID for GPGPU pipe. XeHP and non-OCL only.", true)
DECLARE_IGC_REGKEY(
    DWORD, EnablePassInlineData, 0,
    "1: Force pass 1st GRF of cross-thread payload as inline data; -1: Force disable passing inline data", true)
DECLARE_IGC_REGKEY(bool, ForceInlineDataForXeHPC, false, "Force InlineData for XeHPC. For testing purposes.", true)
DECLARE_IGC_REGKEY(DWORD, ScratchSpaceSizeReserved, 0,
                   "Reserved size of scratch space. XeHP and above only. Test only. Remove it once stabalized.", true)
DECLARE_IGC_REGKEY(DWORD, ScratchSpaceSizeLimit, 0,
                   "Size limit of scratch space. XeHP and above only. Test only. Remove it once stabalized.", true)
DECLARE_IGC_REGKEY(
    bool, EnablePromoteI8, true,
    "Enable promoting i8 (char) to i16 on all ALU insts that does support i8. It's only for XeHPC+ for now.", true)
DECLARE_IGC_REGKEY(bool, ForcePromoteI8, false, "Force promoting i8 (char) to i16 on all ALU insts (for testing).",
                   true)
DECLARE_IGC_REGKEY(bool, DumpPromoteI8, false, "Dump useful info during promoting i8 to i16", true)
DECLARE_IGC_REGKEY(bool, EnablePromoteI8Vec, true,
                   "Control if a certain i8 vector needs to be promoted (detail in code)", true)
DECLARE_IGC_REGKEY(DWORD, ForceTexelMaskClear, 0,
                   "If set to 1 or 2, forces evaluate messages to clear the texel mask to 0 or 1, respectively.", true)
DECLARE_IGC_REGKEY(bool, EnablePvtMemHalfToFloat, true, "Enable conversion from half to float for private memory.",
                   true)
DECLARE_IGC_REGKEY(bool, EnableQWRotateInstructions, true, "Enable QW type support for rotate instructions. PVC only.",
                   true)
DECLARE_IGC_REGKEY(bool, DPASTokenReduction, false, "optimization to reduce the tokens used for DPAS instruction.",
                   true)
DECLARE_IGC_REGKEY(bool, EnableAdd3, true, "Enable Add3. XeHP+ only", true)
DECLARE_IGC_REGKEY(bool, EnableBfn, true, "Enable Bfn. XeHP+ only", true)
DECLARE_IGC_REGKEY(bool, SeparateSpillPvtScratchSpace, false,
                   "Separate scratch spaces for spillfill and privatememory. XeHP and above only. Test only. Remove it "
                   "once stabalized.",
                   true)
DECLARE_IGC_REGKEY(
    bool, EnableSeparateScratchWA, false,
    "Apply the workaround in slot0 and slot1 sizes when separating scratch spacesSeparate scratch space.", true)
DECLARE_IGC_REGKEY(bool, DisableThreeALUPipes, false, "Disable three ALU Pipelines. XeHP only", true)
DECLARE_IGC_REGKEY(bool, Enable16DWURBWrite, false, "Enable 16 Dword URB Write messages", true)
DECLARE_IGC_REGKEY(bool, Enable16OWSLMBlockRW, true, "Enable 16 OWord (8 GRF) SLM block read/write message", true)
DECLARE_IGC_REGKEY(bool, Enable64BMediaBlockRW, false, "Enable 64 byte wide media block read/write message", true)
DECLARE_IGC_REGKEY(bool, EnableUntypedSurfRWofSS, true, "Enable untyped surface RW to scratch space. XeHP A0 only.",
                   true)
DECLARE_IGC_REGKEY(bool, GetSendAfterWriteDistance, false, "Get the after write dependence distance", true)
DECLARE_IGC_REGKEY(bool, EnableReadStateToA64Read, false,
                   "Instead of using Read State info to fetch surface format etc use direct A64 read of Surface state "
                   "for Xe3P+ platforms",
                   false)

DECLARE_IGC_REGKEY(DWORD, ForceHWThreadNumberPerEU, 0, "Total HW thread number per-EU.", false)
DECLARE_IGC_REGKEY(bool, UseMathWithLUT, false,
                   "Use the implementations of cos, cospi, log, sin, sincos, and sinpi with Look-Up Tables (LUT).",
                   false)
DECLARE_IGC_REGKEY(bool, GlobalSendVarSplit, false, "Enable global send variable splitting when we are about to spill",
                   false)
DECLARE_IGC_REGKEY(DWORD, EnableSendFusion, 1,
                   "Enable(!=0)/disable(0)/force(2) send fusion. Valid for simd8 shader/kernel only.", false)
DECLARE_IGC_REGKEY(bool, EnableAtomicFusion, false,
                   "To enable/disable atomic send fusion (simd8 shaders). Valid if EnableSendFusion is on.", false)
DECLARE_IGC_REGKEY(bool, Use16ByteBindlessSampler, false, "True if 16-byte aligned bindless sampler state is used",
                   false)
DECLARE_IGC_REGKEY(bool, AvoidDstSrcGRFOverlap, false,
                   "avoid GRF overlap for destination and source operands of an SIMD16/SIMD32 instruction ", false)
DECLARE_IGC_REGKEY(bool, AvoidSrc1Src2Overlap, false,
                   "avoid src1 and src2 GRF overlap to avoid the conflict without read suppression ", false)
DECLARE_IGC_REGKEY(bool, UseLinearScanRA, false, "use Linear Scan as default register allocation algorithm ", false)
DECLARE_IGC_REGKEY(bool, DisableWriteCombine, false, "Disable write combine. PVC+ only", false)
DECLARE_IGC_REGKEY(bool, Force32bitConstantGEPLowering, false,
                   "Go back to old version of GEP lowering for constant address space. PVC only", false)
DECLARE_IGC_REGKEY(bool, GEPLoweringTruncOptEnabled, true,
                   "Enable using truncation to avoid recalculation in GEP lowering", false)
DECLARE_IGC_REGKEY(bool, NewSpillCostFunction, false, "Use new spill cost function in VISA RA", false)
DECLARE_IGC_REGKEY(bool, EnableCoalesceScalarMoves, true, "Enable scalar moves to be coalesced into fewer moves", true)
DECLARE_IGC_REGKEY(DWORD, EnableSpillSpaceCompression, 2,
                   "Enable spill space compression. 0 - off, 1 - on, 2 - platform default", false)
DECLARE_IGC_REGKEY(DWORD, SpillCompressionThresholdOverride, 0,
                   "Set a threshold number (1K based) to run with spill compression", false)
DECLARE_IGC_REGKEY(bool, EnableRemoveLoopDependency, false,
                   "Enable removing of fantom loop dependency introduced by SROA", true)
DECLARE_IGC_GROUP("IGC Optimization")
DECLARE_IGC_REGKEY(bool, AllowMem2Reg, false,
                   "Setting this to true makes IGC run mem2reg even when optimizations are disabled", true)
DECLARE_IGC_REGKEY(bool, DisableIGCOptimizations, false,
                   "Setting this to 1/true adds a compiler switch to disables all the above IGC optimizations", false)
DECLARE_IGC_REGKEY(bool, DisableLLVMGenericOptimizations, false, "Disable LLVM generic optimization passes", false)
DECLARE_IGC_REGKEY(bool, DisableCodeSinking, false,
                   "Setting this to 1/true adds a compiler switch to disable code-sinking", false)
DECLARE_IGC_REGKEY(bool, DisableCodeSinkingInputVec, false,
                   "Setting this to 1/true disable sinking inputVec inst (test)", false)
DECLARE_IGC_REGKEY(DWORD, CodeSinkingMinSize, 32, "Don't sink if the number of instructions in the kernel is less",
                   false)

// Code Loop Sinking
DECLARE_IGC_REGKEY(bool, DisableLoopSink, false, "Disable sinking in all loops", true)
DECLARE_IGC_REGKEY(bool, ForceLoopSink, false, "Force sinking in all loops", false)
DECLARE_IGC_REGKEY(bool, EnableLoadsLoopSink, true, "Allow sinking of loads in the loop", false)
DECLARE_IGC_REGKEY(bool, ForceLoadsLoopSink, false, "Force sinking of loads in the loop from the beginning", false)
DECLARE_IGC_REGKEY(bool, PrepopulateLoadChainLoopSink, true,
                   "Check the loop for loop chains before sinking to use the existing chains in a heuristic", false)
DECLARE_IGC_REGKEY(bool, EnableLoadChainLoopSink, true,
                   "Allow sinking of load address calculation when the load was sinked to the loop, even if the needed "
                   "regpressure is achieved (only single use instructions)",
                   false)
DECLARE_IGC_REGKEY(DWORD, LoopSinkRegpressureMargin, 10,
                   "Sink into the loop until the pressure becomes less than #grf-margin", false)
DECLARE_IGC_REGKEY(DWORD, CodeLoopSinkingMinSize, 100,
                   "Don't sink in the loop if the number of instructions in the kernel is less", false)
DECLARE_IGC_REGKEY(DWORD, CodeSinkingLoadSchedulingInstr, 20,
                   "Instructions number to step to schedule loads in advance before the load use to cover latency. 0 "
                   "to insert it immediately before use",
                   false)
DECLARE_IGC_REGKEY(DWORD, CodeSinking2dLoadSchedulingInstr, 5,
                   "Instructions number to step to schedule 2d loads in advance before the load use to cover latency. "
                   "0 to insert it immediately before use",
                   false)
DECLARE_IGC_REGKEY(DWORD, LoopSinkMinSaveUniform, 6,
                   "If loop sink can have save more scalar (uniform) values than this Minimum, do it; otherwise, skip",
                   false)
DECLARE_IGC_REGKEY(DWORD, LoopSinkMinSave, 1,
                   "If loop sink can have save more 32-bit values than this Minimum, do it; otherwise, skip", false)
DECLARE_IGC_REGKEY(DWORD, LoopSinkThresholdDelta, 30,
                   "Do loop sink If the estimated register pressure is higher than this + #avaialble registers", false)
DECLARE_IGC_REGKEY(DWORD, LoopSinkRollbackThreshold, 15,
                   "Rollback loop sinking if the estimated regpressure after the sinking is still higher than this + "
                   "#available registers, and the number of registers can be increased",
                   false)
DECLARE_IGC_REGKEY(bool, LoopSinkEnableLoadsRescheduling, true, "Allow sinking the loads that are already in the loop",
                   false)
DECLARE_IGC_REGKEY(bool, LoopSinkCoarserLoadsRescheduling, false,
                   "Try to reschedule multi-instruction load candidates in larger chunks", false)
DECLARE_IGC_REGKEY(bool, LoopSinkEnable2dBlockReads, true, "Allow sinking of the 2d block reads", false)
DECLARE_IGC_REGKEY(bool, LoopSinkEnableVectorShuffle, true, "Allow sinking of the lowered vector shuffle pattern",
                   false)
DECLARE_IGC_REGKEY(bool, LoopSinkForceRollback, false, "Rollback every loop sinking change (for debug purposes only)",
                   false)
DECLARE_IGC_REGKEY(bool, LoopSinkDisableRollback, false,
                   "Disable loopsink rollback completely (even in case of increased regpressure)", false)
DECLARE_IGC_REGKEY(
    bool, LoopSinkAvoidSplittingDPAS, true,
    "Sink before the whole DPAS sequence if the first use of the sinked instruction is not the first DPAS", false)
DECLARE_IGC_REGKEY(bool, LoopSinkForce2dBlockReadsMaxSink, true,
                   "Sink as much as possible in presence of 2d block loads", false)
DECLARE_IGC_REGKEY(bool, LoopSinkEnableLateRescheduling, false,
                   "Schedule more aggressively in the end if the needed regpressure is still not achieved", false)
DECLARE_IGC_REGKEY(bool, LoopSinkSkipDPASMacro, false, "If a dpas macro sequence is present, skip load sinking", true)

// Load Splitting
DECLARE_IGC_REGKEY(bool, LS_enableLoadSplitting, false, "Enable load splitting pass.", true)
DECLARE_IGC_REGKEY(bool, LS_ignoreSplitThreshold, false,
                   "If true, the pass splits loads regardless of the register pressure.", true)
DECLARE_IGC_REGKEY(DWORD, LS_minSplitSize_GRF, 1, "Minimal split size in GRFs.", true)
DECLARE_IGC_REGKEY(DWORD, LS_minSplitSize_E, 4, "Minimal split size in elements.", true)
DECLARE_IGC_REGKEY(DWORD, LS_splitThresholdDelta_GRF, 2,
                   "Register pressure must exceed total GRFs by this much for the load splitting to fire up.", true)
DECLARE_IGC_REGKEY(bool, LS_onlyStrided, true, "If true, only strided loads are considered for splitting.", true)

// Code Scheduling
DECLARE_IGC_REGKEY(bool, DisableCodeScheduling, false, "Disable local code scheduling", true)
DECLARE_IGC_REGKEY(bool, CodeSchedulingOnlyRecompilation, false, "Enable code scheduling only on 2nd try", true)

DECLARE_IGC_REGKEY(bool, EnableCodeSchedulingIfNoSpills, false, "Try rescheduling also when there are no spills", true)
DECLARE_IGC_REGKEY(bool, CodeSchedulingGreedyRPHigherRPCommit, false,
                   "If GreedyRP was chosen, commit it also if the estimated RP "
                   "is higher than the original schedule RP",
                   true)
DECLARE_IGC_REGKEY(bool, CodeSchedulingMWOptimizedHigherRPCommit, true,
                   "If the new schedule is expected to have better latency hiding, "
                   "commit it also if the estimated RP is higher than the original schedule RP",
                   true)
DECLARE_IGC_REGKEY(bool, CodeSchedulingForceMWOnly, false, "Force scheduling to consider only latency", true)
DECLARE_IGC_REGKEY(bool, CodeSchedulingForceRPOnly, false, "Force scheduling to consider only register pressure", true)
DECLARE_IGC_REGKEY(DWORD, CodeSchedulingAttemptsLimit, 10, "Limit the number of scheduling attempts", true)
DECLARE_IGC_REGKEY(DWORD, CodeSchedulingRPMargin, 15,
                   "Schedule so that the register pressure is less than #grf - margin", true)
DECLARE_IGC_REGKEY(bool, CodeSchedulingCommitGreedyRP, true,
                   "Commit greedy regpressure scheduling in case better "
                   "scheduling has not succeed",
                   true)
DECLARE_IGC_REGKEY(DWORD, CodeSchedulingRPThreshold, 0,
                   "Do scheduling only if the original register pressure is "
                   "higher than #GRF - margin + threshold",
                   true)

DECLARE_IGC_REGKEY(bool, DumpCodeScheduling, false, "Dump code scheduling", true)
DECLARE_IGC_REGKEY(DWORD, CodeSchedulingDumpLevel, 1, "Code scheduling dump verbosity level", true)
DECLARE_IGC_REGKEY(bool, CodeSchedulingRenameAll, false, "Allow renaming all values for debug purposes", false)
DECLARE_IGC_REGKEY(debugString, CodeSchedulingConfig, 0,
                   "Override the default scheduling config. Debug only - no backward compatibility", false)

DECLARE_IGC_REGKEY(bool, EnableLoopHoistConstant, false,
                   "Enables pass to check for specific loop patterns where variables are constant across all but the "
                   "last iteration, and hoist them out of the loop.",
                   false)
DECLARE_IGC_REGKEY(bool, DisableCodeHoisting, false,
                   "Setting this to 1/true adds a compiler switch to disable code-hoisting", false)
DECLARE_IGC_REGKEY(bool, EnableDeSSA, true, "Setting this to 0/false adds a compiler switch to disable De-SSA", false)
DECLARE_IGC_REGKEY(bool, EnableDeSSAWA, true, "[tmp]Keep some piece of code to avoid perf regression", false)
DECLARE_IGC_REGKEY(
    bool, DisablePayloadCoalescing, false,
    "Setting this to 1/true adds a compiler switch to disable payload coalescing optimization for all types", false)
DECLARE_IGC_REGKEY(
    bool, DisablePayloadCoalescing_RT, false,
    "Setting this to 1/true adds a compiler switch to disable payload coalescing optimization for RT only", false)
DECLARE_IGC_REGKEY(
    bool, DisablePayloadCoalescing_Sample, false,
    "Setting this to 1/true adds a compiler switch to disable payload coalescing optimization for Samplers only", false)
DECLARE_IGC_REGKEY(
    bool, DisablePayloadCoalescing_URB, false,
    "Setting this to 1/true adds a compiler switch to disable payload coalescing optimization for URB writes only",
    false)
DECLARE_IGC_REGKEY(
    bool, DisablePayloadCoalescing_AtomicTyped, false,
    "Setting this to 1/true adds a compiler switch to disable payload coalescing optimization for atomic typed only",
    false)
DECLARE_IGC_REGKEY(bool, DisableUniformAnalysis, false,
                   "Setting this to 1/true adds a compiler switch to disable uniform_analysis", false)
DECLARE_IGC_REGKEY(bool, EnableWorkGroupUniformGoto, false,
                   "Setting to 1 enables generating uniform goto for work group uniform [eu fusion only]", false)
DECLARE_IGC_REGKEY(DWORD, DisablePushConstant, 0,
                   "Bit mask to disable push constant per shader stages. bit0 = All, Bit 1 = VS, Bit 2 = HS, Bit 3 = "
                   "DS, Bit 4 = GS, Bit 5 = PS",
                   false)
DECLARE_IGC_REGKEY(
    DWORD, DisableAttributePush, 0,
    "Bit mask to disable push Attribute per shader stages. bit0 = All, Bit 1 = VS, Bit 2 = HS, Bit 3 = DS, Bit 4 = GS",
    false)
DECLARE_IGC_REGKEY(bool, EnableRobustBufferAccessPush, false,
                   "Setting to 1/true will allow a single push buffer to be supported when the client requests robust "
                   "buffer access (DG2+ only)",
                   false)
DECLARE_IGC_REGKEY(bool, DisableSimplePushWithDynamicUniformBuffers, false,
                   "Disable Simple Push Constants Optimization for dynamic uniform buffers.", false)
DECLARE_IGC_REGKEY(bool, DisableStaticCheck, false, "Disable static check to push constants.", false)
DECLARE_IGC_REGKEY(bool, DisableStaticCheckForConstantFolding, true, "Disable static check to fold constants.", false)
DECLARE_IGC_REGKEY(int, forcePushConstantMode, 0,
                   "set the push constant mode, 0 is default behavior, 1 is simple push, 2 is gather constant, 3 is "
                   "none/pull constants",
                   false)
DECLARE_IGC_REGKEY(bool, EnableSimplePushSizeBasedOpimization, true,
                   "Enable the simplepush optimization to do push based on size", false)
DECLARE_IGC_REGKEY(bool, DisableConstantCoalescing, false,
                   "Setting this to 1/true adds a compiler switch to disable constant coalesing", false)
DECLARE_IGC_REGKEY(bool, DisableConstantCoalescingOutOfBoundsCheck, false,
                   "Setting this to 1/true adds a compiler switch to disable constant coalesing out of bounds check",
                   false)
DECLARE_IGC_REGKEY(
    bool, DisableConstantCoalescingOfStatefulNonUniformLoads, false,
    "Disable merging non-uniform loads from stateful buffers. Note: does not affect merging to sampler loads", false)
DECLARE_IGC_REGKEY(bool, EnableTextureLoadCoalescing, false, "Enable merging non-uniform loads from bindless textures",
                   false)
DECLARE_IGC_REGKEY(bool, UseHDCTypedReadForAllTextures, false,
                   "Setting this to use HDC message rather than sampler ld for texture read", false)
DECLARE_IGC_REGKEY(bool, UseHDCTypedReadForAllTypedBuffers, false,
                   "Setting this to use HDC message rather than sampler ld for buffer read", false)
DECLARE_IGC_REGKEY(bool, DisableUniformTypedAccess, false, "Setting this will disable uniform typed access handling",
                   false)
DECLARE_IGC_REGKEY(bool, DisableURBWriteMerge, false,
                   "Setting this to 1/true adds a compiler switch to disable URB write merge", false)
DECLARE_IGC_REGKEY(bool, DisableURBReadMerge, false, "Disable IGC pass that merges URB Read instructions.", false)
DECLARE_IGC_REGKEY(bool, DisableURBPartialWritesPass, false,
                   "Disable IGC pass that converts URB partial writes to full-mask writes.", false)
DECLARE_IGC_REGKEY(DWORD, SetURBFullWriteGranularity, 0,
                   "Overrides the minimum access granularity for URB full writes.\
                                                            Valid values are 0, 16 and 32, value 0 means use default for the platform.",
                   true)
DECLARE_IGC_REGKEY(bool, DisableUniformURBWrite, false, "Disables generation of uniform URB write messages", false)
DECLARE_IGC_REGKEY(bool, DisableMatchFloor, false,
                   "Setting this to 1/true adds a compiler switch to disable sub-frc = floor optimization", false)
DECLARE_IGC_REGKEY(bool, DisableEmptyBlockRemoval, false,
                   "Setting this to 1/true adds a compiler switch to disable empty block optimization", false)
DECLARE_IGC_REGKEY(bool, DisableSIMD32Slicing, false,
                   "Setting this to 1/true adds a compiler switch to disable emitting SIMD32 VISA code in slices",
                   false)
DECLARE_IGC_REGKEY(bool, DisableMatchMad, false,
                   "Setting this to 1/true adds a compiler switch to disable mul+add = mad optimization", false)
DECLARE_IGC_REGKEY(bool, WaAllowMatchMadOptimizationforVS, false,
                   "Setting this to 1/true adds a compiler switch to enable mul+add = mad optimization for VS", false)
DECLARE_IGC_REGKEY(bool, WaDisableMatchMadOptimizationForCS, false,
                   "Setting this to 1/true adds a compiler switch to disable mul+add = mad optimization for CS", false)
DECLARE_IGC_REGKEY(bool, DisableLoadSinking, false,
                   "Setting this to 1/true adds a compiler switch to disable load sinking during retry", false)
DECLARE_IGC_REGKEY(bool, EnableIntegerMad, true,
                   "Setting this to 1/true adds a compiler switch to enable integer mul+add = mad optimization", false)
DECLARE_IGC_REGKEY(bool, DisableMatchPredAdd, false,
                   "Setting this to 1/true adds a compiler switch to disable pred+add = predAdd optimization", false)
DECLARE_IGC_REGKEY(bool, DisableMatchSimpleAdd, false,
                   "Setting this to 1/true adds a compiler switch to disable simple cmp+and+add optimization", false)
DECLARE_IGC_REGKEY(bool, DisableMatchPow, false,
                   "Setting this to 1/true adds a compiler switch to disable log2/mul/exp2 = pow optimization", false)
DECLARE_IGC_REGKEY(bool, DisableIRVerification, false,
                   "Setting this to 1/true adds a compiler switch to disable IGC IR verification.", false)
DECLARE_IGC_REGKEY(bool, EnableJumpThreading, true,
                   "Setting this to 1/true adds a compiler switch to enable llvm jumpThreading pass.", true)
DECLARE_IGC_REGKEY(bool, DisableLoopUnroll, false,
                   "Setting this to 1/true adds a compiler switch to disable loop unrolling.", true)
DECLARE_IGC_REGKEY(DWORD, RuntimeLoopUnrolling, 0,
                   "Setting this to switch on/off runtime loop unrolling. 0: default (on), 1: force on, 2: force off",
                   false)
DECLARE_IGC_REGKEY(bool, EnableIndVarSimplification, true, "Enables IndVarSimplification pass.", true)
DECLARE_IGC_REGKEY(bool, DisableBranchSwaping, false,
                   "Setting this to 1/true adds a compiler switch to disable branch swapping.", false)
DECLARE_IGC_REGKEY(bool, DisableSynchronizationObjectCoalescingPass, false,
                   "Disable SynchronizationObjectCoalescing pass", false)
DECLARE_IGC_REGKEY(
    bool, EnableIndependentSharedMemoryFenceFunctionality, false,
    "Enable treating global memory fences as shared memory fences in SynchronizationObjectCoalescing pass", false)
DECLARE_IGC_REGKEY(DWORD, SynchronizationObjectCoalescingConfig, 0,
                   "Modify the default behavior of SynchronizationObjectCoalescing value is a bitmask bit0 – remove "
                   "fences in read barrier write scenario",
                   true)
DECLARE_IGC_REGKEY(
    DWORD, DisableCoalescingSynchronizationObjectMask, 0,
    "The mask is casted to IGC::SyncInstMask and informs which synchronization objects should not be coalesced. Note "
    "that synchronization objects classified in multiple types are not disabled if any bit describing them is off.",
    true)
DECLARE_IGC_REGKEY(
    bool, ReplaceAtomicFenceWithSourceValue, true,
    "Fences are required to maintain the order of atomic memory instructions. This flag will replace the fence with "
    "GenISA_source_value intrinsic which sources the result of atomic operation and still maintains the order.",
    true)
DECLARE_IGC_REGKEY(bool, UnrollLoopForCodeSizeOnly, false,
                   "Only unroll the loop if it can reduce program size/register pressure. Ignore all other threshold "
                   "setting but still enable PromoteLoopUnrollwithAlloca due to high likelyhood to reduce size.",
                   true)
DECLARE_IGC_REGKEY(DWORD, SetLoopUnrollThreshold, 0,
                   "Set the loop unroll threshold. Value 0 will use the default threshold.", false)
DECLARE_IGC_REGKEY(DWORD, SetLoopUnrollThresholdForHighRegPressure, 200,
                   "Set the loop unroll threshold for shaders with high reg pressure.", false)
DECLARE_IGC_REGKEY(DWORD, SetLoopUnrollMaxPercentThresholdBoostForHighRegPressure, 400,
                   "Set the loop unroll max allowed threshold boost in percentage for shaders with high reg pressure. "
                   "The LLVM internal value is 400.",
                   false)
DECLARE_IGC_REGKEY_ENUM(
    ForcePromoteLoopUnrollwithAlloca, -1,
    "Loop cost estimation assumes Load/Store who accesses Alloca with index deductible to loop count having 0 cost. "
    "Disable this flag makes them always cost something as well as disables dynamic threshold increase based on the "
    "size of alloca and number of GEP to the alloca in the loop, leading to the loop less likely to be unrolled."
    "-1 - default behavior, decided by platforms"
    " 0 - force disabled"
    " 1 - force enabled",
    TRIBOOL_OPTIONS, false)
DECLARE_IGC_REGKEY(DWORD, PromoteLoopUnrollwithAllocaCountThreshold, 256,
                   "The loop trip count OR number of alloca elements cutoff to stop regkey "
                   "EnablePromoteLoopUnrollwithAlloca (Check regkey description).",
                   false)
DECLARE_IGC_REGKEY(DWORD, SetRegisterPressureThresholdForLoopUnroll, 96,
                   "Set the register pressure threshold for limiting the loop unroll to smaller loops", false)
DECLARE_IGC_REGKEY(DWORD, SetBranchSwapThreshold, 400, "Set the branch swaping threshold.", false)
DECLARE_IGC_REGKEY(debugString, LLVMCommandLine, 0, "applies LLVM command line", false)
DECLARE_IGC_REGKEY(debugString, SelectiveHashOptions, 0, "applies options to hash range via string", false)
DECLARE_IGC_REGKEY(bool, DisableDX9LowPrecision, true, "Disables HF in DX9.", false)
DECLARE_IGC_REGKEY(
    bool, EnablePingPongTextureOpt, true,
    "Enables the Ping Pong texture optimization which is used only for Compute Shaders for back to back dispatches",
    false)
DECLARE_IGC_REGKEY(DWORD, EnableAtomicBranch, 0,
                   "Bitmask to enable Atomic branch optimization that predicates atomic with if/else. 1: if Val == 0 "
                   "ignore iadd/sub/umax 0. 2: checks if memory is lower than Val for umax. 4: checks if memory if "
                   "greater than Val for umin. 8: generate load_ugm for untyped atomics, otherwise ld_lz",
                   false)
DECLARE_IGC_REGKEY(bool, EnableThreeWayLoadSpiltOpt, false, "Enable three way load spilt opt.", false)
DECLARE_IGC_REGKEY(bool, DisableTypedWriteZeroStoreCheck, false,
                   "Disables eliminating a potential zero store by a typed "
                   "write instruction (moving the instruction under a "
                   "if-statement to guarantee a non-zero store)",
                   false)
DECLARE_IGC_REGKEY(
    DWORD, EnableSamplerChannelReturn, 1,
    "Setting this to 1/true adds a compiler switch to enable using header to return selective channels from sampler."
    "Setting this to 2 makes it always use the selected channels, without heuristic.",
    false)
DECLARE_IGC_REGKEY(bool, EnableThreadCombiningOpt, true,
                   "Enables the thread combining optimization which is used only for Compute Shaders for combining a "
                   "number of software threads to dispatch smaller number of hardware threads",
                   false)
DECLARE_IGC_REGKEY(bool, DisablePromotePrivMem, false,
                   "Setting this to 1/true adds a compiler switch to disable IGC private array promotion", false)
DECLARE_IGC_REGKEY(bool, EnableSimplifyGEP, true, "Enable IGC to simplify indices expr of GEP.", false)
DECLARE_IGC_REGKEY(bool, DisableCustomUnsafeOpt, false, "Disable IGC to run custom unsafe optimizations", false)
DECLARE_IGC_REGKEY(bool, DisableReducePow, false, "Disable IGC to reduce pow instructions", false)
DECLARE_IGC_REGKEY(bool, EnableFastSampleD, false, "Enable fast sample D opt.", false)
DECLARE_IGC_REGKEY(bool, DisableSqrtOpt, false, "Prevent IGC from doing the optimization y*y = x if y = sqrt(x)", false)
DECLARE_IGC_REGKEY(bool, EnableFastMath, false, "Enable fast math optimizations in IGC", false)
DECLARE_IGC_REGKEY(bool, DisableFlattenSmallSwitch, false, "Disable the flatten small switch pass", false)
DECLARE_IGC_REGKEY(bool, DisableIPConstantPropagation, false, "Disable Inter-procedrual constant propgation", false)
DECLARE_IGC_REGKEY(bool, EnableSplitIndirectEEtoSel, true, "Enable the split indirect extractelement to icmp+sel pass",
                   false)
DECLARE_IGC_REGKEY(DWORD, SplitIndirectEEtoSelThreshold, 8, "Split indirect extractelement cost threshold", false)
DECLARE_IGC_REGKEY(bool, DisableImmConstantOpt, false, "Disable IGC IndirectICBPropagaion optimization", false)
DECLARE_IGC_REGKEY(DWORD, MaxImmConstantSizePushed, 256, "Set the max size of immediate constant buffer pushed", false)
DECLARE_IGC_REGKEY(bool, RemoveUnusedSLM, true, "Remove SLM that are not used", false)
DECLARE_IGC_REGKEY(bool, RemoveUnusedTGMFence, false, "Remove TGM Fences that are not used/read", false)
DECLARE_IGC_REGKEY(bool, EnableCustomLoopVersioning, true, "Enable IGC to do custom loop versioning", false)
DECLARE_IGC_REGKEY(bool, DisableMCSOpt, false, "Disable IGC to run MCS optimization", false)
DECLARE_IGC_REGKEY(bool, MCSOptTwoStagesMode, false, "MCSOptimization gather all candidates than process", false)
DECLARE_IGC_REGKEY(bool, DisableGatingSimilarSamples, false, "Disable Gating of similar sample instructions", false)
DECLARE_IGC_REGKEY(bool, EnableSoftwareStencil, false, "Enable software stencil for PS.", false)
DECLARE_IGC_REGKEY(bool, EnableInterpreterPatternMatching, false,
                   "Enable Interpreter pattern matching and force retry if the pattern was found.", false)
DECLARE_IGC_REGKEY(bool, EnableSumFractions, false, "Enable SumFractions optimization in CustomUnsafeOptPass.", false)
DECLARE_IGC_REGKEY(bool, EnableExtractCommonMultiplier, false,
                   "Enable ExtractCommonMultiplier optimization in CustomUnsafeOptPass.", false)
DECLARE_IGC_REGKEY(bool, EnablePowToLogMulExp, false,
                   "Enable pow to exp(log(x)*y) optimization in CustomUnsafeOptPass.", false)
DECLARE_IGC_REGKEY(bool, DisablePullConstantHeuristics, true,
                   "Disable the heuristics to determine the no. push constants based on payload size.", false)
DECLARE_IGC_REGKEY(DWORD, PayloadSizeThreshold, 11,
                   "Set the max payload size threshold for short shades that have PSD bottleneck.", false)
DECLARE_IGC_REGKEY(DWORD, BlockPushConstantGRFThreshold, 0xFFFFFFFF,
                   "Set the maximum limit for block push constants i.e. UBO data pushed.\
                                                                Set to 0xFFFFFFFF to use the default threshold for the platform.\
                                                                Note that for small pixel shaders the PayloadSizeThreshold may be the limiting factor.",
                   false)
DECLARE_IGC_REGKEY(bool, PSSIMD32HeuristicFP16, true, "enable PS SIMD32 heuristic based on fp16 characteristic ", false)
DECLARE_IGC_REGKEY(bool, PSSIMD32HeuristicLoopAndDiscard, true,
                   "enable PS SIMD32 heuristic based on loop info and discard", false)
DECLARE_IGC_REGKEY(bool, EnableBlendToDiscard, true, "Enable blend to discard based on blend state.", false)
DECLARE_IGC_REGKEY(bool, EnableBlendToFill, true, "Enable blend to fill based on blend state.", false)
DECLARE_IGC_REGKEY(bool, UseTiledCSThreadOrder, true, "Use 4x4 disaptch for CS order when it seems beneficial", false)
DECLARE_IGC_REGKEY(bool, EnableWaveForce32, false, "Force Wave to use simd32", false)
DECLARE_IGC_REGKEY(bool, ForceLinearWalkOnLinearUAV, false, "Force linear walk on linear UAV buffer", false)
DECLARE_IGC_REGKEY(bool, ForceSupportsStaticRegSharing, false, "ForceSupportsStaticRegSharing", true)
DECLARE_IGC_REGKEY(bool, ForceSupportsAutoGRFSelection, false, "ForceSupportsAutoGRFSelection", true)
DECLARE_IGC_REGKEY(bool, EnableVRT, true, "Enable Variable Register per Thread", true)
DECLARE_IGC_REGKEY(bool, forceFullUrbWriteMask, true, "Set Full URB write mask.", false)
DECLARE_IGC_REGKEY(DWORD, RovOpt, 3,
                   "Bitmask for ROV optimizations. 0 for all off, 1 for force fence flush none, 2 for setting "
                   "LSC_L1UC_L3C_WB, 3 for both opt on",
                   false)
// DECLARE_IGC_REGKEY(bool, EnablePlatformFenceOpt,        true,  "Force fence optimization", false)
DECLARE_IGC_REGKEY(bool, EnableLSCFence, true, "Enable LSC Fence in ConvertDXIL for the device has LSC", false)
DECLARE_IGC_REGKEY(DWORD, MinCompressionThreshold, 60,
                   "Set the minimum compression threshold that is desired (100 is disabling it)", true)
DECLARE_IGC_REGKEY(bool, EnableSLMConstProp, true, "Enable SLM constant propagation (compute shader only).", false)
DECLARE_IGC_REGKEY(
    bool, EnableStatelessToStateful, true,
    "Enable Stateless To Stateful transformation for global and constant address space in OpenCL kernels", false)
DECLARE_IGC_REGKEY(bool, EnableStatefulToken, true,
                   "Enable to indicate ptr arguments are fully converted to stateful (temporary)", false)
DECLARE_IGC_REGKEY(bool, DisableConstBaseGlobalBaseArg, false,
                   "Do no generate kernel implicit arguments: constBase and globalBase", false)
DECLARE_IGC_REGKEY(bool, EnableGenUpdateCB, false, "Enable derived constant optimization.", false)
DECLARE_IGC_REGKEY(bool, EnableGenUpdateCBResInfo, false, "Enable derived constant optimization with resinfo.", false)
DECLARE_IGC_REGKEY(bool, EnableHighestSIMDForNoSpill, false,
                   "When there is no spill choose highest SIMD (compute shader only).", false)
DECLARE_IGC_REGKEY(bool, ForceAddressArithSinking, false, "Force sinking address arithmetic closer to the usage", false)
DECLARE_IGC_REGKEY(bool, SetDefaultTileYWalk, true, "Use TileY walk as default for HW generating threadID", true)
DECLARE_IGC_REGKEY(bool, ForceTileY, false, "Force TileY mode on DG2", false)
DECLARE_IGC_REGKEY(DWORD, EnableNewTileYCheck, 2, "Enable new TileY check. 0 - off, 1 - on, 2 - platform default",
                   false)
DECLARE_IGC_REGKEY(DWORD, KeepTileYForFlattened, 2,
                   "Keep TileY for FlattenedThreadIdInGroup. 0 - off, 1 - on, 2 - platform default", false)
DECLARE_IGC_REGKEY(bool, EnableSelectCSWalkOrderPass, true,
                   "Enable SelectCSWalkOrderPass at the earlier stage than PreCompile time", false)

DECLARE_IGC_REGKEY(bool, DisableDynamicTextureFolding, false, "Disable Dynamic Texture Folding", false)
DECLARE_IGC_REGKEY(bool, DisableDynamicResInfoFolding, true, "Disable Dynamic ResInfo Instruction Folding", false)
DECLARE_IGC_REGKEY(bool, DisableRectListOpt, false, "Disable Rect List optimization", false)

DECLARE_IGC_REGKEY(DWORD, EnableCodeAssumption, 1,
                   "If set (> 0), generate llvm.assume to help certain optimizations. It is OCL only for now. \
     Only 1 and 2 are valid. 2 will be 1 plus additional assumption. It also does other minor changes.",
                   false)
DECLARE_IGC_REGKEY(bool, EnableHoistMulInLoop, true, "Hoist multiply with loop invirant out of loop, FP unsafe", false)
DECLARE_IGC_REGKEY(bool, EnableGVN, true, "Enable LLVM global value numbering", false)
DECLARE_IGC_REGKEY(bool, EnableLogicalAndToBranch, true, "Enable convert logical AND to conditional branch", false)
DECLARE_IGC_REGKEY(bool, DisableMovingInstanceIDIndexOfVS, false,
                   "Disable moving index of InstanceID in VS to last location.", false)
DECLARE_IGC_REGKEY(bool, EnableSplitUnalignedVector, true, "Enable Splitting of unaligned vectors for loads and stores",
                   false)
DECLARE_IGC_REGKEY(bool, DisableFDivReassociation, false,
                   "Disable reassociation for Fdiv operations to avoid precision difference", false)
DECLARE_IGC_REGKEY(bool, EnableTrigFuncRangeReduction, false, "reduce the sin and cosing function domain range", true)
DECLARE_IGC_REGKEY(bool, EnableUnmaskedFunctions, true, "Enable unmaksed functions SYCL feature.", true)
DECLARE_IGC_REGKEY(bool, EnableStatefulAtomic, false, "Enable promoting stateless atomic to stateful atomic.", false)
DECLARE_IGC_REGKEY(bool, EnableHoistDp3, false, "Enable dp3 Hoisting.", false)
DECLARE_IGC_REGKEY(bool, ForceHoistDp3, false, "force dp3 Hoisting.", false)
DECLARE_IGC_REGKEY(bool, EnableBitcastedLoadNarrowing, false, "Enable narrowing of vector loads in bitcasts patterns.",
                   false)
DECLARE_IGC_REGKEY(bool, EnableBitcastedLoadNarrowingToScalar, false,
                   "Enable narrowing of vector loads to scalar ones in bitcasts patterns.", false)
DECLARE_IGC_REGKEY(bool, EnableOptReportLoadNarrowing, false, "Generate opt report for narrowing of vector loads.",
                   false)
DECLARE_IGC_REGKEY(bool, EnableGEPLSR, true, "Enables GEP Loop Strength Reduction pass", true)
DECLARE_IGC_REGKEY(bool, RunGEPLSRAfterLICM, false, "Runs GEP Loop Strength Reduction pass after first LICM", true)
DECLARE_IGC_REGKEY(DWORD, GEPLSRThresholdRatio, 100,
                   "Ratio for register pressure threshold in GEP Loop Strength Reduction pass", true)
DECLARE_IGC_REGKEY(bool, EnableGEPLSRToPreheader, true,
                   "Enables reduction to loop's preheader in GEP Loop Strength Reduction pass", true)
DECLARE_IGC_REGKEY(
    bool, EnableGEPLSRAnyIntBitWidth, false,
    "Enables reduction of SCEV with illegal integers. Requires legalization pass to clear up expanded code.", true)
DECLARE_IGC_REGKEY(bool, EnableGEPLSRMulExpr, true,
                   "Experimental: Enables reduction with constant, but unknown step if step contains multiplication.",
                   true)
DECLARE_IGC_REGKEY(bool, EnableGEPLSRUnknownConstantStep, false,
                   "Experimental: Enables reduction with constant, but unknown step.", true)
DECLARE_IGC_REGKEY(bool, PrintWaveClusteredInterleave, false,
                   "(Debug) Print if WaveClusteredInterleave pattern was found.", true)
DECLARE_IGC_REGKEY(DWORD, FPRoundingModeCoalescingMaxDistance, 20,
                   "Max distance in instructions for reordering FP instructions with common rounding mode", false)
DECLARE_IGC_REGKEY(bool, DisableDotAddToDp4aMerge, false, "Disable Dot and Add ops to Dp4a merge optimization.", false)
DECLARE_IGC_REGKEY(bool, DisableLoopSplitWidePHIs, false,
                   "Disable splitting of loop PHI values to eliminate subvector extract operations", false)
DECLARE_IGC_REGKEY(bool, EnableBarrierControlFlowOptimizationPass, false,
                   "Enable barrier control flow optimization pass", false)
DECLARE_IGC_REGKEY(bool, DisableBarrierSkipOptimization, false,
                   "Disable barrier skip optimization for small thread groups", false)
DECLARE_IGC_REGKEY(bool, EnableWaveShuffleIndexSinking, true,
                   "Hoist identical instructions operating on WaveShuffleIndex instructions with the same source and a "
                   "constant lane/channel",
                   false)
DECLARE_IGC_REGKEY(DWORD, WaveShuffleIndexSinkingMaxIterations, 3,
                   "Max number of iterations to run iterative WaveShuffleIndexSinking", false)
DECLARE_IGC_REGKEY(bool, EnableWaveAllJointReduction, false, "Enable Joint Reduction Optimization.", false)
DECLARE_IGC_REGKEY(bool, EnablePromoteToPredicatedMemoryAccess, false, "Enable predicated load/store if conversion.",
                   true)
DECLARE_IGC_REGKEY(bool, EnableIntDivRemIncrementReduction, true,
                   "Enable consecutive Int DivRem increment by constant optimization", false)
DECLARE_IGC_REGKEY(
    bool, DivRemIncrementCondBranchSimplify, false,
    "Create branches when simplifying consecutive udiv/urem groups increment dividend by constant greater than 1",
    false)
DECLARE_IGC_REGKEY(bool, SanitizeDivRemIncrementDivisorIsZero, false,
                   "Add ICmp comparison of divisor to zero to return -1 when performing optimization to avoid UB",
                   false)
DECLARE_IGC_REGKEY(bool, GuardDivRemIncrementDividendOverflow, false,
                   "Check for no unsigned wrap flag on increment/decrement operation before optimizing", false)
DECLARE_IGC_REGKEY(bool, EnableInstructionHoistingOptimization, false,
                   "Enable optimization for hoisting latency instructions", false)
DECLARE_IGC_REGKEY(bool, EnableResourceLoopDestLifeTimeStart, true,
                   "Enable lifetime_start set for destination in resource loop", false)
DECLARE_IGC_REGKEY(bool, EnableSinkPointerConstAdd, true,
                   "Enable sinking of pointer constant additions closer to their use", false)
DECLARE_IGC_REGKEY(bool, ForceHoistUDivURem, false,
                   "Always hoist UDiv/URem to common ancestor, even if it results in speculative execution", false)
DECLARE_IGC_GROUP("Shader debugging")
DECLARE_IGC_REGKEY(bool, CopyA0ToDBG0, false, " Copy a0 used for extended msg descriptor to dbg0 to help debug", false)
DECLARE_IGC_REGKEY(bool, CopyMsg0ToDbg0, false, " Copy msg0.2 used for Multi-Q AppQID to dbg0 to help debug", false)
DECLARE_IGC_REGKEY(bool, EnableDebugging, false, " Enable shader debugging for release internal", false)
DECLARE_IGC_REGKEY_BITMASK(
    GenerateOptionsFile, 0,
    "Create Options.txt(usually for SIMD related bugs to narrow down shaders), in the shader dump folder.",
    SHADER_TYPE_MASKS, false)
DECLARE_IGC_REGKEY(bool, ForceDisableShaderDebugHashCodeInKernel, false,
                   "Disable hash code addition to the binary after EOT", true)
DECLARE_IGC_REGKEY(bool, EnableHashMovsAtPrologue, false,
                   "Rather than after EOT, insert hash code movs at shader entry", true)
DECLARE_IGC_REGKEY(bool, ShaderDebugHashCodeInKernel, false, "Add hash code to the binary", true)
DECLARE_IGC_REGKEY(
    int, ShaderDebugHashCode, 0,
    "The driver will set a breakpoint in the first instruction of the shader which has the provided hash code.\
                                                                It works only when the value is different then 0 and SystemThreadEnable is set to TRUE.\
                                                                Ex: VS_asm2df26246434553ad_nos0000000000000000 , only the LowPart Need \
                                                                to be Enterd in Registry Ex : 0x434553ad ,i.e Lower 8 Hex Digits of the 16 Digit Hash Code \
                                                                for Compatibilty Reasons",
    false)
DECLARE_IGC_REGKEY(bool, EnableZeroSomeARF, false,
                   "If set, insert mov inst to zero a0, acc, etc to assist HW debugging.", false)
DECLARE_IGC_REGKEY(DWORD, ShaderDisableOptPassesAfter, 0,
                   "Will only run first N optimization passes, any further passes will be ignored. This flag can be "
                   "used to bisect optimization passes.",
                   false)
DECLARE_IGC_REGKEY(bool, ShaderOverride, false,
                   "Will override any LLVM shader with matching name in c:\\Intel\\IGC\\ShaderOverride", false)
DECLARE_IGC_REGKEY(bool, CompileOneAtTime, false,
                   "Compile only one kernel (out of many in llvm::module) at a time. Prints compiled kenrels names to "
                   "stdout. Useful to debug compilation time and crashes - it does not produce valid binary.",
                   false)
DECLARE_IGC_REGKEY(
    bool, SystemThreadEnable, false,
    "This key forces software to create a system thread. The system thread may still be created by software even \
                                                                if this control is set to false.The system thread is invoked if either the software requires \
                                                                exception handling or if kernel debugging is active and a breakpoint is hit.",
    false)

DECLARE_IGC_REGKEY(bool, EnableSIPOverride, false, "This key forces load of SIP from a a Local File.", false)
DECLARE_IGC_REGKEY(debugString, SIPOverrideFilePath, 0,
                   "This key when enabled with EnableSIPOverride load of SIP from a specified path.", false)
DECLARE_IGC_REGKEY(bool, DumpPayloadToScratch, false,
                   "Setting this to 1/true dumps thread payload to scartch space. Used for  workloads which doesnt use "
                   "scartch space for other purposes",
                   false)
DECLARE_IGC_REGKEY(DWORD, DebugInternalSwitch, 0, "Code pass selection, debug only", false)
DECLARE_IGC_REGKEY(bool, SToSProducesPositivePointer, false,
                   "This key is for StatelessToStateful optimization if the  user knows the pointer offset is postive "
                   "to the kernel argument.",
                   false)
DECLARE_IGC_REGKEY(bool, EnableSupportBufferOffset, false,
                   "[debugging]For StatelessToStateful optimization [OCL], support implicit buffer offset argument "
                   "(same as -cl-intel-has-buffer-offset-arg).",
                   false)
DECLARE_IGC_REGKEY(bool, EnableOptionalBufferOffset, true,
                   "For StatelessToStateful optimization [OCL], if true, make buffer offset optional. Valid only if "
                   "buffer offset is supported.",
                   true)
DECLARE_IGC_REGKEY(bool, EnableTestIGCBuiltin, false, "Enable testing igc builtin (precompiled kernels) using OCL.",
                   false)
DECLARE_IGC_REGKEY(bool, TestIGCPreCompiledFunctions, false, "Enable testing for precompiled kernels. [TEST ONLY]",
                   false)
DECLARE_IGC_REGKEY(DWORD, ForceEmuKind, 0,
                   "Force emuKind used by PreCompiledFuncImport pass. This flag takes emulation kind value that is "
                   "defined in EmuKind enum in PreCompiledFuncImport.hpp [TEST ONLY]",
                   false)
DECLARE_IGC_REGKEY(bool, EnableCSSIMD32, false,
                   "Enable computer shader SIMD32 mode, and fall back to lower SIMD when spill", false)
DECLARE_IGC_REGKEY(bool, ForceCSSIMD32, false, "Force computer shader SIMD32 mode", false)
DECLARE_IGC_REGKEY(bool, ForceCSSIMD16, false,
                   "Force computer shader SIMD16 mode if allowed, otherwise it will use SIMD32", false)
DECLARE_IGC_REGKEY(bool, ForceCSLeastSIMD, false, "Force computer shader to the lowest allowed SIMD mode", false)
DECLARE_IGC_REGKEY(bool, ForceRecompilation, false, "Force RetryManager to make recompilation", false)
DECLARE_IGC_REGKEY(DWORD, RouteByLodHint, 0, "An integer offset addon to route the resource to HDC on DG2", false)
DECLARE_IGC_REGKEY(bool, EnableTrivialEmulateSinCos, false, "Enable Emulation for Sine and Cosine instructions", false)
DECLARE_IGC_REGKEY(bool, HandlePhiNodeInChannelPrune, false,
                   "During channel prune don't stop at phinode but look at it's users.", false)
DECLARE_IGC_REGKEY(DWORD, ld2dmsInstsClubbingThreshold, 3,
                   "Do not club more than these ld2dms insts into the new BB during MCSOpt", false)
DECLARE_IGC_REGKEY(bool, Splitld2dmsAfterFirst, false,
                   "Instead of splitting after second ld2dms message, split after first to avoid waiting", false)
DECLARE_IGC_REGKEY(DWORD, ForcePerThreadPrivateMemorySize, 0,
                   "Useful for ensuring a certain amount of private memory when doing a shader override.", true)
DECLARE_IGC_REGKEY(DWORD, RetryManagerFirstStateId, 0,
                   "For debugging purposes, it can be useful to start on a particular id rather than id 0.", false)
DECLARE_IGC_REGKEY(bool, DisableSendSrcDstOverlapWA, false,
                   "Disable Send Source/destination overlap WA which is enabled for GEN10/GEN11 and whenever Wddm2Svm "
                   "is set in WATable",
                   false)
DECLARE_IGC_REGKEY(
    debugString, DisablePassToggles, 0,
    "Disable each IGC pass by setting the bit. HEXADECIMAL ONLY!. Ex: C0 is to disable pass 6 and pass 7.", false)
DECLARE_IGC_REGKEY(bool, ShaderDisplayAllPassesNames, false,
                   "Display to console all passes name with their ID and occurrence number.", false)
DECLARE_IGC_REGKEY(
    debugString, ShaderPassDisable, 0,
    "Disable specific passes eg. '9;17-19;239-;Error Check;ResolveOCLAtomics:2;Dead Code Elimination:3-5;BreakConstantExprPass:7-' \
                                                                disable pass 9, disable passes from 17 to 19, disable all passes after 238, disable all occurrences of pass Error Check, \
                                                                disable second occurrence of ResolveOCLAtomics, disable pass Dead Code Elimination occurrences from 3 to 5, \
                                                                disable all BreakConstantExprPass after his 6 occurrence \
                                                                To show a list of pass names and their occurrence set ShaderDisplayAllPassesNames.\
                                                                Must be used with ShaderDumpEnableAll flag.",
    false)
DECLARE_IGC_REGKEY(bool, PrintVerboseGenericControlFlowLog, 0,
                   "Forces compiler to print detailed log about additional control flow generated due to a presence of "
                   "generic memory operations",
                   true)
DECLARE_IGC_REGKEY(bool, ForceStatelessForQueueT, true,
                   "In OCL, force to use stateless memory to hold queue_t*. This is a legacy feature to be removed.",
                   false)
DECLARE_IGC_REGKEY(
    bool, ForceMemoryFenceBeforeEOT, false,
    "Forces inserting SLM or gloabal memory fence before EOT if shader writes to SLM or goblam memory respectively.",
    false)
DECLARE_IGC_REGKEY(bool, EnableRTmaskPso, true, "Enable render target mask optimization in PSO opt", false)
DECLARE_IGC_REGKEY(DWORD, MSAAClearedKernel, 0, "Insert the discard code for MSAA_MSC_Cleared kernels. 2/4/8/16", false)
DECLARE_IGC_REGKEY(bool, EnablerReadSuppressionWA, true, "Enable read suppression WA for the send and indirect access",
                   false)
DECLARE_IGC_REGKEY(bool, EnableLSCFenceUGMBeforeEOT, true,
                   "Enable inserting fence.ugm.06.tile before EOT if a kernel has any write to UGM [XeHPC, PVC].", true)
DECLARE_IGC_REGKEY(bool, EnableRTLSCFenceUGMBeforeEOT, true,
                   "[tmp]Enable inserting fence.ugm.06.tile before EOT for RT shader [XeHPC, PVC].", false)
DECLARE_IGC_REGKEY(bool, manualEnableRSWA, false, "Enable read suppression WA for the send and indirect access", false)
DECLARE_IGC_REGKEY(bool, DPASReadSuppressionWA, true, "Enable read suppression WA for the send and indirect access",
                   false)
DECLARE_IGC_REGKEY(bool, EnableDivergentBarrierCheck, false,
                   "Uses WIAnalysis to find barriers in divergent flow control. May have false positives.", false)
DECLARE_IGC_REGKEY(bool, EnableBitcastExtractInsertPattern, true,
                   "Enable BitcastExtractInsertPattern in CustomSafeOptPass.", true)
DECLARE_IGC_REGKEY(DWORD, ForceLoosenSimd32Occu, 2,
                   "Control loosenSimd32occu return value. 0 - off, 1 - on, 2 - platform default", false)
DECLARE_IGC_REGKEY(bool, ForceFunctionsToNop, false,
                   "Replace functions with immediate return to help narrow down shaders; use with Options.txt.", false)
DECLARE_IGC_REGKEY(bool, DisableWarnings, false, "Disable all warnings generated from IGC compiler", true)
DECLARE_IGC_REGKEY(bool, DisableDuplicateWarnings, true, "Limit duplicate warnings to a single occurrence", true)

DECLARE_IGC_GROUP("Shader dumping")
DECLARE_IGC_REGKEY(bool, EnableCosDump, false, "Enable cos dump", true)
DECLARE_IGC_REGKEY(bool, EnableCisDump, false, "Enable cis dump", true)
DECLARE_IGC_REGKEY(bool, DumpLLVMIR, false, "dump LLVM IR", true)
DECLARE_IGC_REGKEY(bool, QualityMetricsEnable, false, "Enable Quality Metrics for IGC", true)
DECLARE_IGC_REGKEY(bool, ShaderDumpEnable, false, "dump LLVM IR, visaasm, and GenISA", true)
DECLARE_IGC_REGKEY(bool, ShaderDumpEnableAll, false, "dump all LLVM IR passes, visaasm, and GenISA", true)
DECLARE_IGC_REGKEY(DWORD, ShaderDumpEnableG4, false,
                   "same as ShaderDumpEnable but adds G4 dumps (0 = off, 1 = some, 2 = all)", 0)
DECLARE_IGC_REGKEY(DWORD, ShaderDumpEnableIGAJSON, false,
                   "adds IGA JSON output to shader dumps (0 = off, 1 = enabled, 2 = include def/use info but causes "
                   "longer compile times)",
                   0)
DECLARE_IGC_REGKEY(bool, ShaderDumpEnableRAMetadata, false, "adds RA Metadata file to shader dumps", true)
DECLARE_IGC_REGKEY(
    bool, ShaderDumpInstNamer, false,
    "dump all unnamed LLVM IR instruction with variable names 'tmp' which makes easier for shaderoverriding", true)
DECLARE_IGC_REGKEY(debugString, ShaderDumpRegexFilter, 0, "Only dump files matching the given regex", true)
DECLARE_IGC_REGKEY_ENUM(ShaderDumpCollisionMode, 0, "What to do when file collision happens", FILENAME_COLLISION_MODES,
                        true)
DECLARE_IGC_REGKEY(bool, DumpZEInfoToConsole, false, "Dump zeinfo to console", true)
DECLARE_IGC_REGKEY(debugString, ProgbinDumpFileName, 0,
                   "Specify filename to use for dumping progbin file to current dir", true)
DECLARE_IGC_REGKEY(bool, ElfDumpEnable, false, "dump ELF file", true)
DECLARE_IGC_REGKEY(bool, ElfTempDumpEnable, false, "dump temporary ELF files", true)
DECLARE_IGC_REGKEY(bool, SpvAsmDumpEnable, false, "Dump spvasm file", true)
DECLARE_IGC_REGKEY(
    debugString, DebugDumpNamePrefix, 0,
    "Set a prefix to debug info dump filenames(with path) and drop hash info from them (for testing purposes)", true)
DECLARE_IGC_REGKEY(bool, ShowFullVectorsInShaderDumps, false,
                   "print all elements of vectors in ShaderDumps, can dramatically increase ShaderDumps size", true)
DECLARE_IGC_REGKEY(bool, PrintHexFloatInShaderDumpAsm, true, "print floats in hex in asm dump", true)
DECLARE_IGC_REGKEY(bool, PrintInstOffsetInShaderDumpAsm, false, "print instruction offsets as comments in asm dump",
                   true)
DECLARE_IGC_REGKEY(debugString, PrintAfter, 0,
                   "Take either all or comma/semicolon-separated list of pass names. If set, enable print LLVM IR "
                   "after the given pass is done (mimic llvm print-after)",
                   true)
DECLARE_IGC_REGKEY(debugString, PrintBefore, 0,
                   "Take either all or comma/semicolon-separated list of pass names. If set, enable print LLVM IR "
                   "before the given pass is done (mimic llvm print-before)",
                   true)
DECLARE_IGC_REGKEY(bool, PrintMDBeforeModule, false,
                   "Print metadata of the module at the beginning of the dump. Used for LIT tests.", true)
DECLARE_IGC_REGKEY(bool, DumpUseShorterName, true, "If set, use an internal shader name(_entry_id) in dump file name",
                   true)
DECLARE_IGC_REGKEY(bool, EnableKernelNamesBasedHash, false,
                   "If set, use kernels' names to calculate the hash. Doesn't work on .cl dump's hash. Will overwrite "
                   "dumps if multiple modules have the same kernel names.",
                   false)
DECLARE_IGC_REGKEY(bool, InterleaveSourceShader, true, "Interleave the source shader in asm dump", true)
DECLARE_IGC_REGKEY(bool, ShaderDumpPidDisable, false, "disabled adding PID to the name of shader dump directory", true)
DECLARE_IGC_REGKEY(bool, DumpToCurrentDir, false, "dump shaders to the current directory", true)
DECLARE_IGC_REGKEY(debugString, DumpToCustomDir, 0, "Dump shaders to custom directory. Parent directory must exist.",
                   true)
DECLARE_IGC_REGKEY(bool, EnableShaderNumbering, false,
                   "Number shaders in the order they are dumped based on their hashes", true)
DECLARE_IGC_REGKEY(bool, PrintToConsole, false, "dump to console", true)
DECLARE_IGC_REGKEY(bool, EnableCapsDump, false, "Enable hardware caps dump", true)
DECLARE_IGC_REGKEY(bool, EnableLivenessDump, false, "Enable dumping out liveness info on stderr.", true)
DECLARE_IGC_REGKEY(DWORD, ForceRPE, 0, "Force RPE (RegisterEstimator) computation if > 0. If 2, force RPE per inst.",
                   true)
DECLARE_IGC_REGKEY(DWORD, RPEDumpLevel, 0,
                   "> 0 : dump info of register pressure estimate on stderr. See igc_flags.hpp level defs.", false)
DECLARE_IGC_REGKEY(bool, DumpVariableAlias, false, "Dump variable alias info, valid if EnableVariableAlias is on", true)
DECLARE_IGC_REGKEY(bool, DumpResourceLoop, false, "dump resource loop detected by ResourceLoopAnalysis", true)
DECLARE_IGC_REGKEY(bool, DumpDeSSA, false, "dump DeSSA info into file.", true)
DECLARE_IGC_REGKEY(bool, DumpWIA, false, "dump WI (uniform) infomation into files in dump directory if set to true",
                   false)
DECLARE_IGC_REGKEY(bool, EnableScalarizerDebugLog, false, "print step by step scalarizer debug info.", true)
DECLARE_IGC_REGKEY(bool, DumpTimeStats, false, "Timing of translation, code generation, finalizer, etc", true)
DECLARE_IGC_REGKEY(bool, DumpTimeStatsCoarse, false,
                   "Only collect/dump coarse level time stats, i.e. skip opt detail timer for now", true)
DECLARE_IGC_REGKEY(bool, DumpTimeStatsPerPass, false, "Collect Timing of IGC/LLVM passes", true)
DECLARE_IGC_REGKEY(bool, DumpHasNonKernelArgLdSt, false, "Print if hasNonKernelArg load/store to stderr", true)
DECLARE_IGC_REGKEY(bool, PrintPsoDdiHash, true, "Print psoDDIHash in TimeStats_Shaders.csv file", true)
DECLARE_IGC_REGKEY(bool, ShaderDataBaseStats, false, "Enable gathering sends' sizes for shader statistics", false)
DECLARE_IGC_REGKEY(bool, ShaderSendInfoRework, false, "Temporary Regkey for reworking sendinfo", false)
DECLARE_IGC_REGKEY(bool, DumpLoopSink, false, "Dump debug info in LoopSink", false)
DECLARE_IGC_REGKEY(DWORD, LoopSinkDumpLevel, 1, "1, 2 or 3: Dump loop sink with the needed verbosity", false)
DECLARE_IGC_REGKEY(
    debugString, ShaderDataBaseStatsFilePath, 0,
    "Path to a file with dumped shader stats additional data e.g. data available during compilation only", false)
DECLARE_IGC_REGKEY(bool, EnableRemarks, false, "Enable remark for Divergent Barrier", false)
DECLARE_IGC_REGKEY(bool, AddExtraIntfInfo, false,
                   "Will add extra inteference info from .extraintf files from c:\\Intel\\IGC\\ShaderOverride", false)

DECLARE_IGC_GROUP("Debugging features")
DECLARE_IGC_REGKEY(debugString, ForceSpillVariables, 0,
                   "comma-separated string, each provide the declare id of variable which will be spilled", true)
DECLARE_IGC_REGKEY(debugString, ForceAssignRhysicalReg, 0, "Force assigning dclId to phyiscal reg.", true)
DECLARE_IGC_REGKEY(bool, InitializeUndefValueEnable, false,
                   "Setting this to 1/true initializes all undefs in URB payload to 0", false)
DECLARE_IGC_REGKEY(
    bool, InitializeRegistersEnable, false,
    "Setting this to 1/true initializes all GRFs, Flag and address registers to 0 at the beginning of the shader",
    false)
DECLARE_IGC_REGKEY(bool, InitializeAddressRegistersBeforeUse, false,
                   "Setting this to 1 (true) initializes address register to 0 before each use", false)
DECLARE_IGC_REGKEY(bool, AvoidUsingR0R1, false, "Do not use r0 and r1 as generic usage registers", false)
DECLARE_IGC_REGKEY(bool, EnableRelocations, false,
                   "Setting this to 1 (true) makes IGC emit relocatable ELF with debug info", true)
DECLARE_IGC_REGKEY(bool, EnableWriteOldFPToStack, true,
                   "Setting this to 1 (true) writes the caller frame's frame-pointer to the start of callee's frame on "
                   "stack, to support stack walk",
                   false)
DECLARE_IGC_REGKEY(bool, ZeBinCompatibleDebugging, true,
                   "Setting this to 1 (true) enables embed debug info in zeBinary", true)
DECLARE_IGC_REGKEY(bool, DebugInfoEnforceAmd64EM, false,
                   "Enforces elf file with the debug infomation to have eMachine set to AMD64", false)
DECLARE_IGC_REGKEY(bool, DebugInfoValidation, false,
                   "Enable optional (strict) checks to detect debug information inconsistencies", false)
DECLARE_IGC_REGKEY(bool, deadLoopForFloatException, false, "enable a dead loop if float exception happened", false)
DECLARE_IGC_REGKEY(bool, EnableIEEEFloatExceptionTrap, false, "Enable CR0 IEEE float exception trap bit", true)
DECLARE_IGC_REGKEY(debugString, ExtraOCLOptions, 0, "Extra options for OpenCL", true)
DECLARE_IGC_REGKEY(debugString, ExtraOCLInternalOptions, 0, "Extra internal options for OpenCL", true)
DECLARE_IGC_REGKEY(bool, UseVISAVarNames, false,
                   "Make VISA generate names for virtual variables so they match with dbg file", true)
DECLARE_IGC_REGKEY(DWORD, MetricsDumpEnable, 0, "Dump IGC Metrics to file *.optrpt in current working directory.\
                                                                Setting to 0 - disabled, 1 - makes in binary format, 2 - makes in plain-text format.",
                   true)
DECLARE_IGC_REGKEY(bool, PrintDebugSettings, false, "Prints all non-default debug settings", false)
DECLARE_IGC_REGKEY(bool, UseMTInLLD, false, "Use multi-threading when linking multiple elf files", true)
DECLARE_IGC_REGKEY(bool, NoCatchAllDebugLine, false,
                   "Don't emit special placeholder instruction to map VISA orphan instructions", false)
DECLARE_IGC_REGKEY(bool, EnableTestSplitI64, false,
                   "Test legalization that split i64 store unnecessarily, to be deleted once test is done[temp]", true)
DECLARE_IGC_REGKEY(bool, ShaderDumpTranslationOnly, false,
                   "Dump LLVM IR right after translation from SPIRV to stderr and ignore all passes", false)
DECLARE_IGC_REGKEY(bool, UseVMaskPredicate, false, "Use VMask as predicate for subspan usage", false)
DECLARE_IGC_REGKEY(bool, UseVMaskPredicateForLoads, true, "Use VMask as predicate for subspan usage (loads only)", true)
DECLARE_IGC_REGKEY(bool, UseVMaskPredicateForIndirectMove, true,
                   "Use VMask as predicate for subspan usage (indirect mov only)", true)
DECLARE_IGC_REGKEY(bool, StackOverflowDetection, false,
                   "Inserts checks for stack overflow when stack calls or VLAs are used. See documentation: "
                   "documentation/igc/StackOverflowDetection/StackOverflowDetection.md",
                   true)
DECLARE_IGC_REGKEY(bool, Disable512GRFISA, false, "Disable 512GRF ISA", true)
DECLARE_IGC_REGKEY(bool, BufferBoundsChecking, false, "Setting this to 1 (true) enables buffer bounds checking", true)
DECLARE_IGC_REGKEY(DWORD, MinimumValidAddress, 0,
                   "If it's greater than 0, it enables minimal valid address checking where the threshold is the given "
                   "value (in hex).",
                   true)
DECLARE_IGC_REGKEY(bool, AssignZeroToUndefPhiNodes, false,
                   "Assigns a null value to such a phi node which has an undefined value during emitting vISA", false)
DECLARE_IGC_REGKEY(bool, DisableMovOfUndefPhiSources, false, "Do not emit VISA mov instructions for undef PHI sources",
                   false)
DECLARE_IGC_REGKEY(bool, DisableMovOfUndefPhiVectorSources, true,
                   "Do not emit VISA mov instructions for undef PHI vector-type sources", false)
DECLARE_IGC_REGKEY_ENUM(InjectPrintfFlag, 0, "Inject printf debugging flag", INJECT_PRINTF_OPTIONS, true)
DECLARE_IGC_REGKEY(DWORD, AdHoc, 0,
                   "Unassigned debug key that can be used for experiments. Do not commit usages of this regkey", false)

DECLARE_IGC_GROUP("IGC Features")
DECLARE_IGC_REGKEY(bool, EnableOCLSIMD16, true, "Enable OCL SIMD16 mode", true)
DECLARE_IGC_REGKEY(bool, EnableOCLSIMD32, true, "Enable OCL SIMD32 mode", true)
DECLARE_IGC_REGKEY(DWORD, ForceOCLSIMDWidth, 0,
                   "Force using SIMD width specified. 0 : no forcing. This overrides driver forced SIMD value(if any) "
                   "and runtime behaviour could be different if driver expects something fixed",
                   true)
DECLARE_IGC_REGKEY(bool, SendMultipleSIMDModesCS, true, "Send multiple SIMD modes for CS", false)
DECLARE_IGC_REGKEY(DWORD, OCLSIMD16SelectionMask, 6, "Select SIMD 16 heuristics. Valid values are 0, 1, 2 and 3", false)
DECLARE_IGC_REGKEY(bool, EnableHSSinglePatchDispatch, false,
                   "Setting this to 1/true enables SIMD8 single-patch dispatch in HullShader. Default is either SIMD8 "
                   "single patch/dual patch dispatch based on control point count",
                   false)
DECLARE_IGC_REGKEY(bool, DisableGPGPUIndirectPayload, false, "Disable OCL indirect GPGPU payload", false)
DECLARE_IGC_REGKEY(bool, DisableDSDualPatch, false,
                   "Setting it to true with enable Single and Dual Patch dispatch mode for Domain Shader", false)
DECLARE_IGC_REGKEY(bool, DisableMemOpt, false, "Disable MemOpt, merging load/store", true)
DECLARE_IGC_REGKEY(DWORD, MemOptGEPCanon, 2,
                   "[test] GEP canonicalization in MemOpt. 0 : enable; 1: disable; 2: disable only for OCL;", true)
DECLARE_IGC_REGKEY(bool, DisableMemOpt2, false, "Disable MemOpt2", false)
DECLARE_IGC_REGKEY(
    bool, EnableExplicitCopyForByVal, true,
    "Enable generating an explicit copy (alloca + memcpy) in a caller for aggregate argumentes with byval attribute",
    true)
DECLARE_IGC_REGKEY(DWORD, EnableLdStCombine, 1,
                   "Enable load/store combine pass if set to 1 (lsc message only) or 2; bit 3 = 1 [tmp for testing] : "
                   "enabled load combine (intend to replace memopt)",
                   true)
DECLARE_IGC_REGKEY(bool, EnableLdStCombinewithDummyLoad, false,
                   "Adds extra load instruction to increase the size of coalesced load", true)
DECLARE_IGC_REGKEY(DWORD, MaxStoreVectorSizeInBytes, 0,
                   "[LdStCombine] the max non-uniform vector size for the coalesced store. 0: compiler choice "
                   "(default, 16(4DW)); others: 4/8/16/32",
                   true)
DECLARE_IGC_REGKEY(DWORD, MaxLoadVectorSizeInBytes, 0,
                   "[LdStCombine] the max non-uniform vector size for the coalesced load.  0: compiler choice "
                   "(default, 16(4DW)); others: 4/8/16/32",
                   true)
DECLARE_IGC_REGKEY(bool, DisableMergeStore, false,
                   "[temp]If EnableLdStCombine is on, disable mergestore (memopt) if this is set. Temp key for testing",
                   true)
DECLARE_IGC_REGKEY(DWORD, MaxLiveOutThreshold, 0, "Max LiveOut Threshold in MemOpt2", false)
DECLARE_IGC_REGKEY(bool, DisableScalarAtomics, false, "Disable the Scalar Atomics optimization", false)
DECLARE_IGC_REGKEY(bool, EnableScalarTypedAtomics, true, "Enable the Scalar Typed Atomics optimization", false)
DECLARE_IGC_REGKEY(bool, EnableScalarPhisMerger, true,
                   "enable optimization that merges scalar phi nodes into vector ones", true)
DECLARE_IGC_REGKEY(bool, EnableVectorizer, true, "Enable IGCVectorizer pass", true)
DECLARE_IGC_REGKEY(DWORD, VectorizerDepWindowMultiplier, 6,
                   "Multiplier for the slice size to account for vectorizer dependency check window", true)
DECLARE_IGC_REGKEY(bool, VectorizerCheckScalarizer, false, "Add scalariser after vectorizer to check performance", true)
DECLARE_IGC_REGKEY(DWORD, VectorizerList, -1, "Vectorize only one seed instruction with the provided number", true)
DECLARE_IGC_REGKEY(bool, EnableVectorEmitter, true, "Enable Vector Emission for a vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowI32, true, "Allow I32 versions of instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowFPTRUNC, true, "Allow FPTRUNC instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowFDIV, true, "Allow FDIV instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowFMUL, true, "Allow FMUL instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowFADD, true, "Allow FADD instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowFSUB, true, "Allow FSUB instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowEXP2, true, "Allow EXP2 instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowMAXNUM, true, "Allow MAXNUM instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowWAVEALL, true, "Allow WAVEALL instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowWAVEALLJoint, true, "Allow WAVEALL instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowCMP, true, "Allow CMP instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowUniformCMP, true, "Allow CMP instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowSelect, true, "Allow Select instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowUniformSelect, true, "Allow Select instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowSamePredSelect, false,
                   "Allow Select instructions with identical predicate inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowFMADMatching, true,
                   "Allow FADD and FMUL instructions to be matched later in the pattern match pass", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowMUL, true, "Allow MUL instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowADD, true, "Allow ADD instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerAllowSUB, true, "Allow SUB instructions inside vectorizer", true)
DECLARE_IGC_REGKEY(bool, VectorizerUniformValueVectorizationEnabled, true,
                   "Vector Emitter emits vectorized instruction for uniform values", true)
DECLARE_IGC_REGKEY(
    bool, VectorizerEnablePartialVectorization, true,
    "Not fully tested option, allows to substitute scalar part with partially vectorized through extract elements",
    true)
DECLARE_IGC_REGKEY(bool, DisableOCLScalarizer, false, "Disable ScalarizeFunction pass in OCL pipeline", true)
DECLARE_IGC_REGKEY(bool, DisablePHIScalarization, false, "Disable scalarization of PHINode instructions", true)
DECLARE_IGC_REGKEY(bool, EnableSelectiveScalarizer, false, "enable selective scalarizer on GPGPU path", true)
DECLARE_IGC_REGKEY(bool, HoistPSConstBufferValues, true,
                   "Hoists up down converts for contant buffer accesses, so they an be vectorized more easily.", false)
DECLARE_IGC_REGKEY(bool, EnableSingleVertexDispatch, false, "Vertex Shader Single Patch Dispatch Regkey", false)
DECLARE_IGC_REGKEY(bool, allowLICM, true, "Enable LICM in IGC.", true)
DECLARE_IGC_REGKEY(bool, allowDecompose2DBlockFuncs, true, "Enable decomposition of 2D block intrinsics in IGC.", true)
DECLARE_IGC_REGKEY(bool, allowImmOff2DBlockFuncs, false,
                   "Allow compiler to decide to use immediate offsets in 2D block intrinsics in IGC.", false)
DECLARE_IGC_REGKEY(DWORD, CSSpillThresholdSLM, 0, "Spill Threshold for CS SIMD16 with SLM", false)
DECLARE_IGC_REGKEY(DWORD, CSSpillThresholdNoSLM, 5, "Spill Threshold for CS SIMD16 without SLM", false)
DECLARE_IGC_REGKEY(DWORD, AllowedSpillRegCount, 0, "Max allowed spill size without recompile", false)
DECLARE_IGC_REGKEY(DWORD, CSSpillThreshold2xGRFRetry, 3500, "Spill Threshold for CS to trigger 2xGRFRetry", false)
DECLARE_IGC_REGKEY(DWORD, LICMStatThreshold, 70, "LICM stat threshold to avoid retry SIMD16 for CS", false)
DECLARE_IGC_REGKEY(bool, EnableTypeDemotion, true, "Enable Type Demotion", false)
DECLARE_IGC_REGKEY(bool, EnablePreRARematFlag, true, "Enable PreRA Rematerialization of Flag", false)
DECLARE_IGC_REGKEY(bool, EnableGASResolver, true, "Enable GAS Resolver", false)
DECLARE_IGC_REGKEY(bool, EnableLowerGPCallArg, true, "Enable pass to lower generic pointers in function arguments",
                   false)
DECLARE_IGC_REGKEY(bool, EnableGenericCastToPtrOpt, true,
                   "Enable simplification of GenericCastToPtrExplicit_ToGlobal calls", false)
DECLARE_IGC_REGKEY(bool, DisableRecompilation, false, "Disable recompilation, skip retry stage", true)
DECLARE_IGC_REGKEY(bool, SampleMultiversioning, false,
                   "Create branches aroung samplers which can be redundant with some values", false)
DECLARE_IGC_REGKEY(bool, EnableSMRescheduling, false,
                   "Change instruction order to enable extra Sample Multiversioning cases", false)
DECLARE_IGC_REGKEY(bool, DisableEarlyOutPatterns, false,
                   "Disable optimization trying to create an early out after sampleC messages", false)
DECLARE_IGC_REGKEY_BITMASK(EarlyOutPatternSelectPS, 0xff, "Each bit selects a pattern match to enable/disable.",
                           EARLY_OUT_PS_PATTERNS, false)
DECLARE_IGC_REGKEY_BITMASK(EarlyOutPatternSelectCS, 0xff, "Each bit selects a pattern match to enable/disable.",
                           EARLY_OUT_CS_PATTERNS, false)
DECLARE_IGC_REGKEY(bool, OCLEnableReassociate, false, "Enable reassociation", true)
DECLARE_IGC_REGKEY(bool, EnableOCLScratchPrivateMemory, true,
                   "Enable the use of scratch space for private memory [OCL only]", true)
DECLARE_IGC_REGKEY(bool, EnableMaxWGSizeCalculation, true, "Enable max work group size calculation [OCL only]", true)
DECLARE_IGC_REGKEY(bool, Enable64BitEmulation, false, "Enable 64-bit emulation", false)
DECLARE_IGC_REGKEY(bool, Enable64BitEmulationOnSelectedPlatform, true, "Enable 64-bit emulation on selected platforms",
                   false)
DECLARE_IGC_REGKEY(DWORD, EnableConstIntDivReduction, 0x1,
                   "Enables strength reduction on integer division/remainder with constant divisors/moduli", true)
DECLARE_IGC_REGKEY(DWORD, EnableIntDivRemCombine, 0x0,
                   "Given div/rem pairs with same operands merged; replace rem with mul+sub on quotient; 0x3 (set "
                   "bit[1]) forces this on constant power of two divisors as well",
                   true)
DECLARE_IGC_REGKEY(bool, EnableHFpacking, false, "Enable HF packing", false)
DECLARE_IGC_REGKEY(bool, Force32BitIntDivRemEmu, false,
                   "Force 32-bit Int Div/Rem emulation using fp64, ignored if no native fp64 support", true)
DECLARE_IGC_REGKEY(
    bool, Force32BitIntDivRemEmuSP, false,
    "Force 32-bit Int Div/Rem emulation using fp32, ignored if Force32BitIntDivRemEmu is set and actually used", true)
DECLARE_IGC_REGKEY(bool, EnableMullh, true, "Enable i32 mul in SAO layout", true)
DECLARE_IGC_REGKEY(bool, EnableNativeFP32LocalAtomicAdd, true, "Enable native fp32 local atomic add", true)
DECLARE_IGC_REGKEY(bool, EnableNativeTanh, true, "Enable native tanh instruction", true)
DECLARE_IGC_REGKEY(bool, EnableFP64Dpas, false, "Enable fp64 dpas", true)
DECLARE_IGC_REGKEY(bool, EnableFP4Dpas, true, "Enable fp4 dpas", true)
DECLARE_IGC_REGKEY(bool, EnableOutOfBoundsBuiltinChecks, true, "Enable extra checks for OOB in builtins", true)
DECLARE_IGC_REGKEY(bool, EnableNativeSinCos, true, "Enable native sin and cos", true)
DECLARE_IGC_REGKEY(bool, EnableRecursionOpenCL, true, "Enable recursion with OpenCL user functions", false)
DECLARE_IGC_REGKEY(bool, ForceDPEmulation, false, "Force double emulation for testing purpose", false)
DECLARE_IGC_REGKEY(
    bool, EnableDPEmulation, false,
    "Enforce double precision floating point operations emulation on platforms that do not support it natively", true)
DECLARE_IGC_REGKEY(bool, DPEmuNeedI64Emu, true,
                   "Double Emulation needs I64 emulation. Unsetting it to disable I64 Emulation for testing.", false)
DECLARE_IGC_REGKEY(bool, ForceDisableDPToHFConvEmu, false,
                   "Force the compiler to disable an emulation for the conversion from fp64 to fp16 (use a native "
                   "(inaccurate) operations instead - fp64 to fp32 and then fp32 to fp16)",
                   false)
DECLARE_IGC_REGKEY(bool, SelectiveLoopUnrollForDPEmu, true,
                   "Setting this to 0/false disable selective loop unrolling for DP emu.", true)
DECLARE_IGC_REGKEY(bool, ForceSPDivEmulation, false, "Force SP Div emulation for testing purpose", false)
DECLARE_IGC_REGKEY(
    DWORD, ForceI64DivRemEmu, 0,
    "Forces specific int64 div/rem emulation: 0 = platform default, 1 = int based, 2 = SP based, 3 = DP based", false)
DECLARE_IGC_REGKEY(bool, EnableGen11TwoStackTSG, false, "Enable Two stack TSG gen11 feature", false)
DECLARE_IGC_REGKEY(bool, Enable16BitLDMCS, true, "Enable 16-bit ld_mcs on supported platforms", true)
DECLARE_IGC_REGKEY(bool, EnableDualSIMD8, true, "enable dual SIMD8 on supported platforms", true)
DECLARE_IGC_REGKEY(bool, ForceSampleDEmulation, false, "Enable emulation of sample_d on pre-XeHP platforms.", true)
DECLARE_IGC_REGKEY(bool, RemoveLegacyOCLStatelessPrivateMemoryCases, false,
                   "Remove cases where OCL uses stateless private memory. XeHP and above only! [OCL only]", true)
DECLARE_IGC_REGKEY(DWORD, SkipPsSimdWithDualSimd, 1,
                   "Setting it to values def in igc.h will force SIMD mode to skip if the dual-SIMD8 kernel exists",
                   true)
DECLARE_IGC_REGKEY(bool, EnablePostCullPatchFIFOLP, true, "Enable Post-Cull Patch Decoupling FIFO. GEN12LP.", true)
DECLARE_IGC_REGKEY(bool, EnablePostCullPatchFIFOHP, true, "Enable Post-Cull Patch Decoupling FIFO. XeHP.", true)
DECLARE_IGC_REGKEY(bool, EnableAIParameterCombiningWithLODBias, true,
                   "Enable AI parameter combining With LOD Bias parameter. XeHP", true)
DECLARE_IGC_REGKEY(DWORD, ForceMeshShaderSimdSize, 0, "Force mesh shader simd size,\
                                                                valid values are 0 (not set), 8, 16 and 32\
                                                                ignored if produces invalid cofiguration, e.g. simd size too small for workgroup size",
                   true)
DECLARE_IGC_REGKEY(DWORD, ForceTaskShaderSimdSize, 0, "Force task shader simd size,\
                                                                valid values are 0 (not set), 8, 16 and 32\
                                                                ignored if produces invalid cofiguration, e.g. simd size too small for workgroup size",
                   true)
DECLARE_IGC_REGKEY(DWORD, EnableMeshShaderSimdSize, 0, "Set allowed simd sizes for mesh shader compilation,\
                                                                bitmask bit0 - simd8, bit1 - simd16, bit2 - simd32,\
                                                                e.g. 0x7 enables all simd sizes and 0x2 enables only simd16,\
                                                                valid values are from 0 to 7\
                                                                ignored if produces invalid cofiguration, e.g. simd size too small for workgroup size,\
                                                                ignored if ForceMeshShaderSimdSize is set",
                   true)
DECLARE_IGC_REGKEY(DWORD, EnableTaskShaderSimdSize, 0, "Set allowed simd sizes for task shader compilation,\
                                                                bitmask bit0 - simd8, bit1 - simd16, bit2 - simd32,\
                                                                e.g. 0x7 enables all simd sizes and 0x2 enables only simd16,\
                                                                valid values are from 0 to 7\
                                                                ignored if produces invalid cofiguration, e.g. simd size too small for workgroup size,\
                                                                ignored if ForceMeshShaderSimdSize is set",
                   true)
DECLARE_IGC_REGKEY(DWORD, EnableMeshSLMCache, 0, "Enables caching Mesh shader outputs in SLM,\
                                                                bitmask:\
                                                                bit0 - cache AND flush mode, enable caching of Primitive Count and Primitive Indices, \
                                                                bit1 - cache AND flush mode, enable caching of per-vertex outputs,\
                                                                bit2 - cache AND flush mode, enable caching of per-primitive outputs,\
                                                                bit3 - mirror mode, if this bit is set bits 0, 1 and 2 are ignored, \
                                                                       enable caching of outputs that are read in the shader\
                                                                       data is only mirrored in SLM",
                   true)
DECLARE_IGC_REGKEY(bool, DisableShrinkArrayAllocaPass, false, "Disables ShrinkArrayAllocaPass", true)
DECLARE_IGC_REGKEY(bool, DisableAddRequiredMemoryFencesPass, false, "Disables AddRequiredMemoryFencesPass", true)
DECLARE_IGC_REGKEY(bool, EnableL3FlushForGlobal, false, "Enable/disable flushing L3 cache for globals", false)
DECLARE_IGC_REGKEY(bool, EnableCPSOmaskWA, true, "Enable workaround for oMask with CPS", false)
DECLARE_IGC_REGKEY(bool, EnableCPSMSAAOMaskWA, false,
                   "Enable WA which forces rt writes to happen at pixel rate when cps, msaa, and omask are present.",
                   true)
DECLARE_IGC_REGKEY(bool, EnableSampleBMLODWA, true,
                   "Enable workaround for sample_b messages that use the mlod parameter", false)
DECLARE_IGC_REGKEY(bool, EnableFallbackToBindless, true, "This key enables fallback to bindless mode on all shaders",
                   false)
DECLARE_IGC_REGKEY(bool, EnableFallbackToStateless, true, "This key enables fallback to stateless mode on all shaders",
                   false)
DECLARE_IGC_REGKEY(bool, DisablePromoteToDirectAS, false, "This key disables the PromoteResourceToDirectAS pass", false)
DECLARE_IGC_REGKEY(bool, EnableAdvCodeMotion, true, "Enable advanced code motion", false)
DECLARE_IGC_REGKEY(bool, AdvCodeMotionControl, true, "Control bits to fine-tune advanced code motion", false)
DECLARE_IGC_REGKEY(bool, EnableAdvRuntimeUnroll, true, "Enable advanced runtime unroll", false)
DECLARE_IGC_REGKEY(DWORD, AdvRuntimeUnrollCount, 0, "Advanced runtime unroll count", false)
DECLARE_IGC_REGKEY(bool, EnableAdvMemOpt, true, "Enable advanced memory optimization", false)
DECLARE_IGC_REGKEY(bool, UniformMemOpt4OW, false, "increase uniform memory optimization from 2 owords to 4 owords",
                   true)
DECLARE_IGC_REGKEY(bool, EnableFunctionPointer, true, "Enables support for function pointers and indirect calls", false)
DECLARE_IGC_REGKEY(bool, EnableIndirectCallOptimization, true,
                   "Enables inlining indirect calls by comparing function addresses", false)
DECLARE_IGC_REGKEY(bool, EnableSIMDVariantCompilation, false, "Enables compiling kernels in variant SIMD sizes", false)
DECLARE_IGC_REGKEY(bool, ForceFFIDOverwrite, false, "Force overwriting ffid in sr0.0", false)
DECLARE_IGC_REGKEY(bool, EnableReadGTPinInput, true,
                   "Enables setting GTPin context flags by reading the input to the compiler adapters", false)
DECLARE_IGC_REGKEY(bool, ForceStaticToDynamic, false, "Force write of vertex count in GS", false)
DECLARE_IGC_REGKEY(bool, DisableWaSampleLZ, false, "Disable The Sample Lz workaround and generate Sample LZ", false)
DECLARE_IGC_REGKEY(DWORD, OverrideRevIdForWA, 0xff,
                   "Enable this to override the stepping/RevId, default is a0 = 0, b0 = 1, c0 = 2, so on...", false)
DECLARE_IGC_REGKEY(DWORD, OverrideDeviceIdForWA, 0, "Enable this to override DeviceId ", false)
DECLARE_IGC_REGKEY(DWORD, OverrideProductFamilyForWA, 0,
                   "Enable this to override the product family, get the correct enum from igfxfmid.h", false)
DECLARE_IGC_REGKEY(
    bool, EnableImplicitArgAsIntrinsic, true,
    "Use GenISAIntrinsic instructions for supported implicit args instead of passing them as function arguments", true)
DECLARE_IGC_REGKEY(bool, EnableSamplerSupport, false, "Enables sampler messages generation for PVC.", true)
DECLARE_IGC_REGKEY(bool, EnableLSC, false, "Enables the new dataport encoding for LSC messages.", true)
DECLARE_IGC_REGKEY(bool, ForceNoLSC, false, "Disables the new dataport encoding for LSC messages.", true)
DECLARE_IGC_REGKEY(bool, EnableMadLoopSlice, true, "Enables the slicing of mad loops.", true)
DECLARE_IGC_REGKEY(bool, EnableGEPSimplification, true, "Enable GEP simplification", true)
DECLARE_IGC_REGKEY(bool, TestGEPSimplification, false,
                   "[Test] Testing GEP simplification without actually lowering GEP. Used in lit test", false)
DECLARE_IGC_REGKEY(bool, DisableSystemMemoryCachingInGPUForConstantBuffers, false,
                   "Disables caching system memory in GPU for loads from constant buffers", false)
DECLARE_IGC_REGKEY(bool, EnableInsertingPairedResourcePointer, true,
                   "Enable to insert a bindless paired resource address into sampler headers in context of sampling "
                   "feedback resources",
                   true)
DECLARE_IGC_REGKEY(
    bool, EnablePromotionToSampleMlod, true,
    "Enables promotion of sample and sample_c to sample_mlod and sample_c_mlod instructions when min lod is present",
    false)
DECLARE_IGC_REGKEY(bool, DisableCorrectlyRoundedMacros, false,
                   "Tmp flag to disable correcly rounded macros for BMG+. This flag will be removed in the future.",
                   false)
DECLARE_IGC_REGKEY_ENUM(EnableLscSamplerRouting, -1,
                        "Enables conversion of LD to LD_L instructions. Xe2+"
                        "-1 - Platform default"
                        " 0 - Force enable conversion to LD_L. Disallow loads via LSC"
                        " 1 - Force disable conversion to LD_L. Allow loads via LSC",
                        TRIBOOL_OPTIONS, false)
DECLARE_IGC_REGKEY(bool, EnableSIMD16ForXe2, false, "Enable CS SIMD16 for Xe2", false)
DECLARE_IGC_REGKEY(bool, EnableSIMD16ForNonWaveXe2, true, "Enable CS SIMD16 for Xe2 if the shader doesn't have wave",
                   false)
DECLARE_IGC_REGKEY(
    DWORD, CheckCSSLMLimit, 2,
    "Check SLM or threads limit on compute shader to turn on Enable2xGRF on DG2+"
    "0 - off, 1 - SLM limit heuristic, 2 - platform based heuristic (XE2 - threads limit, others - SLM limit)",
    false)
DECLARE_IGC_REGKEY(DWORD, Enable2xGRF, 2,
                   "Enable 2x GRF for high SLM or high threads usage"
                   "0 - off, 1 - on, 2 - platform default",
                   false)
DECLARE_IGC_REGKEY(bool, EnableKernelCostInfo, false, "Enable collecting kernel cost info", true)
DECLARE_IGC_REGKEY(bool, EnableKernelCostDebug, false, "Enable kernel cost info debuging", false)
DECLARE_IGC_REGKEY(bool, EnableTileYForExperiments, false, "Enable TileY heuristics for experiments", false)
DECLARE_IGC_REGKEY(bool, EnableDG2LSCSIMD8WA, true,
                   "Enables WA for DG2 LSC simd8 d32-v8/d64-v3/d64-v4. [temp, should be replaced with WA id", false)
DECLARE_IGC_REGKEY(bool, EnableScratchMessageD64WA, false, "Enables WA to legalize D64 scratch messages to D32", false)
DECLARE_IGC_REGKEY(DWORD, LscImmOffsMatch, 1,
                   "Match address patterns that have an immediate offset for the vISA LSC API"
                   "(0 means off/no matching,"
                   " 1 means on/match for supported platforms (Xe2+) and APIs,"
                   " 2 means force on for all platforms (vISA will emulate the addition if HW lacks support) and APIs,"
                   " 3 is the same as 2 and additionally skip the check if A32 offset is a positive value;"
                   " also see LscImmOffsVisaOpts",
                   true)
DECLARE_IGC_REGKEY(DWORD, LscImmOffsVisaOpts, 0x3003E,
                   "This maps to vISA_lscEnableImmOffsFor"
                   "(enables/disables immediate offsets for various address types; "
                   "see that option for semantics)",
                   true)
DECLARE_IGC_REGKEY(bool, DisableStatefulFolding, false,
                   "Turns off all folding for stateful messages (imm offset, scaling, and surface state idx)", false)
DECLARE_IGC_REGKEY(bool, EnableStatefulScaleFolding, false, "Enables folding of shl into the scale of a stateful send",
                   false)
DECLARE_IGC_REGKEY(bool, DisableLSCForTypedUAV, false,
                   "Forces legacy HDC messages for typed UAV read/write."
                   "Temporary knob for XE2 bringup.",
                   true)
DECLARE_IGC_REGKEY(bool, DisableLSCSIMD32TGMMessages, false,
                   "Forces splitting SIMD32 typed messages into 2xSIMD16."
                   "Only valid on XE2+.",
                   true)
DECLARE_IGC_REGKEY(bool, Enable_Wa1807084924, false, "Enable Wa_1807084924 regardless of the platfrom stepping", true)
DECLARE_IGC_REGKEY(bool, Enable_Wa1507979211, false, "Enable Wa_1507979211 regardless of the platfrom stepping", true)
DECLARE_IGC_REGKEY(bool, Enable_Wa14010017096, false, "Enable Wa_14010017096 regardless of the platfrom stepping", true)
DECLARE_IGC_REGKEY(bool, Enable_Wa22010487853, false, "Enable Wa_22010487853 regardless of the platfrom stepping", true)
DECLARE_IGC_REGKEY(bool, Enable_Wa22010493955, false, "Enable Wa_22010493955 regardless of the platfrom stepping", true)
DECLARE_IGC_REGKEY(bool, EnablePartialEmuI64, true, "Enable the partial I64 emulation for PVC-B, Xe2", true)
DECLARE_IGC_REGKEY_ENUM(LscLoadCacheControlOverride, 0, "Overrides cache-control options for non-intrinsic LSC loads.",
                        LSC_CACHE_CTRL_OPTIONS, true)
DECLARE_IGC_REGKEY_ENUM(LscStoreCacheControlOverride, 0,
                        "Overrides cache-control options for non-intrinsic LSC stores.", LSC_CACHE_CTRL_OPTIONS, true)
DECLARE_IGC_REGKEY_ENUM(TgmLoadCacheControlOverride, 0,
                        "Overrides cache-control options for non-intrinsic LSC tgm loads.", LSC_CACHE_CTRL_OPTIONS,
                        true)
DECLARE_IGC_REGKEY_ENUM(TgmStoreCacheControlOverride, 0,
                        "Overrides cache-control options for non-intrinsic LSC tgm stores.", LSC_CACHE_CTRL_OPTIONS,
                        true)
DECLARE_IGC_REGKEY(bool, LscForceSpillNonStackcall, false, "Non-stack call kernels that spill will use LSC on DG2+",
                   true)
DECLARE_IGC_REGKEY(bool, EnableEmitMoreMoviCases, false,
                   "Enables emitting movi for waveShuffle cases using And to keep index within single register.", true)
DECLARE_IGC_REGKEY_ENUM(ForceRegisterAccessBoundsChecks, -1,
                        "Controls the behavior of RegisterAccessBoundsChecks, the pass that adds runtime bounds-checks "
                        "for vector-indexing instructions."
                        "-1 - default behavior, the pass is enabled based on the API type or AILs"
                        " 0 - force disabled"
                        " 1 - force enabled",
                        TRIBOOL_OPTIONS, true)

DECLARE_IGC_REGKEY(
    bool, EnableGlobalStateBuffer, true,
    "This key allows stack calls to read implicit args from side buffer. It also emits a relocatable add in VISA.",
    true)
DECLARE_IGC_REGKEY(
    bool, LateInlineUnmaskedFunc, false,
    "Postpone inlining of Unmasked functions till end of CG to avoid code movement inside/outside of unmasked region",
    false)
DECLARE_IGC_REGKEY(bool, ForceFormatConversionDG2Plus, false,
                   "Forces SW image format conversion for R10G10B10A2_UNORM, R11G11B10_FLOAT, R10G10B10A2_UINT image "
                   "formats on DG2+ platforms",
                   true)
DECLARE_IGC_REGKEY(bool, EnableDivergentBarrierWA, false,
                   "Generate continuation code to handle shaders that places barriers in divergent control flow", false)
DECLARE_IGC_REGKEY(bool, DivergentBarrierUniformLoad, false,
                   "Optimize loads for spill/fill generated by DivergentBarrier with uniform analysis", true)
DECLARE_IGC_REGKEY(bool, ForcePrefetchToL1Cache, false, "Forces standard builtin prefetch to use L1 cache", true)
DECLARE_IGC_REGKEY(bool, DisablePrefetchToL1Cache, false, "Disable prefetch to L1 cache", true)
DECLARE_IGC_REGKEY(bool, ForceXYZworkGroupWalkOrder, true, "Force X/Y/Z WorkGroup walk order", true)
DECLARE_IGC_REGKEY(
    bool, ValidateSPIRVExtensionSupport, true,
    "When enabled, validate each SPIR-V OpExtension against device support and fail compilation if any are "
    "unsupported. Disabled by default until the supported extension list in SPIRVExtensions.td is complete.",
    true)

DECLARE_IGC_GROUP("Performance experiments")
DECLARE_IGC_REGKEY(DWORD, ManageableBarriersMode, 0,
                   "Set the ManageableBarriers mode in which should work"
                   "0 - Mix Mode of simple and dynamic ManageableBarriers"
                   "1 - Dynamic Mode Only, it will use SLM to store data related with barrier and use them in gateway "
                   "nbarrier instructions."
                   "2 - Simple Mode Only, it will use constant value in gateway nbarrier instructions (without SLM).",
                   true)
DECLARE_IGC_REGKEY(bool, ForceNonCoherentStatelessBTI, false,
                   "Enable gneeration of non cache coherent stateless messages", false)
DECLARE_IGC_REGKEY(bool, ForceSendsSupportOnSKLA0, false, "Allow sends on SKL A0, may be unsafe", false)
DECLARE_IGC_REGKEY(bool, DisableWaSendSEnableIndirectMsgDesc, false,
                   "Disable a C0 WA WaSendSEnableIndirectMsgDesc, may be unsafe", false)
DECLARE_IGC_REGKEY(bool, DisableWaDisableSIMD16On3SrcInstr, false,
                   "Disable C0 WA WaDisableSIMD16On3SrcInstr, may be unsafe", false)
DECLARE_IGC_REGKEY(bool, DiableWaSamplerNoMask, false, "Disable WA DiableWaSamplerNoMask", false)
DECLARE_IGC_REGKEY(bool, DisableDualBlendSource, false, "Force the compiler to never use dual blend source messages",
                   false)
DECLARE_IGC_REGKEY(
    bool, ForceDisableSrc0Alpha, false,
    "Force the compiler to skip sending src0 alpha. Only works if we are sure alpha to coverage and alpha test is off",
    false)
DECLARE_IGC_REGKEY(bool, EnableLTO, true, "Enable link time optimization", false)
DECLARE_IGC_REGKEY(bool, DisableLTOinMesh, false, "Disable link time optimization in Mesh Shaders only", false)
DECLARE_IGC_REGKEY(bool, EnableLTODebug, false, "Enable debug information for LTO", true)
DECLARE_IGC_REGKEY(DWORD, FunctionControl, 0,
                   "Control function inlining/subroutine/stackcall. See value defs in igc_flags.hpp.", true)
DECLARE_IGC_REGKEY(
    DWORD, SelectiveFunctionControl, 0,
    "Selectively enables FunctionControl for a list of line-separated function names in "
    "file specified by SelectiveFunctionControlFile or 'FunctionDebug.txt' in the IGC output dir, in that order."
    "When set by this flag, the functions in the list will override the default FunctionControl mode."
    "0 - Disable, 1 - Enable and read from SelectiveFunctionControlFile, 2 - Print all callable functions to file"
    "See comments in ProcessFuncAttributes.cpp for how to use this flag.",
    true)
DECLARE_IGC_REGKEY(debugString, SelectiveFunctionControlFile, 0,
                   "Set file with path that'll be used by SelectiveFunctionControl", true)
DECLARE_IGC_REGKEY(
    bool, EnableStackCallFuncCall, false,
    "If enabled, the default function call mode will be set to stack call. Otherwise, subroutine call is used.", true)
DECLARE_IGC_REGKEY(bool, EnableByValStructArgPromotion, true,
                   "If enabled, byval/sret struct arguments are promoted to pass-by-value if possible.", true)
DECLARE_IGC_REGKEY(bool, ForceInlineStackCallWithImplArg, false,
                   "If enabled, stack calls that uses implicit args will be force inlined.", true)
DECLARE_IGC_REGKEY(bool, EnableFunctionCloningControl, true,
                   "If enabled, limits function cloning by converting stackcalls to indirect calls based on the "
                   "FunctionCloningThreshold value.",
                   true)
DECLARE_IGC_REGKEY(DWORD, FunctionCloningThreshold, 0,
                   "Limits the number of cloned functions when called from multiple function groups."
                   "If number of cloned functions exceeds the threshold, compile the function only once and use "
                   "address relocation instead."
                   "Setting this to '0' allows IGC to choose the default threshold.",
                   true)
DECLARE_IGC_REGKEY(bool, ForceLowestSIMDForStackCalls, true,
                   "If enabled, compile to the lowest allowed SIMD mode when stack calls or indirect calls are present",
                   true)
DECLARE_IGC_REGKEY(DWORD, OCLInlineThreshold, 512, "Setting OCL inline thershold", true)
DECLARE_IGC_REGKEY(bool, DisableAddingAlwaysAttribute, false, "Disable adding always attribute", true)
DECLARE_IGC_REGKEY(bool, EnableForceGroupSize, false,
                   "Enable forcing thread Group Size ForceGroupSizeX and ForceGroupSizeY", false)
DECLARE_IGC_REGKEY(bool, EnableForceThreadCombining, false,
                   "Enable forcing Thread Combining with thread Group Size ForceGroupSizeX and ForceGroupSizeY", false)
DECLARE_IGC_REGKEY(DWORD, ForceGroupSizeShaderHash, 0,
                   "Shader hash for forcing thread group size or thread combining (lower 8 hex digits)", false)
DECLARE_IGC_REGKEY(DWORD, ForceGroupSizeX, 8, "force group size along X", false)
DECLARE_IGC_REGKEY(DWORD, ForceGroupSizeY, 8, "force group size along Y", false)
DECLARE_IGC_REGKEY(bool, EnableThreadCombiningWithNoSLM, false, "Enable thread combining opt for shader without SLM",
                   false)
DECLARE_IGC_REGKEY(bool, DisableInlining, false, "Disable inlining of all functions", false)
DECLARE_IGC_REGKEY(bool, EnableDropTargetFunctions, false, "Enables pass for dropping targeted functions", false)
DECLARE_IGC_REGKEY(bool, VerboseDropTargetFunctions, false, "Enables verbose logging for dropping targeted functions",
                   false)
DECLARE_IGC_REGKEY(bool, CrashOnDroppedFnAccess, false, "Enables crash on access to dropped functions", true)
DECLARE_IGC_REGKEY(debugString, DropTargetFnListPath, 0, "Path to folder with lists of functions to drop", false)
DECLARE_IGC_REGKEY(bool, EnableDropTargetBBs, false, "Enables pass for dropping targeted BBs", false)
DECLARE_IGC_REGKEY(bool, VerboseDropTargetBBs, false, "Enables verbose logging for dropping targeted BBs", false)
DECLARE_IGC_REGKEY(debugString, DropTargetBBListPath, 0, "Path to folder with lists of BBs to drop", false)
DECLARE_IGC_REGKEY(DWORD, PrintFunctionSizeAnalysis, 0, "Print analysis data of function sizes", true)
DECLARE_IGC_REGKEY(DWORD, SubroutineThreshold, 110000, "Minimal kernel size to enable subroutines", false)
DECLARE_IGC_REGKEY(DWORD, SubroutineInlinerThreshold, 3000, "Subroutine inliner threshold", false)
DECLARE_IGC_REGKEY(bool, EnableLargeFunctionCallMerging, true,
                   "Merge mutually exclusive calls to large functions to enable inlining", false)
DECLARE_IGC_REGKEY(bool, ControlKernelTotalSize, true, "Control kernel total size", true)
DECLARE_IGC_REGKEY(bool, StaticProfileGuidedTrimming, false, "Enable static analysis in the kernel trimming", true)
DECLARE_IGC_REGKEY(debugString, SelectiveTrimming, 0, "Choose a specific function to trim", true)
DECLARE_IGC_REGKEY(bool, EnableGreedyTrimming, false, "Find the optimal set of functions to trim", true)
DECLARE_IGC_REGKEY(bool, EnableLeafCollapsing, false,
                   "Collapse leaf functions in order to avoid trimming small leaf functions", true)
DECLARE_IGC_REGKEY(bool, UseFrequencyInfoForSPGT, true, "Consider frequency information for trimming functions", true)
DECLARE_IGC_REGKEY(bool, EnableSizeContributionOptimization, false,
                   "Put more weight on a function when the potential size contirubion is big", true)
DECLARE_IGC_REGKEY(DWORD, SkipTrimmingOneCopyFunction, 3000,
                   "Don't trim a function whose size contribution is no more than its size", true)
DECLARE_IGC_REGKEY(bool, LoopCountAwareTrimming, false,
                   "Take loop count into account in measuring the function size for trimming", true)
DECLARE_IGC_REGKEY(DWORD, MaxUnrollCountForFunctionSizeAnalysis, 16,
                   "The maximum number of loop unrolling assumed in function size analaysis", true)
DECLARE_IGC_REGKEY(bool, ControlInlineImplicitArgs, true, "Avoid trimming functions with implicit args", true)
DECLARE_IGC_REGKEY(DWORD, ControlInlineTinySize, 200, "Tiny function size for controlling kernel total size", true)
DECLARE_IGC_REGKEY(DWORD, ControlInlineTinySizeForSPGT, 300, "Tiny function size for controlling kernel total size",
                   true)
DECLARE_IGC_REGKEY(DWORD, SizeWeightForSPGT, 3, "Size weight for a trimming threshold", true)
DECLARE_IGC_REGKEY(DWORD, FrequencyWeightForSPGT, 2, "Frequency weight for a trimming threshold", true)
DECLARE_IGC_REGKEY(DWORD, PrintControlKernelTotalSize, 0, "Print Control kernel total size", true)
DECLARE_IGC_REGKEY(bool, AddNoInlineToTrimmedFunctions, false, "Tell late passes not to inline trimmed functions",
                   false)
DECLARE_IGC_REGKEY(bool, ForceInlineExternalFunctions, false, "not to trim functions called from multiple kernels",
                   true)
DECLARE_IGC_REGKEY(DWORD, KernelTotalSizeThreshold, 50000, "Trimming target of kernel total size", true)
DECLARE_IGC_REGKEY(DWORD, LargeKernelThresholdMultiplier, 13,
                   "Multipler to kernel threshold. When exceeded more agressive trimming will be performed", false)
DECLARE_IGC_REGKEY(bool, PartitionUnit, false, "Partition compilation unit", true)
DECLARE_IGC_REGKEY(DWORD, PrintPartitionUnit, 0, "Print information about compilation unit partitioning", true)
DECLARE_IGC_REGKEY(bool, PartitionWithFastHybridRA, false, "Enable FastRA and HybridRA when partition is enabled", true)
DECLARE_IGC_REGKEY(DWORD, UnitSizeThreshold, 3000, "Compilation unit size threshold", true)
DECLARE_IGC_REGKEY(bool, StaticProfileGuidedPartitioning, 0, "Enable static analysis in the partitioning algorithm.",
                   true)
DECLARE_IGC_REGKEY(DWORD, PrintStaticProfileGuidedKernelSizeReduction, 0,
                   "Print information about static profile-guided trimming and partitioning", true)
DECLARE_IGC_REGKEY(DWORD, MetricForKernelSizeReduction, 2,
                   "Set 1 to active a normal distribution, 2 a long-tail distribution, and 4 an average%", true)
DECLARE_IGC_REGKEY(bool, BlockFrequencySampling, true, "Use block frequencies to derive a distribution", true)
DECLARE_IGC_REGKEY(DWORD, ParameterForColdFuncThreshold, 80,
                   "C/10-STD for a normal distribution / low K% for a long-tail distribution", true)
DECLARE_IGC_REGKEY(bool, ControlUnitSize, false, "Control compilation unit size by unit trimming", true)
DECLARE_IGC_REGKEY(DWORD, ExpandedUnitSizeThreshold, 50000, "Trimming target of compilation unit size", true)
DECLARE_IGC_REGKEY(DWORD, PrintControlUnitSize, 0, "Print information about unit trimming", true)
DECLARE_IGC_REGKEY(DWORD, StaticProfileGuidedSpillCostAnalysis, 8,
                   "Use static profile information to estimate spill cost, "
                   "1 for profile generation, 2 for profile transfer, 4 for profile embedding, "
                   "8 for spill computation, and 16 for enabling frequency-based spill selection ",
                   true)
DECLARE_IGC_REGKEY(DWORD, StaticProfileGuidedSpillCostAnalysisScale, 4,
                   "Scale adjustment for static profile guided spill cost analysis", true)
DECLARE_IGC_REGKEY(DWORD, StaticProfileGuidedSpillCostAnalysisFunc, 1,
                   "Spill cost function where 0 is based on a new spill cost and 1 the existing one", true)
DECLARE_IGC_REGKEY(DWORD, PrintStaticProfileGuidedSpillCostAnalysis, 0, "Print debug messages for profile embedding",
                   true)
DECLARE_IGC_REGKEY(bool, EnableConstantPromotion, true, "Enable global constant data to register promotion", false)
DECLARE_IGC_REGKEY(bool, AllowNonLoopConstantPromotion, false,
                   "Allows promotion for constants not in loop (e.g. used once)", false)
DECLARE_IGC_REGKEY(DWORD, ConstantPromotionSize, 2, "Threshold in number of GRFs", false)
DECLARE_IGC_REGKEY(DWORD, ConstantPromotionCmpSelSize, 4, "Array size threshold for cmp-sel transform", false)
DECLARE_IGC_REGKEY(bool, FuseResourceLoop, false, "Enable fusing resource loops", false)
DECLARE_IGC_REGKEY(DWORD, ResourceLoopUnrollIteration, 1,
                   "Unroll resource loop iterations (larger than 1): 1 (default) - no sub-iteration", false)
DECLARE_IGC_REGKEY(bool, DisableResourceLoopUnrollExclusiveLoad, false,
                   "Disable visa ExclusiveLoad for the SBID in Unroll resource loop", false)
DECLARE_IGC_REGKEY(DWORD, ResourceLoopUnrollNested, 0,
                   "Unroll resource loop iterations (larger than 0): 0 (default) - no nested loop", false)
DECLARE_IGC_REGKEY(bool, DisableResourceLoopUnrollNestedLsc, false, "Disable unroll nested for lsc load.", false)
DECLARE_IGC_REGKEY(bool, DisableResourceLoopUnrollNestedSampler, false, "Disable unroll nested for sampler.", false)
DECLARE_IGC_REGKEY(bool, EnableVariableReuse, true, "Enable local variable reuse", false)
DECLARE_IGC_REGKEY(bool, EnableVariableAlias, true,
                   "Enable variable aliases (part of VariableReuse Pass, but separate functionality)", false)
DECLARE_IGC_REGKEY(DWORD, VectorAlias, 1,
                   "Vector aliasing control under EnableVariableAlias. Some features are still experimental", true)
DECLARE_IGC_REGKEY(DWORD, VectorAliasBBThreshold, 200,
                   "Max number of BBs of a function that VectorAlias will apply. VectorAlias will skip for funtions "
                   "beyond this threshold",
                   true)
DECLARE_IGC_REGKEY(
    DWORD, ScalarAliasBBSizeThreshold, 500,
    "Max size of BB for which scalar aliasing will apply. Scalar aliasing will skip for BBs beyond this threshold",
    true)
DECLARE_IGC_REGKEY(bool, EnableExtractMask, false,
                   "When enabled, it is mostly for reducing response size of send messages.", false)
DECLARE_IGC_REGKEY(DWORD, VariableReuseByteSize, 64, "The byte size threshold for variable reuse", false)
DECLARE_IGC_REGKEY(bool, EnableGather4cpoWA, true, "Enable WA transforming gather4cpo/gather4po into gather4c/gather4",
                   false)
DECLARE_IGC_REGKEY(bool, EnableIntelFast, false, "Enable intel fast, experimental flag.", false)
DECLARE_IGC_REGKEY(bool, disableUnormTypedReadWA, false, "disable software conversion for UNORM surface in Dx10", false)
DECLARE_IGC_REGKEY(bool, forceGlobalRA, false, "force global register allocator", false)
DECLARE_IGC_REGKEY(bool, disableVarSplit, false, "disable variable splitting", false)
DECLARE_IGC_REGKEY(bool, delayVarSplit, false, "delay local variable splitting", false)
DECLARE_IGC_REGKEY(bool, disableRemat, false, "disable re-materialization", false)
DECLARE_IGC_REGKEY(bool, EnableDisableMidThreadPreemptionOpt, true, "Disable mid thread preemption", false)
DECLARE_IGC_REGKEY(DWORD, MidThreadPreemptionDisableThreshold, 600, "Threshold to disable mid thread preemption", false)
DECLARE_IGC_REGKEY(DWORD, DispatchGPGPUWalkerAlongYFirst, 1,
                   "0 = No SW Y-walk, 1 = Dispatch GPGPU walker along Y first", false)
DECLARE_IGC_REGKEY(DWORD, DispatchAlongY_XY_ratio, 0, "min threshold for thread group size x / y for dispatchAlongY",
                   false)
DECLARE_IGC_REGKEY(DWORD, DispatchAlongY_X_threshold, 0, "min threshold for thread group size x for dispatchAlongY",
                   false)
DECLARE_IGC_REGKEY(bool, LimitConstantBuffersPushed, true,
                   "Limit max number of CBs pushed when SupportIndirectConstantBuffer is true", false)
DECLARE_IGC_REGKEY(bool, forceSamplerHeader, false, "force sampler messages to use header", false)
DECLARE_IGC_REGKEY(bool, samplerHeaderWA, false, "enable sampler header to solve HW WA", false)
DECLARE_IGC_REGKEY(bool, VFPackingDisablePartialElements, false,
                   "disable packing for partial vertex element as it causes performance drops", false)
DECLARE_IGC_REGKEY(bool, cl_khr_srgb_image_writes, false, "Enable cl_khr_srgb_image_writes extension", false)
DECLARE_IGC_REGKEY(bool, MSAA16BitPayloadEnable, true,
                   "Enable support for MSAA 16 bit payload , a hardware DCN supporting this from ICL+ to improve perf "
                   "on MSAA workloads",
                   false)
DECLARE_IGC_REGKEY(bool, EnableInsertElementScalarCoalescing, false,
                   "Enable coalescing on the scalar operand of insertelement", false)
DECLARE_IGC_REGKEY(bool, EnableMixIntOperands, true, "Enable generating mix-sized operands for int ALU", false)
DECLARE_IGC_REGKEY(bool, PixelShaderDoNotAbortOnSpill, false, "Do not abort on a spill", false)
DECLARE_IGC_REGKEY(DWORD, ForceScratchSpaceSize, 0, "Override Scratch Space Size in bytes for perf testing", false)
DECLARE_IGC_REGKEY(DWORD, SkipPaddingScratchSpaceSize, 4096,
                   "Skip adding padding when estimated scratch space size is smaller than or equal to this value",
                   false)
DECLARE_IGC_REGKEY(
    DWORD, ForcePixelShaderSIMDMode, 0,
    "Setting it to values def in igc.h will force SIMD mode compilation for pixel shaders. Note that only SIMD8 is "
    "compiled unless other ForcePixelShaderSIMD* are also selected. 1-SIMD8, 2-SIMD16,4-SIMD32",
    false)
DECLARE_IGC_REGKEY(DWORD, StagedCompilationExperiments, 0, "Experiment with staged compilation when != 0", false)
DECLARE_IGC_REGKEY(bool, DisableDynamicPolyPackingPolicies, true,
                   "Disable dynamic poly packing policies for Xe3+ platforms", false)
DECLARE_IGC_REGKEY(bool, RequestStage2, true, "Enable staged compilation via requesting stage 2", false)
DECLARE_IGC_REGKEY(bool, LTOForStage1Compilation, true, "LTO for stage 1 compilation", false)
DECLARE_IGC_REGKEY(bool, PSOForStage1Compilation, true, "PSO for stage 1 compilation", false)
DECLARE_IGC_REGKEY(bool, SWStencilForStage1Compilation, true, "SWStencil for stage 1 compilation", false)
DECLARE_IGC_REGKEY(bool, EnableTrackPtr, false, "Track Staging Context alloc/dealloc", false)
DECLARE_IGC_REGKEY(bool, ExtraRetrySIMD16, false, "Enable extra simd16 with retry for STAGE1_BEST_PREF", false)
DECLARE_IGC_REGKEY(bool, SaveRestoreIR, true, "Save/Restore IR for staged compilation to avoid duplicated compilations",
                   false)
DECLARE_IGC_REGKEY(DWORD, SSOShifter, 9,
                   "Adjust ScratchSurfaceOffset with shl(hwtid, shifter). 0 menas disabling padding", false)
DECLARE_IGC_REGKEY(DWORD, DelayEmuInt64AddLimit, 0, "Delay emulating Int64 Add operations in vISA", false)
DECLARE_IGC_REGKEY(DWORD, CodePatch, 2, "Enable Pixel Shader code patching to directly emit code after stitching",
                   false)
DECLARE_IGC_REGKEY(DWORD, CodePatchLimit, 0, "Debug CodePatch via limiting the number of shader been patched", false)
DECLARE_IGC_REGKEY(DWORD, CodePatchExperiments, 0, "Experiment with code patching when != 0", false)
DECLARE_IGC_REGKEY(DWORD, CodePatchFilter, 0x7, "Filter out unsupported patterns", false)
DECLARE_IGC_REGKEY(
    DWORD, FirstStagedSIMD, 0,
    "Force Pixel shader to be 1: FastSIMD (SIMD8), 2: BestSIMD (SIMD16 or SIMD8), 3: FatestSIMD (SIMD8 opt off)", false)
DECLARE_IGC_REGKEY(DWORD, FastestS1Experiments, 0, "Select configs for fastest compilation by bits.", false)
DECLARE_IGC_REGKEY(
    bool, ForceAddingStackcallKernelPrerequisites, false,
    "Force adding static overhead for stackcall to the kernel entry such as HWTID instructions for experiments", true)
DECLARE_IGC_REGKEY(bool, DisableFastestLinearScan, false, "Disable LinearScanRA in FastestSIMD.", false)
DECLARE_IGC_REGKEY(bool, DisableFastestGopt, false, "Disable global optimizations for stage 1 shaders.", false)
DECLARE_IGC_REGKEY(bool, ForceFastestSIMD, false,
                   "Force PS, CS, VS to return lowest possible SIMD as fast as possible.", false)
DECLARE_IGC_REGKEY(bool, EnableFastestSingleCSSIMD, true, "Enable selecting single CS SIMD in staged compilation.",
                   false)
DECLARE_IGC_REGKEY(bool, ForceFastestSingleCSSIMD, false,
                   "Force selecting single CS SIMD in staged compilation on unsupported platforms.", false)
DECLARE_IGC_REGKEY(bool, ForceBestSIMD, false, "Force pixel shader to return the best SIMD, either SIMD16 or SIMD8.",
                   false)
DECLARE_IGC_REGKEY(bool, SkipTREarlyExitCheck, false, "Skip SIMD16 early exit check in ShaderCodeGen", false)
DECLARE_IGC_REGKEY(
    bool, EnableTCSHWBarriers, false,
    "Enable TCS pass with HW barriers support. Default TCS pass is TCS pass with multiple continuation functions.",
    false)
DECLARE_IGC_REGKEY(
    bool, ForceMCFBarriers, false,
    "Force TCS pass with MCF (SW) barriers support. Default TCS pass is TCS pass with multiple continuation functions.",
    false)
DECLARE_IGC_REGKEY(bool, EnableAccSub, true, "Enable accumulator substitution", false)
DECLARE_IGC_REGKEY(bool, EnablePreRAAccSchedAndSub, false, "Enable accumulator substitution", false)
DECLARE_IGC_REGKEY(
    DWORD, NumGeneralAcc, 0,
    "set the number [1-8] of general acc for accumulator substitution. 0 means using the platform-default value", false)
DECLARE_IGC_REGKEY(bool, HasDoubleAcc, false, "has doubled accumulators", false)
DECLARE_IGC_REGKEY(bool, ForceSWCoalescingOfAtomicCounter, false, "Force software coalescing of atomic counter", false)
DECLARE_IGC_REGKEY(bool, ForceMixMode, false, "force enable mix mode even on platforms that do not support it", false)
DECLARE_IGC_REGKEY(bool, DisableFDIV, false, "Disable fdiv support", false)
DECLARE_IGC_REGKEY(bool, EmulateFDIV, false, "Emulate fdiv instructions", false)
DECLARE_IGC_REGKEY(bool, UpConvertF16Sampler, true, "up-convert fp16 sampler message to return fp32", false)
DECLARE_IGC_REGKEY(bool, DownConvertI32Sampler, false, "Convert i32 sampler messages to return i16.\
    This optimization can only be enabled for resources with 16bit integer format\
    or if it is known that the upper 16bits of data is always 0.",
                   false)
DECLARE_IGC_REGKEY(bool, FuseTypedWrite, false, "Enable fusing of simd8 typed write", false)
DECLARE_IGC_REGKEY(bool, DisableUndefAlphaOutputAsRed, false, "Disable output red for undefined alpha output", false)
DECLARE_IGC_REGKEY(
    bool, EnableHalfPromotion, true,
    "Enable pass that replaces instructions using halfs with corresponding float counterparts for pre-SKL", false)
DECLARE_IGC_REGKEY(bool, ForceHalfPromotion, false,
                   "Force enable pass that replaces instructions using halfs with corresponding float counterparts",
                   false)
DECLARE_IGC_REGKEY(
    bool, ForceNoInfiniteLoops, false,
    "Limit # of loop iterations to UINT_MAX in while/for loops. Can be used to detect infinite loops in shaders", false)
DECLARE_IGC_REGKEY(
    bool, DisbleLocalFences, false,
    "On CNL+ we need to emit local fences. Setting this to true removes those. It may be functionaly not correct.",
    false)
DECLARE_IGC_REGKEY(bool, FastSpill, false,
                   "fast spill code gen. This may produce worse equality code for the spilling shader", false)
DECLARE_IGC_REGKEY(bool, EnableGSURBEntryPadding, true,
                   "Enable padding of GS URB Entry by adding extra portions of Control Data Header.", false)
DECLARE_IGC_REGKEY(bool, EnableGSVtxCountMsgHalfCLSize, true,
                   "Enable the Vertex Count msg of half CL size, instead of 1DW size.", false)
DECLARE_IGC_REGKEY(bool, EnableTEFactorsPadding, true, "Enable padding of the TE factors.", false)
DECLARE_IGC_REGKEY(bool, EnableTEFactorsClear, true, "Enable clearing of tessellation factors.", false)
DECLARE_IGC_REGKEY(DWORD, EmulationFunctionControl, 0,
                   "FunctionControl on some DP emulation functions. It has the same value as FunctionControl.", true)
DECLARE_IGC_REGKEY(DWORD, InlinedEmulationThreshold, 125000, "Inlined instruction threshold for enabling subroutines",
                   false)
DECLARE_IGC_REGKEY(int, ByPassAllocaSizeHeuristic, 0,
                   "Force some Alloca to pass the pressure heuristic until the given size", true)
DECLARE_IGC_REGKEY(DWORD, MemOptWindowSize, 150,
                   "Size of the window in unit of instructions in which load/stores are allowed to be coalesced. Keep "
                   "it limited in order to avoid creating long liveranges. Default value is 150",
                   false)
DECLARE_IGC_REGKEY(DWORD, RematFlowThreshold, 10,
                   "Proportion of the whole rematerialization targets to cutoff remat chain", false)
DECLARE_IGC_REGKEY(DWORD, RematChainLimit, 12,
                   "If number of instructions we've collected is more than this value, we bail on it", false)
DECLARE_IGC_REGKEY(DWORD, RematRPELimit, 120,
                   "Cutoff value for register estimator, lower than that, kernel won't be rematted", false)
DECLARE_IGC_REGKEY(bool, RematEnable, false, "Enable clone adress arithmetic pass not only on retry", false)
DECLARE_IGC_REGKEY(bool, RematLog, false, "Dump Remat Log, usefull for analyzing spills as well", false)
DECLARE_IGC_REGKEY(
    bool, RematSameBBScope, false,
    "Confine rematerialization only to variables within the same BB, we won't pull down values from predeccors", false)
DECLARE_IGC_REGKEY(bool, RematRespectUniformity, false, "Cutoff computation chain on uniform values", false)
DECLARE_IGC_REGKEY(bool, RematAllowExtractElement, true, "Allow Extract Element to computation chain", false)
DECLARE_IGC_REGKEY(bool, RematDataAllowCMP, true, "Allow rematerialization of cmp instructions", true)
DECLARE_IGC_REGKEY(bool, RematReassocBefore, false,
                   "Enable short sequence of passes before clone address arithmetic pass to potentially decrese amount "
                   "of operations that will be rematerialized",
                   false)
DECLARE_IGC_REGKEY(bool, RematInstCombineBefore, false,
                   "Enable short sequence of passes before clone address arithmetic pass to potentially decrese amount "
                   "of operations that will be rematerialized",
                   false)
DECLARE_IGC_REGKEY(bool, RematAddrSpaceCastToUse, true,
                   "Allow rematerialization of inttoptr that are used inside AddrSpaceCastInst", false)
DECLARE_IGC_REGKEY(bool, RematCallsOperand, true, "Allow rematerialization of inttoptr that are used as call's operand",
                   false)
DECLARE_IGC_REGKEY(bool, RematCollectCallArgs, true, "Allow collection of call arguments for rematerialization", false)
DECLARE_IGC_REGKEY(bool, RematAllowOneUseLoad, false,
                   "Remat allow to move loads that have one use and it's inside the chain", false)
DECLARE_IGC_REGKEY(bool, RematAllowLoads, false,
                   "Remat allow to move loads, no checks, exclusively for testing purposes", false)
DECLARE_IGC_REGKEY_BITMASK(RematOptionsForRetry, 0,
                           "Options for CloneAddressArithmetic pass when recompiling shader. Valid for non-OpenCL only",
                           REMAT_MASK, false)
DECLARE_IGC_REGKEY_BITMASK(RematOptionsForVRT, 0,
                           "Options for CloneAddressArithmetic pass when compiling shader. Valid for non-OpenCL only",
                           REMAT_MASK, false)
DECLARE_IGC_REGKEY(bool, DumpRegPressureEstimate, false, "Dump RegPressureEstimate to a file", false)
DECLARE_IGC_REGKEY(debugString, DumpRegPressureEstimateFilter, 0,
                   "Only dump RegPressureEstimate for functions matching the given regex", false)
DECLARE_IGC_REGKEY(bool, AddressSpacePhiPropagation, true,
                   "Lower loads from PHI nodes into incoming nodes in case they cause extra address space casts.",
                   false)
DECLARE_IGC_REGKEY(bool, VectorizerLog, false, "Dump Vectorizer Log, usefull for analyzing vectorization issues", true)
DECLARE_IGC_REGKEY(bool, VectorizerLogToErr, false, "Dump Vectorizer Log to stdErr", true)
DECLARE_IGC_REGKEY(bool, EnableReusingXYZWStoreConstPayload, true, "Enable reusing XYZW stores const payload", false)
DECLARE_IGC_REGKEY(bool, EnableReusingLSCStoreConstPayload, false, "Enable reusing LSC stores const payload", false)
DECLARE_IGC_REGKEY(bool, AllowSIMD16DropForXE2Plus, true, "Controls the switch for XE2 and XE3 simd16 drop", false)
DECLARE_IGC_REGKEY(bool, AllowEarlySIMD16DropForXE3, true, "Controls the early drop to simd16 for XE3", false)
DECLARE_IGC_REGKEY(DWORD, EarlySIMD16DropForXE3Threshold, 256, "Threshold for the early drop to simd16 for XE3", false)
DECLARE_IGC_REGKEY(DWORD, RegPressureVerbocity, 2, "Different printing types", false)
DECLARE_IGC_REGKEY(DWORD, RetryRevertExcessiveSpillingKernelThreshold, 10000,
                   "Sets the threshold for Retry Manager to know which kernel is considered as Excessive Spilling and "
                   "applies different set of rules",
                   false)
DECLARE_IGC_REGKEY(
    DWORD, RetryRevertExcessiveSpillingKernelCoefficient, 102,
    "Sets the coefficient for Retry Manager to know whether we should revert back to a previously compiled kernel",
    false)
DECLARE_IGC_REGKEY(
    DWORD, ForceSIMDRPELimit, 1000,
    "Cutoff value for register estimator, when higher than that kernel is switched to lower SIMD when possible", false)
DECLARE_IGC_REGKEY(DWORD, EarlyRetryLargeGRFThreshold, 500,
                   "Cutoff value for register estimation, when highter than that kernel skips first compilation stage "
                   "and goes to retry immediately for large GRF.",
                   false)
DECLARE_IGC_REGKEY(DWORD, EarlyRetryDefaultGRFThreshold, 190,
                   "Cutoff value for register estimation, when highter than that kernel skips first compilation stage "
                   "and goes to retry immediately for default GRF.",
                   false)
DECLARE_IGC_REGKEY(bool, ForceNoFP64bRegioning, false, "force regioning rules for FP and 64b FPU instructions", false)
DECLARE_IGC_REGKEY(bool, EnableA64WA, true, "Guarantee A64 load/store addres-hi is uniform", true)
DECLARE_IGC_REGKEY(bool, EnableSamplerSplit, false, "Split Sampler 3d message to odd and even", false)
DECLARE_IGC_REGKEY(bool, EnableEvaluateSamplerSplit, true,
                   "Split evaluate messages to sampler into either SIMD8 or SIMD1 messages", false)
DECLARE_IGC_REGKEY(DWORD, AllocaRAPressureThreshold, 500, "The threshold for the register pressure potential", false)
DECLARE_IGC_REGKEY(DWORD, HPCInstNumThreshold, 1000000, "The threshold for the register pressure potential", false)
DECLARE_IGC_REGKEY(DWORD, HPCGlobalInstNumThreshold, 500000, "The threshold for the register pressure potential", false)
DECLARE_IGC_REGKEY(bool, HPCFastCompilation, false, "Force to do fast compilation for HPC kernel", false)
DECLARE_IGC_REGKEY(bool, UseOldSubRoutineAugIntf, false, "Use the old subroutine augmentation code which is slower",
                   false)
DECLARE_IGC_REGKEY(bool, DisableFastRAWA, true, "Disable Fast RA for hanging issues on large workloads", false)
DECLARE_IGC_REGKEY(bool, FastCompileRA, false, "Provide the fast compilatoin path for RA, fail safe at first iteration",
                   false)
DECLARE_IGC_REGKEY(bool, HybridRAWithSpill, false, "Did Hybrid RA with Spill", false)
DECLARE_IGC_REGKEY(bool, SelectiveFastRA, false, "Apply fast RA with spills selectively using heuristics", true)
DECLARE_IGC_REGKEY(DWORD, AllowStackCallRetry, 2,
                   "Enable/Disable retry when stack function spill. 0 - Don't allow, 1 - Allow retry on kernel group, "
                   "2 - Allow retry per function",
                   false)
DECLARE_IGC_REGKEY(bool, PrintStackCallDebugInfo, false,
                   "Print all debug info to command line related to stack call debugging", true)
DECLARE_IGC_REGKEY(DWORD, StripDebugInfo, 0,
                   "Strip debug info from llvm IR lowered from input to IGC ."
                   "Possible values: 0 - dont strip, 1 - strip all, 2 - strip non-line info",
                   true)
DECLARE_IGC_REGKEY(bool, EmitPreDefinedForAllFunctions, false,
                   "When enabled, pre-defined variables for gid, grid, lid are emitted for all functions. This causes "
                   "those functions to be inlined even when stack calls is enabled.",
                   true)
DECLARE_IGC_REGKEY(bool, EnableGPUFenceScopeOnSingleTileGPUs, false,
                   "Allow the use of `GPU` fence scope on single-tile GPUs. By default the `TILE` scope is used "
                   "instead of `GPU` scope on single-tile GPUs.",
                   true)
DECLARE_IGC_REGKEY(bool, EnableLocalIdCalculationInShader, false,
                   "Enables calcualtion of local thread IDs in shader. Valid only in compute"
                   "shaders on XeHP+. IDs are calculated only if HW generated IDs cannot be"
                   "used.",
                   true)
DECLARE_IGC_REGKEY(int, JointMatrixLoadStoreOpt, 3,
                   "Selects subgroup (0), or block read/write (1), or optimized block read/write (2), 2d block "
                   "read/write (3) implementation of Joint Matrix Load/Store built-ins",
                   true)
DECLARE_IGC_REGKEY(bool, EnableVector8LoadStore, false,
                   "Enable Vectorizer to generate 8x32i and 4x64i loads and stores", true)
DECLARE_IGC_REGKEY(bool, EnableOpaquePointersBackend, false,
                   "[Experimental] Force opaque pointers' usage within IGC/LLVM passes", false)
DECLARE_IGC_REGKEY(bool, ExcludeIRFromZEBinary, false, "Exclude IR sections from ZE binary", true)
DECLARE_IGC_REGKEY(bool, AllocateZeroInitializedVarsInBss, true,
                   "Allocate zero initialized global variables in .bss section in ZEBinary", true)
DECLARE_IGC_REGKEY(DWORD, OverrideOCLMaxParamSize, 0,
                   "Override the value imposed on the kernel by CL_DEVICE_MAX_PARAMETER_SIZE. Value in bytes, if "
                   "value==0 no override happens.",
                   true)

DECLARE_IGC_REGKEY(bool, EnableOptReportPrivateMemoryToSLM, false,
                   "[POC] Generate opt report file for moving private memory allocations to SLM.", false)
DECLARE_IGC_REGKEY(bool, ForceAllPrivateMemoryToSLM, false, "[POC] Force moving all private memory allocations to SLM.",
                   false)
DECLARE_IGC_REGKEY(debugString, ForcePrivateMemoryToSLMOnBuffers, 0,
                   "[POC] Force moving private memory allocations to SLM, semicolon-separated list of buffers.", false)
DECLARE_IGC_REGKEY(bool, ForcePrivateMemoryToGlobalOnGeneric, true,
                   "Force moving private memory allocations to global buffer when generic pointer is present", true)
DECLARE_IGC_REGKEY(
    bool, DetectCastToGAS, true,
    "Check if the module contains local/private to GAS (Gerneric Address Space) cast, it also check internal flags",
    true)
DECLARE_IGC_REGKEY(bool, EnableProgrammableOffsetsMessageBitInHeader, false,
                   "Use pre-delta feature (legacy) method of passing MSB of PO messages opcode. ", false)
DECLARE_IGC_REGKEY(bool, EnableEfficient64b, false,
                   "Enable efficient64b feature such as new inline data and new send messages and descriptor formats, "
                   "valid for xe3p+.",
                   true)
DECLARE_IGC_REGKEY(bool, EnableForcedEfficient64b, false,
                   "Temporary regkey to be enabled when testing new cobalt features. Remove when synced with cobalt.",
                   true)
DECLARE_IGC_REGKEY(bool, EnableSkipUnusedColorPayload, true,
                   "Enables skipping unused color phases of render target write.", true)
DECLARE_IGC_REGKEY(bool, EnableResourceLoopNonUniformCmpLowerHalfDWOnly, true,
                   "Only compare the lower half of 64-bit resource address in the resource loop. This is to assume the "
                   "number of the resource in the heap will never exceed 2^32 limitation.",
                   true)
DECLARE_IGC_REGKEY(bool, DisableSWManagedStack, false,
                   "Disables SW managed stack for RayQuery, the compiler will use legacy stack size and stackID "
                   "calculation, valid for xe3p+.",
                   true)
DECLARE_IGC_REGKEY(DWORD, SWManagedStackNumStacks, 0,
                   "Forces the number of syncRT stacks per DSS. If explicitly set to 0, 2048 is assumed to maintain "
                   "backward compatibility.",
                   true)
DECLARE_IGC_REGKEY(DWORD, EnableScalarPipe, 0,
                   "for scalar-pipe experiment, N specifies the number of scalar registers in Nx16 dwords", false)
DECLARE_IGC_REGKEY(bool, DisableEngineID, false, "Disables usage of engine ID from ARF", true)
DECLARE_IGC_REGKEY(bool, Enable32bSampler, true, "Enables 32b samplers", false)
DECLARE_IGC_REGKEY(bool, OverrideCsWalkOrderEnable, false, "Enable overriding compute walker walk order", true)
DECLARE_IGC_REGKEY(int, OverrideCsWalkOrder, 0, "Override compute walker walk order", true)
DECLARE_IGC_REGKEY(bool, OverrideCsTileLayoutEnable, false, "Enable overriding compute walker tile layout", true)
DECLARE_IGC_REGKEY(int, OverrideCsTileLayout, 0, "Override compute walker tile layout enum class ThreadIDLayout", true)
DECLARE_IGC_REGKEY_ENUM(OverrideHWGenerateLID, -1,
                        "Override HW Generate Local ID setting"
                        "-1 - default behavior,"
                        " 0 - force disabled,"
                        " 1 - force enabled",
                        TRIBOOL_OPTIONS, true)
DECLARE_IGC_REGKEY(DWORD, MemCpyLoweringUnrollThreshold, 12,
                   "Min number of mem instructions that require non-unrolled loop when lowering memcpy", false)
DECLARE_IGC_REGKEY(DWORD, EnablePrivMemNewSOATranspose, 1,
                   "0 : disable new algo; 1 and up : enable new algo. "
                   "1 : enable new algo just for array of struct; "
                   "2 : 1 plus new algo for array of dw[xn]/qw[xn],etc "
                   "3 : 2 plus new algo for array of complicated struct.",
                   true)
DECLARE_IGC_REGKEY(bool, NewSOATransposeForOpenCL, true,
                   "If true, EnablePrivMemNewSOATranspose only applies to OpenCL kernels. For testing purpose", true)
DECLARE_IGC_REGKEY(bool, EnableSOAPromotionDisablingHeuristic, false,
                   "Enable heuristic to disable SOA promotion when it may be not beneficial", false)
DECLARE_IGC_REGKEY(bool, DisableSOAPromotion, false,
                   "If true, SOA cannot be used (private memory transposition). For testing purpose", true)
DECLARE_IGC_REGKEY(bool, DisableCSContentCheck, false, "Disable CS content check that can force SIMD32", true)
DECLARE_IGC_REGKEY(bool, DisableFastMathConstantHandling, false, "Disable Fast Math Constant Handling", true)
DECLARE_IGC_REGKEY_ENUM(SupportUniformPrivateMemorySpace, -1,
                        "Controls the behavior of PrivateMemoryResolution to emit uniform private memory allocas to "
                        "reduce the memory consumption."
                        "-1 - default behavior, the pass is enabled based on the API type or AILs"
                        " 0 - force disabled"
                        " 1 - force enabled",
                        TRIBOOL_OPTIONS, true)
DECLARE_IGC_REGKEY_ENUM(ShortImplicitPayloadHeader, -1,
                        "Controls the behavior of implicit kernel argument 'payloadHeader'."
                        "-1 - platform default"
                        " 0 - force old 8xi32 payloadHeader"
                        " 1 - force 3xi32 payloadHeader (global_id_offset only)",
                        TRIBOOL_OPTIONS, true)
DECLARE_IGC_REGKEY_ENUM(RemoveUnusedIdImplicitArguments, -1,
                        "Remove implicit arguments: global_id_offset (payloadHeader) and/or enqueued_local_size if "
                        "unused. Useful if kernel doesn't use global id."
                        "-1 - platform default"
                        " 0 - force disabled"
                        " 1 - force enabled",
                        TRIBOOL_OPTIONS, true)
DECLARE_IGC_REGKEY_ENUM(RemoveUnusedIdImplicitLocalIDs, -1,
                        "Remove implicit arguments localIDs if unused."
                        "-1 - platform default"
                        " 0 - force disabled"
                        " 1 - force enabled",
                        TRIBOOL_OPTIONS, true)
DECLARE_IGC_REGKEY(bool, RemoveImplicitScratchPointer, true,
                   "Allows skipping scratch pointer implicit kernel argument if unused. If false, arg is always added.",
                   true)
DECLARE_IGC_REGKEY(int, RemoveImplicitScratchPointerInstThreshold, 2000,
                   "Maximum number of instructions in kernel for which scratch pointer is considered for removal.",
                   true)
DECLARE_IGC_REGKEY(bool, AllowCrossBlockMatchMad, false,
                   "Enable cross basic block matching of mad instructions. This may lead to increased register "
                   "pressure, but in exchange, may reduce instruction count",
                   false)
DECLARE_IGC_REGKEY(
    bool, AllowMultipleMulUsesMatchMad, false,
    "Enable a multiply instruction with multiple uses to be matched to a mad instruction. This essentially forces the "
    "recalculation of the intermediate multiply result for every potential mad instruction, which will have "
    "performance impacts but may reduce instruction count and register pressure in case both mul operands need to be "
    "live past the add/sub but the intermediate mul result does not.",
    false)
DECLARE_IGC_REGKEY(bool, AllowConstMadOpMovToReg, false,
                   "Enable matching of mad instruction if constant greater than 16-bits. This will generate a mov in "
                   "vISA for the constant operand due to it not fitting as an imm16 operand. At this point, the "
                   "generated asm likely will fall back onto mul+add for the main case where src1 is the constant",
                   false)

DECLARE_IGC_GROUP("Generating precompiled headers")
DECLARE_IGC_REGKEY(bool, ApplyConservativeRastWAHeader, true,
                   "Apply WaConservativeRasterization for the platforms enabled", false)

DECLARE_IGC_GROUP("Raytracing Options")
DECLARE_IGC_REGKEY(bool, DisableEntryFences, false, "Don't emit the evict and invalidate fences for A0 WA", false)
DECLARE_IGC_REGKEY(bool, EnableRayTracingTGMFence, false, "Enable tgm fence in RT workloads for debugging", false)
DECLARE_IGC_REGKEY(bool, RayTracingDumpYaml, false, "Dump yaml input/output files", true)
DECLARE_IGC_REGKEY(bool, EnableCompressedRayIndices, false,
                   "Use an alternate form with bit twiddling to pack stack pointer and indices into two DWORDs", true)
DECLARE_IGC_REGKEY(bool, ForceNullBVH, false, "Swap BVH with null pointer. Infinitely fast ray traversal.", true)
DECLARE_IGC_REGKEY(bool, DisableFuseContinuations, false,
                   "If set, we will look for small duplicated continuations to merge into one.", true)
DECLARE_IGC_REGKEY(bool, DisableMatchRegisterRegion, false, "Disable matching for debug purposes", true)
DECLARE_IGC_REGKEY(bool, DisableCanonizationWA, false,
                   "WA for A0 to inject shifts to canonize global and local pointers", true)
DECLARE_IGC_REGKEY(bool, DisableEarlyRemat, false, "Disable quick remats to avoid some spills", true)
DECLARE_IGC_REGKEY(bool, DisableLateRemat, false, "Disable quick remats to avoid some spills", true)
DECLARE_IGC_REGKEY(DWORD, RematThreshold, 6, "Tunes how aggresively we should remat values into continuations", true)
DECLARE_IGC_REGKEY(bool, DisableCompactifySpills, false, "Just emit spill/fill at the point of def/use", true)
DECLARE_IGC_REGKEY(bool, AllowSpillCompactionOnRetry, false, "Allow spill compaction on retry - may increase spills",
                   true)
DECLARE_IGC_REGKEY(bool, EnableHoistRemat, false,
                   "Hoist rematerialized instructions to shader entry. Longer live ranges but common values fused.",
                   true)
DECLARE_IGC_REGKEY(bool, DisableRTGlobalsKnownValues, false,
                   "load MaxBVHLevels from RTGlobals rather than assumming = 2", true)
DECLARE_IGC_REGKEY(bool, DisableRaytracingIntrinsicAttributes, false, "Turn off noalias and dereferenceable attributes",
                   true)
DECLARE_IGC_REGKEY(bool, DisablePayloadSinking, false, "sink stores to payload into inlined continuations", true)
DECLARE_IGC_REGKEY(DWORD, ContinuationInlineThreshold, 1,
                   "If number of continuations is greater than threshold, default to indirect", true)
DECLARE_IGC_REGKEY(bool, EnableInlinedContinuations, false, "Forcibly inline all continuations", true)
DECLARE_IGC_REGKEY(bool, EnableIndirectContinuations, false,
                   "Enable BTD for continuation shaders (regardless of inline threshold).", true)
DECLARE_IGC_REGKEY(bool, ForceWholeProgramCompile, false, "Compile as if we know all of the shaders upfront", true)
DECLARE_IGC_REGKEY(bool, DeferCollectionStateObjectCompilation, false, "Wait to compile till the RTPSO stage", true)
DECLARE_IGC_REGKEY(bool, ForceFirstFencesEvict, false, "Force evict fence op on fences prior to the stack ID release",
                   true)
DECLARE_IGC_REGKEY(bool, RayTracingKeepUDivRemWA, false, "Workaround till jitIsa supports cr0 for rtz conversions",
                   true)
DECLARE_IGC_REGKEY(bool, DisablePromoteToScratch, false, "Use scratch space rather than SWStack when possible.", true)
DECLARE_IGC_REGKEY(bool, DisableInvariantLoad, false, "Disabled !invariant_load metadata for raytracing shaders", true)
DECLARE_IGC_REGKEY(bool, DisablePreSplitOpts, false, "Disable last minute optimizations befoer shader splitting", true)
DECLARE_IGC_REGKEY(bool, EnableKnownBTIBase, false, "For testing, assume that we know what baseBTI is in RTGlobals",
                   true)
DECLARE_IGC_REGKEY(DWORD, KnownBTIBaseValue, 0, "If EnableKnownBTIBase is set, use this value for baseBTI", true)
DECLARE_IGC_REGKEY(bool, DisableStatefulRTStackAccess, false,
                   "do stateless rather than stateful accesses to the HW portion of the async stack", true)
DECLARE_IGC_REGKEY(bool, DisableStatefulRTSyncStackAccess, true,
                   "do stateless rather than stateful accesses to the HW portion of the sync stack", true)
DECLARE_IGC_REGKEY(bool, DisableStatefulSWHotZoneAccess, false,
                   "do stateless rather than stateful accesses to the SW HotZone", true)
DECLARE_IGC_REGKEY(bool, DisableStatefulSWStackAccess, false,
                   "do stateless rather than stateful accesses to the SW Stack", true)
DECLARE_IGC_REGKEY(bool, DisableStatefulRTSyncStackAccess4RTShader, true,
                   "do stateless rather than stateful accesses to the HW portion of the sync stack. RT Shader only.",
                   true)
DECLARE_IGC_REGKEY(bool, DisableStatefulRTSyncStackAccess4nonRTShader, true,
                   "do stateless rather than stateful accesses to the HW portion of the sync stack. nonRT Shader only.",
                   true)
DECLARE_IGC_REGKEY(bool, DisableRTBindlessAccess, false,
                   "do bindful rather than bindless accesses to raytracing memory", true)
DECLARE_IGC_REGKEY(bool, DisableRTStackOpts, false,
                   "Disable some optimizations that minimize reads/writes to the RTStack", true)
DECLARE_IGC_REGKEY(bool, DisablePrepareLoadsStores, false, "Disable preparation for MemOpt", true)
DECLARE_IGC_REGKEY(bool, DisableRayTracingConstantCoalescing, false, "Disable coalescing", true)
DECLARE_IGC_REGKEY(bool, DisableMergeAllocas, false, "Do not merge allocas prior to SplitAsyncPass", false)
DECLARE_IGC_REGKEY(bool, DisableMergeAllocasPrivateMemory, false,
                   "Do not merge allocas prior to PrivateMemoryResolution", true)
DECLARE_IGC_REGKEY(bool, DisableMergingOfMultipleAllocasWithOffset, true,
                   "Do not merge multiple smaller allocas under one larger one with different offsets.", true)
DECLARE_IGC_REGKEY(bool, DisableMergingOfAllocasWithDifferentType, true, "Do not merge allocas of different types.",
                   true)
DECLARE_IGC_REGKEY(DWORD, RayTracingConstantCoalescingMinBlockSize, 4,
                   "Set the minimum load size in # OWords = [1,2,4,8,16].", true)
DECLARE_IGC_REGKEY(bool, DisableRayTracingOptimizations, false, "Disable RayTracing Optimizations for debugging", true)
DECLARE_IGC_REGKEY(DWORD, RayTracingCustomTileXDim1D, 0, "X dimension of tile (default: DG2=256, Xe2+=512)", true)
DECLARE_IGC_REGKEY(DWORD, RayTracingCustomTileYDim1D, 0, "Y dimension of tile (default: 1)", true)
DECLARE_IGC_REGKEY(DWORD, RayTracingCustomTileXDim2D, 0, "X dimension of tile (default: 32)", true)
DECLARE_IGC_REGKEY(DWORD, RayTracingCustomTileYDim2D, 0, "Y dimension of tile (default: 4 for XE, 32 for XE2+)", true)
DECLARE_IGC_REGKEY(bool, DisableLSCControlsForRayTracing, false,
                   "Disable different LSC Controls for HW and SW portions of the RTStack", true)
DECLARE_IGC_REGKEY(bool, ForceRTStackLoadCacheCtrl, false,
                   "Enables RTStackLoadCacheCtrl regkey for custom lsc load cache controls in the RTStack", true)
DECLARE_IGC_REGKEY_ENUM(RTStackLoadCacheCtrl, 0, "Load Cache Controls", LSC_CACHE_CTRL_OPTIONS, true)
DECLARE_IGC_REGKEY(bool, ForceRTStackStoreCacheCtrl, false,
                   "Enables RTStackStoreCacheCtrl regkey for custom lsc store cache controls in the RTStack", true)
DECLARE_IGC_REGKEY_ENUM(RTStackStoreCacheCtrl, 0, "Store Cache Controls", LSC_CACHE_CTRL_OPTIONS, true)
DECLARE_IGC_REGKEY(bool, ForceSWStackLoadCacheCtrl, false,
                   "Enables SWStackLoadCacheCtrl regkey for custom lsc load cache controls in the SWStack", true)
DECLARE_IGC_REGKEY_ENUM(SWStackLoadCacheCtrl, 0, "Load Cache Controls", LSC_CACHE_CTRL_OPTIONS, true)
DECLARE_IGC_REGKEY(bool, ForceSWStackStoreCacheCtrl, false,
                   "Enables SWStackStoreCacheCtrl regkey for custom lsc store cache controls in the SWStack", true)
DECLARE_IGC_REGKEY_ENUM(SWStackStoreCacheCtrl, 0, "Store Cache Controls", LSC_CACHE_CTRL_OPTIONS, true)
DECLARE_IGC_REGKEY(bool, ForceSWHotZoneLoadCacheCtrl, false,
                   "Enables SWHotZoneLoadCacheCtrl regkey for custom lsc load cache controls in the SWHotZone", true)
DECLARE_IGC_REGKEY_ENUM(SWHotZoneLoadCacheCtrl, 0, "Load Cache Controls", LSC_CACHE_CTRL_OPTIONS, true)
DECLARE_IGC_REGKEY(bool, ForceSWHotZoneStoreCacheCtrl, false,
                   "Enables SWHotZoneStoreCacheCtrl regkey for custom lsc store cache controls in the SWHotZone", true)
DECLARE_IGC_REGKEY_ENUM(SWHotZoneStoreCacheCtrl, 0, "Store Cache Controls", LSC_CACHE_CTRL_OPTIONS, true)
DECLARE_IGC_REGKEY(bool, ForceGenMemLoadCacheCtrl, false,
                   "Enables GenMemLoadCacheCtrl regkey for custom lsc load cache controls in other memory", true)
DECLARE_IGC_REGKEY_ENUM(GenMemLoadCacheCtrl, 0, "Load Cache Controls", LSC_CACHE_CTRL_OPTIONS, true)
DECLARE_IGC_REGKEY(bool, ForceGenMemStoreCacheCtrl, false,
                   "Enables GenMemStoreCacheCtrl regkey for custom lsc store cache controls in other memory", true)
DECLARE_IGC_REGKEY_ENUM(GenMemStoreCacheCtrl, 0, "Store Cache Controls", LSC_CACHE_CTRL_OPTIONS, true)
DECLARE_IGC_REGKEY(bool, ForceRTConstantBufferCacheCtrl, false,
                   "Enables RTConstantBufferCacheCtrl regkey for custom lsc load cache controls for constant buffers",
                   true)
DECLARE_IGC_REGKEY_ENUM(RTConstantBufferCacheCtrl, 0, "Constant Buffer Load Cache Controls for raytracing shaders",
                        LSC_CACHE_CTRL_OPTIONS, true)
DECLARE_IGC_REGKEY(
    bool, ForceGenMemDefaultCacheCtrl, false,
    "If enabled, no message specific cache ctrls are set on memory outside of RTStack, SWStack, and SWHotZone", true)
DECLARE_IGC_REGKEY(bool, EnableRTPrintf, false, "Enable printf for ray tracing.", true)
DECLARE_IGC_REGKEY(DWORD, PrintfBufferSize, 0, "Set printf buffer size. Unit: KB.", true)
DECLARE_IGC_REGKEY(bool, DisableRayQueryReturnOptimization, false, "RayQuery Return Optimization", true)
DECLARE_IGC_REGKEY_BITMASK(UseNewInlineRaytracing, 4, "Use the new rayquery implementation for particular case",
                           NEW_INLINE_RAYTRACING_MASK, true)
DECLARE_IGC_REGKEY(DWORD, AddDummySlotsForNewInlineRaytracing, 0,
                   "Add dummy rayquery slots when doing new inline raytracing", true)
DECLARE_IGC_REGKEY(
    bool, UseCrossBlockLoadVectorizationForInlineRaytracing, true,
    "If enabled, will try to vectorize loads that are not adjacent to each other. May increase GRF pressure", true)
DECLARE_IGC_REGKEY(bool, OverrideRayQueryThrottling, false,
                   "Force rayquery throttling (dynamic ray management) to be enabled or disabled. Default value of "
                   "this key is ignored",
                   true)
DECLARE_IGC_REGKEY(bool, DisableRayQueryDynamicRayManagementMechanismForExternalFunctionsCalls, false,
                   "Disable dynamic ray management mechanism for shaders with external functions calls", true)
DECLARE_IGC_REGKEY(bool, DisableRayQueryDynamicRayManagementMechanismForBarriers, false,
                   "Disable dynamic ray management mechanism for shaders with barriers", true)
DECLARE_IGC_REGKEY(bool, EnableOuterLoopHoistingForRayQueryDynamicRayManagementMechanism, false,
                   "Disable dynamic ray management mechanism for shaders with barriers", true)
DECLARE_IGC_REGKEY(bool, DisableProceedBasedApproachForRayQueryDynamicRayManagementMechanism, false,
                   "Disables proceed based approach for dynamic ray management mechanism", true)
DECLARE_IGC_REGKEY(bool, DisableInvalidateRTStackAfterLastRead, true,
                   "Disables L1 cache invalidation after the last read of the RT stack. Affects rayqueries only", true)
DECLARE_IGC_REGKEY(bool, DisableSWSubTriangleOpacityCullingEmulation, false,
                   "Software Sub-Triangle Opacity Culling emulation", true)
DECLARE_IGC_REGKEY(bool, DisableRayTracingExtendedCacheControl, false,
                   "Disables the Extended Cache Control for Raytracing.", false)
DECLARE_IGC_REGKEY(bool, DisableNewRTStackLayoutOptimization, false,
                   "Ray Tracing New Stack Layout sync/async trace ray message optimization", false)
DECLARE_IGC_REGKEY(bool, EnableNewBTDIndirect0DescriptorProgramming, true,
                   "Due to Bspec error globals pointer is always shifted by 6 bits in BTDIndirect0Descriptor.\
                                                                                 This flag enables BTDIndirect0Descriptor programming without this shift.",
                   false)
DECLARE_IGC_REGKEY(bool, EnableDoNotSendPayloadForCheckReleaseInEff64, true,
                   "According to Bspec payload should not be send for RayQuery Check/Release messages.", false)
DECLARE_IGC_REGKEY(bool, DisableWideTraceRay, false, "Disable SIMD16 style message payloads for send.rta", true)
DECLARE_IGC_REGKEY(bool, ForceRTCheckInstanceLeafPtr, true,
                   "Check MemHit::valid before loading GeometryIndex, PrimitiveIndex, etc.", true)
DECLARE_IGC_REGKEY(DWORD, RTInValidDefaultIndex, 0xFFFFFFFF,
                   "If MemHit::valid is false, the default value to return for some intrinsics like GeometryIndex or "
                   "PrimitiveIndex etc.",
                   true)
DECLARE_IGC_REGKEY(DWORD, ForceRTCheckInstanceLeafPtrMask, 0xF, "Test only. 1: committedindex; 2: potentialindex", true)
DECLARE_IGC_REGKEY(bool, ForceRTShortCircuitingOR, true,
                   "Only for specific test.... Short curcite OR condition if CommittedGeometryIndex is used", true)
DECLARE_IGC_REGKEY(DWORD, RTFenceToggle, 0, "Toggle fences", true)
DECLARE_IGC_REGKEY(bool, EnableLSCCacheOptimization, false,
                   "Optimize store instructions for utilizing the LSC-L1 cache", false)
DECLARE_IGC_REGKEY(bool, EnableSingleRQMemRayStore, true, "Store RayQuery MemRay[TOP] only once.", false)
DECLARE_IGC_REGKEY(DWORD, TotalGRFNum4RQ, 0,
                   "Total GRF used for register allocation for RayQuery only. Test only. Delete later.", false)
DECLARE_IGC_REGKEY(bool, ForceCSLeastSIMD4RQ, false,
                   "Force computer shader with RayQuery to the lowest allowed SIMD mode", false)
DECLARE_IGC_REGKEY(DWORD, ForceCSSimdSize4RQ, 0, "Force RayQuery compute shader simd size,\
                                                      valid values are 0 (not set), 8, 16 and 32\
                                                      ignored if produces invalid cofiguration, e.g. simd size too small for workgroup size",
                   true)
DECLARE_IGC_REGKEY(bool, EnableRQHideLatency, false, "Hide RayQuery Proceed latency.", false)
DECLARE_IGC_REGKEY(bool, DisableShaderFusion, false, "Don't check for duplicate, renamed shaders", false)
DECLARE_IGC_REGKEY(DWORD, ShaderFusionThrehold, 1000,
                   "If there are less shaders than this, don't spend time checking duplicates", false)
DECLARE_IGC_REGKEY(bool, DisableRTAliasAnalysis, false, "Disable Raytracing Alias Analysis", false)
DECLARE_IGC_REGKEY(bool, DisableExamineRayFlag, false,
                   "Don't do IPO to see if we can fold control flow given knowledge of possible rayflag values", false)
DECLARE_IGC_REGKEY(bool, DisableSpillReorder, false, "Disables reordering of spills to try to minmize spills in a loop",
                   false)
DECLARE_IGC_REGKEY(bool, DisablePromoteContinuation, false,
                   "BTD-able continuations in the raygen may be moved to the shader identifier", false)
DECLARE_IGC_REGKEY(bool, EnableStackIDReleaseScheduling, false,
                   "Schedule Stack ID Release messages prior to the end of the shader", false)
DECLARE_IGC_REGKEY(bool, DisableRTMemDSE, false,
                   "Analyze stores to SWStack, etc. that aren't read before Stack ID Release", false)
DECLARE_IGC_REGKEY(bool, DisableRTFenceElision, false, "Disable optimization to remove unneeded fences", false)
DECLARE_IGC_REGKEY(bool, DisableDPSE, false, "Disable Dead PayloadStore Elimination.", true)
DECLARE_IGC_REGKEY(bool, EnableRTDispatchAlongY, false, "Dispatch Compute Walker along Y first", true)
DECLARE_IGC_REGKEY(bool, DisablePredicatedStackIDRelease, false,
                   "Emit a single stack ID release at the end of the shader", true)
DECLARE_IGC_REGKEY(bool, DisableCrossFillRemat, false, "Rematerialize values if they use already spilled values", true)
DECLARE_IGC_REGKEY(bool, EnableSyncDispatchRays, false, "Enable sync DispatchRays implementation", false)
DECLARE_IGC_REGKEY(bool, ForceRTRetry, false, "Raytracing is compiled in the second retry state", false)
DECLARE_IGC_REGKEY(
    bool, DisableRTRetryPickBetter, false,
    "Disables raytracing retry to pick the best compilation instead of always using the retry compilation.", false)
DECLARE_IGC_REGKEY(DWORD, RetryRTSpillMemThreshold, 200, "Only retry if spill mem used is more than this value", false)
DECLARE_IGC_REGKEY(DWORD, RetryRTSpillCostThreshold, 5,
                   "Only retry if the percentage of spills (over total instructions) is more than this value", false)
DECLARE_IGC_REGKEY(DWORD, RetryRTPickBetterThreshold, 10,
                   "Only pick the retry shader if the spill cost of the 2nd compilation is at least this percentage "
                   "better than the previous compilation",
                   false)
DECLARE_IGC_REGKEY(bool, EnableFillScheduling, false, "Schedule fills for reduced register pressure", false)
DECLARE_IGC_REGKEY(bool, DisableSWStackOffsetElision, false, "Avoid loading offseting when known at compile-time",
                   false)
DECLARE_IGC_REGKEY(DWORD, OverrideTMax, 0, "Force TMax to the given value. When 0, do nothing.", false)
DECLARE_IGC_REGKEY(bool, DisableLoadAsFenceOpInRaytracing, true,
                   "Disable load as fence op in raytracing (rayquery only)", false)

DECLARE_IGC_GROUP("VectorCompiler Options")
DECLARE_IGC_REGKEY(bool, DisableEuFusion, false, "Require disable of EU fusion", true)
DECLARE_IGC_REGKEY(bool, VCOptimizeNone, false, "Same as -optimize=none in vector compiler options", true)
DECLARE_IGC_REGKEY(bool, VCStrictOptionParser, true, "Produce error on unknown API options in vector compiler", true)
DECLARE_IGC_REGKEY(debugString, VCApiOptions, 0, "Extra API options for VC", true)
DECLARE_IGC_REGKEY(debugString, VCInternalOptions, 0, "Extra Internal options to pass to VC", true)
DECLARE_IGC_REGKEY(bool, VCLocalizeAccUsage, false, "Localization of possible accumulator usages for vISA RA", true)
DECLARE_IGC_REGKEY(bool, VCDisableNonOverlappingRegionOpt, false, "Disable non-overlapping region optimization", true)
DECLARE_IGC_REGKEY(bool, VCEnableExtraDebugLogging, false,
                   "Turns on extra debug output to trace IGC/VC-specific execution", true)
DECLARE_IGC_REGKEY(DWORD, VCNoOptFinalizerControl, 0, "Controls if finalizer is invoked with -debug flag", true)
DECLARE_IGC_REGKEY(DWORD, VCDisableLRCoalescingControl, 0, "Controls if LR coalescing", true)
DECLARE_IGC_REGKEY(DWORD, VCDisableExtraCoalescing, 0, "Disable extra coalescing", true)
DECLARE_IGC_REGKEY(bool, VCSaveStackCallLinkage, false, "Do not override stack calls linkage as internal", true)
DECLARE_IGC_REGKEY(bool, VCDirectCallsOnly, false, "Generate code under the assumption all unknown calls are direct",
                   true)
DECLARE_IGC_REGKEY(DWORD, VCLoopUnrollThreshold, 0,
                   "Set the loop unroll threshold for VC. Value 0 will use the default threshold.", true)
DECLARE_IGC_REGKEY(bool, VCIgnoreLoopUnrollThresholdOnPragma, false,
                   "Ignore threshold for loop unrolling when pragma is used", true)
DECLARE_IGC_REGKEY(DWORD, VCDepressurizerGRFThreshold, 2560, "Threshold for GRF pressure reduction", true)
DECLARE_IGC_REGKEY(DWORD, VCDepressurizerFlagGRFTolerance, 3840, "Threshold for disabling flag pressure reduction",
                   true)

