/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/IGCPassSupport.h"
#include "Compiler/InitializePasses.h"
#include "common/Types.hpp"
#include "ScalarizerCodeGen.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-scalarizer-in-codegen"
#define PASS_DESCRIPTION "Scalarizer in codegen"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ScalarizerCodeGen, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ScalarizerCodeGen, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ScalarizerCodeGen::ID = 0;

#define DEBUG_TYPE "ScalarizerCodeGen"

ScalarizerCodeGen::ScalarizerCodeGen() : FunctionPass(ID) {
  initializeScalarizerCodeGenPass(*PassRegistry::getPassRegistry());
}

bool ScalarizerCodeGen::runOnFunction(Function &F) {
  llvm::IRBuilder<> builder(F.getContext());
  m_builder = &builder;

  visit(F);
  return false;
}

void ScalarizerCodeGen::visitBinaryOperator(llvm::BinaryOperator &I) {
  // Scalarizing vector type And/Or instructions
  if (I.getOpcode() == Instruction::And || I.getOpcode() == Instruction::Or || I.getOpcode() == Instruction::Xor) {
    if (I.getType()->isVectorTy()) {
      bool isNewTypeVector = false;

      IGCLLVM::FixedVectorType *instType = cast<IGCLLVM::FixedVectorType>(I.getType());
      unsigned numElements = int_cast<unsigned>(instType->getNumElements());
      unsigned scalarSize = instType->getScalarSizeInBits();
      unsigned newScalarBits = numElements * scalarSize;
      Type *newType = nullptr;
      //  Check if the operands can be bitcasted to types int8/16/32
      if (newScalarBits == 8)
        newType = m_builder->getInt8Ty();
      else if (newScalarBits == 16)
        newType = m_builder->getInt16Ty();
      else if (newScalarBits == 32)
        newType = m_builder->getInt32Ty();
      else {
        // Check the suitable vector type to cast to, inorder to minimize the number of instructions
        isNewTypeVector = true;
        if (newScalarBits % 32 == 0)
          newType = IGCLLVM::FixedVectorType::get(m_builder->getInt32Ty(), newScalarBits / 32);
        else if (newScalarBits % 16 == 0)
          newType = IGCLLVM::FixedVectorType::get(m_builder->getInt16Ty(), newScalarBits / 16);
        else if (newScalarBits % 8 == 0)
          newType = IGCLLVM::FixedVectorType::get(m_builder->getInt8Ty(), newScalarBits / 8);
        else
          isNewTypeVector = false;
      }

      if (newType) {
        Value *src0 = I.getOperand(0);
        Value *src1 = I.getOperand(1);
        auto logicOp = I.getOpcode();
        m_builder->SetInsertPoint(&I);
        // bitcast the operands to new type
        Value *castedSrc0 = m_builder->CreateBitCast(src0, newType);
        Value *castedSrc1 = m_builder->CreateBitCast(src1, newType);
        Value *newBitCastInst;

        // Generate scalar logic operations, and then bitcast the result to a vector type
        if (!isNewTypeVector) {
          Value *newLogicInst = m_builder->CreateBinOp(logicOp, castedSrc0, castedSrc1);
          newBitCastInst = m_builder->CreateBitCast(newLogicInst, instType);
        } else {
          IGCLLVM::FixedVectorType *newVecType = cast<IGCLLVM::FixedVectorType>(newType);
          unsigned newVecTypeNumEle = int_cast<unsigned>(newVecType->getNumElements());
          Value *ieLogicOp = UndefValue::get(newType);
          for (unsigned i = 0; i < newVecTypeNumEle; i++) {
            Value *constIndex = ConstantInt::get(m_builder->getInt32Ty(), i);
            Value *eeSrc0 = m_builder->CreateExtractElement(castedSrc0, constIndex);
            Value *eeSrc1 = m_builder->CreateExtractElement(castedSrc1, constIndex);
            Value *newLogicInst = m_builder->CreateBinOp(logicOp, eeSrc0, eeSrc1);
            ieLogicOp = m_builder->CreateInsertElement(ieLogicOp, newLogicInst, constIndex);
          }
          newBitCastInst = m_builder->CreateBitCast(ieLogicOp, instType);
        }
        // Now replace all the instruction users with the newly bitcasted Logic Instruction
        I.replaceAllUsesWith(newBitCastInst);
        I.eraseFromParent();
      }
    }
  }
}

void ScalarizerCodeGen::visitCastInst(llvm::CastInst &I) {
  // Scalarizing vector type Trunc/Ext instructions
  if (I.getOpcode() == Instruction::Trunc || I.getOpcode() == Instruction::ZExt || I.getOpcode() == Instruction::SExt) {
    if (I.getType()->isVectorTy()) {
      IGCLLVM::FixedVectorType *instType = cast<IGCLLVM::FixedVectorType>(I.getType());
      unsigned numElements = int_cast<unsigned>(instType->getNumElements());
      Type *dstType = instType->getScalarType();
      Value *src0 = I.getOperand(0);
      auto castOp = I.getOpcode();
      m_builder->SetInsertPoint(&I);

      Value *lastOp = UndefValue::get(instType);
      for (unsigned i = 0; i < numElements; i++) {
        Value *constIndex = ConstantInt::get(m_builder->getInt32Ty(), i);
        Value *eeSrc0 = m_builder->CreateExtractElement(src0, constIndex);
        Value *newCastInst = nullptr;
        switch (castOp) {
        case Instruction::Trunc:
          newCastInst = m_builder->CreateTrunc(eeSrc0, dstType);
          break;
        case Instruction::ZExt:
          newCastInst = m_builder->CreateZExt(eeSrc0, dstType);
          break;
        case Instruction::SExt:
          newCastInst = m_builder->CreateSExt(eeSrc0, dstType);
          break;
        default:
          IGC_ASSERT(0);
        }
        lastOp = m_builder->CreateInsertElement(lastOp, newCastInst, constIndex);
      }

      // Now replace all the instruction users with the newly created instruction
      I.replaceAllUsesWith(lastOp);
      I.eraseFromParent();
    }
  }
}

void ScalarizerCodeGen::visitFNeg(llvm::UnaryOperator &I) {
  if (I.getType()->isVectorTy()) {
    IGCLLVM::FixedVectorType *InstType = cast<IGCLLVM::FixedVectorType>(I.getType());
    unsigned NumElements = int_cast<unsigned>(InstType->getNumElements());
    Value *Src = I.getOperand(0);
    m_builder->SetInsertPoint(&I);

    Value *LastOp = UndefValue::get(InstType);
    for (unsigned Idx = 0; Idx < NumElements; Idx++) {
      Value *ConstIndex = ConstantInt::get(m_builder->getInt32Ty(), Idx);
      Value *EESrc0 = m_builder->CreateExtractElement(Src, ConstIndex);
      Value *NewFNegInst = m_builder->CreateFNeg(EESrc0);
      LastOp = m_builder->CreateInsertElement(LastOp, NewFNegInst, ConstIndex);
    }

    // Now replace all the instruction users with the newly created instruction
    I.replaceAllUsesWith(LastOp);
    I.eraseFromParent();
  }
}
