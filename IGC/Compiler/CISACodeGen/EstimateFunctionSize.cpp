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
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/SyntheticCountsUtils.h"
#include "llvm/Analysis/CallGraph.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/BasicBlock.h"
#include "llvmWrapper/ADT/Optional.h"
#include "Probe/Assertion.h"
#include <deque>
#include <cfloat>
#include <algorithm>
#include <cmath>
#include <optional>

using namespace llvm;
using namespace IGC;
using Scaled64 = ScaledNumber<uint64_t>;
char EstimateFunctionSize::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(EstimateFunctionSize, "EstimateFunctionSize", "EstimateFunctionSize", false, true)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(BranchProbabilityInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(BlockFrequencyInfoWrapperPass)
IGC_INITIALIZE_PASS_END(EstimateFunctionSize, "EstimateFunctionSize", "EstimateFunctionSize", false, true)

llvm::ModulePass *IGC::createEstimateFunctionSizePass() {
  initializeEstimateFunctionSizePass(*PassRegistry::getPassRegistry());
  return new EstimateFunctionSize;
}

llvm::ModulePass *IGC::createEstimateFunctionSizePass(bool EnableStaticProfileGuidedTrimming) {
  initializeEstimateFunctionSizePass(*PassRegistry::getPassRegistry());
  return new EstimateFunctionSize(EstimateFunctionSize::AnalysisLevel::AL_Module, EnableStaticProfileGuidedTrimming);
}

llvm::ModulePass *IGC::createEstimateFunctionSizePass(EstimateFunctionSize::AnalysisLevel AL) {
  initializeEstimateFunctionSizePass(*PassRegistry::getPassRegistry());
  return new EstimateFunctionSize(AL, false);
}

EstimateFunctionSize::EstimateFunctionSize(AnalysisLevel AL, bool EnableStaticProfileGuidedTrimming)
    : ModulePass(ID), M(nullptr), AL(AL), tmpHasImplicitArg(false), HasRecursion(false), EnableSubroutine(false) {
  thresholdForTrimming = Scaled64::get(IGC_GET_FLAG_VALUE(ControlInlineTinySizeForSPGT));
  threshold_func_freq = Scaled64::getLargest();

  // Flags for Kernel trimming
  ControlKernelTotalSize = IGC_IS_FLAG_ENABLED(ControlKernelTotalSize);
  ControlUnitSize = IGC_IS_FLAG_ENABLED(ControlUnitSize);
  ControlInlineTinySize = IGC_GET_FLAG_VALUE(ControlInlineTinySize);
  UnitSizeThreshold = IGC_GET_FLAG_VALUE(UnitSizeThreshold);

  // Flags for Static Profile-guided trimming
  StaticProfileGuidedTrimming = IGC_IS_FLAG_ENABLED(StaticProfileGuidedTrimming);
  UseFrequencyInfoForSPGT = IGC_IS_FLAG_ENABLED(UseFrequencyInfoForSPGT);
  BlockFrequencySampling = IGC_IS_FLAG_ENABLED(BlockFrequencySampling);
  EnableLeafCollapsing = IGC_IS_FLAG_ENABLED(EnableLeafCollapsing);
  EnableSizeContributionOptimization = IGC_IS_FLAG_ENABLED(EnableSizeContributionOptimization);
  LoopCountAwareTrimming = IGC_IS_FLAG_ENABLED(LoopCountAwareTrimming);
  EnableGreedyTrimming = IGC_IS_FLAG_ENABLED(EnableGreedyTrimming);
  SizeWeightForSPGT = IGC_GET_FLAG_VALUE(SizeWeightForSPGT);
  FrequencyWeightForSPGT = IGC_GET_FLAG_VALUE(FrequencyWeightForSPGT);
  MetricForKernelSizeReduction = IGC_GET_FLAG_VALUE(MetricForKernelSizeReduction);
  ParameterForColdFuncThreshold = IGC_GET_FLAG_VALUE(ParameterForColdFuncThreshold);
  ControlInlineTinySizeForSPGT = IGC_GET_FLAG_VALUE(ControlInlineTinySizeForSPGT);
  MaxUnrollCountForFunctionSizeAnalysis = IGC_GET_FLAG_VALUE(MaxUnrollCountForFunctionSizeAnalysis);
  SkipTrimmingOneCopyFunction = IGC_GET_FLAG_VALUE(SkipTrimmingOneCopyFunction);
  SelectiveTrimming = IGC_GET_REGKEYSTRING(SelectiveTrimming);
  // Flags for Partitioning
  PartitionUnit = IGC_IS_FLAG_ENABLED(PartitionUnit);
  StaticProfileGuidedPartitioning = IGC_IS_FLAG_ENABLED(StaticProfileGuidedPartitioning);

  // Flags for implcit arguments and external functions
  ForceInlineExternalFunctions = IGC_IS_FLAG_ENABLED(ForceInlineExternalFunctions);
  ForceInlineStackCallWithImplArg = IGC_IS_FLAG_ENABLED(ForceInlineStackCallWithImplArg);
  ControlInlineImplicitArgs = IGC_IS_FLAG_ENABLED(ControlInlineImplicitArgs);
  SubroutineThreshold = IGC_GET_FLAG_VALUE(SubroutineThreshold);
  LargeKernelThresholdMultiplier = IGC_GET_FLAG_VALUE(LargeKernelThresholdMultiplier);
  KernelTotalSizeThreshold = IGC_GET_FLAG_VALUE(KernelTotalSizeThreshold);
  ExpandedUnitSizeThreshold = IGC_GET_FLAG_VALUE(ExpandedUnitSizeThreshold);
  if (EnableStaticProfileGuidedTrimming) {
    StaticProfileGuidedTrimming = true;
    EnableLeafCollapsing = true;
    EnableSizeContributionOptimization = true;
    LoopCountAwareTrimming = true;
  }
}

EstimateFunctionSize::~EstimateFunctionSize() { clear(); }

void EstimateFunctionSize::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<BranchProbabilityInfoWrapperPass>();
  AU.addRequired<BlockFrequencyInfoWrapperPass>();
  AU.addRequired<ScalarEvolutionWrapperPass>();
}

