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

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/debug/Debug.hpp"
#include "common/igc_regkeys.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvmWrapper/IR/Function.h>
#include <llvm/IR/CFG.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/IR/Constants.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include <string>
#include <stack>
#include <sstream>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace IGC::Debug;

static cl::opt<bool> PrintWiaCheck(
    "print-wia-check", cl::init(false), cl::Hidden,
    cl::desc("Debug wia-check analysis"));

// Register pass to igc-opt
#define PASS_FLAG "igc-wi-analysis"
#define PASS_DESCRIPTION "WIAnalysis provides work item dependency info"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(WIAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(TranslationTable)
IGC_INITIALIZE_PASS_END(WIAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char WIAnalysis::ID = 0;

WIAnalysis::WIAnalysis() : FunctionPass(ID)
{
    initializeWIAnalysisPass(*PassRegistry::getPassRegistry());
}

const unsigned int WIAnalysisRunner::MinIndexBitwidthToPreserve = 16;

/// Define shorter names for dependencies, for clarity of the conversion maps
#define UNI WIAnalysis::UNIFORM
#define SEQ WIAnalysis::CONSECUTIVE
#define PTR WIAnalysis::PTR_CONSECUTIVE
#define STR WIAnalysis::STRIDED
#define RND WIAnalysis::RANDOM

static const char* const dep_str[] = {
  "uniform",
  "consecu",
  "p_conse",
  "strided",
  "random "
};

/// Dependency maps (define output dependency according to 2 input deps)

static const WIAnalysis::WIDependancy
add_conversion[WIAnalysis::NumDeps][WIAnalysis::NumDeps] = {
    /*          UNI, SEQ, PTR, STR, RND */
    /* UNI */  {UNI, SEQ, PTR, STR, RND},
    /* SEQ */  {SEQ, STR, STR, STR, RND},
    /* PTR */  {PTR, STR, STR, STR, RND},
    /* STR */  {STR, STR, STR, STR, RND},
    /* RND */  {RND, RND, RND, RND, RND}
};

static const WIAnalysis::WIDependancy
sub_conversion[WIAnalysis::NumDeps][WIAnalysis::NumDeps] = {
    /*          UNI, SEQ, PTR, STR, RND */
    /* UNI */  {UNI, STR, RND, RND, RND},
    /* SEQ */  {SEQ, RND, RND, RND, RND},
    /* PTR */  {PTR, RND, RND, RND, RND},
    /* STR */  {STR, RND, RND, RND, RND},
    /* RND */  {RND, RND, RND, RND, RND}
};


static const WIAnalysis::WIDependancy
mul_conversion[WIAnalysis::NumDeps][WIAnalysis::NumDeps] = {
    /*          UNI, SEQ, PTR, STR, RND */
    /* UNI */  {UNI, STR, STR, STR, RND},
    /* SEQ */  {STR, RND, RND, RND, RND},
    /* PTR */  {STR, RND, RND, RND, RND},
    /* STR */  {STR, RND, RND, RND, RND},
    /* RND */  {RND, RND, RND, RND, RND}
};

static const WIAnalysis::WIDependancy
select_conversion[WIAnalysis::NumDeps][WIAnalysis::NumDeps] = {
    /*          UNI, SEQ, PTR, STR, RND */
    /* UNI */  {UNI, STR, STR, STR, RND},
    /* SEQ */  {STR, SEQ, STR, STR, RND},
    /* PTR */  {STR, STR, PTR, STR, RND},
    /* STR */  {STR, STR, STR, STR, RND},
    /* RND */  {RND, RND, RND, RND, RND}
};

static const WIAnalysis::WIDependancy
gep_conversion[WIAnalysis::NumDeps][WIAnalysis::NumDeps] = {
    /* ptr\index UNI, SEQ, PTR, STR, RND */
    /* UNI */  {UNI, PTR, RND, RND, RND},
    /* SEQ */  {RND, RND, RND, RND, RND},
    /* PTR */  {PTR, RND, RND, RND, RND},
    /* STR */  {RND, RND, RND, RND, RND},
    /* RND */  {RND, RND, RND, RND, RND}
};

namespace IGC {
    /// @Brief, given a conditional branch and its immediate post dominator,
    /// find its influence-region and partial joins within the influence region
    class BranchInfo
    {
    public:
        BranchInfo(const IGCLLVM::TerminatorInst* inst, const llvm::BasicBlock* ipd);

        void print(llvm::raw_ostream& OS) const;

        const IGCLLVM::TerminatorInst* cbr;
        const llvm::BasicBlock* full_join;
        llvm::DenseSet<llvm::BasicBlock*> influence_region;
        llvm::SmallPtrSet<llvm::BasicBlock*, 4> partial_joins;
        llvm::BasicBlock* fork_blk;
    };
} // namespace IGC

void WIAnalysisRunner::print(raw_ostream& OS, const Module*) const
{
    DenseMap<BasicBlock*, int> BBIDs;
    int id = 0;
    for (Function::iterator I = m_func->begin(), E = m_func->end(); I != E; ++I, ++id) {
        BasicBlock* BB = &*I;
        BBIDs[BB] = id;
    }

    std::stringstream ss;
    ss << "WIAnalysis: " << m_func->getName().str();
    Banner(OS, ss.str());

    OS << "Args: \n";
    for (Function::arg_iterator I = m_func->arg_begin(), E = m_func->arg_end();
        I != E; ++I) {
        Value* AVal = &*I;

        if (m_depMap.GetAttributeWithoutCreating(AVal) != m_depMap.end())
            OS << "    " << dep_str[m_depMap.GetAttributeWithoutCreating(AVal)] << " " << *AVal << "\n";
        else
            OS << "  unknown " << *AVal << "\n";

    }
    OS << "\n";

    for (Function::iterator I = m_func->begin(), E = m_func->end(); I != E; ++I) {
        BasicBlock* BB = &*I;
        OS << "BB:" << BBIDs[BB];
        if (BB->hasName())
            OS << " " << BB->getName();
        OS << "       ; preds =";
        bool isFirst = true;
        for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI) {
            BasicBlock* pred = *PI;
            OS << ((isFirst) ? " " : ", ") << "BB:" << BBIDs[pred] << "  ";
            if (pred->hasName())
                OS << pred->getName();
            isFirst = false;
        }
        OS << "\n";
        for (BasicBlock::iterator it = BB->begin(), ie = BB->end(); it != ie; ++it) {
            Instruction* I = &*it;
            if (m_depMap.GetAttributeWithoutCreating(I) != m_depMap.end())
            {
                OS << "  " << dep_str[m_depMap.GetAttributeWithoutCreating(I)] << " " << *I;
            }
            else
            {
                OS << "  unknown " << *I;
            }
            if (I->isTerminator()) {
                IGCLLVM::TerminatorInst* TI = dyn_cast<IGCLLVM::TerminatorInst>(I);
                OS << " [";
                for (unsigned i = 0, e = TI->getNumSuccessors(); i < e; ++i) {
                    BasicBlock* succ = TI->getSuccessor(i);
                    OS << " BB:" << BBIDs[succ];
                }
                OS << " ]";
            }
            OS << "\n";
        }
        OS << "\n";
    }
}

