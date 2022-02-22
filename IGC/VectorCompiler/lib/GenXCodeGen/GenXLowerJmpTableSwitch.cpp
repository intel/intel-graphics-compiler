/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXLowerJmpTableSwitchPass
/// ---------------------------
/// This pass replaces switch instructions with a internal_jump_table intrinsic
/// and indirect branch instruction. Then at CisaBuilder stage,
/// internal_jump_table and indirect branch will be emitted as visa switchjmp
/// instruction. The internal_jump_table intrinsic is required because indirect
/// branch takes the address of the BB to jump to and the full set of possible
/// destinations as blockaddresses that the address may point to. But the visa
/// switchjmp takes the full set of possible destinations and the index of
/// destination in this set. Hence, internal_jump_table is a helper that
///   * makes llvm ir legal because it takes the full set of destinations and
///   returns a pointer to the selected label;
///   * holds the index of the BB that must be used in the visa switchjmp.
///

#define DEBUG_TYPE "GENX_LOWERJMPTABLESWITCH"

#include "GenX.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "Probe/Assertion.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

#include "vc/InternalIntrinsics/InternalIntrinsics.h"

using namespace llvm;
using namespace genx;

static cl::opt<bool>
    NoJumpTables("no-jump-tables",
                 llvm::cl::desc("Disable switch to jump table lowening"),
                 cl::init(false), cl::Hidden);

namespace {

class GenXLowerJmpTableSwitch : public FunctionPass {
  const GenXSubtarget *ST = nullptr;

  bool processSwitchCandidates(Function &F, ArrayRef<SwitchInst *> Candidates);

public:
  static char ID;

  explicit GenXLowerJmpTableSwitch() : FunctionPass(ID) {}

  StringRef getPassName() const override { return "GenX LowerJmpTableSwitch"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;
};

} // end anonymous namespace

char GenXLowerJmpTableSwitch::ID = 0;
namespace llvm {
void initializeGenXLowerJmpTableSwitchPass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXLowerJmpTableSwitch, "GenXLowerJmpTableSwitch",
                      "GenXLowerJmpTableSwitch", false, false)
INITIALIZE_PASS_END(GenXLowerJmpTableSwitch, "GenXLowerJmpTableSwitch",
                    "GenXLowerJmpTableSwitch", false, false)

static SmallVector<SwitchInst *, 4> collectSwitchCandidates(Function &F) {
  SmallVector<SwitchInst *, 4> Switches;
  for (auto &BB : F) {
    for (auto &Inst : BB) {
      if (auto *Switch = dyn_cast<SwitchInst>(&Inst))
        Switches.push_back(Switch);
    }
  }
  return Switches;
}

// returns Min and Max case values.
static std::tuple<ConstantInt *, ConstantInt *>
findMinAndMaxValues(SwitchInst *SI) {
  auto Comp = [](const auto &L, const auto &R) {
    const APInt &LA = L.getCaseValue()->getValue();
    const APInt &RA = R.getCaseValue()->getValue();
    return LA.slt(RA);
  };
  auto CB = SI->case_begin();
  auto CE = SI->case_end();
  ConstantInt *Min = std::min_element(CB, CE, Comp)->getCaseValue();
  ConstantInt *Max = std::max_element(CB, CE, Comp)->getCaseValue();

  return std::make_tuple(Min, Max);
}

// Changes the switch inst condition to index in jump table. The case values
// will be changed from [first; last] to [0; last - first].
static Value *getJumpTableIndex(IRBuilder<> &Builder, SwitchInst *SI,
                                ConstantInt *V) {
  Builder.SetInsertPoint(SI);
  Value *Cond = SI->getCondition();
  if (V->getZExtValue() != 0) {
    LLVM_DEBUG(dbgs() << "Change old condition: " << *Cond << "\nto ");
    Cond = Builder.CreateSub(Cond, V, Cond->getName() + ".new.jt.cond");
    LLVM_DEBUG(dbgs() << "new condition: " << *Cond << "\n");
  }

  return Cond;
}

// Creates a vector with the destinations from the switch inst. The order of BB
// is sorted by case values.
static std::vector<BlockAddress *>
collectBlockAddresses(SwitchInst *SI, ConstantInt *MinCaseV) {
  unsigned NumCases = SI->getNumCases();
  std::vector<BlockAddress *> BAs(NumCases);
  for (auto CaseIt : SI->cases()) {
    APInt Idx = CaseIt.getCaseValue()->getValue() - MinCaseV->getValue();
    IGC_ASSERT(Idx.getZExtValue() < NumCases);
    BAs[Idx.getZExtValue()] = BlockAddress::get(CaseIt.getCaseSuccessor());
  }
  return BAs;
}

static bool canLower(unsigned NumCases) {
  constexpr unsigned MaxNumCases = 32;
  constexpr unsigned MinNumCases = 4;
  // Doesn't fit into one visa instuction.
  // FIXME: split into several instructions.
  if (NumCases > MaxNumCases)
    return false;
  // Binary search will not be worse
  if (NumCases < MinNumCases)
    return false;
  return true;
}

FunctionPass *llvm::createGenXLowerJmpTableSwitchPass() {
  initializeGenXLowerJmpTableSwitchPass(*PassRegistry::getPassRegistry());
  return new GenXLowerJmpTableSwitch();
}

void GenXLowerJmpTableSwitch::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
}

