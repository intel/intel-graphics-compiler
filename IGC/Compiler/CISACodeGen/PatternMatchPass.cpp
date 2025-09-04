/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/PatternMatchPass.hpp"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "Compiler/CISACodeGen/DeSSA.hpp"
#include "common/igc_regkeys.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/PatternMatch.h>
#include <llvmWrapper/IR/Instructions.h>
#include "llvmWrapper/Support/Alignment.h"
#include <llvm/IR/IntrinsicInst.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/InitializePasses.h"
#include "Compiler/DebugInfo/ScalarVISAModule.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

char CodeGenPatternMatch::ID = 0;
#define PASS_FLAG "CodeGenPatternMatch"
#define PASS_DESCRIPTION "Does pattern matching"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(CodeGenPatternMatch, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(LiveVarsAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(PositionDepAnalysis)
IGC_INITIALIZE_PASS_END(CodeGenPatternMatch, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC {
CodeGenPatternMatch::CodeGenPatternMatch()
    : FunctionPass(ID), m_rootIsSubspanUse(false), m_blocks(nullptr), m_numBlocks(0), m_root(nullptr),
      m_currentPattern(nullptr), m_Platform(), m_AllowContractions(true), m_NeedVMask(false),
      m_samplertoRenderTargetEnable(false), m_ctx(nullptr), DT(nullptr), PDT(nullptr), LI(nullptr), m_DL(0),
      m_WI(nullptr), m_LivenessInfo(nullptr), m_PosDep(nullptr) {
  initializeCodeGenPatternMatchPass(*PassRegistry::getPassRegistry());
}

CodeGenPatternMatch::~CodeGenPatternMatch() { delete[] m_blocks; }

void CodeGenPatternMatch::CodeGenNode(llvm::DomTreeNode *node) {
  struct NodeRange {
    NodeRange(llvm::DomTreeNode *node, llvm::DomTreeNode::iterator current, llvm::DomTreeNode::iterator end)
        : m_Node(node), m_Current(current), m_End(end) {}
    llvm::DomTreeNode *m_Node;
    llvm::DomTreeNode::iterator m_Current;
    llvm::DomTreeNode::iterator m_End;
  };
  std::list<NodeRange> m_NodeContainer;
  m_NodeContainer.push_front(NodeRange(node, node->begin(), node->end()));

  // Process blocks by processing the dominance tree depth first
  while (!m_NodeContainer.empty()) {
    NodeRange &currNodeRange = m_NodeContainer.front();
    if (currNodeRange.m_Current == currNodeRange.m_End) {
      llvm::BasicBlock *bb = currNodeRange.m_Node->getBlock();
      CodeGenBlock(bb);
      m_NodeContainer.pop_front();
      continue;
    }
    llvm::DomTreeNode *child = *currNodeRange.m_Current;
    m_NodeContainer.push_front(NodeRange(child, child->begin(), child->end()));
    currNodeRange.m_Current++;
  }
}

bool CodeGenPatternMatch::runOnFunction(llvm::Function &F) {
  m_blockMap.clear();
  ConstantPlacement.clear();
  PairOutputMap.clear();
  UniformBools.clear();

  delete[] m_blocks;
  m_blocks = nullptr;

  m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo()) {
    return false;
  }

  m_AllowContractions = true;
  if (m_ctx->m_DriverInfo.NeedCheckContractionAllowed()) {
    m_AllowContractions = modMD->compOpt.FastRelaxedMath || modMD->compOpt.MadEnable;
  }
  m_Platform = m_ctx->platform;

  DT = &getAnalysis<llvm::DominatorTreeWrapperPass>().getDomTree();
  PDT = &getAnalysis<llvm::PostDominatorTreeWrapperPass>().getPostDomTree();
  LI = &getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
  m_DL = &F.getParent()->getDataLayout();
  m_WI = &getAnalysis<WIAnalysis>();
  m_PosDep = &getAnalysis<PositionDepAnalysis>();
  // pattern match will update liveness held by LiveVar, which needs
  // WIAnalysis result for uniform variable
  m_LivenessInfo = &getAnalysis<LiveVarsAnalysis>().getLiveVars();
  CreateBasicBlocks(&F);
  CodeGenNode(DT->getRootNode());
  return false;
}

inline bool HasSideEffect(llvm::Instruction &inst) {
  if (inst.mayWriteToMemory() || inst.isTerminator()) {
    return true;
  }
  return false;
}

inline bool HasPhiUse(llvm::Value &inst) {
  for (auto UI = inst.user_begin(), E = inst.user_end(); UI != E; ++UI) {
    llvm::User *U = *UI;
    if (llvm::isa<llvm::PHINode>(U)) {
      return true;
    }
  }
  return false;
}

// If "v" is a canonicalize instruction, return its source argument. Return
// "v" otherwise.
inline Value *SkipCanonicalize(Value *v) {
  IntrinsicInst *intr = dyn_cast<IntrinsicInst>(v);
  if (intr && intr->getIntrinsicID() == Intrinsic::canonicalize) {
    return intr->getOperand(0);
  }
  return v;
}

bool CodeGenPatternMatch::IsDbgInst(llvm::Instruction &inst) const {
  if (llvm::isa<llvm::DbgInfoIntrinsic>(&inst)) {
    // FIXME: We probably don't need that.
    return true;
  }
  // perThreadOffset is special variable we should be alive for O0 runs.
  // This is intended for debug, but we do it for non-debug too to
  // avoid altering generated code by adding -g.
  if (m_ctx->getModuleMetaData()->compOpt.OptDisable && inst.getMetadata("perThreadOffset")) {
    return true;
  }
  return false;
}

bool CodeGenPatternMatch::IsConstOrSimdConstExpr(Value *C) {
  if (isa<ConstantInt>(C)) {
    return true;
  }
  if (Instruction *inst = dyn_cast<Instruction>(C)) {
    return SIMDConstExpr(inst);
  }
  return false;
}

// Checks if denorms are flushed on instruction's output.
// When denorm mode in CR0 is set to flush to zero, denorms are flushed on
// input and output of any floating-point mathematical operation with the
// following exceptions:
// - raw mov retains denorms
// - conversion instructions retain denorms (includes mix-mode instructions)
// - extended math instructions retain half float denorms
bool CodeGenPatternMatch::FlushesDenormsOnOutput(Instruction &I) {
  bool flushesDenorms = false;
  if ((m_ctx->getModuleMetaData()->compOpt.FloatDenormMode16 == IGC::FLOAT_DENORM_FLUSH_TO_ZERO &&
       I.getType()->isHalfTy()) ||
      (m_ctx->getModuleMetaData()->compOpt.FloatDenormMode32 == IGC::FLOAT_DENORM_FLUSH_TO_ZERO &&
       I.getType()->isFloatTy()) ||
      (m_ctx->getModuleMetaData()->compOpt.FloatDenormMode64 == IGC::FLOAT_DENORM_FLUSH_TO_ZERO &&
       I.getType()->isDoubleTy())) {
    switch (GetOpCode(&I)) {
    case llvm_select:
      flushesDenorms = true;
      for (uint i = 1; i < I.getNumOperands(); ++i) {
        Instruction *inst = dyn_cast<Instruction>(I.getOperand(i));
        if (inst && !FlushesDenormsOnOutput(*inst)) {
          flushesDenorms = false;
          break;
        }
      }
      break;
    case llvm_max:
    case llvm_min:
      flushesDenorms = true;
      for (uint i = 0; i < I.getNumOperands(); ++i) {
        Instruction *inst = dyn_cast<Instruction>(I.getOperand(i));
        if (inst && !FlushesDenormsOnOutput(*inst)) {
          flushesDenorms = false;
          break;
        }
      }
      break;
    case llvm_fsat:
    case llvm_fabs:
      if (isa<Instruction>(I.getOperand(0))) {
        flushesDenorms = FlushesDenormsOnOutput(*(cast<Instruction>(I.getOperand(0))));
      }
      break;
    case llvm_canonicalize:
    case llvm_fadd:
    case llvm_fadd_rtz:
    case llvm_fsub:
    case llvm_fmul:
    case llvm_fmul_rtz:
    case llvm_fma:
    case llvm_roundne:
    case llvm_round_z:
    case llvm_floor:
    case llvm_ceil:
    case llvm_frc:
      flushesDenorms = true;
      break;
    case llvm_fdiv:
    case llvm_frem:
    case llvm_cos:
    case llvm_sin:
    case llvm_log:
    case llvm_exp:
    case llvm_pow:
    case llvm_sqrt:
    case llvm_rsq:
      // extended math retain half denorms
      flushesDenorms = !I.getType()->isHalfTy();
      break;
    default:
      break;
    }
  }
  return flushesDenorms;
}
bool CodeGenPatternMatch::FlushesDenormsOnInput(Instruction &I) {
  bool flushesDenorms = false;
  if ((m_ctx->getModuleMetaData()->compOpt.FloatDenormMode16 == IGC::FLOAT_DENORM_FLUSH_TO_ZERO &&
       I.getType()->isHalfTy()) ||
      (m_ctx->getModuleMetaData()->compOpt.FloatDenormMode32 == IGC::FLOAT_DENORM_FLUSH_TO_ZERO &&
       I.getType()->isFloatTy()) ||
      (m_ctx->getModuleMetaData()->compOpt.FloatDenormMode64 == IGC::FLOAT_DENORM_FLUSH_TO_ZERO &&
       I.getType()->isDoubleTy())) {
    switch (GetOpCode(&I)) {
    case llvm_canonicalize:
    case llvm_fadd:
    case llvm_fadd_rtz:
    case llvm_fsub:
    case llvm_fmul:
    case llvm_fmul_rtz:
    case llvm_fma:
    case llvm_roundne:
    case llvm_round_z:
    case llvm_floor:
    case llvm_ceil:
    case llvm_frc:
      flushesDenorms = true;
      break;
    case llvm_fdiv:
    case llvm_frem:
    case llvm_cos:
    case llvm_sin:
    case llvm_log:
    case llvm_exp:
    case llvm_pow:
    case llvm_sqrt:
    case llvm_rsq:
      // extended math retain half denorms
      flushesDenorms = !I.getType()->isHalfTy();
      break;
    default:
      break;
    }
  }
  return flushesDenorms;
}

bool CodeGenPatternMatch::ContractionAllowed(llvm::Instruction &I) const {
  if (m_AllowContractions || (m_ctx->m_DriverInfo.RespectPerInstructionContractFlag() && I.hasAllowContract())) {
    return true;
  }
  return false;
}

// this function need to be in sync with CShader::EvaluateSIMDConstExpr on what can be supported
bool CodeGenPatternMatch::SIMDConstExpr(Instruction *C) {
  auto it = m_IsSIMDConstExpr.find(C);
  if (it != m_IsSIMDConstExpr.end()) {
    return it->second;
  }
  bool isConstExpr = false;
  if (BinaryOperator *op = dyn_cast<BinaryOperator>(C)) {
    switch (op->getOpcode()) {
    case Instruction::Add:
    case Instruction::Sub:
    case Instruction::Mul:
    case Instruction::Shl:
    case Instruction::Or:
    case Instruction::And:
      isConstExpr = IsConstOrSimdConstExpr(op->getOperand(0)) && IsConstOrSimdConstExpr(op->getOperand(1));
      break;
    default:
      break;
    }
  } else if (llvm::GenIntrinsicInst *genInst = dyn_cast<GenIntrinsicInst>(C)) {
    if (genInst->getIntrinsicID() == GenISAIntrinsic::GenISA_simdSize) {
      isConstExpr = true;
    }
  }
  m_IsSIMDConstExpr.insert(std::make_pair(C, isConstExpr));
  return isConstExpr;
}

bool CodeGenPatternMatch::NeedInstruction(llvm::Instruction &I) {
  if (SIMDConstExpr(&I)) {
    return false;
  }
  if (HasPhiUse(I) || HasSideEffect(I) || IsDbgInst(I) || (m_usedInstructions.find(&I) != m_usedInstructions.end())) {
    return true;
  }
  return false;
}

void CodeGenPatternMatch::AddToConstantPool(llvm::BasicBlock *UseBlock, llvm::Value *Val) {
  Constant *C = dyn_cast_or_null<Constant>(Val);
  if (!C)
    return;

  BasicBlock *LCA = UseBlock;
  // Determine where we put the constant initialization.
  // Choose loop pre-header as LICM.
  // XXX: Further investigation/tuning is needed to see whether
  // we need to hoist constant initialization out of the
  // top-level loop within a nested loop. So far, we only hoist
  // one level up.
  if (Loop *L = LI->getLoopFor(LCA)) {
    if (BasicBlock *Preheader = L->getLoopPreheader())
      LCA = Preheader;
  }
  // Find the common dominator as CSE.
  if (BasicBlock *BB = ConstantPlacement.lookup(C))
    LCA = DT->findNearestCommonDominator(LCA, BB);
  IGC_ASSERT_MESSAGE(LCA, "LCA always exists for reachable BBs within a function!");
  ConstantPlacement[C] = LCA;
}

bool CodeGenPatternMatch::supportsLSCImmediateGlobalBaseOffset() {
  bool res = IGC_GET_FLAG_VALUE(LscImmOffsMatch) > 1 || m_Platform.matchImmOffsetsLSC();
  return res;
}

// Check bool values that can be emitted as a single element predicate.
void CodeGenPatternMatch::gatherUniformBools(Value *Val) {
  if (!isUniform(Val) || Val->getType()->getScalarType()->isIntegerTy(1))
    return;

  // Only starts from select instruction for now.
  // It is more complicate for uses in terminators.
  if (SelectInst *SI = dyn_cast<SelectInst>(Val)) {
    Value *Cond = SI->getCondition();
    if (Cond->getType()->isVectorTy() || !Cond->hasOneUse())
      return;

    // All users of bool values.
    DenseSet<Value *> Vals;
    Vals.insert(SI);

    // Grow the list of bool values to be checked.
    std::vector<Value *> ValList;
    ValList.push_back(Cond);

    bool IsLegal = true;
    while (!ValList.empty()) {
      Value *V = ValList.back();
      ValList.pop_back();
      IGC_ASSERT(nullptr != V);
      IGC_ASSERT(nullptr != V->getType());
      IGC_ASSERT(isUniform(V));
      IGC_ASSERT(V->getType()->isIntegerTy(1));

      // Check uses.
      for (auto UI = V->user_begin(), UE = V->user_end(); UI != UE; ++UI) {
        Value *U = *UI;
        if (!Vals.count(U))
          goto FAIL;
      }

      // Check defs.
      Vals.insert(V);
      if (auto CI = dyn_cast<CmpInst>(V)) {
        IGC_ASSERT(isUniform(CI->getOperand(0)));
        IGC_ASSERT(isUniform(CI->getOperand(1)));
        if (CI->getOperand(0)->getType()->getScalarSizeInBits() == 1)
          goto FAIL;
        continue;
      } else if (auto BI = dyn_cast<BinaryOperator>(V)) {
        IGC_ASSERT(isUniform(BI->getOperand(0)));
        IGC_ASSERT(isUniform(BI->getOperand(1)));
        if (isa<Instruction>(BI->getOperand(0)))
          ValList.push_back(BI->getOperand(0));
        if (isa<Instruction>(BI->getOperand(1)))
          ValList.push_back(BI->getOperand(1));
        continue;
      }

    FAIL:
      IsLegal = false;
      break;
    }

    // Populate all boolean values if legal.
    if (IsLegal) {
      for (auto V : Vals) {
        if (V->getType()->isIntegerTy(1))
          UniformBools.insert(V);
      }
    }
  }
}

void CodeGenPatternMatch::CodeGenBlock(llvm::BasicBlock *bb) {
  llvm::BasicBlock::InstListType::reverse_iterator I, E;
  auto it = m_blockMap.find(bb);
  IGC_ASSERT(it != m_blockMap.end());
  SBasicBlock *block = it->second;

  // loop through instructions bottom up
  for (I = bb->rbegin(), E = bb->rend(); I != E; ++I) {
    llvm::Instruction &inst = (*I);

    if (NeedInstruction(inst)) {
      SetPatternRoot(inst);
      Pattern *pattern = Match(inst);
      if (pattern) {
        block->m_dags.push_back(SDAG(pattern, m_root));
        gatherUniformBools(m_root);
      }
    }
  }
}

void CodeGenPatternMatch::CreateBasicBlocks(llvm::Function *pLLVMFunc) {
  m_numBlocks = pLLVMFunc->size();
  m_blocks = new SBasicBlock[m_numBlocks];
  uint i = 0;
  for (BasicBlock &bb : *pLLVMFunc) {
    m_blocks[i].id = i;
    m_blocks[i].bb = &bb;
    m_blockMap.insert(std::pair<llvm::BasicBlock *, SBasicBlock *>(&bb, &m_blocks[i]));
    i++;
  }
}
Pattern *CodeGenPatternMatch::Match(llvm::Instruction &inst) {
  m_currentPattern = nullptr;
  visit(inst);
  return m_currentPattern;
}

void CodeGenPatternMatch::SetPatternRoot(llvm::Instruction &inst) {
  m_root = &inst;
  m_rootIsSubspanUse = IsSubspanUse(m_root);
}

template <typename Op_t, typename ConstTy> struct ClampWithConstants_match {
  typedef ConstTy *ConstPtrTy;

  Op_t Op;
  ConstPtrTy &CMin, &CMax;

  ClampWithConstants_match(const Op_t &OpMatch, ConstPtrTy &Min, ConstPtrTy &Max) : Op(OpMatch), CMin(Min), CMax(Max) {}

  template <typename OpTy> bool match(OpTy *V) {
    CallInst *GII = dyn_cast<CallInst>(V);
    if (!GII)
      return false;

    EOPCODE op = GetOpCode(GII);

    if (op != llvm_max && op != llvm_min)
      return false;

    Value *X = GII->getOperand(0);
    Value *C = GII->getOperand(1);
    if (isa<ConstTy>(X))
      std::swap(X, C);

    ConstPtrTy C0 = dyn_cast<ConstTy>(C);
    if (!C0)
      return false;

    CallInst *GII2 = dyn_cast<CallInst>(X);
    if (!GII2)
      return false;

    EOPCODE op2 = GetOpCode(GII2);
    if (!(op == llvm_min && op2 == llvm_max) && !(op == llvm_max && op2 == llvm_min))
      return false;

    X = GII2->getOperand(0);
    C = GII2->getOperand(1);
    if (isa<ConstTy>(X))
      std::swap(X, C);

    ConstPtrTy C1 = dyn_cast<ConstTy>(C);
    if (!C1)
      return false;

    if (!Op.match(X))
      return false;

    CMin = (op2 == llvm_min) ? C0 : C1;
    CMax = (op2 == llvm_min) ? C1 : C0;
    return true;
  }
};

template <typename OpTy, typename ConstTy>
inline ClampWithConstants_match<OpTy, ConstTy> m_ClampWithConstants(const OpTy &Op, ConstTy *&Min, ConstTy *&Max) {
  return ClampWithConstants_match<OpTy, ConstTy>(Op, Min, Max);
}

template <typename Op_t> struct IsNaN_match {
  Op_t Op;

  IsNaN_match(const Op_t &OpMatch) : Op(OpMatch) {}

  template <typename OpTy> bool match(OpTy *V) {
    using namespace llvm::PatternMatch;

    FCmpInst *FCI = dyn_cast<FCmpInst>(V);
    if (!FCI)
      return false;

    switch (FCI->getPredicate()) {
    case FCmpInst::FCMP_UNE:
      return FCI->getOperand(0) == FCI->getOperand(1) && Op.match(FCI->getOperand(0));
    case FCmpInst::FCMP_UNO:
      return m_Zero().match(FCI->getOperand(1)) && Op.match(FCI->getOperand(0));
    default:
      break;
    }

    return false;
  }
};

template <typename OpTy> inline IsNaN_match<OpTy> m_IsNaN(const OpTy &Op) { return IsNaN_match<OpTy>(Op); }

std::tuple<Value *, unsigned, VISA_Type> CodeGenPatternMatch::isFPToIntegerSatWithExactConstant(llvm::CastInst *I) {
  using namespace llvm::PatternMatch; // Scoped using declaration.

  unsigned Opcode = I->getOpcode();
  IGC_ASSERT(Opcode == Instruction::FPToSI || Opcode == Instruction::FPToUI);

  unsigned BitWidth = I->getDestTy()->getIntegerBitWidth();
  APFloat FMin(I->getSrcTy()->getFltSemantics());
  APFloat FMax(I->getSrcTy()->getFltSemantics());
  if (Opcode == Instruction::FPToSI) {
    if (FMax.convertFromAPInt(APInt::getSignedMaxValue(BitWidth), true, APFloat::rmNearestTiesToEven) != APFloat::opOK)
      return std::make_tuple(nullptr, 0, ISA_TYPE_F);
    if (FMin.convertFromAPInt(APInt::getSignedMinValue(BitWidth), true, APFloat::rmNearestTiesToEven) != APFloat::opOK)
      return std::make_tuple(nullptr, 0, ISA_TYPE_F);
  } else {
    if (FMax.convertFromAPInt(APInt::getMaxValue(BitWidth), false, APFloat::rmNearestTiesToEven) != APFloat::opOK)
      return std::make_tuple(nullptr, 0, ISA_TYPE_F);
    if (FMin.convertFromAPInt(APInt::getMinValue(BitWidth), false, APFloat::rmNearestTiesToEven) != APFloat::opOK)
      return std::make_tuple(nullptr, 0, ISA_TYPE_F);
  }

  llvm::ConstantFP *CMin = nullptr;
  llvm::ConstantFP *CMax = nullptr;
  llvm::Value *X = nullptr;

  if (!match(I->getOperand(0), m_ClampWithConstants(m_Value(X), CMin, CMax)))
    return std::make_tuple(nullptr, 0, ISA_TYPE_F);

  if (!CMin->isExactlyValue(FMin) || !CMax->isExactlyValue(FMax))
    return std::make_tuple(nullptr, 0, ISA_TYPE_F);

  return std::make_tuple(X, Opcode, GetType(I->getType(), m_ctx));
}

// The following pattern matching is targeted to the conversion from FP values
// to INTEGER values with saturation where the MAX and/or MIN INTEGER values
// cannot be represented in FP values exactly. E.g., UINT_MAX (2**32-1) in
// 'unsigned' cannot be represented in 'float', where only 23 significant bits
// are available but UINT_MAX needs 32 significant bits. We cannot simply
// express that conversion with saturation as
//
//  o := fptoui(clamp(x, float(UINT_MIN), float(UINT_MAX));
//
// as, in LLVM, fptoui is undefined when the 'unsigned' source cannot fit in
// 'float', where clamp(x, MIN, MAX) is defined as max(min(x, MAX), MIN),
//
// Hence, OCL use the following sequence (over-simplified by excluding the NaN
// case.)
//
//  o := select(fptoui(x), UINT_MIN, x < float(UINT_MIN));
//  o := select(o,         UINT_MAX, x > float(UINT_MAX));
//
// (We SHOULD use 'o := select(o, UINTMAX, x >= float(UINT_MAX))' as
// 'float(UINT_MAX)' will be rounded to UINT_MAX+1, i.e. 2 ** 32, and the next
// smaller value than float(UINT_MAX) in 'float' is (2 ** 24 - 1) << 8. For
// 'int', that's also true for INT_MIN.)

std::tuple<Value *, unsigned, VISA_Type>
CodeGenPatternMatch::isFPToSignedIntSatWithInexactConstant(llvm::SelectInst *SI) {
  using namespace llvm::PatternMatch; // Scoped using declaration.

  // TODO
  return std::make_tuple(nullptr, 0, ISA_TYPE_F);
}

std::tuple<Value *, unsigned, VISA_Type>
CodeGenPatternMatch::isFPToUnsignedIntSatWithInexactConstant(llvm::SelectInst *SI) {
  using namespace llvm::PatternMatch; // Scoped using declaration.

  Constant *C0 = dyn_cast<Constant>(SI->getTrueValue());
  if (!C0)
    return std::make_tuple(nullptr, 0, ISA_TYPE_F);
  if (!isa<ConstantFP>(C0) && !isa<ConstantInt>(C0))
    return std::make_tuple(nullptr, 0, ISA_TYPE_F);
  Value *Cond = SI->getCondition();

  SelectInst *SI2 = dyn_cast<SelectInst>(SI->getFalseValue());
  if (!SI2)
    return std::make_tuple(nullptr, 0, ISA_TYPE_F);
  Constant *C1 = dyn_cast<Constant>(SI2->getTrueValue());
  if (!C1)
    return std::make_tuple(nullptr, 0, ISA_TYPE_F);
  if (!isa<ConstantFP>(C1) && !isa<ConstantInt>(C1))
    return std::make_tuple(nullptr, 0, ISA_TYPE_F);
  Value *Cond2 = SI2->getCondition();

  Value *X = SI2->getFalseValue();
  Type *Ty = X->getType();
  if (Ty->isFloatTy()) {
    BitCastInst *BC = dyn_cast<BitCastInst>(X);
    if (!BC)
      return std::make_tuple(nullptr, 0, ISA_TYPE_F);
    X = BC->getOperand(0);
    Ty = X->getType();
    C1 = ConstantExpr::getBitCast(C1, Ty);
    C0 = ConstantExpr::getBitCast(C0, Ty);
  }
  IntegerType *ITy = dyn_cast<IntegerType>(Ty);
  if (!ITy)
    return std::make_tuple(nullptr, 0, ISA_TYPE_F);
  unsigned BitWidth = ITy->getBitWidth();
  FPToUIInst *CI = dyn_cast<FPToUIInst>(X);
  if (!CI)
    return std::make_tuple(nullptr, 0, ISA_TYPE_F);
  Ty = CI->getSrcTy();
  if (!(Ty->isFloatTy() && BitWidth == 32) && !(Ty->isDoubleTy() && BitWidth == 64))
    return std::make_tuple(nullptr, 0, ISA_TYPE_F);
  X = CI->getOperand(0);

  ConstantInt *CMin = dyn_cast<ConstantInt>(C0);
  ConstantInt *CMax = dyn_cast<ConstantInt>(C1);
  if (!CMax || !CMin || !CMax->isMaxValue(false) || !CMin->isMinValue(false))
    return std::make_tuple(nullptr, 0, ISA_TYPE_F);

  Constant *FMin = ConstantExpr::getUIToFP(CMin, Ty);
  Constant *FMax = ConstantExpr::getUIToFP(CMax, Ty);

  FCmpInst::Predicate Pred = FCmpInst::FCMP_FALSE;
  if (!match(Cond2, m_FCmp(Pred, m_Specific(X), m_Specific(FMax))))
    return std::make_tuple(nullptr, 0, ISA_TYPE_F);
  if (Pred != FCmpInst::FCMP_OGT) // FIXME: We should use OGE instead of OGT.
    return std::make_tuple(nullptr, 0, ISA_TYPE_F);

  FCmpInst::Predicate Pred2 = FCmpInst::FCMP_FALSE;
  if (!match(Cond, m_Or(m_FCmp(Pred, m_Specific(X), m_Specific(FMin)), m_FCmp(Pred2, m_Specific(X), m_Specific(X))))) {
    if (!match(Cond, m_Or(m_FCmp(Pred, m_Specific(X), m_Specific(FMin)), m_Zero()))) {
      return std::make_tuple(nullptr, 0, ISA_TYPE_F);
    }
    // Special case where the staturatured result is bitcasted into float
    // again (due to typedwrite only accepts `float`. So the isNaN(X) is
    // reduced to `false`.
    Pred2 = FCmpInst::FCMP_UNE;
  }
  if (Pred != FCmpInst::FCMP_OLT || Pred2 != FCmpInst::FCMP_UNE)
    return std::make_tuple(nullptr, 0, ISA_TYPE_F);

  VISA_Type type = GetType(CI->getType(), m_ctx);

  // Fold extra clamp.
  Value *X2 = nullptr;
  ConstantFP *CMin2 = nullptr;
  ConstantFP *CMax2 = nullptr;
  if (match(X, m_ClampWithConstants(m_Value(X2), CMin2, CMax2))) {
    if (CMin2 == FMin) {
      if (CMax2->isExactlyValue(255.0)) {
        X = X2;
        type = ISA_TYPE_B;
      } else if (CMax2->isExactlyValue(65535.0)) {
        X = X2;
        type = ISA_TYPE_W;
      }
    }
  }

  return std::make_tuple(X, Instruction::FPToUI, type);
}

bool CodeGenPatternMatch::MatchFPToIntegerWithSaturation(llvm::Instruction &I) {
  Value *X = nullptr;
  unsigned Opcode = 0;
  VISA_Type type = ISA_TYPE_NUM;

  if (CastInst *CI = dyn_cast<CastInst>(&I)) {
    std::tie(X, Opcode, type) = isFPToIntegerSatWithExactConstant(CI);
    if (!X)
      return false;
  } else if (SelectInst *SI = dyn_cast<SelectInst>(&I)) {
    std::tie(X, Opcode, type) = isFPToSignedIntSatWithInexactConstant(SI);
    if (!X) {
      std::tie(X, Opcode, type) = isFPToUnsignedIntSatWithInexactConstant(SI);
      if (!X)
        return false;
    }
  } else {
    return false;
  }

  // Match!
  IGC_ASSERT(Opcode == Instruction::FPToSI || Opcode == Instruction::FPToUI);

  struct FPToIntegerWithSaturationPattern : public Pattern {
    bool isUnsigned, needBitCast;
    VISA_Type type;
    SSource src;
    virtual void Emit(EmitPass *pass, const DstModifier &dstMod) {
      pass->EmitFPToIntWithSat(isUnsigned, needBitCast, type, src, dstMod);
    }
  };

  bool isUnsigned = (Opcode == Instruction::FPToUI);
  FPToIntegerWithSaturationPattern *pat = new (m_allocator) FPToIntegerWithSaturationPattern();
  pat->isUnsigned = isUnsigned;
  pat->needBitCast = !I.getType()->isIntegerTy();
  pat->type = type;
  pat->src = GetSource(X, !isUnsigned, false, IsSourceOfSample(&I));
  AddPattern(pat);

  return true;
}

std::tuple<Value *, bool, bool> CodeGenPatternMatch::isIntegerSatTrunc(llvm::SelectInst *SI) {
  using namespace llvm::PatternMatch; // Scoped using declaration.

  ICmpInst *Cmp = dyn_cast<ICmpInst>(SI->getOperand(0));
  if (!Cmp)
    return std::make_tuple(nullptr, false, false);

  ICmpInst::Predicate Pred = Cmp->getPredicate();
  if (Pred != ICmpInst::ICMP_SGT && Pred != ICmpInst::ICMP_UGT)
    return std::make_tuple(nullptr, false, false);

  ConstantInt *CI = dyn_cast<ConstantInt>(Cmp->getOperand(1));
  if (!CI)
    return std::make_tuple(nullptr, false, false);

  // Truncate into unsigned integer by default.
  bool isSignedDst = false;
  unsigned DstBitWidth = SI->getType()->getIntegerBitWidth();
  unsigned SrcBitWidth = Cmp->getOperand(0)->getType()->getIntegerBitWidth();
  APInt UMax = APInt::getMaxValue(DstBitWidth);
  APInt UMin = APInt::getMinValue(DstBitWidth);
  APInt SMax = APInt::getSignedMaxValue(DstBitWidth);
  APInt SMin = APInt::getSignedMinValue(DstBitWidth);
  if (SrcBitWidth > DstBitWidth) {
    UMax = UMax.zext(SrcBitWidth);
    UMin = UMin.zext(SrcBitWidth);
    SMax = SMax.sext(SrcBitWidth);
    SMin = SMin.sext(SrcBitWidth);
  } else {
    // SrcBitwidth should be always wider than DstBitwidth,
    // since src is a source of a trunc instruction, and dst
    // have the same width as its destination.
    return std::make_tuple(nullptr, false, false);
  }

  if (CI->getValue() != UMax && CI->getValue() != SMax)
    return std::make_tuple(nullptr, false, false);
  if (CI->getValue() == SMax) // Truncate into signed integer.
    isSignedDst = true;

  APInt MinValue = isSignedDst ? SMin : UMin;
  CI = dyn_cast<ConstantInt>(SI->getOperand(1));
  if (!CI || !CI->isMaxValue(isSignedDst))
    return std::make_tuple(nullptr, false, false);

  TruncInst *TI = dyn_cast<TruncInst>(SI->getOperand(2));
  if (!TI)
    return std::make_tuple(nullptr, false, false);

  Value *Val = TI->getOperand(0);
  if (Val != Cmp->getOperand(0))
    return std::make_tuple(nullptr, false, false);

  // Truncate from unsigned integer.
  if (Pred == ICmpInst::ICMP_UGT)
    return std::make_tuple(Val, isSignedDst, false);

  // Truncate from signed integer. Need to check further for lower bound.
  Value *LHS = nullptr, *RHS = nullptr;
  if (!match(Val, m_SMax(m_Value(LHS), m_Value(RHS))))
    return std::make_tuple(nullptr, false, false);

  if (isa<ConstantInt>(LHS))
    std::swap(LHS, RHS);

  CI = dyn_cast<ConstantInt>(RHS);
  if (!CI || CI->getValue() != MinValue)
    return std::make_tuple(nullptr, false, false);

  return std::make_tuple(LHS, isSignedDst, true);
}

bool CodeGenPatternMatch::MatchIntegerTruncSatModifier(llvm::SelectInst &I) {
  // Only match BYTE or WORD.
  if (!I.getType()->isIntegerTy(8) && !I.getType()->isIntegerTy(16))
    return false;
  auto [Src, isSignedDst, isSignedSrc] = isIntegerSatTrunc(&I);
  if (!Src)
    return false;

  struct IntegerSatTruncPattern : public Pattern {
    SSource src;
    bool isSignedDst;
    bool isSignedSrc;
    virtual void Emit(EmitPass *pass, const DstModifier &dstMod) {
      pass->EmitIntegerTruncWithSat(isSignedDst, isSignedSrc, src, dstMod);
    }
  };

  IntegerSatTruncPattern *pat = new (m_allocator) IntegerSatTruncPattern();
  pat->src = GetSource(Src, isSignedSrc, false, IsSourceOfSample(&I));
  pat->isSignedDst = isSignedDst;
  pat->isSignedSrc = isSignedSrc;
  AddPattern(pat);

  return true;
}

bool CodeGenPatternMatch::MatchShrSatModifier(llvm::SelectInst &I) {
  struct ShrSatPattern : public Pattern {
    SSource sources[2];
    Instruction *inst;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      DstModifier mod = modifier;
      mod.sat = true;
      pass->EmitSimpleAlu(GetOpCode(inst), sources, mod, true /*isUnsigned*/);
    }
  };

  Value *LHS = nullptr, *RHS = nullptr;
  if (isMax(&I, LHS, RHS)) {
    if (isa<ConstantInt>(LHS) || isa<ConstantInt>(RHS)) {
      ConstantInt *c = isa<ConstantInt>(LHS) ? cast<ConstantInt>(LHS) : cast<ConstantInt>(RHS);
      if (c->getZExtValue() == 0) {
        Value *v = isa<ConstantInt>(LHS) ? RHS : LHS;
        if (isa<LShrOperator>(v) || isa<AShrOperator>(v)) {
          Instruction *inst = cast<Instruction>(v);
          ShrSatPattern *pattern = new (m_allocator) ShrSatPattern();
          pattern->inst = inst;
          pattern->sources[0] = GetSource(inst->getOperand(0), false, false, IsSourceOfSample(&I));
          pattern->sources[1] = GetSource(inst->getOperand(1), false, false, IsSourceOfSample(&I));
          AddPattern(pattern);

          return true;
        }
      }
    }
  }
  return false;
}

