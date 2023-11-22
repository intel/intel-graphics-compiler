/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenXLiveElements.h"
#include "GenX.h"
#include "GenXIntrinsics.h"
#include "GenXRegionUtils.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/Debug.h"

#include "Probe/Assertion.h"
#include "vc/Utils/General/IndexFlattener.h"

#define DEBUG_TYPE "GENX_LIVE_ELEMENTS"

using namespace llvm;
using namespace genx;

using GenXIntrinsic::GenXRegion::OldValueOperandNum;
using GenXIntrinsic::GenXRegion::NewValueOperandNum;
using GenXIntrinsic::GenXRegion::PredicateOperandNum;

static cl::opt<bool>
    PrintLiveElementsInfo("print-live-elements-info", cl::init(false),
                          cl::Hidden,
                          cl::desc("Print live elements analysis info"));

LiveElements::LiveElements(Type *Ty, bool IsLive) {
  unsigned NumElems = IndexFlattener::getNumElements(Ty);
  LiveElems.reserve(NumElems);

  for (unsigned Idx = 0; Idx < NumElems; Idx++) {
    unsigned ElemSize = 1;
    if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(
            IndexFlattener::getElementType(Ty, Idx)))
      ElemSize = VT->getNumElements();
    LiveElems.emplace_back(ElemSize, IsLive);
  }
}

LiveElements LiveElements::operator|=(const LiveElements &Rhs) {
  IGC_ASSERT(size() == Rhs.size());

  for (unsigned Idx = 0; Idx < LiveElems.size(); Idx++) {
    IGC_ASSERT(LiveElems[Idx].size() == Rhs[Idx].size());
    LiveElems[Idx] |= Rhs[Idx];
  }

  return *this;
}

void LiveElements::print(raw_ostream &OS) const {
  SmallVector<std::string, 2> LiveElemsStr;
  LiveElemsStr.reserve(LiveElems.size());

  for (auto &LiveBits : LiveElems) {
    std::string LiveBitsStr;
    LiveBitsStr.reserve(LiveBits.size());
    for (unsigned Idx = 0; Idx < LiveBits.size(); Idx++)
      LiveBitsStr.push_back(LiveBits[Idx] ? '1' : '0');
    LiveElemsStr.push_back(std::move(LiveBitsStr));
  }

  OS << '{' << join(LiveElemsStr, ", ") << '}';
}

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
void LiveElements::dump() const {
  print(dbgs());
  dbgs() << "\n";
}
#endif // if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)

LiveElements GenXLiveElements::getLiveElements(const Value *V) const {
  if (auto It = LiveMap.find(V); It != LiveMap.end())
    return It->second;
  return LiveElements(V->getType());
}

LiveElements GenXLiveElements::getLiveElements(const Use *U) const {
  auto Inst = cast<Instruction>(U->getUser());
  return getOperandLiveElements(Inst, U->getOperandNo(), getLiveElements(Inst));
}

// getBitCastLiveElements : propagation function for bitcast - scaling of
// destination bitmask to source size
LiveElements GenXLiveElements::getBitCastLiveElements(
    const BitCastInst *BCI, const LiveElements &InstLiveElems) const {
  IGC_ASSERT(InstLiveElems.size() == 1);

  auto &DstLiveBits = InstLiveElems[0];
  unsigned DstSize = DstLiveBits.size();

  unsigned SrcSize = 1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(BCI->getSrcTy()))
    SrcSize = VT->getNumElements();
  SmallBitVector SrcLiveBits(SrcSize);

  if (SrcSize < DstSize) {
    IGC_ASSERT(DstSize % SrcSize == 0);
    unsigned Scale = DstSize / SrcSize;
    for (unsigned Idx = 0; Idx < DstSize; Idx++)
      if (DstLiveBits[Idx])
        SrcLiveBits.set(Idx / Scale);
  } else {
    IGC_ASSERT(SrcSize % DstSize == 0);
    unsigned Scale = SrcSize / DstSize;
    for (unsigned Idx = 0; Idx < DstSize; Idx++)
      if (DstLiveBits[Idx])
        SrcLiveBits.set(Idx * Scale, (Idx + 1) * Scale);
  }

  return LiveElements(std::move(SrcLiveBits));
}

