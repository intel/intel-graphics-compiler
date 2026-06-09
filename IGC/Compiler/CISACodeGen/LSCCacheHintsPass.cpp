/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This pass is responsible for setting lsc.cache.ctrl metadata for all IO
// operations that need it, but don't have it set yet.
// Please do not add any future code that would be setting our cache controls
// after this pass. If you need to add it - add it here, or before this pass.
//
// Global cache-control overrides (driver-level CacheControlOverride and the
// matching IGC registry keys: {Lsc,Tgm}{Load,Store}CacheControlOverride) are
// applied here as well. They have the highest priority and will overwrite any
// cache-hint metadata previously attached by analysis passes.

#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include <llvm/Support/Debug.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include "LSCCacheHintsPass.h"
#include "getCacheOpts.h"
#include "MDFrameWork.h"
#include "ShaderCodeGen.hpp"
#include <llvm/ADT/ArrayRef.h>
#include <optional>

using namespace llvm;
using namespace IGC;

namespace IGC {

#define PASS_FLAG "LSC-Cache-Hints"
#define PASS_DESCRIPTION "Resolve LSC cache hints metadata."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LSCCacheHints, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(LSCCacheHints, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
#define DEBUG_TYPE PASS_FLAG

char LSCCacheHints::ID = 0;

llvm::FunctionPass *createLSCCacheHintsPass() { return new LSCCacheHints(); }

LSCCacheHints::LSCCacheHints() : FunctionPass(ID) {}

void LSCCacheHints::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MetaDataUtilsWrapper>();
  AU.addRequired<CodeGenContextWrapper>();
  AU.setPreservesCFG();
}

namespace {
std::optional<unsigned> GetOperandCacheControlIndex(GenISAIntrinsic::ID Id) {
  switch (Id) {
  case GenISAIntrinsic::GenISA_LSCLoad:
  case GenISAIntrinsic::GenISA_LSCLoadWithSideEffects:
  case GenISAIntrinsic::GenISA_LSCLoadBlock:
  case GenISAIntrinsic::GenISA_LSCPrefetch:
  case GenISAIntrinsic::GenISA_LSCLoadStatus:
    return 4;
  case GenISAIntrinsic::GenISA_LSCAtomicInts:
  case GenISAIntrinsic::GenISA_LSCAtomicFP32:
  case GenISAIntrinsic::GenISA_LSCAtomicFP64:
  case GenISAIntrinsic::GenISA_LSCAtomicBF16:
    return 5;
  default:
    return std::nullopt;
  }
}

bool IsMetadataChannelIntrinsic(GenISAIntrinsic::ID Id) {
  switch (Id) {
  case GenISAIntrinsic::GenISA_typedread:
  case GenISAIntrinsic::GenISA_typedwrite:
  case GenISAIntrinsic::GenISA_typedreadMS:
  case GenISAIntrinsic::GenISA_typedwriteMS:
  case GenISAIntrinsic::GenISA_LSCTypedLoadStatus:
  case GenISAIntrinsic::GenISA_simdBlockRead:
  case GenISAIntrinsic::GenISA_simdBlockReadBindless:
  case GenISAIntrinsic::GenISA_simdBlockWrite:
  case GenISAIntrinsic::GenISA_simdBlockWriteBindless:
    return true;
  default:
    return false;
  }
}

bool IsLoadShapedMetadataChannelIntrinsic(GenISAIntrinsic::ID Id) {
  switch (Id) {
  case GenISAIntrinsic::GenISA_typedread:
  case GenISAIntrinsic::GenISA_typedreadMS:
  case GenISAIntrinsic::GenISA_LSCTypedLoadStatus:
  case GenISAIntrinsic::GenISA_simdBlockRead:
  case GenISAIntrinsic::GenISA_simdBlockReadBindless:
    return true;
  default:
    return false;
  }
}
} // namespace

