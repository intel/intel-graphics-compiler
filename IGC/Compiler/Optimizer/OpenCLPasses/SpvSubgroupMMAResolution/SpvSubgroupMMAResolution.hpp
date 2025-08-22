/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <unordered_map>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

namespace IGC {

class SpvSubgroupMMAResolution final : public llvm::ModulePass, public llvm::InstVisitor<SpvSubgroupMMAResolution> {
public:
  enum ElType {
    Unknown = 0,
    I32,
    I16,
    F32,
    F16,
  };

  static char ID;

  SpvSubgroupMMAResolution();
  ~SpvSubgroupMMAResolution() override = default;

  llvm::StringRef getPassName() const override { return "SpvSubgroupMMAResolution"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  bool runOnModule(llvm::Module &M) override;
  void visitCallInst(llvm::CallInst &CI);

private:
  using OperandsTable = std::unordered_map<int, llvm::StringRef>;
  using BTable = std::unordered_map<ElType, OperandsTable>;
  using ATable = std::unordered_map<ElType, BTable>;
  using RTable = std::unordered_map<ElType, ATable>;
  using SupportedTable = std::unordered_map<uint32_t, RTable>;

  static void populateSimd8Table();
  static void populateSimd16Table();
  SupportedTable *getSupportedTable();

  void emitError(const llvm::Twine &message, const llvm::CallInst &CI);

  ElType getElType(const llvm::Type *Ty) const;
  llvm::StringRef getElTypeStr(const ElType Ty) const;
  ElType getValidMatrixType(const llvm::Type *Ty) const;
  int getElemCount(const llvm::Type *Ty) const;
  template <typename TableType> std::string getValidTypesStr(const TableType &table) const;

  bool validateI32Constant(const llvm::Value *V, const llvm::Twine &ParamName, const llvm::CallInst &CI);
  bool validateCType(const llvm::Type *ResultTy, const llvm::Type *CType, const llvm::CallInst &CI);
  bool validateElementType(const ElType ElemTy, llvm::StringRef ParamName, const llvm::CallInst &CI);
  bool validateElemCounts(int M, int AElemCount, int BElemCount, uint32_t Operands, llvm::CallInst &CI);

  template <typename T>
  bool validateKDimInTable(const T KIt, int K, const SupportedTable *table, const llvm::CallInst &CI);
  template <typename T>
  bool validateResultElementInTable(const T RIt, int K, ElType ResultElemTy, const RTable &table,
                                    const llvm::CallInst &CI);
  template <typename T>
  bool validateAElementInTable(const T AIt, int K, ElType ResultElemTy, ElType AElemTy, const ATable &table,
                               const llvm::CallInst &CI);
  template <typename T>
  bool validateBElementInTable(const T BIt, int K, ElType ResultElemTy, ElType AElemTy, ElType BElemTy,
                               const BTable &table, const llvm::CallInst &CI);
  template <typename T>
  bool validateOperands(const T OpIt, int K, ElType ResultElemTy, ElType AElemTy, ElType BElemTy, uint32_t Operands,
                        const OperandsTable &operandMap, const llvm::CallInst &CI);

  bool isDoubleSubgroup(llvm::CallInst &CI);

  llvm::DenseSet<llvm::Function *> m_BuiltinsToRemove;
  bool m_Changed = false;
  IGC::CodeGenContext *m_Ctx = nullptr;
  llvm::Module *m_Module = nullptr;
  static SupportedTable m_Simd8Table;
  static SupportedTable m_Simd16Table;
};
} // namespace IGC
