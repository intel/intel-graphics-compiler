/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifdef DECLARE_SCHEDULING_OPTION

// For the usage with IGC_CodeSchedulingConfig option, see CodeScheduling.cpp, class SchedulingConfig
// Publicly available options are in igc_flags.h, search for CodeScheduling*

// Edge weigths
DECLARE_SCHEDULING_OPTION(DefaultWeight,                     10,     "Default edge weight for dependency graph")
DECLARE_SCHEDULING_OPTION(UseHighRPWeight,                   1,      "Use alternative weights when register pressure is high")
DECLARE_SCHEDULING_OPTION(Weight2dBlockReadSrcDep,           300,    "Edge weight for 2D block read source dependency")
DECLARE_SCHEDULING_OPTION(Weight2dBlockReadDstDep,           3000,   "Edge weight for 2D block read destination dependency")
DECLARE_SCHEDULING_OPTION(Weight2dBlockReadDstDepHighRP,     100,    "Edge weight for 2D block read destination dependency under high register pressure")
DECLARE_SCHEDULING_OPTION(Weight2dBlockSetPayloadFieldDstDep,30,     "Edge weight for 2D block set payload field destination dependency")
DECLARE_SCHEDULING_OPTION(WeightPrefetch,                    100000, "Edge weight for prefetch instructions")
DECLARE_SCHEDULING_OPTION(WeightDPASDstDep,                  1000,   "Edge weight for DPAS destination dependency")
DECLARE_SCHEDULING_OPTION(WeightDPASDstDepHighRP,            6000,   "Edge weight for DPAS destination dependency under high register pressure")
DECLARE_SCHEDULING_OPTION(WeightExtendedMathDstDep,          200,    "Edge weight for extended math destination dependency")
DECLARE_SCHEDULING_OPTION(WeightUnknownMemoryReadDstDep,     500,    "Edge weight for unknown memory read destination dependency")
DECLARE_SCHEDULING_OPTION(WeightUnknownVectorShuffleDstDep,  50,     "Edge weight for unknown vector shuffle destination dependency")

// Heurictics
DECLARE_SCHEDULING_OPTION(PrioritizeLargeBlockLoadsInRP,     32,     "Heuristic: Prioritize block loads larger than the value when register pressure is high")
DECLARE_SCHEDULING_OPTION(PrioritizeDPASHighRP,              0,      "Heuristic: Prioritize DPAS instructions under high register pressure")

// RP management control options
DECLARE_SCHEDULING_OPTION(GreedyRPThresholdDelta,            20,     "Threshold delta for greedy register pressure scheduling")
DECLARE_SCHEDULING_OPTION(LowRPThresholdDelta,               200,    "Unused: Threshold delta for low register pressure")
DECLARE_SCHEDULING_OPTION(MinLiveIntervalForCloning,         200,    "Minimum live interval for cloning instructions")
DECLARE_SCHEDULING_OPTION(ReservedRegisters,                 2,      "Number of always reserved registers")

// Other
DECLARE_SCHEDULING_OPTION(ForceSIMDSize,                     16,      "Force SIMD mode for scheduling, 0 is no force, 8 is SIMD8, 16 is SIMD16, etc.")
DECLARE_SCHEDULING_OPTION(DefaultNumGRF,                     128,    "Default number of GRFs for scheduling, used when the context does not provide it")
DECLARE_SCHEDULING_OPTION(DefaultNumGRFAuto,                 256,    "Default number of GRFs for scheduling, used when the context does not provide it, but auto is enabled")

#endif