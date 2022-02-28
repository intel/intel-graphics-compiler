/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
#include "llvm/IR/Argument.h"
#include "llvm/IR/Attributes.h"
#include "llvmWrapper/Analysis/InlineCost.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvmWrapper/IR/CallSite.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"
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
}

Function* GenXCodeGenModule::cloneFunc(Function* F)
{
    ValueToValueMapTy VMap;

    Function* ClonedFunc = CloneFunction(F, VMap);
    //if the function is not added as part of clone, add it
    if (!F->getParent()->getFunction(ClonedFunc->getName()))
        F->getParent()->getFunctionList().push_back(ClonedFunc);
    CloneFuncMetadata(pMdUtils, ClonedFunc, F);

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

    // Get the cloning threshold. If the number of function groups a function belongs to
    // exceeds the threshold, instead of cloning the function N times, make it an indirect call
    // and use relocation instead. The function will only be compiled once and runtime must relocate
    // its address for each caller. This greatly saves on compile time when there are many function
    // groups that all call the same function.
    auto cloneTheshold = IGC_GET_FLAG_VALUE(FunctionCloningThreshold);
    if (F.hasFnAttribute("visaStackCall") && cloneTheshold > 0 && CallerFGs.size() > cloneTheshold)
    {
        auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        auto IFG = FGA->getOrCreateIndirectCallGroup(F.getParent());
        if (IFG)
        {
            F.addFnAttr("referenced-indirectly");
            pCtx->m_enableFunctionPointer = true;
            FGA->addToFunctionGroup(&F, IFG, &F);
            return;
        }
    }

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

            // Copy F's property over
            copyFuncProperties(FCloned, &F);

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

    // Use the same cloning threshold for single function SCCs, but making every function
    // in the SCC indirect calls to prevent cloning the entire SCC N times.
    auto cloneTheshold = IGC_GET_FLAG_VALUE(FunctionCloningThreshold);
    if (cloneTheshold > 0 && CallerFGs.size() > cloneTheshold)
    {
        auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        auto Mod = (*SCCNodes).front()->getFunction()->getParent();
        auto IFG = FGA->getOrCreateIndirectCallGroup(Mod);
        if (IFG)
        {
            for (CallGraphNode* Node : (*SCCNodes))
            {
                Function* F = Node->getFunction();
                F->addFnAttr("referenced-indirectly");
                pCtx->m_enableFunctionPointer = true;
                FGA->addToFunctionGroup(F, IFG, F);
            }
            return;
        }
    }

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

                // Copy properties
                copyFuncProperties(FCloned, F);

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

void GenXCodeGenModule::setFuncProperties(CallGraph& CG)
{
    for (auto I = scc_begin(&CG), IE = scc_end(&CG); I != IE; ++I)
    {
        const std::vector<CallGraphNode*>& SCCNodes = (*I);
        for (CallGraphNode* Node : SCCNodes)
        {
            Function* F = Node->getFunction();
            if (F != nullptr && !F->isDeclaration())
            {
                if (Node->empty()) {
                    FGA->setLeafFunc(F);
                }
            }
        }
    }
}

void GenXCodeGenModule::copyFuncProperties(llvm::Function* To, llvm::Function* From)
{
    FGA->copyFuncProperties(To, From);
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
                // FIXME: Below lines possibly can be refactored to be simpler.
                AttributeList attrSet = AttributeList::get(Arg.getParent()->getContext(), Arg.getArgNo() + 1, llvm::Attribute::NonNull);
                Arg.addAttr(attrSet.getAttribute(Arg.getArgNo() + 1, llvm::Attribute::NonNull));
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

    setFuncProperties(CG);

    std::vector<std::vector<CallGraphNode*>*> SCCVec;
    for (auto I = scc_begin(&CG), IE = scc_end(&CG); I != IE; ++I)
    {
        std::vector<CallGraphNode*>* SCCNodes = new std::vector<CallGraphNode*>((*I));
        SCCVec.push_back(SCCNodes);
    }

    // Add all indirect functions to the default kernel group
    FGA->addIndirectFuncsToKernelGroup(&M);

    for (auto I = SCCVec.rbegin(), IE = SCCVec.rend(); I != IE; ++I)
    {
        std::vector<CallGraphNode*>* SCCNodes = (*I);
        for (CallGraphNode* Node : (*SCCNodes))
        {
            Function* F = Node->getFunction();
            if (!F || F->isDeclaration()) continue;
            // skip functions belonging to the indirect call group
            if (FGA->isIndirectCallGroup(F)) continue;

            if (isEntryFunc(pMdUtils, F))
            {
                FGA->setSubGroupMap(F, F);
                FGA->createFunctionGroup(F);
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
        delete SCCNodes;
    }

    this->pMdUtils->save(M.getContext());

    // Check and set FG attribute flags
    FGA->setGroupAttributes();

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

    // Changing simd size from 32 to 16 for function groups with function calls due to slicing
    for (auto GI = FGA->begin(), GE = FGA->end(); GI != GE; ++GI)
    {
        FunctionGroup* FG = *GI;
        if (!FG->isSingle() || FG->hasStackCall())
        {
            Function* Kernel = FG->getHead();
            IGC::IGCMD::FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(Kernel);
            int simd_size = funcInfoMD->getSubGroupSize()->getSIMD_size();
            if (simd_size == 32)
                funcInfoMD->getSubGroupSize()->setSIMD_size(16);
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
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
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
    AU.addRequired<CodeGenContextWrapper>();
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
                if (F->hasFnAttribute("referenced-indirectly"))
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

FunctionGroup* GenXFunctionGroupAnalysis::getOrCreateIndirectCallGroup(Module* pModule)
{
    if (IndirectCallGroup) return IndirectCallGroup;

    auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    // Use the dummy kernel if it exists. Otherwise use the unique entry function.
    // OCL shaders should always use the dummy kernel.
    llvm::Function* defaultKernel = IGC::getIntelSymbolTableVoidProgram(pModule);
    if (!defaultKernel && pCtx->type != ShaderType::OPENCL_SHADER)
    {
        defaultKernel = IGC::getUniqueEntryFunc(pCtx->getMetaDataUtils(), pCtx->getModuleMetaData());
    }
    // No default kernel found
    if (!defaultKernel) return nullptr;

    IndirectCallGroup = getGroup(defaultKernel);
    if (!IndirectCallGroup)
    {
        setSubGroupMap(defaultKernel, defaultKernel);
        IndirectCallGroup = createFunctionGroup(defaultKernel);
    }
    return IndirectCallGroup;
}

bool GenXFunctionGroupAnalysis::useStackCall(llvm::Function* F)
{
    return (F->hasFnAttribute("visaStackCall"));
}

void GenXFunctionGroupAnalysis::setGroupAttributes()
{
    for (auto FG : Groups)
    {
        for (auto FI = FG->begin(), FE = FG->end(); FI != FE; ++FI)
        {
            Function* F = *FI;

            // Ignore indirect functions
            if (F->hasFnAttribute("referenced-indirectly"))
            {
                continue;
            }
            else if (F->hasFnAttribute("visaStackCall"))
            {
                FG->m_hasStackCall = true;
            }

            // check all functions in the group to see if there's an vla alloca
            // function attribute "hasVLA" should be set at ProcessFuncAttributes pass
            if (F->hasFnAttribute("hasVLA"))
            {
                FG->m_hasVaribleLengthAlloca = true;
            }

            // check if FG uses recursion. The "hasRecursion" attribute is set in
            // ProcessFuncAttributes pass by using Tarjan's algorithm to find recursion.
            if (F->hasFnAttribute("hasRecursion"))
            {
                FG->m_hasRecursion = true;
            }

            // For the remaining attributes we need to loop through all the call instructions
            for (auto ii = inst_begin(*FI), ei = inst_end(*FI); ii != ei; ii++)
            {
                if (CallInst* call = dyn_cast<CallInst>(&*ii))
                {
                    Function* calledF = call->getCalledFunction();
                    if (call->isInlineAsm())
                    {
                        // Uses inline asm call
                        FG->m_hasInlineAsm = true;
                    }
                    else if (calledF && calledF->hasFnAttribute("referenced-indirectly"))
                    {
                        // This is the case where the callee has the "referenced-indirectly" attribute, but we still
                        // see the callgraph. The callee may not belong to the same FG as the caller, but it still
                        // counts as a stackcall.
                        FG->m_hasStackCall = true;
                    }
                    else if (!calledF || (calledF->isDeclaration() && calledF->hasFnAttribute("referenced-indirectly")))
                    {
                        // This is the true indirect call case, where either the callee's address is taken, or it belongs
                        // to an external module. We do not know the callgraph in this case, so set the indirectcall flag.
                        FG->m_hasStackCall = true;
                        FG->m_hasIndirectCall = true;
                    }
                    else if (calledF && calledF->isDeclaration() && calledF->hasFnAttribute("invoke_simd_target"))
                    {
                        // Invoke_simd targets use stack call by convention.
                        FG->m_hasStackCall = true;
                    }
                }
            }
        }
    }
}

void GenXFunctionGroupAnalysis::addIndirectFuncsToKernelGroup(llvm::Module* pModule)
{
    auto pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto IFG = getOrCreateIndirectCallGroup(pModule);

    if (!IFG) return;

    // Find all indirectly called functions that require a symbol
    for (auto I = pModule->begin(), E = pModule->end(); I != E; ++I)
    {
        Function* F = &(*I);
        if (F->isDeclaration() || isEntryFunc(pMdUtils, F)) continue;

        if (F->hasFnAttribute("referenced-indirectly"))
        {
            IGC_ASSERT(getGroup(F) == nullptr);
            addToFunctionGroup(F, IFG, F);
        }
    }
}

bool GenXFunctionGroupAnalysis::rebuild(llvm::Module* Mod) {
    clear();
    auto pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    // Re-add all indirect functions to the default kernel group
    addIndirectFuncsToKernelGroup(Mod);

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
        // Skip functions belonging to the indirect call group
        if (isIndirectCallGroup(F))
            continue;

        if (isEntryFunc(pMdUtils, F))
        {
            CurFG = createFunctionGroup(F);
            CurSubGrpH = F;
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

    // Reset attribute flags
    setGroupAttributes();

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
    DenseMap<const Function*, Function*>::iterator SGII, SGIE;
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
    IndirectCallGroup = nullptr;
    M = nullptr;
}

FunctionGroup* GenXFunctionGroupAnalysis::getGroup(const Function* F)
{
    auto I = GroupMap.find(F);
    if (I == GroupMap.end())
        return nullptr;
    return I->second;
}

FunctionGroup* GenXFunctionGroupAnalysis::getGroupForHead(const Function* F)
{
    auto FG = getGroup(F);
    if (FG->getHead() == F)
        return FG;
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
    class SubroutineInliner : public LegacyInlinerBase, public llvm::InstVisitor<SubroutineInliner>
    {
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
        void verifyIfGEPIandLoadHasTheSameAS(CallGraphSCC& SCC);
        void visitGetElementPtrInst(GetElementPtrInst& I);

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


void SubroutineInliner::visitGetElementPtrInst(GetElementPtrInst& GEPI)
{
    for (auto* useOfGEPI : GEPI.users())
    {
        if (LoadInst* loadInst = dyn_cast<LoadInst>(useOfGEPI))
        {
            auto GepReturnValueAS = GEPI.getPointerAddressSpace();
            auto LoadOperandAS = loadInst->getPointerAddressSpace();
            if (GepReturnValueAS != LoadOperandAS)
            {
                auto* GEPIPointerOperand = GEPI.getPointerOperand();
                SmallVector<llvm::Value*, 8> Idx(GEPI.idx_begin(), GEPI.idx_end());
                // we need to create a new GEPI because the old one has coded old AS,
                // and we can not create new load instruction with the old GEPI with the correct AS
                // This is WA for a bug in LLVM 11.
                GetElementPtrInst* newGEPI = GetElementPtrInst::Create(GEPI.getSourceElementType(), GEPIPointerOperand, Idx, "", &GEPI);
                newGEPI->setIsInBounds(GEPI.isInBounds());
                newGEPI->setDebugLoc(GEPI.getDebugLoc());

                auto* newLoad = new LoadInst(loadInst->getType(), newGEPI, "", loadInst);
                newLoad->setAlignment(IGCLLVM::getAlign(loadInst->getAlignment()));
                loadInst->replaceAllUsesWith(newLoad);
                newLoad->setDebugLoc(loadInst->getDebugLoc());
            }
        }
    }
}

// When this pass encounters a byVal argument, it creates an alloca to then copy the data from global memory to local memory.
// When creating a new alloca, it replaces all occurrences of the argument in the function with that alloca.
// The problem arises when the pointer operant (or more precisely its address space) is replaced in GetElementPtrInst.
// Because from now on the resulting pointer of this instruction is in a different address space.
// On the other hand, a load instruction that uses the returned GetElementPtrInst pointer still operates on the old address space.
// By which we are referring to the wrong area of ​​memory. The resolution for this problem is to create new load instruction.
// This is WA for a bug in LLVM 11.
void SubroutineInliner::verifyIfGEPIandLoadHasTheSameAS(CallGraphSCC& SCC)
{
    for (CallGraphNode* Node : SCC)
    {
        Function* F = Node->getFunction();
        if (F) visit(F);
    }
}

bool SubroutineInliner::runOnSCC(CallGraphSCC& SCC)
{
    FSA = &getAnalysis<EstimateFunctionSize>();
    bool changed = LegacyInlinerBase::runOnSCC(SCC);
    if (changed) verifyIfGEPIandLoadHasTheSameAS(SCC);

    return changed;
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

        if (IGC::ForceAlwaysInline())
            return IGCLLVM::InlineCost::getAlways();

        if (pCtx->m_enableSubroutine == false)
            return IGCLLVM::InlineCost::getAlways();

        if (pCtx->type == ShaderType::OPENCL_SHADER &&
            Callee->hasFnAttribute(llvm::Attribute::NoInline))
            return IGCLLVM::InlineCost::getNever();

        if (Callee->hasFnAttribute("KMPLOCK"))
            return IGCLLVM::InlineCost::getNever();

        if (Callee->hasFnAttribute("igc-force-stackcall"))
            return IGCLLVM::InlineCost::getNever();

        if (FCtrl == FLAG_FCALL_DEFAULT)
        {
            std::size_t PerFuncThreshold = IGC_GET_FLAG_VALUE(SubroutineInlinerThreshold);

            // A single block function containing only a few instructions.
            auto isTrivialCall = [](const llvm::Function* F) {
                if (!F->empty() && F->size() == 1)
                    return F->front().size() <= 5;
                return false;
            };

            if (FSA->getExpandedSize(Caller) <= PerFuncThreshold)
            {
                return IGCLLVM::InlineCost::getAlways();
            }
            else if (isTrivialCall(Callee) || FSA->onlyCalledOnce(Callee))
            {
                return IGCLLVM::InlineCost::getAlways();
            }
            else if (!FSA->shouldEnableSubroutine())
            {
                // This function returns true if the estimated total inlining size exceeds some module threshold.
                // If we don't exceed it, and there's no preference on inline vs noinline, we just inline.
                return IGCLLVM::InlineCost::getAlways();
            }
        }
    }

    return IGCLLVM::InlineCost::getNever();
}

Pass* IGC::createSubroutineInlinerPass()
{
    initializeSubroutineInlinerPass(*PassRegistry::getPassRegistry());
    return new SubroutineInliner();
}

bool FunctionGroup::checkSimdModeValid(SIMDMode Mode) const {
    switch (Mode) {
    default:
        break;
    case SIMDMode::SIMD8: return SIMDModeValid[0];
    case SIMDMode::SIMD16: return SIMDModeValid[1];
    case SIMDMode::SIMD32: return SIMDModeValid[2];
    }
    return true;
}

void FunctionGroup::setSimdModeInvalid(SIMDMode Mode) {
    switch (Mode) {
    default:
        break;
    case SIMDMode::SIMD8: SIMDModeValid[0] = false; break;
    case SIMDMode::SIMD16: SIMDModeValid[1] = false; break;
    case SIMDMode::SIMD32: SIMDModeValid[2] = false; break;
    }
}
