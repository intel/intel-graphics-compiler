/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/TransformSimd1LoadsStores.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "common/debug/Debug.hpp"
#include <llvm/IR/Function.h>
#include "common/igc_regkeys.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-transform-simd1-loads-stores"
#define PASS_DESCRIPTION "Collapse SIMD32 load or stores into SIMD1 to reduce pressure"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(TransformSimd1LoadsStores, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(TransformSimd1LoadsStores, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char TransformSimd1LoadsStores::ID = 0;

TransformSimd1LoadsStores::TransformSimd1LoadsStores() : FunctionPass(ID) {
  initializeTransformSimd1LoadsStoresPass(*PassRegistry::getPassRegistry());
}

bool TransformSimd1LoadsStores::runOnFunction(Function &F) {
  mCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  unsigned loadFlagValue = IGC_GET_FLAG_VALUE(EnableTransformSimd1Loads);
  unsigned storeFlagValue = IGC_GET_FLAG_VALUE(EnableTransformSimd1Stores);

  bool isDualKSPPS = mCtx->type == ShaderType::PIXEL_SHADER && mCtx->platform.hasDualKSPPS();

  // bit4 (16) - allow pixel shader with dual KSP
  bool loadAllowPS = (loadFlagValue & 16) != 0;
  bool storeAllowPS = (storeFlagValue & 16) != 0;

  bool enableLoads = loadFlagValue != 0 &&
                     mCtx->m_retryManager->GetLastSpillSize() < IGC_GET_FLAG_VALUE(TransformSimd1LoadsSpillThreshold) &&
                     (loadAllowPS || !isDualKSPPS);

  bool enableStores =
      storeFlagValue != 0 &&
      mCtx->m_retryManager->GetLastSpillSize() < IGC_GET_FLAG_VALUE(TransformSimd1StoresSpillThreshold) &&
      (storeAllowPS || !isDualKSPPS);

  if (!enableLoads && !enableStores)
    return false;

  mWI = &getAnalysis<WIAnalysis>();
  mMod = F.getParent();

  mChanged = false;
  mDeferredClusterUGMLoads.clear();
  mDeferredStores.clear();
  mBBWaveInfoMap.clear();

  mLoadType = enableLoads ? loadFlagValue : 0;
  mStoreType = enableStores ? storeFlagValue : 0;

  // bit3 (8) - allow runtime uniformity check
  mAllowLoadRuntimeUniformCheck = (mLoadType & 8) != 0;
  mAllowStoreRuntimeUniformCheck = (mStoreType & 8) != 0;

  // Early exit if neither loads nor stores are enabled
  if (mLoadType == 0 && mStoreType == 0)
    return false;

  visit(F);

  if (mDeferredClusterUGMLoads.size() > 0) {
    for (LdRawIntrinsic *I : mDeferredClusterUGMLoads)
      multiversionLoadUGM(I);
    mDeferredClusterUGMLoads.clear();
  }

  if (mDeferredStores.size() > 0) {
    for (Instruction *I : mDeferredStores)
      multiversionStore(I);
    mDeferredStores.clear();
  }

  return mChanged;
}

// Ensure WaveBallot/Lane are created for BB, and store them in the per-BB map.
// Also sets mWaveBallot/mLane to the values for this BB.
void TransformSimd1LoadsStores::ensureBBWaveInfo(BasicBlock *BB, Instruction &InsertBefore) {
  auto it = mBBWaveInfoMap.find(BB);
  if (it != mBBWaveInfoMap.end()) {
    // Already created - just activate
    mWaveBallot = it->second.WaveBallot;
    mLane = it->second.Lane;
    return;
  }

  createWaveBallot(InsertBefore);
  createLane(InsertBefore);

  BBWaveInfo info;
  info.WaveBallot = mWaveBallot;
  info.Lane = mLane;
  mBBWaveInfoMap[BB] = info;
}

void TransformSimd1LoadsStores::createWaveBallot(Instruction &I) {
  IRBuilder<> builder(&I);

  Type *TypesBal[2] = {builder.getInt1Ty(), builder.getInt32Ty()};
  Function *waveBallotFn = GenISAIntrinsic::getDeclaration(mMod, GenISAIntrinsic::GenISA_WaveBallot, TypesBal);

  Value *ArgsBal[2] = {builder.getTrue(), builder.getInt32(0)};
  mWaveBallot = builder.CreateCall(waveBallotFn, ArgsBal, "WaveBallot");
}

