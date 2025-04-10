/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This pass attempts to reduce number of times FP rounding mode is switched by
// moving and grouping together instructions using the same rounding mode.
// This pass works after optimizations, before emitter. Instructions are
// reordered only in the same basic block, with optional distance limit for move.

#include "Compiler/CISACodeGen/FPRoundingModeCoalescing.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/igc_regkeys.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

#define DEBUG_TYPE "fp-rounding-mode-coalescing"

// Represents collection of FP instructions with the same RM. RM is not switched
// between the first and the last instruction in the group.
class FPRoundingModeGroup {
  public:
    FPRoundingModeGroup(Instruction *I, ERoundingMode RM) : Head(I), Tail(I), RM(RM) { Instructions.insert(I); }

    ERoundingMode getRoundingMode() const { return RM; }

    Instruction *getHead() const { return Head; }

    bool contains(Instruction* I) const;
    bool uses(Instruction* I) const;

    void setHead(Instruction *I);
    void insertAfterGroup(Instruction *I);
    bool comesBeforeTail(Instruction *I);

  private:
    // First custom RM FP instruction in the group.
    Instruction *Head = nullptr;

    // Last custom RM FP instruction. If instruction to insert to group has
    // uses that are FP instructions in different RM mode than this
    // group, these instructions would be moved here.
    //
    // Example:
    //   %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %0, double %1, double %2) ; <- To insert
    //   %tmp = fadd double %0, %fma.rtz.1.result                                                                   ; <- instruction in different RM
    //   %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %3, double %4, double %5) ; <- HEAD
    //   %fma.rtz.3.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %6, double %7, double %8) ; <- TAIL
    //
    // Because %tmp depends on the result of instruction to move, the FP instructions would be reordered to:
    //
    //   %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %0, double %1, double %2) ; <- new HEAD
    //   %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %3, double %4, double %5) ;
    //   %fma.rtz.3.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %6, double %7, double %8) ; <- TAIL
    //   %tmp = fadd double %0, %fma.rtz.1.result
    //
    // Because only FP instructions are expected to be moved, there should be no impact on latency on FP pipe.
    Instruction *Tail = nullptr;

    SmallPtrSet<Instruction *, 4> Instructions;

    const ERoundingMode RM;
};

// Attempts to reorder FP instructions in one basic block.
class FPRoundingModeCoalescingImpl {
  public:
    FPRoundingModeCoalescingImpl(ModuleMetaData *MMD, BasicBlock &BB) : MMD(MMD), BB(BB) {}

    bool coalesce();

  private:
    bool update();

    bool setsRoundingMode(Instruction &ToMove);

    bool checkMoveThreshold(Instruction &ToMove, Instruction *InsertPoint);

    bool tryMove(Instruction &ToMove);

    bool tryMove(Instruction &ToMove, FPRoundingModeGroup &InsertPoint);

    ModuleMetaData *const MMD;
    BasicBlock &BB;

    SmallVector<FPRoundingModeGroup, 8> Groups;

    SmallPtrSet<Instruction *, 32> VisitedInstructions;
};

class FPRoundingModeCoalescing : public llvm::FunctionPass {
  public:
    static char ID;

    FPRoundingModeCoalescing();

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
        AU.setPreservesCFG();
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addPreserved<MetaDataUtilsWrapper>();
    }

    bool runOnFunction(llvm::Function &F) override;

    virtual llvm::StringRef getPassName() const override { return "FPRoundingModeCoalescing"; }
};

// Utility function to move instruction + log.
void moveAfter(Instruction* I, Instruction* InsertPoint) {
    LLVM_DEBUG(dbgs() << "Moving ["; I->print(dbgs(), true); dbgs() << "] after ["; InsertPoint->print(dbgs(), true); dbgs() << "]\n");
    I->moveAfter(InsertPoint);
}

bool FPRoundingModeGroup::contains(Instruction *I) const {
    return Instructions.count(I);
}

bool FPRoundingModeGroup::uses(Instruction *I) const {
    for (auto V = I->users().begin(); V != I->users().end(); ++V) {
        if (auto *User = dyn_cast<Instruction>(*V)) {
            if (this->contains(User))
                return true;
        }
    }
    return false;
}

// Sets instruction as new head of the group, without moving it.
void FPRoundingModeGroup::setHead(Instruction *I) {
    LLVM_DEBUG(dbgs() << "Setting ["; I->print(dbgs(), true); dbgs() << "] as new head before ["; Head->print(dbgs(), true); dbgs() << "]\n");
    Head = I;
    Instructions.insert(I);
}

