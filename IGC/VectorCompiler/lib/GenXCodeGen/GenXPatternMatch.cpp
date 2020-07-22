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
/// GenXPatternMatch
/// ----------------
///
/// This pass performs a small number of GenX-specific peephole optimizations.
///
/// It is named *pattern match* with the idea that it is analogous to the
/// pattern matching pass in IGC. However IGC's pattern matching is more
/// extensive, and I believe some of its functionality is covered by GenXBaling
/// in the GenX backend.
///
/// * Turns fp and integer mul+add into mad, if it decides it is profitable.
///
///   For an integer mul+add, the pass looks at the inputs after accounting for
///   extends that will get baled into the operation in the GenX backend, or
///   folded into the instruction in the finalizer, and it uses mad only if both
///   inputs are short or byte. Our experience on HSW was that using int mad
///   where the inputs are actually 32 bit ints is counterproductive because of
///   the way that the finalizer has to implement it using the hardware's 32x16
///   multiply.
///
///   However, this criterion could probably be looser on any arch that has a
///   32x32 multiply (BDW+, but excluding some later LP variants). This is
///   something to investigate.
///
///   To implement this, the pass would need to use GenXSubtarget, and there
///   would need to be a has32x32Multiply flag in GenXSubtarget.
///
/// * Turns cmp+sel into min/max if possible.
///
/// * Flips a boolean not if profitable.
///
/// * Cleanup predicate region reads if possible.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "GENX_PATTERN_MATCH"
#include "GenX.h"
#include "GenXConstants.h"
#include "GenXModule.h"
#include "GenXRegion.h"
#include "GenXSubtarget.h"
#include "GenXUtil.h"
#include "GenXVectorDecomposer.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/TargetFolder.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/GenXIntrinsics/GenXIntrinsicInst.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/Utils/Local.h"

#include <functional>
#include <limits>

using namespace llvm;
using namespace llvm::PatternMatch;
using namespace genx;

STATISTIC(NumOfMadMatched, "Number of mad instructions matched");
STATISTIC(NumOfMinMaxMatched, "Number of min/max instructions matched");

static cl::opt<bool> EnableMadMatcher("enable-mad", cl::init(true), cl::Hidden,
                                      cl::desc("Enable mad matching."));

static cl::opt<bool> EnableMinMaxMatcher("enable-minmax", cl::init(true),
                                         cl::Hidden,
                                         cl::desc("Enable min/max matching."));

namespace {

class GenXPatternMatch : public FunctionPass,
                         public InstVisitor<GenXPatternMatch> {
  DominatorTree *DT = nullptr;
  LoopInfo *LI = nullptr;
  const DataLayout *DL = nullptr;
  const TargetOptions *Options;
  // Indicates whether there is any change.
  bool Changed = false;

public:
  static char ID;
  GenXPatternMatch(const TargetOptions *Options = nullptr)
      : FunctionPass(ID), Options(Options) {}

  StringRef getPassName() const override { return "GenX pattern match"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addPreserved<GenXModule>();
    AU.setPreservesCFG();
  }

  void visitBinaryOperator(BinaryOperator &I);

  void visitCallInst(CallInst &I);

  void visitSelectInst(SelectInst &I);

  void visitFDiv(BinaryOperator &I);

  void visitICmpInst(ICmpInst &I);

  bool runOnFunction(Function &F) override;

  bool isFpMadEnabled() const {
    return EnableMadMatcher &&
           (!Options || Options->AllowFPOpFusion != FPOpFusion::Strict);
  }

private:
  // flipBoolNot : flip a (vector) bool not instruction if beneficial
  bool flipBoolNot(Instruction *Inst);
  // foldBoolAnd : fold a (vector) bool and into sel/wrregion if beneficial
  bool foldBoolAnd(Instruction *Inst);
  bool simplifyPredRegion(CallInst *Inst);
  bool simplifyWrRegion(CallInst *Inst);
  bool simplifyRdRegion(CallInst* Inst);
  bool simplifyTruncSat(CallInst *Inst);
  bool simplifySelect(Function *F);
  bool simplifyVolatileGlobals(Function *F);
  bool decomposeSelect(Function *F);
  // Preprocessing to help generate integer MAD.
  bool distributeIntegerMul(Function *F);
  bool propagateFoldableRegion(Function *F);
  bool reassociateIntegerMad(Function *F);
  bool vectorizeConstants(Function *F);
  bool placeConstants(Function *F);
  bool simplifyCmp(CmpInst *Cmp);
  CmpInst *reduceCmpWidth(CmpInst *Cmp);
  bool simplifyNullDst(CallInst *Inst);
  // Transform logic operation with a mask from <N x iM> to <N/(32/M) x i32>
  bool extendMask(BinaryOperator *BO);
};

} // namespace

char GenXPatternMatch::ID = 0;

namespace llvm {
void initializeGenXPatternMatchPass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXPatternMatch, "GenXPatternMatch", "GenXPatternMatch",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_END(GenXPatternMatch, "GenXPatternMatch", "GenXPatternMatch",
                    false, false)

FunctionPass *llvm::createGenXPatternMatchPass(const TargetOptions *Options) {
  initializeGenXPatternMatchPass(*PassRegistry::getPassRegistry());
  return new GenXPatternMatch(Options);
}

bool GenXPatternMatch::runOnFunction(Function &F) {
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  DL = &F.getParent()->getDataLayout();

  // Before we get the simd-control-flow representation right,
  // we avoid dealing with predicate constants
  auto P = getAnalysisIfAvailable<GenXSubtargetPass>();
  const GenXSubtarget *ST = P ? P->getSubtarget() : nullptr;
  loadPhiConstants(&F, DT, true, ST);
  Changed |= distributeIntegerMul(&F);
  Changed |= propagateFoldableRegion(&F);
  Changed |= reassociateIntegerMad(&F);
  Changed |= placeConstants(&F);
  Changed |= vectorizeConstants(&F);

  visit(F);

  Changed |= simplifyVolatileGlobals(&F);
  Changed |= simplifySelect(&F);
  // Break big predicate variables and run after min/max pattern match.
  Changed |= decomposeSelect(&F);

  return Changed;
}

namespace {

// Helper class to share common code.
class MadMatcher {
public:
  explicit MadMatcher(Instruction *I)
      : AInst(I), MInst(nullptr), ID(GenXIntrinsic::not_any_intrinsic), NegIndex(-1) {
    assert(I && "null instruction");
    Srcs[0] = Srcs[1] = Srcs[2] = nullptr;
  }

  // Match mads with floating point operands.
  bool matchFpMad();

  // Match integer mads that starts with binary operators.
  bool matchIntegerMad();

  // Match integer mads that starts with genx_*add intrinsic calls.
  bool matchIntegerMad(unsigned IID);

private:
  // Return true if changes are made.
  bool emit();

  // Check whether it is profitable to emit a mad.
  //
  // Each mad out of add implies a duplicated mul and jitter usually can not
  // remove it in the end.
  //
  // It is a bit more subtle for the integer case. Since 32 bit mul is not well
  // supported in HW, it may lead to worse code if a 32 bit integer mad cannot
  // be emitted as mac in the end and mul + mach could be emitted.
  bool isProfitable() const;

  // Checks whether a fp mad is being matched or not.
  bool isFpMad() const { return ID == Intrinsic::fma; }

  void setMInst(Instruction *I) { MInst = I; }

  // Checks whether 'MInst' is an integer shift, which could be turned back to
  // an integer muliplication.
  bool isLShift() const { return MInst->getOpcode() == Instruction::Shl; }

  std::tuple<Value *, bool> getNarrowI16Vector(IRBuilder<> &, Instruction *,
                                               Value *, unsigned) const;

private:
  // The instruction starts the mad matching:
  // * fadd/fsub
  // * add/sub
  // * genx_*add
  Instruction *AInst;

  // The instruction being sinked into:
  // * fmul
  // * mul/shl
  // * genx_*mul
  Instruction *MInst;

  // The mad intrinsic ID.
  unsigned ID;

  // Source operands for the mad intrinsic call, representing mad as
  // srcs[0] * srcs[1] + srcs[2].
  Value *Srcs[3];

  // Indicates whether Srcs[NegIndex] needs to be negated. Value -1 means no
  // negation is needed.
  int NegIndex;
};



// Class to identify cases where a comparison and select are equivalent to a
// min or max operation. These are replaced by a min/max intrinsic which allows
// the jitter to produce better code for these cases.
class MinMaxMatcher {
public:
  explicit MinMaxMatcher(Instruction *I)
      : SelInst(I), CmpInst(nullptr), ID(GenXIntrinsic::not_any_intrinsic) {
    assert(I && "null instruction");
    Srcs[0] = Srcs[1] = nullptr;
    Annotation = 0;
  }

  // Match select instruction that are equivalent to min/max
  bool matchMinMax();

  bool valuesMatch(llvm::Value *Op1, llvm::Value *Op2);

  static bool isEnabled() { return EnableMinMaxMatcher; }

private:
  // Return true if changes are made.
  bool emit();

  void setSelInst(Instruction *I) { SelInst = I; }

private:
  // The select instruction
  Instruction *SelInst;

  // The compare instruction
  llvm::CmpInst *CmpInst;

  // The min/max intrinsic ID.
  unsigned ID;

  // Source operands for the min/max intrinsic call
  Value *Srcs[2];

  // Effective operands for the cmp ignoring some casts
  Value *CmpSrcs[2];

