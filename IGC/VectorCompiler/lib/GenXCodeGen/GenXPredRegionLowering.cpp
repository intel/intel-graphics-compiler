/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXPredRegionLowering
/// ----------------------
///
/// VISA does not have instructions to match rdpredregion and wrpredregion.
/// According to it, predicate operand supports neither offsets not regions
/// that's why predicate region intrinsics do not have VISA equivalents. In
/// some special cases rd/wr-predregion can be baled and those bales do have
/// VISA equivalents, but in the general case we have to replace these
/// intrinsics with a sequence of other instructions. There are a few
/// instructions in VISA that may set/read a predicate:
///   * SEL - can create a general vector variable from a predicate by selecting
///   ones or zeros according to the predicate:
///   * CMP - compares two general vector variables and sets the result to a
///   predicate.
/// VISA also has SETP and MOV instructions but they seems less useful since
/// read/write a general variable in a scalar form.
///
// clang-format off
///
/// There are three cases that this pass transforms:
///   * lone rdpredregion
///       p1 = <16 x i1> rdpredregion(<32 x i1> p0, i32 16)
///         --->
///       bale { // VISA SEL
///         p1 = <16 x i1> rdpredregion(<32 x i1> p0, i32 16)
///         g0 = select p1, <16 x i16> <i16 1, i16 1, ..., i16 1>, <16 x i16> zeroinitializer
///       }
///       p.new = icmp eq <16 x i16> g0, <16 x i16> <i16 1, i16 1, ..., i16 1> // VISA CMP
///
///   * lone wrpredregion
///       p1 = <32 x i1> wrpredregion(<32 x i1> p0, <16 x i1> p.new, i32 16)
///         --->
///       g0 = select p.new, <16 x i16> <i16 1, i16 1, ..., i16 1>, <16 x i16> zeroinitializer // VISA SEL
///       bale { // VISA CMP
///         p.new1 = icmp eq <16 x i16> g0, <16 x i16> <i16 1, i16 1, ..., i16 1>
///         p1 = <32 x i1> wrpredregion(<32 x i1> p0, <16 x i1> p.new1, i32 16)
///       }
///
///   * bale {
///       p1 = <16 x i1> rdpredregion(<32 x i1> p0, i32 16)
///       p3 = <32 x i1> wrpredregion(<32 x i1> p2, <16 x i1> p1, i32 16)
///     }
///         --->
///       bale { // VISA SEL
///         p1 = <16 x i1> rdpredregion(<32 x i1> p0, i32 16)
///         g0 = select p1, <16 x i16> <i16 1, i16 1, ..., i16 1>, <16 x i16>
///         zeroinitializer
///       }
///       bale { // VISA CMP
///         p.new = icmp eq <16 x i16> g0, <16 x i16> <i16 1, i16 1, ..., i16 1>
///         p3 = <32 x i1> wrpredregion(<32 x i1> p2, <16 x i1> p.new, i32 16)
///       }
///
// clang-format on
///
/// Note that the transformations described above preserve correct instructions'
/// width, so no additional legalization is required. It's important since in
/// practice only legalization generates these problematic rd/wr-predregions to
/// lower.
//===----------------------------------------------------------------------===//

#include "GenXBaling.h"
#include "GenXTargetMachine.h"
#include "Probe/Assertion.h"
#include "llvmWrapper/IR/Value.h"

#include <llvm/GenXIntrinsics/GenXIntrinsics.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>

using namespace llvm;

static cl::opt<bool>
    EnablePredRegionLowering("vc-lower-predregion", cl::init(true), cl::Hidden,
                             cl::desc("enables GenXPredRegionLowering pass"));

namespace {

// FIXME: this must be in vc-intrinsics
enum PredRegion {
  OldValueOperandNum = 0,
  RdIndexOperandNum = 1,

