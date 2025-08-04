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
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/IGCLivenessAnalysis.h"
#include <fstream>

using namespace llvm;
using namespace IGC;

static Value *getPrivateMemoryValue(Function &F);

namespace {

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
  CloneAddressArithmetic() : FunctionPass(ID) {
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
  IGCLivenessAnalysis *RPE = nullptr;

private:
  bool greedyRemat(Function &F);
  bool rematerialize(RematSet &ToProcess, unsigned int FlowThreshold);
  bool isRegPressureLow(Function &F);
  bool skipChain(RematChain &Chain, Instruction *Root);

  RematChain collectRematChain(llvm::Instruction *I, unsigned int NumOfUsesLimit);

  unsigned int collectFlow(RematSet &ToProcess, Function &F);

  void countUses(Function &);
  void speculateWholeChain(RematSet &ToProcess, unsigned int UsesLimit);
  void collectInstToProcess(RematSet &ToProcess, Function &F);
  void addToSystem(CloneAddressArithmetic::RematSet &Set, llvm::Instruction *I);
  void computeFlow(llvm::Instruction *I);
  void rematWholeChain(llvm::Instruction *I, RematChain &Chain);
  void estimateProfit(RematSet &ToProcess);
  void initializeLogFile(Function &F);
  void writeLog();
};
} // end namespace

FunctionPass *IGC::createCloneAddressArithmeticPass() { return new CloneAddressArithmetic(); }

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
  auto CI = IGC_IS_FLAG_ENABLED(RematAddrSpaceCastToUse) ? llvm::isa<AddrSpaceCastInst>(Use.getUser()) : false;
  // TODO: move to whitelist option
  // sometimes it helps to rematerialize arguments for llvm.debug functions in general it's not safe.
  // this is not airtight, use only for testing purposes, if performance gains are significant it should be
  // investigated. visa can exhibit strange behavior sometimes
  auto CLI = IGC_IS_FLAG_ENABLED(RematCallsOperand) ? llvm::isa<CallInst>(Use.getUser()) : false;

  bool Result = LI || SI || BI || CI || CLI || SelI;
  return Result;
}

