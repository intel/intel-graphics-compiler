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
#include "ComputeShaderLowering.hpp"
#include "IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include "common/LLVMWarningsPop.hpp"
#include "helper.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

class ComputeShaderLowering : public FunctionPass
{
public:
    ComputeShaderLowering() : FunctionPass(ID) {}
    virtual bool runOnFunction(Function& F) override;
    virtual llvm::StringRef getPassName() const override
    {
        return "ComputeShaderLowering";
    }
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<CodeGenContextWrapper>();
    }
    static char ID;
protected:
    Function* m_function = nullptr;
};

char ComputeShaderLowering::ID = 0;

bool ComputeShaderLowering::runOnFunction(Function& F)
{
    for (auto BI = F.begin(), BE = F.end(); BI != BE; BI++)
    {
        for (auto II = BI->begin(), IE = BI->end(); II != IE; II++)
        {
            if (GenIntrinsicInst * inst = dyn_cast<GenIntrinsicInst>(II))
            {
                if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_DCL_SystemValue)
                {
                    II = AdjustSystemValueCall(inst)->getIterator();
                }
            }
        }
    }

    return true;
}

namespace IGC {
#define PASS_FLAG "igc-compute-shader-lowering"
#define PASS_DESCRIPTION "This is the compute shader lowering pass "
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
    IGC_INITIALIZE_PASS_BEGIN(ComputeShaderLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_END(ComputeShaderLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

        FunctionPass* CreateComputeShaderLowering()
    {
        return new ComputeShaderLowering();
    }
}
