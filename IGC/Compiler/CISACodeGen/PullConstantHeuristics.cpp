/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PullConstantHeuristics.hpp"

#include "Platform.hpp"

using namespace llvm;
using namespace IGC;


static const unsigned EUInstCycleCount = 2;
static const unsigned SendInstCycleCount = 190;
static const unsigned RTWriteInstCycleCount = 190;

char PullConstantHeuristics::ID = 0;

#define PASS_FLAG "Analyse shader to determine push const threshold"
#define PASS_DESCRIPTION "This is an analysis pass for pulling constants for short shaders "
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(PullConstantHeuristics, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(PullConstantHeuristics, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)


//needs to be fine-tuned after obtaining feedback from visa
static unsigned estimateShaderLifetime(unsigned int EUCnt, unsigned SendMsgCnt, unsigned RTWriteCnt)
{
    return EUCnt * EUInstCycleCount + SendMsgCnt * SendInstCycleCount + RTWriteCnt * RTWriteInstCycleCount;
}

static bool isSendMessage(GenIntrinsicInst* inst)
{
    if (isSampleInstruction(inst) || isSampleLoadGather4InfoInstruction(inst))
    {
        return true;
    }
    return false;
}

//approximating EU insts count - TODO: need a better way to do this
static unsigned getEUInstEstimate(Instruction* inst)
{
    //handle ALU, Logical, and load-store insts
    //Presently we're restricting to shaders with 1 BB only, i.e, short shaders
    //But this handles branch insts as well (in case of multiple BB support in future)
    if (inst->getOpcode() <= Instruction::Fence)
    {
        return 1;
    }

    //handling remaining inst types
    switch (inst->getOpcode())
    {
    case Instruction::FCmp:
    case Instruction::ICmp:
    case Instruction::Select:
    case Instruction::Ret:
        return 1;
    default:
        //bitcast insts don't make an EU inst in visa
        return 0;
    }
}

//estimate EU, SendMsg and RTWrite insts required by the PS
static std::tuple<unsigned, unsigned, unsigned> getInstStats(const Function& F) {
    unsigned EUInstCnt = 0;
    unsigned int SendMsgInstCnt = 0;
    unsigned int RTWriteInstCnt = 0;
    for (auto BBI = F.getBasicBlockList().begin(); BBI != F.getBasicBlockList().end(); BBI++)
    {
        llvm::BasicBlock* BB = const_cast<llvm::BasicBlock*>(&*BBI);
        for (auto II = BB->begin(); II != BB->end(); II++)
        {
            if (llvm::GenIntrinsicInst * pIntrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(II))
            {
                if (isSendMessage(pIntrinsic))
                    SendMsgInstCnt++;
                else if (pIntrinsic->getIntrinsicID() == GenISAIntrinsic::GenISA_RTWrite)
                    RTWriteInstCnt++;
            }
            else
            {
                EUInstCnt += getEUInstEstimate(&*II);
            }
        }
    }
    return std::make_tuple(EUInstCnt, SendMsgInstCnt, RTWriteInstCnt);
}

//Pixel Shader Dispatch can be bottleneck if
//    thread_payload_size > max(simd4_sample_instr, simd4_eu_instr / 16, simd4_rt_write * 2, shader_lifetime / 56)
unsigned int PullConstantHeuristics::getPSDBottleNeckThreshold(const Function& F)
{
    CodeGenContext* ctx = nullptr;
    ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    const unsigned numThreadsPerSubslice = ctx->platform.getMaxNumberThreadPerSubslice(); //read from ctx.platform
    const unsigned roofLineIPC = 16;
    const unsigned pixelRate = 2;

    unsigned EstimatedEUInstCnt = 0;
    unsigned SendMsgInstCnt = 0;
    unsigned RTWriteInstCnt = 0;
    std::tie(EstimatedEUInstCnt, SendMsgInstCnt, RTWriteInstCnt) = getInstStats(F);

    unsigned shaderLifetime = estimateShaderLifetime(EstimatedEUInstCnt, SendMsgInstCnt, RTWriteInstCnt);

    //calculate payload size threshold assuming SIMD16 to stop pushing constants
    unsigned SIMD4_EU_Cnt = EstimatedEUInstCnt * 4;
    unsigned SIMD4_Sample_Cnt = SendMsgInstCnt * 4;
    unsigned SIMD4_RT_Write_Cnt = RTWriteInstCnt * 4;
    unsigned payloadThreshold = std::max(std::max(SIMD4_Sample_Cnt, SIMD4_EU_Cnt / roofLineIPC),
        std::max(SIMD4_RT_Write_Cnt * pixelRate, shaderLifetime / numThreadsPerSubslice));
    return payloadThreshold;
}

//currentPayloadSize = payloadHeaderSIMD16 + payloadBarySIMD16 + inputSize;
//
static unsigned getCurrentPayloadSizeEstimate(const Function& F)
{
    unsigned payloadHeaderSIMD16 = 3;
    unsigned payloadBarySIMD16 = 4;
    unsigned inputGRFSize = 1;

    //helper variables
    unsigned maxValueFromInputVec = 0;
    std::set<unsigned> countOfDifferentBary;
    for (auto BBI = F.getBasicBlockList().begin(); BBI != F.getBasicBlockList().end(); BBI++)
    {
        llvm::BasicBlock* BB = const_cast<llvm::BasicBlock*>(&*BBI);
        for (auto II = BB->begin(); II != BB->end(); II++)
        {
            if (llvm::GenIntrinsicInst * pIntrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(II))
            {
                if (pIntrinsic->getIntrinsicID() == GenISAIntrinsic::GenISA_DCL_inputVec)
                {
                    countOfDifferentBary.insert((unsigned)llvm::cast<llvm::ConstantInt>(II->getOperand(1))->getZExtValue());
                    maxValueFromInputVec = std::max(maxValueFromInputVec, (unsigned)llvm::cast<llvm::ConstantInt>(II->getOperand(0))->getZExtValue());
                }
            }
        }
    }
    payloadBarySIMD16 = countOfDifferentBary.size() * 4;
    inputGRFSize = (unsigned)(maxValueFromInputVec / 2) % 2 == 0 ? (maxValueFromInputVec / 2) : ((maxValueFromInputVec / 2) + 1);
    return payloadHeaderSIMD16 + payloadBarySIMD16 + inputGRFSize;
}

bool PullConstantHeuristics::runOnModule(Module& M)
{
    if (IGC_IS_FLAG_ENABLED(DisablePullConstantHeuristics))
    {
        return false;
    }
    CodeGenContext* ctx = nullptr;
    ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (ctx->type == ShaderType::PIXEL_SHADER && ctx->platform.hasPSDBottleneck())
    {
        for (auto& F : M)
        {
            if (F.getBasicBlockList().size() == 1)
            {
                BasicBlock* BB = &(*F.begin());
                if (BB->getInstList().size() < 200)
                {//short shaders
                    int PSDBottleNeckThreshold = getPSDBottleNeckThreshold(F);
                    int PayloadWithoutConstants = getCurrentPayloadSizeEstimate(F);
                    int maxPayload_Regkey = (IGC_GET_FLAG_VALUE(PayloadSizeThreshold));
                    PSDBottleNeckThreshold = PSDBottleNeckThreshold > maxPayload_Regkey ?
                        maxPayload_Regkey : PSDBottleNeckThreshold;

                    int threshold = PSDBottleNeckThreshold - PayloadWithoutConstants;
                    threshold = threshold < 0 ? 0 : threshold;
                    thresholdMap.insert(std::make_pair(&F, threshold));
                }
            }
        }

    }
    return false;//does not modify IR
}
