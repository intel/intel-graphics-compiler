/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/GEPLoopStrengthReduction/GEPLoopStrengthReduction.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "llvm/Support/Debug.h"
#include <llvm/Transforms/Utils/Local.h>
#include "llvmWrapper/Transforms/Utils/LoopUtils.h"
#include "llvmWrapper/Transforms/Utils/ScalarEvolutionExpander.h"
#include "llvmWrapper/IR/Instructions.h"
#include "common/LLVMWarningsPop.hpp"

#include "Probe/Assertion.h"
#include "common/igc_regkeys.hpp"
#include "Compiler/CISACodeGen/IGCLivenessAnalysis.h"
#include "Compiler/CodeGenPublic.h"

using namespace llvm;
using namespace IGC;

#define DEBUG_TYPE "gep-loop-strength-reduction"

// This pass reduces strength of address calculations inside loops. Xe architecture doesn't have native
// support for access from "base pointer plus offset". When access "pointer[offset]" is compiled into GEP
// instruction "getelementptr %constantPointer, %variableOffset", in codegen compiler must translate it
// into "(long long)pointer + offset*sizeof(*pointer)", generating additional mov/add/shl instructions.
// If access happens in the loop, it might be beneficial to replace it into "getelementptr %variablePointer,
// %constantOffset". This generates less instruction for address calculation in codegen.
//
//
// This pass does three kinds of optimizations:
//
// 1. If GEP's index is constant between loop iterations, move GEP to loop preheader. This is very similar
//    to LICM, with one difference: if loop contains multiple GEPs with the same pointer and different
//    offsets, LICM moves all GEPs to preheader, but this pass moves only base GEP and keeps remaining
//    address calculations inside loops, but replace them with constant offsets. This gives less register
//    pressure than LICM (only one address in preheader) and still simplifies address calculation in
//    loop body.
//
// 2. If GEP's index in incremented in constant steps in the loop, move GEP for base address calculation
//    to loop preheader and add it as new induction variable to the loop. This pass translates code:
//
//      int id = get_global_id(0) * n_iters;
//      for (int i = 0; i < n_iters; ++i) {
//        buf[id + i] = buf[id + i] + 3.0f;
//      }
//
//    Into:
//
//      int id = get_global_id(0) * n_iters;
//      global float *p = buf + id;
//      for (int i = 0; i < n_iters; ++i, ++p) {
//        *p = *p + 3.0f;
//      }
//
//    GEP instruction with multiple indices are also supported if last index is the only variable. In this
//    case new induction variable is direct pointer to indexed type. Before reduction:
//
//      for (int i = 0; i < n_iters; ++i) {
//        s->buf[id + i] = 3.0f; // s = struct { int float[] }
//      }
//
//    After:
//
//      global float *p = s->buf + id;
//      for (int i = 0; i < n_iters; ++i, ++p) {
//        *p = 3.0f;
//      }
//
// 3. If GEP's index in incremented in constant steps in the loop, but base address calculation can't be
//    moved to loop preheader, simplify index calculation for other GEPs using the same index plus constant
//    offset. This will translate code:
//
//      int id = get_global_id(0) * n_iters;
//      for (int i = 0; i < n_iters; i+=2) {
//        buf[id + i + 1] = buf[id + i] + 3.0f;
//      }
//
//    Into:
//
//      int id = get_global_id(0) * n_iters;
//      for (int i = 0; i < n_iters; i+=2) {
//        global float *p = buf + id + i;
//        *(p + 1) = *p + 3.0f;
//      }

enum ReductionType {
  // Moves GEP for base address calculation to loop's preheader. If address changes inside loop
  // with constant step, adds pointer as new induction variable to the loop. All GEPs using the
  // same base address + constant offset are modified to use new pointer.
  REDUCE_TO_PREHEADER,

  // Does not move GEP to loop's preheader and does not add new induction variable to the
  // loop. Base address for first GEP is recalculated on each iteration, but all related
  // GEPs are modified to use the same pointer + constant offset, reducing address calculation
  // inside loop. Doesn't increase register pressure.
  REDUCE_INDEX_ONLY,
};

// Score used by heuristic for deciding what type of reduction to apply.
struct Score {
  Score() : ReducesInstructions(false), RegisterPressure(0), ContainsMuli64(false) {}

  // True if reduction to preheader would lower number of instructions in
  // loop. False otherwise.
  bool ReducesInstructions;

  // Estimated increase in register pressure when reducing to loop's preheader.
  unsigned RegisterPressure;

  // Flag to show presence of i64 mul instructions in the address calculation.
  bool ContainsMuli64;
};

// Single GEP instruction that is a candiate for reduction. It is expressed by SCEV:
//     { start, +, step }
// Where step is constant integer (including 0), and start can be calculated in loop's preheader.
struct ReductionCandidate {
  ReductionCandidate(GetElementPtrInst *GEP, const SCEV *S, int64_t Delta) : GEP(GEP), S(S), Delta(Delta) {}

  bool operator==(const ReductionCandidate &RHS) const { return GEP == RHS.GEP && S == RHS.S && Delta == RHS.Delta; }

  bool isValidForReduction(const Loop *L, const DominatorTree *DT);

  bool isBetterForReduction(const ReductionCandidate &Other);

  void print(raw_ostream &OS);

  // Instruction to reduce
  GetElementPtrInst *GEP;

  // SCEV expressing starting index
  const SCEV *S;

  // Index delta between group's base pointer and this pointer
  int64_t Delta;
};

// Sometimes reduction candidates might be grouped. This is possible if both candidates:
// 1) delta between start values can be expressed in constant int
// 2) have the same step increment value (including 0)
//
// Example: This loop has three uses of "output" buffer with three different offsets.
//
//    int id = get_global_id(0);
//    for (int i = 32; i < n_iters - 32; i += 32) {
//        output[id + i] = output[id + i + 32] * output[id + i - 32];
//    }
//
// Each access could be reduced independently, but since all three offsets are increased in
// the same constant step, it is more beneficial to reduce them to one common pointer:
//
//    int id = get_global_id(0);
//    global float *outputm32 = output + id;
//    for (int i = 32; i < n_iters - 32; i += 32) {
//        *(outputm32 + 32) = *(outputm32 + 64) * *outputm32;
//        outputm32 += 32;
//    }
class ReductionCandidateGroup {
  friend class Scorer;

public:
  ReductionCandidateGroup(Loop *L, const DominatorTree *DT, GetElementPtrInst *GEP, const SCEV *S, const SCEV *Step)
      : L(L), DT(DT), Step(Step), Base(GEP, S, 0), RT(REDUCE_TO_PREHEADER) {}

  bool addToGroup(ScalarEvolution &SE, GetElementPtrInst *GEP, const SCEV *S, const SCEV *Step);

  void transform(IGCLLVM::IRBuilder<> &IRB, SCEVExpander &E);

  bool isValid();

  // Return how many GEP instructions will be reduced.
  uint32_t getGEPCount() { return 1 + Others.size(); }

  // Returns reduced GEPs. Filled after successful reduction.
  const SmallVectorImpl<GetElementPtrInst *> &getReduced() { return Reduced; }

