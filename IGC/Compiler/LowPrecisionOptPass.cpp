/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "Compiler/LowPrecisionOptPass.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/CommandLine.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"

#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/IGCIRBuilder.h"
using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace GenISAIntrinsic;

char LowPrecisionOpt::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-low-precision-opt"
#define PASS_DESCRIPTION "Low Precision Opt"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LowPrecisionOpt, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(LowPrecisionOpt, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

LowPrecisionOpt::LowPrecisionOpt() : FunctionPass(ID)
{
    initializeLowPrecisionOptPass(*PassRegistry::getPassRegistry());
    m_func_llvm_GenISA_DCL_inputVec_f16 = nullptr;
    m_func_llvm_GenISA_DCL_inputVec_f32 = nullptr;
    m_currFunction = nullptr;
    func_llvm_floor_f32 = nullptr;
}

bool LowPrecisionOpt::runOnFunction(Function &F)
{
    m_changed = false;
    CodeGenContextWrapper* pCtxWrapper = &getAnalysis<CodeGenContextWrapper>();
    CodeGenContext* ctx = pCtxWrapper->getCodeGenContext();

    MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
    {
        return m_changed;
    }
    llvm::IGCIRBuilder<> builder(F.getContext());
    m_builder = &builder;
    m_currFunction = &F;
    shdrType = ctx->type;
    bundles.clear();
    m_simplifyAlu = true;
    m_changeSample = false;
    visit(F);
    // change sampler only after we simplified fext + ftrunc
    m_simplifyAlu = false;
    m_changeSample = true;
    visit(F);
    std::sort(bundles.begin(), bundles.end(), cmpOperator);
    auto bundleEnd = bundles.end();
    for (auto bundle = bundles.begin(); bundle != bundleEnd; ++bundle)
    {
        (*bundle).cInst->moveBefore(&(*(m_currFunction->getEntryBlock().begin())));
        (*bundle).fpTrunc->moveBefore(&(*(m_currFunction->getEntryBlock().begin())));
    }
    return m_changed;
}

void LowPrecisionOpt::visitFPExtInst(llvm::FPExtInst &I)
{
    if(!m_simplifyAlu)
    {
        return;
    }
    if (I.getOperand(0)->getType()->isHalfTy())
    {
        Instruction* I0 = dyn_cast<Instruction>(I.getOperand(0));
        llvm::GenIntrinsicInst* callInst = llvm::dyn_cast<llvm::GenIntrinsicInst>(I.getOperand(0));

        if (I0 && I0->getOpcode() == Instruction::FPTrunc && I.getDestTy() == I0->getOperand(0)->getType())
        {
            I.replaceAllUsesWith(I0->getOperand(0));
            I.eraseFromParent();
            m_changed = true;
        }
        else if (callInst &&  callInst->hasOneUse())
        {
            GenISAIntrinsic::ID ID = callInst->getIntrinsicID();
            if(ID == GenISAIntrinsic::GenISA_DCL_ShaderInputVec || ID == GenISAIntrinsic::GenISA_DCL_inputVec)
            {
                /*
                Catches a pattern where we have a lowp input, then extend it back up. This
                generates mixed mode instructions and so it's better to keep it as PLN.
                Example if it's used directly in the sample instruction before CNL.
                */

                if (m_func_llvm_GenISA_DCL_inputVec_f32 == nullptr)
                {
                    m_func_llvm_GenISA_DCL_inputVec_f32 = llvm::GenISAIntrinsic::getDeclaration(
                        m_currFunction->getParent(),
                        ID,
                        Type::getFloatTy(m_builder->getContext()));
                }

                m_builder->SetInsertPoint(callInst);
                Value* v = m_builder->CreateCall2(m_func_llvm_GenISA_DCL_inputVec_f32, callInst->getOperand(0), callInst->getOperand(1));
#if VALUE_NAME_ENABLE
                v->setName(callInst->getName());
#endif
                I.replaceAllUsesWith(v);
                I.eraseFromParent();
                callInst->eraseFromParent();
                m_changed = true;
            }
        }
    }
}

