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

#include "GenCodeGenModule.h"
#include "EstimateFunctionSize.h"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/DebugInfo/ScalarVISAModule.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/igc_regkeys.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvmWrapper/IR/Argument.h"
#include "llvmWrapper/IR/Attributes.h"
#include "llvmWrapper/Analysis/InlineCost.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvmWrapper/IR/CallSite.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DIBuilder.h"
#include "common/LLVMWarningsPop.hpp"
#include "DebugInfo/VISADebugEmitter.hpp"
#include <numeric>
#include <utility>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

char GenXCodeGenModule::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(GenXCodeGenModule, "GenXCodeGenModule", "GenXCodeGenModule", false, false)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(GenXFunctionGroupAnalysis)
IGC_INITIALIZE_PASS_END(GenXCodeGenModule, "GenXCodeGenModule", "GenXCodeGenModule", false, false)

llvm::ModulePass* IGC::createGenXCodeGenModulePass()
{
    initializeGenXCodeGenModulePass(*PassRegistry::getPassRegistry());
    return new GenXCodeGenModule;
}

GenXCodeGenModule::GenXCodeGenModule()
    : llvm::ModulePass(ID), FGA(nullptr), pMdUtils(nullptr), Modified(false)
{
    initializeGenXCodeGenModulePass(*PassRegistry::getPassRegistry());
}

GenXCodeGenModule::~GenXCodeGenModule() {}

void GenXCodeGenModule::getAnalysisUsage(AnalysisUsage& AU) const
{
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<GenXFunctionGroupAnalysis>();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<llvm::CallGraphWrapperPass>();
}

// Update cloned function's metadata.
static inline void CloneFuncMetadata(IGCMD::MetaDataUtils* pM,
    llvm::Function* ClonedF,
    llvm::Function* F)
{
    using namespace IGC::IGCMD;
    auto Info = pM->getFunctionsInfoItem(F);
    auto NewInfo = FunctionInfoMetaDataHandle(FunctionInfoMetaData::get());

    // Copy function type info.
    if (Info->isTypeHasValue())
    {
        NewInfo->setType(Info->getType());
    }

    // Copy explicit argument info, if any.
    unsigned i = 0;
    for (auto AI = Info->begin_ArgInfoList(), AE = Info->begin_ArgInfoList();
        AI != AE; ++AI)
    {
        NewInfo->addArgInfoListItem(Info->getArgInfoListItem(i));
        i++;
    }

    // Copy implicit argument info, if any.
    i = 0;
    for (auto AI = Info->begin_ImplicitArgInfoList(),
        AE = Info->end_ImplicitArgInfoList();
        AI != AE; ++AI)
    {
        NewInfo->addImplicitArgInfoListItem(Info->getImplicitArgInfoListItem(i));
        i++;
    }

    pM->setFunctionsInfoItem(ClonedF, Info);
    pM->save(F->getContext());
}

Function* GenXCodeGenModule::cloneFunc(Function* F)
{
    ValueToValueMapTy VMap;

    Function* ClonedFunc = CloneFunction(F, VMap);
    //if the function is not added as part of clone, add it
    if (!F->getParent()->getFunction(ClonedFunc->getName()))
        F->getParent()->getFunctionList().push_back(ClonedFunc);
    CloneFuncMetadata(pMdUtils, ClonedFunc, F);

    // record function is cloned in debug-info
    {
        IF_DEBUG_INFO(bool full;)
            IF_DEBUG_INFO(bool lineOnly;)
            IF_DEBUG_INFO(CodeGenContext * ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();)
            bool anyDebugInfo;
        anyDebugInfo = false;
        IF_DEBUG_INFO(anyDebugInfo = DebugMetadataInfo::hasAnyDebugInfo(ctx, full, lineOnly);)


            if (anyDebugInfo)
            {
                auto modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
                if (modMD)
                {
                    auto funcIt = modMD->FuncMD.find(ClonedFunc);
                    if (funcIt != modMD->FuncMD.end())
                    {
                        funcIt->second.isCloned = true;
                    }
                    else
                    {
                        // copy metadata from original function.
                        auto orgFuncMetadataIt = modMD->FuncMD.find(F);
                        if (orgFuncMetadataIt != modMD->FuncMD.end()) {
                            IGC::FunctionMetaData fMD = orgFuncMetadataIt->second;
                            fMD.isCloned = true;
                            modMD->FuncMD.insert(std::make_pair(ClonedFunc, fMD));
                        }
                        else
                        {
                            // If this assert is triggered, it probably means that ProcessBuiltinMetaData pass
                            // needs to be changed to recognize duplicate functions and run before DebugInfo pass.
                            IGC_ASSERT_MESSAGE(false, "Couldn't find metadata for cloned function!");
                        }
                    }
                    IF_DEBUG_INFO(ClonedFunc->setName(DebugMetadataInfo::getUniqueFuncName(*F));)
                }
            }
    }
    return ClonedFunc;
}