void TransformSimd1LoadsStores::createLane(Instruction &I) {
  IRBuilder<> builder(&I);
  Function *laneFn;

  // FirstBitLo returns the # of LSB zeros
  laneFn = GenISAIntrinsic::getDeclaration(mMod, GenISAIntrinsic::GenISA_firstbitLo);
  mLane = builder.CreateCall(laneFn, mWaveBallot, "Lane");
}

Function *TransformSimd1LoadsStores::createWaveShuffleIdxFn(Instruction &I, Type *type) {
  IRBuilder<> builder(&I);
  Type *TypesShuf[3] = {type, builder.getInt32Ty(), builder.getInt32Ty()};
  return GenISAIntrinsic::getDeclaration(mMod, GenISAIntrinsic::GenISA_WaveShuffleIndex, TypesShuf);
}

Value *TransformSimd1LoadsStores::createWaveShuffleIdx(Instruction &I, Value *value) {
  IRBuilder<> builder(&I);
  Function *waveShuffleIndexFn = createWaveShuffleIdxFn(I, value->getType());
  Value *ArgsShuf[3] = {value, mLane, builder.getInt32(0)};
  return builder.CreateCall(waveShuffleIndexFn, ArgsShuf, "WaveShufIdx");
}

void TransformSimd1LoadsStores::useWaveShuffleIdx(Instruction &I, uint valueIdx, Value *waveShuffleIndex) {
  I.setOperand(valueIdx, waveShuffleIndex);
}

void TransformSimd1LoadsStores::createWaveShuffleIdx4TGM(Instruction &I, uint idx_x, uint idx_y, uint idx_z,
                                                         uint idx_w) {
  IRBuilder<> builder(&I);
  Value *x = I.getOperand(idx_x);
  Value *y = I.getOperand(idx_y);
  Value *z = I.getOperand(idx_z);
  Value *w = I.getOperand(idx_w);

  bool useOneWaveShuf = false;
  if (x == y && z == w && x == w)
    useOneWaveShuf = true;

  Function *waveShuffleIndexFn = createWaveShuffleIdxFn(I, x->getType());
  if (useOneWaveShuf) {
    Value *ArgsShuf[3] = {x, mLane, builder.getInt32(0)};
    Value *waveShuffleIndex = builder.CreateCall(waveShuffleIndexFn, ArgsShuf, "WaveShufIdx");

    I.setOperand(idx_x, waveShuffleIndex);
    I.setOperand(idx_y, waveShuffleIndex);
    I.setOperand(idx_z, waveShuffleIndex);
    I.setOperand(idx_w, waveShuffleIndex);
  } else {
    Value *ArgsShuf_x[3] = {x, mLane, builder.getInt32(0)};
    Value *ArgsShuf_y[3] = {y, mLane, builder.getInt32(0)};
    Value *ArgsShuf_z[3] = {z, mLane, builder.getInt32(0)};
    Value *ArgsShuf_w[3] = {w, mLane, builder.getInt32(0)};
    Value *waveShuffleIndex_x = builder.CreateCall(waveShuffleIndexFn, ArgsShuf_x, "WaveShufIdx_x");
    Value *waveShuffleIndex_y = builder.CreateCall(waveShuffleIndexFn, ArgsShuf_y, "WaveShufIdx_y");
    Value *waveShuffleIndex_z = builder.CreateCall(waveShuffleIndexFn, ArgsShuf_z, "WaveShufIdx_z");
    Value *waveShuffleIndex_w = builder.CreateCall(waveShuffleIndexFn, ArgsShuf_w, "WaveShufIdx_w");
    I.setOperand(idx_x, waveShuffleIndex_x);
    I.setOperand(idx_y, waveShuffleIndex_y);
    I.setOperand(idx_z, waveShuffleIndex_z);
    I.setOperand(idx_w, waveShuffleIndex_w);
  }
}

Value *TransformSimd1LoadsStores::createRuntimeUniformCheck(Instruction &I, Value *val, Value *&outShuffledVal) {
  IRBuilder<> builder(&I);

  outShuffledVal = createWaveShuffleIdx(I, val);

  Value *cmp = builder.CreateICmpEQ(val, outShuffledVal, "CmpUniform");

  Type *TypesBal[2] = {builder.getInt1Ty(), builder.getInt32Ty()};
  Function *waveBallotFn = GenISAIntrinsic::getDeclaration(mMod, GenISAIntrinsic::GenISA_WaveBallot, TypesBal);
  Value *ArgsBal[2] = {cmp, builder.getInt32(0)};
  Value *ballotCmp = builder.CreateCall(waveBallotFn, ArgsBal, "BallotCmp");

  Value *allSame = builder.CreateICmpEQ(ballotCmp, mWaveBallot, "AllLanesSame");

  return allSame;
}

