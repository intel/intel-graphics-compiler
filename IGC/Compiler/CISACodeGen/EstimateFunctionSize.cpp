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
// (2) Expand all leaf nodes until all become leaf nodes. We update the data
// structure to the following after expanding functions E and F:
//
// A --> {size(A) + size(F), [B],    [] }
// B --> {size(B),           [C, C], [A] }
// C --> {size(C) + size(E), [D],    [B] }
// D --> {size(D) + size(F), [],     [C] }
// E --> {size(E),           [],     [C] }
// F --> {size(F),           [],     [A, D] }
//
namespace {

    /// Associate each function with a partially expanded size and remaining
    /// unexpanded function list, etc.
    struct FunctionNode {
        FunctionNode(Function* F, std::size_t Size)
            : F(F), Size(Size), InitialSize(Size), Processed(0), KernelNum(0),
              CallingSubroutine(false), HasImplicitArg(false) {}

        Function* F;

        /// \brief Partially expanded size, or completely expanded size for a
        /// leaf node.
        std::size_t Size;

        /// \brief Initial size before expansion.
        std::size_t InitialSize;

        /// \brief A number to indicate whether this node has been processed in current traversal.
        uint32_t Processed;

        /// \brief A number to indicate whether this node belongs to the current kernel.
        uint32_t KernelNum;

        /// \brief A flag to indicate whether this node has a subroutine call before
        /// expanding.
        bool CallingSubroutine;

        /// \brief A flag to indicate whether this node should be always inlined.
        bool ToBeInlined;

        bool HasImplicitArg;

        /// \brief All functions directly called in this function.
        std::vector<Function*> CalleeList;

        /// \brief All functions that call this function F.
        std::vector<Function*> CallerList;

        /// \brief A node becomes a leaf when all called functions are expanded.
        bool isLeaf() const { return CalleeList.empty(); }

        /// \brief Add a caller or callee.
        // A caller may call the same callee multiple times, e.g. A->{B,B,B}: A->CalleeList(B,B,B), B->CallerList(A,A,A)
        void addCallee(Function* G) {
            IGC_ASSERT(!G->empty());
            CalleeList.push_back(G);
            CallingSubroutine = true;
        }
        void addCaller(Function* G) {
            IGC_ASSERT(!G->empty());
            CallerList.push_back(G);
        }

        /// \brief A single step to expand F: accumulate the size and remove it
        /// from the callee list.
        void expand(FunctionNode* Node) {
            if( IGC_IS_FLAG_ENABLED(ControlInlineImplicitArgs) && HasImplicitArg == false && Node->HasImplicitArg == true )
            {
                HasImplicitArg = true;
                if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x40 ) != 0 )
                {
                    std::cout << "Func " << this->F->getName().str() << " expands to hasimplicitarg due to " << Node->F->getName().str() << std::endl;
                }
            }
            // Multiple calls to a function is allowed as in the example above.
            for (auto I = CalleeList.begin(); I != CalleeList.end(); /* empty */) {
                if (*I == Node->F) {
                    Size += Node->Size;
                    I = CalleeList.erase(I);
                }
                else
                    ++I;
            }
        }

        /// \brief A single step to expand F: accumulate the size and DON't remove it
        /// from the callee list.
        void expandSpecial( FunctionNode* Node ) {
            Size += Node->Size;
        }

#if defined(_DEBUG)
        void print(raw_ostream& os);

        void dump() { print(llvm::errs()); }
#endif
    };

} // namespace
#if defined(_DEBUG)

void FunctionNode::print(raw_ostream& os) {
    os << "Function: " << F->getName() << ", " << Size << "\n";
    for (auto G : CalleeList)
        os << "--->>>" << G->getName() << "\n";
    for (auto G : CallerList)
        os << "<<<---" << G->getName() << "\n";
}
#endif

