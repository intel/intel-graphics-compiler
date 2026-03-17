/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenContextWrapper.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"

using namespace llvm;

namespace IGC {

/// Combined pass that collapses SIMD32 loads and stores into SIMD1
/// when addresses are uniform, reducing register pressure.
/// Controlled independently by EnableTransformSimd1Loads and
/// EnableTransformSimd1Stores regkeys.
class TransformSimd1LoadsStores : public FunctionPass, public InstVisitor<TransformSimd1LoadsStores> {

public:
  static char ID;

  TransformSimd1LoadsStores();
  virtual bool runOnFunction(Function &F) override;
  virtual llvm::StringRef getPassName() const override { return "TransformSimd1LoadsStores"; }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<WIAnalysis>();
    AU.setPreservesAll();
  }

  // InstVisitor callbacks - handle both loads and stores
  void visitLoadInst(LoadInst &I);
  void visitStoreInst(StoreInst &I);
  void visitCallInst(CallInst &C);

private:
  CodeGenContext *mCtx = nullptr;
  Module *mMod = nullptr;
  WIAnalysis *mWI = nullptr;
  CallInst *mWaveBallot = nullptr;
  Value *mLane = nullptr;

  // Per-BB storage for WaveBallot and Lane values
  struct BBWaveInfo {
    CallInst *WaveBallot = nullptr;
    Value *Lane = nullptr;
  };
  DenseMap<BasicBlock *, BBWaveInfo> mBBWaveInfoMap;

  bool mAllowLoadRuntimeUniformCheck = false;
  bool mAllowStoreRuntimeUniformCheck = false;

  bool mChanged = false;

  // --- Load-specific state ---
  unsigned mLoadType = 0; // bit0 - SLM; bit1 - TGM; bit2 - UGM

  // --- Store-specific state ---
  unsigned mStoreType = 0; // bit0 - SLM; bit1 - TGM; bit2 - UGM
  Value *mSimdLaneId = nullptr;
  Value *mClusterActiveLeader = nullptr;
  Value *mAmILeader = nullptr;
  SmallVector<LdRawIntrinsic *, 8> mDeferredClusterUGMLoads;

  // --- Shared helpers (single implementation) ---
  void ensureBBWaveInfo(BasicBlock *BB, Instruction &InsertBefore);
  void createWaveBallot(Instruction &I);
  void createLane(Instruction &I);
  Function *createWaveShuffleIdxFn(Instruction &I, Type *type);
  Value *createWaveShuffleIdx(Instruction &I, Value *value);
  void useWaveShuffleIdx(Instruction &I, uint valueIdx, Value *waveShuffleIndex);
  void createWaveShuffleIdx4TGM(Instruction &I, uint idx_x, uint idx_y, uint idx_z, uint idx_w);
  bool isUniform(Value *v);
  bool isUniform(Value *v1, Value *v2, Value *v3, Value *v4);
  bool isNonuniform(Value *v1, Value *v2, Value *v3, Value *v4);
  Value *createRuntimeUniformCheck(Instruction &I, Value *val, Value *&outShuffledVal);

  // --- Store-specific helpers ---
  void storeTGM(GenIntrinsicInst *I);
  void storeUGM(StoreRawIntrinsic *I);

  // --- Load-specific helpers ---
  void loadTGM(GenIntrinsicInst *I);
  void loadUGM(LdRawIntrinsic *I);
  void multiversionLoadUGM(LdRawIntrinsic *I);
};

} // namespace IGC
