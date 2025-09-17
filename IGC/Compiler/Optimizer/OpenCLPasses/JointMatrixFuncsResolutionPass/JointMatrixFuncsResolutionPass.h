/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
struct JointMatrixTypeDescription;

class JointMatrixFuncsResolutionPass final : public llvm::ModulePass,
                                             public llvm::InstVisitor<JointMatrixFuncsResolutionPass> {
public:
  static char ID;

  JointMatrixFuncsResolutionPass();
  ~JointMatrixFuncsResolutionPass() {}

  virtual llvm::StringRef getPassName() const override { return "JointMatrixFuncsResolutionPass"; }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<IGC::MetaDataUtilsWrapper>();
    AU.addRequired<IGC::CodeGenContextWrapper>();
  }

  virtual bool runOnModule(llvm::Module &M) override;
  bool runOnFunction(llvm::Function &F);
  void visitCallInst(llvm::CallInst &CI);
  void visitAllocaInst(llvm::AllocaInst &I);
  void visitGetElementPtrInst(llvm::GetElementPtrInst &I);
  void visitPtrToIntInst(llvm::PtrToIntInst &I);
  void visitStoreInst(llvm::StoreInst &I);
  void visitBitCastInst(llvm::BitCastInst &I);
  void visitAddrSpaceCastInst(llvm::AddrSpaceCastInst &I);
  void visitLoadInst(llvm::LoadInst &I);
  void visitPHINode(llvm::PHINode &I);
  void visitReturnInst(llvm::ReturnInst &I);

private:
  bool ResolveFunction(llvm::Function *OriginalFunction);
  bool ResolveCallFuncWithMatrixArgs(llvm::Function *ResolvedFunction, llvm::CallInst *CI);
  llvm::Instruction *ResolvePrefetch(llvm::CallInst *CI);
  template <bool IsJointMatrix, bool isChecked> llvm::Instruction *ResolveLoad(llvm::CallInst *CI);
  template <bool IsJointMatrix, bool IsChecked> llvm::Instruction *ResolveStore(llvm::CallInst *CI);
  llvm::Instruction *ResolveMad(llvm::CallInst *CI, unsigned OperationType);
  int getSliceSize(const JointMatrixTypeDescription *desc);
  llvm::Value *ResolveFill(llvm::CallInst *CI);
  llvm::Instruction *ResolveFillChecked(llvm::CallInst *CI);
  llvm::Value *ResolveWILength(llvm::CallInst *CI);
  llvm::Value *getAcc2x64ElementPtr(llvm::CallInst *CI, llvm::Value *matrix, llvm::Value *index,
                                    llvm::IRBuilder<> *builder, llvm::Value **MatPtr,
                                    const JointMatrixTypeDescription &desc);
  llvm::Value *ResolveSliceInsert(llvm::CallInst *CI);
  llvm::Value *ResolveSliceExtract(llvm::CallInst *CI);
  llvm::Instruction *ResolveGetCoord(llvm::CallInst *CI);
  llvm::Value *ResolveCall(llvm::CallInst *CI);
  llvm::Value *ResolveGeneric(llvm::Instruction *OldInst);
  void clearFunctionCache();

  // Main recursive method used across pass to resolve Joint Matrix-related values
  llvm::Value *Resolve(llvm::Value *value);
  bool preprocessAccessChain(llvm::Function *F);

  bool parseMatrixTypeNameLegacy(const llvm::Type *opaqueType, JointMatrixTypeDescription *outDescription);
  bool ParseMatrixTypeName(llvm::Type *opaqueType, JointMatrixTypeDescription *outDescription);
  bool ParseMatrixTypeNameNonExtTypeDetails(llvm::Type *opaqueType, llvm::StringRef name, bool IsJointMatrix,
                                            JointMatrixTypeDescription *outDescription);
#if LLVM_VERSION_MAJOR >= 16
  llvm::Type *TryFindTargetExtensionTypeOfOpaquePtr(llvm::Value *V);
  llvm::Type *TryFindTypeOfOpaquePtr(llvm::Value *V);
  bool ParseMatrixTypeNameExtTypeDetails(llvm::Type *opaqueType, bool IsJointMatrix,
                                         IGC::JointMatrixTypeDescription *outDescription);
