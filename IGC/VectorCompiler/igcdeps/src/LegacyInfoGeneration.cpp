/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LegacyInfoGeneration.h"
#include "vc/GenXCodeGen/GenXOCLRuntimeInfo.h"

#include "Probe/Assertion.h"
#include "RelocationInfo.h"

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <string>
#include <tuple>

using namespace llvm;

void vc::validateFunctionSymbolTable(
    const GenXOCLRuntimeInfo::SymbolSeq &Funcs) {
  auto IsKernel = [](const vISA::ZESymEntry &Entry) {
    return Entry.s_type == vISA::GenSymType::S_KERNEL;
  };
  IGC_ASSERT_MESSAGE(std::count_if(Funcs.begin(), Funcs.end(), IsKernel) < 2u,
                     "There can be only one or no kernel symbols");
  IGC_ASSERT_MESSAGE(std::is_partitioned(Funcs.begin(), Funcs.end(), IsKernel),
                     "Kernel symbols should be partitioned");
}

std::tuple<void *, unsigned, unsigned>
vc::emitLegacyFunctionSymbolTable(const GenXOCLRuntimeInfo::SymbolSeq &Funcs) {
  vc::validateFunctionSymbolTable(Funcs);
  // Kernel symbol is ZE binary only, thus it can be skipped here.
  auto Start = std::partition_point(
      Funcs.begin(), Funcs.end(), [](const vISA::ZESymEntry &Entry) {
        return Entry.s_type == vISA::GenSymType::S_KERNEL;
      });
  auto Entries = Funcs.end() - Start;
  if (Entries == 0) {
    return {nullptr, 0, 0};
  }
  auto Size = Entries * sizeof(vISA::GenSymEntry);
  auto *Buffer = reinterpret_cast<vISA::GenSymEntry *>(
      std::malloc(Entries * sizeof(vISA::GenSymEntry)));
  return {Buffer, Size, Entries};
}