  NewValueOperandNum = 1,
  WrIndexOperandNum = 2
};

class GenXPredRegionLowering final
    : public FunctionPass,
      public InstVisitor<GenXPredRegionLowering> {
  GenXBaling *Baling = nullptr;
  std::vector<genx::Bale> RdPredBales;
  std::vector<genx::Bale> WrPredBales;
  std::vector<genx::Bale> RdWrPredBales;

  void processRdPredRegionBales();
  void processWrPredRegionBales();
  void processRdWrPredRegionBales();
  void processRdPredRegion(Instruction &RdPredRegion);
  void processWrPredRegion(Instruction &WrPredRegion);

public:
  static char ID;

  GenXPredRegionLowering() : FunctionPass(ID) {}

  StringRef getPassName() const override {
    return "GenX Rd/WrPredRegion lowering";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<GenXGroupBaling>();
    AU.addRequired<FunctionGroupAnalysis>();
    AU.setPreservesAll();
  }

  bool runOnFunction(Function &F) override;

  void visitCallInst(CallInst &CI);

  void clear() {
    RdPredBales.clear();
    WrPredBales.clear();
    RdWrPredBales.clear();
  }
};
} // namespace

char GenXPredRegionLowering::ID = 0;

INITIALIZE_PASS_BEGIN(GenXPredRegionLowering, "GenXPredRegionLowering",
                      "GenXPredRegionLowering", false, false);
INITIALIZE_PASS_DEPENDENCY(GenXGroupBalingWrapper)
INITIALIZE_PASS_END(GenXPredRegionLowering, "GenXPredRegionLowering",
                    "GenXPredRegionLowering", false, false);

namespace llvm {
FunctionPass *createGenXPredRegionLoweringPass() {
  initializeGenXPredRegionLoweringPass(*PassRegistry::getPassRegistry());
  return new GenXPredRegionLowering();
}
} // namespace llvm

static Value *getI16Splat(uint16_t Constant, unsigned NumElts,
                          IRBuilder<> &IRB) {
  ConstantInt *C = IRB.getInt16(Constant);
  return IRB.CreateVectorSplat(NumElts, C);
}

static Value *createSelect(Value &Pred, IRBuilder<> &IRB) {
  Type *PredTy = Pred.getType();
  IGC_ASSERT(PredTy->isVectorTy());
  IGC_ASSERT(PredTy->getScalarType()->isIntegerTy(1));
  unsigned NumElts = cast<IGCLLVM::FixedVectorType>(PredTy)->getNumElements();

  Value *AllOnes = getI16Splat(1, NumElts, IRB);
  Value *AllZeros = getI16Splat(0, NumElts, IRB);

  return IRB.CreateSelect(&Pred, AllOnes, AllZeros,
                          Pred.getName() + ".sel.predlower");
}

static Value *createPredWithICmp(Value &ConvertToPred, IRBuilder<> &IRB) {
  unsigned NumElts =
      cast<IGCLLVM::FixedVectorType>(ConvertToPred.getType())->getNumElements();
  Value *AllOnes = getI16Splat(1, NumElts, IRB);
  return IRB.CreateICmpEQ(&ConvertToPred, AllOnes,
                          ConvertToPred.getName() + ".pred.predlower");
}

void GenXPredRegionLowering::processRdPredRegion(Instruction &RdPredRegion) {
  IGC_ASSERT(vc::getAnyIntrinsicID(&RdPredRegion) ==
             GenXIntrinsic::genx_rdpredregion);
  IRBuilder<> IRB(RdPredRegion.getNextNode());
  IRB.SetCurrentDebugLocation(RdPredRegion.getDebugLoc());

  Value *Sel = createSelect(RdPredRegion, IRB);
  Value *NewPred = createPredWithICmp(*Sel, IRB);

  IGCLLVM::replaceUsesWithIf(&RdPredRegion, NewPred,
                             [Sel](Use &U) { return U.getUser() != Sel; });

  Baling->processInst(&RdPredRegion);
  Baling->processInst(cast<Instruction>(Sel));
  Baling->processInst(cast<Instruction>(NewPred));
}

