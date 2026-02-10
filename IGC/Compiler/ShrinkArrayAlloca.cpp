/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/IRBuilder.h"

#include "common/Types.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#include "ShrinkArrayAlloca.h"

#define PASS_FLAG "shrink-array-alloca"
#define PASS_DESCRIPTION "Detect and remove unused elements of array allocas"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

namespace IGC {
using namespace llvm;
IGC_INITIALIZE_PASS_BEGIN(ShrinkArrayAllocaPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ShrinkArrayAllocaPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ShrinkArrayAllocaPass::ID = 0;

////////////////////////////////////////////////////////////////////////////////
ShrinkArrayAllocaPass::ShrinkArrayAllocaPass() : FunctionPass(ShrinkArrayAllocaPass::ID) {
  initializeShrinkArrayAllocaPassPass(*llvm::PassRegistry::getPassRegistry());
}

////////////////////////////////////////////////////////////////////////////////
llvm::StringRef ShrinkArrayAllocaPass::getPassName() const { return "ShrinkArrayAllocaPass"; }

////////////////////////////////////////////////////////////////////////////////
// @brief Returns true if some but not all elements in the input vector are set.
inline bool PariallyUsed(const SmallVector<bool, 4> &used) {
  size_t numUsed = std::count(used.begin(), used.end(), true);
  return (numUsed > 0 && numUsed < used.size());
}

////////////////////////////////////////////////////////////////////////////////
// @brief Returns the number of elements set to "true" in the input vector.
inline uint32_t NumUsed(const SmallVector<bool, 4> &used) {
  uint32_t numUsed = 0;
  std::for_each(used.begin(), used.end(), [&numUsed](const bool n) {
    if (n)
      ++numUsed;
  });
  return numUsed;
}

////////////////////////////////////////////////////////////////////////////////
// @brief Returns the number of elements in the vector type. The input type may
// be a scalar or vector or a pointer to a scalar or vector. If the input type
// is a scalar or a pointer to a scalar the function returns 1.
inline uint32_t GetNumElements(Type *type) {
  if (type->isPointerTy() && !IGCLLVM::isPointerTy(type)) {
    type = IGCLLVM::getNonOpaquePtrEltTy(type); // Legacy code: getNonOpaquePtrEltTy
  }
  IGC_ASSERT(type->isSingleValueType());
  uint32_t numElements = 1;
  if (type->isVectorTy()) {
    numElements = int_cast<uint32_t>(cast<IGCLLVM::FixedVectorType>(type)->getNumElements());
  }
  return numElements;
}

////////////////////////////////////////////////////////////////////////////////
// @brief Constructs a new vector type in the number of components is greater
// than 1. If 'numElements' is 1 the returned type is the scalar type.
inline Type *GetNewType(Type *scalarType, uint32_t numElements) {
  IGC_ASSERT(scalarType->isIntegerTy() || scalarType->isFloatingPointTy());
  return numElements == 1 ? scalarType : IGCLLVM::FixedVectorType::get(scalarType, numElements);
}

////////////////////////////////////////////////////////////////////////////////
// @brief Extracts used elements of the vector and repacks into a new vector.
inline Value *RepackToNewType(IGCLLVM::IRBuilder<> &builder, Value *data, const SmallVector<bool, 4> &used,
                              const SmallVector<uint32_t, 4> &mapping) {
  SmallVector<Value *, 4> elems;
  for (uint32_t idx = 0; idx < mapping.size(); ++idx) {
    if (used[idx]) {
      elems.push_back(builder.CreateExtractElement(data, idx));
    }
  }
  if (elems.size() == 1) {
    return elems[0];
  }
  Type *type = IGCLLVM::FixedVectorType::get(elems[0]->getType(), elems.size());
  Value *vec = llvm::UndefValue::get(type);
  for (uint32_t i = 0; i < elems.size(); i++) {
    vec = builder.CreateInsertElement(vec, elems[i], (uint64_t)i);
  }
  return vec;
}

////////////////////////////////////////////////////////////////////////////////
// @brief Extracts elements of the input vector and repacks into a bigger vector
// whose type matches the unoptimized vector type used in the alloca.
inline Value *RepackToOldType(IGCLLVM::IRBuilder<> &builder, Value *data, const SmallVector<bool, 4> &used,
                              const SmallVector<uint32_t, 4> &mapping) {
  SmallVector<Value *, 4> elems;
  uint64_t numUsed = NumUsed(used);
  if (numUsed == 1) {
    elems.push_back(data);
  } else {
    for (uint64_t idx = 0; idx < NumUsed(used); ++idx) {
      elems.push_back(builder.CreateExtractElement(data, idx));
    }
  }

  Type *type = IGCLLVM::FixedVectorType::get(elems[0]->getType(), used.size());
  Value *vec = llvm::UndefValue::get(type);
  uint32_t usedIdx = 0;
  for (uint32_t i = 0; i < used.size(); i++) {
    if (used[i]) {
      Value *elem = elems[usedIdx++];
      vec = builder.CreateInsertElement(vec, elem, (uint64_t)i);
    }
  }
  return vec;
}

////////////////////////////////////////////////////////////////////////////////
// @brief Checks uses of the input value and sets the information about accessed
// elements. Returns false if a dynamic or unsupported vector access pattern was
// found.
// Note: only some use patterns are supported, i.e.:
// - GEP access to entire vectors, not to vector elements
// - only bitcast, PHI, load, store and extract element instructions are
//   supported
inline bool GetUsedVectorElements(Value *parentPtr, Value *val, SmallVector<bool, 4> &used) {
  // Check for supported read patterns and update accesses vector elements.
  if (GetElementPtrInst *gepInst = dyn_cast<GetElementPtrInst>(val)) {
    if (gepInst->getNumIndices() != 2) {
      // Unsupported alloca use type.
      return false;
    }
    parentPtr = gepInst;
  } else if (ExtractElementInst *ee = dyn_cast<ExtractElementInst>(val)) {
    Value *indexVal = ee->getIndexOperand();
    if (isa<ConstantInt>(indexVal)) {
      uint32_t index = int_cast<uint32_t>(cast<ConstantInt>(indexVal)->getZExtValue());
      IGC_ASSERT(index < used.size());
      used[index] = true;
      return true;
    }
    // Bail early if dynamic index was found.
    return false;
  } else if (BitCastInst *bc = dyn_cast<BitCastInst>(val)) {
    // Currently supported use pattern is a bitcast to a vector with
    // the same number of components and with the scalar type of
    // the same bitwidth.
    Type *srcTy = bc->getOperand(0)->getType();
    Type *dstTy = bc->getType();
    if (srcTy->isPointerTy() != dstTy->isPointerTy()) {
      return false;
    }
    if (srcTy->isPointerTy() && dstTy->isPointerTy()) {
      if (IGCLLVM::isPointerTy(srcTy))
        return false;
      srcTy = IGCLLVM::getNonOpaquePtrEltTy(srcTy); // Legacy code: getNonOpaquePtrEltTy
      dstTy = IGCLLVM::getNonOpaquePtrEltTy(dstTy); // Legacy code: getNonOpaquePtrEltTy
    }
    if (!srcTy->isVectorTy() || !dstTy->isVectorTy()) {
      return false;
    }
    IGC_ASSERT(GetNumElements(srcTy) == used.size());
    if (GetNumElements(srcTy) != GetNumElements(dstTy) ||
        srcTy->getScalarSizeInBits() != dstTy->getScalarSizeInBits()) {
      return false;
    }
    if (srcTy->isPointerTy()) {
      parentPtr = bc;
    }
    // Supported bit cast type, check users for extract element
    // instructions.
  } else if (StoreInst *store = dyn_cast<StoreInst>(val)) {
    // This function only needs to check read access.
    if (store->getPointerOperand() == parentPtr) {
      return true;
    }
    // Bail as the entire vector (loaded from a candidate alloca) is stored
    // elsewhere.
    IGC_ASSERT(store->getValueOperand()->getType()->isVectorTy());
    return false;
  } else if (!(isa<AllocaInst>(val) || isa<PHINode>(val) || isa<InsertElementInst>(val) || isa<LoadInst>(val))) {
    return false;
  }

  // Follow the def-use chain
  for (User *user : val->users()) {
    if (!GetUsedVectorElements(parentPtr, user, used)) {
      return false;
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// @brief Replaces uses of the old operand value with the new operand value.
// Propagates the new type to the entire def-use chain.
//
// @param user user instruction to replace the operand uses in
// @param oldOp operand value whose uses need to be updated
// @param newOp operand value to replace the old operand
// @param used information about used vector elements
// @param mapping vector element indices mapping
inline void ReplaceUseWith(Value *user, Value *oldOp, Type *newOpTy, Value *newOp, const SmallVector<bool, 4> &used,
                           const SmallVector<uint32_t, 4> &mapping) {
  uint32_t numElements = NumUsed(used);
  bool isScalar = 1 == numElements;

  Value *newInst = nullptr;
  Type *newInstElTy = nullptr;
  if (GetElementPtrInst *gepInst = dyn_cast<GetElementPtrInst>(user)) {
    IGCLLVM::IRBuilder<> builder(gepInst);
    IGC_ASSERT(isa<AllocaInst>(oldOp));
    IGC_ASSERT(isa<AllocaInst>(newOp));
    IGC_ASSERT(gepInst->getNumIndices() == 2);
    SmallVector<Value *, 4> indices(gepInst->indices());

    newInst = gepInst->isInBounds() ? builder.CreateInBoundsGEP(newOpTy, newOp, indices, gepInst->getName())
                                    : builder.CreateGEP(newOpTy, newOp, indices, gepInst->getName());
    newInstElTy = cast<llvm::GetElementPtrInst>(newInst)->getResultElementType();
  } else if (LoadInst *load = dyn_cast<LoadInst>(user)) {
    IGCLLVM::IRBuilder<> builder(load);
    LoadInst *newLoad = builder.CreateLoad(newOpTy, newOp, load->getName());
    newLoad->setAlignment(IGCLLVM::getCorrectAlign(newLoad->getType()->getPrimitiveSizeInBits() / 8));
    newInst = newLoad;
    newInstElTy = newLoad->getType();
  } else if (BitCastInst *bc = dyn_cast<BitCastInst>(user)) {
    Type *bcType = bc->getType();
    if (!bcType->isPointerTy() || !IGCLLVM::isPointerTy(bcType)) {
      IGCLLVM::IRBuilder<> builder(bc);
      if (bcType->isPointerTy()) {
        bcType = IGCLLVM::getNonOpaquePtrEltTy(bcType); // Legacy code: getNonOpaquePtrEltTy
      }
      Type *scalarType = bcType->getScalarType();
      Type *newDstType = GetNewType(scalarType, numElements);
      newInstElTy = newDstType;
      if (bc->getType()->isPointerTy()) {
        newDstType = PointerType::get(newDstType, ADDRESS_SPACE_PRIVATE);
      }
      newInst = builder.CreateBitCast(newOp, newDstType, bc->getName());
    }
  } else if (ExtractElementInst *ee = dyn_cast<ExtractElementInst>(user)) {
    IGCLLVM::IRBuilder<> builder(ee);
    Value *indexVal = ee->getIndexOperand();
    IGC_ASSERT(isa<ConstantInt>(indexVal));
    uint32_t index = int_cast<uint32_t>(cast<ConstantInt>(indexVal)->getZExtValue());
    IGC_ASSERT(index < mapping.size());
    index = mapping[index];
    Value *newVal = isScalar ? newOp : builder.CreateExtractElement(newOp, (uint64_t)index, ee->getName());
    if (isa<Instruction>(newVal)) {
      cast<Instruction>(newVal)->copyMetadata(*ee);
    }
    ee->replaceAllUsesWith(newVal);
    ee->eraseFromParent();
  } else if (isa<StoreInst>(user) && oldOp == cast<StoreInst>(user)->getPointerOperand()) {
    StoreInst *st = cast<StoreInst>(user);
    IGCLLVM::IRBuilder<> builder(st);
    Value *data = st->getValueOperand();
    Value *newData = RepackToNewType(builder, data, used, mapping);
    StoreInst *newStore = builder.CreateStore(newData, newOp, st->isVolatile());
    newStore->setAlignment(IGCLLVM::getCorrectAlign(newData->getType()->getPrimitiveSizeInBits() / 8));
    newStore->copyMetadata(*st);
    st->eraseFromParent();
  } else if (Instruction *inst = dyn_cast<Instruction>(user)) {
    IGCLLVM::IRBuilder<> builder(inst);
    if (isa<PHINode>(user)) {
      builder.SetInsertPoint(cast<Instruction>(newOp)->getParent()->getTerminator());
    }
    IGC_ASSERT(oldOp->getType()->isVectorTy());
    Value *newData = RepackToOldType(builder, newOp, used, mapping);
    inst->replaceUsesOfWith(oldOp, newData);
  } else {
    IGC_ASSERT_MESSAGE(0, "Unexpected value type!");
  }
  if (newInst) {
    if (isa<Instruction>(newInst) && isa<Instruction>(user)) {
      cast<Instruction>(newInst)->copyMetadata(*(cast<Instruction>(user)));
    }
    for (auto uit = user->user_begin(), eit = user->user_end(); uit != eit;) {
      Value *user1 = *uit++;
      ReplaceUseWith(user1, user, newInstElTy, newInst, used, mapping);
    }
    cast<Instruction>(user)->eraseFromParent();
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Gets all alloca instructions whose type is an array of vector
/// Note: this pass is supposed to be run after the function inliner.
/// LLVM inliner hoists all static alloca instructions to the beginning of
/// function's entry block.
void ShrinkArrayAllocaPass::GatherAllocas(Function &F) {
  for (Instruction &I : F.getEntryBlock()) {
    AllocaInst *alloca = dyn_cast<AllocaInst>(&I);
    if (!alloca) {
      // Note: it may be necessary to change the `break` to `continue`
      // if it turns out that optimization passes insert non-alloca
      // instructions before or in between alloca instructions in the
      // entry basic block.
      break;
    }
    Type *allocatedTy = alloca->getAllocatedType();
    uint32_t as = alloca->getType()->getAddressSpace();
    if (allocatedTy->isArrayTy() && allocatedTy->getArrayElementType()->isVectorTy() && as == ADDRESS_SPACE_PRIVATE) {
      Type *arrayElementType = allocatedTy->getArrayElementType();
      uint32_t numElements = int_cast<uint32_t>(cast<IGCLLVM::FixedVectorType>(arrayElementType)->getNumElements());
      UsageInfo used;
      used.resize(numElements, false);
      Value *dummyParentVal = nullptr;
      if (GetUsedVectorElements(dummyParentVal, alloca, used) && PariallyUsed(used)) {
        m_AllocaInfo.emplace_back(std::make_pair(alloca, used));
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Replace array access to private vector variables with extract
/// and insert element instructions.
bool ShrinkArrayAllocaPass::Resolve() {
  bool modified = false;
  for (const auto &info : m_AllocaInfo) {
    // Remap indices
    auto used = info.second;
    SmallVector<uint32_t, 4> mapping;
    mapping.resize(used.size());
    uint32_t newIdx = 0;
    for (uint32_t i = 0; i < used.size(); ++i) {
      if (used[i]) {
        mapping[i] = newIdx++;
      }
    }
    AllocaInst *allocaInst = info.first;
    Type *arrayElementType = allocaInst->getAllocatedType()->getArrayElementType();

    Type *newArrayElementType = newIdx == 1 ? arrayElementType->getScalarType()
                                            : IGCLLVM::FixedVectorType::get(arrayElementType->getScalarType(), newIdx);
    Type *newType = ArrayType::get(newArrayElementType, allocaInst->getAllocatedType()->getArrayNumElements());

    IGCLLVM::IRBuilder<> builder(allocaInst);
    AllocaInst *newAlloca = builder.CreateAlloca(newType);
    newAlloca->setAlignment(IGCLLVM::getCorrectAlign(newArrayElementType->getPrimitiveSizeInBits() / 8));
    newAlloca->copyMetadata(*allocaInst);
    for (auto uit = allocaInst->user_begin(), eit = allocaInst->user_end(); uit != eit;) {
      Value *user = *uit++;
      ReplaceUseWith(user, allocaInst, newAlloca->getAllocatedType(), newAlloca, used, mapping);
    }

    allocaInst->eraseFromParent();
    modified = true;
  }
  return modified;
}

///////////////////////////////////////////////////////////////////////////////
bool ShrinkArrayAllocaPass::runOnFunction(llvm::Function &F) {
  m_AllocaInfo.clear();
  GatherAllocas(F);

  const bool modified = Resolve();

  return modified;
}
} // namespace IGC