inline Function* getCallerFunc(Value* user)
{
    IGC_ASSERT(nullptr != user);
    Function* caller = nullptr;
    if (CallInst * CI = dyn_cast<CallInst>(user))
    {
        IGC_ASSERT(nullptr != CI->getParent());
        caller = CI->getParent()->getParent();
    }
    IGC_ASSERT_MESSAGE(caller, "cannot be indirect call");
    return caller;
}

void GenXCodeGenModule::processFunction(Function& F)
{
    // force stack-call for self-recursion
    for (auto U : F.users())
    {
        if (CallInst * CI = dyn_cast<CallInst>(U))
        {
            Function* Caller = CI->getParent()->getParent();
            if (Caller == &F)
            {
                F.addFnAttr("visaStackCall");
                break;
            }
        }
    }

    // See what FunctionGroups this Function is called from.
    SetVector<std::pair<FunctionGroup*, Function*>> CallerFGs;
    for (auto U : F.users())
    {
        Function* Caller = getCallerFunc(U);
        FunctionGroup* FG = FGA->getGroup(Caller);
        Function* SubGrpH = FGA->useStackCall(&F) ? (&F) : FGA->getSubGroupMap(Caller);
        if (FG == nullptr || SubGrpH == nullptr)
            continue;
        CallerFGs.insert(std::make_pair(FG, SubGrpH));
    }

    IGC_ASSERT(CallerFGs.size() >= 1);
    bool FirstPair = true;
    for (auto FGPair : CallerFGs)
    {
        if (FirstPair)
        {
            FGA->addToFunctionGroup(&F, FGPair.first, FGPair.second);
            FirstPair = false;
        }
        else
        {
            // clone the function, add to this function group
            auto FCloned = cloneFunc(&F);
            Function* SubGrpH = FGA->useStackCall(&F) ? FCloned : FGPair.second;
            FGA->addToFunctionGroup(FCloned, FGPair.first, SubGrpH);
            Modified = true;
            // update the edge after clone, it can handle self-recursion
            for (auto UI = F.use_begin(), UE = F.use_end(); UI != UE; /*empty*/)
            {
                // Increment iterator after setting U to change the use.
                Use* U = &*UI++;
                Function* Caller = cast<CallInst>(U->getUser())->getParent()->getParent();
                FunctionGroup* InFG = FGA->getGroup(Caller);
                Function* InSubGrpH = FGA->useStackCall(&F) ? FCloned : FGA->getSubGroupMap(Caller);
                if (InFG == FGPair.first && InSubGrpH == SubGrpH)
                {
                    *U = FCloned;
                }
            }
        }
    }
}

