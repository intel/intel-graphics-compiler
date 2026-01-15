/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <algorithm>
#include <vector>
#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
#include "common/LLVMUtils.h"
#include "Compiler/CISACodeGen/ConstantCoalescing.hpp"
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/KnownBits.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

/// @brief ConstantCoalescing merges multiple constant loads into one load
/// of larger quantity
/// - change to oword loads if the address is uniform
/// - change to gather4 or sampler loads if the address is not uniform

using namespace llvm;
using namespace IGC;
using IGCLLVM::getAlign;

// Register pass to igc-opt
#define PASS_FLAG "igc-constant-coalescing"
#define PASS_DESCRIPTION "Constant Coalescing"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ConstantCoalescing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ConstantCoalescing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

ConstantCoalescing::ConstantCoalescing() : FunctionPass(ID) {
  curFunc = NULL;
  irBuilder = NULL;
  initializeConstantCoalescingPass(*PassRegistry::getPassRegistry());
}

bool ConstantCoalescing::runOnFunction(Function &func) {
  CodeGenContextWrapper *pCtxWrapper = &getAnalysis<CodeGenContextWrapper>();
  m_TT = &getAnalysis<TranslationTable>();

  m_ctx = pCtxWrapper->getCodeGenContext();
  if (isOptDisabledForFunction(m_ctx->getModuleMetaData(), getPassName(), &func))
    return false;

  IGCMD::MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  if (pMdUtils->findFunctionsInfoItem(&func) != pMdUtils->end_FunctionsInfo()) {
    ProcessFunction(&func);
  }
  return true;
}

void ConstantCoalescing::ProcessFunction(Function *function) {
  curFunc = function;
  irBuilder = new IRBuilderWrapper(function->getContext(), m_TT);
  dataLayout = &function->getParent()->getDataLayout();
  wiAns = &getAnalysis<WIAnalysis>();
  m_BaseOffsets.clear();

  // clean up unnecessary lcssa-phi
  for (Function::iterator I = function->begin(), E = function->end(); I != E; ++I) {
    if (I->getSinglePredecessor()) {
      for (BasicBlock::iterator BBI = I->begin(), BBE = I->end(); BBI != BBE;) {
        Instruction *PHI = &(*BBI++);
        if (!isa<PHINode>(PHI)) {
          break;
        }
        IGC_ASSERT(PHI->getNumOperands() <= 1);
        Value *src = PHI->getOperand(0);
        if (wiAns->whichDepend(src) == wiAns->whichDepend(PHI)) {
          PHI->replaceAllUsesWith(src);
          PHI->eraseFromParent();
        }
      }
    }
  }

  // get the dominator-tree to traverse
  DominatorTree &dom_tree = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  // separate the loads into 3 streams to speed up process
  std::vector<BufChunk *> dircb_owloads;
  std::vector<BufChunk *> indcb_owloads;
  std::vector<BufChunk *> indcb_gathers;

  for (df_iterator<DomTreeNode *> dom_it = df_begin(dom_tree.getRootNode()), dom_end = df_end(dom_tree.getRootNode());
       dom_it != dom_end; ++dom_it) {
    BasicBlock *cur_blk = dom_it->getBlock();
    // pop out all the chunks that do not dominate this block
    while (!dircb_owloads.empty()) {
      BufChunk *top_chunk = dircb_owloads.back();
      BasicBlock *top_blk = top_chunk->chunkIO->getParent();
      if (dom_tree.dominates(top_blk, cur_blk))
        break;
      dircb_owloads.pop_back();
      delete top_chunk;
    }
    while (!indcb_owloads.empty()) {
      BufChunk *top_chunk = indcb_owloads.back();
      BasicBlock *top_blk = top_chunk->chunkIO->getParent();
      if (dom_tree.dominates(top_blk, cur_blk))
        break;
      indcb_owloads.pop_back();
      delete top_chunk;
    }
    while (!indcb_gathers.empty()) {
      BufChunk *top_chunk = indcb_gathers.back();
      BasicBlock *top_blk = top_chunk->chunkIO->getParent();
      if (dom_tree.dominates(top_blk, cur_blk))
        break;
      indcb_gathers.pop_back();
      delete top_chunk;
    }
    // scan and rewrite cb-load in this block
    ProcessBlock(cur_blk, dircb_owloads, indcb_owloads, indcb_gathers);
    CleanupExtract(cur_blk);
    VectorizePrep(cur_blk);
  }

  // clean up
  while (!dircb_owloads.empty()) {
    BufChunk *top_chunk = dircb_owloads.back();
    dircb_owloads.pop_back();
    delete top_chunk;
  }
  while (!indcb_owloads.empty()) {
    BufChunk *top_chunk = indcb_owloads.back();
    indcb_owloads.pop_back();
    delete top_chunk;
  }
  while (!indcb_gathers.empty()) {
    BufChunk *top_chunk = indcb_gathers.back();
    indcb_gathers.pop_back();
    delete top_chunk;
  }
  curFunc = nullptr;
  delete irBuilder;
  irBuilder = nullptr;
}

static void checkInsertExtractMatch(InsertElementInst *insertInst, Value *base, SmallVector<bool, 4> &mask) {
  auto vectorBase = insertInst->getOperand(0);
  auto extractElt = dyn_cast<ExtractElementInst>(insertInst->getOperand(1));
  auto index = dyn_cast<ConstantInt>(insertInst->getOperand(2));
  if (!index || !extractElt || extractElt->getOperand(0) != base) {
    return;
  }
  int indexVal = (int)index->getZExtValue();
  auto extractIndex = dyn_cast<ConstantInt>(extractElt->getOperand(1));
  if (!extractIndex || indexVal != extractIndex->getZExtValue()) {
    return;
  }
  if (mask[indexVal]) {
    return;
  }
  mask[indexVal] = true;
  if (auto prevInsert = dyn_cast<InsertElementInst>(vectorBase)) {
    checkInsertExtractMatch(prevInsert, base, mask);
  }
};

static bool canReplaceInsert(InsertElementInst *insertElt) {
  IGCLLVM::FixedVectorType *VTy = cast<IGCLLVM::FixedVectorType>(insertElt->getOperand(0)->getType());
  ConstantInt *index = dyn_cast<ConstantInt>(insertElt->getOperand(2));
  if (!index || index->getZExtValue() != VTy->getNumElements() - 1) {
    return false;
  }
  auto extractElt = dyn_cast<ExtractElementInst>(insertElt->getOperand(1));
  if (!extractElt || extractElt->getOperand(0)->getType() != insertElt->getType()) {
    return false;
  }
  auto vectorBase = extractElt->getOperand(0);

  int size = (int)VTy->getNumElements();
  llvm::SmallVector<bool, 4> mask;
  for (int i = 0; i < size; ++i) {
    mask.push_back(false);
  }
  checkInsertExtractMatch(insertElt, vectorBase, mask);
  for (int i = 0; i < size; ++i) {
    if (!mask[i]) {
      return false;
    }
  }
  return true;
}

// pattern match away redundant insertElt/extractElt pairs introduced by coalescing
//
//  %26 = load <2 x float>, <2 x float> addrspace(65546)* %chunkPtr36, align 4
//  % 27 = extractelement <2 x float> % 26, i32 1
//  % 28 = extractelement <2 x float> % 26, i32 0
//  %29 = insertelement <2 x float> undef, float %28, i32 0
//  %30 = insertelement <2 x float> % 29, float %27, i32 1
//  We can fold the extract/insert and directly use the %26
bool ConstantCoalescing::CleanupExtract(llvm::BasicBlock *bb) {
  bool changed = false;
  for (auto I = bb->rbegin(), IEnd = bb->rend(); I != IEnd; ++I) {
    // we assume the last element is also the last one to be inserted to the vector
    if (auto insertElt = dyn_cast<InsertElementInst>(&(*I))) {
      if (canReplaceInsert(insertElt)) {
        changed = true;
        // insertElt->dump();
        auto vectorBase = dyn_cast<ExtractElementInst>(insertElt->getOperand(1))->getOperand(0);
        insertElt->replaceAllUsesWith(vectorBase);
      }
    }
  }

  return changed;
}

// reorder the extract and its use so that vISA can merge single movs into vector movs
//
// %43 = extractelement <4 x float> % 42, i32 2
// %44 = extractelement <4 x float> % 42, i32 1
// %45 = extractelement <4 x float> % 42, i32 0
// ...
// %src0_s111 = fptrunc float %45 to half
// ...
// %src0_s112 = fptrunc float %44 to half
// ...
// %src0_s113 = fptrunc float %43 to half
//
//        to ordered and adjacent sequence:
//
// %43 = extractelement <4 x float> % 42, i32 0
// %src0_s111 = fptrunc float %43 to half
// %44 = extractelement <4 x float> % 42, i32 1
// %src0_s112 = fptrunc float %44 to half
// %45 = extractelement <4 x float> % 42, i32 2
// %src0_s113 = fptrunc float %45 to half
void ConstantCoalescing::VectorizePrep(llvm::BasicBlock *bb) {
  uint32_t srcNElts = 0;
  for (auto I = bb->rbegin(), IEnd = bb->rend(); I != IEnd; ++I) {
    if (LoadInst *load = dyn_cast<LoadInst>(&(*I))) {
      if (load->getType()->isVectorTy() && wiAns->isUniform(load)) {
        srcNElts = (uint32_t)cast<IGCLLVM::FixedVectorType>(load->getType())->getNumElements();
        DenseMap<uint64_t, Instruction *> extractElementMap;

        for (auto iter = load->user_begin(); iter != load->user_end(); iter++) {
          ExtractElementInst *extractElt = dyn_cast<ExtractElementInst>(*iter);
          if (extractElt && extractElt->getParent() == bb) {
            if (ConstantInt *C = dyn_cast<ConstantInt>(extractElt->getOperand(1))) {
              extractElementMap[C->getZExtValue()] = extractElt;
            }
          }
        }

        for (int ie = srcNElts - 1; ie >= 0; ie--) {
          if (Instruction *extEle = extractElementMap[ie]) {
            if (extEle->hasOneUse() && safeToMoveInstUp(extEle, load)) {
              extEle->moveBefore(load->getNextNode());
              Instruction *extractUse = cast<Instruction>(*extEle->user_begin());
              if (safeToMoveInstUp(extractUse, extEle)) {
                extractUse->moveBefore(extEle->getNextNode());
              }
            }
          }
        }
      }
    }
  }
}

// check if it is safe to move "inst" to right after "newLocation"
bool ConstantCoalescing::safeToMoveInstUp(Instruction *inst, Instruction *newLocation) {
  if (inst->mayHaveSideEffects() || inst->mayReadFromMemory() || inst->getParent() != newLocation->getParent() ||
      isa<PHINode>(inst)) {
    return false;
  }

  Instruction *current = inst;
  while (current && current != newLocation) {
    if (current->getParent() != newLocation->getParent()) {
      break;
    }

    for (uint i = 0; i < inst->getNumOperands(); i++) {
      if (inst->getOperand(i) == current) {
        return false;
      }
    }
    current = current->getPrevNode();
  }

  return true;
}

// 32 dwords, meaning 8 owords; This would allow us to use 4 GRFs.
// Hardware limit is 8 GRFs but could build pressure on RA. Hence keeping it to 4 for now.
#define MAX_OWLOAD_SIZE (32 * 4)

// Maximum size of vector in elements. This is currently limited by the the size
// of the extract mask (see emitVectorBitcast).
#define MAX_VECTOR_NUM_ELEMENTS 32

bool ConstantCoalescing::isProfitableLoad(const Instruction *I, uint32_t &MaxEltPlus) const {
  auto *LoadTy = I->getType();
  MaxEltPlus = 1;
  if (LoadTy->isVectorTy()) {
    // \todo, another parameter to tune
    uint32_t MaxVectorInput = (isa<LoadInst>(I) && wiAns->isUniform(I)) ? 16 : 4;

    if (cast<IGCLLVM::FixedVectorType>(LoadTy)->getNumElements() > MaxVectorInput)
      return false;

    MaxEltPlus = CheckVectorElementUses(I);
    // maxEltPlus == 0, means that vector may be used with index or as-vector,
    // skip it for now
    if (MaxEltPlus == 0)
      return false;
  }

  return true;
}

