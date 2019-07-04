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

/*========================== MadRoundDepAnalysis.cpp ==========================

MadRoundDepAnalysis tries to find the floor(currently) instructions that 
depend directly and indirectly(that does not change the mantissa part)
on the result of Mad(Mul + Add) instructions

This pass maintains a list of add/sub instructions that will be used in floor later:

            mul x a b
            add y x c
            floor z y

            ----or--- 

            mul x a b
            add y x c
            floor z abs(y)

If the case falls in either of the two cases, Mad optimisation is skipped because
small precision difference between Mul+add and Mad can be extrapolated by floor

=============================================================================*/

#include "Compiler/CISACodeGen/MadRoundDepAnalysis.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
using namespace IGC;
using namespace llvm;

char MadRoundDepAnalysis::ID = 0;
#define PASS_FLAG "MadRoundDepAnalysis"
#define PASS_DESCRIPTION "Checks is Mad output used in Round"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(MadRoundDepAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(MadRoundDepAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC
{


    MadRoundDepAnalysis::MadRoundDepAnalysis() : FunctionPass(ID)
    {
        initializeMadRoundDepAnalysisPass(*PassRegistry::getPassRegistry());
    }

    bool MadRoundDepAnalysis::runOnFunction(llvm::Function &F)
    {
        visit(F);
        return false;
    }

    void MadRoundDepAnalysis::visitIntrinsicInst(llvm::IntrinsicInst &I)
    {

        if (I.getIntrinsicID() == Intrinsic::floor)
        {
            Value* oper = I.getOperand(0);
            Instruction* source = dyn_cast<Instruction>(oper);
            IntrinsicInst* source_intrinsic = dyn_cast<IntrinsicInst>(oper);
            if (source_intrinsic && source_intrinsic->getIntrinsicID() == Intrinsic::fabs)
            {
                source = dyn_cast<Instruction>(source_intrinsic->getOperand(0));
            }
            if (source && (source->getOpcode() == Instruction::FAdd || source->getOpcode() == Instruction::FSub))
                m_RoundingDep.insert(source);
        }
        
    }

    bool MadRoundDepAnalysis::RoundingDependsOnInst(Instruction* inst)
    {
        bool retval = m_RoundingDep.find(inst) != m_RoundingDep.end();
        return retval;
    }




}