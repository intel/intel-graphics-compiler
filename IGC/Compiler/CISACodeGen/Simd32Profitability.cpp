/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/Simd32Profitability.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/Platform.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvmWrapper/IR/ConstantFold.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include <llvmWrapper/Transforms/Utils/LoopUtils.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include <llvm/Support/CommandLine.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "simd32-profit"
#define PASS_DESCRIPTION "Check SIMD32 Profitability for OpenCL"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(Simd32ProfitabilityAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(Simd32ProfitabilityAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

static cl::opt<bool>
    enableProfitabilityPrint("enable-profitability-print", cl::init(false), cl::Hidden,
                             cl::desc("Enable m_isSimd32Profitable/m_isSimd16Profitable fields print"));

char Simd32ProfitabilityAnalysis::ID = 0;

const unsigned BRANCHY_MINPATH = 8;

Simd32ProfitabilityAnalysis::Simd32ProfitabilityAnalysis()
    : FunctionPass(ID), F(nullptr), PDT(nullptr), LI(nullptr), pMdUtils(nullptr), WI(nullptr),
      m_isSimd32Profitable(true), m_isSimd16Profitable(true) {
  initializeSimd32ProfitabilityAnalysisPass(*PassRegistry::getPassRegistry());
}

static std::tuple<Value * /*INIT*/, Value * /*CURR*/, Value * /*STEP*/, Value * /*NEXT*/>
getInductionVariable(Loop *L) {
  BasicBlock *H = L->getHeader();

  BasicBlock *Incoming = 0, *Backedge = 0;
  pred_iterator PI = pred_begin(H);
  IGC_ASSERT_MESSAGE(PI != pred_end(H), "Loop must have at least one backedge!");
  Backedge = *PI++;
  if (PI == pred_end(H)) // dead loop
    return std::make_tuple(nullptr, nullptr, nullptr, nullptr);
  Incoming = *PI++;
  if (PI != pred_end(H)) // multiple backedges?
    return std::make_tuple(nullptr, nullptr, nullptr, nullptr);

  if (L->contains(Incoming)) {
    if (L->contains(Backedge))
      return std::make_tuple(nullptr, nullptr, nullptr, nullptr);
    std::swap(Incoming, Backedge);
  } else if (!L->contains(Backedge))
    return std::make_tuple(nullptr, nullptr, nullptr, nullptr);

  // Loop over all of the PHI nodes, looking for an indvar.
  for (auto I = H->begin(); isa<PHINode>(I); ++I) {
    PHINode *PN = cast<PHINode>(I);
    if (auto Inc = dyn_cast<Instruction>(PN->getIncomingValueForBlock(Backedge))) {
      if (Inc->getOpcode() == Instruction::Add && Inc->getOperand(0) == PN) {
        return std::make_tuple(PN->getIncomingValueForBlock(Incoming), PN, Inc->getOperand(1), Inc);
      }
    }
  }

  return std::make_tuple(nullptr, nullptr, nullptr, nullptr);
}

enum { LOOPCOUNT_LIKELY_SMALL, LOOPCOUNT_LIKELY_LARGE, LOOPCOUNT_UNKNOWN };

static bool isSignedPredicate(CmpInst::Predicate Pred) {
  switch (Pred) {
  default:
    break;
  case CmpInst::ICMP_EQ:
  case CmpInst::ICMP_NE:
  case CmpInst::ICMP_SGT:
  case CmpInst::ICMP_SLT:
  case CmpInst::ICMP_SGE:
  case CmpInst::ICMP_SLE:
    return true;
  }
  return false;
}

static bool isUnsignedPredicate(CmpInst::Predicate Pred) {
  switch (Pred) {
  default:
    break;
  case CmpInst::ICMP_EQ:
  case CmpInst::ICMP_NE:
  case CmpInst::ICMP_UGT:
  case CmpInst::ICMP_ULT:
  case CmpInst::ICMP_UGE:
  case CmpInst::ICMP_ULE:
    return true;
  }
  return false;
}

static bool hasSameSignedness(CmpInst::Predicate LHS, CmpInst::Predicate RHS) {
  if (isSignedPredicate(LHS) && isSignedPredicate(RHS))
    return true;
  if (isUnsignedPredicate(LHS) && isUnsignedPredicate(RHS))
    return true;
  return false;
}

