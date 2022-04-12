/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/EstimateFunctionSize.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/igc_regkeys.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"
#include <deque>
#include <iostream>

using namespace llvm;
using namespace IGC;

char EstimateFunctionSize::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(EstimateFunctionSize, "EstimateFunctionSize", "EstimateFunctionSize", false, true)
IGC_INITIALIZE_PASS_END(EstimateFunctionSize, "EstimateFunctionSize", "EstimateFunctionSize", false, true)

llvm::ModulePass* IGC::createEstimateFunctionSizePass() {
    initializeEstimateFunctionSizePass(*PassRegistry::getPassRegistry());
    return new EstimateFunctionSize;
}

llvm::ModulePass*
IGC::createEstimateFunctionSizePass(EstimateFunctionSize::AnalysisLevel AL) {
    initializeEstimateFunctionSizePass(*PassRegistry::getPassRegistry());
    return new EstimateFunctionSize(AL);
}

EstimateFunctionSize::EstimateFunctionSize(AnalysisLevel AL)
    : ModulePass(ID), M(nullptr), AL(AL), tmpHasImplicitArg(false), HasRecursion(false), EnableSubroutine(false) {}

EstimateFunctionSize::~EstimateFunctionSize() { clear(); }

void EstimateFunctionSize::getAnalysisUsage(AnalysisUsage& AU) const {
    AU.setPreservesAll();
}

bool EstimateFunctionSize::runOnModule(Module& Mod) {
    clear();
    M = &Mod;
    analyze();
    checkSubroutine();
    return false;
}

// Given a module, estimate the maximal function size with complete inlining.
/*
   A ----> B ----> C ---> D ---> F
    \       \       \
     \       \       \---> E
      \       \
       \       \---> C ---> D --> F
        \             \
         \----> F      \---> E
*/
// ExpandedSize(A) = size(A) + size(B) + 2 * size(C) + 2 * size(D)
//                   + 2 * size(E) + 3 * size(F)
//
// We compute the size as follows:
//
// (1) Initialize the data structure
//
// A --> {size(A), [B, F], [] }
// B --> {size(B), [C, C], [A] }
// C --> {size(C), [D, E], [B] }
// D --> {size(D), [F],    [C] }
// E --> {size(E), [],     [C] }
// F --> {size(F), [],     [A, D] }
//
// where the first list consists of functions to be expanded and the second list
// consists of its caller functions.
//
// (2) Traverse in a reverse topological order and expand each node

namespace {

    // Function Attribute Flag type
    typedef enum
    {
        FA_BEST_EFFORT_INLINE= 0,       /// \brief A flag to indicate whether it is to be inlined but it can be trimmed or assigned stackcall
        FA_FORCE_INLINE = (0x1 << 0x0), /// \brief A flag to indicate whether it is to be inlined and it cannot be reverted
        FA_TRIMMED = (0x1 << 0x1),      /// \brief A flag to indicate whetehr it will be trimmed
        FA_STACKCALL = (0x1 << 0x2),    /// \brief A flag to indicate whether this node should be a stack call header
        FA_KERNEL_ENTRY = (0x1 << 0x3), /// \brief A flag to indicate whether this node is a kernel entry. It will be affected by any schemes.
    } FA_FLAG_t;
    /// Associate each function with a partially expanded size and remaining
    /// unexpanded function list, etc.

    struct FunctionNode {
        FunctionNode(Function* F, std::size_t Size)
            : F(F), InitialSize(Size), UnitSize(Size), ExpandedSize(Size), tmpSize(Size), CallingSubroutine(false),
            FunctionAttr(0), InMultipleUnit(false), HasImplicitArg(false) {}

        Function* F;

        /// leaf node.

        /// \brief Initial size before partition
        uint32_t InitialSize;

        //  \brief the size of a compilation unit
        uint32_t UnitSize;

        /// \brief Expanded size when all functions in a unit below the node are expanded
        uint32_t ExpandedSize;

        /// \brief used to update unit size or expanded unit size in topological sort
        uint32_t tmpSize;

        uint8_t FunctionAttr;

        /// \brief A flag to indicate whether this node has a subroutine call before
        /// expanding.
        bool CallingSubroutine;

        /// \brief A flag to indicate whether it is located in multiple kernels or units
        bool InMultipleUnit;

        bool HasImplicitArg;

        /// \brief All functions directly called in this function.
        std::unordered_map<FunctionNode*, uint16_t> CalleeList;

        /// \brief All functions that call this function F.
        std::unordered_map<FunctionNode*, uint16_t> CallerList;

        /// \brief A node becomes a leaf when all called functions are expanded.
        bool isLeaf() const { return CalleeList.empty(); }

