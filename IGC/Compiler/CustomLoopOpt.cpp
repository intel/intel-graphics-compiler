/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/LoopUtils.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/Constants.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include "llvmWrapper/IR/Function.h"
#include "common/LLVMUtils.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CustomLoopOpt.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-custom-loop-opt"
#define PASS_DESC "IGC Custom Loop Opt"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CustomLoopVersioning, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper);
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass);
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass);
IGC_INITIALIZE_PASS_DEPENDENCY(LCSSAWrapperPass)
IGC_INITIALIZE_PASS_END(CustomLoopVersioning, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

char CustomLoopVersioning::ID = 0;

CustomLoopVersioning::CustomLoopVersioning() : FunctionPass(ID) {
  initializeCustomLoopVersioningPass(*PassRegistry::getPassRegistry());
}

bool CustomLoopVersioning::isCBLoad(Value *val, unsigned &bufId, unsigned &offset) {
  LoadInst *ld = dyn_cast<LoadInst>(val);
  if (!ld)
    return false;

  unsigned as = ld->getPointerAddressSpace();
  bool directBuf = false;
  BufferType bufType = DecodeAS4GFXResource(as, directBuf, bufId);
  if (!(bufType == CONSTANT_BUFFER && directBuf))
    return false;

  Value *ptr = ld->getPointerOperand();
  if (IntToPtrInst *itop = dyn_cast<IntToPtrInst>(ptr)) {
    ConstantInt *ci = dyn_cast<ConstantInt>(itop->getOperand(0));
    if (ci) {
      offset = int_cast<unsigned>(ci->getZExtValue());
      return true;
    }
  }
  if (ConstantExpr *itop = dyn_cast<ConstantExpr>(ptr)) {
    if (itop->getOpcode() == Instruction::IntToPtr) {
      offset = int_cast<unsigned>(cast<ConstantInt>(itop->getOperand(0))->getZExtValue());
      return true;
    }
  }
  return false;
}

bool CustomLoopVersioning::runOnFunction(Function &F) {
  // Skip non-kernel function.
  IGCMD::MetaDataUtils *mdu = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  auto FII = mdu->findFunctionsInfoItem(&F);
  if (FII == mdu->end_FunctionsInfo())
    return false;

  m_cgCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  m_LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  m_DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  m_function = &F;

  bool changed = false;
  for (auto &LI : *m_LI) {
    Loop *L = &(*LI);

    // only check while loop with single BB loop body
    if (L->isSafeToClone() && L->getLoopDepth() == 1 && L->getNumBlocks() == 1 && L->getNumBackEdges() == 1 &&
        L->getHeader() == L->getExitingBlock() && L->getLoopPreheader() && L->isLCSSAForm(*m_DT)) {
      changed = processLoop(L);
      if (changed)
        break;
    }
  }

  if (changed) {
    m_cgCtx->getModuleMetaData()->psInfo.hasVersionedLoop = true;
    DumpLLVMIR(m_cgCtx, "customloop");
  }
  return changed;
}

//
// float t = ...;
// float nextT = t * CB_Load;
// [loop] while (t < loop_range_y)
// {
//     float val0 = max(t, loop_range_x);
//     float val1 = min(nextT, loop_range_y);
//     ...
//     t = nextT;
//     nextT *= CB_Load;
// }
//
// pre_header:
//   %cb = load float, float addrspace(65538)* ...
//   %nextT_start = fmul float %t_start, %cb
//
// loop_header:
//   %t = phi float [ %t_start, %then409 ], [ %nextT, %break_cont ]
//   %nextT = phi float [ %nextT_start, %then409 ], [ %res_s588, %break_cont ]
//   %cond = fcmp ult float %t, %loop_range_y
//   br i1 %cond, label %break_cont, label %after_loop
//
// loop_body:
//   %206 = call float @llvm.maxnum.f32(float %loop_range_x, float %t)
//   %207 = call float @llvm.minnum.f32(float %loop_range_y, float %nextT)
//   ...
//   %258 = load float, float addrspace(65538)* ...
//   %res_s588 = fmul float %nextT, %258
//   br label %loop_entry
//
//
bool CustomLoopVersioning::detectLoop(Loop *loop, Value *&var_range_x, Value *&var_range_y,
                                      LoadInst *&var_CBLoad_preHdr, Value *&var_t_preHdr, Value *&var_nextT_preHdr) {
  BasicBlock *preHdr = loop->getLoopPreheader();
  BasicBlock *header = loop->getHeader();
  BasicBlock *body = loop->getLoopLatch();

  Instruction *i0 = body->getFirstNonPHIOrDbg();
  Instruction *i1 = i0->getNextNonDebugInstruction();

  CallInst *imax = dyn_cast<CallInst>(i0);
  CallInst *imin = i1 ? dyn_cast<CallInst>(i1) : nullptr;

  if (!(imax && GetOpCode(imax) == llvm_max && imin && GetOpCode(imin) == llvm_min)) {
    return false;
  }

  CallInst *interval_x = dyn_cast<CallInst>(imax->getArgOperand(0));
  CallInst *interval_y = dyn_cast<CallInst>(imin->getArgOperand(0));

  if (!(interval_x && GetOpCode(interval_x) == llvm_max) || !(interval_y && GetOpCode(interval_y) == llvm_min)) {
    return false;
  }
  var_range_x = interval_x;
  var_range_y = interval_y;

  PHINode *var_t;
  PHINode *var_nextT;

  var_t = dyn_cast<PHINode>(imax->getArgOperand(1));
  var_nextT = dyn_cast<PHINode>(imin->getArgOperand(1));
  if (var_t == nullptr || var_nextT == nullptr) {
    return false;
  }

  if (var_t->getParent() != header || var_nextT->getParent() != header) {
    return false;
  }

  // check for "nextT = t * CB_Load" before loop
  BinaryOperator *fmul = dyn_cast<BinaryOperator>(var_nextT->getIncomingValueForBlock(preHdr));
  if (!fmul) {
    return false;
  }
  if (fmul->getOperand(0) != var_t->getIncomingValueForBlock(preHdr)) {
    return false;
  }
  var_t_preHdr = var_t->getIncomingValueForBlock(preHdr);
  var_nextT_preHdr = var_nextT->getIncomingValueForBlock(preHdr);

  unsigned bufId, cbOffset;
  if (!isCBLoad(fmul->getOperand(1), bufId, cbOffset)) {
    return false;
  }
  var_CBLoad_preHdr = cast<LoadInst>(fmul->getOperand(1));

  // check for "t = nextT" inside loop
  if (var_t->getIncomingValueForBlock(body) != var_nextT) {
    return false;
  }

  fmul = dyn_cast<BinaryOperator>(var_nextT->getIncomingValueForBlock(body));
  if (!fmul) {
    return false;
  }

  // check for "nextT *= CB_Load" inside loop
  Value *src0 = fmul->getOperand(0);
  if (src0 != var_nextT) {
    return false;
  }

  unsigned bufId2, cbOffset2;
  if (!isCBLoad(fmul->getOperand(1), bufId2, cbOffset2)) {
    return false;
  }
  if (bufId != bufId2 || cbOffset != cbOffset2) {
    return false;
  }

  BranchInst *br = cast<BranchInst>(body->getTerminator());
  if (!br->isConditional()) {
    return false;
  }

  // check for "while (t < loop_range_y)"
  FCmpInst *fcmp = dyn_cast<FCmpInst>(br->getCondition());
  if (!fcmp || fcmp->getOperand(0) != var_nextT) {
    return false;
  }

  if (fcmp->getOperand(1) != interval_y) {
    return false;
  }

  return true;
}

// while (t < loop_range_y)
//     float val0 = max(t, loop_range_x);
//     float val1 = min(nextT, loop_range_y);
// -->
// while (t < loop_range_x)
//     float val0 = loop_range_x;
//     float val1 = nextT;
void CustomLoopVersioning::rewriteLoopSeg1(Loop *loop, Value *interval_x, Value *interval_y) {
  BasicBlock *header = loop->getHeader();
  IGC_ASSERT(nullptr != header);
  BasicBlock *body = loop->getLoopLatch();
  IGC_ASSERT(nullptr != body);

  BranchInst *br = cast<BranchInst>(header->getTerminator());
  IGC_ASSERT(nullptr != br);
  FCmpInst *fcmp = dyn_cast<FCmpInst>(br->getCondition());
  IGC_ASSERT(nullptr != fcmp);
  IGC_ASSERT(fcmp->getOperand(1) == interval_y);

  fcmp->setOperand(1, interval_x);

  Instruction *i0 = body->getFirstNonPHIOrDbg();
  Instruction *i1 = i0->getNextNonDebugInstruction();

  IntrinsicInst *imax = cast<IntrinsicInst>(i0);
  IntrinsicInst *imin = cast<IntrinsicInst>(i1);
  IGC_ASSERT(imax);
  IGC_ASSERT(imin);

  imax->replaceAllUsesWith(interval_x);
  imin->replaceAllUsesWith(imin->getArgOperand(1));
}

void CustomLoopVersioning::hoistSeg2Invariant(Loop *loop, Instruction *fmul, Value *cbLoad) {
  BasicBlock *preHdr = loop->getLoopPreheader();
  BasicBlock *body = loop->getLoopLatch();

  // detecting loop invariant and move it to header:
  //   %211 = call float @llvm.fabs.f32(float %210)
  //   %212 = call float @llvm.log2.f32(float %211)
  //   %res_s465 = fmul float %165, %212
  //   %213 = call float @llvm.exp2.f32(float %res_s465)
  IntrinsicInst *intrin_abs = nullptr;
  IntrinsicInst *intrin_log2 = nullptr;
  Instruction *fmul_log2 = nullptr;
  Value *fmul_log2_opnd = nullptr;

  for (auto *UI : fmul->users()) {
    IntrinsicInst *intrin = cast<IntrinsicInst>(UI);
    if (intrin->getIntrinsicID() == Intrinsic::fabs && intrin->hasOneUse()) {
      intrin_abs = intrin;
      break;
    }
  }

  if (intrin_abs && intrin_abs->getParent() == body) {
    IntrinsicInst *intrin = dyn_cast<IntrinsicInst>(*intrin_abs->users().begin());
    if (intrin && intrin->getIntrinsicID() == Intrinsic::log2 && intrin->hasOneUse()) {
      intrin_log2 = intrin;
    }
  }

  if (intrin_log2 && intrin_log2->getParent() == body) {
    Instruction *fmul = dyn_cast<Instruction>(*intrin_log2->users().begin());
    if (fmul && fmul->getOpcode() == Instruction::FMul && fmul->hasOneUse()) {
      unsigned id = fmul->getOperand(0) == intrin_log2 ? 1 : 0;
      // make sure another operand is coming from out of loop
      Instruction *i = dyn_cast<Instruction>(fmul->getOperand(id));
      if (i && !loop->contains(i->getParent())) {
        fmul_log2 = fmul;
        fmul_log2_opnd = fmul->getOperand(id);
      }
    }
  }

  if (fmul_log2 && fmul_log2->getParent() == body) {
    IntrinsicInst *intrin = dyn_cast<IntrinsicInst>(*fmul_log2->users().begin());
    if (intrin && intrin->getIntrinsicID() == Intrinsic::exp2) {
      IRBuilder<> irb(preHdr->getFirstNonPHIOrDbg());
      irb.setFastMathFlags(fmul_log2->getFastMathFlags());

      Function *flog =
          Intrinsic::getDeclaration(m_function->getParent(), llvm::Intrinsic::log2, intrin_log2->getType());
      Function *fexp =
          Intrinsic::getDeclaration(m_function->getParent(), llvm::Intrinsic::exp2, intrin_log2->getType());
      Value *v = irb.CreateCall(flog, cbLoad);
      v = irb.CreateFMul(fmul_log2_opnd, v);
      v = irb.CreateCall(fexp, v);
      intrin->replaceAllUsesWith(v);
    }
  }
  fmul->replaceAllUsesWith(cbLoad);
}

// while (t < loop_range_y)
//     float val0 = max(t, loop_range_x);
//     float val1 = min(nextT, loop_range_y);
// -->
// while (t < loop_range_y/CB_Load)
//     float val0 = t;
//     float val1 = next;
void CustomLoopVersioning::rewriteLoopSeg2(Loop *loop, Value *interval_y, Value *cbLoad) {
  BasicBlock *header = loop->getHeader();
  IGC_ASSERT(nullptr != header);
  BasicBlock *body = loop->getLoopLatch();
  IGC_ASSERT(nullptr != body);

  BranchInst *br = cast<BranchInst>(header->getTerminator());
  IGC_ASSERT(nullptr != br);
  FCmpInst *fcmp = dyn_cast<FCmpInst>(br->getCondition());
  IGC_ASSERT(nullptr != fcmp);
  IGC_ASSERT(fcmp->getOperand(1) == interval_y);

  Instruction *v = BinaryOperator::Create(Instruction::FDiv, interval_y, cbLoad, "", fcmp);
  v->setFast(true);
  fcmp->setOperand(1, v);

  Instruction *i0 = body->getFirstNonPHIOrDbg();
  Instruction *i1 = i0->getNextNonDebugInstruction();

  IntrinsicInst *imax = cast<IntrinsicInst>(i0);
  IntrinsicInst *imin = cast<IntrinsicInst>(i1);
  IGC_ASSERT(imax && imin);

  // find
  //   %206 = call float @llvm.maxnum.f32()
  //   %207 = call float @llvm.minnum.f32()
  //   %209 = fdiv float 1.000000e+00, % 206
  //   %210 = fmul float %207, % 209
  Instruction *fmul = nullptr;
  for (auto *max_Users : imax->users()) {
    if (Instruction *fdiv = dyn_cast<BinaryOperator>(max_Users)) {
      if (ConstantFP *cf = dyn_cast<ConstantFP>(fdiv->getOperand(0))) {
        if (cf->isExactlyValue(1.0)) {
          for (auto *UI : fdiv->users()) {
            if ((fmul = dyn_cast<BinaryOperator>(UI))) {
              if (fmul->getOperand(0) == imin || (fmul->getOperand(1) == imin && fmul->getParent() == body)) {
                // find val1/val0
                hoistSeg2Invariant(loop, fmul, cbLoad);

                break;
              }
            }
          }
        }
      }
    }
  }

  imax->replaceAllUsesWith(imax->getArgOperand(1));
  imin->replaceAllUsesWith(imin->getArgOperand(1));
}

//     float val0 = max(t, loop_range_x);
//     float val1 = min(nextT, loop_range_y);
// -->
//     float val0 = t;
//     float val1 = loop_range_y;
void CustomLoopVersioning::rewriteLoopSeg3(BasicBlock *bb, Value *interval_y) {
  Instruction *i0 = bb->getFirstNonPHIOrDbg();
  Instruction *i1 = i0->getNextNonDebugInstruction();

  IntrinsicInst *imax = cast<IntrinsicInst>(i0);
  IntrinsicInst *imin = cast<IntrinsicInst>(i1);
  IGC_ASSERT(imax && imin);

  imax->replaceAllUsesWith(imax->getArgOperand(1));
  imin->replaceAllUsesWith(interval_y);

  auto II = bb->begin();
  auto IE = BasicBlock::iterator(bb->getFirstNonPHI());

  while (II != IE) {
    PHINode *PN = cast<PHINode>(II);

    IGC_ASSERT(PN->getNumIncomingValues() == 2);
    for (unsigned i = 0; i < PN->getNumIncomingValues(); i++) {
      if (PN->getIncomingBlock(i) != bb) {
        PN->replaceAllUsesWith(PN->getIncomingValue(i));
      }
    }
    ++II;
    PN->eraseFromParent();
  }
}

void CustomLoopVersioning::linkLoops(Loop *loopSeg1, Loop *loopSeg2, BasicBlock *afterLoop) {
  // we are handling do/while loop
  IGC_ASSERT(loopSeg1->getHeader() == loopSeg1->getLoopLatch());
  IGC_ASSERT(loopSeg2->getHeader() == loopSeg2->getLoopLatch());

  BasicBlock *seg1Body = loopSeg1->getLoopLatch();
  BasicBlock *seg2PreHdr = loopSeg2->getLoopPreheader();
  BasicBlock *seg2Body = loopSeg2->getLoopLatch();

  BranchInst *br = cast<BranchInst>(seg1Body->getTerminator());
  unsigned idx = br->getSuccessor(0) == afterLoop ? 0 : 1;
  br->setSuccessor(idx, loopSeg2->getLoopPreheader());

  auto II_1 = seg1Body->begin(), II_2 = seg2Body->begin();
  auto IE_2 = BasicBlock::iterator(seg2Body->getFirstNonPHI());

  for (; II_2 != IE_2; ++II_2, ++II_1) {
    PHINode *PN2 = cast<PHINode>(II_2);
    PHINode *PN1 = cast<PHINode>(II_1);
    Value *liveOut = nullptr;

    for (unsigned i = 0; i < PN1->getNumIncomingValues(); i++) {
      if (PN1->getIncomingBlock(i) == seg1Body) {
        liveOut = PN1->getIncomingValue(i);
        break;
      }
    }

    IGC_ASSERT(liveOut != nullptr);
    for (unsigned i = 0; i < PN2->getNumIncomingValues(); i++) {

      if (PN2->getIncomingBlock(i) != seg2Body) {
        PN2->setIncomingValue(i, liveOut);
        PN2->setIncomingBlock(i, seg2PreHdr);
      }
    }
  }
}

bool CustomLoopVersioning::processLoop(Loop *loop) {
  Value *var_range_x;
  Value *var_range_y;
  LoadInst *var_CBLoad_preHdr;
  Value *var_t_preHdr;
  Value *var_nextT_preHdr;
  bool found = false;

  found = detectLoop(loop, var_range_x, var_range_y, var_CBLoad_preHdr, var_t_preHdr, var_nextT_preHdr);

  if (!found)
    return false;

  const SmallVectorImpl<Instruction *> &liveOut = llvm::findDefsUsedOutsideOfLoop(loop);

  BasicBlock *preHdr = loop->getLoopPreheader();

  // apply the transformation
  BasicBlock *PH = llvm::SplitBlock(preHdr, preHdr->getTerminator(), m_DT, m_LI);

  // create loop seg 1 and insert before orig loop
  SmallVector<BasicBlock *, 8> seg1Blocks;
  Loop *loopSeg1 = llvm::cloneLoopWithPreheader(PH, preHdr, loop, m_vmapToSeg1, ".seg1", m_LI, m_DT, seg1Blocks);
  llvm::remapInstructionsInBlocks(seg1Blocks, m_vmapToSeg1);

  // create the check for fast loop
  // if (CB_Load > 1.0 &&
  //     loop_range_x * CB_Load < loop_range_y)
  //     fast version;
  // else
  //     orig version;
  preHdr->getTerminator()->eraseFromParent();

  IRBuilder<> irb(preHdr);
  FastMathFlags FMF;
  FMF.setFast();
  irb.setFastMathFlags(FMF);
  Value *cond0 = irb.CreateFCmpOGT(var_CBLoad_preHdr, ConstantFP::get(var_CBLoad_preHdr->getType(), 1.0));

  Value *cond1 = irb.CreateFCmpOLT(irb.CreateFMul(var_range_x, var_CBLoad_preHdr), var_range_y);

  irb.CreateCondBr(irb.CreateAnd(cond0, cond1), loopSeg1->getLoopPreheader(), loop->getLoopPreheader());

  BasicBlock *const afterLoop = loop->getExitBlock();
  IGC_ASSERT_MESSAGE(nullptr != afterLoop, "No single successor to loop exit block");

  // create loop seg 2 and insert before orig loop (after loop seg 1)
  SmallVector<BasicBlock *, 8> seg2Blocks;
  Loop *loopSeg2 =
      llvm::cloneLoopWithPreheader(PH, loopSeg1->getHeader(), loop, m_vmapToSeg2, ".seg2", m_LI, m_DT, seg2Blocks);
  llvm::remapInstructionsInBlocks(seg2Blocks, m_vmapToSeg2);

  // rewrite loop seg 1
  rewriteLoopSeg1(loopSeg1, var_range_x, var_range_y);

  // link loop seg1 to loop seg2
  linkLoops(loopSeg1, loopSeg2, afterLoop);

  // create seg3 after seg2 before changing loop2 body
  SmallVector<BasicBlock *, 8> seg3Blocks;
  Loop *loopSeg3 =
      llvm::cloneLoopWithPreheader(PH, loopSeg2->getHeader(), loop, m_vmapToSeg3, ".seg3", m_LI, m_DT, seg3Blocks);
  llvm::remapInstructionsInBlocks(seg3Blocks, m_vmapToSeg3);
  BasicBlock *bbSeg3 = loopSeg3->getLoopLatch();

  // rewrite loop seg2
  rewriteLoopSeg2(loopSeg2, var_range_y, var_CBLoad_preHdr);

  // link seg2 -> seg3 -> after_loop
  linkLoops(loopSeg2, loopSeg3, afterLoop);

  bbSeg3->getTerminator()->eraseFromParent();
  BranchInst::Create(afterLoop, bbSeg3);

  rewriteLoopSeg3(bbSeg3, var_range_y);

  addPhiNodes(liveOut, loopSeg1, loopSeg2, bbSeg3, loop);

  return true;
}

void CustomLoopVersioning::addPhiNodes(const SmallVectorImpl<Instruction *> &liveOuts, Loop *loopSeg1, Loop *loopSeg2,
                                       BasicBlock *bbSeg3, Loop *origLoop) {
  BasicBlock *const phiBB = origLoop->getExitBlock();
  IGC_ASSERT_MESSAGE(nullptr != phiBB, "No single successor to loop exit block");

  for (auto *Inst : liveOuts) {
    Value *seg3Val = m_vmapToSeg3[Inst];
    PHINode *phi;

    phi = PHINode::Create(Inst->getType(), 2, "", &phiBB->front());
    SmallVector<Instruction *, 8> instToDel;
    for (auto *User : Inst->users()) {
      PHINode *pu = dyn_cast<PHINode>(User);
      if (pu && pu->getParent() == phiBB) {
        // replace LCSSA phi with newly created phi node
        pu->replaceAllUsesWith(phi);
        instToDel.push_back(pu);
      }
    }
    for (auto *I : instToDel) {
      I->eraseFromParent();
    }
    phi->addIncoming(seg3Val, bbSeg3);
    phi->addIncoming(Inst, origLoop->getExitingBlock());
  }
}

// This pass is mostly forked from LoopSimplification pass
class LoopCanonicalization : public llvm::FunctionPass {
public:
  static char ID;

  LoopCanonicalization();

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
    AU.addRequiredID(llvm::LCSSAID);
    AU.addPreservedID(LCSSAID);
  }

  bool runOnFunction(Function &F);
  bool processLoop(Loop *loop, DominatorTree *DT, LoopInfo *LI, bool PreserveLCSSA);
  bool processOneLoop(Loop *loop, DominatorTree *DT, LoopInfo *LI, bool PreserveLCSSA);

  llvm::StringRef getPassName() const { return "IGC loop canonicalization"; }

private:
  CodeGenContext *m_cgCtx = nullptr;
  llvm::LoopInfo *m_LI = nullptr;
  llvm::DominatorTree *m_DT = nullptr;
  llvm::Function *m_function = nullptr;
};
#undef PASS_FLAG
#undef PASS_DESC
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS
#define PASS_FLAG "igc-loop-canonicalization"
#define PASS_DESC "IGC Loop canonicalization"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LoopCanonicalization, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass);
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass);
IGC_INITIALIZE_PASS_DEPENDENCY(LCSSAWrapperPass)
IGC_INITIALIZE_PASS_END(LoopCanonicalization, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

char LoopCanonicalization::ID = 0;

LoopCanonicalization::LoopCanonicalization() : FunctionPass(ID) {
  initializeLoopCanonicalizationPass(*PassRegistry::getPassRegistry());
}
/// \brief This method is called when the specified loop has more than one
/// backedge in it.
///
/// If this occurs, revector all of these backedges to target a new basic block
/// and have that block branch to the loop header.  This ensures that loops
/// have exactly one backedge.
static BasicBlock *insertUniqueBackedgeBlock(Loop *L, BasicBlock *Preheader, DominatorTree *DT, LoopInfo *LI) {
  IGC_ASSERT(nullptr != L);
  IGC_ASSERT_MESSAGE(L->getNumBackEdges() > 1, "Must have > 1 backedge!");

  // Get information about the loop
  BasicBlock *Header = L->getHeader();
  Function *F = Header->getParent();

  // Unique backedge insertion currently depends on having a preheader.
  if (!Preheader)
    return nullptr;

  // The header is not an EH pad; preheader insertion should ensure this.
  IGC_ASSERT_MESSAGE(!Header->isEHPad(), "Can't insert backedge to EH pad");

  // Figure out which basic blocks contain back-edges to the loop header.
  std::vector<BasicBlock *> BackedgeBlocks;
  for (pred_iterator I = pred_begin(Header), E = pred_end(Header); I != E; ++I) {
    BasicBlock *P = *I;

    // Indirectbr edges cannot be split, so we must fail if we find one.
    if (isa<IndirectBrInst>(P->getTerminator()))
      return nullptr;

    if (P != Preheader)
      BackedgeBlocks.push_back(P);
  }

  // Create and insert the new backedge block...
  BasicBlock *BEBlock = BasicBlock::Create(Header->getContext(), Header->getName() + ".backedge", F);
  BranchInst *BETerminator = BranchInst::Create(Header, BEBlock);
  BETerminator->setDebugLoc(Header->getFirstNonPHI()->getDebugLoc());

  // Move the new backedge block to right after the last backedge block.
  Function::iterator InsertPos = ++BackedgeBlocks.back()->getIterator();
  IGCLLVM::splice(F, InsertPos, F, BEBlock);

  // Now that the block has been inserted into the function, create PHI nodes in
  // the backedge block which correspond to any PHI nodes in the header block.
  for (BasicBlock::iterator I = Header->begin(); isa<PHINode>(I); ++I) {
    PHINode *PN = cast<PHINode>(I);
    PHINode *NewPN = PHINode::Create(PN->getType(), BackedgeBlocks.size(), PN->getName() + ".be", BETerminator);

    // Loop over the PHI node, moving all entries except the one for the
    // preheader over to the new PHI node.
    unsigned PreheaderIdx = ~0U;
    bool HasUniqueIncomingValue = true;
    Value *UniqueValue = nullptr;
    for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
      BasicBlock *IBB = PN->getIncomingBlock(i);
      Value *IV = PN->getIncomingValue(i);
      if (IBB == Preheader) {
        PreheaderIdx = i;
      } else {
        NewPN->addIncoming(IV, IBB);
        if (HasUniqueIncomingValue) {
          if (!UniqueValue)
            UniqueValue = IV;
          else if (UniqueValue != IV)
            HasUniqueIncomingValue = false;
        }
      }
    }

    // Delete all of the incoming values from the old PN except the preheader's
    IGC_ASSERT_MESSAGE(PreheaderIdx != ~0U, "PHI has no preheader entry??");
    if (PreheaderIdx != 0) {
      PN->setIncomingValue(0, PN->getIncomingValue(PreheaderIdx));
      PN->setIncomingBlock(0, PN->getIncomingBlock(PreheaderIdx));
    }
    // Nuke all entries except the zero'th.
    for (unsigned i = 0, e = PN->getNumIncomingValues() - 1; i != e; ++i)
      PN->removeIncomingValue(e - i, false);

    // Finally, add the newly constructed PHI node as the entry for the BEBlock.
    PN->addIncoming(NewPN, BEBlock);

    // As an optimization, if all incoming values in the new PhiNode (which is a
    // subset of the incoming values of the old PHI node) have the same value,
    // eliminate the PHI Node.
    if (HasUniqueIncomingValue) {
      NewPN->replaceAllUsesWith(UniqueValue);
      NewPN->eraseFromParent();
    }
  }

  // Now that all of the PHI nodes have been inserted and adjusted, modify the
  // backedge blocks to jump to the BEBlock instead of the header.
  // If one of the backedges has llvm.loop metadata attached, we remove
  // it from the backedge and add it to BEBlock.
  unsigned LoopMDKind = BEBlock->getContext().getMDKindID("llvm.loop");
  MDNode *LoopMD = nullptr;
  for (unsigned i = 0, e = BackedgeBlocks.size(); i != e; ++i) {
    IGCLLVM::TerminatorInst *TI = BackedgeBlocks[i]->getTerminator();
    if (!LoopMD)
      LoopMD = TI->getMetadata(LoopMDKind);
    TI->setMetadata(LoopMDKind, nullptr);
    for (unsigned Op = 0, e = TI->getNumSuccessors(); Op != e; ++Op)
      if (TI->getSuccessor(Op) == Header)
        TI->setSuccessor(Op, BEBlock);
  }
  BEBlock->getTerminator()->setMetadata(LoopMDKind, LoopMD);

  //===--- Update all analyses which we must preserve now -----------------===//

  // Update Loop Information - we know that this block is now in the current
  // loop and all parent loops.
  L->addBasicBlockToLoop(BEBlock, *LI);

  // Update dominator information
  DT->splitBlock(BEBlock);

  return BEBlock;
}

