/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/SampleCmpToDiscard.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
using namespace llvm;
using namespace IGC;
using namespace GenISAIntrinsic;

class SampleCmpToDiscard : public FunctionPass
{
public:
    static char ID;

    SampleCmpToDiscard() : FunctionPass(ID)
    {
    }
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const
    {
        AU.addRequired<CodeGenContextWrapper>();
    }

    virtual bool runOnFunction(Function& F);

    virtual llvm::StringRef getPassName() const
    {
        return "SampleCmpToDiscard";
    }
private:
    bool processBlock(BasicBlock* BB);
    bool canFoldValue(Instruction* inst, std::map<Value*, APFloat>& instToValMap);
};

char SampleCmpToDiscard::ID = 0;

#define PASS_FLAG "igc-discard-samplecmp"
#define PASS_DESCRIPTION "Checks possibility of SampleCmpToDiscard optimization"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SampleCmpToDiscard, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(SampleCmpToDiscard, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

FunctionPass* IGC::CreateSampleCmpToDiscardPass()
{
    return new SampleCmpToDiscard();
}

bool SampleCmpToDiscard::runOnFunction(Function& F)
{
    auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (pCtx->type != ShaderType::PIXEL_SHADER)
    {
        return false;
    }
    bool changed = false;
    for (auto BI = F.begin(), BE = F.end(); BI != BE;)
    {
        BasicBlock* currentBB = &(*BI);
        ++BI;
        changed |= processBlock(currentBB);
    }
    return changed;
}

bool SampleCmpToDiscard::processBlock(BasicBlock* BB)
{
    auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    std::map<Value*, APFloat> instToValMap;
    for (auto II = BB->rbegin(), IE = BB->rend(); II != IE; ++II)
    {
        instToValMap.clear();
        if (GenIntrinsicInst * genIntr = dyn_cast<GenIntrinsicInst>(&(*II)))
        {
            Instruction* valueToFold = nullptr;
            GenISAIntrinsic::ID ID = genIntr->getIntrinsicID();
            // only consider sample_lc
            if (ID == GenISAIntrinsic::GenISA_sampleCptr ||
                ID == GenISAIntrinsic::GenISA_sampleCMlodptr ||
                ID == GenISAIntrinsic::GenISA_sampleBCMlodptr ||
                ID == GenISAIntrinsic::GenISA_sampleBCptr)
            {
                if (genIntr->hasOneUse())
                {
                    if (ExtractElementInst * ext = dyn_cast<ExtractElementInst>(*genIntr->user_begin()))
                    {
                        if (ConstantInt * index = dyn_cast<ConstantInt>(ext->getIndexOperand()))
                        {
                            if (index->isZero())
                            {
                                valueToFold = ext;
                            }
                        }
                    }
                }
            }
            // if we haven't found the pattern we want to skip to the next instruction
            if (valueToFold == nullptr)
            {
                continue;
            }
            APFloat newAPFloat = cast<ConstantFP>(ConstantFP::get(valueToFold->getType(), 1.0))->getValueAPF();
            instToValMap.emplace(valueToFold, newAPFloat);

            bool foldableInst = canFoldValue(valueToFold, instToValMap);
            if (foldableInst)
            {
                llvm::Value* texOp = cast<SampleIntrinsic>(genIntr)->getTextureValue();
                // set compiler outputs
                if (texOp->getType()->getPointerAddressSpace())
                {
                    uint as = texOp->getType()->getPointerAddressSpace();
                    uint bufferIndex;
                    bool directIndexing;
                    DecodeAS4GFXResource(as, directIndexing, bufferIndex);
                    // set the compiler outputs to indicate sample_Cqa which could write 1.0f to the driver
                    pCtx->m_instrTypes.sampleCmpToDiscardOptimizationPossible = true;
                    pCtx->m_instrTypes.sampleCmpToDiscardOptimizationSlot = bufferIndex;
                    return true;
                }
            }
        }
    }
    instToValMap.clear();
    return false;
}