static bool isAddressArithmetic(Instruction *I) {
  bool Result = isa<GetElementPtrInst>(I) || isa<InsertElementInst>(I) || isa<InsertValueInst>(I) ||
                isa<BinaryOperator>(I) || isa<AddrSpaceCastInst>(I) || isa<SelectInst>(I) || isa<CastInst>(I) ||
                (isa<UnaryInstruction>(I) && !isa<LoadInst>(I)) ||
                (IGC_GET_FLAG_VALUE(RematAllowLoads) && isa<LoadInst>(I)) ||
                (IGC_GET_FLAG_VALUE(RematAllowOneUseLoad) && isa<LoadInst>(I) && I->hasOneUse()) ||
                (IGC_GET_FLAG_VALUE(RematAllowExtractElement) && isa<ExtractElementInst>(I));

  return Result;
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

CloneAddressArithmetic::RematChain CloneAddressArithmetic::collectRematChain(llvm::Instruction *I,
                                                                             unsigned int NumOfUsesLimit) {

  RematChain RematVector;
  std::queue<llvm::Instruction *> BFSQ;

  BFSQ.push(I);

  PRINT_LOG("Collect chain for: ");
  PRINT_INST(I);
  PRINT_LOG_NL("");

  llvm::SmallVector<unsigned int, 4> StateVector;
  std::unordered_set<llvm::Instruction *> Explored;

  // we are travdrsing ssa-chain for address arithmetic
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

      PRINT_LOG("\t\t " << "BB:" << SameBB << "Uses:" << NotTooManyUses << "Ar:" << AddressArithmetic
                        << "Un:" << NotUniform);
      bool Skip =
          !(SameBB && NotConstant && NotPHI && NotTooManyUses && AddressArithmetic && NotUniform && NotExplored);
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

void CloneAddressArithmetic::rematWholeChain(llvm::Instruction *I, RematChain &Chain) {

  std::unordered_map<Instruction *, Instruction *> OldToNew;

  for (auto el : Chain) {

    auto Clone = el->clone();
    OldToNew[el] = Clone;
    for (unsigned int i = 0; i < Clone->getNumOperands(); ++i) {

      auto OldOp = llvm::dyn_cast<Instruction>(Clone->getOperand(i));

      if (OldToNew.count(OldOp))
        Clone->setOperand(i, OldToNew[OldOp]);
    }

    MDNode *Node = MDNode::get(I->getContext(), MDString::get(I->getContext(), "remat"));
    Clone->setMetadata("remat", Node);
    Clone->setName("remat");
    Clone->insertBefore(I);
  }

  auto OldOp = dyn_cast<Instruction>(I->getOperand(0));
  if (OldToNew.count(OldOp))
    I->setOperand(0, OldToNew[OldOp]);

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

  const unsigned RematChainLimit = IGC_GET_FLAG_VALUE(RematChainLimit);
  bool Result = InstructionToCopy >= RematChainLimit;

  PRINT_LOG_NL("RootFlow: " << RootFlow << "  Limit: " << RematChainLimit << "  Steps: " << InstructionToCopy);
  return Result;
}

bool CloneAddressArithmetic::rematerialize(RematSet &ToProcess, unsigned int FlowThreshold) {

  for (auto El : ToProcess) {

    PRINT_LOG("rematerialize: ");
    PRINT_INST(El);

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

    RematChain Chain = collectRematChain(El, FlowThreshold);
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

      auto Clone = El->clone();
      MDNode *Node = MDNode::get(El->getContext(), MDString::get(El->getContext(), "remat"));
      Clone->setMetadata("remat", Node);
      Clone->setName("cloned_" + El->getName());
      Clone->insertBefore(UserInst);
      *Use = Clone;

      PRINT_INST_NL(Clone);
      rematWholeChain(Clone, Chain);
    }
    PRINT_LOG_NL("");
  }
  return true;
}

void CloneAddressArithmetic::estimateProfit(RematSet &ToProcess) {

  if (!DEBUG)
    return;

  PRINT_LOG_NL("FINAL: ");
  PRINT_LOG_NL("SIZE: " << Vector.size());
  for (const auto &el : Vector) {

    auto &OriginSet = el.first;
    auto &ValueSet = el.second;

    unsigned int SetSize = ValueSet.size();
    PRINT_LOG_NL("SetSize: " << SetSize);

    PRINT_LOG_NL("origin nodes:");
    for (auto originEl : OriginSet) {
      PRINT_INST_NL(originEl);
    }

    PRINT_LOG_NL("------");
    for (auto vecEl : ValueSet) {
      PRINT_LOG("uses: " << Uses[vecEl] << " ");
      PRINT_INST_NL(vecEl);
    }
  }

  return;
}

void CloneAddressArithmetic::speculateWholeChain(RematSet &ToProcess, unsigned int UsesLimit) {

  PRINT_LOG_NL("speculate, FlowThreshold:" << UsesLimit);
  for (auto I : ToProcess) {

    RematChain Chain = collectRematChain(I, UsesLimit);
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
  Vector.clear();
  return;
}

bool CloneAddressArithmetic::isRegPressureLow(Function &F) {

  RPE = &getAnalysis<IGCLivenessAnalysis>();
  unsigned int SIMD = numLanes(RPE->bestGuessSIMDSize(&F));
  unsigned int PressureLimit = IGC_GET_FLAG_VALUE(RematRPELimit);
  unsigned int MaxPressure = RPE->getMaxRegCountForFunction(F, SIMD, &WI->Runner);
  bool Result = MaxPressure < PressureLimit;
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

      bool IsLoad = llvm::isa<LoadInst>(I);
      bool IsStore = llvm::isa<StoreInst>(I);
      bool IsCall = llvm::isa<CallInst>(I);

      if (!IsLoad && !IsStore && !IsCall)
        continue;

      llvm::Value *V =
          IsLoad ? static_cast<LoadInst *>(&I)->getPointerOperand() : static_cast<StoreInst *>(&I)->getPointerOperand();

      if (isRematInstruction(V))
        ToProcess.insert(static_cast<Instruction *>(V));

      if (IsCall && IGC_IS_FLAG_ENABLED(RematCollectCallArgs)) {
        for (auto &Arg : cast<CallInst>(I).args()) {
          if (isRematInstruction(Arg)) {
            ToProcess.insert(cast<Instruction>(&Arg));
          }
        }
      }
    }
  }
}

unsigned int CloneAddressArithmetic::collectFlow(RematSet &ToProcess, Function &F) {

  unsigned int FlowBudget = 0;
  for (auto el : ToProcess)
    FlowBudget += Uses[el];

  PRINT_LOG_NL("FlowBudget: " << FlowBudget);
  unsigned int Base = IGC_GET_FLAG_VALUE(RematFlowThreshold);
  float Coefficient = 0.01f * (float)Base;
  unsigned int Result = (unsigned int)((float)FlowBudget * Coefficient);

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

  return Result;
}

bool CloneAddressArithmetic::greedyRemat(Function &F) {

  if (isRegPressureLow(F))
    return false;

  initializeLogFile(F);
  countUses(F);

  RematSet ToProcess;
  collectInstToProcess(ToProcess, F);

  unsigned int FlowThreshold = collectFlow(ToProcess, F);
  writeLog();

  speculateWholeChain(ToProcess, FlowThreshold);
  writeLog();

  rematerialize(ToProcess, FlowThreshold);
  writeLog();

  FlowMap.clear();
  return true;
}

void CloneAddressArithmetic::writeLog() {

  if (IGC_IS_FLAG_ENABLED(RematLog) && OutputLogFile->is_open())
    *OutputLogFile << OutputLogStream.str();

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
                  .Retry(CGCtx->m_retryManager.GetRetryId())
                  .Pass(ss.str().c_str())
                  .Extension("ll");

  OutputLogFile = std::make_unique<std::ofstream>(Name.str());
}

bool CloneAddressArithmetic::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  WI = &getAnalysis<WIAnalysis>();

  bool Modified = false;
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
        Instruction *insertPoint = BB->getFirstNonPHIOrDbgOrLifetime();
        newAdd->insertBefore(insertPoint);
        newIntToPtr->insertBefore(insertPoint);
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
    Clone->insertBefore(dyn_cast<Instruction>(CurV));
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