        /// \brief Add a caller or callee.
        // A caller may call the same callee multiple times, e.g. A->{B,B,B}: A->CalleeList(B,B,B), B->CallerList(A,A,A)
        void addCallee(FunctionNode* G) {
            IGC_ASSERT(G);
            if (!CalleeList.count(G)) //First time added, Initialize it
                CalleeList[G] = 0;
            CalleeList[G] += 1;
            CallingSubroutine = true;
        }
        void addCaller(FunctionNode* G) {
            IGC_ASSERT(G);
            if (!CallerList.count(G)) //First time added, Initialize it
                CallerList[G] = 0;
            CallerList[G] += 1;
        }

        void setKernelEntry()
        {
            FunctionAttr = FA_KERNEL_ENTRY;
            return;
        }
        void setForceInline()
        {
            IGC_ASSERT(FunctionAttr != FA_KERNEL_ENTRY); //Can't force inline kernel entry
            FunctionAttr = FA_FORCE_INLINE;
            return;
        }
        void setTrimmed()
        {
            IGC_ASSERT(FunctionAttr == FA_BEST_EFFORT_INLINE); //Only best effort inline function can be trimmed
            FunctionAttr = FA_TRIMMED;
            return;
        }

        void setStackCall()
        {
            //Can't assign stack call to force inlined function, kernel entry, and functions that already assigned stack call
            IGC_ASSERT(FunctionAttr == FA_BEST_EFFORT_INLINE || FunctionAttr ==  FA_TRIMMED);
            FunctionAttr = FA_STACKCALL;
            return;
        }

        bool isTrimmed() { return FunctionAttr == FA_TRIMMED; }
        bool willBeInlined() { return FunctionAttr == FA_BEST_EFFORT_INLINE || FunctionAttr == FA_FORCE_INLINE; }
        bool isStackCallAssigned() { return FA_STACKCALL == FunctionAttr; }
        bool canAssignStackCall()
        {
            if (FA_BEST_EFFORT_INLINE == FunctionAttr ||
                FA_TRIMMED == FunctionAttr) //The best effort inline or manually trimmed functions can be assigned stack call
                return true;
            return false;
        }

        bool isGoodtoTrim()
        {
            if (FunctionAttr != FA_BEST_EFFORT_INLINE) //Only best effort inline can be trimmed
                return false;
            if (InitialSize < IGC_GET_FLAG_VALUE(ControlInlineTinySize)) //Too small to trim
                return false;
            // to allow trimming functions called from other kernels, set the regkey to false
            if (IGC_IS_FLAG_ENABLED(ForceInlineExternalFunctions) && InMultipleUnit)
                return false;
            return true;
        }

        //Top down bfs to find the size of a compilation unit
        uint32_t updateUnitSize() {
            std::unordered_set<FunctionNode*> visit;
            std::deque<FunctionNode*> TopDownQueue;
            TopDownQueue.push_back(this);
            visit.insert(this);
            uint32_t total = 0;
            while (!TopDownQueue.empty())
            {
                FunctionNode* Node = TopDownQueue.front();
                TopDownQueue.pop_front();
                total += Node->InitialSize;
                for (auto Callee : Node->CalleeList)
                {
                    FunctionNode* calleeNode = Callee.first;
                    if (visit.count(calleeNode) || calleeNode->isStackCallAssigned()) //Already processed or head of stack call
                        continue;
                    visit.insert(calleeNode);
                    TopDownQueue.push_back(calleeNode);
                }
            }
            return UnitSize = total;
        }

        /// \brief A single step to expand F
        void expand(FunctionNode* callee)
        {
            //When the collaped callee has implicit arguments
            //the node will have implicit arguments too
            //In this scenario, when ControlInlineImplicitArgs is set
            //the node should be inlined unconditioinally so exempt from a stackcall and trimming target
            if (HasImplicitArg == false && callee->HasImplicitArg == true)
            {
                HasImplicitArg = true;
                if ((IGC_GET_FLAG_VALUE(PrintControlKernelTotalSize) & 0x40) != 0)
                {
                    std::cout << "Func " << this->F->getName().str() << " expands to has implicit arg due to " << callee->F->getName().str() << std::endl;
                }

                if (FunctionAttr != FA_KERNEL_ENTRY) //Can't inline kernel entry
                {
                    if (isStackCallAssigned()) //When stackcall is assigned we need to determine based on the flag
                    {
                        if (IGC_IS_FLAG_ENABLED(ForceInlineStackCallWithImplArg))
                            setForceInline();
                    }
                    else if (IGC_IS_FLAG_ENABLED(ControlInlineImplicitArgs)) //Force inline ordinary functions with implicit arguments
                        setForceInline();
                }
            }
            uint32_t sizeIncrease = callee->ExpandedSize * CalleeList[callee];
            tmpSize += sizeIncrease;
        }
#if defined(_DEBUG)
        void print(raw_ostream& os);

