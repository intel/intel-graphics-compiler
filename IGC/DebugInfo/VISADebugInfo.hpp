/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/DenseMap.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

#include "VISADebugDecoder.hpp"

#include <unordered_map>
#include <vector>
#include <map>

namespace IGC {

class VISAModule;

struct IDX_Gen2Visa {
  unsigned GenOffset;
  unsigned VisaOffset;
};

// provides access to debug information for the compiled VISA object
// that supports user-friendly queries and provides several LUT
// for GEN<->VISA mappings.
class VISAObjectDebugInfo {

public:
  using CompiledObjectInfo = IGC::DbgDecoder::DbgInfoFormat;
  using CallFrameInfo = IGC::DbgDecoder::CallFrameInfo;
  using SubroutinesList = decltype(CompiledObjectInfo::subs);
  using RelocOffsetTy = decltype(CompiledObjectInfo::relocOffset);
  using CisaIndexLUT = decltype(CompiledObjectInfo::CISAIndexMap);
  using VISAVariablesList = decltype(CompiledObjectInfo::Vars);

  using GenIsaToSizeMapping = llvm::DenseMap<unsigned, unsigned>;
  using VisaToGenMapping = std::map<unsigned, std::vector<unsigned>>;
  using GenToVisaIndexes = std::vector<IDX_Gen2Visa>;

private:
  const CompiledObjectInfo &CO;

  GenIsaToSizeMapping GenISAInstSizeBytes;
  VisaToGenMapping VISAIndexToAllGenISAOff;
  GenToVisaIndexes GenISAToVISAIndex;

public:
  VISAObjectDebugInfo(const CompiledObjectInfo &COIn);

  const CallFrameInfo &getCFI() const { return CO.cfi; }

  RelocOffsetTy getRelocOffset() const { return CO.relocOffset; }

  const SubroutinesList &getSubroutines() const { return CO.subs; }
  const VISAVariablesList &getVISAVariables() const { return CO.Vars; }

  const CisaIndexLUT &getCISAIndexLUT() const { return CO.CISAIndexMap; }

  const VisaToGenMapping &getVisaToGenLUT() const { return VISAIndexToAllGenISAOff; }

  const GenIsaToSizeMapping &getGenToSizeInBytesLUT() const { return GenISAInstSizeBytes; }

  const GenToVisaIndexes &getGenToVisaIndexLUT() const { return GenISAToVISAIndex; }

  void dump() const;
  void print(llvm::raw_ostream &OS) const;
};

// This class is essentially a wrapper over IGC::DbgDecoder object
class VISADebugInfo {
  const IGC::DbgDecoder DecodedDebugStorage;

  using CompiledObjectDI = IGC::DbgDecoder::DbgInfoFormat;
  using DebugInfoHolders = std::unordered_map<const CompiledObjectDI *, VISAObjectDebugInfo>;
  DebugInfoHolders DebugInfoMap;

public:
  // VISADebugInfo(const IGC::DbgDecoder &DecodedDebugStorageIn);
  VISADebugInfo(const void *RawDbgDataPtr);

  // get's the underlying IGC::DbgDecoder object
  // TODO: remove, for now we need it for backwards compatibility.
  const IGC::DbgDecoder &getRawDecodedData() const { return DecodedDebugStorage; }

  // gets Visa Object Debug Info that corresponds to a particular named
  // entity - that is a kernel/stack-called function or a subroutine
  const VISAObjectDebugInfo &getVisaObjectDI(const VISAModule &VM) const;
  const VISAObjectDebugInfo &getVisaObjectByCompliledObjectName(llvm::StringRef CompiledObjectName) const;

  void dump() const;
  void print(llvm::raw_ostream &OS) const;
};

} // namespace IGC
