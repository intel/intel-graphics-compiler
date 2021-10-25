/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "TypesLegalizationPass.hpp"
#include "../Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/Support/Alignment.h"
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

// Register pass to igc-opt
#define PASS_FLAG "types-legalization-pass"
#define PASS_DESCRIPTION "Types Legalization pass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

IGC_INITIALIZE_PASS_BEGIN( TypesLegalizationPass,PASS_FLAG,PASS_DESCRIPTION,PASS_CFG_ONLY,PASS_ANALYSIS )
IGC_INITIALIZE_PASS_END( TypesLegalizationPass,PASS_FLAG,PASS_DESCRIPTION,PASS_CFG_ONLY,PASS_ANALYSIS )
char TypesLegalizationPass::ID = 0;

TypesLegalizationPass::TypesLegalizationPass()
  : FunctionPass( TypesLegalizationPass::ID ) {
  initializeTypesLegalizationPassPass( *PassRegistry::getPassRegistry() );
}

TypesLegalizationPass::TypesLegalizationPass(bool legalizePhi, bool legalizeExtractValue, bool legalizeStore)
  : FunctionPass( TypesLegalizationPass::ID ),
    m_LegalizePhi(legalizePhi),
    m_LegalizeExtractValue(legalizeExtractValue),
    m_LegalizeStore(legalizeStore) {
  initializeTypesLegalizationPassPass( *PassRegistry::getPassRegistry() );
}

void TypesLegalizationPass::visitExtractValueInst( ExtractValueInst &ev ) {
  if(m_LegalizeExtractValue)
  {
    m_ExtractValueInst.push_back(&ev);
  }
}

void TypesLegalizationPass::visitStoreInst( StoreInst &storeInst ) {
  if(m_LegalizeStore)
  {
    Value *arg = storeInst.getOperand( 0 );
    if(arg != NULL)
    {
      Type *type = arg->getType();

      if((type != NULL) && type->isAggregateType()) {
        m_StoreInst.push_back( &storeInst );
      }
    }
  }
}

void TypesLegalizationPass::visitPHINode( PHINode &phi ) {
  if(m_LegalizePhi)
  {
    Type *type = phi.getType();

    if((type != NULL) && type->isAggregateType()) {
      m_PhiNodes.push_back( &phi );
    }
  }
}

///////////////////////////////////////////////////////////////////////
/// @brief Creates an alloca instruction at the beginning of current
/// function
/// @param phi instruction to create the alloca for
AllocaInst* TypesLegalizationPass::CreateAlloca( Instruction *inst ) {
  PHINode *phi = dyn_cast<PHINode>(inst);
  if( phi != NULL)
  {
    IRBuilder<> builder( phi );
    Function *curFunc = builder.GetInsertBlock()->getParent();
    BasicBlock &firstBB = curFunc->getEntryBlock();
    BasicBlock::iterator firstInsertPoint = firstBB.getFirstInsertionPt();

    builder.SetInsertPoint( &(*firstInsertPoint) );
    Type *allocaType = phi->getType();
    return builder.CreateAlloca( allocaType );
  }
  else
  {
    return NULL;
  }
}

///////////////////////////////////////////////////////////////////////
/// @brief Creates GEP instruction.
/// @param  builder llvm IR builder to use
/// @param  ptr memory pointer to
/// @param  indices set of constant indices to use in GEP
/// @return Value* - GetElementPtrInst* instruction.
Value* TypesLegalizationPass::CreateGEP( IGCLLVM::IRBuilder<> &builder,Value *ptr,SmallVector<unsigned,8> &indices ) {
  SmallVector<   Value *,8> gepIndices;
  gepIndices.reserve( indices.size() + 1 );
  gepIndices.push_back( builder.getInt32( 0 ) );
  for(unsigned idx : indices) {
    gepIndices.push_back( builder.getInt32( idx ) );
  }
  return builder.CreateGEP( ptr,gepIndices );
}

///////////////////////////////////////////////////////////////////////
/// @brief Resolves a PHI node to alloca store and load.
/// @param phi instruction to resolve