        void dump() { print(llvm::errs()); }
#endif
    };

} // namespace
#if defined(_DEBUG)

void FunctionNode::print(raw_ostream& os) {
    os << "Function: " << F->getName() << ", " << InitialSize << "\n";
    for (auto G : CalleeList)
        os << "--->>>" << G.first->F->getName() << "\n";
    for (auto G : CallerList)
        os << "<<<---" << G.first->F->getName() << "\n";
}
#endif

void EstimateFunctionSize::clear() {
    M = nullptr;
    for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I) {
        auto Node = (FunctionNode*)I->second;
        delete Node;
    }
    ECG.clear();
    kernelEntries.clear();
    stackCalls.clear();
}

bool EstimateFunctionSize::matchImplicitArg( CallInst& CI )
{
    bool matched = false;
    StringRef funcName = CI.getCalledFunction()->getName();
    if( funcName.equals( GET_LOCAL_ID_X ) ||
        funcName.equals( GET_LOCAL_ID_Y ) ||
        funcName.equals( GET_LOCAL_ID_Z ) )
    {
        matched = true;
    }
    else if( funcName.equals( GET_GROUP_ID ) )
    {
        matched = true;
    }
    else if( funcName.equals( GET_LOCAL_THREAD_ID ) )
    {
        matched = true;
    }
    else if( funcName.equals( GET_GLOBAL_OFFSET ) )
    {
        matched = true;
    }
    else if( funcName.equals( GET_GLOBAL_SIZE ) )
    {
        matched = true;
    }
    else if( funcName.equals( GET_LOCAL_SIZE ) )
    {
        matched = true;
    }
    else if( funcName.equals( GET_WORK_DIM ) )
    {
        matched = true;
    }
    else if( funcName.equals( GET_NUM_GROUPS ) )
    {
        matched = true;
    }
    else if( funcName.equals( GET_ENQUEUED_LOCAL_SIZE ) )
    {
        matched = true;
    }
    else if( funcName.equals( GET_STAGE_IN_GRID_ORIGIN ) )
    {
        matched = true;
    }
    else if( funcName.equals( GET_STAGE_IN_GRID_SIZE ) )
    {
        matched = true;
    }
    else if( funcName.equals( GET_SYNC_BUFFER ) )
    {
        matched = true;
    }
    if( matched && ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x40 ) != 0 )
    {
        std::cout << "Matched implicit arg " << funcName.str() << std::endl;
    }
    return matched;
}

// visit Call inst to determine if implicit args are used by the caller
void EstimateFunctionSize::visitCallInst( CallInst& CI )
{
    if( !CI.getCalledFunction() )
    {
        return;
    }
    // Check for implicit arg function calls
    bool matched = matchImplicitArg( CI );
    tmpHasImplicitArg = matched;
}

