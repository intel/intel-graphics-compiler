/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PreprocessSPVIR.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Support/Regex.h>
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "common/BuiltinTypes.h"

#include <regex>
#include <unordered_set>

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-preprocess-spvir"
#define PASS_DESCRIPTION "Adjust SPV-IR produced by Khronos SPIRV-LLVM Translator to be consumable by IGC BiFModule"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PreprocessSPVIR, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(PreprocessSPVIR, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PreprocessSPVIR::ID = 0;

PreprocessSPVIR::PreprocessSPVIR() : ModulePass(ID) { initializePreprocessSPVIRPass(*PassRegistry::getPassRegistry()); }

void PreprocessSPVIR::createCallAndReplace(CallInst &oldCallInst, StringRef newFuncName, std::vector<Value *> &args) {
  Function *F = oldCallInst.getCalledFunction();
  IGC_ASSERT(F);

  std::vector<Type *> argTypes;
  for (auto arg : args)
    argTypes.push_back(arg->getType());

  FunctionType *FT = FunctionType::get(oldCallInst.getType(), argTypes, false);
  auto *newFunction = cast<Function>(m_Module->getOrInsertFunction(newFuncName, FT, F->getAttributes()));
  newFunction->setCallingConv(F->getCallingConv());
  CallInst *newCall = CallInst::Create(newFunction, args, "", &oldCallInst);
  newCall->setCallingConv(oldCallInst.getCallingConv());
  newCall->setAttributes(oldCallInst.getAttributes());
  oldCallInst.replaceAllUsesWith(newCall);
}

// Replace functions like:
//  i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)*)
//  i32 @_Z18__spirv_ocl_printfPU3AS2ci(i8 addrspace(2)*, i32)
// With:
//  i32 @printf(i8 addrspace(2)*, ...)
//
// Khronos SPV-IR represents printf function as a non-variadic one. Since
// IGC supports clang-consistent representation of printf (which is unmangled,
// variadic function), all printf calls must get replaced.
void PreprocessSPVIR::visitOpenCLEISPrintf(llvm::CallInst &CI) {
  FunctionType *FT = FunctionType::get(CI.getType(), Type::getInt8PtrTy(m_Module->getContext(), 2), true);
  Function *newPrintf = cast<Function>(m_Module->getOrInsertFunction("printf", FT));
  CI.setCalledFunction(newPrintf);

  m_changed = true;
}

bool PreprocessSPVIR::isSPVIR(StringRef funcName) {
  bool is_regular_pattern = Regex("_Z[0-9]+__spirv_[A-Z].*").match(funcName);
  bool is_eis_pattern = Regex("_Z[0-9]+__spirv_ocl_[a-z].*").match(funcName);

  return is_regular_pattern || is_eis_pattern;
}

bool PreprocessSPVIR::hasArrayArg(llvm::Function &F) {
  for (auto &Arg : F.args()) {
    if (Arg.getType()->isArrayTy())
      return true;
  }
  return false;
}

void PreprocessSPVIR::processBuiltinsWithArrayArguments(llvm::Function &F) {
  if (F.user_empty())
    return;

  IGC_ASSERT(F.hasName());
  StringRef origName = F.getName();

  // add postfix to original function name, since new function with original
  // name and different arguments types is going to be created
  F.setName(origName + ".old");

  std::unordered_set<CallInst *> callInstsToErase;
  for (auto *user : F.users()) {
    if (auto *CI = dyn_cast<CallInst>(user)) {
      std::vector<Value *> newArgs;
      for (auto &arg : CI->args()) {
        auto *T = arg->getType();
        if (!T->isArrayTy()) {
          // leave non-array arguments unchanged
          newArgs.push_back(arg);
          continue;
        }

        auto FBegin = CI->getFunction()->begin()->getFirstInsertionPt();
        auto *Alloca = new AllocaInst(T, 0, "", &(*FBegin));
        new StoreInst(arg, Alloca, false, CI);
        auto *Zero = ConstantInt::getNullValue(Type::getInt32Ty(T->getContext()));
        Value *Index[] = {Zero, Zero};
        auto *GEP = GetElementPtrInst::CreateInBounds(T, Alloca, Index, "", CI);
        newArgs.push_back(GEP);
      }

      createCallAndReplace(*CI, origName, newArgs);
      callInstsToErase.insert(CI);
    }
  }

  for (auto *CI : callInstsToErase)
    CI->eraseFromParent();

  F.eraseFromParent();
  m_changed = true;
}

void PreprocessSPVIR::processBuiltinsWithArrayArguments() {
  for (auto &F : make_early_inc_range(m_Module->functions())) {
    if (F.hasName() && F.isDeclaration()) {
      StringRef Name = F.getName();
      if (hasArrayArg(F) && isSPVIR(Name)) {
        if (Name.contains("BuildNDRange")) {
          processBuiltinsWithArrayArguments(F);
        }
      }
    }
  }
}

void PreprocessSPVIR::visitCallInst(CallInst &CI) {
  Function *F = CI.getCalledFunction();
  if (!F)
    return;

  StringRef Name = F->getName();
  if (!isSPVIR(Name))
    return;

  if (Name.contains("printf")) {
    visitOpenCLEISPrintf(CI);
  }
}