bool LSCCacheHints::runOnFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "\nRunning LSCCacheHints on function \"" << F.getName() << "\":\n");
  ModuleMetaData *ModMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  bool Changed = false;

  for (auto &I : instructions(F)) {
    // Operand-channel intrinsics (GenISA_LSCLoad/Prefetch/LoadStatus/Atomic*)
    // carry their cache control in an immediate operand, not lsc.cache.ctrl
    // metadata. Resolve them in place by rewriting the operand, and skip the
    // metadata channel entirely.
    if (IsOperandChannelIntrinsic(I)) {
      Changed |= ResolveOperandCacheControl(I);
      continue;
    }
    // Non-operand atomics (scalar/typed intatomicraw etc.) have no cache
    // operand and do not consume lsc.cache.ctrl in their normal flow. Their
    // only cache decision is the LscAtomicCacheControlOverride regkey, which we
    // resolve into metadata here. They must skip the metadata channel below
    // (its load/store defaults and Lsc/Tgm overrides do not apply to atomics).
    if (IsOverridableAtomic(I)) {
      Changed |= ResolveAtomicCacheControl(I);
      continue;
    }
    // This pass is restricted to IO-shaped instructions, so we don't pollute
    // unrelated IR with cache hints. Operand and atomic paths above do not need
    // ModuleMetaData; metadata-channel decisions do.
    if (ModMD != nullptr && IsCacheHintRelevantInstruction(I))
      Changed |= VisitInstruction(ModMD, I);
  }

  return Changed;
}

// TGM (typed-resource) IO intrinsics. Used to gate the TGM-only
// platform check and to pick the Tgm{Load,Store}CacheControlOverride
// flag. Kept in one place so the callers can't drift apart.
bool LSCCacheHints::IsTGMIntrinsic(GenISAIntrinsic::ID Id) {
  switch (Id) {
  case GenISAIntrinsic::GenISA_typedread:
  case GenISAIntrinsic::GenISA_typedwrite:
  case GenISAIntrinsic::GenISA_typedreadMS:
  case GenISAIntrinsic::GenISA_typedwriteMS:
  case GenISAIntrinsic::GenISA_LSCTypedLoadStatus:
    return true;
  default:
    return false;
  }
}

// Instructions whose cache hint may be represented by lsc.cache.ctrl metadata:
// plain loads/stores, raw indexed load/store, predicated load/store, typed
// (TGM) messages, simd block read/write, and cooperative-matrix
// load/store/prefetch.
//
// Intentionally excluded:
//   - Atomics: routed through LscAtomicCacheControlOverride at emit
//     time and do not consume lsc.cache.ctrl metadata.
//   - Fences (LSCFence, typedmemoryfence, systemmemoryfence).
//   - GenISA_LSC{Load,Store,Prefetch}* family and LSC2DBlock*: take
//     their cache controls from an instruction operand
//     (translateLSCCacheControlsFromValue), not metadata.
bool LSCCacheHints::IsCacheHintRelevantInstruction(const llvm::Instruction &I) {
  if (isa<LoadInst>(&I) || isa<StoreInst>(&I))
    return true;
  if (isa<LdRawIntrinsic>(&I) || isa<StoreRawIntrinsic>(&I))
    return true;
  if (isa<PredicatedLoadIntrinsic>(&I) || isa<PredicatedStoreIntrinsic>(&I))
    return true;
  const GenIntrinsicInst *II = dyn_cast<GenIntrinsicInst>(&I);
  if (!II)
    return false;
  return IsMetadataChannelIntrinsic(II->getIntrinsicID());
}

// True for the load-shaped half of IsCacheHintRelevantInstruction.
// Prefetches count as loads for cache-control override selection, so this
// can't be derived from the return type alone:
// CooperativeMatrixPrefetch returns void but is load-shaped.
// Caller must ensure I is in the set accepted by
// IsCacheHintRelevantInstruction; anything else returns false.
bool LSCCacheHints::IsLoadShapedIO(const llvm::Instruction &I) {
  if (isa<LoadInst>(&I) || isa<LdRawIntrinsic>(&I) || isa<PredicatedLoadIntrinsic>(&I))
    return true;
  const GenIntrinsicInst *II = dyn_cast<GenIntrinsicInst>(&I);
  if (!II)
    return false;
  return IsLoadShapedMetadataChannelIntrinsic(II->getIntrinsicID());
}

