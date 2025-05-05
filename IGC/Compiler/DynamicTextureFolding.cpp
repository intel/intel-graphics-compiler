/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/IGCPassSupport.h"
#include "Compiler/InitializePasses.h"
#include "Compiler/CodeGenPublic.h"
#include "DynamicTextureFolding.h"
using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-dynamic-texture-folding"
#define PASS_DESCRIPTION "dynamic texture folding"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(DynamicTextureFolding, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(DynamicTextureFolding, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char DynamicTextureFolding::ID = 0;

#define DEBUG_TYPE "DynamicTextureFolding"

DynamicTextureFolding::DynamicTextureFolding() : FunctionPass(ID)
{
    initializeDynamicTextureFoldingPass(*PassRegistry::getPassRegistry());
}

void DynamicTextureFolding::FoldSingleTextureValue(CallInst& I)
{
    ModuleMetaData* modMD = m_context->getModuleMetaData();
    Type* type1DArray = GetResourceDimensionType(*m_module, RESOURCE_DIMENSION_TYPE::DIM_1D_ARRAY_TYPE);
    Type* type2DArray = GetResourceDimensionType(*m_module, RESOURCE_DIMENSION_TYPE::DIM_2D_ARRAY_TYPE);
    Type* typeCubeArray = GetResourceDimensionType(*m_module, RESOURCE_DIMENSION_TYPE::DIM_CUBE_ARRAY_TYPE);
    bool skipBoundaryCheck = 1;

    unsigned addrSpace = 0;
   if (SamplerLoadIntrinsic * lInst = dyn_cast<SamplerLoadIntrinsic>(&I))
    {
        addrSpace = lInst->getTextureValue()->getType()->getPointerAddressSpace();
        Type* textureType = lInst->getTexturePtrEltTy();
        if (textureType == type1DArray || textureType == type2DArray || textureType == typeCubeArray)
        {
            skipBoundaryCheck = 0;
        }
    }
    else
    {
        return;
    }

    bool directIdx = false;
    uint textureIndex = 0;
    DecodeAS4GFXResource(addrSpace, directIdx, textureIndex);
    IRBuilder<> builder(&I);
    // if the current texture index is found in modMD as uniform texture, replace the texture load/sample as constant.

    auto it = modMD->inlineDynTextures.find(textureIndex);
    if (it != modMD->inlineDynTextures.end())
    {
        Value* cmpInstA = nullptr;
        if (!skipBoundaryCheck)
        {
            builder.SetInsertPoint(&I);
            // check if the array index is out of bound. The array index size is passed in from UMD
            // we might check xyz coordinate later but skip for now
            if (I.getOperand(3)->getType()->isIntOrIntVectorTy())
                cmpInstA = builder.CreateICmp(ICmpInst::ICMP_SLT, I.getOperand(3), ConstantInt::get(Type::getInt32Ty(I.getContext()), it->second[7]));
            else if (I.getOperand(3)->getType()->isFPOrFPVectorTy())
                cmpInstA = builder.CreateFCmp(FCmpInst::FCMP_ULT, I.getOperand(3), ConstantFP::get(Type::getFloatTy(I.getContext()), it->second[7]));
            else
                return;
        }
        for (auto iter = I.user_begin(); iter != I.user_end(); iter++)
        {
            if (llvm::ExtractElementInst* pExtract = llvm::dyn_cast<llvm::ExtractElementInst>(*iter))
            {
                if (llvm::ConstantInt* pIdx = llvm::dyn_cast<llvm::ConstantInt>(pExtract->getIndexOperand()))
                {
                    if ((&I)->getType()->isIntOrIntVectorTy())
                    {
                        if (skipBoundaryCheck)
                        {
                            pExtract->replaceAllUsesWith(ConstantInt::get((pExtract)->getType(), (it->second[(uint32_t)(pIdx->getZExtValue())])));
                        }
                        else
                        {
                            llvm::Value* Zero = ConstantInt::get(Type::getInt32Ty(I.getContext()), 0);
                            Value* IntInst = ConstantInt::get((pExtract)->getType(), (it->second[(uint32_t)(pIdx->getZExtValue())]));
                            Value* newInst = builder.CreateSelect(cmpInstA, IntInst, Zero);
                            pExtract->replaceAllUsesWith(newInst);
                        }
                    }
                    else if ((&I)->getType()->isFPOrFPVectorTy())
                    {
                        if (skipBoundaryCheck)
                        {
                            pExtract->replaceAllUsesWith(ConstantFP::get((pExtract)->getType(), *(float*)&(it->second[(uint32_t)(pIdx->getZExtValue())])));
                        }
                        else
                        {
                            llvm::Value* ZeroFP = ConstantFP::get(Type::getFloatTy(I.getContext()), 0.0);
                            Value* fpInst = ConstantFP::get((pExtract)->getType(), *(float*)&(it->second[(uint32_t)(pIdx->getZExtValue())]));
                            Value* newInst = builder.CreateSelect(cmpInstA, fpInst, ZeroFP);
                            pExtract->replaceAllUsesWith(newInst);
                        }
                    }
                }
            }
        }
    }
}
Value* DynamicTextureFolding::ShiftByLOD(Instruction* pCall, unsigned int dimension, Value* val)
{
    IRBuilder<> builder(pCall);
    Value* tmp = builder.getInt32(dimension + 1);
    Value* lod = pCall->getOperand(1);
    builder.SetInsertPoint(pCall);
    Value* Lshr =  builder.CreateLShr(tmp, lod);
    if (val)
        return builder.CreateMul(Lshr, val);
    else
        return Lshr;
}
void DynamicTextureFolding::FoldResInfoValue(llvm::GenIntrinsicInst* pCall)
{
    ModuleMetaData* modMD = m_context->getModuleMetaData();
    llvm::Value* r;
    llvm::Value* g;
    llvm::Value* b;
    llvm::Value* a;
    BufferType bufType;
    bool directIdx = false;
    uint textureIndex = 0;
    Value* texOp = pCall->getOperand(0);
    unsigned addrSpace = texOp->getType()->getPointerAddressSpace();
    bufType = DecodeAS4GFXResource(addrSpace, directIdx, textureIndex);
    if (!directIdx || (bufType != RESOURCE && bufType != UAV))
        return;
    auto I32Ty = Type::getInt32Ty(pCall->getContext());
    llvm::Value* Zero = ConstantInt::get(I32Ty, 0);
    for(unsigned int i = 0; i < modMD->inlineResInfoData.size();i++)
    {
        if (textureIndex == modMD->inlineResInfoData[i].textureID)
        {
            ConstantInt* isLODConstant = dyn_cast<ConstantInt>(pCall->getOperand(1));
            a = ConstantInt::get(I32Ty, modMD->inlineResInfoData[i].MipCount);
            switch (modMD->inlineResInfoData[i].SurfaceType)
            {
            case GFXSURFACESTATE_SURFACETYPE_1D:
            {
                g = ConstantInt::get(I32Ty, (modMD->inlineResInfoData[i].SurfaceArray > 0) ? modMD->inlineResInfoData[i].Depth + 1 : 0);
                b = Zero;
                if (isLODConstant)
                {
                    uint64_t lod = isLODConstant->getZExtValue();
                    r = ConstantInt::get(I32Ty, (modMD->inlineResInfoData[i].WidthOrBufferSize + 1) >> lod);
                }
                else
                {
                    r = ShiftByLOD(dyn_cast<Instruction>(pCall), modMD->inlineResInfoData[i].WidthOrBufferSize, nullptr);
                }
                break;
            }
            case GFXSURFACESTATE_SURFACETYPE_2D:
            {
                b = ConstantInt::get(I32Ty,(modMD->inlineResInfoData[i].SurfaceArray > 0) ? modMD->inlineResInfoData[i].Depth + 1 : 0);
                if (isLODConstant)
                {
                    uint64_t lod = isLODConstant->getZExtValue();
                    r = ConstantInt::get(I32Ty, ((modMD->inlineResInfoData[i].WidthOrBufferSize + 1) >> lod)* (modMD->inlineResInfoData[i].QWidth + 1));
                    g = ConstantInt::get(I32Ty, ((modMD->inlineResInfoData[i].Height + 1) >> lod)* (modMD->inlineResInfoData[i].QHeight + 1));
                }
                else
                {
                    Value* QWidth = ConstantInt::get(I32Ty,(modMD->inlineResInfoData[i].QWidth + 1));
                    Value* QHeight = ConstantInt::get(I32Ty, (modMD->inlineResInfoData[i].QHeight + 1));
                    r = ShiftByLOD(dyn_cast<Instruction>(pCall), modMD->inlineResInfoData[i].WidthOrBufferSize, QWidth);
                    g = ShiftByLOD(dyn_cast<Instruction>(pCall), modMD->inlineResInfoData[i].Height, QHeight);
                }
                break;
            }
            case GFXSURFACESTATE_SURFACETYPE_3D:
            {
                if(isLODConstant)
                {
                    uint64_t lod = isLODConstant->getZExtValue();
                    r = ConstantInt::get(I32Ty,((modMD->inlineResInfoData[i].WidthOrBufferSize + 1) >> lod));
                    g = ConstantInt::get(I32Ty, ((modMD->inlineResInfoData[i].Height + 1) >> lod));
                    b = ConstantInt::get(I32Ty, ((modMD->inlineResInfoData[i].Depth + 1) >> lod));
                }
                else
                {
                    r = ShiftByLOD(dyn_cast<Instruction>(pCall), modMD->inlineResInfoData[i].WidthOrBufferSize, nullptr);
                    g = ShiftByLOD(dyn_cast<Instruction>(pCall), modMD->inlineResInfoData[i].Height, nullptr);
                    b = ShiftByLOD(dyn_cast<Instruction>(pCall), modMD->inlineResInfoData[i].Depth, nullptr);
                }
                break;
            }
            case GFXSURFACESTATE_SURFACETYPE_CUBE:
            {
                b = ConstantInt::get(I32Ty, (modMD->inlineResInfoData[i].SurfaceArray > 0) ? modMD->inlineResInfoData[i].Depth + 1 : 0);
                if (isLODConstant)
                {
                    uint64_t lod = isLODConstant->getZExtValue();
                    r = ConstantInt::get(I32Ty, ((modMD->inlineResInfoData[i].WidthOrBufferSize + 1) >> lod));
                    g = ConstantInt::get(I32Ty, ((modMD->inlineResInfoData[i].Height + 1) >> lod));
                }
                else
                {
                    r = ShiftByLOD(dyn_cast<Instruction>(pCall), modMD->inlineResInfoData[i].WidthOrBufferSize, nullptr);
                    g = ShiftByLOD(dyn_cast<Instruction>(pCall), modMD->inlineResInfoData[i].Height, nullptr);
                }
                break;
            }
            case GFXSURFACESTATE_SURFACETYPE_BUFFER:
            case GFXSURFACESTATE_SURFACETYPE_STRBUF:
            case GFXSURFACESTATE_SURFACETYPE_SCRATCH:

            {
                r = (modMD->inlineResInfoData[i].WidthOrBufferSize != UINT_MAX) ? ConstantInt::get(I32Ty, modMD->inlineResInfoData[i].WidthOrBufferSize) : Zero;
                g = Zero;
                b = Zero;
                a = Zero;
                break;
            }
            default:
            {
                r = Zero;
                g = Zero;
                b = Zero;
                a = Zero;
                break;
            }
            }
            for (auto iter = pCall->user_begin(); iter != pCall->user_end(); iter++)
            {
                if (llvm::ExtractElementInst* pExtract = llvm::dyn_cast<llvm::ExtractElementInst>(*iter))
                {
                    if (llvm::ConstantInt* pIdx = llvm::dyn_cast<llvm::ConstantInt>(pExtract->getIndexOperand()))
                    {
                        if (pIdx->getZExtValue() == 0)
                        {
                            pExtract->replaceAllUsesWith(r);
                            InstsToRemove.push_back(pExtract);
                        }
                        else if (pIdx->getZExtValue() == 1)
                        {
                            pExtract->replaceAllUsesWith(g);
                            InstsToRemove.push_back(pExtract);
                        }
                        else if (pIdx->getZExtValue() == 2)
                        {
                            pExtract->replaceAllUsesWith(b);
                            InstsToRemove.push_back(pExtract);
                        }
                        else if (pIdx->getZExtValue() == 3)
                        {
                            pExtract->replaceAllUsesWith(a);
                            InstsToRemove.push_back(pExtract);
                        }
                    }
                }
            }
        }
    }
}