void CodeGenPatternMatch::visitFPToSIInst(llvm::FPToSIInst &I) {
  bool match = MatchFPToIntegerWithSaturation(I) || MatchModifier(I);
  IGC_ASSERT_MESSAGE(match, "Pattern match Failed");
}

void CodeGenPatternMatch::visitFPToUIInst(llvm::FPToUIInst &I) {
  bool match = MatchFPToIntegerWithSaturation(I) || MatchModifier(I);
  IGC_ASSERT_MESSAGE(match, "Pattern match Failed");
}

bool CodeGenPatternMatch::MatchSIToFPZExt(llvm::SIToFPInst *S2FI) {
  ZExtInst *ZEI = dyn_cast<ZExtInst>(S2FI->getOperand(0));
  if (!ZEI)
    return false;
  if (!ZEI->getSrcTy()->isIntegerTy(1))
    return false;

  struct SIToFPExtPattern : public Pattern {
    SSource src;
    virtual void Emit(EmitPass *pass, const DstModifier &dstMod) { pass->EmitSIToFPZExt(src, dstMod); }
  };

  SIToFPExtPattern *pat = new (m_allocator) SIToFPExtPattern();
  pat->src = GetSource(ZEI->getOperand(0), false, false, IsSourceOfSample(S2FI));
  AddPattern(pat);

  return true;
}

void CodeGenPatternMatch::visitCastInst(llvm::CastInst &I) {
  bool match = 0;
  if (I.getOpcode() == Instruction::SExt) {
    match = MatchUnpack4i8(I) || MatchCmpSext(I) || MatchModifier(I);
  } else if (I.getOpcode() == Instruction::ZExt) {
    match = MatchUnpack4i8(I) || MatchModifier(I);
  } else if (I.getOpcode() == Instruction::SIToFP) {
    match = MatchSIToFPZExt(cast<SIToFPInst>(&I)) || MatchModifier(I);
  } else if (I.getOpcode() == Instruction::Trunc) {
    match = MatchModifier(I);
  } else {
    match = MatchModifier(I);
  }
}

bool CodeGenPatternMatch::NeedVMask() { return m_NeedVMask; }

bool CodeGenPatternMatch::HasUseOutsideLoop(llvm::Value *v) {
  if (Instruction *inst = dyn_cast<Instruction>(v)) {
    if (Loop *L = LI->getLoopFor(inst->getParent())) {
      for (auto UI = inst->user_begin(), E = inst->user_end(); UI != E; ++UI) {
        if (!L->contains(cast<Instruction>(*UI))) {
          return true;
        }
      }
    }
  }
  return false;
}

void CodeGenPatternMatch::HandleSubspanUse(llvm::Value *v, bool isSampleSource) {
  IGC_ASSERT(m_root != nullptr);
  if (m_ctx->type != ShaderType::PIXEL_SHADER) {
    return;
  }
  if (!isa<Constant>(v) && !m_WI->isUniform(v)) {
    if (isa<PHINode>(v) || HasUseOutsideLoop(v)) {
      // If a phi is used in a subspan we cannot propagate the subspan use and need to use VMask
      m_NeedVMask = true;

      if (isSampleSource && isa<PHINode>(v)) {
        m_sampleSource.insert(v);
      }

      if (Instruction *I = dyn_cast<Instruction>(v)) {
        // this is WA for situation where application has early return (not discard) from shader
        // in this case phi node will not set sample coords properly on all simd lanes
        // WA is to initialize register used for phi node with zero, which eliminates corruptions
        // here we collect suspected phi nodes to be resolved later in EmitVISAPass
        if (m_ctx->getModuleMetaData()->compOpt.initializePhiSampleSourceWA && isSampleSource &&
            !PDT->dominates(I->getParent(), &I->getFunction()->getEntryBlock())) {
          m_sampleUnderCFPHIsource.insert(v);
        }
      }
    } else {
      if (isa<SampleIntrinsic>(v)) {
        // Also, we want to apply vector mask instead of no mask to sample instructions,
        // because it should have better performance on workloads with many disabled subspans
        m_NeedVMask = true;
      }

      if (Instruction *I = dyn_cast<Instruction>(v)) {
        if (isSampleSource) {
          m_sampleSource.insert(v);
          if (!PDT->dominates(I->getParent(), &I->getFunction()->getEntryBlock())) {
            m_sampleUnderCFsource.insert(v);
          }

          if (Loop *L = LI->getLoopFor(I->getParent())) {
            if (L->contains(I)) {
              m_sampleUnderCFsource.insert(v);
            }
          }
        }
      }

      m_subSpanUse.insert(v);
      if (LoadInst *load = dyn_cast<LoadInst>(v)) {
        if (load->getPointerAddressSpace() == ADDRESS_SPACE_PRIVATE) {
          m_NeedVMask = true;
        }
      }
      if (HasPhiUse(*v) && m_WI->insideDivergentCF(m_root)) {
        // \todo, more accurate condition for force-isolation
        ForceIsolate(v);
      }
    }
  }
}

bool CodeGenPatternMatch::MatchMinMax(llvm::SelectInst &SI) {
  // Pattern to emit.
  struct MinMaxPattern : public Pattern {
    SSource srcs[2];
    bool isMin, isUnsigned;
    virtual void Emit(EmitPass *pass, const DstModifier &dstMod) {
      // FIXME: We should tell umax/umin from max/min as integers in LLVM
      // have no sign!
      pass->EmitMinMax(isMin, isUnsigned, srcs, dstMod);
    }
  };

  // Skip min/max pattern matching on FP, which needs to either explicitly
  // use intrinsics or convert them into intrinsic in GenIRLower pass.
  if (SI.getType()->isFloatingPointTy())
    return false;

  bool isMin = false, isUnsigned = false;
  llvm::Value *LHS = nullptr, *RHS = nullptr;

  if (!isMinOrMax(&SI, LHS, RHS, isMin, isUnsigned))
    return false;

  MinMaxPattern *pat = new (m_allocator) MinMaxPattern();
  // FIXME: We leave unsigned operand without source modifier so far. When
  // its behavior is detailed and correcty modeled, consider to add source
  // modifier support.
  pat->srcs[0] = GetSource(LHS, !isUnsigned, false, IsSourceOfSample(&SI));
  pat->srcs[1] = GetSource(RHS, !isUnsigned, false, IsSourceOfSample(&SI));
  pat->isMin = isMin;
  pat->isUnsigned = isUnsigned;
  AddPattern(pat);

  return true;
}

void CodeGenPatternMatch::visitSelectInst(SelectInst &I) {
  bool match = MatchFloatingPointSatModifier(I) || MatchIntegerTruncSatModifier(I) || MatchShrSatModifier(I) ||
               MatchAbsNeg(I) || MatchFPToIntegerWithSaturation(I) || MatchMinMax(I) || MatchCmpSelect(I) ||
               MatchSelectModifier(I);
  IGC_ASSERT_MESSAGE(match, "Pattern Match failed");
}

void CodeGenPatternMatch::visitBinaryOperator(llvm::BinaryOperator &I) {

  bool match = false;
  switch (I.getOpcode()) {
  case Instruction::FSub:
    match = MatchFloor(I) || MatchFrc(I) || MatchLrp(I) || MatchPredAdd(I) || MatchMad(I) || MatchAbsNeg(I) ||
            MatchModifier(I);
    break;
  case Instruction::Sub:
    match = MatchMad(I) || MatchAdd3(I) || MatchAbsNeg(I) || MatchMulAdd16(I) || MatchModifier(I);
    break;
  case Instruction::Mul:
    match = MatchFullMul32(I) ||
            MatchMulAdd16(I) || MatchModifier(I);
    break;
  case Instruction::Add:
    match = MatchMad(I) || MatchAdd3(I) || MatchMulAdd16(I) || MatchModifier(I);
    break;
  case Instruction::UDiv:
  case Instruction::SDiv:
    match = MatchAvg(I) || MatchModifier(I);
    break;
  case Instruction::AShr:
    match = MatchAvg(I) || MatchBinaryUnpack4i8(I) || MatchModifier(I);
    break;
  case Instruction::FMul:
    match = MatchArcpFdiv(I) || MatchModifier(I);
    break;
  case Instruction::URem:
  case Instruction::SRem:
  case Instruction::FRem:
    match = MatchModifier(I);
    break;
  case Instruction::Shl:
    match = MatchBinaryUnpack4i8(I) || MatchModifier(I);
    break;
  case Instruction::LShr:
    match = MatchBinaryUnpack4i8(I) || MatchModifier(I, false);
    break;
  case Instruction::FDiv:
    match = MatchRsqrt(I) || MatchModifier(I);
    break;
  case Instruction::FAdd:
    match = MatchLrp(I) || MatchPredAdd(I) || MatchMad(I) || MatchSimpleAdd(I) || MatchModifier(I);
    break;
  case Instruction::And:
    match = MatchBfn(I) || MatchBoolOp(I) || MatchLogicAlu(I);
    break;
  case Instruction::Or:
    match = MatchPack4i8(I) || MatchBfn(I) || MatchBoolOp(I) || MatchLogicAlu(I);
    break;
  case Instruction::Xor:
    match = MatchBfn(I) || MatchLogicAlu(I);
    break;
  default:
    IGC_ASSERT_MESSAGE(0, "unknown binary instruction");
    break;
  }
  IGC_ASSERT(match == true);
}

void CodeGenPatternMatch::visitCmpInst(llvm::CmpInst &I) {
  bool match = MatchGenericPointersCmp(I) || MatchCondModifier(I) || MatchModifier(I);
  IGC_ASSERT(match);
}

void CodeGenPatternMatch::visitBranchInst(llvm::BranchInst &I) { MatchBranch(I); }

void CodeGenPatternMatch::visitCallInst(CallInst &I) {
  bool match = false;
  using namespace GenISAIntrinsic;
  if (GenIntrinsicInst *GII = llvm::dyn_cast<GenIntrinsicInst>(&I)) {
    switch (GII->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_ROUNDNE:
    case GenISAIntrinsic::GenISA_imulH:
    case GenISAIntrinsic::GenISA_umulH:
    case GenISAIntrinsic::GenISA_uaddc:
    case GenISAIntrinsic::GenISA_usubb:
    case GenISAIntrinsic::GenISA_bfrev:
    case GenISAIntrinsic::GenISA_IEEE_Sqrt:
    case GenISAIntrinsic::GenISA_IEEE_Divide:
    case GenISAIntrinsic::GenISA_rsq:
    case GenISAIntrinsic::GenISA_inv:
      match = MatchModifier(I);
      break;
    case GenISAIntrinsic::GenISA_intatomicraw:
    case GenISAIntrinsic::GenISA_floatatomicraw:
    case GenISAIntrinsic::GenISA_intatomicrawA64:
    case GenISAIntrinsic::GenISA_floatatomicrawA64:
    case GenISAIntrinsic::GenISA_icmpxchgatomicraw:
    case GenISAIntrinsic::GenISA_fcmpxchgatomicraw:
    case GenISAIntrinsic::GenISA_icmpxchgatomicrawA64:
    case GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64:
    case GenISAIntrinsic::GenISA_dwordatomicstructured:
    case GenISAIntrinsic::GenISA_floatatomicstructured:
    case GenISAIntrinsic::GenISA_cmpxchgatomicstructured:
    case GenISAIntrinsic::GenISA_fcmpxchgatomicstructured:
    case GenISAIntrinsic::GenISA_intatomictyped:
    case GenISAIntrinsic::GenISA_icmpxchgatomictyped:
    case GenISAIntrinsic::GenISA_typedread:
    case GenISAIntrinsic::GenISA_typedwrite:
    case GenISAIntrinsic::GenISA_typedreadMS:
    case GenISAIntrinsic::GenISA_typedwriteMS:
    case GenISAIntrinsic::GenISA_floatatomictyped:
    case GenISAIntrinsic::GenISA_fcmpxchgatomictyped:
    case GenISAIntrinsic::GenISA_ldlptr:
    case GenISAIntrinsic::GenISA_ldstructured:
    case GenISAIntrinsic::GenISA_storestructured1:
    case GenISAIntrinsic::GenISA_storestructured2:
    case GenISAIntrinsic::GenISA_storestructured3:
    case GenISAIntrinsic::GenISA_storestructured4:
    case GenISAIntrinsic::GenISA_atomiccounterinc:
    case GenISAIntrinsic::GenISA_atomiccounterpredec:
    case GenISAIntrinsic::GenISA_ldptr:
      if (supportsLSCImmediateGlobalBaseOffset()) {
        match = MatchImmOffsetLSC(I);
        if (match)
          return;
      }
      match = MatchSingleInstruction(I);
      break;
    case GenISAIntrinsic::GenISA_ldrawvector_indexed:
    case GenISAIntrinsic::GenISA_ldraw_indexed:
    case GenISAIntrinsic::GenISA_storerawvector_indexed:
    case GenISAIntrinsic::GenISA_storeraw_indexed:
      if (supportsLSCImmediateGlobalBaseOffset()) {
        match = MatchImmOffsetLSC(I);
        if (match)
          return;
      }
      match = MatchSingleInstruction(I);
      break;
    case GenISAIntrinsic::GenISA_GradientX:
    case GenISAIntrinsic::GenISA_GradientY:
    case GenISAIntrinsic::GenISA_GradientXfine:
    case GenISAIntrinsic::GenISA_GradientYfine:
      match = MatchGradient(*GII);
      break;
    case GenISAIntrinsic::GenISA_sampleptr:
    case GenISAIntrinsic::GenISA_sampleBptr:
    case GenISAIntrinsic::GenISA_sampleBCptr:
    case GenISAIntrinsic::GenISA_sampleCptr:
    case GenISAIntrinsic::GenISA_gather4Bptr:
    case GenISAIntrinsic::GenISA_gather4BPOptr:
    case GenISAIntrinsic::GenISA_gather4Iptr:
    case GenISAIntrinsic::GenISA_gather4IPOptr:
    case GenISAIntrinsic::GenISA_gather4ICptr:
    case GenISAIntrinsic::GenISA_gather4ICPOptr:
    case GenISAIntrinsic::GenISA_sampleMlodptr:
    case GenISAIntrinsic::GenISA_sampleCMlodptr:
    case GenISAIntrinsic::GenISA_sampleBCMlodptr:
    case GenISAIntrinsic::GenISA_samplePOptr:
    case GenISAIntrinsic::GenISA_samplePOBptr:
    case GenISAIntrinsic::GenISA_samplePOCptr:
    case GenISAIntrinsic::GenISA_gather4POPackedBptr:
    case GenISAIntrinsic::GenISA_gather4POPackedIptr:
    case GenISAIntrinsic::GenISA_gather4POPackedICptr:
    case GenISAIntrinsic::GenISA_lodptr:
    case GenISAIntrinsic::GenISA_sampleKillPix:
      match = MatchSampleDerivative(*GII);
      break;
    case GenISAIntrinsic::GenISA_fsat:
      match = MatchFloatingPointSatModifier(I);
      break;
    case GenISAIntrinsic::GenISA_usat:
    case GenISAIntrinsic::GenISA_isat:
      match = MatchIntegerSatModifier(I);
      break;
    case GenISAIntrinsic::GenISA_WaveShuffleIndex:
    case GenISAIntrinsic::GenISA_WaveBroadcast:
      match = MatchRegisterRegion(*GII) || MatchShuffleBroadCast(*GII) || MatchWaveShuffleIndex(*GII);
      break;
    case GenISAIntrinsic::GenISA_WaveBallot:
    case GenISAIntrinsic::GenISA_WaveInverseBallot:
    case GenISAIntrinsic::GenISA_WaveClusteredBallot:
    case GenISAIntrinsic::GenISA_WaveAll:
    case GenISAIntrinsic::GenISA_WaveClustered:
    case GenISAIntrinsic::GenISA_WaveInterleave:
    case GenISAIntrinsic::GenISA_WaveClusteredInterleave:
    case GenISAIntrinsic::GenISA_WavePrefix:
    case GenISAIntrinsic::GenISA_WaveClusteredPrefix:
      match = MatchWaveInstruction(*GII);
      break;
    case GenISAIntrinsic::GenISA_simdBlockRead:
    case GenISAIntrinsic::GenISA_simdBlockWrite:
      match = MatchBlockReadWritePointer(*GII) || MatchSingleInstruction(*GII);
      break;
    case GenISAIntrinsic::GenISA_UnmaskedRegionBegin:
      match = MatchUnmaskedRegionBoundary(I, true);
      break;
    case GenISAIntrinsic::GenISA_UnmaskedRegionEnd:
      match = MatchUnmaskedRegionBoundary(I, false);
      break;
    case GenISAIntrinsic::GenISA_sub_group_dpas:
    case GenISAIntrinsic::GenISA_dpas:
      match = MatchDpas(*GII);
      break;
    case GenISAIntrinsic::GenISA_dp4a_ss:
    case GenISAIntrinsic::GenISA_dp4a_su:
    case GenISAIntrinsic::GenISA_dp4a_us:
    case GenISAIntrinsic::GenISA_dp4a_uu:
      match = MatchDp4a(*GII);
      break;
    default:
      match = MatchSingleInstruction(I);
      // no pattern for the rest of the intrinsics
      break;
    }
    IGC_ASSERT_MESSAGE(match, "no pattern found for GenISA intrinsic");
  } else {
    Function *Callee = I.getCalledFunction();

    if (IGCMetrics::IGCMetric::isMetricFuncCall(&I)) {
      // dont do anything with metrics calls
      return;
    }

    // Match inline asm
    if (I.isInlineAsm()) {
      if (getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->m_DriverInfo.SupportInlineAssembly()) {
        match = MatchSingleInstruction(I);
      }
    }
    // Match indirect call, support declarations for indirect funcs
    else if (!Callee || Callee->hasFnAttribute("referenced-indirectly") ||
             Callee->hasFnAttribute("invoke_simd_target")) {
      match = MatchSingleInstruction(I);
    }
    // Match direct call, skip declarations
    else if (!Callee->isDeclaration()) {
      match = MatchSingleInstruction(I);
    }
  }
  IGC_ASSERT_MESSAGE(match, "no match for this call");
}

void CodeGenPatternMatch::visitUnaryInstruction(llvm::UnaryInstruction &I) {
  bool match = false;
  switch (I.getOpcode()) {
  case Instruction::Alloca:
  case Instruction::Load:
    match = MatchSingleInstruction(I);
    break;
  case Instruction::FNeg:
    match = MatchAbsNeg(I);
    break;
  }
  IGC_ASSERT(match);
}

void CodeGenPatternMatch::visitIntrinsicInst(llvm::IntrinsicInst &I) {
  bool match = false;
  switch (I.getIntrinsicID()) {
  case Intrinsic::sqrt:
  case Intrinsic::log2:
  case Intrinsic::cos:
  case Intrinsic::sin:
  case Intrinsic::pow:
  case Intrinsic::powi:
  case Intrinsic::floor:
  case Intrinsic::ceil:
  case Intrinsic::trunc:
  case Intrinsic::ctpop:
  case Intrinsic::ctlz:
  case Intrinsic::cttz:
    match = MatchModifier(I);
    break;
  case Intrinsic::exp2:
    match = MatchPow(I) || MatchModifier(I);
    break;
  case Intrinsic::abs:
  case Intrinsic::fabs:
    match = MatchAbsNeg(I);
    break;
  case Intrinsic::fma:
    match = MatchFMA(I);
    break;
  case Intrinsic::maxnum:
  case Intrinsic::minnum:
    match = MatchFloatingPointSatModifier(I) || MatchModifier(I);
    break;
  case Intrinsic::fshl:
  case Intrinsic::fshr:
    match = MatchFunnelShiftRotate(I);
    break;
  case Intrinsic::canonicalize:
    match = MatchCanonicalizeInstruction(I);
    break;
  default:
    match = MatchSingleInstruction(I);
    // no pattern for the rest of the intrinsics
    break;
  }
  IGC_ASSERT_MESSAGE(match, "no pattern found");
}

void CodeGenPatternMatch::visitStoreInst(StoreInst &I) {
  bool match = false;
  if (supportsLSCImmediateGlobalBaseOffset()) {
    match = MatchImmOffsetLSC(I);
    if (match)
      return;
  }
  match = MatchSingleInstruction(I);
  IGC_ASSERT(match);
}

void CodeGenPatternMatch::visitLoadInst(LoadInst &I) {
  bool match = false;
  if (supportsLSCImmediateGlobalBaseOffset()) {
    match = MatchImmOffsetLSC(I);
    if (match)
      return;
  }
  match = MatchSingleInstruction(I);
  IGC_ASSERT(match);
}

void CodeGenPatternMatch::visitInstruction(llvm::Instruction &I) {
  // use default pattern
  MatchSingleInstruction(I);
}

void CodeGenPatternMatch::visitExtractElementInst(llvm::ExtractElementInst &I) {
  Value *VecOpnd = I.getVectorOperand();
  if (isa<Constant>(VecOpnd)) {
    const Function *F = I.getParent()->getParent();
    unsigned NUse = 0;
    for (auto User : VecOpnd->users()) {
      if (auto Inst = dyn_cast<Instruction>(User)) {
        NUse += (Inst->getParent()->getParent() == F);
      }
    }

    // Only add it to pool when there are multiple uses within this
    // function; otherwise no benefit but to hurt RP.
    if (NUse > 1)
      AddToConstantPool(I.getParent(), VecOpnd);
  }
  MatchSingleInstruction(I);
}

void CodeGenPatternMatch::visitPHINode(PHINode &I) {
  struct PHIPattern : Pattern {
    // bool isInDivergentLoop = false;
    llvm::PHINode *phi = nullptr;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      IGC_ASSERT(modifier.sat == false);
      IGC_ASSERT(modifier.flag == nullptr);
      pass->EmitInitializePHI(phi);
    }
  };

  if (Loop *L = LI->getLoopFor(I.getParent())) {
    SmallVector<BasicBlock *, 4> exitingBlocks;
    SmallPtrSet<Instruction *, 8> visitedInstructions;
    L->getExitingBlocks(exitingBlocks);
    for (auto exitingBlock : exitingBlocks) {
      if (!isUniform(exitingBlock->getTerminator())) {
        std::vector<Instruction *> worklist;
        worklist.push_back(&I);
        while (!worklist.empty()) {
          Instruction *currentInst = worklist.back();
          worklist.pop_back();
          for (auto operand : currentInst->operand_values()) {
            if (auto nextInst = dyn_cast<Instruction>(operand)) {
              if (nextInst == &I) {
                return;
              }
              if (L->contains(nextInst->getParent()) && !visitedInstructions.contains(nextInst)) {
                worklist.push_back(nextInst);
                visitedInstructions.insert(nextInst);
              }
            }
          }
        }
        break;
      }
    }
  }
  if (IsSourceOfSample(&I)) {
    PHIPattern *pattern = new (m_allocator) PHIPattern();
    pattern->phi = &I;
    // pattern->isInDivergentLoop = isInDivergentLoop;
    AddPattern(pattern);
  }
}

