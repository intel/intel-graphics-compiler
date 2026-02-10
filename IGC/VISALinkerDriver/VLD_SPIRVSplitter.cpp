/*========================== begin_copyright_notice ============================

Copyright (C) 2023-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "VLD_SPIRVSplitter.hpp"

#include "Probe/Assertion.h"
#include "spirv/unified1/spirv.hpp"

namespace IGC {
namespace VLD {

llvm::Expected<SPVMetadata> GetVLDMetadata(const char *spv_buffer, uint32_t spv_buffer_size_in_bytes) {
  return SpvSplitter().Parse(spv_buffer, spv_buffer_size_in_bytes);
}

llvm::Expected<std::pair<ProgramStreamType, ProgramStreamType>> SplitSPMDAndESIMD(const char *spv_buffer,
                                                                                  uint32_t spv_buffer_size_in_bytes) {

  SpvSplitter splitter;
  return splitter.Split(spv_buffer, spv_buffer_size_in_bytes);
}

llvm::Error SpvSplitter::ParseSPIRV(const char *spv_buffer, uint32_t spv_buffer_size_in_bytes) {
  const uint32_t *const binary = reinterpret_cast<const uint32_t *>(spv_buffer);
  const size_t word_count = (spv_buffer_size_in_bytes / sizeof(uint32_t));

  if (word_count < 5) {
    return llvm::createStringError(llvm::inconvertibleErrorCode(), "SPIR-V binary is too short!");
  }

  // Skip the header (magic, version, generator, bound, schema)
  size_t offset = 5;

  llvm::Error err = HandleHeader(llvm::ArrayRef<uint32_t>(&binary[0], 5));
  if (err)
    return err;

  // Now read the instructions
  while (offset < word_count) {
    uint32_t word = binary[offset];
    uint16_t wordCount = word >> 16;

    if (wordCount == 0) {
      return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                     "Invalid SPIR-V instruction with word count 0 at offset " +
                                         std::to_string(offset));
    }

    if (offset + wordCount > word_count) {
      return llvm::createStringError(llvm::inconvertibleErrorCode(), "SPIR-V instruction at offset " +
                                                                         std::to_string(offset) +
                                                                         " extends beyond the end of the binary");
    }

    llvm::ArrayRef<uint32_t> instWords(&binary[offset], wordCount);

    // Handle the instruction
    err = HandleInstruction(instWords);
    if (err)
      return err;

    offset += wordCount;
  }

  if (!has_spmd_functions_ && !has_esimd_functions_) {
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "SPIR-V file did not contain any SPMD or ESIMD functions!");
  }

  return llvm::Error::success();
}

SPIRVTypeEnum SpvSplitter::GetCurrentSPIRVType() const {
  if (has_spmd_functions_ && has_esimd_functions_) {
    return SPIRVTypeEnum::SPIRV_SPMD_AND_ESIMD;
  } else if (has_spmd_functions_) {
    return SPIRVTypeEnum::SPIRV_SPMD;
  }

  return SPIRVTypeEnum::SPIRV_ESIMD;
}
llvm::Expected<std::pair<ProgramStreamType, ProgramStreamType>> SpvSplitter::Split(const char *spv_buffer,
                                                                                   uint32_t spv_buffer_size_in_bytes) {
  this->Reset();
  this->only_detect_ = false;
  auto parseError = ParseSPIRV(spv_buffer, spv_buffer_size_in_bytes);

  if (parseError) {
    return std::move(parseError);
  }

  // Add declarations of ESIMD functions that are called from SPMD module,
  // otherwise SPIR-V reader might fail.
  for (auto esimd_func_id : esimd_functions_to_declare_) {
    if (esimd_function_declarations_.find(esimd_func_id) == esimd_function_declarations_.end()) {
      return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                     "SPIR-V Splitter error: ESIMD function declaration not found!");
    }

    spmd_program_.insert(spmd_program_.end(), esimd_function_declarations_[esimd_func_id].begin(),
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

llvm::Expected<SPVMetadata> SpvSplitter::Parse(const char *spv_buffer, uint32_t spv_buffer_size_in_bytes) {
  this->Reset();
  this->only_detect_ = true;
  auto parseError = ParseSPIRV(spv_buffer, spv_buffer_size_in_bytes);

  if (parseError) {
    return std::move(parseError);
  }

  return GetVLDMetadata();
}

bool SpvSplitter::HasEntryPoints() const {
  auto CurSPIRVType = GetCurrentSPIRVType();
  if (entry_points_.size() == 0)
    return false;

  bool AllEntryPointsAreSPMD = std::all_of(entry_points_.begin(), entry_points_.end(), [&](auto el) {
    return esimd_decorated_ids_.find(el) == esimd_decorated_ids_.end();
  });

  bool AllEntryPointsAreESIMD = std::all_of(entry_points_.begin(), entry_points_.end(), [&](auto el) {
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
  if (entry_point_to_subgroup_size_map_.size() == 0)
    return 0;
  IGC_ASSERT(std::all_of(entry_point_to_subgroup_size_map_.begin(), entry_point_to_subgroup_size_map_.end(),
                         [&](auto &el) { return el.second == entry_point_to_subgroup_size_map_.begin()->second; }));

  return entry_point_to_subgroup_size_map_.begin()->second;
}

const std::vector<std::string> &SpvSplitter::GetExportedFunctions() const { return exported_functions_; }

const std::vector<std::string> &SpvSplitter::GetImportedFunctions() const { return imported_functions_; }

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

llvm::Error SpvSplitter::HandleHeader(llvm::ArrayRef<uint32_t> words) {
  // insert the same header to both spmd and esimd programs.
  if (!only_detect_) {
    spmd_program_.insert(spmd_program_.end(), words.begin(), words.end());
    esimd_program_.insert(esimd_program_.end(), words.begin(), words.end());
  }
  return llvm::Error::success();
}

llvm::Error SpvSplitter::HandleInstruction(llvm::ArrayRef<uint32_t> words) {
  uint16_t opcode = words[0] & 0xFFFF;

  switch (opcode) {
  case spv::OpDecorate:
    return HandleDecorate(words);
  case spv::OpGroupDecorate:
    return HandleGroupDecorate(words);
  case spv::OpFunction:
    return HandleFunctionStart(words);
  case spv::OpFunctionParameter:
    return HandleFunctionParameter(words);
  case spv::OpFunctionEnd:
    return HandleFunctionEnd(words);
  case spv::OpEntryPoint:
    return HandleEntryPoint(words);
  case spv::OpExecutionMode:
    return HandleExecutionMode(words);
  default:
    if (!is_inside_spmd_function_) {
      AddInstToProgram(words, esimd_program_);
    }
    if (!is_inside_esimd_function_) {
      AddInstToProgram(words, spmd_program_);
    }
    break;
  }

  return llvm::Error::success();
}

llvm::Error SpvSplitter::HandleDecorate(llvm::ArrayRef<uint32_t> words) {
  // OpDecorate instruction format:
  // Word 0: WordCount and Opcode
  // Word 1: Target ID
  // Word 2: Decoration (enum)
  // Word 3..N: Extra operands depending on decoration

  IGC_ASSERT(words.size() >= 3);

  uint32_t targetId = words[1];
  uint32_t decoration = words[2];

  // Check if it's DecorationVectorComputeFunctionINTEL
  if (decoration == spv::DecorationVectorComputeFunctionINTEL) {
    esimd_decorated_ids_.insert(targetId);
  } else if (decoration == spv::DecorationStackCallINTEL) {
    // StackCallINTEL is a decoration specific to ESIMD
    esimd_functions_to_declare_.insert(targetId);
  } else if (decoration == spv::DecorationLinkageAttributes) {
    // DecorationLinkageAttributes has the following operands:
    // Target (ID), Decoration, Name (string), LinkageType (enum)
    if (words.size() >= 5) {
      // Extract the name from words[3..N-2]
      std::string funcName = DecodeStringLiteral(words, 3);
      uint32_t linkageType = words[words.size() - 1];
      if (linkageType == spv::LinkageTypeExport) {
        exported_functions_.push_back(funcName);
      } else if (linkageType == spv::LinkageTypeImport) {
        imported_functions_.push_back(funcName);
      }
    }

    AddInstToProgram(words, spmd_program_);
    AddInstToProgram(words, esimd_program_);
  } else {
    AddInstToProgram(words, spmd_program_);
  }
  AddInstToProgram(words, esimd_program_);

  return llvm::Error::success();
}

llvm::Error SpvSplitter::HandleGroupDecorate(llvm::ArrayRef<uint32_t> words) {
  // OpGroupDecorate instruction format:
  // Word 0: WordCount and Opcode
  // Word 1: Decoration Group ID
  // Words 2..N: Target IDs to be decorated with the group

  IGC_ASSERT(words.size() >= 2);

  uint32_t groupId = words[1];

  if (esimd_decorated_ids_.find(groupId) != esimd_decorated_ids_.end()) {
    // Apply the group decoration to the target IDs
    for (size_t i = 2; i < words.size(); ++i) {
      uint32_t targetId = words[i];
      esimd_decorated_ids_.insert(targetId);
    }
  } else {
    AddInstToProgram(words, spmd_program_);
  }
  AddInstToProgram(words, esimd_program_);

  return llvm::Error::success();
}

llvm::Error SpvSplitter::HandleFunctionStart(llvm::ArrayRef<uint32_t> words) {
  // OpFunction instruction format:
  // Word 0: WordCount and Opcode
  // Word 1: Result Type ID
  // Word 2: Result ID
  // Word 3: Function Control
  // Word 4: Function Type ID

  IGC_ASSERT(words.size() >= 5);

  uint32_t resultId = words[2];

  if (esimd_decorated_ids_.find(resultId) != esimd_decorated_ids_.end()) {
    is_inside_esimd_function_ = true;
    has_esimd_functions_ = true;
    cur_esimd_function_id_ = resultId;

    AddInstToProgram(words, esimd_program_);
    AddInstToProgram(words, esimd_function_declarations_[cur_esimd_function_id_]);
  } else {
    is_inside_spmd_function_ = true;
    has_spmd_functions_ = true;
    AddInstToProgram(words, spmd_program_);
  }

  return llvm::Error::success();
}

llvm::Error SpvSplitter::HandleFunctionParameter(llvm::ArrayRef<uint32_t> words) {
  // OpFunctionParameter instruction format:
  // Word 0: WordCount and Opcode
  // Word 1: Result Type ID
  // Word 2: Result ID

  if (is_inside_esimd_function_) {
    AddInstToProgram(words, esimd_program_);
    AddInstToProgram(words, esimd_function_declarations_[cur_esimd_function_id_]);
  } else {
    AddInstToProgram(words, spmd_program_);
  }
  return llvm::Error::success();
}

llvm::Error SpvSplitter::HandleFunctionEnd(llvm::ArrayRef<uint32_t> words) {
  // OpFunctionEnd instruction has no operands other than the opcode and word
  // count

  if (is_inside_esimd_function_) {
    AddInstToProgram(words, esimd_program_);
    AddInstToProgram(words, esimd_function_declarations_[cur_esimd_function_id_]);
    cur_esimd_function_id_ = -1;
  }
  if (is_inside_spmd_function_) {
    AddInstToProgram(words, spmd_program_);
  }

  is_inside_esimd_function_ = false;
  is_inside_spmd_function_ = false;
  return llvm::Error::success();
}

llvm::Error SpvSplitter::HandleEntryPoint(llvm::ArrayRef<uint32_t> words) {
  // OpEntryPoint instruction format:
  // Word 0: WordCount and Opcode
  // Word 1: Execution Model
  // Word 2: Entry Point ID
  // Words 3..N: Name (string), Interface IDs

  IGC_ASSERT(words.size() >= 3);

  uint32_t entryPointId = words[2];
  entry_points_.insert(entryPointId);

  AddInstToProgram(words, spmd_program_);
  AddInstToProgram(words, esimd_program_);

  return llvm::Error::success();
}

llvm::Error SpvSplitter::HandleExecutionMode(llvm::ArrayRef<uint32_t> words) {
  // OpExecutionMode instruction format:
  // Word 0: WordCount and Opcode
  // Word 1: Entry Point ID
  // Word 2: Execution Mode
  // Words 3..N: Optional operands

  IGC_ASSERT(words.size() >= 3);

  uint32_t entryPointId = words[1];
  uint32_t execMode = words[2];

  if (execMode == spv::ExecutionModeSubgroupSize) {
    if (words.size() >= 4) {
      uint32_t sgSize = words[3];
      entry_point_to_subgroup_size_map_.insert({entryPointId, sgSize});
    } else {
      return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                     "OpExecutionMode SubgroupSize requires an additional operand");
    }
  }

  AddInstToProgram(words, spmd_program_);
  AddInstToProgram(words, esimd_program_);

  return llvm::Error::success();
}

void SpvSplitter::AddInstToProgram(llvm::ArrayRef<uint32_t> words, ProgramStreamType &program) {
  if (!only_detect_) {
    program.insert(program.end(), words.begin(), words.end());
  }
}

std::string SpvSplitter::DecodeStringLiteral(llvm::ArrayRef<uint32_t> words, size_t startIndex) {
  std::string result;
  for (size_t i = startIndex; i < words.size(); ++i) {
    uint32_t word = words[i];
    for (int j = 0; j < 4; ++j) {
      char c = (char)((word >> (j * 8)) & 0xFF);
      if (c == '\0') {
        return result;
      }
      result += c;
    }
  }
  return result;
}

} // namespace VLD
} // namespace IGC