static std::tuple<Value *, Value *, Value *, bool> isOutOfRangeComparison(Value *Cond) {
  BinaryOperator *BO = dyn_cast<BinaryOperator>(Cond);
  if (!BO || BO->getOpcode() != Instruction::Or)
    return std::make_tuple(nullptr, nullptr, nullptr, false);

  ICmpInst *LHS = dyn_cast<ICmpInst>(BO->getOperand(0));
  ICmpInst *RHS = dyn_cast<ICmpInst>(BO->getOperand(1));

  if (!LHS || !RHS)
    return std::make_tuple(nullptr, nullptr, nullptr, false);

  CmpInst::Predicate P0 = LHS->getPredicate();
  CmpInst::Predicate P1 = RHS->getPredicate();

  if (!hasSameSignedness(P0, P1))
    return std::make_tuple(nullptr, nullptr, nullptr, false);

  // Simplify the checking since they have the same signedness.
  P0 = ICmpInst::getSignedPredicate(P0);
  P1 = ICmpInst::getSignedPredicate(P1);

  if (!(P0 == CmpInst::ICMP_SLT || P0 == CmpInst::ICMP_SLE)) {
    std::swap(LHS, RHS);
    std::swap(P0, P1);
  }
  if (!(P0 == CmpInst::ICMP_SLT || P0 == CmpInst::ICMP_SLE) || !(P1 == CmpInst::ICMP_SGT || P1 == CmpInst::ICMP_SGE))
    return std::make_tuple(nullptr, nullptr, nullptr, false);

  if (LHS->getOperand(0) != RHS->getOperand(0))
    return std::make_tuple(nullptr, nullptr, nullptr, false);

  return std::make_tuple(LHS->getOperand(0), LHS->getOperand(1), RHS->getOperand(1),
                         isSignedPredicate(LHS->getPredicate()));
}

static Value *getLoopCounter(Loop *L, Value *X) {
  BasicBlock *H = L->getHeader();

  BasicBlock *Incoming = 0, *Backedge = 0;
  pred_iterator PI = pred_begin(H);
  IGC_ASSERT_MESSAGE(PI != pred_end(H), "Loop must have at least one backedge!");
  Backedge = *PI++;
  if (PI == pred_end(H)) // dead loop
    return nullptr;
  Incoming = *PI++;
  if (PI != pred_end(H)) // multiple backedges?
    return nullptr;

  if (L->contains(Incoming)) {
    if (L->contains(Backedge))
      return nullptr;
    std::swap(Incoming, Backedge);
  } else if (!L->contains(Backedge))
    return nullptr;

  for (auto I = H->begin(); isa<PHINode>(I); ++I) {
    PHINode *PN = cast<PHINode>(I);
    if (X == PN->getIncomingValueForBlock(Backedge))
      return PN;
  }

  return nullptr;
}

static std::tuple<int, int> countOperands(Value *V, Value *LHS, Value *RHS) {
  if (V == LHS || V == RHS)
    return std::make_tuple((V == LHS), (V == RHS));

  // Count LHS, RHS in an expression like m*L + n*R +/- C, where C is
  // constant.
  BinaryOperator *BO = dyn_cast<BinaryOperator>(V);
  if (!BO || (BO->getOpcode() != Instruction::Add && BO->getOpcode() != Instruction::Sub &&
              BO->getOpcode() != Instruction::Shl && BO->getOpcode() != Instruction::Xor))
    return std::make_tuple(0, 0);

  if (BO->getOpcode() == Instruction::Shl) {
    ConstantInt *CI = dyn_cast<ConstantInt>(BO->getOperand(1));
    if (!CI)
      return std::make_tuple(0, 0);
    auto [L, R] = countOperands(BO->getOperand(0), LHS, RHS);
    uint64_t ShAmt = CI->getZExtValue();
    return std::make_tuple((L << ShAmt), (R << ShAmt));
  }

  if (BO->getOpcode() == Instruction::Xor) {
    ConstantInt *CI = dyn_cast<ConstantInt>(BO->getOperand(1));
    if (!CI || CI->getSExtValue() != -1)
      return std::make_tuple(0, 0);
    auto [L, R] = countOperands(BO->getOperand(0), LHS, RHS);
    return std::make_tuple(-L, -R);
  }

  IGC_ASSERT((BO->getOpcode() == Instruction::Add) || (BO->getOpcode() == Instruction::Sub));

  if (isa<Constant>(BO->getOperand(1)))
    return countOperands(BO->getOperand(0), LHS, RHS);
  auto [L0, L1] = countOperands(BO->getOperand(0), LHS, RHS);
  auto [R0, R1] = countOperands(BO->getOperand(1), LHS, RHS);
  if (BO->getOpcode() == Instruction::Add)
    return std::make_tuple(L0 + R0, L1 + R1);

  IGC_ASSERT(BO->getOpcode() == Instruction::Sub);
  return std::make_tuple(L0 - R0, L1 - R1);
}

static bool isNegatedByLB(Value *V, Value *X, Value *LB) {
  // Check if `V` is calculated as LB - X +/- C, where C is constant.
  auto [L, R] = countOperands(V, LB, X);
  return (L == 1) && (R == -1);
}

static bool isNegatedBy2UB(Value *V, Value *X, Value *UB) {
  // Check if `V` is calculated as 2UB - X +/- C, where C is constant.
  auto [L, R] = countOperands(V, UB, X);
  return (L == 2) && (R == -1);
}

