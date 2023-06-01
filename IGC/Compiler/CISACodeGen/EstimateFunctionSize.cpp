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
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/SyntheticCountsUtils.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvmWrapper/IR/BasicBlock.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"
#include <deque>
#include <iostream>
#include <cfloat>

using namespace llvm;
using namespace IGC;
using Scaled64 = ScaledNumber<uint64_t>;
char EstimateFunctionSize::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(EstimateFunctionSize, "EstimateFunctionSize", "EstimateFunctionSize", false, true)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(BranchProbabilityInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(BlockFrequencyInfoWrapperPass)
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
    : ModulePass(ID), M(nullptr), AL(AL), tmpHasImplicitArg(false), HasRecursion(false), EnableSubroutine(false),
      threshold_func_freq(Scaled64::getLargest()), thresholdForTrimming(Scaled64::get(IGC_GET_FLAG_VALUE(ControlInlineSizeContribution))){}

EstimateFunctionSize::~EstimateFunctionSize() { clear(); }

void EstimateFunctionSize::getAnalysisUsage(AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<BranchProbabilityInfoWrapperPass>();
    AU.addRequired<BlockFrequencyInfoWrapperPass>();
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

#define PrintPartitionUnit(hex_val,contents) if ((IGC_GET_FLAG_VALUE(PrintPartitionUnit) & hex_val) != 0) {dbgs() << "PartitionUnit0x" << hex_val << ": " << contents << "\n";}
#define PrintControlUnitSize(hex_val,contents) if ((IGC_GET_FLAG_VALUE(PrintControlUnitSize) & hex_val) != 0) {dbgs() << "ControlUnitSize0x" << hex_val << ": " << contents << "\n";}
#define PrintControlKernelTotalSize(hex_val,contents) if ((IGC_GET_FLAG_VALUE(PrintControlKernelTotalSize) & hex_val) != 0) {dbgs() << "ControlKernelTotalSize0x" << hex_val << ": " << contents << "\n";}
#define PrintTrimUnit(hex_val,contents) if ((IGC_GET_FLAG_VALUE(PrintControlKernelTotalSize) & hex_val) != 0 || (IGC_GET_FLAG_VALUE(PrintControlUnitSize) & hex_val) != 0) {dbgs() << "TrimUnit0x" << hex_val << ": " << contents << "\n";}
#define PrintFunctionSizeAnalysis(hex_val,contents) if ((IGC_GET_FLAG_VALUE(PrintFunctionSizeAnalysis) & hex_val) != 0) {dbgs() << "FunctionSizeAnalysis0x" << hex_val << ": " << contents << "\n";}
#define PrintStaticProfilingForKernelSizeReduction(hex_val,contents) if ((IGC_GET_FLAG_VALUE(PrintStaticProfilingForKernelSizeReduction) & hex_val) != 0) {dbgs() << "StaticProfilingForKernelSizeReduction0x" << hex_val << ": " << contents << "\n";}

    typedef enum
    {
        SP_NO_METRIC = 0, /// \brief A flag to indicate whether no metric is used. We use this especially when we only need static profile infomation without enforcement
        SP_NORMAL_DISTRIBUTION = (0x1 << 0x0), /// \brief A flag to indicate whether a normal distribution is used as metric
        SP_LONGTAIL_DISTRIBUTION = (0x1 << 0x1),      /// \brief A flag to indicate whether a long tail distribution is used as metric
        SP_AVERAGE_PERCENTAGE = (0x1 << 0x2),    /// \brief A flag to indicate whether average % is used as metric
    } StatiProfile_FLAG_t;

    // Function Attribute Flag type
    typedef enum
    {
        FA_BEST_EFFORT_INLINE= 0,       /// \brief A flag to indicate whether it is to be inlined but it can be trimmed or assigned stackcall
        FA_FORCE_INLINE = (0x1 << 0x0), /// \brief A flag to indicate whether it is to be inlined and it cannot be reverted
        FA_TRIMMED = (0x1 << 0x1),      /// \brief A flag to indicate whetehr it will be trimmed
        FA_STACKCALL = (0x1 << 0x2),    /// \brief A flag to indicate whether this node should be a stack call header
        FA_KERNEL_ENTRY = (0x1 << 0x3), /// \brief A flag to indicate whether this node is a kernel entry. It will be affected by any schemes.
        FA_ADDR_TAKEN = (0x1 << 0x4),   /// \brief A flag to indicate whether this node is an address taken function.
    } FA_FLAG_t;
    /// Associate each function with a partially expanded size and remaining
    /// unexpanded function list, etc.

    typedef enum
    {
        FT_NOT_APPLICABLE = 0,       /// \brief A flag to indicate functions don't need to be considered
        FT_NOT_BEST_EFFORT = (0x1 << 0x1),    /// \brief A flag to indicate function is not open to trimming or partitioning
        FT_MUL_KERNEL = (0x1 << 0x2),    /// \brief A flag to indicate function is in multiple kernels and they are forced to be inlined
        FT_BIG_ENOUGH = (0x1 << 0x3),    /// \brief A flag to indicate functions are big enough to trim
        FT_TOO_TINY = (0x1 << 0x4),    /// \brief A flag to indicate function is too tiny to be trimmed
        FT_HIGHER_WEIGHT = (0x1 << 0x5),    /// \brief a flag to indicate the function has higher weight than threshold
        FT_LOWER_WEIGHT = (0x1 << 0x6),    /// \brief a flag to indicate the function has lower weight than threshold
    } FUNCTION_TRAIT_FLAG_t;
    struct FunctionNode {
        FunctionNode(Function* F, std::size_t Size)
            : F(F), InitialSize(Size), UnitSize(Size), ExpandedSize(Size), Inline_cnt(0), tmpSize(Size), CallingSubroutine(false),
            FunctionAttr(0), InMultipleUnit(false), HasImplicitArg(false), staticFuncFreq(0,0) {}

        Function* F;

        /// leaf node.

        /// \brief Initial size before partition
        uint32_t InitialSize;

        //  \brief the size of a compilation unit
        uint32_t UnitSize;

        /// \brief Expanded size when all functions in a unit below the node are expanded
        uint32_t ExpandedSize;

        /// \brief How many times the function is inlined at callsites.
        uint32_t Inline_cnt;

        /// \brief used to update unit size or expanded unit size in topological sort
        uint32_t tmpSize;

        /// \brief Function attribute
        uint8_t FunctionAttr;

        /// \brief An estimated static function frequency
        Scaled64 staticFuncFreq;

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

        void setStaticFuncFreq(Scaled64 freq) { staticFuncFreq = freq; }

        Scaled64 getStaticFuncFreq() { return staticFuncFreq; }

        std::string getStaticFuncFreqStr() { return staticFuncFreq.toString();}

        uint64_t getSizeContribution() { return Inline_cnt == 0 ? InitialSize : Inline_cnt * InitialSize; }
        Scaled64 getWeightForTrimming() {
            return staticFuncFreq == 0 ? Scaled64::get(getSizeContribution()) : Scaled64::get(getSizeContribution())/staticFuncFreq;
        }

        /// \brief A node becomes a leaf when all called functions are expanded.
        bool isLeaf() const { return CalleeList.empty(); }

        /// \brief Add a caller or callee.
        // A caller may call the same callee multiple times, e.g. A->{B,B,B}: A->CalleeList(B,B,B), B->CallerList(A,A,A)
        void addCallee(FunctionNode* G) {
            IGC_ASSERT(G);
            if (CalleeList.find(G) == CalleeList.end()) //First time added, Initialize it
                CalleeList[G] = 0;
            CalleeList[G] += 1;
            CallingSubroutine = true;
        }
        void addCaller(FunctionNode* G) {
            IGC_ASSERT(G);
            if (CallerList.find(G) == CallerList.end()) //First time added, Initialize it
                CallerList[G] = 0;
            CallerList[G] += 1;
        }

        void setKernelEntry()
        {
            FunctionAttr = FA_KERNEL_ENTRY;
            return;
        }
        void setAddressTaken()
        {
            FunctionAttr = FA_ADDR_TAKEN;
        }
        void setForceInline()
        {
            IGC_ASSERT(FunctionAttr != FA_KERNEL_ENTRY
                && FunctionAttr != FA_ADDR_TAKEN); //Can't force inline a kernel entry or address taken function
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
            //Can't assign stack call to force inlined function, kernel entry,
            //address taken functions and functions that already assigned stack call
            IGC_ASSERT(FunctionAttr == FA_BEST_EFFORT_INLINE || FunctionAttr ==  FA_TRIMMED);
            FunctionAttr = FA_STACKCALL;
            return;
        }

        bool isTrimmed() { return FunctionAttr == FA_TRIMMED; }
        bool isEntryFunc() { return FunctionAttr == FA_KERNEL_ENTRY;}
        bool isAddrTakenFunc() { return FunctionAttr == FA_ADDR_TAKEN; }
        bool willBeInlined() { return FunctionAttr == FA_BEST_EFFORT_INLINE || FunctionAttr == FA_FORCE_INLINE; }
        bool isStackCallAssigned() { return FA_STACKCALL == FunctionAttr; }
        bool canAssignStackCall()
        {
            if (FA_BEST_EFFORT_INLINE == FunctionAttr ||
                FA_TRIMMED == FunctionAttr) //The best effort inline or manually trimmed functions can be assigned stack call
                return true;
            return false;
        }

        uint16_t getFunctionTrait(Scaled64 thresholdForTrimming)
        {
            if (FunctionAttr != FA_BEST_EFFORT_INLINE) //Only best effort inline can be trimmed
                return FT_NOT_BEST_EFFORT;
            // to allow trimming functions called from other kernels, set the regkey to false
            if (IGC_IS_FLAG_ENABLED(ForceInlineExternalFunctions) && InMultipleUnit)
                return FT_MUL_KERNEL;

            if (IGC_IS_FLAG_ENABLED(StaticProfileGuidedInliningTrimming))
            {
                if (InitialSize < IGC_GET_FLAG_VALUE(ControlInlineTinySizeForSizeContribution)) //It's too small to trim
                    return FT_TOO_TINY;
                if (getWeightForTrimming() < thresholdForTrimming)
                    return FT_LOWER_WEIGHT;
                else
                    return FT_HIGHER_WEIGHT;
            }

            //This is a legacy mode trimming based on the size
            if (InitialSize < IGC_GET_FLAG_VALUE(ControlInlineTinySize)) //It's too small to trim
                return FT_TOO_TINY;
            else
                return FT_BIG_ENOUGH;

            return FT_NOT_APPLICABLE;
        }

        std::string getFuncAttrStr()
        {
            switch (FunctionAttr) {
            case FA_BEST_EFFORT_INLINE:
                return "Best effort innline";
            case FA_FORCE_INLINE:
                return "Force innline";
            case FA_TRIMMED:
                return "Trimmed";
            case FA_STACKCALL:
                return "Stack call";
            case FA_KERNEL_ENTRY:
                return "Kernel entry";
            case FA_ADDR_TAKEN:
                return "Address taken";
            default:
                return "Wrong value";
            }
            return "";
        }

        //Top down bfs to find the size of a compilation unit
        uint32_t updateUnitSize() {
            std::unordered_set<FunctionNode*> visit;
            std::deque<FunctionNode*> TopDownQueue;
            TopDownQueue.push_back(this);
            visit.insert(this);
            uint32_t total = 0;
            PrintFunctionSizeAnalysis(0x4, "Functions in the unit " << F->getName().str())
            while (!TopDownQueue.empty())
            {
                FunctionNode* Node = TopDownQueue.front();
                PrintFunctionSizeAnalysis(0x4, Node->F->getName().str() << ": " << Node->InitialSize)
                TopDownQueue.pop_front();
                total += Node->InitialSize;
                for (auto Callee : Node->CalleeList)
                {
                    FunctionNode* calleeNode = Callee.first;
                    if (visit.find(calleeNode) != visit.end() || calleeNode->isStackCallAssigned()) //Already processed or head of stack call
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
                PrintFunctionSizeAnalysis(0x4, "Func " << this->F->getName().str() << " expands to has implicit arg due to " << callee->F->getName().str())

                if (FunctionAttr != FA_KERNEL_ENTRY && FunctionAttr != FA_ADDR_TAKEN) //Can't inline kernel entry or address taken functions
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
    stackCallFuncs.clear();
    addressTakenFuncs.clear();
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
    else if( funcName.equals( GET_ASSERT_BUFFER ) )
    {
        matched = true;
    }
    if( matched && ( IGC_GET_FLAG_VALUE( PrintControlKernelTotalSize ) & 0x40 ) != 0 )
    {
        PrintFunctionSizeAnalysis(0x8, "Matched implicit arg " << funcName.str())
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


void EstimateFunctionSize::updateStaticFuncFreq()
{
    DenseMap<Function*, ScaledNumber<uint64_t>> Counts;
    auto MayHaveIndirectCalls = [](Function& F) {
        for (auto* U : F.users()) {
            if (!isa<CallInst>(U) && !isa<InvokeInst>(U))
                return true;
        }
        return false;
    };
    uint64_t InitialSyntheticCount = 10;
    uint64_t InlineSyntheticCount = 15;
    uint64_t ColdSyntheticCount = 5;
    for (Function& F : *M) {
        uint64_t InitialCount = InitialSyntheticCount;
        if (F.isDeclaration())
            continue;
        if (F.hasFnAttribute(llvm::Attribute::AlwaysInline) ||
            F.hasFnAttribute(llvm::Attribute::InlineHint)) {
            // Use a higher value for inline functions to account for the fact that
            // these are usually beneficial to inline.
            InitialCount = InlineSyntheticCount;
        }
        else if (F.hasLocalLinkage() && !MayHaveIndirectCalls(F)) {
            // Local functions without inline hints get counts only through
            // propagation.
            InitialCount = 0;
        }
        else if (F.hasFnAttribute(llvm::Attribute::Cold) ||
            F.hasFnAttribute(llvm::Attribute::NoInline)) {
            // Use a lower value for noinline and cold functions.
            InitialCount = ColdSyntheticCount;
        }
        Counts[&F] = Scaled64(InitialCount, 0);
    }
    // Edge includes information about the source. Hence ignore the first
    // parameter.
    auto GetCallSiteProfCount = [&](const CallGraphNode*,
        const CallGraphNode::CallRecord& Edge) {
#if LLVM_VERSION_MAJOR < 11
            Optional<Scaled64> Res = None;
            if (!Edge.first)
                return Res;
            assert(isa<Instruction>(Edge.first));
            CallSite CS(cast<Instruction>(Edge.first));
            Function* Caller = CS.getCaller();
            BasicBlock* CSBB = CS.getInstruction()->getParent();
#else
            Optional<Scaled64> Res = None;
            if (!Edge.first)
                return Res;
            CallBase& CB = *cast<CallBase>(*Edge.first);
            Function* Caller = CB.getCaller();
            BasicBlock* CSBB = CB.getParent();
#endif
            auto& BFI = getAnalysis<BlockFrequencyInfoWrapperPass>(*Caller).getBFI();
            // Now compute the callsite count from relative frequency and
            // entry count:

            Scaled64 EntryFreq(BFI.getEntryFreq(), 0);
            Scaled64 BBCount(BFI.getBlockFreq(CSBB).getFrequency(), 0);
            IGC_ASSERT(EntryFreq != 0);
            BBCount /= EntryFreq;
            BBCount *= Counts[Caller];
            return Optional<Scaled64>(BBCount);
    };
    CallGraph CG(*M);
    // Propgate the entry counts on the callgraph.
    SyntheticCountsUtils<const CallGraph*>::propagate(
        &CG, GetCallSiteProfCount, [&](const CallGraphNode* N, Scaled64 New) {
            auto F = N->getFunction();
            if (!F || F->isDeclaration())
                return;
            Counts[F] += New;
        });

    for (auto& F : M->getFunctionList()) {
        if (F.empty())
            continue;
        FunctionNode* Node = get<FunctionNode>(&F);

        if (Counts.find(&F) != Counts.end())
            Node->setStaticFuncFreq(Counts[&F]);
    }
    return;
}

void EstimateFunctionSize::runStaticAnalysis()
{
    //Analyze function frequencies from SyntheticCountsPropagation
    PrintStaticProfilingForKernelSizeReduction(0x1, "------------------Static analysis start------------------")
    updateStaticFuncFreq();
    std::vector<Scaled64> freqLog;
    if (IGC_GET_FLAG_VALUE(BlockFrequencySampling)) //Set basic blocks as the sample space
    {
        for (auto& F : M->getFunctionList()) {
            if (F.empty())
                continue;
            FunctionNode* Node = get<FunctionNode>(&F);
            auto& BFI = getAnalysis<BlockFrequencyInfoWrapperPass>(F).getBFI();
            Scaled64 EntryFreq(BFI.getEntryFreq(), 0);
            for (auto& B : F.getBasicBlockList())
            {
                Scaled64 BBCount(BFI.getBlockFreq(&B).getFrequency(), 0);

                BBCount /= EntryFreq;
                BBCount *= Node->getStaticFuncFreq();
                PrintStaticProfilingForKernelSizeReduction(0x1, "Block frequency of " << B.getName().str() << ": " << BBCount.toString())

                if (BBCount > 0) //Can't represent 0 in log scale so ignore, better idea?
                    freqLog.push_back(BBCount);
            }
        }
    }
    else
    {
        for (auto& F : M->getFunctionList())
        {
            if (F.empty())
                continue;
            FunctionNode* Node = get<FunctionNode>(&F);
            PrintStaticProfilingForKernelSizeReduction(0x1, "Function frequency of " << Node->F->getName().str() << ": " << Node->getStaticFuncFreqStr())
            if (Node->getStaticFuncFreq() > 0) //Can't represent 0 in log scale so ignore, better idea?
                freqLog.push_back(Node->getStaticFuncFreq());
        }
    }

    if ((IGC_GET_FLAG_VALUE(MetricForKernelSizeReduction) & SP_NORMAL_DISTRIBUTION) != 0 && !freqLog.empty()) //When using a normal distribution. Ignore when there are no frequency data
    {
        IGC_ASSERT(IGC_GET_FLAG_VALUE(ParameterForColdFuncThreshold) >= 0 && IGC_GET_FLAG_VALUE(ParameterForColdFuncThreshold) <= 30);
        //Find a threshold from a normal distribution
        std::sort(freqLog.begin(), freqLog.end());  //Sort frequency data
        std::vector<double> freqLogDbl;
        std::unordered_map<double, Scaled64> map_log10_to_scaled64;
        double log10_2 = std::log10(2);
        for (Scaled64& val : freqLog) //transform into log10 scale
        {
            double logedVal = std::log10(val.getDigits()) + val.getScale() * log10_2;
            map_log10_to_scaled64[logedVal] = val;
            freqLogDbl.push_back(logedVal);
        }
        double sum_val = std::accumulate(freqLogDbl.begin(), freqLogDbl.end(), 0.0);
        double mean = sum_val / freqLogDbl.size();
        double sq_sum = std::inner_product(freqLogDbl.begin(), freqLogDbl.end(), freqLogDbl.begin(), 0.0,
            [](double const& x, double const& y) {return x + y;},
            [mean](double const& x, double const& y) {return (x - mean) * (y - mean);});
        double standard_deviation = std::sqrt(sq_sum / freqLogDbl.size());
        float C = (float)IGC_GET_FLAG_VALUE(ParameterForColdFuncThreshold) / 10; //Since 1 STD is too wide in the majority case, we need to scale down
        double threshold_log10 = mean - C * standard_deviation;
        auto it_lower = std::lower_bound(freqLogDbl.begin(), freqLogDbl.end(), threshold_log10);
        if (it_lower == freqLogDbl.end())
            threshold_func_freq = freqLog.back();
        else
            threshold_func_freq = map_log10_to_scaled64[*it_lower];
        thresholdForTrimming = thresholdForTrimming / threshold_func_freq;
        PrintStaticProfilingForKernelSizeReduction(0x1, "Metric: Normal distribution");
        PrintStaticProfilingForKernelSizeReduction(0x1, "Sample count: " << freqLogDbl.size());
        PrintStaticProfilingForKernelSizeReduction(0x1, "Execution frequency mean (Log10 scale): " << mean);
        PrintStaticProfilingForKernelSizeReduction(0x1, "Standard deviation (Log10 scale): " << standard_deviation);
        PrintStaticProfilingForKernelSizeReduction(0x1, "Execution frequency threshold with Constant(C) " << C << ": " << threshold_func_freq.toString());
    }
    else if ((IGC_GET_FLAG_VALUE(MetricForKernelSizeReduction) & SP_LONGTAIL_DISTRIBUTION) != 0 && !freqLog.empty()) //When using a long-tail distribution. Ignore when there are no frequency data
    {
        IGC_ASSERT(IGC_GET_FLAG_VALUE(ParameterForColdFuncThreshold) > 0 && IGC_GET_FLAG_VALUE(ParameterForColdFuncThreshold) <= 100);
        //Find a threshold from a long tail distribution
        uint32_t threshold_cold = (uint32_t)IGC_GET_FLAG_VALUE(ParameterForColdFuncThreshold);
        uint32_t C_pos = freqLog.size() * threshold_cold / 100;
        std::nth_element(freqLog.begin(), freqLog.begin() + C_pos, freqLog.end(),
            [](Scaled64& x, Scaled64& y) {return x < y;}); //Low C%
        threshold_func_freq = freqLog[C_pos];
        thresholdForTrimming = thresholdForTrimming / threshold_func_freq;
        PrintStaticProfilingForKernelSizeReduction(0x1, "Metric: Long tail distribution");
        PrintStaticProfilingForKernelSizeReduction(0x1, "Low " << threshold_cold << "% pos: " << C_pos << " out of " << freqLog.size());
        PrintStaticProfilingForKernelSizeReduction(0x1, "Execution frequency threshold: " << threshold_func_freq);
    }
    else if ((IGC_GET_FLAG_VALUE(MetricForKernelSizeReduction) & SP_AVERAGE_PERCENTAGE) != 0 && !freqLog.empty()) //When using a average C%
    {
        Scaled64 sum_val = std::accumulate(freqLog.begin(), freqLog.end(), Scaled64::getZero());
        Scaled64 mean = sum_val / Scaled64::get(freqLog.size());
        Scaled64 C = Scaled64::get(IGC_GET_FLAG_VALUE(ParameterForColdFuncThreshold)) / Scaled64::get(10); //Scale down /10
        IGC_ASSERT(C > 0 && C <= 100);
        threshold_func_freq = mean * (C / Scaled64::get(100));
        thresholdForTrimming = thresholdForTrimming / threshold_func_freq;
        PrintStaticProfilingForKernelSizeReduction(0x1, "Metric: Average%");
        PrintStaticProfilingForKernelSizeReduction(0x1, "Average threshold * " << C.toString() << "%: " << threshold_func_freq.toString());
    }

    PrintStaticProfilingForKernelSizeReduction(0x1, "------------------Static analysis end------------------\n")
    return;
}
void EstimateFunctionSize::analyze() {
    auto getSize = [](llvm::Function& F) -> std::size_t {
        std::size_t Size = 0;
        for (auto& BB : F.getBasicBlockList())
            Size += IGCLLVM::sizeWithoutDebug(&BB);
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
                BasicBlock* BB = CI->getParent();
                Function* G = BB->getParent();
                FunctionNode* GN = get<FunctionNode>(G);
                GN->addCallee(Node);
                Node->addCaller(GN);
            }
        }
    }

    //Find all address taken functions
    for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I)
    {
        FunctionNode* Node = (FunctionNode*)I->second;
        //Address taken functions neither have callers nor is an entry function
        if (Node->CallerList.empty() && !Node->isEntryFunc())
            Node->setAddressTaken();
    }

    bool needImplAnalysis = IGC_IS_FLAG_ENABLED(ControlInlineImplicitArgs) || IGC_IS_FLAG_ENABLED(ForceInlineStackCallWithImplArg);
    // check functions and mark those that use implicit args.
    PrintFunctionSizeAnalysis(0x1, "--------------------------Function size analysis start--------------------------");
    if (needImplAnalysis)
        performImplArgsAnalysis();

    // Update expanded and static unit size and propagate implicit argument information which might cancel some stackcalls
    for (void *entry : kernelEntries)
    {
        FunctionNode* kernelEntry = (FunctionNode*)entry;
        updateExpandedUnitSize(kernelEntry->F, true);
        kernelEntry->updateUnitSize();
        PrintFunctionSizeAnalysis(0x1, "Unit size (kernel entry) " << kernelEntry->F->getName().str() << ": " << kernelEntry->UnitSize);
        PrintFunctionSizeAnalysis(0x1, "Expanded unit size (kernel entry) " << kernelEntry->F->getName().str() << ": " << kernelEntry->ExpandedSize);
    }

    // Find all survived stackcalls and address taken functions and update unit sizes
    for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I)
    {
        FunctionNode* Node = (FunctionNode*)I->second;
        if (Node->isStackCallAssigned())
        {
            stackCallFuncs.push_back(Node);
            Node->updateUnitSize();
            PrintFunctionSizeAnalysis(0x1, "Unit size (stack call) " << Node->F->getName().str() << ": " << Node->UnitSize);
        }
        else if (Node->isAddrTakenFunc())
        {
            addressTakenFuncs.push_back(Node);
            updateExpandedUnitSize(Node->F, true);
            Node->updateUnitSize();
            PrintFunctionSizeAnalysis(0x1, "Unit size (address taken) " << Node->F->getName().str() << ": " << Node->UnitSize);
            PrintFunctionSizeAnalysis(0x1, "Expanded unit size (address taken) " << Node->F->getName().str() << ": " << Node->ExpandedSize);
        }
    }
    PrintFunctionSizeAnalysis(0x1, "Function count= " << ECG.size());
    PrintFunctionSizeAnalysis(0x1, "Kernel count= " << kernelEntries.size());
    PrintFunctionSizeAnalysis(0x1, "Manual stack call count= " << stackCallFuncs.size());
    PrintFunctionSizeAnalysis(0x1, "Address taken function call count= " << addressTakenFuncs.size());
    PrintFunctionSizeAnalysis(0x1, "--------------------------Function size analysis end--------------------------\n");
    return;
}

void EstimateFunctionSize::performImplArgsAnalysis()
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
        static int cnt = 0;
        const char* Name;
        if (Node->isLeaf())
            Name = "Leaf";
        else
            Name = "nonLeaf";
        PrintFunctionSizeAnalysis(0x8, Name << " Func " << ++cnt << " " << Node->F->getName().str() << " calls implicit args so HasImplicitArg")

            if (Node->isEntryFunc() || Node->isAddrTakenFunc()) //Can't inline kernel entry or address taken functions
                continue;

        if (Node->isStackCallAssigned()) //When stackcall is assigned we need to determine based on the flag
        {
            if (IGC_IS_FLAG_ENABLED(ForceInlineStackCallWithImplArg))
                Node->setForceInline();
            continue;
        }

        //For other cases
        if (IGC_IS_FLAG_ENABLED(ControlInlineImplicitArgs)) //Force inline ordinary functions with implicit arguments
            Node->setForceInline();
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
    for (auto I : addressTakenFuncs) {
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
        uint32_t subroutineThreshold = IGC_GET_FLAG_VALUE(SubroutineThreshold);
        uint32_t expandedMaxSize = getMaxExpandedSize();

        if (AL != AL_Module) // at the second call of EstimationFucntionSize, halve the threshold
            subroutineThreshold = subroutineThreshold >> 1;

        if (expandedMaxSize <= subroutineThreshold )
        {
            PrintTrimUnit(0x1, "No need to reduce the kernel size. (The max expanded kernel size is small) " << expandedMaxSize << " < " << subroutineThreshold)
            if(!HasRecursion)
                EnableSubroutine = false;
        }
        else if (AL == AL_Module && IGC_IS_FLAG_DISABLED(DisableAddingAlwaysAttribute)) //kernel trimming and partitioning only kick in at the first EstimationFunctionSize
        {
            //Analyze Function/Block frequencies
            if (IGC_IS_FLAG_ENABLED(StaticProfilingForPartitioning) ||
                IGC_IS_FLAG_ENABLED(StaticProfileGuidedInliningTrimming)) // Either a normal or long-tail distribution is enabled
                runStaticAnalysis();

            // If the max unit size exceeds threshold, do partitioning
            if (IGC_IS_FLAG_ENABLED(PartitionUnit))
            {
                PrintPartitionUnit(0x1, "--------------------------Partition unit start--------------------------");
                uint32_t unitThreshold = IGC_GET_FLAG_VALUE(UnitSizeThreshold);
                uint32_t maxUnitSize = getMaxUnitSize();
                if (maxUnitSize > unitThreshold)
                {
                    PrintPartitionUnit(0x1, "Max unit size " << maxUnitSize << " is larger than the threshold (to partition) " << unitThreshold)
                    partitionKernel();
                }
                else
                {
                    PrintPartitionUnit(0x1, "Max unit size " << maxUnitSize << " is smaller than the threshold (No partitioning needed) " << unitThreshold)
                }
                PrintPartitionUnit(0x1, "--------------------------Partition unit end--------------------------\n");
            }

            PrintTrimUnit(0x1, "Need to reduce the kernel size. (The max expanded kernel size is large) " << expandedMaxSize << " > " << subroutineThreshold)
            PrintTrimUnit(0x1, "-----------------------------Trimming start-----------------------------")
            if (IGC_IS_FLAG_ENABLED(ControlKernelTotalSize))
            {
                reduceKernelSize();
            }
            else if (IGC_IS_FLAG_ENABLED(ControlUnitSize))
            {
                reduceCompilationUnitSize();
            }
            PrintTrimUnit(0x1, "-----------------------------Trimming end-----------------------------\n")
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
    llvm::SmallVector<void*, 64> unitHeads;
    for (auto node : kernelEntries)
        unitHeads.push_back((FunctionNode*)node);
    for (auto node : addressTakenFuncs)
        unitHeads.push_back((FunctionNode*)node);
    trimCompilationUnit(unitHeads, threshold, true);
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
            if (FunctionsInKernel.find(CalleeNode) != FunctionsInKernel.end())
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

//Update inlined sizes of functions
void EstimateFunctionSize::updateInlineCnt(Function *root)
{
    FunctionNode* root_node = get<FunctionNode>(root);
    std::unordered_map<void*, uint32_t> unprocessed_callers;//A data structure to collect the number of callers for a functoin in a kernel boundary
    unprocessed_callers[root_node] = 0;

    std::deque<void*> TopdownQueue;TopdownQueue.push_back(root_node);

    std::unordered_set<void*> visit;visit.insert(root_node);

    //Top down traversal to initialize the number of callers and inline count in a kernel boundary
    //This step is just for initialization for the topological traverse at the second step
    while (!TopdownQueue.empty())
    {
        FunctionNode* node = (FunctionNode*)TopdownQueue.front(); TopdownQueue.pop_front();
        node->Inline_cnt = 0;
        for (auto callee_info : node->CalleeList)
        {
            FunctionNode* callee = callee_info.first;
            if (unprocessed_callers.find(callee) == unprocessed_callers.end())
                unprocessed_callers[callee] = 0; //Initialize callee's caller count

            unprocessed_callers[callee] += 1; //Increment by 1 since the callee is called by the node
            if (visit.find(callee) == visit.end())
            {
                visit.insert(callee);
                TopdownQueue.push_back(callee);
            }
        }
    }
    TopdownQueue.push_back(root_node);
    while (!TopdownQueue.empty())
    {
        FunctionNode* node = (FunctionNode*)TopdownQueue.front(); TopdownQueue.pop_front();
        for (auto callee_info : node->CalleeList)
        {
            FunctionNode* callee = callee_info.first;
            uint16_t call_cnt = callee_info.second;
            IGC_ASSERT(unprocessed_callers[callee] != 0);
            unprocessed_callers[callee] -= 1;
            if(callee->willBeInlined())
                callee->Inline_cnt += call_cnt * (node->Inline_cnt == 0 ? 1 : node->Inline_cnt);
            if (unprocessed_callers[callee] == 0)
                TopdownQueue.push_back(callee);
        }
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
            //dbgs() << "Not be inlined Attr: " << (int)node->FunctionAttr << "\n";
            unitTotalSize += node->ExpandedSize;
        }

        for (auto c : node->CallerList)
        {
            FunctionNode* caller = c.first;
            if (FunctionsInUnit.find(caller) == FunctionsInUnit.end()) //Caller is in another compilation unit
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
            Node->UnitSize > threshold && Node->updateUnitSize() > threshold &&
            Node->getStaticFuncFreq() < threshold_func_freq;

        if (beStackCall)
        {
            PrintPartitionUnit(0x4, "Stack call marked " << Node->F->getName().str() << " Unit size: " << Node->UnitSize << " > Threshold " << threshold
                << " Function frequency: " << Node->getStaticFuncFreqStr() << " < " << threshold_func_freq.toString())
            stackCallFuncs.push_back(Node); //We have a new unit head
            Node->setStackCall();
            max_unit_size = std::max(max_unit_size, Node->UnitSize);
            stackCall_cnt += 1;
        }
        else
        {
            if (!Node->canAssignStackCall())
            {
                PrintPartitionUnit(0x4, "Stack call not marked: not best effort or trimmed " << Node->F->getName().str())
            }
            else if (Node->UnitSize <= threshold || Node->updateUnitSize() <= threshold)
            {
                PrintPartitionUnit(0x4, "Stack call not marked: unit size too small " << Node->F->getName().str())
            }
            else
            {
                PrintPartitionUnit(0x4, "Stack call not marked: too many function frequencies " << Node->getStaticFuncFreqStr()
                                    << " > " << threshold_func_freq.toString() << " " << Node->F->getName().str())
            }
        }

        for (auto c : Node->CallerList)
        {
            FunctionNode* caller = c.first;
            if (FunctionsInUnit.find(caller) == FunctionsInUnit.end()) //The caller is in another kernel, skip
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
    for (auto node : stackCallFuncs)
        unitHeads.push_back((FunctionNode*)node);
    for (auto node : addressTakenFuncs)
        unitHeads.push_back((FunctionNode*)node);

    for (auto node : unitHeads) {
        FunctionNode* UnitHead = (FunctionNode*)node;
        if (UnitHead->UnitSize <= threshold) //Unit size is within threshold, skip
        {
            max_unit_size = std::max(max_unit_size, UnitHead->UnitSize);
            continue;
        }
        PrintPartitionUnit(0x2, "Partition Kernel " << UnitHead->F->getName().str() << " Original Unit Size: " << UnitHead->UnitSize)
        uint32_t size_after_partition = bottomUpHeuristic(UnitHead->F, stackCall_cnt);
        max_unit_size = std::max(max_unit_size, size_after_partition);
        PrintPartitionUnit(0x2, "Unit size after partitioning: " << size_after_partition)
    }
    float threshold_err = (float)(max_unit_size - threshold) / threshold * 100;
    PrintPartitionUnit(0x2, "Max unit size: " << max_unit_size << " Threshold Error Rate: " << threshold_err << "%");
    PrintPartitionUnit(0x2, "Stack call cnt: " << stackCall_cnt);
    return;
}

//Work same as reduceKernel except for stackcall functions
void EstimateFunctionSize::reduceCompilationUnitSize() {
    uint32_t threshold = IGC_GET_FLAG_VALUE(ExpandedUnitSizeThreshold);
    llvm::SmallVector<void*, 64> unitHeads;
    for (auto node : kernelEntries)
        unitHeads.push_back((FunctionNode*)node);
    for (auto node : stackCallFuncs)
        unitHeads.push_back((FunctionNode*)node);
    for (auto node : addressTakenFuncs)
        unitHeads.push_back((FunctionNode*)node);

    trimCompilationUnit(unitHeads, threshold,false);
    return;
}

//Top down traverse to find and retrieve functions that meet trimming criteria
void EstimateFunctionSize::getFunctionsToTrim(llvm::Function* root, llvm::SmallVector<void*, 64>& trimming_pool, bool ignoreStackCallBoundary, uint32_t &func_cnt)
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
        func_cnt += 1;
        uint64_t tinySizeThreshold = IGC_IS_FLAG_ENABLED(StaticProfileGuidedInliningTrimming) ? IGC_GET_FLAG_VALUE(ControlInlineTinySizeForSizeContribution) : IGC_GET_FLAG_VALUE(ControlInlineTinySize);
        uint16_t func_trait = Node->getFunctionTrait(thresholdForTrimming);

        switch (func_trait)
        {
        case FT_NOT_BEST_EFFORT:
            PrintTrimUnit(0x4, "Can't trim (not best effort inline) " << Node->F->getName().str()
                << " Function Attribute: " << Node->getFuncAttrStr());
            break;
        case FT_MUL_KERNEL:
            PrintTrimUnit(0x4, "Can't trim (in multiple kernels) " << Node->F->getName().str());
            break;
        case FT_BIG_ENOUGH://Functions are big enough to trim
            trimming_pool.push_back(Node);
            PrintTrimUnit(0x4, "Good to trim (Big enough) " << Node->F->getName().str()
                << " function size: " << Node->InitialSize << " > " << tinySizeThreshold);
            break;
        case FT_TOO_TINY:
            PrintTrimUnit(0x4, "Can't trim (Too tiny) " << Node->F->getName().str()
                << " function size: " << Node->InitialSize << " < " << tinySizeThreshold);
            break;
        case FT_HIGHER_WEIGHT:
            trimming_pool.push_back(Node);
            PrintTrimUnit(0x4, "Good to trim (High weight) " << Node->F->getName().str()
                << " Size contribution: " << Node->getSizeContribution()
                << " Freq: " << Node->getStaticFuncFreqStr()
                << " Weight: " << Node->getWeightForTrimming().toString() << " > " << thresholdForTrimming);
            break;
        case FT_LOWER_WEIGHT:
            PrintTrimUnit(0x4, "Can't trim (Low weight) " << Node->F->getName().str()
                << " Size contribution: " << Node->getSizeContribution()
                << " Freq: " << Node->getStaticFuncFreqStr()
                << " Weight: " << Node->getWeightForTrimming().toString() << " < " << thresholdForTrimming);
            break;
        default:
            PrintTrimUnit(0x4, "Something goes wrong with the function property " << Node->F->getName().str());
            break;
        }


        for (auto Callee : Node->CalleeList)
        {
            FunctionNode* calleeNode = Callee.first;
            if (visit.find(calleeNode) != visit.end() || (!ignoreStackCallBoundary && calleeNode->isStackCallAssigned()))
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
    llvm::SmallVector<FunctionNode*, 64> unitsToTrim;
    //Extract kernels / units that are larger than threshold
    for (auto node : unitHeads)
    {
        FunctionNode* unitEntry = (FunctionNode*)node;
        //Partitioning can add more stackcalls. So need to recompute the expanded unit size.
        updateExpandedUnitSize(unitEntry->F, ignoreStackCallBoundary);
        if (unitEntry->ExpandedSize > threshold)
        {
            PrintTrimUnit(0x2, "Kernel / Unit " << unitEntry->F->getName().str() << " expSize= " << unitEntry->ExpandedSize << " > " << threshold)
            unitsToTrim.push_back(unitEntry);
        }
        else
            PrintTrimUnit(0x2, "Kernel / Unit " << unitEntry->F->getName().str() << " expSize= " << unitEntry->ExpandedSize << " <= " << threshold)
    }

    if (unitsToTrim.empty())
    {
        PrintTrimUnit(0x2, "Kernels / Units become no longer big enough to be trimmed (affected by partitioning)")
        return;
    }

    std::sort(unitsToTrim.begin(), unitsToTrim.end(),
        [&](const FunctionNode* LHS, const FunctionNode* RHS) { return LHS->ExpandedSize > RHS->ExpandedSize;}); //Sort by expanded size

    // Iterate over units
    for (auto unit : unitsToTrim) {
        size_t expandedUnitSize = updateExpandedUnitSize(unit->F, ignoreStackCallBoundary); //A kernel size can be reduced by a function that is trimmed at previous kernels, so recompute it.
        if(IGC_IS_FLAG_ENABLED(StaticProfileGuidedInliningTrimming))
            updateInlineCnt(unit->F);
        PrintTrimUnit(0x2, "Trimming kernel / unit " << unit->F->getName().str() << " expanded size= " << expandedUnitSize)
        if (expandedUnitSize <= threshold) {
            PrintTrimUnit(0x2, "Kernel / unit " << unit->F->getName().str() << ": The expanded unit size(" << expandedUnitSize << ") is smaller than threshold(" << threshold << ")")
            continue;
        }
        PrintTrimUnit(0x2, "Kernel size is bigger than threshold")

        SmallVector<void*, 64> trimming_pool;

        uint32_t func_cnt = 0;
        getFunctionsToTrim(unit->F, trimming_pool, ignoreStackCallBoundary, func_cnt);
        PrintTrimUnit(0x2, "Kernel / Unit " << unit->F->getName().str() << " has " << trimming_pool.size() << " functions for trimming out of " << func_cnt)
        if (trimming_pool.empty())
        {
            PrintTrimUnit(0x2, "Kernel / Unit " << unit->F->getName().str() << " size " << unit->ExpandedSize << " has no sorted list")
            continue; // all functions are tiny.
        }
        uint64_t size_before_trimming = unit->ExpandedSize;
        performTrimming(unit->F,trimming_pool,threshold,ignoreStackCallBoundary); //Trim cold functions
        if (!trimming_pool.empty())
        {
            PrintTrimUnit(0x2, "Kernel / Unit " << unit->F->getName().str() << ": The size becomes below threshold")
        }
        else
        {
            PrintTrimUnit(0x2, "Kernel / Unit " << unit->F->getName().str() << ": The size is still above threhosld even though all candidates are trimmed")
        }

        PrintTrimUnit(0x2, "Kernel / Unit " << unit->F->getName().str() << " final size " << unit->ExpandedSize << " reduced from " << size_before_trimming)
    }
}

void EstimateFunctionSize::performTrimming(Function *head, llvm::SmallVector<void*, 64>& functions_to_trim, uint32_t threshold, bool ignoreStackCallBoundary)
{
    FunctionNode* unitHead = get<FunctionNode>(head);
    //Sort all to-be trimmed function according to the its actual size

    //Repeat trimming functions for cold functions until the unit size is smaller than threshold
    while (!functions_to_trim.empty() && unitHead->ExpandedSize >= threshold)
    {
        std::sort(functions_to_trim.begin(), functions_to_trim.end(),
            [&](const void* LHS, const void* RHS) {
                if (IGC_IS_FLAG_ENABLED(StaticProfileGuidedInliningTrimming))
                    return ((FunctionNode*)LHS)->getWeightForTrimming() < ((FunctionNode*)RHS)->getWeightForTrimming();
                return ((FunctionNode*)LHS)->InitialSize < ((FunctionNode*)RHS)->InitialSize;
            }); //Sort by the expanded size in an ascending order;
        FunctionNode* functionToTrim = (FunctionNode*)functions_to_trim.back(); //Pick the largest one first to trim
        functions_to_trim.pop_back();
        uint64_t original_expandedSize = unitHead->ExpandedSize;

        if (IGC_IS_FLAG_ENABLED(StaticProfileGuidedInliningTrimming))
        {
            uint64_t size_contribution = functionToTrim->getSizeContribution();
            if (functionToTrim->InitialSize == size_contribution) //We don't need to trim function that will be called just one time
            {
                PrintTrimUnit(0x8, "Do not trim the funciton since a potential size contribution is the same as its original size: " << size_contribution);
                continue;
            }
            //Trim the function
            PrintTrimUnit(0x8, "Trim the function " << functionToTrim->F->getName().str() << " Initial Size: " << functionToTrim->InitialSize << " Size contribution: " << size_contribution);
            functionToTrim->setTrimmed();
            //update size contribution
            updateInlineCnt(head);
            PrintTrimUnit(0x8, "The size contribution of the trimmed function changes to " << functionToTrim->getSizeContribution());
        }
        else
        {
            //Trim the function
            PrintTrimUnit(0x8, "Trim the function " << functionToTrim->F->getName().str() << " Initial Size: " << functionToTrim->InitialSize);
            functionToTrim->setTrimmed();
        }
        //After trimming, update exapnded size
        updateExpandedUnitSize(head, ignoreStackCallBoundary);
        PrintTrimUnit(0x8, "The kernel size is reduced after trimming from " << original_expandedSize << " to " << unitHead->ExpandedSize);
    }
    return;
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
    for (auto stackCallFunc : stackCallFuncs) //For all address taken functions, update unitsize
    {
        FunctionNode* head = (FunctionNode*)stackCallFunc;
        max_val = std::max(max_val, head->UnitSize);
    }
    for (auto addrTakenFunc : addressTakenFuncs) //For all address taken functions, update unitsize
    {
        FunctionNode* head = (FunctionNode*)addrTakenFunc;
        max_val = std::max(max_val, head->UnitSize);
    }
    return max_val;
}