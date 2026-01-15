/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/TranslationTable.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include <llvmWrapper/IR/IRBuilder.h>
#include <llvm/ADT/SmallBitVector.h>
#include <llvm/Transforms/Utils.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/IGCIRBuilder.h"

#include <map>

/// @brief ConstantCoalescing merges multiple constant loads into one load
/// of larger quantity
/// - change to oword loads if the address is uniform
/// - change to gather4 or sampler loads if the address is not uniform

using namespace llvm;

namespace IGC {

enum ExtensionKind {
  EK_NotExtended,
  EK_SignExt,
  EK_ZeroExt,
};

struct BufChunk {
  llvm::Value *bufIdxV;       // buffer index when it is indirect
  llvm::Value *baseIdxV;      // base-address index when it is indirect
  uint addrSpace;             // resource address space when it is direct
  uint elementSize;           // size in bytes of the basic data element
  uint chunkStart;            // offset of the first data element in chunk in units of elementSize
  uint chunkSize;             // chunk size in elements
  llvm::Instruction *chunkIO; // coalesced load
  uint loadOrder;             // direct CB used order.
  ExtensionKind extensionKind;
};

class ConstantCoalescing : public llvm::FunctionPass {
public:
  static char ID;
  ConstantCoalescing();

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addPreservedID(WIAnalysis::ID);
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<WIAnalysis>();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<TranslationTable>();
    AU.addPreservedID(TranslationTable::ID);
  }

  void ProcessBlock(llvm::BasicBlock *blk, std::vector<BufChunk *> &dircb_owlds, std::vector<BufChunk *> &indcb_owlds,
                    std::vector<BufChunk *> &indcb_gathers);
  void ProcessFunction(llvm::Function *function);

  virtual bool runOnFunction(llvm::Function &func) override;

  virtual StringRef getPassName() const override { return IGCOpts::ConstantCoalescingPass; }

private:
  class IRBuilderWrapper : protected llvm::IGCIRBuilder<> {

  public:
    IRBuilderWrapper(LLVMContext &C, TranslationTable *pTT) : llvm::IGCIRBuilder<>(C), m_TT(pTT) {}

    /// \brief Get a constant 32-bit value.
    ConstantInt *getInt32(uint32_t C) { return IGCIRBuilder<>::getInt32(C); }

    ConstantInt *getFalse() { return IGCIRBuilder<>::getFalse(); }

    using IGCIRBuilder<>::getCurrentDebugLocation;
    using IGCIRBuilder<>::getInt32Ty;
    using IGCIRBuilder<>::getFloatTy;
    using IGCIRBuilder<>::SetInsertPoint;

    // Instruction creators:

    Value *CreateAdd(Value *LHS, Value *RHS, const Twine &Name = "", bool HasNUW = false, bool HasNSW = false) {
      Value *val = IGCIRBuilder<>::CreateAdd(LHS, RHS, Name, HasNUW, HasNSW);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    Value *CreateOr(Value *LHS, Value *RHS, const Twine &Name = "") {
      Value *val = IGCIRBuilder<>::CreateOr(LHS, RHS, Name);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    Value *CreateAnd(Value *LHS, Value *RHS, const Twine &Name = "") {
      Value *val = IRBuilder<>::CreateAnd(LHS, RHS, Name);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    Value *CreatePtrToInt(Value *V, Type *DestTy, const Twine &Name = "") {
      Value *val = IGCIRBuilder<>::CreatePtrToInt(V, DestTy, Name);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    LoadInst *CreateLoad(Type *Ty, Value *Ptr, const char *Name) {
      LoadInst *val = IGCIRBuilder<>::CreateLoad(Ty, Ptr, Name);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    LoadInst *CreateLoad(Type *Ty, Value *Ptr, const Twine &Name = "") {
      LoadInst *val = IGCIRBuilder<>::CreateLoad(Ty, Ptr, Name);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    LoadInst *CreateLoad(Type *Ty, Value *Ptr, bool isVolatile, const Twine &Name = "") {
      LoadInst *val = IGCIRBuilder<>::CreateLoad(Ty, Ptr, isVolatile, Name);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    CallInst *CreateCall2(Value *Callee, Value *Arg1, Value *Arg2, const Twine &Name = "") {
      CallInst *val = IGCIRBuilder<>::CreateCall2(Callee, Arg1, Arg2, Name);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    Value *CreateMul(Value *LHS, Value *RHS, const Twine &Name = "", bool HasNUW = false, bool HasNSW = false) {
      Value *val = IGCIRBuilder<>::CreateMul(LHS, RHS, Name, HasNUW, HasNSW);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    Value *CreateShl(Value *LHS, Value *RHS, const Twine &Name = "", bool HasNUW = false, bool HasNSW = false) {
      Value *val = IGCIRBuilder<>::CreateShl(LHS, RHS, Name, HasNUW, HasNSW);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    Value *CreateLShr(Value *LHS, Value *RHS, const Twine &Name = "", bool isExact = false) {
      Value *val = IGCIRBuilder<>::CreateLShr(LHS, RHS, Name, isExact);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    Value *CreateBitCast(Value *V, Type *DestTy, const Twine &Name = "") {
      Value *val = IGCIRBuilder<>::CreateBitCast(V, DestTy, Name);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    Value *CreateZExt(Value *V, Type *DestTy, const Twine &Name = "") {
      Value *val = IRBuilder<>::CreateZExt(V, DestTy, Name);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    Value *CreateSExt(Value *V, Type *DestTy, const Twine &Name = "") {
      Value *val = IRBuilder<>::CreateSExt(V, DestTy, Name);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    Value *CreateZExtOrTrunc(Value *V, Type *DestTy, const Twine &Name = "") {
      Value *val = IRBuilder<>::CreateZExtOrTrunc(V, DestTy, Name);
      if (val != V) {
        m_TT->RegisterNewValueAndAssignID(val);
      }
      return val;
    }

    Value *CreateExtractElement(Value *Vec, Value *Idx, const Twine &Name = "") {
      Value *val = IGCIRBuilder<>::CreateExtractElement(Vec, Idx, Name);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    CallInst *CreateCall(Value *Callee, ArrayRef<Value *> Args, const Twine &Name = "") {
      CallInst *val = IGCIRBuilder<>::CreateCall(Callee, Args, Name);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

    Value *CreateInsertElement(Value *Vec, Value *NewElt, Value *Idx, const Twine &Name = "") {
      Value *val = IGCIRBuilder<>::CreateInsertElement(Vec, NewElt, Idx, Name);
      m_TT->RegisterNewValueAndAssignID(val);
      return val;
    }

  private:
    TranslationTable *m_TT;
  };

  // In current implementation merged loads are aligned to at least 4 bytes.
  const alignment_t m_ChunkMinAlignment = 4;

  CodeGenContext *m_ctx;
  llvm::Function *curFunc;
  // Helper map that tracks already processed or created base offsets.
  // The map holds `add` instructions with their arguments as the key.
  std::map<std::pair<llvm::Value *, llvm::Value *>, llvm::Instruction *> m_BaseOffsets;
  // agent to modify the llvm-ir
  IRBuilderWrapper *irBuilder;
  // maintain the uniformness info
  WIAnalysis *wiAns;
  const llvm::DataLayout *dataLayout;
  TranslationTable *m_TT;

  /// Examines the uniformity of the load and the number of used elements
  /// to determine whether we should try to merge it.
  bool isProfitableLoad(const Instruction *I, uint32_t &MaxEltPlus) const;
  /// Is this a chunk we should be creating?
  static bool profitableChunkSize(uint32_t ub, uint32_t lb, uint32_t eltSizeInBytes);
  static bool profitableChunkSize(uint32_t chunkSize, uint32_t eltSizeInBytes);
  /// check if two access have the same buffer-base
  static bool CompareBufferBase(const llvm::Value *bufIdxV1, uint bufid1, const llvm::Value *bufIdxV2, uint bufid2);
  /// find element base and element imm-offset
  llvm::Value *SimpleBaseOffset(llvm::Value *elt_idxv, uint &offset, ExtensionKind &Extension);
  /// finds the minimum power-of-2 alignment for an offset in buffer
  uint GetOffsetAlignment(Value *val, PHINode *currPhi) const;
  /// used along ocl path, based upon int2ptr
  bool DecomposePtrExp(llvm::Value *ptr_val, llvm::Value *&buf_idxv, llvm::Value *&elt_idxv, uint &eltid,
                       ExtensionKind &Extension);
  static uint CheckVectorElementUses(const llvm::Instruction *load);
  void AdjustChunk(BufChunk *cov_chunk, uint start_adj, uint size_adj, const ExtensionKind &Extension);
  void EnlargeChunk(BufChunk *cov_chunk, uint size_adj);
  void MoveExtracts(BufChunk *cov_chunk, llvm::Instruction *load, uint start_adj);
  llvm::Value *FormChunkAddress(BufChunk *chunk, const ExtensionKind &Extension);
  void CombineTwoLoads(BufChunk *cov_chunk, llvm::Instruction *load, uint eltid, uint numelt,
                       const ExtensionKind &Extension);
  llvm::Instruction *CreateChunkLoad(llvm::Instruction *load, BufChunk *chunk, uint eltid, alignment_t alignment,
                                     const ExtensionKind &Extension);
  llvm::Instruction *AddChunkExtract(llvm::Instruction *load, uint offset);
  llvm::Instruction *FindOrAddChunkExtract(BufChunk *cov_chunk, uint eltid);
  llvm::Instruction *EnlargeChunkAddExtract(BufChunk *cov_chunk, uint size_adj, uint eltid);
  llvm::Instruction *AdjustChunkAddExtract(BufChunk *cov_chunk, uint start_adj, uint size_adj, uint eltid,
                                           const ExtensionKind &Extension);
  llvm::Instruction *CreateSamplerLoad(llvm::Value *index, llvm::Value *resourcePtr, uint addrSpace);
  void ReplaceLoadWithSamplerLoad(Instruction *loadToReplace, Instruction *ldData, uint offsetInBytes);
  void MergeUniformLoad(llvm::Instruction *load, llvm::Value *bufIdxV, uint addrSpace, llvm::Value *eltIdxV,
                        uint offsetInBytes, uint maxEltPlus, const ExtensionKind &Extension,
                        std::vector<BufChunk *> &chunk_vec);
  void MergeScatterLoad(llvm::Instruction *load, llvm::Value *bufIdxV, uint bufid, llvm::Value *eltIdxV,
                        uint offsetInBytes, uint maxEltPlus, const ExtensionKind &Extension,
                        std::vector<BufChunk *> &chunk_vec);
  void ScatterToSampler(llvm::Instruction *load, llvm::Value *bufIdxV, uint bufid, llvm::Value *eltIdxV, uint eltid,
                        std::vector<BufChunk *> &chunk_vec);

  bool CleanupExtract(llvm::BasicBlock *bb);
  void VectorizePrep(llvm::BasicBlock *bb);
  bool safeToMoveInstUp(Instruction *inst, Instruction *newLocation);

  bool IsSamplerAlignedAddress(Value *addr) const;
  Value *GetSamplerAlignedAddress(Value *inst);

  bool IsDwordAligned(Value *val) const;

  alignment_t GetAlignment(Instruction *load) const;
  void SetAlignment(Instruction *load, uint alignment);
  void SetAlignmentFromOffset(Instruction *load);

  bool CheckForAliasingWrites(uint32_t addrSpace, llvm::Instruction *dominatingChunk,
                              llvm::Instruction *mergeCandidate);

  bool CompareMetadata(Instruction *instA, Instruction *instB) const;
  void CopyMetadata(Instruction *dst, Instruction *src);
};

} // namespace IGC
