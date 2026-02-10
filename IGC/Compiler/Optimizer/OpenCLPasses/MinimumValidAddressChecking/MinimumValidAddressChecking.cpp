/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MinimumValidAddressChecking.hpp"

#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Mangler.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Regex.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-minimum-valid-address-checking"
#define PASS_DESCRIPTION "Minimum valid address checking"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(MinimumValidAddressChecking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(MinimumValidAddressChecking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char MinimumValidAddressChecking::ID = 0;

static cl::opt<unsigned long long> MinimumValidAddressCheckingArg("igc-minimum-valid-address-checking_arg",
                                                                  cl::desc("Set minimum valid address (default 0)"),
                                                                  cl::init(0), cl::Hidden);

MinimumValidAddressChecking::MinimumValidAddressChecking(uint64_t minimumValidAddress) : ModulePass(ID) {
  this->minimumValidAddress = std::max(uint64_t(MinimumValidAddressCheckingArg), minimumValidAddress);
  initializeMinimumValidAddressCheckingPass(*PassRegistry::getPassRegistry());
}

bool MinimumValidAddressChecking::runOnModule(Module &M) {
  // No reason to run the pass because all accesses are valid.
  if (minimumValidAddress == 0) {
    return false;
  }

  bool modified = false;

  compileUnit = nullptr;
  if (M.debug_compile_units().begin() != M.debug_compile_units().end()) {
    compileUnit = *M.debug_compile_units().begin();
  }

  for (auto &function : M.functions()) {
    // Collect loads/stores
    loadsAndStoresToCheck.clear();
    visit(function);

    // Handle loads/stores
    for (auto &instruction : loadsAndStoresToCheck) {
      handleLoadStore(instruction);
    }

    modified |= !loadsAndStoresToCheck.empty();
  }

  return modified;
}

void MinimumValidAddressChecking::visitLoadInst(LoadInst &load) {
  if (load.getPointerOperandType()->getPointerAddressSpace() == ADDRESS_SPACE_GLOBAL ||
      load.getPointerOperandType()->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC) {
    loadsAndStoresToCheck.push_back(&load);
  }
}

void MinimumValidAddressChecking::visitStoreInst(StoreInst &store) {
  if (store.getPointerOperandType()->getPointerAddressSpace() == ADDRESS_SPACE_GLOBAL ||
      store.getPointerOperandType()->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC) {
    loadsAndStoresToCheck.push_back(&store);
  }
}

void MinimumValidAddressChecking::handleLoadStore(Instruction *instruction) {
  Value *pointer = nullptr;
  if (auto load = dyn_cast<LoadInst>(instruction)) {
    pointer = load->getPointerOperand();
  } else if (auto store = dyn_cast<StoreInst>(instruction)) {
    pointer = store->getPointerOperand();
  } else {
    IGC_ASSERT(0);
  }

  auto addressSpace = pointer->getType()->getPointerAddressSpace();

  // Address is valid if it is greater or equal to the minimum valid address
  // and the address space is global or generic=global.
  IRBuilder<> builder(instruction);
  auto address = builder.CreatePtrToInt(pointer, builder.getInt64Ty());
  Value *isValid = nullptr;

  if (addressSpace == ADDRESS_SPACE_GLOBAL) {
    isValid = builder.CreateICmpUGE(address, builder.getInt64(minimumValidAddress));
  } else if (addressSpace == ADDRESS_SPACE_GENERIC) {
    // Check the pointer's address space tag.
    auto tag = builder.CreateLShr(address, builder.getInt64(61));
    auto tagIsZero = builder.CreateICmpEQ(tag, builder.getInt64(0b000));
    auto tagIsSeven = builder.CreateICmpEQ(tag, builder.getInt64(0b111));
    auto isGlobalAddressSpace = builder.CreateOr(tagIsZero, tagIsSeven);

    // Remove the address space tag.
    address = builder.CreateShl(address, 4);
    address = builder.CreateAShr(address, 4);

    isValid = builder.CreateOr(builder.CreateNot(isGlobalAddressSpace),
                               builder.CreateICmpUGE(address, builder.getInt64(minimumValidAddress)));
  }

  // Generate if-then-else
  Instruction *thenTerminator = nullptr;
  Instruction *elseTerminator = nullptr;
  SplitBlockAndInsertIfThenElse(isValid, instruction, &thenTerminator, &elseTerminator);

  // Merge block
  auto mergeBlock = instruction->getParent();
  mergeBlock->setName("minimumvalidaddresschecking.end");

  // Valid offset
  auto thenBlock = thenTerminator->getParent();
  thenBlock->setName("minimumvalidaddresschecking.valid");
  instruction->moveBefore(thenTerminator);

  // Invalid offset
  auto elseBlock = elseTerminator->getParent();
  elseBlock->setName("minimumvalidaddresschecking.invalid");
  createAssertCall(address, instruction, elseTerminator);
  auto replacement = createLoadStoreReplacement(instruction, elseTerminator);

  // PhiNode
  if (isa<LoadInst>(instruction)) {
    PHINode *phi = PHINode::Create(instruction->getType(), 2, "", &mergeBlock->front());
    instruction->replaceUsesOutsideBlock(phi, thenBlock);
    phi->addIncoming(instruction, thenBlock);
    phi->addIncoming(replacement, elseBlock);
  }
}