void CodeGenPatternMatch::visitBitCastInst(BitCastInst &I) {
  // detect
  // %66 = insertelement <2 x i32> <i32 0, i32 undef>, i32 %xor19.i, i32 1
  // %67 = bitcast <2 x i32> % 66 to i64
  // and replace it with a shl 32
  struct Shl32Pattern : public Pattern {
    SSource sources[2];
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) { pass->Binary(EOPCODE_SHL, sources, modifier); }
  };

  if (m_Platform.hasPartialInt64Support() && I.getType()->isIntegerTy(64) && I.getOperand(0)->getType()->isVectorTy() &&
      cast<VectorType>(I.getOperand(0)->getType())->getElementType()->isIntegerTy(32)) {
    if (auto IEI = dyn_cast<InsertElementInst>(I.getOperand(0))) {
      auto vec = dyn_cast<ConstantVector>(IEI->getOperand(0));
      bool isCandidate =
          vec && vec->getNumOperands() == 2 && IsZero(vec->getOperand(0)) && isa<UndefValue>(vec->getOperand(1));
      auto index = dyn_cast<ConstantInt>(IEI->getOperand(2));
      isCandidate &= index && index->getZExtValue() == 1;
      if (isCandidate) {
        Shl32Pattern *Pat = new (m_allocator) Shl32Pattern();
        Pat->sources[0] = GetSource(IEI->getOperand(1), false, false, IsSourceOfSample(&I));
        Pat->sources[1] =
            GetSource(ConstantInt::get(Type::getInt32Ty(I.getContext()), 32), false, false, IsSourceOfSample(&I));
        AddPattern(Pat);
        return;
      }
    }
  }
  bool match = false;
  match = MatchRepack4i8(I) || MatchPack4i8(I) || MatchSingleInstruction(I);
  IGC_ASSERT_MESSAGE(match, "Unsupported BitCast instruction");
}

void CodeGenPatternMatch::visitIntToPtrInst(IntToPtrInst &I) { MatchSingleInstruction(I); }

void CodeGenPatternMatch::visitPtrToIntInst(PtrToIntInst &I) { MatchSingleInstruction(I); }

void CodeGenPatternMatch::visitAddrSpaceCast(AddrSpaceCastInst &I) { MatchSingleInstruction(I); }

void CodeGenPatternMatch::visitDbgInfoIntrinsic(DbgInfoIntrinsic &I) { MatchDbgInstruction(I); }

void CodeGenPatternMatch::visitExtractValueInst(ExtractValueInst &I) {
  bool Match = false;

  // Ignore the extract value instruction. Handled in the call inst.
  if (CallInst *call = dyn_cast<CallInst>(I.getOperand(0))) {
    if (call->isInlineAsm() && call->getType()->isStructTy()) {
      MarkAsSource(call, IsSourceOfSample(&I));
      return;
    }
  }

  Match = matchAddPair(&I) || matchSubPair(&I) || matchMulPair(&I) || matchPtrToPair(&I) || MatchSingleInstruction(I);

  IGC_ASSERT_MESSAGE(Match, "Unknown `extractvalue` instruction!");
}

bool CodeGenPatternMatch::matchAddPair(ExtractValueInst *Ex) {
  Value *V = Ex->getOperand(0);
  GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(V);
  if (!GII || GII->getIntrinsicID() != GenISAIntrinsic::GenISA_add_pair)
    return false;

  if (Ex->getNumIndices() != 1)
    return false;
  unsigned Idx = *Ex->idx_begin();
  if (Idx != 0 && Idx != 1)
    return false;

  struct AddPairPattern : public Pattern {
    GenIntrinsicInst *GII;
    SSource Sources[4]; // L0, H0, L1, H1
    virtual void Emit(EmitPass *Pass, const DstModifier &DstMod) { Pass->EmitAddPair(GII, Sources, DstMod); }
  };

  struct AddPairSubPattern : public Pattern {
    virtual void Emit(EmitPass *Pass, const DstModifier &Mod) {
      // DO NOTHING. Dummy pattern.
    }
  };

  auto [MI, New] = PairOutputMap.insert(std::make_pair(GII, PairOutputTy()));
  if (New) {
    AddPairPattern *Pat = new (m_allocator) AddPairPattern();
    Pat->GII = GII;
    Pat->Sources[0] = GetSource(GII->getOperand(0), false, false, IsSourceOfSample(Ex));
    Pat->Sources[1] = GetSource(GII->getOperand(1), false, false, IsSourceOfSample(Ex));
    Pat->Sources[2] = GetSource(GII->getOperand(2), false, false, IsSourceOfSample(Ex));
    Pat->Sources[3] = GetSource(GII->getOperand(3), false, false, IsSourceOfSample(Ex));
    AddPattern(Pat);
  } else {
    AddPairSubPattern *Pat = new (m_allocator) AddPairSubPattern();
    AddPattern(Pat);
  }
  if (Idx == 0)
    MI->second.first = Ex;
  else
    MI->second.second = Ex;

  return true;
}

bool CodeGenPatternMatch::matchSubPair(ExtractValueInst *Ex) {
  Value *V = Ex->getOperand(0);
  GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(V);
  if (!GII || GII->getIntrinsicID() != GenISAIntrinsic::GenISA_sub_pair)
    return false;

  if (Ex->getNumIndices() != 1)
    return false;
  unsigned Idx = *Ex->idx_begin();
  if (Idx != 0 && Idx != 1)
    return false;

  struct SubPairPattern : public Pattern {
    GenIntrinsicInst *GII;
    SSource Sources[4]; // L0, H0, L1, H1
    virtual void Emit(EmitPass *Pass, const DstModifier &DstMod) { Pass->EmitSubPair(GII, Sources, DstMod); }
  };

  struct SubPairSubPattern : public Pattern {
    virtual void Emit(EmitPass *Pass, const DstModifier &Mod) {
      // DO NOTHING. Dummy pattern.
    }
  };

  auto [MI, New] = PairOutputMap.insert(std::make_pair(GII, PairOutputTy()));
  if (New) {
    SubPairPattern *Pat = new (m_allocator) SubPairPattern();
    Pat->GII = GII;
    Pat->Sources[0] = GetSource(GII->getOperand(0), false, false, IsSourceOfSample(Ex));
    Pat->Sources[1] = GetSource(GII->getOperand(1), false, false, IsSourceOfSample(Ex));
    Pat->Sources[2] = GetSource(GII->getOperand(2), false, false, IsSourceOfSample(Ex));
    Pat->Sources[3] = GetSource(GII->getOperand(3), false, false, IsSourceOfSample(Ex));
    AddPattern(Pat);
  } else {
    SubPairSubPattern *Pat = new (m_allocator) SubPairSubPattern();
    AddPattern(Pat);
  }
  if (Idx == 0)
    MI->second.first = Ex;
  else
    MI->second.second = Ex;

  return true;
}

bool CodeGenPatternMatch::matchMulPair(ExtractValueInst *Ex) {
  Value *V = Ex->getOperand(0);
  GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(V);
  if (!GII || GII->getIntrinsicID() != GenISAIntrinsic::GenISA_mul_pair)
    return false;

  if (Ex->getNumIndices() != 1)
    return false;
  unsigned Idx = *Ex->idx_begin();
  if (Idx != 0 && Idx != 1)
    return false;

  struct MulPairPattern : public Pattern {
    GenIntrinsicInst *GII;
    SSource Sources[4]; // L0, H0, L1, H1
    virtual void Emit(EmitPass *Pass, const DstModifier &DstMod) { Pass->EmitMulPair(GII, Sources, DstMod); }
  };

  struct MulPairSubPattern : public Pattern {
    virtual void Emit(EmitPass *Pass, const DstModifier &Mod) {
      // DO NOTHING. Dummy pattern.
    }
  };

  auto [MI, New] = PairOutputMap.insert(std::make_pair(GII, PairOutputTy()));
  if (New) {
    MulPairPattern *Pat = new (m_allocator) MulPairPattern();
    Pat->GII = GII;
    Pat->Sources[0] = GetSource(GII->getOperand(0), false, false, IsSourceOfSample(Ex));
    Pat->Sources[1] = GetSource(GII->getOperand(1), false, false, IsSourceOfSample(Ex));
    Pat->Sources[2] = GetSource(GII->getOperand(2), false, false, IsSourceOfSample(Ex));
    Pat->Sources[3] = GetSource(GII->getOperand(3), false, false, IsSourceOfSample(Ex));
    AddPattern(Pat);
  } else {
    MulPairSubPattern *Pat = new (m_allocator) MulPairSubPattern();
    AddPattern(Pat);
  }
  if (Idx == 0)
    MI->second.first = Ex;
  else
    MI->second.second = Ex;

  return true;
}

bool CodeGenPatternMatch::matchPtrToPair(ExtractValueInst *Ex) {
  Value *V = Ex->getOperand(0);
  GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(V);
  if (!GII || GII->getIntrinsicID() != GenISAIntrinsic::GenISA_ptr_to_pair)
    return false;

  if (Ex->getNumIndices() != 1)
    return false;
  unsigned Idx = *Ex->idx_begin();
  if (Idx != 0 && Idx != 1)
    return false;

  struct PtrToPairPattern : public Pattern {
    GenIntrinsicInst *GII;
    SSource Sources[1]; // Ptr
    virtual void Emit(EmitPass *Pass, const DstModifier &DstMod) { Pass->EmitPtrToPair(GII, Sources, DstMod); }
  };

  struct PtrToPairSubPattern : public Pattern {
    virtual void Emit(EmitPass *Pass, const DstModifier &Mod) {
      // DO NOTHING. Dummy pattern.
    }
  };

  auto [MI, New] = PairOutputMap.insert(std::make_pair(GII, PairOutputTy()));
  if (New) {
    PtrToPairPattern *Pat = new (m_allocator) PtrToPairPattern();
    Pat->GII = GII;
    Pat->Sources[0] = GetSource(GII->getOperand(0), false, false, IsSourceOfSample(Ex));
    AddPattern(Pat);
  } else {
    PtrToPairSubPattern *Pat = new (m_allocator) PtrToPairSubPattern();
    AddPattern(Pat);
  }
  if (Idx == 0)
    MI->second.first = Ex;
  else
    MI->second.second = Ex;

  return true;
}

bool CodeGenPatternMatch::MatchAbsNeg(llvm::Instruction &I) {
  struct MovModifierPattern : public Pattern {
    SSource source;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) { pass->Mov(source, modifier); }
  };
  bool match = false;
  e_modifier mod{};
  Value *source = nullptr;
  if (GetModifier(I, mod, source)) {
    MovModifierPattern *pattern = new (m_allocator) MovModifierPattern();
    pattern->source = GetSource(source, mod, false, IsSourceOfSample(&I));
    match = true;
    AddPattern(pattern);
  }
  return match;
}

bool CodeGenPatternMatch::MatchFrc(llvm::BinaryOperator &I) {
  if (!ContractionAllowed(I))
    return false;
  struct FrcPattern : public Pattern {
    SSource source;
    void Emit(EmitPass *pass, const DstModifier &modifier) override { pass->Frc(source, modifier); }

    bool supportsSaturate() override { return false; }
  };
  IGC_ASSERT(I.getOpcode() == Instruction::FSub);
  llvm::Value *source0 = I.getOperand(0);
  llvm::IntrinsicInst *source1 = llvm::dyn_cast<llvm::IntrinsicInst>(I.getOperand(1));
  bool found = false;
  if (source1 && source1->getIntrinsicID() == Intrinsic::floor) {
    if (source1->getOperand(0) == source0) {
      found = true;
    }
  }
  if (found) {
    FrcPattern *pattern = new (m_allocator) FrcPattern();
    pattern->source = GetSource(source0, true, false, IsSourceOfSample(&I));
    AddPattern(pattern);
  }
  return found;
}

/*
below pass handles x - frac(x) = floor(x) pattern. Refer below :

frc (8|M0) r20.0<1>:f r19.0<8;8,1>:f {Compacted, @1}
add (8|M0) (ge)f0.1 r19.0<1>:f r19.0<8;8,1>:f -r20.0<8;8,1>:f {@1}
rndd (8|M0) r21.0<1>:f (abs)r19.0<8;8,1>:f {Compacted, @1}
*/

bool CodeGenPatternMatch::MatchFloor(llvm::BinaryOperator &I) {
  if (IGC_IS_FLAG_ENABLED(DisableMatchFloor)) {
    return false;
  }
  struct FloorPattern : public Pattern {
    SSource source;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) { pass->Floor(source, modifier); }
  };
  IGC_ASSERT(I.getOpcode() == Instruction::FSub);
  llvm::Value *source0 = I.getOperand(0);
  GenIntrinsicInst *source1 = dyn_cast<GenIntrinsicInst>(I.getOperand(1));
  bool found = false;
  if (source1 && source1->getIntrinsicID() == GenISAIntrinsic::GenISA_frc) {
    if (source1->getOperand(0) == source0) {
      found = true;
    }
  }
  if (found) {
    FloorPattern *pattern = new (m_allocator) FloorPattern();
    pattern->source = GetSource(source0, true, false, IsSourceOfSample(&I));
    AddPattern(pattern);
  }
  return found;
}

SSource CodeGenPatternMatch::GetSource(llvm::Value *value, bool modifier, bool regioning, bool isSampleDerivative) {
  llvm::Value *sourceValue = value;
  e_modifier mod = EMOD_NONE;
  if (modifier) {
    GetModifier(*sourceValue, mod, sourceValue);
  }
  return GetSource(sourceValue, mod, regioning, isSampleDerivative);
}

SSource CodeGenPatternMatch::GetSource(llvm::Value *value, e_modifier mod, bool regioning, bool isSampleDerivative) {
  SSource source;
  GetRegionModifier(source, value, regioning);
  source.value = value;
  source.mod = mod;
  MarkAsSource(value, isSampleDerivative);
  return source;
}

void CodeGenPatternMatch::MarkAsSource(llvm::Value *v, bool isSampleDerivative) {
  // update liveness of the sources
  if (IsConstOrSimdConstExpr(v)) {
    // skip constant
    return;
  }
  if (isa<Instruction>(v) || isa<Argument>(v)) {
    m_LivenessInfo->HandleVirtRegUse(v, m_root->getParent(), m_root);
  }
  // mark the source as used so that we know we need to generate this value
  if (llvm::Instruction *inst = llvm::dyn_cast<Instruction>(v)) {
    m_usedInstructions.insert(inst);
  }
  if (m_rootIsSubspanUse) {
    HandleSubspanUse(v, isSampleDerivative);
  }
}

bool CodeGenPatternMatch::IsSubspanUse(llvm::Value *v) { return m_subSpanUse.find(v) != m_subSpanUse.end(); }

bool CodeGenPatternMatch::IsSourceOfSample(llvm::Value *v) { return m_sampleSource.find(v) != m_sampleSource.end(); }

bool CodeGenPatternMatch::IsSourceOfSampleUnderCF(llvm::Value *v) {
  return m_sampleUnderCFsource.find(v) != m_sampleUnderCFsource.end();
}

bool CodeGenPatternMatch::IsPHISourceOfSampleUnderCF(llvm::Value *v) {
  return m_sampleUnderCFPHIsource.find(v) != m_sampleUnderCFPHIsource.end();
}

bool CodeGenPatternMatch::MatchFMA(llvm::IntrinsicInst &I) {
  struct FMAPattern : Pattern {
    SSource sources[3];
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) { pass->Mad(sources, modifier); }
  };

  FMAPattern *pattern = new (m_allocator) FMAPattern();
  for (int i = 0; i < 3; i++) {
    llvm::Value *V = I.getOperand(i);
    pattern->sources[i] = GetSource(V, true, false, IsSourceOfSample(&I));
    if (isa<Constant>(V) &&
        (!m_Platform.support16BitImmSrcForMad() || V->getType()->getTypeID() != llvm::Type::HalfTyID || i == 1)) {
      // CNL+ mad instruction allows 16 bit immediate for src0 and src2
      AddToConstantPool(I.getParent(), V);
      pattern->sources[i].fromConstantPool = true;
    }
  }
  AddPattern(pattern);

  return true;
}

bool CodeGenPatternMatch::MatchPredAdd(llvm::BinaryOperator &I) {
  struct PredAddPattern : Pattern {
    SSource sources[2];
    SSource pred;
    e_predMode predMode;
    bool invertPred;
    void Emit(EmitPass *pass, const DstModifier &modifier) override {
      DstModifier modf = modifier;
      modf.predMode = predMode;
      pass->PredAdd(pred, invertPred, sources, modf);
    }

    // Cannot use the ".sat" modifier as it is applied only to data lanes enabled by the predication
    // while saturation operation matched MatchFloatingPointSatModifier should be executed
    // irrespective of the predicate value matched by MatchPredAdd.
    bool supportsSaturate() override { return false; }
  };

  if (m_ctx->getModuleMetaData()->isPrecise) {
    return false;
  }

  if (m_ctx->type == ShaderType::VERTEX_SHADER || !m_ctx->m_DriverInfo.SupportMatchPredAdd()) {
    return false;
  }

  bool found = false;

  llvm::Value *sources[2] = {nullptr, nullptr};
  llvm::Value *pred = nullptr;
  e_modifier src_mod[2] = {e_modifier::EMOD_NONE, e_modifier::EMOD_NONE};
  e_modifier pred_mod = e_modifier::EMOD_NONE;
  bool invertPred = false;
  if (!ContractionAllowed(I) || IGC_IS_FLAG_ENABLED(DisableMatchPredAdd)) {
    return false;
  }

  // Skip the pattern match if FPTrunc/FPEXt is used right after fadd
  if (I.hasOneUse()) {
    FPTruncInst *FPTrunc = llvm::dyn_cast<llvm::FPTruncInst>(*I.user_begin());
    FPExtInst *FPExt = llvm::dyn_cast<llvm::FPExtInst>(*I.user_begin());

    if (FPTrunc || FPExt) {
      return false;
    }
  }

  IGC_ASSERT(I.getOpcode() == Instruction::FAdd || I.getOpcode() == Instruction::FSub);
  for (uint iAdd = 0; iAdd < 2 && !found; iAdd++) {
    Value *src = I.getOperand(iAdd);
    llvm::BinaryOperator *mul = llvm::dyn_cast<llvm::BinaryOperator>(src);
    if (mul && mul->getOpcode() == Instruction::FMul) {
      if (!mul->hasOneUse() || !ContractionAllowed(*mul)) {
        continue;
      }

      for (uint iMul = 0; iMul < 2; iMul++) {
        if (llvm::SelectInst *selInst = dyn_cast<SelectInst>(mul->getOperand(iMul))) {
          ConstantFP *C1 = dyn_cast<ConstantFP>(selInst->getOperand(1));
          ConstantFP *C2 = dyn_cast<ConstantFP>(selInst->getOperand(2));
          if (C1 && C2 && selInst->hasOneUse()) {
            // select i1 %res_s48, float 1.000000e+00, float 0.000000e+00
            if ((C2->isZero() && C1->isExactlyValue(1.f))) {
              invertPred = false;
            }
            // select i1 %res_s48, float 0.000000e+00, float 1.000000e+00
            else if (C1->isZero() && C2->isExactlyValue(1.f)) {
              invertPred = true;
            } else {
              continue;
            }
          } // if (C1 && C2 && selInst->hasOneUse())
          else {
            continue;
          }

          //   % 97 = select i1 %res_s48, float 1.000000e+00, float 0.000000e+00
          //   %102 = fmul fast float %97 %98

          // case 1 (add)
          // Before match
          //   %105 = fadd %102 %103
          // After match
          //              %105 = %103
          //   (%res_s48) %105 = fadd %105 %98

          // case 2 (fsub match @ iAdd = 0)
          // Before match
          //   %105 = fsub %102 %103
          // After match
          //              %105 = -%103
          //   (%res_s48) %105 = fadd %105 %98

          // case 3 (fsub match @ iAdd = 1)
          // Before match
          //   %105 = fsub %103 %102
          // After match
          //              %105 = %103
          //   (%res_s48) %98 = fadd %105 -%98

          // sources[0]: store add operand (i.e. %103 above)
          // sources[1]: store mul operand (i.e. %98 above)

          sources[0] = I.getOperand(1 ^ iAdd);
          sources[1] = mul->getOperand(1 ^ iMul);
          pred = selInst->getOperand(0);

          GetModifier(*sources[0], src_mod[0], sources[0]);
          GetModifier(*sources[1], src_mod[1], sources[1]);
          GetModifier(*pred, pred_mod, pred);

          if (I.getOpcode() == Instruction::FSub) {
            src_mod[iAdd] = CombineModifier(EMOD_NEG, src_mod[iAdd]);
          }

          found = true;
          break;
        } //  if (llvm::SelectInst* selInst = dyn_cast<SelectInst>(mul->getOperand(iMul)))
      } // for (uint iMul = 0; iMul < 2; iMul++)
    } // if (mul && mul->getOpcode() == Instruction::FMul)
  } // for (uint iAdd = 0; iAdd < 2; iAdd++)

  if (found) {
    PredAddPattern *pattern = new (m_allocator) PredAddPattern();
    pattern->predMode = EPRED_NORMAL;
    pattern->sources[0] = GetSource(sources[0], src_mod[0], false, IsSourceOfSample(&I));
    pattern->sources[1] = GetSource(sources[1], src_mod[1], false, IsSourceOfSample(&I));
    pattern->pred = GetSource(pred, pred_mod, false, IsSourceOfSample(&I));
    pattern->invertPred = invertPred;

    AddPattern(pattern);
  }
  return found;
}

// we match the following pattern
// %c = fcmp %1 %2
// %g = sext %c to i32
// %h = and i32 %g 1065353216
// %m = bitcast i32 %h to float
// %p = fadd %m %n
bool CodeGenPatternMatch::MatchSimpleAdd(llvm::BinaryOperator &I) {
  struct SimpleAddPattern : public Pattern {
    SSource sources[2];
    SSource pred;

    e_predMode predMode;
    bool invertPred;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      DstModifier modf = modifier;
      modf.predMode = predMode;
      pass->PredAdd(pred, invertPred, sources, modf);
    }
  };

  IGC_ASSERT(I.getOpcode() == Instruction::FAdd);

  if (IGC_IS_FLAG_ENABLED(DisableMatchSimpleAdd)) {
    return false;
  }

  unsigned int repAddOperand = 0;
  BitCastInst *bitCastInst0 = llvm::dyn_cast<llvm::BitCastInst>(I.getOperand(0));
  BitCastInst *bitCastInst1 = llvm::dyn_cast<llvm::BitCastInst>(I.getOperand(1));
  BitCastInst *bitCastInst = NULL;

  for (auto UI = I.user_begin(); UI != I.user_end(); ++UI) {
    if (isa<GenIntrinsicInst>(*UI, GenISAIntrinsic::GenISA_fsat)) {
      return false;
    }
  }

  if (!bitCastInst0 && !bitCastInst1) {
    return false;
  }

  if (bitCastInst1) {
    bitCastInst = bitCastInst1;
    repAddOperand = 1;
  } else {
    bitCastInst = bitCastInst0;
    repAddOperand = 0;
  }

  if (!bitCastInst->hasOneUse()) {
    return false;
  }

  Instruction *andInst = dyn_cast<Instruction>(bitCastInst->getOperand(0));
  if (!andInst || (andInst->getOpcode() != Instruction::And)) {
    return false;
  }

  // check %h = and i32 %g 1065353216
  if (!andInst->getType()->isIntegerTy(32)) {
    return false;
  }

  ConstantInt *CInt = dyn_cast<ConstantInt>(andInst->getOperand(1));
  if (!CInt || (CInt->getZExtValue() != 0x3f800000)) {
    return false;
  }

  SExtInst *SExt = llvm::dyn_cast<llvm::SExtInst>(andInst->getOperand(0));
  if (!SExt) {
    return false;
  }

  CmpInst *cmp = llvm::dyn_cast<CmpInst>(SExt->getOperand(0));
  if (!cmp) {
    return false;
  }

  // match found
  SimpleAddPattern *pattern = new (m_allocator) SimpleAddPattern();
  llvm::Value *sources[2], *pred;
  e_modifier src_mod[2], pred_mod;

  sources[0] = I.getOperand(1 - repAddOperand);
  sources[1] = ConstantFP::get(I.getType(), 1.0);
  pred = cmp;

  GetModifier(*sources[0], src_mod[0], sources[0]);
  GetModifier(*sources[1], src_mod[1], sources[1]);
  GetModifier(*pred, pred_mod, pred);

  pattern->predMode = EPRED_NORMAL;
  pattern->sources[0] = GetSource(sources[0], src_mod[0], false, IsSourceOfSample(&I));
  pattern->sources[1] = GetSource(sources[1], src_mod[1], false, IsSourceOfSample(&I));
  pattern->pred = GetSource(pred, pred_mod, false, IsSourceOfSample(&I));
  pattern->invertPred = false;
  AddPattern(pattern);

  return true;
}

bool CodeGenPatternMatch::MatchMad(llvm::BinaryOperator &I) {
  struct MadPattern : Pattern {
    SSource sources[3];
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      if (IGC_IS_FLAG_ENABLED(EnableVectorEmitter) && sources[0].value->getType()->isVectorTy() &&
          sources[1].value->getType()->isVectorTy() && sources[2].value->getType()->isVectorTy())
        pass->VectorMad(sources, modifier);
      else
        pass->Mad(sources, modifier);
    }
  };

  auto isFpMad = [](const Instruction &I) {
    auto vecType = llvm::dyn_cast<FixedVectorType>(I.getType());
    if (!vecType)
      return I.getType()->isFloatingPointTy();

    bool isFPType = vecType->getElementType()->isFloatingPointTy() && IGC_IS_FLAG_ENABLED(VectorizerAllowFMADMatching);
    return isFPType;
  };

  if (isFpMad(I) &&
      (m_ctx->getModuleMetaData()->isPrecise || m_ctx->getModuleMetaData()->compOpt.disableMathRefactoring)) {
    return false;
  }
  if (m_ctx->type == ShaderType::VERTEX_SHADER && m_ctx->m_DriverInfo.DisabeMatchMad()) {
    return false;
  }

  if (bool allow = m_ctx->getModuleMetaData()->allowMatchMadOptimizationforVS ||
                   IGC_IS_FLAG_ENABLED(WaAllowMatchMadOptimizationforVS);
      m_ctx->type == ShaderType::VERTEX_SHADER && m_ctx->m_DriverInfo.PreventZFighting() && !allow) {
    if (m_PosDep->PositionDependsOnInst(&I)) {
      return false;
    }
  }

  if (m_ctx->type == ShaderType::COMPUTE_SHADER && m_ctx->getModuleMetaData()->disableMatchMadOptimizationForCS) {
    return false;
  }

  if (IGC_IS_FLAG_ENABLED(DisableMatchMad)) {
    return false;
  }

  bool isFpMadWithContractionOverride = false;
  if (isFpMad(I) && m_AllowContractions == false) {
    if (I.hasAllowContract() && m_ctx->m_DriverInfo.RespectPerInstructionContractFlag()) {
      isFpMadWithContractionOverride = true;
    } else {
      return false;
    }
  }
  if (!isFpMad(I) && !(m_Platform.doIntegerMad() && m_ctx->m_DriverInfo.EnableIntegerMad())) {
    return false;
  }

  bool found = false;
  llvm::Value *sources[3];
  e_modifier src_mod[3];

  IGC_ASSERT(I.getOpcode() == Instruction::FAdd || I.getOpcode() == Instruction::FSub ||
             I.getOpcode() == Instruction::Add || I.getOpcode() == Instruction::Sub);
  if (I.getOperand(0) != I.getOperand(1)) {
    for (uint i = 0; i < 2; i++) {
      Value *src = SkipCanonicalize(I.getOperand(i));
      if (FPExtInst *fpextInst = llvm::dyn_cast<llvm::FPExtInst>(src)) {
        if (!m_Platform.supportMixMode() && fpextInst->getSrcTy()->getTypeID() == llvm::Type::HalfTyID) {
          // no mix mode instructions
        } else if (fpextInst->getSrcTy()->getTypeID() != llvm::Type::DoubleTyID &&
                   fpextInst->getDestTy()->getTypeID() != llvm::Type::DoubleTyID) {
          src = fpextInst->getOperand(0);
        }
      }
      llvm::BinaryOperator *mul = llvm::dyn_cast<llvm::BinaryOperator>(src);

      if (mul && (mul->getOpcode() == Instruction::FMul || mul->getOpcode() == Instruction::Mul)) {
        // in case we know we won't be able to remove the mul we don't merge it
        if (!m_PosDep->PositionDependsOnInst(mul) && NeedInstruction(*mul))
          continue;

        if (isFpMadWithContractionOverride && !mul->hasAllowContract())
          continue;

        sources[2] = SkipCanonicalize(I.getOperand(1 - i));
        sources[1] = SkipCanonicalize(mul->getOperand(0));
        sources[0] = SkipCanonicalize(mul->getOperand(1));

        GetModifier(*sources[0], src_mod[0], sources[0]);
        GetModifier(*sources[1], src_mod[1], sources[1]);
        GetModifier(*sources[2], src_mod[2], sources[2]);

        sources[0] = SkipCanonicalize(sources[0]);
        sources[1] = SkipCanonicalize(sources[1]);
        sources[2] = SkipCanonicalize(sources[2]);
        if (I.getOpcode() == Instruction::FSub || I.getOpcode() == Instruction::Sub) {
          if (i == 0) {
            src_mod[2] = CombineModifier(EMOD_NEG, src_mod[2]);
          } else {
            if (llvm::isa<llvm::Constant>(sources[0])) {
              src_mod[1] = CombineModifier(EMOD_NEG, src_mod[1]);
            } else {
              src_mod[0] = CombineModifier(EMOD_NEG, src_mod[0]);
            }
          }
        }
        found = true;
        break;
      }
    }
  }

  // Check integer mad profitability.
  if (found && !isFpMad(I)) {
    uint8_t numConstant = 0;
    for (int i = 0; i < 3; i++) {
      if (isa<Constant>(sources[i]))
        numConstant++;

      // Only one immediate is supported
      if (numConstant > 1)
        return false;
    }

    auto isByteOrWordValue = [](Value *V) {
      if (isa<ConstantInt>(V)) {
        // only 16-bit int immediate is supported
        APInt val = cast<ConstantInt>(V)->getValue();
        return val.sge(SHRT_MIN) && val.sle(SHRT_MAX);
      }
      // Trace the def-use chain and return the first non up-cast related value.
      while (isa<ZExtInst>(V) || isa<SExtInst>(V) || isa<BitCastInst>(V))
        V = cast<Instruction>(V)->getOperand(0);
      const unsigned DWordSizeInBits = 32;
      return V->getType()->getScalarSizeInBits() < DWordSizeInBits;
    };

    // One multiplicant should be *W or *B.
    if (!isByteOrWordValue(sources[0]) &&
        !isByteOrWordValue(sources[1])
    )
      return false;

    auto isQWordValue = [](Value *V) {
      while (isa<ZExtInst>(V) || isa<SExtInst>(V) || isa<BitCastInst>(V))
        V = cast<Instruction>(V)->getOperand(0);
      Type *T = V->getType();
      return (T->isIntegerTy() && T->getScalarSizeInBits() == 64);
    };

    // Mad instruction doesn't support QW type
    if (isQWordValue(sources[0]) || isQWordValue(sources[1]))
      return false;
  }

  if (found) {
    MadPattern *pattern = new (m_allocator) MadPattern();
    for (int i = 0; i < 3; i++) {
      pattern->sources[i] = GetSource(sources[i], src_mod[i], false, IsSourceOfSample(&I));
      if (isa<Constant>(sources[i]) &&
          (!m_Platform.support16BitImmSrcForMad() ||
           (!sources[i]->getType()->isHalfTy() && !sources[i]->getType()->isIntegerTy()) || i == 1)) {
        // CNL+ mad instruction allows 16 bit immediate for src0 and src2
        AddToConstantPool(I.getParent(), sources[i]);
        pattern->sources[i].fromConstantPool = true;
      }
    }
    AddPattern(pattern);
  }
  return found;
}