void DynamicTextureFolding::visitCallInst(CallInst& I)
{
    ModuleMetaData* modMD = m_context->getModuleMetaData();
    if (GenIntrinsicInst* pCall = dyn_cast<GenIntrinsicInst>(&I))
    {
        auto ID = pCall->getIntrinsicID();
        if (!IGC_IS_FLAG_ENABLED(DisableDynamicTextureFolding) && modMD->inlineDynTextures.size() != 0)
        {
            if (ID == GenISAIntrinsic::GenISA_ldptr
                || ID == GenISAIntrinsic::GenISA_ldlptr
                )
            {
                FoldSingleTextureValue(I);
            }
        }
        if (!IGC_IS_FLAG_ENABLED(DisableDynamicResInfoFolding) && ID == GenISAIntrinsic::GenISA_resinfoptr)
        {
            if ( modMD->inlineResInfoData.size() > 0)
            {
                FoldResInfoValue(pCall);
            }
            else
            {
                BufferType bufType;
                bool directIdx = false;
                uint textureIndex = 0;
                Value* texOp = pCall->getOperand(0);
                unsigned addrSpace = texOp->getType()->getPointerAddressSpace();
                bufType = DecodeAS4GFXResource(addrSpace, directIdx, textureIndex);
                m_ResInfoFoldingOutput[textureIndex].textureID = textureIndex;
                if (!directIdx || (bufType != RESOURCE && bufType != UAV))
                    return;
                for (auto UI = pCall->user_begin(), UE = pCall->user_end(); UI != UE; ++UI)
                {
                    if (llvm::ExtractElementInst* useInst = dyn_cast<llvm::ExtractElementInst>(*UI))
                    {
                        ConstantInt* eltID = dyn_cast<ConstantInt>(useInst->getOperand(1));
                        if (!eltID)
                            continue;
                        m_ResInfoFoldingOutput[textureIndex].value[int_cast<unsigned>(eltID->getZExtValue())] = true;
                    }
                }
            }
        }
    }
}

template<typename ContextT>
void DynamicTextureFolding::copyResInfoData(ContextT* pShaderCtx)
{
    pShaderCtx->programOutput.m_ResInfoFoldingOutput.clear();
    for (unsigned int i = 0; i < m_ResInfoFoldingOutput.size(); i++)
    {
        pShaderCtx->programOutput.m_ResInfoFoldingOutput.push_back(m_ResInfoFoldingOutput[i]);
    }
}

bool DynamicTextureFolding::doFinalization(llvm::Module& M)
{
    return false;
}

bool DynamicTextureFolding::runOnFunction(Function& F)
{
    m_context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_module = F.getParent();
    visit(F);
    for (auto *insn : InstsToRemove)
        insn->eraseFromParent();

    return false;
}
