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

// appendLegacySymbolTable: a helper function to append symbols to a legacy
// symbol table.
// Output iterator is represented by a pointer \p OutIt, so the table/array
// it points to has to have enough space.
// The range [\p First, \p Last) must consist of vISA::ZESymEntry elements.
template <typename InputIter>
void appendLegacySymbolTable(InputIter First, InputIter Last,
                             vISA::GenSymEntry *OutIt) {
  std::transform(
      First, Last, OutIt, [](const vISA::ZESymEntry &SI) -> vISA::GenSymEntry {
        vISA::GenSymEntry Entry;
        Entry.s_offset = SI.s_offset;
        Entry.s_size = SI.s_size;
        Entry.s_type = SI.s_type;
        IGC_ASSERT_MESSAGE(SI.s_name.size() < vISA::MAX_SYMBOL_NAME_LENGTH,
                           "no solution for long symbol names for legacy "
                           "symbol info is yet provided");
        IGC_ASSERT_MESSAGE(sizeof(Entry.s_name) ==
                               vISA::MAX_SYMBOL_NAME_LENGTH *
                                   sizeof(Entry.s_name[0]),
                           "vISA::GenSymEntry structure is inconsistent");
        std::copy(SI.s_name.begin(), SI.s_name.end(), Entry.s_name);
        // Null-terminating the string.
        Entry.s_name[SI.s_name.size()] = '\0';
        return Entry;
      });
}

std::tuple<void *, unsigned, unsigned>
vc::emitLegacyModuleSymbolTable(const GenXOCLRuntimeInfo::SymbolSeq &Constants,
                                const GenXOCLRuntimeInfo::SymbolSeq &Globals) {
  unsigned Entries = Constants.size() + Globals.size();
  if (Entries == 0)
    return {nullptr, 0, 0};

  unsigned Size = Entries * sizeof(vISA::GenSymEntry);
  // this will be eventually freed in AdaptorOCL
  auto *Buffer = static_cast<vISA::GenSymEntry *>(
      std::malloc(Entries * sizeof(vISA::GenSymEntry)));
  auto *Inserter = Buffer;
  appendLegacySymbolTable(Constants.begin(), Constants.end(), Inserter);
  Inserter += Constants.size();
  appendLegacySymbolTable(Globals.begin(), Globals.end(), Inserter);
  return {Buffer, Size, Entries};
}

void vc::validateFunctionSymbolTable(
    const GenXOCLRuntimeInfo::SymbolSeq &Funcs) {
  IGC_ASSERT_MESSAGE(Funcs.size() > 0u, "Must have at least one function");
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
  // this will be eventually freed in AdaptorOCL
  auto *Buffer = reinterpret_cast<vISA::GenSymEntry *>(
      std::malloc(Entries * sizeof(vISA::GenSymEntry)));
  appendLegacySymbolTable(Start, Funcs.end(), Buffer);
  return {Buffer, Size, Entries};
}
