/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMUtils.h"
#include "common/IGCIRBuilder.h"
#include "PixelShaderLowering.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

using namespace llvm;

//#define DEBUG_BLEND_TO_DISCARD

namespace IGC
{

#define PASS_FLAG "igc-pixel-shader-addmask"
#define PASS_DESCRIPTION "Pixel shader add mask pass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
    IGC_INITIALIZE_PASS_BEGIN(PixelShaderAddMask, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_END(PixelShaderAddMask, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS

    char PixelShaderAddMask::ID = 0;

PixelShaderAddMask::PixelShaderAddMask() :
    FunctionPass(ID)
{
    initializePixelShaderAddMaskPass(*PassRegistry::getPassRegistry());
}

bool PixelShaderAddMask::runOnFunction(llvm::Function& F)
{
    m_cgCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    Module* mod = F.getParent();
    bool hasDiscard;

    hasDiscard = (mod->getNamedMetadata("KillPixel") != nullptr);
    m_modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    IGCMD::MetaDataUtils* pMdUtils =
        getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    if (!hasDiscard || pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
    {
        return false;
    }

    Instruction* globalMask = nullptr;
    Instruction* updateMask = nullptr;

    unsigned numUpdateMask = 0;

    for (auto BI = F.begin(), BE = F.end(); BI != BE; BI++)
    {
        for (auto II = BI->begin(), IE = BI->end(); II != IE; II++)
        {
            if (isa<GenIntrinsicInst>(II, GenISAIntrinsic::GenISA_InitDiscardMask))
            {
                globalMask = &(*II);
            }
            else
                if (isa<GenIntrinsicInst>(II, GenISAIntrinsic::GenISA_UpdateDiscardMask))
                {
                    numUpdateMask++;
                    updateMask = &(*II);
                }
        }
    }
    if (!globalMask)
    {
        return false;
    }

    if (F.size() == 1 && numUpdateMask == 1)
    {
        // handle special case function has 1 BB and 1 discard, then we
        // can directly use the discard condition for RTWrite, no need to
        // generate GetPixelMask.
        Value* discardCond = updateMask->getOperand(1);
        updateMask->eraseFromParent();
        globalMask->eraseFromParent();
        Value* mask = nullptr;

        for (auto BI = F.begin(), BE = F.end(); BI != BE; BI++)
        {
            RTWritIntrinsic* rtw;
            RTDualBlendSourceIntrinsic* drt;

            for (auto II = BI->begin(), IE = BI->end(); II != IE; II++)
            {
                if ((rtw = dyn_cast<RTWritIntrinsic>(II)))
                {
                    IGC_ASSERT(isa<ConstantInt>(rtw->getPMask()));
                    if (!mask)
                    {
                        mask = BinaryOperator::CreateNot(discardCond, "", rtw);
                    }

                    rtw->setPMask(mask);
                }
                else
                    if ((drt = dyn_cast<RTDualBlendSourceIntrinsic>(II)))
                    {
                        IGC_ASSERT(isa<ConstantInt>(drt->getPMask()));
                        if (!mask)
                        {
                            mask = BinaryOperator::CreateNot(discardCond, "", drt);
                        }

                        drt->setPMask(mask);
                    }
            }
        }
    }
    else
    {
        globalMask->moveBefore(globalMask->getParent()->getFirstNonPHI());

        Function* getMaskF;
        getMaskF = GenISAIntrinsic::getDeclaration(mod,
            GenISAIntrinsic::GenISA_GetPixelMask);

        Value* mask = nullptr;

        for (auto BI = F.begin(), BE = F.end(); BI != BE; BI++)
        {
            RTWritIntrinsic* rtw;
            RTDualBlendSourceIntrinsic* drt;

            mask = nullptr;
            for (auto II = BI->begin(), IE = BI->end(); II != IE; II++)
            {
                if ((rtw = dyn_cast<RTWritIntrinsic>(II)) && globalMask)
                {
                    if (!mask)
                    {
                        mask = CallInst::Create(getMaskF, { globalMask }, "", rtw);
                    }
                    IGC_ASSERT(isa<ConstantInt>(rtw->getPMask()));
                    rtw->setPMask(mask);
                }
                else
                    if ((drt = dyn_cast<RTDualBlendSourceIntrinsic>(II)) && globalMask)
                    {
                        if (!mask)
                        {
                            mask = CallInst::Create(getMaskF, { globalMask }, "", drt);
                        }
                        IGC_ASSERT(isa<ConstantInt>(drt->getPMask()));
                        drt->setPMask(mask);
                    }
            }
        }
    }

    return false;
}

char PixelShaderLowering::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-pixel-shader-lowering"
#define PASS_DESCRIPTION "This is the pixel shader lowering pass "
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(PixelShaderLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
    IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
    IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
    IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
    IGC_INITIALIZE_PASS_END(PixelShaderLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS

    PixelShaderLowering::PixelShaderLowering() :
    FunctionPass(ID),
    m_module(nullptr),
    PDT(nullptr),
    m_ReturnBlock(nullptr),
    SkipSrc0Alpha(false),
    m_dualSrcBlendEnabled(false),
    uavPixelSync(false)
{
    initializePixelShaderLoweringPass(*PassRegistry::getPassRegistry());
}

bool PixelShaderLowering::runOnFunction(llvm::Function& F)
{
    m_cgCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    IGCMD::MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    if (!isEntryFunc(pMdUtils, &F))
    {
        return false;
    }
    m_modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

    for (llvm::Function::iterator bb = F.begin(), be = F.end(); bb != be; ++bb)
    {
        if (llvm::isa<llvm::ReturnInst>(bb->getTerminator()))
        {
            m_ReturnBlock = &(*bb);
            break;
        }
    }
    if (m_ReturnBlock == nullptr)
    {
        F.begin()->getTerminator()->eraseFromParent();
        ReturnInst::Create(F.getContext(), &(*F.begin()));
        m_ReturnBlock = &(*F.begin());
    }
    m_outputBlock = nullptr;

    m_module = F.getParent();
    ColorOutputArray colors;
    DebugLocArray debugLocs;
    Value* depth = nullptr;
    Value* mask = nullptr;
    Value* src0Alpha = nullptr;
    Value* stencil = nullptr;

    // src0Alphas need not be sent when renderTargetBlending metadata is disabled
    // this means alpha to coverage and alpha test is disabled
    // this also means the render target blending is disabled
    SkipSrc0Alpha = m_modMD->psInfo.SkipSrc0Alpha || IGC_IS_FLAG_ENABLED(ForceDisableSrc0Alpha);

    // Check whether metadata indicates that dual source blending should be disabled
    bool dualSourceBlendingDisabled =
        IGC_IS_FLAG_ENABLED(DisableDualBlendSource) ||
        m_modMD->psInfo.DualSourceBlendingDisabled;

    m_dualSrcBlendEnabled = !dualSourceBlendingDisabled;

    m_isPerSample = false;

    m_hasDiscard = (m_module->getNamedMetadata("KillPixel") != nullptr);

    // In case we are using intrinsic retrieve the output
    FindIntrinsicOutput(colors, depth, stencil, mask, src0Alpha, debugLocs);

    if (uavPixelSync)
    {
        // Emitting a fence to ensure that the uav write is completed before an EOT is issued
        IRBuilder<> builder(F.getContext());

        bool fenceFlushNone = IGC_GET_FLAG_VALUE(RovOpt) & 1;
        EmitMemoryFence(builder, fenceFlushNone);
    }

    // EmitRender target write intrinsic
    EmitRTWrite(colors, depth, stencil, mask, src0Alpha, debugLocs);

    Function* pixelPhase = nullptr;
    Function* coarsePhase = nullptr;
    NamedMDNode* coarseNode = F.getParent()->getNamedMetadata(NAMED_METADATA_COARSE_PHASE);
    NamedMDNode* pixelNode = F.getParent()->getNamedMetadata(NAMED_METADATA_PIXEL_PHASE);
    bool cfgChanged = false;
    if (coarseNode)
    {
        coarsePhase = mdconst::dyn_extract<Function>(coarseNode->getOperand(0)->getOperand(0));
    }
    if (pixelNode)
    {
        pixelPhase = mdconst::dyn_extract<Function>(pixelNode->getOperand(0)->getOperand(0));
    }

    if (&F == coarsePhase && pixelPhase != nullptr && mask != nullptr)
    {
        EmitCoarseMask(mask);
    }
    return cfgChanged;
}

void PixelShaderLowering::FindIntrinsicOutput(
    ColorOutputArray& colors,
    Value*& depth,
    Value*& stencil,
    Value*& mask,
    Value*& src0Alpha,
    DebugLocArray& debugLocs)
{
    constexpr uint cMaxInputs = 32;
    constexpr uint cMaxInputComponents = cMaxInputs * 4;
    std::bitset<cMaxInputComponents> inputComponentsUsed;
    std::bitset<cMaxInputs> isLinearInterpolation;

    llvm::Instruction* primId = nullptr;
    llvm::Instruction* pointCoordX = nullptr;
    llvm::Instruction* pointCoordY = nullptr;
    SmallVector<GenIntrinsicInst*, 4> outputInstructions;
    SmallVector<Instruction*, 4> instructionToRemove;
    Function& F = *m_ReturnBlock->getParent();
    Value* btrue = llvm::ConstantInt::get(Type::getInt1Ty(m_module->getContext()), true);

    m_modMD->psInfo.colorOutputMask.resize(USC::NUM_PSHADER_OUTPUT_REGISTERS);

    for (auto BI = F.begin(), BE = F.end(); BI != BE; BI++)
    {
        for (auto II = BI->begin(), IE = BI->end(); II != IE; II++)
        {
            if (GenIntrinsicInst * inst = dyn_cast<GenIntrinsicInst>(II))
            {
                GenISAIntrinsic::ID IID = inst->getIntrinsicID();
                if (IID == GenISAIntrinsic::GenISA_uavSerializeAll ||
                    IID == GenISAIntrinsic::GenISA_uavSerializeOnResID)
                {
                    uavPixelSync = true;
                }
                else if (IID == GenISAIntrinsic::GenISA_OUTPUT)
                {
                    m_outputBlock = inst->getParent();
                    outputInstructions.push_back(inst);
                    uint outputType = (uint)llvm::cast<llvm::ConstantInt>(inst->getOperand(4))->getZExtValue();
                    IGC_ASSERT(outputType == SHADER_OUTPUT_TYPE_DEFAULT ||
                        outputType == SHADER_OUTPUT_TYPE_DEPTHOUT ||
                        outputType == SHADER_OUTPUT_TYPE_STENCIL ||
                        outputType == SHADER_OUTPUT_TYPE_OMASK);

                    //Need to save debug location
                    debugLocs.push_back(((Instruction*)inst)->getDebugLoc());

                    // delete the output
                    instructionToRemove.push_back(inst);
                }
                else if (IID == GenISAIntrinsic::GenISA_DCL_SystemValue)
                {
                    SGVUsage usage = (SGVUsage)
                        llvm::cast<llvm::ConstantInt>(inst->getOperand(0))->getZExtValue();
                    if (usage == PRIMITIVEID)
                    {
                        primId = inst;
                    }
                    else if (usage == POINT_COORD_X)
                    {
                        pointCoordX = inst;
                    }
                    else if (usage == POINT_COORD_Y)
                    {
                        pointCoordY = inst;
                    }
                    else if (usage == POSITION_X || usage == POSITION_Y)
                    {
                        LowerPositionInput(inst, usage);
                    }
                    else if (usage == SAMPLEINDEX)
                    {
                        m_isPerSample = true;
                    }
                }
                else if (IID == GenISAIntrinsic::GenISA_DCL_inputVec)
                {
                    uint setupIndex =
                        (uint)llvm::cast<llvm::ConstantInt>(inst->getOperand(0))->getZExtValue();

                    IGC_ASSERT_MESSAGE(setupIndex < cMaxInputComponents, "Max inputs cannot be greater than 32 x 4");
                    inputComponentsUsed.set(setupIndex);

                    e_interpolation mode = (e_interpolation)
                        llvm::cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue();
                    switch (mode)
                    {
                    case EINTERPOLATION_CONSTANT:
                        IGC_ASSERT(!isLinearInterpolation.test(setupIndex / 4));
                        break;
                    case EINTERPOLATION_LINEARSAMPLE:
                    case EINTERPOLATION_LINEARNOPERSPECTIVESAMPLE:
                        m_isPerSample = true;
                        // fall through
                    case EINTERPOLATION_LINEAR:
                    case EINTERPOLATION_LINEARCENTROID:
                    case EINTERPOLATION_LINEARNOPERSPECTIVE:
                    case EINTERPOLATION_LINEARNOPERSPECTIVECENTROID:
                        isLinearInterpolation.set(setupIndex / 4);
                        break;
                    case EINTERPOLATION_UNDEFINED:
                    case EINTERPOLATION_VERTEX:
                    default:
                        IGC_ASSERT_MESSAGE(0, "Unexpected Pixel Shader input interpolation mode.");
                    }
                }
            }
        }
    }
    if (primId)
    {
        // When PrimitiveId input is present in shader IGC allocates an additional input and returns
        // information about the PrimitiveID input to UMD (to program SBE). This new input component
        // is created with constant interpolation and cannot be placed in a (4-dword) location that
        // has linearly interpolated components. Alernatively code in MarkConstantInterpolation()
        // could be modified to ignore the additional input created for PrimitveID.
        unsigned int location;
        for (location = 0; location < cMaxInputComponents; location++)
        {
            bool isLocationSuitable = inputComponentsUsed.test(location) == false &&
                isLinearInterpolation.test(location / 4) == false;
            if (isLocationSuitable)
            {
                break;
            }
        }
        Value* arguments[] =
        {
            ConstantInt::get(Type::getInt32Ty(m_module->getContext()), location),
            ConstantInt::get(Type::getInt32Ty(m_module->getContext()), EINTERPOLATION_CONSTANT),
        };
        CallInst* in = GenIntrinsicInst::Create(
            GenISAIntrinsic::getDeclaration(
                m_module,
                GenISAIntrinsic::GenISA_DCL_inputVec,
                Type::getFloatTy(m_module->getContext())),
            arguments,
            "",
            primId);
        in->setDebugLoc(primId->getDebugLoc());
        primId->replaceAllUsesWith(in);
        NamedMDNode* primIdMD = m_module->getOrInsertNamedMetadata("PrimIdLocation");

        Constant* cval = ConstantInt::get(
            Type::getInt32Ty(m_module->getContext()), location);
        llvm::MDNode* locationNd = llvm::MDNode::get(
            m_module->getContext(),
            ConstantAsMetadata::get(cval));
        primIdMD->addOperand(locationNd);
    }
    if (pointCoordX || pointCoordY)
    {
        // Although PointCoords needs only 2 DWORDs, IGC must allocate 4 additional input and returns
        // information about the PointCoord input to UMD (to program SBE). These new input components
        // are created with linear interpolation and must be placed in an empty attribute index (4 DWORDs).
        unsigned int location;
        for (location = 0; location < cMaxInputComponents; location += 4)
        {
            bool isAttributeIndexEmpty =
                inputComponentsUsed.test(location) == false &&
                inputComponentsUsed.test(location + 1) == false &&
                inputComponentsUsed.test(location + 2) == false &&
                inputComponentsUsed.test(location + 3) == false;
            if (isAttributeIndexEmpty)
            {
                isLinearInterpolation.set(location / 4);
                break;
            }
        }
        IGC_ASSERT(location < cMaxInputComponents);

        llvm::Instruction* inputPointCoords[] = { pointCoordX, pointCoordY };
        for (unsigned int i = 0; i < sizeof(inputPointCoords) / sizeof(inputPointCoords[0]); i++)
        {
            if (inputPointCoords[i] == nullptr)
            {
                continue;
            }
            Value* arguments[] =
            {
                ConstantInt::get(Type::getInt32Ty(m_module->getContext()), location + i),
                ConstantInt::get(Type::getInt32Ty(m_module->getContext()), EINTERPOLATION_LINEAR),
            };
            CallInst* in = GenIntrinsicInst::Create(
                GenISAIntrinsic::getDeclaration(
                    m_module,
                    GenISAIntrinsic::GenISA_DCL_inputVec,
                    Type::getFloatTy(m_module->getContext())),
                arguments,
                "",
                inputPointCoords[i]);
            in->setDebugLoc(inputPointCoords[i]->getDebugLoc());
            inputPointCoords[i]->replaceAllUsesWith(in);
            instructionToRemove.push_back(inputPointCoords[i]);
        }

        NamedMDNode* PointCoordMD = m_module->getOrInsertNamedMetadata("PointCoordLocation");
        Constant* cval = ConstantInt::get(
            Type::getInt32Ty(m_module->getContext()), location);
        llvm::MDNode* locationNd = llvm::MDNode::get(
            m_module->getContext(),
            ConstantAsMetadata::get(cval));
        PointCoordMD->addOperand(locationNd);

    }
    for (GenIntrinsicInst* pInst : outputInstructions)
    {
        uint outputType = (uint)llvm::cast<llvm::ConstantInt>(pInst->getOperand(4))->getZExtValue();
        if (outputType == SHADER_OUTPUT_TYPE_DEFAULT)
        {
            uint RTIndex = (uint)llvm::cast<llvm::ConstantInt>(pInst->getOperand(5))->getZExtValue();

            unsigned mask = 0;
            // if any of the color channel is undef, initialize it
            // to 0 for color compression perf.
            for (int i = 0; i < 4; i++)
            {
                if (isa<UndefValue>(pInst->getOperand(i)))
                {
                    if (i == 3 &&
                        IGC_IS_FLAG_ENABLED(EnableUndefAlphaOutputAsRed))
                    {
                        // if it's alpha, then set default value to
                        // color.r, see IGC-959.
                        pInst->setOperand(i, pInst->getOperand(0));
                    }
                    else
                    {
                        pInst->setOperand(i,
                            ConstantFP::get(pInst->getOperand(i)->getType(), 0.0f));
                    }
                }
                else
                {
                    mask |= 1 << i;
                }
            }
            if (RTIndex == 0)
            {
                src0Alpha = pInst->getOperand(3);
            }
            m_modMD->psInfo.colorOutputMask[RTIndex] = mask;
            ColorOutput data;
            data.RTindex = RTIndex;
            data.color[0] = pInst->getOperand(0);
            data.color[1] = pInst->getOperand(1);
            data.color[2] = pInst->getOperand(2);
            data.color[3] = pInst->getOperand(3);
            data.mask = btrue;
            data.blendStateIndex = nullptr;
            data.bb = pInst->getParent();
            colors.push_back(data);
        }
        else if (outputType == SHADER_OUTPUT_TYPE_DEPTHOUT)
        {
            depth = pInst->getOperand(0);
        }
        else if (outputType == SHADER_OUTPUT_TYPE_STENCIL)
        {
            stencil = pInst->getOperand(0);
        }
        else if (outputType == SHADER_OUTPUT_TYPE_OMASK)
        {
            mask = pInst->getOperand(0);
        }
    }
    for (unsigned int i = 0; i < instructionToRemove.size(); i++)
    {
        instructionToRemove[i]->eraseFromParent();
    }
}

void PixelShaderLowering::EmitMemoryFence(IRBuilder<>& builder, bool forceFlushNone)
{
    Value* trueValue = builder.getInt1(true);
    Value* falseValue = builder.getInt1(false);

    Value* arguments[] =
    {
        trueValue,
        falseValue,
        falseValue,
        falseValue,
        falseValue,
        trueValue,
        falseValue,
    };

    CallInst* memFence = GenIntrinsicInst::Create(GenISAIntrinsic::getDeclaration(m_module, GenISAIntrinsic::GenISA_memoryfence),
        arguments,
        "",
        m_ReturnBlock->getTerminator());

    if (forceFlushNone && m_cgCtx->platform.hasLSC() && (IGC_GET_FLAG_VALUE(RovOpt) & 1))
    {
        MDNode* indM = MDNode::get(memFence->getContext(), ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(memFence->getContext()), 1)));
        memFence->setMetadata("forceFlushNone", indM);
    }
}

CallInst* PixelShaderLowering::addRTWrite(
    BasicBlock* bbToAdd, Value* src0Alpha,
    Value* oMask, ColorOutput& color,
    Value* depth, Value* stencil)
{
    bool isHF = false;
    Value* undefSrc0Alpha = nullptr;
    Value* r = color.color[0];
    Value* g = color.color[1];
    Value* b = color.color[2];
    Value* a = color.color[3];

    //True if src0Alpha exists and renderTargetBlendingDisabled is false
    bool needsSrc0Alpha = ((src0Alpha && color.RTindex > 0) && (!SkipSrc0Alpha) && src0Alpha != color.color[3]);
    bool src0AlphaIsHF = (needsSrc0Alpha && isa<FPExtInst>(src0Alpha)) || !needsSrc0Alpha;

    if (m_cgCtx->platform.supportFP16() &&
        (llvm::isa<llvm::FPExtInst>(r) &&
            llvm::isa<llvm::FPExtInst>(g) &&
            llvm::isa<llvm::FPExtInst>(b) &&
            llvm::isa<llvm::FPExtInst>(a)) &&
        src0AlphaIsHF &&
        !SkipSrc0Alpha)
    {

        FPExtInst* rInst = llvm::cast<llvm::FPExtInst>(r);
        FPExtInst* gInst = llvm::cast<llvm::FPExtInst>(g);
        FPExtInst* bInst = llvm::cast<llvm::FPExtInst>(b);
        FPExtInst* aInst = llvm::cast<llvm::FPExtInst>(a);
        FPExtInst* src0AlphaInst = nullptr;

        if (needsSrc0Alpha &&
            llvm::isa<llvm::FPExtInst>(src0Alpha))
            src0AlphaInst = llvm::cast<llvm::FPExtInst>(src0Alpha);

        r = rInst->getOperand(0);

        g = gInst->getOperand(0);

        b = bInst->getOperand(0);

        a = aInst->getOperand(0);

        if (src0AlphaInst)
        {
            src0Alpha = src0AlphaInst->getOperand(0);
        }
        isHF = true;
    }

    if (r->getType()->isHalfTy())
    {
        isHF = true;
    }

    /*
        In case src0Alpha comes from a HF RT Write
        */
    IRBuilder<> builder(bbToAdd->getTerminator());
    if (!isHF &&
        needsSrc0Alpha &&
        src0Alpha->getType()->isHalfTy())
    {
        if (llvm::isa<llvm::FPTruncInst>(src0Alpha))
        {
            src0Alpha = (llvm::cast<llvm::FPTruncInst>(src0Alpha))->getOperand(0);
        }
        else
        {
            src0Alpha = builder.CreateFPExt(src0Alpha, builder.getFloatTy());
        }
    }
    else if (isHF &&
        needsSrc0Alpha &&
        src0Alpha->getType()->isFloatTy())
    {
        /*
            reverse, src0Alpha comes from half float in to float RT Write
            */
        if (llvm::isa<llvm::FPExtInst>(src0Alpha))
        {
            src0Alpha = (llvm::cast<llvm::FPExtInst>(src0Alpha))->getOperand(0);
        }
        else
        {
            src0Alpha = builder.CreateFPTrunc(src0Alpha, llvm::Type::getHalfTy(m_module->getContext()));
        }
    }

    if (isHF)
        undefSrc0Alpha = llvm::UndefValue::get(Type::getHalfTy(m_module->getContext()));
    else
        undefSrc0Alpha = llvm::UndefValue::get(Type::getFloatTy(m_module->getContext()));

    Type* i32t = Type::getInt32Ty(m_module->getContext());
    Type* i1t = Type::getInt1Ty(m_module->getContext());
    Value* undef = llvm::UndefValue::get(Type::getFloatTy(m_module->getContext()));
    Value* iundef = llvm::UndefValue::get(i32t);
    Value* i1true = ConstantInt::get(i1t, 1);
    Value* i1false = ConstantInt::get(i1t, 0);
    Value* vrtIdx = ConstantInt::get(i32t, color.RTindex);
    Value* vblendIdx = color.blendStateIndex ? color.blendStateIndex : vrtIdx;
    Value* hasOmask = (oMask || m_modMD->psInfo.outputMask) ? i1true : i1false;
    Value* hasDepth = (depth || m_modMD->psInfo.outputDepth) ? i1true : i1false;
    Value* hasStencil = (stencil || m_modMD->psInfo.outputStencil) ? i1true : i1false;

    Value* arguments[] = {
        needsSrc0Alpha ? src0Alpha : undefSrc0Alpha,    // 0
        oMask ? oMask : undef,                          // 1 - oMask
        color.mask,                                     // 2 - pMask
        r, g, b, a,                                     // 3,4,5,6
        depth ? depth : undef,                          // 7
        stencil ? stencil : undef,                      // 8
        vrtIdx,                                         // 9 - RT index
        vblendIdx,                                      // 10 - blend state index
        hasOmask,                                       // 11
        hasDepth,                                       // 12
        hasStencil,                                     // 13
        i1false,                                        // 14 - per sample
        iundef                                          // 15 - sample idx
    };

    Function* frtw;

    if (isHF)
    {
        frtw = GenISAIntrinsic::getDeclaration(m_module,
            GenISAIntrinsic::GenISA_RTWrite,
            Type::getHalfTy(this->m_module->getContext()));
    }
    else
    {
        frtw = GenISAIntrinsic::getDeclaration(m_module,
            GenISAIntrinsic::GenISA_RTWrite,
            Type::getFloatTy(this->m_module->getContext()));
    }

    return GenIntrinsicInst::Create(frtw, arguments, "",
        bbToAdd->getTerminator());
}

#ifdef DEBUG_BLEND_TO_DISCARD
// debug function
static void dbgPrintBlendOptMode(uint64_t hash,
    std::vector<int>& blendOpt, unsigned ncolors)
{
    static const char* blendOptName[] =
    {
        "BLEND_OPTIMIZATION_NONE",
        "BLEND_OPTIMIZATION_SRC_ALPHA",
        "BLEND_OPTIMIZATION_INV_SRC_ALPHA",
        "BLEND_OPTIMIZATION_SRC_ALPHA_DISCARD_ONLY",
        "BLEND_OPTIMIZATION_SRC_ALPHA_FILL_ONLY",
        "BLEND_OPTIMIZATION_SRC_COLOR_ZERO",
        "BLEND_OPTIMIZATION_SRC_COLOR_ONE",
        "BLEND_OPTIMIZATION_SRC_BOTH_ZERO",
        "BLEND_OPTIMIZATION_SRC_BOTH_ONE",
        "BLEND_OPTIMIZATION_SRC_ALPHA_OR_COLOR_ZERO",
        "BLEND_OPTIMIZATION_SRC_COLOR_ZERO_ALPHA_ONE",
        "BLEND_OPTIMIZATION_SRC_COLOR_ZERO_ALPHA_IGNORE"
    };
    bool doprint = false;
    for (unsigned i = 0; i < ncolors; i++)
    {
        if (blendOpt[i] != USC::BLEND_OPTIMIZATION_NONE)
            doprint = true;
    }
    if (doprint)
    {
        printf("%016llx blend opt[%d]:\n", hash, ncolors);
        for (unsigned i = 0; i < ncolors; i++)
        {
            printf("  %s\n", blendOptName[blendOpt[i]]);
        }
    }
}
#endif

void PixelShaderLowering::EmitRTWrite(
    ColorOutputArray& colors, Value* depth, Value* stencil,
    Value* oMask, Value* src0Alpha, DebugLocArray& debugLocs)
{
    if (!m_hasDiscard)
    {
        // no discard found
        //IGC_ASSERT(m_module->getNamedMetadata("KillPixel") == nullptr);

        // check blend to discard optimization and generate mask for each
        // render target output
        std::vector<int>& blendOpt = m_modMD->psInfo.blendOptimizationMode;
#ifdef DEBUG_BLEND_TO_DISCARD
        dbgPrintBlendOptMode(m_cgCtx->hash.getAsmHash(), blendOpt, colors.size());
#endif

        if (blendOpt.size() && !useDualSrcBlend(colors))
        {
            bool hasDiscard = false;

            unsigned maxRTIndex = 0;
            for (unsigned i = 0; i < colors.size(); i++)
            {
                if (maxRTIndex < colors[i].RTindex)
                {
                    maxRTIndex = colors[i].RTindex;
                }
            }

            for (unsigned i = 0; i < colors.size(); i++)
            {
                USC::BLEND_OPTIMIZATION_MODE blendOptMode =
                    static_cast<USC::BLEND_OPTIMIZATION_MODE>(blendOpt[i]);

                // Only do blend to fill if the shader is persample, hardware
                // already does blend to fill for other cases.
                bool enableBlendToFill =
                    m_cgCtx->m_DriverInfo.SupportBlendToFillOpt() &&
                    maxRTIndex <= 4 && m_isPerSample;

                if (optBlendState(blendOptMode, colors[i], enableBlendToFill))
                {
                    // for blend to discard opt, we need to force earlyz
                    hasDiscard = true;
                    m_modMD->psInfo.forceEarlyZ = true;
                }
            }

            if (hasDiscard)
            {
                m_module->getOrInsertNamedMetadata("KillPixel");
            }
        }
    }

    uint32_t RTindexVal = -1;
    //According to Spec, the RT Write instruction must follow this order : dual source followed by single source
    if (useDualSrcBlend(colors))
    {
        //If RT0 is executed first when size is 2
        if (colors[0].RTindex == 0 && colors[1].RTindex == 1)
        {
            RTindexVal = 0;
        }
        else if (colors[0].RTindex == 1 && colors[1].RTindex == 0)
        {

            RTindexVal = 1;
        }
    }

    if (RTindexVal != -1)
    {
        //dual source RTWrite first
        CallInst* dualSourceRTW = addDualBlendWrite(
            colors[RTindexVal].bb,
            oMask,
            colors[RTindexVal],
            colors[1 - RTindexVal],
            depth, stencil, 0);
        dualSourceRTW->setDebugLoc(debugLocs[RTindexVal]);

        if (needsSingleSourceRTWWithDualSrcBlend())
        {
            //Single source RTWrite
            CallInst* singleSourceRTW = addRTWrite(
                colors[1 - RTindexVal].bb,
                src0Alpha,
                oMask, colors[1 - RTindexVal],
                depth,
                stencil);
            singleSourceRTW->setDebugLoc(debugLocs[1 - RTindexVal]);
            colors[RTindexVal].inst = dualSourceRTW;
            colors[1 - RTindexVal].inst = singleSourceRTW;
        }
        else
        {
            colors[0].inst = dualSourceRTW;
        }
    }
    else
    {
        for (unsigned int i = 0; i < colors.size(); i++)
        {
            colors[i].inst = addRTWrite(
                colors[i].bb,
                src0Alpha,
                oMask, colors[i],
                depth,
                stencil);

            colors[i].inst->setDebugLoc(debugLocs[i]);
        }
    }

    // pick up 1 RTWrite and move it to return block, so we don't need to
    // generate an additional null surface write for EOT.
    if (m_hasDiscard)
    {
        moveRTWritesToReturnBlock(colors);
    }

    checkAndCreateNullRTWrite(oMask, depth, stencil);
}

inline Value* fixHFSource(IRBuilder<>& builder, Value* val)
{
    if (val->getType()->isFloatTy())
        return val;

    if (llvm::isa<llvm::FPTruncInst>(val))
    {
        return (llvm::cast<llvm::FPTruncInst>(val))->getOperand(0);
    }
    else
    {
        return builder.CreateFPExt(val, builder.getFloatTy());
    }
}

CallInst* PixelShaderLowering::addDualBlendWrite(
    BasicBlock* bbToAdd, Value* oMask,
    ColorOutput& color0, ColorOutput& color1,
    Value* depth, Value* stencil, uint index)
{
    bool isFP16 = false;
    bool isFP32 = false;
    Value* pMask = color0.mask;
    Value* r0 = color0.color[0];
    Value* g0 = color0.color[1];
    Value* b0 = color0.color[2];
    Value* a0 = color0.color[3];
    Value* r1 = color1.color[0];
    Value* g1 = color1.color[1];
    Value* b1 = color1.color[2];
    Value* a1 = color1.color[3];

    IGC_ASSERT(color0.mask == color1.mask);

    //assuming types are consistent
    if (r0->getType()->isHalfTy() ||
        r1->getType()->isHalfTy())
    {
        isFP16 = true;
    }

    if (r0->getType()->isFloatTy() ||
        r1->getType()->isFloatTy())
    {
        isFP32 = true;
    }

    /*
        if we are combining FP32 and FP16 RT writes
        promote everything to FP32
        Three Cases:
        Case 1) Immediate, extend to FP32 Immediate.
        Case 2) FP16 Not Immediate. Not result to FPTrunc. Add FPExt Instruction
        Case 3) FP16 Not Immediate. Result of FPTrunc. Use src of FPTrunc
    */
    if (isFP16 && isFP32)
    {
        IRBuilder<> builder(bbToAdd->getTerminator());
        r0 = fixHFSource(builder, r0);
        g0 = fixHFSource(builder, g0);
        b0 = fixHFSource(builder, b0);
        a0 = fixHFSource(builder, a0);
        r1 = fixHFSource(builder, r1);
        g1 = fixHFSource(builder, g1);
        b1 = fixHFSource(builder, b1);
        a1 = fixHFSource(builder, a1);
    }

    Type* i32t = Type::getInt32Ty(m_module->getContext());
    Type* i1t = Type::getInt1Ty(m_module->getContext());
    Value* undef = llvm::UndefValue::get(Type::getFloatTy(m_module->getContext()));
    Value* iundef = llvm::UndefValue::get(i32t);
    Value* i1true = ConstantInt::get(i1t, 1);
    Value* i1false = ConstantInt::get(i1t, 0);

    Value* arguments[] = {
        oMask ? oMask : undef,          // 0 - oMask
        pMask,                          // 1 - pMask
        r0, g0, b0, a0,                 // 2, 3, 4, 5
        r1, g1, b1, a1,                 // 6, 7, 8, 9
        depth ? depth : undef,          // 10
        stencil ? stencil : undef,      // 11
        ConstantInt::get(i32t, index),  // 12 - RT index
        oMask ? i1true : i1false,       // 13
        depth ? i1true : i1false,       // 14
        stencil ? i1true : i1false,     // 15
        i1false,                        // 16 - per sample
        iundef,                         // 17 - sample index
    };
    return GenIntrinsicInst::Create(
        GenISAIntrinsic::getDeclaration(m_module, GenISAIntrinsic::GenISA_RTDualBlendSource, r0->getType()),
        arguments,
        "",
        bbToAdd->getTerminator());
}

void PixelShaderLowering::EmitCoarseMask(llvm::Value* mask)
{
    Type* floatTy = Type::getFloatTy(m_module->getContext());
    Value* undef = llvm::UndefValue::get(floatTy);
    Value* oMaskType =
        ConstantInt::get(Type::getInt32Ty(m_module->getContext()), SHADER_OUTPUT_TYPE_OMASK);
    Value* zero = ConstantInt::get(Type::getInt32Ty(m_module->getContext()), 0);
    Value* arguments[] =
    {
        mask,
        undef,
        undef,
        undef,
        oMaskType,
        zero,
    };

    GenIntrinsicInst::Create(
        GenISAIntrinsic::getDeclaration(m_module, GenISAIntrinsic::GenISA_OUTPUT, floatTy),
        arguments,
        "",
        m_ReturnBlock->getTerminator());
}

void PixelShaderLowering::LowerPositionInput(GenIntrinsicInst* positionInstr, uint usage)
{
    IRBuilder<> builder(positionInstr);
    Function* positionIntr = GenISAIntrinsic::getDeclaration(m_module,
        usage == POSITION_X ? GenISAIntrinsic::GenISA_PixelPositionX : GenISAIntrinsic::GenISA_PixelPositionY);
    Value* intPosition = builder.CreateCall(positionIntr);
    Value* floatPosition = positionInstr;
    if (floatPosition->hasOneUse())
    {
        if (BinaryOperator * fadd = dyn_cast<BinaryOperator>(*floatPosition->user_begin()))
        {
            if (ConstantFP * cst = dyn_cast<ConstantFP>(fadd->getOperand(1)))
            {
                float constant = cst->getValueAPF().convertToFloat();
                if (constant >= 0.0f && constant < 1.f)
                {
                    floatPosition = fadd;
                }
            }
        }
    }
    if (floatPosition->hasOneUse())
    {
        Value* v = *floatPosition->user_begin();
        if (v->getType()->isIntegerTy(32) && (isa<FPToUIInst>(v) || isa<FPToSIInst>(v)))
        {
            for (auto UI = v->user_begin(), UE = v->user_end(); UI != UE;)
            {
                Value* use = *UI++;
                if (TruncInst * truncI = dyn_cast<TruncInst>(use))
                {
                    truncI->replaceAllUsesWith(builder.CreateZExtOrTrunc(intPosition, truncI->getType()));
                }
            }
            if (!v->user_empty())
            {
                v->replaceAllUsesWith(builder.CreateZExt(intPosition, v->getType()));
            }
            return;
        }
    }
    positionInstr->replaceAllUsesWith(builder.CreateUIToFP(intPosition, positionInstr->getType()));
}

// Based on blend state, check color output and discard them if possible.
bool PixelShaderLowering::optBlendState(
    USC::BLEND_OPTIMIZATION_MODE blendOpt,
    ColorOutput& colorOut,
    bool enableBlendToFill)
{
    Function* fBallot = GenISAIntrinsic::getDeclaration(m_module,
        GenISAIntrinsic::GenISA_WaveBallot);

    bool enableBlendToDiscard =
        IGC_IS_FLAG_ENABLED(EnableBlendToDiscard) &&
        m_cgCtx->platform.enableBlendToDiscardAndFill();
    enableBlendToFill = enableBlendToFill &&
        IGC_IS_FLAG_ENABLED(EnableBlendToFill) &&
        m_cgCtx->platform.enableBlendToDiscardAndFill();

    bool hasDiscard = false;

    if (m_modMD->psInfo.outputDepth || m_modMD->psInfo.outputStencil)
    {
        enableBlendToDiscard = false;
    }

    IGCIRBuilder<> irb(m_ReturnBlock->getTerminator());

    switch (blendOpt)
    {

    case USC::BLEND_OPTIMIZATION_SRC_ALPHA:
    {
        // discard: src.a == 0, fill: src.a == 1

        if (enableBlendToDiscard)
        {
            Constant* f0 = ConstantFP::get(colorOut.color[3]->getType(), 0.0);
            Value* ane0 = irb.CreateFCmpUNE(colorOut.color[3], f0);
            colorOut.mask = ane0;
            hasDiscard = true;
        }

        if (enableBlendToFill)
        {
            // ifany(src.a != 1.0) ? RTIndex : RTIndex + 4
            Constant* f1 = ConstantFP::get(colorOut.color[3]->getType(), 1.0);
            Value* ane1 = irb.CreateFCmpUNE(colorOut.color[3], f1);
            Value* ane1_ballot = irb.CreateCall(fBallot, { ane1, irb.getInt32(0) });
            Value* any = irb.CreateICmpNE(ane1_ballot, irb.getInt32(0));
            colorOut.blendStateIndex = irb.CreateSelect(any,
                irb.getInt32(colorOut.RTindex),
                irb.getInt32(colorOut.RTindex + 4));
            m_modMD->psInfo.blendToFillEnabled = true;
        }
        return hasDiscard;
    }

    case USC::BLEND_OPTIMIZATION_INV_SRC_ALPHA:
    {
        // discard: src.a == 1, fill: src.a == 0
        Constant* f1 = ConstantFP::get(colorOut.color[0]->getType(), 1.0);

        if (enableBlendToDiscard)
        {
            Value* ane1 = irb.CreateFCmpUNE(colorOut.color[3], f1);
            colorOut.mask = ane1;
            hasDiscard = true;
        }

        if (enableBlendToFill)
        {
            // ifall(src.a == 0) ? RTIndex + 4 : RTIndex
            // ifany(src.a != 0) ? RTIndex : RTIndex + 4
            Value* ai = irb.CreateBitCast(colorOut.color[3], irb.getInt32Ty());
            Value* ane0 = irb.CreateICmpNE(ai, irb.getInt32(0));
            Value* ane0_ballot = irb.CreateCall(fBallot, { ane0 });
            Value* any = irb.CreateICmpNE(ane0_ballot, irb.getInt32(0));
            colorOut.blendStateIndex = irb.CreateSelect(any,
                irb.getInt32(colorOut.RTindex),
                irb.getInt32(colorOut.RTindex + 4));
            m_modMD->psInfo.blendToFillEnabled = true;
        }
        return hasDiscard;
    }

    case USC::BLEND_OPTIMIZATION_SRC_ALPHA_DISCARD_ONLY:
    {
        // discard: src.a == 0
        if (enableBlendToDiscard)
        {
            Constant* f0 = ConstantFP::get(colorOut.color[3]->getType(), 0.0);
            Value* ane0 = irb.CreateFCmpUNE(colorOut.color[3], f0);
            colorOut.mask = ane0;
            hasDiscard = true;
        }
        return hasDiscard;
    }

    case USC::BLEND_OPTIMIZATION_SRC_ALPHA_FILL_ONLY:
    {
        // fill: src.a == 1
        if (enableBlendToFill)
        {
            // ifall(src.a == 1.0) ? RTIndex + 4 : RTIndex
            // ifany(src.a != 1.0) ? RTIndex : RTIndex + 4
            Constant* f1 = ConstantFP::get(colorOut.color[3]->getType(), 1.0);
            Value* ane1 = irb.CreateFCmpUNE(colorOut.color[3], f1);
            Value* ane1_ballot = irb.CreateCall(fBallot, { ane1 });
            Value* any = irb.CreateICmpNE(ane1_ballot, irb.getInt32(0));
            colorOut.blendStateIndex = irb.CreateSelect(any,
                irb.getInt32(colorOut.RTindex),
                irb.getInt32(colorOut.RTindex + 4));
            m_modMD->psInfo.blendToFillEnabled = true;
        }
        return false;
    }

    case USC::BLEND_OPTIMIZATION_SRC_COLOR_ZERO:
    {
        // discard: src.rgb == 0
        if (enableBlendToDiscard)
        {
            colorOut.mask = irb.CreateAnyValuesNotZero(colorOut.color, 3);
            hasDiscard = true;
        }
        return hasDiscard;
    }

    case USC::BLEND_OPTIMIZATION_SRC_COLOR_ONE:
    {
        // discard if src.rgb == 1
        if (enableBlendToDiscard)
        {
            ConstantFP* f1 = cast<ConstantFP>(
                ConstantFP::get(colorOut.color[0]->getType(), 1.0));

            Value* rne1 = fcmpUNEConst(irb, colorOut.color[0], f1);
            Value* gne1 = fcmpUNEConst(irb, colorOut.color[1], f1);
            Value* bne1 = fcmpUNEConst(irb, colorOut.color[2], f1);

            colorOut.mask = createOr(irb, bne1, createOr(irb, rne1, gne1));
            hasDiscard = true;
        }
        return hasDiscard;
    }

    case USC::BLEND_OPTIMIZATION_SRC_BOTH_ZERO:
    {
        // discard: src.rgba == 0
        if (enableBlendToDiscard)
        {
            colorOut.mask = irb.CreateAnyValuesNotZero(colorOut.color, 4);

            hasDiscard = true;
        }
        return hasDiscard;
    }

    case USC::BLEND_OPTIMIZATION_SRC_BOTH_ONE:
    {
        // discard if src.rgba == 1
        if (enableBlendToDiscard)
        {
            Constant* f1 = ConstantFP::get(colorOut.color[0]->getType(), 1.0);

            Value* rne1 = irb.CreateFCmpUNE(colorOut.color[0], f1);
            Value* gne1 = irb.CreateFCmpUNE(colorOut.color[1], f1);
            Value* bne1 = irb.CreateFCmpUNE(colorOut.color[2], f1);
            Value* ane1 = irb.CreateFCmpUNE(colorOut.color[3], f1);
            colorOut.mask = irb.CreateOr(ane1, irb.CreateOr(bne1, irb.CreateOr(rne1, gne1)));
            hasDiscard = true;
        }
        return hasDiscard;
    }

    case USC::BLEND_OPTIMIZATION_SRC_ALPHA_OR_COLOR_ZERO:
    {
        // discard: src.a == 0 || src.rgb == 0
        if (enableBlendToDiscard)
        {
            Value* a = colorOut.color[3];
            Constant* f0 = ConstantFP::get(a->getType(), 0.0);

            Value* ane0 = irb.CreateFCmpUNE(a, f0);

            Value* cne0 = irb.CreateAnyValuesNotZero(colorOut.color, 3);

            colorOut.mask = irb.CreateAnd(ane0, cne0);
            hasDiscard = true;
        }
        return hasDiscard;
    }

    case USC::BLEND_OPTIMIZATION_SRC_COLOR_ZERO_ALPHA_ONE:
    {
        // discard: src.rgb == 0 && src.a == 1
        // equivalently mask = (r|g|b != 0) || (a != 1)
        if (enableBlendToDiscard)
        {
            Value* cne0 = irb.CreateAnyValuesNotZero(colorOut.color, 3);

            Value* a = colorOut.color[3];
            Constant* f1 = ConstantFP::get(a->getType(), 1.0);
            Value* ane1 = irb.CreateFCmpUNE(a, f1);

            colorOut.mask = irb.CreateOr(cne0, ane1);
            hasDiscard = true;
        }

        return hasDiscard;
    }

    case USC::BLEND_OPTIMIZATION_SRC_COLOR_ZERO_ALPHA_IGNORE:
    {
        // Discard: src.rgb == 0 and don't compute src.a
        // equivalently mask = (r|g|b != 0)
        if (enableBlendToDiscard)
        {
            colorOut.mask = irb.CreateAnyValuesNotZero(colorOut.color, 3);
            hasDiscard = true;
        }

        // set output alpha as output.r, see IGC-959
        if (IGC_IS_FLAG_ENABLED(EnableUndefAlphaOutputAsRed))
        {
            colorOut.color[3] = colorOut.color[0];
        }
        else
        {
            colorOut.color[3] = ConstantFP::get(
                colorOut.color[3]->getType(), 0.0);
        }

        return hasDiscard;
    }

    default:
        return false;
    }
}

void PixelShaderLowering::moveRTWriteToBlock(
    CallInst* call, SmallVector<BasicBlock*, 8> & predBB, BasicBlock* toBB,
    llvm::DenseMap<llvm::Value*, llvm::PHINode*>& valueToPhiMap)
{
    unsigned numPredBB = predBB.size();
    if (numPredBB > 1)
    {
        for (unsigned i = 0; i < IGCLLVM::getNumArgOperands(call); i++)
        {
            if (Instruction * inst = dyn_cast<Instruction>(call->getArgOperand(i)))
            {
                auto it = valueToPhiMap.find(inst);
                if (it != valueToPhiMap.end())
                {
                    call->setArgOperand(i, it->second);
                    continue;
                }

                PHINode* phi = PHINode::Create(
                    inst->getType(), numPredBB, "", &(*toBB->begin()));
                valueToPhiMap[inst] = phi;
                for (unsigned j = 0; j < numPredBB; j++)
                {
                    Value* inVal;
                    if (predBB[j] == call->getParent())
                    {
                        inVal = inst;
                    }
                    else
                    {
                        inVal = UndefValue::get(inst->getType());
                    }
                    phi->addIncoming(inVal, predBB[j]);
                }
                call->setArgOperand(i, phi);
            }
        }
    }

    call->removeFromParent();
    call->insertBefore(toBB->getTerminator());
}

void PixelShaderLowering::moveRTWritesToReturnBlock(
    const ColorOutputArray& colors)
{
    if (colors.size())
    {
        IGC_ASSERT(colors[0].inst != nullptr);
        SmallVector<BasicBlock*, 8> predBB;
        DenseMap<Value*, PHINode*> valueToPhiMap;
        for (auto PI = pred_begin(m_ReturnBlock), PE = pred_end(m_ReturnBlock);
            PI != PE; ++PI)
        {
            predBB.push_back(*PI);
        }

        if (useDualSrcBlend(colors) &&
            needsSingleSourceRTWWithDualSrcBlend())
        {
            // For SIMD16 PS thread with two output colors must send
            // messages in the following sequence for each RT: SIMD8 dual
            // source RTW message (low); SIMD8 dual source RTW message
            // (high); SIMD16 single src RTW message with second color.
            CallInst* const dualSourceRTW =
                isa<RTDualBlendSourceIntrinsic>(colors[0].inst) ? colors[0].inst : colors[1].inst;
            CallInst* const singleSourceRTW =
                isa<RTDualBlendSourceIntrinsic>(colors[0].inst) ? colors[1].inst : colors[0].inst;

            IGC_ASSERT(isa<RTWritIntrinsic>(singleSourceRTW));
            IGC_ASSERT(isa<RTDualBlendSourceIntrinsic>(dualSourceRTW));

            moveRTWriteToBlock(dualSourceRTW, predBB, m_ReturnBlock, valueToPhiMap);
            moveRTWriteToBlock(singleSourceRTW, predBB, m_ReturnBlock, valueToPhiMap);
        }
        else
        {
            moveRTWriteToBlock(colors[0].inst, predBB, m_ReturnBlock, valueToPhiMap);
        }
    }
}

PHINode* PixelShaderLowering::createPhiForRTWrite(Value* val,
    smallvector<BasicBlock*, 8> & predBB, BasicBlock* toBB)
{
    PHINode* phi = PHINode::Create(
        val->getType(), predBB.size(), "", &(*toBB->begin()));
    for (auto* BB : predBB)
    {
        Value* inVal;
        if (BB == m_outputBlock)
            inVal = val;
        else
            inVal = UndefValue::get(val->getType());
        phi->addIncoming(inVal, BB);
    }
    return phi;
}

// create a null surface write in return block if there's no one
void PixelShaderLowering::checkAndCreateNullRTWrite(
    Value* oMask, Value* depth, Value* stencil)
{
    bool hasRTW = false;
    for (auto& I : *m_ReturnBlock)
    {
        if (isa<RTWritIntrinsic>(&I) ||
            isa<RTDualBlendSourceIntrinsic>(&I))
        {
            hasRTW = true;
            break;
        }
    }

    if (!hasRTW)
    {
        Value* undef = UndefValue::get(Type::getFloatTy(m_module->getContext()));
        ColorOutput color;
        color.color[0] = color.color[1] = color.color[2] = color.color[3] = undef;
        color.mask = ConstantInt::get(Type::getInt1Ty(m_module->getContext()), true);
        color.RTindex = -1;
        color.blendStateIndex = nullptr;

        if (m_outputBlock != m_ReturnBlock)
        {
            smallvector<BasicBlock*, 8> predBB;

            for (auto PI = pred_begin(m_ReturnBlock), PE = pred_end(m_ReturnBlock);
                PI != PE; ++PI)
            {
                predBB.push_back(*PI);
            }
            if (predBB.size() > 1)
            {
                if (oMask)
                {
                    oMask = createPhiForRTWrite(oMask, predBB, m_ReturnBlock);
                }
                if (depth)
                {
                    depth = createPhiForRTWrite(depth, predBB, m_ReturnBlock);
                }
                if (stencil)
                {
                    stencil = createPhiForRTWrite(stencil, predBB, m_ReturnBlock);
                }
            }
        }
        addRTWrite(
            m_ReturnBlock,
            undef,
            oMask, color,
            depth, stencil);
    }
}

bool PixelShaderLowering::needsSingleSourceRTWWithDualSrcBlend() const
{
    bool needsSingleSourceMessage =
        m_cgCtx->m_DriverInfo.sendSingleSourceRTWAfterDualSourceRTW() ||
        m_cgCtx->getModuleMetaData()->psInfo.forceSingleSourceRTWAfterDualSourceRTW ||
        m_hasDiscard ||
        IGC_IS_FLAG_ENABLED(ForceSingleSourceRTWAfterDualSourceRTW);
    return needsSingleSourceMessage;
}

///////////////////////////////////////////////////////////////////////
// Lower discard intrinsics
///////////////////////////////////////////////////////////////////////

#define PASS_FLAG "igc-lower-discard"
#define PASS_DESCRIPTION "Lower discard intrinsics"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(DiscardLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
    IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
    IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
    IGC_INITIALIZE_PASS_END(DiscardLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS

    char DiscardLowering::ID = 0;

DiscardLowering::DiscardLowering()
    : FunctionPass(ID)
{
    initializeDiscardLoweringPass(*PassRegistry::getPassRegistry());
}

bool DiscardLowering::lowerDiscards(Function& F)
{
    if (m_discards.empty() && m_isHelperInvocationCalls.empty())
    {
        return false;
    }

    m_earlyRet = BasicBlock::Create(m_module->getContext(), "DiscardRet", &F);

    // add OUTPUT_PIXELMASK call to track discard conditions
    IRBuilder<> irb(m_earlyRet);

    irb.CreateRetVoid();

    if (m_retBB)
    {
        m_retBB->getTerminator()->eraseFromParent();
        BranchInst::Create(m_earlyRet, m_retBB);
    }
    m_retBB = m_earlyRet;

    Function* fInitMask = GenISAIntrinsic::getDeclaration(m_module,
        GenISAIntrinsic::GenISA_InitDiscardMask);
    Function* fSetMask = GenISAIntrinsic::getDeclaration(m_module,
        GenISAIntrinsic::GenISA_UpdateDiscardMask);

    Value* discardMask = CallInst::Create(fInitMask, llvm::None, "",
        m_entryBB->getFirstNonPHI());

    bool killsPixels = false;

    for (auto discard : m_discards)
    {
        IGC_ASSERT(discard->isGenIntrinsic(GenISAIntrinsic::GenISA_discard));
        killsPixels = true;

        BasicBlock* bbDiscard;
        BasicBlock* bbAfter;

        bbDiscard = discard->getParent();

        BasicBlock::iterator bi = discard->getIterator();
        ++bi;
        bbAfter = bbDiscard->splitBasicBlock(
            bi, "postDiscard");

        // erase the branch inserted by splitBasicBLock
        bbDiscard->getTerminator()->eraseFromParent();

        // create conditional branch to early ret
        IRBuilder<> B(discard);

        // call discard(%dcond)
        // -->
        // UpdatePixelMask(%globalMask, %dcond) ; update discard pixel mask in dmask
        // %all = WaveBallot(%dcond)
        // %1 = icmp eq i32 %all, -1    ; if.all %dcond returnBB
        // br %1, returnBB, postDiscardBB

        Value* discardCond = discard->getOperand(0);
        Value* v = B.CreateCall(fSetMask, { discardMask, discardCond });

        B.CreateCondBr(v, m_earlyRet, bbAfter);
    }

    if (killsPixels)
    {
        m_module->getOrInsertNamedMetadata("KillPixel");
    }

    for (auto inst : m_isHelperInvocationCalls)
    {
        IRBuilder<> B(inst);
        Function* getPixelMask = GenISAIntrinsic::getDeclaration(m_module,
            GenISAIntrinsic::GenISA_GetPixelMask);
        llvm::Value* pixelMask = B.CreateCall(getPixelMask, { discardMask });
        inst->replaceAllUsesWith(B.CreateNot(pixelMask));
        inst->eraseFromParent();
    }

    for (auto discard : m_discards)
    {
        discard->eraseFromParent();
    }


    return true;
}

bool DiscardLowering::runOnFunction(Function& F)
{
    IGCMD::MetaDataUtils* pMdUtils = nullptr;
    pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
    {
        return false;
    }

    m_cgCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

    m_entryBB = &F.getEntryBlock();
    m_module = F.getParent();

    // find return block
    for (auto& bb : F)
    {
        if (llvm::isa<llvm::ReturnInst>(bb.getTerminator()))
        {
            m_retBB = &bb;
            break;
        }
    }

    SmallVector<GenIntrinsicInst*, 4> discardToDel;

    for (auto BI = F.begin(), BE = F.end(); BI != BE; BI++)
    {
        for (auto II = BI->begin(), IE = BI->end(); II != IE; II++)
        {
            GenIntrinsicInst* inst = dyn_cast<GenIntrinsicInst>(II);
            if (inst)
            {
                if (inst->isGenIntrinsic(GenISAIntrinsic::GenISA_discard))
                {
                    // get rid of discard(false)
                    if (ConstantInt * cval = dyn_cast<ConstantInt>(inst->getOperand(0)))
                    {
                        if (cval->isZero())
                        {
                            discardToDel.push_back(inst);
                            continue;
                        }
                    }
                    m_discards.push_back(inst);
                }
                else if (inst->isGenIntrinsic(GenISAIntrinsic::GenISA_IsHelperInvocation))
                {
                    m_isHelperInvocationCalls.push_back(inst);
                }
                else
                    if (inst->isGenIntrinsic(GenISAIntrinsic::GenISA_OUTPUT))
                    {
                        // Check whether PS output omask/depth/stencil and save to
                        // metadata, since after discard lowering, the OUTPUT
                        // could become dead code and get cleaned. While we need to
                        // know it when creating null surface write.
                        uint outputType = (uint)llvm::cast<llvm::ConstantInt>(
                            inst->getOperand(4))->getZExtValue();
                        IGC_ASSERT(outputType == SHADER_OUTPUT_TYPE_DEFAULT ||
                            outputType == SHADER_OUTPUT_TYPE_DEPTHOUT ||
                            outputType == SHADER_OUTPUT_TYPE_STENCIL ||
                            outputType == SHADER_OUTPUT_TYPE_OMASK);
                        switch (outputType)
                        {
                        case SHADER_OUTPUT_TYPE_DEPTHOUT:
                            m_modMD->psInfo.outputDepth = true;
                            break;
                        case SHADER_OUTPUT_TYPE_STENCIL:
                            m_modMD->psInfo.outputStencil = true;
                            break;
                        case SHADER_OUTPUT_TYPE_OMASK:
                            m_modMD->psInfo.outputMask = true;
                            break;
                        default:
                            break;
                        }
                    }
            }
        }
    }

    for (auto I : discardToDel)
    {
        I->eraseFromParent();
    }


    Function* samplePhaseEntry = nullptr;
    Function* pixelPhaseEntry = nullptr;
    NamedMDNode* pixelNode = F.getParent()->getNamedMetadata("pixel_phase");
    NamedMDNode* sampleNode = F.getParent()->getNamedMetadata("sample_phase");
    if (sampleNode)
    {
        samplePhaseEntry = mdconst::dyn_extract<Function>(
            sampleNode->getOperand(0)->getOperand(0));
    }
    if (pixelNode)
    {
        pixelPhaseEntry = mdconst::dyn_extract<Function>(
            pixelNode->getOperand(0)->getOperand(0));
    }

    bool cfgChanged = false;

    // For multirate PS, we will run discard lowering twice, first on sample
    // phase entry before link multi rate pass, second on pixel entry after
    // link multi rate pass. The check is to make sure only lower discards on
    // sample phase entry before link multi rate pass.
    if (samplePhaseEntry == nullptr || pixelPhaseEntry != &F)
    {
        cfgChanged = lowerDiscards(F);
    }
    m_discards.clear();

#ifdef DEBUG_DISCARD_OPT
    DumpLLVMIR(getAnalysis<CodeGenContextWrapper>().getCodeGenContext(), "discard");
#endif

    return cfgChanged;
}

}//namespace IGC