#endif

  void RecursiveSearchAndFixCanonicalizdGEPandLifetime(std::unordered_set<llvm::Value *> &visited,
                                                       const llvm::DataLayout &DL, llvm::Value *rootValue,
                                                       uint64_t matrixTypeAllocSize, uint64_t totalAllocationSize);
  llvm::StringRef GetMatrixTypeName(llvm::Type *opaqueType);
  bool SetLayoutFromUse(IGC::JointMatrixTypeDescription *outDescription);
  unsigned GetUseFromLegacyLayout(unsigned int legacyLayout);

  unsigned getNumRowsPerWI(const JointMatrixTypeDescription *desc);
  llvm::Type *ResolveType(llvm::Type *opaqueType, JointMatrixTypeDescription *outDesc);
  llvm::Type *ResolveTypes(llvm::Type *t);
  llvm::Type *ResolveStructType(llvm::Type *t);
  llvm::Type *ResolveArrayType(llvm::Type *t);
  llvm::Type *ResolvePointerType(llvm::Type *t);
  void CacheResolvedValue(llvm::Value *oldValue, llvm::Value *newValue);
  void CacheResolvedTypes(llvm::Type *oldType, llvm::Type *newType);

  // Inserts undef placeholder with the correct resolved type
  // to unblock recursive resolving of dependencies
  void InsertPlaceholder(llvm::Value *v);
  llvm::Function *CloneFunction(llvm::Function *pOriginalFunction);

  enum GetMatrixFuncNameOperation {
    GetCoord,
    Prefetch,
    Load,
    Store,
    LoadChecked,
    StoreChecked,
  };
  std::string GetMatrixFuncName(GetMatrixFuncNameOperation operation, unsigned operationLayout, unsigned address_space,
                                const JointMatrixTypeDescription *desc, const std::string &prefix);

  bool ValidateIntegerBitWidth(unsigned int width);
  bool ValidateLoadStore(bool isLoad, unsigned operationLayout, const JointMatrixTypeDescription *desc,
                         llvm::Value *ctx);

  void Validate2DBlockLoadStore(GetMatrixFuncNameOperation operation, unsigned operationLayout, unsigned address_space,
                                const JointMatrixTypeDescription *desc, llvm::Value *ctx);

  // SIMD Size helpers
  llvm::Function *getEntryFunction(llvm::Function *F);
  void ResolveSIMDSize(llvm::Function *F);
  int32_t DetermineForcedSIMDSize();
  int32_t DefineKernelSIMDSize();
  bool IsSIMDSizeValid(int32_t simdSize);
  void ForceKernelSIMDSize(llvm::Function *F, int32_t forcedSIMDSize);

  llvm::ValueMap<llvm::Value *, llvm::Instruction *> PlaceholderInstructions;
  llvm::ValueMap<llvm::Value *, llvm::Value *> ResolvedValues;
  llvm::ValueMap<llvm::Value *, llvm::Value *> MatrixAllocas;
  std::unordered_map<llvm::Type *, llvm::Type *> ResolvedTypes;
  llvm::SmallPtrSet<llvm::Instruction *, 8> InstsToErase;
  // Maps function to it's kernel entry function
  std::unordered_map<llvm::Function *, llvm::Function *> FunctionEntryMap;
  // Maps function with old signature to function with new signature
  std::unordered_map<llvm::Function *, llvm::Function *> ResolvedFuncSignatures;
  // Keeps track of new functions with new signature
  std::unordered_set<llvm::Function *> NewFuncWithResolvedSignatures;

  ModuleMetaData *MMD = nullptr;
  CodeGenContext *m_Ctx = nullptr;
  IGCMD::MetaDataUtils *m_mdUtils = nullptr;
  bool Changed = false;
  int32_t m_SIMDSize = 0;
};
}; // namespace IGC