// Moves given instruction after last instruction in group. Does not expand group.
void FPRoundingModeGroup::insertAfterGroup(Instruction *I) {
    LLVM_DEBUG(
        dbgs() << "Moving [";
        I->print(dbgs(), true);
        dbgs() << "] after tail [";
        Tail->print(dbgs(), true);
        dbgs() << "] (head is [";
        Head->print(dbgs(), true);
        dbgs() << "])\n";
    );
    I->moveAfter(Tail);
}

// Returns true if given instruction comes before group's tail.
bool FPRoundingModeGroup::comesBeforeTail(Instruction *I) {
    return IGCLLVM::comesBefore(I, Tail);
}

// Returns true if at least one change is made in basic block.
bool FPRoundingModeCoalescingImpl::coalesce() {
    bool result = false;

    // Repeat until nothing can be moved.
    while (true) {
        if (update()) {
            result = true;
        } else {
            break;
        }
    }

    return result;
}

bool FPRoundingModeCoalescingImpl::update() {

    for (auto I = BB.rbegin(); I != BB.rend(); ++I) {

        if (VisitedInstructions.insert(&*I).second == false)
            continue;

        if (!setsRoundingMode(*I))
            continue;

        ERoundingMode RM = GetRoundingMode_FP(MMD, &*I);

        // For safety reasons limit transformation to only one non-default RM.
        if (!Groups.empty() && Groups.begin()->getRoundingMode() != RM)
            continue;

        if (tryMove(*I))
            return true;

        // Instruction can't be moved, create new group.
        Groups.emplace_back(&*I, RM);
    }

    return false;
}

// Returns true if given instruction will change rounding mode.
bool FPRoundingModeCoalescingImpl::setsRoundingMode(Instruction &ToMove) {
    if (!setsRMExplicitly(&ToMove))
        return false;

    // Instruction sets explicit rounding mode. But this mode might already match default one
    // in kernel, so there might be no need to change it.

    ERoundingMode RM = GetRoundingMode_FP(MMD, &ToMove);
    ERoundingMode ConvRM = GetRoundingMode_FPCvtInt(MMD, &ToMove);

    return (RM != static_cast<ERoundingMode>(MMD->compOpt.FloatRoundingMode) && RM != ERoundingMode::ROUND_TO_ANY) ||
        (ConvRM != static_cast<ERoundingMode>(MMD->compOpt.FloatCvtIntRoundingMode) && ConvRM != ERoundingMode::ROUND_TO_ANY);
}

bool FPRoundingModeCoalescingImpl::checkMoveThreshold(Instruction &ToMove, Instruction *InsertPoint) {
    unsigned Dist = 1;
    for (Instruction *I = ToMove.getNextNonDebugInstruction(); I != InsertPoint; I = I->getNextNonDebugInstruction(), ++Dist) {
        if (Dist >= IGC_GET_FLAG_VALUE(FPRoundingModeCoalescingMaxDistance))
            return false;
    }

    return true;
}

// Returns true if instruction is moved.
bool FPRoundingModeCoalescingImpl::tryMove(Instruction &ToMove) {
    for (auto &InsertPoint : Groups) {
        if (tryMove(ToMove, InsertPoint))
            return true;
    }

    return false;
}