unsigned Simd32ProfitabilityAnalysis::estimateLoopCount_CASE1(Loop *L) {
  BasicBlock *Exit = L->getExitingBlock();
  if (!Exit)
    return LOOPCOUNT_UNKNOWN;

  BranchInst *Br = dyn_cast<BranchInst>(Exit->getTerminator());
  if (!Br || !Br->isConditional())
    return LOOPCOUNT_UNKNOWN;
  if (!L->contains(Br->getSuccessor(0)))
    return LOOPCOUNT_UNKNOWN;

  auto [X, LB, UB, Signed] = isOutOfRangeComparison(Br->getCondition());
  if (!X) {
    ICmpInst *Cmp = dyn_cast<ICmpInst>(Br->getCondition());
    if (!Cmp)
      return LOOPCOUNT_UNKNOWN;
    switch (Cmp->getPredicate()) {
    default:
      return LOOPCOUNT_UNKNOWN;
    case CmpInst::ICMP_UGT:
    case CmpInst::ICMP_UGE:
      // A smart use of unsigned comparison on signed values to perform a
      // out-of-range change of (0, N).
      break;
    }
    X = Cmp->getOperand(0);
    LB = Constant::getNullValue(X->getType());
    UB = Cmp->getOperand(1);
    Signed = true;
  }

  Value *LC = getLoopCounter(L, X);
  if (!LC)
    return LOOPCOUNT_UNKNOWN;

  if (PHINode *PN = dyn_cast<PHINode>(X)) {
    if (PN->getNumIncomingValues() != 2)
      return LOOPCOUNT_UNKNOWN;
    BasicBlock *BB0 = PN->getIncomingBlock(0);
    BasicBlock *IfBB = BB0->getSinglePredecessor();
    if (!IfBB)
      return LOOPCOUNT_UNKNOWN;
    Br = dyn_cast<BranchInst>(IfBB->getTerminator());
    if (!Br || !Br->isConditional())
      return LOOPCOUNT_UNKNOWN;
    ICmpInst *Cmp = dyn_cast<ICmpInst>(Br->getCondition());
    if (!Cmp)
      return LOOPCOUNT_UNKNOWN;
    CmpInst::Predicate Pred = Cmp->getPredicate();
    Value *LHS = Cmp->getOperand(0);
    Value *RHS = Cmp->getOperand(1);
    if (LHS != LC) {
      std::swap(LHS, RHS);
      Pred = CmpInst::getSwappedPredicate(Pred);
    }
    if (LHS != LC)
      return LOOPCOUNT_UNKNOWN;
    if (!Signed)
      Pred = ICmpInst::getSignedPredicate(Pred);
    if (Pred != CmpInst::ICMP_SLT && Pred != CmpInst::ICMP_SLE)
      return LOOPCOUNT_UNKNOWN;
    if (RHS != LB)
      return LOOPCOUNT_UNKNOWN;

    Value *X0 = PN->getIncomingValue(0);
    Value *X1 = PN->getIncomingValue(1);
    if (!isNegatedByLB(X0, LC, LB))
      return LOOPCOUNT_UNKNOWN;
    if (!isNegatedBy2UB(X1, LC, UB))
      return LOOPCOUNT_UNKNOWN;
  } else if (BinaryOperator *BO = dyn_cast<BinaryOperator>(X)) {
    if (BO->getOpcode() != Instruction::Sub)
      return LOOPCOUNT_UNKNOWN;
    if (BO->getOperand(1) != LC)
      return LOOPCOUNT_UNKNOWN;
    SelectInst *SI = dyn_cast<SelectInst>(BO->getOperand(0));
    if (!SI)
      return LOOPCOUNT_UNKNOWN;
    ICmpInst *Cmp = dyn_cast<ICmpInst>(SI->getCondition());
    if (!Cmp)
      return LOOPCOUNT_UNKNOWN;
    CmpInst::Predicate Pred = Cmp->getPredicate();
    Value *LHS = Cmp->getOperand(0);
    Value *RHS = Cmp->getOperand(1);
    if (LHS != LC) {
      std::swap(LHS, RHS);
      Pred = CmpInst::getSwappedPredicate(Pred);
    }
    if (LHS != LC)
      return LOOPCOUNT_UNKNOWN;
    if (!Signed)
      Pred = ICmpInst::getSignedPredicate(Pred);
    Value *X0 = SI->getTrueValue();
    Value *X1 = SI->getFalseValue();
    if (Pred == CmpInst::ICMP_SGT || Pred == CmpInst::ICMP_SGE) {
      std::swap(X0, X1);
      Pred = CmpInst::getInversePredicate(Pred);
    }
    if (Pred != CmpInst::ICMP_SLT && Pred != CmpInst::ICMP_SLE)
      return LOOPCOUNT_UNKNOWN;
    if (RHS != LB)
      return LOOPCOUNT_UNKNOWN;
    auto [L0, R0] = countOperands(X0, LB, nullptr);
    auto [L1, R1] = countOperands(X1, UB, nullptr);
    if (L0 != 1 || L1 != 2)
      return LOOPCOUNT_UNKNOWN;
  } else
    return LOOPCOUNT_UNKNOWN;

  // Ok, we found a loop of the following pattern:
  //
  // do {
  //   if (x < 0) {
  //      x = 0 - x +/- c0;
  //   } else {
  //      x = 2 * UB - x +/- c1;
  //   }
  // } while (x < LB || x > UB);
  //
  // such loop will run only once or twice when non-arbitary large `x`. If a
  // non-uniform loop only runs several iterations, divergence cost due to
  // SIMD32 could be ignored.
  return LOOPCOUNT_LIKELY_SMALL;
}

