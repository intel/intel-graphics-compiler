/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/LoopCountAnalysis.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "common/igc_regkeys.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/IVDescriptors.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolutionAliasAnalysis.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstrTypes.h>
#include "common/LLVMWarningsPop.hpp"

#include <algorithm>
#include <optional>

using namespace llvm;
using namespace IGC;
using IGC::CollectLoopCount;

namespace
{
    class LoopCountAnalysis : public FunctionPass
    {
    public:
        static char ID; // Pass identification, replacement for typeid
        LoopCountAnalysis();

        struct LoopBoundInfo {
            Value* m_initVal;
            Value* m_stepVal;
            Value* m_finalVal;
            bool signedCmp;
            bool decreasingIdx;

            LoopBoundInfo(Value* I, Value* S, Value* F, bool sign, bool dec)
                : m_initVal(I), m_stepVal(S), m_finalVal(F), signedCmp(sign), decreasingIdx(dec) {}
        };

        StringRef getPassName() const override { return "LoopCount"; }
        bool runOnFunction(Function& F) override;
        void getAnalysisUsage(AnalysisUsage& AU) const override
        {
            AU.setPreservesAll();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<LoopInfoWrapperPass>();
            AU.addRequired<ScalarEvolutionWrapperPass>();
            AU.addRequired<CollectLoopCount>();
        }
        ScalarEvolution* SE;
        const DataLayout* dl;

    private:
        // Loops are numbered from 0 and up, in program order.
        DenseMap<Loop*, int> LoopNum;
        // L::getBounds() does not work after breakCriticalEdges due to
        // a dummy block as latch block.
        // The following member functions are copied from llvm::Loop member
        // functions with minor changes to handle this dummy block.
        ICmpInst* getLatchCmpInst(Loop* L) const;
        PHINode* getInductionVariable(Loop* L, ScalarEvolution& SE, bool& signedCM, bool& decCompare) const;
        Value* findFinalIVValue(Loop& L, PHINode& IndVar, Instruction& StepInst) const;
        std::optional<LoopBoundInfo> getBounds(Loop* L, ScalarEvolution& SE);
        void processLoop(Loop* L);

        CollectLoopCount* collectCount;
    };
}

// Register pass to igc-opt
#define PASS_FLAG "igc-collectLoopCount"
#define PASS_DESCRIPTION "Collect loop count."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(CollectLoopCount, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(CollectLoopCount, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char CollectLoopCount::ID = 0;

CollectLoopCount::CollectLoopCount() : ImmutablePass(ID) {
    initializeCollectLoopCountPass(*PassRegistry::getPassRegistry());
}

FunctionPass* IGC::createLoopCountAnalysisPass()
{
    return new LoopCountAnalysis();
}

