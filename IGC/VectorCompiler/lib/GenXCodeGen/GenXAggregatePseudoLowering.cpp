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
//
/// GenXAggregatePseudoLowering
/// ---------------------------
///
/// The pass is meant to replace all instructions that work with aggregate
/// values with instructions that work with elementary types (scalar, vector),
/// so there's no aggregate values in IR at all. But this pass doesn't do full
/// job, that's why it has pseudo in its name.
/// This pass replaces every instruction (except call, extract/insertvalue, etc)
/// that either has aggregate as operand, or returns an aggregate with series
/// of extractvalue instructions (if there was an aggregate operand) which
/// return only elementary values, then sequence of splits of the original
/// instruction (but now each one is working only with an elementary value) and
/// finally the sequence of insertvalues that join all elementary results back
/// to the original aggregate result.
///
/// Example:
/// Before pass:
///   %struct_t = type { <16 x float>, <16 x float>, <16 x float> }
///   %res = select i1 %c, %struct_t %arg.0, %struct_t %arg.1
/// After pass:
///   %struct_t = type { <16 x float>, <16 x float>, <16 x float> }
///   %arg.0.0 = extractvalue %struct_t %arg.0, 0
///   %arg.0.1 = extractvalue %struct_t %arg.0, 1
///   %arg.0.2 = extractvalue %struct_t %arg.0, 2
///   %arg.1.0 = extractvalue %struct_t %arg.1, 0
///   %arg.1.1 = extractvalue %struct_t %arg.1, 1
///   %arg.1.2 = extractvalue %struct_t %arg.1, 2
///   %res.0 = select i1 %c, <16 x float> %arg.0.0, <16 x float> %arg.1.0
///   %res.1 = select i1 %c, <16 x float> %arg.0.1, <16 x float> %arg.1.1
///   %res.2 = select i1 %c, <16 x float> %arg.0.2, <16 x float> %arg.1.2
///   %tmp.0 = insertvalue %struct_t undef,  <16 x float> %res.0, 0
///   %tmp.1 = insertvalue %struct_t %tmp.0, <16 x float> %res.1, 1
///   %res   = insertvalue %struct_t %tmp.1, <16 x float> %res.2, 2
///
/// As you can see the pass doesn't fully get rid of aggregate values, it only
/// locally replaces operations over aggregates with operations over elementary
/// fields of aggregates. But if there is the instruction combine pass after
/// this pass, it can easily merge extractvalue and insertvalue so the there's
/// no aggregate values in code anymore.
///
/// Terminology:
/// Split instructions - the instructions into which original instruction
///                      is split, e.g. %res.0, %res.1, %res.2 are split insts
///                      (%res is corresponding original instruction)
/// Split operands - the instructions into which original operands are split,
///                  they are always extractvalue instructions, e.g.
///                  %arg.0.0, %arg.0.1, %arg.0.2 are split operands
///                  (%arg.0 is corresponding original operand)
///
/// Note: split instruction operands is operands of a split instruction, not
/// split operands, though split instruction operands contain at least one
/// split operand, e.g. %c, %arg.0.0, %arg.1.0 for %res.0 instruction.
///
/// TODO: currently this pass can only handle only flat structures (without
/// nested aggregates). Supported instructions are phi and select.
//
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXModule.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include <unordered_map>

using namespace llvm;
using namespace genx;

namespace {

// It is a map between original aggregate instruction operand
// and corresponding split operands.
// Split operands are always extractvalue instructions.
using SplitOpsMap = std::unordered_map<Use *, std::vector<Instruction *>>;

class GenXAggregatePseudoLowering : public FunctionPass {
  std::vector<Instruction *> ToErase;

public:
  static char ID;
  explicit GenXAggregatePseudoLowering() : FunctionPass(ID) {}
  StringRef getPassName() const override {
    return "GenX aggregate pseudo lowering";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;

private:
  void processInst(Instruction &Inst);
};

} // end namespace

char GenXAggregatePseudoLowering::ID = 0;
namespace llvm {
void initializeGenXAggregatePseudoLoweringPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXAggregatePseudoLowering,
                      "GenXAggregatePseudoLowering",
                      "GenXAggregatePseudoLowering", false, false)
INITIALIZE_PASS_END(GenXAggregatePseudoLowering, "GenXAggregatePseudoLowering",
                    "GenXAggregatePseudoLowering", false, false)

FunctionPass *llvm::createGenXAggregatePseudoLoweringPass() {
  initializeGenXAggregatePseudoLoweringPass(*PassRegistry::getPassRegistry());
  return new GenXAggregatePseudoLowering;
}

void GenXAggregatePseudoLowering::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
}