bool LSCCacheHints::VisitInstruction(const ModuleMetaData *ModMD, llvm::Instruction &I) {
  // Global cache-control overrides take precedence over any pre-set cache
  // metadata or analysis-based defaults below.
  bool Changed = false;
  if (TryApplyGlobalOverride(ModMD, I, Changed))
    return Changed;

  const bool HasLscCacheCtrl = (I.getMetadata("lsc.cache.ctrl") != nullptr);

  if (!HasLscCacheCtrl)
    return SetupLscCacheCtrl(ModMD, I);

  return false;
}


bool LSCCacheHints::SetInstructionCacheHint(llvm::Instruction &I, const unsigned CacheHint, llvm::StringRef Reason,
                                            bool AllowOverwrite) const {
  CodeGenContext *pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  if (!pContext->platform.LSCEnabled())
    return false;

  unsigned int NewCacheHint = CacheHint;
  const char *EntryName = "lsc.cache.ctrl";
  if (MDNode *ExistingMD = I.getMetadata(EntryName)) {
    if (ExistingMD->getNumOperands() > 0) {
      if (auto *CI = mdconst::dyn_extract<ConstantInt>(ExistingMD->getOperand(0))) {
        if (CI->getZExtValue() == NewCacheHint)
          return false;
      }
    }
    if (!AllowOverwrite)
      IGC_ASSERT_MESSAGE(0, "conflicting cache control metadata");
  }
  ConstantInt *CICC = ConstantInt::get(Type::getInt32Ty(I.getContext()), NewCacheHint);
  MDNode *node = MDNode::get(I.getContext(), ConstantAsMetadata::get(CICC));
  I.setMetadata(EntryName, node);
  LLVM_DEBUG(dbgs() << "\t" << I.getOpcodeName() << ": " << EntryName << " set to: " << NewCacheHint
                    << " due to: " << Reason << "\n");
  return true;
}

