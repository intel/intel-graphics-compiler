/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MaxGRFTables.h"
#include "Compiler/CISACodeGen/Platform.hpp"

using namespace std;
namespace IGC {
// clang-format off
static constexpr MaxGRFEntry XE3_GRF[] = {
  {128, SIMDMode::SIMD16, HWLocalId::EITHER, 1024},
  {128, SIMDMode::SIMD32, HWLocalId::VALUE1, 1024},
  {128, SIMDMode::SIMD32, HWLocalId::VALUE0, 2048},
  {160, SIMDMode::SIMD16, HWLocalId::EITHER, 768},
  {160, SIMDMode::SIMD32, HWLocalId::VALUE1, 1024},
  {160, SIMDMode::SIMD32, HWLocalId::VALUE0, 1536},
  {192, SIMDMode::SIMD16, HWLocalId::EITHER, 640},
  {192, SIMDMode::SIMD32, HWLocalId::VALUE1, 1024},
  {192, SIMDMode::SIMD32, HWLocalId::VALUE0, 1280},
  {256, SIMDMode::SIMD16, HWLocalId::EITHER, 512},
  {256, SIMDMode::SIMD32, HWLocalId::EITHER, 1024}};

// clang-format on

bool MaxGRFTable::MatchHWLocalID(HWLocalId fromTable, HWLocalId target) {
  // If target = '0', table entries with '0 or 1' and '0' are matched
  // If target = '1', table entries with '0 or 1' and '1' are matched
  if (fromTable == HWLocalId::EITHER || fromTable == target)
    return true;
  else
    return false;
}

uint16_t MaxGRFTable::GetMaxGRF(SIMDMode simt, HWLocalId hwlid, uint witems) {
  // Reverse order intentionally
  // GRF column is accessed in descending order
  for (auto &entry : llvm::reverse(table)) {
    if (simt != entry.SIMT)
      continue;

    if (MatchHWLocalID(entry.HWLID, hwlid) && witems <= entry.Workitems) {
      return entry.GRF;
    }
  }

  return 0;
}

void MaxGRFTable::LoadTable(const CPlatform &platform) {
  {
    table = XE3_GRF;
  }
}

} // namespace IGC
