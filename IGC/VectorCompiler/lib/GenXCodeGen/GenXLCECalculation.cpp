/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXLCECalculation
/// -------------------
///
/// GenXLCECalculation is a function pass that analyzes loop bounds and tries
/// to calculate loop count expression in a form of 'Factor * Symbol + Addend'
/// where symbol is an info about a kernel argument. E.g. for the loop in the
/// following kernel:
///    void foo(int N, ...) {
///      for (int i = 0; i < (N / 2 + 1); ++i) {...}
///    }
/// LoopCountExpr = 0.5 * Symbol(N) + 1, where
/// Symbol(N) = {0, 0, 4, false}: direct symbol with Num=0, Offset=0, Size=4
//
//===----------------------------------------------------------------------===//

#include "GenX.h"

#include "vc/Utils/GenX/CostInfo.h"
#include "vc/Utils/GenX/KernelInfo.h"

#include "llvmWrapper/IR/IRBuilder.h"

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>

using namespace llvm;
using namespace genx;

namespace {

class LoopCountExprWrapper {
  friend class LCEFinder;
  vc::LoopCountExpr Expr;

public:
  LoopCountExprWrapper() {}
  LoopCountExprWrapper(float F, vc::ArgSym S, float A) {
    Expr.Symbol = S;
    Expr.Factor = F;
    Expr.Addend = A;
    Expr.IsUndef = false;
  }

  bool isUndef() const { return Expr.IsUndef; }
  bool save(const llvm::Loop &L, llvm::Module &M) const {
    return vc::saveLCEToMetadata(L, M, Expr);
  }

  LoopCountExprWrapper operator-(LoopCountExprWrapper const &RHS) const {
    // Propagate undef.
    if (Expr.IsUndef || RHS.Expr.IsUndef)
      return LoopCountExprWrapper{};
    // If both expressions are not constant they should share
    // the same symbol.
    if (Expr.Factor != 0.0 && RHS.Expr.Factor != 0.0 &&
        Expr.Symbol != RHS.Expr.Symbol)
      return LoopCountExprWrapper{};
    return LoopCountExprWrapper(Expr.Factor - RHS.Expr.Factor,
                                Expr.Factor != 0.0 ? Expr.Symbol
                                                   : RHS.Expr.Symbol,
                                Expr.Addend - RHS.Expr.Addend);
  }
  LoopCountExprWrapper operator/(unsigned Val) const {
    auto Res =
        LoopCountExprWrapper(Expr.Factor / Val, Expr.Symbol, Expr.Addend / Val);
    Res.Expr.IsUndef = Expr.IsUndef;
    return Res;
  }
};

// This class traverses IR to find LCE for the loop bound.
class LCEFinder : public InstVisitor<LCEFinder, Value *> {
  LoopCountExprWrapper LCE;
  const DataLayout *DL;

public:
  LCEFinder(const DataLayout *DataL) : DL(DataL), LCE(1.0, {}, 0.0) {}

  LoopCountExprWrapper getLCE(Value &Start);

  Value *visitInstruction(Instruction &I);
  Value *visitBinaryOperator(BinaryOperator &BO);
  Value *visitCastInst(CastInst &CI);
  Value *visitGetElementPtrInst(GetElementPtrInst &GEP);
  Value *visitLoadInst(LoadInst &LI);
};

class GenXLCECalculation : public FunctionPass {
  using LoopDirection = Loop::LoopBounds::Direction;
  struct LCELoopInfo {
    // The initial value of induction variable.
    LoopCountExprWrapper Init;
    // The final value of induction variable.
    LoopCountExprWrapper Final;
    LoopCountExprWrapper TripCount;
    LoopDirection Direction = LoopDirection::Unknown;
    unsigned AbsStepValue = 0;
  };
  DenseMap<Loop *, LCELoopInfo> LoopMap;

public:
  static char ID;
  explicit GenXLCECalculation() : FunctionPass(ID) {}
  StringRef getPassName() const override {
    return "GenX loop count expression calculation";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.addPreserved<LoopInfoWrapperPass>();
    AU.addPreserved<ScalarEvolutionWrapperPass>();
  }
  bool runOnFunction(Function &F) override;

private:
  LCELoopInfo processLoop(const Loop &L, const Function &F, ScalarEvolution &SE,
                          LoopInfo &LI) const;
};

} // end namespace

char GenXLCECalculation::ID = 0;
namespace llvm {
void initializeGenXLCECalculationPass(PassRegistry &);
} // end namespace llvm