void GenXCodeGenModule::processSCC(std::vector<llvm::CallGraphNode*>* SCCNodes)
{
    // force stack-call for every function in SCC
    for (CallGraphNode* Node : (*SCCNodes))
    {
        Function* F = Node->getFunction();
        F->addFnAttr("visaStackCall");
    }

    // See what FunctionGroups this SCC is called from.
    SetVector<FunctionGroup*> CallerFGs;
    for (CallGraphNode* Node : (*SCCNodes))
    {
        Function* F = Node->getFunction();
        for (auto U : F->users())
        {
            Function* Caller = getCallerFunc(U);
            FunctionGroup* FG = FGA->getGroup(Caller);
            if (FG == nullptr)
                continue;
            CallerFGs.insert(FG);
        }
    }
    IGC_ASSERT(CallerFGs.size() >= 1);
    bool FirstPair = true;
    for (auto FG : CallerFGs)
    {
        if (FirstPair)
        {
            for (CallGraphNode* Node : (*SCCNodes))
            {
                Function* F = Node->getFunction();
                FGA->addToFunctionGroup(F, FG, F);
            }
            FirstPair = false;
        }
        else
        {
            // clone the functions in scc, add to this function group
            llvm::DenseMap<Function*, Function*> CloneMap;
            for (CallGraphNode* Node : (*SCCNodes))
            {
                Function* F = Node->getFunction();
                auto FCloned = cloneFunc(F);
                FGA->addToFunctionGroup(FCloned, FG, FCloned);
                CloneMap.insert(std::make_pair(F, FCloned));
            }
            Modified = true;
            // update the call-edges for every function in SCC,
            // move edges to the cloned SCC, including the recursion edge
            for (CallGraphNode* Node : (*SCCNodes))
            {
                Function* F = Node->getFunction();
                for (auto UI = F->use_begin(), UE = F->use_end(); UI != UE; /*empty*/)
                {
                    // Increment iterator after setting U to change the use.
                    Use* U = &*UI++;
                    Function* Caller = cast<CallInst>(U->getUser())->getParent()->getParent();
                    FunctionGroup* InFG = FGA->getGroup(Caller);
                    if (InFG == FG)
                    {
                        *U = CloneMap[F];
                    }
                }
            }
        }
    }
}

/// Deduce non-null argument attributes for subroutines.
static bool DeduceNonNullAttribute(Module& M)
{
    bool Modifided = false;
    for (auto& F : M.getFunctionList())
    {
        if (F.isDeclaration() || F.hasExternalLinkage() || F.hasAddressTaken())
            continue;

        bool Skip = false;
        for (auto U : F.users())
        {
            if (!isa<CallInst>(U))
            {
                Skip = true;
                break;
            }
        }
        if (Skip)
            continue;

        for (auto& Arg : F.args())
        {
            // Only for used pointer arguments.
            if (Arg.use_empty() || !Arg.getType()->isPointerTy())
                continue;

            // If all call sites are passing a non-null value to this argument, then
            // this argument cannot be a null ptr.
            bool NotNull = true;
            for (auto U : F.users())
            {
                auto CI = cast<CallInst>(U);
                Value* V = CI->getArgOperand(Arg.getArgNo());
                auto DL = F.getParent()->getDataLayout();
                if (!isKnownNonZero(V, DL))
                {
                    NotNull = false;
                    break;
                }
            }

            if (NotNull) {
                IGCLLVM::ArgumentAddAttr(Arg, Arg.getArgNo() + 1, llvm::Attribute::NonNull);
                Modifided = true;
            }
        }
    }
    return Modifided;
}

bool GenXCodeGenModule::runOnModule(Module& M)
{
    FGA = &getAnalysis<GenXFunctionGroupAnalysis>();

    // Already built.
    if (FGA->getModule())
        return false;

    pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    CallGraph& CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();

    std::vector<std::vector<CallGraphNode*>*> SCCVec;
    for (auto I = scc_begin(&CG), IE = scc_end(&CG); I != IE; ++I)
    {
        std::vector<CallGraphNode*>* SCCNodes = new std::vector<CallGraphNode*>((*I));
        SCCVec.push_back(SCCNodes);
    }

    for (auto I = SCCVec.rbegin(), IE = SCCVec.rend(); I != IE; ++I)
    {
        std::vector<CallGraphNode*>* SCCNodes = (*I);
        for (CallGraphNode* Node : (*SCCNodes))
        {
            Function* F = Node->getFunction();
            if (F != nullptr && !F->isDeclaration())
            {
                if (isEntryFunc(pMdUtils, F))
                {
                    FGA->setSubGroupMap(F, F);
                    FGA->createFunctionGroup(F);
                }
                else if (F->hasFnAttribute("IndirectlyCalled"))
                {
                    continue;
                }
                else if (SCCNodes->size() == 1)
                {
                    processFunction(*F);
                }
                else
                {
                    processSCC(SCCNodes);
                    break;
                }
            }
        }
        delete SCCNodes;
    }

    // Before adding indirect functions to groups, check and set stack call for each group
    FGA->setGroupStackCall();

    // Add all indirect functions to the default kernel group
    FGA->addIndirectFuncsToKernelGroup(&M);

    // By swapping, we sort the function list to ensure codegen order for
    // functions. This relies on llvm module pass manager's implementation detail.
    SmallVector<Function*, 16> OrderedList;
    for (auto GI = FGA->begin(), GE = FGA->end(); GI != GE; ++GI)
    {
        for (auto SubGI = (*GI)->Functions.begin(), SubGE = (*GI)->Functions.end();
            SubGI != SubGE; ++SubGI)
        {
            for (auto FI = (*SubGI)->begin(), FE = (*SubGI)->end(); FI != FE; ++FI)
            {
                OrderedList.push_back(*FI);
            }
        }
    }

    //  Input L1 = [A, B, C, D, E]       // Functions in groups
    //  Input L2 = [A, C, G, B, D, E, F] // Functions in the module
    // Output L2 = [A, B, C, D, E, G, F] // Ordered functions in the module
    IGC_ASSERT_MESSAGE(OrderedList.size() <= M.size(), "out of sync");
    Function* CurF = &(*M.begin());
    for (auto I = OrderedList.begin(), E = OrderedList.end(); I != E; ++I)
    {
        IGC_ASSERT(nullptr != CurF);
        Function* F = *I;
        if (CurF != F)
            // Move F before CurF. This just works See BasicBlock::moveBefore.
            // CurF remains the same for the next iteration.
            M.getFunctionList().splice(CurF->getIterator(), M.getFunctionList(), F);
        else
        {
            auto it = CurF->getIterator();
            CurF = &*(++it);
        }
    }

    IGC_ASSERT(FGA->verify());

    FGA->setModule(&M);
    Modified |= DeduceNonNullAttribute(M);

    return Modified;
}