void EstimateFunctionSize::analyze() {
    auto getSize = [](llvm::Function& F) -> std::size_t {
        std::size_t Size = 0;
        for (auto& BB : F.getBasicBlockList())
            Size += BB.size();
        return Size;
    };

    auto MdWrapper = getAnalysisIfAvailable<MetaDataUtilsWrapper>();
    auto pMdUtils = MdWrapper->getMetaDataUtils();

    // Initialize the data structure. find all noinline and stackcall properties
    for (auto& F : M->getFunctionList()) {
        if (F.empty())
            continue;
        FunctionNode* node = new FunctionNode(&F, getSize(F));
        ECG[&F] = node;
        if (isEntryFunc(pMdUtils, node->F)) ///Entry function
        {
            node->setKernelEntry();
            kernelEntries.push_back(node);
        }
        else if (F.hasFnAttribute("igc-force-stackcall"))
            node->setStackCall();
        else if (F.hasFnAttribute(llvm::Attribute::NoInline))
            node->setTrimmed();
        else if (F.hasFnAttribute(llvm::Attribute::AlwaysInline))
            node->setForceInline();
        //Otherwise, the function attribute to be assigned is best effort
    }

    // Visit all call instructions and populate CG.
    for (auto& F : M->getFunctionList()) {
        if (F.empty())
            continue;
        FunctionNode* Node = get<FunctionNode>(&F);
        for (auto U : F.users()) {
            // Other users (like bitcast/store) are ignored.
            if (auto* CI = dyn_cast<CallInst>(U)) {
                // G calls F, or G --> F
                Function* G = CI->getParent()->getParent();
                FunctionNode* GN = get<FunctionNode>(G);
                GN->addCallee(Node);
                Node->addCaller(GN);
            }
        }
    }

    bool needImplAnalysis = IGC_IS_FLAG_ENABLED(ControlInlineImplicitArgs) || IGC_IS_FLAG_ENABLED(ForceInlineStackCallWithImplArg);
    // check functions and mark those that use implicit args.
    if (needImplAnalysis)
    {
        for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I)
        {
            FunctionNode* Node = (FunctionNode*)I->second;
            IGC_ASSERT(Node);
            tmpHasImplicitArg = false;
            visit(Node->F);
            if (!tmpHasImplicitArg) //The function doesn't have an implicit argument: skip
                continue;
            Node->HasImplicitArg = true;
            if ((IGC_GET_FLAG_VALUE(PrintControlKernelTotalSize) & 0x40) != 0)
            {
                static int cnt = 0;
                const char* Name;
                if (Node->isLeaf())
                    Name = "Leaf";
                else
                    Name = "nonLeaf";
                std::cout << Name << " Func " << ++cnt << " " << Node->F->getName().str() << " calls implicit args so HasImplicitArg" << std::endl;
            }

            if (Node->FunctionAttr == FA_KERNEL_ENTRY) //Can't inline kernel entry
                continue;

            if (Node->isStackCallAssigned()) //When stackcall is assigned we need to determined based on the flag
            {
                if(IGC_IS_FLAG_ENABLED(ForceInlineStackCallWithImplArg))
                    Node->setForceInline();
                continue;
            }

            //For other cases
            if(IGC_IS_FLAG_ENABLED(ControlInlineImplicitArgs)) //Force inline ordinary functions with implicit arguments
                Node->setForceInline();
        }
    }

    // Update expanded and static unit size and propagate implicit argument information which might cancel some stackcalls
    for (void *entry : kernelEntries)
    {
        FunctionNode* kernelEntry = (FunctionNode*)entry;
        updateExpandedUnitSize(kernelEntry->F, true);
        kernelEntry->updateUnitSize();
    }

    // Find all survived stackcalls
    for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I)
    {
        FunctionNode* Node = (FunctionNode*)I->second;
        if (Node->isStackCallAssigned())
        {
            stackCalls.push_back(Node);
            Node->updateUnitSize();
        }
    }

    if ((IGC_GET_FLAG_VALUE(PrintControlUnitSize) & 0x1) != 0) {
        std::cout << "Function count= " << ECG.size() << std::endl;
    }

    return;
}

/// \brief Return the estimated maximal function size after complete inlining.
std::size_t EstimateFunctionSize::getMaxExpandedSize() const {
    uint32_t MaxSize = 0;
    for (auto I : kernelEntries) {
        FunctionNode* Node = (FunctionNode*)I;
        MaxSize = std::max(MaxSize, Node->ExpandedSize);
    }
    return MaxSize;
}

void EstimateFunctionSize::checkSubroutine() {
    auto CGW = getAnalysisIfAvailable<CodeGenContextWrapper>();
    if (!CGW) return;

    EnableSubroutine = true;
    CodeGenContext* pContext = CGW->getCodeGenContext();
    if (pContext->type != ShaderType::OPENCL_SHADER &&
        pContext->type != ShaderType::COMPUTE_SHADER)
        EnableSubroutine = false;

    if (EnableSubroutine)
    {
        uint32_t unitThreshold = IGC_GET_FLAG_VALUE(UnitSizeThreshold);
        uint32_t maxUnitSize = getMaxUnitSize();
        // If the max unit size exceeds threshold, do partitioning
        if (AL == AL_Module &&
            (IGC_GET_FLAG_VALUE(PartitionUnit) & 0x3) != 0 &&
            maxUnitSize > unitThreshold)
        {
            if ((IGC_GET_FLAG_VALUE(PrintPartitionUnit) & 0x1) != 0)
            {
                std::cout << "Max unit size " << maxUnitSize << " is larger than the threshold (to partition) " << unitThreshold << std::endl;
            }
            partitionKernel();
        }

        std::size_t subroutineThreshold = IGC_GET_FLAG_VALUE(SubroutineThreshold);
        std::size_t expandedMaxSize = getMaxExpandedSize();
        if (expandedMaxSize <= subroutineThreshold && !HasRecursion)
        {
            EnableSubroutine = false;
        }
        else if (AL == AL_Module &&
                expandedMaxSize > subroutineThreshold &&
                IGC_IS_FLAG_DISABLED(DisableAddingAlwaysAttribute))
        {
            // If max threshold is exceeded, do analysis on kernel or unit trimming
            if (IGC_IS_FLAG_ENABLED(ControlKernelTotalSize))
            {
                if ((IGC_GET_FLAG_VALUE(PrintControlKernelTotalSize) & 0x1) != 0)
                {
                    std::cout << "Max expanded unit size " << expandedMaxSize << " is larger than the threshold (to trim) " << subroutineThreshold << std::endl;
                }
                reduceKernelSize();
            }
            else if (IGC_IS_FLAG_ENABLED(ControlUnitSize))
            {
                reduceCompilationUnitSize();
            }
        }
    }
    IGC_ASSERT(!HasRecursion || EnableSubroutine);
    return;
}

