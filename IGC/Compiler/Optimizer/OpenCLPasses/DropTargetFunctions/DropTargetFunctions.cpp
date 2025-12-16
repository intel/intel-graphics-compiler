/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/DropTargetFunctions/DropTargetFunctions.h"

#include "Probe/Assertion.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"

#include "CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"
#include "common/igc_regkeys.hpp"

#include <fstream>
#include <llvm/IR/Module.h>
#include <string>
#include <filesystem>

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-drop-target-fns"
#define PASS_DESCRIPTION "Drop target functions."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(DropTargetFunctions, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(DropTargetFunctions, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

#define DROP_FN_DEBUG(msg)                                                                                             \
  do {                                                                                                                 \
    if (VerboseLog) {                                                                                                  \
      llvm::errs() << msg << "\n";                                                                                     \
    }                                                                                                                  \
  } while (0)

char DropTargetFunctions::ID = 0;

DropTargetFunctions::DropTargetFunctions()
    : ModulePass(ID), VerboseLog(IGC_GET_FLAG_VALUE(VerboseDropTargetFunctions)) {
  initializeDropTargetFunctionsPass(*PassRegistry::getPassRegistry());
}

static llvm::Function *getPlaceholderFn(llvm::Function *F) {
  llvm::Function *NewF =
      llvm::Function::Create(F->getFunctionType(), F->getLinkage(), "tmp_dropped_fn", F->getParent());
  NewF->takeName(F);
  NewF->copyAttributesFrom(F);
  NewF->setCallingConv(F->getCallingConv());

  LLVMContext &Ctx = NewF->getContext();
  BasicBlock *BB = BasicBlock::Create(Ctx, "drop_entry", NewF);
  IRBuilder<> builder(BB);

  if (IGC_GET_FLAG_VALUE(CrashOnDroppedFnAccess)) {
    PointerType *PtrAsTy = PointerType::get(Type::getInt32Ty(Ctx), ::IGC::ADDRESS_SPACE_GLOBAL);
    Value *NullPtr = Constant::getNullValue(PtrAsTy);
    builder.CreateStore(ConstantInt::get(Type::getInt32Ty(Ctx), 42), NullPtr, true);
    builder.CreateUnreachable();

    if (NewF->hasFnAttribute(llvm::Attribute::AlwaysInline)) {
      NewF->removeFnAttr(llvm::Attribute::AlwaysInline);
    }
    NewF->addFnAttr(llvm::Attribute::NoInline);
    NewF->addFnAttr(llvm::Attribute::OptimizeNone);
  } else {
    builder.CreateUnreachable();
  }

  return NewF;
}

bool DropTargetFunctions::runOnModule(llvm::Module &M) {

  IGCMD::MetaDataUtils *MdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  DROP_FN_DEBUG("DropTargetFunctions: Starting ...");

  // Find kernel function
  Function *Kernel = nullptr;
  for (auto i = MdUtils->begin_FunctionsInfo(), e = MdUtils->end_FunctionsInfo(); i != e; ++i) {
    auto *KernelFunc = i->first;
    if (isEntryFunc(MdUtils, KernelFunc)) {
      if (Kernel != nullptr) {
        DROP_FN_DEBUG("DropTargetFunctions: File with multiple kernels... SKIPPING ...");
        return false;
      }
      Kernel = KernelFunc;
    }
  }

  IGC_ASSERT(Kernel != nullptr);

  const char *DropFnListBasePath = IGC_GET_REGKEYSTRING(DropTargetFnListPath);
  SmallVector<std::string, 4> FnNamesToDrop;
  std::filesystem::path DropFnListPath = std::filesystem::path(DropFnListBasePath) / (Kernel->getName().str() + ".txt");
  {
    std::ifstream DropFnFile(DropFnListPath);
    if (!DropFnFile.is_open()) {
      DROP_FN_DEBUG("DropTargetFunctions: Could not open DropTargetFnListPath file: " << DropFnListPath.string());
      return false;
    }

    std::string line;
    while (std::getline(DropFnFile, line)) {
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      FnNamesToDrop.push_back(line);
    }
    DropFnFile.close();
  }
  DROP_FN_DEBUG("DropTargetFunctions: Start FN Checks ...");

  SmallPtrSet<Function *, 8> FnsToDrop;
  SmallPtrSet<Function *, 8> Processed;
  for (auto &F : M.functions()) {
    if (F.isDeclaration()) {
      continue;
    }
    if (!Processed.contains(&F) &&
        llvm::any_of(FnNamesToDrop, [&F](std::string &Name) { return Name == F.getName(); })) {
      auto *EmptyFn = getPlaceholderFn(&F);
      Processed.insert(EmptyFn);
      DROP_FN_DEBUG("Dropping function: " << EmptyFn->getName());
      F.replaceAllUsesWith(EmptyFn);
      FnsToDrop.insert(&F);
    }
  }

  for (auto *F : FnsToDrop) {
    F->eraseFromParent();
  }
  DROP_FN_DEBUG("DropTargetFunctions: Exiting ...");

  return FnsToDrop.size() > 0;
}