// is at least one of instruction's operands an aggregate value
static bool hasAggregateOperand(const Instruction &Inst) {
  return llvm::any_of(Inst.operand_values(), [](const Value *V) {
    return V->getType()->isAggregateType();
  });
}

// does instruction have an aggregate as an operand or return value
static bool hasAggregate(const Instruction &Inst) {
  return Inst.getType()->isAggregateType() || hasAggregateOperand(Inst);
}

bool GenXAggregatePseudoLowering::runOnFunction(Function &F) {
  std::vector<Instruction *> WorkList;
  auto WorkRange = make_filter_range(instructions(F), [](Instruction &Inst) {
    return hasAggregate(Inst) && !isa<InsertValueInst>(Inst) &&
           !isa<ExtractValueInst>(Inst) && !isa<CallInst>(Inst) &&
           !isa<ReturnInst>(Inst);
  });
  llvm::transform(WorkRange, std::back_inserter(WorkList),
                  [](Instruction &Inst) { return &Inst; });
  if (WorkList.empty())
    return false;

  for (auto *Inst : WorkList)
    processInst(*Inst);

  for (auto *Inst : ToErase)
    Inst->eraseFromParent();
  ToErase.clear();
  return true;
}

// Returns first instruction after provided instruciton \p Inst,
// before which new instruction can be inserted.
static Instruction *getFirstInsertionPtAfter(Instruction &Inst) {
  if (isa<PHINode>(Inst))
    return Inst.getParent()->getFirstNonPHI();
  return Inst.getNextNode();
}

// Returns first instruction before which new instruction that represent new
// operand can be inserted, so the new instruction precedes provided
// instruction. \p Inst. Operand \Op is the operator to be updated.
static Instruction *getFirstInsertionPtBefore(Use &Op, Instruction &Inst) {
  if (!isa<PHINode>(Inst))
    return &Inst;
  return cast<PHINode>(Inst).getIncomingBlock(Op)->getTerminator();
}

// Arguments:
//    \p Inst - an instruction
//    \p Op - operand of the instruction \p Inst
//
// Returns an instruction before which new operand for instruction \p Inst,
// that correspond to the operand \p Op, can be inserted
static Instruction *getInsertionPtForSplitOp(Use &Op, Instruction &Inst) {
  auto &OpVal = *Op.get();
  if (isa<Instruction>(OpVal))
    return getFirstInsertionPtAfter(cast<Instruction>(OpVal));
  assert(isa<Constant>(OpVal) && "only instruction or constant are expected");
  return getFirstInsertionPtBefore(Op, Inst);
}

// Arguments:
//    \p Inst - an instruction
//    \p Op - operand of the instruction \p Inst
//
// Splits operand \p Op of the instruction \p Inst into elementary values.
// Those values are extractvalue instructions. Inserts those instruction in
// proper places, so if we insert new instruction right after or right before
// \p Inst those instructions could be reached.
//
// Returns the vector of created instructions.
static std::vector<Instruction *> createSplitOperand(Use &Op,
                                                     Instruction &Inst) {
  auto &OpVal = *Op.get();
  assert(OpVal.getType()->isAggregateType() && "wrong argument");
  // TODO: support ArrayType
  auto *InstTy = cast<StructType>(OpVal.getType());
  auto *InsertionPt = getInsertionPtForSplitOp(Op, Inst);
  std::vector<Instruction *> SplitOperand;
  for (unsigned i = 0; i < InstTy->getNumElements(); ++i) {
    assert(!InstTy->getElementType(i)->isAggregateType() &&
           "folded structures is yet unsupported");
    SplitOperand.push_back(
        ExtractValueInst::Create(&OpVal, i, "", InsertionPt));
  }
  return SplitOperand;
}

// Arguments:
//    \p Inst - an instruction
//
// Splits all aggregate operands of provided \p Inst.
// Returns a map between original operands and created instructions.
static SplitOpsMap createSplitOperands(Instruction &Inst) {
  assert(hasAggregateOperand(Inst) &&
         "wrong argument: inst must have aggregate operand");
  auto AggregateOps = make_filter_range(Inst.operands(), [](const Use &U) {
    return U->getType()->isAggregateType();
  });

  SplitOpsMap SplitOps;
  llvm::transform(AggregateOps, std::inserter(SplitOps, SplitOps.end()),
                  [&Inst](Use &U) {
                    return std::make_pair(&U, createSplitOperand(U, Inst));
                  });
  return SplitOps;
}