void ConstantCoalescing::ProcessBlock(BasicBlock *blk, std::vector<BufChunk *> &dircb_owloads,
                                      std::vector<BufChunk *> &indcb_owloads, std::vector<BufChunk *> &indcb_gathers) {
  // get work-item analysis, need to update uniformness information
  for (BasicBlock::iterator BBI = blk->begin(), BBE = blk->end(); BBI != BBE; ++BBI) {
    // skip dead instructions
    if (BBI->use_empty())
      continue;

    // bindless case
    if (auto *ldRaw = dyn_cast<LdRawIntrinsic>(BBI)) {
      bool directIdx = false;
      unsigned int bufId = 0;
      BufferType bufType =
          DecodeAS4GFXResource(ldRaw->getResourceValue()->getType()->getPointerAddressSpace(), directIdx, bufId);


      uint offsetInBytes = 0;
      Value *baseOffsetInBytes = nullptr;
      ExtensionKind Extension = EK_NotExtended;
      if (ConstantInt *offsetConstVal = dyn_cast<ConstantInt>(ldRaw->getOffsetValue())) {
        offsetInBytes = int_cast<uint>(offsetConstVal->getZExtValue());
      } else {
        baseOffsetInBytes = SimpleBaseOffset(ldRaw->getOffsetValue(), offsetInBytes, Extension);
      }
      if ((int32_t)offsetInBytes >= 0) {
        uint maxEltPlus = 1;
        if (!isProfitableLoad(ldRaw, maxEltPlus)) {
          continue;
        }
        uint addrSpace = ldRaw->getResourceValue()->getType()->getPointerAddressSpace();
        if (wiAns->isUniform(ldRaw)) {
          MergeUniformLoad(ldRaw, ldRaw->getResourceValue(), addrSpace, baseOffsetInBytes, offsetInBytes, maxEltPlus,
                           Extension, baseOffsetInBytes ? indcb_owloads : dircb_owloads);
        } else if (IsUntypedBuffer(bufType)) {
          if (UsesTypedConstantBuffer(m_ctx, bufType)) {
            ScatterToSampler(ldRaw, ldRaw->getResourceValue(), addrSpace, baseOffsetInBytes, offsetInBytes,
                             indcb_gathers);
          } else if (IGC_IS_FLAG_DISABLED(DisableConstantCoalescingOfStatefulNonUniformLoads)) {
            // TODO: remove this condition
            if (!IsWritableBuffer(bufType)) {
              MergeScatterLoad(ldRaw, ldRaw->getResourceValue(), addrSpace, baseOffsetInBytes, offsetInBytes,
                               maxEltPlus, Extension, indcb_gathers);
            }
          }
        } else if (IsTypedBuffer(bufType) && IGC_IS_FLAG_ENABLED(EnableTextureLoadCoalescing)) {
          MergeScatterLoad(ldRaw, ldRaw->getResourceValue(), addrSpace, baseOffsetInBytes, offsetInBytes, maxEltPlus,
                           Extension, indcb_gathers);
        }
      }
      continue;
    }

    auto *LI = dyn_cast<LoadInst>(BBI);
    // skip load on struct or array type
    if (!LI || LI->getType()->isAggregateType()) {
      continue;
    }
    Type *loadType = LI->getType();
    Type *elemType = loadType->getScalarType();
    // right now, only work on load with dword element-type
    if (elemType->getPrimitiveSizeInBits() != SIZE_DWORD * 8) {
      continue;
    }

    // stateless path: use int2ptr, and support vector-loads
    uint addrSpace = LI->getPointerAddressSpace();
    if (addrSpace == ADDRESS_SPACE_CONSTANT) {
      // another limit: load has to be dword-aligned
      if (IGCLLVM::getAlignmentValue(LI) % 4)
        continue;
      uint maxEltPlus = 1;
      if (!isProfitableLoad(LI, maxEltPlus))
        continue;

      Value *buf_idxv = nullptr;
      Value *elt_idxv = nullptr;
      uint offsetInBytes = 0;
      ExtensionKind Extension = EK_NotExtended;
      if (DecomposePtrExp(LI->getPointerOperand(), buf_idxv, elt_idxv, offsetInBytes, Extension)) {
        // TODO: Disabling constant coalescing when we see that the offset to the constant buffer is negtive
        // As we handle all negative offsets as uint and some arithmetic operations do not work well. Needs more
        // detailed fix
        if ((int32_t)offsetInBytes >= 0) {
          if (wiAns->isUniform(LI)) { // uniform
            if (elt_idxv)
              MergeUniformLoad(LI, buf_idxv, addrSpace, elt_idxv, offsetInBytes, maxEltPlus, Extension, indcb_owloads);
            else
              MergeUniformLoad(LI, buf_idxv, addrSpace, nullptr, offsetInBytes, maxEltPlus, Extension, dircb_owloads);
          } else { // not uniform
            MergeScatterLoad(LI, buf_idxv, addrSpace, elt_idxv, offsetInBytes, maxEltPlus, Extension, indcb_gathers);
          }
        }
      }
      continue;
    }

    uint bufId = 0;
    Value *elt_ptrv = nullptr;
    // \todo, handle dynamic buffer-indexing if necessary
    BufferType bufType = BUFFER_TYPE_UNKNOWN;
    if (IsReadOnlyLoadDirectCB(LI, bufId, elt_ptrv, bufType)) {
      uint maxEltPlus = 1;
      if (!isProfitableLoad(LI, maxEltPlus))
        continue;
      if (isa<ConstantPointerNull>(elt_ptrv)) {
        MergeUniformLoad(LI, nullptr, addrSpace, nullptr, 0, maxEltPlus, EK_NotExtended, dircb_owloads);
      } else if (isa<IntToPtrInst>(elt_ptrv)) {
        Value *elt_idxv = cast<Instruction>(elt_ptrv)->getOperand(0);
        ConstantInt *offsetConstant = dyn_cast<ConstantInt>(elt_idxv);
        if (offsetConstant) { // direct access
          uint offsetInBytes = (uint)offsetConstant->getZExtValue();
          // TODO: Disabling constant coalescing when we see that the offset to the constant buffer is negtive
          // As we handle all negative offsets as uint and some arithmetic operations do not work well. Needs more
          // detailed fix
          if ((int32_t)offsetInBytes >= 0) {
            MergeUniformLoad(LI, nullptr, addrSpace, nullptr, offsetInBytes, maxEltPlus, EK_NotExtended, dircb_owloads);
          }
        } else { // indirect access
          uint offsetInBytes = 0;
          ExtensionKind Extension = EK_NotExtended;
          elt_idxv = SimpleBaseOffset(elt_idxv, offsetInBytes, Extension);
          // TODO: Disabling constant coalescing when we see that the offset to the constant buffer is negtive
          // As we handle all negative offsets as uint and some arithmetic operations do not work well. Needs more
          // detailed fix
          if ((int32_t)offsetInBytes >= 0) {
            if (wiAns->isUniform(LI)) { // uniform
              MergeUniformLoad(LI, nullptr, addrSpace, elt_idxv, offsetInBytes, maxEltPlus, Extension, indcb_owloads);
            } else if (bufType == CONSTANT_BUFFER) { // not uniform
              if (UsesTypedConstantBuffer(m_ctx, bufType)) {
                ScatterToSampler(LI, nullptr, addrSpace, elt_idxv, offsetInBytes, indcb_gathers);
              } else if (IGC_IS_FLAG_DISABLED(DisableConstantCoalescingOfStatefulNonUniformLoads)) {
                MergeScatterLoad(LI, nullptr, addrSpace, elt_idxv, offsetInBytes, maxEltPlus, Extension, indcb_gathers);
              }
            }
          }
        }
      }
    } // end of if gfx cbload handling
  } // loop over inst in block
}

bool ConstantCoalescing::profitableChunkSize(uint32_t ub, uint32_t lb, uint32_t eltSizeInBytes) {
  IGC_ASSERT(ub >= lb);
  return profitableChunkSize(ub - lb, eltSizeInBytes);
}

bool ConstantCoalescing::profitableChunkSize(uint32_t chunkSize, uint32_t eltSizeInBytes) {
  // limit the size to one GRF due to scalar-RA limitation
  if (IGC_GET_FLAG_VALUE(EnableScalarPipe))
    return chunkSize * eltSizeInBytes <= 64 && chunkSize <= MAX_VECTOR_NUM_ELEMENTS;
  return chunkSize * eltSizeInBytes <= MAX_OWLOAD_SIZE && chunkSize <= MAX_VECTOR_NUM_ELEMENTS;
}

/// check if two access have the same buffer-base
bool ConstantCoalescing::CompareBufferBase(const Value *bufIdxV1, uint addrSpace1, const Value *bufIdxV2,
                                           uint addrSpace2) {
  if (bufIdxV1 == bufIdxV2) {
    return (addrSpace1 == addrSpace2);
  }
  if (bufIdxV1 && bufIdxV2) {
    // both are valid value, and they are not equal
    if (isa<PtrToIntInst>(bufIdxV1))
      bufIdxV1 = dyn_cast<PtrToIntInst>(bufIdxV1)->getOperand(0);
    if (isa<PtrToIntInst>(bufIdxV2))
      bufIdxV2 = dyn_cast<PtrToIntInst>(bufIdxV2)->getOperand(0);
    return (bufIdxV1 == bufIdxV2);
  }
  return false;
}

bool ConstantCoalescing::IsDwordAligned(Value *val) const {
  KnownBits knownBits = computeKnownBits(val, *dataLayout);
  return knownBits.countMinTrailingZeros() >= 2;
}

// VectorProcess pass requires that:
// - if a vector load size is greater than 4 bytes, then the size of the loaded
//   vector must be a multiple of DWORD.
// - if a vector load size is smaller than 4 bytes, then the size of the loaded
//   vector must be 1 or 2
inline uint RoundChunkSize(uint numElements, uint elementSizeInBytes) {
  uint32_t vectorSizeInBytes = numElements * elementSizeInBytes;
  if (vectorSizeInBytes > 4 && (vectorSizeInBytes % 4) != 0) {
    return iSTD::RoundNonPow2(vectorSizeInBytes, 4) / elementSizeInBytes;
  } else if (vectorSizeInBytes < 4 && numElements == 3) {
    return 4;
  }
  return numElements;
}

void ConstantCoalescing::MergeScatterLoad(Instruction *load, Value *bufIdxV, uint addrSpace, Value *eltIdxV,
                                          uint offsetInBytes, uint maxEltPlus, const ExtensionKind &Extension,
                                          std::vector<BufChunk *> &chunk_vec) {
  const uint scalarSizeInBytes = load->getType()->getScalarSizeInBits() / 8;
  const alignment_t alignment = GetAlignment(load);

  IGC_ASSERT((offsetInBytes % scalarSizeInBytes) == 0);
  const uint eltid = offsetInBytes / scalarSizeInBytes;

  // Current assumption is that a chunk start needs to be DWORD aligned. In
  // the future we can consider adding support for merging 4 bytes or
  // 2 i16s/halfs into a single non-aligned DWORD.
  const bool isDwordAligned =
      ((offsetInBytes % 4) == 0 && (alignment >= 4 || eltIdxV == nullptr || IsDwordAligned(eltIdxV)));

  BufChunk *cov_chunk = nullptr;
  for (std::vector<BufChunk *>::reverse_iterator rit = chunk_vec.rbegin(), rie = chunk_vec.rend(); rit != rie; ++rit) {
    BufChunk *cur_chunk = *rit;
    if (CompareBufferBase(cur_chunk->bufIdxV, cur_chunk->addrSpace, bufIdxV, addrSpace) &&
        cur_chunk->baseIdxV == eltIdxV &&
        cur_chunk->chunkIO->getType()->getScalarType() == load->getType()->getScalarType() &&
        CompareMetadata(cur_chunk->chunkIO, load) && !CheckForAliasingWrites(addrSpace, cur_chunk->chunkIO, load)) {
      uint lb = std::min(eltid, cur_chunk->chunkStart);
      uint ub = std::max(eltid + maxEltPlus, cur_chunk->chunkStart + cur_chunk->chunkSize);
      static_assert(MAX_VECTOR_NUM_ELEMENTS >= SIZE_OWORD, "Code below may need an update");
      // if input load is not DWORD aligned it can only be appended to an aligned chunk
      if (((ub - lb) * scalarSizeInBytes) <= SIZE_OWORD && (ub - lb) <= MAX_VECTOR_NUM_ELEMENTS &&
          (isDwordAligned || eltid >= cur_chunk->chunkStart)) {
        // Since the algorithm allows changing chunk's starting point as
        // well as the size it is possible that a load "matches"
        // multiple chunks, e.g., a load at offset 1 (in elements) would
        // potentially "match" a chunk at offset 0 and a chunk at offset
        // 4. Compare chunks' starting points and avoid creating
        // potentially overlapping chunks. The condition assumes that
        // in most cases shaders read constant buffer data pieces that
        // are OWORD aligned.

        // TODO: we need to consider reworking this code to first gather
        // information about loaded data (starting offsets and sizes)
        // and then split loaded data into chunks.
        const uint owordSizeInElems = SIZE_OWORD / scalarSizeInBytes;
        if (!cov_chunk || (eltid / owordSizeInElems == cur_chunk->chunkStart / owordSizeInElems)) {
          cov_chunk = cur_chunk;
        }
      }
    }
  }

  if (!cov_chunk) {
    if (isDwordAligned) {
      cov_chunk = new BufChunk();
      cov_chunk->bufIdxV = bufIdxV;
      cov_chunk->addrSpace = addrSpace;
      cov_chunk->baseIdxV = eltIdxV;
      cov_chunk->elementSize = scalarSizeInBytes;
      cov_chunk->chunkStart = eltid;
      cov_chunk->chunkSize = RoundChunkSize(maxEltPlus, scalarSizeInBytes);
      const alignment_t chunkAlignment = std::max<alignment_t>(alignment, 4);
      cov_chunk->chunkIO = CreateChunkLoad(load, cov_chunk, eltid, chunkAlignment, Extension);

      // Update load alignment if needed, set it to DWORD aligned
      if (alignment < 4) {
        SetAlignment(load, 4);
      }

      chunk_vec.push_back(cov_chunk);
    }
  } else if (!cov_chunk->chunkIO->getType()->isVectorTy()) {
    // combine the initial scalar loads with this incoming load (which can be a vector-load),
    // then add extracts
    CombineTwoLoads(cov_chunk, load, eltid, maxEltPlus, Extension);
  } else if (load->getType()->isVectorTy()) {
    // just to modify all the extract, and connect it to the chunk-load
    uint lb = std::min(eltid, cov_chunk->chunkStart);
    uint ub = std::max(eltid + maxEltPlus, cov_chunk->chunkStart + cov_chunk->chunkSize);
    uint start_adj = cov_chunk->chunkStart - lb;
    uint size_adj = RoundChunkSize(ub - lb, cov_chunk->elementSize) - cov_chunk->chunkSize;
    if (start_adj == 0) {
      if (size_adj) {
        EnlargeChunk(cov_chunk, size_adj);
      }
    } else {
      AdjustChunk(cov_chunk, start_adj, size_adj, Extension);
    }
    MoveExtracts(cov_chunk, load, (eltid - cov_chunk->chunkStart));
  } else {
    Instruction *splitter = nullptr;
    uint start_adj = 0;
    uint size_adj = 0;
    if (eltid < cov_chunk->chunkStart) {
      start_adj = cov_chunk->chunkStart - eltid;
      size_adj = RoundChunkSize(start_adj, cov_chunk->elementSize);
    } else if (eltid >= cov_chunk->chunkStart + cov_chunk->chunkSize) {
      size_adj = RoundChunkSize(eltid - cov_chunk->chunkStart + 1, cov_chunk->elementSize) - cov_chunk->chunkSize;
    }

    if (start_adj == 0 && size_adj == 0) {
      splitter = FindOrAddChunkExtract(cov_chunk, eltid);
    } else if (start_adj > 0) {
      splitter = AdjustChunkAddExtract(cov_chunk, start_adj, size_adj, eltid, Extension);
    } else if (size_adj > 0) {
      splitter = EnlargeChunkAddExtract(cov_chunk, size_adj, eltid);
    }
    wiAns->incUpdateDepend(splitter, WIAnalysis::RANDOM);
    load->replaceAllUsesWith(splitter);
  }
}