void EstimateFunctionSize::clear() {
    M = nullptr;
    for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I) {
        auto Node = (FunctionNode*)I->second;
        delete Node;
    }
    ECG.clear();
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

    // Initial the data structure.
    for (auto& F : M->getFunctionList()) {
        if (F.empty())
            continue;
        ECG[&F] = new FunctionNode(&F, getSize(F));
    }

    // Visit all call instructions and populate CG.
    for (auto& F : M->getFunctionList()) {
        if (F.empty())
            continue;

        FunctionNode* Node = get<FunctionNode>(&F);
        for (auto U : F.users()) {
            // Other users (like bitcast/store) are ignored.
            if (auto * CI = dyn_cast<CallInst>(U)) {
                // G calls F, or G --> F
                Function* G = CI->getParent()->getParent();
                FunctionNode* GN = get<FunctionNode>( G );
                GN->addCallee(&F);
                Node->addCaller(G);
            }
        }
    }
    // check functions and mark those that use implicit args.
    if( IGC_IS_FLAG_ENABLED( ControlInlineImplicitArgs ) )
    {
        for( auto I = ECG.begin(), E = ECG.end(); I != E; ++I )
        {
            auto Node = (FunctionNode*)I->second;
            if( Node /* && Node->isLeaf() a non-leaf function may also call implicit args */ )
            {
                tmpHasImplicitArg = false;
                visit( Node->F );
                if( tmpHasImplicitArg )
                {
                    Node->HasImplicitArg = true;
                    if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x40 ) != 0 )
                    {
                        static int cnt = 0;
                        const char* Name;
                        if( Node->isLeaf() )
                            Name = "Leaf";
                        else
                            Name = "nonLeaf";
                        std::cout << Name << " Func " << ++cnt << " " << Node->F->getName().str() << " calls implicit args so HasImplicitArg" << std::endl;
                    }
                }
            }
        }
    }

    // Expand leaf nodes until all are expanded (the second list is empty).
    while (true) {
        // Find unexpanded leaf nodes.
        SmallVector<FunctionNode*, 8> LeafNodes;
        for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I) {
            auto Node = (FunctionNode*)I->second;
            if (!Node->Processed && Node->isLeaf())
                LeafNodes.push_back(Node);
        }

        // Done.
        if (LeafNodes.empty())
            break;

        // Expand leaf nodes one by one.
        for (auto Node : LeafNodes) {
            IGC_ASSERT(Node->CalleeList.empty());
            // Populate to its Callers.
            for (auto Caller : Node->CallerList)
                get<FunctionNode>(Caller)->expand(Node);
            Node->Processed = true;
        }
    }

    HasRecursion = false;
    for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I) {
        FunctionNode* Node = (FunctionNode*)I->second;
        if (!Node->isLeaf()) {
            HasRecursion = true;
        }
    }
}

/// \brief Return the estimated maximal function size after complete inlining.
std::size_t EstimateFunctionSize::getMaxExpandedSize() const {
    std::size_t MaxSize = 0;
    for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I) {
        FunctionNode* Node = (FunctionNode*)I->second;
        // Only functions with subroutine calls count.
        if (Node->CallingSubroutine)
            MaxSize = std::max(MaxSize, Node->Size);
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
        std::size_t Threshold = IGC_GET_FLAG_VALUE(SubroutineThreshold);
        std::size_t MaxSize = getMaxExpandedSize();
        if (MaxSize <= Threshold && !HasRecursion)
        {
            EnableSubroutine = false;
        }
        else if (MaxSize > Threshold && AL == AL_Module)
        {
            // If max threshold is exceeded, do analysis on kernel trimming
            if (IGC_IS_FLAG_ENABLED(ControlKernelTotalSize) &&
                IGC_IS_FLAG_DISABLED(DisableAddingAlwaysAttribute))
            {
                if ((IGC_GET_FLAG_VALUE(PrintControlKernelTotalSize) & 0x1) != 0)
                {
                    std::cout << "Max size " << MaxSize << " is larger than the threshold (to trim) " << Threshold << std::endl;
                }
                reduceKernelSize();
            }
        }
    }
    IGC_ASSERT(!HasRecursion || EnableSubroutine);
}

std::size_t EstimateFunctionSize::getExpandedSize(const Function* F) const {
    auto I = ECG.find((Function*)F);
    if (I != ECG.end()) {
        FunctionNode* Node = (FunctionNode*)I->second;
        IGC_ASSERT(F == Node->F);
        return Node->Size;
    }

    // Unknown.
    return std::numeric_limits<std::size_t>::max();
}

bool EstimateFunctionSize::onlyCalledOnce(const Function* F) {
    auto I = ECG.find((Function*)F);
    if (I != ECG.end()) {
        FunctionNode* Node = (FunctionNode*)I->second;
        IGC_ASSERT(F == Node->F);
        // one call-site and not a recursion
        if (Node->CallerList.size() == 1 &&
            Node->CallerList.front() != F) {
            return true;
        }
        // OpenCL specific, called once by each kernel
        SmallPtrSet<Function*, 8> CallerSet;
        auto MdWrapper = getAnalysisIfAvailable<MetaDataUtilsWrapper>();
        if (MdWrapper) {
            auto pMdUtils = MdWrapper->getMetaDataUtils();
            for (auto Caller : Node->CallerList) {
                if (!isEntryFunc(pMdUtils, Caller)) {
                    return false;
                }
                if (CallerSet.count(Caller)) {
                    return false;
                }
                CallerSet.insert(Caller);
            }
            return true;
        }
    }
    return false;
}