#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS
#define PASS_FLAG "igc-loopcount"
#define PASS_DESCRIPTION "Loop count analysis."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(LoopCountAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(CollectLoopCount)
IGC_INITIALIZE_PASS_END(LoopCountAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char LoopCountAnalysis::ID = 0;

LoopCountAnalysis::LoopCountAnalysis() : FunctionPass(ID)
{
    initializeLoopCountAnalysisPass(*PassRegistry::getPassRegistry());
}

bool LoopCountAnalysis::runOnFunction(Function& F)
{
    LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
    collectCount = &getAnalysis<CollectLoopCount>();
    Module* M = F.getParent();
    dl = &(M->getDataLayout());

    if (LI.empty())
    {
        return false;
    }
    int numLoops = 0;

    for (BasicBlock& BB : F) {
        Loop* L = LI.getLoopFor(&BB);
        if (L && L->getHeader() == &BB) {
            SmallVector<Loop*, 4> loops;
            do {
                loops.push_back(L);
                L = L->getParentLoop();
            } while (L && L->getHeader() == &BB);

            for (int i = (int)loops.size() - 1; i >= 0; i--)
            {
                processLoop(loops[i]);
                numLoops++;
            }
        }
    }
    if (IGC_IS_FLAG_ENABLED(EnableKernelCostDebug)) {
        dbgs() << "Total number of loops: " << numLoops << "\n";
    }
    return false;
}

//add LCE to loopCountExpression
void CollectLoopCount::addLCE(int argNo, int byteOffset, int sizeInBytes, bool isInDirect, float factor, float C) {
    ArgSym argSym(argNo, byteOffset, sizeInBytes, isInDirect);
    //Check if argument symbol already exists
    //if it doesn't exist push argSym to loopArgs
    auto argSymIt = std::find(loopArgs.begin(), loopArgs.end(), argSym);
    int argSymIndex = -1;
    if (argSymIt != loopArgs.end()) {
        argSymIndex = std::distance(loopArgs.begin(), argSymIt);
    }
    else {
        if (argNo != -1) {
            loopArgs.push_back(argSym);
            argSymIndex = loopArgs.size() - 1;
        }
    }
    LCE lce(factor, argSymIndex, C);
    loopCountExpressions.push_back(lce);
}

std::vector<CollectLoopCount::LCE>& CollectLoopCount::getLCE() {
    return loopCountExpressions;
}

std::vector<CollectLoopCount::ArgSym>& CollectLoopCount::getloopArgs() {
    return loopArgs;
}

// mimic Loop::getBounds() and others
//   The difference is that these function will handle the dummy latch BB that has
//   unconditional branch. This dummy latch BB is created by BreakCriticalEdges pass.

/// Get the latch condition instruction.
ICmpInst* LoopCountAnalysis::getLatchCmpInst(Loop* L) const
{
    BasicBlock* Latch = L->getLoopLatch();
    if (!Latch)
        return nullptr;

    if (BranchInst* BI = dyn_cast_or_null<BranchInst>(Latch->getTerminator())) {
        if (BI->isConditional()) {
            return dyn_cast<ICmpInst>(BI->getCondition());
        }
        else if (BasicBlock* PreBB = Latch->getUniquePredecessor()) {
            if (BranchInst* br = dyn_cast_or_null<BranchInst>(PreBB->getTerminator())) {
                if (br->isConditional()) {
                    if (ICmpInst* icmpInst = dyn_cast<ICmpInst>(br->getCondition())) {
                        return icmpInst;
                    }
                    //return dyn_cast<ICmpInst>(br->getCondition());
                }
            }
        }
    }
    return nullptr;
}

PHINode* LoopCountAnalysis::getInductionVariable(Loop* L, ScalarEvolution& SE, bool& signedComp, bool& decComp) const
{
    if (!L->isLoopSimplifyForm())
        return nullptr;

    BasicBlock* Header = L->getHeader();
    assert(Header && "Expected a valid loop header");
    BasicBlock* Latch = L->getLoopLatch();
    if (!Latch)
        return nullptr;

    // If Latch is a dummy latch, get the real one
    ICmpInst* CmpInst = getLatchCmpInst(L);
    if (!CmpInst)
        return nullptr;

    Value* LatchCmpOp0 = CmpInst->getOperand(0);
    Value* LatchCmpOp1 = CmpInst->getOperand(1);

    signedComp = CmpInst->isSigned();

    if (CmpInst->getPredicate() == ICmpInst::ICMP_SGT || CmpInst->getPredicate() == ICmpInst::ICMP_UGT
        || CmpInst->getPredicate() == ICmpInst::ICMP_SGE || CmpInst->getPredicate() == ICmpInst::ICMP_UGE) {
        decComp = true;
    }

    //Operands that are extended from i32 to i64
    if (ZExtInst* zextInst = dyn_cast<ZExtInst>(LatchCmpOp0)) {
        LatchCmpOp0 = zextInst->getOperand(0);
    }

    for (PHINode& IndVar : Header->phis()) {
        InductionDescriptor IndDesc;
        if (!InductionDescriptor::isInductionPHI(&IndVar, L, &SE, IndDesc))
            continue;

        Value* StepInst = IndVar.getIncomingValueForBlock(Latch);

        // case 1:
        // IndVar = phi[{InitialValue, preheader}, {StepInst, latch}]
        // StepInst = IndVar + step
        // cmp = StepInst < FinalValue
        if (StepInst == LatchCmpOp0 || StepInst == LatchCmpOp1)
            return &IndVar;

        // case 2:
        // IndVar = phi[{InitialValue, preheader}, {StepInst, latch}]
        // StepInst = IndVar + step
        // cmp = IndVar < FinalValue
        if (&IndVar == LatchCmpOp0 || &IndVar == LatchCmpOp1)
            return &IndVar;
    }

    return nullptr;
}

/// Return the final value of the loop induction variable if found.
Value* LoopCountAnalysis::findFinalIVValue(Loop& L, PHINode& IndVar,
    Instruction& StepInst) const
{
    ICmpInst* LatchCmpInst = getLatchCmpInst(&L);
    if (!LatchCmpInst)
        return nullptr;

    Value* Op0 = LatchCmpInst->getOperand(0);
    Value* Op1 = LatchCmpInst->getOperand(1);

    //Operands that are extended from i32 to i64
    if (ZExtInst* zextInst = dyn_cast<ZExtInst>(Op0)) {
        Op0 = zextInst->getOperand(0);
    }
    if (Op0 == &IndVar || Op0 == &StepInst)
        return Op1;

    if (Op1 == &IndVar || Op1 == &StepInst)
        return Op0;

    return nullptr;
}

std::optional<LoopCountAnalysis::LoopBoundInfo> LoopCountAnalysis::getBounds(
    Loop* L, ScalarEvolution& SE)
{
    bool signedComp = false;
    bool decComp = false;
    if (PHINode* IndVar = getInductionVariable(L, SE, signedComp, decComp)) {
        InductionDescriptor IndDesc;
        if (!InductionDescriptor::isInductionPHI(IndVar, L, &SE, IndDesc))
            return std::nullopt;

        Value* InitialIVValue = IndDesc.getStartValue();
        Instruction* StepInst = IndDesc.getInductionBinOp();
        if (!InitialIVValue || !StepInst)
            return std::nullopt;

        const SCEV* Step = IndDesc.getStep();
        Value* StepInstOp1 = StepInst->getOperand(1);
        Value* StepInstOp0 = StepInst->getOperand(0);
        Value* StepValue = nullptr;
        if (SE.getSCEV(StepInstOp1) == Step)
            StepValue = StepInstOp1;
        else if (SE.getSCEV(StepInstOp0) == Step)
            StepValue = StepInstOp0;

        Value* FinalIVValue = findFinalIVValue(*L, *IndVar, *StepInst);
        if (!FinalIVValue)
            return std::nullopt;

        return LoopBoundInfo(InitialIVValue, StepValue, FinalIVValue, signedComp, decComp);
    }
    return std::nullopt;
}

//return argument from loop end value
const Argument* getArgumentFromEndValue(Value* In, ConstantInt** addValue) {
    const Argument* arg = nullptr;
    //if loop end value is a load instruction
    if (const LoadInst* loadinst = dyn_cast<LoadInst>(In)) {
        arg = dyn_cast<Argument>(loadinst->getOperand(0));
        if (!arg) {
            if (IntToPtrInst* I2P = dyn_cast<IntToPtrInst>(loadinst->getOperand(0))) {
                if (Instruction* addI = dyn_cast<Instruction>(I2P->getOperand(0))) {
                    if (PtrToIntInst* P2I = dyn_cast<PtrToIntInst>(addI->getOperand(0)))
                    {
                        if ((*addValue = dyn_cast<ConstantInt>(addI->getOperand(1)))) {
                            arg = dyn_cast<Argument>(P2I->getOperand(0));
                        }
                    }
                }
            }
        }
    }
    else
    {
        arg = dyn_cast<Argument>(In);
    }
    return arg;
}

//update sizeInBytes and isInDirect value for ArgSym
void updateArgSym(const Argument* arg, ConstantInt* addValue, int& argumentIndex, int& byteOffset, int& sizeInBytes, bool& isInDirect, const DataLayout& dl) {
    argumentIndex = arg->getArgNo();
    if (arg->getType()->isIntegerTy()) {
        unsigned bitWidth = arg->getType()->getIntegerBitWidth();
        sizeInBytes = bitWidth / 8;
    }
    else if (arg->getType()->isPointerTy()) {
        if (addValue) {
            byteOffset = addValue->getSExtValue();
        }
        sizeInBytes = dl.getTypeSizeInBits(arg->getType()) / 8;
        isInDirect = true;
    }
}

//get initial value for loop and update C for LCE
void updateLCEfactor(ConstantInt* stepInt, float& factor, LoopCountAnalysis::LoopBoundInfo* LB) {
    //Find Factor for LCE
    factor = 1;

    int stepCount = 1;
    if (LB->signedCmp) {
        stepCount = stepInt->getSExtValue();
    }
    else {
        stepCount = stepInt->getZExtValue();
    }
    if (stepCount > 1) {
        factor = (float)factor / (float)stepCount;
    }
    else if (stepCount < 0) {
        factor = (float)factor / (-1 * (float)stepCount);
    }

}

//get initial value for loop and update C for LCE
void updateLCEConstant(ConstantInt* initialValue, float& C, float factor, LoopCountAnalysis::LoopBoundInfo* LB) {

    int initial = 0;
    if (initialValue->getBitWidth() <= 64) {
        if (LB->signedCmp) {
            initial = initialValue->getSExtValue();
        }
        else
        {
            initial = initialValue->getZExtValue();
        }
        if (initial) {
            C = -1 * ((float)(initial)*factor);
        }
    }
}

void getMultiplicationFactorFromValue(Value* val, Value** shlOperand, uint64_t& multiplicationFactor) {
    if (ZExtInst* zextInst = dyn_cast<ZExtInst>(val)) {
        val = zextInst->getOperand(0);
    }
    if (auto* shlInst = dyn_cast<BinaryOperator>(val)) {
        *shlOperand = shlInst->getOperand(0);
        Value* shlfactor = shlInst->getOperand(1);
        if (auto* constInt = dyn_cast<ConstantInt>(shlfactor)) {
            uint64_t shiftValue = constInt->getZExtValue();
            multiplicationFactor = 1ULL << shiftValue;
        }
        else {
            *shlOperand = nullptr;
        }
    }
}
// loop bounds with constant value. End value and step bounded by the same variable
// for (size_t j = 0; j < sgSize * 16; j += sgSize)
//
// Return true if it finds constant count; return false otherwise.
bool getConstantBoundLCE(Value* In, Value* step, float& C) {
    Value* shlEndOperand = nullptr;
    Value* shlStepOperand = nullptr;
    uint64_t endmultiplicationFactor = 0;
    uint64_t stepmultiplicationFactor = 1;

    getMultiplicationFactorFromValue(In, &shlEndOperand, endmultiplicationFactor);
    getMultiplicationFactorFromValue(step, &shlStepOperand, stepmultiplicationFactor);

    if (shlEndOperand) {
        //end = shl nsw i32 %endShl, 4
        //step = shl nsw i32 %stepShl, 2
        //check if end shift operand and step shift operand are the same
        if (shlStepOperand && shlEndOperand == shlStepOperand) {
            C = (float)endmultiplicationFactor / (float)stepmultiplicationFactor;
            return true;
        }
        //end = shl nuw nsw i32 %endShl, 4
        //step = phi instruction
        //Check if endOperand and step are the same
        if (shlEndOperand == step) {
            C = float(endmultiplicationFactor);
            return true;
        }
        //end = zext i32 %endZext to i64
        // %endZext = shl nsw i32 %endShl, 4
        //step = sext i32 %stepSext to i64
        //check if %endShl and %stepSext have the same operand
        else if (SExtInst* sextStepInst = dyn_cast<SExtInst>(step)) {
            Value* sextOperand = sextStepInst->getOperand(0);
            if (shlEndOperand == sextOperand) {
                C = float(endmultiplicationFactor);
                return true;
            }
        }
    }
    return false;
}

bool checkLegalArgument(const Argument* arg) {
    return arg && (arg->getType()->isIntegerTy() || arg->getType()->isPointerTy());
}

void LoopCountAnalysis::processLoop(Loop* L) {
    raw_ostream& output = llvm::outs();
    std::optional<LoopBoundInfo> bounds = getBounds(L, *SE);
    //Values used for Nested loop analysis

    if (IGC_IS_FLAG_ENABLED(EnableKernelCostDebug)) {
        dbgs() << "Loop at depth " << L->getLoopDepth() << " with header " << L->getHeader()->getName() << "\n";
    }

    if (bounds.has_value()) {
        LoopBoundInfo* LB = &bounds.value();

        //Values for ArgSym
        //Argumnent Index will be -1 for loops with untracable argument
        int argumentIndex = -1;
        int byteOffset = 0;
        int sizeInBytes = 4;
        bool isInDirect = false;
        //Values for LCE
        float factor = 0;
        float C = 0;

        Value* I0 = LB->m_initVal;
        Value* In = LB->m_finalVal;
        Value* step = LB->m_stepVal;

        if (LB->decreasingIdx) {
            I0 = LB->m_finalVal;
            In = LB->m_initVal;
        }

        if (IGC_IS_FLAG_ENABLED(EnableKernelCostDebug)) {
            dbgs() << " init value: ";
            I0->print(output);
            dbgs() << "\n";
            dbgs() << " end value: ";
            In->print(output);
            dbgs() << "\n";
            dbgs() << " step value: ";
            step->print(output);
            dbgs() << "\n";
        }

        ConstantInt* addValue = nullptr;
        ConstantInt* initialValue = dyn_cast<ConstantInt>(I0);
        ConstantInt* stepInt = dyn_cast<ConstantInt>(step);
        //get argument for end value if argument is tracable to argument list and update addValue if argument is pointer
        const Argument* arg = getArgumentFromEndValue(In, &addValue);

        bool hasCount = true;
        //Check if argument is legal and initial value is constant
        if (checkLegalArgument(arg) && initialValue && stepInt) {
            //update argument symbol based on argument and addvalue
            updateArgSym(arg, addValue, argumentIndex, byteOffset, sizeInBytes, isInDirect, *dl);
            //update LCE factor based on step value
            updateLCEfactor(stepInt, factor, LB);
            //update LCE constant based on factor and initial value
            updateLCEConstant(initialValue, C, factor, LB);
            if (IGC_IS_FLAG_ENABLED(EnableKernelCostDebug)) {
                dbgs() << "Argument symbol found with index " << argumentIndex << " isInDirect = " << isInDirect << "\n";
            }
        }
        else {
            //If loop bound is a constant
            //loop bounds with constant value. End value and step bounded by the same variable
            //for (size_t j = 0; j < sgSize * 16; j += sgSize)
            hasCount = getConstantBoundLCE(In, step, C);
        }
        if (hasCount) {
            collectCount->addLCE(argumentIndex, byteOffset, sizeInBytes,
                                    isInDirect, factor, C);
        } else {
            collectCount->addLCE(-1, 0, 0, false, 1, 0);
        }

    }
}
