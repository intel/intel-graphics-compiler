/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/BfloatBuiltins/BfloatBuiltinsResolution.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Demangle/Demangle.h"
#include "common/LLVMWarningsPop.hpp"

#include <unordered_map>

using namespace llvm;
using namespace IGC;
char BfloatBuiltinsResolution::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-bfloat-builtins-resolution"
#define PASS_DESCRIPTION "BfloatBuiltinsResolution"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(BfloatBuiltinsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(BfloatBuiltinsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)


BfloatBuiltinsResolution::BfloatBuiltinsResolution(void) : FunctionPass(ID) {
  initializeBfloatBuiltinsResolutionPass(*PassRegistry::getPassRegistry());
}

// Array with supported function names
static const std::array FuncNames = {"__builtin_spirv_ClampConvertBF16ToE4M3INTEL",
                                     "__builtin_spirv_ClampConvertBF16ToE5M2INTEL",
                                     "__builtin_spirv_ClampStochasticRoundBF16ToE4M3INTEL",
                                     "__builtin_spirv_ClampStochasticRoundBF16ToE5M2INTEL",
                                     "__builtin_spirv_ConvertBF16ToE2M1INTEL",
                                     "__builtin_spirv_ConvertBF16ToE4M3INTEL",
                                     "__builtin_spirv_ConvertBF16ToE5M2INTEL",
                                     "__builtin_spirv_ConvertBF16ToInt4INTEL",
                                     "__builtin_spirv_ConvertE2M1ToBF16INTEL",
                                     "__builtin_spirv_ConvertE4M3ToBF16INTEL",
                                     "__builtin_spirv_ConvertE5M2ToBF16INTEL",
                                     "__builtin_spirv_ConvertInt4ToBF16INTEL",
                                     "__builtin_spirv_StochasticRoundBF16ToE2M1INTEL",
                                     "__builtin_spirv_StochasticRoundBF16ToE4M3INTEL",
                                     "__builtin_spirv_StochasticRoundBF16ToE5M2INTEL",
                                     "__builtin_spirv_StochasticRoundBF16ToInt4INTEL",
                                     "__spirv_FSigmoidINTEL"};

bool BfloatBuiltinsResolution::runOnFunction(Function &F) {
  visit(F);

  bool Changed = !CallInstPairs.empty();
  for (const auto &pair : CallInstPairs) {
    resolveCallInstPair(pair);
  }
  CallInstPairs.clear();

  return Changed;
}

// Helper function to replace all occurrences of a substring
static std::string replaceAll(StringRef input, StringRef search, StringRef replacement) {
  std::string result = input.str();
  size_t pos = 0;
  while ((pos = result.find(search.data(), pos, search.size())) != std::string::npos) {
    result.replace(pos, search.size(), replacement.data(), replacement.size());
    pos += replacement.size();
  }
  return result;
}

static bool callInstUsesBfloat(const CallInst &CI) {
  // Check if mangled name contains "DF16b"
  if (CI.getCalledFunction()) {
    StringRef MangledName = CI.getCalledFunction()->getName();
    if (MangledName.contains("DF16b")) {
      return true;
    }
  }

  // Check return type
  Type *RetTy = CI.getType();
  if (RetTy->isBFloatTy()) {
    return true;
  } else if (auto RetVecTy = dyn_cast<IGCLLVM::FixedVectorType>(RetTy)) {
    if (RetVecTy->getElementType()->isBFloatTy()) {
      return true;
    }
  }

  return false;
}

void BfloatBuiltinsResolution::visitCallInst(CallInst &CI) {
  if (!CI.getCalledFunction())
    return;

  if (!callInstUsesBfloat(CI))
    return;

  StringRef MangledName = CI.getCalledFunction()->getName();

  // The functions that we are about to resolve are in mangled form.
  // Quick check before demangling to save compilation time.
  {
    bool ContainsSupported = false;
    for (const auto &funcName : FuncNames) {
      if (MangledName.contains(funcName)) {
        ContainsSupported = true;
        break;
      }
    }

    if (!ContainsSupported)
      return;
  }

#if LLVM_VERSION_MAJOR < 20
  // Workaround for LLVM 16 demangler not supporting DF16b mangling (even though LLVM 16 supports bfloat type)
  // Support was implemented in LLVM 20
  // (https://github.com/llvm/llvm-project/commit/a100fd8cbd3dad3846a6212d97279ca23db85c75)
  std::string MangledNameForDemangling = MangledName.str();
  if (MangledName.contains("DF16b")) {
    MangledNameForDemangling = replaceAll(MangledName, "DF16b", "u6__bf16");
  }
#endif

  // Demangle function name
  std::string DemangledName = demangle(MangledNameForDemangling);
  size_t OpenParen = DemangledName.find('(');
  if (OpenParen != std::string::npos) {
    DemangledName = DemangledName.substr(0, OpenParen);
  }

  // Make sure that the demangled name fully matches
  // with a string from FuncNames map
  {
    StringRef DemangledNameRef = DemangledName;
    bool IsSupported = false;
    for (const auto &funcName : FuncNames) {
      if (DemangledNameRef.equals(funcName)) {
        IsSupported = true;
        break;
      }
    }

    if (!IsSupported)
      return;
  }

  std::string NewFuncName = replaceAll(MangledName, "DF16b", "s");
  CallInstPairs.push_back({&CI, NewFuncName});
}