// match simdblockRead/Write with preceding inttoptr if possible
// to save a copy move
bool CodeGenPatternMatch::MatchBlockReadWritePointer(llvm::GenIntrinsicInst &I) {
  struct BlockReadWritePointerPattern : public Pattern {
    GenIntrinsicInst *inst;
    Value *offset;
    explicit BlockReadWritePointerPattern(GenIntrinsicInst *I, Value *ofst) : inst(I), offset(ofst) {}
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      switch (inst->getIntrinsicID()) {
      case GenISAIntrinsic::GenISA_simdBlockRead:
        pass->emitSimdBlockRead(inst, offset);
        break;
      case GenISAIntrinsic::GenISA_simdBlockWrite:
        pass->emitSimdBlockWrite(inst, offset);
        break;
      default:
        IGC_ASSERT_MESSAGE(0, "unexpected intrinsic");
        break;
      }
    }
  };

  if (I.getIntrinsicID() != GenISAIntrinsic::GenISA_simdBlockRead &&
      I.getIntrinsicID() != GenISAIntrinsic::GenISA_simdBlockWrite) {
    return false;
  }

  // check the address is inttoptr with same dst and src type width
  auto ptrVal = I.getOperand(0);
  auto I2P = dyn_cast<IntToPtrInst>(ptrVal);
  if (I2P && I2P->getOperand(0)->getType()->getIntegerBitWidth() ==
                 m_ctx->getRegisterPointerSizeInBits(I2P->getAddressSpace())) {
    auto addrBase = I2P->getOperand(0);
    BlockReadWritePointerPattern *pattern = new (m_allocator) BlockReadWritePointerPattern(&I, addrBase);
    MarkAsSource(addrBase, IsSourceOfSample(&I));
    // for write mark data ptr as well
    if (I.getIntrinsicID() == GenISAIntrinsic::GenISA_simdBlockWrite) {
      MarkAsSource(I.getOperand(1), IsSourceOfSample(&I));
    }

    AddPattern(pattern);
    return true;
  }
  return false;
}


// Pattern matching to detect and handle immediate offsets in load/store
// instructions.  It detects offsets of the form "add dst, var, imm"
// The add instruction is removed, and imm included in the LSC message descriptor.
//
// OpenCL Load/Store launder the address through an inttoptr....
//   %2 = add i32 %0, 64
//   %3 = inttoptr i32 %2 to i32 addrspace(131072)*
//   store i32 16, i32 addrspace(131072)* %3, align 8
// Fold to vISA lsc_store... [%0 + 64].
//
// Stuff like 3D GenISA_ldrawvector_indexed directly reference the add.
//   %723 = add i32 %713, 16
//   %724 = call <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621448v4f32(
//            <4 x float> addrspace(2621448)* %runtime_value_1, i32 %723, i32 4, i1 false)
// Fold to lsc_load [%713 + 16].
static uint GetNbSources(llvm::Instruction &v) {
  if (llvm::CallInst *intrin = llvm::dyn_cast<llvm::CallInst>(&v)) {
    return IGCLLVM::getNumArgOperands(intrin);
  }
  return v.getNumOperands();
}

// Returns true if the value is an integer constant that can be encoded in
// 32 bits.
inline bool Is32BitConstInt(llvm::Value *val) {
  llvm::ConstantInt *imm = llvm::dyn_cast<ConstantInt>(val);
  if (imm && imm->getSExtValue() <= std::numeric_limits<int32_t>::max() &&
      imm->getSExtValue() >= std::numeric_limits<int32_t>::min()) {
    return true;
  }
  return false;
}

bool CodeGenPatternMatch::MatchImmOffsetLSC(llvm::Instruction &I) {
  struct LSCImmOffsetPattern : public Pattern {
    explicit LSCImmOffsetPattern(Instruction *I, llvm::Value *varOffset, llvm::ConstantInt *immOffset)
        : m_inst(I), m_varOff(varOffset), m_immOff(immOffset) {}

    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      if (isa<LoadInst>(m_inst)) {
        pass->emitLoad(cast<LoadInst>(m_inst), m_varOff, m_immOff);
      } else if (isa<StoreInst>(m_inst)) {
        pass->emitStore(cast<StoreInst>(m_inst), m_varOff, m_immOff);
      } else if (isa<LdRawIntrinsic>(m_inst)) {
        pass->emitLoadRawIndexed(cast<LdRawIntrinsic>(m_inst), m_varOff, nullptr, m_immOff);
      } else if (isa<StoreRawIntrinsic>(m_inst)) {
        pass->emitStoreRawIndexed(cast<StoreRawIntrinsic>(m_inst), m_varOff, nullptr, m_immOff);
      } else {
        IGC_ASSERT_MESSAGE(false, "unmatched imm off pattern");
      }
    }

  private:
    llvm::Instruction *m_inst;
    llvm::Value *m_varOff;
    llvm::ConstantInt *m_immOff;
  }; // LSCImmOffsetPattern

  // match:
  //   %addr = (add/sub %var, %imm) | (add %imm, %var)
  //   [ %addr = inttoptr %addr, ... ]
  //   load/store/whatever ... %addr
  int addrOpnd = -1;
  if (llvm::isa<LoadInst>(&I) || llvm::isa<StoreInst>(&I)) {
    // buffer load/store instructions in compute languages are usually
    // buried behind a inttoptr op
    addrOpnd = llvm::isa<LoadInst>(&I) ? 0 : 1;
  } else if (llvm::isa<llvm::GenIntrinsicInst>(I)) {
    llvm::GenIntrinsicInst &GI = llvm::cast<llvm::GenIntrinsicInst>(I);
    switch (GI.getIntrinsicID()) {
    // TODO: incrementally enable others
    //
    // case GenISAIntrinsic::GenISA_intatomicraw:
    // case GenISAIntrinsic::GenISA_floatatomicraw:
    // case GenISAIntrinsic::GenISA_intatomicrawA64:
    // case GenISAIntrinsic::GenISA_floatatomicrawA64:
    // case GenISAIntrinsic::GenISA_icmpxchgatomicraw:
    // case GenISAIntrinsic::GenISA_icmpxchgatomicrawA64:
    // case GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64:
    //  all these will target => emitAtomicRaw
    //
    case GenISAIntrinsic::GenISA_ldrawvector_indexed:
    case GenISAIntrinsic::GenISA_ldraw_indexed:
    case GenISAIntrinsic::GenISA_storerawvector_indexed:
    case GenISAIntrinsic::GenISA_storeraw_indexed:
      addrOpnd = 1;
      break;
      // return false;
    // TODO: are these even reachable here???
    // case GenISAIntrinsic::GenISA_ldstructured:
    // case GenISAIntrinsic::GenISA_storestructured1:
    // case GenISAIntrinsic::GenISA_storestructured2:
    // case GenISAIntrinsic::GenISA_storestructured3:
    // case GenISAIntrinsic::GenISA_storestructured4:
    //    addrOpnd = 2;
    //    break;
    default:
      return false;
    }
  } else {
    return false;
  }

  if (addrOpnd < 0) {
    return false;
  }

  llvm::Instruction *addSubInst = llvm::dyn_cast<llvm::Instruction>(I.getOperand((unsigned)addrOpnd));
  if (!addSubInst) {
    // e.g. the address is a constant
    return false;
  }

  Type *addInstType = addSubInst->getType();
  // Note that the A64 addressing mode can be only connected with load and store instructions
  bool isA64AddressingModel = addInstType->isPointerTy() && IGC::isA64Ptr(cast<PointerType>(addInstType), m_ctx);

  llvm::Instruction *intToPtrInst = nullptr;
  if (addSubInst->getOpcode() == llvm::Instruction::IntToPtr) {
    intToPtrInst = addSubInst;
    addSubInst = llvm::dyn_cast<llvm::Instruction>(addSubInst->getOperand(0));
  }
  if (!addSubInst) {
    return false; // e.g. intToPtr of constant
  } else if (addSubInst->getOpcode() != llvm::Instruction::Add && addSubInst->getOpcode() != llvm::Instruction::Sub) {
    return false;
  }

  // VISA interface accepts 32-bit immediate offset values
  const bool isConstant0 = Is32BitConstInt(addSubInst->getOperand(0));
  const bool isConstant1 = Is32BitConstInt(addSubInst->getOperand(1));
  if (isConstant1 || (isConstant0 && addSubInst->getOpcode() == llvm::Instruction::Add)) {
    // YES: var + imm
    // YES: var - imm
    // YES: imm + var
    // NO:  imm - var  (IGC drops the negation otherwise)
    IGC_ASSERT_MESSAGE(!isConstant0 || !isConstant1,
                       "Both operands are immediate - constants should be folded elsewhere.");

    llvm::Value *varOffset = isConstant0 ? addSubInst->getOperand(1) : addSubInst->getOperand(0);

    if (!isA64AddressingModel && m_ctx->type != ShaderType::OPENCL_SHADER && IGC_GET_FLAG_VALUE(LscImmOffsMatch) < 2)
      return false;

    // HW does an early bounds check on varOffset for A32 messages. Thus, if varOffset
    // is negative, then the bounds check fails early even though the immediate offset
    // would bring the final calculation to a positive number.
    bool disableA32ImmediateGlobalBaseOffset =
        !isA64AddressingModel && !UsedWithoutImmInMemInst(varOffset) && !valueIsPositive(varOffset, m_DL) &&
        IGC_GET_FLAG_VALUE(LscImmOffsMatch) < 3;

    if (disableA32ImmediateGlobalBaseOffset)
      return false;

    MarkAsSource(varOffset, IsSourceOfSample(&I));

    unsigned numSources = GetNbSources(I);
    for (unsigned i = 0; i < numSources; i++) {
      if (I.getOperand(i) != intToPtrInst && I.getOperand(i) != addSubInst) {
        MarkAsSource(I.getOperand(i), IsSourceOfSample(&I));
      }
    }

    llvm::Value *immOffset = isConstant0 ? addSubInst->getOperand(0) : addSubInst->getOperand(1);

    LSCImmOffsetPattern *pattern =
        new (m_allocator) LSCImmOffsetPattern(&I, varOffset, llvm::dyn_cast<llvm::ConstantInt>(immOffset));
    AddPattern(pattern);
    return true;
  }

  return false;
}

bool CodeGenPatternMatch::MatchLrp(llvm::BinaryOperator &I) {
  struct LRPPattern : public Pattern {
    SSource sources[3];
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) { pass->Lrp(sources, modifier); }
  };

  if (!I.getType()->isFloatTy())
    return false;
  if (!m_Platform.supportLRPInstruction())
    return false;

  if (m_ctx->getModuleMetaData()->isPrecise) {
    return false;
  }

  bool found = false;
  llvm::Value *sources[3];
  e_modifier src_mod[3];

  if (!ContractionAllowed(I)) {
    return false;
  }

  IGC_ASSERT((I.getOpcode() == Instruction::FAdd) || (I.getOpcode() == Instruction::FSub));

  bool startPatternIsAdd = false;
  if (I.getOpcode() == Instruction::FAdd) {
    startPatternIsAdd = true;
  }

  // match the case: dst = src0 (src1 - src2)  + src2;
  for (uint i = 0; i < 2; i++) {
    llvm::BinaryOperator *mul = llvm::dyn_cast<llvm::BinaryOperator>(I.getOperand(i));
    if (mul && mul->getOpcode() == Instruction::FMul && ContractionAllowed(*mul)) {
      for (uint j = 0; j < 2; j++) {
        llvm::BinaryOperator *sub = llvm::dyn_cast<llvm::BinaryOperator>(mul->getOperand(j));
        if (sub && ContractionAllowed(*sub)) {
          llvm::ConstantFP *zero = llvm::dyn_cast<llvm::ConstantFP>(sub->getOperand(0));
          if (zero && zero->isExactlyValue(0.f)) {
            // in this case we can optimize the pattern into fmad and give better result
            continue;
          }

          if ((startPatternIsAdd && sub->getOpcode() == Instruction::FSub) ||
              (!startPatternIsAdd && i == 0 && sub->getOpcode() == Instruction::FAdd)) {
            if (sub->getOperand(1) == I.getOperand(1 - i) && mul->getOperand(0) != mul->getOperand(1)) {
              sources[0] = mul->getOperand(1 - j);
              sources[1] = sub->getOperand(0);
              sources[2] = sub->getOperand(1);
              GetModifier(*sources[0], src_mod[0], sources[0]);
              GetModifier(*sources[1], src_mod[1], sources[1]);
              GetModifier(*sources[2], src_mod[2], sources[2]);

              if (!startPatternIsAdd && i == 0) {
                // handle patterns like this:
                // dst = src0 (src1 + src2) - src2;
                src_mod[2] = CombineModifier(EMOD_NEG, src_mod[2]);
              }

              found = true;
              break;
            }
          }
        }
      }
    }
    if (found) {
      break;
    }
  }

  // match the case: dst = src0 * src1 + src2 * (1.0 - src0);
  if (!found) {
    llvm::BinaryOperator *mul[2];
    mul[0] = llvm::dyn_cast<llvm::BinaryOperator>(I.getOperand(0));
    mul[1] = llvm::dyn_cast<llvm::BinaryOperator>(I.getOperand(1));
    if (mul[0] && mul[0]->getOpcode() == Instruction::FMul && ContractionAllowed(*mul[0]) && mul[1] &&
        mul[1]->getOpcode() == Instruction::FMul && ContractionAllowed(*mul[1]) &&
        !llvm::isa<llvm::ConstantFP>(mul[0]->getOperand(0)) && !llvm::isa<llvm::ConstantFP>(mul[0]->getOperand(1)) &&
        !llvm::isa<llvm::ConstantFP>(mul[1]->getOperand(0)) && !llvm::isa<llvm::ConstantFP>(mul[1]->getOperand(1))) {
      for (uint i = 0; i < 2; i++) {
        for (uint j = 0; j < 2; j++) {
          llvm::BinaryOperator *sub = llvm::dyn_cast<llvm::BinaryOperator>(mul[i]->getOperand(j));
          if (sub && sub->getOpcode() == Instruction::FSub && ContractionAllowed(*sub)) {
            llvm::ConstantFP *one = llvm::dyn_cast<llvm::ConstantFP>(sub->getOperand(0));
            if (one && one->isExactlyValue(1.f)) {
              for (uint k = 0; k < 2; k++) {
                if (sub->getOperand(1) == mul[1 - i]->getOperand(k)) {
                  sources[0] = sub->getOperand(1);
                  sources[1] = mul[1 - i]->getOperand(1 - k);
                  sources[2] = mul[i]->getOperand(1 - j);
                  GetModifier(*sources[0], src_mod[0], sources[0]);
                  GetModifier(*sources[1], src_mod[1], sources[1]);
                  GetModifier(*sources[2], src_mod[2], sources[2]);
                  if (!startPatternIsAdd) {
                    if (i == 1) {
                      // handle patterns like this:
                      // dst = (src1 * src0) - (src2 * (1.0 - src0))
                      src_mod[2] = CombineModifier(EMOD_NEG, src_mod[2]);
                    } else {
                      // handle patterns like this:
                      // dst = (src2 * (1.0 - src0)) - (src1 * src0)
                      src_mod[1] = CombineModifier(EMOD_NEG, src_mod[1]);
                    }
                  }
                  found = true;
                  break;
                }
              }
            }
          }
          if (found) {
            break;
          }
        }
        if (found) {
          break;
        }
      }
    }
  }

  if (!found) {
    // match the case: dst = src2 - (src0 * src2) + (src0 * src1);
    // match the case: dst = (src0 * src1) + src2 - (src0 * src2);
    // match the case: dst = src2 + (src0 * src1) - (src0 * src2);
    if (I.getOpcode() == Instruction::FAdd || I.getOpcode() == Instruction::FSub) {
      // dst = op[0] +/- op[1] +/- op[2]
      llvm::Instruction *op[3];
      llvm::Instruction *addSub1 = llvm::dyn_cast<llvm::Instruction>(I.getOperand(0));
      if (addSub1 && (addSub1->getOpcode() == Instruction::FSub || addSub1->getOpcode() == Instruction::FAdd)) {
        op[0] = llvm::dyn_cast<llvm::Instruction>(addSub1->getOperand(0));
        op[1] = llvm::dyn_cast<llvm::Instruction>(addSub1->getOperand(1));
        op[2] = llvm::dyn_cast<llvm::Instruction>(I.getOperand(1));

        if (op[0] && op[1] && op[2]) {
          for (uint casei = 0; casei < 3; casei++) {
            // i, j, k are the index for op[]
            uint i = (casei == 2 ? 1 : 0);
            uint j = (casei == 0 ? 1 : 2);
            uint k = 2 - casei;

            // op[i] and op[j] should be fMul, and op[k] is src2
            if (op[i]->getOpcode() == Instruction::FMul && op[j]->getOpcode() == Instruction::FMul &&
                ContractionAllowed(*op[i]) && ContractionAllowed(*op[j])) {
              for (uint srci = 0; srci < 2; srci++) {
                for (uint srcj = 0; srcj < 2; srcj++) {
                  // op[i] and op[j] needs to have one common source. this common source will be src0
                  if (op[i]->getOperand(srci) == op[j]->getOperand(srcj)) {
                    // one of the non-common source from op[i] and op[j] needs to be the same as op[k], which is src2
                    if (op[k] == op[i]->getOperand(1 - srci) || op[k] == op[j]->getOperand(1 - srcj)) {
                      // disable if any of the sources is immediate
                      if (llvm::isa<llvm::ConstantFP>(op[i]->getOperand(srci)) ||
                          llvm::isa<llvm::ConstantFP>(op[i]->getOperand(1 - srci)) ||
                          llvm::isa<llvm::ConstantFP>(op[j]->getOperand(srcj)) ||
                          llvm::isa<llvm::ConstantFP>(op[j]->getOperand(1 - srcj)) ||
                          llvm::isa<llvm::ConstantFP>(op[k])) {
                        break;
                      }

                      // check the add/sub cases and add negate to the sources when needed.
                      /*
                      ( src0src1, -src0src2, src2 )   okay
                      ( src0src1, -src0src2, -src2 )  skip
                      ( src0src1, src0src2, src2 )    skip
                      ( src0src1, src0src2, -src2 )   negate src2
                      ( -src0src1, -src0src2, src2 )  negate src1
                      ( -src0src1, -src0src2, -src2 ) skip
                      ( -src0src1, src0src2, src2 )   skip
                      ( -src0src1, src0src2, -src2 )  negate src1 src2
                      */

                      bool SignPositiveOp[3];
                      SignPositiveOp[0] = true;
                      SignPositiveOp[1] = (addSub1->getOpcode() == Instruction::FAdd);
                      SignPositiveOp[2] = (I.getOpcode() == Instruction::FAdd);

                      uint mulSrc0Src1Index = op[k] == op[i]->getOperand(1 - srci) ? j : i;
                      uint mulSrc0Src2Index = op[k] == op[i]->getOperand(1 - srci) ? i : j;

                      if (SignPositiveOp[mulSrc0Src2Index] == SignPositiveOp[k]) {
                        // abort the cases marked as "skip" in the comment above
                        break;
                      }

                      sources[0] = op[i]->getOperand(srci);
                      sources[1] = op[k] == op[i]->getOperand(1 - srci) ? op[j]->getOperand(1 - srcj)
                                                                        : op[i]->getOperand(1 - srci);
                      sources[2] = op[k];
                      GetModifier(*sources[0], src_mod[0], sources[0]);
                      GetModifier(*sources[1], src_mod[1], sources[1]);
                      GetModifier(*sources[2], src_mod[2], sources[2]);

                      if (SignPositiveOp[mulSrc0Src1Index] == false) {
                        src_mod[1] = CombineModifier(EMOD_NEG, src_mod[1]);
                      }
                      if (SignPositiveOp[k] == false) {
                        src_mod[2] = CombineModifier(EMOD_NEG, src_mod[2]);
                      }

                      found = true;
                    }
                  }
                }
                if (found) {
                  break;
                }
              }
            }
            if (found) {
              break;
            }
          }
        }
      }
    }
  }

  if (found) {
    LRPPattern *pattern = new (m_allocator) LRPPattern();
    for (int i = 0; i < 3; i++) {
      pattern->sources[i] = GetSource(sources[i], src_mod[i], false, IsSourceOfSample(&I));
    }
    AddPattern(pattern);
  }
  return found;
}

bool CodeGenPatternMatch::MatchCmpSext(llvm::Instruction &I) {
  /*
      %res_s42 = icmp eq i32 %src1_s41, 0
      %17 = sext i1 %res_s42 to i32
          to
      %res_s42 (i32) = icmp eq i32 %src1_s41, 0


      %res_s73 = fcmp oge float %res_s61, %42
      %46 = sext i1 %res_s73 to i32
          to
      %res_s73 (i32) = fcmp oge float %res_s61, %42
  */

  struct CmpSextPattern : Pattern {
    llvm::CmpInst *inst;
    SSource sources[2];
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      pass->Cmp(inst->getPredicate(), sources, modifier);
    }
  };
  bool match = false;

  if (CmpInst *cmpInst = dyn_cast<CmpInst>(I.getOperand(0))) {
    if (cmpInst->getOperand(0)->getType()->getPrimitiveSizeInBits() == I.getType()->getPrimitiveSizeInBits()) {
      CmpSextPattern *pattern = new (m_allocator) CmpSextPattern();
      bool supportModifier = SupportsModifier(cmpInst, m_Platform);

      pattern->inst = cmpInst;
      pattern->sources[0] = GetSource(cmpInst->getOperand(0), supportModifier, false, IsSourceOfSample(&I));
      pattern->sources[1] = GetSource(cmpInst->getOperand(1), supportModifier, false, IsSourceOfSample(&I));
      AddPattern(pattern);
      match = true;
    }
  }

  return match;
}

// Matches the following sequence:
//  %src0v4i8 = bitcast i32 %x to <4 x i8>
//  %elem1 = extractelement <4 x i8> %src0v4i8, i32 1
//  %dst = {s|z}ext i8 %elem1 to i32
bool CodeGenPatternMatch::MatchUnpack4i8(Instruction &I) {
  struct UnpackPattern : public Pattern {
    SSource source;
    uint32_t index;
    bool isUnsigned;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      pass->EmitUnpack4i8(source, index, isUnsigned, modifier);
    }
  };
  if (I.getType()->isIntegerTy(32) && (isa<ZExtInst>(&I) || isa<SExtInst>(&I))) {
    auto extract = dyn_cast<ExtractElementInst>(I.getOperand(0));
    if (extract && isa<ConstantInt>(extract->getIndexOperand()) && isa<BitCastInst>(extract->getVectorOperand()) &&
        cast<BitCastInst>(extract->getVectorOperand())->getType()->isIntOrIntVectorTy(8) &&
        !cast<BitCastInst>(extract->getVectorOperand())->getOperand(0)->getType()->isVectorTy() &&
        cast<BitCastInst>(extract->getVectorOperand())->getOperand(0)->getType()->getPrimitiveSizeInBits() == 32) {
      UnpackPattern *pattern = new (m_allocator) UnpackPattern();
      auto bitcast = cast<BitCastInst>(extract->getVectorOperand());
      uint32_t index = int_cast<uint32_t>(cast<ConstantInt>(extract->getIndexOperand())->getZExtValue());
      pattern->source = GetSource(bitcast->getOperand(0), false, false, IsSourceOfSample(&I));
      pattern->index = index;
      pattern->isUnsigned = isa<ZExtInst>(&I);
      AddPattern(pattern);
      return true;
    }
  }
  return false;
}

