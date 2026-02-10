/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"

#include <string>
#include <queue>

#define VISIT_FUNCTION_IMPL(InstructionType)                                                                           \
  void visit##InstructionType(llvm::InstructionType &instruction) {                                                    \
    changed |= !wasPromoted(instruction.getFunction()) && getOrCreatePromotedValue(&instruction) != &instruction;      \
  }

namespace IGC {
class PromoteSubByte : public llvm::ModulePass, public llvm::InstVisitor<PromoteSubByte> {
public:
  static char ID;

  PromoteSubByte();
  ~PromoteSubByte() {}

  virtual llvm::StringRef getPassName() const override { return "PromoteSubByte"; }

  virtual bool runOnModule(llvm::Module &module) override;

  VISIT_FUNCTION_IMPL(AllocaInst)
  VISIT_FUNCTION_IMPL(CallInst)
  VISIT_FUNCTION_IMPL(GetElementPtrInst)
  VISIT_FUNCTION_IMPL(LoadInst)
  VISIT_FUNCTION_IMPL(StoreInst)
  VISIT_FUNCTION_IMPL(ICmpInst)
  VISIT_FUNCTION_IMPL(SExtInst)
  VISIT_FUNCTION_IMPL(ZExtInst)

private:
  bool changed;
  llvm::Module *M = nullptr;

  llvm::Value *convertI1ToI8(llvm::Value *argument, llvm::IRBuilder<> &builder);
  llvm::Value *convertI1ToI8(llvm::Value *argument, llvm::Instruction *insertBefore);
  llvm::Value *convertI8ToI1(llvm::Value *argument, llvm::IRBuilder<> &builder);
  llvm::Value *convertI8ToI1(llvm::Value *argument, llvm::Instruction *insertBefore);
  llvm::Value *castTo(llvm::Value *value, llvm::Type *desiredType, llvm::IRBuilder<> &builder);
  llvm::Value *castTo(llvm::Value *value, llvm::Type *desiredType, llvm::Instruction *insertBefore);
  llvm::Value *castAggregate(llvm::Value *value, llvm::Type *desiredType, llvm::IRBuilder<> &builder);

  void cleanUp(llvm::Module &module);

  // Checking if type needs promotion
  bool typeNeedsPromotion(llvm::Type *type, llvm::DenseSet<llvm::Type *> visitedTypes = {});

  // Promoting types
  llvm::DenseMap<llvm::Type *, llvm::Type *> promotedTypesCache;
  llvm::Type *getOrCreatePromotedType(llvm::Type *type);

  // Promoting values
  llvm::DenseMap<llvm::Value *, llvm::Value *> promotedValuesCache;
  std::queue<llvm::Value *> promotionQueue;
  bool wasPromoted(llvm::Value *value);

  template <typename Range> bool wasPromotedAnyOf(const Range &range) {
    return std::any_of(std::begin(range), std::end(range), [this](const auto &item) { return wasPromoted(item); });
  }

  template <typename T> void setPromotedAttributes(T *callOrFunc, const llvm::AttributeList &attributeList);

  llvm::Value *getOrCreatePromotedValue(llvm::Value *value);
  llvm::Function *promoteFunction(llvm::Function *function);
  llvm::GlobalVariable *promoteGlobalVariable(llvm::GlobalVariable *globalVariable);
  llvm::Constant *promoteConstant(llvm::Constant *constant);
  llvm::AllocaInst *promoteAlloca(llvm::AllocaInst *alloca);
  llvm::AddrSpaceCastInst *promoteAddrSpaceCast(llvm::AddrSpaceCastInst *addrSpaceCast);
  llvm::Value *promoteBitCast(llvm::BitCastInst *bitcast);
  llvm::CallInst *promoteCall(llvm::CallInst *call);
  llvm::CallInst *promoteIndirectCallOrInlineAsm(llvm::CallInst *call);
  llvm::ExtractValueInst *promoteExtractValue(llvm::ExtractValueInst *extractValue);
  llvm::GetElementPtrInst *promoteGetElementPtr(llvm::GetElementPtrInst *getElementPtr);
  llvm::Value *promoteICmp(llvm::ICmpInst *icmp);
  llvm::InlineAsm *promoteInlineAsm(llvm::InlineAsm *inlineAsm);
  llvm::InsertValueInst *promoteInsertValue(llvm::InsertValueInst *insertValue);
  llvm::LoadInst *promoteLoad(llvm::LoadInst *load);
  llvm::PHINode *promotePHI(llvm::PHINode *phi);
  llvm::StoreInst *promoteStore(llvm::StoreInst *store);
  llvm::IntToPtrInst *promoteIntToPtr(llvm::IntToPtrInst *inttoptr);
  llvm::ExtractElementInst *promoteExtractElement(llvm::ExtractElementInst *extractElement);
  llvm::Value *promoteInsertElement(llvm::InsertElementInst *insertElement);
  llvm::Value *promoteShuffleVector(llvm::ShuffleVectorInst *shuffleVector);
  llvm::Value *promoteTrunc(llvm::TruncInst *trunc);
  llvm::Value *promoteZExt(llvm::ZExtInst *zext);
  llvm::Value *promoteSExt(llvm::SExtInst *sext);
  llvm::Type *ReplaceScalarTypeWithI8(llvm::Type *type);
  llvm::Value *promoteAndUnpackInt4Vector(llvm::Value *unpromotedInput, llvm::IRBuilder<> &builder,
                                          bool signExtend = false);
  llvm::Value *packInt4Vector(llvm::Value *input, llvm::IRBuilder<> &builder);
  llvm::Constant *unpackConstantInt4Vector(llvm::Value *input, llvm::Type *outputTy, bool signExtend);
  llvm::Constant *packConstantInt4Vector(llvm::Value *input);

  // Promoting values - helping vars
  llvm::DenseSet<llvm::PHINode *> visitedPHINodes;
};
} // namespace IGC
