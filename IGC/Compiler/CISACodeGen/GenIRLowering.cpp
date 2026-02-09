/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// vim:ts=2:sw=2:et:
#include "common/LLVMUtils.h"
#include "Compiler/CISACodeGen/GenIRLowering.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/Analysis/TargetFolder.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/Support/KnownBits.h>
#include <llvm/Transforms/Utils/ScalarEvolutionExpander.h>
#include <llvm/Transforms/Utils/Local.h>
#include "llvmWrapper/IR/Intrinsics.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/MathExtras.h"
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "common/IGCIRBuilder.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using IGCLLVM::FixedVectorType;

namespace {
class GenIRLowering : public FunctionPass {
  using BuilderTy = IGCIRBuilder<TargetFolder>;
  BuilderTy *Builder = nullptr;

public:
  static char ID;

  GenIRLowering() : FunctionPass(ID) { initializeGenIRLoweringPass(*PassRegistry::getPassRegistry()); }

  StringRef getPassName() const override { return "GenIR Lowering"; }

  bool runOnFunction(Function &F) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<LoopInfoWrapperPass>();
  }

private:
  // Helpers
  Value *rearrangeAdd(Value *, Loop *) const;

  bool combineFMaxFMin(CallInst *GII, BasicBlock::iterator &BBI) const;
  bool combineSelectInst(SelectInst *Sel, BasicBlock::iterator &BBI) const;
  bool combinePack4i8Or2i16(Instruction *inst, uint64_t numBits) const;

  bool constantFoldFMaxFMin(CallInst *GII, BasicBlock::iterator &BBI) const;
};

char GenIRLowering::ID = 0;

// Pattern match helpers.

template <typename LHS_t, typename RHS_t, typename Pred_t> struct FMaxMinCast_match {
  unsigned &CastOpcode;
  LHS_t L;
  RHS_t R;

  FMaxMinCast_match(unsigned &Opcode, const LHS_t &LHS, const RHS_t &RHS) : CastOpcode(Opcode), L(LHS), R(RHS) {}

  bool isEqualOrCasted(Value *V, Value *Orig, unsigned Opcode) const {
    if (V == Orig)
      return true;
    // Check V is casted from Orig.
    CastInst *Cast = dyn_cast<CastInst>(V);
    if (Cast && Cast->getOpcode() == Opcode && Cast->getOperand(0) == Orig)
      return true;
    if (Constant *C = dyn_cast<Constant>(Orig)) {
      if (!CastInst::castIsValid(Instruction::CastOps(Opcode), C, V->getType()))
        return false;
      // TODO: Need to check isExact for FPToSI/FPToUI.
      Constant *Casted = ConstantExpr::getCast(Opcode, C, V->getType());
      if (V == Casted)
        return true;
    }
    return false;
  }

  template <typename OpTy> bool match(OpTy *V) {
    SelectInst *SI = dyn_cast<SelectInst>(V);
    if (!SI)
      return false;
    FCmpInst *Cmp = dyn_cast<FCmpInst>(SI->getCondition());
    if (!Cmp)
      return false;
    Value *TVal = SI->getTrueValue();
    Value *FVal = SI->getFalseValue();

    // Check cast op if any. If both operands use cast op, they should match.
    unsigned Opcode = Instruction::UserOp1;
    if (CastInst *Cast = dyn_cast<CastInst>(TVal))
      Opcode = Cast->getOpcode();
    if (CastInst *Cast = dyn_cast<CastInst>(FVal)) {
      unsigned Op = Cast->getOpcode();
      if (Opcode != Instruction::UserOp1 && Opcode != Op)
        return false;
      Opcode = Op;
    }

    Value *LHS = Cmp->getOperand(0);
    Value *RHS = Cmp->getOperand(1);
    if ((!isEqualOrCasted(TVal, LHS, Opcode) || !isEqualOrCasted(FVal, RHS, Opcode)) &&
        (!isEqualOrCasted(TVal, RHS, Opcode) || !isEqualOrCasted(FVal, LHS, Opcode)))
      return false;

    FCmpInst::Predicate Pred = Cmp->getPredicate();
    if (!isEqualOrCasted(TVal, LHS, Opcode)) {
      Pred = Cmp->getSwappedPredicate();
      std::swap(TVal, FVal);
    }
    if (!Pred_t::match(Pred))
      return false;

    if (L.match(LHS) && R.match(RHS)) {
      CastOpcode = Opcode;
      return true;
    }
    return false;
  }
};

template <typename LHS, typename RHS>
inline FMaxMinCast_match<LHS, RHS, llvm::PatternMatch::ofmax_pred_ty> m_OrdFMaxCast(unsigned &Opcode, const LHS &L,
                                                                                    const RHS &R) {
  return FMaxMinCast_match<LHS, RHS, llvm::PatternMatch::ofmax_pred_ty>(Opcode, L, R);
}

template <typename LHS, typename RHS>
inline FMaxMinCast_match<LHS, RHS, llvm::PatternMatch::ofmin_pred_ty> m_OrdFMinCast(unsigned &Opcode, const LHS &L,
                                                                                    const RHS &R) {
  return FMaxMinCast_match<LHS, RHS, llvm::PatternMatch::ofmin_pred_ty>(Opcode, L, R);
}

template <typename Op_t, typename ConstTy> struct ClampWithConstants_match {
  typedef ConstTy *ConstPtrTy;

  Op_t Op;
  ConstPtrTy &CMin, &CMax;

  ClampWithConstants_match(const Op_t &OpMatch, ConstPtrTy &Min, ConstPtrTy &Max) : Op(OpMatch), CMin(Min), CMax(Max) {}

  template <typename OpTy> bool match(OpTy *V) {
    CallInst *GII = dyn_cast<CallInst>(V);
    if (!GII)
      return false;

    EOPCODE GIID = GetOpCode(GII);
    if (GIID != llvm_max && GIID != llvm_min)
      return false;

    Value *X = GII->getOperand(0);
    Value *C = GII->getOperand(1);
    if (isa<ConstTy>(X))
      std::swap(X, C);

    ConstPtrTy C0 = dyn_cast<ConstTy>(C);
    if (!C0)
      return false;

    CallInst *GII2 = dyn_cast<CallInst>(X);
    if (!GII2)
      return false;

    EOPCODE GIID2 = GetOpCode(GII2);
    if (!(GIID == llvm_min && GIID2 == llvm_max) && !(GIID == llvm_max && GIID2 == llvm_min))
      return false;

    X = GII2->getOperand(0);
    C = GII2->getOperand(1);
    if (isa<ConstTy>(X))
      std::swap(X, C);

    ConstPtrTy C1 = dyn_cast<ConstTy>(C);
    if (!C1)
      return false;

    if (!Op.match(X))
      return false;

    CMin = (GIID2 == llvm_min) ? C0 : C1;
    CMax = (GIID2 == llvm_min) ? C1 : C0;
    return true;
  }
};

