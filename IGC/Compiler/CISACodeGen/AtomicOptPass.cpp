/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AtomicOptPass.hpp"
#include "CodeGenPublicEnums.h"
#include <llvm/IR/Function.h>

#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace llvm::PatternMatch;

char AtomicOptPass::ID = 0;

#define PASS_FLAG "opt-atomics-pass"
#define PASS_DESCRIPTION "Optimization of atomic instructions that write to the same memory"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(AtomicOptPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_END(AtomicOptPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

AtomicOptPass::AtomicOptPass() : FunctionPass(ID) {
    initializeAtomicOptPassPass(*PassRegistry::getPassRegistry());
}

//This function creates GenISA.WaveAll intrinsic before the Pos instruction.
//This function uses the arguments of an atomic operation.
Instruction *AtomicOptPass::createReduce(Instruction *Pos, Value *ValueForReduce) {
    IRBuilder<> Builder(Pos);

    Value *OpVal = Builder.getInt8((uint8_t)WaveOps::FSUM);
    Value *Args[3] = {ValueForReduce, OpVal, Builder.getInt32(0)};

    Function *WaveAll = GenISAIntrinsic::getDeclaration(M,
        GenISAIntrinsic::GenISA_WaveAll,
        ValueForReduce->getType());

    Value *WaveCall = Builder.CreateCall(WaveAll, Args);

    return cast<Instruction>(WaveCall);
}

//This function checks the atomic emulation pattern:
//
//%call1 = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* %1, i32 addrspace(1)* %1, i32 0, i32 9)
//%bc1 = bitcast i32 %call1 to float
//%operation = fadd float %bc1, -2.000000e+00
//%bc2 = bitcast float %operation to i32
//%call2 = call i32 @llvm.genx.GenISA.icmpxchgatomicrawA64.i32.p1i32.p1i32(i32 addrspace(1)* %1, i32 addrspace(1)* %1, i32 %call1, i32 %bc2)
//%cmp = icmp eq i32 %call1, %call2
//br i1 %cmp, label %exit, label %back
bool AtomicOptPass::checkFloatAtomicEmulation(Instruction *Inst, size_t &OperandPos) {
    GenIntrinsicInst *GInst = cast<GenIntrinsicInst>(Inst);

    if (GInst->getIntrinsicID() != GenISAIntrinsic::GenISA_intatomicrawA64)
        return false;

    ConstantInt *Op = dyn_cast<ConstantInt>(Inst->getOperand(3));
    ConstantInt *AtomVal = dyn_cast<ConstantInt>(Inst->getOperand(2));
    if (!Op || !AtomVal)
        return false;

    if ((Op->getSExtValue() != AtomicOp::EATOMIC_OR) || (AtomVal->getSExtValue() != 0))
        return false;

    BasicBlock *BbWithAtomic = GInst->getParent();
    if (BbWithAtomic->hasNPredecessorsOrMore(3))
        return false;

    BitCastInst *FirstBitcastInstr = dyn_cast<BitCastInst>(GInst->getNextNonDebugInstruction());
    if (!FirstBitcastInstr)
        return false;

    Instruction *OpInstr = FirstBitcastInstr->getNextNonDebugInstruction();
    if (!OpInstr || !OpInstr->isFast())
        return false;

    BitCastInst *SecondBitcastInstr = dyn_cast<BitCastInst>(OpInstr->getNextNonDebugInstruction());
    if (!SecondBitcastInstr)
        return false;

    GenIntrinsicInst *AtomicFinishInstr = dyn_cast<GenIntrinsicInst>(SecondBitcastInstr->getNextNonDebugInstruction());

    if (!AtomicFinishInstr)
        return false;

    if (AtomicFinishInstr->getIntrinsicID() != GenISAIntrinsic::GenISA_icmpxchgatomicrawA64)
        return false;

    CmpInst *CmpInstr = dyn_cast<CmpInst>(AtomicFinishInstr->getNextNonDebugInstruction());
    if (!CmpInstr)
        return false;

    auto BitCastPat1 =  m_BitCast(m_FAdd(m_BitCast(m_Instruction(Inst)), m_Value()));
    auto BitCastPat2 =  m_BitCast(m_FAdd(m_Value(), m_BitCast(m_Instruction(Inst))));
    if (match(cast<Value>(SecondBitcastInstr), BitCastPat1))
        OperandPos = 1;
    else if (match(cast<Value>(SecondBitcastInstr), BitCastPat2))
        OperandPos = 0;
    else
        return false;

    CmpInst::Predicate Pred = CmpInst::Predicate::ICMP_EQ;
    Instruction *FinishInstr = cast<Instruction>(AtomicFinishInstr);
    auto CmpPattern = m_Cmp(Pred, m_Instruction(Inst), m_Instruction(FinishInstr));

    if (!match(cast<Value>(CmpInstr), CmpPattern))
        return false;

    return true;
}