Value *ConstantCoalescing::FormChunkAddress(BufChunk *chunk, const ExtensionKind &Extension) {
  IGC_ASSERT(nullptr != chunk);
  IGC_ASSERT_MESSAGE((chunk->bufIdxV || chunk->baseIdxV), "at least one!");
  WIAnalysis::WIDependancy uniformness =
      chunk->bufIdxV ? wiAns->whichDepend(chunk->bufIdxV) : wiAns->whichDepend(chunk->baseIdxV);
  Value *eac = chunk->baseIdxV;
  if (chunk->chunkStart && chunk->baseIdxV) {
    IGC_ASSERT(chunk->baseIdxV->getType()->isIntegerTy());
    Value *cv_start = ConstantInt::get(chunk->baseIdxV->getType(), chunk->chunkStart * chunk->elementSize);
    eac = irBuilder->CreateAdd(chunk->baseIdxV, cv_start);
    wiAns->incUpdateDepend(eac, wiAns->whichDepend(chunk->baseIdxV));
    if (!wiAns->isUniform(chunk->baseIdxV)) {
      uniformness = WIAnalysis::RANDOM;
    }
  }
  Value *bufsrc = chunk->bufIdxV ? chunk->bufIdxV : ConstantInt::get(chunk->baseIdxV->getType(), 0);
  if (bufsrc->getType()->isPointerTy()) {
    Type *ptrIntTy = dataLayout->getIntPtrType(cast<PointerType>(chunk->bufIdxV->getType()));
    bufsrc = irBuilder->CreatePtrToInt(chunk->bufIdxV, ptrIntTy);
    wiAns->incUpdateDepend(bufsrc, wiAns->whichDepend(chunk->bufIdxV));
  } else {
    IGC_ASSERT(bufsrc->getType()->isIntegerTy());
  }
  if (eac) {
    if (eac->getType()->getPrimitiveSizeInBits() < bufsrc->getType()->getPrimitiveSizeInBits()) {
      if (Extension == EK_SignExt)
        eac = irBuilder->CreateSExt(eac, bufsrc->getType());
      else
        eac = irBuilder->CreateZExt(eac, bufsrc->getType());
      wiAns->incUpdateDepend(eac, uniformness);
    }
    IGC_ASSERT(eac->getType() == bufsrc->getType());
    eac = irBuilder->CreateAdd(bufsrc, eac);
    wiAns->incUpdateDepend(eac, uniformness);
  } else if (chunk->chunkStart) {
    eac = ConstantInt::get(bufsrc->getType(), chunk->chunkStart * chunk->elementSize);
    eac = irBuilder->CreateAdd(bufsrc, eac);
    wiAns->incUpdateDepend(eac, uniformness);
  } else {
    eac = bufsrc;
  }
  return eac;
}

void ConstantCoalescing::CombineTwoLoads(BufChunk *cov_chunk, Instruction *load, uint eltid, uint numelt,
                                         const ExtensionKind &Extension) {
  uint eltid0 = cov_chunk->chunkStart;
  uint lb = std::min(eltid0, eltid);
  uint ub = std::max(eltid0, eltid + numelt - 1);
  cov_chunk->chunkStart = lb;
  cov_chunk->chunkSize = RoundChunkSize(ub - lb + 1, cov_chunk->elementSize);
  Instruction *load0 = cov_chunk->chunkIO;
  // remove redundant load
  if (cov_chunk->chunkSize <= 1) {
    load->replaceAllUsesWith(load0);
    return;
  }
  Type *vty = IGCLLVM::FixedVectorType::get(cov_chunk->chunkIO->getType()->getScalarType(), cov_chunk->chunkSize);
  irBuilder->SetInsertPoint(load0);
  if (isa<LoadInst>(cov_chunk->chunkIO)) {
    Value *addr_ptr = cov_chunk->chunkIO->getOperand(0);
    IGC_ASSERT(nullptr != addr_ptr);
    IGC_ASSERT(nullptr != (cast<PointerType>(addr_ptr->getType())));
    unsigned addrSpace = (cast<PointerType>(addr_ptr->getType()))->getAddressSpace();

    if (addrSpace == ADDRESS_SPACE_CONSTANT) {
      // OCL path
      IGC_ASSERT(isa<IntToPtrInst>(addr_ptr) || isa<BitCastInst>(addr_ptr) ||
                 cast<GetElementPtrInst>(addr_ptr)->hasAllZeroIndices());
      Value *eac = cast<Instruction>(addr_ptr)->getOperand(0);
      // non-uniform, address must be non-uniform
      // modify the address calculation if the chunk-start is changed
      if (eltid0 != cov_chunk->chunkStart) {
        eac = FormChunkAddress(cov_chunk, Extension);
      }
      // new IntToPtr and new load
      // cannot use irbuilder to create IntToPtr. It may create ConstantExpr instead of instruction
      auto *ptrcast = CastInst::CreateBitOrPointerCast(eac, PointerType::get(vty, addrSpace), "twoScalar", load0);
      m_TT->RegisterNewValueAndAssignID(ptrcast);
      wiAns->incUpdateDepend(ptrcast, WIAnalysis::RANDOM);
      cov_chunk->chunkIO = irBuilder->CreateLoad(vty, ptrcast, false);
      wiAns->incUpdateDepend(cov_chunk->chunkIO, WIAnalysis::RANDOM);
    } else {
      IGC_ASSERT(isa<IntToPtrInst>(addr_ptr));
      addr_ptr->mutateType(PointerType::get(vty, addrSpace));
      cov_chunk->chunkIO = irBuilder->CreateLoad(cov_chunk->chunkIO->getType(), addr_ptr, false);
      wiAns->incUpdateDepend(cov_chunk->chunkIO, WIAnalysis::RANDOM);
      // modify the address calculation if the chunk-start is changed
      if (eltid0 != cov_chunk->chunkStart) {
        IGC_ASSERT(cov_chunk->baseIdxV);
        // src0 is the buffer base pointer, src1 is the address calculation
        Value *eac = cast<Instruction>(addr_ptr)->getOperand(0);
        IGC_ASSERT(isa<Instruction>(eac));
        if (cast<Instruction>(eac)->getOpcode() == Instruction::Add ||
            cast<Instruction>(eac)->getOpcode() == Instruction::Or) {
          Value *cv_start = ConstantInt::get(irBuilder->getInt32Ty(), cov_chunk->chunkStart * cov_chunk->elementSize);
          cast<Instruction>(eac)->setOperand(1, cv_start);
        }
      }
    }
  } else {
    IGC_ASSERT(isa<LdRawIntrinsic>(load0));
    IGC_ASSERT(isa<LdRawIntrinsic>(load));
    LdRawIntrinsic *ldRaw0 = cast<LdRawIntrinsic>(load0);
    LdRawIntrinsic *ldRaw1 = cast<LdRawIntrinsic>(load);
    IGC_ASSERT(ldRaw0->getResourceValue()->getType() == ldRaw1->getResourceValue()->getType());
    Type *types[] = {
        vty,
        ldRaw0->getResourceValue()->getType(),
    };
    Function *ldRawFn =
        GenISAIntrinsic::getDeclaration(curFunc->getParent(), GenISAIntrinsic::GenISA_ldrawvector_indexed, types);

    Value *offsetInBuffer = ldRaw0->getOffsetValue();
    if (eltid0 != cov_chunk->chunkStart) {
      // Chunk start was updated, need to create new offset in buffer.
      Value *chunkStartOffset = irBuilder->getInt32(cov_chunk->chunkStart * cov_chunk->elementSize);
      Instruction *offsetInBufferInst = dyn_cast<Instruction>(offsetInBuffer);
      if (cov_chunk->baseIdxV == nullptr) {
        offsetInBuffer = chunkStartOffset;
      } else if (offsetInBufferInst && offsetInBufferInst->hasOneUse() &&
                 (offsetInBufferInst->getOpcode() == Instruction::Add ||
                  offsetInBufferInst->getOpcode() == Instruction::Or) &&
                 offsetInBufferInst->getOperand(0) == cov_chunk->baseIdxV &&
                 isa<ConstantInt>(offsetInBufferInst->getOperand(0))) {
        offsetInBufferInst->setOperand(1, chunkStartOffset);
      } else {
        offsetInBuffer = irBuilder->CreateAdd(cov_chunk->baseIdxV, chunkStartOffset);
        wiAns->incUpdateDepend(offsetInBuffer, wiAns->whichDepend(cov_chunk->baseIdxV));
      }
    }
    IGC_ASSERT(!ldRaw0->isVolatile() && !ldRaw1->isVolatile());
    Value *args[] = {ldRaw0->getResourceValue(), offsetInBuffer, irBuilder->getInt32(m_ChunkMinAlignment),
                     irBuilder->getFalse()};
    cov_chunk->chunkIO = irBuilder->CreateCall(ldRawFn, args, ldRaw0->getName());
    IGC_ASSERT(CompareMetadata(ldRaw0, ldRaw1));
    CopyMetadata(cov_chunk->chunkIO, ldRaw0);
    wiAns->incUpdateDepend(cov_chunk->chunkIO, wiAns->whichDepend(ldRaw0));
  }

  // Update the alignment of the new load instruction created above
  SetAlignmentFromOffset(cov_chunk->chunkIO);

  // add two splitters
  Instruction *splitter;
  splitter = AddChunkExtract(cov_chunk->chunkIO, eltid0 - cov_chunk->chunkStart);
  load0->replaceAllUsesWith(splitter);
  wiAns->incUpdateDepend(splitter, WIAnalysis::RANDOM);
  if (!load->getType()->isVectorTy()) {
    splitter = AddChunkExtract(cov_chunk->chunkIO, eltid - cov_chunk->chunkStart);
    load->replaceAllUsesWith(splitter);
    wiAns->incUpdateDepend(splitter, WIAnalysis::RANDOM);
  } else {
    // move all the extract from the input load to the chunk-load
    MoveExtracts(cov_chunk, load, eltid - cov_chunk->chunkStart);
  }
}

alignment_t ConstantCoalescing::GetAlignment(Instruction *load) const {
  IGC_ASSERT(isa<LdRawIntrinsic>(load) || isa<LoadInst>(load));
  alignment_t alignment = 1;

  if (isa<LoadInst>(load)) {
    alignment = IGCLLVM::getAlignmentValue(cast<LoadInst>(load));
  } else {
    alignment = cast<LdRawIntrinsic>(load)->getAlignment();
  }

  return alignment;
}

void ConstantCoalescing::SetAlignment(Instruction *load, uint alignment) {
  IGC_ASSERT(isa<LdRawIntrinsic>(load) || isa<LoadInst>(load));
  IGC_ASSERT(isPowerOf2_32(alignment));

  if (isa<LoadInst>(load)) {
    cast<LoadInst>(load)->setAlignment(getAlign(alignment));
  } else {
    cast<LdRawIntrinsic>(load)->setAlignment(alignment);
  }
}

void ConstantCoalescing::SetAlignmentFromOffset(Instruction *load) {
  IGC_ASSERT(isa<LdRawIntrinsic>(load) || isa<LoadInst>(load));
  Value *offset = nullptr;
  if (auto loadInst = dyn_cast<LoadInst>(load)) {
    Value *ptr = loadInst->getOperand(0);
    if (!isa<IntToPtrInst>(ptr)) {
      IGC_ASSERT_MESSAGE(false, "Unexpected pointer operand for LoadInst");
      return;
    }
    offset = cast<IntToPtrInst>(ptr)->getOperand(0);
  } else {
    offset = cast<LdRawIntrinsic>(load)->getOffsetValue();
  }
  IGC_ASSERT(offset != nullptr);
  const DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  KnownBits kb = computeKnownBits(offset, *dataLayout, 0 /*current depth*/, nullptr /*AssumptionCache*/, load, &DT);
  uint32_t numTrailZeros = std::min(kb.countMinTrailingZeros(), Value::MaxAlignmentExponent);
  alignment_t alignment = (1ull << std::min(kb.getBitWidth() - 1, numTrailZeros));
  alignment = std::max<alignment_t>(alignment, m_ChunkMinAlignment);
  SetAlignment(load, alignment);
}

