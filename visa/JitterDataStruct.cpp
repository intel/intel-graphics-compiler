/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "JitterDataStruct.h"

namespace vISA {

llvm::json::Value toJSON(const PERF_STATS &p) {
  // llvm Json cannot support u64 type, force to print string for binaryHash
  return llvm::json::Object{
    {"binaryHash", std::to_string(p.binaryHash)},
    {"numGRFUsed", p.numGRFUsed},
    {"numGRFTotal", p.numGRFTotal},
    {"numThreads", p.numThreads},
    {"numAsmCount", p.numAsmCountUnweighted},
    {"numFlagSpillStore", p.numFlagSpillStore},
    {"numFlagSpillLoad", p.numFlagSpillLoad},
    {"numGRFSpillFill", p.numGRFSpillFillWeighted},
    {"GRFSpillSize", p.spillMemUsed},
    {"numCycles", p.numCycles},
    {"maxGRFPressurePreRA", p.maxGRFPressurePreRA},
    {"maxGRFPressure", p.maxGRFPressure}
  };
}

llvm::json::Value toJSON(const PERF_STATS_VERBOSE &p) {
  llvm::json::Object jsonObject = llvm::json::Object{
      {"BCNum", p.BCNum},
      {"numByteRMWs", p.numByteRMWs},
      {"numALUInst", p.numALUInst},
      {"accSubDef", p.accSubDef},
      {"accSubUse", p.accSubUse},
      {"accSubCandidateDef", p.accSubCandidateDef},
      {"accSubCandidateUse", p.accSubCandidateUse},
      {"syncInstCount", p.syncInstCount},
      {"tokenReuseCount", p.tokenReuseCount},
      {"singlePipeAtOneDistNum", p.singlePipeAtOneDistNum},
      {"allAtOneDistNum", p.allAtOneDistNum},
      {"AfterWriteTokenDepCount", p.AfterWriteTokenDepCount},
  };
  if (p.RAIterNum) {
    jsonObject.insert({"RAIterNum", p.RAIterNum});
    jsonObject.insert({"varNum", p.varNum});
    jsonObject.insert({"globalVarNum", p.globalVarNum});
    jsonObject.insert({"maxRP", p.maxRP});
    jsonObject.insert({"maxNeighbors", p.maxNeighbors});
    jsonObject.insert({"avgNeighbors", p.avgNeighbors});
    jsonObject.insert({"normIntfNum", p.normIntfNum});
    jsonObject.insert({"augIntfNum", p.augIntfNum});
  }

  return jsonObject;
}

} // namespace vISA