bool LoopCanonicalization::runOnFunction(llvm::Function &F) {
  bool Changed = false;
  LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  bool PreserveLCSSA = mustPreserveAnalysisID(LCSSAID);

  // Simplify each loop nest in the function.
  for (LoopInfo::iterator I = LI->begin(), E = LI->end(); I != E; ++I)
    Changed |= processLoop(*I, DT, LI, PreserveLCSSA);
  return Changed;
}

bool LoopCanonicalization::processLoop(llvm::Loop *L, DominatorTree *DT, LoopInfo *LI, bool PreserveLCSSA) {
  bool changed = false;
  // Worklist maintains our depth-first queue of loops in this nest to process.
  SmallVector<Loop *, 4> Worklist;
  Worklist.push_back(L);

  // Walk the worklist from front to back, pushing newly found sub loops onto
  // the back. This will let us process loops from back to front in depth-first
  // order. We can use this simple process because loops form a tree.
  for (unsigned Idx = 0; Idx != Worklist.size(); ++Idx) {
    Loop *L2 = Worklist[Idx];
    Worklist.append(L2->begin(), L2->end());
  }

  while (!Worklist.empty())
    changed |= processOneLoop(Worklist.pop_back_val(), DT, LI, PreserveLCSSA);
  return changed;
}