// Matches the following patterns
//
// Pattern 1, pack 4 i8 into i32
// Match the following sequence:
//  %x8 = trunc i32 %x to i8
//  %y8 = trunc i32 %y to i8
//  %z8 = trunc i32 %z to i8
//  %w8 = trunc i32 %w to i8
//  %vec0 = insertelement <4 x i8> undef, i8 %x8, i64 0
//  %vec1 = insertelement <4 x i8> %vec0, i8 %y8, i64 1
//  %vec2 = insertelement <4 x i8> %vec1, i8 %z8, i64 2
//  %vec3 = insertelement <4 x i8> %vec2, i8 %w8, i64 3
//  %dst = bitcast <4 x i8> %vec3 to i32
// And generate:
//  mov (M1_NM, 1) dst_0(0,0)<4> x(0,0)<0;1,0>
//  mov (M1_NM, 1) dst_0(0,1)<4> y(0,0)<0;1,0>
//  mov (M1_NM, 1) dst_0(0,2)<4> z(0,0)<0;1,0>
//  mov (M1_NM, 1) dst_0(0,3)<4> w(0,0)<0;1,0>
//
// Pattern 2, pack 4 i8 into i32 with saturation
// Match the following sequence:
//  %x1 = max i32 %x0, 0
//  %x2 = min i32 %x1, 127
//  %y1 = max i32 %y0, 0
//  %y2 = min i32 %y1, 127
//  %z1 = max i32 %z0, 0
//  %z2 = min i32 %z1, 127
//  %w1 = max i32 %w0, 0
//  %w2 = min i32 %w1, 127
//  %x8 = trunc i32 %x2 to i8
//  %y8 = trunc i32 %y2 to i8
//  %z8 = trunc i32 %z2 to i8
//  %w8 = trunc i32 %w2 to i8
//  %vec0 = insertelement <4 x i8> undef, i8 %x8, i64 0
//  %vec1 = insertelement <4 x i8> %vec0, i8 %y8, i64 1
//  %vec2 = insertelement <4 x i8> %vec1, i8 %z8, i64 2
//  %vec3 = insertelement <4 x i8> %vec2, i8 %w8, i64 3
//  %dst = bitcast <4 x i8> %vec3 to i32
// And generate:
//  max.sat (M1_NM, 1) xyzw_0(0,0)<4> x(0,0)<0;1,0> 0x0:d
//  max.sat (M1_NM, 1) xyzw_0(0,1)<4> y(0,0)<0;1,0> 0x0:d
//  max.sat (M1_NM, 1) xyzw_0(0,2)<4> z(0,0)<0;1,0> 0x0:d
//  max.sat (M1_NM, 1) xyzw_0(0,3)<4> w(0,0)<0;1,0> 0x0:d
//
// Pattern 3, pack 4 i8 into i32 with saturation
// Match the following sequence:
//  %x1 = max i32 %x0, 0
//  %x2 = min i32 %x1, 127
//  %y1 = max i32 %y0, 0
//  %y2 = min i32 %y1, 127
//  %z1 = max i32 %z0, 0
//  %z2 = min i32 %z1, 127
//  %w1 = max i32 %w0, 0
//  %w2 = min i32 %w1, 127
//  %y8 = shl nuw nsw i32 %y2, 8
//  %xy = or i32 %x2, %y8
//  %z8 = shl nuw nsw i32 %z2, 16
//  %xyz = or i32 %z8, %xy
//  %w8 = shl nuw nsw i32 %w2, 24
//  %dst = or i32 %xyz, %w8
// And generate:
//  max.sat (M1_NM, 1) xyzw_0(0,0)<4> x(0,0)<0;1,0> 0x0:d
//  max.sat (M1_NM, 1) xyzw_0(0,1)<4> y(0,0)<0;1,0> 0x0:d
//  max.sat (M1_NM, 1) xyzw_0(0,2)<4> z(0,0)<0;1,0> 0x0:d
//  max.sat (M1_NM, 1) xyzw_0(0,3)<4> w(0,0)<0;1,0> 0x0:d
//
bool CodeGenPatternMatch::MatchPack4i8(Instruction &I) {
  using namespace llvm::PatternMatch; // Scoped using declaration.

  struct PackPattern : public Pattern {
    std::array<EOPCODE, 4> opcodes;
    std::array<SSource, 4> sources0;
    std::array<SSource, 4> sources1;
    std::array<bool, 4> isSat;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      pass->EmitPack4i8(opcodes, sources0, sources1, isSat, modifier);
    }
  };

  // Lambda matches a `min` or `max` operation with a specific immediate
  // constant value (equal to `imm`) and returns the other source value
  // in the `otherSrc`.
  auto MatchMinMaxWithImm = [this](Value *v, uint64_t imm, bool matchMin, Value *&otherSrc) -> bool {
    bool isUnsigned;
    bool isMin;
    Value *src[2];
    if (isMinOrMax(v, src[0], src[1], isMin, isUnsigned) && isMin == matchMin &&
        (isa<ConstantInt>(src[0]) || isa<ConstantInt>(src[1]))) {
      ConstantInt *c = isa<ConstantInt>(src[0]) ? cast<ConstantInt>(src[0]) : cast<ConstantInt>(src[1]);
      if (c->getZExtValue() == imm) {
        otherSrc = isa<ConstantInt>(src[0]) ? src[1] : src[0];
        return true;
      }
    }
    return false;
  };
  // Lambda matches clamp(x, 0, 127) pattern.
  // If the pattern is found `x` is returned in the `clampedVal` reference.
  auto MatchClamp0_127 = [&MatchMinMaxWithImm](Value *v, Value *&clampedVal) -> bool {
    bool matchMin = true;
    bool matchMax = false;
    Value *src[2];
    // Match either of:
    // v = min(max(x, 0), 127)
    // v = max(min(x, 127), 0)
    if ((MatchMinMaxWithImm(v, 127, matchMin, src[0]) && MatchMinMaxWithImm(src[0], 0, matchMax, src[1])) ||
        (MatchMinMaxWithImm(v, 0, matchMax, src[0]) && MatchMinMaxWithImm(src[0], 127, matchMin, src[1]))) {
      clampedVal = src[1];
      return true;
    }
    return false;
  };

  EOPCODE opcodes[4] = {};
  Value *sources0[4] = {};
  Value *sources1[4] = {};
  bool isSat[4] = {};

  uint32_t elemsFound = 0;

  // Match patterns 1 and 2
  if (isa<BitCastInst>(I) && !I.getType()->isVectorTy() && I.getType()->getPrimitiveSizeInBits() == 32 &&
      I.getOperand(0)->getType()->isIntOrIntVectorTy(8) && isa<InsertElementInst>(I.getOperand(0))) {
    auto ie = cast<InsertElementInst>(I.getOperand(0));
    // Match sequences of InsertElementInsts, e.g.:
    //  %vec0 = insertelement <4 x i8> undef, i8 %x8, i64 0
    //  %vec1 = insertelement <4 x i8> %vec0, i8 %y8, i64 1
    //  %vec2 = insertelement <4 x i8> %vec1, i8 %z8, i64 2
    //  %vec3 = insertelement <4 x i8> %vec2, i8 %w8, i64 3
    while (ie) {
      auto idxVal = dyn_cast<ConstantInt>(ie->getOperand(2));
      auto idx = idxVal ? idxVal->getZExtValue() : 0;
      if (idxVal && sources0[idx] == nullptr) {
        sources0[idx] = ie->getOperand(1);
        ++elemsFound;
      } else {
        // Non-constant index is not supported.
        break;
      }
      ie = dyn_cast<InsertElementInst>(ie->getOperand(0));
    }
    if (elemsFound == 4) {
      // Match sequences integer trunc, e.g.:
      //  %x8 = trunc i32 %x2 to i8
      elemsFound = 0;
      for (uint32_t i = 0; i < 4; ++i) {
        auto trunc = dyn_cast<TruncInst>(sources0[i]);
        if (trunc) {
          opcodes[i] = llvm_bitcast;
          sources0[i] = trunc->getOperand(0);
          ++elemsFound;
        }
      }
    }
    if (elemsFound == 4) {
      // Match pattern 2
      // Match clamping of values to 0..127 range, e.g.:
      //  %x1 = max i32 %x0, 0
      //  %x2 = min i32 %x1, 127
      for (uint32_t i = 0; i < 4; ++i) {
        Value *srcToSat;
        if (MatchClamp0_127(sources0[i], srcToSat)) {
          opcodes[i] = llvm_max;
          sources0[i] = srcToSat;
          sources1[i] = ConstantInt::get(srcToSat->getType(), 0);
          isSat[i] = true;
        }
      }
    }
  }
  // Match pattern 3
  else if (I.getType()->getPrimitiveSizeInBits() == 32 && isa<BinaryOperator>(&I) &&
           cast<BinaryOperator>(&I)->getOpcode() == Instruction::Or) {
    auto orInst = cast<BinaryOperator>(&I);
    while (orInst) {
      // Match sequences of or + shl instructions, e.g.
      //  %y8 = shl nuw nsw i32 %y2, 8
      //  %xy = or i32 %x2, %y8
      ConstantInt *shlImm = nullptr;
      Value *shlSrc = nullptr;
      Value *orSrc = nullptr;
      if (match(orInst, m_Or(m_Value(orSrc), m_Shl(m_Value(shlSrc), m_ConstantInt(shlImm)))) ||
          match(orInst, m_Or(m_Shl(m_Value(shlSrc), m_ConstantInt(shlImm)), m_Value(orSrc)))) {
        // Lambda sets source values of a `sel` instruction.
        auto SetSource = [&](uint32_t i, Value *src0) {
          IGC_ASSERT(sources0[i] == nullptr);
          IGC_ASSERT(sources1[i] == nullptr);
          opcodes[i] = llvm_max;
          sources0[i] = src0;
          sources1[i] = ConstantInt::get(src0->getType(), 0);
          isSat[i] = true;
        };
        // Match clamping of the `shl` source to 0..127 range, e.g.:
        //  %x1 = max i32 %x0, 0
        //  %x2 = min i32 %x1, 127
        //  %x8 = shl nuw nsw i32 %x2, 8
        Value *srcToSat;
        uint32_t i = int_cast<uint32_t>(shlImm->getZExtValue());
        if (MatchClamp0_127(shlSrc, srcToSat) && ((i % 8) == 0 && (i / 8) < 4)) {
          SetSource(i / 8, srcToSat);
          ++elemsFound;
        }
        // Match clamping of the `or` source to 0..127 range, e.g.:
        //  %x1 = max i32 %x0, 0
        //  %x2 = min i32 %x1, 127
        //  %xy = or i32 %x2, %y8
        if (MatchClamp0_127(orSrc, srcToSat)) {
          SetSource(0, srcToSat);
          ++elemsFound;
        }
      }
      if (elemsFound != 4 && orSrc && isa<BinaryOperator>(orSrc) &&
          cast<BinaryOperator>(orSrc)->getOpcode() == Instruction::Or) {
        // Continue checking other remaining channels
        orInst = cast<BinaryOperator>(orSrc);
      } else {
        // All elements found, or an unsupported pattern.
        orInst = nullptr;
      }
    }
  }
  // TODO: consider adding support for patterns, where not all 4 i8 values
  // are packed, for example:
  //   %x8 = trunc i32 %x to i8
  //   %y8 = trunc i32 %y to i8
  //   %z8 = trunc i32 %z to i8
  //   %vec0 = insertelement <4 x i8> zeroinitializer, i8 %x8, i64 0
  //   %vec1 = insertelement <4 x i8> %vec0, i8 %y8, i64 1
  //   %vec2 = insertelement <4 x i8> %vec1, i8 %z8, i64 2
  //   %dst = bitcast <4 x i8> %vec3 to i32
  if (elemsFound == 4) {
    PackPattern *pattern = new (m_allocator) PackPattern();
    for (uint32_t i = 0; i < 4; ++i) {
      pattern->opcodes[i] = opcodes[i];
      pattern->sources0[i] = GetSource(sources0[i], false, false, IsSourceOfSample(&I));
      if (sources1[i] != nullptr) {
        pattern->sources1[i] = GetSource(sources1[i], false, false, IsSourceOfSample(&I));
      }
      pattern->isSat[i] = isSat[i];
    }
    AddPattern(pattern);
    return true;
  }
  return false;
}

// Matches repacking of packed <4 x i8> values, for example:
//   %src0v4i8 = bitcast i32 %x to <4 x i8>
//   %src1v4i8 = bitcast i32 %y to <4 x i8>
//   %elem1 = extractelement <4 x i8> %src0v4i8, i32 1
//   %dst0 = insertelement <4 x i8> %src0v4i8, i8 %elem1, i32 1
//   %elem2 = extractelement <4 x i8> %src1v4i8, i32 0
//   %dst1 = insertelement <4 x i8> %dst0, i8 %elem2, i32 2
//   %elem3 = extractelement <4 x i8> %src1v4i8, i32 1
//   %dst2 = insertelement <4 x i8> %dst1, i8 %elem3, i32 3
//   %dst3 = bitcast <4 x i8> %dst2 to i32
bool CodeGenPatternMatch::MatchRepack4i8(BitCastInst &I) {
  struct RepackPattern : public Pattern {
    std::array<SSource, 4> sources;
    std::array<uint32_t, 4> mappings;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) { pass->EmitRepack4i8(sources, mappings, modifier); }
  };

  if (!I.getType()->isVectorTy() && I.getType()->getPrimitiveSizeInBits() == 32 &&
      I.getOperand(0)->getType()->isIntOrIntVectorTy(8) && isa<InsertElementInst>(I.getOperand(0))) {
    Value *sources[4] = {};
    uint32_t mappings[4] = {};
    auto ie = cast<InsertElementInst>(I.getOperand(0));
    Value *baseVec = nullptr;
    uint32_t elemsFound = 0;
    while (ie) {
      auto idxVal = dyn_cast<ConstantInt>(ie->getOperand(2));
      if (!idxVal) {
        return false;
      }
      auto idx = idxVal->getZExtValue();
      if (idxVal && sources[idx] == nullptr) {
        sources[idx] = ie->getOperand(1);
        ++elemsFound;
      }
      baseVec = ie->getOperand(0);
      ie = dyn_cast<InsertElementInst>(baseVec);
    }

    for (uint32_t i = 0; i < 4; ++i) {
      if (sources[i]) {
        auto ee = dyn_cast<ExtractElementInst>(sources[i]);
        if (!ee || !isa<ConstantInt>(ee->getOperand(1)) || !isa<BitCastInst>(ee->getOperand(0)) ||
            (cast<BitCastInst>(ee->getOperand(0)))->getOperand(0)->getType()->isVectorTy() ||
            ((cast<BitCastInst>(ee->getOperand(0)))->getOperand(0)->getType()->getPrimitiveSizeInBits() != 32)) {
          return false;
        }
        auto bitcast = cast<BitCastInst>(ee->getOperand(0));
        mappings[i] = int_cast<uint32_t>(cast<ConstantInt>(ee->getOperand(1))->getZExtValue());
        sources[i] = bitcast->getOperand(0);
      } else {
        if (!isa<UndefValue>(baseVec) &&
            (!isa<BitCastInst>(baseVec) || (cast<BitCastInst>(baseVec))->getOperand(0)->getType()->isVectorTy() ||
             (cast<BitCastInst>(baseVec))->getOperand(0)->getType()->getPrimitiveSizeInBits() != 32)) {
          return false;
        }
        mappings[i] = i;
        auto bitcast = dyn_cast<BitCastInst>(baseVec);
        sources[i] = bitcast ? bitcast->getOperand(0) : baseVec;
      }
    }
    RepackPattern *pattern = new (m_allocator) RepackPattern();
    for (uint32_t i = 0; i < 4; ++i) {
      pattern->sources[i] = GetSource(sources[i], false, false, IsSourceOfSample(&I));
      pattern->sources[i].type = ISA_TYPE_UB;
      pattern->mappings[i] = mappings[i];
    }
    AddPattern(pattern);
    return true;
  }
  return false;
}

// Matches binary operations on packed <4 x i8> values.
// For example, the following patterns are matched:
//
// Shl+Asr pattern:
//   %x0 = shl i32 %x, 24
//   %dst = ashr i32 %x0, 28
//
// Unpack + shl pattern:
//   %src0v4i8 = bitcast i32 %x to <4 x i8>
//   %elem1 = extractelement <4 x i8> %src0v4i8, i32 1
//   %elem132 = sext i8 %elem1 to i32
//   %dst = shl i32 %elem132, 4
bool CodeGenPatternMatch::MatchBinaryUnpack4i8(Instruction &I) {
  struct BinaryUnpackPattern : public Pattern {
    SSource sources[2];
    Instruction *inst;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) { pass->BinaryUnary(inst, sources, modifier); }
  };

  Instruction *instToEmit = &I;
  if (I.getType()->isIntegerTy(32)) {
    std::tuple<bool, bool, Value *, uint32_t> sources[2];
    bool foundUnpack = false;
    // Shl+Asr pattern:
    //   %x0 = shl i32 %x, 24
    //   %dst = ashr i32 %x0, 28
    if ((isa<LShrOperator>(&I) || isa<AShrOperator>(&I)) && isa<ConstantInt>(cast<Instruction>(&I)->getOperand(1))) {
      auto shr = cast<Instruction>(&I);
      auto shl = dyn_cast<ShlOperator>(shr->getOperand(0));
      if (shl && isa<ConstantInt>(shl->getOperand(1)) && cast<ConstantInt>(shl->getOperand(1))->getZExtValue() == 24) {
        sources[0] = std::make_tuple(true, isa<LShrOperator>(&I), shl->getOperand(0), 0);
        auto shift = cast<ConstantInt>(shr->getOperand(1))->getZExtValue();
        bool isShl = shift < 24;
        if (isShl) {
          instToEmit = cast<Instruction>(shl);
          shift = 24 - shift;
        } else {
          shift = shift - 24;
        }

        sources[1] = std::make_tuple(false, false, ConstantInt::get(shr->getOperand(1)->getType(), shift), 0);
        foundUnpack = true;
      }
    } else {
      for (uint32_t i = 0; i < 2; ++i) {
        Value *op = I.getOperand(i);
        sources[i] = std::make_tuple(false, false, op, 0);
        if (isa<ZExtInst>(op) || isa<SExtInst>(op)) {
          auto ext = cast<Instruction>(op);
          if (auto extract = dyn_cast<ExtractElementInst>(ext->getOperand(0))) {
            if (isa<ConstantInt>(extract->getIndexOperand()) && isa<BitCastInst>(extract->getVectorOperand()) &&
                cast<BitCastInst>(extract->getVectorOperand())->getType()->isIntOrIntVectorTy(8) &&
                !cast<BitCastInst>(extract->getVectorOperand())->getOperand(0)->getType()->isVectorTy() &&
                cast<BitCastInst>(extract->getVectorOperand())->getOperand(0)->getType()->getPrimitiveSizeInBits() ==
                    32) {
              sources[i] = std::make_tuple(
                  true, isa<ZExtInst>(op), cast<BitCastInst>(extract->getVectorOperand())->getOperand(0),
                  int_cast<uint32_t>(cast<ConstantInt>(extract->getIndexOperand())->getZExtValue()));
              foundUnpack = true;
            }
          }
        }
      }
    }
    if (foundUnpack) {
      BinaryUnpackPattern *pattern = new (m_allocator) BinaryUnpackPattern();
      pattern->inst = instToEmit;
      for (uint32_t i = 0; i < 2; ++i) {
        auto [isUnpack, isUnsigned, source, subreg] = sources[i];
        pattern->sources[i] = GetSource(source, false, false, IsSourceOfSample(&I));
        if (isUnpack) {
          pattern->sources[i].region_set = true;
          pattern->sources[i].elementOffset = subreg;
          pattern->sources[i].region[0] = 4;
          pattern->sources[i].region[1] = 1;
          pattern->sources[i].region[2] = 0;
          pattern->sources[i].type = isUnsigned ? ISA_TYPE_UB : ISA_TYPE_B;
        }
      }
      AddPattern(pattern);
      return true;
    }
  }
  return false;
}

// Match the pattern of 32 x 32 = 64, a full 32-bit multiplication.
bool CodeGenPatternMatch::MatchFullMul32(llvm::Instruction &I) {
  using namespace llvm::PatternMatch; // Scoped namespace using.

  struct FullMul32Pattern : public Pattern {
    SSource srcs[2];
    bool isUnsigned;
    virtual void Emit(EmitPass *pass, const DstModifier &dstMod) { pass->EmitFullMul32(isUnsigned, srcs, dstMod); }
  };

  IGC_ASSERT_MESSAGE(I.getOpcode() == llvm::Instruction::Mul, "Mul instruction is expected!");

  if (!I.getType()->isIntegerTy(64))
    return false;

  llvm::Value *LHS = I.getOperand(0);
  llvm::Value *RHS = I.getOperand(1);

  // Swap operand to ensure the constant is always RHS.
  if (isa<ConstantInt>(LHS))
    std::swap(LHS, RHS);

  bool IsUnsigned = false;
  llvm::Value *L;
  llvm::Value *R;

  // Check LHS
  if (match(LHS, m_SExt(m_Value(L)))) {
    // Bail out if there's non 32-bit integer.
    if (!L->getType()->isIntegerTy(32))
      return false;
  } else if (match(LHS, m_ZExt(m_Value(L)))) {
    // Bail out if there's non 32-bit integer.
    if (!L->getType()->isIntegerTy(32))
      return false;
    IsUnsigned = true;
  } else {
    // Bailout if it's unknown that LHS have less significant bits than the
    // product.
    // NOTE we don't assertion fail the case where LHS is an constant to prevent
    // the assertion in O0 mode. Otherwise, we expect there's at most 1
    // constant operand.
    return false;
  }

  // Check RHS
  if (match(RHS, m_SExt(m_Value(R)))) {
    // Bail out if there's signedness mismatch or non 32-bit integer.
    if (IsUnsigned || !R->getType()->isIntegerTy(32))
      return false;
  } else if (match(RHS, m_ZExt(m_Value(R)))) {
    // Bail out if there's signedness mismatch or non 32-bit integer.
    if (!IsUnsigned || !R->getType()->isIntegerTy(32))
      return false;
    IsUnsigned = true;
  } else if (ConstantInt *CI = dyn_cast<ConstantInt>(RHS)) {
    APInt Val = CI->getValue();
    // 31-bit unsigned integer could be used as either signed or
    // unsigned one. Otherwise, we need special check how MSB is used.
    if (!Val.isIntN(31)) {
      if (!(IsUnsigned && Val.isIntN(32)) && !(!IsUnsigned && Val.isSignedIntN(32))) {
        return false;
      }
    }
    R = ConstantExpr::getTrunc(CI, L->getType());
  } else {
    // Bailout if it's unknown that RHS have less significant bits than the
    // product.
    return false;
  }

  FullMul32Pattern *Pat = new (m_allocator) FullMul32Pattern();
  Pat->srcs[0] = GetSource(L, !IsUnsigned, false, IsSourceOfSample(&I));
  Pat->srcs[1] = GetSource(R, !IsUnsigned, false, IsSourceOfSample(&I));
  Pat->isUnsigned = IsUnsigned;
  AddPattern(Pat);

  return true;
}

// For 32 bit integer mul/add/sub, use 16bit operands if possible. Thus,
// This will match 16x16->32, 16x32->32, the same for add/sub.
//
// For example:
//   1.  before:
//        %9 = ashr i32 %8, 16
//        %10 = mul nsw i32 %9, -1024
//        ( asr (16|M0)  r19.0<1>:d  r19.0<8;8,1>:d  16:w
//          mul (16|M0)  r19.0<1>:d  r19.0<8;8,1>:d  -1024:w )
//
//      after:
//      --> %10:d = mul %9.1<16;8:2>:w -1024:w
//          (  mul (16|M0)  r23.0<1>:d   r19.1<2;1,0>:w   -1024:w )
//
//  2. before:
//        %9  = lshr i32 %8, 16
//        %10 = and i32 %8, 65535
//        %11 = mul nuw i32 %9, %10
//        ( shr  (16|M0)   r14.0<1>:d  r12.0<8;8,1>:ud   16:w
//          and(16 | M0)   r12.0<1>:d  r12.0<8;8,1>:d  65535:d
//          mul(16 | M0)   r14.0<1>:d  r14.0<8;8,1>:d  r12.0<8;8,1>:d )
//
//     after:
//     --> %11:d = mul %8.1<16;8,2>:uw %8.0<16;8,2>:uw
//         ( mul (16|M0)  r14.0<1>:d   r12.1<2;1,0>:w   r12.0<2;1,0>:w )
//
bool CodeGenPatternMatch::MatchMulAdd16(Instruction &I) {
  using namespace llvm::PatternMatch;

  struct Oprd16Pattern : public Pattern {
    SSource srcs[2];
    Instruction *rootInst;
    virtual void Emit(EmitPass *pass, const DstModifier &dstMod) { pass->emitMulAdd16(rootInst, srcs, dstMod); }
  };

  // The code is under the control of registry key EnableMixIntOperands.
  if (IGC_IS_FLAG_DISABLED(EnableMixIntOperands)) {
    return false;
  }

  unsigned opc = I.getOpcode();
  IGC_ASSERT_MESSAGE((opc == Instruction::Mul) || (opc == Instruction::Add) || (opc == Instruction::Sub),
                     "Mul instruction is expected!");

  // Handle 32 bit integer mul/add/sub only.
  if (!I.getType()->isIntegerTy(32)) {
    return false;
  }

  // Try to replace any source operands with ones of type short if any. As vISA
  // allows the mix of any integer type, each operand is considered separately.
  struct {
    Value *src;
    bool useLower;
    bool isSigned;
  } oprdInfo[2];
  bool isCandidate = false;

  for (int i = 0; i < 2; ++i) {
    Value *oprd = I.getOperand(i);
    Value *L = nullptr;

    // oprdInfo[i].src == null --> no W operand replacement.
    oprdInfo[i].src = nullptr;
    if (ConstantInt *CI = dyn_cast<ConstantInt>(oprd)) {
      int64_t val = CI->isNegative() ? CI->getSExtValue() : CI->getZExtValue();
      // If src needs to be negated (y = x - a = x + (-a), as gen only
      // has add), need to check if the negated src fits into W/UW.
      bool isNegSrc = (opc == Instruction::Sub && i == 1);
      if (isNegSrc) {
        val = -val;
      }
      if (INT16_MIN <= val && val <= INT16_MAX) {
        oprdInfo[i].src = oprd;
        oprdInfo[i].useLower = true; // does not matter for const
        oprdInfo[i].isSigned = true;
        isCandidate = true;
      } else if (0 <= val && val <= UINT16_MAX) {
        oprdInfo[i].src = oprd;
        oprdInfo[i].useLower = true; // does not matter for const
        oprdInfo[i].isSigned = false;
        isCandidate = true;
      }
    } else if (match(oprd, m_And(m_Value(L), m_SpecificInt(0xFFFF)))) {
      oprdInfo[i].src = L;
      oprdInfo[i].useLower = true;
      oprdInfo[i].isSigned = false;
      isCandidate = true;
    } else if (match(oprd, m_LShr(m_Value(L), m_SpecificInt(16)))) {
      oprdInfo[i].src = L;
      oprdInfo[i].useLower = false;
      oprdInfo[i].isSigned = false;
      isCandidate = true;
    } else if (match(oprd, m_AShr(m_Shl(m_Value(L), m_SpecificInt(16)), m_SpecificInt(16)))) {
      oprdInfo[i].src = L;
      oprdInfo[i].useLower = true;
      oprdInfo[i].isSigned = true;
      isCandidate = true;
    } else if (match(oprd, m_AShr(m_Value(L), m_SpecificInt(16)))) {
      oprdInfo[i].src = L;
      oprdInfo[i].useLower = false;
      oprdInfo[i].isSigned = true;
      isCandidate = true;
    }
  }

  if (!isCandidate) {
    return false;
  }

  Oprd16Pattern *Pat = new (m_allocator) Oprd16Pattern();
  for (int i = 0; i < 2; ++i) {
    if (oprdInfo[i].src) {
      Pat->srcs[i] = GetSource(oprdInfo[i].src, false, false, IsSourceOfSample(&I));
      SSource &thisSrc = Pat->srcs[i];

      // for now, Use W/UW only if region_set is false or the src is scalar
      if (thisSrc.region_set && !(thisSrc.region[0] == 0 && thisSrc.region[1] == 1 && thisSrc.region[2] == 0)) {
        Pat->srcs[i] = GetSource(I.getOperand(i), true, false, IsSourceOfSample(&I));
      } else {
        // Note that SSource's type, if set by GetSource(), should be 32bit type. It's
        // safe to override it with either UW or W. But for SSource's offset, need to
        // re-calculate in term of 16bit, not 32bit.
        thisSrc.type = oprdInfo[i].isSigned ? ISA_TYPE_W : ISA_TYPE_UW;
        thisSrc.elementOffset = (2 * thisSrc.elementOffset) + (oprdInfo[i].useLower ? 0 : 1);
      }
    } else {
      Pat->srcs[i] = GetSource(I.getOperand(i), true, false, IsSourceOfSample(&I));
    }
  }
  Pat->rootInst = &I;
  AddPattern(Pat);

  return true;
}

bool CodeGenPatternMatch::BitcastSearch(SSource &source, llvm::Value *&value, bool broadcast) {
  if (auto elemInst = dyn_cast<ExtractElementInst>(value)) {
    if (auto bTInst = dyn_cast<BitCastInst>(elemInst->getOperand(0))) {
      // Pattern Matching (Instruction) + ExtractElem + (Vector)Bitcast
      //
      // In order to set the regioning for the ALU operand
      // I require three things:
      //      -The first is the source number of elements
      //      -The second is the destination number of elements
      //      -The third is the index from the extract element
      //
      // For example if I have <4 x i32> to <16 x i8> all I need is
      // the 4 (vstride) and the i8 (b) in this case the operand would look
      // like this -> r22.x <4;1,0>:b
      // x is calculated below and later on using the simdsize

      uint32_t index, srcNElts, dstNElts, nEltsRatio;
      llvm::Type *srcTy = bTInst->getOperand(0)->getType();
      llvm::Type *dstTy = bTInst->getType();

      srcNElts = (srcTy->isVectorTy()) ? (uint32_t)cast<IGCLLVM::FixedVectorType>(srcTy)->getNumElements() : 1;
      dstNElts = (dstTy->isVectorTy()) ? (uint32_t)cast<IGCLLVM::FixedVectorType>(dstTy)->getNumElements() : 1;

      if (srcNElts < dstNElts && srcTy->getScalarSizeInBits() < 64) {
        if (isa<ConstantInt>(elemInst->getIndexOperand())) {
          index = int_cast<uint>(cast<ConstantInt>(elemInst->getIndexOperand())->getZExtValue());
          nEltsRatio = dstNElts / srcNElts;
          source.value = bTInst->getOperand(0);
          source.SIMDOffset = iSTD::RoundDownNonPow2(index, nEltsRatio);
          source.elementOffset = source.elementOffset * nEltsRatio + index % nEltsRatio;
          value = source.value;
          if (!broadcast) {
            source.region_set = true;
            if (m_WI->isUniform(value))
              source.region[0] = 0;
            else
              source.region[0] = (unsigned char)nEltsRatio;
            source.region[1] = 1;
            source.region[2] = 0;
          }
          return true;
        }
      }
    }
  }
  return false;
}