bool EstimateFunctionSize::runOnModule(Module &Mod) {
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

#define PrintPartitionUnit(hex_val, contents)                                                                          \
  if ((IGC_GET_FLAG_VALUE(PrintPartitionUnit) & hex_val) != 0) {                                                       \
    dbgs() << "PartitionUnit0x" << hex_val << ": " << contents << "\n";                                                \
  }
#define PrintControlUnitSize(hex_val, contents)                                                                        \
  if ((IGC_GET_FLAG_VALUE(PrintControlUnitSize) & hex_val) != 0) {                                                     \
    dbgs() << "ControlUnitSize0x" << hex_val << ": " << contents << "\n";                                              \
  }
#define PrintControlKernelTotalSize(hex_val, contents)                                                                 \
  if ((IGC_GET_FLAG_VALUE(PrintControlKernelTotalSize) & hex_val) != 0) {                                              \
    dbgs() << "ControlKernelTotalSize0x" << hex_val << ": " << contents << "\n";                                       \
  }
#define PrintTrimUnit(hex_val, contents)                                                                               \
  if ((IGC_GET_FLAG_VALUE(PrintControlKernelTotalSize) & hex_val) != 0 ||                                              \
      (IGC_GET_FLAG_VALUE(PrintControlUnitSize) & hex_val) != 0) {                                                     \
    dbgs() << "TrimUnit0x" << hex_val << ": " << contents << "\n";                                                     \
  }
#define PrintFunctionSizeAnalysis(hex_val, contents)                                                                   \
  if ((IGC_GET_FLAG_VALUE(PrintFunctionSizeAnalysis) & hex_val) != 0) {                                                \
    dbgs() << "FunctionSizeAnalysis0x" << hex_val << ": " << contents << "\n";                                         \
  }
#define PrintStaticProfileGuidedKernelSizeReduction(hex_val, contents)                                                 \
  if ((IGC_GET_FLAG_VALUE(PrintStaticProfileGuidedKernelSizeReduction) & hex_val) != 0) {                              \
    dbgs() << "StaticProfileGuidedKernelSizeReduction0x" << hex_val << ": " << contents << "\n";                       \
  }

static Scaled64 getSPGTWeight(unsigned Size, Scaled64 Freq, unsigned SizeWeightForSPGT,
                              unsigned FrequencyWeightForSPGT) {
  Scaled64 ScaledSize = Scaled64::get(Size);
  unsigned SizeWeight = SizeWeightForSPGT;
  Scaled64 WeightedSize = Scaled64::getOne();
  for (unsigned i = 0; i < SizeWeight; i++)
    WeightedSize *= ScaledSize;
  if (Freq == 0)
    return WeightedSize;
  unsigned FreqWeight = FrequencyWeightForSPGT;
  Scaled64 WeightedFreq = Scaled64::getOne();
  for (unsigned i = 0; i < FreqWeight; i++)
    WeightedFreq *= Freq;
  return WeightedSize / WeightedFreq;
}

typedef enum {
  SP_NO_METRIC = 0, /// \brief A flag to indicate whether no metric is used. We use this especially when we only need
                    /// static profile infomation without enforcement
  SP_NORMAL_DISTRIBUTION = (0x1 << 0x0), /// \brief A flag to indicate whether a normal distribution is used as metric
  SP_LONGTAIL_DISTRIBUTION =
      (0x1 << 0x1),                     /// \brief A flag to indicate whether a long tail distribution is used as metric
  SP_AVERAGE_PERCENTAGE = (0x1 << 0x2), /// \brief A flag to indicate whether average % is used as metric
} StatiProfile_FLAG_t;

// Function Attribute Flag type
typedef enum {
  FA_BEST_EFFORT_INLINE =
      0, /// \brief A flag to indicate whether it is to be inlined but it can be trimmed or assigned stackcall
  FA_FORCE_INLINE = (0x1 << 0x0), /// \brief A flag to indicate whether it is to be inlined and it cannot be reverted
  FA_TRIMMED = (0x1 << 0x1),      /// \brief A flag to indicate whetehr it will be trimmed
  FA_STACKCALL = (0x1 << 0x2),    /// \brief A flag to indicate whether this node should be a stack call header
  FA_KERNEL_ENTRY =
      (0x1
       << 0x3), /// \brief A flag to indicate whether this node is a kernel entry. It will be affected by any schemes.
  FA_ADDR_TAKEN = (0x1 << 0x4), /// \brief A flag to indicate whether this node is an address taken function.
} FA_FLAG_t;
/// Associate each function with a partially expanded size and remaining
/// unexpanded function list, etc.

typedef enum {
  FT_NOT_APPLICABLE = 0,             /// \brief A flag to indicate functions don't need to be considered
  FT_NOT_BEST_EFFORT = (0x1 << 0x1), /// \brief A flag to indicate function is not open to trimming or partitioning
  FT_MUL_KERNEL =
      (0x1 << 0x2), /// \brief A flag to indicate function is in multiple kernels and they are forced to be inlined
  FT_BIG_ENOUGH = (0x1 << 0x3),    /// \brief A flag to indicate functions are big enough to trim
  FT_TOO_TINY = (0x1 << 0x4),      /// \brief A flag to indicate function is too tiny to be trimmed
  FT_HIGHER_WEIGHT = (0x1 << 0x5), /// \brief a flag to indicate the function has higher weight than threshold
  FT_LOWER_WEIGHT = (0x1 << 0x6),  /// \brief a flag to indicate the function has lower weight than threshold
} FUNCTION_TRAIT_FLAG_t;
struct FunctionNode {
  FunctionNode(Function *F, std::size_t Size)
      : F(F), InitialSize(Size), UnitSize(Size), ExpandedSize(Size), SizeAfterCollapsing(Size), Inline_cnt(0),
        tmpSize(Size), CallingSubroutine(false), FunctionAttr(0), InMultipleUnit(false), HasImplicitArg(false),
        staticFuncFreq(0, 0), EntryFreq(0, 0) {}

  Function *F;

  /// leaf node.

  /// \brief Initial size before partition
  uint32_t InitialSize;

  //  \brief the size of a compilation unit
  uint32_t UnitSize;

  /// \brief Expanded size when all functions in a unit below the node are expanded
  uint32_t ExpandedSize;

  /// \brief Expanded size when all functions in a unit below the node are expanded
  uint32_t SizeAfterCollapsing;

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

  Scaled64 EntryFreq;
  std::unordered_map<llvm::BasicBlock *, Scaled64> blockFreqs;

  /// \brief All functions directly called in this function.
  std::unordered_map<FunctionNode *, uint16_t> CalleeList;

  /// \brief All functions that call this function F.
  std::unordered_map<FunctionNode *, uint16_t> CallerList;

  bool EnableLeafCollapsing;
  bool EnableSizeContributionOptimization;
  bool StaticProfileGuidedTrimming;
  bool UseFrequencyInfoForSPGT;
  bool ForceInlineExternalFunctions;
  unsigned ControlInlineTinySize;
  bool ForceInlineStackCallWithImplArg;
  bool ControlInlineImplicitArgs;
  unsigned SizeWeightForSPGT;
  unsigned FrequencyWeightForSPGT;

  void setFlags(bool EnableLC, bool EnableSCO, bool SPGT, bool UseFreqInfo, bool ForceInlineExtFun, unsigned TinySize,
                bool InlineStkCallWithImplArg, bool InlineImplArgs, unsigned SizeWeight, unsigned FreqWeight) {
    EnableLeafCollapsing = EnableLC;
    EnableSizeContributionOptimization = EnableSCO;
    StaticProfileGuidedTrimming = SPGT;
    UseFrequencyInfoForSPGT = UseFreqInfo;
    ForceInlineExternalFunctions = ForceInlineExtFun;
    ControlInlineTinySize = TinySize;
    ForceInlineStackCallWithImplArg = InlineStkCallWithImplArg;
    ControlInlineImplicitArgs = InlineImplArgs;
    SizeWeightForSPGT = SizeWeight;
    FrequencyWeightForSPGT = FreqWeight;
    return;
  }

  void setStaticFuncFreq(Scaled64 freq) { staticFuncFreq = freq; }

  Scaled64 getStaticFuncFreq() { return staticFuncFreq; }

  std::string getStaticFuncFreqStr() { return staticFuncFreq.toString(); }

  // \brief return the size used for Static Profile Guided Trimming
  uint64_t getPotentialBodySize() { return EnableLeafCollapsing ? SizeAfterCollapsing : InitialSize; }

  uint64_t getSizeContribution() {
    return Inline_cnt == 0 ? getPotentialBodySize() : static_cast<uint64_t>(Inline_cnt) * getPotentialBodySize();
  }

  uint64_t getSizeForTrimming() {
    return EnableSizeContributionOptimization ? getSizeContribution() : getPotentialBodySize();
  }

  Scaled64 getWeightForTrimming() {
    if (StaticProfileGuidedTrimming && UseFrequencyInfoForSPGT) {
      return getSPGTWeight(getSizeForTrimming(), staticFuncFreq, SizeWeightForSPGT, FrequencyWeightForSPGT);
    }
    return Scaled64::get(getSizeForTrimming());
  }

  /// \brief A node becomes a leaf when all called functions are expanded.
  bool isLeaf() const { return CalleeList.empty(); }

  /// \brief Add a caller or callee.
  // A caller may call the same callee multiple times, e.g. A->{B,B,B}: A->CalleeList(B,B,B), B->CallerList(A,A,A)
  void addCallee(FunctionNode *G, unsigned weight) {
    IGC_ASSERT(G);
    if (CalleeList.find(G) == CalleeList.end()) // First time added, Initialize it
      CalleeList[G] = 0;
    CalleeList[G] += weight;
    CallingSubroutine = true;
  }
  void addCaller(FunctionNode *G, unsigned weight) {
    IGC_ASSERT(G);
    if (CallerList.find(G) == CallerList.end()) // First time added, Initialize it
      CallerList[G] = 0;
    CallerList[G] += weight;
  }

  void setKernelEntry() {
    FunctionAttr = FA_KERNEL_ENTRY;
    return;
  }
  void setAddressTaken() { FunctionAttr = FA_ADDR_TAKEN; }
  void setForceInline() {
    IGC_ASSERT(FunctionAttr != FA_KERNEL_ENTRY &&
               FunctionAttr != FA_ADDR_TAKEN); // Can't force inline a kernel entry or address taken function
    FunctionAttr = FA_FORCE_INLINE;
    return;
  }
  void setTrimmed() {
    IGC_ASSERT(FunctionAttr == FA_BEST_EFFORT_INLINE); // Only best effort inline function can be trimmed
    FunctionAttr = FA_TRIMMED;
    return;
  }
  void unsetTrimmed() {
    IGC_ASSERT(FunctionAttr == FA_TRIMMED); // Only best effort inline function can be trimmed
    FunctionAttr = FA_BEST_EFFORT_INLINE;
    return;
  }

  void setStackCall() {
    // Can't assign stack call to force inlined function, kernel entry,
    // address taken functions and functions that already assigned stack call
    IGC_ASSERT(FunctionAttr == FA_BEST_EFFORT_INLINE || FunctionAttr == FA_TRIMMED);
    FunctionAttr = FA_STACKCALL;
    return;
  }

  void setEntryFrequency(uint64_t digit, uint16_t scale) { EntryFreq = Scaled64(digit, scale); }
  Scaled64 getEntryFrequency() { return EntryFreq; }

  bool isEntryFunc() { return FunctionAttr == FA_KERNEL_ENTRY; }
  bool isAddrTakenFunc() { return FunctionAttr == FA_ADDR_TAKEN; }
  bool isTrimmed() { return FunctionAttr == FA_TRIMMED; }
  bool isForcedInlined() { return FunctionAttr == FA_FORCE_INLINE; }
  bool isBestEffortInline() { return FunctionAttr == FA_BEST_EFFORT_INLINE; }
  bool hasNoCaller() { return isAddrTakenFunc() || isEntryFunc(); }
  bool willBeInlined() { return isBestEffortInline() || isForcedInlined(); }
  bool isStackCallAssigned() { return FunctionAttr == FA_STACKCALL; }
  bool canAssignStackCall() {
    if (FA_BEST_EFFORT_INLINE == FunctionAttr ||
        FA_TRIMMED == FunctionAttr) // The best effort inline or manually trimmed functions can be assigned stack call
      return true;
    return false;
  }

  uint16_t getFunctionTrait(Scaled64 thresholdForTrimming) {
    if (FunctionAttr != FA_BEST_EFFORT_INLINE) // Only best effort inline can be trimmed
      return FT_NOT_BEST_EFFORT;
    // to allow trimming functions called from other kernels, set the regkey to false
    if (ForceInlineExternalFunctions && InMultipleUnit)
      return FT_MUL_KERNEL;

    uint64_t tinySize = ControlInlineTinySize;

    if (getPotentialBodySize() < tinySize) // It's too small to trim
      return FT_TOO_TINY;

    if (StaticProfileGuidedTrimming) {
      if (getWeightForTrimming() < thresholdForTrimming) {
        return FT_LOWER_WEIGHT;
      } else {
        return FT_HIGHER_WEIGHT;
      }
    }

    return FT_BIG_ENOUGH;
  }

  std::string getFuncAttrStr() {
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

  void dumpFuncInfo(uint16_t type, std::string message) {
    std::string dumpInfo = message + ", ";
    dumpInfo += F->getName().str();
    dumpInfo += ", Function Attribute: ";
    dumpInfo += getFuncAttrStr();
    dumpInfo += ", Function size: ";
    dumpInfo += std::to_string(InitialSize);
    if (EnableLeafCollapsing) {
      dumpInfo += ", Size after collapsing: ";
      dumpInfo += std::to_string(SizeAfterCollapsing);
    }
    if (EnableSizeContributionOptimization) {
      dumpInfo += ", Size contribution: ";
      dumpInfo += std::to_string(getSizeContribution());
    }
    if (UseFrequencyInfoForSPGT) {
      dumpInfo += ", Freq: ";
      dumpInfo += getStaticFuncFreqStr();
    }
    if (StaticProfileGuidedTrimming) {
      dumpInfo += ", Weight: ";
      dumpInfo += getWeightForTrimming().toString();
    }
    PrintTrimUnit(type, dumpInfo);
  }

  // Top down bfs to find the size of a compilation unit
  uint32_t updateUnitSize() {
    std::unordered_set<FunctionNode *> visit;
    std::deque<FunctionNode *> TopDownQueue;
    TopDownQueue.push_back(this);
    visit.insert(this);
    uint32_t total = 0;
    PrintFunctionSizeAnalysis(0x4, "Functions in the unit " << F->getName().str()) while (!TopDownQueue.empty()) {
      FunctionNode *Node = TopDownQueue.front();
      PrintFunctionSizeAnalysis(0x4, Node->F->getName().str() << ": " << Node->InitialSize) TopDownQueue.pop_front();
      total += Node->InitialSize;
      for (auto &Callee : Node->CalleeList) {
        FunctionNode *calleeNode = Callee.first;
        if (visit.find(calleeNode) != visit.end() ||
            calleeNode->isStackCallAssigned()) // Already processed or head of stack call
          continue;
        visit.insert(calleeNode);
        TopDownQueue.push_back(calleeNode);
      }
    }
    return UnitSize = total;
  }

  /// \brief A single step to expand F
  void expand(FunctionNode *callee) {
    // When the collaped callee has implicit arguments
    // the node will have implicit arguments too
    // In this scenario, when ControlInlineImplicitArgs is set
    // the node should be inlined unconditioinally so exempt from a stackcall and trimming target
    if (HasImplicitArg == false && callee->HasImplicitArg == true) {
      HasImplicitArg = true;
      PrintFunctionSizeAnalysis(0x4, "Func " << this->F->getName().str() << " expands to has implicit arg due to "
                                             << callee->F->getName().str())

          if (!hasNoCaller()) // Can't inline kernel entry or address taken functions
      {
        if (isStackCallAssigned()) { // When stackcall is assigned we need to determine based on the flag
          if (ForceInlineStackCallWithImplArg)
            setForceInline();
        } else if (ControlInlineImplicitArgs) { // Force inline ordinary functions with implicit arguments
          setForceInline();
        }
      }
    }
    uint32_t sizeIncrease = callee->ExpandedSize * CalleeList[callee];
    tmpSize += sizeIncrease;
  }
#if defined(_DEBUG)
  void print(raw_ostream &os);

  void dump() { print(llvm::errs()); }
#endif
};

} // namespace
#if defined(_DEBUG)

void FunctionNode::print(raw_ostream &os) {
  os << "Function: " << F->getName() << ", " << InitialSize << "\n";
  for (const auto &G : CalleeList)
    os << "--->>>" << G.first->F->getName() << "\n";
  for (const auto &G : CallerList)
    os << "<<<---" << G.first->F->getName() << "\n";
}
#endif

void EstimateFunctionSize::clear() {
  M = nullptr;
  for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I) {
    auto Node = (FunctionNode *)I->second;
    delete Node;
  }
  ECG.clear();
  kernelEntries.clear();
  stackCallFuncs.clear();
  addressTakenFuncs.clear();
}

