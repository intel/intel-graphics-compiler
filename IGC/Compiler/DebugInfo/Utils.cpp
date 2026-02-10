/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DIBuilder.h"

#include "Utils.h"

namespace IGC {
namespace Utils {

/// @brief return true if given module contain debug info
/// @param M The LLVM module.
/// @return true if given module contain debug info
bool HasDebugInfo(llvm::Module &M) {
  llvm::NamedMDNode *CU_Nodes = M.getNamedMetadata("llvm.dbg.cu");
  return (CU_Nodes != nullptr);
}

/// @brief creates a new call instruction to llvm.dbg.value intrinsic with
///        same information as in debug info of given global variable and
///        with value set to new given value.
/// @param pGlobalVar global variable to handle its debug info
/// @param pNewVal new value to map to the source variable (in the debug info)
/// @param pEntryPoint entry point instruction to add new instructions before.
/// @isIndirect true iff pNewValue type is a pointer to source variable type.
/// @return new call instruction to llvm.dbg.value intrinsic
llvm::Instruction *UpdateGlobalVarDebugInfo(llvm::GlobalVariable *pGlobalVar, llvm::Value *pNewVal,
                                            llvm::Instruction *pEntryPoint, bool isIndirect) {
  llvm::Function *userFunc = pEntryPoint->getParent()->getParent();
  llvm::Module &M = *userFunc->getParent();
  llvm::NamedMDNode *CU_Nodes = M.getNamedMetadata("llvm.dbg.cu");
  if (!CU_Nodes) {
    return nullptr;
  }

  llvm::DINode::DIFlags flags = llvm::DINode::FlagZero;
  llvm::DIScope *spScope = nullptr;
  llvm::DILocation *loc = nullptr;
  bool done = false;
  for (auto bbIt = userFunc->begin(); bbIt != userFunc->end() && !done; bbIt++) {
    for (auto instIt = bbIt->begin(); instIt != bbIt->end(); instIt++) {
      // Discover first valid Loc in function
      // and use it in dbg.declare nodes inserted
      // later. Make sure the location belongs to
      // the function and not to an inlined
      // callee.
      if (instIt->getDebugLoc() && !instIt->getDebugLoc().getInlinedAt()) {
        loc = instIt->getDebugLoc().get();
        spScope = loc->getScope()->getSubprogram();
        done = true;
        break;
      }
    }
  }

  llvm::SmallVector<llvm::DIGlobalVariableExpression *, 1> GVs;
  pGlobalVar->getDebugInfo(GVs);
  for (unsigned int j = 0; j < GVs.size(); j++) {
    IGCLLVM::DIBuilder Builder(M);
    llvm::DIGlobalVariable *GV = GVs[j]->getVariable();
    llvm::DIScope *scopeToUse = GV->getScope();
    llvm::DILocation *locToUse = llvm::DILocation::get(scopeToUse->getContext(), GV->getLine(), 0, scopeToUse, nullptr);
    if (llvm::isa<llvm::DICompileUnit>(scopeToUse) || llvm::isa<llvm::DINamespace>(scopeToUse)) {
      // Function has no DebugLoc so it is either internal
      // or optimized. So there is no point inserting
      // global var metadata as "local" to function.
      if (!done)
        continue;

      // Use scope of current sub-program
      scopeToUse = spScope;
      locToUse = loc;
    }
    llvm::DIVariable *Var = Builder.createAutoVariable(scopeToUse, GV->getDisplayName(),
                                                       Builder.createFile(GV->getFilename(), GV->getDirectory()),
                                                       GV->getLine(), GV->getType(), false, flags);

    if (isIndirect)
      return Builder.insertDeclare(pNewVal, llvm::cast<llvm::DILocalVariable>(Var), Builder.createExpression(),
                                   locToUse, pEntryPoint);

    return Builder.insertDbgValueIntrinsic(pNewVal, 0, llvm::cast<llvm::DILocalVariable>(Var),
                                           Builder.createExpression(), locToUse, pEntryPoint);
  }
  return nullptr;
}
} // namespace Utils
} // namespace IGC
