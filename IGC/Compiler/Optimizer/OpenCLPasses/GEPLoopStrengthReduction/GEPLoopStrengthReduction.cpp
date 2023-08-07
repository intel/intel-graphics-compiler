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
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include "llvm/Support/Debug.h"
#include <llvm/Transforms/Utils/Local.h>
#include "llvmWrapper/Transforms/Utils/LoopUtils.h"
#include "llvmWrapper/Transforms/Utils/ScalarEvolutionExpander.h"
#include "common/LLVMWarningsPop.hpp"

#include "Probe/Assertion.h"
#include "common/igc_regkeys.hpp"
#include "common/IGCIRBuilder.h"
#include "Compiler/CISACodeGen/RegisterPressureEstimate.hpp"
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
//
// TODO: At the moment pass accepts constant indices as llvm::Constant. These could be any llvm::Value
// as long as value doesn't change inside loop body (is constant for full loop lifetime).


enum ReductionType
{
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
struct Score
{
    Score() : ReducedInstructions(0), RegisterPressure(0) {}

    // How many instructions are estimated to be reduced from loop.
    // Can be negative if instructions are added to loop.
    int ReducedInstructions;

    // Estimated increase in register pressure when reducing to loop's preheader.
    unsigned RegisterPressure;
};


// Single GEP instruction that is a candiate for reduction. It is expressed by SCEV:
//     { start, +, step }
// Where step is constant integer (including 0), and start can be calculated in loop's preheader.
struct ReductionCandidate
{
    ReductionCandidate(GetElementPtrInst *GEP, const SCEV *S, int64_t Delta)
        : GEP(GEP), S(S), Delta(Delta) {}

    bool operator==(const ReductionCandidate &RHS) const
    {
        return GEP == RHS.GEP && S == RHS.S && Delta == RHS.Delta;
    }

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
class ReductionCandidateGroup
{
friend class Scorer;

public:

    ReductionCandidateGroup(GetElementPtrInst *GEP, const SCEV *S, int64_t Step)
        : Step(Step), Base(GEP, S, 0), RT(REDUCE_TO_PREHEADER) {}

    bool addToGroup(ScalarEvolution &SE, GetElementPtrInst *GEP, const SCEV *S, int64_t Step);

    void transform(IGCLLVM::IRBuilder<> &IRB, Loop &L, SCEVExpander &E);

    // Return how many GEP instructions will be reduced.
    uint32_t getGEPCount() { return 1 + Others.size(); }

    ReductionType getReductionType() { return RT; }
    void setReductionType(ReductionType RT) { this->RT = RT; }

    Score getScore() { return Score; }

    void print(raw_ostream &OS);

private:

    void append(GetElementPtrInst *GEP, const SCEV *S, int64_t Delta);
    void append(ReductionCandidate& C) { append(C.GEP, C.S, C.Delta); }

    ReductionCandidate& getCheapestCandidate();

    void swapBase(ReductionCandidate &C);

    void reduceToPreheader(IGCLLVM::IRBuilder<> &IRB, Loop &L, SCEVExpander &E);
    void reduceIndexOnly(IGCLLVM::IRBuilder<> &IRB, Loop &L, SCEVExpander &E);

    // Base GEP to reduce
    ReductionCandidate Base;

    // Other GEPs that can be reduced together with base
    SmallVector<ReductionCandidate, 4> Others;

    // Increment step value
    int64_t Step;

    // Group member with the smallest SCEV expression. Preferred for reduction
    // to preheader, as it should give smallest increase in register pressure.
    std::optional<ReductionCandidate> Cheapest;

    Score Score;

    ReductionType RT;
};


// Scores reduction candidates:
//   1. Estimates removed instructions by reducing "base_ptr + index * sizeof(type)".
//   2. Estimates increase in register pressure.
class Scorer
{
public:
    Scorer(const DataLayout &DL, Loop &L, ModuleMetaData &MMD)
        : DL(DL), L(L), MMD(MMD) {}

    void score(SmallVectorImpl<ReductionCandidateGroup> &Candidates);

private:
    void score(ReductionCandidateGroup &Candidate);

    void scoreReducedInstructions(ReductionCandidateGroup &Candidate);
    void scoreRegisterPressure(ReductionCandidateGroup &Candidate);

