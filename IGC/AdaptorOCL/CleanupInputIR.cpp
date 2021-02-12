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

// CleanIGCInput.cpp
//     FE may generate many non-user functions. Those functions, especially under -O0,
//     could make IGC take a very long time to compile under some circumstances. Those
//     non-user functions are part of FE's implementation, and thus are not subject to
//     -O0 or any other flags required by users. To shorten compiling time and to generate
//     better code, FE should make those "implementation" as optimal as possible.
//
//     This file is to do this for FE, and should be removed once FE has done it.
//

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/LLVMContext.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Pass.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Transforms/InstCombine/InstCombineWorklist.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "common/LLVMWarningsPop.hpp"

#include "CleanupInputIR.hpp"
#include "common/LLVMUtils.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/Optimizer/IGCInstCombiner/IGCInstructionCombining.hpp"
#include "llvmWrapper/IR/CallSite.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Analysis/InlineCost.h"

#include <utility>


using namespace llvm;
using namespace IGC::IGCMD;

// forward decl
namespace IGC
{
    Pass* createCleanupInlinerPassAllFuncNotInSet(DenseSet<Function*>* pFS);
    Pass* createCleanupInlinerPassAllCalleeNotInSet(DenseSet<Function*>* pFS);
}

namespace
{
    // Custom inliner for Cleanup input IR.
    class CleanupInliner : public LegacyInlinerBase
    {
        enum class InlineControl {
            // Inline every call whose caller and callee are not in m_funcSet.
            InlineAllFuncNotInSet,
            // Inline every call whose caller is in m_funcSet, but callee isn't
            InlineAllCalleeNotInSet,
            NoInline,
        };

        InlineControl m_inlineControl;
        const DenseSet<Function*>* m_funcSet;

        void setInlineControl(InlineControl IC)
        {
            m_inlineControl = IC;
        }

    public:
        static char ID; // Pass identification, replacement for typeid

        CleanupInliner(DenseSet<Function*> *pFS = nullptr)
            : LegacyInlinerBase(ID, /*InsertLifetime*/ false),
            m_inlineControl(InlineControl::NoInline),
            m_funcSet(pFS)
        {}

        InlineCost getInlineCost(IGCLLVM::CallSiteRef CS) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override;
        bool runOnSCC(CallGraphSCC& SCC) override;
        StringRef getPassName() const override { return "Cleanup Input IR"; }

        friend Pass* IGC::createCleanupInlinerPassAllFuncNotInSet(DenseSet<Function*>* pFS);
        friend Pass* IGC::createCleanupInlinerPassAllCalleeNotInSet(DenseSet<Function*>* pFS);
    };

    // Remove llvm.dbg intrinsics calls
    class RemoveDbgCall : public FunctionPass
    {
        const DenseSet<Function*>* m_funcSet;
    public:
        static char ID; // Pass identification, replacement for typeid

        RemoveDbgCall(DenseSet<Function*>* pFS = nullptr)
            : FunctionPass(ID),
              m_funcSet(pFS)
        {}

        void getAnalysisUsage(AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
        }
        StringRef getPassName() const override { return "Remove llvm dbg intrinsic calls"; }
        bool runOnFunction(Function& F) override;
    };
} // namespace

#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