////////////////////////////////////////////////////////////////////////////////
/// GenXFunctionGroupAnalysis implementation detail
////////////////////////////////////////////////////////////////////////////////

char GenXFunctionGroupAnalysis::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(GenXFunctionGroupAnalysis, "GenXFunctionGroupAnalysis", "GenXFunctionGroupAnalysis", false, true)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(GenXFunctionGroupAnalysis, "GenXFunctionGroupAnalysis", "GenXFunctionGroupAnalysis", false, true)

llvm::ImmutablePass* IGC::createGenXFunctionGroupAnalysisPass() {
    initializeGenXFunctionGroupAnalysisPass(*PassRegistry::getPassRegistry());
    return new GenXFunctionGroupAnalysis;
}

GenXFunctionGroupAnalysis::GenXFunctionGroupAnalysis()
    : ImmutablePass(ID), M(nullptr) {
    initializeGenXFunctionGroupAnalysisPass(*PassRegistry::getPassRegistry());
}

void GenXFunctionGroupAnalysis::getAnalysisUsage(AnalysisUsage& AU) const {
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.setPreservesAll();
}

bool GenXFunctionGroupAnalysis::verify()
{
    for (auto GI = begin(), GE = end(); GI != GE; ++GI)
    {
        for (auto SubGI = (*GI)->Functions.begin(), SubGE = (*GI)->Functions.end();
            SubGI != SubGE; ++SubGI)
        {
            for (auto FI = (*SubGI)->begin(), FE = (*SubGI)->end(); FI != FE; ++FI)
            {
                Function* F = *FI;
                if (F->hasFnAttribute("IndirectlyCalled"))
                {
                    continue;
                }
                // If F is an unused non-kernel function, although it should have been
                // deleted, that is fine.
                for (auto U : F->users())
                {
                    Function* Caller = getCallerFunc(U);
                    FunctionGroup* CallerFG = getGroup(Caller);
                    // Caller's FG should be the same as FG. Otherwise, something is wrong.
                    if (CallerFG != (*GI))
                    {
                        printf("%s\n", F->getName().data());
                        printf("%s\n", Caller->getName().data());
                        return false;
                    }
                    Function* SubGrpH = getSubGroupMap(F);
                    // Caller's sub-group header must be the first element of the sub-vector
                    if (SubGrpH != (*SubGI)->front())
                        return false;
                }
            }
        }
    }
    // Everything is OK.
    return true;
}

bool GenXFunctionGroupAnalysis::useStackCall(llvm::Function* F)
{
    return (F->hasFnAttribute("visaStackCall"));
}

