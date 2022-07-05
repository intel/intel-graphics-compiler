/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXVectorCombiner
/// ------------
// This pass checks whether we use whole vector in operation,
// but this operation is splitted for parts of vector,
// and if we can combine them together, we do this
///
//===----------------------------------------------------------------------===//

#include <algorithm>

#include <llvm/ADT/Statistic.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include "llvmWrapper/Support/TypeSize.h"

#include "GenX.h"
#include "GenXIntrinsics.h"
#include "GenXModule.h"
#include "vc/Utils/GenX/Region.h"
#include "vc/Utils/General/InstRebuilder.h"

#include "IGC/common/debug/DebugMacros.hpp"

using namespace llvm;
using namespace genx;

#define DEBUG_TYPE "GENX_VECTOR_COMBINER"

STATISTIC(NumOfWidenInsructions,
          "Number of combined to wider variant instructions");

static cl::opt<bool>
    EnableGenXVectorCombiner("enable-Vector-Combiner-pass", cl::init(true),
                             cl::Hidden,
                             cl::desc("Enable Vector Combiner pass"));

namespace {

// GenXVectorCombiner : vectorize already vector instructions

// Currently pass work with
// add
// fadd
// bitcast
// trunc
// llvm.genx.absi
// llvm.genx.absf
// llvm.genx.bf.cvt
class GenXVectorCombiner final : public FunctionPass {
  struct InstructionPack {
    Instruction *RdRegion;
    Instruction *Operation;
    Instruction *WrRegion;
    explicit InstructionPack(Instruction *Rd, Instruction *Oper,
                             Instruction *Wr)
        : RdRegion{Rd}, Operation{Oper}, WrRegion{Wr} {
      IGC_ASSERT_MESSAGE(Rd && Oper && Wr, "Error: get nullptr");
    }
  };
  using ListOfPacks = SmallVector<InstructionPack, 4>;
  std::unordered_map<Value *, ListOfPacks> WorkList;

public:
  static char ID;
  explicit GenXVectorCombiner() : FunctionPass(ID) {}
  StringRef getPassName() const override { return "GenXVectorCombiner"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override {}
  bool runOnFunction(Function &F) override;

private:
  void dumpWorkList() const;
  void collectWorkList(Function &F);
  void filterWorkList();
  bool processWorkList();
  bool checkWrRegions(const SmallVectorImpl<InstructionPack> &List);
  bool checkRdRegions(const SmallVectorImpl<InstructionPack> &List);
  bool checkSameInst(const SmallVectorImpl<InstructionPack> &List);
  void createNewInstruction(Instruction *UnsteadOf, Instruction *Operation,
                            const SmallVectorImpl<Value *> &Vals);
  bool isSupportedInst(const Instruction *Inst);
  bool isSupportedGenXIntrinsic(GenXIntrinsic::ID OpCode);
  bool isSupportedOpcode(unsigned OpCode);
  bool isSuitableRdRegion(Instruction &RdRegionInst);
};
} // end namespace

char GenXVectorCombiner::ID = 0;
namespace llvm {
void initializeGenXVectorCombinerPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXVectorCombiner, "GenXVectorCombiner",
                      "GenXVectorCombiner", false, false)

INITIALIZE_PASS_END(GenXVectorCombiner, "GenXVectorCombiner",
                    "GenXVectorCombiner", false, false)
namespace llvm {
FunctionPass *createGenXVectorCombinerPass() {
  initializeGenXVectorCombinerPass(*PassRegistry::getPassRegistry());
  return new GenXVectorCombiner;
}
} // namespace llvm
// Checks whether this instruction can be optimized by this pass.
bool GenXVectorCombiner::isSupportedOpcode(unsigned OpCode) {
  switch (OpCode) {
  default:
    return false;
  case Instruction::Add:
  case Instruction::FAdd:
  case Instruction::BitCast:
  case Instruction::Trunc:
    return true;
  }
  IGC_ASSERT_MESSAGE(false, "unreachable");
  return false;
}

// Checks whether this GenXIntrinsic can be optimized by this pass.
bool GenXVectorCombiner::isSupportedGenXIntrinsic(GenXIntrinsic::ID IdCode) {
  IGC_ASSERT(IdCode != GenXIntrinsic::not_genx_intrinsic);
  switch (IdCode) {
  default:
    return false;
  case GenXIntrinsic::genx_bf_cvt:
  case GenXIntrinsic::genx_absf:
  case GenXIntrinsic::genx_absi:
    return true;
  }
  IGC_ASSERT(0);
  return false;
}

bool GenXVectorCombiner::isSupportedInst(const Instruction *Inst) {
  IGC_ASSERT_MESSAGE(Inst, "Error: nullptr");
  if (isSupportedOpcode(Inst->getOpcode()))
    return true;
  if (GenXIntrinsic::isGenXIntrinsic(Inst) &&
      isSupportedGenXIntrinsic(GenXIntrinsic::getGenXIntrinsicID(Inst)))
    return true;
  return false;
}

bool GenXVectorCombiner::isSuitableRdRegion(Instruction &RdRegionInst) {
  if (!GenXIntrinsic::isRdRegion(&RdRegionInst))
    return false;
  if (!RdRegionInst.getType()->isVectorTy())
    return false;
  if (!RdRegionInst.hasOneUse())
    return false;
  Value *Original = RdRegionInst.getOperand(0);

  Instruction *Inst = dyn_cast<Instruction>(*RdRegionInst.user_begin());
  if (!Inst)
    return false;

  if (!Inst->hasOneUse())
    return false;

  if (!isSupportedInst(Inst))
    return false;

  Instruction *WrRegionInst = dyn_cast<Instruction>(*Inst->user_begin());
  if (!WrRegionInst)
    return false;
  if (!GenXIntrinsic::isWrRegion(WrRegionInst))
    return false;
  // we must check that wr region and rdregion use the same part of vector, and
  // splat is equal to one
  vc::CMRegion RdRegion{&RdRegionInst};
  vc::CMRegion WrRegion{WrRegionInst};

  unsigned OriginalElementCount =
      cast<IGCLLVM::FixedVectorType>(Original->getType())->getNumElements();
  unsigned WrRegionElementCount =
      cast<IGCLLVM::FixedVectorType>(WrRegionInst->getType())->getNumElements();
  if (OriginalElementCount != WrRegionElementCount)
    return false;

  if (RdRegion.Width != WrRegion.Width)
    return false;
  if (RdRegion.Stride != 1 || RdRegion.VStride != 0)
    return false;
  if (RdRegion.Stride != WrRegion.Stride)
    return false;
  if (RdRegion.VStride != WrRegion.VStride)
    return false;
  // for trunc offset is different, but number of elements the same
  unsigned RdRegionElementTySize = RdRegion.ElementTy->getScalarSizeInBits();
  unsigned WrRegionElementTySize = WrRegion.ElementTy->getScalarSizeInBits();
  IGC_ASSERT_MESSAGE(RdRegionElementTySize != 0, "Error: get ScalarSizeInBits");
  IGC_ASSERT_MESSAGE(WrRegionElementTySize != 0, "Error: get ScalarSizeInBits");
  return RdRegion.Offset / RdRegionElementTySize ==
         WrRegion.Offset / WrRegionElementTySize;
}

// Checks that wrregions are in right order:
// 1) element in list should be connected with next one
// 2) RdRegions and WrRegions have the same arguments for offset and width, so do not check them twice
bool GenXVectorCombiner::checkWrRegions(
    const SmallVectorImpl<InstructionPack> &List) {
  if (!List.front().WrRegion->hasOneUse())
    return false;

  auto ResIterator =
      std::adjacent_find(List.begin(), List.end(),
                         [](const auto &CurrentPack, const auto &NextPack) {
                           Instruction *CurrentWrRegion = CurrentPack.WrRegion;
                           Instruction *NextWrRegion = NextPack.WrRegion;
                           if (NextWrRegion->getOperand(0) != CurrentWrRegion)
                             return true;
                           return !NextWrRegion->hasOneUse();
                         });
  return ResIterator == List.end();
}

// Checks that rdregions are in right order and "fill" all original vector.
bool GenXVectorCombiner::checkRdRegions(
    const SmallVectorImpl<InstructionPack> &List) {
  Value* Original = List.front().RdRegion->getOperand(0);
  unsigned OriginalWidth =
      cast<IGCLLVM::FixedVectorType>(Original->getType())->getNumElements();

  Type *RdRegionTy = List.front().RdRegion->getType();
  IGC_ASSERT_MESSAGE(!RdRegionTy->isPtrOrPtrVectorTy(),
                     "Error: PointerType not expected");
  unsigned ElementSizeInBits = RdRegionTy->getScalarSizeInBits();
  IGC_ASSERT_MESSAGE(ElementSizeInBits != 0, "getScalarSizeInBits return 0");

  unsigned ElementSize = ElementSizeInBits / ByteBits;

  vc::CMRegion FirstRdRegion{List.front().RdRegion};
  if (FirstRdRegion.Offset != 0)
    return false;

  auto IteratorRes = std::adjacent_find(
      List.begin(), List.end(),
      [ElementSize](const auto &CurrentPack, const auto &NextPack) {
        vc::CMRegion RdRegionCurrent{CurrentPack.RdRegion};
        vc::CMRegion RdRegionNext{NextPack.RdRegion};
        IGC_ASSERT(RdRegionCurrent.Width == RdRegionCurrent.NumElements);
        return RdRegionCurrent.Offset + ElementSize * RdRegionCurrent.Width !=
               RdRegionNext.Offset;
      });
  if (IteratorRes != List.end())
    return false;

  vc::CMRegion LastRdRegion{List.back().RdRegion};
  IGC_ASSERT(LastRdRegion.Width == LastRdRegion.NumElements);
  return LastRdRegion.Offset + ElementSize * LastRdRegion.Width ==
         OriginalWidth * ElementSize;
}

bool GenXVectorCombiner::checkSameInst(
    const SmallVectorImpl<InstructionPack> &List) {
  Instruction *Operation = List.front().Operation;
  if (GenXIntrinsic::isGenXIntrinsic(Operation)) {
    GenXIntrinsic::ID IdCode = GenXIntrinsic::getGenXIntrinsicID(Operation);
    IGC_ASSERT(IdCode != GenXIntrinsic::not_genx_intrinsic);

    // TODO: currently no support for intrinsics with two srcs
    return std::all_of(
        std::next(List.begin()), List.end(), [IdCode](const auto &InstPack) {
          Instruction *Operation = InstPack.Operation;
          return IdCode == GenXIntrinsic::getGenXIntrinsicID(Operation);
        });
  }
  unsigned OpCode = Operation->getOpcode();
  bool TwoSources = Operation->getNumOperands() == 2;
  Constant *SecondSrc = nullptr;
  if (TwoSources)
    SecondSrc = dyn_cast<Constant>(Operation->getOperand(1));

  if (TwoSources && SecondSrc == nullptr)
    return false;
  // check that second source is the same
  return std::all_of(std::next(List.begin()), List.end(),
                     [TwoSources, OpCode, SecondSrc](const auto &InstPack) {
                       Instruction *Operation = InstPack.Operation;
                       if (OpCode != Operation->getOpcode())
                         return false;
                       return !(TwoSources &&
                                Operation->getOperand(1) != SecondSrc);
                     });
}

// Performs analysis for collected SmallVectors:
// 1) Checks RdRegion "read" whole original vector
// 2) The same Operation is called for all small vectors
// 3) Write order is right
void GenXVectorCombiner::filterWorkList() {
  std::vector<Value *> ToEraseFromWorkList;
  for (auto &MapElement : WorkList) {
    if (MapElement.second.size() == 1) {
      ToEraseFromWorkList.push_back(MapElement.first);
      continue;
    }
    if (!checkRdRegions(MapElement.second)) {
      ToEraseFromWorkList.push_back(MapElement.first);
      continue;
    }
    if (!checkWrRegions(MapElement.second)) {
      ToEraseFromWorkList.push_back(MapElement.first);
      continue;
    }
    if (!checkSameInst(MapElement.second)) {
      ToEraseFromWorkList.push_back(MapElement.first);
      continue;
    }
  }
  for (auto *EraseVal : ToEraseFromWorkList)
    WorkList.erase(EraseVal);
}

// Places new instruction or GenX intrinsic.
void GenXVectorCombiner::createNewInstruction(
    Instruction *InsteadOf, Instruction *Operation,
    const SmallVectorImpl<Value *> &Vals) {
  IGC_ASSERT_MESSAGE(InsteadOf && Operation, "Error: nullptr input");
  if (GenXIntrinsic::isGenXIntrinsic(Operation)) {
    IRBuilder<> Builder{InsteadOf};
    Function *Fn = nullptr;
    GenXIntrinsic::ID IdCode = GenXIntrinsic::getGenXIntrinsicID(Operation);
    Module *M = Operation->getParent()->getParent()->getParent();
    switch (IdCode) {
    case GenXIntrinsic::genx_absf:
    case GenXIntrinsic::genx_absi:
      Fn = GenXIntrinsic::getAnyDeclaration(M, IdCode, {InsteadOf->getType()});
      break;
    case GenXIntrinsic::genx_bf_cvt:
      Fn = GenXIntrinsic::getAnyDeclaration(
          M, IdCode, {InsteadOf->getType(), Vals[0]->getType()});
      break;
    default:
      IGC_ASSERT_MESSAGE(false, "unsupported intrinsic");
      return;
    }
    IGC_ASSERT(Fn);
    CallInst *CI = Builder.CreateCall(Fn, Vals, VALUE_NAME("widened"));
    InsteadOf->replaceAllUsesWith(CI);
    return;
  }
  Instruction *NewInst = vc::cloneInstWithNewOps(*Operation, Vals);
  IGC_ASSERT(NewInst);
  NewInst->insertBefore(InsteadOf);
  NewInst->setDebugLoc(InsteadOf->getDebugLoc());
  NewInst->takeName(Operation);
  InsteadOf->replaceAllUsesWith(NewInst);
}

// Puts new instructions instead of every SmallVector.
// returns true if IR changed
bool GenXVectorCombiner::processWorkList() {
  if (WorkList.empty())
    return false;
  for (auto &MapElement : WorkList) {
    Value *OriginalSrc = MapElement.first;
    InstructionPack &RefAnyInstPack = MapElement.second.back();
    // last wrregion from sequence
    Instruction *InsteadOf = MapElement.second.back().WrRegion;

    SmallVector<Value *, 2> Vals;
    Vals.push_back(OriginalSrc);
    // two src - second should be const
    bool IsGenXIntrinsic =
        GenXIntrinsic::isGenXIntrinsic(RefAnyInstPack.Operation);
    if (!IsGenXIntrinsic && RefAnyInstPack.Operation->getNumOperands() == 2) {
      Constant *ScalarSecondOperand =
          cast<Constant>(RefAnyInstPack.Operation->getOperand(1))
              ->getSplatValue();
      unsigned NumericWidth = cast<IGCLLVM::FixedVectorType>(OriginalSrc->getType())
                      ->getNumElements();
      auto Width = IGCLLVM::getElementCount(NumericWidth);
      Constant *WideConstant =
          ConstantVector::getSplat(Width, ScalarSecondOperand);
      Vals.push_back(WideConstant);
    }
    createNewInstruction(InsteadOf, RefAnyInstPack.Operation, Vals);

    ++NumOfWidenInsructions;
  }
  for (auto &MapElement : WorkList) {
    // wrregions are connected with each other, so last wrregion first
    std::for_each(MapElement.second.rbegin(), MapElement.second.rend(),
                  [](InstructionPack &Pack) {
                    Pack.WrRegion->eraseFromParent();
                    Pack.Operation->eraseFromParent();
                    Pack.RdRegion->eraseFromParent();
                  });
  }
  WorkList.clear();
  return true;
}

// Collects and checks some available on that level restrictions:
// * pattern: RdRegion -> Operation -> WrRegion
void GenXVectorCombiner::collectWorkList(Function &F) {
  for (auto &Inst : instructions(F)) {
    if (!isSuitableRdRegion(Inst))
      continue;
    Value *SrcV = Inst.getOperand(0);
    Instruction *RdRegion = &Inst;
    Instruction *Operation = cast<Instruction>(*RdRegion->user_begin());
    Instruction *WrRegion = cast<Instruction>(*Operation->user_begin());
    IGC_ASSERT_MESSAGE(
        cast<IGCLLVM::FixedVectorType>(SrcV->getType())->getNumElements() ==
            cast<IGCLLVM::FixedVectorType>(WrRegion->getType())
                ->getNumElements(),
        "Error: mismatch in element count unexpected");

    WorkList[SrcV].emplace_back(RdRegion, Operation, WrRegion);
  }
}

// Algorithm :
// 1) find all available combinations of RdRegion, Operation and WrRegion,
//        put it in SmallVector in history order
// 2) Analyze collected SmallVectors and check if we can combine them into wider
// one
// 3) Perform transformation
bool GenXVectorCombiner::runOnFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "GenXVectorCombiner started\n");
  if (!EnableGenXVectorCombiner)
    return false;
  collectWorkList(F);
  filterWorkList();
  return processWorkList();
}

void GenXVectorCombiner::dumpWorkList() const {
  for (auto &List : WorkList) {
    dbgs() << "Original: ";
    List.first->dump();
    for (auto &InstPack : List.second) {
      Instruction *RdRegion = InstPack.RdRegion;
      Instruction *WrRegion = InstPack.WrRegion;
      Instruction *Operation = InstPack.Operation;
      RdRegion->dump();
      dbgs() << "\t";
      Operation->dump();
      dbgs() << "\t\t";
      WrRegion->dump();
    }
  }
}