void WIAnalysisRunner::dump() const
{
    auto name =
        DumpName(IGC::Debug::GetShaderOutputName())
        .Hash(m_CGCtx->hash)
        .Type(m_CGCtx->type)
        .Pass("WIAnalysis")
        .Extension("txt");
    print(Dump(name, DumpType::DBG_MSG_TEXT).stream());
}

void WIAnalysisRunner::init(
    llvm::Function* F,
    llvm::PostDominatorTree* PDT,
    IGC::IGCMD::MetaDataUtils* MDUtils,
    IGC::CodeGenContext* CGCtx,
    IGC::ModuleMetaData* ModMD,
    IGC::TranslationTable* TransTable)
{
    m_func = F;
    this->PDT = PDT;
    m_pMdUtils = MDUtils;
    m_CGCtx = CGCtx;
    m_ModMD = ModMD;
    m_TT = TransTable;
}

bool WIAnalysisRunner::run()
{
    auto& F = *m_func;
    if (m_pMdUtils->findFunctionsInfoItem(&F) == m_pMdUtils->end_FunctionsInfo())
        return false;

    m_depMap.Initialize(m_TT);
    m_TT->RegisterListener(&m_depMap);

    m_changed1.clear();
    m_changed2.clear();
    m_pChangedNew = &m_changed1;
    m_pChangedOld = &m_changed2;
    m_ctrlBranches.clear();

    m_backwardList.clear();
    m_storeDepMap.clear();
    m_allocaDepMap.clear();

    updateArgsDependency(&F);

    if (!IGC_IS_FLAG_ENABLED(DisableUniformAnalysis))
    {
        // Compute the  first iteration of the WI-dep according to ordering
        // instructions this ordering is generally good (as it ususally correlates
        // well with dominance).
        inst_iterator it = inst_begin(F);
        inst_iterator  e = inst_end(F);
        for (; it != e; ++it)
        {
            calculate_dep(&*it);
        }

        // Recursively check if WI-dep changes and if so reclaculates
        // the WI-dep and marks the users for re-checking.
        // This procedure is guranteed to converge since WI-dep can only
        // become less unifrom (uniform->consecutive->ptr->stride->random).
        updateDeps();
    }

    genSpecificBackwardUpdate();
    if (PrintWiaCheck)
    {
        print(ods());
    }
    return false;
}

bool WIAnalysis::runOnFunction(Function& F)
{
    auto* MDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto* PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
    auto* CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    auto* ModMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    auto* pTT = &getAnalysis<TranslationTable>();

    Runner.init(&F, PDT, MDUtils, CGCtx, ModMD, pTT);
    return Runner.run();
}

void WIAnalysisRunner::updateDeps()
{
    // As lonst as we have values to update
    while (!m_pChangedNew->empty())
    {
        // swap between changedSet pointers - recheck the newChanged(now old)
        std::swap(m_pChangedNew, m_pChangedOld);
        // clear the newChanged set so it will be filled with the users of
        // instruction which their WI-dep canged during the current iteration
        m_pChangedNew->clear();

        // update all changed values
        std::vector<const Value*>::iterator it = m_pChangedOld->begin();
        std::vector<const Value*>::iterator e = m_pChangedOld->end();
        for (; it != e; ++it)
        {
            // remove first instruction
            // calculate its new dependencey value
            calculate_dep(*it);
        }
    }
}

bool WIAnalysisRunner::isInstructionSimple(const Instruction* inst)
{
    // avoid changing cb load to sampler load, since sampler load
    // has longer latency.
    if (isa<LoadInst>(inst))
    {
        return false;
    }

    if (isa<UnaryInstruction>(inst) ||
        isa<BinaryOperator>(inst) ||
        isa<CmpInst>(inst) ||
        isa<SelectInst>(inst))
    {
        return true;
    }
    if (IsMathIntrinsic(GetOpCode((Instruction*)inst)))
    {
        return true;
    }

    return false;
}

bool WIAnalysisRunner::needToBeUniform(const Value* val)
{
    for (auto UI = val->user_begin(), E = val->user_end(); UI != E; ++UI)
    {
        if (const RTWritIntrinsic * use = dyn_cast<RTWritIntrinsic>(*UI))
        {
            if (use->getSampleIndex() == val || use->getBlendStateIndex() == val)
            {
                return true;
            }
        }
        // TODO add sampler cases
    }
    return false;
}

bool WIAnalysisRunner::allUsesRandom(const Value* val)
{
    for (auto UI = val->user_begin(), E = val->user_end(); UI != E; ++UI)
    {
        const Value* use = (*UI);
        if (getDependency(use) != WIAnalysis::RANDOM)
        {
            return false;
        }
    }
    return true;
}

void WIAnalysisRunner::genSpecificBackwardUpdate()
{
    while (!m_backwardList.empty())
    {
        const Instruction* inst = m_backwardList.back();
        m_backwardList.pop_back();
        for (unsigned i = 0; i < inst->getNumOperands(); ++i)
        {
            Instruction* def = dyn_cast<Instruction>(inst->getOperand(i));
            if (def && getDependency(def) == WIAnalysis::UNIFORM && allUsesRandom(def) && !needToBeUniform(def))
            {

                // if it is cheap and easy to mark it as RANDOM
                if (isInstructionSimple(def))
                {
                    m_depMap.SetAttribute(def, WIAnalysis::RANDOM);
                }
            }
        }
    }
}

void WIAnalysisRunner::updateArgsDependency(llvm::Function* pF)
{
    /*
    Function Signature: define void @kernel(
        [OCL function args...],
        [implicit args...],
        [push analysis args...])

    Example push analysis args:
        float %urb_read_0, float %urb_read_1, float %urb_read_2, float %urb_read_3, float %urb_read_4

    Metadata Generated:
    !igc.pushanalysis.wi.info = !{!3, !4, !5, !6, !7}

    !3 = metadata !{metadata !"urb_read_0", i32 0, i32 4}
    !4 = metadata !{metadata !"urb_read_1", i32 1, i32 4}
    !5 = metadata !{metadata !"urb_read_2", i32 2, i32 4}
    !6 = metadata !{metadata !"urb_read_3", i32 3, i32 4}
    !7 = metadata !{metadata !"urb_read_4", i32 4, i32 4}

    Assumption is that the order of metadata matches the order of arguments in function.
    */

    // For a subroutine, conservatively assume that all user provided arguments
    // are random. Note that all other functions are treated as kernels.
    // To enable subroutine for other FEs, we need to update this check.
    bool IsSubroutine = !isEntryFunc(m_pMdUtils, pF) || isNonEntryMultirateShader(pF);

    ImplicitArgs implicitArgs(*pF, m_pMdUtils);
    int implicitArgStart = (unsigned)(IGCLLVM::GetFuncArgSize(pF)
        - implicitArgs.size()
        - (IsSubroutine ? 0 : m_ModMD->pushInfo.pushAnalysisWIInfos.size()));
    IGC_ASSERT(implicitArgStart >= 0 && "Function arg size does not match meta data and push args.");

    llvm::Function::arg_iterator ai, ae;
    ai = pF->arg_begin();
    ae = pF->arg_end();

    // 1. add all kernel function args as uniform, or
    //    add all subroutine function args as random
    for (int i = 0; i < implicitArgStart; ++i, ++ai)
    {
        IGC_ASSERT(ai != ae);
        incUpdateDepend(&(*ai), IsSubroutine ? WIAnalysis::RANDOM : WIAnalysis::UNIFORM);
    }

    // 2. add implicit args
    //    By default, local IDs are not uniform. But if we know that the runtime dispatchs
    //    order (intel_reqd_workgroup_walk_order()) and work group size (reqd_work_group_size()),
    //    we may derive that some of local IDs are uniform.
    bool localX_uniform = false, localY_uniform = false, localZ_uniform = false;
    // DispatchOCLWGInLinearOrder should be removed after testing the guarded code.
    if (!IsSubroutine &&
        IGC_IS_FLAG_ENABLED(DispatchOCLWGInLinearOrder))
    {
        checkLocalIdUniform(pF, localX_uniform, localY_uniform, localZ_uniform);
    }

    for (unsigned i = 0; i < implicitArgs.size(); ++i, ++ai)
    {
        IGC_ASSERT(ai != ae);
        const ImplicitArg& iArg = implicitArgs[ai->getArgNo() - implicitArgStart];
        WIAnalysis::WIDependancy dependency = iArg.getDependency();
        if ((localX_uniform && iArg.getArgType() == ImplicitArg::ArgType::LOCAL_ID_X) ||
            (localY_uniform && iArg.getArgType() == ImplicitArg::ArgType::LOCAL_ID_Y) ||
            (localZ_uniform && iArg.getArgType() == ImplicitArg::ArgType::LOCAL_ID_Z)) {
            dependency = WIAnalysis::UNIFORM;
        }

        incUpdateDepend(&(*ai), dependency);
    }

    // 3. add push analysis args
    if (!IsSubroutine)
    {
        for (unsigned i = 0; i < m_ModMD->pushInfo.pushAnalysisWIInfos.size(); ++i, ++ai)
        {
            IGC_ASSERT(ai != ae);
            WIAnalysis::WIDependancy dependency =
                static_cast<WIAnalysis::WIDependancy>(m_ModMD->pushInfo.pushAnalysisWIInfos[i].argDependency);
            incUpdateDepend(&(*ai), dependency);
        }
    }
}