// clang-format off
// Multiversion a UGM load with nonuniform offset, following AtomicWaveFuse pattern:
//
// Original:
//   %val = ldraw(resource, offset, ...)   ; offset nonuniform
//
// Transformed:
//   %shuffledOff = WaveShuffleIndex(offset, Lane, 0)
//   %cmp         = icmp eq offset, %shuffledOff
//   %ballot      = WaveBallot(%cmp, 0)
//   %allSame     = icmp eq %ballot, %WaveBallot
//   br %allSame, label %uniform.load, label %default.load
//
// uniform.load:                            ; all lanes have same offset
//   %val.uniform = ldraw(resource, offset, ...)
//   %broadcast   = WaveShuffleIndex(%val.uniform, Lane, 0)
//   br label %merge
//
// default.load:                            ; offset varies across lanes
//   %val.default = ldraw(resource, offset, ...)
//   br label %merge
//
// merge:
//   %val = phi [%broadcast, %uniform.load], [%val.default, %default.load]
// clang-format on
void TransformSimd1LoadsStores::multiversionLoadUGM(LdRawIntrinsic *I) {
  Value *resource = I->getResourceValue();
  Value *offset = I->getOffsetValue();

  // Resource must still be uniform; only offset can be nonuniform
  if (!isUniform(resource))
    return;

  // If offset is already uniform, the normal path handles it
  if (isUniform(offset))
    return;

  // Result must be nonuniform (otherwise EmitVISAPass handles it)
  if (mWI->isUniform(I))
    return;

  // Only scalar results for now
  Type *resultTy = I->getType();
  if (resultTy->isVectorTy())
    return;

  BasicBlock *BB = I->getParent();
  LLVMContext &context = BB->getContext();

  // Ensure WaveBallot/Lane are set up for this BB and activate them
  ensureBBWaveInfo(BB, *I);

  // --- Build the runtime uniformity check before the load ---
  Value *shuffledOffset = nullptr;
  Value *allSame = createRuntimeUniformCheck(*I, offset, shuffledOffset);

  // --- Split: BB -> [uniformBB, defaultBB] -> mergeBB ---
  // Clone the load for the default (fallback) path
  Instruction *defaultLoad = I->clone();
  IGCLLVM::insertBefore(defaultLoad, I);

  BasicBlock *defaultBB = BB->splitBasicBlock(defaultLoad, "load-default");
  BasicBlock *mergeBB = defaultBB->splitBasicBlock(I, "load-merge");

  BasicBlock *uniformBB = BasicBlock::Create(context, "load-uniform", BB->getParent(), defaultBB);

  // Replace BB's unconditional branch with conditional branch on allSame
  BB->getTerminator()->eraseFromParent();
  IRBuilder<> builderBB(BB);
  builderBB.CreateCondBr(allSame, uniformBB, defaultBB);

  // --- Uniform path: load once + broadcast ---
  IRBuilder<> builderUniform(uniformBB);
  // The load in the uniform path uses the original instruction (moved here)
  I->removeFromParent();
  IGCLLVM::insertBefore(I, builderUniform.CreateUnreachable()); // placeholder
  uniformBB->getTerminator()->eraseFromParent();       // remove unreachable

  // Broadcast result from elected lane
  Function *waveShuffleIndexFn = createWaveShuffleIdxFn(*I, resultTy);
  Value *ArgsShuf[3] = {I, mLane, builderUniform.getInt32(0)};
  Value *broadcastVal = builderUniform.CreateCall(waveShuffleIndexFn, ArgsShuf, "LoadBroadcast");
  builderUniform.CreateBr(mergeBB);

  // --- Default path already has the cloned load, just branch to merge ---
  // (splitBasicBlock already created the branch)

  // --- Merge: PHI ---
  IRBuilder<> builderMerge(&mergeBB->front());
  PHINode *phi = builderMerge.CreatePHI(resultTy, 2, "load.merge");
  phi->addIncoming(broadcastVal, uniformBB);
  phi->addIncoming(defaultLoad, defaultBB);

  // Replace all remaining uses of the original load with the PHI
  // (the WaveShuffleIndex in uniformBB still references I directly, so exclude it)
  I->replaceUsesWithIf(phi, [&](Use &U) { return U.getUser() != cast<CallInst>(broadcastVal); });

  mChanged = true;
}

