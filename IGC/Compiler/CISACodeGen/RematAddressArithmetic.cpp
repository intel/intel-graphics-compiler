/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/RematAddressArithmetic.h"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/ADT/BreadthFirstIterator.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/Local.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Instructions.h"
#include "Compiler/CISACodeGen/Platform.hpp"
#include "Compiler/CISACodeGen/IGCLivenessAnalysis.h"
#include <fstream>

using namespace llvm;
using namespace IGC;

static cl::opt<bool> FirstCompileOpt("remat-first-compile", cl::init(false), cl::Hidden,
                                     cl::desc("Run the default-constructed pass in first-compile mode: roll "
                                              "back rematerialization that does not reach the pressure budget"));

static cl::opt<bool> WidestSIMD("remat-widest-simd", cl::init(false), cl::Hidden,
                                cl::desc("Estimate register pressure in the default-constructed pass at the "
                                         "widest allowed SIMD instead of the best-guess SIMD"));

namespace IGC {
enum REMAT_OPTIONS : uint8_t {
#define REMAT_FLAG(Name, Val, Description) Name = Val,
#include "igc_regkeys_enums_defs.h"
  REMAT_MASK
#undef REMAT_FLAG
#undef REMAT_MASK
      REMAT_ALL = 255
};
} // namespace IGC

static Value *getPrivateMemoryValue(Function &F);

namespace {

// A reversible IR edit. Edits are recorded as they are made, and the transaction is then either committed or rolled
// back. Instructions are never deleted before the commit: removed ones are only detached from their block together
// with the instruction that followed them, which lets a rollback put them back where they were.
class IRTransaction {
public:
  IRTransaction() = default;
  IRTransaction(const IRTransaction &) = delete;
  IRTransaction &operator=(const IRTransaction &) = delete;
  ~IRTransaction() { IGC_ASSERT_MESSAGE(isClosed(), "transaction was neither committed nor rolled back"); }

  // Makes U use New and remembers the old value.
  void replaceUse(Use &U, Value *New) {
    RewrittenUses.push_back({&U, U.get()});
    U.set(New);
  }

  // Registers an instruction inserted by the transaction. A rollback erases it.
  void created(Instruction *I) { Created.push_back(I); }

  // Registers an instruction whose uses were all replaced. detachDead() removes it when it is really dead.
  void replaced(Instruction *I) { Replaced.push_back(I); }

  // Removes I from its block without deleting it.
  void detach(Instruction *I) {
    Detached.push_back({I, I->getNextNode()});
    I->removeFromParent();
  }

  // Detaches the replaced instructions that have no users left, and the operands only they kept alive. This is
  // the IR that is left once dead code is removed, which is what an analysis run before the commit has to see.
  void detachDead() {
    llvm::SmallSetVector<Instruction *, 16> Dead;
    llvm::SmallVector<Instruction *, 16> Worklist(Replaced.begin(), Replaced.end());
    while (!Worklist.empty()) {
      Instruction *I = Worklist.pop_back_val();
      if (Dead.count(I) || !llvm::wouldInstructionBeTriviallyDead(I))
        continue;
      if (!llvm::all_of(I->users(), [&](User *U) { return Dead.count(cast<Instruction>(U)); }))
        continue;
      Dead.insert(I);
      for (Value *Op : I->operands())
        if (auto *OpI = dyn_cast<Instruction>(Op))
          Worklist.push_back(OpI);
    }
    for (Instruction *I : Dead)
      detach(I);
    Replaced.clear();
  }

  // Keeps the edit: the detached instructions are deleted.
  void commit() {
    for (auto &D : Detached)
      D.I->dropAllReferences();
    for (auto &D : Detached)
      D.I->deleteValue();
    clear();
  }

  // Restores the IR: uses point to their old values, detached instructions return to their place, and the inserted
  // instructions are erased.
  void rollback() {
    for (auto &[U, V] : llvm::reverse(RewrittenUses))
      U->set(V);
    // Reverse order of detaching, so the recorded successor is always in a block again.
    for (auto &D : llvm::reverse(Detached))
      IGCLLVM::insertBefore(D.I, D.Next);
    for (Instruction *I : Created)
      I->dropAllReferences();
    for (Instruction *I : Created)
      I->eraseFromParent();
    clear();
  }

private:
  struct DetachedInst {
    Instruction *I;
    Instruction *Next;
  };

  bool isClosed() const { return RewrittenUses.empty() && Created.empty() && Replaced.empty() && Detached.empty(); }
  void clear() {
    RewrittenUses.clear();
    Created.clear();
    Replaced.clear();
    Detached.clear();
  }

  SmallVector<std::pair<Use *, Value *>, 64> RewrittenUses;
  SmallVector<Instruction *, 64> Created;
  SmallVector<Instruction *, 16> Replaced;
  SmallVector<DetachedInst, 64> Detached;
};

class RematAddressArithmetic : public FunctionPass {

public:
  static char ID;

  RematAddressArithmetic() : FunctionPass(ID) {
    initializeRematAddressArithmeticPass(*PassRegistry::getPassRegistry());
  }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<PostDominatorTreeWrapperPass>();
  }

  bool runOnFunction(Function &) override;

private:
  bool rematerializePrivateMemoryAddressCalculation(Function &F);
  bool rematerializePhiMemoryAddressCalculation(Function &F);
  bool rematerialize(Instruction *I, SmallVectorImpl<Value *> &Chain);
};

class CloneAddressArithmetic : public FunctionPass {
public:
  static char ID;
  WIAnalysis *WI = nullptr;

  ~CloneAddressArithmetic() { Uses.clear(); }
  CloneAddressArithmetic()
      : CloneAddressArithmetic(rematFlagsFromRegKeys(), IGC_GET_FLAG_VALUE(RematChainLimit),
                               IGC_GET_FLAG_VALUE(RematFlowThreshold), IGC_GET_FLAG_VALUE(RematRPELimit),
                               WidestSIMD ? widestAllowedSIMDSize : bestGuessSIMDSize, FirstCompileOpt) {}

  CloneAddressArithmetic(unsigned ChainLimit, unsigned FlowThresholdPercent, unsigned RPELimitPercent,
                         RematSIMDSizeFn GetSIMDSize = bestGuessSIMDSize, bool FirstCompile = false)
      : CloneAddressArithmetic(rematFlagsFromRegKeys(), ChainLimit, FlowThresholdPercent, RPELimitPercent, GetSIMDSize,
                               FirstCompile) {}

  CloneAddressArithmetic(IGC::REMAT_OPTIONS options, unsigned ChainLimit, unsigned FlowThresholdPercent,
                         unsigned RPELimitPercent, RematSIMDSizeFn GetSIMDSize = bestGuessSIMDSize,
                         bool FirstCompile = false)
      : FunctionPass(ID), m_rematFlags(options), m_chainLimit(ChainLimit), m_flowThresholdPercent(FlowThresholdPercent),
        m_rpeLimitPercent(RPELimitPercent), m_getSIMDSize(GetSIMDSize), m_firstCompile(FirstCompile) {

    initializeCloneAddressArithmeticPass(*PassRegistry::getPassRegistry());
  }

  CloneAddressArithmetic(const CloneAddressArithmetic &) = delete;            // Delete copy-constructor
  CloneAddressArithmetic &operator=(const CloneAddressArithmetic &) = delete; // Delete assignment operator

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<IGCLivenessAnalysis>();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<WIAnalysis>();
  }

  using RematChain = llvm::SmallVector<llvm::Instruction *, 16>;
  using RematSet = llvm::SmallSetVector<llvm::Instruction *, 16>;
  using RematPair = std::pair<RematSet, RematSet>;
  using SliceToRematTargetVector = llvm::SmallVector<RematPair, 16>;

  bool runOnFunction(Function &) override;

  std::unordered_map<llvm::Value *, unsigned int> Uses;
  std::unordered_map<llvm::Instruction *, unsigned int> FlowMap;
  SliceToRematTargetVector Vector;
  RematSet MinCut;

  std::unique_ptr<std::ofstream> OutputLogFile;
  std::string LogStr;
  llvm::raw_string_ostream OutputLogStream = raw_string_ostream(LogStr);

  CodeGenContext *CGCtx = nullptr;
  IGCLivenessAnalysisRunner *RPE = nullptr;
  GenXFunctionGroupAnalysis *FGA = nullptr;

  unsigned int SIMD = 0;
  unsigned int MaxPressure = 0;
  llvm::Function *Func = nullptr;

private:
  static IGC::REMAT_OPTIONS rematFlagsFromRegKeys() {
    unsigned Flags = REMAT_ALL;

    if (IGC_IS_FLAG_DISABLED(RematDataAllowCMP))
      Flags &= ~REMAT_COMPARISONS;

    if (IGC_IS_FLAG_DISABLED(RematCollectCallArgs))
      Flags &= ~REMAT_ARGS;

    return static_cast<IGC::REMAT_OPTIONS>(Flags);
  }

  IGC::REMAT_OPTIONS m_rematFlags = REMAT_NONE;
  unsigned m_chainLimit = 0;
  unsigned m_flowThresholdPercent = 0;
  unsigned m_rpeLimitPercent = 0;
  RematSIMDSizeFn m_getSIMDSize = bestGuessSIMDSize;
  bool m_firstCompile = false;
  bool greedyRemat(Function &F);
  bool singleFlowRemat(Function &F);
  bool rematerialize(RematSet &ToProcess, unsigned int FlowThreshold, IRTransaction &Tx, bool OwnedChainsOnly);
  bool isRegPressureLow(Function &F);
  bool skipChain(RematChain &Chain, Instruction *Root);

  RematChain collectRematChain(llvm::Instruction *I, unsigned int NumOfUsesLimit, const RematSet *OwnedSeeds);

  unsigned int getFlowThreshold(RematSet &ToProcess, Function &F);

  void countUses(Function &);
  void speculateWholeChain(RematSet &ToProcess, unsigned int UsesLimit, bool OwnedChainsOnly);
  void collectInstToProcess(RematSet &ToProcess, Function &F);
  void addToSystem(CloneAddressArithmetic::RematSet &Set, llvm::Instruction *I);
  void computeFlow(RematSet &ToProcess);
  void computeFlow(llvm::Instruction *I);
  llvm::Instruction *cloneWithDependency(llvm::Instruction *I);
  void rematWholeChain(llvm::Instruction *I, RematChain &Chain, IRTransaction &Tx);
  unsigned int measurePressure(Function &F);
  unsigned int pressureBudget(Function &F);
  unsigned int simd16DropPressure(Function &F, unsigned int Budget);
  bool prefersSIMD16OnVRT(Function &F);
  unsigned int retryPressureBudget(Function &F);
  bool isOwnedByFlow(Instruction *Op, const RematSet &Seeds);
  void commitIfBudgetReached(Function &F, IRTransaction &Remat, unsigned int Budget, unsigned int SIMD16Pressure);
  void rematerializeWithinBudget(Function &F, RematSet &ToProcess, unsigned int FlowThreshold, bool OwnedChainsOnly);
  void estimateProfit(RematSet &ToProcess);
  void initializeLogFile(Function &F);
  void writeLog();
};
} // end namespace

FunctionPass *IGC::createCloneAddressArithmeticPass(unsigned ChainLimit, unsigned FlowThresholdPercent,
                                                    unsigned RPELimitPercent, RematSIMDSizeFn GetSIMDSize,
                                                    bool FirstCompile) {
  return new CloneAddressArithmetic(ChainLimit, FlowThresholdPercent, RPELimitPercent, GetSIMDSize, FirstCompile);
}
FunctionPass *IGC::createCloneAddressArithmeticPassWithFlags(IGC::REMAT_OPTIONS options, unsigned ChainLimit,
                                                             unsigned FlowThresholdPercent, unsigned RPELimitPercent,
                                                             RematSIMDSizeFn GetSIMDSize, bool FirstCompile) {
  return new CloneAddressArithmetic(options, ChainLimit, FlowThresholdPercent, RPELimitPercent, GetSIMDSize,
                                    FirstCompile);
}

char CloneAddressArithmetic::ID = 0;

#define PASS_FLAG_2 "igc-clone-address-arithmetic"
#define PASS_DESC_2 "Clone Address Arithmetic"
#define PASS_CFG_ONLY_2 false
#define PASS_ANALYSIS_2 false
namespace IGC {
IGC_INITIALIZE_PASS_BEGIN(CloneAddressArithmetic, PASS_FLAG_2, PASS_DESC_2, PASS_CFG_ONLY_2, PASS_ANALYSIS_2)
IGC_INITIALIZE_PASS_DEPENDENCY(IGCLivenessAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(CloneAddressArithmetic, PASS_FLAG_2, PASS_DESC_2, PASS_CFG_ONLY_2, PASS_ANALYSIS_2)
} // namespace IGC

#define DEBUG IGC_IS_FLAG_ENABLED(RematLog)
#if 1
#define PRINT_LOG(Str)                                                                                                 \
  if (IGC_IS_FLAG_ENABLED(RematLog))                                                                                   \
    OutputLogStream << Str;
#define PRINT_LOG_NL(Str)                                                                                              \
  if (IGC_IS_FLAG_ENABLED(RematLog))                                                                                   \
    OutputLogStream << Str << "\n";
#define PRINT_INST(I)                                                                                                  \
  if (IGC_IS_FLAG_ENABLED(RematLog)) {                                                                                 \
    I->print(OutputLogStream, false);                                                                                  \
  }
#define PRINT_INST_NL(I)                                                                                               \
  if (IGC_IS_FLAG_ENABLED(RematLog)) {                                                                                 \
    I->print(OutputLogStream, false);                                                                                  \
    OutputLogStream << "\n";                                                                                           \
  }
#define PRINT_DS(Str, DS)                                                                                              \
  if (IGC_IS_FLAG_ENABLED(RematLog)) {                                                                                 \
    for (auto DS_EL : DS) {                                                                                            \
      PRINT_LOG(Str);                                                                                                  \
      PRINT_INST_NL(DS_EL);                                                                                            \
    }                                                                                                                  \
  }
#else
#define PRINT_LOG(Str)                                                                                                 \
  if (IGC_IS_FLAG_ENABLED(RematLog))                                                                                   \
    llvm::errs() << Str;
#define PRINT_LOG_NL(Str)                                                                                              \
  if (IGC_IS_FLAG_ENABLED(RematLog))                                                                                   \
    llvm::errs() << Str << "\n";
#define PRINT_INST(I)                                                                                                  \
  if (IGC_IS_FLAG_ENABLED(RematLog)) {                                                                                 \
    I->print(llvm::errs(), false);                                                                                     \
  }
#define PRINT_INST_NL(I)                                                                                               \
  if (IGC_IS_FLAG_ENABLED(RematLog)) {                                                                                 \
    I->print(llvm::errs(), false);                                                                                     \
    llvm::errs() << "\n";                                                                                              \
  }
#endif

static bool isSafelyRematerializable(Use &Use) {

  auto LI = llvm::isa<LoadInst>(Use.getUser());
  auto SI = llvm::isa<StoreInst>(Use.getUser());
  auto BI = llvm::isa<BitCastInst>(Use.getUser());
  auto SelI = llvm::isa<SelectInst>(Use.getUser());
  auto BO = llvm::isa<BinaryOperator>(Use.getUser());
  auto CI = IGC_IS_FLAG_ENABLED(RematAddrSpaceCastToUse) ? llvm::isa<AddrSpaceCastInst>(Use.getUser()) : false;
  // TODO: move to whitelist option
  // sometimes it helps to rematerialize arguments for llvm.debug functions in general it's not safe.
  // this is not airtight, use only for testing purposes, if performance gains are significant it should be
  // investigated. visa can exhibit strange behavior sometimes
  auto CLI = IGC_IS_FLAG_ENABLED(RematCallsOperand) ? llvm::isa<CallInst>(Use.getUser()) : false;

  bool Result = LI || SI || BI || CI || CLI || SelI || BO;
  return Result;
}

static bool isAddressArithmetic(Instruction *I) {
  bool Result = isa<GetElementPtrInst, InsertElementInst, InsertValueInst, BinaryOperator, AddrSpaceCastInst,
                    SelectInst, CastInst>(I) ||
                (isa<UnaryInstruction>(I) && !isa<LoadInst>(I)) ||
                (IGC_GET_FLAG_VALUE(RematAllowLoads) && isa<LoadInst>(I)) ||
                (IGC_GET_FLAG_VALUE(RematAllowOneUseLoad) && isa<LoadInst>(I) && I->hasOneUse()) ||
                (IGC_GET_FLAG_VALUE(RematAllowExtractElement) && isa<ExtractElementInst>(I));

  return Result;
}

static bool hasKnownPrimitiveSize(Type *Ty) { return Ty->isIntOrIntVectorTy() || Ty->isFPOrFPVectorTy(); }

// An instruction is "always profitable" to rematerialize, regardless of the
// use-count heuristic, when cloning it does not increase live-value pressure:
//   - a size-expanding unary cast (e.g. i16->i32 zext/sext, i16->float sitofp,
//     float->double fpext): keep the narrow source value live and produce the
//     wider value right next to the use.
//   - a binary operator with at least one constant operand: the clone only
//     keeps its non-constant operand(s) live.
static bool isAlwaysProfitableToRemat(Instruction *I) {
  if (auto *CI = dyn_cast<CastInst>(I)) {
    Type *SrcTy = CI->getSrcTy();
    Type *DstTy = CI->getDestTy();
    if (!hasKnownPrimitiveSize(SrcTy) || !hasKnownPrimitiveSize(DstTy))
      return false;
    return DstTy->getPrimitiveSizeInBits() > SrcTy->getPrimitiveSizeInBits();
  }

  if (auto *BO = dyn_cast<BinaryOperator>(I))
    return (isa<Constant>(BO->getOperand(0)) || isa<Constant>(BO->getOperand(1)));

  return false;
}

void addToSetRemat(llvm::Instruction *Inst, CloneAddressArithmetic::RematSet &Set) {
  for (auto &Op : Inst->operands()) {
    llvm::Value *V = Op.get();
    // We are counting only instructions right now
    // potetntially we should also count globals, but
    // we defintely shouldn't count:
    // br label %bb1 (basic block names)
    // call %functionName (function names)
    // add %a, 1 (constants)
    if (!(llvm::isa<llvm::Instruction>(V) || llvm::isa<llvm::Argument>(V)))
      continue;
    // fix it in sameSet processing
    if (llvm::isa<llvm::PHINode>(V))
      continue;
    Set.insert(static_cast<llvm::Instruction *>(V));
  }
}

bool setCompare(CloneAddressArithmetic::RematSet &A, CloneAddressArithmetic::RematSet &B) {

  if (B.empty() || A.empty())
    return false;
  bool IsSame = true;
  for (auto *Elem : A)
    IsSame &= (bool)B.count(Elem);
  return IsSame && B.size() == A.size();
}

bool isSubset(CloneAddressArithmetic::RematSet &A, CloneAddressArithmetic::RematSet &B) {

  bool IsSame = true;
  for (auto *Elem : A)
    IsSame &= (bool)B.count(Elem);
  // #TODO: process supersets and subsets
  return IsSame;
}

void CloneAddressArithmetic::computeFlow(llvm::Instruction *I) {

  std::queue<llvm::Instruction *> BFSQ;
  BFSQ.push(I);
  unsigned int NumOfUses = Uses[I];

  std::unordered_set<llvm::Instruction *> Explored;

  while (!BFSQ.empty()) {

    llvm::Instruction *CurrI = BFSQ.front();
    BFSQ.pop();
    for (unsigned int i = 0; i < CurrI->getNumOperands(); ++i) {

      Instruction *Op = llvm::dyn_cast<Instruction>(CurrI->getOperand(i));
      if (!Op)
        continue;

      bool NotPHI = !llvm::isa<llvm::PHINode>(Op);
      bool NotConstant = !llvm::isa<llvm::Constant>(Op);
      bool NotUniform = IGC_IS_FLAG_ENABLED(RematRespectUniformity) ? !WI->isUniform(Op) : true;
      bool AddressArithmetic = isAddressArithmetic(Op);
      bool NotExplored = !Explored.count(Op);

      bool Skip = !(NotConstant && NotPHI && AddressArithmetic && NotUniform && NotExplored);
      if (Skip)
        continue;

      FlowMap[Op] = FlowMap[Op] + NumOfUses;
      Explored.insert(Op);
      BFSQ.push(Op);
    }
  }
}

// With OwnedSeeds, an operand with several register operands is only cloned if isOwnedByFlow.
CloneAddressArithmetic::RematChain CloneAddressArithmetic::collectRematChain(llvm::Instruction *I,
                                                                             unsigned int NumOfUsesLimit,
                                                                             const RematSet *OwnedSeeds) {

  RematChain RematVector;
  std::queue<llvm::Instruction *> BFSQ;

  BFSQ.push(I);

  PRINT_LOG("Collect chain for: ");
  PRINT_INST(I);
  PRINT_LOG_NL("");

  bool IsCmp = llvm::isa<CmpInst>(I);
  bool UseOnlyAlwaysProfitable = IsCmp;

  llvm::SmallVector<unsigned int, 4> StateVector;
  std::unordered_set<llvm::Instruction *> Explored;

  // we are traversing ssa-chain for address arithmetic
  while (!BFSQ.empty()) {

    llvm::Instruction *CurrI = BFSQ.front();
    BFSQ.pop();

    for (unsigned int i = 0; i < CurrI->getNumOperands(); ++i) {

      Instruction *Op = llvm::dyn_cast<Instruction>(CurrI->getOperand(i));
      if (!Op)
        continue;

      PRINT_LOG("Candidate: [" << FlowMap[Op] << "] ");
      PRINT_INST(Op);

      bool NotPHI = !llvm::isa<llvm::PHINode>(Op);
      bool NotConstant = !llvm::isa<llvm::Constant>(Op);
      bool SameBB = IGC_IS_FLAG_ENABLED(RematSameBBScope) ? Op->getParent() == I->getParent() : true;
      bool NotUniform = IGC_IS_FLAG_ENABLED(RematRespectUniformity) ? !WI->isUniform(Op) : true;
      bool AddressArithmetic = isAddressArithmetic(Op);
      bool NotTooManyUses = FlowMap[Op] <= NumOfUsesLimit;
      bool NotExplored = !Explored.count(Op);
      bool AlwaysProfitable = isAlwaysProfitableToRemat(Op);
      bool Eligible = SameBB && NotConstant && NotPHI && (NotTooManyUses || AlwaysProfitable) && AddressArithmetic &&
                      NotUniform && NotExplored;
      bool MultipleVariableOps = llvm::count_if(Op->operands(), [](Use &U) { return !isa<Constant>(U.get()); }) > 1;
      bool NotOwned = Eligible && OwnedSeeds && MultipleVariableOps && !isOwnedByFlow(Op, *OwnedSeeds);

      PRINT_LOG("\t\t " << "BB:" << SameBB << "Uses:" << NotTooManyUses << "Ar:" << AddressArithmetic
                        << "Un:" << NotUniform << "Prof:" << AlwaysProfitable << "NotOwn:" << NotOwned);
      bool Skip = !Eligible || NotOwned;
      if (UseOnlyAlwaysProfitable)
        Skip = !(AlwaysProfitable && NotExplored);
      if (Skip) {
        PRINT_LOG_NL("\t\t --> Rejected");
        continue;
      }

      BFSQ.push(Op);
      Explored.insert(Op);
      RematVector.push_back(Op);

      PRINT_LOG_NL("\t\t --> Accepted");
    }
  }

  return RematVector;
}

void CloneAddressArithmetic::addToSystem(RematSet &Set, llvm::Instruction *I) {

  PRINT_LOG_NL("\n");
  PRINT_LOG_NL("Size: " << Vector.size());
  PRINT_LOG("Inst: ");
  PRINT_INST(I);
  PRINT_LOG_NL("");

  for (auto originEl : Set) {
    PRINT_LOG("Set: ");
    PRINT_INST(originEl);
    PRINT_LOG_NL("");
  }

  bool Same = false;
  for (auto &Pair : Vector) {

    auto &ExistingSet = Pair.first;
    auto &ExistingRematVector = Pair.second;

    Same = setCompare(Set, ExistingSet);
    if (Same) {
      PRINT_LOG("found set: ");
      PRINT_INST(I);
      PRINT_LOG_NL("");
      ExistingRematVector.insert(I);
      break;
    }
  }

  if (!Same) {
    llvm::SmallSetVector<llvm::Instruction *, 16> NewSet;
    NewSet.insert(I);
    Vector.push_back(RematPair(Set, NewSet));
  }

  PRINT_LOG_NL("");
}

// Clones I and gives the clone the WIAnalysis dependency of I, if one was computed.
Instruction *CloneAddressArithmetic::cloneWithDependency(Instruction *I) {
  Instruction *Clone = I->clone();
  if (WI->Runner.hasDependency(I))
    WI->incUpdateDepend(Clone, WI->whichDepend(I));
  return Clone;
}

void CloneAddressArithmetic::rematWholeChain(llvm::Instruction *I, RematChain &Chain, IRTransaction &Tx) {

  std::unordered_map<Instruction *, Instruction *> OldToNew;

  for (auto el : Chain) {

    auto Clone = cloneWithDependency(el);
    OldToNew[el] = Clone;
    for (unsigned int i = 0; i < Clone->getNumOperands(); ++i) {

      auto OldOp = llvm::dyn_cast<Instruction>(Clone->getOperand(i));

      if (OldToNew.count(OldOp))
        Clone->setOperand(i, OldToNew[OldOp]);
    }

    MDNode *Node = MDNode::get(I->getContext(), MDString::get(I->getContext(), "remat"));
    Clone->setMetadata("remat", Node);
    Clone->setName("remat");
    IGCLLVM::insertBefore(Clone, I);
    Tx.created(Clone);
  }

  for (unsigned int i = 0; i < I->getNumOperands(); ++i) {
    auto OldOp = dyn_cast<Instruction>(I->getOperand(i));
    if (OldToNew.count(OldOp))
      I->setOperand(i, OldToNew[OldOp]);
  }

  OldToNew.clear();
}

bool CloneAddressArithmetic::skipChain(RematChain &Chain, Instruction *Root) {

  // this is a base flow
  // instructions that have equal flow to origin instruction
  // aren't result in copies, they just moved down
  unsigned int RootFlow = Uses[Root];
  unsigned int InstructionToCopy = 0;

  for (auto &El : Chain)
    if (RootFlow != FlowMap[El])
      InstructionToCopy++;

  bool Result = InstructionToCopy >= m_chainLimit;

  PRINT_LOG_NL("RootFlow: " << RootFlow << "  Limit: " << m_chainLimit << "  Steps: " << InstructionToCopy);
  return Result;
}

bool CloneAddressArithmetic::rematerialize(RematSet &ToProcess, unsigned int FlowThreshold, IRTransaction &Tx,
                                           bool OwnedChainsOnly) {

  for (auto El : ToProcess) {

    PRINT_LOG("rematerialize: ");
    PRINT_INST_NL(El);

    Value *V = El;
    llvm::SmallVector<llvm::Use *, 8> VectorOfUses;
    // collect all uses of particular addrArith inst
    bool ShouldBeRemated = true;
    for (auto &U : V->uses()) {
      ShouldBeRemated &= isSafelyRematerializable(U);
      VectorOfUses.push_back(&U);
    }

    if (!ShouldBeRemated)
      continue;
    PRINT_LOG_NL(" ---> all uses accepted ");

    RematChain Chain = collectRematChain(El, FlowThreshold, OwnedChainsOnly ? &ToProcess : nullptr);
    if (skipChain(Chain, El))
      continue;
    std::reverse(Chain.begin(), Chain.end());

    for (auto Use : VectorOfUses) {

      // take use of addrArith instruction, clone instruction,
      // insert clone right before the use, swap use to clone, remat
      auto User = Use->getUser();
      auto UserInst = llvm::dyn_cast<Instruction>(User);

      if (!UserInst)
        continue;

      PRINT_LOG("remat: ");
      PRINT_INST(User);
      PRINT_LOG(" --> ");

      auto Clone = cloneWithDependency(El);
      MDNode *Node = MDNode::get(El->getContext(), MDString::get(El->getContext(), "remat"));
      Clone->setMetadata("remat", Node);
      Clone->setName("cloned_" + El->getName());
      IGCLLVM::insertBefore(Clone, UserInst);
      Tx.created(Clone);
      Tx.replaceUse(*Use, Clone);

      PRINT_INST_NL(Clone);
      rematWholeChain(Clone, Chain, Tx);
    }

    Tx.replaced(El);

    PRINT_LOG_NL("");
  }
  return true;
}

void CloneAddressArithmetic::estimateProfit(RematSet &ToProcess) {

  const DataLayout &DL = Func->getParent()->getDataLayout();

  PRINT_LOG_NL("FINAL: ");
  PRINT_LOG_NL("SIZE: " << Vector.size());

  auto Comp = [](RematPair &A, RematPair &B) { return A.second.size() > B.second.size(); };
  std::sort(Vector.begin(), Vector.end(), Comp);

  for (const auto &el : Vector) {

    auto &OriginSet = el.first;
    auto &ValueSet = el.second;

    unsigned int SetSize = ValueSet.size();
    unsigned int OriginSetSize = OriginSet.size();
    unsigned int OriginRegSize = RPE->bytesToRegisters(RPE->estimateSizeInBytes(OriginSet, *Func, SIMD, &WI->Runner));
    PRINT_DS("Origin node: ", OriginSet);

    unsigned int ValueRegSize = RPE->bytesToRegisters(RPE->estimateSizeInBytes(ValueSet, *Func, SIMD, &WI->Runner));
    for (auto vecEl : ValueSet) {
      unsigned int RegSize = RPE->bytesToRegisters(RPE->computeSizeInBytes(vecEl, SIMD, &WI->Runner, DL));
      PRINT_LOG("uses: " << Uses[vecEl] << " size: " << RegSize);
      PRINT_INST_NL(vecEl);
    }
    PRINT_LOG_NL("ValueSetSize: " << SetSize << " ValueSet Reg_Size: " << ValueRegSize);
    PRINT_LOG_NL("OriginSetSize: " << OriginSetSize << " OriginSet Reg_Size: " << OriginRegSize);

    // if origin values (values that are going to be used to recompute) weigh more
    // than what we're rematting we should not
    // check cloneAddressArithmetic-not-profitable.ll test for reference
    if (ValueRegSize < OriginRegSize) {
      PRINT_LOG_NL("Not profitable");
      for (auto vecEl : ValueSet)
        if (ToProcess.count(vecEl))
          ToProcess.remove(vecEl);
    }
    PRINT_LOG_NL("------");
  }

  return;
}

// Pressure a stage has to stay within to be kept, 0 when no such target applies to the function.
// On XE2 an OpenCL kernel whose SIMD32 pressure is above the GRF budget is dropped to SIMD16 before codegen, so
// rematerialization helps if it brings the pressure back under that budget, or if it lets the SIMD16 kernel fit its
// GRF number (see commitIfBudgetReached). On platforms with VRT a kernel above the largest GRF mode cannot get more
// registers, so it only helps if it brings the pressure under that mode.
unsigned int CloneAddressArithmetic::pressureBudget(Function &F) {
  if (!m_firstCompile)
    return IGC_IS_FLAG_ENABLED(RematRetryRollback) ? retryPressureBudget(F) : 0;
  if (SIMD == numLanes(SIMDMode::SIMD32) && !IGC::hasFixedSIMDSize(CGCtx, &F, FGA)) {
    unsigned int Budget = IGC::getXe2SIMD16DropBudget(CGCtx, &F);
    if (Budget && MaxPressure > Budget)
      return Budget;
  }
  auto VRTTable = CGCtx->platform.getVRTTable(/*SelectableOnly=*/true);
  if (!VRTTable.empty() && (int)MaxPressure > VRTTable.back().first)
    return VRTTable.back().first;
  return 0;
}

// A retry stage must not raise the pressure past the GRF count the kernel gets, or past the end of its current
// VRT thread band, unless the pressure is already beyond it.
unsigned int CloneAddressArithmetic::retryPressureBudget(Function &F) {
  unsigned int GRFBudget = CGCtx->getNumGRFPerThread(true, &F);
  if (CGCtx->isAutoGRFSelectionEnabled(&F) && CGCtx->getNumGRFPerThread(false, &F) == 0) {
    auto Modes = CGCtx->getAutoGRFVRTTable();
    if (!Modes.empty())
      GRFBudget = findVRTBand(Modes, MaxPressure)->first;
  }
  return std::max(MaxPressure, GRFBudget);
}

// True if Op stays dead after its clone, i.e. all users are seeds or carry flow of the stage. Clones of earlier
// chains are outside the flow, as they keep Op live.
bool CloneAddressArithmetic::isOwnedByFlow(Instruction *Op, const RematSet &Seeds) {
  return llvm::all_of(Op->users(), [&](User *U) {
    auto *UI = dyn_cast<Instruction>(U);
    if (!UI || Seeds.count(UI) || isa<DbgInfoIntrinsic>(UI))
      return true;
    auto It = FlowMap.find(UI);
    return It != FlowMap.end() && It->second != 0;
  });
}

// Pressure of the function as it is now.
unsigned int CloneAddressArithmetic::measurePressure(Function &F) {
  RPE->rerunLivenessAnalysis(F);
  return RPE->getMaxRegCountForFunction(F, SIMD, &WI->Runner);
}

// On XE2 a kernel that stays over the SIMD16 drop budget is compiled at SIMD16. Returns the SIMD16 pressure of the
// function as it is now when that applies to a first-compile stage with the given budget, otherwise 0.
unsigned int CloneAddressArithmetic::simd16DropPressure(Function &F, unsigned int Budget) {
  if (!m_firstCompile || !Budget || SIMD != numLanes(SIMDMode::SIMD32) ||
      Budget != IGC::getXe2SIMD16DropBudget(CGCtx, &F))
    return 0;
  return RPE->getMaxRegCountForFunction(F, numLanes(SIMDMode::SIMD16), &WI->Runner);
}

// The XE3 VRT SIMD16 drop picks SIMD16 when SIMD16 pressure is low, even if SIMD32 would fit. A fixed SIMD32 width
// is never dropped. Checked on the current IR, for a stage that estimates pressure at SIMD32.
bool CloneAddressArithmetic::prefersSIMD16OnVRT(Function &F) {
  if (SIMD != numLanes(SIMDMode::SIMD32) || !CGCtx->supportsVRT() || IGC_IS_FLAG_DISABLED(AllowSIMD16DropForXE2Plus) ||
      !CGCtx->isAutoGRFSelectionEnabled(&F) || CGCtx->getNumGRFPerThread(false, &F) != 0 ||
      IGC::hasFixedSIMDSize(CGCtx, &F, FGA))
    return false;
  unsigned int Simd16Pressure = RPE->getMaxRegCountForFunction(F, numLanes(SIMDMode::SIMD16), &WI->Runner);
  unsigned int Simd32Pressure = RPE->getMaxRegCountForFunction(F, SIMD, &WI->Runner);
  PRINT_LOG_NL("SIMD16 pressure: " << Simd16Pressure << " SIMD32 pressure: " << Simd32Pressure);
  return IGC::prefersSIMD16OnXE3VRT(CGCtx, &F, Simd16Pressure, Simd32Pressure);
}

// The static profit estimate assumes every cloned value is live at the pressure peak and is far too optimistic to
// predict whether a budget is reached, so the pressure of the rematerialized function is measured instead.
// Rematerialization that leaves it over the budget is undone: the clones would only add instructions. The exception
// is the XE2 SIMD16 drop: the kernel then compiles at SIMD16, and the clones are kept when they bring SIMD16 pressure
// (SIMD16Pressure, measured before rematerialization) from over the kernel's SIMD16 GRF number to within it, since
// without them the SIMD16 compile would spill. First-compile rematerialization at SIMD32 that lowers the pressure
// enough for the XE3 VRT SIMD16 drop is undone too: the kernel would not keep SIMD32, and the clones would only shrink
// the SIMD16 register allocation. Otherwise the transaction is committed.
void CloneAddressArithmetic::commitIfBudgetReached(Function &F, IRTransaction &Remat, unsigned int Budget,
                                                   unsigned int SIMD16Pressure) {
  const unsigned int Pressure = measurePressure(F);
  PRINT_LOG_NL("Pressure budget: " << Budget << " MaxPressure: " << MaxPressure << " -> " << Pressure);
  bool OverBudget = Budget && Pressure > Budget;
  const unsigned int SIMD16NumGRF = SIMD16Pressure ? IGC::getXe2SIMD16NumGRF(CGCtx, &F) : 0;
  if (OverBudget && SIMD16Pressure > SIMD16NumGRF) {
    const unsigned int NewSIMD16Pressure = simd16DropPressure(F, Budget);
    PRINT_LOG_NL("SIMD16 GRF number: " << SIMD16NumGRF << " SIMD16 pressure: " << SIMD16Pressure << " -> "
                                       << NewSIMD16Pressure);
    OverBudget = NewSIMD16Pressure > SIMD16NumGRF;
  }
  if (OverBudget || (m_firstCompile && prefersSIMD16OnVRT(F))) {
    PRINT_LOG_NL((OverBudget ? "Budget not reached" : "SIMD16 drop after rematerialization") << ", rolling back");
    Remat.rollback();
    RPE->rerunLivenessAnalysis(F);
    return;
  }
  Remat.commit();
}

// Keeps the remat only if the measured pressure is within pressureBudget, or the XE2 SIMD16 drop exception of
// commitIfBudgetReached applies; liveness is up to date afterwards.
// The caller must report a change even after a rollback to invalidate WIAnalysis entries of the erased clones.
void CloneAddressArithmetic::rematerializeWithinBudget(Function &F, RematSet &ToProcess, unsigned int FlowThreshold,
                                                       bool OwnedChainsOnly) {
  const unsigned int Budget = pressureBudget(F);
  const unsigned int SIMD16Pressure = simd16DropPressure(F, Budget);
  IRTransaction Remat;
  rematerialize(ToProcess, FlowThreshold, Remat, OwnedChainsOnly);
  Remat.detachDead();
  commitIfBudgetReached(F, Remat, Budget, SIMD16Pressure);
}

void CloneAddressArithmetic::speculateWholeChain(RematSet &ToProcess, unsigned int UsesLimit, bool OwnedChainsOnly) {

  PRINT_LOG_NL("speculate, FlowThreshold:" << UsesLimit);
  for (auto I : ToProcess) {

    RematChain Chain = collectRematChain(I, UsesLimit, OwnedChainsOnly ? &ToProcess : nullptr);
    RematSet Set;

    addToSetRemat(I, Set);
    for (auto &el : Chain) {
      Set.remove(el);
      PRINT_LOG("[" << FlowMap[el] << "] ");
      PRINT_INST_NL(el);
      addToSetRemat(el, Set);
    }

    for (auto *el : Set) {
      PRINT_LOG("origin: ");
      PRINT_INST(el);
      PRINT_LOG_NL("");
    }

    addToSystem(Set, I);
  }

  estimateProfit(ToProcess);
  PRINT_LOG_NL("end_speculate");
  return;
}

static bool shouldRematForVRT(llvm::ArrayRef<VRT::ModeEntry> Table, uint32_t MaxPressure, uint32_t ProximityPercent) {
  if (Table.empty())
    return false;

  if ((int)MaxPressure > Table.back().first)
    return true;

  auto Cur = findVRTBand(Table, MaxPressure);

  // Walk down to the nearest band that grants more threads and check whether it
  // is within reach.
  for (auto It = Cur; It != Table.begin();) {
    --It;
    if (It->second == Cur->second)
      continue;
    int TargetBudget = It->first;
    uint32_t Threshold = TargetBudget + (uint32_t)TargetBudget * ProximityPercent / 100;
    return MaxPressure <= Threshold;
  }

  return false;
}

bool CloneAddressArithmetic::isRegPressureLow(Function &F) {

  RPE = &getAnalysis<IGCLivenessAnalysis>().getLivenessRunner();
  SIMD = numLanes(m_getSIMDSize(CGCtx, &F, FGA));
  // Rematerializing at SIMD32 would only undo the XE3 VRT SIMD16 drop.
  if (prefersSIMD16OnVRT(F))
    SIMD = numLanes(SIMDMode::SIMD16);
  unsigned int GRFSize = CGCtx->getNumGRFPerThread(true, &F);
  unsigned int PressureLimit = 0.01f * (float)m_rpeLimitPercent * (float)GRFSize;
  MaxPressure = RPE->getMaxRegCountForFunction(F, SIMD, &WI->Runner);

  auto VRTTable = CGCtx->platform.getVRTTable(/*SelectableOnly=*/true);
  if (!VRTTable.empty()) {
    unsigned int Proximity = IGC_GET_FLAG_VALUE(RematVRTProximityPercent);
    bool ShouldRemat = shouldRematForVRT(VRTTable, MaxPressure, Proximity);
    PRINT_LOG_NL("VRT remat decision: SIMD: " << SIMD << " MaxPressure: " << MaxPressure
                                              << " ProximityPercent: " << Proximity << " ShouldRemat: " << ShouldRemat);
    writeLog();
    return !ShouldRemat;
  }

  bool Result = MaxPressure < PressureLimit;
  PRINT_LOG_NL("SIMD: " << SIMD << " MaxPressure: " << MaxPressure << " PressureLimit: " << PressureLimit);
  writeLog();
  return Result;
}

void CloneAddressArithmetic::countUses(Function &F) {
  for (BasicBlock &BB : F) {
    for (auto &I : BB) {
      unsigned int NonDebugUses = 0;
      for (auto U : I.users()) {
        if (!llvm::isa<DbgInfoIntrinsic>(U))
          NonDebugUses += 1;
      }
      Uses[&I] = NonDebugUses;
    }
  }
}

bool isRematInstruction(llvm::Value *V) {

  bool IntToPtr = llvm::isa<IntToPtrInst>(V);
  bool AddrSpCast = llvm::isa<AddrSpaceCastInst>(V);
  // use only bitcasts on pointers as a seed instruction
  bool BitCast = llvm::isa<BitCastInst>(V) && V->getType()->isPointerTy();
  bool GEP = llvm::isa<GetElementPtrInst>(V);

  bool Result = IntToPtr || AddrSpCast || BitCast || GEP;
  return Result;
}

void CloneAddressArithmetic::collectInstToProcess(RematSet &ToProcess, Function &F) {
  for (BasicBlock &BB : F) {
    for (auto &I : BB) {

      bool IsLoad = llvm::isa<LoadInst>(I) && m_rematFlags & REMAT_LOADS;
      bool IsStore = llvm::isa<StoreInst>(I) && m_rematFlags & REMAT_STORES;
      bool IsCall = llvm::isa<CallInst>(I) && m_rematFlags & REMAT_ARGS;
      bool IsCmp = llvm::isa<CmpInst>(I) && m_rematFlags & REMAT_COMPARISONS;
      bool AllowDataFlowRematerialization = m_rematFlags & REMAT_DATAFLOW;

      if (!IsLoad && !IsStore && !IsCall && !IsCmp)
        continue;

      if (IsCmp) {
        ToProcess.insert(static_cast<Instruction *>(&I));
        continue;
      }

      if (IsCall) {
        for (auto &Arg : cast<CallInst>(I).args()) {
          if (isRematInstruction(Arg)) {
            ToProcess.insert(cast<Instruction>(&Arg));
          }
        }

        continue;
      }

      llvm::Value *V =
          IsLoad ? static_cast<LoadInst *>(&I)->getPointerOperand() : static_cast<StoreInst *>(&I)->getPointerOperand();

      if (isRematInstruction(V))
        ToProcess.insert(static_cast<Instruction *>(V));

      if (AllowDataFlowRematerialization && IsStore) {
        llvm::Instruction *Inst = llvm::dyn_cast<llvm::Instruction>(static_cast<StoreInst *>(&I)->getOperand(0));
        if (Inst && isAddressArithmetic(Inst))
          ToProcess.insert(static_cast<Instruction *>(Inst));
      }
    }
  }
}

void CloneAddressArithmetic::computeFlow(RematSet &ToProcess) {

  for (auto el : ToProcess) {
    PRINT_LOG("Start to compute flow: ");
    PRINT_INST_NL(el);
    computeFlow((Instruction *)el);
  }

  if (DEBUG) {
    for (const auto &el : FlowMap) {
      PRINT_LOG("[" << el.second << "] {" << Uses[el.first] << "}\t");
      PRINT_INST_NL(el.first);
    }
  }
}

unsigned int CloneAddressArithmetic::getFlowThreshold(RematSet &ToProcess, Function &F) {

  unsigned int FlowBudget = 0;
  for (auto el : ToProcess)
    FlowBudget += Uses[el];

  PRINT_LOG_NL("FlowBudget: " << FlowBudget);
  float Coefficient = 0.01f * (float)m_flowThresholdPercent;
  unsigned int Result = (unsigned int)((float)FlowBudget * Coefficient);

  return Result;
}

// this brings instructions that are uniquely used
// by rematerialization target closer to the use
// add %3, %2, %1 (this wont be touched it has more than 1 flow -> it's used in more targets that one)
// add %4, %3, %1 (this instruction will be put closer to the store)
// add %5, %3, %2 (this as well)
// ....
// store %4 -> %ptr0
// ....
// store %5 -> %ptr1
//
// AFTER:
// add %3, %2, %1
// .....
// add %remat4, %3, %1
// store %remat4 -> %ptr0
// ....
// add %remat5, %3, %2
// store %remat5 -> %ptr1
bool CloneAddressArithmetic::singleFlowRemat(Function &F) {

  PRINT_LOG_NL("Single Flow Remat");
  if (isRegPressureLow(F))
    return false;

  countUses(F);

  RematSet ToProcess;
  collectInstToProcess(ToProcess, F);

  if (ToProcess.empty())
    return false;

  computeFlow(ToProcess);
  writeLog();

  const bool OwnedChainsOnly = IGC_IS_FLAG_ENABLED(RematSingleFlowOwnedChains);
  speculateWholeChain(ToProcess, 1, OwnedChainsOnly);
  writeLog();

  rematerializeWithinBudget(F, ToProcess, 1, OwnedChainsOnly);
  writeLog();

  FlowMap.clear();
  Vector.clear();
  return true;
}

bool CloneAddressArithmetic::greedyRemat(Function &F) {

  PRINT_LOG_NL("Greedy Remat");
  if (isRegPressureLow(F))
    return false;

  countUses(F);

  RematSet ToProcess;
  collectInstToProcess(ToProcess, F);

  if (ToProcess.empty())
    return false;

  unsigned int FlowThreshold = getFlowThreshold(ToProcess, F);
  computeFlow(ToProcess);
  writeLog();

  if (DEBUG) {
    speculateWholeChain(ToProcess, FlowThreshold, false);
    writeLog();
  }

  rematerializeWithinBudget(F, ToProcess, FlowThreshold, false);
  writeLog();

  FlowMap.clear();
  Vector.clear();
  return true;
}

void CloneAddressArithmetic::writeLog() {

  if (IGC_IS_FLAG_ENABLED(RematLog) && IGC_IS_FLAG_DISABLED(RematLogToErr) && OutputLogFile->is_open())
    *OutputLogFile << OutputLogStream.str();

  if (IGC_IS_FLAG_ENABLED(RematLog) && IGC_IS_FLAG_ENABLED(RematLogToErr))
    llvm::errs() << OutputLogStream.str();

  OutputLogStream.str().clear();
}

void CloneAddressArithmetic::initializeLogFile(Function &F) {

  if (!IGC_IS_FLAG_ENABLED(RematLog))
    return;

  std::stringstream ss;
  ss << F.getName().str() << "_" << "Remat";
  auto Name = Debug::DumpName(IGC::Debug::GetShaderOutputName())
                  .Hash(CGCtx->hash)
                  .Type(CGCtx->type)
                  .Retry(CGCtx->m_retryManager->GetRetryId())
                  .Pass(ss.str().c_str())
                  .Extension("ll");

  OutputLogFile = std::make_unique<std::ofstream>(Name.str());
}

bool CloneAddressArithmetic::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  if (m_rematFlags == REMAT_NONE)
    return false;

  Func = &F;
  CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  WI = &getAnalysis<WIAnalysis>();
  FGA = getAnalysisIfAvailable<GenXFunctionGroupAnalysis>();

  initializeLogFile(F);

  auto OriginalRematFlags = m_rematFlags;
  bool Modified = false;

  // The single flow stage only runs on retry.
  m_rematFlags = static_cast<IGC::REMAT_OPTIONS>(m_rematFlags & ~REMAT_COMPARISONS);
  Modified |= !m_firstCompile && IGC_IS_FLAG_ENABLED(RematSingleFlowRematEnabled) && singleFlowRemat(F);

  // we only enable DATA_FLOW for first stage and we allow CMP remat back if it was enabled
  m_rematFlags = OriginalRematFlags;
  m_rematFlags = static_cast<IGC::REMAT_OPTIONS>(m_rematFlags & ~REMAT_DATAFLOW);

  Modified |= greedyRemat(F);
  return Modified;
}

FunctionPass *IGC::createRematAddressArithmeticPass() { return new RematAddressArithmetic(); }

char RematAddressArithmetic::ID = 0;

#define PASS_FLAG "igc-remat-address-arithmetic"
#define PASS_DESC "Remat Address Arithmetic"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
IGC_INITIALIZE_PASS_BEGIN(RematAddressArithmetic, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(RematAddressArithmetic, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
} // namespace IGC

bool RematAddressArithmetic::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  bool modified = false;
  modified |= rematerializePhiMemoryAddressCalculation(F);
  modified |= rematerializePrivateMemoryAddressCalculation(F);
  return modified;
}

// Compares if two instructions are of the same kind, have the same return
// type and the same types of operands.
template <typename InstT> static inline bool CompareInst(Value *a, Value *b) {
  if (a == nullptr || b == nullptr || a->getType() != b->getType() || !isa<InstT>(a) || !isa<InstT>(b)) {
    return false;
  }
  if (isa<Instruction>(a)) {
    // For instructions also check opcode and operand types
    InstT *instA = cast<InstT>(a);
    InstT *instB = cast<InstT>(b);
    if (instA->getOpcode() != instB->getOpcode()) {
      return false;
    }
    for (uint i = 0; i < instA->getNumOperands(); ++i) {
      if (instA->getOperand(i)->getType() != instB->getOperand(i)->getType()) {
        return false;
      }
    }
  }
  return true;
}

// Rematerialize address calculations if address is a Phi instruction and all
// incoming values are results of identical address calculations, e.g.:
//
// true-bb:
//   %addrTrue = add i64 %base, 4
//   %ptrTrue  = inttoptr i64 %addrTrue to i64 addrspace(2)*
//   br label %merge-bb
//
// false-bb:
//   %addrFalse = add i64 %base, 4
//   %ptrFalse  = inttoptr i64 %addrFalse to i64 addrspace(2)*
//   br label %merge-bb
//
// merge-bb:
//   %addr = phi i64 addrspace(2)* [ %ptrTrue, %true-bb ], [ %ptrFalse, %false-bb ]
//   %result = load i64, i64 addrspace(2)* %addr, align 4
//
// Such "diamond-like" pattern can be created by GVN.
//
// The goal of the optimization is to potentially make the final memory
// operation uniform. Note that it many cases it would also be possible
// to hoist address calculations to the dominator basic block instead
// of rematerialization but hoisting could increase register pressure.
bool RematAddressArithmetic::rematerializePhiMemoryAddressCalculation(Function &F) {
  bool modified = false;
  auto PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
  // Process all basic blocks in postdominator tree breadth first traversal.
  for (auto domIter = bf_begin(PDT->getRootNode()), domEnd = bf_end(PDT->getRootNode()); domIter != domEnd; ++domIter) {
    BasicBlock *BB = domIter->getBlock();
    if (BB == nullptr) {
      continue;
    }
    for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II) {
      PHINode *phi = dyn_cast<PHINode>(&*II);
      if (!phi) {
        // No more Phi nodes in this BB, go to the next BB
        break;
      }
      if (!phi->getType()->isPointerTy() || phi->hasNUses(0)) {
        // Not an address, go to the next Phi
        continue;
      }
      bool doRemat = true;
      // For all incoming values compare the address calculations in
      // predecessors.
      for (uint i = 0; i < phi->getNumIncomingValues(); ++i) {
        // Current implementation only detects the inttoptr + add
        // pattern, e.g.:
        //   %offset = add i64 %2, 168
        //   %ptr = inttoptr i64 %offset to i64 addrspace(2)*
        Value *first = phi->getIncomingValue(0);
        Value *other = phi->getIncomingValue(i);
        if (!CompareInst<IntToPtrInst>(first, other)) {
          doRemat = false;
          break;
        }
        first = cast<IntToPtrInst>(first)->getOperand(0);
        other = cast<IntToPtrInst>(other)->getOperand(0);
        if (!CompareInst<BinaryOperator>(first, other)) {
          doRemat = false;
          break;
        }
        BinaryOperator *firstBinOp = cast<BinaryOperator>(first);
        BinaryOperator *otherBinOp = cast<BinaryOperator>(other);
        if (firstBinOp->getOpcode() != Instruction::Add || firstBinOp->getOperand(0) != otherBinOp->getOperand(0) ||
            firstBinOp->getOperand(1) != otherBinOp->getOperand(1)) {
          doRemat = false;
          break;
        }
      }
      if (doRemat) {
        IntToPtrInst *intToPtr = cast<IntToPtrInst>(phi->getIncomingValue(0));
        BinaryOperator *add = cast<BinaryOperator>(intToPtr->getOperand(0));
        // Clone address computations
        Instruction *newAdd = add->clone();
        Instruction *newIntToPtr = intToPtr->clone();
        newIntToPtr->setOperand(0, newAdd);
        // and insert in after the phi
        Instruction *insertPoint = IGCLLVM::getFirstNonPHIOrDbgOrLifetime(BB);
        IGCLLVM::insertBefore(newAdd, insertPoint);
        IGCLLVM::insertBefore(newIntToPtr, insertPoint);
        phi->replaceAllUsesWith(newIntToPtr);
        modified = true;
      }
    }
  }
  return modified;
}

bool RematAddressArithmetic::rematerializePrivateMemoryAddressCalculation(Function &F) {
  bool changed = false;
  Value *PrivateBase = getPrivateMemoryValue(F);
  if (PrivateBase == nullptr)
    return false;

  DenseMap<Value *, SmallVector<Instruction *, 4>> BaseMap;
  SmallVector<std::pair<Instruction *, IntToPtrInst *>, 32> PointerList;

  SmallVector<std::pair<Value *, Value *>, 16> WorkList;
  WorkList.push_back(std::make_pair(PrivateBase, nullptr));
  while (!WorkList.empty()) {
    auto [V, U] = WorkList.back();
    WorkList.pop_back();

    if (auto Ptr = dyn_cast<IntToPtrInst>(V)) {
      BaseMap[U].push_back(Ptr);
      continue;
    }

    for (User *US : V->users()) {
      // Don't add to chain of uses if it is PHINode
      if (isa<PHINode>(US))
        continue;
      WorkList.push_back(std::make_pair(US, V));
    }
  }

  DenseMap<Value *, SmallVector<Value *, 16>> CommonBaseMap;
  DenseMap<Value *, SmallVector<Value *, 4>> UseChain;
  for (auto &BM : BaseMap) {
    Value *Base = BM.first;
    auto &BaseUsers = BM.second;

    auto BO = dyn_cast<BinaryOperator>(Base);
    if (BO == nullptr)
      continue;

    if (isa<ConstantInt>(BO->getOperand(1))) {
      for (auto U : BaseUsers) {
        if (BO->getParent() != U->getParent()) {
          CommonBaseMap[BO->getOperand(0)].push_back(U);
          UseChain[U].push_back(BO);
        }
      }
    }
  }

  for (auto &CB : CommonBaseMap) {
    if (CB.second.size() < 2)
      continue;

    changed = true;
    for (auto V : CB.second) {
      auto I = dyn_cast<Instruction>(V);
      IGC_ASSERT(I != nullptr);
      rematerialize(I, UseChain[I]);
    }
  }
  return changed;
}

bool RematAddressArithmetic::rematerialize(Instruction *I, SmallVectorImpl<Value *> &Chain) {
  Value *CurV = I;
  for (auto *V : Chain) {
    Instruction *Clone = dyn_cast<Instruction>(V)->clone();
    IGCLLVM::insertBefore(Clone, dyn_cast<Instruction>(CurV));
    for (auto &U : V->uses()) {
      if (CurV == U.getUser())
        U.set(Clone);
    }
    CurV = V;
  }
  return true;
}

static Value *getPrivateMemoryValue(Function &F) {
  Value *PrivateBase = nullptr;
  for (auto AI = F.arg_begin(), AE = F.arg_end(); AI != AE; ++AI) {
    if (!AI->hasName())
      continue;
    auto Name = AI->getName().str();
    if (Name == "privateBase" && !AI->use_empty())
      PrivateBase = &*AI;
  }
  return PrivateBase;
}