// getExtractValueLiveElements : propagation function for extractvalue
LiveElements GenXLiveElements::getExtractValueLiveElements(
    const ExtractValueInst *EVI, unsigned OperandNo,
    const LiveElements &InstLiveElems) const {
  auto OpTy = EVI->getOperand(OperandNo)->getType();
  if (OperandNo != 0)
    return LiveElements(OpTy, true);

  unsigned Size = IndexFlattener::getNumElements(EVI->getType());
  unsigned StartIdx = IndexFlattener::flatten(
      EVI->getAggregateOperand()->getType(), EVI->getIndices());

  LiveElements Res(OpTy);
  for (unsigned Idx = 0; Idx < Size; Idx++)
    Res[StartIdx + Idx] = InstLiveElems[Idx];

  return Res;
}

// getInsertValueLiveElements : propagation function for insertvalue
LiveElements GenXLiveElements::getInsertValueLiveElements(
    const InsertValueInst *IVI, unsigned OperandNo,
    const LiveElements &InstLiveElems) const {
  auto OpTy = IVI->getOperand(OperandNo)->getType();
  if (OperandNo != 0 && OperandNo != 1)
    return LiveElements(OpTy, true);

  unsigned Size =
      IndexFlattener::getNumElements(IVI->getInsertedValueOperand()->getType());
  unsigned StartIdx = IndexFlattener::flatten(
      IVI->getAggregateOperand()->getType(), IVI->getIndices());

  if (OperandNo == 1)
    return LiveElements(
        ArrayRef<SmallBitVector>(&InstLiveElems[StartIdx], Size));

  LiveElements Res(InstLiveElems);
  for (unsigned Idx = 0; Idx < Size; Idx++)
    Res[StartIdx + Idx].reset();

  return Res;
}

// getRdRegionLiveElements : propagation function of rdregion/rdpredregion
LiveElements GenXLiveElements::getRdRegionLiveElements(
    const Instruction *RdR, unsigned OperandNo,
    const LiveElements &InstLiveElems) const {
  auto OpTy = RdR->getOperand(OperandNo)->getType();
  if (OperandNo != OldValueOperandNum)
    return LiveElements(OpTy, true);

  IGC_ASSERT(InstLiveElems.size() == 1);
  auto &DstLiveBits = InstLiveElems[0];

  IGC_ASSERT(isa<IGCLLVM::FixedVectorType>(OpTy));
  unsigned SrcSize = cast<IGCLLVM::FixedVectorType>(OpTy)->getNumElements();

  Region R = makeRegionFromBaleInfo(RdR, BaleInfo());
  if (R.Indirect)
    return LiveElements(SmallBitVector(SrcSize, true));

  SmallBitVector SrcLiveBits(SrcSize);
  auto Indices = R.getAccessIndices();
  IGC_ASSERT(DstLiveBits.size() == Indices.size());

  for (unsigned DstIdx = 0; DstIdx < DstLiveBits.size(); DstIdx++)
    if (DstLiveBits[DstIdx]) {
      unsigned SrcIdx = Indices[DstIdx];
      if (SrcIdx < SrcLiveBits.size())
        SrcLiveBits.set(SrcIdx);
    }

  return LiveElements(std::move(SrcLiveBits));
}

// getWrRegionLiveElements : propagation function of wrregion/wrpredregion
LiveElements GenXLiveElements::getWrRegionLiveElements(
    const Instruction *WrR, unsigned OperandNo,
    const LiveElements &InstLiveElems) const {
  auto OpTy = WrR->getOperand(OperandNo)->getType();
  if (OperandNo != OldValueOperandNum && OperandNo != NewValueOperandNum &&
      !(OperandNo == PredicateOperandNum && OpTy->isVectorTy()))
    return LiveElements(OpTy, true);

  IGC_ASSERT(InstLiveElems.size() == 1);
  auto &DstLiveBits = InstLiveElems[0];

  Region R = makeRegionFromBaleInfo(WrR, BaleInfo());
  if (OperandNo == OldValueOperandNum) {
    // Process OldValueOperand - copy of destination bitmask
    SmallBitVector SrcLiveBits(DstLiveBits);
    // Unset all re-written elements
    if (!R.Indirect && !R.Mask) {
      auto Indices = R.getAccessIndices();
      for (unsigned Idx : Indices)
        if (Idx < SrcLiveBits.size())
          SrcLiveBits.reset(Idx);
    }
    return LiveElements(std::move(SrcLiveBits));
  }

  // Process NewValueOperand or PredicateOperand
  if (R.Indirect)
    return LiveElements(OpTy, true);

  unsigned SrcSize = 1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(OpTy))
    SrcSize = VT->getNumElements();

  SmallBitVector SrcLiveBits(SrcSize);
  auto Indices = R.getAccessIndices();
  IGC_ASSERT(SrcLiveBits.size() == Indices.size());

  for (unsigned SrcIdx = 0; SrcIdx < SrcLiveBits.size(); SrcIdx++) {
    unsigned DstIdx = Indices[SrcIdx];
    if (DstIdx < DstLiveBits.size() && DstLiveBits[DstIdx])
      SrcLiveBits.set(SrcIdx);
  }

  return LiveElements(std::move(SrcLiveBits));
}

