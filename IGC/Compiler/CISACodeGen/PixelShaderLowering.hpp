/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/PixelShaderCodeGen.hpp"
#include "common/IGCIRBuilder.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include <llvm/IR/DebugLoc.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
// Generate GetPixelMask intrinsics for RTWrites, and move InitPixelMask
// to the beginning of the entry block.  This pass should be added after
// pre-RA scheduler & code sinking to avoid the InitPixelMask is moved again.
// InitPixelMask will create nomask instruction on ARF, and CS in visa
// won't move instruction across it, so if we put it in the middle of BB,
// it will hurt performance.
class PixelShaderAddMask : public llvm::FunctionPass
{
public:
    PixelShaderAddMask();

    virtual bool runOnFunction(llvm::Function& F);

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const
    {
        AU.setPreservesCFG();
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addRequired<CodeGenContextWrapper>();
    }

    static char ID;

protected:
    IGC::ModuleMetaData* m_modMD;
    CodeGenContext* m_cgCtx;
};
void initializePixelShaderAddMaskPass(llvm::PassRegistry&);

class PixelShaderLowering : public llvm::FunctionPass
{
public:
    PixelShaderLowering();

    virtual bool runOnFunction(llvm::Function& F) override;

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addRequired<CodeGenContextWrapper>();
    }

    virtual llvm::StringRef getPassName() const override
    {
        return "PixelShaderLowering";
    }

    static char         ID;

private:
    struct ColorOutput
    {
        llvm::Value* color[4];
        llvm::Value* mask;
        unsigned int RTindex;
        llvm::Value* blendStateIndex;
        llvm::BasicBlock* bb;
        llvm::CallInst* inst;

        ColorOutput()
        {
            memset(this, 0, sizeof(ColorOutput));
        }

    };
    typedef smallvector<ColorOutput, 4> ColorOutputArray;
    typedef smallvector<llvm::DebugLoc, 4> DebugLocArray;

    // For multirate PS, output & discard will be lowered to RTWrite during
    // unification.  Some optimization (GVN) may convert the pixel mask in
    // RTWrite from variable to constant (true). This will cause issue since
    // if RTWrite with header, we need to fill the proper mask.
    void FixSamplePhaseRTWriteMask(llvm::Function& F);

    void FindIntrinsicOutput(ColorOutputArray& color,
        llvm::Value*& depth, llvm::Value*& stencil,
        llvm::Value*& mask, llvm::Value*& src0Alpha,
        DebugLocArray& debugLocs);

    void EmitMemoryFence(llvm::IRBuilder<>& builder, bool forceFlushNone = 0);
    void EmitRTWrite(ColorOutputArray& color, llvm::Value* depth,
        llvm::Value* stencil, llvm::Value* mask,
        llvm::Value* src0Alpha, DebugLocArray& debugLocs);
    void EmitCoarseMask(llvm::Value* mask);

    llvm::Value* fcmpUNEConst(llvm::IGCIRBuilder<>& irb,
        llvm::Value* value, llvm::ConstantFP* cmpConst)
    {
        if (llvm::ConstantFP * cfp = llvm::dyn_cast<llvm::ConstantFP>(value))
        {
            if (cfp->getType() == cmpConst->getType())
            {
                if (cfp->getType()->isFloatTy() &&
                    (cfp->getValueAPF().convertToFloat() == cmpConst->getValueAPF().convertToFloat()))
                {
                    return irb.getInt1(false);
                }
                else if (cfp->getType()->isDoubleTy() &&
                    (cfp->getValueAPF().convertToDouble() == cmpConst->getValueAPF().convertToDouble()))
                {
                    return irb.getInt1(false);
                }
            }
            return irb.getInt1(true);
        }
        else
        {
            return irb.CreateFCmpUNE(value, cmpConst);
        }
    }
    llvm::Value* createOr(llvm::IGCIRBuilder<>& irb,
        llvm::Value* v0, llvm::Value* v1)
    {
        llvm::Value* ctrue = irb.getInt1(true);
        if (v0 == ctrue && v1 == ctrue)
            return ctrue;
        else
            if (v0 == ctrue)
                return v1;
            else
                if (v1 == ctrue)
                    return v0;
                else
                    return irb.CreateOr(v0, v1);
    }

    bool optBlendState(USC::BLEND_OPTIMIZATION_MODE blendOpt,
        ColorOutput& color, bool enableBlendToFill);

    llvm::CallInst* addRTWrite(
        llvm::BasicBlock* bbToAdd, llvm::Value* src0Alpha,
        llvm::Value* oMask, ColorOutput& color,
        llvm::Value* depth, llvm::Value* stencil);

    llvm::CallInst* addDualBlendWrite(
        llvm::BasicBlock* bbToAdd, llvm::Value* oMask,
        ColorOutput& color0, ColorOutput& color1,
        llvm::Value* depth, llvm::Value* stencil,
        uint rtIndex);

    void LowerPositionInput(llvm::GenIntrinsicInst* positionIntr, uint usage);

    void moveRTWriteToBlock(
        llvm::CallInst* rtWrite,
        llvm::SmallVector<llvm::BasicBlock*, 8> & predBB, llvm::BasicBlock* toBB,
        llvm::DenseMap<llvm::Value*, llvm::PHINode*>& valueToPhiMap);

    void moveRTWritesToReturnBlock(
        const ColorOutputArray& colors);

    llvm::PHINode* createPhiForRTWrite(llvm::Value* val,
        smallvector<llvm::BasicBlock*, 8> & predBB, llvm::BasicBlock* toBB);

    void checkAndCreateNullRTWrite(
        llvm::Value* oMask, llvm::Value* depth, llvm::Value* stencil);

    inline bool useDualSrcBlend(const ColorOutputArray& colors)
    {
        return (colors.size() == 2 && m_dualSrcBlendEnabled);
    }

    bool needsSingleSourceRTWWithDualSrcBlend() const;

    llvm::Module* m_module;
    llvm::PostDominatorTree* PDT;
    IGC::ModuleMetaData* m_modMD;
    CodeGenContext* m_cgCtx;

    llvm::BasicBlock* m_ReturnBlock;
    llvm::BasicBlock* m_outputBlock;

    bool SkipSrc0Alpha;
    bool m_dualSrcBlendEnabled;
    bool m_hasDiscard;

    // whether the pixel shader is persample, see CPixelShader::IsPerSample()
    bool m_isPerSample;
    bool uavPixelSync;
};
void initializePixelShaderLoweringPass(llvm::PassRegistry&);

class DiscardLowering : public llvm::FunctionPass
{
private:
    CodeGenContext* m_cgCtx;
    llvm::BasicBlock* m_entryBB;
    llvm::BasicBlock* m_retBB = nullptr;
    llvm::BasicBlock* m_earlyRet;
    llvm::Module* m_module;
    IGC::ModuleMetaData* m_modMD;

    smallvector<llvm::GenIntrinsicInst*, 4> m_discards;
    smallvector<llvm::GenIntrinsicInst*, 4> m_isHelperInvocationCalls;

    bool lowerDiscards(llvm::Function& F);

public:
    static char ID;

    DiscardLowering();

    virtual bool runOnFunction(llvm::Function& F) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addRequired<CodeGenContextWrapper>();
    }

    virtual llvm::StringRef getPassName() const override
    {
        return "Lower Discard";
    }
};
  void initializeDiscardLoweringPass(llvm::PassRegistry&);
}//namespace IGC
