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

void LiveElements::dump(raw_ostream &OS) const {
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

// isRootInst : check if instruction should be the start point for backward
// propagation (i.e. always live)
static bool isRootInst(const Instruction *I) {
  return I->isTerminator() || I->mayHaveSideEffects() ||
         isa<DbgInfoIntrinsic>(I);
}

// isElementWise : check if instruction does operation independently for each
// element
static bool isElementWise(const Instruction *I) {
  return isa<UnaryOperator>(I) || isa<BinaryOperator>(I) || isa<PHINode>(I) ||
         isa<CmpInst>(I) || isa<SelectInst>(I) || isa<CastInst>(I) ||
         GenXIntrinsicInfo(vc::getAnyIntrinsicID(I)).isElementWise();
}

// getBitCastLiveElements : propagation function for bitcast - scaling of
// destination bitmask to source size
static LiveElements getBitCastLiveElements(const BitCastInst *BCI,
                                           const LiveElements &InstLiveElems) {
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

  return LiveElements(SrcLiveBits);
}

// getExtractValueLiveElements : propagation function for extractvalue
static LiveElements
getExtractValueLiveElements(const ExtractValueInst *EVI, unsigned OperandNo,
                            const LiveElements &InstLiveElems) {
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
static LiveElements
getInsertValueLiveElements(const InsertValueInst *IVI, unsigned OperandNo,
                           const LiveElements &InstLiveElems) {
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
static LiveElements getRdRegionLiveElements(const Instruction *RdR,
                                            unsigned OperandNo,
                                            const LiveElements &InstLiveElems) {
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

  for (unsigned Idx = 0; Idx < DstLiveBits.size(); Idx++)
    if (DstLiveBits[Idx])
      SrcLiveBits.set(Indices[Idx]);

  return LiveElements(SrcLiveBits);
}

// getWrRegionLiveElements : propagation function of wrregion/wrpredregion
static LiveElements getWrRegionLiveElements(const Instruction *WrR,
                                            unsigned OperandNo,
                                            const LiveElements &InstLiveElems) {
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
        SrcLiveBits.reset(Idx);
    }
    return LiveElements(SrcLiveBits);
  }

  // Process NewValueOperand or PredicateOperand
  if (R.Indirect)
    return LiveElements(OpTy, true);

  unsigned SrcSize = 1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(OpTy))
    SrcSize = VT->getNumElements();

  SmallBitVector SrcLiveBits(SrcSize);
  auto Indices = R.getAccessIndices();
  for (unsigned Idx = 0; Idx < Indices.size(); Idx++)
    if (DstLiveBits[Indices[Idx]])
      SrcLiveBits.set(Idx);

  return LiveElements(SrcLiveBits);
}

// getTwoDstInstInstLiveElements : propagation function for intrinsics with
// two destinations (addc, subb)
static LiveElements
getTwoDstInstLiveElements(const LiveElements &InstLiveElems) {
  IGC_ASSERT(InstLiveElems.size() == 2);
  IGC_ASSERT(InstLiveElems[0].size() == InstLiveElems[1].size());
  return LiveElements(InstLiveElems[0] | InstLiveElems[1]);
}

// getOperandLiveElements : propagation function for instruction operand.
// Calculates what elements inside operand are required to correctly produce
// instruction result with live elements InstLiveElems
static LiveElements getOperandLiveElements(const Instruction *Inst,
                                           unsigned OperandNo,
                                           const LiveElements &InstLiveElems) {
  IGC_ASSERT(OperandNo < Inst->getNumOperands());
  auto OpTy = Inst->getOperand(OperandNo)->getType();

  if (InstLiveElems.isAllDead() && !Inst->mayHaveSideEffects())
    return LiveElements(OpTy);

  if (auto BCI = dyn_cast<BitCastInst>(Inst))
    return getBitCastLiveElements(BCI, InstLiveElems);

  if (auto EVI = dyn_cast<ExtractValueInst>(Inst))
    return getExtractValueLiveElements(EVI, OperandNo, InstLiveElems);

  if (auto IVI = dyn_cast<InsertValueInst>(Inst))
    return getInsertValueLiveElements(IVI, OperandNo, InstLiveElems);

  if (GenXIntrinsic::isRdRegion(Inst) ||
      vc::getAnyIntrinsicID(Inst) == GenXIntrinsic::genx_rdpredregion)
    return getRdRegionLiveElements(Inst, OperandNo, InstLiveElems);

  if (GenXIntrinsic::isWrRegion(Inst) ||
      vc::getAnyIntrinsicID(Inst) == GenXIntrinsic::genx_wrpredregion)
    return getWrRegionLiveElements(Inst, OperandNo, InstLiveElems);

  if (auto ID = vc::getAnyIntrinsicID(Inst);
      ID == GenXIntrinsic::genx_addc || ID == GenXIntrinsic::genx_subb)
    return getTwoDstInstLiveElements(InstLiveElems);

  if (isElementWise(Inst))
    return InstLiveElems;

  return LiveElements(OpTy, true);
}

void LiveElementsAnalysis::processFunction(const Function &F) {
  // List of instructions that live elements were changed and requires
  // re-processing. SetVector is used to obtain deterministic order of work
  SmallSetVector<const Instruction *, 16> Worklist;

  for (auto &I : instructions(F))
    if (isRootInst(&I)) {
      LLVM_DEBUG(dbgs() << "Adding\n" << I << "\n");
      LiveMap.insert({&I, LiveElements(I.getType(), true)});
      Worklist.insert(&I);
    }

  while (!Worklist.empty()) {
    auto Inst = Worklist.pop_back_val();
    IGC_ASSERT(LiveMap.count(Inst));
    auto InstLiveElems = LiveMap[Inst];
    LLVM_DEBUG(dbgs() << "Visiting\n" << *Inst << " " << InstLiveElems << "\n");
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
      LiveMap[Op] = NewLiveElems;
      if (auto OpInst = dyn_cast<Instruction>(Op))
        Worklist.insert(OpInst);
    }
  }

  if (PrintLiveElementsInfo)
    print(outs());
}

void LiveElementsAnalysis::print(raw_ostream &OS) const {
  OS << "Live elements:\n";
  for (auto LiveElem : LiveMap)
    OS << *LiveElem.first << ": " << LiveElem.second << '\n';
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