bool EstimateFunctionSize::funcIsGoodtoTrim( llvm::Function* F)
{
    FunctionNode* func = get<FunctionNode>( F );
    if ( func->InitialSize < IGC_GET_FLAG_VALUE( ControlInlineTinySize ) )
        return false; /* tiny function */
    if ( func->F->hasFnAttribute( llvm::Attribute::AlwaysInline ) )
        return false; /* user specified alwaysInline */
    if ( isTrimmedFunction( F ) ) /* already trimmed by other kernels */
        return false;
    if( IGC_IS_FLAG_ENABLED( ControlInlineImplicitArgs ) && func->HasImplicitArg )
    {
        if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x20 ) != 0 )
        {
            std::cout << "Func not inlined: " << func->F->getName().str() << " has implicit args " << std::endl;
        }
        return false;
    }
    // to allow trimming functions called from other kernels, set the regkey to false
    if( IGC_IS_FLAG_ENABLED( ForceInlineExternalFunctions ) )
    {
        for( auto C : func->CallerList )
        {
            FunctionNode* caller = get<FunctionNode>( C );
            if( caller->KernelNum != func->KernelNum )
                return false; // always inline if there is a caller outside this kernel
        }
    }
    return true;
}

/*
For all F: F->ToBeInlined = True
For each kernel K
     kernelTotalSize = findKernelTotalSize(K)  // O(C) >= O(N*logN)
     IF (FullInlinedKernelSize > T)
    workList= non-tiny-functions sorted by size from large to small // O(N*logN)
    AdjustInterval = sizeof(worklist) / 20; cnt = 0;
    WHILE (worklist not empty) // O(N)
        remove F from worklist
        F->ToBeInlined = False
        if (++cnt == AdjustInterval) {  kernelTotalSize = findKernelTotalSize(K); cnt = 0; }  // exact kernelTotalSize
        else { kernelTotalSize -= (F->#callers ?1) * F->expdSize ; }
        IF (kernelTotalSize <= T) break
    ENDWHILE
     Inline functions with ToBeInlined = True
     Inline functions with single caller // done
*/
void EstimateFunctionSize::reduceKernelSize() {
    auto MdWrapper = getAnalysisIfAvailable<MetaDataUtilsWrapper>();
    auto pMdUtils = MdWrapper->getMetaDataUtils();
    std::vector<FunctionNode*> kernels;

    uint32_t uniqKn = 0; // unique kernel number
    uint32_t uniqProc = 0; // unique processed number (Processed already updated in EstimateFunctionSize::analyze()
    // init unique kernel/processed numbers (UK/UP) and identify kernel functions
    for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I) {
        auto Node = (FunctionNode*)I->second;
        Node->KernelNum = uniqKn;
        Node->Processed = uniqProc;
        if (Node->F->hasFnAttribute(llvm::Attribute::NoInline)) { /* user specified noinline */
            Node->ToBeInlined = false;
        } else {
            Node->ToBeInlined = true;
        }
        if (isEntryFunc(pMdUtils, Node->F)) {
            if( Node->Size > IGC_GET_FLAG_VALUE( KernelTotalSizeThreshold ) )
            {
                kernels.push_back( Node);
                /* if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x1 ) != 0 )
                {
                    std::cout << "Enqueue kernel " << Node->F->getName().str() << " expanded size=" << Node->Size << std::endl;
                } */
            }
            else
            {
                /* if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x1 ) != 0 )
                {
                    std::cout << "Notenqueue kernel " << Node->F->getName().str() << " expanded size=" << Node->Size << std::endl;
                } */
            }
        }
    }

    // Visit all call instructions and populate call graph (callers and calees in function nodes).
    // The following are already done in EstimateFunctionSize::analyze(), before calling this function
    // But Callee was destroyed in expand.  Redo here, but don't call addCaller() which is still available.
    // Should only build CG once -- to be improved late.
    for (auto& F : M->getFunctionList()) {
        if (F.empty())
            continue;

        // FunctionNode* Node = get<FunctionNode>(&F);
        // need to verify: F.users(), getParent()->getParent(), etc
        for (auto U : F.users()) {
            // Other users (like bitcast/store) are ignored.
            if (auto* CI = dyn_cast<CallInst>(U)) {
                // G calls F, or G --> F
                Function* G = CI->getParent()->getParent();
                get<FunctionNode>(G)->addCallee(&F);
                // Node->addCaller(G);
            }
        }
    }

    std::sort( kernels.begin(), kernels.end(),
        [&]( const FunctionNode* LHS, const FunctionNode* RHS ) { return LHS->Size > RHS->Size; });

    // Iterate over kernels
    for (auto Kernel : kernels) {

        uniqKn++; // get a unique number for this kernel

        if ( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x1 ) != 0 ) {
            std::cout << "Trimming Kernel " << Kernel->F->getName().str() << " expSize= " << Kernel->Size << std::endl;
        }

        size_t KernelSize = findKernelTotalSize(Kernel->F, uniqKn, uniqProc );
        if ( (IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize) & 0x2) != 0) {
            std::cout << "findKernelTotalSize  " << KernelSize << std::endl;
        }

        if (KernelSize <= IGC_GET_FLAG_VALUE(KernelTotalSizeThreshold)) {
            if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x2 ) != 0 )
            {
                std::cout << "Kernel " << Kernel->F->getName().str() << " ok size " << KernelSize << std::endl;
            }
            continue;
        }

        if ( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x2 ) != 0 ) {
            std::cout << "Kernel size is bigger than threshold " << std::endl;
            if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x10 ) != 0 )
            {
                continue; // dump collected kernels only
            }
        }

        std::vector<FunctionNode*> SortedKernelFunctions;

        std::deque<FunctionNode*> Queue;
        Queue.push_back(Kernel);

        uniqProc++;
        // top down traversal to find non-tiny-functions and sort them from large to small
        while (!Queue.empty()) {
            FunctionNode* Node = Queue.front();
            Node->Size = Node->InitialSize;
            Node->Processed = uniqProc;
            Node->KernelNum = uniqKn;
            if ( Node != Kernel && /* not kernel itself */
                 funcIsGoodtoTrim(Node->F)) /* and other criterias */
            {
               SortedKernelFunctions.push_back(Node);
            }
            //std::cout << "Node       " << Node->F->getName().str() << std::endl;
            Queue.pop_front();

            for (auto Callee : Node->CalleeList) {
                FunctionNode* CalleeNode = get<FunctionNode>(Callee);
                if( CalleeNode->Processed != uniqProc )  // Not processed yet
                {
                    Queue.push_back(CalleeNode);
                    CalleeNode->Processed = uniqProc; // prevent this callee from entering queue again
                }
            }
        }
        if( SortedKernelFunctions.empty() )
        {
            if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x4 ) != 0 )
            {
                std::cout << "Kernel " << Kernel->F->getName().str() << " size " << KernelSize << " has no sorted list " << std::endl;
            }
            continue; // all functions are tiny.
        }


        auto Cmp = [](const FunctionNode* LHS, const FunctionNode* RHS) {
            return LHS->Size > RHS->Size;
        };
        std::sort(SortedKernelFunctions.begin(), SortedKernelFunctions.end(), Cmp);

        if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x4 ) != 0 )
        {
            std::cout << "Kernel " << Kernel->F->getName().str() << " has " << SortedKernelFunctions.size() << " funcs to consider for trimming" << std::endl;
        }
        uint32_t AdjustInterval = 1; //  SortedKernelFunctions.size() / 20;
        uint32_t cnt = 0;

        while (KernelSize > IGC_GET_FLAG_VALUE(KernelTotalSizeThreshold) && !SortedKernelFunctions.empty() ) {
            FunctionNode* FunctionToRemove = SortedKernelFunctions.front();
            SortedKernelFunctions.erase(SortedKernelFunctions.begin());

            FunctionToRemove->ToBeInlined = false;
            // TrimmingCandidates[FunctionToRemove->F] = true;
            if ( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x4 ) != 0 ) {
                std::cout << "FunctionToRemove " << FunctionToRemove->F->getName().str() <<  " initSize "<< FunctionToRemove->InitialSize << " #callers " << FunctionToRemove->CallerList.size() << std::endl;
            }

            if( ++cnt == AdjustInterval )
            {
                cnt = 0;
                KernelSize = findKernelTotalSize(Kernel->F, uniqKn, uniqProc);
                if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x4 ) != 0 )
                {
                    std::cout << "Precise size after trimming " << FunctionToRemove->F->getName().str() << " is " << KernelSize << std::endl;
                }
            } else {
                KernelSize -= (FunctionToRemove->CallerList.size()- 1) * FunctionToRemove->Size; // #caller is underestimated
                if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x4 ) != 0 )
                {
                    std::cout << "Estimated trimming -=" << ( FunctionToRemove->CallerList.size() - 1 ) << " * " << FunctionToRemove->Size << std::endl;
                }
            }
            if ( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x4 ) != 0 ) {
                std::cout << "Kernel size is " << KernelSize << " after trimming " << FunctionToRemove->F->getName().str() << std::endl;
            }
        }
        if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x1 ) != 0 )
        {
            size_t ks = findKernelTotalSize(Kernel->F, uniqKn, uniqProc);
            std::cout << "Kernel " << Kernel->F->getName().str() << " final size " << KernelSize << " real size "<< ks << std::endl;
        }
    }
}

