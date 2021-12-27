/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "VLD_SPIRVSplitter.hpp"

#include <llvm/ADT/ScopeExit.h>

#include "Probe/Assertion.h"
#include "spirv/unified1/spirv.hpp"

namespace IGC {
namespace VLD {

llvm::Expected<std::pair<ProgramStreamType, ProgramStreamType>>
SplitSPMDAndESIMD(const char *spv_buffer, uint32_t spv_buffer_size_in_bytes) {

  SpvSplitter splitter;
  return splitter.Split(spv_buffer, spv_buffer_size_in_bytes);
}

llvm::Expected<std::pair<ProgramStreamType, ProgramStreamType>>
SpvSplitter::Split(const char *spv_buffer, uint32_t spv_buffer_size_in_bytes) {
  const spv_target_env target_env = SPV_ENV_UNIVERSAL_1_5;
  spv_context context = spvContextCreate(target_env);
  const uint32_t *const binary = reinterpret_cast<const uint32_t *>(spv_buffer);
  const size_t word_count = (spv_buffer_size_in_bytes / sizeof(uint32_t));
  spv_diagnostic diagnostic = nullptr;
  auto scope_exit = llvm::make_scope_exit([&] {
    spvDiagnosticDestroy(diagnostic);
    spvContextDestroy(context);
  });

  const spv_result_t result = spvBinaryParse(
      context, this, binary, word_count, SpvSplitter::HandleHeaderCallback,
      SpvSplitter::HandleInstructionCallback, &diagnostic);

  if (result != SPV_SUCCESS) {
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   diagnostic->error);
  }

  if (!has_spmd_functions_ && !has_esimd_functions_) {
    return llvm::createStringError(
        llvm::inconvertibleErrorCode(),
        "SPIR-V file did not contain any SPMD or ESIMD functions!");
  }

  // Add declarations of ESIMD functions that are called from SPMD module,
  // otherwise SPIR-V reader might fail.
  for (auto esimd_func_id : esimd_functions_to_declare_) {
    if (esimd_function_declarations_.find(esimd_func_id) ==
        esimd_function_declarations_.end()) {
      return llvm::createStringError(
          llvm::inconvertibleErrorCode(),
          "SPIR-V Splitter error: ESIMD function declaration not found!");
    }

    spmd_program_.insert(spmd_program_.end(),
                         esimd_function_declarations_[esimd_func_id].begin(),
                         esimd_function_declarations_[esimd_func_id].end());
  }

  // If all entry points are marked as ESIMD, treat this as fully ESIMD module,
  // even if there are functions that are not marked with ESIMD (known bug in
  // VC).
  if (!entry_points_.empty() && std::all_of(entry_points_.begin(), entry_points_.end(), [&](auto el) {
        return esimd_decorated_ids_.find(el) != esimd_decorated_ids_.end();
      })) {
    spmd_program_.clear();
    IGC_ASSERT(has_esimd_functions_);
  } else if (!has_esimd_functions_) {
    // This is SPMD module as entry points are included in the flag.
    esimd_program_.clear();
  }