// Do basic loop canonicalization to ensure correctness. We need a single header and single latch
bool LoopCanonicalization::processOneLoop(Loop *L, DominatorTree *DT, LoopInfo *LI, bool PreserveLCSSA) {
  bool changed = false;
  // Does the loop already have a preheader?  If so, don't insert one.
  BasicBlock *Preheader = L->getLoopPreheader();
  if (!Preheader) {
    Preheader = InsertPreheaderForLoop(L, DT, LI, nullptr, PreserveLCSSA);
    if (Preheader) {
      changed = true;
    }
  }

  // If the header has more than two predecessors at this point (from the
  // preheader and from multiple backedges), we must adjust the loop.
  BasicBlock *LoopLatch = L->getLoopLatch();
  if (!LoopLatch) {
    // If we either couldn't, or didn't want to, identify nesting of the loops,
    // insert a new block that all backedges target, then make it jump to the
    // loop header.
    LoopLatch = insertUniqueBackedgeBlock(L, Preheader, DT, LI);
    if (LoopLatch) {
      changed = true;
    }
  }

  return changed;
}

namespace IGC {
FunctionPass *createLoopCanonicalization() { return new LoopCanonicalization(); }
} // namespace IGC

// This pass pattern match loops where the loop body contains variables
// that are constant for all except the last iteration of the loop, in
// which case we can hoist these values out of the loop.
//
// Input Loop:
//  Input loop compares the loop induction variable to the loop size using a min
//  instruction. The ALU instructions dependent on the result of the 'min' can
//  be done at compile time for most iterations of the loop.
//
// loop.header:
// %preInc = phi float[%132, %preheader], [%Inc, %loop.end]
// %postInc = fmul fast float %preInc, %x
// %178 = call fast float @llvm.minnum.f32(float %postInc, float %LoopSize)
// %179 = fsub fast float %178, %preInc
// %180 = fdiv fast float %178, %preInc
// ...
// %cmp = fcmp fast ult float %postInc, %LoopSize
// br i1 %cmp, label %loop.header, label %loop.end
//
//
// Transformed Loop:
//  Loop is split into if/then/else branch, where the if block is only entered
//  if (%preInc < %LoopSize). This allows later passes to simpifly instructions
//  in the if BB by doing the computation at compile time.
//
// loop.header:
// %preInc = phi float[%132, %preheader], [%Inc, %loop.end]
// %postInc = fmul fast float %preInc, %x
// %cmpHoist = fcmp ult float %preInc, %LoopSize
// br i1 %cmpHoist, label %loop.if.hoist, label %loop.else.hoist
//
// loop.if.hoist:
// %180 = fsub fast float %preInc, %preInc
// %181 = fdiv fast float %preInc, %preInc
// br label %loop.end.hoist
//
// loop.else.hoist:
// %190 = call fast float @llvm.minnum.f32(float %postInc, float %LoopSize)
// %191 = fsub fast float %190, %preInc
// %192 = fdiv fast float %190, %preInc
// br label %loop.end.hoist
//
// loop.end.hoist:
// %200 = phi float [ %180, %loop.if.hoist ], [ %191, %loop.else.hoist ]
// %201 = phi float [ %181, %loop.if.hoist ], [ %192, %loop.else.hoist ]
// ...
// %cmp = fcmp fast ult float %postInc, %LoopSize
// br i1 %cmp, label %loop.header, label %loop.end
//
class LoopHoistConstant : public llvm::LoopPass {
public:
  static char ID;

