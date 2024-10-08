/*========================== begin_copyright_notice ============================

Copyright (C) 2023-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <llvm/Support/Error.h>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "VLD.hpp"

#pragma once

namespace IGC {
namespace VLD {

using ProgramStreamType = std::vector<uint32_t>;

// Splits SPIR-V module that contains ESIMD and SPMD parts into separate
// SPIR-V modules.
// Returns a pair of binaries: first is SPMD module, second is ESIMD module.
llvm::Expected<std::pair<ProgramStreamType, ProgramStreamType>>
SplitSPMDAndESIMD(const char *spvBuffer, uint32_t spvBufferSizeInBytes);

// Detects the type of the SPIR-V module, e.g. SPMD, ESIMD, SPMD+ESIMD
llvm::Expected<SPVMetadata> GetVLDMetadata(const char *spv_buffer,
                                           uint32_t spv_buffer_size_in_bytes);

// Class used to split SPMD and ESIMD parts of input SPIR-V module.
class SpvSplitter {
public:
  // Splits SPIR-V module that contains ESIMD and SPMD parts into separate
  // SPIR-V modules.
  // Returns a pair of binaries: first is SPMD module, second is ESIMD module.
  llvm::Expected<std::pair<ProgramStreamType, ProgramStreamType>>
  Split(const char *spv_buffer, uint32_t spv_buffer_size_in_bytes);

  // Parses the SPIR-V module and returns metadata necessary for visa linking.
  llvm::Expected<SPVMetadata> Parse(const char *spv_buffer,
                                    uint32_t spv_buffer_size_in_bytes);

  const uint32_t GetForcedSubgroupSize() const;

  bool HasEntryPoints() const;

  const std::vector<std::string> &GetExportedFunctions() const;

  const std::vector<std::string> &GetImportedFunctions() const;

  SPVMetadata GetVLDMetadata() const;

  void Reset();

  const ProgramStreamType &spmd_program() const { return spmd_program_; }
  const ProgramStreamType &esimd_program() const { return esimd_program_; }

private:
  llvm::Error HandleHeader(llvm::ArrayRef<uint32_t> words);

  llvm::Error HandleInstruction(llvm::ArrayRef<uint32_t> words);

  llvm::Error HandleDecorate(llvm::ArrayRef<uint32_t> words);

  llvm::Error HandleGroupDecorate(llvm::ArrayRef<uint32_t> words);

  llvm::Error HandleFunctionStart(llvm::ArrayRef<uint32_t> words);

  llvm::Error HandleFunctionParameter(llvm::ArrayRef<uint32_t> words);

  llvm::Error HandleFunctionEnd(llvm::ArrayRef<uint32_t> words);

  llvm::Error HandleEntryPoint(llvm::ArrayRef<uint32_t> words);

  llvm::Error HandleExecutionMode(llvm::ArrayRef<uint32_t> words);

  void AddInstToProgram(llvm::ArrayRef<uint32_t> words,
                        ProgramStreamType &program);

  std::string DecodeStringLiteral(llvm::ArrayRef<uint32_t> words,
                                  size_t startIndex);

  llvm::Error ParseSPIRV(const char *spv_buffer,
                         uint32_t spv_buffer_size_in_bytes);

  // Returns current SPIR-V Module type. Must be called after ParseSPIRV
  SPIRVTypeEnum GetCurrentSPIRVType() const;

  // When this flag is set to true, instructions will not be added to
  // spmd_program_ and esimd_program_ buffers.
  bool only_detect_ = false;

  ProgramStreamType spmd_program_;
  ProgramStreamType esimd_program_;
  std::unordered_set<uint32_t> esimd_decorated_ids_;
  std::unordered_set<uint32_t> entry_points_;
  std::unordered_map<uint32_t, ProgramStreamType> esimd_function_declarations_;
  std::unordered_set<uint32_t> esimd_functions_to_declare_;
  std::unordered_map<uint32_t, uint32_t> entry_point_to_subgroup_size_map_;
  std::vector<std::string> exported_functions_;
  std::vector<std::string> imported_functions_;

  bool is_inside_spmd_function_ = false;
  bool is_inside_esimd_function_ = false;
  bool has_spmd_functions_ = false;
  bool has_esimd_functions_ = false;
  int cur_esimd_function_id_ = -1;
};

} // namespace VLD
} // namespace IGC
