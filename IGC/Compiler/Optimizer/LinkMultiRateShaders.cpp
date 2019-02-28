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
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "LLVM3DBuilder/MetadataBuilder.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"
#include "LLVM3DBuilder/BuiltinsFrontend.hpp"

#include "common/FunctionUpgrader.h"

using namespace IGC;
using namespace IGC::IGCMD;
using namespace llvm;

class LinkMultiRateShader : public ModulePass
{
public:
    LinkMultiRateShader() : ModulePass(ID)
    {

    }
    static char ID;

    bool runOnModule(llvm::Module &M);

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const
    {
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addRequired<CodeGenContextWrapper>();
    }

    virtual llvm::StringRef getPassName() const
    {
        return "LinkMultirateShader";
    }
private:
    void Link(Function* pixelPhase, Function* samplePhase, Module& M);
    Function* PatchSamplePhaseSignature(
        Function* samplePhase, 
        SmallDenseMap<unsigned int, unsigned int, 16>& linkSignature);
    void GetPixelPhaseOutput(
        Function* pixelPhase, 
        SmallVector<Value*, 10>& outputs, 
        const SmallDenseMap<unsigned int, unsigned int, 16>& linkSignature);
};

char LinkMultiRateShader::ID = 0;

Pass* CreateLinkMultiRateShaderPass()
{
    return new LinkMultiRateShader();
}

bool LinkMultiRateShader::runOnModule(llvm::Module &M)
{
    Function* pixelPhase = nullptr;
    Function* samplePhase = nullptr;
    NamedMDNode* pixelNode = M.getNamedMetadata("pixel_phase");
    NamedMDNode* sampleNode = M.getNamedMetadata("sample_phase");
    if(sampleNode == nullptr)
    {
        // if there is no sample phase no need to link
        return false;
    }
    if(pixelNode)
    {
        pixelPhase = mdconst::dyn_extract<Function>(pixelNode->getOperand(0)->getOperand(0));
    }
    if(sampleNode)
    {
        samplePhase = mdconst::dyn_extract<Function>(sampleNode->getOperand(0)->getOperand(0));
    }
    if(pixelPhase == nullptr)
    {
        IRBuilder<> builder(M.getContext());
        pixelPhase = Function::Create(FunctionType::get(builder.getVoidTy(), false),
            GlobalValue::ExternalLinkage,
            "multiRatePS",
            &M);

        BasicBlock* bb = BasicBlock::Create(M.getContext(), "dummyBB", pixelPhase);
        builder.SetInsertPoint(bb);
        builder.CreateRetVoid();
        MetadataBuilder metadatBuilder(&M);
        metadatBuilder.SetShadingRate(pixelPhase, PSPHASE_PIXEL);
    }
    Link(pixelPhase, samplePhase, M);
    sampleNode->eraseFromParent();

    MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();    
    pMdUtils->clearFunctionsInfo();
    IGCMetaDataHelper::addFunction(*pMdUtils, pixelPhase);
    NamedMDNode* coarseNode = M.getNamedMetadata("coarse_phase");
    if (coarseNode != nullptr)
    {
        Function* coarsePhase =
            llvm::mdconst::dyn_extract<Function>(coarseNode->getOperand(0)->getOperand(0));
        IGCMetaDataHelper::addFunction(*pMdUtils, coarsePhase);
    }

    return true;
}

void LinkMultiRateShader::GetPixelPhaseOutput(
    Function* pixelPhase, 
    SmallVector<Value*, 10>& outputs,
    const SmallDenseMap<unsigned int, unsigned int, 16>& linkSignature)
{
    Function* phaseInput = GenISAIntrinsic::getDeclaration(
        pixelPhase->getParent(), 
        GenISAIntrinsic::GenISA_PHASE_OUTPUT);
    SmallVector<Instruction*, 10> phaseIntrinsic;
    for(auto I = phaseInput->user_begin(), E = phaseInput->user_end(); I!=E; ++I)
    {
        if(Instruction* inst = dyn_cast<Instruction>(*I))
        {
            if(inst->getParent()->getParent() == pixelPhase)
            {
                unsigned int index = (unsigned int)cast<ConstantInt>(inst->getOperand(1))->getZExtValue();
                auto it = linkSignature.find(index);
                if(it != linkSignature.end())
                {
                    unsigned int location = it->second;
                    outputs[location + 1] = inst->getOperand(0);
                }
                phaseIntrinsic.push_back(inst);
            }
        }
    }
    for(auto it = phaseIntrinsic.begin(), itEnd = phaseIntrinsic.end(); it != itEnd; ++it)
    {
        (*it)->eraseFromParent();
    }
}