  LoopHoistConstant();

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.addPreservedID(LCSSAID);
  }

  bool runOnLoop(Loop *L, LPPassManager &LPM);

  llvm::StringRef getPassName() const { return "IGC loop hoist constant"; }

private:
};
#undef PASS_FLAG
#undef PASS_DESC
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS
#define PASS_FLAG "igc-loop-hoist-constant"
#define PASS_DESC "IGC loop hoist constant"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LoopHoistConstant, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(LoopHoistConstant, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

char LoopHoistConstant::ID = 0;

LoopHoistConstant::LoopHoistConstant() : LoopPass(ID) {
  initializeLoopHoistConstantPass(*PassRegistry::getPassRegistry());
}

bool LoopHoistConstant::runOnLoop(Loop *L, LPPassManager &LPM) {
  LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

  if (!L->getLoopPreheader() || !L->getLoopLatch() || !L->isSafeToClone() || L->getNumBackEdges() != 1)
    return false;

  BasicBlock *Header = L->getHeader();
  BasicBlock *LoopLatch = L->getLoopLatch();

  PHINode *InductionPreInc = nullptr;         // Induction variable pre-increment
  BinaryOperator *InductionPostInc = nullptr; // // Induction variable post-increment
  FCmpInst *LoopCond = nullptr;               // The loop exit condition
  BranchInst *LoopBranch = nullptr;           // The pre-hoisted loop branching instruction
  Value *LoopSize = nullptr;
  IntrinsicInst *MinInst = nullptr;

  // Match the loop induction variable
  InductionPostInc = dyn_cast<BinaryOperator>(Header->getFirstNonPHIOrDbg());
  if (InductionPostInc && InductionPostInc->getOpcode() == BinaryOperator::FMul) {
    InductionPreInc = dyn_cast<PHINode>(InductionPostInc->getOperand(0));
    if (!InductionPreInc)
      InductionPreInc = dyn_cast<PHINode>(InductionPostInc->getOperand(1));
  }
  if (!InductionPreInc || !InductionPostInc)
    return false;
  if (InductionPreInc->getBasicBlockIndex(LoopLatch) < 0 ||
      InductionPreInc->getIncomingValueForBlock(LoopLatch) != InductionPostInc)
    return false;

  // Match the loop exit condition and branch
  LoopBranch = dyn_cast<BranchInst>(LoopLatch->getTerminator());
  if (LoopBranch && LoopBranch->isConditional()) {
    LoopCond = dyn_cast<FCmpInst>(LoopBranch->getCondition());
    if (LoopCond && (LoopCond->getPredicate() == CmpInst::FCMP_ULT || LoopCond->getPredicate() == CmpInst::FCMP_OLT)) {
      if (LoopCond->getOperand(0) == InductionPostInc) {
        LoopSize = LoopCond->getOperand(1);
      }
    }
  }
  if (!LoopBranch || !LoopCond || !LoopSize)
    return false;

  // Match the minnum comparison between the induction var and the loop size
  // Should appear right after the post-incremented induction variable
  MinInst = dyn_cast<IntrinsicInst>(InductionPostInc->getNextNonDebugInstruction());
  if (MinInst && MinInst->getIntrinsicID() == llvm::Intrinsic::minnum) {
    Value *min1 = MinInst->getOperand(0);
    Value *min2 = MinInst->getOperand(1);
    if ((min1 == InductionPostInc && min2 == LoopSize) || (min2 == InductionPostInc && min1 == LoopSize)) {
    } else {
      return false;
    }
    // All uses of the minnum should be within the loop body BB
    if (MinInst->isUsedOutsideOfBlock(Header))
      return false;
  } else {
    return false;
  }

  // We now have all the info to hoist out the constant variables.
  // First split the HeaderBB into if/then/else blocks.
  Instruction *ifTerm;
  Instruction *elseTerm;
  auto cmpIfHoist =
      FCmpInst::Create(LoopCond->getOpcode(), LoopCond->getPredicate(), InductionPostInc, LoopSize, "", MinInst);
  llvm::SplitBlockAndInsertIfThenElse(cmpIfHoist, MinInst, &ifTerm, &elseTerm);

  BasicBlock *ifHoistBB = ifTerm->getParent();
  BasicBlock *elseHoistBB = elseTerm->getParent();
  BasicBlock *endHoistBB = elseHoistBB->getNextNode();

  // Set the new block names
  ifHoistBB->setName(Header->getName() + ".if.hoist");
  elseHoistBB->setName(Header->getName() + ".else.hoist");
  endHoistBB->setName(Header->getName() + ".end.hoist");

  // Add new blocks to the current loop
  L->addBasicBlockToLoop(ifHoistBB, *LI);
  L->addBasicBlockToLoop(elseHoistBB, *LI);
  L->addBasicBlockToLoop(endHoistBB, *LI);

  // Clone the instructions starting from the minnum up to the terminator.
  // The cloned instructions go into the if block, and the original instructions
  // are moved into the else block.
  ValueToValueMapTy VMap;
  Instruction *II = cast<Instruction>(endHoistBB->begin());
  while (II != endHoistBB->getTerminator()) {
    Instruction *currI = II;
    II = II->getNextNode();

    Instruction *clonedI = currI->clone();
    clonedI->insertBefore(ifTerm);
    currI->moveBefore(elseTerm);
    VMap[currI] = clonedI;
  }
  // Update the operands for the cloned instructions
  for (auto II = ifHoistBB->begin(), IE = ifHoistBB->end(); II != IE; ++II) {
    for (unsigned op = 0, E = II->getNumOperands(); op != E; ++op) {
      Value *Op = II->getOperand(op);
      ValueToValueMapTy::iterator It = VMap.find(Op);
      if (It != VMap.end())
        II->setOperand(op, It->second);
    }
  }

  // Replace the minnum instruction with the known value in the if block
  Instruction *newMinInst = dyn_cast<Instruction>(VMap[MinInst]);
  IGC_ASSERT(newMinInst && newMinInst->getParent() == ifHoistBB);
  newMinInst->replaceAllUsesWith(InductionPostInc);

  // Replace the minnum instruction with the known value in the else block
  MinInst->replaceAllUsesWith(LoopSize);

  // Update successors and users of the original BB
  Header->replaceSuccessorsPhiUsesWith(endHoistBB);
  for (auto &II : *elseHoistBB) {
    // For users of the original instruction outside of the HeaderBB, we need a new PHINode
    // to pick between the if.hoist and else.hoist blocks
    if (II.isUsedOutsideOfBlock(elseHoistBB) && VMap.find(&II) != VMap.end()) {
      PHINode *PN = PHINode::Create(II.getType(), 2, "", endHoistBB->getTerminator());
      if (PN) {
        II.replaceUsesOutsideBlock(PN, elseHoistBB);
        PN->addIncoming(VMap[&II], ifHoistBB);
        PN->addIncoming(&II, elseHoistBB);
      }
    }
  }

  return true;
}

