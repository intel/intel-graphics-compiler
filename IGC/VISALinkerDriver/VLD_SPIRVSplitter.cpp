/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "VLD_SPIRVSplitter.hpp"

#include <llvm/ADT/ScopeExit.h>

#include "Probe/Assertion.h"
#include "spirv/unified1/spirv.hpp"
#include "spirv-tools/libspirv.h"

// helper function from SPIR-V Tools.
std::string spvDecodeLiteralStringOperand(const spv_parsed_instruction_t& inst,
  const uint16_t operand_index);

namespace IGC {
namespace VLD {

llvm::Expected<SPVMetadata> GetVLDMetadata(const char *spv_buffer,
                                   uint32_t spv_buffer_size_in_bytes) {
    return SpvSplitter().Parse(spv_buffer, spv_buffer_size_in_bytes);
}

llvm::Expected<std::pair<ProgramStreamType, ProgramStreamType>>
SplitSPMDAndESIMD(const char *spv_buffer, uint32_t spv_buffer_size_in_bytes) {

  SpvSplitter splitter;
  return splitter.Split(spv_buffer, spv_buffer_size_in_bytes);
}

llvm::Expected<spv_result_t> SpvSplitter::ParseSPIRV(const char* spv_buffer, uint32_t spv_buffer_size_in_bytes) {
  const spv_target_env target_env = SPV_ENV_UNIVERSAL_1_5;
  spv_context context = spvContextCreate(target_env);
  if (!context) {
      return llvm::createStringError(llvm::inconvertibleErrorCode(),
          "Couldn't create SPIR-V Tools context!");
  }
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

  return result;
}

SPIRVTypeEnum SpvSplitter::GetCurrentSPIRVType() const {
  if (has_spmd_functions_ && has_esimd_functions_) {
    return SPIRVTypeEnum::SPIRV_SPMD_AND_ESIMD;
  } else if (has_spmd_functions_) {
    return SPIRVTypeEnum::SPIRV_SPMD;
  }

  return SPIRVTypeEnum::SPIRV_ESIMD;
}

llvm::Expected<std::pair<ProgramStreamType, ProgramStreamType>>
SpvSplitter::Split(const char *spv_buffer, uint32_t spv_buffer_size_in_bytes) {
  this->Reset();
  this->only_detect_ = false;
  auto result = ParseSPIRV(spv_buffer, spv_buffer_size_in_bytes);

  if (!result) {
    return result.takeError();
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

  switch (GetCurrentSPIRVType()) {
  case SPIRVTypeEnum::SPIRV_ESIMD:
    spmd_program_.clear();
    break;
  case SPIRVTypeEnum::SPIRV_SPMD:
    esimd_program_.clear();
    break;
  case SPIRVTypeEnum::SPIRV_SPMD_AND_ESIMD:
  default:
    break;
  }

  return std::make_pair(spmd_program_, esimd_program_);
}

llvm::Expected<SPVMetadata>
SpvSplitter::Parse(const char* spv_buffer, uint32_t spv_buffer_size_in_bytes) {
  this->Reset();
  this->only_detect_ = true;
  auto result = ParseSPIRV(spv_buffer, spv_buffer_size_in_bytes);

  if (!result) {
    return result.takeError();
  }

  return GetVLDMetadata();
}

const std::string &SpvSplitter::GetErrorMessage() const {
  return error_message_;
}

bool SpvSplitter::HasError() const { return !error_message_.empty(); }

bool SpvSplitter::HasEntryPoints() const {
  auto CurSPIRVType = GetCurrentSPIRVType();
  if (entry_points_.size() == 0) return false;

  bool AllEntryPointsAreSPMD =
      std::all_of(entry_points_.begin(), entry_points_.end(), [&](auto el) {
        return esimd_decorated_ids_.find(el) == esimd_decorated_ids_.end();
      });

  bool AllEntryPointsAreESIMD =
    std::all_of(entry_points_.begin(), entry_points_.end(), [&](auto el) {
    return esimd_decorated_ids_.find(el) != esimd_decorated_ids_.end();
      });

  if (CurSPIRVType == SPIRVTypeEnum::SPIRV_ESIMD && AllEntryPointsAreSPMD) {
    return false;
  }

  if (CurSPIRVType == SPIRVTypeEnum::SPIRV_SPMD && AllEntryPointsAreESIMD) {
    return false;
  }

  // We currently do not support entry points in both parts.
  IGC_ASSERT(AllEntryPointsAreESIMD || AllEntryPointsAreSPMD);

  return true;

}

SPVMetadata SpvSplitter::GetVLDMetadata() const {
  SPVMetadata Metadata;
  Metadata.SpirvType = GetCurrentSPIRVType();
  Metadata.HasEntryPoints = HasEntryPoints();
  Metadata.ForcedSubgroupSize = GetForcedSubgroupSize();
  Metadata.ExportedFunctions = GetExportedFunctions();
  Metadata.ImportedFunctions = GetImportedFunctions();
  return Metadata;
}

const uint32_t SpvSplitter::GetForcedSubgroupSize() const {
  if (entry_point_to_subgroup_size_map_.size() == 0) return 0;
  IGC_ASSERT(std::all_of(entry_point_to_subgroup_size_map_.begin(), entry_point_to_subgroup_size_map_.end(), [&](auto& el) {
    return el.second == entry_point_to_subgroup_size_map_.begin()->second;
    }));

  return entry_point_to_subgroup_size_map_.begin()->second;
}

const std::vector<std::string>& SpvSplitter::GetExportedFunctions() const {
  return exported_functions_;
}

const std::vector<std::string>& SpvSplitter::GetImportedFunctions() const {
  return imported_functions_;
}

void SpvSplitter::Reset() {
  spmd_program_.clear();
  esimd_program_.clear();
  esimd_decorated_ids_.clear();
  entry_points_.clear();
  esimd_function_declarations_.clear();
  esimd_functions_to_declare_.clear();
  entry_point_to_subgroup_size_map_.clear();
  exported_functions_.clear();
  imported_functions_.clear();

  is_inside_spmd_function_ = false;
  is_inside_esimd_function_ = false;
  has_spmd_functions_ = false;
  has_esimd_functions_ = false;
  cur_esimd_function_id_ = -1;
}

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
  if(!only_detect_) {
    append_header(spmd_program_);
    append_header(esimd_program_);
  }
  return SPV_SUCCESS;
}

spv_result_t SpvSplitter::HandleInstruction(
    const spv_parsed_instruction_t *parsed_instruction) {
  spv_result_t ret = SPV_SUCCESS;

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
  case spv::OpExecutionMode:
    ret = HandleExecutionMode(parsed_instruction);
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

  auto getOperand = [&parsed_instruction](int operandNumber) {
    return parsed_instruction->words[parsed_instruction->operands[operandNumber].offset];
  };

  auto isSpecificFunctionDecoration = [&parsed_instruction, &getOperand](
                                          auto decoration_type) {
    if (parsed_instruction->num_operands == 2 &&
        parsed_instruction->operands[1].type == SPV_OPERAND_TYPE_DECORATION &&
        getOperand(1) == decoration_type) {
      uint32_t function_id = getOperand(0);
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
  } else if (getOperand(1) == spv::DecorationLinkageAttributes) {
    if (parsed_instruction->num_operands == 4 &&
        parsed_instruction->operands[2].type ==
            SPV_OPERAND_TYPE_LITERAL_STRING) {
      auto funcName = spvDecodeLiteralStringOperand(*parsed_instruction, 2);
      if (getOperand(3) == spv::LinkageTypeExport) {
        exported_functions_.push_back(funcName);
      } else if (getOperand(3) == spv::LinkageTypeImport) {
        imported_functions_.push_back(funcName);
      }
    }

    AddInstToProgram(parsed_instruction, spmd_program_);
    AddInstToProgram(parsed_instruction, esimd_program_);
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

spv_result_t SpvSplitter::HandleExecutionMode(
  const spv_parsed_instruction_t *parsed_instruction) {
  IGC_ASSERT(parsed_instruction &&
    parsed_instruction->opcode == spv::OpExecutionMode);
  IGC_ASSERT(parsed_instruction->num_operands > 0);

  uint32_t id =
    parsed_instruction->words[parsed_instruction->operands[0].offset];

  uint32_t execMode =
    parsed_instruction->words[parsed_instruction->operands[1].offset];
  if (execMode == spv::ExecutionModeSubgroupSize)
  {
    uint32_t sgSize =
      parsed_instruction->words[parsed_instruction->operands[2].offset];
    entry_point_to_subgroup_size_map_.insert({ id,sgSize });
  }

  AddInstToProgram(parsed_instruction, spmd_program_);
  AddInstToProgram(parsed_instruction, esimd_program_);

  return SPV_SUCCESS;
}

void SpvSplitter::AddInstToProgram(
    const spv_parsed_instruction_t *parsed_instruction,
    ProgramStreamType &program) {
  if (!only_detect_) {
    program.insert(program.end(), parsed_instruction->words,
                   parsed_instruction->words + parsed_instruction->num_words);
  }
}

} // namespace VLD
} // namespace IGC
