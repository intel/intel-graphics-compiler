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
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

namespace IGC {

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class SpvSubgroupMMAResolution final : public llvm::InstVisitor<SpvSubgroupMMAResolution> {
public:
  enum ElType {
    Unknown = 0,
    I32,
    I16,
    F32,
    F16,
  };

  SpvSubgroupMMAResolution() {}
  ~SpvSubgroupMMAResolution() = default;

  static llvm::StringRef getPassName() { return "SpvSubgroupMMAResolution"; }

  bool run(llvm::Module &M, CodeGenContext *pCtx, IGCMD::MetaDataUtils *pMdUtils);
  void visitCallInst(llvm::CallInst &CI);

private:
  using OperandsTable = std::unordered_map<int, llvm::StringRef>;
  using BTable = std::unordered_map<ElType, OperandsTable>;
  using ATable = std::unordered_map<ElType, BTable>;
  using RTable = std::unordered_map<ElType, ATable>;
  using SupportedTable = std::unordered_map<uint32_t, RTable>;

  static void populateSimd8Table();
  static void populateSimd16Table();
  static void populateSimd16ScaledTable();
  SupportedTable *getSupportedTable();

  void emitError(const llvm::Twine &message, const llvm::CallInst &CI);

  ElType getElType(const llvm::Type *Ty) const;
  llvm::StringRef getElTypeStr(const ElType Ty) const;
  ElType getValidMatrixType(const llvm::Type *Ty) const;
  int getElemCount(const llvm::Type *Ty) const;
  template <typename TableType> std::string getValidTypesStr(const TableType &table) const;

  bool validateI32Constant(const llvm::Value *V, const llvm::Twine &ParamName, llvm::StringRef BuiltinName,
                           const llvm::CallInst &CI);
  bool validateCType(const llvm::Type *ResultTy, const llvm::Type *CType, llvm::StringRef BuiltinName,
                     const llvm::CallInst &CI);
  bool validateElementType(const ElType ElemTy, llvm::StringRef ParamName, llvm::StringRef BuiltinName,
                           const llvm::CallInst &CI);
  bool validateDpasElemCounts(int M, int AElemCount, int BElemCount, uint32_t Operands, llvm::CallInst &CI);
  bool validateBdpasElemCounts(int M, int AElemCount, int BElemCount, const llvm::CallInst &CI);
  bool validateScaleType(const llvm::Value *Scale, llvm::StringRef ParamName, int K, const llvm::CallInst &CI);

  template <typename T>
  bool validateKDimInTable(const T KIt, int K, const SupportedTable *table, llvm::StringRef BuiltinName,
                           const llvm::CallInst &CI);
  template <typename T>
  bool validateResultElementInTable(const T RIt, int K, ElType ResultElemTy, const RTable &table,
                                    llvm::StringRef BuiltinName, const llvm::CallInst &CI);
  template <typename T>
  bool validateAElementInTable(const T AIt, int K, ElType ResultElemTy, ElType AElemTy, const ATable &table,
                               llvm::StringRef BuiltinName, const llvm::CallInst &CI);
  template <typename T>
  bool validateBElementInTable(const T BIt, int K, ElType ResultElemTy, ElType AElemTy, ElType BElemTy,
                               const BTable &table, llvm::StringRef BuiltinName, const llvm::CallInst &CI);
  template <typename T>
  bool validateOperands(const T OpIt, int K, ElType ResultElemTy, ElType AElemTy, ElType BElemTy, uint32_t Operands,
                        const OperandsTable &operandMap, llvm::StringRef BuiltinName, const llvm::CallInst &CI);

  bool isDoubleSubgroup(llvm::CallInst &CI);

  void lowerToDpasBuiltin(llvm::CallInst &CI, llvm::Function *F);
  void lowerToBdpasBuiltin(llvm::CallInst &CI, llvm::Function *F);

  llvm::DenseSet<llvm::Function *> m_BuiltinsToRemove;
  bool m_Changed = false;
  IGC::CodeGenContext *m_Ctx = nullptr;
  IGC::IGCMD::MetaDataUtils *m_pMdUtils = nullptr;
  llvm::Module *m_Module = nullptr;
  static SupportedTable m_Simd8Table;
  static SupportedTable m_Simd16Table;
  static SupportedTable m_Simd16ScaledTable;
};

// Legacy Pass Manager wrapper.
class SpvSubgroupMMAResolutionLPM final : public llvm::ModulePass {
public:
  static char ID;

  SpvSubgroupMMAResolutionLPM();
  ~SpvSubgroupMMAResolutionLPM() override = default;

  llvm::StringRef getPassName() const override { return SpvSubgroupMMAResolution::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  bool runOnModule(llvm::Module &M) override {
    return SpvSubgroupMMAResolution().run(M, getAnalysis<CodeGenContextWrapper>().getCodeGenContext(),
                                          getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class SpvSubgroupMMAResolutionNPM : public llvm::PassInfoMixin<SpvSubgroupMMAResolutionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-spv-subgroup-mma-resolution"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