namespace IGC {
LoopPass *createLoopHoistConstant() { return new LoopHoistConstant(); }
} // namespace IGC

// This pass disables LICM optimization by adding llvm.licm.disable
// - when Loop depends on SIMD Lane Id and operates on local memory or
// - when the loops are part of a large function and the number of loops
//   is sizable where a potential stack overflow from memory SSA updater
//   could occur.

class SpecialCasesDisableLICM : public llvm::FunctionPass {
public:
  static char ID;

  SpecialCasesDisableLICM();

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.addPreservedID(LCSSAID);
    AU.addRequired<llvm::LoopInfoWrapperPass>();
  }

  bool runOnFunction(Function &F);
  bool LoopHasLoadFromLocalAddressSpace(const Loop &L);
  bool LoopDependsOnSIMDLaneId(const Loop &L);
  bool AddLICMDisableMedatadaToSpecificLoop(Loop &L);

  llvm::StringRef getPassName() const { return "IGC special cases disable LICM"; }
};

#undef PASS_FLAG
#undef PASS_DESC
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS
#define PASS_FLAG "igc-special-cases-disable-licm"
#define PASS_DESC "IGC special cases disable LICM"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SpecialCasesDisableLICM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(SpecialCasesDisableLICM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

char SpecialCasesDisableLICM::ID = 0;

SpecialCasesDisableLICM::SpecialCasesDisableLICM() : FunctionPass(ID) {
  initializeSpecialCasesDisableLICMPass(*PassRegistry::getPassRegistry());
}

bool SpecialCasesDisableLICM::runOnFunction(llvm::Function &F) {
  constexpr size_t HIGH_BB_THRESHOLD_FOR_LICM = 2500;
  constexpr size_t HIGH_LOOP_THRESHOLD_FOR_LICM = 450;

  bool Changed = false;
  LoopInfo *LI = nullptr;

  auto getLoopInfo = [&]() -> LoopInfo * {
    if (nullptr == LI)
      return &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    return LI;
  };

  if (F.size() > HIGH_BB_THRESHOLD_FOR_LICM) {
    LI = getLoopInfo();

    if (llvm::size(*LI) > HIGH_LOOP_THRESHOLD_FOR_LICM
    ) {
      for (auto *L : *LI)
        AddLICMDisableMedatadaToSpecificLoop(*L);

      Changed = true;
    }
  }

  if (!Changed) {
    LI = getLoopInfo();

    for (auto *L : *LI) {
      if (LoopHasLoadFromLocalAddressSpace(*L) && LoopDependsOnSIMDLaneId(*L))
        Changed |= AddLICMDisableMedatadaToSpecificLoop(*L);
    }
  }

  return Changed;
}

bool SpecialCasesDisableLICM::LoopHasLoadFromLocalAddressSpace(const Loop &L) {
  for (BasicBlock *BB : L.blocks()) {
    for (Instruction &I : *BB) {
      if (LoadInst *LI = dyn_cast<LoadInst>(&I))
        if (LI->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL)
          return true;
    }
  }
  return false;
}

bool SpecialCasesDisableLICM::LoopDependsOnSIMDLaneId(const Loop &L) {
  auto ComeFromSIMDLaneID = [](Value *I) {
    Value *stripZExt = I;
    if (auto *zext = dyn_cast<ZExtInst>(I))
      stripZExt = zext->getOperand(0);
    if (auto *intrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(stripZExt))
      return intrinsic->getIntrinsicID() == GenISAIntrinsic::GenISA_simdLaneId;

    return false;
  };

  if (BasicBlock *LoopHeader = L.getHeader()) {
    for (Instruction &I : *LoopHeader) {
      if (CmpInst *cmp = dyn_cast<CmpInst>(&I)) {
        if (ComeFromSIMDLaneID(cmp->getOperand(0)) || ComeFromSIMDLaneID(cmp->getOperand(1)))
          return true;
      }
    }
  }

  return false;
}

bool SpecialCasesDisableLICM::AddLICMDisableMedatadaToSpecificLoop(Loop &L) {
  LLVMContext &context = L.getHeader()->getContext();

  MDNode *selfRef{};
  MDNode *licm_disable = MDNode::get(context, MDString::get(context, "llvm.licm.disable"));
  selfRef = MDNode::get(context, ArrayRef<Metadata *>({selfRef, licm_disable}));
  selfRef->replaceOperandWith(0, selfRef);

  if (BasicBlock *LoopLatch = L.getLoopLatch()) {
    LoopLatch->getTerminator()->setMetadata(LLVMContext::MD_loop, selfRef);
    return true;
  }
  return false;
}

namespace IGC {
FunctionPass *createSpecialCasesDisableLICM() { return new SpecialCasesDisableLICM(); }
} // namespace IGC

// The LoopSplitWidePHIs pass finds opportunities to eliminate shuffles which
// split and re-join wide vectors. The motivating case are loops with
// accumulators of width k*N where the accumulator operations are of width N.
// In such a case we replace the accumulator PHI with k individual PHIs for
// each part, e.g.:
//
// loop.header:
// %accum = phi <float x 16> [%zeroinitializer, %preheader], [%accum.join, %loop.end]
// %accum.lo = shuffle %accum, %accum, <0..7>
// %accum.hi = shuffle %accum, %accum, <8..15>
// %accum.lo1 = dpas %accum.lo, ...
// %accum.hi1 = dpas %accum.hi ...
// ..
// loop.end:
// %accum.join = shuffle %accum.loN, %accum.hiN, <0..7, 0..7>
// ..
// loop.exit:
// store <float x 16> %accum.join, ...
//
// If there are no other uses of %accum and %accum.join in the loop, this can
// be transformed to:
//
// loop.header:
// %accum.lo = phi <float x 8> [%zeroinitializer, %preheader], [%accum.loN, %loop.end]
// %accum.hi = phi <float x 8> [%zeroinitializer, %preheader], [%accum.hiN, %loop.end]
// %accum.lo1 = dpas %accum.lo, ...
// %accum.hi1 = dpas %accum.hi ...
// ..
// loop.end:
// ..
// loop.exit:
// %accum.join = shuffle %accum.loN, %accum.hiN, <0..7, 0..7>
// store <float x 16> %accum.join, ...
//
// The major benefit of this transformation is to allow the finalizer
// to easily identify opportunities to use indexed operands to access individual
// parts of a wider accumulator register. Otherwise, the legalized shuffles
// cannot be simplified without loop analysis and may lead to unneccesary mov
// operations and more edges in the interference graph.
//
class LoopSplitWidePHIs : public llvm::FunctionPass {
public:
  static char ID;

  LoopSplitWidePHIs();

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const { AU.addRequired<llvm::LoopInfoWrapperPass>(); }

  bool runOnFunction(Function &F);
  bool processLoop(Loop *L, LoopInfo *LI);
  bool processOneLoop(Loop *L, LoopInfo *LI);
  bool processPHI(SmallVectorImpl<PHINode *> &WL, Loop *L);

  llvm::StringRef getPassName() const { return "IGC split wide loop PHIs"; }

private:
  // Structure used to describe a concatenation Result = [Head, Tail],
  // where all values are fixed vector types of the same element size.
  struct CatenatedValue {
    ShuffleVectorInst *Result = nullptr;
    BitCastInst *Bitcast = nullptr;
    // a.k.a. { Head, Tail }
    Value *Parts[2] = {nullptr, nullptr};
    // Types of each part.
    IGCLLVM::FixedVectorType *Types[2] = {nullptr, nullptr};
    // Element count of Parts[1]
    unsigned TailSize = 0;
    // Offset of Parts[1].
    unsigned Offset = 0;
  };

  bool getCatenatedValue(Instruction *I, CatenatedValue &CV);
  Instruction *createCatenatedValue(const CatenatedValue &CV, Value *Head, Value *Tail, Instruction *InsertBeforeI);

  PHINode *foldBitcasts(PHINode *PHI, CatenatedValue &CV, Loop *L);
};

#undef PASS_FLAG
#undef PASS_DESC
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS
#define PASS_FLAG "igc-loop-split-wide-phis"
#define PASS_DESC "IGC split wide loop PHIs"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LoopSplitWidePHIs, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(LoopSplitWidePHIs, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

char LoopSplitWidePHIs::ID = 0;

LoopSplitWidePHIs::LoopSplitWidePHIs() : FunctionPass(ID) {
  initializeLoopSplitWidePHIsPass(*PassRegistry::getPassRegistry());
}

bool LoopSplitWidePHIs::runOnFunction(llvm::Function &F) {
  bool Changed = false;
  LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  for (LoopInfo::iterator I = LI->begin(), E = LI->end(); I != E; ++I)
    Changed |= processLoop(*I, LI);
  return Changed;
}

bool LoopSplitWidePHIs::processLoop(Loop *L, LoopInfo *LI) {
  bool Changed = false;
  // Worklist maintains our depth-first queue of loops in this nest to process.
  SmallVector<Loop *, 4> Worklist;
  Worklist.push_back(L);

  // Walk the worklist from front to back, pushing newly found sub loops onto
  // the back. This will let us process loops from back to front in depth-first
  // order. We can use this simple process because loops form a tree.
  for (unsigned Idx = 0; Idx != Worklist.size(); ++Idx) {
    Loop *L2 = Worklist[Idx];
    Worklist.append(L2->begin(), L2->end());
  }

  while (!Worklist.empty())
    Changed |= processOneLoop(Worklist.pop_back_val(), LI);
  return Changed;
}

