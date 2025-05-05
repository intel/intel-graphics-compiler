/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MCSOptimization.hpp"
#include "IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include <set>
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include <llvm/IR/InstVisitor.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Support/Casting.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/IGCIRBuilder.h"
#include "common/igc_regkeys.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
/************************************************************************
This transformation is not safe in general. It can be applied only in those case:
-We know that the resouce is MCS compressed
-We need to know that we don't access out of bound sample index
************************************************************************/
class MCSOptimization : public FunctionPass, public InstVisitor<MCSOptimization>
{
public:
    MCSOptimization() : FunctionPass(ID) {}
    bool runOnFunction(Function& F);
    void visitCallInst(llvm::CallInst& I);
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const
    {
        AU.addRequired<CodeGenContextWrapper>();
    }
    virtual llvm::StringRef getPassName() const
    {
        return "MCSOptimization";
    }

    static char ID;
    bool m_changed = false;

private:
    bool shaderSamplesCompressedSurfaces(CodeGenContext* ctx)
    {
        ModuleMetaData* modMD = ctx->getModuleMetaData();
        for (unsigned int i = 0; i < NUM_SHADER_RESOURCE_VIEW_SIZE; i++)
        {
            if (modMD->m_ShaderResourceViewMcsMask[i] != 0)
            {
                return true;
            }
        }
        return false;
    }
protected:
};

char MCSOptimization::ID = 0;

bool MCSOptimization::runOnFunction(Function& F)
{

    if (IGC_IS_FLAG_ENABLED(DisableMCSOpt))
    {
        return false;
    }
    m_changed = false;
    visit(F);
    return m_changed;
}