// Builds the per-lane fusing condition used by multiversionStore: it is true on
// lanes whose address operand(s) match the first active lane's address(es).
// Only nonuniform operands are compared; uniform ones are trivially equal.
// Mirrors AtomicWaveFuse::GenerateFusingCondition.
Value *TransformSimd1LoadsStores::createStoreFuseCondition(Instruction &I, ArrayRef<Value *> addrs) {
  IRBuilder<> builder(&I);
  Value *cond = nullptr;
  for (Value *a : addrs) {
    if (isUniform(a))
      continue;
    Value *addrInt = a;
    if (a->getType()->isPointerTy())
      addrInt = builder.CreatePtrToInt(a, builder.getInt32Ty(), "AddrAsInt");
    Value *shuffled = createWaveShuffleIdx(I, addrInt);
    Value *cmp = builder.CreateICmpEQ(addrInt, shuffled, "FuseMatch");
    cond = cond ? builder.CreateAnd(cond, cmp, "FuseAnd") : cmp;
  }
  // All address operands uniform: nothing to gate on (should not normally reach
  // here since we only defer nonuniform-address stores).
  if (!cond)
    cond = builder.getTrue();
  return cond;
}

// clang-format off
// Branch-based store coalescing, following the AtomicWaveFuse (no-return) pattern
// (see multiversionAtomicSingleMsg / atomic_add_ret_not_used). For a store whose
// address is nonuniform across the wave, only a SINGLE store instruction is kept
// (the original one, moved into store-fused) - there is no cloned store.
//
// Original:
//   store value, addr        ; addr nonuniform
//
// Transformed:
//   %match = <addr == WaveShuffleIndex(addr, firstActiveLane)>   ; per-lane
//   br %match, label %store-match, label %store-decider
//
// store-match:                ; lanes sharing the first active lane's address
//   %isFirst = simdLaneId == firstActiveLane
//   br label %store-decider
//
// store-decider:
//   %shouldStore = phi [true, entry], [%isFirst, store-match]
//   dummyInst()                ; convergent, blocks jump threading
//   br %shouldStore, label %store-fused, label %store-merge
//
// store-fused:                 ; non-matching lanes (shouldStore==true) and the
//   store value, addr          ; first active lane of each matching group
//   br label %store-merge
//
// store-merge:
//   ...
//
// Lanes whose address differs from the first active lane's take the entry->
// decider edge with shouldStore==true, so they store individually. Matching
// lanes store only from the first active lane; since they all target the same
// address, letting the first active lane store its own value is a valid
// resolution of the otherwise racing writes.
// clang-format on
void TransformSimd1LoadsStores::multiversionStore(Instruction *I) {
  // Determine the address operands whose runtime uniformity gates coalescing.
  SmallVector<Value *, 4> addrs;
  if (auto *SI = dyn_cast<StoreInst>(I)) {
    addrs.push_back(SI->getPointerOperand());
  } else if (auto *GI = dyn_cast<GenIntrinsicInst>(I)) {
    switch (GI->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_typedwrite:
      addrs.push_back(GI->getOperand(1)); // u
      addrs.push_back(GI->getOperand(2)); // v
      addrs.push_back(GI->getOperand(3)); // r
      addrs.push_back(GI->getOperand(4)); // LOD
      break;
    case GenISAIntrinsic::GenISA_storeraw_indexed:
    case GenISAIntrinsic::GenISA_storerawvector_indexed:
      addrs.push_back(cast<StoreRawIntrinsic>(GI)->getOffsetValue());
      break;
    default:
      return;
    }
  } else {
    return;
  }

  BasicBlock *BB = I->getParent();
  LLVMContext &context = BB->getContext();

  // Ensure WaveBallot/Lane are set up for this BB and activate them.
  ensureBBWaveInfo(BB, *I);

  // Per-lane condition: does this lane's address match the first active lane's?
  Value *fuseCond = createStoreFuseCondition(*I, addrs);

  // --- Split: BB -> [store-match / store-decider] -> store-fused -> store-merge ---
  // store-merge holds everything after the store; the store itself is moved
  // into store-fused so exactly one store instruction remains.
  BasicBlock *mergeBB = BB->splitBasicBlock(I, "store-merge");

  BasicBlock *matchBB = BasicBlock::Create(context, "store-match", BB->getParent(), mergeBB);
  BasicBlock *deciderBB = BasicBlock::Create(context, "store-decider", BB->getParent(), mergeBB);
  BasicBlock *fusedBB = BasicBlock::Create(context, "store-fused", BB->getParent(), mergeBB);

  // BB: branch on whether this lane matches the first active lane's address.
  // Non-matching lanes go straight to the decider (shouldStore == true).
  BB->getTerminator()->eraseFromParent();
  IRBuilder<> builderBB(BB);
  builderBB.CreateCondBr(fuseCond, matchBB, deciderBB);

  // store-match: among matching lanes, elect the first active lane to store.
  IRBuilder<> builderMatch(matchBB);
  Function *laneFn = GenISAIntrinsic::getDeclaration(mMod, GenISAIntrinsic::GenISA_simdLaneId);
  Value *simdLaneId = builderMatch.CreateCall(laneFn, {}, "simdLaneId");
  Value *simdLaneId32 = builderMatch.CreateZExt(simdLaneId, builderMatch.getInt32Ty());
  Value *isFirstLane = builderMatch.CreateICmpEQ(simdLaneId32, mLane, "isFirstLane");
  builderMatch.CreateBr(deciderBB);

  // store-decider: pick who performs the (single) store.
  IRBuilder<> builderDecider(deciderBB);
  PHINode *shouldStore = builderDecider.CreatePHI(builderDecider.getInt1Ty(), 2, "shouldStore");
  shouldStore->addIncoming(builderDecider.getTrue(), BB);
  shouldStore->addIncoming(isFirstLane, matchBB);
  // Convergent dummy to keep jump threading / SimplifyCFG from collapsing this
  // wave region (mirrors AtomicWaveFuse).
  Function *dummyFn =
      GenISAIntrinsic::getDeclaration(mMod, GenISAIntrinsic::GenISA_dummyInstID, {builderDecider.getInt32Ty()});
  CallInst *dummyInst = builderDecider.CreateCall(dummyFn, {builderDecider.getInt32(0)});
  dummyInst->setConvergent();
  builderDecider.CreateCondBr(shouldStore, fusedBB, mergeBB);

  // store-fused: the single store (original instruction moved here).
  IRBuilder<> builderFused(fusedBB);
  Instruction *brFused = builderFused.CreateBr(mergeBB);
  I->removeFromParent();
  IGCLLVM::insertBefore(I, brFused);

  mChanged = true;
}

