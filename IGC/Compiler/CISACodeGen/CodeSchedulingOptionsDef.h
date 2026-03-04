/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifdef DECLARE_SCHEDULING_OPTION

// For the usage with IGC_CodeSchedulingConfig option, see CodeScheduling.cpp, class SchedulingConfig
// Publicly available options are in igc_flags.h, search for CodeScheduling*

// Generate default options line:
// clang-format off
// python3 -c "print('IGC_CodeSchedulingConfig=\"' + ';'.join([line.split(',')[1].strip() for line in open('CodeSchedulingOptionsDef.h') if line.strip().startswith('DECLARE_SCHEDULING_OPTION')]) + '\"')"
// clang-format on

// Edge weights
DECLARE_SCHEDULING_OPTION(DefaultWeight, 10, "Default edge weight for dependency graph")
DECLARE_SCHEDULING_OPTION(UseHighRPWeight, 1, "Use alternative weights when register pressure is high")
DECLARE_SCHEDULING_OPTION(Weight2dBlockReadSrcDep, 0, "Edge weight for 2D block read source dependency")
DECLARE_SCHEDULING_OPTION(Weight2dBlockReadDstDep, 30000, "Edge weight for 2D block read destination dependency")
DECLARE_SCHEDULING_OPTION(Weight2dBlockReadDstDepHighRP, 50,
                          "Edge weight for 2D block read destination "
                          "dependency under high register pressure")
DECLARE_SCHEDULING_OPTION(Weight2dBlockSetPayloadFieldDstDep, 0,
                          "Edge weight for 2D block set payload field destination dependency")
DECLARE_SCHEDULING_OPTION(WeightPrefetch, 100000, "Edge weight for prefetch instructions")
DECLARE_SCHEDULING_OPTION(PrioritizeDataLoadsOverPrefetches, 1,
                          "Heuristic: When the ready list contains both a prefetch and a data load "
                          "with at least one DPAS consumer, always prefer the data load regardless "
                          "of MW comparison. (0=disabled; 1=enabled)")
DECLARE_SCHEDULING_OPTION(WeightDPASDstDep, 1000, "Edge weight for DPAS destination dependency")
DECLARE_SCHEDULING_OPTION(WeightDPASDstDepHighRP, 6000,
                          "Edge weight for DPAS destination dependency under high register pressure")
DECLARE_SCHEDULING_OPTION(WeightExtendedMathDstDep, 200, "Edge weight for extended math destination dependency")
DECLARE_SCHEDULING_OPTION(WeightWaveAllDstDep, 10, "Edge weight for wave all destination dependency")
DECLARE_SCHEDULING_OPTION(WeightWaveAllDstDepHighRP, 20,
                          "Edge weight for wave all destination dependency under high "
                          "register pressure")
DECLARE_SCHEDULING_OPTION(WeightUnknownMemoryReadDstDep, 500,
                          "Edge weight for unknown memory read destination dependency")
DECLARE_SCHEDULING_OPTION(WeightUnknownVectorShuffleDstDep, 50,
                          "Edge weight for unknown vector shuffle destination dependency")
DECLARE_SCHEDULING_OPTION(LoadSizeAdditionalWeight, 0,
                          "Add this weight * multiplier * load size to the basic load weight")
DECLARE_SCHEDULING_OPTION(LoadSizeWeightFactor, 1,
                          "Add additional weight * this multiplier * load size "
                          "to the basic load weight")
DECLARE_SCHEDULING_OPTION(WeightLoadDPASPositionBonus, 200,
                          "Additional edge weight per DPAS position closer to the beginning "
                          "of the block for 2D block read destination dependencies")
DECLARE_SCHEDULING_OPTION(WeightSharedLoadBonus, 200,
                          "Additional edge weight for 2D block read destination dependencies "
                          "when the load feeds multiple DPAS instructions")
DECLARE_SCHEDULING_OPTION(AddWeightToTerminatorEdge, 1,
                          "Add weight to the edge from the last instruction in "
                          "the block to the terminator instruction (0/1)")

// Heuristics
DECLARE_SCHEDULING_OPTION(PrioritizeLargeBlockLoadsInRP, 0,
                          "Heuristic: Prioritize block loads larger than the "
                          "value when register pressure is high. 0 is disabled")
DECLARE_SCHEDULING_OPTION(PrioritizeDPASHighRP, 1,
                          "Heuristic: Prioritize DPAS instructions under high register pressure")
DECLARE_SCHEDULING_OPTION(PrioritizeDPASAndOtherOverImmediateVS, 1,
                          "Heuristic: Prioritize DPAS and some other instructions before vector "
                          "shuffle no-noop patterns that are supposed to be immediate, "
                          " but only when the instruction doesn't increase register pressure significantly")
