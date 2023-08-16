/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// clang-format off
#include "Utils.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DIBuilder.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

using namespace llvm;

namespace IGC {
namespace Utils {

bool hasDebugInfo(llvm::Module &M) {
  llvm::NamedMDNode *CU_Nodes = M.getNamedMetadata("llvm.dbg.cu");
  return (CU_Nodes != nullptr);
}

llvm::Instruction *updateGlobalVarDebugInfo(llvm::GlobalVariable *pGlobalVar,
                                            llvm::Value *pNewVal,
                                            llvm::Instruction *pEntryPoint,
                                            bool isIndirect) {
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
  for (auto bbIt = userFunc->begin(); bbIt != userFunc->end() && !done;
       bbIt++) {
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
    llvm::DILocation *locToUse = llvm::DILocation::get(
        scopeToUse->getContext(), GV->getLine(), 0, scopeToUse, nullptr);
    if (llvm::isa<llvm::DICompileUnit>(scopeToUse) ||
        llvm::isa<llvm::DINamespace>(scopeToUse)) {
      // Function has no DebugLoc so it is either internal
      // or optimized. So there is no point inserting
      // global var metadata as "local" to function.
      if (!done)
        continue;

      // Use scope of current sub-program
      scopeToUse = spScope;
      locToUse = loc;
    }
    llvm::DIVariable *Var = Builder.createAutoVariable(
        scopeToUse, GV->getDisplayName(),
        Builder.createFile(GV->getFilename(), GV->getDirectory()),
        GV->getLine(), GV->getType(), false, flags);

    if (isIndirect)
      return Builder.insertDeclare(
          pNewVal, llvm::cast<llvm::DILocalVariable>(Var),
          Builder.createExpression(), locToUse, pEntryPoint);

    return Builder.insertDbgValueIntrinsic(
        pNewVal, 0, llvm::cast<llvm::DILocalVariable>(Var),
        Builder.createExpression(), locToUse, pEntryPoint);
  }
  return nullptr;
}

unsigned int getSpecialDebugVariableHash(const std::string &name) {
  // Function returns:
  // 0 for __ocl_dbg_gid0
  // 1 for __ocl_dbg_gid1
  // 2 for __ocl_dbg_gid2
  // 3 for __ocl_dbg_lid0
  // 4 for __ocl_dbg_lid1
  // 5 for __ocl_dbg_lid2
  // 6 for __ocl_dbg_grid0
  // 7 for __ocl_dbg_grid1
  // 8 for __ocl_dbg_grid2

  unsigned int idx = 0;

  // 0 for ocl_dbg_gid*, so at position size-4 check only 'l' for lid and 'r'
  // for grid
  size_t significantCharPos = name.size() - 4;
  if (name.at(significantCharPos) == 'l')
    idx = 3;
  else if (name.at(significantCharPos) == 'r')
    idx = 6;

  idx += std::strtoul(name.substr(name.size() - 1).data(), nullptr, 0);

  return idx;
}

bool isSpecialDebugVariable(const std::string &name) {
  return (name.find("__ocl_dbg", 0) == 0);
}

std::string getSpecialVariableMetaName(const llvm::Instruction *inst) {
  // Check for OCL special debug variable such as in metadata
  //   "preserve__ocl_dbg_gid0"
  //   "preserve__ocl_dbg_gid1"
  //   "preserve__ocl_dbg_gid2"
  std::string names[__OCL_DBG_VARIABLES] = {
      "__ocl_dbg_gid0",  "__ocl_dbg_gid1",  "__ocl_dbg_gid2",
      "__ocl_dbg_lid0",  "__ocl_dbg_lid1",  "__ocl_dbg_lid2",
      "__ocl_dbg_grid0", "__ocl_dbg_grid1", "__ocl_dbg_grid2"};

  for (int i = 0; i < __OCL_DBG_VARIABLES; i++) {
    if (inst->getMetadata(names[i]))
      return names[i];
  }

  return "";
}

int32_t getSourceLangLiteralMDValue(const llvm::Module &module) {
  auto sourceLangLiteral = module.getModuleFlag("Source Lang Literal");
  if (!sourceLangLiteral)
    return SOURCE_LANG_LITERAL_MD_IS_NOT_PRESENT;

  auto constant = llvm::cast<llvm::ConstantAsMetadata>(sourceLangLiteral);
  return int32_t(
      llvm::cast<llvm::ConstantInt>(constant->getValue())->getZExtValue());
}

uint16_t getSourceLanguage(llvm::DICompileUnit *compileUnit,
                           const llvm::Module *module) {
  int32_t sourceLanguage = getSourceLangLiteralMDValue(*module);
  if (sourceLanguage == SOURCE_LANG_LITERAL_MD_IS_NOT_PRESENT) {
    sourceLanguage = compileUnit->getSourceLanguage();
  }
  return uint16_t(sourceLanguage);
}

void eraseAddressClassDIExpression(llvm::Function &F) {
  DIBuilder di(*F.getParent());

  for (auto &bb : F) {
    for (auto &pInst : bb) {
      if (auto *DI = dyn_cast<DbgVariableIntrinsic>(&pInst)) {
        const DIExpression *DIExpr = DI->getExpression();
        llvm::SmallVector<uint64_t, 5> newElements;
        for (auto I = DIExpr->expr_op_begin(), E = DIExpr->expr_op_end();
             I != E; ++I) {
          if (I->getOp() == dwarf::DW_OP_constu) {
            auto patternI = I;
            if (++patternI != E && patternI->getOp() == dwarf::DW_OP_swap &&
                ++patternI != E && patternI->getOp() == dwarf::DW_OP_xderef) {
              I = patternI;
              continue;
            }
          }
          I->appendToVector(newElements);
        }

        if (newElements.size() < DIExpr->getNumElements()) {
          DIExpression *newDIExpr = di.createExpression(newElements);
#if LLVM_VERSION_MAJOR < 13
          DI->setArgOperand(
              2, MetadataAsValue::get(newDIExpr->getContext(), newDIExpr));
#else
          DI->setExpression(newDIExpr);
#endif
        }
      }
    }
  }
}

} // namespace Utils
} // namespace IGC
