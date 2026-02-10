/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "InjectPrintf.hpp"
#include <common/igc_regkeys.hpp>
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/IRBuilder.h"
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;
using namespace llvm;

char InjectPrintf::ID = 0;

#define PASS_FLAG "inject-printf"
#define PASS_DESCRIPTION "Inject printf before load and store operations for ptr Pass."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(InjectPrintf, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(InjectPrintf, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

enum class InjectPrintfFlagType {
#define INJECT_PRINTF_OPTION(Name, Val) Name = Val,
#include "igc_regkeys_enums_defs.h"
  INJECT_PRINTF_OPTIONS
#undef INJECT_PRINTF_OPTION
#undef INJECT_PRINTF_OPTIONS
};

InjectPrintf::InjectPrintf() : FunctionPass(ID) { initializeInjectPrintfPass(*PassRegistry::getPassRegistry()); }

GlobalVariable *InjectPrintf::createGlobalFormatStr(Module *Module, LLVMContext &Context) {
  IGC_ASSERT(Module != nullptr);

  const std::string FormatStrLiteral = "Pointer: %p, Type: %s\n";
  size_t FormatStrSize = FormatStrLiteral.length() + 1;

  GlobalVariable *FormatStrGlobal =
      new GlobalVariable(*Module, ArrayType::get(Type::getInt8Ty(Context), FormatStrSize), true,
                         GlobalValue::InternalLinkage, ConstantDataArray::getString(Context, FormatStrLiteral, true),
                         "", nullptr, GlobalValue::ThreadLocalMode::NotThreadLocal, 2);
  FormatStrGlobal->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);

  return FormatStrGlobal;
}

Value *InjectPrintf::createGEP(GlobalVariable *GlobalVariable, Instruction *InsertBefore) {
  IGC_ASSERT(GlobalVariable != nullptr);
  IGC_ASSERT(InsertBefore != nullptr);

  LLVMContext &Context = GlobalVariable->getParent()->getContext();
  const auto Zero = ConstantInt::getSigned(Type::getInt32Ty(Context), 0);
  auto Result =
      GetElementPtrInst::Create(GlobalVariable->getValueType(), GlobalVariable, {Zero, Zero}, "", InsertBefore);
  Result->setIsInBounds(true);
  return Result;
}

void InjectPrintf::insertPrintf(IRBuilder<> &Builder, FunctionCallee PrintfFunc, GlobalVariable *FormatStrGlobal,
                                Instruction *Inst, Value *PointerOperand, Type *ValueType) {
  IGC_ASSERT(FormatStrGlobal != nullptr);
  IGC_ASSERT(Inst != nullptr);
  IGC_ASSERT(PointerOperand != nullptr);
  IGC_ASSERT(ValueType != nullptr);

  Builder.SetInsertPoint(Inst);
  Value *FormatStr = createGEP(FormatStrGlobal, Inst);
  std::string TypeStr;
  raw_string_ostream RSO(TypeStr);
  ValueType->print(RSO);
  Value *TypeStrGlobal = Builder.CreateGlobalStringPtr(RSO.str());
  Builder.CreateCall(PrintfFunc, {FormatStr, PointerOperand, TypeStrGlobal});
}

bool InjectPrintf::runOnFunction(Function &F) {
  LLVMContext &Context = F.getContext();
  Module *Module = F.getParent();
  if (!Module) {
    errs() << "Error: Module is null\n";
    return false;
  }

  IRBuilder<> Builder(Context);
  GlobalVariable *FormatStrGlobal = createGlobalFormatStr(Module, Context);

  FunctionCallee PrintfFunc =
      Module->getOrInsertFunction("printf", FunctionType::get(IntegerType::getInt32Ty(Context),
                                                              PointerType::get(Type::getInt8Ty(Context), 2), true));

  auto InjectPrintfFlag = static_cast<InjectPrintfFlagType>(IGC_GET_FLAG_VALUE(InjectPrintfFlag));
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (auto *Load = dyn_cast<LoadInst>(&I)) {
        if (InjectPrintfFlag == InjectPrintfFlagType::InjectPrintfLoads ||
            InjectPrintfFlag == InjectPrintfFlagType::InjectPrintfLoadsAndStores) {
          insertPrintf(Builder, PrintfFunc, FormatStrGlobal, Load, Load->getPointerOperand(), Load->getType());
        }
      }
      if (auto *Store = dyn_cast<StoreInst>(&I)) {
        if (InjectPrintfFlag == InjectPrintfFlagType::InjectPrintfStores ||
            InjectPrintfFlag == InjectPrintfFlagType::InjectPrintfLoadsAndStores) {
          insertPrintf(Builder, PrintfFunc, FormatStrGlobal, Store, Store->getPointerOperand(),
                       Store->getValueOperand()->getType());
        }
      }
    }
  }

  return true;
}