void ConstantCoalescing::MergeUniformLoad(Instruction *load, Value *bufIdxV, uint addrSpace, Value *eltIdxV,
                                          uint offsetInBytes, uint maxEltPlus, const ExtensionKind &Extension,
                                          std::vector<BufChunk *> &chunk_vec) {
  // Only natural alignment is supported
  const alignment_t alignment = GetAlignment(load);

  if (!isPowerOf2_64(alignment)) {
    return;
  }

  const Type *loadDataTy = load->getType();
  const Type *loadEltTy;
  if (!loadDataTy->getScalarType()->isPointerTy())
    loadEltTy = loadDataTy->getScalarType();
  else {
    BufferType buffType = DecodeBufferType(loadDataTy->getPointerAddressSpace());
    if (IsBindless(buffType) || buffType == STATELESS_A32 || buffType == SSH_BINDLESS_CONSTANT_BUFFER)
      loadEltTy = irBuilder->getInt32Ty();
    else
      return;
  }
  const uint scalarSizeInBytes = (const uint)(loadEltTy->getPrimitiveSizeInBits() / 8);

  if (0 == scalarSizeInBytes || (offsetInBytes % scalarSizeInBytes) != 0) {
    return;
  }

  const uint eltid = offsetInBytes / scalarSizeInBytes;

  // Current assumption is that a chunk start needs to be DWORD aligned. In
  // the future we can consider adding support for merging 4 bytes or
  // 2 i16s/halfs into a single non-aligned DWORD.
  const bool isDwordAligned =
      ((offsetInBytes % 4) == 0 && (alignment >= 4 || eltIdxV == nullptr || IsDwordAligned(eltIdxV)));

  auto shouldMerge = [&](const BufChunk *cur_chunk) {
    if (CompareBufferBase(cur_chunk->bufIdxV, cur_chunk->addrSpace, bufIdxV, addrSpace) &&
        cur_chunk->baseIdxV == eltIdxV && cur_chunk->chunkIO->getType()->getScalarType() == loadEltTy &&
        CompareMetadata(cur_chunk->chunkIO, load) && !CheckForAliasingWrites(addrSpace, cur_chunk->chunkIO, load) &&
        cur_chunk->extensionKind == Extension) {
      uint lb = std::min(eltid, cur_chunk->chunkStart);
      uint ub = std::max(eltid + maxEltPlus, cur_chunk->chunkStart + cur_chunk->chunkSize);
      if (profitableChunkSize(ub, lb, scalarSizeInBytes) && (isDwordAligned || eltid >= cur_chunk->chunkStart)) {
        return true;
      }
    }

    return false;
  };

  BufChunk *cov_chunk = nullptr;
  if (auto I = std::find_if(chunk_vec.begin(), chunk_vec.end(), shouldMerge); I != chunk_vec.end()) {
    cov_chunk = *I;
  }

  // Stateless access needs OOB checks to avoid page faults.
  bool needsOutOfBoundsChecks = IGC_IS_FLAG_DISABLED(DisableConstantCoalescingOutOfBoundsCheck) &&
                                (addrSpace == ADDRESS_SPACE_CONSTANT || addrSpace == ADDRESS_SPACE_GLOBAL);

  // Lambda rounds the chunk size up to the next value supported by data port
  // loads or LSC transposed load.
  // Legacy data port supports:
  // - block loads of 1, 2, 4 or 8 OWords
  // - scattered loads of 1, 2 and 4 DWords/QWords
  // - (byte aligned) 1, 2 and 4 Bytes
  // LSC supports:
  // - transposed loads of 1, 2, 3, 4, 8, 16, 32 and 64 DWords/QWords
  // - (byte aligned) loads of 1, 2 and 4 Bytes
  auto RoundChunkSize = [this, scalarSizeInBytes, load](const uint32_t chunkSize) {
    bool supportsVec3Load = scalarSizeInBytes >= 4 && EmitPass::shouldGenerateLSCQuery(*m_ctx, load) == Tristate::True;
    uint32_t validChunkSize = (supportsVec3Load && chunkSize == 3) ? 3 : iSTD::RoundPower2((DWORD)chunkSize);
    return validChunkSize;
  };
  const uint32_t loadNumElements =
      loadDataTy->isVectorTy() ? int_cast<uint32_t>(cast<IGCLLVM::FixedVectorType>(loadDataTy)->getNumElements()) : 1;
  // VectorPreProcess pass legalizes loaded data size.
  if (!cov_chunk) {
    if (isDwordAligned) {
      cov_chunk = new BufChunk();
      cov_chunk->bufIdxV = bufIdxV;
      cov_chunk->addrSpace = addrSpace;
      cov_chunk->baseIdxV = eltIdxV;
      cov_chunk->elementSize = scalarSizeInBytes;
      cov_chunk->chunkStart = eltid;
      cov_chunk->chunkSize = RoundChunkSize(maxEltPlus);
      cov_chunk->extensionKind = Extension;
      const alignment_t chunkAlignment = std::max<alignment_t>(alignment, 4);
      cov_chunk->chunkIO = CreateChunkLoad(load, cov_chunk, eltid, chunkAlignment, Extension);
      chunk_vec.push_back(cov_chunk);
    }
  } else {
    // Determine load boundaries
    uint lb = std::min(eltid, cov_chunk->chunkStart);
    uint ub = std::max(eltid + maxEltPlus, cov_chunk->chunkStart + cov_chunk->chunkSize);

    // Calculate load start and size adjustments
    uint start_adj = cov_chunk->chunkStart - lb;
    uint size_adj = RoundChunkSize(ub - lb) - cov_chunk->chunkSize;

    // Out of bounds check
    if (needsOutOfBoundsChecks) {
      uint furthest_access = size_adj + lb + cov_chunk->chunkSize;
      if (furthest_access > ub) {
        // We are reaching out of bounds (OOB) of the backside of the buffer
        // First lets try moving the start index by the difference it is OOB
        uint new_start_adj = start_adj + (furthest_access - ub);
        if (cov_chunk->chunkStart < new_start_adj) // need to check if we are OOB on the frontside
        {
          // Looks like we are not able to adjust the starting point lets do a deeper check to
          // see if the furthest access is really okay by looking at other starting points
          uint furthest_cb_access = 0;
          for (BufChunk *cur_chunk : chunk_vec) {
            if (CompareBufferBase(cur_chunk->bufIdxV, cur_chunk->addrSpace, bufIdxV, addrSpace))
              furthest_cb_access = std::max(furthest_cb_access, cur_chunk->chunkStart + cur_chunk->chunkSize);
          }
          if (furthest_cb_access < furthest_access)
            return; // Not able to really benefit from merge for its not safe with this size
        } else
          start_adj = new_start_adj;
      }
    }

    // Apply adjustments
    if (load->getType()->isVectorTy()) {
      if (start_adj == 0) {
        if (size_adj)
          EnlargeChunk(cov_chunk, size_adj);
      } else {
        AdjustChunk(cov_chunk, start_adj, size_adj, Extension);
      }
      MoveExtracts(cov_chunk, load, eltid - cov_chunk->chunkStart);
    } else {
      Instruction *splitter = nullptr;
      if (start_adj == 0 && size_adj == 0) {
        splitter = FindOrAddChunkExtract(cov_chunk, eltid);
      } else if (start_adj > 0) {
        splitter = AdjustChunkAddExtract(cov_chunk, start_adj, size_adj, eltid, Extension);
      } else if (size_adj > 0) {
        splitter = EnlargeChunkAddExtract(cov_chunk, size_adj, eltid);
      }
      load->replaceAllUsesWith(splitter);
      wiAns->incUpdateDepend(splitter, wiAns->whichDepend(load));
    }
  }
}

#define IDENTICAL_PHI 0
uint ConstantCoalescing::GetOffsetAlignment(Value *val, PHINode *currPhi) const {
  GenIntrinsicInst *intr = dyn_cast<llvm::GenIntrinsicInst>(val);
  if (m_ctx->m_DriverInfo.SupportsDynamicUniformBuffers() && intr &&
      intr->getIntrinsicID() == GenISAIntrinsic::GenISA_RuntimeValue && isa<ConstantInt>(intr->getOperand(0))) {
    const PushInfo &pushInfo = m_ctx->getModuleMetaData()->pushInfo;
    const uint index = int_cast<uint>(cast<ConstantInt>(intr->getOperand(0))->getZExtValue());
    if (index >= pushInfo.dynamicBufferInfo.firstIndex &&
        index < pushInfo.dynamicBufferInfo.firstIndex + pushInfo.dynamicBufferInfo.numOffsets) {
      const uint minDynamicBufferOffsetAlignment = m_ctx->platform.getMinPushConstantBufferAlignment() * sizeof(DWORD);
      return minDynamicBufferOffsetAlignment; // 32 bytes
    }
  }

  Instruction *inst = dyn_cast<Instruction>(val);
  if (inst && inst->getNumOperands() == 2 && isa<Instruction>(inst->getOperand(0)) &&
      (inst->getOpcode() == Instruction::Add || inst->getOpcode() == Instruction::And ||
       inst->getOpcode() == Instruction::Mul || inst->getOpcode() == Instruction::Or ||
       inst->getOpcode() == Instruction::Shl)) {
    Value *src0 = inst->getOperand(0);
    Value *src1 = inst->getOperand(1);
    ConstantInt *cSrc1 = dyn_cast<ConstantInt>(src1);
    uint imm1 = cSrc1 ? int_cast<uint>(cSrc1->getZExtValue()) : 0;
    uint align0 = GetOffsetAlignment(src0, currPhi);
    uint align1 = (!cSrc1) ? GetOffsetAlignment(src1, currPhi) : 1;

    if (align0 == IDENTICAL_PHI && align1 == IDENTICAL_PHI)
      return 1;
    if (align0 != IDENTICAL_PHI && align1 == IDENTICAL_PHI)
      return align0;
    // In the case of align0 == IDENTICAL_PHI we cannot immediately return align1 bc in following code
    // align1 might be changed based on whether it is a constant.
    switch (inst->getOpcode()) {
    case Instruction::Add:
    case Instruction::Or: {
      if (cSrc1) {
        align1 = (1 << iSTD::bsf(imm1));
      }
      if (align0 == IDENTICAL_PHI)
        return align1;
      return std::min(align0, align1);
    }
    case Instruction::And: {
      if (cSrc1) {
        align1 = (1 << iSTD::bsf(imm1));
      }
      if (align0 == IDENTICAL_PHI)
        return align1;
      return std::max(align0, align1);
    }
    case Instruction::Mul: {
      if (cSrc1) {
        align1 = (1 << iSTD::bsf(imm1));
      }
      if (align0 == IDENTICAL_PHI)
        return align1;
      return align0 * align1;
    }
    case Instruction::Shl: {
      if (cSrc1) {
        align1 = imm1;
      }
      if (align0 == IDENTICAL_PHI)
        return align1;
      return align0 << align1;
    }
    default:
      break;
    }
  }

  /**
   * Including the alignment calculation passing a phi node. example:
   *
   * ```llvm
   *   %BB0_end = mul i32 %LocalID_Z, 288
   *   br label %1
   * 1:
   *   %phi = phi i32 [ %BB0_end, %0 ], [ %BB1_end, %1 ]
   *   %shl = shl i32 %phi, 4
   *   %ldraw0_0 = call <4 x i32> @llvm.genx.GenISA.ldrawvector ... %shl
   *   ...
   *   %or0_0 = or i32 %shl, 16
   *   %ldraw0_1 = call <4 x i32> @llvm.genx.GenISA.ldrawvector ... %or0_0
   *   ...
   *   %BB1_end = add nsw i32 %phi, 144
   *   br label %1
   * ```
   * The alignment for `or0_0` has to consider both `%shl` and `%BB0_end` from a phi
   */
  if (auto *phi = dyn_cast<PHINode>(val)) {
    if (currPhi) {
      if (phi == currPhi)
        return IDENTICAL_PHI;
      else
        return 1;
    }

    std::vector<uint> aligns;
    aligns.reserve(phi->getNumIncomingValues());
    for (uint i = 0; i < phi->getNumIncomingValues(); ++i) {
      uint m_align = GetOffsetAlignment(phi->getIncomingValue(i), phi);
      if (m_align != IDENTICAL_PHI)
        aligns.push_back(m_align);
    }
    if (aligns.empty()) // in case all are IDENTICAL_PHI
      return 1;

    return *std::min_element(aligns.begin(), aligns.end());
  }

  return 1;
}