template <typename OpTy, typename ConstTy>
inline ClampWithConstants_match<OpTy, ConstTy> m_ClampWithConstants(const OpTy &Op, ConstTy *&Min, ConstTy *&Max) {
  return ClampWithConstants_match<OpTy, ConstTy>(Op, Min, Max);
}

// This pass lowers GEP into primitive ones (i.e. addition and/or
// multiplication, converted to shift if applicable) to expose address
// calculation to LLVM optimizations, such as CSE, LICM, and etc.
//
class GEPLowering : public FunctionPass {
  const DataLayout *DL = nullptr;
  CodeGenContext *m_ctx = nullptr;
  using BuilderTy = IGCIRBuilder<TargetFolder>;
  BuilderTy *Builder = nullptr;
  llvm::LoopInfo *m_LI = nullptr;
  ModuleMetaData *modMD = nullptr;
  ScalarEvolution *SE = nullptr;

public:
  static char ID;

  GEPLowering() : FunctionPass(ID) { initializeGEPLoweringPass(*PassRegistry::getPassRegistry()); }

  StringRef getPassName() const override { return "GEP Lowering"; }

  bool runOnFunction(Function &F) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.addRequired<DominatorTreeWrapperPass>();
  }

  void releaseMemory() override { truncatedInsts.clear(); }

protected:
  // Helpers
  Value *getSExtOrTrunc(Value *, Type *);
  Value *truncExpr(Value *, Type *, bool);

  bool simplifyGEP(BasicBlock &BB);
  bool lowerGetElementPtrInst(GetElementPtrInst *GEP);

private:
  DenseMap<Instruction *, Instruction *> truncatedInsts;
  DominatorTree *DT = nullptr;
};

char GEPLowering::ID = 0;

} // End anonymous namespace

bool GenIRLowering::runOnFunction(Function &F) {
  // Skip non-kernel function.
  MetaDataUtils *MDU = nullptr;
  MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  auto FII = MDU->findFunctionsInfoItem(&F);
  if (FII == MDU->end_FunctionsInfo())
    return false;

  auto &DL = F.getParent()->getDataLayout();

  BuilderTy TheBuilder(F.getContext(), TargetFolder(DL));
  Builder = &TheBuilder;

  bool Changed = false;

  // Replace SLM PtrToInt by the assigned immed offset
  // Later optimization (InstCombine) can fold away some address computation

  FunctionMetaData *funcMD = &modMD->FuncMD[&F];

  for (auto localOffetsItr = funcMD->localOffsets.begin(), localOffsetsEnd = funcMD->localOffsets.end();
       localOffetsItr != localOffsetsEnd; ++localOffetsItr) {
    LocalOffsetMD localOffset = *localOffetsItr;

    // look up the value-to-offset mapping
    Value *V = localOffset.m_Var;
    unsigned Offset = localOffset.m_Offset;

    // Skip non-pointer values.
    if (!V->getType()->isPointerTy())
      continue;
    // Skip non-local pointers.
    unsigned AS = V->getType()->getPointerAddressSpace();
    if (AS != ADDRESS_SPACE_LOCAL)
      continue;

    // It is possible that a global (slm) is used in more than one kernels
    // and each kernel might have a different offset for this global. Thus,
    // we can only replace the uses within this kernel function. We will check
    // instructions only as the constant expressions have been broken up
    // before this pass.
    PointerType *PTy = cast<PointerType>(V->getType());
    Constant *CO = ConstantInt::get(Type::getInt32Ty(F.getContext()), Offset);
    Constant *NewBase = ConstantExpr::getIntToPtr(CO, PTy);
    auto NI = V->user_begin();
    for (auto I = NI, E = V->user_end(); I != E; I = NI) {
      ++NI;
      Instruction *Inst = dyn_cast<Instruction>(*I);
      if (!Inst || Inst->getParent()->getParent() != &F) {
        continue;
      }

      // As constant exprs have been broken up, need to check insts only.
      if (GetElementPtrInst *GEPI = dyn_cast<GetElementPtrInst>(Inst)) {
        // sanity check
        if (GEPI->getOperand(0) == V) {
          // operand 0 is pointer operand
          GEPI->setOperand(0, NewBase);
          Changed = true;
        }
      } else if (PtrToIntInst *PI = dyn_cast<PtrToIntInst>(Inst)) {
        Value *CI = ConstantInt::get(PI->getType(), Offset);
        PI->replaceAllUsesWith(CI);
        PI->eraseFromParent();
        Changed = true;
      } else if (BitCastInst *BCI = dyn_cast<BitCastInst>(Inst)) {
        BCI->setOperand(0, NewBase);
        Changed = true;
      } else if (LoadInst *LI = dyn_cast<LoadInst>(Inst)) {
        LI->setOperand(0, NewBase);
        Changed = true;
      } else if (StoreInst *SI = dyn_cast<StoreInst>(Inst)) {
        if (SI->getPointerOperand() == V) {
          // pointer operand is operand 1!
          SI->setOperand(1, NewBase);
          Changed = true;
        }
      } else if (CallInst *CI = dyn_cast<CallInst>(Inst)) {
        CI->replaceUsesOfWith(V, NewBase);
        Changed = true;
      } else if (CmpInst *CI = dyn_cast<CmpInst>(Inst)) {
        CI->replaceUsesOfWith(V, NewBase);
        Changed = true;
      }
    }
  }

  for (auto &BB : F) {
    for (auto BI = BB.begin(), BE = BB.end(); BI != BE;) {
      Instruction *Inst = &(*BI++);
      Builder->SetInsertPoint(Inst);

      switch (Inst->getOpcode()) {
      default: // By default, DO NOTHING
        break;
      case Instruction::Call:
        if (CallInst *GII = dyn_cast<CallInst>(Inst)) {
          switch (GetOpCode(GII)) {
          case llvm_max:
          case llvm_min:
            Changed |= combineFMaxFMin(GII, BI);
            break;
          default:
            break;
          }
        }
        break;
      case Instruction::Select:
        // Enable the pattern match only when NaNs can be ignored.
        if (modMD->compOpt.NoNaNs || modMD->compOpt.FiniteMathOnly) {
          Changed |= combineSelectInst(cast<SelectInst>(Inst), BI);
        }
        break;
      case Instruction::Or:
        if (Inst->getType()->isIntegerTy(32)) {
          // Detect packing of 4 i8 values and convert to a pattern that is
          // matched CodeGenPatternMatch::MatchPack4i8
          Changed |= combinePack4i8Or2i16(Inst, 8 /*numBits*/);
          // TODO: also detect <2 x i16> packing once PatternMatch is updated
          // to packing of 16-bit values.
        }
        break;
      }
    }
  }

  Builder = nullptr;

  return Changed;
}