bool SampleCmpToDiscard::canFoldValue(Instruction* inst, std::map<Value*, APFloat>& instToValMap)
{
    // currently we are handling the following type of cases
    /*
    * %92 = call <4 x half> @llvm.genx.GenISA.sampleCptr.v4f16.f16.p196609v4f32.p524289i32(
        half %91, half %89, half %90, half %91, half %87, half 0xH0000, <4 x float> addrspace(196609)* null,
        i32 addrspace(524289)* null, i32 0, i32 0, i32 0)
    * %93 = extractelement <4 x half> %92, i32 0
    * %94 = fsub half 0xH3C00, %93 // 1 - B
    * %95 = fmul half %94, 0xH3400 // 0.25 * (1-B)
    * %96 = fadd half %95, %93 // 0.25 * ( 1-B) + B
    * call void @llvm.genx.GenISA.OUTPUT.f16(half %96, half %96, half %96, half %96, i32 0, i32 0, i32 15)
    * ret void
    */

    /*
    * %62 = call <4 x float> @llvm.genx.GenISA.sampleBCptr.v4f32.f32.p196609i8.p524293i8(
    * float %61, float 0.000000e+00, float %57, float %58, float 0.000000e+00,
    * float 0.000000e+00, i8 addrspace(196609)* null,
    * i8 addrspace(524293)* inttoptr (i64 5 to i8 addrspace(524293)*), i32 0, i32 0, i32 0)
    * %scalar40 = extractelement <4 x float> %62, i32 0
    * %phitmp = fmul float %scalar40, 7.500000e-01
    * %63 = fadd float %phitmp, 2.500000e-01
    * call void @llvm.genx.GenISA.OUTPUT.f32(float %63, float %63, float %63, float %63, i32 0, i32 0, i32 15)
    */
    bool foldInst = false;
    APFloat newConstantFloat = cast<ConstantFP>(ConstantFP::get(inst->getType(), 1.0))->getValueAPF();
    auto mapFind = instToValMap.find(inst);
    if (mapFind == instToValMap.end())
    {
        return false;
    }

    for (auto UI = inst->user_begin(), UE = inst->user_end(); UI != UE; ++UI)
    {
        // assume the value of inst returns value 1;
        BinaryOperator* bin = dyn_cast<BinaryOperator>(*UI);
        if (bin && bin->getOpcode() == Instruction::FSub)
        {
            auto itOtherOp = (bin->getOperand(0) == inst) ? instToValMap.find(bin->getOperand(1)) : instToValMap.find(bin->getOperand(0));
            if (isa<ConstantFP>(bin->getOperand(0)))
            {
                ConstantFP* C0 = cast<ConstantFP>(bin->getOperand(0));
                newConstantFloat = C0->getValueAPF();
                newConstantFloat.subtract(mapFind->second, llvm::APFloat::rmNearestTiesToEven);
                instToValMap.emplace(bin, newConstantFloat);
                foldInst = canFoldValue(bin, instToValMap);
            }
            else if (isa<ConstantFP>(bin->getOperand(1)))
            {
                ConstantFP* C0 = cast<ConstantFP>(bin->getOperand(1));
                newConstantFloat = mapFind->second;
                newConstantFloat.subtract(C0->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
                instToValMap.emplace(bin, newConstantFloat);
                foldInst = canFoldValue(bin, instToValMap);
            }
            else if (itOtherOp != instToValMap.end())
            {
                if (bin->getOperand(0) == inst)
                {
                    newConstantFloat = mapFind->second;
                    newConstantFloat.subtract(itOtherOp->second, llvm::APFloat::rmNearestTiesToEven);
                }
                else
                {
                    newConstantFloat = itOtherOp->second;
                    newConstantFloat.subtract(mapFind->second, llvm::APFloat::rmNearestTiesToEven);
                }
                instToValMap.emplace(bin, newConstantFloat);
                foldInst = canFoldValue(bin, instToValMap);
            }
        }

        else if (bin && bin->getOpcode() == Instruction::FAdd)
        {
            auto itOtherOp = (bin->getOperand(0) == inst) ? instToValMap.find(bin->getOperand(1)) : instToValMap.find(bin->getOperand(0));
            if (isa<ConstantFP>(bin->getOperand(0)))
            {
                ConstantFP* C0 = cast<ConstantFP>(bin->getOperand(0));
                newConstantFloat = mapFind->second;
                newConstantFloat.add(C0->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
                instToValMap.emplace(bin, newConstantFloat);
                foldInst = canFoldValue(bin, instToValMap);
            }
            else if (isa<ConstantFP>(bin->getOperand(1)))
            {
                ConstantFP* C0 = cast<ConstantFP>(bin->getOperand(1));
                newConstantFloat = mapFind->second;
                newConstantFloat.add(C0->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
                instToValMap.emplace(bin, newConstantFloat);
                foldInst = canFoldValue(bin, instToValMap);
            }
            else if (itOtherOp != instToValMap.end())
            {
                newConstantFloat = mapFind->second;
                newConstantFloat.add(itOtherOp->second, llvm::APFloat::rmNearestTiesToEven);
                instToValMap.emplace(bin, newConstantFloat);
                foldInst = canFoldValue(bin, instToValMap);
            }
        }

        else if (bin && bin->getOpcode() == Instruction::FMul)
        {
            auto itOtherOp = (bin->getOperand(0) == inst) ? instToValMap.find(bin->getOperand(1)) : instToValMap.find(bin->getOperand(0));
            if (isa<ConstantFP>(bin->getOperand(0)))
            {
                ConstantFP* C0 = cast<ConstantFP>(bin->getOperand(0));
                newConstantFloat = mapFind->second;
                newConstantFloat.multiply(C0->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
                instToValMap.emplace(bin, newConstantFloat);
                foldInst = canFoldValue(bin, instToValMap);
            }
            else if (isa<ConstantFP>(bin->getOperand(1)))
            {
                ConstantFP* C0 = cast<ConstantFP>(bin->getOperand(1));
                newConstantFloat = mapFind->second;
                newConstantFloat.multiply(C0->getValueAPF(), llvm::APFloat::rmNearestTiesToEven);
                instToValMap.emplace(bin, newConstantFloat);
                foldInst = canFoldValue(bin, instToValMap);
            }
            else if (itOtherOp != instToValMap.end())
            {
                newConstantFloat = mapFind->second;
                newConstantFloat.multiply(itOtherOp->second, llvm::APFloat::rmNearestTiesToEven);
                instToValMap.emplace(bin, newConstantFloat);
                foldInst = canFoldValue(bin, instToValMap);
            }
        }

        else if (isa<GenIntrinsicInst>(*UI) && ((mapFind->second).compare(newConstantFloat) == APFloat::cmpEqual))
        {
            GenIntrinsicInst* CI = cast<GenIntrinsicInst>(*UI);
            if ((llvm::isa<llvm::RTWriteIntrinsic>(CI)) && (llvm::isa<llvm::ReturnInst>(CI->getNextNode())))
            {
                return true;
            }
        }
    }

    return foldInst;
}