void WIAnalysis::print(
    llvm::raw_ostream& OS, const llvm::Module* M) const
{
    Runner.print(OS, M);
}

void WIAnalysis::dump() const
{
    Runner.dump();
}

void WIAnalysis::incUpdateDepend(const llvm::Value* val, WIDependancy dep)
{
    Runner.incUpdateDepend(val, dep);
}

WIAnalysis::WIDependancy WIAnalysis::whichDepend(const llvm::Value* val)
{
    return Runner.whichDepend(val);
}

bool WIAnalysis::isUniform(const llvm::Value* val)
{
    return Runner.isUniform(val);
}

bool WIAnalysis::insideDivergentCF(const llvm::Value* val)
{
    return Runner.insideDivergentCF(val);
}

WIAnalysis::WIDependancy WIAnalysisRunner::whichDepend(const Value* val)
{
    IGC_ASSERT_MESSAGE(m_pChangedNew->empty(), "set should be empty before query");
    IGC_ASSERT_MESSAGE(nullptr != val, "Bad value");
    if (isa<Constant>(val))
    {
        return WIAnalysis::UNIFORM;
    }
    auto EL = m_depMap.GetAttributeWithoutCreating(val);
    if (IGC_IS_FLAG_ENABLED(DisableUniformAnalysis))
    {
        if (EL == m_depMap.end())
        {
            return WIAnalysis::RANDOM;
        }
    }
    IGC_ASSERT(EL != m_depMap.end());
    return EL;
}

bool WIAnalysisRunner::isUniform(const llvm::Value* val)
{
    if (!hasDependency(val))
        return false;

    return whichDepend(val) == WIAnalysis::UNIFORM;
}

WIAnalysis::WIDependancy WIAnalysisRunner::getDependency(const Value* val)
{
    if (m_depMap.GetAttributeWithoutCreating(val) == m_depMap.end())
    {
        // Make sure that constants are not added in the map.
        if (!isa<Instruction>(val))
        {
            return WIAnalysis::UNIFORM;
        }
        // Don't expect this happens, let's assertion fail
        IGC_ASSERT_MESSAGE(0, "Dependence for 'val' should bave been set already!");
    }
    IGC_ASSERT(m_depMap.GetAttributeWithoutCreating(val) != m_depMap.end());
    return m_depMap.GetAttributeWithoutCreating(val);
}

bool WIAnalysisRunner::hasDependency(const Value* val)
{

    if (!isa<Instruction>(val) && !isa<Argument>(val))
    {
        return true;
    }
    return (m_depMap.GetAttributeWithoutCreating(val) != m_depMap.end());
}

static bool HasPhiUse(const llvm::Value* inst)
{
    for (auto UI = inst->user_begin(), E = inst->user_end(); UI != E; ++UI)
    {
        if (llvm::isa<llvm::PHINode>(*UI))
        {
            return true;
        }
    }
    return false;
}

void WIAnalysisRunner::calculate_dep(const Value* val)
{
    IGC_ASSERT_MESSAGE(nullptr != val, "Bad value");

    // Not an instruction, must be a constant or an argument
    // Could this vector type be of a constant which
    // is not uniform ?
    IGC_ASSERT_MESSAGE(isa<Instruction>(val), "Could we reach here with non instruction value?");

    const Instruction* const inst = dyn_cast<Instruction>(val);
    IGC_ASSERT_MESSAGE(nullptr != inst, "This Value is not an Instruction");

    bool hasOriginal = hasDependency(inst);
    WIAnalysis::WIDependancy orig;
    // We only calculate dependency on unset instructions if all their operands
    // were already given dependency. This is good for compile time since these
    // instructions will be visited again after the operands dependency is set.
    // An exception are phi nodes since they can be the ancestor of themselves in
    // the def-use chain. Note that in this case we force the phi to have the
    // pre-header value already calculated.
    if (!hasOriginal)
    {
        unsigned int unsetOpNum = 0;
        for (unsigned i = 0; i < inst->getNumOperands(); ++i)
        {
            if (!hasDependency(inst->getOperand(i))) unsetOpNum++;
        }
        if (isa<PHINode>(inst))
        {
            // We do not calculate PhiNode with all incoming values unset.
            //
            // This seems right as we don't expect a phi that only depends upon other
            // phi's (if it happens, those phis form a cycle dependency) so any phi's
            // calculation will eventually be triggered from calculating a non-phi one
            // which the phi depends upon.
            if (unsetOpNum == inst->getNumOperands()) return;
        }
        else
        {
            // We do not calculate non-PhiNode instruction that have unset operands
            if (unsetOpNum > 0) return;
        }
        orig = WIAnalysis::UNIFORM;
    }
    else
    {
        orig = m_depMap.GetAttributeWithoutCreating(inst);

        // if inst is already marked random, it cannot get better
        if (orig == WIAnalysis::RANDOM)
        {
            return;
        }
    }

    WIAnalysis::WIDependancy dep = orig;

    // LLVM does not have compile time polymorphisms
    // TODO: to make things faster we may want to sort the list below according
    // to the order of their probability of appearance.
    if (const BinaryOperator * BI = dyn_cast<BinaryOperator>(inst))         dep = calculate_dep(BI);
    else if (const CallInst * CI = dyn_cast<CallInst>(inst))                     dep = calculate_dep(CI);
    else if (isa<CmpInst>(inst))                                                dep = calculate_dep_simple(inst);
    else if (isa<ExtractElementInst>(inst))                                     dep = calculate_dep_simple(inst);
    else if (const GetElementPtrInst * GEP = dyn_cast<GetElementPtrInst>(inst))  dep = calculate_dep(GEP);
    else if (isa<InsertElementInst>(inst))                                      dep = calculate_dep_simple(inst);
    else if (isa<InsertValueInst>(inst))                                        dep = calculate_dep_simple(inst);
    else if (const PHINode * Phi = dyn_cast<PHINode>(inst))                      dep = calculate_dep(Phi);
    else if (isa<ShuffleVectorInst>(inst))                                      dep = calculate_dep_simple(inst);
    else if (isa<StoreInst>(inst))                                              dep = calculate_dep_simple(inst);
    else if (inst->isTerminator())                                              dep = calculate_dep_terminator(dyn_cast<IGCLLVM::TerminatorInst>(inst));
    else if (const SelectInst * SI = dyn_cast<SelectInst>(inst))                 dep = calculate_dep(SI);
    else if (const AllocaInst * AI = dyn_cast<AllocaInst>(inst))                 dep = calculate_dep(AI);
    else if (const CastInst * CI = dyn_cast<CastInst>(inst))                     dep = calculate_dep(CI);
    else if (isa<ExtractValueInst>(inst))                                       dep = calculate_dep_simple(inst);
    else if (const LoadInst * LI = dyn_cast<LoadInst>(inst))                     dep = calculate_dep(LI);
    else if (const VAArgInst * VAI = dyn_cast<VAArgInst>(inst))                  dep = calculate_dep(VAI);

    if (m_func->hasFnAttribute("KMPLOCK"))
    {
        dep = WIAnalysis::UNIFORM;
    }

    // If the value was changed in this calculation
    if (!hasOriginal || dep != orig)
    {
        // i1 instructions used in phi cannot be uniform as it may prevent us from removing the phi of 1
        if (inst->getType()->isIntegerTy(1) && dep == WIAnalysis::UNIFORM && HasPhiUse(inst))
        {
            dep = WIAnalysis::RANDOM;
        }
        // Save the new value of this instruction
        updateDepMap(inst, dep);
        // divergent branch, trigger updates due to control-dependence
        if (inst->isTerminator() && dep != WIAnalysis::UNIFORM)
        {
            update_cf_dep(dyn_cast<IGCLLVM::TerminatorInst>(inst));
        }
    }
}

