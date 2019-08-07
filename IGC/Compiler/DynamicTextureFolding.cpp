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

#include "Compiler/IGCPassSupport.h"
#include "Compiler/InitializePasses.h"
#include "Compiler/CodeGenPublic.h"
#include "common/secure_mem.h"
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
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    ModuleMetaData* modMD = ctx->getModuleMetaData();

    // nothing to fold
    if (modMD->inlineDynTextures.size() == 0)
    {
        return;
    }

    llvm::Value* textureArgValue = cast<SampleIntrinsic>(&I)->getTextureValue();
    assert(textureArgValue);

    uint addrSpace = textureArgValue->getType()->getPointerAddressSpace();
    bool directIdx = false;
    uint textureIndex = 0;
    DecodeAS4GFXResource(addrSpace, directIdx, textureIndex);

    // if the current texture index is found in modMD as uniform texture, replace the texture load/sample as constant.
    auto it = modMD->inlineDynTextures.find(textureIndex);
    if (it != modMD->inlineDynTextures.end())
    {
        if ((&I)->getType()->isIntOrIntVectorTy())
        {
            (&I)->replaceAllUsesWith(ConstantInt::get((&I)->getType(), it->second));
        }
        else
        {
            (&I)->replaceAllUsesWith(ConstantFP::get((&I)->getType(), *(float*) & (it->second)));
        }
    }
}

void DynamicTextureFolding::visitCallInst(CallInst& I)
{
    if (GenIntrinsicInst * pCall = dyn_cast<GenIntrinsicInst>(&I))
    {
        auto ID = pCall->getIntrinsicID();
        if (ID == GenISAIntrinsic::GenISA_sampleptr ||
            ID == GenISAIntrinsic::GenISA_sampleLptr ||
            ID == GenISAIntrinsic::GenISA_sampleBptr ||
            ID == GenISAIntrinsic::GenISA_sampleDptr)
        {
            FoldSingleTextureValue(I);
            //todo: FoldUNormTexture(I);
        }
    }
}

bool DynamicTextureFolding::runOnFunction(Function& F)
{
    visit(F);
    return false;
}
