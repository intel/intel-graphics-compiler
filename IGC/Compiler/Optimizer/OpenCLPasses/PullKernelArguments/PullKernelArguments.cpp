/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/PullKernelArguments/PullKernelArguments.hpp"
#include "Compiler/Optimizer/OpenCLPasses/AggregateArguments/AggregateArguments.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/helper.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/IRBuilder.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

#include <string>

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
  Argument *Arg;
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

// Emit a ptr to the argument slot in cross-thread memory
static Value *createKernelArgPtr(IGCLLVM::IRBuilder<> &B, Argument &Arg) {
  PointerType *PtrTy = IGCLLVM::PointerType::get(Arg.getType(), ADDRESS_SPACE_THREAD_ARG);
  Function *GetArgPtr =
      GenISAIntrinsic::getDeclaration(Arg.getParent()->getParent(), GenISAIntrinsic::GenISA_getKernelArgPtr, PtrTy);
  return B.CreateCall(GetArgPtr, B.getInt32(Arg.getArgNo()), Arg.getName() + "_pulled");
}

// Replace all uses of arg with a load of its value from cross-thread memory
static void pullArgument(Function &F, Argument &Arg, const DataLayout &DL) {
  Type *ArgTy = Arg.getType();
  BasicBlock &Entry = F.getEntryBlock();
  IGCLLVM::IRBuilder<> B(&Entry, Entry.getFirstInsertionPt());
  Value *ArgPtr = createKernelArgPtr(B, Arg);

  Align ArgAlign = getPulledKernelArgAlign(Arg, DL);
  LoadInst *Val = B.CreateAlignedLoad(ArgTy, ArgPtr, ArgAlign, Arg.getName());
  Val->setMetadata(LLVMContext::MD_invariant_load, MDNode::get(F.getContext(), {}));
  Arg.replaceAllUsesWith(Val);
}

// Replace byval struct arg with a private copy filled from cross-thread memory
static void rewriteByValArg(Function &F, Argument &Arg, const DataLayout &DL) {
  Type *StructTy = Arg.getParamByValType();
  BasicBlock &Entry = F.getEntryBlock();
  IGCLLVM::IRBuilder<> B(&Entry, Entry.getFirstInsertionPt());
  Value *ArgPtr = createKernelArgPtr(B, Arg);

  AllocaInst *Copy = B.CreateAlloca(StructTy, nullptr, Arg.getName() + "_copy");
  Copy->setAlignment(getKernelArgAlign(Arg, DL));
  B.CreateMemCpy(Copy, Copy->getAlign(), ArgPtr, getPulledKernelArgAlign(Arg, DL), getKernelArgAllocSize(Arg, DL));
  Arg.replaceAllUsesWith(Copy);
}

// Replace extractvalue uses of struct value arg with loads of its members from cross-thread memory
static void rewriteStructValueArg(Function &F, Argument &Arg, const DataLayout &DL) {
  BasicBlock &Entry = F.getEntryBlock();
  IGCLLVM::IRBuilder<> B(&Entry, Entry.getFirstInsertionPt());
  Value *ArgPtr = createKernelArgPtr(B, Arg);
  replaceStructValueUses(&Arg, ArgPtr, Arg.getType(), getPulledKernelArgAlign(Arg, DL), B, /*InvariantLoads=*/true);
}
} // namespace

bool PullKernelArguments::run(Module &M, IGCMD::MetaDataUtils *pMdUtils, CodeGenContext *pCtx) {
  if (!AllowPullingKernelArguments(pCtx)) {
    return false;
  }

  const DataLayout &DL = M.getDataLayout();
  ModuleMetaData *ModMD = pCtx->getModuleMetaData();
  const uint64_t MaxParamSize = pCtx->platform.getMaxOCLParameteSize();
  bool Changed = false;

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

    if (PushedSize > MaxParamSize) {
      std::string ErrorMsg =
          "Total size of kernel arguments that cannot be pulled exceeds limit! Size: " + std::to_string(PushedSize) +
          ", limit: " + std::to_string(MaxParamSize);
      pCtx->EmitError(ErrorMsg.c_str(), &F);
      continue;
    }

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

      if (ArgDesc.Arg->hasByValAttr()) {
        rewriteByValArg(F, *ArgDesc.Arg, DL);
      } else if (ArgDesc.Arg->getType()->isStructTy()) {
        rewriteStructValueArg(F, *ArgDesc.Arg, DL);
      } else {
        pullArgument(F, *ArgDesc.Arg, DL);
      }
      Changed = true;
    }
  }

  return Changed;
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses PullKernelArgumentsNPM::run(Module &M, ModuleAnalysisManager &AM) {
  auto &MDU = AM.getResult<MetaDataUtilsAnalysis>(M);
  CodeGenContext *pCtx = AM.getResult<CodeGenContextAnalysis>(M).Ctx;
  bool Changed = PullKernelArguments().run(M, MDU.MdUtils, pCtx);
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