unsigned Simd32ProfitabilityAnalysis::estimateLoopCount_CASE2(Loop *L) {
  SmallVector<BasicBlock *, 8> ExitingBBs;
  L->getExitingBlocks(ExitingBBs);

  auto [Init, Curr, Step, Next] = getInductionVariable(L);
  if (!Init || !Curr || !Step || !Next)
    return LOOPCOUNT_UNKNOWN;
  ConstantInt *I0 = dyn_cast<ConstantInt>(Init);
  ConstantInt *S0 = dyn_cast<ConstantInt>(Step);
  if (!I0 || !S0)
    return LOOPCOUNT_UNKNOWN;

  for (auto BB : ExitingBBs) {
    BranchInst *Br = dyn_cast<BranchInst>(BB->getTerminator());
    if (!Br || !Br->isConditional())
      continue;
    if (!L->contains(Br->getSuccessor(0))) // Not condition of `continue`.
      continue;
    ICmpInst *Cmp = dyn_cast<ICmpInst>(Br->getCondition());
    if (!WI->isUniform(Br)) {
      BinaryOperator *BO = dyn_cast<BinaryOperator>(Br->getCondition());
      if (!BO)
        continue;
      if (BO->getOpcode() != Instruction::And)
        continue;
      ICmpInst *Cond = nullptr;
      ICmpInst *Op0 = dyn_cast<ICmpInst>(BO->getOperand(0));
      if (Op0 && WI->isUniform(Op0))
        Cond = Op0;
      if (!Cond) {
        ICmpInst *Op1 = dyn_cast<ICmpInst>(BO->getOperand(1));
        if (Op1 && WI->isUniform(Op1))
          Cond = Op1;
      }
      if (!Cond)
        continue;
      Cmp = Cond;
    }
    if (!Cmp)
      continue;
    CmpInst::Predicate Pred = Cmp->getPredicate();
    switch (Pred) {
    default:
      // TODO: Handle more predicates.
      continue;
    case ICmpInst::ICMP_SLT:
    case ICmpInst::ICMP_ULT:
      break;
    }
    Value *Op0 = Cmp->getOperand(0);
    Value *Op1 = Cmp->getOperand(1);
    if (Op0 != Next)
      continue;
    ConstantInt *E0 = dyn_cast<ConstantInt>(Op1);
    if (!E0)
      continue;
    unsigned OpCode = Pred == ICmpInst::ICMP_SLT ? Instruction::SDiv : Instruction::UDiv;
    ConstantInt *N =
        dyn_cast<ConstantInt>(IGCLLVM::ConstantFoldBinaryInstruction(OpCode, ConstantExpr::getSub(E0, I0), S0));
    if (!N)
      continue;
    if (N->getValue().slt(0))
      continue;
    if (N->getValue().slt(100))
      return LOOPCOUNT_LIKELY_SMALL;
  }

  // Ok, we found a non-uniform loop with multiple exiting conditions.
  // However, one of them is uniform one and has small loop count.
  return LOOPCOUNT_UNKNOWN;
}

unsigned Simd32ProfitabilityAnalysis::estimateLoopCount(Loop *L) {
  unsigned Ret;

  Ret = estimateLoopCount_CASE1(L);
  if (Ret != LOOPCOUNT_UNKNOWN)
    return Ret;

  Ret = estimateLoopCount_CASE2(L);
  if (Ret != LOOPCOUNT_UNKNOWN)
    return Ret;

  return Ret;
}

static Value *getLoopCount(Value *Start, Value *End) {
  // Poorman's loop count checking as we need to check that result with WIA.
  ConstantInt *CStart = dyn_cast<ConstantInt>(Start);
  ConstantInt *CEnd = dyn_cast<ConstantInt>(End);
  if (CStart && CEnd)
    return ConstantExpr::getSub(CEnd, CStart);

  if (CStart && CStart->isNullValue())
    return End;

  BinaryOperator *BO = dyn_cast<BinaryOperator>(End);
  if (!BO || BO->getOpcode() != Instruction::Add)
    return nullptr;

  Value *Op0 = BO->getOperand(0);
  Value *Op1 = BO->getOperand(1);
  if (Op0 != Start)
    std::swap(Op0, Op1);
  if (Op0 == Start)
    return Op1;

  return nullptr;
}

/// hasIEEESqrtOrDivFunc - Check whether IEEE correctly-rounded SQRT or DIV is
/// used in the given function.
static bool hasIEEESqrtOrDivFunc(const Function &F) {
  for (auto &BB : F)
    for (auto &I : BB) {
      const GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(&I);
      if (!GII)
        continue;
      switch (GII->getIntrinsicID()) {
      case GenISAIntrinsic::GenISA_IEEE_Sqrt:
      case GenISAIntrinsic::GenISA_IEEE_Divide:
        return true;
      default:
        break;
      }
    }
  return false;
}

