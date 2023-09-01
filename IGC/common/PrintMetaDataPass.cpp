/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Instructions.h"
#include "common/LLVMWarningsPop.hpp"

#include "PrintMetaDataPass.h"
#include <Probe/Assertion.h>

namespace IGC {
char PrintMetaDataPass::ID = 0;

PrintMetaDataPass::PrintMetaDataPass(llvm::raw_ostream &Stream)
    : Stream(Stream), ModulePass(ID) {}

bool PrintMetaDataPass::runOnModule(llvm::Module &M) {
  CollectModuleMD(&M);

  for (auto func_i = M.begin(); func_i != M.end(); ++func_i) {
    CollectFunctionMD(&*func_i);
  }

  PrintAllMD(&M);

  return false;
}

void PrintMetaDataPass::PrintAllMD(llvm::Module *pM) {
  PrintNamedMD(pM);

  for (auto allMD_i = allMD.begin(); allMD_i != allMD.end(); ++allMD_i) {
    (*allMD_i)->print(Stream, pM);
    // Add new line
    Stream << "\n";
  }
}

void PrintMetaDataPass::PrintNamedMD(llvm::Module *M) {
  for (auto md_i = M->named_metadata_begin(); md_i != M->named_metadata_end();
       ++md_i) {
    auto Node = &*md_i;

    if (Node) {
      Node->print(Stream);
      // Add new line
      Stream << "\n";
    }
  }
}

void PrintMetaDataPass::CollectModuleMD(llvm::Module *M) {
  for (auto md_i = M->named_metadata_begin(); md_i != M->named_metadata_end();
       ++md_i) {
    auto Node = &*md_i;

    if (Node) {
      for (auto node_i = Node->op_begin(); node_i != Node->op_end(); ++node_i) {
        CollectInsideMD(*node_i);
      }
    }
  }

  for (auto global_i = M->global_begin(); global_i != M->global_end();
       ++global_i) {
    CollectValueMD(&*global_i);
  }
}

void PrintMetaDataPass::CollectFunctionMD(llvm::Function *Func) {
  CollectValueMD(Func);

  for (auto bb_i = Func->begin(); bb_i != Func->end(); ++bb_i) {
    for (auto inst_i = bb_i->begin(); inst_i != bb_i->end(); ++inst_i) {
      CollectValueMD(&*inst_i);
    }
  }
}

void PrintMetaDataPass::CollectValueMD(llvm::Value *Val) {
  if (Val) {
    llvm::SmallVector<std::pair<unsigned, llvm::MDNode *>, 8> MDs;

    if (auto func = llvm::dyn_cast<llvm::Function>(Val)) {
      func->getAllMetadata(MDs);
    } else if (auto instr = llvm::dyn_cast<llvm::Instruction>(Val)) {
      instr->getAllMetadata(MDs);

      if (auto callInstr = llvm::dyn_cast<llvm::CallInst>(instr)) {
        if (auto callFunc = callInstr->getCalledFunction()) {
          if (callFunc->getName().startswith("llvm.")) {
            for (unsigned i = 0; i < instr->getNumOperands(); ++i) {
              if (auto valAsMD = llvm::dyn_cast<llvm::MetadataAsValue>(
                      instr->getOperand(i))) {
                CollectInsideMD(valAsMD->getMetadata());
              }
            }
          }
        }
      }
    } else if (auto global = llvm::dyn_cast<llvm::GlobalVariable>(Val)) {
      global->getAllMetadata(MDs);
    } else {
      IGC_ASSERT_MESSAGE(false, "Unrecognized type");
    }

    for (auto md_record = MDs.begin(); md_record != MDs.end(); ++md_record) {
      auto mdnode = md_record->second;
      CollectInsideMD(mdnode);
    }
  }
}

void PrintMetaDataPass::CollectInsideMD(llvm::MDNode *Node) {
  if (Node) {
    if (std::find(allMD.begin(), allMD.end(), Node) == allMD.end()) {
      allMD.push_back(Node);

      for (auto node_i = Node->op_begin(); node_i != Node->op_end(); ++node_i) {
        CollectInsideMD(*node_i);
      }
    }
  }
}

void PrintMetaDataPass::CollectInsideMD(llvm::Metadata *Node) {
  if (Node) {
    if (auto mdnode_op = llvm::dyn_cast<llvm::MDNode>(Node)) {
      CollectInsideMD(mdnode_op);
    }
  }
}
} // namespace IGC
