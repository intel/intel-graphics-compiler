/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/PassInfo.h"
#include "llvm/PassRegistry.h"
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "IGC/Compiler/CodeGenPublic.h"
#include "Probe/Assertion.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"
#include "ResourceLoopUnroll.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "LLVM3DBuilder/BuiltinsFrontend.hpp"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

using namespace llvm;
namespace IGC
{
/////////////////////////////////////////////////////////////////////////////////////////////
class ResourceLoopUnroll : public llvm::FunctionPass, public llvm::InstVisitor<ResourceLoopUnroll>
{
public:
    static char ID; ///< ID used by the llvm PassManager (the value is not important)

    ResourceLoopUnroll();

    ////////////////////////////////////////////////////////////////////////
    virtual bool runOnFunction(llvm::Function& F);

    ////////////////////////////////////////////////////////////////////////
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;

    ////////////////////////////////////////////////////////////////////////
    void visitCallInst(llvm::CallInst& CI);

private:
    ////////////////////////////////////////////////////////////////////////
    bool ProcessFunction(llvm::Function& F);
    bool emitResourceLoop( llvm::CallInst* CI );

    ////////////////////////////////////////////////////////////////////////
    void InvalidateMembers();

    ////////////////////////////////////////////////////////////////////////
    llvm::Function* m_CurrentFunction = nullptr;

    WIAnalysis* m_WIAnalysis = nullptr;
    CodeGenContext* m_pCodeGenContext = nullptr;