Value *ConstantCoalescing::SimpleBaseOffset(Value *elt_idxv, uint &offset, ExtensionKind &Extension) {
  // in case expression comes from a smaller type arithmetic
  if (ZExtInst *reducedOffset = dyn_cast<ZExtInst>(elt_idxv)) {
    if (Extension == EK_SignExt) {
      offset = 0;
      return elt_idxv;
    }

    Extension = EK_ZeroExt;
    elt_idxv = reducedOffset->getOperand(0);
  }
  if (SExtInst *reducedOffset = dyn_cast<SExtInst>(elt_idxv)) {
    if (Extension == EK_ZeroExt) {
      offset = 0;
      return elt_idxv;
    }

    Extension = EK_SignExt;
    elt_idxv = reducedOffset->getOperand(0);
  }

  Instruction *expr = dyn_cast<Instruction>(elt_idxv);
  if (!expr) {
    offset = 0;
    return elt_idxv;
  }
  IGC_ASSERT(!isa<IntToPtrInst>(expr));
  if (!expr->getType()->isIntegerTy()) {
    offset = 0;
    return elt_idxv;
  }
  if (expr->getOpcode() == Instruction::Add) {
    Value *src0 = expr->getOperand(0);
    ConstantInt *csrc1 = dyn_cast<ConstantInt>(expr->getOperand(1));
    if (csrc1) {
      // Matches or+add pattern, e.g.:
      //    %535 = or i32 %519, 12
      //    %537 = add i32 %535, 16
      uint offset1 = 0;
      Value *base = SimpleBaseOffset(src0, offset1, Extension);
      offset = offset1 + static_cast<uint>(csrc1->getZExtValue());
      return base;
    }

    // clang-format off
    // Match complex patterns where the `add` with a constant offset
    // is not the last operation in the chain, e.g.:
    //  %a = call i32 foo()
    //  %b = call i32 foo()
    //  %ox = add i32 %a, %b
    //  %x = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p3276800(ptr addrspace(3276800) %bso, i32 %ox, i32 4, i1 false)
    //  %c = add i32 %b, 4
    //  %oy = add i32 %a, %c
    //  %y = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p3276800(ptr addrspace(3276800) %bso, i32 %oy, i32 4, i1 false)
    // clang-format on
    Value *src1 = expr->getOperand(1);
    // Helper lambda to create a sorted pair of Values
    auto MakeSortedPair = [](Value *a, Value *b) { return a < b ? std::make_pair(a, b) : std::make_pair(b, a); };
    // Check if this is a previously seen pair
    auto it = m_BaseOffsets.find(MakeSortedPair(src0, src1));
    if (it != m_BaseOffsets.end()) {
      return it->second;
    }
    // Check both operands for constant offsets
    uint offset0 = 0;
    uint offset1 = 0;
    ExtensionKind extension1 = EK_NotExtended;
    ExtensionKind extension2 = EK_NotExtended;
    Value *base0 = SimpleBaseOffset(src0, offset0, extension1);
    Value *base1 = SimpleBaseOffset(src1, offset1, extension2);
    // Bail if no constant offset was found or extension kinds don't match.
    if ((offset0 == 0 && offset1 == 0) || extension1 != extension2) {
      // Check for previously created add instruction with these operands.
      auto [it, inserted] = m_BaseOffsets.try_emplace(MakeSortedPair(src0, src1), expr);
      return it->second;
    }
    IGC_ASSERT(base0 != src0 || base1 != src1);
    Extension = extension1;
    offset = offset0 + offset1;
    // Find or create an `add` instruction with both bases.
    auto it1 = m_BaseOffsets.find(MakeSortedPair(base0, base1));
    if (it1 != m_BaseOffsets.end()) {
      return it1->second;
    }
    Instruction *newBase = BinaryOperator::Create(Instruction::Add, base0, base1, "", expr);
    wiAns->incUpdateDepend(newBase, std::max(wiAns->whichDepend(base0), wiAns->whichDepend(base1)));
    m_BaseOffsets.insert({MakeSortedPair(base0, base1), newBase});
    return newBase;
  } else if (expr->getOpcode() == Instruction::Or) {
    Value *src0 = expr->getOperand(0);
    Value *src1 = expr->getOperand(1);
    Instruction *or_inst0 = dyn_cast<Instruction>(src0);
    ConstantInt *or_csrc1 = dyn_cast<ConstantInt>(src1);
    if (or_inst0 && or_csrc1) {
      uint or_offset = int_cast<uint>(or_csrc1->getZExtValue());

      Instruction *inst0 = or_inst0;
      unsigned inst0_op = inst0->getOpcode();

      offset = or_offset;

      // Examples of patterns handled below:
      // - shl/add/or
      //      %27 = shl i32 %26, 4
      //      %168 = add i32 %27, 32
      //      %fromGBP33 = inttoptr i32 %168 to float addrspace(65538)*
      //      %ldrawidx34 = load float, float addrspace(65538)* %fromGBP33, align 16
      //      %173 = or i32 %168, 4
      //      %fromGBP35 = inttoptr i32 %173 to float addrspace(65538)*
      //      %ldrawidx36 = load float, float addrspace(65538)* %fromGBP35, align 4
      //
      // - mul/shl/or
      //        %6 = mul i32 %4, 76
      //        %7 = shl i32 %6, 4
      //        %11 = or i32 %7, 8
      //        %12 = inttoptr i32 %11 to <2 x i32> addrspace(65536)*
      //
      if (inst0_op == Instruction::Add && isa<Instruction>(inst0->getOperand(0)) &&
          isa<ConstantInt>(inst0->getOperand(1))) {
        uint add_offset = int_cast<uint>(cast<ConstantInt>(inst0->getOperand(1))->getZExtValue());
        if (or_offset < add_offset && ((iSTD::RoundPower2((DWORD)or_offset) - 1) & add_offset) == 0) {
          offset = or_offset + add_offset;
          src0 = inst0->getOperand(0);
          inst0 = cast<Instruction>(src0);
          inst0_op = inst0->getOpcode();
        }
      }

      if (or_offset < GetOffsetAlignment(inst0, NULL)) {
        return src0;
      }
    }
  }
  offset = 0;
  return elt_idxv;
}

// This is taken from GetPointerBaseWithConstantOffset() but:
// 1) We don't walk to look through addrspacecast
// 2) Don't look at globals
static Value *getPointerBaseWithConstantOffset(Value *Ptr, int64_t &Offset, const DataLayout &DL) {
  unsigned BitWidth = DL.getIndexTypeSizeInBits(Ptr->getType());
  APInt ByteOffset(BitWidth, 0);

  // We walk up the defs but use a visited set to handle unreachable code. In
  // that case, we stop after accumulating the cycle once (not that it
  // matters).
  SmallPtrSet<Value *, 16> Visited;
  while (Visited.insert(Ptr).second) {
    if (Ptr->getType()->isVectorTy())
      break;

    if (GEPOperator *GEP = dyn_cast<GEPOperator>(Ptr)) {
      APInt GEPOffset(DL.getIndexTypeSizeInBits(Ptr->getType()), 0);
      if (!GEP->accumulateConstantOffset(DL, GEPOffset))
        break;

      ByteOffset += GEPOffset.getSExtValue();

      Ptr = GEP->getPointerOperand();
    } else if (Operator::getOpcode(Ptr) == Instruction::BitCast) {
      Ptr = cast<Operator>(Ptr)->getOperand(0);
    } else {
      break;
    }
  }
  Offset = ByteOffset.getSExtValue();
  return Ptr;
}

bool ConstantCoalescing::DecomposePtrExp(Value *ptr_val, Value *&buf_idxv, Value *&elt_idxv, uint &offset,
                                         ExtensionKind &Extension) {
  buf_idxv = ptr_val;
  elt_idxv = nullptr;
  offset = 0;

  if (auto *i2p = dyn_cast<IntToPtrInst>(ptr_val)) {
    // get the int-type address computation
    auto *expr = dyn_cast<Instruction>(i2p->getOperand(0));
    if (expr == nullptr || !(expr->getType()->isIntegerTy())) {
      return false;
    }
    // look for the buf_idxv from a ptr2int
    if (isa<PtrToIntInst>(expr)) {
      // %expr = ptrtoint %x
      // %ptr_val = inttoptr %expr
      buf_idxv = expr;
      elt_idxv = nullptr;
      offset = 0;
      return true;
    }
    if (expr->getOpcode() == Instruction::Add) {
      // %expr    = add %src0 %src1
      // %ptr_val = inttoptr %expr
      Value *src0 = expr->getOperand(0);
      Value *src1 = expr->getOperand(1);
      if (isa<PtrToIntInst>(src0)) {
        // %src0 = ptrtoint %x
        buf_idxv = src0;
        if (auto *elt_idx = dyn_cast<ConstantInt>(src1)) { // direct access
          offset = (uint)elt_idx->getZExtValue();
          elt_idxv = nullptr;
        } else {
          elt_idxv = SimpleBaseOffset(src1, offset, Extension);
        }
        return true;
      } else if (isa<PtrToIntInst>(src1)) {
        buf_idxv = src1;
        if (auto *elt_idx = dyn_cast<ConstantInt>(src0)) { // direct access
          offset = (uint)elt_idx->getZExtValue();
          elt_idxv = nullptr;
        } else {
          elt_idxv = SimpleBaseOffset(src0, offset, Extension);
        }
        return true;
      } else if (auto *elt_idx = dyn_cast<ConstantInt>(src1)) {
        offset = (uint)elt_idx->getZExtValue();
        elt_idxv = nullptr;
        buf_idxv = src0;
        return true;
      } else if (auto *elt_idx = dyn_cast<ConstantInt>(src0)) {
        offset = (uint)elt_idx->getZExtValue();
        elt_idxv = nullptr;
        buf_idxv = src1;
        return true;
      }
    }
    buf_idxv = expr;
  } else {
    int64_t Offset = 0;
    auto *Ptr = getPointerBaseWithConstantOffset(ptr_val, Offset, *dataLayout);

    if ((m_ctx->type != ShaderType::OPENCL_SHADER && Ptr == ptr_val) || Offset < 0)
      return false;

    buf_idxv = Ptr;
    elt_idxv = nullptr;
    offset = (uint)Offset;
  }

  return true;
}

/// look at all the uses of a vector load. If they are all extract-elements
/// with constant indices, return the max-elt-index + 1.
uint ConstantCoalescing::CheckVectorElementUses(const Instruction *load) {
  uint maxEltPlus = 0;
  for (auto *U : load->users()) {
    if (auto *extract = dyn_cast<ExtractElementInst>(U)) {
      if (auto *index = dyn_cast<ConstantInt>(extract->getIndexOperand())) {
        uint cv = static_cast<uint>(index->getZExtValue());
        maxEltPlus = std::max(maxEltPlus, cv + 1);
      } else {
        return 0;
      }
    } else {
      return 0;
    }
  }
  return maxEltPlus;
}

Instruction *ConstantCoalescing::CreateChunkLoad(Instruction *seedi, BufChunk *chunk, uint eltid, alignment_t alignment,
                                                 const ExtensionKind &Extension) {
  irBuilder->SetInsertPoint(seedi);
  if (LoadInst *load = dyn_cast<LoadInst>(seedi)) {
      IGC_ASSERT(!load->isVolatile()); // no constant buffer volatile loads

    LoadInst *chunkLoad;
    Value *cb_ptr = load->getPointerOperand();

    Value *eac;
    // ocl case: no gep
    if (load->getPointerAddressSpace() == ADDRESS_SPACE_CONSTANT ||
        load->getPointerAddressSpace() == ADDRESS_SPACE_GLOBAL) {
      eac = cb_ptr;
      if (eltid == chunk->chunkStart && isa<IntToPtrInst>(eac))
        eac = dyn_cast<IntToPtrInst>(eac)->getOperand(0);
      else
        eac = FormChunkAddress(chunk, Extension);
    } else {
      // gfx case
      eac = ConstantInt::get(irBuilder->getInt32Ty(), chunk->chunkStart * chunk->elementSize);
      if (chunk->baseIdxV) {
        if (chunk->chunkStart) {
          eac = irBuilder->CreateAdd(chunk->baseIdxV, eac);
          wiAns->incUpdateDepend(eac, wiAns->whichDepend(chunk->baseIdxV));
        } else {
          eac = chunk->baseIdxV;
        }
      }
    }
    Type *vty = IGCLLVM::FixedVectorType::get(load->getType()->getScalarType(), chunk->chunkSize);
    unsigned addrSpace = (cast<PointerType>(cb_ptr->getType()))->getAddressSpace();
    PointerType *pty = PointerType::get(vty, addrSpace);
    // cannot use irbuilder to create IntToPtr. It may create ConstantExpr instead of instruction
    Instruction *ptr = IntToPtrInst::Create(Instruction::IntToPtr, eac, pty, "chunkPtr", seedi);
    m_TT->RegisterNewValueAndAssignID(ptr);
    // Update debug location
    ptr->setDebugLoc(irBuilder->getCurrentDebugLocation());
    wiAns->incUpdateDepend(ptr, wiAns->whichDepend(seedi));
    chunkLoad = irBuilder->CreateLoad(vty, ptr);
    chunkLoad->setAlignment(getAlign(alignment));
    CopyMetadata(chunkLoad, seedi);
    chunk->chunkIO = chunkLoad;
  } else {
    // bindless case
    LdRawIntrinsic *ldRaw = cast<LdRawIntrinsic>(seedi);
    IGC_ASSERT(!ldRaw->isVolatile()); // no constant buffer volatile loads
    Value *eac = irBuilder->getInt32(chunk->chunkStart * chunk->elementSize);
    if (chunk->baseIdxV) {
      Value *baseIdxV = chunk->baseIdxV;
      if (baseIdxV->getType()->getIntegerBitWidth() < 32) {
        baseIdxV = irBuilder->CreateZExt(baseIdxV, eac->getType());
      }
      if (chunk->chunkStart) {
        eac = irBuilder->CreateAdd(baseIdxV, eac);
        wiAns->incUpdateDepend(eac, wiAns->whichDepend(chunk->baseIdxV));
      } else {
        eac = baseIdxV;
      }
    }
    Type *vty = IGCLLVM::FixedVectorType::get(ldRaw->getType()->getScalarType(), chunk->chunkSize);
    Type *types[] = {
        vty,
        ldRaw->getResourceValue()->getType(),
    };
    Value *arguments[] = {ldRaw->getResourceValue(), eac, irBuilder->getInt32((uint32_t)alignment),
                          irBuilder->getFalse()};
    Function *ldRawFn =
        GenISAIntrinsic::getDeclaration(curFunc->getParent(), GenISAIntrinsic::GenISA_ldrawvector_indexed, types);
    chunk->chunkIO = irBuilder->CreateCall(ldRawFn, arguments);
    CopyMetadata(chunk->chunkIO, ldRaw);
  }

  wiAns->incUpdateDepend(chunk->chunkIO, wiAns->whichDepend(seedi));

  if (!seedi->getType()->isVectorTy()) {
    Instruction *splitter = AddChunkExtract(chunk->chunkIO, eltid - chunk->chunkStart);
    seedi->replaceAllUsesWith(splitter);
    wiAns->incUpdateDepend(splitter, wiAns->whichDepend(seedi));
  } else {
    MoveExtracts(chunk, seedi, eltid - chunk->chunkStart);
  }
  return chunk->chunkIO;
}

