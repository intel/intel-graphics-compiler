/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// Utility break Constant functions for the GenX backend.
//
//===----------------------------------------------------------------------===//
#include "vc/Utils/GenX/BreakConst.h"

#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Casting.h>

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Instructions.h"

#include "vc/Utils/GenX/Region.h"
#include "vc/Utils/GenX/TypeSize.h"

#include <cstddef>
#include <iterator>

using namespace llvm;
using namespace vc;

static Value *undefVector(Type *ScalarType, unsigned ElNum) {
  IGC_ASSERT(!ScalarType->isVectorTy());
  return UndefValue::get(IGCLLVM::FixedVectorType::get(ScalarType, ElNum));
}

static Value *buildNonLegalizedConstantVector(ArrayRef<Value *> Vals,
                                              Instruction *InsertPt,
                                              const DebugLoc &DbgLoc) {
  IGC_ASSERT(!Vals.empty());
  Value *Result = undefVector(Vals.front()->getType(), Vals.size());
  IRBuilder<> Builder(InsertPt);
  Builder.SetCurrentDebugLocation(DbgLoc);
  for (unsigned j = 0, N = Vals.size(); j < N; ++j)
    Result = Builder.CreateInsertElement(Result, Vals[j], j);
  return Result;
}

static Value *buildLegalizedConstantVector(ArrayRef<Value *> Vals,
                                           Instruction *InsertPt,
                                           const DebugLoc &DbgLoc) {
  IGC_ASSERT(!Vals.empty());
  const auto& DL = InsertPt->getModule()->getDataLayout();
  Type *ScalarType = Vals.front()->getType();
  Value *Result = undefVector(ScalarType, Vals.size());
  IntegerType *I16Ty = Type::getInt16Ty(InsertPt->getContext());
  unsigned ElBytes = vc::getTypeSize(ScalarType, &DL).inBytes();
  for (unsigned j = 0, N = Vals.size(); j < N; ++j) {
    CMRegion R(Vals[j], &DL);
    R.Indirect = ConstantInt::get(I16Ty, ElBytes * j);
    Result = R.createWrRegion(Result, Vals[j], ".vconst", InsertPt, DbgLoc);
  }
  return Result;
}

Value *vc::breakConstantVector(ConstantVector *CV, Instruction *CurInst,
                               Instruction *InsertPt,
                               vc::LegalizationStage LegalizationStage) {
  IGC_ASSERT(CurInst);
  IGC_ASSERT(InsertPt);
  if (!CV)
    return nullptr;

  const auto &DbgLoc = CurInst->getDebugLoc();
  // Splat case.
  if (auto *S = dyn_cast_or_null<ConstantExpr>(CV->getSplatValue())) {
    // Turn element into an instruction
    auto Inst = S->getAsInstruction();
    Inst->setDebugLoc(DbgLoc);
    Inst->insertBefore(InsertPt);

    Value *Result = nullptr;
    // Splat the value.
    if (LegalizationStage == vc::LegalizationStage::Legalized) {
      const auto &DL = InsertPt->getModule()->getDataLayout();
      Result = CMRegion::createRdVectorSplat(DL, CV->getNumOperands(), Inst,
                                             ".splat", InsertPt, DbgLoc);
    } else {
      IRBuilder<> Builder(InsertPt);
      Builder.SetCurrentDebugLocation(DbgLoc);
      Result = Builder.CreateVectorSplat(CV->getNumOperands(), Inst);
    }

    // Update i-th operand with newly created splat.
    CurInst->replaceUsesOfWith(CV, Result);
    return Result;
  }

  SmallVector<Value *, 8> Vals;
  bool HasConstExpr = false;
  for (unsigned j = 0, N = CV->getNumOperands(); j < N; ++j) {
    Value *Elt = CV->getOperand(j);
    if (auto CE = dyn_cast<ConstantExpr>(Elt)) {
      auto Inst = CE->getAsInstruction();
      Inst->setDebugLoc(DbgLoc);
      Inst->insertBefore(InsertPt);
      Vals.push_back(Inst);
      HasConstExpr = true;
    } else
      Vals.push_back(Elt);
  }

  if (!HasConstExpr)
    return nullptr;

  Value *Val = (LegalizationStage == vc::LegalizationStage::Legalized)
                   ? buildLegalizedConstantVector(Vals, InsertPt, DbgLoc)
                   : buildNonLegalizedConstantVector(Vals, InsertPt, DbgLoc);
  CurInst->replaceUsesOfWith(CV, Val);

  return Val;
}

bool vc::breakConstantExprs(Instruction *I,
                            vc::LegalizationStage LegalizationStage) {
  if (!I)
    return false;
  bool Modified = false;
  std::list<Instruction *> Worklist = {I};
  while (!Worklist.empty()) {
    auto *CurInst = Worklist.front();
    Worklist.pop_front();
    PHINode *PN = dyn_cast<PHINode>(CurInst);
    for (unsigned i = 0, e = CurInst->getNumOperands(); i < e; ++i) {
      auto *InsertPt = PN ? PN->getIncomingBlock(i)->getTerminator() : CurInst;
      Value *Op = CurInst->getOperand(i);
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(Op)) {
        Instruction *NewInst = CE->getAsInstruction();
        NewInst->setDebugLoc(CurInst->getDebugLoc());
        NewInst->insertBefore(CurInst);
        CurInst->setOperand(i, NewInst);
        Worklist.push_back(NewInst);
        Modified = true;
      } else if (auto *CV = dyn_cast<ConstantVector>(Op)) {
        if (auto *CVInst =
                breakConstantVector(CV, CurInst, InsertPt, LegalizationStage)) {
          Worklist.push_back(cast<Instruction>(CVInst));
          Modified = true;
        }
      }
    }
  }
  return Modified;
}

bool vc::breakConstantExprs(Function *F,
                            vc::LegalizationStage LegalizationStage) {
  bool Modified = false;
  for (po_iterator<BasicBlock *> i = po_begin(&F->getEntryBlock()),
                                 e = po_end(&F->getEntryBlock());
       i != e; ++i) {
    BasicBlock *BB = *i;
    // The effect of this loop is that we process the instructions in reverse
    // order, and we re-process anything inserted before the instruction
    // being processed.
    for (Instruction *CurInst = BB->getTerminator(); CurInst;) {
      Modified |= breakConstantExprs(CurInst, LegalizationStage);
      CurInst = CurInst == &BB->front() ? nullptr : CurInst->getPrevNode();
    }
  }
  return Modified;
}