bool LoopSplitWidePHIs::processOneLoop(Loop *L, LoopInfo *LI) {
  bool Changed = false;
  BasicBlock *Preheader = L->getLoopPreheader();
  BasicBlock *Latch = L->getLoopLatch();
  // Skip if the loop is not canonical.
  if (!Preheader || !Latch)
    return false;
  IGC_ASSERT(Preheader->getTerminator());

  // Initialize a worklist of all 2-source PHI nodes.
  // We will repeatedly call processPHI to maybe transform
  // the top PHI; this pushes any newly-created PHIs onto the
  // worklist.
  SmallVector<PHINode *, 8> Worklist;
  for (auto &PHI : L->getHeader()->phis()) {
    if (PHI.getNumIncomingValues() == 2 && PHI.getBasicBlockIndex(Preheader) >= 0 &&
        PHI.getBasicBlockIndex(Latch) >= 0 && dyn_cast<IGCLLVM::FixedVectorType>(PHI.getType())) {
      Worklist.push_back(&PHI);
    }
  }

  // Process the worklist. This may add two new items which
  // are half the element count of the original PHI.
  while (!Worklist.empty())
    Changed |= processPHI(Worklist, L);
  return Changed;
}

// Helper for getCatenatedValue. If I effectively catenates two vectors
// [Head, Tail], where the element count of Tail is at most that of Head,
// return true and set Head, Tail to the source values of each part, and set
// Offset and Width to the element counts of Head, Tail respectively.
// We assume that I is of fixed vector type.
static bool findCatenateSources(ShuffleVectorInst *I, Value *&Head, Value *&Tail, unsigned &Offset, unsigned &Width) {
  // First handle the simple case of a concat shuffle, where
  // the head and tail are of equal size.
  if (I->isConcat()) {
    Head = I->getOperand(0);
    Tail = I->getOperand(1);
    Offset = (unsigned)cast<IGCLLVM::FixedVectorType>(Head->getType())->getNumElements();
    Width = Offset;
    return true;
  }

  // Otherwise, we may have an insert subvector shuffle
  // appending a shorter tail to a longer head.
  int NumSubElts = 0, Index = 0;
  if (IGCLLVM::isInsertSubvectorMask(I, NumSubElts, Index)) {
    Head = I->getOperand(0);
    Tail = I->getOperand(1);
    // We expect the tail value to be padded to the same element count
    // as the head, and require the padded value to only be used
    // in the catenating shuffle.
    if ((int)cast<IGCLLVM::FixedVectorType>(Tail->getType())->getNumElements() > NumSubElts) {
      auto SVI = dyn_cast<ShuffleVectorInst>(Tail);
      if (SVI && SVI->isIdentityWithPadding() && SVI->hasOneUse())
        Tail = SVI->getOperand(0);
      else
        return false;
    }
    Offset = Index;
    Width = NumSubElts;
    return true;
  }
  return false;
}

// Helper to identify shuffles which defined a catenated value that
// is a candidate for splitting. Returns true and populates CV
// with the description if successful.
bool LoopSplitWidePHIs::getCatenatedValue(Instruction *I, CatenatedValue &CV) {
  BitCastInst *BitcastI = nullptr;

  // Look through at most one bitcast for a candidate
  // shuffle. If the bitcast defines the latch value of a PHI
  // and all PHI uses have an inverse bitcast, these
  // can be folded away (see foldBitcasts()).
  if (auto BCI = dyn_cast<BitCastInst>(I)) {
    BitcastI = BCI;
    I = dyn_cast<Instruction>(BCI->getOperand(0));
    if (!I)
      return false;
  }
  if (auto SVI = dyn_cast<ShuffleVectorInst>(I)) {
    if (!isa<IGCLLVM::FixedVectorType>(SVI->getType()))
      return false;
    Value *Head, *Tail;
    unsigned Offset, Width;
    if (findCatenateSources(SVI, Head, Tail, Offset, Width)) {
      IGC_ASSERT(Head && Tail);
      CV.Result = SVI;
      CV.Bitcast = BitcastI;
      CV.Parts[0] = Head;
      CV.Parts[1] = Tail;
      CV.Types[0] = cast<IGCLLVM::FixedVectorType>(Head->getType());
      CV.Types[1] = cast<IGCLLVM::FixedVectorType>(Tail->getType());
      CV.TailSize = Width;
      CV.Offset = Offset;
      return true;
    }
  }
  return false;
}

// Helpers to generate shuffle masks.
// Generates a mask which is the catenation of one or two contiguously
// increasing sequences and padded with undef elements up to the total.
// padded with UndefMaskElem up to Total.
static Value *getShuffleMask(LLVMContext &Ctx, uint64_t Total, uint64_t FromA, uint64_t ToA, uint64_t FromB = 0,
                             uint64_t ToB = 0) {
  IGC_ASSERT(FromA <= ToA && FromB <= ToB);
  std::vector<Constant *> Components;
  IntegerType *MaskElemTy = IntegerType::get(Ctx, 32);
  for (uint64_t i = FromA; i < ToA; ++i)
    Components.push_back(ConstantInt::get(MaskElemTy, i));
  for (uint64_t i = FromB; i < ToB; ++i)
    Components.push_back(ConstantInt::get(MaskElemTy, i));
  for (uint64_t i = Components.size(); i < Total; ++i)
    Components.push_back(UndefValue::get(MaskElemTy));
  return ConstantVector::get(Components);
}

// Check that the given value has no users in the loop, except
// the given instruction (if non-null), and that any external PHI
// uses are single-source.
static bool hasOnlySimpleExternalUses(Value *Val, Loop *L, Value *IgnoreUse = nullptr) {
  for (auto *U : Val->users()) {
    if (IgnoreUse && IgnoreUse == U)
      continue;
    auto *UseI = dyn_cast<Instruction>(U);
    if (!UseI || L->contains(UseI->getParent()))
      return false;
    else if (auto PHI = dyn_cast<PHINode>(UseI)) {
      if (PHI->getNumIncomingValues() > 1)
        return false;
    }
  }
  return true;
}

// If I is an extract-like shuffle of ElemWidth values from a wider
// source, return the offset of the extracted portion.
// Returns -1 otherwise.
static int getExtractIndex(Instruction *I, unsigned ElemWidth) {
  if (auto *UseShufI = dyn_cast<ShuffleVectorInst>(I)) {
    auto ShufTy = dyn_cast<IGCLLVM::FixedVectorType>(UseShufI->getType());
    int Index;
    if (UseShufI->isExtractSubvectorMask(Index) && ShufTy->getNumElements() == ElemWidth)
      return Index;
  }
  return -1;
}