void TransformSimd1LoadsStores::visitLoadInst(LoadInst &I) {
  bool modeSelected = (mLoadType & 1); // bit0 - SLM load
  if (!modeSelected)
    return;

  Value *addr = I.getPointerOperand();

  // support SLM only
  if (I.getPointerAddressSpace() != ADDRESS_SPACE_LOCAL)
    return;

  // Address must be uniform (same constant buffer address across all lanes)
  if (!mWI->isUniform(addr))
    return;

  // The loaded result should be nonuniform (otherwise already handled)
  if (mWI->isUniform(&I))
    return;

  // support scalar type only
  if (I.getType()->isVectorTy())
    return;

  BasicBlock *bb = I.getParent();
  ensureBBWaveInfo(bb, I);

  // Insert shuffle after the load to broadcast the first lane's value
  IRBuilder<> builder(I.getNextNode());
  Function *waveShuffleIndexFn = createWaveShuffleIdxFn(I, I.getType());
  Value *ArgsShuf[3] = {&I, mLane, builder.getInt32(0)};
  Value *broadcastVal = builder.CreateCall(waveShuffleIndexFn, ArgsShuf, "LoadBroadcast");

  // Replace all uses of the load with the broadcast value (except the shuffle itself)
  I.replaceAllUsesWith(broadcastVal);
  // Fix up the shuffle to use the original load as its source
  cast<CallInst>(broadcastVal)->setArgOperand(0, &I);

  mChanged = true;
}

