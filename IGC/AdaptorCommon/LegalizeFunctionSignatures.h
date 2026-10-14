/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/IRBuilder.h>

#include <map>

namespace IGC {

enum class ArgLowering {
  Keep,
  // Int or int vector of an illegal width, widened to i8, i16, i32 or i64.
  WidenInt,
  // Promotable "byval" struct pointer, passed as the struct value.
  ByValToValue,
  // Struct or array value that is not passed in registers, passed as a "byval" pointer.
  ValueToByVal,
  // Promotable "sret" struct pointer, removed. See RetLowering::SRetToValue.
  SRet,
};

enum class RetLowering {
  Keep,
  // Int or int vector of an illegal width, widened to i8, i16, i32 or i64.
  WidenInt,
  // Promotable "sret" struct pointer argument, returned as the struct value.
  SRetToValue,
  // Value that is not returned in registers, returned through a new "sret" pointer argument.
  ValueToSRet,
  // Small struct returned by value. The signature is kept, the returns are rebuilt.
  StructValue,
};

// How one signature is lowered to the IGC function call ABI. The lowering computed for a
// function drives its new declaration, its new body and all direct calls to it.
struct SignatureLowering {
  RetLowering Ret = RetLowering::Keep;
  // One per original argument.
  llvm::SmallVector<ArgLowering, 8> Args;
  llvm::FunctionType *LoweredTy = nullptr;

  bool isNeeded() const {
    return Ret != RetLowering::Keep || llvm::any_of(Args, [](ArgLowering A) { return A != ArgLowering::Keep; });
  }
};

struct LoweredFunction {
  llvm::Function *NewFunc = nullptr;
  SignatureLowering Lowering;
};

} // namespace IGC

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class LegalizeFunctionSignatures {
public:
  LegalizeFunctionSignatures() {}
  ~LegalizeFunctionSignatures() {}

  static llvm::StringRef getPassName() { return "LegalizeFunctionSignatures"; }

  bool run(llvm::Module &M, IGC::IGCMD::MetaDataUtils *MDUtils, IGC::CodeGenContext *Ctx);

  void fixFunctionSignatures(llvm::Module &M);
  void fixFunctionBodies(llvm::Module &M);
  void fixFunctionUsers(llvm::Module &M);
  void fixCallInstruction(llvm::Module &M, llvm::CallInst *CI);
  void copyAttributesAndAdjustForSkippedFunctionArgs(llvm::Function *F, llvm::Function *NewF, bool HasPromotedSRetArg);

private:
  llvm::MapVector<llvm::Function *, IGC::LoweredFunction> OldToNewFuncMap;

  IGC::IGCMD::MetaDataUtils *MDUtils = nullptr;
  IGC::CodeGenContext *Ctx = nullptr;
};

// Legacy Pass Manager wrapper.
class LegalizeFunctionSignaturesLPM : public llvm::ModulePass {
public:
  static char ID;

  LegalizeFunctionSignaturesLPM();

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.addRequired<IGC::CodeGenContextWrapper>();
    AU.addRequired<IGC::MetaDataUtilsWrapper>();
  }

  virtual bool runOnModule(llvm::Module &M) {
    return Impl.run(M, getAnalysis<IGC::MetaDataUtilsWrapper>().getMetaDataUtils(),
                    getAnalysis<IGC::CodeGenContextWrapper>().getCodeGenContext());
  }

  virtual llvm::StringRef getPassName() const { return LegalizeFunctionSignatures::getPassName(); }

private:
  LegalizeFunctionSignatures Impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class LegalizeFunctionSignaturesNPM : public llvm::PassInfoMixin<LegalizeFunctionSignaturesNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-legalize-function-signatures"; }
  static bool isRequired() { return true; }

private:
  LegalizeFunctionSignatures Impl;
};
#endif // LLVM_VERSION_MAJOR >= 16