//Create Subgroup Local Id Built-in befor the instruction Pos
Value *AtomicOptPass::getSubgroupLocalIdBI(Instruction *Pos) {
    IRBuilder<> Builder(Pos);

    Function *simdLaneIdIntrinsic = GenISAIntrinsic::getDeclaration(
        Pos->getModule(),
        GenISAIntrinsic::GenISA_simdLaneId);

    Value *subgroupLocalInvocationId = Builder.CreateZExtOrTrunc(
        Builder.CreateCall(simdLaneIdIntrinsic),
        Builder.getInt32Ty());

    return subgroupLocalInvocationId;
}

bool AtomicOptPass::runOnFunction(Function &F)
{
    Changed = false;
    M = F.getParent();
    llvm::SmallVector<std::tuple<Instruction*, BasicBlock*, BasicBlock*, Instruction*, size_t>, 32> AtomicsEmulationToProcess;
    Wi = &getAnalysis<WIAnalysis>();

    for (auto &B : F) {
        for (auto &I : B) {
            if (!isa<GenIntrinsicInst>(&I))
                continue;

            if (I.getNumOperands() == 0)
                continue;

            uint8_t DepType = Wi->whichDepend(I.getOperand(0));
            if (DepType != IGC::WIBaseClass::WIDependancy::UNIFORM_GLOBAL &&
                DepType != IGC::WIBaseClass::WIDependancy::UNIFORM_WORKGROUP &&
                DepType != IGC::WIBaseClass::WIDependancy::UNIFORM_THREAD) {
                continue;
            }

            size_t OperandPos = 0;
            //Here we check if this is an atomic instruction emulation or not.
            if (checkFloatAtomicEmulation(&I, OperandPos)) {
                Instruction *FirstBitcastInstr = I.getNextNonDebugInstruction();
                Instruction *MainInstr = FirstBitcastInstr->getNextNonDebugInstruction();

                BasicBlock *BbWithAtomic = I.getParent();
                BasicBlock *BackBb = nullptr;
                BasicBlock *EntryBb = nullptr;
                BasicBlock *ExitBb = nullptr;

                //Here we are trying to determine the basic block from which we enter the emulation (EntryBb),
                //the basic block, which brings us back to the emulation loop (BackBb) and
                //the basic block that comes right after the emulation (ExitBb)
                for (BasicBlock *Pred : predecessors(BbWithAtomic)) {
                    BasicBlock *PredBb = Pred->getSinglePredecessor();

                    if (Pred == BbWithAtomic)
                        BackBb = BbWithAtomic;
                    else if (PredBb != BbWithAtomic)
                        EntryBb = Pred;
                    else if (PredBb == BbWithAtomic)
                        BackBb = Pred;
                }

                if (!BackBb || !EntryBb)
                    continue;

                if (Wi->insideDivergentCF(EntryBb->getTerminator()))
                    continue;

                for (BasicBlock *Succ : successors(BbWithAtomic))
                    if (Succ != BackBb)
                        ExitBb = Succ;

                if (!ExitBb)
                    continue;

                std::tuple instrTuple = std::make_tuple(&I, BackBb, ExitBb, MainInstr, OperandPos);
                AtomicsEmulationToProcess.push_back(instrTuple);
            }
        }
    }

    for (auto& [AtomicInstr, BackBb, FinishBb, AtomicMain, MainInstrArgPos] : AtomicsEmulationToProcess)
    {
        BasicBlock *Bb = AtomicInstr->getParent();
        BasicBlock *Bb1 = Bb->splitBasicBlock(AtomicInstr);

        Instruction *BbTerm = Bb->getTerminator();
        Value *ReduceVal = AtomicMain->getOperand((unsigned int)MainInstrArgPos);
        Instruction *NewReduce = createReduce(BbTerm, ReduceVal);
        if (!NewReduce)
            continue;

        IRBuilder<> Builder(BbTerm);
        AtomicMain->setOperand((unsigned int)MainInstrArgPos, NewReduce);

        Value *Id = getSubgroupLocalIdBI(NewReduce);
        Value *ConstInt = Builder.getInt32(0);
        Value *Cmp = Builder.CreateICmpEQ(Id, ConstInt);
        Builder.CreateCondBr(Cmp, Bb1, FinishBb);
        BbTerm->eraseFromParent();

        if (BackBb == Bb)
            BackBb = Bb1;

        BranchInst *BackBranch = cast<BranchInst>(BackBb->getTerminator());

        if (BackBranch->isConditional()) {
            if (BackBranch->getSuccessor(0) == Bb) {
                BackBranch->setSuccessor(0, Bb1);
            } else if (BackBranch->getSuccessor(1) == Bb) {
                BackBranch->setSuccessor(1, Bb1);
            }
        } else {
            BackBranch->setSuccessor(0, Bb1);
        }
        Changed = true;
    }
    return Changed;
}