bool GenXLowerJmpTableSwitch::processSwitchCandidates(
    Function &F, ArrayRef<SwitchInst *> Candidates) {
  bool Modified = false;
  LLVMContext &Ctx = F.getContext();
  IRBuilder<> Builder(Ctx);

  for (SwitchInst *SI : Candidates) {
    unsigned NumCases = SI->getNumCases();
    if (!canLower(NumCases))
      continue;

    ConstantInt *Min = nullptr, *Max = nullptr;
    std::tie(Min, Max) = findMinAndMaxValues(SI);
    // Only a switch without missed case numbers may be lowered to a jump table.
    int64_t MaxSE = Max->getSExtValue();
    int64_t MinSE = Min->getSExtValue();
    if (MaxSE - MinSE != NumCases - 1)
      continue;

    LLVM_DEBUG(dbgs() << "Replacing switch: " << *SI << "\n");

    // Get the switch condition. Also make it suitable for indexing in the jump
    // table.
    Value *JTIdx = getJumpTableIndex(Builder, SI, Min);

    // Create cond branch for default case.
    BasicBlock *SIBB = SI->getParent();
    BasicBlock *JTBB = BasicBlock::Create(Ctx, SIBB->getName() + ".jt", &F);
    JTBB->moveAfter(SIBB);
    APInt MaxJTValue = Max->getValue() - Min->getValue();
    // The default case branch must be taken if there is no entry in a jump
    // table with the JTIdx. Entries in the jump table start from 0 to NumCases
    // and form a continuous range of integers.
    Value *DefaultCaseCmp =
        Builder.CreateICmpULE(JTIdx, Builder.getInt(MaxJTValue), ".jt.default");
    Builder.CreateCondBr(DefaultCaseCmp, JTBB, SI->getDefaultDest());
    Builder.SetInsertPoint(JTBB);

    // Collect blockaddresses in sorted order.
    std::vector<BlockAddress *> BAs = collectBlockAddresses(SI, Min);

    auto IID = vc::InternalIntrinsic::jump_table;
    // Collect output and input Idx types as well as blockaddress type repeated
    // NumCases times to create internal_jump_table decl.
    std::vector<Type *> InTys(NumCases + 2, BAs[0]->getType());
    // Return type
    InTys[0] = Builder.getInt8PtrTy();
    // Index in jump table. Only this arg will be really needed.
    InTys[1] = JTIdx->getType();
    Function *JTDecl = vc::InternalIntrinsic::getInternalDeclaration(
        SI->getModule(), IID, InTys);

    // Collect args.
    std::vector<Value *> InArgs;
    InArgs.reserve(NumCases + 1);
    InArgs.push_back(JTIdx);
    InArgs.insert(InArgs.end(), BAs.begin(), BAs.end());
    CallInst *JumpTable =
        IntrinsicInst::Create(JTDecl, InArgs, "switch.jt", JTBB);
    LLVM_DEBUG(dbgs() << "Jump table for the switch: " << *JumpTable << "\n");

    IndirectBrInst *Br = Builder.CreateIndirectBr(JumpTable, BAs.size());
    for (auto BA : BAs)
      Br->addDestination(BA->getBasicBlock());
    LLVM_DEBUG(dbgs() << "IndirectBr: " << *Br << "\n");

    // Since the original BasicBlock with switch has been split into 2, PHINodes
    // must be updated.
    for (BasicBlock *Succ : Br->successors()) {
      for (PHINode &Phi : Succ->phis()) {
        int Idx = Phi.getBasicBlockIndex(SIBB);
        IGC_ASSERT_MESSAGE(Idx >= 0, "Switch successor's PHINode doesn't have "
                                     "the switch BB as incoming block.");
        Phi.setIncomingBlock(Idx, JTBB);
      }
    }

    SI->eraseFromParent();
    Modified = true;
  }

  return Modified;
}

bool GenXLowerJmpTableSwitch::runOnFunction(Function &F) {
  ST = &getAnalysis<TargetPassConfig>()
            .getTM<GenXTargetMachine>()
            .getGenXSubtarget();
  if (ST->disableJumpTables() || NoJumpTables)
    return false;

  if (!ST->hasSwitchjmp())
    return false;

  auto Candidates = collectSwitchCandidates(F);
  return processSwitchCandidates(F, Candidates);
}
