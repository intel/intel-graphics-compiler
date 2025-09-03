/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/DropTargetBBs.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/IGCPassSupport.h"
#include "common/igc_regkeys.hpp"

#include <fstream>
#include <string>
#include <optional>
#include <filesystem>

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-drop-target-bbs"
#define PASS_DESCRIPTION "Drop target basic blocks"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(DropTargetBBs, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(DropTargetBBs, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

#define DROP_BB_DEBUG(msg)                                                                                             \
  do {                                                                                                                 \
    if (VerboseLog) {                                                                                                  \
      llvm::errs() << msg << "\n";                                                                                     \
    }                                                                                                                  \
  } while (0)

char IGC::DropTargetBBs::ID = 0;

DropTargetBBs::DropTargetBBs() : FunctionPass(ID), VerboseLog(IGC_GET_FLAG_VALUE(VerboseDropTargetBBs)) {
  initializeDropTargetBBsPass(*PassRegistry::getPassRegistry());
}

void DropTargetBBs::getAnalysisUsage(llvm::AnalysisUsage &AU) const {}

static std::optional<int> getBBIndexByName(Function &F, const std::string &BBName) {
  int bbIndex = 0;
  for (auto &BB : F) {
    if (BB.hasName() && BB.getName() == BBName) {
      return bbIndex;
    }
    bbIndex++;
  }
  return std::nullopt;
}

static std::optional<SmallVector<std::string, 8>> getBBNamesToDrop(const Function &F, const char *DropBBListBasePath) {
  SmallVector<std::string, 8> BBsToDrop;
  std::filesystem::path DropBBListPath = std::filesystem::path(DropBBListBasePath) / (F.getName().str() + ".txt");
  {
    std::ifstream DropBBFile(DropBBListPath);
    if (!DropBBFile.is_open()) {
      return std::nullopt;
    }

    std::string line;
    while (std::getline(DropBBFile, line)) {
      if (line.empty()) {
        continue;
      }

      if (line.back() == '\r') {
        line.pop_back();
      }

      BBsToDrop.push_back(line);
    }
    DropBBFile.close();
  }
  return BBsToDrop;
}

bool DropTargetBBs::runOnFunction(Function &F) {
  if (F.isDeclaration()) {
    return false;
  }

  auto BBsToDropOptional = getBBNamesToDrop(F, IGC_GET_REGKEYSTRING(DropTargetBBListPath));
  if (!BBsToDropOptional.has_value()) {
    return false;
  }

  const auto &BBsToDrop = BBsToDropOptional.value();
  SmallVector<std::uint32_t, 8> BBIndexesToDrop;
  for (const auto &BBName : BBsToDrop) {
    auto BBIndex = getBBIndexByName(F, BBName);
    if (BBIndex.has_value()) {
      BBIndexesToDrop.push_back(BBIndex.value());
    }
  }

  if (BBIndexesToDrop.empty()) {
    return false;
  }

  DROP_BB_DEBUG("DropTargetBBss: Examining function: " << F.getName());
  DROP_BB_DEBUG("DropTargetBBss: BB num: " << F.size());

  for (const auto &BBIndex : BBIndexesToDrop) {
    if (BBIndex >= F.size()) {
      continue;
    }
    auto BI = F.begin();
    std::advance(BI, BBIndex);
    BasicBlock *BB = &*BI;
    DROP_BB_DEBUG("DropTargetBBs: Dropping BB index: " << BBIndex << " Name: " << BB->getName());

    while (!BB->use_empty()) {
      auto *User = *BB->user_begin();
      if (auto *PHI = dyn_cast<PHINode>(User)) {
        PHI->removeIncomingValue(BB, true);
      } else if (auto *Br = dyn_cast<BranchInst>(User)) {
        BasicBlock *NextBB = BB->getNextNode();
        if (NextBB) {
          Br->setSuccessor(Br->getSuccessor(0) == BB ? 0 : 1, NextBB);
        } else {
          Br->eraseFromParent();
        }
      } else if (auto *SI = dyn_cast<SwitchInst>(User)) {
        if (auto *CI = SI->findCaseDest(BB)) {
          SI->removeCase(SI->findCaseValue(CI));
        }
      } else {
        User->replaceUsesOfWith(BB, PoisonValue::get(BB->getType()));
      }
    }

    for (Instruction &I : *BB) {
      if (!I.use_empty()) {
        I.replaceAllUsesWith(PoisonValue::get(I.getType()));
      }
    }

    BB->eraseFromParent();
  }

  return true;
}
