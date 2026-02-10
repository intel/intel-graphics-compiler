/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SetVector.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/IPO.h>
#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/IPO/GlobalDCE.h"
#include "llvmWrapper/Transforms/IPO/StripSymbols.h"
#include "llvmWrapper/Transforms/IPO/StripDeadPrototypes.h"

#include <common/LLVMUtils.h>
#include <common/ModuleSplitter.h>
#include <Compiler/CodeGenPublic.h>
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"

namespace IGC {
KernelModuleSplitter::KernelModuleSplitter(IGC::OpenCLProgramContext &oclContext, llvm::Module &module)
    : _oclContext(oclContext), _originalModule(module), _splittedModule(nullptr) {}

KernelModuleSplitter::~KernelModuleSplitter() { restoreOclContextModule(); }

void KernelModuleSplitter::splitModuleForKernel(const llvm::Function *kernelF) {
  using namespace llvm;
  IGC_ASSERT_EXIT_MESSAGE(kernelF != nullptr, "Cannot split for null function!");

  std::vector<const Function *> workqueue;
  SetVector<const GlobalValue *> GVs;

  // add all functions called by the kernel, recursively
  // start with the kernel...
  GVs.insert(kernelF);
  workqueue.push_back(kernelF);

  // and for all called functions...
  while (!workqueue.empty()) {
    const Function *F = workqueue.back();
    workqueue.pop_back();

    for (const auto &I : instructions(F)) {
      if (const CallBase *CB = dyn_cast<CallBase>(&I)) {
        if (const Function *CF = CB->getCalledFunction()) {
          if (CF->isDeclaration() || GVs.count(CF))
            continue;

          // add only defined ones and rerun for their's calls ...
          GVs.insert(CF);
          workqueue.push_back(CF);
        }
      }
    }
  }

  // add all globals - it's easier to let them be removed later than search for them here
  for (auto &GV : _originalModule.globals()) {
    GVs.insert(&GV);
  }

  // create new module with selected globals and functions
  ValueToValueMapTy VMap;
  std::unique_ptr<Module> kernelM =
      CloneModule(_originalModule, VMap, [&](const GlobalValue *GV) { return GVs.count(GV); });
  IGC_ASSERT_EXIT_MESSAGE(kernelM, "Cloning module failed!");

  // Do cleanup.
  IGC::IGCPassManager mpm(&_oclContext, "CleanupAfterModuleSplitting");
  mpm.add(IGCLLVM::createLegacyWrappedGlobalDCEPass());           // Delete unreachable globals.
  mpm.add(IGCLLVM::createLegacyWrappedStripDeadDebugInfoPass());  // Remove dead debug info.
  mpm.add(IGCLLVM::createLegacyWrappedStripDeadPrototypesPass()); // Remove dead func decls.

  mpm.run(*kernelM.get());
  _splittedModule = std::move(kernelM);
}

void KernelModuleSplitter::retry() {
  if (_splittedModule) {
    restoreOclContextModule();
    delete _splittedModule.release();
  }
}

void KernelModuleSplitter::restoreOclContextModule() {
  if (_splittedModule) {
    _oclContext.clearMD();
    _oclContext.setModule(&_originalModule);
  }
}

void KernelModuleSplitter::setSplittedModuleInOCLContext() {
  if (_splittedModule) {
    _oclContext.clearMD();
    _oclContext.setModule(_splittedModule.get());
  }
}
} // namespace IGC