// Arguments:
//    \p elemIdx - element index of the aggregate for which we construct
//                 split instruction
//    \p OrigOps - original instruction operands (contain aggregates)
//    \p SplitOps - map between original aggregate operands and corresponding
//                  split operands
//
// Returns vector of operands (as Value*) for split instruction with index \p
// elemIdx.
template <typename OpRange>
std::vector<Value *> createSplitInstOperands(int elemIdx, OpRange OrigOps,
                                             const SplitOpsMap &SplitOps) {
  std::vector<Value *> NewOps;
  llvm::transform(OrigOps, std::back_inserter(NewOps),
                  [elemIdx, &SplitOps](Use &OrigOp) -> Value * {
                    if (OrigOp.get()->getType()->isAggregateType())
                      return SplitOps.at(&OrigOp)[elemIdx];
                    return OrigOp.get();
                  });
  return NewOps;
}

// Arguments:
//    \p Inst - original instruction
//    \p NewOps - operands for split instruction
//
// Creates split instruction based on the kind of original instruction.
// New instruction is inserted right before \p Inst.
// Split instruction is returned.
static Instruction *createSplitInst(Instruction &Inst,
                                    const std::vector<Value *> &NewOps) {
  if (isa<SelectInst>(Inst)) {
    assert(NewOps.size() == 3 && "select must have 3 operands");
    auto *NewSelect =
        SelectInst::Create(NewOps[0], NewOps[1], NewOps[2],
                           Inst.getName() + ".split.aggr", &Inst, &Inst);
    NewSelect->setDebugLoc(Inst.getDebugLoc());
    return NewSelect;
  }
  assert(isa<PHINode>(Inst) && "unsupported instruction");
  assert(Inst.getNumOperands() == NewOps.size() && "");
  auto *NewPHI = PHINode::Create(NewOps[0]->getType(), NewOps.size(),
                                 Inst.getName() + ".split.aggr", &Inst);

  auto &OldPHI = cast<PHINode>(Inst);
  for (auto &&Incoming : zip(NewOps, OldPHI.blocks())) {
    Value *OpVal = std::get<0>(Incoming);
    BasicBlock *OpBB = std::get<1>(Incoming);
    assert(isa<ExtractValueInst>(OpVal) &&
           "phi operands must be previously in this pass created "
           "extractvalue insts");
    auto *OpInst = cast<Instruction>(OpVal);
    NewPHI->addIncoming(OpInst, OpBB);
  }
  NewPHI->setDebugLoc(Inst.getDebugLoc());
  return NewPHI;
}

// Arguments:
//    \p Inst - original instruction
//    \p SplitOps - map between original aggregate operands and corresponding
//                  elementary operands
//
// Creates all split instructions for original \p Inst, inserts them before the
// original one. Returns vector of created split instructions.
static std::vector<Instruction *>
createSplitInsts(Instruction &Inst, const SplitOpsMap &SplitOps) {
  // TODO: support ArrayType
  auto &InstTy = *cast<StructType>(Inst.getType());
  int NumNewInsts = InstTy.getNumElements();
  std::vector<Instruction *> NewInsts;
  NewInsts.reserve(NumNewInsts);
  for (int i = 0; i < NumNewInsts; ++i) {
    auto NewOps = createSplitInstOperands(i, Inst.operands(), SplitOps);
    NewInsts.push_back(createSplitInst(Inst, NewOps));
  }
  return NewInsts;
}

// Arguments:
//    \p SplitInsts - split instructions
//    \p JoinTy - aggregate type that all split instructions together should
//                form \p InsertBefore - insertion point
//
// Combines split instructions back into aggregate value with a sequence of
// inservalue instructions.
// Last insertvalue instruction that form full aggregate value is returned.
static Instruction *joinSplitInsts(const std::vector<Instruction *> &SplitInsts,
                                   Type *JoinTy, Instruction *InsertBefore) {
  assert(SplitInsts.size() == cast<StructType>(JoinTy)->getNumElements() &&
         "number of splitted insts doesn't correspond with aggregate type");
  Value *JoinInst = UndefValue::get(JoinTy);
  unsigned Idx = 0;
  for (auto *SplitInst : SplitInsts) {
    JoinInst =
        InsertValueInst::Create(JoinInst, SplitInst, Idx++, "", InsertBefore);
  }
  return cast<Instruction>(JoinInst);
}

void GenXAggregatePseudoLowering::processInst(Instruction &Inst) {
  assert(hasAggregate(Inst) &&
         "wrong argument: instruction doesn't work with aggregates");
  SplitOpsMap NewOperands;
  if (hasAggregateOperand(Inst))
    NewOperands = createSplitOperands(Inst);
  auto NewInsts = createSplitInsts(Inst, NewOperands);
  auto *JoinInst =
      joinSplitInsts(NewInsts, Inst.getType(), getFirstInsertionPtAfter(Inst));
  Inst.replaceAllUsesWith(JoinInst);
  ToErase.push_back(&Inst);
}