void TypesLegalizationPass::ResolvePhiNode( PHINode *phi ) {
  AllocaInst *allocaPtr = TypesLegalizationPass::CreateAlloca( phi );
  if(allocaPtr != NULL)
  {
    for(unsigned i = 0; i < phi->getNumOperands(); ++i)
    {
      IGCLLVM::IRBuilder<> builder( phi->getIncomingBlock( i )->getTerminator() );
      Value* source = phi->getOperand( i );
      StoreInst* newStore = builder.CreateStore( source,allocaPtr );
      m_StoreInst.push_back( newStore );
    }
    IGCLLVM::IRBuilder<> builder( phi );
    BasicBlock *block = builder.GetInsertBlock();
    builder.SetInsertPoint( &(*block->getFirstInsertionPt()) );
    Value* newLoad = builder.CreateLoad( allocaPtr );
    phi->replaceAllUsesWith( newLoad );
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
  for(PHINode *phi : m_PhiNodes) {
    TypesLegalizationPass::ResolvePhiNode( phi );
    modified = true;
  }

  for(ExtractValueInst *ev : m_ExtractValueInst) {
    TypesLegalizationPass::ResolveExtractValue( ev );
    modified = true;
  }

  for(StoreInst *st : m_StoreInst) {
    TypesLegalizationPass::ResolveStoreInst( st );
    modified = true;
  }
  return modified;
}

///////////////////////////////////////////////////////////////////////
/// @brief Finds a structure field or array element pointed by indices
/// @param  ip insert point for new instructions
/// @param  val is a structure or array to extract the elements from
/// @param  indices specify array element or structure field to get
/// @return    Value* array element or structure field
Value *
TypesLegalizationPass::ResolveValue( Instruction *ip,Value *val,SmallVector<unsigned,8> &indices ) {

  if(InsertValueInst *civ = dyn_cast<InsertValueInst>(val)) {
    ArrayRef<unsigned> civIndices = civ->getIndices();
    if(indices.size() < civIndices.size()) {
      // We have less indices collected than it is needed to reference scalar value
      // inserted here. We will skip this instruction and anticipate there is further
      // ExtractValue further that will specify missing indices.
      return nullptr;
    }
    unsigned i = 0;
    for(; i < civIndices.size(); ++i) {
      if(civIndices[i] != indices[i]) {
        break;
      }
    }
    if(i == civIndices.size()) {
      if(i == indices.size()) {
        return civ->getInsertedValueOperand();
      }
      SmallVector<unsigned,8> ri( &indices[i],indices.end() );
      return ResolveValue( civ,civ->getInsertedValueOperand(),ri );
    }
    return ResolveValue( civ,civ->getAggregateOperand(),indices );
  }
  else if(InsertElementInst* ie =
    dyn_cast<InsertElementInst>(val))
  {
    IRBuilder<> builder( ie );
    IGC_ASSERT(indices.size() == 1);
    return builder.CreateExtractElement( ie,builder.getInt32( indices[0] ) );
  }
  else if(LoadInst* ld = dyn_cast<LoadInst>(val))
  {
    IGCLLVM::IRBuilder<> builder( ld );
    Value* gep = CreateGEP( builder,ld->getOperand( 0 ),indices );
    unsigned alignment = ld->getAlignment();
    unsigned pointerTypeSize = gep->getType()->getPointerElementType()->getScalarSizeInBits() / 8;
    if ( alignment && pointerTypeSize == alignment )
      return builder.CreateAlignedLoad( gep, IGCLLVM::getAlign(alignment) );
    return builder.CreateLoad( gep );
  }
  else if(Constant *c = dyn_cast<Constant>(val)) {
    IRBuilder<> builder( ip );
    // Create ExtractValue - it will be folded.
    return builder.CreateExtractValue( c,indices );
  }
  else if(ExtractValueInst* ev =
    dyn_cast<ExtractValueInst>(val))
  {
    auto evi = ev->getIndices();
    SmallVector<unsigned,8> newIndices( evi.begin(),evi.end() );
    newIndices.append( &indices.front(),&indices.back() );

    return ResolveValue( ev,ev->getAggregateOperand(),newIndices );
  }
  else if (auto *II = dyn_cast<IntrinsicInst>(val))
  {
      switch (II->getIntrinsicID())
      {
      case Intrinsic::uadd_with_overflow:
      {
          // This gets handled in the legalizer.
          IRBuilder<> builder(ip);
          return builder.CreateExtractValue(val, indices);
      }
      default:
          break;
      }
  }
  else if ((isa<Argument>(val) || isa<CallInst>(val)) &&
    (val->getType()->isStructTy() || val->getType()->isArrayTy())) {
    // Handle struct and array types of arguments or call instructions return value
    IRBuilder<> builder(ip);
    return builder.CreateExtractValue(val, indices);
  }
  else if (PHINode* phi = dyn_cast<PHINode>(val))
  {
      IRBuilder<> builder(phi);
      PHINode* newPhi = builder.CreatePHI(ip->getType(), phi->getNumIncomingValues());
      for (unsigned i = 0; i < phi->getNumIncomingValues(); i++)
      {
          Value* v = ResolveValue(ip, phi->getIncomingValue(i), indices);
          newPhi->addIncoming(v, phi->getIncomingBlock(i));
      }
      return newPhi;
  }
  else if (SelectInst* select = dyn_cast<SelectInst>(val))
  {
    IGC_ASSERT(3 == select->getNumOperands());

    Value* condition = select->getOperand(0);
    Value* whenTrue = select->getOperand(1);
    Value* whenFalse = select->getOperand(2);

    whenTrue = ResolveValue(select, whenTrue, indices);
    whenFalse = ResolveValue(select, whenFalse, indices);

    IRBuilder<> builder(select);
    return builder.CreateSelect(condition, whenTrue, whenFalse);
  }

  // What other kind of instruction can we have here?
  IGC_ASSERT_MESSAGE(0, "Unresolved instruction!");

  // Fallback by creating an ExtractValueInstr.
  IRBuilder<> builder( ip );
  return builder.CreateExtractValue( val,indices );
}

///////////////////////////////////////////////////////////////////////
/// @brief Resolves extract value instructions of composite types into
/// sequences of GEP + load
/// @param  evInst instruction being resolved
/// @param  type    Type* type of current strucuter field or array
/// element being resolved
/// @param  indices leading to current structure filed or array element
void TypesLegalizationPass::ResolveExtractValue( ExtractValueInst* evInst )
{
  IRBuilder<>  builder( evInst );
  auto evIndices = evInst->getIndices();
  SmallVector<unsigned,8> indices( evIndices.begin(),evIndices.end() );

  Value* val = ResolveValue( evInst,evInst->getOperand( 0 ),indices );
  if(val)
  {
    evInst->replaceAllUsesWith( val );
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
void TypesLegalizationPass::ResolveStoreInst(
  StoreInst *storeInst,Type *type,
  SmallVector<unsigned,8> &indices ) {

  if(type->isStructTy()) {
    for(unsigned field = 0; field < type->getStructNumElements(); field++) {
      indices.push_back( field );
      ResolveStoreInst( storeInst,type->getStructElementType( field ),indices );
      indices.pop_back();
    }
  }
  else if(type->isArrayTy()) {
    for(unsigned elem = 0; elem < type->getArrayNumElements(); ++elem) {
      indices.push_back( elem );
      ResolveStoreInst( storeInst,type->getArrayElementType(),indices );
      indices.pop_back();
    }
  }
  else {
    Value *val =
      ResolveValue( storeInst,storeInst->getOperand( 0 ),indices );
    if (val) {
        IGCLLVM::IRBuilder<> builder(storeInst);
        Value* pGEP = CreateGEP(builder, storeInst->getOperand(1), indices);
        builder.CreateStore(val, pGEP);
    }
  }
}

///////////////////////////////////////////////////////////////////////
/// @brief Resolves store instructions of composite types into
/// sequences of GEP + store
/// @param  storeInst instruction to be resolved
void TypesLegalizationPass::ResolveStoreInst( StoreInst *storeInst ) {
  SmallVector<unsigned,8> indices;
  Value *arg = storeInst->getOperand( 0 );
  if(arg != NULL)
  {
    ResolveStoreInst( storeInst,arg->getType(),indices );
    storeInst->eraseFromParent();
  }
}

bool TypesLegalizationPass::runOnFunction( Function &function ) {
  this->visit( function );
  bool shaderModified = LegalizeTypes();
  m_StoreInst.clear();
  m_ExtractValueInst.clear();
  m_PhiNodes.clear();
  if (function.hasOptNone()) {
    // type-legalization may create dead load, clean up those loads
    // when DCE would not. Otherwise IGC cannot handle aggregate loads
    SmallVector<LoadInst*, 32> DeadLoads;
    for (inst_iterator I = inst_begin(function), E = inst_end(function);
        I != E; ++I) {
      Instruction * inst = &*I;
      if (LoadInst* Load = dyn_cast<LoadInst>(inst)) {
        if (!Load->isVolatile() && Load->use_empty() &&
          Load->getType()->isAggregateType()) {
          DeadLoads.push_back(Load);
        }
      }
    }
    for (LoadInst* ld : DeadLoads) {
      ld->eraseFromParent();
    }
  }
  return shaderModified;
}