  ReductionType getReductionType() { return RT; }
  void setReductionType(ReductionType RT) { this->RT = RT; }

  Loop *getLoop() { return L; }

  Score getScore() { return Score; }

  void print(raw_ostream &OS);

private:
  void append(GetElementPtrInst *GEP, const SCEV *S, int64_t Delta);
  void append(ReductionCandidate &C) { append(C.GEP, C.S, C.Delta); }

  ReductionCandidate &getCheapestCandidate();

  void swapBase(ReductionCandidate &C);

  void reduceToPreheader(IGCLLVM::IRBuilder<> &IRB, SCEVExpander &E);
  void reduceIndexOnly(IGCLLVM::IRBuilder<> &IRB, SCEVExpander &E);

  Value *getStepValue(IGCLLVM::IRBuilder<> &IRB, SCEVExpander &E, BasicBlock *BB);

  // Base GEP to reduce
  ReductionCandidate Base;

  // Other GEPs that can be reduced together with base
  SmallVector<ReductionCandidate, 4> Others;

  // Increment step value
  const SCEV *Step;

  // Group member with the smallest SCEV expression. Preferred for reduction
  // to preheader, as it should give smallest increase in register pressure.
  std::optional<ReductionCandidate> Cheapest;

  // All GEPs that were reduced and are safe to delete.
  SmallVector<GetElementPtrInst *, 8> Reduced;

  Loop *L;

  const DominatorTree *DT;

  Score Score;

  ReductionType RT;
};

// Scores reduction candidates:
//   1. Estimates removed instructions by reducing "base_ptr + index * sizeof(type)".
//   2. Estimates increase in register pressure.
class Scorer {
public:
  Scorer(const DataLayout &DL, ModuleMetaData &MMD, IGCLivenessAnalysis &RPE, WIAnalysisRunner &WI)
      : DL(DL), MMD(MMD), RPE(RPE), WI(WI) {}

  void score(SmallVectorImpl<ReductionCandidateGroup> &Candidates);

private:
  void score(ReductionCandidateGroup &Candidate);

  void scoreReducedInstructions(ReductionCandidateGroup &Candidate);
  void scoreRegisterPressure(ReductionCandidateGroup &Candidate);

  int estimateIndexInstructions(const Loop &L, GetElementPtrInst *GEP, bool &ContainsMuli64);
  int estimatePointerAddition(ReductionCandidateGroup &Candidate);

  const DataLayout &DL;
  IGCLivenessAnalysis &RPE;
  ModuleMetaData &MMD;
  WIAnalysisRunner &WI;
};

// Analyzes GEP instructions in a single loop and selects candidates for reduction.
class Analyzer {
public:
  Analyzer(const DataLayout &DL, DominatorTree &DT, Loop &L, LoopInfo &LI, ScalarEvolution &SE, SCEVExpander &E)
      : DT(DT), L(L), LI(LI), SE(SE), E(E) {}

  void analyze(SmallVectorImpl<ReductionCandidateGroup> &Result);

private:
  // Represents deconstructed SCEV expression { start, +, step }.
  // Start SCEV will be used to calculate base pointer, and Step SCEV
  // will increase new induction variable on each iteration.
  struct DeconstructedSCEV {
    DeconstructedSCEV() : Start(nullptr), Step(nullptr), ConvertedMulExpr(false) {}

    bool isValid();

    const SCEV *Start;
    const SCEV *Step;

    // True if input SCEV:
    //   x * { start, +, step }
    // Was converted into:
    //   { x * start, +, x * step }
    bool ConvertedMulExpr;
  };

  void analyzeGEP(GetElementPtrInst *GEP);
  bool doInitialValidation(GetElementPtrInst *GEP);

  bool deconstructSCEV(const SCEV *S, DeconstructedSCEV &Result);

  DominatorTree &DT;
  Loop &L;
  LoopInfo &LI;
  ScalarEvolution &SE;
  SCEVExpander &E;

  SmallVector<ReductionCandidateGroup, 16> Candidates;
};

// Tracks estimated register pressure.
class RegisterPressureTracker {
public:
  RegisterPressureTracker(Function &F, CodeGenContext &CGC, IGCLivenessAnalysis &RPE,
                          IGCFunctionExternalRegPressureAnalysis &FRPE, WIAnalysisRunner &WI);

  bool fitsPressureThreshold(ReductionCandidateGroup &C);
  void updatePressure(ReductionCandidateGroup &C, SCEVExpander &E);

  void trackDeletedInstruction(Value *V);

private:
  unsigned MaxAllowedPressure;
  unsigned FunctionExternalPressure;

  IGCLivenessAnalysis &RPE;
  WIAnalysisRunner &WI;

  // Basic Blocks impacted by reduction, requiring register pressure reestimation.
  BBSet BBsToUpdate;

  // Keep track what new instructions inserted by SCEV Expander were already added to estimation.
  SmallPtrSet<Instruction *, 32> VisitedNewInsts;
};

// Does reduction on collected candidates.
class Reducer {
public:
  Reducer(IGCLLVM::IRBuilder<> &IRB, RegisterPressureTracker &RPT, SCEVExpander &E, bool AllowLICM)
      : IRB(IRB), RPT(RPT), E(E), AllowLICM(AllowLICM) {}

  bool reduce(SmallVectorImpl<ReductionCandidateGroup> &Candidates);

private:
  void cleanup(ReductionCandidateGroup &C);

  IGCLLVM::IRBuilder<> &IRB;
  RegisterPressureTracker &RPT;
  SCEVExpander &E;

  bool AllowLICM;
};

// Set of functions/classes helping manipulating SCEV objects.
namespace SCEVHelper {
const SCEV *dropExt(const SCEV *S);

bool isValid(const SCEV *S);
bool isEqual(const SCEV *A, const SCEV *B);

// ScalarEvolution::getAddExpr requires all operands to have the same
// type. Extend type if required.

// Builds SCEVAddExpr instance. Function ScalarEvolution::getAddExpr requires all
// operands to have the same type. This class wraps ScalarEvolution::getAddExpr,
// but extends operands if it is needed to keep them all in one type.
class SCEVAddBuilder {
public:
  SCEVAddBuilder(ScalarEvolution &SE, bool DropExt = false) : SE(SE), DropExt(DropExt) {}

  SCEVAddBuilder &add(const SCEV *S, bool Negative = false);

  SCEVAddBuilder &addNegative(const SCEV *S) { return add(S, true); }

  const SCEV *build();

private:
  struct Op {
    Op(const SCEV *S, bool Negative) : S(S), Negative(Negative) {}

    const SCEV *S;
    bool Negative;
  };

  ScalarEvolution &SE;
  SmallVector<Op, 16> Ops;
  bool DropExt;
};

// Builds SCEVMulExpr instance. Function ScalarEvolution::getMulExpr requires all
// operands to have the same type. This class wraps ScalarEvolution::getMulExpr,
// but extends operands if it is needed to keep them all in one type.
class SCEVMulBuilder {
public:
  SCEVMulBuilder(ScalarEvolution &SE) : SE(SE) {}