size_t EstimateFunctionSize::findKernelTotalSize(llvm::Function* Kernel, uint32_t UK, uint32_t &UP)
{
    FunctionNode* k = get<FunctionNode>( Kernel );
    std::deque<FunctionNode*> TopDownQueue;
    std::deque<FunctionNode*> LeafNodes;

    UP++;  // unique process number for traversals of call graph
    TopDownQueue.push_back(get<FunctionNode>(Kernel));
    get<FunctionNode>( Kernel )->Processed = UP;
    // top down traversal to find leafNodes.  Could be done once outside this routine and pass here
    while (!TopDownQueue.empty()) {
        FunctionNode* Node = TopDownQueue.front();
        TopDownQueue.pop_front();
        Node->Size = Node->InitialSize;
        if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x8 ) != 0 )
        {
            std::cout << "tdqueue node " << Node->F->getName().str() << " size " << Node->Size << std::endl;
        }
        Node->Processed = UP; // processed
        Node->KernelNum = UK;

        for (auto Callee : Node->CalleeList) {
            FunctionNode* CalleeNode = get<FunctionNode>(Callee);
            if( CalleeNode->Processed != UP )  // Not processed yet
            {
                if( CalleeNode->isLeaf() ) {
                    CalleeNode->Size = CalleeNode->InitialSize;
                    LeafNodes.push_back( CalleeNode );
                    if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x8 ) != 0 )
                    {
                        std::cout << "found leaf node " << CalleeNode->F->getName().str() << " initSize=Size " << CalleeNode->Size << std::endl;
                    }
                }
                else
                {
                    TopDownQueue.push_back( CalleeNode );
                }
                CalleeNode->Processed = UP; // not to enter into queu again
            }
        }
    }
    IGC_ASSERT(!LeafNodes.empty());
    if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x8 ) != 0 )
    {
        std::cout << "Kernel " << Kernel->getName().str() << " initial size " << k->Size << std::endl;
    }
    // Expand leaf nodes until all are expanded (the second list is empty).
    UP++;
    while (!LeafNodes.empty()) {

        // Expand leaf nodes one by one.
        FunctionNode* Node = LeafNodes.front();
        LeafNodes.pop_front();
        if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x8 ) != 0 )
        {
            std::cout << "Visit leaf node " << Node->F->getName().str() << " size " << Node->Size << std::endl;
            if( Node->Processed == UP )
                std::cout << "  already processed " << std::endl;
        }

        if( Node->Processed == UP )
            continue; // already processed, e.g. the same caller might be enqueued multiple times -- CallerList duplication issue

        Node->Processed = UP; // processed

        if (Node->ToBeInlined) {
            // Populate size to its Callers.
            for (auto Caller : Node->CallerList) {
                get<FunctionNode>(Caller)->expandSpecial(Node);
                if( ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x8 ) != 0 )
                {
                    std::cout << "Expand to caller " << Caller->getName().str() << " from callee " << Node->F->getName().str() << " new size " << get<FunctionNode>( Caller )->Size << std::endl;
                }
            }
        }
        else {
            get<FunctionNode>(Kernel)->Size += Node->Size;
        }

        for (auto C : Node->CallerList) {
            FunctionNode* caller = get<FunctionNode>(C);
            if ( caller == k || caller->KernelNum != UK)
                continue; // Skip kernel entry and callers outside this kernel
            if( caller->Processed == UP )  // Already processed
                continue;
            bool is_new_leaf = true;
            for (auto S : caller->CalleeList) {
                if ( get<FunctionNode>(S)->Processed != UP) {
                    is_new_leaf = false;
                    break;
                }
            }
            if (is_new_leaf) {
                LeafNodes.push_back(caller);
            }
        }
    }

    return k->Size;
}

bool EstimateFunctionSize::isTrimmedFunction( llvm::Function* F) {
    return get<FunctionNode>(F)->ToBeInlined == false;
}
