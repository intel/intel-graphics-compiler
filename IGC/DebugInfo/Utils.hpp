/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DIBuilder.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "common/LLVMWarningsPop.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "Probe/Assertion.h"
// clang-format on

namespace IGC {
namespace Utils {

#define __OCL_DBG_VARIABLES 9

/// @brief Return true if given module contains debug info.
/// @param M The LLVM module.
/// @return True if given module contains debug info.
bool hasDebugInfo(llvm::Module &M);

/// @brief Creates a new call instruction to llvm.dbg.value intrinsic with
///        same information as in debug info of given global variable and
///        with value set to new given value.
/// @param pGlobalVar  Global variable to handle its debug info.
/// @param pNewVal     New value to map to the source variable (in the debug
/// info).
/// @param pEntryPoint Entry point instruction to add new instructions before.
/// @param isIndirect  True if pNewValue type is a pointer to source variable
/// type.
/// @return New call instruction to llvm.dbg.value intrinsic
llvm::Instruction *updateGlobalVarDebugInfo(llvm::GlobalVariable *pGlobalVar,
                                            llvm::Value *pNewVal,
                                            llvm::Instruction *pEntryPoint,
                                            bool isIndirect);

/// @brief Calculate hash index for OCL special debug variables.
/// @param Variable name like: __ocl_dbg_gid0.
/// @return Hash index for variable name.
unsigned int getSpecialDebugVariableHash(const std::string &name);

/// @brief Check for OCL special debug variable such as __ocl_dbg_gid0.
///        Assumes all special variables start with __ocl_dbg.
/// @param Variable name like: __ocl_dbg_gid0.
/// @return True if variable starts with "__ocl_dbg" prefix.
bool isSpecialDebugVariable(const std::string &name);

/// @brief Check for OCL special debug variable such as in metadata.
///        Assumes all special variables start with __ocl_dbg.
/// @param Instruction with target metadata.
/// @return Special variable name if inst has corresponding metadata.
std::string getSpecialVariableMetaName(const llvm::Instruction *inst);

int32_t getSourceLangLiteralMDValue(const llvm::Module &module);
uint16_t getSourceLanguage(llvm::DICompileUnit *compileUnit,
                           const llvm::Module *module);

/// @brief Detect instructions with an address class pattern.
///        Then remove all opcodes of this pattern from
///        this instruction's last operand (metadata of DIExpression).
/// @param Function to erase DIExpression's from.
void eraseAddressClassDIExpression(llvm::Function &F);

template <class ContainerType, class BinaryFunction,
          class Sorter = std::less<typename ContainerType::key_type>>
static void OrderedTraversal(const ContainerType &Data, BinaryFunction Visit,
                             Sorter SortProcedure = {}) {
  std::vector<typename ContainerType::key_type> Keys;
  std::transform(Data.begin(), Data.end(), std::back_inserter(Keys),
                 [](const auto &KV) { return KV.first; });

  std::sort(Keys.begin(), Keys.end(), SortProcedure);
  for (const auto &Key : Keys) {
    auto FoundIt = Data.find(Key);
    IGC_ASSERT(FoundIt != Data.end());
    const auto &Val = FoundIt->second;
    Visit(Key, Val);
  }
}

constexpr int32_t SOURCE_LANG_LITERAL_MD_IS_NOT_PRESENT = -1;

//
// Flag "Source Lang Literal" contains sourceLanguage.
//
// Example:
//   !llvm.module.flags = !{..., !3, ...}
//   ...
//   !3 = !{i32 2, !"Source Lang Literal", 33}
//   ...
//
inline int32_t getSourceLangLiteralMDValueLegacy(const llvm::Module *module) {
  auto sourceLangLiteral = module->getModuleFlag("Source Lang Literal");
  if (!sourceLangLiteral) {
    return SOURCE_LANG_LITERAL_MD_IS_NOT_PRESENT;
  }

  auto constant = llvm::cast<llvm::ConstantAsMetadata>(sourceLangLiteral);
  return int32_t(
      llvm::cast<llvm::ConstantInt>(constant->getValue())->getZExtValue());
}

//
// Flag "Source Lang Literal" contains a list (MDTuple) of pairs (MDTuple):
//   (compileUnit, sourceLanguage)
//
// Example:
//   !llvm.module.flags = !{..., !3, ...}
//   ...
//   !3 = !{i32 2, !"Source Lang Literal", !4}
//   !4 = !{!5, !1834, ...}
//   !5 = !{!6, i32 33}
//   !6 = !DICompileUnit(...
//   ...
//   !1834 = !{!1835, i32 33}
//   !1835 = !DICompileUnit(...
//   ...
//
inline int32_t
getSourceLangLiteralMDValue(const llvm::DICompileUnit *compileUnit,
                            const llvm::Module *module) {
  const auto flagName = "Source Lang Literal";

  if (!module->getModuleFlag(flagName)) {
    return SOURCE_LANG_LITERAL_MD_IS_NOT_PRESENT;
  }

  auto node = llvm::dyn_cast<llvm::MDTuple>(module->getModuleFlag(flagName));
  if (!node) {
    return getSourceLangLiteralMDValueLegacy(module);
  }

  for (auto &op : node->operands()) {
    auto entry = llvm::cast<llvm::MDTuple>(op);
    if (llvm::cast<llvm::DICompileUnit>(entry->getOperand(0)) == compileUnit) {
      auto constant =
          llvm::cast<llvm::ConstantAsMetadata>(entry->getOperand(1));
      return int32_t(
          llvm::cast<llvm::ConstantInt>(constant->getValue())->getZExtValue());
    }
  }

  return SOURCE_LANG_LITERAL_MD_IS_NOT_PRESENT;
}

inline uint16_t getSourceLanguage(const llvm::DICompileUnit *compileUnit,
                                  const llvm::Module *module) {
  int32_t sourceLanguage = getSourceLangLiteralMDValue(compileUnit, module);
  if (sourceLanguage == SOURCE_LANG_LITERAL_MD_IS_NOT_PRESENT) {
    sourceLanguage = compileUnit->getSourceLanguage();
  }
  return uint16_t(sourceLanguage);
}

} // namespace Utils
} // namespace IGC