#define PASS_FLAG "cleanupinputir_inliner"
#define PASS_DESCRIPTION "Custom inliner to inline all non-user functions"
IGC_INITIALIZE_PASS_BEGIN(CleanupInliner, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(CleanupInliner, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
#undef PASS_FLAG
#undef PASS_DESCRIPTION

#define PASS_FLAG "cleanupinputir_removedbg"
#define PASS_DESCRIPTION "Remove all llvm dbg intrinsic calls"
IGC_INITIALIZE_PASS_BEGIN(RemoveDbgCall, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(RemoveDbgCall, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
#undef PASS_FLAG
#undef PASS_DESCRIPTION

char CleanupInliner::ID = 0;

void CleanupInliner::getAnalysisUsage(AnalysisUsage& AU) const
{
    LegacyInlinerBase::getAnalysisUsage(AU);
}

bool CleanupInliner::runOnSCC(CallGraphSCC& SCC)
{
    if (m_inlineControl == InlineControl::NoInline || m_funcSet == nullptr)
    {
        return false;
    }
    return LegacyInlinerBase::runOnSCC(SCC);
}

InlineCost CleanupInliner::getInlineCost(IGCLLVM::CallSiteRef CS)
{
    IGC_ASSERT(m_funcSet);

    Function* Callee = CS.getCalledFunction();
    Function* Caller = CS.getCaller();

    if (Callee && !Callee->isDeclaration()
#if LLVM_VERSION_MAJOR >= 11
        && isInlineViable(*Callee).isSuccess()
#else
        && isInlineViable(*Callee)
#endif
        )
    {
        if (m_inlineControl == InlineControl::InlineAllFuncNotInSet &&
            m_funcSet->count(Callee) == 0 &&
            m_funcSet->count(Caller) == 0)
        {
            return IGCLLVM::InlineCost::getAlways();
        }

        if (m_inlineControl == InlineControl::InlineAllCalleeNotInSet &&
            m_funcSet->count(Callee) == 0 &&
            m_funcSet->count(Caller) > 0)
        {
            return IGCLLVM::InlineCost::getAlways();
        }
    }
    return IGCLLVM::InlineCost::getNever();
}

char RemoveDbgCall::ID = 0;

bool RemoveDbgCall::runOnFunction(Function& F)
{
    // For any function that is not in the set, remove llvm.dbg calls
    bool changed = false;
    if (m_funcSet->count(&F) == 0)
    {
        auto Next = inst_begin(F);
        for (auto II = Next, IE = inst_end(F); II != IE; II = Next)
        {
            ++Next;
            Instruction* I = &*II;
            if (IntrinsicInst* IntriInst = dyn_cast<IntrinsicInst>(I))
            {
                Function* Callee = IntriInst->getCalledFunction();
                if (Callee && Callee->getName().startswith("llvm.dbg."))
                {
                    I->eraseFromParent();
                    changed = true;
                }
            }
        }
    }
    return changed;
}

static bool isNonUserFunc(Function* F)
{
    StringRef FN = F->getName();

    // Guess which would be a non-user function
    if (FN.contains_lower("sycl") &&
        ((FN.contains_lower("accessor")) ||
         (FN.contains("__spirv")) ||
         (FN.contains("getDelinearizedIndex")) ||
         (FN.contains("detail") && (FN.contains("array") || FN.contains("declptr"))) ||
         (FN.contains("enable_if") && FN.contains("type")) ||
         (FN.contains("Builder") && FN.contains("getElement"))))
    {
        return true;
    }
    return false;
}

namespace IGC
{
    Pass* createCleanupInlinerPassAllFuncNotInSet(DenseSet<Function*>* pFS)
    {
        initializeCleanupInlinerPass(*PassRegistry::getPassRegistry());
        CleanupInliner* P = new CleanupInliner(pFS);
        P->setInlineControl(CleanupInliner::InlineControl::InlineAllFuncNotInSet);
        return P;
    }

    Pass* createCleanupInlinerPassAllCalleeNotInSet(DenseSet<Function*>* pFS)
    {
        initializeCleanupInlinerPass(*PassRegistry::getPassRegistry());
        CleanupInliner* P = new CleanupInliner(pFS);
        P->setInlineControl(CleanupInliner::InlineControl::InlineAllCalleeNotInSet);
        return P;
    }

    Pass* createRemoveDbgCallPass(DenseSet<Function*>* pFS)
    {
        initializeRemoveDbgCallPass(*PassRegistry::getPassRegistry());
        return new RemoveDbgCall(pFS);
    }

    void CleanupInputIR(OpenCLProgramContext* pContext)
    {
        DenseSet<Function*> userFuncSet;
        DenseMap<Function*, AttributeList> origUserFuncAttrs;
        DenseMap<Function*, int> allFuncs;

        IGCLLVM::Module* M = pContext->getModule();
        MetaDataUtils* pMdUtils = pContext->getMetaDataUtils();

        // Handle all user functions
        for (auto& F : *M)
        {
            if (F.isDeclaration())
            {
                continue;
            }
            bool isKernel = isEntryFunc(pMdUtils, &F);
            bool isNonUser = isNonUserFunc(&F);
            int key = isKernel ? 1 : (isNonUser ? 2 : 0);
            allFuncs.insert(std::make_pair(&F, key));
        }

        // Non-user function shall not invoke users functions.
        bool change = true;
        while (change)
        {
            change = false;
            for (auto& II : allFuncs)
            {
                Function* F = II.first;
                if (II.second != 0) continue;

                // Check its users, if any user is not user function, this
                // function will not be user function.
                for (auto UI = F->user_begin(), UE = F->user_end(); UI != UE; ++UI)
                {
                    CallInst* Call = dyn_cast<CallInst>(*UI);
                    Function* Caller = (Call ? Call->getParent()->getParent() : nullptr);
                    auto callerII = allFuncs.find(Caller);
                    if (callerII != allFuncs.end() && callerII->second == 2)
                    {
                        II.second = 2;
                        change = true;
                        break;
                    }
                }
            }
        }

        // Now, non-user functions are determined. Prepare for clean-up.
        for (auto& II : allFuncs)
        {
            Function* F = II.first;
            if (II.second == 2)
            {
                // Non-user functions: set internal linkage and optimize fully
                F->setLinkage(GlobalValue::InternalLinkage);
                F->removeFnAttr("optnone");
            }
            else
            {
                origUserFuncAttrs.insert(std::make_pair(F, F->getAttributes()));
                userFuncSet.insert(F);

                F->addFnAttr("optnone");
                F->removeFnAttr("noinline");
            }
        }
        allFuncs.clear();

        // Now, do clean-up
        // First, initialize some analysis passes
        initializeTargetLibraryInfoWrapperPassPass(*PassRegistry::getPassRegistry());
        initializeAssumptionCacheTrackerPass(*PassRegistry::getPassRegistry());
        initializeProfileSummaryInfoWrapperPassPass(*PassRegistry::getPassRegistry());
        initializeCallGraphWrapperPassPass(*PassRegistry::getPassRegistry());

        IGCPassManager mpm(pContext, "CleanupInputIR");
        mpm.add(createRemoveDbgCallPass(&userFuncSet));
        mpm.add(createCleanupInlinerPassAllFuncNotInSet(&userFuncSet));
        mpm.add(createLoopRotatePass());
        mpm.add(createLoopUnrollPass());
        mpm.add(createSROAPass());
        mpm.add(createCFGSimplificationPass());
        mpm.add(createIGCInstructionCombiningPass());
        mpm.add(createAggressiveDCEPass());
        mpm.add(createCFGSimplificationPass());
        //mpm.add(createCleanupInlinerPassAllCalleeNotInSet(&userFuncSet));
        mpm.run(*M);

        // restore the original attrbutes
        for (auto& II : origUserFuncAttrs)
        {
            Function* aF = II.first;
            aF->setAttributes(II.second);
        }
    }

} // namespace IGC