  SCEVMulBuilder &add(const SCEV *S);

  const SCEV *build();

private:
  ScalarEvolution &SE;
  SmallVector<const SCEV *, 4> Ops;
};
}; // namespace SCEVHelper

class GEPLoopStrengthReduction : public llvm::FunctionPass {
public:
  static char ID; // Pass identification, replacement for typeid

  GEPLoopStrengthReduction(bool AllowLICM = true);

  ~GEPLoopStrengthReduction() {}

  virtual llvm::StringRef getPassName() const override { return "GEPLoopStrengthReduction"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  virtual bool runOnFunction(llvm::Function &F) override;

private:
  bool AllowLICM;
};

void ReductionCandidate::print(raw_ostream &OS) {
  OS << "{gep=";
  GEP->printAsOperand(OS, false);
  OS << ", ptr=";
  GEP->getPointerOperand()->printAsOperand(OS, false);
  OS << ", scev=";
  S->print(OS);
  OS << ", delta=";
  OS << Delta;
  OS << "}";
}

void ReductionCandidateGroup::print(raw_ostream &OS) {
  OS << "{loop=";
  OS << L->getName();
  OS << ", geps=" << Others.size() + 1;
  OS << ", base=";
  Base.print(OS);
  OS << ", others=[";
  for (auto it = Others.begin(); it != Others.end();) {
    it->print(OS);
    ++it;
    if (it != Others.end())
      OS << ", ";
  }
  OS << "], step=";
  Step->print(OS);
  OS << "}";
}

// Considering loop:
//
//   for (i ...)
//   {
//     if (cond)
//     {
//       x[i - 1] = ...
//     }
//     x[i] = ...
//   }
//
// If address is calculated in if-case, if there is no guarantee it would be calculated on the
// first iteration, it can't be reduced to preheader, as value could be invalid.
bool ReductionCandidate::isValidForReduction(const Loop *L, const DominatorTree *DT) {
  if (!DT->dominates(GEP->getParent(), L->getLoopLatch()))
    return false;

  // Even if address is always calculated, it might not always be used.
  // Find at least one always-executed use to proof address is safe.
  for (auto U = GEP->users().begin(); U != GEP->users().end(); ++U) {
    if (auto *I = dyn_cast<Instruction>(*U)) {
      if (L->contains(I) && DT->dominates(I->getParent(), L->getLoopLatch()))
        return true;
    }
  }

  return false;
}

bool ReductionCandidate::isBetterForReduction(const ReductionCandidate &Other) {
  // If lowest delta is selected as base, remaining GEPs in the group will use
  // positive indices only. This should make other load/store optimizations easier.
  return (Delta < Other.Delta) || (Delta == Other.Delta && S->getExpressionSize() < Other.S->getExpressionSize());
}

// If possible, adds another reduction candidate to this group. Reductions
// can be grouped together if:
//   1) delta between start values can be expressed in constant int
//   2) have the same increment step value
// New candidate can take position of group's base if it uses less instructions
// to calculate.
// Returns true if candidate was added to group.
bool ReductionCandidateGroup::addToGroup(ScalarEvolution &SE, GetElementPtrInst *GEP, const SCEV *S, const SCEV *Step) {
  if (!SCEVHelper::isEqual(this->Step, Step))
    return false;

  if (Base.GEP->getPointerOperand() != GEP->getPointerOperand())
    return false;

  if (Base.GEP->getType() != GEP->getType())
    return false;

  // Compare indices (except last one)
  if (Base.GEP->getNumIndices() != GEP->getNumIndices())
    return false;

  if (!std::equal(Base.GEP->indices().begin(), Base.GEP->indices().end() - 1, GEP->indices().begin()))
    return false;

  // Can't use ScalarEvolution::computeConstantDifference, as it only
  // supports SCEVAddExpr with two operands. Calculate difference as:
  //     new candidate's operands + (-1 * base's operands)
  SCEVHelper::SCEVAddBuilder Builder(SE, true);
  const SCEVConstant *Sum = dyn_cast<SCEVConstant>(Builder.add(S).addNegative(Base.S).build());
  if (!Sum)
    return false;

  int64_t Delta = Sum->getValue()->getSExtValue();

  // Delta is constant int, reductions can be grouped together.
  append(GEP, S, Delta);

  return true;
}

void ReductionCandidateGroup::append(GetElementPtrInst *GEP, const SCEV *S, int64_t Delta) {
  Others.emplace_back(GEP, S, Delta);

  // Group changed, forget which member is the cheapest.
  Cheapest.reset();
}

// Sets group's base GEP to provided candidate.
void ReductionCandidateGroup::swapBase(ReductionCandidate &C) {
  if (Base == C)
    return;

  auto *ToReplace = std::find(Others.begin(), Others.end(), C);
  IGC_ASSERT(ToReplace != Others.end());

  ReductionCandidate NewBase = *ToReplace;

  // Directly replace old slot with old base.
  *ToReplace = Base;

  // Update candidates with new delta.
  for (auto &O : Others)
    O.Delta -= NewBase.Delta;

  Base = NewBase;
  Base.Delta = 0;
}

ReductionCandidate &ReductionCandidateGroup::getCheapestCandidate() {
  if (Cheapest)
    return Cheapest.value();

  ReductionCandidate *BestCandidate = nullptr;

  for (auto It = Others.begin(); It != Others.end(); ++It) {
    if (!It->isValidForReduction(L, DT))
      continue;

    if (!BestCandidate || It->isBetterForReduction(*BestCandidate))
      BestCandidate = It;
  }

  if (BestCandidate && (!Base.isValidForReduction(L, DT) || BestCandidate->isBetterForReduction(Base)))
    return Cheapest.emplace(*BestCandidate);

  return Cheapest.emplace(Base);
}

bool ReductionCandidateGroup::isValid() {
  if (Base.isValidForReduction(L, DT))
    return true;

  for (auto It = Others.begin(); It != Others.end(); ++It) {
    if (It->isValidForReduction(L, DT))
      return true;
  }

  return false;
}

void ReductionCandidateGroup::transform(IGCLLVM::IRBuilder<> &IRB, SCEVExpander &E) {
  switch (RT) {
  case REDUCE_TO_PREHEADER:
    return reduceToPreheader(IRB, E);
  case REDUCE_INDEX_ONLY:
  default:
    return reduceIndexOnly(IRB, E);
  }
}

// Transforms all grouped GEP instructions:
//   1) Calculate start value in loop's preheader
//   2) Add this new pointer as loop's induction variable with constant step
//   3) Replace GEP instructions to use this pointer + constant offset
void ReductionCandidateGroup::reduceToPreheader(IGCLLVM::IRBuilder<> &IRB, SCEVExpander &E) {
  // Updates group's base to candidate with smallest SCEV expression.
  swapBase(getCheapestCandidate());

  // Create pointer's starting value in preheader
  BasicBlock *LPH = L->getLoopPreheader();
  const SCEV *Start = Base.S;
  Value *StartIndex = E.expandCodeFor(Start, Start->getType(), &LPH->back());

  IRB.SetInsertPoint(&LPH->back());
  SmallVector<Value *, 4> Indices(Base.GEP->indices().begin(), Base.GEP->indices().end() - 1);
  Indices.push_back(StartIndex);
  Value *Pointer = IRB.CreateGEP(Base.GEP->getSourceElementType(), Base.GEP->getPointerOperand(), Indices);
  Type *ptrElTy = IGCLLVM::getGEPIndexedType(Base.GEP->getSourceElementType(), Indices);

  // Create phi node if pointer is moved in loop
  if (!Step->isZero()) {
    // Add new phi node with pointer as induction variable
    SmallVector<BasicBlock *, 4> Latches;
    L->getLoopLatches(Latches);

    IRB.SetInsertPoint(L->getHeader(), L->getHeader()->begin());
    PHINode *Phi = IRB.CreatePHI(Pointer->getType(), Latches.size() + 1);

    Phi->addIncoming(Pointer, LPH);

    // In each latch increment pointer and add to phi node
    for (auto *L : Latches) {
      IRB.SetInsertPoint(&L->back());
      Value *Inc = IRB.CreateGEP(ptrElTy, Phi, getStepValue(IRB, E, L));
      Phi->addIncoming(Inc, L);
    }

    Pointer = Phi;
  }

  // Replace base GEP
  Base.GEP->replaceAllUsesWith(Pointer);
  Reduced.push_back(Base.GEP);

  // Replace remaining GEPs in the group
  for (auto &Other : Others) {
    IRB.SetInsertPoint(Other.GEP);
    Other.GEP->replaceAllUsesWith(Other.Delta == 0 ? Pointer
                                                   : IRB.CreateGEP(ptrElTy, Pointer, IRB.getInt64(Other.Delta)));
    Reduced.push_back(Other.GEP);
  }
}

// Keeps base GEP untouched and transforms other GEPs in the group to use
// base GEP + constant offset.
void ReductionCandidateGroup::reduceIndexOnly(IGCLLVM::IRBuilder<> &IRB, SCEVExpander &E) {
  for (auto &Other : Others) {
    // TODO If GEPs instructions are in different BBs, base GEP could be moved
    // to one common position (e.g. at the start of the iteration) to allow
    // other GEPs to share the same pointer. But it increases pressure inside loop?
    if (Base.GEP->getParent() == Other.GEP->getParent()) {
      IRB.SetInsertPoint(Other.GEP);
      Other.GEP->replaceAllUsesWith(
          Other.Delta == 0 ? Base.GEP
                           : IRB.CreateGEP(Base.GEP->getResultElementType(), Base.GEP, IRB.getInt64(Other.Delta)));
      Reduced.push_back(Other.GEP);
    }
  }
}

Value *ReductionCandidateGroup::getStepValue(IGCLLVM::IRBuilder<> &IRB, SCEVExpander &E, BasicBlock *BB) {
  if (auto *S = dyn_cast<SCEVConstant>(Step))
    return IRB.getInt64(dyn_cast<SCEVConstant>(Step)->getValue()->getSExtValue());

  if (auto *S = dyn_cast<SCEVUnknown>(Step))
    return S->getValue();

  return E.expandCodeFor(Step, Step->getType(), &BB->back());
}

void Scorer::score(SmallVectorImpl<ReductionCandidateGroup> &Candidates) {
  for (auto &C : Candidates) {
    score(C);
  }
}

void Scorer::score(ReductionCandidateGroup &Candidate) {
  scoreReducedInstructions(Candidate);
  scoreRegisterPressure(Candidate);
}

// Estimate if reduction will remove instructions from loop. Takes into account instructions
// added when lowering GEP to direct pointer arithmetics.
void Scorer::scoreReducedInstructions(ReductionCandidateGroup &Candidate) {
  auto &Cheapest = Candidate.getCheapestCandidate();

  // Score "index * sizeof(type) + base_ptr". Split into parts:
  //   1. "index"          - number of instructions required to calculate value (might be 0
  //                         if index is just loop's induction variable).
  //   2. "* sizeof(type)" - single "shl" instruction
  //   3. "+ base_ptr"     - single "add" instruction
  int score = 0;

  if (!Candidate.Step->isZero()) {
    // Reduction adds new instruction - incrementation of new induction variable at the end
    // of the iteration.
    score -= 1;
  }

  // Score "* sizeof(type)"
  if (unsigned(DL.getTypeSizeInBits(Cheapest.GEP->getResultElementType())) > 8)
    score += 1;

  // Score "+ base_ptr"
  score += estimatePointerAddition(Candidate);
  bool ContainsMuli64 = false;

  // Score "index"
  score += estimateIndexInstructions(*Candidate.getLoop(), Cheapest.GEP, ContainsMuli64);

  Candidate.Score.ReducesInstructions = score > 0;
  Candidate.Score.ContainsMuli64 = ContainsMuli64;
}

// Estimate if compiler will emit "base_ptr + offset" instruction.
int Scorer::estimatePointerAddition(ReductionCandidateGroup &Candidate) {
  if (Candidate.Base.GEP->getPointerAddressSpace() != ADDRESS_SPACE_LOCAL)
    return 1;

  // Local address space pointers are replaced to constant integers by GenIRLowering
  // pass (CG phase), giving more chances to optimize pointer arithmetics.

  FunctionMetaData &FMD = MMD.FuncMD[Candidate.Base.GEP->getFunction()];

  // Check if base pointer is zero ("0 + offset" will be optimized out).
  for (auto &Offets : FMD.localOffsets) {
    if (Candidate.Base.GEP->getPointerOperand() == Offets.m_Var) {
      if (Offets.m_Offset == 0)
        return 0;
      break; // exit early
    }
  }

  return 1;
}

// Estimates how many instructions required to calculate index would be reduced to preheader.
// This differs from checking SCEV expression size, which it might represent simplified index
// calculation.
// Sets ContainsMuli64 flag to show if i64 multiplication is present in the gep index calculation.
int Scorer::estimateIndexInstructions(const Loop &L, GetElementPtrInst *GEP, bool &ContainsMuli64) {
  Instruction *Index = dyn_cast<Instruction>(*(GEP->operands().end() - 1));
  if (!Index)
    return 0;

  // Keep track of visited instructions (don't go into cycles).
  SmallPtrSet<Instruction *, 8> Visited;

  std::function<int(Instruction *)> Visit = [&](Instruction *I) {
    if (Visited.insert(I).second == false)
      return 0;

    if (!L.contains(I))
      return 0;

    int instructions = 0;

    switch (I->getOpcode()) {
    case Instruction::PHI:
      if (I->getParent() == L.getHeader())
        return 0; // no need to go further
      break;
    case Instruction::SExt:
    case Instruction::ZExt:
      break;
    default:
      instructions += 1;
    }

    if (I->getOpcode() == Instruction::Mul && I->getType()->isIntegerTy(64))
      ContainsMuli64 = true;

    for (auto *It = I->operands().begin(); It != I->operands().end(); ++It) {
      if (auto *Next = dyn_cast<Instruction>(It))
        instructions += Visit(Next);
    }

    return instructions;
  };

  return Visit(Index);
}

// Returns estimated increase in pressure.
void Scorer::scoreRegisterPressure(ReductionCandidateGroup &Candidate) {
  auto &Cheapest = Candidate.getCheapestCandidate();

  // Method SCEVExpander::isHighCostExpansion could be a good candidate to use here, but:
  //   1. It doesn't return increase value, only boolean if expansion fits in given budget.
  //   2. Depends on correctly defined TargetTransformInfo.

  // To estimate increase in register pressure, iterate over all instructions inside loop
  // that used to calculate index and estimate register size. In reality, change in
  // pressure might be different, as SCEVExpander might simplify calculations.

  Instruction *Index = dyn_cast<Instruction>(*(Cheapest.GEP->operands().end() - 1));
  if (!Index)
    return;

  auto *L = Candidate.getLoop();
  auto *F = Cheapest.GEP->getParent()->getParent();
  uint SIMD = numLanes(RPE.bestGuessSIMDSize(F));

  ValueSet Instructions;

  // Keep track of visited instructions (don't go into cycles).
  ValueSet Visited;

  std::function<void(Instruction *)> Visit = [&](Instruction *I) {
    if (Visited.insert(I).second == false)
      return;

    if (!L->contains(I))
      return;

    switch (I->getOpcode()) {
    case Instruction::PHI:
      if (I->getParent() == L->getHeader())
        return; // no need to go further
      break;
    default:
      Instructions.insert(I);
    }

    for (auto *It = I->operands().begin(); It != I->operands().end(); ++It) {
      if (auto *Next = dyn_cast<Instruction>(It))
        Visit(Next);
    }
  };

  Visit(Index);

  uint SizeInBytes = RPE.estimateSizeInBytes(Instructions, *F, SIMD, &WI);

  Candidate.Score.RegisterPressure = RPE.bytesToRegisters(SizeInBytes);
}

void Analyzer::analyze(SmallVectorImpl<ReductionCandidateGroup> &Result) {
  // Require simple loop with one entry and one latch.
  if (!IGCLLVM::isInnermost(&L) || !L.isLoopSimplifyForm() || !L.getLoopLatch())
    return;

  Candidates.clear();

  for (auto *BB : L.getBlocks()) {
    for (auto &I : *BB) {
      if (auto *GEP = dyn_cast<GetElementPtrInst>(&I))
        analyzeGEP(GEP);
    }
  }

  // After grouping candidates together, it's possible some groups don't have
  // at least one valid pointer to use as a base for reduction.
  Candidates.erase(std::remove_if(Candidates.begin(), Candidates.end(),
                                  [](ReductionCandidateGroup &C) {
                                    if (C.isValid())
                                      return false;
                                    LLVM_DEBUG(dbgs() << "  dropping candidate with no GEP valid for reduction: ";
                                               C.print(dbgs()); dbgs() << "\n");
                                    return true;
                                  }),
                   Candidates.end());

  Result.append(Candidates.begin(), Candidates.end());
}

// Checks if GEP instruction can be transformed.
void Analyzer::analyzeGEP(GetElementPtrInst *GEP) {
  if (!doInitialValidation(GEP))
    return;

  Value *Index = *(GEP->indices().end() - 1);

  const SCEV *S = SE.getSCEV(Index);
  if (!SCEVHelper::isValid(S))
    return;

  Analyzer::DeconstructedSCEV Result;
  if (!deconstructSCEV(S, Result))
    return;

  if (!Result.isValid())
    return;

  const SCEV *Start = Result.Start;
  const SCEV *Step = Result.Step;

  if (S->getType() != Start->getType())
    Start = isa<SCEVSignExtendExpr>(S) ? SE.getSignExtendExpr(Start, S->getType())
                                       : SE.getZeroExtendExpr(Start, S->getType());

  // Try adding reduction to existing group
  for (auto &c : Candidates) {
    if (c.addToGroup(SE, GEP, Start, Step)) {
      LLVM_DEBUG(dbgs() << "  updated group of candidates: "; c.print(dbgs()); dbgs() << "\n");
      return;
    }
  }

  Candidates.emplace_back(&L, &DT, GEP, Start, Step);
  LLVM_DEBUG(dbgs() << "  found new candidate to optimize: "; Candidates.back().print(dbgs()); dbgs() << "\n");
}

// Does early GEP instruction analysis, before reaching for SCEV analysis.
// Returns false if GEP can be dropped early.
bool Analyzer::doInitialValidation(GetElementPtrInst *GEP) {
  // Optimize only explicit global/local memory access. Private memory might be
  // promoted to GRFs, keep them as is.
  auto AddressSpace = GEP->getPointerAddressSpace();
  if (AddressSpace != ADDRESS_SPACE_GLOBAL && AddressSpace != ADDRESS_SPACE_LOCAL &&
      AddressSpace != ADDRESS_SPACE_GENERIC)
    return false;

  // If pointer is instruction, it must be usable in loop preheader.
  if (auto *I = dyn_cast<Instruction>(GEP->getPointerOperand())) {
    if (!DT.dominates(I, L.getLoopPreheader()) && I->getParent() != L.getLoopPreheader())
      return false;
  }

  // TODO: Pointer could be inttopointer cast of value available in loop preheader.
  // In this case transformation is still possible if casting is moved to preheader.

  // Only last index can be variable
  if (!std::all_of(GEP->indices().begin(), GEP->indices().end() - 1, [](Value *V) { return isa<Constant>(V); }))
    return false;
  Value *Index = *(GEP->indices().end() - 1);

  // Make sure last indexed type is array. Don't try to mix up and optimize access
  // to different fields in aggregate type.
  if (GEP->getNumIndices() > 1) {
    SmallVector<Value *, 8> Indices(GEP->indices().begin(), GEP->indices().end() - 1);
    Type *Ty = GetElementPtrInst::getIndexedType(GEP->getSourceElementType(), Indices);
    if (!Ty || !Ty->isArrayTy())
      return false;
  }

  // Don't reduce if index is used outside of loop to access the same pointer.
  // TODO: These accesses could be modified to also use pointer induction variable
  // added by this pass.
  SmallPtrSet<Value *, 8> Visited = {GEP}; // keep track of visited instructions
  std::function<bool(Value *)> CheckOutsideAccess = [&](Value *V) {
    if (Visited.insert(V).second == false)
      return false;

    for (auto It = V->users().begin(); It != V->users().end(); ++It) {
      if (auto *Phi = dyn_cast<PHINode>(*It)) {
        if (CheckOutsideAccess(Phi))
          return true;
      }

      if (auto *OtherGEP = dyn_cast<GetElementPtrInst>(*It)) {
        if (!L.contains(OtherGEP))
          return true;
      }
    }

    return false;
  };
  if (CheckOutsideAccess(Index))
    return false;

  return true;
}

// Takes SCEV expression returned by ScalarEvolution and deconstructs it into
// expected format { start, +, step }. Returns false if expressions can't be
// parsed and reduced.
bool Analyzer::deconstructSCEV(const SCEV *S, Analyzer::DeconstructedSCEV &Result) {
  // In case of ext instruction analyze nested content.
  if (isa<SCEVZeroExtendExpr>(S) || isa<SCEVSignExtendExpr>(S)) {
    if (!deconstructSCEV(dyn_cast<SCEVCastExpr>(S)->getOperand(), Result))
      return false;

    if (S->getType() != Result.Start->getType())
      Result.Start = isa<SCEVSignExtendExpr>(S) ? SE.getSignExtendExpr(Result.Start, S->getType())
                                                : SE.getZeroExtendExpr(Result.Start, S->getType());

    return IGCLLVM::isSafeToExpandAt(Result.Start, &L.getLoopPreheader()->back(), &SE, &E);
  }

  // First check if expression can be fully expanded in preheader. If so, no need
  // to process is further, but instead treat expression as:
  //   { start, +, 0 }
  // This will do LICM-like reduction moving GEP to preheader, without adding new
  // induction variable.
  if (SE.isLoopInvariant(S, &L)) {
    Result.Start = S;
    Result.Step = SE.getConstant(Type::getInt64Ty(L.getHeader()->getContext()), 0);
    return true;
  }

    // Expect SCEV expression:
    //   { start, +, step }
    // where step is constant
    if (auto *Add = dyn_cast<SCEVAddRecExpr>(S))
    {
        if (!Add->isAffine())
            return false;

    if (Add->getNumOperands() != 2)
      return false;

    // Scalar Evolution can produce SCEVAddRecExpr based on boolean type, for example:
    //   {(true + (trunc i16 %localIdX to i1)),+,true}
    // Ignore such expressions.
    Type *Ty = Add->getStart()->getType();
    if (Ty->isIntegerTy() && Ty->getScalarSizeInBits() == 1)
      return false;

    const SCEV *OpStep = Add->getOperand(1);

    // Step must be constant in loop's body.
    if (!SE.isLoopInvariant(OpStep, &L))
      return false;

        Result.Start = Add->getStart();
        Result.Step = OpStep;

    return IGCLLVM::isSafeToExpandAt(Result.Start, &L.getLoopPreheader()->back(), &SE, &E);
  }

  // If expression is:
  //   x + { start, +, step }
  // then change it to:
  //   { start + x, +, step }
  //
  // It is possible that "x" is not constant inside loop, but is recalculated
  // on every iteration. In this case it is not a valid scenario for reduction
  // and will be dropped by IGCLLVM::isSafeToExpandAt.
  if (auto *Add = dyn_cast<SCEVAddExpr>(S)) {
    // There can be only one expression with step != 0.
    Result.Step = SE.getConstant(Type::getInt64Ty(L.getHeader()->getContext()), 0);

    SCEVHelper::SCEVAddBuilder Builder(SE);

    for (auto *Op : Add->operands()) {
      Analyzer::DeconstructedSCEV OpResult;

      if (!deconstructSCEV(Op, OpResult))
        return false;

      if (!OpResult.Step->isZero()) {
        if (!Result.Step->isZero())
          return false; // unsupported expression with multiple steps
        Result.Step = OpResult.Step;
      }

      Builder.add(OpResult.Start);
    }

    Result.Start = Builder.build();

    return IGCLLVM::isSafeToExpandAt(Result.Start, &L.getLoopPreheader()->back(), &SE, &E);
  }

  // If expression is:
  //   x * { start, +, step }
  // then change it to:
  //   { x * start, +, x * step }
  //
  // Warning: GEP's new index will not be a constant integer, but a new SCEV expression.
  if (auto *Mul = dyn_cast<SCEVMulExpr>(S)) {
    // SCEVAddRecExpr will be SCEV with step != 0. Any other SCEV is a multiplier.
    bool FoundAddRec = false;
    SCEVHelper::SCEVMulBuilder StartBuilder(SE), StepBuilder(SE);

    for (auto *Op : Mul->operands()) {
      Analyzer::DeconstructedSCEV OpResult;
      if (!deconstructSCEV(Op, OpResult))
        return false;

      if (OpResult.Step->isZero()) {
        StartBuilder.add(OpResult.Start);
        StepBuilder.add(OpResult.Start);
      } else {
        if (FoundAddRec)
          return false; // unsupported expression with multiple SCEVAddRecExpr
        FoundAddRec = true;

        StartBuilder.add(OpResult.Start);
        StepBuilder.add(OpResult.Step);
      }
    }

    if (!FoundAddRec)
      return false;

    Result.Start = StartBuilder.build();
    Result.Step = StepBuilder.build();
    Result.ConvertedMulExpr = true;

    if (!SE.isLoopInvariant(Result.Step, &L))
      return false;

    return IGCLLVM::isSafeToExpandAt(Result.Start, &L.getLoopPreheader()->back(), &SE, &E);
  }

  return false;
}

bool Analyzer::DeconstructedSCEV::isValid() {
  if (!Start || !Step)
    return false;

  // Validate step.
  auto Ty = SCEVHelper::dropExt(Step)->getSCEVType();

  if (Ty == scConstant)
    return true;

  bool IsMul = Ty == scMulExpr || ConvertedMulExpr;
  if (IsMul && IGC_IS_FLAG_ENABLED(EnableGEPLSRMulExpr))
    return true;

  return IGC_IS_FLAG_ENABLED(EnableGEPLSRUnknownConstantStep);
}

RegisterPressureTracker::RegisterPressureTracker(Function &F, CodeGenContext &CGC, IGCLivenessAnalysis &RPE,
                                                 IGCFunctionExternalRegPressureAnalysis &FRPE, WIAnalysisRunner &WI)
    : RPE(RPE), WI(WI) {
  MaxAllowedPressure =
      static_cast<unsigned>(CGC.getNumGRFPerThread() * IGC_GET_FLAG_VALUE(GEPLSRThresholdRatio) / 100.0f);

  FunctionExternalPressure = FRPE.getExternalPressureForFunction(&F);
}

void RegisterPressureTracker::trackDeletedInstruction(Value *V) {
  if (auto *I = dyn_cast<Instruction>(V))
    BBsToUpdate.insert(I->getParent());

  for (auto It = RPE.getInSet().begin(); It != RPE.getInSet().end(); ++It) {
    if (It->second.count(V))
      BBsToUpdate.insert(It->first);
  }

  for (auto It = RPE.getOutSet().begin(); It != RPE.getOutSet().end(); ++It) {
    if (It->second.count(V))
      BBsToUpdate.insert(It->first);
  }
}

bool RegisterPressureTracker::fitsPressureThreshold(ReductionCandidateGroup &C) {
  BasicBlock *Preheader = C.getLoop()->getLoopPreheader();
  auto *F = Preheader->getParent();
  uint SIMD = numLanes(RPE.bestGuessSIMDSize(F));

  unsigned MaxLoopPressure = RPE.getMaxRegCountForLoop(*C.getLoop(), SIMD, &WI);
  unsigned AdditionalPressure = C.getScore().RegisterPressure;

  InsideBlockPressureMap BBListing;
  RPE.collectPressureForBB(*Preheader, BBListing, SIMD, &WI);
  unsigned LoopExternalPressureInBytes = BBListing[cast<Value>(Preheader->getTerminator())];
  unsigned LoopExternalPressure = RPE.bytesToRegisters(LoopExternalPressureInBytes);

  unsigned InitialPressure = FunctionExternalPressure + MaxLoopPressure;
  unsigned EstimatedPressure = InitialPressure + AdditionalPressure;

  // Try not to increase register pressure above threshold.
  if (EstimatedPressure >= MaxAllowedPressure) {
    // Even if the optimization icnreases register pressure, apply it in case we can move mul i64 to preheader.
    // This heuristic is based on the fact that mul i64 is expensive instruction and potential spills are generated out
    // of the loop.
    unsigned NewInternalLoopPressure = LoopExternalPressure - MaxLoopPressure + AdditionalPressure;
    if (C.getScore().ContainsMuli64 && NewInternalLoopPressure < MaxAllowedPressure) {
      return true;
    }

    LLVM_DEBUG(dbgs() << "  Estimated register pressure " << EstimatedPressure << " above threshold "
                      << MaxAllowedPressure << "; can't fully reduce ";
               C.print(dbgs()); dbgs() << "\n");
    return false;
  }

  return true;
}

void RegisterPressureTracker::updatePressure(ReductionCandidateGroup &C, SCEVExpander &E) {


  // Refresh all BBs in loop.
  BBsToUpdate.insert(C.getLoop()->getBlocks().begin(), C.getLoop()->getBlocks().end());

  if (C.getReductionType() == REDUCE_TO_PREHEADER) {
    // When adding new induction variable to loop, we need to refresh loop's preheader and body (all BBs).
    // SCEVExpander can push new instructions to outer loops, including adding new induction variables.
    // Find outermost loop touched by SCEVExpander and refresh register estimation for it.

    // Query SCEVExpander for new instructions.
    auto AllInsertedInstructions = E.getAllInsertedInstructions();

    // Start searching from inner loop.
    Loop *TopLoop = C.getLoop();

    for (auto *I : AllInsertedInstructions) {
      // Skip if instruction was inserted by previous reductions.
      if (VisitedNewInsts.insert(I).second == false)
        continue;

      if (TopLoop->contains(I))
        continue;

      if (TopLoop->getLoopPreheader() == I->getParent())
        continue;

      // At this point we know that instruction is not in current loop. Check outer loops.

      Loop *L;
      for (L = TopLoop->getParentLoop(); L != nullptr; L = L->getParentLoop()) {
        if (L->contains(I) || L->getLoopPreheader() == I->getParent())
          break;
      }

      if (L) {
        // Found new loop.
        TopLoop = L;
      } else {
        LLVM_DEBUG(dbgs() << "  Found instruction in basic block [" << I->getParent()->getName()
                          << "] not present in any loop; instruction: ";
                   I->print(dbgs(), true); dbgs() << "\n");
        BBsToUpdate.insert(I->getParent());
      }
    }

    BBsToUpdate.insert(TopLoop->block_begin(), TopLoop->block_end());
    BBsToUpdate.insert(TopLoop->getLoopPreheader());
  }

  // BBs with removed instructions should be already collected in Reducer::cleanup.

  Function *F = C.getLoop()->getLoopPreheader()->getParent();

  if (!BBsToUpdate.empty()) {
    RPE.rerunLivenessAnalysis(*F, &BBsToUpdate);
    BBsToUpdate.clear();
  }
}

bool Reducer::reduce(SmallVectorImpl<ReductionCandidateGroup> &Candidates) {
  if (Candidates.empty())
    return false;

  bool changed = false;

  // If LICM is disabled, drop all candidates that would move only one GEP instruction to preheader.
  if (!AllowLICM) {
    Candidates.erase(std::remove_if(Candidates.begin(), Candidates.end(),
                                    [](ReductionCandidateGroup &C) { return C.getGEPCount() < 2; }),
                     Candidates.end());
  }

  // Sort candidates in descending order from best to worse:
  //  1. First by number of reduced GEP instructions (higher is better).
  //  2. Then by estimated register pressure increase (lower is better).
  auto Comparator = [](ReductionCandidateGroup &L, ReductionCandidateGroup &R) {
    if (L.getGEPCount() == R.getGEPCount())
      return L.getScore().RegisterPressure < R.getScore().RegisterPressure;

    return R.getGEPCount() < L.getGEPCount();
  };
  std::sort(Candidates.begin(), Candidates.end(), Comparator);

  // After sorting candidates, iterate over them and do reduction, keeping
  // register pressure under threshold.
  for (auto It = Candidates.begin(); It != Candidates.end(); ++It) {
    if (IGC_IS_FLAG_ENABLED(EnableGEPLSRToPreheader) && It->getScore().ReducesInstructions) {
      // It is beneficial to reduce to preheader, but keep register pressure in check.
      if (!RPT.fitsPressureThreshold(*It)) {
        // Above threshold, just simplify index calculation inside loop.
        It->setReductionType(REDUCE_INDEX_ONLY);
      }
    } else {
      // If nothing from the loop would be reduced to preheader, only
      // simplify index calculation inside loop.
      It->setReductionType(REDUCE_INDEX_ONLY);
    }

    // Check if nothing would be reduced/simplified.
    if (It->getReductionType() == REDUCE_INDEX_ONLY && It->getGEPCount() == 1)
      continue;

    LLVM_DEBUG(dbgs() << "  Executing reduction="
                      << (It->getReductionType() == REDUCE_TO_PREHEADER ? "TO_PREHEADER" : "INDEX_ONLY") << " for ";
               It->print(dbgs()); dbgs() << "\n");

    It->transform(IRB, E);

    cleanup(*It);
    RPT.updatePressure(*It, E);

    changed = true;
  }

  return changed;
}

void Reducer::cleanup(ReductionCandidateGroup &C) {
  // Delete GEP instructions together with index calculations. Inform Register
  // Pressure Estimator about removed instructions.
  for (auto *GEP : C.getReduced()) {
    RecursivelyDeleteTriviallyDeadInstructions(GEP, nullptr, nullptr,
                                               [&](Value *V) { RPT.trackDeletedInstruction(V); });
  }
}

// If SCEV is zext/sext, drop extend.
const SCEV *SCEVHelper::dropExt(const SCEV *S) {
  do {
    if (auto *Zext = dyn_cast<SCEVZeroExtendExpr>(S))
      S = Zext->getOperand();
    else if (auto *Sext = dyn_cast<SCEVSignExtendExpr>(S))
      S = Sext->getOperand();
    else
      break;
  } while (true);

  return S;
}

// Returns true is SCEV expression legal.
bool SCEVHelper::isValid(const SCEV *S) {
  if (isa<SCEVCouldNotCompute>(S))
    return false;

  // Scalar Evolution doesn't have SCEV expression for bitwise-and. Instead,
  // if possible, SE produces expressions for any integer size, leaving cleanup
  // to legalization pass. For example this code:
  //     %1 = shl i64 %0, 32
  //     %2 = ashr exact i64 %1, 30
  // produces i34 integer SCEV.
  //
  // By default don't allow illegal integer types.
  if (IGC_IS_FLAG_ENABLED(EnableGEPLSRAnyIntBitWidth))
    return true;

  std::function<bool(Type *)> IsInvalidInt = [](Type *Ty) {
    if (!Ty->isIntegerTy())
      return false;

    auto bits = Ty->getScalarSizeInBits();
    switch (bits) {
    case 8:
    case 16:
    case 32:
    case 64:
      return false;
    default:
      return bits > 8;
    }
  };

  bool HasInvalidInt = SCEVExprContains(S, [&](const SCEV *S) {
    if (auto *Cast = dyn_cast<SCEVCastExpr>(S))
      return IsInvalidInt(Cast->getOperand()->getType()) || IsInvalidInt(Cast->getType());
    return false;
  });

  LLVM_DEBUG(if (HasInvalidInt) {
    dbgs() << "  Dropping SCEV with invalid integer type: ";
    S->print(dbgs());
    dbgs() << "\n";
  });

  return !HasInvalidInt;
}

bool SCEVHelper::isEqual(const SCEV *A, const SCEV *B) {
  // Scalar Evolution keeps unique SCEV instances, so we can compare pointers.
  if (A == B)
    return true;

  if (A->getSCEVType() != B->getSCEVType())
    return false;

  switch (A->getSCEVType()) {
  case scConstant:
    // Can be different bit width, but same integer value.
    return cast<SCEVConstant>(A)->getValue()->getZExtValue() == cast<SCEVConstant>(B)->getValue()->getZExtValue();
  default:
    return false;
  }
}

SCEVHelper::SCEVAddBuilder &SCEVHelper::SCEVAddBuilder::add(const SCEV *S, bool Negative) {
  IGC_ASSERT(S->getType()->isIntegerTy());

  // strip extend
  if (DropExt)
    S = SCEVHelper::dropExt(S);

  if (auto *Expr = dyn_cast<SCEVAddExpr>(S)) {
    for (auto *Op : Expr->operands())
      add(Op, Negative);
    return *this;
  }

  Ops.emplace_back(S, Negative);

  return *this;
}

const SCEV *SCEVHelper::SCEVAddBuilder::build() {
  // ScalarEvolution::getAddExpr requires all operands to have the same
  // type. First find the widest type.
  Type *T = nullptr;
  for (auto *It = Ops.begin(); It != Ops.end(); ++It) {
    T = T ? SE.getWiderType(T, It->S->getType()) : It->S->getType();
  }

  // Join list of operands, extending type if required.
  SmallVector<const SCEV *, 16> FinalOps;

  for (auto *It = Ops.begin(); It != Ops.end(); ++It) {
    const SCEV *S = It->S;
    S = S->getType() == T ? S : SE.getSignExtendExpr(S, T);
    FinalOps.push_back(It->Negative ? SE.getNegativeSCEV(S) : S);
  }

  return SE.getAddExpr(FinalOps);
}

SCEVHelper::SCEVMulBuilder &SCEVHelper::SCEVMulBuilder::add(const SCEV *S) {
  IGC_ASSERT(S->getType()->isIntegerTy());

  Ops.emplace_back(S);

  return *this;
}

const SCEV *SCEVHelper::SCEVMulBuilder::build() {
  // ScalarEvolution::getMulExpr requires all operands to have the same
  // type. First find the widest type.
  Type *T = nullptr;
  for (auto S : Ops) {
    T = T ? SE.getWiderType(T, S->getType()) : S->getType();
  }

  // Join list of operands, extending type if required.
  SmallVector<const SCEV *, 4> FinalOps;

  for (auto S : Ops) {
    FinalOps.push_back(S->getType() == T ? S : SE.getSignExtendExpr(S, T));
  }

  return SE.getMulExpr(FinalOps);
}

GEPLoopStrengthReduction::GEPLoopStrengthReduction(bool AllowLICM) : FunctionPass(ID), AllowLICM(AllowLICM) {
  initializeGEPLoopStrengthReductionPass(*PassRegistry::getPassRegistry());
}

void GEPLoopStrengthReduction::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<CodeGenContextWrapper>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<IGCFunctionExternalRegPressureAnalysis>();
  AU.addRequired<IGCLivenessAnalysis>();
  AU.addRequired<llvm::LoopInfoWrapperPass>();
  AU.addRequired<MetaDataUtilsWrapper>();
  AU.addRequired<llvm::ScalarEvolutionWrapperPass>();
}

