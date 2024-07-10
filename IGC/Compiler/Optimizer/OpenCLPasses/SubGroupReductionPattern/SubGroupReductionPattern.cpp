/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SubGroupReductionPattern.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvmWrapper/IR/PatternMatch.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/MetaDataUtilsWrapper.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

using namespace llvm;
using namespace llvm::PatternMatch;
using namespace IGC;
using namespace IGCMD;

// Pattern of instructions:
//   %lane8 = call i32 <shuffle-op>(i32 %input, i32 8)
//   %value8 = <alu-op> i32 %input, %lane8
//   %lane4 = call i32 <shuffle-op>(i32 %value8, i32 4)
//   %value4 = <alu-op> i32 %value8, %lane4
//   %lane2 = call i32 <shuffle-op>(i32 %value4, i32 2)
//   %value2 = <alu-op> i32 %value4, %lane2
//   %lane1 = call i32 <shuffle-op>(i32 %value2, i32 1)
//   %result = <alu-op> i32 %value2, %lane1
//
// Where shuffle-op is a builtin returning a value from a different lane in the subgroup
// (like simdShuffleXor or WaveShuffleIndex).
class ShufflePattern
{
public:

    struct PatternStep
    {
        PatternStep(GenIntrinsicInst *ShuffleOp, Instruction *Op, uint64_t Lane)
            : ShuffleOp(ShuffleOp), Op(Op), Lane(Lane) { }

        GenIntrinsicInst *ShuffleOp;
        Instruction *Op;
        uint64_t Lane;
    };

    ShufflePattern(GenIntrinsicInst *ShuffleOp, Instruction *Op, WaveOps OpType, uint64_t Lane)
        : OpType(OpType)
    {
        Steps.emplace_back(ShuffleOp, Op, Lane);
    }

    bool append(GenIntrinsicInst *ShuffleOp, Instruction *Op, WaveOps OpType, uint64_t Lane);

    WaveOps OpType;
    SmallVector<PatternStep, 8> Steps;
};

// Pass for matching common manual subgroup reduction pattern and replacing them
// with corresponding GenISA.Wave* call.
class SubGroupReductionPattern : public llvm::FunctionPass, public llvm::InstVisitor<SubGroupReductionPattern>
{
public:
    static char ID; // Pass identification, replacement for typeid

    SubGroupReductionPattern();