void LowPrecisionOpt::visitFPTruncInst(llvm::FPTruncInst &I)
{
    if(!m_simplifyAlu)
    {
        return;
    }
    llvm::GenIntrinsicInst* cInst = llvm::dyn_cast<llvm::GenIntrinsicInst>(I.getOperand(0));

    if (cInst &&
        cInst->getIntrinsicID() == GenISAIntrinsic::GenISA_RuntimeValue)
    {
        if (!IGC_IS_FLAG_ENABLED(HoistPSConstBufferValues) ||
            shdrType != ShaderType::PIXEL_SHADER)
            return;
        moveBundle bundle;
        bundle.index = (uint)llvm::cast<llvm::ConstantInt>(cInst->getOperand(0))->getZExtValue();
        bundle.cInst = cInst;
        bundle.fpTrunc = &I;
        bundles.push_back(bundle);
    }
}

// If all the uses of a sampler instruction are converted to a different floating point type
// try to propagate the type in the sampler
bool LowPrecisionOpt::propagateSamplerType(llvm::GenIntrinsicInst &I)
{
    if (IGC_IS_FLAG_DISABLED(UpConvertF16Sampler) && I.getType()->getVectorElementType()->isHalfTy())
    {
        return false;
    }

    IGC::CodeGenContext &CGContext = *getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (!CGContext.platform.supportFP16())
    {
        return false;
    }
    
    Type* eltTy = I.getType()->getVectorElementType();
    Type *newDstType = nullptr;
    if (eltTy->isFloatingPointTy())
    {
        // check that all uses are extractelement followed by fpext
        newDstType = I.getType()->getVectorElementType()->isFloatTy() ?
            m_builder->getHalfTy() : m_builder->getFloatTy();
        for (auto use = I.user_begin(); use != I.user_end(); ++use)
        {
            auto extractElt = dyn_cast<ExtractElementInst>(*use);
            if (!(extractElt && extractElt->hasOneUse()))
            {
                return false;
            }
            auto fpExtOrTrunc = dyn_cast<CastInst>(*extractElt->user_begin());

            if (!(fpExtOrTrunc && fpExtOrTrunc->getType() == newDstType))
            {
                return false;
            }
        }
    }
    else if (eltTy == m_builder->getInt32Ty())
    {
        // check if we can lower the sampler return to 16-bit
        newDstType = m_builder->getInt16Ty();
        for (auto use = I.user_begin(); use != I.user_end(); ++use)
        {
            auto extractElt = dyn_cast<ExtractElementInst>(*use);
            if (!(extractElt && extractElt->hasOneUse()))
            {
                return false;
            }
            auto isUpperBitClear = [this](User* U)
            {
                // match the pattern 
                // %scalar59 = extractelement <4 x i32> % 83, i32 3
                // % 84 = and i32 %scalar59, 65535
                if (U->getType() != m_builder->getInt32Ty())
                {
                    return false;
                }
                auto andInst = dyn_cast<BinaryOperator>(U);
                if (!andInst || andInst->getOpcode() != BinaryOperator::And)
                {
                    return false;
                }
                auto andSrc1 = dyn_cast<ConstantInt>(andInst->getOperand(1));
                if (!andSrc1 || andSrc1->getZExtValue() != 0xFFFF)
                {
                    return false;
                }
                return true;
            };

            auto Use = *extractElt->user_begin();
            bool isInt32to16Trunc = dyn_cast<TruncInst>(Use) && Use->getType() == m_builder->getInt16Ty();
            if (!isInt32to16Trunc && !isUpperBitClear(Use))
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }

    VectorType* oldTy = cast<VectorType>(I.getType());
    llvm::SmallVector<llvm::Type*, 4> overloadTys;
    auto retTy = VectorType::get(newDstType, int_cast<unsigned int>(oldTy->getNumElements()));
    overloadTys.push_back(retTy);
    auto ID = I.getIntrinsicID();
    switch (ID)
    {
    case GenISAIntrinsic::GenISA_sampleptr:
    case GenISAIntrinsic::GenISA_sampleBptr:
    case GenISAIntrinsic::GenISA_sampleCptr:
    case GenISAIntrinsic::GenISA_sampleDptr:
    case GenISAIntrinsic::GenISA_sampleDCptr:
    case GenISAIntrinsic::GenISA_sampleLptr:
    case GenISAIntrinsic::GenISA_sampleLCptr:
    case GenISAIntrinsic::GenISA_sampleBCptr:
        // 4 overloaded tys: ret, arg0, resource, sampler
        overloadTys.push_back(I.getArgOperand(0)->getType());
        overloadTys.push_back(cast<SampleIntrinsic>(&I)->getTextureValue()->getType());
        overloadTys.push_back(cast<SampleIntrinsic>(&I)->getSamplerValue()->getType());
        break;
    case GenISAIntrinsic::GenISA_ldptr:
    case GenISAIntrinsic::GenISA_ldmsptr:
        overloadTys.push_back(cast<SamplerLoadIntrinsic>(&I)->getTextureValue()->getType());
        break;
    case GenISAIntrinsic::GenISA_gather4ptr:
    case GenISAIntrinsic::GenISA_gather4Cptr:
    case GenISAIntrinsic::GenISA_gather4POptr:
    case GenISAIntrinsic::GenISA_gather4POCptr:
        // 4 overloaded tys: ret, arg0, resource, sampler
        overloadTys.push_back(I.getArgOperand(0)->getType());
        overloadTys.push_back(cast<SamplerGatherIntrinsic>(&I)->getTextureValue()->getType());
        overloadTys.push_back(cast<SamplerGatherIntrinsic>(&I)->getSamplerValue()->getType());
        break;
    case GenISAIntrinsic::GenISA_sampleL:
    case GenISAIntrinsic::GenISA_sampleD:
        overloadTys.push_back(I.getArgOperand(0)->getType());
        break;
    default:
        return false;
    }

    Function* newSample = GenISAIntrinsic::getDeclaration(
        m_currFunction->getParent(), I.getIntrinsicID(), overloadTys);
    llvm::SmallVector<llvm::Value*, 8> newArgs;
    for (unsigned int i = 0, argSize = I.getNumArgOperands(); i < argSize; i++)
    {
        newArgs.push_back(I.getArgOperand(i));
    }
    m_builder->SetInsertPoint(&I);
    auto newCall = m_builder->CreateCall(newSample, newArgs);

    for (auto use = I.user_begin(); use != I.user_end(); ++use)
    {
        ExtractElementInst*  extractElt = cast<ExtractElementInst>(*use);
        Value* extractUse = *extractElt->user_begin();
        Value* newExtract = m_builder->CreateExtractElement(newCall, extractElt->getIndexOperand());
        if (extractUse->getType()->isFloatingPointTy())
        {
            extractUse->replaceAllUsesWith(newExtract);
        }
        else
        {
            if (dyn_cast<TruncInst>(extractUse))
            {
                // replace trunc with new extractElt
                extractUse->replaceAllUsesWith(newExtract);
            }
            else
            {
                // replace and with zext
                Value* zextInst = m_builder->CreateZExt(newExtract, m_builder->getInt32Ty());
                extractUse->replaceAllUsesWith(zextInst);
            }
        }
    }
    return true;
}

void LowPrecisionOpt::visitIntrinsicInst(llvm::IntrinsicInst &I)
{
    if(!m_simplifyAlu)
    {
        return;
    }
    if (I.getIntrinsicID() != llvm::Intrinsic::floor ||
        I.getType() != Type::getHalfTy(m_builder->getContext()))
        return;

    auto src = I.getOperand(0);
    m_builder->SetInsertPoint(&I);

    auto fpTrunc = llvm::dyn_cast <llvm::FPTruncInst>(src);
    if (fpTrunc)
    {
        src = fpTrunc->getOperand(0);
    }
    else
    {
        src = m_builder->CreateFPExt(src, m_builder->getFloatTy());
    }

    if (!func_llvm_floor_f32)
        func_llvm_floor_f32 = llvm::Intrinsic::getDeclaration(m_currFunction->getParent(), Intrinsic::floor, m_builder->getFloatTy());

    auto floor32 = m_builder->CreateCall(func_llvm_floor_f32, src);
#if VALUE_NAME_ENABLE
    floor32->setName(I.getName());
#endif

    if (I.hasOneUse())
    {
        auto hfSub = llvm::dyn_cast<llvm::BinaryOperator>(*I.user_begin());

        if (hfSub && hfSub->getOpcode() == llvm::Instruction::BinaryOps::FSub)
        {
            if (hfSub->getOperand(0) == I.getOperand(0))
            {
                auto fSub = m_builder->CreateFSub(src, floor32, hfSub->getName());
                auto fpdst = m_builder->CreateFPTrunc(fSub, Type::getHalfTy(m_builder->getContext()));
                hfSub->replaceAllUsesWith(fpdst);
            }
        }
    }
    else
    {
        auto fpdst = m_builder->CreateFPTrunc(floor32, Type::getHalfTy(m_builder->getContext()));
        I.replaceAllUsesWith(fpdst);
        I.eraseFromParent();
    }

}

/*FP16SamplerOptimization*/
void LowPrecisionOpt::visitCallInst(CallInst &I)
{
    if(!m_changeSample)
    {
        return;
    }
    if (isSampleLoadGather4InfoInstruction(&I))
    {
        bool changed = propagateSamplerType(*cast<GenIntrinsicInst>(&I));
        if (changed)
        {
            return;
        }
    }
    if (IGC_IS_FLAG_ENABLED(EnableFP16SamplerOpt))
    {
		/*
		Convert following pattern :
		%5 = load half, half addrspace(1)* %4, align 2
		%6 = fpext half %5 to float
		%7 = icmp eq i32 %smpSnapWA, 0
		%8 = fcmp olt half %5, 0xH0000
		%9 = select i1 %8, float -1.000000e+00, float %6
		%.0.i.i = select i1 %7, float %6, float %9
		%10 = call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float %.0.i.i, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0)

		To ->

		%8 = load half, half addrspace(1)* %7, align 2
		%9 = fcmp olt half %8, 0xH0000
		%10 = select i1 %9, half 0xHBC00, half %8
		%11 = icmp eq i32 %smpSnapWA, 0
		%12 = select i1 %11, half %8, half %10
		%13 = call <4 x float> @genx.GenISA.sampleL.v4f32.f16(half 0xH0000, half %12, half 0xH0000, half 0xH0000, half 0xH0000, i32 0, i32 0, i32 0, i32 0, i32 0)
		*/

        if (I.getCalledFunction())
        {
            IGC::CodeGenContext &CGContext = *getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
            if (CGContext.platform.supportSamplerFp16Input())
            {
                if (getIntrinsicID(I.getCalledFunction()) == GenISAIntrinsic::GenISA_sampleL)
                {
                    SmallVector<Value *, 15>oprndFPExt;
                    SmallVector<Instruction *, 30>newInst;
                    int cntDownConverted = 0;
                    bool insertNewInst = true;

                    for (unsigned int i = 0; i < I.getNumArgOperands(); ++i)
                    {
                        if (Value *oprndi = I.getOperand(i))
                        {
                            if (oprndi->getType()->isFloatTy()) // the operand is a float. only then, check to see if it can be SAFELY converted to half type
                            {
                                APFloat tempAPF((float)(0.0));
                                llvm::integerPart tempInt;
                                bool isExact = false;
                                // 1. is a constant and can be SAFELY converted to half type
                                if (ConstantFP *CArgi = dyn_cast<ConstantFP>(oprndi))
                                {
                                    tempAPF = CArgi->getValueAPF();
                                    tempAPF.convertToInteger(&tempInt, 32, tempAPF.isNegative(), llvm::APFloat::rmTowardZero, &isExact);
                                    if (isExact)
                                    {
                                        tempAPF.convert(llvm::APFloat::IEEEhalf(), llvm::APFloat::rmTowardZero, &isExact);
                                        oprndFPExt.push_back(ConstantFP::get(I.getContext(), tempAPF));
                                        cntDownConverted++;
                                    }
                                }
                                // 2. The float coordinates are a direct result of a upconversion from half to float
                                else if (FPExtInst *oprndiInst = dyn_cast<FPExtInst>(oprndi))
                                {
                                    if (oprndiInst->getOperand(0)->getType()->isHalfTy())
                                    {
                                        oprndFPExt.push_back(oprndiInst->getOperand(0)); //replace the fpext with the original half operand.
                                        cntDownConverted++;
                                    }
                                }	
								// 3. Handle selectInst generated as result of snapWA
								else if (SelectInst *selInst = dyn_cast<SelectInst>(oprndi))
								{
									Value *S1 = NULL;
									Value *S2 = NULL;
									Value *oprndTrueSelect = selInst->getTrueValue();
									Value *oprndFalseSelect = selInst->getFalseValue();

									if (FPExtInst *COprndFPExt = dyn_cast<FPExtInst>(oprndTrueSelect)) {
										if (SelectInst *COprndSelInst = dyn_cast<SelectInst>(oprndFalseSelect)) {
											if (COprndFPExt->getOperand(0)->getType()->isHalfTy() &&
												(dyn_cast<ConstantFP>(COprndSelInst->getTrueValue()) && dyn_cast<FPExtInst>(COprndSelInst->getFalseValue()))) {
												S1 = COprndFPExt->getOperand(0);
												tempAPF = dyn_cast<ConstantFP>(COprndSelInst->getTrueValue())->getValueAPF();
												tempAPF.convertToInteger(&tempInt, 32, tempAPF.isNegative(), llvm::APFloat::rmTowardZero, &isExact);
												if (isExact)
												{
													if (tempInt == -1) {
														Instruction *newSelect = SelectInst::Create(COprndSelInst->getCondition(), ConstantFP::get(Type::getHalfTy(I.getContext()), (double)(-1.0)),
															dyn_cast<FPExtInst>(COprndSelInst->getFalseValue())->getOperand(0), "", COprndSelInst);
														newInst.push_back(newSelect);
														S2 = newSelect;
													}
												}
											}
										}
									}
									else if (FPExtInst *COprndFPExt = dyn_cast<FPExtInst>(oprndFalseSelect)) {
										if (SelectInst *COprndSelInst = dyn_cast<SelectInst>(oprndTrueSelect)) {
											if (COprndFPExt->getOperand(0)->getType()->isHalfTy() &&
												(dyn_cast<ConstantFP>(COprndSelInst->getTrueValue()) && dyn_cast<FPExtInst>(COprndSelInst->getFalseValue()))) {
												S1 = COprndFPExt->getOperand(0);
												tempAPF = dyn_cast<ConstantFP>(COprndSelInst->getTrueValue())->getValueAPF();
												tempAPF.convertToInteger(&tempInt, 32, tempAPF.isNegative(), llvm::APFloat::rmTowardZero, &isExact);
												if (isExact)
												{
													if (tempInt == -1) {
														Instruction *newSelect = SelectInst::Create(COprndSelInst->getCondition(), ConstantFP::get(Type::getHalfTy(I.getContext()), (double)(-1.0)),
															dyn_cast<FPExtInst>(COprndSelInst->getFalseValue())->getOperand(0), "", COprndSelInst);
														newInst.push_back(newSelect);
														S2 = newSelect;
													}
												}
											}											
										}
									}
									
									if (S1 && S2) {
										Instruction *newSelect = SelectInst::Create(selInst->getCondition(), S1, S2, "", selInst);
										oprndFPExt.push_back(newSelect); 
										cntDownConverted++;
									}
								}

                                // 4. The float operand came from a PHI node.
                                else if (PHINode *phiInst = dyn_cast<PHINode>(oprndi))
                                {
                                    PHINode *newPhiInst = PHINode::Create(Type::getHalfTy(I.getContext()), phiInst->getNumIncomingValues(), "", phiInst);
                                    newInst.push_back(newPhiInst);
                                    unsigned int numVal = phiInst->getNumIncomingValues();
                                    unsigned int l;
                                    for (l = 0; l < numVal; l++)
                                    {
                                        bool nodeCanBeDownConverted = false;
                                        if (phiInst->getIncomingValue(l))
                                        {
                                            // 4.a The incominng value is a direct result of a upconversion from half to float
                                            if (FPExtInst *instTemp = dyn_cast<FPExtInst>(phiInst->getIncomingValue(l)))
                                            {
                                                if (instTemp->getOperand(0)->getType()->isHalfTy())
                                                {
                                                    newPhiInst->addIncoming(instTemp->getOperand(0), phiInst->getIncomingBlock(l)); // Edit the incoming value to replace the fpext with the original half operand. \
                                                                                                                                                                                    // But retain the incoming block
                                                    nodeCanBeDownConverted = true;
                                                }
                                            }
                                            // 4.b The incoming node is a result of the snapWA
                                            else if (SelectInst *nodeInst = dyn_cast_or_null<SelectInst>(phiInst->getIncomingValue(l)))
                                            {
                                                Value *oprndTrueSelect = nodeInst->getTrueValue();
                                                Value *oprndFalseSelect = nodeInst->getFalseValue();

                                                if (ConstantFP *COprndTrue = dyn_cast<ConstantFP>(oprndTrueSelect))
                                                {
                                                    tempAPF = COprndTrue->getValueAPF();
                                                    tempAPF.convertToInteger(&tempInt, 32, tempAPF.isNegative(), llvm::APFloat::rmTowardZero, &isExact);
                                                    if (isExact)
                                                    {
                                                        if ((instTemp = dyn_cast<FPExtInst>(oprndFalseSelect)))
                                                        {
                                                            if (instTemp->getOperand(0)->getType()->isHalfTy())
                                                            {
                                                                tempAPF.convert(llvm::APFloat::IEEEhalf(), llvm::APFloat::rmTowardZero, &isExact);

                                                                //create a new select and add that as incoming to the phi node
                                                                Instruction *newSelect = SelectInst::Create(nodeInst->getCondition(), ConstantFP::get(I.getContext(), tempAPF), instTemp->getOperand(0), "", nodeInst);
                                                                newInst.push_back(newSelect);
                                                                newPhiInst->addIncoming(newSelect, phiInst->getIncomingBlock(l)); // Edit the incoming value, to insert a new half-type select instead of the original. \
                                                                                                                                                                                              // But retain the incoming block
                                                                nodeCanBeDownConverted = true;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        if (!nodeCanBeDownConverted)
                                        {
                                            break; //if one of the incoming nodes cannot be converted into half type, we skip checking the next one.
                                        }
                                    }
                                    if (l == numVal) //both nodes of the PhiNode can be downconverted to half
                                    {
                                        oprndFPExt.push_back(newPhiInst);
                                        cntDownConverted++;
                                    }
                                }
                            }
                            else
                            {
                                if (i > 0)                      /* If the first operand is not a float, this is not a fp32 sample.
                                                                Do not increment cntDownConverted so that we fail the next check and break out of the loop.*/
                                {
                                    oprndFPExt.push_back(oprndi);
                                    cntDownConverted++;
                                }
                            }
                        }

                        if (cntDownConverted != (i + 1))    /*  this is the case when:
                                                            1. this is not a fp32 sample
                                                            OR
                                                            2. at least one of the fp32 operands cannot be converted to half
                                                            (either of the coord did not come from a fpext (either direct or via a Phi Node)
                                                            sample instruction requires the coords to be type-matched with other float operands.
                                                            so, if either one cannot be SAFELY converted to half, we cannot modify this call.
                                                            */
                        {
                            insertNewInst = false;
                            break;                          // since one of the float operands cannot be SAFELY converted to half we need not check the other operands
                        }
                    }

                    if (insertNewInst)
                    {
                        //create a new fp16 sample_L intrinsic
                        Type* types[] = { I.getCalledFunction()->getReturnType(), Type::getHalfTy(I.getContext()) };
                        llvm::Function *func_llvm_GenISA_sampleL_v4f16_f32 = llvm::GenISAIntrinsic::getDeclaration(I.getCalledFunction()->getParent(), GenISAIntrinsic::GenISA_sampleL, llvm::ArrayRef<llvm::Type*>(types, 2));

                        SmallVector<Value *, 15> args;
                        for (unsigned int ii = 0; ii < I.getNumArgOperands(); ++ii)
                        {
                            args.push_back(oprndFPExt[ii]);
                        }
                        Instruction *fp16_sampleL = CallInst::Create(func_llvm_GenISA_sampleL_v4f16_f32, args, "", &I);

                        I.replaceAllUsesWith(fp16_sampleL);
                        I.eraseFromParent();

                        args.clear();
                    }
                    else
                    {
                        // erase the new instructions we inserted earlier
                        for (unsigned int jj = 0; jj < newInst.size(); ++jj)
                        {
                            newInst[jj]->eraseFromParent();
                        }
                    }
                }
            }
        }
    }
}