/// hasSubGroupFunc - Check whether subgroup functions are used in the given
/// function.
static bool hasSubGroupFunc(const Function &F) {
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (isSubGroupIntrinsic(&I)) {
        return true;
      }
    }
  }

  return false;
}

bool Simd32ProfitabilityAnalysis::runOnFunction(Function &F) {
  this->F = &F;
  CodeGenContext *context = nullptr;
  context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  if (context->type == ShaderType::OPENCL_SHADER) {
    PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
    LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    WI = &getAnalysis<WIAnalysis>();
    pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    m_isSimd16Profitable = checkSimd16Profitable(context);
    m_isSimd32Profitable = m_isSimd16Profitable && checkSimd32Profitable(context);
  } else if (context->type == ShaderType::PIXEL_SHADER) {
    LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    m_isSimd32Profitable = checkPSSimd32Profitable();
  }

  if (enableProfitabilityPrint)
    print(IGC::Debug::ods());

  return false;
}

void Simd32ProfitabilityAnalysis::print(llvm::raw_ostream &OS) const {
  OS << "\nisSimd16Profitable: " << m_isSimd16Profitable;
  OS << "\nisSimd32Profitable: " << m_isSimd32Profitable << "\n\n";
}

bool Simd32ProfitabilityAnalysis::checkSimd32Profitable(CodeGenContext *ctx) {
  // If a kernel is too big, it would probably have enough work for EUs
  // even without simd32; and simd32 would have more visa variables than
  // 64K limit (ocl c99 64 bit PrintHalf/half8.c for example); thus make
  // sense to skip simd32.
  size_t programSizeLimit = 8000;
  size_t programSize = 0;
  for (Function::iterator FI = F->begin(), FE = F->end(); FI != FE; ++FI) {
    BasicBlock *BB = &*FI;
    programSize += BB->size();
  }
  if (programSize > programSizeLimit) {
    return false;
  }

  // If we have workgroup size (or workgroup size hint) metadata, check whether the X dimension
  // is expected to be of size 16 or below. If it is, no point in using SIMD32, we'll just
  // get empty lanes.
  auto funcInfoMD = pMdUtils->findFunctionsInfoItem(F);
  if (funcInfoMD != pMdUtils->end_FunctionsInfo()) {
    ThreadGroupSizeMetaDataHandle tgSize = funcInfoMD->second->getThreadGroupSize();
    ThreadGroupSizeMetaDataHandle tgSizeHint = funcInfoMD->second->getThreadGroupSizeHint();
    const int tgSizeLimit = 16;
    int tgSizeCal = tgSize->getXDim() * tgSize->getYDim() * tgSize->getZDim();
    int tgSizeHintCal = tgSizeHint->getXDim() * tgSizeHint->getYDim() * tgSizeHint->getZDim();

    if (ctx->getModuleMetaData()->csInfo.maxWorkGroupSize &&
        ctx->getModuleMetaData()->csInfo.maxWorkGroupSize <= tgSizeLimit)
      return false;

    if ((tgSize->hasValue() && tgSizeCal <= tgSizeLimit) || (tgSizeHint->hasValue() && tgSizeHintCal <= tgSizeLimit)) {
      return false;
    }
  }

  // WORKAROUND - Skip SIMD32 if subgroup functions are present.
  bool hasSubGrFunc = hasSubGroupFunc(*F);
  if (hasSubGrFunc) {
    return false;
  }

  const CPlatform *platform = &ctx->platform;
  switch (platform->GetPlatformFamily()) {
  case IGFX_GEN9_CORE:
    /* TODO: Try to apply for platform->getPlatformInfo().eProductFamily ==
     * IGFX_BROXTON only. */
    // FALL THROUGH
  case IGFX_GEN10_CORE:
    if (hasIEEESqrtOrDivFunc(*F)) {
      return false;
    }
    break;
  default:
    break;
  }
  // END OF WORKAROUND

  // Ok, that's not the case.
  // Now, check whether we have any non-uniform loops.
  // The idea is that if there are divergenet loops, then SIMD32 will be harmful,
  // because we'll waste time running loops with very few full lanes.
  // If there are no divergent loops, SIMD32 is worth a shot. It still may not
  // be selected, due to spills.
  for (LoopInfo::iterator li = LI->begin(), le = LI->end(); li != le; ++li) {
    llvm::Loop *loop = *li;

    SmallVector<BasicBlock *, 8> exitingBlocks;
    loop->getExitingBlocks(exitingBlocks);

    bool AllUniform = true;
    for (auto BBI = exitingBlocks.begin(), BBE = exitingBlocks.end(); BBI != BBE; ++BBI) {
      BasicBlock *block = *BBI;

      Instruction *term = block->getTerminator();
      if (!WI->isUniform(term)) {
        auto Br = dyn_cast<BranchInst>(term);
        // Check special case for non-uniform loop where, except the
        // initial, current, and next values, STEP and COUNT are
        // uniform. In such a case, the loop is only diverged at the
        // termination. It should be still profitable to be compiled
        // into SIMD32 mode.
        if (Br && Br->isConditional()) {
          auto ICmp = dyn_cast<ICmpInst>(Br->getCondition());
          if (ICmp) {
            auto [Init, Curr, Step, Next] = getInductionVariable(loop);
            if (Init && Curr && Next && Step && WI->isUniform(Step)) {
              auto Op0 = ICmp->getOperand(0);
              auto Op1 = ICmp->getOperand(1);
              if (SExtInst *SI0 = dyn_cast<SExtInst>(Op0))
                Op0 = SI0->getOperand(0);
              if (SExtInst *SI1 = dyn_cast<SExtInst>(Op1))
                Op1 = SI1->getOperand(0);
              if (Op0 != Next && Op0 != Curr)
                std::swap(Op0, Op1);
              // Skip non-uniform loop which only terminates on
              // comparison between non-uniform induction variable
              // and uniform value.
              if (Op0 == Next || Op0 == Curr) {
                // TODO: Need to check whether Init is linear to
                // global/local ID. However, that checking is not
                // that straightforward before code emitter.
                if (WI->isUniform(Op1))
                  continue;
                // TODO: Eable IndVarSimplify to simlify the
                // following check.
                if (Value *Count = getLoopCount(Init, Op1)) {
                  if (WI->isUniform(Count))
                    continue;
                }
              }
            }
          }
        }
        AllUniform = false;
        break;
      }
    }
    if (!AllUniform) {
      unsigned int estLoopCnt = estimateLoopCount(loop);
      switch (estLoopCnt) {
      case LOOPCOUNT_LIKELY_LARGE:
      case LOOPCOUNT_UNKNOWN:
        return false;
      case LOOPCOUNT_LIKELY_SMALL:
        break;
      }
    }
  }

  return true;
}