void TransformSimd1LoadsStores::loadTGM(GenIntrinsicInst *I) {
  bool modeSelected = (mLoadType & 2); // bit1
  if (!modeSelected)
    return;

  llvm::Value *src = I->getOperand(0);
  llvm::Value *u = I->getOperand(1);
  llvm::Value *v = I->getOperand(2);
  llvm::Value *r = I->getOperand(3);
  llvm::Value *LOD = I->getOperand(4);

  // Source resource needs to be uniform
  if (!isUniform(src))
    return;

  // u/v/r/LOD need to be uniform (same address across all lanes)
  if (!isUniform(u, v, r, LOD))
    return;

  // The result should be nonuniform (otherwise already handled by EmitVISAPass)
  if (mWI->isUniform(I))
    return;

  BasicBlock *bb = I->getParent();
  ensureBBWaveInfo(bb, *I);

  SmallVector<ExtractElementInst *, 4> extracts;
  for (auto *user : I->users()) {
    if (auto *EE = dyn_cast<ExtractElementInst>(user))
      extracts.push_back(EE);
  }

  if (extracts.empty()) {
    if (!I->getType()->isVectorTy()) {
      IRBuilder<> builder(I->getNextNode());
      Function *waveShuffleIndexFn = createWaveShuffleIdxFn(*I, I->getType());
      Value *ArgsShuf[3] = {I, mLane, builder.getInt32(0)};
      Value *broadcastVal = builder.CreateCall(waveShuffleIndexFn, ArgsShuf, "TGMLoadBroadcast");
      I->replaceAllUsesWith(broadcastVal);
      cast<CallInst>(broadcastVal)->setArgOperand(0, I);
      mChanged = true;
    }
    return;
  }

  for (auto *EE : extracts) {
    IRBuilder<> builder(EE->getNextNode());
    Function *waveShuffleIndexFn = createWaveShuffleIdxFn(*EE, EE->getType());
    Value *ArgsShuf[3] = {EE, mLane, builder.getInt32(0)};
    Value *broadcastVal = builder.CreateCall(waveShuffleIndexFn, ArgsShuf, "TGMLoadBroadcast");
    EE->replaceAllUsesWith(broadcastVal);
    cast<CallInst>(broadcastVal)->setArgOperand(0, EE);
  }

  mChanged = true;
}

void TransformSimd1LoadsStores::loadUGM(LdRawIntrinsic *I) {
  bool modeSelected = (mLoadType & 4); // bit2
  if (!modeSelected)
    return;

  Value *resource = I->getResourceValue();
  Value *offset = I->getOffsetValue();

  bool resourceUniform = isUniform(resource);
  bool offsetUniform = isUniform(offset);

  // If both are uniform, take the existing fast path (no BB split needed)
  if (resourceUniform && offsetUniform) {
    // The result should be nonuniform (otherwise already handled)
    if (mWI->isUniform(I))
      return;

    BasicBlock *bb = I->getParent();
    ensureBBWaveInfo(bb, *I);

    Type *resultTy = I->getType();
    auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(resultTy);

    if (VTy) {
      const unsigned maxElemSize = 8;
      unsigned elemCount = int_cast<unsigned>(VTy->getNumElements());
      if (elemCount > maxElemSize)
        return;

      // Follow the loadTGM pattern: find existing ExtractElement users
      // and broadcast each one individually, avoiding creation of new
      // extract/insert chains that extend live ranges and cause spills.
      SmallVector<ExtractElementInst *, 8> extracts;
      for (auto *user : I->users()) {
        if (auto *EE = dyn_cast<ExtractElementInst>(user))
          extracts.push_back(EE);
      }

      // No ExtractElement users - kip vector transform
      if (extracts.empty())
        return;

      // Skip if not all users are ExtractElement - mixed use patterns
      // can cause the load to remain live alongside the broadcasts,
      // increasing register pressure instead of reducing it.
      if (extracts.size() != I->getNumUses())
        return;

      for (auto *EE : extracts) {
        IRBuilder<> builder(EE->getNextNode());
        Function *waveShuffleIndexFn = createWaveShuffleIdxFn(*EE, EE->getType());
        Value *ArgsShuf[3] = {EE, mLane, builder.getInt32(0)};
        Value *broadcastVal = builder.CreateCall(waveShuffleIndexFn, ArgsShuf, "UGMLoadBroadcast");
        EE->replaceAllUsesWith(broadcastVal);
        cast<CallInst>(broadcastVal)->setArgOperand(0, EE);
      }
    } else {
      IRBuilder<> builder(I->getNextNode());
      Function *waveShuffleIndexFn = createWaveShuffleIdxFn(*I, resultTy);
      Value *ArgsShuf[3] = {I, mLane, builder.getInt32(0)};
      Value *broadcastVal = builder.CreateCall(waveShuffleIndexFn, ArgsShuf, "UGMLoadBroadcast");
      I->replaceAllUsesWith(broadcastVal);
      cast<CallInst>(broadcastVal)->setArgOperand(0, I);
    }

    mChanged = true;
    return;
  }

  // Address is nonuniform: try runtime uniformity check if enabled
  // (following the AtomicWaveFuse multiversioning pattern)
  if (mAllowLoadRuntimeUniformCheck) {
    mDeferredClusterUGMLoads.push_back(I);
  }
}