void LinkMultiRateShader::Link(Function* pixelPhase, Function* samplePhase, Module& M)
{
    IRBuilder<> builder(M.getContext());
    SmallDenseMap<unsigned int, unsigned int, 16> linkSignature;
    Function* samplePhasePatched = PatchSamplePhaseSignature(samplePhase, linkSignature);

    BasicBlock* returnBB = &pixelPhase->getBasicBlockList().back();
    BasicBlock* loopHeader = BasicBlock::Create(M.getContext(), "SampleLoopHeader", pixelPhase);
    BasicBlock* loopBlock = BasicBlock::Create(M.getContext(), "LoopBlock", pixelPhase);
    BasicBlock* newRet = BasicBlock::Create(M.getContext(), "ret", pixelPhase);
    returnBB->getTerminator()->eraseFromParent();
    builder.SetInsertPoint(returnBB);
    builder.CreateBr(loopHeader);
    builder.SetInsertPoint(&(*pixelPhase->getEntryBlock().begin()));
    Value* loopCounterPtr = builder.CreateAlloca(builder.getInt32Ty());
    
    // Initialize the loop counter in the loop header 
    builder.SetInsertPoint(loopHeader);
    unsigned int addrSpaceRsc = IGC::EncodeAS4GFXResource(*builder.getInt32(0), RENDER_TARGET, 0);
    PointerType* ptrTy = llvm::PointerType::get(builder.getInt32Ty(), addrSpaceRsc);
    Value* pRsrc = ConstantPointerNull::get(ptrTy);
    Function* sampleInfoptr = GenISAIntrinsic::getDeclaration(&M, GenISAIntrinsic::GenISA_sampleinfoptr,  pRsrc->getType());
    Value* sampleInfoValue = builder.CreateCall(sampleInfoptr, pRsrc);

    Value* msaaRate = builder.CreateExtractElement(sampleInfoValue, builder.getInt32(0));
    builder.CreateStore(msaaRate, loopCounterPtr);
    builder.CreateBr(loopBlock);

    //Loop block decrement the counter and call the sample phase
    builder.SetInsertPoint(loopBlock);
    Value* counter = builder.CreateLoad(loopCounterPtr);
    counter = builder.CreateAdd(counter, builder.getInt32(-1));
    SmallVector<Value*, 10> inputs(linkSignature.size()+1, UndefValue::get(builder.getFloatTy()));
    inputs[0] = counter;
    GetPixelPhaseOutput(pixelPhase, inputs, linkSignature);
    builder.CreateStore(counter, loopCounterPtr);

    builder.CreateCall(samplePhasePatched, inputs);
    Value* cond = builder.CreateICmp(CmpInst::Predicate::ICMP_EQ, counter, builder.getInt32(0));
    builder.CreateCondBr(cond, newRet, loopBlock);
    
    // The new return block is empty
    builder.SetInsertPoint(newRet);
    builder.CreateRetVoid();
}