DECLARE_SCHEDULING_OPTION(PrioritizeOverImmediateVSMaxRPInBytes, 8,
                          "Heuristic: Maximum register pressure in bytes to consider for prioritizing "
                          "DPAS and other instructions over immediate vector shuffle patterns")
DECLARE_SCHEDULING_OPTION(PrioritizeLoadsThatUnlockDPASesHighRP, 1,
                          "Heuristic: Prioritize loads that unlock DPAS "
                          "instructions under high register pressure")
DECLARE_SCHEDULING_OPTION(PrioritizeLoadsThatUnlockDPASesHighRP_MaxLoadSize, 32,
                          "Heuristic: Maximum load size (in number of elements) to consider for "
                          "prioritizing loads that unlock DPAS instructions. "
                          "The value is for SIMD16, for SIMD32 it will be divided by 2")
DECLARE_SCHEDULING_OPTION(PreferEarliestDPASInUnlock, 1,
                          "Heuristic: When multiple loads unlock different DPAS instructions, "
                          "prefer the one that unlocks the earliest DPAS")
DECLARE_SCHEDULING_OPTION(FocusLoadsOnOneDPAS, 1,
                          "Heuristic: Focus loads on one DPAS instruction in case we have to choose from "
                          "many loads")
DECLARE_SCHEDULING_OPTION(TiebreakByDPASConsumerPosition, 1,
                          "Heuristic: When breaking ties in scheduling, prefer 2D block reads "
                          "that feed earlier DPAS instructions. Among loads feeding the same "
                          "DPAS, prefer shared loads (multiple consumers) over unique loads")
DECLARE_SCHEDULING_OPTION(AllowLargerRPWindowRPThreshold, 200,
                          "Heuristic: Allow larger register pressure window if register pressure is higher than "
                          "a threshold, so allow also the instructions that have not lowest but similar register "
                          "pressure, the threshold in bytes")
DECLARE_SCHEDULING_OPTION(AllowLargerRPWindowSize, 64,
                          "Heuristic: Allow larger register pressure window if register pressure is higher than "
                          "a threshold, so allow also the instructions that have not lowest but similar register "
                          "pressure, the size of the window in bytes")
DECLARE_SCHEDULING_OPTION(PrioritizeMaxnumWaveallHighRP, 0,
                          "Heuristic: Maxnum and Waveall instructions are prioritized when register pressure is "
                          "high")
DECLARE_SCHEDULING_OPTION(PrioritizePopulatingOneVectorHighRP, 1,
                          "Heuristic: Prioritize populating one vector when register pressure is high")
DECLARE_SCHEDULING_OPTION(DeferDistantLoads, 1,
                          "Heuristic: Exclude loads whose consumer DPAS is far from the earliest "
                          "consumer DPAS in the ready list, deferring them to a later round")
DECLARE_SCHEDULING_OPTION(DeferDistantLoadsThreshold, 32,
                          "Heuristic: Maximum DPAS position distance to consider a load as near. "
                          "Loads whose consumer is further than this from the earliest consumer "
                          "get deferred")
DECLARE_SCHEDULING_OPTION(FireChainGroupHeads, 1,
                          "Heuristic: When a DPAS chain head fires, immediately prioritize "
                          "remaining chain heads in the same group. Also proactively fire all "
                          "group heads when RP approaches the threshold")
DECLARE_SCHEDULING_OPTION(FireChainGroupHeadsMinLoadBytes, 512,
                          "Heuristic: Minimum load size in bytes for the proactive chain group "
                          "head firing trigger")
DECLARE_SCHEDULING_OPTION(FireChainGroupHeadsMaxBoostHeads, 0,
                          "Experimental heuristic: Maximum number of sibling chain heads to boost when "
                          "reactive mode fires. Limits how many heads are added to the ready "
                          "list at once to avoid disrupting the scheduler's natural operand-aware "
                          "pairing on large groups. 0 = no limit (default)")
DECLARE_SCHEDULING_OPTION(LimitActiveLargeLoads, 32,
                          "Heuristic: Only allow one large load to be active at a time. "
                          "Value is in i32 elements for SIMD16, internally converted to bytes "
                          "(value * 4) and compared against load byte size. 0=disabled")