bool CodeGenPatternMatch::MatchModifier(llvm::Instruction &I, bool SupportSrc0Mod) {
  struct ModifierPattern : public Pattern {
    SSource sources[2];
    llvm::Instruction *instruction;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      pass->BinaryUnary(instruction, sources, modifier);
    }
  };

  ModifierPattern *pattern = new (m_allocator) ModifierPattern();
  pattern->instruction = &I;
  uint nbSources = GetNbSources(I);

  llvm::SmallVector<llvm::Value *, 4> sources(I.op_begin(), I.op_end());
  if (FlushesDenormsOnInput(I)) {
    // Denorms are flushed at input - skip canonicalize
    std::transform(sources.begin(), sources.end(), sources.begin(),
                   [](llvm::Value *src) -> llvm::Value * { return SkipCanonicalize(src); });
  }
  bool supportModifierSrc0 = SupportsModifier(&I, m_Platform);
  bool supportRegioning = SupportsRegioning(&I);
  llvm::Instruction *src0Inst = llvm::dyn_cast<llvm::Instruction>(I.getOperand(0));
  if (I.getOpcode() == llvm::Instruction::UDiv && src0Inst && src0Inst->getOpcode() == llvm::Instruction::Sub) {
    supportModifierSrc0 = false;
  }
  pattern->sources[0] =
      GetSource(sources[0], supportModifierSrc0 && SupportSrc0Mod, supportRegioning, IsSourceOfSample(&I));
  if (nbSources > 1) {
    bool supportModifierSrc1 = SupportsModifier(&I, m_Platform);
    llvm::Instruction *src1Inst = llvm::dyn_cast<llvm::Instruction>(I.getOperand(1));
    if (I.getOpcode() == llvm::Instruction::UDiv && src1Inst && src1Inst->getOpcode() == llvm::Instruction::Sub) {
      supportModifierSrc1 = false;
    }
    pattern->sources[1] = GetSource(sources[1], supportModifierSrc1, supportRegioning, IsSourceOfSample(&I));

    // add df imm to constant pool for binary/ternary inst
    // we do 64-bit int imm bigger than 32 bits, since smaller may fit in D/W
    for (int i = 0, numSrc = (int)nbSources; i < numSrc; ++i) {
      Value *op = sources[i];
      if (isCandidateForConstantPool(op)) {
        AddToConstantPool(I.getParent(), op);
        pattern->sources[i].fromConstantPool = true;
      }
    }
  }

  AddPattern(pattern);

  return true;
}

bool CodeGenPatternMatch::MatchSingleInstruction(llvm::Instruction &I) {
  struct SingleInstPattern : Pattern {
    llvm::Instruction *inst;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      IGC_ASSERT(modifier.sat == false);
      IGC_ASSERT(modifier.flag == nullptr);
      pass->EmitNoModifier(inst);
    }
  };
  SingleInstPattern *pattern = new (m_allocator) SingleInstPattern();
  pattern->inst = &I;
  bool flushesDenorms = FlushesDenormsOnInput(I);
  uint numSources = GetNbSources(I);
  for (uint i = 0; i < numSources; i++) {
    Value *src = I.getOperand(i);
    if (flushesDenorms) {
      // Denorms are flushed at input - skip canonicalize
      src = SkipCanonicalize(src);
    }
    MarkAsSource(src, IsSourceOfSample(&I));
  }

  if (CallInst *callinst = dyn_cast<CallInst>(&I)) {
    // Mark the function pointer in indirect calls as a source
    if (!callinst->getCalledFunction()) {
      MarkAsSource(IGCLLVM::getCalledValue(callinst), IsSourceOfSample(&I));
    }
  }
  AddPattern(pattern);
  return true;
}

bool CodeGenPatternMatch::MatchCanonicalizeInstruction(llvm::Instruction &I) {
  struct CanonicalizeInstPattern : Pattern {
    CanonicalizeInstPattern(llvm::Instruction *pInst) : m_pInst(pInst) {}

    llvm::Instruction *m_pInst;
    Pattern *m_pPattern = nullptr;

    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      if (m_pPattern) {
        m_pPattern->Emit(pass, modifier);
      } else {
        pass->emitCanonicalize(m_pInst, modifier);
      }
    }
  };

  CanonicalizeInstPattern *pattern = new (m_allocator) CanonicalizeInstPattern(&I);
  IGC_ASSERT(isa<Instruction>(I.getOperand(0)) || isa<Argument>(I.getOperand(0)));
  // Current implementation assumes that mix mode is disabled if
  // half float or 32-bit float denorms must be flushed.
  if (m_ctx->getModuleMetaData()->compOpt.FloatDenormMode16 == IGC::FLOAT_DENORM_FLUSH_TO_ZERO ||
      m_ctx->getModuleMetaData()->compOpt.FloatDenormMode32 == IGC::FLOAT_DENORM_FLUSH_TO_ZERO) {
    IGC_ASSERT(!m_Platform.supportMixMode() || m_ctx->getModuleMetaData()->disableMixMode);
  }
  if (isa<Argument>(I.getOperand(0)) || !FlushesDenormsOnOutput(*(cast<Instruction>(I.getOperand(0))))) {
    MarkAsSource(I.getOperand(0), IsSourceOfSample(&I));
  } else {
    pattern->m_pPattern = Match(*llvm::cast<llvm::Instruction>(I.getOperand(0)));
  }

  AddPattern(pattern);
  return true;
}

bool CodeGenPatternMatch::MatchBranch(llvm::BranchInst &I) {
  using namespace llvm::PatternMatch;
  struct CondBrInstPattern : Pattern {
    SSource cond;
    llvm::BranchInst *inst;
    e_predMode predMode = EPRED_NORMAL;
    bool isDiscardBranch = false;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      if (isDiscardBranch) {
        pass->emitDiscardBranch(inst, cond);
      } else {
        pass->emitBranch(inst, cond, predMode);
      }
    }
  };
  CondBrInstPattern *pattern = new (m_allocator) CondBrInstPattern();
  pattern->inst = &I;

  if (!I.isUnconditional()) {
    Value *orSrc0 = nullptr;
    Value *orSrc1 = nullptr;
    Value *cond = I.getCondition();
    if (dyn_cast<GenIntrinsicInst>(cond, GenISAIntrinsic::GenISA_UpdateDiscardMask)) {
      pattern->isDiscardBranch = true;
    } else if (match(cond, m_Or(m_Value(orSrc0), m_Value(orSrc1)))) {
      // %6 = call i1 @llvm.genx.GenISA.UpdateDiscardMask(i1 %0, i1 %2)
      // %7 = or i1 %6, %2  (or: %7 = or i1 %2, %6)
      // br i1 %7, label %DiscardRet, label %PostDiscard
      if (auto intr = dyn_cast<GenIntrinsicInst>(orSrc0, GenISAIntrinsic::GenISA_UpdateDiscardMask)) {
        if (intr->getOperand(1) == orSrc1) {
          pattern->isDiscardBranch = true;
        }
      } else if (auto intr = dyn_cast<GenIntrinsicInst>(orSrc1, GenISAIntrinsic::GenISA_UpdateDiscardMask)) {
        if (intr->getOperand(1) == orSrc0) {
          pattern->isDiscardBranch = true;
        }
      }
    }
    pattern->cond = GetSource(I.getCondition(), false, false, IsSourceOfSample(&I));
  }
  AddPattern(pattern);
  return true;
}

bool CodeGenPatternMatch::MatchFloatingPointSatModifier(llvm::Instruction &I) {
  struct SatPattern : Pattern {
    Pattern *pattern;
    SSource source;
    bool isUnsigned;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      DstModifier mod = modifier;
      mod.sat = true;
      if (pattern) {
        pattern->Emit(pass, mod);
      } else {
        pass->Mov(source, mod);
      }
    }
  };
  bool match = false;
  llvm::Value *source = nullptr;
  bool isUnsigned = false;
  if (isSat(&I, source, isUnsigned)) {
    SatPattern *satPattern = new (m_allocator) SatPattern();
    if (llvm::Instruction *inst = llvm::dyn_cast<Instruction>(source)) {
      // As an heuristic we only match saturate if the instruction has one use
      // to avoid duplicating expensive instructions and increasing reg pressure
      // without improve code quality this may be refined in the future
      if (inst->hasOneUse() && SupportsSaturate(inst)) {
        bool isSourceOfSample = IsSourceOfSample(&I);
        if (isSourceOfSample) {
          m_sampleSource.insert(inst);
        }
        auto *pattern = Match(*inst);
        IGC_ASSERT_MESSAGE(pattern, "Failed to match pattern");
        // Even though the original `inst` may support saturate,
        // we need to know if the instruction(s) generated from
        // the pattern support it.
        if (pattern->supportsSaturate()) {
          satPattern->pattern = pattern;
          match = true;
        } else if (isSourceOfSample) {
          m_sampleSource.erase(inst);
        }
      }
    }
    if (!match) {
      satPattern->pattern = nullptr;
      satPattern->source = GetSource(source, true, false, IsSourceOfSample(&I));
      match = true;
    }
    if (isUniform(&I) && source->hasOneUse()) {
      gatherUniformBools(source);
    }
    AddPattern(satPattern);
  }
  return match;
}

bool CodeGenPatternMatch::MatchIntegerSatModifier(llvm::Instruction &I) {
  // a default pattern
  struct SatPattern : Pattern {
    Pattern *pattern;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      DstModifier mod = modifier;
      mod.sat = true;
      pattern->Emit(pass, mod);
    }
  };

  // a special pattern is required because of the fact that the instruction works on unsigned values
  // whereas the default type is signed for arithmetic instructions
  struct UAddPattern : Pattern {
    BinaryOperator *inst;

    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      DstModifier mod = modifier;
      mod.sat = true;
      pass->EmitUAdd(inst, mod);
    }
  };

  struct IntegerSatTruncPattern : public Pattern {
    SSource src;
    bool isSigned;
    virtual void Emit(EmitPass *pass, const DstModifier &dstMod) {
      pass->EmitIntegerTruncWithSat(isSigned, isSigned, src, dstMod);
    }
  };

  bool match = false;
  llvm::Value *source = nullptr;
  bool isUnsigned = false;
  if (isSat(&I, source, isUnsigned)) {
    IGC_ASSERT(llvm::isa<Instruction>(source));

    // As an heuristic we only match saturate if the instruction has one use
    // to avoid duplicating expensive instructions and increasing reg pressure
    // without improve code quality this may be refined in the future.
    if (llvm::Instruction *sourceInst = llvm::cast<llvm::Instruction>(source);
        sourceInst && sourceInst->hasOneUse() && SupportsSaturate(sourceInst)) {
      if (llvm::BinaryOperator *binaryOpInst = llvm::dyn_cast<llvm::BinaryOperator>(source);
          binaryOpInst && (binaryOpInst->getOpcode() == llvm::BinaryOperator::BinaryOps::Add) && isUnsigned) {
        match = true;

        uint numSources = GetNbSources(*sourceInst);
        for (uint i = 0; i < numSources; i++) {
          MarkAsSource(sourceInst->getOperand(i), IsSourceOfSample(&I));
        }

        UAddPattern *uAddPattern = new (m_allocator) UAddPattern();
        uAddPattern->inst = binaryOpInst;
        AddPattern(uAddPattern);
      } else if (binaryOpInst && (binaryOpInst->getOpcode() == llvm::BinaryOperator::BinaryOps::Add) && !isUnsigned) {
        match = true;
        SatPattern *satPattern = new (m_allocator) SatPattern();
        satPattern->pattern = Match(*sourceInst);
        AddPattern(satPattern);
      } else if (llvm::TruncInst *truncInst = llvm::dyn_cast<llvm::TruncInst>(source); truncInst) {
        match = true;
        IntegerSatTruncPattern *satPattern = new (m_allocator) IntegerSatTruncPattern();
        satPattern->isSigned = !isUnsigned;
        satPattern->src = GetSource(truncInst->getOperand(0), !isUnsigned, false, IsSourceOfSample(&I));
        AddPattern(satPattern);
      } else {
        IGC_ASSERT_MESSAGE(0, "An undefined pattern match");
      }
    }
  }
  return match;
}

bool CodeGenPatternMatch::MatchSelectModifier(llvm::SelectInst &I) {
  if (I.getType()->isAggregateType())
    return MatchSingleInstruction(I);

  struct SelectPattern : Pattern {
    SSource sources[3];
    e_predMode predMode;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      DstModifier modf = modifier;
      modf.predMode = predMode;
      pass->Select(sources, modf);
    }
  };
  SelectPattern *pattern = new (m_allocator) SelectPattern();
  pattern->predMode = EPRED_NORMAL;

  pattern->sources[0] = GetSource(I.getCondition(), false, false, IsSourceOfSample(&I));
  pattern->sources[1] = GetSource(I.getTrueValue(), true, false, IsSourceOfSample(&I));
  pattern->sources[2] = GetSource(I.getFalseValue(), true, false, IsSourceOfSample(&I));

  // try to add to constant pool whatever possible.
  if (isCandidateForConstantPool(I.getTrueValue())) {
    AddToConstantPool(I.getParent(), I.getTrueValue());
    pattern->sources[1].fromConstantPool = true;
  }

  if (isCandidateForConstantPool(I.getFalseValue())) {
    AddToConstantPool(I.getParent(), I.getFalseValue());
    pattern->sources[2].fromConstantPool = true;
  }

  AddPattern(pattern);
  return true;
}

static bool IsPositiveFloat(Value *v, unsigned int depth = 0) {
  if (depth > 3) {
    // limit the depth of recursion to avoid compile time problem
    return false;
  }
  if (ConstantFP *cst = dyn_cast<ConstantFP>(v)) {
    if (!cst->getValueAPF().isNegative()) {
      return true;
    }
  } else if (Instruction *I = dyn_cast<Instruction>(v)) {
    switch (I->getOpcode()) {
    case Instruction::FMul:
    case Instruction::FAdd:
      return IsPositiveFloat(I->getOperand(0), depth + 1) && IsPositiveFloat(I->getOperand(1), depth + 1);
    case Instruction::Call:
      if (IntrinsicInst *intrinsicInst = dyn_cast<IntrinsicInst>(I)) {
        if (intrinsicInst->getIntrinsicID() == Intrinsic::fabs) {
          return true;
        }
      } else if (isa<GenIntrinsicInst>(I, GenISAIntrinsic::GenISA_fsat)) {
        return true;
      }
      break;
    default:
      break;
    }
  }
  return false;
}

bool CodeGenPatternMatch::MatchPow(llvm::IntrinsicInst &I) {
  if (IGC_IS_FLAG_ENABLED(DisableMatchPow)) {
    return false;
  }

  // For this pattern match exp(log(x) * y) = pow
  // if x < 0 and y is an integer (ex: 1.0)
  // with pattern match : pow(x, 1.0) = x
  // without pattern match : exp(log(x) * 1.0) = NaN because log(x) is NaN.
  //
  // Since pow is 2x slower than exp/log, disabling this optimization might not hurt much.
  // Keep the code and disable MatchPow to track any performance change for now.
  struct PowPattern : public Pattern {
    SSource sources[2];
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) { pass->Pow(sources, modifier); }
  };
  bool found = false;
  llvm::Value *source0 = NULL;
  llvm::Value *source1 = NULL;
  if (I.getIntrinsicID() == Intrinsic::exp2) {
    llvm::BinaryOperator *mul = dyn_cast<BinaryOperator>(I.getOperand((0)));
    if (mul && mul->getOpcode() == Instruction::FMul) {
      for (uint j = 0; j < 2; j++) {
        llvm::IntrinsicInst *log = dyn_cast<IntrinsicInst>(mul->getOperand(j));
        if (log && log->getIntrinsicID() == Intrinsic::log2) {
          if (IsPositiveFloat(log->getOperand(0))) {
            source0 = log->getOperand(0);
            source1 = mul->getOperand(1 - j);
            found = true;
            break;
          }
        }
      }
    }
  }
  if (found) {
    PowPattern *pattern = new (m_allocator) PowPattern();
    pattern->sources[0] = GetSource(source0, true, false, IsSourceOfSample(&I));
    pattern->sources[1] = GetSource(source1, true, false, IsSourceOfSample(&I));
    AddPattern(pattern);
  }
  return found;
}

// Helper function for MatchGenericPointersCmp, not to be used anywhere else.
// It searches recursively for its arg origin until it tracks it down to
// ExtractValueInst (where author assumed that there's no need to search further)
// or reaches the depth limit (the default value of 3 should be deep enough).
// The tag is contained in 3 highest bits of the pointer, so we're interested
// only in the half containing high bits, hence the HIGH_ADDR_INDEX check.
// The function returns pair where first tells whether any emulated 64bit pointer
// was found, second tells whether any of the emulated pointers found needs its tag
// cleared.
// The following example is to show why we're looking for any emu ptr, without
// requiring all of them to be that:
// Let's say that someone hardcoded a 64bit generic ptr as an unsigned int value
// and wants to assign it to an operand based on a condition, to then compare it
// with another operand.
//
//      unsigned generic_ptr_val = 0xFF00FFFFFFDF0000
//      ptr = min(&some_val, generic_ptr_val)
//      if (ptr < other_generic_ptr)
//      {
//          ...
//      }
//
// In such case we still need to treat it as a regular generic ptr and clear the tag.
static std::pair<bool, bool> tracksDownToEmu(const Value *op, const unsigned depth = 3) {
  if (depth == 0) {
    return std::make_pair(false, false);
  }

  constexpr unsigned HIGH_ADDR_INDEX = 1;
  static auto hasGenericPtrTy = [](const Value *V) {
    const Type *Ty = V->getType();
    return isa<PointerType>(Ty) && Ty->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC;
  };

  if (const auto *e = dyn_cast<ExtractValueInst>(op)) {
    if (e->getNumIndices() != 1 || e->getIndices()[0] != HIGH_ADDR_INDEX) {
      return std::make_pair(false, false);
    }
    if (const auto *GII = dyn_cast<GenIntrinsicInst>(e->getAggregateOperand())) {
      if (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_ptr_to_pair) {
        const Value *ptr = GII->getOperand(0);
        if (hasGenericPtrTy(ptr)) {
          return std::make_pair(true, !isa<ConstantPointerNull>(ptr));
        }
      }
    }
  } else if (const auto *selectInst = dyn_cast<SelectInst>(op)) {
    auto [op1EmuFound, op1NeedsTagClearing] = tracksDownToEmu(selectInst->getOperand(1), depth - 1);
    auto [op2EmuFound, op2NeedsTagClearing] = tracksDownToEmu(selectInst->getOperand(2), depth - 1);
    return std::make_pair(op1EmuFound | op2EmuFound, op1NeedsTagClearing | op2NeedsTagClearing);
  }
  return std::make_pair(false, false);
}

// When a NULL pointer is directly assigned to a generic pointer, then
// it doesn't have a pointer tag, so comparing it with NULL pointers that
// were firstly assigned to a named addrspace and then casted to a
// generic pointer may lead to incorrect comparison results.
//
// Example pseudo-code:
//  null_generic_ptr = NULL               ; <-- not tagged generic NULL pointer
//  local_ptr = NULL
//  generic_ptr = local_ptr               ; <-- local NULL pointer gets tagged while casting to generic pointer
//  if(generic_ptr == null_generic_ptr)   ; <-- comparing tagged generic pointer with not-tagged one
//
// Pointer tag should not be taken into account during generic pointer
// comparison, so the following pattern match detects generic pointers
// comparisons and triggers a special handling for them. The special handling
// relies on clearing tag bits right before pointers comparison.
//
// It matches the following patterns, for platforms:
//  1. With a native support for 64-bit integer instructions:
//    a) Pointers comparison through integers
//      %pti0 = ptrtoint i32 addrspace(4)* %ptr0 to i64
//      %pti1 = ptrtoint i32 addrspace(4)* %ptr1 to i64
//      %cmp = icmp eq i64 %pti0, %pti1
//    b) Regular pointers comparison
//      %cmp = icmp eq i32 addrspace(4)* %ptr0, i32 addrspace(4)* %ptr1
//
//  2. Without a native support for 64-bit integer instructions:
//    %ptp0 = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p4i32(i32 addrspace(4)* %ptr0)
//    %highAddr0 = extractvalue { i32, i32 } %ptp0, 1
//    %ptp1 = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p4i32(i32 addrspace(4)* %ptr1)
//    %highAddr1 = extractvalue { i32, i32 } %ptp1, 1
//    %cmp = icmp eq i32 %highAddr0, %highAddr1
bool CodeGenPatternMatch::MatchGenericPointersCmp(llvm::CmpInst &I) {
  using namespace llvm::PatternMatch;
  struct GenericPointersCmpPattern : public Pattern {
    llvm::CmpInst *cmp;
    SSource cmpSources[2];
    uint8_t clearTagMask;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      pass->EmitGenericPointersCmp(cmp, cmpSources, modifier, clearTagMask);
    }
  };

  auto hasGenericPtrTy = [](Value *V) {
    Type *Ty = V->getType();
    return isa<PointerType>(Ty) && Ty->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC;
  };

  bool found = false;
  uint8_t clearTagMask = 0;

  if (m_ctx->m_hasEmu64BitInsts && m_Platform.hasNoFullI64Support()) {
    // %ptp = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p4i32(i32 addrspace(4)* %48)
    // %highAddr = extractvalue { i32, i32 } %ptp, 1
    // %cmp = icmp eq i32 %highAddr, 0
    for (uint8_t i = 0; i < 2; ++i) {
      Value *src = I.getOperand(i);
      auto [emuFound, needsTagClearing] = tracksDownToEmu(src);
      if (emuFound) {
        found = true;
        clearTagMask |= (needsTagClearing << i);
      }
    }
  } else {
    Value *src0 = I.getOperand(0);
    Value *src1 = I.getOperand(1);

    Value *ptr0 = nullptr;
    Value *ptr1 = nullptr;

    // %cmp = icmp eq i32 addrspace(4)* %2, addrspace(4)* %2
    if (hasGenericPtrTy(src0) && hasGenericPtrTy(src1)) {
      ptr0 = src0;
      ptr1 = src1;
      found = true;
    }
    // %pti0 = ptrtoint i32 addrspace(4)* %ptr0 to i64
    // %pti1 = ptrtoint i32 addrspace(4)* %ptr1 to i64
    // %cmp = icmp eq i64 %pti0, %pti1
    else if (match(src0, m_PtrToInt(m_Value(ptr0))) && match(src1, m_PtrToInt(m_Value(ptr1))))
      found = (hasGenericPtrTy(ptr0) && hasGenericPtrTy(ptr1));

    if (found) {
      clearTagMask |= (!isa<ConstantPointerNull>(ptr0) << 0);
      clearTagMask |= (!isa<ConstantPointerNull>(ptr1) << 1);
    }
  }

  if (found) {
    GenericPointersCmpPattern *pattern = new (m_allocator) GenericPointersCmpPattern();

    bool supportsMod = SupportsModifier(&I, m_Platform);
    pattern->cmpSources[0] = GetSource(I.getOperand(0), supportsMod, false, IsSourceOfSample(&I));
    pattern->cmpSources[1] = GetSource(I.getOperand(1), supportsMod, false, IsSourceOfSample(&I));
    pattern->cmp = &I;
    pattern->clearTagMask = clearTagMask;

    AddPattern(pattern);
  }

  return found;
}

// We match this pattern
// %1 = add %2 %3
// %b = %cmp %1 0
// right now we don't match if the alu has more than 1 use has it could generate worse code
bool CodeGenPatternMatch::MatchCondModifier(llvm::CmpInst &I) {
  struct CondModifierPattern : Pattern {
    Pattern *pattern;
    Instruction *alu;
    CmpInst *cmp;
    int aluOprdNum = 0; // 0: alu is src0; otherwise, alu is src1
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      IGC_ASSERT(modifier.flag == nullptr);
      IGC_ASSERT(modifier.sat == false);
      pass->emitAluConditionMod(pattern, alu, cmp, aluOprdNum);
    }
  };
  bool found = false;
  for (uint i = 0; i < 2; i++) {
    if (IsZero(I.getOperand(i))) {
      llvm::Instruction *alu = dyn_cast<Instruction>(I.getOperand(1 - i));
      if (alu && alu->hasOneUse() && SupportsCondModifier(alu)) {
        CondModifierPattern *pattern = new (m_allocator) CondModifierPattern();
        pattern->pattern = Match(*alu);
        IGC_ASSERT_MESSAGE(pattern->pattern, "Failed to match pattern");
        pattern->alu = alu;
        pattern->cmp = &I;
        pattern->aluOprdNum = 1 - i;
        AddPattern(pattern);
        found = true;
        break;
      }
    }
  }
  return found;
}

// we match the following pattern
// %f = cmp %1 %2
// %o = or/and %f %g
bool CodeGenPatternMatch::MatchBoolOp(llvm::BinaryOperator &I) {
  struct BoolOpPattern : public Pattern {
    Pattern *cmpPattern;
    llvm::BinaryOperator *boolOp;
    SSource binarySource;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      pass->CmpBoolOp(cmpPattern, boolOp, binarySource, modifier);
    }
  };

  IGC_ASSERT(I.getOpcode() == Instruction::Or || I.getOpcode() == Instruction::And);
  bool found = false;
  if (I.getType()->isIntegerTy(1)) {
    for (uint i = 0; i < 2; i++) {
      if (CmpInst *cmp = llvm::dyn_cast<CmpInst>(I.getOperand(i))) {
        // only beneficial if the other operand only have one use
        if (I.getOperand(1 - i)->hasOneUse()) {
          BoolOpPattern *pattern = new (m_allocator) BoolOpPattern();
          pattern->cmpPattern = Match(*cmp);
          pattern->boolOp = &I;
          pattern->binarySource = GetSource(I.getOperand(1 - i), false, false, IsSourceOfSample(&I));
          AddPattern(pattern);
          found = true;
          break;
        }
      }
    }
  }
  return found;
}

bool CodeGenPatternMatch::MatchFunnelShiftRotate(llvm::IntrinsicInst &I) {
  // Hanlde only funnel shift that can be turned into rotate.
  struct funnelShiftRotatePattern : public Pattern {
    SSource sources[2];
    llvm::IntrinsicInst *instruction;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      bool isShl = instruction->getIntrinsicID() == Intrinsic::fshl;
      pass->Binary(isShl ? EOPCODE_ROL : EOPCODE_ROR, sources, modifier);
    }
  };

  if (!m_Platform.supportRotateInstruction() || I.getType()->isVectorTy()) {
    return false;
  }

  Value *A = I.getOperand(0);
  Value *B = I.getOperand(1);
  Value *Amt = I.getOperand(2);
  uint32_t typebits = I.getType()->getScalarSizeInBits();
  if (A != B || (typebits != 16 && typebits != 32 && typebits != 64)) {
    return false;
  }
  if (typebits == 64 && !m_Platform.supportQWRotateInstructions()) {
    return false;
  }

  // Found the pattern.
  funnelShiftRotatePattern *pattern = new (m_allocator) funnelShiftRotatePattern();
  pattern->instruction = &I;
  pattern->sources[0] = GetSource(A, true, false, IsSourceOfSample(&I));
  pattern->sources[1] = GetSource(Amt, true, false, IsSourceOfSample(&I));

  AddPattern(pattern);
  return true;
}

bool CodeGenPatternMatch::MatchUnmaskedRegionBoundary(Instruction &I, bool start) {
  struct UnmaskedBoundaryPattern : public Pattern {
    bool start;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      (void)modifier;
      pass->emitUnmaskedRegionBoundary(start);
    }
  };

  UnmaskedBoundaryPattern *pattern = new (m_allocator) UnmaskedBoundaryPattern();
  pattern->start = start;
  AddPattern(pattern);
  return true;
}