bool EstimateFunctionSize::matchImplicitArg(CallInst &CI) {
  bool matched = false;
  StringRef funcName = CI.getCalledFunction()->getName();
  if (funcName.equals(GET_LOCAL_ID_X) || funcName.equals(GET_LOCAL_ID_Y) || funcName.equals(GET_LOCAL_ID_Z)) {
    matched = true;
  } else if (funcName.equals(GET_GROUP_ID)) {
    matched = true;
  } else if (funcName.equals(GET_LOCAL_THREAD_ID)) {
    matched = true;
  } else if (funcName.equals(GET_GLOBAL_OFFSET)) {
    matched = true;
  } else if (funcName.equals(GET_GLOBAL_SIZE)) {
    matched = true;
  } else if (funcName.equals(GET_LOCAL_SIZE)) {
    matched = true;
  } else if (funcName.equals(GET_WORK_DIM)) {
    matched = true;
  } else if (funcName.equals(GET_NUM_GROUPS)) {
    matched = true;
  } else if (funcName.equals(GET_ENQUEUED_LOCAL_SIZE)) {
    matched = true;
  } else if (funcName.equals(GET_STAGE_IN_GRID_ORIGIN)) {
    matched = true;
  } else if (funcName.equals(GET_STAGE_IN_GRID_SIZE)) {
    matched = true;
  } else if (funcName.equals(GET_SYNC_BUFFER)) {
    matched = true;
  } else if (funcName.equals(GET_ASSERT_BUFFER)) {
    matched = true;
  } else if (funcName.equals(GET_REGION_GROUP_SIZE)) {
    matched = true;
  } else if (funcName.equals(GET_REGION_GROUP_WG_COUNT)) {
    matched = true;
  } else if (funcName.equals(GET_REGION_GROUP_BARRIER_BUFFER)) {
    matched = true;
  }

  if (matched && (IGC_GET_FLAG_VALUE(PrintControlKernelTotalSize) & 0x40) != 0) {
    PrintFunctionSizeAnalysis(0x8, "Matched implicit arg " << funcName.str())
  }
  return matched;
}

// visit Call inst to determine if implicit args are used by the caller
void EstimateFunctionSize::visitCallInst(CallInst &CI) {
  if (!CI.getCalledFunction()) {
    return;
  }
  // Check for implicit arg function calls
  bool matched = matchImplicitArg(CI);
  tmpHasImplicitArg = matched;
}

void EstimateFunctionSize::updateStaticFuncFreq() {
  DenseMap<Function *, ScaledNumber<uint64_t>> Counts;
  auto MayHaveIndirectCalls = [](Function &F) {
    for (auto *U : F.users()) {
      if (!isa<CallInst>(U) && !isa<InvokeInst>(U))
        return true;
    }
    return false;
  };
  uint64_t InitialSyntheticCount = 10;
  uint64_t InlineSyntheticCount = 15;
  uint64_t ColdSyntheticCount = 5;
  for (Function &F : *M) {
    uint64_t InitialCount = InitialSyntheticCount;
    if (F.empty() || F.isDeclaration())
      continue;
    if (F.hasFnAttribute(llvm::Attribute::AlwaysInline) || F.hasFnAttribute(llvm::Attribute::InlineHint)) {
      // Use a higher value for inline functions to account for the fact that
      // these are usually beneficial to inline.
      InitialCount = InlineSyntheticCount;
    } else if (F.hasLocalLinkage() && !MayHaveIndirectCalls(F)) {
      // Local functions without inline hints get counts only through
      // propagation.
      InitialCount = 0;
    } else if (F.hasFnAttribute(llvm::Attribute::Cold) || F.hasFnAttribute(llvm::Attribute::NoInline)) {
      // Use a lower value for noinline and cold functions.
      InitialCount = ColdSyntheticCount;
    }
    Counts[&F] = Scaled64(InitialCount, 0);
  }
  // Edge includes information about the source. Hence ignore the first
  // parameter.
  auto GetCallSiteProfCount = [&](const CallGraphNode *, const CallGraphNode::CallRecord &Edge) {
    std::optional<Scaled64> Res = std::nullopt;
    if (!Edge.first)
      return IGCLLVM::makeLLVMOptional(Res);
    CallBase &CB = *cast<CallBase>(*Edge.first);
    Function *Caller = CB.getCaller();
    BasicBlock *CSBB = CB.getParent();
    // Now compute the callsite count from relative frequency and
    // entry count:
    Scaled64 EntryFreq = get<FunctionNode>(Caller)->getEntryFrequency();
    Scaled64 BBCount = get<FunctionNode>(Caller)->blockFreqs[CSBB];
    IGC_ASSERT(EntryFreq != 0);
    BBCount /= EntryFreq;
    BBCount *= Counts[Caller];
    return IGCLLVM::makeLLVMOptional(std::optional<Scaled64>(BBCount));
  };
  CallGraph CG(*M);
  // Propgate the entry counts on the callgraph.
  SyntheticCountsUtils<const CallGraph *>::propagate(&CG, GetCallSiteProfCount,
                                                     [&](const CallGraphNode *N, Scaled64 New) {
                                                       auto F = N->getFunction();
                                                       if (!F || F->isDeclaration())
                                                         return;
                                                       Counts[F] += New;
                                                     });

  for (auto &F : M->getFunctionList()) {
    if (F.empty())
      continue;
    FunctionNode *Node = get<FunctionNode>(&F);

    if (Counts.find(&F) != Counts.end())
      Node->setStaticFuncFreq(Counts[&F]);
  }
  return;
}