void GenXFunctionGroupAnalysis::setHasVariableLengthAlloca(llvm::Module* pModule)
{
    // check all functions in the group to see if there's an vla alloca
    // function attribute "hasVLA" should be set at ProcessFuncAttributes pass
    for (auto GI = begin(), GE = end(); GI != GE; ++GI) {
        for (auto SubGI = (*GI)->Functions.begin(), SubGE = (*GI)->Functions.end();
            SubGI != SubGE; ++SubGI) {
            for (auto FI = (*SubGI)->begin(), FE = (*SubGI)->end(); FI != FE; ++FI) {
                Function* F = *FI;
                if (F->hasFnAttribute("hasVLA")) {
                    (*GI)->m_hasVaribleLengthAlloca = true;
                    break;
                }
            }
            if ((*GI)->m_hasVaribleLengthAlloca)
                break;
        }
    }
}

void GenXFunctionGroupAnalysis::addIndirectFuncsToKernelGroup(llvm::Module* pModule)
{
    auto pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    Function* defaultKernel = IGC::getIntelSymbolTableVoidProgram(pModule);
    if (!defaultKernel)
    {
        return;
    }

    FunctionGroup* defaultFG = getGroupForHead(defaultKernel);
    IGC_ASSERT_MESSAGE(nullptr != defaultFG, "default kernel group does not exist");

    // Add all externally linked functions into the default kernel group
    for (auto I = pModule->begin(), E = pModule->end(); I != E; ++I)
    {
        Function* F = &(*I);
        if (isEntryFunc(pMdUtils, F)) continue;

        if (F->hasFnAttribute("IndirectlyCalled"))
        {
            if (!F->isDeclaration())
            {
                addToFunctionGroup(F, defaultFG, F);
            }
        }
    }

    // Indirect call is treated as a stackcall
    // Even if there are no functions attached to this group, we still need to check if has an indirect call for stack alloca
    for (auto I = pModule->begin(), E = pModule->end(); I != E; ++I)
    {
        Function* F = &(*I);
        if (auto FG = getGroup(F))
        {
            // Already set for this group
            if (FG->m_hasStackCall) continue;
            // Don't set it for the dummy kernel group
            if (FG == defaultFG) continue;
            // Set hasStackCall if there are any indirect calls
            for (auto ii = inst_begin(F), ei = inst_end(F); ii != ei; ii++)
            {
                if (CallInst* call = dyn_cast<CallInst>(&*ii))
                {
                    Function* calledF = call->getCalledFunction();
                    if ((!call->isInlineAsm() && !calledF) ||
                        calledF->hasFnAttribute("IndirectlyCalled"))
                    {
                        FG->m_hasStackCall = true;
                        break;
                    }
                }
            }
        }
    }
}

bool GenXFunctionGroupAnalysis::rebuild(llvm::Module* Mod) {
    clear();
    auto pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    // Build and verify function list layout.
    // Given a list of functions, [K1, A, B, K2, C, K3, D, E, F], we build groups
    // [K1, A, B], [K2, C], [K3, D, E, F] and verify that none of CallInst escapes
    // from its group. It is rather cheap to build and verify when there is no
    // subroutine in this module.
    //
    FunctionGroup* CurFG = nullptr;
    Function* CurSubGrpH = nullptr;
    for (auto I = Mod->begin(), E = Mod->end(); I != E; ++I)
    {
        Function* F = &(*I);

        // Skip declarations.
        if (F->empty())
            continue;

        if (isEntryFunc(pMdUtils, F))
        {
            CurFG = createFunctionGroup(F);
            CurSubGrpH = F;
        }
        else if (F->hasFnAttribute("IndirectlyCalled"))
        {
            continue;
        }
        else
        {
            if (useStackCall(F))
                CurSubGrpH = F;
            if (CurFG && CurSubGrpH)
                addToFunctionGroup(F, CurFG, CurSubGrpH);
            else
            {
                clear();
                return false;
            }
        }
    }

    // Reset stack call flag
    setGroupStackCall();

    // Re-add all indirect functions to the default kernel group
    addIndirectFuncsToKernelGroup(Mod);

    // Once FGs are formed, set FG's HasVariableLengthAlloca
    setHasVariableLengthAlloca(Mod);

    // Verification.
    if (!verify())
    {
        clear();
        return false;
    }

    // Commit.
    M = Mod;
    return true;
}