bool WIAnalysisRunner::isRegionInvariant(const llvm::Instruction* defi, BranchInfo* brInfo, unsigned level)
{
    if (level >= 4)
    {
        return false;
    }
    if (isa<PHINode>(defi))
    {
        return false;
    }
    const unsigned nOps = defi->getNumOperands();
    for (unsigned i = 0; i < nOps; ++i)
    {
        Value* op = defi->getOperand(i);
        Instruction* srci = dyn_cast<Instruction>(op);
        if (srci)
        {
            if (!brInfo->influence_region.count(srci->getParent()))
            {
                return true;
            }
            else if (!isRegionInvariant(srci, brInfo, level + 1))
            {
                return false;
            }
        }
    }
    return true;
}

void WIAnalysisRunner::update_cf_dep(const IGCLLVM::TerminatorInst* inst)
{
    BasicBlock* blk = (BasicBlock*)(inst->getParent());
    BasicBlock* ipd = PDT->getNode(blk)->getIDom()->getBlock();
    // a branch can have NULL immediate post-dominator when a function
    // has multiple exits in llvm-ir
    // compute influence region and the partial-joins
    BranchInfo br_info(inst, ipd);
    // debug: dump influence region and partial-joins
    // br_info.print(ods());

    // check dep-type for every phi in the full join
    if (ipd)
    {
        updatePHIDepAtJoin(ipd, &br_info);
    }
    // check dep-type for every phi in the partial-joins
    for (SmallPtrSet<BasicBlock*, 4>::iterator join_it = br_info.partial_joins.begin(),
        join_e = br_info.partial_joins.end(); join_it != join_e; ++join_it)
    {
        updatePHIDepAtJoin(*join_it, &br_info);
    }

    // walk through all the instructions in the influence-region
    // update the dep-type based upon its uses
    DenseSet<BasicBlock*>::iterator blk_it = br_info.influence_region.begin();
    DenseSet<BasicBlock*>::iterator blk_e = br_info.influence_region.end();
    for (; blk_it != blk_e; ++blk_it)
    {
        BasicBlock* def_blk = *blk_it;
        // add the branch into the controlling-branch set of the block
        // if the block is in the influence-region
        IGC_ASSERT(def_blk != br_info.full_join);
        m_ctrlBranches[def_blk].insert(inst);

        for (BasicBlock::iterator I = def_blk->begin(), E = def_blk->end(); I != E; ++I)
        {
            Instruction* defi = &(*I);

            const unsigned nOps = defi->getNumOperands();
            for (unsigned i = 0; i < nOps; ++i)
            {
                Value* op = defi->getOperand(i);
                Instruction* srci = dyn_cast<Instruction>(op);
                if (srci)
                {
                    for (auto UI = srci->user_begin(), E = srci->user_end(); UI != E; ++UI)
                    {
                        PHINode* phi = dyn_cast<PHINode>(*UI);
                        if (phi)
                        {
                            BasicBlock* phi_block = phi->getParent();
                            if (phi_block == br_info.full_join ||
                                br_info.partial_joins.count(phi_block))
                            {
                                updateDepMap(phi, WIAnalysis::RANDOM);
                            }
                        }
                    }
                }
            }

            if (hasDependency(defi) && getDependency(defi) == WIAnalysis::RANDOM)
            {
                continue;
            }
            if (isRegionInvariant(defi, &br_info, 0))
            {
                continue;
            }
            // look at the uses
            Value::use_iterator use_it = defi->use_begin();
            Value::use_iterator use_e = defi->use_end();

            for (; use_it != use_e; ++use_it)
            {
                Instruction* user = dyn_cast<Instruction>((*use_it).getUser());
                IGC_ASSERT(user);
                BasicBlock* user_blk = user->getParent();
                PHINode* phi = dyn_cast<PHINode>(user);
                if (phi)
                {
                    // another place we assume all critical edges have been split and
                    // phi-move will be placed on the blocks created on those
                    user_blk = phi->getIncomingBlock(*use_it);
                }
                if (user_blk == def_blk)
                {
                    // local def-use, not related to control-dependence
                    continue; // skip
                }
                if (user_blk == br_info.full_join ||
                    br_info.partial_joins.count(user_blk) ||
                    !br_info.influence_region.count(user_blk))
                {
                    updateDepMap(defi, WIAnalysis::RANDOM);
                    // break out of the use loop
                    // since def is changed to RANDOM, all uses will be changed later
                    break;
                }
            } // end of usei loop
        } // end of defi loop within a block
    } // end of influence-region block loop
}