void EstimateFunctionSize::runStaticAnalysis() {
  // Analyze function frequencies from SyntheticCountsPropagation
  PrintStaticProfileGuidedKernelSizeReduction(
      0x1, "------------------Static analysis start------------------") for (auto &F : M->getFunctionList()) {
    if (F.empty())
      continue;
    auto &BFI = getAnalysis<BlockFrequencyInfoWrapperPass>(F).getBFI();
    FunctionNode *Node = get<FunctionNode>(&F);
    Node->setEntryFrequency(BFI.getEntryFreq(), 0);

    for (auto &B : F)
      Node->blockFreqs[&B] = Scaled64(BFI.getBlockFreq(&B).getFrequency(), 0);
  }
  updateStaticFuncFreq();
  std::vector<Scaled64> freqLog;
  if (BlockFrequencySampling) { // Set basic blocks as the sample space
    for (auto &F : M->getFunctionList()) {
      if (F.empty())
        continue;
      FunctionNode *Node = get<FunctionNode>(&F);
      Scaled64 EntryFreq = Node->getEntryFrequency();
      PrintStaticProfileGuidedKernelSizeReduction(0x1, "Function frequency of "
                                                           << Node->F->getName().str() << ": "
                                                           << Node->getStaticFuncFreqStr()) for (auto &B : F) {
        Scaled64 BBCount = Node->blockFreqs[&B];
        BBCount /= EntryFreq;
        BBCount *= Node->getStaticFuncFreq();
        PrintStaticProfileGuidedKernelSizeReduction(0x1, "Block frequency of " << B.getName().str() << ": "
                                                                               << BBCount.toString())

            if (BBCount > 0) // Can't represent 0 in log scale so ignore, better idea?
            freqLog.push_back(BBCount);
      }
    }
  } else {
    for (auto &F : M->getFunctionList()) {
      if (F.empty())
        continue;
      FunctionNode *Node = get<FunctionNode>(&F);
      PrintStaticProfileGuidedKernelSizeReduction(
          0x1, "Function frequency of "
                   << Node->F->getName().str() << ": "
                   << Node->getStaticFuncFreqStr()) if (Node->getStaticFuncFreq() >
                                                        0) // Can't represent 0 in log scale so ignore, better idea?
          freqLog.push_back(Node->getStaticFuncFreq());
    }
  }

  if ((MetricForKernelSizeReduction & SP_NORMAL_DISTRIBUTION) != 0 &&
      !freqLog.empty()) { // When using a normal distribution. Ignore when there are no frequency data
    IGC_ASSERT(ParameterForColdFuncThreshold >= 0 && ParameterForColdFuncThreshold <= 30);
    // Find a threshold from a normal distribution
    std::sort(freqLog.begin(), freqLog.end()); // Sort frequency data
    std::vector<double> freqLogDbl;
    std::unordered_map<double, Scaled64> map_log10_to_scaled64;
    double log10_2 = std::log10(2);
    for (Scaled64 &val : freqLog) // transform into log10 scale
    {
      double logedVal = std::log10(val.getDigits()) + val.getScale() * log10_2;
      map_log10_to_scaled64[logedVal] = val;
      freqLogDbl.push_back(logedVal);
    }
    double sum_val = std::accumulate(freqLogDbl.begin(), freqLogDbl.end(), 0.0);
    double mean = sum_val / freqLogDbl.size();
    double sq_sum = std::inner_product(
        freqLogDbl.begin(), freqLogDbl.end(), freqLogDbl.begin(), 0.0,
        [](double const &x, double const &y) { return x + y; },
        [mean](double const &x, double const &y) { return (x - mean) * (y - mean); });
    double standard_deviation = std::sqrt(sq_sum / freqLogDbl.size());
    float C = (float)ParameterForColdFuncThreshold /
              10; // Since 1 STD is too wide in the majority case, we need to scale down
    double threshold_log10 = mean - C * standard_deviation;
    auto it_lower = std::lower_bound(freqLogDbl.begin(), freqLogDbl.end(), threshold_log10);
    if (it_lower == freqLogDbl.end())
      threshold_func_freq = freqLog.back();
    else
      threshold_func_freq = map_log10_to_scaled64[*it_lower];
    PrintStaticProfileGuidedKernelSizeReduction(0x1, "Metric: Normal distribution");
    PrintStaticProfileGuidedKernelSizeReduction(0x1, "Sample count: " << freqLogDbl.size());
    PrintStaticProfileGuidedKernelSizeReduction(0x1, "Execution frequency mean (Log10 scale): " << mean);
    PrintStaticProfileGuidedKernelSizeReduction(0x1, "Standard deviation (Log10 scale): " << standard_deviation);
    PrintStaticProfileGuidedKernelSizeReduction(0x1, "Execution frequency threshold with Constant(C) "
                                                         << C << ": " << threshold_func_freq.toString());
  } else if ((MetricForKernelSizeReduction & SP_LONGTAIL_DISTRIBUTION) != 0 &&
             !freqLog.empty()) { // When using a long-tail distribution. Ignore when there are no frequency data
    IGC_ASSERT(ParameterForColdFuncThreshold > 0 && ParameterForColdFuncThreshold <= 100);
    // Find a threshold from a long tail distribution
    uint32_t threshold_cold = (uint32_t)ParameterForColdFuncThreshold;
    uint32_t C_pos = freqLog.size() * threshold_cold / 100;
    std::nth_element(freqLog.begin(), freqLog.begin() + C_pos, freqLog.end(),
                     [](Scaled64 &x, Scaled64 &y) { return x < y; }); // Low C%
    threshold_func_freq = freqLog[C_pos];
    PrintStaticProfileGuidedKernelSizeReduction(0x1, "Metric: Long tail distribution");
    PrintStaticProfileGuidedKernelSizeReduction(0x1, "Low " << threshold_cold << "% pos: " << C_pos << " out of "
                                                            << freqLog.size());
    PrintStaticProfileGuidedKernelSizeReduction(0x1, "Execution frequency threshold: " << threshold_func_freq);
  } else if ((MetricForKernelSizeReduction & SP_AVERAGE_PERCENTAGE) != 0 &&
             !freqLog.empty()) { // When using a average C%
    Scaled64 sum_val = std::accumulate(freqLog.begin(), freqLog.end(), Scaled64::getZero());
    Scaled64 mean = sum_val / Scaled64::get(freqLog.size());
    Scaled64 C = Scaled64::get(ParameterForColdFuncThreshold) / Scaled64::get(10); // Scale down /10
    IGC_ASSERT(C > 0 && C <= 100);
    threshold_func_freq = mean * (C / Scaled64::get(100));
    PrintStaticProfileGuidedKernelSizeReduction(0x1, "Metric: Average%");
    PrintStaticProfileGuidedKernelSizeReduction(0x1, "Average threshold * " << C.toString()
                                                                            << "%: " << threshold_func_freq.toString());
  }

  unsigned sizeThreshold = ControlInlineTinySizeForSPGT;
  if (UseFrequencyInfoForSPGT) {
    thresholdForTrimming = getSPGTWeight(sizeThreshold, threshold_func_freq, SizeWeightForSPGT, FrequencyWeightForSPGT);
  } else {
    thresholdForTrimming = Scaled64::get(sizeThreshold); // If we don't want to use freq data,
                                                         // just use size only
  }

  PrintStaticProfileGuidedKernelSizeReduction(0x1, "------------------Static analysis end------------------\n") return;
}

void EstimateFunctionSize::estimateTotalLoopIteration(llvm::Function &F, LoopInfo *LI) {
  auto &SE = getAnalysis<ScalarEvolutionWrapperPass>(F).getSE();
  for (Loop *L : LI->getLoopsInPreorder()) {
    Scaled64 ParentLCnt = Scaled64::getOne();
    Loop *ParentL = L->getParentLoop();
    if (ParentL) {
      IGC_ASSERT(LoopIterCnts.find(ParentL) != LoopIterCnts.end());
      ParentLCnt = LoopIterCnts[ParentL];
    }
    StringRef LoopCntAttr = " Back edge count not available";
    if (SE.hasLoopInvariantBackedgeTakenCount(L)) {
      unsigned TripCount = 0;
      SmallVector<BasicBlock *, 8> ExitingBlocks;
      L->getExitingBlocks(ExitingBlocks);
      for (BasicBlock *ExitingBlock : ExitingBlocks)
        if (unsigned TC = SE.getSmallConstantTripCount(L, ExitingBlock))
          if (!TripCount || TC < TripCount)
            TripCount = TC;
      if (TripCount) {
        // We assume that loop unrolling will not exceed 16 times
        unsigned MaxUnrollCount = MaxUnrollCountForFunctionSizeAnalysis;
        TripCount = std::min(TripCount, MaxUnrollCount);
        LoopIterCnts[L] = ParentLCnt * Scaled64::get(TripCount);
        LoopCntAttr = " Trip count available";
      } else {
        // TODO: We currently set a loop count to 5
        // if we don't know the exact number
        LoopIterCnts[L] = ParentLCnt * Scaled64::get(5);
        LoopCntAttr = " Upper bound available";
      }
    } else {
      LoopIterCnts[L] = Scaled64::getOne();
    }
    PrintFunctionSizeAnalysis(0x2, "Loop " << L->getName().str() << ": Loop Count = " << LoopIterCnts[L].toString()
                                           << ", Parent Loop Count = " << ParentLCnt.toString() << LoopCntAttr)
  }
  return;
}

void EstimateFunctionSize::analyze() {
  auto getSize = [&](llvm::Function &F) {
    std::size_t Size = 0;
    for (auto &BB : F) {
      std::size_t BlkSize = IGCLLVM::sizeWithoutDebug(&BB);
      Size += BlkSize;
    }
    return Size;
  };

  auto getSizeWithLoopCnt = [&](llvm::Function &F, LoopInfo &LI) {
    std::size_t Size = 0;
    for (auto &BB : F) {
      std::size_t BlkSize = IGCLLVM::sizeWithoutDebug(&BB);
      Loop *L = LI.getLoopFor(&BB);
      if (L) {
        BlkSize = BlkSize * LoopIterCnts[L].toInt<size_t>();
      }
      Size += BlkSize;
    }
    return Size;
  };

  auto MdWrapper = getAnalysisIfAvailable<MetaDataUtilsWrapper>();
  auto pMdUtils = MdWrapper->getMetaDataUtils();
  // Initialize the data structure. find all noinline and stackcall properties
  for (auto &F : M->getFunctionList()) {
    if (F.empty())
      continue;
    FunctionNode *node = nullptr;
    if (LoopCountAwareTrimming) {
      auto &LI = getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();
      estimateTotalLoopIteration(F, &LI);
      size_t FuncSize = getSize(F);
      size_t FuncSizeWithLoopCnt = getSizeWithLoopCnt(F, LI);
      node = new FunctionNode(&F, FuncSizeWithLoopCnt);
      PrintFunctionSizeAnalysis(0x1, "Function " << F.getName().str() << " Original Size: " << FuncSize
                                                 << " Size with Loop Iter: " << FuncSizeWithLoopCnt);
    } else {
      node = new FunctionNode(&F, getSize(F));
    }
    node->setFlags(EnableLeafCollapsing, EnableSizeContributionOptimization, StaticProfileGuidedTrimming,
                   UseFrequencyInfoForSPGT, ForceInlineExternalFunctions, ControlInlineTinySize,
                   ForceInlineStackCallWithImplArg, ControlInlineImplicitArgs, SizeWeightForSPGT,
                   FrequencyWeightForSPGT);
    bool isForceTrim = false;
    if (!SelectiveTrimming.empty()) {
      std::string functionToTrim = SelectiveTrimming;
      if (F.getName().str() == functionToTrim) {
        isForceTrim = true;
        PrintFunctionSizeAnalysis(0x1, "Force trimming (No inline) " << functionToTrim);
      }
    }
    ECG[&F] = node;
    if (isEntryFunc(pMdUtils, node->F)) { /// Entry function
      node->setKernelEntry();
      kernelEntries.push_back(node);
    } else if (F.hasFnAttribute("igc-force-stackcall")) {
      node->setStackCall();
    } else if (F.hasFnAttribute(llvm::Attribute::NoInline) || isForceTrim) {
      node->setTrimmed();
    } else if (F.hasFnAttribute(llvm::Attribute::AlwaysInline)) {
      node->setForceInline();
    }
    // Otherwise, the function attribute to be assigned is best effort
  }

  // Visit all call instructions and populate CG.
  for (auto &F : M->getFunctionList()) {
    if (F.empty())
      continue;
    FunctionNode *Node = get<FunctionNode>(&F);
    auto &LI = getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();
    for (auto U : F.users()) {
      // Other users (like bitcast/store) are ignored.
      if (auto *CI = dyn_cast<CallInst>(U)) {
        // G calls F, or G --> F
        BasicBlock *BB = CI->getParent();
        Function *G = BB->getParent();
        FunctionNode *GN = get<FunctionNode>(G);
        unsigned LoopCnt = 1;
        if (LoopCountAwareTrimming) {
          Loop *L = LI.getLoopFor(BB);
          if (L) {
            IGC_ASSERT(LoopIterCnts.find(L) != LoopIterCnts.end());
            LoopCnt = LoopIterCnts[L].toInt<size_t>();
          }
        }
        GN->addCallee(Node, LoopCnt);
        Node->addCaller(GN, LoopCnt);
      }
    }
  }

  // Find all address taken functions
  for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I) {
    FunctionNode *Node = (FunctionNode *)I->second;
    // Address taken functions neither have callers nor is an entry function
    if (Node->CallerList.empty() && !Node->isEntryFunc())
      Node->setAddressTaken();
  }

  bool needImplAnalysis = ControlInlineImplicitArgs || ForceInlineStackCallWithImplArg;
  // check functions and mark those that use implicit args.
  PrintFunctionSizeAnalysis(0x1, "--------------------------Function size analysis start--------------------------");
  if (needImplAnalysis)
    performImplArgsAnalysis();

  // Update expanded and static unit size and propagate implicit argument information which might cancel some stackcalls
  for (void *entry : kernelEntries) {
    FunctionNode *kernelEntry = (FunctionNode *)entry;
    updateExpandedUnitSize(kernelEntry->F, true);
    kernelEntry->updateUnitSize();
    PrintFunctionSizeAnalysis(0x1, "Unit size (kernel entry) " << kernelEntry->F->getName().str() << ": "
                                                               << kernelEntry->UnitSize);
    PrintFunctionSizeAnalysis(0x1, "Expanded unit size (kernel entry) " << kernelEntry->F->getName().str() << ": "
                                                                        << kernelEntry->ExpandedSize);
  }

  // Find all survived stackcalls and address taken functions and update unit sizes
  for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I) {
    FunctionNode *Node = (FunctionNode *)I->second;
    if (Node->isStackCallAssigned()) {
      stackCallFuncs.push_back(Node);
      Node->updateUnitSize();
      PrintFunctionSizeAnalysis(0x1, "Unit size (stack call) " << Node->F->getName().str() << ": " << Node->UnitSize);
    } else if (Node->isAddrTakenFunc()) {
      addressTakenFuncs.push_back(Node);
      updateExpandedUnitSize(Node->F, true);
      Node->updateUnitSize();
      PrintFunctionSizeAnalysis(0x1,
                                "Unit size (address taken) " << Node->F->getName().str() << ": " << Node->UnitSize);
      PrintFunctionSizeAnalysis(0x1, "Expanded unit size (address taken) " << Node->F->getName().str() << ": "
                                                                           << Node->ExpandedSize);
    }
  }
  PrintFunctionSizeAnalysis(0x1, "Function count= " << ECG.size());
  PrintFunctionSizeAnalysis(0x1, "Kernel count= " << kernelEntries.size());
  PrintFunctionSizeAnalysis(0x1, "Manual stack call count= " << stackCallFuncs.size());
  PrintFunctionSizeAnalysis(0x1, "Address taken function call count= " << addressTakenFuncs.size());
  PrintFunctionSizeAnalysis(0x1, "--------------------------Function size analysis end--------------------------\n");
  return;
}