// Maybe split the PHI at the back of the worklist. Returns true
// if the PHI was split, and appends newly created candidate PHIs
// to the list.
bool LoopSplitWidePHIs::processPHI(SmallVectorImpl<PHINode *> &WL, Loop *L) {
  // Caller ensures that loop is canonical and has a well-formed
  // preheader.
  BasicBlock *Latch = L->getLoopLatch();
  BasicBlock *Preheader = L->getLoopPreheader();
  IGC_ASSERT(!WL.empty());

  // Assume worklist elements are 2-source PHIs of fixed vector type
  // fed by the preheader and latch, which exist.
  // We check the required conditions, populating lists of uses
  // to be replaced with the split components or their joined result.
  PHINode *PHI = WL.pop_back_val();
  Value *DefI = PHI->getIncomingValueForBlock(Latch);

  // Identify whether the latch value's definition is a candidate
  // for splitting.
  CatenatedValue CV;
  if (!isa<Instruction>(DefI) || !getCatenatedValue(cast<Instruction>(DefI), CV))
    return false;

  // If there is a bitcast of CV.Result, attempt to fold it away.
  // This will replace PHI with a new PHI with the same type as
  // CV.Result if successful, in which case we can proceed as usual.
  if (CV.Bitcast) {
    PHI = foldBitcasts(PHI, CV, L);
    if (!PHI)
      return false;
  }

  // Check the uses of the PHI value to ensure that
  // all uses in the loop are extract-vector-like shuffles.
  //
  // We don't permit uses of the full value in L
  // as this could result in a net increase in
  // shuffles. It is possible to handle these cases
  // if we do the accounting for added/removed
  // shuffles.
  using SplitUse = std::pair<Instruction *, int>;
  SmallVector<SplitUse, 8> PHIUses;
  for (auto U : PHI->users()) {
    if (auto *UseI = dyn_cast<Instruction>(U)) {
      int Idx = getExtractIndex(UseI, CV.TailSize);
      if (Idx >= 0) {
        PHIUses.emplace_back(UseI, Idx);
        continue;
      }
    }
    return false;
  }

  // Check all uses of the latch value.
  // * Any other uses in the loop (excluding the PHI) disqualify the candidate.
  // * Uses out of the loop can be replaced with a concat
  //   of ShufI's sources. This means a use in an external PHI
  //   must be a single-source PHI which we can replace with
  //   PHIs for each part, with uses of the original PHI replaced
  //   by the catenation of the new PHIs.
  if (!hasOnlySimpleExternalUses(CV.Result, L, PHI))
    return false;

  // Bail early if there is nothing to be done.
  if (PHIUses.empty())
    return false;

  // Shuffle masks for extracting the catenated parts.
  Value *MaskParts[2] = {
      getShuffleMask(PHI->getContext(), CV.Types[0]->getNumElements(), 0, CV.Offset),
      getShuffleMask(PHI->getContext(), CV.Types[1]->getNumElements(), CV.Offset, CV.Offset + CV.TailSize)};

  // Create the new PHIs for each part. Here we also need to split
  // the incoming preheader value.
  PHINode *NewPHI[2];
  for (unsigned i = 0; i < 2; ++i)
    NewPHI[i] = PHINode::Create(CV.Types[i], 2, "split", PHI);
  int PreIdx = PHI->getBasicBlockIndex(Preheader);
  auto PreValue = PHI->getIncomingValue(PreIdx);
  for (unsigned i = 0; i < 2; ++i) {
    auto *PreShufI = new ShuffleVectorInst(PreValue, PreValue, MaskParts[i]);
    PreShufI->insertBefore(Preheader->getTerminator());
    NewPHI[i]->addIncoming(PreShufI, Preheader);
    NewPHI[i]->addIncoming(CV.Parts[i], Latch);
  }

  // Rewrite the PHI uses to use the new split PHI results.
  // A use that extracts the tail part has it's uses replaced
  // with NewPHI[1].
  // A use that extracts any other part is rewritten to
  // extract from NewPHI[0], or folded away if it becomes
  // an identity shuffle.
  for (SplitUse &SU : PHIUses) {
    // Set ReplaceWith with the appropriate source based on the
    // extract index.
    Value *ReplaceWith;
    if (SU.second == CV.Offset)
      ReplaceWith = NewPHI[1];
    else {
      auto SVI = cast<ShuffleVectorInst>(SU.first);
      auto NewSVI = new ShuffleVectorInst(NewPHI[0], IGCLLVM::PoisonValue::get(NewPHI[0]->getType()),
                                          IGCLLVM::getShuffleMaskForBitcode(SVI));
      NewSVI->insertBefore(SU.first);
      ReplaceWith = NewSVI;

      if (NewSVI->isIdentity()) {
        ReplaceWith = NewPHI[0];
        NewSVI->eraseFromParent();
      }
    }

    SmallVector<User *, 8u> Users(SU.first->users());
    for (auto U : Users)
      U->replaceUsesOfWith(SU.first, ReplaceWith);
    IGC_ASSERT(!SU.first->hasNUsesOrMore(1));
    SU.first->eraseFromParent();
  }

  // Shuffle mask which catenates the parts back together.
  auto FullOffset = CV.Types[0]->getNumElements();
  auto FullWidth = cast<IGCLLVM::FixedVectorType>(PHI->getType())->getNumElements();
  Value *MaskJoin = getShuffleMask(PHI->getContext(), FullWidth, 0, CV.Offset, FullOffset, FullOffset + CV.TailSize);

  // Shuffle mask to extend tail to the same element count as the head.
  // This is left null if an extend is not needed.
  Value *MaskPad = nullptr;
  if (CV.Types[1]->getNumElements() < CV.Types[0]->getNumElements())
    MaskPad = getShuffleMask(PHI->getContext(), CV.Types[0]->getNumElements(), 0, CV.Types[1]->getNumElements());

  // Delete the PHI now so it will not appear as a use of the latch value.
  IGC_ASSERT(!PHI->hasNUsesOrMore(1));
  PHI->eraseFromParent();

  // Latch value uses require a concat of ShufI's sources. Where this is
  // inserted depends on whether the use is in a PHI or not.
  SmallVector<User *, 8u> LatchUsers(CV.Result->users());
  for (auto *LatchUse : LatchUsers) {
    if (auto PN = dyn_cast<PHINode>(LatchUse)) {
      IGC_ASSERT(PN->getNumIncomingValues() == 1);

      // Replace PN with PHIs for each of ShufI's sources, and replace
      // all uses with a concat of the new PHI's results.
      auto InsBeforeI = PN->getParent()->getFirstNonPHI();
      Instruction *ToConcat[2];
      for (unsigned i = 0; i < 2; ++i) {
        auto PHIPart = PHINode::Create(CV.Types[i], 1, "join", PN);
        PHIPart->addIncoming(CV.Parts[i], Latch);
        ToConcat[i] = PHIPart;
      }

      if (MaskPad) {
        ToConcat[1] = new ShuffleVectorInst(ToConcat[1], IGCLLVM::PoisonValue::get(ToConcat[1]->getType()), MaskPad);
        ToConcat[1]->insertBefore(InsBeforeI);
      }
      auto ConcatI = new ShuffleVectorInst(ToConcat[0], ToConcat[1], MaskJoin);
      ConcatI->insertBefore(InsBeforeI);

      SmallVector<User *, 8u> Users(PN->users());
      for (auto *U : Users)
        U->replaceUsesOfWith(PN, ConcatI);
      IGC_ASSERT(!PN->hasNUsesOrMore(1));
      PN->eraseFromParent();
    } else {
      // Insert a concat of ShufI's sources before the use
      // instruction.
      auto *I = cast<Instruction>(LatchUse);
      auto ConcatI = new ShuffleVectorInst(CV.Parts[0], CV.Parts[1], MaskJoin);
      ConcatI->insertBefore(I);
      I->replaceUsesOfWith(CV.Result, ConcatI);
    }
  }

  // Finally, we can delete CV.Result and any padding shuffles
  // we looked through to find the tail value.
  auto Tail = CV.Result->getOperand(1);
  IGC_ASSERT(!CV.Result->hasNUsesOrMore(1));
  CV.Result->eraseFromParent();

  if (Tail != CV.Parts[1]) {
    auto SVI = dyn_cast<ShuffleVectorInst>(Tail);
    IGC_ASSERT(SVI && SVI->isIdentityWithPadding());
    Tail = SVI->getOperand(0);
    IGC_ASSERT(!SVI->hasNUsesOrMore(1));
    SVI->eraseFromParent();
  }

  // Finally, add our new PHIs to the worklist, which we know
  // to satisfy the assumptions of worklist items.
  for (unsigned i = 0; i < 2; ++i)
    WL.push_back(NewPHI[i]);
  return true;
}

// The IR for catenating or extracting the parts of a CV
// may involve bitcasts to/from other vector types.
// In particular, we may have CVs where the shuffle instructions
// to join/extract the parts are on a different type than
// the PHI node type.
// This method takes a PHI, and a CV with a bitcast V -> V',
// where V is the type of the shuffle operations and V' is the type
// of the PHI.
// If every use of the PHI is as a bitcast V -> V', then
// we can eliminate the bitcasts and rewrite PHI to use V,
// and proceed to split further if possible.
//
// In principle we could eliminate any sequences of bitcasts
// with the same start and end types, or insert bitcasts
// to fix up uses of the PHI value as type V.
//
PHINode *LoopSplitWidePHIs::foldBitcasts(PHINode *PHI, CatenatedValue &CV, Loop *L) {
  // Check that any uses of the PHI are as a bitcast to the shuffle type.
  if (llvm::any_of(PHI->users(),
                   [&CV](const User *U) { return !isa<BitCastInst>(U) || U->getType() != CV.Result->getType(); }))
    return nullptr;

  // Ensure all uses of CV.Result can be rewritten to use the shuffle type.
  if (!hasOnlySimpleExternalUses(CV.Bitcast, L, PHI))
    return nullptr;

  BasicBlock *Latch = L->getLoopLatch();
  BasicBlock *Preheader = L->getLoopPreheader();

  // Create the new PHI of the shuffle type, and insert a bitcast
  // to the shuffle type for the preheader value.
  auto NewPHI = PHINode::Create(CV.Result->getType(), 2, "", PHI);
  auto PreValue = PHI->getIncomingValueForBlock(Preheader);
  auto PreValueBCI = new BitCastInst(PreValue, CV.Result->getType(), "", Preheader->getTerminator());
  NewPHI->addIncoming(PreValueBCI, Preheader);
  NewPHI->addIncoming(CV.Result, Latch);

  SmallVector<User *, 8u> PHIUsers(PHI->users());
  for (auto *PU : PHIUsers) {
    auto BCI = cast<BitCastInst>(PU);
    SmallVector<User *, 8u> Users(BCI->users());
    for (auto *U : Users)
      U->replaceUsesOfWith(BCI, NewPHI);
    IGC_ASSERT(!BCI->hasNUsesOrMore(1));
    BCI->eraseFromParent();
  }

  // The PHI value is now dead. We still need to eliminate the bitcast
  // defining the latch value, which may have other uses.
  IGC_ASSERT(!PHI->hasNUsesOrMore(1));
  PHI->eraseFromParent();

  SmallVector<User *, 8u> BCIUsers(CV.Bitcast->users());
  for (auto *U : BCIUsers) {
    // For a PHI use, we need to generate a new PHI of the shuffle type
    // and a bitcast back to the type of the use.
    // Otherwise, we just generate a bitcast to the use type in place.
    if (auto *PN = dyn_cast<PHINode>(U)) {
      auto InsBeforeI = PN->getParent()->getFirstNonPHI();
      auto NewPN = PHINode::Create(CV.Result->getType(), 1, "", PN);
      NewPN->addIncoming(CV.Result, Latch);
      auto BCI = new BitCastInst(NewPN, PN->getType(), "", InsBeforeI);

      SmallVector<User *, 8u> Users(PN->users());
      for (auto *U : Users)
        U->replaceUsesOfWith(PN, BCI);
      PN->eraseFromParent();
    } else {
      auto *I = cast<Instruction>(U);
      auto BCI = new BitCastInst(CV.Result, U->getType(), "", I);
      I->replaceUsesOfWith(CV.Bitcast, BCI);
    }
  }
  IGC_ASSERT(!CV.Bitcast->hasNUsesOrMore(1));
  CV.Bitcast->eraseFromParent();
  CV.Bitcast = nullptr;
  return NewPHI;
}

namespace IGC {
FunctionPass *createLoopSplitWidePHIs() { return new LoopSplitWidePHIs(); }
} // namespace IGC