// clang-format off
// From
//   store i32 % 5, i32 addrspace(3)* inttoptr(i32 8 to i32 addrspace(3)*), align 8
// To
//   %WaveBallot = call i32 @llvm.genx.GenISA.WaveBallot.i1.i32(i1 true, i32 0)
//   %Lane = call i32 @llvm.genx.GenISA.firstbitLo(i32 %WaveBallot)
//   %WaveShufIdx = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32.i32.i32(i32 %5, i32 %Lane, i32 0)
//   store i32 %WaveShufIdx, i32 addrspace(3)* inttoptr (i32 8 to i32 addrspace(3)*), align 8
// clang-format on
void TransformSimd1LoadsStores::visitStoreInst(StoreInst &I) {
  bool modeSelected = (mStoreType & 1); // bit0
  if (!modeSelected)
    return;

  Value *dest = I.getPointerOperand();
  Value *value = I.getValueOperand(); // index 0

  Align InstAlign = IGCLLVM::getAlign(I);
  Align DataAlign = IGCLLVM::getABITypeAlign(I.getModule()->getDataLayout(), value->getType());

  // Base address must be data element aligned.
  if ((IGCLLVM::getAlignmentValue(InstAlign) % IGCLLVM::getAlignmentValue(DataAlign)) != 0)
    return;

  // support scalar type only
  if (value->getType()->isVectorTy())
    return;

  // support SLM only
  if (I.getPointerAddressSpace() != ADDRESS_SPACE_LOCAL)
    return;

  bool destUniform = mWI->isUniform(dest);
  bool valueUniform = mWI->isUniform(value);

  // Already uniform dest+value is handled by EmitVISAPass
  if (destUniform && valueUniform)
    return;

  // Value must be nonuniform for this optimization to be worthwhile
  if (valueUniform)
    return;

  // Nonuniform dest requires runtime check, only do it if enabled
  if (!destUniform && !mAllowStoreRuntimeUniformCheck)
    return;

  // Nonuniform dest: defer for branch-based multiversioning (see multiversionStore).
  if (!destUniform) {
    mDeferredStores.push_back(&I);
    return;
  }

  // Original case: dest uniform, value nonuniform
  BasicBlock *bb = I.getParent();
  ensureBBWaveInfo(bb, I);

  Value *waveShuffleIndex = createWaveShuffleIdx(I, value);
  useWaveShuffleIdx(I, 0, waveShuffleIndex);

  mChanged = true;
}

void TransformSimd1LoadsStores::storeTGM(GenIntrinsicInst *I) {
  bool modeSelected = (mStoreType & 2); // bit1
  if (!modeSelected)
    return;

  llvm::Value *dest = I->getOperand(0);
  llvm::Value *u = I->getOperand(1);
  llvm::Value *v = I->getOperand(2);
  llvm::Value *r = I->getOperand(3);
  llvm::Value *LOD = I->getOperand(4);
  llvm::Value *x = I->getOperand(5);
  llvm::Value *y = I->getOperand(6);
  llvm::Value *z = I->getOperand(7);
  llvm::Value *w = I->getOperand(8);

  if (!isUniform(dest))
    return;

  bool coordsUniform = isUniform(u, v, r, LOD);

  if (!coordsUniform && !mAllowStoreRuntimeUniformCheck)
    return;

  if (!isNonuniform(x, y, z, w))
    return;

  // Nonuniform coords: defer for branch-based multiversioning (see multiversionStore).
  if (!coordsUniform) {
    mDeferredStores.push_back(I);
    return;
  }

  BasicBlock *bb = I->getParent();
  ensureBBWaveInfo(bb, *I);

  createWaveShuffleIdx4TGM(*I, 5, 6, 7, 8);

  mChanged = true;
}

