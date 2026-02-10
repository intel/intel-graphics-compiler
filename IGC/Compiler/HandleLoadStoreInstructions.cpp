/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/HandleLoadStoreInstructions.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvmWrapper/IR/IRBuilder.h>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-dp-to-fp-load-store"
#define PASS_DESCRIPTION "Convert load/store on doubles into store/loads on i32 or float types"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(HandleLoadStoreInstructions, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(HandleLoadStoreInstructions, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char HandleLoadStoreInstructions::ID = 0;

HandleLoadStoreInstructions::HandleLoadStoreInstructions() : FunctionPass(ID) {
  initializeHandleLoadStoreInstructionsPass(*PassRegistry::getPassRegistry());
}

bool HandleLoadStoreInstructions::runOnFunction(llvm::Function &F) {
  m_changed = false;
  visit(F);
  return m_changed;
}

void HandleLoadStoreInstructions::visitLoadInst(llvm::LoadInst &I) {
  IGCLLVM::IRBuilder<> builder(&I);
  llvm::Value *ptrv = llvm::cast<llvm::LoadInst>(I).getPointerOperand();

  if (I.getType()->isDoubleTy() ||
      (I.getType()->isVectorTy() && cast<VectorType>(I.getType())->getElementType()->isDoubleTy())) {
    // scalar/vector double instruction
    // Found an instruction of type
    // %11 = load <2 x double> addrspace(8585216)* %10, align 16
    uint32_t numVectorElements = 1;
    llvm::Type *doubleDstType = builder.getDoubleTy();

    if (I.getType()->isVectorTy()) {
      numVectorElements = (uint32_t)cast<IGCLLVM::FixedVectorType>(I.getType())->getNumElements();
      doubleDstType = IGCLLVM::FixedVectorType::get(builder.getDoubleTy(), numVectorElements);
    }
    uint as = ptrv->getType()->getPointerAddressSpace();
    BufferType bufType = GetBufferType(as);
    llvm::Value *newInst = nullptr;
    // WA: driver does not support gather4(untyped surface read) from constant buffer
    if (bufType == CONSTANT_BUFFER) {
      llvm::Type *floatyptr = llvm::PointerType::get(builder.getFloatTy(), as);
      llvm::Value *byteOffset = nullptr;
      if (!isa<ConstantPointerNull>(ptrv)) {
        if (ConstantExpr *ptrExpr = dyn_cast<ConstantExpr>(ptrv)) {
          IGC_ASSERT(ptrExpr->getOpcode() == Instruction::IntToPtr);
          byteOffset = ptrExpr->getOperand(0);
        } else if (IntToPtrInst *i2p = dyn_cast<IntToPtrInst>(ptrv)) {
          byteOffset = i2p->getOperand(0);
        } else {
          byteOffset = builder.CreatePtrToInt(ptrv, builder.getInt32Ty());
        }
      }

      Value *vec = UndefValue::get(IGCLLVM::FixedVectorType::get(builder.getFloatTy(), numVectorElements * 2));
      for (unsigned int i = 0; i < numVectorElements * 2; ++i) {
        Value *offset = builder.getInt32(i * 4);
        if (byteOffset) {
          offset = builder.CreateAdd(byteOffset, offset);
        }
        // new IntToPtr and new load
        // cannot use irbuilder to create IntToPtr. It may create ConstantExpr instead of instruction
        Value *i2p = llvm::IntToPtrInst::Create(Instruction::IntToPtr, offset, floatyptr, "splitDouble", &I);
        Value *data = builder.CreateLoad(builder.getFloatTy(), i2p);
        vec = builder.CreateInsertElement(vec, data, builder.getInt32(i));
      }
      newInst = builder.CreateBitCast(vec, doubleDstType);
    }
    // End of WA
    else {
      // double to <floatx2> ; <doublex2> to <floatx4>
      llvm::Type *dataType = IGCLLVM::FixedVectorType::get(builder.getFloatTy(), numVectorElements * 2);
      llvm::PointerType *ptrType = llvm::PointerType::get(dataType, ptrv->getType()->getPointerAddressSpace());
      llvm::Value *newPtrv = builder.CreateBitCast(ptrv, ptrType);
      Value *newLoad = builder.CreateLoad(dataType, newPtrv);
      newInst = builder.CreateBitCast(newLoad, doubleDstType);
    }
    I.replaceAllUsesWith(newInst);
    I.eraseFromParent();
    m_changed = true;
  }
}

void HandleLoadStoreInstructions::visitStoreInst(llvm::StoreInst &I) {
  IGCLLVM::IRBuilder<> builder(&I);
  llvm::Value *ptrv = llvm::cast<llvm::StoreInst>(I).getPointerOperand();
  if (I.getValueOperand()->getType()->isDoubleTy() ||
      (I.getValueOperand()->getType()->isVectorTy() &&
       cast<VectorType>(I.getValueOperand()->getType())->getElementType()->isDoubleTy())) {
    // scalar/vector double instruction
    uint32_t numVectorElements = 1;

    if (I.getValueOperand()->getType()->isVectorTy()) {
      numVectorElements = (uint32_t)cast<IGCLLVM::FixedVectorType>(I.getValueOperand()->getType())->getNumElements();
    }

    // %9 = bitcast double addrspace(8519681)* %8 to <2 x float> addrspace(8519681)*
    llvm::Type *floatDatType = IGCLLVM::FixedVectorType::get(builder.getFloatTy(), numVectorElements * 2);
    llvm::PointerType *floatPtrType = llvm::PointerType::get(floatDatType, ptrv->getType()->getPointerAddressSpace());
    llvm::Value *newPtrv = builder.CreateBitCast(ptrv, floatPtrType);
    I.setOperand(I.getPointerOperandIndex(), newPtrv);

    // %10 = bitcast double %4 to <2 x float>
    llvm::Value *srcFloatInst = builder.CreateBitCast(I.getValueOperand(), floatDatType);
    I.setOperand(0, srcFloatInst);
    m_changed = true;
  }
}