void WIAnalysisRunner::updatePHIDepAtJoin(BasicBlock* blk, BranchInfo* brInfo)
{
    for (BasicBlock::iterator I = blk->begin(), E = blk->end(); I != E; ++I)
    {
        Instruction* defi = &(*I);
        PHINode* phi = dyn_cast<PHINode>(defi);
        if (!phi)
        {
            break;
        }
        if (hasDependency(phi) && getDependency(phi) == WIAnalysis::RANDOM)
        {
            continue;
        }
        Value* trickySrc = nullptr;
        for (unsigned predIdx = 0; predIdx < phi->getNumOperands(); ++predIdx)
        {
            Value* srcVal = phi->getOperand(predIdx);
            Instruction* defi = dyn_cast<Instruction>(srcVal);
            if (defi && brInfo->influence_region.count(defi->getParent()))
            {
                updateDepMap(phi, WIAnalysis::RANDOM);
                break;
            }
            else
            {
                // if the src is an immed, or an argument, or defined outside,
                // think about the phi-move that can be placed in the incoming block.
                // this phi should be random if we have two different src-values like that.
                // this is one place where we assume all critical edges have been split
                BasicBlock* predBlk = phi->getIncomingBlock(predIdx);
                if (brInfo->influence_region.count(predBlk))
                {
                    if (!trickySrc)
                    {
                        trickySrc = srcVal;
                    }
                    else if (trickySrc != srcVal)
                    {
                        updateDepMap(phi, WIAnalysis::RANDOM);
                        break;
                    }
                }
            }
        }
    }
}

void WIAnalysisRunner::updateDepMap(const Instruction* inst, WIAnalysis::WIDependancy dep)
{
    // Save the new value of this instruction
    m_depMap.SetAttribute(inst, dep);
    // Register for update all of the dependent values of this updated
    // instruction.
    Value::const_user_iterator it = inst->user_begin();
    Value::const_user_iterator e = inst->user_end();
    for (; it != e; ++it)
    {
        m_pChangedNew->push_back(*it);
    }
    if (const StoreInst * st = dyn_cast<StoreInst>(inst))
    {
        auto it = m_storeDepMap.find(st);
        if (it != m_storeDepMap.end())
        {
            m_pChangedNew->push_back(it->second);
        }
    }
    // accumulate work-list for backward adjustment
    if (dep == WIAnalysis::RANDOM)
    {
        EOPCODE eopcode = GetOpCode((Instruction*)inst);
        if (eopcode == llvm_insert)
        {
            updateInsertElements((const InsertElementInst*)inst);
        }
        else if (eopcode == llvm_URBWrite || eopcode == llvm_RTWrite || eopcode == llvm_dualRTWrite)
        {
            m_backwardList.push_back(inst);
        }
    }
}