void GenXFunctionGroupAnalysis::replaceEntryFunc(Function* OF, Function* NF)
{
    auto groupMapIter = GroupMap.find(OF);
    FunctionGroup* FG = groupMapIter->second;
    GroupMap.erase(groupMapIter);
    GroupMap.insert(std::make_pair(NF, FG));

    FG->replaceGroupHead(OF, NF);

    // For Entry func, SubGroupMap needs to be updated as well
    auto SGIter = SubGroupMap.find(OF);
    if (SGIter != SubGroupMap.end())
    {
        SubGroupMap.erase(SGIter);
        SubGroupMap.insert(std::make_pair(NF, NF));
    }
    DenseMap<Function*, Function*>::iterator SGII, SGIE;
    for (SGII = SubGroupMap.begin(), SGIE = SubGroupMap.end();
        SGII != SGIE; ++SGII)
    {
        Function* SGH = SGII->second;
        if (SGH == OF)
        {
            SGII->second = NF;
        }
    }
}

void GenXFunctionGroupAnalysis::clear()
{
    GroupMap.clear();
    SubGroupMap.clear();
    for (auto I = begin(), E = end(); I != E; ++I)
        delete* I;
    Groups.clear();
    M = nullptr;
}

FunctionGroup* GenXFunctionGroupAnalysis::getGroup(Function* F)
{
    auto I = GroupMap.find(F);
    if (I == GroupMap.end())
        return nullptr;
    return I->second;
}

FunctionGroup* GenXFunctionGroupAnalysis::getGroupForHead(Function* F)
{
    auto FG = getGroup(F);
    if (FG->getHead() == F)
    {
        return FG;
    }
    return nullptr;
}

void GenXFunctionGroupAnalysis::addToFunctionGroup(Function* F,
    FunctionGroup* FG,
    Function* SubGrpH)
{
    IGC_ASSERT_MESSAGE(!GroupMap[F], "Function already attached to FunctionGroup");
    GroupMap[F] = FG;
    setSubGroupMap(F, SubGrpH);
    if (F == SubGrpH)
    {
        auto* SubGrp = new llvm::SmallVector<llvm::AssertingVH<llvm::Function>, 8>();
        SubGrp->push_back(F);
        FG->Functions.push_back(SubGrp);
    }
    else
    {
        for (auto I = FG->Functions.begin(), E = FG->Functions.end(); I != E; I++)
        {
            auto* SubGrp = (*I);
            IGC_ASSERT(nullptr != SubGrp);
            if (SubGrp->front() == SubGrpH)
            {
                SubGrp->push_back(F);
                return;
            }
        }
        IGC_ASSERT(0);
    }
}

FunctionGroup* GenXFunctionGroupAnalysis::createFunctionGroup(Function* F)
{
    auto FG = new FunctionGroup;
    Groups.push_back(FG);
    addToFunctionGroup(F, FG, F);
    return FG;
}

void GenXFunctionGroupAnalysis::print(raw_ostream& os)
{
    if (!M)
    {
        os << "(nil)\n";
        return;
    }

    unsigned TotalSize = 0;
    for (auto GI = begin(), GE = end(); GI != GE; ++GI)
    {
        for (auto SubGI = (*GI)->Functions.begin(), SubGE = (*GI)->Functions.end();
            SubGI != SubGE; ++SubGI)
        {
            for (auto FI = (*SubGI)->begin(), FE = (*SubGI)->end(); FI != FE; ++FI)
            {
                Function* F = *FI;
                unsigned Size = std::accumulate(
                    F->begin(), F->end(), 0,
                    [](unsigned s, BasicBlock& BB) { return (unsigned)BB.size() + s; });
                TotalSize += Size;
                if (F == (*GI)->getHead())
                    os << "K: " << F->getName() << " [" << Size << "]\n";
                else if (F == (*SubGI)->front())
                    os << "  F: " << F->getName() << " [" << Size << "]\n";
                else
                    os << "     " << F->getName() << " [" << Size << "]\n";
            }
        }
    }
    os << "Module: " << M->getModuleIdentifier() << " [" << TotalSize << "]\n";
}

#if defined(_DEBUG)
void GenXFunctionGroupAnalysis::dump() {
    print(llvm::errs());
}
#endif

namespace {