/// Cyclomatic complexity measures of the number of linearly independent paths
/// through a region.
///
/// M = a * E - N + 2 where
/// E = the number of edges of the graph
/// N = the number of nodes of the graph
/// a = scalar factor (1 for uniform branches).
///
/// We focus on loops instead of the entire program, since cyclomatic
/// complexity is roughly linear when concatenating two programs, i.e.
/// CC(F # G) = (E1 + E2 + 1) - (N1 + N2) + 2
///           = (E1 - N1 + 2) + (E2 - N2 + 2) - 1
///           = CC(F) + CC(G) - 1.
///
static const unsigned CYCLOMATIC_COMPLEXITY_THRESHOLD = 200;

unsigned Simd32ProfitabilityAnalysis::getLoopCyclomaticComplexity() {
  unsigned MaxCC = 0;
  for (LoopInfo::iterator I = LI->begin(), E = LI->end(); I != E; ++I) {
    Loop *L = *I;
    unsigned CC = 2;
    for (auto BI = L->block_begin(), BE = L->block_end(); BI != BE; ++BI) {
      BasicBlock *BB = *BI;
      IGCLLVM::TerminatorInst *TI = BB->getTerminator();
      bool IsUniform = WI->isUniform(TI);
      CC += TI->getNumSuccessors() * (IsUniform ? 1 : 2);
    }
    CC -= L->getNumBlocks();
    MaxCC = std::max(CC, MaxCC);
  }
  return MaxCC;
}

static unsigned getNumOfNonUniformExits(Loop *L, WIAnalysis *WI) {
  SmallVector<BasicBlock *, 8> ExistingBlocks;
  L->getExitingBlocks(ExistingBlocks);
  unsigned Count = 0;
  for (auto BB : ExistingBlocks) {
    IGCLLVM::TerminatorInst *TI = BB->getTerminator();
    bool IsUniform = WI->isUniform(TI);
    Count += !IsUniform;
  }

  return Count;
}

/// Check if a loop or its subloop has multiple non-uniform exists.
static bool hasMultipleExits(Loop *L, WIAnalysis *WI) {
  if (getNumOfNonUniformExits(L, WI) > 1)
    return true;
  for (auto InnerL : L->getSubLoops())
    if (hasMultipleExits(InnerL, WI))
      return true;
  return false;
}

/// Given a loop, return nested (inner) loops with multiple non-uniform exits.
/// E.g. assume L2, L3, L5, L7 are only loops with multiple non-uniform exists
/// L1
///    L2
///       L3
///    L4
///       L5
///          L6
///             L7
/// then it returns {L2, L5}
static void getNestedLoopsWithMultpleExists(Loop *L, WIAnalysis *WI, SmallVectorImpl<Loop *> &Result) {
  if (getNumOfNonUniformExits(L, WI) > 1) {
    for (auto InnerL : L->getSubLoops()) {
      if (hasMultipleExits(InnerL, WI)) {
        Result.push_back(L);
        return;
      }
    }
    // Only a single level, do not add into the result.
    return;
  }

  // Outer loop is normal. Check its inner loop structure, recursively.
  for (auto InnerL : L->getSubLoops())
    getNestedLoopsWithMultpleExists(InnerL, WI, Result);
}