Instruction *ConstantCoalescing::AddChunkExtract(Instruction *load, uint eltid) {
  irBuilder->SetInsertPoint(load->getNextNode());
  Value *cv_eltid = ConstantInt::get(irBuilder->getInt32Ty(), eltid);
  Value *extract = irBuilder->CreateExtractElement(load, cv_eltid);
  wiAns->incUpdateDepend(extract, wiAns->whichDepend(load));
  return cast<Instruction>(extract);
}

Instruction *ConstantCoalescing::FindOrAddChunkExtract(BufChunk *cov_chunk, uint eltid) {
  Instruction *splitter = nullptr;
  // look for the splitter in existing uses
  Value::user_iterator use_it = cov_chunk->chunkIO->user_begin();
  Value::user_iterator use_e = cov_chunk->chunkIO->user_end();
  for (; use_it != use_e; ++use_it) {
    Instruction *const usei = dyn_cast<Instruction>(*use_it);
    IGC_ASSERT(nullptr != usei);
    IGC_ASSERT(isa<ExtractElementInst>(usei));
    ConstantInt *e_idx = dyn_cast<ConstantInt>(usei->getOperand(1));
    if (!e_idx) {
      continue;
    }
    IGC_ASSERT(nullptr != e_idx);
    uint val = (uint)e_idx->getZExtValue();
    if (val == eltid - cov_chunk->chunkStart) {
      splitter = usei;
      // move the extract element to make sure it dominates the new use
      usei->moveBefore(cov_chunk->chunkIO->getNextNode());
      break;
    }
  }
  // if a splitter does not exist, add a new splitter
  if (!splitter) {
    splitter = AddChunkExtract(cov_chunk->chunkIO, eltid - cov_chunk->chunkStart);
  }
  return splitter;
}

void ConstantCoalescing::AdjustChunk(BufChunk *cov_chunk, uint start_adj, uint size_adj,
                                     const ExtensionKind &Extension) {
  cov_chunk->chunkSize = RoundChunkSize(cov_chunk->chunkSize + size_adj, cov_chunk->elementSize);
  cov_chunk->chunkStart -= start_adj;
  // mutateType to change array-size
  Type *originalType = cov_chunk->chunkIO->getType();
  Type *vty = IGCLLVM::FixedVectorType::get(cov_chunk->chunkIO->getType()->getScalarType(), cov_chunk->chunkSize);
  cov_chunk->chunkIO->mutateType(vty);
  // change the dest ptr-type on bitcast
  if (isa<LoadInst>(cov_chunk->chunkIO)) {
    Value *addr_ptr = cov_chunk->chunkIO->getOperand(0);
    unsigned addrSpace = (cast<PointerType>(addr_ptr->getType()))->getAddressSpace();
    if (addrSpace == ADDRESS_SPACE_CONSTANT || addrSpace == ADDRESS_SPACE_GLOBAL) {
      // ocl path
      IGC_ASSERT(isa<IntToPtrInst>(addr_ptr));
      Instruction *intToPtr = cast<Instruction>(addr_ptr);
      irBuilder->SetInsertPoint(intToPtr);
      addr_ptr->mutateType(PointerType::get(vty, addrSpace));
      Value *eac = intToPtr->getOperand(0);
      Instruction *expr = dyn_cast<Instruction>(eac);
      bool foundOffset = false;
      if (expr && expr->getOpcode() == Instruction::Add && expr->hasOneUse()) {
        uint srcIdx = 2;
        if (expr->getOperand(0) == cov_chunk->bufIdxV)
          srcIdx = 1;
        else if (expr->getOperand(1) == cov_chunk->bufIdxV)
          srcIdx = 0;
        if (srcIdx == 0 || srcIdx == 1) {
          if (isa<ConstantInt>(expr->getOperand(srcIdx))) {
            Value *cv_start = ConstantInt::get(expr->getType(), cov_chunk->chunkStart * cov_chunk->elementSize);
            expr->setOperand(srcIdx, cv_start);
            foundOffset = true;
          } else {
            Instruction *expr2 = dyn_cast<Instruction>(expr->getOperand(srcIdx));
            if (expr2 && expr2->hasOneUse()) {
              if ((isa<ZExtInst>(expr2) || isa<SExtInst>(expr2)) && isa<BinaryOperator>(expr2->getOperand(0)))
                expr2 = cast<Instruction>(expr2->getOperand(0));
              IGC_ASSERT(isa<BinaryOperator>(expr2));

              if (expr2->getOperand(0) == cov_chunk->baseIdxV && isa<ConstantInt>(expr2->getOperand(1))) {
                Value *cv_start = ConstantInt::get(expr2->getType(), cov_chunk->chunkStart * cov_chunk->elementSize);
                expr2->setOperand(1, cv_start);
                foundOffset = true;
              } else if (expr2->getOperand(1) == cov_chunk->baseIdxV && isa<ConstantInt>(expr2->getOperand(0))) {
                Value *cv_start = ConstantInt::get(expr->getType(), cov_chunk->chunkStart * cov_chunk->elementSize);
                expr2->setOperand(0, cv_start);
                foundOffset = true;
              }
            }
          }
        }
      }
      if (!foundOffset) {
        // if we cannot modify the offset, create a new chain of address calculation
        eac = FormChunkAddress(cov_chunk, Extension);
        intToPtr->setOperand(0, eac);
      }
    } else {
      // gfx path
      IGC_ASSERT(isa<IntToPtrInst>(addr_ptr) || isa<BitCastInst>(addr_ptr));
      addr_ptr->mutateType(PointerType::get(vty, addrSpace));
      Instruction *intToPtrInst = cast<Instruction>(addr_ptr);
      while (isa<IntToPtrInst>(intToPtrInst->getOperand(0)) || isa<BitCastInst>(intToPtrInst->getOperand(0))) {
        intToPtrInst = cast<Instruction>(intToPtrInst->getOperand(0));
      }
      IGC_ASSERT(isa<IntToPtrInst>(intToPtrInst));
      if (cov_chunk->baseIdxV == nullptr) {
        Value *cv_start = ConstantInt::get(irBuilder->getInt32Ty(), cov_chunk->chunkStart * cov_chunk->elementSize);
        intToPtrInst->setOperand(0, cv_start);
      } else {
        IGC_ASSERT(isa<Instruction>(intToPtrInst->getOperand(0)));
        Instruction *eac = cast<Instruction>(intToPtrInst->getOperand(0));
        IGC_ASSERT(nullptr != eac);
        // Has to be `Add` or `Or` instruction, see SimpleBaseOffset().
        IGC_ASSERT((eac->getOpcode() == Instruction::Add) || (eac->getOpcode() == Instruction::Or));
        ConstantInt *cv_start =
            ConstantInt::get(irBuilder->getInt32Ty(), cov_chunk->chunkStart * cov_chunk->elementSize);
        IGC_ASSERT(nullptr != cv_start);
        if (cv_start->isZero()) {
          intToPtrInst->setOperand(0, eac->getOperand(0));
        } else {
          eac->setOperand(1, cv_start);
        }
      }
    }
  } else {
    // bindless case
    LdRawIntrinsic *ldRaw = cast<LdRawIntrinsic>(cov_chunk->chunkIO);
    Type *types[] = {
        vty,
        ldRaw->getResourceValue()->getType(),
    };
    Function *ldRawFn =
        GenISAIntrinsic::getDeclaration(curFunc->getParent(), GenISAIntrinsic::GenISA_ldrawvector_indexed, types);
    ldRaw->setCalledFunction(ldRawFn);
    ldRaw->mutateType(vty);
    if (cov_chunk->baseIdxV == nullptr) {
      Value *cv_start = irBuilder->getInt32(cov_chunk->chunkStart * cov_chunk->elementSize);
      ldRaw->setOffsetValue(cv_start);
    } else {
      Value *eac = ldRaw->getOffsetValue();
      IGC_ASSERT(isa<Instruction>(eac));
      IGC_ASSERT(cast<Instruction>(eac));
      IGC_ASSERT((cast<Instruction>(eac)->getOpcode() == Instruction::Add) ||
                 (cast<Instruction>(eac)->getOpcode() == Instruction::Or));
      Value *cv_start = irBuilder->getInt32(cov_chunk->chunkStart * cov_chunk->elementSize);
      cast<Instruction>(eac)->setOperand(1, cv_start);
    }
  }

  // Update the alignment if the starting offset changed.
  if (start_adj != 0) {
    SetAlignmentFromOffset(cov_chunk->chunkIO);
  }

  SmallVector<Instruction *, 4> use_set;
  // adjust all the splitters
  Value::user_iterator use_it = cov_chunk->chunkIO->user_begin();
  Value::user_iterator use_e = cov_chunk->chunkIO->user_end();
  for (; use_it != use_e; ++use_it) {
    if (ExtractElementInst *usei = dyn_cast<ExtractElementInst>(*use_it)) {
      if (llvm::ConstantInt *e_idx = llvm::dyn_cast<llvm::ConstantInt>(usei->getOperand(1))) {
        uint val = (uint)e_idx->getZExtValue();
        val += start_adj;
        // update the index source
        e_idx = ConstantInt::get(irBuilder->getInt32Ty(), val);
        usei->setOperand(1, e_idx);
        continue;
      }
    }
    use_set.push_back(llvm::cast<Instruction>(*use_it));
  }
  if (use_set.size() > 0) {
    WIAnalysis::WIDependancy loadDep = wiAns->whichDepend(cov_chunk->chunkIO);
    irBuilder->SetInsertPoint(cov_chunk->chunkIO->getNextNode());
    Value *vec = UndefValue::get(originalType);
    for (unsigned i = 0; i < cast<IGCLLVM::FixedVectorType>(originalType)->getNumElements(); i++) {
      Value *channel = irBuilder->CreateExtractElement(cov_chunk->chunkIO, irBuilder->getInt32(i + start_adj));
      wiAns->incUpdateDepend(channel, loadDep);
      vec = irBuilder->CreateInsertElement(vec, channel, irBuilder->getInt32(i));
      wiAns->incUpdateDepend(vec, loadDep);
    }
    for (auto it : use_set) {
      it->replaceUsesOfWith(cov_chunk->chunkIO, vec);
    }
  }
}

Instruction *ConstantCoalescing::AdjustChunkAddExtract(BufChunk *cov_chunk, uint start_adj, uint size_adj, uint eltid,
                                                       const ExtensionKind &Extension) {
  AdjustChunk(cov_chunk, start_adj, size_adj, Extension);
  return AddChunkExtract(cov_chunk->chunkIO, eltid - cov_chunk->chunkStart);
}

void ConstantCoalescing::MoveExtracts(BufChunk *cov_chunk, Instruction *load, uint start_adj) {
  // modify the extract-elements from the load, and move it to the chunk
  Value::user_iterator use_it = load->user_begin();
  Value::user_iterator use_e = load->user_end();
  bool noneDirectExtract = false;
  std::vector<Instruction *> use_set;
  for (; use_it != use_e; ++use_it) {
    Instruction *usei = cast<Instruction>(*use_it);
    if (!isa<ExtractElementInst>(usei) || !isa<ConstantInt>(usei->getOperand(1))) {
      noneDirectExtract = true;
      break;
    }
    use_set.push_back(usei);
  }
  if (noneDirectExtract == false) {
    uint num_uses = use_set.size();
    for (uint i = 0; i < num_uses; ++i) {
      Instruction *usei = use_set[i];
      if (start_adj) {
        llvm::ConstantInt *e_idx = cast<ConstantInt>(usei->getOperand(1));
        uint val = (uint)e_idx->getZExtValue();
        val += start_adj;
        // update the index source
        e_idx = ConstantInt::get(irBuilder->getInt32Ty(), val);
        usei->setOperand(1, e_idx);
      }
      usei->setOperand(0, cov_chunk->chunkIO);
    }
  } else {
    if (start_adj || cov_chunk->chunkIO->getType() != load->getType()) {
      WIAnalysis::WIDependancy loadDep = wiAns->whichDepend(load);
      irBuilder->SetInsertPoint(load->getNextNode());
      Type *vecType = load->getType();
      Value *vec = UndefValue::get(vecType);
      for (unsigned i = 0; i < cast<IGCLLVM::FixedVectorType>(vecType)->getNumElements(); i++) {
        Value *channel = irBuilder->CreateExtractElement(cov_chunk->chunkIO, irBuilder->getInt32(i + start_adj));
        wiAns->incUpdateDepend(channel, loadDep);
        vec = irBuilder->CreateInsertElement(vec, channel, irBuilder->getInt32(i));
        wiAns->incUpdateDepend(vec, loadDep);
      }
      load->replaceAllUsesWith(vec);
    } else {
      load->replaceAllUsesWith(cov_chunk->chunkIO);
    }
  }
}