  // Annotation for the min/max call
  const char *Annotation;
};

} // namespace

void GenXPatternMatch::visitBinaryOperator(BinaryOperator &I) {
  auto P = getAnalysisIfAvailable<GenXSubtargetPass>();
  const GenXSubtarget *ST = P ? P->getSubtarget() : nullptr;
  if (isPredNot(&I))
    Changed |= flipBoolNot(&I);
  else
    switch (I.getOpcode()) {
    default:
      break;
    case Instruction::FAdd:
    case Instruction::FSub:
      Changed |= isFpMadEnabled() && MadMatcher(&I).matchFpMad();
      break;
    case Instruction::Add:
    case Instruction::Sub:
      if (EnableMadMatcher && MadMatcher(&I).matchIntegerMad())
        Changed = true;
      break;
    case Instruction::And:
      if (I.getType()->getScalarType()->isIntegerTy(1)) {
        if (foldBoolAnd(&I))
          Changed = true;
      } else if (extendMask(&I))
        Changed = true;
      break;
    case Instruction::Or:
    case Instruction::Xor:
      if (extendMask(&I))
        Changed = true;
      break;
    }
}

void GenXPatternMatch::visitCallInst(CallInst &I) {
  auto P = getAnalysisIfAvailable<GenXSubtargetPass>();
  const GenXSubtarget *ST = P ? P->getSubtarget() : nullptr;
  switch (unsigned ID = GenXIntrinsic::getGenXIntrinsicID(&I)) {
  default:
    break;
  case GenXIntrinsic::genx_ssadd_sat:
  case GenXIntrinsic::genx_suadd_sat:
  case GenXIntrinsic::genx_usadd_sat:
  case GenXIntrinsic::genx_uuadd_sat:
    if (EnableMadMatcher && MadMatcher(&I).matchIntegerMad(ID))
      Changed = true;
    break;
  case GenXIntrinsic::genx_rdpredregion:
    Changed |= simplifyPredRegion(&I);
    break;
  case GenXIntrinsic::genx_wrregioni:
  case GenXIntrinsic::genx_wrregionf:
    Changed |= simplifyWrRegion(&I);
    break;
  case GenXIntrinsic::genx_rdregioni:
  case GenXIntrinsic::genx_rdregionf:
    Changed |= simplifyRdRegion(&I);
    break;
  case GenXIntrinsic::genx_sstrunc_sat:
  case GenXIntrinsic::genx_sutrunc_sat:
  case GenXIntrinsic::genx_ustrunc_sat:
  case GenXIntrinsic::genx_uutrunc_sat:
    Changed |= simplifyTruncSat(&I);
    break;
  case GenXIntrinsic::genx_dword_atomic_add:
  case GenXIntrinsic::genx_dword_atomic_and:
  case GenXIntrinsic::genx_dword_atomic_cmpxchg:
  case GenXIntrinsic::genx_dword_atomic_dec:
  case GenXIntrinsic::genx_dword_atomic_fcmpwr:
  case GenXIntrinsic::genx_dword_atomic_fmax:
  case GenXIntrinsic::genx_dword_atomic_fmin:
  case GenXIntrinsic::genx_dword_atomic_imax:
  case GenXIntrinsic::genx_dword_atomic_imin:
  case GenXIntrinsic::genx_dword_atomic_max:
  case GenXIntrinsic::genx_dword_atomic_min:
  case GenXIntrinsic::genx_dword_atomic_or:
  case GenXIntrinsic::genx_dword_atomic_sub:
  case GenXIntrinsic::genx_dword_atomic_xchg:
  case GenXIntrinsic::genx_dword_atomic_xor:
    Changed |= simplifyNullDst(&I);
    break;
  }
}

void GenXPatternMatch::visitICmpInst(ICmpInst &I) {
  // Ignore dead comparison.
  if (I.use_empty())
    return;

  Value *V0 = nullptr;
  Constant *C1 = nullptr;
  Constant *C2 = nullptr;
  ICmpInst::Predicate Pred = CmpInst::BAD_ICMP_PREDICATE;

  // Transform icmp (V0 & 65535), C2 ==> icmp (trunc V0 to i16), C2.
  // TODO: Only consider unsigned comparisons so do not inspect the sign bit.
  if (I.isUnsigned() &&
      match(&I, m_ICmp(Pred, m_OneUse(m_And(m_Value(V0), m_Constant(C1))),
                       m_Constant(C2))) &&
      C1->getType()->isVectorTy()) {
    Type *Ty = V0->getType();
    if (auto Elt = dyn_cast_or_null<ConstantInt>(C1->getSplatValue())) {
      auto Known = computeKnownBits(C2, *DL);
      unsigned NBits = Known.Zero.countLeadingOnes();

      IRBuilder<> Builder(&I);
      uint64_t Int16Mask = std::numeric_limits<uint16_t>::max();
      uint64_t Int8Mask = std::numeric_limits<uint8_t>::max();

      // Check if it is safe to truncate to lower type without loss of bits.
      Type *DstTy = nullptr;
      uint64_t Val = Elt->getZExtValue();
      unsigned NElts = Ty->getVectorNumElements();
      unsigned BitWidth = Elt->getType()->getPrimitiveSizeInBits();
      if (Val == Int16Mask && NBits + 16 >= BitWidth)
        DstTy = VectorType::get(Builder.getInt16Ty(), NElts);
      else if (Val == Int8Mask && NBits + 8 >= BitWidth)
        DstTy = VectorType::get(Builder.getInt8Ty(), NElts);

      // Lower trunc to bitcast followed by a region read
      // as such bitcast is not support after IR lowering.
      if (DstTy) {
        Type *InEltTy = Ty->getVectorElementType();
        Type *OutEltTy = DstTy->getVectorElementType();
        assert(OutEltTy->getPrimitiveSizeInBits());
        unsigned Stride = InEltTy->getPrimitiveSizeInBits() /
                          OutEltTy->getPrimitiveSizeInBits();
        // Create the new bitcast.
        Instruction *BC = CastInst::Create(
            Instruction::BitCast, V0, VectorType::get(OutEltTy, Stride * NElts),
            ".bc", &I /*InsertBefore*/);
        BC->setDebugLoc(I.getDebugLoc());

        // Create the new rdregion.
        Region R(BC);
        R.NumElements = NElts;
        R.Stride = Stride;
        R.Width = NElts;
        R.VStride = R.Stride * R.Width;
        Value *LHS = R.createRdRegion(BC, "", &I /*InsertBefore*/,
                                      I.getDebugLoc(), false /*AllowScalar*/);
        Value *RHS = Builder.CreateTrunc(C2, DstTy);
        assert(isa<Constant>(RHS));
        Value *NewICmp = Builder.CreateICmp(Pred, LHS, RHS);
        if (auto Inst = dyn_cast<Instruction>(NewICmp))
          Inst->setDebugLoc(I.getDebugLoc());
        I.replaceAllUsesWith(NewICmp);
        Changed = true;
      }
    }
  }

  // Explore (icmp.ne V0, 0) where V0 is promoted from i1.
  if (match(&I, m_ICmp(Pred, m_Value(V0), m_Zero())) &&
      Pred == CmpInst::ICMP_NE) {
    // V0 is calculated from AND, OR, NOT, and (select (cmp ...), 0, 1)
    SmallVector<Value *, 8> WorkList;
    SmallVector<Value *, 8> PreOrder;
    bool Profitable = true;
    WorkList.push_back(V0);
    while (!WorkList.empty()) {
      Value *V = WorkList.pop_back_val();
      Value *LHS = nullptr, *RHS = nullptr;
      if (match(V, m_OneUse(m_Or(m_Value(LHS), m_Value(RHS))))) {
        WorkList.push_back(LHS);
        WorkList.push_back(RHS);
        PreOrder.push_back(V);
        continue;
      }
      if (match(V, m_OneUse(m_And(m_Value(LHS), m_Value(RHS))))) {
        WorkList.push_back(LHS);
        WorkList.push_back(RHS);
        PreOrder.push_back(V);
        continue;
      }
      if (match(V, m_OneUse(m_Not(m_Value(LHS))))) {
        WorkList.push_back(LHS);
        PreOrder.push_back(V);
        continue;
      }
      Value *Cond = nullptr;
      if (match(V, m_OneUse(m_Select(m_Value(Cond), m_One(), m_Zero())))) {
        PreOrder.push_back(Cond);
        continue;
      }
      Profitable = false;
      break;
    }
    if (Profitable) {
      IRBuilder<> Builder(&I);
      // For simplicity, a stack is used to reconstruct tree. With a next
      // pointer, that stack is not necessary.
      SmallVector<Value *, 8> OpStack;
      while (!PreOrder.empty()) {
        Value *V = PreOrder.pop_back_val();
        if (V->getType()->getScalarType()->isIntegerTy(1)) {
          OpStack.push_back(V);
          continue;
        }
        Value *LHS, *RHS;
        if (match(V, m_Or(m_Value(LHS), m_Value(RHS)))) {
          assert(OpStack.size() >= 2);
          RHS = OpStack.pop_back_val();
          LHS = OpStack.pop_back_val();
          OpStack.push_back(Builder.CreateOr(LHS, RHS));
          continue;
        }
        if (match(V, m_And(m_Value(LHS), m_Value(RHS)))) {
          assert(OpStack.size() >= 2);
          RHS = OpStack.pop_back_val();
          LHS = OpStack.pop_back_val();
          OpStack.push_back(Builder.CreateAnd(LHS, RHS));
          continue;
        }
        if (match(V, m_Not(m_Value(LHS)))) {
          assert(OpStack.size() >= 1);
          LHS = OpStack.pop_back_val();
          OpStack.push_back(Builder.CreateNot(LHS));
        }
        assert(false && "Unhandled logic op!");
      }
      assert(OpStack.size() == 1);
      I.replaceAllUsesWith(OpStack.pop_back_val());
      Changed = true;
      return;
    }
  }

  // Skip the following optimization specific to scalar comparison.
  if (!I.getType()->isIntegerTy(1))
    return;

  // Transform the evaluation of flag == 0 into (~flag).all().
  // TODO: Transform flag != 0 into flag.any().
  if (match(&I, m_ICmp(Pred, m_OneUse(m_BitCast(m_OneUse(m_Value(V0)))),
                       m_Zero())) &&
      Pred == CmpInst::ICMP_EQ && isa<CmpInst>(V0) &&
      V0->getType()->isVectorTy() &&
      V0->getType()->getScalarType()->isIntegerTy(1)) {
    VectorType *VTy = cast<VectorType>(V0->getType());
    unsigned NumElts = VTy->getNumElements();
    if (NumElts == 2 || NumElts == 4 || NumElts == 8 || NumElts == 16) {
      IRBuilder<> Builder(&I);
      auto Cmp = cast<CmpInst>(V0);
      // Inverse the evaluation of flag.
      Cmp->setPredicate(Cmp->getInversePredicate());
      if (auto NewCmp = reduceCmpWidth(Cmp)) {
        // Once the cmp could be reduced into narrower one (with the assumption
        // that the reduced part is always TRUE), reduce it into narrow one.
        Cmp = NewCmp;
        VTy = cast<VectorType>(Cmp->getType());
      }
      simplifyCmp(Cmp);
      // Call 'all'.
      auto M = I.getParent()->getParent()->getParent();
      auto Fn = GenXIntrinsic::getGenXDeclaration(M, GenXIntrinsic::genx_all, VTy);
      auto NewVal = Builder.CreateCall(Fn, Cmp);
      I.replaceAllUsesWith(NewVal);
      Changed = true;
      return;
    }
  }
}

// Simplify the sequence of (cmp.eq (and (wrregion zero v), 1), 0) to
// (cmp.eq (and v, 1), 0) with a narrow vector length with the assumption that
// the reduced part will be always TRUE.
CmpInst *GenXPatternMatch::reduceCmpWidth(CmpInst *Cmp) {
  ICmpInst::Predicate Pred = CmpInst::BAD_ICMP_PREDICATE;
  Value *V0 = nullptr;
  if (!Cmp->hasOneUse() || !Cmp->getType()->isVectorTy() ||
      !match(Cmp, m_ICmp(Pred, m_And(m_Value(V0), m_One()), m_Zero())) ||
      Pred != CmpInst::ICMP_EQ || !GenXIntrinsic::isWrRegion(V0))
    return nullptr;

  GenXIntrinsicInst *WII = cast<GenXIntrinsicInst>(V0);
  if (!match(WII->getOperand(0), m_Zero()))
    return nullptr;

  V0 = WII->getOperand(1);
  VectorType *VTy = cast<VectorType>(V0->getType());
  unsigned NumElts = VTy->getNumElements();

  Region R(WII, BaleInfo());
  if (R.Indirect || R.Offset || R.VStride || R.Stride != 1 ||
      R.Width != NumElts)
    return nullptr;
  if (R.Width != 2 && R.Width != 4 && R.Width != 8 && R.Width != 16)
    return nullptr;

  // As the rest parts of the original vector are all zeros, the sequence could
  // be reduced into a narrower one (R.Width) and skip the wrregion.
  IRBuilder<> Builder(Cmp);

  auto One = ConstantInt::get(VTy, 1);
  auto Zero = Constant::getNullValue(VTy);

  auto V1 = Builder.CreateAnd(V0, One);
  auto V2 = Builder.CreateICmp(Pred, V1, Zero);

  return cast<CmpInst>(V2);
}

// Simplify the sequence of (cmp (and (select (cmp ...) 1, 0), 1), 0)
bool GenXPatternMatch::simplifyCmp(CmpInst *Cmp) {
  ICmpInst::Predicate P0 = ICmpInst::BAD_ICMP_PREDICATE;
  ICmpInst::Predicate P1 = ICmpInst::BAD_ICMP_PREDICATE;
  Value *LHS = nullptr;
  Value *RHS = nullptr;
  if (!match(Cmp, m_ICmp(P0,
                         m_And(m_Select(m_ICmp(P1, m_Value(LHS), m_Value(RHS)),
                                        m_One(), m_Zero()),
                               m_One()),
                         m_Zero())))
    return false;
  if (P0 != ICmpInst::ICMP_EQ && P0 != ICmpInst::ICMP_NE)
    return false;
  if (P0 == ICmpInst::ICMP_EQ)
    P1 = ICmpInst::getInversePredicate(P1);
  Cmp->setPredicate(P1);
  Cmp->setOperand(0, LHS);
  Cmp->setOperand(1, RHS);
  return true;
}

/***********************************************************************
 * notHasRealUse : detect whether an instruction has a use that counts as
 *      a "real" use of a bool not, that is one where it would need to be
 *      calculated rather than just baled in
 */
static bool notHasRealUse(Instruction *Inst) {
  for (auto ui = Inst->use_begin(), ue = Inst->use_end(); ui != ue; ++ui) {
    auto user = cast<Instruction>(ui->getUser());
    if (isPredNot(user))
      continue;
    if (isa<SelectInst>(user))
      continue;
    if (user->use_empty())
      continue; // ignore dead instruction
    switch (GenXIntrinsic::getGenXIntrinsicID(user)) {
    case GenXIntrinsic::genx_any:
    case GenXIntrinsic::genx_all:
    case GenXIntrinsic::genx_wrregioni:
    case GenXIntrinsic::genx_wrregionf:
      continue;
    default:
      return true;
    }
  }
  return false;
}

/***********************************************************************
 * GenXPatternMatch::flipBoolNot : attempt to flip (vector) bool not
 *
 * A vector bool not is bad if its value actually needs to be calculated,
 * as opposed to just baling it into a predicate field. In gen code,
 * calculating it involves using a sel to get it into a GRF, then doing
 * an xor that sets flags. Here we call any use that requires it to be
 * calculated a "real" use.
 *
 * This code detects the case that:
 * 1. the not has at least one "real" use
 * 2. the input to the not is the result of a cmp and does not have any
 *    "real" use.
 * If these conditions hold, then we flip the not by inverting the
 * cmp and replacing uses of the not with the new inverted cmp. If the
 * original cmp has any uses other than the original not, then we create
 * a new not and change uses to that.
 *
 * In this way we save an actual calculation of the original not.
 *
 * We only do this for a v16i1 or smaller.
 */
bool GenXPatternMatch::flipBoolNot(Instruction *Inst) {
  if (Inst->getType()->getPrimitiveSizeInBits() > 16)
    return false; // too big
  auto Input = dyn_cast<CmpInst>(Inst->getOperand(0));
  if (!Input)
    return false; // input not cmp
  if (!notHasRealUse(Inst))
    return false; // result of not has no "real" use
  if (notHasRealUse(Input))
    return false; // input has a "real" use, so we don't want to flip
  // We want to flip the not by inverting the comparison that generates its
  // input.
  CmpInst::Predicate InversedPred = Input->getInversePredicate();
  // We do not have a support of unordered comparisons except UNE.
  if (CmpInst::isUnordered(InversedPred) && InversedPred != CmpInst::FCMP_UNE)
    return false;
  // ORD and ONE are not supported.
  if (InversedPred == CmpInst::FCMP_ORD || InversedPred == CmpInst::FCMP_ONE)
    return false;
  auto NewCmp = CmpInst::Create(Input->getOpcode(), InversedPred,
                                Input->getOperand(0), Input->getOperand(1),
                                Input->getName() + ".inversed", Input);
  NewCmp->setDebugLoc(Input->getDebugLoc());
  Inst->replaceAllUsesWith(NewCmp);
  if (!Input->use_empty()) {
    auto NewNot = BinaryOperator::Create(
        Instruction::Xor, NewCmp, Constant::getAllOnesValue(NewCmp->getType()),
        "", Input);
    NewNot->setDebugLoc(Input->getDebugLoc());
    NewNot->takeName(Inst);
    Input->replaceAllUsesWith(NewNot);
  }
  return true;
}

/***********************************************************************
 * foldBoolAnd : fold a (vector) bool and into sel/wrregion if beneficial
 *
 * A bool and takes a sequence of 3 gen instructions. Here we detect if
 * a bool and has a single use in a select or wrregion, and if so we fold
 * it in to have two selects or rdregion, select, wrregion respectively.
 *
 * We only do this for a v16i1 or smaller.
 */
bool GenXPatternMatch::foldBoolAnd(Instruction *Inst) {
  if (Inst->getType()->getPrimitiveSizeInBits() > 16)
    return false; // too big
  if (!isa<VectorType>(Inst->getType()))
    return false; // too small
  if (!Inst->hasOneUse())
    return false; // more than one use
  auto user = cast<Instruction>(Inst->use_begin()->getUser());
  if (auto Sel = dyn_cast<SelectInst>(user)) {
    // Fold and into sel.
    auto NewSel1 = SelectInst::Create(Inst->getOperand(0), Sel->getOperand(1),
                                      Sel->getOperand(2),
                                      Sel->getName() + ".foldand", Sel);
    NewSel1->setDebugLoc(Sel->getDebugLoc());
    auto NewSel2 = SelectInst::Create(Inst->getOperand(1), NewSel1,
                                      Sel->getOperand(2), "", Sel);
    NewSel2->takeName(Sel);
    NewSel2->setDebugLoc(Sel->getDebugLoc());
    Sel->replaceAllUsesWith(NewSel2);
    return true;
  }
  if (!GenXIntrinsic::isWrRegion(user))
    return false;
  // Fold and into wrregion, giving rdregion, select and wrregion, as long
  // as the original wrregion is not indirect.
  Region R(user, BaleInfo());
  if (R.Indirect)
    return false;
  auto NewRdRegion =
      R.createRdRegion(user->getOperand(0), user->getName() + ".foldand1", user,
                       user->getDebugLoc(), false);
  auto NewSel =
      SelectInst::Create(Inst->getOperand(0), user->getOperand(1), NewRdRegion,
                         user->getName() + ".foldand2", user);
  NewSel->setDebugLoc(user->getDebugLoc());
  R.Mask = Inst->getOperand(1);
  auto NewWrRegion = cast<Instruction>(R.createWrRegion(
      user->getOperand(0), NewSel, "", user, user->getDebugLoc()));
  NewWrRegion->takeName(user);
  user->replaceAllUsesWith(NewWrRegion);
  return true;
}

void GenXPatternMatch::visitSelectInst(SelectInst &I) {
  Changed |= MinMaxMatcher::isEnabled() && MinMaxMatcher(&I).matchMinMax();
}

// Trace the def-use chain and return the first non up-cast related value.
static Value *getEffectiveValueUp(Value *V) {
  if (isa<ZExtInst>(V) || isa<SExtInst>(V) || isa<BitCastInst>(V))
    return getEffectiveValueUp(cast<Instruction>(V)->getOperand(0));

  return V;
}

// Determine whether it is profitable to match a mad. This function assumes
// that it is valid to match.
bool MadMatcher::isProfitable() const {
  // Do not match unused instructions.
  if (AInst->use_empty())
    return false;

  // For the following case,
  // %m = mul %a, %b
  // %a1 = add %m, %c1
  // %a2 = add %m, %c2
  //
  // If we match them into two mads as
  //
  // %m1 = mad(%a, %b, %c1)
  // %m2 = mad(%a, %b, %c2)
  //
  // and it fails to emit two mac/mads then there are redundant instructions in
  // the end. Conservatively, only match when there is a single use for MInst.
  //
  // Update: There are enough cases where this transformation helps spilling
  // (particularly for long sequences) that mean it is of more value to enable
  // multiple use cases. May need to revisit. if (!MInst->hasOneUse())
  //   return false;

  // Do not match x * y +/- 0.0f
  // FIXME: specify fp mode. ICL certainly is not strict in general.
  if (Constant *C = dyn_cast<Constant>(Srcs[2]))
    if (C->isZeroValue())
      return false;

  // Ignores upward or bit casts, which usually will be performed by copy
  // propagation within jitter.
  Value *Vals[] = {getEffectiveValueUp(Srcs[0]), getEffectiveValueUp(Srcs[1]),
                   getEffectiveValueUp(Srcs[2])};

  auto isIndirectRdRegion = [](Value *V) -> bool {
    if (!GenXIntrinsic::isRdRegion(V))
      return false;
    Region R(cast<Instruction>(V), BaleInfo());
    return R.Indirect;
  };

  auto isIndirectWrRegion = [](User *U) -> bool {
    if (!GenXIntrinsic::isWrRegion(U))
      return false;
    Region R(cast<Instruction>(U), BaleInfo());
    return R.Indirect;
  };

  // If the result of this mad used solely in an indirect
  // region write, count it as an indirect access.
  bool IsIndirectDst = false;
  if (AInst->hasOneUse()) {
    User *U = AInst->use_begin()->getUser();
    IsIndirectDst = isIndirectWrRegion(U);
  }

  if (isFpMad()) {
    // Agressive on floating point types since there are fewer constraints,
    // considering up to one indirect region access to be worthwhile.
    // For non-FP mads, any indirect region accesses make it not worth
    // bothering.
    unsigned IndirectCount = 0;
    if (isIndirectRdRegion(Vals[0]))
      IndirectCount++;
    if (isIndirectRdRegion(Vals[1]))
      IndirectCount++;
    if (isIndirectRdRegion(Vals[2]))
      IndirectCount++;
    if (IsIndirectDst)
      IndirectCount++;
    return IndirectCount <= 1;
  }

  if (IsIndirectDst || isIndirectRdRegion(Vals[2]) ||
      (isIndirectRdRegion(Vals[0]) && isIndirectRdRegion(Vals[1])))
    // For integer mad, we only support indirect access on one of
    // multiplicative operands.
    return false;

  // This is an integer mad.
  // Do not match constant add. I was getting bad results from allowing this,
  // although it may have been largely from scalar address computations.
  if (isa<Constant>(Srcs[2]))
    return false;

  // Do not match unless both of multiplicants are of type *B/*W.
  bool IsProfitable = true;

  auto Checker = [](Value *V) -> bool {
    // TODO, handle constants more accurately.
    if (isa<Constant>(V))
      return true;
    const unsigned DWordSizeInBits = 32;
    return (V->getType()->getScalarSizeInBits() < DWordSizeInBits);
  };

  auto HasKnownShAmtLT16 = [](Value *V) -> bool {
    ConstantInt *C = dyn_cast<ConstantInt>(V);
    if (!C) {
      if (!isa<Constant>(V))
        return false;
      C = dyn_cast<ConstantInt>(cast<Constant>(V)->getSplatValue());
      if (!C)
        return false;
    }
    return C->getValue().ult(16);
  };

  IsProfitable = Checker(Vals[0]);
  if (!IsProfitable)
    return false;

  IsProfitable = isLShift() ? HasKnownShAmtLT16(Vals[1]) : Checker(Vals[1]);
  if (!IsProfitable)
    return false;

  // Safety check on indirect access if any.
  GenXIntrinsicInst *RII = nullptr;
  if (isIndirectRdRegion(Vals[0]))
    RII = cast<GenXIntrinsicInst>(Vals[0]);
  else if (isIndirectRdRegion(Vals[1]))
    RII = cast<GenXIntrinsicInst>(Vals[1]);

  // Always profitable if there's no indirect access.
  if (!RII)
    return true;
  // Assume not profitable if the indirect access is defined in another BB to
  // avoid expensive alias analysis.
  if (RII->getParent() != AInst->getParent())
    return false;

  return IsProfitable;
}

static Value *getBroadcastFromScalar(Value *V) {
  VectorType *VTy = dyn_cast<VectorType>(V->getType());
  // Skip if it's not vector type.
  if (!VTy)
    return nullptr;
  // Skip if it's not from rdregion.
  if (!GenXIntrinsic::isRdRegion(V))
    return nullptr;
  GenXIntrinsicInst *RII = cast<GenXIntrinsicInst>(V);
  Region R(RII, BaleInfo());
  if (!R.isScalar() || R.Width != 1 || R.Offset != 0)
    return nullptr;
  Value *Src = RII->getArgOperand(0);
  auto *BC = dyn_cast<BitCastInst>(Src);
  if (!BC)
    return nullptr;
  VTy = dyn_cast<VectorType>(BC->getType());
  if (!VTy || VTy->getNumElements() != 1 ||
      VTy->getScalarType() != BC->getOperand(0)->getType())
    return nullptr;
  return BC->getOperand(0);
}

class FAddOperator
    : public ConcreteOperator<FPMathOperator, Instruction::FAdd> {};

class FSubOperator
    : public ConcreteOperator<FPMathOperator, Instruction::FSub> {};

class FMulOperator
    : public ConcreteOperator<FPMathOperator, Instruction::FMul> {};

class ExtOperator : public Operator {
public:
  static bool isExtOpcode(unsigned Opc) {
    return Opc == Instruction::SExt || Opc == Instruction::ZExt;
  }
  static inline bool classof(const Instruction *I) {
    return isExtOpcode(I->getOpcode());
  }
  static inline bool classof(const ConstantExpr *CE) {
    return isExtOpcode(CE->getOpcode());
  }
  static inline bool classof(const Value *V) {
    return (isa<Instruction>(V) && classof(cast<Instruction>(V))) ||
           (isa<ConstantExpr>(V) && classof(cast<ConstantExpr>(V)));
  }
};

class MulLikeOperator : public Operator {
public:
  static bool isMulLikeOpcode(unsigned Opc) {
    return Opc == Instruction::Mul || Opc == Instruction::Shl;
  }
  static inline bool classof(const Instruction *I) {
    return isMulLikeOpcode(I->getOpcode());
  }
  static inline bool classof(const Value *V) {
    return isa<Instruction>(V) && classof(cast<Instruction>(V));
  }
};

std::tuple<Value *, bool>
MadMatcher::getNarrowI16Vector(IRBuilder<> &Builder, Instruction *AInst,
                               Value *V, unsigned NumElts) const {
  assert(V->getType()->getScalarType()->isIntegerTy(32) && "I32 is expected!");
  if (auto Ext = dyn_cast<ExtOperator>(V)) {
    V = Ext->getOperand(0);
    if (V->getType()->getScalarType()->isIntegerTy(8)) {
      Type *DstTy = Builder.getInt16Ty();
      if (auto VTy = dyn_cast<VectorType>(V->getType()))
        DstTy = VectorType::get(DstTy, VTy->getNumElements());
      // Extend to i16 first.
      V = Builder.CreateCast(Instruction::CastOps(Ext->getOpcode()), V, DstTy);
    }
    if (!V->getType()->isVectorTy()) {
      // Broadcast through rdregion.
      Type *NewTy = VectorType::get(V->getType(), 1);
      V = Builder.CreateBitCast(V, NewTy);
      Region R(V);
      R.Offset = 0;
      R.Width = 1;
      R.Stride = R.VStride = 0;
      R.NumElements = NumElts;
      V = R.createRdRegion(V, ".splat", AInst, AInst->getDebugLoc());
    }
    return std::make_tuple(V, Ext->getOpcode() == Instruction::SExt);
  }
  if (auto CI = dyn_cast<ConstantInt>(V)) {
    const APInt &Val = CI->getValue();
    if (Val.isIntN(16)) {
      V = ConstantVector::getSplat(NumElts,
                                   Builder.getIntN(16, Val.getZExtValue()));
      return std::make_tuple(V, Val.isSignedIntN(16));
    }
  }
  return std::make_tuple(nullptr, false);
}

// The floating point case is relatively simple. Only need to match with fmul.
bool MadMatcher::matchFpMad() {
  assert(AInst->getOpcode() == Instruction::FAdd ||
         AInst->getOpcode() == Instruction::FSub);
  Value *Ops[2] = {AInst->getOperand(0), AInst->getOperand(1)};

  for (unsigned Idx = 0; Idx != 2; ++Idx) {
    Value *Op0 = Ops[Idx];
    Value *Op1 = Ops[1 - Idx];
    if (BinaryOperator *BO = dyn_cast<BinaryOperator>(Op0)) {
      // Case +/-(X * Y) +/- Z
      if (BO->getOpcode() == Instruction::FMul) {
        Srcs[0] = BO->getOperand(0);
        Srcs[1] = BO->getOperand(1);
        Srcs[2] = Op1;

        setMInst(BO);
        if (AInst->getOpcode() == Instruction::FSub)
          NegIndex = 2 - Idx;
        break;
      }
    }
    if (!MInst) {
      if (BinaryOperator *BO = dyn_cast<BinaryOperator>(Op1)) {
        // Case Z +/- X * Y
        if (BO->getOpcode() == Instruction::FMul) {
          Srcs[0] = BO->getOperand(0);
          Srcs[1] = BO->getOperand(1);
          Srcs[2] = Op0;

          setMInst(BO);
          if (AInst->getOpcode() == Instruction::FSub)
            NegIndex = 1;
          break;
        }
      }
    }
  }

  // No genx intrinsic mad for the fp case.
  ID = Intrinsic::fma;

  // Emit mad if matched and profitable.
  return emit();
}

bool MadMatcher::matchIntegerMad() {
  assert(AInst->getOpcode() == Instruction::Add ||
         AInst->getOpcode() == Instruction::Sub);
  Value *Ops[2] = {AInst->getOperand(0), AInst->getOperand(1)};

  if (auto BI = dyn_cast<MulLikeOperator>(Ops[0])) {
    // Case X * Y +/- Z
    Srcs[2] = Ops[1];
    Srcs[1] = BI->getOperand(1);
    Srcs[0] = BI->getOperand(0);
    setMInst(cast<Instruction>(BI));
    if (isProfitable()) {
      if (AInst->getOpcode() == Instruction::Sub)
        NegIndex = 2;
    } else
      setMInst(nullptr);
  }

  if (!MInst) {
    if (auto BI = dyn_cast<MulLikeOperator>(Ops[1])) {
      // Case Z +/- X * Y
      Srcs[2] = Ops[0];
      Srcs[1] = BI->getOperand(1);
      Srcs[0] = BI->getOperand(0);
      setMInst(cast<Instruction>(BI));
      if (isProfitable()) {
        if (AInst->getOpcode() == Instruction::Sub)
          NegIndex = 1;
      } else
        setMInst(nullptr);
    }
  }

  if (!MInst) { // Check if operand 0 is broadcasted from scalar.
    if (auto S = getBroadcastFromScalar(Ops[0])) {
      if (auto BI = dyn_cast<MulLikeOperator>(S)) {
        // Case X * Y +/- Z
        Srcs[2] = Ops[1];
        Srcs[1] = BI->getOperand(1);
        Srcs[0] = BI->getOperand(0);
        setMInst(cast<Instruction>(BI));
        if (isProfitable()) {
          if (AInst->getOpcode() == Instruction::Sub)
            NegIndex = 2;
        } else
          setMInst(nullptr);
      }
    }
  }

  if (!MInst) { // Check if operand 1 is broadcasted from scalar.
    if (auto S = getBroadcastFromScalar(Ops[1])) {
      if (auto BI = dyn_cast<MulLikeOperator>(S)) {
        // Case X * Y +/- Z
        Srcs[2] = Ops[0];
        Srcs[1] = BI->getOperand(1);
        Srcs[0] = BI->getOperand(0);
        setMInst(cast<Instruction>(BI));
        if (isProfitable()) {
          if (AInst->getOpcode() == Instruction::Sub)
            NegIndex = 1;
        } else
          setMInst(nullptr);
      }
    }
  }

  // Always use ssmad.
  ID = GenXIntrinsic::genx_ssmad;

  // Emit mad if matched and profitable.
  return emit();
}

bool MadMatcher::matchIntegerMad(unsigned IID) {
  assert((GenXIntrinsic::getAnyIntrinsicID(AInst) == IID) && "input out of sync");
  Value *Ops[2] = {AInst->getOperand(0), AInst->getOperand(1)};

  // TODO: handle cases like: cm_add(cm_mul(u, v), w).
  if (BinaryOperator *BI = dyn_cast<BinaryOperator>(Ops[0])) {
    if (BI->getOpcode() == Instruction::Mul ||
        BI->getOpcode() == Instruction::Shl) {
      // Case X * Y +/- Z
      Srcs[2] = Ops[1];
      Srcs[1] = BI->getOperand(1);
      Srcs[0] = BI->getOperand(0);
      setMInst(BI);
      if (!isProfitable())
        setMInst(nullptr);
    }
  }
  if (!MInst) {
    if (BinaryOperator *BI = dyn_cast<BinaryOperator>(Ops[1])) {
      // Case Z +/- X * Y
      if (BI->getOpcode() == Instruction::Mul ||
          BI->getOpcode() == Instruction::Shl) {
        Srcs[2] = Ops[0];
        Srcs[1] = BI->getOperand(1);
        Srcs[0] = BI->getOperand(0);
        setMInst(BI);
        if (!isProfitable())
          setMInst(nullptr);
      }
    }
  }

  switch (IID) {
  default:
    llvm_unreachable("unexpected intrinsic ID");
  case GenXIntrinsic::genx_ssadd_sat:
    ID = GenXIntrinsic::genx_ssmad_sat;
    break;
  case GenXIntrinsic::genx_suadd_sat:
    ID = GenXIntrinsic::genx_sumad_sat;
    break;
  case GenXIntrinsic::genx_usadd_sat:
    ID = GenXIntrinsic::genx_usmad_sat;
    break;
  case GenXIntrinsic::genx_uuadd_sat:
    ID = GenXIntrinsic::genx_uumad_sat;
    break;
  }

  // Emit mad if matched and profitable.
  return emit();
}

bool MadMatcher::emit() {
  if (MInst == nullptr || !isProfitable())
    return false;

  IRBuilder<> Builder(AInst);

  VectorType *VTy = dyn_cast<VectorType>(Srcs[2]->getType());
  if (!isFpMad() && VTy && VTy->getScalarType()->isIntegerTy(32)) {
    Value *V = getBroadcastFromScalar(Srcs[2]);
    if (!V)
      V = Srcs[2];
    auto BO = dyn_cast<BinaryOperator>(V);
    if (BO && BO->getOpcode() == Instruction::Mul) {
      // If both operands could be reduced to narrow integer types, use 'mul'
      // intrinsic.
      Value *V0 = nullptr, *V1 = nullptr;
      bool S0 = false, S1 = false;
      std::tie(V0, S0) = getNarrowI16Vector(Builder, AInst, BO->getOperand(0),
                                            VTy->getNumElements());
      std::tie(V1, S1) = getNarrowI16Vector(Builder, AInst, BO->getOperand(1),
                                            VTy->getNumElements());
      if (V0 && V1) {
        GenXIntrinsic::ID IID =
            S0 ? (S1 ? GenXIntrinsic::genx_ssmul : GenXIntrinsic::genx_sumul)
               : (S1 ? GenXIntrinsic::genx_usmul : GenXIntrinsic::genx_uumul);
        Module *M = AInst->getParent()->getParent()->getParent();
        Type *Tys[2] = {VTy, V0->getType()};
        Function *Fn = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        Value *Vals[2] = {V0, V1};
        CallInst *CI = Builder.CreateCall(Fn, Vals, "mul");
        Srcs[2] = CI;
      }
    }
  }

  Value *Vals[3] = {Srcs[0], Srcs[1], Srcs[2]};

  if (isa<BinaryOperator>(AInst)) {
    ExtOperator *E0 = dyn_cast<ExtOperator>(Vals[0]);
    ExtOperator *E1 = dyn_cast<ExtOperator>(Vals[1]);
    if (E0 && E1 &&
        E0->getOperand(0)->getType() == E1->getOperand(0)->getType()) {
      if (E0->getOpcode() == Instruction::SExt) {
        if (E1->getOpcode() == Instruction::SExt)
          ID = GenXIntrinsic::genx_ssmad;
        else
          ID = GenXIntrinsic::genx_sumad;
      } else {
        if (E1->getOpcode() == Instruction::SExt)
          ID = GenXIntrinsic::genx_usmad;
        else
          ID = GenXIntrinsic::genx_uumad;
      }
      Vals[0] = E0->getOperand(0);
      Vals[1] = E1->getOperand(0);
    }
  }

  if (auto VTy = dyn_cast<VectorType>(Vals[2]->getType())) {
    // Splat scalar sources if necessary.
    for (unsigned i = 0; i != 2; ++i) {
      Value *V = Vals[i];
      if (V->getType()->isVectorTy())
        continue;
      if (auto C = dyn_cast<Constant>(V)) {
        Vals[i] = ConstantVector::getSplat(VTy->getNumElements(), C);
        continue;
      }
      auto Ext = dyn_cast<ExtOperator>(V);
      if (Ext)
        V = Ext->getOperand(0);
      Type *NewTy = VectorType::get(V->getType(), 1);
      V = Builder.CreateBitCast(V, NewTy);
      // Broadcast through rdregin.
      Region R(V);
      R.Offset = 0;
      R.Width = 1;
      R.Stride = R.VStride = 0;
      R.NumElements = VTy->getNumElements();
      V = R.createRdRegion(V, ".splat", AInst, AInst->getDebugLoc());
      if (Ext)
        V = Builder.CreateCast(Instruction::CastOps(Ext->getOpcode()), V, VTy);
      Vals[i] = V;
    }
  }

  if (isLShift()) {
    Type *Ty = Vals[0]->getType();
    Constant *Base = ConstantInt::get(Ty->getScalarType(), 1);
    if (Ty->isVectorTy())
      Base = ConstantVector::getSplat(Ty->getVectorNumElements(), Base);
    Vals[1] = Builder.CreateShl(Base, Vals[1]);
  }

  // Perform source operand negation if necessary.
  if (NegIndex >= 0) {
    if (AInst->getType()->isFPOrFPVectorTy())
      Vals[NegIndex] = Builder.CreateFNeg(Vals[NegIndex], "fneg");
    else
      Vals[NegIndex] = Builder.CreateNeg(Vals[NegIndex], "neg");
  }

  Function *Fn = nullptr;
  {
    Module *M = AInst->getParent()->getParent()->getParent();
    if (AInst->getType()->isFPOrFPVectorTy())
      Fn = GenXIntrinsic::getAnyDeclaration(M, ID, AInst->getType());
    else {
      Type *Tys[2] = {AInst->getType(), Vals[0]->getType()};
      Fn = GenXIntrinsic::getAnyDeclaration(M, ID, Tys);
    }
  }
  CallInst *CI = Builder.CreateCall(Fn, Vals, "mad");
  CI->setDebugLoc(AInst->getDebugLoc());
  AInst->replaceAllUsesWith(CI);

  NumOfMadMatched++;
  return true;
}



bool MinMaxMatcher::valuesMatch(llvm::Value *Op1, llvm::Value *Op2) {
  // Handle casts for instructions.
  bool ZExt = false;
  if (CastInst *CI = dyn_cast<CastInst>(Op1)) {
    Op1 = CI->getOperand(0);
    if (CI->getOpcode() == Instruction::ZExt)
      ZExt = true;
  }
  if (CastInst *CI = dyn_cast<CastInst>(Op2)) {
    Op2 = CI->getOperand(0);
    if (CI->getOpcode() == Instruction::ZExt && !ZExt)
      return false;
  }

  // the easy case - the operands match
  if (Op1 == Op2)
    return true;

  // Handle constant zeros before data vectors.
  if (isa<ConstantAggregateZero>(Op1) && isa<ConstantAggregateZero>(Op2)) {
    ConstantAggregateZero *C1 = cast<ConstantAggregateZero>(Op1);
    ConstantAggregateZero *C2 = cast<ConstantAggregateZero>(Op2);
    if (C1->getNumElements() != C2->getNumElements())
      return false;
    Type *C1Ty = C1->getType();
    Type *C2Ty = C2->getType();
    if (C1Ty->isVectorTy()) {
      C1Ty = C1Ty->getSequentialElementType();
      C2Ty = C2Ty->getSequentialElementType();
    }

    return (C1Ty->isIntegerTy() && C2Ty->isIntegerTy()) ||
           (C1Ty->isFloatingPointTy() && C2Ty->isFloatingPointTy());
  }

  // ConstantDataVectors aren't always matched as different instances are
  // constructed containing the same values, so we'll compare the values to
  // catch this case.
  llvm::ConstantDataVector *C1 = dyn_cast<ConstantDataVector>(Op1);
  llvm::ConstantDataVector *C2 = dyn_cast<ConstantDataVector>(Op2);
  if (!C1 || !C2 || (C1->getNumElements() != C2->getNumElements()))
    return false;

  Type *C1Ty = C1->getElementType();
  Type *C2Ty = C2->getElementType();
  if (C1Ty->isIntegerTy() && C2Ty->isIntegerTy()) {
    for (unsigned i = 0, e = C1->getNumElements(); i < e; ++i)
      if (C1->getElementAsInteger(i) != C2->getElementAsInteger(i))
        return false;
    return true;
  }

  if (C1Ty->isFloatingPointTy() && C2Ty->isFloatingPointTy()) {
    for (unsigned i = 0, e = C1->getNumElements(); i < e; ++i) {
      double C1Val = C1Ty->isFloatTy() ? C1->getElementAsFloat(i)
                                       : C1->getElementAsDouble(i);
      double C2Val = C2Ty->isFloatTy() ? C2->getElementAsFloat(i)
                                       : C2->getElementAsDouble(i);
      if (C1Val != C2Val)
        return false;
    }
    return true;
  }

  return false;
}

bool MinMaxMatcher::matchMinMax() {
  assert(SelInst->getOpcode() == Instruction::Select && "expected SelectInst");
  if ((CmpInst = dyn_cast<llvm::CmpInst>(SelInst->getOperand(0)))) {
    Srcs[0] = SelInst->getOperand(1);
    Srcs[1] = SelInst->getOperand(2);
    CmpSrcs[0] = CmpInst->getOperand(0);
    CmpSrcs[1] = CmpInst->getOperand(1);

    bool Inverse = false;
    if (valuesMatch(CmpSrcs[1], Srcs[0]) && valuesMatch(CmpSrcs[0], Srcs[1]))
      Inverse = true;
    else if (!(valuesMatch(CmpSrcs[0], Srcs[0]) &&
               valuesMatch(CmpSrcs[1], Srcs[1])))
      return false;

    // We choose the min/max intrinsic based on the condition and whether the
    // operand ordering is the same in the cmp and select.
    switch (CmpInst->getPredicate()) {
    default:
      // this is not a candidate for min/max
      return false;
    case llvm::CmpInst::FCMP_OGE:
    case llvm::CmpInst::FCMP_OGT:
      if (Inverse) {
        ID = GenXIntrinsic::genx_fmin;
        Annotation = "min";
      } else {
        ID = GenXIntrinsic::genx_fmax;
        Annotation = "max";
      }
      break;
    case llvm::CmpInst::FCMP_OLE:
    case llvm::CmpInst::FCMP_OLT:
      if (Inverse) {
        ID = GenXIntrinsic::genx_fmax;
        Annotation = "max";
      } else {
        ID = GenXIntrinsic::genx_fmin;
        Annotation = "min";
      }
      break;
    case llvm::CmpInst::ICMP_SGE:
    case llvm::CmpInst::ICMP_SGT:
      if (Inverse) {
        ID = GenXIntrinsic::genx_smin;
        Annotation = "min";
      } else {
        ID = GenXIntrinsic::genx_smax;
        Annotation = "max";
      }
      break;
    case llvm::CmpInst::ICMP_SLE:
    case llvm::CmpInst::ICMP_SLT:
      if (Inverse) {
        ID = GenXIntrinsic::genx_smax;
        Annotation = "max";
      } else {
        ID = GenXIntrinsic::genx_smin;
        Annotation = "min";
      }
      break;
    case llvm::CmpInst::ICMP_UGE:
    case llvm::CmpInst::ICMP_UGT:
      if (Inverse) {
        ID = GenXIntrinsic::genx_umin;
        Annotation = "min";
      } else {
        ID = GenXIntrinsic::genx_umax;
        Annotation = "max";
      }
      break;
    case llvm::CmpInst::ICMP_ULE:
    case llvm::CmpInst::ICMP_ULT:
      if (Inverse) {
        ID = GenXIntrinsic::genx_umax;
        Annotation = "max";
      } else {
        ID = GenXIntrinsic::genx_umin;
        Annotation = "min";
      }
      break;
    }
  }

  // Emit min/max if matched
  return emit();
}

bool MinMaxMatcher::emit() {
  if ((ID == GenXIntrinsic::not_any_intrinsic) || (Srcs[0] == nullptr) ||
      (Srcs[1] == nullptr))
    return false;

  IRBuilder<> Builder(SelInst);
  Module *M = SelInst->getParent()->getParent()->getParent();
  Type *Tys[2] = {SelInst->getType(), Srcs[0]->getType()};
  Function *Fn = GenXIntrinsic::getAnyDeclaration(M, ID, Tys);
  CallInst *CI = Builder.CreateCall(Fn, Srcs, Annotation);
  CI->setDebugLoc(SelInst->getDebugLoc());
  SelInst->replaceAllUsesWith(CI);

  NumOfMinMaxMatched++;
  return true;
}

// For a given instruction, find the insertion position which is the closest
// to all the similar users to the specified reference user.
static std::tuple<BasicBlock *, Instruction *>
findOptimalInsertionPos(Instruction *I, Instruction *Ref, DominatorTree *DT,
                        std::function<bool(Instruction *)> IsSimilar) {
  assert(!isa<PHINode>(Ref) && "PHINode is not expected!");

  // Shortcut case. If it's single-used, insert just before that user.
  if (I->hasOneUse())
    return std::make_tuple(nullptr, Ref);

  DenseMap<BasicBlock *, Instruction *> BBs;
  for (auto U : I->users()) {
    Instruction *User = dyn_cast<Instruction>(U);
    if (!User || !IsSimilar(User))
      continue;
    BasicBlock *UseBB = User->getParent();
    DenseMap<BasicBlock *, Instruction *>::iterator MI;
    bool New = false;
    std::tie(MI, New) = BBs.insert(std::make_pair(UseBB, User));
    if (New)
      continue;
    // Find the earliest user if they are in the same block.
    BasicBlock::iterator BI = UseBB->begin();
    for (; &*BI != User && &*BI != MI->second; ++BI)
      /* EMPTY */;
    MI->second = &*BI;
  }

  assert(BBs.size() != 0 && "Must find at least one BB!");

  auto MI = BBs.begin();
  // Another shortcut case. If it's only used in a single BB,
  if (BBs.size() == 1)
    return std::make_tuple(MI->first, MI->second);

  BasicBlock *BB = MI->first;
  for (++MI; MI != BBs.end(); ++MI)
    BB = DT->findNearestCommonDominator(BB, MI->first);

  MI = BBs.find(BB);
  Instruction *Pos = nullptr;
  if (MI != BBs.end()) {
    BB = MI->first;
    Pos = MI->second;
  }
  assert(BB);
  return std::make_tuple(BB, Pos);
}

// For the specified constant, calculate its reciprocal if it's safe;
// otherwise, return null.
static Constant *getReciprocal(Constant *C, bool HasAllowReciprocal) {
  assert(C->getType()->isFPOrFPVectorTy() &&
         "Floating point value is expected!");

  // TODO: remove this and use ConstantExpr::getFDiv.

  // Reciprocal of undef can be undef.
  if (isa<UndefValue>(C))
    return C;

  if (ConstantFP *CFP = dyn_cast<ConstantFP>(C)) {
    // Compute the reciprocal of C.
    const APFloat &Divisor = CFP->getValueAPF();
    APFloat Rcp(Divisor.getSemantics(), 1U);
    APFloat::opStatus Status =
        Rcp.divide(Divisor, APFloat::rmNearestTiesToEven);
    // Only fold it if it's safe.
    if (Status == APFloat::opOK ||
        (HasAllowReciprocal && Status == APFloat::opInexact))
      return ConstantFP::get(C->getType()->getContext(), Rcp);
    return nullptr;
  }

  VectorType *VTy = cast<VectorType>(C->getType());
  IntegerType *ITy = Type::getInt32Ty(VTy->getContext());

  SmallVector<Constant *, 16> Result;
  for (unsigned i = 0, e = VTy->getNumElements(); i != e; ++i) {
    Constant *Elt =
        ConstantExpr::getExtractElement(C, ConstantInt::get(ITy, i));
    Constant *Rcp = getReciprocal(Elt, HasAllowReciprocal);
    // Skip if any of elements fails to be folded as reciprocal.
    if (!Rcp)
      return nullptr;
    Result.push_back(Rcp);
  }
  return ConstantVector::get(Result);
}

// For the given value, calculate its reciprocal and performance constant
// folding if allowed.
static Value *getReciprocal(IRBuilder<> &IRB, Value *V,
                            bool HasAllowReciprocal = true) {
  if (Constant *C = dyn_cast<Constant>(V))
    return getReciprocal(C, HasAllowReciprocal);

  if (!HasAllowReciprocal)
    return nullptr;

  Module *M = IRB.GetInsertBlock()->getParent()->getParent();
  Twine Name = V->getName() + ".inv";
  auto Func = GenXIntrinsic::getGenXDeclaration(M, GenXIntrinsic::genx_inv,
                                                V->getType());
  auto Inv = IRB.CreateCall(Func, V, Name);
  return Inv;
}

/// visitFDiv : reduce fdiv strength.
///
/// If fast-math is present, perform the following transforms:
///
/// (fdiv x, y)         -> (fmul x0, (fdiv 1., x1))
/// (fdiv 1., x)        -> (rcp x)
/// (fdiv 1., (sqrt x)) -> (rsqrt x)
///
/// Otherwise, try to reduce fdiv with constant divisor to fmul if the
/// reciprocal is exact.
///
void GenXPatternMatch::visitFDiv(BinaryOperator &I) {
  if (isInstructionTriviallyDead(&I)) {
    // Clean up dead 'fdiv', which may be left due to the limitation of
    // iterator used in instruction visitor, where only the instruction being
    // visited could be safely erased/removed.
    I.eraseFromParent();
    Changed |= true;
    return;
  }

  IRBuilder<> IRB(&I);

  Value *Op0 = I.getOperand(0);
  Value *Op1 = I.getOperand(1);
  // Constant folding Op1 if it's safe.
  if (Constant *C1 = dyn_cast<Constant>(Op1)) {
    Constant *Rcp = getReciprocal(C1, I.hasAllowReciprocal());
    if (!Rcp)
      return;
    IRB.setFastMathFlags(I.getFastMathFlags());
    Value *FMul = IRB.CreateFMul(Op0, Rcp);
    I.replaceAllUsesWith(FMul);
    I.eraseFromParent();
    Changed |= true;
    return;
  }

  // Skip if reciprocal optimization is not allowed.
  if (!I.hasAllowReciprocal())
    return;

  Instruction *Divisor = dyn_cast<Instruction>(Op1);
  if (!Divisor)
    return;

  auto IsSimilar = [](Instruction *User) {
    return User->getOpcode() == Instruction::FDiv && User->hasAllowReciprocal();
  };

  BasicBlock *BB = nullptr;
  Instruction *Pos = nullptr;
  std::tie(BB, Pos) = findOptimalInsertionPos(Divisor, &I, DT, IsSimilar);
  if (Pos)
    IRB.SetInsertPoint(Pos);
  else
    IRB.SetInsertPoint(BB);
  auto Rcp = getReciprocal(IRB, Divisor);
  cast<Instruction>(Rcp)->setDebugLoc(I.getDebugLoc());

  for (auto U : Divisor->users()) {
    Instruction *User = dyn_cast<Instruction>(U);
    if (!User || User == Rcp || !IsSimilar(User))
      continue;
    Op0 = User->getOperand(0);
    Value *NewVal = Rcp;
    if (!match(Op0, m_FPOne())) {
      IRB.SetInsertPoint(User);
      IRB.setFastMathFlags(User->getFastMathFlags());
      NewVal = IRB.CreateFMul(Op0, Rcp);
    }
    User->replaceAllUsesWith(NewVal);
    // Skip removing dead instruction if it's the current instruction being
    // visited as that might invalidate the iterator of this BB. These dead
    // 'fdiv' will be removed when they are visited then.
    if (User == &I)
      User->eraseFromParent();
  }
  Changed |= true;
  return;
}

namespace {

class MulLike {
public:
  virtual ~MulLike() {}
  static MulLike &get(Instruction *I);

  virtual Instruction *getMul(Instruction *) const { return nullptr; }
  virtual bool isAdd(User *) const { return false; }
};

class FPMulLike : public MulLike {
public:
  Instruction *getMul(Instruction *I) const override {
    if (isa<FMulOperator>(I))
      return I;
    return nullptr;
  }
  bool isAdd(User *U) const override {
    return isa<FAddOperator>(U) || isa<FSubOperator>(U);
  }
};

class IntMulLike : public MulLike {
public:
  Instruction *getMul(Instruction *I) const override {
    if (isa<MulOperator>(I) || isa<ShlOperator>(I))
      return I;
    return nullptr;
  }
  bool isAdd(User *U) const override {
    if (isa<AddOperator>(U) || isa<SubOperator>(U))
      return true;
    if (CallInst *CI = dyn_cast<CallInst>(U)) {
      switch (GenXIntrinsic::getGenXIntrinsicID(CI)) {
      // Keep this list consistent with the one used for matchIntegerMad(IID).
      case GenXIntrinsic::genx_ssadd_sat:
      case GenXIntrinsic::genx_suadd_sat:
      case GenXIntrinsic::genx_usadd_sat:
      case GenXIntrinsic::genx_uuadd_sat:
        return true;
      default:
        break;
      }
    }
    return false;
  }
};

MulLike &MulLike::get(Instruction *I) {
  Type *Ty = I->getType()->getScalarType();
  if (Ty->isFloatingPointTy()) {
    static FPMulLike FPMul;
    return FPMul;
  }
  if (Ty->isIntegerTy()) {
    static IntMulLike IntMul;
    return IntMul;
  }
  static MulLike Null;
  return Null;
}
} // End anonymous namespace

bool GenXPatternMatch::propagateFoldableRegion(Function *F) {
  ReversePostOrderTraversal<Function *> RPOT(F);
  bool Changed = false;
  for (auto *BB : RPOT)
    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
      MulLike &Ring = MulLike::get(&*BI);
      Instruction *Mul = Ring.getMul(&*BI);
      if (!Mul)
        continue;
      // Traverse each wrregion use of mul.
      for (auto *User : Mul->users()) {
        if (!GenXIntrinsic::isWrRegion(User))
          continue;
        GenXIntrinsicInst *WII = cast<GenXIntrinsicInst>(User);
        if (WII->getOperand(1) != Mul)
          continue;
        Region W(WII, BaleInfo());
        Region V(Mul);
        // TODO: Consider the broadcast and similar cases.
        if (!W.isStrictlySimilar(V))
          continue;
        // Check if all rdregion usage could be folded.
        SmallVector<GenXIntrinsicInst *, 16> Rds;
        SmallVector<GenXIntrinsicInst *, 1> Wrs; // Assume just 1 live wrregion.
        Wrs.push_back(WII);
        bool HasUnsafeUse = false;
        while (!HasUnsafeUse && !Wrs.empty()) {
          GenXIntrinsicInst *II = Wrs.back();
          Wrs.pop_back();
          for (auto *U : II->users()) {
            if (GenXIntrinsic::isRdRegion(U)) {
              GenXIntrinsicInst *RII = cast<GenXIntrinsicInst>(U);
              Region R(RII, BaleInfo());
              if (R == W) {
                for (auto *U2 : RII->users())
                  if (!Ring.isAdd(U2)) {
                    HasUnsafeUse = true;
                    break;
                  }
                if (HasUnsafeUse)
                  break;
                Rds.push_back(RII);
              } else if (R.overlap(W)) {
                HasUnsafeUse = true;
                break;
              }
            } else if (GenXIntrinsic::isWrRegion(U)) {
              GenXIntrinsicInst *WII2 = cast<GenXIntrinsicInst>(U);
              Region W2(WII2, BaleInfo());
              if (W2 == W) {
                // No more wrregion needs tracing. DO NOTHING.
              } else if (W2.overlap(W)) {
                HasUnsafeUse = true;
                break;
              } else // Otherwise, look over that non-overlapping wrregion.
                Wrs.push_back(WII2);
            } else {
              HasUnsafeUse = true;
              break;
            }
          }
        }
        // Skip if there is any unsafe use.
        if (HasUnsafeUse)
          continue;
        auto *ScalarOrVectorMul = scalarizeOrVectorizeIfNeeded(Mul, Rds.begin(), Rds.end());
        // Fold mul directly into its use after wrregion/rdregion pair.
        for (auto *II : Rds) {
          if (II->getType() != Mul->getType())
            II->replaceAllUsesWith(ScalarOrVectorMul);
          else
            II->replaceAllUsesWith(Mul);
          Changed = true;
        }
        // Collapse wrregion if there are rdregion folded away.
        if (!Rds.empty()) {
          WII->replaceAllUsesWith(WII->getArgOperand(0));
          Changed = true;
        }
      }
    }
  return Changed;
}

// Simplify:
//   %1 = zext i8 %0 to i32>
//   %2 = bitcast i32 %2 to <32 x i1>
//   %3 = call <8 x i1> @llvm.genx.rdpredregion.v8i1.v32i1(<32 x i1> %2, i32 0)
// into
//   %1 = bitcast i8 %0 to <8 x i1>
//   RAUW %1
//
bool GenXPatternMatch::simplifyPredRegion(CallInst *CI) {
  assert(GenXIntrinsic::getGenXIntrinsicID(CI) == GenXIntrinsic::genx_rdpredregion);
  bool Changed = false;

  unsigned NElts = CI->getType()->getVectorNumElements();
  ConstantInt *C = dyn_cast<ConstantInt>(CI->getArgOperand(1));
  assert(C && "constant integer expected");
  unsigned Offset = (unsigned)C->getZExtValue();
  assert(Offset % NElts == 0);

  // The number of actual bits required.
  unsigned NBits = NElts + Offset;
  NBits = 1U << llvm::Log2_32_Ceil(NBits);

  Value *Src = CI->getArgOperand(0);
  Value *Input = nullptr;
  if (match(Src, m_BitCast(m_ZExt(m_Value(Input))))) {
    unsigned InputBits = Input->getType()->getPrimitiveSizeInBits();
    if (NBits == InputBits) {
      IRBuilder<> Builder(CI);
      auto BC = Builder.CreateBitCast(Input, CI->getType(), "bitcast");
      if (auto Inst = dyn_cast<Instruction>(BC))
        Inst->setDebugLoc(CI->getDebugLoc());
      CI->replaceAllUsesWith(BC);
      Changed = true;
    }
  }
  return Changed;
}

bool GenXPatternMatch::simplifyRdRegion(CallInst* Inst) {
  assert(GenXIntrinsic::isRdRegion(Inst));
  auto NewVTy = Inst->getType();
  // rewrite indirect rdregion with constant offsets
  auto R = Region::getWithOffset(Inst, false /*ParentWidth*/);
  if (R.Indirect && R.IndirectIdx == 0 && R.IndirectAddrOffset == 0) {
    int64_t starti = 0;
    int64_t diffi = 0;
    if (IsLinearVectorConstantInts(R.Indirect, starti, diffi)) {
      R.Indirect = nullptr;
      R.Width = NewVTy->getVectorNumElements();
      R.Offset += starti;
      R.Stride = (diffi * 8) / NewVTy->getVectorElementType()->getPrimitiveSizeInBits();
      R.VStride = 0;
      Value* OldV = Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
      auto NewInst = R.createRdRegion(OldV, Inst->getName(),
      Inst /*InsertBefore*/, Inst->getDebugLoc());
      Inst->replaceAllUsesWith(NewInst);
      return true;
    }
  }
  return false;
}

bool GenXPatternMatch::simplifyWrRegion(CallInst *Inst) {
  assert(GenXIntrinsic::isWrRegion(Inst));
  Value *NewV = Inst->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum);
  Type *NewVTy = NewV->getType();

  // Rewrite a single element insertion to undef as a region splat.
  auto check1 = [=]() {
    Value *OldV = Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
    if (!isa<UndefValue>(OldV))
      return false;
    if (NewVTy->isVectorTy() && NewVTy->getVectorNumElements() > 1)
      return false;
    // Do not rewrite if input is another region read, as two region reads
    // cannot be groupped into a single bale.
    if (GenXIntrinsic::isRdRegion(NewV))
      return false;
    for (auto U : Inst->users()) {
      if (auto BC = dyn_cast<BitCastInst>(U)) {
        for (auto User : BC->users())
          if (GenXIntrinsic::isWrRegion(User))
            return false;
      }

      if (GenXIntrinsic::isWrRegion(U))
        return false;
    }

    // OK, rewrite it!
    return true;
  };

  if (check1()) {
    if (!NewVTy->isVectorTy()) {
      IRBuilder<> B(Inst);
      NewV = B.CreateBitCast(NewV, VectorType::get(NewVTy, 1));
    }
    Region R(Inst->getType());
    R.Width = R.NumElements;
    R.Stride = 0;
    NewV = R.createRdRegion(NewV, "splat", Inst, Inst->getDebugLoc(), false);
    Inst->replaceAllUsesWith(NewV);
    return true;
  }

  // rewrite indirect wrregion with constant offsets
  auto R = Region::getWithOffset(Inst, false/*ParentWidth*/);
  if (R.Indirect && R.IndirectIdx == 0 && R.IndirectAddrOffset == 0) {
    int64_t starti = 0;
    int64_t diffi = 0;
    if (IsLinearVectorConstantInts(R.Indirect, starti, diffi)) {
      R.Indirect = nullptr;
      R.Width = NewVTy->getVectorNumElements();
      R.Offset += starti;
      R.Stride = (diffi * 8) / NewVTy->getVectorElementType()->getPrimitiveSizeInBits();
      R.VStride = 0;
      Value* OldV = Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
      auto NewInst = R.createWrRegion(OldV, NewV, Inst->getName(),
          Inst /*InsertBefore*/, Inst->getDebugLoc());
      Inst->replaceAllUsesWith(NewInst);
      return true;
    }
  }
  return false;
}

// Simplify (trunc.sat (ext V)) to (trunc.sat V). Even if the source and
// destination has the same type, it's incorrect to fold them into V directly
// as the saturation is necessary.
bool GenXPatternMatch::simplifyTruncSat(CallInst *Inst) {
  assert(GenXIntrinsic::isIntegerSat(Inst) && "Unexpected integer saturation intrinsic!");

  GenXIntrinsicInst *II = cast<GenXIntrinsicInst>(Inst);
  ExtOperator *Ext = dyn_cast<ExtOperator>(Inst->getOperand(0));
  if (!Ext)
    return false;

  auto IID = GenXIntrinsic::getGenXIntrinsicID(II);
  Value *Src = Ext->getOperand(0);
  bool isZExt = (Ext->getOpcode() == Instruction::ZExt);

  switch (IID) {
  case GenXIntrinsic::genx_sstrunc_sat:
    IID = isZExt ? GenXIntrinsic::genx_sutrunc_sat
                 : GenXIntrinsic::genx_sstrunc_sat;
    break;
  case GenXIntrinsic::genx_sutrunc_sat:
    IID = isZExt ? GenXIntrinsic::genx_sutrunc_sat
                 : GenXIntrinsic::genx_sstrunc_sat;
    break;
  case GenXIntrinsic::genx_ustrunc_sat:
    IID = isZExt ? GenXIntrinsic::genx_uutrunc_sat
                 : GenXIntrinsic::genx_ustrunc_sat;
    break;
  case GenXIntrinsic::genx_uutrunc_sat:
    IID = isZExt ? GenXIntrinsic::genx_uutrunc_sat
                 : GenXIntrinsic::genx_ustrunc_sat;
    break;
  default:
    llvm_unreachable("Unknown intrinsic!");
  }

  Module *M = Inst->getParent()->getParent()->getParent();
  Type *Tys[2] = {Inst->getType(), Src->getType()};
  Function *Fn = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);

  Inst->setCalledFunction(Fn);
  Inst->setOperand(0, Src);

  return true;
}

// Merge select into a write region if possible.
//
// a = rrd(x, R);               a = rrd(x, R)
// c = a op b               ==> c = a op b
// d = select p, c, a
// wrr(x, d, R)                 wrr(x, c, R, p)
//
bool GenXPatternMatch::simplifySelect(Function *F) {
  using namespace GenXIntrinsic::GenXRegion;

  bool Changed = false;
  for (auto &BB : *F) {
    for (auto BI = BB.begin(), BE = BB.end(); BI != BE; /*empty*/) {
      SelectInst *Inst = dyn_cast<SelectInst>(&*BI++);
      if (!Inst || !Inst->hasOneUse() || !Inst->getType()->isVectorTy() ||
          !Inst->getCondition()->getType()->isVectorTy())
        continue;
      if (!GenXIntrinsic::isWrRegion(Inst->user_back()))
        continue;
      CallInst *Wr = cast<CallInst>(Inst->user_back());
      if (Wr->getOperand(NewValueOperandNum) != Inst)
        continue;

      auto match = [](Instruction *Wr, Value *V) -> bool {
        if (!GenXIntrinsic::isRdRegion(V))
          return false;
        CallInst *Rd = cast<CallInst>(V);
        if (Wr->getOperand(OldValueOperandNum) !=
            Rd->getOperand(OldValueOperandNum))
          return false;

        Region WrReg(Wr, BaleInfo());
        Region RdReg(Rd, BaleInfo());
        if (WrReg != RdReg || WrReg.Indirect)
          return false;

        if (WrReg.Mask == nullptr)
          return true;
        if (auto C = dyn_cast<Constant>(WrReg.Mask))
          if (C->isAllOnesValue())
            return true;

        return false;
      };

      for (int i = 1; i <= 2; ++i) {
        Value *Op = Inst->getOperand(i);
        if (match(Wr, Op)) {
          Value *Mask = Inst->getCondition();
          if (i == 1) {
            IRBuilder<> B(Inst);
            Mask = B.CreateNot(Mask, "not");
          }

          Region WrReg(Wr, BaleInfo());
          WrReg.Mask = Mask;
          Value *NewWr = WrReg.createWrRegion(
              Wr->getOperand(OldValueOperandNum), Inst->getOperand(3 - i),
              Wr->getName(), Wr, Wr->getDebugLoc());
          Wr->replaceAllUsesWith(NewWr);
          Changed = true;

          if (Wr == &*BI)
            ++BI;
          Wr->eraseFromParent();
          Inst->eraseFromParent();
          break;
        }
      }
    }
  }

  return Changed;
}

// Perform volatile global related simplifications.
bool GenXPatternMatch::simplifyVolatileGlobals(Function *F) {
  bool Changed = false;
  for (auto &BB : F->getBasicBlockList()) {
    for (auto I = BB.begin(); I != BB.end(); /*empty*/) {
      Instruction *Inst = &*I++;
      if (isa<LoadInst>(Inst))
        Changed |= normalizeGloads(Inst);
    }
    for (auto I = BB.rbegin(); I != BB.rend(); /*empty*/) {
      Instruction *Inst = &*I++;
      if (isInstructionTriviallyDead(Inst))
        Inst->eraseFromParent();
    }
  }
  return Changed;
}

// Decompose predicate operand for large vector selects.
bool GenXPatternMatch::decomposeSelect(Function *F) {
  auto P = getAnalysisIfAvailable<GenXSubtargetPass>();
  const GenXSubtarget *ST = P ? P->getSubtarget() : nullptr;
  SelectDecomposer SD(ST);
  for (auto &BB : F->getBasicBlockList())
    for (auto &Inst : BB.getInstList())
      if (isa<SelectInst>(Inst))
        SD.addStartSelect(&Inst);

  return SD.run();
}

bool GenXPatternMatch::reassociateIntegerMad(Function *F) {
  auto isSingleUsedAdd = [](Value *V) -> bool {
    auto BO = dyn_cast<BinaryOperator>(V);
    if (!BO || !BO->hasOneUse())
      return false;
    // FIXME: Consider 'sub' as well.
    return BO->getOpcode() == Instruction::Add;
  };

  auto isSingleUsedMul = [](Value *V) -> bool {
    auto BO = dyn_cast<BinaryOperator>(V);
    if (!BO || !BO->hasOneUse())
      return false;
    return (BO->getOpcode() == Instruction::Mul ||
            BO->getOpcode() == Instruction::Shl);
  };

  bool Changed = false;
  for (auto &BB : *F) {
    for (auto BI = BB.begin(), BE = BB.end(); BI != BE; /*EMPTY*/) {
      if (!isSingleUsedAdd(&*BI)) {
        ++BI;
        continue;
      }

      auto BO = cast<BinaryOperator>(&*BI);
      if (!isSingleUsedMul(BO->getOperand(0)) ||
          !isSingleUsedMul(BO->getOperand(1))) {
        ++BI;
        continue;
      }

      // Found (a0 * b0) + (a1 * b1), track through the chain to check it is
      //
      //  (a0 * b0) + (a1 * b1) + ... + c
      //
      // and transform it into
      //
      //  c + (a0 * b0) + (a1 * b1) + ...
      //
      SmallVector<BinaryOperator *, 16> AccChain;
      AccChain.push_back(BO);
      bool Found = false;
      unsigned OpndNo = 0;
      while (!Found) {
        Use &U = *BO->use_begin();
        if (!isSingleUsedAdd(U.getUser()))
          break;
        BO = cast<BinaryOperator>(U.getUser());
        if (BO->getParent() != &BB)
          break;
        if (!isSingleUsedMul(BO->getOperand(1 - U.getOperandNo()))) {
          OpndNo = 1 - U.getOperandNo();
          Found = true;
        }
        AccChain.push_back(BO);
      }
      if (!Found) {
        ++BI;
        continue;
      }

      BO = AccChain.back();
      AccChain.pop_back();

      IRBuilder<> IRB(BO);
      // Reconstruct a new accumulation chain.
      Instruction *Acc = cast<Instruction>(IRB.CreateAdd(
          BO->getOperand(OpndNo), AccChain.front()->getOperand(0)));
      OpndNo = 1;
      for (auto CI = AccChain.begin(), CE = AccChain.end(); CI != CE; ++CI) {
        auto BO2 = *CI;
        Value *Opnd = BO2->getOperand(OpndNo);
        Acc = cast<Instruction>(IRB.CreateAdd(Acc, Opnd));
        Acc->setDebugLoc(BO2->getDebugLoc());
        Use &U = *BO2->use_begin();
        OpndNo = 1 - U.getOperandNo();
      }
      BO->replaceAllUsesWith(Acc);

      // Erase old accumulation chain.
      BI = std::next(BasicBlock::iterator(BO));
      BO->eraseFromParent();
      while (!AccChain.empty()) {
        BO = AccChain.back();
        AccChain.pop_back();
        BI = std::next(BasicBlock::iterator(BO));
        BO->eraseFromParent();
      }
      Changed = true;
    }
  }

  return Changed;
}

bool GenXPatternMatch::distributeIntegerMul(Function *F) {
  bool Changed = false;
  for (auto &BB : *F) {
    for (auto BI = BB.begin(), BE = BB.end(); BI != BE; /*EMPTY*/) {
      auto Mul = dyn_cast<MulOperator>(&*BI++);
      if (!Mul || Mul->getType()->getScalarSizeInBits() < 32)
        continue;
      // Find the following pattern
      //
      //  A * (B + C) and all components are extended from 8-/16-bit integers.
      //
      // and transform it to
      //
      //  A * B + A * C.
      //
      // This transformation won't bring two much difference on SKL but could
      // improve code quality a lot on platforms without multiplication of
      // D * D -> D, e.g. CNL.
      Value *LHS = Mul->getOperand(0);
      Value *RHS = Mul->getOperand(1);
      if (!isa<ExtOperator>(LHS))
        std::swap(LHS, RHS);
      // Skip if both LHS & RHS are not ext operators.
      if (!isa<ExtOperator>(LHS))
        continue;
      // Skip if both LHS & RHS are already operands extended from narrow
      // types.
      if (isa<ExtOperator>(RHS))
        continue;

      auto collect = [](Value *V, SmallVectorImpl<Value *> &Ops) -> bool {
        SmallVector<Value *, 32> CheckList;
        CheckList.push_back(V);

        while (!CheckList.empty()) {
          V = CheckList.pop_back_val();
          // Collect values if they are extended from narrow types.
          if (isa<ExtOperator>(V)) {
            Ops.push_back(V);
            continue;
          }
          // FIXME: Add 'sub' support.
          AddOperator *Add = dyn_cast<AddOperator>(V);
          if (!Add || !Add->hasOneUse())
            return true;
          // DFT that 'add' tree.
          CheckList.push_back(Add->getOperand(1));
          CheckList.push_back(Add->getOperand(0));
        }

        return false;
      };

      SmallVector<Value *, 16> Ops;
      if (collect(RHS, Ops))
        continue;

      assert(!Ops.empty() && "There's no operands collected!");

      IRBuilder<> Builder(cast<Instruction>(Mul));
      Value *Sum = nullptr;
      for (auto V : Ops) {
        Value *Prod = Builder.CreateMul(LHS, V);
        if (!Sum)
          Sum = Prod;
        else
          Sum = Builder.CreateAdd(Sum, Prod);
      }
      Mul->replaceAllUsesWith(Sum);
      RecursivelyDeleteTriviallyDeadInstructions(Mul);

      Changed = true;
    }
  }
  return Changed;
}

// The shift pattern:
// V[0:7]   = ShtAmt[0]
// V[8:15]  = ShtAmt[0] + ShtAmt[1]
// V[16:23] = ShtAmt[0] + ShtAmt[2]
// V[24:31] = ShtAmt[0] + ShtAmt[3]
// where ShtAmt[0] is a constant vector and ShtAmt[i] are constant splats.
static bool analyzeForShiftPattern(Constant *C,
                                   SmallVectorImpl<Constant *> &ShtAmt,
                                   const DataLayout &DL) {
  unsigned Width = 8;
  VectorType *VT = dyn_cast<VectorType>(C->getType());
  if (!VT || VT->getVectorNumElements() <= Width ||
      VT->getScalarSizeInBits() == 1)
    return false;
  unsigned NElts = VT->getVectorNumElements();
  if (NElts % Width != 0)
    return false;

  SmallVector<Constant *, 8> Elts(Width, nullptr);
  for (unsigned i = 0; i < Width; ++i) {
    Constant *Elt = C->getAggregateElement(i);
    if (isa<UndefValue>(Elt))
      return false;
    Elts[i] = Elt;
  }
  Constant *Base = ConstantVector::get(Elts);
  ShtAmt.push_back(Base);

  for (unsigned i = Width; i < NElts; i += Width) {
    SmallVector<Constant *, 8> Elts(Width, nullptr);
    for (unsigned j = 0; j < Width; ++j) {
      Constant *Elt = C->getAggregateElement(i + j);
      if (isa<UndefValue>(Elt))
        return false;
      Elts[j] = Elt;
    }
    unsigned Op = Base->getType()->isFPOrFPVectorTy() ? Instruction::FSub
                                                      : Instruction::Sub;
    Constant *A[] = {ConstantVector::get(Elts), Base};
    auto X = ConstantFoldBinaryOpOperands(Op, A[0], A[1], DL);
    if (!X)
      return false;
    if (!X->getSplatValue()) {
      // This is not a splat and it is an integer vector.
      if (!Base->getType()->isFPOrFPVectorTy())
        return false;

      // Check if A and B are within a few ULPs.
      auto isWithinMaxULP = [](APFloat A, APFloat B, unsigned NSteps) {
        APFloat::cmpResult cmpRes = A.compare(B);
        if (cmpRes == APFloat::cmpEqual)
          return true;
        if (cmpRes == APFloat::cmpUnordered)
          return false;

        unsigned MAX_ULP = 3 * NSteps;
        bool nextDown = cmpRes == APFloat::cmpGreaterThan;
        for (unsigned i = 0; i < MAX_ULP; ++i) {
          A.next(nextDown);
          if (A.compare(B) == APFloat::cmpEqual)
            return true;
        }
        return false;
      };

      // This is not an exact splat fp vector. We check if they are within a few
      // ULPs, as divisions are actually not correctly rounded during folding.
      ConstantFP *X0 = dyn_cast_or_null<ConstantFP>(X->getAggregateElement(0U));
      if (!X0)
        return false;
      for (unsigned j = 1; j < Width; ++j) {
        ConstantFP *Xj =
            dyn_cast_or_null<ConstantFP>(X->getAggregateElement(j));
        unsigned NSteps = NElts / Width;
        if (!Xj ||
            !isWithinMaxULP(Xj->getValueAPF(), X0->getValueAPF(), NSteps))
          return false;
      }
      X = ConstantDataVector::getSplat(Width, X0);
    }
    ShtAmt.push_back(X);
  }
  return true;
}

bool GenXPatternMatch::vectorizeConstants(Function *F) {
  bool Changed = false;
  for (auto &BB : F->getBasicBlockList()) {
    for (auto I = BB.begin(); I != BB.end();) {
      Instruction *Inst = &*I++;
      if (isa<PHINode>(Inst))
        continue;
      unsigned NumOpnds = Inst->getNumOperands();
      auto CI = dyn_cast<CallInst>(Inst);
      if (CI)
        NumOpnds = CI->getNumArgOperands();
      for (unsigned i = 0, e = NumOpnds; i != e; ++i) {
        auto C = dyn_cast<Constant>(Inst->getOperand(i));
        if (!C || isa<UndefValue>(C))
          continue;
        if (opMustBeConstant(Inst, i))
          continue;
        auto Ty = C->getType();
        if (!Ty->isVectorTy() || Ty->getVectorNumElements() < 16 ||
            C->getSplatValue())
          continue;
        SmallVector<Constant *, 8> ShtAmt;
        if (analyzeForShiftPattern(C, ShtAmt, *DL)) {
          // W1 = wrrregion(undef, ShtAmt[0], 0);
          // V2 = fadd ShtAmt[0], ShtAmt[1]
          // W2 = wrregion(W1, V2, Width)
          // V3 = fadd ShtAmt[0], ShtAmt[2]
          // W2 = wrregion(W2, V3, Width * 2)
          // ...
          Value *Base = nullptr;
          {
            Value *Args[] = {ShtAmt[0]};
            Type *Tys[] = {ShtAmt[0]->getType()};
            auto ID = C->getType()->isFPOrFPVectorTy()
                          ? GenXIntrinsic::genx_constantf
                          : GenXIntrinsic::genx_constanti;
            Module *M = F->getParent();
            Function *Decl = GenXIntrinsic::getGenXDeclaration(M, ID, Tys);
            auto NewInst = CallInst::Create(Decl, Args, "constant", Inst);
            NewInst->setDebugLoc(Inst->getDebugLoc());
            Base = NewInst;
          }

          IRBuilder<> Builder(Inst);
          unsigned Width = ShtAmt[0]->getType()->getVectorNumElements();
          Region R(C->getType());
          R.getSubregion(0, Width);
          Value *Val = UndefValue::get(C->getType());
          Val = R.createWrRegion(Val, Base, "", Inst, Inst->getDebugLoc());
          for (unsigned j = 1; j < (unsigned)ShtAmt.size(); ++j) {
            auto Opc = C->getType()->isFPOrFPVectorTy() ? Instruction::FAdd
                                                        : Instruction::Add;
            auto Input = Builder.CreateBinOp(Opc, Base, ShtAmt[j]);
            Region R1(C->getType());
            R1.getSubregion(Width * j, Width);
            Val = R1.createWrRegion(Val, Input, "", Inst, Inst->getDebugLoc());
          }

          // Update this operand with newly vectorized constant.
          auto ID = GenXIntrinsic::getGenXIntrinsicID(Inst);
          if (ID == GenXIntrinsic::genx_constantf ||
              ID == GenXIntrinsic::genx_constanti) {
            Inst->replaceAllUsesWith(Val);
            Inst->eraseFromParent();
          } else
            Inst->setOperand(i, Val);

          Changed = true;
        }
      }
    }
  }

  return Changed;
}

static Instruction *insertConstantLoad(Constant *C, Instruction *InsertBefore) {
  assert(!C->getType()->getScalarType()->isIntegerTy(1));
  Value *Args[] = {C};
  Type *Ty[] = {C->getType()};
  auto IntrinsicID = GenXIntrinsic::genx_constanti;
  if (C->getType()->isFPOrFPVectorTy())
    IntrinsicID = GenXIntrinsic::genx_constantf;
  Module *M = InsertBefore->getParent()->getParent()->getParent();
  Function *F = GenXIntrinsic::getGenXDeclaration(M, IntrinsicID, Ty);
  Instruction *Inst = CallInst::Create(F, Args, "constant", InsertBefore);
  Inst->setDebugLoc(InsertBefore->getDebugLoc());
  return Inst;
}

bool GenXPatternMatch::placeConstants(Function *F) {
  bool Changed = false;
  for (auto &BB : F->getBasicBlockList()) {
    for (auto I = BB.begin(); I != BB.end();) {
      Instruction *Inst = &*I++;
      auto ID = GenXIntrinsic::getGenXIntrinsicID(Inst);
      if (ID == GenXIntrinsic::genx_constantf ||
          ID == GenXIntrinsic::genx_constanti)
        continue;

      for (unsigned i = 0, e = Inst->getNumOperands(); i != e; ++i) {
        auto C = dyn_cast<Constant>(Inst->getOperand(i));
        if (!C || isa<UndefValue>(C))
          continue;
        if (opMustBeConstant(Inst, i))
          continue;
        auto Ty = C->getType();
        if (!Ty->isVectorTy() || C->getSplatValue())
          continue;
        if (Ty->getScalarSizeInBits() == 1)
          continue;

        // Counting the bit size of non-undef values.
        unsigned NBits = 0;
        for (unsigned i = 0, e = Ty->getVectorNumElements(); i != e; ++i) {
          Constant *Elt = C->getAggregateElement(i);
          if (Elt && !isa<UndefValue>(Elt))
            NBits += Ty->getScalarSizeInBits();
        }
        if (NBits <= 256)
          continue;

        // Collect uses inside this function.
        SmallVector<Use *, 8> ConstantUses;
        std::set<Instruction *> ConstantUsers;

        for (auto &U : C->uses()) {
          auto I = dyn_cast<Instruction>(U.getUser());
          if (!I || I->getParent()->getParent() != F)
            continue;
          ConstantUses.push_back(&U);
          ConstantUsers.insert(I);
        }
        if (ConstantUsers.empty())
          continue;

        // Single use in a loop.
        if (ConstantUsers.size() == 1) {
          // Do not lift this constant, for now, to avoid spills.
#if 0
          Use *U = ConstantUses.back();
          Instruction *UseInst = cast<Instruction>(U->getUser());
          BasicBlock *UseBB = UseInst->getParent();
          if (Loop *L = LI->getLoopFor(UseBB)) {
            if (BasicBlock *Preheader = L->getLoopPreheader()) {
              if (Preheader != UseBB) {
                // Insert constant initialization in loop preheader.
                Instruction *InsertBefore = Preheader->getTerminator();
                Value *Val = insertConstantLoad(C, InsertBefore);
                U->set(Val);
                Changed = true;
              }
            }
          }
#endif
          continue; // skip to the next constant
        }

        // It is profitable to use a common constant pool in register.
        assert(ConstantUses.size() >= 2);
        BasicBlock *InsertBB = nullptr;
        for (auto U : ConstantUses) {
          auto UseInst = cast<Instruction>(U->getUser());
          auto UseBB = UseInst->getParent();
          if (InsertBB == nullptr)
            InsertBB = UseBB;
          else if (InsertBB != UseBB) {
            InsertBB = DT->findNearestCommonDominator(InsertBB, UseBB);
          }
        }

        // InsertBlock is in a loop.
        if (Loop *L = LI->getLoopFor(InsertBB))
          if (BasicBlock *Preheader = L->getLoopPreheader())
            if (Preheader != InsertBB)
              InsertBB = Preheader;

        // If the insert block is the same as some use block, find the first
        // use instruction as the insert point. Otherwise, use the terminator of
        // the insert block.
        Instruction *InsertBefore = InsertBB->getTerminator();
        for (auto UseInst : ConstantUsers) {
          if (InsertBB == UseInst->getParent()) {
            for (auto &I : InsertBB->getInstList()) {
              if (ConstantUsers.find(&I) != ConstantUsers.end()) {
                InsertBefore = &I;
                goto Found;
              }
            }
          }
        }
      Found:
        assert(!isa<PHINode>(InsertBefore));
        Value *Val = insertConstantLoad(C, InsertBefore);
        for (auto U : ConstantUses)
          U->set(Val);
        Changed = true;
      }
    }
  }

  return Changed;
}

bool GenXPatternMatch::simplifyNullDst(CallInst *Inst) {
  if (Inst->getNumUses() != 1)
    return false;

  PHINode *Phi = dyn_cast<PHINode>(Inst->use_begin()->getUser());
  if (Phi == nullptr)
    return false;

  if (Phi->getNumUses() == 1 && Phi->use_begin()->getUser() == Inst) {
    Phi->replaceAllUsesWith(UndefValue::get(Phi->getType()));
    Phi->eraseFromParent();
    return true;
  }

  return false;
}

bool canExtendMask(BinaryOperator *BO) {
  Type *InstTy = BO->getType();
  auto Op0 = dyn_cast<ConstantDataVector>(BO->getOperand(0));
  auto Op1 = dyn_cast<ConstantDataVector>(BO->getOperand(1));
  return InstTy->isVectorTy() &&
         (InstTy->getScalarSizeInBits() == genx::ByteBits) && (Op0 || Op1);
}

bool GenXPatternMatch::extendMask(BinaryOperator *BO) {
  if (!canExtendMask(BO))
    return false;

  Type *InstTy = BO->getType();
  Type *I32Ty = Type::getInt32Ty(InstTy->getContext());
  unsigned SizeInBits = InstTy->getScalarSizeInBits();
  unsigned Scale = I32Ty->getPrimitiveSizeInBits() / SizeInBits;
  unsigned NumElts = InstTy->getVectorNumElements();

  // Cannot bitcast <N x iM> to <N/(32/M) x i32>
  if (NumElts % Scale != 0)
    return false;
  NumElts /= Scale;

  Type *NewTy = VectorType::get(I32Ty, NumElts);
  IRBuilder<TargetFolder> Builder(BO->getParent(), BasicBlock::iterator(BO),
                                  TargetFolder(*DL));
  StringRef Name = BO->getName();

  Value *Op0 =
      Builder.CreateBitCast(BO->getOperand(0), NewTy, Name + ".extend.mask.op");
  Value *Op1 =
      Builder.CreateBitCast(BO->getOperand(1), NewTy, Name + ".extend.mask.op");
  Value *NewAnd = Builder.CreateAnd(Op0, Op1, Name + ".extend.mask");
  NewAnd = Builder.CreateBitCast(NewAnd, InstTy, Name + ".extend.mask.trunc");

  BO->replaceAllUsesWith(NewAnd);

  return true;
}