static const float NestedLoopsWithMultipleExits_THRESHOLD = 0.7f;

/// Return the ratio between loops with multiple exists to other instructions in function.
static float NestedLoopsWithMultipleExitsRatio(Function *F, LoopInfo *LI, WIAnalysis *WI) {
  // Find top level nested loops with multiple non-uniform exists.
  SmallVector<Loop *, 8> Loops;
  for (LoopInfo::iterator I = LI->begin(), E = LI->end(); I != E; ++I) {
    Loop *L = *I;
    getNestedLoopsWithMultpleExists(L, WI, Loops);
  }

  // Sum the IR size of these loops.
  unsigned LoopSize = 0;
  for (auto L : Loops)
    for (auto BB : L->getBlocks())
      LoopSize += (unsigned)BB->size();

  // Check the ratio between nested loops with multiple exists and the total
  // number of instructions. A higher ratio means these loops dominate this
  // kernel.
  unsigned FuncSize = 0;
  for (auto &BB : *F)
    FuncSize += (unsigned)BB.size();

  if (FuncSize > 0) {
    return float(LoopSize) / FuncSize;
  } else {
    return 0.0f;
  }
}

static const unsigned LongStridedLdStInLoop_THRESHOLD = 3;

typedef struct _LdStInLoop {
  unsigned LDs = 0;
  unsigned STs = 0;
  llvm::Loop *pProblematicLoop = nullptr;
} LdStInLoop;

static const LdStInLoop LongStridedLdStInLoop(Function *F, LoopInfo *LI, WIAnalysis *WI) {
  LdStInLoop retVal;
  SmallVector<Loop *, 32> Loops;
  // Collect innermost simple loop.
  for (auto I = LI->begin(), E = LI->end(); I != E; ++I) {
    auto L = *I;
    if (!IGCLLVM::isInnermost(L))
      continue;
    if (L->getNumBlocks() != 2)
      continue;
    auto *Latch = L->getLoopLatch();
    if (!Latch || !Latch->front().isTerminator())
      continue;
    Loops.push_back(L);
  }
  unsigned LDs = 0;
  unsigned STs = 0;
  for (auto L : Loops) {
    auto BB = L->getHeader();
    for (auto I = BB->begin(), E = BB->end(); I != E; ++I) {
      if (auto LD = dyn_cast<LoadInst>(&*I)) {
        VectorType *VTy = dyn_cast<VectorType>(LD->getType());
        if (!VTy || IGCLLVM::GetVectorTypeBitWidth(VTy) <= 128)
          continue;
        if (WI->isUniform(LD))
          continue;
        ++LDs;
      }
      if (auto ST = dyn_cast<StoreInst>(&*I)) {
        Value *Ptr = ST->getPointerOperand();
        Value *Val = ST->getValueOperand();
        VectorType *VTy = dyn_cast<VectorType>(Val->getType());
        if (!VTy || IGCLLVM::GetVectorTypeBitWidth(VTy) <= 128)
          continue;
        if (WI->isUniform(Ptr))
          continue;
        ++STs;
      }
    }
    if (LDs > LongStridedLdStInLoop_THRESHOLD || STs > LongStridedLdStInLoop_THRESHOLD) {
      retVal.LDs = LDs;
      retVal.STs = STs;
      retVal.pProblematicLoop = L;
      return retVal;
    }
  }
  return retVal;
}

bool Simd32ProfitabilityAnalysis::checkSimd16Profitable(CodeGenContext *ctx) {
  if ((IGC_GET_FLAG_VALUE(OCLSIMD16SelectionMask) & 0x1)) {
    int loopCyclomaticComplexity = getLoopCyclomaticComplexity();

    if (loopCyclomaticComplexity >= CYCLOMATIC_COMPLEXITY_THRESHOLD) {
      return false;
    }
  }

  if (IGC_GET_FLAG_VALUE(OCLSIMD16SelectionMask) & 0x2) {
    float nestedLoopsWithMultipleExits = NestedLoopsWithMultipleExitsRatio(F, LI, WI);

    if (nestedLoopsWithMultipleExits >= NestedLoopsWithMultipleExits_THRESHOLD) {
      return false;
    }
  }

  // If there's wider vector load/store in a loop, skip SIMD16.
  if (IGC_GET_FLAG_VALUE(OCLSIMD16SelectionMask) & 0x4) {
    LdStInLoop ldStInLoop = LongStridedLdStInLoop(F, LI, WI);

    if (ldStInLoop.pProblematicLoop != nullptr) {
      return false;
    }
  }

  auto hasDouble = [](Function &F) {
    for (auto &BB : F)
      for (auto &I : BB) {
        if (I.getType()->isDoubleTy())
          return true;
        for (Value *V : I.operands())
          if (V->getType()->isDoubleTy())
            return true;
      }
    return false;
  };

  const CPlatform *platform = &ctx->platform;
  if (platform->GetPlatformFamily() == IGFX_GEN9_CORE &&
      platform->getPlatformInfo().eProductFamily == IGFX_GEMINILAKE && hasDouble(*F)) {
    return false;
  }

  return true;
}