bool CodeGenPatternMatch::MatchAdd3(Instruction &I) {
  using namespace llvm::PatternMatch;

  struct Add3Pattern : public Pattern {
    SSource sources[3];
    Instruction *instruction;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) { pass->Tenary(EOPCODE_ADD3, sources, modifier); }
  };

  if (IGC_IS_FLAG_DISABLED(EnableAdd3) || !m_Platform.supportAdd3Instruction()) {
    return false;
  }

  // Only handle D & W integer types.
  Type *Ty = I.getType();
  if (!(Ty->isIntegerTy(16) || Ty->isIntegerTy(32))) {
    return false;
  }

  Value *s0 = I.getOperand(0);
  Value *s1 = nullptr, *s2 = nullptr;
  e_modifier Mod1 = EMOD_NONE, Mod2 = EMOD_NONE;
  Instruction *I0 = dyn_cast<Instruction>(s0);
  Instruction *Add2 = nullptr;
  if (I0) {
    if (I0->getOpcode() == Instruction::Sub) {
      s0 = I0->getOperand(0);
      s1 = I0->getOperand(1);
      Mod1 = EMOD_NEG;
      Add2 = I0;
    } else if (I0->getOpcode() == Instruction::Add) {
      s0 = I0->getOperand(0);
      s1 = I0->getOperand(1);
      Add2 = I0;
    }
  }

  bool isNeg = (I.getOpcode() == Instruction::Sub);
  if (s1 == nullptr) {
    s1 = I.getOperand(1);
    Instruction *I1 = dyn_cast<Instruction>(s1);
    if (I1) {
      if (I1->getOpcode() == Instruction::Sub) {
        s1 = I1->getOperand(0);
        s2 = I1->getOperand(1);
        Add2 = I1;
        if (isNeg) {
          Mod1 = EMOD_NEG;
        } else {
          Mod2 = EMOD_NEG;
        }
      } else if (I1->getOpcode() == Instruction::Add) {
        s1 = I1->getOperand(0);
        s2 = I1->getOperand(1);
        Add2 = I1;
        if (isNeg) {
          Mod1 = EMOD_NEG;
          Mod2 = EMOD_NEG;
        }
      }
    }
  } else {
    s2 = I.getOperand(1);
    if (isNeg) {
      Mod2 = EMOD_NEG;
    }
  }

  if (s2 == nullptr) {
    return false;
  }

  // If source operand corresponding to first add instruction
  // has many uses the add3 pattern match is unlikely to be profitable,
  // as it increases register pressure and makes register bank
  // conflicts more likely. Unless some of the operands are constants
  // then there is no increase of register pressure from add3 construction
  if (m_ctx->type == ShaderType::OPENCL_SHADER) {
    const int Threshold = 3;

    if (!isa<ConstantInt>(s0) && !isa<ConstantInt>(s1) && !isa<ConstantInt>(s2) &&
        (Add2->getOperand(0)->hasOneUse() || Add2->getOperand(1)->hasOneUse()) && Add2->hasNUsesOrMore(Threshold)) {
      // add3 is unlikely to be profitable.
      return false;
    }
  }

  // Found the pattern.
  // Make sure that the middle one is not constant
  if (isa<ConstantInt>(s1)) {
    std::swap(s1, s2);
    std::swap(Mod1, Mod2);
  }

  Add3Pattern *pattern = new (m_allocator) Add3Pattern();
  pattern->instruction = &I;
  pattern->sources[0] = GetSource(s0, true, false, IsSourceOfSample(&I));
  pattern->sources[1] = GetSource(s1, true, false, IsSourceOfSample(&I));
  pattern->sources[2] = GetSource(s2, true, false, IsSourceOfSample(&I));
  if (Mod1 != EMOD_NONE) {
    pattern->sources[1].mod = CombineModifier(Mod1, pattern->sources[1].mod);
  }
  if (Mod2 != EMOD_NONE) {
    pattern->sources[2].mod = CombineModifier(Mod2, pattern->sources[2].mod);
  }
  AddPattern(pattern);
  return true;
}

bool CodeGenPatternMatch::MatchBfn(Instruction &I) {
  using namespace llvm::PatternMatch;

  struct BfnPattern : public Pattern {
    uint8_t booleanFuncCtrl = 0;
    SSource sources[3];
    Instruction *instruction;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) { pass->Bfn(booleanFuncCtrl, sources, modifier); }
  };

  if (IGC_IS_FLAG_DISABLED(EnableBfn) || !m_Platform.supportBfnInstruction()) {
    return false;
  }

  // Only handle D & W integer types.
  Type *Ty = I.getType();
  if (!(Ty->isIntegerTy(16) || Ty->isIntegerTy(32))) {
    return false;
  }

  struct CtrlCalculator {
    enum OP { NONE, AND, OR, XOR };
    enum SOURCE {
      S0 = 0, // s0, s1, s2
      S1 = 1,
      S2 = 2,
      NS0 = 3, // ~s0, ~s1, ~s2
      NS1 = 4,
      NS2 = 5
    };
    // Value of the sources for calculating BooleanFuncCtrl. The
    // index maps to enum SOURCE
    const uint8_t s[6] = {0xAA, 0xCC, 0xF0, 0x55, 0x33, 0x0F};

    typedef llvm::SmallVector<OP, 2> OpVecType;
    typedef llvm::SmallVector<SOURCE, 3> SourceVecType;

    // The sequence of matched operators, follow the calculation order
    OpVecType ops;
    // The sequence of matched sources, follow the calculation order
    SourceVecType source_idx;

    void addSource(SOURCE src) { source_idx.push_back(src); }
    void addOPFromLLVMOp(unsigned llvmOp) {
      if (llvmOp == Instruction::And)
        ops.push_back(AND);
      else if (llvmOp == Instruction::Or)
        ops.push_back(OR);
      else if (llvmOp == Instruction::Xor)
        ops.push_back(XOR);
      else {
        IGC_ASSERT_MESSAGE(0, "MatchBfn: inccorect opcode");
        return;
      }
    }

    uint8_t getBooleanFuncCtrl() {
      IGC_ASSERT_MESSAGE((source_idx.size() == (ops.size() + 1)), "MatchBfn: OPs and Sources length missmatched");
      SourceVecType::iterator s_it = source_idx.begin(), s_end = source_idx.end();
      // start from the first source
      uint8_t result = s[*s_it];
      ++s_it;

      // iterate through the matched sequence and compute the BooleanFuncCtrl
      OpVecType::iterator op_it = ops.begin();
      for (; s_it != s_end; ++s_it, ++op_it) {
        switch (*op_it) {
        case AND:
          result = result & s[*s_it];
          break;
        case OR:
          result = result | s[*s_it];
          break;
        case XOR:
          result = result ^ s[*s_it];
          break;
        default:
          IGC_ASSERT_MESSAGE(0, "MatchBfn:CtrlCalculator: inccorect OP");
          break;
        }
      }
      return result;
    }
  };

  auto isBinaryLogic = [](Instruction::BinaryOps op) {
    return op == Instruction::Or || op == Instruction::And || op == Instruction::Xor;
  };
  // Find BFN patterns. Matched patterns: (op0 and op1 are boolean operations)
  // ~s0  ~s1                  ~s1  ~s2   <-- extra check to matchNot on each found src
  //  |    |                    |    |
  // s0   s1  ~s2         ~s0   s1  s2
  //   \ /    |            |     \  /
  //    op    s2     OR    s0   op1
  //     \   /              \   /
  //      op0                op0   <-- match to BFN

  // if source operand has many uses the bfn pattern match is unlikely to be profitable,
  // as it increases register pressure and makes register bank conflicts more likely
  // ToDo: tune the value of N
  const int useThreshold = 4;
  // if both operands of the root is binary logic op, use simple heuristics
  // to fold one of them
  if (isa<BinaryOperator>(I.getOperand(0)) && isa<BinaryOperator>(I.getOperand(1))) {
    auto I0 = cast<BinaryOperator>(I.getOperand(0));
    auto I1 = cast<BinaryOperator>(I.getOperand(1));
    if (I0->hasNUsesOrMore(useThreshold) && I1->hasNUsesOrMore(useThreshold)) {
      // bfn is unlikely to be profitable.
      return false;
    } else if (I0->getNumUses() > I1->getNumUses()) {
      I.setOperand(0, I1);
      I.setOperand(1, I0);
    }
  }

  // Further match s to not operation. Update s if matched.
  auto matchNot = [](Value *&s) {
    BinaryOperator *bo = dyn_cast<BinaryOperator>(s);
    if (bo && match(s, m_Not(m_Specific(bo->getOperand(0))))) {
      s = bo->getOperand(0);
      return true;
    }
    return false;
  };

  CtrlCalculator ctrlcal;
  Value *s0 = I.getOperand(0);
  Value *s1 = nullptr, *s2 = nullptr;
  BinaryOperator *I0 = dyn_cast<BinaryOperator>(s0);
  if (I0) {
    if (isBinaryLogic(I0->getOpcode()) && !I0->hasNUsesOrMore(useThreshold)) {
      s0 = I0->getOperand(0);
      s1 = I0->getOperand(1);
    }
  }

  if (s1 == nullptr) {
    s1 = I.getOperand(1);
    BinaryOperator *I1 = dyn_cast<BinaryOperator>(s1);
    if (I1) {
      if (isBinaryLogic(I1->getOpcode()) && !I1->hasNUsesOrMore(useThreshold)) {
        s1 = I1->getOperand(0);
        s2 = I1->getOperand(1);

        // add ops and sources by execution order
        ctrlcal.addSource(matchNot(s1) ? CtrlCalculator::NS1 : CtrlCalculator::S1);
        ctrlcal.addOPFromLLVMOp(I1->getOpcode());
        ctrlcal.addSource(matchNot(s2) ? CtrlCalculator::NS2 : CtrlCalculator::S2);
        ctrlcal.addOPFromLLVMOp(I.getOpcode());
        ctrlcal.addSource(matchNot(s0) ? CtrlCalculator::NS0 : CtrlCalculator::S0);
      }
    }
  } else {
    s2 = I.getOperand(1);

    // add ops and sources by execution order
    ctrlcal.addSource(matchNot(s0) ? CtrlCalculator::NS0 : CtrlCalculator::S0);
    ctrlcal.addOPFromLLVMOp(I0->getOpcode());
    ctrlcal.addSource(matchNot(s1) ? CtrlCalculator::NS1 : CtrlCalculator::S1);
    ctrlcal.addOPFromLLVMOp(I.getOpcode());
    ctrlcal.addSource(matchNot(s2) ? CtrlCalculator::NS2 : CtrlCalculator::S2);
  }

  if (s2 == nullptr)
    return false;

  BfnPattern *pattern = new (m_allocator) BfnPattern();
  pattern->booleanFuncCtrl = ctrlcal.getBooleanFuncCtrl();
  pattern->instruction = &I;
  pattern->sources[0] = GetSource(s0, false, false, IsSourceOfSample(&I));
  pattern->sources[1] = GetSource(s1, false, false, IsSourceOfSample(&I));
  pattern->sources[2] = GetSource(s2, false, false, IsSourceOfSample(&I));

  // BFN can use imm16 in src0 and src2, check for those;
  // otherwise try to add to constant pool even int32.
  if (dyn_cast<ConstantInt>(s0) && !s0->getType()->isIntegerTy(16)) {
    AddToConstantPool(I.getParent(), s0);
    pattern->sources[0].fromConstantPool = true;
  }
  if (dyn_cast<ConstantInt>(s1)) {
    AddToConstantPool(I.getParent(), s1);
    pattern->sources[1].fromConstantPool = true;
  }
  if (dyn_cast<ConstantInt>(s2) && !s2->getType()->isIntegerTy(16)) {
    AddToConstantPool(I.getParent(), s2);
    pattern->sources[2].fromConstantPool = true;
  }

  AddPattern(pattern);
  return true;
}

// Match this pattern
// %1 = cmp %2 %3
// %6 = select %1 $4 %5
// to
// %1 = cmp %2 %3
// %6  = bfn %1 %4 %5
//
// For the original cmp-sel sequence, a flag-based sequence is generated.
// We instead want to generate a non-flag cmp-bfn sequence which has shorter latency.
bool CodeGenPatternMatch::MatchCmpSelect(llvm::SelectInst &I) {
  struct CmpSelectPattern : public Pattern {
    uint8_t execSize;
    llvm::CmpInst::Predicate predicate;
    SSource cmpSources[2];
    uint8_t booleanFuncCtrl = 0xD8; // represents s0&s1|~s0&s2
    SSource bfnSources[3];
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      pass->CmpBfn(predicate, cmpSources, booleanFuncCtrl, bfnSources, modifier);
    }
  };

  if (IGC_IS_FLAG_DISABLED(EnableBfn) || !m_Platform.supportBfnInstruction()) {
    return false;
  }

  if (llvm::CmpInst *cmp = llvm::dyn_cast<llvm::CmpInst>(I.getOperand(0))) {
    // handle one use for now
    if (!cmp->hasOneUse()) {
      return false;
    }

    // BFN only supports D & W types.
    Type *selTy = I.getType();
    if (!(selTy->isIntegerTy(16) || selTy->isIntegerTy(32))) {
      return false;
    }

    Type *cmpS0Ty = cmp->getOperand(0)->getType();
    if (selTy->getPrimitiveSizeInBits() != cmpS0Ty->getPrimitiveSizeInBits()) {
      return false;
    }

    llvm::Value *selSources[2] = {};
    e_modifier selMod[2] = {};
    selSources[0] = I.getOperand(1);
    selSources[1] = I.getOperand(2);

    // BFN only supports 16bit immediate
    if ((isa<Constant>(selSources[0]) && selSources[0]->getType()->isIntegerTy(32)) ||
        (isa<Constant>(selSources[1]) && selSources[1]->getType()->isIntegerTy(32))) {
      return false;
    }

    // As BFN doesn't support src modifier, it is not worth to generate the cmp-bfn
    // sequence if one of its sources will need an extra move.
    if (GetModifier(*selSources[0], selMod[0], selSources[0]) ||
        GetModifier(*selSources[1], selMod[1], selSources[1])) {
      return false;
    }

    CmpSelectPattern *pattern = new (m_allocator) CmpSelectPattern();
    pattern->predicate = cmp->getPredicate();
    bool supportsModifier = SupportsModifier(cmp, m_Platform);
    pattern->cmpSources[0] = GetSource(cmp->getOperand(0), supportsModifier, false, IsSourceOfSample(&I));
    pattern->cmpSources[1] = GetSource(cmp->getOperand(1), supportsModifier, false, IsSourceOfSample(&I));

    pattern->bfnSources[1] = GetSource(selSources[0], false, false, IsSourceOfSample(&I));
    pattern->bfnSources[2] = GetSource(selSources[1], false, false, IsSourceOfSample(&I));
    AddPattern(pattern);

    return true;
  }
  return false;
}

bool CodeGenPatternMatch::MatchDpas(GenIntrinsicInst &I) {
  struct DpasPattern : public Pattern {
    SSource source[3];
    GenIntrinsicInst *instruction;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      pass->emitDpas(instruction, source, modifier);
      // pass->BinaryUnary(instruction, source, modifier);
    }
  };

  GenISAIntrinsic::ID dpasID = I.getIntrinsicID();
  IGC_ASSERT_MESSAGE((dpasID == GenISAIntrinsic::GenISA_dpas || dpasID == GenISAIntrinsic::GenISA_sub_group_dpas),
                     "Unexpected DPAS intrinsic!");

  Value *src0 = I.getOperand(0); // input
  Value *src1 = I.getOperand(1); // activation. operand(3) is its precision
  Value *src2 = I.getOperand(2); // weight. operand(4) is its precision
  // ConstantInt* pa = dyn_cast<ConstantInt>(I.getOperand(3));
  // ConstantInt* pb = dyn_cast<ConstantInt>(I.getOperand(4));
  ConstantInt *sdepth = cast<ConstantInt>(I.getOperand(5));
  ConstantInt *rcount = cast<ConstantInt>(I.getOperand(6));
  int SD = (int)sdepth->getZExtValue();
  int RC = (int)rcount->getZExtValue();

  if (dpasID == GenISAIntrinsic::GenISA_dpas && RC == 1 && SD == 8) {
    // Special-handling of activation (src1 - uniform). The case handled is:
    //
    // %s0 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %5, i32 0)
    // %s1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %5, i32 1)
    // %s2 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %5, i32 2)
    // %s3 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %5, i32 3)
    // %s4 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %5, i32 4)
    // %s5 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %5, i32 5)
    // %s6 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %5, i32 6)
    // %s7 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %5, i32 7)
    // %a0 = insertelement <8 x i32> undef, i32 %s0, i32 0
    // %a1 = insertelement <8 x i32> %a0,   i32 %s1, i32 1
    // %a2 = insertelement <8 x i32> %a1,   i32 %s2, i32 2
    // %a3 = insertelement <8 x i32> %a2,   i32 %s3, i32 3
    // %a4 = insertelement <8 x i32> %a3,   i32 %s4, i32 4
    // %a5 = insertelement <8 x i32> %a4,   i32 %s5, i32 5
    // %a6 = insertelement <8 x i32> %a5,   i32 %s6, i32 6
    // %a7 = insertelement <8 x i32> %a6,   i32 %s7, i32 7
    //
    // %c0 = call i32 @llvm.genx.GenISA.dpas.8(i32 %9, <8 x i32> %a7, i32 7, <8 x i32> %14, i32 7)
    //
    // %a7 will be replaced with %5 in vISA.
    //

    InsertElementInst *rootIEI = dyn_cast<InsertElementInst>(src1);
    if (rootIEI) {
      // Currently, the src1 can be at most int8.
      Value *srcVec[8];
      Value *WSVal = nullptr;
      for (int i = 0; i < 8; srcVec[i] = nullptr, ++i)
        ;
      InsertElementInst *IEI = rootIEI;
      while (IEI) {
        Value *elem = IEI->getOperand(1);
        ConstantInt *CI = dyn_cast<ConstantInt>(IEI->getOperand(2));
        if (!CI) {
          // Only handle constant index
          WSVal = nullptr;
          break;
        }
        uint32_t ix = (uint32_t)CI->getZExtValue();
        if (ix > 7 || srcVec[ix]) {
          // Assume each element is inserted once.
          WSVal = nullptr;
          break;
        }

        // Check if the inserted element is created
        // by WaveShuffleIndex intrinsic
        GenIntrinsicInst *WSI = dyn_cast<GenIntrinsicInst>(elem);
        if (!WSI || (WSI->getIntrinsicID() != GenISAIntrinsic::GenISA_WaveShuffleIndex &&
                     WSI->getIntrinsicID() != GenISAIntrinsic::GenISA_WaveBroadcast)) {
          WSVal = nullptr;
          break;
        }
        Value *shuffleVal = WSI->getOperand(0);
        Value *ixVal = WSI->getOperand(1);
        if (WSVal == nullptr) {
          WSVal = shuffleVal;
        } else if (WSVal != shuffleVal) {
          WSVal = nullptr;
          break;
        }
        if (ConstantInt *CIX = dyn_cast<ConstantInt>(ixVal)) {
          if ((uint32_t)CIX->getZExtValue() != ix) {
            WSVal = nullptr;
            break;
          }
        }
        srcVec[ix] = elem;
        IEI = dyn_cast<InsertElementInst>(IEI->getOperand(0));
      }

      if (WSVal) {
        for (int i = 0; i < SD; ++i) {
          if (srcVec[i] == nullptr) {
            WSVal = nullptr;
            break;
          }
        }
      }

      // If WSVal is set at this point, it is one that will
      // replace src1.
      if (WSVal) {
        src1 = WSVal;
      }
    }
  }

  DpasPattern *pattern = new (m_allocator) DpasPattern();
  pattern->instruction = &I;
  pattern->source[0] = GetSource(src0, false, false, IsSourceOfSample(&I));
  pattern->source[1] = GetSource(src1, false, false, IsSourceOfSample(&I));
  pattern->source[2] = GetSource(src2, false, false, IsSourceOfSample(&I));
  AddPattern(pattern);

  return true;
}

bool CodeGenPatternMatch::MatchDp4a(GenIntrinsicInst &I) {
  struct MatchDp4a : public Pattern {
    SSource source[3];
    GenIntrinsicInst *instruction;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) { pass->emitDP4A(instruction, source, modifier); }
  };

  // Attempt to find a pattern like this:
  // %scalar52.6.2474 = extractelement <4 x i8> %145, i32 0
  // %scalar53.6.2475 = extractelement <4 x i8> % 145, i32 1
  // %scalar54.6.2476 = extractelement <4 x i8> % 145, i32 2
  // %scalar55.6.2477 = extractelement <4 x i8> % 145, i32 3
  // %simdShuffle.6.2480 = call i8 @llvm.genx.GenISA.WaveShuffleIndex.i8(i8 % scalar52.6.2474, i32 0, i32 0)
  // %simdShuffle33.6.2481 = call i8 @llvm.genx.GenISA.WaveShuffleIndex.i8(i8 % scalar53.6.2475, i32 0, i32 0)
  // %simdShuffle34.6.2482 = call i8 @llvm.genx.GenISA.WaveShuffleIndex.i8(i8 % scalar54.6.2476, i32 0, i32 0)
  // %simdShuffle35.6.2483 = call i8 @llvm.genx.GenISA.WaveShuffleIndex.i8(i8 % scalar55.6.2477, i32 0, i32 0)
  // %assembled.vect.6.2484 = insertelement <4 x i8> undef, i8 % simdShuffle.6.2480, i32 0
  // %assembled.vect56.6.2485 = insertelement <4 x i8> % assembled.vect.6.2484, i8 % simdShuffle33.6.2481, i32 1
  // %assembled.vect57.6.2486 = insertelement <4 x i8> % assembled.vect56.6.2485, i8 % simdShuffle34.6.2482, i32 2
  // %assembled.vect58.6.2487 = insertelement <4 x i8> % assembled.vect57.6.2486, i8 % simdShuffle35.6.2483, i32 3
  // %astype.i45.6.2489 = bitcast <4 x i8> % assembled.vect58.6.2487 to i32
  // %call.i4738.6.2490 = call i32 @llvm.genx.GenISA.dp4a.uu(i32 % call.i4738.6.3.1, i32 % astype.i45.6.2489, i32 % 135)
  // %call.i1239.6.2492 = call i32 @llvm.genx.GenISA.dp4a.uu(i32 % call.i1239.6.3.1, i32 % astype.i45.6.2489, i32
  // 16843009)
  //
  // If the pattern is found, we can avoid creating extra movs by changing the source of the dp4a
  // to use the input from the bitcast instead of using the result of the bitcast.
  // This pattern only happens if the WaveShuffleIndex uses 0,0 for the arguments.
  if (llvm::BitCastInst *bcInst = llvm::dyn_cast<BitCastInst>(I.getOperand(1))) {
    llvm::CallInst *waveInst[4] = {nullptr};
    llvm::InsertElementInst *insertInst[4] = {nullptr};

    // Find the 4 x insertelement
    insertInst[3] = llvm::dyn_cast<InsertElementInst>(bcInst->getOperand(0));
    if (insertInst[3]) {
      for (int i = 2; i >= 0; i--) {
        insertInst[i] = llvm::dyn_cast<InsertElementInst>(insertInst[i + 1]->getOperand(0));
        if (!insertInst[i])
          break;
      }
    }

    // Find the 4 x WaveShuffleIndex
    for (int i = 3; i >= 0; i--) {
      if (!insertInst[i])
        break;
      CallInst *temp = llvm::dyn_cast<CallInst>(insertInst[i]->getOperand(1));
      if (!temp)
        break;

      llvm::GenIntrinsicInst *intrin = llvm::dyn_cast<llvm::GenIntrinsicInst>(temp);
      if (!intrin || (intrin->getIntrinsicID() != GenISAIntrinsic::GenISA_WaveShuffleIndex &&
                      intrin->getIntrinsicID() != GenISAIntrinsic::GenISA_WaveBroadcast))
        break;
      waveInst[i] = temp;
    }

    // Check to see if the WaveShuffleIndex uses 0,0
    llvm::Constant *wavesrc1 = nullptr, *wavesrc2 = nullptr;
    if (waveInst[0]) {
      wavesrc1 = llvm::dyn_cast<llvm::Constant>(waveInst[0]->getOperand(1));
      wavesrc2 = llvm::dyn_cast<llvm::Constant>(waveInst[0]->getOperand(2));
    }

    if (wavesrc1 && wavesrc2 && wavesrc1->isZeroValue() && wavesrc2->isZeroValue()) {
      if (llvm::ExtractElementInst *wavesrc0 = llvm::dyn_cast<llvm::ExtractElementInst>(waveInst[0]->getOperand(0))) {
        llvm::ExtractElementInst *extractInst[4];

        // Find the 4 x extractelement
        extractInst[0] = llvm::dyn_cast<llvm::ExtractElementInst>(waveInst[0]->getOperand(0));
        extractInst[1] = llvm::dyn_cast<llvm::ExtractElementInst>(waveInst[1]->getOperand(0));
        extractInst[2] = llvm::dyn_cast<llvm::ExtractElementInst>(waveInst[2]->getOperand(0));
        extractInst[3] = llvm::dyn_cast<llvm::ExtractElementInst>(waveInst[3]->getOperand(0));

        if (extractInst[0] && extractInst[1] && extractInst[2] && extractInst[3] &&
            extractInst[0]->getOperand(0) == extractInst[1]->getOperand(0) &&
            extractInst[0]->getOperand(0) == extractInst[2]->getOperand(0) &&
            extractInst[0]->getOperand(0) == extractInst[3]->getOperand(0)) {
          MatchDp4a *pattern = new (m_allocator) MatchDp4a();
          pattern->instruction = &I;
          pattern->source[0] = GetSource(I.getOperand(0), false, false, IsSourceOfSample(&I));

          // set regioning to: <0;1,0>, as if we were still going to use the result of the WaveShuffleIndex
          llvm::BitCastInst *bitCast = llvm::dyn_cast<BitCastInst>(extractInst[0]->getOperand(0));
          if (bitCast) {
            pattern->source[1] = GetSource(bitCast->getOperand(0), false, false, IsSourceOfSample(&I));
            pattern->source[1].region[0] = 0;
            pattern->source[1].region[1] = 1;
            pattern->source[1].region[2] = 0;
            pattern->source[1].region_set = true;

            pattern->source[2] = GetSource(I.getOperand(2), false, false, IsSourceOfSample(&I));
            AddPattern(pattern);
            return true;
          }
        }
      }
    }
  }
  return MatchSingleInstruction(I);
}

bool CodeGenPatternMatch::MatchLogicAlu(llvm::BinaryOperator &I) {
  struct LogicInstPattern : public Pattern {
    SSource sources[2];
    llvm::Instruction *instruction;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      pass->BinaryUnary(instruction, sources, modifier);
    }
  };
  LogicInstPattern *pattern = new (m_allocator) LogicInstPattern();
  pattern->instruction = &I;
  for (unsigned int i = 0; i < 2; ++i) {
    e_modifier mod = EMOD_NONE;
    Value *src = I.getOperand(i);
    if (!I.getType()->isIntegerTy(1)) {
      if (BinaryOperator *notInst = dyn_cast<BinaryOperator>(src)) {
        if (notInst->getOpcode() == Instruction::Xor) {
          if (ConstantInt *minusOne = dyn_cast<ConstantInt>(notInst->getOperand(1))) {
            if (minusOne->isMinusOne()) {
              mod = EMOD_NOT;
              src = notInst->getOperand(0);
            }
          }
        }
      }
    }
    pattern->sources[i] = GetSource(src, mod, false, IsSourceOfSample(&I));

    if (isCandidateForConstantPool(src)) {
      AddToConstantPool(I.getParent(), src);
      pattern->sources[i].fromConstantPool = true;
    }
  }
  AddPattern(pattern);
  return true;
}

bool CodeGenPatternMatch::MatchRsqrt(llvm::BinaryOperator &I) {
  struct RsqrtPattern : public Pattern {
    SSource source;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) { pass->Rsqrt(source, modifier); }
  };

  bool found = false;
  llvm::Value *source = NULL;
  if (I.getOpcode() == Instruction::FDiv) {
    // by vISA document, rsqrt doesn't support double type
    if (isOne(I.getOperand(0)) && I.getType()->getTypeID() != Type::DoubleTyID) {
      if (llvm::IntrinsicInst *sqrt = dyn_cast<IntrinsicInst>(I.getOperand(1))) {
        if (sqrt->getIntrinsicID() == Intrinsic::sqrt) {
          source = sqrt->getOperand(0);
          found = true;
        }
      }
    }
  }
  if (found) {
    RsqrtPattern *pattern = new (m_allocator) RsqrtPattern();
    pattern->source = GetSource(source, true, false, IsSourceOfSample(&I));
    AddPattern(pattern);
  }
  return found;
}

bool CodeGenPatternMatch::MatchArcpFdiv(llvm::BinaryOperator &I) {

  using namespace llvm::PatternMatch;

  struct ArcpFdivPattern : public Pattern {
    SSource sources[2];
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) { pass->FDiv(sources, modifier); }
  };

  if (!I.getType()->isDoubleTy() || !I.hasAllowReciprocal())
    return false;

  // Look for fdiv.
  Instruction *fdiv = nullptr;
  Value *dividend = nullptr, *divisor = nullptr;

  auto fdivPattern = m_OneUse(m_FDiv(m_FPOne(), m_Value(divisor)));

  if (match(I.getOperand(0), fdivPattern)) {
    fdiv = dyn_cast<Instruction>(I.getOperand(0));
    dividend = I.getOperand(1);
  } else if (match(I.getOperand(1), fdivPattern)) {
    fdiv = dyn_cast<Instruction>(I.getOperand(1));
    dividend = I.getOperand(0);
  }

  if (!fdiv || !fdiv->hasAllowReciprocal())
    return false;

  // Pattern found.
  ArcpFdivPattern *pattern = new (m_allocator) ArcpFdivPattern();
  Value *sources[2] = {dividend, divisor};
  e_modifier src_mod[2] = {};

  if (FlushesDenormsOnInput(*fdiv)) {
    sources[0] = SkipCanonicalize(sources[0]);
    sources[1] = SkipCanonicalize(sources[1]);
  }

  GetModifier(*sources[0], src_mod[0], sources[0]);
  GetModifier(*sources[1], src_mod[1], sources[1]);

  pattern->sources[0] = GetSource(sources[0], src_mod[0], false, IsSourceOfSample(&I));
  pattern->sources[1] = GetSource(sources[1], src_mod[1], false, IsSourceOfSample(&I));

  // Try to add to constant pool whatever possible.
  if (isCandidateForConstantPool(sources[0])) {
    AddToConstantPool(I.getParent(), sources[0]);
    pattern->sources[0].fromConstantPool = true;
  }
  if (isCandidateForConstantPool(sources[1])) {
    AddToConstantPool(I.getParent(), sources[1]);
    pattern->sources[1].fromConstantPool = true;
  }

  AddPattern(pattern);

  return true;
}