std::size_t EstimateFunctionSize::getExpandedSize(const Function* F) const {
    //IGC_ASSERT(IGC_IS_FLAG_DISABLED(ControlKernelTotalSize));
    auto I = ECG.find((Function*)F);
    if (I != ECG.end()) {
        FunctionNode* Node = (FunctionNode*)I->second;
        IGC_ASSERT(F == Node->F);
        return Node->ExpandedSize;
    }
    return std::numeric_limits<std::size_t>::max();
}

bool EstimateFunctionSize::onlyCalledOnce(const Function* F) {
    //IGC_ASSERT(IGC_IS_FLAG_DISABLED(ControlKernelTotalSize));
    auto I = ECG.find((Function*)F);
    if (I != ECG.end()) {
        FunctionNode* Node = (FunctionNode*)I->second;
        IGC_ASSERT(F == Node->F);
        // one call-site and not a recursion
        if (Node->CallerList.size() == 1 &&
            Node->CallerList.begin()->second == 1 &&
            Node->CallerList.begin()->first != Node) {
            return true;
        }
        // OpenCL specific, called once by each kernel
        auto MdWrapper = getAnalysisIfAvailable<MetaDataUtilsWrapper>();
        if (MdWrapper) {
            auto pMdUtils = MdWrapper->getMetaDataUtils();
            for (auto node : Node->CallerList) {
                FunctionNode* Caller = node.first;
                uint32_t cnt = node.second;
                if (cnt > 1) {
                    return false;
                }
                if (!isEntryFunc(pMdUtils, Caller->F)) {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}


void EstimateFunctionSize::reduceKernelSize() {
    uint32_t threshold = IGC_GET_FLAG_VALUE(KernelTotalSizeThreshold);
    trimCompilationUnit(kernelEntries, threshold, true);
    return;
}


bool EstimateFunctionSize::isTrimmedFunction( llvm::Function* F) {
    return get<FunctionNode>(F)->isTrimmed();
}


//Initialize data structures for topological traversal: FunctionsInKernel and BottomUpQueue.
//FunctionsInKernel is a map data structure where the key is FunctionNode and value is the number of edges to callee nodes.
//FunctionsInKernel is primarily used for topological traversal and also used to check whether a function is in the currently processed kernel/unit.
//BottomUpQueue will contain the leaf nodes of a kernel/unit and they are starting points of topological traversal.
void EstimateFunctionSize::initializeTopologicalVisit(Function* root, std::unordered_map<void*, uint32_t>& FunctionsInKernel, std::deque<void*>& BottomUpQueue, bool ignoreStackCallBoundary)
{
    std::deque<FunctionNode*> Queue;
    FunctionNode* unitHead = get<FunctionNode>(root);
    Queue.push_back(unitHead);
    FunctionsInKernel[unitHead] = unitHead->CalleeList.size();
    // top down traversal to visit functions which will be processed reversely
    while (!Queue.empty()) {
        FunctionNode* Node = Queue.front();Queue.pop_front();
        Node->tmpSize = Node->InitialSize;
        for (auto Callee : Node->CalleeList) {
            FunctionNode* CalleeNode = Callee.first;
            if (FunctionsInKernel.count(CalleeNode))
                continue;
            if (!ignoreStackCallBoundary && CalleeNode->isStackCallAssigned()) //This callee is a compilation unit head, so not in the current compilation unit
            {
                FunctionsInKernel[Node] -= 1; //Ignore different compilation unit
                continue;
            }
            FunctionsInKernel[CalleeNode] = CalleeNode->CalleeList.size(); //Update the number of edges to callees
            Queue.push_back(CalleeNode);
        }
        if (FunctionsInKernel[Node] == 0) // This means no children or all children are compilation unit heads: leaf node
            BottomUpQueue.push_back(Node);
    }
    return;
}

//Find the total size of a unit when to-be-inlined functions are expanded
//Topologically traverse from leaf nodes and expand nodes to callers except noinline and stackcall functions
uint32_t EstimateFunctionSize::updateExpandedUnitSize(Function* F, bool ignoreStackCallBoundary)
{
    FunctionNode* root = get<FunctionNode>(F);
    std::deque<void*> BottomUpQueue;
    std::unordered_map<void*, uint32_t> FunctionsInUnit;
    initializeTopologicalVisit(root->F, FunctionsInUnit, BottomUpQueue, ignoreStackCallBoundary);
    uint32_t unitTotalSize = 0;
    while (!BottomUpQueue.empty()) //Topologically visit nodes and collape for each compilation unit
    {
        FunctionNode* node = (FunctionNode*)BottomUpQueue.front();BottomUpQueue.pop_front();
        IGC_ASSERT(FunctionsInUnit[node] == 0);
        FunctionsInUnit.erase(node);
        node->ExpandedSize = node->tmpSize; //Update the size of an expanded chunk
        if (!node->willBeInlined())
        {
            //std::cout << "Not be inlined Attr: " << (int)node->FunctionAttr << std::endl;
            unitTotalSize += node->ExpandedSize;
        }

        for (auto c : node->CallerList)
        {
            FunctionNode* caller = c.first;
            if (!FunctionsInUnit.count(caller)) //Caller is in another compilation unit
            {
                node->InMultipleUnit = true;
                continue;
            }
            FunctionsInUnit[caller] -= 1;
            if (FunctionsInUnit[caller] == 0)
                BottomUpQueue.push_back(caller);
            if (node->willBeInlined())
                caller->expand(node); //collapse and update tmpSize of the caller
        }
    }
    //Has recursion
    if (!FunctionsInUnit.empty())
        HasRecursion = true;

    return root->ExpandedSize = unitTotalSize;
}

//Partition kernels using bottom-up heristic.
uint32_t EstimateFunctionSize::bottomUpHeuristic(Function* F, uint32_t& stackCall_cnt) {
    uint32_t threshold = IGC_GET_FLAG_VALUE(UnitSizeThreshold);
    std::deque<void*> BottomUpQueue;
    std::unordered_map<void*, uint32_t> FunctionsInUnit; //Set of functions in the boundary of a kernel. Record unprocessed callee counter for topological sort.
    initializeTopologicalVisit(F, FunctionsInUnit, BottomUpQueue, false);
    FunctionNode* unitHeader = get<FunctionNode>(F);
    uint32_t max_unit_size = 0;
    while (!BottomUpQueue.empty()) {
        FunctionNode* Node = (FunctionNode*)BottomUpQueue.front();
        BottomUpQueue.pop_front();
        IGC_ASSERT(FunctionsInUnit[Node] == 0);
        FunctionsInUnit.erase(Node);
        Node->UnitSize = Node->tmpSize; //Update the size

        if (Node == unitHeader) //The last node to process is the unit header
        {
            max_unit_size = std::max(max_unit_size, Node->updateUnitSize());
            continue;
        }

        bool beStackCall = Node->canAssignStackCall() &&
                           Node->UnitSize > threshold && Node->updateUnitSize() > threshold;
        if (beStackCall)
        {
            if ((IGC_GET_FLAG_VALUE(PrintPartitionUnit) & 0x2) != 0) {
                std::cout << "Stack call marked " << Node->F->getName().str() << " Unit size: " << Node->UnitSize << " > Threshold " << threshold << std::endl;
            }
            stackCalls.push_back(Node); //We have a new unit head
            Node->setStackCall();
            max_unit_size = std::max(max_unit_size, Node->UnitSize);
            stackCall_cnt += 1;
        }

        for (auto c : Node->CallerList)
        {
            FunctionNode* caller = c.first;
            if (!FunctionsInUnit.count(caller)) //The caller is in another kernel, skip
                continue;
            FunctionsInUnit[caller] -= 1;
            if (FunctionsInUnit[caller] == 0) //All callees of the caller are processed: become leaf.
                BottomUpQueue.push_back(caller);
            if (!beStackCall)
                caller->tmpSize += Node->UnitSize;
        }
    }
    return max_unit_size;
}

//For all function F : F->Us = size(F), F->U# = 0 // unit size and unit number
//For each kernel K
//    kernelSize = K->UnitSize // O(C)
//    IF(kernelSize > T)
//        workList = ReverseTopoOrderList(K)  // Bottom up traverse
//        WHILE(worklist not empty) // O(N)
//            remove F from worklist
//            //F->Us might be overestimated due to overcounting issue -> recompute F->Us to find the actual size
//            IF(F->Us > T || recompute(F->Us) > T) {   // recompute(F->Us): O(N) only when F->Us is larger than T
//                mark F as stackcall;
//                Add F to end of headList;
//                continue;
//            }
//            Foreach F->callers P{ P->Us += F->Us; }
//        ENDWHILE
//    ENDIF
//ENDFOR
void EstimateFunctionSize::partitionKernel() {
    uint32_t threshold = IGC_GET_FLAG_VALUE(UnitSizeThreshold);
    uint32_t max_unit_size = 0;
    uint32_t stackCall_cnt = 0;

    // Iterate over kernel
    llvm::SmallVector<void*, 64> unitHeads;
    for (auto node : kernelEntries)
        unitHeads.push_back((FunctionNode*)node);
    for (auto node : stackCalls)
        unitHeads.push_back((FunctionNode*)node);
    for (auto node : unitHeads) {
        FunctionNode* UnitHead = (FunctionNode*)node;
        if (UnitHead->UnitSize <= threshold) //Unit size is within threshold, skip
        {
            max_unit_size = std::max(max_unit_size, UnitHead->UnitSize);
            continue;
        }

        if ((IGC_GET_FLAG_VALUE(PrintPartitionUnit) & 0x1) != 0) {
            std::cout << "------------------------------------------------------------------------------------------" << std::endl;
            std::cout << "Partition Kernel " << UnitHead->F->getName().str() << " Original Unit Size: " << UnitHead->UnitSize << std::endl;
        }
        max_unit_size = std::max(max_unit_size, bottomUpHeuristic(UnitHead->F, stackCall_cnt));

    }
    if ((IGC_GET_FLAG_VALUE(PrintPartitionUnit) & 0x1) != 0) {
        std::cout << "------------------------------------------------------------------------------------------" << std::endl;
        float threshold_err = (float)(max_unit_size - threshold) / threshold * 100;
        std::cout << "Max unit size: " << max_unit_size << " Threshold Error Rate: " << threshold_err << "%" << std::endl;
        std::cout << "Stack call cnt: " << stackCall_cnt << std::endl;
    }

    return;
}

//Work same as reduceKernel except for stackcall functions
void EstimateFunctionSize::reduceCompilationUnitSize() {
    uint32_t threshold = IGC_GET_FLAG_VALUE(ExpandedUnitSizeThreshold);
    llvm::SmallVector<void*, 64> unitHeads;
    for (auto node : kernelEntries)
        unitHeads.push_back((FunctionNode*)node);
    for (auto node : stackCalls)
        unitHeads.push_back((FunctionNode*)node);

    trimCompilationUnit(unitHeads, threshold,false);
    return;
}

//Top down traverse to find and retrieve functions that meet trimming criteria
void EstimateFunctionSize::getFunctionsToTrim(llvm::Function* root, llvm::SmallVector<void*, 64>& functions_to_trim, bool ignoreStackCallBoundary)
{
    FunctionNode* unitHead = get<FunctionNode>(root);
    std::unordered_set<FunctionNode*> visit;
    std::deque<FunctionNode*> TopDownQueue;
    TopDownQueue.push_back(unitHead);
    visit.insert(unitHead);
    //Find all functions that meet trimming criteria
    while (!TopDownQueue.empty())
    {
        FunctionNode* Node = TopDownQueue.front();TopDownQueue.pop_front();
        if (Node->isGoodtoTrim())
        {
            functions_to_trim.push_back(Node);
        }
        for (auto Callee : Node->CalleeList)
        {
            FunctionNode* calleeNode = Callee.first;
            if (visit.count(calleeNode) || (!ignoreStackCallBoundary && calleeNode->isStackCallAssigned()))
                continue;
            visit.insert(calleeNode);
            TopDownQueue.push_back(calleeNode);
        }
    }
    return;
}

//Trim kernel/unit by canceling out inline candidate functions one by one until the total size is within threshold
/*
For all F: F->ToBeInlined = True
For each kernel K
     kernelTotalSize = updateExpandedUnitSize(K)  // O(C) >= O(N*logN)
     IF (FullInlinedKernelSize > T)
         workList= non-tiny-functions sorted by size from large to small // O(N*logN)
         WHILE (worklist not empty) // O(N)
             remove F from worklist
             F->ToBeInlined = False
            kernelTotalSize = updateExpandedUnitSize(K)
            IF (kernelTotalSize <= T) break
         ENDWHILE
     Inline functions with ToBeInlined = True
     Inline functions with single caller // done
*/
void EstimateFunctionSize::trimCompilationUnit(llvm::SmallVector<void*, 64> &unitHeads, uint32_t threshold, bool ignoreStackCallBoundary)
{
    uint32_t PrintTrimUnit = IGC_GET_FLAG_VALUE(PrintControlKernelTotalSize) | IGC_GET_FLAG_VALUE(PrintControlUnitSize);
    llvm::SmallVector<FunctionNode*, 64> unitsToTrim;
    //Extract kernels / units that are larger than threshold
    for (auto node : unitHeads)
    {
        FunctionNode* unitEntry = (FunctionNode*)node;
        //Partitioning can add more stackcalls. So need to recompute the expanded unit size.
        updateExpandedUnitSize(unitEntry->F, ignoreStackCallBoundary);
        if (unitEntry->ExpandedSize > threshold)
        {
            if ((PrintTrimUnit & 0x1) != 0)
            {
                std::cout << "Kernel / Unit " << unitEntry->F->getName().str() << " expSize= " << unitEntry->ExpandedSize << " > " << threshold << std::endl;
            }
            unitsToTrim.push_back(unitEntry);
        }
        else
        {
            if ((PrintTrimUnit & 0x1) != 0)
            {
                std::cout << "Kernel / Unit" << unitEntry->F->getName().str() << " expSize= " << unitEntry->ExpandedSize << " <= " << threshold << std::endl;
            }
        }
    }

    if (unitsToTrim.empty())
    {
        if ((PrintTrimUnit & 0x1) != 0)
        {
            std::cout << "Kernels / Units become no longer big enough to be trimmed (affected by partitioning)" << std::endl;
        }
        return;
    }

    std::sort(unitsToTrim.begin(), unitsToTrim.end(),
        [&](const FunctionNode* LHS, const FunctionNode* RHS) { return LHS->ExpandedSize > RHS->ExpandedSize;}); //Sort by expanded size

    // Iterate over units
    for (auto unit : unitsToTrim) {
        if ((PrintTrimUnit & 0x1) != 0) {
            std::cout << "Trimming kernel / unit " << unit->F->getName().str() << " expSize= " << unit->ExpandedSize << std::endl;
        }
        size_t UnitSize = updateExpandedUnitSize(unit->F, ignoreStackCallBoundary); //A kernel size can be reduced by a function that is trimmed at previous kernels, so recompute it.
        if ((PrintTrimUnit & 0x2) != 0) {
            std::cout << "findKernelTotalSize " << UnitSize << std::endl;
        }
        if (UnitSize <= threshold) {
            if ((PrintTrimUnit & 0x2) != 0)
            {
                std::cout << "Kernel " << unit->F->getName().str() << " ok size " << UnitSize << std::endl;
            }
            continue;
        }
        if ((PrintTrimUnit & 0x2) != 0) {
            std::cout << "Kernel size is bigger than threshold " << std::endl;
            if ((IGC_GET_FLAG_VALUE(PrintControlKernelTotalSize) & 0x10) != 0)
            {
                continue; // dump collected kernels only
            }
        }

        SmallVector<void*, 64> functions_to_trim;
        getFunctionsToTrim(unit->F,functions_to_trim, ignoreStackCallBoundary);
        if (functions_to_trim.empty())
        {
            if ((PrintTrimUnit & 0x4) != 0)
            {
                std::cout << "Kernel / Unit " << unit->F->getName().str() << " size " << unit->ExpandedSize << " has no sorted list " << std::endl;
            }
            continue; // all functions are tiny.
        }

        //Sort all to-be trimmed function according to the its actual size
        std::sort(functions_to_trim.begin(), functions_to_trim.end(),
            [&](const void* LHS, const void* RHS) { return ((FunctionNode*)LHS)->InitialSize < ((FunctionNode*)RHS)->InitialSize;}); //Sort by the original function size in an ascending order;

        if ((PrintTrimUnit & 0x1) != 0)
        {
            std::cout << "Kernel / Unit " << unit->F->getName().str() << " has " << functions_to_trim.size() << " functions to consider for trimming" << std::endl;
        }

        //Repeat trimming functions until the unit size is smaller than threshold
        while (!functions_to_trim.empty() && unit->ExpandedSize > threshold)
        {
            FunctionNode* functionToTrim = (FunctionNode*)functions_to_trim.back();
            functions_to_trim.pop_back();
            if ((PrintTrimUnit & 0x2) != 0) {
                std::cout << functionToTrim->F->getName().str() << ": Now trimmed Total Kernel / Unit Size: " << unit->ExpandedSize << std::endl;
            }
            //Trim the function
            functionToTrim->setTrimmed();
            if ((PrintTrimUnit & 0x4) != 0) {
                std::cout << "FunctionToRemove " << functionToTrim->F->getName().str() << " initSize " << functionToTrim->InitialSize << " #callers " << functionToTrim->CallerList.size() << std::endl;
            }
            //Update the unit size
            updateExpandedUnitSize(unit->F,ignoreStackCallBoundary);
            if ((PrintTrimUnit & 0x4) != 0) {
                std::cout << "Kernel / Unit size is " << unit->ExpandedSize << " after trimming " << functionToTrim->F->getName().str() << std::endl;
            }
        }
        if ((PrintTrimUnit & 0x1) != 0)
        {
            std::cout << "Kernel / Unit " << unit->F->getName().str() << " final size " << unit->ExpandedSize << std::endl;
        }
    }
}

bool EstimateFunctionSize::isStackCallAssigned(llvm::Function* F) {
    FunctionNode* Node = get<FunctionNode>(F);
    return Node->isStackCallAssigned();
}

uint32_t EstimateFunctionSize::getMaxUnitSize() {
    uint32_t max_val = 0;
    for (auto kernelEntry : kernelEntries) //For all kernel, update unitsize
    {
        FunctionNode* head = (FunctionNode*)kernelEntry;
        max_val = std::max(max_val, head->UnitSize);
    }
    return max_val;
}