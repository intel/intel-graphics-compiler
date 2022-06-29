/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
/// TODO: Supported instructions are phi, select, load and store.
//
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXModule.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/Support/Alignment.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#include <unordered_map>
#include <vector>

using namespace llvm;
using namespace genx;

namespace {

// It is a map between original aggregate instruction operand
// and corresponding split operands.
// Split operands are always extractvalue instructions.
using SplitOpsMap = std::unordered_map<Use *, std::vector<Instruction *>>;

// For iterating over elementary values in the case of nested aggregates, it is
// convenient to use a list of indices, rather than a single index. Each index
// in the list is an index at a given nesting depth. Example:
//   %struct_t = type { float, type { [5 x i32], i8 } }
// The float element will have a list of indices {0}, and the fourth element of
// the array will have a list of indices {1, 0, 3}.
using IdxListType = std::vector<unsigned>;

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
  // Atomic cmpxchg returns a struct, though we shouldn't process it here.
  auto WorkRange = make_filter_range(instructions(F), [](Instruction &Inst) {
    return hasAggregate(Inst) && !isa<InsertValueInst>(Inst) &&
           !isa<ExtractValueInst>(Inst) && !isa<CallInst>(Inst) &&
           !isa<AtomicCmpXchgInst>(Inst) && !isa<ReturnInst>(Inst);
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
  IGC_ASSERT_MESSAGE(isa<Constant>(OpVal) || isa<Argument>(OpVal),
    "only instruction, constant or argument are expected");
  return getFirstInsertionPtBefore(Op, Inst);
}

// Arguments:
//    \p Inst - an instruction
//    \p Op - operand of the instruction \p Inst
//    \p IdxLists - lists of indices for all elementary values of \p Op (see the
//                  description of IdxListType)
//
// Splits operand \p Op of the instruction \p Inst into elementary values.
// Those values are extractvalue instructions. Inserts those instruction in
// proper places, so if we insert new instruction right after or right before
// \p Inst those instructions could be reached.
//
// Returns the vector of created instructions.
static std::vector<Instruction *>
createSplitOperand(Use &Op, Instruction &Inst,
                   const std::vector<IdxListType> &IdxLists) {
  auto &OpVal = *Op.get();
  IGC_ASSERT_MESSAGE(OpVal.getType()->isAggregateType(), "wrong argument");
  auto *InsertionPt = getInsertionPtForSplitOp(Op, Inst);
  std::vector<Instruction *> SplitOperand;
  for (const auto &IdxList : IdxLists) {
    SplitOperand.push_back(
        ExtractValueInst::Create(&OpVal, IdxList, "", InsertionPt));
  }
  return SplitOperand;
}

// Arguments:
//    \p Inst - an instruction
//    \p IdxLists - lists of indices for all elementary values of aggregates of
//                  \p Inst (see the description of IdxListType).
//                  It is assumed that all aggregate operands of \p Inst and
//                  it's return value, if it is aggregate, have the same type.
//
// Splits all aggregate operands of provided \p Inst.
// Returns a map between original operands and created instructions.
static SplitOpsMap
createSplitOperands(Instruction &Inst,
                    const std::vector<IdxListType> &IdxLists) {
  IGC_ASSERT_MESSAGE(hasAggregateOperand(Inst),
    "wrong argument: inst must have aggregate operand");
  auto AggregateOps = make_filter_range(Inst.operands(), [](const Use &U) {
    return U->getType()->isAggregateType();
  });

  SplitOpsMap SplitOps;
  llvm::transform(AggregateOps, std::inserter(SplitOps, SplitOps.end()),
                  [&Inst, &IdxLists](Use &U) {
                    return std::make_pair(
                        &U, createSplitOperand(U, Inst, IdxLists));
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

class SplitInstCreator : public InstVisitor<SplitInstCreator, Instruction *> {
  const std::vector<Value *> &NewOps;
  // The list of indices of the currently considered element of an aggregate.
  const IdxListType &IdxList;

public:
  SplitInstCreator(const std::vector<Value *> &NewOpsIn,
                   const IdxListType &IdxListIn)
      : NewOps{NewOpsIn}, IdxList{IdxListIn} {
    IGC_ASSERT_MESSAGE(
        !IdxList.empty(),
        "the list of indices of an aggregate element cannot be of zero length");
  }

  Instruction *visitInstruction(Instruction &I) const {
    IGC_ASSERT_MESSAGE(0, "yet unsupported instruction");
    return nullptr;
  }

  Instruction *create(Instruction &I) {
    auto *NewInst = visit(I);
    IGC_ASSERT_MESSAGE(!hasAggregate(*NewInst),
                       "split instruction must not have aggregate as an "
                       "operand or a return value");
    return NewInst;
  }

  Instruction *visitSelectInst(SelectInst &Inst) const {
    IGC_ASSERT_MESSAGE(NewOps.size() == 3, "select must have 3 operands");
    auto *NewSelect =
        SelectInst::Create(NewOps[0], NewOps[1], NewOps[2],
                           Inst.getName() + ".split.aggr", &Inst, &Inst);
    NewSelect->setDebugLoc(Inst.getDebugLoc());
    return NewSelect;
  }

  Instruction *visitPHINode(PHINode &OldPHI) const {
    IGC_ASSERT(OldPHI.getNumOperands() == NewOps.size());
    auto *NewPHI = PHINode::Create(NewOps[0]->getType(), NewOps.size(),
                                   OldPHI.getName() + ".split.aggr", &OldPHI);

    for (auto &&Incoming : zip(NewOps, OldPHI.blocks())) {
      Value *OpVal = std::get<0>(Incoming);
      BasicBlock *OpBB = std::get<1>(Incoming);
      IGC_ASSERT_MESSAGE(isa<ExtractValueInst>(OpVal),
                         "phi operands must be previously in this pass created "
                         "extractvalue insts");
      auto *OpInst = cast<Instruction>(OpVal);
      NewPHI->addIncoming(OpInst, OpBB);
    }
    NewPHI->setDebugLoc(OldPHI.getDebugLoc());
    return NewPHI;
  }

  std::vector<Value *> CreateIdxListForGEP(IRBuilder<> &IRB) const {
    std::vector<Value *> IdxListForGEP = {IRB.getInt32(0)};
    llvm::transform(IdxList, std::back_inserter(IdxListForGEP),
                    [&IRB](auto Idx) { return IRB.getInt32(Idx); });
    return IdxListForGEP;
  }

  Instruction *visitLoadInst(LoadInst &OrigLoad) const {
    IGC_ASSERT_MESSAGE(NewOps.size() == 1, "load has only one operand");
    IGC_ASSERT_MESSAGE(OrigLoad.getPointerOperand() == NewOps[0],
                       "should take the operand from the original load");
    IRBuilder<> IRB{&OrigLoad};
    Value *PointerOp = OrigLoad.getPointerOperand();
    Type *Ty = cast<PointerType>(PointerOp->getType()->getScalarType())
                   ->getPointerElementType();
    auto *GEP = IRB.CreateInBoundsGEP(Ty, PointerOp, CreateIdxListForGEP(IRB),
                                      OrigLoad.getName() + "aggr.gep");
    // FIXME: replace a structure alignment with an element alignment
    Type *GEPPtrTy = GEP->getType()->getPointerElementType();
    return IRB.CreateAlignedLoad(GEPPtrTy, GEP, IGCLLVM::getAlign(OrigLoad),
                                 OrigLoad.isVolatile(),
                                 OrigLoad.getName() + ".split.aggr");
  }

  Instruction *visitStoreInst(StoreInst &OrigStore) const {
    IRBuilder<> IRB{&OrigStore};
    Value *PointerOp = OrigStore.getPointerOperand();
    Type *Ty = cast<PointerType>(PointerOp->getType()->getScalarType())
                   ->getPointerElementType();
    auto *GEP = IRB.CreateInBoundsGEP(Ty, PointerOp, CreateIdxListForGEP(IRB),
                                      OrigStore.getName() + "aggr.gep");
    // FIXME: replace a structure alignment with an element alignment
    return IRB.CreateAlignedStore(NewOps[0], GEP, IGCLLVM::getAlign(OrigStore),
                                  OrigStore.isVolatile());
  }
};

// Arguments:
//    \p Inst - original instruction
//    \p NewOps - operands for split instruction
//    \p IdxList - the list of indices of the currently considered elementary
//                 value
//
// Creates split instruction based on the kind of original instruction.
// New instruction is inserted right before \p Inst.
// Split instruction is returned.
static Instruction *createSplitInst(Instruction &Inst,
                                    const std::vector<Value *> &NewOps,
                                    const IdxListType &IdxList) {
  return SplitInstCreator{NewOps, IdxList}.create(Inst);
}

// Arguments:
//    \p Inst - original instruction
//    \p SplitOps - map between original aggregate operands and corresponding
//                  elementary operands
//    \p IdxLists - lists of indices for all elementary values of aggregates of
//                  \p Inst (see the description of IdxListType).
//                  It is assumed that all aggregate operands of \p Inst have
//                  the same type.
//
// Creates all split instructions for original \p Inst, inserts them before the
// original one. Returns vector of created split instructions.
static std::vector<Instruction *>
createSplitInsts(Instruction &Inst, const SplitOpsMap &SplitOps,
                 const std::vector<IdxListType> &IdxLists) {
  std::vector<Instruction *> NewInsts;
  for (auto IdxList : enumerate(IdxLists)) {
    auto NewOps =
        createSplitInstOperands(IdxList.index(), Inst.operands(), SplitOps);
    NewInsts.push_back(createSplitInst(Inst, NewOps, IdxList.value()));
  }
  return NewInsts;
}

// Arguments:
//    \p SplitInsts - split instructions
//    \p JoinTy - aggregate type that all split instructions together should
//                form \p InsertBefore - insertion point
//    \p IdxLists - lists of indices for \p SplitInsts
//
// Combines split instructions back into aggregate value with a sequence of
// inservalue instructions.
// Last insertvalue instruction that form full aggregate value is returned.
static Instruction *joinSplitInsts(const std::vector<Instruction *> &SplitInsts,
                                   Type *JoinTy,
                                   const std::vector<IdxListType> &IdxLists,
                                   Instruction *InsertBefore) {
  IGC_ASSERT_MESSAGE(SplitInsts.size() == IdxLists.size(),
                     "the number of splitted insts doesn't correspond with the "
                     "number of index lists");
  Value *JoinInst = UndefValue::get(JoinTy);
  for (auto &&[SplitInst, IdxList] : zip(SplitInsts, IdxLists)) {
    JoinInst =
        InsertValueInst::Create(JoinInst, SplitInst, IdxList, "", InsertBefore);
  }
  return cast<Instruction>(JoinInst);
}

static Type *getAggregateTypeImpl(Instruction &Inst) {
  if (Inst.getType()->isAggregateType())
    return Inst.getType();
  auto AggrTypeIt = llvm::find_if(Inst.operands(), [](const Use &U) {
    return U->getType()->isAggregateType();
  });
  IGC_ASSERT_MESSAGE(AggrTypeIt != Inst.operands().end(),
                     "no aggregate operand or return value");
  return (*AggrTypeIt)->getType();
}

// Returns the type of the first aggregate operand of Inst, or the type of its
// return value, if it is aggregate. It is assumed that all aggregate operands
// of Inst and it's return value, if it is aggregate, have the same type.
static Type *getAggregateType(Instruction &Inst) {
  Type *AggrTy = getAggregateTypeImpl(Inst);
  IGC_ASSERT_MESSAGE(
      llvm::all_of(Inst.operands(),
                   [AggrTy](const Use &U) {
                     return !U->getType()->isAggregateType() ||
                            U->getType() == AggrTy;
                   }),
      "different aggregate types in the same instruction are not supported");
  return AggrTy;
}

// Returns the type of an aggregate's element at specific index.
static Type *getTypeAtIndex(Type *AggrTy, unsigned Index) {
  IGC_ASSERT_MESSAGE(isa<StructType>(AggrTy) || isa<ArrayType>(AggrTy),
                     "unexpected type");
  if (isa<StructType>(AggrTy))
    return cast<StructType>(AggrTy)->getTypeAtIndex(Index);
  return cast<ArrayType>(AggrTy)->getElementType();
}

// Returns the number of elements of an aggregate.
static unsigned getNumElements(Type *AggrTy) {
  IGC_ASSERT_MESSAGE(isa<StructType>(AggrTy) || isa<ArrayType>(AggrTy),
                     "unexpected type");
  if (isa<StructType>(AggrTy))
    return cast<StructType>(AggrTy)->getNumElements();
  return cast<ArrayType>(AggrTy)->getNumElements();
}

// Returns lists of indices for all elementary values of Inst's aggregate
// operands or return value.
static std::vector<IdxListType> createIdxLists(Type *AggrTy) {
  std::vector<IdxListType> IdxLists;
  std::vector<std::pair<Type *, unsigned>> AggrStack = {{AggrTy, 0}};
  while (!AggrStack.empty()) {
    Type *CurrAggr = AggrStack.back().first;
    unsigned CurrIndex = AggrStack.back().second;
    if (CurrIndex == getNumElements(CurrAggr)) {
      AggrStack.pop_back();
      if (!AggrStack.empty())
        ++AggrStack.back().second;
      continue;
    }
    Type *TypeAtIndex = getTypeAtIndex(CurrAggr, CurrIndex);
    if (TypeAtIndex->isAggregateType()) {
      AggrStack.emplace_back(TypeAtIndex, 0);
      continue;
    }
    IdxListType CurrIdxList;
    llvm::transform(AggrStack, std::back_inserter(CurrIdxList),
                    [](auto &Item) { return Item.second; });
    IdxLists.push_back(std::move(CurrIdxList));
    ++AggrStack.back().second;
  }
  return IdxLists;
}

void GenXAggregatePseudoLowering::processInst(Instruction &Inst) {
  IGC_ASSERT_MESSAGE(hasAggregate(Inst),
    "wrong argument: instruction doesn't work with aggregates");
  Type *AggrTy = getAggregateType(Inst);
  auto IdxLists = createIdxLists(AggrTy);
  SplitOpsMap NewOperands;
  if (hasAggregateOperand(Inst))
    NewOperands = createSplitOperands(Inst, IdxLists);
  auto NewInsts = createSplitInsts(Inst, NewOperands, IdxLists);
  if (Inst.getType()->isAggregateType()) {
    auto *JoinInst = joinSplitInsts(NewInsts, Inst.getType(), IdxLists,
                                    getFirstInsertionPtAfter(Inst));
    Inst.replaceAllUsesWith(JoinInst);
    JoinInst->takeName(&Inst);
  }
  ToErase.push_back(&Inst);
}