Value *MinimumValidAddressChecking::createLoadStoreReplacement(Instruction *instruction, Instruction *insertBefore) {
  if (isa<LoadInst>(instruction)) {
    return Constant::getNullValue(instruction->getType());
  } else if (auto store = dyn_cast<StoreInst>(instruction)) {
    return new StoreInst(store->getValueOperand(),
                         ConstantPointerNull::get(dyn_cast<PointerType>(store->getPointerOperandType())), insertBefore);
  } else {
    IGC_ASSERT(0);
    return nullptr;
  }
}

void MinimumValidAddressChecking::createAssertCall(Value *address, Instruction *instruction,
                                                   Instruction *insertBefore) {
  auto M = insertBefore->getModule();

  auto assertArgs = createAssertArgs(address, instruction, insertBefore);
  auto assertArgsTypes = SmallVector<Type *, 4>{};
  std::transform(assertArgs.begin(), assertArgs.end(), std::back_inserter(assertArgsTypes),
                 [](Value *value) { return value->getType(); });

  auto assert =
      cast<Function>(M->getOrInsertFunction(compileUnit ? "__minimumvalidaddresschecking_assert"
                                                        : "__minimumvalidaddresschecking_assert_nodebug",
                                            FunctionType::get(Type::getVoidTy(M->getContext()), assertArgsTypes, false))
                         .getCallee());

  auto call = CallInst::Create(assert, assertArgs, "", insertBefore);
  call->setCallingConv(CallingConv::SPIR_FUNC);
}

SmallVector<Value *, 4> MinimumValidAddressChecking::createAssertArgs(Value *address, Instruction *instruction,
                                                                      Instruction *insertBefore) {
  auto createGEP = [insertBefore](GlobalVariable *globalVariable) {
    const auto zero = ConstantInt::getSigned(Type::getInt32Ty(globalVariable->getParent()->getContext()), 0);
    auto result =
        GetElementPtrInst::Create(globalVariable->getValueType(), globalVariable, {zero, zero}, "", insertBefore);
    result->setIsInBounds(true);
    return result;
  };

  SmallVector<Value *, 4> result;
  if (compileUnit) {
    auto debugLoc = instruction->getDebugLoc();

    result.push_back(createGEP(getOrCreateGlobalConstantString(insertBefore->getModule(), compileUnit->getFilename())));
    result.push_back(
        ConstantInt::getSigned(Type::getInt32Ty(insertBefore->getContext()), debugLoc ? debugLoc->getLine() : 0));
    result.push_back(
        ConstantInt::getSigned(Type::getInt32Ty(insertBefore->getContext()), debugLoc ? debugLoc->getColumn() : 0));
  }
  result.push_back(address);

  return result;
}

GlobalVariable *MinimumValidAddressChecking::getOrCreateGlobalConstantString(Module *M, StringRef str) {
  if (stringsCache.count(str) == 0) {
    stringsCache[str] =
        new GlobalVariable(*M, ArrayType::get(Type::getInt8Ty(M->getContext()), str.size() + 1), true,
                           GlobalValue::InternalLinkage, ConstantDataArray::getString(M->getContext(), str, true), "",
                           nullptr, GlobalValue::ThreadLocalMode::NotThreadLocal, ADDRESS_SPACE_CONSTANT);
    stringsCache[str]->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
  }

  return stringsCache[str];
}