void TransformSimd1LoadsStores::storeUGM(StoreRawIntrinsic *I) {
  bool modeSelected = (mStoreType & 4); // bit2
  if (!modeSelected)
    return;

  const unsigned maxElemSize = 8; // support up to vector of 8 elements
  Value *dest = I->getResourceValue();
  Value *value = I->getStoreValue(); // index 2
  Value *offset = I->getOffsetValue();

  bool destUniform = isUniform(dest);
  bool offsetUniform = isUniform(offset);
  bool valueUniform = isUniform(value);

  // dest must be uniform
  if (!destUniform)
    return;

  // Already uniform dest+offset+value is handled by EmitVISAPass
  if (offsetUniform && valueUniform)
    return;

  // Value must be nonuniform for this optimization to be worthwhile
  if (valueUniform)
    return;

  // Nonuniform offset requires runtime check, only do it if enabled
  if (!offsetUniform && !mAllowStoreRuntimeUniformCheck)
    return;

  // Nonuniform offset: defer for branch-based multiversioning (see multiversionStore).
  if (!offsetUniform) {
    mDeferredStores.push_back(I);
    return;
  }

  auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(value->getType());
  InsertElementInst *IE = dyn_cast<InsertElementInst>(value);
  GenIntrinsicInst *gen = dyn_cast<GenIntrinsicInst>(value); // default dyn_cast for scalar value
  if (VTy) {
    if (!IE)
      return;
    gen = dyn_cast<GenIntrinsicInst>(IE->getOperand(1));
  }

  // No need to transform if the value is WaveShuffleIndex
  if (gen && gen->getIntrinsicID() == GenISAIntrinsic::GenISA_WaveShuffleIndex) {
    return;
  }

  BasicBlock *bb = I->getParent();
  ensureBBWaveInfo(bb, *I);

  // dest+offset uniform, value nonuniform
  {
    IRBuilder<> builder(I);
    if (VTy) {
      unsigned elemCount = int_cast<unsigned>(VTy->getNumElements());

      if (elemCount > maxElemSize)
        return;

      Value *insertE = nullptr;
      SmallVector<Value *, maxElemSize> vExtracts;
      SmallVector<Value *, maxElemSize> vWaveShufs;
      for (unsigned e = 0; e < elemCount; e++) {
        Value *extractE = builder.CreateExtractElement(value, builder.getInt32(e), "Extract" + std::to_string(e));
        vExtracts.push_back(extractE);
      }

      for (unsigned e = 0; e < elemCount; e++) {
        Value *waveShuffleIndex = createWaveShuffleIdx(*I, vExtracts[e]);
        vWaveShufs.push_back(waveShuffleIndex);
      }

      for (unsigned e = 0; e < elemCount; e++) {
        if (e == 0)
          insertE = builder.CreateInsertElement(PoisonValue::get(IE->getType()), vWaveShufs[e], builder.getInt32(e),
                                                "Insert" + std::to_string(e));
        else
          insertE =
              builder.CreateInsertElement(insertE, vWaveShufs[e], builder.getInt32(e), "Insert" + std::to_string(e));
      }

      useWaveShuffleIdx(*I, 2, insertE);
    } else {
      Value *waveShuffleIndex = createWaveShuffleIdx(*I, value);
      useWaveShuffleIdx(*I, 2, waveShuffleIndex);
    }
  }

  mChanged = true;
}

void TransformSimd1LoadsStores::visitCallInst(CallInst &C) {
  GenIntrinsicInst *GI = llvm::dyn_cast<GenIntrinsicInst>(&C);
  if (!GI)
    return;

  switch (GI->getIntrinsicID()) {
  // Store intrinsics
  case GenISAIntrinsic::GenISA_typedwrite:
    storeTGM(GI);
    break;
  case GenISAIntrinsic::GenISA_storeraw_indexed:
  case GenISAIntrinsic::GenISA_storerawvector_indexed:
    storeUGM(cast<StoreRawIntrinsic>(GI));
    break;

  // Load intrinsics
  case GenISAIntrinsic::GenISA_typedread:
    loadTGM(GI);
    break;
  case GenISAIntrinsic::GenISA_ldraw_indexed:
  case GenISAIntrinsic::GenISA_ldrawvector_indexed:
    loadUGM(cast<LdRawIntrinsic>(GI));
    break;

  default:
    return;
  }
}

bool TransformSimd1LoadsStores::isUniform(Value *v) { return mWI->isUniform(v); }

bool TransformSimd1LoadsStores::isUniform(Value *v1, Value *v2, Value *v3, Value *v4) {
  bool uniform = mWI->isUniform(v1) && mWI->isUniform(v2) && mWI->isUniform(v3) && mWI->isUniform(v4);
  return uniform;
}

bool TransformSimd1LoadsStores::isNonuniform(Value *v1, Value *v2, Value *v3, Value *v4) {
  bool nonuniform = !isUniform(v1, v2, v3, v4);
  return nonuniform;
}

#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS
