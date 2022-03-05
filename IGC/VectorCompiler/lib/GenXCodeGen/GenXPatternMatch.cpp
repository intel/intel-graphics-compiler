/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"
#include "GenXVectorDecomposer.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/TargetFolder.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/GenXIntrinsics/GenXIntrinsicInst.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
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
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"

#include "llvmWrapper/IR/Constants.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/TypeSize.h"
#include "llvmWrapper/Transforms/Utils/Local.h"

#include "vc/Utils/General/InstRebuilder.h"

#include <functional>
#include <limits>
#include "Probe/Assertion.h"
#include "IGC/common/StringMacros.hpp"
#include "IGC/common/debug/DebugMacros.hpp"

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
STATISTIC(NumOfAdd3Matched, "Number of add3 instructions matched");
static cl::opt<bool> EnableAdd3Matcher(IGC_STRDEBUG("enable-add3"), cl::init(true),
                                       cl::Hidden,
                                       cl::desc("Enable add3 matching."));
STATISTIC(NumOfBfnMatched, "Number of BFN instructions matched");
static cl::opt<bool> EnableBfnMatcher("enable-bfn", cl::init(true), cl::Hidden,
                                      cl::desc("Enable bfn matching."));
static cl::opt<bool> EnableLscAddrFoldOffset("enable-lsc-addr-fold-offset",
                                             cl::init(true), cl::Hidden,
                                             cl::desc("Enable LSC offset folding"));
// Currently not supported in Finalizer
static cl::opt<bool> EnableLscAddrFoldScale("enable-lsc-addr-fold-scale",
                                            cl::init(false), cl::Hidden,
                                            cl::desc("Enable LSC scale folding"));

namespace {

class GenXPatternMatch : public FunctionPass,
                         public InstVisitor<GenXPatternMatch> {
  DominatorTree *DT = nullptr;
  LoopInfo *LI = nullptr;
  const DataLayout *DL = nullptr;
  const TargetOptions *Options = nullptr;
  // Indicates whether there is any change.
  bool Changed = false;

public:
  static char ID;
  GenXPatternMatch() : FunctionPass(ID) {}

  StringRef getPassName() const override { return "GenX pattern match"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<TargetPassConfig>();
    AU.addPreserved<GenXModule>();
    AU.setPreservesCFG();
  }

  void visitBinaryOperator(BinaryOperator &I);

  void visitCallInst(CallInst &I);

  void visitSelectInst(SelectInst &I);

  void visitFDiv(BinaryOperator &I);

  void visitICmpInst(ICmpInst &I);

  void visitSRem(BinaryOperator &I);

  void visitSDiv(BinaryOperator &I);

  void visitURem(BinaryOperator &I);

  void visitUDiv(BinaryOperator &I);

#if LLVM_VERSION_MAJOR >= 10
  void visitFreezeInst(FreezeInst &I);
#endif

  bool runOnFunction(Function &F) override;

  bool isFpMadEnabled() const {
    return EnableMadMatcher &&
           (!Options || Options->AllowFPOpFusion != FPOpFusion::Strict);
  }

private:
  // flipBoolNot : flip a (vector) bool not instruction if beneficial
  bool flipBoolNot(Instruction *Inst);
  // foldBoolAnd : fold a (vector) bool and into sel/wrregion if beneficial
  bool matchInverseSqrt(Instruction *I);
  bool foldLscAddrCalculation(CallInst *Inst);
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

FunctionPass *llvm::createGenXPatternMatchPass() {
  initializeGenXPatternMatchPass(*PassRegistry::getPassRegistry());
  return new GenXPatternMatch();
}

bool GenXPatternMatch::runOnFunction(Function &F) {
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  DL = &F.getParent()->getDataLayout();
  Options = &getAnalysis<TargetPassConfig>().getTM<GenXTargetMachine>().Options;

  // Before we get the simd-control-flow representation right,
  // we avoid dealing with predicate constants
  const GenXSubtarget *ST = &getAnalysis<TargetPassConfig>()
                                 .getTM<GenXTargetMachine>()
                                 .getGenXSubtarget();
  loadPhiConstants(F, DT, *ST, *DL, true);
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

  // Simplify instructions after select decomposition and clear dead ones.
  for (auto &BB : F)
    Changed |= SimplifyInstructionsInBlock(&BB);

  return Changed;
}

namespace {

// Helper class to share common code.
class MadMatcher {
public:
  explicit MadMatcher(Instruction *I)
      : AInst(I), MInst(nullptr), ID(GenXIntrinsic::not_any_intrinsic), NegIndex(-1) {
    IGC_ASSERT_MESSAGE(I, "null instruction");
    Srcs[0] = Srcs[1] = Srcs[2] = nullptr;
  }

  // Match mads with floating point operands.
  bool matchFpMad();

  // Match integer mads that starts with binary operators.
  bool matchIntegerMad();

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

  // mad on i64 is restricted.
  // Checks if add/sub/mul/shl/.. operates on i64 or <n x i64>.
  bool isOperationOnI64() const {
    auto isI64 = [this](Value *Val) -> bool {
      return Val->getType()->getScalarType()->isIntegerTy(64);
    };

    return isI64(MInst) || isI64(AInst);
  }

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

class Add3Matcher {
public:
  explicit Add3Matcher(Instruction *I)
      : AInst(I), A2Inst(nullptr), ID(GenXIntrinsic::not_any_intrinsic) {
    IGC_ASSERT_MESSAGE(I, "null instruction");
    Srcs[0] = Srcs[1] = Srcs[2] = nullptr;
    Negs[0] = Negs[1] = Negs[2] = false;
  }

  // Match integer mads that starts with binary operators.
  bool matchIntegerAdd3();

  bool isProfitable(Instruction *A2) const;

private:
  // Return true if changes are made.
  bool emit();

  void setA2Inst(Instruction *I) { A2Inst = I; }

private:
  // The instruction starts the add3 matching:
  // * add/sub
  // * genx_*add
  Instruction *AInst;

  // The instruction being sinked into:
  // * add/sub
  Instruction *A2Inst;

  // The add3 intrinsic ID.
  unsigned ID;

  // Source operands for the add3 intrinsic call, representing add3 as
  // srcs[0] + srcs[1] + srcs[2].
  std::array<Value *, 3> Srcs;

  // Indicates whether Srcs[i] needs to be negated.
  std::array<bool, 3> Negs;
};

class BfnMatcher {
public:
  explicit BfnMatcher(Instruction *I) {
    IGC_ASSERT_MESSAGE(I, "null instruction");
    MainBfnInst = I;
    PrevBfnInst = nullptr;
    // in prevBfnInst operands are Srcs[0] and Srcs[1]
    // MainBfnInst is prevInst operands are result of prevInst and Srcs[2]
    Srcs[1] = I->getOperand(0);
    Srcs[2] = I->getOperand(1);
    Srcs[0] = nullptr;
    IGC_ASSERT_MESSAGE(Srcs[1] && Srcs[2], "null operands");
  }

  bool match();

private:
  static bool checkBfnTypes(const Value *V) {
    IGC_ASSERT_MESSAGE(V, "Error: nullptr input");
    return V->getType()->isIntOrIntVectorTy(16) ||
           V->getType()->isIntOrIntVectorTy(32);
  }
  // These constants are from VISA docs for calculating bfn constant.
  // Combine these constants with any logical operations to get the function
  // index for that logical combination
  using FunctionIndexT = unsigned char;
  static constexpr std::array<FunctionIndexT, 3> BfnIndex = {0xaa, 0xcc, 0xf0};
  FunctionIndexT getFunctionIndex() const;

  void growPattern(const Use *U);

  // Return true if changes are made.
  bool emit();

  // The instructions in the bfn matching:
  // and/or/xor
  Instruction *MainBfnInst;
  Instruction *PrevBfnInst;

  // Source operands for the bfn intrinsic call
  std::array<Value *, 3> Srcs;
};

// Class to identify cases where a comparison and select are equivalent to a
// min or max operation. These are replaced by a min/max intrinsic which allows
// the jitter to produce better code for these cases.
class MinMaxMatcher {
public:
  explicit MinMaxMatcher(Instruction *I)
      : SelInst(I), CmpInst(nullptr), ID(GenXIntrinsic::not_any_intrinsic) {
    IGC_ASSERT_MESSAGE(I, "null instruction");
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
  const GenXSubtarget *ST = &getAnalysis<TargetPassConfig>()
                                 .getTM<GenXTargetMachine>()
                                 .getGenXSubtarget();
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
      else if (ST && (ST->hasAdd3Bfn()))
        Changed |= EnableAdd3Matcher && Add3Matcher(&I).matchIntegerAdd3();
      break;
    case Instruction::And:
      if (I.getType()->getScalarType()->isIntegerTy(1)) {
        if (foldBoolAnd(&I))
          Changed = true;
      } else if (extendMask(&I))
        Changed = true;
      else if (ST && (ST->hasAdd3Bfn()))
        Changed |= EnableBfnMatcher && BfnMatcher(&I).match();
      break;
    case Instruction::Or:
    case Instruction::Xor:
      if (!I.getType()->getScalarType()->isIntegerTy(1) &&
          (ST && ST->hasAdd3Bfn())) {
        Changed |= EnableBfnMatcher && BfnMatcher(&I).match();
      } else
      if (extendMask(&I))
        Changed = true;
      break;
    }
}

void GenXPatternMatch::visitCallInst(CallInst &I) {
  switch (unsigned ID = GenXIntrinsic::getGenXIntrinsicID(&I)) {
  default:
    break;
  case GenXIntrinsic::genx_inv:
    Changed |= matchInverseSqrt(&I);
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
  case GenXIntrinsic::genx_lsc_load_slm:
  case GenXIntrinsic::genx_lsc_load_stateless:
  case GenXIntrinsic::genx_lsc_load_bindless:
  case GenXIntrinsic::genx_lsc_load_bti:
  case GenXIntrinsic::genx_lsc_prefetch_slm:
  case GenXIntrinsic::genx_lsc_prefetch_bti:
  case GenXIntrinsic::genx_lsc_prefetch_stateless:
  case GenXIntrinsic::genx_lsc_prefetch_bindless:
  case GenXIntrinsic::genx_lsc_load_quad_slm:
  case GenXIntrinsic::genx_lsc_load_quad_stateless:
  case GenXIntrinsic::genx_lsc_load_quad_bindless:
  case GenXIntrinsic::genx_lsc_load_quad_bti:
  case GenXIntrinsic::genx_lsc_store_slm:
  case GenXIntrinsic::genx_lsc_store_stateless:
  case GenXIntrinsic::genx_lsc_store_bindless:
  case GenXIntrinsic::genx_lsc_store_bti:
  case GenXIntrinsic::genx_lsc_store_quad_slm:
  case GenXIntrinsic::genx_lsc_store_quad_stateless:
  case GenXIntrinsic::genx_lsc_store_quad_bindless:
  case GenXIntrinsic::genx_lsc_store_quad_bti:
  case GenXIntrinsic::genx_lsc_xatomic_slm:
  case GenXIntrinsic::genx_lsc_xatomic_stateless:
  case GenXIntrinsic::genx_lsc_xatomic_bindless:
  case GenXIntrinsic::genx_lsc_xatomic_bti:
    Changed |= foldLscAddrCalculation(&I);
  case GenXIntrinsic::genx_dword_atomic_fadd:
  case GenXIntrinsic::genx_dword_atomic_fsub:
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
      unsigned NElts = cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements();
      unsigned BitWidth = Elt->getType()->getPrimitiveSizeInBits();
      if (Val == Int16Mask && NBits + 16 >= BitWidth)
        DstTy = IGCLLVM::FixedVectorType::get(Builder.getInt16Ty(), NElts);
      else if (Val == Int8Mask && NBits + 8 >= BitWidth)
        DstTy = IGCLLVM::FixedVectorType::get(Builder.getInt8Ty(), NElts);

      // Lower trunc to bitcast followed by a region read
      // as such bitcast is not support after IR lowering.
      if (DstTy) {
        Type *InEltTy = cast<VectorType>(Ty)->getElementType();
        Type *OutEltTy = cast<VectorType>(DstTy)->getElementType();
        IGC_ASSERT(OutEltTy->getPrimitiveSizeInBits());
        unsigned Stride = InEltTy->getPrimitiveSizeInBits() /
                          OutEltTy->getPrimitiveSizeInBits();
        // Create the new bitcast.
        Instruction *BC = CastInst::Create(
            Instruction::BitCast, V0,
            IGCLLVM::FixedVectorType::get(OutEltTy, Stride * NElts), ".bc",
            &I /*InsertBefore*/);
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
        IGC_ASSERT(isa<Constant>(RHS));
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
          IGC_ASSERT(OpStack.size() >= 2);
          RHS = OpStack.pop_back_val();
          LHS = OpStack.pop_back_val();
          OpStack.push_back(Builder.CreateOr(LHS, RHS));
          continue;
        }
        if (match(V, m_And(m_Value(LHS), m_Value(RHS)))) {
          IGC_ASSERT(OpStack.size() >= 2);
          RHS = OpStack.pop_back_val();
          LHS = OpStack.pop_back_val();
          OpStack.push_back(Builder.CreateAnd(LHS, RHS));
          continue;
        }
        if (match(V, m_Not(m_Value(LHS)))) {
          IGC_ASSERT(OpStack.size() >= 1);
          LHS = OpStack.pop_back_val();
          OpStack.push_back(Builder.CreateNot(LHS));
        }
        IGC_ASSERT_MESSAGE(0, "Unhandled logic op!");
      }
      IGC_ASSERT(OpStack.size() == 1);
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
    auto *VTy = cast<IGCLLVM::FixedVectorType>(V0->getType());
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
        VTy = cast<IGCLLVM::FixedVectorType>(Cmp->getType());
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
  auto *VTy = cast<IGCLLVM::FixedVectorType>(V0->getType());
  unsigned NumElts = VTy->getNumElements();

  Region R = makeRegionFromBaleInfo(WII, BaleInfo());
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

static Instruction *createInverseSqrt(Value *Op, Instruction *InsertBefore) {
  IGC_ASSERT(Op && InsertBefore);
  Function *Decl = GenXIntrinsic::getGenXDeclaration(
      InsertBefore->getModule(), GenXIntrinsic::genx_rsqrt, {Op->getType()});
  auto *RSqrt =
      CallInst::Create(Decl, {Op}, Op->getName() + "inversed", InsertBefore);
  RSqrt->setDebugLoc(InsertBefore->getDebugLoc());
  return RSqrt;
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

/// (inv (sqrt x)) -> (rsqrt x)
bool GenXPatternMatch::matchInverseSqrt(Instruction *I) {
  IGC_ASSERT(I);

  auto *OpInst = dyn_cast<CallInst>(I->getOperand(0));
  if (!OpInst)
    return false;

  // Leave as it is for double types
  if (OpInst->getType()->getScalarType()->isDoubleTy())
    return false;

  // Generate inverse sqrt only if fast flag for llvm intrinsic is used or
  // genx sqrt intrinsics is specified
  auto IID = vc::getAnyIntrinsicID(OpInst);
  if (!(IID == GenXIntrinsic::genx_sqrt ||
        (IID == Intrinsic::sqrt && OpInst->getFastMathFlags().isFast())))
    return false;

  // Leave as it if sqrt has multiple uses:
  // generating rsqrt operation is not beneficial
  if (OpInst->getNumUses() > 1)
    return false;

  auto *Rsqrt = createInverseSqrt(OpInst->getOperand(0), I->getNextNode());
  I->replaceAllUsesWith(Rsqrt);
  I->eraseFromParent();

  OpInst->eraseFromParent();
  return true;
}

/***********************************************************************
 * applyLscAddrFolding : fold address calculation of LSC intriniscs
 * Addr = ImmOffset + Offsets * Scale
 * If Offsets is add-like operation (Offsets = Offsets0 + Imm0), it can be
 * folded in new ImmOffset: (ImmOffset + Imm0 * Scale) + Offsets0 * Scale
 * If Offsets is mul-like operation (Offsets = Offsets0 * Imm0), it can be
 * folded in new Scale: ImmOffset + Offsets * (Scale * Imm0)
 * This folding is done iteratively for chains of such operations.
 */
static bool
applyLscAddrFolding(Value *Offsets, APInt& Scale, APInt& Offset) {
  if (!Offsets->hasOneUse())
    return false;
  if (!isa<BinaryOperator>(Offsets))
    return false;
  auto *BinOp = cast<BinaryOperator>(Offsets);
  unsigned ConstIdx;
  if (isa<Constant>(BinOp->getOperand(0)))
    ConstIdx = 0;
  else if (isa<Constant>(BinOp->getOperand(1)))
    ConstIdx = 1;
  else
    return false;
  auto *ConstOp = cast<Constant>(BinOp->getOperand(ConstIdx));
  if (!isa<ConstantInt>(ConstOp) &&
      (!ConstOp->getType()->isVectorTy() || !ConstOp->getSplatValue()))
    return false;
  auto Imm = ConstOp->getUniqueInteger();

  auto NewScale(Scale);
  auto NewOffset(Offset);
  bool Overflow = false;
  switch (BinOp->getOpcode()) {
    case Instruction::Add:
    case Instruction::Sub:
      if (!EnableLscAddrFoldOffset)
        return false;
      if (Imm.getMinSignedBits() > Offset.getBitWidth())
        return false;
      Imm = Imm.sextOrTrunc(Offset.getBitWidth())
               .smul_ov(Scale.zext(Imm.getBitWidth()), Overflow);
      if (Overflow)
        return false;
      if (BinOp->getOpcode() == Instruction::Add)
        NewOffset = Offset.sadd_ov(Imm, Overflow);
      else if (BinOp->getOpcode() == Instruction::Sub)
        NewOffset = Offset.ssub_ov(Imm, Overflow);
      break;
    case Instruction::Mul:
      if (!EnableLscAddrFoldScale)
        return false;
      if (!Imm.isIntN(Scale.getBitWidth()))
        return false;
      if (Imm.getBitWidth() > Scale.getBitWidth())
        Imm = Imm.trunc(Scale.getBitWidth());
      NewScale = Scale.umul_ov(Imm, Overflow);
      break;
    case Instruction::Shl:
      if (!EnableLscAddrFoldScale)
        return false;
      NewScale = Scale.ushl_ov(Imm, Overflow);
      break;
    default:
      return false;
  }

  if (Overflow)
    return false;
  Scale = NewScale;
  Offset = NewOffset;
  BinOp->replaceAllUsesWith(BinOp->getOperand(1 - ConstIdx));
  BinOp->eraseFromParent();
  return true;
}

bool GenXPatternMatch::foldLscAddrCalculation(CallInst *Inst) {
  constexpr unsigned AddrScaleOpndNum = 4,
                     ImmOffsetOpndNum = 5,
                     OffsetsOpndNum = 10;
  IGC_ASSERT_MESSAGE(isa<ConstantInt>(Inst->getOperand(AddrScaleOpndNum)) &&
                     isa<ConstantInt>(Inst->getOperand(ImmOffsetOpndNum)),
                     "Scale and ImmOffset should be constant");
  auto Scale = cast<ConstantInt>(Inst->getOperand(AddrScaleOpndNum))->getValue();
  auto Offset = cast<ConstantInt>(Inst->getOperand(ImmOffsetOpndNum))->getValue();
  bool Changed = false;
  while (applyLscAddrFolding(Inst->getOperand(OffsetsOpndNum), Scale, Offset))
    Changed = true;
  if (Changed) {
    Inst->setOperand(AddrScaleOpndNum,
        ConstantInt::get(Type::getInt16Ty(Inst->getContext()), Scale));
    Inst->setOperand(ImmOffsetOpndNum,
        ConstantInt::get(Type::getInt32Ty(Inst->getContext()), Offset));
  }
  return Changed;
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
  Region R = makeRegionFromBaleInfo(user, BaleInfo());
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
  auto NewWrRegion = R.createWrRegion(user->getOperand(0), NewSel, "", user,
                                      user->getDebugLoc());
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
    Region R = makeRegionFromBaleInfo(cast<Instruction>(V), BaleInfo());
    return R.Indirect;
  };

  auto isIndirectWrRegion = [](User *U) -> bool {
    if (!GenXIntrinsic::isWrRegion(U))
      return false;
    Region R = makeRegionFromBaleInfo(cast<Instruction>(U), BaleInfo());
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
      C = dyn_cast_or_null<ConstantInt>(cast<Constant>(V)->getSplatValue());
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
  auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(V->getType());
  // Skip if it's not vector type.
  if (!VTy)
    return nullptr;
  // Skip if it's not from rdregion.
  if (!GenXIntrinsic::isRdRegion(V))
    return nullptr;
  GenXIntrinsicInst *RII = cast<GenXIntrinsicInst>(V);
  Region R = makeRegionFromBaleInfo(RII, BaleInfo());
  if (!R.isScalar() || R.Width != 1 || R.Offset != 0)
    return nullptr;
  Value *Src = RII->getArgOperand(0);
  auto *BC = dyn_cast<BitCastInst>(Src);
  if (!BC)
    return nullptr;
  VTy = dyn_cast<IGCLLVM::FixedVectorType>(BC->getType());
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
  Type *ScalarType = V->getType()->getScalarType();
  IGC_ASSERT_MESSAGE(ScalarType->isIntegerTy(32), "I32 is expected!");
  if (auto Ext = dyn_cast<ExtOperator>(V)) {
    V = Ext->getOperand(0);
    if (V->getType()->getScalarType()->isIntegerTy(8)) {
      Type *DstTy = Builder.getInt16Ty();
      if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(V->getType()))
        DstTy = IGCLLVM::FixedVectorType::get(DstTy, VTy->getNumElements());
      // Extend to i16 first.
      V = Builder.CreateCast(Instruction::CastOps(Ext->getOpcode()), V, DstTy);
    }
    if (!V->getType()->isVectorTy()) {
      // Broadcast through rdregion.
      Type *NewTy = IGCLLVM::FixedVectorType::get(V->getType(), 1);
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
      V = ConstantVector::getSplat(IGCLLVM::getElementCount(NumElts),
                                   Builder.getIntN(16, Val.getZExtValue()));
      return std::make_tuple(V, Val.isSignedIntN(16));
    }
  }
  return std::make_tuple(nullptr, false);
}

// The floating point case is relatively simple. Only need to match with fmul.
bool MadMatcher::matchFpMad() {
  const bool isFAdd = AInst->getOpcode() == Instruction::FAdd;
  const bool isFSub = AInst->getOpcode() == Instruction::FSub;
  IGC_ASSERT(isFAdd || isFSub);
  (void) isFAdd;

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
        if (isFSub)
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
          if (isFSub)
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
  const bool isAdd = AInst->getOpcode() == Instruction::Add;
  const bool isSub = AInst->getOpcode() == Instruction::Sub;
  IGC_ASSERT(isAdd || isSub);
  (void) isAdd;

  Value *Ops[2] = {AInst->getOperand(0), AInst->getOperand(1)};

  if (auto BI = dyn_cast<MulLikeOperator>(Ops[0])) {
    // Case X * Y +/- Z
    Srcs[2] = Ops[1];
    Srcs[1] = BI->getOperand(1);
    Srcs[0] = BI->getOperand(0);
    setMInst(cast<Instruction>(BI));
    if (isProfitable()) {
      if (isSub)
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
        if (isSub)
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
          if (isSub)
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
          if (isSub)
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

bool MadMatcher::emit() {
  if (MInst == nullptr || !isProfitable() || isOperationOnI64())
    return false;

  IRBuilder<> Builder(AInst);

  auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Srcs[2]->getType());
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
        auto IID = GenXIntrinsic::getGenXMulIID(S0, S1);
        Module *M = AInst->getParent()->getParent()->getParent();
        Type *Tys[2] = {VTy, V0->getType()};
        Function *Fn = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
        IGC_ASSERT(V0->getType() == V1->getType());
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

  if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Vals[2]->getType())) {
    // Splat scalar sources if necessary.
    for (unsigned i = 0; i != 2; ++i) {
      Value *V = Vals[i];
      if (V->getType()->isVectorTy())
        continue;
      if (auto C = dyn_cast<Constant>(V)) {
        Vals[i] = ConstantVector::getSplat(
            IGCLLVM::getElementCount(VTy->getNumElements()), C);
        continue;
      }
      auto Ext = dyn_cast<ExtOperator>(V);
      if (Ext)
        V = Ext->getOperand(0);
      Type *NewTy = IGCLLVM::FixedVectorType::get(V->getType(), 1);
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
      Base = ConstantVector::getSplat(
          IGCLLVM::getElementCount(
              cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements()),
          Base);
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

/// The beginning of the Add3Matcher
bool Add3Matcher::isProfitable(Instruction *A2) const {
  // Do not match unused instructions.
  if (AInst->use_empty())
    return false;

  if (!A2->hasOneUse())
    return false;

  auto nb1 = AInst->getType()->getScalarSizeInBits();
  if (nb1 != 16 && nb1 != 32)
    return false;

  int nc = 0;
  for (int i = 0; i < 3; ++i) {
    // all sources must be 16-bit or 32-bit
    auto Ext = dyn_cast<ExtOperator>(Srcs[i]);
    if (Ext) {
      Value *RV = Ext->getOperand(0);
      auto nb3 = RV->getType()->getScalarSizeInBits();
      if (nb3 != 16 && nb3 != 32)
        return false;
    }
    if (auto C = dyn_cast<Constant>(Srcs[i])) {
      nc++;
      // less than two immediates
      if (nc > 1)
        return false;
      // the immediate must be less than 16-bits
      if (!C->getType()->isVectorTy() || C->getSplatValue()) {
        if (C->getUniqueInteger().uge(1 << 16))
          return false;
      }
    }
  }
  return true;
}

bool Add3Matcher::matchIntegerAdd3() {
  const bool isAdd = AInst->getOpcode() == Instruction::Add;
  const bool isSub = AInst->getOpcode() == Instruction::Sub;
  IGC_ASSERT(isAdd || isSub);
  (void) isAdd;

  Value *Ops[2] = {AInst->getOperand(0), AInst->getOperand(1)};

  if (Instruction *BI = dyn_cast<Instruction>(Ops[0])) {
    if (BI->getOpcode() == Instruction::Add ||
        BI->getOpcode() == Instruction::Sub) {
      // Case X +/- Y +/- Z
      Srcs[2] = Ops[1];
      Srcs[1] = BI->getOperand(1);
      Srcs[0] = BI->getOperand(0);
      if (isProfitable(BI)) {
        setA2Inst(BI);
        Negs[2] = (isSub);
        Negs[1] = (A2Inst->getOpcode() == Instruction::Sub);
      }
    }
  }

  if (!A2Inst) {
    if (Instruction *BI = dyn_cast<Instruction>(Ops[1])) {
      if (BI->getOpcode() == Instruction::Add ||
          BI->getOpcode() == Instruction::Sub) {
        // Case Z +/- (X +/- Y)
        Srcs[0] = Ops[0];
        Srcs[1] = BI->getOperand(0);
        Srcs[2] = BI->getOperand(1);
        if (isProfitable(BI)) {
          setA2Inst(BI);
          Negs[1] = (isSub);
          Negs[2] = (AInst->getOpcode() != A2Inst->getOpcode());
        }
      }
    }
  }
  if (!A2Inst) { // Check if operand 0 is broadcasted from scalar.
    if (auto S = getBroadcastFromScalar(Ops[0])) {
      if (Instruction *BI = dyn_cast<Instruction>(S)) {
        if (BI->getOpcode() == Instruction::Add ||
            BI->getOpcode() == Instruction::Sub) {
          // Case X +/- Y +/- Z
          Srcs[2] = Ops[1];
          Srcs[1] = BI->getOperand(1);
          Srcs[0] = BI->getOperand(0);
          if (isProfitable(BI)) {
            setA2Inst(BI);
            Negs[2] = (isSub);
            Negs[1] = (A2Inst->getOpcode() == Instruction::Sub);
          }
        }
      }
    }
  }

  if (!A2Inst) { // Check if operand 1 is broadcasted from scalar.
    if (auto S = getBroadcastFromScalar(Ops[1])) {
      if (Instruction *BI = dyn_cast<Instruction>(S)) {
        if (BI->getOpcode() == Instruction::Add ||
            BI->getOpcode() == Instruction::Sub) {
          // Case Z +/- (X +/- Y)
          Srcs[0] = Ops[0];
          Srcs[1] = BI->getOperand(0);
          Srcs[2] = BI->getOperand(1);
          if (isProfitable(BI)) {
            setA2Inst(BI);
            Negs[1] = (isSub);
            Negs[2] = (AInst->getOpcode() != A2Inst->getOpcode());
          }
        }
      }
    }
  }

  // Always use add3.
  ID = GenXIntrinsic::genx_add3;

  // Emit add3 if matched and profitable.
  return emit();
}

// If Value *V is scalar type, return new Value* - splat of a specific type
// for example, if VTy is <2 x i32>, and V is i32 type constant i32 7, we return
// pointer to new Value <7, 7>
Value *SplatValueIfNecessary(Value *V, IGCLLVM::FixedVectorType *VTy,
                             IRBuilder<> &Builder) {
  IGC_ASSERT_MESSAGE(V && VTy, "Error: get nullptr input");
  IGC_ASSERT_MESSAGE(V->getType()->isIntOrIntVectorTy() &&
                         VTy->isIntOrIntVectorTy(),
                     "Error: expect integer types");
  // currenty zext is after add, so types should be the same
  IGC_ASSERT_MESSAGE(V->getType()->getScalarType() == VTy->getScalarType(),
                     "Error: not corresponding types");
  if (V->getType()->isVectorTy())
    return V;
  if (auto C = dyn_cast<Constant>(V)) {
    return ConstantVector::getSplat(
        IGCLLVM::getElementCount(VTy->getNumElements()), C);
  }
  auto* Ext = dyn_cast<ExtOperator>(V);
  if (Ext)
    V = Ext->getOperand(0);
  Type *NewTy = IGCLLVM::FixedVectorType::get(V->getType(), 1);
  V = Builder.CreateBitCast(V, NewTy);
  // Broadcast through rdregin.
  Region R(V);
  R.Offset = 0;
  R.Width = 1;
  R.Stride = R.VStride = 0;
  R.NumElements = VTy->getNumElements();
  Instruction *InsertionPt = &(*Builder.GetInsertPoint());
  V = R.createRdRegion(V, ".splat", InsertionPt, InsertionPt->getDebugLoc());
  if (Ext)
    V = Builder.CreateCast(Instruction::CastOps(Ext->getOpcode()), V, VTy);
  return V;
};

bool Add3Matcher::emit() {
  if (A2Inst == nullptr)
    return false;

  IRBuilder<> Builder(AInst);

  std::array<Value *, 3> Vals(Srcs);

  if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(AInst->getType())) {
    // Splat scalar sources if necessary.
    std::transform(Vals.begin(), Vals.end(), Vals.begin(),
                   [&Builder, VTy](Value *V) {
                     return SplatValueIfNecessary(V, VTy, Builder);
                   });
  }
  // Perform source operand negation if necessary.
  for (int i = 0; i < 3; ++i)
    if (Negs[i]) {
      Vals[i] = Builder.CreateNeg(Vals[i], "neg");
    }

  Function *Fn = nullptr;
  {
    Module *M = AInst->getParent()->getParent()->getParent();
    Type *Tys[2] = {AInst->getType(), Vals[0]->getType()};
    Fn = GenXIntrinsic::getAnyDeclaration(M, ID, Tys);
  }
  IGC_ASSERT_MESSAGE(Fn, "not found currect intrinsic");
  IGC_ASSERT_MESSAGE(Vals[0]->getType() == Vals[1]->getType() &&
                         Vals[1]->getType() == Vals[2]->getType(),
                     "add3 should have the same type Operands");
  CallInst *CI = Builder.CreateCall(Fn, Vals, VALUE_NAME("add3"));
  CI->setDebugLoc(AInst->getDebugLoc());
  AInst->replaceAllUsesWith(CI);

  NumOfAdd3Matched++;
  return true;
}

/// the Beginning of the BfnMatcher
bool BfnMatcher::match() {
  if (MainBfnInst->use_empty())
    return false;
  if (!checkBfnTypes(MainBfnInst))
    return false;

  Use *U = std::find_if(
      MainBfnInst->op_begin(), MainBfnInst->op_end(), [](const Use &U) {
        Instruction *I = dyn_cast<Instruction>(U.get());
        if (!I)
          return false;
        if (!I->hasOneUse())
          return false;
        return I->isBitwiseLogicOp();
      });
  if (U == MainBfnInst->op_end())
    return false;

  growPattern(U);
  return emit();
}

void BfnMatcher::growPattern(const Use *U) {
  IGC_ASSERT(U);
  Instruction *I = cast<Instruction>(U->get());
  IGC_ASSERT(I && I->isBitwiseLogicOp());

  unsigned SrcIdx = U->getOperandNo();
  // we do not work with this operand
  Value *BfnOtherOperand = MainBfnInst->getOperand((SrcIdx + 1) % 2);

  Srcs[0] = I->getOperand(0);
  Srcs[1] = I->getOperand(1);
  Srcs[2] = BfnOtherOperand;
  PrevBfnInst = I;
}

BfnMatcher::FunctionIndexT BfnMatcher::getFunctionIndex() const {
  // calculation left to right, with no brackets,
  // so first calculation for PrevBfnInst, next MainBfnInst

  auto calculateOperation = [](unsigned Opcode, FunctionIndexT Op0,
                               FunctionIndexT Op1) {
    switch (Opcode) {
    case Instruction::And:
      return Op0 & Op1;
    case Instruction::Or:
      return Op0 | Op1;
    case Instruction::Xor:
      return Op0 ^ Op1;
    default:
      IGC_ASSERT_MESSAGE(false, "Wrong Opcode");
      return 0;
    }
  };
  FunctionIndexT result =
      calculateOperation(PrevBfnInst->getOpcode(), BfnIndex[0], BfnIndex[1]);
  return calculateOperation(MainBfnInst->getOpcode(), result, BfnIndex[2]);
}

// Return true if changes are made.
bool BfnMatcher::emit() {
  IGC_ASSERT_MESSAGE(Srcs[0] && Srcs[1] && Srcs[2] && MainBfnInst &&
                         PrevBfnInst,
                     "Error: wrong class structure");
  FunctionIndexT Index = getFunctionIndex();

  IRBuilder<> Builder{MainBfnInst};
  // create the BFN call
  Function *Fn = nullptr;
  {
    Module *M = MainBfnInst->getParent()->getParent()->getParent();
    Type *Tys[2] = {MainBfnInst->getType(), Srcs[0]->getType()};
    Fn = GenXIntrinsic::getGenXDeclaration(M, GenXIntrinsic::genx_bfn, Tys);
  }
  std::array<Value *, 4> Args;

  std::copy(Srcs.begin(), Srcs.end(), Args.begin());
  Args.back() = Builder.getInt8(Index);

  CallInst *CI = Builder.CreateCall(Fn, Args, "bfn");
  MainBfnInst->replaceAllUsesWith(CI);

  NumOfBfnMatched++;
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
    if (IGCLLVM::getElementCount(*C1) != IGCLLVM::getElementCount(*C2))
      return false;
    Type *C1Ty = C1->getType();
    Type *C2Ty = C2->getType();
    if (auto C1VTy = dyn_cast<VectorType>(C1Ty)) {
      C1Ty = C1VTy->getElementType();
      C2Ty = cast<VectorType>(C2Ty)->getElementType();
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
  IGC_ASSERT_MESSAGE(SelInst->getOpcode() == Instruction::Select,
    "expected SelectInst");
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
  IGC_ASSERT_MESSAGE(!isa<PHINode>(Ref), "PHINode is not expected!");

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

  IGC_ASSERT_MESSAGE(!BBs.empty(), "Must find at least one BB!");

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
  IGC_ASSERT(BB);
  return std::make_tuple(BB, Pos);
}

// For the specified constant, calculate its reciprocal if it's safe;
// otherwise, return null.
static Constant *getReciprocal(Constant *C, bool HasAllowReciprocal) {
  IGC_ASSERT_MESSAGE(C->getType()->isFPOrFPVectorTy(),
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

  auto *VTy = cast<IGCLLVM::FixedVectorType>(C->getType());
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

  // (fdiv 1., (sqrt x)) -> (rsqrt x)
  // TODO: This can be removed if pattern match is applied
  // incrementally: first match reciprocal, then generate rsqrt
  // when visiting it
  auto IID = vc::getAnyIntrinsicID(Divisor);
  if ((IID == GenXIntrinsic::genx_sqrt ||
       (IID == Intrinsic::sqrt && Divisor->getFastMathFlags().isFast())) &&
      match(Op0, m_FPOne()) && Divisor->hasOneUse()) {
    auto *Rsqrt = createInverseSqrt(Divisor->getOperand(0), Pos);
    I.replaceAllUsesWith(Rsqrt);
    I.eraseFromParent();
    Divisor->eraseFromParent();

    Changed |= true;
    return;
  }

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
  static const MulLike &get(Instruction *I);

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

const MulLike &MulLike::get(Instruction *I) {
  Type *Ty = I->getType()->getScalarType();
  if (Ty->isFloatingPointTy()) {
    static const FPMulLike FPMul;
    return FPMul;
  }
  if (Ty->isIntegerTy()) {
    static const IntMulLike IntMul;
    return IntMul;
  }
  static const MulLike Null;
  return Null;
}
} // End anonymous namespace

bool GenXPatternMatch::propagateFoldableRegion(Function *F) {
  ReversePostOrderTraversal<Function *> RPOT(F);
  bool Changed = false;
  for (auto *BB : RPOT)
    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
      const MulLike &Ring = MulLike::get(&*BI);
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
        Region W = makeRegionFromBaleInfo(WII, BaleInfo());
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
              Region R = makeRegionFromBaleInfo(RII, BaleInfo());
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
              Region W2 = makeRegionFromBaleInfo(WII2, BaleInfo());
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
  IGC_ASSERT(GenXIntrinsic::getGenXIntrinsicID(CI) == GenXIntrinsic::genx_rdpredregion);
  bool Changed = false;

  unsigned NElts =
      cast<IGCLLVM::FixedVectorType>(CI->getType())->getNumElements();
  ConstantInt *C = dyn_cast<ConstantInt>(CI->getArgOperand(1));
  IGC_ASSERT_MESSAGE(C, "constant integer expected");
  unsigned Offset = (unsigned)C->getZExtValue();
  IGC_ASSERT(NElts);
  IGC_ASSERT(Offset % NElts == 0);

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
  IGC_ASSERT(GenXIntrinsic::isRdRegion(Inst));
  auto NewVTy = Inst->getType();
  // rewrite indirect rdregion with constant offsets
  auto R = genx::makeRegionWithOffset(Inst);
  if (R.Indirect && R.IndirectIdx == 0 && R.IndirectAddrOffset == 0) {
    int64_t starti = 0;
    int64_t diffi = 0;
    if (IsLinearVectorConstantInts(R.Indirect, starti, diffi)) {
      R.Indirect = nullptr;
      R.Width = cast<IGCLLVM::FixedVectorType>(NewVTy)->getNumElements();
      R.Offset += starti;
      R.Stride =
          (diffi * 8) /
          cast<VectorType>(NewVTy)->getElementType()->getPrimitiveSizeInBits();
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
  IGC_ASSERT(GenXIntrinsic::isWrRegion(Inst));
  Value *NewV = Inst->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum);
  Type *NewVTy = NewV->getType();

  // Rewrite a single element insertion to undef as a region splat.
  auto check1 = [=]() {
    Value *OldV = Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
    if (!isa<UndefValue>(OldV))
      return false;

    // have to keep faddr's wrregion to ensure faddr's proper baling
    auto *Parent = NewV;
    while (isa<BitCastInst>(Parent))
      Parent = cast<Instruction>(Parent)->getOperand(0);
    if (vc::getAnyIntrinsicID(Parent) == GenXIntrinsic::genx_faddr)
      return false;

    if (NewVTy->isVectorTy() &&
        cast<IGCLLVM::FixedVectorType>(NewVTy)->getNumElements() > 1)
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

      if (GenXIntrinsic::isReadWritePredefReg(U))
        return false;
    }

    // OK, rewrite it!
    return true;
  };

  if (check1()) {
    if (!NewVTy->isVectorTy()) {
      IRBuilder<> B(Inst);
      NewV = B.CreateBitCast(NewV, IGCLLVM::FixedVectorType::get(NewVTy, 1));
    }
    Region R(Inst->getType());
    R.Width = R.NumElements;
    R.Stride = 0;
    NewV = R.createRdRegion(NewV, "splat", Inst, Inst->getDebugLoc(),
                            !Inst->getType()->isVectorTy());
    Inst->replaceAllUsesWith(NewV);
    return true;
  }

  // rewrite indirect wrregion with constant offsets
  auto R = genx::makeRegionWithOffset(Inst);
  if (R.Indirect && R.IndirectIdx == 0 && R.IndirectAddrOffset == 0) {
    int64_t starti = 0;
    int64_t diffi = 0;
    if (IsLinearVectorConstantInts(R.Indirect, starti, diffi)) {
      R.Indirect = nullptr;
      R.Width = cast<IGCLLVM::FixedVectorType>(NewVTy)->getNumElements();
      R.Offset += starti;
      R.Stride =
          (diffi * 8) /
          cast<VectorType>(NewVTy)->getElementType()->getPrimitiveSizeInBits();
      R.VStride = 0;
      Value* OldV = Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
      auto NewInst = R.createWrRegion(OldV, NewV, Inst->getName(),
          Inst /*InsertBefore*/, Inst->getDebugLoc());
      Inst->replaceAllUsesWith(NewInst);
      return true;
    }
  }

  // Convert WrRegion to a matching Select instruction
  // Also perform Min/Max optimization if enabled
  if (R.isWhole(Inst->getType(), DL)) {
    Value *OldV =
        Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
    Type *OldVTy = OldV->getType();

    Value *MaskV =
        Inst->getOperand(GenXIntrinsic::GenXRegion::PredicateOperandNum);
    Type *MaskVTy = MaskV->getType();

    if (!(isa<UndefValue>(OldV)) && OldVTy->isVectorTy() &&
        NewVTy->isVectorTy() && MaskVTy->isVectorTy() &&
        cast<IGCLLVM::FixedVectorType>(OldVTy)->getNumElements() ==
            cast<IGCLLVM::FixedVectorType>(NewVTy)->getNumElements() &&
        cast<IGCLLVM::FixedVectorType>(OldVTy)->getNumElements() ==
            cast<IGCLLVM::FixedVectorType>(MaskVTy)->getNumElements()) {
      Instruction *InsertBefore = Inst->getNextNode();
      auto SelectInstruction =
          SelectInst::Create(MaskV, NewV, OldV, "", InsertBefore, Inst);
      SelectInstruction->setDebugLoc(Inst->getDebugLoc());
      SelectInstruction->takeName(Inst);
      Inst->replaceAllUsesWith(SelectInstruction);
      Inst->eraseFromParent();

      if (MinMaxMatcher::isEnabled())
        MinMaxMatcher(SelectInstruction).matchMinMax();

      return true;
    }
  }

  return false;
}

// Simplify (trunc.sat (ext V)) to (trunc.sat V). Even if the source and
// destination has the same type, it's incorrect to fold them into V directly
// as the saturation is necessary.
bool GenXPatternMatch::simplifyTruncSat(CallInst *Inst) {
  IGC_ASSERT_MESSAGE(GenXIntrinsic::isIntegerSat(Inst),
    "Unexpected integer saturation intrinsic!");

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
    IGC_ASSERT_EXIT_MESSAGE(0, "Unknown intrinsic!");
  }

  Module *M = Inst->getParent()->getParent()->getParent();
  Type *Tys[2] = {Inst->getType(), Src->getType()};
  Function *Fn = GenXIntrinsic::getGenXDeclaration(M, IID, Tys);

  Inst->setCalledFunction(Fn);
  Inst->setOperand(0, Src);

  return true;
}

// Try to canonize select as masked operation:
// sel_val = select cond, val, identity_vec
// masked_val = binary_operation old_val, sel_val
// ===>
// new_val = binary_operation oldval, val
// masked_val = select cond, new_val, old_val
//
static bool canonizeSelect(SelectInst *SI) {
  // Multiple uses can increase number of instructions.
  if (!SI->hasOneUse())
    return false;
  const Use &U = *SI->use_begin();
  BinaryOperator *BI = dyn_cast<BinaryOperator>(U.getUser());
  if (!BI)
    return false;
  // Get identity value for this binary instruction, allowing non-commutative ones
  // if select value placed on second operand.
  auto BinOpIdentity = ConstantExpr::getBinOpIdentity(BI->getOpcode(), BI->getType(),
      /*AllowRHSConstant*/ U.getOperandNo() == 1);
  for (unsigned i = 1; i <= 2; ++i) {
    if (SI->getOperand(i) != BinOpIdentity)
      continue;
    BI->setOperand(U.getOperandNo(), SI->getOperand(3 - i));
    Value *OldVal = BI->getOperand(1 - U.getOperandNo());
    UndefValue *UndefVal = UndefValue::get(SI->getType());
    SelectInst *NewSI =
        SelectInst::Create(SI->getCondition(), UndefVal, UndefVal,
                           SI->getName(), BI->getNextNode(), SI);
    BI->replaceAllUsesWith(NewSI);
    NewSI->setTrueValue(i == 1 ? OldVal : BI);
    NewSI->setFalseValue(i == 1 ? BI : OldVal);
    return true;
  }
  return false;
}

// Try to merge select condition to wrregion mask:
//
// Scenario 1
// old_val = rdregion(x, R)
// masked_val = select cond, new_val, old_val
// wrregion(x, masked_val, R)
// ===>
// old_val = rdregion(x, R)
// wrregion(x, new_val, R, cond)
//
// Scenario 2 (replace undef with x)
// old_val = rdregion(x, R)
// masked_val = select cond, new_val, old_val
// wrregion(undef, masked_val, R)
// ===>
// old_val = rdregion(x, R)
// wrregion(x, new_val, R, cond)
//
// Also generate cond inversion if select operands are swapped)
//
static bool mergeToWrRegion(SelectInst *SI) {
  using namespace GenXIntrinsic::GenXRegion;

  CallInst *Rd = nullptr;
  bool Inverted = false;

  for (Value *Val : {SI->getTrueValue(), SI->getFalseValue()}) {
    if (!GenXIntrinsic::isRdRegion(Val))
      continue;

    Rd = cast<CallInst>(Val);
    Inverted = Val == SI->getTrueValue();
    Region RdReg = makeRegionFromBaleInfo(Rd, BaleInfo());

    auto CanMergeToWrRegion = [&](const Use &U) -> bool {
      if (!GenXIntrinsic::isWrRegion(U.getUser()))
        return false;
      if (U.getOperandNo() != NewValueOperandNum)
        return false;
      CallInst *Wr = cast<CallInst>(U.getUser());
      Region WrReg = makeRegionFromBaleInfo(Wr, BaleInfo());
      if (WrReg.Mask) {
        // If wrregion already has mask, it should be all ones constant.
        auto *C = dyn_cast<Constant>(WrReg.Mask);
        if (!C)
          return false;
        if (!C->isAllOnesValue())
          return false;
      }
      WrReg.Mask = nullptr;
      if (WrReg != RdReg)
        return false;

      Value *WrOldValOp = Wr->getOperand(OldValueOperandNum);
      Value *RdOldValOp = Rd->getOperand(OldValueOperandNum);

      bool SameOrigin = (WrOldValOp == RdOldValOp);
      bool CanReplaceUndef = isa<UndefValue>(WrOldValOp) &&
                             (WrOldValOp->getType() == RdOldValOp->getType());
      return SameOrigin || CanReplaceUndef;
    };

    auto DoMergeToWrRegion = [&](User *U) {
      IGC_ASSERT(isa<CallInst>(U));
      CallInst *Wr = cast<CallInst>(U);
      Value *Mask = SI->getCondition();
      // Invert mask if needed.
      if (Inverted)
        Mask = llvm::genx::invertCondition(Mask);
      // Create new wrregion.
      Region WrReg = makeRegionFromBaleInfo(Wr, BaleInfo());
      WrReg.Mask = Mask;
      Value *NewWr = WrReg.createWrRegion(Rd->getOperand(OldValueOperandNum),
                                          Inverted ? SI->getFalseValue()
                                                   : SI->getTrueValue(),
                                          Wr->getName(), Wr, Wr->getDebugLoc());
      BasicBlock::iterator WrIt(Wr);
      ReplaceInstWithValue(Wr->getParent()->getInstList(), WrIt, NewWr);
    };

    if (std::all_of(SI->use_begin(), SI->use_end(), CanMergeToWrRegion)) {
      for (auto I = SI->user_begin(), E = SI->user_end(); I != E;)
        DoMergeToWrRegion(*I++);
      return true;
    }
  }
  return false;
}

// Process peephole transformations on select.
bool GenXPatternMatch::simplifySelect(Function *F) {
  bool Changed = false;
  for (auto &BB : *F) {
    for (auto BI = BB.begin(), BE = BB.end(); BI != BE; /*empty*/) {
      SelectInst *SI = dyn_cast<SelectInst>(&*BI++);
      if (!SI || !SI->getType()->isVectorTy() ||
          !SI->getCondition()->getType()->isVectorTy())
        continue;
      if (canonizeSelect(SI) || mergeToWrRegion(SI)) {
        IGC_ASSERT_MESSAGE(SI->use_empty(),
                           "uses of transformed select aren't empty");
        Changed = true;
        BI = BasicBlock::iterator(SI->getNextNode());
        SI->eraseFromParent();
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
  }
  return Changed;
}

// a helper routine for decomposeSdivPow2
// return a new ConstantVector with the same type as input vector, that consists
// of log2 of original vector;
// input vector consists of only positive integer or only one positive integer
static Constant *getFloorLog2(const Constant *C) {
  IGC_ASSERT_MESSAGE(C, "getFloorLog2 get nullptr");
  IGC_ASSERT_MESSAGE(C->getType()->isIntOrIntVectorTy(),
                     "Error: getFloorLog2 get not int or vector of int type");
  if (C->getType()->isVectorTy()) {
    VectorType *Ty = cast<VectorType>(C->getType());
    SmallVector<Constant *, 4> Elts;
    const ConstantDataVector *Input = cast<ConstantDataVector>(C);
    Elts.reserve(Input->getNumElements());
    for (int V = 0; V != Input->getNumElements(); ++V) {
      APInt Elt = Input->getElementAsAPInt(V);
      Constant *Log2 = ConstantInt::get(Ty->getScalarType(), Elt.logBase2());
      Elts.push_back(Log2);
    }
    return ConstantVector::get(Elts);
  }
  Type *Ty = C->getType();
  const ConstantInt *Elt = cast<ConstantInt>(C);
  const APInt Val = Elt->getValue();
  return ConstantInt::get(Ty->getScalarType(), Val.logBase2());
}

enum class DivRemOptimize {
  // No optimization can be applied.
  Not,
  // Power of 2 case optimization.
  Pow2,
};

// Check if unsigned UDiv/URem can be optimized,
// based on its divisor \p Operand.
static DivRemOptimize isSuitableUDivURemOperand(Value *Operand) {
  IGC_ASSERT(Operand);
  // Constant data vector or constant.
  if (!isa<Constant>(Operand))
    return DivRemOptimize::Not;
  Type *OperandTy = Operand->getType();
  // TODO support i8, i16 & i64 cases
  // for pow2 case just turning on the same pattern as i32 width
  // Not int and not vector of int, or width wrong( not 32).
  if (!OperandTy->isIntOrIntVectorTy(genx::DWordBits))
    return DivRemOptimize::Not;
  // TODO: Remove this as we have tests for this pattern.
  if (PatternMatch::match(Operand, PatternMatch::m_Negative()))
    return DivRemOptimize::Not;
  if (PatternMatch::match(Operand, PatternMatch::m_Power2()))
    return DivRemOptimize::Pow2;
  return DivRemOptimize::Not;
}

// Check if unsigned SDiv/URem can be optimized,
// based on its divisor \p Operand.
static DivRemOptimize isSuitableSDivSRemOperand(Value *Operand) {
  IGC_ASSERT(Operand);
  // Constant data vector or constant.
  if (!isa<Constant>(Operand))
    return DivRemOptimize::Not;
  Type *OperandTy = Operand->getType();
  // TODO support i8, i16 & i64 cases
  // i8, i16 - by creating zext/sext to i32
  // i64 - just turning on the same pattern as i32 width,
  //  as emulation for shift and add much faster than emulation division.
  // Not int and not vector of int, or width wrong( not 32).
  if (!OperandTy->isIntOrIntVectorTy(genx::DWordBits))
    return DivRemOptimize::Not;
  if (PatternMatch::match(Operand, PatternMatch::m_Negative()))
    return DivRemOptimize::Not;
  if (!PatternMatch::match(Operand, PatternMatch::m_Power2()))
    return DivRemOptimize::Not;
  return DivRemOptimize::Pow2;
}

// Optimization for signed x / 2^p.
// if 2^p positite value
// intWidth = 32
// x / y = ashr( x + lshr( ashr(x, intWidth - 1), intWidth - log2(y)), log2(y))
static void decomposeSDivPow2(BinaryOperator &SDivOp) {
  IGC_ASSERT(SDivOp.getOpcode() == Instruction::SDiv);
  Value *Dividend = SDivOp.getOperand(0);
  IGC_ASSERT(isSuitableSDivSRemOperand(SDivOp.getOperand(1)) ==
             DivRemOptimize::Pow2);
  Constant *Divisor = cast<Constant>(SDivOp.getOperand(1));

  IRBuilder<> Builder{&SDivOp};

  const char *Name = "genxSdivOpt";
  Type *OperationTy = Dividend->getType();
  Type *ElementTy = OperationTy->getScalarType();
  IGC_ASSERT(ElementTy);
  unsigned ElementBitWidth = ElementTy->getIntegerBitWidth();

  Constant *VecSignBit =
      Constant::getIntegerValue(OperationTy, APInt{32, ElementBitWidth - 1});
  Constant *VecBitWidth =
      Constant::getIntegerValue(OperationTy, APInt{32, ElementBitWidth});

  Constant *Log2Divisor = getFloorLog2(Divisor);
  IGC_ASSERT(Log2Divisor != nullptr);

  Value *ShiftSize = Builder.CreateSub(VecBitWidth, Log2Divisor, Name);
  // if op0 is negative, Signdetect all ones, else all zeros
  Value *SignDetect = Builder.CreateAShr(Dividend, VecSignBit, Name);
  Value *Addition = Builder.CreateLShr(SignDetect, ShiftSize, Name);
  Value *NewRhs = Builder.CreateAdd(Dividend, Addition, Name);
  Value *Res = Builder.CreateAShr(NewRhs, Log2Divisor);
  SDivOp.replaceAllUsesWith(Res);
  Res->takeName(&SDivOp);
}

void GenXPatternMatch::visitSDiv(BinaryOperator &I) {
  auto CheckRes = isSuitableSDivSRemOperand(I.getOperand(1));
  if (CheckRes == DivRemOptimize::Not)
    return;
  IGC_ASSERT(CheckRes == DivRemOptimize::Pow2);
  decomposeSDivPow2(I);
  Changed = true;
}

// Optimization for unsigned x / 2^p.
// p = log2(2^p)
// x / 2 ^ p = x >> p (lshr)
static void decomposeUDivPow2(BinaryOperator &UDivOp) {
  IGC_ASSERT(UDivOp.getOpcode() == Instruction::UDiv);
  Value *Dividend = UDivOp.getOperand(0);
  IGC_ASSERT(isSuitableUDivURemOperand(UDivOp.getOperand(1)) ==
             DivRemOptimize::Pow2);
  Constant *Divisor = cast<Constant>(UDivOp.getOperand(1));
  IRBuilder<> Builder{&UDivOp};
  Constant *Log2Divisor = getFloorLog2(Divisor);
  IGC_ASSERT(Log2Divisor);
  Value *Res = Builder.CreateLShr(Dividend, Log2Divisor);
  UDivOp.replaceAllUsesWith(Res);
  Res->takeName(&UDivOp);
}

void GenXPatternMatch::visitUDiv(BinaryOperator &I) {
  auto CheckRes = isSuitableUDivURemOperand(I.getOperand(1));
  if (CheckRes == DivRemOptimize::Not)
    return;
  IGC_ASSERT(CheckRes == DivRemOptimize::Pow2);
  Changed = true;
  return decomposeUDivPow2(I);
}

// Optimization for signed x % 2^p.
// 2^p is positive value
// x % y = x - y * (x / y)
static void decomposeSRemPow2(BinaryOperator &SRemOp) {
  IGC_ASSERT(SRemOp.getOpcode() == Instruction::SRem);
  Value *Dividend = SRemOp.getOperand(0);
  IGC_ASSERT(isSuitableSDivSRemOperand(SRemOp.getOperand(1)) ==
             DivRemOptimize::Pow2);
  Constant *Divisor = cast<Constant>(SRemOp.getOperand(1));

  IRBuilder<> Builder{&SRemOp};

  const char *Name = "genxSremOpt";
  Value *Sdiv = Builder.CreateSDiv(Dividend, Divisor, Name);
  Value *MulRes = Builder.CreateMul(Sdiv, Divisor, Name);
  Value *Res = Builder.CreateSub(Dividend, MulRes);

  decomposeSDivPow2(*cast<BinaryOperator>(Sdiv));

  SRemOp.replaceAllUsesWith(Res);
  Res->takeName(&SRemOp);
}

void GenXPatternMatch::visitSRem(BinaryOperator &I) {
  auto CheckRes = isSuitableSDivSRemOperand(I.getOperand(1));
  if (CheckRes == DivRemOptimize::Not)
    return;
  IGC_ASSERT(CheckRes == DivRemOptimize::Pow2);
  decomposeSRemPow2(I);
  Changed = true;
}

// Optimization for unsigned x % 2^p.
// p = log2(2^p)
// x % 2^p = (x & ((1<<p)-1)) = x & (2^p - 1)
static void decomposeURemPow2(BinaryOperator &URemOp) {
  IGC_ASSERT(URemOp.getOpcode() == Instruction::URem);
  Value *Dividend = URemOp.getOperand(0);
  IGC_ASSERT(isSuitableUDivURemOperand(URemOp.getOperand(1)) ==
             DivRemOptimize::Pow2);
  Constant *Divisor = cast<Constant>(URemOp.getOperand(1));
  Type *OperationTy = Dividend->getType();

  IRBuilder<> Builder{&URemOp};

  Constant *One = Constant::getIntegerValue(OperationTy, APInt{32, 1});
  Value *Res = Builder.CreateAnd(Dividend, Builder.CreateSub(Divisor, One));
  URemOp.replaceAllUsesWith(Res);
  Res->takeName(&URemOp);
}

void GenXPatternMatch::visitURem(BinaryOperator &I) {
  auto CheckRes = isSuitableUDivURemOperand(I.getOperand(1));
  if (CheckRes == DivRemOptimize::Not)
    return;
  IGC_ASSERT(CheckRes == DivRemOptimize::Pow2);
  Changed = true;
  return decomposeURemPow2(I);
}

#if LLVM_VERSION_MAJOR >= 10
// Quick fix of IGC LLVM 11 based compilation failures.
void GenXPatternMatch::visitFreezeInst(FreezeInst &I) {
  Value *Op = I.getOperand(0);
  I.replaceAllUsesWith(Op);
  Changed = true;
}
#endif

// Decompose predicate operand for large vector selects.
bool GenXPatternMatch::decomposeSelect(Function *F) {
  const GenXSubtarget *ST = &getAnalysis<TargetPassConfig>()
                                 .getTM<GenXTargetMachine>()
                                 .getGenXSubtarget();
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

      IGC_ASSERT_MESSAGE(!Ops.empty(), "There's no operands collected!");

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
  auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(C->getType());
  if (!VT || VT->getNumElements() <= Width || VT->getScalarSizeInBits() == 1)
    return false;
  unsigned NElts = VT->getNumElements();
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
        if (!Ty->isVectorTy() ||
            cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements() < 16 ||
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
          unsigned Width = cast<IGCLLVM::FixedVectorType>(ShtAmt[0]->getType())
                               ->getNumElements();
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
  IGC_ASSERT(!C->getType()->getScalarType()->isIntegerTy(1));
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
        for (unsigned i = 0,
                      e = cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements();
             i != e; ++i) {
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
        IGC_ASSERT(ConstantUses.size() >= 2);
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
        IGC_ASSERT(!isa<PHINode>(InsertBefore));
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
  unsigned NumElts = cast<IGCLLVM::FixedVectorType>(InstTy)->getNumElements();

  // Cannot bitcast <N x iM> to <N/(32/M) x i32>
  if (NumElts % Scale != 0)
    return false;
  NumElts /= Scale;

  Type *NewTy = IGCLLVM::FixedVectorType::get(I32Ty, NumElts);
  IRBuilder<TargetFolder> Builder(BO->getParent(), BasicBlock::iterator(BO),
                                  TargetFolder(*DL));
  StringRef Name = BO->getName();

  Value *Op0 =
      Builder.CreateBitCast(BO->getOperand(0), NewTy, Name + ".extend.mask.op");
  Value *Op1 =
      Builder.CreateBitCast(BO->getOperand(1), NewTy, Name + ".extend.mask.op");

  Instruction *NewInst = vc::cloneInstWithNewOps(*BO, {Op0, Op1});
  IGC_ASSERT(NewInst);
  NewInst->insertBefore(BO);
  NewInst->takeName(BO);

  Value *Inst = Builder.CreateBitCast(NewInst, InstTy, Name + ".extend.mask.trunc");
  BO->replaceAllUsesWith(Inst);

  return true;
}