INITIALIZE_PASS_BEGIN(GenXLCECalculation, "GenXLCECalculation",
                      "GenXLCECalculation", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
INITIALIZE_PASS_END(GenXLCECalculation, "GenXLCECalculation",
                    "GenXLCECalculation", false, false)

FunctionPass *llvm::createGenXLCECalculationPass() {
  initializeGenXLCECalculationPass(*PassRegistry::getPassRegistry());
  return new GenXLCECalculation;
}

// Stop traversal when instruction is unknown.
Value *LCEFinder::visitInstruction(Instruction &I) { return nullptr; }

Value *LCEFinder::visitBinaryOperator(BinaryOperator &BO) {
  // Operand of the instruction which is a constant factor/addend.
  Value *C = nullptr;
  // Operand of the instruction which will be traversed further.
  Value *V = nullptr;
  // True when constant is the first operand and value is the second.
  bool IsReversed = false;

  if (isa<ConstantData>(BO.getOperand(0))) {
    IsReversed = true;
    C = BO.getOperand(0);
    V = BO.getOperand(1);
  } else if (isa<ConstantData>(BO.getOperand(1))) {
    C = BO.getOperand(1);
    V = BO.getOperand(0);
  } else
    return nullptr;

  auto Opcode = BO.getOpcode();
  if (IsReversed && !BO.isCommutative() && Opcode != Instruction::Sub &&
      Opcode != Instruction::FSub)
    return nullptr;

  float ConstAsFP = isa<ConstantFP>(C)
                         ? cast<ConstantFP>(C)->getValue().convertToFloat()
                         : cast<ConstantInt>(C)->getSExtValue();
  switch (Opcode) {
  default:
    return nullptr;
  case Instruction::Add:
  case Instruction::FAdd:
    LCE.Expr.Addend += ConstAsFP * LCE.Expr.Factor;
    break;
  case Instruction::Sub:
  case Instruction::FSub: {
    ConstAsFP *= LCE.Expr.Factor;
    if (!IsReversed)
      // sub V, C -> add V, -C
      LCE.Expr.Addend += -ConstAsFP;
    else {
      // sub C, V -> add -V, C
      LCE.Expr.Factor = -LCE.Expr.Factor;
      LCE.Expr.Addend += ConstAsFP;
    }
    break;
  }
  case Instruction::Mul:
  case Instruction::FMul:
    LCE.Expr.Factor *= ConstAsFP;
    break;
  case Instruction::UDiv:
  case Instruction::SDiv:
  case Instruction::FDiv:
    LCE.Expr.Factor /= ConstAsFP;
    break;
  case Instruction::Shl:
    LCE.Expr.Factor *= 1 << (unsigned)ConstAsFP;
    break;
  case Instruction::LShr:
    LCE.Expr.Factor /= 1 << (unsigned)ConstAsFP;
    break;
  }
  return V;
}

Value *LCEFinder::visitCastInst(CastInst &CI) { return CI.getOperand(0); }

Value *LCEFinder::visitGetElementPtrInst(GetElementPtrInst &GEP) {
  if (!LCE.Expr.Symbol.IsIndirect)
    return nullptr;
  auto GTI = gep_type_begin(GEP);
  for (auto OI = GEP.op_begin() + 1, E = GEP.op_end(); OI != E; ++OI, ++GTI) {
    auto *Idx = dyn_cast<ConstantInt>(*OI);
    if (!Idx)
      return nullptr;
    // TODO: Should struct types be handled?
    if (GTI.getStructTypeOrNull())
      return nullptr;
    LCE.Expr.Symbol.Offset +=
        DL->getTypeAllocSize(GTI.getIndexedType()) * Idx->getSExtValue();
  }
  return GEP.getPointerOperand();
}

Value *LCEFinder::visitLoadInst(LoadInst &LI) {
  if (LCE.Expr.Symbol.IsIndirect)
    return nullptr;
  LCE.Expr.Symbol.IsIndirect = true;
  return LI.getOperand(0);
}

LoopCountExprWrapper LCEFinder::getLCE(Value &Start) {
  if (auto *CI = dyn_cast<ConstantInt>(&Start)) {
    LCE.Expr.Factor = 0.0f;
    LCE.Expr.Addend = CI->getSExtValue();
    return LCE;
  }

  auto *Prev = &Start;
  Instruction *NextInst = nullptr;
  while (NextInst = dyn_cast_or_null<Instruction>(Prev))
    Prev = visit(NextInst);

  auto *Arg = dyn_cast_or_null<Argument>(Prev);
  if (!Arg)
    return LoopCountExprWrapper{};
  auto *ArgTy = Arg->getType();
  // Implicit arguments must come at the end.
  // So we don't take them into an account.
  LCE.Expr.Symbol.Num = Arg->getArgNo();
  LCE.Expr.Symbol.Size = ArgTy->isPointerTy()
                             ? DL->getPointerTypeSize(ArgTy)
                             : DL->getTypeSizeInBits(ArgTy) / 8;
  return LCE;
}

GenXLCECalculation::LCELoopInfo
GenXLCECalculation::processLoop(const Loop &L, const Function &F,
                                ScalarEvolution &SE, LoopInfo &LI) const {
  auto LBOptional = L.getBounds(SE);
  if (!LBOptional)
    return LCELoopInfo{};

  auto LB = *LBOptional;
  auto *StepValueCI = dyn_cast_or_null<ConstantInt>(LB.getStepValue());
  // We analyze only loops with a constant StepValue.
  if (!StepValueCI)
    return LCELoopInfo{};

  auto *DL = &F.getParent()->getDataLayout();
  LCELoopInfo Result;
  Result.AbsStepValue = std::abs(StepValueCI->getSExtValue());
  Result.Direction = LB.getDirection();

  auto &IVFinalV = LB.getFinalIVValue();
  Result.Final = LCEFinder{DL}.getLCE(IVFinalV);
  if (!Result.Final.isUndef()) {
    auto &IVInitCI = LB.getInitialIVValue();
    Result.Init = LCEFinder{DL}.getLCE(IVInitCI);
  }
  return Result;
}

bool GenXLCECalculation::runOnFunction(Function &F) {
  if (!vc::isKernel(&F))
    return false;

  bool Changed = false;
  auto &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
  auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

  SmallVector<Loop *, 4> Loops = LI.getLoopsInPreorder();
  for (auto *L : Loops) {
    auto LCEInfo = processLoop(*L, F, SE, LI);
    auto Res = LCEInfo.Direction == LoopDirection::Increasing
                   ? (LCEInfo.Final - LCEInfo.Init) / LCEInfo.AbsStepValue
                   : (LCEInfo.Init - LCEInfo.Final) / LCEInfo.AbsStepValue;
    Changed |= Res.save(*L, *F.getParent());
  }
  return Changed;
}