bool CodeGenPatternMatch::MatchGradient(llvm::GenIntrinsicInst &I) {
  struct GradientPattern : public Pattern {
    SSource sources[2];
    llvm::Instruction *instruction;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      pass->BinaryUnary(instruction, sources, modifier);
    }
  };
  GradientPattern *pattern = new (m_allocator) GradientPattern();
  pattern->instruction = &I;
  pattern->sources[0] = GetSource(I.getOperand(0), true, false, IsSourceOfSample(&I));
  pattern->sources[1] = {};
  AddPattern(pattern);
  // mark the source as subspan use
  HandleSubspanUse(pattern->sources[0].value, IsSourceOfSample(&I));
  return true;
}

bool CodeGenPatternMatch::MatchSampleDerivative(llvm::GenIntrinsicInst &I) {
  HandleSampleDerivative(I);
  return MatchSingleInstruction(I);
}

bool CodeGenPatternMatch::MatchDbgInstruction(llvm::DbgInfoIntrinsic &I) {
  struct DbgInstPattern : Pattern {
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      // Nothing to emit.
    }
  };
  DbgInstPattern *pattern = new (m_allocator) DbgInstPattern();
  AddPattern(pattern);
  return true;
}

bool CodeGenPatternMatch::MatchAvg(llvm::Instruction &I) {
  // "Average value" pattern:
  // (x + y + 1) / 2  -->  avg(x, y)
  //
  // We're looking for patterns like this:
  //    % 14 = add nsw i32 % 10, % 13
  //    % 15 = add nsw i32 % 14, 1
  //    % 16 = ashr i32 % 15, 1

  struct AvgPattern : Pattern {
    SSource sources[2];
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) { pass->Avg(sources, modifier); }
  };

  bool found = false;
  llvm::Value *sources[2] = {};
  e_modifier src_mod[2] = {};

  IGC_ASSERT(I.getOpcode() == Instruction::SDiv || I.getOpcode() == Instruction::UDiv ||
             I.getOpcode() == Instruction::AShr);

  // We expect 2 for "div" and 1 for "right shift".
  int expectedVal = (I.getOpcode() == Instruction::SDiv ? 2 : 1);
  Value *opnd1 = I.getOperand(1); // Divisor or shift factor.
  if (!isa<ConstantInt>(opnd1) || (cast<ConstantInt>(opnd1))->getZExtValue() != expectedVal) {
    return false;
  }

  if (Instruction *divSrc = dyn_cast<Instruction>(I.getOperand(0))) {
    if (divSrc->getOpcode() == Instruction::Add && !NeedInstruction(*divSrc)) {
      Instruction *instAdd = cast<Instruction>(divSrc);
      for (int i = 0; i < 2; i++) {
        if (ConstantInt *cnst = dyn_cast<ConstantInt>(instAdd->getOperand(i))) {
          // "otherArg" is the second argument of "instAdd" (which is not constant).
          Value *otherArg = instAdd->getOperand(i == 0 ? 1 : 0);
          if (cnst->getZExtValue() == 1 && isa<AddOperator>(otherArg) &&
              !NeedInstruction(*cast<Instruction>(otherArg))) {
            Instruction *firstAdd = cast<Instruction>(otherArg);
            sources[0] = firstAdd->getOperand(0);
            sources[1] = firstAdd->getOperand(1);
            GetModifier(*sources[0], src_mod[0], sources[0]);
            GetModifier(*sources[1], src_mod[1], sources[1]);
            found = true;
            break;
          }
        }
      }
    }
  }

  if (found) {
    AvgPattern *pattern = new (m_allocator) AvgPattern();
    pattern->sources[0] = GetSource(sources[0], src_mod[0], false, IsSourceOfSample(&I));
    pattern->sources[1] = GetSource(sources[1], src_mod[1], false, IsSourceOfSample(&I));
    AddPattern(pattern);
  }
  return found;
}

bool CodeGenPatternMatch::MatchShuffleBroadCast(llvm::GenIntrinsicInst &I) {
  // Match cases like:
  //    %84 = bitcast <2 x i32> %vCastload to <4 x half>
  //    %scalar269 = extractelement <4 x half> % 84, i32 0
  //    %simdShuffle = call half @llvm.genx.GenISA.simdShuffle.f.f16(half %scalar269, i32 0)
  //
  // to mov with region and offset
  struct BroadCastPattern : public Pattern {
    SSource source;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) { pass->Mov(source, modifier); }
  };
  bool match = false;
  SSource source;
  Value *sourceV = &I;
  if (GetRegionModifier(source, sourceV, true)) {
    BroadCastPattern *pattern = new (m_allocator) BroadCastPattern();
    GetModifier(*sourceV, source.mod, sourceV);
    source.value = sourceV;
    pattern->source = source;
    MarkAsSource(sourceV, IsSourceOfSample(&I));
    match = true;
    AddPattern(pattern);
  }
  return match;
}

bool CodeGenPatternMatch::MatchWaveShuffleIndex(llvm::GenIntrinsicInst &I) {
  auto helperLaneMode = cast<ConstantInt>(I.getOperand(2));
  IGC_ASSERT(helperLaneMode);
  if (int_cast<int>(helperLaneMode->getSExtValue()) == 1) {
    // only if helperLaneMode==1, we enable helper lane under some shuffleindex cases (not for all cases).
    HandleSubspanUse(I.getArgOperand(0), IsSourceOfSample(&I));
    HandleSubspanUse(I.getArgOperand(1), IsSourceOfSample(&I));
    HandleSubspanUse(&I, IsSourceOfSample(&I));
  }
  return MatchSingleInstruction(I);
}

bool CodeGenPatternMatch::MatchWaveInstruction(llvm::GenIntrinsicInst &I) {
  if (subgroupIntrinsicHasHelperLanes(I)) {
    m_NeedVMask = true;
  }
  return MatchSingleInstruction(I);
}

bool CodeGenPatternMatch::MatchRegisterRegion(llvm::GenIntrinsicInst &I) {
  struct MatchRegionPattern : public Pattern {
    SSource source;
    virtual void Emit(EmitPass *pass, const DstModifier &modifier) {
      const bool isSimd32Dispatch = (pass->m_currShader->m_State.m_dispatchSize == SIMDMode::SIMD32);
      if (isSimd32Dispatch && pass->m_currShader->m_numberInstance == 2) {
        pass->emitCrossInstanceMov(source, modifier);
      } else {
        pass->Mov(source, modifier);
      }
    }
  };

  /*
  * Match case 1 - With SubReg Offset: Shuffle( data, (laneID << x) + y )
  *   %25 = call i16 @llvm.genx.GenISA.simdLaneId()
  *   %30 = zext i16 %25 to i32
  *   %31 = shl nuw nsw i32 %30, 1  - Current LaneID shifted by x
  *   %36 = add i32 %31, 1          - Current LaneID shifted by x + y  Shuffle( data, (laneID << x) + 1 )
  *   %37 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %21, i32 %36)

  * Match case 2(Special case of Match Case 1) - No SubReg Offset: Shuffle( data, (laneID << x) + 0 )
  *    %25 = call i16 @llvm.genx.GenISA.simdLaneId()
  *    %30 = zext i16 %25 to i32
  *    %31 = shl nuw nsw i32 %30, 1 - Current LaneID shifted by x
  *    %32 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %21, i32 %31)
  */

  Value *data = I.getOperand(0);
  Value *source = I.getOperand(1);
  uint typeByteSize = data->getType()->getScalarSizeInBits() / 8;
  bool isMatch = false;
  int subReg = 0;
  uint verticalStride = 1; // Default value for special case  Shuffle( data, (laneID << x) + y )  when x = 0

  if (auto binaryInst = dyn_cast<BinaryOperator>(source)) {
    // Will be skipped for match case 2
    if (binaryInst->getOpcode() == Instruction::Add) {
      if (llvm::ConstantInt *simDOffSetInst = llvm::dyn_cast<llvm::ConstantInt>(binaryInst->getOperand(1))) {
        subReg = int_cast<int>(cast<ConstantInt>(simDOffSetInst)->getSExtValue());

        // Subregister must be a number between 0 and 15 for a valid region
        //  We could support up to 31 but we need to handle reading from different SIMD16 var chunks
        if (subReg >= 0 && subReg < 16) {
          source = binaryInst->getOperand(0);
        }
      }
    }
  }

  if (auto binaryInst = dyn_cast<BinaryOperator>(source)) {
    if (binaryInst->getOpcode() == Instruction::Shl) {
      source = binaryInst->getOperand(0);

      if (llvm::ConstantInt *simDOffSetInst = llvm::dyn_cast<llvm::ConstantInt>(binaryInst->getOperand(1))) {
        uint shiftFactor = int_cast<uint>(simDOffSetInst->getZExtValue());
        // Check to make sure we dont end up with an invalid Vertical Stride.
        // Only 1, 2, 4, 8, 16 are supported.
        if (shiftFactor <= 4)
          verticalStride = (1U << shiftFactor);
        else
          return false;
      }
    }
  }

  if (auto zExtInst = llvm::dyn_cast<llvm::ZExtInst>(source)) {
    source = zExtInst->getOperand(0);
  }

  llvm::GenIntrinsicInst *intrin = llvm::dyn_cast<llvm::GenIntrinsicInst>(source);

  // Finally check for simLaneID intrisic
  if (intrin && (intrin->getIntrinsicID() == GenISAIntrinsic::GenISA_simdLaneId)) {
    // To avoid compiler crash, pattern match with direct mov will be disable
    // Conservetively, we assum simd16 for 32 bytes GRF platforms and simd32 for 64 bytes GRF platforms
    bool cross2GRFs = typeByteSize * (subReg + verticalStride * (m_Platform.getGRFSize() > 32 ? 32 : 16)) >
                      (2 * m_Platform.getGRFSize());
    if (!cross2GRFs) {
      MatchRegionPattern *pattern = new (m_allocator) MatchRegionPattern();
      pattern->source.elementOffset = subReg;

      // Set Region Parameters <VerString;Width,HorzString>
      pattern->source.region_set = true;
      pattern->source.region[0] = verticalStride;
      pattern->source.region[1] = 1;
      pattern->source.region[2] = 0;

      pattern->source.value = data;
      MarkAsSource(data, IsSourceOfSample(&I));
      HandleSubspanUse(data, IsSourceOfSample(&I));
      AddPattern(pattern);

      isMatch = true;
    }
  }

  return isMatch;
}

bool CodeGenPatternMatch::GetRegionModifier(SSource &sourceMod, llvm::Value *&source, bool regioning) {
  bool found = false;
  Value *OrignalSource = source;
  if (llvm::BitCastInst *bitCast = llvm::dyn_cast<BitCastInst>(source)) {
    if (!bitCast->getType()->isVectorTy() && !bitCast->getOperand(0)->getType()->isVectorTy()) {
      source = bitCast->getOperand(0);
      found = true;
    }
  }

  if (llvm::GenIntrinsicInst *intrin = llvm::dyn_cast<llvm::GenIntrinsicInst>(source)) {
    GenISAIntrinsic::ID id = intrin->getIntrinsicID();
    if (id == GenISAIntrinsic::GenISA_WaveShuffleIndex || id == GenISAIntrinsic::GenISA_WaveBroadcast) {
      if (llvm::ConstantInt *channelVal = llvm::dyn_cast<llvm::ConstantInt>(intrin->getOperand(1))) {
        unsigned int offset = int_cast<unsigned int>(channelVal->getZExtValue());
        if (offset < 16 && !isUniform(intrin->getOperand(0))) {
          sourceMod.elementOffset = offset;
          // SIMD shuffle force region <0,1;0>
          sourceMod.region_set = true;
          sourceMod.region[0] = 0;
          sourceMod.region[1] = 1;
          sourceMod.region[2] = 0;
          sourceMod.instance = EINSTANCE_FIRST_HALF;
          source = intrin->getOperand(0);
          found = true;
          BitcastSearch(sourceMod, source, true);
        }
      }
    }
  }
  if (regioning && !sourceMod.region_set) {
    found |= BitcastSearch(sourceMod, source, false);
  }
  if (found && sourceMod.type == VISA_Type::ISA_TYPE_NUM) {
    // keep the original type
    sourceMod.type = GetType(OrignalSource->getType(), m_ctx);
  }
  return found;
}

void CodeGenPatternMatch::HandleSampleDerivative(llvm::GenIntrinsicInst &I) {
  switch (I.getIntrinsicID()) {
  case GenISAIntrinsic::GenISA_sampleptr:
  case GenISAIntrinsic::GenISA_lodptr:
  case GenISAIntrinsic::GenISA_sampleKillPix:
    HandleSubspanUse(I.getOperand(0), true);
    HandleSubspanUse(I.getOperand(1), true);
    HandleSubspanUse(I.getOperand(2), true);
    break;
  case GenISAIntrinsic::GenISA_sampleBptr:
  case GenISAIntrinsic::GenISA_sampleCptr:
    HandleSubspanUse(I.getOperand(1), true);
    HandleSubspanUse(I.getOperand(2), true);
    HandleSubspanUse(I.getOperand(3), true);
    break;
  case GenISAIntrinsic::GenISA_sampleBCptr:
    HandleSubspanUse(I.getOperand(2), true);
    HandleSubspanUse(I.getOperand(3), true);
    HandleSubspanUse(I.getOperand(4), true);
    break;
  case GenISAIntrinsic::GenISA_gather4Iptr:
  case GenISAIntrinsic::GenISA_gather4IPOptr:
  case GenISAIntrinsic::GenISA_gather4POPackedIptr:
    HandleSubspanUse(I.getOperand(0), true);
    HandleSubspanUse(I.getOperand(1), true);
    HandleSubspanUse(I.getOperand(2), true);
    break;
  case GenISAIntrinsic::GenISA_samplePOptr:
    HandleSubspanUse(I.getOperand(0), true);
    HandleSubspanUse(I.getOperand(1), true);
    HandleSubspanUse(I.getOperand(2), true);
    break;
  case GenISAIntrinsic::GenISA_gather4Bptr:
  case GenISAIntrinsic::GenISA_gather4BPOptr:
  case GenISAIntrinsic::GenISA_gather4ICptr:
  case GenISAIntrinsic::GenISA_gather4ICPOptr:
  case GenISAIntrinsic::GenISA_gather4POPackedBptr:
  case GenISAIntrinsic::GenISA_gather4POPackedICptr:
    HandleSubspanUse(I.getOperand(1), true);
    HandleSubspanUse(I.getOperand(2), true);
    HandleSubspanUse(I.getOperand(3), true);
    break;
  case GenISAIntrinsic::GenISA_sampleMlodptr:
    HandleSubspanUse(I.getOperand(1), true);
    HandleSubspanUse(I.getOperand(2), true);
    HandleSubspanUse(I.getOperand(3), true);
    break;
  case GenISAIntrinsic::GenISA_sampleCMlodptr:
  case GenISAIntrinsic::GenISA_sampleBCMlodptr:
    HandleSubspanUse(I.getOperand(2), true);
    HandleSubspanUse(I.getOperand(3), true);
    HandleSubspanUse(I.getOperand(4), true);
    break;
  case GenISAIntrinsic::GenISA_samplePOBptr:
  case GenISAIntrinsic::GenISA_samplePOCptr:
    HandleSubspanUse(I.getOperand(1), true);
    HandleSubspanUse(I.getOperand(2), true);
    HandleSubspanUse(I.getOperand(3), true);
    break;
  default:
    break;
  }
}

// helper function for pattern match
static inline bool isLowerPredicate(llvm::CmpInst::Predicate pred) {
  switch (pred) {
  case llvm::CmpInst::FCMP_ULT:
  case llvm::CmpInst::FCMP_ULE:
  case llvm::CmpInst::FCMP_OLT:
  case llvm::CmpInst::FCMP_OLE:
  case llvm::CmpInst::ICMP_ULT:
  case llvm::CmpInst::ICMP_ULE:
  case llvm::CmpInst::ICMP_SLT:
  case llvm::CmpInst::ICMP_SLE:
    return true;
  default:
    break;
  }
  return false;
}

// helper function for pattern match
static inline bool isGreaterOrLowerPredicate(llvm::CmpInst::Predicate pred) {
  switch (pred) {
  case llvm::CmpInst::FCMP_UGT:
  case llvm::CmpInst::FCMP_UGE:
  case llvm::CmpInst::FCMP_ULT:
  case llvm::CmpInst::FCMP_ULE:
  case llvm::CmpInst::FCMP_OGT:
  case llvm::CmpInst::FCMP_OGE:
  case llvm::CmpInst::FCMP_OLT:
  case llvm::CmpInst::FCMP_OLE:
  case llvm::CmpInst::ICMP_UGT:
  case llvm::CmpInst::ICMP_UGE:
  case llvm::CmpInst::ICMP_ULT:
  case llvm::CmpInst::ICMP_ULE:
  case llvm::CmpInst::ICMP_SGT:
  case llvm::CmpInst::ICMP_SGE:
  case llvm::CmpInst::ICMP_SLT:
  case llvm::CmpInst::ICMP_SLE:
    return true;
  default:
    break;
  }
  return false;
}

static bool isIntegerAbs(SelectInst *SI, e_modifier &mod, Value *&source) {
  using namespace llvm::PatternMatch; // Scoped using declaration.

  Value *Cond = SI->getOperand(0);
  Value *TVal = SI->getOperand(1);
  Value *FVal = SI->getOperand(2);

  ICmpInst::Predicate IPred = FCmpInst::FCMP_FALSE;
  Value *LHS = nullptr;
  Value *RHS = nullptr;

  if (!match(Cond, m_ICmp(IPred, m_Value(LHS), m_Value(RHS))))
    return false;

  if (!ICmpInst::isSigned(IPred))
    return false;

  if (match(LHS, m_Zero())) {
    IPred = ICmpInst::getSwappedPredicate(IPred);
    std::swap(LHS, RHS);
  }

  if (!match(RHS, m_Zero()))
    return false;

  if (match(TVal, m_Neg(m_Specific(FVal)))) {
    IPred = ICmpInst::getInversePredicate(IPred);
    std::swap(TVal, FVal);
  }

  if (!match(FVal, m_Neg(m_Specific(TVal))))
    return false;

  if (LHS != TVal)
    return false;

  source = TVal;
  mod = (IPred == ICmpInst::ICMP_SGT || IPred == ICmpInst::ICMP_SGE) ? EMOD_ABS : EMOD_NEGABS;

  return true;
}

bool isAbs(llvm::Value *abs, e_modifier &mod, llvm::Value *&source) {
  bool found = false;

  if (IntrinsicInst *intrinsicInst = dyn_cast<IntrinsicInst>(abs)) {
    if (intrinsicInst->getIntrinsicID() == Intrinsic::fabs) {
      source = intrinsicInst->getOperand(0);
      mod = EMOD_ABS;
      return true;
    }
    if (intrinsicInst->getIntrinsicID() == Intrinsic::abs) {
      source = intrinsicInst->getOperand(0);
      mod = EMOD_ABS;
      return true;
    }
  }

  llvm::SelectInst *select = llvm::dyn_cast<llvm::SelectInst>(abs);
  if (!select)
    return false;

  // Try to find floating point abs first
  if (llvm::FCmpInst *cmp = llvm::dyn_cast<llvm::FCmpInst>(select->getOperand(0))) {
    llvm::CmpInst::Predicate pred = cmp->getPredicate();
    if (isGreaterOrLowerPredicate(pred)) {
      for (int zeroIndex = 0; zeroIndex < 2; zeroIndex++) {
        llvm::ConstantFP *zero = llvm::dyn_cast<llvm::ConstantFP>(cmp->getOperand(zeroIndex));
        if (zero && zero->isZero()) {
          llvm::Value *cmpSource = cmp->getOperand(1 - zeroIndex);
          for (int sourceIndex = 0; sourceIndex < 2; sourceIndex++) {
            if (cmpSource == select->getOperand(1 + sourceIndex)) {
              llvm::Instruction *opnd = llvm::dyn_cast<llvm::Instruction>(select->getOperand(1 + (1 - sourceIndex)));
              llvm::Value *negateSource = NULL;
              if (opnd && IsNegate(opnd, negateSource) && negateSource == cmpSource) {
                found = true;
                source = cmpSource;
                // depending on the order source in cmp/select it can abs() or -abs()
                bool isNegateAbs = (zeroIndex == 0) ^ isLowerPredicate(pred) ^ (sourceIndex == 1);
                mod = isNegateAbs ? EMOD_NEGABS : EMOD_ABS;
              }
              break;
            }
          }
          break;
        }
      }
    }
  }

  // If not found, try integer abs
  return found || isIntegerAbs(select, mod, source);
}

// combine two modifiers, this function is *not* communtative
e_modifier CombineModifier(e_modifier mod1, e_modifier mod2) {
  e_modifier mod = EMOD_NONE;
  switch (mod1) {
  case EMOD_ABS:
  case EMOD_NEGABS:
    mod = mod1;
    break;
  case EMOD_NEG:
    if (mod2 == EMOD_NEGABS) {
      mod = EMOD_ABS;
    } else if (mod2 == EMOD_ABS) {
      mod = EMOD_NEGABS;
    } else if (mod2 == EMOD_NEG) {
      mod = EMOD_NONE;
    } else {
      mod = EMOD_NEG;
    }
    break;
  default:
    mod = mod2;
  }
  return mod;
}

bool GetModifier(llvm::Value &modifier, e_modifier &mod, llvm::Value *&source) {
  mod = EMOD_NONE;
  if (llvm::Instruction *bin = llvm::dyn_cast<llvm::Instruction>(&modifier)) {
    return GetModifier(*bin, mod, source);
  }
  return false;
}

bool GetModifier(llvm::Instruction &modifier, e_modifier &mod, llvm::Value *&source) {
  llvm::Value *modifierSource = NULL;
  mod = EMOD_NONE;
  if (IsNegate(&modifier, modifierSource)) {
    e_modifier absModifier = EMOD_NONE;
    llvm::Value *absSource = NULL;
    if (isAbs(modifierSource, absModifier, absSource)) {
      source = absSource;
      mod = IGC::CombineModifier(EMOD_NEG, absModifier);
    } else {
      source = modifierSource;
      mod = EMOD_NEG;
    }
    return true;
  } else if (isAbs(&modifier, mod, modifierSource)) {
    source = modifierSource;
    return true;
  }
  return false;
}

bool IsNegate(llvm::Instruction *inst, llvm::Value *&negateSource) {
  BinaryOperator *binop = dyn_cast<BinaryOperator>(inst);
  if (binop && (inst->getOpcode() == Instruction::FSub || inst->getOpcode() == Instruction::Sub)) {
    if (IsZero(inst->getOperand(0))) {
      negateSource = inst->getOperand(1);
      return true;
    }
  }
  UnaryOperator *unop = dyn_cast<UnaryOperator>(inst);
  if (unop && inst->getOpcode() == Instruction::FNeg) {
    negateSource = inst->getOperand(0);
    return true;
  }
  return false;
}

bool IsZero(llvm::Value *zero) {
  if (llvm::ConstantFP *FCst = llvm::dyn_cast<llvm::ConstantFP>(zero)) {
    if (FCst->isZero()) {
      return true;
    }
  }
  if (llvm::ConstantInt *ICst = llvm::dyn_cast<llvm::ConstantInt>(zero)) {
    if (ICst->isZero()) {
      return true;
    }
  }
  return false;
}

inline bool isMinOrMax(llvm::Value *inst, llvm::Value *&source0, llvm::Value *&source1, bool &isMin, bool &isUnsigned) {
  bool found = false;
  llvm::Instruction *max = llvm::dyn_cast<llvm::Instruction>(inst);
  if (!max)
    return false;

  EOPCODE op = GetOpCode(max);
  if (op == llvm_min || op == llvm_max) {
    source0 = max->getOperand(0);
    source1 = max->getOperand(1);
    isUnsigned = false;
    isMin = (op == llvm_min);
    return true;
  } else if (op == llvm_select) {
    if (llvm::CmpInst *cmp = llvm::dyn_cast<llvm::CmpInst>(max->getOperand(0))) {
      auto SkipAsr = [max](uint32_t idx) {
        ConstantInt *c1 = dyn_cast<ConstantInt>(max->getOperand(1));
        ConstantInt *c2 = dyn_cast<ConstantInt>(max->getOperand(2));
        Instruction *op = dyn_cast<Instruction>(max->getOperand(idx));
        if (((c1 && c1->isZeroValue()) || (c2 && c2->isZeroValue())) && (op && op->getOpcode() == Instruction::AShr)) {
          return op->getOperand(0);
        }
        return max->getOperand(idx);
      };
      if (isGreaterOrLowerPredicate(cmp->getPredicate())) {
        if ((cmp->getOperand(0) == max->getOperand(1) && cmp->getOperand(1) == max->getOperand(2)) ||
            (cmp->getOperand(0) == max->getOperand(2) && cmp->getOperand(1) == max->getOperand(1))) {
          isMin = isLowerPredicate(cmp->getPredicate()) ^ (cmp->getOperand(0) == max->getOperand(2));
          found = true;
        } else if ((cmp->getOperand(0) == SkipAsr(1) && cmp->getOperand(1) == SkipAsr(2)) ||
                   (cmp->getOperand(0) == SkipAsr(2) && cmp->getOperand(1) == SkipAsr(1))) {
          isMin = isLowerPredicate(cmp->getPredicate()) ^ (cmp->getOperand(0) == SkipAsr(2));
          found = true;
        }
        if (found) {
          source0 = max->getOperand(1);
          source1 = max->getOperand(2);
          isUnsigned = IsUnsignedCmp(cmp->getPredicate());
        }
      }
    }
  }
  return found;
}

bool isMax(llvm::Value *max, llvm::Value *&source0, llvm::Value *&source1) {
  bool isMin, isUnsigned;
  llvm::Value *maxSource0;
  llvm::Value *maxSource1;
  if (isMinOrMax(max, maxSource0, maxSource1, isMin, isUnsigned)) {
    if (!isMin) {
      source0 = maxSource0;
      source1 = maxSource1;
      return true;
    }
  }
  return false;
}

bool isMin(llvm::Value *min, llvm::Value *&source0, llvm::Value *&source1) {
  bool isMin, isUnsigned;
  llvm::Value *maxSource0;
  llvm::Value *maxSource1;
  if (isMinOrMax(min, maxSource0, maxSource1, isMin, isUnsigned)) {
    if (isMin) {
      source0 = maxSource0;
      source1 = maxSource1;
      return true;
    }
  }
  return false;
}

bool isOne(llvm::Value *zero) {
  if (llvm::ConstantFP *FCst = llvm::dyn_cast<llvm::ConstantFP>(zero)) {
    if (FCst->isExactlyValue(1.f)) {
      return true;
    }
  }
  if (llvm::ConstantInt *ICst = llvm::dyn_cast<llvm::ConstantInt>(zero)) {
    if (ICst->isOne()) {
      return true;
    }
  }
  return false;
}

bool isSat(llvm::Instruction *sat, llvm::Value *&source, bool &isUnsigned) {
  bool found = false;
  llvm::Value *sources[2] = {0};
  bool floatMatch = sat->getType()->isFloatingPointTy();
  GenIntrinsicInst *intrin = dyn_cast<GenIntrinsicInst>(sat);
  if (intrin && (intrin->getIntrinsicID() == GenISAIntrinsic::GenISA_fsat ||
                 intrin->getIntrinsicID() == GenISAIntrinsic::GenISA_usat ||
                 intrin->getIntrinsicID() == GenISAIntrinsic::GenISA_isat)) {
    source = intrin->getOperand(0);
    found = true;
    isUnsigned = intrin->getIntrinsicID() == GenISAIntrinsic::GenISA_usat;
  } else if (floatMatch && isMax(sat, sources[0], sources[1])) {
    for (int i = 0; i < 2; i++) {
      if (IsZero(sources[i])) {
        llvm::Value *maxSources[2] = {0};
        if (isMin(sources[1 - i], maxSources[0], maxSources[1])) {
          for (int j = 0; j < 2; j++) {
            if (isOne(maxSources[j])) {
              found = true;
              source = maxSources[1 - j];
              isUnsigned = false;
              break;
            }
          }
        }
        break;
      }
    }
  } else if (floatMatch && isMin(sat, sources[0], sources[1])) {
    for (int i = 0; i < 2; i++) {
      if (isOne(sources[i])) {
        llvm::Value *maxSources[2] = {0};
        if (isMax(sources[1 - i], maxSources[0], maxSources[1])) {
          for (int j = 0; j < 2; j++) {
            if (IsZero(maxSources[j])) {
              found = true;
              source = maxSources[1 - j];
              isUnsigned = false;
              break;
            }
          }
        }
        break;
      }
    }
  }
  return found;
}

bool isCandidateForConstantPool(llvm::Value *val) {
  auto ci = dyn_cast<ConstantInt>(val);
  bool isBigQW = ci && !ci->getValue().isNullValue() && !ci->getValue().isSignedIntN(32);
  bool isDF = val->getType()->isDoubleTy();
  return (isBigQW || isDF);
}

uint CodeGenPatternMatch::GetBlockId(llvm::BasicBlock *block) {
  auto it = m_blockMap.find(block);
  IGC_ASSERT(it != m_blockMap.end());

  uint blockID = it->second->id;
  return blockID;
}

} // namespace IGC