void MCSOptimization::visitCallInst(llvm::CallInst& I)
{
    Function* F = I.getParent()->getParent();
    IGCIRBuilder<> IRB(F->getContext());

    if (LdmcsInstrinsic * ldMcs = dyn_cast<LdmcsInstrinsic>(&I))
    {
        CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

        {
            if (!shaderSamplesCompressedSurfaces(ctx))
            {
                return;
            }

            llvm::Value* textureArgValue = ldMcs->getTextureValue();
            uint textureIndex;
            if (textureArgValue->getType()->isPointerTy())
            {
                uint addrSpace = textureArgValue->getType()->getPointerAddressSpace();
                uint bufferIndex = 0;
                bool directIdx = false;
                DecodeAS4GFXResource(addrSpace, directIdx, bufferIndex);
                textureIndex = bufferIndex;
            }
            else
            {
                textureIndex = int_cast<uint>(GetImmediateVal(textureArgValue));
            }

            const unsigned int shaderResourceViewMcsMaskIndex = textureIndex / BITS_PER_QWORD;
            const unsigned long long resourceViewMcsMaskElement = ctx->getModuleMetaData()->m_ShaderResourceViewMcsMask[shaderResourceViewMcsMaskIndex];
            const unsigned int resourceViewMaskTextureBit = textureIndex % BITS_PER_QWORD;
            IGC_ASSERT_MESSAGE(textureIndex <= 127, "Texture index is incorrectly extracted from ld_mcs");

            unsigned long long resultBit = resourceViewMcsMaskElement >> resourceViewMaskTextureBit;
            if ((resultBit & 1) == 0)
            {
                return;
            }
        }
        ExtractElementInst* EEI = nullptr;
        for (auto useItr : ldMcs->users())
        {
            if (ExtractElementInst * ee1 = dyn_cast<ExtractElementInst>(useItr))
            {
                if (ConstantInt * channel = dyn_cast<ConstantInt>(ee1->getOperand(1)))
                {
                    if (channel->isZero())
                    {
                        EEI = ee1;
                        break;
                    }
                }
            }
        }

        if (EEI != nullptr)
        {
            if (EEI->hasOneUse())
                return; //only one use of EEI -- noOptimization

            LdmsInstrinsic* firstUse = nullptr;

            for (auto it = EEI->getIterator(); it != EEI->getParent()->end(); ++it)
            {
                if (LdmsInstrinsic * ldmsIntr = dyn_cast<LdmsInstrinsic>(&*it))
                {
                    if (ldmsIntr->getOperand(1) == dyn_cast<Value>(EEI))
                    {
                        //first use and in the def's BB
                        firstUse = ldmsIntr;
                        break;
                    }
                }
            }

            if (!firstUse)
                return;

            //collect all blocks where this EEI insts is getting used
            std::set<BasicBlock*> useBlocks;
            for (auto BitcastUses = EEI->user_begin(); BitcastUses != EEI->user_end(); BitcastUses++)
            {
                Instruction* ldmsInst = dyn_cast<Instruction>(*BitcastUses);
                if (ldmsInst)
                {
                    if (dyn_cast<ConstantInt>(ldmsInst->getOperand(0)))
                    {
                        useBlocks.insert(ldmsInst->getParent());
                    }
                    else
                    {
                        return;
                    }
                }
            }

            //iterate over useBlocks.
            //For each useBlock, collect all the ldms insts present within the use block corresponding to this EEI
            for (auto BB : useBlocks)
            {
                std::vector<LdmsInstrinsic*> ldmsInstsToMove;
                std::vector<LdmsInstrinsic*> ldmsInstsToClub;
                for (auto inst = BB->begin(); inst != BB->end(); inst++)
                {
                    if (LdmsInstrinsic * ldmsIntr = dyn_cast<LdmsInstrinsic>(inst))
                    {
                        if (ldmsIntr->getOperand(1) == dyn_cast<Value>(EEI))
                        {
                            if (ldmsIntr == firstUse)
                                continue; //don't move the first use into the then block , need it for phi Node
                            ldmsInstsToMove.push_back(ldmsIntr);
                        }
                    }
                }

                //this is added because clubbing all ld2dms into a single then block
                //increases register pressure and causes spilling
                int instClubThreshold = IGC_GET_FLAG_VALUE(ld2dmsInstsClubbingThreshold); //# ld2dms insts that can be moved into the then block
                //int instClubThreshold = 2;
                bool allInstsWillBeMoved = false;

                while (!allInstsWillBeMoved)
                {
                    ldmsInstsToClub.clear();
                    //Threshold is more than # of insts that are to be moved. So move all.
                    if (instClubThreshold >= static_cast<int>(ldmsInstsToMove.size()))
                    {
                        ldmsInstsToClub = ldmsInstsToMove;
                        allInstsWillBeMoved = true;
                    }
                    else
                    {
                        //pick the first 0-threshold # of insts and move them only
                        for (int i = 0; i < instClubThreshold; i++)
                        {
                            ldmsInstsToClub.push_back(ldmsInstsToMove[i]);
                        }
                        ldmsInstsToMove.erase(ldmsInstsToMove.begin(), ldmsInstsToMove.begin() + instClubThreshold);
                    }

                    //split the block into a new then block
                    BasicBlock* ldmsUseBB = nullptr; //second entry to the phi node
                    BasicBlock* thenBlock = nullptr;
                    IGCLLVM::TerminatorInst* thenBlockTerminator = nullptr;
                    if (ldmsInstsToClub.size() != 0)
                    {
                        LdmsInstrinsic* ldmsUse = ldmsInstsToClub[0];
                        ldmsUseBB = ldmsUse->getParent();
                        IRB.SetInsertPoint(ldmsUse);
                        Value* ValueisMCSNotZero = nullptr;
                        for (unsigned int i = 0; i < ldmsUse->getNumMcsOperands(); i++)
                        {
                            Value* mcs = firstUse->getMcsOperand(i);
                            Value* cnd1 = IRB.CreateICmpNE(mcs, ConstantInt::get(mcs->getType(), 0));
                            if (ValueisMCSNotZero == nullptr)
                            {
                                ValueisMCSNotZero = cnd1;
                            }
                            else
                            {
                                ValueisMCSNotZero = IRB.CreateOr(ValueisMCSNotZero, cnd1);
                            }
                        }
                        thenBlockTerminator = SplitBlockAndInsertIfThen(ValueisMCSNotZero, ldmsUse, false);
                        thenBlock = thenBlockTerminator->getParent();
                    }

                    //Move the collected ldms insts into the then block and insert their phi nodes in the successor of the then block
                    if (thenBlockTerminator)
                    {
                        for (auto instToMove : ldmsInstsToClub)
                        {
                            instToMove->moveBefore(thenBlockTerminator);
                            IRB.SetInsertPoint(&*(thenBlockTerminator->getSuccessor(0)->begin()));
                            PHINode* PN = IRB.CreatePHI(instToMove->getType(), 2);
                            instToMove->replaceAllUsesWith(PN);
                            PN->addIncoming(instToMove, thenBlock);
                            PN->addIncoming(firstUse, ldmsUseBB);
                            m_changed = true;
                        }
                    }

                }
            }
            m_changed = true;
        }
    }
}

namespace IGC {
// Optimize ld2ms message assuming resources are always compressed"
#define PASS_FLAG "igc-mcs-optimization"
#define PASS_DESCRIPTION "This is an optimization pass for ld2dms message "
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
    IGC_INITIALIZE_PASS_BEGIN(MCSOptimization, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
    IGC_INITIALIZE_PASS_END(MCSOptimization, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

        FunctionPass* CreateMCSOptimization()
    {
        return new MCSOptimization();
    }
}