// For each basic block, simplify GEPs based on the analysis result from SCEV.
bool GEPLowering::simplifyGEP(BasicBlock &BB) {
  // Pointers with the form base + zext(idx).
  struct PointerExpr {
    GetElementPtrInst *GEP;
    const SCEV *Idx;
    GetElementPtrInst *Base = nullptr; // A simplified offset if any.
    const SCEV *Offset = nullptr;      // A simplified offset if any.
  };
  // Each visited base pointer have a collection of base expr.
  DenseMap<Value *, SmallVector<PointerExpr, 128>> Pointers;

  auto IsUsedByBindless = [](const GetElementPtrInst *GEP) {
    for (auto *U : GEP->users())
      if (auto *P2I = dyn_cast<PtrToIntInst>(U))
        if (P2I->getType()->isIntegerTy(32))
          return true;
    return false;
  };

  bool Changed = false;
  for (auto BI = BB.begin(), BE = BB.end(); BI != BE; ++BI) {
    auto *GEP = dyn_cast<GetElementPtrInst>(BI);
    // So far, for simplicity, consider GEPs on the generic/global address
    // with a single index only. It should be straight-forward to extend
    // the support to other cases, where multiple indices are present.
    if (!GEP || !GEP->isInBounds() || GEP->getNumIndices() != 1 ||
        (GEP->getAddressSpace() != ADDRESS_SPACE_GLOBAL && GEP->getAddressSpace() != ADDRESS_SPACE_GENERIC))
      continue;
    if (IsUsedByBindless(GEP))
      continue;
    auto *Idx = GEP->getOperand(1);
    if (auto *ZExt = dyn_cast<ZExtInst>(Idx)) {
      Idx = ZExt->getOperand(0);
    } else if (auto *SExt = dyn_cast<SExtInst>(Idx)) {
      Idx = SExt->getOperand(0);
      Operator *Opr = dyn_cast<Operator>(Idx);
      if (Opr && Opr->getOpcode() == BinaryOperator::BinaryOps::SDiv) {
        // Skip if it is SDiv. This special check is needed as
        // OverflowingBinaryOperator does not include SDiv
        continue;
      }
      auto *Op = dyn_cast<OverflowingBinaryOperator>(Idx);
      if (Op && !Op->hasNoSignedWrap())
        continue;
    }

    const SCEV *E = SE->getSCEV(Idx);
    // Skip if the offset to the base is already a constant.
    if (isa<SCEVConstant>(E))
      continue;
    Value *Base = GEP->getPointerOperand();
    auto &Exprs = Pointers[Base];

    auto EI = Exprs.begin();
    auto EE = Exprs.end();
    const SCEV *Offset = nullptr;

    // Let GEP_a be one gep from Pointers[Base];
    // GEP (it is 'GEP' var in this loop iteration) reuses GEP_a's address
    // as its base
    //   1. if GEP_a is the first in Pointer[Base] such that diff of GEP_a
    //      and GEP is constant; otherwise
    //   2. if GEP_a is the first in Pointers[Base] such that diff of GEP_a
    //      and GEP is 1 (a single value), otherwise
    //   3. if GEP_a has the smallest diff or if more than one GEPs with the
    //      same diff, GEP_a is the last one in Pointers[Base].
    // Both 1 and 2 may potentially save a few instructions. 3 is a
    // heuristic and may be further tuned.
    constexpr unsigned DIFF_SIZE_THRESHOLD = 3;
    unsigned MinDiff = DIFF_SIZE_THRESHOLD;
    bool isDiffOne = false;
    GetElementPtrInst *BaseWithMinDiff = nullptr;
    for (/*EMPTY*/; EI != EE; ++EI) {
      // Skip if the result types do not match.
      if (EI->GEP->getType() != GEP->getType() || E->getType() != EI->Idx->getType())
        continue;

      auto *Diff = SE->getMinusSCEV(E, EI->Idx);
      unsigned exprSize = Diff->getExpressionSize();
      if (exprSize <= MinDiff) {
        if (isa<SCEVConstant>(Diff)) {
          BaseWithMinDiff = EI->GEP;
          Offset = Diff;
          MinDiff = exprSize;
          break;
        }
        if (!isDiffOne) {
          BaseWithMinDiff = EI->GEP;
          Offset = Diff;
          MinDiff = exprSize;
          isDiffOne = (MinDiff == 1);
        }
      }
    }
    // Not found, add this GEP as a potential base expr.
    if (!Offset) {
      Exprs.emplace_back(PointerExpr{GEP, E, nullptr, nullptr});
      continue;
    }
    Exprs.emplace_back(PointerExpr{GEP, E, BaseWithMinDiff, Offset});
  }
  std::vector<Instruction *> DeadInsts;
  for (const auto &B : Pointers) {
    for (auto PI = B.second.rbegin(), PE = B.second.rend(); PI != PE; ++PI) {
      auto &P = *PI;
      if (P.Offset) {
        SCEVExpander E(*SE, *DL, "gep-simplification");
        Value *V = E.expandCodeFor(P.Offset, P.Idx->getType(), P.GEP);
        Builder->SetInsertPoint(P.GEP);
        auto *NewGEP = Builder->CreateInBoundsGEP(P.Base->getResultElementType(), P.Base,
                                                  Builder->CreateSExt(V, P.GEP->getOperand(1)->getType()));
        P.GEP->replaceAllUsesWith(NewGEP);
        DeadInsts.push_back(P.GEP);
        Changed = true;
      }
    }
  }
  for (auto *I : DeadInsts)
    RecursivelyDeleteTriviallyDeadInstructions(I);
  return Changed;
}

bool GEPLowering::runOnFunction(Function &F) {
  // Skip non-kernel function.
  modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  m_LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  MetaDataUtils *MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  auto FII = MDU->findFunctionsInfoItem(&F);
  if (FII == MDU->end_FunctionsInfo())
    return false;

  CodeGenContextWrapper *pCtxWrapper = &getAnalysis<CodeGenContextWrapper>();
  m_ctx = pCtxWrapper->getCodeGenContext();

  DL = &F.getParent()->getDataLayout();
  SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  BuilderTy TheBuilder(F.getContext(), TargetFolder(*DL));
  Builder = &TheBuilder;

  bool Changed = false;

  // For efficient64b, GEP simplification may be not needed or
  // need different tuning hueristic
  if (!m_ctx->platform.hasEfficient64bEnabled() && IGC_IS_FLAG_ENABLED(EnableGEPSimplification)) {
    for (auto &BB : F)
      Changed |= simplifyGEP(BB);

    if (IGC_IS_FLAG_ENABLED(TestGEPSimplification))
      return Changed;
  }

  for (auto &BB : F) {
    for (auto BI = BB.begin(), BE = BB.end(); BI != BE;) {
      Instruction *Inst = &(*BI++);
      Builder->SetInsertPoint(Inst);

      switch (Inst->getOpcode()) {
      default: // By default, DO NOTHING
        break;
      // Lower GEPs to inttoptr/ptrtoint with offsets.
      case Instruction::GetElementPtr:
        Changed |= lowerGetElementPtrInst(cast<GetElementPtrInst>(Inst));
        break;
      }
    }
  }

  return Changed;
}

