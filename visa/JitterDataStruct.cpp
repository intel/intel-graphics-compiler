/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "JitterDataStruct.h"

using namespace vISA;

constexpr char FINALIZER_INFO::kIsTrivialRA[];
constexpr char FINALIZER_INFO::kIsLocallRA[];
constexpr char FINALIZER_INFO::kIsHybridRA[];
constexpr char FINALIZER_INFO::kIsGlobalRA[];

llvm::json::Value PERF_STATS::toJSON() {
  // llvm Json cannot support u64 type, force to print string for binaryHash
  return llvm::json::Object{{"binaryHash", std::to_string(binaryHash)},
                            {"numGRFUsed", numGRFUsed},
                            {"numGRFTotal", numGRFTotal},
                            {"numThreads", numThreads},
                            {"numAsmCount", numAsmCountUnweighted},
                            {"numFlagSpillStore", numFlagSpillStore},
                            {"numFlagSpillLoad", numFlagSpillLoad},
                            {"numGRFSpillFill", numGRFSpillFillWeighted},
                            {"numCycles", numCycles}};
}

llvm::json::Value PERF_STATS_VERBOSE::toJSON() {
  return llvm::json::Object();
}