    std::vector<llvm::CallInst*> m_ResourceLoopCandidates;
};

char ResourceLoopUnroll::ID = 0;

////////////////////////////////////////////////////////////////////////////
ResourceLoopUnroll::ResourceLoopUnroll() :
    llvm::FunctionPass(ID)
{
    initializeResourceLoopUnrollPass(*PassRegistry::getPassRegistry());
}

////////////////////////////////////////////////////////////////////////
bool ResourceLoopUnroll::runOnFunction(llvm::Function& F)
{
    m_pCodeGenContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_WIAnalysis = &getAnalysis<WIAnalysis>();

    if (m_WIAnalysis == nullptr || m_pCodeGenContext == nullptr)
    {
        IGC_ASSERT(0);
        return false;
    }

    m_CurrentFunction = &F;

    InvalidateMembers();

    return ProcessFunction(F);
}

////////////////////////////////////////////////////////////////////////
void ResourceLoopUnroll::InvalidateMembers()
{
    m_ResourceLoopCandidates.clear();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
/// %victimLane_0 = getActiveChannel();
/// %res_0 = broadcast(%res, %victimLane_0)
/// %f = cmp(res, res_0);
/// br %f, label %L_uniform_send_0, label %L_else_0
/// L_uniform_send_0:
///   %value_0 = send(%payload, res_0); // VISA: generate $0 SSBO token
///   br label %L_joinPoint;
/// L_else_0:
///   %victimLane_1 = getActiveChannel();
///   %res_1 = broadcast(%res, %victimLane_1); //VISA: use a0.2:w or higher for broadcast to avoid dependency with a0.0:dw (aka a0.0:w and a0.1:w)
///   %f = cmp(res, res_1);
///   br %f, label %L_uniform_send_1, label% L_else_1
///   L_uniform_send_1:
///     %value_1 = send(%payload, res_1); // VISA: dependency on $0.src for filling a0.0 for indirect descr, // generate $1 SSBO
///     br labe l%L_joinPoint;
///   L_else_1:
///      %victimLane_2 = getActiveChannel();
///      %res_2 = broadcast(%res, %victimLane_2);
///      %f = cmp(res, res_2);
///      br %f, label %L_uniformsend_2, label %L_else_2
///      L_uniform_send_2:
///         %value_2 = send(%payload, res_2);  // VISA: dependency on $1.src for filling a0.0 for indirect descr // generate $2 SSBO
///         br label% L_joinPoint;
///      L_else_2:
///         ... // recurrently generate like above
///
/// L_joinPoint:
/// join();
/// %value = phi XX[%value_0, %L_uniform_send_0], [%value_1, %L_uniform_send_1], [%value_2, %L_uniform_send_2], ...// VISA: on first usage of %value we should issue sync to wait for all tokens $X.dst used in above construct
bool ResourceLoopUnroll::emitResourceLoop(llvm::CallInst* CI)
{
    int LOOP_COUNT = IGC_GET_FLAG_VALUE(ResourceLoopUnrollNested);
    if (!LOOP_COUNT)
        return false;

    BasicBlock* BB = CI->getParent();
    LLVMContext& context = BB->getContext();
    PLATFORM platform; // WA, platform is not needed
    LLVM3DBuilder<> builder(context, platform);

    auto createResLoopIter = [&builder, this]
    (Instruction* inst, BasicBlock* checkBB, BasicBlock* nextBB, BasicBlock* exitBB)
        {
            Value* resource = nullptr;
            Value* sampler = nullptr;
            Value* texture = nullptr;
            Value* pairTexture = nullptr;

            Value* resourceNew = nullptr;
            Value* samplerNew = nullptr;
            Value* textureNew = nullptr;
            Value* pairTextureNew = nullptr;

            GetResourceOperand(inst, resource, pairTexture, texture, sampler);

            // Fill UniformSendBB
            builder.SetInsertPoint(checkBB);

            Value* cond = builder.getTrue();

            // if ld instr available
            if (resource)
            {
                resourceNew = builder.readFirstLane(resource);
                resourceNew->setName("firstActiveRes");
                cond = builder.CreateICmpEQ(resource, resourceNew);
            }
            else if (pairTexture && texture && sampler) // then must be sampler
            {
                samplerNew = sampler;
                textureNew = texture;
                pairTextureNew = pairTexture;

                Value* textureCond = nullptr;
                Value* samplerCond = nullptr;

                // No need to repeat 3 possible times to call readFirstLane(src).
                // As readFirstLane(src) here will call:
                // firstLaneID = getFirstLaneID(helperLaneMode), and
                // create_waveshuffleIndex(src, firstLaneID, helperLaneMode);
                // Here the helperLaneMode is actually nullptr (default).
                llvm::Value* firstLaneID = builder.getFirstLaneID(nullptr);

                // need care about pairTexture uniform???
                if (!m_WIAnalysis->isUniform(pairTexture))
                {
                    pairTextureNew = builder.create_waveshuffleIndex(pairTexture, firstLaneID, nullptr);
                    pairTextureNew->setName("firstActivePairTex");
                }

                if (!m_WIAnalysis->isUniform(texture))
                {
                    textureNew = builder.create_waveshuffleIndex(texture, firstLaneID, nullptr);
                    textureNew->setName("firstActiveTex");
                }

                if (!m_WIAnalysis->isUniform(sampler))
                {
                    samplerNew = builder.create_waveshuffleIndex(sampler, firstLaneID, nullptr);
                    samplerNew->setName("firstActiveSampler");
                }

                if (textureNew != texture)
                {
                    // if textureNew == uniform
                    textureCond = builder.CreateICmpEQ(texture, textureNew);
                }

                if (samplerNew != sampler)
                {
                    // if samplerNew == uniform
                    samplerCond = builder.CreateICmpEQ(sampler, samplerNew);
                }

                if (textureCond && samplerCond)
                {
                    // if textureNew == uniform && samplerNew == uniform
                    cond = builder.CreateAnd(textureCond, samplerCond);
                }
                else if (textureCond)
                {
                    cond = textureCond;
                }
                else if (samplerCond)
                {
                    cond = samplerCond;
                }
            }
            else
            {
                IGC_ASSERT(0);
            }

            llvm::Instruction* predSendInstr = inst->clone();
            SetResourceOperand(predSendInstr, resourceNew, pairTextureNew, textureNew, samplerNew);
            predSendInstr->setName("resLoopSubIterSend");
            builder.Insert(predSendInstr);

            // add the cmp/instruction combo to our predication map
            m_pCodeGenContext->getModuleMetaData()->predicationMap[predSendInstr] = cond;

            builder.CreateCondBr(cond, exitBB, nextBB);

            return predSendInstr;
        };

    //////////////////////////////////////////////////////////////////////////

    // those are created always
    BasicBlock* mergeBB = BB->splitBasicBlock(CI, "unroll-merge");
    BasicBlock* latch = BasicBlock::Create(context, "latch", BB->getParent(), mergeBB);

    // Fill MergeBB
    builder.SetInsertPoint(mergeBB->getFirstNonPHI());
    PHINode* PN = builder.CreatePHI(CI->getType(), LOOP_COUNT, "");

    //we create it "from the end", adding new blocks before previously inserted
    BasicBlock* before = latch;
    Instruction* send = nullptr;
    for (int i = 0; i < LOOP_COUNT; i++)
    {
        // Basicblocks for loop
        BasicBlock* partialCheckBB = BasicBlock::Create(context, "partial_check", BB->getParent(), before);

        send = createResLoopIter(CI, partialCheckBB, before, mergeBB);

        PN->addIncoming(send, partialCheckBB);
        before = partialCheckBB;
    }

    // save the first unroll BB and the send use (the use is for all unroll BB in this loop)
    m_pCodeGenContext->getModuleMetaData()->lifeTimeStartMap[before] = send;

    // latch goes back to last created BB, which actually will be first BB due to ordering of creating and "before" poitner
    builder.SetInsertPoint(latch);
    builder.CreateBr(before);

    // BB pointed to latch
    BB->getTerminator()->eraseFromParent();
    builder.SetInsertPoint(BB);
    builder.CreateBr(before);

    // set phinode MD to something, so we can capture it later during codegen to set exclusive visa marker
    PN->setMetadata("MyUniqueExclusiveLoadMetadata", llvm::MDNode::get(context, {}));

    CI->replaceAllUsesWith(PN);
    CI->eraseFromParent();

    return true;
}

void ResourceLoopUnroll::visitCallInst(llvm::CallInst& CI)
{
    if (llvm::GenIntrinsicInst* pIntr = llvm::dyn_cast<llvm::GenIntrinsicInst>(&CI))
    {
        if (auto* LI = dyn_cast<LdRawIntrinsic>(pIntr))
        {
            if (IGC_IS_FLAG_DISABLED(DisableResourceLoopUnrollNestedLsc))
            {
                llvm::Value* pResourceValue = LI->getResourceValue();
                if (pResourceValue &&
                    !m_WIAnalysis->isUniform(pResourceValue))
                {
                    /// here add it to some list of instructions to be reslooped
                    m_ResourceLoopCandidates.push_back(LI);
                    return;
                }
            }
        }
        else if (auto* SI = dyn_cast<SampleIntrinsic>(pIntr))
        {
            if (IGC_IS_FLAG_DISABLED(DisableResourceLoopUnrollNestedSampler))
            {
                // call help function
                llvm::Value* pPairedTextureValue = nullptr;
                llvm::Value* pTextureValue = nullptr;
                llvm::Value* pSamplerValue = nullptr;
                IGC::getTextureAndSamplerOperands(
                    pIntr,
                    pPairedTextureValue,
                    pTextureValue,
                    pSamplerValue);

                if ((pSamplerValue && !m_WIAnalysis->isUniform(pSamplerValue) && pSamplerValue->getType()->isPointerTy()) ||
                    (pTextureValue && !m_WIAnalysis->isUniform(pTextureValue) && pTextureValue->getType()->isPointerTy()) ||
                    (pPairedTextureValue && !m_WIAnalysis->isUniform(pPairedTextureValue) && pPairedTextureValue->getType()->isPointerTy()))
                {
                    /// here add it to some list of instructions to be reslooped
                    m_ResourceLoopCandidates.push_back(SI);
                    return;
                }
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////
bool ResourceLoopUnroll::ProcessFunction(llvm::Function& F)
{
    visit(F);

    bool bModified = false;

    for( llvm::CallInst* CI : m_ResourceLoopCandidates )
    {
        bModified = emitResourceLoop( CI );
    }
    return bModified;
}

////////////////////////////////////////////////////////////////////////
void ResourceLoopUnroll::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<WIAnalysis>();
}


////////////////////////////////////////////////////////////////////////
llvm::Pass* createResourceLoopUnroll()
{
    return new ResourceLoopUnroll();
}

}

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-resource-loop-unroll"
#define PASS_DESCRIPTION "ResourceLoopUnroll"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ResourceLoopUnroll, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ResourceLoopUnroll, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