// This pass pattern matches 1-BB loops with non-constant loop upper bound
// and memory accesses to alloca that is constant size array.
//
// Then if all alloca accesses are in bound we could create a new
// loop count with constant upper bound of alloca array size.
//
// Input Loop:
//
// loop.header:                                         ; preds = %preheader, %loop.header
//   %24 = phi i32 [ %29, %loop.header ], [ 0, %preheader ]
//   %25 = zext i32 %24 to i64
//   %26 = getelementptr inbounds [8 x %struct.Color], [8 x %struct.Color]* %10, i64 0, i64 %25, i32 0
//   store float 0.000000e+00, float* %26, align 4
//   ...
//   %29 = add nuw nsw i32 %24, 1
//   %30 = icmp slt i32 %29, %20
//   br i1 %30, label %loop.header, label %exit
//
// Transformed Loop:
//  Loop is split into head/ifcond/continue blocks, where the ifcond block is entered
//  by initial loop condition.
//
// loop.header:                                         ; preds = %preheader, %loop.continue
//   %newind = phi i32 [ %newind.next, %loop.continue ], [ 0, %preheader ]
//   %cond.phi = phi i1 [ %30, %loop.continue ], [ true, %preheader ]
//   %24 = phi i32 [ %29, %loop.continue ], [ 0, %preheader ]
//   br i1 %cond.phi, label %loop.cond, label %loop.continue
//
// loop.ifcond:
//   %26 = getelementptr inbounds [8 x %struct.Color], [8 x %struct.Color]* %10, i64 0, i64 %25, i32 0
//   store float 0.000000e+00, float* %26, align 4
//   ...
//   br label %loop.continue
//
// loop.continue:
//   %29 = add nuw nsw i32 %24, 1
//   %30 = icmp slt i32 %29, %20
//   %newind.next = add nuw nsw i32 %newind, 1
//   %newcmp = icmp slt i32 %newind.next, %allocasize
//   br i1 %newcmp, label %loop.header, label %exit
//
class LoopAllocaUpperbound : public llvm::LoopPass {
public:
  static char ID;

  LoopAllocaUpperbound();

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.addPreservedID(LCSSAID);
  }

  bool runOnLoop(Loop *L, LPPassManager &LPM);

  llvm::StringRef getPassName() const { return "IGC Loop Alloca Upperbound"; }

private:
};

#undef PASS_FLAG
#undef PASS_DESC
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS
#define PASS_FLAG "igc-loop-alloca-upperbound"
#define PASS_DESC "IGC Loop Alloca Upperbound"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LoopAllocaUpperbound, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(LoopAllocaUpperbound, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

char LoopAllocaUpperbound::ID = 0;

LoopAllocaUpperbound::LoopAllocaUpperbound() : LoopPass(ID) {
  initializeLoopAllocaUpperboundPass(*PassRegistry::getPassRegistry());
}

static const unsigned MaxAllocaSize = 32;

// Return the index variable for alloca LD/ST and its range
// null if LD/ST is not alloca address
static Value *getArrayIndex(const Instruction *I, unsigned &ArraySize) {
  const Value *Ptr = nullptr;
  if (auto LD = dyn_cast<LoadInst>(I))
    Ptr = LD->getPointerOperand();
  else if (auto ST = dyn_cast<StoreInst>(I))
    Ptr = ST->getPointerOperand();
  else
    return nullptr;

  // Processing GEPs sequence while not find alloca
  const GEPOperator *GEPOp = dyn_cast_or_null<GEPOperator>(Ptr);
  const AllocaInst *Alloca = nullptr;
  while (GEPOp && GEPOp->isInBounds()) {
    Ptr = GEPOp->getPointerOperand();
    Alloca = dyn_cast_or_null<AllocaInst>(Ptr);
    if (Alloca)
      break;
    GEPOp = dyn_cast_or_null<GEPOperator>(Ptr);
  }

  if (!Alloca)
    return nullptr;

  // Consider only simple case with one non-constant index
  if (GEPOp->countNonConstantIndices() != 1)
    return nullptr;

  // If alloca type is array then GEP operand corresponding to
  // array element is number 2
  Type *AllocaTy = GEPOp->getSourceElementType();
  if (AllocaTy->isArrayTy() && !isa<ConstantInt>(GEPOp->getOperand(2))) {
    ArraySize = int_cast<unsigned>(AllocaTy->getArrayNumElements());
    return GEPOp->getOperand(2);
  }
  return nullptr;
}

bool LoopAllocaUpperbound::runOnLoop(Loop *L, LPPassManager &LPM) {
  LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

  // Check that loop with single BB loop body
  if (!(L->isSafeToClone() && L->getNumBlocks() == 1 && L->getNumBackEdges() == 1 && L->getLoopPreheader())) {
    return false;
  }

  BasicBlock *Header = L->getHeader();
  BasicBlock *Incoming = nullptr, *Backedge = nullptr;
  if (!L->getIncomingAndBackEdge(Incoming, Backedge))
    return false;

  PHINode *InductionPhi = L->getCanonicalInductionVariable(); // Induction variable pre-increment
  if (!InductionPhi)
    return false;
  Instruction *InductionInc =
      dyn_cast<Instruction>(InductionPhi->getIncomingValueForBlock(Backedge)); // Induction increment
  // Check that induction increment has no other uses except induction phi and loop condition
  if (InductionInc->getNumUses() != 2)
    return false;
  ICmpInst *LoopCond = nullptr;     // The loop exit condition
  BranchInst *LoopBranch = nullptr; // The loop branching instruction
  Value *LoopSize = nullptr;        // Loop count

  // Match the loop exit condition and branch
  LoopBranch = dyn_cast<BranchInst>(Header->getTerminator());
  if (LoopBranch && LoopBranch->isConditional()) {
    LoopCond = dyn_cast<ICmpInst>(LoopBranch->getCondition());
    if (LoopCond && (LoopCond->getPredicate() == CmpInst::ICMP_SLT)) {
      if (LoopCond->getOperand(0) == InductionInc) {
        LoopSize = LoopCond->getOperand(1);
      }
    }
  }
  if (!LoopBranch || !LoopCond || !LoopSize)
    return false;

  // Do not apply to the Loops with constant loop count
  if (isa<ConstantInt>(LoopSize))
    return false;

  // Form ST/LD list
  std::vector<const Instruction *> MemRefList;
  for (const Instruction &I : *Header) {
    if (I.mayReadOrWriteMemory())
      MemRefList.push_back(&I);
  }

  // Find alloca array size
  unsigned ArraySize = std::numeric_limits<unsigned>::max();
  for (auto I : MemRefList) {
    unsigned CurrSize = 0;
    Instruction *IndexInst = dyn_cast_or_null<Instruction>(getArrayIndex(I, CurrSize));
    if (!IndexInst)
      continue;

    // Check that array index derives from inductive variable
    if (IndexInst != InductionPhi) {
      auto CInst = dyn_cast<CastInst>(IndexInst);
      if (!CInst || CInst->getOperand(0) != InductionPhi)
        continue;
    }
    ArraySize = std::min(ArraySize, CurrSize);
  }

  // Since loop transformation makes sense only in case when loop unroll could be applied
  // to transformed loop, we need to bound alloca array size we process
  if (ArraySize > MaxAllocaSize)
    return false;

  // We now have all the info to apply transformation
  Instruction *IfTerm;
  IRBuilder<> IRB(InductionPhi);
  PHINode *CondPHI = IRB.CreatePHI(LoopCond->getType(), 2, LoopCond->getName() + ".cond.phi");
  if (CondPHI) {
    CondPHI->addIncoming(ConstantInt::getTrue(LoopCond->getType()), Incoming);
    CondPHI->addIncoming(LoopCond, Backedge);
  }
  PHINode *NewInductionPHI = IRB.CreatePHI(InductionPhi->getType(), 2, LoopCond->getName() + ".newind.phi");
  IRB.SetInsertPoint(Header->getTerminator());
  Value *NewAdd = IRB.CreateAdd(NewInductionPHI, ConstantInt::get(InductionPhi->getType(), 1), ".newind.next");
  Value *NewCMP = IRB.CreateICmpSLT(NewAdd, ConstantInt::get(InductionPhi->getType(), ArraySize), ".newcmp");
  if (NewInductionPHI) {
    NewInductionPHI->addIncoming(ConstantInt::get(NewAdd->getType(), 0), Incoming);
    NewInductionPHI->addIncoming(NewAdd, Backedge);
  }
  // Split the Header into:
  //   Header
  //   |    |
  //   |   IfCondBB
  //   |    |
  //   |    |
  //   ContinueBB
  IfTerm = SplitBlockAndInsertIfThen(CondPHI, Header->getFirstNonPHIOrDbg(), false);
  BasicBlock *IfCondBB = IfTerm->getParent();
  BasicBlock *ContinueBB = IfCondBB->getNextNode();

  // Set the new block names
  IfCondBB->setName(Header->getName() + ".if.cond");
  ContinueBB->setName(Header->getName() + ".cont");

  // Add new blocks to the current loop
  L->addBasicBlockToLoop(IfCondBB, *LI);
  L->addBasicBlockToLoop(ContinueBB, *LI);

  // Move the instructions to IfCondBB block.
  Instruction *II = cast<Instruction>(ContinueBB->begin());
  while (II != NewAdd) {
    Instruction *CurrI = II;
    II = II->getNextNode();

    CurrI->moveBefore(IfTerm);
  }

  // Move old loop condition and induction increment to ContinueBB
  LoopCond->moveBefore(cast<Instruction>(NewAdd));
  InductionInc->moveBefore(cast<Instruction>(LoopCond));
  LoopBranch->replaceUsesOfWith(LoopCond, NewCMP);

  // Update Phi to preserve LCSSA form
  IRB.SetInsertPoint(InductionInc);
  for (Instruction &I : *Header) {
    auto *PI = dyn_cast<PHINode>(&I);
    if (PI) {
      if (PI == InductionPhi || PI == NewInductionPHI || PI == CondPHI)
        continue;
      PHINode *PN = IRB.CreatePHI(I.getType(), 2, PI->getName() + ".cont.phi");
      if (PN) {
        PI->replaceUsesOutsideBlock(PN, IfCondBB);
        PN->addIncoming(PI, Header);
        auto *U = PI->getIncomingValueForBlock(ContinueBB);
        U->replaceUsesOutsideBlock(PN, IfCondBB);
        PN->addIncoming(U, IfCondBB);
      }
    }
  }
  return true;
}

namespace IGC {
LoopPass *createLoopAllocaUpperbound() { return new LoopAllocaUpperbound(); }
} // namespace IGC