bool Simd32ProfitabilityAnalysis::checkPSSimd32Profitable() {
  unsigned int numberInstructions = 0;
  unsigned int numberOfHalfInstructions = 0;
  unsigned int numberOfCmp = 0;
  unsigned int numberOfSample = 0;
  unsigned int numberOfBB = 0;
  BasicBlock *returnBlock = nullptr;
  bool hasDiscard = F->getParent()->getNamedMetadata("KillPixel") != nullptr;
  for (Function::iterator FI = F->begin(), FE = F->end(); FI != FE; ++FI) {
    for (auto II = FI->begin(), IE = FI->end(); II != IE; ++II) {
      if (II->getType() == Type::getHalfTy(F->getContext())) {
        numberOfHalfInstructions++;
      }
      if (isa<CmpInst>(*II)) {
        numberOfCmp++;
      }
      if (isSampleLoadGather4InfoInstruction(&(*II))) {
        numberOfSample++;
      }
      numberInstructions++;
    }
    if (isa<ReturnInst>(FI->getTerminator())) {
      returnBlock = &(*FI);
    }
    numberOfBB++;
  }
  if (numberInstructions > 4000 || numberInstructions == 0) {
    return false;
  }

  // Original SIMD32 heurtistic
  // if 1BB, short, has sample, no discard, no cmp, enable SIMD32
  // skip cmp to avoid flag spill
  if (!hasDiscard && numberOfCmp == 0 && numberOfSample > 0 && numberOfBB == 1 && numberInstructions < 80) {
    return true;
  }

  // disable SIMD32 for shader with multiple render target as it puts pressure on the render cache
  unsigned int numberRTWrite = 0;
  for (auto it = returnBlock->begin(), ie = returnBlock->end(); it != ie; ++it) {
    if (GenIntrinsicInst *intr = dyn_cast<GenIntrinsicInst>(it)) {
      if (llvm::isa<llvm::RTWriteIntrinsic>(intr)) {
        numberRTWrite++;
      }
    }
  }
  if (numberRTWrite > 1) {
    return false;
  }

  // Case where we expect to be bound by pixel dispatch time. For small shaderd without IO
  // It is better to go with SIMD32
  if (returnBlock == &F->getEntryBlock() && !hasDiscard) {
    bool hasIO = false;
    unsigned int numberInstructions = returnBlock->size();
    if (numberInstructions < 10) {
      for (auto II = returnBlock->begin(), IE = returnBlock->end(); II != IE; ++II) {
        if (II->mayReadOrWriteMemory() && !isa<RTWriteIntrinsic>(II)) {
          hasIO = true;
          break;
        }
        if (isa<SampleIntrinsic>(II) || isa<SamplerLoadIntrinsic>(II) || isa<InfoIntrinsic>(II) ||
            isa<SamplerGatherIntrinsic>(II)) {
          hasIO = true;
          break;
        }
      }
      if (!hasIO) {
        // for small program without IO using SIMD32 allows hiding the thread dispatch time
        return true;
      }
    }
  }

  if (IGC_IS_FLAG_ENABLED(PSSIMD32HeuristicFP16)) {
    // If we have a large ratio of half use SIMD32 to hide latency better
    float ratioHalf = (float)numberOfHalfInstructions / (float)numberInstructions;
    if (ratioHalf >= 0.5f) {
      return true;
    }
  }

  if (IGC_IS_FLAG_ENABLED(PSSIMD32HeuristicLoopAndDiscard)) {
    // If we have a discard and the first block is small we may be bound by PSD so we try to enable SIMD32
    if (hasDiscard) {
      BasicBlock &entryBB = F->getEntryBlock();
      if (!isa<ReturnInst>(entryBB.getTerminator()) && entryBB.size() < 50) {
        return true;
      }
    }

    // If we have a loop with high latency enable SIMD32 to reduce latency
    unsigned int numberOfInstructions = 0;
    unsigned int numberOfHighLatencyInst = 0;
    for (LoopInfo::iterator li = LI->begin(), le = LI->end(); li != le; ++li) {
      llvm::Loop *loop = *li;
      for (auto BI = loop->block_begin(), BE = loop->block_end(); BI != BE; ++BI) {
        for (auto II = (*BI)->begin(), IE = (*BI)->end(); II != IE; ++II) {
          if (isa<SampleIntrinsic>(II)) {
            numberOfHighLatencyInst++;
          }
          numberOfInstructions++;
        }
      }
    }
    if (numberOfInstructions < 85 && numberOfHighLatencyInst >= 1) {
      // high latency small loop
      return true;
    }
  }
  return false;
}