Value *GEPLowering::getSExtOrTrunc(Value *Val, Type *NewTy) {
  Type *OldTy = Val->getType();
  unsigned OldWidth;
  unsigned NewWidth;

  IGC_ASSERT_MESSAGE(OldTy->isIntOrIntVectorTy(), "Index should be Integer or vector of Integer!");

  if (auto OldVecTy = dyn_cast<IGCLLVM::FixedVectorType>(OldTy)) {
    OldWidth = (unsigned)OldVecTy->getNumElements() * OldVecTy->getElementType()->getIntegerBitWidth();
    NewWidth = (unsigned)OldVecTy->getNumElements() * NewTy->getIntegerBitWidth();
  } else {
    OldWidth = OldTy->getIntegerBitWidth();
    NewWidth = NewTy->getIntegerBitWidth();
  }

  if (OldWidth < NewWidth) { // SExt
    return Builder->CreateSExt(Val, NewTy);
  }

  if (OldWidth > NewWidth) { // Trunc
    return truncExpr(Val, NewTy, true);
  }

  return Val;
}

Value *GEPLowering::truncExpr(Value *Val, Type *NewTy, bool initialVal = false) {
  // Truncation on Gen could be as cheap as NOP by creating the proper region.
  // Instead of truncating the value itself unless it has multiple users, try to truncate how it's
  // calculated.
  if (Constant *C = dyn_cast<Constant>(Val))
    return Builder->CreateIntCast(C, NewTy, false);

  if (!isa<Instruction>(Val))
    return Builder->CreateTrunc(Val, NewTy);

  auto dominatesInst = [&](const llvm::Instruction *A, const llvm::Instruction *B) -> bool {
    if (!A || !B) {
      return false;
    }
    return DT->dominates(A, B);
  };

  auto getTruncatedInst = [&](llvm::Instruction *I) -> Instruction * {
    auto truncatedInstIt = truncatedInsts.find(I);
    if (truncatedInstIt == truncatedInsts.end()) {
      return nullptr;
    }
    if (!dominatesInst(truncatedInstIt->second, I)) {
      truncatedInstIt->second->moveAfter(I);
    }

    return truncatedInstIt->second;
  };

  auto getOrCreateTruncForOp = [&](Value *Op, Type *NewTy) -> Value * {
    if (isa<Instruction>(Op)) {
      auto *truncatedIt = getTruncatedInst(cast<Instruction>(Op));
      if (truncatedIt) {
        return truncatedIt;
      }
    }
    return truncExpr(Op, NewTy);
  };

  auto *I = cast<Instruction>(Val);
  unsigned Opc = I->getOpcode();
  switch (Opc) {
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor: {
    // If the value is used in multiple places, we can just re-use it without duplicating calculation since it can not
    // be removed.
    if (initialVal && IGC_IS_FLAG_ENABLED(GEPLoweringTruncOptEnabled)) {
      auto *truncatedIt = getTruncatedInst(I);
      if (truncatedIt) {
        return truncatedIt;
      }
      if (llvm::any_of(Val->users(), [](const llvm::User *U) { return !llvm::isa<llvm::GetElementPtrInst>(U); })) {
        auto *newInst = Builder->CreateTrunc(Val, NewTy);
        truncatedInsts[I] = cast<Instruction>(newInst);
        return newInst;
      }
    }
    BinaryOperator *BO = cast<BinaryOperator>(I);
    Value *LHS = getOrCreateTruncForOp(BO->getOperand(0), NewTy);
    Value *RHS = getOrCreateTruncForOp(BO->getOperand(1), NewTy);
    return Builder->CreateBinOp(BO->getOpcode(), LHS, RHS);
  }
  case Instruction::Trunc:
  case Instruction::ZExt:
  case Instruction::SExt: {
    Value *Opnd = I->getOperand(0);
    if (Opnd->getType() == NewTy)
      return Opnd;
    return Builder->CreateIntCast(Opnd, NewTy, Opc == Instruction::SExt);
  }
  case Instruction::Select: {
    Value *TVal = truncExpr(I->getOperand(1), NewTy);
    Value *FVal = truncExpr(I->getOperand(2), NewTy);
    return Builder->CreateSelect(I->getOperand(0), TVal, FVal);
  }
#if 0
                              // TODO: Rewrite truncExpr into iterative one instead of recursive one to
                              // easily found the loop due to phi-node.
    case Instruction::PHI: {
        PHINode* PN = cast<PHINode>(I);
        PHINode* Res = PHINode::Create(NewTy, PN->getNumIncomingValues());
        for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
            Value* V = truncExpr(PN->getIncomingValue(i), NewTy);
            Res->addIncoming(V, PN->getIncomingBlock(i));
        }
        return Res;
    }
#endif
  default:
    // Don't know truncate its calculation safely, fall back to the regular
    // way.
    break;
  }

  return Builder->CreateTrunc(Val, NewTy);
}

//
// reassociate chain of address adds so that the loop invariant terms appear on RHS tree
//
Value *GenIRLowering::rearrangeAdd(Value *val, Loop *loop) const {
  BinaryOperator *binOp = dyn_cast<BinaryOperator>(val);
  if (!binOp || binOp->getOpcode() != Instruction::Add) {
    return val;
  }

  Value *LHS = binOp->getOperand(0);
  Value *RHS = binOp->getOperand(1);

  if (loop->isLoopInvariant(LHS)) {
    Value *newRHS = rearrangeAdd(binOp->getOperand(1), loop);
    if (!loop->isLoopInvariant(newRHS)) {
      BinaryOperator *RHSBinOp = dyn_cast<BinaryOperator>(newRHS);
      if (RHSBinOp && RHSBinOp->getOpcode() == Instruction::Add) {
        // LI + (a + b) --> a + (b + LI)
        Value *LHSofNewRHS = RHSBinOp->getOperand(0);
        Value *RHSofNewRHS = RHSBinOp->getOperand(1);
        return Builder->CreateAdd(LHSofNewRHS, Builder->CreateAdd(RHSofNewRHS, LHS));
      }
    }

    // LI + a --> a + LI
    return Builder->CreateAdd(newRHS, LHS);
  } else {
    Value *newLHS = rearrangeAdd(LHS, loop);
    BinaryOperator *LHSBinOp = dyn_cast<BinaryOperator>(newLHS);
    if (LHSBinOp && LHSBinOp->getOpcode() == Instruction::Add) {
      Value *LHSofLHS = LHSBinOp->getOperand(0);
      Value *RHSofLHS = LHSBinOp->getOperand(1);
      if (loop->isLoopInvariant(RHSofLHS)) {
        // (a + LI) + b --> a + (b + LI)
        return Builder->CreateAdd(LHSofLHS, rearrangeAdd(Builder->CreateAdd(RHS, RHSofLHS), loop));
      }
    }

    return Builder->CreateAdd(newLHS, rearrangeAdd(RHS, loop));
  }
}