void ConstantCoalescing::EnlargeChunk(BufChunk *cov_chunk, uint size_adj) {
  cov_chunk->chunkSize = RoundChunkSize(cov_chunk->chunkSize + size_adj, cov_chunk->elementSize);
  // mutateType to change array-size
  Type *originalType = cov_chunk->chunkIO->getType();
  Type *vty = IGCLLVM::FixedVectorType::get(cov_chunk->chunkIO->getType()->getScalarType(), cov_chunk->chunkSize);
  cov_chunk->chunkIO->mutateType(vty);
  if (isa<LoadInst>(cov_chunk->chunkIO)) {
    // change the dest ptr-type on bitcast
    Value *addr_ptr = cov_chunk->chunkIO->getOperand(0);
    unsigned addrSpace = (cast<PointerType>(addr_ptr->getType()))->getAddressSpace();
    if (isa<BitCastInst>(addr_ptr) || isa<IntToPtrInst>(addr_ptr)) {
      addr_ptr->mutateType(PointerType::get(vty, addrSpace));
    }
  } else {
    LdRawIntrinsic *ldRaw = cast<LdRawIntrinsic>(cov_chunk->chunkIO);
    Type *types[] = {
        vty,
        ldRaw->getResourceValue()->getType(),
    };
    Function *ldRawFn =
        GenISAIntrinsic::getDeclaration(curFunc->getParent(), GenISAIntrinsic::GenISA_ldrawvector_indexed, types);
    ldRaw->setCalledFunction(ldRawFn);
  }
  // for none extract uses correct the use
  Value::user_iterator use_it = cov_chunk->chunkIO->user_begin();
  Value::user_iterator use_e = cov_chunk->chunkIO->user_end();
  SmallVector<Instruction *, 4> use_set;
  for (; use_it != use_e; ++use_it) {
    if (!isa<ExtractElementInst>(*use_it)) {
      use_set.push_back(cast<Instruction>(*use_it));
    }
  }
  if (use_set.size() > 0) {
    WIAnalysis::WIDependancy loadDep = wiAns->whichDepend(cov_chunk->chunkIO);
    irBuilder->SetInsertPoint(cov_chunk->chunkIO->getNextNode());
    Value *vec = UndefValue::get(originalType);
    for (unsigned i = 0; i < cast<IGCLLVM::FixedVectorType>(originalType)->getNumElements(); i++) {
      Value *channel = irBuilder->CreateExtractElement(cov_chunk->chunkIO, irBuilder->getInt32(i));
      wiAns->incUpdateDepend(channel, loadDep);
      vec = irBuilder->CreateInsertElement(vec, channel, irBuilder->getInt32(i));
      wiAns->incUpdateDepend(vec, loadDep);
    }
    for (auto it : use_set) {
      it->replaceUsesOfWith(cov_chunk->chunkIO, vec);
    }
  }
}

Instruction *ConstantCoalescing::EnlargeChunkAddExtract(BufChunk *cov_chunk, uint size_adj, uint eltid) {
  EnlargeChunk(cov_chunk, size_adj);
  // add a new splitter
  return AddChunkExtract(cov_chunk->chunkIO, eltid - cov_chunk->chunkStart);
}

// Returns true if input address is 16 bytes aligned.
bool ConstantCoalescing::IsSamplerAlignedAddress(Value *addr) const {
  Instruction *inst = dyn_cast<Instruction>(addr);
  if (inst && (inst->getOpcode() == Instruction::Shl || inst->getOpcode() == Instruction::Mul ||
               inst->getOpcode() == Instruction::And || inst->getOpcode() == Instruction::Or ||
               inst->getOpcode() == Instruction::Add)) {
    ConstantInt *src1ConstVal = dyn_cast<ConstantInt>(inst->getOperand(1));
    unsigned int constant = src1ConstVal ? int_cast<unsigned int>(src1ConstVal->getZExtValue()) : 0;

    if (inst->getOpcode() == Instruction::Shl && src1ConstVal) {
      if (constant >= 4) {
        return true;
      }
    } else if (inst->getOpcode() == Instruction::Mul && src1ConstVal) {
      if ((constant % 16) == 0) {
        return true;
      }
    } else if (inst->getOpcode() == Instruction::And && src1ConstVal) {
      if ((constant & 0xf) == 0) {
        return true;
      }
    } else if (inst->getOpcode() == Instruction::Add || inst->getOpcode() == Instruction::Or) {
      if (IsSamplerAlignedAddress(inst->getOperand(0)) && IsSamplerAlignedAddress(inst->getOperand(1))) {
        return true;
      }
    }
  } else if (ConstantInt *constant = dyn_cast<ConstantInt>(addr)) {
    if ((int_cast<uint>(constant->getZExtValue()) % 16) == 0) {
      return true;
    }
  }
  return false;
}

// Calculates the address in 4DWORD units, ready to be used in
// sampler ld message. Input value is in bytes.
Value *ConstantCoalescing::GetSamplerAlignedAddress(Value *addr) {
  IGC_ASSERT(IsSamplerAlignedAddress(addr));

  Value *elementIndex = nullptr;
  Instruction *inst = dyn_cast<Instruction>(addr);

  if (inst &&
      (inst->getOpcode() == Instruction::Shl || inst->getOpcode() == Instruction::Mul ||
       inst->getOpcode() == Instruction::And) &&
      isa<ConstantInt>(inst->getOperand(1))) {
    irBuilder->SetInsertPoint(inst);
    unsigned int constant = int_cast<unsigned int>(cast<ConstantInt>(inst->getOperand(1))->getZExtValue());
    elementIndex = inst->getOperand(0);

    if (inst->getOpcode() == Instruction::Shl) {
      IGC_ASSERT(constant >= 4);

      if (constant > 4 && constant < 32) {
        elementIndex = irBuilder->CreateShl(elementIndex, ConstantInt::get(elementIndex->getType(), (constant - 4)));
        wiAns->incUpdateDepend(elementIndex, WIAnalysis::RANDOM);
      }
    } else if (inst->getOpcode() == Instruction::Mul) {
      IGC_ASSERT(constant % 16 == 0);

      if (constant != 16) {
        elementIndex = irBuilder->CreateMul(elementIndex, ConstantInt::get(elementIndex->getType(), (constant / 16)));
        wiAns->incUpdateDepend(elementIndex, WIAnalysis::RANDOM);
      }
    } else {
      IGC_ASSERT(nullptr != inst);
      IGC_ASSERT(inst->getOpcode() == Instruction::And);
      IGC_ASSERT((constant & 0xf) == 0);

      elementIndex = irBuilder->CreateLShr(elementIndex, ConstantInt::get(elementIndex->getType(), 4));
      wiAns->incUpdateDepend(elementIndex, WIAnalysis::RANDOM);

      if (constant != 0xFFFFFFF0) {
        elementIndex = irBuilder->CreateAnd(elementIndex, ConstantInt::get(elementIndex->getType(), (constant >> 4)));
        wiAns->incUpdateDepend(elementIndex, WIAnalysis::RANDOM);
      }
    }
  } else if (inst && (inst->getOpcode() == Instruction::Add || inst->getOpcode() == Instruction::Or)) {
    Value *a = GetSamplerAlignedAddress(inst->getOperand(0));
    Value *b = GetSamplerAlignedAddress(inst->getOperand(1));

    irBuilder->SetInsertPoint(inst);
    if (inst->getOpcode() == Instruction::Add) {
      elementIndex = irBuilder->CreateAdd(a, b);
    } else {
      elementIndex = irBuilder->CreateOr(a, b);
    }
    wiAns->incUpdateDepend(elementIndex, WIAnalysis::RANDOM);
  } else if (ConstantInt *constant = dyn_cast<ConstantInt>(addr)) {
    uint offset = int_cast<uint>(constant->getZExtValue());
    IGC_ASSERT((offset % 16) == 0);

    elementIndex = ConstantInt::get(constant->getType(), (offset / 16));
  }

  return elementIndex;
}

/// Replace non-uniform scatter load with sampler load messages.
void ConstantCoalescing::ScatterToSampler(Instruction *load, Value *bufIdxV, uint addrSpace,
                                          Value *baseAddressInBytes, // base address calculated by SimpleBaseOffset()
                                          uint offsetInBytes,        // offset from baseAddressInBytes
                                          std::vector<BufChunk *> &chunk_vec) {
  constexpr uint samplerElementSizeInBytes = sizeof(uint32_t);
  constexpr uint samplerLoadSizeInDwords = 4;
  constexpr uint samplerLoadSizeInBytes = samplerLoadSizeInDwords * samplerElementSizeInBytes;
  const uint loadSizeInBytes = (unsigned int)load->getType()->getPrimitiveSizeInBits() / 8;

  IGC_ASSERT(nullptr != (load->getType()));
  IGC_ASSERT((!load->getType()->isVectorTy()) ||
             (cast<IGCLLVM::FixedVectorType>(load->getType())->getNumElements() <= 4));


  // Code below doesn't support crossing 4 DWORD boundary i.e. mapping a
  // single input load to multiple sampler loads.
  const bool canBeLoadedUsingSampler =
      (((offsetInBytes % samplerLoadSizeInBytes) + loadSizeInBytes) <= samplerLoadSizeInBytes);

  if (baseAddressInBytes && isa<Instruction>(baseAddressInBytes) && IsSamplerAlignedAddress(baseAddressInBytes) &&
      canBeLoadedUsingSampler) {
    Instruction *baseInBytes = cast<Instruction>(baseAddressInBytes);
    irBuilder->SetInsertPoint(baseInBytes);

    WIAnalysis::WIDependancy baseInBytesDep = wiAns->whichDepend(baseInBytes);
    Value *baseAddressInOwords = GetSamplerAlignedAddress(baseInBytes);
    IGC_ASSERT(baseAddressInOwords);

    // it is possible that baseInBytes is uniform, yet load is non-uniform due to the use location of load
    if (baseAddressInOwords != baseInBytes->getOperand(0)) {
      Value *newVal = irBuilder->CreateShl(baseAddressInOwords, ConstantInt::get(baseAddressInOwords->getType(), 4));
      wiAns->incUpdateDepend(newVal, baseInBytesDep);
      baseInBytes->replaceAllUsesWith(newVal);
    } else if (wiAns->whichDepend(baseAddressInOwords) != baseInBytesDep) {
      // quick fix for a special case: baseAddressInOwords is uniform and baseInBytes is not uniform.
      // If we use baseInBytes-src0 (elementIndx) directly at cf-join point by this transform,
      // we can change the uniformness of baseAddressInOwords
      baseAddressInOwords =
          irBuilder->CreateShl(baseAddressInOwords, ConstantInt::get(baseAddressInOwords->getType(), 0));
      wiAns->incUpdateDepend(baseAddressInOwords, baseInBytesDep);
      Value *newVal = irBuilder->CreateShl(baseAddressInOwords, ConstantInt::get(baseAddressInOwords->getType(), 4));
      wiAns->incUpdateDepend(newVal, baseInBytesDep);
      baseInBytes->replaceAllUsesWith(newVal);
    }

    Value *chunkBaseAddress = baseAddressInOwords;

    BufChunk *cov_chunk = nullptr;
    for (std::vector<BufChunk *>::reverse_iterator rit = chunk_vec.rbegin(), rie = chunk_vec.rend(); rit != rie;
         ++rit) {
      BufChunk *cur_chunk = *rit;
      // Look for an existing sampler load covering data range of the input load.
      if (CompareBufferBase(cur_chunk->bufIdxV, cur_chunk->addrSpace, bufIdxV, addrSpace) &&
          cur_chunk->baseIdxV == chunkBaseAddress) {
        const uint chunkStartInBytes = cur_chunk->chunkStart * cur_chunk->elementSize;
        const uint chunkEndInBytes = (cur_chunk->chunkStart + cur_chunk->chunkSize) * cur_chunk->elementSize;
        if (offsetInBytes >= chunkStartInBytes && (offsetInBytes + loadSizeInBytes) <= chunkEndInBytes) {
          cov_chunk = cur_chunk;
          break;
        }
      }
    }
    irBuilder->SetInsertPoint(load);
    Instruction *ld = nullptr;
    if (!cov_chunk) {
      cov_chunk = new BufChunk();
      cov_chunk->bufIdxV = bufIdxV;
      cov_chunk->addrSpace = addrSpace;
      cov_chunk->baseIdxV = chunkBaseAddress;
      cov_chunk->elementSize = samplerElementSizeInBytes; // 4 bytes
      cov_chunk->chunkStart = iSTD::RoundDown((offsetInBytes / samplerElementSizeInBytes),
                                              samplerLoadSizeInDwords); // in DWORDS aligned to OWORDs
      cov_chunk->chunkSize = samplerLoadSizeInDwords;                   // in DWORDs

      Value *dataAddress = chunkBaseAddress;
      if (offsetInBytes >= samplerLoadSizeInBytes) {
        const uint32_t chunkOffset = offsetInBytes / samplerLoadSizeInBytes;
        dataAddress = irBuilder->CreateAdd(dataAddress, ConstantInt::get(dataAddress->getType(), chunkOffset));
        wiAns->incUpdateDepend(dataAddress, WIAnalysis::RANDOM);
      }
      if (dataAddress->getType()->getIntegerBitWidth() >= 32) {
        dataAddress = irBuilder->CreateAnd(dataAddress, ConstantInt::get(dataAddress->getType(), 0x0FFFFFFF));
        wiAns->incUpdateDepend(dataAddress, WIAnalysis::RANDOM);
      }
      if (dataAddress->getType() != irBuilder->getInt32Ty()) {
        dataAddress = irBuilder->CreateZExtOrTrunc(dataAddress, irBuilder->getInt32Ty());
        wiAns->incUpdateDepend(dataAddress, WIAnalysis::RANDOM);
      }

      ld = CreateSamplerLoad(dataAddress, bufIdxV, addrSpace);
      cov_chunk->chunkIO = ld;
      chunk_vec.push_back(cov_chunk);
    } else {
      ld = cov_chunk->chunkIO;
    }

    ReplaceLoadWithSamplerLoad(load, ld, (offsetInBytes % samplerLoadSizeInBytes));
  }
}

