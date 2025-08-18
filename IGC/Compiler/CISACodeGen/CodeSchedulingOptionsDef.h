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
DECLARE_SCHEDULING_OPTION(Weight2dBlockReadDstDepHighRP, 100,
                          "Edge weight for 2D block read destination "
                          "dependency under high register pressure")
DECLARE_SCHEDULING_OPTION(Weight2dBlockSetPayloadFieldDstDep, 0,
                          "Edge weight for 2D block set payload field destination dependency")
DECLARE_SCHEDULING_OPTION(WeightPrefetch, 100000, "Edge weight for prefetch instructions")
DECLARE_SCHEDULING_OPTION(WeightDPASDstDep, 1000, "Edge weight for DPAS destination dependency")
DECLARE_SCHEDULING_OPTION(WeightDPASDstDepHighRP, 6000,
                          "Edge weight for DPAS destination dependency under high register pressure")
DECLARE_SCHEDULING_OPTION(WeightExtendedMathDstDep, 200, "Edge weight for extended math destination dependency")
DECLARE_SCHEDULING_OPTION(WeightWaveAllDstDep, 10, "Edge weight for wave all destination dependency")
DECLARE_SCHEDULING_OPTION(WeightWaveAllDstDepHighRP, 20, "Edge weight for wave all destination dependency under high "
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
                          "prioritizing loads that unlock DPAS instructions")
DECLARE_SCHEDULING_OPTION(FocusLoadsOnOneDPAS, 1,
                          "Heuristic: Focus loads on one DPAS instruction in case we have to choose from "
                          "many loads")
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

// RP management control options
DECLARE_SCHEDULING_OPTION(GreedyRPThresholdDelta, 20, "Threshold delta for greedy register pressure scheduling")
DECLARE_SCHEDULING_OPTION(LowRPThresholdDelta, 200, "Unused: Threshold delta for low register pressure")
DECLARE_SCHEDULING_OPTION(MinLiveIntervalForCloning, 200, "Minimum live interval for cloning instructions")
DECLARE_SCHEDULING_OPTION(ReservedRegisters, 5, "Number of always reserved registers")
DECLARE_SCHEDULING_OPTION(LargeBlockLoadSize, 16,
                          "Size of large load to always make a checkpoint, in number of elements")
DECLARE_SCHEDULING_OPTION(LargeLoadSizeForFragmentationAdjustment, 16,
                          "Size of large load to consider for fragmentation "
                          "adjustment, in number of elements")
DECLARE_SCHEDULING_OPTION(RPMarginIncreaseForFragmentationAdjustment, 34,
                          "Increase register pressure margin for fragmentation adjustment")
DECLARE_SCHEDULING_OPTION(FragmentationAdjustmentsMinGRF, 200,
                          "Minimum number of GRFs to apply fragmentation adjustments")
DECLARE_SCHEDULING_OPTION(IgnoreFragmentationForLastLoad, 1,
                          "Ignore fragmentation for the last load in the block, i.e. do not increase "
                          "register pressure margin for it")

// Other
DECLARE_SCHEDULING_OPTION(ForceSIMDSize, 16,
                          "Force SIMD mode for scheduling, 0 is no force, 8 is "
                          "SIMD8, 16 is SIMD16, etc.")
DECLARE_SCHEDULING_OPTION(DefaultNumGRF, 128,
                          "Default number of GRFs for scheduling, used when "
                          "the context does not provide it")
DECLARE_SCHEDULING_OPTION(DefaultNumGRFAuto, 256,
                          "Default number of GRFs for scheduling, used when the context does not "
                          "provide it, but auto is enabled")

#endif