  return std::make_pair(spmd_program_, esimd_program_);
}

const std::string &SpvSplitter::GetErrorMessage() const {
  return error_message_;
}

bool SpvSplitter::HasError() const { return !error_message_.empty(); }

spv_result_t SpvSplitter::HandleInstructionCallback(
    void *user_data, const spv_parsed_instruction_t *parsed_instruction) {
  IGC_ASSERT(user_data);
  auto splitter = static_cast<SpvSplitter *>(user_data);
  return splitter->HandleInstruction(parsed_instruction);
}

spv_result_t SpvSplitter::HandleHeaderCallback(
    void *user_data, spv_endianness_t endian, uint32_t magic, uint32_t version,
    uint32_t generator, uint32_t id_bound, uint32_t schema) {
  IGC_ASSERT(user_data);
  auto splitter = static_cast<SpvSplitter *>(user_data);
  return splitter->HandleHeader(endian, magic, version, generator, id_bound,
                                schema);
}

spv_result_t SpvSplitter::HandleHeader(spv_endianness_t endian, uint32_t magic,
                                       uint32_t version, uint32_t generator,
                                       uint32_t id_bound, uint32_t schema) {
  // insert the same header to both spmd and esimd programs.
  auto append_header = [&](std::vector<uint32_t> &programVector) {
    programVector.insert(programVector.end(),
                         {magic, version, generator, id_bound, schema});
  };
  append_header(spmd_program_);
  append_header(esimd_program_);
  return SPV_SUCCESS;
}

spv_result_t SpvSplitter::HandleInstruction(
    const spv_parsed_instruction_t *parsed_instruction) {
  spv_result_t ret = SPV_SUCCESS;

  auto add_to_program = [parsed_instruction](decltype(spmd_program_) &p) {
    p.insert(p.end(), parsed_instruction->words,
             parsed_instruction->words + parsed_instruction->num_words);
  };

  // Handlers decide if given instruction should be addded.
  switch (parsed_instruction->opcode) {
  case spv::OpDecorate:
    ret = HandleDecorate(parsed_instruction);
    break;
  case spv::OpGroupDecorate:
    ret = HandleGroupDecorate(parsed_instruction);
    break;
  case spv::OpFunction:
    ret = HandleFunctionStart(parsed_instruction);
    break;
  case spv::OpFunctionParameter:
    ret = HandleFunctionParameter(parsed_instruction);
    break;
  case spv::OpFunctionEnd:
    ret = HandleFunctionEnd(parsed_instruction);
    break;
  case spv::OpEntryPoint:
    ret = HandleEntryPoint(parsed_instruction);
    break;
  default:
    if (!is_inside_spmd_function_) {
      AddInstToProgram(parsed_instruction, esimd_program_);
    }
    if (!is_inside_esimd_function_) {
      AddInstToProgram(parsed_instruction, spmd_program_);
    }
    break;
  }
  return ret;
}

// Looks for decorations that mark functions specific to ESIMD module.
spv_result_t SpvSplitter::HandleDecorate(
    const spv_parsed_instruction_t *parsed_instruction) {
  IGC_ASSERT(parsed_instruction &&
             parsed_instruction->opcode == spv::OpDecorate);

  auto isSpecificFunctionDecoration = [&parsed_instruction](auto decoration_type) {
      if (parsed_instruction->num_operands == 2 &&
          parsed_instruction->operands[1].type == SPV_OPERAND_TYPE_DECORATION &&
          parsed_instruction->words[parsed_instruction->operands[1].offset] ==
          decoration_type) {
          uint32_t function_id =
              parsed_instruction->words[parsed_instruction->operands[0].offset];
          return function_id;
      }
      return (uint32_t)0;
  };

  // Look for VectorComputeFunctionINTEL decoration.
  if (auto function_id = isSpecificFunctionDecoration(spv::DecorationVectorComputeFunctionINTEL)) {
    esimd_decorated_ids_.insert(function_id);
  } else if (auto function_id = isSpecificFunctionDecoration(spv::DecorationStackCallINTEL)) {
    // StackCallINTEL is a decoration specific to ESIMD, so do not add it to SPMD program.
    esimd_functions_to_declare_.insert(function_id);
  } else {
    AddInstToProgram(parsed_instruction, spmd_program_);
  }
  AddInstToProgram(parsed_instruction, esimd_program_);

  return SPV_SUCCESS;
}

// Looks for group decorations that mark functions specific to ESIMD module.
spv_result_t SpvSplitter::HandleGroupDecorate(
    const spv_parsed_instruction_t *parsed_instruction) {
  IGC_ASSERT(parsed_instruction &&
             parsed_instruction->opcode == spv::OpGroupDecorate);
  IGC_ASSERT(parsed_instruction->num_operands > 0);
  // Look for decoration groups previously marked with
  // VectorComputeFunctionINTEL decoration.
  uint32_t group_id =
      parsed_instruction->words[parsed_instruction->operands[0].offset];
  if (esimd_decorated_ids_.find(group_id) != esimd_decorated_ids_.end()) {
    for (uint32_t i = 1; i < parsed_instruction->num_operands; ++i) {
      uint32_t id =
          parsed_instruction->words[parsed_instruction->operands[i].offset];
      esimd_decorated_ids_.insert(id);
    }
  } else {
    AddInstToProgram(parsed_instruction, spmd_program_);
  }
  AddInstToProgram(parsed_instruction, esimd_program_);

  return SPV_SUCCESS;
}

spv_result_t SpvSplitter::HandleFunctionStart(
    const spv_parsed_instruction_t *parsed_instruction) {
  IGC_ASSERT(parsed_instruction &&
             parsed_instruction->opcode == spv::OpFunction);
  if (esimd_decorated_ids_.find(parsed_instruction->result_id) !=
      esimd_decorated_ids_.end()) {
    is_inside_esimd_function_ = true;
    has_esimd_functions_ = true;
    cur_esimd_function_id_ = parsed_instruction->result_id;

    AddInstToProgram(parsed_instruction, esimd_program_);
    AddInstToProgram(parsed_instruction,
                     esimd_function_declarations_[cur_esimd_function_id_]);
  } else {
    is_inside_spmd_function_ = true;
    has_spmd_functions_ = true;
    AddInstToProgram(parsed_instruction, spmd_program_);
  }

  return SPV_SUCCESS;
}

spv_result_t SpvSplitter::HandleFunctionParameter(
    const spv_parsed_instruction_t *parsed_instruction) {
  IGC_ASSERT(parsed_instruction &&
             parsed_instruction->opcode == spv::OpFunctionParameter);
  if (is_inside_esimd_function_) {
    AddInstToProgram(parsed_instruction, esimd_program_);
    AddInstToProgram(parsed_instruction,
                     esimd_function_declarations_[cur_esimd_function_id_]);
  } else {
    AddInstToProgram(parsed_instruction, spmd_program_);
  }
  return SPV_SUCCESS;
}

spv_result_t SpvSplitter::HandleFunctionEnd(
    const spv_parsed_instruction_t *parsed_instruction) {
  IGC_ASSERT(parsed_instruction &&
             parsed_instruction->opcode == spv::OpFunctionEnd);

  if (is_inside_esimd_function_) {
    AddInstToProgram(parsed_instruction, esimd_program_);
    AddInstToProgram(parsed_instruction,
                     esimd_function_declarations_[cur_esimd_function_id_]);
    cur_esimd_function_id_ = -1;
  }
  if (is_inside_spmd_function_) {
    AddInstToProgram(parsed_instruction, spmd_program_);
  }

  is_inside_esimd_function_ = false;
  is_inside_spmd_function_ = false;
  return SPV_SUCCESS;
}

spv_result_t SpvSplitter::HandleEntryPoint(
    const spv_parsed_instruction_t *parsed_instruction) {
  IGC_ASSERT(parsed_instruction &&
             parsed_instruction->opcode == spv::OpEntryPoint);
  IGC_ASSERT(parsed_instruction->num_operands > 0);

  uint32_t id =
      parsed_instruction->words[parsed_instruction->operands[1].offset];
  entry_points_.insert(id);
  AddInstToProgram(parsed_instruction, spmd_program_);
  AddInstToProgram(parsed_instruction, esimd_program_);

  return SPV_SUCCESS;
}

void SpvSplitter::AddInstToProgram(
    const spv_parsed_instruction_t *parsed_instruction,
    ProgramStreamType &program) {
  program.insert(program.end(), parsed_instruction->words,
                 parsed_instruction->words + parsed_instruction->num_words);
}

} // namespace VLD
} // namespace IGC