// getTwoDstInstInstLiveElements : propagation function for intrinsics with
// two destinations (addc, subb)
LiveElements GenXLiveElements::getTwoDstInstLiveElements(
    const LiveElements &InstLiveElems) const {
  IGC_ASSERT(InstLiveElems.size() == 2);
  IGC_ASSERT(InstLiveElems[0].size() == InstLiveElems[1].size());
  return LiveElements(InstLiveElems[0] | InstLiveElems[1]);
}

// isElementWise : check if instruction does operation independently for each
// element
static bool isElementWise(const Instruction *I) {
  return isa<UnaryOperator>(I) || isa<BinaryOperator>(I) || isa<PHINode>(I) ||
         isa<CmpInst>(I) || isa<SelectInst>(I) || isa<CastInst>(I) ||
         GenXIntrinsicInfo(vc::getAnyIntrinsicID(I)).isElementWise();
}

// getOperandLiveElements : propagation function for instruction operand.
// Calculates what elements inside operand are required to correctly produce
// instruction result with live elements InstLiveElems
LiveElements GenXLiveElements::getOperandLiveElements(
    const Instruction *Inst, unsigned OperandNo,
    const LiveElements &InstLiveElems) const {
  IGC_ASSERT(OperandNo < Inst->getNumOperands());
  auto OpTy = Inst->getOperand(OperandNo)->getType();

  if (InstLiveElems.isAllDead() && !Inst->mayHaveSideEffects())
    return LiveElements(OpTy, false);

  if (auto BCI = dyn_cast<BitCastInst>(Inst))
    return getBitCastLiveElements(BCI, InstLiveElems);

  if (auto EVI = dyn_cast<ExtractValueInst>(Inst))
    return getExtractValueLiveElements(EVI, OperandNo, InstLiveElems);

  if (auto IVI = dyn_cast<InsertValueInst>(Inst))
    return getInsertValueLiveElements(IVI, OperandNo, InstLiveElems);

  auto *BO = dyn_cast<BinaryOperator>(Inst);
  if (BO && BO->getOpcode() == Instruction::And) {
    Value *OtherOp = BO->getOperand(1 - OperandNo);
    if (isa<ConstantAggregateZero>(OtherOp))
      return LiveElements(OpTy, false);
    if (auto *CDV = dyn_cast<ConstantDataVector>(OtherOp)) {
      IGC_ASSERT(InstLiveElems.size() == 1);
      LiveElements Result(InstLiveElems);
      for (unsigned Idx = 0; Idx < CDV->getNumElements(); Idx++)
        if (CDV->getElementAsConstant(Idx)->isZeroValue())
          Result[0].reset(Idx);
      return Result;
    }
  }

  auto ID = vc::getAnyIntrinsicID(Inst);
  if (GenXIntrinsic::isRdRegion(Inst) || ID == GenXIntrinsic::genx_rdpredregion)
    return getRdRegionLiveElements(Inst, OperandNo, InstLiveElems);

  if (GenXIntrinsic::isWrRegion(Inst) || ID == GenXIntrinsic::genx_wrpredregion)
    return getWrRegionLiveElements(Inst, OperandNo, InstLiveElems);

  if (ID == GenXIntrinsic::genx_addc || ID == GenXIntrinsic::genx_subb)
    return getTwoDstInstLiveElements(InstLiveElems);

  auto OpLiveElems = LiveElements(OpTy);
  if (isElementWise(Inst) && InstLiveElems.size() == OpLiveElems.size()) {
    bool EqualSize = true;
    for (unsigned Idx = 0; Idx < InstLiveElems.size(); Idx++)
      if (InstLiveElems[Idx].size() != OpLiveElems[Idx].size()) {
        EqualSize = false;
        break;
      }
    if (EqualSize)
      return InstLiveElems;
  }

  return LiveElements(OpTy, true);
}