bool LSCCacheHints::SetupLscCacheCtrl(const ModuleMetaData *ModMD, llvm::Instruction &I) {
  CodeGenContext *pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  bool IsTGM = false;
  if (GenIntrinsicInst *II = dyn_cast<GenIntrinsicInst>(&I)) {
    if (IsTGMIntrinsic(II->getIntrinsicID()))
      IsTGM = true;
  }

  const bool CanGenerateLSC = CShader::shouldGenerateLSC(*pContext, &I, IsTGM, SIMDMode::UNKNOWN, std::nullopt);
  if (CanGenerateLSC && (IGC_GET_FLAG_VALUE(RovOpt) & 2) && UseRasterizerOrderedByteAddressBuffer(ModMD, I))
    return SetInstructionCacheHint(I, LSC_L1UC_L3C_WB, "RovOpt");

  // Nontemporal metadata has higher priority than the addrspace/RT/CB
  // baselines below.
  if (I.getMetadata(LLVMContext::MD_nontemporal)) {
    return SetInstructionCacheHint(I, LSC_L1UC_L3UC, "nontemporal hint");
  }

  if (TryApplyRovOrForceList(ModMD, I))
    return true;

  // TGM (typed-resource) messages only use explicit lsc.cache.ctrl metadata;
  // the addrspace/RT/CB/OCL-fence defaults below do not apply.
  if (IsTGM)
    return false;

  // Resolve metadata-channel defaults in priority order. The carrier is the
  // IGC1 LSC_L1_L3_CC enum: SetInstructionCacheHint writes lsc.cache.ctrl with
  // the chosen enum. If no decision applies, leave lsc.cache.ctrl unset so the
  // default cache behavior is preserved.
  std::optional<unsigned> CacheHint;
  llvm::StringRef Reason;
  const bool IsLoad = IsLoadShapedIO(I);

  // (1) Baseline: OpenCL fence-before-EOT default. The compOpt defaults are
  // themselves IGC1 LSC_L1_L3_CC enums.
  if (pContext->type == ShaderType::OPENCL_SHADER && IGC_IS_FLAG_ENABLED(EnableLSCFenceUGMBeforeEOT) &&
      pContext->platform.NeedsLSCFenceUGMBeforeEOT()) {
    if (IsLoad && ModMD->compOpt.LoadCacheDefault != -1) {
      CacheHint = static_cast<unsigned>(ModMD->compOpt.LoadCacheDefault);
      Reason = "OCL fence-before-EOT load default";
    } else if (!IsLoad && ModMD->compOpt.StoreCacheDefault != -1) {
      CacheHint = static_cast<unsigned>(ModMD->compOpt.StoreCacheDefault);
      Reason = "OCL fence-before-EOT store default";
    }
  }

  // (2) Baseline (overrides #1): private/constant address-space default.
  // load priv/const -> {CACHED, CACHED}    == LSC_L1C_WT_L3C_WB   (enum 4)
  // store priv      -> {WRITEBACK, WRITEBACK} == LSC_L1IAR_WB_L3C_WB (enum 7)
  // Only plain LoadInst/StoreInst use these address-space defaults; other IO
  // shapes are untouched.
  if (auto *LI = dyn_cast<LoadInst>(&I)) {
    const unsigned AddrSpace = LI->getPointerOperand()->getType()->getPointerAddressSpace();
    if (AddrSpace == ADDRESS_SPACE_PRIVATE || AddrSpace == ADDRESS_SPACE_CONSTANT) {
      CacheHint = LSC_L1C_WT_L3C_WB;
      Reason = "priv/const addrspace load default";
    }
  } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
    const unsigned AddrSpace = SI->getPointerOperand()->getType()->getPointerAddressSpace();
    if (AddrSpace == ADDRESS_SPACE_PRIVATE) {
      CacheHint = LSC_L1IAR_WB_L3C_WB;
      Reason = "priv addrspace store default";
    }
  }

  // (3) RT default policy (overrides #1/#2). Fires for RAYTRACING_SHADER or a
  // function with sync RT calls. A concrete enum overrides earlier defaults; a
  // nullopt (platform default) clears the pending hint back to "unset".
  bool HasRTCall = (pContext->type == ShaderType::RAYTRACING_SHADER) || pContext->hasSyncRTCalls(I.getFunction());
  if (HasRTCall) {
    if (auto RtEnum = IGC::getDefaultRaytracingCacheControlEnum(IsLoad, *pContext)) {
      CacheHint = static_cast<unsigned>(*RtEnum);
      Reason = "RT default policy";
    } else {
      CacheHint.reset();
      Reason = "RT default policy (platform default)";
    }
  }

  // (4) Constant-buffer load caching (overrides #1/#2/#3), load-only. For a
  // constant-addrspace load this stacks on top of the addrspace baseline:
  // {C,C} (enum 4 -> .ca.ca), CB forces L3 const-cached (LSC_L1C_L3CC enum 9 ->
  // .ca.cc) unless DisableSystemMemoryCachingInGPUForConstantBuffers.
  if (IsLoad) {
    if (auto CbEnum = IGC::getConstantBufferLoadCacheControlEnum(&I, *pContext)) {
      CacheHint = static_cast<unsigned>(*CbEnum);
      Reason = "constant-buffer load caching";
    }
  }

  if (CacheHint)
    return SetInstructionCacheHint(I, *CacheHint, Reason);

  return false;
}

