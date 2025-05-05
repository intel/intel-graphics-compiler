/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BlockMemOpAddrScalarizationPass.hpp"
#include <llvm/IR/Function.h>

#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

char BlockMemOpAddrScalarizationPass::ID = 0;

#define PASS_FLAG "block-memop-addr-scalar"
#define PASS_DESCRIPTION "Scalarization of address calculations for block memory operations."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(BlockMemOpAddrScalarizationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_END(BlockMemOpAddrScalarizationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

BlockMemOpAddrScalarizationPass::BlockMemOpAddrScalarizationPass() : FunctionPass(ID) {
    initializeBlockMemOpAddrScalarizationPassPass(*PassRegistry::getPassRegistry());
}

bool BlockMemOpAddrScalarizationPass::runOnFunction(Function &F) {
    Changed = false;
    WI = &getAnalysis<WIAnalysis>();
    visit(F);
    InstCanBeScalarized.clear();
    ExistingBroadcasts.clear();
    return Changed;
}

void BlockMemOpAddrScalarizationPass::visitCallInst(CallInst& C) {
    if (GenIntrinsicInst *I = dyn_cast<GenIntrinsicInst>(&C)) {
        GenISAIntrinsic::ID id = I->getIntrinsicID();
        if (id == GenISAIntrinsic::GenISA_simdBlockRead || id == GenISAIntrinsic::GenISA_simdBlockWrite)
            scalarizeAddrArithmForBlockRdWr(I);
    }
}

// This function checks if InstForCheck can be scalarized.
bool BlockMemOpAddrScalarizationPass::canInstBeScalarized(Instruction *InstForCheck, Instruction *Root) {
    if (checkInst(InstForCheck) != InstType::CanBeScalar)
        return false;

    bool GotFinalInst = false;
    for (auto Op = InstForCheck->op_begin(), E = InstForCheck->op_end(); Op != E; Op++) {
        if (Instruction *IOp = dyn_cast<Instruction>(Op)) {
            GotFinalInst = true;
            // Don't process any vector instructions.
            if (IOp->getType()->isVectorTy())
                return false;
        }
    }

    // If InstForCheck does not have any instruction operands, scalarize its result which is used in Root instruction.
    if (!GotFinalInst)
        return false;

    // This check showes that InstForCheck is used only the address calculation chain.
    if (InstForCheck->hasOneUse())
        return true;

    SmallVector<std::tuple<Instruction*, Instruction*, bool>, 32> UseStack;
    SmallVector<Instruction*, 32> Steps;
    Steps.push_back(InstForCheck);
    for (auto U : InstForCheck->users()) {
        if (Instruction *I = dyn_cast<Instruction>(U)) {
            if (I != Root) {
                UseStack.push_back({I, InstForCheck, false});
            }
        }
    }

    while (UseStack.size()) {
        auto &[CurrUse, CurrRoot, Analized] = UseStack.back();

        if (Steps.back() != CurrRoot)
            Steps.pop_back();

        // If we have already analyzed this instruction.
        if (Analized) {
            UseStack.pop_back();
            continue;
        }

        // Mark use as visited.
        Analized = true;

        InstType Res = checkInst(CurrUse);
        if (Res == InstType::BlcokMemOp) {
            Instruction *Op0 = dyn_cast<Instruction>(CurrUse->getOperand(0));
            if (Op0 == CurrRoot) {
                UseStack.pop_back();
                continue;
            }
        } else if (Res == InstType::PreventScalar) {
            return false;
        }

        if (!CurrUse->use_empty()) {
            Steps.push_back(CurrUse);
            for (auto U : CurrUse->users()) {
                if (Instruction *I = dyn_cast<Instruction>(U)) {
                    // This check helps to avoid hanging in the following example:
                    //  entry:
                    //    ...
                    //    br label bb1
                    //  bb1:
                    //    %phires = phi i32 [ %0, %entry ], [ %sum, %bb2 ]
                    //    %cmp = icmp ult i32 %phires, 20
                    //    br i1 %cmp, label %bb2, label %bb3
                    //  bb2:
                    //    %sum = add i32 %%phires, 1
                    //  bb3:
                    //    ...
                    if (std::find(Steps.begin(), Steps.end(), I) != Steps.end())
                        continue;

                    UseStack.push_back({I, CurrUse, false});
                }
            }
        } else {
            UseStack.pop_back();
        }
    }

    return true;
}

InstType BlockMemOpAddrScalarizationPass::checkInst(Instruction *I) {
    bool Check = false;
    // If this I instruction is BlockRead/BlockWrite then return true for current user.
    if (GenIntrinsicInst *GenInst = dyn_cast<GenIntrinsicInst>(I)) {
        GenISAIntrinsic::ID Id = GenInst->getIntrinsicID();
        if (Id == GenISAIntrinsic::GenISA_simdBlockRead || Id == GenISAIntrinsic::GenISA_simdBlockWrite)
            return InstType::BlcokMemOp;
    }

    if (I->isBinaryOp())
        Check = true;

    if (I->isCast())
        Check = true;

    if (isa<GetElementPtrInst>(I))
        Check = true;

    if (isa<PHINode>(I))
        Check = true;

    // Skip intrinsics that don't actually represent code after lowering.
    auto canSkipCall = [](Instruction *I) {
        if (IntrinsicInst *Intr = dyn_cast<IntrinsicInst>(I)) {
            switch (Intr->getIntrinsicID()) {
                default:
                    break;
                case Intrinsic::assume:
                case Intrinsic::dbg_declare:
                case Intrinsic::dbg_value:
                case Intrinsic::dbg_label:
                case Intrinsic::lifetime_start:
                case Intrinsic::lifetime_end:
                    return true;
            }
        }
        return false;
    };

    if (canSkipCall(I))
        Check = true;

    if (!Check)
        return InstType::PreventScalar;

    return InstType::CanBeScalar;
}