// RP management control options
DECLARE_SCHEDULING_OPTION(GreedyRPThresholdDelta, 20, "Threshold delta for greedy register pressure scheduling")
DECLARE_SCHEDULING_OPTION(LowRPThresholdDelta, 200, "Unused: Threshold delta for low register pressure")
DECLARE_SCHEDULING_OPTION(MinLiveIntervalForCloning, 200, "Minimum live interval for cloning instructions")
DECLARE_SCHEDULING_OPTION(ReservedRegisters, 5, "Number of always reserved registers")
DECLARE_SCHEDULING_OPTION(LargeBlockLoadSize, 16,
                          "Size of large load to always make a checkpoint, in number of elements. "
                          "The value is for SIMD16, for SIMD32 it will be divided by 2")
DECLARE_SCHEDULING_OPTION(LargeLoadSizeForFragmentationAdjustment, 16,
                          "Size of large load to consider for fragmentation "
                          "adjustment, in number of elements, the value is for SIMD16, "
                          "for SIMD32 it will be divided by 2")
DECLARE_SCHEDULING_OPTION(RPMarginIncreaseForFragmentationAdjustment, 34,
                          "Increase register pressure margin for fragmentation adjustment")
DECLARE_SCHEDULING_OPTION(FragmentationAdjustmentsMinGRF, 200,
                          "Minimum number of GRFs to apply fragmentation adjustments")
DECLARE_SCHEDULING_OPTION(IgnoreFragmentationForLastLoad, 1,
                          "Ignore fragmentation for the last load in the block, i.e. do not increase "
                          "register pressure margin for it")
DECLARE_SCHEDULING_OPTION(ApplyStaticFragAdjustmentToHighRP, 1,
                          "Apply RPMarginIncreaseForFragmentationAdjustment to "
                          "isRegpressureHigh() for large 2D block reads, in addition to "
                          "isRegpressureCritical(). Makes MW scheduling switch to high-RP "
                          "weights earlier when large loads cause fragmentation "
                          "(0=disabled, 1=enabled)")
DECLARE_SCHEDULING_OPTION(HighInitialPressureRPMargin, 20,
                          "Additional RP margin applied to both High and Critical thresholds "
                          "when the BB initial pressure exceeds HighInitialPressureThreshold. "
                          "Accounts for cross-BB live-range overhead in loop bodies (0=disabled)")
DECLARE_SCHEDULING_OPTION(HighInitialPressureThreshold, 180,
                          "Threshold for initial BB pressure (in GRFs) above which "
                          "HighInitialPressureRPMargin is applied")
// Load-density-based RA fragmentation compensation
DECLARE_SCHEDULING_OPTION(LargeLoadFragOverheadPerLoad, 2,
                          "Estimated GRF fragmentation overhead per 2D block read in the BB. "
                          "Accounts for contiguous allocation waste and alignment constraints "
                          "that the IR-level RP estimator does not model (0=disabled)")
DECLARE_SCHEDULING_OPTION(LargeLoadFragDivisor, 2,
                          "Divisor for total load-density fragmentation margin: "
                          "margin = NumBlockReads * OverheadPerLoad / Divisor. "
                          "Higher values make the heuristic less aggressive (0=disabled)")
DECLARE_SCHEDULING_OPTION(LargeLoadDensityMarginApplyToCritical, 0,
                          "Apply LargeLoadDensityMargin to isRegpressureCritical() "
                          "in addition to isRegpressureHigh(). 0=only applied to "
                          "isRegpressureHigh (avoids false spill predictions for BBs "
                          "that MW-schedule cleanly)")
DECLARE_SCHEDULING_OPTION(LargeLoadDensityMarginCriticalMaxInitialRP, 120,
                          "Apply LargeLoadDensityMargin to isRegpressureCritical() when "
                          "initialBBPressure < this threshold (in GRF units). 0 = never apply "
                          "based on this gate; use LargeLoadDensityMarginApplyToCritical for "
                          "unconditional control. Set to 120 to apply density margin to "
                          "RP_critical only for low-initial-pressure BBs")
DECLARE_SCHEDULING_OPTION(LargeLoadHighRPAdjustmentsMinInitialRP, 160,
                          "Apply large-load RP_high adjustments (LargeLoadDensityMargin + "
                          "ApplyStaticFragAdjustmentToHighRP) only when "
                          "initialBBPressure >= this threshold (in GRF units). 0 = always apply "
                          "(current behavior)")

// Other
DECLARE_SCHEDULING_OPTION(ForceSIMDSize, 0,
                          "Force SIMD mode for scheduling, 0 is no force, 8 is "
                          "SIMD8, 16 is SIMD16, etc.")
DECLARE_SCHEDULING_OPTION(DefaultNumGRF, 128,
                          "Default number of GRFs for scheduling, used when "
                          "the context does not provide it")
DECLARE_SCHEDULING_OPTION(DefaultNumGRFAuto, 256,
                          "Default number of GRFs for scheduling, used when the context does not "
                          "provide it, but auto is enabled")

#endif