bool LSCCacheHints::TryApplyRovOrForceList(const ModuleMetaData *ModMD, llvm::Instruction &I) {
  // ROV and forceLscCacheList are LSCCacheHints-local decisions that, in the
  // production pipeline, set lsc.cache.ctrl and therefore win over the
  // addrspace/RT/CB defaults below. Preserve that precedence by resolving them
  // after the nontemporal early-return and before the baseline
  // metadata-channel defaults.
  const std::map<uint32_t, uint32_t> &ForceLscCacheList = ModMD->forceLscCacheList;
  const std::set<uint32_t> &RasterizerOrderedViews = ModMD->RasterizerOrderedViews;
  for (unsigned int i = 0; i < I.getNumOperands(); i++) {
    Value *Op = I.getOperand(i);
    if (!Op->getType()->isPointerTy())
      continue;

    const unsigned int AddrSpace = Op->getType()->getPointerAddressSpace();
    if (RasterizerOrderedViews.find(AddrSpace) != RasterizerOrderedViews.end())
      return SetInstructionCacheHint(I, LSC_L1UC_L3C_WB, "accessing ROV");

    const auto ForceIt = ForceLscCacheList.find(AddrSpace);
    if (ForceIt != ForceLscCacheList.end())
      return SetInstructionCacheHint(I, ForceIt->second, "addrspace matching ForceLscCacheList");
  }
  return false;
}

// Operand-channel intrinsics: their cache control lives in an immediate i32
// operand, not in lsc.cache.ctrl metadata. This is exactly the set whose
// cache-control decisions are resolved here:
//   - GenISA_LSCLoad / LSCLoadWithSideEffects / LSCLoadBlock:
//       constant-buffer load override.
//   - GenISA_LSCPrefetch / LSCLoadStatus:
//       prefetch validation (read-invalidate / PVC-XL-A0 default).
//   - GenISA_LSCAtomic{Ints,FP32,FP64,BF16}:
//       LscAtomicCacheControlOverride.
// Pure-read operand intrinsics (LSCLoadCmask, store family, LSC2DBlock*) carry
// no decision today, so they are intentionally NOT listed: leaving their
// frontend operand untouched still propagates to downstream consumers unchanged.
bool LSCCacheHints::IsOperandChannelIntrinsic(const llvm::Instruction &I) {
  const GenIntrinsicInst *II = dyn_cast<GenIntrinsicInst>(&I);
  return II && GetOperandCacheControlIndex(II->getIntrinsicID()).has_value();
}

