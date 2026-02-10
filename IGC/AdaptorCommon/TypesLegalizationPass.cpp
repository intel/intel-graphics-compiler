/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "TypesLegalizationPass.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Support/Alignment.h"
#include <llvmWrapper/IR/Instructions.h>

using namespace llvm;

// Register pass to igc-opt
#define PASS_FLAG "types-legalization-pass"
#define PASS_DESCRIPTION "Types Legalization pass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
#define MAX_ARRAY_SIZE_THRESHOLD 8

IGC_INITIALIZE_PASS_BEGIN(TypesLegalizationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(TypesLegalizationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
char TypesLegalizationPass::ID = 0;

TypesLegalizationPass::TypesLegalizationPass() : FunctionPass(TypesLegalizationPass::ID) {
  initializeTypesLegalizationPassPass(*PassRegistry::getPassRegistry());
}

void TypesLegalizationPass::visitExtractValueInst(ExtractValueInst &ev) { m_ExtractValueInst.push_back(&ev); }

void TypesLegalizationPass::visitStoreInst(StoreInst &storeInst) {
  Value *arg = storeInst.getOperand(0);
  if (arg != NULL) {
    Type *type = arg->getType();

    if ((type != NULL) && type->isAggregateType()) {
      m_StoreInst.push_back(&storeInst);
    }
  }
}

void TypesLegalizationPass::visitPHINode(PHINode &phi) {
  Type *type = phi.getType();

  if ((type != NULL) && type->isAggregateType()) {
    m_PhiNodes.push_back(&phi);
  }
}

///////////////////////////////////////////////////////////////////////
/// @brief Creates an alloca instruction at the beginning of current
/// function
/// @param phi instruction to create the alloca for
AllocaInst *TypesLegalizationPass::CreateAlloca(Instruction *inst) {
  PHINode *phi = dyn_cast<PHINode>(inst);
  if (phi != NULL) {
    IRBuilder<> builder(phi);
    Function *curFunc = builder.GetInsertBlock()->getParent();
    BasicBlock &firstBB = curFunc->getEntryBlock();
    BasicBlock::iterator firstInsertPoint = firstBB.getFirstInsertionPt();

    builder.SetInsertPoint(&(*firstInsertPoint));
    Type *allocaType = phi->getType();
    return builder.CreateAlloca(allocaType);
  } else {
    return NULL;
  }
}

///////////////////////////////////////////////////////////////////////
/// @brief Creates GEP instruction.
/// @param  builder llvm IR builder to use
/// @param  Ty element type
/// @param  ptr memory pointer to
/// @param  indices set of constant indices to use in GEP
/// @return Value* - GetElementPtrInst* instruction.
Value *TypesLegalizationPass::CreateGEP(IGCLLVM::IRBuilder<> &builder, Type *Ty, Value *ptr,
                                        SmallVector<unsigned, 8> &indices) {
  SmallVector<Value *, 8> gepIndices;
  gepIndices.reserve(indices.size() + 1);
  gepIndices.push_back(builder.getInt32(0));
  for (unsigned idx : indices) {
    gepIndices.push_back(builder.getInt32(idx));
  }
  return builder.CreateGEP(Ty, ptr, gepIndices);
}

///////////////////////////////////////////////////////////////////////
/// @brief Compute safe alignment for a scalar load/store when splitting an aggregate.
/// @param aggTy Aggregate type being split
/// @param scalarTy Scalar element type
/// @param instAlign Alignment from load/store instruction (0 if not specified)
/// @param DL Data layout for ABI alignment queries
/// @return Align alignment that respects both the aggregate pointer's alignment
///         and the scalar type's requirements
static Align ComputeSafeScalarAlignment(Type *aggTy, Type *scalarTy, uint64_t instAlign, const DataLayout &DL) {
  // Get ABI alignment required by the scalar type
  uint64_t scalarTypeABIAlign = IGCLLVM::getAlignmentValue(IGCLLVM::getABITypeAlign(DL, scalarTy));

  // Determine effective alignment of the aggregate pointer.
  // If the instruction has no explicit alignment attribute, LLVM assumes
  // the pointer is aligned to the ABI alignment of the aggregate type.
  uint64_t aggPtrAlign = instAlign;
  if (aggPtrAlign == 0) {
    aggPtrAlign = IGCLLVM::getAlignmentValue(IGCLLVM::getABITypeAlign(DL, aggTy));
  }

  // Check if aggregate is packed (no padding between fields)
  bool isPacked = false;
  if (StructType *ST = dyn_cast<StructType>(aggTy)) {
    isPacked = ST->isPacked();
  }

  if (isPacked) {
    // Packed structs have no padding between fields, so fields may be at byte offsets
    // that don't satisfy their ABI alignment. Must use align=1 for safety.
    return IGCLLVM::getAlign(1);
  } else if (aggPtrAlign >= scalarTypeABIAlign) {
    // The pointer alignment is sufficient for the scalar's ABI alignment.
    // Non-packed structs have padding to ensure fields meet their ABI alignment,
    // so we can safely use the scalar's ABI alignment for the load/store.
    return IGCLLVM::getAlign(scalarTypeABIAlign);
  } else {
    // The pointer alignment is less than the scalar's ABI alignment.
    // This can happen when loading/storing from under-aligned aggregate pointers.
    // Use the actual pointer alignment to avoid undefined behavior.
    IGC_ASSERT(aggPtrAlign > 0);
    return IGCLLVM::getAlign(aggPtrAlign);
  }
}

///////////////////////////////////////////////////////////////////////
/// @brief Resolves a PHI node to alloca store and load.
/// @param phi instruction to resolve

void TypesLegalizationPass::ResolvePhiNode(PHINode *phi) {
  AllocaInst *allocaPtr = TypesLegalizationPass::CreateAlloca(phi);
  if (allocaPtr != NULL) {
    for (unsigned i = 0; i < phi->getNumOperands(); ++i) {
      IGCLLVM::IRBuilder<> builder(phi->getIncomingBlock(i)->getTerminator());
      Value *source = phi->getOperand(i);
      StoreInst *newStore = builder.CreateStore(source, allocaPtr);
      m_StoreInst.push_back(newStore);
    }
    IGCLLVM::IRBuilder<> builder(phi);
    BasicBlock *block = builder.GetInsertBlock();
    builder.SetInsertPoint(&(*block->getFirstInsertionPt()));
    Value *newLoad = builder.CreateLoad(allocaPtr->getAllocatedType(), allocaPtr);
    phi->replaceAllUsesWith(newLoad);
    phi->eraseFromParent();
  }
}

///////////////////////////////////////////////////////////////////////
/// @brief Resolves the use of composite types (i.e. structures or
/// arrays of vectors) in store and PHI nodes.
/// PHI node are resolved to alloca store and loads. Store instructions
/// of composite values are resolved to GEP + store of individual
/// elements of composite type.
bool TypesLegalizationPass::LegalizeTypes() {
  bool modified = false;

  // Handle PHI nodes before store instruction as the code creates
  // new store instructions.
  for (PHINode *phi : m_PhiNodes) {
    TypesLegalizationPass::ResolvePhiNode(phi);
    modified = true;
  }

  for (ExtractValueInst *ev : m_ExtractValueInst) {
    TypesLegalizationPass::ResolveExtractValue(ev);
    modified = true;
  }

  for (StoreInst *st : m_StoreInst) {
    TypesLegalizationPass::ResolveStoreInst(st);
    modified = true;
  }
  return modified;
}

///////////////////////////////////////////////////////////////////////
/// @brief Checks if an array contains null constants
/// @param  storeInst is an instruction that stores array
/// @param  indices leading to current structure filed or array elemen
/// @param  numelems is a number of elements in the array
bool TypesLegalizationPass::CheckNullArray(Instruction *storeInst) {
  Value *op0 = storeInst->getOperand(0);
  Type *type = op0->getType();

  bool isElemVectorType = type->getArrayElementType()->isVectorTy();
  bool isConstantAggregateZero = isa<ConstantAggregateZero>(op0);

  if (isElemVectorType || !isConstantAggregateZero)
    return false;

  return true;
}

///////////////////////////////////////////////////////////////////////
/// @brief Finds a structure field or array element pointed by indices
/// @param  ip insert point for new instructions
/// @param  val is a structure or array to extract the elements from
/// @param  indices specify array element or structure field to get
/// @return    Value* array element or structure field
Value *TypesLegalizationPass::ResolveValue(Instruction *ip, Value *val, SmallVector<unsigned, 8> &indices) {

  if (InsertValueInst *civ = dyn_cast<InsertValueInst>(val)) {
    ArrayRef<unsigned> civIndices = civ->getIndices();
    if (indices.size() < civIndices.size()) {
      // We have less indices collected than it is needed to reference scalar value
      // inserted here. We will skip this instruction and anticipate there is further
      // ExtractValue further that will specify missing indices.
      return nullptr;
    }
    unsigned i = 0;
    for (; i < civIndices.size(); ++i) {
      if (civIndices[i] != indices[i]) {
        break;
      }
    }
    if (i == civIndices.size()) {
      if (i == indices.size()) {
        return civ->getInsertedValueOperand();
      }
      SmallVector<unsigned, 8> ri(&indices[i], indices.end());
      return ResolveValue(civ, civ->getInsertedValueOperand(), ri);
    }
    return ResolveValue(civ, civ->getAggregateOperand(), indices);
  } else if (LoadInst *ld = dyn_cast<LoadInst>(val)) {
    IGCLLVM::IRBuilder<> builder(ld);
    Value *gep = CreateGEP(builder, ld->getType(), ld->getOperand(0), indices);
    Type *gepTy = IGCLLVM::getGEPIndexedType(ld->getType(), indices);

    // Determine safe alignment for the scalar element load
    auto loadInstAlign = IGCLLVM::getAlignmentValue(ld);
    const DataLayout &DL = ld->getModule()->getDataLayout();
    Align useAlign = ComputeSafeScalarAlignment(ld->getType(), gepTy, loadInstAlign, DL);
    return builder.CreateAlignedLoad(gepTy, gep, useAlign);
  } else if (Constant *c = dyn_cast<Constant>(val)) {
    IRBuilder<> builder(ip);
    // Create ExtractValue - it will be folded.
    return builder.CreateExtractValue(c, indices);
  } else if (ExtractValueInst *ev = dyn_cast<ExtractValueInst>(val)) {
    auto evi = ev->getIndices();
    SmallVector<unsigned, 8> newIndices(evi.begin(), evi.end());
    newIndices.append(&indices.front(), &indices.back());

    return ResolveValue(ev, ev->getAggregateOperand(), newIndices);
  } else if (auto *II = dyn_cast<IntrinsicInst>(val)) {
    switch (II->getIntrinsicID()) {
    case Intrinsic::uadd_with_overflow:
    case Intrinsic::sadd_with_overflow:
    case Intrinsic::ssub_with_overflow:
    case Intrinsic::usub_with_overflow:
    case Intrinsic::smul_with_overflow:
    case Intrinsic::umul_with_overflow: {
      // This gets handled in the legalizer.
      IRBuilder<> builder(ip);
      return builder.CreateExtractValue(val, indices);
    }
    default:
      IGC_ASSERT_MESSAGE(0, "Unsupported intrinsic instruction!");
      break;
    }
  } else if (isa<CallInst>(val) && (val->getType()->isStructTy() || val->getType()->isArrayTy())) {
    // Handle struct and array types of call instructions return value
    IRBuilder<> builder(ip);
    return builder.CreateExtractValue(val, indices);
  } else if (isa<Argument>(val)) {

    IGC_ASSERT_MESSAGE(!val->getType()->isStructTy(),
                       "Illegal IR. Structures are passed as a pointer to a struct with the byval attribute.!");

    if ((val->getType()->isArrayTy())) {
      IRBuilder<> builder(ip);
      return builder.CreateExtractValue(val, indices);
    }
  } else if (SelectInst *select = dyn_cast<SelectInst>(val)) {
    IGC_ASSERT(3 == select->getNumOperands());

    Value *condition = select->getOperand(0);
    Value *whenTrue = select->getOperand(1);
    Value *whenFalse = select->getOperand(2);

    whenTrue = ResolveValue(select, whenTrue, indices);
    whenFalse = ResolveValue(select, whenFalse, indices);

    IRBuilder<> builder(select);
    return builder.CreateSelect(condition, whenTrue, whenFalse);
  }

  // What other kind of instruction can we have here?
  IGC_ASSERT_MESSAGE(0, "Unresolved instruction!");

  // Fallback by creating an ExtractValueInstr.
  IRBuilder<> builder(ip);
  return builder.CreateExtractValue(val, indices);
}

///////////////////////////////////////////////////////////////////////
/// @brief Resolves extract value instructions of composite types into
/// sequences of GEP + load
/// @param  evInst instruction being resolved
/// @param  type    Type* type of current strucuter field or array
/// element being resolved
/// @param  indices leading to current structure filed or array element
void TypesLegalizationPass::ResolveExtractValue(ExtractValueInst *evInst) {
  IRBuilder<> builder(evInst);
  auto evIndices = evInst->getIndices();
  SmallVector<unsigned, 8> indices(evIndices.begin(), evIndices.end());

  Value *val = ResolveValue(evInst, evInst->getOperand(0), indices);
  if (val) {
    evInst->replaceAllUsesWith(val);
    evInst->eraseFromParent();
  }
}

///////////////////////////////////////////////////////////////////////
/// @brief Resolves store instructions of composite types into
/// sequences of GEP + store
/// @param  storeInst instruction to be resolved
/// @param  type    Type* type of current strucuter field or array
/// element being resolved
/// @param  indices leading to current structure filed or array element
void TypesLegalizationPass::ResolveStoreInst(StoreInst *storeInst, Type *type, SmallVector<unsigned, 8> &indices) {

  if (type->isStructTy()) {
    for (unsigned field = 0; field < type->getStructNumElements(); field++) {
      indices.push_back(field);
      ResolveStoreInst(storeInst, type->getStructElementType(field), indices);
      indices.pop_back();
    }
  } else if (type->isArrayTy()) {
    auto num = type->getArrayNumElements();
    // Check if recursion depth is 0 (indices.size() == 0) and
    // create memset instead of multiple store instructions that writes nulls
    if (!indices.size() && num >= MAX_ARRAY_SIZE_THRESHOLD && CheckNullArray(storeInst)) {
      IRBuilder<> Builder(storeInst);
      unsigned int size = type->getArrayElementType()->getScalarSizeInBits() / 8;
      auto al = IGCLLVM::getAlignmentValue(storeInst);
      Builder.CreateMemSet(storeInst->getOperand(1), Builder.getInt8(0), Builder.getInt64(num * size),
                           IGCLLVM::getAlign(al), storeInst->isVolatile());
    } else {
      for (unsigned elem = 0; elem < type->getArrayNumElements(); ++elem) {
        indices.push_back(elem);
        ResolveStoreInst(storeInst, type->getArrayElementType(), indices);
        indices.pop_back();
      }
    }
  } else {
    Value *val = ResolveValue(storeInst, storeInst->getOperand(0), indices);
    if (val) {
      IGCLLVM::IRBuilder<> builder(storeInst);

      // Determine safe alignment for the scalar element store
      Type *aggTy = storeInst->getValueOperand()->getType();
      Type *scalarTy = val->getType();
      auto storeInstAlign = IGCLLVM::getAlignmentValue(storeInst);
      const DataLayout &DL = storeInst->getModule()->getDataLayout();
      Align useAlign = ComputeSafeScalarAlignment(aggTy, scalarTy, storeInstAlign, DL);
      Value *pGEP = CreateGEP(builder, aggTy, storeInst->getOperand(1), indices);
      builder.CreateAlignedStore(val, pGEP, useAlign);
    }
  }
}

///////////////////////////////////////////////////////////////////////
/// @brief Resolves store instructions of composite types into
/// sequences of GEP + store
/// @param  storeInst instruction to be resolved
void TypesLegalizationPass::ResolveStoreInst(StoreInst *storeInst) {
  SmallVector<unsigned, 8> indices;
  Value *arg = storeInst->getOperand(0);
  if (arg != NULL) {
    ResolveStoreInst(storeInst, arg->getType(), indices);
    storeInst->eraseFromParent();
  }
}

bool TypesLegalizationPass::runOnFunction(Function &function) {
  this->visit(function);
  bool shaderModified = LegalizeTypes();
  m_StoreInst.clear();
  m_ExtractValueInst.clear();
  m_PhiNodes.clear();
  if (function.hasOptNone()) {
    // type-legalization may create dead load, clean up those loads
    // when DCE would not. Otherwise IGC cannot handle aggregate loads
    SmallVector<LoadInst *, 32> DeadLoads;
    for (inst_iterator I = inst_begin(function), E = inst_end(function); I != E; ++I) {
      Instruction *inst = &*I;
      if (LoadInst *Load = dyn_cast<LoadInst>(inst)) {
        if (!Load->isVolatile() && Load->use_empty() && Load->getType()->isAggregateType()) {
          DeadLoads.push_back(Load);
        }
      }
    }
    for (LoadInst *ld : DeadLoads) {
      ld->eraseFromParent();
    }
  }
  return shaderModified;
}