// Attempts to move given instruction on top of group of other FP instructions.
// Returns true if instruction is moved.
bool FPRoundingModeCoalescingImpl::tryMove(Instruction &ToMove, FPRoundingModeGroup &Group) {

    if (GetRoundingMode_FP(MMD, &ToMove) != Group.getRoundingMode())
        return false;

    // First collect all uses of instruction to move that are before group's tail.
    // In case instruction will have to be moved, uses will have to be moved too.
    // Keep uses sorted in order of occurence in LLVM IR.
    auto Cmp = [](Instruction *a, Instruction *b) { return IGCLLVM::comesBefore(a, b); };
    std::set<Instruction *, decltype(Cmp)> Users(Cmp);

    SmallVector<Instruction*, 8> worklist;
    SmallSet<Instruction*, 8> visited;
    worklist.push_back(&ToMove);
    while (!worklist.empty())
    {
        Instruction* ToCheck = worklist.back();
        worklist.pop_back();
        auto [_, inserted] = visited.insert(ToCheck);
        if (!inserted)
        {
            continue;
        }
        for (auto V = ToCheck->users().begin(); V != ToCheck->users().end(); ++V) {
            if (auto* I = dyn_cast<Instruction>(*V)) {
                if (I->getParent() != ToMove.getParent())
                    continue;
                if (isa<PHINode>(I))
                    continue;
                // Special case - if group directly uses ToMove, then this use is already on correct position.
                if (Group.comesBeforeTail(I) && (ToCheck != &ToMove || !Group.contains(I))) {
                    Users.insert(I);
                    worklist.push_back(I);
                }
            }
        }
    }

    // Now that all users are collected, count how many users would switch RM.
    int UsersSwitchingRM = 0;
    for (auto *I : Users) {
        if (!ignoresRoundingMode(I) && GetRoundingMode_FP(MMD, &ToMove) != GetRoundingMode_FP(MMD, I))
            ++UsersSwitchingRM;
    }

    // Next, find first instruction before group switching RM that is NOT an user
    // of instruction to move. This will be an insert point.
    Instruction *InsertPoint = nullptr;
    for (Instruction *I = Group.getHead()->getPrevNonDebugInstruction(); I != &ToMove; I = I->getPrevNonDebugInstruction()) {
        if (!ignoresRoundingMode(I) && Users.count(I) == 0) {
            InsertPoint = I;
            break;
        }
    }

    // If there is no insert point and no user switching rounding mode, then it means there is
    // no change in RM between instruction to move and group. There is need to move.
    if (InsertPoint == nullptr && UsersSwitchingRM == 0) {
        Group.setHead(&ToMove);
        return true;
    }

    // A move is needed. Before any move, check move threshold.
    if (!checkMoveThreshold(ToMove, InsertPoint ? InsertPoint : Group.getHead()))
        return false;

    // Check if there are any users not suitable for move.
    if (std::find_if(Users.begin(), Users.end(), [](Instruction *I) { return I->mayReadOrWriteMemory() || I->mayHaveSideEffects(); }) != Users.end())
        return false;

    // Reordering too much could have an impact on latency, so consider only two scenarios:
    // 1. If there are no uses depending on RM, then all uses will be moved after insert point, BEFORE group.
    //    Higher number of impacted instructions keep their original order.
    // 2. If at least one user depends on RM, in order to minimize number of times RM is switched, all instructions
    //    are moved AFTER group.

    if (UsersSwitchingRM > 0) {

        // Move all uses after group. First check if this is safe - instruction can't be moved
        // if it is already a part of the group or is used by group's member.
        if (std::find_if(Users.begin(), Users.end(), [&](Instruction *I) { return Group.contains(I) || Group.uses(I); }) != Users.end())
            return false;

        for (auto It = Users.rbegin(); It != Users.rend(); ++It)
            Group.insertAfterGroup(*It);

    } else {
        // All uses to move ignore RM. They can be moved after insert point, before group.
        // Better preserves original order of instructions.
        for (auto It = Users.rbegin(); It != Users.rend(); ++It) {
            if (IGCLLVM::comesBefore(*It, InsertPoint))
                moveAfter(*It, InsertPoint);
        }
    }

    if (InsertPoint)
        moveAfter(&ToMove, InsertPoint);

    Group.setHead(&ToMove);

    return true;
}

FPRoundingModeCoalescing::FPRoundingModeCoalescing() : FunctionPass(ID) { initializeFPRoundingModeCoalescingPass(*PassRegistry::getPassRegistry()); }

bool FPRoundingModeCoalescing::runOnFunction(Function &F) {
    LLVM_DEBUG(dbgs() << "Running on function " << F.getName() << ", FPRoundingModeCoalescingMaxDistance=" << IGC_GET_FLAG_VALUE(FPRoundingModeCoalescingMaxDistance) << "\n");

    auto *MMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

    bool result = false;

    for (auto &BB : F) {
        result |= FPRoundingModeCoalescingImpl(MMD, BB).coalesce();
    }

    return result;
}

char FPRoundingModeCoalescing::ID = 0;

#define PASS_FLAG "igc-fp-rounding-mode-coalescing"
#define PASS_DESCRIPTION "Groups FP instructions with common rounding mode"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(FPRoundingModeCoalescing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(FPRoundingModeCoalescing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

FunctionPass *IGC::createFPRoundingModeCoalescingPass() { return new FPRoundingModeCoalescing(); }