// Returns true if the value does not originate from a ptrtoint of a generic address space (AS4) pointer. Generic
// pointers carry tag bits in the upper bits that encode the source address space. Reusing such an integer across an
// addrspacecast (which would strip the tags) is incorrect.
//
// This traces the value backwards through arithmetic, phi nodes, selects, and integer casts. If any path reaches a
// ptrtoint of a generic pointer the value is considered tainted. If all paths reach safe leaves (loads, calls,
// constants, non-generic ptrtoint, arguments, etc.) the value is considered tag-free and safe to reuse.
static bool isFreeOfGenericPtrTagBits(Value *V) {
  SmallPtrSet<Value *, 16> Visited;
  SmallVector<Value *, 8> Worklist;
  Worklist.push_back(V);

  while (!Worklist.empty()) {
    Value *Cur = Worklist.pop_back_val();
    if (!Visited.insert(Cur).second)
      continue; // Already visited, skip to break cycles.

    // If found ptrtoint, check the pointer's address space.
    if (auto *P2I = dyn_cast<PtrToIntInst>(Cur)) {
      if (P2I->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC)
        return false; // Tainted by generic-pointer tag bits.
      continue;       // Non-generic ptrtoint is a safe leaf.
    }

    // Safe leaf values that do not propagate pointer-derived tag bits.
    if (isa<Constant>(Cur) || isa<LoadInst>(Cur) || isa<CallInst>(Cur) || isa<Argument>(Cur) ||
        isa<ExtractElementInst>(Cur) || isa<ExtractValueInst>(Cur))
      continue;

    // Binary arithmetic, both operands may contribute bits.
    if (auto *BinOp = dyn_cast<BinaryOperator>(Cur)) {
      Worklist.push_back(BinOp->getOperand(0));
      Worklist.push_back(BinOp->getOperand(1));
      continue;
    }

    // PHI nodes, all incoming values.
    if (auto *Phi = dyn_cast<PHINode>(Cur)) {
      for (Value *Inc : Phi->incoming_values())
        Worklist.push_back(Inc);
      continue;
    }

    // Select, both true and false values.
    if (auto *Sel = dyn_cast<SelectInst>(Cur)) {
      Worklist.push_back(Sel->getTrueValue());
      Worklist.push_back(Sel->getFalseValue());
      continue;
    }

    // Integer casts (zext, sext, trunc).
    if (isa<ZExtInst>(Cur) || isa<SExtInst>(Cur) || isa<TruncInst>(Cur)) {
      Worklist.push_back(cast<Instruction>(Cur)->getOperand(0));
      continue;
    }

    // Treat unknown instructions conservatively as tainted.
    return false;
  }

  // Every path reached a safe leaf. The value does not carry generic-pointer tag bits.
  return true;
}