void EstimateFunctionSize::performImplArgsAnalysis() {
  for (auto I = ECG.begin(), E = ECG.end(); I != E; ++I) {
    FunctionNode *Node = (FunctionNode *)I->second;
    IGC_ASSERT(Node);
    tmpHasImplicitArg = false;
    visit(Node->F);
    if (!tmpHasImplicitArg) // The function doesn't have an implicit argument: skip
      continue;
    Node->HasImplicitArg = true;
    static int cnt = 0;
    const char *Name;
    if (Node->isLeaf()) {
      Name = "Leaf";
    } else {
      Name = "nonLeaf";
    }
    PrintFunctionSizeAnalysis(0x8, Name << " Func " << ++cnt << " " << Node->F->getName().str()
                                        << " calls implicit args so HasImplicitArg")

        if (Node->hasNoCaller()) // Can't inline kernel entry or address taken functions
        continue;

    if (Node->isStackCallAssigned()) // When stackcall is assigned we need to determine based on the flag
    {
      if (ForceInlineStackCallWithImplArg)
        Node->setForceInline();
      continue;
    }

    // For other cases
    if (ControlInlineImplicitArgs) // Force inline ordinary functions with implicit arguments
      Node->setForceInline();
  }
  return;
}

/// \brief Return the estimated maximal function size after complete inlining.
std::size_t EstimateFunctionSize::getMaxExpandedSize() const {
  uint32_t MaxSize = 0;
  for (auto I : kernelEntries) {
    FunctionNode *Node = (FunctionNode *)I;
    MaxSize = std::max(MaxSize, Node->ExpandedSize);
  }
  for (auto I : addressTakenFuncs) {
    FunctionNode *Node = (FunctionNode *)I;
    MaxSize = std::max(MaxSize, Node->ExpandedSize);
  }
  return MaxSize;
}

void EstimateFunctionSize::checkSubroutine() {
  auto CGW = getAnalysisIfAvailable<CodeGenContextWrapper>();
  if (!CGW)
    return;

  EnableSubroutine = true;
  CodeGenContext *pContext = CGW->getCodeGenContext();
  if (pContext->type != ShaderType::OPENCL_SHADER && pContext->type != ShaderType::COMPUTE_SHADER &&
      pContext->type != ShaderType::RAYTRACING_SHADER)
    EnableSubroutine = false;

  if (EnableSubroutine) {
    uint32_t subroutineThreshold = SubroutineThreshold;
    uint32_t expandedMaxSize = getMaxExpandedSize();

    if (AL != AL_Module) // at the second call of EstimationFucntionSize, halve the threshold
      subroutineThreshold = subroutineThreshold >> 1;

    if (expandedMaxSize <= subroutineThreshold) {
      PrintTrimUnit(0x1, "No need to reduce the kernel size. (The max expanded kernel size is small) "
                             << expandedMaxSize << " < " << subroutineThreshold) if (!HasRecursion) EnableSubroutine =
          false;
    } else if (AL == AL_Module &&
               IGC_IS_FLAG_DISABLED(DisableAddingAlwaysAttribute)) { // kernel trimming and partitioning only kick in at
                                                                     // the first EstimationFunctionSize
      // Analyze Function/Block frequencies

      if (StaticProfileGuidedPartitioning ||
          StaticProfileGuidedTrimming) // Either a normal or long-tail distribution is enabled
        runStaticAnalysis();

      // If the max unit size exceeds threshold, do partitioning
      if (PartitionUnit) {
        PrintPartitionUnit(0x1, "--------------------------Partition unit start--------------------------");
        uint32_t unitThreshold = UnitSizeThreshold;
        uint32_t maxUnitSize = getMaxUnitSize();
        if (maxUnitSize > unitThreshold) {
          PrintPartitionUnit(0x1, "Max unit size " << maxUnitSize << " is larger than the threshold (to partition) "
                                                   << unitThreshold) partitionKernel();
        } else {
          PrintPartitionUnit(0x1, "Max unit size " << maxUnitSize
                                                   << " is smaller than the threshold (No partitioning needed) "
                                                   << unitThreshold)
        }
        PrintPartitionUnit(0x1, "--------------------------Partition unit end--------------------------\n");
      }

      PrintTrimUnit(0x1, "Need to reduce the kernel size. (The max expanded kernel size is large) "
                             << expandedMaxSize << " > " << subroutineThreshold)
          PrintTrimUnit(
              0x1,
              "-----------------------------Trimming start-----------------------------") if (ControlKernelTotalSize) {
        reduceKernelSize();
      }
      else if (ControlUnitSize) {
        reduceCompilationUnitSize();
      }
      PrintTrimUnit(0x1, "-----------------------------Trimming end-----------------------------\n")
    }
  }
  IGC_ASSERT(!HasRecursion || EnableSubroutine);
  return;
}

std::size_t EstimateFunctionSize::getExpandedSize(const Function *F) const {
  // IGC_ASSERT(IGC_IS_FLAG_DISABLED(ControlKernelTotalSize));
  auto I = ECG.find((Function *)F);
  if (I != ECG.end()) {
    FunctionNode *Node = (FunctionNode *)I->second;
    IGC_ASSERT(F == Node->F);
    return Node->ExpandedSize;
  }
  return std::numeric_limits<std::size_t>::max();
}

