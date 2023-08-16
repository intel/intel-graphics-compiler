/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IntrinsicInst.h"
#include "common/LLVMWarningsPop.hpp"

#include <unordered_set>

using namespace llvm;

namespace IGC {
class CVariable;
class VISAModule;
class CShader;
class CVariable;
class IDebugEmitter;

class CShaderDebugInfo {
public:
  CShader *m_pShader = nullptr;
  IDebugEmitter *m_pDebugEmitter = nullptr;

  // Store mapped VISAModule for every Function in CShader.
  llvm::DenseMap<llvm::Function *, VISAModule *> m_VISAModules;

  // Store mapping of llvm::Value->CVariable per llvm::Function.
  // The mapping is obtained from CShader at end of EmitVISAPass for F.
  llvm::DenseMap<const llvm::Function *,
                 llvm::DenseMap<llvm::Value *, CVariable *>>
      m_FunctionSymbols;

  // Adds Function -> VISAModule mapping to the mapping table.
  void addVISAModule(llvm::Function *F, VISAModule *m);

  // Saves CVariable -> VISA register mappings.
  // This needs to be done because CShader clear that values
  // after processing every llvm::Function.
  void saveVISAIdMappings(const llvm::Function &F);

  // Returns CVariable pointer mapped to given Value pointer from F.
  CVariable *getMapping(const llvm::Function &F, const llvm::Value *V);

  // Returns register number mapped to CVariable.
  // SIMD32 variables may occupy 2 registers, so index might be 0 or 1.
  unsigned int getVISADclId(const CVariable *CVar, unsigned index);

  void markAsOutputNoOffset(llvm::Function &F);
  void markVarAsOutput(const llvm::Instruction *pInst);

  void markAsOutputWithOffset(llvm::Function &F);
  void markPrivateBaseAsOutput();
  void markInstAsOutput(llvm::Instruction *pInst, const char *pMetaDataName);

private:
  // CVariable -> register pair mapping.
  llvm::DenseMap<const CVariable *, std::pair<unsigned int, unsigned int>>
      CVarToVISADclId;

  std::unordered_set<const CVariable *> m_outputVals;
};
}; // namespace IGC