Value *BlockMemOpAddrScalarizationPass::insertBroadcast(Instruction *InstForBroadcast) {
    Value *ShuffleRes = nullptr;
    Instruction *PlaceForInsert = nullptr;

    if (isa<PHINode>(InstForBroadcast))
        PlaceForInsert = InstForBroadcast->getParent()->getFirstNonPHI();
    else
        PlaceForInsert = InstForBroadcast->getNextNonDebugInstruction();

    IRBuilder<> Builder(PlaceForInsert);

    if (ExistingBroadcasts.count(InstForBroadcast)) {
        // If broadcast was created before.
        ShuffleRes = ExistingBroadcasts[InstForBroadcast];
    } else {
        Type *CurType = InstForBroadcast->getType();
        Value *ValForShuffle = nullptr;

        if (CurType->getScalarSizeInBits() == 1)
            ValForShuffle = Builder.CreateZExtOrTrunc(InstForBroadcast, Builder.getInt8Ty());
        else if (CurType->isPointerTy())
            ValForShuffle = Builder.CreatePtrToInt(InstForBroadcast, Builder.getInt64Ty());
        else
            ValForShuffle = cast<Value>(InstForBroadcast);

        Value *Args[3] = {ValForShuffle, Builder.getInt32(0), Builder.getInt32(0)};
        Type *Types[3] = {ValForShuffle->getType(), Builder.getInt32Ty(), Builder.getInt32Ty()};
        Function *BroadcastFunc = GenISAIntrinsic::getDeclaration(InstForBroadcast->getModule(),
            GenISAIntrinsic::GenISA_WaveShuffleIndex,
            Types);
        Value *BroadcastCall = Builder.CreateCall(BroadcastFunc, Args);

        if (CurType->getScalarSizeInBits() == 1)
            ShuffleRes = Builder.CreateZExtOrTrunc(BroadcastCall, CurType);
        else if (CurType->isPointerTy())
            ShuffleRes = Builder.CreateIntToPtr(BroadcastCall, CurType);
        else
            ShuffleRes = BroadcastCall;

        ExistingBroadcasts.insert({InstForBroadcast, ShuffleRes});
    }

    return ShuffleRes;
}

bool BlockMemOpAddrScalarizationPass::scalarizeAddrArithmForBlockRdWr(GenIntrinsicInst *BlockInstr)
{
    bool Scalarized = false;
    Instruction *AddrInstr = dyn_cast<Instruction>(BlockInstr->getOperand(0));
    if (!AddrInstr)
        return Scalarized;

    // This map will contain instructions (keys) that will be broadcast, and instructions (values) where the result of the broadcast will be used.
    DenseMap<Instruction*, SmallVector<Instruction*, 4>> InstForBrd;

    SmallVector<Instruction*, 2> V = {AddrInstr};
    // This vector contains the root instruction that was checked in previous steps and its operands that will be checked in the current step.
    SmallVector<std::tuple<Instruction*, SmallVector<Instruction*, 2>>, 2> InstrVector = {{BlockInstr, V}};
    while (InstrVector.size()) {
        // Data structure for further iterations.
        decltype(InstrVector) NewInstrVector;

        for (const auto& [Root, Insts] : InstrVector) {
            for (Instruction *I : Insts) {
                if (InstCanBeScalarized.count(I))
                    continue;

                // Check I instruction and its users.
                if (canInstBeScalarized(I, Root)) {
                    InstCanBeScalarized.insert(I);

                    // Now lets check its arguments.
                    SmallVector<Instruction*, 2> newInsts;
                    for (auto Op = I->op_begin(), E = I->op_end(); Op != E; Op++) {
                        if (Instruction *InOp = dyn_cast<Instruction>(*Op)) {
                            if (WI->isUniform(InOp))
                                continue;

                            newInsts.push_back(InOp);
                        }
                    }

                    NewInstrVector.emplace_back(I, newInsts);
                } else {
                    // Terminate the algorithm if the address is NOT used in any instructions other than BlockWrite/BlockRead.
                    if (I == BlockInstr->getOperand(0))
                        return Scalarized;

                    if (InstForBrd.count(I)) {
                        if (std::find(InstForBrd[I].begin(), InstForBrd[I].end(), Root) != InstForBrd[I].end())
                            continue;

                        InstForBrd[I].push_back(Root);
                    } else {
                        InstForBrd.insert({I, {Root}});
                    }
                }
            }
        }
        // Update instructions list for next check.
        InstrVector = std::move(NewInstrVector);
    }

    // Insert broadcast instructions
    for (const auto &Item : InstForBrd) {
        Instruction *InstForBrd = Item.first;

        if (GenIntrinsicInst *GenInst = dyn_cast<GenIntrinsicInst>(InstForBrd)) {
            GenISAIntrinsic::ID Id = GenInst->getIntrinsicID();
            if (Id == GenISAIntrinsic::GenISA_WaveShuffleIndex)
                continue;
        }

        Value *BroadcastInstr = insertBroadcast(InstForBrd);
        if (!BroadcastInstr)
            continue;
        Scalarized = true;

        for (auto Root : Item.second) {
            size_t ArgNum = 0;
            for (auto Op = Root->op_begin(), E = Root->op_end(); Op != E; Op++) {
                if (dyn_cast<Instruction>(Op) == InstForBrd) {
                    Root->setOperand(ArgNum, BroadcastInstr);
                    break;
                }
                ArgNum++;
            }
        }
    }

    return Scalarized;
}