    /// \brief Custom inliner for subroutines.
    class SubroutineInliner : public LegacyInlinerBase {
        EstimateFunctionSize* FSA;

    public:
        static char ID; // Pass identification, replacement for typeid

        // Use extremely low threshold.
        SubroutineInliner()
            : LegacyInlinerBase(ID, /*InsertLifetime*/ false),
            FSA(nullptr) {}

        InlineCost getInlineCost(IGCLLVM::CallSiteRef CS) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override;
        bool runOnSCC(CallGraphSCC& SCC) override;

        using llvm::Pass::doFinalization;
        bool doFinalization(CallGraph& CG) override {
            return removeDeadFunctions(CG);
        }
    };

} // namespace

IGC_INITIALIZE_PASS_BEGIN(SubroutineInliner, "SubroutineInliner", "SubroutineInliner", false, false)
IGC_INITIALIZE_PASS_DEPENDENCY(EstimateFunctionSize)
IGC_INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
IGC_INITIALIZE_PASS_END(SubroutineInliner, "SubroutineInliner", "SubroutineInliner", false, false)

char SubroutineInliner::ID = 0;

void SubroutineInliner::getAnalysisUsage(AnalysisUsage& AU) const
{
    AU.addRequired<EstimateFunctionSize>();
    AU.addRequired<CodeGenContextWrapper>();
    LegacyInlinerBase::getAnalysisUsage(AU);
}

bool SubroutineInliner::runOnSCC(CallGraphSCC& SCC)
{
    FSA = &getAnalysis<EstimateFunctionSize>();
    return LegacyInlinerBase::runOnSCC(SCC);
}

/// \brief Get the inline cost for the subroutine-inliner.
///
InlineCost SubroutineInliner::getInlineCost(IGCLLVM::CallSiteRef CS)
{
    Function* Callee = CS.getCalledFunction();
    Function* Caller = CS.getCaller();
    CodeGenContext* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    // Inline direct calls to functions with always inline attribute or a function
    // whose estimated size is under certain predefined limit.
    if (Callee && !Callee->isDeclaration() && isInlineViable(*Callee)
#if LLVM_VERSION_MAJOR >= 11
        .isSuccess()
#endif
        )
    {
        if (CS.hasFnAttr(llvm::Attribute::AlwaysInline))
            return IGCLLVM::InlineCost::getAlways();

        int FCtrl = IGC_GET_FLAG_VALUE(FunctionControl);

        if (FCtrl == FLAG_FCALL_FORCE_INLINE)
            return IGCLLVM::InlineCost::getAlways();

        // If m_enableSubroutine is disabled by EstimateFunctionCost pass, always inline
        if (pCtx->m_enableSubroutine == false)
            return IGCLLVM::InlineCost::getAlways();

        if (pCtx->type == ShaderType::OPENCL_SHADER &&
            Callee->hasFnAttribute(llvm::Attribute::NoInline))
            return IGCLLVM::InlineCost::getNever();

        if (Callee->hasFnAttribute("KMPLOCK"))
            return IGCLLVM::InlineCost::getNever();

        if (Callee->hasFnAttribute("UserSubroutine") &&
            Callee->hasFnAttribute(llvm::Attribute::NoInline))
            return IGCLLVM::InlineCost::getNever();

        if (Callee->hasFnAttribute("igc-force-stackcall"))
            return IGCLLVM::InlineCost::getNever();

        if (FCtrl != FLAG_FCALL_FORCE_SUBROUTINE &&
            FCtrl != FLAG_FCALL_FORCE_STACKCALL)
        {
            std::size_t Threshold = IGC_GET_FLAG_VALUE(SubroutineInlinerThreshold);

            // A single block function containing only a few instructions.
            auto isTrivialCall = [](const llvm::Function* F) {
                if (!F->empty() && F->size() == 1)
                    return F->front().size() <= 5;
                return false;
            };

            if (FSA->getExpandedSize(Caller) <= Threshold ||
                FSA->onlyCalledOnce(Callee) || isTrivialCall(Callee))
                return IGCLLVM::InlineCost::getAlways();
        }
    }

    return IGCLLVM::InlineCost::getNever();
}

Pass* IGC::createSubroutineInlinerPass()
{
    initializeSubroutineInlinerPass(*PassRegistry::getPassRegistry());
    return new SubroutineInliner();
}
