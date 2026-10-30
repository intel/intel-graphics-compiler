/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/PullKernelArguments/PullKernelArguments.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/helper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-pull-kernel-args"
#define PASS_DESCRIPTION "Keep kernel arguments exceeding the push budget in cross-thread memory"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PullKernelArgumentsLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(PullKernelArgumentsLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PullKernelArgumentsLPM::ID = 0;

PullKernelArgumentsLPM::PullKernelArgumentsLPM() : ModulePass(ID) {
  initializePullKernelArgumentsLPMPass(*PassRegistry::getPassRegistry());
}

namespace {
struct ArgDescriptor {
  const Argument *Arg;
  KernelArg::ArgType Type;
  uint64_t Size;
};

static bool isPullableArgType(KernelArg::ArgType Type) {
  switch (Type) {
  case KernelArg::ArgType::STRUCT:
  case KernelArg::ArgType::CONSTANT_REG:
  case KernelArg::ArgType::PTR_GLOBAL:
  case KernelArg::ArgType::PTR_CONSTANT:
  case KernelArg::ArgType::PTR_LOCAL:
    return true;
  default:
    return false;
  }
}
} // namespace

bool PullKernelArguments::run(Module &M, IGCMD::MetaDataUtils *pMdUtils, CodeGenContext *pCtx) {
  if (!AllowPullingKernelArguments(pCtx)) {
    return false;
  }

  const DataLayout &DL = M.getDataLayout();
  ModuleMetaData *ModMD = pCtx->getModuleMetaData();
  const uint64_t MaxParamSize = pCtx->platform.getMaxOCLParameteSize();

  for (Function &F : M) {
    if (!isEntryFunc(pMdUtils, &F)) {
      continue;
    }

    FunctionMetaData &FuncMD = ModMD->FuncMD[&F];

    uint64_t TotalSize = 0;
    SmallVector<ArgDescriptor, 16> AllArgs;

    for (auto &Arg : F.args()) {
      const unsigned ArgNo = Arg.getArgNo();
      // Make sure that we don't go out of bounds when accessing the base type metadata.
      // Empty string classifies only by type in calcArgType()
      StringRef TypeStr =
          ArgNo < FuncMD.m_OpenCLArgBaseTypes.size() ? StringRef(FuncMD.m_OpenCLArgBaseTypes[ArgNo]) : "";
      const uint64_t Size = getKernelArgAllocSize(Arg, DL);
      AllArgs.push_back({&Arg, KernelArg::calcArgType(&Arg, TypeStr), Size});
      TotalSize += Size;
    }

    // Skip if the total size of kernel arguments is within the limit
    if (TotalSize <= MaxParamSize) {
      continue;
    }

    bool Overflow = false;
    uint64_t PushedSize = 0;
    SmallVector<ArgDescriptor, 16> ArgsToPull;

    // Reserve the size for args that cannot be pulled e.g. image
    for (const ArgDescriptor &ArgDesc : AllArgs) {
      if (!isPullableArgType(ArgDesc.Type)) {
        PushedSize += ArgDesc.Size;
      }
    }

    IGC_ASSERT_MESSAGE(PushedSize <= MaxParamSize, "Non-pullable kernel arguments exceed the push budget");

    for (const ArgDescriptor &ArgDesc : AllArgs) {
      // Non-pullable arguments are already counted in PushedSize
      if (!isPullableArgType(ArgDesc.Type)) {
        continue;
      }
      if (!Overflow && PushedSize + ArgDesc.Size <= MaxParamSize) {
        PushedSize += ArgDesc.Size;
        continue;
      }

      Overflow = true;
      ArgsToPull.push_back(ArgDesc);
    }

    IGC_ASSERT_MESSAGE(FuncMD.pulledArgs.empty(), "pulledArgs already selected for this kernel");
    for (const ArgDescriptor &ArgDesc : ArgsToPull) {
      FuncMD.pulledArgs.push_back(ArgDesc.Arg->getArgNo());
    }
  }

  return false;
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses PullKernelArgumentsNPM::run(Module &M, ModuleAnalysisManager &AM) {
  auto &MDU = AM.getResult<MetaDataUtilsAnalysis>(M);
  CodeGenContext *pCtx = AM.getResult<CodeGenContextAnalysis>(M).Ctx;
  bool Changed = PullKernelArguments().run(M, MDU.MdUtils, pCtx);
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
