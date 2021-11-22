/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "spirv-tools/libspirv.h"
#include <llvm/Support/Error.h>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#pragma once

namespace IGC {
namespace VLD {

using ProgramStreamType = std::vector<uint32_t>;

// Splits SPIR-V module that contains ESIMD and SPMD parts into separate
// SPIR-V modules.
// Returns a pair of binaries: first is SPMD module, second is ESIMD module.
llvm::Expected<std::pair<ProgramStreamType, ProgramStreamType>>
SplitSPMDAndESIMD(const char *spvBuffer, uint32_t spvBufferSizeInBytes);

// Class used to split SPMD and ESIMD parts of input SPIR-V module.
class SpvSplitter {
public:
  // Splits SPIR-V module that contains ESIMD and SPMD parts into separate
  // SPIR-V modules.
  // Returns a pair of binaries: first is SPMD module, second is ESIMD module.
  llvm::Expected<std::pair<ProgramStreamType, ProgramStreamType>>
  Split(const char *spv_buffer, uint32_t spv_buffer_size_in_bytes);

  const std::string &GetErrorMessage() const;

  bool HasError() const;

  // Callbacks used by SPIR-V Tools parser.
  static spv_result_t
  HandleInstructionCallback(void *user_data,
                            const spv_parsed_instruction_t *parsed_instruction);

  static spv_result_t HandleHeaderCallback(void *user_data,
                                           spv_endianness_t endian,
                                           uint32_t magic, uint32_t version,
                                           uint32_t generator,
                                           uint32_t id_bound, uint32_t schema);

  const ProgramStreamType &spmd_program() const { return spmd_program_; }
  const ProgramStreamType &esimd_program() const { return esimd_program_; }

private:
  spv_result_t HandleHeader(spv_endianness_t endian, uint32_t magic,
                            uint32_t version, uint32_t generator,
                            uint32_t id_bound, uint32_t schema);

  spv_result_t
  HandleInstruction(const spv_parsed_instruction_t *parsed_instruction);

  spv_result_t
  HandleDecorate(const spv_parsed_instruction_t *parsed_instruction);

  spv_result_t
  HandleGroupDecorate(const spv_parsed_instruction_t *parsed_instruction);

  spv_result_t
  HandleFunctionStart(const spv_parsed_instruction_t *parsed_instruction);

  spv_result_t
  HandleFunctionParameter(const spv_parsed_instruction_t *parsed_instruction);

  spv_result_t
  HandleFunctionEnd(const spv_parsed_instruction_t *parsed_instruction);

  spv_result_t
  HandleEntryPoint(const spv_parsed_instruction_t *parsed_instruction);

  void AddInstToProgram(const spv_parsed_instruction_t *parsed_instruction,
                        ProgramStreamType &program);

  ProgramStreamType spmd_program_;
  ProgramStreamType esimd_program_;
  std::unordered_set<uint32_t> esimd_decorated_ids_;
  std::unordered_set<uint32_t> entry_points_;
  std::unordered_map<uint32_t, ProgramStreamType> esimd_function_declarations_;
  std::unordered_set<uint32_t> esimd_functions_to_declare_;

  bool is_inside_spmd_function_ = false;
  bool is_inside_esimd_function_ = false;
  bool has_spmd_functions_ = false;
  bool has_esimd_functions_ = false;
  int cur_esimd_function_id_ = -1;

  std::string error_message_;
};

} // namespace VLD
} // namespace IGC
