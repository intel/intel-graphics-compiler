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

bool LSCCacheHints::runOnFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "\nRunning LSCCacheHints on function \"" << F.getName() << "\":\n");
  ModuleMetaData *ModMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  if (ModMD != nullptr) {
    for (auto &I : instructions(F)) {
      // This pass is restricted to IO-shaped instructions,
      // so we don't pollute unrelated IR with cache hints.
      if (IsCacheHintRelevantInstruction(I))
        VisitInstruction(ModMD, I);
    }
  }
  return false;
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

// Instructions whose cache hint may be steered by the global override.
// Mirrors the set of emission sites in EmitVISAPass that consume
// lsc.cache.ctrl via translateLSCCacheControlsFromMetadata: plain LSC
// loads/stores, raw indexed load/store, predicated load/store, typed
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
  if (IsTGMIntrinsic(II->getIntrinsicID()))
    return true;
  switch (II->getIntrinsicID()) {
  case GenISAIntrinsic::GenISA_simdBlockRead:
  case GenISAIntrinsic::GenISA_simdBlockReadBindless:
  case GenISAIntrinsic::GenISA_simdBlockWrite:
  case GenISAIntrinsic::GenISA_simdBlockWriteBindless:
    return true;
  default:
    return false;
  }
}

// True for the load-shaped half of IsCacheHintRelevantInstruction.
// Prefetches count as loads (they consume LscLoadCacheControlOverride
// and EmitVISAPass interprets their lsc.cache.ctrl with isLoad=true),
// so this can't be derived from the return type alone:
// CooperativeMatrixPrefetch returns void but is load-shaped.
// Caller must ensure I is in the set accepted by
// IsCacheHintRelevantInstruction; anything else returns false.
bool LSCCacheHints::IsLoadShapedIO(const llvm::Instruction &I) {
  if (isa<LoadInst>(&I) || isa<LdRawIntrinsic>(&I) || isa<PredicatedLoadIntrinsic>(&I))
    return true;
  const GenIntrinsicInst *II = dyn_cast<GenIntrinsicInst>(&I);
  if (!II)
    return false;
  switch (II->getIntrinsicID()) {
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

void LSCCacheHints::VisitInstruction(const ModuleMetaData *ModMD, llvm::Instruction &I) {
  // Global cache-control overrides take precedence over any pre-set cache
  // metadata or analysis-based defaults below.
  if (TryApplyGlobalOverride(ModMD, I))
    return;

  const bool HasLscCacheCtrl = (I.getMetadata("lsc.cache.ctrl") != nullptr);

  if (!HasLscCacheCtrl)
    SetupLscCacheCtrl(ModMD, I);
}


void LSCCacheHints::SetInstructionCacheHint(llvm::Instruction &I, const unsigned CacheHint,
                                            llvm::StringRef Reason) const {
  unsigned int NewCacheHint = CacheHint;
  const std::string EntryName = "lsc.cache.ctrl";
  IGC_ASSERT(I.getMetadata(EntryName) == nullptr);
  ConstantInt *CICC = ConstantInt::get(Type::getInt32Ty(I.getContext()), NewCacheHint);
  MDNode *node = MDNode::get(I.getContext(), ConstantAsMetadata::get(CICC));
  I.setMetadata(EntryName, node);
  LLVM_DEBUG(dbgs() << "\t" << I.getOpcodeName() << ": " << EntryName << " set to: " << NewCacheHint
                    << " due to: " << Reason << "\n");
}

void LSCCacheHints::SetupLscCacheCtrl(const ModuleMetaData *ModMD, llvm::Instruction &I) {
  const std::map<uint32_t, uint32_t> &ForceLscCacheList = ModMD->forceLscCacheList;
  const std::set<uint32_t> &RasterizerOrderedViews = ModMD->RasterizerOrderedViews;
  const CodeGenContext *pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  bool IsTGM = false;
  if (GenIntrinsicInst *II = dyn_cast<GenIntrinsicInst>(&I)) {
    if (IsTGMIntrinsic(II->getIntrinsicID()))
      IsTGM = true;
  }

  if (ShouldGenerateLSC_Duplicate(I, IsTGM) && (IGC_GET_FLAG_VALUE(RovOpt) & 2) &&
      UseRasterizerOrderedByteAddressBuffer(ModMD, I)) {
    SetInstructionCacheHint(I, LSC_L1UC_L3C_WB, "RovOpt");
    return;
  }
  if (I.getMetadata(LLVMContext::MD_nontemporal)) {
    SetInstructionCacheHint(I, LSC_L1UC_L3UC, "nontemporal hint");
    return;
  }
  // RT specific handling
  if (pContext->type == ShaderType::RAYTRACING_SHADER || pContext->hasSyncRTCalls(I.getFunction())) {
    // TODO:
  }
  for (unsigned int i = 0; i < I.getNumOperands(); i++) {
    auto Op = I.getOperand(i);
    if (Op->getType()->isPointerTy()) {
      const unsigned int AddrSpace = Op->getType()->getPointerAddressSpace();
      if (RasterizerOrderedViews.find(AddrSpace) != RasterizerOrderedViews.end()) {
        SetInstructionCacheHint(I, LSC_L1UC_L3C_WB, "accessing ROV");
        return;
      } else if (!ForceLscCacheList.empty()) {
        for (auto &l : ForceLscCacheList) {
          if (l.first == AddrSpace) {
            SetInstructionCacheHint(I, l.second, "addrspace matching ForceLscCacheList");
            return;
          }
        }
      } else if (AddrSpace == ADDRESS_SPACE_PRIVATE || AddrSpace == ADDRESS_SPACE_CONSTANT) {
        // TBD: Do we need this: SetInstructionCacheHint(I, LSC_L1C_WT_L3C_WB, "Priv/const addrspace")?
        // For IGC1 translateLSCCacheControlsFromMetadata sets the cacheOpts for private and
        // constant address spaces to: {CC, CC}/{WB, WB}, but the IGC2.0 is aware of these address
        // spaces and can handle these on its own if necessary, so my guess is, it can be skipped here.
      }
    }
  } // for getNumOperands()
}

// The following three functions are the duplicates of similarly named functions from
// EmitPass or CShader, they're marked with "_Duplicate" suffix. I hate to introduce
// them here, but I haven't found a better way to use their functionality here yet and
// I don't want to risk regressions caused by not replicating original code exactly,
// at least not now. But, it would be good to get rid of these from here eventually.

bool LSCCacheHints::ShouldGenerateLSC_Duplicate(llvm::Instruction &I, bool isTGM) const {
  CodeGenContext *pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  if (pContext->m_DriverInfo.SupportForceRouteAndCache() &&
      (!isTGM || pContext->platform.supportsNonDefaultLSCCacheSetting())) {
    // check if umd specified lsc caching mode and set the metadata if needed.
    if (I.getMetadata("lsc.cache.ctrl")) {
      // if umd force the caching mode, also assume it wants the resource to be in lsc.
      return true;
    }
  }

  if (auto result = CShader::shouldGenerateLSCQuery(*pContext, &I, SIMDMode::UNKNOWN); result != Tristate::Unknown)
    return (result == Tristate::True);

  // ensure both source and destination are not uniform
  Value *addrs = nullptr;
  if (GenIntrinsicInst *inst = dyn_cast<GenIntrinsicInst>(&I)) {
    addrs = inst->getOperand(0); // operand 0 is always addr for loads and stores
  } // else others?

  // The original shouldGenerateLSC from EmitPass checks for the uniformity of the address:
  // "...we can generate LSC only if it's not uniform (SIMD1) or A32..."
  // This analysis is not available here and we're going to assume that address is always not uniform.
  // This may cause potential issues here and there, and we may need to analyze them in the future.
  bool canGenerate = true;
  if (addrs) {
    bool isA32 = false; // TODO: see below
    if (PointerType *ptrType = dyn_cast<PointerType>(addrs->getType())) {
      isA32 = !IGC::isA64Ptr(ptrType, pContext);
    }
    // it was: "canGenerate &= isA32 || !GetSymbol(addrs)->IsUniform()";
    canGenerate &= isA32;

    // it was: "if (!isA32 && GetSymbol(addrs)->IsUniform())"
    if (!isA32) {
      // This is A64 and Uniform case. The LSC is not allowed.
      // However, before exit check the total bytes to be stored or loaded.
      if (TotalBytesToStoreOrLoad_Duplicate(&I) >= 4) {
        canGenerate = true;
      }
    }
  }
  return canGenerate;
}

uint32_t LSCCacheHints::TotalBytesToStoreOrLoad_Duplicate(llvm::Instruction *vectorLdStInst) const {
  if (dyn_cast<LoadInst>(vectorLdStInst) || dyn_cast<StoreInst>(vectorLdStInst)) {
    Type *Ty = nullptr;
    if (LoadInst *inst = dyn_cast<LoadInst>(vectorLdStInst)) {
      Ty = inst->getType();
    } else if (StoreInst *inst = dyn_cast<StoreInst>(vectorLdStInst)) {
      Value *storedVal = inst->getValueOperand();
      Ty = storedVal->getType();
    }
    if (Ty) {
      IGCLLVM::FixedVectorType *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
      Type *eltTy = VTy ? VTy->getElementType() : Ty;
      uint32_t eltBytes = GetScalarTypeSizeInRegister_Duplicate(eltTy);
      uint32_t elts = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;
      return (eltBytes * elts);
    }
  }
  return 0;
}

unsigned int LSCCacheHints::GetScalarTypeSizeInRegister_Duplicate(const llvm::Type *Ty) const {
  unsigned int sizeInBits = Ty->getScalarSizeInBits();
  if (Ty->isPtrOrPtrVectorTy()) {
    CodeGenContext *pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    sizeInBits = pContext->getRegisterPointerSizeInBits(Ty->getPointerAddressSpace());
  }
  return sizeInBits / 8;
}

bool LSCCacheHints::UseRasterizerOrderedByteAddressBuffer(const ModuleMetaData *ModMD, llvm::Instruction &I) const {
  if (IGC_GET_FLAG_VALUE(RovOpt) == 0 || I.getNumOperands() == 0)
    return false;

  bool isRov = false;
  std::vector<uint32_t> ROV_RV = ModMD->RasterizerOrderedByteAddressBuffer;

  unsigned calleeArgNo = 0;
  const PushInfo &pushInfo = ModMD->pushInfo;

  Value *src = IGC::TracePointerSource(I.getOperand(0));
  if (src) {
    if (Argument *calleeArg = dyn_cast<Argument>(src)) {
      calleeArgNo = calleeArg->getArgNo();
      for (auto index_it = pushInfo.constantReg.begin(); index_it != pushInfo.constantReg.end(); ++index_it) {
        if (index_it->second == calleeArgNo) {
          if (std::find(ROV_RV.begin(), ROV_RV.end(), index_it->first) != ROV_RV.end()) {
            isRov = true;
            break;
          }
        }
      }
    }
  }
  return isRov;
}

bool LSCCacheHints::TryApplyGlobalOverride(const ModuleMetaData *ModMD, llvm::Instruction &I) {
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

  // Override wins over any pre-existing cache MD; clear so the
  // SetInstructionCacheHint invariant ("no metadata set") holds.
  I.setMetadata("lsc.cache.ctrl", nullptr);
  SetInstructionCacheHint(I, CacheCtrl, "global override");
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