bool EstimateFunctionSize::onlyCalledOnce(const Function *F, const Function *CallerF) {
  // IGC_ASSERT(IGC_IS_FLAG_DISABLED(ControlKernelTotalSize));
  auto I = ECG.find((Function *)F);
  if (I != ECG.end()) {
    auto *Node = (FunctionNode *)I->second;
    IGC_ASSERT(F == Node->F);
    // one call-site and not a recursion
    if (Node->CallerList.size() == 1 && Node->CallerList.begin()->second == 1 &&
        Node->CallerList.begin()->first != Node) {
      return true;
    }
    // OpenCL specific, called once by passed kernel
    auto *MdWrapper = getAnalysisIfAvailable<MetaDataUtilsWrapper>();
    if (MdWrapper) {
      auto *pMdUtils = MdWrapper->getMetaDataUtils();
      for (const auto &[Caller, CallCount] : Node->CallerList) {
        if (CallCount > 1 && Caller->F == CallerF) {
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
  uint32_t threshold = KernelTotalSizeThreshold;
  llvm::SmallVector<void *, 64> unitHeads;
  for (auto node : kernelEntries)
    unitHeads.push_back((FunctionNode *)node);
  for (auto node : addressTakenFuncs)
    unitHeads.push_back((FunctionNode *)node);
  trimCompilationUnit(unitHeads, threshold, true);
  return;
}

bool EstimateFunctionSize::isTrimmedFunction(llvm::Function *F) { return get<FunctionNode>(F)->isTrimmed(); }

bool EstimateFunctionSize::isLargeKernelThresholdExceeded() const {
  for (auto *node : kernelEntries) {
    auto *kernelNode = (FunctionNode *)node;
    if (kernelNode->ExpandedSize > KernelTotalSizeThreshold * LargeKernelThresholdMultiplier) {
      return true;
    }
  }
  return false;
}

// Initialize data structures for topological traversal: FunctionsInKernel and BottomUpQueue.
// FunctionsInKernel is a map data structure where the key is FunctionNode and value is the number of edges to callee
// nodes. FunctionsInKernel is primarily used for topological traversal and also used to check whether a function is in
// the currently processed kernel/unit. BottomUpQueue will contain the leaf nodes of a kernel/unit and they are starting
// points of topological traversal.
void EstimateFunctionSize::initializeTopologicalVisit(Function *root,
                                                      std::unordered_map<void *, uint32_t> &FunctionsInKernel,
                                                      std::deque<void *> &BottomUpQueue, bool ignoreStackCallBoundary) {
  std::deque<FunctionNode *> Queue;
  FunctionNode *unitHead = get<FunctionNode>(root);
  Queue.push_back(unitHead);
  FunctionsInKernel[unitHead] = unitHead->CalleeList.size();
  // top down traversal to visit functions which will be processed reversely
  while (!Queue.empty()) {
    FunctionNode *Node = Queue.front();
    Queue.pop_front();
    Node->tmpSize = Node->InitialSize;
    for (auto &Callee : Node->CalleeList) {
      FunctionNode *CalleeNode = Callee.first;
      if (FunctionsInKernel.find(CalleeNode) != FunctionsInKernel.end())
        continue;
      if (!ignoreStackCallBoundary &&
          CalleeNode
              ->isStackCallAssigned()) // This callee is a compilation unit head, so not in the current compilation unit
      {
        FunctionsInKernel[Node] -= 1; // Ignore different compilation unit
        continue;
      }
      FunctionsInKernel[CalleeNode] = CalleeNode->CalleeList.size(); // Update the number of edges to callees
      Queue.push_back(CalleeNode);
    }
    if (FunctionsInKernel[Node] == 0) // This means no children or all children are compilation unit heads: leaf node
      BottomUpQueue.push_back(Node);
  }
  return;
}

llvm::ScaledNumber<uint64_t> EstimateFunctionSize::calculateTotalWeight(Function *root) {
  FunctionNode *root_node = get<FunctionNode>(root);
  std::deque<void *> TopdownQueue;
  TopdownQueue.push_back(root_node);
  std::unordered_set<void *> visit;
  visit.insert(root_node);
  Scaled64 totalSizeContributionSq = Scaled64::getZero();
  Scaled64 totalSubroutineFreq = Scaled64::getZero();
  while (!TopdownQueue.empty()) {
    FunctionNode *node = (FunctionNode *)TopdownQueue.front();
    TopdownQueue.pop_front();
    totalSizeContributionSq += Scaled64::get(node->getSizeContribution() * node->getSizeContribution());
    if (!node->willBeInlined())
      totalSubroutineFreq += node->getStaticFuncFreq();
    for (auto &callee_info : node->CalleeList) {
      FunctionNode *callee = callee_info.first;
      if (visit.find(callee) == visit.end()) {
        visit.insert(callee);
        TopdownQueue.push_back(callee);
      }
    }
  }
  return totalSizeContributionSq * totalSizeContributionSq * totalSubroutineFreq;
}

// Update the information about how many time a function will be inlined
void EstimateFunctionSize::updateInlineCnt(Function *root) {
  FunctionNode *root_node = get<FunctionNode>(root);
  std::unordered_map<void *, uint32_t>
      unprocessed_callers; // A data structure to collect the number of callers for a functoin in a kernel boundary
  unprocessed_callers[root_node] = 0;

  std::deque<void *> TopdownQueue;
  TopdownQueue.push_back(root_node);

  std::unordered_set<void *> visit;
  visit.insert(root_node);

  // Top down traversal to initialize the number of callers and inline count in a kernel boundary
  // This step is just for initialization for the topological traverse at the second step
  while (!TopdownQueue.empty()) {
    FunctionNode *node = (FunctionNode *)TopdownQueue.front();
    TopdownQueue.pop_front();
    node->Inline_cnt = 0;
    for (auto &callee_info : node->CalleeList) {
      FunctionNode *callee = callee_info.first;
      if (unprocessed_callers.find(callee) == unprocessed_callers.end())
        unprocessed_callers[callee] = 0; // Initialize callee's caller count

      unprocessed_callers[callee] += 1; // Increment by 1 since the callee is called by the node
      if (visit.find(callee) == visit.end()) {
        visit.insert(callee);
        TopdownQueue.push_back(callee);
      }
    }
  }
  TopdownQueue.push_back(root_node);
  while (!TopdownQueue.empty()) {
    FunctionNode *node = (FunctionNode *)TopdownQueue.front();
    TopdownQueue.pop_front();
    for (auto &callee_info : node->CalleeList) {
      FunctionNode *callee = callee_info.first;
      uint16_t call_cnt = callee_info.second;
      IGC_ASSERT(unprocessed_callers[callee] != 0);
      unprocessed_callers[callee] -= 1;
      if (callee->willBeInlined())
        callee->Inline_cnt += call_cnt * (node->Inline_cnt == 0 ? 1 : node->Inline_cnt);
      if (unprocessed_callers[callee] == 0)
        TopdownQueue.push_back(callee);
    }
  }
  return;
}

// This function compute the size of each function when must-be-inlined functions are all inlined
// must-be-inlined functions are two kinds: 1) have force-inline attribute, 2) small leaf functions
// Functions with those two kinds should be inlined no matter what the reason is.
// When all small leaf functions are inlined and collapsed, there may be a set of new leaf functions
// So, the algorithm repeat collapsing small leaf functions until only large leaf functions are left
void EstimateFunctionSize::UpdateSizeAfterCollapsing(std::deque<void *> &nodesToProcess,
                                                     std::unordered_set<void *> &funcsInKernel) {
  for (auto n : funcsInKernel) {
    // Initialize the size after inlining
    FunctionNode *Node = (FunctionNode *)n;
    Node->SizeAfterCollapsing = Node->InitialSize;
  }
  std::unordered_map<FunctionNode *, uint16_t> remainingCallee;
  std::unordered_set<FunctionNode *> hasCalleesAfterInline;

  while (!nodesToProcess.empty()) {
    FunctionNode *Node = (FunctionNode *)nodesToProcess.front();
    nodesToProcess.pop_front();
    bool hasCallee = hasCalleesAfterInline.find(Node) != hasCalleesAfterInline.end();
    if (Node->willBeInlined() && !hasCallee && Node->SizeAfterCollapsing < ControlInlineTinySizeForSPGT) {
      if (!Node->isForcedInlined()) {
        PrintTrimUnit(0x8, "Small leaf functions should always be inlined"
                               << Node->F->getName().str() << ", Size after Inline: " << Node->SizeAfterCollapsing);
        Node->setForceInline(); // If the node is supposed to have no callee in the end and small size, it should be
                                // inlined
      }
    }

    for (const auto &c : Node->CallerList) {
      FunctionNode *caller = c.first;
      uint16_t call_cnt = c.second;
      if (funcsInKernel.find(caller) ==
          funcsInKernel.end()) // This caller must not be in the currently processing kernel
        continue;

      if (remainingCallee.find(caller) == remainingCallee.end())
        remainingCallee[caller] = caller->CalleeList.size();
      remainingCallee[caller] -= 1;

      if (remainingCallee[caller] == 0)
        nodesToProcess.push_back((FunctionNode *)caller);

      if (Node->isForcedInlined()) { // Will be inlined in any case
        caller->SizeAfterCollapsing += Node->SizeAfterCollapsing * call_cnt;
        if (hasCallee) // Fucntion that already has force inline might have callee
          hasCalleesAfterInline.insert(caller);
      } else { // Otherwise we don't know, so conservatively mark it having callees
        hasCalleesAfterInline.insert(caller);
      }
    }
  }
  return;
}

// Find the total size of a unit when to-be-inlined functions are expanded
// Topologically traverse from leaf nodes and expand nodes to callers except noinline and stackcall functions
uint32_t EstimateFunctionSize::updateExpandedUnitSize(Function *F, bool ignoreStackCallBoundary) {
  FunctionNode *root = get<FunctionNode>(F);
  std::deque<void *> BottomUpQueue;
  std::unordered_map<void *, uint32_t> FunctionsInUnit;
  initializeTopologicalVisit(root->F, FunctionsInUnit, BottomUpQueue, ignoreStackCallBoundary);
  uint32_t unitTotalSize = 0;
  while (!BottomUpQueue.empty()) // Topologically visit nodes and collape for each compilation unit
  {
    FunctionNode *node = (FunctionNode *)BottomUpQueue.front();
    BottomUpQueue.pop_front();
    IGC_ASSERT(FunctionsInUnit[node] == 0);
    FunctionsInUnit.erase(node);
    node->ExpandedSize = node->tmpSize; // Update the size of an expanded chunk
    if (!node->willBeInlined()) {
      // dbgs() << "Not be inlined Attr: " << (int)node->FunctionAttr << "\n";
      unitTotalSize += node->ExpandedSize;
      PrintTrimUnit(0x10, "Expansion stop at " << node->F->getName().str() << ", Attribute: " << node->getFuncAttrStr()
                                               << ", Chunck size: " << node->ExpandedSize
                                               << ", Total chunck size: " << unitTotalSize);
    }

    for (const auto &c : node->CallerList) {
      FunctionNode *caller = c.first;
      if (FunctionsInUnit.find(caller) == FunctionsInUnit.end()) // Caller is in another compilation unit
      {
        node->InMultipleUnit = true;
        continue;
      }
      FunctionsInUnit[caller] -= 1;
      if (FunctionsInUnit[caller] == 0)
        BottomUpQueue.push_back(caller);
      if (node->willBeInlined())
        caller->expand(node); // collapse and update tmpSize of the caller
    }
  }
  // Has recursion
  if (!FunctionsInUnit.empty())
    HasRecursion = true;

  PrintTrimUnit(0x10, "Final expanded size of " << root->F->getName().str() << ": " << unitTotalSize);
  return root->ExpandedSize = unitTotalSize;
}

// Partition kernels using bottom-up heristic.
uint32_t EstimateFunctionSize::bottomUpHeuristic(Function *F, uint32_t &stackCall_cnt) {
  uint32_t threshold = UnitSizeThreshold;
  std::deque<void *> BottomUpQueue;
  std::unordered_map<void *, uint32_t> FunctionsInUnit; // Set of functions in the boundary of a kernel. Record
                                                        // unprocessed callee counter for topological sort.
  initializeTopologicalVisit(F, FunctionsInUnit, BottomUpQueue, false);
  FunctionNode *unitHeader = get<FunctionNode>(F);
  uint32_t max_unit_size = 0;
  while (!BottomUpQueue.empty()) {
    FunctionNode *Node = (FunctionNode *)BottomUpQueue.front();
    BottomUpQueue.pop_front();
    IGC_ASSERT(FunctionsInUnit[Node] == 0);
    FunctionsInUnit.erase(Node);
    Node->UnitSize = Node->tmpSize; // Update the size

    if (Node == unitHeader) // The last node to process is the unit header
    {
      max_unit_size = std::max(max_unit_size, Node->updateUnitSize());
      continue;
    }

    bool beStackCall = Node->canAssignStackCall() && Node->UnitSize > threshold && Node->updateUnitSize() > threshold &&
                       Node->getStaticFuncFreq() < threshold_func_freq;

    if (beStackCall) {
      PrintPartitionUnit(0x4, "Stack call marked " << Node->F->getName().str() << " Unit size: " << Node->UnitSize
                                                   << " > Threshold " << threshold
                                                   << " Function frequency: " << Node->getStaticFuncFreqStr() << " < "
                                                   << threshold_func_freq.toString())
          stackCallFuncs.push_back(Node); // We have a new unit head
      Node->setStackCall();
      max_unit_size = std::max(max_unit_size, Node->UnitSize);
      stackCall_cnt += 1;
    } else {
      if (!Node->canAssignStackCall()) {
        PrintPartitionUnit(0x4, "Stack call not marked: not best effort or trimmed " << Node->F->getName().str())
      } else if (Node->UnitSize <= threshold || Node->updateUnitSize() <= threshold) {
        PrintPartitionUnit(0x4, "Stack call not marked: unit size too small " << Node->F->getName().str())
      } else {
        PrintPartitionUnit(0x4, "Stack call not marked: too many function frequencies "
                                    << Node->getStaticFuncFreqStr() << " > " << threshold_func_freq.toString() << " "
                                    << Node->F->getName().str())
      }
    }

    for (const auto &c : Node->CallerList) {
      FunctionNode *caller = c.first;
      if (FunctionsInUnit.find(caller) == FunctionsInUnit.end()) // The caller is in another kernel, skip
        continue;
      FunctionsInUnit[caller] -= 1;
      if (FunctionsInUnit[caller] == 0) // All callees of the caller are processed: become leaf.
        BottomUpQueue.push_back(caller);
      if (!beStackCall)
        caller->tmpSize += Node->UnitSize;
    }
  }
  return max_unit_size;
}

// For all function F : F->Us = size(F), F->U# = 0 // unit size and unit number
// For each kernel K
//     kernelSize = K->UnitSize // O(C)
//     IF(kernelSize > T)
//         workList = ReverseTopoOrderList(K)  // Bottom up traverse
//         WHILE(worklist not empty) // O(N)
//             remove F from worklist
//             //F->Us might be overestimated due to overcounting issue -> recompute F->Us to find the actual size
//             IF(F->Us > T || recompute(F->Us) > T) {   // recompute(F->Us): O(N) only when F->Us is larger than T
//                 mark F as stackcall;
//                 Add F to end of headList;
//                 continue;
//             }
//             Foreach F->callers P{ P->Us += F->Us; }
//         ENDWHILE
//     ENDIF
// ENDFOR
void EstimateFunctionSize::partitionKernel() {
  uint32_t threshold = UnitSizeThreshold;
  uint32_t max_unit_size = 0;
  uint32_t stackCall_cnt = 0;

  // Iterate over kernel
  llvm::SmallVector<void *, 64> unitHeads;
  for (auto node : kernelEntries)
    unitHeads.push_back((FunctionNode *)node);
  for (auto node : stackCallFuncs)
    unitHeads.push_back((FunctionNode *)node);
  for (auto node : addressTakenFuncs)
    unitHeads.push_back((FunctionNode *)node);

  for (auto node : unitHeads) {
    FunctionNode *UnitHead = (FunctionNode *)node;
    if (UnitHead->UnitSize <= threshold) // Unit size is within threshold, skip
    {
      max_unit_size = std::max(max_unit_size, UnitHead->UnitSize);
      continue;
    }
    PrintPartitionUnit(0x2, "Partition Kernel " << UnitHead->F->getName().str()
                                                << " Original Unit Size: " << UnitHead->UnitSize)
        uint32_t size_after_partition = bottomUpHeuristic(UnitHead->F, stackCall_cnt);
    max_unit_size = std::max(max_unit_size, size_after_partition);
    PrintPartitionUnit(0x2, "Unit size after partitioning: " << size_after_partition)
  }
  float threshold_err = (float)(max_unit_size - threshold) / threshold * 100;
  PrintPartitionUnit(0x2, "Max unit size: " << max_unit_size << " Threshold Error Rate: " << threshold_err << "%");
  PrintPartitionUnit(0x2, "Stack call cnt: " << stackCall_cnt);
  return;
}

// Work same as reduceKernel except for stackcall functions
void EstimateFunctionSize::reduceCompilationUnitSize() {
  uint32_t threshold = ExpandedUnitSizeThreshold;
  llvm::SmallVector<void *, 64> unitHeads;
  for (auto node : kernelEntries)
    unitHeads.push_back((FunctionNode *)node);
  for (auto node : stackCallFuncs)
    unitHeads.push_back((FunctionNode *)node);
  for (auto node : addressTakenFuncs)
    unitHeads.push_back((FunctionNode *)node);

  trimCompilationUnit(unitHeads, threshold, false);
  return;
}

// Top down traverse to find and retrieve functions that meet trimming criteria
void EstimateFunctionSize::getFunctionsToTrim(llvm::Function *root, llvm::SmallVector<void *, 64> &trimming_pool,
                                              llvm::SmallVector<void *, 64> &tiny_fn_trimming_pool,
                                              bool ignoreStackCallBoundary, uint32_t &func_cnt) {
  FunctionNode *unitHead = get<FunctionNode>(root);
  std::unordered_set<void *> visit;
  std::deque<FunctionNode *> TopDownQueue;
  TopDownQueue.push_back(unitHead);
  visit.insert((void *)unitHead);

  SmallVector<FunctionNode *, 64> funcsInKernel;
  uint64_t tinySizeThreshold = ControlInlineTinySize;

  std::deque<void *> bottomUpQueue;

  // Profile function information in the kernel boundary
  while (!TopDownQueue.empty()) {
    FunctionNode *Node = TopDownQueue.front();
    TopDownQueue.pop_front();
    for (auto &Callee : Node->CalleeList) {
      FunctionNode *calleeNode = Callee.first;
      if (visit.find((void *)calleeNode) != visit.end() ||
          (!ignoreStackCallBoundary && calleeNode->isStackCallAssigned()))
        continue;
      visit.insert((void *)calleeNode);
      TopDownQueue.push_back(calleeNode);
    }

    funcsInKernel.push_back(Node);
    if (Node->CalleeList.empty())
      bottomUpQueue.push_back((void *)Node);
  }
  func_cnt += visit.size();

  if (EnableSizeContributionOptimization)
    updateInlineCnt(root);
  if (EnableLeafCollapsing)
    UpdateSizeAfterCollapsing(bottomUpQueue, visit);

  if (EnableGreedyTrimming) {
    trimming_pool = llvm::SmallVector<void *, 64>(funcsInKernel.size());
    // Node with best effort and larger size contribution could be trimmed
    llvm::copy_if(funcsInKernel, std::back_inserter(trimming_pool),
                  [](void *node) { return ((FunctionNode *)node)->isBestEffortInline(); });
    return;
  }

  // Find all functions that meet trimming criteria

  for (FunctionNode *Node : funcsInKernel) {
    uint16_t func_trait = Node->getFunctionTrait(thresholdForTrimming);
    switch (func_trait) {
    case FT_NOT_BEST_EFFORT:
      Node->dumpFuncInfo(0x4, "Can't trim (not best effort inline)");
      break;
    case FT_MUL_KERNEL:
      Node->dumpFuncInfo(0x4, "Can't trim (in multiple kernels)");
      break;
    case FT_BIG_ENOUGH: // Functions are big enough to trim
      trimming_pool.push_back(Node);
      Node->dumpFuncInfo(0x4, "Good to trim (Big enough > " + std::to_string(tinySizeThreshold) + ")");
      break;
    case FT_TOO_TINY:
      // Small functions will be trimmed in special case if kernel still far exceeds threshold
      tiny_fn_trimming_pool.push_back(Node);
      Node->dumpFuncInfo(0x4, "Can't trim (Too tiny < " + std::to_string(tinySizeThreshold) + ")");
      break;
    case FT_HIGHER_WEIGHT:
      trimming_pool.push_back(Node);
      Node->dumpFuncInfo(0x4, "Good to trim (High weight > " + thresholdForTrimming.toString() + ")");
      break;
    case FT_LOWER_WEIGHT:
      Node->dumpFuncInfo(0x4, "Can't trim (Low weight < " + thresholdForTrimming.toString() + ")");
      break;
    default:
      PrintTrimUnit(0x4, "Something goes wrong with the function property");
      break;
    }
  }
  return;
}

// Trim kernel/unit by canceling out inline candidate functions one by one until the total size is within threshold
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
void EstimateFunctionSize::trimCompilationUnit(llvm::SmallVector<void *, 64> &unitHeads, uint32_t threshold,
                                               bool ignoreStackCallBoundary) {
  llvm::SmallVector<FunctionNode *, 64> unitsToTrim;
  // Extract kernels / units that are larger than threshold
  for (auto node : unitHeads) {
    FunctionNode *unitEntry = (FunctionNode *)node;
    // Partitioning can add more stackcalls. So need to recompute the expanded unit size.
    updateExpandedUnitSize(unitEntry->F, ignoreStackCallBoundary);
    if (unitEntry->ExpandedSize > threshold) {
      PrintTrimUnit(0x2, "Kernel / Unit " << unitEntry->F->getName().str() << " expSize= " << unitEntry->ExpandedSize
                                          << " > " << threshold) unitsToTrim.push_back(unitEntry);
    } else {
      PrintTrimUnit(0x2, "Kernel / Unit " << unitEntry->F->getName().str() << " expSize= " << unitEntry->ExpandedSize
                                          << " <= " << threshold)
    }
  }

  if (unitsToTrim.empty()) {
    PrintTrimUnit(0x2, "Kernels / Units become no longer big enough to be trimmed (affected by partitioning)") return;
  }

  std::sort(unitsToTrim.begin(), unitsToTrim.end(), [&](const FunctionNode *LHS, const FunctionNode *RHS) {
    return LHS->ExpandedSize > RHS->ExpandedSize;
  }); // Sort by expanded size

  // Iterate over units
  for (auto unit : unitsToTrim) {
    size_t expandedUnitSize =
        updateExpandedUnitSize(unit->F, ignoreStackCallBoundary); // A kernel size can be reduced by a function that is
                                                                  // trimmed at previous kernels, so recompute it.
    PrintTrimUnit(0x2, "Trimming kernel / unit " << unit->F->getName().str() << " expanded size= "
                                                 << expandedUnitSize) if (expandedUnitSize <= threshold) {
      PrintTrimUnit(0x2, "Kernel / unit " << unit->F->getName().str() << ": The expanded unit size(" << expandedUnitSize
                                          << ") is smaller than threshold(" << threshold << ")") continue;
    }
    PrintTrimUnit(0x2, "Kernel size is bigger than threshold")

        SmallVector<void *, 64>
            trimming_pool;
    SmallVector<void *, 64> tiny_fn_trimming_pool;
    uint32_t func_cnt = 0;
    getFunctionsToTrim(unit->F, trimming_pool, tiny_fn_trimming_pool, ignoreStackCallBoundary, func_cnt);
    PrintTrimUnit(0x2, "Kernel / Unit " << unit->F->getName().str() << " has " << trimming_pool.size()
                                        << " functions for trimming out of " << func_cnt) if (trimming_pool.empty()) {
      PrintTrimUnit(0x2, "Kernel / Unit " << unit->F->getName().str() << " size " << unit->ExpandedSize
                                          << " has no sorted list") continue; // all functions are tiny.
    }
    uint64_t size_before_trimming = unit->ExpandedSize;
    if (EnableGreedyTrimming) {
      performGreedyTrimming(unit->F, trimming_pool, threshold, ignoreStackCallBoundary);
    } else {
      performTrimming(unit->F, trimming_pool, threshold, ignoreStackCallBoundary);
      if (ignoreStackCallBoundary && unit->ExpandedSize > threshold * LargeKernelThresholdMultiplier) {
        PrintTrimUnit(0x2, "Kernel / Unit " << unit->F->getName().str() << ": Size: " << unit->ExpandedSize
                                            << " is much larger than threshold, trimming small functions as well.")
            performTrimming(unit->F, tiny_fn_trimming_pool, threshold, ignoreStackCallBoundary);
      }
    }
    if (unit->ExpandedSize < threshold) {
      PrintTrimUnit(0x2, "Kernel / Unit " << unit->F->getName().str() << ": The size becomes below threshold")
    } else {
      PrintTrimUnit(0x2, "Kernel / Unit "
                             << unit->F->getName().str()
                             << ": The size is still above threshold even though all candidates are trimmed")
    }

    PrintTrimUnit(0x2, "Kernel / Unit " << unit->F->getName().str() << " final size " << unit->ExpandedSize
                                        << " reduced from " << size_before_trimming)
  }
}

void EstimateFunctionSize::performGreedyTrimming(Function *head, llvm::SmallVector<void *, 64> &functions_to_trim,
                                                 uint32_t threshold, bool ignoreStackCallBoundary) {

  llvm::SmallVector<FunctionNode *, 64> candidates;
  llvm::SmallVector<FunctionNode *, 64> funcWithNoEffect;

  for (auto f : functions_to_trim) {
    FunctionNode *func = (FunctionNode *)f;
    if (func->getSizeContribution() != func->getPotentialBodySize()) {
      candidates.push_back(func);
    } else {
      funcWithNoEffect.push_back(func);
    }
  }

  uint32_t total_trim_cnt = 0;
  while (!candidates.empty()) {
    Scaled64 minWeight = calculateTotalWeight(head);
    FunctionNode *bestForTrim = NULL;
    Scaled64 weightBeforeTrim = minWeight;
    PrintTrimUnit(0x8, "Trimming candidate count: " << candidates.size());
    for (auto func : candidates) {
      func->setTrimmed();
      // Update inline count
      updateInlineCnt(head);
      // calculate weight
      Scaled64 weight = calculateTotalWeight(head);
      if (weight < minWeight) {
        minWeight = weight;
        bestForTrim = func;
      }
      func->unsetTrimmed();
      updateInlineCnt(head);
    }
    PrintTrimUnit(0x8, "Total weight before trim: " << weightBeforeTrim.toString()
                                                    << " Total weight after trim: " << minWeight.toString());
    if (bestForTrim == NULL) // Trimming any of functions result in better code
      break;
    PrintTrimUnit(0x8, "Trim the function " << bestForTrim->F->getName().str()
                                            << ", Function Attribute: " << bestForTrim->getFuncAttrStr()
                                            << ", Function size: " << bestForTrim->InitialSize
                                            << ", Size after inlining: " << bestForTrim->SizeAfterCollapsing
                                            << ", Size contribution: " << bestForTrim->getSizeContribution()
                                            << ", Freq: " << bestForTrim->getStaticFuncFreqStr()
                                            << ", Weight: " << bestForTrim->getWeightForTrimming().toString());

    bestForTrim->setTrimmed();
    updateInlineCnt(head);
    total_trim_cnt += 1;
    PrintTrimUnit(0x8,
                  "The size contribution of the trimmed function changes to " << bestForTrim->getSizeContribution());

    llvm::SmallVector<FunctionNode *, 64> new_candidates;
    for (auto func : candidates) {
      if (func->getSizeContribution() != func->getPotentialBodySize()) {
        new_candidates.push_back(func);
      } else {
        funcWithNoEffect.push_back(func);
      }
    }
    candidates = std::move(new_candidates);
  }
  updateExpandedUnitSize(head, ignoreStackCallBoundary);
  for (FunctionNode *trimNoGain : candidates) // Those remaining candidates will likely degrade performance
  {
    PrintTrimUnit(0x8, "Dont't trim (Performance penalty is higher than size reduction)"
                           << trimNoGain->F->getName().str() << ", Function Attribute: " << trimNoGain->getFuncAttrStr()
                           << ", Function size: " << trimNoGain->InitialSize
                           << ", Size after inlining: " << trimNoGain->SizeAfterCollapsing << ", Size contribution: "
                           << trimNoGain->getSizeContribution() << ", Freq: " << trimNoGain->getStaticFuncFreqStr()
                           << ", Weight: " << trimNoGain->getWeightForTrimming().toString());
  }
  for (FunctionNode *trimNoGain : funcWithNoEffect) // The kernel size will not change when those functions are trimmed
  {
    PrintTrimUnit(0x8, "Dont't trim (Trimming doesn't give size reduction)"
                           << trimNoGain->F->getName().str() << ", Function Attribute: " << trimNoGain->getFuncAttrStr()
                           << ", Function size: " << trimNoGain->InitialSize
                           << ", Size after inlining: " << trimNoGain->SizeAfterCollapsing << ", Size contribution: "
                           << trimNoGain->getSizeContribution() << ", Freq: " << trimNoGain->getStaticFuncFreqStr()
                           << ", Weight: " << trimNoGain->getWeightForTrimming().toString());
  }
  PrintTrimUnit(0x8, "In total, " << total_trim_cnt << " function(s) are trimmed out of " << functions_to_trim.size());
  return;
}

void EstimateFunctionSize::performTrimming(Function *head, llvm::SmallVector<void *, 64> &functions_to_trim,
                                           uint32_t threshold, bool ignoreStackCallBoundary) {
  FunctionNode *unitHead = get<FunctionNode>(head);
  uint32_t total_cand = functions_to_trim.size();
  uint32_t total_trim_cnt = 0;
  // Sort all to-be trimmed function according to the its actual size

  // Repeat trimming functions for cold functions until the unit size is smaller than threshold
  while (!functions_to_trim.empty() && unitHead->ExpandedSize >= threshold) {
    std::sort(functions_to_trim.begin(), functions_to_trim.end(), [&](const void *LHS, const void *RHS) {
      return ((FunctionNode *)LHS)->getWeightForTrimming() < ((FunctionNode *)RHS)->getWeightForTrimming();
    });
    FunctionNode *functionToTrim = (FunctionNode *)functions_to_trim.back(); // Pick the largest one first to trim
    functions_to_trim.pop_back();
    uint64_t original_expandedSize = unitHead->ExpandedSize;

    if (EnableSizeContributionOptimization) {
      uint64_t size_contribution = functionToTrim->getSizeContribution();
      uint64_t FuncSize = functionToTrim->getPotentialBodySize();
      if (FuncSize == size_contribution && FuncSize < SkipTrimmingOneCopyFunction) {
        functionToTrim->dumpFuncInfo(0x8, "Don't trim (Same size contribution and too small)");
        continue;
      }
      functionToTrim->dumpFuncInfo(0x8, "Trim the function");
      functionToTrim->setTrimmed();
      updateInlineCnt(head);
      PrintTrimUnit(0x8, "The size contribution of the trimmed function changes to "
                             << functionToTrim->getSizeContribution());
    } else {
      functionToTrim->dumpFuncInfo(0x8, "Trim the function");
      functionToTrim->setTrimmed();
    }
    total_trim_cnt += 1;
    // After trimming, update expanded size
    updateExpandedUnitSize(head, ignoreStackCallBoundary);
    PrintTrimUnit(0x8, "The kernel size is reduced after trimming from " << original_expandedSize << " to "
                                                                         << unitHead->ExpandedSize);
  }
  PrintTrimUnit(0x8, "In total, " << total_trim_cnt << " function(s) are trimmed out of " << total_cand);
  return;
}

bool EstimateFunctionSize::isStackCallAssigned(llvm::Function *F) {
  FunctionNode *Node = get<FunctionNode>(F);
  return Node->isStackCallAssigned();
}

uint32_t EstimateFunctionSize::getMaxUnitSize() {
  uint32_t max_val = 0;
  for (auto kernelEntry : kernelEntries) // For all kernel, update unitsize
  {
    FunctionNode *head = (FunctionNode *)kernelEntry;
    max_val = std::max(max_val, head->UnitSize);
  }
  for (auto stackCallFunc : stackCallFuncs) // For all address taken functions, update unitsize
  {
    FunctionNode *head = (FunctionNode *)stackCallFunc;
    max_val = std::max(max_val, head->UnitSize);
  }
  for (auto addrTakenFunc : addressTakenFuncs) // For all address taken functions, update unitsize
  {
    FunctionNode *head = (FunctionNode *)addrTakenFunc;
    max_val = std::max(max_val, head->UnitSize);
  }
  return max_val;
}