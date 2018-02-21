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
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CustomSafeOptPass.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

//#define DEBUG_DISCARDONALPHA

namespace {

/**
 * Check sampler result and do early out using discard based on blend state.
 */
class DiscardOnAlpha : public FunctionPass
{
public:
    static char ID;

    DiscardOnAlpha() : FunctionPass(ID)
    {
        initializeDiscardOnAlphaPass(*PassRegistry::getPassRegistry());
    }

    void getAnalysisUsage(AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<CodeGenContextWrapper>();
        AU.addRequired<MetaDataUtilsWrapper>();
    }
    
    virtual llvm::StringRef getPassName() const override { return "DiscardOnAlpha"; }
    bool runOnFunction(Function& F) override;
    
private:
    CodeGenContext* m_cgCtx;
    ModuleMetaData* m_modMD;
    Module* m_module;

    bool processOutput(GenIntrinsicInst* out);
    Instruction* isFMul(Value* v)
    {
        Instruction* inst = dyn_cast<Instruction>(v);
        if (inst && inst->getOpcode() == Instruction::FMul)
        {
            return inst;
        }
        return nullptr;
    }

    void createDiscard(Instruction* extract, bool check0);
};

} // namespace

char DiscardOnAlpha::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(DiscardOnAlpha,
    "DiscardOnAlpha", "DiscardOnAlpha", false, false)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper);
IGC_INITIALIZE_PASS_END(DiscardOnAlpha, 
    "DiscardOnAlpha", "DiscardOnAlpha", false, false)

FunctionPass* IGC::createDiscardOnAlphaPass()
{
    return new DiscardOnAlpha();
}

bool DiscardOnAlpha::runOnFunction(Function& F)
{
    // Skip non-kernel function.                                                  
    IGCMD::MetaDataUtils *mdu = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto FII = mdu->findFunctionsInfoItem(&F);
    if (FII == mdu->end_FunctionsInfo())
        return false;

    m_cgCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_modMD = m_cgCtx->getModuleMetaData();
    m_module = F.getParent();

    assert(m_cgCtx->type == ShaderType::PIXEL_SHADER);
    GenIntrinsicInst* output0 = nullptr;

    // render target write 0 only for now
    for (auto& BI : F)
    {
        for (auto& II : BI)
        {
            GenIntrinsicInst* out = dyn_cast<GenIntrinsicInst>(
                &II, GenISAIntrinsic::GenISA_OUTPUT);

            if (out)
            {
                assert(isa<ConstantInt>(out->getOperand(4)) &&
                    isa<ConstantInt>(out->getOperand(5)));
                ShaderOutputType otype = (ShaderOutputType)
                    llvm::cast<llvm::ConstantInt>(out->getOperand(4))->getZExtValue();
                unsigned rtIdx = (unsigned)
                    llvm::cast<llvm::ConstantInt>(out->getOperand(4))->getZExtValue();
                if (otype == SHADER_OUTPUT_TYPE_DEFAULT && rtIdx == 0)
                {
                    output0 = out;
                }
                else
                {
                    return false;
                }
            }
        }
    }
    if (output0 == nullptr)
    {
        return false;
    }

    bool changed = processOutput(output0);
    
#ifdef DEBUG_DISCARDONALPHA
    if (changed)
    {
        DumpLLVMIR(m_cgCtx, "discardonalpha");
    }
#endif

    return changed;
}

bool DiscardOnAlpha::processOutput(GenIntrinsicInst* out)
{
    Instruction* alpha = isFMul(out->getArgOperand(3));
    bool check0;
    bool changed = false;

    if (m_modMD->psInfo.blendOptimizationMode.size() == 0)
        return false;

    switch (m_modMD->psInfo.blendOptimizationMode[0])
    {
    case USC::BLEND_OPTIMIZATION_SRC_ALPHA:
    case USC::BLEND_OPTIMIZATION_SRC_ALPHA_DISCARD_ONLY:
        check0 = true;
        break;

    case USC::BLEND_OPTIMIZATION_INV_SRC_ALPHA:
        check0 = false;
        break;

    default:
        return false;
    }

    if (alpha)
    {
        // search back all fmul for sample result
        std::vector<Instruction*> opnds;
        SmallPtrSet<Value*, 32> visited;
        unsigned li = 0;

        opnds.push_back(alpha);
        visited.insert(alpha);

        while (li < opnds.size())
        {
            Instruction* inst = opnds[li];
            li++;

            for (unsigned i = 0; i < inst->getNumOperands(); i++)
            {
                Value* v = inst->getOperand(i);

                if (visited.count(v))
                {
                    continue;
                }

                if (ExtractElementInst* extract = dyn_cast<ExtractElementInst>(v))
                {
                    GenIntrinsicInst* intrin = dyn_cast<GenIntrinsicInst>(
                        extract->getVectorOperand());
                    if (intrin && isSampleInstruction(intrin))
                    {
                        createDiscard(extract, check0);
                        m_modMD->psInfo.forceEarlyZ = true;
                        changed = true;
                        return changed;
                    }
                }

                Instruction* fmul = isFMul(v);
                if (fmul && !visited.count(fmul))
                {
                    opnds.push_back(fmul);
                }

                visited.insert(v);
            }
        }
    }
    
    return changed;
}

void DiscardOnAlpha::createDiscard(Instruction* extract, bool check0)
{
    if (extract != nullptr)
    {
        Function* fDiscard = GenISAIntrinsic::getDeclaration(m_module,
            GenISAIntrinsic::GenISA_discard);

        IRBuilder<> irb(extract->getNextNode());
        Value* cond;
        if (check0)
        {
            // check against 0
            cond = irb.CreateFCmpOEQ(extract, ConstantFP::get(extract->getType(), 0.0));
        }
        else
        {
            // check against 1
            cond = irb.CreateFCmpOEQ(extract, ConstantFP::get(extract->getType(), 1.0));
        }
        irb.CreateCall(fDiscard, { cond });
    }
}
