/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "JitterDataStruct.h"

using namespace vISA;

llvm::json::Value PERF_STATS::toJSON() {
  // llvm Json cannot support u64 type, force to print string for binaryHash
  return llvm::json::Object{
    {"binaryHash", std::to_string(binaryHash)},
    {"numGRFUsed", numGRFUsed},
    {"numGRFTotal", numGRFTotal},
    {"numThreads", numThreads},
    {"numAsmCount", numAsmCountUnweighted},
    {"numFlagSpillStore", numFlagSpillStore},
    {"numFlagSpillLoad", numFlagSpillLoad},
    {"numGRFSpillFill", numGRFSpillFillWeighted},
    {"GRFSpillSize", spillMemUsed},
    {"numCycles", numCycles},
    {"maxGRFPressure", maxGRFPressure}
  };
}

llvm::json::Value PERF_STATS_VERBOSE::toJSON() {
  llvm::json::Object jsonObject = llvm::json::Object{
      {"BCNum", BCNum},
      {"numByteRMWs", numByteRMWs},
      {"numALUInst", numALUInst},
      {"accSubDef", accSubDef},
      {"accSubUse", accSubUse},
      {"accSubCandidateDef", accSubCandidateDef},
      {"accSubCandidateUse", accSubCandidateUse},
      {"syncInstCount", syncInstCount},
      {"tokenReuseCount", tokenReuseCount},
      {"singlePipeAtOneDistNum", singlePipeAtOneDistNum},
      {"allAtOneDistNum", allAtOneDistNum},
      {"AfterWriteTokenDepCount", AfterWriteTokenDepCount},
  };
  if (RAIterNum) {
    jsonObject.insert({"RAIterNum", RAIterNum});
    jsonObject.insert({"varNum", varNum});
    jsonObject.insert({"globalVarNum", globalVarNum});
    jsonObject.insert({"maxRP", maxRP});
    jsonObject.insert({"maxNeighbors", maxNeighbors});
    jsonObject.insert({"avgNeighbors", avgNeighbors});
    jsonObject.insert({"normIntfNum", normIntfNum});
    jsonObject.insert({"augIntfNum", augIntfNum});
  }

  return jsonObject;
}