// isRootInst : check if instruction should be the start point for backward
// propagation (i.e. always live)
static bool isRootInst(const Instruction *I) {
  if (I->isTerminator() || I->mayHaveSideEffects() || isa<DbgInfoIntrinsic>(I))
    return true;

  // Even if the whole region is overwritten by a chain of wrregions, wrregions
  // to predefined register must not be optimized as they are extremely
  // specific.
  if (GenXIntrinsic::isWrRegion(I) &&
      GenXIntrinsic::isReadPredefReg(
          I->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum)))
    return true;

  return false;
}

void GenXLiveElements::processFunction(const Function &F) {
  // List of instructions that live elements were changed and requires
  // re-processing. SetVector is used to obtain deterministic order of work
  SmallSetVector<const Instruction *, 16> Worklist;

  for (auto &I : instructions(F))
    if (isRootInst(&I)) {
      LLVM_DEBUG(dbgs() << "Adding:\n" << I << "\n");
      LiveMap.insert({&I, LiveElements(I.getType(), true)});
      Worklist.insert(&I);
    }

  while (!Worklist.empty()) {
    auto Inst = Worklist.pop_back_val();
    IGC_ASSERT(LiveMap.count(Inst));
    auto InstLiveElems = LiveMap[Inst];
    LLVM_DEBUG(dbgs() << "Visiting:\n" << *Inst << " " << InstLiveElems << "\n");
    // Estimate each operand
    for (auto &Op : Inst->operands()) {
      if (!isa<Instruction>(Op) && !isa<Argument>(Op))
        continue;
      LiveElements OldLiveElems(Op->getType());
      auto It = LiveMap.find(Op);
      if (It != LiveMap.end())
        OldLiveElems = It->second;
      auto NewLiveElems =
          OldLiveElems |
          getOperandLiveElements(Inst, Op.getOperandNo(), InstLiveElems);
      // Skip adding not-changed and fully dead operands
      if (NewLiveElems == OldLiveElems || NewLiveElems.isAllDead())
        continue;
      LLVM_DEBUG(dbgs() << "Changing:\n"
                        << *Op.get() << " " << NewLiveElems << "\n");
      LiveMap[Op] = std::move(NewLiveElems);
      if (auto OpInst = dyn_cast<Instruction>(Op))
        Worklist.insert(OpInst);
    }
  }

  if (PrintLiveElementsInfo) {
    outs() << "Live elements for " << F.getName() << ":\n";
    for (auto &I : instructions(F)) {
      outs() << I << " ";
      auto It = LiveMap.find(&I);
      if (It != LiveMap.end())
        outs() << It->second;
      else
        outs() << LiveElements(I.getType());
      outs() << "\n";
    }
  }
}

char GenXFuncLiveElements::ID = 0;

INITIALIZE_PASS_BEGIN(GenXFuncLiveElements, "GenXFuncLiveElements",
                      "GenXFuncLiveElements", false, false)
INITIALIZE_PASS_END(GenXFuncLiveElements, "GenXFuncLiveElements",
                    "GenXFuncLiveElements", false, false)

FunctionPass *llvm::createGenXFuncLiveElementsPass() {
  initializeGenXFuncLiveElementsPass(*PassRegistry::getPassRegistry());
  return new GenXFuncLiveElements();
}

INITIALIZE_PASS_BEGIN(GenXGroupLiveElementsWrapper, "GenXGroupLiveElements",
                      "GenXGroupLiveElementsWrapper", false, false)
INITIALIZE_PASS_END(GenXGroupLiveElementsWrapper,
                    "GenXGroupLiveElementsWrapper",
                    "GenXGroupLiveElementsWrapper", false, false)

ModulePass *llvm::createGenXGroupLiveElementsWrapperPass() {
  initializeGenXGroupLiveElementsWrapperPass(*PassRegistry::getPassRegistry());
  return new GenXGroupLiveElementsWrapper();
}