bool GEPLoopStrengthReduction::runOnFunction(llvm::Function &F) {
  if (F.hasOptNone())
    return false;

  LLVM_DEBUG(dbgs() << "  Running on function " << F.getName()
                    << " options: MaxPressureRatio=" << IGC_GET_FLAG_VALUE(GEPLSRThresholdRatio)
                    << " EnableLsrToPreheader=" << IGC_GET_FLAG_VALUE(EnableGEPLSRToPreheader) << "\n");

  auto &CGC = *getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  auto &DL = F.getParent()->getDataLayout();
  auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  auto &MMD = *getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  auto &RPE = getAnalysis<IGCLivenessAnalysis>();
  auto &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();

  // Note: FRPE is a Module analysis and currently runs only once.
  // If function A calls function B then it's possible that transformation of function A
  // increases pressure so LSR reduction should not be applied in function B, but we don't
  // recompute FRPE to save compile time, so reduction might apply for loops in function B.
  auto &FRPE = getAnalysis<IGCFunctionExternalRegPressureAnalysis>();
  auto *WI = &FRPE.getWIAnalysis(&F);

  // Using one SCEV expander between all reductions reduces number of duplicated new instructions.
  auto E = SCEVExpander(SE, DL, "gep-loop-strength-reduction");

  SmallVector<ReductionCandidateGroup, 32> Candidates;

  RegisterPressureTracker RPT(F, CGC, RPE, FRPE, *WI);

  for (Loop *L : LI.getLoopsInPreorder())
    Analyzer(DL, DT, *L, LI, SE, E).analyze(Candidates);

  if (Candidates.empty())
    return false;

  Scorer(DL, MMD, RPE, *WI).score(Candidates);

  IGCLLVM::IRBuilder<> IRB(F.getContext());

  bool changed = Reducer(IRB, RPT, E, AllowLICM).reduce(Candidates);
  if (changed)
    LLVM_DEBUG(dbgs() << "  Modified function " << F.getName() << "\n");

  return changed;
}

// Register pass to igc-opt
#define PASS_FLAG "igc-gep-loop-strength-reduction"
#define PASS_DESCRIPTION "Reduces strength of GEP instructions in loops"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GEPLoopStrengthReduction, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(IGCFunctionExternalRegPressureAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(IGCLivenessAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
IGC_INITIALIZE_PASS_END(GEPLoopStrengthReduction, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char GEPLoopStrengthReduction::ID = 0;

FunctionPass *IGC::createGEPLoopStrengthReductionPass(bool AllowLICM) {
  return new GEPLoopStrengthReduction(AllowLICM);
}