// Resolve the cache-control *operand* of an operand-channel intrinsic in place,
// rewriting the immediate i32 operand to its final IGC1 LSC_L1_L3_CC enum
// value (NEVER a backend-specific encoding).
bool LSCCacheHints::ResolveOperandCacheControl(llvm::Instruction &I) {
  GenIntrinsicInst *II = dyn_cast<GenIntrinsicInst>(&I);
  if (!II)
    return false;
  CodeGenContext *pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  // Operand index of the immediate cache control for each intrinsic family.
  //   load/prefetch -> op4
  //   atomic        -> op5
  const std::optional<unsigned> CacheOpIdx = GetOperandCacheControlIndex(II->getIntrinsicID());
  if (!CacheOpIdx)
    return false;

  ConstantInt *CurCI = dyn_cast<ConstantInt>(II->getOperand(*CacheOpIdx));
  if (!CurCI)
    return false;
  const unsigned CurEnum = static_cast<unsigned>(CurCI->getZExtValue());
  std::optional<unsigned> FinalEnum;

  switch (II->getIntrinsicID()) {
  // (A) Constant-buffer load caching (emitLscIntrinsicLoad, op4). Only
  // GenISA_LSCLoad/LSCLoadBlock/LSCLoadWithSideEffects take this override, and
  // it fires only when the buffer operand is a constant buffer AND (Xe2+ sys-mem
  // default OR RT-CB regkey).
  // When the shared enum decision returns nullopt, no override applies -> leave
  // the frontend operand untouched.
  case GenISAIntrinsic::GenISA_LSCLoad:
  case GenISAIntrinsic::GenISA_LSCLoadWithSideEffects:
  case GenISAIntrinsic::GenISA_LSCLoadBlock:
    if (auto CbEnum = IGC::getConstantBufferLoadCacheControlEnum(&I, *pContext))
      FinalEnum = static_cast<unsigned>(*CbEnum);
    break;

  // (B) Prefetch validation. Check the resolved cache opts and, when an invalid
  // state is detected, rewrite the operand to LSC_L1C_WT_L3C_WB (enum 4), which
  // resolves to {CACHED,CACHED}. Valid inputs are left untouched.
  case GenISAIntrinsic::GenISA_LSCPrefetch:
  case GenISAIntrinsic::GenISA_LSCLoadStatus: {
    const LSC_CACHE_OPTS opts = IGC::translateLSCCacheControlsEnum(static_cast<LSC_L1_L3_CC>(CurEnum),
                                                                   /*isLoad=*/true, &I, *pContext);
    const bool RIInvalid = (opts.l1 == LSC_CACHING_READINVALIDATE && opts.l3 == LSC_CACHING_CACHED);
    const auto &PI = pContext->platform.getPlatformInfo();
    const bool PvcXlA0DefaultInvalid = (opts.l1 == LSC_CACHING_DEFAULT && opts.l3 == LSC_CACHING_DEFAULT &&
                                        PI.eProductFamily == IGFX_PVC && PI.usRevId < REVISION_B);
    if (RIInvalid || PvcXlA0DefaultInvalid)
      FinalEnum = static_cast<unsigned>(LSC_L1C_WT_L3C_WB);
    break;
  }

  // (C) Operand atomic override (emitLSCAtomic, op5). InternalOnly regkey
  // LscAtomicCacheControlOverride, when set, replaces the operand cache control
  // with the regkey's IGC1 enum value.
  case GenISAIntrinsic::GenISA_LSCAtomicInts:
  case GenISAIntrinsic::GenISA_LSCAtomicFP32:
  case GenISAIntrinsic::GenISA_LSCAtomicFP64:
  case GenISAIntrinsic::GenISA_LSCAtomicBF16:
    break;

  default:
    break;
  }

  if (FinalEnum && *FinalEnum != CurEnum) {
    II->setOperand(*CacheOpIdx, ConstantInt::get(Type::getInt32Ty(I.getContext()), *FinalEnum));
    LLVM_DEBUG(dbgs() << "\t" << I.getOpcodeName() << ": operand " << *CacheOpIdx << " cache control rewritten "
                      << CurEnum << " -> " << *FinalEnum << "\n");
    return true;
  }
  return false;
}

// Non-operand atomics: scalar/typed atomic intrinsics that have no
// cache-control operand. They use LSC_DEFAULT_CACHING unless the InternalOnly
// LscAtomicCacheControlOverride regkey supplies an override. These operations
// are store-shaped for cache-control translation (isLoad=false).
bool LSCCacheHints::IsOverridableAtomic(const llvm::Instruction &I) {
  const GenIntrinsicInst *II = dyn_cast<GenIntrinsicInst>(&I);
  if (!II)
    return false;
  switch (II->getIntrinsicID()) {
  case GenISAIntrinsic::GenISA_intatomicraw:
  case GenISAIntrinsic::GenISA_floatatomicraw:
  case GenISAIntrinsic::GenISA_intatomicrawA64:
  case GenISAIntrinsic::GenISA_floatatomicrawA64:
  case GenISAIntrinsic::GenISA_icmpxchgatomicraw:
  case GenISAIntrinsic::GenISA_fcmpxchgatomicraw:
  case GenISAIntrinsic::GenISA_icmpxchgatomicrawA64:
  case GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64:
  case GenISAIntrinsic::GenISA_intatomicrawsinglelane:
  case GenISAIntrinsic::GenISA_intatomictyped:
  case GenISAIntrinsic::GenISA_icmpxchgatomictyped:
  case GenISAIntrinsic::GenISA_floatatomictyped:
  case GenISAIntrinsic::GenISA_fcmpxchgatomictyped:
    return true;
  default:
    return false;
  }
}