bool GEPLowering::lowerGetElementPtrInst(GetElementPtrInst *GEP) {
  Value *const PtrOp = GEP->getPointerOperand();
  IGC_ASSERT(nullptr != PtrOp);
  PointerType *const PtrTy = dyn_cast<PointerType>(PtrOp->getType());
  IGC_ASSERT_MESSAGE(nullptr != PtrTy, "Only accept scalar pointer!");

  unsigned pointerSizeInBits = m_ctx->getRegisterPointerSizeInBits(PtrTy->getAddressSpace());
  unsigned pointerMathSizeInBits = pointerSizeInBits;
  bool reducePointerArith = false;
  bool canReduceNegativeOffset = false;

  // Detect if we can do intermediate pointer arithmetic in 32bits
  if (pointerMathSizeInBits == 64 && GEP->isInBounds()) {
    if (!modMD->compOpt.GreaterThan4GBBufferRequired) {
      bool gepProducesPositivePointer = true;

      // prove that the offset from the base pointer will be positive.  if we cannot
      // prove that all parameters to GEP increase the address of the final calculation
      // we can't fall back to 32bit math
      for (auto U = GEP->idx_begin(), E = GEP->idx_end(); U != E; ++U) {
        Value *Idx = U->get();

        if (Idx != GEP->getPointerOperand()) {
          gepProducesPositivePointer &= valueIsPositive(Idx, DL);
        }
      }

      if (gepProducesPositivePointer) {
        pointerMathSizeInBits = 32;
        reducePointerArith = true;
      }
    } else if (GEP->getAddressSpace() == ADDRESS_SPACE_CONSTANT || !modMD->compOpt.GreaterThan2GBBufferRequired) {
      canReduceNegativeOffset = true;
      pointerMathSizeInBits = m_ctx->platform.hasLargeMaxConstantBufferSize() ? 64 : 32;
      reducePointerArith = true;
    }
  }

  IntegerType *IntPtrTy = IntegerType::get(Builder->getContext(), pointerSizeInBits);
  IntegerType *PtrMathTy = IntegerType::get(Builder->getContext(), pointerMathSizeInBits);

  Value *BasePointer = nullptr;

  // Check if the pointer itself is created from IntToPtr. If it is, and if the int is the same size, we can use the int
  // directly. Otherwise, we need to add PtrToInt.

  Value *PtrSource = PtrOp;

  // First, look through address space casts. This is only valid if the value is free of generic pointer tag bits (from
  // ptrtoint of AS4).
  AddrSpaceCastInst *ASC = dyn_cast<AddrSpaceCastInst>(PtrSource);
  if (ASC)
    PtrSource = ASC->getOperand(0);

  if (IntToPtrInst *I2PI = dyn_cast<IntToPtrInst>(PtrSource)) {
    Value *IntOp = I2PI->getOperand(0);
    if (IntOp->getType() == IntPtrTy) {
      if (!ASC || isFreeOfGenericPtrTagBits(IntOp))
        BasePointer = IntOp;
    }
  }
  if (!BasePointer) {
    BasePointer = Builder->CreatePtrToInt(PtrOp, IntPtrTy);
  }
  // This is the value of the pointer, which will ultimately replace
  // getelementptr.
  Value *PointerValue = nullptr;
  if (reducePointerArith) {
    // in case the pointer arithmetic is done in lower type postpone adding the base to the end
    PointerValue = ConstantInt::get(PtrMathTy, 0);
  } else {
    PointerValue = BasePointer;
  }

  gep_type_iterator GTI = gep_type_begin(GEP);
  for (auto OI = GEP->op_begin() + 1, E = GEP->op_end(); OI != E; ++OI, ++GTI) {
    Value *Idx = *OI;
    if (StructType *StTy = GTI.getStructTypeOrNull()) {
      unsigned Field = int_cast<unsigned>(cast<ConstantInt>(Idx)->getZExtValue());
      if (Field) {
        uint64_t Offset = DL->getStructLayout(StTy)->getElementOffset(Field);
        Value *OffsetValue = Builder->getInt(APInt(pointerMathSizeInBits, Offset));
        PointerValue = Builder->CreateAdd(PointerValue, OffsetValue);
      }
    } else {
      Type *Ty = GTI.getIndexedType();

      if (const ConstantInt *CI = dyn_cast<ConstantInt>(Idx)) {
        if (!CI->isZero()) {
          uint64_t Offset = DL->getTypeAllocSize(Ty) * CI->getSExtValue();
          Value *OffsetValue = Builder->getInt(APInt(pointerMathSizeInBits, Offset));
          PointerValue = Builder->CreateAdd(PointerValue, OffsetValue);
        }
      } else {
        Value *NewIdx = getSExtOrTrunc(Idx, PtrMathTy);
        APInt ElementSize = APInt(pointerMathSizeInBits, DL->getTypeAllocSize(Ty));

        ConstantInt *COffset = nullptr;
        if (IGC_IS_FLAG_ENABLED(EnableSimplifyGEP) && NewIdx->hasOneUse()) {
          // When EnableSimplifyGEP is on, GEP's index can be of V + C
          // where C is constant. If so, we will continue push C up to
          // the top so CSE could do better job.

          //
          // Replace
          //   %nswAdd = add nsw i32 %49, 195
          //   %NewIdx = sext i32 %nswAdd to i64
          //   %PointerValue = %NewIdx * 4 + %Base
          // with
          //   %NewIdx = sext i32 %49
          //   %PointerValue = (%NewIdx * 4 + %Base) + (4 * 195)
          // for later CSE.
          //

          bool performSExt = false;
          if (SExtInst *I = dyn_cast<SExtInst>(NewIdx)) {
            if (OverflowingBinaryOperator *nswAdd = dyn_cast<OverflowingBinaryOperator>(I->getOperand(0))) {
              if ((nswAdd->getOpcode() == Instruction::Add) && nswAdd->hasNoSignedWrap() &&
                  isa<ConstantInt>(nswAdd->getOperand(1))) {
                performSExt = true;
                NewIdx = nswAdd;
              }
            }
          }
          if (Instruction *Inst = dyn_cast<Instruction>(NewIdx)) {
            if (Inst->getOpcode() == Instruction::Add) {
              COffset = dyn_cast<ConstantInt>(Inst->getOperand(1));
              if (COffset) {
                NewIdx = Inst->getOperand(0);
                int64_t cval = COffset->getSExtValue() * ElementSize.getZExtValue();
                COffset = ConstantInt::get(PtrMathTy, cval);
              }
            }
          }
          if (performSExt) {
            NewIdx = Builder->CreateSExt(NewIdx, PtrMathTy);
          }
        }

        if (BinaryOperator *binaryOp = dyn_cast<BinaryOperator>(NewIdx)) {
          // detect the pattern
          // GEP base, a + b
          // where base and a are both loop invariant (but not b), so we could rearrange the lowered code into
          // (base + (a << shftAmt)) + (b << shftAmt)
          // For now we only look at one level
          Loop *loop = m_LI ? m_LI->getLoopFor(binaryOp->getParent()) : nullptr;
          if (loop != nullptr && loop->isLoopInvariant(PtrOp) && binaryOp->getOpcode() == Instruction::Add) {

            Value *LHS = binaryOp->getOperand(0);
            Value *RHS = binaryOp->getOperand(1);
            bool isLHSLI = loop->isLoopInvariant(LHS);
            bool isRHSLI = loop->isLoopInvariant(RHS);

            auto reassociate = [&](Value *invariant, Value *other) {
              Value *invariantVal = nullptr;
              if (ElementSize == 1) {
                invariantVal = invariant;
              } else if (ElementSize.isPowerOf2()) {
                invariantVal = Builder->CreateShl(invariant, APInt(pointerMathSizeInBits, ElementSize.logBase2()));
              } else {
                invariantVal = Builder->CreateMul(invariant, Builder->getInt(ElementSize));
              }
              PointerValue = Builder->CreateAdd(PointerValue, invariantVal);
              NewIdx = other;
            };
            if (isLHSLI && !isRHSLI) {
              reassociate(LHS, RHS);
            } else if (!isLHSLI && isRHSLI) {
              reassociate(RHS, LHS);
            }
          }
        }
        if (ElementSize == 1) {
          // DO NOTHING.
        } else if (ElementSize.isPowerOf2()) {
          APInt ShiftAmount = APInt(pointerMathSizeInBits, ElementSize.logBase2());
          NewIdx = Builder->CreateShl(NewIdx, ShiftAmount);
        } else {
          NewIdx = Builder->CreateMul(NewIdx, Builder->getInt(ElementSize));
        }

        Loop *loop = m_LI ? m_LI->getLoopFor(GEP->getParent()) : nullptr;

        if (loop && loop->isLoopInvariant(PtrOp)) {
          // add COffset to Pointer base first so LICM can kick in later
          // note that PointerValue is guaranteed to be LI since both PtrOp and whatever
          // we've added to it during reassociation must be LI
          if (COffset) {
            PointerValue = Builder->CreateAdd(PointerValue, COffset);
          }
          PointerValue = Builder->CreateAdd(PointerValue, NewIdx);
        } else {
          if (auto NewIdxVT = dyn_cast<IGCLLVM::FixedVectorType>(NewIdx->getType())) {
            Value *result =
                llvm::UndefValue::get(FixedVectorType::get(PtrMathTy, (unsigned)NewIdxVT->getNumElements()));
            for (uint32_t j = 0; j < (uint32_t)NewIdxVT->getNumElements(); j++) {
              result = Builder->CreateInsertElement(result, PointerValue, Builder->getInt32(j));
            }
            PointerValue = result;
          }
          PointerValue = Builder->CreateAdd(PointerValue, NewIdx);
          if (COffset) {
            PointerValue = Builder->CreateAdd(PointerValue, COffset);
          }
        }
      }
    }
  }

  if (reducePointerArith) {
    IGC_ASSERT_MESSAGE(GEP->isInBounds(), "we can only do a zext if the GEP is inbound");
    if (!canReduceNegativeOffset) {
      PointerValue = Builder->CreateZExt(PointerValue, BasePointer->getType());
    } else {
      PointerValue = Builder->CreateSExt(PointerValue, BasePointer->getType());
    }
    PointerValue = Builder->CreateAdd(BasePointer, PointerValue);
  }
  PointerValue = Builder->CreateIntToPtr(PointerValue, GEP->getType());
  GEP->replaceAllUsesWith(PointerValue);
  GEP->eraseFromParent();

  return true;
}