Function* LinkMultiRateShader::PatchSamplePhaseSignature(
    Function* samplePhase, 
    SmallDenseMap<unsigned int, unsigned int, 16>& linkSignature)
{
    SmallDenseMap<unsigned int, Value*, 16> linkArguments;
    Module* M = samplePhase->getParent();
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    LLVM3DBuilder<> builder(samplePhase->getContext(), ctx->platform.getPlatformInfo());
    
    FunctionUpgrader FuncUpgrader;
    
    FuncUpgrader.SetFunctionToUpgrade(samplePhase);
    
    // find all the phase inputs
    Function* phaseInput = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_PHASE_INPUT);
    Value* sampleIndex = FuncUpgrader.AddArgument("", builder.getInt32Ty());

    SmallVector<Type*, 10> funcSignature;
    funcSignature.push_back(builder.getInt32Ty());
    unsigned int inputLocation = 0;
    for(auto I = phaseInput->user_begin(), E = phaseInput->user_end(); I!=E; ++I)
    {
        if(Instruction* inst = dyn_cast<Instruction>(*I))
        {
            if(inst->getParent()->getParent() == samplePhase)
            {
                int index = (int)cast<ConstantInt>(inst->getOperand(0))->getZExtValue();
                Value* arg = nullptr;
                auto it = linkArguments.find(index);
                if(it != linkArguments.end())
                {
                    arg = it->second;
                }
                else
                {
                    arg = FuncUpgrader.AddArgument("", inst->getType());
                    funcSignature.push_back(inst->getType());
                    linkArguments[index] = arg;
                    linkSignature[index] = inputLocation++;
                }
                inst->replaceAllUsesWith(arg);
            }
        }
    }
    
    Function* newSamplePhase = FuncUpgrader.RebuildFunction();
    
    samplePhase->eraseFromParent();
    samplePhase = newSamplePhase;
    samplePhase->addFnAttr(llvm::Attribute::AlwaysInline);
    sampleIndex = FuncUpgrader.GetArgumentFromRebuild(sampleIndex);
    
    FuncUpgrader.Clean();
    
    // Replace sample index intrinsic
    Function* SGV = GenISAIntrinsic::getDeclaration(
        M, 
        GenISAIntrinsic::GenISA_DCL_SystemValue, 
        builder.getFloatTy());
    for(auto I = SGV->user_begin(), E = SGV->user_end(); I!=E; ++I)
    {
        if(Instruction* inst = dyn_cast<Instruction>(*I))
        {
            if(inst->getParent()->getParent() == samplePhase)
            {
                builder.SetInsertPoint(inst->getNextNode());
                SGVUsage usage = (SGVUsage)cast<ConstantInt>(inst->getOperand(0))->getZExtValue();
                if(usage == SAMPLEINDEX)
                {
                    Value* sampleIndexCast = builder.CreateBitCast(sampleIndex, builder.getFloatTy());
                    inst->replaceAllUsesWith(sampleIndexCast);
                }
            }
        }
    }

    Function* input = GenISAIntrinsic::getDeclaration(
        M, 
        GenISAIntrinsic::GenISA_DCL_inputVec, 
        builder.getFloatTy());
    for(auto I = input->user_begin(), E = input->user_end(); I!=E; ++I)
    {
        if(Instruction* inst = dyn_cast<Instruction>(*I))
        {
            if(inst->getParent()->getParent() == samplePhase)
            {
                e_interpolation interpolationMode =
                    (e_interpolation)llvm::cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue();
                bool perspective = true;
                if(interpolationMode == EINTERPOLATION_LINEARNOPERSPECTIVE ||
                    interpolationMode == EINTERPOLATION_LINEARNOPERSPECTIVECENTROID ||
                    interpolationMode == EINTERPOLATION_LINEARNOPERSPECTIVESAMPLE)
                {
                    perspective = false;
                }

                builder.SetInsertPoint(inst);
                Value* InputAtSamplePos = builder.CreateEvalSampleIndex(
                    inst->getOperand(0), sampleIndex, builder.getInt1(perspective));
                inst->replaceAllUsesWith(InputAtSamplePos);
            }
        }
    }
    
    Function* rtRead = GenISAIntrinsic::getDeclaration(
        M,
        GenISAIntrinsic::GenISA_RenderTargetRead);
    for (auto I = rtRead->user_begin(), E = rtRead->user_end(); I != E; ++I)
    {
        if (Instruction* inst = dyn_cast<Instruction>(*I))
        {
            if (inst->getParent()->getParent() == samplePhase)
            {
                builder.SetInsertPoint(inst->getNextNode());

                Value* rtReadSampleFreqargs[] = { 
                    inst->getOperand(0), 
                    sampleIndex 
                };
                Function* pRTSampleFreqIntrinsic = llvm::GenISAIntrinsic::getDeclaration(
                    M,
                    llvm::GenISAIntrinsic::GenISA_RenderTargetReadSampleFreq
                    );
                CallInst* rtReadSampleFreqInst = builder.CreateCall(
                    pRTSampleFreqIntrinsic, rtReadSampleFreqargs);

                inst->replaceAllUsesWith(rtReadSampleFreqInst);
            }
        }
    }

    Function* rtWrite = GenISAIntrinsic::getDeclaration(
        M,
        GenISAIntrinsic::GenISA_RTWrite,
        builder.getFloatTy());
    for(auto I = rtWrite->user_begin(), E = rtWrite->user_end(); I!=E; ++I)
    {
        if (RTWritIntrinsic* inst = dyn_cast<RTWritIntrinsic>(*I))
        {
            if (inst->getParent()->getParent() == samplePhase)
            {
                inst->setPerSample();
                inst->setSampleIndex(sampleIndex);
            }
        }
    }

    Function* dualBlend = GenISAIntrinsic::getDeclaration(
        M,
        GenISAIntrinsic::GenISA_RTDualBlendSource,
        builder.getFloatTy());
    for(auto I = dualBlend->user_begin(), E = dualBlend->user_end(); I != E; ++I)
    {
        if (RTDualBlendSourceIntrinsic* inst = dyn_cast<RTDualBlendSourceIntrinsic>(*I))
        {
            if(inst->getParent()->getParent() == samplePhase)
            {
                inst->setPerSample();
                inst->setSampleIndex(sampleIndex);
            }
        }
    }

    return samplePhase;
}