// Resolve the non-operand atomic cache control by carrying the
// LscAtomicCacheControlOverride decision as cache-hint metadata. When the
// override is unset (0), attach nothing so the default cache behavior remains in
// effect.
// SetInstructionCacheHint writes the backend-appropriate metadata using the
// store path (atomics are not load-shaped: IsLoadShapedIO returns false for
// them, so IsStore == true).
bool LSCCacheHints::ResolveAtomicCacheControl(llvm::Instruction &I) {
  (void)I;
  return false;
}

bool LSCCacheHints::UseRasterizerOrderedByteAddressBuffer(const ModuleMetaData *ModMD, llvm::Instruction &I) const {
  if (IGC_GET_FLAG_VALUE(RovOpt) == 0 || I.getNumOperands() == 0)
    return false;

  const std::vector<uint32_t> &ROV_RV = ModMD->RasterizerOrderedByteAddressBuffer;
  const PushInfo &pushInfo = ModMD->pushInfo;

  Value *src = IGC::TracePointerSource(I.getOperand(0));
  Argument *calleeArg = dyn_cast_or_null<Argument>(src);
  if (!calleeArg)
    return false;

  const unsigned calleeArgNo = calleeArg->getArgNo();
  for (const auto &Index : pushInfo.constantReg) {
    if (Index.second == calleeArgNo && std::find(ROV_RV.begin(), ROV_RV.end(), Index.first) != ROV_RV.end())
      return true;
  }
  return false;
}

bool LSCCacheHints::TryApplyGlobalOverride(const ModuleMetaData *ModMD, llvm::Instruction &I, bool &Changed) {
  bool IsTGM = false;
  if (GenIntrinsicInst *II = dyn_cast<GenIntrinsicInst>(&I)) {
    if (IsTGMIntrinsic(II->getIntrinsicID()))
      IsTGM = true;
  }

  // TGM cache override only applies on platforms that support non-default
  // LSC cache settings; this mirrors the historical gate in
  // EmitPass::translateLSCCacheControlsFromMetadata. Pre-Xe2 messages to
  // UGML/TGM require default cache settings.
  if (IsTGM) {
    const CodeGenContext *pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (!pContext->platform.supportsNonDefaultLSCCacheSetting())
      return false;
  }

  uint32_t CacheCtrl = 0;
  const bool IsLoad = IsLoadShapedIO(I);
  if (!TryOverrideCacheOpts(CacheCtrl, IsLoad, IsTGM, ModMD->m_CacheControlOption))
    return false;

  // Override wins over any pre-existing cache MD. Keep the target carrier in
  // place so a second pass run with the same override is an idempotent no-op;
  // only clear a legacy carrier that the active backend will not consume.
  Changed |= SetInstructionCacheHint(I, CacheCtrl, "global override", /*AllowOverwrite=*/true);
  return true;
}

bool LSCCacheHints::TryOverrideCacheOpts(uint32_t &CacheCtrl, bool IsLoad, bool IsTGM,
                                         const CacheControlOverride &CacheControlOption) {
  if (IsTGM && IsLoad) {
    CacheCtrl = (CacheControlOption.TgmLoadCacheControlOverride | IGC_GET_FLAG_VALUE(TgmLoadCacheControlOverride));
  } else if (IsTGM && !IsLoad) {
    CacheCtrl = CacheControlOption.TgmStoreCacheControlOverride | IGC_GET_FLAG_VALUE(TgmStoreCacheControlOverride);
  } else if (!IsTGM && IsLoad) {
    CacheCtrl = (CacheControlOption.LscLoadCacheControlOverride | IGC_GET_FLAG_VALUE(LscLoadCacheControlOverride));
  } else if (!IsTGM && !IsLoad) {
    CacheCtrl = CacheControlOption.LscStoreCacheControlOverride | IGC_GET_FLAG_VALUE(LscStoreCacheControlOverride);
  }
  return (CacheCtrl != 0);
}


} // namespace IGC