    virtual llvm::StringRef getPassName() const override
    {
        return "SubGroupReductionPattern";
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override;

    virtual bool runOnFunction(Function &F) override;

    void visitCallInst(llvm::CallInst &C);

private:

    void visitSimdShuffleXor(GenIntrinsicInst &ShuffleOp);
    void visitWaveShuffleIndex(GenIntrinsicInst &ShuffleOp);

    void matchShufflePattern(GenIntrinsicInst &ShuffleOp, uint64_t Lane);

    bool reduce(ShufflePattern &Pattern);

    static WaveOps getWaveOp(Instruction* Op);

    int SubGroupSize = 0;
    bool Modified = false;

    SmallVector<ShufflePattern, 8> Matches;
};

SubGroupReductionPattern::SubGroupReductionPattern() : FunctionPass(ID)
{
    initializeSubGroupReductionPatternPass(*PassRegistry::getPassRegistry());
}

void SubGroupReductionPattern::getAnalysisUsage(llvm::AnalysisUsage &AU) const
{
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.setPreservesCFG();
}

bool SubGroupReductionPattern::runOnFunction(llvm::Function &F)
{
    if (F.hasOptNone())
        return false;

    auto MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto FII = MDU->findFunctionsInfoItem(&F);
    if (FII == MDU->end_FunctionsInfo())
        return false;

    auto SubGroupSizeMD = FII->second->getSubGroupSize();
    if (!SubGroupSizeMD->hasValue())
        return false;

    SubGroupSize = SubGroupSizeMD->getSIMDSize();

    Modified = false;
    Matches.clear();

    // Collect matches.
    visit(F);

    // Replace matches.
    for (auto &Match : Matches)
    {
        Modified |= reduce(Match);
    }

    return Modified;
}

void SubGroupReductionPattern::visitCallInst(llvm::CallInst &C)
{
    if (GenIntrinsicInst *I = llvm::dyn_cast<GenIntrinsicInst>(&C))
    {
        switch (I->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_simdShuffleXor:
            return visitSimdShuffleXor(*I);
        case GenISAIntrinsic::GenISA_WaveShuffleIndex:
            return visitWaveShuffleIndex(*I);
        default:
            return;
        }
    }
}

void SubGroupReductionPattern::visitSimdShuffleXor(GenIntrinsicInst &ShuffleOp)
{
    // Expected pattern:
    //   %simdShuffleXor = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %value, i32 8)
    //   %result = <op> i32 %value, %simdShuffleXor

    ConstantInt *Lane = nullptr;

    if (match(ShuffleOp.getOperand(1), m_ConstantInt(Lane)))
        matchShufflePattern(ShuffleOp, Lane->getZExtValue());
}

void SubGroupReductionPattern::visitWaveShuffleIndex(GenIntrinsicInst &ShuffleOp)
{
    // Expected pattern:
    //   %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
    //   %0 = xor i16 %simdLaneId, 16
    //   %1 = zext i16 %xor16 to i32
    //   %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %value, i32 %1, i32 0)
    //   %result = <op> i32 %value, %simdShuffle

    ConstantInt *HelperLanes = dyn_cast<ConstantInt>(ShuffleOp.getOperand(2));
    if (!HelperLanes || HelperLanes->getZExtValue() != 0)
        return;

    Value *SimdLaneId = nullptr;
    ConstantInt *Lane = nullptr;

    if (match(ShuffleOp.getOperand(1), m_ZExt(m_c_Xor(m_Value(SimdLaneId), m_ConstantInt(Lane)))) ||
        match(ShuffleOp.getOperand(1), m_c_Xor(m_ZExt(m_Value(SimdLaneId)), m_ConstantInt(Lane))))
    {
        if (GenIntrinsicInst *I = dyn_cast<GenIntrinsicInst>(SimdLaneId))
        {
            if (I->getIntrinsicID() == GenISAIntrinsic::GenISA_simdLaneId)
                matchShufflePattern(ShuffleOp, Lane->getZExtValue());
        }
    }
}

void SubGroupReductionPattern::matchShufflePattern(GenIntrinsicInst &ShuffleOp, uint64_t Lane)
{
    if (Lane != 1 && Lane % 2 != 0)
        return;

    if (!ShuffleOp.hasOneUse())
        return;

    Instruction *Op = ShuffleOp.user_back();
    if (!Op)
        return;

    WaveOps OpType = getWaveOp(Op);
    if (OpType == WaveOps::UNDEF)
        return;

    Value* Other = ShuffleOp.getOperand(0);
    if (((Op->getOperand(0) == Other && Op->getOperand(1) == &ShuffleOp) || (Op->getOperand(0) == &ShuffleOp && Op->getOperand(1) == Other)) == false)
        return;

    // Continues previous pattern?
    for (auto &Match : Matches)
    {
        if (Match.append(&ShuffleOp, Op, OpType, Lane))
            return;
    }

    // New pattern.
    Matches.emplace_back(&ShuffleOp, Op, OpType, Lane);
}

bool ShufflePattern::append(GenIntrinsicInst *ShuffleOp, Instruction *Op, WaveOps OpType, uint64_t Lane)
{
    if (this->OpType != OpType)
        return false;

    Instruction *PreviousValue = Steps.back().Op;

    if (PreviousValue->getNumUses() != 2)
        return false;

    if (ShuffleOp->getOperand(0) != PreviousValue)
        return false;

    if (Op->getOperand(0) != PreviousValue && Op->getOperand(1) != PreviousValue)
        return false;

    Steps.emplace_back(ShuffleOp, Op, Lane);
    return true;
}

bool SubGroupReductionPattern::reduce(ShufflePattern &Pattern)
{
    // Check XORs pattern.
    // Full reduction for SIMD16: 8, 4, 2, 1

    uint64_t XorMask = 0;
    for (auto &Step : Pattern.Steps)
    {
        if (XorMask & Step.Lane)
            return false; // Each xor must be unique
        XorMask |= Step.Lane;
    }

    if (XorMask != (SubGroupSize - 1))
        return false;

    // Can do full reduction.
    auto &FirstStep = Pattern.Steps.front();

    IRBuilder<> IRB(FirstStep.ShuffleOp);
    IRB.SetCurrentDebugLocation(FirstStep.ShuffleOp->getDebugLoc());

    Value *Args[3] = { FirstStep.ShuffleOp->getOperand(0), IRB.getInt8((uint8_t) Pattern.OpType), IRB.getInt32(0) };
    Function *WaveAll = GenISAIntrinsic::getDeclaration(FirstStep.ShuffleOp->getCalledFunction()->getParent(),
        GenISAIntrinsic::GenISA_WaveAll,
        Args[0]->getType());

    auto &LastStep = Pattern.Steps.back();
    LastStep.Op->replaceAllUsesWith(IRB.CreateCall(WaveAll, Args));
    RecursivelyDeleteTriviallyDeadInstructions(LastStep.Op);

    return true;
}

WaveOps SubGroupReductionPattern::getWaveOp(Instruction *Op)
{

    if (IntrinsicInst *I = llvm::dyn_cast<IntrinsicInst>(Op))
    {
        switch (I->getIntrinsicID())
        {
#if LLVM_VERSION_MAJOR >= 12
        case Intrinsic::umin:
            return WaveOps::UMIN;
        case Intrinsic::umax:
            return WaveOps::UMAX;
        case Intrinsic::smin:
            return WaveOps::IMIN;
        case Intrinsic::smax:
            return WaveOps::IMAX;
#endif
        case Intrinsic::minnum:
            return WaveOps::FMIN;
        case Intrinsic::maxnum:
            return WaveOps::FMAX;
        default:
            return WaveOps::UNDEF;
        }
    }

    if (!isa< BinaryOperator>(Op))
        return WaveOps::UNDEF;

    switch (Op->getOpcode())
    {
    case Instruction::Add:
        return WaveOps::SUM;
    case Instruction::Mul:
        return WaveOps::PROD;
    case Instruction::Or:
        return WaveOps::OR;
    case Instruction::Xor:
        return WaveOps::XOR;
    case Instruction::And:
        return WaveOps::AND;
    case Instruction::FAdd:
        return WaveOps::FSUM;
    case Instruction::FMul:
        return WaveOps::FPROD;
    default:
        return WaveOps::UNDEF;
    }
}

// Register pass to igc-opt
#define PASS_FLAG "igc-subgroup-reduction-pattern"
#define PASS_DESCRIPTION "Matches common patterns for subgroup reductions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SubGroupReductionPattern, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(SubGroupReductionPattern, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char SubGroupReductionPattern::ID = 0;

FunctionPass* IGC::createSubGroupReductionPatternPass()
{
    return new SubGroupReductionPattern();
}