    int estimateIndexInstructions(GetElementPtrInst *GEP);
    int estimatePointerAddition(ReductionCandidateGroup& Candidate);

    const DataLayout &DL;
    Loop &L;
    ModuleMetaData &MMD;
};



// Does reduction on single loop.
class Reducer
{
public:
    Reducer(const DataLayout &DL, DominatorTree &DT, Loop &L, LoopInfo &LI, ModuleMetaData &MMD, RegisterPressureEstimate &RPE, ScalarEvolution &SE, unsigned MaxPressure, bool AllowLICM)
        : DT(DT), L(L), LI(LI), RPE(RPE), SE(SE), E(SE, DL, "gep-loop-strength-reduction"), MaxPressure(MaxPressure), AllowLICM(AllowLICM), Scorer(DL, L, MMD) {}

    bool reduce();

private:
    void analyzeGEP(GetElementPtrInst *GEP);
    bool doInitialValidation(GetElementPtrInst *GEP);

    bool deconstructSCEV(const SCEV *S, const SCEV *&Start, int64_t &Step);

    void filterCandidates();

    DominatorTree &DT;
    Loop &L;
    LoopInfo &LI;
    RegisterPressureEstimate &RPE;
    ScalarEvolution &SE;
    SCEVExpander E;

    Scorer Scorer;

    bool AllowLICM;
    const unsigned MaxPressure;

    SmallVector<ReductionCandidateGroup, 16> Candidates;
};


// Set of functions/classes helping manipulating SCEV objects.
namespace SCEVHelper
{
    const SCEV *dropExt(const SCEV *S);

    // ScalarEvolution::getAddExpr requires all operands to have the same
    // type. Extend type if required.

    // Builds SCEVAddExpr instance. Function ScalarEvolution::getAddExpr requires all
    // operands to have the same type. This class wraps ScalarEvolution::getAddExpr,
    // but extends operands if it is needed to keep them all in one type.
    class SCEVAddBuilder
    {
    public:
        SCEVAddBuilder(ScalarEvolution &SE, Type *T) :
            SE(SE), T(T)
        {
            IGC_ASSERT_MESSAGE(T->isIntegerTy(), "builder requires integer type");
        }

        SCEVAddBuilder &add(const SCEV *S, bool Negative = false);

        SCEVAddBuilder &addNegative(const SCEV *S) { return add(S, true); }

        const SCEV *build() { return SE.getAddExpr(Ops); }

    private:
        ScalarEvolution &SE;
        Type *T;
        SmallVector<const SCEV*, 16> Ops;
    };
};


class GEPLoopStrengthReduction : public llvm::FunctionPass
{
public:
    static char ID; // Pass identification, replacement for typeid

    GEPLoopStrengthReduction(bool AllowLICM = true);

    ~GEPLoopStrengthReduction() {}