bool GenIRLowering::constantFoldFMaxFMin(CallInst *GII, BasicBlock::iterator &BBI) const {
  // Constant fold fmax/fmin only.
  EOPCODE GIID = GetOpCode(GII);
  if (GIID != llvm_max && GIID != llvm_min)
    return false;

  // Skip fmax/fmin with non-constant operand.
  ConstantFP *CFP0 = dyn_cast<ConstantFP>(GII->getOperand(0));
  ConstantFP *CFP1 = dyn_cast<ConstantFP>(GII->getOperand(1));
  if (!CFP0 || !CFP1)
    return false;

  // Fold fmax/fmin following OpenCL spec.
  const APFloat &A = CFP0->getValueAPF();
  const APFloat &B = CFP1->getValueAPF();
  APFloat Result = (GIID == llvm_min) ? minnum(A, B) : maxnum(A, B);
  Constant *C = ConstantFP::get(GII->getContext(), Result);

  GII->replaceAllUsesWith(C);
  GII->eraseFromParent();

  return true;
}

bool GenIRLowering::combineFMaxFMin(CallInst *GII, BasicBlock::iterator &BBI) const {
  using namespace llvm::PatternMatch; // Scoped namespace using.

  // Fold fmax/fmin with all constant operands.
  if (constantFoldFMaxFMin(GII, BBI))
    return true;

  ConstantFP *CMin, *CMax;
  Value *X = nullptr;

  if (!match(GII, m_ClampWithConstants(m_Value(X), CMin, CMax)))
    return false;

  // Optimize chained clamp, i.e. combine
  // (clamp (clamp x, MIN, MAX), MIN, MAX) into
  // (clamp x, MIN, MAX)
  ConstantFP *CMin2, *CMax2;
  Value *X2 = nullptr;
  if (match(X, m_ClampWithConstants(m_Value(X2), CMin2, CMax2)) && CMin == CMin2 && CMax == CMax2) {
    GII->replaceAllUsesWith(X);
    GII->eraseFromParent();

    return true;
  }

  // TODO: The following case should be combined as well
  //  (clamp (clamp x, MIN, MAX), MIN2, MAX2) into
  //  (clamp x, MIN3, MAX3), where
  // MIN3 = max(MIN, MIN2) and MAX3 = min(MAX, MAX2). The above case is just a
  // special case of this general form.

  if (!CMin->isZero() || !CMax->isExactlyValue(1.f))
    return false;

  // TODO: optimize chained fsat, i.e. combine
  // (fsat (fsat x)) into (fsat x)

  auto M = GII->getParent()->getParent()->getParent();
  GenISAIntrinsic::ID IID = GenISAIntrinsic::GenISA_fsat;
  Function *IFunc = GenISAIntrinsic::getDeclaration(M, IID, GII->getType());

  Instruction *I = Builder->CreateCall(IFunc, X);
  GII->replaceAllUsesWith(I);
  GII->eraseFromParent();

  BBI = llvm::BasicBlock::iterator(I);
  ++BBI;

  return true;
}

bool GenIRLowering::combineSelectInst(SelectInst *Sel, BasicBlock::iterator &BBI) const {
  using namespace llvm::PatternMatch; // Scoped namespace using.

  Value *LHS = nullptr;
  Value *RHS = nullptr;
  bool IsMax = false;
  unsigned Opcode = Instruction::UserOp1;

  if (Sel->getType()->isIntegerTy()) {
    IsMax = match(Sel, m_OrdFMaxCast(Opcode, m_Value(LHS), m_Value(RHS)));
    if (!IsMax && !match(Sel, m_OrdFMinCast(Opcode, m_Value(LHS), m_Value(RHS))))
      return false;

    switch (Opcode) {
    default:
      return false;
    case Instruction::FPToSI:
    case Instruction::FPToUI:
    case Instruction::BitCast:
      break;
    }
  } else {
    IsMax = match(Sel, m_OrdFMax(m_Value(LHS), m_Value(RHS)));
    if (!IsMax && !match(Sel, m_OrdFMin(m_Value(LHS), m_Value(RHS))))
      return false;
  }

  IGCLLVM::Intrinsic IID = IsMax ? Intrinsic::maxnum : Intrinsic::minnum;
  Function *IFunc = Intrinsic::getDeclaration(Sel->getParent()->getParent()->getParent(), IID, LHS->getType());

  Instruction *I = Builder->CreateCall2(IFunc, LHS, RHS);
  BBI = BasicBlock::iterator(I); // Don't move to the next one. We still need combine for saturation.
  if (Opcode != Instruction::UserOp1) {
    I = cast<Instruction>(Builder->CreateCast(static_cast<Instruction::CastOps>(Opcode), I, Sel->getType()));
  }
  Sel->replaceAllUsesWith(I);
  Sel->eraseFromParent();

  return false;
}