static Value *bitcastBfloatToI16(IRBuilder<> *Builder, Value *V) {
  Type *VTy = V->getType();
  if (VTy->isBFloatTy()) {
    // Scalar bfloat to i16
    return Builder->CreateBitCast(V, Builder->getInt16Ty());
  } else if (auto VecArgTy = dyn_cast<IGCLLVM::FixedVectorType>(VTy)) {
    if (VecArgTy->getElementType()->isBFloatTy()) {
      // Vector of bfloat to vector of i16
      Type *NewVecType = IGCLLVM::FixedVectorType::get(Builder->getInt16Ty(), VecArgTy->getNumElements());
      return Builder->CreateBitCast(V, NewVecType);
    }
  }
  // No bitcast needed
  return V;
}

static Value *bitcastI16ToBfloat(IRBuilder<> *Builder, Value *V) {
  Type *VTy = V->getType();
  if (VTy->isIntegerTy(16)) {
    // Scalar i16 to bfloat
    return Builder->CreateBitCast(V, Builder->getBFloatTy());
  } else if (auto VecArgTy = dyn_cast<IGCLLVM::FixedVectorType>(VTy)) {
    if (VecArgTy->getElementType()->isIntegerTy(16)) {
      // Vector of i16 to vector of bfloat
      Type *NewVecType = IGCLLVM::FixedVectorType::get(Builder->getBFloatTy(), VecArgTy->getNumElements());
      return Builder->CreateBitCast(V, NewVecType);
    }
  }
  // No bitcast needed
  return V;
}

void BfloatBuiltinsResolution::resolveCallInstPair(const CallInstNamePair &pair) {
  IRBuilder<> Builder(pair.CI->getContext());
  Builder.SetInsertPoint(pair.CI);
  Module *M = Builder.GetInsertBlock()->getModule();

  // New function call arguments
  // Replace bfloat with i16 type
  SmallVector<Value *, 4> NewFuncArgs(pair.CI->arg_begin(), pair.CI->arg_end());
  for (size_t i = 0; i < NewFuncArgs.size(); i++) {
    NewFuncArgs[i] = bitcastBfloatToI16(&Builder, NewFuncArgs[i]);
  }

  // New function return type
  Type *NewFuncRetTy = pair.CI->getType();
  if (NewFuncRetTy->isBFloatTy()) {
    NewFuncRetTy = Builder.getInt16Ty();
  } else if (auto newFuncRetVecTy = dyn_cast<IGCLLVM::FixedVectorType>(NewFuncRetTy)) {
    if (newFuncRetVecTy->getElementType()->isBFloatTy()) {
      NewFuncRetTy = IGCLLVM::FixedVectorType::get(Builder.getInt16Ty(), newFuncRetVecTy->getNumElements());
    }
  }

  // New function argument types
  SmallVector<Type *, 4> NewFuncArgTypes;
  for (const Value *arg : NewFuncArgs) {
    NewFuncArgTypes.push_back(arg->getType());
  }

  // To avoid name collisions with functions that use bfloat on return type only
  // we rename the original function by appending ".old" to its name.
  if (Function *CIFunc = pair.CI->getCalledFunction()) {
    if (pair.NewName == CIFunc->getName()) {
      CIFunc->setName(CIFunc->getName() + ".old");
    }
  }

  FunctionType *NewFuncType = FunctionType::get(NewFuncRetTy, NewFuncArgTypes, false);
  FunctionCallee NewFunc = M->getOrInsertFunction(pair.NewName, NewFuncType);

  // Replace old function with a new function
  CallInst *NewCall = Builder.CreateCall(NewFunc, NewFuncArgs);
  NewCall->setDebugLoc(pair.CI->getDebugLoc());
  Value *NewCallValue = bitcastI16ToBfloat(&Builder, NewCall);

  pair.CI->replaceAllUsesWith(NewCallValue);
  pair.CI->eraseFromParent();
}