    virtual llvm::StringRef getPassName() const override
    {
        return "GEPLoopStrengthReduction";
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

    virtual bool runOnFunction(llvm::Function &F) override;

private:
    bool AllowLICM;
};


void ReductionCandidate::print(raw_ostream &OS)
{
    OS << "{gep=";
    GEP->printAsOperand(OS, false);
    OS << ", scev=";
    S->print(OS);
    OS << ", delta=";
    OS << Delta;
    OS << "}";
}


void ReductionCandidateGroup::print(raw_ostream &OS)
{
    OS << "{base=";
    Base.print(OS);
    OS << ", others=[";
    for (auto it = Others.begin(); it != Others.end();)
    {
        it->print(OS);
        ++it;
        if (it != Others.end())
            OS << ", ";
    }
    OS << "], step=";
    OS << Step;
    OS << "}";
}


// If possible, adds another reduction candidate to this group. Reductions
// can be grouped together if:
//   1) delta between start values can be expressed in constant int
//   2) have the same increment step value
// New candidate can take position of group's base if it uses less instructions
// to calculate.
// Returns true if candidate was added to group.
bool ReductionCandidateGroup::addToGroup(ScalarEvolution &SE, GetElementPtrInst *GEP, const SCEV *S, int64_t Step)
{
    if (this->Step != Step)
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
    SCEVHelper::SCEVAddBuilder Builder(SE, SE.getWiderType(S->getType(), Base.S->getType()));
    const SCEVConstant *Sum = dyn_cast<SCEVConstant>(Builder.add(S).addNegative(Base.S).build());
    if (!Sum)
        return false;

    int64_t Delta = Sum->getValue()->getSExtValue();

    // Delta is constant int, reductions can be grouped together.
    append(GEP, S, Delta);

    return true;
}


void ReductionCandidateGroup::append(GetElementPtrInst *GEP, const SCEV *S, int64_t Delta)
{
    Others.emplace_back(GEP, S, Delta);

    // Group changed, forget which member is the cheapest.
    Cheapest.reset();
}


// Sets group's base GEP to provided candidate.
void ReductionCandidateGroup::swapBase(ReductionCandidate &C)
{
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


ReductionCandidate &ReductionCandidateGroup::getCheapestCandidate()
{
    if (Cheapest)
        return Cheapest.value();

    if (Others.empty())
        return Cheapest.emplace(Base);

    auto *BestCandidate = &Base;

    for (auto It = Others.begin(); It != Others.end(); ++It)
    {
        // If lowest delta is selected as base, remaining GEPs in the group will use
        // positive indices only. This should make other load/store optimizations easier.
        if ((It->Delta < BestCandidate->Delta) ||
            (It->Delta == BestCandidate->Delta && It->S->getExpressionSize() < BestCandidate->S->getExpressionSize()))
        {
            BestCandidate = It;
        }
    }

    return Cheapest.emplace(*BestCandidate);
}


void ReductionCandidateGroup::transform(IGCLLVM::IRBuilder<> &IRB, Loop &L, SCEVExpander &E)
{
    switch (RT)
    {
    case REDUCE_TO_PREHEADER:
        return reduceToPreheader(IRB, L, E);
    case REDUCE_INDEX_ONLY:
    default:
        return reduceIndexOnly(IRB, L, E);
    }
}


// Transforms all grouped GEP instructions:
//   1) Calculate start value in loop's preheader
//   2) Add this new pointer as loop's induction variable with constant step
//   3) Replace GEP instructions to use this pointer + constant offset
void ReductionCandidateGroup::reduceToPreheader(IGCLLVM::IRBuilder<> &IRB, Loop &L, SCEVExpander &E)
{
    // Updates group's base to candidate with smallest SCEV expression.
    swapBase(getCheapestCandidate());

    // Create pointer's starting value in preheader
    BasicBlock *LPH = L.getLoopPreheader();
    const SCEV *Start = Base.S;
    Value *StartIndex = E.expandCodeFor(Start, Start->getType(), &LPH->back());

    IRB.SetInsertPoint(&LPH->back());
    SmallVector<Value*, 4> Indices(Base.GEP->indices().begin(), Base.GEP->indices().end() - 1);
    Indices.push_back(StartIndex);
    Value *Pointer = IRB.CreateGEP(Base.GEP->getPointerOperand(), Indices);

    // Create phi node if pointer is moved in loop
    if (Step != 0)
    {
        // Add new phi node with pointer as induction variable
        SmallVector<BasicBlock*, 4> Latches;
        L.getLoopLatches(Latches);

        IRB.SetInsertPoint(L.getHeader(), L.getHeader()->begin());
        PHINode *Phi = IRB.CreatePHI(Pointer->getType(), Latches.size() + 1);

        Phi->addIncoming(Pointer, LPH);

        // In each latch increment pointer and add to phi node
        for (auto *L : Latches)
        {
            IRB.SetInsertPoint(&L->back());
            Value *Inc = IRB.CreateGEP(Phi, IRB.getInt64(Step));
            Phi->addIncoming(Inc, L);
        }

        Pointer = Phi;
    }

    // Replace base GEP
    Base.GEP->replaceAllUsesWith(Pointer);
    RecursivelyDeleteTriviallyDeadInstructions(Base.GEP);

    // Replace remaining GEPs in the group
    for (auto &Other : Others)
    {
        IRB.SetInsertPoint(Other.GEP);
        Other.GEP->replaceAllUsesWith(Other.Delta == 0 ? Pointer : IRB.CreateGEP(Pointer, IRB.getInt64(Other.Delta)));
        RecursivelyDeleteTriviallyDeadInstructions(Other.GEP);
    }
}


// Keeps base GEP untouched and transforms other GEPs in the group to use
// base GEP + constant offset.
void ReductionCandidateGroup::reduceIndexOnly(IGCLLVM::IRBuilder<> &IRB, Loop &L, SCEVExpander &E)
{
    for (auto &Other : Others)
    {
        // TODO If GEPs instructions are in different BBs, base GEP could be moved
        // to one common position (e.g. at the start of the iteration) to allow
        // other GEPs to share the same pointer. But it increases pressure inside loop?
        if (Base.GEP->getParent() == Other.GEP->getParent())
        {
            IRB.SetInsertPoint(Other.GEP);
            Other.GEP->replaceAllUsesWith(Other.Delta == 0 ? Base.GEP : IRB.CreateGEP(Base.GEP, IRB.getInt64(Other.Delta)));
            RecursivelyDeleteTriviallyDeadInstructions(Other.GEP);
        }
    }
}


void Scorer::score(SmallVectorImpl<ReductionCandidateGroup> &Candidates)
{
    for (auto &C : Candidates)
    {
        score(C);
    }
}


void Scorer::score(ReductionCandidateGroup &Candidate)
{
    scoreReducedInstructions(Candidate);
    scoreRegisterPressure(Candidate);
}


// Returns estimated number of reduced instructions.
void Scorer::scoreReducedInstructions(ReductionCandidateGroup &Candidate)
{
    auto &Cheapest = Candidate.getCheapestCandidate();

    // Score "index * sizeof(type) + base_ptr". Split into parts:
    //   1. "index"          - number of instructions required to calculate value (might be 0
    //                         if index is just loop's induction variable).
    //   2. "* sizeof(type)" - single "shl" instruction
    //   3. "+ base_ptr"     - single "add" instruction
    int score = 0;

    if (Candidate.Step != 0)
    {
        // Reduction adds new instruction - incrementation of new induction variable at the end
        // of the iteration.
        score -= 1;
    }

    // Score "* sizeof(type)"
    if (unsigned(DL.getTypeSizeInBits(Cheapest.GEP->getResultElementType())) > 8)
        score += 1;

    // Score "+ base_ptr"
    score += estimatePointerAddition(Candidate);

    // Only need to deduce if reduction is net positive, no need to continue if already confirmed.
    if (score < 1)
    {
        // Score "index"
        score += estimateIndexInstructions(Cheapest.GEP);
    }

    Candidate.Score.ReducedInstructions = score;
}


// Estimate if compiler will emit "base_ptr + offset" instruction.
int Scorer::estimatePointerAddition(ReductionCandidateGroup& Candidate)
{
    if (Candidate.Base.GEP->getPointerAddressSpace() != ADDRESS_SPACE_LOCAL)
        return 1;

    // Local address space pointers are replaced to constant integers by GenIRLowering
    // pass (CG phase), giving more chances to optimize pointer arithmetics.

    FunctionMetaData& FMD = MMD.FuncMD[Candidate.Base.GEP->getFunction()];

    // Check if base pointer is zero ("0 + offset" will be optimized out).
    for (auto& Offets : FMD.localOffsets)
    {
        if (Candidate.Base.GEP->getPointerOperand() == Offets.m_Var)
        {
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
int Scorer::estimateIndexInstructions(GetElementPtrInst *GEP)
{
    Instruction* Index = dyn_cast<Instruction>(*(GEP->operands().end() - 1));
    if (!Index)
        return 0;

    // Keep track of visited instructions (don't go into cycles).
    SmallPtrSet<Instruction*, 8> Visited;

    std::function<int(Instruction*)> Visit = [&](Instruction* I) -> int
    {
        if (Visited.insert(I).second == false)
            return 0;

        if (!L.contains(I))
            return 0;

        int instructions = 0;

        switch (I->getOpcode())
        {
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

        for (auto *It = I->operands().begin(); It != I->operands().end(); ++It)
        {
            if (auto *Next = dyn_cast<Instruction>(It))
                instructions += Visit(Next);
        }

        return instructions;
    };

    return Visit(Index);
}


// Returns estimated increase in pressure.
void Scorer::scoreRegisterPressure(ReductionCandidateGroup &Candidate)
{
    auto &Cheapest = Candidate.getCheapestCandidate();

    // Method SCEVExpander::isHighCostExpansion could be a good candidate to use here, but:
    //   1. It doesn't return increase value, only boolean if expansion fits in given budget.
    //   2. Depends on correctly defined TargetTransformInfo.

    // TODO Iterate over instructions to expand and estimate reg pressure.

    // Use expression size as an estimation.
    Candidate.Score.RegisterPressure = Cheapest.S->getExpressionSize();
}


bool Reducer::reduce()
{
    if (!IGCLLVM::isInnermost(&L) || !L.isLoopSimplifyForm())
        return false;

    // Collect candidates
    for (auto *BB : L.getBlocks()) {
        for (auto &I : *BB) {
            if (auto *GEP = dyn_cast<GetElementPtrInst>(&I))
                analyzeGEP(GEP);
        }
    }

    Scorer.score(Candidates);

    filterCandidates();

    if (Candidates.empty())
        return false;

    // Transform candidates
    IGCLLVM::IRBuilder<> IRB(L.getLoopPreheader());

    for (auto &C : Candidates)
    {
        C.transform(IRB, L, E);
    }

    return true;
}


// Checks if GEP instruction can be transformed.
void Reducer::analyzeGEP(GetElementPtrInst *GEP)
{
    if (!doInitialValidation(GEP))
        return;

    Value *Index = *(GEP->indices().end() - 1);

    const SCEV *S = SE.getSCEV(Index);
    if (isa<SCEVCouldNotCompute>(S))
        return;

    const SCEV *Start = nullptr;
    int64_t Step = 0;

    if (!deconstructSCEV(S, Start, Step))
        return;

    // Try adding reduction to existing group
    for (auto &c : Candidates)
    {
        if (c.addToGroup(SE, GEP, Start, Step))
        {
            LLVM_DEBUG(
                dbgs() << "  updated group of candidates: ";
                c.print(dbgs());
                dbgs() << "\n");
            return;
        }
    }

    Candidates.emplace_back(GEP, Start, Step);
    LLVM_DEBUG(
        dbgs() << "  found new candidate to optimize: ";
        Candidates.back().print(dbgs());
        dbgs() << "\n");
}

// Does early GEP instruction analysis, before reaching for SCEV analysis.
// Returns false if GEP can be dropped early.
bool Reducer::doInitialValidation(GetElementPtrInst* GEP)
{
    // If pointer is instruction, it must be usable in loop preheader.
    if (auto* I = dyn_cast<Instruction>(GEP->getPointerOperand()))
    {
        if (!DT.dominates(I, L.getLoopPreheader()))
            return false;
    }

    // TODO: Pointer could be inttopointer cast of value available in loop preheader.
    // In this case transformation is still possible if casting is moved to preheader.

    // Only last index can be variable
    if (!std::all_of(GEP->indices().begin(), GEP->indices().end() - 1, [](Value* V) { return isa<Constant>(V); }))
        return false;
    Value* Index = *(GEP->indices().end() - 1);

    // Make sure last indexed type is array. Don't try to mix up and optimize access
    // to different fields in aggregate type.
    if (GEP->getNumIndices() > 1)
    {
        SmallVector<Value*, 8> Indices(GEP->indices().begin(), GEP->indices().end() - 1);
        Type* Ty = GetElementPtrInst::getIndexedType(GEP->getSourceElementType(), Indices);
        if (!Ty || !Ty->isArrayTy())
            return false;
    }

    // Don't reduce if index is used outside of loop to access the same pointer.
    // TODO: These accesses could be modified to also use pointer induction variable
    // added by this pass.
    SmallPtrSet<Value*, 8> Visited = { GEP }; // keep track of visited instructions
    std::function<bool(Value*)> CheckOutsideAccess = [&](Value* V) -> bool
    {
        if (Visited.insert(V).second == false)
            return false;

        for (auto It = V->users().begin(); It != V->users().end(); ++It)
        {
            if (auto* Phi = dyn_cast<PHINode>(*It))
            {
                if (CheckOutsideAccess(Phi))
                    return true;
            }

            if (auto* OtherGEP = dyn_cast<GetElementPtrInst>(*It))
            {
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
bool Reducer::deconstructSCEV(const SCEV *S, const SCEV *&Start, int64_t &Step)
{
    S = SCEVHelper::dropExt(S);

    // First check if expression can be fully expanded in preheader. If so, no need
    // to process is further, but instead treat expression as:
    //   { start, +, 0 }
    // This will do LICM-like reduction moving GEP to preheader, without adding new
    // induction variable.
    if (IGCLLVM::isSafeToExpandAt(S, &L.getLoopPreheader()->back(), &SE, &E))
    {
        Start = S;
        Step = 0;
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

        const SCEVConstant *Op = dyn_cast<SCEVConstant>(Add->getOperand(1));
        if (!Op)
            return false;

        Start = Add->getStart();
        Step = Op->getValue()->getSExtValue();

        return IGCLLVM::isSafeToExpandAt(Start, &L.getLoopPreheader()->back(), &SE, &E);
    }

    // If expression is:
    //   x + { start, +, step }
    // then change it to:
    //   { start + x, +, step }
    //
    // It is possible that "x" is not constant inside loop, but is recalculated
    // on every iteration. In this case it is not a valid scenario for reduction
    // and will be dropped by IGCLLVM::isSafeToExpandAt.
    if (auto *Add = dyn_cast<SCEVAddExpr>(S))
    {
        // There can be only one expression with step != 0.
        Step = 0;

        const SCEV *OpSCEV = nullptr;
        int64_t OpStep = 0;
        SCEVHelper::SCEVAddBuilder Builder(SE, S->getType());

        for (auto *Op : Add->operands())
        {
            if (!deconstructSCEV(Op, OpSCEV, OpStep))
                return false;

            if (OpStep != 0)
            {
                if (Step != 0)
                    return false; // unsupported expression with multiple steps
                Step = OpStep;
            }

            Builder.add(OpSCEV);
        }

        Start = Builder.build();

        return IGCLLVM::isSafeToExpandAt(Start, &L.getLoopPreheader()->back(), &SE, &E);
    }

    return false;
}


// Drops GEP instructions unfit for reduction.
void Reducer::filterCandidates()
{
    LLVM_DEBUG(
        dbgs() << "  found " << Candidates.size() << " candidates to reduce in loop " << L.getName() << "\n");

    // If LICM is disabled, drop all candidates that would move only one GEP instruction to preheader.
    if (!AllowLICM)
    {
        Candidates.erase(
            std::remove_if(Candidates.begin(), Candidates.end(),
                [](ReductionCandidateGroup &C) { return C.getGEPCount() < 2; }),
            Candidates.end());
    }

    // Sort candidates in descending order from best to worse:
    //  1. First by number of reduced GEP instructions (higher is better).
    //  2. Then by estimated register pressure increase (lower is better).
    auto Comparator = [](ReductionCandidateGroup &L, ReductionCandidateGroup &R)
    {
        if (L.getGEPCount() == R.getGEPCount())
            return L.getScore().RegisterPressure < R.getScore().RegisterPressure;

        return R.getGEPCount() < L.getGEPCount();
    };
    std::sort(Candidates.begin(), Candidates.end(), Comparator);

    // After sorting candidates, iterate over them and assign preferred reduction method,
    // keeping register pressure under threshold.
    unsigned CurrentPressure = RPE.getMaxRegisterPressure(L.getLoopPreheader());
    unsigned NewPressure = CurrentPressure;

    for (auto It = Candidates.begin(); It != Candidates.end(); ++It)
    {
        if (IGC_IS_FLAG_ENABLED(EnableGEPLSRToPreheader) && It->getScore().ReducedInstructions > 0)
        {
            // It is beneficial to reduce to preheader, but keep register pressure in check.
            if (NewPressure + It->getScore().RegisterPressure < MaxPressure)
            {
                NewPressure += It->getScore().RegisterPressure;
            }
            else
            {
                // Above threshold, just simplify index calculation inside loop.
                It->setReductionType(REDUCE_INDEX_ONLY);
            }
        }
        else
        {
            // If nothing from the loop would be reduced to preheader, only
            // simplify index calculation inside loop.
            It->setReductionType(REDUCE_INDEX_ONLY);
        }
    }

    auto CleanupFilter = [&](ReductionCandidateGroup& C)
    {
        // Check if nothing would be reduced/simplified.
        return C.getReductionType() == REDUCE_INDEX_ONLY && C.getGEPCount() == 1;
    };
    Candidates.erase(std::remove_if(Candidates.begin(), Candidates.end(), CleanupFilter), Candidates.end());

    LLVM_DEBUG(
        if (Candidates.empty())
        {
            dbgs() << "  dropped all candidates from loop " << L.getName() << "\n";
        }
        else
        {
            dbgs() << "  selected " << Candidates.size() << " candidates to reduce in loop " << L.getName()
                   << "; estimated register pressure before reduction=" << CurrentPressure << "; after reduction=" << NewPressure << "\n";
        }
    );
}


// If SCEV is zext/sext, drop extend.
const SCEV *SCEVHelper::dropExt(const SCEV *S)
{
    do
    {
        if (auto *Zext = dyn_cast<SCEVZeroExtendExpr>(S))
            S = Zext->getOperand();
        else if (auto *Sext = dyn_cast<SCEVSignExtendExpr>(S))
            S = Sext->getOperand();
        else
            break;
    } while (true);

    return S;
}


SCEVHelper::SCEVAddBuilder &SCEVHelper::SCEVAddBuilder::add(const SCEV *S, bool Negative)
{
    IGC_ASSERT(S->getType()->isIntegerTy());

    // strip extend
    S = SCEVHelper::dropExt(S);

    if (auto *Expr = dyn_cast<SCEVAddExpr>(S))
    {
        for (auto *Op : Expr->operands())
            add(Op, Negative);
        return *this;
    }

    // ScalarEvolution::getAddExpr requires all operands to have the same
    // type. Extend type if required.
    S = S->getType() == T ? S : SE.getSignExtendExpr(S, T);

    // Change expresion to "-1 * expression"
    S = Negative ? SE.getNegativeSCEV(S) : S;

    Ops.push_back(S);

    return *this;
}


GEPLoopStrengthReduction::GEPLoopStrengthReduction(bool AllowLICM)
    : FunctionPass(ID), AllowLICM(AllowLICM)
{
    initializeGEPLoopStrengthReductionPass(*PassRegistry::getPassRegistry());
}


void GEPLoopStrengthReduction::getAnalysisUsage(llvm::AnalysisUsage &AU) const
{
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<RegisterPressureEstimate>();
    AU.addRequired<llvm::ScalarEvolutionWrapperPass>();
}


bool GEPLoopStrengthReduction::runOnFunction(llvm::Function &F)
{
    if (F.hasOptNone())
        return false;

    LLVM_DEBUG(dbgs()
        << "  Running on function " << F.getName()
        << " options: MaxPressureRatio=" << IGC_GET_FLAG_VALUE(GEPLSRThresholdRatio)
        << " EnableLsrToPreheader=" << IGC_GET_FLAG_VALUE(EnableGEPLSRToPreheader) << "\n");

    auto &CGC = *getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    auto &DL = F.getParent()->getDataLayout();
    auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    auto &MMD = *getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    auto &RPE = getAnalysis<RegisterPressureEstimate>();
    auto &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();

    if (!RPE.isAvailable())
        return false;

    unsigned MaxPressure = static_cast<unsigned>(
        CGC.getNumGRFPerThread() / 128.0f * CGC.platform.getGRFSize() / 32.0f * IGC_GET_FLAG_VALUE(GEPLSRThresholdRatio));

    bool changed = false;

    for (Loop *L : LI.getLoopsInPreorder())
    {
        changed |= Reducer(DL, DT, *L, LI, MMD, RPE, SE, MaxPressure, AllowLICM).reduce();
    }

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
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(RegisterPressureEstimate)
IGC_INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
IGC_INITIALIZE_PASS_END(GEPLoopStrengthReduction, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char GEPLoopStrengthReduction::ID = 0;

FunctionPass *IGC::createGEPLoopStrengthReductionPass(bool AllowLICM)
{
    return new GEPLoopStrengthReduction(AllowLICM);
}