#if LLVM_VERSION_MAJOR >= 16
static void addNonKernelFuncsArgTypeHints(Module &M) {
  for (Function &F : M) {
    if (F.isDeclaration() || F.getCallingConv() != CallingConv::SPIR_FUNC)
      continue;

    SmallVector<Metadata *, 8> Hints;
    for (Argument &Arg : F.args()) {
      Type *ArgTy = Arg.getType();

      // Hints are needed only for SPIR-V builtin types.
      std::string HintValue = "";
      if (TargetExtType *TET = dyn_cast<TargetExtType>(ArgTy))
        HintValue = TET->getTargetExtName();

      Hints.push_back(MDString::get(M.getContext(), HintValue));
    }

    MDNode *Node = MDNode::get(M.getContext(), Hints);
    F.setMetadata("non_kernel_arg_type_hints", Node);
  }
}
#endif

void PreprocessSPVIR::removePointerAnnotations(Module &M) {
  for (Function &F : M) {
    if (F.isDeclaration())
      continue;

    for (auto &I : llvm::make_early_inc_range(llvm::instructions(F))) {
      auto *CI = llvm::dyn_cast<llvm::CallInst>(&I);
      if (!CI)
        continue;
      auto *Callee = CI->getCalledFunction();
      if (!Callee || !Callee->getName().startswith("llvm.ptr.annotation."))
        continue;

      // @llvm.ptr.annotation returns its first operand (the annotated pointer)
      // with annotation metadata attached. Replace all uses of the intrinsic
      // call with the original pointer and remove the call.
      Value *AnnotatedPtr = CI->getArgOperand(0);
      CI->replaceAllUsesWith(AnnotatedPtr);
      CI->eraseFromParent();
      m_changed = true;
    }
  }
}

static void fixKernelArgBaseTypes(Module &M) {
  LLVMContext &Ctx = M.getContext();

  for (Function &F : M) {
    if (F.isDeclaration())
      continue;

    MDNode *TyMD = F.getMetadata("kernel_arg_type");
    MDNode *BaseMD = F.getMetadata("kernel_arg_base_type");
    if (!TyMD)
      continue;

    if (!BaseMD) {
      // OpenCL base type node missing, copy all.
      F.setMetadata("kernel_arg_base_type", TyMD);
      continue;
    }

    unsigned N = TyMD->getNumOperands();
    if (BaseMD->getNumOperands() != N) {
      // Mismatched sizes.
      continue;
    }

    bool NeedPatch = false;
    SmallVector<Metadata *, 8> NewBase;
    NewBase.reserve(N);

    for (unsigned i = 0; i < N; ++i) {
      auto *TyStr = dyn_cast<MDString>(TyMD->getOperand(i));
      auto *BaseStr = dyn_cast<MDString>(BaseMD->getOperand(i));
      // Keep original if found non‑MDString.
      if (!TyStr || !BaseStr) {
        NewBase.push_back(BaseMD->getOperand(i));
        continue;
      }

      StringRef Ty = TyStr->getString();
      StringRef Base = BaseStr->getString();

      if (Ty.endswith("_t") && Ty != Base) {
        NeedPatch = true;
        NewBase.push_back(MDString::get(Ctx, Ty));
      } else {
        NewBase.push_back(BaseStr);
      }
    }

    if (NeedPatch) {
      MDNode *Patched = MDNode::get(Ctx, NewBase);
      F.setMetadata("kernel_arg_base_type", Patched);
    }
  }
}

bool PreprocessSPVIR::runOnModule(Module &M) {
  m_Module = static_cast<IGCLLVM::Module *>(&M);
  IRBuilder<> builder(M.getContext());
  m_Builder = &builder;

  // Change arguments with array type to pointer type to match signature
  // produced by Clang.
  processBuiltinsWithArrayArguments();

  visit(M);

  // Ensure that every OpenCL builtin type (*_t) listed in !kernel_arg_type is
  // also present in !kernel_arg_base_type at the same position. Some
  // frontends with partial TargetExtTy support emit pointer types in
  // !kernel_arg_base_type (instead of an actual OpenCL builtin type), this is
  // incorrect and inconsistent with the prior behavior.
  fixKernelArgBaseTypes(M);

  // Currently @llvm.ptr.annotation has no use in igc, but it can lead to
  // incorrectly generated code when used, since its not supported and
  // therefore not lowered.
  removePointerAnnotations(M);

#if LLVM_VERSION_MAJOR >= 16
  // Add SPIR-V builtin type hints as metadata to non-kernel functions. Kernel functions already have metadata type
  // hints added by the SPIR-V Reader. Type hints for non-kernel functions are not strictly required, but allow us to
  // apply certain optimizations (e.g. inlining) without doing costly type deduction. Adding type hints is easier before
  // retyping since we can rely on TargetExtTy information.
  addNonKernelFuncsArgTypeHints(M);
#endif

  // Retype function arguments of OpenCL types represented as TargetExtTy to
  // use opaque pointers instead. This is necessary to match function
  // signatures generated by Clang, given that it only has partial TargetExtTy
  // support.
#if LLVM_VERSION_MAJOR >= 16
  retypeOpenCLTargetExtTyAsPointers(&M);
#endif

  return m_changed;
}