/// if one of insert-element is random, turn all the insert-elements into random
void WIAnalysisRunner::updateInsertElements(const InsertElementInst* inst)
{
    /// find the first one in the sequence
    InsertElementInst* curInst = (InsertElementInst*)inst;
    InsertElementInst* srcInst = dyn_cast<InsertElementInst>(curInst->getOperand(0));
    while (srcInst)
    {
        if (hasDependency(srcInst) && getDependency(srcInst) == WIAnalysis::RANDOM)
            return;
        curInst = srcInst;
        srcInst = dyn_cast<InsertElementInst>(curInst->getOperand(0));
    }
    if (curInst != inst)
    {
        m_depMap.SetAttribute(curInst, WIAnalysis::RANDOM);
        Value::user_iterator it = curInst->user_begin();
        Value::user_iterator e = curInst->user_end();
        for (; it != e; ++it)
        {
            m_pChangedNew->push_back(*it);
        }
    }
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep_simple(const Instruction* I)
{
    // simply check that all operands are uniform, if so return uniform, else random
    const unsigned nOps = I->getNumOperands();
    for (unsigned i = 0; i < nOps; ++i)
    {
        const Value* op = I->getOperand(i);
        WIAnalysis::WIDependancy dep = getDependency(op);
        if (dep != WIAnalysis::UNIFORM)
        {
            return WIAnalysis::RANDOM;
        }
    }
    return WIAnalysis::UNIFORM;
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const LoadInst* inst)
{
    return calculate_dep_simple(inst);
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(
    const BinaryOperator* inst)
{
    // Calculate the dependency type for each of the operands
    Value* op0 = inst->getOperand(0);
    Value* op1 = inst->getOperand(1);

    WIAnalysis::WIDependancy dep0 = getDependency(op0);
    WIAnalysis::WIDependancy dep1 = getDependency(op1);

    // For whatever binary operation,
    // uniform returns uniform
    if (WIAnalysis::UNIFORM == dep0 && WIAnalysis::UNIFORM == dep1)
    {
        return WIAnalysis::UNIFORM;
    }

    // FIXME:: assumes that the X value does not cross the +/- border - risky !!!
    // The pattern (and (X, C)), where C preserves the lower k bits of the value,
    // is often used for truncating of numbers in 64bit. We assume that the index
    // properties are not hurt by this.
    if (inst->getOpcode() == Instruction::And)
    {
        ConstantInt* C0 = dyn_cast<ConstantInt>(inst->getOperand(0));
        ConstantInt* C1 = dyn_cast<ConstantInt>(inst->getOperand(1));
        // Use any of the constants. Instcombine places constants on Op1
        // so try Op1 first.
        if (C1 || C0)
        {
            ConstantInt* C = C1 ? C1 : C0;
            WIAnalysis::WIDependancy dep = C1 ? dep0 : dep1;
            // Cannot look at bit pattern of huge integers.
            if (C->getBitWidth() < 65)
            {
                uint64_t val = C->getZExtValue();
                uint64_t ptr_mask = (1 << MinIndexBitwidthToPreserve) - 1;
                // Zero all bits above the lower k bits that we are interested in
                val &= (ptr_mask);
                // Make sure that all of the remaining bits are active
                if (val == ptr_mask)
                {
                    return dep;
                }
            }
        }
    }

    // FIXME:: assumes that the X value does not cross the +/- border - risky !!!
    // The pattern (ashr (shl X, C)C) is used for truncating of numbers in 64bit
    // The constant C must leave at least 32bits of the original number
    if (inst->getOpcode() == Instruction::AShr)
    {
        BinaryOperator* SHL = dyn_cast<BinaryOperator>(inst->getOperand(0));
        // We also allow add of uniform value between the ashr and shl instructions
        // since instcombine creates this pattern when adding a constant.
        // The shl forces all low bits to be zero, so there can be no carry to the
        // high bits due to the addition. Addition with uniform preservs WI-dep.
        if (SHL && SHL->getOpcode() == Instruction::Add)
        {
            Value* addedVal = SHL->getOperand(1);
            if (getDependency(addedVal) == WIAnalysis::UNIFORM)
            {
                SHL = dyn_cast<BinaryOperator>(SHL->getOperand(0));
            }
        }

        if (SHL && SHL->getOpcode() == Instruction::Shl)
        {
            ConstantInt* c_ashr = dyn_cast<ConstantInt>(inst->getOperand(1));
            ConstantInt* c_shl = dyn_cast<ConstantInt>(SHL->getOperand(1));
            const IntegerType* AshrTy = cast<IntegerType>(inst->getType());
            if (c_ashr && c_shl && c_ashr->getZExtValue() == c_shl->getZExtValue())
            {
                // If wordWidth - shift_width >= 32 bits
                if ((AshrTy->getBitWidth() - c_shl->getZExtValue()) >= MinIndexBitwidthToPreserve)
                {
                    // return the dep of the original X
                    return getDependency(SHL->getOperand(0));
                }
            }
        }
    }

    switch (inst->getOpcode())
    {
        // Addition simply adds the stride value, except for ptr_consecutive
        // which is promoted to strided.
        // Another exception is when we subtract the tid: 1 - X which turns the
        // tid order to random.
    case Instruction::Add:
    case Instruction::FAdd:
        return add_conversion[dep0][dep1];
    case Instruction::Sub:
    case Instruction::FSub:
        return sub_conversion[dep0][dep1];

    case Instruction::Mul:
    case Instruction::FMul:
    case Instruction::Shl:
        if (WIAnalysis::UNIFORM == dep0 || WIAnalysis::UNIFORM == dep1)
        {
            // If one of the sides is uniform, then we can adopt
            // the other side (stride*uniform is still stride).
            // stride size is K, where K is the uniform input.
            // An exception to this is ptr_consecutive, which is
            // promoted to strided.
            return mul_conversion[dep0][dep1];
        }
    default:
        //TODO: Support more arithmetic if needed
        return WIAnalysis::RANDOM;
    }
    return WIAnalysis::RANDOM;
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const CallInst* inst)
{
    // handle 3D specific intrinsics
    EOPCODE intrinsic_name = GetOpCode((Instruction*)(inst));
    GenISAIntrinsic::ID GII_id = GenISAIntrinsic::no_intrinsic;
    if (const GenIntrinsicInst * GII = dyn_cast<GenIntrinsicInst>(inst))
    {
        GII_id = GII->getIntrinsicID();
    }
    if (IsMathIntrinsic(intrinsic_name) ||
        intrinsic_name == llvm_input ||
        intrinsic_name == llvm_sgv ||
        intrinsic_name == llvm_shaderinputvec ||
        intrinsic_name == llvm_getbufferptr ||
        intrinsic_name == llvm_runtimeValue ||
        intrinsic_name == llvm_getMessagePhaseX ||
        intrinsic_name == llvm_getMessagePhaseXV ||
        intrinsic_name == llvm_surfaceinfo ||
        intrinsic_name == llvm_simdSize ||
        intrinsic_name == llvm_resinfoptr ||
        intrinsic_name == llvm_sampleinfoptr ||
        intrinsic_name == llvm_ldrawvector_indexed ||
        intrinsic_name == llvm_ldraw_indexed ||
        intrinsic_name == llvm_cycleCounter ||
        intrinsic_name == llvm_waveShuffleIndex ||
        intrinsic_name == llvm_waveBallot ||
        intrinsic_name == llvm_waveAll ||
        intrinsic_name == llvm_waveClustered ||
        intrinsic_name == llvm_ld_ptr ||
        intrinsic_name == llvm_add_pair ||
        intrinsic_name == llvm_sub_pair ||
        intrinsic_name == llvm_mul_pair ||
        intrinsic_name == llvm_ptr_to_pair ||
        intrinsic_name == llvm_pair_to_ptr ||
        intrinsic_name == llvm_fma ||
        GII_id == GenISAIntrinsic::GenISA_getSR0 ||
        GII_id == GenISAIntrinsic::GenISA_mul_rtz ||
        GII_id == GenISAIntrinsic::GenISA_fma_rtz ||
        GII_id == GenISAIntrinsic::GenISA_add_rtz ||
        GII_id == GenISAIntrinsic::GenISA_slice_id ||
        GII_id == GenISAIntrinsic::GenISA_subslice_id ||
        GII_id == GenISAIntrinsic::GenISA_eu_id ||
        GII_id == GenISAIntrinsic::GenISA_eu_thread_id)
    {
        if (intrinsic_name == llvm_input ||
            intrinsic_name == llvm_shaderinputvec)
        {
            e_interpolation mode = (e_interpolation)cast<ConstantInt>(inst->getOperand(1))->getZExtValue();
            if (mode != EINTERPOLATION_CONSTANT
                )
            {
                return WIAnalysis::RANDOM;
            }
        }


        if (intrinsic_name == llvm_sgv)
        {
            SGVUsage usage = (SGVUsage)cast<ConstantInt>(inst->getOperand(0))->getZExtValue();
            if ((usage != VFACE
                ) &&
                usage != ACTUAL_COARSE_SIZE_X &&
                usage != ACTUAL_COARSE_SIZE_Y &&
                usage != THREAD_GROUP_ID_X &&
                usage != THREAD_GROUP_ID_Y &&
                usage != THREAD_GROUP_ID_Z
                )
            {
                return WIAnalysis::RANDOM;
            }
        }
        if (intrinsic_name == llvm_getMessagePhaseX ||
            intrinsic_name == llvm_getMessagePhaseXV)
        {
            return WIAnalysis::UNIFORM;
        }

        if (intrinsic_name == llvm_waveShuffleIndex)
        {
            Value* op0 = inst->getArgOperand(0);
            Value* op1 = inst->getArgOperand(1);
            if (WIAnalysis::UNIFORM == getDependency(op0) ||
                WIAnalysis::UNIFORM == getDependency(op1))
            {
                return WIAnalysis::UNIFORM;
            }
        }

        if (intrinsic_name == llvm_waveBallot || intrinsic_name == llvm_waveAll)
        {
            return WIAnalysis::UNIFORM;
        }

        if (intrinsic_name == llvm_waveClustered)
        {
            const unsigned clusterSize = static_cast<unsigned>(
                cast<llvm::ConstantInt>(inst->getArgOperand(2))->getZExtValue());

            constexpr unsigned maxSimdSize = 32;
            if (clusterSize == maxSimdSize)
            {
                // TODO: do the same for SIMD8 and SIMD16 if possible.
                return WIAnalysis::UNIFORM;
            }
            else
            {
                return WIAnalysis::RANDOM;
            }
        }

        // Iterate over all input dependencies. If all are uniform - propagate it.
        // otherwise - return RANDOM
        unsigned numParams = inst->getNumArgOperands();

        bool isAllUniform = true;
        for (unsigned i = 0; i < numParams; ++i)
        {
            Value* op = inst->getArgOperand(i);
            WIAnalysis::WIDependancy dep = getDependency(op);
            if (WIAnalysis::UNIFORM != dep)
            {
                isAllUniform = false;
                break; // Uniformity check failed. no need to continue
            }
        }
        if (isAllUniform)
        {
            return WIAnalysis::UNIFORM;
        }
    }
    return WIAnalysis::RANDOM;
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(
    const GetElementPtrInst* inst)
{
    // running over the all indices arguments except for the last
    // here we assume the pointer is the first operand
    unsigned num = inst->getNumIndices();
    for (unsigned i = 1; i < num; ++i)
    {
        const Value* op = inst->getOperand(i);
        WIAnalysis::WIDependancy dep = getDependency(op);
        if (dep != WIAnalysis::UNIFORM)
        {
            return WIAnalysis::RANDOM;
        }
    }
    const Value* opPtr = inst->getOperand(0);
    WIAnalysis::WIDependancy depPtr = getDependency(opPtr);

    const Value* lastInd = inst->getOperand(num);
    WIAnalysis::WIDependancy lastIndDep = getDependency(lastInd);

    return gep_conversion[depPtr][lastIndDep];
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const PHINode* inst)
{
    unsigned num = inst->getNumIncomingValues();
    bool foundFirst = 0;
    WIAnalysis::WIDependancy totalDep;

    for (unsigned i = 0; i < num; ++i)
    {
        Value* op = inst->getIncomingValue(i);
        if (hasDependency(op))
        {
            if (!foundFirst)
            {
                totalDep = getDependency(op);
            }
            else
            {
                totalDep = select_conversion[totalDep][getDependency(op)];
            }
            foundFirst = 1;
        }
    }

    IGC_ASSERT_MESSAGE(foundFirst, "We should not reach here with All incoming values are unset");

    return totalDep;
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep_terminator(
    const IGCLLVM::TerminatorInst* inst)
{
    // Instruction has no return value
    // Just need to know if this inst is uniform or not
    // because we may want to avoid predication if the control flows
    // in the function are uniform...
    switch (inst->getOpcode())
    {
    case Instruction::Br:
    {
        const BranchInst* brInst = cast<BranchInst>(inst);
        if (brInst->isConditional())
        {
            // Conditional branch is uniform, if its condition is uniform
            Value* op = brInst->getCondition();
            WIAnalysis::WIDependancy dep = getDependency(op);
            if (WIAnalysis::UNIFORM == dep)
            {
                return WIAnalysis::UNIFORM;
            }
            return WIAnalysis::RANDOM;
        }
        // Unconditional branch is non TID-dependent
        return WIAnalysis::UNIFORM;
    }
    //Return instructions are unconditional
    case Instruction::Ret:
        return WIAnalysis::UNIFORM;
    case Instruction::Unreachable:
        return WIAnalysis::UNIFORM;
    case Instruction::IndirectBr:
        return WIAnalysis::RANDOM;
        // TODO: Define the dependency requirements of indirectBr
    case Instruction::Switch:
        return WIAnalysis::RANDOM;
        // TODO: Should this depend only on the condition, like branch?
    default:
        return WIAnalysis::RANDOM;
    }
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const SelectInst* inst)
{
    Value* op0 = inst->getOperand(0); // mask
    WIAnalysis::WIDependancy dep0 = getDependency(op0);
    if (WIAnalysis::UNIFORM == dep0)
    {
        Value* op1 = inst->getOperand(1);
        Value* op2 = inst->getOperand(2);
        WIAnalysis::WIDependancy dep1 = getDependency(op1);
        WIAnalysis::WIDependancy dep2 = getDependency(op2);
        // In case of constant scalar select we can choose according to the mask.
        if (ConstantInt * C = dyn_cast<ConstantInt>(op0))
        {
            uint64_t val = C->getZExtValue();
            if (val) return dep1;
            else return dep2;
        }
        // Select the "weaker" dep, but if only one dep is ptr_consecutive,
        // it must be promoted to strided ( as this data may
        // propagate to Load/Store instructions.
        return select_conversion[dep1][dep2];
    }
    // In case the mask is non-uniform the select outcome can be a combination
    // so we don't know nothing about it.
    return WIAnalysis::RANDOM;
}

bool WIAnalysisRunner::TrackAllocaDep(const Value* I, AllocaDep& dep)
{
    for (Value::const_user_iterator use_it = I->user_begin(), use_e = I->user_end(); use_it != use_e; ++use_it)
    {
        if (const GetElementPtrInst * gep = dyn_cast<GetElementPtrInst>(*use_it))
        {
            if (TrackAllocaDep(gep, dep))
                continue;
        }
        if (const llvm::LoadInst * pLoad = llvm::dyn_cast<llvm::LoadInst>(*use_it))
        {
            if (!pLoad->isSimple())
                return false;
        }
        else if (const llvm::StoreInst * pStore = llvm::dyn_cast<llvm::StoreInst>(*use_it))
        {
            if (!pStore->isSimple())
                return false;
            const llvm::Value* pValueOp = pStore->getValueOperand();
            if (pValueOp == I)
            {
                // GEP instruction is the stored value of the StoreInst (not supported case)
                return false;
            }
            dep.stores.push_back(pStore);
        }
        else if (const llvm::BitCastInst * pBitCast = llvm::dyn_cast<llvm::BitCastInst>(*use_it))
        {
            if (TrackAllocaDep(pBitCast, dep))
            {
                continue;
            }
            // Not a candidate.
            return false;
        }
        else if (const IntrinsicInst * intr = dyn_cast<IntrinsicInst>(*use_it))
        {
            llvm::Intrinsic::ID  IID = intr->getIntrinsicID();
            if (IID == llvm::Intrinsic::lifetime_start ||
                IID == llvm::Intrinsic::lifetime_end)
            {
                continue;
            }
            return false;
        }
        else
        {
            // This is some other instruction. Right now we don't want to handle these
            return false;
        }
    }
    return true;
}


WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const AllocaInst* inst)
{
    if (m_CGCtx->platform.getWATable().WaNoA32ByteScatteredStatelessMessages)
    {
        // avoid generating A32 byte scatter on platforms not supporting it
        return WIAnalysis::RANDOM;
    }
    if (!hasDependency(inst))
    {
        AllocaDep dep;
        if (TrackAllocaDep(inst, dep))
        {
            m_allocaDepMap.insert(std::make_pair(inst, dep));
            for (auto it : dep.stores)
            {
                m_storeDepMap.insert(std::make_pair(&(*it), inst));
            }
        }
    }
    auto depIt = m_allocaDepMap.find(inst);
    if (depIt == m_allocaDepMap.end())
    {
        // If we haven't been able to track the dependency of the alloca make it random
        return WIAnalysis::RANDOM;
    }
    // otherwise assume the alloca is uniform by default
    WIAnalysis::WIDependancy dep = WIAnalysis::UNIFORM;
    for (auto it : depIt->second.stores)
    {
        if (hasDependency(&(*it)))
        {
            WIAnalysis::WIDependancy dep2 = getDependency(&(*it));
            if (dep2 != WIAnalysis::UNIFORM)
            {
                return WIAnalysis::RANDOM;
            }
            if (insideDivergentCF(&(*it)))
            {
                return WIAnalysis::RANDOM;
            }
        }
    }
    return dep;
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const CastInst* inst)
{
    Value* op0 = inst->getOperand(0);
    WIAnalysis::WIDependancy dep0 = getDependency(op0);

    // independent remains independent
    if (WIAnalysis::UNIFORM == dep0) return dep0;

    switch (inst->getOpcode())
    {
    case Instruction::SExt:
    case Instruction::FPTrunc:
    case Instruction::FPExt:
    case Instruction::PtrToInt:
    case Instruction::IntToPtr:
    case Instruction::AddrSpaceCast:
    case Instruction::UIToFP:
    case Instruction::FPToUI:
    case Instruction::FPToSI:
    case Instruction::SIToFP:
        return dep0;
    case Instruction::BitCast:
    case Instruction::ZExt:
        return WIAnalysis::RANDOM;
        // FIXME:: assumes that the value does not cross the +/- border - risky !!!!
    case Instruction::Trunc: {
        const Type* destType = inst->getDestTy();
        const IntegerType* intType = dyn_cast<IntegerType>(destType);
        if (intType && (intType->getBitWidth() >= MinIndexBitwidthToPreserve))
        {
            return dep0;
        }
        return WIAnalysis::RANDOM;
    }
    default:
        IGC_ASSERT_MESSAGE(0, "no such opcode");
        // never get here
        return WIAnalysis::RANDOM;
    }
}

WIAnalysis::WIDependancy WIAnalysisRunner::calculate_dep(const VAArgInst* inst)
{
    IGC_ASSERT_MESSAGE(0, "Are we supporting this ??");
    return WIAnalysis::RANDOM;
}

// Set IsLxUniform/IsLyUniform/IsLxUniform to true if they are uniform;
// do nothing otherwise.
void WIAnalysisRunner::checkLocalIdUniform(
    Function* F,
    bool& IsLxUniform,
    bool& IsLyUniform,
    bool& IsLzUniform)
{
    if (m_CGCtx->type != ShaderType::OPENCL_SHADER)
    {
        return;
    }

    FunctionInfoMetaDataHandle funcInfoMD = m_pMdUtils->getFunctionsInfoItem(F);
    ModuleMetaData* modMD = m_CGCtx->getModuleMetaData();
    auto funcMD = modMD->FuncMD.find(F);

    int32_t WO_0 = -1, WO_1 = -1, WO_2 = -1;
    if (funcMD != modMD->FuncMD.end())
    {
        WorkGroupWalkOrderMD workGroupWalkOrder = funcMD->second.workGroupWalkOrder;
        if (workGroupWalkOrder.dim0 || workGroupWalkOrder.dim1 || workGroupWalkOrder.dim2)
        {
            WO_0 = workGroupWalkOrder.dim0;
            WO_1 = workGroupWalkOrder.dim1;
            WO_2 = workGroupWalkOrder.dim2;
        }
    }

    uint32_t simdSize = 0;
    SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
    if (subGroupSize->hasValue())
    {
        simdSize = (uint32_t)subGroupSize->getSIMD_size();
    }
    simdSize = simdSize >= 8 ? simdSize : 32;

    int32_t X = -1, Y = -1, Z = -1;
    ThreadGroupSizeMetaDataHandle threadGroupSize = funcInfoMD->getThreadGroupSize();
    if (threadGroupSize->hasValue())
    {
        X = (int32_t)threadGroupSize->getXDim();
        Y = (int32_t)threadGroupSize->getYDim();
        Z = (int32_t)threadGroupSize->getZDim();

    }

    if (WO_0 == 0 && ((X / simdSize) * simdSize) == X)
    {
        // each thread will have Y and Z unchanged.
        IsLyUniform = true;
        IsLzUniform = true;
    }
    else if (WO_0 == 1 && ((Y / simdSize) * simdSize) == Y)
    {
        // each thread will have X and Z unchanged.
        IsLxUniform = true;
        IsLzUniform = true;
    }
    else if (WO_0 == 2 && ((Z / simdSize) * simdSize) == Z)
    {
        // each thread will have X and Y unchanged.
        IsLxUniform = true;
        IsLyUniform = true;
    }

    if (X == 1)
    {
        IsLxUniform = true;
    }
    if (Y == 1)
    {
        IsLyUniform = true;
    }
    if (Z == 1)
    {
        IsLzUniform = true;
    }

    if (IGC_IS_FLAG_ENABLED(DispatchOCLWGInLinearOrder) ||
        (WO_0 == 0 && WO_1 == 1 && WO_2 == 2))
    {
        // linear order dispatch
        uint32_t XxY = X * Y;
        if (X > 0 && (X % simdSize) == 0)
        {
            // X is multiple of simdSize
            IsLyUniform = true;
            IsLzUniform = true;
        }
        else if (X > 0 && Y > 0 && (XxY % simdSize) == 0)
        {
            // X*Y is multiple of simdSize
            IsLzUniform = true;
        }
    }
}

BranchInfo::BranchInfo(const IGCLLVM::TerminatorInst* inst, const BasicBlock* ipd)
    : cbr(inst),
    full_join(ipd)
{
    const BasicBlock* fork_blk = inst->getParent();
    IGC_ASSERT_MESSAGE(cbr == fork_blk->getTerminator(), "block terminator mismatch");

    if (cbr->getNumSuccessors() != 2) {
        std::set<const BasicBlock*> Reached;
        for (auto SI = succ_begin(fork_blk),
            SE = succ_end(fork_blk); SI != SE; ++SI) {
            auto Succ = *SI;
            if (Succ == full_join)
                continue;
            std::set<const BasicBlock*> Visited;
            std::stack<const BasicBlock*> WorkSet;
            WorkSet.push(Succ);
            while (!WorkSet.empty()) {
                const BasicBlock* BB = WorkSet.top();
                WorkSet.pop();
                Visited.insert(BB);
                influence_region.insert(const_cast<BasicBlock*>(BB));
                if (Reached.count(BB))
                    partial_joins.insert(const_cast<BasicBlock*>(BB));
                for (auto I = succ_begin(BB), E = succ_end(BB); I != E; ++I) {
                    auto SBB = *I;
                    if (SBB != full_join && !Visited.count(SBB))
                        WorkSet.push(SBB);
                }
            }
            // Merge Visited into Reached.
            for (auto BB : Visited)
                Reached.insert(BB);
        }
    }
    else {
        std::set<BasicBlock*> f_set, t_set;
        std::stack<BasicBlock*> work_set;
        if (cbr->getSuccessor(0) != full_join)
        {
            work_set.push(cbr->getSuccessor(0));
            while (!work_set.empty())
            {
                BasicBlock* cur_blk = work_set.top();
                work_set.pop();
                f_set.insert(cur_blk);
                influence_region.insert(cur_blk);
                for (succ_iterator SI = succ_begin(cur_blk), E = succ_end(cur_blk); SI != E; ++SI)
                {
                    BasicBlock* succ_blk = (*SI);
                    if (succ_blk != full_join && !f_set.count(succ_blk))
                    {
                        work_set.push(succ_blk);
                    }
                }
            }
        }
        if (cbr->getSuccessor(1) != full_join)
        {
            work_set.push(cbr->getSuccessor(1));
            while (!work_set.empty())
            {
                BasicBlock* cur_blk = work_set.top();
                work_set.pop();
                t_set.insert(cur_blk);
                influence_region.insert(cur_blk);
                if (f_set.count(cur_blk))
                {
                    partial_joins.insert(cur_blk);
                }
                for (succ_iterator SI = succ_begin(cur_blk), E = succ_end(cur_blk); SI != E; ++SI)
                {
                    BasicBlock* succ_blk = (*SI);
                    if (succ_blk != full_join && !t_set.count(succ_blk))
                    {
                        work_set.push(succ_blk);
                    }
                }
            }
        }
    }
}

void BranchInfo::print(raw_ostream& OS) const
{
    OS << "\nCBR: " << *cbr;
    OS << "\nIPD: ";
    if (full_join)
    {
        full_join->print(IGC::Debug::ods());
    }
    OS << "\nPartial Joins:";
    SmallPtrSet<BasicBlock*, 4>::iterator join_it = partial_joins.begin();
    SmallPtrSet<BasicBlock*, 4>::iterator join_e = partial_joins.end();
    for (; join_it != join_e; ++join_it)
    {
        BasicBlock* cur_blk = *join_it;
        OS << "\n    ";
        cur_blk->print(IGC::Debug::ods());
    }
    OS << "\nInfluence Region:";
    DenseSet<BasicBlock*>::const_iterator blk_it = influence_region.begin();
    DenseSet<BasicBlock*>::const_iterator blk_e = influence_region.end();
    for (; blk_it != blk_e; ++blk_it)
    {
        BasicBlock* cur_blk = *blk_it;
        OS << "\n    ";
        cur_blk->print(IGC::Debug::ods());
    }
    OS << "\n";
}

extern "C"
{
    void* createWIAnalysisPass()
    {
        return new WIAnalysis();
    }
}