Instruction *ConstantCoalescing::CreateSamplerLoad(Value *index, Value *resourcePtr, uint addrSpace) {
  IGC_ASSERT(!resourcePtr || isa<PointerType>(resourcePtr->getType()));
  IGC_ASSERT(!resourcePtr || addrSpace == resourcePtr->getType()->getPointerAddressSpace());

  PointerType *resourceType =
      resourcePtr ? cast<PointerType>(resourcePtr->getType()) : PointerType::get(irBuilder->getFloatTy(), addrSpace);

  Type *types[] = {IGCLLVM::FixedVectorType::get(irBuilder->getFloatTy(), 4), resourceType, resourceType};
  Function *l = GenISAIntrinsic::getDeclaration(curFunc->getParent(), llvm::GenISAIntrinsic::GenISA_ldptr, types);
  Value *attr[] = {
      index,
      irBuilder->getInt32(0),
      irBuilder->getInt32(0),
      irBuilder->getInt32(0),
      UndefValue::get(resourceType),
      resourcePtr ? resourcePtr : ConstantPointerNull::get(resourceType),
      irBuilder->getInt32(0),
      irBuilder->getInt32(0),
      irBuilder->getInt32(0),
  };
  Instruction *ld = irBuilder->CreateCall(l, attr);
  wiAns->incUpdateDepend(ld, WIAnalysis::RANDOM);
  return ld;
}

/// Extract data from sampler load, repack data if needed and replace the load instruction.
void ConstantCoalescing::ReplaceLoadWithSamplerLoad(
    Instruction *loadToReplace, ///< input scattered load to replace
    Instruction *ldData,        ///< corresponding sampler load result
    uint offsetInBytes) ///< offset in bytes from the start of the sampler load to the beginning of the data to extract
{
  Type *const srcTy = ldData->getType();
  Type *const dstTy = loadToReplace->getType();
  IGC_ASSERT(nullptr != srcTy);
  IGC_ASSERT(nullptr != dstTy);
  IGC_ASSERT(srcTy->isVectorTy());
  IGC_ASSERT(cast<VectorType>(srcTy)->getElementType());
  IGC_ASSERT(cast<VectorType>(srcTy)->getElementType()->isFloatTy());

  const uint dstSizeInBytes = (unsigned int)dstTy->getPrimitiveSizeInBits() / 8;

  irBuilder->SetInsertPoint(loadToReplace);

  Value *result = nullptr;
  if (srcTy == dstTy) {
    // result is vector of 4 floats
    IGC_ASSERT(offsetInBytes == 0);
    result = ldData;
  } else if (srcTy->getPrimitiveSizeInBits() == dstTy->getPrimitiveSizeInBits()) {
    // result is a vector of one of the following:
    // - 2 64-bit values
    // - 4 32-bit integer values
    IGC_ASSERT(offsetInBytes == 0);
    result = irBuilder->CreateBitCast(ldData, dstTy);
    wiAns->incUpdateDepend(result, WIAnalysis::RANDOM);
  } else if ((dstSizeInBytes % 4) == 0 && (offsetInBytes % 4) == 0) {
    // result is one of the following:
    // - 64-bit scalar
    // - vector of 2 or 3 32-bit integer values
    // - dword aligned vector of 2 or 4 16-bit values
    // - dword aligned vector of 4 8-bit values

    // create a new vector with 2 or 3 components from sampler load data and
    // bitcast to the destination type
    const uint numElem = (dstSizeInBytes + 3) / 4;
    const uint firstElem = offsetInBytes / 4;
    result = UndefValue::get(IGCLLVM::FixedVectorType::get(cast<VectorType>(srcTy)->getElementType(), numElem));
    for (uint i = 0; i < numElem; ++i) {
      Value *element = (irBuilder->CreateExtractElement(ldData, irBuilder->getInt32(firstElem + i)));
      wiAns->incUpdateDepend(element, WIAnalysis::RANDOM);
      if (numElem == 1) {
        result = element;
      } else {
        result = irBuilder->CreateInsertElement(result, element, irBuilder->getInt32(i));
        wiAns->incUpdateDepend(result, WIAnalysis::RANDOM);
      }
    }
    if (dstTy != result->getType()) {
      result = irBuilder->CreateBitCast(result, dstTy);
      wiAns->incUpdateDepend(result, WIAnalysis::RANDOM);
    }
  } else {
    const uint numElem = (dstSizeInBytes + 3) / 4;
    const uint firstElem = offsetInBytes / 4;

    // extract up to 4 DWORDs of sampler data
    SmallVector<Value *, 4> srcData;
    for (uint i = 0; i < numElem; ++i) {
      Value *element = irBuilder->CreateExtractElement(ldData, irBuilder->getInt32(firstElem + i));
      wiAns->incUpdateDepend(element, WIAnalysis::RANDOM);
      srcData.push_back(element);
    }

    // Extracts a single element of destination data type.
    auto ExtractFromSamplerData = [this, &offsetInBytes, &srcData](Type *dstTy, uint i) -> Value * {
      IGC_ASSERT(dstTy->isIntegerTy() || dstTy->isFloatingPointTy());
      IGC_ASSERT(dstTy->getPrimitiveSizeInBits() < 64);
      const uint offsetInBits = ((offsetInBytes % 4) * 8) + ((unsigned int)dstTy->getPrimitiveSizeInBits() * i);

      IGC_ASSERT(offsetInBits + dstTy->getPrimitiveSizeInBits() <= 32);
      Value *result = irBuilder->CreateBitCast(srcData[offsetInBits / 32], irBuilder->getInt32Ty());
      wiAns->incUpdateDepend(result, WIAnalysis::RANDOM);
      if (offsetInBits > 0) {
        result = irBuilder->CreateLShr(result, irBuilder->getInt32(offsetInBits % 32));
        wiAns->incUpdateDepend(result, WIAnalysis::RANDOM);
      }
      if (dstTy->getScalarSizeInBits() < 32) {
        result = irBuilder->CreateZExtOrTrunc(
            result, IntegerType::get(dstTy->getContext(), (unsigned int)dstTy->getPrimitiveSizeInBits()));
        wiAns->incUpdateDepend(result, WIAnalysis::RANDOM);
      }
      if (result->getType() != dstTy) {
        result = irBuilder->CreateBitCast(result, dstTy);
        wiAns->incUpdateDepend(result, WIAnalysis::RANDOM);
      }
      return result;
    };

    if (dstTy->isVectorTy()) {
      result = UndefValue::get(dstTy);
      for (uint i = 0; i < cast<IGCLLVM::FixedVectorType>(dstTy)->getNumElements(); i++) {
        Value *tmpData = ExtractFromSamplerData(cast<VectorType>(dstTy)->getElementType(), i);
        result = irBuilder->CreateInsertElement(result, tmpData, irBuilder->getInt32(i));
        wiAns->incUpdateDepend(result, WIAnalysis::RANDOM);
      }
    } else {
      result = ExtractFromSamplerData(dstTy, 0);
    }
  }

  loadToReplace->replaceAllUsesWith(result);
}

// Check if any of the paths from the `dominatingChunk` to `mergeCandidate` has
// aliasing memory store or atomic operations.
bool ConstantCoalescing::CheckForAliasingWrites(uint32_t addrSpace, Instruction *dominatingChunk,
                                                Instruction *mergeCandidate) {
  BufferType accessType = DecodeBufferType(addrSpace);
  // Only writable resources need to be checked.
  if (accessType == STATELESS_READONLY || accessType == CONSTANT_BUFFER || accessType == BINDLESS_CONSTANT_BUFFER ||
      accessType == SSH_BINDLESS_CONSTANT_BUFFER) {
    return false;
  }

  const DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  const bool statefulResourcesNotAliased = m_ctx->getModuleMetaData()->statefulResourcesNotAliased;
  BasicBlock *chunkBB = dominatingChunk->getParent();
  BasicBlock *candidateBB = mergeCandidate->getParent();

  // Lambda checks if an instruction may write to memory that potentially
  // aliases the loaded memory.
  auto IsAliasedWrite = [accessType, statefulResourcesNotAliased](Instruction *inst) {
    // Treat memory fences as aliasing writes as they may change results
    // of subsequent loads.
    auto *intr = dyn_cast<GenIntrinsicInst>(inst);
    if (intr && (intr->getIntrinsicID() == GenISAIntrinsic::GenISA_memoryfence ||
                 intr->getIntrinsicID() == GenISAIntrinsic::GenISA_LSCFence)) {
      return true;
    }
    // Treat all calls to user functions that can write to memory as if they
    // could change results of subsequent loads.
    if (isUserFunctionCall(inst) && inst->mayWriteToMemory()) {
      return true;
    }

    if (!inst->mayWriteToMemory()) {
      return false;
    }
    Value *ptr = GetBufferOperand(inst);
    if (ptr == nullptr || !ptr->getType()->isPointerTy()) {
      return false;
    }
    BufferType writeAccessType = DecodeBufferType(ptr->getType()->getPointerAddressSpace());
    if (writeAccessType != STATELESS && writeAccessType != UAV && writeAccessType != BINDLESS &&
        writeAccessType != BINDLESS_WRITEONLY && writeAccessType != SSH_BINDLESS) {
      return false;
    }
    if (statefulResourcesNotAliased && writeAccessType != accessType) {
      // Note: the following improvements are possible:
      // - check BTI index for direct access to UAV/CONSTANT_BUFFER
      // - consider checking the "unique_id" part of the addressspace
      //   for bindless access
      return false;
    }
    // By default assume that the write access may alias the load
    return true;
  };

  // Lambda checks if a range of instructions has an instruction that may
  // write to memory that potentially aliases the loaded memory.
  auto HasAliasedWrite = [&IsAliasedWrite](auto beginIt, auto endIt) {
    for (auto II = beginIt, EI = endIt; II != EI; ++II) {
      if (IsAliasedWrite(&*II)) {
        return true;
      }
    }
    return false;
  };

  bool sameBB = chunkBB == candidateBB;
  // First check the merge candidate BB
  if (HasAliasedWrite(sameBB ? dominatingChunk->getIterator() : candidateBB->begin(), mergeCandidate->getIterator())) {
    return true;
  }
  if (!sameBB) {
    // Check the dominating BB
    if (HasAliasedWrite(dominatingChunk->getIterator(), chunkBB->end())) {
      return true;
    }
    // Check all potentially reachable BBs starting from predecessors of the
    // candidate merge BB and going up to to the dominating chunk BB.
    IGC_ASSERT(DT.dominates(chunkBB, candidateBB));
    SmallSet<BasicBlock *, 16> seen;
    SmallVector<BasicBlock *, 16> worklist(pred_begin(candidateBB), pred_end(candidateBB));
    seen.insert(chunkBB);
    seen.insert(candidateBB);
    while (!worklist.empty()) {
      BasicBlock *bb = worklist.pop_back_val();
      if (seen.count(bb) > 0) {
        continue;
      }
      if (HasAliasedWrite(bb->begin(), bb->end())) {
        return true;
      }
      seen.insert(bb);
      worklist.insert(worklist.end(), pred_begin(bb), pred_end(bb));
    }
  }
  return false;
}

// Checks if instructions are decorated with the same or compatible metadata.
// Returns true if metadata is compatible.
// Note: currently only LSC cache control metadata is checked.
bool ConstantCoalescing::CompareMetadata(Instruction *instA, Instruction *instB) const {
  const MDNode *nodeA = instA->getMetadata("lsc.cache.ctrl");
  const MDNode *nodeB = instB->getMetadata("lsc.cache.ctrl");
  IGC_ASSERT(nodeA == nullptr || nodeA->isUniqued());
  IGC_ASSERT(nodeB == nullptr || nodeB->isUniqued());
  return nodeA == nodeB;
}

// Copy known metadata.
// Note: currently only LSC cache control metadata is copied.
void ConstantCoalescing::CopyMetadata(Instruction *dst, Instruction *src) {
  MDNode *nodeDst = dst->getMetadata("lsc.cache.ctrl");
  MDNode *nodeSrc = src->getMetadata("lsc.cache.ctrl");
  if (nodeDst != nodeSrc) {
    IGC_ASSERT(nodeDst == nullptr);
    dst->setMetadata("lsc.cache.ctrl", nodeSrc);
  }
}

char IGC::ConstantCoalescing::ID = 0;