void GenXPredRegionLowering::processRdPredRegionBales() {
  for (genx::Bale &B : RdPredBales) {
    Instruction *RdPredRegion = B.front().Inst;
    IGC_ASSERT(RdPredRegion);

    processRdPredRegion(*RdPredRegion);
  }
}

void GenXPredRegionLowering::processWrPredRegionBales() {
  for (genx::Bale &B : WrPredBales) {
    Instruction *WrPredRegion = B.front().Inst;
    IGC_ASSERT(WrPredRegion);

    processWrPredRegion(*WrPredRegion);
  }
}

void GenXPredRegionLowering::processWrPredRegion(Instruction &WrPredRegion) {
  IGC_ASSERT(vc::getAnyIntrinsicID(&WrPredRegion) ==
             GenXIntrinsic::genx_wrpredregion);
  IRBuilder<> IRB(&WrPredRegion);

  Value *NewPred = WrPredRegion.getOperand(PredRegion::NewValueOperandNum);
  Value *Sel = createSelect(*NewPred, IRB);
  NewPred = createPredWithICmp(*Sel, IRB);

  WrPredRegion.setOperand(PredRegion::NewValueOperandNum, NewPred);

  Baling->unbale(&WrPredRegion);
  Baling->processInst(cast<Instruction>(Sel));
  Baling->processInst(cast<Instruction>(NewPred));
  Baling->processInst(&WrPredRegion);
}

void GenXPredRegionLowering::processRdWrPredRegionBales() {
  for (genx::Bale &B : RdWrPredBales) {
    Instruction *RdPredRegion = B.front().Inst;
    IGC_ASSERT(RdPredRegion);

    processRdPredRegion(*RdPredRegion);

    Instruction *WrPredRegion = B.getHead()->Inst;
    IGC_ASSERT(vc::getAnyIntrinsicID(WrPredRegion) ==
               GenXIntrinsic::genx_wrpredregion);
    Baling->unbale(WrPredRegion);
    Baling->processInst(WrPredRegion);
  }
}

// We are collecting only lone rdpredregion, lone wrpredregion and bale of rd
// and wr predregons. So, it is enough to iterate over call instructions.
void GenXPredRegionLowering::visitCallInst(CallInst &CI) {
  // Iterate over heads of bales.
  if (Baling->isBaled(&CI))
    return;

  genx::Bale B;
  Baling->buildBale(&CI, &B);
  // Do not process a bale if it has instruction other than rd/wr-predregion.
  if (std::any_of(B.begin(), B.end(), [](genx::BaleInst &BI) {
        return BI.Info.Type != genx::BaleInfo::RDPREDREGION &&
               BI.Info.Type != genx::BaleInfo::WRPREDREGION;
      }))
    return;
  bool HasRdPredRegion =
      std::any_of(B.begin(), B.end(), [](genx::BaleInst &BI) {
        return BI.Info.Type == genx::BaleInfo::RDPREDREGION;
      });
  bool HasWrPredRegion =
      std::any_of(B.begin(), B.end(), [](genx::BaleInst &BI) {
        return BI.Info.Type == genx::BaleInfo::WRPREDREGION;
      });

  if (HasWrPredRegion && HasRdPredRegion)
    RdWrPredBales.push_back(std::move(B));
  else if (HasWrPredRegion)
    WrPredBales.push_back(std::move(B));
  else if (HasRdPredRegion)
    RdPredBales.push_back(std::move(B));
}

bool GenXPredRegionLowering::runOnFunction(Function &F) {
  if (!EnablePredRegionLowering)
    return false;

  bool Changed = false;

  auto *FGA = &getAnalysis<FunctionGroupAnalysis>();
  Baling = &(getAnalysis<GenXGroupBalingWrapper>())
                .getFGPassImpl(FGA->getAnyGroup(&F));

  visit(F);

  processRdPredRegionBales();
  processWrPredRegionBales();
  processRdWrPredRegionBales();

  clear();
  return Changed;
}