////////////////////////////////////////////////////////////////////////////////
// Detect complex patterns that pack 2 16-bit or 4 8-bit integers into a 32-bit
// value. Generate equivalent sequence of instructions that is later matched in
// the CodeGenPatternMatch::MatchPack4i8().
// Pattern example for <4 x i8> packing:
//   %x1 = and i32 %x, 127
//   %x2 = lshr i32 %x, 24
//   %x3 = and i32 %x2, 128
//   %x4 = or i32 %x3, %x1
//   %y1 = and i32 %y, 127
//   %y2 = lshr i32 %y, 24
//   %y3 = and i32 %y2, 128
//   %y4 = or i32 %y3, %y1
//   %y5 = shl nuw nsw i32 %y4, 8
//   %xy = or i32 %x4, %y5
//   %z1 = and i32 %z, 127
//   %z2 = lshr i32 %z, 24
//   %z3 = and i32 %z2, 128
//   %z4 = or i32 %z3, %z1
//   %z5 = shl nuw nsw i32 %z4, 16
//   %xyz = or i32 %xy, %z5
//   %w1 = shl nsw i32 %w, 24
//   %w2 = and i32 %w1, 2130706432
//   %w3 = and i32 %w, -2147483648
//   %w4 = or i32 %w2, %w3
//   %xyzw = or i32 %xyz, %w4
// and generate:
//   %0 = trunc i32 %x to i8
//   %1 = insertelement <4 x i8> poison, i8 %0, i32 0
//   %2 = trunc i32 %y to i8
//   %3 = insertelement <4 x i8> %1, i8 %2, i32 1
//   %4 = trunc i32 %z to i8
//   %5 = insertelement <4 x i8> %3, i8 %4, i32 2
//   %6 = trunc i32 %w to i8
//   %7 = insertelement <4 x i8> %5, i8 %6, i32 3
//   %8 = bitcast <4 x i8> %7 to i32
bool GenIRLowering::combinePack4i8Or2i16(Instruction *inst, uint64_t numBits) const {
  using namespace llvm::PatternMatch;

  const DataLayout &DL = inst->getModule()->getDataLayout();
  // Vector of 4 or 2 values that will be packed into a single 32-bit value.
  // The std::pair contains the 32-bit value that contains the element
  // to pack and the LSB where the packed value starts in the 32-bit value.
  SmallVector<std::pair<Value *, uint64_t>, 4> toPack;
  IGC_ASSERT(numBits == 8 || numBits == 16);
  uint64_t packedVecSize = 32 / numBits;
  toPack.resize(packedVecSize);
  uint64_t cSignMask = QWBIT(numBits - 1);
  uint64_t cMagnMask = BITMASK(numBits - 1);
  // The std::pair contains the 32-bit value that contains the element
  // to pack and the left shift bits that indicate the element position
  // in the packed vector.
  SmallVector<std::pair<Value *, uint64_t>, 4> args;
  args.push_back({isa<BitCastInst>(inst) ? inst->getOperand(0) : inst, 0});
  // In the first step traverse the chain of `or` and `shl` instructions
  // and find all elements of the packed vector.
  while (!args.empty()) {
    auto [v, prevShlBits] = args.pop_back_val();
    Value *lOp = nullptr;
    Value *rOp = nullptr;

    // Detect left shift by multiple of `numBits`. The `shl` operation sets the
    // `index` argument in the corresponding InsertElement instruction in the
    // final packing sequence. This operation can also be viewed as repacking
    // of already packed vector into another packed vector.
    uint64_t shlBits = 0;
    if (match(v, m_Shl(m_Value(lOp), m_ConstantInt(shlBits))) && (shlBits % numBits) == 0) {
      args.push_back({lOp, shlBits + prevShlBits});
      continue;
    }
    // Detect values that fit into `numBits` bits - a single element of
    // the packed vector.
    KnownBits kb = computeKnownBits(v, DL);
    uint32_t nonZeroBits = ~(static_cast<uint32_t>(kb.Zero.getZExtValue()));
    uint32_t lsb = IGCLLVM::findFirstSet(nonZeroBits);
    uint32_t msb = IGCLLVM::findLastSet(nonZeroBits);
    if (msb != lsb && (msb / numBits) == (lsb / numBits)) {
      uint32_t idx = (prevShlBits / numBits) + (lsb / numBits);
      if (idx < packedVecSize && toPack[idx].first == nullptr) {
        toPack[idx] = std::make_pair(v, alignDown(lsb, numBits));
        continue;
      }
    }
    // Detect packing of two disjoint values. This `or` operation corresponds
    // to an InsertElement instruction in the final packing sequence.
    if (match(v, m_Or(m_Value(lOp), m_Value(rOp)))) {
      KnownBits kbL = computeKnownBits(lOp, DL);
      KnownBits kbR = computeKnownBits(rOp, DL);
      uint32_t nonZeroBitsL = ~(static_cast<uint32_t>(kbL.Zero.getZExtValue()));
      uint32_t nonZeroBitsR = ~(static_cast<uint32_t>(kbR.Zero.getZExtValue()));
      if ((nonZeroBitsL & nonZeroBitsR) == 0) {
        args.push_back({lOp, prevShlBits});
        args.push_back({rOp, prevShlBits});
      }
      continue;
    }
    if (std::all_of(toPack.begin(), toPack.end(), [](const auto &c) { return c.first != nullptr; })) {
      break;
    }
    // Unsupported pattern.
    return false;
  }
  if (std::any_of(toPack.begin(), toPack.end(), [](const auto &c) { return c.first == nullptr; })) {
    return false;
  }
  // In the second step match the pattern that packs sign and magnitude parts
  // and simple masking with `and` instruction.
  for (uint32_t i = 0; i < packedVecSize; ++i) {
    auto [v, lsb] = toPack[i];
    Value *lOp = nullptr;
    Value *rOp = nullptr;
    uint64_t lMask = 0;
    uint64_t rMask = 0;
    // Match patterns that pack the sign and magnitude parts.
    if (match(v, m_Or(m_And(m_Value(lOp), m_ConstantInt(lMask)), m_And(m_Value(rOp), m_ConstantInt(rMask)))) &&
        (IGCLLVM::popcount(rMask) == 1 || IGCLLVM::popcount(lMask) == 1)) {
      Value *signOp = IGCLLVM::popcount(rMask) == 1 ? rOp : lOp;
      Value *magnOp = IGCLLVM::popcount(rMask) == 1 ? lOp : rOp;
      uint64_t signMask = IGCLLVM::popcount(rMask) == 1 ? rMask : lMask;
      uint64_t magnMask = IGCLLVM::popcount(rMask) == 1 ? lMask : rMask;
      uint64_t shlBits = 0;
      uint64_t shrBits = 0;
      // %b = shl nsw i32 %a, 24
      // %c = and i32 %b, 2130706432
      // %sign = and i32 %a, -2147483648
      // %e = or i32 %sign, %c
      if (match(magnOp, m_Shl(m_Value(v), m_ConstantInt(shlBits))) && v == signOp && (shlBits % numBits) == 0 &&
          shlBits == (i * numBits) && (cSignMask << shlBits) == signMask && (cMagnMask << shlBits) == magnMask &&
          lsb == shlBits) {
        toPack[i] = std::make_pair(v, 0);
        continue;
      }
      // %b = and i32 %a, 127
      // %c = lshr i32 %a, 24
      // %sign = and i32 %c, 128
      // %e = or i32 %sign, %b
      if (match(signOp, m_LShr(m_Value(v), m_ConstantInt(shrBits))) && v == magnOp && shrBits == (32 - numBits) &&
          cSignMask == signMask && cMagnMask == magnMask && lsb == 0) {
        toPack[i] = std::make_pair(v, 0);
        continue;
      }
    }
    uint64_t andMask = 0;
    if (match(v, m_And(m_Value(lOp), m_ConstantInt(andMask))) && andMask == BITMASK(numBits) && lsb == 0) {
      toPack[i] = std::make_pair(lOp, 0);
      continue;
    }
    if (lsb > 0) {
      return false;
    }
  }

  // Create the packing sequence that is matched in the PatternMatch later.
  Type *elemTy = Builder->getIntNTy(numBits);
  Value *packed = PoisonValue::get(IGCLLVM::FixedVectorType::get(elemTy, packedVecSize));
  for (uint32_t i = 0; i < packedVecSize; ++i) {
    auto [elem, lsb] = toPack[i];
    IGC_ASSERT(lsb == 0);
    elem = Builder->CreateTrunc(elem, elemTy);
    packed = Builder->CreateInsertElement(packed, elem, Builder->getInt32(i));
  }
  packed = Builder->CreateBitCast(packed, inst->getType());
  inst->replaceAllUsesWith(packed);
  inst->eraseFromParent();
  return true;
}

FunctionPass *IGC::createGenIRLowerPass() { return new GenIRLowering(); }

// Register pass to igc-opt
#define PASS_FLAG "igc-gen-ir-lowering"
#define PASS_DESCRIPTION "Lowers GEP into primitive ones"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GenIRLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(GenIRLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

FunctionPass *IGC::createGEPLoweringPass() { return new GEPLowering(); }

// Register pass to igc-opt
#define PASS_FLAG2 "igc-gep-lowering"
#define PASS_DESCRIPTION2 "Lowers GEP into primitive ones"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(GEPLowering, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass);
IGC_INITIALIZE_PASS_END(GEPLowering, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
